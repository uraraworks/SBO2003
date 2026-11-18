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
#include "SboCliSendCmd.h"

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
PWNDINFO MakeWindowIMPORTANT					(void);				/* 大事な物の一覧 */
PWNDINFO MakeWindowCOMMANDITEMBATTLE			(void);				/* 道具(戦闘マップ) */
PWNDINFO MakeWindowCOMMANDITEMPROC				(void);				/* 道具処理選択 */
PWNDINFO MakeWindowSORTMENU						(BYTE);				/* 並べ替え：入れ替え・種類順・50音順 */
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
PWNDINFO MakeWindowLEAVE_DISPOSEMENU			(void);				/* 預かり所：手放す品を売る・捨てる */
PWNDINFO MakeWindowLEAVE_DISPOSECOUNT			(BYTE);				/* 預かり所：手放す個数 */
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
PWNDINFO MakeWindowBATTLE_TARGET				(int nArea, BOOL bMyChar, int nType, BOOL bMerc);	/* 戦闘：ターゲット選択 */

PWNDINFO MakeWindowSUBSUGOROKU_JOINMODE			(void);				/* すごろく：参加モード選択 */
PWNDINFO MakeWindowSUBSUGOROKU_COMMAND			(void);				/* すごろく：行動状態 */
PWNDINFO MakeWindowSUBSUGOROKU_IDLE				(void);				/* すごろく：平常状態 */

