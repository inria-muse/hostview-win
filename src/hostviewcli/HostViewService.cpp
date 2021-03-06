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

#include <atlconv.h>

CHostViewService::CHostViewService(PWSTR pszServiceName, BOOL fCanStop, BOOL fCanShutdown, BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	InitializeCriticalSection(&m_cs);

	m_fStopping = FALSE;
	m_fUserStopped = FALSE;
	m_dwUserStoppedTime = 0;
	m_fIdle = FALSE;
	m_fFullScreen = FALSE;
	m_startTime = 0;
	m_hasSeenUI = FALSE;
	m_upload = NULL;
	m_fUpdatePending = FALSE;

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
	if (m_upload != NULL)
		delete m_upload;
	DeleteCriticalSection(&m_cs);
}

void CHostViewService::OnStart(DWORD dwArgc, PWSTR *pszArgv)
{
	srand((unsigned int)time(0));

	// new trace
	Trace(0);
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

	// session autorotate info
	DWORD lastSessionCheck = dwNow;
	WORD lastRotateDay = 0;

	while (!m_fStopping)
	{
		Sleep(500);
		dwNow = GetTickCount();

		if (m_fUpdatePending) {
			Debug("ServiceWorkerThread m_fUpdatePending");
			continue; // will stop soon
		}
		if (m_fUserStopped)
		{
			Debug("ServiceWorkerThread m_fUserStopped %d", m_fUserStopped);
			if (dwNow - m_dwUserStoppedTime >= m_settings.GetULong(AutoRestartTimeout))
			{
				// autorestart
				SendServiceMessage(Message(MessageStartCapture));
			}

			continue; // we're still stopped
		}

		// Run different periodic actions (if its the time now)
		ReadIpTable(dwNow);
		ReadPerfTable(dwNow);
		RunNetworkLabeling();
		RunQuestionnaireIfCase(dwNow);
		RunAutoSubmit(dwNow);
		if (RunAutoUpdate(dwNow)) {
			m_fUpdatePending = TRUE;
			continue; // going to update
		}

		// check for max session duration once per hour (and rotate at night)
		if (dwNow - lastSessionCheck > 3600000) {
			Debug("Session check");
			SYSTEMTIME st;
			GetLocalTime(&st);

			// NOTE: this will happen (by desing) only if the computer happens to run 
			// at night (e.g. a pc that is on all the time), we except sessions 
			// mostly to follow computer sleep cycles (see suspend+resume). This
			// is here just to ensure some hard limit on the duration of a single session.
			if (st.wHour >= 2 && st.wHour <= 3 && lastRotateDay < st.wDay) {
				lastRotateDay = st.wDay;
				Debug("Session check _ MessageRestartSession");
				SendServiceMessage(Message(MessageRestartSession));
			}
			
			lastSessionCheck = dwNow;
		}

	} // end while
	Debug("m_hStoppedEvent");
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

		Trace("Hostview service stop.");
		if (!m_fUserStopped)
			StopCollect(SessionEvent::Stop, GetHiResTimestamp());
		StopServiceCommunication();
	}
}

void CHostViewService::OnShutdown()
{
	OnStop();
}

void CHostViewService::Cleanup() {
	WIN32_FIND_DATAA wfd;
	HANDLE hFind;
		
	hFind = FindFirstFileA(DATA_DIRECTORY_GLOB, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.cFileName[0] == '.') 
				continue;
			Trace("Cleanup pcap file %s.", wfd.cFileName);
			MoveFileToSubmit(wfd.cFileName, m_settings.GetBoolean(DebugMode));
		} while (FindNextFileA(hFind, &wfd));
		FindClose(hFind);
	}

	hFind = FindFirstFileA(TEMP_PATH_DB_GLOB, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.cFileName[0] == '.')
				continue;
			Trace("Cleanup db file %s.", wfd.cFileName);
			MoveFileToSubmit(wfd.cFileName, m_settings.GetBoolean(DebugMode));
		} while (FindNextFileA(hFind, &wfd));
		FindClose(hFind);
	}
}

