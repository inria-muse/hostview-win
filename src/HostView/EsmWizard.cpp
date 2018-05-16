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
		TCHAR szId[MAX_PATH] = { 0 };

		bFirstTime = false;
		m_appCount = 0;

		CallClientScript(_T("ClearContainers"), 0, 0);

		for (AppListT::iterator it = m_apps.begin(); it != m_apps.end(); it ++)
		{
			CStringArray params;

			_stprintf_s(szId, _T("%d"), m_appCount);
			params.Add(szId);
			params.Add(it->app.c_str());
			params.Add(it->description.c_str());
			params.Add(it->extra.c_str());
			CallClientScript(_T("AppendApp"), &params, 0);
			m_appCount++;
		}

		for (AppListT::iterator it = m_installedApps.begin(); it != m_installedApps.end(); it ++)
		{
			CStringArray params;
			_stprintf_s(szId, _T("%d"), m_appCount);
			params.Add(szId);
			params.Add(it->app.c_str());
			params.Add(it->description.c_str());
			CallClientScript(_T("AppendInstalledApp"), &params, 0);
			m_appCount++;
		}

		TCHAR szText[MAX_PATH] = {0};
		TCHAR szAppName[MAX_PATH] = {0};
		TCHAR szTitle[MAX_PATH] = {0};

		LoadString(NULL, IDS_ESM_TITLE, szText, _countof(szText));
		LoadString(NULL, IDS_APP_TITLE, szAppName, _countof(szAppName));

		_stprintf_s(szTitle, _T("%s - %s"), szAppName, szText);

		SetElementText(_T("WindowTitle"), szTitle);

		CallClientScript(_T("SelectFirst"), 0, 0);

		LoadTags(AppIssue, _T("issues"));
		LoadTags(AppPCIssue, _T("pcissues"));
		LoadTags(AppCategory, _T("appusefor"));
		LoadTags(AppImportance, _T("appimportance"));
		LoadTags(AppPerformance, _T("appperformance"));
		LoadTags(AppPCPerformance, _T("pcperformance"));

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
	DWORD dur = GetTickCount() - m_dwStart;

	CComVariant varRes, varResC, varResPcPer;
	CStringArray arrArgs;
	if (CallClientScript(L"GetQoeScore", &arrArgs, &varRes))
	{
		// Create questionnaire
		SubmitQuestionnaire(dur, varRes.bstrVal);

		// Add activity + prob tags for all apps (pass appId as arg)
		for (int appId = 0; appId < m_appCount; appId++) {
			TCHAR szId[8] = { 0 };
			CComVariant varResA, varResP, varResI, varResPer;
			CStringArray arrArgsA;
			_stprintf_s(szId, _T("%d"), appId);
			arrArgsA.Add(szId);

			varResA.bstrVal = NULL;
			if (CallClientScript(L"GetActivityTags", &arrArgsA, &varResA) && _tcslen(varResA.bstrVal)>1)
			{
				SubmitQuestionnaireActivity(varResA.bstrVal);
			}

			varResP.bstrVal = NULL;
			if (CallClientScript(L"GetProblemTags", &arrArgsA, &varResP) && _tcslen(varResP.bstrVal)>1)
			{
				SubmitQuestionnaireProblem(varResP.bstrVal);
			}

			varResI.bstrVal = NULL;
			if (CallClientScript(L"GetAppImportanceTags", &arrArgsA, &varResI) && _tcslen(varResI.bstrVal)>1)
			{
				SubmitQuestionnaireAppImportance(varResI.bstrVal);
			}

			varResPer.bstrVal = NULL;
			if (CallClientScript(L"GetAppPerformanceTags", &arrArgsA, &varResPer) && _tcslen(varResPer.bstrVal)>1)
			{
				SubmitQuestionnaireAppPerformance(varResPer.bstrVal);
			}
			
		}

		// will return the computer wide prob tags (no appId arg)
		varResC.bstrVal = NULL;
		if (CallClientScript(L"GetProblemTags", NULL, &varResC) && _tcslen(varResC.bstrVal)>1)
		{
			SubmitQuestionnaireProblem(varResC.bstrVal);
		}

		varResPcPer.bstrVal = NULL;
		if (CallClientScript(L"GetPCPerformanceTags", &arrArgs, &varResPcPer) && _tcslen(varResPcPer.bstrVal)>1)
		{
			SubmitQuestionnaireAppPerformance(varResPcPer.bstrVal);
		}

		// Signal that we're done
		SubmitQuestionnaireDone();

		// Save new user tags
		CComVariant vaCat, vaIss, vaPCIss, vaAppImp, vaAppPer, vaPCPer;
		CallClientScript(L"GetNewCategories", &arrArgs, &vaCat);
		CallClientScript(L"GetNewPCIssues", &arrArgs, &vaPCIss);
		CallClientScript(L"GetNewIssues", &arrArgs, &vaIss);
		CallClientScript(L"GetAppImportance", &arrArgs, &vaAppImp);
		CallClientScript(L"GetAppPerformance", &arrArgs, &vaAppPer);
		CallClientScript(L"GetPCPerformance", &arrArgs, &vaPCPer);


		SetTags(AppCategory, vaCat.bstrVal);
		SetTags(AppIssue, vaIss.bstrVal);
		SetTags(AppPCIssue, vaPCIss.bstrVal);
		SetTags(AppImportance, vaAppImp.bstrVal);
		SetTags(AppPerformance, vaAppPer.bstrVal);
		SetTags(AppPCPerformance, vaPCPer.bstrVal);
	}

	m_appCount = 0;
	m_dwStart = 0;

	return __super::OnClickClose(pElement);
}

