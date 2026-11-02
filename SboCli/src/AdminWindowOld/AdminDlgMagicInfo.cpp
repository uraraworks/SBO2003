/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		魔法情報ダイアログ処理実装ファイル							 */
/* ファイル名：	AdminDlgMagicInfo.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/12													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgMagicInfoEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgMagicInfo(HWND hWndParent);											/* ダイアログ表示(魔法情報) */
void CloseDlgMagicInfo(void);													/* ダイアログ終了(魔法情報) */
void RenewMagicInfo(void);														/* 魔法情報更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgMagicInfo													 */
/* 内容：ダイアログ表示(魔法情報)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void ShowDlgMagicInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_MAGICINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgMagicInfo												 */
/* 内容：ダイアログ終了(魔法情報)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void CloseDlgMagicInfo(void)
{
	CloseDlgMagicInfoEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewMagicInfo													 */
/* 内容：魔法情報更新														 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void RenewMagicInfo(void)
{
	RenewList ();
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/12															 */
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
/* 日付：2004/07/12															 */
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
	lvcol.pszText = "名前";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 80;
	lvcol.pszText = "効果";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	lvcol.pszText = "種別";
	ListView_InsertColumn (hWndTmp, 2, &lvcol);
	lvcol.pszText = "対象";
	ListView_InsertColumn (hWndTmp, 3, &lvcol);
	lvcol.pszText = "人数";
	ListView_InsertColumn (hWndTmp, 4, &lvcol);
	lvcol.pszText = "発動";
	ListView_InsertColumn (hWndTmp, 5, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/12															 */
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
			SendCMDMAGICINFOREQ ((BYTE)nNo);
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

			ShowDlgMagicInfoEdit (hWnd, nNo);
		}
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgMagicInfoEdit (hWnd, -1);
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
			SendCMDMAGICINFO ((BYTE)nNo);
		}
		break;

	case IDOK:
		switch (nCodeNotify) {
		case IDD_MAGICINFO_EDIT:	/* 編集画面 */
			RenewList ();
			break;
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2004/07/12															 */
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

	nCount = g_aMagicInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo;
		char szTmp[128];

		pInfo = g_aMagicInfo[i];
		item.iItem = i;

		/* 魔法名 */
		item.pszText = pInfo->szName;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);

		/* 効果種別 */
		strcpy (szTmp, g_aStrType[pInfo->byType]);
		item.iSubItem = 1;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);

		/* 種別 */
		strcpy (szTmp, g_aStrTypeInfo[pInfo->byTypeInfoID]);
		item.iSubItem = 2;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);

		/* 使用対象 */
		strcpy (szTmp, g_aStrTarget[pInfo->byTarget]);
		item.iSubItem = 3;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);

		/* 発動人数 */
		strcpy (szTmp, g_aStrTargetCount[pInfo->byTargetCount]);
		item.iSubItem = 4;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);

		/* 発動種別 */
		strcpy (szTmp, g_aStrTargeType[pInfo->byTargetType]);
		item.iSubItem = 5;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);
	}

	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
