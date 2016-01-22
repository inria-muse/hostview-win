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

#define SettingsVersion "version"                          // settings version

#define AutoRestartTimeout "autoRestartTimeout"            // restart hostview after pause (ms)

#define WirelessMonitorTimeout	"wirelessMonitorTimeout"   // check wireless status (ms)
#define BatteryMonitorTimeout "batteryMonitorTimeout"      // check battery status (ms)
#define SocketStatsTimeout "socketStatsTimeout"            // check socket table (ms)             
#define SystemStatsTimeout "systemStatsTimeout"            // check performance stats (ms)
#define IoTimeout "ioTimeout"                              // check io activity (ms)
#define UserMonitorTimeout "userMonitorTimeout"            // check user activity (ms)
#define UserIdleTimeout "userIdleTimeout"                  // definition of user idle (ms)

#define NetLabellingActive "netLabellingActive"      // boolean
#define NetLocationActive "netLocationActive"        // boolean

#define QuestionnaireActive "questionnaireActive"    // boolean
#define EsmCoinFlipInterval "esmCoinFlipInterval"    // flip coin every x (ms)
#define EsmCoinFlipProb "esmCoinFlipProb"            // coin flip success probability [0-100]
#define EsmMaxShows "esmMaxShows"                    // max num of questionnaires per day
#define EsmStartHour "esmStartHour"                  // check between startHour
#define EsmStopHour "esmStopHour"                    // and stopHour only

#define PcapSizeLimit "pcapSizeLimit"                // max pcap filesize (bytes)

#define SubmitServer "submitServer"                       // submit url
#define AutoSubmitInterval "autoSubmitInterval"           // submit new data (ms)
#define AutoSubmitRetryCount "autoSubmitRetryCount"
#define AutoSubmitRetryInterval "autoSubmitRetryInterval" // (ms)

#define UploadLowSpeedLimit "curlUploadLowSpeedLimit" // curl config (bytes/s)
#define UploadLowSpeedTime "curlUploadLowSpeedTime"   // curl config (seconds)
#define UploadMaxSendSpeed "curlUploadMaxSendSpeed"   // curl config (bytes/s)
#define UploadVerifyPeer "curlUploadVerifyPeer"       // curl config (bytes/s)

#define AutoUpdateInterval "autoUpdateInterval"   // check for updates (ms)
#define UpdateLocation "updateLocation"           // update url

class SETTINGSAPI CSettings
{
public:
	/** Create from default settings. */
	CSettings(void);

	/** Create from given file (check updates). */
	CSettings(char * file);

	~CSettings(void);

	/** Current settings version. */
	ULONG GetVersion() { return version; };

	bool HasKey(char *szKey);

	ULONG GetULong(char *szKey);

	char *GetString(char *szKey);

	bool GetBoolean(char *szKey);

	bool ReloadSettings();

private:
	std::hash_map<std::string, ULONG> *mLongSettings;
	std::hash_map<std::string, std::string> *mSettings;
	char filename[MAX_PATH];
	ULONG version;
};