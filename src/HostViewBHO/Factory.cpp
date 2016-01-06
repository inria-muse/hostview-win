#include "common.h"
#include "Factory.h"
#include "Plugin.h"

const IID CFactory::SupportedIIDs[] = {IID_IUnknown, IID_IClassFactory};

CFactory::CFactory()
	: CUnknown<IClassFactory>(SupportedIIDs, 2)
{
}

CFactory::~CFactory()
{
}

STDMETHODIMP CFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	if(pUnkOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	if(IsBadWritePtr(ppvObject, sizeof(void*)))
	{
		return E_POINTER;
	}

	(*ppvObject) = NULL;

	CBHOPlugin* pObject = new CBHOPlugin();
	if(pObject == NULL)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pObject->QueryInterface(riid, ppvObject);
	if(FAILED(hr))
	{
		delete pObject;
	}
	return hr;
}

STDMETHODIMP CFactory::LockServer(BOOL fLock)
{
	if(fLock)
	{
		InterlockedIncrement(&nDllRefCount);
	}
	else
	{
		InterlockedDecrement(&nDllRefCount);
	}
	return S_OK;
}