void CHostViewService::StartCollect(SessionEvent e, ULONGLONG ts)
{
	if (m_startTime == 0) {
		// cleanup any pending files from previous close (pcap + temp)
		Cleanup();

		// new session id
		m_startTime = ts;

		// make sure we are using the latest settings
		m_settings.ReloadSettings();
		if (!m_settings.readSalt()) {
			Trace("[SRC] error: failed reading the salt");
		}
		if(!m_settings.readIdentifier()) {
			Trace("[SRC] error: failed reading the identifier");
		}

		Trace("Hostview service session %llu start [reason=%d].", ts, e);

		// get sysinfo (data that wont change during the session)
		//TODO system info contains sensitive data that should be hashed
		QuerySystemInfo(m_sysInfo);
		//TODO who cares anymore about the hddserial, I'm going to use a random id
		if (_tcslen(m_sysInfo.hddSerial) <= 0)
		{
			Debug("[SRV] fatal : we don't know the hddSerial");
			m_startTime = 0;
			return;
		}
		//get valueas from settings
		std::string salt = m_settings.GetSalt();
		std::string identifier = m_settings.GetIdentifier();
		//hashedWStrings.addSalt((unsigned char*)salt.c_str(), salt.length());
		//hashedStrings.addSalt((unsigned char*)salt.c_str(), salt.length());
		//hashedIPs.addSalt((unsigned char*)salt.c_str(), salt.length());

		//Hash password for additional security
		DWORD errNumber;
		char hexHash[65];
		BYTE *hash;
		unsigned long hashLen;

		if ((errNumber = hashedStrings.getHashString(identifier, &hash, &hashLen))|| hashLen == 0 || hash == NULL) {
			Trace("[SRV] Error hashing the user id from %s", identifier.c_str());
			sprintf_s(szHdd, "%s", identifier.c_str());
		}
		else {
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash[2 * j], "%02X", hash[j]);
			hexHash[64] = 0;
			sprintf_s(szHdd, "%s", hexHash);
			Trace("[SRV] Using id %s from identifier %s", szHdd, identifier.c_str());
		}

		if (!m_store.Open(m_startTime)) {
			Debug("[SRV] fatal : failed to open the data store");
			m_startTime = 0;
			return;
		}

		// log session start + once per session data
		hash = NULL;

		//hashing hdd serial number
		TCHAR toHashSerial_wc[MAX_PATH] = { 0 };
		wsprintf(toHashSerial_wc, L"%S", m_sysInfo.hddSerial);
		char hexHash_c[MAX_PATH];
		getHashedWString(toHashSerial_wc, m_sysInfo.hddSerial);


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
	Debug("StopCollect");
	if (m_startTime > 0) {
		Trace("Hostview service session %llu stop [reason=%d].", m_startTime, e);

		m_store.InsertSession(ts, e);
		m_store.Close();
	}

	StopHttpDispatcher();
	StopWifiMonitor();
	StopInterfacesMonitor(*this);

	m_startTime = 0;  
}

