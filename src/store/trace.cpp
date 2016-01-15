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

__int64 GetHiResTimestamp()
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

long GetFileSize(char *szFilename)
{
	long nSize = 0;

	FILE * f = NULL;
	fopen_s(&f, szFilename, "r");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		nSize = ftell(f);
		fclose(f);
	}

	return nSize;
}

void Debug(char *szFormat, ...) {
#ifdef _DEBUG
	va_list vArgs;
	va_start(vArgs, szFormat);
	char szMessage[1024] = { 0 };
	vsprintf_s(szMessage, szFormat, vArgs);
	fprintf(stderr, "[%llu] [trace] %s\n", GetHiResTimestamp(), szMessage);
	va_end(vArgs);
#endif
}

void Trace(char *szFormat, ...) {
	if (!szFormat || GetFileSize(LOGFILE) >= 2 * 1024 * 1024)
	{
		MoveFileToSubmit(LOGFILE, true);
	}

	if (szFormat)
	{
		char szMessage[1024] = { 0 };
		__int64 ts = GetHiResTimestamp();

		va_list vArgs;
		va_start(vArgs, szFormat);

		vsprintf_s(szMessage, szFormat, vArgs);

#ifdef _DEBUG
		fprintf(stderr, "[%llu] [trace] %s\n", ts, szMessage);
#endif

		FILE * f = NULL;
		fopen_s(&f, LOGFILE, "a+");
		if (f)
		{
			fprintf(f, "[%llu] %s\n", ts, szMessage);
			fclose(f);
		}

		va_end(vArgs);
	}
}