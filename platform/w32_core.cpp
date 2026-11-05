/* ========================================================================= */
/* ファイル名：	w32_core.cpp												 */
/* 概要：		Win32 互換レイヤ：ウィンドウ・メッセージ・タイマー・入力		 */
/* ========================================================================= */
/*
 * 仕組み
 *  - HWND は W32Wnd 構造体へのポインタ。最初に作られたトップレベルウィンドウを
 *    ブラウザの canvas に対応する「メインウィンドウ」とみなす。
 *  - それ以外のトップレベルウィンドウ（メッセージログ、デバッグ、吹き出しなど）は
 *    作られるが画面には出ない。
 *  - メニューを持つウィンドウ（管理者ウィンドウ）とダイアログだけは、ブラウザの
 *    ページ上に「小窓」として出す。中のコントロールはすべて HTML の部品にする。
 *  - メインウィンドウの EDIT / BUTTON は HTML の <input> / <button> を canvas の上に重ねて代用する。
 *  - メッセージキューは Win32 と同じ順序で処理する。
 *    ポストされたメッセージ → WM_TIMER → WM_PAINT の順。
 *  - キーボード・マウスは JavaScript 側のイベントから W32Js* 関数が呼ばれ、
 *    メッセージとしてキューに積まれる。
 */
#include "w32_internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <signal.h>
#include <deque>
#include <map>
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* 内部データ																 */
/* ------------------------------------------------------------------------- */
static std::map<std::string, WNDCLASS>	s_mapClass;			/* 登録クラス（キーは小文字） */
static std::vector<W32Wnd *>			s_apWnd;			/* 生きているウィンドウ */
static std::deque<MSG>					s_queMsg;			/* メッセージキュー */
static HWND								s_hWndScreen;		/* メインウィンドウ */
static HWND								s_hWndFocus;		/* フォーカスのあるウィンドウ */
static HWND								s_hWndCapture;		/* マウスキャプチャ中のウィンドウ */
static HWND								s_hWndActive;		/* アクティブウィンドウ */
static BOOL								s_bQuit;			/* WM_QUIT を出す */
static int								s_nQuitCode;
static BYTE								s_abyKey[256];		/* キー状態(0x80:押下 0x01:トグル) */
static POINT							s_ptMouse;			/* マウス位置（メインウィンドウのクライアント座標） */
static DWORD							s_dwLastError;

struct W32Timer {
	HWND		hWnd;
	UINT_PTR	nID;
	UINT		uElapse;
	DWORD		dwNext;
	TIMERPROC	pfn;
};
static std::vector<W32Timer>			s_aTimer;

static std::string ToLower(const char *psz)
{
	std::string s = psz ? psz : "";
	for (size_t i = 0; i < s.size(); i ++) {
		s[i] = (char)tolower((unsigned char)s[i]);
	}
	return s;
}

W32Wnd *W32GetWnd(HWND hWnd)
{
	if (hWnd == NULL) {
		return NULL;
	}
	W32Wnd *p = (W32Wnd *)hWnd;
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		if (s_apWnd[i] == p) {
			return p->bDestroyed ? NULL : p;
		}
	}
	return NULL;
}

const std::vector<W32Wnd *> &W32GetWndList(void)
{
	return s_apWnd;
}

W32Wnd *W32GetPopupRoot(W32Wnd *p)
{
	while (p) {
		if (p->hWndParent == NULL) {
			return p->bPopup ? p : NULL;
		}
		p = W32GetWnd(p->hWndParent);
	}
	return NULL;
}

W32Wnd *W32FindWndByHtmlID(int nHtmlID)
{
	if (nHtmlID == 0) {
		return NULL;
	}
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		if (s_apWnd[i]->nHtmlID == nHtmlID) {
			return s_apWnd[i];
		}
	}
	return NULL;
}

/* 小窓の枠：縁 3px、タイトル 19px、メニュー 19px（HTML 側の CSS と同じ値） */
void W32GetFrameInsets(const W32Wnd *p, RECT *pInsets)
{
	SetRectEmpty(pInsets);
	if (p == NULL || !p->bPopup) {
		return;
	}
	SetRect(pInsets, 3, 3 + 19, 3, 3);
	if (p->hMenu) {
		pInsets->top += 19;
	}
}

HWND W32GetScreenWnd(void)
{
	return s_hWndScreen;
}

void W32GetScreenSize(int *pcx, int *pcy)
{
	W32Wnd *p = W32GetWnd(s_hWndScreen);
	if (p) {
		*pcx = p->rc.right - p->rc.left;
		*pcy = p->rc.bottom - p->rc.top;
	} else {
		*pcx = 640;
		*pcy = 480;
	}
}

/* ------------------------------------------------------------------------- */
/* 標準コントロールのウィンドウプロシージャ									 */
/* ------------------------------------------------------------------------- */
static LRESULT CALLBACK W32ControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return 0;
	}
	switch (uMsg) {
	case WM_SETTEXT:
		p->strText = lParam ? (const char *)lParam : "";
		W32HtmlSetText(p);
		return TRUE;
	case WM_GETTEXT:
		W32HtmlGetText(p);
		if (wParam > 0 && lParam) {
			size_t n = std::min((size_t)wParam - 1, p->strText.size());
			memcpy((char *)lParam, p->strText.c_str(), n);
			((char *)lParam)[n] = 0;
			return (LRESULT)n;
		}
		return 0;
	case WM_GETTEXTLENGTH:
		W32HtmlGetText(p);
		return (LRESULT)p->strText.size();
	case BM_GETCHECK:
		return p->nCheck;
	case BM_SETCHECK:
		p->nCheck = (int)wParam;
		W32HtmlUpdate(p);
		return 0;
	case EM_REPLACESEL:
		/* ログ表示用：末尾に追加するだけ */
		if (lParam) {
			p->strText += (const char *)lParam;
			if (p->strText.size() > 64 * 1024) {
				p->strText.erase(0, p->strText.size() - 48 * 1024);
			}
			W32HtmlSetText(p);
		}
		return 0;
	case EM_GETLINECOUNT:
		{
			W32HtmlGetText(p);
			LRESULT n = 1;
			for (size_t i = 0; i < p->strText.size(); i ++) {
				if (p->strText[i] == '\n') {
					n ++;
				}
			}
			return n;
		}
	case EM_LINEINDEX:
	case EM_LINELENGTH:
	case EM_SETSEL:
	case EM_GETSEL:
	case EM_SCROLL:
	case EM_LINESCROLL:
	case EM_SCROLLCARET:
	case EM_LIMITTEXT:
	case EM_SETREADONLY:
	case EM_SETCHARFORMAT:
	case EM_GETCHARFORMAT:
	case EM_SETBKGNDCOLOR:
	case EM_EXLIMITTEXT:
	case EM_STREAMOUT:
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void RegisterBuiltinClass(const char *pszName)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = W32ControlProc;
	wc.lpszClassName = pszName;
	s_mapClass[ToLower(pszName)] = wc;
}

