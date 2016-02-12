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
	m_dwUserStoppedTime = 0;
	m_dwLastSubmit = 0;
	m_dwLastUpdateCheck = 0;
	m_fIdle = FALSE;
	m_fFullScreen = FALSE;
	m_startTime = 0;
	m_hasSeenUI = FALSE;

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

	Trace("Hostview service start.");
	WriteEventLogEntry(L"CHostViewService in OnStart", EVENTLOG_INFORMATION_TYPE);

	// Queue the main service function for execution in a worker thread.
	CThreadPool::QueueUserWorkItem(&CHostViewService::ServiceWorkerThread, this);

	StartServiceCommunication(*this);
	StartCollect(SessionEvent::Start, GetHiResTimestamp());
}

void CHostViewService::ServiceWorkerThread(void)
{
	DWORD dwNow = GetTickCount();

	// next upload will take place after configured interval
	m_dwLastSubmit = dwNow;
	CUpload *pendingUpload = NULL; // current upload session

	// next update check will take place 5min after service start
	m_dwLastUpdateCheck = dwNow - m_settings.GetULong(AutoUpdateInterval) + 5 * 60 * 1000;

	// session autorotate info
	DWORD lastSessionCheck = dwNow;
	WORD lastRotateDay = 0;

	while (!m_fStopping)
	{
		Sleep(500);
		dwNow = GetTickCount();

		if (m_fUserStopped)
		{
			if (dwNow - m_dwUserStoppedTime >= m_settings.GetULong(AutoRestartTimeout))
			{
				// send self message to re-start capture
				SendServiceMessage(Message(MessageStartCapture));
			}

			continue; // we're stopped
		}

		// Run different measurement actions
		ReadIpTable();
		ReadPerfTable();
		RunNetworkLabeling();
		RunQuestionnaireIfCase();
		
		// automatic submission
		if (dwNow - m_dwLastSubmit >= m_settings.GetULong(AutoSubmitInterval))
		{
			if (pendingUpload == NULL) {
				// new upload session
				pendingUpload = new CUpload();
			}

			if (!pendingUpload->TrySubmit(szHdd)) {
				// no need to retry - either this was a success or then max retries was reached
				delete pendingUpload;
				pendingUpload = NULL;
				m_dwLastSubmit = dwNow;
			}
		}

		// autoupdates
		if (dwNow - m_dwLastUpdateCheck >= m_settings.GetULong(AutoUpdateInterval))
		{
			m_dwLastUpdateCheck = dwNow;

			// HostView update 
			if (CheckForUpdate())
			{
				Trace("Hostview update available.");
				TCHAR szUpdate[MAX_PATH] = {0};
				if (DownloadUpdate(szUpdate, _countof(szUpdate)))
				{
					Trace("Hostview going to update.");
					// TODO: will be as system; what happens to UI?
					ShellExecute(NULL, L"open", szUpdate, 0, 0, SW_SHOW);
					continue; // or break ?
				}
			}

			// UI + settings
			CheckForResourceUpdates();
		}

		// check for max session duration
		if (dwNow - lastSessionCheck > 3600000) {
			SYSTEMTIME st;
			GetLocalTime(&st);

			// NOTE: this will happen (by desing) only if the computer happens to run 
			// at night (e.g. a pc that is on all the time), we except sessions 
			// mostly to follow computer sleep cycles (see suspend+resume). This
			// is here just to ensure some hard limit on the duration of a single session.
			if (st.wHour >= 3 && st.wHour <= 5 && (dwNow - m_startTime >= 21*3600*1000)) {
				SendServiceMessage(Message(MessageRestartSession));
			}

			lastSessionCheck = dwNow;
		}
	} // end while

	// signal the stopped event;
	SetEvent(m_hStoppedEvent);
}

void CHostViewService::OnStop()
{
	if (!m_fStopping) {
		m_fStopping = TRUE;

		WriteEventLogEntry(L"CHostViewService in OnStop", EVENTLOG_INFORMATION_TYPE);

		// wait for the main thread
		if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
		{
			throw GetLastError();
		}

		if (!m_fUserStopped)
			StopCollect(SessionEvent::Stop, GetHiResTimestamp());
		Trace("Hostview service stop.");
		Trace(0);

		StopServiceCommunication();
	}
}

