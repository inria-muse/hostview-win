#ifndef __TESTS_H_
#define __TESTS_H_

#include "stdafx.h"
#include "comm.h"
#include "proc.h"
#include "capture.h"
#include "http_server.h"
#include "Store.h"
#include "Upload.h"
#include "Settings.h"
#include "trace.h"
#include "update.h"
#include "HostViewService.h"
#include "questionnaire.h"

CSettings settings;
CStore store;

typedef void (* ConsoleFunction)(void);
struct Function
{
	std::tstring name;
	std::tstring help;
	ConsoleFunction function;
	Function(std::tstring name, std::tstring help, ConsoleFunction function)
	{
		this->name = name;
		this->help = help;
		this->function = function;
	}
};

#define MAKE_COMMAND(C, H, F) commands.push_back(Function(_T(C), _T(H), F))

std::vector<Function> commands;

bool quit;
void quitConsole(void)
{
	quit = true;
}

void printHelp(void)
{
	printf("Available commands:\r\n");

	for (std::vector<Function>::iterator it = commands.begin();
		it != commands.end(); it ++)
	{
		printf("%8S	%20S\r\n", it->name.c_str(), it->help.c_str());
	}
}

void printSockets(void)
{
	printf("%10s%7s%20s%41s%7s%41s%7s%7s\r\n", "protocol", "pid", "name", "srcip", "port", "destip", "port", "state");

	IpTable *table = GetIpTable();
	__int64 timestamp = GetHiResTimestamp();

	const char * udp = "UDP";
	const char * tcp = "TCP";
	for (size_t i = 0; i < table->size; i ++)
	{
		IpRow it = table->get(i);
		printf("%10s%7d%20s%41s%7d%41s%7d%7d\r\n", it.IsUDP() ? udp : tcp,
			it.pid, it.name, it.srcIp, it.srcport, it.destIp, it.destport, it.state);

		store.Insert(it.pid, it.name, it.IsUDP() ? IPPROTO_UDP : IPPROTO_TCP, it.srcIp, it.destIp, it.srcport, it.destport, it.state, timestamp);
	}

	ReleaseIpTable(table);
}

void printProcs(void)
{
	PerfTable *table = GetPerfTable();
	__int64 timestamp = GetHiResTimestamp();

	printf("%7s%30s%10s%10s\r\n", "PID", "Image name", "Memory", "CPU");

	for (size_t i = 0; i < table->size; i ++)
	{
		PerfRow it = table->get(i);
		printf("%7d%30s%10d%10.2f\r\n", it.pid, it.name, it.memory, it.cpu);
		store.Insert(it.pid, it.name, it.memory, it.cpu, timestamp);
	}

	ReleasePerfTable(table);
}

void printSysinfo(void)
{
	SysInfo info;
	QuerySystemInfo(info);

	printf("%14s%60S\r\n", "Manufacturer", info.manufacturer);
	printf("%14s%60S\r\n", "Product", info.productName);
	printf("%14s%60S\r\n", "OS", info.windowsName);
	printf("%14s%60S\r\n", "CPU", info.cpuName);
	printf("%14s%57.2f GB\r\n", "RAM", (double)(info.totalRAM / 1024 / 1024) / 1024.0);
	printf("%14s%57.2f GB\r\n", "HDD", (double)(info.totalDisk / 1024 / 1024) / 1024.0);
	printf("%14s%60S\r\n", "Serial", info.hddSerial);
}

class CNetCallback : public CInterfacesCallback
{
	void LogNetwork(const NetworkInterface &ni, __int64 timestamp, bool connected)
	{
		TCHAR szGateways[MAX_PATH] = {0};
		for (size_t i = 0; i < ni.gateways.size(); i ++)
		{
			_tcscat_s(szGateways, ni.gateways[i].c_str());

			if (i < ni.gateways.size() - 1)
			{
				_tcscat_s(szGateways, _T(","));
			}
		}
		TCHAR szDnses[MAX_PATH] = {0};
		for (size_t i = 0; i < ni.dnses.size(); i ++)
		{
			_tcscat_s(szDnses, ni.dnses[i].c_str());

			if (i < ni.dnses.size() - 1)
			{
				_tcscat_s(szDnses, _T(","));
			}
		}
		TCHAR szIps[MAX_PATH] = {0};
		for (size_t i = 0; i < ni.ips.size(); i ++)
		{
			_tcscat_s(szIps, ni.ips[i].c_str());

			if (i < ni.ips.size() - 1)
			{
				_tcscat_s(szIps, _T(","));
			}
		}

		store.Insert(ni.strName.c_str(), ni.strFriendlyName.c_str(), ni.strDescription.c_str(), ni.strDnsSuffix.c_str(),
			ni.strMac.c_str(), szIps, szGateways, szDnses, ni.tSpeed, ni.rSpeed, ni.wireless, ni.strProfile.c_str(), ni.strSSID.c_str(),
			ni.strBSSID.c_str(), ni.strBSSIDType.c_str(), ni.strPHYType.c_str(), ni.phyIndex, ni.channel, ni.signal,
			ni.rssi, connected, timestamp);
	}

public:
	virtual void OnInterfaceConnected(const NetworkInterface &networkInterface)
	{
		__int64 timestamp = GetHiResTimestamp();
		LogNetwork(networkInterface, timestamp, true);
	}

	virtual void OnInterfaceDisconnected(const NetworkInterface &networkInterface)
	{
		__int64 timestamp = GetHiResTimestamp();
		LogNetwork(networkInterface, timestamp, false);
	}

} netCallback;

