/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015-2016 MUSE / Inria
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/
#pragma region Includes
#include "stdafx.h"
#include "HostViewService.h"
#include "ThreadPool.h"
#pragma endregion


CHostViewService::CHostViewService(PWSTR pszServiceName, BOOL fCanStop, BOOL fCanShutdown, BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	InitializeCriticalSection(&m_cs);

	m_fStopping = FALSE;
	m_fUserStopped = FALSE;
	m_dwLastSubmit = GetTickCount(); // delay next submit to now + interval
	m_dwRetryCount = 0;
	m_dwUserStoppedTime = 0;
	m_dwLastUpdateCheck = 0; // check as soon as possible

	m_fIdle = FALSE;
	m_fFullScreen = FALSE;

	// Create a manual-reset event that is not signaled at first to indicate 
	// the stopped signal of the service.
	m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hStoppedEvent == NULL)
	{
		throw GetLastError();
	}
}

CHostViewService::~CHostViewService(void)
{
	if (m_hStoppedEvent)
	{
		CloseHandle(m_hStoppedEvent);
		m_hStoppedEvent = NULL;
	}
	DeleteCriticalSection(&m_cs);
}

void CHostViewService::OnStart(DWORD dwArgc, PWSTR *pszArgv)
{
	srand((unsigned int)time(0));

	m_startTime = GetHiResTimestamp();

	Trace("Hostview service start.");

	WriteEventLogEntry(L"CHostViewService in OnStart", EVENTLOG_INFORMATION_TYPE);

	if (!m_store.Open()) {
		fprintf(stderr, "[SRV] fatal : failed to open the data store");
		exit(1);
	}

	QuerySystemInfo(m_sysInfo);
	if (_tcslen(m_sysInfo.hddSerial) <= 0)
	{
		fprintf(stderr, "[SRV] fatal : we don't know the hddSerial");
		exit(1);
	}
	sprintf_s(szHdd, "%S", m_sysInfo.hddSerial);

	// log session start + once per session data
	m_store.Insert(m_startTime, SessionEvent::Start);
	m_store.Insert(m_startTime, m_sysInfo);

	// make sure we don't have any pending pcaps in the capture folder
	CleanAllCaptureFiles();

	// Queue the main service function for execution in a worker thread.
	CThreadPool::QueueUserWorkItem(&CHostViewService::ServiceWorkerThread, this);

	StartServiceCommunication(*this);
	StartCollect();
}

void CHostViewService::ServiceWorkerThread(void)
{
	while (!m_fStopping)
	{
		Sleep(500);
		DWORD dwNow = GetTickCount();

		if (m_fUserStopped)
		{
			// auto-start in 30 minutes TODO: put this in settings;
			if (dwNow - m_dwUserStoppedTime >= m_settings.GetULong(AutoRestartTimeout))
			{
				// send self message to re-start capture;
				SendServiceMessage(Message(MessageStartCapture));
			}
			continue; // we're still stopped - do nothing else on this round
		}

		// Run different measurement actions if need
		ReadIpTable();
		ReadPerfTable();
		RunNetworkLabeling();
		RunQuestionnaireIfCase();
		
		// automatic submission
		if (dwNow - m_dwLastSubmit >= m_settings.GetULong(AutoSubmitInterval))
		{
			if (m_dwRetryCount > m_settings.GetULong(AutoSubmitRetryCount)) 
			{
				// failed x times in a row, mark as done for now and try again in full interval
				CheckDiskUsage();
				m_dwLastSubmit = dwNow;
				m_dwRetryCount = 0;
			}
			else if (DoSubmit(szHdd))
			{
				Trace("Data submission ok (retry count: %d)", m_dwRetryCount);
				m_dwLastSubmit = dwNow;
				m_dwRetryCount = 0;
			}
			else
			{
				// trick the last submit time so that we try again sooner
				m_dwRetryCount++;
				m_dwLastSubmit = dwNow - m_settings.GetULong(AutoSubmitInterval) + m_settings.GetULong(AutoSubmitRetryInterval);
				Trace("Data submission failed (retry count: %d)", m_dwRetryCount);
			}
		}

		// automatic update
		if (dwNow - m_dwLastUpdateCheck >= m_settings.GetULong(AutoUpdateInterval))
		{
			m_dwLastUpdateCheck = dwNow;

			if (CheckForUpdate())
			{
				TCHAR szUpdate[MAX_PATH] = {0};
				if (DownloadUpdate(szUpdate, _countof(szUpdate)))
				{
					// TODO: will be as system; what happens to UI?
					ShellExecute(NULL, L"open", szUpdate, 0, 0, SW_SHOW);
				}
			}
		}

		// update resources
		PullLatestResources(dwNow);

	} // end while

	// signal the stopped event;
	SetEvent(m_hStoppedEvent);
}

