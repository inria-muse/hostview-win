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
#include "questionnaire.h"
#include "comm.h"
#include "trace.h"

std::map<std::tstring, AppListT> mUserApps;
std::map<std::tstring, std::map<std::tstring, AppListT>> mUserBrowserTabs;

const int nMaxApps = 10;

AppListT UpdateMostRecentApps(AppListT &vApps, TCHAR *szApp, TCHAR *szDescription)
{
	// remove any existing process with same name
	for (size_t i = 0; i < vApps.size(); i ++)
	{
		App app = vApps[i];

		// same process or site name
		// but title or description might differ
		if (app.app == szApp)
		{
			vApps.erase(vApps.begin() + i);
			break;
		}
	}

	if (vApps.size())
	{
		// last app used also 1ms ago
		vApps[vApps.size() - 1].time = GetTickCount();
	}

	vApps.push_back(App(szApp, szDescription, L"", GetTickCount()));

	if (vApps.size() > nMaxApps)
	{
		return AppListT(vApps.end() - nMaxApps, vApps.end());
	}
	else
	{
		return vApps;
	}
}

void ESMAPI UpdateQuestionnaireTabsList(TCHAR *szUser, TCHAR *szHost, TCHAR *szBrowser)
{
	mUserBrowserTabs[szUser][szBrowser] = 
		UpdateMostRecentApps(mUserBrowserTabs[szUser][szBrowser], szHost, szBrowser);
}

void ESMAPI UpdateQuestionnaireAppsList(TCHAR *szUser, TCHAR *szApp, TCHAR *szDescription)
{
	mUserApps[szUser] = UpdateMostRecentApps(mUserApps[szUser], szApp, szDescription);
}

void GenerateQuestionnaireCommand(TCHAR *szUser, BOOL fOnDemand, TCHAR *szCmdLine, size_t nSize)
{
	_stprintf_s(szCmdLine, nSize, _T("HostView.exe %s %s"), fOnDemand ? _T("/odm") : _T(""), _T("/esm"));
}

bool IsBrowser(App &app)
{
	// TODO: add others?!
	return app.app == _T("iexplore.exe")
		|| app.app == _T("chrome.exe")
		|| app.app == _T("firefox.exe");
}

void SaveAppsList(TCHAR *szUser, TCHAR *szFilename, size_t nSize)
{
	TCHAR szCurrDir[MAX_PATH] =  {0};
	GetCurrentDirectory(_countof(szCurrDir), szCurrDir);

	_stprintf_s(szFilename, nSize, _T("%s\\%s\\%d"), szCurrDir, TEMP_PATH, GetTickCount());

	FILE * f = NULL;
	_tfopen_s(&f, szFilename, _T("w"));
	if (f)
	{
		std::vector<App> vApps = mUserApps[szUser];

		DWORD dwNow = GetTickCount();

		for (int i = (int) vApps.size() - 1; i >= 0; i --)
		{
			App app = vApps[i];

			// if it's neither the only one nor in the last minute
			// TODO: extract this: '1 minute' in the settings file
			if (dwNow - app.time > 1 * 60 * 1000 && i != (int) vApps.size() - 1)
				continue;

			bool bHasTabs = false;
			if (IsBrowser(app))
			{
				// add tabs with extra this app's description
				std::vector<App> vTabs = mUserBrowserTabs[szUser][app.app.c_str()];
				for (int i = (int) vTabs.size() - 1; i >= 0; i --)
				{
					bHasTabs = true;

					App tab = vTabs[i];
					if (dwNow - tab.time > 1 * 60 * 1000 && i != (int) vTabs.size() - 1)
						continue;

					_ftprintf_s(f, _T("%s\n"), tab.app.c_str());
					_ftprintf_s(f, _T("%s\n"), tab.app.c_str());
					_ftprintf_s(f, _T("%s\n"), app.description.c_str());
				}
			}
			
			if (!bHasTabs)
			{
				_ftprintf_s(f, _T("%s\n"), app.app.c_str());
				_ftprintf_s(f, _T("%s\n"), app.description.c_str());
				_ftprintf_s(f, _T("\n"));
			}
		}
		fclose(f);
	}
}