static void InitBuiltinClass(void)
{
	static BOOL bInit = FALSE;
	if (bInit) {
		return;
	}
	bInit = TRUE;
	RegisterBuiltinClass("EDIT");
	RegisterBuiltinClass("BUTTON");
	RegisterBuiltinClass("STATIC");
	RegisterBuiltinClass("RICHEDIT");
	RegisterBuiltinClass("RichEdit20A");
	RegisterBuiltinClass("RichEdit20W");
	RegisterBuiltinClass("LISTBOX");
	RegisterBuiltinClass("COMBOBOX");
	RegisterBuiltinClass("SCROLLBAR");
	RegisterBuiltinClass("SysListView32");
	s_mapClass["listbox"].lpfnWndProc		= W32CtrlProc;
	s_mapClass["combobox"].lpfnWndProc		= W32CtrlProc;
	s_mapClass["scrollbar"].lpfnWndProc		= W32CtrlProc;
	s_mapClass["syslistview32"].lpfnWndProc	= W32CtrlProc;
	/* ダイアログ */
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = W32DialogWndProc;
	wc.lpszClassName = WC_DIALOG;
	wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
	s_mapClass[WC_DIALOG] = wc;
}

/* 次に作るダイアログのプロシージャ */
static DLGPROC s_pfnNextDlgProc;

void W32SetNextDlgProc(DLGPROC pfn)
{
	s_pfnNextDlgProc = pfn;
}

/* ------------------------------------------------------------------------- */
/* ウィンドウクラス・ウィンドウ												 */
/* ------------------------------------------------------------------------- */
ATOM RegisterClass(const WNDCLASS *pWc)
{
	InitBuiltinClass();
	if (pWc == NULL || pWc->lpszClassName == NULL) {
		return 0;
	}
	std::string key = ToLower(pWc->lpszClassName);
	if (s_mapClass.count(key)) {
		s_dwLastError = 1410;	/* ERROR_CLASS_ALREADY_EXISTS */
		return 0;
	}
	s_mapClass[key] = *pWc;
	return (ATOM)s_mapClass.size();
}

BOOL GetClassInfo(HINSTANCE, LPCSTR pszClass, LPWNDCLASS pWc)
{
	InitBuiltinClass();
	if (pszClass == NULL || pWc == NULL) {
		return FALSE;
	}
	std::map<std::string, WNDCLASS>::iterator it = s_mapClass.find(ToLower(pszClass));
	if (it == s_mapClass.end()) {
		return FALSE;
	}
	*pWc = it->second;
	return TRUE;
}

ATOM RegisterClassEx(const WNDCLASSEX *pWc)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style			= pWc->style;
	wc.lpfnWndProc		= pWc->lpfnWndProc;
	wc.hInstance		= pWc->hInstance;
	wc.lpszClassName	= pWc->lpszClassName;
	return RegisterClass(&wc);
}

BOOL UnregisterClass(LPCSTR pszClass, HINSTANCE)
{
	return s_mapClass.erase(ToLower(pszClass)) ? TRUE : FALSE;
}

HWND CreateWindowEx(DWORD dwExStyle, LPCSTR pszClass, LPCSTR pszName, DWORD dwStyle,
	int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, HINSTANCE hInst, LPVOID pParam)
{
	InitBuiltinClass();
	std::string key = ToLower(pszClass);
	std::map<std::string, WNDCLASS>::iterator it = s_mapClass.find(key);
	if (it == s_mapClass.end()) {
		printf("[w32] CreateWindow: 未登録のクラス %s\n", pszClass ? pszClass : "(null)");
		return NULL;
	}

	W32Wnd *p = new W32Wnd;
	p->dwMagic		= W32WND_MAGIC;
	p->strClass		= pszClass;
	p->strText		= pszName ? pszName : "";
	p->pfnWndProc	= it->second.lpfnWndProc;
	p->hWndParent	= (dwStyle & WS_CHILD) ? hWndParent : NULL;
	p->hWndOwner	= (dwStyle & WS_CHILD) ? NULL : hWndParent;
	p->hbrBackground = it->second.hbrBackground;
	p->dwStyle		= dwStyle;
	p->dwExStyle	= dwExStyle;
	p->nID			= (dwStyle & WS_CHILD) ? (int)(INT_PTR)hMenu : 0;
	if (x == CW_USEDEFAULT) {
		x = 0;
		y = 0;
	}
	if (cx == CW_USEDEFAULT) {
		cx = 640;
		cy = 480;
	}
	SetRect(&p->rc, x, y, x + cx, y + cy);
	p->bVisible		= (dwStyle & WS_VISIBLE) ? TRUE : FALSE;
	p->bEnabled		= (dwStyle & WS_DISABLED) ? FALSE : TRUE;
	p->bInvalid		= TRUE;
	p->bDestroyed	= FALSE;
	p->lUserData	= 0;
	p->nCheck		= 0;
	p->nHtmlID		= 0;
	p->bScreen		= FALSE;

	if (key == "edit") {
		p->nKind = W32WND_EDIT;
	} else if (key == "button") {
		p->nKind = W32WND_BUTTON;
	} else if (key.compare(0, 8, "richedit") == 0) {
		p->nKind = W32WND_RICHEDIT;
	} else if (key == "static") {
		p->nKind = W32WND_STATIC;
	} else if (key == "combobox") {
		p->nKind = W32WND_COMBOBOX;
	} else if (key == "listbox") {
		p->nKind = W32WND_LISTBOX;
	} else if (key == "scrollbar") {
		p->nKind = W32WND_SCROLLBAR;
	} else if (key == "syslistview32") {
		p->nKind = W32WND_LISTVIEW;
	} else {
		p->nKind = W32WND_NORMAL;
	}
	if (key == WC_DIALOG) {
		p->bDialog		= TRUE;
		p->pfnDlgProc	= s_pfnNextDlgProc;
		s_pfnNextDlgProc = NULL;
	}
	/* メニュー：トップレベルなら hMenu 引数、無ければクラスのメニュー */
	if (!(dwStyle & WS_CHILD)) {
		if (hMenu) {
			p->hMenu = hMenu;
		} else if (it->second.lpszMenuName) {
			p->hMenu = LoadMenu(hInst, it->second.lpszMenuName);
		}
	}

	/* 最初のトップレベルウィンドウを画面(canvas)に対応させる */
	if (p->hWndParent == NULL && p->nKind == W32WND_NORMAL && s_hWndScreen == NULL) {
		p->bScreen = TRUE;
		SetRect(&p->rc, 0, 0, cx, cy);
		s_hWndScreen = (HWND)p;
		s_hWndActive = (HWND)p;
	}
	/* メニュー付き・タイトル付きのウィンドウとダイアログは、ブラウザに小窓として出す */
	if (p->hWndParent == NULL && !p->bScreen && p->nKind == W32WND_NORMAL &&
		(p->hMenu || p->bDialog || (p->dwStyle & WS_CAPTION) == WS_CAPTION)) {
		p->bPopup = TRUE;
	}
	s_apWnd.push_back(p);

	/* 画面に出るのは、メインウィンドウの子コントロールと小窓の中身 */
	if ((p->nKind == W32WND_EDIT || p->nKind == W32WND_BUTTON) && p->hWndParent && p->hWndParent == s_hWndScreen) {
		W32HtmlCreate(p);
	} else if (W32GetPopupRoot(p)) {
		W32HtmlCreate(p);
		if (p->bPopup && p->hMenu) {
			W32Menu *pMenu = W32GetMenu(p->hMenu);
			if (pMenu) {
				pMenu->hWnd = (HWND)p;
			}
			W32HtmlMenu(p);
		}
	}

	CREATESTRUCT cs;
	ZeroMemory(&cs, sizeof(cs));
	cs.lpCreateParams	= pParam;
	cs.hInstance		= hInst;
	cs.hMenu			= hMenu;
	cs.hwndParent		= hWndParent;
	cs.cx = cx;
	cs.cy = cy;
	cs.x = x;
	cs.y = y;
	cs.style			= (LONG)dwStyle;
	cs.lpszName			= pszName;
	cs.lpszClass		= pszClass;
	cs.dwExStyle		= dwExStyle;
	if (SendMessage((HWND)p, WM_CREATE, 0, (LPARAM)&cs) == -1) {
		DestroyWindow((HWND)p);
		return NULL;
	}
	W32HtmlUpdate(p);
	return (HWND)p;
}

