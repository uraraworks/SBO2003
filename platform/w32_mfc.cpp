/* ========================================================================= */
/* ファイル名：	w32_mfc.cpp													 */
/* 概要：		Web版: MFC 互換クラスの実装（管理画面で使う分だけ）			 */
/* ========================================================================= */
/*
 * 仕組み
 *  - HWND と CWnd の対応表を持つ。MFC で作ったウィンドウは AfxWndProc、
 *    ダイアログは AfxDlgProc を通り、メッセージマップを引いてハンドラを呼ぶ。
 *  - 作成中のウィンドウは最初のメッセージが届いた時に CWnd と結び付ける
 *    （WM_CREATE や WM_INITDIALOG より前に結び付いている必要があるため）。
 *  - DoModal はブラウザでは待てないので DoModalAsync を使う。閉じたあと、
 *    いちばん外側のメッセージ処理が終わってから知らせと delete を行う
 *    （ハンドラの途中で自分自身を消さないため）。
 */
#include "w32_internal.h"
#include "include/afx_compat.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdarg.h>
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* HWND と CWnd の対応															 */
/* ------------------------------------------------------------------------- */
static std::map<HWND, CWnd *>	s_mapPermanent;		/* MFC のオブジェクトが持っている窓 */
static std::map<HWND, CWnd *>	s_mapTemporary;		/* FromHandle で仮に作った窓 */
static CWnd						*s_pWndInit;		/* 作成中のウィンドウ */

/* Default() 用に、処理中のメッセージを積んでおく */
struct AfxCurMsg {
	CWnd	*pWnd;
	UINT	uMsg;
	WPARAM	wParam;
	LPARAM	lParam;
};
static std::vector<AfxCurMsg>	s_aCurMsg;

/* 閉じたモーダルダイアログ（外側のメッセージ処理が終わったら知らせる） */
static std::vector<CDialog *>	s_apModalDone;

static void FlushModalDone(void)
{
	if (!s_aCurMsg.empty()) {
		return;
	}
	while (!s_apModalDone.empty()) {
		CDialog *pDlg = s_apModalDone.front();
		s_apModalDone.erase(s_apModalDone.begin());
		if (pDlg->m_pModalTarget && pDlg->m_pfnModalEnd) {
			(pDlg->m_pModalTarget->*pDlg->m_pfnModalEnd)(pDlg, pDlg->m_nModalResult, pDlg->m_lModalParam);
		}
		delete pDlg;
	}
}

CWnd *CWnd::FromHandlePermanent(HWND hWnd)
{
	std::map<HWND, CWnd *>::iterator it = s_mapPermanent.find(hWnd);
	return (it != s_mapPermanent.end()) ? it->second : NULL;
}

CWnd *CWnd::FromHandle(HWND hWnd)
{
	if (hWnd == NULL) {
		return NULL;
	}
	CWnd *pWnd = FromHandlePermanent(hWnd);
	if (pWnd) {
		return pWnd;
	}
	std::map<HWND, CWnd *>::iterator it = s_mapTemporary.find(hWnd);
	if (it != s_mapTemporary.end()) {
		return it->second;
	}
	pWnd = new CWnd;
	pWnd->m_hWnd = hWnd;
	s_mapTemporary[hWnd] = pWnd;
	return pWnd;
}

/* 窓が無くなった時に、その窓と子の対応を外す */
static void DetachTree(HWND hWnd)
{
	std::vector<HWND> ahWnd;
	for (std::map<HWND, CWnd *>::iterator it = s_mapPermanent.begin(); it != s_mapPermanent.end(); ++ it) {
		if (it->first == hWnd || IsChild(hWnd, it->first)) {
			ahWnd.push_back(it->first);
		}
	}
	for (size_t i = 0; i < ahWnd.size(); i ++) {
		CWnd *pWnd = s_mapPermanent[ahWnd[i]];
		s_mapPermanent.erase(ahWnd[i]);
		pWnd->m_hWnd = NULL;
	}
	std::map<HWND, CWnd *>::iterator it = s_mapTemporary.find(hWnd);
	if (it != s_mapTemporary.end()) {
		delete it->second;
		s_mapTemporary.erase(it);
	}
}

