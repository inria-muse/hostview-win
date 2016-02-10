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

#include <Windows.h>
#include <shellapi.h>

#include <winnt.h>
#include <winhttp.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include <tlhelp32.h>
#include <psapi.h>
#include <intrin.h>

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#include <shlwapi.h>

#include "proc.h"
#include "comm.h"
#include "product.h"

#define SAFE_FREE(X) \
	if (X)\
	{\
		free(X);\
		X = NULL;\
	}


#include <map>

ULARGE_INTEGER nowCPU;
ULARGE_INTEGER lastCPU;
int nCores;

std::map<DWORD, ULARGE_INTEGER> kernelTime;
std::map<DWORD, ULARGE_INTEGER> userTime;

void StartCPUQuery()
{
	if (nCores == 0)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		nCores = sysInfo.dwNumberOfProcessors;
	}

	FILETIME ftime;
	GetSystemTimeAsFileTime(&ftime);
	memcpy(&nowCPU, &ftime, sizeof(FILETIME));
}

void StopCPUQuery()
{
	lastCPU = nowCPU;
}

double QueryCPU(DWORD dwPid, HANDLE hProcess)
{
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER sys, user;

	double percent = 0;

	GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));

	ULARGE_INTEGER lastSysCPU = kernelTime[dwPid];
	ULARGE_INTEGER lastUserCPU = userTime[dwPid];

	if (lastSysCPU.QuadPart > 0 && lastUserCPU.QuadPart > 0)
	{
		percent = (double) ((sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart));

		percent /= (nowCPU.QuadPart - lastCPU.QuadPart);
		percent /= nCores;
	}

	if (percent > 1)
	{
		kernelTime.erase(dwPid);
		userTime.erase(dwPid);
		percent = 0;
	}

	kernelTime[dwPid] = sys;
	userTime[dwPid] = user;

	return percent * 100;
}

PROCAPI PerfTable* GetPerfTable()
{
	DWORD dwProcs[1024] = {0}, cbNeeded;

	std::vector<PerfRow> vRows;
	EnumProcesses(dwProcs, sizeof(dwProcs), &cbNeeded);

	StartCPUQuery();

	for (size_t i = 0; i < cbNeeded / sizeof(DWORD); i ++)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcs[i]);
		if (hProcess)
		{
			char szName[MAX_PATH] = {0};
			PROCESS_MEMORY_COUNTERS pmc;
			GetModuleBaseNameA(hProcess, NULL, szName, MAX_PATH);

			if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
			{
				vRows.push_back(PerfRow(dwProcs[i], szName, QueryCPU(dwProcs[i], hProcess), (DWORD)(pmc.WorkingSetSize / 1024)));
			}

			CloseHandle(hProcess);
		}
	}

	StopCPUQuery();

	PerfTable *pTable = new PerfTable(vRows.size());

	for (size_t i = 0; i < vRows.size(); i ++)
		pTable->get(i) = vRows[i];

	return pTable;
}

PROCAPI void ReleasePerfTable(PerfTable *table)
{
	if (table)
	{
		delete table;
	}
}

void GetProcessName(DWORD pid, char name[MAX_PATH])
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess) 
	{
		GetModuleBaseNameA(hProcess, NULL, name, MAX_PATH);
		CloseHandle(hProcess);
	}
}

// TODO: duplicate with inet_ntop from pcap.dll
const char * inet_ntop2(int af, const void *src, char *dst, int cnt)
{
	struct sockaddr_in srcaddr;
	memset(&srcaddr, 0, sizeof(srcaddr));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
	srcaddr.sin_family = af;

	struct sockaddr_in6 srcaddr6;
	memset(&srcaddr6, 0, sizeof(srcaddr6));
	memcpy(&(srcaddr6.sin6_addr), src, sizeof(srcaddr6.sin6_addr));
	srcaddr6.sin6_family = af;

	struct sockaddr * pAddr = (struct sockaddr *) &srcaddr;
	DWORD dwSize = sizeof(srcaddr);

	if (af == AF_INET6)
	{
		pAddr = (struct sockaddr *) &srcaddr6;
		dwSize = sizeof(srcaddr6);
	}

	if (WSAAddressToStringA(pAddr, dwSize, 0, dst, (LPDWORD) &cnt) != 0)
	{
		DWORD dwErr = WSAGetLastError();
		return NULL;
	}
	return dst;
}