BOOL DestroyWindow(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	/* 子ウィンドウと、持ち主になっているウィンドウを先に壊す */
	for (size_t i = 0; i < s_apWnd.size(); ) {
		W32Wnd *pChild = s_apWnd[i];
		if (!pChild->bDestroyed && (pChild->hWndParent == hWnd || pChild->hWndOwner == hWnd)) {
			DestroyWindow((HWND)pChild);
			i = 0;
			continue;
		}
		i ++;
	}
	SendMessage(hWnd, WM_DESTROY, 0, 0);
	SendMessage(hWnd, WM_NCDESTROY, 0, 0);
	W32HtmlDestroy(p);
	p->bDestroyed = TRUE;
	W32FreeWndSurface(p);
	for (size_t i = 0; i < s_aTimer.size(); ) {
		if (s_aTimer[i].hWnd == hWnd) {
			s_aTimer.erase(s_aTimer.begin() + i);
		} else {
			i ++;
		}
	}
	if (s_hWndFocus == hWnd) {
		s_hWndFocus = NULL;
	}
	if (s_hWndCapture == hWnd) {
		s_hWndCapture = NULL;
	}
	if (s_hWndScreen == hWnd) {
		s_hWndScreen = NULL;
	}
	/* 構造体はダングリング参照対策のため解放せず、一覧からだけ外す */
	s_apWnd.erase(std::remove(s_apWnd.begin(), s_apWnd.end(), p), s_apWnd.end());
	return TRUE;
}

BOOL ShowWindow(HWND hWnd, int nCmdShow)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	BOOL bOld = p->bVisible;
	p->bVisible = (nCmdShow != SW_HIDE) ? TRUE : FALSE;
	if (p->bVisible) {
		p->dwStyle |= WS_VISIBLE;
		p->bInvalid = TRUE;
	} else {
		p->dwStyle &= ~WS_VISIBLE;
	}
	W32HtmlUpdate(p);
	return bOld;
}

BOOL UpdateWindow(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p && p->bInvalid && p->bVisible) {
		SendMessage(hWnd, WM_PAINT, 0, 0);
	}
	return TRUE;
}

BOOL IsWindow(HWND hWnd)
{
	return W32GetWnd(hWnd) ? TRUE : FALSE;
}

BOOL IsWindowVisible(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return (p && p->bVisible) ? TRUE : FALSE;
}

BOOL IsIconic(HWND)
{
	return FALSE;
}

BOOL IsWindowEnabled(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return (p && p->bEnabled) ? TRUE : FALSE;
}

BOOL EnableWindow(HWND hWnd, BOOL bEnable)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	BOOL bOld = !p->bEnabled;
	p->bEnabled = bEnable ? TRUE : FALSE;
	W32HtmlUpdate(p);
	return bOld;
}

BOOL MoveWindow(HWND hWnd, int x, int y, int cx, int cy, BOOL)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	if (p->bScreen) {
		return TRUE;		/* 画面の大きさは変えない */
	}
	SetRect(&p->rc, x, y, x + cx, y + cy);
	W32HtmlUpdate(p);
	SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(cx, cy));
	return TRUE;
}

BOOL SetWindowPos(HWND hWnd, HWND, int x, int y, int cx, int cy, UINT uFlags)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	if (!p->bScreen) {
		int nW = p->rc.right - p->rc.left;
		int nH = p->rc.bottom - p->rc.top;
		if (!(uFlags & SWP_NOMOVE)) {
			p->rc.left = x;
			p->rc.top = y;
		}
		if (!(uFlags & SWP_NOSIZE)) {
			nW = cx;
			nH = cy;
		}
		p->rc.right = p->rc.left + nW;
		p->rc.bottom = p->rc.top + nH;
	}
	if (uFlags & SWP_SHOWWINDOW) {
		p->bVisible = TRUE;
	}
	if (uFlags & SWP_HIDEWINDOW) {
		p->bVisible = FALSE;
	}
	W32HtmlUpdate(p);
	return TRUE;
}

BOOL GetWindowRect(HWND hWnd, LPRECT pRect)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || pRect == NULL) {
		return FALSE;
	}
	*pRect = p->rc;
	if (p->hWndParent) {
		POINT pt = {0, 0};
		ClientToScreen(p->hWndParent, &pt);
		OffsetRect(pRect, pt.x, pt.y);
	}
	return TRUE;
}

BOOL GetClientRect(HWND hWnd, LPRECT pRect)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || pRect == NULL) {
		return FALSE;
	}
	RECT rcIn;
	W32GetFrameInsets(p, &rcIn);
	SetRect(pRect, 0, 0,
			std::max(0, (int)(p->rc.right - p->rc.left - rcIn.left - rcIn.right)),
			std::max(0, (int)(p->rc.bottom - p->rc.top - rcIn.top - rcIn.bottom)));
	return TRUE;
}

BOOL ClientToScreen(HWND hWnd, LPPOINT pPt)
{
	W32Wnd *p = W32GetWnd(hWnd);
	while (p && pPt) {
		RECT rcIn;
		W32GetFrameInsets(p, &rcIn);
		pPt->x += p->rc.left + rcIn.left;
		pPt->y += p->rc.top + rcIn.top;
		p = W32GetWnd(p->hWndParent);
	}
	return TRUE;
}

BOOL ScreenToClient(HWND hWnd, LPPOINT pPt)
{
	POINT pt = {0, 0};
	ClientToScreen(hWnd, &pt);
	if (pPt) {
		pPt->x -= pt.x;
		pPt->y -= pt.y;
	}
	return TRUE;
}

BOOL AdjustWindowRect(LPRECT, DWORD, BOOL)
{
	return TRUE;
}

HDWP BeginDeferWindowPos(int)
{
	return (HDWP)1;
}

HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
{
	SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
	return hWinPosInfo;
}

BOOL EndDeferWindowPos(HDWP)
{
	return TRUE;
}

BOOL IsChild(HWND hWndParent, HWND hWnd)
{
	if (W32GetWnd(hWndParent) == NULL) {
		return FALSE;
	}
	for (W32Wnd *p = W32GetWnd(hWnd); p && p->hWndParent; p = W32GetWnd(p->hWndParent)) {
		if (p->hWndParent == hWndParent) {
			return TRUE;
		}
	}
	return FALSE;
}

HWND GetParent(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return p ? p->hWndParent : NULL;
}

HWND SetParent(HWND hWnd, HWND hWndParent)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return NULL;
	}
	HWND hOld = p->hWndParent;
	p->hWndParent = hWndParent;
	return hOld;
}

HWND GetDlgItem(HWND hWnd, int nID)
{
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		if (s_apWnd[i]->hWndParent == hWnd && s_apWnd[i]->nID == nID) {
			return (HWND)s_apWnd[i];
		}
	}
	return NULL;
}

