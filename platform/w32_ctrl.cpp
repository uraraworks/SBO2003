/* ========================================================================= */
/* ファイル名：	w32_ctrl.cpp												 */
/* 概要：		Win32 互換レイヤ：コンボボックス・リストボックス・			 */
/*				スクロールバー・リストビュー								 */
/* ========================================================================= */
/*
 * 仕組み
 *  - 項目や選択状態などの中身は C++ 側（W32Wnd）で持ち、変わったら
 *    W32Html* で HTML の部品へ知らせる。HTML 側で操作されたときは
 *    W32JsOn* が呼ばれ、ここで状態を変えてから親へ通知を送る。
 *  - 画面なし（サーバー）でも中身はそのまま動く（W32Html* が何もしないだけ）。
 */
#include "w32_internal.h"
#include "include/commctrl.h"

#include <algorithm>

#include <emscripten.h>

/* ------------------------------------------------------------------------- */
/* 色																		 */
/* ------------------------------------------------------------------------- */
DWORD GetSysColor(int nIndex)
{
	switch (nIndex) {
	case COLOR_WINDOW:		return RGB(255, 255, 255);
	case COLOR_WINDOWTEXT:	return RGB(0, 0, 0);
	case COLOR_BTNTEXT:		return RGB(0, 0, 0);
	case COLOR_GRAYTEXT:	return RGB(109, 109, 109);
	case COLOR_BTNSHADOW:	return RGB(160, 160, 160);
	case COLOR_BTNHIGHLIGHT:	return RGB(255, 255, 255);
	case COLOR_BACKGROUND:	return RGB(0, 0, 0);
	}
	return RGB(240, 240, 240);		/* COLOR_BTNFACE など */
}

HBRUSH GetSysColorBrush(int nIndex)
{
	/* 色ごとに 1 つ作って使い回す（削除されないよう印を付ける） */
	static HBRUSH s_ahBrush[32];
	nIndex &= 31;
	if (s_ahBrush[nIndex] == NULL) {
		s_ahBrush[nIndex] = CreateSolidBrush(GetSysColor(nIndex));
		W32GdiObj *p = W32GetGdi(s_ahBrush[nIndex]);
		if (p) {
			p->bStock = TRUE;
		}
	}
	return s_ahBrush[nIndex];
}

void InitCommonControls(void)
{
}

/* ------------------------------------------------------------------------- */
/* 共通																		 */
/* ------------------------------------------------------------------------- */
/* 親へ WM_COMMAND の通知を送る */
static void NotifyCommand(W32Wnd *p, UINT uCode)
{
	if (p->hWndParent) {
		PostMessage(p->hWndParent, WM_COMMAND, MAKEWPARAM(p->nID, uCode), (LPARAM)(HWND)p);
	}
}

static LRESULT CopyText(const std::string &str, LPARAM lParam)
{
	if (lParam) {
		memcpy((char *)lParam, str.c_str(), str.size() + 1);
	}
	return (LRESULT)str.size();
}

/* 前方一致・完全一致で探す（大文字小文字は区別しない） */
static int FindItem(W32Wnd *p, int nStart, const char *psz, BOOL bExact)
{
	if (psz == NULL) {
		return -1;
	}
	size_t nLen = strlen(psz);
	int nCount = (int)p->aItems.size();
	for (int n = 0; n < nCount; n ++) {
		int i = (nStart + 1 + n) % nCount;
		const std::string &s = p->aItems[i];
		if (bExact ? (_stricmp(s.c_str(), psz) == 0) : (_strnicmp(s.c_str(), psz, nLen) == 0)) {
			return i;
		}
	}
	return -1;
}

