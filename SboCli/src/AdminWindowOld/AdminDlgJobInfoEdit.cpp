/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		職業情報編集ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgJobInfoEdit.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/10													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgLvUpInfo.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* テーブル番号 */
static CMYARRAYLVUPINFO s_aLvUpInfo;							/* 編集中のレベルアップ情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgJobInfoEdit(HWND hWndParent, int nNo);								/* ダイアログ表示(職業情報) */
void CloseDlgJobInfoEdit(void);													/* ダイアログ終了(職業情報) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgJobInfoEdit												 */
/* 内容：ダイアログ表示(職業情報編集)										 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
void ShowDlgJobInfoEdit(HWND hWndParent, int nNo)
{
	if (s_hWndDlg) {
		return;
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_JOBINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgJobInfoEdit												 */
/* 内容：ダイアログ終了(職業情報編集)										 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
void CloseDlgJobInfoEdit(void)
{
	CloseDlgLvUpInfo ();
	g_JobInfo.DeleteAllLvUpInfo (&s_aLvUpInfo);

	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/10															 */
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
/* 日付：2004/07/10															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	if (s_nNo >= 0) {
		PJOBINFO pInfo;
		HWND hWndTmp;
		hWndTmp = GetDlgItem (hWnd, IDC_NAME);

		pInfo = g_JobInfo.GetPtr (s_nNo);
		SetWindowText (hWndTmp, pInfo->pszName);

		/* アイテム情報を見る */
		CheckDlgButton (hWnd, IDC_VIEWITEMINFO, (pInfo->dwFlg & JOBINFOFLG_VIEWITEMINFO) ? BST_CHECKED : BST_UNCHECKED);
		/* レベルアップ情報は OK を押すまで写しを編集する */
		g_JobInfo.CopyLvUpInfo (&s_aLvUpInfo, &pInfo->aLevelUpInfo);
	} else {
		g_JobInfo.DeleteAllLvUpInfo (&s_aLvUpInfo);
	}
	
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		{
			PJOBINFO pInfo;
			char szTmp[128];
			HWND hWndTmp;

			/* 新規登録？ */
			if (s_nNo < 0) {
				pInfo = g_JobInfo.GetNew ();
				g_JobInfo.Add (pInfo);

			} else {
				pInfo = g_JobInfo.GetPtr (s_nNo);
			}
			hWndTmp = GetDlgItem (hWnd, IDC_NAME);
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (hWndTmp, szTmp, sizeof (szTmp) - 1);
			szTmp[SboTextFitWidth (szTmp, MAX_JOBNAME_WIDTH)] = 0;		/* 画面に収まる長さまで */

			g_JobInfo.SetJobName (pInfo, szTmp);

			/* アイテム情報を見る */
			pInfo->dwFlg &= ~JOBINFOFLG_VIEWITEMINFO;
			if (IsDlgButtonChecked (hWnd, IDC_VIEWITEMINFO) == BST_CHECKED) {
				pInfo->dwFlg |= JOBINFOFLG_VIEWITEMINFO;
			}
			/* レベルアップ情報 */
			g_JobInfo.CopyLvUpInfo (&pInfo->aLevelUpInfo, &s_aLvUpInfo);

			if (s_nNo < 0) {
				SendCMDJOBINFO ((BYTE)(g_JobInfo.GetCount () - 1));
			}

			PostMessage (s_hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, IDD_JOBINFO_EDIT), 0);
			CloseDlgJobInfoEdit ();
		}
		break;

	case IDC_LVUPINFO:	/* レベルアップ情報の設定 */
		ShowDlgLvUpInfo (hWnd, &s_aLvUpInfo);
		break;

	case IDCANCEL:
		CloseDlgJobInfoEdit ();
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