/* 子ウィンドウを順に渡す（Windows と同じく孫も含む。途中で増減しても困らないよう先に一覧を作る） */
BOOL EnumChildWindows(HWND hWndParent, WNDENUMPROC pfn, LPARAM lParam)
{
	if (pfn == NULL) {
		return FALSE;
	}
	std::vector<HWND> aList;
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		W32Wnd *p = s_apWnd[i];
		if (p->bDestroyed || p->hWndParent == NULL) {
			continue;
		}
		if (hWndParent == NULL || IsChild(hWndParent, (HWND)p)) {
			aList.push_back((HWND)p);
		}
	}
	for (size_t i = 0; i < aList.size(); i ++) {
		if (W32GetWnd(aList[i]) == NULL) {
			continue;
		}
		if (!pfn(aList[i], lParam)) {
			break;
		}
	}
	return TRUE;
}

int GetClassName(HWND hWnd, LPSTR pszClass, int nMax)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || pszClass == NULL || nMax <= 0) {
		return 0;
	}
	int nLen = std::min((int)p->strClass.size(), nMax - 1);
	memcpy(pszClass, p->strClass.c_str(), nLen);
	pszClass[nLen] = '\0';
	return nLen;
}

int GetDlgCtrlID(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return p ? p->nID : 0;
}

LONG GetWindowLong(HWND hWnd, int nIndex)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return 0;
	}
	if (p->bDialog) {
		switch (nIndex) {
		case DWL_MSGRESULT:	return (LONG)p->lDlgMsgResult;
		case DWL_DLGPROC:	return (LONG)(LONG_PTR)p->pfnDlgProc;
		case DWL_USER:		return (LONG)p->lDlgUser;
		}
	}
	switch (nIndex) {
	case GWL_WNDPROC:	return (LONG)(LONG_PTR)p->pfnWndProc;
	case GWL_STYLE:		return (LONG)p->dwStyle;
	case GWL_EXSTYLE:	return (LONG)p->dwExStyle;
	case GWL_ID:		return p->nID;
	case GWL_USERDATA:	return p->lUserData;
	}
	return 0;
}

LONG SetWindowLong(HWND hWnd, int nIndex, LONG lValue)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return 0;
	}
	LONG lOld = GetWindowLong(hWnd, nIndex);
	if (p->bDialog) {
		switch (nIndex) {
		case DWL_MSGRESULT:	p->lDlgMsgResult = lValue;					return lOld;
		case DWL_DLGPROC:	p->pfnDlgProc = (DLGPROC)(LONG_PTR)lValue;	return lOld;
		case DWL_USER:		p->lDlgUser = lValue;						return lOld;
		}
	}
	switch (nIndex) {
	case GWL_WNDPROC:	p->pfnWndProc = (WNDPROC)(LONG_PTR)lValue;	break;
	case GWL_STYLE:		p->dwStyle = (DWORD)lValue;					break;
	case GWL_EXSTYLE:	p->dwExStyle = (DWORD)lValue;				break;
	case GWL_ID:		p->nID = (int)lValue;						break;
	case GWL_USERDATA:	p->lUserData = lValue;						break;
	}
	return lOld;
}

DWORD GetClassLong(HWND, int)
{
	return 0;
}

DWORD SetClassLong(HWND, int, LONG)
{
	return 0;
}

BOOL SetWindowText(HWND hWnd, LPCSTR pszText)
{
	return (BOOL)SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)pszText);
}

int GetWindowText(HWND hWnd, LPSTR pszText, int nMax)
{
	return (int)SendMessage(hWnd, WM_GETTEXT, (WPARAM)nMax, (LPARAM)pszText);
}

int GetWindowTextLength(HWND hWnd)
{
	return (int)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
}

HWND SetFocus(HWND hWnd)
{
	HWND hOld = s_hWndFocus;
	W32Wnd *p = W32GetWnd(hWnd);
	if (hWnd && p == NULL) {
		return NULL;
	}
	if (hOld == hWnd) {
		return hOld;
	}
	s_hWndFocus = hWnd;
	if (W32GetWnd(hOld)) {
		SendMessage(hOld, WM_KILLFOCUS, (WPARAM)hWnd, 0);
	}
	if (p) {
		W32HtmlFocus(p->nHtmlID ? p : NULL);
		SendMessage(hWnd, WM_SETFOCUS, (WPARAM)hOld, 0);
	} else {
		W32HtmlFocus(NULL);
	}
	return hOld;
}

HWND GetFocus(void)
{
	return s_hWndFocus;
}

HWND SetActiveWindow(HWND hWnd)
{
	HWND hOld = s_hWndActive;
	s_hWndActive = hWnd;
	return hOld;
}

HWND GetActiveWindow(void)
{
	return s_hWndActive;
}

BOOL SetForegroundWindow(HWND hWnd)
{
	SetActiveWindow(hWnd);
	return TRUE;
}

HWND GetForegroundWindow(void)
{
	return s_hWndActive;
}

HWND SetCapture(HWND hWnd)
{
	HWND hOld = s_hWndCapture;
	s_hWndCapture = hWnd;
	return hOld;
}

BOOL ReleaseCapture(void)
{
	s_hWndCapture = NULL;
	return TRUE;
}

HWND GetCapture(void)
{
	return s_hWndCapture;
}

HWND FindWindow(LPCSTR pszClass, LPCSTR pszName)
{
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		W32Wnd *p = s_apWnd[i];
		if (pszClass && ToLower(pszClass) != ToLower(p->strClass.c_str())) {
			continue;
		}
		if (pszName && p->strText != pszName) {
			continue;
		}
		return (HWND)p;
	}
	return NULL;
}

BOOL BringWindowToTop(HWND)
{
	return TRUE;
}

BOOL FlashWindow(HWND, BOOL)
{
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* メッセージ																 */
/* ------------------------------------------------------------------------- */
LRESULT DefWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	W32Wnd *p = W32GetWnd(hWnd);
	switch (uMsg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_PAINT:
		if (p && W32GetPopupRoot(p)) {
			/* 小窓の中は背景を HTML 側で塗っているので描かなくてよい */
			p->bInvalid = FALSE;
			return 0;
		}
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_SETTEXT:
		if (p) {
			p->strText = lParam ? (const char *)lParam : "";
			W32HtmlSetText(p);
		}
		return TRUE;
	case WM_GETTEXT:
		if (p && wParam > 0 && lParam) {
			size_t n = std::min((size_t)wParam - 1, p->strText.size());
			memcpy((char *)lParam, p->strText.c_str(), n);
			((char *)lParam)[n] = 0;
			return (LRESULT)n;
		}
		return 0;
	case WM_GETTEXTLENGTH:
		return p ? (LRESULT)p->strText.size() : 0;
	case WM_ERASEBKGND:
		return 1;
	}
	return 0;
}

LRESULT CallWindowProc(WNDPROC pfn, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pfn == NULL) {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return pfn(hWnd, uMsg, wParam, lParam);
}

LRESULT SendMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || p->pfnWndProc == NULL) {
		return 0;
	}
#ifdef W32_BROWSER
	if (uMsg == WM_PAINT && p->bScreen && W32PerfEnabled()) {
		double dStart = W32PerfNow();
		LRESULT lRet = p->pfnWndProc(hWnd, uMsg, wParam, lParam);
		W32PerfAdd(W32PERF_PAINT, W32PerfNow() - dStart);
		return lRet;
	}
#endif
	return p->pfnWndProc(hWnd, uMsg, wParam, lParam);
}