/* ------------------------------------------------------------------------- */
/* コンボボックス・リストボックス											 */
/* ------------------------------------------------------------------------- */
static LRESULT ListProc(W32Wnd *p, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bCombo = (p->nKind == W32WND_COMBOBOX);
	int nErr = bCombo ? CB_ERR : LB_ERR;
	/* LB_* を CB_* と同じ扱いにそろえる */
	UINT m = uMsg;
	if (!bCombo) {
		switch (uMsg) {
		case LB_ADDSTRING:		m = CB_ADDSTRING;		break;
		case LB_INSERTSTRING:	m = CB_INSERTSTRING;	break;
		case LB_DELETESTRING:	m = CB_DELETESTRING;	break;
		case LB_RESETCONTENT:	m = CB_RESETCONTENT;	break;
		case LB_GETCOUNT:		m = CB_GETCOUNT;		break;
		case LB_GETCURSEL:		m = CB_GETCURSEL;		break;
		case LB_SETCURSEL:		m = CB_SETCURSEL;		break;
		case LB_GETTEXT:		m = CB_GETLBTEXT;		break;
		case LB_GETTEXTLEN:		m = CB_GETLBTEXTLEN;	break;
		case LB_GETITEMDATA:	m = CB_GETITEMDATA;		break;
		case LB_SETITEMDATA:	m = CB_SETITEMDATA;		break;
		case LB_FINDSTRING:		m = CB_FINDSTRING;		break;
		case LB_FINDSTRINGEXACT:	m = CB_FINDSTRINGEXACT;	break;
		case LB_SELECTSTRING:	m = CB_SELECTSTRING;	break;
		case LB_GETSEL:
			return ((int)wParam == p->nCurSel) ? 1 : 0;
		case LB_SETSEL:
			if (wParam) {
				p->nCurSel = (int)lParam;
				W32HtmlItems(p, 3, p->nCurSel, NULL);
			}
			return 0;
		case LB_GETSELCOUNT:
			return (p->nCurSel >= 0) ? 1 : 0;
		case LB_SETTOPINDEX:
		case LB_GETTOPINDEX:
		case LB_SETHORIZONTALEXTENT:
		case LB_SETITEMHEIGHT:
		case LB_SETTABSTOPS:
			return 0;
		}
	}
	int nCount = (int)p->aItems.size();
	switch (m) {
	case CB_ADDSTRING:
	case CB_INSERTSTRING:
		{
			const char *psz = lParam ? (const char *)lParam : "";
			int nIndex = (m == CB_ADDSTRING) ? -1 : (int)wParam;
			if (m == CB_ADDSTRING && (p->dwStyle & (bCombo ? CBS_SORT : LBS_SORT))) {
				nIndex = 0;
				while (nIndex < nCount && _stricmp(p->aItems[nIndex].c_str(), psz) <= 0) {
					nIndex ++;
				}
			}
			if (nIndex < 0 || nIndex > nCount) {
				nIndex = nCount;
			}
			p->aItems.insert(p->aItems.begin() + nIndex, psz);
			p->aItemData.insert(p->aItemData.begin() + nIndex, 0);
			if (p->nCurSel >= nIndex) {
				p->nCurSel ++;
			}
			W32HtmlItems(p, 1, nIndex, psz);
			return nIndex;
		}
	case CB_DELETESTRING:
		{
			int nIndex = (int)wParam;
			if (nIndex < 0 || nIndex >= nCount) {
				return nErr;
			}
			p->aItems.erase(p->aItems.begin() + nIndex);
			p->aItemData.erase(p->aItemData.begin() + nIndex);
			if (p->nCurSel == nIndex) {
				p->nCurSel = -1;
			} else if (p->nCurSel > nIndex) {
				p->nCurSel --;
			}
			W32HtmlItems(p, 2, nIndex, NULL);
			W32HtmlItems(p, 3, p->nCurSel, NULL);
			return (LRESULT)p->aItems.size();
		}
	case CB_RESETCONTENT:
		p->aItems.clear();
		p->aItemData.clear();
		p->nCurSel = -1;
		W32HtmlItems(p, 0, 0, NULL);
		return 0;
	case CB_GETCOUNT:
		return nCount;
	case CB_GETCURSEL:
		return p->nCurSel;
	case CB_SETCURSEL:
		{
			int nIndex = (int)wParam;
			if (nIndex < 0 || nIndex >= nCount) {
				p->nCurSel = -1;
				W32HtmlItems(p, 3, -1, NULL);
				return nErr;
			}
			p->nCurSel = nIndex;
			W32HtmlItems(p, 3, nIndex, NULL);
			return nIndex;
		}
	case CB_GETLBTEXT:
		if ((int)wParam < 0 || (int)wParam >= nCount) {
			return nErr;
		}
		return CopyText(p->aItems[wParam], lParam);
	case CB_GETLBTEXTLEN:
		if ((int)wParam < 0 || (int)wParam >= nCount) {
			return nErr;
		}
		return (LRESULT)p->aItems[wParam].size();
	case CB_GETITEMDATA:
		if ((int)wParam < 0 || (int)wParam >= nCount) {
			return nErr;
		}
		return (LRESULT)p->aItemData[wParam];
	case CB_SETITEMDATA:
		if ((int)wParam < 0 || (int)wParam >= nCount) {
			return nErr;
		}
		p->aItemData[wParam] = lParam;
		return 0;
	case CB_FINDSTRING:
	case CB_FINDSTRINGEXACT:
		return FindItem(p, (int)wParam, (const char *)lParam, m == CB_FINDSTRINGEXACT);
	case CB_SELECTSTRING:
		{
			int nIndex = FindItem(p, (int)wParam, (const char *)lParam, FALSE);
			if (nIndex >= 0) {
				p->nCurSel = nIndex;
				W32HtmlItems(p, 3, nIndex, NULL);
			}
			return (nIndex >= 0) ? nIndex : nErr;
		}
	case CB_GETDROPPEDSTATE:
	case CB_SHOWDROPDOWN:
	case CB_SETDROPPEDWIDTH:
	case CB_GETDROPPEDWIDTH:
	case CB_SETITEMHEIGHT:
	case CB_LIMITTEXT:
	case CB_SETEXTENDEDUI:
	case CB_INITSTORAGE:
	case CB_SETEDITSEL:
		return 0;
	case WM_SETTEXT:
		p->strText = lParam ? (const char *)lParam : "";
		return TRUE;
	case WM_GETTEXT:
		{
			/* 選んでいる項目の文字（無ければ自分の文字） */
			std::string str = (p->nCurSel >= 0 && p->nCurSel < nCount) ? p->aItems[p->nCurSel] : p->strText;
			if (wParam == 0 || lParam == 0) {
				return 0;
			}
			size_t n = std::min((size_t)wParam - 1, str.size());
			memcpy((char *)lParam, str.c_str(), n);
			((char *)lParam)[n] = 0;
			return (LRESULT)n;
		}
	case WM_GETTEXTLENGTH:
		return (p->nCurSel >= 0 && p->nCurSel < nCount) ? (LRESULT)p->aItems[p->nCurSel].size() : (LRESULT)p->strText.size();
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* JavaScript から呼ばれる：コンボボックス・リストボックスで選ばれた */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnSelect(int nHtmlID, int nIndex, int bDblClick)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled) {
		return;
	}
	if (nIndex >= (int)p->aItems.size()) {
		nIndex = -1;
	}
	BOOL bChanged = (p->nCurSel != nIndex);
	p->nCurSel = nIndex;
	if (p->nKind == W32WND_COMBOBOX) {
		if (bChanged) {
			NotifyCommand(p, CBN_SELCHANGE);
		}
		NotifyCommand(p, CBN_SELENDOK);
	} else if (p->dwStyle & LBS_NOTIFY) {
		NotifyCommand(p, bDblClick ? LBN_DBLCLK : LBN_SELCHANGE);
	}
}