PWNDINFO MakeWindowMAKER						(int);				/* 作り手：作れる物の一覧 */
PWNDINFO MakeWindowMAKER_CONF					(int);				/* 作り手：確認 */
PWNDINFO MakeWindowAPPRAISE					(int);				/* 鑑定屋：未鑑定の持ち物の一覧 */
int AppraiseGetList							(int *, int);		/* 未鑑定の持ち物の位置を取得 */
DWORD AppraiseCost							(WORD);				/* 鑑定の代金 */
PWNDINFO MakeWindowTOWNWARP					(int nMode);				/* 町転送：行き先の選択 */
PWNDINFO MakeWindowQUEST					(int nMode);				/* ギルドの依頼：一覧・進み具合 */
int MakerGetRecipeList							(int, int *, int);	/* 作り手の一覧に出すレシピの番号を並び順で取得 */
int MakerGetCursorRecipe						(PWNDINFO);			/* 一覧のカーソル位置のレシピの番号 */
int MakerCountHave								(WORD);				/* 材料として使える数（持ち物+預かり所） */
BOOL MakerCanMake								(int);				/* 材料と手数料が足りているか */
DWORD MakerStateHash							(int);				/* 画面に出ている内容が変わったか調べる値 */
void MakerSetMsg								(PWNDINFO, LPCSTR);	/* 作り手の画面のメッセージを差し替える */
void MakerSetTalkMsg							(PWNDINFO);			/* 作り手の話しかけた時の台詞を出す */
void MakerScrollFit								(PWNDINFO, int, int);	/* 一覧のカーソルとスクロールを整える */
int ListScrollTop								(int, int, int, int, int);	/* 一覧の先頭位置を余白付きで決める */
void ListScrollFitMargin						(PWNDINFO, int, int, int);	/* 一覧のカーソルとスクロールを余白付きで整える */
BOOL ListScrollKey								(PWNDINFO, BYTE, int);	/* 一覧の上下左右キーでカーソルを動かす */
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
	g_bLoginHide = FALSE;
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
	/* 自分か仲間が依頼を受けている時だけ「依頼」の段を出す（nData2=1）。1 行ぶん高くする */
	if (QuestHasAny ()) {
		pWnd->nData2 = 1;
		pWnd->nCy = 16 * 8;
	}
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
	int i, nCount = GetItemCount (g_nMyCharID), nRows = GetItemListRows (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[i]));
		nLen = max (nLen, nLenTmp);
	}
	/* 大事な物を持っている時は、最後の行に「大事な物…」を出す */
	if (nRows > nCount) {
		nLen = max (nLen, SboTextWidth ("大事な物…"));
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = min (nRows, LISTROWS_MAX);						/* 10行を超える分はスクロール */
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCx = max (pWnd->nCx, 16 * 8);						/* 入れ替え中の題字「入れ替え」と右上の▲が重ならない幅 */
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDITEM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowIMPORTANT												 */
/* 内容：ウィンドウの作成(大事な物の一覧。見るだけ)							 */
/* 備考：道具の一覧の最後の行「大事な物…」から開く。10行ずつ見せてスクロール	 */
/* ========================================================================= */
PWNDINFO MakeWindowIMPORTANT(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	int i, nCount = GetImportantCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < nCount; i ++) {
		int nItemID = GetImportantItem (g_nMyCharID, i);
		if ((nItemID <= 0) || (nItemID >= g_aItemInfo.GetSize ())) {
			continue;
		}
		nLenTmp = SboTextWidth (g_aItemInfo[nItemID]->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = min (max (nCount, 1), LISTROWS_MAX);				/* 10行を超える分はスクロール */
	pWnd->nX = 32 + 16 * 4;
	pWnd->nY = 50 + 16 * 3;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCx = max (pWnd->nCx, 16 * 8);							/* 題字「大事な物」と右上の▲が重ならない幅 */
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_IMPORTANT;
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
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[i]));
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData2 = min (nCount, LISTROWS_MAX);						/* 10行を超える分はスクロール */
	pWnd->nX = 32 + 16 * 3;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCx = max (pWnd->nCx, 16 * 7);						/* 題字「道具」と右上の▲が重ならない幅 */
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
	pWnd->nData2 = 3;
	pWnd->nX = 32 + 16 * 8;
	pWnd->nY = 50 + 16 * 5;
	pWnd->nCx = 16 * 7;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_COMMANDITEMPROC;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowSORTMENU												 */
/* 内容：ウィンドウの作成(並べ替え：入れ替え・種類順・50音順)				 */
/*       byTarget=対象(SORTITEMTARGET_*。手持ちの道具・預かり所で共用)		 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowSORTMENU(BYTE byTarget)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->byData1 = byTarget;
	if (byTarget == SORTITEMTARGET_LEAVE) {
		pWnd->nX = 16 * 21;
		pWnd->nY = 16 * 8;
	} else if (byTarget == SORTITEMTARGET_MAGIC) {
		/* 「強さ」の特技の一覧の右寄りに重ねる */
		pWnd->nX = 32 + 16 * 15;
		pWnd->nY = 50 + 16 * 3;
	} else {
		pWnd->nX = 32 + 16 * 9;
		pWnd->nY = 50 + 16 * 6;
	}
	pWnd->nCx = 16 * 7;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_SORTMENU;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	/* 預かり所では、預かり所の人のセリフを出す */
	if (byTarget == SORTITEMTARGET_LEAVE) {
		PWNDINFO pWndNew = MakeWindowMSG ("＊｢どのように並べ替えましょう？@2", TRUE);
		pWndNew->pParent = pWnd;
		pWnd->pChild = pWndNew;
		g_WndInfo.Add (pWndNew);
	}
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
	pWnd->nData2 = 11;
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
	/* 買う時のアイテム情報に出す預かり所の数のため、預かり所の中身を取りに行く */
	g_bProcShopLeaveOK = FALSE;
	SendCmdLEAVEITEMGET ((DWORD)-1);
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
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[i]));
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = min (nCount, LISTROWS_MAX);						/* 10行を超える分はスクロール */
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
	g_bProcShopLeaveOK = FALSE;
	SendCmdLEAVEITEMGET ((DWORD)-1);

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
	pWnd->dwData = (DWORD)pChar->wDFMagic;			/* 魔法防御（後から足した値なので2行目に出す） */
	pWnd->nX = 32 + 16;
	pWnd->nY = 16 * 14;
	pWnd->nCx = 16 * 13;
	pWnd->nCy = 16 * 4;
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
	int i;
	BOOL bMerc = FALSE;

	/* 仲間の一覧に傭兵がいれば「傭兵と別れる」も出す(誰の傭兵かはサーバーが見る) */
	for (i = 0; i < g_aPartyInfo.GetSize (); i ++) {
		int nNo = GetCharNo2 (g_aPartyInfo[i]);
		if ((nNo >= 0) && (g_aCharInfo[nNo]->byNpcType == NPCTYPE_MERCENARY)) {
			bMerc = TRUE;
			break;
		}
	}

	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = bMerc ? 5 : 4;
	pWnd->nX = 32;
	pWnd->nY = 16 * 7 + 2;
	/* 「傭兵と別れる」が出る時は、右に余裕ができるよう 1 文字ぶん広げる */
	pWnd->nCx = 16 * (bMerc ? 9 : 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
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
	pWnd->nData2 = 5;
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
		pWndNew = MakeWindowMSG ("＊｢これ以上お預かりできません。@1　 （100種類までです）@1　 他に御用は？@2", TRUE);

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

	} else if (bySelect == 10) {
		pWndNew = MakeWindowMSG ("＊｢きれいに並べ替えておきましたよ。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 11) {
		/* 手放して預かり品が無くなった */
		char szTmp[512];
		wsprintf (szTmp, "%s@1＊｢これでお預かりの品は無くなりました。@1　 他に御用は？@2", g_szLeaveResult);
		g_szLeaveResult[0] = 0;
		pWndNew = MakeWindowMSG (szTmp, TRUE);

	} else if (bySelect == 12) {
		pWndNew = MakeWindowMSG ("＊｢そのお品は手放せません。@1　 他に御用は？@2", TRUE);

	} else if (bySelect == 13) {
		pWndNew = MakeWindowMSG ("＊｢依頼の品はお預かりできません。@1　 他に御用は？@2", TRUE);

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
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[i]));
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = min (nCount, LISTROWS_MAX);						/* 10行を超える分はスクロール */
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

	/* 並べ替え・手放す時は状態を変えない（引き取る時と同じ一覧を使う） */
	if ((g_nProcShopState != LEAVESTATE_ITEMSORTSWAP) && (g_nProcShopState != LEAVESTATE_ITEMSELECTDISPOSE)) {
		g_nProcShopState = LEAVESTATE_ITEMSELECTGET;
	}
	g_bProcShopConfirm = FALSE;

	/* 手放した後などで作り直す時は、前に選んでいた辺りにカーソルを合わせる */
	if ((g_nLeaveListPos >= 0) && (g_pLeaveInfo->byCount > 0)) {
		int nPos = min (g_nLeaveListPos, (int)g_pLeaveInfo->byCount - 1);
		ListScrollFitMargin (pWnd, nPos, (int)g_pLeaveInfo->byCount, LISTSCROLL_MARGIN);
	}
	g_nLeaveListPos = -1;

	char szTmp[512];
	if (g_nProcShopState == LEAVESTATE_ITEMSORTSWAP) {
		wsprintf (szTmp, "＊｢現在%d品お預かりしています。@1　 入れ替える２つを順にお選びください。@2", g_pLeaveInfo->byCount);
	} else if (g_nProcShopState == LEAVESTATE_ITEMSELECTDISPOSE) {
		if (g_szLeaveResult[0]) {
			/* 手放した直後：結果を頭に付ける */
			wsprintf (szTmp, "%s@1＊｢他に手放すお品はありますか？@2", g_szLeaveResult);
			g_szLeaveResult[0] = 0;
		} else {
			wsprintf (szTmp, "＊｢現在%d品お預かりしています。@1　 どれを手放しますか？@1　 （売るか捨てるかは次に選べます）@2", g_pLeaveInfo->byCount);
		}
	} else {
		wsprintf (szTmp, "＊｢現在%d品お預かりしています。@1　 どれをお引取りなさいますか？@2", g_pLeaveInfo->byCount);
	}
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
/* 関数名：MakeWindowLEAVE_DISPOSEMENU										 */
/* 内容：ウィンドウの作成(預かり所：手放す品を売る・捨てる)					 */
/*       選んだ品は g_nProcShopSelect（預かり所の何番目か）					 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_DISPOSEMENU(void)
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

	g_nProcShopState = LEAVESTATE_DISPOSEMENU;
	g_bProcShopConfirm = FALSE;

	char szTmp[512];
	PITEMINFO pItem = g_aItemInfo[g_pLeaveInfo->waItem[g_nProcShopSelect]];
	int nCount = g_pLeaveInfo->byItemCount[g_nProcShopSelect];
	if (ItemGoldOf (g_pLeaveInfo->waItem[g_nProcShopSelect]) == 0) {
		wsprintf (szTmp, "＊｢%s（%d個）ですね。@1　 値打ちが無いので買い取れませんが、@1　 捨てることはできますよ。@2", ItemNameOf (g_pLeaveInfo->waItem[g_nProcShopSelect]), nCount);
	} else {
		wsprintf (szTmp, "＊｢%s（%d個）ですね。@1　 売るなら1個%dゴールドで@1　 買い取りますよ。@2", ItemNameOf (g_pLeaveInfo->waItem[g_nProcShopSelect]), nCount, max (ItemGoldOf (g_pLeaveInfo->waItem[g_nProcShopSelect]) / 4, (DWORD)1));
	}
	PWNDINFO pWndNew = MakeWindowMSG (szTmp, TRUE);
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowLEAVE_DISPOSECOUNT										 */
/* 内容：ウィンドウの作成(預かり所：手放す個数)								 */
/*       byMode=LEAVEDISPOSE_*。nData1=個数、nData2=最大、byData1=方法		 */
/*       ↑↓で1個ずつ（1と最大でひと回り）、←→で10個ずつ					 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
PWNDINFO MakeWindowLEAVE_DISPOSECOUNT(BYTE byMode)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nType = WNDTYPE_LEAVE;
	pWnd->pDib = new CImg32;

	PITEMINFO pItem = g_aItemInfo[g_pLeaveInfo->waItem[g_nProcShopSelect]];
	int nMax = g_pLeaveInfo->byItemCount[g_nProcShopSelect];
	if (byMode == LEAVEDISPOSE_SELL) {
		/* 所持金が 999999 を超えない分まで */
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		DWORD dwSell = max (ItemGoldOf (g_pLeaveInfo->waItem[g_nProcShopSelect]) / 4, (DWORD)1);
		DWORD dwRoom = (pChar->dwGold >= 999999) ? 0 : (999999 - pChar->dwGold);
		nMax = min (nMax, (int)(dwRoom / dwSell));
	}
	pWnd->nData1 = 1;
	pWnd->nData2 = nMax;
	pWnd->byData1 = byMode;

	pWnd->nX = 32 + 16 * 15;
	pWnd->nY = 50 + 16 * 10;
	pWnd->nCx = 16 * 2 + 8 * 13;
	pWnd->nCy = 16 * ((byMode == LEAVEDISPOSE_SELL) ? 4 : 3);
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcShopState = LEAVESTATE_DISPOSECOUNT;
	g_bProcShopConfirm = FALSE;

	char szTmp[512];
	wsprintf (szTmp, "＊｢%sをいくつ%s？@1　 ↑↓で1個ずつ、←→で10個ずつ@1　 変えられます。@2", ItemNameOf (g_pLeaveInfo->waItem[g_nProcShopSelect]),
			(byMode == LEAVEDISPOSE_SELL) ? "売りますか" : "捨てますか");
	PWNDINFO pWndNew = MakeWindowMSG (szTmp, TRUE);
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
/* 関数名：GetTradeTargets													 */
/* 内容：取り引きできる相手の一覧(テーブルID)を集めて数を返す				 */
/*		 目の前の人が先、その後に仲間のHPが見えている範囲の仲間・傭兵		 */
/* ========================================================================= */
#define TRADE_PARTYRANGE	(10)		/* 仲間となら、向き合わなくても取り引きできる範囲(仲間のHPが見える範囲) */
int GetTradeTargets(DWORD *padwTblID, int nMax)
{
	int i, j, nCount;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
		  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	PMAPPARTSINFO pParts = &g_pMapPartsInfo[g_pMapInfo->pwMap[dwX + dwY * g_pMapInfo->nCX]];
	if (pParts->byHitType == PARTSHIT_COUNTER) {
		dwX += anPosAddTblX[pChar->byDirection];
		dwY += anPosAddTblY[pChar->byDirection];
	}

	nCount = 0;
	for (i = 0; (i < g_aCharInfo.GetSize ()) && (nCount < nMax); i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* 目の前にいる？ */
		if ((dwX == pCharTmp->dwX) && (dwY == pCharTmp->dwY)) {
			/* プレイヤーか、傭兵(道具を持たせる) */
			if ((pCharTmp->byNpcType == 0) || (pCharTmp->byNpcType == NPCTYPE_MERCENARY)) {
				padwTblID[nCount ++] = pCharTmp->dwTblID;
			}
		}
	}

	/* 仲間(傭兵も)は、HPが見えている範囲にいれば向き合っていなくても取り引きできる */
	for (i = 0; (i < g_aPartyInfo.GetSize ()) && (nCount < nMax); i ++) {
		int nNo = GetCharNo2 (g_aPartyInfo[i]);
		if ((nNo < 0) || (nNo == g_nMyCharID)) {
			continue;
		}
		PCHARINFO pCharTmp = g_aCharInfo[nNo];
		if ((pCharTmp->byNpcType != 0) && (pCharTmp->byNpcType != NPCTYPE_MERCENARY)) {
			continue;
		}
		if ((pCharTmp->wMaxHP == 0) || (pCharTmp->wHP == 0) || (pCharTmp->byCharState == CHARSTATE_DEAD)) {
			continue;
		}
		if ((abs ((int)pCharTmp->dwX - (int)pChar->dwX) > TRADE_PARTYRANGE) ||
			(abs ((int)pCharTmp->dwY - (int)pChar->dwY) > TRADE_PARTYRANGE)) {
			continue;
		}
		/* 目の前の人として、もう入っている？ */
		for (j = 0; j < nCount; j ++) {
			if (padwTblID[j] == pCharTmp->dwTblID) {
				break;
			}
		}
		if (j < nCount) {
			continue;
		}
		padwTblID[nCount ++] = pCharTmp->dwTblID;
	}
	return nCount;
}

