/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgEtcSubGame.cpp										 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/31													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgEtcSubGameEdit.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGame(HWND hWndParent);										/* ダイアログ表示(サブゲーム情報) */
void CloseDlgEtcSubGame(void);													/* ダイアログ終了(サブゲーム情報) */
void EtcSubGameRenewInfo(void);													/* 情報更新 */
void OnMapLbuttonDownDlgEtcSubGame(int x, int y);								/* マップ上で左クリックされた */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void MakeInfo(LPSTR pszInfo);											/* 情報メッセージ作成 */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGame												 */
/* 内容：ダイアログ表示(サブゲーム情報)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
void ShowDlgEtcSubGame(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_OLD_ETC_SUBGAME, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGame												 */
/* 内容：ダイアログ終了(サブゲーム情報)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
void CloseDlgEtcSubGame(void)
{
	CloseDlgEtcSubGameEdit ();
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：EtcSubGameRenewInfo												 */
/* 内容：情報更新															 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
void EtcSubGameRenewInfo(void)
{
	int i;
	HWND hWndTmp;
	char szTmp[1024];

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_ID);
	/* 現在登録中の項目を削除 */
	while (SendMessage (hWndTmp, CB_GETCOUNT, 0, 0) != 0) {
		SendMessage (hWndTmp, CB_DELETESTRING, 0, 0);
	}

	/* 登録数分項目を登録 */
	for (i = 0; i < g_nSubGameCount; i ++) {
		wsprintf (szTmp, "%d", i);
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendMessage(hWndTmp, CB_SETCURSEL, (WPARAM)g_nSelectSubGameID, 0);

	MakeInfo (szTmp);

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_INFO);
	SetWindowText (hWndTmp, szTmp);
}

/* ========================================================================= */
/* 関数名：OnMapLbuttonDownDlgEtcSubGame									 */
/* 内容：マップ上で左クリックされた											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void OnMapLbuttonDownDlgEtcSubGame(int x, int y)
{
	OnMapLbuttonDownDlgEtcSubGameEdit (x, y);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_ADMINSUBGAMEINFO:		/* サブゲーム情報更新 */
		EtcSubGameRenewInfo ();
		break;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	if (g_pSubGameInfo) {
		SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
		SAFE_DELETE (g_pSubGameInfo);
	}
	g_pSubGameInfo = new SUBGAMEINFO;
	ZeroMemory (g_pSubGameInfo, sizeof (SUBGAMEINFO));

	g_nSubGameCount = 0;
	g_nSelectSubGameID = 0;

	/* 最初の情報を取得 */
	SendCmdSUBGAMEINFOGET (0);
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_GET:		/* 取得 */
		if (g_nSelectSubGameID < 0) {
			break;
		}
		SendCmdSUBGAMEINFOGET (g_nSelectSubGameID);
		break;

	case IDC_ID:		/* サブゲームID */
		{
			HWND hWndTmp;
			char szTmp[64];

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_ID);
			switch (nCodeNotify) {
			case CBN_SELCHANGE:		/* 選択項目が変更された */
				g_nSelectSubGameID = SendMessage (hWndTmp, CB_GETCURSEL, 0, 0);
				SendCmdSUBGAMEINFOGET (g_nSelectSubGameID);
				break;

			case CBN_EDITCHANGE:	/* 直接入力で値が変更された */
				GetWindowText (hWndTmp, szTmp, sizeof (szTmp));
				g_nSelectSubGameID = (int)atoi (szTmp);
				break;
			}
		}
		break;

	case IDC_SEND:		/* 送信 */
		SendCmdSUBGAMEINFO (g_nSelectSubGameID);
		break;

	case IDC_CLEAR:		/* クリア */
		if (g_pSubGameInfo == NULL) {
			break;
		}
		if (g_pSubGameInfo) {
			SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
		}
		ZeroMemory (g_pSubGameInfo, sizeof (SUBGAMEINFO));
		EtcSubGameRenewInfo ();
		break;

	case IDC_MODIFY:	/* 編集 */
		ShowDlgEtcSubGameEdit (hWnd);
		break;

	case IDC_ADD:		/* 追加 */
		SendCmdSUBGAMEINFOADD ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeInfo															 */
