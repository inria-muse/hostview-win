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

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <curl/curl.h>

#include "ServiceInstaller.h"
#include "HostViewService.h"
#include "ServiceBase.h"
#include "tests.h"

#define SERVICE_NAME			L"HostViewService"
#define SERVICE_DISPLAY_NAME	L"HostView Service"
#define SERVICE_START_TYPE		SERVICE_AUTO_START
#define SERVICE_DEPENDENCIES	L""
#define SERVICE_ACCOUNT			NULL
#define SERVICE_PASSWORD		NULL

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA ws;
	WSAStartup(MAKEWORD(2, 2), &ws);

	curl_global_init(CURL_GLOBAL_SSL);

	EnableDebugPrivileges();

	InitCurrentDirectory();

	CreatePublicFolder(_T("tags"));
	CreatePublicFolder(TEMP_PATH);

	Trace("Hostviewcli started.");

	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_tcsicmp(L"install", argv[1] + 1) == 0)
		{
			InstallService(SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_START_TYPE, SERVICE_DEPENDENCIES, SERVICE_ACCOUNT, SERVICE_PASSWORD);

			Trace("Service installed.");
		}
		else if (!_tcsicmp(L"remove", argv[1] + 1))
		{
			Trace("Service uninstall.");

			// stops captures if running
			SendServiceMessage(Message(MessageSuspend));

			SysInfo info;
			QuerySystemInfo(info);
			char szHdd[MAX_PATH] = { 0 };
			sprintf_s(szHdd, "%S", info.hddSerial);

			// double make sure all data is in the submit folder
			CleanAllCaptureFiles();
			Trace(0); // moves the logfile to submit

			// upload all files
			CUpload upload;
			upload.TrySubmit(szHdd);

			// shutdown and uninstall service
			UninstallService(SERVICE_NAME);
		}
		else if (!_tcsicmp(L"restart", argv[1] + 1)) 
		{
			SendServiceMessage(Message(MessageRestartSession));
		}
		else if (!_tcsicmp(L"debug", argv[1] + 1))
		{
			// run service on the foreground and stop on key stroke
			CHostViewService service(SERVICE_NAME);
			service.OnStart(NULL, NULL);
			fprintf(stderr, "hostview service started\n");

			system("pause");

			fprintf(stderr, "hostview service stopping ...\n");
			service.OnStop();
		}
		else if (!_tcsicmp(L"test", argv[1] + 1))
		{
			if (!_tcsicmp(L"download", argv[2]))
				testDownload();
			if (!_tcsicmp(L"comm", argv[2]))
				testComm();
			if (!_tcsicmp(L"submit", argv[2]))
				testSubmit();
			if (!_tcsicmp(L"tcpcomm", argv[2]))
				testTcpComm();
			if (!_tcsicmp(L"logs", argv[2]))
				testLogs();
			if (!_tcsicmp(L"counter", argv[2]))
				testCounter();
			if (!_tcsicmp(L"procs", argv[2]))
				printProcs();
			if (!_tcsicmp(L"socks", argv[2]))
				printSockets();
			if (!_tcsicmp(L"sysinfo", argv[2]))
				printSysinfo();
		}
		else {
			printf("./hostviewcli.exe \t- run the hostview service.\n\n");
			printf("Parameters (optional):\n");
			printf("\t/install \t- to install the service.\n");
			printf("\t/remove \t- to remove the service.\n");
			printf("\t/restart \t- to restart a session.\n");
			printf("\t/debug \t- to start the service for debugging.\n");
			printf("\t/test <test> \t- to run a selected test.\n");
		}
	}
	else
	{
		// Default is to run the installed service
		Trace("Hostviewcli run service.");
		CHostViewService service(SERVICE_NAME);
		if (!CServiceBase::Run(service))
		{
			fprintf(stderr, "Service failed to run w/err %d\n", GetLastError());
		}
	}

	curl_global_cleanup();
	WSACleanup();
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}