/* ------------------------------------------------------------------------- */
/* ウィンドウプロシージャ														 */
/* ------------------------------------------------------------------------- */
static CWnd *AttachInit(HWND hWnd)
{
	CWnd *pWnd = CWnd::FromHandlePermanent(hWnd);
	if (pWnd == NULL && s_pWndInit) {
		pWnd = s_pWndInit;
		s_pWndInit = NULL;
		pWnd->Attach(hWnd);
	}
	return pWnd;
}

static LRESULT CALLBACK AfxWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWnd *pWnd = AttachInit(hWnd);
	if (pWnd == NULL) {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	AfxCurMsg cur = {pWnd, uMsg, wParam, lParam};
	s_aCurMsg.push_back(cur);
	LRESULT lResult = pWnd->WindowProc(uMsg, wParam, lParam);
	s_aCurMsg.pop_back();
	if (uMsg == WM_NCDESTROY) {
		DetachTree(hWnd);
		pWnd->PostNcDestroy();
	}
	FlushModalDone();
	return lResult;
}

static INT_PTR CALLBACK AfxDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWnd *pWnd = AttachInit(hWnd);
	if (pWnd == NULL) {
		return FALSE;
	}
	AfxCurMsg cur = {pWnd, uMsg, wParam, lParam};
	s_aCurMsg.push_back(cur);
	LRESULT lResult = 0;
	BOOL bHandled = pWnd->OnWndMsg(uMsg, wParam, lParam, &lResult);
	s_aCurMsg.pop_back();
	INT_PTR nRet = FALSE;
	if (uMsg == WM_INITDIALOG) {
		nRet = (INT_PTR)lResult;
	} else if (bHandled && IsWindow(hWnd)) {
		SetWindowLong(hWnd, DWL_MSGRESULT, (LONG)lResult);
		nRet = TRUE;
	}
	if (uMsg == WM_NCDESTROY) {
		DetachTree(hWnd);
		pWnd->PostNcDestroy();
	}
	FlushModalDone();
	return nRet;
}

/* ------------------------------------------------------------------------- */
/* CString																	 */
/* ------------------------------------------------------------------------- */
CString::CString() : m_psz(NULL)					{ Set(""); }
CString::CString(const CString &src) : m_psz(NULL)	{ Set(src.m_psz); }
CString::CString(LPCSTR psz) : m_psz(NULL)			{ Set(psz); }
CString::~CString()									{ free(m_psz); }

void CString::Set(LPCSTR psz)
{
	char *pNew = strdup(psz ? psz : "");
	free(m_psz);
	m_psz = pNew;
}

const CString &CString::operator=(const CString &src)	{ Set(src.m_psz); return *this; }
const CString &CString::operator=(LPCSTR psz)			{ Set(psz); return *this; }

const CString &CString::operator+=(LPCSTR psz)
{
	std::string str = m_psz;
	str += psz ? psz : "";
	Set(str.c_str());
	return *this;
}

void CString::Format(LPCSTR pszFormat, ...)
{
	char szBuf[1024];
	va_list ap;
	va_start(ap, pszFormat);
	vsnprintf(szBuf, sizeof(szBuf), pszFormat, ap);
	va_end(ap);
	Set(szBuf);
}

LPSTR CString::GetBuffer(int nMinLen)
{
	int nLen = GetLength();
	if (nMinLen > nLen) {
		char *pNew = (char *)calloc((size_t)nMinLen + 1, 1);
		memcpy(pNew, m_psz, (size_t)nLen);
		free(m_psz);
		m_psz = pNew;
	}
	return m_psz;
}

void CString::ReleaseBuffer(int nNewLen)
{
	if (nNewLen >= 0) {
		m_psz[nNewLen] = '\0';
	}
}

