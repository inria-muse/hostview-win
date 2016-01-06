/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Muse / INRIA
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

	EnableDebugPrivileges();

	InitCurrentDirectory();

	CreatePublicFolder(_T("tags"));
	CreatePublicFolder(TEMP_PATH);

	Trace("Program started.");

	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_tcsicmp(L"install", argv[1] + 1) == 0)
		{
			InstallService(SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_START_TYPE, SERVICE_DEPENDENCIES, SERVICE_ACCOUNT, SERVICE_PASSWORD);

			Trace("Service installed.");
		}
		else if (!_tcsicmp(L"remove", argv[1] + 1))
		{
			// one last submit before uninstall;
			CHostViewService service(SERVICE_NAME);

			Trace("Last submit. Uninstalling.");
			Trace(0);

			service.SubmitData();
			UninstallService(SERVICE_NAME);
		}
		else if (!_tcsicmp(L"debug", argv[1] + 1))
		{
			CHostViewService service(SERVICE_NAME);
			service.OnStart(NULL, NULL);

			system("pause");

			service.OnStop();
		}
		else if (!_tcsicmp(L"test", argv[1] + 1))
		{
			// TODO: some other tests
			// testDownload();
			// testComm();
			// testSubmit();
			testTcpComm();
			// testLogs();
			// testCounter();
			// printProcs();
			// printSockets();
			printSysinfo();
		}
	}
	else
	{
		printf("Parameters:\n");
		printf(" /install  to install the service.\n");
		printf(" /remove   to remove the service.\n");

		CHostViewService service(SERVICE_NAME);
		if (!CServiceBase::Run(service))
		{
			printf("Service failed to run w/err %d\n", GetLastError());
		}
	}

	WSACleanup();
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	Trace("Program stopped.");
	return 0;
}

