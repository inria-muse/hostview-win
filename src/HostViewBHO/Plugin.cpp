#include "common.h"
#include "Plugin.h"
#include "EventSink.h"

const IID CBHOPlugin::SupportedIIDs[]={IID_IUnknown, IID_IObjectWithSite};

CBHOPlugin::CBHOPlugin()
	: CUnknown<IObjectWithSite>(SupportedIIDs, 2),
	m_dwAdviseCookie(0),
	m_pWebBrowser(NULL),
	m_pCP(NULL)
{
}

CBHOPlugin::~CBHOPlugin()
{
	DisconnectEventSink();
}

STDMETHODIMP CBHOPlugin::SetSite(IUnknown *pUnkSite)
{
	if(pUnkSite)
	{
		pUnkSite->AddRef();
	}

	DisconnectEventSink();

	if(pUnkSite == NULL)
	{
		return S_OK;
	}

	HRESULT hr = pUnkSite->QueryInterface(IID_IWebBrowser2, (void**) &m_pWebBrowser);

	pUnkSite->Release();

	if(FAILED(hr))
	{
		return hr;
	}

	ConnectEventSink();
	return S_OK;
}

STDMETHODIMP CBHOPlugin::GetSite(REFIID riid, void **ppvSite)
{
	if(IsBadWritePtr(ppvSite, sizeof(void*)))
	{
		return E_POINTER;
	}

	(*ppvSite) = NULL;

	if(m_pWebBrowser == NULL)
	{
		return E_FAIL;
	}

	return m_pWebBrowser->QueryInterface(riid, ppvSite);
}

void CBHOPlugin::ConnectEventSink()
{
	if(m_pWebBrowser == NULL)
	{
		return;
	}

	IConnectionPointContainer* pCPC = 0;
	if(FAILED(m_pWebBrowser->QueryInterface(IID_IConnectionPointContainer,
		(void**)&pCPC)))
	{
		return;
	}

	if(FAILED(pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &m_pCP)))
	{
		pCPC->Release();
		return;
	}

	m_pCP->Advise((IUnknown*)&eventSink, &m_dwAdviseCookie);
	eventSink.SetBrowser(m_pWebBrowser);
}

void CBHOPlugin::DisconnectEventSink()
{
	if(m_pCP)
	{
		m_pCP->Unadvise(m_dwAdviseCookie);
		m_dwAdviseCookie = 0;
		m_pCP->Release();
		m_pCP = NULL;
	}

	if(m_pWebBrowser)
	{
		m_pWebBrowser->Release();
		m_pWebBrowser = NULL;
	}
}
