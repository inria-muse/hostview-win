#pragma once

#include <map>
#include <Exdisp.h>
#include <shlguid.h>
#include <Exdispid.h>

class CEventSink : public DWebBrowserEvents2
{
public:
	CEventSink();

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	
	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT *) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **) { return E_NOTIMPL; }
	STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR *, UINT, LCID, DISPID *) { return E_NOTIMPL; }

	// called upon events
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
		VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

	void OnTabChanged();
	void OnUrlChanged(BSTR strLocation);

	void TrimUrl(TCHAR *szUrl);

	void SetBrowser(IWebBrowser2 *pWebBrowser);
	IWebBrowser2 * GetBrowser();

protected:
	std::map<DWORD, IWebBrowser2 *> browsers;
};

