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

#include <stdio.h>
#include <stdlib.h>
#include <wininet.h>
#include <curl/curl.h>

#include "http_server.h"
#include "http_parser.h"
#include "trace.h"
#include "product.h"

HANDLE hSrvThread = NULL;
bool isServerRunning = false;

const char * szHttpOK = "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nAccess-Control-Allow-Origin: *\n\nOK.\n\n";
const char * szHttpErr = "HTTP/1.1 500 Interal Server Error\nContent-Type: text/html; charset=utf-8\nAccess-Control-Allow-Origin: *\n\nFailed.\n\n";

// very basic ok resp helper
void send_http_ok(SOCKET s) {
	send(s, szHttpOK, (int)strlen(szHttpOK), 0);
}

// very basic error resp helper
void send_http_err(SOCKET s) {
	send(s, szHttpErr, (int)strlen(szHttpErr), 0);
}

void extract_body_params(char *szBuffer, ParamsT &params)
{
	char szKeyValue[HTTPSERVER_DEFAULT_BUFLEN] = { 0 };
	char szKey[MAX_PATH] = { 0 };
	char szValue[HTTPSERVER_DEFAULT_BUFLEN] = { 0 };
	char *szCtx = 0;
	char *szToken = strtok_s(szBuffer, "&", &szCtx);

	while (szToken)
	{
		szKey[0] = szValue[0] = szKeyValue[0] = 0;

		strcpy_s(szKeyValue, szToken);

		char *szCtx2 = 0;
		char *szToken2 = strtok_s(szKeyValue, "=", &szCtx2);

		strcpy_s(szKey, szToken2);
		szToken2 = strtok_s(NULL, "=", &szCtx2);

		if (szToken2)
		{
			strcpy_s(szValue, szToken2);
		}

		if (strlen(szKey) && strlen(szValue))
		{
			char szCanonical[1024] = { 0 };
			DWORD dwSize = _countof(szCanonical);
			InternetCanonicalizeUrlA(szValue, szCanonical, &dwSize, ICU_DECODE | ICU_NO_ENCODE);

			TCHAR szUtf8[1024] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, szCanonical, (int)strlen(szCanonical) + 1, szUtf8, _countof(szUtf8));
			params.insert(std::make_pair(szKey, szUtf8));
		}

		szToken = strtok_s(NULL, "&", &szCtx);
	}
}

typedef struct {
	SOCKET s;
	char *body;
	size_t body_len;
	char *url;
	size_t url_len;
	bool complete;
	CHttpCallback *pCallback;
} http_data_t;

size_t strlncat(char **dst, size_t dlen, const char *src, size_t slen) {
	if (*dst == NULL) {
		*dst = (char *)malloc(slen + 1);
		memset(*dst, 0, slen + 1);
	}
	else {
		*dst = (char *)realloc(*dst, dlen + slen + 1);
		memset(*dst + dlen, 0, slen + 1);
	}
	memcpy(*dst + dlen, src, slen);
	return dlen + slen;
}

int url_data_cb(http_parser *parser, const char *at, size_t len) {
	http_data_t *d = (http_data_t*)parser->data;
	d->url_len = strlncat(&d->url, d->url_len, at, len);
	return 0;
}

int body_data_cb(http_parser *parser, const char *at, size_t len) {
	http_data_t *d = (http_data_t*)parser->data;
	d->body_len = strlncat(&d->body, d->body_len, at, len);
	return 0;
}