void CHostViewService::OnStop()
{
	WriteEventLogEntry(L"CHostViewService in OnStop", EVENTLOG_INFORMATION_TYPE);

	Trace("Hostview service stop.");

	// Indicate that the service is stopping and wait for the finish of the 
	// main service function (ServiceWorkerThread).
	m_fStopping = TRUE;
	if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	{
		throw GetLastError();
	}

	StopCollect();
	StopServiceCommunication();

	// clean up all data files and move to submit
	CleanAllCaptureFiles();
	Trace(0);
	m_store.Insert(GetHiResTimestamp(), SessionEvent::Stop);
	m_store.Close();
}

void CHostViewService::OnShutdown()
{
	OnStop();
}

void CHostViewService::StartCollect()
{
	StartInterfacesMonitor(*this, m_settings.GetULong(PcapSizeLimit), m_settings.GetULong(InterfaceMonitorTimeout));
	StartWifiMonitor(*this, m_settings.GetULong(WirelessMonitorTimeout));
	StartBatteryMonitor(*this, m_settings.GetULong(BatteryMonitorTimeout));
	StartHttpDispatcher(*this);
}

void CHostViewService::StopCollect()
{
	StopHttpDispatcher();
	StopBatteryMonitor();
	StopWifiMonitor();
	StopInterfacesMonitor();
	StopAllCaptures();
}

void CHostViewService::ReadIpTable()
{
	static DWORD dwIpLastWrite = GetTickCount();
	DWORD dwNow = GetTickCount();

	if (dwNow - dwIpLastWrite >= m_settings.GetULong(SocketStatsTimeout))
	{
		IpTable *table = GetIpTable();
		ULONGLONG timestamp = GetHiResTimestamp();

		const char * udp = "UDP";
		const char * tcp = "TCP";
		for (size_t i = 0; i < table->size; i ++)
		{
			IpRow it = table->get(i);
			m_store.Insert(it.pid, it.name, it.IsUDP() ? IPPROTO_UDP : IPPROTO_TCP, it.srcIp, it.destIp, it.srcport, it.destport, it.state, timestamp);
		}

		ReleaseIpTable(table);
		dwIpLastWrite = dwNow;
	}
}

void CHostViewService::ReadPerfTable()
{
	static DWORD dwPerfLastWrite = GetTickCount();
	DWORD dwNow = GetTickCount();

	if (dwNow - dwPerfLastWrite >= m_settings.GetULong(SystemStatsTimeout))
	{
		PerfTable *table = GetPerfTable();
		ULONGLONG timestamp = GetHiResTimestamp();

		for (size_t i = 0; i < table->size; i ++)
		{
			PerfRow it = table->get(i);
			m_store.Insert(it.pid, it.name, it.memory, it.cpu, timestamp);
		}

		ReleasePerfTable(table);
		dwPerfLastWrite = dwNow;
	}
}

// interfaces monitor
void CHostViewService::LogNetwork(const NetworkInterface &ni, __int64 timestamp, bool connected)
{
	if (connected)
	{
		LabelNetwork(ni);
	}

	TCHAR szGateways[MAX_PATH] = {0};
	for (size_t i = 0; i < ni.gateways.size(); i ++)
	{
		_tcscat_s(szGateways, ni.gateways[i].c_str());

		if (i < ni.gateways.size() - 1)
		{
			_tcscat_s(szGateways, _T(","));
		}
	}
	TCHAR szDnses[MAX_PATH] = {0};
	for (size_t i = 0; i < ni.dnses.size(); i ++)
	{
		_tcscat_s(szDnses, ni.dnses[i].c_str());

		if (i < ni.dnses.size() - 1)
		{
			_tcscat_s(szDnses, _T(","));
		}
	}
	TCHAR szIps[MAX_PATH] = {0};
	for (size_t i = 0; i < ni.ips.size(); i ++)
	{
		_tcscat_s(szIps, ni.ips[i].c_str());

		if (i < ni.ips.size() - 1)
		{
			_tcscat_s(szIps, _T(","));
		}
	}

	m_store.Insert(ni.strName.c_str(), ni.strFriendlyName.c_str(), ni.strDescription.c_str(), ni.strDnsSuffix.c_str(),
		ni.strMac.c_str(), szIps, szGateways, szDnses, ni.tSpeed, ni.rSpeed, ni.wireless, ni.strProfile.c_str(),
		ni.strSSID.c_str(), ni.strBSSID.c_str(), ni.strBSSIDType.c_str(), ni.strPHYType.c_str(), ni.phyIndex,
		ni.channel, ni.rssi, ni.signal, connected, timestamp);
}

