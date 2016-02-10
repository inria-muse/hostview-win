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

	do
	{
		Sleep(10);

		// write the db in batches
		if (GetQueueSize() < 100 && !closing)
			continue;

		exec("BEGIN TRANSACTION");

		std::vector<std::string> newinserts = dequeue();
		for (size_t i = 0; i < newinserts.size(); i ++)
		{
			exec(newinserts[i].c_str());
		}

		exec("COMMIT TRANSACTION");

		if (GetQueueSize() == 0 && closing)
		{
			// queue is empty and shutting down
			break;
		}
	}
	while (true);

	return 0;
}

CStore::CStore(void)
	: db(NULL),
	closing(false),
	m_session(0),
	hExecThread(NULL)
{
	InitializeCriticalSection(&cs);
}


CStore::~CStore(void)
{
	DeleteCriticalSection(&cs);
}

bool CStore::Open(ULONGLONG session)
{
	if (m_session > 0)
		Close();

	if (sqlite3_open_v2(STORE_FILE, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
	{
		seterror(sqlite3_errmsg(db));
		return false;
	}
	seterror("Success");

	InitTables();

	m_session = session;
	closing = false;
	hExecThread = CreateThread(NULL, NULL, ExecThreadFunc, this, NULL, NULL);

	return true;
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

	if (db)
	{
		sqlite3_close(db);
	}

	if (m_session > 0) {
		// rename with session id
		char uploadfile[MAX_PATH] = { 0 };
		sprintf_s(uploadfile, "%llu_%s", m_session, STORE_FILE);
		MoveFileA(STORE_FILE, uploadfile);
		MoveFileToSubmit(uploadfile);
	}

	closing = false;
	db = NULL;
	m_session = 0;
}

void CStore::InitTables()
{
	exec("CREATE TABLE IF NOT EXISTS connectivity(guid VARCHAR(260), friendlyname VARCHAR(260), description VARCHAR(260), dnssuffix VARCHAR(260), \
		 mac VARCHAR(64), ips VARCHAR(300), gateways VARCHAR(300), dnses VARCHAR(300), tspeed INT8, rspeed INT8, wireless TINYINT, profile VARCHAR(64), \
		 ssid VARCHAR(64), bssid VARCHAR(64), bssidtype VARCHAR(20), phytype VARCHAR(20), phyindex INTEGER, channel INTEGER, \
		 connected TINYINT, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS wifistats(guid VARCHAR(260), tspeed INT8, rspeed INT8, signal INTEGER, \
		 rssi INTEGER, state INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS procs(pid INTEGER, name VARCHAR(260), memory INTEGER, \
		cpu DOUBLE, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS ports(pid INTEGER, name VARCHAR(260), protocol INTEGER, \
		srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, state INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS activity(user VARCHAR(260), pid INTEGER, name VARCHAR(260), description VARCHAR(260), \
		 fullscreen TINYINT, idle TINYINT, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS io(device INTEGER, pid INTEGER, name VARCHAR(260), timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS http(connstart INT8, httpverb VARCHAR(64), httpverbparam VARCHAR(300), httpstatuscode VARCHAR(64), \
		 httphost VARCHAR(300), referer VARCHAR(300), contenttype VARCHAR(300), contentlength VARCHAR(300), protocol INTEGER,\
		 srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS dns(connstart INT8, type INTEGER, ip VARCHAR(42), host VARCHAR(260), protocol INTEGER, \
		srcip VARCHAR(42), destip VARCHAR(42), srcport INTEGER, destport INTEGER, timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS location(guid VARCHAR(260), public_ip VARCHAR(100), reverse_dns VARCHAR(100), \
		 asnumber VARCHAR(100), asname VARCHAR(100),\
		 countryCode VARCHAR(100), city VARCHAR(100), lat VARCHAR(100), lon VARCHAR(100), timestamp INT8)");

	exec("CREATE TABLE IF NOT EXISTS browseractivity(timestamp INT8, browser VARCHAR(1024), location VARCHAR(1024))");

	exec("CREATE TABLE IF NOT EXISTS session(timestamp INT8, event VARCHAR(32))");

	exec("CREATE TABLE IF NOT EXISTS sysinfo(timestamp INT8, manufacturer VARCHAR(32), product VARCHAR(32), os VARCHAR(32),\
	     cpu VARCHAR(32), totalRAM INTEGER, totalHDD INTEGER, serial VARCHAR(32), hostview_version VARCHAR(32), settings_version INTEGER,\
		 timezone VARCHAR(32), timezone_offset INTEGER)");

	exec("CREATE TABLE IF NOT EXISTS powerstate(timestamp INT8, event VARCHAR(32), value INT)");

	exec("CREATE TABLE IF NOT EXISTS netlabel(timestamp INT8, guid VARCHAR(260), gateway VARCHAR(64), label VARCHAR(260))");

	exec("PRAGMA synchronous = OFF");
	exec("PRAGMA journal_mode = MEMORY");
}

void CStore::InsertNetLabel(__int64 timestamp, TCHAR *szGUID, TCHAR *szGW, TCHAR *szLabel) {
	char szStatement[4096] = { 0 };
	sprintf_s(szStatement, "INSERT INTO %s(timestamp, guid, gateway, label) VALUES(%llu, \"%S\", \"%S\", \"%S\")",
		"netlabel", timestamp, szGUID, szGW, szLabel);
	enqueue(szStatement);
}


void CStore::InsertSession(__int64 timestamp, SessionEvent e) {
	char szStatement[1024] = { 0 };
	switch (e) {
	case SessionEvent::Start:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"start\")", "session", timestamp);
		break;
	case SessionEvent::Stop:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"stop\")", "session", timestamp);
		break;
	case SessionEvent::Pause:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"pause\")", "session", timestamp);
		break;
	case SessionEvent::Restart:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"restart\")", "session", timestamp);
		break;
	case SessionEvent::Suspend:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"suspend\")", "session", timestamp);
		break;
	case SessionEvent::Resume:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"resume\")", "session", timestamp);
		break;
	case SessionEvent::AutoStop:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"autostop\")", "session", timestamp);
		break;
	case SessionEvent::AutoStart:
		sprintf_s(szStatement, "INSERT INTO %s(timestamp, event) VALUES(%llu, \"autostart\")", "session", timestamp);
		break;
	default:
		return;
	}

	enqueue(szStatement);
}

