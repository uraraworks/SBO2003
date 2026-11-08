/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			ウィンドウ作成											 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2005/10/31												 */
/* ========================================================================= */
#include "stdafx.h"
#include "../Common/Info/InfoJob.h"
#include "Glbldata.h"
#include "SboCliDrawWindow.h"
#include "SboCliRecvCmd.h"
#include "SboCliKeyProc.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* ウィンドウの作成 */
PWNDINFO GetNewWindow							(void);				/* 新規ウィンドウを取得 */
PWNDINFO MakeWindowMSGVIEW						(LPSTR);			/* 常に表示メッセージ */
PWNDINFO MakeWindowMSG3SEC						(LPSTR);			/* ３秒メッセージ */
PWNDINFO MakeWindowLOGIN						(void);				/* ログイン情報 */
PWNDINFO MakeWindowLOGINMENU					(void);				/* ログインメニュー */
PWNDINFO MakeWindowLOGINMENUCHARINFO			(void);				/* ログインメニューのキャラ情報 */
PWNDINFO MakeWindowLOGINMENUCHARMAKE			(void);				/* ログインメニューのキャラ作成 */
PWNDINFO MakeWindowCOMMAND						(void);				/* コマンド */
PWNDINFO MakeWindowCOMMANDETC					(void);				/* その他コマンド */
PWNDINFO MakeWindowCOMMANDITEM					(void);				/* 道具 */
PWNDINFO MakeWindowCOMMANDITEMBATTLE			(void);				/* 道具(戦闘マップ) */
PWNDINFO MakeWindowCOMMANDITEMPROC				(void);				/* 道具処理選択 */
PWNDINFO MakeWindowCOMMANDOPTION				(void);				/* オプション */
PWNDINFO MakeWindowCOMMANDOPTIONVOLSOUND		(void);				/* 効果音音量設定 */
PWNDINFO MakeWindowCOMMANDOPTIONVOLMUSIC		(void);				/* 曲音量設定 */
PWNDINFO MakeWindowSPEECH						(void);				/* 発言入力ウィンドウ */
PWNDINFO MakeWindowCONNECTLOST					(LPSTR);			/* 接続途切れた */
PWNDINFO MakeWindowMSG							(LPSTR, BOOL);		/* メッセージ */
PWNDINFO MakeWindowMALEFAMALE					(void);				/* 男女 */
PWNDINFO MakeWindowDELETECONFIRM				(void);				/* 削除確認 */
PWNDINFO MakeWindowNAMECOLOR					(void);				/* 名前の色設定 */
PWNDINFO MakeWindowSHOP							(int, int);			/* 店 */
PWNDINFO MakeWindowSHOP_BUY						(int);				/* 店:売り物一覧 */
PWNDINFO MakeWindowSHOP_SELL					(void);				/* 店:売る物一覧 */
PWNDINFO MakeWindowSHOP_CONF					(void);				/* 店:確認 */
PWNDINFO MakeWindowSHOPSELL						(int);				/* 店(売るだけ) */
PWNDINFO MakeWindowSPEECHEDIT					(void);				/* 自己紹介設定ウィンドウ */
PWNDINFO MakeWindowCOMMANDETC_CONF				(void);				/* 管理人呼び出し確認ウィンドウ */
PWNDINFO MakeWindowCOMMANDSTATUS				(void);				/* 強さウィンドウ */
PWNDINFO MakeWindowCOMMANDEQU					(void);				/* 装備ウィンドウ */
PWNDINFO MakeWindowCOMMANDEQU2					(void);				/* 装備ウィンドウ */
PWNDINFO MakeWindowCOMMANDEQUSub				(void);				/* 装備ウィンドウ(値表示用) */
PWNDINFO MakeWindowCOMMANDEQUSelect				(BYTE byType);		/* 装備ウィンドウ(選択) */
PWNDINFO MakeWindowCOMMANDPARTY					(void);				/* 仲間ウィンドウ */
PWNDINFO MakeWindowCOMMANDPARTYCHAR				(void);				/* 誘う相手選択 */
PWNDINFO MakeWindowLEAVE						(void);				/* 預かり所 */
PWNDINFO MakeWindowLEAVE_SELECT					(BYTE);				/* 預かり所：どちらか選択 */
PWNDINFO MakeWindowLEAVE_ITEM					(void);				/* 預かり所：アイテム選択 */
PWNDINFO MakeWindowLEAVE_ITEMGET				(void);				/* 預かり所：引き取るアイテム選択 */
PWNDINFO MakeWindowLEAVE_GOLD					(BYTE);				/* 預かり所：預ける金額設定 */
PWNDINFO MakeWindowCOMMANDITEMCHAR				(void);				/* 道具使う相手選択 */
PWNDINFO MakeWindowCOMMANDTRADE					(void);				/* 取り引きコマンド */
PWNDINFO MakeWindowTRADE						(DWORD);			/* 取り引き */
PWNDINFO MakeWindowTRADEITEM					(void);				/* 取り引き：アイテム選択 */
PWNDINFO MakeWindowTRADEGOLD					(void);				/* 取り引き：金額設定 */
PWNDINFO MakeWindowGOLD							(void);				/* 所持金表示 */
PWNDINFO MakeWindowSUBGAME_BOMBINFO				(void);				/* サブゲーム：爆弾ゲーム情報 */
PWNDINFO MakeWindowSUBGAME_BOMBSET				(void);				/* サブゲーム：爆弾ゲーム設定 */
PWNDINFO MakeWindowCHGPASSWORD					(void);				/* パスワードの変更 */
PWNDINFO MakeWindowCONF							(int, int, int);	/* 確認 */
PWNDINFO MakeWindowJOB							(BYTE);				/* 冒険者登録所 */
PWNDINFO MakeWindowJOB_JOBSELECT				(void);				/* 冒険者登録所：職業の選択 */
PWNDINFO MakeWindowCOMMANDMAGIC					(void);				/* 魔法 */
PWNDINFO MakeWindowCOMMANDMAGICBATTLE			(void);				/* 魔法(戦闘マップ) */
PWNDINFO MakeWindowSUBWAR_SELECTTEAM			(void);				/* 戦場ゲーム：チーム選択 */
PWNDINFO MakeWindowSUBWAR_WAIT					(void);				/* 戦場ゲーム：他のプレイヤー待ち */
PWNDINFO MakeWindowSUBWAR_SELECTRULE			(void);				/* 戦場ゲーム：ルール選択 */
PWNDINFO MakeWindowSUBWAR_SELECTJOB				(void);				/* 戦場ゲーム：職業選択 */
PWNDINFO MakeWindowSUBWAR_SELECTTURN			(void);				/* 戦場ゲーム：行動順序 */
PWNDINFO MakeWindowSUBWAR_TURN					(void);				/* 戦場ゲーム：行動選択 */
PWNDINFO MakeWindowSUBWAR_TURN_MOVE				(void);				/* 戦場ゲーム：行動(移動) */
PWNDINFO MakeWindowSUBWAR_TURN_ATACK			(void);				/* 戦場ゲーム：行動(攻撃) */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_FIGHTER	(void);				/* 戦場ゲーム：行動(防御：戦士) */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_MARTIAL	(void);				/* 戦場ゲーム：行動(防御：武道家) */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH	(void);				/* 戦場ゲーム：行動(防御：魔法使い) */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_PRIEST	(void);				/* 戦場ゲーム：行動(防御：僧侶) */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH2	(void);				/* 戦場ゲーム：行動(魔法防御：魔法使い) */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_WITCH		(void);				/* 戦場ゲーム：行動(魔法：魔法使い) */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_TARGET		(void);				/* 戦場ゲーム：行動(魔法対象者選択) */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_PRIEST		(void);				/* 戦場ゲーム：行動(魔法：蘇生) */
PWNDINFO MakeWindowITEMEQU						(void);				/* 装備アイテム */
PWNDINFO MakeWindowSTATUS_ATTR					(void);				/* ステータス表示(属性) */
PWNDINFO MakeWindowBATTLE_COMMAND				(void);				/* 戦闘：コマンド */
PWNDINFO MakeWindowBATTLE_TARGET				(int nArea, BOOL bMyChar, int nType);	/* 戦闘：ターゲット選択 */

PWNDINFO MakeWindowSUBSUGOROKU_JOINMODE			(void);				/* すごろく：参加モード選択 */
PWNDINFO MakeWindowSUBSUGOROKU_COMMAND			(void);				/* すごろく：行動状態 */
PWNDINFO MakeWindowSUBSUGOROKU_IDLE				(void);				/* すごろく：平常状態 */

PWNDINFO MakeWindowMAKER						(int);				/* 作り手：作れる物の一覧 */
PWNDINFO MakeWindowMAKER_CONF					(int);				/* 作り手：確認 */
int MakerGetRecipeList							(int, int *, int);	/* 作り手の一覧に出すレシピの番号を並び順で取得 */
int MakerGetCursorRecipe						(PWNDINFO);			/* 一覧のカーソル位置のレシピの番号 */
int MakerCountHave								(WORD);				/* 材料として使える数（持ち物+預かり所） */
BOOL MakerCanMake								(int);				/* 材料と手数料が足りているか */
DWORD MakerStateHash							(int);				/* 画面に出ている内容が変わったか調べる値 */
void MakerSetMsg								(PWNDINFO, LPCSTR);	/* 作り手の画面のメッセージを差し替える */
void MakerSetTalkMsg							(PWNDINFO);			/* 作り手の話しかけた時の台詞を出す */
void MakerScrollFit								(PWNDINFO, int, int);	/* 一覧のカーソルとスクロールを整える */
PWNDINFO MakeWindowITEMINFO						(int x, int y);		/* アイテム情報 */
PWNDINFO MakeWindowITEMINFOList					(PWNDINFO pWndList);	/* アイテム情報(一覧の横) */
void UpdateWindowITEMINFOList					(PWNDINFO pWndList);	/* アイテム情報(一覧の横)の更新 */
PWNDINFO MakeWindowDEAD_COMAND					(void);				/* 死亡中：コマンド */

