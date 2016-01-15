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
	fprintf(stderr, "[upload] read %d bytes\n", nRead);
	return nRead;
}

// zip helper
bool zipfile(char *src, char *dest)
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

		Debug("7za.exe exit %d\n", dwExit);
		return (dwExit == 0);
	}

	return false;
}

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

	fprintf(stderr, "[upload] submit %s to %s %d bytes\n", fileName, url, nTotalFileSize);

	headers = curl_slist_append(headers, "Expect:"); // remove default Expect header

	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, settings.GetULong(UploadLowSpeedLimit));
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, settings.GetULong(UploadLowSpeedTime));
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

bool MoveFileToSubmit(const char *file, bool renameWithTs) {
	if (!PathFileExistsA(file)) {
		fprintf(stderr, "[upload] no such file %s\n", file);
		return false;
	}

	// ensure directory
	if (!PathFileExistsA(SUBMIT_DIRECTORY)) {
		CreateDirectoryA(SUBMIT_DIRECTORY, NULL);
	}

	// new name: ./submit/timestamp_filename.ext
	char sFile[MAX_PATH] = { 0 };
	if (!renameWithTs) {
		// pcap files have a timestamp already
		sprintf_s(sFile, "%s\\%s", SUBMIT_DIRECTORY, PathFindFileNameA(file));
	}
	else {
		// others need to add timestamp to get unique names
		sprintf_s(sFile, "%s\\%llu_%s", SUBMIT_DIRECTORY, GetHiResTimestamp(), PathFindFileNameA(file));
	}

	if (!MoveFileA(file, sFile)) {
		fprintf(stderr, "[upload] failed to move file %s to %s [%d]\n", file, sFile, GetLastError());
		return false;
	}

	Trace("Add submit file %s.", sFile);
	return true;
}

bool CopyFileToSubmit(const char *file, bool renameWithTs) {
	if (!PathFileExistsA(file)) {
		fprintf(stderr, "[upload] no such file %s\n", file);
		return false;
	}

	// ensure directory
	if (!PathFileExistsA(SUBMIT_DIRECTORY)) {
		CreateDirectoryA(SUBMIT_DIRECTORY, NULL);
	}

	// new name: ./submit/timestamp_filename.ext
	char sFile[MAX_PATH] = { 0 };
	if (!renameWithTs) {
		// pcap files have a timestamp already
		sprintf_s(sFile, "%s\\%s", SUBMIT_DIRECTORY, PathFindFileNameA(file));
	}
	else {
		// others need to add timestamp to get unique names
		sprintf_s(sFile, "%s\\%llu_%s", SUBMIT_DIRECTORY, GetHiResTimestamp(), PathFindFileNameA(file));
	}

	if (!CopyFileA(file, sFile, false)) {
		fprintf(stderr, "[upload] failed to copy file %s to %s [%d]\n", file, sFile, GetLastError());
		return false;
	}

	Trace("Add submit file %s.", sFile);
	return true;
}

bool DoSubmit(const char *deviceId) {
	bool result = true;
	CUpload upload;

	char szFilename[MAX_PATH] = { 0 };
	char szZipFilename[MAX_PATH] = { 0 };

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

			sprintf_s(szFilename, "%s\\%s", SUBMIT_DIRECTORY, wfd.cFileName);

			if (strcmp(PathFindExtensionA(szFilename), ".zip") != 0)
			{
				sprintf_s(szZipFilename, "%s\\%s.zip", SUBMIT_DIRECTORY, wfd.cFileName);
				Debug("[upload] create zip %s -> %s\n", szFilename, szZipFilename);

				if (zipfile(szFilename, szZipFilename))
				{
					// remove original file,
					// since we have the zip;
					DeleteFileA(szFilename);
					Trace("Create zip file %s.", szZipFilename);
				}
				else
				{
					// remove broken zip
					fprintf(stderr, "[upload] failed to create zip file %s\n", szZipFilename);
					DeleteFileA(szZipFilename);
					result = false;
				}
			}
			else {
				// already zipped
				sprintf_s(szZipFilename, "%s", szFilename);
			}

			if (PathFileExistsA(szZipFilename))
			{
				Debug("[upload] send zip %s\n", szZipFilename);
				if (upload.SubmitFile(szZipFilename, deviceId))
				{
					DeleteFileA(szZipFilename);
					Trace("Remove submitted file %s.", szZipFilename);
				}
				else
				{
					result = false;
				}
			}
		} while (result && FindNextFileA(hFind, &wfd));

		FindClose(hFind);
	}

	return result;
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

void CheckDiskUsage() {
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