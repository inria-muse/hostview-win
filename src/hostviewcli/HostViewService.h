/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Muse / INRIA
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
#pragma once

#include "comm.h"

#include "ServiceBase.h"
#include "proc.h"
#include "capture.h"
#include "Store.h"
#include "Settings.h"
#include "KnownNetworks.h"
#include "Upload.h"
#include "questionnaire.h"
#include "update.h"
#include "shellapi.h"

#define __LIGHT_SOCKS__
#include "socks.h"
#undef __LIGHT_SOCKS__

class CHostViewService :
	public CServiceBase,
	public CInterfacesCallback,
	public CWifiMonitor,
	public CBatteryMonitor,
	public CCaptureCallback,
	public CHttpCallback,
	public CMessagesCallback
{
public:
	CHostViewService(PWSTR pszServiceName, 
		BOOL fCanStop = TRUE, 
		BOOL fCanShutdown = TRUE, 
		BOOL fCanPauseContinue = FALSE);
	~CHostViewService(void);


	// main HostView ctl;
	void StartCollect();
	void StopCollect();

	void ReadIpTable();
	void ReadPerfTable();

	// interfaces monitor
	void OnInterfaceConnected(const NetworkInterface& networkInterface);
	void OnInterfaceDisconnected(const NetworkInterface& networkInterface);
	void OnInterfaceRestarted(const NetworkInterface& networkInterface);
	void LogNetwork(const NetworkInterface &ni, __int64 timestamp, bool connected);

	// wireless stats monitor
	void OnWifiStats(const std::tstring &interfaceGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state);

	// battery stats monitor
	void OnBatteryStats(byte status, byte percent);

	// capture callback
	void OnTCPPacket(char *szSrc, u_short srcport, char *szDest, u_short destport, int size) {};
	void OnUDPPacket(char *szSrc, u_short srcport, char *szDest, u_short destport, int size) {};
	void OnIGMPPacket(char *szSrc, char *szDest, int size) {};
	void OnICMPPacket(char *szSrc, char *szDest, int size) {};

	// TODO: should these be moved?
	void OnHttpHeaders(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
		char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength);
	void OnDNSAnswer(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost);

	void OnProcessPacketStart() { m_nTimestamp = GetTimestamp(); };
	void OnProcessPacketEnd() {};

	// COMM callback
	Message OnMessage(Message &message);

	// SOCKS callback
	bool OnMessage(ParamsT &params);

	// from CServiceBase
	void OnStart(DWORD dwArgc, PWSTR *pszArgv);
	void OnStop();

	bool SubmitData();
	void EnsureOptimalDiskUsage();
	void RemoveOldestSubmitFile();
	ULONGLONG GetSubmitDiskUsage();

protected:
	void OnShutdown();
	void ServiceWorkerThread(void);

	bool ShowQuestionnaireUI(BOOL fOnDemand = TRUE);

private:

	// used to query installed app icons in an async manner
	std::vector<std::tstring> m_appList;
	void QueryIconsThreadFunc();
	static DWORD WINAPI QueryIconsThreadProc(LPVOID lpParameter);

	BOOL m_fIdle;
	BOOL m_fFullScreen;

	void RunQuestionnaireIfCase();
	void PullLatestResources(DWORD dwNow);

	void PullInstalledApps(TCHAR *szPath, DWORD dwSize);

	BOOL m_fStopping;
	HANDLE m_hStoppedEvent;

	// TOOD: not setting this might trigger a submit every startup
	DWORD m_dwLastSubmit;
	DWORD m_dwRetryCount;

	DWORD m_dwLastUpdateCheck;

	BOOL m_fUserStopped;
	DWORD m_dwUserStoppedTime;

	__int64 m_nTimestamp;
	CSettings m_settings;
	CStore m_store;

	// Network Labelling
	void LabelNetwork(const NetworkInterface &ni);

	// Updates the in-memory app usage info for the questionnaire
	void UpdateQuestionnaireAppUsageInfo(DWORD dwPid, TCHAR *szUser, TCHAR *szApp);
	void UpdateQuestionnaireSiteUsageInfo(TCHAR *szLocation, TCHAR *szBrowser);

	// networks are added to this queue to be displayed
	// when a user is ready
	std::vector<NetworkInterface> m_interfacesQueue;
	CRITICAL_SECTION m_cs;

	CKnownNetworks m_networks;
};