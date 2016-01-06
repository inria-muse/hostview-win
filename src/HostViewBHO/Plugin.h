#ifndef __OBJECTWITHSITE_H__
#define __OBJECTWITHSITE_H__

#include <Ocidl.h>
#include <shlguid.h>
#include <exdispid.h>
#include "Unknown.h"
#include "EventSink.h"

class CBHOPlugin : public CUnknown<IObjectWithSite>
{
public:
	// Constructor and destructor
	CBHOPlugin();
	virtual ~CBHOPlugin();

	// IObjectWithSite methods
	STDMETHODIMP SetSite(IUnknown *pUnkSite);
	STDMETHODIMP GetSite(REFIID riid,void **ppvSite);

protected:
	void ConnectEventSink();
	void DisconnectEventSink();

	DWORD m_dwAdviseCookie;
	IConnectionPoint *m_pCP;
	IWebBrowser2 *m_pWebBrowser;
	static const IID SupportedIIDs[2];
};

// We only have one global object of this
extern CEventSink eventSink;

#endif // __OBJECTWITHSITE_H__
