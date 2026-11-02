/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		すごろくイベント情報編集ダイアログ処理実装ファイル			 */
/* ファイル名：	AdminDlgSugorokuEventEdit.cpp								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/23													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;											/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;										/* 親ウィンドウ */
static int s_nNo = -1;													/* 選択番号 */
static PSUGOROKUEVENTINFO s_pEventInfo = NULL;							/* イベント情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgSugorokuEventEdit(HWND hWndParent, int nNo, PSUGOROKUEVENTINFO pInfo);	/* ダイアログ表示(すごろくイベント情報編集) */
void CloseDlgSugorokuEventEdit(void);												/* ダイアログ終了(すごろくイベント情報編集) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);						/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);					/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);			/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgSugorokuEventEdit											 */
/* 内容：ダイアログ表示(すごろくイベント情報編集)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void ShowDlgSugorokuEventEdit(HWND hWndParent, int nNo, PSUGOROKUEVENTINFO pInfo)
{
	if (s_hWndDlg) {
		return;
	}

	SAFE_DELETE (s_pEventInfo);
	s_pEventInfo = new SUGOROKUEVENTINFO;
	ZeroMemory (s_pEventInfo, sizeof (SUGOROKUEVENTINFO));
	if (pInfo) {
		CopyMemory (s_pEventInfo, pInfo, sizeof (SUGOROKUEVENTINFO));
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_SUGOROKU_EVENT_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgSugorokuEventEdit										 */
/* 内容：ダイアログ終了(すごろくイベント情報編集)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void CloseDlgSugorokuEventEdit(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	SAFE_DELETE (s_pEventInfo);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/12/23															 */
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
/* 日付：2004/12/23															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	int i;
	HWND hWndTmp;
	char szTmp[128];

	/* 「イベント種別」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPE);
	for (i = 0; i < SUBGAME_SUGOROKU_EVENT_MAX; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)aSugorokuEventTbl[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, (WPARAM)(s_pEventInfo->byEventType), 0);
	wsprintf (szTmp, "%d", s_pEventInfo->byX);
	SetWindowText (GetDlgItem (hWnd, IDC_X), szTmp);
	wsprintf (szTmp, "%d", s_pEventInfo->byY);
	SetWindowText (GetDlgItem (hWnd, IDC_Y), szTmp);
	wsprintf (szTmp, "%d", s_pEventInfo->byPara1);
	SetWindowText (GetDlgItem (hWnd, IDC_PARA1), szTmp);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		{
			PSUGOROKUEVENTINFO pInfo;
			char szTmp[128];

			pInfo = new SUGOROKUEVENTINFO;
			ZeroMemory (pInfo, sizeof (SUGOROKUEVENTINFO));
			pInfo->byEventType = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_GETCURSEL, 0, 0);
			GetWindowText (GetDlgItem (hWnd, IDC_X), szTmp, sizeof (szTmp));
			pInfo->byX = (BYTE)atoi (szTmp);
			GetWindowText (GetDlgItem (hWnd, IDC_Y), szTmp, sizeof (szTmp));
			pInfo->byY = (BYTE)atoi (szTmp);
			GetWindowText (GetDlgItem (hWnd, IDC_PARA1), szTmp, sizeof (szTmp));
			pInfo->byPara1 = (BYTE)atoi (szTmp);
			PostMessage (s_hWndParent, WM_ADMINCLOSEDIALOG, (WPARAM)pInfo, (LPARAM)s_nNo);

			CloseDlgSugorokuEventEdit ();
		}
		break;

	case IDCANCEL:
		CloseDlgSugorokuEventEdit ();
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