BOOL PostMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hWnd && W32GetWnd(hWnd) == NULL) {
		return FALSE;
	}
	MSG msg;
	msg.hwnd	= hWnd;
	msg.message	= uMsg;
	msg.wParam	= wParam;
	msg.lParam	= lParam;
	msg.time	= timeGetTime();
	msg.pt		= s_ptMouse;
	s_queMsg.push_back(msg);
	return TRUE;
}

void PostQuitMessage(int nExitCode)
{
	s_bQuit = TRUE;
	s_nQuitCode = nExitCode;
}

/* 期限の来たタイマーを探す */
static BOOL PickTimer(LPMSG pMsg, BOOL bRemove)
{
	DWORD dwNow = timeGetTime();
	for (size_t i = 0; i < s_aTimer.size(); i ++) {
		W32Timer &t = s_aTimer[i];
		if ((LONG)(dwNow - t.dwNext) >= 0) {
			pMsg->hwnd		= t.hWnd;
			pMsg->message	= WM_TIMER;
			pMsg->wParam	= t.nID;
			pMsg->lParam	= (LPARAM)t.pfn;
			pMsg->time		= dwNow;
			pMsg->pt		= s_ptMouse;
			if (bRemove) {
				t.dwNext = dwNow + std::max(t.uElapse, (UINT)1);
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* 再描画が必要なウィンドウを探す */
static BOOL PickPaint(LPMSG pMsg)
{
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		W32Wnd *p = s_apWnd[i];
		if (p->bInvalid && p->bVisible && p->nKind == W32WND_NORMAL) {
			pMsg->hwnd		= (HWND)p;
			pMsg->message	= WM_PAINT;
			pMsg->wParam	= 0;
			pMsg->lParam	= 0;
			pMsg->time		= timeGetTime();
			pMsg->pt		= s_ptMouse;
			/* WM_PAINT は BeginPaint で無効領域が消えるまで出続けるが、
			   描かないウィンドウで止まらないようにここで消しておく */
			p->bInvalid = FALSE;
			return TRUE;
		}
	}
	return FALSE;
}

/* 終了の依頼（Ctrl+C などのシグナルから。シグナルハンドラでは旗を立てるだけにする） */
static volatile sig_atomic_t s_bCloseRequest;

void W32RequestClose(void)
{
	s_bCloseRequest = 1;
}

/* 旗が立っていたら、トップレベルのウィンドウに WM_CLOSE を送る */
static void CheckCloseRequest(void)
{
	if (!s_bCloseRequest) {
		return;
	}
	s_bCloseRequest = 0;
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		W32Wnd *p = s_apWnd[i];
		if (!p->bDestroyed && p->hWndParent == NULL) {
			PostMessage((HWND)p, WM_CLOSE, 0, 0);
		}
	}
}

static BOOL PeekMessageSub(LPMSG pMsg, UINT uRemove);

BOOL PeekMessage(LPMSG pMsg, HWND hWnd, UINT uMin, UINT uMax, UINT uRemove)
{
	(void)hWnd;
	(void)uMin;
	(void)uMax;
	if (pMsg == NULL) {
		return FALSE;
	}
	CheckCloseRequest();
	W32FlushSurfaces();
	if (PeekMessageSub(pMsg, uRemove)) {
		return TRUE;
	}
	return FALSE;
}

static BOOL PeekMessageSub(LPMSG pMsg, UINT uRemove)
{
	if (!s_queMsg.empty()) {
		*pMsg = s_queMsg.front();
		if (uRemove & PM_REMOVE) {
			s_queMsg.pop_front();
		}
		return TRUE;
	}
	if (s_bQuit) {
		ZeroMemory(pMsg, sizeof(MSG));
		pMsg->message = WM_QUIT;
		pMsg->wParam = (WPARAM)s_nQuitCode;
		if (uRemove & PM_REMOVE) {
			s_bQuit = FALSE;
		}
		return TRUE;
	}
	if (PickTimer(pMsg, (uRemove & PM_REMOVE) ? TRUE : FALSE)) {
		return TRUE;
	}
	if (PickPaint(pMsg)) {
		return TRUE;
	}
	return FALSE;
}

BOOL GetMessage(LPMSG pMsg, HWND hWnd, UINT uMin, UINT uMax)
{
	/* ブラウザでは待てないので、無ければ WM_NULL を返す */
	if (PeekMessage(pMsg, hWnd, uMin, uMax, PM_REMOVE)) {
		return (pMsg->message == WM_QUIT) ? FALSE : TRUE;
	}
	ZeroMemory(pMsg, sizeof(MSG));
	return TRUE;
}

BOOL TranslateMessage(const MSG *pMsg)
{
	if (pMsg == NULL || pMsg->message != WM_KEYDOWN) {
		return FALSE;
	}
	/* 文字の入力そのものは HTML の入力欄が行うので、制御文字だけ WM_CHAR を作る */
	switch (pMsg->wParam) {
	case VK_RETURN:
	case VK_TAB:
	case VK_ESCAPE:
	case VK_BACK:
		{
			MSG msg = *pMsg;
			msg.message = WM_CHAR;
			s_queMsg.push_front(msg);
		}
		return TRUE;
	}
	return FALSE;
}

LRESULT DispatchMessage(const MSG *pMsg)
{
	if (pMsg == NULL) {
		return 0;
	}
	if (pMsg->message == WM_TIMER && pMsg->lParam) {
		((TIMERPROC)pMsg->lParam)(pMsg->hwnd, WM_TIMER, pMsg->wParam, pMsg->time);
		return 0;
	}
	if (pMsg->hwnd == NULL) {
		return 0;
	}
	return SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
}

/* 「ちょっと休憩」を頼まれたミリ秒（-1 なら無し） */
static int s_nYieldMs = -1;

int W32TakeYieldRequest(void)
{
	int nRet = s_nYieldMs;
	s_nYieldMs = -1;
	return nRet;
}

DWORD MsgWaitForMultipleObjects(DWORD, const HANDLE *, BOOL, DWORD dwMilliseconds, DWORD)
{
	/* ブラウザ・Node.js では待てない（メインループ自体が 1 回分ずつ呼ばれている）。
	   待ちたい時間だけ覚えておき、メインループ側でそこで一度戻る */
	int nMs = (dwMilliseconds == INFINITE) ? 1000 : (int)dwMilliseconds;
	if (s_nYieldMs < 0 || nMs < s_nYieldMs) {
		s_nYieldMs = nMs;
	}
	return WAIT_TIMEOUT;
}

BOOL InvalidateRect(HWND hWnd, const RECT *, BOOL)
{
	if (hWnd == NULL) {
		for (size_t i = 0; i < s_apWnd.size(); i ++) {
			s_apWnd[i]->bInvalid = TRUE;
		}
		return TRUE;
	}
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	p->bInvalid = TRUE;
	return TRUE;
}

BOOL ValidateRect(HWND hWnd, const RECT *)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p) {
		p->bInvalid = FALSE;
	}
	return TRUE;
}

UINT_PTR SetTimer(HWND hWnd, UINT_PTR nID, UINT uElapse, TIMERPROC pfn)
{
	for (size_t i = 0; i < s_aTimer.size(); i ++) {
		if (s_aTimer[i].hWnd == hWnd && s_aTimer[i].nID == nID) {
			s_aTimer[i].uElapse = uElapse;
			s_aTimer[i].dwNext = timeGetTime() + uElapse;
			s_aTimer[i].pfn = pfn;
			return nID ? nID : 1;
		}
	}
	W32Timer t;
	t.hWnd		= hWnd;
	t.nID		= nID;
	t.uElapse	= uElapse;
	t.dwNext	= timeGetTime() + uElapse;
	t.pfn		= pfn;
	s_aTimer.push_back(t);
	return nID ? nID : 1;
}

