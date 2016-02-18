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

#define HAVE_REMOTE

#include <winnt.h>
#include <winsock2.h>

#include <vector>
#include <map>
#include <set>
#include <string>

#include <pcap/pcap.h>

#if defined(PCAPLIBRARY_EXPORT) // inside DLL
#   define PCAPAPI   __declspec(dllexport)
#else // outside DLL
#   define PCAPAPI   __declspec(dllimport)
#endif  // PCAPLIBRARY_EXPORT

#define LINE_LEN 16

#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif

#define DATA_DIRECTORY ".\\pcap"

/**
 *	Callback Interface.
 *	Through this we can save date into tables, compute statistics, etc.
 **/
class PCAPAPI CCaptureCallback
{
public:
	virtual ~CCaptureCallback() {};

	virtual void OnHttpHeaders(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, char *szVerb, char *szVerbParam,
		char *szStatusCode, char *szHost, char *szReferer, char *szContentType, char *szContentLength) = 0;
	virtual void OnDNSAnswer(ULONGLONG connection, int protocol, char *szSrc, u_short srcport, char *szDest, u_short destport, int type, char *szIp, char *szHost) = 0;
};

extern "C" PCAPAPI bool StartCapture(CCaptureCallback &callback, ULONGLONG session, ULONGLONG connection, ULONG maxPcapSize, bool debugMode, const char *adapterId = "interactive");
extern "C" PCAPAPI bool StopCapture(const char *adapterId);

/**
 * NetInterf with custom details.
 **/
struct NetworkInterface
{
	std::string strGuid;
	std::tstring strFriendlyName;
	std::tstring strDescription;
	std::tstring strMac;
	std::tstring strDnsSuffix;
	std::vector<std::tstring> ips;
	std::vector<std::tstring> gateways;
	std::vector<std::tstring> dnses;

	unsigned __int64 tSpeed;
	unsigned __int64 rSpeed;

	bool wireless;

	// wlan details
	std::tstring strProfile;
	std::string strSSID;
	std::tstring strBSSID;
	std::string strBSSIDType;
	std::string strPHYType;
	ULONG phyIndex;
	ULONG channel;

	NetworkInterface()
		: wireless(false),
		tSpeed(0),
		rSpeed(0),
		channel(0),
		phyIndex(0)
	{
	}
	
	bool operator<(const NetworkInterface& other) const
	{
		// TODO: should be more complex;
		int compare = strGuid.compare(other.strGuid);
		if (compare == 0)
		{
			if (gateways.size() < other.gateways.size())
			{
				compare = -1;
			}
			else if (gateways.size() > other.gateways.size())
			{
				compare = 1;
			}
			else
			{
				for (size_t i = 0; i < gateways.size(); i ++)
				{
					if (compare = gateways[i].compare(other.gateways[i]))
					{
						break;
					}
				}
			}
		}
		return compare > 0;
	}
};

/**
 * Handles network interfaces changes.
 **/
class CInterfacesCallback
{
public:
	virtual void OnInterfaceConnected(const NetworkInterface& networkInterface) = 0;
	virtual void OnInterfaceDisconnected(const NetworkInterface& networkInterface) = 0;
};

extern "C" PCAPAPI bool StartInterfacesMonitor(CInterfacesCallback &callback, unsigned long pcapSizeLimit, bool debugMode);
extern "C" PCAPAPI bool StopInterfacesMonitor(CInterfacesCallback &callback);


/**
 * Handles wireless continuous monitoring.
 **/
class CWifiMonitor
{
public:
	virtual void OnWifiStats(const char *szGuid, unsigned __int64 tSpeed, unsigned __int64 rSpeed,
		ULONG signal, ULONG rssi, short state) = 0;
};

extern "C" PCAPAPI bool StartWifiMonitor(CWifiMonitor &callback, unsigned long wifiMonitorTimeout);
extern "C" PCAPAPI bool StopWifiMonitor();
