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
#include "Capture.h"

#include <hash_map>
#include <string>

#ifndef SAFE_DELETE
#define SAFE_DELETE(X) if (X) {delete X; X = 0;}
#endif

// also in proc.cpp
void trim(char * s)
{
	char * p = s;
	size_t l = strlen(p);

	while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
	while(* p && isspace(* p)) ++p, --l;

	memmove(s, p, l + 1);
}


typedef std::hash_map<std::string, std::string> S2SMapT;
typedef std::hash_map<std::string, S2SMapT> S2MMapT;

#define HttpVerb			"http-verb"
#define HttpVerbParam		"http-verb-param"
#define HttpStatusCode		"http-status-code"

#define HttpGET				"GET"
#define HttpPOST			"POST"
#define HttpPUT				"PUT"
#define HttpDELETE			"DELETE"

#define HttpHost			"host"
#define HttpRefer			"referer"
#define HttpContentType		"content-type"
#define HttpContentLength	"content-length"

static char *statusVerb = "HTTP/";
static char *verbs[] = {"GET", "POST", "PUT", "DELETE"};
static char * headers[] = {"host", "referer", "content-type", "content-length"};

S2MMapT perConnectionMap;

void PutHeader(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *header, char *content)
{
	char szConn[MAX_PATH] = {0};
	sprintf_s(szConn, "%d_%s_%d_%s_%d", protocol, szSrc, srcport, szDest, destport);

	perConnectionMap[szConn][header] = content;
}

char* GetHeader(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *header)
{
	char szConn[MAX_PATH] = {0}, szConnRev[MAX_PATH] = {0};
	sprintf_s(szConn, "%d_%s_%d_%s_%d", protocol, szSrc, srcport, szDest, destport);
	sprintf_s(szConnRev, "%d_%s_%d_%s_%d", protocol, szDest, destport, szSrc, srcport);

	if (perConnectionMap[szConn].find(header) != perConnectionMap[szConn].end())
	{
		return (char *) perConnectionMap[szConn][header].c_str();
	}
	else
	{
		return (char *) perConnectionMap[szConnRev][header].c_str();
	}
}

bool OnHttpHeader(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szHttp);
void OnHttpHeadersEnd(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, CCaptureCallback &callback);

// HTTP parsing
bool parseHTTP(int nProtocol, char *szSrc, char *szDest, u_short sp, u_short dp, u_char *data, int packetSize, CCaptureCallback &callback)
{
	bool bHeaderFound = false;

	if (sp == 80 || sp == 8080 || dp == 80 || dp == 8080)
	{
		const char * delim = "\r\n";

		char buffer[4096] = {0};
		int i = 0;
		while (i < packetSize)
		{
			int start = i, end = i;
			while (strncmp((char *) data + end, delim, strlen(delim))
				&& (size_t) end < packetSize - strlen(delim) && end - start + 1 < sizeof(buffer))
			{
				end ++;
			}

			memcpy(buffer, data + start, end - start);
			buffer[end - start] = 0;

			i = end + (int) strlen(delim);

			if (strlen(buffer) == 0 && bHeaderFound)
			{
				OnHttpHeadersEnd(nProtocol, szSrc, sp, szDest, dp, callback);
			}
			else
			{
				bHeaderFound |= OnHttpHeader(nProtocol, szSrc, sp, szDest, dp, buffer);
			}
		}
	}

	return bHeaderFound;
}

void OnHttpHeadersEnd(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, CCaptureCallback &callback)
{
	char szConn[MAX_PATH] = {0}, szConnReq[MAX_PATH] = {0};
	sprintf_s(szConn, "%d_%s_%d_%s_%d", protocol, szSrc, srcport, szDest, destport);
	sprintf_s(szConnReq, "%d_%s_%d_%s_%d", protocol, szDest, destport, szSrc, srcport);

	// is there data on both request / response ?
	if (perConnectionMap.find(szConn) != perConnectionMap.end() && perConnectionMap.find(szConnReq) != perConnectionMap.end())
	{
		callback.OnHttpHeaders(protocol, szSrc, srcport, szDest, destport,
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpVerb),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpVerbParam),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpStatusCode),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpHost),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpRefer),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpContentType),
			GetHeader(protocol, szSrc, srcport, szDest, destport, HttpContentLength));

		perConnectionMap.erase(szConnReq);
		perConnectionMap.erase(szConn);
	}
}

// checks if there is any meaningful http header / verb inside this line
// szHttp can be for example content-type: something or HTTP/1.1 200 OK, etc.
bool OnHttpHeader(int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szHttp)
{
	static const int MAX_URL = 2084;

	char *szHeaderValue = 0;

	// check if it's a header
	for (int i = 0; i < _countof(headers); i ++)
	{
		if (_strnicmp(szHttp, headers[i], strlen(headers[i])))
		{
			continue;
		}
		else
		{
			szHeaderValue = new char[MAX_URL];

			// TODO: ':' is always immediately after the key?
			strcpy_s(szHeaderValue, MAX_URL, szHttp + strlen(headers[i]) + 1);
			trim(szHeaderValue);
			PutHeader(protocol, szSrc, srcport, szDest, destport, headers[i], szHeaderValue);

			SAFE_DELETE(szHeaderValue);

			return true;
		}
	}

	// check if it's a verb
	for (int i = 0; i < _countof(verbs); i ++)
	{
		if (strncmp(szHttp, verbs[i], strlen(verbs[i])))
			continue;

		szHeaderValue = new char[MAX_URL];
		strcpy_s(szHeaderValue, MAX_URL, szHttp + strlen(verbs[i]) + 1);

		// anonymize what's after '?'
		char *szQ = strstr(szHeaderValue, "?");
		if (szQ)
		{
			*szQ = 0;
		}
		else
		{
			szQ = strstr(szHeaderValue, "HTTP");

			if (szQ)
				*szQ = 0;
		}
		trim(szHeaderValue);

		PutHeader(protocol, szSrc, srcport, szDest, destport, HttpVerb, verbs[i]);
		PutHeader(protocol, szSrc, srcport, szDest, destport, HttpVerbParam, szHeaderValue);

		SAFE_DELETE(szHeaderValue);

		return true;
	}

	// check if it's a status verb
	if (!strncmp(szHttp, statusVerb, strlen(statusVerb)))
	{
		char *szQ = szHttp;
		while (szQ && *szQ)
		{
			if (isspace((unsigned char) *szQ))
			{
				szQ ++;
				break;
			}
			else
			{
				szQ ++;
			}
		}

		if (szQ && *szQ)
		{
			szHeaderValue = new char[MAX_URL];

			sprintf_s(szHeaderValue, MAX_URL, "%d", atoi(szQ));
			PutHeader(protocol, szSrc, srcport, szDest, destport, HttpStatusCode, szHeaderValue);

			SAFE_DELETE(szHeaderValue);

			return true;
		}
	}

	return false;
}
