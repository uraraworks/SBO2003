/* ========================================================================= */
/* ファイル名：	w32_dlg.cpp													 */
/* 概要：		Win32 互換レイヤ：ダイアログ・メニュー・小窓の操作			 */
/* ========================================================================= */
/*
 * 仕組み
 *  - ダイアログの定義は gen_resources.py が .rc から作った表（g_aW32DlgEntry）にある。
 *    CreateDialog はその表どおりにウィンドウとコントロールを作り、WM_INITDIALOG を送る。
 *  - 大きさの単位（ダイアログ単位）は「ＭＳ Ｐゴシック 9pt」と同じ 横 6/4・縦 12/8 で
 *    ピクセルに直す。
 *  - メニューも表（g_aW32MenuEntry）から作り、小窓の上にメニューバーとして出す。
 *  - 小窓での操作（マウス・メニュー・閉じるボタン・移動）は W32JsOn* で受け取る。
 */
#include "w32_internal.h"
#include "include/w32_resources.h"

#include <algorithm>

#include <emscripten.h>

/* ------------------------------------------------------------------------- */
/* 表を引く																	 */
/* ------------------------------------------------------------------------- */
static const W32DlgEntry *FindDialog(LPCSTR pszTemplate)
{
	for (const W32DlgEntry *p = g_aW32DlgEntry; p->pszName; p ++) {
		if (IS_INTRESOURCE(pszTemplate)) {
			if (p->nID == (int)(ULONG_PTR)pszTemplate) {
				return p;
			}
		} else if (_stricmp(p->pszName, pszTemplate) == 0) {
			return p;
		}
	}
	return NULL;
}

