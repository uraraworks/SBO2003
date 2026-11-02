/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報編集ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgEtcSubGameEdit.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/07													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../SboCliRecvCmd.h"
#include "AdminDlgEtcSubGameEditBomb.h"
#include "AdminDlgEtcSubGameEditMaze.h"
#include "AdminDlgEtcSubGameEditDungeon.h"
#include "AdminDlgEtcSubGameEditWar.h"
#include "AdminDlgEtcSubGameEditSugoroku.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEdit(HWND hWndParent, int nNo);							/* ダイアログ表示(サブゲーム情報) */
void CloseDlgEtcSubGameEdit(void);												/* ダイアログ終了(サブゲーム情報) */
void OnMapLbuttonDownDlgEtcSubGameEdit(int x, int y);							/* マップ上で左クリックされた */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam);					/* メッセージハンドラ(WM_INITEND) */
static void OnAdminPosLeft(HWND hWnd, int x, int y);							/* メッセージハンドラ(WM_ADMINPOSLEFT) */
static void ChgSubGameType(BYTE byType);										/* 種別変更 */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEdit											 */
/* 内容：ダイアログ表示(サブゲーム情報編集)									 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEdit(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEdit											 */
/* 内容：ダイアログ終了(サブゲーム情報編集)									 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEdit(void)
{
	CloseDlgEtcSubGameEditBomb ();
	CloseDlgEtcSubGameEditMaze ();
	CloseDlgEtcSubGameEditDungeon ();
	CloseDlgEtcSubGameEditWar ();
	CloseDlgEtcSubGameEditSugoroku ();
	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：OnMapLbuttonDownDlgEtcSubGameEdit								 */
/* 内容：マップ上で左クリックされた											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void OnMapLbuttonDownDlgEtcSubGameEdit(int x, int y)
{
	if (s_hWndDlg == NULL) {
		return;
	}

	PostMessage (s_hWndDlg, WM_ADMINPOSLEFT, (WPARAM)x, (LPARAM)y);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_INITEND:
		OnInitEnd (hWnd, wParam, lParam);
		break;

	case WM_ADMINPOSLEFT:
		OnAdminPosLeft (hWnd, (int)wParam, (int)lParam);
		break;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	int i;
	HWND hWndTmp;

	/* 「種別」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPE);
	for (i = 0; ; i ++) {
		if (aSubGameNameTbl[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)aSubGameNameTbl[i]);
	}
	SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_SETCURSEL, (WPARAM)g_pSubGameInfo->byType, 0);

	PostMessage (hWnd, WM_INITEND, 0, 0);
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_TYPE:	/* 種別 */
		{
			HWND hWndTmp;

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_TYPE);
			switch (nCodeNotify) {
			case CBN_SELCHANGE:		/* 選択項目が変更された */
				ChgSubGameType ((BYTE)SendMessage (hWndTmp, CB_GETCURSEL, 0, 0));
				PostMessage (hWnd, WM_INITEND, 0, 0);
				break;
			}
		}
		break;

	case IDOK:
		switch (g_pSubGameInfo->byType) {
		case SUBGAMEID_BOMB:
			GetDataDlgEtcSubGameEditBomb ();
			break;
		case SUBGAMEID_MAZE:
			GetDataDlgEtcSubGameEditMaze ();
			break;
		case SUBGAMEID_DUNGEON:
			GetDataDlgEtcSubGameEditDungeon ();
			break;
		case SUBGAMEID_WAR:
			GetDataDlgEtcSubGameEditWar ();
			break;
		case SUBGAMEID_SUGOROKU:
			GetDataDlgEtcSubGameEditSugoroku ();
			break;
		}

		PostMessage (s_hWndParent, WM_ADMINSUBGAMEINFO, 0, 0);
		CloseDlgEtcSubGameEdit ();
		break;

	case IDCANCEL:
		CloseDlgEtcSubGameEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnInitEnd														 */
/* 内容：メッセージハンドラ(WM_INITEND)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HWND hWndTmp;

	CloseDlgEtcSubGameEditBomb ();
	CloseDlgEtcSubGameEditMaze ();
	CloseDlgEtcSubGameEditDungeon ();
	CloseDlgEtcSubGameEditWar ();
	CloseDlgEtcSubGameEditSugoroku ();

	hWndTmp = GetDlgItem (hWnd, IDC_INFO);

	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_BOMB:
		ShowDlgEtcSubGameEditBomb (hWndTmp);
		break;
	case SUBGAMEID_MAZE:
		ShowDlgEtcSubGameEditMaze (hWndTmp);
		break;
	case SUBGAMEID_DUNGEON:
		ShowDlgEtcSubGameEditDungeon (hWndTmp);
		break;
	case SUBGAMEID_WAR:
		ShowDlgEtcSubGameEditWar (hWndTmp);
		break;
	case SUBGAMEID_SUGOROKU:
		ShowDlgEtcSubGameEditSugoroku (hWndTmp);
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnAdminPosLeft													 */
/* 内容：メッセージハンドラ(WM_ADMINPOSLEFT)								 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void OnAdminPosLeft(HWND hWnd, int x, int y)
{
	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_WAR:
		OnMapLbuttonDownDlgEtcSubGameEditWar (x, y);
		break;
	}
}

/* ========================================================================= */
/* 関数名：ChgSubGameType													 */
/* 内容：種別変更															 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void ChgSubGameType(BYTE byType)
{
	int nSize;

	if (g_pSubGameInfo) {
		SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
	}

	g_pSubGameInfo->byType = byType;
	nSize = GetSubGameDataSize (byType);
	g_pSubGameInfo->pInfo = new BYTE[nSize];
	ZeroMemory (g_pSubGameInfo->pInfo, nSize);
}

/* Copyright(C)URARA-works 2003-2005 */
