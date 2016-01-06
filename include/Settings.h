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
};