static const W32MenuEntry *FindMenu(LPCSTR pszName)
{
	for (const W32MenuEntry *p = g_aW32MenuEntry; p->pszName; p ++) {
		if (IS_INTRESOURCE(pszName)) {
			if (p->nID != 0 && p->nID == (int)(ULONG_PTR)pszName) {
				return p;
			}
		} else if (_stricmp(p->pszName, pszName) == 0) {
			return p;
		}
	}
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* ダイアログ																 */
/* ------------------------------------------------------------------------- */
LONG GetDialogBaseUnits(void)
{
	return MAKELONG(6, 12);
}

BOOL MapDialogRect(HWND, LPRECT pRect)
{
	if (pRect == NULL) {
		return FALSE;
	}
	pRect->left		= W32DLU_X(pRect->left);
	pRect->right	= W32DLU_X(pRect->right);
	pRect->top		= W32DLU_Y(pRect->top);
	pRect->bottom	= W32DLU_Y(pRect->bottom);
	return TRUE;
}

HWND CreateDialogParam(HINSTANCE hInst, LPCSTR pszTemplate, HWND hWndParent, DLGPROC pfnDlg, LPARAM lParam)
{
	const W32DlgEntry *pTpl = FindDialog(pszTemplate);
	if (pTpl == NULL) {
		if (IS_INTRESOURCE(pszTemplate)) {
			printf("[w32] CreateDialog: ダイアログ %d がありません\n", (int)(ULONG_PTR)pszTemplate);
		} else {
			printf("[w32] CreateDialog: ダイアログ %s がありません\n", pszTemplate);
		}
		return NULL;
	}
	DWORD dwStyle = (DWORD)pTpl->dwStyle;
	BOOL bChild = (dwStyle & WS_CHILD) ? TRUE : FALSE;
	RECT rc;
	SetRect(&rc, pTpl->x, pTpl->y, pTpl->x + pTpl->cx, pTpl->y + pTpl->cy);
	MapDialogRect(NULL, &rc);
	int x = rc.left;
	int y = rc.top;
	int cx = rc.right - rc.left;
	int cy = rc.bottom - rc.top;
	if (!bChild) {
		/* 位置は持ち主のクライアント領域から。大きさはクライアント領域なので枠の分を足す */
		POINT pt = {x, y};
		if (hWndParent) {
			ClientToScreen(hWndParent, &pt);
		}
		x = pt.x;
		y = pt.y;
		cx += 3 + 3;
		cy += 3 + 19 + 3;
		if (dwStyle & DS_CENTER) {
			RECT rcOwner;
			if (hWndParent && GetWindowRect(hWndParent, &rcOwner)) {
				x = (rcOwner.left + rcOwner.right - cx) / 2;
				y = (rcOwner.top + rcOwner.bottom - cy) / 2;
			}
		}
	}

	W32SetNextDlgProc(pfnDlg);
	HWND hDlg = CreateWindowEx((DWORD)pTpl->dwExStyle, WC_DIALOG, pTpl->pszCaption,
			dwStyle & ~WS_VISIBLE, x, y, cx, cy, hWndParent, NULL, hInst, NULL);
	if (hDlg == NULL) {
		return NULL;
	}

	/* コントロール */
	HWND hWndFirst = NULL;
	for (const W32DlgItem *pItem = pTpl->pItems; pItem->pszClass; pItem ++) {
		RECT rcItem;
		SetRect(&rcItem, pItem->x, pItem->y, pItem->x + pItem->cx, pItem->y + pItem->cy);
		MapDialogRect(hDlg, &rcItem);
		HWND hCtl = CreateWindowEx((DWORD)pItem->dwExStyle, pItem->pszClass, pItem->pszText,
				(DWORD)pItem->dwStyle, rcItem.left, rcItem.top, rcItem.right - rcItem.left, rcItem.bottom - rcItem.top,
				hDlg, (HMENU)(INT_PTR)pItem->nID, hInst, NULL);
		if (hCtl == NULL) {
			printf("[w32] CreateDialog: コントロール %s を作れません\n", pItem->pszClass);
			continue;
		}
		if (hWndFirst == NULL && (pItem->dwStyle & WS_TABSTOP)) {
			hWndFirst = hCtl;
		}
	}

	LRESULT lRet = SendMessage(hDlg, WM_INITDIALOG, (WPARAM)hWndFirst, lParam);
	if (lRet && hWndFirst && W32GetWnd(hWndFirst)) {
		SetFocus(hWndFirst);
	}
	if (W32GetWnd(hDlg) && (dwStyle & WS_VISIBLE)) {
		ShowWindow(hDlg, SW_SHOW);
	}
	return W32GetWnd(hDlg) ? hDlg : NULL;
}

INT_PTR DialogBoxParam(HINSTANCE hInst, LPCSTR pszTemplate, HWND hWndParent, DLGPROC pfnDlg, LPARAM lParam)
{
	/* ブラウザでは終わるまで待てないので、モードレスとして出して「キャンセル」を返す */
	printf("[w32] DialogBox: 待てないのでモードレスで表示します\n");
	HWND hDlg = CreateDialogParam(hInst, pszTemplate, hWndParent, pfnDlg, lParam);
	if (hDlg) {
		ShowWindow(hDlg, SW_SHOW);
	}
	return IDCANCEL;
}

BOOL EndDialog(HWND hDlg, INT_PTR)
{
	W32Wnd *p = W32GetWnd(hDlg);
	if (p == NULL) {
		return FALSE;
	}
	if (p->hWndOwner) {
		EnableWindow(p->hWndOwner, TRUE);
	}
	DestroyWindow(hDlg);
	return TRUE;
}

LRESULT DefDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CLOSE:
		/* 閉じるボタンは「キャンセル」として伝える */
		PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, IDCANCEL));
		return 0;
	case WM_ERASEBKGND:
		return 1;
	}
	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

LRESULT CALLBACK W32DialogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p && p->pfnDlgProc) {
		p->lDlgMsgResult = 0;
		LRESULT lRet = p->pfnDlgProc(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_INITDIALOG) {
			return lRet;
		}
		if (lRet) {
			return W32GetWnd(hWnd) ? p->lDlgMsgResult : 0;
		}
		if (W32GetWnd(hWnd) == NULL) {
			return 0;
		}
	}
	return DefDlgProc(hWnd, uMsg, wParam, lParam);
}

BOOL IsDialogMessage(HWND, LPMSG)
{
	return FALSE;
}

UINT GetDlgItemText(HWND hDlg, int nID, LPSTR pszText, int nMax)
{
	return (UINT)GetWindowText(GetDlgItem(hDlg, nID), pszText, nMax);
}

BOOL SetDlgItemText(HWND hDlg, int nID, LPCSTR pszText)
{
	return SetWindowText(GetDlgItem(hDlg, nID), pszText);
}

