#ifndef __COMMON_H__
#define __COMMON_H__

#define _WIN32_WINNT 0x0600

#define _WINSOCKAPI_  
#include <windows.h>
#include <tchar.h>
#include <string>
#include <map>

#define CLSID_IEPlugin_Str _T("{3543619C-D563-43f7-95EA-4DA7E1CC396A}")

#define REG_CLSID _T("CLSID\\")
#define REG_IE_PATH _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\")

extern const CLSID CLSID_IEPlugin;
extern volatile LONG nDllRefCount;
extern HINSTANCE hInstance;

#endif // __COMMON_H__
