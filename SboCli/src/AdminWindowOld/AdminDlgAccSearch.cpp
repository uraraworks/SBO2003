/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		アカウント検索ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgAccSearch.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/27													 */
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
void ShowDlgAccSearch(HWND hWndParent);											/* ダイアログ表示(アカウント検索) */
void CloseDlgAccSearch(void);													/* ダイアログ終了(アカウント検索) */
void AccSearchSetTblID(DWORD);													/* アカウント発見 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgAccSearch													 */
/* 内容：ダイアログ表示(アカウント検索)										 */
/* 日付：2004/07/27															 */
/* ========================================================================= */
void ShowDlgAccSearch(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ACC_SEARCH, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgAccSearch												 */
/* 内容：ダイアログ終了(アカウント検索)										 */
/* 日付：2004/07/27															 */
/* ========================================================================= */
void CloseDlgAccSearch(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：AccSearchSetTblID												 */
/* 内容：アカウント発見														 */
/* 日付：2004/07/27															 */
/* ========================================================================= */
void AccSearchSetTblID(DWORD dwTblID)
{
	HWND hWndTmp;
	char szTmp[MAX_CHARNAME * 2];

	wsprintf (szTmp, "%d", dwTblID);
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_TBLID);
	SetWindowText (hWndTmp, szTmp);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/27															 */
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
/* 日付：2004/07/27															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	HWND hWndTmp;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	/* ラジオボタンの初期値を設定 */
	hWndTmp = GetDlgItem (hWnd, IDC_ACCOUNT);
	Button_SetCheck (hWndTmp, BST_CHECKED);

	/* 検索開始IDの初期値を設定 */
	hWndTmp = GetDlgItem (hWnd, IDC_TBLID);
	SetWindowText (hWndTmp, "0");

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/27															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_SEARCH:		/* 検索 */
	case IDC_SEARCHNEXT:	/* 次を検索 */
		{
			int nTmp;
			BYTE byTmp;
			HWND hWndTmp;
			DWORD dwTblID;
			char szTmp[MAX_CHARNAME * 2];

			ZeroMemory (szTmp, sizeof (szTmp));
			hWndTmp = GetDlgItem (hWnd, IDC_CHARNAME);
			nTmp = Button_GetCheck (hWndTmp);
			byTmp = 0;

			/* キャラ名で検索？ */
			if (nTmp == BST_CHECKED) {
				byTmp = 1;
			}

			hWndTmp = GetDlgItem (hWnd, IDC_TBLID);
			GetWindowText (hWndTmp, szTmp, sizeof (szTmp));
			dwTblID = (DWORD)atoi (szTmp);

			if (nID == IDC_SEARCHNEXT) {
				dwTblID ++;
			}

			hWndTmp = GetDlgItem (hWnd, IDC_KEY);
			GetWindowText (hWndTmp, szTmp, sizeof (szTmp));
			SendCmdACCSEARCH (byTmp, dwTblID, szTmp);
		}
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
