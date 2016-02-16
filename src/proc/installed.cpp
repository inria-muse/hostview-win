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
#include "proc.h"

// TODO: better use built-in MsiEnumProducts

#define InstalledApplications64	_T("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
#define InstalledApplications	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

#define DisplayName	_T("DisplayName")
#define DisplayIcon	_T("DisplayIcon")

typedef std::vector<std::tstring> AppListT;

void QueryRegFolder(TCHAR *szPath, AppListT &vPaths, AppListT &vDescriptions)
{
	HKEY hKey = 0;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return;

	DWORD dwIndex = 0;
	LONG lRet = 0;
	DWORD cbName = MAX_PATH;
	TCHAR szSubKeyName[MAX_PATH];

	while ((lRet = ::RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &cbName, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
	{
		// Do we have a key to open?
		if (lRet == ERROR_SUCCESS)
		{
			// Open the key and get the value
			HKEY hItem = 0;
			if (::RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_READ, &hItem) != ERROR_SUCCESS)
				continue;

			TCHAR szDisplayName[MAX_PATH] = {0};
			TCHAR szAppPath[MAX_PATH] = {0};
			DWORD dwSize = sizeof(szDisplayName);
			DWORD dwType = 0;

			BOOL bAdd = ::RegQueryValueEx(hItem, DisplayName, NULL, &dwType, (LPBYTE)&szDisplayName, &dwSize) == ERROR_SUCCESS;

			dwSize = sizeof(szAppPath);

			if ((bAdd &= ::RegQueryValueEx(hItem, DisplayIcon, NULL, &dwType, (LPBYTE)&szAppPath, &dwSize) == ERROR_SUCCESS))
			{
				if (_tcsstr(szAppPath, _T(",")))
				{
					TCHAR *p = szAppPath;
					while (*p)
					{
						if (*p == ',' || *p == '"')
						{
							*p = 0;
							break;
						}
						p ++;
					}
				}

				vDescriptions.push_back(szDisplayName);
				vPaths.push_back(szAppPath);
			}

			::RegCloseKey(hItem);
		}
		dwIndex++;
		cbName = MAX_PATH;
	}
	::RegCloseKey(hKey);
}

PROCAPI int QueryApplicationsList(TCHAR ** &pszPaths, TCHAR ** &pszDescriptions)
{
	AppListT vPaths, vDesc;
	QueryRegFolder(InstalledApplications, vPaths, vDesc);
	QueryRegFolder(InstalledApplications64, vPaths, vDesc);

	// TODO: sort stuff
	for (size_t i = 0; i < vPaths.size() - 1; i ++)
	{
		for (size_t j = i + 1; j < vPaths.size(); j ++)
		{
			if (vDesc[i].compare(vDesc[j]) > 0)
			{
				std::tstring temp = vPaths[j];
				vPaths[j] = vPaths[i];
				vPaths[i] = temp;

				temp = vDesc[j];
				vDesc[j] = vDesc[i];
				vDesc[i] = temp;
			}
		}
	}

	pszPaths = new TCHAR *[vPaths.size()];
	pszDescriptions = new TCHAR *[vDesc.size()];
	for (size_t i = 0; i < vPaths.size(); i ++)
	{
		pszPaths[i] = new TCHAR[vPaths[i].length() + 1];
		_tcscpy_s(pszPaths[i], vPaths[i].length() + 1, vPaths[i].c_str());

		pszDescriptions[i] = new TCHAR[vDesc[i].length() + 1];
		_tcscpy_s(pszDescriptions[i], vDesc[i].length() + 1, vDesc[i].c_str());
	}

	return (int) vPaths.size();
}

PROCAPI void ReleaseApplicationsList(int nCount, TCHAR ** &pszPaths, TCHAR ** &pszDescriptions)
{
	for (int i = 0; i < nCount; i ++)
	{
		delete [] pszPaths[i];
		delete [] pszDescriptions[i];
	}

	delete [] pszPaths;
	delete [] pszDescriptions;

	pszPaths = 0;
	pszDescriptions = 0;
}

PROCAPI void PullInstalledApps(TCHAR *szPath, DWORD dwSize)
{
	TCHAR szCurrDir[MAX_PATH] = { 0 };
	GetCurrentDirectory(_countof(szCurrDir), szCurrDir);

	TCHAR szOut[MAX_PATH] = { 0 };
	_stprintf_s(szPath, dwSize, _T("%s\\%s\\%d"), szCurrDir, TEMP_PATH, GetTickCount());

	TCHAR ** pszApps = 0, ** pszDesc = 0;
	int nCount = QueryApplicationsList(pszApps, pszDesc);

	ImpersonateCurrentUser();

	FILE *f = 0;
	_tfopen_s(&f, szPath, _T("w"));
	if (f)
	{
		for (int i = 0; i < nCount; i++)
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

	// starts a thread to update icons on the background
	QueryIcons(nCount, pszApps);

	ReleaseApplicationsList(nCount, pszApps, pszDesc);
}
