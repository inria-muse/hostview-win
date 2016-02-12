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

#include "stdafx.h"
#include "HostView.h"
#include "comm.h"
#include "proc.h"
#include "Store.h"
#include "Settings.h"
#include "shlwapi.h"
#include "LifetimeCtrl.h"
#include "EsmWizard.h"
#include "NetWizard.h"
#include "resource.h"

#define MAX_LOADSTRING				100
#define WM_USER_SHELLICON			WM_USER + 10

#define TIMER_STATUS				100
#define TIMER_USER_IO				200
#define TIMER_STATUS_TIME			5000

#define TIMER_NOTIFICATION			300
#define TIMER_NOTIFICATION_TIME		60 * 1000
#define TIMER_NOTIFICATION_TIMEOUT	5 * 60 * 1000

INT_PTR CALLBACK DlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HiddenDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HWND g_hWnd;
CWinApp theApp;
HICON g_hIcons[2];
NOTIFYICONDATA g_nidApp;
CSettings settings;
CLifetimeCtrl ltCtrl;

TCHAR g_szUser[MAX_PATH] = {0};
DWORD g_dwLastPid = -1;

HINSTANCE g_hInstance = NULL;

class CUserMonitor : public CMonitorCallback
{
	virtual void OnApplication(DWORD dwPid, bool isFullScreen, bool isIdle)
	{
		g_dwLastPid = dwPid;
		SendServiceMessage(Message(MessageUserActivity, g_szUser, dwPid, isFullScreen, isIdle));
	}
} userMonitor;

// per user i/o
void ReadUserIoTable()
{
	DWORD *pSessions = 0;

	// speaker
	int nCount = QueryMultiMediaSessions(pSessions, false);
	for (int i = 0; i < nCount; i ++)
	{
		SendServiceMessage(Message(MessageSpeakerUsage, g_szUser, pSessions[i], false, false));
	}
	ReleaseMultiMediaSessions(pSessions);

	// microphone
	nCount = QueryMultiMediaSessions(pSessions, true);
	for (int i = 0; i < nCount; i ++)
	{
		SendServiceMessage(Message(MessageMicrophoneUsage, g_szUser, pSessions[i], false, false));
	}
	ReleaseMultiMediaSessions(pSessions);

	// camera
	nCount = QueryWebcamSessions(pSessions);
	for (int i = 0; i < nCount; i ++)
	{
		SendServiceMessage(Message(MessageCameraUsage, g_szUser, pSessions[i], false, false));
	}

	// keyboard + mouse
	bool isMouse = false, isKeyboard = false;
	QueryMouseKeyboardState(isMouse, isKeyboard);
	if (isMouse || isKeyboard)
	{
		// TODO: hack! better encapsulation of messages
		SendServiceMessage(Message(MessageMouseKeyUsage, g_szUser, g_dwLastPid, isMouse, isKeyboard));
	}
}

void LoadNotificationStrings(HINSTANCE hInstance, BOOL bEnabled)
{
	LoadString(hInstance, bEnabled ? IDS_HOSTVIEW_ENABLE_TITLE : IDS_HOSTVIEW_DISABLE_TITLE, g_nidApp.szInfoTitle, MAX_LOADSTRING);
	LoadString(hInstance, bEnabled ? IDS_HOSTVIEW_ENABLE_TEXT : IDS_HOSTVIEW_DISABLE_TEXT, g_nidApp.szInfo, MAX_LOADSTRING);
}

void InitNotificationResources(HINSTANCE hInstance)
{
	g_hIcons[0] = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLAY));
	g_hIcons[1] = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAUSE));

	TCHAR szAppName[MAX_PATH] = {0};
	char szProdVer[MAX_PATH] = {0};

	LoadString(hInstance, IDS_APP_TITLE, szAppName, MAX_LOADSTRING);

	GetProductVersion(szProdVer, sizeof(szProdVer));
	char *s = szProdVer;
	while (*s ++) *s = *s == ',' ? '.' : *s;

	_stprintf_s(g_nidApp.szTip, _T("%s %S"), szAppName, szProdVer);

	LoadNotificationStrings(hInstance, FALSE);

	UuidCreate(&g_nidApp.guidItem);

	g_hInstance = hInstance;
}

