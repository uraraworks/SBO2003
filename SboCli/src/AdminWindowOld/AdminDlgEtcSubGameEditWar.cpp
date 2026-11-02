/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		サブゲーム情報(戦場ゲーム)ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgEtcSubGameEditWar.cpp								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/09													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static PSUBGAME_WARINFOSAVE	s_pWarInfo = NULL;					/* 編集中の情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditWar(HWND hWndParent);								/* ダイアログ表示(サブゲーム情報(戦場ゲーム)) */
void CloseDlgEtcSubGameEditWar(void);										/* ダイアログ終了(サブゲーム情報(戦場ゲーム)) */
void GetDataDlgEtcSubGameEditWar(void);										/* 入力データを取得(サブゲーム情報(戦場ゲーム)) */
void OnMapLbuttonDownDlgEtcSubGameEditWar(int x, int y);					/* マップ上で左クリックされた */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);				/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);			/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);	/* メッセージハンドラ(WM_COMMAND) */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam);				/* メッセージハンドラ(WM_INITEND) */
static void OnAdminPosLeft(HWND hWnd, int x, int y);						/* メッセージハンドラ(WM_ADMINPOSLEFT) */
static void RenewInfo(void);												/* 情報更新 */

/* ========================================================================= */
/* 関数名：ShowDlgEtcSubGameEditWar											 */
/* 内容：ダイアログ表示(サブゲーム情報(戦場ゲーム))							 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void ShowDlgEtcSubGameEditWar(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ETC_SUBGAME_EDIT_WAR, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgEtcSubGameEditWar										 */
/* 内容：ダイアログ終了(サブゲーム情報(戦場ゲーム))							 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void CloseDlgEtcSubGameEditWar(void)
{
	DestroyWindow (s_hWndDlg);
	SAFE_DELETE (s_pWarInfo);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：GetDataDlgEtcSubGameEditWar										 */
/* 内容：入力データを取得(サブゲーム情報(戦場ゲーム))						 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void GetDataDlgEtcSubGameEditWar(void)
{
	char szTmp[64];
	PSUBGAME_WARINFOSAVE pInfo;

	pInfo = (PSUBGAME_WARINFOSAVE)g_pSubGameInfo->pInfo;
	/* マップID */
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MAPID), szTmp, sizeof (szTmp));
	s_pWarInfo->dwMapID = (DWORD)atoi (szTmp);
	/* メンバー数 */
	s_pWarInfo->byMember = (BYTE)SendMessage (GetDlgItem (s_hWndDlg, IDC_MEMBER), CB_GETCURSEL, 0, 0);

	CopyMemory (pInfo, s_pWarInfo, sizeof (SUBGAME_WARINFOSAVE));
}

