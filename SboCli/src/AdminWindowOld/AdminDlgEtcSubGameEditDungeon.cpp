/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(ダンジョン)ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgEtcSubGameEditDungeon.cpp							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/08													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "SelectMapPartsWindow.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static WORD s_wStart = 0,										/* 入り口 */
			s_wGoal = 0,										/* 出口 */
			s_wRoad = 0,										/* 道 */
			s_wBlock = 0;										/* 壁 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditDungeon(HWND hWndParent);								/* ダイアログ表示(サブゲーム情報(ダンジョン)) */
void CloseDlgEtcSubGameEditDungeon(void);										/* ダイアログ終了(サブゲーム情報(ダンジョン)) */
void GetDataDlgEtcSubGameEditDungeon(void);										/* 入力データを取得(サブゲーム情報(ダンジョン)) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_LBUTTONDOWN) */
static void OnPaint(HWND);														/* メッセージハンドラ(WM_PAINT) */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditDungeon										 */
/* 内容：ダイアログ表示(サブゲーム情報(ダンジョン))							 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditDungeon(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_DUNGEON, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditDungeon									 */
/* 内容：ダイアログ終了(サブゲーム情報(ダンジョン))							 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditDungeon(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditDungeon									 */
/* 内容：入力データを取得(サブゲーム情報(ダンジョン))						 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditDungeon(void)
{
	char szTmp[64];
	PSUBGAME_DUNGEONINFOSAVE pInfo;

	pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MAPID), szTmp, sizeof (szTmp));
	pInfo->dwMapID = (DWORD)atoi (szTmp);
	/* 入り口(X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_STARTX), szTmp, sizeof (szTmp));
	pInfo->byStartX = (BYTE)atoi (szTmp);
	/* 入り口(Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_STARTY), szTmp, sizeof (szTmp));
	pInfo->byStartY = (BYTE)atoi (szTmp);
	/* エリア(開始X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAX1), szTmp, sizeof (szTmp));
	pInfo->byX1 = (BYTE)atoi (szTmp);
	/* エリア(開始Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAY1), szTmp, sizeof (szTmp));
	pInfo->byY1 = (BYTE)atoi (szTmp);
	/* エリア(終了X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAX2), szTmp, sizeof (szTmp));
	pInfo->byX2 = (BYTE)atoi (szTmp);
	/* エリア(終了Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_AREAY2), szTmp, sizeof (szTmp));
	pInfo->byY2 = (BYTE)atoi (szTmp);
	/* 出口(X) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_GOALX), szTmp, sizeof (szTmp));
	pInfo->byGoalX = (BYTE)atoi (szTmp);
	/* 出口(Y) */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_GOALY), szTmp, sizeof (szTmp));
	pInfo->byGoalY = (BYTE)atoi (szTmp);
	/* 入り口 */
	pInfo->wChipIDStart = s_wStart;
	/* 出口 */
	pInfo->wChipIDGoal = s_wGoal;
	/* 道 */
	pInfo->wChipIDRoad = s_wRoad;
	/* 壁 */
	pInfo->wChipIDBlock = s_wBlock;
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);
	HANDLE_MSG (hWnd, WM_PAINT, OnPaint);

	case WM_ADMINSELECTMAPPARTS:	/* マップパーツ選択 */
		{
			PSUBGAME_DUNGEONINFOSAVE pInfo;

			pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
			switch (wParam) {
			case IDC_GRPSTART:			/* 入り口 */
				s_wStart = (WORD)lParam;
				break;
			case IDC_GRPGOAL:			/* 出口 */
				s_wGoal = (WORD)lParam;
				break;
			case IDC_GRPROAD:			/* 道 */
				s_wRoad = (WORD)lParam;
				break;
			case IDC_GRPBLOCK:			/* 壁 */
				s_wBlock = (WORD)lParam;
				break;
			}
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
/* 日付：2004/08/08															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	char szTmp[64];
	PSUBGAME_DUNGEONINFOSAVE pInfo;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	wsprintf (szTmp, "%d", pInfo->dwMapID);
	SetWindowText (GetDlgItem (hWnd, IDC_MAPID), szTmp);
	/* 入り口(X) */
	wsprintf (szTmp, "%d", pInfo->byStartX);
	SetWindowText (GetDlgItem (hWnd, IDC_STARTX), szTmp);
	/* 入り口(Y) */
	wsprintf (szTmp, "%d", pInfo->byStartY);
	SetWindowText (GetDlgItem (hWnd, IDC_STARTY), szTmp);
	/* エリア(開始X) */
	wsprintf (szTmp, "%d", pInfo->byX1);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAX1), szTmp);
	/* エリア(開始Y) */
	wsprintf (szTmp, "%d", pInfo->byY1);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAY1), szTmp);
	/* エリア(終了X) */
	wsprintf (szTmp, "%d", pInfo->byX2);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAX2), szTmp);
	/* エリア(終了Y) */
	wsprintf (szTmp, "%d", pInfo->byY2);
	SetWindowText (GetDlgItem (hWnd, IDC_AREAY2), szTmp);
	/* 出口(X) */
	wsprintf (szTmp, "%d", pInfo->byGoalX);
	SetWindowText (GetDlgItem (hWnd, IDC_GOALX), szTmp);
	/* 出口(Y) */
	wsprintf (szTmp, "%d", pInfo->byGoalY);
	SetWindowText (GetDlgItem (hWnd, IDC_GOALY), szTmp);
	/* 入り口 */
	s_wStart = pInfo->wChipIDStart;
	/* 出口 */
	s_wGoal = pInfo->wChipIDGoal;
	/* 道 */
	s_wRoad = pInfo->wChipIDRoad;
	/* 壁 */
	s_wBlock = pInfo->wChipIDBlock;

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnLButtonDown													 */
/* 内容：メッセージハンドラ(WM_LBUTTONDOWN)									 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	HWND hWndTmp;
	POINT pt;
	RECT rc;

	pt.x = x;
	pt.y = y;
	ClientToScreen (hWnd, &pt);

	hWndTmp = GetDlgItem (hWnd, IDC_GRPSTART);
	GetWindowRect (hWndTmp, &rc);
	/* [入り口]でクリックされた？ */
	if (PtInRect (&rc, pt)) {
		ShowSelectMapPartsWindow (hWnd, IDC_GRPSTART);
		return;
	}

	hWndTmp = GetDlgItem (hWnd, IDC_GRPGOAL);
	GetWindowRect (hWndTmp, &rc);
	/* [出口]でクリックされた？ */
	if (PtInRect (&rc, pt)) {
		ShowSelectMapPartsWindow (hWnd, IDC_GRPGOAL);
		return;
	}

	hWndTmp = GetDlgItem (hWnd, IDC_GRPROAD);
	GetWindowRect (hWndTmp, &rc);
	/* [道]でクリックされた？ */
	if (PtInRect (&rc, pt)) {
		ShowSelectMapPartsWindow (hWnd, IDC_GRPROAD);
		return;
	}

	hWndTmp = GetDlgItem (hWnd, IDC_GRPBLOCK);
	GetWindowRect (hWndTmp, &rc);
	/* [壁]でクリックされた？ */
	if (PtInRect (&rc, pt)) {
		ShowSelectMapPartsWindow (hWnd, IDC_GRPBLOCK);
		return;
	}
}

