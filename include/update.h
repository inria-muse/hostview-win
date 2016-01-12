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

#include <vector>

#define ConnectionTimeout 500

#if defined(UPDATELIBRARY_EXPORT) // inside DLL
#   define UPDATEAPI   __declspec(dllexport)
#else // outside DLL
#   define UPDATEAPI   __declspec(dllimport)
#endif  // UPDATELIBRARY_EXPORT

UPDATEAPI BOOL CheckForUpdate();
UPDATEAPI BOOL DownloadUpdate(TCHAR *szUpdatePath, DWORD dwSize);
UPDATEAPI BOOL GetProductVersion(char *szProductVersion, DWORD dwSize);

UPDATEAPI BOOL DownloadFile(TCHAR *szUrl, TCHAR *szFile);

/**
 * Given a relative file path it pulls it from UpdateLocation
 **/
UPDATEAPI VOID PullFile(TCHAR *szFilePath);

extern "C" UPDATEAPI bool QueryPublicInfo(std::vector<std::string> * &info);
extern "C" UPDATEAPI void FreePublicInfo(std::vector<std::string> * &info);
