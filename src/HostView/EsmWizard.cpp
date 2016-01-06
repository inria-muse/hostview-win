#include "StdAfx.h"
#include "EsmWizard.h"

BEGIN_DHTML_EVENT_MAP(CEsmWizard)
	DHTML_EVENT_ONCLICK(_T("ButtonClose"), OnClickClose)
	DHTML_EVENT_ONCLICK(_T("ButtonDone"), OnClickDone)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, OnHtmlMouseDown)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONDRAGSTART, OnHtmlMouseDown)
END_DHTML_EVENT_MAP()

BEGIN_MESSAGE_MAP(CEsmWizard, CHtmlDialog)
END_MESSAGE_MAP()

void CEsmWizard::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

BOOL CEsmWizard::OnInitDialog()
{
	__super::OnInitDialog();

	TCHAR szText[MAX_PATH] = {0};
	LoadString(NULL, IDS_ESM_TITLE, szText, _countof(szText));

	SetWindowText(szText);

	TCHAR szDirectory[MAX_PATH] = {0};
	GetModuleFileName(NULL, szDirectory, _countof(szDirectory));
	PathRemoveFileSpec(szDirectory);
	PathAppend(szDirectory, _T("html"));
	PathAppend(szDirectory, _T("esm_wizard.html"));


	TCHAR szUrl[MAX_PATH] = {0};
	DWORD dwSize = _countof(szUrl);
	UrlCreateFromPath(szDirectory, szUrl, &dwSize, 0);

	Navigate(szUrl);
	return FALSE;
}

void CEsmWizard::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	__super::OnDocumentComplete(pDisp, szUrl);

	static bool bFirstTime = true;

	if (bFirstTime && _tcsstr(szUrl, _T(".html")))
	{
		bFirstTime = false;

		int checkIdx = 0;
		CallClientScript(_T("ClearContainers"), 0, 0);

		for (AppListT::iterator it = m_apps.begin(); it != m_apps.end(); it ++)
		{
			TCHAR szId[MAX_PATH] = {0};
			_tcscpy_s(szId, it->app.c_str());

			// replace space with underscore; TODO: improove this
			for (size_t i = 0; i < _tcslen(szId); i ++)
			{
				// some sort of formatting?
				if (szId[i] == ' ' || szId[i] == '"')
				{
					szId[i] = '_';
				}
			}

			CStringArray params;
			params.Add(szId);
			params.Add(it->app.c_str());
			params.Add(it->description.c_str());
			params.Add(it->extra.c_str());
			CallClientScript(_T("AppendAppCategories"), &params, 0);
			CallClientScript(_T("AppendAppIssues"), &params, 0);
			checkIdx ++;
		}

		for (AppListT::iterator it = m_installedApps.begin(); it != m_installedApps.end(); it ++)
		{
			CStringArray params;
			params.Add(it->app.c_str());
			params.Add(it->description.c_str());
			CallClientScript(_T("AppendInstalledApp"), &params, 0);
		}

		TCHAR szText[MAX_PATH] = {0};
		TCHAR szAppName[MAX_PATH] = {0};

		TCHAR szTitle[MAX_PATH] = {0};

		LoadString(NULL, IDS_ESM_TITLE, szText, _countof(szText));
		LoadString(NULL, IDS_APP_TITLE, szAppName, _countof(szAppName));

		_stprintf_s(szTitle, _T("%s - %s"), szAppName, szText);

		SetElementText(_T("WindowTitle"), szTitle);

		CallClientScript(_T("SelectIssuesFirstTab"), 0, 0);
		CallClientScript(_T("SelectCategoriesFirstTab"), 0, 0);

		LoadTags(AppIssue, _T("issues"));
		LoadTags(AppPCIssue, _T("pcissues"));
		LoadTags(AppCategory, _T("categories"));

		// user sees it
		m_dwStart = GetTickCount();
	}
}

void CEsmWizard::LoadTags(TCHAR *szCategory, TCHAR *szCtlId)
{
	std::tstring tags = GetTags(szCategory);

	CStringArray vArgs;
	vArgs.Add(szCtlId);
	vArgs.Add(tags.c_str());

	CallClientScript(_T("AppendLabel"), &vArgs, 0);
}

HRESULT CEsmWizard::OnClickDone(IHTMLElement *pElement)
{
	CComVariant varRes;
	CStringArray arrArgs;

	TCHAR szDuration[64] = {0};
	_stprintf_s(szDuration, _T("%ul"), GetTickCount() - m_dwStart);
	arrArgs.Add(szDuration);

	if(CallClientScript(L"BuildResult", &arrArgs, &varRes))
	{
		if(varRes.vt == VT_BSTR)
		{
			SubmitQuestionnaire(varRes.bstrVal);

			CComVariant vaCat, vaIss, vaPCIss;
			CallClientScript(L"GetNewCategories", &arrArgs, &vaCat);
			CallClientScript(L"GetNewPCIssues", &arrArgs, &vaPCIss);
			CallClientScript(L"GetNewIssues", &arrArgs, &vaIss);

			SetTags(AppCategory, vaCat.bstrVal);
			SetTags(AppIssue, vaIss.bstrVal);
			SetTags(AppPCIssue, vaPCIss.bstrVal);
		}
	}

	return __super::OnClickClose(pElement);
}

