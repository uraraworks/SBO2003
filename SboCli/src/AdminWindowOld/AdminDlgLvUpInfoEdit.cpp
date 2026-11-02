/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		レベルアップ情報編集ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgLvUpInfoEdit.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2026/10/28													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static PLVUPINFO s_pInfo = NULL;								/* 編集するレベルアップ情報 */
static int s_nLevel = 0;										/* 編集するレベル(見出し用) */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgLvUpInfoEdit(HWND hWndParent, PLVUPINFO pInfo, int nLevel);			/* ダイアログ表示(レベルアップ情報編集) */
void CloseDlgLvUpInfoEdit(void);												/* ダイアログ終了(レベルアップ情報編集) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static UINT GetValue(HWND hWnd, int nID, UINT uMax);							/* 入力欄の数値を取得 */

/* ========================================================================= */
/* 関数名：ShowDlgLvUpInfoEdit												 */
/* 内容：ダイアログ表示(レベルアップ情報編集)								 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
void ShowDlgLvUpInfoEdit(HWND hWndParent, PLVUPINFO pInfo, int nLevel)
{
	if (s_hWndDlg) {
		return;
	}

	s_pInfo = pInfo;
	s_nLevel = nLevel;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_LVUPINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgLvUpInfoEdit												 */
/* 内容：ダイアログ終了(レベルアップ情報編集)								 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
void CloseDlgLvUpInfoEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_pInfo = NULL;
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2026/10/28															 */
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
/* 日付：2026/10/28															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	char szTmp[64];

	/* どのレベルを編集しているか分かるよう見出しに入れる */
	wsprintf (szTmp, "レベルアップ情報の編集（レベル%d）", s_nLevel);
	SetWindowText (hWnd, szTmp);

	if (s_pInfo) {
		SetDlgItemInt (hWnd, IDC_PW,		s_pInfo->byPW,		FALSE);
		SetDlgItemInt (hWnd, IDC_DEX,		s_pInfo->byDEX,		FALSE);
		SetDlgItemInt (hWnd, IDC_ST,		s_pInfo->byST,		FALSE);
		SetDlgItemInt (hWnd, IDC_IT,		s_pInfo->byIT,		FALSE);
		SetDlgItemInt (hWnd, IDC_LK,		s_pInfo->byLK,		FALSE);
		SetDlgItemInt (hWnd, IDC_MAXHP,		s_pInfo->wMAXHP,	FALSE);
		SetDlgItemInt (hWnd, IDC_MAXMANA,	s_pInfo->wMAXMANA,	FALSE);
		SetDlgItemInt (hWnd, IDC_EXP,		(UINT)s_pInfo->dwExp,	FALSE);
	}

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		if (s_pInfo) {
			s_pInfo->byPW		= (BYTE)GetValue (hWnd, IDC_PW,		0xFF);
			s_pInfo->byDEX		= (BYTE)GetValue (hWnd, IDC_DEX,	0xFF);
			s_pInfo->byST		= (BYTE)GetValue (hWnd, IDC_ST,		0xFF);
			s_pInfo->byIT		= (BYTE)GetValue (hWnd, IDC_IT,		0xFF);
			s_pInfo->byLK		= (BYTE)GetValue (hWnd, IDC_LK,		0xFF);
			s_pInfo->wMAXHP		= (WORD)GetValue (hWnd, IDC_MAXHP,	0xFFFF);
			s_pInfo->wMAXMANA	= (WORD)GetValue (hWnd, IDC_MAXMANA,0xFFFF);
			s_pInfo->dwExp		= (DWORD)GetValue (hWnd, IDC_EXP,	0xFFFFFFFF);
		}
		PostMessage (s_hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, IDD_LVUPINFO_EDIT), 0);
		CloseDlgLvUpInfoEdit ();
		break;

	case IDCANCEL:
		CloseDlgLvUpInfoEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：GetValue															 */
/* 内容：入力欄の数値を取得(型に入りきらない値は上限で止める)				 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
static UINT GetValue(HWND hWnd, int nID, UINT uMax)
{
	UINT uRet;

	uRet = GetDlgItemInt (hWnd, nID, NULL, FALSE);
	if (uRet > uMax) {
		uRet = uMax;
	}

	return uRet;
}

/* Copyright(C)URARA-works 2003-2005 */