/* ========================================================================= */
/* 関数名：MakeWindowCOMMANDTRADE											 */
/* 内容：ウィンドウの作成(取り引きコマンド)									 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
PWNDINFO MakeWindowCOMMANDTRADE(void)
{
	int i, nCount;
	DWORD adwTblID[10];

	DeleteUseCharInfo (&g_aUseCharInfo);

	int nLen = 0;
	nCount = GetTradeTargets (adwTblID, 10);
	for (i = 0; i < nCount; i ++) {
		int nNo = GetCharNo2 (adwTblID[i]);
		if (nNo < 0) {
			continue;
		}
		PCHARINFO pCharTmp = g_aCharInfo[nNo];
		PUSECHARINFO pInfo = new USECHARINFO;
		ZeroMemory (pInfo, sizeof (USECHARINFO));
		pInfo->dwTblID = pCharTmp->dwTblID;
		strcat (pInfo->szName, pCharTmp->szName);
		nLen = max (SboTextWidth (pInfo->szName), nLen);
		g_aUseCharInfo.Add (pInfo);
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
	pWnd->nY = 0;										/* 並べる品は両側20個まで。432ドットの高さが画面(480)に収まるよう一番上に置く */
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
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[i]));
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = min (nCount, LISTROWS_MAX);						/* 10行を超える分はスクロール */
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
/* 関数名：MakeWindowTOWNWARP												 */
/* 内容：ウィンドウの作成(町転送：行き先の選択)								 */
/* ========================================================================= */
PWNDINFO MakeWindowTOWNWARP(int nMode)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	char szTmp[MAX_TOWNWARPNAME + 32];
	int i, nCount = GetTownWarpSelectCount (), nLen = SboTextWidth (GetTownWarpTitle (nMode)), nLenTmp;
	for (i = 0; i < nCount; i ++) {
		GetTownWarpLabel (i, szTmp, sizeof (szTmp));
		nLenTmp = SboTextWidth (szTmp);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}
	pWnd->nData1 = 0;
	pWnd->nData2 = nCount;
	pWnd->nX = 32 + 16 * 4;
	pWnd->nY = 50 + 16 * 2;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCx = min (pWnd->nCx, SCRSIZEX - pWnd->nX);
	pWnd->nCy = 16 * (2 + max (1, pWnd->nData2));
	pWnd->nType = WNDTYPE_TOWNWARP;
	pWnd->nType2 = nMode;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowQUEST													 */
