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
#include "Upload.h"
#include "Settings.h"

CUpload::CUpload(void)
	: curl(NULL)
{
	curl = curl_easy_init();
}

CUpload::~CUpload(void)
{
	curl_easy_cleanup(curl);
}

bool CUpload::SubmitFile(char *server, char *userId, char *deviceId, char *fileName)
{
	bool result = true;

	return result;
}

bool CUpload::ZipFile(char *src, char *dest)
{
	// FIXME: there's no library interface to this compresssion tool we could use ?
	TCHAR szCmdLine[1024] = { 0 };
	_stprintf_s(szCmdLine, _T("7za.exe a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on %S %S"), dest, src);

	PROCESS_INFORMATION proc = { 0 };

	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);

	// create the process
	if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &proc))
	{
		// wait for process
		WaitForSingleObject(proc.hProcess, INFINITE);

		// get exit code
		DWORD dwExit = 0;
		BOOL bResult = GetExitCodeProcess(proc.hProcess, &dwExit);

		CloseHandle(proc.hProcess);
		CloseHandle(proc.hThread);

		return bResult ? true : false;
	}

	return false;
}

