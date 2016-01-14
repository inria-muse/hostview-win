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

#include "StdAfx.h"

#include "trace.h"
#include "Store.h"
#include "Settings.h"
#include "Upload.h"

DWORD dwLastFileSizeCheck = 0;

std::vector<std::string> inserts;

__int64 GetTimestamp()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	ULARGE_INTEGER epoch;
	epoch.LowPart = 0xD53E8000;
	epoch.HighPart = 0x019DB1DE;

	ULARGE_INTEGER ts;
	ts.LowPart = ft.dwLowDateTime;
	ts.HighPart = ft.dwHighDateTime;
	ts.QuadPart -= epoch.QuadPart;

	return ts.QuadPart / 10000;
}

void CStore::enqueue(char *statement)
{
	EnterCriticalSection(&cs);

	inserts.push_back(statement);

	LeaveCriticalSection(&cs);
}

std::vector<std::string> CStore::dequeue()
{
	EnterCriticalSection(&cs);

	std::vector<std::string> newqueue = inserts;
	inserts.clear();

	LeaveCriticalSection(&cs);

	return newqueue;
}

size_t CStore::GetQueueSize()
{
	EnterCriticalSection(&cs);
	size_t size = inserts.size();
	LeaveCriticalSection(&cs);
	return size;
}

DWORD WINAPI ExecThreadFunc(LPVOID lpParameter)
{
	CStore *ctx = (CStore *) lpParameter;
	if (ctx)
	{
		return ctx->ExecThread();
	}
	return 0L;
}

// TODO: use prepared statements
DWORD CStore::ExecThread()
{
	CSettings settings;
	unsigned long dbSizeLimit = settings.GetULong(DbSizeLimit);

	do
	{
		Sleep(5);

#ifndef _DEBUG
		// write the db in batches
		if (GetQueueSize() < 1000 && !closing)
			continue;
#endif

		exec("BEGIN TRANSACTION");

		std::vector<std::string> newinserts = dequeue();
		for (size_t i = 0; i < newinserts.size(); i ++)
		{
			Debug("[store] write %s\n", newinserts[i].c_str());
			exec(newinserts[i].c_str());
		}

		exec("COMMIT TRANSACTION");

		if (GetTickCount() - dwLastFileSizeCheck > 60 * 1000 && !closing)
		{
			dwLastFileSizeCheck = GetTickCount();

			WIN32_FIND_DATAA data;
			HANDLE hFind = FindFirstFileA(szDbFile, &data);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				__int64 fileSize = data.nFileSizeLow | (__int64)data.nFileSizeHigh << 32;
				FindClose(hFind);

				if (fileSize >= dbSizeLimit)
				{
					// reset the db file
					closeDbFile();
					openDbFile();
				}
			}
		}

		if (GetQueueSize() == 0 && closing)
		{
			break;
		}
	}
	while (true);

	return 0;
}

CStore::CStore(void)
	: db(NULL),
	closing(false),
	hExecThread(NULL)
{
	InitializeCriticalSection(&cs);
}


CStore::~CStore(void)
{
	Close();
	DeleteCriticalSection(&cs);
}

bool CStore::Open()
{
	bool result = openDbFile();
	if (result) {
		closing = false;
		hExecThread = CreateThread(NULL, NULL, ExecThreadFunc, this, NULL, NULL);
	}
	return result;
}

void CStore::Close()
{
	if (hExecThread)
	{
		closing = true;
		WaitForSingleObject(hExecThread, INFINITE);
		CloseHandle(hExecThread);
		hExecThread = NULL;
	}
	closeDbFile();
}