int message_complete_cb(http_parser *parser) {
	http_data_t *d = (http_data_t*)parser->data;

	Debug("[SRV] complete msg %s with %d bytes of data", d->url, d->body_len);

	if (strcmp("/upload", d->url) == 0) {
		// browser posting json data to be submitted
		d->pCallback->OnJsonUpload(&d->body, d->body_len);
		send_http_ok(d->s);
	}
	else if (strcmp("/locupdate", d->url) == 0) {
		// browser posting a new user location
		ParamsT params;
		TCHAR tszLoc[2014] = { 0 }, tszBrow[2014] = { 0 };
		extract_body_params(d->body, params);

		if (params.find("location") != params.end() && params.find("browser") != params.end()) {
			_tcscpy_s(tszLoc, params["location"].c_str());
			_tcscpy_s(tszBrow, params["browser"].c_str());
			d->pCallback->OnBrowserLocationUpdate(tszLoc, tszBrow);
			send_http_ok(d->s);
		}
		else {
			Debug("[SRV] invalid location update %s\n", d->body);
			send_http_err(d->s);
		}
	} 
	else if (strcmp("/ping", d->url) == 0) {
		// simple test alive handler
		send_http_ok(d->s);
	}
	else {
		// unknown
		fprintf(stderr, "[SRV] unknow url %s\n", d->url);
		send_http_err(d->s);
	}

	d->complete = true;
	return 0;
}


DWORD WINAPI ServerProc(LPVOID lpParameter)
{
	CHttpCallback *pCallback = (CHttpCallback *)lpParameter;

	SOCKET srvSocket = INVALID_SOCKET;
	SOCKET cliSocket = INVALID_SOCKET;

	char szBuffer[HTTPSERVER_DEFAULT_BUFLEN];
	int nBuffLen = HTTPSERVER_DEFAULT_BUFLEN;

	http_parser_settings psettings = { 0 };
	http_parser *parser = NULL;
	http_data_t data;

	psettings.on_body = body_data_cb;
	psettings.on_url = url_data_cb;
	psettings.on_message_complete = message_complete_cb;

	// Create a SOCKET for connecting to server
	srvSocket = (unsigned int)socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket == INVALID_SOCKET)
	{
		Debug("[SRV] socket failed with error: %ld", WSAGetLastError());
		return 1;
	}

	BOOL optval = TRUE;
	if (setsockopt(srvSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
		Debug("[SRV] setsockopt failed with error: %d", WSAGetLastError());
		return 1;
	}

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(HTTPSERVER_DEFAULT_PORT);

	// Setup the TCP listening socket
	if (bind(srvSocket, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR)
	{
		Debug("[SRV] bind failed with error: %d", WSAGetLastError());
		closesocket(srvSocket);
		return 1;
	}

	if (listen(srvSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		Debug("[SRV] listen failed with error: %d", WSAGetLastError());
		closesocket(srvSocket);
		return 1;
	}

	u_long iMode = 1;
	ioctlsocket(srvSocket, FIONBIO, &iMode);

	while (isServerRunning)
	{
		// FIXME: should really start a thread per cli and process multiple http requests per connection
		// now we are just handling a request per connection and we will close the socket after
		cliSocket = INVALID_SOCKET;
		do
		{
			cliSocket = accept(srvSocket, NULL, NULL);
			Sleep(50); // FIXME: why this sleep, above will block until ?!?
			//FIXME we should avoid polling. make accept blocking and handle closing the thread and all the sockets
		} while (cliSocket == INVALID_SOCKET && isServerRunning);

		if (isServerRunning && cliSocket != INVALID_SOCKET) {
			parser = (http_parser*)malloc(sizeof(http_parser));
			if (!parser) {
				// error allocating parser
				Debug("[SRV] failed to allocate new http parser");
				send_http_err(cliSocket);
			}
			else {
				int nRecv = 0;
				int nParsed = 0;

				http_parser_init(parser, HTTP_REQUEST);

				// reset custom data
				data.pCallback = pCallback;
				data.s = cliSocket;
				data.url = NULL;
				data.body = NULL;
				data.url_len = 0;
				data.body_len = 0;
				data.complete = false;

				parser->data = &data;

				do
				{
					nRecv = recv(cliSocket, szBuffer, nBuffLen, 0);
					if (nRecv >= 0)
					{
						nParsed = http_parser_execute(parser, &psettings, szBuffer, nRecv);
						if (nParsed != nRecv) {
							// parsing error
							Debug("[SRV] parser error");
							send_http_err(cliSocket);
							break;
						}
					}
					else {
						// recv error
						if (!data.complete) {
							//TODO: fix as this triggers all the time
							Debug("[SRV] recv failed with error: %d[%d] socket id", nRecv, WSAGetLastError());
							send_http_err(cliSocket);
						}
					}
				} while (nRecv > 0 && !data.complete);


				if (data.body)
					free(data.body);
				if (data.url)
					free(data.url);
				free(parser);
				parser = NULL;
			}

			closesocket(cliSocket);
		}
		else if (cliSocket != INVALID_SOCKET) {
			// server is shutting down - ignore this client
			closesocket(cliSocket);
		}
	}

	closesocket(srvSocket);
	return 0;
}

bool StartHttpDispatcher(CHttpCallback &callback)
{
	if (!hSrvThread && !isServerRunning)
	{
		isServerRunning = true;
		hSrvThread = CreateThread(NULL, NULL, ServerProc, &callback, NULL, NULL);
		return true;
	}
	return false;
}

bool StopHttpDispatcher()
{
	if (hSrvThread && isServerRunning)
	{
		isServerRunning = false;
		WaitForSingleObject(hSrvThread, INFINITE);
		CloseHandle(hSrvThread);
		hSrvThread = NULL;
		return true;
	}
	return false;
}

bool SendServerMessage(char *szMessage)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;
	bool fres = true;

	char ua[512] = { 0 };
	sprintf_s(ua, 512, "Hostview Windows %s", ProductVersionStr);

	char url[MAX_PATH] = { 0 };
	sprintf_s(url, "http://127.0.0.1:%d/locupdate", HTTPSERVER_DEFAULT_PORT);

	curl = curl_easy_init();
	if (!curl) {
		Debug("[SRV] curl_easy_init failed");
		return false;
	}

	headers = curl_slist_append(headers, "Expect:"); // remove default Expect header

	// FIXME: should really reuse the connection but the server code does not support that either ...
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L); // connection / request
	curl_easy_setopt(curl, CURLOPT_USERAGENT, ua);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szMessage);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		Debug("[SRV] curl_easy_perform failed: %s", curl_easy_strerror(res));
		fres = false;
	}

	if (curl)
		curl_easy_cleanup(curl);

	return fres;
}