/* 内容：ウィンドウの作成(ギルドの依頼：一覧・進み具合)						 */
/*		 nData1=選んでいる依頼 nData2=一覧の先頭（状態の窓は先頭の行）		 */
/* ========================================================================= */
PWNDINFO MakeWindowQUEST(int nMode)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	pWnd->nData1 = 0;
	pWnd->nData2 = 0;
	pWnd->nX = 32;
	pWnd->nY = 50;
	pWnd->nCx = QUEST_WINCX;
	pWnd->nCy = QUEST_WINCY;
	pWnd->nType = WNDTYPE_QUEST;
	pWnd->nType2 = nMode;
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
	/* 名前の右に、特技ショートカットに登録した番号を出す分(半角2文字)を足す */
	pWnd->nCx = (16 * 4) + (nLen * 8) + 16;
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
		nLenTmp = SboTextWidth (ItemNameOf (pChar->waItem[pChar->waEqu[i]]));
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
	int nType,			/* [in] 種別 */
	BOOL bMerc)			/* [in] 雇った傭兵も含める？(回復・蘇生の特技) */
{
	int nResult, nLen;
	PCHARINFO pMyChar;
	PWNDINFO pWnd = new WNDINFO;
	PUSECHARINFO pInfo;

	SAFE_DELETE (g_pTargetSelectInfo);
	nLen = 0;
	pMyChar = g_aCharInfo[g_nMyCharID];

	nResult = GetUseCharInfo (nArea, pMyChar, bMyChar, nType, bMerc, g_aUseCharInfo, nLen);
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
	} else if ((pWndList->nType == WNDTYPE_MARKET) && (pWndList->nType2 == MARKETWND_LIST)) {
		/* 取引所の一覧は出品の品を出す（byData1 だけスクロールしている） */
		nNo += pWndList->byData1;
		if ((nNo >= 0) && (nNo < (int)g_MarketList.byCount)) {
			nItemID = g_MarketList.aEntry[nNo].wItemID;
		}
	} else if ((pWndList->nType == WNDTYPE_RECYCLE) && (pWndList->nType2 == RECYCLEWND_LIST)) {
		/* リサイクルボックスの「もらう」の一覧は箱の中身の品を出す（byData1 だけスクロールしている） */
		nNo += pWndList->byData1;
		if ((nNo >= 0) && (nNo < (int)g_RecycleList.byKind)) {
			nItemID = g_RecycleList.aEntry[nNo].wItemID;
		}
	} else if ((pWndList->nType == WNDTYPE_RECYCLE) && (pWndList->nType2 == RECYCLEWND_ITEM)) {
		/* 「入れる」の品選びは手持ちをまとめた一覧の品を出す */
		nNo += pWndList->byData1;
		if ((nNo >= 0) && (nNo < g_nRecycleHandCount)) {
			nItemID = g_aRecycleHand[nNo].wItemID;
		}
	} else if ((pWndList->nType == WNDTYPE_LEAVE) && ((g_nProcShopState == LEAVESTATE_ITEMSELECTGET) || (g_nProcShopState == LEAVESTATE_ITEMSORTSWAP) || (g_nProcShopState == LEAVESTATE_ITEMSELECTDISPOSE))) {
		/* 預かり所の引き取る一覧は byData1 だけスクロールしている */
		nNo += pWndList->byData1;
		if (g_pLeaveInfo && (nNo >= 0) && (nNo < g_pLeaveInfo->byCount) && (nNo < MAX_LEAVEITEM)) {
			nItemID = g_pLeaveInfo->waItem[nNo];
		}
	} else if (pWndList->nType == WNDTYPE_IMPORTANT) {
		/* 大事な物の一覧（byData1 だけスクロールしている） */
		nNo += pWndList->byData1;
		if ((g_nMyCharID >= 0) && (g_nMyCharID < g_aCharInfo.GetSize ()) && (nNo >= 0)) {
			nItemID = GetImportantItem (g_nMyCharID, nNo);
		}
	} else {
		/* 自分の持ち物の一覧（byData1 だけスクロールしている） */
		nNo += pWndList->byData1;
		if ((g_nMyCharID >= 0) && (g_nMyCharID < g_aCharInfo.GetSize ()) && (nNo >= 0) && (nNo < MAX_MYITEM)) {
			pChar = g_aCharInfo[g_nMyCharID];
			nItemID = pChar->waItem[nNo];
		}
	}
	if ((nItemID < 0) || (nItemID > 0xFFFF) || !ItemIDValid ((WORD)nItemID)) {
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
	pWnd->nCx		= max (pWnd->nCx, 16 * 22);
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
/* 関数名：ListScrollFitMargin												 */
/* 内容：一覧のカーソルとスクロールを、上下に余白を残して整える				 */
/*       byData1=先頭の番号、nData1=窓内のカーソル行、nData2=窓の行数		 */
/*       カーソルが窓の端から nMargin 行以内に来たら一覧を1行ずらす。		 */
/*       先頭・末尾まで来て、ずらせない時はカーソルだけが動く				 */
/* ========================================================================= */
int ListScrollTop(int nTop, int nIndex, int nCount, int nRows, int nMargin)
{
	/* 窓が小さい時は余白を詰める */
	nRows = max (nRows, 1);
	nMargin = min (nMargin, (nRows - 1) / 2);
	if (nIndex < nTop + nMargin) {
		nTop = nIndex - nMargin;
	}
	if (nIndex > nTop + nRows - 1 - nMargin) {
		nTop = nIndex - (nRows - 1 - nMargin);
	}
	return max (min (nTop, nCount - nRows), 0);
}

void ListScrollFitMargin(PWNDINFO pWndList, int nIndex, int nCount, int nMargin)
{
	int nTop;

	if (nIndex >= nCount) {
		nIndex = nCount - 1;
	}
	if (nIndex < 0) {
		nIndex = 0;
	}
	nTop = ListScrollTop ((int)pWndList->byData1, nIndex, nCount, pWndList->nData2, nMargin);
	pWndList->byData1 = (BYTE)nTop;
	pWndList->nData1 = nIndex - nTop;
}

/* ========================================================================= */
/* 関数名：ListScrollKey													 */
/* 内容：一覧の上下左右キーの処理（余白付きスクロール）					 */
/*       ←で先頭、→で末尾。動かしたら TRUE									 */
/* ========================================================================= */
BOOL ListScrollKey(PWNDINFO pWndList, BYTE byCode, int nCount)
{
	int nIndex = (int)pWndList->byData1 + pWndList->nData1;
	int nIndexNew = nIndex;

	if ((byCode == VK_UP) && (nIndex > 0)) {
		nIndexNew = nIndex - 1;
	} else if ((byCode == VK_DOWN) && (nIndex < nCount - 1)) {
		nIndexNew = nIndex + 1;
	} else if (byCode == VK_LEFT) {
		nIndexNew = 0;
	} else if (byCode == VK_RIGHT) {
		nIndexNew = max (nCount - 1, 0);
	} else {
		return FALSE;
	}
	ListScrollFitMargin (pWndList, nIndexNew, nCount, LISTSCROLL_MARGIN);
	return TRUE;
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

/* ========================================================================= */
/* 関数名：AppraiseGetList													 */
/* 内容：持ち物のうち、未鑑定の物の位置を並び順で取得する（件数を返す）		 */
/* ========================================================================= */
int AppraiseGetList(int *pnList, int nMax)
{
	int i, nCount = 0;

	if ((g_nMyCharID < 0) || (g_aCharInfo[g_nMyCharID] == NULL)) {
		return 0;
	}
	for (i = 0; i < MAX_MYITEM; i ++) {
		WORD wID = g_aCharInfo[g_nMyCharID]->waItem[i];
		if ((wID == 0) || !ItemIDValid (wID) || !ItemIsUnknown (wID)) {
			continue;
		}
		if (nCount >= nMax) {
			break;
		}
		pnList[nCount] = i;
		nCount ++;
	}
	return nCount;
}

/* 鑑定の代金（元の値段の 1/10。最低 10 ゴールド） */
DWORD AppraiseCost(WORD wItemID)
{
	DWORD dwGold = g_aItemInfo[wItemID]->dwGold / 10;

	return (dwGold < 10) ? 10 : dwGold;
}

/* ========================================================================= */
/* 関数名：MakeWindowAPPRAISE												 */
/* 内容：ウィンドウの作成(鑑定屋：未鑑定の持ち物の一覧)						 */
/*		 nData1=画面内のカーソル位置 byData1=先頭の番号 nData2=表示行数		 */
/* ========================================================================= */
PWNDINFO MakeWindowAPPRAISE(int nMaker)
{
	int anList[MAX_MYITEM], nCount;
	PWNDINFO pWnd;

	nCount = AppraiseGetList (anList, MAX_MYITEM);

	pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = min (max (nCount, 1), MAKER_LISTROWS);
	pWnd->nX = 16;
	pWnd->nY = 16;
	pWnd->nCx = 16 * 22;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MAKER;
	pWnd->nType2 = MAKERWND_APPRAISE;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));

	g_nProcMakerID = nMaker;
	g_bProcMakerLeaveOK = FALSE;
	g_bProcMakerWait = FALSE;
	return pWnd;
}