/* 内容：情報メッセージ作成													 */
/* 日付：2004/08/07															 */
/* ========================================================================= */
static void MakeInfo(LPSTR pszInfo)
{
	char szTmp[1024];

	ZeroMemory (szTmp, sizeof (szTmp));
	wsprintf (pszInfo, "[情報]:\n種別:%s\n",
			aSubGameNameTbl[g_pSubGameInfo->byType]);

	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
		{
			PSUBGAME_BOMBINFOSAVE pInfo = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;

			wsprintf (szTmp,
					"マップID:%d\n"
					"エリア:(%3d, %3d)-(%3d, %3d)\n"
					"受付座標 X:%3d Y:%3d\n"
					"開始座標 X:%3d Y:%3d",
					pInfo->dwMapID,
					pInfo->byX1, pInfo->byY1, pInfo->byX2, pInfo->byY2,
					pInfo->dwX, pInfo->dwY,
					pInfo->dwStartX, pInfo->dwStartY);
		}
		break;

	case SUBGAMEID_MAZE:		/* 迷路 */
		{
			PSUBGAME_MAZEINFOSAVE pInfo = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;

			wsprintf (szTmp,
					"マップID:%d\n"
					"エリア:(%3d, %3d)-(%3d, %3d)\n"
					"スタート地点 X:%3d Y:%3d\n"
					"ゴール地点(移動元) X:%3d Y:%3d\n"
					"ゴール地点(移動先) X:%3d Y:%3d\n",
					pInfo->dwMapID,
					pInfo->byX1, pInfo->byY1, pInfo->byX2, pInfo->byY2,
					pInfo->byStartX, pInfo->byStartY,
					pInfo->byGoalFromX, pInfo->byGoalFromY,
					pInfo->byGoalToX, pInfo->byGoalToY);
		}		
		break;

	case SUBGAMEID_DUNGEON:		/* ダンジョン */
		{
			PSUBGAME_DUNGEONINFOSAVE pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;

			wsprintf (szTmp,
					"マップID:%d\n"
					"エリア:(%3d, %3d)-(%3d, %3d)\n"
					"入り口座標 X:%3d Y:%3d\n"
					"出口座標 X:%3d Y:%3d\n",
					pInfo->dwMapID,
					pInfo->byX1, pInfo->byY1, pInfo->byX2, pInfo->byY2,
					pInfo->byStartX, pInfo->byStartY,
					pInfo->byGoalX, pInfo->byGoalY);
		}		
		break;

	case SUBGAMEID_WAR:			/* 戦場ゲーム */
		{
			PSUBGAME_WARINFOSAVE pInfo = (PSUBGAME_WARINFOSAVE)g_pSubGameInfo->pInfo;
			LPCSTR aszMember[] = {"３人", "４人", "z"};

			wsprintf (szTmp,
					"マップID:%d\n"
					"参加メンバー数:%s\n"
					"参加受付 X:%d Y:%d\n"
					"審判キャラ X:%d Y:%d\n"
					"ｴﾘｱ (%d,%d)-(%d,%d)\n"
					"HP　 X:%d Y:%d  X:%d Y:%d\n"
					"蘇生 X:%d Y:%d  X:%d Y:%d\n"
					"開始 X:%d Y:%d  X:%d Y:%d\n"
					"城   X:%d Y:%d  X:%d Y:%d\n",
					pInfo->dwMapID,
					aszMember[pInfo->byMember],
					pInfo->byX, pInfo->byY,
					pInfo->byJudgeX, pInfo->byJudgeY,
					pInfo->byX1, pInfo->byY1,
					pInfo->byX2, pInfo->byY2,
					pInfo->byHP1X, pInfo->byHPY,
					pInfo->byHP2X, pInfo->byHPY,
					pInfo->byRes1X, pInfo->byResY,
					pInfo->byRes2X, pInfo->byResY,
					pInfo->byStart1X, pInfo->byStartY,
					pInfo->byStart2X, pInfo->byStartY,
					pInfo->byCastle1X, pInfo->byCastleY,
					pInfo->byCastle2X, pInfo->byCastleY
					);
		}
		break;

	case SUBGAMEID_SUGOROKU:		/* すごろく */
		{
			PSUBGAME_SUGOROKUINFOSAVE pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;

			wsprintf (szTmp,
					"マップID:%d\n"
					"受付座標 X:%3d Y:%3d\n",
					pInfo->dwMapID,
					pInfo->byX, pInfo->byY);
		}
		break;
	}

	strcat (pszInfo, szTmp);
}

/* Copyright(C)URARA-works 2003-2005 */