std::vector<IpRow> ListUDPTable(int family)
{
	std::vector<IpRow> table;

	if (family == AF_INET)
	{
		MIB_UDPTABLE_OWNER_PID *pUDPInfo = NULL;
		DWORD size = 0;
    
		GetExtendedUdpTable(NULL, &size, false, family, UDP_TABLE_OWNER_PID, 0);

		pUDPInfo = (MIB_UDPTABLE_OWNER_PID*) malloc(size);

		if (GetExtendedUdpTable(pUDPInfo, &size, false, family, UDP_TABLE_OWNER_PID, 0) != NO_ERROR)
		{
			printf("Could not get our IP table.\r\n");
			SAFE_FREE(pUDPInfo);
			return table;
		}

		for (DWORD dwLoop = 0; dwLoop < pUDPInfo->dwNumEntries; dwLoop++)
		{
			MIB_UDPROW_OWNER_PID *owner = &pUDPInfo->table[dwLoop];

			u_short localPort = ntohs((u_short) owner->dwLocalPort);

			char name[MAX_PATH] = {0};
			GetProcessName(owner->dwOwningPid, name);

			char local[MAX_PATH] = {0};

			inet_ntop2(family, &owner->dwLocalAddr, local, _countof(local));

			table.push_back(IpRow(owner->dwOwningPid, name, localPort, local));
		}
		SAFE_FREE(pUDPInfo);
	}
	else if (family == AF_INET6)
	{
		MIB_UDP6TABLE_OWNER_PID *pUDPInfo = NULL;
		DWORD size = 0;
    
		GetExtendedUdpTable(NULL, &size, false, family, UDP_TABLE_OWNER_PID, 0);

		pUDPInfo = (MIB_UDP6TABLE_OWNER_PID*) malloc(size);

		if (GetExtendedUdpTable(pUDPInfo, &size, false, family, UDP_TABLE_OWNER_PID, 0) != NO_ERROR)
		{
			printf("Could not get our IP table.\r\n");
			free(pUDPInfo);
			return table;
		}

		for (DWORD dwLoop = 0; dwLoop < pUDPInfo->dwNumEntries; dwLoop++)
		{
			MIB_UDP6ROW_OWNER_PID *owner = &pUDPInfo->table[dwLoop];

			u_short localPort = ntohs((u_short) owner->dwLocalPort);

			char name[MAX_PATH] = {0};
			GetProcessName(owner->dwOwningPid, name);

			char local[MAX_PATH] = {0};
			inet_ntop2(family, &owner->ucLocalAddr, local, _countof(local));

			table.push_back(IpRow(owner->dwOwningPid, name, localPort, local));
		}
		SAFE_FREE(pUDPInfo);
	}
	return table;
}

std::vector<IpRow> ListTCPTable(int family)
{
	std::vector<IpRow> table;

	if (family == AF_INET)
	{
		MIB_TCPTABLE_OWNER_PID *pTCPInfo = NULL;
		DWORD size = 0;
    
		GetExtendedTcpTable(NULL, &size, false, family, TCP_TABLE_OWNER_PID_ALL, 0);
		pTCPInfo = (MIB_TCPTABLE_OWNER_PID*) malloc(size);
		if (GetExtendedTcpTable(pTCPInfo, &size, false, family, TCP_TABLE_OWNER_PID_ALL, 0) != NO_ERROR)
		{
			printf("Could not get our IP table.\r\n");
			SAFE_FREE(pTCPInfo);
			return table;
		}

		for (DWORD dwLoop = 0; dwLoop < pTCPInfo->dwNumEntries; dwLoop++)
		{
			MIB_TCPROW_OWNER_PID *owner = &pTCPInfo->table[dwLoop];

			u_short localPort = ntohs((u_short) owner->dwLocalPort);
			u_short remotePort = ntohs((u_short) owner->dwRemotePort);

			char remote[MAX_PATH] = {0}, local[MAX_PATH] = {0};

			inet_ntop2(family, &owner->dwLocalAddr, local, _countof(local));
			inet_ntop2(family, &owner->dwRemoteAddr, remote, _countof(remote));

			char name[MAX_PATH] = {0};
			GetProcessName(owner->dwOwningPid, name);

			table.push_back(IpRow(owner->dwOwningPid, name, localPort, local, remotePort, remote, owner->dwState));
		}
		SAFE_FREE(pTCPInfo);
	}
	else if (family == AF_INET6)
	{
		MIB_TCP6TABLE_OWNER_PID *pTCPInfo = NULL;
		DWORD size = 0;
    
		GetExtendedTcpTable(NULL, &size, false, family, TCP_TABLE_OWNER_PID_ALL, 0);
		pTCPInfo = (MIB_TCP6TABLE_OWNER_PID*) malloc(size);
		if (GetExtendedTcpTable(pTCPInfo, &size, false, family, TCP_TABLE_OWNER_PID_ALL, 0) != NO_ERROR)
		{
			printf("Could not get our IP table.\r\n");
			SAFE_FREE(pTCPInfo);
			return table;
		}

		for (DWORD dwLoop = 0; dwLoop < pTCPInfo->dwNumEntries; dwLoop++)
		{
			MIB_TCP6ROW_OWNER_PID *owner = &pTCPInfo->table[dwLoop];

			u_short localPort = ntohs((u_short) owner->dwLocalPort);
			u_short remotePort = ntohs((u_short) owner->dwRemotePort);

			char remote[MAX_PATH] = {0}, local[MAX_PATH] = {0};

			inet_ntop2(family, &owner->ucLocalAddr, local, _countof(local));
			inet_ntop2(family, &owner->ucRemoteAddr, remote, _countof(remote));

			char name[MAX_PATH] = {0};
			GetProcessName(owner->dwOwningPid, name);

			table.push_back(IpRow(owner->dwOwningPid, name, localPort, local, remotePort, remote, owner->dwState));
		}
		SAFE_FREE(pTCPInfo);
	}

	return table;
}