/* ========================================================================= */
/* 取引所（商人ギルドの出張所）の受付                                        */
/* 台詞が "@b" のNPCに話しかけると、メニュー（MARKETWND_MENU）が出る。        */
/* 買う・自分の出品の一覧は MARKETWND_LIST、出品する品選びは MARKETWND_ITEM、 */
/* 値段の入力は MARKETWND_PRICE、確認は MARKETWND_CONF。                     */
/* 一覧・結果はサーバーから届く（SboCliRecvCmd.cpp の RecvDataProcMARKET*）。 */
/* 画面の決まり事は docs/market.md                                            */
/* ========================================================================= */

/* 品の名前（種類IDが範囲外でも落ちないように） */
LPCSTR MarketItemName(WORD wItemID)
{
	if ((wItemID == 0) || !ItemIDValid (wItemID)) {
		return "品物";
	}
	return ItemNameOf (wItemID);
}

/* 値段の下限（品の元の売値。サーバーと同じ「価値の 1/4・最低 1」） */
DWORD MarketPriceMin(WORD wItemID)
{
	if ((wItemID == 0) || !ItemIDValid (wItemID)) {
		return 1;
	}
	return max (ItemGoldOf (wItemID) / 4, (DWORD)1);
}

/* 手数料（値段の 5%。切り捨て） */
DWORD MarketFee(DWORD dwPrice)
{
	return (DWORD)((unsigned long long)dwPrice * MARKET_FEE_PERCENT / 100);
}

/* 取引所の窓を探す（種別 nType2 の窓。無ければ NULL） */
PWNDINFO MarketFindWindow(int nType2)
{
	for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
		if ((g_WndInfo[i]->nType == WNDTYPE_MARKET) && (g_WndInfo[i]->nType2 == nType2)) {
			return g_WndInfo[i];
		}
	}
	return NULL;
}

/* 取引所の窓が開いているか */
BOOL MarketIsOpen(void)
{
	for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
		if (g_WndInfo[i]->nType == WNDTYPE_MARKET) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 取引所の窓を全部消す（所持金の窓は残す。台詞・横の詳細も一緒に消える） */
void MarketEraseAll(void)
{
	for (;;) {
		PWNDINFO pWnd = NULL;
		for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
			if (g_WndInfo[i]->nType == WNDTYPE_MARKET) {
				pWnd = g_WndInfo[i];
				break;
			}
		}
		if (pWnd == NULL) {
			break;
		}
		EraseWindow (pWnd);
	}
}

/* 受付を閉じる（窓も所持金の窓も消して、別れの台詞を出す） */
void MarketClose(LPCSTR pszMsg)
{
	MarketEraseAll ();
	int nNo = GetWindowID (g_dwMsgIDGold);
	if (nNo >= 0) {
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
	}
	g_nMarketWait = MARKETWAIT_NONE;
	g_szMarketResult[0] = 0;
	if (pszMsg) {
		PWNDINFO pWndMsg = MakeWindowMSG ((LPSTR)pszMsg, TRUE);
		g_WndInfo.Add (pWndMsg);
		g_dwActiveMsgID = pWndMsg->dwID;
	}
	g_bDraw = TRUE;
}

/* 取引所の窓の台詞を差し替える（台詞は窓の子にする。MakeWindowMSG が前の台詞を消す） */
void MarketSetMsg(PWNDINFO pWndOwner, LPCSTR pszMsg)
{
	PWNDINFO pWndMsg = MakeWindowMSG ((LPSTR)pszMsg, TRUE);

	pWndMsg->pParent = pWndOwner;
	pWndOwner->pChild = pWndMsg;
	g_WndInfo.Add (pWndMsg);
}

/* 全角・半角まざりの文字列を、表示幅 nMaxUnit（半角=1・全角=2）に収まるように切る */
void MarketCutText(char *pszDst, int nDstSize, LPCSTR pszSrc, int nMaxUnit)
{
	int nPos = 0, nUnit = 0, nLen;
	char szOne[8];

	pszDst[0] = 0;
	while (pszSrc[nPos]) {
		nLen = max (SboTextCharLen (&pszSrc[nPos]), 1);
		if ((nLen >= (int)sizeof (szOne)) || (nPos + nLen >= nDstSize)) {
			break;
		}
		ZeroMemory (szOne, sizeof (szOne));
		CopyMemory (szOne, &pszSrc[nPos], nLen);
		int nW = SboTextWidth (szOne);
		if (nUnit + nW > nMaxUnit) {
			break;
		}
		nUnit += nW;
		nPos += nLen;
		pszDst[nPos] = 0;
	}
	CopyMemory (pszDst, pszSrc, nPos);
	pszDst[nPos] = 0;
}

/* 一覧の3列目（買う一覧は出品者、自分の出品は残り日数）の文字 */
void MarketThirdText(int nKind, const MARKETENTRY *pEntry, char *pszDst, int nDstSize)
{
	if (nKind == MARKETLIST_BUY) {
		MarketCutText (pszDst, nDstSize, pEntry->szSeller, MAX_CHARNAME_WIDTH);
	} else if (pEntry->byState == MARKETSTATE_EXPIRED) {
		lstrcpyn (pszDst, "期限切れ", nDstSize);
	} else if (pEntry->dwRemain >= 24 * 60 * 60) {
		/* 日数は切り上げ（出したばかりの7日は「残り7日」） */
		wsprintf (pszDst, "残り%d日", (int)((pEntry->dwRemain + 24 * 60 * 60 - 1) / (24 * 60 * 60)));
	} else {
		wsprintf (pszDst, "残り%d時間", (int)max (pEntry->dwRemain / (60 * 60), (DWORD)1));
	}
}

/* 一覧の列の幅を求める（nNameUnit=品名・nPriceChar=値段の字数・nThirdUnit=3列目。半角=1） */
void MarketGetLayout(int nKind, int *pnNameUnit, int *pnPriceChar, int *pnThirdUnit)
{
	/* 一覧ごとに列の幅を決め打ちにする（中身によって枠の幅や列の位置が変わらないように）
	 * 品名=全角12字・値段=「999999G」の7字・3列目=買う一覧はキャラ名、自分の出品は「残り23時間」が入る幅 */
	*pnNameUnit = 24;
	*pnPriceChar = 7;
	*pnThirdUnit = (nKind == MARKETLIST_BUY) ? MAX_CHARNAME_WIDTH : 10;
}

