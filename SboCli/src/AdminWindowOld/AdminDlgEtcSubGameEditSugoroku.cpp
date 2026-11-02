/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(すごろく)ダイアログ処理実装ファイル			 */
/* ファイル名：	AdminDlgEtcSubGameEditSugoroku.cpp							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/20													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgSugorokuParts.h"
#include "AdminDlgSugorokuEvent.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditSugoroku(HWND hWndParent);							/* ダイアログ表示(サブゲーム情報(すごろく)) */
void CloseDlgEtcSubGameEditSugoroku(void);										/* ダイアログ終了(サブゲーム情報(すごろく)) */
void GetDataDlgEtcSubGameEditSugoroku(void);									/* 入力データを取得(サブゲーム情報(すごろく)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditSugoroku									 */
/* 内容：ダイアログ表示(サブゲーム情報(すごろく))							 */
/* 日付：2004/12/20															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditSugoroku(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_SUGOROKU, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditSugoroku									 */
/* 内容：ダイアログ終了(サブゲーム情報(すごろく))							 */
/* 日付：2004/12/20															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditSugoroku(void)
{
	CloseDlgSugorokuParts ();
	CloseDlgSugorokuEvent ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditSugoroku									 */
/* 内容：入力データを取得(サブゲーム情報(すごろく))							 */
/* 日付：2004/12/20															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditSugoroku(void)
{
	char szTmp[64];
	PSUBGAME_SUGOROKUINFOSAVE pInfo;

	pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MAPID), szTmp, sizeof (szTmp));
	pInfo->dwMapID = (DWORD)atoi (szTmp);
	/* 参加受付(X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_X), szTmp, sizeof (szTmp));
	pInfo->byX = (BYTE)atoi (szTmp);
	/* 参加受付(Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_Y), szTmp, sizeof (szTmp));
	pInfo->byY = (BYTE)atoi (szTmp);
	/* ルール */
	pInfo->byRule = (BYTE)SendMessage (GetDlgItem (s_hWndDlg, IDC_TYPE), CB_GETCURSEL, 0, 0);
	/* 参加人数 */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_COUNT), szTmp, sizeof (szTmp));
	pInfo->byMaxCount = (BYTE)atoi (szTmp);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/12/20															 */
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
/* 日付：2004/12/20															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	int i;
	char szTmp[64];
	HWND hWndTmp;
	PSUBGAME_SUGOROKUINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	/* 「ルール」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPE);
	for (i = 0; i < SUBGAME_SUGOROKU_RULE_MAX; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)aSugorokuRuleTbl[i]);
	}

	pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	wsprintf (szTmp, "%d", pInfo->dwMapID);
	SetWindowText (GetDlgItem (hWnd, IDC_MAPID), szTmp);
	/* 参加受付(X) */
	wsprintf (szTmp, "%d", pInfo->byX);
	SetWindowText (GetDlgItem (hWnd, IDC_X), szTmp);
	/* 参加受付(Y) */
	wsprintf (szTmp, "%d", pInfo->byY);
	SetWindowText (GetDlgItem (hWnd, IDC_Y), szTmp);
	/* ルール */
	SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_SETCURSEL, pInfo->byRule, 0);
	/* 参加人数 */
	wsprintf (szTmp, "%d", pInfo->byMaxCount);
	SetWindowText (GetDlgItem (hWnd, IDC_COUNT), szTmp);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/12/20															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_EVENT:
		ShowDlgSugorokuEvent (hWnd);
		break;

	case IDC_PARTS:
		ShowDlgSugorokuParts (hWnd);
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