/* ------------------------------------------------------------------------- */
/* CCmdTarget・CWnd															 */
/* ------------------------------------------------------------------------- */
const AFX_MSGMAP *CCmdTarget::GetThisMessageMap(void)
{
	static const AFX_MSGMAP_ENTRY s_aEntry[] = {{0, 0, 0, 0}};
	static const AFX_MSGMAP s_map = {NULL, &s_aEntry[0]};
	return &s_map;
}

const AFX_MSGMAP *CCmdTarget::GetMessageMap(void) const
{
	return GetThisMessageMap();
}

BEGIN_MESSAGE_MAP(CWnd, CCmdTarget)
END_MESSAGE_MAP()

CWnd::CWnd() : m_hWnd(NULL)
{
}

CWnd::~CWnd()
{
	if (m_hWnd && FromHandlePermanent(m_hWnd) == this) {
		HWND hWnd = m_hWnd;
		if (IsWindow(hWnd)) {
			::DestroyWindow(hWnd);
		}
		if (FromHandlePermanent(hWnd) == this) {
			s_mapPermanent.erase(hWnd);
		}
	}
	m_hWnd = NULL;
}

BOOL CWnd::Attach(HWND hWnd)
{
	if (hWnd == NULL) {
		return FALSE;
	}
	std::map<HWND, CWnd *>::iterator it = s_mapTemporary.find(hWnd);
	if (it != s_mapTemporary.end()) {
		delete it->second;
		s_mapTemporary.erase(it);
	}
	m_hWnd = hWnd;
	s_mapPermanent[hWnd] = this;
	return TRUE;
}

HWND CWnd::Detach(void)
{
	HWND hWnd = m_hWnd;
	if (hWnd && FromHandlePermanent(hWnd) == this) {
		s_mapPermanent.erase(hWnd);
	}
	m_hWnd = NULL;
	return hWnd;
}

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCSTR pszClassName, LPCSTR pszWindowName, DWORD dwStyle,
	int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, LPVOID pParam)
{
	s_pWndInit = this;
	HWND hWnd = ::CreateWindowEx(dwExStyle, pszClassName, pszWindowName, dwStyle,
			x, y, cx, cy, hWndParent, hMenu, AfxGetInstanceHandle(), pParam);
	if (s_pWndInit == this) {
		/* ウィンドウプロシージャを通らなかった（MFC のクラスでない）時はここで結び付ける */
		s_pWndInit = NULL;
		if (hWnd) {
			Attach(hWnd);
		}
	}
	return hWnd ? TRUE : FALSE;
}

BOOL CWnd::DestroyWindow(void)
{
	if (m_hWnd == NULL) {
		return FALSE;
	}
	return ::DestroyWindow(m_hWnd);
}

