/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		すごろくマップパーツ情報編集ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgSugorokuPartsEdit.cpp								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/23													 */
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
static int s_nNo = -1;											/* 選択番号 */
static PSUGOROKUPARTSINFO s_pPartsInfo = NULL;					/* パーツ情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgSugorokuPartsEdit(HWND hWndParent,
		int nNo, PSUGOROKUPARTSINFO pInfo);										/* ダイアログ表示(すごろくマップパーツ情報編集) */
void CloseDlgSugorokuPartsEdit(void);											/* ダイアログ終了(すごろくマップパーツ情報編集) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y,
					UINT keyFlags);												/* メッセージハンドラ(WM_LBUTTONDOWN) */
static void OnPaint(HWND);														/* メッセージハンドラ(WM_PAINT) */

/* ========================================================================= */
/* 関数名：ShowDlgSugorokuPartsEdit											 */
/* 内容：ダイアログ表示(すごろくマップパーツ情報編集)						 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void ShowDlgSugorokuPartsEdit(HWND hWndParent, int nNo, PSUGOROKUPARTSINFO pInfo)
{
	if (s_hWndDlg) {
		return;
	}

	SAFE_DELETE (s_pPartsInfo);
	s_pPartsInfo = new SUGOROKUPARTSINFO;
	ZeroMemory (s_pPartsInfo, sizeof (SUGOROKUPARTSINFO));
	if (pInfo) {
		CopyMemory (s_pPartsInfo, pInfo, sizeof (SUGOROKUPARTSINFO));
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_SUGOROKU_PARTS_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgSugorokuPartsEdit										 */
/* 内容：ダイアログ終了(すごろくマップパーツ情報編集)						 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void CloseDlgSugorokuPartsEdit(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	SAFE_DELETE (s_pPartsInfo);
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
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);
	HANDLE_MSG (hWnd, WM_PAINT, OnPaint);

	case WM_ADMINSELECTMAPPARTS:	/* マップパーツ選択 */
		{
			s_pPartsInfo->wPartsID = (WORD)lParam;
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
	int i;
	HWND hWndTmp;

	/* 「種別」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPE);
	for (i = 0; i < SUBGAME_SUGOROKU_PARTS_MAX; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)aSugorokuPartsTbl[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, (WPARAM)(s_pPartsInfo->byType), 0);

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
	case IDOK:
		{
			PSUGOROKUPARTSINFO pInfo;

			pInfo = new SUGOROKUPARTSINFO;
			pInfo->byType = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_GETCURSEL, 0, 0);
			pInfo->wPartsID = s_pPartsInfo->wPartsID;
			PostMessage (s_hWndParent, WM_ADMINCLOSEDIALOG, (WPARAM)pInfo, (LPARAM)s_nNo);

			CloseDlgSugorokuPartsEdit ();
		}
		break;
	case IDCANCEL:
		CloseDlgSugorokuPartsEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnLButtonDown													 */
/* 内容：メッセージハンドラ(WM_LBUTTONDOWN)									 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
static void OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	HWND hWndTmp;
	POINT pt;
	RECT rc;

	pt.x = x;
	pt.y = y;
	ClientToScreen (hWnd, &pt);

	hWndTmp = GetDlgItem (hWnd, IDC_GRP);
	GetWindowRect (hWndTmp, &rc);
	/* [画像]でクリックされた？ */
	if (PtInRect (&rc, pt)) {
		ShowSelectMapPartsWindow (hWnd, IDC_GRP);
		return;
	}
}

/* ========================================================================= */
/* 関数名：OnPaint															 */
/* 内容：メッセージハンドラ(WM_PAINT)										 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void OnPaint(HWND hWnd)
{
	HDC hDC, hDCTmp;
	PAINTSTRUCT ps;
	int nNo, nID, nParts, xx, yy;
	HWND hWndTmp;
	PMAPPARTSINFO pParts;
	CImg32 DibTmp;
	PSUBGAME_DUNGEONINFOSAVE pInfo;

	hDC = BeginPaint(hWnd, &ps);

	DibTmp.Create (32, 32);
	/* パーツ画像表示 */
	HDC hDCParts = DibTmp.Lock ();

	pInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
	pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[s_pPartsInfo->wPartsID];
	nParts = pParts->wBackID;
	if (pParts->byAnimeCount) {
		PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
		if (pAnime == NULL) {
			goto Exit;
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

	hWndTmp = GetDlgItem (hWnd, IDC_GRP);
	hDCTmp = GetWindowDC (hWndTmp);

	/* 画面に転送 */
	BitBlt (hDCTmp, 0, 0, 32, 32, hDCParts, 0, 0, SRCCOPY);

Exit:
	ReleaseDC (hWndTmp, hDCTmp);
	DibTmp.Unlock ();
	EndPaint (hWnd, &ps);
}

/* Copyright(C)URARA-works 2003-2005 */
