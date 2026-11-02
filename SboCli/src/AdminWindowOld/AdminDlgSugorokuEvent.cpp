/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		すごろくイベント情報ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgSugorokuEvent.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/23													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgSugorokuEventEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;											/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;										/* 親ウィンドウ */
static CmyArray<SUGOROKUEVENTINFO, SUGOROKUEVENTINFO>	s_aEventInfo;	/* イベント情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgSugorokuEvent(HWND hWndParent);										/* ダイアログ表示(すごろくイベント情報) */
void CloseDlgSugorokuEvent(void);												/* ダイアログ終了(すごろくイベント情報) */
void RenewSugorokuEvent(void);													/* すごろくイベント情報更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgSugorokuEvent												 */
/* 内容：ダイアログ表示(すごろくイベント情報)								 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void ShowDlgSugorokuEvent(HWND hWndParent)
{
	int i, nCount;
	PSUBGAME_SUGOROKUINFOSAVE pInfo;

	if (s_hWndDlg) {
		return;
	}

	s_aEventInfo.RemoveAll ();
	pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
	nCount = pInfo->aEventInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		s_aEventInfo.Add (pInfo->aEventInfo[i]);
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_SUGOROKU_EVENT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgSugorokuEvent											 */
/* 内容：ダイアログ終了(すごろくイベント情報)								 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void CloseDlgSugorokuEvent(void)
{
	CloseDlgSugorokuEventEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewSugorokuEvent												 */
/* 内容：すごろくイベント情報更新											 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void RenewSugorokuEvent(void)
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
			PSUGOROKUEVENTINFO pInfo;

			nNo = (int)lParam;
			pInfo = (PSUGOROKUEVENTINFO)wParam;

			if (nNo < 0) {
				s_aEventInfo.Add (*pInfo);

			} else {
				s_aEventInfo[nNo] = *pInfo;
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
	lvcol.pszText = "X座標";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 90;
	lvcol.pszText = "Y座標";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	lvcol.cx = 80;
	lvcol.pszText = "イベント種別";
	ListView_InsertColumn (hWndTmp, 2, &lvcol);
	lvcol.cx = 60;
	lvcol.pszText = "パラメータ";
	ListView_InsertColumn (hWndTmp, 3, &lvcol);

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

			ShowDlgSugorokuEventEdit (hWnd, nNo, &s_aEventInfo[nNo]);
		}
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgSugorokuEventEdit (hWnd, -1, NULL);
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
			s_aEventInfo.RemoveAt (nNo);
		}
		break;

	case IDOK:
		{
			int i, nCount;
			PSUBGAME_SUGOROKUINFOSAVE pInfo;

			pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
			nCount = s_aEventInfo.GetSize ();
			pInfo->aEventInfo.RemoveAll ();
			for (i = 0; i < nCount; i ++) {
				pInfo->aEventInfo.Add (s_aEventInfo[i]);
			}

			CloseDlgSugorokuEvent ();
		}
		break;

	case IDCANCEL:
		CloseDlgSugorokuEvent ();
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

	nCount = s_aEventInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		char szTmp[128];

		item.iItem = i;

		/* X座標 */
		wsprintf (szTmp, "%d", s_aEventInfo[i].byX);
		item.pszText = szTmp;
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);

		/* Y座標 */
		wsprintf (szTmp, "%d", s_aEventInfo[i].byY);
		item.pszText = szTmp;
		item.iSubItem = 1;
		ListView_SetItem (hWndTmp, &item);

		/* イベント種別 */
		strcpy (szTmp, aSugorokuEventTbl[s_aEventInfo[i].byEventType]);
		item.pszText = szTmp;
		item.iSubItem = 2;
		ListView_SetItem (hWndTmp, &item);

		/* パラメータ */
		wsprintf (szTmp, "%d", s_aEventInfo[i].byPara1);
		item.pszText = szTmp;
		item.iSubItem = 3;
		ListView_SetItem (hWndTmp, &item);
	}

	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