PROCAPI IpTable* GetIpTable()
{
	std::vector<IpRow> table = ListUDPTable(AF_INET);

	std::vector<IpRow> table2 = ListUDPTable(AF_INET6);
	table.insert(table.end(), table2.begin(), table2.end());

	table2 = ListTCPTable(AF_INET);
	table.insert(table.end(), table2.begin(), table2.end());

	table2 = ListTCPTable(AF_INET6);
	table.insert(table.end(), table2.begin(), table2.end());

	IpTable* result = new IpTable(table.size());

	for (size_t i = 0; i < table.size(); i ++)
	{
		result->get(i) = table[i];
	}

	return result;
}

PROCAPI void ReleaseIpTable(IpTable *table)
{
	if (table != NULL)
	{
		delete table;
	}
}

void QueryRegValue(HKEY hRoot, TCHAR *szPath, TCHAR *szName, TCHAR *szBuffer, DWORD dwBufferSize)
{
	HKEY hKey = NULL;
	RegOpenKey(hRoot, szPath, &hKey);
	if (hKey)
	{
		DWORD dwSize = dwBufferSize;
		DWORD dwType = REG_SZ;
		RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE) szBuffer, &dwSize);
		szBuffer[dwSize] = 0;

		RegCloseKey(hKey);
		hKey = NULL;
	}
}

bool QueryRegString(TCHAR *szKey, TCHAR *szBuffer, DWORD dwBuffSize)
{
	HKEY hKey = NULL;
	RegCreateKey(HKEY_LOCAL_MACHINE, HOSTVIEW_REG, &hKey);

	DWORD dwSize = dwBuffSize - 1;
	DWORD dwType = REG_SZ;
	RegQueryValueEx(hKey, szKey, NULL, &dwType, (LPBYTE) szBuffer, &dwSize);
	RegCloseKey(hKey);

	szBuffer[dwSize] = 0;

	return _tcslen(szBuffer) > 0;
}

void SetRegString(TCHAR *szKey, TCHAR *szBuffer)
{
	HKEY hKey = NULL;
	RegCreateKey(HKEY_LOCAL_MACHINE, HOSTVIEW_REG, &hKey);

	DWORD dwSize = sizeof(szBuffer[0]) * (DWORD) _tcslen(szBuffer);
//	DWORD dwType = REG_SZ;
	RegSetValueEx(hKey, szKey, NULL, REG_SZ, (LPBYTE) szBuffer, dwSize);
	RegCloseKey(hKey);
}

#define HddSerial _T("HddSerial")

// also in http.cpp
void trim(TCHAR * s)
{
	TCHAR * p = s;
	size_t l = _tcslen(p);

	while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
	while(* p && isspace(* p)) ++p, --l;

	_tcsncpy_s(s, l + 1, p, l + 1);
}
void QueryDriveSerial(TCHAR *szBuffer, DWORD dwBufferSize)
{
	if (QueryRegString(HddSerial, szBuffer, dwBufferSize))
	{
		return;
	}

	HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED); 

	if (FAILED(hres))
	{
		return;
	}

	hres =  CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

	if (FAILED(hres))
	{
		// TODO: we don't quite care
		//	CoUninitialize();
		//	return;
	}

	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,             
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator, (LPVOID *) &pLoc);
 
	if (FAILED(hres))
	{
		CoUninitialize();
		return;
	}

	IWbemServices *pSvc = NULL;
 
	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (for example, Kerberos)
			0,                       // Context object 
			&pSvc                    // pointer to IWbemServices proxy
			);
    
	if (FAILED(hres))
	{
		pLoc->Release();     
		CoUninitialize();
		return;
	}

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return;
	}

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"), 
		bstr_t("SELECT SerialNumber FROM Win32_PhysicalMedia"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
		NULL,
		&pEnumerator);
    
	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return;
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------
 
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;

	HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
		&pclsObj, &uReturn);

	if(uReturn > 0)
	{
		VARIANT vtProp;
		vtProp.bstrVal = NULL;

		// Get the value of the Name property
		hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);

		if (vtProp.bstrVal != NULL)
		{
			_stprintf_s(szBuffer, dwBufferSize, _T("%s"), vtProp.bstrVal);
		}
		else
		{
			_stprintf_s(szBuffer, dwBufferSize, _T("%s"), _T("error"));
		}

		VariantClear(&vtProp);

		pclsObj->Release();
	}

	// Cleanup
	// ========
    
	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	
	if(!pclsObj)
	{
		pclsObj->Release();
	}
	CoUninitialize();

	// trim white spaces
	trim(szBuffer);

	SetRegString(HddSerial, szBuffer);
}

