/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		マップパーツ選択ウィンドウ実装ファイル						 */
/* ファイル名：	SelectMapPartsWindow.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/08													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../Glbldata.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define SBOSELECTMAPPARTSWNDNAME "SboSelectMapPartsWindow"			/* 登録クラス名 */
#define MAPPARTSSCRMAX	(g_wCountMapParts / 8 - 7)					/* 最大スクロール位置 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */
#define IDC_SCRBAR	100									/* スクロールバーのコントロールID */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWnd = NULL,								/* ウィンドウ */
			s_hWndScrBar = NULL,						/* スクロールバー */
			s_hWndParent = NULL;						/* 親ウィンドウ */
static int	s_nCode = -1;								/* 親へ返す時のコード */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowSelectMapPartsWindow(HWND hWndParent, int nCode);						/* マップパーツ選択ウィンドウ作成 */
void CloseSelectMapPartsWindow(void);											/* マップパーツ選択ウィンドウ破棄 */
static LRESULT WINAPI WndProcSelectMapPartsWindow(HWND, UINT, WPARAM, LPARAM);	/* メッセージの処理 */
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);					/* メッセージハンドラ(WM_CREATE) */
static void OnPaint(HWND);														/* メッセージハンドラ(WM_PAINT) */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_LBUTTONDOWN) */
static void OnRButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_RBUTTONDOWN) */
static void OnVScroll(HWND hWnd, HWND hWndCtl, UINT code, int pos);				/* メッセージハンドラ(WM_VSCROLL) */

/* ========================================================================= */
/* 関数名：ShowSelectMapPartsWindow											 */
/* 内容：マップパーツ選択ウィンドウ作成										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void ShowSelectMapPartsWindow(HWND hWndParent, int nCode)
{
	WNDCLASS wc;
	RECT rect;

	if (s_hWnd) {
		return;
	}

	s_hWndParent = hWndParent;
	s_nCode = nCode;

	wc.hInstance = g_hInstance;
	wc.lpszClassName = SBOSELECTMAPPARTSWNDNAME;
	wc.lpfnWndProc = (WNDPROC)WndProcSelectMapPartsWindow;
	wc.style = 0;
	wc.hIcon = LoadIcon ((HINSTANCE)NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor ((HINSTANCE)NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	/* ウィンドウのクラスを登録 */
	RegisterClass (&wc);

	RECT rcTmp;
	GetWindowRect (hWndParent, &rcTmp);
	SetRect (&rect, rcTmp.left, rcTmp.top,
			32 * 8 + 2 + GetSystemMetrics (SM_CXEDGE) * 2 + 16,
			32 * 8 + 2 + GetSystemMetrics (SM_CYEDGE) * 2 + GetSystemMetrics (SM_CYCAPTION));

	/* ウィンドウ作成 */
	s_hWnd = CreateWindow (SBOSELECTMAPPARTSWNDNAME, "マップパーツ選択",
				WS_OVERLAPPED | WS_CAPTION | WS_CLIPCHILDREN,
				rect.left, rect.top, rect.right, rect.bottom,
				hWndParent, NULL, g_hInstance, NULL);

	if (s_hWnd) {
		EnableWindow (hWndParent, FALSE);
		ShowWindow (s_hWnd, SW_SHOWNOACTIVATE);
	}
}