BOOL KillTimer(HWND hWnd, UINT_PTR nID)
{
	for (size_t i = 0; i < s_aTimer.size(); i ++) {
		if (s_aTimer[i].hWnd == hWnd && s_aTimer[i].nID == nID) {
			s_aTimer.erase(s_aTimer.begin() + i);
			return TRUE;
		}
	}
	return FALSE;
}

#ifdef W32_BROWSER
EM_JS(void, W32JsAlert, (const char *pszCaption, const char *pszText), {
	var cap = UTF8ToString(pszCaption);
	var txt = UTF8ToString(pszText);
	console.warn('[MessageBox] ' + cap + ': ' + txt);
	if (typeof Module['onMessageBox'] === 'function') {
		Module['onMessageBox'](cap, txt);
	} else {
		alert(cap + '\n\n' + txt);
	}
});
/* はい/いいえ を聞く（ブラウザの confirm はその場で待てる）。はいなら 1 */
EM_JS(int, W32JsConfirm, (const char *pszCaption, const char *pszText), {
	var cap = UTF8ToString(pszCaption);
	var txt = UTF8ToString(pszText);
	return confirm((cap ? cap + '\n\n' : '') + txt) ? 1 : 0;
});
#else
/* 画面なしではダイアログを出せないのでログに出す */
static void W32JsAlert(const char *pszCaption, const char *pszText)
{
	fprintf(stderr, "[MessageBox] %s: %s\n", pszCaption, pszText);
}
static int W32JsConfirm(const char *pszCaption, const char *pszText)
{
	W32JsAlert(pszCaption, pszText);
	return 1;
}
#endif

int MessageBox(HWND, LPCSTR pszText, LPCSTR pszCaption, UINT uType)
{
	std::string strText = W32TextToUtf8Str(pszText ? pszText : "");
	std::string strCap = W32TextToUtf8Str(pszCaption ? pszCaption : "");
	/* はい/いいえ・OK/キャンセルは確認のダイアログで聞く（画面なしのサーバーでは「はい」「OK」扱い） */
	if ((uType & 0x0F) == MB_YESNO) {
		return W32JsConfirm(strCap.c_str(), strText.c_str()) ? IDYES : IDNO;
	}
	if ((uType & 0x0F) == MB_OKCANCEL) {
		return W32JsConfirm(strCap.c_str(), strText.c_str()) ? IDOK : IDCANCEL;
	}
	W32JsAlert(strCap.c_str(), strText.c_str());
	return IDOK;
}