extern int GetWindowID(DWORD);										/* ウィンドウ番号を取得 */
#ifdef __EMSCRIPTEN__
extern void SboWebSpeechInputMade(HWND hWnd);							/* 発言入力欄に残り文字数の表示を付ける（ブラウザ版） */
#endif


/* ========================================================================= */
/* 関数名：	GetNewWindow													 */
/* 内容：	新規ウィンドウを取得											 */
/* 日付：	2005/10/31														 */
/* ========================================================================= */

PWNDINFO GetNewWindow(void)
{
	PWNDINFO pWnd;

	pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	return pWnd;
}


/* ========================================================================= */
/* 関数名：	MakeWindowMSGVIEW												 */
/* 内容：	ウィンドウの作成(常に表示メッセージ)							 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

PWNDINFO MakeWindowMSGVIEW(LPSTR pszMsg)
{
	PWNDINFO pWnd;
	int nCx;
	LPSTR pszTmp = (LPSTR)new BYTE [strlen (pszMsg) + 1];

	nCx = SboTextWidth (pszMsg) * 8 + 48;
	nCx = (nCx % 16) ? nCx + nCx % 16 : nCx;
	ZeroMemory (pszTmp, strlen (pszMsg) + 1);
	strcat (pszTmp, pszMsg);

	pWnd = GetNewWindow ();
	pWnd->nX = SCRSIZEX / 2 - nCx / 2;
	pWnd->nY = SCRSIZEY / 2 - 48 / 2;
	pWnd->nCx = nCx;
	pWnd->nCy = 48;
	pWnd->nType = WNDTYPE_MSGVIEW;
	pWnd->pszMsg = pszTmp;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}


/* ========================================================================= */
/* 関数名：	MakeWindowMSG3SEC												 */
/* 内容：	ウィンドウの作成(３秒メッセージ)								 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

PWNDINFO MakeWindowMSG3SEC(LPSTR pszMsg)
{
	PWNDINFO pWnd;
	int nCx;
	LPSTR pszTmp = (LPSTR)new BYTE [strlen (pszMsg) + 1];

	nCx = SboTextWidth (pszMsg) * 8 + 48;
	nCx = (nCx % 16) ? nCx + nCx % 16 : nCx;
	ZeroMemory (pszTmp, strlen (pszMsg) + 1);
	strcat (pszTmp, pszMsg);

	pWnd = GetNewWindow ();
	pWnd->nX = SCRSIZEX / 2 - nCx / 2;
	pWnd->nY = SCRSIZEY / 2 - 48 / 2;
	pWnd->nCx = nCx;
	pWnd->nCy = 48;
	pWnd->nType = WNDTYPE_MSG3SEC;
	pWnd->pszMsg = pszTmp;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}


/* ========================================================================= */
/* 関数名：	MakeWindowLOGIN													 */
/* 内容：	ウィンドウの作成(ログイン情報)									 */
/* 日付：	2003/05/01														 */
/* ========================================================================= */