/* ========================================================================= */
/* 関数名：MakeWindowMARKET_MENU											 */
/* 内容：ウィンドウの作成(取引所：受付のメニュー)							 */
/*       nData1=カーソル（買う・出品する・自分の出品・売上を受け取る・やめる）	 */
/* ========================================================================= */
PWNDINFO MakeWindowMARKET_MENU(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 5;
	pWnd->nX = 16 * 19;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 10;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MARKET;
	pWnd->nType2 = MARKETWND_MENU;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMARKET_LIST											 */
/* 内容：ウィンドウの作成(取引所：出品の一覧)								 */
/*       nKind=MARKETLIST_*（dwData に持つ）。一覧は g_MarketList				 */
/*       byData1=先頭の番号 nData1=画面内のカーソル位置 nData2=表示行数		 */
/* ========================================================================= */
PWNDINFO MakeWindowMARKET_LIST(int nKind)
{
	int nNameUnit, nPriceChar, nThirdUnit;
	PWNDINFO pWnd = new WNDINFO;

	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->dwData = (DWORD)nKind;
	pWnd->nData2 = min (max ((int)g_MarketList.byCount, 1), MARKET_LISTROWS);
	MarketGetLayout (nKind, &nNameUnit, &nPriceChar, &nThirdUnit);
	pWnd->nX = 16;
	pWnd->nY = MARKET_LISTY;
	/* 左の枠とカーソル + 品名 + 値段 + 3列目 + 右の枠（画面の幅まで） */
	pWnd->nCx = 32 + (nNameUnit + 1 + nPriceChar + 1 + nThirdUnit) * 8 + 16;
	pWnd->nCx = (pWnd->nCx + 15) / 16 * 16;
	pWnd->nCx = min (pWnd->nCx, SCRSIZEX - 32);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MARKET;
	pWnd->nType2 = MARKETWND_LIST;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMARKET_ITEM											 */
/* 内容：ウィンドウの作成(取引所：出品する品を手持ちから選ぶ)				 */
/*       作りは預かり所の「預ける」の品選びと同じ（10行を超える分はスクロール） */
/* ========================================================================= */
PWNDINFO MakeWindowMARKET_ITEM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0;
	for (i = 0; i < nCount; i ++) {
		nLen = max (nLen, SboTextWidth (ItemNameOf (pChar->waItem[i])));
	}
	if (nLen % 2) {
		nLen ++;
	}
	pWnd->nData2 = min (max (nCount, 1), LISTROWS_MAX);
	pWnd->nX = 16;
	pWnd->nY = MARKET_LISTY;
	pWnd->nCx = (16 * 4) + (nLen * 8);
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MARKET;
	pWnd->nType2 = MARKETWND_ITEM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMARKET_PRICE											 */
/* 内容：ウィンドウの作成(取引所：出品の値段の入力)							 */
/*       預かり所の金額入力と同じ。nData1=値段 byData1=いじっている桁(0〜5)	 */
/* ========================================================================= */
PWNDINFO MakeWindowMARKET_PRICE(DWORD dwPrice)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 16;
	pWnd->nY = 16 * 14;
	pWnd->nCx = (16 * 4) + (8 * 4);
	pWnd->nCy = 16 * 3;
	pWnd->nType = WNDTYPE_MARKET;
	pWnd->nType2 = MARKETWND_PRICE;
	pWnd->nData1 = (int)min (dwPrice, (DWORD)MARKET_PRICEMAX);
	pWnd->byData1 = 5;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowMARKET_CONF											 */
/* 内容：ウィンドウの作成(取引所：「はい／いいえ」)							 */
/*       byData1=MARKETCONF_* nData1=カーソル(0:はい 1:いいえ)				 */
/* ========================================================================= */
PWNDINFO MakeWindowMARKET_CONF(int nKind)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 2;
	pWnd->nX = 16;
	pWnd->nY = MARKET_CONFY;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_MARKET;
	pWnd->nType2 = MARKETWND_CONF;
	pWnd->byData1 = (BYTE)nKind;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
	return pWnd;
}

/* 所持金の窓が無ければ出す */
static void MarketEnsureGold(void)
{
	if (GetWindowID (g_dwMsgIDGold) < 0) {
		g_WndInfo.Add (MakeWindowGOLD ());
	}
}

/* ========================================================================= */
/* 関数名：MarketShowMenu													 */
/* 内容：受付のメニューを出す（pszMsg=台詞。NULL なら「他に御用は？」）		 */
/* ========================================================================= */
void MarketShowMenu(LPCSTR pszMsg)
{
	MarketEraseAll ();
	MarketEnsureGold ();
	PWNDINFO pWnd = MakeWindowMARKET_MENU ();
	g_WndInfo.Add (pWnd);
	MarketSetMsg (pWnd, pszMsg ? pszMsg : "＊｢他に御用は？@2");
	g_dwActiveMsgID = pWnd->dwID;
	g_nMarketWait = MARKETWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：MarketOpen														 */
/* 内容：受付に話しかけた時：所持金の窓とメニューを出す						 */
/* ========================================================================= */
void MarketOpen(void)
{
	g_szMarketResult[0] = 0;
	ZeroMemory (&g_MarketList, sizeof (g_MarketList));
	MarketShowMenu ("＊｢ようこそ、商人ギルドの出張所へ。@1　 どのような御用でしょう？@2");
}

/* ========================================================================= */
/* 関数名：MarketShowList													 */
/* 内容：出品の一覧（g_MarketList）を出す。nCursor=合わせる番号				 */
/*       一覧が空ならメニューへ戻る。g_szMarketResult があれば台詞の頭に付ける	 */
/* ========================================================================= */
void MarketShowList(int nKind, int nCursor)
{
	char szMsg[640], szHead[256];

	/* 直前の結果のセリフ（一度出したら空にする） */
	lstrcpyn (szHead, g_szMarketResult, sizeof (szHead));
	g_szMarketResult[0] = 0;

	if (g_MarketList.byCount == 0) {
		if (nKind == MARKETLIST_BUY) {
			wsprintf (szMsg, "%s%s＊｢いま出品はありません。@1　 他に御用は？@2", szHead, szHead[0] ? "@1" : "");
		} else {
			wsprintf (szMsg, "%s%s＊｢出品している品はありません。@1　 他に御用は？@2", szHead, szHead[0] ? "@1" : "");
		}
		MarketShowMenu (szMsg);
		return;
	}

	MarketEraseAll ();
	MarketEnsureGold ();
	PWNDINFO pWnd = MakeWindowMARKET_LIST (nKind);
	g_WndInfo.Add (pWnd);
	ListScrollFitMargin (pWnd, nCursor, (int)g_MarketList.byCount, LISTSCROLL_MARGIN);
	if (nKind == MARKETLIST_BUY) {
		wsprintf (szMsg, "%s%s＊｢どれをお買い求めですか？@2", szHead, szHead[0] ? "@1" : "");
	} else {
		wsprintf (szMsg, "%s%s＊｢どれを引き取りますか？@1　 （出品中の品も取り下げられます）@2", szHead, szHead[0] ? "@1" : "");
	}
	MarketSetMsg (pWnd, szMsg);
	/* 一覧の横にカーソルの品の詳細を出す（一覧を登録した後で付ける） */
	MakeWindowITEMINFOList (pWnd);
	g_dwActiveMsgID = pWnd->dwID;
	g_nMarketWait = MARKETWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：MarketShowItem													 */
/* 内容：出品する品を手持ちから選ぶ窓を出す（nCursor=合わせる手持ちの位置）	 */
/* ========================================================================= */
void MarketShowItem(int nCursor)
{
	int nCount = GetItemCount (g_nMyCharID);

	if (nCount == 0) {
		MarketShowMenu ("何も持っていません。@1@1＊｢他に御用は？@2");
		return;
	}
	MarketEraseAll ();
	MarketEnsureGold ();
	PWNDINFO pWnd = MakeWindowMARKET_ITEM ();
	g_WndInfo.Add (pWnd);
	ListScrollFitMargin (pWnd, nCursor, nCount, LISTSCROLL_MARGIN);
	MarketSetMsg (pWnd, "＊｢どれを出品しますか？@1　 （装備中の品は出せません）@2");
	MakeWindowITEMINFOList (pWnd);
	g_dwActiveMsgID = pWnd->dwID;
	g_nMarketWait = MARKETWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：MarketShowPrice													 */
/* 内容：出品の値段の入力窓を出す（選んだ品は g_wMarketSelItem・手持ちの位置は	 */
/*       g_nMarketSelNo）。dwPrice=最初の値段（0 なら下限）					 */
/* ========================================================================= */
void MarketShowPrice(DWORD dwPrice)
{
	char szMsg[256];
	DWORD dwMin = MarketPriceMin (g_wMarketSelItem);

	if (dwPrice < dwMin) {
		dwPrice = dwMin;
	}
	MarketEraseAll ();
	MarketEnsureGold ();
	PWNDINFO pWnd = MakeWindowMARKET_PRICE (dwPrice);
	g_WndInfo.Add (pWnd);
	wsprintf (szMsg, "＊｢%sをいくらで出しますか？@1　 %dゴールド以上で、@1　 ↑↓で数字、←→で桁を選びます。@2", MarketItemName (g_wMarketSelItem), (int)dwMin);
	MarketSetMsg (pWnd, szMsg);
	g_dwActiveMsgID = pWnd->dwID;
	g_nMarketWait = MARKETWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：MarketShowConf													 */
/* 内容：「はい／いいえ」を出す。pszMsg=確認の台詞							 */
/*       買う・引き取りは一覧を残してその上に、出品・売上は他の窓を消して出す	 */
/* ========================================================================= */
void MarketShowConf(int nKind, LPCSTR pszMsg)
{
	PWNDINFO pWndOwner = NULL;

	if ((nKind == MARKETCONF_BUY) || (nKind == MARKETCONF_TAKEBACK)) {
		pWndOwner = MarketFindWindow (MARKETWND_LIST);
	}
	if (pWndOwner == NULL) {
		MarketEraseAll ();
		MarketEnsureGold ();
	} else {
		/* 一覧の下の「はい／いいえ」が残っていたら消す */
		PWNDINFO pWndOld = MarketFindWindow (MARKETWND_CONF);
		if (pWndOld) {
			EraseWindow (pWndOld);
		}
	}
	PWNDINFO pWnd = MakeWindowMARKET_CONF (nKind);
	g_WndInfo.Add (pWnd);
	MarketSetMsg ((pWndOwner != NULL) ? pWndOwner : pWnd, pszMsg);
	g_dwActiveMsgID = pWnd->dwID;
	g_nMarketWait = MARKETWAIT_NONE;
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* リサイクルボックス（おすそわけの共有の箱）                                */
/* マップの箱を調べる（Sキー）と、サーバーが箱を開いて中身を送ってくるので、     */
/* 届いたらメニュー                                                          */
/* （RECYCLEWND_MENU）を出す。「もらう」は一覧 RECYCLEWND_LIST（選ぶと 1 個もらう）、*/
/* 「入れる」は手持ちを品ごとにまとめた一覧 RECYCLEWND_ITEM → 個数 RECYCLEWND_COUNT。*/
/* 品を選ぶ画面の作りは取引所・預かり所と同じ（▲▼で続きを示す）。          */
/* 一覧・結果はサーバーから届く（SboCliRecvCmd.cpp の RecvDataProcRECYCLE*）。 */
/* 画面の決まり事は docs/recycle.md                                          */
/* ========================================================================= */

/* リサイクルボックスの窓を探す（種別 nType2 の窓。無ければ NULL） */
PWNDINFO RecycleFindWindow(int nType2)
{
	for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
		if ((g_WndInfo[i]->nType == WNDTYPE_RECYCLE) && (g_WndInfo[i]->nType2 == nType2)) {
			return g_WndInfo[i];
		}
	}
	return NULL;
}

/* リサイクルボックスの窓が開いているか */
BOOL RecycleIsOpen(void)
{
	for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
		if (g_WndInfo[i]->nType == WNDTYPE_RECYCLE) {
			return TRUE;
		}
	}
	return FALSE;
}

/* リサイクルボックスの窓を全部消す（台詞・横の詳細も一緒に消える） */
void RecycleEraseAll(void)
{
	for (;;) {
		PWNDINFO pWnd = NULL;
		for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
			if (g_WndInfo[i]->nType == WNDTYPE_RECYCLE) {
				pWnd = g_WndInfo[i];
				break;
			}
		}
		if (pWnd == NULL) {
			break;
		}
		EraseWindow (pWnd);
	}
}

/* 閉じる（窓を消して、別れの台詞を出す。サーバーへ知らせて、箱を閉じた絵に戻してもらう） */
void RecycleClose(LPCSTR pszMsg)
{
	if (RecycleIsOpen ()) {
		SendCmdRECYCLE (CMD_RECYCLECLOSE, 0, 0);
	}
	RecycleEraseAll ();
	g_nRecycleWait = RECYCLEWAIT_NONE;
	g_szRecycleResult[0] = 0;
	if (pszMsg) {
		PWNDINFO pWndMsg = MakeWindowMSG ((LPSTR)pszMsg, TRUE);
		g_WndInfo.Add (pWndMsg);
		g_dwActiveMsgID = pWndMsg->dwID;
	}
	g_bDraw = TRUE;
}

/* 「今日はあと○個もらえます」（もらえない日は「今日はもうもらえません」） */
void RecycleLeftText(char *pszDst, int nDstSize)
{
	if (g_nRecycleLeft > 0) {
		wsprintf (pszDst, "今日はあと%d個もらえます。", g_nRecycleLeft);
	} else {
		lstrcpyn (pszDst, "今日はもうもらえません。", nDstSize);
	}
}

/* 手持ちを品の種類ごとにまとめる（出てきた順。入れられる数は、装備中と取り引き不可を除いた数） */
void RecycleBuildHand(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, j, k;

	g_nRecycleHandCount = 0;
	ZeroMemory (g_aRecycleHand, sizeof (g_aRecycleHand));
	for (i = 0; i < MAX_MYITEM; i ++) {
		WORD wItemID = pChar->waItem[i];
		BOOL bOk;
		if ((wItemID == 0) || !ItemIDValid (wItemID)) {
			continue;
		}
		bOk = !(g_aItemInfo[wItemID]->byFlg & ITEMFLG_TRADE);
		for (j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				bOk = FALSE;
			}
		}
		for (k = 0; k < g_nRecycleHandCount; k ++) {
			if (g_aRecycleHand[k].wItemID == wItemID) {
				break;
			}
		}
		if (k == g_nRecycleHandCount) {
			g_aRecycleHand[k].wItemID = wItemID;
			g_nRecycleHandCount ++;
		}
		g_aRecycleHand[k].byHave ++;
		if (bOk) {
			g_aRecycleHand[k].byOk ++;
		}
	}
}

/* 一覧の幅（左の枠とカーソル + 品名 + 個数 + 右の枠。画面の幅まで） */
static int RecycleListWidth(void)
{
	int nCx = 32 + (RECYCLE_NAMEUNIT + 1 + RECYCLE_COUNTCHAR) * 8 + 16;

	nCx = (nCx + 15) / 16 * 16;
	return min (nCx, SCRSIZEX - 32);
}

/* ========================================================================= */
/* 関数名：MakeWindowRECYCLE_MENU											 */
/* 内容：ウィンドウの作成(リサイクルボックス：メニュー)						 */
/*       nData1=カーソル（入れる・もらう・やめる）							 */
/* ========================================================================= */
PWNDINFO MakeWindowRECYCLE_MENU(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = 3;
	pWnd->nX = 16 * 19;
	pWnd->nY = 16 * 8;
	pWnd->nCx = 16 * 6;
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_RECYCLE;
	pWnd->nType2 = RECYCLEWND_MENU;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowRECYCLE_LIST											 */
/* 内容：ウィンドウの作成(リサイクルボックス：箱の中身の一覧)				 */
/*       一覧は g_RecycleList。byData1=先頭の番号 nData1=画面内のカーソル位置 */
/*       nData2=表示行数													 */
/* ========================================================================= */
PWNDINFO MakeWindowRECYCLE_LIST(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = min (max ((int)g_RecycleList.byKind, 1), MARKET_LISTROWS);
	pWnd->nX = 16;
	pWnd->nY = MARKET_LISTY;
	pWnd->nCx = RecycleListWidth ();
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_RECYCLE;
	pWnd->nType2 = RECYCLEWND_LIST;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowRECYCLE_ITEM											 */
/* 内容：ウィンドウの作成(リサイクルボックス：入れる品を手持ちから選ぶ)		 */
/*       作りは取引所・預かり所の品選びと同じ（行数を超える分はスクロール）	 */
/*       手持ちは RecycleBuildHand でまとめた g_aRecycleHand				 */
/* ========================================================================= */
PWNDINFO MakeWindowRECYCLE_ITEM(void)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nData2 = min (max (g_nRecycleHandCount, 1), MARKET_LISTROWS);
	pWnd->nX = 16;
	pWnd->nY = MARKET_LISTY;
	pWnd->nCx = RecycleListWidth ();
	pWnd->nCy = 16 * (2 + pWnd->nData2);
	pWnd->nType = WNDTYPE_RECYCLE;
	pWnd->nType2 = RECYCLEWND_ITEM;
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：MakeWindowRECYCLE_COUNT											 */
/* 内容：ウィンドウの作成(リサイクルボックス：入れる個数を選ぶ)				 */
/*       nData1=個数（1〜nMax） nData2=入れられる最大（nMax）				 */
/* ========================================================================= */
PWNDINFO MakeWindowRECYCLE_COUNT(int nMax)
{
	PWNDINFO pWnd = new WNDINFO;
	ZeroMemory (pWnd, sizeof (WNDINFO));
	pWnd->dwID = g_dwNewID;
	g_dwNewID ++;
	pWnd->nX = 16;
	pWnd->nY = 16 * 14;
	pWnd->nCx = 16 * 7;
	pWnd->nCy = 16 * 3;
	pWnd->nType = WNDTYPE_RECYCLE;
	pWnd->nType2 = RECYCLEWND_COUNT;
	pWnd->nData1 = 1;
	pWnd->nData2 = max (nMax, 1);
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (pWnd->nCx, pWnd->nCy);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	return pWnd;
}

/* ========================================================================= */
/* 関数名：RecycleShowMenu													 */
/* 内容：メニューを出す（pszMsg=台詞。NULL なら今日の残りを知らせる台詞）	 */
/* ========================================================================= */
void RecycleShowMenu(LPCSTR pszMsg)
{
	char szMsg[512], szLeft[96];

	RecycleEraseAll ();
	PWNDINFO pWnd = MakeWindowRECYCLE_MENU ();
	g_WndInfo.Add (pWnd);
	if (pszMsg == NULL) {
		RecycleLeftText (szLeft, sizeof (szLeft));
		wsprintf (szMsg, "リサイクルボックスです。@1　 いらない物を入れて、ほしい人が@1　 もらっていきます。%s@2", szLeft);
		pszMsg = szMsg;
	}
	MarketSetMsg (pWnd, pszMsg);
	g_dwActiveMsgID = pWnd->dwID;
	g_nRecycleWait = RECYCLEWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecycleOpen														 */
/* 内容：箱を調べて（Sキー）サーバーが箱を開いた時：窓（メニュー）を開く		 */
/*       中身は知らせと一緒に届いて g_RecycleList に入っている				 */
/* ========================================================================= */
void RecycleOpen(void)
{
	g_szRecycleResult[0] = 0;
	g_nRecycleLeft = (int)g_RecycleList.byLeft;
	RecycleEraseAll ();
	RecycleShowMenu (NULL);
}

/* ========================================================================= */
/* 関数名：RecycleShowList													 */
/* 内容：「もらう」の一覧（g_RecycleList）を出す。nCursor=合わせる番号		 */
/*       箱が空ならメニューへ戻る。g_szRecycleResult があれば台詞の頭に付ける */
/* ========================================================================= */
void RecycleShowList(int nCursor)
{
	char szMsg[640], szHead[256], szLeft[96];

	/* 直前の結果のセリフ（一度出したら空にする） */
	lstrcpyn (szHead, g_szRecycleResult, sizeof (szHead));
	g_szRecycleResult[0] = 0;
	RecycleLeftText (szLeft, sizeof (szLeft));

	if (g_RecycleList.byKind == 0) {
		wsprintf (szMsg, "%s%s箱の中は空っぽです。@2", szHead, szHead[0] ? "@1" : "");
		RecycleShowMenu (szMsg);
		return;
	}

	RecycleEraseAll ();
	PWNDINFO pWnd = MakeWindowRECYCLE_LIST ();
	g_WndInfo.Add (pWnd);
	ListScrollFitMargin (pWnd, nCursor, (int)g_RecycleList.byKind, LISTSCROLL_MARGIN);
	wsprintf (szMsg, "%s%sどれをもらいますか？（1回に1個）@1　 %s@2", szHead, szHead[0] ? "@1" : "", szLeft);
	MarketSetMsg (pWnd, szMsg);
	/* 一覧の横にカーソルの品の詳細を出す（一覧を登録した後で付ける） */
	MakeWindowITEMINFOList (pWnd);
	g_dwActiveMsgID = pWnd->dwID;
	g_nRecycleWait = RECYCLEWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecycleShowItem													 */
/* 内容：入れる品を手持ちから選ぶ窓を出す（nCursor=合わせる一覧の位置）		 */
/* ========================================================================= */
void RecycleShowItem(int nCursor)
{
	RecycleBuildHand ();
	if (g_nRecycleHandCount == 0) {
		RecycleShowMenu ("何も持っていません。@2");
		return;
	}
	RecycleEraseAll ();
	PWNDINFO pWnd = MakeWindowRECYCLE_ITEM ();
	g_WndInfo.Add (pWnd);
	ListScrollFitMargin (pWnd, nCursor, g_nRecycleHandCount, LISTSCROLL_MARGIN);
	MarketSetMsg (pWnd, "どれを入れますか？@1　 （装備中の品・取り引きできない品は入れられません）@2");
	MakeWindowITEMINFOList (pWnd);
	g_dwActiveMsgID = pWnd->dwID;
	g_nRecycleWait = RECYCLEWAIT_NONE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecycleShowCount													 */
/* 内容：入れる個数の選択を出す（選んだ品は g_wRecycleSelItem。入れられる最大は */
/*       手持ちの一覧の byOk）。nCount=最初の個数							 */
/* ========================================================================= */
void RecycleShowCount(int nCount)
{
	char szMsg[256];
	int nMax = 1;

	if ((g_nRecycleSelNo >= 0) && (g_nRecycleSelNo < g_nRecycleHandCount)) {
		nMax = max ((int)g_aRecycleHand[g_nRecycleSelNo].byOk, 1);
	}
	RecycleEraseAll ();
	PWNDINFO pWnd = MakeWindowRECYCLE_COUNT (nMax);
	g_WndInfo.Add (pWnd);
	pWnd->nData1 = min (max (nCount, 1), nMax);
	wsprintf (szMsg, "%sをいくつ入れますか？@1　 ↑↓で数を選びます。（最大%d個）@2", MarketItemName (g_wRecycleSelItem), nMax);
	MarketSetMsg (pWnd, szMsg);
	g_dwActiveMsgID = pWnd->dwID;
	g_nRecycleWait = RECYCLEWAIT_NONE;
	g_bDraw = TRUE;
}
