/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		キャラ画像選択ウィンドウ定義ファイル						 */
/* ファイル名：	SelectCharGrpWindow.h										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/09/26													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../Glbldata.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define SBOSELECTCHARGRPWNDNAME "SboSelectCharGrpWindow"			/* 登録クラス名 */
#define MAPPARTSSCRMAX	(g_wCountMapParts / 8 - 7)					/* 最大スクロール位置 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */
#define IDC_SCRBAR	100									/* スクロールバーのコントロールID */
#define IDC_COMBO	101									/* 種別コンボボックスのコントロールID */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWnd = NULL,								/* ウィンドウ */
			s_hWndScrBar = NULL,						/* スクロールバー */
			s_hWndCombo = NULL,							/* 種別コンボボックス */
			s_hWndParent = NULL;						/* 親ウィンドウ */
static int	s_nCode = -1,								/* 親へ返す時のコード */
			s_nSelect = 0;								/* 種別選択 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowSelectCharGrpWindow(HWND hWndParent, int nCode);						/* キャラ画像選択ウィンドウ作成 */
void CloseSelectCharGrpWindow(void);											/* キャラ画像選択ウィンドウ破棄 */
static LRESULT WINAPI WndProcSelectCharGrpWindow(HWND, UINT, WPARAM, LPARAM);	/* メッセージの処理 */
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);					/* メッセージハンドラ(WM_CREATE) */
static void OnPaint(HWND);														/* メッセージハンドラ(WM_PAINT) */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_LBUTTONDOWN) */
static void OnRButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_RBUTTONDOWN) */
static void OnVScroll(HWND hWnd, HWND hWndCtl, UINT code, int pos);				/* メッセージハンドラ(WM_VSCROLL) */
static void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowSelectCharGrpWindow											 */
/* 内容：キャラ画像選択ウィンドウ作成										 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
void ShowSelectCharGrpWindow(HWND hWndParent, int nCode)
{
	WNDCLASS wc;
	RECT rect;

	if (s_hWnd) {
		return;
	}

	s_hWndParent = hWndParent;
	s_nCode = nCode;
	s_nSelect = 0;

	wc.hInstance = g_hInstance;
	wc.lpszClassName = SBOSELECTCHARGRPWNDNAME;
	wc.lpfnWndProc = (WNDPROC)WndProcSelectCharGrpWindow;
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
	s_hWnd = CreateWindow (SBOSELECTCHARGRPWNDNAME, "キャラ画像選択",
				WS_OVERLAPPED | WS_CAPTION | WS_CLIPCHILDREN,
				rect.left, rect.top, rect.right, rect.bottom,
				hWndParent, NULL, g_hInstance, NULL);

	if (s_hWnd) {
		EnableWindow (hWndParent, FALSE);
		ShowWindow (s_hWnd, SW_SHOWNOACTIVATE);
	}
}

/* ========================================================================= */
/* 関数名：CloseSelectCharGrpWindow											 */
/* 内容：キャラ画像選択ウィンドウ破棄										 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
void CloseSelectCharGrpWindow(void)
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
/* 関数名：WndProcSelectCharGrpWindow										 */
/* 内容：メッセージの処理													 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
LRESULT WINAPI WndProcSelectCharGrpWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG (hWnd, WM_PAINT, OnPaint);
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);
	HANDLE_MSG (hWnd, WM_RBUTTONDOWN, OnRButtonDown);
	HANDLE_MSG (hWnd, WM_VSCROLL, OnVScroll);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_CLOSE:
		CloseSelectCharGrpWindow ();
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
/* 日付：2004/09/26															 */
/* ========================================================================= */
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	int i;
	LPCTSTR liststr[] = {"16x16", "戦場ゲーム", "16x32", "32x32", "48x48"};

	s_hWndScrBar = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_VERT,
			32 * 8, 0, 16, 32 * 8, hWnd, (HMENU)IDC_SCRBAR, g_hInstance, NULL);
	SetScrollRange (s_hWndScrBar, SB_CTL, 0, g_wCountCharGrp * 2, FALSE);

	s_hWndCombo = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			0, 0, 12 * 10, 12 * 10, hWnd, (HMENU)IDC_COMBO, g_hInstance, NULL);
	SendMessage (s_hWndCombo, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);


	for (i = 0; i < 5; i ++) {
		SendMessage (s_hWndCombo, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
	}
	SendMessage (s_hWndCombo, CB_SETCURSEL, 0, 0);

	return TRUE;
}

