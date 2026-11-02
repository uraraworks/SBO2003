/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			デバッグ情報ウィンドウ									 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */
#include "stdafx.h"
#include "Glbldata.h"
#include "SboCliSendCmd.h"
#include "SboCliRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define CLNAME "SboDebugWindow"									/* 登録クラス名 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
DWORD m_dwNumMsgs,												/* メッセージキューの数 */
	  m_dwThrowghPutSend,										/* リンク・スループット量(送信) */
	  m_dwThrowghPutRecv;										/* リンク・スループット量(受信) */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void MakeDebugWindow(void);										/* 管理者ウィンドウ作成 */
LRESULT WINAPI WndProcDebugWindow(HWND, UINT, WPARAM, LPARAM);	/* メッセージの処理 */
void DebugOnPaint(HWND);										/* WM_PAINT処理 */

extern void MyTextOut(HDC, int, int, char *, COLORREF Color = RGB (255, 255, 255));	/* 文字描画 */

/* ========================================================================= */
/* 関数名：MakeDebugWindow													 */
/* 内容：デバッグ情報ウィンドウ作成											 */
/* 日付：2003/06/26															 */
/* ========================================================================= */
void MakeDebugWindow(void)
{
	WNDCLASS wc;
	RECT rect;

	if (g_hWndDebug) {
		return;
	}

	wc.hInstance = g_hInstance;
	wc.lpszClassName = CLNAME;
	wc.lpfnWndProc = (WNDPROC)WndProcDebugWindow;
	wc.style = 0;
	wc.hIcon = LoadIcon ((HINSTANCE)NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor ((HINSTANCE)NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	/* ウィンドウのクラスを登録 */
	RegisterClass (&wc);

	if (rcDebug.right && rcDebug.bottom) {
		SetRect (&rect, rcDebug.left, rcDebug.top,
			rcDebug.right - rcDebug.left, rcDebug.bottom - rcDebug.top);

	} else {
		RECT rcTmp;
		GetWindowRect (g_hWndMain, &rcTmp);
		SetRect (&rect, rcTmp.right, rcTmp.top, 212, 12 * 11);
	}

	/* ウィンドウ作成 */
	g_hWndDebug = CreateWindow (CLNAME, "デバッグ情報ウィンドウ",
				WS_OVERLAPPED | WS_THICKFRAME | WS_CLIPCHILDREN,
				rect.left, rect.top, rect.right, rect.bottom,
				g_hWndMain, NULL, g_hInstance, NULL);
	ShowWindow (g_hWndDebug, SW_SHOWNOACTIVATE);
}

/* ========================================================================= */
/* 関数名：WndProcDebugWindow												 */
/* 内容：メッセージの処理													 */
/* 日付：2003/06/26															 */
/* ========================================================================= */
LRESULT WINAPI WndProcDebugWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_PAINT, DebugOnPaint);

	case WM_CREATE:
		m_dwNumMsgs = 0;
		m_dwThrowghPutSend = 0;
		m_dwThrowghPutRecv = 0;
		SetTimer (hWnd, 0, 1000, NULL);
		/* Alt + F4を無効化 */
		SetClassLong (hWnd, GCL_STYLE, GetClassLong (hWnd, GCL_STYLE) | (CS_NOCLOSE));
		break;

	case WM_TIMER:
		{
			m_dwThrowghPutSend = cliSock.GetThrowghPutSend (URARASOCK_IDBASE);
			m_dwThrowghPutRecv = cliSock.GetThrowghPutRecv (URARASOCK_IDBASE);
			m_dwNumMsgs = cliSock.GetQueCount (URARASOCK_IDBASE);
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	case WM_ERASEBKGND:
		break;

	case WM_DESTROY:
		KillTimer (hWnd, 0);
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：DebugOnPaint														 */
/* 内容：WM_PAINT処理														 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void DebugOnPaint(HWND hWnd)
{
	HDC hDC, hDCMem;
	HBITMAP hBmp, hBmpOld;
	PAINTSTRUCT ps;
	HFONT hFontOld;
	RECT rc;
	SIZE size;
	char szBuf[128];

	if (g_aCharInfo.GetSize () == 0) {
		return;
	}
	if (g_nMyCharID < 0) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	hDC = BeginPaint(hWnd, &ps);

	hDCMem = CreateCompatibleDC (hDC);
	GetWindowRect (hWnd, &rc);
	size.cx = rc.right - rc.left;
	size.cy = rc.bottom - rc.top;
	hBmp = CreateCompatibleBitmap (hDC, size.cx, size.cy);
	hBmpOld = (HBITMAP)SelectObject (hDCMem, hBmp);
	BitBlt (hDCMem, 0, 0, size.cx, size.cy, NULL, 0, 0, BLACKNESS);

	hFontOld = (HFONT)SelectObject (hDCMem, g_hFont12);
	SetBkMode (hDCMem, TRANSPARENT);
	SetTextColor (hDCMem, RGB (255, 255, 255));

	wsprintf (szBuf, "Online %d(管理人:%d)  Ping %dms", g_dwOnilne, g_byAdminCount, g_dwTimeHartbeat);
	MyTextOut (hDCMem, 0, 12 * 0, szBuf);

	wsprintf (szBuf, "fps %2d  描画時間 %3dms", g_byFps, g_dwTimeDrawTime);
	MyTextOut (hDCMem, 0, 12 * 1, szBuf);

	wsprintf (szBuf, "座標 MAP:%d X:%d Y:%d", pChar->dwMapID, pChar->dwX, pChar->dwY);
	MyTextOut (hDCMem, 0, 12 * 2, szBuf);

	wsprintf (szBuf, "キャラ数 画面:%3d PC:%3d NPC:%3d",
		g_aCharInfo.GetSize (), g_dwCountPC, g_dwCountNPC);
	MyTextOut (hDCMem, 0, 12 * 3, szBuf);

//	wsprintf (szBuf, "自キャラ処理待ち時間:%ld", timeGetTime () - g_dwTimeLastProc);
	wsprintf (szBuf, "通信量 送信:%5dBps 受信:%5dBps", m_dwThrowghPutSend, m_dwThrowghPutRecv);
	MyTextOut (hDCMem, 0, 12 * 4, szBuf);

//	wsprintf (szBuf, "１秒間のループ回数:%ld", g_dwFpsCount);
//	wsprintf (szBuf, "送信キュー数:%ld", m_dwNumMsgs);
//	MyTextOut (hDCMem, 0, 12 * 5, szBuf);

	SelectObject (hDCMem, hFontOld);

	BitBlt (hDC, 0, 0, size.cx, size.cy, hDCMem, 0, 0, SRCCOPY);
	SelectObject (hDCMem, hBmpOld);
	DeleteObject (hBmp);
	DeleteDC (hDCMem);

	EndPaint (hWnd, &ps);
}

/* Copyright(C)URARA-works 2003-2005 */
