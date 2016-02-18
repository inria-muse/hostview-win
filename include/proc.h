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

#include <winsock2.h>
#include <string>
#include <vector>
#include <Shlwapi.h>
#include <shellapi.h>

#if defined(PROCLIBRARY_EXPORT) // inside DLL
#   define PROCAPI   extern "C" __declspec(dllexport)
#else // outside DLL
#   define PROCAPI   extern "C" __declspec(dllimport)
#endif  // PROCLIBRARY_EXPORT

#ifndef byte
typedef unsigned char byte;
#endif

#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif

struct IpRow
{
private:
	bool udp;

public:
	DWORD pid;
	DWORD state;
	u_short srcport;
	u_short destport;
	char srcIp[MAX_PATH];
	char destIp[MAX_PATH];
	char name[MAX_PATH];

	bool IsUDP()
	{
		return udp;
	}

	IpRow()
		: pid(0),
		srcport(0),
		destport(0),
		state(0)
	{
		srcIp[0] = 0;
		destIp[0] = 0;
	}

	IpRow& operator=(const IpRow &rhs)
	{
		this->pid = rhs.pid;
		this->srcport = rhs.srcport;
		this->destport = rhs.destport;
		this->udp = rhs.udp;
		this->state = rhs.state;

		strcpy_s(this->srcIp, rhs.srcIp);
		strcpy_s(this->destIp, rhs.destIp);
		strcpy_s(this->name, rhs.name);
		return *this;
	}

	IpRow(DWORD pid, char *name, u_short srcport, char *srcIp)
	{
		this->pid = pid;
		this->srcport = srcport;
		this->state = 2; // MIB_TCP_STATE_LISTEN;

		strcpy_s(this->name, name);
		strcpy_s(this->srcIp, srcIp);

		destport = 0;
		destIp[0] = 0;

		udp = true;
	}

	IpRow(DWORD pid, char *name, u_short srcport, char *srcIp,
		u_short destport, char*destIp, DWORD state)
	{
		this->pid = pid;
		this->srcport = srcport;
		this->destport = destport;
		this->state = state;

		strcpy_s(this->srcIp, srcIp);
		strcpy_s(this->destIp, destIp);
		strcpy_s(this->name, name);

		udp = false;
	}
};

struct IpTable
{
	IpRow *rows;
	size_t size;

	IpTable(size_t size)
	{
		this->size = size;
		rows = new IpRow[size];
	}

	~IpTable()
	{
		if (rows)
		{
			delete [] rows;
			size = 0;
		}
	}

	IpRow& get(size_t i)
	{
		return rows[i];
	}
};

extern "C" PROCAPI IpTable* GetIpTable();
extern "C" PROCAPI void ReleaseIpTable(IpTable *table);

struct PerfRow
{
	DWORD pid;
	char name[MAX_PATH];
	double cpu;
	DWORD memory;

	PerfRow& operator=(const PerfRow &rhs)
	{
		this->pid = rhs.pid;
		strcpy_s(this->name, rhs.name);
		this->cpu = rhs.cpu;
		this->memory = rhs.memory;

		return *this;
	}

	PerfRow()
		: pid(0),
		cpu(0),
		memory(0)
	{
		name[0] = 0;
	}

	PerfRow(DWORD pid, char *name, double cpu, DWORD memory)
	{
		this->pid = pid;
		strcpy_s(this->name, name);
		this->cpu = cpu;
		this->memory = memory;
	}
};


struct PerfTable
{
	PerfRow *rows;
	size_t size;

	PerfTable(size_t size)
	{
		this->size = size;
		rows = new PerfRow[size];
	}

	~PerfTable()
	{
		if (rows)
		{
			delete [] rows;
			size = 0;
		}
	}

	PerfRow& get(size_t i)
	{
		return rows[i];
	}
};

extern "C" PROCAPI PerfTable* GetPerfTable();
extern "C" PROCAPI void ReleasePerfTable(PerfTable *table);

struct SysInfo
{
	TCHAR productName[MAX_PATH];
	TCHAR manufacturer[MAX_PATH];
	TCHAR cpuName[MAX_PATH];
	unsigned __int64 totalRAM;
	unsigned __int64 totalDisk;
	TCHAR windowsName[MAX_PATH];
	TCHAR hddSerial[MAX_PATH];	
	TCHAR timezone[32];
	unsigned long timezone_offset;

	SysInfo()
	{
		totalRAM = 0;
		totalDisk = 0;
		productName[0] = 0;
		manufacturer[0] = 0;
		cpuName[0] = 0;
		windowsName[0] = 0;
		hddSerial[0] = 0;
		timezone[0] = 0;
		timezone_offset = 0;
 	}
};

extern "C" PROCAPI void QuerySystemInfo(SysInfo &info);

class CMonitorCallback
{
public:
	virtual void OnApplication(DWORD dwPid, bool isFullScreen, bool isIdle) = 0;
};

extern "C" PROCAPI bool StartUserMonitor(CMonitorCallback &callback, unsigned long userMonitorTimeout, unsigned long userIdleTimeout);
extern "C" PROCAPI bool StopUserMonitor();
extern "C" PROCAPI bool GetProcessName(DWORD dwPid, TCHAR *szBuffer, DWORD nBufferSize);
extern "C" PROCAPI bool GetProcessDescription(DWORD dwPid, TCHAR *szDescription, DWORD dwSize);
extern "C" PROCAPI bool IsProductProcess(DWORD dwPid);

// Utils
extern "C" PROCAPI BOOL EnableDebugPrivileges();
extern "C" PROCAPI BOOL RunAsCurrentUser(TCHAR *szCmdLine);
extern "C" PROCAPI BOOL ImpersonateCurrentUser();
extern "C" PROCAPI BOOL ImpersonateRevert();
extern "C" PROCAPI BOOL GetLoggedOnUser(TCHAR *szUser, DWORD dwSize);
extern "C" PROCAPI BOOL CreatePublicFolder(TCHAR *szPath);
extern "C" PROCAPI void InitCurrentDirectory();
extern "C" PROCAPI void QueryProcessIcon(DWORD dwPid, TCHAR *szApp);
extern "C" PROCAPI void QueryIcons(int nCount, TCHAR ** &szAppList);

// installed apps
extern "C" PROCAPI void PullInstalledApps(TCHAR *szPath, DWORD dwSize);
extern "C" PROCAPI int QueryApplicationsList(TCHAR ** &pszPaths, TCHAR ** &pszDescriptions);
extern "C" PROCAPI void ReleaseApplicationsList(int nCount, TCHAR ** &pszPaths, TCHAR ** &pszDescriptions);

// I / O
extern "C" PROCAPI int QueryWebcamSessions(DWORD * &pSessions);
extern "C" PROCAPI void ReleaseWebcamSessions(DWORD * &pSessions);

extern "C" PROCAPI int QueryMultiMediaSessions(DWORD * &pSession, bool isCapture);
extern "C" PROCAPI void ReleaseMultiMediaSessions(DWORD * &pSession);

extern "C" PROCAPI void QueryMouseKeyboardState(bool &isMouseUsed, bool &isKeyboardUsed);