void CHostViewService::OnInterfaceConnected(const NetworkInterface &networkInterface)
{
	ULONGLONG timestamp = GetHiResTimestamp();

	std::vector<std::string> * pInfo = 0;
	if (QueryPublicInfo(pInfo) && pInfo->size() > 7)
	{
		m_store.Insert((*pInfo)[0].c_str(), (*pInfo)[1].c_str(), (*pInfo)[2].c_str(), (*pInfo)[3].c_str(),
			(*pInfo)[4].c_str(), (*pInfo)[5].c_str(), (*pInfo)[6].c_str(), (*pInfo)[7].c_str(), timestamp);
	}

	FreePublicInfo(pInfo);

	LogNetwork(networkInterface, timestamp, true);
	StartCapture(*this, m_startTime, timestamp, networkInterface.strName.c_str());
}

void CHostViewService::OnInterfaceDisconnected(const NetworkInterface &networkInterface)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	StopCapture(networkInterface.strName.c_str());
	LogNetwork(networkInterface, timestamp, false);
}

void CHostViewService::LabelNetwork(const NetworkInterface &ni)
{
	if (m_settings.GetULong(NetLabellingActive))
	{
		if (!m_networks.IsKnown(ni) && ni.strBSSID.length() > 0)
		{
			EnterCriticalSection(&m_cs);
			m_interfacesQueue.push_back(ni);
			LeaveCriticalSection(&m_cs);
		}
	}
}

void CHostViewService::RunNetworkLabeling() {
	if (m_interfacesQueue.size() > 0) {
		NetworkInterface ni = m_interfacesQueue[0];
		TCHAR szCmdLine[MAX_PATH] = { 0 };

		ImpersonateCurrentUser();
		NetworkInterfaceToCommand(ni, szCmdLine, _countof(szCmdLine));
		ImpersonateRevert();

		if (RunAsCurrentUser(szCmdLine))
		{
			EnterCriticalSection(&m_cs);
			m_interfacesQueue.erase(m_interfacesQueue.begin());
			LeaveCriticalSection(&m_cs);
		}
	}
}

void CHostViewService::OnWifiStats(const std::tstring &interfaceGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.Insert(interfaceGuid.c_str(), tSpeed, rSpeed, signal, rssi, state, timestamp);
}

void CHostViewService::OnBatteryStats(byte status, byte percent)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.Insert(status, percent, timestamp);
}

void CHostViewService::OnDNSAnswer(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.Insert(type, szIp, szHost, protocol, szSrc, szDest, srcport, destport, timestamp);
}

void CHostViewService::OnHttpHeaders(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
	char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.Insert(szVerb, szVerbParam, szStatusCode, szHost, szReferer, szContentType, szContentLength, protocol, szSrc, szDest,
		srcport, destport, timestamp);
}


void CHostViewService::RunQuestionnaireIfCase()
{
	static DWORD sdwLastCheck = 0;

	if (m_settings.GetULong(QuestionnaireActive)) {
		DWORD dwNow = GetTickCount();
		// check once per interval (and when the user is there)
		if (dwNow - sdwLastCheck >= m_settings.GetULong(EsmCoinFlipInterval) && !m_fIdle && !m_fFullScreen)
		{
			sdwLastCheck = dwNow;

			// enforce max shows per day
			if (QueryQuestionnaireCounter() < m_settings.GetULong(EsmMaxShows))
			{
				// pick random number between [0,100) and compare to the selected probability
				if ((double) rand() / (RAND_MAX + 1) * 100.0 <= 1.0*m_settings.GetULong(EsmCoinFlipProb))
				{
					ShowQuestionnaireUI(FALSE);
				}
			}
		}
	}
}

void CHostViewService::PullLatestResources(DWORD dwNow)
{
	static BOOL fUpdated = FALSE;
	if (fUpdated)
		return;

	// if 10 minutes after system restart, 1 time update
	if (dwNow > m_settings.GetULong(AutoUpdateInterval))
	{
		// TODO: replace auto-update with this:
		// TODO: - pull md5 first => pull entire file if different
		// TODO: - pull recursively (e.g. md5 for entire folder, etc.)
		PullFile(_T("settings"));
		PullFile(_T("html/esm_wizard.html"));
		PullFile(_T("html/network_wizard.html"));
		fUpdated = TRUE;
	}
}