BOOL MessageBeep(UINT)
{
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* 入力																		 */
/* ------------------------------------------------------------------------- */
int GetSystemMetrics(int nIndex)
{
	switch (nIndex) {
	case SM_CXSCREEN:	return 640;
	case SM_CYSCREEN:	return 480;
	}
	/* 枠やタイトルバーは無いので 0（CreateWindow の大きさがそのままクライアント領域になる） */
	return 0;
}

short GetKeyState(int nVirtKey)
{
	BYTE b = s_abyKey[nVirtKey & 0xFF];
	return (short)(((b & 0x80) ? 0x8000 : 0) | (b & 0x01));
}

short GetAsyncKeyState(int nVirtKey)
{
	return (s_abyKey[nVirtKey & 0xFF] & 0x80) ? (short)0x8000 : 0;
}

BOOL GetKeyboardState(PBYTE pKeyState)
{
	if (pKeyState == NULL) {
		return FALSE;
	}
	memcpy(pKeyState, s_abyKey, 256);
	return TRUE;
}

BOOL GetCursorPos(LPPOINT pPt)
{
	if (pPt) {
		*pPt = s_ptMouse;
	}
	return TRUE;
}

HCURSOR LoadCursor(HINSTANCE, LPCSTR)		{ return (HCURSOR)1; }
HCURSOR SetCursor(HCURSOR h)				{ return h; }
int ShowCursor(BOOL bShow)					{ return bShow ? 1 : 0; }
HICON LoadIcon(HINSTANCE, LPCSTR)			{ return (HICON)1; }

HIMC ImmGetContext(HWND)					{ return (HIMC)1; }
BOOL ImmReleaseContext(HWND, HIMC)			{ return TRUE; }
BOOL ImmSetOpenStatus(HIMC, BOOL)			{ return TRUE; }
BOOL ImmGetOpenStatus(HIMC)					{ return FALSE; }
/* 入力欄から IME を外す（hImc が NULL）と、その欄は半角英数字記号だけになる */
HIMC ImmAssociateContext(HWND hWnd, HIMC hImc)
{
	W32Wnd *pWnd = W32GetWnd(hWnd);
	if (pWnd && pWnd->nKind == W32WND_EDIT) {
		W32HtmlSetNoIme(pWnd, hImc == NULL);
	}
	return NULL;
}

/* JavaScript から呼ばれる：キーが押された/離された
   nHtmlID は入力欄にフォーカスがある時のその番号（無ければ 0） */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnKey(int nVk, int bDown, int nHtmlID)
{
	nVk &= 0xFF;
	/* 小窓の入力欄でのキーは、その入力欄にだけ渡す（ゲームのキー状態は変えない） */
	W32Wnd *pHtml = W32FindWndByHtmlID(nHtmlID);
	if (pHtml && W32GetPopupRoot(pHtml)) {
		PostMessage((HWND)pHtml, bDown ? WM_KEYDOWN : WM_KEYUP, (WPARAM)nVk, 1);
		return;
	}
	if (bDown) {
		if (!(s_abyKey[nVk] & 0x80)) {
			s_abyKey[nVk] ^= 0x01;
		}
		s_abyKey[nVk] |= 0x80;
	} else {
		s_abyKey[nVk] &= ~0x80;
	}
	/* 左右のシフト・コントロールもまとめて押下扱いにする */
	if (nVk == VK_LSHIFT || nVk == VK_RSHIFT) {
		s_abyKey[VK_SHIFT] = (s_abyKey[VK_SHIFT] & ~0x80) | (bDown ? 0x80 : 0);
	}
	if (nVk == VK_LCONTROL || nVk == VK_RCONTROL) {
		s_abyKey[VK_CONTROL] = (s_abyKey[VK_CONTROL] & ~0x80) | (bDown ? 0x80 : 0);
	}

	HWND hWndTarget = s_hWndFocus;
	if (nHtmlID) {
		for (size_t i = 0; i < s_apWnd.size(); i ++) {
			if (s_apWnd[i]->nHtmlID == nHtmlID) {
				hWndTarget = (HWND)s_apWnd[i];
				s_hWndFocus = hWndTarget;
				break;
			}
		}
	} else if (W32GetWnd(s_hWndFocus) && W32GetWnd(s_hWndFocus)->nHtmlID) {
		/* 入力欄からフォーカスが外れているのでゲーム画面に戻す */
		s_hWndFocus = s_hWndScreen;
		hWndTarget = s_hWndScreen;
	}
	if (W32GetWnd(hWndTarget) == NULL) {
		hWndTarget = s_hWndScreen;
	}
	PostMessage(hWndTarget, bDown ? WM_KEYDOWN : WM_KEYUP, (WPARAM)nVk, 1);
}

/* JavaScript から呼ばれる：フォーカスを失った時は押下状態を全部戻す */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnActivate(int bActive)
{
	if (!bActive) {
		for (int i = 0; i < 256; i ++) {
			s_abyKey[i] &= ~0x80;
		}
	}
	if (s_hWndScreen) {
		PostMessage(s_hWndScreen, WM_ACTIVATE, bActive ? WA_ACTIVE : WA_INACTIVE, 0);
	}
}

/* JavaScript から呼ばれる：マウス（座標はメインウィンドウのクライアント座標）
   nType 0:移動 1:押した 2:離した 3:ダブルクリック  nButton 0:左 2:右 */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnMouse(int nType, int nButton, int x, int y)
{
	s_ptMouse.x = x;
	s_ptMouse.y = y;
	if (nType == 1) {
		s_abyKey[nButton == 2 ? VK_RBUTTON : VK_LBUTTON] |= 0x80;
	} else if (nType == 2) {
		s_abyKey[nButton == 2 ? VK_RBUTTON : VK_LBUTTON] &= ~0x80;
	}
	HWND hWnd = s_hWndCapture ? s_hWndCapture : s_hWndScreen;
	if (hWnd == NULL) {
		return;
	}
	WPARAM wKey = 0;
	if (s_abyKey[VK_LBUTTON] & 0x80)	wKey |= MK_LBUTTON;
	if (s_abyKey[VK_RBUTTON] & 0x80)	wKey |= MK_RBUTTON;
	if (s_abyKey[VK_SHIFT] & 0x80)		wKey |= MK_SHIFT;
	if (s_abyKey[VK_CONTROL] & 0x80)	wKey |= MK_CONTROL;
	UINT uMsg = WM_MOUSEMOVE;
	if (nType == 1) {
		uMsg = (nButton == 2) ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
		/* ゲーム画面をクリックしたら入力欄からフォーカスを外す */
		if (W32GetWnd(s_hWndFocus) && W32GetWnd(s_hWndFocus)->nHtmlID) {
			s_hWndFocus = s_hWndScreen;
		}
	} else if (nType == 2) {
		uMsg = (nButton == 2) ? WM_RBUTTONUP : WM_LBUTTONUP;
	} else if (nType == 3) {
		uMsg = (nButton == 2) ? WM_RBUTTONDBLCLK : WM_LBUTTONDBLCLK;
	}
	PostMessage(hWnd, uMsg, wKey, MAKELPARAM(x, y));
}

/* JavaScript から呼ばれる：HTML のボタンが押された */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnButton(int nHtmlID)
{
	for (size_t i = 0; i < s_apWnd.size(); i ++) {
		W32Wnd *p = s_apWnd[i];
		if (p->nHtmlID != nHtmlID || p->nKind != W32WND_BUTTON) {
			continue;
		}
		if (!p->bEnabled) {
			return;
		}
		DWORD dwType = p->dwStyle & 0x0F;
		if (dwType == BS_AUTOCHECKBOX) {
			p->nCheck = p->nCheck ? BST_UNCHECKED : BST_CHECKED;
		} else if (dwType == BS_AUTORADIOBUTTON) {
			/* 同じグループ（WS_GROUP から次の WS_GROUP の手前まで）の他のボタンを外す */
			std::vector<W32Wnd *> aSib;
			for (size_t j = 0; j < s_apWnd.size(); j ++) {
				if (s_apWnd[j]->hWndParent == p->hWndParent && !s_apWnd[j]->bDestroyed) {
					aSib.push_back(s_apWnd[j]);
				}
			}
			size_t nSelf = std::find(aSib.begin(), aSib.end(), p) - aSib.begin();
			size_t nTop = nSelf;
			while (nTop > 0 && !(aSib[nTop]->dwStyle & WS_GROUP)) {
				nTop --;
			}
			for (size_t j = nTop; j < aSib.size(); j ++) {
				if (j > nTop && (aSib[j]->dwStyle & WS_GROUP)) {
					break;
				}
				W32Wnd *q = aSib[j];
				if (q->nKind == W32WND_BUTTON && (q->dwStyle & 0x0F) == BS_AUTORADIOBUTTON) {
					q->nCheck = (q == p) ? BST_CHECKED : BST_UNCHECKED;
					W32HtmlUpdate(q);
				}
			}
		}
		W32HtmlUpdate(p);
		if (p->hWndParent) {
			PostMessage(p->hWndParent, WM_COMMAND, MAKEWPARAM(p->nID, BN_CLICKED), (LPARAM)p);
		}
		return;
	}
}

/* ------------------------------------------------------------------------- */
/* 矩形																		 */
/* ------------------------------------------------------------------------- */
BOOL SetRect(LPRECT p, int l, int t, int r, int b)
{
	if (p == NULL) {
		return FALSE;
	}
	p->left = l;
	p->top = t;
	p->right = r;
	p->bottom = b;
	return TRUE;
}

BOOL SetRectEmpty(LPRECT p)
{
	return SetRect(p, 0, 0, 0, 0);
}

BOOL CopyRect(LPRECT pDst, const RECT *pSrc)
{
	if (pDst == NULL || pSrc == NULL) {
		return FALSE;
	}
	*pDst = *pSrc;
	return TRUE;
}

BOOL OffsetRect(LPRECT p, int dx, int dy)
{
	if (p == NULL) {
		return FALSE;
	}
	p->left += dx;
	p->right += dx;
	p->top += dy;
	p->bottom += dy;
	return TRUE;
}

BOOL InflateRect(LPRECT p, int dx, int dy)
{
	if (p == NULL) {
		return FALSE;
	}
	p->left -= dx;
	p->right += dx;
	p->top -= dy;
	p->bottom += dy;
	return TRUE;
}

BOOL IsRectEmpty(const RECT *p)
{
	return (p == NULL || p->left >= p->right || p->top >= p->bottom) ? TRUE : FALSE;
}

BOOL IntersectRect(LPRECT pDst, const RECT *a, const RECT *b)
{
	if (pDst == NULL || a == NULL || b == NULL) {
		return FALSE;
	}
	RECT rc;
	rc.left		= std::max(a->left, b->left);
	rc.top		= std::max(a->top, b->top);
	rc.right	= std::min(a->right, b->right);
	rc.bottom	= std::min(a->bottom, b->bottom);
	if (IsRectEmpty(&rc)) {
		SetRectEmpty(pDst);
		return FALSE;
	}
	*pDst = rc;
	return TRUE;
}

BOOL UnionRect(LPRECT pDst, const RECT *a, const RECT *b)
{
	if (pDst == NULL || a == NULL || b == NULL) {
		return FALSE;
	}
	if (IsRectEmpty(a)) {
		*pDst = *b;
	} else if (IsRectEmpty(b)) {
		*pDst = *a;
	} else {
		RECT rc;
		rc.left		= std::min(a->left, b->left);
		rc.top		= std::min(a->top, b->top);
		rc.right	= std::max(a->right, b->right);
		rc.bottom	= std::max(a->bottom, b->bottom);
		*pDst = rc;
	}
	return !IsRectEmpty(pDst);
}

BOOL PtInRect(const RECT *p, POINT pt)
{
	return (p && pt.x >= p->left && pt.x < p->right && pt.y >= p->top && pt.y < p->bottom) ? TRUE : FALSE;
}

BOOL EqualRect(const RECT *a, const RECT *b)
{
	return (a && b && memcmp(a, b, sizeof(RECT)) == 0) ? TRUE : FALSE;
}

/* ------------------------------------------------------------------------- */
/* 時間																		 */
/* ------------------------------------------------------------------------- */
/* 起動してからのミリ秒（小数あり） */
static double W32NowMs(void)
{
	return emscripten_get_now();
}

/* 1970/1/1 からのミリ秒 */
static double W32DateNowMs(void)
{
	return emscripten_date_now();
}

/* W32BeginTimeCache 〜 W32EndTimeCache の間に返す時刻 */
static BOOL		s_bTimeCache = FALSE;
static DWORD	s_dwTimeCache;

void W32BeginTimeCache(void)
{
	s_dwTimeCache = (DWORD)(unsigned long long)W32NowMs();
	s_bTimeCache = TRUE;
}

void W32EndTimeCache(void)
{
	s_bTimeCache = FALSE;
}

DWORD timeGetTime(void)
{
	if (s_bTimeCache) {
		return s_dwTimeCache;
	}
	return (DWORD)(unsigned long long)W32NowMs();
}

MMRESULT timeBeginPeriod(UINT)	{ return TIMERR_NOERROR; }
MMRESULT timeEndPeriod(UINT)	{ return TIMERR_NOERROR; }

MMRESULT timeGetDevCaps(LPTIMECAPS pTc, UINT)
{
	if (pTc) {
		pTc->wPeriodMin = 1;
		pTc->wPeriodMax = 1000000;
	}
	return TIMERR_NOERROR;
}

DWORD GetTickCount(void)
{
	return timeGetTime();
}

void Sleep(DWORD)
{
	/* ブラウザのメインスレッドは止められないので何もしない */
}

static void TmToSystemTime(const struct tm *t, LPSYSTEMTIME pSt, int nMs)
{
	pSt->wYear			= (WORD)(t->tm_year + 1900);
	pSt->wMonth			= (WORD)(t->tm_mon + 1);
	pSt->wDayOfWeek		= (WORD)t->tm_wday;
	pSt->wDay			= (WORD)t->tm_mday;
	pSt->wHour			= (WORD)t->tm_hour;
	pSt->wMinute		= (WORD)t->tm_min;
	pSt->wSecond		= (WORD)t->tm_sec;
	pSt->wMilliseconds	= (WORD)nMs;
}

void GetLocalTime(LPSYSTEMTIME pSt)
{
	if (pSt == NULL) {
		return;
	}
	double dNow = W32DateNowMs();
	time_t t = (time_t)(dNow / 1000.0);
	struct tm tmLocal;
	localtime_r(&t, &tmLocal);
	TmToSystemTime(&tmLocal, pSt, (int)fmod(dNow, 1000.0));
}

void GetSystemTime(LPSYSTEMTIME pSt)
{
	if (pSt == NULL) {
		return;
	}
	double dNow = W32DateNowMs();
	time_t t = (time_t)(dNow / 1000.0);
	struct tm tmUtc;
	gmtime_r(&t, &tmUtc);
	TmToSystemTime(&tmUtc, pSt, (int)fmod(dNow, 1000.0));
}

BOOL QueryPerformanceCounter(LARGE_INTEGER *p)
{
	if (p) {
		p->QuadPart = (LONGLONG)(W32NowMs() * 1000.0);
	}
	return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER *p)
{
	if (p) {
		p->QuadPart = 1000000;
	}
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* プロセス・同期（シングルスレッドなので形だけ）							 */
/* ------------------------------------------------------------------------- */
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR)
{
	s_dwLastError = 0;
	return (HANDLE)1;
}

BOOL ReleaseMutex(HANDLE)									{ return TRUE; }
uintptr_t _beginthreadex(void *, unsigned, unsigned (*)(void *), void *, unsigned, unsigned *)	{ return 0; }
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCSTR)	{ return (HANDLE)1; }
BOOL SetEvent(HANDLE)										{ return TRUE; }
BOOL ResetEvent(HANDLE)										{ return TRUE; }
DWORD WaitForSingleObject(HANDLE, DWORD)					{ return WAIT_OBJECT_0; }
DWORD GetLastError(void)									{ return s_dwLastError; }
void SetLastError(DWORD dwErr)								{ s_dwLastError = dwErr; }
void InitializeCriticalSection(LPCRITICAL_SECTION)			{}
void DeleteCriticalSection(LPCRITICAL_SECTION)				{}
void EnterCriticalSection(LPCRITICAL_SECTION)				{}
void LeaveCriticalSection(LPCRITICAL_SECTION)				{}
DWORD GetCurrentThreadId(void)								{ return 1; }
DWORD GetCurrentProcessId(void)								{ return 1; }
BOOL IsDebuggerPresent(void)								{ return FALSE; }
HRESULT CoInitialize(LPVOID)								{ return S_OK; }
HRESULT CoInitializeEx(LPVOID, DWORD)						{ return S_OK; }
void CoUninitialize(void)									{}

