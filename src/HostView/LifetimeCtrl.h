#pragma once

#include "stdafx.h"

#define EVENT_NAME _T("Global\\HostViewUILifetimeCloseEvent")

typedef void (*CloseFunction)();

DWORD WINAPI MonitorLifetimeProc(LPVOID lpParam);

/**
 * This class allows your application to be closed in a clean manner from outside.
 **/
class CLifetimeCtrl
{
public:

	CLifetimeCtrl(void)
	{
	}

	virtual ~CLifetimeCtrl(void)
	{
	}

	void CloseAll()
	{
		SetEvent(m_hEvents[1]);
	}

	void Start(CloseFunction pFunction)
	{
		m_pFunction = pFunction;
		
		BYTE sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
		SECURITY_ATTRIBUTES sa;

		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);

		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = &sd;

		m_hEvents[0] = CreateEvent(&sa, TRUE, FALSE, NULL);
		m_hEvents[1] = CreateEvent(&sa, TRUE, FALSE, EVENT_NAME);

		m_hMonitorThread = CreateThread(NULL, NULL, ::MonitorLifetimeProc, this, NULL, NULL);
	}

	void Stop()
	{
		SetEvent(m_hEvents[0]);

		if (m_hMonitorThread)
		{
			WaitForSingleObject(m_hMonitorThread, INFINITE);
			CloseHandle(m_hMonitorThread);
			m_hMonitorThread = NULL;
		}

		CloseHandle(m_hEvents[0]);
		CloseHandle(m_hEvents[1]);

		m_hEvents[0] = 0;
		m_hEvents[1] = 0;
	}

	void MonitorLifetimeProc()
	{
		DWORD dwResult = WaitForMultipleObjects(_countof(m_hEvents), m_hEvents, FALSE, INFINITE);
		if (dwResult == WAIT_OBJECT_0 + 1)
		{
			if (m_pFunction)
			{
				m_pFunction();
			}
		}
	}

private:

	HANDLE m_hEvents[2];
	HANDLE m_hMonitorThread;
	CloseFunction m_pFunction;
};

DWORD WINAPI MonitorLifetimeProc(LPVOID lpParam)
{
	CLifetimeCtrl *pThis = (CLifetimeCtrl *) lpParam;
	if (pThis)
	{
		pThis->MonitorLifetimeProc();
	}
	return 0L;
}