UINT GetDlgItemInt(HWND hDlg, int nID, BOOL *pbTranslated, BOOL bSigned)
{
	char szBuf[64];
	szBuf[0] = 0;
	GetDlgItemText(hDlg, nID, szBuf, sizeof(szBuf));
	char *pEnd = NULL;
	long lValue = bSigned ? strtol(szBuf, &pEnd, 10) : (long)strtoul(szBuf, &pEnd, 10);
	if (pbTranslated) {
		*pbTranslated = (pEnd && pEnd != szBuf) ? TRUE : FALSE;
	}
	return (UINT)lValue;
}

BOOL SetDlgItemInt(HWND hDlg, int nID, UINT uValue, BOOL bSigned)
{
	char szBuf[32];
	if (bSigned) {
		snprintf(szBuf, sizeof(szBuf), "%d", (int)uValue);
	} else {
		snprintf(szBuf, sizeof(szBuf), "%u", uValue);
	}
	return SetDlgItemText(hDlg, nID, szBuf);
}

LRESULT SendDlgItemMessage(HWND hDlg, int nID, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return SendMessage(GetDlgItem(hDlg, nID), uMsg, wParam, lParam);
}

BOOL CheckDlgButton(HWND hDlg, int nID, UINT uCheck)
{
	SendMessage(GetDlgItem(hDlg, nID), BM_SETCHECK, (WPARAM)uCheck, 0);
	return TRUE;
}

UINT IsDlgButtonChecked(HWND hDlg, int nID)
{
	return (UINT)SendMessage(GetDlgItem(hDlg, nID), BM_GETCHECK, 0, 0);
}

BOOL CheckRadioButton(HWND hDlg, int nFirst, int nLast, int nCheck)
{
	for (int i = nFirst; i <= nLast; i ++) {
		CheckDlgButton(hDlg, i, (i == nCheck) ? BST_CHECKED : BST_UNCHECKED);
	}
	return TRUE;
}

