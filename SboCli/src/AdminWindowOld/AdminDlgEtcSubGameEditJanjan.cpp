/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(SBOじゃん)ダイアログ処理実装ファイル			 */
/* ファイル名：	AdminDlgEtcSubGameEditJanjan.cpp							 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../../../common/Janjan.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* CPU強さの表示名（番号＝byCpuLevel。1〜3） */
static const char *s_apszCpuLevel[JANJAN_CPU_LEVEL_MAX + 1] = { "(既定=ふつう)", "よわい", "ふつう", "つよい" };

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditJanjan(HWND hWndParent);								/* ダイアログ表示(サブゲーム情報(SBOじゃん)) */
void CloseDlgEtcSubGameEditJanjan(void);										/* ダイアログ終了(サブゲーム情報(SBOじゃん)) */
void GetDataDlgEtcSubGameEditJanjan(void);										/* 入力データを取得(サブゲーム情報(SBOじゃん)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void FillItemCombo(HWND hWnd, int nID, int nValue);						/* チケットのアイテム一覧（0=なし）を作る */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditJanjan										 */
/* 内容：ダイアログ表示(サブゲーム情報(SBOじゃん))							 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditJanjan(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_JANJAN, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditJanjan										 */
/* 内容：ダイアログ終了(サブゲーム情報(SBOじゃん))							 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditJanjan(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditJanjan									 */
/* 内容：入力データを取得(サブゲーム情報(SBOじゃん))						 */
/*       範囲外の入力は、遊べる範囲に丸める（サーバーも丸める）			 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditJanjan(void)
{
	PSUBGAME_JANJANINFOSAVE pInfo;
	char szTmp[16];
	int nSel, nNum;

	pInfo = (PSUBGAME_JANJANINFOSAVE)g_pSubGameInfo->pInfo;
	/* 参加チケット（アイテムと必要枚数。枚数 0 は無料でSキー） */
	nSel = (int)SendMessage (GetDlgItem (s_hWndDlg, IDC_JJ_TICKET), CB_GETCURSEL, 0, 0);
	pInfo->wTicketItemID = (WORD)((nSel < 0) ? 0 : (nSel > 65535) ? 65535 : nSel);
	ZeroMemory (szTmp, sizeof (szTmp));
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_JJ_TICKETNUM), szTmp, sizeof (szTmp));
	nNum = atoi (szTmp);
	pInfo->byTicketNum = (BYTE)((nNum < 0) ? 0 : (nNum > JANJAN_TICKET_MAX) ? JANJAN_TICKET_MAX : nNum);
	/* CPU強さ */
	nSel = (int)SendMessage (GetDlgItem (s_hWndDlg, IDC_JJ_LEVEL), CB_GETCURSEL, 0, 0);
	pInfo->byCpuLevel = (BYTE)((nSel < 0) ? 0 : (nSel > JANJAN_CPU_LEVEL_MAX) ? JANJAN_CPU_LEVEL_MAX : nSel);
	JanjanInfoClamp (pInfo);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	int i;
	char szTmp[16];
	HWND hWndTmp;
	PSUBGAME_JANJANINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	/* 「CPU強さ」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_JJ_LEVEL);
	for (i = 0; i <= JANJAN_CPU_LEVEL_MAX; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)s_apszCpuLevel[i]);
	}

	pInfo = (PSUBGAME_JANJANINFOSAVE)g_pSubGameInfo->pInfo;
	SendMessage (hWndTmp, CB_SETCURSEL, (pInfo->byCpuLevel > JANJAN_CPU_LEVEL_MAX) ? JANJAN_CPU_LEVEL_MAX : pInfo->byCpuLevel, 0);
	FillItemCombo (hWnd, IDC_JJ_TICKET, pInfo->wTicketItemID);
	wsprintf (szTmp, "%d", pInfo->byTicketNum);
	SetWindowText (GetDlgItem (hWnd, IDC_JJ_TICKETNUM), szTmp);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：FillItemCombo													 */
/* 内容：チケットのアイテム一覧（0=なし）を作る								 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void FillItemCombo(HWND hWnd, int nID, int nValue)
{
	HWND hWndCombo = GetDlgItem (hWnd, nID);
	char szTmp[160];
	int i, nCount = g_aItemInfo.GetSize ();

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"0: なし");
	for (i = 1; (i < nCount) || (i <= nValue); i ++) {
		PITEMINFO pItem = (i < nCount) ? g_aItemInfo[i] : NULL;
		snprintf (szTmp, sizeof (szTmp), "%d: %s", i, pItem ? pItem->szName : "(不明)");
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nValue, 0);
}

/* Copyright(C)URARA-works 2003-2005 */