/* ========================================================================= */
/* 関数名：CloseSelectMapPartsWindow										 */
/* 内容：マップパーツ選択ウィンドウ破棄										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void CloseSelectMapPartsWindow(void)
{
	if (s_hWnd == NULL) {
		return;
	}

	DestroyWindow (s_hWnd);
	EnableWindow (s_hWndParent, TRUE);

	InvalidateRect (s_hWndParent, NULL, TRUE);
	s_hWnd = NULL;
	s_hWndParent = NULL;
	s_nCode = -1;
}

/* ========================================================================= */
/* 関数名：WndProcSelectMapPartsWindow										 */
/* 内容：メッセージの処理													 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
LRESULT WINAPI WndProcSelectMapPartsWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG (hWnd, WM_PAINT, OnPaint);
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);
	HANDLE_MSG (hWnd, WM_RBUTTONDOWN, OnRButtonDown);
	HANDLE_MSG (hWnd, WM_VSCROLL, OnVScroll);

	case WM_CLOSE:
		CloseSelectMapPartsWindow ();
		return TRUE;

	case WM_ERASEBKGND:
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCreate															 */
/* 内容：メッセージハンドラ(WM_CREATE)										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	s_hWndScrBar = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_VERT,
			32 * 8, 0, 16, 32 * 8, hWnd, (HMENU)IDC_SCRBAR, g_hInstance, NULL);
	SetScrollRange (s_hWndScrBar, SB_CTL, 0, MAPPARTSSCRMAX, FALSE);

	return TRUE;
}

/* ========================================================================= */
/* 関数名：OnPaint															 */
/* 内容：メッセージハンドラ(WM_PAINT)										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void OnPaint(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rc;
	SIZE size;
	int i, pos, nNo, nID, nParts, xx, yy;
	PMAPPARTSINFO pParts;
	CImg32 DibTmp;

	hDC = BeginPaint(hWnd, &ps);

	/* クライアント領域サイズのビットマップ作成 */
	GetWindowRect (hWnd, &rc);
	size.cx = rc.right - rc.left;
	size.cy = rc.bottom - rc.top;
	DibTmp.Create (size.cx, size.cy);

	pos = GetScrollPos (s_hWndScrBar, SB_CTL);
	pos *= 8;

	/* パーツ画像表示 */
	HDC hDCComEfc = DibTmp.Lock ();

	for (i = 0; i < 64; i ++) {
		if (pos + i >= g_wCountMapParts) {
			break;
		}
		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[pos + i];

		nParts = pParts->wBackID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			if (pAnime == NULL) {
				continue;
			}
			nParts = pAnime->wBackID;
		}

		nNo = (nParts) / (g_byMapGrpCountX * g_byMapGrpCountY);
		nID = (nParts) % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = nID % 16 * 32;
		yy = nID / 16 * 32;

		DibTmp.Blt ((i % 8) * 32, (i / 8) * 32, 32, 32, g_apDibMap[nNo], xx, yy);


		nParts = pParts->wGrpID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			nParts = pAnime->wGrpID;
		}

		nNo = (nParts) / (g_byMapGrpCountX * g_byMapGrpCountY);
		nID = (nParts) % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = nID % 16 * 32;
		yy = nID / 16 * 32;

		DibTmp.Blt ((i % 8) * 32, (i / 8) * 32, 32, 32, g_apDibMap[nNo], xx, yy, TRUE);

	}

	/* 画面に転送 */
	BitBlt (hDC, 0, 0, size.cx, size.cy, hDCComEfc, 0, 0, SRCCOPY);

	DibTmp.Unlock ();
	EndPaint (hWnd, &ps);
}

/* ========================================================================= */
/* 関数名：OnLButtonDown													 */
/* 内容：メッセージハンドラ(WM_LBUTTONDOWN)									 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	int nCode;

	nCode = x / 32 + (y / 32) * 8;
	nCode += (GetScrollPos (s_hWndScrBar, SB_CTL) * 8);

	if (nCode >= g_wCountMapParts) {
		return;
	}

	PostMessage (s_hWndParent, WM_ADMINSELECTMAPPARTS, (WPARAM)s_nCode, (LPARAM)nCode);
	CloseSelectMapPartsWindow ();
}

/* ========================================================================= */
/* 関数名：OnRButtonDown													 */
/* 内容：メッセージハンドラ(WM_RBUTTONDOWN)									 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void OnRButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	CloseSelectMapPartsWindow ();
}

/* ========================================================================= */
/* 関数名：OnVScroll														 */
/* 内容：メッセージハンドラ(WM_VSCROLL)										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void OnVScroll(HWND hWnd, HWND hWndCtl, UINT code, int pos)
{
	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	pos = min (MAPPARTSSCRMAX, pos);
	pos = max (0, pos);

	SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
	InvalidateRect (hWnd, NULL, TRUE);
}

/* Copyright(C)URARA-works 2003-2005 */