HWND GetNextDlgTabItem(HWND, HWND, BOOL)
{
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* メニュー																	 */
/* ------------------------------------------------------------------------- */
W32Menu *W32GetMenu(HMENU hMenu)
{
	W32Menu *p = (W32Menu *)hMenu;
	return (p && p->dwMagic == W32MENU_MAGIC) ? p : NULL;
}

static W32Menu *NewMenu(void)
{
	W32Menu *p = new W32Menu;
	p->dwMagic = W32MENU_MAGIC;
	p->hWnd = NULL;
	return p;
}

/* 表の pItem から 1 段分を読む（W32MENU_END で戻る） */
static const W32MenuTplItem *BuildMenu(W32Menu *pMenu, const W32MenuTplItem *pItem)
{
	while (pItem->pszText) {
		if (pItem->nKind == W32MENU_END) {
			return pItem + 1;
		}
		W32MenuItem item;
		item.uFlags = 0;
		item.uID = (UINT)pItem->nID;
		item.strText = pItem->pszText;
		item.pSub = NULL;
		if (pItem->nKind == W32MENU_SEPARATOR) {
			item.uFlags = MF_SEPARATOR;
			pItem ++;
		} else if (pItem->nKind == W32MENU_POPUP) {
			item.uFlags = MF_POPUP;
			item.pSub = NewMenu();
			pItem = BuildMenu(item.pSub, pItem + 1);
		} else {
			pItem ++;
		}
		pMenu->aItem.push_back(item);
	}
	return pItem;
}

HMENU LoadMenu(HINSTANCE, LPCSTR pszName)
{
	const W32MenuEntry *pTpl = FindMenu(pszName);
	if (pTpl == NULL) {
		return NULL;
	}
	W32Menu *pMenu = NewMenu();
	BuildMenu(pMenu, pTpl->pItems);
	return (HMENU)pMenu;
}

HMENU CreateMenu(void)
{
	return (HMENU)NewMenu();
}

HMENU CreatePopupMenu(void)
{
	return (HMENU)NewMenu();
}

BOOL DestroyMenu(HMENU hMenu)
{
	/* 使っているウィンドウがあるかもしれないので解放はしない */
	return W32GetMenu(hMenu) ? TRUE : FALSE;
}

static void RefreshMenuBar(W32Menu *pMenu)
{
	W32Wnd *p = pMenu ? W32GetWnd(pMenu->hWnd) : NULL;
	if (p) {
		W32HtmlMenu(p);
	}
}

BOOL AppendMenu(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR pszNewItem)
{
	W32Menu *pMenu = W32GetMenu(hMenu);
	if (pMenu == NULL) {
		return FALSE;
	}
	W32MenuItem item;
	item.uFlags = uFlags;
	item.uID = (uFlags & MF_POPUP) ? 0 : (UINT)uIDNewItem;
	item.pSub = (uFlags & MF_POPUP) ? W32GetMenu((HMENU)uIDNewItem) : NULL;
	item.strText = (pszNewItem && !(uFlags & MF_SEPARATOR)) ? pszNewItem : "";
	pMenu->aItem.push_back(item);
	RefreshMenuBar(pMenu);
	return TRUE;
}

/* 項目を探す（サブメニューの中も） */
static W32MenuItem *FindMenuItem(W32Menu *pMenu, UINT uID, UINT uFlags)
{
	if (pMenu == NULL) {
		return NULL;
	}
	if (uFlags & MF_BYPOSITION) {
		return (uID < pMenu->aItem.size()) ? &pMenu->aItem[uID] : NULL;
	}
	for (size_t i = 0; i < pMenu->aItem.size(); i ++) {
		W32MenuItem &item = pMenu->aItem[i];
		if (!(item.uFlags & (MF_POPUP | MF_SEPARATOR)) && item.uID == uID) {
			return &item;
		}
		if (item.pSub) {
			W32MenuItem *pHit = FindMenuItem(item.pSub, uID, uFlags);
			if (pHit) {
				return pHit;
			}
		}
	}
	return NULL;
}

/* 一番上のメニュー（メニューバーとして付いているもの）を探して表示し直す */
static void RefreshMenuOwner(W32Menu *pMenu)
{
	const std::vector<W32Wnd *> &apWnd = W32GetWndList();
	for (size_t i = 0; i < apWnd.size(); i ++) {
		W32Wnd *p = apWnd[i];
		W32Menu *pBar = W32GetMenu(p->hMenu);
		if (pBar == NULL || !p->bPopup) {
			continue;
		}
		if (pBar == pMenu) {
			W32HtmlMenu(p);
			continue;
		}
		for (size_t j = 0; j < pBar->aItem.size(); j ++) {
			if (pBar->aItem[j].pSub == pMenu) {
				W32HtmlMenu(p);
			}
		}
	}
}

BOOL CheckMenuItem(HMENU hMenu, UINT uID, UINT uCheck)
{
	W32Menu *pMenu = W32GetMenu(hMenu);
	W32MenuItem *pItem = FindMenuItem(pMenu, uID, uCheck);
	if (pItem == NULL) {
		return (BOOL)-1;
	}
	BOOL bOld = (pItem->uFlags & MF_CHECKED) ? MF_CHECKED : MF_UNCHECKED;
	pItem->uFlags = (pItem->uFlags & ~MF_CHECKED) | (uCheck & MF_CHECKED);
	RefreshMenuOwner(pMenu);
	return bOld;
}

BOOL EnableMenuItem(HMENU hMenu, UINT uID, UINT uEnable)
{
	W32Menu *pMenu = W32GetMenu(hMenu);
	W32MenuItem *pItem = FindMenuItem(pMenu, uID, uEnable);
	if (pItem == NULL) {
		return (BOOL)-1;
	}
	BOOL bOld = pItem->uFlags & (MF_GRAYED | MF_DISABLED);
	pItem->uFlags = (pItem->uFlags & ~(MF_GRAYED | MF_DISABLED)) | (uEnable & (MF_GRAYED | MF_DISABLED));
	RefreshMenuOwner(pMenu);
	return bOld;
}

UINT GetMenuState(HMENU hMenu, UINT uID, UINT uFlags)
{
	W32MenuItem *pItem = FindMenuItem(W32GetMenu(hMenu), uID, uFlags);
	return pItem ? (pItem->uFlags & (MF_CHECKED | MF_GRAYED | MF_DISABLED | MF_SEPARATOR | MF_POPUP)) : (UINT)-1;
}

HMENU GetSubMenu(HMENU hMenu, int nPos)
{
	W32Menu *pMenu = W32GetMenu(hMenu);
	if (pMenu == NULL || nPos < 0 || nPos >= (int)pMenu->aItem.size()) {
		return NULL;
	}
	return (HMENU)pMenu->aItem[nPos].pSub;
}

int GetMenuItemCount(HMENU hMenu)
{
	W32Menu *pMenu = W32GetMenu(hMenu);
	return pMenu ? (int)pMenu->aItem.size() : -1;
}

BOOL SetMenu(HWND hWnd, HMENU hMenu)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	p->hMenu = hMenu;
	W32Menu *pMenu = W32GetMenu(hMenu);
	if (pMenu) {
		pMenu->hWnd = hWnd;
	}
	W32HtmlMenu(p);
	W32HtmlUpdate(p);
	return TRUE;
}