void CStore::InsertSys(__int64 timestamp, SysInfo &info, char *hostview_version, ULONG settings_version) {
	char szStatement[8192] = { 0 };

	sprintf_s(szStatement, "INSERT INTO %s(timestamp, manufacturer, product, os, cpu, totalRAM, totalHDD, serial, \
							hostview_version, settings_version, timezone, timezone_offset) \
							VALUES(%llu, \"%S\", \"%S\", \"%S\", \"%S\", %llu, %llu, \"%S\", \"%s\", %lu, \"%S\", %lu)",
							"sysinfo", 
							timestamp, info.manufacturer, info.productName, info.windowsName,
							info.cpuName, info.totalRAM, info.totalDisk, info.hddSerial, 
							hostview_version, settings_version, info.timezone,info.timezone_offset);

	enqueue(szStatement);
}

void CStore::InsertConn(const char *szGuid, const TCHAR *szFriendlyName, const TCHAR *szDescription, const TCHAR *szDnsSuffix,
	const TCHAR *szMac, const TCHAR *szIps, const TCHAR *szGateways, const TCHAR *szDnses, unsigned __int64 tSpeed, unsigned __int64 rSpeed,
	bool wireless, const TCHAR *szProfile, const char *szSSID, const TCHAR *szBSSID, const char *szBSSIDType, const char *szPHYType,
	unsigned long phyIndex, unsigned long channel, bool connected, __int64 timestamp)
{
	char szStatement[8192] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(guid, friendlyname, description, dnssuffix, mac, ips, gateways, dnses, tspeed, rspeed, wireless, profile, \
		 ssid, bssid, bssidtype, phytype, phyindex, channel, connected, timestamp) VALUES(\"%s\", \"%S\", \"%S\", \"%S\", \"%S\", \
		 \"%S\",\"%S\", \"%S\", %llu, %llu, %d, \"%S\", \"%s\", \"%S\", \"%s\", \"%s\", %lu, %lu, %d, %llu)", 
		"connectivity", szGuid, szFriendlyName, szDescription, szDnsSuffix, szMac, szIps, szGateways, szDnses, tSpeed, rSpeed, 
		 wireless ? 1 : 0, szProfile, szSSID, szBSSID, szBSSIDType, szPHYType, phyIndex, channel, connected ? 1 : 0, timestamp);

	enqueue(szStatement);
}

void CStore::InsertWifi(const char *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed, ULONG signal, ULONG rssi, short state, __int64 timestamp)
{
	char szStatement[4096] = { 0 };
	sprintf_s(szStatement, "INSERT INTO %s(guid, tspeed, rspeed, signal, rssi, state, timestamp) VALUES(\"%s\", %llu, %llu, %d, %d, %d, %llu)",
		"wifistats", szGuid, tSpeed, rSpeed, signal, rssi, state, timestamp);

	enqueue(szStatement);
}

