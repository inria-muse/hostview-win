#pragma once

#include <time.h>
#include <vector>
#include <string>
#include <Windows.h>
#include "sqlite3.h"

#if defined(STORELIBRARY_EXPORT) // inside DLL
#   define STOREAPI   __declspec(dllexport)
#else // outside DLL
#   define STOREAPI   __declspec(dllimport)
#endif  // STORELIBRARY_EXPORT

typedef std::vector<std::string> SQLRow;
typedef std::vector<SQLRow> SQLTable;

typedef enum IoDevice {Camera = 0, Keyboard = 1, Microphone = 2, Mouse = 3, Speaker = 4};

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

	const char* GetFile();

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

	size_t GetQueueSize();
	DWORD ExecThread();
private:
	sqlite3 *db;
	char szError[MAX_PATH];
	const char* error();
	void seterror(const char *error);

	bool open(const char *fn);
	void close();

	bool exec(const char *statement);
	SQLTable query(const char *statement);

	void enqueue(char *statement);
	std::vector<std::string> dequeue();
	HANDLE hExecThread;
	bool closing;
	CRITICAL_SECTION cs;
};

/**
 * Zips a file from source to dest.
 **/
extern "C" STOREAPI bool ZipFile(char *src, char *dest);

/**
 * Submits a file to the server with the given details.
 * Returns:
 * -1 in case of success but upload was slow
 *  0 in case of error
 *  1 in case of success and upload was fast
 **/
extern "C" STOREAPI int SubmitFile(char *server, char *userId, char *deviceId, char *fileName);

/*
 * Logs a message (used for meta-information about everything). Trace NULL to force a file submission.
 */
extern "C" STOREAPI void Trace(char *szFormat, ...);