BOOL CWnd::SetWindowPos(const CWnd *pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
{
	return ::SetWindowPos(m_hWnd, pWndInsertAfter ? pWndInsertAfter->m_hWnd : NULL, x, y, cx, cy, nFlags);
}

void CWnd::MoveWindow(LPCRECT pRect, BOOL bRepaint)
{
	::MoveWindow(m_hWnd, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, bRepaint);
}

void CWnd::GetWindowText(CString &str) const
{
	int nLen = ::GetWindowTextLength(m_hWnd);
	::GetWindowText(m_hWnd, str.GetBuffer(nLen + 1), nLen + 1);
	str.ReleaseBuffer();
}

/* MFC のフレームは、ハンドラの無いメニュー項目を自動で灰色にする */
static BOOL HasCommand(const CCmdTarget *pTarget, UINT uID)
{
	for (const AFX_MSGMAP *pMap = pTarget->GetMessageMap(); pMap; pMap = pMap->pfnGetBaseMap ? pMap->pfnGetBaseMap() : NULL) {
		for (const AFX_MSGMAP_ENTRY *pEntry = pMap->lpEntries; pEntry->nMessage; pEntry ++) {
			if (pEntry->nMessage == WM_COMMAND && pEntry->nID == uID) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void GrayUnhandled(const CCmdTarget *pTarget, W32Menu *pMenu)
{
	for (size_t i = 0; i < pMenu->aItem.size(); i ++) {
		W32MenuItem &item = pMenu->aItem[i];
		if (item.pSub) {
			GrayUnhandled(pTarget, item.pSub);
		} else if (!(item.uFlags & MF_SEPARATOR) && !HasCommand(pTarget, item.uID)) {
			item.uFlags |= MF_GRAYED;
		}
	}
}

BOOL CWnd::SetMenu(CMenu *pMenu)
{
	HMENU hMenu = pMenu ? pMenu->m_hMenu : NULL;
	W32Menu *pW32Menu = W32GetMenu(hMenu);
	if (pW32Menu && dynamic_cast<CFrameWnd *>(this)) {
		GrayUnhandled(this, pW32Menu);
	}
	return ::SetMenu(m_hWnd, hMenu);
}

BOOL CWnd::UpdateData(BOOL bSaveAndValidate)
{
	CDataExchange dx(this, bSaveAndValidate);
	DoDataExchange(&dx);
	return TRUE;
}

LRESULT CWnd::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;
	if (!OnWndMsg(uMsg, wParam, lParam, &lResult)) {
		lResult = DefWindowProc(uMsg, wParam, lParam);
	}
	return lResult;
}

BOOL CWnd::OnWndMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	UINT uCode = 0, uID = 0;
	if (uMsg == WM_COMMAND) {
		uID = LOWORD(wParam);
		uCode = HIWORD(wParam);
	}
	for (const AFX_MSGMAP *pMap = GetMessageMap(); pMap; pMap = pMap->pfnGetBaseMap ? pMap->pfnGetBaseMap() : NULL) {
		for (const AFX_MSGMAP_ENTRY *pEntry = pMap->lpEntries; pEntry->nMessage; pEntry ++) {
			if (pEntry->nMessage != uMsg) {
				continue;
			}
			if (uMsg == WM_COMMAND && (pEntry->nID != uID || pEntry->nCode != uCode)) {
				continue;
			}
			LRESULT lResult = pEntry->pfn(this, wParam, lParam);
			if (pResult) {
				*pResult = lResult;
			}
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CWnd::DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

BOOL CWnd::PreCreateWindow(CREATESTRUCT &)
{
	return TRUE;
}

LRESULT CWnd::Default(void)
{
	if (s_aCurMsg.empty()) {
		return 0;
	}
	const AfxCurMsg &cur = s_aCurMsg.back();
	return cur.pWnd->DefWindowProc(cur.uMsg, cur.wParam, cur.lParam);
}

/* ------------------------------------------------------------------------- */
/* クラス登録																 */
/* ------------------------------------------------------------------------- */
HINSTANCE AfxGetInstanceHandle(void)
{
	return GetModuleHandle(NULL);
}

BOOL AfxRegisterClass(WNDCLASS *pWc)
{
	WNDCLASS wc;
	if (GetClassInfo(pWc->hInstance, pWc->lpszClassName, &wc)) {
		return TRUE;
	}
	return RegisterClass(pWc) ? TRUE : FALSE;
}

LPCSTR AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor, HBRUSH hbrBackground, HICON hIcon)
{
	static std::set<std::string> s_setName;		/* 名前の文字列を持ち続ける */
	char szName[96];
	snprintf(szName, sizeof(szName), "Afx:%x:%lx:%lx:%lx", nClassStyle,
			(unsigned long)(ULONG_PTR)hCursor, (unsigned long)(ULONG_PTR)hbrBackground, (unsigned long)(ULONG_PTR)hIcon);
	LPCSTR pszName = s_setName.insert(szName).first->c_str();

	WNDCLASS wc;
	if (!GetClassInfo(NULL, pszName, &wc)) {
		ZeroMemory(&wc, sizeof(wc));
		wc.style			= nClassStyle;
		wc.lpfnWndProc		= AfxWndProc;
		wc.hInstance		= AfxGetInstanceHandle();
		wc.hCursor			= hCursor;
		wc.hbrBackground	= hbrBackground;
		wc.hIcon			= hIcon;
		wc.lpszClassName	= pszName;
		RegisterClass(&wc);
	}
	return pszName;
}

/* ------------------------------------------------------------------------- */
/* CMenu																	 */
/* ------------------------------------------------------------------------- */
CMenu::~CMenu()
{
	/* メニューバーとして付いている間は窓が使っているので消さない（窓と一緒に捨てる） */
}

BOOL CMenu::LoadMenu(LPCSTR pszName)
{
	m_hMenu = ::LoadMenu(AfxGetInstanceHandle(), pszName);
	return m_hMenu ? TRUE : FALSE;
}

/* ------------------------------------------------------------------------- */
/* CFrameWnd																 */
/* ------------------------------------------------------------------------- */
BEGIN_MESSAGE_MAP(CFrameWnd, CWnd)
END_MESSAGE_MAP()

BOOL CFrameWnd::PreCreateWindow(CREATESTRUCT &cs)
{
	if (cs.lpszClass == NULL) {
		WNDCLASS wc;
		if (!GetClassInfo(NULL, "AfxFrameOrView", &wc)) {
			ZeroMemory(&wc, sizeof(wc));
			wc.style			= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc		= AfxWndProc;
			wc.hInstance		= AfxGetInstanceHandle();
			wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground	= GetSysColorBrush(COLOR_BTNFACE);
			wc.lpszClassName	= "AfxFrameOrView";
			RegisterClass(&wc);
		}
		cs.lpszClass = "AfxFrameOrView";
	}
	return TRUE;
}

BOOL CFrameWnd::Create(LPCSTR pszClassName, LPCSTR pszWindowName, DWORD dwStyle, const RECT &rect,
	CWnd *pParentWnd, LPCSTR pszMenuName, DWORD dwExStyle)
{
	CREATESTRUCT cs;
	ZeroMemory(&cs, sizeof(cs));
	cs.lpszClass	= pszClassName;
	cs.lpszName		= pszWindowName;
	cs.style		= (LONG)dwStyle;
	cs.dwExStyle	= dwExStyle;
	cs.x			= rect.left;
	cs.y			= rect.top;
	cs.cx			= rect.right - rect.left;
	cs.cy			= rect.bottom - rect.top;
	cs.hwndParent	= pParentWnd ? pParentWnd->m_hWnd : NULL;
	cs.hInstance	= AfxGetInstanceHandle();
	if (pszMenuName) {
		cs.hMenu = ::LoadMenu(cs.hInstance, pszMenuName);
	}
	if (!PreCreateWindow(cs)) {
		return FALSE;
	}
	return CreateEx(cs.dwExStyle, cs.lpszClass, cs.lpszName, (DWORD)cs.style,
			cs.x, cs.y, cs.cx, cs.cy, cs.hwndParent, cs.hMenu, cs.lpCreateParams);
}

/* ------------------------------------------------------------------------- */
/* CDialog																	 */
/* ------------------------------------------------------------------------- */
BEGIN_MESSAGE_MAP(CDialog, CWnd)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
END_MESSAGE_MAP()

CDialog::CDialog()
	: m_nIDHelp(0), m_nIDTemplate(0), m_pParentWnd(NULL), m_nModalResult(-1), m_bModal(FALSE),
	  m_pModalTarget(NULL), m_pfnModalEnd(NULL), m_lModalParam(0)
{
}

CDialog::CDialog(UINT nIDTemplate, CWnd *pParentWnd)
	: m_nIDHelp(nIDTemplate), m_nIDTemplate(nIDTemplate), m_pParentWnd(pParentWnd), m_nModalResult(-1), m_bModal(FALSE),
	  m_pModalTarget(NULL), m_pfnModalEnd(NULL), m_lModalParam(0)
{
}

CDialog::~CDialog()
{
	/* 窓は CWnd のデストラクタで消える */
	s_apModalDone.erase(std::remove(s_apModalDone.begin(), s_apModalDone.end(), this), s_apModalDone.end());
}

BOOL CDialog::Create(UINT nIDTemplate, CWnd *pParentWnd)
{
	m_nIDTemplate = nIDTemplate;
	if (pParentWnd) {
		m_pParentWnd = pParentWnd;
	}
	s_pWndInit = this;
	HWND hWnd = ::CreateDialogParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDTemplate),
			m_pParentWnd ? m_pParentWnd->m_hWnd : NULL, AfxDlgProc, 0);
	if (s_pWndInit == this) {
		s_pWndInit = NULL;
	}
	return hWnd ? TRUE : FALSE;
}

int CDialog::DoModal(void)
{
	/* ブラウザでは閉じるまで待てない。呼び出し元は DoModalAsync を使うこと */
	printf("[mfc] DoModal は使えません（DoModalAsync を使ってください）\n");
	return IDCANCEL;
}

/* 持ち主は「いま処理中の窓」のいちばん上の親（MFC の GetSafeOwner と同じ考え方） */
static HWND ModalOwner(CWnd *pParent)
{
	HWND hWnd = pParent ? pParent->m_hWnd : NULL;
	if (hWnd == NULL && !s_aCurMsg.empty()) {
		hWnd = s_aCurMsg.back().pWnd->m_hWnd;
	}
	while (hWnd && (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) && GetParent(hWnd)) {
		hWnd = GetParent(hWnd);
	}
	return hWnd;
}

void CDialog::DoModalAsync(CWnd *pTarget, AFX_PMODALEND pfnEnd, LPARAM lParam)
{
	m_bModal		= TRUE;
	m_nModalResult	= -1;
	m_pModalTarget	= pTarget;
	m_pfnModalEnd	= pfnEnd;
	m_lModalParam	= lParam;

	HWND hWndOwner = ModalOwner(m_pParentWnd);
	s_pWndInit = this;
	HWND hWnd = ::CreateDialogParam(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_nIDTemplate), hWndOwner, AfxDlgProc, 0);
	if (s_pWndInit == this) {
		s_pWndInit = NULL;
	}
	if (hWnd == NULL) {
		m_nModalResult = IDCANCEL;
		s_apModalDone.push_back(this);
		FlushModalDone();
		return;
	}
	if (hWndOwner) {
		::EnableWindow(hWndOwner, FALSE);
	}
	::ShowWindow(hWnd, SW_SHOW);
}

void CDialog::EndDialog(int nResult)
{
	m_nModalResult = nResult;
	if (m_hWnd) {
		::EndDialog(m_hWnd, nResult);
	}
}

void CDialog::PostNcDestroy(void)
{
	if (m_bModal) {
		m_bModal = FALSE;
		if (m_nModalResult == -1) {
			m_nModalResult = IDCANCEL;
		}
		s_apModalDone.push_back(this);
	}
}

BOOL CDialog::OnInitDialog(void)
{
	UpdateData(FALSE);
	return TRUE;
}

void CDialog::OnOK(void)
{
	if (!UpdateData(TRUE)) {
		return;
	}
	EndDialog(IDOK);
}

void CDialog::OnCancel(void)
{
	EndDialog(IDCANCEL);
}

BOOL CDialog::OnWndMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	if (uMsg == WM_INITDIALOG) {
		*pResult = OnInitDialog();
		return TRUE;
	}
	return CWnd::OnWndMsg(uMsg, wParam, lParam, pResult);
}

