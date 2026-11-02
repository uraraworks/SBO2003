/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		レベルアップ情報ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgLvUpInfo.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2026/10/28													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "AdminDlgLvUpInfoEdit.h"
#include "AdminDlgLvUpInfo.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static LPCMYARRAYLVUPINFO s_pDst = NULL;						/* OK時の書き戻し先 */
static CMYARRAYLVUPINFO s_aLvUpInfo;							/* 編集中のレベルアップ情報 */
static LVUPINFO s_LvUpInfoEdit;									/* 編集画面に渡す1件分 */
static int s_nEditNo = -1;										/* 編集中の配列番号(-1:追加) */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgLvUpInfo(HWND hWndParent, LPCMYARRAYLVUPINFO pDst);					/* ダイアログ表示(レベルアップ情報) */
void CloseDlgLvUpInfo(void);													/* ダイアログ終了(レベルアップ情報) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */
static int GetSelectNo(void);													/* 選択中の配列番号を取得 */

/* ========================================================================= */
/* 関数名：ShowDlgLvUpInfo													 */
/* 内容：ダイアログ表示(レベルアップ情報)									 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
void ShowDlgLvUpInfo(HWND hWndParent, LPCMYARRAYLVUPINFO pDst)
{
	if (s_hWndDlg) {
		return;
	}

	/* 職業情報の編集画面が持っている内容を写して編集する */
	s_pDst = pDst;
	g_JobInfo.CopyLvUpInfo (&s_aLvUpInfo, pDst);

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_LVUPINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		g_JobInfo.DeleteAllLvUpInfo (&s_aLvUpInfo);
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgLvUpInfo													 */
/* 内容：ダイアログ終了(レベルアップ情報)									 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
void CloseDlgLvUpInfo(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	CloseDlgLvUpInfoEdit ();

	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_pDst = NULL;
	g_JobInfo.DeleteAllLvUpInfo (&s_aLvUpInfo);
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
	int i;
	HWND hWndTmp;
	LV_COLUMN lvcol;
	static LPSTR apszName[] = {"レベル", "力", "素早さ", "体力", "賢さ", "運のよさ", "最大HP", "最大Mana", "必要な経験値"};
	static int anWidth[] = {50, 40, 50, 40, 40, 60, 60, 70, 90};

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	for (i = 0; i < 9; i ++) {
		lvcol.cx = anWidth[i];
		lvcol.pszText = apszName[i];
		ListView_InsertColumn (hWndTmp, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	/* WM_INITDIALOG の時点ではまだ s_hWndDlg が入っていないので先に入れておく */
	s_hWndDlg = hWnd;
	RenewList ();

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
	case IDC_ADD:	/* 追加 */
		{
			int nCount;
			PLVUPINFO pLast;

			/* 入力の手間を減らすため、最後のレベルの値を初期値にする */
			ZeroMemory (&s_LvUpInfoEdit, sizeof (s_LvUpInfoEdit));
			nCount = s_aLvUpInfo.GetSize ();
			if (nCount > 0) {
				pLast = s_aLvUpInfo[nCount - 1];
				g_JobInfo.CopyLvUpInfo (&s_LvUpInfoEdit, pLast);
			}
			s_nEditNo = -1;
			ShowDlgLvUpInfoEdit (hWnd, &s_LvUpInfoEdit, nCount + 1);
		}
		break;

	case IDC_EDIT:	/* 編集 */
		{
			int nNo;

			nNo = GetSelectNo ();
			if (nNo < 0) {
				break;
			}
			g_JobInfo.CopyLvUpInfo (&s_LvUpInfoEdit, s_aLvUpInfo[nNo]);
			s_nEditNo = nNo;
			ShowDlgLvUpInfoEdit (hWnd, &s_LvUpInfoEdit, nNo + 1);
		}
		break;

	case IDC_DELETE:	/* 削除 */
		{
			int nNo;

			nNo = GetSelectNo ();
			if (nNo < 0) {
				break;
			}
			g_JobInfo.DeleteLvUpInfo (s_aLvUpInfo[nNo]);
			s_aLvUpInfo.RemoveAt (nNo);
			RenewList ();
		}
		break;

	case IDOK:
		switch (nCodeNotify) {
		case IDD_LVUPINFO_EDIT:	/* 編集画面でOKが押された */
			if (s_nEditNo < 0) {
				PLVUPINFO pNew;

				pNew = g_JobInfo.GetNewLvUpInfo ();
				g_JobInfo.CopyLvUpInfo (pNew, &s_LvUpInfoEdit);
				s_aLvUpInfo.Add (pNew);
			} else if (s_nEditNo < s_aLvUpInfo.GetSize ()) {
				g_JobInfo.CopyLvUpInfo (s_aLvUpInfo[s_nEditNo], &s_LvUpInfoEdit);
			}
			RenewList ();
			break;

		default:		/* このダイアログのOKボタン */
			/* 職業情報の編集画面へ書き戻す(サーバーへは職業情報の「送信」で送る) */
			if (s_pDst) {
				g_JobInfo.CopyLvUpInfo (s_pDst, &s_aLvUpInfo);
			}
			CloseDlgLvUpInfo ();
			break;
		}
		break;

	case IDCANCEL:
		CloseDlgLvUpInfo ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
static void RenewList(void)
{
	int i, nNo, nCount;
	char szTmp[32];
	HWND hWndTmp;
	LV_ITEM item;
	PLVUPINFO pInfo;

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);
	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	nCount = s_aLvUpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = s_aLvUpInfo[i];

		/* 1件目がレベル1(初期値)、n件目がレベルnになるのに必要な値 */
		wsprintf (szTmp, "%d", i + 1);
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);

		wsprintf (szTmp, "%u", pInfo->byPW);
		ListView_SetItemText (hWndTmp, i, 1, szTmp);
		wsprintf (szTmp, "%u", pInfo->byDEX);
		ListView_SetItemText (hWndTmp, i, 2, szTmp);
		wsprintf (szTmp, "%u", pInfo->byST);
		ListView_SetItemText (hWndTmp, i, 3, szTmp);
		wsprintf (szTmp, "%u", pInfo->byIT);
		ListView_SetItemText (hWndTmp, i, 4, szTmp);
		wsprintf (szTmp, "%u", pInfo->byLK);
		ListView_SetItemText (hWndTmp, i, 5, szTmp);
		wsprintf (szTmp, "%u", pInfo->wMAXHP);
		ListView_SetItemText (hWndTmp, i, 6, szTmp);
		wsprintf (szTmp, "%u", pInfo->wMAXMANA);
		ListView_SetItemText (hWndTmp, i, 7, szTmp);
		wsprintf (szTmp, "%lu", pInfo->dwExp);
		ListView_SetItemText (hWndTmp, i, 8, szTmp);
	}

	if (nNo >= nCount) {
		nNo = nCount - 1;
	}
	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED, LVIS_SELECTED);
	}
}

/* ========================================================================= */
/* 関数名：GetSelectNo														 */
/* 内容：選択中の配列番号を取得												 */
/* 日付：2026/10/28															 */
/* ========================================================================= */
static int GetSelectNo(void)
{
	int nNo;
	HWND hWndTmp;

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);
	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	if (nNo >= s_aLvUpInfo.GetSize ()) {
		nNo = -1;
	}

	return nNo;
}

/* Copyright(C)URARA-works 2003-2005 */