void OutputDebugString(LPCSTR pszText)
{
	if (pszText) {
		printf("%s", W32TextToUtf8Str(pszText).c_str());
	}
}

/* ------------------------------------------------------------------------- */
/* 文字列の MS 拡張															 */
/* ------------------------------------------------------------------------- */
char *_strlwr(char *s)
{
	for (char *p = s; p && *p; p ++) {
		if (IsDBCSLeadByte((BYTE)*p) && p[1]) {
			p ++;
			continue;
		}
		*p = (char)tolower((unsigned char)*p);
	}
	return s;
}

char *_strupr(char *s)
{
	for (char *p = s; p && *p; p ++) {
		if (IsDBCSLeadByte((BYTE)*p) && p[1]) {
			p ++;
			continue;
		}
		*p = (char)toupper((unsigned char)*p);
	}
	return s;
}

static char *ItoaCore(unsigned long v, BOOL bNeg, char *str, int radix)
{
	char buf[40];
	int n = 0;
	if (radix < 2 || radix > 36) {
		radix = 10;
	}
	do {
		int d = (int)(v % (unsigned long)radix);
		buf[n ++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
		v /= (unsigned long)radix;
	} while (v);
	char *p = str;
	if (bNeg) {
		*p ++ = '-';
	}
	while (n) {
		*p ++ = buf[-- n];
	}
	*p = 0;
	return str;
}

char *_itoa(int value, char *str, int radix)
{
	if (radix == 10 && value < 0) {
		return ItoaCore((unsigned long)(-(long)value), TRUE, str, radix);
	}
	return ItoaCore((unsigned int)value, FALSE, str, radix);
}

char *_ltoa(long value, char *str, int radix)
{
	if (radix == 10 && value < 0) {
		return ItoaCore((unsigned long)(-value), TRUE, str, radix);
	}
	return ItoaCore((unsigned long)value, FALSE, str, radix);
}

char *_ultoa(unsigned long value, char *str, int radix)
{
	return ItoaCore(value, FALSE, str, radix);
}


/* ------------------------------------------------------------------------- */
/* winsock の初期化（実際の通信は CUraraSockTCP の Web 実装が行う）			 */
/* ------------------------------------------------------------------------- */
#include "include/winsock2.h"

int WSAStartup(WORD wVersion, LPWSADATA pData)
{
	if (pData) {
		ZeroMemory(pData, sizeof(WSADATA));
		pData->wVersion = wVersion;
		pData->wHighVersion = wVersion;
	}
	return 0;
}

int WSACleanup(void)		{ return 0; }
int WSAGetLastError(void)	{ return 0; }