/* ------------------------------------------------------------------------- */
/* スクロールバー															 */
/* ------------------------------------------------------------------------- */
static int ClampScrollPos(W32Wnd *p, int nPos)
{
	int nMax = p->nScrMax;
	if (p->nScrPage > 0) {
		nMax = std::max(p->nScrMin, p->nScrMax - (int)p->nScrPage + 1);
	}
	return std::max(p->nScrMin, std::min(nMax, nPos));
}

int SetScrollPos(HWND hWnd, int, int nPos, BOOL)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return 0;
	}
	int nOld = p->nScrPos;
	p->nScrPos = ClampScrollPos(p, nPos);
	W32HtmlScroll(p);
	return nOld;
}

int GetScrollPos(HWND hWnd, int)
{
	W32Wnd *p = W32GetWnd(hWnd);
	return p ? p->nScrPos : 0;
}

BOOL SetScrollRange(HWND hWnd, int, int nMin, int nMax, BOOL)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	p->nScrMin = nMin;
	p->nScrMax = std::max(nMin, nMax);
	p->nScrPos = ClampScrollPos(p, p->nScrPos);
	W32HtmlScroll(p);
	return TRUE;
}

BOOL GetScrollRange(HWND hWnd, int, LPINT pnMin, LPINT pnMax)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return FALSE;
	}
	if (pnMin) {
		*pnMin = p->nScrMin;
	}
	if (pnMax) {
		*pnMax = p->nScrMax;
	}
	return TRUE;
}

