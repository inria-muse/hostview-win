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

#include <windows.h>
#include <shlwapi.h>

#include "Upload.h"
#include "trace.h"
#include "product.h"

// read helper for curl uploads
size_t read_file(char *bufptr, size_t size, size_t nitems, void *userp) {
	size_t nRead = 0;
	FILE *fd = (FILE *)userp;
	if (fd && !feof(fd)) {
		nRead = fread(bufptr, size, nitems, fd);
	}
	return nRead;
}

// assumues src is already in .\submit !
bool createzip(char *src)
{
	char dest[MAX_PATH] = { 0 };
	sprintf_s(dest, "%s.zip", src);

	// FIXME: there's no library interfac we could use ?!? this is quite horrible ...
	TCHAR szCmdLine[1024] = { 0 };
	_stprintf_s(szCmdLine, _T("7za.exe a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on %S %S"), dest, src);

	PROCESS_INFORMATION proc = { 0 };
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &proc))
	{
		DWORD dwExit = 0;
		WaitForSingleObject(proc.hProcess, INFINITE);
		BOOL bResult = GetExitCodeProcess(proc.hProcess, &dwExit);
		CloseHandle(proc.hProcess);
		CloseHandle(proc.hThread);

		// make sure no broken zip files are left behind
		if (dwExit != 0) {
			DeleteFileA(dest);
		}
		else {
			DeleteFileA(src); // remove src since we now have the zip
		}

		return (dwExit == 0);
	}

	return false;
}

CUpload::CUpload()
	: curl(NULL), 
	retryCount(0),
	lastRetry(0)
{
	char ua[512] = {0};
	sprintf_s(ua, 512, "HostView Windows %s", ProductVersionStr);

	curl = curl_easy_init();
	if (!curl) {
		Debug("[upload] curl_easy_init failed");
		return;
	}

	// common options
	curl_easy_setopt(curl, CURLOPT_USERAGENT, ua);

	if (settings.HasKey(UploadVerifyPeer))
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, settings.GetULong(UploadVerifyPeer));

	if (settings.HasKey(UploadLowSpeedLimit))
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, settings.GetULong(UploadLowSpeedLimit));

	if (settings.HasKey(UploadLowSpeedTime))
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, settings.GetULong(UploadLowSpeedTime));

	if (settings.HasKey(UploadMaxSendSpeed))
		curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, settings.GetULong(UploadMaxSendSpeed));

#ifdef _DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
}

CUpload::~CUpload(void)
{
	if (curl)
		curl_easy_cleanup(curl);
}

bool CUpload::SubmitFile(const char *fileName, const char *deviceId)
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

	sprintf_s(url, 1024, "%s/%s/%s/%s", settings.GetString(SubmitServer), ProductVersionStr, deviceId, PathFindFileNameA(fileName));

	fprintf(stderr, "[upload] submit %s to %s %zd bytes\n", fileName, url, nTotalFileSize);

	headers = curl_slist_append(headers, "Expect:"); // remove default Expect header

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_file);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, f);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)nTotalFileSize);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		Debug("[upload] curl_easy_perform failed: %s", curl_easy_strerror(res));
		result = false;
	}
	else {
		Debug("[upload] curl_easy_perform success");
		result = true;
	}

	curl_slist_free_all(headers);
	
	fclose(f);

	return result;
}

// static helper used by all data collectors to add new files to '\submit'.
bool MoveFileToSubmit(const char *file, bool debug) {
	if (!PathFileExistsA(file)) {
		return false;
	}

	if (debug) {
		// keep a local copy of all files
		if (!PathFileExistsA(".\\debug")) {
			CreateDirectoryA(".\\debug", NULL);
		}
		char debugFile[MAX_PATH] = { 0 };
		sprintf_s(debugFile, ".\\debug\\%s", PathFindFileNameA(file));
		CopyFileA(file, debugFile, false);
	}

	char sFile[MAX_PATH] = { 0 };
	sprintf_s(sFile, "%s\\%s", SUBMIT_DIRECTORY, PathFindFileNameA(file));
	if (!MoveFileA(file, sFile)) {
		Debug("[upload] failed to move file %s to %s %d", file, sFile, GetLastError());
		return false;
	}

	if (!createzip(sFile)) {
		// TODO: this will leave unzipped file to submit -- so upload needs to make sure things are zipped
		Debug("[upload] failed to zip file %s.zip", sFile);
		return false;
	}

	Trace("Add submit file %s.zip", sFile);
	return true;
}

