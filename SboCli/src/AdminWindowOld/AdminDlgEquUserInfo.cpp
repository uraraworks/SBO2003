/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		装備職業情報ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgEquUserInfo.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/17													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgEquUserInfoEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEquUserInfo(HWND hWndParent);										/* ダイアログ表示(装備職業情報) */
void CloseDlgEquUserInfo(void);													/* ダイアログ終了(装備職業情報) */
void RenewEquUserInfo(void);													/* 装備職業情報更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgEquUserInfo												 */
/* 内容：ダイアログ表示(装備職業情報)										 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void ShowDlgEquUserInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_EQUUSERINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEquUserInfo												 */
/* 内容：ダイアログ終了(装備職業情報)										 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void CloseDlgEquUserInfo(void)
{
	CloseDlgEquUserInfoEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewEquUserInfo													 */
/* 内容：装備職業情報更新													 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void RenewEquUserInfo(void)
{
	RenewList ();
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/17															 */
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
/* 日付：2004/07/17															 */
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
	lvcol.cx = 250;
	lvcol.pszText = "職業";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/17															 */
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
			SendCMDEQUUSERINFOREQ ((BYTE)nNo);
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

			ShowDlgEquUserInfoEdit (hWnd, nNo);
		}
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgEquUserInfoEdit (hWnd, -1);
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
			SendCMDEQUUSERINFO ((BYTE)nNo);
		}
		break;

	case IDOK:
		switch (nCodeNotify) {
		case IDD_EQUUSERINFO_EDIT:	/* 編集画面 */
			RenewList ();
			break;
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
static void RenewList(void)
{
	/* リストへ登録 */
	LV_ITEM item;
	int i, nNo, nCount;
	HWND hWndTmp;

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = g_aEquUserInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PEQUUSERINFO pInfo;

		pInfo = g_aEquUserInfo[i];
		item.iItem = i;

		/* 名前 */
		item.pszText = pInfo->szName;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);

		if (pInfo->byCount) {
			/* 職業 */
			int j;
			LPSTR pszTmp;

			pszTmp = new char[MAX_EQUNAME * pInfo->byCount];
			ZeroMemory (pszTmp, MAX_EQUNAME * pInfo->byCount);

			for (j = 0; j < pInfo->byCount; j ++) {
				PJOBINFO pJobInfo;

				pJobInfo = g_JobInfo.GetPtr (pInfo->paJobIDTbl[j]);
				strcat (pszTmp, pJobInfo->pszName);
				if (j < pInfo->byCount - 1) {
					strcat (pszTmp, "、");
				}
			}

			item.iSubItem = 1;
			item.pszText = pszTmp;
			ListView_SetItem (hWndTmp, &item);
			delete [] pszTmp;
		}
	}

	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
