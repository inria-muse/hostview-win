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
#include "StdAfx.h"
#include "Settings.h"
#include "comm.h"

// FIXME: why are these not properties of the class (maybe static if need ?) ?
SettingsMap mDefSettings;
SettingsMap mSettings;

std::hash_map<std::string, unsigned long> mLongSettings;

static volatile long nRefCount = 0;
CRITICAL_SECTION g_cs;

void initLock()
{
	if (InterlockedIncrement(&nRefCount) == 1)
	{
		InitializeCriticalSection(&g_cs);
	}
}

void uninitLock()
{
	if (InterlockedDecrement(&nRefCount) == 0)
	{
		DeleteCriticalSection(&g_cs);
	}
}

void lock()
{
	EnterCriticalSection(&g_cs);
}

void unlock()
{
	LeaveCriticalSection(&g_cs);
}

CSettings::CSettings()
{
	initLock();

	lock();
	if (!mSettings.size())
	{
		load();
	}
	unlock();
}

CSettings::~CSettings()
{
	// TODO: should clean the map? it'a a smart object though
	uninitLock();
}

unsigned long CSettings::GetULong(char *szKey)
{
	unsigned long lRes = 0L;

	lock();
	if (mLongSettings.find(szKey) == mLongSettings.end())
	{
		const char *szValue = GetString(szKey);
		if (szValue)
		{
			mLongSettings[szKey] = (unsigned long) atol(szValue);
			lRes = mLongSettings[szKey];
		}
	}
	else
	{
		lRes = mLongSettings[szKey];
	}
	unlock();

	return lRes;
}

char *CSettings::GetString(char *szKey)
{
	char * pszResult = 0;

	lock();
	if (mSettings.find(szKey) != mSettings.end())
	{
		pszResult = (char *) mSettings[szKey].c_str();
	}
	else if (mDefSettings.find(szKey) != mDefSettings.end())
	{
		pszResult = (char *) mDefSettings[szKey].c_str();
	}
	unlock();

	return pszResult;
}

void CSettings::trim(char * s)
{
	char * p = s;
	size_t l = strlen(p);

	while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
	while(* p && isspace(* p)) ++p, --l;

	memmove(s, p, l + 1);
}

bool CSettings::LoadSettings()
{
	bool res;
	lock();
	res = load();
	unlock();
	return res;
}

// should be called with lock !
bool CSettings::load()
{
	mSettings.clear();
	mDefSettings.clear();

	// default hardcoded settings
	mDefSettings[EndUser] = "notset";
	mDefSettings[InterfaceMonitorTimeout] = "250";
	mDefSettings[WirelessMonitorTimeout] = "60000";
	mDefSettings[AutoUpdateInterval] = "1800000";
	mDefSettings[UserMonitorTimeout] = "1000";
	mDefSettings[IoTimeout] = "1000";
	mDefSettings[UserIdleTimeout] = "5000";
	mDefSettings[SocketStatsTimeout] = "60000";
	mDefSettings[SystemStatsTimeout] = "60000";
	mDefSettings[PcapSizeLimit] = "5242880";
	mDefSettings[DbSizeLimit] = "5242880";
	mDefSettings[BatteryMonitorTimeout] = "15000";
	mDefSettings[EsmCoinFlipInterval] = "60000";
	mDefSettings[EsmCoinFlipMaximum] = "100";
	mDefSettings[EsmCoinFlipRange] = "1";
	mDefSettings[EsmMaxShows] = "3";
	mDefSettings[UploadLowSpeedLimit] = "32000"; // bytes/s
	mDefSettings[UploadLowSpeedTime] = "5";  // seconds
#ifndef _DEBUG
	mDefSettings[AutoSubmitInterval] = "1800000";
	mDefSettings[SubmitServer] = "https://muse.inria.fr/hostview";
	mDefSettings[UpdateLocation] = "https://muse.inria.fr/hostview/latest/";
	mDefSettings[QuestionnaireActive] = "1";
	mDefSettings[NetLabellingActive] = "1";
#else
	mDefSettings[AutoSubmitInterval] = "120000"; // ms
	mDefSettings[SubmitServer] = "http://localhost:3000";
	mDefSettings[UpdateLocation] = "http://foobar";
	mDefSettings[QuestionnaireActive] = "0";
	mDefSettings[NetLabellingActive] = "0";
	mDefSettings[EndUser] = "testuser";
#endif

	FILE *f = NULL;
	fopen_s(&f, "settings", "r");
	if (f)
	{
		char szBuffer[1024] = {0};
		while (fgets(szBuffer, _countof(szBuffer), f))
		{
			trim(szBuffer);

			char * context = NULL;
			char * token = strtok_s(szBuffer, "=", &context);

			if (token != NULL)
			{
				char szKey[MAX_PATH] = {0};
				char szValue[MAX_PATH] = {0};

				strcpy_s(szKey, token);
				trim(szKey);

				token = strtok_s(NULL, "=", &context);

				if (token != NULL)
				{
					strcpy_s(szValue, token);
					trim(szValue);

					mSettings[szKey] = szValue;
				}
			}
		}
		fclose(f);

		HKEY hKey = 0;
		RegCreateKey(HKEY_LOCAL_MACHINE, HOSTVIEW_REG, &hKey);

		char szUser[MAX_PATH] = {0};
		DWORD dwSize = sizeof(szUser);
		DWORD dwType = REG_SZ;
		if (RegQueryValueExA(hKey, EndUser, NULL, &dwType, (LPBYTE) szUser, &dwSize) == ERROR_SUCCESS)
		{
			szUser[dwSize] = 0;
			mSettings[EndUser] = szUser;
		}

		RegCloseKey(hKey);
		return true;
	}

	return false;
}