PROCAPI void QuerySystemInfo(SysInfo &info)
{
	QueryRegValue(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\SystemInformation"),
		_T("SystemManufacturer"), info.manufacturer, _countof(info.manufacturer));
	QueryRegValue(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\SystemInformation"),
		_T("SystemProductName"), info.productName, _countof(info.productName));

	QueryRegValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
		_T("ProductName"), info.windowsName, _countof(info.windowsName));

	// architecture
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	TCHAR szArch[6] = _T("MISC");
	switch (sysInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		_tcscpy_s(szArch, _T("AMD64"));
		break;
	case PROCESSOR_ARCHITECTURE_ARM:
		_tcscpy_s(szArch, _T("ARM"));
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		_tcscpy_s(szArch, _T("IA64"));
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		_tcscpy_s(szArch, _T("INTEL"));
		break;
	}

	_tcscat_s(info.windowsName, _T(" "));
	_tcscat_s(info.windowsName, szArch);

	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	info.totalRAM = statex.ullTotalPhys;
	
	int CPUInfo[4] = {-1};
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];
	
	// Get the information associated with each extended ID.
	char cpuBrand[0x40] = { 0 };
	for(unsigned int i = 0x80000000; i <= nExIds; ++ i)
	{
		__cpuid(CPUInfo, i);
		
		// Interpret CPU brand string and cache information.
		if  (i == 0x80000002)
		{
			memcpy(cpuBrand, CPUInfo, sizeof(CPUInfo));
		}
		else if(i == 0x80000003)
		{
			memcpy(cpuBrand + 16, CPUInfo, sizeof(CPUInfo));
		}
		else if( i == 0x80000004 )
		{
			memcpy(cpuBrand + 32, CPUInfo, sizeof(CPUInfo));
		}
	}

	_stprintf_s(info.cpuName, _T("%S"), cpuBrand);

	ULARGE_INTEGER totalSize;
	GetDiskFreeSpaceEx(NULL, NULL, &totalSize, NULL);

	info.totalDisk = totalSize.QuadPart;

	QueryDriveSerial(info.hddSerial, _countof(info.hddSerial));

	// Current local timezone
	TIME_ZONE_INFORMATION tzInfo;
	GetTimeZoneInformation(&tzInfo);

	_stprintf_s(info.timezone, _T("%wS"), tzInfo.StandardName);
	info.timezone_offset = tzInfo.Bias;
}


HANDLE hMonitorThread = NULL;
bool closing = false;

bool isFullScreenPrev;
bool isIdlePrev;
DWORD dwPidPrev;

unsigned long g_userMonitorTimeout;
unsigned long g_userIdleTimeout;

struct LANGANDCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;

void GetFileDescription(TCHAR *szPath, TCHAR *szDescription, DWORD dwSize)
{
	DWORD verHandle = NULL;
	UINT size = 0;
	LPBYTE lpBuffer = NULL;
	DWORD verSize = GetFileVersionInfoSize(szPath, &verHandle);
	
	if (verSize != NULL)
	{
		LPSTR verData = new char[verSize];
		
		if (GetFileVersionInfo(szPath, verHandle, verSize, verData))
		{
			TCHAR szBuff[MAX_PATH] = {0};

			VerQueryValue(verData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*) &lpBuffer, &size);
			if (size)
			{
				LANGANDCODEPAGE *pLangs = (LANGANDCODEPAGE *) lpBuffer;
				_stprintf_s(szBuff, TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"), pLangs[0].wLanguage, pLangs[0].wCodePage);
			
				VerQueryValue(verData, szBuff, (LPVOID*) &lpBuffer, &size);
				if (size)
				{
					_tcscpy_s(szDescription, dwSize, (TCHAR *) lpBuffer);
				}
			}
		}
		delete [] verData;
	}
}