HMENU GetMenu(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return p ? p->hMenu : NULL;
}

BOOL DrawMenuBar(HWND hWnd)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p) {
		W32HtmlMenu(p);
	}
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* 小窓の操作（JavaScript から呼ばれる）										 */
/* ------------------------------------------------------------------------- */
/* マウス（座標はそのウィンドウのクライアント座標）
   nType 0:移動 1:押した 2:離した 3:ダブルクリック  nButton 0:左 2:右 */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnWndMouse(int nHtmlID, int nType, int nButton, int nButtons, int x, int y)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled) {
		return;
	}
	WPARAM wKey = 0;
	if (nButtons & 1)	wKey |= MK_LBUTTON;
	if (nButtons & 2)	wKey |= MK_RBUTTON;
	UINT uMsg = WM_MOUSEMOVE;
	if (nType == 1) {
		uMsg = (nButton == 2) ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
	} else if (nType == 2) {
		uMsg = (nButton == 2) ? WM_RBUTTONUP : WM_LBUTTONUP;
	} else if (nType == 3) {
		uMsg = (nButton == 2) ? WM_RBUTTONDBLCLK : WM_LBUTTONDBLCLK;
	}
	PostMessage((HWND)p, uMsg, wKey, MAKELPARAM(x, y));
}

/* メニューの項目が選ばれた */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnMenu(int nHtmlID, int nCmd)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled) {
		return;
	}
	PostMessage((HWND)p, WM_COMMAND, MAKEWPARAM(nCmd, 0), 0);
}

/* 閉じるボタンが押された */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnClose(int nHtmlID)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled) {
		return;
	}
	PostMessage((HWND)p, WM_CLOSE, 0, 0);
}

/* 小窓が動かされた（座標はメインウィンドウを原点にした画面座標） */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnFrameMove(int nHtmlID, int x, int y)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL) {
		return;
	}
	int cx = p->rc.right - p->rc.left;
	int cy = p->rc.bottom - p->rc.top;
	SetRect(&p->rc, x, y, x + cx, y + cy);
	PostMessage((HWND)p, WM_MOVE, 0, MAKELPARAM(x, y));
}

/* 小窓の枠を引っぱって大きさを変えた（cx, cy は枠を含めた大きさ） */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnFrameSize(int nHtmlID, int cx, int cy)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bPopup) {
		return;
	}
	if (cx == p->rc.right - p->rc.left && cy == p->rc.bottom - p->rc.top) {
		return;
	}
	p->rc.right = p->rc.left + cx;
	p->rc.bottom = p->rc.top + cy;
	W32HtmlUpdate(p);
	RECT rc;
	GetClientRect((HWND)p, &rc);
	PostMessage((HWND)p, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	/* 自分で描いている内容は、新しい大きさで描き直してもらう */
	InvalidateRect((HWND)p, NULL, TRUE);
}

/* 入力欄の中身が変わった */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnEditChange(int nHtmlID)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || p->hWndParent == NULL) {
		return;
	}
	W32HtmlGetText(p);
	UINT uCode = (p->nKind == W32WND_COMBOBOX) ? CBN_EDITCHANGE : EN_CHANGE;
	PostMessage(p->hWndParent, WM_COMMAND, MAKEWPARAM(p->nID, uCode), (LPARAM)(HWND)p);
}