LRESULT CDialog::DefWindowProc(UINT, WPARAM, LPARAM)
{
	/* ダイアログの既定の処理は、AfxDlgProc が FALSE を返したあと互換層が行う */
	return 0;
}

/* ------------------------------------------------------------------------- */
/* コントロール																 */
/* ------------------------------------------------------------------------- */
int CListCtrl::InsertColumn(int nCol, LPCSTR pszHeading, int nFormat, int nWidth, int nSubItem)
{
	LVCOLUMN lvc;
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask	= LVCF_TEXT | LVCF_FMT;
	lvc.fmt		= nFormat;
	lvc.pszText	= (LPSTR)pszHeading;
	if (nWidth != -1) {
		lvc.mask |= LVCF_WIDTH;
		lvc.cx = nWidth;
	}
	if (nSubItem != -1) {
		lvc.mask |= LVCF_SUBITEM;
		lvc.iSubItem = nSubItem;
	}
	return ListView_InsertColumn(m_hWnd, nCol, &lvc);
}

int CListCtrl::InsertItem(int nItem, LPCSTR pszItem)
{
	LVITEM lvi;
	ZeroMemory(&lvi, sizeof(lvi));
	lvi.mask	= LVIF_TEXT;
	lvi.iItem	= nItem;
	lvi.pszText	= (LPSTR)pszItem;
	return ListView_InsertItem(m_hWnd, &lvi);
}

