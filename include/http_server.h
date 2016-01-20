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

#include <map>
#include <string>

#if defined(HTTPSERVERLIBRARY_EXPORT) // inside DLL
#   define HTTPSERVERAPI   __declspec(dllexport)
#else // outside DLL
#   define HTTPSERVERAPI   __declspec(dllimport)
#endif  // HTTPSERVERLIBRARY_EXPORT

#define HTTPSERVER_DEFAULT_BUFLEN 80*1024
#define HTTPSERVER_DEFAULT_PORT 40123

#ifndef tstring
#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif
#endif

typedef std::map<std::string, std::tstring> ParamsT;

/** Data callbacks interface. */
class CHttpCallback
{
public:
	virtual bool OnBrowserLocationUpdate(TCHAR *location, TCHAR *browser) = 0;
	virtual bool OnJsonUpload(char **jsonbuf, size_t len) = 0;
};

extern "C" bool HTTPSERVERAPI StartHttpDispatcher(CHttpCallback &callback);

extern "C" bool HTTPSERVERAPI StopHttpDispatcher();

extern "C" void HTTPSERVERAPI SendHttpMessage(ParamsT &params);
