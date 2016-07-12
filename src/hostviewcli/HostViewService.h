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
#pragma once

#include "comm.h"
#include "shellapi.h"

#include "ServiceBase.h"
#include "Settings.h"
#include "proc.h"
#include "capture.h"
#include "Store.h"
#include "KnownNetworks.h"
#include "Upload.h"
#include "questionnaire.h"
#include "update.h"
#include "http_server.h"
#include "trace.h"
#include "product.h"


class CHostViewService :
	public CServiceBase,
	public CInterfacesCallback,
	public CWifiMonitor,
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

	// from CInterfacesCallback
	void OnInterfaceConnected(const NetworkInterface& networkInterface);
	void OnInterfaceDisconnected(const NetworkInterface& networkInterface);

	// from CWifiMonitor
	void OnWifiStats(const char *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state);

	// from CCaptureCallback
	void OnHttpHeaders(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
		char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength);
	void OnDNSAnswer(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost);

	// from CMessagesCallback
	Message OnMessage(Message &message);

	// from CHttpCallback
	bool OnBrowserLocationUpdate(TCHAR *location, TCHAR *browser);
	bool OnJsonUpload(char **jsonbuf, size_t len);

	// from CServiceBase
	void OnStart(DWORD dwArgc, PWSTR *pszArgv);
	void OnStop();
	void OnShutdown();
	void OnPowerEvent(PPOWERBROADCAST_SETTING event);

protected:
	void ServiceWorkerThread(void);

private:
	BOOL m_fIdle;
	BOOL m_fFullScreen;

	// this is the current session identifier
	ULONGLONG m_startTime;

	SysInfo m_sysInfo;
	char szHdd[MAX_PATH] = { 0 };

	BOOL m_fUpdatePending;

	BOOL m_fStopping;
	HANDLE m_hStoppedEvent;

	BOOL m_fUserStopped;
	DWORD m_dwUserStoppedTime;

	CSettings m_settings;
	CStore m_store;
	CUpload *m_upload;

	// start/stop sessions
	void StartCollect(SessionEvent e, ULONGLONG ts);
	void StopCollect(SessionEvent e, ULONGLONG ts);

	// periodic checks, return true if run, else false
	bool ReadIpTable(DWORD now);
	bool ReadPerfTable(DWORD now);
	bool RunQuestionnaireIfCase(DWORD now);
	bool RunAutoSubmit(DWORD now, BOOL force=FALSE);
	bool RunAutoUpdate(DWORD now, BOOL force = FALSE);
	void RunNetworkLabeling();

	void LogNetwork(const NetworkInterface &ni, ULONGLONG timestamp, bool connected);
	bool ShowQuestionnaireUI(BOOL fOnDemand = TRUE);

	// current active questionnaire info
	ULONGLONG m_qStartTime;
	BOOL m_qOnDemand;
	size_t m_qCounter;

	// Updates the in-memory app usage info for the questionnaire
	void UpdateQuestionnaireAppUsageInfo(DWORD dwPid, TCHAR *szUser, TCHAR *szApp);
	void UpdateQuestionnaireSiteUsageInfo(TCHAR *szLocation, TCHAR *szBrowser);

	// networks are added to this queue to be displayed
	// when a user is ready
	bool m_hasSeenUI;
	std::vector<NetworkInterface> m_interfacesQueue;
	CRITICAL_SECTION m_cs;
	CKnownNetworks m_networks;
};