bool IsVistaOrLater()
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);
	return osvi.dwMajorVersion >= 6;
}

void OnEsmCommand(BOOL fOnDemand)
{
	CoInitialize(NULL);

	CEsmWizard dlg;

	BOOL fContinue = TRUE;

	if (!fOnDemand)
	{
		fContinue = (BOOL) DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL,
			MakeProcInstance(HiddenDlgCallback, NULL));
	}

	if (fContinue)
	{
		// TODO: should have failsafe if service is down!
		AppListT *runningApps = NULL, *installedApps = 0;

		Message result = SendServiceMessage(Message(MessageQueryLastApps, g_szUser, 0, false, false));
		if (result.type == MessageResult) {
			LoadAppList(result.szUser, runningApps);

			result = SendServiceMessage(Message(MessageQueryInstalledApps, g_szUser, 0, false, false));
			if (result.type == MessageResult) {
				LoadAppList(result.szUser, installedApps);

				dlg.SetApps(*runningApps, *installedApps);
				dlg.DoModal();

				ClearAppList(runningApps);
			}
			ClearAppList(installedApps);
		} // else the service is not reachable ..
	}
	CoUninitialize();
}

void OnNetCommand(const NetworkInterface &ni)
{
	CoInitialize(NULL);

	CNetWizard dlg;
	dlg.SetNetwork(ni);

	TCHAR szEventName[MAX_PATH] = {0};
	if (ni.wireless)
	{
		_stprintf_s(szEventName, _T("Event_%S"), ni.strSSID.c_str());
	}
	else
	{
		_stprintf_s(szEventName, _T("Event_%s"), ni.strBSSID.c_str());
	}
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, szEventName);

	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		dlg.DoModal();
	}

	if (hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hEvent);
		hEvent = NULL;
	}

	CoUninitialize();
}
void CloseHostViewUI()
{
	if (g_hWnd != NULL)
	{
		PostMessage(g_hWnd, WM_CLOSE, 0, 0);
	}
}

// capture state
bool g_isRunning = true;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpCmdLine, int)
{
	TCHAR szCmdLine[MAX_PATH] = {0};
	_tcscpy_s(szCmdLine, lpCmdLine);

	DWORD dwSize = sizeof(g_szUser);
	GetUserName(g_szUser, &dwSize);

	if (_tcscmp(g_szUser, _T("SYSTEM")) == 0)
	{
		// TODO: check on Windows XP for different username
		TCHAR szModule[MAX_PATH] = {0};
		GetModuleFileName(NULL, szModule, _countof(szModule));

		_tcscat_s(szModule, _T(" "));
		_tcscat_s(szModule, szCmdLine);
		RunAsCurrentUser(szModule);
		return 0;
	}

	AfxWinInit(::GetModuleHandle(NULL), NULL, szCmdLine, 0);
	InitCurrentDirectory();

	ltCtrl.Start(CloseHostViewUI);

    if (_tcsstr(szCmdLine, _T("/stop")))
	{
		// called from uninstaller - closes all open hostview UI components
		ltCtrl.CloseAll();
		return FALSE;
	}
	else if (_tcsstr(szCmdLine, _T("/esm")))
	{
		if (IsVistaOrLater())
		{
			OnEsmCommand(_tcsstr(szCmdLine, _T("/odm")) != NULL);
		}
		return FALSE;
	}
	else if (_tcsstr(szCmdLine, _T("/net")))
	{
		NetworkInterface *ni = NULL;
		if (CommandToNetworkInterface(szCmdLine, ni))
		{
			if (IsVistaOrLater())
			{
				OnNetCommand(*ni);
				ClearNetworkInterface(ni);
			}
		}
		return FALSE;
	}

	// load resources
	InitNotificationResources(hInstance);

	// dialog box
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), NULL, MakeProcInstance(DlgCallback, hInstance));

	DestroyIcon(g_hIcons[0]);
	DestroyIcon(g_hIcons[1]);

	ltCtrl.Stop();
	return TRUE;
}

void RefreshSystrayIcon()
{
	g_nidApp.hIcon = g_hIcons[g_isRunning ? 0 : 1];
	g_nidApp.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID; 
	Shell_NotifyIcon(NIM_MODIFY, &g_nidApp);
}

