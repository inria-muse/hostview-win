#ifndef __CAMERA_H_
#define __CAMERA_H_

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <vector>
#include <map>
#include <string>

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>


#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif

struct __declspec(uuid("6BDD1FC6-810F-11D0-BEC7-08002BE2092F")) GUID_DEVINTERFACE_IMAGE 
{
};

std::vector<std::tstring> GetWebcams()
{
	std::vector<std::tstring> webcams;

	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD i;

	// Create a HDEVINFO with all present devices.
	// {6BDD1FC6-810F-11D0-BEC7-08002BE2092F}
	GUID guidUsbDevClass = __uuidof(GUID_DEVINTERFACE_IMAGE);

	hDevInfo = SetupDiGetClassDevs(&guidUsbDevClass, 0, 0, DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		// Insert error handling here.
		return webcams;
	}

	// Enumerate through all devices in Set.

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i ++)
	{
		DWORD DataT;
		LPTSTR buffer = NULL;
		DWORD buffersize = 0;

		while (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
			&DataT, (PBYTE) buffer, buffersize, &buffersize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Change the buffer size.
				if (buffer) LocalFree(buffer);
				buffer = (LPTSTR) LocalAlloc(LPTR, buffersize * 2);
			}
			else
			{
				break;
			}
		}

		if (buffer)
		{
			if (_tcslen(buffer))
				webcams.push_back(buffer);
			LocalFree(buffer);
		}
	}


	if ( GetLastError() != NO_ERROR && GetLastError() != ERROR_NO_MORE_ITEMS )
	{
		// Insert error handling here.
		return webcams;
	}

	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return webcams;
}

#endif