/* ========================================================================= */
/* 関数名：OnMapLbuttonDownDlgEtcSubGameEditWar								 */
/* 内容：マップ上で左クリックされた											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void OnMapLbuttonDownDlgEtcSubGameEditWar(int x, int y)
{
	if (s_hWndDlg == NULL) {
		return;
	}

	PostMessage (s_hWndDlg, WM_ADMINPOSLEFT, (WPARAM)x, (LPARAM)y);
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_INITEND:
		OnInitEnd (hWnd, wParam, lParam);
		break;

	case WM_ADMINPOSLEFT:
		OnAdminPosLeft (hWnd, (int)wParam, (int)lParam);
		break;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	int i;
	RECT rt;
	HWND hWndTmp;
	char szTmp[64];
	LPCSTR aszMember[] = {"３人", "４人", "z"};

	SAFE_DELETE (s_pWarInfo);
	s_pWarInfo = new SUBGAME_WARINFOSAVE;
	ZeroMemory (s_pWarInfo, sizeof (SUBGAME_WARINFOSAVE));

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	CopyMemory (s_pWarInfo, g_pSubGameInfo->pInfo, sizeof (SUBGAME_WARINFOSAVE));
	/* マップID */
	wsprintf (szTmp, "%d", s_pWarInfo->dwMapID);
	SetWindowText (GetDlgItem (hWnd, IDC_MAPID), szTmp);

	/* 「メンバー数」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_MEMBER);
	for (i = 0; ; i ++) {
		if (aszMember[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)aszMember[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, (WPARAM)s_pWarInfo->byMember, 0);

	Button_SetCheck (GetDlgItem (hWnd, IDC_XY), BST_CHECKED);

	PostMessage (hWnd, WM_INITEND, 0, 0);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
//	switch (nID) {
//	}
}

/* ========================================================================= */
/* 関数名：OnInitEnd														 */
/* 内容：メッセージハンドラ(WM_INITEND)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	RenewInfo ();
}

/* ========================================================================= */
/* 関数名：OnAdminPosLeft													 */
/* 内容：メッセージハンドラ(WM_ADMINPOSLEFT)								 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void OnAdminPosLeft(HWND hWnd, int x, int y)
{
	HWND hWndTmp;

	/* 参加受付 */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_XY);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byX = (BYTE)x;
		s_pWarInfo->byY = (BYTE)y;
		goto Exit;
	}
	/* 審判キャラ座標 */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_JUDGE);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byJudgeX = (BYTE)x;
		s_pWarInfo->byJudgeY = (BYTE)y;
		goto Exit;
	}
	/* エリア(左上) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_AREA1);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byX1 = (BYTE)x;
		s_pWarInfo->byY1 = (BYTE)y;
		goto Exit;
	}
	/* エリア(右下) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_AREA2);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byX2 = (BYTE)x;
		s_pWarInfo->byY2 = (BYTE)y;
		goto Exit;
	}
	/* HP(チーム１) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_HP1);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byHP1X = (BYTE)x;
		s_pWarInfo->byHPY = (BYTE)y;
		goto Exit;
	}
	/* 蘇生カウンタ(チーム１) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_RES1);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byRes1X = (BYTE)x;
		s_pWarInfo->byResY = (BYTE)y;
		goto Exit;
	}
	/* 開始位置(チーム１) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_START1);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byStart1X = (BYTE)x;
		s_pWarInfo->byStartY = (BYTE)y;
		goto Exit;
	}
	/* 城の位置(チーム１) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_GOAL1);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byCastle1X = (BYTE)x;
		s_pWarInfo->byCastleY = (BYTE)y;
		goto Exit;
	}
	/* HP(チーム２) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_HP2);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byHP2X = (BYTE)x;
		s_pWarInfo->byHPY = (BYTE)y;
		goto Exit;
	}
	/* 蘇生カウンタ(チーム２) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_RES2);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byRes2X = (BYTE)x;
		s_pWarInfo->byResY = (BYTE)y;
		goto Exit;
	}
	/* 開始位置(チーム２) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_START2);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byStart2X = (BYTE)x;
		s_pWarInfo->byStartY = (BYTE)y;
		goto Exit;
	}
	/* 城の位置(チーム２) */
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_GOAL2);
	if (Button_GetCheck (hWndTmp)) {
		s_pWarInfo->byCastle2X = (BYTE)x;
		s_pWarInfo->byCastleY = (BYTE)y;
		goto Exit;
	}

Exit:
	RenewInfo ();
}

/* ========================================================================= */
/* 関数名：RenewInfo														 */
/* 内容：情報更新															 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void RenewInfo(void)
{
	HWND hWndTmp;
	char szTmp[1024];

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_INFO);
	wsprintf (szTmp,
			"参加受付 X:%d Y:%d\n"
			"審判キャラ X:%d Y:%d\n"
			"ｴﾘｱ (%d,%d)-(%d,%d)\n"
			"HP　 X:%d Y:%d  X:%d Y:%d\n"
			"蘇生 X:%d Y:%d  X:%d Y:%d\n"
			"開始 X:%d Y:%d  X:%d Y:%d\n"
			"城   X:%d Y:%d  X:%d Y:%d\n",
			s_pWarInfo->byX, s_pWarInfo->byY,
			s_pWarInfo->byJudgeX, s_pWarInfo->byJudgeY,
			s_pWarInfo->byX1, s_pWarInfo->byY1,
			s_pWarInfo->byX2, s_pWarInfo->byY2,
			s_pWarInfo->byHP1X, s_pWarInfo->byHPY,
			s_pWarInfo->byHP2X, s_pWarInfo->byHPY,
			s_pWarInfo->byRes1X, s_pWarInfo->byResY,
			s_pWarInfo->byRes2X, s_pWarInfo->byResY,
			s_pWarInfo->byStart1X, s_pWarInfo->byStartY,
			s_pWarInfo->byStart2X, s_pWarInfo->byStartY,
			s_pWarInfo->byCastle1X, s_pWarInfo->byCastleY,
			s_pWarInfo->byCastle2X, s_pWarInfo->byCastleY
			);
	SetWindowText (hWndTmp, szTmp);
}

/* Copyright(C)URARA-works 2003-2005 */
