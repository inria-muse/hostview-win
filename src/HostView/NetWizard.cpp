#include "StdAfx.h"
#include "NetWizard.h"

BEGIN_DHTML_EVENT_MAP(CNetWizard)
	DHTML_EVENT_ONCLICK(_T("ButtonClose"), OnClickClose)
	DHTML_EVENT_ONCLICK(_T("ButtonOK"), OnClickOK)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, OnHtmlMouseDown)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONDRAGSTART, OnHtmlMouseDown)
END_DHTML_EVENT_MAP()

BEGIN_MESSAGE_MAP(CNetWizard, CHtmlDialog)
END_MESSAGE_MAP()

void CNetWizard::DoDataExchange(CDataExchange* pDX)
{
	CHtmlDialog::DoDataExchange(pDX);
}

BOOL CNetWizard::OnInitDialog()
{
	CHtmlDialog::OnInitDialog();

	TCHAR szText[MAX_PATH] = {0};
	LoadString(NULL, IDS_NET_LABEL_TITLE, szText, _countof(szText));
	SetWindowText(szText);


	TCHAR szDirectory[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDirectory, _countof(szDirectory));
	PathRemoveFileSpec(szDirectory);
	PathAppend(szDirectory, _T("html"));
	PathAppend(szDirectory, _T("network_wizard.html"));


	TCHAR szUrl[MAX_PATH] = {0};
	DWORD dwSize = _countof(szUrl);
	UrlCreateFromPath(szDirectory, szUrl, &dwSize, 0);

	Navigate(szUrl);

	return FALSE;
}

void CNetWizard::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	__super::OnDocumentComplete(pDisp, szUrl);

	TCHAR szText[MAX_PATH] = {0};
	TCHAR szAppName[MAX_PATH] = {0};

	TCHAR szTitle[MAX_PATH] = {0};

	LoadString(NULL, IDS_NET_LABEL_TITLE, szText, _countof(szText));
	LoadString(NULL, IDS_APP_TITLE, szAppName, _countof(szAppName));

	_stprintf_s(szTitle, _T("%s - %s"), szAppName, szText);

	SetElementText(_T("WindowTitle"), szTitle);
	
	TCHAR szNetwork[MAX_PATH] = {0};

	if (m_ni.strSSID.length() > 0)
	{
		_stprintf_s(szNetwork, _T("%S - %s"), m_ni.strSSID.c_str(),
			m_ni.strFriendlyName.c_str());
	}
	else
	{
		_stprintf_s(szNetwork, _T("%s"), m_ni.strFriendlyName.c_str());
	}

	SetElementText(_T("NetworkName"), szNetwork);

	std::tstring tags = GetTags(NetworkCategory);
	CStringArray params;
	params.InsertAt(0, tags.c_str());
	CallClientScript(L"SetUserLabels", &params, 0);
}

HRESULT CNetWizard::OnClickOK(IHTMLElement *pElement)
{
	CComVariant vaRes;
	CStringArray arrArgs;

	TCHAR szProfile[MAX_PATH] = {0};

	CallClientScript(L"GetResult", &arrArgs, &vaRes);
	_tcscpy_s(szProfile, vaRes.bstrVal);

	if (_tcslen(szProfile) == 0)
	{
		CallClientScript(L"ShowModal", 0, 0);
	}
	else
	{
		CallClientScript(L"GetUserLabels", &arrArgs, &vaRes);
		SetTags(NetworkCategory, vaRes.bstrVal);

		LabelNetwork(m_ni, szProfile);
		EndDialog(IDOK);
	}
	return S_OK;
}

void CNetWizard::SetNetwork(const NetworkInterface &ni)
{
	m_ni = ni;
}
