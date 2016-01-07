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

#include <curl/curl.h>

#if defined(UPLOADLIBRARY_EXPORT) // inside DLL
#   define UPLOADAPI   __declspec(dllexport)
#else // outside DLL
#   define UPLOADAPI   __declspec(dllimport)
#endif  // UPLOADLIBRARY_EXPORT

/** Class for handling an upload session using cURL. */
class UPLOADAPI CUpload
{
public:
	CUpload(void);
	~CUpload(void);

	/** Returns true on success, false if should stop uploading other files (upload failed or was too slow). */
	bool SubmitFile(char *server, char *userId, char *deviceId, char *fileName);

private:
	bool ZipFile(char *src, char *dest);
	CURL *curl;
};