void CHostViewService::OnShutdown()
{
	OnStop();
}

void CHostViewService::OnSuspend()
{
	Trace("Hostview service suspend.");
	if (!m_fUserStopped) {
		StopCollect(SessionEvent::Suspend, GetHiResTimestamp());
	}
	m_fUserStopped = FALSE;
}

void CHostViewService::OnResume()
{
	Trace("Hostview service resume.");
	StartCollect(SessionEvent::Resume, GetHiResTimestamp());
}	

void CHostViewService::StartCollect(SessionEvent e, ULONGLONG ts)
{
	if (m_startTime == 0) {
		// new session id
		m_startTime = ts;

		// make sure we are using the latest settings
		m_settings.ReloadSettings();

		Trace("Hostview service session %llu start [reason=%d].", ts, e);

		// get sysinfo (data that wont change during the session)
		QuerySystemInfo(m_sysInfo);
		if (_tcslen(m_sysInfo.hddSerial) <= 0)
		{
			Debug("[SRV] fatal : we don't know the hddSerial");
			return;
		}
		sprintf_s(szHdd, "%S", m_sysInfo.hddSerial);

		// make sure we don't have any pending pcaps in the capture folder
		CleanAllCaptureFiles(m_settings.GetBoolean(DebugMode));

		if (!m_store.Open(m_startTime)) {
			Debug("[SRV] fatal : failed to open the data store");
			return;
		}

		// log session start + once per session data
		m_store.InsertSession(m_startTime, e);
		m_store.InsertSys(m_startTime, m_sysInfo, ProductVersionStr, m_settings.GetVersion());

		// start data collection
		StartInterfacesMonitor(*this, m_settings.GetULong(PcapSizeLimit), m_settings.GetBoolean(DebugMode));
		StartWifiMonitor(*this, m_settings.GetULong(WirelessMonitorTimeout));
		StartHttpDispatcher(*this);
	}
}

void CHostViewService::StopCollect(SessionEvent e, ULONGLONG ts)
{
	if (m_startTime > 0) {
		Trace("Hostview service session %llu stop [reason=%d].", m_startTime, e);

		m_store.InsertSession(ts, e);
		m_store.Close();
		m_startTime = 0;
	}

	StopHttpDispatcher();
	StopWifiMonitor();
	StopInterfacesMonitor(*this);
}

void CHostViewService::OnPowerEvent(PPOWERBROADCAST_SETTING event)
{
	if (!m_fUserStopped) {
		if (IsEqualGUID(event->PowerSetting, GUID_BATTERY_PERCENTAGE_REMAINING)) {
			m_store.InsertPowerState(GetHiResTimestamp(), "battery", event->Data[0]);
		}
		else if (IsEqualGUID(event->PowerSetting, GUID_ACDC_POWER_SOURCE)) {
			m_store.InsertPowerState(GetHiResTimestamp(), "powersource", event->Data[0]);
		}
		else if (IsEqualGUID(event->PowerSetting, GUID_GLOBAL_USER_PRESENCE)) {
			m_store.InsertPowerState(GetHiResTimestamp(), "userpresence", event->Data[0]);
		}
		else if (IsEqualGUID(event->PowerSetting, GUID_CONSOLE_DISPLAY_STATE)) {
			m_store.InsertPowerState(GetHiResTimestamp(), "display", event->Data[0]);
		}
		else if (IsEqualGUID(event->PowerSetting, GUID_MONITOR_POWER_ON)) {
			m_store.InsertPowerState(GetHiResTimestamp(), "monitor", event->Data[0]);
		}
	}
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
			m_store.InsertPort(it.pid, it.name, it.IsUDP() ? IPPROTO_UDP : IPPROTO_TCP, it.srcIp, it.destIp, it.srcport, it.destport, it.state, timestamp);
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
			m_store.InsertProc(it.pid, it.name, it.memory, it.cpu, timestamp);
		}

		ReleasePerfTable(table);
		dwPerfLastWrite = dwNow;
	}
}

