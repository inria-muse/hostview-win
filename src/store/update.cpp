#include "StdAfx.h"
#include "update.h"
#include "wininet.h"
#include "Shellapi.h"
#include "Settings.h"

BOOL GetLatestProductVersion(char *szLatestProdVer, DWORD dwSize);

BOOL ExecuteAndWait(TCHAR *szProgram, TCHAR *szArgs);

CSettings settings;

BOOL CheckForUpdate()
{
	char szProdVer[MAX_PATH] = {0};
	char szLatestProdVer[MAX_PATH] = {0};

	if (GetProductVersion(szProdVer, _countof(szProdVer))
		&& GetLatestProductVersion(szLatestProdVer, _countof(szLatestProdVer)))
	{
		return strcmp(szProdVer, szLatestProdVer) != 0;
	}
	return FALSE;
}

BOOL DownloadUpdate(TCHAR *szUpdatePath, DWORD dwSize)
{
	const TCHAR * szTempName = _T("hvupdate");

	BOOL bResult = FALSE;

	TCHAR szTempPath[MAX_PATH] = {0}, szTempFile[MAX_PATH] = {0};
	GetTempPath(_countof(szTempPath), szTempPath);

	// TODO: should be in settings
	TCHAR szURL[MAX_PATH] = {0};
	_stprintf_s(szURL, L"%S/w%d.zip", settings.GetString(UpdateLocation), sizeof(int *) == 4 ? 32 : 64);
	_stprintf_s(szTempFile, _T("%s\\%s.zip"), szTempPath, szTempName);

	if (DownloadFile(szURL, szTempFile))
	{
		// TODO: unzip
		TCHAR szArgs[MAX_PATH] = {0};
		_stprintf_s(szArgs, _T("x -y %s\\%s.zip -o%s\\%s"), szTempPath, szTempName, szTempPath, szTempName);

		if (ExecuteAndWait(L"7za.exe", szArgs))
		{
			// TODO: build update path;
			_stprintf_s(szUpdatePath, dwSize, _T("%s\\%s\\update.bat"), szTempPath, szTempName);
			bResult = TRUE;
		}
	}
	
	DeleteFile(szTempFile);

	return bResult;
}

BOOL ExecuteAndWait(TCHAR *szProgram, TCHAR *szCmdLine)
{
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = szProgram;
	ShExecInfo.lpParameters = szCmdLine;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL; 
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	return TRUE;
}

UPDATEAPI BOOL DownloadFile(TCHAR *szUrl, TCHAR *szFile)
{
	TCHAR szBuffer[4096] = {0};
	BOOL bResult = FALSE;

	HINTERNET hConnect = NULL;
	HINTERNET hFile = NULL;

	hConnect = InternetOpen(L"HostView", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	if (hConnect)
	{
		DWORD dwTimeout = 1000;
		InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT,
			&dwTimeout, sizeof(dwTimeout));

		hFile = InternetOpenUrl(hConnect, szUrl, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, NULL);
		if (hFile)
		{
			TCHAR szResponseText[MAX_PATH] = {0};
			DWORD dwSize = sizeof(szResponseText);

			// check HTTP error code;
			if (HttpQueryInfo(hFile, HTTP_QUERY_STATUS_CODE, szResponseText, &dwSize, NULL))
			{
				DWORD dwStatusCode = _ttoi(szResponseText);

				if (dwStatusCode == HTTP_STATUS_OK)
				{
					FILE *f = NULL;
					_tfopen_s(&f, szFile, L"wb");

					if (f)
					{
						DWORD dwRead = sizeof(szBuffer);

						while (InternetReadFile(hFile, szBuffer, dwRead, &dwRead))
						{
							if (!dwRead)
								break;
							fwrite(szBuffer, dwRead, 1, f);
						}

						fclose(f);
						bResult = TRUE;
					}
				}
			}

			InternetCloseHandle(hFile);
		}
		InternetCloseHandle(hConnect);
	}

	return bResult;
}