bool CHostViewService::ShowQuestionnaireUI(BOOL fOnDemand /*= TRUE*/)
{
	TCHAR szUser[MAX_PATH] = {0};
	if (GetLoggedOnUser(szUser, _countof(szUser)))
	{
		TCHAR szCmdLine[MAX_PATH] = {0};
				
		ImpersonateCurrentUser();
		GenerateQuestionnaireCommand(szUser, fOnDemand, szCmdLine, _countof(szCmdLine));
		ImpersonateRevert();

		RunAsCurrentUser(szCmdLine);

		Trace("Questionnaire shown [on demand = %d].", fOnDemand);

		return true;
	}

	return false;
}

void CHostViewService::UpdateQuestionnaireAppUsageInfo(DWORD dwPid, TCHAR *szUser, TCHAR *szApp)
{
	if (!IsProductProcess(dwPid) && _tcsicmp(szApp, _T("explorer.exe")) != 0)
	{
		TCHAR szDescription[MAX_PATH] = {0};
		GetProcessDescription(dwPid, szDescription, _countof(szDescription));

		// TODO: elevated should be once
		if (dwPid > 0 && _tcscmp(szApp, _T("<elevated>")) && _tcslen(szApp))
		{
			UpdateQuestionnaireAppsList(szUser, szApp, szDescription);

			_stprintf_s(szDescription, _T("html\\out\\%s.ico"), szApp);
			QueryProcessIcon(dwPid, szDescription);
		}
	}
}

void CHostViewService::UpdateQuestionnaireSiteUsageInfo(TCHAR *szLocation, TCHAR *szBrowser)
{
	TCHAR szUser[MAX_PATH] = {0};
	GetLoggedOnUser(szUser, _countof(szUser));

	if (szLocation && szBrowser)
	{
		TCHAR szBrowserExe[MAX_PATH] = {0};
		_stprintf_s(szBrowserExe, _T("%s.exe"), szBrowser);

		UpdateQuestionnaireTabsList(szUser, szLocation, szBrowserExe);

		TCHAR szRemote[MAX_PATH] = {0}, szLocal[MAX_PATH] = {0};

		_stprintf_s(szLocal, _T("html\\out\\%s.ico"), szLocation);

		if (GetFileAttributes(szLocal) == INVALID_FILE_ATTRIBUTES)
		{
			_stprintf_s(szRemote, _T("http://%s/favicon.ico"), szLocation);
			DownloadFile(szRemote, szLocal);
		}
	}
}

void CHostViewService::PullInstalledApps(TCHAR *szPath, DWORD dwSize)
{
	TCHAR szCurrDir[MAX_PATH] =  {0};
	GetCurrentDirectory(_countof(szCurrDir), szCurrDir);

	TCHAR szOut[MAX_PATH] = {0};
	_stprintf_s(szPath, dwSize, _T("%s\\%s\\%d"), szCurrDir, TEMP_PATH, GetTickCount());

	TCHAR ** pszApps = 0, ** pszDesc = 0;
	int nCount = QueryApplicationsList(pszApps, pszDesc);

	ImpersonateCurrentUser();

	FILE *f = 0;
	_tfopen_s(&f, szPath, _T("w"));

	if (f)
	{
		for (int i = 0; i < nCount; i ++)
		{
			TCHAR *szName = PathFindFileName(pszApps[i]);

			_ftprintf_s(f, _T("%s\n"), szName);
			_ftprintf_s(f, _T("%s\n"), pszDesc[i]);

			// the extra line (not used)
			_ftprintf_s(f, _T("\n"));
		}

		fclose(f);
	}

	ImpersonateRevert();

	// prepare app list
	m_appList.clear();
	for (int i = 0; i < nCount; i ++)
	{
		m_appList.push_back(pszApps[i]);
	}

	// spawn thread
	CloseHandle(CreateThread(NULL, NULL, QueryIconsThreadProc, this, NULL, NULL));
	ReleaseApplicationsList(nCount, pszApps, pszDesc);
}

DWORD WINAPI CHostViewService::QueryIconsThreadProc(LPVOID lpParameter)
{
	CHostViewService *pThis = (CHostViewService *) lpParameter;
	pThis->QueryIconsThreadFunc();
	return 0L;
}

