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
#pragma once

#define TEMP_PATH ".\\temp"
#define TEMP_PATH_DB_GLOB ".\\temp\\*.db"

#if defined(TRACELIBRARY_EXPORT) // inside DLL
#   define HOSTVIEWTRACEAPI   __declspec(dllexport)
#else // outside DLL
#   define HOSTVIEWTRACEAPI   __declspec(dllimport)
#endif  // TRACELIBRARY_EXPORT

// TODO: these two could be in some utils lib really ..
extern "C" HOSTVIEWTRACEAPI ULONGLONG GetSizeInBytes(const char* fileName);
extern "C" HOSTVIEWTRACEAPI ULONGLONG GetHiResTimestamp();

// Trace interface
extern "C" HOSTVIEWTRACEAPI void InitTrace();
extern "C" HOSTVIEWTRACEAPI void Debug(char *szFormat, ...);
extern "C" HOSTVIEWTRACEAPI void Trace(char *szFormat, ...);
extern "C" HOSTVIEWTRACEAPI void RawWrite(char *szFormat, ...);