BOOL GetProductVersion(char *szProductVersion, DWORD dwSize)
{
	BOOL bResult = FALSE;
	TCHAR szCurrPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szCurrPath, _countof(szCurrPath));
	
	DWORD verHandle = NULL;
	UINT size = 0;
	LPBYTE lpBuffer = NULL;
	DWORD verSize = GetFileVersionInfoSize(szCurrPath, &verHandle);
	
	if (verSize != NULL)
	{
		LPSTR verData = new char[verSize];
		
		if (GetFileVersionInfo(szCurrPath, verHandle, verSize, verData))
		{
			if (VerQueryValue(verData, L"\\",(VOID FAR* FAR*)&lpBuffer,&size))
			{
				if (size)
				{
					VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
					if (verInfo->dwSignature == 0xfeef04bd)
					{
						sprintf_s(szProductVersion, dwSize, "%d,%d,%d,%d",
							(verInfo->dwProductVersionMS >> 16) & 0xffff, (verInfo->dwProductVersionMS >>  0) & 0xffff,
							(verInfo->dwProductVersionLS >> 16) & 0xffff,(verInfo->dwProductVersionLS >>  0) & 0xffff);
						bResult = TRUE;
					}
				}
			}
		}
		delete [] verData;
	}

	return bResult;
}

BOOL GetLatestProductVersion(char *szLatestProdVer, DWORD dwSize)
{
	BOOL bResult = FALSE;

	TCHAR szURL[MAX_PATH] = {0};
	_stprintf_s(szURL, _T("%S/version"), settings.GetString(UpdateLocation));

	HINTERNET hConnect = NULL;
	HINTERNET hFile = NULL;

	hConnect = InternetOpen(L"HostView", NULL, NULL, NULL, NULL);

	DWORD dwTimeOut = ConnectionTimeout;
	InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeOut, sizeof(dwTimeOut));

	if (hConnect)
	{
		hFile = InternetOpenUrl(hConnect, szURL, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, NULL);
		if (hFile)
		{
			DWORD dwRead = dwSize;

			InternetReadFile(hFile, szLatestProdVer, dwSize, &dwRead);
			bResult = dwRead > 0;

			InternetCloseHandle(hFile);
		}
		InternetCloseHandle(hConnect);
	}

	return bResult;
}


VOID PullFile(TCHAR *szFilePath)
{
	TCHAR szURL[MAX_PATH] = {0};
	_stprintf_s(szURL, _T("%S/%s"), settings.GetString(UpdateLocation), szFilePath);

	// TODO: check return
	TCHAR szTempPath[MAX_PATH] = {0};
	_stprintf_s(szTempPath, _T("%s.tmp"), szFilePath);

	if (DownloadFile(szURL, szTempPath))
	{
		MoveFileEx(szTempPath, szFilePath, MOVEFILE_REPLACE_EXISTING);
	}
	else
	{
		DeleteFile(szTempPath);
	}
}

UPDATEAPI bool QueryPublicInfo(std::vector<std::string> * &info)
{
	info = new std::vector<std::string>();

	bool bResult = false;

	TCHAR szURL[MAX_PATH] = {0};
	_tcscpy_s(szURL, _T("https://muse.inria.fr/hostview/location"));

	HINTERNET hConnect = NULL;
	HINTERNET hFile = NULL;

	hConnect = InternetOpen(L"HostView", NULL, NULL, NULL, NULL);

	DWORD dwTimeout = 1000;
	InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT,
		&dwTimeout, sizeof(dwTimeout));

	char szInfo[2048] = {0};

	DWORD dwSize = _countof(szInfo);
	DWORD dwRead = dwSize;

	if (hConnect)
	{
		hFile = InternetOpenUrl(hConnect, szURL, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, NULL);
		if (hFile)
		{
			InternetReadFile(hFile, szInfo, dwSize, &dwRead);

			bResult = dwRead > 0;

			InternetCloseHandle(hFile);
		}
		InternetCloseHandle(hConnect);
	}

	if (bResult)
	{
		char *ctx = 0;
		char *token = strtok_s(szInfo, ",", &ctx);
		while (token)
		{
			info->push_back(token);
			token = strtok_s(0, ",", &ctx);
		}
	}

	return bResult;
}

UPDATEAPI void FreePublicInfo(std::vector<std::string> * &info)
{
	if (info)
	{
		info->clear();
		delete info;
		info = 0;
	}
}