POINT lpClickPoint = {0, 0};
HMENU hPopMenu = NULL;

void ShowPopupMenu(HWND hWnd)
{
	TCHAR szBuffer[MAX_PATH] = {0};

	UINT uFlag = MF_BYPOSITION | MF_STRING;
	GetCursorPos(&lpClickPoint);

	hPopMenu = CreatePopupMenu();

	LoadString(NULL, g_isRunning ? IDS_PAUSE_HV : IDS_RESUME_HV, szBuffer, _countof(szBuffer));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_PAUSE_HOSTVIEW, szBuffer);

	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, ID_SEPARATOR,_T("SEP"));

	LoadString(NULL, IDS_EXIT_TRAY, szBuffer, _countof(szBuffer));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT_TRAY, szBuffer);

	SetForegroundWindow(hWnd);

	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
		lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
}

DWORD dwNotificationStart = 0;
INT_PTR CALLBACK HiddenDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			LoadString(NULL, IDS_ESM_TITLE, g_nidApp.szTip, MAX_LOADSTRING);

			LoadString(NULL, IDS_ESM_TITLE, g_nidApp.szInfoTitle, MAX_LOADSTRING);
			LoadString(NULL, IDS_ESM_BALOON_MESSAGE, g_nidApp.szInfo, MAX_LOADSTRING);

			UuidCreate(&g_nidApp.guidItem);

			g_hWnd = hDlg;

			g_nidApp.cbSize = NOTIFYICONDATA_V3_SIZE; 
			g_nidApp.hWnd = (HWND) hDlg;
			g_nidApp.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
			g_nidApp.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TRAY));
			g_nidApp.uTimeout = 15000;
			g_nidApp.uCallbackMessage = WM_USER_SHELLICON; 
			Shell_NotifyIcon(NIM_ADD, &g_nidApp);

			g_nidApp.uFlags |= NIF_INFO;

			g_nidApp.dwInfoFlags = NIIF_NOSOUND | NIIF_INFO;
			Shell_NotifyIcon(NIM_MODIFY, &g_nidApp);

			SetTimer(hDlg, TIMER_NOTIFICATION, TIMER_NOTIFICATION_TIME, NULL);
			dwNotificationStart = GetTickCount();

			SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_SHOWWINDOW);
		}
		break;
	case WM_DESTROY:
		{
			KillTimer(hDlg, TIMER_NOTIFICATION);
			Shell_NotifyIcon(NIM_DELETE, &g_nidApp);
			DestroyIcon(g_nidApp.hIcon);
		}
		break;
	case WM_TIMER:
		if (wParam == TIMER_NOTIFICATION)
		{
			if (GetTickCount() - dwNotificationStart >= TIMER_NOTIFICATION_TIMEOUT)
			{
				// expired
				EndDialog(hDlg, FALSE);
			}
			else
			{
				g_nidApp.uFlags |= NIF_INFO;
				g_nidApp.dwInfoFlags = NIIF_NOSOUND | NIIF_INFO;
				g_nidApp.uTimeout = 15000;
				g_nidApp.uCallbackMessage = WM_USER_SHELLICON; 
				Shell_NotifyIcon(NIM_MODIFY, &g_nidApp);
			}
		}
		break;
	case WM_USER_SHELLICON:
		{
			switch (LOWORD(lParam))
			{
			case WM_LBUTTONDOWN:
			case NIN_BALLOONUSERCLICK:
				EndDialog(hDlg, TRUE);
				break;
			}
		}
		break;
	}
	return (INT_PTR) FALSE;
}

Message queryMsg(MessageQueryStatus);

