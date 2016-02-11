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

#include "Settings.h"

#include <curl/curl.h>

#if defined(UPLOADLIBRARY_EXPORT) // inside DLL
#   define UPLOADAPI   __declspec(dllexport)
#else // outside DLL
#   define UPLOADAPI   __declspec(dllimport)
#endif  // UPLOADLIBRARY_EXPORT

#ifndef UPLOAD_HTTP_DEFAULT_BUFFER_SIZE
# define UPLOAD_HTTP_DEFAULT_BUFFER_SIZE 4096
#endif

#define SUBMIT_DIRECTORY ".\\submit"
#define SUBMIT_DIRECTORY_GLOB ".\\submit\\*.*"

/** Class for handling an upload session using cURL. */
class UPLOADAPI CUpload
{
public:
	CUpload();
	~CUpload(void);

	/** Loop over files in the submit folder and upload to the server.
	 *  Returns true if the caller should try again soon (retry), false if
	 *  this upload session has failed.
	 */
	bool TrySubmit(const char *deviceId);

private:
	/** Returns true on success, false if should stop uploading other files (upload failed or was too slow). */
	bool SubmitFile(const char *fileName, const char *deviceId);

	/** Make sure we are not using too much disk space, and make room if so. */
	void CheckDiskUsage();

	CSettings settings;
	CURL *curl;
	ULONG retryCount;
	DWORD lastRetry;
};

/** Move a file to the submit folder. */
extern "C" UPLOADAPI bool MoveFileToSubmit(const char *file, bool debug);

