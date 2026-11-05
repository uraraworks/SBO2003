/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		魔法利用者情報編集ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgMagicUserInfoEdit.cpp								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/15													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* テーブル番号 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgMagicUserInfoEdit(HWND hWndParent, int nNo);						/* ダイアログ表示(魔法利用者情報) */
void CloseDlgMagicUserInfoEdit(void);											/* ダイアログ終了(魔法利用者情報) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* ========================================================================= */
/* 関数名：ShowDlgMagicUserInfoEdit											 */
/* 内容：ダイアログ表示(魔法利用者情報編集)									 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void ShowDlgMagicUserInfoEdit(HWND hWndParent, int nNo)
{
	if (s_hWndDlg) {
		return;
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_MAGICUSERINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgMagicUserInfoEdit										 */
/* 内容：ダイアログ終了(魔法利用者情報編集)									 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void CloseDlgMagicUserInfoEdit(void)
{
	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/15															 */
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
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	HWND hWndTmp;
	LV_ITEM item;
	LV_COLUMN lvcol;
	int i, nCount;

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	/* リストのラベルを設定 */
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 200;
	lvcol.pszText = "職業名";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);


	/* リストの中身を更新 */
	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	nCount = g_JobInfo.GetCount ();
	for (i = 0; i < nCount; i ++) {
		PJOBINFO pInfo;

		pInfo = g_JobInfo.GetPtr (i);

		item.iItem = i;
		item.pszText = pInfo->pszName;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);
	}


	if (s_nNo >= 0) {
		PMAGICUSERINFO pInfo;
		HWND hWndName;
		hWndName = GetDlgItem (hWnd, IDC_NAME);

		/* 名前 */
		pInfo = g_aMagicUserInfo[s_nNo];
		SetWindowText (hWndName, pInfo->szName);

		/* 職業 */
		nCount = pInfo->byCount;
		for (i = 0; i < nCount; i ++) {
			ListView_SetCheckState(hWndTmp, pInfo->paJobIDTbl[i], 1);
		}
	}

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		{
			PMAGICUSERINFO pInfo;
			char szTmp[128];
			HWND hWndTmp;
			int i, nCount;

			/* 新規登録？ */
			if (s_nNo < 0) {
				pInfo = new MAGICUSERINFO;

				ZeroMemory (pInfo, sizeof (MAGICUSERINFO));
				g_aMagicUserInfo.Add (pInfo);

			} else {
				pInfo = g_aMagicUserInfo[s_nNo];
			}
			/* 名前 */
			hWndTmp = GetDlgItem (hWnd, IDC_NAME);
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (hWndTmp, szTmp, sizeof (szTmp) - 1);
			szTmp[SboTextFitWidth (szTmp, MAX_MAGICNAME_WIDTH)] = 0;		/* 画面に収まる長さまで */
			strcpy (pInfo->szName, szTmp);


			/* 職業 */
			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);

			nCount = ListView_GetItemCount(hWndTmp);
			CmyArray<BYTE, BYTE> aTmp;
			for (i = 0; i < nCount; i ++) {
				if (ListView_GetCheckState (hWndTmp, i)) {
					aTmp.Add (i);
				}
			}
			if (nCount) {
				nCount = aTmp.GetSize ();
				pInfo->paJobIDTbl = new BYTE[nCount];
				ZeroMemory (pInfo->paJobIDTbl, nCount);
				for (i = 0; i < nCount; i ++) {
					pInfo->paJobIDTbl[i] = aTmp[i];
				}
				pInfo->byCount = nCount;
			}


			if (s_nNo < 0) {
				SendCMDMAGICUSERINFO ((BYTE)(g_aMagicUserInfo.GetSize () - 1));
			}

			PostMessage (s_hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, IDD_MAGICUSERINFO_EDIT), 0);
			CloseDlgMagicUserInfoEdit ();
		}
		break;

	case IDCANCEL:
		CloseDlgMagicUserInfoEdit ();
		break;
	}
}

/* Copyright(C)URARA-works 2003-2005 */
