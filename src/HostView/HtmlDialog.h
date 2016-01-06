#pragma once

#include "resource.h"

/**
 * Our base HTML Window
 */
class CHtmlDialog : public CDHtmlDialog
{
public:
	CHtmlDialog(CWnd* pParent = NULL);	// standard constructor
	CHtmlDialog(UINT nIDTemplate, UINT nHtmlResID = 0, CWnd *pParentWnd = NULL) : CDHtmlDialog(nIDTemplate, nHtmlResID, pParentWnd) { m_hIcon = AfxGetApp()->LoadIcon(MAKEINTRESOURCE(IDI_HOSTVIEW)); }
	virtual ~CHtmlDialog() {}

	enum { IDD = 0, IDH = 0 };
	virtual void OnWindowPosChanging(WINDOWPOS* lpwndpos);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	HRESULT OnHtmlMouseDown(IHTMLElement *pElement);
	HRESULT OnClickClose(IHTMLElement *pElement);
	BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);


	HICON m_hIcon;
	BOOL m_bVisible;
	virtual void OnOK();
	virtual void OnCancel();
	BOOL CallClientScript(LPCTSTR pStrFuncName, CStringArray* pArrFuncArgs, CComVariant* pOutVarRes);

	virtual BOOL OnInitDialog();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
};


#define DHTML_EVENT_TAG_ALL(dispid, memberFxn)\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("a"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("abbr"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("acronym"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("address"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("applet"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("area"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("b"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("base"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("basefont"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("bdo"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("big"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("blockquote"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("body"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("br"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("button"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("caption"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("center"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("cite"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("code"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("col"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("colgroup"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("dd"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("del"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("dir"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("div"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("dfn"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("dl"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("dt"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("em"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("fieldset"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("font"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("form"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("frame"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("frameset"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("h1"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("h2"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("h3"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("h4"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("head"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("hr"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("html"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("i"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("iframe"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("img"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("input"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("ins"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("isindex"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("kbd"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("label"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("legend"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("li"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("link"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("map"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("menu"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("meta"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("noframes"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("noscript"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("object"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("ol"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("optgroup"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("option"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("p"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("param"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("pre"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("q"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("s"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("samp"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("script"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("select"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("small"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("span"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("strike"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("strong"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("style"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("sub"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("sup"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("table"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("tbody"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("td"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("textarea"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("tfoot"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("th"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("thead"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("title"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("tr"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("tt"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("u"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("ul"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("var"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },\
{ DHTMLEVENTMAPENTRY_TAG, dispid, _T("xmp"), (DHEVTFUNCCONTROL) (DHEVTFUNC) theClass::memberFxn },