TCHAR g_szCurrPath[MAX_PATH] = {0};
PROCAPI bool IsProductProcess(DWORD dwPid)
{
	TCHAR szProcessPath[MAX_PATH] = {0};
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (hProcess)
	{
		GetModuleFileNameEx(hProcess, NULL, szProcessPath, _countof(szProcessPath));
		PathRemoveFileSpec(szProcessPath);
		CloseHandle(hProcess);
	}

	return _tcscmp(szProcessPath, g_szCurrPath) == 0;
}

PROCAPI bool GetProcessDescription(DWORD dwPid, TCHAR *szDescription, DWORD dwSize)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (hProcess)
	{
		TCHAR szProcess[MAX_PATH] = {0};
		GetModuleFileNameEx(hProcess, NULL, szProcess, _countof(szProcess));
		CloseHandle(hProcess);

		GetFileDescription(szProcess, szDescription, dwSize);
		return true;
	}

	return false;
}

PROCAPI bool GetProcessName(DWORD dwPid, TCHAR *szBuffer, DWORD nBufferSize)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (hProcess)
	{
		TCHAR szProcess[MAX_PATH] = {0};
		GetModuleBaseName(hProcess, NULL, szProcess, _countof(szProcess));
		_tcslwr_s(szProcess);

		CloseHandle(hProcess);

		_tcscpy_s(szBuffer, nBufferSize, szProcess);
	}
	else
	{
		_tcscpy_s(szBuffer, nBufferSize, _T("<elevated>"));
	}

	return true;
}

typedef HRESULT(WINAPI *SHQueryUserNotificationStatePtr) (QUERY_USER_NOTIFICATION_STATE* state);

SHQueryUserNotificationStatePtr pSHQueryUserNotificationState = NULL;

bool IsFullScreen(HWND hForeground)
{
	if (pSHQueryUserNotificationState)
	{
		QUERY_USER_NOTIFICATION_STATE notif;
		pSHQueryUserNotificationState(&notif);
		return notif != QUNS_ACCEPTS_NOTIFICATIONS;
	}
	else
	{
		if (hForeground)
		{
			// Get the monitor where the window is located.
			RECT wnd_rect;
			if (!::GetWindowRect(hForeground, &wnd_rect))
				return false;
			HMONITOR monitor = ::MonitorFromRect(&wnd_rect, MONITOR_DEFAULTTONULL);
			if (!monitor)
				return false;
			MONITORINFO monitor_info = { sizeof(monitor_info) };
			if (!::GetMonitorInfo(monitor, &monitor_info))
				return false;

			// It should be the main monitor.
			if (!(monitor_info.dwFlags & MONITORINFOF_PRIMARY))
				return false;

			// The window should be at least as large as the monitor.
			if (!::IntersectRect(&wnd_rect, &wnd_rect, &monitor_info.rcMonitor))
				return false;
			if (!::EqualRect(&wnd_rect, &monitor_info.rcMonitor))
				return false;

			// At last, the window style should not have WS_DLGFRAME and WS_THICKFRAME and
			// its extended style should not have WS_EX_WINDOWEDGE and WS_EX_TOOLWINDOW.
			LONG style = ::GetWindowLong(hForeground, GWL_STYLE);
			LONG ext_style = ::GetWindowLong(hForeground, GWL_EXSTYLE);
			return !((style & (WS_DLGFRAME | WS_THICKFRAME)) || (ext_style & (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW)));
		}
		return false;
	}
}

DWORD WINAPI MonitorThreadFunction(LPVOID lpParameter)
{
	HMODULE hShell32 = GetModuleHandle(_T("shell32.dll"));
	if (hShell32)
	{
		pSHQueryUserNotificationState = (SHQueryUserNotificationStatePtr) GetProcAddress(hShell32, "SHQueryUserNotificationState");
	}

	CMonitorCallback *pCallback = (CMonitorCallback *) lpParameter;
	if (!pCallback)
		return 0L;

	LASTINPUTINFO lastInput;
	while (!closing)
	{
		HWND hForeground = GetForegroundWindow();

		DWORD dwPid = 0;

		GetWindowThreadProcessId(hForeground, &dwPid);

		bool isFullScreen = IsFullScreen(hForeground);

		lastInput.cbSize = sizeof(lastInput);
		bool isIdle = false;
		if (GetLastInputInfo(&lastInput))
		{
			isIdle = (GetTickCount() - lastInput.dwTime) > g_userIdleTimeout;
		}

		if (isIdle != isIdlePrev
			|| isFullScreenPrev != isFullScreen
			|| dwPidPrev != dwPid)
		{
			isIdlePrev = isIdle;
			isFullScreenPrev = isFullScreen;
			dwPidPrev = dwPid;

			TCHAR szDescription[MAX_PATH] = {0};
			GetProcessDescription(dwPid, szDescription, _countof(szDescription));

			pCallback->OnApplication(dwPidPrev, isFullScreenPrev, isIdlePrev);
		}

		// now sleep until next poll time
		unsigned long dwSlept = 0;
		while (dwSlept < g_userMonitorTimeout && !closing)
		{
			Sleep(100);
			dwSlept += 100;
		}
	}
	return 0L;
}