void CHostViewService::OnPowerEvent(PPOWERBROADCAST_SETTING event)
{
	if (!m_fUserStopped && m_startTime!=0) {
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

void CHostViewService::LogNetwork(const NetworkInterface &ni, ULONGLONG timestamp, bool connected)
{
	if (connected && m_settings.GetULong(NetLabellingActive))
	{
		// queue unknown net for labeling (by user)
		if (!m_networks.IsKnown(ni) && ni.strBSSID.length() > 0)
		{
			EnterCriticalSection(&m_cs);
			m_interfacesQueue.push_back(ni);
			LeaveCriticalSection(&m_cs);
		}
	}

	//TODO  use a larger MAX_PATH (and update the info in the db too)
	size_t size = 0;
	TCHAR szGateways[MAX_PATH] = { 0 };
	for (size_t i = 0; i < ni.gateways.size(); i++)
	{
		size += ni.gateways[i].length();
		if (size > MAX_PATH) {
			//we can't write more info in the structure
			Trace("Impossible to write more ip addresses in the gateway list. Not enough space in the structure");
			break;
		}
		_tcscat_s(szGateways, ni.gateways[i].c_str());

		if (i < ni.gateways.size() - 1)
		{
			size += 1;
			if (size > MAX_PATH) {
				//we can't write more info in the structure
				Trace("Impossible to write more ip addresses in the gateway list. Not enough space in the structure");
				break;
			}
			_tcscat_s(szGateways, _T(","));
		}
	}

	size = 0;
	TCHAR szDnses[MAX_PATH] = { 0 };
	for (size_t i = 0; i < ni.dnses.size(); i++)
	{
		size += ni.dnses[i].length();
		if (size > MAX_PATH) {
			//we can't write more info in the structure
			Trace("Impossible to write more ip addresses in the DNS list. Not enough space in the structure");
			break;
		}
		_tcscat_s(szDnses, ni.dnses[i].c_str());

		if (i < ni.dnses.size() - 1)
		{
			size += 1;
			if (size > MAX_PATH) {
				//we can't write more info in the structure
				Trace("Impossible to write more ip addresses in the DNS list. Not enough space in the structure");
				break;
			}
			_tcscat_s(szDnses, _T(","));
		}
	}

	size = 0;
	TCHAR szIps[MAX_PATH] = { 0 };
	for (size_t i = 0; i < ni.ips.size(); i++)
	{
		size += ni.ips[i].length();
		if (size > MAX_PATH) {
			//we can't write more info in the structure
			Trace("Impossible to write more ip addresses in the list of addresses associated with a NIC. Not enough space in the structure");
			break;
		}
		_tcscat_s(szIps, ni.ips[i].c_str());

		if (i < ni.ips.size() - 1)
		{
			size += 1;
			if (size > MAX_PATH) {
				//we can't write more info in the structure
				Trace("Impossible to write more ip addresses in the list of addresses associated with a NIC. Not enough space in the structure");
				break;
			}
			_tcscat_s(szIps, _T(","));
		}
	}


	//hashing  (todo: put it in a function)
	BYTE *hash;
	unsigned long hashLen;
	DWORD errNumber;
	size_t outSize;

	//hashing GUID
	char hexHash_c[65];
	TCHAR hexHashGUID[65] = { 0 };
	std::string toHashGUID(ni.strGuid.c_str());
	
	TCHAR toHashGUID_wc[65] = { 0 };
	wsprintf(toHashGUID_wc, L"%S", ni.strGuid.c_str());
	std::wstring toHashGUID_w(toHashGUID_wc);

	if ((errNumber = hashedWStrings.getHashWString(toHashGUID_w, &hash, &hashLen))) {
		Debug("Error hashing the GUID: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[2 * j], "%02X", hash[j]);
	//	sprintf(&hexHash_c[j], "%X", hash[j]);

	hexHash_c[64] = 0;
	wsprintf(hexHashGUID, L"%S", hexHash_c); 
	
	Debug(" getHashWString len %lu %lu", hashLen,  toHashGUID.length());
	Debug("hexHashGUID from ni %S <- %s", hexHashGUID, ni.strGuid.c_str());

	//Hashing Mac adddress
	hash = NULL;
	TCHAR hexHashMacAddress[65];
	std::wstring toHash_W(ni.strMac);

	if ((errNumber = hashedWStrings.getHashWString(toHash_W, &hash, &hashLen))) {
		Debug("Error hashing the MAC address: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[j], "%X", hash[j]);

	mbstowcs_s(&outSize, hexHashMacAddress, 65, hexHash_c, 64);
	Debug("macAddressHash %S", hexHashMacAddress);

	//hashing BSSID
	hash = NULL;
	TCHAR hexHashBSSID[65];
	toHash_W = ni.strBSSID;

	if ((errNumber = hashedWStrings.getHashWString(toHash_W, &hash, &hashLen))) {
		Debug("Error hashing the BSSID address: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[j * 2], "%02X", hash[j]);

	mbstowcs_s(&outSize, hexHashBSSID, 65, hexHash_c, 64);
	Debug("BSSID %S", hexHashBSSID);

	//Hashing strSSID
	hash = NULL;
	TCHAR hexHashSSID[65] = { 0 };
	std::string toHashSSID(ni.strSSID.c_str());

	TCHAR toHashSSID_wc[65] = { 0 };
	wsprintf(toHashSSID_wc, L"%S", toHashSSID);
	std::wstring toHashSSID_w(toHashSSID_wc);


	if ((errNumber = hashedWStrings.getHashWString(toHashSSID_w, &hash, &hashLen))) {
		Debug("Error hashing the SSID: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[2 * j], "%02X", hash[j]);
		//sprintf(&hexHash_c[j], "%X", hash[j]);

	hexHash_c[64] = 0;
	wsprintf(hexHashSSID, L"%S", hexHash_c);

	Debug("strSSID %S", hexHashSSID);

	//Hashing Profile
	hash = NULL;
	TCHAR hexHashProfile[65];
	toHash_W = ni.strProfile;

	if ((errNumber = hashedWStrings.getHashWString(toHash_W, &hash, &hashLen))) {
		Debug("Error hashing the BSSID address: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[j * 2], "%02X", hash[j]);
//		sprintf(&hexHash_c[j], "%X", hash[j]);

	hexHash_c[64] = 0;
	wsprintf(hexHashProfile, L"%S", hexHash_c);
	Debug("Profile before mbstowcs_s %S", hexHashProfile);
	mbstowcs_s(&outSize, hexHashProfile, 65, hexHash_c, 64);
	Debug("Profile with mbstowcs_s %S", hexHashProfile);


	m_store.InsertConn(hexHashGUID/*ni.strGuid.c_str()*/, ni.strFriendlyName.c_str(), ni.strDescription.c_str(), ni.strDnsSuffix.c_str(),
		hexHashMacAddress/*ni.strMac.c_str()*/, szIps, szGateways, szDnses, ni.tSpeed, ni.rSpeed, ni.wireless, hexHashProfile/*ni.strProfile.c_str()*/,
		hexHashSSID /*ni.strSSID.c_str()*/, hexHashBSSID/*ni.strBSSID.c_str()*/, ni.strBSSIDType.c_str(), ni.strPHYType.c_str(), ni.phyIndex,
		ni.channel, connected, timestamp);
}

void CHostViewService::OnInterfaceConnected(const NetworkInterface &networkInterface)
{
	if (m_startTime == 0) {
		return; // no session
	}

	ULONGLONG timestamp = GetHiResTimestamp(); // connection id

	LogNetwork(networkInterface, timestamp, true);
	StartCapture(*this, m_startTime, timestamp, m_settings.GetULong(PcapSizeLimit), m_settings.GetBoolean(DebugMode), networkInterface.strGuid.c_str());

	// resolve network location
	if (m_settings.GetBoolean(NetLocationActive)) {
		// FIXME: this could be a bit more intelligent - not sure we need to query this stuff
		// every time an interface goes up .. ?
		std::vector<std::string> * pInfo = 0;
		if (QueryPublicInfo(m_settings.GetString(NetLocationApiUrl), pInfo) && pInfo->size() > 7)
		{
			m_store.InsertLoc(networkInterface.strGuid.c_str(), 
				(*pInfo)[0].c_str(), // public ip 
				(*pInfo)[1].c_str(), // rev dns
				(*pInfo)[2].c_str(), // as number
				(*pInfo)[3].c_str(), // as name
				(*pInfo)[4].c_str(), // country
				(*pInfo)[5].c_str(), // city
				(*pInfo)[6].c_str(), // lat
				(*pInfo)[7].c_str(), // lon
				timestamp);
		}
		FreePublicInfo(pInfo);
	}
}

void CHostViewService::OnInterfaceDisconnected(const NetworkInterface &networkInterface)
{
	if (m_startTime == 0) {
		return; // no session
	}

	ULONGLONG timestamp = GetHiResTimestamp();
	StopCapture(networkInterface.strGuid.c_str());
	LogNetwork(networkInterface, timestamp, false);
}

void CHostViewService::OnWifiStats(std::wstring szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state)
{
	if (m_startTime == 0) {
		return; // no session
	}

	ULONGLONG timestamp = GetHiResTimestamp();

	BYTE *hash;
	unsigned long hashLen;
	DWORD errNumber;
	size_t outSize;

	//hashing GUID
	char hexHash_c[65];
	TCHAR hexHashGUID[65] = { 0 };

	if ((errNumber = hashedWStrings.getHashWString(szGuid, &hash, &hashLen))) {
		Debug("Error hashing the GUID: %x \n", errNumber);
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[2 * j], "%02X", hash[j]);
	hexHash_c[64] = 0;
	wsprintf(hexHashGUID, L"%S", hexHash_c);
	
	Debug("hexHashGUID from wifiStats %S", hexHashGUID);
	m_store.InsertWifi(hexHashGUID, tSpeed, rSpeed, signal, rssi, state, timestamp);
}

void CHostViewService::OnDNSAnswer(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost)
{
	if (m_startTime == 0) {
		return; // no session
	}

	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.InsertDns(connection, type, szIp, szHost, protocol, szSrc, szDest, srcport, destport, timestamp);
}

void CHostViewService::OnHttpHeaders(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
	char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength)
{
	if (m_startTime == 0) {
		return; // no session
	}

	ULONGLONG timestamp = GetHiResTimestamp();
	m_store.InsertHttp(connection, szVerb, szVerbParam, szStatusCode, szHost, szReferer, szContentType, szContentLength, protocol, szSrc, szDest,
		srcport, destport, timestamp);
}

bool CHostViewService::OnBrowserLocationUpdate(TCHAR *location, TCHAR *browser)
{
	if (location && browser && !m_fUserStopped && m_startTime!=0)
	{
		m_store.InsertBrowser(browser, location, GetHiResTimestamp());
		UpdateQuestionnaireSiteUsageInfo(location, browser);
		return true;
	}
	return false;
}

bool CHostViewService::OnJsonUpload(char **jsonbuf, size_t len) {
	if (jsonbuf && len > 0 && !m_fUserStopped && m_startTime != 0) {
		char fn[MAX_PATH];
		sprintf_s(fn, "%s\\%llu_%llu_browserupload.json", TEMP_PATH, m_startTime, GetHiResTimestamp());
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

bool CHostViewService::ReadIpTable(DWORD now)
{
	static DWORD dwIpLastWrite = 0;
	if (m_startTime == 0) {
		return false; // no session
	}

	if (now - dwIpLastWrite >= m_settings.GetULong(SocketStatsTimeout))
	{
		IpTable *table = GetIpTable();
		ULONGLONG timestamp = GetHiResTimestamp();

		for (size_t i = 0; i < table->size; i++)
		{
			IpRow it = table->get(i);
			char *hexHash = getHashedIPFromCharPtr(it.srcIp);
			if(hexHash!=NULL) 
				m_store.InsertPort(it.pid, it.name, it.IsUDP() ? IPPROTO_UDP : IPPROTO_TCP, hexHash, it.destIp, it.srcport, it.destport, it.state, timestamp);
			else
				m_store.InsertPort(it.pid, it.name, it.IsUDP() ? IPPROTO_UDP : IPPROTO_TCP, it.srcIp, it.destIp, it.srcport, it.destport, it.state, timestamp);
		}

		ReleaseIpTable(table);
		dwIpLastWrite = now;
		return TRUE;
	}
	return FALSE;
}

bool CHostViewService::ReadPerfTable(DWORD now)
{
	static DWORD dwPerfLastWrite = 0;
	if (m_startTime == 0) {
		return false; // no session
	}

	if (now - dwPerfLastWrite >= m_settings.GetULong(SystemStatsTimeout))
	{
		PerfTable *table = GetPerfTable();
		ULONGLONG timestamp = GetHiResTimestamp();

		for (size_t i = 0; i < table->size; i++)
		{
			PerfRow it = table->get(i);
			m_store.InsertProc(it.pid, it.name, it.memory, it.cpu, timestamp);
		}

		ReleasePerfTable(table);
		dwPerfLastWrite = now;
		return TRUE;
	}
	return FALSE;
}

void CHostViewService::RunNetworkLabeling() {
	if (m_settings.GetBoolean(NetLabellingActive) &&
		m_hasSeenUI && !m_fUserStopped && !m_fIdle && !m_fFullScreen &&
		m_interfacesQueue.size() > 0 && m_startTime != 0) 
	{
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

bool CHostViewService::RunAutoSubmit(DWORD now, BOOL force) {
	static DWORD sdwLastSubmit = GetTickCount();

	if (force || (now - sdwLastSubmit >= m_settings.GetULong(AutoSubmitInterval) &&
		!m_fFullScreen && m_fIdle))
	{
		Debug("[SRV] checking RunAutoSubmit");
		if (force)Debug("[SRV] Forcing the upload of the files to submit");
		if (m_upload == NULL) {
			// new upload session
			m_upload = new CUpload();
		} // else retrying with old

		if (!m_upload->TrySubmit(szHdd)) {
			// no need to retry - either this was a success or then max retries was reached
			Debug("[SRV] Succesfully uploaded all files");
			delete m_upload;
			m_upload = NULL;
			sdwLastSubmit = now;
			return TRUE;
		}
		Debug("[SRV] RunAutoSubmit check DONE");

	}
	return FALSE;
}

bool CHostViewService::RunAutoUpdate(DWORD now, BOOL force) {
	static DWORD sdwLastCheck = 0; // check as soon as possible for the first time

	if (force || (now - sdwLastCheck >= m_settings.GetULong(AutoUpdateInterval) &&
		  		  !m_fFullScreen && m_fIdle))
	{
		sdwLastCheck = now;

		// HostView update 
		Trace("Hostview check update.");
		if (CheckForUpdate(m_settings.GetString(UpdateLocation)))
		{
			Trace("Hostview update available.");
			TCHAR szUpdate[MAX_PATH] = { 0 };
			if (DownloadUpdate(m_settings.GetString(UpdateLocation), szUpdate, _countof(szUpdate)))
			{
				Trace("Hostview going to update!!");
				// TODO: will be as system; what happens to UI?
				ShellExecute(NULL, L"open", szUpdate, 0, 0, SW_SHOW);
				return TRUE;
			}
		}
		else {
			// UI + settings updates
			CheckForResourceUpdates(m_settings.GetString(UpdateLocation));
		}
	}
	return FALSE;
}

bool CHostViewService::RunQuestionnaireIfCase(DWORD now)
{
	static DWORD sdwLastCheck = GetTickCount();

	if (m_settings.GetBoolean(EsmActive) && 
		now - sdwLastCheck >= m_settings.GetULong(EsmCoinFlipInterval) && 
		!m_fUserStopped && !m_fIdle && !m_fFullScreen && m_hasSeenUI && m_startTime!=0)
	{
		sdwLastCheck = now;

		// enforce max shows per day and start + end hours
		if (QueryQuestionnaireCounter() < m_settings.GetULong(EsmMaxShows))
		{
			// pick random number between [0,100) and compare to the selected probability
			if ((double) rand() / (RAND_MAX + 1) * 100.0 <= 1.0*m_settings.GetULong(EsmCoinFlipProb))
			{
				ShowQuestionnaireUI(FALSE);
				return TRUE;
			}
		}
	}
	return FALSE;
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

		m_qOnDemand = fOnDemand;
		m_qCounter = QueryQuestionnaireCounter();
		m_qStartTime = GetHiResTimestamp();

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
			QueryProcessIcon(dwPid, szApp);
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

		// FIXME: should be in icon.cpp I guess
		TCHAR szLocal[MAX_PATH] = {0};
		_stprintf_s(szLocal, _T(".\\html\\out\\%s.ico"), szLocation);
		if (GetFileAttributes(szLocal) == INVALID_FILE_ATTRIBUTES)
		{
			// FIXME: this probably doesn't work all the time ..
			TCHAR szRemote[MAX_PATH] = { 0 };
			_stprintf_s(szRemote, _T("http://%s/favicon.ico"), szLocation);
			DownloadFile(szRemote, szLocal);
		}
	}
}

char *CHostViewService::getHashedIPFromCharPtr(char *strIp) {
	//TODO do not use pre fixed size
	char hexHash[65];
	unsigned int ipToHash, newIp;
	DWORD errNumber;

	ipToHash = inet_addr(strIp);
	if (ipToHash == INADDR_NONE) return NULL;

	if ((errNumber = hashedIPs.get32Hash(ipToHash, &newIp))) {
		Debug("Error hashing the SRC IP from the IP table: %x \n", errNumber);
	}

	struct in_addr in;
	in.S_un.S_addr = newIp;
	return inet_ntoa(in);
}

std::string CHostViewService::getHashedIPFromString(std::string &strIp) {
	return std::string(getHashedIPFromCharPtr((char *)strIp.c_str()));
}

bool CHostViewService::getHashedWString(std::wstring stringToHash, TCHAR* output) {
	BYTE *hash;
	unsigned long hashLen;
	DWORD errNumber;
	size_t outSize;

	char hexHash_c[MAX_PATH];

	if ((errNumber = hashedWStrings.getHashWString(stringToHash, &hash, &hashLen))) {
		Debug("Error hashing the GUID: %x \n", errNumber);
		return false;
	}
	for (int j = 0; j < hashLen; j++)
		sprintf(&hexHash_c[2 * j], "%02X", hash[j]);
	hexHash_c[MAX_PATH-1] = 0;
	wsprintf(output, L"%S", hexHash_c);

	Debug("getHashedWString %S", output);
	return true;
}

Message CHostViewService::OnMessage(Message &message)
{
	Debug("OnMessage %d", message.type );

	switch (message.type)
	{
	case MessageUpload:
		RunAutoSubmit(GetTickCount(), TRUE);
		break;

	case MessageCheckUpdate:
		RunAutoUpdate(GetTickCount(), TRUE);
		break;

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
		{
			Trace("Hostview service suspend.");
			if (!m_fUserStopped) {
				StopCollect(SessionEvent::Suspend, GetHiResTimestamp());
			}
			m_fUserStopped = FALSE; // so that everything is resumed OnResume
			m_fIdle = FALSE;
			m_fFullScreen = FALSE;
			m_hasSeenUI = FALSE;
		}
		break;

	case MessageResume:
		{
			Trace("Hostview service resume.");
			StartCollect(SessionEvent::Resume, GetHiResTimestamp());
		}
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
			m_dwUserStoppedTime = GetTickCount();
			m_fUserStopped = TRUE;
			m_fIdle = FALSE;
			m_fFullScreen = FALSE;
			m_hasSeenUI = FALSE;

			//todo_GG: this should be called in the case of user pausing or stopping Hostview (I believe). The SessionEvent should differentiate between these 2 cases and not be always PAUSE  
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

			//Hashing user name
			TCHAR toHashUser[MAX_PATH] = { 0 };
			TCHAR hexHashUser[MAX_PATH];
			wsprintf(toHashUser, L"%S", message.szUser);

			getHashedWString(toHashUser, hexHashUser);
			Debug("hexHashUser %S", hexHashUser);

			m_store.InsertActivity(hexHashUser, message.dwPid, szApp, szDescription, message.isFullScreen, message.isIdle, GetHiResTimestamp());

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
			Debug("OnMessage %d  DONE", message.type);
			return result;
		}
		break;

	case MessageNetworkLabel:
		{			
			Debug("New network label [%S].", message.szUser); 
			
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
			//TODO check for correctness
			// remember labeled networks

			// store for upload
			TCHAR hexHash[65];
			TCHAR hexHashGUID[65];
			char hexHash_c[65];
			char hexHashGUID_c[65];
			TCHAR test[65];

			std::wstring toHash(szBSSID);
			std::wstring toHashGUID(szGUID);

			BYTE *hash;
			unsigned long hashLen;

			DWORD errNumber;

			size_t outSize;



			if ((errNumber = hashedWStrings.getHashWString(toHash, &hash, &hashLen))) {
				Debug("Error hashing the BSSID: %x \n", errNumber);
			}
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash_c[j * 2], "%02X", hash[j]);

			mbstowcs_s(&outSize, hexHash, 65, hexHash_c, 64);
			Debug("mbstowcs_s %S", hexHash);

			hexHash[hashLen*2] = 0;

			// Hashing GUID
			if ((errNumber = hashedWStrings.getHashWString(toHashGUID, &hash, &hashLen))) {
				Debug("Error hashing the BSSID: %x \n", errNumber);
			}
			//Debug(" getHashWString len %lu %lu %lu", hashLen, sizeof(wchar_t), toHashGUID.length());

			for (int j = 0; j < hashLen; j++) {
				//ssprintf(&hexHashGUID_c[j], "%X", hash[j]);
				sprintf(&hexHashGUID_c[2*j], "%02X", hash[j]);

			}
			wsprintf(hexHashGUID, L"%S", hexHashGUID_c);
			Debug("mbstowcs NO_s_hexHashGUID_c MAH %S", hexHashGUID);
			mbstowcs_s(&outSize, hexHashGUID, 65, hexHashGUID_c, 64);
			Debug("mbstowcs NO_s_hexHashGUID_c with mbstowcs_s  %S", hexHashGUID);

			m_store.InsertNetLabel(GetHiResTimestamp(), hexHashGUID, hexHash, szLabel);

			m_networks.OnNetworkLabel(hexHashGUID, hexHash, szLabel);
		}
		break;

	case MessageQuest:
		{
			if (m_qStartTime > 0) {
				Trace("Questionnaire [%u]: took %llu ms qoe %d", m_qCounter, message.dwPid, _wtoi(message.szUser));
				m_store.InsertEsm(m_qStartTime, m_qOnDemand, message.dwPid, _wtoi(message.szUser));
			}
		}
		break;

	case MessageQuestAppImportance:
		{
			if (m_qStartTime > 0) {
				Trace("MessageQuestAppImportance Questionnaire app importance tags [%u]: %S", m_qCounter, message.szUser);

				TCHAR szName[MAX_PATH] = { 0 };
				TCHAR szDesc[MAX_PATH] = { 0 };
				TCHAR szTags[MAX_PATH] = { 0 };
				TCHAR *szCtx = NULL;
				TCHAR *szToken = _tcstok_s(message.szUser, _T(";"), &szCtx);
				if (szToken)
				{
					_tcscpy_s(szName, szToken);
					szToken = _tcstok_s(NULL, _T(";"), &szCtx);
					if (szToken)
					{
						_tcscpy_s(szDesc, szToken);
						szToken = _tcstok_s(NULL, _T(";"), &szCtx);
						if (szToken)
						{
							_tcscpy_s(szTags, szToken);
							m_store.InsertEsmActivityImportance(m_qStartTime, szName, szDesc, szTags);
						}
					}
				}
			}
		}
	break;

	case MessageQuestAppPerformance:
	{
		if (m_qStartTime > 0) {
			Trace("MessageQuestAppPerformance Questionnaire app performance tags [%u]: %S", m_qCounter, message.szUser);

			TCHAR szName[MAX_PATH] = { 0 };
			TCHAR szDesc[MAX_PATH] = { 0 };
			TCHAR szTags[MAX_PATH] = { 0 };
			TCHAR *szCtx = NULL;
			TCHAR *szToken = _tcstok_s(message.szUser, _T(";"), &szCtx);
			if (szToken)
			{
				_tcscpy_s(szName, szToken);
				szToken = _tcstok_s(NULL, _T(";"), &szCtx);
				if (szToken)
				{
					_tcscpy_s(szDesc, szToken);
					szToken = _tcstok_s(NULL, _T(";"), &szCtx);
					if (szToken)
					{
						_tcscpy_s(szTags, szToken);
						m_store.InsertEsmActivityQoE(m_qStartTime, szName, szDesc, szTags);
					}
				}
			}
		}
	}
	break;

	case MessageQuestActitivyTags:
		{
			if (m_qStartTime > 0) {
				Trace("Questionnaire activity tags [%u]: %S", m_qCounter, message.szUser);

				// FIXME: the messages should really have a more versatile format, this is ugly ...
				TCHAR szName[MAX_PATH] = { 0 };
				TCHAR szDesc[MAX_PATH] = { 0 };
				TCHAR szTags[MAX_PATH] = { 0 };
				TCHAR *szCtx = NULL;
				TCHAR *szToken = _tcstok_s(message.szUser, _T(";"), &szCtx);
				if (szToken)
				{
					_tcscpy_s(szName, szToken);
					szToken = _tcstok_s(NULL, _T(";"), &szCtx);
					if (szToken)
					{
						_tcscpy_s(szDesc, szToken);
						szToken = _tcstok_s(NULL, _T(";"), &szCtx);
						if (szToken)
						{
							_tcscpy_s(szTags, szToken);

							m_store.InsertEsmActivity(m_qStartTime, szName, szDesc, szTags);
						}
					}
				}
			}
		}
		break;

	case MessageQuestProblemTags:
		{
			if (m_qStartTime > 0) {
				Trace("Questionnaire problem tags [%u]: %S", m_qCounter, message.szUser);

				// FIXME: the messages should really have a more versatile format, this is ugly ...
				TCHAR szName[MAX_PATH] = { 0 };
				TCHAR szDesc[MAX_PATH] = { 0 };
				TCHAR szTags[MAX_PATH] = { 0 };
				TCHAR *szCtx = NULL;
				TCHAR *szToken = _tcstok_s(message.szUser, _T(";"), &szCtx);
				if (szToken)
				{
					_tcscpy_s(szName, szToken);
					szToken = _tcstok_s(NULL, _T(";"), &szCtx);
					if (szToken)
					{
						_tcscpy_s(szDesc, szToken);
						szToken = _tcstok_s(NULL, _T(";"), &szCtx);
						if (szToken)
						{
							_tcscpy_s(szTags, szToken);

							m_store.InsertEsmProblems(m_qStartTime, szName, szDesc, szTags);
						}
					}
				}
			}
		}
		break;

	case MessageQuestDone:
		{
			if (m_qStartTime > 0) {
				Trace("Questionnaire done [%u].", m_qCounter);
				SetQuestionnaireCounter(m_qCounter + 1);
				m_qCounter = 0;
				m_qStartTime = 0;
				m_qOnDemand = false;
			}
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
			SaveAppsList(message.szUser, szFile, _countof(szFile), m_settings.GetULong(EsmAppListHistory));
			ImpersonateRevert();

			Message result(0, szFile, 0, true, true);
			Debug("OnMessage %d  DONE", message.type);
			return result;
		}
		break;

	case MessageQueryInstalledApps:
		{
			TCHAR szFile[MAX_PATH] = {0};
			PullInstalledApps(szFile, _countof(szFile));
			Debug("OnMessage %d  DONE", message.type);
			return Message(0, szFile, 0, true, true);
		}
		break;
	}
	Debug("OnMessage %d  DONE", message.type);

	return message;
}
