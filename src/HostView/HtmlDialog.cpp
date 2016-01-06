#include "StdAfx.h"
#include "HtmlDialog.h"
#include "afxdialogex.h"

BEGIN_DHTML_EVENT_MAP(CHtmlDialog)
	DHTML_EVENT_ONCLICK(_T("ButtonClose"), OnClickClose)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, OnHtmlMouseDown)
	DHTML_EVENT_TAG_ALL(DISPID_HTMLELEMENTEVENTS_ONDRAGSTART, OnHtmlMouseDown)
END_DHTML_EVENT_MAP()

extern HWND g_hWnd;

CHtmlDialog::CHtmlDialog(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(CHtmlDialog::IDD, CHtmlDialog::IDH, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(MAKEINTRESOURCE(IDI_HOSTVIEW));
	m_bVisible = FALSE;
}

void CHtmlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHtmlDialog, CDHtmlDialog)
END_MESSAGE_MAP()

void CHtmlDialog::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	//allow to hide dialog at the startup of dialog,
	//delay the show of dialog until m_bVisible is set
	if(!m_bVisible)
	{
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
	}

	CDHtmlDialog::OnWindowPosChanging(lpwndpos);
}

BOOL CHtmlDialog::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();

	g_hWnd = GetSafeHwnd();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LONG lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_BORDER | WS_SIZEBOX | WS_DLGFRAME | WS_POPUP);
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLong(m_hWnd, GWL_EXSTYLE, lExStyle);

	CRect rc;
	GetWindowRect(&rc);
	ScreenToClient(&rc);

	HRGN hRgn = CreateRoundRectRgn(0, 0, rc.Width() + 1, rc.Height() + 1, 0, 0);
	SetWindowRgn(hRgn, TRUE);
	DeleteObject(hRgn);

	m_pBrowserApp->put_RegisterAsDropTarget(VARIANT_FALSE);
	m_pBrowserApp->put_RegisterAsBrowser(VARIANT_FALSE);

#ifndef _DEBUG
	m_pBrowserApp->put_Silent(VARIANT_TRUE);
#endif

	SetHostFlags(DOCHOSTUIFLAG_DIALOG
		| DOCHOSTUIFLAG_SCROLL_NO
		| DOCHOSTUIFLAG_NO3DBORDER
		| DOCHOSTUIFLAG_DPI_AWARE);

	SetFocus();
	SetForegroundWindow();
	SetActiveWindow();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

HRESULT CHtmlDialog::OnClickClose(IHTMLElement *pElement)
{
	EndDialog(IDCLOSE);
	return S_OK;
}

void CHtmlDialog::OnOK()
{
}

void CHtmlDialog::OnCancel()
{
}

BOOL CHtmlDialog::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		return TRUE;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		{
			if (pMsg->wParam == VK_RETURN)
				return CDHtmlDialog::PreTranslateMessage(pMsg);

			switch (pMsg->wParam)
			{
			case VK_F1:
			case VK_HELP:
			case VK_F2:
			case VK_F3:
			case VK_F4:
			case VK_F5:
			case VK_F6:
			case VK_F7:
			case VK_F8:
			case VK_F9:
			case VK_F10:
			case VK_F11:
			case VK_F12:
				return TRUE;
			}

			if((GetAsyncKeyState(VK_CONTROL) & 0x8000) && ('N' == pMsg->wParam) // Ctrl + N
				|| (GetAsyncKeyState(VK_CONTROL) & 0x8000) && ('P' == pMsg->wParam) // Ctrl + P
				|| (GetAsyncKeyState(VK_CONTROL) & 0x8000) && ('D' == pMsg->wParam) // Ctrl + D
				|| (GetAsyncKeyState(VK_MENU) & 0x8000) && (VK_LEFT == pMsg->wParam) // forward
				|| (GetAsyncKeyState(VK_MENU) & 0x8000 ) && (VK_RIGHT == pMsg->wParam)) // backward
			{
				return TRUE;
			}
		}
		break;
	}

	return CDHtmlDialog::PreTranslateMessage(pMsg);
}

HCURSOR CHtmlDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

HRESULT CHtmlDialog::OnHtmlMouseDown(IHTMLElement* pElement)
{
	HRESULT hr = S_FALSE;

	CComBSTR bsClass;
	hr = pElement->get_className(&bsClass);
	if(SUCCEEDED(hr) && bsClass.m_str && _tcsstr(bsClass.m_str, _T("titlebar")))
	{
		POINT pt;
		GetCursorPos(&pt);
		::PostMessage(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKEWPARAM(pt.x, pt.y));
		return S_FALSE;
	}
	return S_OK;
}

void CHtmlDialog::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
	__super::OnDocumentComplete(pDisp, szUrl);

	m_bVisible = TRUE;
	ShowWindow(SW_SHOW);
}

BOOL CHtmlDialog::CallClientScript(LPCTSTR pStrFuncName, CStringArray* pArrFuncArgs, CComVariant* pOutVarRes)
{
	//Call client function in HTML
	//'pStrFuncName' = client script function name
	//'pArrFuncArgs' = if not NULL, list of arguments
	//'pOutVarRes' = if not NULL, will receive the return value
	//RETURN:
	//      = TRUE if done
	BOOL bRes = FALSE;

	CComVariant vaResult;

	CComPtr<IHTMLDocument2> pIDoc2;
	if(SUCCEEDED(this->GetDHtmlDocument(&pIDoc2)))  //Uses CDHtmlDialog as 'this'
	{
		//Getting IDispatch for Java Script objects
		CComPtr<IDispatch> spScript;
		if(SUCCEEDED(pIDoc2->get_Script(&spScript)))
		{
			//Find dispid for given function in the object
			CComBSTR bstrMember(pStrFuncName);
			DISPID dispid = NULL;
			if(SUCCEEDED(spScript->GetIDsOfNames(IID_NULL, &bstrMember, 1, LOCALE_USER_DEFAULT, &dispid)))
			{

				const int arraySize = pArrFuncArgs ? (int) pArrFuncArgs->GetSize() : 0;

				//Putting parameters  
				DISPPARAMS dispparams;
				memset(&dispparams, 0, sizeof dispparams);
				dispparams.cArgs      = arraySize;
				dispparams.rgvarg     = new VARIANT[dispparams.cArgs];
				dispparams.cNamedArgs = 0;

				for( int i = 0; i < arraySize; i++)
				{
					CComBSTR bstr = pArrFuncArgs->GetAt(arraySize - 1 - i); // back reading
					bstr.CopyTo(&dispparams.rgvarg[i].bstrVal);
					dispparams.rgvarg[i].vt = VT_BSTR;
				}

				EXCEPINFO excepInfo;
				memset(&excepInfo, 0, sizeof excepInfo);
				UINT nArgErr = (UINT)-1;  // initialize to invalid arg

				//Call JavaScript function         
				if(SUCCEEDED(spScript->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &dispparams, &vaResult, &excepInfo, &nArgErr)))
				{
					//Done!
					bRes = TRUE;
				}

				//Free mem
				delete [] dispparams.rgvarg;
			}
		}
	}

	if(pOutVarRes)
		*pOutVarRes = vaResult;

	return bRes;
}