PROCAPI bool StartUserMonitor(CMonitorCallback &callback, unsigned long userMonitorTimeout, unsigned long userIdleTimeout)
{
	g_userMonitorTimeout = userMonitorTimeout;
	g_userIdleTimeout = userIdleTimeout;

	if (!hMonitorThread)
	{
		closing = false;
		hMonitorThread = CreateThread(NULL, NULL, MonitorThreadFunction, &callback, NULL, NULL);
		return true;
	}

	return false;
}

PROCAPI bool StopUserMonitor()
{
	if (hMonitorThread)
	{
		closing = true;
		WaitForSingleObject(hMonitorThread, INFINITE);
		CloseHandle(hMonitorThread);
		hMonitorThread = NULL;
		return true;
	}
	return false;
}


BOOL EnableDebugPriv(LPCWSTR lpPrivilege, BOOL bEnable)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return FALSE;
	
	tp.PrivilegeCount = 1;
	LookupPrivilegeValue (NULL, lpPrivilege, &tp.Privileges[0].Luid);
	
	tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
	
	DWORD dwErr = GetLastError();
	return dwErr == ERROR_SUCCESS;
}

PROCAPI BOOL EnableDebugPrivileges()
{
	BOOL bRes = TRUE;
	bRes &= EnableDebugPriv(SE_TCB_NAME, TRUE);
	bRes &= EnableDebugPriv(SE_DEBUG_NAME, TRUE);
	bRes &= EnableDebugPriv(SE_ASSIGNPRIMARYTOKEN_NAME, TRUE);
	bRes &= EnableDebugPriv(SE_INCREASE_QUOTA_NAME, TRUE);
	return bRes;
}

void __cdecl DbgReport (char* __pszFormat, ...)
{
	static char s_acBuf [ 2048] = {0};

	va_list _args;

	va_start ( _args, __pszFormat);

	vsprintf_s(s_acBuf, __pszFormat, _args);

	OutputDebugStringA ( s_acBuf);

	va_end ( _args);
}


DWORD GetExplorerProcessID()
{
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	ZeroMemory(&pe32, sizeof(pe32));
	DWORD temp = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(Process32First(hSnapshot,&pe32))
	{
		do
		{
			if(!_tcscmp(pe32.szExeFile, _T("explorer.exe")))
			{
				temp = pe32.th32ProcessID;
				break;
			}
		}
		while(Process32Next(hSnapshot,&pe32));
	}

	DbgReport("Explorer PID: %d\n", temp);

	return temp;
}

void GetSidUser(PSID psid, TCHAR *pName, DWORD dwNameSize)
{
	TCHAR acReferencedDomain[MAX_PATH];
	DWORD dwDomainBufSize = sizeof(acReferencedDomain);
	SID_NAME_USE eUse;

	//  lookup clear text name of the owner
	if(!LookupAccountSid(NULL, psid, pName, &dwNameSize, acReferencedDomain, &dwDomainBufSize, &eUse))
	{
		DWORD dwErr = GetLastError();
		DbgReport("LookupAccountSid() failed: %d\n", dwErr);
	}
	else
	{
		DbgReport("SID represents %S\\%S\n", acReferencedDomain, pName);
	}
}

DWORD ExecuteCmd(TCHAR *szArgs, HANDLE hToken)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;

	if (!CreateProcessAsUser(hToken, NULL, szArgs, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		char sz[MAX_PATH] = {0};
		sprintf_s(sz, "%S", szArgs);
		WinExec(sz, SW_SHOW);
	}

	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);
	return  (0);
}

PROCAPI BOOL GetLoggedOnUser(TCHAR *szUser, DWORD dwSize)
{
	BOOL fResult = FALSE;
	HANDLE hToken = NULL;
	HANDLE hProcess = NULL;
	
	DWORD processID = GetExplorerProcessID();
	
	if(processID)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processID);
		
		if(hProcess)
		{
			if(OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
			{
				TOKEN_USER* ptu;
				DWORD dw;
				GetTokenInformation(hToken, TokenUser, NULL, 0, &dw);
				ptu = (TOKEN_USER*) _alloca(dw);
				
				if (!GetTokenInformation(hToken, TokenUser, ptu, dw, &dw))
					DbgReport("GetTokenInformation() failed, reason: %d\n", GetLastError());
				
				GetSidUser(ptu->User.Sid, szUser, dwSize);
				
				CloseHandle(hToken);

				fResult = TRUE;
			}

			CloseHandle(hProcess);
		}
	}

	return fResult;
}

