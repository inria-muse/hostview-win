#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <WinInet.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "wininet.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT 27015

#ifndef __SOCKS__H_
#define __SOCKS__H_


#ifndef tstring
#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif
#endif

typedef std::map<std::string, std::tstring> ParamsT;

class CHttpCallback
{
public:
	virtual bool OnMessage(ParamsT &params) = 0;
};
#endif

bool StartHttpDispatcher(CHttpCallback &callback);
bool StopHttpDispatcher();
void SendHttpMessage(ParamsT &params);
void ExtractParams(char *szBuffer, ParamsT &params);

#ifndef __LIGHT_SOCKS__
HANDLE hSrvThread = NULL;
bool isServerRunning = false;

// removes any http header before payload
void ExtractPostData(char *szBuffer)
{
	size_t nLen = strlen(szBuffer);

	for (size_t i = 0; i < nLen - 2; i ++)
	{
		if (szBuffer[i] != '\n')
			continue;
		if (szBuffer[i + 1] == '\r' && szBuffer[i + 2] == '\n')
			strcpy_s(szBuffer, nLen, szBuffer + i + 3);
		else if (szBuffer[i + 1] == '\n')
			strcpy_s(szBuffer, nLen, szBuffer + i + 2);
	}
}

char szKeyValue[DEFAULT_BUFLEN] = {0};
char szKey[DEFAULT_BUFLEN] = {0}, szValue[DEFAULT_BUFLEN] = {0};

void ExtractParams(char *szBuffer, ParamsT &params)
{
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
			char szCanonical[1024] = {0};
			DWORD dwSize = _countof(szCanonical);
			InternetCanonicalizeUrlA(szValue, szCanonical, &dwSize, ICU_DECODE | ICU_NO_ENCODE);

			TCHAR szUtf8[1024] = {0};
			MultiByteToWideChar(CP_UTF8, 0, szCanonical, (int) strlen(szCanonical) + 1, szUtf8, _countof(szUtf8));
			params.insert(std::make_pair(szKey, szUtf8));
		}

		szToken = strtok_s(NULL, "&", &szCtx);
	}
}

DWORD WINAPI ServerProc(LPVOID lpParameter)
{
	CHttpCallback *pCallback = (CHttpCallback *) lpParameter;

	SOCKET srvSocket = INVALID_SOCKET;
	SOCKET cliSocket = INVALID_SOCKET;

	char szBuffer[DEFAULT_BUFLEN];
	int nBuffLen = DEFAULT_BUFLEN;

	// Create a SOCKET for connecting to server
	srvSocket = (unsigned int) socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket == INVALID_SOCKET)
	{
		printf("[SRV] socket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(DEFAULT_PORT);

	// Setup the TCP listening socket
	if (bind(srvSocket, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("[SRV] bind failed with error: %d\n", WSAGetLastError());
		closesocket(srvSocket);
		return 1;
	}

	if (listen(srvSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("[SRV] listen failed with error: %d\n", WSAGetLastError());
		closesocket(srvSocket);
		return 1;
	}

	u_long iMode = 1;
	ioctlsocket(srvSocket, FIONBIO, &iMode);

	while (isServerRunning)
	{
		do 
		{
			cliSocket = (unsigned int) accept(srvSocket, NULL, NULL);
			Sleep(50);
		}
		while (cliSocket == INVALID_SOCKET && isServerRunning);

		if (!isServerRunning)
			continue;

		// Receive until the peer shuts down the connection
		int nRecieved = 0;
		do
		{
			nRecieved = recv(cliSocket, szBuffer, nBuffLen, 0);
			if (nRecieved > 0 && nRecieved < nBuffLen)
			{
				szBuffer[nRecieved] = 0;
				ExtractPostData(szBuffer);

				ParamsT params;
				ExtractParams(szBuffer, params);

				// TODO: send another status ok?!
				if (pCallback->OnMessage(params))
				{
					char * szHttpOK = "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nAccess-Control-Allow-Origin: *\n\nOK.\n\n";
					send(cliSocket, szHttpOK, (int) strlen(szHttpOK), 0);
				}
			}
			else if (nRecieved <= 0)
			{
				break;
			}
		}
		while (nRecieved > 0);

		// shutdown the connection since we're done
		shutdown(cliSocket, SD_SEND);

		// cleanup
		closesocket(cliSocket);
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
	SOCKET sockClient = INVALID_SOCKET;
	
	struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(DEFAULT_PORT);

	sockClient = (unsigned int) socket(AF_INET, SOCK_STREAM, 0);

	// Connect to server.
	if (connect(sockClient, (struct sockaddr *) &server, (int) sizeof(server)) == SOCKET_ERROR)
	{
		closesocket(sockClient);
		sockClient = INVALID_SOCKET;

		printf("Unable to connect to server!\n");
		return false;
	}

	// Send an initial buffer
	if (send(sockClient, szMessage, (int) strlen(szMessage), 0) == SOCKET_ERROR)
	{
		printf("[CLI] send failed with error: %d\n", WSAGetLastError());
		closesocket(sockClient);
		return false;
	}

	// shutdown the connection since no more data will be sent
	if (shutdown(sockClient, SD_SEND) == SOCKET_ERROR)
	{
		closesocket(sockClient);
		return false;
	}

	// cleanup
	closesocket(sockClient);

	return true;
}

DWORD WINAPI SendMessageProc(LPVOID lpParam)
{
	char *szMessage = (char *) lpParam;
	SendServerMessage(szMessage);
	delete [] szMessage;
	return 0;
}

bool AsyncSendMessage(char *szMsg)
{
	char *szMessage = new char[strlen(szMsg) + 1];
	strcpy_s(szMessage, strlen(szMsg) + 1, szMsg);
	CloseHandle(CreateThread(NULL, NULL, SendMessageProc, szMessage, NULL, NULL));
	return true;
}

void SendHttpMessage(ParamsT &params)
{
	char szResult[4096] = {0};
	char szBuffer[1024] = {0};

	char szUtf8[1024] = {0};
	char szCanonical[1024] = {0};

	for (ParamsT::iterator it = params.begin(); it != params.end(); it ++)
	{
		DWORD dwLen = sizeof(szCanonical);
		
		WideCharToMultiByte(CP_UTF8, 0, it->second.c_str(), (int) _tcslen(it->second.c_str()) + 1,
			szUtf8, _countof(szUtf8), 0, 0);

		InternetCanonicalizeUrlA(szUtf8, szCanonical, &dwLen, 0);

		sprintf_s(szBuffer, "%s=%s", it->first.c_str(), szCanonical);

		if (strlen(szResult) > 0)
		{
			strcat_s(szResult, "&");
		}
		strcat_s(szResult, szBuffer);
	}

	AsyncSendMessage(szResult);
}

#endif
