/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		職業情報ダイアログ処理実装ファイル							 */
/* ファイル名：	AdminDlgJobInfo.cpp											 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/10													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgJobInfoEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgJobInfo(HWND hWndParent);											/* ダイアログ表示(職業情報) */
void CloseDlgJobInfo(void);														/* ダイアログ終了(職業情報) */
void RenewJobInfo(void);														/* 職業情報更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgJobInfo													 */
/* 内容：ダイアログ表示(職業情報)											 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
void ShowDlgJobInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_JOBINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgJobInfo													 */
/* 内容：ダイアログ終了(職業情報)											 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
void CloseDlgJobInfo(void)
{
	CloseDlgJobInfoEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewJobInfo														 */
/* 内容：職業情報更新														 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void RenewJobInfo(void)
{
	RenewList ();
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
	RECT rt;
	HWND hWndTmp;
	LV_COLUMN lvcol;

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 90;
	lvcol.pszText = "職業名";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 80;
	lvcol.pszText = "特技種別";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	lvcol.pszText = "魔法種別";
	ListView_InsertColumn (hWndTmp, 2, &lvcol);
	lvcol.pszText = "装備種別";
	ListView_InsertColumn (hWndTmp, 3, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

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
	case IDC_GET:	/* 取得 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			SendCMDJOBINFOREQ ((BYTE)nNo);
		}
		break;

	case IDC_EDIT:	/* 編集 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			ShowDlgJobInfoEdit (hWnd, nNo);
		}
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgJobInfoEdit (hWnd, -1);
		break;

	case IDC_SEND:	/* 送信 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			SendCMDJOBINFO ((BYTE)nNo);
		}
		break;

	case IDOK:
		switch (nCodeNotify) {
		case IDD_JOBINFO_EDIT:	/* 編集画面 */
			RenewList ();
			break;
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
static void RenewList(void)
{
	/* リストへ登録 */
	LV_ITEM item;
//	char szTmp[128];
	int i, nNo, nCount;
	HWND hWndTmp;

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = g_JobInfo.GetCount ();
	for (i = 0; i < nCount; i ++) {
		PJOBINFO pInfo;

		pInfo = g_JobInfo.GetPtr (i);
		item.iItem = i;

		/* 職業名 */
		item.pszText = pInfo->pszName;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);

		/* 特技種別 */
//		item.iSubItem = 1;
//		ListView_SetItem (hWndTmp, &item);
		/* 魔法種別 */
		/* 装備種別 */
	}

	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