void CHostViewService::QueryIconsThreadFunc()
{
	TCHAR szOut[MAX_PATH] = {0};

	// query the app icons
	for (size_t i = 0; i < m_appList.size(); i ++)
	{
		TCHAR *szName = PathFindFileName(m_appList[i].c_str());
		_stprintf_s(szOut, _T("html\\out\\%s.ico"), szName);
		QueryImageIcon((TCHAR *) m_appList[i].c_str(), szOut);
	}
}

bool CHostViewService::OnBrowserLocationUpdate(TCHAR *location, TCHAR *browser)
{
	if (location && browser)
	{
		m_store.Insert(browser, location, GetHiResTimestamp());
		UpdateQuestionnaireSiteUsageInfo(location, browser);
		return true;
	}
	return false;
}

bool CHostViewService::OnJsonUpload(const char *jsonbuf, size_t len) {
	if (jsonbuf && len > 0) {
		m_store.Insert(GetHiResTimestamp(), jsonbuf, len);
		return true;
	}
	return false;
}

Message CHostViewService::OnMessage(Message &message)
{
	switch (message.type)
	{
	case MessageStartCapture:
		if (m_fUserStopped)
		{
			m_fUserStopped = FALSE;
			StartCollect();

			Trace("Capture re-started.");
			m_store.Insert(GetHiResTimestamp(), SessionEvent::Restart);
		}
		break;
	case MessageStopCapture:
		if (!m_fUserStopped)
		{
			m_fUserStopped = TRUE;
			m_dwUserStoppedTime = GetTickCount();
			StopCollect();

			Trace("Capture paused.");
			m_store.Insert(GetHiResTimestamp(), SessionEvent::Pause);
		}
		break;
	case MessageSpeakerUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.Insert(IoDevice::Speaker, message.dwPid, szApp, GetHiResTimestamp());
		}
		break;
	case MessageMicrophoneUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.Insert(IoDevice::Microphone, message.dwPid, szApp, GetHiResTimestamp());
		}
		break;
	case MessageCameraUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.Insert(IoDevice::Camera, message.dwPid, szApp, GetHiResTimestamp());
		}
		break;
	case MessageMouseKeyUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			// TODO: hacky! better encapsulation of messages
			if (message.isFullScreen)
			{
				m_store.Insert(IoDevice::Mouse, message.dwPid, szApp, GetHiResTimestamp());
			}
			if (message.isIdle)
			{
				m_store.Insert(IoDevice::Keyboard, message.dwPid, szApp, GetHiResTimestamp());
			}

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
		}
		break;
	case MessageUserActivity:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			TCHAR szDescription[MAX_PATH] = {0};

			GetProcessName(message.dwPid, szApp, _countof(szApp));
			GetProcessDescription(message.dwPid, szDescription, _countof(szDescription));

			m_store.Insert(message.szUser, message.dwPid, szApp, szDescription, message.isFullScreen, message.isIdle, GetHiResTimestamp());

			// TODO: multiple users
			m_fIdle = message.isIdle ? TRUE : FALSE;
			m_fFullScreen = message.isFullScreen ? TRUE : FALSE;

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
		}
		break;
	case MessageQueryStatus:
		{
			Message result(m_fUserStopped ? MessageStopCapture : MessageStartCapture);
			return result;
		}
		break;
	case MessageNetworkLabel:
		{
			m_networks.OnNetworkLabel(message);
			Trace("Network labelled.");
		}
		break;
	case MessageQuestionnaireDone:
		{
			char uFile[MAX_PATH] = {};
			sprintf_s(uFile, "%S", message.szUser);
			MoveFileToSubmit(uFile, true);

			SetQuestionnaireCounter(QueryQuestionnaireCounter() + 1);
			Trace("Questionnaire done.");
		}
		break;
	case MessageQuestionnaireShow:
		{
			ShowQuestionnaireUI();
		}
		break;
	case MessageQueryLastApps:
		{
			TCHAR szFile[MAX_PATH] = {0};

			ImpersonateCurrentUser();
			SaveAppsList(message.szUser, szFile, _countof(szFile));
			ImpersonateRevert();

			Message result(0, szFile, 0, true, true);
			return result;
		}
		break;
	case MessageQueryInstalledApps:
		{
			TCHAR szFile[MAX_PATH] = {0};
			PullInstalledApps(szFile, _countof(szFile));

			return Message(0, szFile, 0, true, true);
		}
		break;
	}
	return message;
}
