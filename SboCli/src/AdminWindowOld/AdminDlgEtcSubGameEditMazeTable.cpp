/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(迷路対戦の台)ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgEtcSubGameEditMazeTable.cpp							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "common/MazeTable.h"
#include "SelectMapPartsWindow.h"
#include "SelectCharGrpWindow.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* CPU強さの表示名（番号＝byCpuLevel） */
static const char *s_apszCpuLevel[MAZE_CPU_LEVEL_MAX + 1] = { "なし", "よわい", "ふつう", "つよい" };

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditMazeTable(HWND hWndParent);							/* ダイアログ表示(サブゲーム情報(迷路対戦の台)) */
void CloseDlgEtcSubGameEditMazeTable(void);										/* ダイアログ終了(サブゲーム情報(迷路対戦の台)) */
void GetDataDlgEtcSubGameEditMazeTable(void);									/* 入力データを取得(サブゲーム情報(迷路対戦の台)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnPaint(HWND);														/* メッセージハンドラ(WM_PAINT) */
static void SetNumber(HWND hWnd, int nID, int nValue);							/* 欄に数を入れる */
static int GetNumber(int nID, int nMin, int nMax);								/* 欄の数を範囲内で取得 */
static void PaintParts(HWND hWnd, int nPreviewID, int nEditID);					/* マップパーツのプレビュー */
static void PaintCharGrp(HWND hWnd, int nPreviewID, int nEditID);				/* キャラ画像のプレビュー(1コマ) */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditMazeTable									 */
/* 内容：ダイアログ表示(サブゲーム情報(迷路対戦の台))						 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditMazeTable(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_MAZETABLE, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditMazeTable									 */
/* 内容：ダイアログ終了(サブゲーム情報(迷路対戦の台))						 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditMazeTable(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditMazeTable								 */
/* 内容：入力データを取得(サブゲーム情報(迷路対戦の台))						 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditMazeTable(void)
{
	PSUBGAME_MAZETABLEINFOSAVE pInfo;
	int nSel;

	pInfo = (PSUBGAME_MAZETABLEINFOSAVE)g_pSubGameInfo->pInfo;
	/* 台の名前 */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_NAME), pInfo->szName, sizeof (pInfo->szName));
	/* 盤の位置・座る場所（範囲は MazeTableInfoFix でも丸める） */
	pInfo->chDx = (char)GetNumber (IDC_MT_DX, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	pInfo->chDy = (char)GetNumber (IDC_MT_DY, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	pInfo->chSdx = (char)GetNumber (IDC_MT_SDX, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	pInfo->chSdy = (char)GetNumber (IDC_MT_SDY, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	pInfo->chSdx2 = (char)GetNumber (IDC_MT_SDX2, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	pInfo->chSdy2 = (char)GetNumber (IDC_MT_SDY2, -MAZE_PARAM_LIMIT, MAZE_PARAM_LIMIT);
	/* 床パーツA・B・隙間パーツ */
	pInfo->wFloorA = (WORD)GetNumber (IDC_MT_FLOORA, 0, 65535);
	pInfo->wFloorB = (WORD)GetNumber (IDC_MT_FLOORB, 0, 65535);
	pInfo->wGap = (WORD)GetNumber (IDC_MT_GAP, 0, 65535);
	/* CPU強さ */
	nSel = (int)SendMessage (GetDlgItem (s_hWndDlg, IDC_MT_LEVEL), CB_GETCURSEL, 0, 0);
	pInfo->byCpuLevel = (BYTE)((nSel < 0) ? 0 : (nSel > MAZE_CPU_LEVEL_MAX) ? MAZE_CPU_LEVEL_MAX : nSel);
	/* CPUのキャラ画像・名前 */
	pInfo->wCpuGrpID = (WORD)GetNumber (IDC_MT_CPUGRP, 0, 65535);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MT_CPUNAME), pInfo->szCpuName, sizeof (pInfo->szCpuName));
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
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);
	HANDLE_MSG (hWnd, WM_PAINT, OnPaint);

	case WM_ADMINSELECTMAPPARTS:	/* マップパーツ選択 */
		switch (wParam) {
		case IDC_MT_SELA:		/* 床パーツA */
			SetNumber (hWnd, IDC_MT_FLOORA, (int)lParam);
			break;
		case IDC_MT_SELB:		/* 床パーツB */
			SetNumber (hWnd, IDC_MT_FLOORB, (int)lParam);
			break;
		case IDC_MT_SELGAP:		/* 隙間パーツ */
			SetNumber (hWnd, IDC_MT_GAP, (int)lParam);
			break;
		}
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	case WM_ADMINSELECTCHARGRP:		/* キャラ画像選択（種別は上位、番号は下位） */
		if (wParam == IDC_MT_SELCPU) {
			if (HIWORD (lParam) != CHARGRP_MAIN_NORMAL) {
				/* CPUの絵は NPC の wGrpID と同じ（通常のキャラ画像）だけ */
				MessageBox (hWnd, "CPUの絵には、種別の一番上（通常のキャラ画像）の中から選んでください。", "迷路対戦の台", MB_OK);
			} else {
				SetNumber (hWnd, IDC_MT_CPUGRP, (int)LOWORD (lParam));
			}
		}
		InvalidateRect (hWnd, NULL, TRUE);
		break;

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
	HWND hWndTmp;
	PSUBGAME_MAZETABLEINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	/* 「CPU強さ」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_MT_LEVEL);
	for (i = 0; i <= MAZE_CPU_LEVEL_MAX; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)s_apszCpuLevel[i]);
	}

	pInfo = (PSUBGAME_MAZETABLEINFOSAVE)g_pSubGameInfo->pInfo;
	SetWindowText (GetDlgItem (hWnd, IDC_NAME), pInfo->szName);
	SendMessage (GetDlgItem (hWnd, IDC_NAME), EM_LIMITTEXT, sizeof (pInfo->szName) - 1, 0);
	SetNumber (hWnd, IDC_MT_DX, pInfo->chDx);
	SetNumber (hWnd, IDC_MT_DY, pInfo->chDy);
	SetNumber (hWnd, IDC_MT_SDX, pInfo->chSdx);
	SetNumber (hWnd, IDC_MT_SDY, pInfo->chSdy);
	SetNumber (hWnd, IDC_MT_SDX2, pInfo->chSdx2);
	SetNumber (hWnd, IDC_MT_SDY2, pInfo->chSdy2);
	SetNumber (hWnd, IDC_MT_FLOORA, pInfo->wFloorA);
	SetNumber (hWnd, IDC_MT_FLOORB, pInfo->wFloorB);
	SetNumber (hWnd, IDC_MT_GAP, pInfo->wGap);
	SendMessage (hWndTmp, CB_SETCURSEL, (pInfo->byCpuLevel > MAZE_CPU_LEVEL_MAX) ? MAZE_CPU_LEVEL_MAX : pInfo->byCpuLevel, 0);
	SetNumber (hWnd, IDC_MT_CPUGRP, pInfo->wCpuGrpID);
	SetWindowText (GetDlgItem (hWnd, IDC_MT_CPUNAME), pInfo->szCpuName);
	SendMessage (GetDlgItem (hWnd, IDC_MT_CPUNAME), EM_LIMITTEXT, sizeof (pInfo->szCpuName) - 1, 0);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_MT_SELA:		/* 床パーツA 選択 */
	case IDC_MT_SELB:		/* 床パーツB 選択 */
	case IDC_MT_SELGAP:		/* 隙間パーツ 選択 */
		if (nCodeNotify == BN_CLICKED) {
			ShowSelectMapPartsWindow (hWnd, nID);
		}
		break;

	case IDC_MT_SELCPU:		/* CPUのキャラ画像 選択 */
		if (nCodeNotify == BN_CLICKED) {
			ShowSelectCharGrpWindow (hWnd, nID);
		}
		break;

	case IDC_MT_FLOORA:		/* 番号を手で直した時もプレビューを更新 */
	case IDC_MT_FLOORB:
	case IDC_MT_GAP:
	case IDC_MT_CPUGRP:
		if (nCodeNotify == EN_CHANGE) {
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnPaint															 */
/* 内容：メッセージハンドラ(WM_PAINT)										 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;

	BeginPaint (hWnd, &ps);
	PaintParts (hWnd, IDC_MT_PVA, IDC_MT_FLOORA);
	PaintParts (hWnd, IDC_MT_PVB, IDC_MT_FLOORB);
	PaintParts (hWnd, IDC_MT_PVGAP, IDC_MT_GAP);
	PaintCharGrp (hWnd, IDC_MT_PVCPU, IDC_MT_CPUGRP);
	EndPaint (hWnd, &ps);
}

/* ========================================================================= */
/* 関数名：SetNumber														 */
/* 内容：欄に数を入れる														 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void SetNumber(HWND hWnd, int nID, int nValue)
{
	char szTmp[16];

	wsprintf (szTmp, "%d", nValue);
	SetWindowText (GetDlgItem (hWnd, nID), szTmp);
}

/* ========================================================================= */
/* 関数名：GetNumber														 */
/* 内容：欄の数を範囲内で取得												 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static int GetNumber(int nID, int nMin, int nMax)
{
	char szTmp[16];
	int n;

	GetWindowText (GetDlgItem (s_hWndDlg, nID), szTmp, sizeof (szTmp));
	n = atoi (szTmp);
	return (n < nMin) ? nMin : (n > nMax) ? nMax : n;
}

/* ========================================================================= */
/* 関数名：PaintParts														 */
/* 内容：マップパーツのプレビュー(32x32。範囲外・読み込み前は何も描かない)	 */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void PaintParts(HWND hWnd, int nPreviewID, int nEditID)
{
	int nNo, nID, nParts, xx, yy, nValue;
	HWND hWndTmp;
	HDC hDCTmp;
	PMAPPARTSINFO pParts;
	CImg32 DibTmp;

	nValue = GetNumber (nEditID, 0, 65535);
	if ((g_pMapPartsInfo == NULL) || (nValue >= g_wCountMapParts)) {
		return;
	}
	pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[nValue];

	DibTmp.Create (32, 32);
	HDC hDCParts = DibTmp.Lock ();

	nParts = pParts->wBackID;
	if (pParts->byAnimeCount) {
		PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
		if (pAnime == NULL) {
			DibTmp.Unlock ();
			return;
		}
		nParts = pAnime->wBackID;
	}
	nNo = nParts / (g_byMapGrpCountX * g_byMapGrpCountY);
	nID = nParts % (g_byMapGrpCountX * g_byMapGrpCountY);
	xx = nID % 16 * 32;
	yy = nID / 16 * 32;
	DibTmp.Blt (0, 0, 32, 32, g_apDibMap[nNo], xx, yy);

	nParts = pParts->wGrpID;
	if (pParts->byAnimeCount) {
		nParts = pParts->pAnimeInfo[0].wGrpID;
	}
	nNo = nParts / (g_byMapGrpCountX * g_byMapGrpCountY);
	nID = nParts % (g_byMapGrpCountX * g_byMapGrpCountY);
	xx = nID % 16 * 32;
	yy = nID / 16 * 32;
	DibTmp.Blt (0, 0, 32, 32, g_apDibMap[nNo], xx, yy, TRUE);

	hWndTmp = GetDlgItem (hWnd, nPreviewID);
	hDCTmp = GetWindowDC (hWndTmp);
	BitBlt (hDCTmp, 0, 0, 32, 32, hDCParts, 0, 0, SRCCOPY);
	ReleaseDC (hWndTmp, hDCTmp);

	DibTmp.Unlock ();
}

/* ========================================================================= */
/* 関数名：PaintCharGrp														 */
/* 内容：キャラ画像のプレビュー(通常の画像の下向き1コマ目。0・範囲外は描かない) */
/* 日付：2026/11/27															 */
/* ========================================================================= */
static void PaintCharGrp(HWND hWnd, int nPreviewID, int nEditID)
{
	int nNo, nValue;
	HWND hWndTmp;
	HDC hDCTmp;
	CImg32 DibTmp;

	nValue = GetNumber (nEditID, 0, 65535);
	nNo = nValue / 16;
	hWndTmp = GetDlgItem (hWnd, nPreviewID);
	if ((nValue == 0) || (nNo >= g_wCountCharGrp)) {
		/* 絵が無い時は枠の中を白で消す */
		RECT rcClear = { 0, 0, 32, 32 };
		hDCTmp = GetWindowDC (hWndTmp);
		FillRect (hDCTmp, &rcClear, (HBRUSH)GetStockObject (WHITE_BRUSH));
		ReleaseDC (hWndTmp, hDCTmp);
		return;
	}

	DibTmp.Create (32, 32);
	HDC hDCChar = DibTmp.Lock ();
	DibTmp.Blt (0, 0, 32, 32, g_apDibChar[nNo], 1 * 64, 32 * (nValue % 16), TRUE);		/* 下向き・アニメ0コマ目 */

	hDCTmp = GetWindowDC (hWndTmp);
	BitBlt (hDCTmp, 0, 0, 32, 32, hDCChar, 0, 0, SRCCOPY);
	ReleaseDC (hWndTmp, hDCTmp);

	DibTmp.Unlock ();
}

/* Copyright(C)URARA-works 2003-2005 */
