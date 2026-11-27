/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(ピンボール)ダイアログ処理実装ファイル			 */
/* ファイル名：	AdminDlgEtcSubGameEditPinball.cpp							 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../../../common/PinballInfo.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditPinball(HWND hWndParent);								/* ダイアログ表示(サブゲーム情報(ピンボール)) */
void CloseDlgEtcSubGameEditPinball(void);										/* ダイアログ終了(サブゲーム情報(ピンボール)) */
void GetDataDlgEtcSubGameEditPinball(void);										/* 入力データを取得(サブゲーム情報(ピンボール)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static int GetNum(int nID);														/* 入力欄の数を取得 */
static void SetNum(HWND hWnd, int nID, int nNum);								/* 入力欄に数を設定 */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditPinball										 */
/* 内容：ダイアログ表示(サブゲーム情報(ピンボール))							 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditPinball(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_PINBALL, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditPinball									 */
/* 内容：ダイアログ終了(サブゲーム情報(ピンボール))							 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditPinball(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditPinball									 */
/* 内容：入力データを取得(サブゲーム情報(ピンボール))						 */
/*       範囲外の入力は、遊べる範囲に丸める（サーバーも丸める）			 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditPinball(void)
{
	PSUBGAME_PINBALLINFOSAVE pInfo;

	pInfo = (PSUBGAME_PINBALLINFOSAVE)g_pSubGameInfo->pInfo;
	pInfo->wCoinItem		= (WORD)PbiClampInt (GetNum (IDC_PB_COINITEM), 0, 65535);
	pInfo->byCoinCount		= (BYTE)GetNum (IDC_PB_COINCOUNT);
	pInfo->byBalls			= (BYTE)GetNum (IDC_PB_BALLS);
	pInfo->bySaveSec		= (BYTE)GetNum (IDC_PB_SAVESEC);
	pInfo->wScoreBumper		= (WORD)PbiClampInt (GetNum (IDC_PB_SCOREBUMPER), 0, PB_CFG_SCORE_MAX);
	pInfo->wScoreTarget		= (WORD)PbiClampInt (GetNum (IDC_PB_SCORETARGET), 0, PB_CFG_SCORE_MAX);
	pInfo->wScoreBonus		= (WORD)PbiClampInt (GetNum (IDC_PB_SCOREBONUS), 0, PB_CFG_SCORE_MAX);
	pInfo->wComboMs			= (WORD)PbiClampInt (GetNum (IDC_PB_COMBOMS), 0, PB_CFG_MS_MAX);
	pInfo->cDx				= (signed char)PbiClampInt (GetNum (IDC_PB_DX), -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	pInfo->cDy				= (signed char)PbiClampInt (GetNum (IDC_PB_DY), -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	pInfo->cSeatDx			= (signed char)PbiClampInt (GetNum (IDC_PB_SEATDX), -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	pInfo->cSeatDy			= (signed char)PbiClampInt (GetNum (IDC_PB_SEATDY), -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	pInfo->byBandSide		= (BYTE)GetNum (IDC_PB_BANDSIDE);
	pInfo->byPair			= (BYTE)PbiClampInt (GetNum (IDC_PB_PAIR), 0, 255);
	pInfo->wVsSec			= (WORD)PbiClampInt (GetNum (IDC_PB_VSSEC), 0, 65535);	/* 範囲外は PinballInfoClamp で丸める（0=既定） */
	PinballInfoClamp (pInfo);
	/* 保存の版と予約領域は、サーバーから受け取った値のまま */
}

/* ========================================================================= */
/* 関数名：GetNum															 */
/* 内容：入力欄の数を取得（空・数字でない時は 0。負の数も読む）				 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static int GetNum(int nID)
{
	char szTmp[64];

	ZeroMemory (szTmp, sizeof (szTmp));
	GetWindowText (GetDlgItem (s_hWndDlg, nID), szTmp, sizeof (szTmp));
	return atoi (szTmp);
}

/* ========================================================================= */
/* 関数名：SetNum															 */
/* 内容：入力欄に数を設定													 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void SetNum(HWND hWnd, int nID, int nNum)
{
	char szTmp[64];

	wsprintf (szTmp, "%d", nNum);
	SetWindowText (GetDlgItem (hWnd, nID), szTmp);
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
	PSUBGAME_PINBALLINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	pInfo = (PSUBGAME_PINBALLINFOSAVE)g_pSubGameInfo->pInfo;
	SetNum (hWnd, IDC_PB_COINITEM, pInfo->wCoinItem);
	SetNum (hWnd, IDC_PB_COINCOUNT, pInfo->byCoinCount);
	SetNum (hWnd, IDC_PB_BALLS, pInfo->byBalls);
	SetNum (hWnd, IDC_PB_SAVESEC, pInfo->bySaveSec);
	SetNum (hWnd, IDC_PB_SCOREBUMPER, pInfo->wScoreBumper);
	SetNum (hWnd, IDC_PB_SCORETARGET, pInfo->wScoreTarget);
	SetNum (hWnd, IDC_PB_SCOREBONUS, pInfo->wScoreBonus);
	SetNum (hWnd, IDC_PB_COMBOMS, pInfo->wComboMs);
	SetNum (hWnd, IDC_PB_DX, pInfo->cDx);
	SetNum (hWnd, IDC_PB_DY, pInfo->cDy);
	SetNum (hWnd, IDC_PB_SEATDX, pInfo->cSeatDx);
	SetNum (hWnd, IDC_PB_SEATDY, pInfo->cSeatDy);
	SetNum (hWnd, IDC_PB_BANDSIDE, pInfo->byBandSide);
	SetNum (hWnd, IDC_PB_PAIR, pInfo->byPair);
	SetNum (hWnd, IDC_PB_VSSEC, pInfo->wVsSec);

	return FALSE;
}

/* Copyright(C)URARA-works 2003-2005 */