/* ========================================================================= */
/* 関数名：OnPaint															 */
/* 内容：メッセージハンドラ(WM_PAINT)										 */
/* 日付：2004/08/08															 */
/* ========================================================================= */
void OnPaint(HWND hWnd)
{
	HDC hDC, hDCTmp;
	PAINTSTRUCT ps;
	int i, nNo, nID, nParts, xx, yy, nControl;
	WORD wID;
	HWND hWndTmp;
	PMAPPARTSINFO pParts;
	CImg32 DibTmp;
	PSUBGAME_DUNGEONINFOSAVE pInfo;

	hDC = BeginPaint(hWnd, &ps);

	DibTmp.Create (32, 32);
	/* パーツ画像表示 */
	HDC hDCParts = DibTmp.Lock ();

	pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;

	for (i = 0; i < 4; i ++) {
		switch (i) {
		case 0:		/* 入り口 */
			nControl = IDC_GRPSTART;
			wID = s_wStart;
			break;
		case 1:		/* 出口 */
			nControl = IDC_GRPGOAL;
			wID = s_wGoal;
			break;
		case 2:		/* 道 */
			nControl = IDC_GRPROAD;
			wID = s_wRoad;
			break;
		case 3:		/* 壁 */
			nControl = IDC_GRPBLOCK;
			wID = s_wBlock;
			break;
		}

		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[wID];

		nParts = pParts->wBackID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			if (pAnime == NULL) {
				continue;
			}
			nParts = pAnime->wBackID;
		}

		nNo = (nParts) / (g_byMapGrpCountX * g_byMapGrpCountY);
		nID = (nParts) % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = nID % 16 * 32;
		yy = nID / 16 * 32;

		DibTmp.Blt (0, 0, 32, 32, g_apDibMap[nNo], xx, yy);


		nParts = pParts->wGrpID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			nParts = pAnime->wGrpID;
		}

		nNo = (nParts) / (g_byMapGrpCountX * g_byMapGrpCountY);
		nID = (nParts) % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = nID % 16 * 32;
		yy = nID / 16 * 32;

		DibTmp.Blt (0, 0, 32, 32, g_apDibMap[nNo], xx, yy, TRUE);

		hWndTmp = GetDlgItem (hWnd, nControl);
		hDCTmp = GetWindowDC (hWndTmp);

		/* 画面に転送 */
		BitBlt (hDCTmp, 0, 0, 32, 32, hDCParts, 0, 0, SRCCOPY);
		ReleaseDC (hWndTmp, hDCTmp);
	}

	DibTmp.Unlock ();
	EndPaint (hWnd, &ps);
}

/* Copyright(C)URARA-works 2003-2005 */