bool CUpload::TrySubmit(const char *deviceId) {
	char szFilename[MAX_PATH] = { 0 };
	WIN32_FIND_DATAA wfd;
	DWORD dwNow = GetTickCount();

	ULONG maxRetries = settings.GetULong(AutoSubmitRetryCount);
	if (retryCount >= maxRetries) {
		return false; // we have reached the max retries - stop trying to submit
	}

	// is it time yet ?
	if (lastRetry > 0 && ((dwNow - lastRetry) < settings.GetULong(AutoSubmitInterval)))
		return true; // try again later

	// ok lets try
	retryCount += 1;
	lastRetry = dwNow;

	int submittedFiles = 0;
	bool result = true;
	HANDLE hFind = FindFirstFileA(SUBMIT_DIRECTORY_GLOB, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.cFileName[0] == '.')
			{
				// just ignore . & ..
				continue;
			}

			sprintf_s(szFilename, "%s\\%s", SUBMIT_DIRECTORY, wfd.cFileName);

			// TODO: we need this here now as move/copy to submit may leave unzipped files to submit ..
			if (strcmp(PathFindExtensionA(szFilename), ".zip") != 0)
			{
				if (!createzip(szFilename))
				{
					Debug("[upload] failed to create zip file %s.zip", szFilename);
					continue; // move forward and try others ..
				}
				else {
					sprintf_s(szFilename, "%s\\%s.zip", SUBMIT_DIRECTORY, wfd.cFileName);
				}
			}

			result = SubmitFile(szFilename, deviceId);
			if (result)
			{
				Trace("Submitted file %s.", szFilename);
				submittedFiles += 1;
				DeleteFileA(szFilename);
			}

		} while (result && FindNextFileA(hFind, &wfd));

		FindClose(hFind);
	}

	bool doretry = true;
	if (result) {
		if (submittedFiles>0)
			Trace("Data submission ok (retry count: %d, files: %d)", retryCount, submittedFiles);
		doretry = false; // stop here
	}
	else {
		// error in sending - should retry
		Trace("Data submission failed (retry count: %d, files: %d)", retryCount, submittedFiles);
	}
	
	if (retryCount == maxRetries) {
		Trace("Data submission done (reached max retry count)", maxRetries);
		CheckDiskUsage();
		doretry = false; // stop here
	}

	return doretry;
}

void removeoldest()
{
	char szOldestFilename[MAX_PATH] = { 0 };

	SYSTEMTIME stNow;
	GetSystemTime(&stNow);

	FILETIME ftOldest;
	SystemTimeToFileTime(&stNow, &ftOldest);

	WIN32_FIND_DATAA wfd;
	HANDLE hFind = FindFirstFileA(SUBMIT_DIRECTORY_GLOB, &wfd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.cFileName[0] == '.')
			{
				// just ignore . & ..
				continue;
			}

			if (CompareFileTime(&wfd.ftCreationTime, &ftOldest) < 0)
			{
				ftOldest = wfd.ftCreationTime;
				sprintf_s(szOldestFilename, "%s\\%s", SUBMIT_DIRECTORY, wfd.cFileName);
			}
		} while (FindNextFileA(hFind, &wfd));

		FindClose(hFind);
	}

	if (strlen(szOldestFilename))
	{
		DeleteFileA(szOldestFilename);
		Trace("Remove old file %s.", szOldestFilename);
	}
}

ULONGLONG getUsage()
{
	ULONGLONG ulUsedSpace = 0L;
	WIN32_FIND_DATAA wfd;
	HANDLE hFind = FindFirstFileA(SUBMIT_DIRECTORY_GLOB, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.cFileName[0] == '.')
			{
				// just ignore . & ..
				continue;
			}

			ULARGE_INTEGER sz;
			sz.LowPart = wfd.nFileSizeLow;
			sz.HighPart = wfd.nFileSizeHigh;
			ulUsedSpace += sz.QuadPart;

		} while (FindNextFileA(hFind, &wfd));
		FindClose(hFind);
	}

	return ulUsedSpace;
}

void CUpload::CheckDiskUsage() {
	ULARGE_INTEGER totalSize;
	GetDiskFreeSpaceEx(NULL, NULL, &totalSize, NULL);

	bool isOptimal = false;
	while (!isOptimal)
	{
		// hostview used space
		ULONGLONG ullUsedSpace = getUsage();
		// free space if hostview was not there
		ULONGLONG ullFreeSpace = totalSize.QuadPart + ullUsedSpace;

		// we can only fill up to 50% of the 'theoretical' free space
		if (ullUsedSpace > ullFreeSpace / 2.0)
		{
			removeoldest();
			isOptimal = false;
		}
		else
		{
			isOptimal = true;
		}
	}
}