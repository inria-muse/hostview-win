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

#include <windows.h>
#include <wininet.h>
#include <shellapi.h>

#include "update.h"
#include "trace.h"
#include "product.h"

// connection handle
HINTERNET HttpConnect() {
	HINTERNET hConnect = NULL;
	TCHAR ua[512] = { 0 };
	_stprintf_s(ua, 512, _T("HostView Windows %S"), ProductVersionStr);

	hConnect = InternetOpen(ua, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	if (hConnect)
	{
		DWORD dwTimeout = 2500;
		InternetSetOption(hConnect, INTERNET_OPTION_CONNECT_TIMEOUT,
			&dwTimeout, sizeof(dwTimeout));
	}
	return hConnect;
}

// request handle
HINTERNET HttpGet(HINTERNET hConnect, TCHAR *szUrl) {
	HINTERNET hFile = NULL;
	Debug("http get %S", szUrl);
	return InternetOpenUrl(hConnect, szUrl, NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, NULL);
}

BOOL GetLatestProductVersion(char *updateBaseUrl, char *szLatestProdVer, DWORD dwSize)
{
	BOOL bResult = FALSE;

	TCHAR szUrl[MAX_PATH] = { 0 };
	_stprintf_s(szUrl, _T("%S/version"), updateBaseUrl);

	HINTERNET hConnect = HttpConnect();
	if (hConnect) {
		HINTERNET hFile = HttpGet(hConnect, szUrl);
		if (hFile)
		{
			// check HTTP error code;
			TCHAR szResponseText[MAX_PATH] = { 0 };
			DWORD dwRsSize = sizeof(szResponseText);
			if (HttpQueryInfo(hFile, HTTP_QUERY_STATUS_CODE, szResponseText, &dwRsSize, NULL))
			{
				DWORD dwStatusCode = _ttoi(szResponseText);
				Debug("http get response code %S", szResponseText);
				if (dwStatusCode == HTTP_STATUS_OK)
				{
					DWORD dwRead = dwSize;
					InternetReadFile(hFile, szLatestProdVer, dwSize, &dwRead);
					Debug("http get response size %lu", dwRead);
					bResult = (dwRead > 0);
				}
			}
			InternetCloseHandle(hFile);
		}
		InternetCloseHandle(hConnect);
	}

	return bResult;
}

BOOL ExecuteAndWait(TCHAR *szProgram, TCHAR *szCmdLine)
{
	SHELLEXECUTEINFO ShExecInfo = { 0 };
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

BOOL CheckForUpdate(char *updateBaseUrl)
{
	char szProdVer[MAX_PATH] = { 0 };
	char szLatestProdVer[MAX_PATH] = { 0 };

	bool res = (GetProductVersion(szProdVer, _countof(szProdVer))
		&& GetLatestProductVersion(updateBaseUrl, szLatestProdVer, _countof(szLatestProdVer)));

	Debug("check update: current=%s latest=%s [succ=%d]", szProdVer, szLatestProdVer, res);
	if (res)
	{
		return (strcmp(szProdVer, szLatestProdVer) != 0);
	}
	return FALSE;
}

BOOL DownloadUpdate(char *updateBaseUrl, TCHAR *szUpdatePath, DWORD dwSize)
{
	BOOL bResult = FALSE;

	const TCHAR * szTempName = _T("hvupdate");
	TCHAR szTempPath[MAX_PATH] = {0}, szTempFile[MAX_PATH] = {0};
	GetTempPath(_countof(szTempPath), szTempPath);

	TCHAR szURL[MAX_PATH] = {0};
	_stprintf_s(szURL, L"%S/w%d.zip", updateBaseUrl, sizeof(int *) == 4 ? 32 : 64);
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

BOOL DownloadFile(TCHAR *szUrl, TCHAR *szFile)
{
	TCHAR szBuffer[4096] = {0};
	BOOL bResult = FALSE;

	TCHAR szTempPath[MAX_PATH] = { 0 };
	_stprintf_s(szTempPath, _T("%s.download"), szFile);

	HINTERNET hConnect = NULL;
	HINTERNET hFile = NULL;

	hConnect = HttpConnect();
	if (hConnect)
	{
		hFile = HttpGet(hConnect, szUrl);
		if (hFile)
		{
			TCHAR szResponseText[MAX_PATH] = {0};
			DWORD dwSize = sizeof(szResponseText);

			// check HTTP error code;
			if (HttpQueryInfo(hFile, HTTP_QUERY_STATUS_CODE, szResponseText, &dwSize, NULL))
			{
				DWORD dwStatusCode = _ttoi(szResponseText);
				Debug("http get response code %S", szResponseText);

				if (dwStatusCode == HTTP_STATUS_OK)
				{
					FILE *f = NULL;
					_tfopen_s(&f, szTempPath, L"wb");

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

	// TODO: could check that we received all bytes based on content-length ?
	if (bResult) {
		MoveFileEx(szTempPath, szFile, MOVEFILE_REPLACE_EXISTING);
	} else {
		DeleteFile(szTempPath);
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

BOOL GetProductVersionStr(char *szProductVersion, DWORD dwSize)
{
	BOOL bResult = FALSE;
	TCHAR szCurrPath[MAX_PATH] = { 0 };
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
			if (VerQueryValue(verData, L"\\", (VOID FAR* FAR*)&lpBuffer, &size))
			{
				if (size)
				{
					VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
					if (verInfo->dwSignature == 0xfeef04bd)
					{
						sprintf_s(szProductVersion, dwSize, "%d.%d.%d.%d",
							(verInfo->dwProductVersionMS >> 16) & 0xffff, (verInfo->dwProductVersionMS >> 0) & 0xffff,
							(verInfo->dwProductVersionLS >> 16) & 0xffff, (verInfo->dwProductVersionLS >> 0) & 0xffff);
						bResult = TRUE;
					}
				}
			}
		}
		delete[] verData;
	}

	return bResult;
}

BOOL CheckForResourceUpdates(char *updateBaseUrl) {
	TCHAR szURL[MAX_PATH] = { 0 };

	_stprintf_s(szURL, _T("%S/settings"), updateBaseUrl);
	BOOL res = DownloadFile(szURL, _T(".\\settings"));

#ifdef DOWNLOADHTMLUPDATES
	_stprintf_s(szURL, _T("%S/html/esm_wizard.html"), updateBaseUrl);
	DownloadFile(szURL, _T(".\\html\\esm_wizard.html"));

	_stprintf_s(szURL, _T("%S/html/network_wizard.html"), updateBaseUrl);
	DownloadFile(szURL, _T(".\\html\\network_wizard.html"));
#endif

	return res;
}

bool QueryPublicInfo(char *locationApiUrl, std::vector<std::string> * &info)
{
	bool bResult = false;
	info = new std::vector<std::string>();

	TCHAR szURL[MAX_PATH] = { 0 };
	_stprintf_s(szURL, _T("%S"), locationApiUrl);

	HINTERNET hConnect = NULL;
	HINTERNET hFile = NULL;

	char szInfo[2048] = {0};
	DWORD dwSize = _countof(szInfo);
	DWORD dwRead = dwSize;

	hConnect = HttpConnect();
	if (hConnect)
	{
		hFile = HttpGet(hConnect, szURL);
		if (hFile)
		{
			TCHAR szResponseText[MAX_PATH] = { 0 };
			DWORD dwSize = sizeof(szResponseText);

			// check HTTP error code;
			if (HttpQueryInfo(hFile, HTTP_QUERY_STATUS_CODE, szResponseText, &dwSize, NULL))
			{
				DWORD dwStatusCode = _ttoi(szResponseText);
				Debug("http get response code %S", szResponseText);
				if (dwStatusCode == HTTP_STATUS_OK)
				{
					if (InternetReadFile(hFile, szInfo, dwSize, &dwRead))
					{
						Debug("http get response size %lu", dwRead);
						bResult = dwRead > 0;
					}
				}
			}
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

void FreePublicInfo(std::vector<std::string> * &info)
{
	if (info)
	{
		info->clear();
		delete info;
		info = 0;
	}
}

