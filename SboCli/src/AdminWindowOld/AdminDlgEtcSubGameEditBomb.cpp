/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(爆弾ゲーム)ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgEtcSubGameEditBomb.cpp								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/08													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditBomb(HWND hWndParent);								/* ダイアログ表示(サブゲーム情報(爆弾ゲーム)) */
void CloseDlgEtcSubGameEditBomb(void);											/* ダイアログ終了(サブゲーム情報(爆弾ゲーム)) */
void GetDataDlgEtcSubGameEditBomb(void);										/* 入力データを取得(サブゲーム情報(爆弾ゲーム)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditBomb										 */
/* 内容：ダイアログ表示(サブゲーム情報(爆弾ゲーム))							 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditBomb(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_BOMB, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditBomb										 */
/* 内容：ダイアログ終了(サブゲーム情報(爆弾ゲーム))							 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditBomb(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditBomb										 */
/* 内容：入力データを取得(サブゲーム情報(爆弾ゲーム))						 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditBomb(void)
{
	char szTmp[64];
	PSUBGAME_BOMBINFOSAVE pInfo;

	pInfo = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MAPID), szTmp, sizeof (szTmp));
	pInfo->dwMapID = (DWORD)atoi (szTmp);
	/* 参加受付(X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_X), szTmp, sizeof (szTmp));
	pInfo->dwX = (DWORD)atoi (szTmp);
	/* 参加受付(Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_Y), szTmp, sizeof (szTmp));
	pInfo->dwY = (DWORD)atoi (szTmp);
	/* 開始位置(X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_STARTX), szTmp, sizeof (szTmp));
	pInfo->dwStartX = (DWORD)atoi (szTmp);
	/* 開始位置(Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_STARTY), szTmp, sizeof (szTmp));
	pInfo->dwStartY = (DWORD)atoi (szTmp);
	/* ゲームエリア(開始X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAX1), szTmp, sizeof (szTmp));
	pInfo->byX1 = (BYTE)atoi (szTmp);
	/* ゲームエリア(開始Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAY1), szTmp, sizeof (szTmp));
	pInfo->byY1 = (BYTE)atoi (szTmp);
	/* ゲームエリア(終了X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAX2), szTmp, sizeof (szTmp));
	pInfo->byX2 = (BYTE)atoi (szTmp);
	/* ゲームエリア(終了Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAY2), szTmp, sizeof (szTmp));
	pInfo->byY2 = (BYTE)atoi (szTmp);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	char szTmp[64];
	PSUBGAME_BOMBINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	pInfo = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	wsprintf (szTmp, "%d", pInfo->dwMapID);
	SetWindowText (GetDlgItem (hWnd, IDC_MAPID), szTmp);
	/* 参加受付(X) */
	wsprintf (szTmp, "%d", pInfo->dwX);
	SetWindowText (GetDlgItem (hWnd, IDC_X), szTmp);
	/* 参加受付(Y) */
	wsprintf (szTmp, "%d", pInfo->dwY);
	SetWindowText (GetDlgItem (hWnd, IDC_Y), szTmp);
	/* 開始位置(X) */
	wsprintf (szTmp, "%d", pInfo->dwStartX);
	SetWindowText (GetDlgItem (hWnd, IDC_STARTX), szTmp);
	/* 開始位置(Y) */
	wsprintf (szTmp, "%d", pInfo->dwStartY);
	SetWindowText (GetDlgItem (hWnd, IDC_STARTY), szTmp);
	/* ゲームエリア(開始X) */
	wsprintf (szTmp, "%d", pInfo->byX1);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAX1), szTmp);
	/* ゲームエリア(開始Y) */
	wsprintf (szTmp, "%d", pInfo->byY1);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAY1), szTmp);
	/* ゲームエリア(終了X) */
	wsprintf (szTmp, "%d", pInfo->byX2);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAX2), szTmp);
	/* ゲームエリア(終了Y) */
	wsprintf (szTmp, "%d", pInfo->byY2);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAY2), szTmp);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
//	switch (nID) {
//	}
}

/* Copyright(C)URARA-works 2003-2005 */