BOOL CListCtrl::SetItemText(int nItem, int nSubItem, LPCSTR pszText)
{
	ListView_SetItemText(m_hWnd, nItem, nSubItem, (LPSTR)pszText);
	return TRUE;
}

CString CListCtrl::GetItemText(int nItem, int nSubItem) const
{
	char szBuf[1024];
	szBuf[0] = '\0';
	ListView_GetItemText(m_hWnd, nItem, nSubItem, szBuf, (int)sizeof(szBuf));
	return CString(szBuf);
}

BOOL CListCtrl::SetItemState(int nItem, UINT nState, UINT nMask)
{
	ListView_SetItemState(m_hWnd, nItem, nState, nMask);
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* DDX																		 */
/* ------------------------------------------------------------------------- */
static HWND DlgItem(CDataExchange *pDX, int nIDC)
{
	return ::GetDlgItem(pDX->m_pDlgWnd->m_hWnd, nIDC);
}

void DDX_Control(CDataExchange *pDX, int nIDC, CWnd &rControl)
{
	if (rControl.m_hWnd == NULL) {
		rControl.Attach(DlgItem(pDX, nIDC));
	}
}

void DDX_Text(CDataExchange *pDX, int nIDC, CString &value)
{
	HWND hWnd = DlgItem(pDX, nIDC);
	if (pDX->m_bSaveAndValidate) {
		CWnd::FromHandle(hWnd)->GetWindowText(value);
	} else {
		::SetWindowText(hWnd, value);
	}
}

long long AfxDDXGetNumber(CDataExchange *pDX, int nIDC, BOOL bSigned)
{
	char szBuf[64];
	szBuf[0] = '\0';
	::GetWindowText(DlgItem(pDX, nIDC), szBuf, (int)sizeof(szBuf));
	return bSigned ? strtoll(szBuf, NULL, 10) : (long long)strtoull(szBuf, NULL, 10);
}

void AfxDDXSetNumber(CDataExchange *pDX, int nIDC, long long value, BOOL bSigned)
{
	char szBuf[64];
	if (bSigned) {
		snprintf(szBuf, sizeof(szBuf), "%lld", value);
	} else {
		snprintf(szBuf, sizeof(szBuf), "%llu", (unsigned long long)value);
	}
	::SetWindowText(DlgItem(pDX, nIDC), szBuf);
}

void DDX_Check(CDataExchange *pDX, int nIDC, int &value)
{
	HWND hWnd = DlgItem(pDX, nIDC);
	if (pDX->m_bSaveAndValidate) {
		value = (int)::SendMessage(hWnd, BM_GETCHECK, 0, 0);
	} else {
		::SendMessage(hWnd, BM_SETCHECK, (WPARAM)value, 0);
	}
}

void DDX_CBIndex(CDataExchange *pDX, int nIDC, int &index)
{
	HWND hWnd = DlgItem(pDX, nIDC);
	if (pDX->m_bSaveAndValidate) {
		index = (int)::SendMessage(hWnd, CB_GETCURSEL, 0, 0);
	} else {
		::SendMessage(hWnd, CB_SETCURSEL, (WPARAM)index, 0);
	}
}