PROCAPI BOOL ImpersonateCurrentUser()
{
	HANDLE hToken = NULL;
	HANDLE hProcess = NULL;

	TCHAR acName[MAX_PATH] = {0};
	DWORD dwNameSize = sizeof(acName);
	
	DWORD processID = GetExplorerProcessID();
	
	if(processID)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processID);
		
		if(hProcess)
		{
			if(OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
			{
				
				TOKEN_USER* ptu;
				DWORD dw;
				GetTokenInformation(hToken, TokenUser, NULL, 0, &dw);
				ptu = (TOKEN_USER*) _alloca(dw);
				
				if (!GetTokenInformation(hToken, TokenUser, ptu, dw, &dw))
					DbgReport("GetTokenInformation() failed, reason: %d\n", GetLastError());
				
				GetSidUser(ptu->User.Sid, acName, dwNameSize);

				if (!ImpersonateLoggedOnUser(hToken))
				{
					 DbgReport("ImpersonateLoggedOnUser() failed, reason: %d\n", GetLastError());
				}

				CloseHandle(hToken);

				return TRUE;
			}
			else
			{
				DbgReport("OpenProcessToken() failed, reason: %d\n", GetLastError());
			}
			CloseHandle(hProcess);
		}
		else
		{
			 DbgReport("OpenProcess() failed, reason: %d\n", GetLastError());
		}
	}

	return FALSE;
}

PROCAPI BOOL ImpersonateRevert()
{
	RevertToSelf();
	return TRUE;
}

PROCAPI BOOL RunAsCurrentUser(TCHAR * szCmdLine)
{
	HANDLE hToken = NULL;
	HANDLE hProcess = NULL;

	TCHAR acName[MAX_PATH] = {0};
	DWORD dwNameSize = sizeof(acName);
	
	DWORD processID = GetExplorerProcessID();
	
	if(processID)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processID);
		
		if(hProcess)
		{
			if(OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
			{
				
				TOKEN_USER* ptu;
				DWORD dw;
				GetTokenInformation(hToken, TokenUser, NULL, 0, &dw);
				ptu = (TOKEN_USER*) _alloca(dw);
				
				if (!GetTokenInformation(hToken, TokenUser, ptu, dw, &dw))
					DbgReport("GetTokenInformation() failed, reason: %d\n", GetLastError());
				
				GetSidUser(ptu->User.Sid, acName, dwNameSize);

				if (!ImpersonateLoggedOnUser(hToken))
				{
					 DbgReport("ImpersonateLoggedOnUser() failed, reason: %d\n", GetLastError());
				}

				DbgReport("Launching command: %S as \'%S\'\n", szCmdLine, acName);
				ExecuteCmd(szCmdLine, hToken);

				RevertToSelf();

				CloseHandle(hToken);

				return TRUE;
			}
			else
			{
					DbgReport("OpenProcessToken() failed, reason: %d\n", GetLastError());
			}
			CloseHandle(hProcess);
		}
		else
		{
			 DbgReport("OpenProcess() failed, reason: %d\n", GetLastError());
		}
	}

	return FALSE;
}

PROCAPI BOOL CreatePublicFolder(TCHAR *szPath)
{
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	PSID everyone_sid = NULL;
	AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 
		0, 0, 0, 0, 0, 0, 0, &everyone_sid);

	EXPLICIT_ACCESS ea;
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPWSTR)everyone_sid;

	PACL acl = NULL;
	SetEntriesInAcl(1, &ea, NULL, &acl);

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, 
	SECURITY_DESCRIPTOR_MIN_LENGTH);
	InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = sd;
	sa.bInheritHandle = FALSE;

	CreateDirectory(szPath, &sa);

	FreeSid(everyone_sid);
	LocalFree(sd);
	LocalFree(acl);

	return TRUE;
}

PROCAPI void InitCurrentDirectory()
{
	GetModuleFileName(NULL, g_szCurrPath, _countof(g_szCurrPath));
	PathRemoveFileSpec(g_szCurrPath);
	SetCurrentDirectory(g_szCurrPath);
}


// MultiMedia Sessions - Mic or Speakers (works from Vista+)
#include "stdafx.h"
#include "Objbase.h"
#include "Mmdeviceapi.h"
#include "Audiopolicy.h"
#include <vector>

