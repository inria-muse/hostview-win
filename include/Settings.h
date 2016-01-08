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
#include <Windows.h>

#if defined(SETTINGSLIBRARY_EXPORT) // inside DLL
#   define SETTINGSAPI   __declspec(dllexport)
#else // outside DLL
#   define SETTINGSAPI   __declspec(dllimport)
#endif  // SETTINGSLIBRARY_EXPORT

// settings keys constats
#define EndUser "enduser"
#define InterfaceMonitorTimeout "interfaceMonitorTimeout"
#define WirelessMonitorTimeout	"wirelessMonitorTimeout"
#define BatteryMonitorTimeout "batteryMonitorTimeout"
#define AutoSubmitInterval "autoSubmitInterval"
#define AutoUpdateInterval "autoUpdateInterval"
#define EsmCoinFlipInterval "esmCoinFlipInterval"
#define EsmCoinFlipMaximum "esmCoinFlipMaximum"
#define EsmCoinFlipRange "esmCoinFlipRange"
#define EsmMaxShows "esmMaxShows"
#define UserMonitorTimeout "userMonitorTimeout"
#define IoTimeout "ioTimeout"
#define UserIdleTimeout "userIdleTimeout"
#define SocketStatsTimeout "socketStatsTimeout"
#define SystemStatsTimeout "systemStatsTimeout"
#define PcapSizeLimit "pcapSizeLimit"
#define DbSizeLimit "dbSizeLimit"
#define SubmitServer "submitServer"
#define UpdateLocation "updateLocation"
#define QuestionnaireActive "questionnaireActive" 
#define NetLabellingActive "netLabellingActive"
#define UploadLowSpeedLimit "uploadLowSpeedLimit"
#define UploadLowSpeedTime "uploadLowSpeedTime"

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

