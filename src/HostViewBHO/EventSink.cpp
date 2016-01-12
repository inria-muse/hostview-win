#include "common.h"
#include "EventSink.h"
#include "http_server.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <wininet.h>

// The single global object of CEventSink
CEventSink eventSink;

CEventSink::CEventSink()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
}

void CEventSink::SetBrowser(IWebBrowser2 *pWebBrowser)
{
	// TODO: why set site occurs multiple times?
	if (browsers.find(GetCurrentProcessId()) == browsers.end())
	{
		browsers[GetCurrentProcessId()] = pWebBrowser;
	}
}

IWebBrowser2 *CEventSink::GetBrowser()
{
	return browsers[GetCurrentProcessId()];
}

STDMETHODIMP CEventSink::QueryInterface(REFIID riid, void **ppvObject)
{
	if(IsBadWritePtr(ppvObject, sizeof(void*)))
	{
		return E_POINTER;
	}

	(*ppvObject) = NULL;

	if(!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IDispatch)
		&& !IsEqualIID(riid, DIID_DWebBrowserEvents2))
	{
		return E_NOINTERFACE;
	}

	(*ppvObject) = (void*) &eventSink;
	return S_OK;
}

STDMETHODIMP_(ULONG) CEventSink::AddRef()
{
	// one instance
	return 1;
}

STDMETHODIMP_(ULONG) CEventSink::Release()
{
	// one instance
	return 1;
}

STDMETHODIMP CEventSink::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	UNREFERENCED_PARAMETER(lcid);
	UNREFERENCED_PARAMETER(wFlags);
	UNREFERENCED_PARAMETER(pVarResult);
	UNREFERENCED_PARAMETER(pExcepInfo);
	UNREFERENCED_PARAMETER(puArgErr);
	UNREFERENCED_PARAMETER(pDispParams);

	if(!IsEqualIID(riid, IID_NULL))
	{
		// riid should always be IID_NULL
		return DISP_E_UNKNOWNINTERFACE;
	}

	if (dispIdMember == DISPID_COMMANDSTATECHANGE
		|| dispIdMember == DISPID_BEFORENAVIGATE2
		|| dispIdMember == DISPID_DOCUMENTCOMPLETE)
	{
		OnTabChanged();
	}
	return S_OK;
}

void CEventSink::TrimUrl(TCHAR *szUrl)
{
	if (_tcsncmp(szUrl, L"https://", _tcslen(L"https://")) == 0)
	{
		_tcscpy_s(szUrl, _tcslen(szUrl), szUrl + _tcslen(L"https://"));
	}

	if (_tcsncmp(szUrl, L"http://", _tcslen(L"http://")) == 0)
	{
		_tcscpy_s(szUrl, _tcslen(szUrl), szUrl + _tcslen(L"http://"));
	}

	for (size_t i = 0; i < _tcslen(szUrl); i ++)
	{
		if (szUrl[i] == '?' || szUrl[i] == '/')
		{
			szUrl[i] = 0;
			break;
		}
	}
}

void CEventSink::OnUrlChanged(BSTR strLocation)
{
	TCHAR szLocation[4024] = {0};
	_tcscpy_s(szLocation, strLocation);

	if (_tcscmp(szLocation, L"about:Tabs") != 0
		&& _tcscmp(szLocation, L"about:blank") != 0
		&& _tcslen(szLocation) > 0)
	{
		TrimUrl(szLocation);

		ParamsT params;			
		params.insert(std::make_pair("browser", L"iexplore"));
		params.insert(std::make_pair("location", szLocation));

		SendHttpMessage(params);
	}
}

void CEventSink::OnTabChanged()
{
	IServiceProvider* pServiceProvider = NULL;
	
	if (SUCCEEDED(GetBrowser()->QueryInterface(IID_IServiceProvider, (void**) &pServiceProvider)))
	{
		IOleWindow* pWindow = NULL;
		if (SUCCEEDED(pServiceProvider->QueryService(SID_SShellBrowser, IID_IOleWindow, (void**)&pWindow)))
		{
			HWND hwndBrowser = NULL;
			if (SUCCEEDED(pWindow->GetWindow(&hwndBrowser)))
			{
				// hwndBrowser is the handle of TabWindowClass
				if (IsWindow((HWND) hwndBrowser) && IsWindowVisible((HWND) hwndBrowser))
				{
					BSTR strLocation = NULL;
					GetBrowser()->get_LocationURL(&strLocation);

					OnUrlChanged(strLocation);
				}
			}

			pWindow->Release();
		}
		pServiceProvider->Release();
	}
}