/* ========================================================================= */
/* 関数名：OnPaint															 */
/* 内容：メッセージハンドラ(WM_PAINT)										 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
void OnPaint(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rc;
	SIZE size;
	int i, pos, nNo, xx, yy, nCount;
	CImg32 DibTmp, **pDib;
	HDC hDCComEfc;

	hDC = BeginPaint(hWnd, &ps);
	BitBlt (hDC, 0, 0, ps.rcPaint.bottom, ps.rcPaint.right, NULL, 0, 0, BLACKNESS);

	/* 種別毎にグラフィックを選択 */
	switch (s_nSelect) {
	case 0:
		pDib = &g_apDibChar[0];
		nCount = g_wCountCharGrp * 16;
		break;
	case 1:
		pDib = &g_pDibWChar;
		nCount = 16;
		break;
	case 2:
		pDib = &g_apDibChar1632[0];
		nCount = g_wCountCharGrp1632 * 8;
		break;
	case 3:
		pDib = &g_apDibChar3232[0];
		nCount = g_wCountCharGrp3232 * 8;
		break;
	case 4:
		pDib = &g_apDibChar4848[0];
		nCount = g_wCountCharGrp4848 * 8;
		break;
	default:
		goto Exit;
	}

	/* クライアント領域サイズのビットマップ作成 */
	GetWindowRect (hWnd, &rc);
	size.cx = rc.right - rc.left;
	size.cy = rc.bottom - rc.top;
	DibTmp.Create (size.cx, size.cy);

	pos = GetScrollPos (s_hWndScrBar, SB_CTL);
	pos *= 8;

	/* パーツ画像表示 */
	hDCComEfc = DibTmp.Lock ();

	for (i = 0; i < 64; i ++) {
		if (pos + i >= nCount) {
			break;
		}

		nNo = 0;
		switch (s_nSelect) {
		case 0:
			nNo = (pos + i) / 16;
			xx = 32 * 3;
			yy = ((pos + i) % 16) * 32;
			break;

		case 1:
			xx = 32 * 3;
			yy = ((pos + i) % 8) * 32;
			break;

		case 2:
			nNo = (pos + i) / 16;
			xx = 32 * 3;
			yy = ((pos + i) % 8) * 64;
			break;

		case 3:
			nNo = (pos + i) / 16;
			xx = 64 * 3;
			yy = ((pos + i) % 8) * 64;
			break;

		case 4:
			nNo = (pos + i) / 16;
			xx = 96 * 3;
			yy = ((pos + i) % 8) * 96;
			break;
		}

		DibTmp.Blt ((i % 8) * 32, (i / 8) * 32, 32, 32, pDib[nNo], xx, yy, TRUE);

	}

	/* 画面に転送 */
	BitBlt (hDC, 0, 24, size.cx, size.cy, hDCComEfc, 0, 0, SRCCOPY);

	DibTmp.Unlock ();

Exit:
	EndPaint (hWnd, &ps);
}

/* ========================================================================= */
/* 関数名：OnLButtonDown													 */
/* 内容：メッセージハンドラ(WM_LBUTTONDOWN)									 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	int nCode;

	nCode = x / 32 + ((y - 24) / 32) * 8;
	nCode += (GetScrollPos (s_hWndScrBar, SB_CTL) * 8);

	if (nCode >= g_wCountMapParts) {
		return;
	}

	PostMessage (s_hWndParent, WM_ADMINSELECTCHARGRP, (WPARAM)s_nCode, MAKELPARAM((WORD)nCode, (WORD)s_nSelect));
	CloseSelectCharGrpWindow ();
}

/* ========================================================================= */
/* 関数名：OnRButtonDown													 */
/* 内容：メッセージハンドラ(WM_RBUTTONDOWN)									 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
static void OnRButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	CloseSelectCharGrpWindow ();
}

/* ========================================================================= */
/* 関数名：OnVScroll														 */
/* 内容：メッセージハンドラ(WM_VSCROLL)										 */
/* 日付：2004/09/26															 */
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

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	case IDC_COMBO:		/* 種別コンボボックス */
		s_nSelect = (int)SendMessage (s_hWndCombo, CB_GETCURSEL, 0, 0);
		switch (s_nSelect) {
		case 0:
			SetScrollRange (s_hWndScrBar, SB_CTL, 0, g_wCountCharGrp * 2, FALSE);
			break;

		case 1:
			SetScrollRange (s_hWndScrBar, SB_CTL, 0, 1, FALSE);
			break;
		}

		InvalidateRect (hWnd, NULL, TRUE);
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
