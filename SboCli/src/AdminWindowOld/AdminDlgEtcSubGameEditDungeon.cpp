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

/* 種類（コンボボックスの並び順） */
enum {
	DGKIND_RANDOM = 0,											/* 気まぐれダンジョン */
	DGKIND_DEFENSE,												/* 拠点防衛 */
};

/* 拠点防衛の時だけ出す欄 */
static const int s_anDefenseOnly[] = {
	IDC_DGLBL_FIX, IDC_DGLBL_FIXX, IDC_DGLBL_FIXY, IDC_DGFIX_STARTX, IDC_DGFIX_STARTY,
	IDC_DGLBL_DEFHDR,
	IDC_DGLBL_DEF1, IDC_DGDEF_HP, IDC_DGLBL_DEF2, IDC_DGDEF_TMPL,
	IDC_DGLBL_DEF3, IDC_DGDEF_WAVES, IDC_DGLBL_DEF4, IDC_DGDEF_PREP,
	IDC_DGLBL_DEF5, IDC_DGDEF_WAVESEC, IDC_DGLBL_DEF6, IDC_DGDEF_SPRING,
	IDC_DGLBL_DEF7, IDC_DGDEF_SPRINGX, IDC_DGLBL_DEF8, IDC_DGDEF_SPRINGY,
	IDC_DGLBL_DEF9, IDC_DGDEF_SPRINGPCT,
};
/* 気まぐれダンジョンの時だけ出す欄（拠点防衛では隠す） */
static const int s_anRandomOnly[] = {
	IDC_DGLBL_CHIP1, IDC_DGLBL_CHIP2, IDC_DGLBL_CHIP3, IDC_DGLBL_CHIP4,
	IDC_GRPSTART, IDC_GRPGOAL, IDC_GRPROAD, IDC_GRPBLOCK,
	IDC_DGLBL_THEME, IDC_DGTHEME, IDC_DGLBL_THEMEHINT,
	IDC_DGLBL_REBUILD, IDC_DGREBUILD_MODE, IDC_DGLBL_REBUILDHINT,
	IDC_DGLBL_CLEAR, IDC_DGCLEAR_MODE, IDC_DGLBL_CLEARHINT,
};

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
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static int GetKind(HWND hWnd);													/* 選択中の種類を取得 */
static void ApplyKind(HWND hWnd);												/* 種類に合わせて見出しと欄の表示を切り替え */

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
	/* ダンジョン生成のテーマ番号（0=従来） */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGTHEME), szTmp, sizeof (szTmp));
	pInfo->dwThemeID = (DWORD)atoi (szTmp);
	/* 番外編：入る姿の数・番号・表示名 */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGTMPL_COUNT), szTmp, sizeof (szTmp));
	pInfo->byTmplCount = (BYTE)max (0, min (atoi (szTmp), MAX_DGTMPL));
	for (int k = 0; k < MAX_DGTMPL; k ++) {
		GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGTMPL_ID1 + k), szTmp, sizeof (szTmp));
		pInfo->wTmplID[k] = (WORD)atoi (szTmp);
		GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGTMPL_NAME1 + k), pInfo->szTmplName[k], sizeof (pInfo->szTmplName[k]));
	}
	/* 作り直しの方式（0=気まぐれ 1=日替わり 2=固定）・クリア条件（0=ゴール 1=敵を全部倒す 2=拠点防衛） */
	if (GetKind (s_hWndDlg) == DGKIND_DEFENSE) {
		/* 拠点防衛は 2・2 で固定 */
		pInfo->byRebuildMode = 2;
		pInfo->byClearMode = 2;
	} else {
		GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGREBUILD_MODE), szTmp, sizeof (szTmp));
		pInfo->byRebuildMode = (BYTE)atoi (szTmp);
		GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGCLEAR_MODE), szTmp, sizeof (szTmp));
		pInfo->byClearMode = (BYTE)atoi (szTmp);
	}
	/* 拠点防衛の設定（0=既定） */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_HP), szTmp, sizeof (szTmp));
	pInfo->wDefBaseHP = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_TMPL), szTmp, sizeof (szTmp));
	pInfo->wDefBaseStTmp = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_WAVES), szTmp, sizeof (szTmp));
	pInfo->byDefWaveCount = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_PREP), szTmp, sizeof (szTmp));
	pInfo->byDefPrepSec = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_WAVESEC), szTmp, sizeof (szTmp));
	pInfo->wDefWaveMaxSec = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_SPRING), szTmp, sizeof (szTmp));
	pInfo->byDefSpringRange = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_SPRINGX), szTmp, sizeof (szTmp));
	pInfo->byDefSpringX = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_SPRINGY), szTmp, sizeof (szTmp));
	pInfo->byDefSpringY = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGFIX_STARTX), szTmp, sizeof (szTmp));
	pInfo->byFixStartX = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGFIX_STARTY), szTmp, sizeof (szTmp));
	pInfo->byFixStartY = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGDEF_SPRINGPCT), szTmp, sizeof (szTmp));
	pInfo->byDefSpringPct = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_DGMESSAGES), pInfo->szMessages, sizeof (pInfo->szMessages));
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
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

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
	/* ダンジョン生成のテーマ番号・番外編の設定 */
	wsprintf (szTmp, "%lu", pInfo->dwThemeID);
	SetWindowText (GetDlgItem (hWnd, IDC_DGTHEME), szTmp);
	wsprintf (szTmp, "%d", pInfo->byTmplCount);
	SetWindowText (GetDlgItem (hWnd, IDC_DGTMPL_COUNT), szTmp);
	for (int k = 0; k < MAX_DGTMPL; k ++) {
		wsprintf (szTmp, "%d", pInfo->wTmplID[k]);
		SetWindowText (GetDlgItem (hWnd, IDC_DGTMPL_ID1 + k), szTmp);
		SetWindowText (GetDlgItem (hWnd, IDC_DGTMPL_NAME1 + k), pInfo->szTmplName[k]);
	}
	wsprintf (szTmp, "%d", pInfo->byRebuildMode);
	SetWindowText (GetDlgItem (hWnd, IDC_DGREBUILD_MODE), szTmp);
	wsprintf (szTmp, "%d", pInfo->byClearMode);
	SetWindowText (GetDlgItem (hWnd, IDC_DGCLEAR_MODE), szTmp);
	wsprintf (szTmp, "%d", pInfo->wDefBaseHP);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_HP), szTmp);
	wsprintf (szTmp, "%d", pInfo->wDefBaseStTmp);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_TMPL), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefWaveCount);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_WAVES), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefPrepSec);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_PREP), szTmp);
	wsprintf (szTmp, "%d", pInfo->wDefWaveMaxSec);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_WAVESEC), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefSpringRange);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_SPRING), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefSpringX);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_SPRINGX), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefSpringY);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_SPRINGY), szTmp);
	wsprintf (szTmp, "%d", pInfo->byFixStartX);
	SetWindowText (GetDlgItem (hWnd, IDC_DGFIX_STARTX), szTmp);
	wsprintf (szTmp, "%d", pInfo->byFixStartY);
	SetWindowText (GetDlgItem (hWnd, IDC_DGFIX_STARTY), szTmp);
	wsprintf (szTmp, "%d", pInfo->byDefSpringPct);
	SetWindowText (GetDlgItem (hWnd, IDC_DGDEF_SPRINGPCT), szTmp);
	SendMessage (GetDlgItem (hWnd, IDC_DGMESSAGES), EM_LIMITTEXT, MAX_DGTEXT - 1, 0);
	SetWindowText (GetDlgItem (hWnd, IDC_DGMESSAGES), pInfo->szMessages);

	/* 種類（作り直し 2・クリア条件 2 なら拠点防衛、それ以外は気まぐれ） */
	HWND hWndKind = GetDlgItem (hWnd, IDC_DGKIND);
	SendMessage (hWndKind, CB_ADDSTRING, 0, (LPARAM)"気まぐれダンジョン");
	SendMessage (hWndKind, CB_ADDSTRING, 0, (LPARAM)"拠点防衛");
	if (pInfo->byRebuildMode == 2 && pInfo->byClearMode == 2) {
		SendMessage (hWndKind, CB_SETCURSEL, DGKIND_DEFENSE, 0);
	} else {
		SendMessage (hWndKind, CB_SETCURSEL, DGKIND_RANDOM, 0);
	}
	ApplyKind (hWnd);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：GetKind															 */
