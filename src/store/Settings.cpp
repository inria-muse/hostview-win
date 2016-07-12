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

// Hardcoded defaults in case the settings file is not available
// for what ever reason. Should not really happen in prod.
std::hash_map<std::string, std::string> mDefSettings;

void loadDefaults() {
	if (!mDefSettings.size()) {
		mDefSettings[SettingsVersion] = "0";
		mDefSettings[DebugMode] = "0";
		mDefSettings[AutoRestartTimeout] = "1800000"; // 30min
		mDefSettings[PcapSizeLimit] = "100000000"; // 100MB (uncompressed)
		mDefSettings[WirelessMonitorTimeout] = "10000";
		mDefSettings[SocketStatsTimeout] = "60000";
		mDefSettings[SystemStatsTimeout] = "60000";		
		mDefSettings[IoTimeout] = "1000";
		mDefSettings[UserMonitorTimeout] = "1000";
		mDefSettings[UserIdleTimeout] = "5000";
		mDefSettings[NetLabellingActive] = "1";
		mDefSettings[NetLocationActive] = "1";
		mDefSettings[EsmActive] = "1";
		mDefSettings[EsmCoinFlipInterval] = "3600000";
		mDefSettings[EsmCoinFlipProb] = "10";
		mDefSettings[EsmMaxShows] = "3";
		mDefSettings[EsmAppListHistory] = "60000";
		mDefSettings[SubmitServer] = "https://muse.inria.fr/hostviewupload-dev";
		mDefSettings[AutoSubmitRetryCount] = "3";
		mDefSettings[AutoSubmitRetryInterval] = "10000";
		mDefSettings[AutoSubmitInterval] = "3600000";
		mDefSettings[AutoUpdateInterval] = "86400000";
		mDefSettings[UpdateLocation] = "https://muse.inria.fr/hostview-dev/latest";
		mDefSettings[NetLocationApiUrl] = "https://muse.inria.fr/hostview-dev/location";

#ifdef _DEBUG
		mDefSettings[AutoSubmitRetryCount] = "1";
		mDefSettings[AutoSubmitRetryInterval] = "1000";
		mDefSettings[AutoSubmitInterval] = "300000";
		mDefSettings[SubmitServer] = "http://137.194.165.58:1337";
		mDefSettings[UpdateLocation] = "http://137.194.165.58:1337";
		mDefSettings[NetLocationActive] = "0";
		mDefSettings[DebugMode] = "1";
		mDefSettings[PcapSizeLimit] = "5000000"; // 5MB
#endif
	}
}

void trim(char * s)
{
	char * p = s;
	size_t l = strlen(p);
	while (l > 0 && isspace(p[l - 1])) p[--l] = 0;
	while (*p && isspace(*p)) ++p, --l;
	memmove(s, p, l + 1);
}

bool load(char *src, std::hash_map<std::string, std::string> &map)
{
	map.clear();

	// read the actual settings from the file
	FILE *f = NULL;
	fopen_s(&f, src, "r");
	if (f)
	{
		char szBuffer[1024] = { 0 };
		while (fgets(szBuffer, _countof(szBuffer), f))
		{
			trim(szBuffer);

			char * context = NULL;
			char * token = strtok_s(szBuffer, "=", &context);

			if (token != NULL)
			{
				char szKey[MAX_PATH] = { 0 };
				char szValue[MAX_PATH] = { 0 };

				strcpy_s(szKey, token);
				trim(szKey);

				token = strtok_s(NULL, "=", &context);

				if (token != NULL)
				{
					strcpy_s(szValue, token);
					trim(szValue);

					map[szKey] = szValue;
				}
			}
		}
		fclose(f);
		return true;
	}
	return false;
}


CSettings::CSettings() :
	version(0),
	filename("settings")
{
	mSettings = new std::hash_map<std::string, std::string>();
	mLongSettings = new std::hash_map<std::string, ULONG>();

	loadDefaults();
	ReloadSettings();
}

CSettings::CSettings(char *file) :
	version(0)
{
	sprintf_s(filename, "%s", file);
	mSettings = new std::hash_map<std::string, std::string>();
	mLongSettings = new std::hash_map<std::string, ULONG>();

	loadDefaults();
	ReloadSettings();
}

CSettings::~CSettings()
{
	delete mSettings;
	delete mLongSettings;
}

bool CSettings::ReloadSettings()
{
	bool res = false;
	if (load(filename, *mSettings) && HasKey(SettingsVersion)) {
		version = GetULong(SettingsVersion);
		res = true;
	}
	return res;
}

bool CSettings::HasKey(char *szKey) {
	bool found = (mSettings->find(szKey) != mSettings->end() || mDefSettings.find(szKey) != mDefSettings.end());
	return found;
}

bool CSettings::GetBoolean(char *szKey) {
	// if the key is missing this will evaluate to false !!
	return (GetULong(szKey) != 0);
}

ULONG CSettings::GetULong(char *szKey)
{
	ULONG lRes = 0L;
	if (mLongSettings->find(szKey) == mLongSettings->end())
	{
		const char *szValue = GetString(szKey);
		if (szValue)
		{
			char *stop;
			(*mLongSettings)[szKey] = strtoul(szValue, &stop, 10);
			lRes = (*mLongSettings)[szKey];
		}
	}
	else
	{
		lRes = (*mLongSettings)[szKey];
	}
	return lRes;
}

char *CSettings::GetString(char *szKey)
{
	char * pszResult = 0;
	if (mSettings->find(szKey) != mSettings->end())
	{
		pszResult = (char *)(*mSettings)[szKey].c_str();
	}
	else if (mDefSettings.find(szKey) != mDefSettings.end())
	{
		pszResult = (char *) mDefSettings[szKey].c_str();
	}
	return pszResult;
}