PWNDINFO MakeWindowLOGIN(void)
{
	PWNDINFO pWnd;

	pWnd = GetNewWindow ();
	pWnd->nX	= 112;
	pWnd->nY	= 50;
	pWnd->nCx	= 16 * 16;
	pWnd->nCy	= 16 * 6;
	pWnd->nType	= WNDTYPE_LOGIN;
	g_dwMsgIDLogin = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	g_hWndAcc = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_LOWERCASE | ES_AUTOHSCROLL,
			pWnd->nX + 16 * 8, pWnd->nY + 16, 6 * 14, 14, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndAcc, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	ImmAssociateContext (g_hWndAcc, NULL);
	g_OrgWndProcAcc = (WNDPROC)GetWindowLong (g_hWndAcc, GWL_WNDPROC);
	SetWindowLong (g_hWndAcc, GWL_WNDPROC, (LONG)AccWndProc);

	g_hWndPas = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL,
			pWnd->nX + 16 * 8, pWnd->nY + 16 * 2, 6 * 14, 14, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndPas, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	ImmAssociateContext (g_hWndPas, NULL);
	g_OrgWndProcPas = (WNDPROC)GetWindowLong (g_hWndPas, GWL_WNDPROC);
	SetWindowLong (g_hWndPas, GWL_WNDPROC, (LONG)PasWndProc);

	g_hWndCon = CreateWindow ("BUTTON", "接続", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			pWnd->nX + 16 * 12, pWnd->nY + 16 * 4, 48, 18, g_hWndMain, (HMENU)IDC_CONNECT, g_hInstance, NULL);
	SendMessage (g_hWndCon, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	g_OrgWndProcCon = (WNDPROC)GetWindowLong (g_hWndCon, GWL_WNDPROC);
	SetWindowLong (g_hWndCon, GWL_WNDPROC, (LONG)ConWndProc);

	if (strlen (g_szAcc)) {
		SetWindowText (g_hWndAcc, g_szAcc);
		SetFocus (g_hWndPas);

	} else {
		SetFocus (g_hWndAcc);
	}
	return pWnd;
}


/* ========================================================================= */
/* 関数名：	MakeWindowLOGINMENU												 */
/* 内容：	ウィンドウの作成(ログインメニュー)								 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

PWNDINFO MakeWindowLOGINMENU(void)
{
	PWNDINFO pWnd;

	pWnd = GetNewWindow ();
	pWnd->nX	= 32;
	pWnd->nY	= 50;
	pWnd->nCx	= 16 * 11;
	pWnd->nCy	= 16 * 5;
	pWnd->nType	= WNDTYPE_LOGINMENU;

	g_dwMsgIDMENU = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}


/* ========================================================================= */
/* 関数名：MakeWindowLOGINMENUCHARINFO										 */
/* 内容：ウィンドウの作成(ログインメニューのキャラ情報)						 */
/* 日付：2003/07/27															 */
/* ========================================================================= */

PWNDINFO MakeWindowLOGINMENUCHARINFO(void)
{
	PWNDINFO pWnd;

	pWnd = GetNewWindow ();
	pWnd->nX	= 224;
	pWnd->nY	= 50;
	pWnd->nCx	= 16 * 15;
	pWnd->nCy	= 16 * 6;
	pWnd->nType	= WNDTYPE_LOGINMENUCHARINFO;

	g_dwMsgIDMENUCHARINFO = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}


/* ========================================================================= */
/* 関数名：MakeWindowLOGINMENUCHARMAKE										 */
/* 内容：ウィンドウの作成(ログインメニューのキャラ作成)						 */
/* 日付：2003/07/28															 */
/* ========================================================================= */
PWNDINFO MakeWindowLOGINMENUCHARMAKE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 112;
	pWnd->nY = 76;
	pWnd->nCx = 16 * 15;
	pWnd->nCy = 16 * 5;
	pWnd->nType = WNDTYPE_LOGINMENUCHARMAKE;
	g_dwMsgIDMENUCHARMAKE = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	g_hWndName = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			pWnd->nX + 16, pWnd->nY + 32, 6 * 30, 14, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndName, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	g_OrgWndProcName = (WNDPROC)GetWindowLong (g_hWndName, GWL_WNDPROC);
	SetWindowLong (g_hWndName, GWL_WNDPROC, (LONG)NameWndProc);

	g_hWndMake = CreateWindow ("BUTTON", "作成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			pWnd->nX + 16 * 10, pWnd->nY + 16 * 3, 48, 18, g_hWndMain, (HMENU)IDC_MAKE, g_hInstance, NULL);
	SendMessage (g_hWndMake, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	g_OrgWndProcMake = (WNDPROC)GetWindowLong (g_hWndMake, GWL_WNDPROC);
	SetWindowLong (g_hWndMake, GWL_WNDPROC, (LONG)MakeWndProc);

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMAND												 */
/* 内容：ウィンドウの作成(コマンド)											 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMAND(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 32;
	pWnd->nY = 50;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * 7;
	pWnd->nType = WNDTYPE_COMMAND;
	g_dwMsgIDCommand = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	/* IMEをオフにする */
	HIMC hImc = ImmGetContext (g_hWndMain);
	ImmSetOpenStatus (hImc, FALSE);
	ImmReleaseContext (g_hWndMain, hImc);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDETC												 */
/* 内容：ウィンドウの作成(その他コマンド)									 */
/* 日付：2003/08/03															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDETC(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 8;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = 16 * 15;
	pWnd->nCy = 16 * (pWnd->nData2 + 2);
	pWnd->nType = WNDTYPE_COMMANDETC;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSPEECH													 */
/* 内容：ウィンドウの作成(発言入力ウィンドウ)								 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
PWNDINFO MakeWindowSPEECH(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 8 * 16;
	pWnd->nY = 8 * 32;
	pWnd->nCx = 14 * 16;
	pWnd->nCy = 16 * 3;
	pWnd->nType = WNDTYPE_SPEECH;
	g_dwMsgIDLogin = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_hWndSpeech = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
			pWnd->nX + 16, pWnd->nY + 16, 12 * 16, 16, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndSpeech, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	g_OrgWndProcSpeech = (WNDPROC)GetWindowLong (g_hWndSpeech, GWL_WNDPROC);
	SetWindowLong (g_hWndSpeech, GWL_WNDPROC, (LONG)SpeechWndProc);
#ifdef __EMSCRIPTEN__
	SboWebSpeechInputMade (g_hWndSpeech);
#endif

	SetFocus (g_hWndSpeech);

	/* IMEをオンにする */
	HIMC hImc = ImmGetContext (g_hWndSpeech);
	ImmSetOpenStatus (hImc, TRUE);
	ImmReleaseContext (g_hWndSpeech, hImc);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCONNECTLOST											 */
/* 内容：ウィンドウの作成(接続途切れた)										 */
/* 日付：2003/05/25															 */
/* ========================================================================= */
PWNDINFO MakeWindowCONNECTLOST(LPSTR pszMsg)
{
	int nCx;
	LPSTR pszTmp = (LPSTR)new BYTE [strlen (pszMsg) + 1];

	nCx = SboTextWidth (pszMsg) * 8 + 48;
	nCx = (nCx % 16) ? nCx + nCx % 16 : nCx;
	ZeroMemory (pszTmp, strlen (pszMsg) + 1);
	strcat (pszTmp, pszMsg);

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = SCRSIZEX / 2 - nCx / 2;
	pWnd->nY = SCRSIZEY / 2 - 48 / 2;
	pWnd->nCx = nCx;
	pWnd->nCy = 48;
	pWnd->nType = WNDTYPE_CONNECTLOST;
	pWnd->pszMsg = pszTmp;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMSG													 */
/* 内容：ウィンドウの作成(メッセージ)										 */
/* 日付：2003/06/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowMSG(LPSTR pszMsg, BOOL bSound)
{
	for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
		/* すでに表示されている？ */
		if (g_WndInfo[i]->nType == WNDTYPE_MSG) {
			EraseWindow (g_WndInfo[i]);
			break;
		}
	}

	LPSTR pszTmp = (LPSTR)new char [strlen (pszMsg) + 1];
	ZeroMemory (pszTmp, strlen (pszMsg) + 1);
	strcpy (pszTmp, pszMsg);

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 16 * 4;
	pWnd->nY = 32 * 11;
	pWnd->nCx = 16 * 22;
	pWnd->nCy = 16 * 7;
	pWnd->nType = WNDTYPE_MSG;
	pWnd->pszMsg = pszTmp;
	pWnd->byData1 = (BYTE)bSound;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMALEFAMALE												 */
/* 内容：ウィンドウの作成(男女)												 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
PWNDINFO MakeWindowMALEFAMALE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 128;
	pWnd->nY = 128;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * 4;
	pWnd->nType = WNDTYPE_MALEFAMALE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowDELETECONFIRM											 */
/* 内容：ウィンドウの作成(削除確認)											 */
/* 日付：2004/01/16															 */
/* ========================================================================= */
PWNDINFO MakeWindowDELETECONFIRM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 128;
	pWnd->nY = 128;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * 4;
	pWnd->nType = WNDTYPE_DELETECONFIRM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDITEM											 */
/* 内容：ウィンドウの作成(道具)												 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDITEM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDITEM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDITEMBATTLE										 */
/* 内容：ウィンドウの作成(道具(戦闘マップ))									 */
/* 日付：2005/02/01															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDITEMBATTLE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDITEMBATTLE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDITEMPROC										 */
/* 内容：ウィンドウの作成(道具処理選択)										 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDITEMPROC(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 32 + 16 * 8;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDITEMPROC;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDOPTION											 */
/* 内容：ウィンドウの作成(オプション)										 */
/* 日付：2003/09/07															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDOPTION(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 10;
	pWnd->nX = 32 + 16 * 2;
	pWnd->nY = 50 + 16 * 11;
	pWnd->nCx = 16 * 16;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDOPTION;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDOPTIONVOLSOUND									 */
/* 内容：ウィンドウの作成(効果音音量設定)									 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDOPTIONVOLSOUND(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData1 = g_byVolSound;
	pWnd->nData2 = 5;
	pWnd->nX = 16 * 3;
	pWnd->nY = 16 * 22 + 2;
	pWnd->nCx = 16 * 16;
	pWnd->nCy = 16 * (2 + 2);
	pWnd->nType = WNDTYPE_COMMANDOPTIONVOLSOUND;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDOPTIONVOLMUSIC									 */
/* 内容：ウィンドウの作成(曲音量設定)										 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDOPTIONVOLMUSIC(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData1 = g_byVolMusic;
	pWnd->nData2 = 5;
	pWnd->nX = 16 * 3;
	pWnd->nY = 16 * 21 + 2;
	pWnd->nCx = 16 * 16;
	pWnd->nCy = 16 * (2 + 2);
	pWnd->nType = WNDTYPE_COMMANDOPTIONVOLMUSIC;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowNAMECOLOR												 */
/* 内容：ウィンドウの作成(名前の色設定)										 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
PWNDINFO MakeWindowNAMECOLOR(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 7;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 9;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_NAMECOLOR;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSHOP													 */
/* 内容：ウィンドウの作成(店)												 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
PWNDINFO MakeWindowSHOP(int nNo, int nMsgNo)
{
	if (nNo >= g_pMapInfo->byCountShop) {
		return NULL;
	}

	int nNoTmp = GetWindowID (g_dwMsgIDGold);
	if (nNoTmp >= 0) {
		PWNDINFO pWndTmp = (PWNDINFO)g_WndInfo[nNoTmp];
		pWndTmp->dwTimeDrawStart = 0;
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->nX = 16 * 20;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SHOP;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopID = nNo;
	g_nProcShopState = SHOPMSG_OPEN;
	g_bProcShopConfirm = FALSE;

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	if (pShop->dwMsgCRC[nMsgNo] != g_crc.GetCRC ((PBYTE)pShop->apMsg[nMsgNo], strlen (pShop->apMsg[nMsgNo]) + 1)) {
		/* あやしいのでそっと終了。 */
		PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
	}

	PWNDINFO pWndNew = MakeWindowMSG (pShop->apMsg[nMsgNo], TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSHOP_BUY												 */
/* 内容：ウィンドウの作成(店:売り物一覧)									 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
PWNDINFO MakeWindowSHOP_BUY(int nMsgNo)
{
	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	g_nProcShopState = SHOPMSG_BUY;

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = pShop->byCountItem;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 22;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SHOP;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	if (pShop->dwMsgCRC[nMsgNo] != g_crc.GetCRC ((PBYTE)pShop->apMsg[nMsgNo], strlen (pShop->apMsg[nMsgNo]) + 1)) {
		/* あやしいのでそっと終了。 */
		PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
	}
	PWNDINFO pWndNew = MakeWindowMSG (pShop->apMsg[nMsgNo], TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	g_nProcShopSelect = 0;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSHOP_SELL												 */
/* 内容：ウィンドウの作成(店:売る物一覧)									 */
/* 日付：2003/11/22															 */
/* ========================================================================= */
PWNDINFO MakeWindowSHOP_SELL(void)
{
	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	g_nProcShopState = SHOPMSG_SELL;

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 5;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SHOP;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	if (pShop->dwMsgCRC[SHOPMSG_SELL] != g_crc.GetCRC ((PBYTE)pShop->apMsg[SHOPMSG_SELL], strlen (pShop->apMsg[SHOPMSG_SELL]) + 1)) {
		/* あやしいのでそっと終了。 */
		PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
	}
	PWNDINFO pWndNew = MakeWindowMSG (pShop->apMsg[SHOPMSG_SELL], TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	g_nProcShopSelect = 0;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSHOP_CONF												 */
/* 内容：ウィンドウの作成(店:確認)											 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
PWNDINFO MakeWindowSHOP_CONF(void)
{
	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 20;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SHOP;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	g_bProcShopConfirm = TRUE;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSHOPSELL												 */
/* 内容：ウィンドウの作成(店(売るだけ))										 */
/* 日付：2003/12/06															 */
/* ========================================================================= */
PWNDINFO MakeWindowSHOPSELL(int nNo)
{
	if (nNo >= g_pMapInfo->byCountShop) {
		return NULL;
	}

	g_nProcShopID = nNo;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWnd = MakeWindowSHOP_BUY (SHOPMSG_BUY);
	pWnd->nType = WNDTYPE_SHOPSELL;

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSPEECHEDIT												 */
/* 内容：ウィンドウの作成(自己紹介設定ウィンドウ)							 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
PWNDINFO MakeWindowSPEECHEDIT(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 16 * 6;
	pWnd->nY = 50 + 16 * 8;
	pWnd->nCx = 16 * 20;
	pWnd->nCy = 16 * 5;
	pWnd->nType = WNDTYPE_SPEECHEDIT;
	g_dwMsgIDLogin = pWnd->dwID;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_hWndSpeech = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			pWnd->nX + 16, pWnd->nY + 16 * 3, pWnd->nCx - (2 * 16), 16, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndSpeech, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	g_OrgWndProcSpeech = (WNDPROC)GetWindowLong (g_hWndSpeech, GWL_WNDPROC);
	SetWindowLong (g_hWndSpeech, GWL_WNDPROC, (LONG)SpeechEditWndProc);

	SetFocus (g_hWndSpeech);

	/* IMEをオンにする */
	HIMC hImc = ImmGetContext (g_hWndSpeech);
	ImmSetOpenStatus (hImc, TRUE);
	ImmReleaseContext (g_hWndSpeech, hImc);

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->pszNpcSpeech) {
		SetWindowText (g_hWndSpeech, pChar->pszNpcSpeech);
	}
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDETC_CONF										 */
/* 内容：ウィンドウの作成(管理人呼び出し確認ウィンドウ)						 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDETC_CONF(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData1 = 1;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 20;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDETC_CONF;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDSTATUS											 */
/* 内容：ウィンドウの作成(強さ)												 */
/* 日付：2003/09/28															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDSTATUS(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX	= 32;
	pWnd->nY	= 50 + 16 * 2;
	pWnd->nCx	= 16 * 10;
	pWnd->nCy	= 16 * 20;
	pWnd->nType	= WNDTYPE_COMMANDSTATUS;
	pWnd->pDib	= new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew1, pWndNew2, pWndNew3;
	pWndNew1			= MakeWindowITEMEQU ();
	pWndNew1->pParent	= pWnd;
	pWnd->pChild		= pWndNew1;
	g_WndInfo.Add (pWndNew1);

	pWndNew2			= MakeWindowSTATUS_ATTR ();
	pWndNew2->pParent	= pWndNew1;
	pWndNew1->pChild	= pWndNew2;
	g_WndInfo.Add (pWndNew2);

	pWndNew3			= MakeWindowGOLD ();
	pWndNew3->pParent	= pWndNew2;
	pWndNew2->pChild	= pWndNew3;
	g_WndInfo.Add (pWndNew3);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDEQU												 */
/* 内容：ウィンドウの作成(装備)												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDEQU(void)
{
	return MakeWindowCOMMANDEQU2();

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX = 32;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 12;
	pWnd->nCy = 16 * 19;
	pWnd->nType = WNDTYPE_COMMANDEQU;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndSub = new WNDINFO;
	ZeroMemory (pWndSub, sizeof (WNDINFO));
	pWndSub->dwID = g_dwNewID;
	g_dwNewID ++;

	pWndSub->nData2 = 1 + GetItemCountTypeEqu (g_nMyCharID, ITEMTYPE_WEAPON);

	pWnd->pChild = pWndSub;
	pWndSub->pParent = pWnd;
	pWndSub->nX = 32 + 16 * 12;
	pWndSub->nY = 16 * 4;
	pWndSub->nCx = 16 * 12;
	pWndSub->nCy = 16 * (2 + pWndSub->nData2);
	pWndSub->nType = WNDTYPE_COMMANDEQUSUB;
	pWndSub->pDib = new CImg32;
	pWndSub->pDib->Create (pWndSub->nCx, pWndSub->nCy);
	pWndSub->pDib->SetColorKey (RGB (40, 40, 40));
	g_WndInfo.Add (pWndSub);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDEQU												 */
/* 内容：ウィンドウの作成(装備)												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDEQU2(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX = 32;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 17;
	pWnd->nCy = 16 * 11;
	pWnd->nType = WNDTYPE_COMMANDEQU;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	pWnd->nData2 = 9;

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDEQUSub											 */
/* 内容：ウィンドウの作成(装備(値表示用))									 */
/* 日付：2005/04/12															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDEQUSub(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PWNDINFO pWnd = new WNDINFO;

	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData1 = (int)pChar->wAT;
	pWnd->nData2 = (int)pChar->wDF;
	pWnd->nX = 32 + 16;
	pWnd->nY = 16 * 14;
	pWnd->nCx = 16 * 13;
	pWnd->nCy = 16 * 3;
	pWnd->nType = WNDTYPE_COMMANDEQUSUB;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDEQUSelect										 */
/* 内容：ウィンドウの作成(装備(選択))										 */
/* 日付：2005/04/12															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDEQUSelect(BYTE byType)
{
	PWNDINFO pWnd = new WNDINFO;
	BYTE byTmp;
	switch (byType) {
	case ITEMTYPE_WEAPON:	byTmp = 0;	break;
	case ITEMTYPE_ARMOR:	byTmp = 1;	break;
	case ITEMTYPE_SHILD:	byTmp = 2;	break;
	case ITEMTYPE_HELMET:	byTmp = 3;	break;
	case ITEMTYPE_EQU_HAND:	byTmp = 4;	break;
	case ITEMTYPE_EQU_LEG:	byTmp = 5;	break;
	case ITEMTYPE_EQU_ACC1:	byTmp = 6;	break;
	case ITEMTYPE_EQU_ACC2:	byTmp = 7;	break;
	case ITEMTYPE_EQU_ACC3:	byTmp = 8;	break;
	}

	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->byData1 = byType;
	pWnd->nData2 = 1 + GetItemCountTypeEqu (g_nMyCharID, byType);
	pWnd->nX = 32 + 16 * 13;
	pWnd->nY = 16 * 6;
	pWnd->nCx = 16 * 13;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDEQUSELECT;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDPARTY											 */
/* 内容：ウィンドウの作成(仲間ウィンドウ)									 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDPARTY(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 4;
	pWnd->nX = 32;
	pWnd->nY = 16 * 7 + 2;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * 6;
	pWnd->nType = WNDTYPE_COMMANDPARTY;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDPARTYCHAR										 */
/* 内容：ウィンドウの作成(誘う相手選択)										 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDPARTYCHAR(void)
{
	int i;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
		  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aUseCharInfo[i]);
	}
	g_aUseCharInfo.RemoveAll ();

	int nLen = 0;

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* 目の前にいる？ */
		if ((dwX == pCharTmp->dwX) && (dwY == pCharTmp->dwY)) {
			if (pCharTmp->byNpcType == 0) {
				PUSECHARINFO pInfo = new USECHARINFO;
				ZeroMemory (pInfo, sizeof (USECHARINFO));
				pInfo->dwTblID = pCharTmp->dwTblID;
				strcat (pInfo->szName, pCharTmp->szName);
				nLen = max (SboTextWidth (pInfo->szName), nLen);
				g_aUseCharInfo.Add (pInfo);
				if (g_aUseCharInfo.GetSize () >= 10) {
					break;
				}
			}
		}
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_COMMANDPARTYCHAR;
	pWnd->pDib = new CImg32;
	pWnd->nData2 = g_aUseCharInfo.GetSize ();
	pWnd->nX = 32;
	pWnd->nY = 16 * 9 + 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE													 */
/* 内容：ウィンドウの作成(預かり所)											 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE(BYTE bySelect)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->nX = 16 * 21;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 7;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = LEAVESTATE_OPEN;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew;
	if (bySelect == 0) {
		pWndNew = MakeWindowMSG ("＊｢ここは預かり所です。どのような御用で　 しょう？@2", TRUE);

	} else if (bySelect == 1) {
		pWndNew = MakeWindowMSG ("＊｢他に御用は？@2", TRUE);

	} else if (bySelect == 2) {
		pWndNew = MakeWindowMSG ("＊｢装備中のものはお預かりできません。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 3) {
		pWndNew = MakeWindowMSG ("＊｢これ以上お預かりできません。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 4) {
		pWndNew = MakeWindowMSG ("＊｢何もお預かりしていません。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 5) {
		pWndNew = MakeWindowMSG ("＊｢それ以上お持ちになれないようです。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 6) {
		pWndNew = MakeWindowMSG ("＊｢お金はお預かりしておりません。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 7) {
		pWndNew = MakeWindowMSG ("何も持っていません。@1@1＊｢他に御用は？@2", TRUE);

	} else if (bySelect == 8) {
		pWndNew = MakeWindowMSG ("所持金がありません。@1@1＊｢他に御用は？@2", TRUE);

	} else if (bySelect == 9) {
		pWndNew = MakeWindowMSG ("これ以上持てません。@1@1＊｢他に御用は？@2", TRUE);

	} else {
		pWndNew = MakeWindowMSG ("＊｢他に御用は？@2", TRUE);
	}
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE_SELECT											 */
/* 内容：ウィンドウの作成(預かり所：どちらか選択)							 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_SELECT(BYTE bySelect)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->nX = 16 * 22;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = LEAVESTATE_SELECT;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew;
	if (bySelect == 0) {
		pWndNew = MakeWindowMSG ("＊｢どちらを預かりましょう？@2", TRUE);
	} else {
		pWndNew = MakeWindowMSG ("＊｢どちらを引き取りますか？@2", TRUE);
	}
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE_ITEM												 */
/* 内容：ウィンドウの作成(預かり所：アイテム選択)							 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_ITEM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 5;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = LEAVESTATE_ITEM;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew = MakeWindowMSG ("＊｢どれを預かりましょう？@2", TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE_ITEMGET											 */
/* 内容：ウィンドウの作成(預かり所：引き取るアイテム選択)					 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_ITEMGET(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	pWnd->nData2 = g_pLeaveInfo->byCount;
	pWnd->nData2 = min (pWnd->nData2, 10);
	pWnd->nX = 32 + 16 * 5;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (33 * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);

	g_nProcShopState = LEAVESTATE_ITEMSELECTGET;
	g_bProcShopConfirm = FALSE;

	char szTmp[256];
	wsprintf (szTmp, "＊｢現在%d品お預かりしています。@1　 どれをお引取りなさいますか？@2", g_pLeaveInfo->byCount);
	PWNDINFO pWndNew = MakeWindowMSG (szTmp, TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE_GOLD												 */
/* 内容：ウィンドウの作成(預かり所：金額設定)								 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_GOLD(BYTE bySelect)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;

	pWnd->nX = 32 + 16 * 16;
	pWnd->nY = 50 + 16 * 10;
	pWnd->nCx = (16 * 4) + (8 * 4);
	pWnd->nCy = 16 * 3;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	pWnd->byData1 = 5;

	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew;
	char szTmp[256];
	if (bySelect == 0) {
		g_nProcShopState = LEAVESTATE_GOLD;
		pWndNew = MakeWindowMSG ("＊｢いくら預かりましょう？@2", TRUE);

	} else if (bySelect == 1) {
		g_nProcShopState = LEAVESTATE_GOLDGET;
		wsprintf (szTmp, "＊｢現在%dゴールドお預かりしています。@1　 いくらお引取りになりますか？@2", g_pLeaveInfo->dwGold);
		pWndNew = MakeWindowMSG (szTmp, TRUE);
	}

	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDITEMCHAR										 */
/* 内容：ウィンドウの作成(道具使う相手選択)									 */
/* 日付：2003/11/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDITEMCHAR(void)
{
	int i;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
		  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aUseCharInfo[i]);
	}
	g_aUseCharInfo.RemoveAll ();

	int nLen = 0;
	PUSECHARINFO pInfo = new USECHARINFO;
	ZeroMemory (pInfo, sizeof (USECHARINFO));
	pInfo->dwTblID = pChar->dwTblID;
	strcat (pInfo->szName, pChar->szName);
	nLen = max (SboTextWidth (pInfo->szName), nLen);
	g_aUseCharInfo.Add (pInfo);

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* 目の前にいる？ */
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (dwY == pCharTmp->dwY)) {
			if ((pCharTmp->byNpcType == 0) ||
				(pCharTmp->byNpcType == NPCTYPE_BATTLE1) ||
				(pCharTmp->byNpcType == NPCTYPE_BATTLE2) ||
				(pCharTmp->byNpcType == NPCTYPE_MERCENARY)
				) {
				pInfo = new USECHARINFO;
				ZeroMemory (pInfo, sizeof (USECHARINFO));
				pInfo->dwTblID = pCharTmp->dwTblID;
				strcat (pInfo->szName, pCharTmp->szName);
				nLen = max (SboTextWidth (pInfo->szName), nLen);
				g_aUseCharInfo.Add (pInfo);
				if (g_aUseCharInfo.GetSize () >= 10) {
					break;
				}
			}
		}
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_COMMANDITEMCHAR;
	pWnd->pDib = new CImg32;
	pWnd->nData2 = g_aUseCharInfo.GetSize ();
	pWnd->nX = 32 + 16 * 9;
	pWnd->nY = 50 + 16 * 7;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDITEMCHARBATTLE									 */
/* 内容：ウィンドウの作成(道具使う相手選択(戦闘マップ))						 */
/* 日付：2005/02/01															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDITEMCHARBATTLE(void)
{
	int i;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
		  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aUseCharInfo[i]);
	}
	g_aUseCharInfo.RemoveAll ();

	int nLen = 0;
	PUSECHARINFO pInfo = new USECHARINFO;
	ZeroMemory (pInfo, sizeof (USECHARINFO));
	pInfo->dwTblID = pChar->dwTblID;
	strcat (pInfo->szName, pChar->szName);
	nLen = max (SboTextWidth (pInfo->szName), nLen);
	g_aUseCharInfo.Add (pInfo);

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* 目の前にいる？ */
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (dwY == pCharTmp->dwY)) {
			if ((pCharTmp->byNpcType == 0) ||
				(pCharTmp->byNpcType == NPCTYPE_BATTLE1) ||
				(pCharTmp->byNpcType == NPCTYPE_BATTLE2) ||
				(pCharTmp->byNpcType == NPCTYPE_MERCENARY)
				) {
				pInfo = new USECHARINFO;
				ZeroMemory (pInfo, sizeof (USECHARINFO));
				pInfo->dwTblID = pCharTmp->dwTblID;
				strcat (pInfo->szName, pCharTmp->szName);
				nLen = max (SboTextWidth (pInfo->szName), nLen);
				g_aUseCharInfo.Add (pInfo);
				if (g_aUseCharInfo.GetSize () >= 10) {
					break;
				}
			}
		}
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_COMMANDITEMCHARBATTLE;
	pWnd->pDib = new CImg32;
	pWnd->nData2 = g_aUseCharInfo.GetSize ();
	pWnd->nX = 32 + 16 * 9;
	pWnd->nY = 50 + 16 * 7;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDTRADE											 */
/* 内容：ウィンドウの作成(取り引きコマンド)									 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDTRADE(void)
{
	int i;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
		  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	PMAPPARTSINFO pParts = &g_pMapPartsInfo[g_pMapInfo->pwMap[dwX + dwY * g_pMapInfo->nCX]];
	if (pParts->byHitType == PARTSHIT_COUNTER) {
		dwX += anPosAddTblX[pChar->byDirection];
		dwY += anPosAddTblY[pChar->byDirection];
	}

	DeleteUseCharInfo (&g_aUseCharInfo);

	int nLen = 0;
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* 目の前にいる？ */
		if ((dwX == pCharTmp->dwX) && (dwY == pCharTmp->dwY)) {
			if (pCharTmp->byNpcType == 0) {
				PUSECHARINFO pInfo = new USECHARINFO;
				ZeroMemory (pInfo, sizeof (USECHARINFO));
				pInfo->dwTblID = pCharTmp->dwTblID;
				strcat (pInfo->szName, pCharTmp->szName);
				nLen = max (SboTextWidth (pInfo->szName), nLen);
				g_aUseCharInfo.Add (pInfo);
				if (g_aUseCharInfo.GetSize () >= 10) {
					break;
				}
			}
		}
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_COMMANDTRADE;
	pWnd->pDib = new CImg32;
	pWnd->nData2 = g_aUseCharInfo.GetSize ();
	pWnd->nX = 32 + 16 * 7;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowTRADE													 */
/* 内容：ウィンドウの作成(取り引き)											 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
PWNDINFO MakeWindowTRADE(DWORD dwTblID)
{
	if (g_pTradeInfo) {
		return NULL;
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_TRADE;
	pWnd->pDib = new CImg32;
	pWnd->nData2 = 3;
	pWnd->nX = 32;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (16 * 22);
	pWnd->nCy = 16 * (2 + 5 + MAX_MYITEM);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	g_dwMsgIDTRADE = pWnd->dwID;

	g_pTradeInfo = new TRADEINFO;
	ZeroMemory (g_pTradeInfo, sizeof (TRADEINFO));
	g_pTradeInfo->dwTblID = dwTblID;
	for (int i = 0; i < MAX_MYITEM; i ++) {
		g_pTradeInfo->waItem1[i] = 0xFFFF;
		g_pTradeInfo->waItem2[i] = 0xFFFF;
	}
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowTRADEITEM												 */
/* 内容：ウィンドウの作成(取り引き：アイテム選択)							 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
PWNDINFO MakeWindowTRADEITEM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_TRADEITEM;
	pWnd->pDib = new CImg32;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 5;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowTRADEGOLD												 */
/* 内容：ウィンドウの作成(取り引き：金額設定)								 */
/* 日付：2004/01/03															 */
/* ========================================================================= */
PWNDINFO MakeWindowTRADEGOLD(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_TRADEGOLD;
	pWnd->pDib = new CImg32;

	pWnd->nX = 32 + 16 * 5;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = (16 * 4) + (8 * 4);
	pWnd->nCy = 16 * 3;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	pWnd->byData1 = 5;

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowGOLD													 */
/* 内容：ウィンドウの作成(所持金表示)										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowGOLD(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_GOLD;
	pWnd->pDib = new CImg32;
	g_dwMsgIDGold = pWnd->dwID;

	pWnd->nX = 32 + 16 * 21;
	pWnd->nY = 16 * 1;
	pWnd->nCx = (16 * 4) + (8 * 4);
	pWnd->nCy = 16 * 3;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBGAME_BOMBINFO										 */
/* 内容：ウィンドウの作成(サブゲーム：爆弾ゲーム情報)						 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBGAME_BOMBINFO(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_SUBGAME_BOMBINFO;
	pWnd->pDib = new CImg32;
	g_dwMsgIDSubGame = pWnd->dwID;

	pWnd->nX = 32 + 16 * 12;
	pWnd->nY = 16 * 0;
	pWnd->nCx = (16 * 4) + (8 * 24);
	pWnd->nCy = 16 * 4;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBGAME_BOMBSET										 */
/* 内容：ウィンドウの作成(サブゲーム：爆弾ゲーム設定)						 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBGAME_BOMBSET(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_SUBGAME_BOMBSET;
	pWnd->pDib = new CImg32;
	g_dwMsgIDCommand = pWnd->dwID;

	pWnd->nData2 = 8;
	pWnd->nX = 32;
	pWnd->nY = 50;
	pWnd->nCx = (16 * 4) + (16 * 12);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCHGPASSWORD											 */
/* 内容：ウィンドウの作成(パスワードの変更)									 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
PWNDINFO MakeWindowCHGPASSWORD(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 16 * 6;
	pWnd->nY = 50 + 16 * 10;
	pWnd->nCx = 16 * 13;
	pWnd->nCy = 16 * 5;
	pWnd->nType = WNDTYPE_CHGPASSWORD;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_hWndSpeech = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
			pWnd->nX + 32, pWnd->nY + 16 * 3, 8 * 15, 16, g_hWndMain, NULL, g_hInstance, NULL);
	SendMessage (g_hWndSpeech, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	ImmAssociateContext (g_hWndSpeech, NULL);
	g_OrgWndProcSpeech = (WNDPROC)GetWindowLong (g_hWndSpeech, GWL_WNDPROC);
	SetWindowLong (g_hWndSpeech, GWL_WNDPROC, (LONG)ChgPasswordWndProc);

	SetFocus (g_hWndSpeech);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->szPas) {
		SetWindowText (g_hWndSpeech, pChar->szPas);
	}

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCONF													 */
/* 内容：ウィンドウの作成(確認)												 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
PWNDINFO MakeWindowCONF(int x, int y, int nType)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType2 = nType;
	pWnd->nData2 = 2;
	pWnd->nX = x;
	pWnd->nY = y;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_CONF;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowJOB													 */
/* 内容：ウィンドウの作成(冒険者登録所)										 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowJOB(BYTE bySelect)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 21;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_JOB;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = JOBSTATE_OPEN;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew;
	if (bySelect == 0) {
		pWndNew = MakeWindowMSG ("＊｢ここは冒険者登録所です。どのような御用でしょう？@2", TRUE);

	} else if (bySelect == 1) {
		pWndNew = MakeWindowMSG ("＊｢他に御用は？@2", TRUE);

	} else if (bySelect == 2) {
		pWndNew = MakeWindowMSG ("＊｢登録完了しました。@1　 他に御用は？@2", TRUE);

	} else {
		pWndNew = MakeWindowMSG ("＊｢他に御用は？@2", TRUE);
	}
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowJOB_JOBSELECT											 */
/* 内容：ウィンドウの作成(冒険者登録所：職業の選択)							 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowJOB_JOBSELECT(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = g_JobInfo.GetCount ();
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 15;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_JOB;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = JOBSTATE_JOBSELECT;
	g_bProcShopConfirm = FALSE;

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG ("＊｢どの職業で登録しますか？@2", TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDMAGIC											 */
/* 内容：ウィンドウの作成(魔法)												 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDMAGIC(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = g_aMagicInfoWalk.GetSize (), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[g_aMagicInfoWalk[i]];
		nLenTmp = SboTextWidth (pInfo->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 1;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDMAGIC;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDMAGICBATTLE										 */
/* 内容：ウィンドウの作成(魔法(戦闘マップ))									 */
/* 日付：2004/09/25															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDMAGICBATTLE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = g_aMagicInfoBattle.GetSize (), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[g_aMagicInfoBattle[i]];
		nLenTmp = SboTextWidth (pInfo->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 1;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDMAGICBATTLE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"使用する特技を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_SELECTTEAM										 */
/* 内容：ウィンドウの作成(戦場ゲーム：チーム選択)							 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_SELECTTEAM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_SELECTTEAM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG ("どちらのチームに参加しますか？@2", FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_WAIT											 */
/* 内容：ウィンドウの作成(戦場ゲーム：他のプレイヤー待ち)					 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_WAIT(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 15;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_WAIT;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_SELECTRULE										 */
/* 内容：ウィンドウの作成(戦場ゲーム：ルール選択)							 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_SELECTRULE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 15;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_SELECTRULE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG ("勝利条件を選択してください。@2", FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_SELECTJOB										 */
/* 内容：ウィンドウの作成(戦場ゲーム：職業選択)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_SELECTJOB(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 4;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_SELECTJOB;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG ("職業を選択してください。@2", FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_SELECTTURN										 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動順序)								 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_SELECTTURN(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * (2 + 4);
	pWnd->nType = WNDTYPE_SUBWAR_SELECTTURN;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"行動順序を設定してください。@1"
					"決定ボタンを押したキャラが一つ上にいきます。@1"
					"上にあるほど先に行動します。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN											 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動選択)								 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 4;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + 4);
	pWnd->nType = WNDTYPE_SUBWAR_TURN;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"行動を選択してください。@1"
					"移動せず待機するとHPが回復します。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_MOVE										 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(移動))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_MOVE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 4;
	pWnd->nX = 16 * 5;
	pWnd->nY = 16 * 5;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + 4);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_MOVE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"移動する方向を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_ATACK										 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(攻撃))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_ATACK(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 4;
	pWnd->nX = 16 * 5;
	pWnd->nY = 16 * 6;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + 4);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_ATACK;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"攻撃する方向を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_DIFENCE_FIGHTER							 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(防御：戦士))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_FIGHTER(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_DIFENCE_FIGHTER;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"防御行動を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_DIFENCE_MARTIAL							 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(防御：武道家))					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_MARTIAL(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_DIFENCE_MARTIAL;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"防御行動を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_DIFENCE_WITCH								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(防御：魔法使い))					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"防御行動を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_DIFENCE_PRIEST								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(防御：僧侶))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_PRIEST(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_DIFENCE_PRIEST;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"防御行動を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_DIFENCE_WITCH2								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(魔法防御：魔法使い))				 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH2(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH2;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"防御行動を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_MAGIC_WITCH								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(魔法：魔法使い))					 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_WITCH(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 5;
	pWnd->nY = 16 * 7;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_MAGIC_WITCH;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"魔法を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_MAGIC_TARGET								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(魔法対象者選択))					 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_TARGET(void)
{
	PWNDINFO pWnd;

	if (g_pWarMagicTarget->byCount > 0) {
		pWnd = new WNDINFO;
		ZeroMemory (pWnd, sizeof (WNDINFO));
		pWnd->dwID = g_dwNewID;
		g_dwNewID ++;
		pWnd->nData2 = g_pWarMagicTarget->byCount;
		pWnd->nX = 16 * 7;
		pWnd->nY = 16 * 10;
		pWnd->nCx = 16 * 10;
		pWnd->nCy = 16 * (2 + pWnd->nData2);
		pWnd->nType = WNDTYPE_SUBWAR_TURN_MAGIC_TARGET;
		pWnd->pDib = new CImg32;
		pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
		pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	} else {
		pWnd = MakeWindowMSG ("対象者が見つかりません。@2", FALSE);
	}

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBWAR_TURN_MAGIC_PRIEST								 */
/* 内容：ウィンドウの作成(戦場ゲーム：行動(魔法：僧侶))						 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_PRIEST(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 5;
	pWnd->nY = 16 * 7;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBWAR_TURN_MAGIC_PRIEST;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG (
					"魔法を選択してください。@2",
					FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowITEMEQU												 */
/* 内容：ウィンドウの作成(装備アイテム)										 */
/* 日付：2004/09/18															 */
/* ========================================================================= */
PWNDINFO MakeWindowITEMEQU(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < MAX_EQUTYPE; i ++) {
		if (pChar->waEqu[i] == 0xFFFF) {
			continue;
		}
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
		nLenTmp = SboTextWidth (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	pWnd->nX	= 32 + 16 * 10;
	pWnd->nY	= 18 + 16 * 12;
	pWnd->nCx	= 16 * 2 + (nLen * 8) + 32;
	pWnd->nCy	= 16 * 12;
	pWnd->nType	= WNDTYPE_ITEMEQU;
	pWnd->pDib	= new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSTATUS_ATTR											 */
/* 内容：ウィンドウの作成(ステータス表示(属性))								 */
/* 日付：2004/09/19															 */
/* ========================================================================= */
PWNDINFO MakeWindowSTATUS_ATTR(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX	= 32;
	pWnd->nY	= 18 + 16 * 24;
	pWnd->nCx	= 32 + 16 * 18;
	pWnd->nCy	= 32 + 16 * 2;
	pWnd->nType	= WNDTYPE_STATUS_ATTR;
	pWnd->pDib	= new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowBATTLE_COMMAND											 */
/* 内容：ウィンドウの作成(戦闘：コマンド)									 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
PWNDINFO MakeWindowBATTLE_COMMAND(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX = 32;
	pWnd->nY = 50;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * 5;
	pWnd->nType = WNDTYPE_BATTLE_COMMAND;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_dwMsgIDCommand = pWnd->dwID;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowBATTLE_TARGET											 */
/* 内容：ウィンドウの作成(戦闘：ターゲット選択)								 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
PWNDINFO MakeWindowBATTLE_TARGET(
	int nArea,			/* [in] 範囲 */
	BOOL bMyChar,		/* [in] 自分を含める？ */
	int nType)			/* [in] 種別 */
{
	int nResult, nLen;
	PCHARINFO pMyChar;
	PWNDINFO pWnd = new WNDINFO;
	PUSECHARINFO pInfo;

	SAFE_DELETE (g_pTargetSelectInfo);
	nLen = 0;
	pMyChar = g_aCharInfo[g_nMyCharID];

	nResult = GetUseCharInfo (nArea, pMyChar, bMyChar, nType, g_aUseCharInfo, nLen);
	if (nResult <= 0) {
		return NULL;
	}

	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nX = 48;
	pWnd->nY = 66;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + nResult);
	pWnd->nType = WNDTYPE_BATTLE_TARGET;
	pWnd->nData2 = g_aUseCharInfo.GetSize ();
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	g_pTargetSelectInfo = new TARGETSELECTINFO;
	ZeroMemory (g_pTargetSelectInfo, sizeof (TARGETSELECTINFO));

	pInfo = g_aUseCharInfo[0];
	g_pTargetSelectInfo->dwTblID = pInfo->dwTblID;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBSUGOROKU_JOINMODE									 */
/* 内容：ウィンドウの作成(すごろく：参加モード選択)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBSUGOROKU_JOINMODE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 8;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBSUGOROKU_JOINMODE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	PWNDINFO pWndNew;
	pWndNew = MakeWindowMSG ("参加モードを選択してください。@2", FALSE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBSUGOROKU_COMMAND									 */
/* 内容：ウィンドウの作成(すごろく：行動選択)								 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBSUGOROKU_COMMAND(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 16;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBSUGOROKU_COMMAND;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSUBSUGOROKU_IDLE										 */
/* 内容：ウィンドウの作成(すごろく：平常状態)								 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
PWNDINFO MakeWindowSUBSUGOROKU_IDLE(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 1;
	pWnd->nX = 16 * 4;
	pWnd->nY = 16 * 4;
	pWnd->nCx = 16 * 16;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SUBSUGOROKU_IDLE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowITEMINFO												 */
/* 内容：ウィンドウの作成(アイテム情報)										 */
/* 日付：2005/03/12															 */
/* ========================================================================= */
PWNDINFO MakeWindowITEMINFO(int x, int y)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = x;
	pWnd->nY = y;
	pWnd->nCx = ITEMINFO_CX;
	pWnd->nCy = ITEMINFO_CY;
	pWnd->nType = WNDTYPE_ITEMINFO;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowITEMINFOList											 */
/* 内容：アイテム一覧ウィンドウの横にアイテム情報ウィンドウを付ける		 */
/* 備考：道具欄・預かり所・店で売る時に、カーソルのアイテムの詳細を出す	 */
/*		 置き場所は描画の時に一覧ウィンドウを見て決める					 */
/* ========================================================================= */
PWNDINFO MakeWindowITEMINFOList(PWNDINFO pWndList)
{
	PWNDINFO pWnd;

	pWnd = MakeWindowITEMINFO (pWndList->nX + pWndList->nCx, pWndList->nY);
	pWnd->nType2 = ITEMINFOPOS_LIST;
	pWnd->nData2 = (int)pWndList->dwID;
	pWndList->pWndTmp = pWnd;
	g_WndInfo.Add (pWnd);

	UpdateWindowITEMINFOList (pWndList);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：UpdateWindowITEMINFOList											 */
/* 内容：一覧のカーソル位置のアイテムをアイテム情報ウィンドウに出す		 */
/* ========================================================================= */
void UpdateWindowITEMINFOList(PWNDINFO pWndList)
{
	PWNDINFO pWnd;
	PCHARINFO pChar;
	int nNo, nItemID;

	if ((pWndList == NULL) || (pWndList->pWndTmp == NULL)) {
		return;
	}
	pWnd = (PWNDINFO)pWndList->pWndTmp;
	if (pWnd->nType != WNDTYPE_ITEMINFO) {
		return;
	}

	nItemID = 0;
	nNo = pWndList->nData1;
	if (pWndList->nType == WNDTYPE_MAKER) {
		/* 作り手の一覧はレシピでできる物を出す */
		int nRecipe = MakerGetCursorRecipe (pWndList);
		if (nRecipe >= 0) {
			nItemID = g_aRecipeInfo[nRecipe].wItem;
		}
	} else if ((pWndList->nType == WNDTYPE_LEAVE) && (g_nProcShopState == LEAVESTATE_ITEMSELECTGET)) {
		/* 預かり所の引き取る一覧は byData1 だけスクロールしている */
		nNo += pWndList->byData1;
		if (g_pLeaveInfo && (nNo >= 0) && (nNo < g_pLeaveInfo->byCount) && (nNo < MAX_LEAVEITEM)) {
			nItemID = g_pLeaveInfo->waItem[nNo];
		}
	} else {
		/* 自分の持ち物の一覧 */
		if ((g_nMyCharID >= 0) && (g_nMyCharID < g_aCharInfo.GetSize ()) && (nNo >= 0) && (nNo < MAX_MYITEM)) {
			pChar = g_aCharInfo[g_nMyCharID];
			nItemID = pChar->waItem[nNo];
		}
	}
	if ((nItemID < 0) || (nItemID >= g_aItemInfo.GetSize ())) {
		nItemID = 0;
	}

	pWnd->nData1 = nItemID;
	pWnd->dwTimeDrawStart = 0;
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：	MakeWindowDEAD_COMMAND											 */
/* 内容：	ウィンドウの作成(死亡中：コマンド)								 */
/* 日付：	2004/11/01														 */
/* ========================================================================= */

PWNDINFO MakeWindowDEAD_COMMAND(void)
{
	PWNDINFO pWnd;

	pWnd = GetNewWindow ();
	pWnd->nX	= 16 * 8;
	pWnd->nY	= 16 * 9;
	pWnd->nCx	= 16 * 15;
	pWnd->nCy	= 16 * 4;
	pWnd->nType	= WNDTYPE_DEAD_COMMAND;
	pWnd->pDib	= new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_dwMsgIDCommand = pWnd->dwID;
	return pWnd;
}

/* Copyright(C)URARA-works 2003-2005 */

/* ========================================================================= */
/* 関数名：	MakeWindowSTATUSMAGIC											 */
/* 内容：	ウィンドウの作成(強さ：覚えている特技)							 */
/* 			「強さ」で決定を押すと、ステータスの次に出す一覧				 */
/* 日付：	2026/11/07														 */
/* ========================================================================= */

PWNDINFO MakeWindowSTATUSMAGIC(void)
{
	PWNDINFO pWnd;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = 0, nLen = 0;

	/* 覚えている特技の数と、いちばん長い名前の幅（半角何文字分か） */
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		BYTE byMagicID = pChar->abyMagic[i];
		if (byMagicID == 0) {
			break;
		}
		if (byMagicID >= g_aMagicInfo.GetSize ()) {
			continue;
		}
		nLen = max (nLen, SboTextWidth (g_aMagicInfo[byMagicID]->szName));
		nCount ++;
	}

	pWnd = GetNewWindow ();
	pWnd->nData1	= 0;			/* カーソル位置 */
	pWnd->nData2	= nCount;		/* 特技の数 */
	pWnd->nX		= 32;
	pWnd->nY		= 50 + 16 * 2;
	/* 左の枠とカーソル + 名前 + 使える場面(半角8) + 消費Mana(半角8) + 右の枠 */
	pWnd->nCx		= 32 + nLen * 8 + 16 + 8 * 8 + 8 + 8 * 8 + 16;
	pWnd->nCx		= (pWnd->nCx + 15) / 16 * 16;
	pWnd->nCx		= max (pWnd->nCx, 16 * 20);
	pWnd->nCx		= min (pWnd->nCx, SCRSIZEX - pWnd->nX);
	pWnd->nCy		= STATUSMAGIC_LIST_CY (nCount) + STATUSMAGIC_EXPLAIN_CY;
	pWnd->nType		= WNDTYPE_STATUSMAGIC;
	pWnd->pDib		= new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	return pWnd;
}

/* ========================================================================= */
/* 作り手（レシピ）の画面                                                    */
/* 一覧（MAKERWND_LIST）と、その下の「はい／いいえ」（MAKERWND_CONF）。       */
/* 一覧の横には MakeWindowITEMINFOList で、できる物の詳細と材料を出す。       */
/* ========================================================================= */

/* ========================================================================= */
/* 関数名：MakerGetRecipeList												 */
/* 内容：作り手の一覧に出すレシピの番号を、並び順（小さい順。同じなら番号順）で取る */
/*		 有効で、できるアイテムが実在するものだけ。戻り値：件数				 */
/* ========================================================================= */
int MakerGetRecipeList(int nMaker, int *pnList, int nMax)
{
	int i, j, nCount;
	PRECIPEINFO pRecipe;

	nCount = 0;
	for (i = 0; (i < g_nRecipeCount) && (i < MAX_RECIPE); i ++) {
		pRecipe = &g_aRecipeInfo[i];
		if ((pRecipe->byState != RECIPESTATE_ON) || ((int)pRecipe->byMaker != nMaker)) {
			continue;
		}
		if ((pRecipe->wItem == 0) || ((int)pRecipe->wItem >= g_aItemInfo.GetSize ())) {
			continue;
		}
		if (nCount >= nMax) {
			break;
		}
		for (j = nCount; j > 0; j --) {
			if (g_aRecipeInfo[pnList[j - 1]].wSort <= pRecipe->wSort) {
				break;
			}
			pnList[j] = pnList[j - 1];
		}
		pnList[j] = i;
		nCount ++;
	}
	return nCount;
}

/* ========================================================================= */
/* 関数名：MakerGetCursorRecipe												 */
/* 内容：一覧のカーソル位置のレシピの番号（無ければ -1）					 */
/* ========================================================================= */
int MakerGetCursorRecipe(PWNDINFO pWndList)
{
	int anList[MAX_RECIPE], nCount, nIndex;

	if ((pWndList == NULL) || (pWndList->nType != WNDTYPE_MAKER) || (g_nProcMakerID < 0)) {
		return -1;
	}
	nCount = MakerGetRecipeList (g_nProcMakerID, anList, MAX_RECIPE);
	nIndex = (int)pWndList->byData1 + pWndList->nData1;
	if ((nIndex < 0) || (nIndex >= nCount)) {
		return -1;
	}
	return anList[nIndex];
}

/* ========================================================================= */
/* 関数名：MakerCountHave													 */
/* 内容：材料として使える数（持ち物のうち装備中・捨てられない物を除いたもの）	 */
/*		 + 預かり所に預けてある数。サーバーの数え方と同じにしておく			 */
/* ========================================================================= */
int MakerCountHave(WORD wItem)
{
	PCHARINFO pChar;
	BOOL bEqu;
	int i, j, nCount;

	if ((wItem == 0) || ((int)wItem >= g_aItemInfo.GetSize ())) {
		return 0;
	}
	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return 0;
	}
	nCount = 0;
	pChar = g_aCharInfo[g_nMyCharID];
	if (!(g_aItemInfo[wItem]->byFlg & ITEMFLG_DROP)) {
		for (i = 0; i < MAX_MYITEM; i ++) {
			if (pChar->waItem[i] != wItem) {
				continue;
			}
			bEqu = FALSE;
			for (j = 0; j < 10; j ++) {
				if (pChar->waEqu[j] == i) {
					bEqu = TRUE;
				}
			}
			if (!bEqu) {
				nCount ++;
			}
		}
	}
	if (g_bProcMakerLeaveOK && g_pLeaveInfo) {
		for (i = 0; (i < (int)g_pLeaveInfo->byCount) && (i < MAX_LEAVEITEM); i ++) {
			if (g_pLeaveInfo->waItem[i] == wItem) {
				nCount += g_pLeaveInfo->byItemCount[i];
			}
		}
	}
	return nCount;
}

/* ========================================================================= */
/* 関数名：MakerCanMake														 */
/* 内容：材料と手数料が足りているか（置き場所はサーバーが決める）			 */
/* ========================================================================= */
BOOL MakerCanMake(int nRecipe)
{
	PRECIPEINFO pRecipe;
	int i;

	if ((nRecipe < 0) || (nRecipe >= MAX_RECIPE)) {
		return FALSE;
	}
	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return FALSE;
	}
	pRecipe = &g_aRecipeInfo[nRecipe];
	if (g_aCharInfo[g_nMyCharID]->dwGold < pRecipe->dwGold) {
		return FALSE;
	}
	for (i = 0; i < MAX_RECIPEMAT; i ++) {
		if ((pRecipe->awMatItem[i] == 0) || (pRecipe->abyMatCount[i] == 0)) {
			continue;
		}
		if (MakerCountHave (pRecipe->awMatItem[i]) < (int)pRecipe->abyMatCount[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MakerStateHash													 */
/* 内容：画面に出ている内容（一覧・持ち物・預かり所・所持金）が変わったか調べる値 */
/* ========================================================================= */
DWORD MakerStateHash(int nMaker)
{
	DWORD dw = 2166136261UL;
	int anList[MAX_RECIPE], nCount, i, j;
	PRECIPEINFO pRecipe;

#define MAKER_MIX(v)	dw = (dw ^ (DWORD)(v)) * 16777619UL
	nCount = MakerGetRecipeList (nMaker, anList, MAX_RECIPE);
	MAKER_MIX (nCount);
	for (i = 0; i < nCount; i ++) {
		pRecipe = &g_aRecipeInfo[anList[i]];
		MAKER_MIX (anList[i]);
		MAKER_MIX (pRecipe->wItem);
		MAKER_MIX (pRecipe->byCount);
		MAKER_MIX (pRecipe->dwGold);
		for (j = 0; j < MAX_RECIPEMAT; j ++) {
			MAKER_MIX (pRecipe->awMatItem[j]);
			MAKER_MIX (pRecipe->abyMatCount[j]);
		}
	}
	if ((g_nMyCharID >= 0) && (g_nMyCharID < g_aCharInfo.GetSize ())) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		MAKER_MIX (pChar->dwGold);
		for (i = 0; i < MAX_MYITEM; i ++) {
			MAKER_MIX (pChar->waItem[i]);
		}
		for (i = 0; i < 10; i ++) {
			MAKER_MIX (pChar->waEqu[i]);
		}
	}
	MAKER_MIX (g_bProcMakerLeaveOK);
	if (g_bProcMakerLeaveOK && g_pLeaveInfo) {
		MAKER_MIX (g_pLeaveInfo->byCount);
		for (i = 0; (i < (int)g_pLeaveInfo->byCount) && (i < MAX_LEAVEITEM); i ++) {
			MAKER_MIX (g_pLeaveInfo->waItem[i]);
			MAKER_MIX (g_pLeaveInfo->byItemCount[i]);
		}
	}
#undef MAKER_MIX
	return dw;
}

/* ========================================================================= */
/* 関数名：MakerScrollFit													 */
/* 内容：一覧の nIndex 番目にカーソルを置き、画面内に収まるようスクロールを整える	 */
/*		 pWndList->byData1 が先頭の番号、nData1 が画面内のカーソル位置			 */
/* ========================================================================= */
void MakerScrollFit(PWNDINFO pWndList, int nIndex, int nCount)
{
	int nRows, nTop;

	nRows = max (pWndList->nData2, 1);
	if (nIndex >= nCount) {
		nIndex = nCount - 1;
	}
	if (nIndex < 0) {
		nIndex = 0;
	}
	nTop = (int)pWndList->byData1;
	if (nIndex < nTop) {
		nTop = nIndex;
	}
	if (nIndex >= nTop + nRows) {
		nTop = nIndex - nRows + 1;
	}
	if (nTop + nRows > nCount) {
		nTop = max (nCount - nRows, 0);
	}
	pWndList->byData1 = (BYTE)nTop;
	pWndList->nData1 = nIndex - nTop;
}

/* ========================================================================= */
/* 関数名：MakerSetMsg														 */
/* 内容：作り手の画面のメッセージを差し替える								 */
/*		 台詞は管理画面で書いた文なので、@0 @1 @2 以外の制御コード（アイテムが	 */
/*		 もらえる @4 など）は使わせず、ただの文字にする						 */
/* ========================================================================= */
void MakerSetMsg(PWNDINFO pWndList, LPCSTR pszMsg)
{
	char szTmp[MAX_MAKERMSG + 8];
	PWNDINFO pWndNew;
	int i, nLen;

	ZeroMemory (szTmp, sizeof (szTmp));
	strncpy (szTmp, pszMsg, MAX_MAKERMSG + 3);
	nLen = strlen (szTmp);
	for (i = 0; i < nLen; i ++) {
		if (szTmp[i] == '@') {
			if ((szTmp[i + 1] == '0') || (szTmp[i + 1] == '1') || (szTmp[i + 1] == '2')) {
				i ++;
			} else {
				szTmp[i] = ' ';
			}
		} else if ((BYTE)szTmp[i] >= 0x80) {
			i += max (SboTextCharLen (&szTmp[i]), 1) - 1;
		}
	}

	pWndNew = MakeWindowMSG (szTmp, TRUE);
	pWndNew->pParent = pWndList;
	pWndList->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
}

/* ========================================================================= */
/* 関数名：MakerSetTalkMsg													 */
/* 内容：作り手の話しかけた時の台詞を出す（台詞が空なら何も出さない）		 */
/* ========================================================================= */
void MakerSetTalkMsg(PWNDINFO pWndList)
{
	if ((g_nProcMakerID < 0) || (g_nProcMakerID >= MAX_MAKER)) {
		return;
	}
	if (g_aMakerInfo[g_nProcMakerID].szMsgTalk[0] == 0) {
		MakerSetMsg (pWndList, "何を作りましょうか？@2");
		return;
	}
	MakerSetMsg (pWndList, g_aMakerInfo[g_nProcMakerID].szMsgTalk);
}

/* ========================================================================= */
/* 関数名：MakeWindowMAKER													 */
/* 内容：ウィンドウの作成(作り手：作れる物の一覧)							 */
/*		 nData1=画面内のカーソル位置 byData1=先頭の番号 nData2=表示行数		 */
/*		 台詞のメッセージ窓は子、横のアイテム情報は pWndTmp につなぐ			 */
/* ========================================================================= */
PWNDINFO MakeWindowMAKER(int nMaker)
{
	int anList[MAX_RECIPE], nCount;
	PWNDINFO pWnd;

	nCount = MakerGetRecipeList (nMaker, anList, MAX_RECIPE);

	pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = min (max (nCount, 1), MAKER_LISTROWS);
	pWnd->nX = 16;
	pWnd->nY = 16;
	/* 一番長い「名前 xN」が入る幅にする（画面の幅まで） */
	int nCx = MAKER_LISTCX;
	for (int i = 0; i < nCount; i ++) {
		PRECIPEINFO pRecipe = &g_aRecipeInfo[anList[i]];
		if (pRecipe->wItem >= g_aItemInfo.GetSize ()) {
			continue;
		}
		int nW = SboTextWidth (g_aItemInfo[pRecipe->wItem]->szName);
		if (pRecipe->byCount > 1) {
			nW += (pRecipe->byCount >= 10) ? 4 : 3;	/* " x3" の分 */
		}
		nCx = max (nCx, 32 + nW * 8 + 16);
	}
	nCx = (nCx + 15) / 16 * 16;
	/* 右上の所持金の窓（x=16*23 から）と重ならない幅まで */
	pWnd->nCx = min (nCx, 16 * 22);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MAKER;
	pWnd->nType2 = MAKERWND_LIST;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcMakerID = nMaker;
	g_bProcMakerLeaveOK = FALSE;
	g_bProcMakerWait = FALSE;
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMAKER_CONF												 */
/* 内容：ウィンドウの作成(作り手：「はい／いいえ」)							 */
/*		 dwData=作ってもらうレシピの番号 nData1=カーソル(0:はい 1:いいえ)		 */
/* ========================================================================= */
PWNDINFO MakeWindowMAKER_CONF(int nRecipe)
{
	PWNDINFO pWnd = new WNDINFO;

	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16;
	pWnd->nY = 16 * 13;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MAKER;
	pWnd->nType2 = MAKERWND_CONF;
	pWnd->dwData = (DWORD)nRecipe;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	return pWnd;
}
