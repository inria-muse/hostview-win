#pragma once

#include "HtmlDialog.h"
#include "questionnaire.h"

class CEsmWizard : public CHtmlDialog
{
public:
	CEsmWizard(CWnd* pParent = NULL) : CHtmlDialog(CEsmWizard::IDD, CEsmWizard::IDH, pParent)/*, m_currPage(0)*/ {};
	virtual ~CEsmWizard(void) {};

	void SetApps(AppListT &apps, AppListT &installedApps) { m_apps = apps; m_installedApps = installedApps; }

	enum { IDD = IDD_HOSTVIEW_DIALOG, IDH = 0 };

	BOOL OnInitDialog();

	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);

	HRESULT OnClickDone(IHTMLElement *pElement);

	void LoadTags(TCHAR *szCategory, TCHAR *szCtlId);
private:
	AppListT m_apps;
	AppListT m_installedApps;
	DWORD m_dwStart;
	int m_appCount;
};