int SetScrollInfo(HWND hWnd, int, LPCSCROLLINFO pSi, BOOL)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || pSi == NULL) {
		return 0;
	}
	if (pSi->fMask & SIF_RANGE) {
		p->nScrMin = pSi->nMin;
		p->nScrMax = std::max(pSi->nMin, pSi->nMax);
	}
	if (pSi->fMask & SIF_PAGE) {
		p->nScrPage = pSi->nPage;
	}
	if (pSi->fMask & SIF_POS) {
		p->nScrPos = pSi->nPos;
	}
	p->nScrPos = ClampScrollPos(p, p->nScrPos);
	W32HtmlScroll(p);
	return p->nScrPos;
}

BOOL GetScrollInfo(HWND hWnd, int, LPSCROLLINFO pSi)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL || pSi == NULL) {
		return FALSE;
	}
	if (pSi->fMask & SIF_RANGE) {
		pSi->nMin = p->nScrMin;
		pSi->nMax = p->nScrMax;
	}
	if (pSi->fMask & SIF_PAGE) {
		pSi->nPage = p->nScrPage;
	}
	if (pSi->fMask & SIF_POS) {
		pSi->nPos = p->nScrPos;
	}
	if (pSi->fMask & SIF_TRACKPOS) {
		pSi->nTrackPos = p->nScrPos;
	}
	return TRUE;
}

BOOL ShowScrollBar(HWND, int, BOOL)		{ return TRUE; }
BOOL EnableScrollBar(HWND, UINT, UINT)	{ return TRUE; }

static LRESULT ScrollProc(W32Wnd *p, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case SBM_SETPOS:
		return SetScrollPos(hWnd, SB_CTL, (int)wParam, (BOOL)lParam);
	case SBM_GETPOS:
		return p->nScrPos;
	case SBM_SETRANGE:
	case SBM_SETRANGEREDRAW:
		SetScrollRange(hWnd, SB_CTL, (int)wParam, (int)lParam, TRUE);
		return 0;
	case SBM_GETRANGE:
		GetScrollRange(hWnd, SB_CTL, (LPINT)wParam, (LPINT)lParam);
		return 0;
	case SBM_SETSCROLLINFO:
		return SetScrollInfo(hWnd, SB_CTL, (LPCSCROLLINFO)lParam, (BOOL)wParam);
	case SBM_GETSCROLLINFO:
		return GetScrollInfo(hWnd, SB_CTL, (LPSCROLLINFO)lParam);
	case SBM_ENABLE_ARROWS:
		return TRUE;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* JavaScript から呼ばれる：スクロールバーが動かされた（nCode は SB_*） */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnScroll(int nHtmlID, int nCode, int nPos)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled || p->hWndParent == NULL) {
		return;
	}
	nPos = ClampScrollPos(p, nPos);
	UINT uMsg = (p->dwStyle & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL;
	PostMessage(p->hWndParent, uMsg, MAKEWPARAM(nCode, (WORD)nPos), (LPARAM)(HWND)p);
}

