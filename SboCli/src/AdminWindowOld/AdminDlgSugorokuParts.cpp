/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		すごろくマップパーツ情報ダイアログ処理実装ファイル			 */
/* ファイル名：	AdminDlgSugorokuParts.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/23													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgSugorokuPartsEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;											/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;										/* 親ウィンドウ */
static CmyArray<SUGOROKUPARTSINFO, SUGOROKUPARTSINFO>	s_aPartsInfo;	/* マップパーツ情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgSugorokuParts(HWND hWndParent);										/* ダイアログ表示(すごろくマップパーツ情報) */
void CloseDlgSugorokuParts(void);												/* ダイアログ終了(すごろくマップパーツ情報) */
void RenewSugorokuParts(void);													/* すごろくマップパーツ情報更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgSugorokuParts												 */
/* 内容：ダイアログ表示(すごろくマップパーツ情報)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void ShowDlgSugorokuParts(HWND hWndParent)
{
	int i, nCount;
	PSUBGAME_SUGOROKUINFOSAVE pInfo;

	if (s_hWndDlg) {
		return;
	}

	s_aPartsInfo.RemoveAll ();
	pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
	nCount = pInfo->aPartsInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		s_aPartsInfo.Add (pInfo->aPartsInfo[i]);
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_SUGOROKU_PARTS, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgSugorokuParts											 */
/* 内容：ダイアログ終了(すごろくマップパーツ情報)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void CloseDlgSugorokuParts(void)
{
	CloseDlgSugorokuPartsEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewSugorokuParts												 */
/* 内容：すごろくマップパーツ情報更新										 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void RenewSugorokuParts(void)
{
	RenewList ();
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

	case WM_ADMINCLOSEDIALOG:	/* ダイアログが閉じた */
		{
			int nNo;
			PSUGOROKUPARTSINFO pInfo;

			nNo = (int)lParam;
			pInfo = (PSUGOROKUPARTSINFO)wParam;

			if (nNo < 0) {
				s_aPartsInfo.Add (*pInfo);

			} else {
				s_aPartsInfo[nNo] = *pInfo;
			}
			delete pInfo;
			RenewList ();
		}
		break;

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
	HWND hWndTmp;
	LV_COLUMN lvcol;

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 90;
	lvcol.pszText = "パーツID";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 250;
	lvcol.pszText = "種別";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

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
	case IDC_MOD:	/* 編集 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			ShowDlgSugorokuPartsEdit (hWnd, nNo, &s_aPartsInfo[nNo]);
		}
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgSugorokuPartsEdit (hWnd, -1, NULL);
		break;

	case IDC_DELETE:	/* 削除 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (hWnd, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			ListView_DeleteItem (hWndTmp, nNo);
			s_aPartsInfo.RemoveAt (nNo);
		}
		break;

	case IDOK:
		{
			int i, nCount;
			PSUBGAME_SUGOROKUINFOSAVE pInfo;

			pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
			nCount = s_aPartsInfo.GetSize ();
			pInfo->aPartsInfo.RemoveAll ();
			for (i = 0; i < nCount; i ++) {
				pInfo->aPartsInfo.Add (s_aPartsInfo[i]);
			}

			CloseDlgSugorokuParts ();
		}
		break;

	case IDCANCEL:
		CloseDlgSugorokuParts ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2004/12/23															 */
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

	nCount = s_aPartsInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		char szTmp[128];

		item.iItem = i;

		/* パーツID */
		wsprintf (szTmp, "%d", s_aPartsInfo[i].wPartsID);
		item.pszText = szTmp;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);

		/* イベント */
		strcpy (szTmp, aSugorokuPartsTbl[s_aPartsInfo[i].byType]);
		item.iSubItem = 1;
		item.pszText = szTmp;
		ListView_SetItem (hWndTmp, &item);
	}

	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
