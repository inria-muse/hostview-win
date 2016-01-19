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
#include <hash_map>

#if defined(SETTINGSLIBRARY_EXPORT) // inside DLL
#   define SETTINGSAPI   __declspec(dllexport)
#else // outside DLL
#   define SETTINGSAPI   __declspec(dllimport)
#endif  // SETTINGSLIBRARY_EXPORT

typedef std::hash_map<std::string, std::string> SettingsMap;

// settings keys constats

#define AutoRestartTimeout "autoRestartTimeout"            // restart hostview after pause (ms)

#define InterfaceMonitorTimeout "interfaceMonitorTimeout"  // check network interface status (ms)
#define WirelessMonitorTimeout	"wirelessMonitorTimeout"   // check wireless status (ms)
#define BatteryMonitorTimeout "batteryMonitorTimeout"      // check battery status (ms)
#define UserMonitorTimeout "userMonitorTimeout"            // check user activity (ms)
#define IoTimeout "ioTimeout"                              // check io activity (ms)
#define UserIdleTimeout "userIdleTimeout"                  // definition of user idle (ms)
#define SocketStatsTimeout "socketStatsTimeout"            // check socket table (ms)             
#define SystemStatsTimeout "systemStatsTimeout"            // check performance stats (ms)

#define EsmCoinFlipInterval "esmCoinFlipInterval"    // flip coin every x (ms)
#define EsmCoinFlipProb "esmCoinFlipProb"            // coin flip success probability [0-100]
#define EsmMaxShows "esmMaxShows"                    // max num of questionnaires per day

#define PcapSizeLimit "pcapSizeLimit"             // max pcap filesize (bytes)
#define DbSizeLimit "dbSizeLimit"                 // max db filesize (bytes)

#define AutoSubmitRetryCount "autoSubmitRetryCount"
#define AutoSubmitRetryInterval "autoSubmitRetryInterval" // (ms)
#define AutoSubmitInterval "autoSubmitInterval"   // submit new data (ms)
#define SubmitServer "submitServer"               // submit url
#define UploadLowSpeedLimit "uploadLowSpeedLimit" // curl config (bytes/s)
#define UploadLowSpeedTime "uploadLowSpeedTime"   // curl config (seconds)

#define AutoUpdateInterval "autoUpdateInterval"   // check for updates (ms)
#define UpdateLocation "updateLocation"           // update url

#define QuestionnaireActive "questionnaireActive" // boolean
#define NetLabellingActive "netLabellingActive"   // boolean

class SETTINGSAPI CSettings
{
public:
	CSettings(void);
	~CSettings(void);

	unsigned long GetULong(char *szKey);
	char *GetString(char *szKey);

	bool LoadSettings();
private:
	void trim(char *s);
	bool load();
};

