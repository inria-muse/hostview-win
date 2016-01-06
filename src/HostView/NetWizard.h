#pragma once

#include "HtmlDialog.h"
#include "KnownNetworks.h"

class CNetWizard : public CHtmlDialog
{
public:
	CNetWizard(CWnd* pParent = NULL) : CHtmlDialog(CNetWizard::IDD, CNetWizard::IDH, pParent) {};
	virtual ~CNetWizard(void) {};

	void SetNetwork(const NetworkInterface &ni);

	enum { IDD = IDD_HOSTVIEW_DIALOG, IDH = 0 };

	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

	HRESULT OnClickOK(IHTMLElement *pElement);

private:
	NetworkInterface m_ni;
};

