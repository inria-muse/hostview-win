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

#include <string>
#include <vector>

#include "sqlite3.h"

#if defined(STORELIBRARY_EXPORT) // inside DLL
#   define STOREAPI   __declspec(dllexport)
#else // outside DLL
#   define STOREAPI   __declspec(dllimport)
#endif  // STORELIBRARY_EXPORT

#define STORE_FILE "stats.db"

typedef std::vector<std::string> SQLRow;
typedef std::vector<SQLRow> SQLTable;

enum IoDevice {
	Camera = 0, 
	Keyboard = 1, 
	Microphone = 2,
	Mouse = 3, 
	Speaker = 4
};

/**
 * Traffic Store Class
 * Used to insert HostView user data.
 **/
class STOREAPI CStore
{
public:
	CStore(void);
	~CStore(void);

	bool Open();
	void Close();

	void InitTables();

	// inserts a process pid mapping
	void Insert(int pid, char *name, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, DWORD state, __int64 timestamp);

	// inserts a process usage
	void Insert(DWORD pid, char *name, int memory, double cpu, __int64 timestamp);

	// insert a http header
	void Insert(char *szVerb, char *szVerbParam, char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength,
		int protocol, char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp);

	// inserts a dns mapping
	void Insert(int type, char *szIp, char *szHost, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp);

	// insert user activity
	void Insert(TCHAR *szUser, DWORD dwPid, TCHAR *szApp, TCHAR *szDescription, bool isFullScreen, bool isIdle, __int64 timestamp);

	// insert generic I/O activity
	void Insert(IoDevice device, DWORD dwPid, TCHAR *szApp, __int64 timestamp);

	// insert wifi stats
	void Insert(const TCHAR *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state, __int64 timestamp);

	// insert battery stats
	void Insert(unsigned char status, unsigned char percent, __int64 timestamp);

	// insert network connection events
	void Insert(const char *szName, const TCHAR *szFriendlyName, const TCHAR *szDescription, const TCHAR * szDnsSuffix, const TCHAR *szMac,
		const TCHAR *szIps, const TCHAR *szGateways, const TCHAR *szDnses, unsigned __int64 tSpeed, unsigned __int64 rSpeed, bool wireless,
		const TCHAR *szProfile, const char *szSSID, const TCHAR *szBSSID, const char *szBSSIDType, const char *szPHYType, unsigned long phyIndex,
		unsigned long channel, unsigned long rssi, unsigned long signal, bool connected, __int64 timestamp);

	// insert location information
	void Insert(const char *szIp, const char *szRDNS, const char *szAsNumber, const char *szAsName, const char *szCountryCode, const char * szCity,
		const char *szLat, const char *szLon, __int64 timestamp);

	// insert JSON object
	void Insert(const char *szJson, __int64 timestamp);

	// insert browser activity log
	void Insert(const TCHAR *szBrowser, const TCHAR *szLocation, __int64 timestamp);

	size_t GetQueueSize();
	DWORD ExecThread();
private:
	sqlite3 *db;
	char szError[MAX_PATH];
	const char* error();
	void seterror(const char *error);

	bool openDbFile();
	void closeDbFile();

	bool exec(const char *statement);
	SQLTable query(const char *statement);

	void enqueue(char *statement);
	std::vector<std::string> dequeue();
	HANDLE hExecThread;
	bool closing;
	CRITICAL_SECTION cs;
};
