/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ステータステンプレート情報ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgStTmpInfo.cpp										 */
/* 作成日：		2026/11/05													 */
/* ========================================================================= */
/*
 * MFC 版（AdminWindow/DlgAdminStTmpInfo.cpp）と同じ画面を、ブラウザ版で使う
 * 旧版の管理者ウィンドウ用に作り直したもの。リソースは IDD_STTMPINFO を使う。
 * ・取得：サーバーから届いている一覧を出し直す
 * ・編集／追加：編集画面を出す（OK でサーバーへ送る）
 * ・送信：選んでいるテンプレートをもう一度サーバーへ送る
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgStTmpInfoEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"名前", 100}, {"移動種別", 100}, {"力", 40}, {"素早さ", 50}, {"体力", 40}, {"賢さ", 40},
	{"運のよさ", 55}, {"最大HP", 55}, {"最大Mana", 65}, {"所持金", 60}, {"ドロップ率", 65},
	{"移動確率X", 65}, {"移動確率Y", 65}, {"検索範囲X", 65}, {"検索範囲Y", 65},
	{"火", 35}, {"水", 35}, {"風", 35}, {"土", 35}, {"光", 35}, {"闇", 35},
	{"魔法防御", 60}, {"レベル", 50}, {"経験値", 60},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgStTmpInfo(HWND hWndParent);											/* ダイアログ表示 */
void CloseDlgStTmpInfo(void);													/* ダイアログ終了 */
void RenewStTmpInfo(void);														/* 更新 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */

/* ========================================================================= */
/* 関数名：ShowDlgStTmpInfo													 */
/* 内容：ダイアログ表示														 */
/* ========================================================================= */
void ShowDlgStTmpInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_STTMPINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgStTmpInfo												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgStTmpInfo(void)
{
	CloseDlgStTmpInfoEdit ();
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewStTmpInfo													 */
/* 内容：サーバーから届いた内容で一覧を更新									 */
/* ========================================================================= */
void RenewStTmpInfo(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	RenewList ();
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	/* 枠を引っぱって大きさを変えた時は、一覧を広げる */
	case WM_SIZE:
		AdminAutoLayout (hWnd, FALSE);
		return 0;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
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
	for (int i = 0; i < (int)(sizeof (s_aColumn) / sizeof (s_aColumn[0])); i ++) {
		lvcol.cx = s_aColumn[i].nWidth;
		lvcol.pszText = (LPSTR)s_aColumn[i].pszName;
		ListView_InsertColumn (hWndTmp, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	HWND hWndList = GetDlgItem (hWnd, IDC_LIST);
	int nNo = ListView_GetNextItem (hWndList, -1, LVNI_ALL | LVNI_SELECTED);

	switch (nID) {
	case IDC_GET:	/* 取得（届いている一覧を出し直す） */
		RenewList ();
		break;

	case IDC_EDIT:	/* 編集 */
		if (nNo < 0 || nNo >= g_aStTmpInfo.GetSize ()) {
			MessageBox (hWnd, "編集するテンプレートを一覧から選んでください。", "ステータステンプレート情報", MB_OK);
			break;
		}
		ShowDlgStTmpInfoEdit (hWnd, nNo);
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgStTmpInfoEdit (hWnd, -1);
		break;

	case IDC_SEND:	/* 送信 */
		if (nNo < 0 || nNo >= g_aStTmpInfo.GetSize ()) {
			MessageBox (hWnd, "送信するテンプレートを一覧から選んでください。", "ステータステンプレート情報", MB_OK);
			break;
		}
		SendCmdSTTMPLATE ((DWORD)nNo, CMDSTTMPLATETYPE_CHG);
		break;

	case IDOK:
		if (nCodeNotify == IDD_STTMPINFO_EDIT) {	/* 編集画面を閉じた */
			RenewList ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* ========================================================================= */
static void RenewList(void)
{
	int i, nNo, nCount, nMoveTypeMax;
	char szTmp[64];
	HWND hWndTmp;
	LV_ITEM item;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	/* 移動種別の名前の数 */
	for (nMoveTypeMax = 0; strcmp (aNpcMoveTypeTbl[nMoveTypeMax], "z") != 0; nMoveTypeMax ++) {
	}

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = g_aStTmpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
		int nCol = 0;

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = nCol ++;
		item.pszText = pInfo->szName;
		ListView_InsertItem (hWndTmp, &item);

#define	SETCOL(pszCol)	{ item.iSubItem = nCol ++; item.pszText = (LPSTR)(pszCol); ListView_SetItem (hWndTmp, &item); }
#define	SETNUM(fmtCol, nCol_)	{ snprintf (szTmp, sizeof (szTmp), fmtCol, nCol_); SETCOL (szTmp); }
		SETCOL ((pInfo->byNpcType < nMoveTypeMax) ? aNpcMoveTypeTbl[pInfo->byNpcType] : "?");
		SETNUM ("%d", pInfo->byPW);
		SETNUM ("%d", pInfo->byDEX);
		SETNUM ("%d", pInfo->byST);
		SETNUM ("%d", pInfo->byIT);
		SETNUM ("%d", pInfo->byLK);
		SETNUM ("%d", pInfo->wMaxHP);
		SETNUM ("%d", pInfo->wMaxMana);
		SETNUM ("%u", (unsigned)pInfo->dwGold);
		SETNUM ("%d", pInfo->byDropProbability);
		SETNUM ("%d", pInfo->byMoveProbabilityX);
		SETNUM ("%d", pInfo->byMoveProbabilityY);
		SETNUM ("%d", pInfo->bySearchCx);
		SETNUM ("%d", pInfo->bySearchCy);
		SETNUM ("%d", pInfo->chAttrFire);
		SETNUM ("%d", pInfo->chAttrWater);
		SETNUM ("%d", pInfo->chAttrWind);
		SETNUM ("%d", pInfo->chAttrEarth);
		SETNUM ("%d", pInfo->chAttrLight);
		SETNUM ("%d", pInfo->chAttrDark);
		SETNUM ("%d", pInfo->wDFMagic);
		SETNUM ("%d", pInfo->byLevel);
		SETNUM ("%u", (unsigned)pInfo->dwExp);
#undef	SETNUM
#undef	SETCOL
	}

	if (nNo >= 0 && nNo < nCount) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

/* Copyright(C)URARA-works 2026 */