bool CStore::openDbFile()
{
	char * error = NULL;
	memset(szDbFile, 0, MAX_PATH);
	sprintf_s(szDbFile, MAX_PATH, "%llu_stats.db", GetTimestamp());

	if (sqlite3_open_v2(szDbFile, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
	{
		seterror(sqlite3_errmsg(db));
		sqlite3_free(error);
		return false;
	}
	else
	{
		seterror("Success");
		InitTables();
		return true;
	}
}

void CStore::closeDbFile()
{
	if (db)
	{
		sqlite3_close(db);
		db = NULL;
		AddFileToSubmit(szDbFile);
		memset(szDbFile, 0, MAX_PATH);
	}
}

void CStore::InitTables()
{
	exec("CREATE TABLE IF NOT EXISTS connectivity(name VARCHAR(260), friendlyname VARCHAR(260), description VARCHAR(260), dnssuffix VARCHAR(260), \
		 mac VARCHAR(64), ips VARCHAR(300), gateways VARCHAR(300), dnses VARCHAR(300), tspeed INT8, rspeed INT8, wireless TINYINT, profile VARCHAR(64), \
		 ssid VARCHAR(64), bssid VARCHAR(64), bssidtype VARCHAR(20), phytype VARCHAR(20), phyindex INTEGER, channel INTEGER, rssi INTEGER, signal INTEGER, \
		 connected TINYINT, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS wifistats(guid VARCHAR(260), tspeed INT8, rspeed INT8, signal INTEGER, \
		 rssi INTEGER, state INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS battery(status INTEGER, percent INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS activity(user VARCHAR(260), pid INTEGER, name VARCHAR(260), description VARCHAR(260), \
		 fullscreen TINYINT, idle TINYINT, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS io(device INTEGER, pid INTEGER, name VARCHAR(260), timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS procs(pid INTEGER, name VARCHAR(260), memory INTEGER, \
		cpu DOUBLE, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS ports(pid INTEGER, name VARCHAR(260), protocol INTEGER, \
		srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, state INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS http(httpverb VARCHAR(64), httpverbparam VARCHAR(300), httpstatuscode VARCHAR(64), \
		 httphost VARCHAR(300), referer VARCHAR(300), contenttype VARCHAR(300), contentlength VARCHAR(300), protocol INTEGER,\
		 srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS dns(type INTEGER, ip VARCHAR(42), host VARCHAR(260), protocol INTEGER, \
		srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS location(ip VARCHAR(100), rdns VARCHAR(100), asnumber VARCHAR(100), asname VARCHAR(100),\
		 countryCode VARCHAR(100), city VARCHAR(100), lat VARCHAR(100), lon VARCHAR(100), timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS browseractivity(timestamp INT8, browser VARCHAR(1024), location VARCHAR(1024))");

	exec("CREATE TABLE IF NOT EXISTS jsonobj(timestamp INT8, obj TEXT)");

	exec("PRAGMA synchronous = OFF");
	exec("PRAGMA journal_mode = MEMORY");
}

void CStore::Insert(const char *szName, const TCHAR *szFriendlyName, const TCHAR *szDescription, const TCHAR *szDnsSuffix,
	const TCHAR *szMac, const TCHAR *szIps, const TCHAR *szGateways, const TCHAR *szDnses, unsigned __int64 tSpeed, unsigned __int64 rSpeed,
	bool wireless, const TCHAR *szProfile, const char *szSSID, const TCHAR *szBSSID, const char *szBSSIDType, const char *szPHYType,
	unsigned long phyIndex, unsigned long channel, unsigned long rssi, unsigned long signal, bool connected, __int64 timestamp)
{
	char szStatement[8192] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(name, friendlyname, description, dnssuffix, mac, ips, gateways, dnses, tspeed, rspeed, wireless, profile, \
		 ssid, bssid, bssidtype, phytype, phyindex, channel, rssi, signal, connected, timestamp) VALUES(\"%s\", \"%S\", \"%S\", \"%S\", \"%S\", \
		 \"%S\",\"%S\", \"%S\", %llu, %llu, %d, \"%S\", \"%s\", \"%S\", \"%s\", \"%s\", %lu, %lu, %ld, %lu, %d, %llu)", "connectivity", szName,
		 szFriendlyName, szDescription, szDnsSuffix, szMac, szIps, szGateways, szDnses, tSpeed, rSpeed, wireless ? 1 : 0, szProfile,
		 szSSID, szBSSID, szBSSIDType, szPHYType, phyIndex, channel, rssi, signal, connected ? 1 : 0, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(const TCHAR *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state, __int64 timestamp)
{
	char szStatement[8192] = {0};

	sprintf_s(szStatement, "INSERT INTO %s(guid, tspeed, rspeed, signal, rssi, state, timestamp) VALUES(\"%S\", %llu, %llu, %d, %d, %d, %llu)",
		"wifistats", szGuid, tSpeed, rSpeed, signal, rssi, state, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(unsigned char status, unsigned char percent, __int64 timestamp)
{
	char szStatement[8192] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(status, percent, timestamp) VALUES(%d, %d, %llu)", "battery", status, percent, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(TCHAR *szUser, DWORD dwPid, TCHAR *szApp, TCHAR *szDescription, bool isFullScreen, bool isIdle, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(user, pid, name, description, fullscreen, idle, timestamp) VALUES(\"%S\", %d, \"%S\", \"%S\", \
						   %d, %d, %llu)", "activity", szUser, dwPid, szApp, szDescription, isFullScreen, isIdle, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(IoDevice device, DWORD dwPid, TCHAR *szApp, __int64 timestamp)
{
	char szStatement[4096] = {0};

	sprintf_s(szStatement, "INSERT INTO %s(device, pid, name, timestamp) VALUES(%d, %d, \"%S\", %llu)",
		"io", device, dwPid, szApp, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(int pid, char *name, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, DWORD state, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(pid, name, protocol, srcip, destip, srcport, destport, state, timestamp)\
						   VALUES(%d, \"%s\", %d, \"%s\", \"%s\", %d, %d, %d, %llu)", "ports", pid, name, protocol,
						   srcIp, destIp, srcPort, destPort, state, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(DWORD pid, char *name, int memory, double cpu, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(pid, name, memory, cpu, timestamp) VALUES(%d, \"%s\", %d, %.2f, %llu)",
		"procs", pid, name, memory, cpu, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(char *szVerb, char *szVerbParam, char *szStatusCode, char *szHost, char *szReferer, char *szContentType,
	char *szContentLength, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(httpverb, httpverbparam, httpstatuscode, httphost, referer, contenttype, contentlength, \
						   protocol, srcip, destip, srcport, destport, timestamp) VALUES(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \
						   \"%s\", %d, \"%s\", \"%s\", %d, %d, %llu)", "http", szVerb, szVerbParam, szStatusCode, szHost, szReferer,
						   szContentType, szContentLength, protocol, srcIp, destIp, srcPort, destPort, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(int type, char *szIp, char *szHost, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(type, ip, host, protocol, srcip, destip, srcport, destport, timestamp) \
						   VALUES(%d, \"%s\", \"%s\", %d, \"%s\", \"%s\", %d, %d, %llu)", "dns", type, szIp, szHost,
						   protocol, srcIp, destIp, srcPort, destPort, timestamp);

	enqueue(szStatement);
}

void CStore::Insert(const char *szIp, const char *szRDNS, const char *szAsNumber, const char *szAsName, const char *szCountryCode, const char * szCity,
	const char *szLat, const char *szLon, __int64 timestamp)
{
	char szStatement[4096] = {0};

	sprintf_s(szStatement, "INSERT INTO %s(ip, rdns, asnumber, asname, countryCode, city, lat, lon, timestamp) \
						   VALUES(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %llu)", "location",
						   szIp, szRDNS, szAsNumber, szAsName, szCountryCode, szCity, szLat, szLon, timestamp);

	exec(szStatement);
}


void CStore::Insert(const char *szJson, __int64 timestamp)
{
	// FIXME: allocate dynamically based on the actual size of the object ?
	char szStatement[65536] = { 0 };

	sprintf_s(szStatement, "INSERT INTO %s(obj, timestamp) VALUES(\"%s\", %llu)", "jsonobj", szJson, timestamp);

	exec(szStatement);
}

void CStore::Insert(const TCHAR *szBrowser, const TCHAR *szLocation, __int64 timestamp)
{
	char szStatement[4096] = { 0 };

	sprintf_s(szStatement, "INSERT INTO %s(browser, location, timestamp) VALUES(\"%S\", \"%S\", %llu)", 
		"browseractivity", szBrowser, szLocation, timestamp);

	exec(szStatement);
}

const char* CStore::error()
{
	return szError;
}

void CStore::seterror(const char *error)
{
	strcpy_s(szError, error);
}

bool CStore::exec(const char *statement)
{
	bool result = true;
	char * error =  NULL;

	if (sqlite3_exec(db, statement, NULL, NULL, &error))
	{
		seterror(sqlite3_errmsg(db));
		sqlite3_free(error);
		result = false;
	}
	else
	{
		seterror("Success");
	}
	return result;
}

SQLTable CStore::query(const char *statement)
{
	SQLTable table;

	char **results = NULL;
	int rows, columns;	
	char * error =  NULL;

	if (sqlite3_get_table(db, statement, &results, &rows, &columns, &error))
	{
		seterror(sqlite3_errmsg(db));
		sqlite3_free(error);
	}
	else
	{
		for (int rowCtr = 0; rowCtr <= rows; ++rowCtr)
		{
			SQLRow row;
			for (int colCtr = 0; colCtr < columns; ++colCtr)
			{
				int cellPosition = (rowCtr * columns) + colCtr;
				row.push_back(results[cellPosition]);
			}
			table.push_back(row);
		}

		sqlite3_free_table(results);
	}
		
	return table;
}