void CHostViewService::LogNetwork(const NetworkInterface &ni, ULONGLONG timestamp, bool connected)
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

	m_store.InsertConn(ni.strGuid.c_str(), ni.strFriendlyName.c_str(), ni.strDescription.c_str(), ni.strDnsSuffix.c_str(),
		ni.strMac.c_str(), szIps, szGateways, szDnses, ni.tSpeed, ni.rSpeed, ni.wireless, ni.strProfile.c_str(),
		ni.strSSID.c_str(), ni.strBSSID.c_str(), ni.strBSSIDType.c_str(), ni.strPHYType.c_str(), ni.phyIndex,
		ni.channel, connected, timestamp);
}

void CHostViewService::OnInterfaceConnected(const NetworkInterface &networkInterface)
{
	ULONGLONG timestamp = GetHiResTimestamp();

	LogNetwork(networkInterface, timestamp, true);
	StartCapture(*this, m_startTime, timestamp, m_settings.GetBoolean(DebugMode), networkInterface.strGuid.c_str());

	// FIXME: this could be a bit more intelligent - not sure we need to query this stuff
	// every time an interface goes up .. ?
	std::vector<std::string> * pInfo = 0;
	if (QueryPublicInfo(pInfo) && pInfo->size() > 7)
	{
		m_store.InsertLoc(networkInterface.strGuid.c_str(), (*pInfo)[0].c_str(), (*pInfo)[1].c_str(), (*pInfo)[2].c_str(), (*pInfo)[3].c_str(),
			(*pInfo)[4].c_str(), (*pInfo)[5].c_str(), (*pInfo)[6].c_str(), (*pInfo)[7].c_str(), timestamp);
	}
	FreePublicInfo(pInfo);
}

