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
#include "trace.h"
#include "Upload.h"
#include "Settings.h"

//FIXME: initially the g_settings variable is still used before being initialized 

// 10 megabytes
#define TRACE_MAX_LOGSIZE 10000000

static CSettings *g_settings;

ULONGLONG GetSizeInBytes(const char* fileName) {
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA(fileName, GetFileExInfoStandard, &fad))
		return -1;

	LARGE_INTEGER size;
	size.HighPart = fad.nFileSizeHigh;
	size.LowPart = fad.nFileSizeLow;
	return size.QuadPart;
}

ULONGLONG GetHiResTimestamp()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	ULARGE_INTEGER epoch;
	epoch.LowPart = 0xD53E8000;
	epoch.HighPart = 0x019DB1DE;

	ULARGE_INTEGER ts;
	ts.LowPart = ft.dwLowDateTime;
	ts.HighPart = ft.dwHighDateTime;
	ts.QuadPart -= epoch.QuadPart;

	return ts.QuadPart / 10000;
}

void InitTrace() {
	g_settings = new CSettings();
}

void Debug(char *szFormat, ...) {
	bool dodebug = g_settings->GetBoolean(DebugMode);
#ifndef _DEBUG
	// early out in non-debug versions
	if (!dodebug) 
		return;
#endif

	va_list vArgs;
	va_start(vArgs, szFormat);
	char szMessage[4096] = { 0 };
	vsprintf_s(szMessage, 4096, szFormat, vArgs);
	__int64 ts = GetHiResTimestamp();

#ifdef _DEBUG
	fprintf(stderr, "[%llu] [debug] %s\n", ts, szMessage);
#endif

	if (dodebug) {
		FILE * f = NULL;
		fopen_s(&f, ".\\temp\\hostview.log", "a+");
		if (f)
		{
			fprintf(f, "[%llu] [debug] %s\n", ts, szMessage);
			fclose(f);
		}
	}
	va_end(vArgs);
}

void RawWrite(char *szFormat, ...) {
	if (szFormat)
	{
		char szMessage[2048] = { 0 };
		__int64 ts = GetHiResTimestamp();

		va_list vArgs;
		va_start(vArgs, szFormat);

		vsprintf_s(szMessage, 2048, szFormat, vArgs);

#ifdef _DEBUG
		fprintf(stderr, "[%llu] [trace] %s\n", ts, szMessage);
#endif
		// FIXME: keep the handle open ? Highly costly but I guess that we can afford it
		FILE * f = NULL;
		fopen_s(&f, ".\\temp\\hostview.log", "a+");
		if (f)
		{
			fprintf(f, "[%llu] %s\n", ts, szMessage);
			fclose(f);
		}

		va_end(vArgs);
	}
}

void Trace(char *szFormat, ...) {
	
	bool dodebug = g_settings->GetBoolean(DebugMode);

	// max log size
	if (!szFormat || GetSizeInBytes(".\\temp\\hostview.log") >= TRACE_MAX_LOGSIZE)
	{
		// rename with timestamp
		char szFile[MAX_PATH] = { 0 };
		sprintf_s(szFile, MAX_PATH, ".\\temp\\%llu_hostview.log", GetHiResTimestamp());
		MoveFileA(".\\temp\\hostview.log", szFile);

		MoveFileToSubmit(szFile, dodebug);

		// log files is usually rotated upon session end, so good time to reload
		g_settings->ReloadSettings();
	}

	if (szFormat)
	{
		char szMessage[2048] = { 0 };
		__int64 ts = GetHiResTimestamp();

		va_list vArgs;
		va_start(vArgs, szFormat);

		vsprintf_s(szMessage, 2048, szFormat, vArgs);

#ifdef _DEBUG
		fprintf(stderr, "[%llu] [trace] %s\n", ts, szMessage);
#endif
		// FIXME: keep the handle open ? Highly costly but I guess that we can afford it
		FILE * f = NULL;
		fopen_s(&f, ".\\temp\\hostview.log", "a+");
		if (f)
		{
			fprintf(f, "[%llu] %s\n", ts, szMessage);
			fclose(f);
		}

		va_end(vArgs);
	}
}