/* ------------------------------------------------------------------------- */
/* リストビュー																 */
/* ------------------------------------------------------------------------- */
/* 通知は PostMessage で送るので、中身を置いておく場所を回して使う */
static NMLISTVIEW *NewNotify(W32Wnd *p, UINT uCode, int nItem)
{
	static NMLISTVIEW s_aNm[32];
	static int s_nNext;
	NMLISTVIEW *pNm = &s_aNm[s_nNext];
	s_nNext = (s_nNext + 1) % 32;
	memset(pNm, 0, sizeof(NMLISTVIEW));
	pNm->hdr.hwndFrom	= (HWND)p;
	pNm->hdr.idFrom		= (UINT_PTR)p->nID;
	pNm->hdr.code		= uCode;
	pNm->iItem			= nItem;
	pNm->iSubItem		= 0;
	if (nItem >= 0 && nItem < (int)p->aRows.size()) {
		pNm->lParam = p->aRows[nItem].lParam;
	}
	return pNm;
}

static void PostNotify(W32Wnd *p, NMLISTVIEW *pNm)
{
	if (p->hWndParent) {
		PostMessage(p->hWndParent, WM_NOTIFY, (WPARAM)p->nID, (LPARAM)pNm);
	}
}

static void SetRowState(W32Wnd *p, int nRow, UINT uState, UINT uMask, BOOL bNotify)
{
	if (nRow < 0 || nRow >= (int)p->aRows.size()) {
		return;
	}
	W32LvRow &r = p->aRows[nRow];
	UINT uOld = r.uState;
	/* 一つしか選べないなら、ほかの選択を外す */
	if ((uMask & LVIS_SELECTED) && (uState & LVIS_SELECTED) && (p->dwStyle & LVS_SINGLESEL)) {
		for (size_t i = 0; i < p->aRows.size(); i ++) {
			if ((int)i != nRow && (p->aRows[i].uState & LVIS_SELECTED)) {
				p->aRows[i].uState &= ~(LVIS_SELECTED | LVIS_FOCUSED);
				W32HtmlListView(p, 5, (int)i, 0);
			}
		}
	}
	r.uState = (r.uState & ~uMask) | (uState & uMask);
	W32HtmlListView(p, 5, nRow, 0);
	if (bNotify && r.uState != uOld) {
		NMLISTVIEW *pNm = NewNotify(p, LVN_ITEMCHANGED, nRow);
		pNm->uOldState	= uOld;
		pNm->uNewState	= r.uState;
		pNm->uChanged	= LVIF_STATE;
		PostNotify(p, pNm);
	}
}

static void SetCell(W32Wnd *p, int nRow, int nCol, const char *psz)
{
	if (nRow < 0 || nRow >= (int)p->aRows.size() || nCol < 0) {
		return;
	}
	W32LvRow &r = p->aRows[nRow];
	if ((int)r.aText.size() <= nCol) {
		r.aText.resize(nCol + 1);
	}
	r.aText[nCol] = psz ? psz : "";
	W32HtmlListView(p, 3, nRow, nCol);
}

