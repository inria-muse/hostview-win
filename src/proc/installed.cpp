#include "stdafx.h"
#include "proc.h"

// TODO: better use built-in MsiEnumProducts

#define InstalledApplications64	_T("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
#define InstalledApplications	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

#define DisplayName	_T("DisplayName")
#define DisplayIcon	_T("DisplayIcon")

#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif

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