INT_PTR CALLBACK DlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		g_hWnd = hDlg;
		g_nidApp.cbSize = NOTIFYICONDATA_V3_SIZE; 
		g_nidApp.hWnd = (HWND) hDlg;
		g_nidApp.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
		g_nidApp.hIcon = g_hIcons[0];
		g_nidApp.uTimeout = 5000;
		g_nidApp.uCallbackMessage = WM_USER_SHELLICON; 
		Shell_NotifyIcon(NIM_ADD, &g_nidApp);

		g_isRunning = true;
		SendServiceMessage(Message(MessageStartCapture)); // make sure the service is capturing data
		StartUserMonitor(userMonitor, settings.GetULong(UserMonitorTimeout), settings.GetULong(UserIdleTimeout));

		SetTimer(hDlg, TIMER_STATUS, TIMER_STATUS_TIME, NULL);
		SetTimer(hDlg, TIMER_USER_IO, settings.GetULong(IoTimeout), NULL);

		SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_SHOWWINDOW);
		break;
	case WM_DESTROY:
		// TODO: this gets called when the user logs out ? is that what we want 
		// or should the service continue capture ?
		if (g_isRunning) {
			KillTimer(hDlg, TIMER_STATUS);
			KillTimer(hDlg, TIMER_USER_IO);
			StopUserMonitor();
			SendServiceMessage(Message(MessageStopCapture));
			g_isRunning = false;
		}

		Shell_NotifyIcon(NIM_DELETE, &g_nidApp);
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		break;
	case WM_TIMER:
		if (wParam == TIMER_STATUS)
		{
			// FIXME: not sure why this needs to be polled from the service ?
			// could have the timer here in the UI ?
			Message status = SendServiceMessage(queryMsg);
			switch (status.type)
			{
			case MessageStartCapture:
				if (!g_isRunning)
				{
					// has been enabled (automatically or by user after a pause)
					g_isRunning = true;
					RefreshSystrayIcon();

					settings.ReloadSettings();
					StartUserMonitor(userMonitor, settings.GetULong(UserMonitorTimeout), settings.GetULong(UserIdleTimeout));
					SetTimer(hDlg, TIMER_USER_IO, settings.GetULong(IoTimeout), NULL);

					// show info popup
					LoadNotificationStrings(g_hInstance, TRUE);
					g_nidApp.uFlags |= NIF_INFO;
					g_nidApp.dwInfoFlags = NIIF_NOSOUND | NIIF_INFO;
					Shell_NotifyIcon(NIM_MODIFY, &g_nidApp);
				}
				break;
			case MessageStopCapture:
				if (g_isRunning)
				{
					// handle user pause
					g_isRunning = false;
					RefreshSystrayIcon();

					StopUserMonitor();
					KillTimer(hDlg, TIMER_USER_IO);

					// show info popup
					LoadNotificationStrings(g_hInstance, FALSE);
					g_nidApp.uFlags |= NIF_INFO;
					g_nidApp.dwInfoFlags = NIIF_NOSOUND | NIIF_INFO;
					Shell_NotifyIcon(NIM_MODIFY, &g_nidApp);
				}
				break;
			case MessageError:
				// FIXME: could not get the service status
				// maybe it's not running ?!?!
				break;
			}
		}
		else if (wParam == TIMER_USER_IO)
		{
			ReadUserIoTable();
		}
		break;
	case WM_USER_SHELLICON:
		switch(LOWORD(lParam))
		{
		case WM_LBUTTONDOWN:
			SendServiceMessage(Message(MessageQuestionnaireShow));
			break;
		case WM_RBUTTONDOWN:
			ShowPopupMenu(hDlg);
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_EXIT_TRAY:
			// data captures etc handled in WM_DESTROY
			ltCtrl.CloseAll();
			//PostMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		case IDM_PAUSE_HOSTVIEW:
			// pause or restart 
			// FIXME: this depends on the service being alive, otherwise nothing happens ..
			SendServiceMessage(Message(g_isRunning ? MessageStopCapture : MessageStartCapture));
			break;
		}
		break;
	case WM_POWERBROADCAST:
		{
			switch (wParam) {
			case PBT_APMSUSPEND:
				if (g_isRunning) {
					// not user stopped
					StopUserMonitor();
					KillTimer(hDlg, TIMER_USER_IO);
				}
				SendServiceMessage(Message(MessageSuspend));
				break;
			case PBT_APMRESUMEAUTOMATIC:
				SendServiceMessage(Message(MessageResume));
				// start in non-user-stopped mode in anycase
				g_isRunning = true;
				StartUserMonitor(userMonitor, settings.GetULong(UserMonitorTimeout), settings.GetULong(UserIdleTimeout));
				SetTimer(hDlg, TIMER_USER_IO, settings.GetULong(IoTimeout), NULL);
				break;
			case PBT_POWERSETTINGCHANGE:
				break;
			}
		}
		break;
	}

	return FALSE;
}