/* 内容：選択中の種類を取得													 */
/* ========================================================================= */
static int GetKind(HWND hWnd)
{
	int nSel;

	nSel = (int)SendMessage (GetDlgItem (hWnd, IDC_DGKIND), CB_GETCURSEL, 0, 0);
	if (nSel == DGKIND_DEFENSE) {
		return DGKIND_DEFENSE;
	}
	return DGKIND_RANDOM;
}

/* ========================================================================= */
/* 関数名：ApplyKind														 */
/* 内容：種類に合わせて見出しと欄の表示を切り替え							 */
/* ========================================================================= */
static void ApplyKind(HWND hWnd)
{
	int i, nKind;
	BOOL bDefense;

	nKind = GetKind (hWnd);
	bDefense = (nKind == DGKIND_DEFENSE);

	/* 見出し */
	if (bDefense) {
		SetWindowText (GetDlgItem (hWnd, IDC_DGLBL_START), "受付そばの入口：");
		SetWindowText (GetDlgItem (hWnd, IDC_DGLBL_GOAL), "拠点の位置：");
		SetWindowText (GetDlgItem (hWnd, IDC_DGLBL_FIX), "入ってすぐの位置（0,0=入口と同じ）：");
	} else {
		SetWindowText (GetDlgItem (hWnd, IDC_DGLBL_START), "入り口：");
		SetWindowText (GetDlgItem (hWnd, IDC_DGLBL_GOAL), "出口：");
	}

	/* 拠点防衛だけの欄 */
	for (i = 0; i < (int)(sizeof (s_anDefenseOnly) / sizeof (s_anDefenseOnly[0])); i ++) {
		ShowWindow (GetDlgItem (hWnd, s_anDefenseOnly[i]), bDefense ? SW_SHOW : SW_HIDE);
	}
	/* 気まぐれだけの欄 */
	for (i = 0; i < (int)(sizeof (s_anRandomOnly) / sizeof (s_anRandomOnly[0])); i ++) {
		ShowWindow (GetDlgItem (hWnd, s_anRandomOnly[i]), bDefense ? SW_HIDE : SW_SHOW);
	}
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	char szTmp[64];

	switch (nID) {
	case IDC_DGKIND:		/* 種類 */
		if (nCodeNotify != CBN_SELCHANGE) {
			break;
		}
		if (GetKind (hWnd) == DGKIND_DEFENSE) {
			/* 拠点防衛は 作り直し 2・クリア条件 2 に固定 */
			SetWindowText (GetDlgItem (hWnd, IDC_DGREBUILD_MODE), "2");
			SetWindowText (GetDlgItem (hWnd, IDC_DGCLEAR_MODE), "2");
		} else {
			/* 拠点防衛の値(2・2)が残っていたら気まぐれの 0・0 に戻す */
			GetWindowText (GetDlgItem (hWnd, IDC_DGREBUILD_MODE), szTmp, sizeof (szTmp));
			int nRebuild = atoi (szTmp);
			GetWindowText (GetDlgItem (hWnd, IDC_DGCLEAR_MODE), szTmp, sizeof (szTmp));
			int nClear = atoi (szTmp);
			if (nRebuild == 2 && nClear == 2) {
				SetWindowText (GetDlgItem (hWnd, IDC_DGREBUILD_MODE), "0");
				SetWindowText (GetDlgItem (hWnd, IDC_DGCLEAR_MODE), "0");
			}
		}
		ApplyKind (hWnd);
		InvalidateRect (hWnd, NULL, TRUE);
		break;
	}
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

	/* 拠点防衛では絵を選ばない（隠している枠の上のクリックは無視） */
	if (GetKind (hWnd) == DGKIND_DEFENSE) {
		return;
	}

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