void startNetworkMonitor()
{
	if (StartInterfacesMonitor(netCallback, 5, 250))
	{
		printf("interfaces monitor started.\r\n");
	}
	else
	{
		printf("interfaces monitor start failed.\r\n");
	}
}

void stopNetworkMonitor()
{
	if (StopInterfacesMonitor())
	{
		printf("interfaces monitor stopped.\r\n");
	}
	else
	{
		printf("interfaces monitor stop failed.\r\n");
	}
}

void submitData()
{
	SysInfo info;
	QuerySystemInfo(info);

	char szHdd[MAX_PATH] = {0};
	sprintf_s(szHdd, "%S", info.hddSerial);

	DoSubmit(szHdd);
}

bool exec(TCHAR * szCommand)
{
	for (std::vector<Function>::iterator it = commands.begin();
		it != commands.end(); it ++)
	{
		if (_tcscmp(szCommand, it->name.c_str()) == 0)
		{
			it->function();
			return true;
		}
	}
	return false;
}

void InitConsole()
{
	const TCHAR *szTitle = _T("HostView Command-Line Interface");
	SetConsoleTitle(szTitle);
	
	HWND hWnd = FindWindow(NULL, szTitle);
	if (hWnd)
	{
		ShowWindow(hWnd, SW_MAXIMIZE);
	}
}

int consoleMain()
{
	InitConsole();
	store.Open();
	StartInterfacesMonitor(netCallback, 5, 250);

//	MAKE_COMMAND("nstart", "start connection monitor", &startNetworkMonitor);
//	MAKE_COMMAND("nstop", "stop connection monitor", &stopNetworkMonitor);
//	MAKE_COMMAND("submit", "submits data", &submitData);
//	MAKE_COMMAND("socks", "print sockets", &printSockets);
//	MAKE_COMMAND("procs", "print procs", &printProcs);
//	MAKE_COMMAND("info", "print sysinfo", &printSysinfo);
//	MAKE_COMMAND("help", "print help", &printHelp);
//	MAKE_COMMAND("quit", "exit program", &quitConsole);

	TCHAR szCommand[MAX_PATH] = {0};
	do
	{
		_tprintf(_T("> "));
		
		_fgetts(szCommand, _countof(szCommand), stdin);
		szCommand[_tcslen(szCommand) - 1] = 0;

		if (_tcslen(szCommand) > 0)
		{
			if (!exec(szCommand))
			{
				_tprintf(_T("Command %s not found. Type help for usage.\r\n"), szCommand);
			}
		}
	}
	while(!quit);

	// just in case
	StopInterfacesMonitor();

	// essential;
	store.Close();

	return 0;
}

class CCallback : public CMessagesCallback
{
public:
	virtual Message OnMessage(Message &message)
	{
		printf("Received message: %d\r\n", message.type);
		return message;
	}

} callback;

#include <assert.h>

DWORD WINAPI ClientThread(LPVOID lpParameter)
{
	int clientId = (int) lpParameter;
	for (int i = 0; i < 100; i ++)
	{
		Message message(i);
		Message result = SendServiceMessage(message);
		assert(result.type == message.type);
		Sleep(10);
	}

	printf("Client <%d> finished.\r\n", clientId);
	return 0;
}

// needs admin rights
void testCounter()
{
	// goes up to 32
	for (int i = 0; i < 32; i ++)
	{
		SetQuestionnaireCounter(i);
		assert(i == QueryQuestionnaireCounter());
	}
	
}

int testComm()
{
	int clients = 10;
	std::vector<HANDLE> hThreads;

	StartServiceCommunication(callback);

	for (int i = 0; i < clients; i ++)
	{
		hThreads.push_back(CreateThread(NULL, NULL, ClientThread, (LPVOID) i, NULL, NULL));
	}

	for (int i = 0; i < clients; i ++)
	{
		WaitForSingleObject(hThreads[i], INFINITE);
		CloseHandle(hThreads[i]);
	}

	StopServiceCommunication();
	return 0;
}

class CMsgCallback : public CHttpCallback
{
public:
	bool OnBrowserLocationUpdate(TCHAR *location, TCHAR *browser) {
		DWORD dwTick = GetTickCount();

		printf("%d\r\n", dwTick);
		printf("\t%S => %S\r\n", location, browser);
		printf("\r\n");

		return true;
	}

	bool OnJsonUpload(const char *jsonbuf, size_t len) {
		DWORD dwTick = GetTickCount();

		printf("%d\r\n", dwTick);
		printf("\t%s", jsonbuf);
		printf("\r\n");

		return true;
	}} srvCallback;


void testTcpComm()
{
	StartHttpDispatcher(srvCallback);

	Sleep(100);

	ParamsT params;
	params.insert(std::make_pair("browser", L"iexplore"));
	params.insert(std::make_pair("location", L"http://test.com"));
	SendHttpMessage(params);

	Sleep(1000);

	StopHttpDispatcher();
}

int testSubmit()
{
	system("fsutil file createnew submit/data1.zip 1");
	system("fsutil file createnew submit/data2.zip 5242880");
	system("fsutil file createnew submit/data3.zip 5242881");
	system("fsutil file createnew submit/data4.zip 52428800");

	submitData();

	return 0;
}

void testLogs()
{
	Trace(0);
	Trace("Ok, now..");
	Trace("Ok, now.. %d %s", 2, "doi");
	Trace(0);
}

int testDownload()
{
	PullFile(_T("settings"));
	return 0;
}

#endif