void CHostViewService::OnInterfaceDisconnected(const NetworkInterface &networkInterface)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	StopCapture(networkInterface.strGuid.c_str());
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
	// will only ask for labels once we have seen some sign of the HostView UI
	if (m_hasSeenUI && m_interfacesQueue.size() > 0) {
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

void CHostViewService::OnWifiStats(const char *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.InsertWifi(szGuid, tSpeed, rSpeed, signal, rssi, state, timestamp);
}

void CHostViewService::OnDNSAnswer(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.InsertDns(connection, type, szIp, szHost, protocol, szSrc, szDest, srcport, destport, timestamp);
}

void CHostViewService::OnHttpHeaders(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
	char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength)
{
	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.InsertHttp(connection, szVerb, szVerbParam, szStatusCode, szHost, szReferer, szContentType, szContentLength, protocol, szSrc, szDest,
		srcport, destport, timestamp);
}

void CHostViewService::RunQuestionnaireIfCase()
{
	static DWORD sdwLastCheck = 0;

	if (m_settings.GetBoolean(EsmActive)) {
		DWORD dwNow = GetTickCount();
		if (sdwLastCheck == 0) {
			// don't check right away on startup but pretend the last check was almost an interval ago
			sdwLastCheck = dwNow - m_settings.GetULong(EsmCoinFlipInterval) + m_settings.GetULong(EsmCoinFlipInterval)/8;
		}

		// check once per interval (if the user is there)
		if (dwNow - sdwLastCheck >= m_settings.GetULong(EsmCoinFlipInterval) && !m_fIdle && !m_fFullScreen)
		{
			sdwLastCheck = dwNow;

			// enforce max shows per day and start + end hours
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
	if (location && browser && !m_fUserStopped)
	{
		m_store.InsertBrowser(browser, location, GetHiResTimestamp());
		UpdateQuestionnaireSiteUsageInfo(location, browser);
		return true;
	}
	return false;
}

bool CHostViewService::OnJsonUpload(char **jsonbuf, size_t len) {
	if (jsonbuf && len > 0 && !m_fUserStopped) {
		char fn[MAX_PATH];
		sprintf_s(fn, "%S\\%llu_%llu_browserupload.json", TEMP_PATH, m_startTime, GetHiResTimestamp());
		FILE * f = NULL;
		fopen_s(&f, fn, "w");
		if (f)
		{
			fprintf(f, "%s", *jsonbuf);
			fclose(f);
		}
		MoveFileToSubmit(fn, m_settings.GetBoolean(DebugMode));
		return true;
	}
	return false;
}

Message CHostViewService::OnMessage(Message &message)
{
	switch (message.type)
	{
	case MessageRestartSession:
		if (!m_fUserStopped)
		{
			ULONGLONG ts = GetHiResTimestamp();
			Trace("Hostview service session autostop");
			StopCollect(SessionEvent::AutoStop, ts);
			Trace("Hostview service session autostart");
			StartCollect(SessionEvent::AutoStart, ts);
		}
		break;
	case MessageSuspend:
		OnSuspend();
		break;
	case MessageResume:
		OnResume();
		break;
	case MessageStartCapture:
		if (m_fUserStopped)
		{
			m_fUserStopped = FALSE;
			StartCollect(SessionEvent::Restart, GetHiResTimestamp());
		}
		break;
	case MessageStopCapture:
		if (!m_fUserStopped)
		{
			m_fUserStopped = TRUE;
			m_dwUserStoppedTime = GetTickCount();
			StopCollect(SessionEvent::Pause, GetHiResTimestamp());
		}
		break;
	case MessageSpeakerUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.InsertIo(IoDevice::Speaker, message.dwPid, szApp, GetHiResTimestamp());
		}
		break;
	case MessageMicrophoneUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.InsertIo(IoDevice::Microphone, message.dwPid, szApp, GetHiResTimestamp());
		}
		break;
	case MessageCameraUsage:
		if (!m_fUserStopped)
		{
			TCHAR szApp[MAX_PATH] = {0};
			GetProcessName(message.dwPid, szApp, _countof(szApp));

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
			m_store.InsertIo(IoDevice::Camera, message.dwPid, szApp, GetHiResTimestamp());
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
				m_store.InsertIo(IoDevice::Mouse, message.dwPid, szApp, GetHiResTimestamp());
			}
			if (message.isIdle)
			{
				m_store.InsertIo(IoDevice::Keyboard, message.dwPid, szApp, GetHiResTimestamp());
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

			m_store.InsertActivity(message.szUser, message.dwPid, szApp, szDescription, message.isFullScreen, message.isIdle, GetHiResTimestamp());

			// TODO: multiple users
			m_fIdle = message.isIdle ? TRUE : FALSE;
			m_fFullScreen = message.isFullScreen ? TRUE : FALSE;

			UpdateQuestionnaireAppUsageInfo(message.dwPid, message.szUser, szApp);
		}
		break;
	case MessageQueryStatus:
		{
			m_hasSeenUI = TRUE;
			Message result(m_fUserStopped ? MessageStopCapture : MessageStartCapture);
			return result;
		}
		break;
	case MessageNetworkLabel:
		{
			Trace("New network label [%S].", message.szUser);

			// FIXME: the messages should really have a more versatile format, this is ugly ...
			TCHAR szGUID[MAX_PATH] = { 0 };
			TCHAR szBSSID[MAX_PATH] = { 0 };
			TCHAR szLabel[MAX_PATH] = { 0 };
			TCHAR *szCtx = NULL;
			TCHAR *szToken = _tcstok_s(message.szUser, _T(","), &szCtx);
			if (szToken)
			{
				_tcscpy_s(szGUID, szToken);
				szToken = _tcstok_s(NULL, _T(","), &szCtx);
				if (szToken)
				{
					_tcscpy_s(szBSSID, szToken);
					szToken = _tcstok_s(NULL, _T(","), &szCtx);
					if (szToken)
					{
						_tcscpy_s(szLabel, szToken);
					}
				}
			}
			// remember labeled networks
			m_networks.OnNetworkLabel(szGUID, szBSSID, szLabel);
			// store for upload
			m_store.InsertNetLabel(GetHiResTimestamp(), szGUID, szBSSID, szLabel);
		}
		break;
	case MessageQuestionnaireDone:
		{
			Trace("Questionnaire done.");

			// questionnaire results
			char uFile[MAX_PATH] = {};
			sprintf_s(uFile, "%S", message.szUser);

			// rename with session id
			char uploadfile[MAX_PATH] = { 0 };
			sprintf_s(uploadfile, "%llu_%s", m_startTime, uFile);
			MoveFileA(uFile, uploadfile);

			MoveFileToSubmit(uploadfile, m_settings.GetBoolean(DebugMode));

			SetQuestionnaireCounter(QueryQuestionnaireCounter() + 1);
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
