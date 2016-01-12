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
#include <Shlwapi.h>
#include "Upload.h"
#include "product.h"

CUpload::CUpload(void)
	: curl(NULL)
{
	char ua[512] = {0};
	sprintf_s(ua, 512, "Hostview Windows %s", ProductVersionStr);

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "[upload] curl_easy_init failed\n");
		return;
	}

	// common options
	curl_easy_setopt(curl, CURLOPT_USERAGENT, ua);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20L);
//	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
}

CUpload::~CUpload(void)
{
	if (curl)
		curl_easy_cleanup(curl);
}

size_t read_file(char *bufptr, size_t size, size_t nitems, void *userp) {
	size_t nRead = 0;
	FILE *fd = (FILE *)userp;
	if (fd && !feof(fd)) {
		nRead = fread(bufptr, size, nitems, fd);
	}
	fprintf(stderr, "[upload] read %d bytes\n", nRead);
	return nRead;
}

bool CUpload::SubmitFile(char *server, char *userId, char *deviceId, char *fileName)
{
	struct curl_slist *headers = NULL;
	FILE *f = NULL;
	size_t nTotalFileSize = 0;
	char url[1024] = { 0 };
	CURLcode res;

	bool result = false;

	if (!curl) {
		fprintf(stderr, "[upload] curl not initialized\n");
		return result;
	}

	fopen_s(&f, fileName, "rb");
	if (!f) {
		fprintf(stderr, "[upload] failed to open file %s\n", fileName);
		return result;
	}

	fseek(f, 0, SEEK_END);
	nTotalFileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	sprintf_s(url, 1024, "%s/%s/%s/%s", server, ProductVersionStr, deviceId, PathFindFileNameA(fileName));

	headers = curl_slist_append(headers, "Expect:"); // remove default Expect header

	fprintf(stderr, "[upload] submit %s to %s\n", fileName, url);

	// FIXME: only watch for the limit if we are sending large enough file ..  ?!
	if (nTotalFileSize > settings.GetULong(UploadLowSpeedLimit)*settings.GetULong(UploadLowSpeedTime)) {
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, settings.GetULong(UploadLowSpeedLimit));
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, settings.GetULong(UploadLowSpeedTime));
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_file);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, f);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)nTotalFileSize);
#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "[upload] curl_easy_perform failed: %s\n", curl_easy_strerror(res));
		result = false;
	}
	else {
		fprintf(stderr, "[upload] curl_easy_perform success\n");
		result = true;
	}

	curl_slist_free_all(headers);
	
	fclose(f);

	return result;
}

bool CUpload::ZipFile(char *src, char *dest)
{
	// FIXME: there's no library interface to this compresssion tool we could use ?!?
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
