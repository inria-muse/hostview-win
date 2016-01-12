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

#include "stdafx.h";
#include "trace.h";

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
	fprintf(stderr, "[debug] %d, %s\n", time(NULL), szMessage);
	va_end(vArgs);
#endif
}

void Trace(char *szFormat, ...) {
	if (!szFormat || GetFileSize(LOGFILE) >= 2 * 1024 * 1024)
	{
		char szFile[MAX_PATH] = { 0 };
		sprintf_s(szFile, "submit\\%d.log", GetTickCount());
		MoveFileA(LOGFILE, szFile);
	}

	if (szFormat)
	{
		va_list vArgs;
		va_start(vArgs, szFormat);

		FILE * f = NULL;

		fopen_s(&f, LOGFILE, "a+");

		if (f)
		{
			char szMessage[1024] = { 0 };
			vsprintf_s(szMessage, szFormat, vArgs);

#ifdef _DEBUG
			fprintf(stderr, "[trace] %d, %s\n", time(NULL), szMessage);
#endif
			fprintf(f, "%d, %s\n", time(NULL), szMessage);
			fclose(f);
		}

		va_end(vArgs);
	}
}