static LRESULT ListViewProc(W32Wnd *p, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nRows = (int)p->aRows.size();
	switch (uMsg) {
	case LVM_SETEXTENDEDLISTVIEWSTYLE:
		{
			DWORD dwOld = p->dwLvExStyle;
			DWORD dwMask = wParam ? (DWORD)wParam : 0xFFFFFFFF;
			p->dwLvExStyle = (p->dwLvExStyle & ~dwMask) | ((DWORD)lParam & dwMask);
			W32HtmlListView(p, 6, 0, 0);
			return dwOld;
		}
	case LVM_GETEXTENDEDLISTVIEWSTYLE:
		return p->dwLvExStyle;
	case LVM_INSERTCOLUMN:
		{
			const LVCOLUMN *pCol = (const LVCOLUMN *)lParam;
			int nIndex = std::max(0, std::min((int)wParam, (int)p->aColText.size()));
			p->aColText.insert(p->aColText.begin() + nIndex, (pCol && (pCol->mask & LVCF_TEXT) && pCol->pszText) ? pCol->pszText : "");
			p->aColWidth.insert(p->aColWidth.begin() + nIndex, (pCol && (pCol->mask & LVCF_WIDTH)) ? pCol->cx : 50);
			W32HtmlListView(p, 1, 0, nIndex);
			return nIndex;
		}
	case LVM_SETCOLUMNWIDTH:
		if ((int)wParam >= 0 && (int)wParam < (int)p->aColWidth.size()) {
			p->aColWidth[wParam] = std::max(0, (int)(short)LOWORD(lParam));
			W32HtmlListView(p, 1, -1, (int)wParam);
		}
		return TRUE;
	case LVM_GETCOLUMNWIDTH:
		return ((int)wParam >= 0 && (int)wParam < (int)p->aColWidth.size()) ? p->aColWidth[wParam] : 0;
	case LVM_INSERTITEM:
		{
			const LVITEM *pItem = (const LVITEM *)lParam;
			if (pItem == NULL) {
				return -1;
			}
			int nIndex = std::max(0, std::min(pItem->iItem, nRows));
			W32LvRow row;
			row.lParam = (pItem->mask & LVIF_PARAM) ? pItem->lParam : 0;
			row.uState = (pItem->mask & LVIF_STATE) ? (pItem->state & pItem->stateMask) : 0;
			if ((row.uState & LVIS_STATEIMAGEMASK) == 0) {
				row.uState |= INDEXTOSTATEIMAGEMASK(1);		/* チェック無し */
			}
			row.aText.resize(1);
			if ((pItem->mask & LVIF_TEXT) && pItem->pszText) {
				row.aText[0] = pItem->pszText;
			}
			p->aRows.insert(p->aRows.begin() + nIndex, row);
			W32HtmlListView(p, 2, nIndex, 0);
			W32HtmlListView(p, 3, nIndex, 0);
			W32HtmlListView(p, 5, nIndex, 0);
			return nIndex;
		}
	case LVM_SETITEM:
		{
			const LVITEM *pItem = (const LVITEM *)lParam;
			if (pItem == NULL || pItem->iItem < 0 || pItem->iItem >= nRows) {
				return FALSE;
			}
			if ((pItem->mask & LVIF_TEXT) && pItem->pszText) {
				SetCell(p, pItem->iItem, pItem->iSubItem, pItem->pszText);
			}
			if (pItem->mask & LVIF_PARAM) {
				p->aRows[pItem->iItem].lParam = pItem->lParam;
			}
			if (pItem->mask & LVIF_STATE) {
				SetRowState(p, pItem->iItem, pItem->state, pItem->stateMask, FALSE);
			}
			return TRUE;
		}
	case LVM_SETITEMTEXT:
		{
			const LVITEM *pItem = (const LVITEM *)lParam;
			if (pItem) {
				SetCell(p, (int)wParam, pItem->iSubItem, pItem->pszText);
			}
			return TRUE;
		}
	case LVM_GETITEMTEXT:
		{
			LVITEM *pItem = (LVITEM *)lParam;
			if (pItem == NULL || pItem->pszText == NULL || pItem->cchTextMax <= 0) {
				return 0;
			}
			std::string str;
			if ((int)wParam >= 0 && (int)wParam < nRows && pItem->iSubItem >= 0 &&
				pItem->iSubItem < (int)p->aRows[wParam].aText.size()) {
				str = p->aRows[wParam].aText[pItem->iSubItem];
			}
			size_t n = std::min((size_t)pItem->cchTextMax - 1, str.size());
			memcpy(pItem->pszText, str.c_str(), n);
			pItem->pszText[n] = 0;
			return (LRESULT)n;
		}
	case LVM_GETITEM:
		{
			LVITEM *pItem = (LVITEM *)lParam;
			if (pItem == NULL || pItem->iItem < 0 || pItem->iItem >= nRows) {
				return FALSE;
			}
			W32LvRow &r = p->aRows[pItem->iItem];
			if (pItem->mask & LVIF_PARAM) {
				pItem->lParam = r.lParam;
			}
			if (pItem->mask & LVIF_STATE) {
				pItem->state = r.uState & pItem->stateMask;
			}
			if ((pItem->mask & LVIF_TEXT) && pItem->pszText && pItem->cchTextMax > 0) {
				std::string str = (pItem->iSubItem >= 0 && pItem->iSubItem < (int)r.aText.size()) ? r.aText[pItem->iSubItem] : "";
				size_t n = std::min((size_t)pItem->cchTextMax - 1, str.size());
				memcpy(pItem->pszText, str.c_str(), n);
				pItem->pszText[n] = 0;
			}
			return TRUE;
		}
	case LVM_SETITEMSTATE:
		{
			const LVITEM *pItem = (const LVITEM *)lParam;
			if (pItem == NULL) {
				return FALSE;
			}
			if ((int)wParam == -1) {
				for (int i = 0; i < nRows; i ++) {
					SetRowState(p, i, pItem->state, pItem->stateMask, FALSE);
				}
			} else {
				SetRowState(p, (int)wParam, pItem->state, pItem->stateMask, FALSE);
			}
			return TRUE;
		}
	case LVM_GETITEMSTATE:
		if ((int)wParam < 0 || (int)wParam >= nRows) {
			return 0;
		}
		return p->aRows[wParam].uState & (UINT)lParam;
	case LVM_GETITEMCOUNT:
		return nRows;
	case LVM_SETITEMCOUNT:
		return TRUE;
	case LVM_GETSELECTEDCOUNT:
		{
			int n = 0;
			for (int i = 0; i < nRows; i ++) {
				if (p->aRows[i].uState & LVIS_SELECTED) {
					n ++;
				}
			}
			return n;
		}
	case LVM_GETNEXTITEM:
		{
			UINT uFlags = LOWORD(lParam);
			for (int i = (int)wParam + 1; i < nRows; i ++) {
				if (uFlags == LVNI_ALL) {
					return i;
				}
				if ((uFlags & LVNI_SELECTED) && !(p->aRows[i].uState & LVIS_SELECTED)) {
					continue;
				}
				if ((uFlags & LVNI_FOCUSED) && !(p->aRows[i].uState & (LVIS_FOCUSED | LVIS_SELECTED))) {
					continue;
				}
				return i;
			}
			return -1;
		}
	case LVM_GETSELECTIONMARK:
		for (int i = 0; i < nRows; i ++) {
			if (p->aRows[i].uState & LVIS_SELECTED) {
				return i;
			}
		}
		return -1;
	case LVM_SETSELECTIONMARK:
		return -1;
	case LVM_DELETEITEM:
		if ((int)wParam < 0 || (int)wParam >= nRows) {
			return FALSE;
		}
		p->aRows.erase(p->aRows.begin() + wParam);
		W32HtmlListView(p, 4, (int)wParam, 0);
		return TRUE;
	case LVM_DELETEALLITEMS:
		p->aRows.clear();
		W32HtmlListView(p, 0, 0, 0);
		return TRUE;
	case LVM_ENSUREVISIBLE:
		W32HtmlListView(p, 7, (int)wParam, 0);
		return TRUE;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* JavaScript から呼ばれる：リストビューの行が操作された
   nKind 0:クリック 1:チェック欄のクリック 2:ダブルクリック 3:右クリック
   （クリックには 0x10:Ctrl（Mac は command） 0x20:Shift を足して渡す） */
extern "C" EMSCRIPTEN_KEEPALIVE void W32JsOnListView(int nHtmlID, int nRow, int nKind)
{
	W32Wnd *p = W32FindWndByHtmlID(nHtmlID);
	if (p == NULL || !p->bEnabled) {
		return;
	}
	BOOL bCtrl = (nKind & 0x10) != 0;
	BOOL bShift = (nKind & 0x20) != 0;
	nKind &= 0x0F;
	if (p->dwStyle & LVS_SINGLESEL) {
		bCtrl = bShift = FALSE;
	}
	if (nRow >= (int)p->aRows.size()) {
		nRow = -1;
	}
	if (nKind == 1 && nRow >= 0) {
		UINT uCheck = (p->aRows[nRow].uState & LVIS_STATEIMAGEMASK) >> 12;
		SetRowState(p, nRow, INDEXTOSTATEIMAGEMASK(uCheck == 2 ? 1 : 2), LVIS_STATEIMAGEMASK, TRUE);
		return;
	}
	if (nRow >= 0) {
		/* 右クリック・ダブルクリックで選択済みの行なら、複数選択はそのまま */
		BOOL bKeep = (nKind != 0) && (p->aRows[nRow].uState & LVIS_SELECTED);
		if (bShift) {
			/* Shift：前にクリックした行から、ここまでを選ぶ */
			int nAnchor = nRow;
			for (int i = 0; i < (int)p->aRows.size(); i ++) {
				if (p->aRows[i].uState & LVIS_FOCUSED) {
					nAnchor = i;
					break;
				}
			}
			int nFrom = std::min(nAnchor, nRow), nTo = std::max(nAnchor, nRow);
			for (int i = 0; i < (int)p->aRows.size(); i ++) {
				if ((i < nFrom || i > nTo) && !bCtrl) {
					SetRowState(p, i, 0, LVIS_SELECTED, TRUE);
				} else if (i >= nFrom && i <= nTo) {
					SetRowState(p, i, LVIS_SELECTED, LVIS_SELECTED, TRUE);
				}
			}
		} else if (bCtrl) {
			/* Ctrl：その行だけ選ぶ・外すを切り替える */
			for (int i = 0; i < (int)p->aRows.size(); i ++) {
				if (i != nRow) {
					SetRowState(p, i, 0, LVIS_FOCUSED, FALSE);
				}
			}
			UINT uSel = (p->aRows[nRow].uState & LVIS_SELECTED) ? 0 : LVIS_SELECTED;
			SetRowState(p, nRow, uSel | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED, TRUE);
		} else {
			/* 普通のクリック：Windows と同じく、ほかの行の選択は外してその行だけにする */
			if (!bKeep) {
				for (int i = 0; i < (int)p->aRows.size(); i ++) {
					if (i != nRow) {
						SetRowState(p, i, 0, LVIS_SELECTED | LVIS_FOCUSED, TRUE);
					}
				}
			}
			SetRowState(p, nRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED, TRUE);
		}
	} else if (!bCtrl && !bShift) {
		/* 何も無い所をクリックしたら選択を外す */
		for (int i = 0; i < (int)p->aRows.size(); i ++) {
			SetRowState(p, i, 0, LVIS_SELECTED | LVIS_FOCUSED, TRUE);
		}
	}
	UINT uCode = NM_CLICK;
	if (nKind == 2) {
		uCode = NM_DBLCLK;
	} else if (nKind == 3) {
		uCode = NM_RCLICK;
	}
	PostNotify(p, NewNotify(p, uCode, nRow));
}

/* ------------------------------------------------------------------------- */
/* プロシージャ																 */
/* ------------------------------------------------------------------------- */
LRESULT CALLBACK W32CtrlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	W32Wnd *p = W32GetWnd(hWnd);
	if (p == NULL) {
		return 0;
	}
	switch (p->nKind) {
	case W32WND_COMBOBOX:
	case W32WND_LISTBOX:
		return ListProc(p, hWnd, uMsg, wParam, lParam);
	case W32WND_SCROLLBAR:
		return ScrollProc(p, hWnd, uMsg, wParam, lParam);
	case W32WND_LISTVIEW:
		return ListViewProc(p, hWnd, uMsg, wParam, lParam);
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