// gets a line from file without \r\n
void GetFileLine(FILE *f, TCHAR szLine[MAX_PATH])
{
	_fgetts(szLine, MAX_PATH, f);

	if (_tcslen(szLine) && szLine[_tcslen(szLine) - 1] == '\n')
	{
		szLine[_tcslen(szLine) - 1] = 0;
	}
}

void LoadAppList(TCHAR *szPath, AppListT* &appList)
{
	appList = new AppListT();

	FILE * f = NULL;
	_tfopen_s(&f, szPath, _T("r"));

	if (f)
	{
		TCHAR szLine[MAX_PATH] = {0};

		while (!feof(f))
		{
			GetFileLine(f, szLine);

			if (!feof(f))
			{
				std::tstring strapp = szLine;

				GetFileLine(f, szLine);
				std::tstring strdesc = szLine;

				GetFileLine(f, szLine);
				std::tstring strextra = szLine;

				appList->push_back(App(strapp, strdesc, strextra, 0));
			}
		}
		fclose(f);
	}

#ifndef _DEBUG
	DeleteFile(szPath);
#endif
}

void ClearAppList(AppListT* &appList)
{
	if (appList)
	{
		delete appList;
		appList = NULL;
	}
}

void SubmitQuestionnaire(const TCHAR *szResult)
{
	TCHAR szCurrDir[MAX_PATH] =  {0};
	TCHAR szTempFile[MAX_PATH] = {0};

	GetCurrentDirectory(_countof(szCurrDir), szCurrDir);
	_stprintf_s(szTempFile, _T("%s\\%s\\%llu_questionnaire.json"), szCurrDir, TEMP_PATH, GetHiResTimestamp());

	FILE * f = NULL;
	_tfopen_s(&f, szTempFile, _T("w"));
	if (f)
	{
		_ftprintf_s(f, _T("%s"), szResult);
		fclose(f);
		
		Message message(MessageQuestionnaireDone);
		_tcscpy_s(message.szUser, szTempFile);
		SendServiceMessage(message);
	}
}

size_t QueryQuestionnaireCounter()
{
	HKEY hKey = 0;
	RegCreateKey(HKEY_LOCAL_MACHINE, HOSTVIEW_REG, &hKey);

	DWORD dwVal = 0;
	DWORD dwSize = sizeof(dwVal);
	DWORD dwType = REG_DWORD;
	RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE) &dwVal, &dwSize);
	RegCloseKey(hKey);

	TCHAR szDate[MAX_PATH] = {0};

	if (dwVal != 0)
	{
		_stprintf_s(szDate, L"%04d/%02d/%02d", (dwVal >> 14) & (2048 - 1) ,
			(dwVal >> 10) & (16 - 1),
			(dwVal >> 5) & (32 - 1));
	}

	size_t nCounter = dwVal & 31;

	SYSTEMTIME st;
	TCHAR szNow[MAX_PATH] = {0};

	GetSystemTime(&st);
	_stprintf_s(szNow, L"%04d/%02d/%02d", st.wYear, st.wMonth, st.wDay);

	if (_tcscmp(szNow, szDate) == 0)
	{
		return nCounter;
	}
	return 0;
}

void SetQuestionnaireCounter(size_t nCounter)
{
	HKEY hKey = 0;
	RegCreateKey(HKEY_LOCAL_MACHINE, HOSTVIEW_REG, &hKey);

	SYSTEMTIME st;
	GetSystemTime(&st);
	
	TCHAR szNow[MAX_PATH] = {0};
	_stprintf_s(szNow, L"%04d/%02d/%02d", st.wYear, st.wMonth, st.wDay);

	DWORD dwVal = (DWORD) nCounter
		| (st.wDay << 5)
		| (st.wMonth << 10)
		| (st.wYear << 14);

	RegSetValueEx(hKey, NULL, NULL, REG_DWORD, (LPBYTE) &dwVal, sizeof(dwVal));
	RegCloseKey(hKey);
}
