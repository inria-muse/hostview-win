#include "common.h"
#include <Olectl.h>
#include "Factory.h"

HINSTANCE hInstance = NULL;
volatile LONG nDllRefCount = 0;

const TCHAR *cszModuleName = _T("HostView BHO");
const CLSID CLSID_IEPlugin = { 0x3543619c, 0xd563, 0x43f7, { 0x95, 0xea, 0x4d, 0xa7, 0xe1, 0xcc, 0x39, 0x6a } };

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);

		TCHAR szExeName[1024] = {0};
		GetModuleFileName(NULL, szExeName, _countof(szExeName));

		size_t nLength = _tcsnlen(szExeName, _countof(szExeName));

		// avoid being loaded in other places
		if(nLength > 12 && !_tcsicmp(szExeName + nLength - 12, _T("explorer.exe")))
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Called by COM to get a reference to our CClassFactory object
STDAPI DllGetClassObject(REFIID rclsid,REFIID riid,LPVOID *ppv)
{
	if(!IsEqualCLSID(rclsid, CLSID_IEPlugin))
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	if(IsBadWritePtr(ppv, sizeof(LPVOID)))
	{
		return E_POINTER;
	}
	
	(*ppv) = NULL;

	CFactory *pFactory = new CFactory();
	if(pFactory == NULL)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pFactory->QueryInterface(riid, ppv);
	if(FAILED(hr))
	{
		delete pFactory;
	}
	return hr;
}

// This function is called by COM to determine if the DLL safe to unload.
// We return true if no objects from this DLL are being used and the DLL is unlocked.
STDAPI DllCanUnloadNow()
{
	return nDllRefCount > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer()
{
	TCHAR szDllPath[1024] = {0};
	GetModuleFileName(hInstance, szDllPath, _countof(szDllPath));

	HKEY hk = NULL;

	if(RegCreateKeyEx(HKEY_CLASSES_ROOT, REG_CLSID CLSID_IEPlugin_Str,
			0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return SELFREG_E_CLASS;
	}

	RegSetValueEx(hk, NULL, 0, REG_SZ, (const BYTE*) cszModuleName,
			(DWORD) (_tcslen(cszModuleName) + 1)* sizeof(TCHAR));
	RegCloseKey(hk);

	if(RegCreateKeyEx(HKEY_CLASSES_ROOT, REG_CLSID CLSID_IEPlugin_Str _T("\\InProcServer32"),
			0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return SELFREG_E_CLASS;
	}
	RegSetValueEx(hk, NULL, 0, REG_SZ, (const BYTE*) szDllPath, (DWORD) (_tcslen(szDllPath) + 1) * sizeof(TCHAR));
	RegSetValueEx(hk,_T("ThreadingModel"), 0, REG_SZ, (const BYTE*) _T("Apartment"), 10 * sizeof(TCHAR));
	RegCloseKey(hk);

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_IE_PATH CLSID_IEPlugin_Str,
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return SELFREG_E_CLASS;
	}

	DWORD n = 1;
	RegSetValueEx(hk, _T("NoExplorer"), 0, REG_DWORD, (const BYTE*)&n, sizeof(DWORD));
	RegCloseKey(hk);

	return S_OK;
}

STDAPI DllUnregisterServer()
{
	RegDeleteKey(HKEY_LOCAL_MACHINE, REG_IE_PATH CLSID_IEPlugin_Str);
	RegDeleteKey(HKEY_CLASSES_ROOT, REG_CLSID CLSID_IEPlugin_Str _T("\\InProcServer32"));
	RegDeleteKey(HKEY_CLASSES_ROOT, REG_CLSID CLSID_IEPlugin_Str);
	return S_OK;
}