DWORD WINAPI SendMessageProc(LPVOID lpParam)
{
	char *szMessage = (char *)lpParam;
	SendServerMessage(szMessage);
	delete[] szMessage;
	return 0;
}

bool AsyncSendMessage(char *szMsg)
{
	char *szMessage = new char[strlen(szMsg) + 1];
	strcpy_s(szMessage, strlen(szMsg) + 1, szMsg);
	CloseHandle(CreateThread(NULL, NULL, SendMessageProc, szMessage, NULL, NULL));
	return true;
}

// TODO: this is only used by the explorer BHO to send loc updates ...
// refactor somewhere else ??
void SendHttpMessage(ParamsT &params)
{
	char szResult[4096] = { 0 };
	char szBuffer[1024] = { 0 };

	char szUtf8[1024] = { 0 };
	char szCanonical[1024] = { 0 };

	for (ParamsT::iterator it = params.begin(); it != params.end(); it++)
	{
		DWORD dwLen = sizeof(szCanonical);

		WideCharToMultiByte(CP_UTF8, 0, it->second.c_str(), (int)_tcslen(it->second.c_str()) + 1,
			szUtf8, _countof(szUtf8), 0, 0);

		InternetCanonicalizeUrlA(szUtf8, szCanonical, &dwLen, 0);

		sprintf_s(szBuffer, "%s=%s", it->first.c_str(), szCanonical);

		if (strlen(szResult) > 0)
		{
			strcat_s(szResult, "&");
		}
		strcat_s(szResult, szBuffer);
	}
	Debug("[FRA] sending a message to server with content: %s", szResult);
	AsyncSendMessage(szResult);
}