void CStore::InsertPowerState(__int64 timestamp, char *event, int value) {
	char szStatement[4096] = { 0 };
	sprintf_s(szStatement, "INSERT INTO %s(timestamp, event, value) VALUES(%llu, \"%s\", %d)",
		"powerstate", timestamp, event, value);

	enqueue(szStatement);
}

void CStore::InsertActivity(TCHAR *szUser, DWORD dwPid, TCHAR *szApp, TCHAR *szDescription, bool isFullScreen, bool isIdle, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(user, pid, name, description, fullscreen, idle, timestamp) VALUES(\"%S\", %d, \"%S\", \"%S\", \
						   %d, %d, %llu)", "activity", szUser, dwPid, szApp, szDescription, isFullScreen, isIdle, timestamp);

	enqueue(szStatement);
}

void CStore::InsertIo(IoDevice device, DWORD dwPid, TCHAR *szApp, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(device, pid, name, timestamp) VALUES(%d, %d, \"%S\", %llu)",
		"io", device, dwPid, szApp, timestamp);

	enqueue(szStatement);
}

void CStore::InsertPort(int pid, char *name, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, DWORD state, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(pid, name, protocol, srcip, destip, srcport, destport, state, timestamp)\
						   VALUES(%d, \"%s\", %d, \"%s\", \"%s\", %d, %d, %d, %llu)", "ports", pid, name, protocol,
						   srcIp, destIp, srcPort, destPort, state, timestamp);

	enqueue(szStatement);
}

void CStore::InsertProc(DWORD pid, char *name, int memory, double cpu, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(pid, name, memory, cpu, timestamp) VALUES(%d, \"%s\", %d, %.2f, %llu)",
		"procs", pid, name, memory, cpu, timestamp);

	enqueue(szStatement);
}

void CStore::InsertHttp(__int64 connstart, char *szVerb, char *szVerbParam, char *szStatusCode, char *szHost, char *szReferer, char *szContentType,
	char *szContentLength, int protocol, char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp)
{
	char szStatement[65536] = { 0 };
	sprintf_s(szStatement, "INSERT INTO %s(connstart, httpverb, httpverbparam, httpstatuscode, httphost, referer, contenttype, contentlength, \
						   protocol, srcip, destip, srcport, destport, timestamp) VALUES(%llu, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \
						   \"%s\", %d, \"%s\", \"%s\", %d, %d, %llu)", 
						   "http", connstart, szVerb, szVerbParam, szStatusCode, szHost, szReferer,
						   szContentType, szContentLength, protocol, srcIp, destIp, srcPort, destPort, timestamp);

	enqueue(szStatement);
}

void CStore::InsertDns(__int64 connstart,  int type, char *szIp, char *szHost, int protocol, 
	char *srcIp, char *destIp, int srcPort, int destPort, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(connstart, type, ip, host, protocol, srcip, destip, srcport, destport, timestamp) \
						   VALUES(%llu, %d, \"%s\", \"%s\", %d, \"%s\", \"%s\", %d, %d, %llu)", 
						   "dns", connstart, type, szIp, szHost, protocol, srcIp, destIp, srcPort, destPort, timestamp);

	enqueue(szStatement);
}

void CStore::InsertLoc(const char *szGuid, const char *szIp, const char *szRDNS, const char *szAsNumber, const char *szAsName, const char *szCountryCode, const char * szCity,
	const char *szLat, const char *szLon, __int64 timestamp)
{
	char szStatement[4096] = {0};
	sprintf_s(szStatement, "INSERT INTO %s(guid, public_ip, reverse_dns, asnumber, asname, countryCode, city, lat, lon, timestamp) \
						   VALUES(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %llu)", "location",
						   szGuid, szIp, szRDNS, szAsNumber, szAsName, szCountryCode, szCity, szLat, szLon, timestamp);

	enqueue(szStatement);
}

void CStore::InsertBrowser(const TCHAR *szBrowser, const TCHAR *szLocation, __int64 timestamp)
{
	char szStatement[4096] = { 0 };
	sprintf_s(szStatement, "INSERT INTO %s(browser, location, timestamp) VALUES(\"%S\", \"%S\", %llu)", 
		"browseractivity", szBrowser, szLocation, timestamp);

	enqueue(szStatement);
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