#define EXIT_ON_ERROR(hr)	if (!SUCCEEDED(hr)) goto Exit;
#define SAFE_RELEASE(p) if (p) { p->Release(), p = NULL;}

// eRender or eCapture
PROCAPI int QueryMultiMediaSessions(DWORD * &pSessions, bool isCapture)
{
	std::vector<DWORD> pids;

	EDataFlow eFlow = isCapture ? eCapture : eRender;

	HRESULT hr;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioSessionManager2 *pSessionMgr = NULL;

	CoInitialize(NULL);

	// Get enumerator for audio endpoint devices.
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
		NULL, CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr);

	hr = pEnumerator->GetDefaultAudioEndpoint(eFlow, eConsole, &pDevice);
	EXIT_ON_ERROR(hr);

	hr = pDevice->Activate(__uuidof(IAudioSessionManager2),
		CLSCTX_ALL, NULL, (void**)&pSessionMgr);
	EXIT_ON_ERROR(hr);


	IAudioSessionEnumerator *pSessionEnumerator = NULL;
	pSessionMgr->GetSessionEnumerator(&pSessionEnumerator);

	if (pSessionEnumerator)
	{
		int nCount = 0;
		pSessionEnumerator->GetCount(&nCount);

		for (int i = 0; i < nCount; i ++)
		{
			IAudioSessionControl *pSessionCtrl = NULL;
			pSessionEnumerator->GetSession(i, &pSessionCtrl);

			if (pSessionCtrl)
			{
				IAudioSessionControl2 *pSessionCtrl2 = (IAudioSessionControl2 *) pSessionCtrl;

				AudioSessionState state;

				pSessionCtrl2->GetState(&state);

				DWORD dwPid = 0;
				pSessionCtrl2->GetProcessId(&dwPid);

				if (dwPid && state == AudioSessionStateActive)
				{
					pids.push_back(dwPid);
				}

				SAFE_RELEASE(pSessionCtrl);
			}
		}
	}

	SAFE_RELEASE(pSessionEnumerator);

Exit:
	SAFE_RELEASE(pEnumerator);
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pSessionMgr);
	CoUninitialize();

	pSessions = new DWORD[pids.size()];
	for (size_t i = 0; i < pids.size(); i ++)
		pSessions[i] = pids[i];

	return (int) pids.size();
}

PROCAPI void ReleaseMultiMediaSessions(DWORD * &pSessions)
{
	if (pSessions)
	{
		delete [] pSessions;
		pSessions = NULL;
	}
}

POINT ptPrevMouse = {-1, -1};

PROCAPI void QueryMouseKeyboardState(bool &isMouseUsed, bool &isKeyboardUsed)
{
	BYTE arrState[256] = {0};
	memset(arrState, 0, sizeof(arrState));
	GetKeyboardState(arrState);

	isKeyboardUsed = false;
	for (int i = 5; i < sizeof(arrState); i ++)
	{
		if (arrState[i] & 0x80)
		{
			isKeyboardUsed = true;
			break;
		}
	}

	isMouseUsed = GetKeyState(VK_LBUTTON) & 0x100
		|| GetKeyState(VK_RBUTTON) & 0x100
		|| GetKeyState(VK_MBUTTON);

	POINT ptCurrMouse;
	GetCursorPos(&ptCurrMouse);

	// mouse moved
	if (ptPrevMouse.x != ptCurrMouse.x || ptPrevMouse.y != ptCurrMouse.y)
	{
		// not first time
		if (ptPrevMouse.x != -1 && ptPrevMouse.y != -1)
		{
			isMouseUsed = true;
		}
	}

	ptPrevMouse.x = ptCurrMouse.x;
	ptPrevMouse.y = ptCurrMouse.y;
}

#include "camera.h"
#include "handles.h"

std::vector<std::tstring> installedCams;

extern "C" PROCAPI int QueryWebcamSessions(DWORD * &pSessions)
{
	if (!installedCams.size())
	{
		// query once
		installedCams = GetWebcams();
	}

	std::vector<DWORD> pids;

	for (size_t i = 0; i < installedCams.size(); i ++)
	{
		int dwPid = QueryOpenedHandle((TCHAR *) installedCams[i].c_str());
		if (dwPid != -1)
			pids.push_back((DWORD)dwPid);
	}

	pSessions = new DWORD[pids.size()];
	for (size_t i = 0; i < pids.size(); i ++)
		pSessions[i] = pids[i];

	return (int) pids.size();
}

extern "C" PROCAPI void ReleaseWebcamSessions(DWORD * &pSessions)
{
	if (pSessions)
	{
		delete [] pSessions;
		pSessions = NULL;
	}
}