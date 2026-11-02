/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			吹き出しウィンドウ										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/12/07												 */
/* ========================================================================= */
#include "stdafx.h"
#include "Glbldata.h"
#include "SboCliSendCmd.h"
#include "SboCliRecvCmd.h"
#include "SboCliMsgLogWindow.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define SBOBALLOONWNDNAME "SboBalloonWindow"									/* 登録クラス名 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void MakeBalloonWindow(void);									/* 吹き出しウィンドウ作成 */
LRESULT WINAPI WndProcBalloonWindow(HWND, UINT, WPARAM, LPARAM);/* メッセージの処理 */
void BalloonOnPaint(HWND);										/* WM_PAINT処理 */

extern void MyTextOut(HDC, int, int, char *, COLORREF Color = RGB (255, 255, 255));	/* 文字描画 */

/* ========================================================================= */
/* 関数名：MakeBalloonWindow												 */
/* 内容：吹き出しウィンドウ作成												 */
/* 日付：2003/12/07															 */
/* ========================================================================= */
void MakeBalloonWindow(void)
{
	WNDCLASS wc;
	RECT rect;

	if (g_hWndBalloon) {
		return;
	}

	wc.hInstance = g_hInstance;
	wc.lpszClassName = SBOBALLOONWNDNAME;
	wc.lpfnWndProc = (WNDPROC)WndProcBalloonWindow;
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
	GetWindowRect (g_hWndMain, &rcTmp);
	SetRect (&rect, rcTmp.right, rcTmp.top,
			176 + 2 + GetSystemMetrics (SM_CXEDGE) * 2,
			176 + 2 + GetSystemMetrics (SM_CYEDGE) * 2 + GetSystemMetrics (SM_CYCAPTION));

	if (rcBalloon.left && rcBalloon.top) {
		rect.left = rcBalloon.left;
		rect.top = rcBalloon.top;
	}

	/* ウィンドウ作成 */
	g_hWndBalloon = CreateWindow (SBOBALLOONWNDNAME, "吹き出しウィンドウ",
				WS_OVERLAPPED | WS_CAPTION | WS_CLIPCHILDREN,
				rect.left, rect.top, rect.right, rect.bottom,
				g_hWndMain, NULL, g_hInstance, NULL);
	if (g_bBalloonWindowView) {
		ShowWindow (g_hWndBalloon, SW_SHOWNOACTIVATE);
	}
}

/* ========================================================================= */
/* 関数名：WndProcBalloonWindow												 */
/* 内容：メッセージの処理													 */
/* 日付：2003/12/07															 */
/* ========================================================================= */
LRESULT WINAPI WndProcBalloonWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_PAINT, BalloonOnPaint);

	case WM_CREATE:
		/* Alt + F4を無効化 */
		SetClassLong (hWnd, GCL_STYLE, GetClassLong (hWnd, GCL_STYLE) | (CS_NOCLOSE));
		break;

	case WM_LBUTTONDOWN:
		{
			POINT pt;
			pt.x = LOWORD (lParam);
			pt.y = HIWORD (lParam);
			if ((pt.x < 16) || (pt.y < 16)) {
				break;
			}
			pt.x -= 16;
			pt.y -= 16;

			SendCmdCOMEFC ((BYTE)((pt.x / 16) + (pt.y / 16 * 10)));
			SetActiveWindow (g_hWndMain);
		}
		break;

	case WM_ERASEBKGND:
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：BalloonOnPaint													 */
/* 内容：WM_PAINT処理														 */
/* 日付：2003/12/07															 */
/* ========================================================================= */
void BalloonOnPaint(HWND hWnd)
{
	HDC hDC, hDCMem;
	HBITMAP hBmp, hBmpOld;
	PAINTSTRUCT ps;
	HFONT hFontOld;
	RECT rc;
	SIZE size;
	char szBuf[128];
	int i;

	CImg32 DibTmp;
	DibTmp.Create (16, 16);

	hDC = BeginPaint(hWnd, &ps);
	hDCMem = CreateCompatibleDC (hDC);

	/* クライアント領域サイズのビットマップ作成 */
	GetWindowRect (hWnd, &rc);
	size.cx = rc.right - rc.left;
	size.cy = rc.bottom - rc.top;
	hBmp = CreateCompatibleBitmap (hDC, size.cx, size.cy);
	hBmpOld = (HBITMAP)SelectObject (hDCMem, hBmp);
	BitBlt (hDCMem, 0, 0, size.cx, size.cy, NULL, 0, 0, BLACKNESS);

	hFontOld = (HFONT)SelectObject (hDCMem, g_hFontBasic);
	SetBkMode (hDCMem, TRANSPARENT);
	SetTextColor (hDCMem, RGB (255, 255, 255));

	/* 吹き出し画像表示 */
	HDC hDCComEfc = DibTmp.Lock ();
	for (i = 0; i < 99; i ++) {
		PCOMEFCINFO pComEfc = &g_pComEfcInfo[i];
		if (pComEfc == NULL) {
			continue;
		}
		PCOMEFCANIMEINFO pAnime = &pComEfc->pAnime[0];
		if (pAnime == NULL) {
			continue;
		}
		DibTmp.Blt (0, 0, 16, 16,
				g_pDibComEfcWindow, 16 * (pAnime->byGrpID % 8), 16 * (pAnime->byGrpID / 8));
		BitBlt (hDCMem, 16 + (i + 1) % 10 * 16, 16 + (i + 1) / 10 * 16, 16, 16, hDCComEfc, 0, 0, SRCCOPY);
	}
	DibTmp.Unlock ();

	/* ガイド用文字表示 */
	for (i = 0; i < 10; i ++) {
		wsprintf (szBuf, "%d", i);
		MyTextOut (hDCMem, 24 + i * 16, 0, szBuf);
		wsprintf (szBuf, "%02d", i * 10);
		MyTextOut (hDCMem, 0, 16 + i * 16, szBuf);
	}

	SelectObject (hDCMem, hFontOld);

	/* 画面に転送 */
	BitBlt (hDC, 0, 0, size.cx, size.cy, hDCMem, 0, 0, SRCCOPY);
	SelectObject (hDCMem, hBmpOld);
	DeleteObject (hBmp);
	DeleteDC (hDCMem);

	EndPaint (hWnd, &ps);
}

/* Copyright(C)URARA-works 2003-2005 */
