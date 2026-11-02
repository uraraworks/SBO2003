/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			ウィンドウ表示											 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../Common/Info/InfoJob.h"
#include "Glbldata.h"
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
void GetSystemGrpRect							(BYTE byType, PRECT pRect);			/* システムグラフィックの座標を取得 */
void GetSystemGrpRectStatus						(BYTE byType, PRECT pRect);			/* システムグラフィックの座標を取得（ステータス） */
void DrawWindow									(CImg32 *);							/* ウィンドウの描画 */
void DrawWindowFrame							(PWNDINFO);							/* ウィンドウ枠の描画 */
void DrawWindowFrame2							(PWNDINFO, int, int, int, int);		/* ウィンドウ枠の描画 */
void DrawCursor									(PWNDINFO, int, int);				/* カーソルの描画 */
/* ウィンドウの描画 */
void DrawWindowMSG3SEC							(PWNDINFO);			/* ３秒メッセージ */
void DrawWindowLOGIN							(PWNDINFO);			/* ログイン情報 */
void DrawWindowLOGINMENU						(PWNDINFO);			/* ログインメニュー */
void DrawWindowLOGINMENUCHARINFO				(PWNDINFO);			/* ログインメニューのキャラ情報 */
void DrawWindowLOGINMENUCHARMAKE				(PWNDINFO);			/* ログインメニューのキャラ作成 */
void DrawWindowCOMMAND							(PWNDINFO);			/* コマンド */
void DrawWindowCOMMANDETC						(PWNDINFO);			/* その他コマンド */
void DrawWindowCOMMANDITEM						(PWNDINFO);			/* 道具 */
void DrawWindowCOMMANDITEMBATTLE				(PWNDINFO);			/* 道具(戦闘マップ) */
void DrawWindowCOMMANDITEMPROC					(PWNDINFO);			/* 道具処理選択 */
void DrawWindowCOMMANDITEMCHAR					(PWNDINFO);			/* 道具使う相手選択 */
void DrawWindowCOMMANDITEMCHARBATTLE			(PWNDINFO);			/* 道具使う相手選択(戦闘マップ) */
void DrawWindowCOMMANDOPTION					(PWNDINFO);			/* オプション */
void DrawWindowCOMMANDOPTIONVOLSOUND			(PWNDINFO);			/* 効果音音量設定 */
void DrawWindowCOMMANDOPTIONVOLMUSIC			(PWNDINFO);			/* 曲音量設定 */
void DrawWindowCOMMANDEQU						(PWNDINFO);			/* 装備 */
void DrawWindowCOMMANDEQU2						(PWNDINFO);			/* 装備 */
void DrawWindowCOMMANDEQUSUB					(PWNDINFO);			/* 装備(種類別) */
void DrawWindowCOMMANDEQUSELECT					(PWNDINFO);			/* 装備(選択) */
void DrawWindowCOMMANDPARTY						(PWNDINFO);			/* 仲間ウィンドウ */
void DrawWindowCOMMANDPARTYCHAR					(PWNDINFO);			/* 誘う相手選択 */
void DrawWindowSPEECH							(PWNDINFO);			/* 発言入力ウィンドウ */
void DrawWindowMSG								(PWNDINFO);			/* メッセージ */
void DrawWindowMALEFAMALE						(PWNDINFO);			/* 男女 */
void DrawWindowDELETECONFIRM					(PWNDINFO);			/* 削除確認 */
void DrawWindowNAMECOLOR						(PWNDINFO);			/* 名前の色設定 */
void DrawWindowSHOP								(PWNDINFO);			/* 店 */
void DrawWindowSHOP_BUY							(PWNDINFO);			/* 店:売り物一覧 */
void DrawWindowSHOP_SELL						(PWNDINFO);			/* 店:売る物一覧 */
void DrawWindowSHOP_CONF						(PWNDINFO);			/* 店:確認 */
void DrawWindowSPEECHEDIT						(PWNDINFO);			/* 自己紹介設定ウィンドウ */
void DrawWindowCOMMANDSTATUS					(PWNDINFO);			/* 強さウィンドウ */
void DrawWindowLEAVE							(PWNDINFO);			/* 預かり所 */
void DrawWindowLEAVE_SELECT						(PWNDINFO);			/* 預かり所：選択 */
void DrawWindowLEAVE_ITEM						(PWNDINFO);			/* 預かり所：アイテム選択 */
void DrawWindowLEAVE_ITEMGET					(PWNDINFO);			/* 預かり所：引き取るアイテム選択 */
void DrawWindowLEAVE_GOLD						(PWNDINFO);			/* 預かり所：預ける金額設定 */
void DrawWindowCOMMANDTRADE						(PWNDINFO);			/* 取り引きコマンド */
void DrawWindowTRADE							(PWNDINFO);			/* 取り引き */
void DrawWindowTRADEITEM						(PWNDINFO);			/* 取り引き：アイテム選択 */
void DrawWindowTRADEGOLD						(PWNDINFO);			/* 取り引き：金額設定 */
void DrawWindowGOLD								(PWNDINFO);			/* 所持金表示 */
void DrawWindowSUBGAME_BOMBINFO					(PWNDINFO);			/* サブゲーム：爆弾ゲーム情報 */
void DrawWindowSUBGAME_BOMBSET					(PWNDINFO);			/* サブゲーム：爆弾ゲーム設定 */
void DrawWindowCHGPASSWORD						(PWNDINFO);			/* パスワードの変更 */
void DrawWindowCONF								(PWNDINFO);			/* 確認 */
void DrawWindowJOB								(PWNDINFO);			/* 冒険者登録所 */
void DrawWindowJOB_JOBSELECT					(PWNDINFO);			/* 冒険者登録所：職業の選択 */
void DrawWindowCOMMANDMAGIC						(PWNDINFO);			/* 魔法 */
void DrawWindowCOMMANDMAGICBATTLE				(PWNDINFO);			/* 魔法(戦闘マップ) */
void DrawWindowSUBWAR_SELECTTEAM				(PWNDINFO);			/* 戦場ゲーム：チーム選択 */
void DrawWindowSUBWAR_WAIT						(PWNDINFO);			/* 戦場ゲーム：他のプレイヤー待ち */
void DrawWindowSUBWAR_SELECTRULE				(PWNDINFO);			/* 戦場ゲーム：ルール選択 */
void DrawWindowSUBWAR_SELECTJOB					(PWNDINFO);			/* 戦場ゲーム：職業選択 */
void DrawWindowSUBWAR_SELECTTURN				(PWNDINFO);			/* 戦場ゲーム：行動順序 */
void DrawWindowSUBWAR_TURN						(PWNDINFO);			/* 戦場ゲーム：行動選択 */
void DrawWindowSUBWAR_TURN_MOVE					(PWNDINFO);			/* 戦場ゲーム：行動(移動) */
void DrawWindowSUBWAR_TURN_ATACK				(PWNDINFO);			/* 戦場ゲーム：行動(攻撃) */
void DrawWindowSUBWAR_TURN_DIFENCE_FIGHTER		(PWNDINFO);			/* 戦場ゲーム：行動(防御：戦士) */
void DrawWindowSUBWAR_TURN_DIFENCE_MARTIAL		(PWNDINFO);			/* 戦場ゲーム：行動(防御：武道家) */
void DrawWindowSUBWAR_TURN_DIFENCE_WITCH		(PWNDINFO);			/* 戦場ゲーム：行動(防御：魔法使い) */
void DrawWindowSUBWAR_TURN_DIFENCE_PRIEST		(PWNDINFO);			/* 戦場ゲーム：行動(防御：僧侶) */
void DrawWindowSUBWAR_TURN_DIFENCE_WITCH2		(PWNDINFO);			/* 戦場ゲーム：行動(魔法防御：魔法使い) */
void DrawWindowSUBWAR_TURN_MAGIC_WITCH			(PWNDINFO);			/* 戦場ゲーム：行動(魔法：魔法使い) */
void DrawWindowSUBWAR_TURN_MAGIC_TARGET			(PWNDINFO);			/* 戦場ゲーム：行動(魔法対象者選択) */
void DrawWindowSUBWAR_TURN_MAGIC_PRIEST			(PWNDINFO);			/* 戦場ゲーム：行動(魔法：僧侶) */
void DrawWindowITEMEQU							(PWNDINFO);			/* 装備アイテム */
void DrawWindowSTATUS_ATTR						(PWNDINFO);			/* ステータス表示(属性) */
void DrawWindowBATTLE_COMMAND					(PWNDINFO);			/* 戦闘：コマンド */
void DrawWindowBATTLE_TARGET					(PWNDINFO);			/* 戦闘：ターゲット選択 */
void DrawWindowITEMINFO							(PWNDINFO);			/* アイテム情報 */
void DrawWindowDEAD_COMMAND						(PWNDINFO);			/* 死亡中：コマンド */

void DrawWindowSUBSUGOROKU_JOINMODE				(PWNDINFO);			/* ウィンドウの描画(すごろく：参加モード選択) */
void DrawWindowSUBSUGOROKU_COMMAND				(PWNDINFO);			/* ウィンドウの描画(すごろく：行動選択) */
void DrawWindowSUBSUGOROKU_IDLE					(PWNDINFO);			/* ウィンドウの描画(すごろく：平常状態) */

void EraseWindow(PWNDINFO);											/* ウィンドウの消去 */

extern void TextOut2(HDC, int, int, char *, COLORREF);				/* 黒縁取りで文字描画 */

/* ========================================================================= */
/* 関数名：GetSystemGrpRect													 */
/* 内容：システムグラフィックの座標を取得									 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void GetSystemGrpRect(BYTE byType, PRECT pRect)
{
	int nX = 0,
		nY = 0;

	switch (byType) {
	case ITEMTYPE_WEAPON:		/* 武器 */
		nY = 16 * 6;
		break;
	case ITEMTYPE_ARMOR:		/* 鎧 */
		nX = 16 * 1;
		nY = 16 * 6;
		break;
	case ITEMTYPE_SHILD:		/* 盾 */
		nX = 16 * 2;
		nY = 16 * 6;
		break;
	case ITEMTYPE_HELMET:		/* 兜 */
		nX = 16 * 3;
		nY = 16 * 6;
		break;
	case ITEMTYPE_EQU_HAND:		/* 装備(小手) */
		nX = 16 * 4;
		nY = 16 * 6;
		break;
	case ITEMTYPE_EQU_LEG:		/* 装備(靴) */
		nX = 16 * 5;
		nY = 16 * 6;
		break;
	case ITEMTYPE_EQU_ACC1:		/* 装備(首飾り) */
		nX = 16 * 6;
		nY = 16 * 6;
		break;
	case ITEMTYPE_EQU_ACC2:		/* 装備(指輪1) */
		nX = 16 * 7;
		nY = 16 * 6;
		break;
	case ITEMTYPE_EQU_ACC3:		/* 装備(指輪2) */
		nX = 16 * 8;
		nY = 16 * 6;
		break;
	}
	SetRect (pRect, nX, nY, nX + 15, nY + 15);
}

/* ========================================================================= */
/* 関数名：GetSystemGrpRectStatus											 */
/* 内容：システムグラフィックの座標を取得（ステータス）						 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void GetSystemGrpRectStatus(BYTE byType, PRECT pRect)
{
	int nX = 0,
		nY = 0;

	switch (byType) {
	case STATUSTYPE_PW:			nX = 16 * 5;		nY = 16 * 3;	break;
	case STATUSTYPE_DEX:		nX = 16 * 6;		nY = 16 * 3;	break;
	case STATUSTYPE_ST:			nX = 16 * 7;		nY = 16 * 3;	break;
	case STATUSTYPE_IT:			nX = 16 * 8;		nY = 16 * 3;	break;
	case STATUSTYPE_LK:			nX = 16 * 9;		nY = 16 * 3;	break;
	case STATUSTYPE_MAXHP:		nX = 16 * 10;		nY = 16 * 3;	break;
	case STATUSTYPE_MAXMANA:	nX = 16 * 11;		nY = 16 * 3;	break;
	case STATUSTYPE_AT:			nX = 16 * 12;		nY = 16 * 3;	break;
	case STATUSTYPE_DF:			nX = 16 * 13;		nY = 16 * 3;	break;
	case STATUSTYPE_DFMAGIC:	nX = 16 * 14;		nY = 16 * 3;	break;
	case STATUSTYPE_ATTR_FIRE:	nX = 16 * 10;		nY = 16 * 2;	break;
	case STATUSTYPE_ATTR_WATER:	nX = 16 * 11;		nY = 16 * 2;	break;
	case STATUSTYPE_ATTR_WIND:	nX = 16 * 12;		nY = 16 * 2;	break;
	case STATUSTYPE_ATTR_EARTH:	nX = 16 * 13;		nY = 16 * 2;	break;
	case STATUSTYPE_ATTR_LIGHT:	nX = 16 * 14;		nY = 16 * 2;	break;
	case STATUSTYPE_ATTR_DARK:	nX = 16 * 15;		nY = 16 * 2;	break;
	}
	SetRect (pRect, nX, nY, nX + 15, nY + 15);
}

/* ========================================================================= */
/* 関数名：DrawWindow														 */
/* 内容：ウィンドウの描画													 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void DrawWindow(CImg32 *pDib, int x, int y)
{
	int i, nCount;
	PWNDINFO pWnd;
	if (pDib == NULL) {
		pDib = g_pDibPrimary;
	}

	nCount = g_WndInfo.GetSize ();
	if (!nCount) {
		return;
	}

	for (i = 0; i < nCount; i ++) {
		pWnd = (PWNDINFO)g_WndInfo[i];
		if (pWnd->pDib == NULL) {
			continue;
		}
		switch (pWnd->nType) {
		case WNDTYPE_MSGVIEW:
		case WNDTYPE_MSG3SEC:				DrawWindowMSG3SEC (pWnd);				break;
		case WNDTYPE_CONNECTLOST:			DrawWindowMSG3SEC (pWnd);				break;
		case WNDTYPE_LOGIN:					DrawWindowLOGIN (pWnd);					break;
		case WNDTYPE_LOGINMENU:				DrawWindowLOGINMENU (pWnd);				break;
		case WNDTYPE_LOGINMENUCHARINFO:		DrawWindowLOGINMENUCHARINFO (pWnd);		break;
		case WNDTYPE_LOGINMENUCHARMAKE:		DrawWindowLOGINMENUCHARMAKE (pWnd);		break;
		case WNDTYPE_COMMAND:				DrawWindowCOMMAND (pWnd);				break;
		case WNDTYPE_COMMANDETC:			DrawWindowCOMMANDETC (pWnd);			break;
		case WNDTYPE_COMMANDITEM:
		case WNDTYPE_COMMANDITEMBATTLE:
			DrawWindowCOMMANDITEM (pWnd);
			break;
		case WNDTYPE_COMMANDITEMPROC:		DrawWindowCOMMANDITEMPROC (pWnd);		break;
		case WNDTYPE_COMMANDITEMCHAR:
		case WNDTYPE_COMMANDITEMCHARBATTLE:
			DrawWindowCOMMANDITEMCHAR (pWnd);
			break;
		case WNDTYPE_COMMANDOPTION:			DrawWindowCOMMANDOPTION (pWnd);			break;
		case WNDTYPE_COMMANDOPTIONVOLSOUND:	DrawWindowCOMMANDOPTIONVOLSOUND (pWnd);	break;
		case WNDTYPE_COMMANDOPTIONVOLMUSIC:	DrawWindowCOMMANDOPTIONVOLMUSIC (pWnd);	break;
		case WNDTYPE_COMMANDSTATUS:			DrawWindowCOMMANDSTATUS (pWnd);			break;
		case WNDTYPE_SPEECH:				DrawWindowSPEECH (pWnd);				break;
		case WNDTYPE_MSG:					DrawWindowMSG (pWnd);					break;
		case WNDTYPE_MALEFAMALE:			DrawWindowMALEFAMALE (pWnd);			break;
		case WNDTYPE_DELETECONFIRM:			DrawWindowDELETECONFIRM (pWnd);			break;
		case WNDTYPE_NAMECOLOR:				DrawWindowNAMECOLOR (pWnd);				break;
		case WNDTYPE_SHOP:
		case WNDTYPE_SHOPSELL:				DrawWindowSHOP (pWnd);					break;
		case WNDTYPE_SPEECHEDIT:			DrawWindowSPEECHEDIT (pWnd);			break;
		case WNDTYPE_COMMANDETC_CONF:		DrawWindowSHOP_CONF (pWnd);				break;
		case WNDTYPE_COMMANDEQU:			DrawWindowCOMMANDEQU2 (pWnd);			break;
		case WNDTYPE_COMMANDEQUSUB:			DrawWindowCOMMANDEQUSUB (pWnd);			break;
		case WNDTYPE_COMMANDEQUSELECT:		DrawWindowCOMMANDEQUSELECT (pWnd);		break;
		case WNDTYPE_LEAVE:					DrawWindowLEAVE (pWnd);					break;
		case WNDTYPE_COMMANDTRADE:			DrawWindowCOMMANDTRADE (pWnd);			break;
		case WNDTYPE_TRADE:					DrawWindowTRADE (pWnd);					break;
		case WNDTYPE_TRADEITEM:				DrawWindowTRADEITEM (pWnd);				break;
		case WNDTYPE_TRADEGOLD:				DrawWindowTRADEGOLD (pWnd);				break;
		case WNDTYPE_GOLD:					DrawWindowGOLD (pWnd);					break;
		case WNDTYPE_SUBGAME_BOMBINFO:		DrawWindowSUBGAME_BOMBINFO (pWnd);		break;
		case WNDTYPE_SUBGAME_BOMBSET:		DrawWindowSUBGAME_BOMBSET (pWnd);		break;
		case WNDTYPE_CHGPASSWORD:			DrawWindowCHGPASSWORD(pWnd);			break;
		case WNDTYPE_COMMANDPARTY:			DrawWindowCOMMANDPARTY(pWnd);			break;
		case WNDTYPE_COMMANDPARTYCHAR:		DrawWindowCOMMANDPARTYCHAR (pWnd);		break;
		case WNDTYPE_CONF:					DrawWindowCONF (pWnd);					break;
		case WNDTYPE_JOB:					DrawWindowJOB (pWnd);					break;
		case WNDTYPE_COMMANDMAGIC:			DrawWindowCOMMANDMAGIC (pWnd);			break;
		case WNDTYPE_COMMANDMAGICBATTLE:	DrawWindowCOMMANDMAGICBATTLE (pWnd);	break;
		case WNDTYPE_SUBWAR_SELECTTEAM:		DrawWindowSUBWAR_SELECTTEAM (pWnd);		break;
		case WNDTYPE_SUBWAR_WAIT:			DrawWindowSUBWAR_WAIT (pWnd);			break;
		case WNDTYPE_SUBWAR_SELECTRULE:		DrawWindowSUBWAR_SELECTRULE (pWnd);		break;
		case WNDTYPE_SUBWAR_SELECTJOB:		DrawWindowSUBWAR_SELECTJOB (pWnd);		break;
		case WNDTYPE_SUBWAR_SELECTTURN:		DrawWindowSUBWAR_SELECTTURN (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN:			DrawWindowSUBWAR_TURN (pWnd);			break;
		case WNDTYPE_SUBWAR_TURN_MOVE:		DrawWindowSUBWAR_TURN_MOVE (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_ATACK:		DrawWindowSUBWAR_TURN_ATACK (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_DIFENCE_FIGHTER:		DrawWindowSUBWAR_TURN_DIFENCE_FIGHTER (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_DIFENCE_MARTIAL:		DrawWindowSUBWAR_TURN_DIFENCE_MARTIAL (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH:			DrawWindowSUBWAR_TURN_DIFENCE_WITCH (pWnd);			break;
		case WNDTYPE_SUBWAR_TURN_DIFENCE_PRIEST:		DrawWindowSUBWAR_TURN_DIFENCE_PRIEST (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH2:		DrawWindowSUBWAR_TURN_DIFENCE_WITCH2 (pWnd);		break;
		case WNDTYPE_SUBWAR_TURN_MAGIC_WITCH:			DrawWindowSUBWAR_TURN_MAGIC_WITCH (pWnd);			break;
		case WNDTYPE_SUBWAR_TURN_MAGIC_TARGET:			DrawWindowSUBWAR_TURN_MAGIC_TARGET (pWnd);			break;
		case WNDTYPE_SUBWAR_TURN_MAGIC_PRIEST:			DrawWindowSUBWAR_TURN_MAGIC_PRIEST (pWnd);			break;
		case WNDTYPE_ITEMEQU:				DrawWindowITEMEQU (pWnd);				break;
		case WNDTYPE_STATUS_ATTR:			DrawWindowSTATUS_ATTR (pWnd);			break;
		case WNDTYPE_BATTLE_COMMAND:		DrawWindowBATTLE_COMMAND (pWnd);		break;
		case WNDTYPE_BATTLE_TARGET:			DrawWindowBATTLE_TARGET (pWnd);			break;
		case WNDTYPE_SUBSUGOROKU_JOINMODE:	DrawWindowSUBSUGOROKU_JOINMODE (pWnd);	break;
		case WNDTYPE_SUBSUGOROKU_COMMAND:	DrawWindowSUBSUGOROKU_COMMAND (pWnd);	break;
		case WNDTYPE_SUBSUGOROKU_IDLE:		DrawWindowSUBSUGOROKU_IDLE (pWnd);		break;
		case WNDTYPE_ITEMINFO:				DrawWindowITEMINFO (pWnd);				break;
		case WNDTYPE_DEAD_COMMAND:			DrawWindowDEAD_COMMAND (pWnd);			break;
		}
		pDib->BltAlpha2 (pWnd->nX + x, pWnd->nY + y, pWnd->nCx, pWnd->nCy, pWnd->pDib, 0, 0, 15, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowFrame													 */
/* 内容：ウィンドウ枠の描画													 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void DrawWindowFrame(PWNDINFO pWnd)
{
	int i;
	CImg32 *pDib = pWnd->pDib;
	if (!pDib) {
		return;
	}

	/* 背景塗りつぶし */
	pDib->FillRect (0, 0, pWnd->nCx, pWnd->nCy, RGB (40, 40, 40));	

	/* 四隅 */
	pDib->Blt (0, 0, 16, 16, g_pDibSystem, 0, 0);
	pDib->Blt (pWnd->nCx - 16, 0, 16, 16, g_pDibSystem, 32, 0);
	pDib->Blt (0, pWnd->nCy - 16, 16, 16, g_pDibSystem, 0, 32);
	pDib->Blt (pWnd->nCx - 16, pWnd->nCy - 16, 16, 16, g_pDibSystem, 32, 32);

	/* 横線 */
	for (i = 0; i < pWnd->nCx / 16 - 2; i ++) {
		pDib->Blt ((i + 1) * 16, 0, 16, 16, g_pDibSystem, 16, 0);
		pDib->Blt ((i + 1) * 16, pWnd->nCy - 16, 16, 16, g_pDibSystem, 16, 32);
	}

	/* 縦線 */
	for (i = 0; i < pWnd->nCy / 16 - 2; i ++) {
		pDib->Blt (0, (i + 1) * 16, 16, 16, g_pDibSystem, 0, 16);
		pDib->Blt (pWnd->nCx - 16, (i + 1) * 16, 16, 16, g_pDibSystem, 32, 16);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowFrame2													 */
/* 内容：ウィンドウ枠の描画													 */
/* 日付：2003/09/28															 */
/* ========================================================================= */
void DrawWindowFrame2(PWNDINFO pWnd, int nX, int nY, int nCx, int nCy)
{
	int i;
	CImg32 *pDib = pWnd->pDib;
	if (!pDib) {
		return;
	}

	/* 背景塗りつぶし */
	pDib->FillRect (nX, nY, nCx, nCy, RGB (40, 40, 40));	

	/* 四隅 */
	pDib->Blt (nX, nY, 16, 16, g_pDibSystem, 0, 0);
	pDib->Blt (nX + nCx - 16, nY, 16, 16, g_pDibSystem, 32, 0);
	pDib->Blt (nX, nY + nCy - 16, 16, 16, g_pDibSystem, 0, 32);
	pDib->Blt (nX + nCx - 16, nY + nCy - 16, 16, 16, g_pDibSystem, 32, 32);

	/* 横線 */
	for (i = 0; i < nCx / 16 - 2; i ++) {
		pDib->Blt (nX + (i + 1) * 16, nY, 16, 16, g_pDibSystem, 16, 0);
		pDib->Blt (nX + (i + 1) * 16, nY + nCy - 16, 16, 16, g_pDibSystem, 16, 32);
	}

	/* 縦線 */
	for (i = 0; i < nCy / 16 - 2; i ++) {
		pDib->Blt (nX, nY + (i + 1) * 16, 16, 16, g_pDibSystem, 0, 16);
		pDib->Blt (nX + nCx - 16, nY + (i + 1) * 16, 16, 16, g_pDibSystem, 32, 16);
	}
}

/* ========================================================================= */
/* 関数名：DrawCursor														 */
/* 内容：カーソルの描画														 */
/* 日付：2003/09/28															 */
/* ========================================================================= */
void DrawCursor(PWNDINFO pWnd, int nX, int nY)
{
	int nSx, nSy, nNo;

	nNo = 0;
	if (g_dwActiveMsgID == pWnd->dwID) {
		nNo = g_byCursorAnime;
	}

	nSx = 16 * (9 + nNo);
	nSy = 16 * 8;

	pWnd->pDib->Blt (nX, nY, 16, 16, g_pDibSystem, nSx, nSy, TRUE);
}

/* ========================================================================= */
/* 関数名：DrawWindowMSG3SEC												 */
/* 内容：ウィンドウの描画(３秒メッセージ)									 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void DrawWindowMSG3SEC(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 16, 16, pWnd->pszMsg, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowLOGIN													 */
/* 内容：ウィンドウの描画(ログイン情報)										 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void DrawWindowLOGIN(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		InvalidateRect (g_hWndAcc, NULL, FALSE);
		InvalidateRect (g_hWndPas, NULL, FALSE);
		InvalidateRect (g_hWndCon, NULL, FALSE);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 16, 16, "アカウント名", RGB (255, 255, 255));
	TextOut2 (hDC, 16, 32, "パスワード", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();

	ShowWindow (g_hWndAcc, SW_SHOW);
	ShowWindow (g_hWndPas, SW_SHOW);
	ShowWindow (g_hWndCon, SW_SHOW);
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowLOGINMENU												 */
/* 内容：ウィンドウの描画(ログインメニュー)									 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void DrawWindowLOGINMENU(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	if (g_nMyCharID < 0) {
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	if (!pChar->szName[0]) {
		clText = RGB (128, 128, 128);
	}
	TextOut2 (hDC, 32, 16, "冒険をする", clText);
	TextOut2 (hDC, 32, 48, "冒険の書をけす", clText);

	clText = RGB (255, 255, 255);
	if (pChar->szName[0]) {
		clText = RGB (128, 128, 128);
	}
	TextOut2 (hDC, 32, 32, "冒険の書をつくる", clText);
	pWnd->dwTimeDrawStart = timeGetTime ();

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowLOGINMENUCHARINFO										 */
/* 内容：ウィンドウの描画(ログインメニューのキャラ情報)						 */
/* 日付：2003/07/27															 */
/* ========================================================================= */
void DrawWindowLOGINMENUCHARINFO(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	if (g_nMyCharID < 0) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "ログイン情報", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "ログイン情報", RGB (255, 255, 255));

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->szName[0]) {
		char szTmp[64];
		TextOut2 (hDC, 16, 16, "名前:", RGB (200, 200, 200));
		TextOut2 (hDC, 16 + 8 * 6, 16, pChar->szName, RGB (150, 255, 200));

		TextOut2 (hDC, 16, 32, "ログイン回数:", RGB (200, 200, 200));
		wsprintf (szTmp, "%10ld", pChar->dwLoginCount);
		TextOut2 (hDC, 16 + 8 * 14, 32, szTmp, RGB (150, 255, 200));
		TextOut2 (hDC, 16, 48, "プレイ時間:", RGB (200, 200, 200));
		wsprintf (szTmp, "%04d:%02d",
				pChar->dwPlayTime / 60, pChar->dwPlayTime % 60);
		TextOut2 (hDC, 16 + 8 * 17, 48, szTmp, RGB (150, 255, 200));
		TextOut2 (hDC, 16, 64, "キャラ作成日:", RGB (200, 200, 200));
		wsprintf (szTmp, "%04d/%02d/%02d",
				pChar->wMakeYear, pChar->wMakeMonth, pChar->wMakeDay);
		TextOut2 (hDC, 16 + 8 * 14, 64, szTmp, RGB (150, 255, 200));

	} else {
		TextOut2 (hDC, 16, 16, "未作成です。", RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowLOGINMENUCHARMAKE										 */
/* 内容：ウィンドウの描画(ログインメニューのキャラ作成)						 */
/* 日付：2003/07/28															 */
/* ========================================================================= */
void DrawWindowLOGINMENUCHARMAKE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 16, "名前(半角14字以内)：", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMAND												 */
/* 内容：ウィンドウの描画(コマンド)											 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void DrawWindowCOMMAND(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16 + (pWnd->nData1 % 2) * 64, 16 + pWnd->nData1 / 2 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "コマンド", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "コマンド", RGB (255, 255, 255));
	COLORREF cl;

	TextOut2 (hDC, 16 * 2, 16, "話す",		RGB (255, 255, 255));
	cl = RGB (255, 255, 255);
	if (pChar->dwTimeTurnMagicLV) {
		cl = RGB (128, 128, 128);
	}
	TextOut2 (hDC, 16 * 6, 16, "特技",		cl);
	TextOut2 (hDC, 16 * 2, 32, "強さ",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 6, 32, "道具",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 2, 48, "装備",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 6, 48, "調べる",	RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 2, 64, "仲間",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 6, 64, "取引",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 2, 80, "扉",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 6, 80, "その他",	RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16 + (pWnd->nData1 % 2) * 64, 16 + pWnd->nData1 / 2 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDETC												 */
/* 内容：ウィンドウの描画(その他コマンド)									 */
/* 日付：2003/08/03															 */
/* ========================================================================= */
void DrawWindowCOMMANDETC(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "その他", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "その他", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 * 1, "ログ保存", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 2, "自己紹介メッセージ確認", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 3, "自己紹介メッセージ設定", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 4, "名前の色設定", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 5, "パスワードの変更", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 6, "管理人呼び出し", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 7, "オプション", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDOPTION											 */
/* 内容：ウィンドウの描画(オプションコマンド)								 */
/* 日付：2003/09/07															 */
/* ========================================================================= */
void DrawWindowCOMMANDOPTION(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "オプション", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "オプション", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 * 1, "発言徐々に消えるON/OFF", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 2, "音楽ON/OFF", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 3, "吹き出し効果音ON/OFF", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 4, "吹き出しウィンドウON/OFF", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 5, "CPU活動具合切り替え", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 6, "音量(曲)", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 7, "音量(効果音)", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 8, "天候の描画ON/OFF", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 9, "タイトル画面のスキップ", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDOPTIONVOLSOUND									 */
/* 内容：ウィンドウの描画(効果音音量設定)									 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
void DrawWindowCOMMANDOPTIONVOLSOUND(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "音量(効果音)", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "音量(効果音)", RGB (255, 255, 255));

	TextOut2 (hDC, 16, 16 * 1, "小　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 0, 16 * 1, "０　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 1, 16 * 1, "１　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 2, 16 * 1, "２　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 3, 16 * 1, "３　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 4, 16 * 1, "４　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 5, 16 * 1, "大", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	if (pWnd->byAnime) {
		pWnd->pDib->Blt (50 + pWnd->nData1 * 32, 32, 16, 16, g_pDibSystem, 48, 32, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDOPTIONVOLMUSIC									 */
/* 内容：ウィンドウの描画(曲音量設定)										 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
void DrawWindowCOMMANDOPTIONVOLMUSIC(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "音量(曲)", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "音量(曲)", RGB (255, 255, 255));

	TextOut2 (hDC, 16, 16 * 1, "小　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 0, 16 * 1, "０　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 1, 16 * 1, "１　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 2, 16 * 1, "２　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 3, 16 * 1, "３　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 4, 16 * 1, "４　", RGB (255, 255, 255));
	TextOut2 (hDC, 48 + 32 * 5, 16 * 1, "大", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	if (pWnd->byAnime) {
		pWnd->pDib->Blt (50 + pWnd->nData1 * 32, 32, 16, 16, g_pDibSystem, 48, 32, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowSPEECH													 */
/* 内容：ウィンドウの描画(発言入力ウィンドウ)								 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void DrawWindowSPEECH(PWNDINFO pWnd)
{
	char szTmp[64];

	if (pWnd->dwTimeDrawStart) {
		InvalidateRect (g_hWndSpeech, NULL, FALSE);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	strcpy (szTmp, "チャット");
	switch (g_bySpeechMode) {
	case 1:
		strcat (szTmp, "(画面内放送)");
		break;
	case 2:
		strcat (szTmp, "(マップ内放送)");
		break;
	}
	TextOut2 (hDC, 16, 0, szTmp, RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowMSG													 */
/* 内容：ウィンドウの描画(メッセージ)										 */
/* 日付：2003/06/11															 */
/* ========================================================================= */
void DrawWindowMSG(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	LPSTR pszTmp = (LPSTR)new char [strlen (pWnd->pszMsg) + 1];
	ZeroMemory (pszTmp, strlen (pWnd->pszMsg) + 1);
	char szTmp[3] = {0, 0, 0};
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	int i, x = 16, y = 16, nCount = 0;
	for (i = 0; i < pWnd->nData1; i ++) {
		if (pWnd->pszMsg[i] == '@') {
			switch (pWnd->pszMsg[i + 1]) {
			case '0':		/* 入力待ち */
				i ++;
				x = 16;
				y += 16;
				nCount ++;
				continue;

			case '1':		/* 改行 */
				i ++;
				x = 16;
				y += 16;
				nCount ++;
				if (y > pWnd->nCy - 32) {
					pWnd->pDib->Blt (16, 16, pWnd->nCx - 32, pWnd->nCy - 48, pWnd->pDib, 16, 32, FALSE);
					pWnd->pDib->FillRect (15, 15, pWnd->nCx - 30, 1, RGB (40, 40, 40));
					pWnd->pDib->FillRect (15, 16, 1, pWnd->nCy - 48, RGB (40, 40, 40));
					pWnd->pDib->FillRect (15, pWnd->nCy - 32, pWnd->nCx - 31, 16, RGB (40, 40, 40));
					y = pWnd->nCy - 32;
					nCount = pWnd->nCy / 16 - 3;
				}
				continue;

			case '2':		/* 点滅無しで入力待ち */
				i ++;
				continue;

			case '3':		/* 効果音 */
				i += 3;
				continue;

			case '4':		/* アイテム取得 */
				i += 6;
				continue;

			case '8':		/* 復活座標指定 */
				i += 10;
				continue;

			case '9':		/* キャラ画像記録 */
				i += 6;
				continue;
			}
		}
		szTmp[0] = pWnd->pszMsg[i];
		szTmp[1] = 0;
		if (IsDBCSLeadByte (szTmp[0])) {
			szTmp[1] = pWnd->pszMsg[i + 1];
			i ++;
		}
		if (x > pWnd->nCx - 32) {
			x = 16;
			y += 16;
			nCount ++;
		}
		if (y > pWnd->nCy - 32) {
			pWnd->pDib->Blt (16, 16, pWnd->nCx - 32, pWnd->nCy - 48, pWnd->pDib, 16, 32, FALSE);
			pWnd->pDib->FillRect (15, 15, pWnd->nCx - 30, 1, RGB (40, 40, 40));
			pWnd->pDib->FillRect (15, 16, 1, pWnd->nCy - 48, RGB (40, 40, 40));
			pWnd->pDib->FillRect (15, pWnd->nCy - 32, pWnd->nCx - 31, 16, RGB (40, 40, 40));
			y = pWnd->nCy - 32;
			nCount = pWnd->nCy / 16 - 3;
		}
		TextOut2 (hDC, x, y, szTmp, RGB (255, 255, 255));
		x += 8;
		if (IsDBCSLeadByte (szTmp[0])) {
			x += 8;
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	SAFE_DELETE_ARRAY (pszTmp);

	/* 入力待ち？ */
	if (pWnd->nData2 == 1) {
		if (pWnd->byAnime) {
			pWnd->pDib->Blt (16 * 11, 16 + nCount * 16, 16, 16, g_pDibSystem, 48, 48, TRUE);
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowMALEFAMALE												 */
/* 内容：ウィンドウの描画(男女)												 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
void DrawWindowMALEFAMALE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 16, "おとこ", clText);
	TextOut2 (hDC, 32, 32, "おんな", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowDELETECONFIRM											 */
/* 内容：ウィンドウの描画(削除確認)											 */
/* 日付：2004/01/16															 */
/* ========================================================================= */
void DrawWindowDELETECONFIRM(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "削除？", clText);
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "削除？", clText);
	TextOut2 (hDC, 32, 16, "はい", clText);
	TextOut2 (hDC, 32, 32, "いいえ", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowNAMECOLOR												 */
/* 内容：ウィンドウの描画(名前の色設定)										 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void DrawWindowNAMECOLOR(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, "名前の色設定", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, "名前の色設定", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 * 1, "赤", aColorTbl[0]);
	TextOut2 (hDC, 32, 16 * 2, "黄", aColorTbl[1]);
	TextOut2 (hDC, 32, 16 * 3, "緑", aColorTbl[2]);
	TextOut2 (hDC, 32, 16 * 4, "青緑", aColorTbl[3]);
	TextOut2 (hDC, 32, 16 * 5, "青", aColorTbl[4]);
	TextOut2 (hDC, 32, 16 * 6, "紫", aColorTbl[5]);
	TextOut2 (hDC, 32, 16 * 7, "白", aColorTbl[6]);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDITEM											 */
/* 内容：ウィンドウの描画(道具)												 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
void DrawWindowCOMMANDITEM(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "道具", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "道具", RGB (255, 255, 255));

	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));

		for (int j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				nEquNo = 16 + i * 16;
				pWnd->pDib->Blt (16, nEquNo, 8, 16, g_pDibSystem, 64, 16, TRUE);
			}
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDITEMPROC										 */
/* 内容：ウィンドウの描画(道具処理選択)										 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
void DrawWindowCOMMANDITEMPROC(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "使う", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 32, "捨てる", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDITEMCHAR										 */
/* 内容：ウィンドウの描画(道具使う相手選択)									 */
/* 日付：2003/11/11															 */
/* ========================================================================= */
void DrawWindowCOMMANDITEMCHAR(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "誰に？", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "誰に？", RGB (255, 255, 255));
	int i;
	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		TextOut2 (hDC, 32, 16 + 16 * i, g_aUseCharInfo[i]->szName, RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSHOP													 */
/* 内容：ウィンドウの描画(店)												 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void DrawWindowSHOP(PWNDINFO pWnd)
{
	if (g_bProcShopConfirm) {
		DrawWindowSHOP_CONF (pWnd);
		return;

	} else {
		switch (g_nProcShopState) {
		case SHOPMSG_BUY:	DrawWindowSHOP_BUY (pWnd);		return;
		case SHOPMSG_SELL:	DrawWindowSHOP_SELL (pWnd);		return;
		}
	}

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "買いにきた", clText);
	TextOut2 (hDC, 32, 32, "売りにきた", clText);
	TextOut2 (hDC, 32, 48, "やめる", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSHOP_BUY												 */
/* 内容：ウィンドウの描画(店:売り物一覧)									 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void DrawWindowSHOP_BUY(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	for (int i = 0; i < pShop->byCountItem; i ++) {
		PITEMINFO pItem = g_aItemInfo[pShop->awItem[i]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, clText);

		wsprintf (szTmp, "%6dG", pItem->dwGold);
		TextOut2 (hDC, 32 + 16 * 15, 16 + i * 16, szTmp, clText);
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSHOP_SELL												 */
/* 内容：ウィンドウの描画(店:売る物一覧)									 */
/* 日付：2003/11/22															 */
/* ========================================================================= */
void DrawWindowSHOP_SELL(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));

		for (int j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				nEquNo = 16 + i * 16;
				pWnd->pDib->Blt (16, nEquNo, 8, 16, g_pDibSystem, 64, 16, TRUE);
			}
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSHOP_CONF												 */
/* 内容：ウィンドウの描画(店:確認)											 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void DrawWindowSHOP_CONF(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "はい", clText);
	TextOut2 (hDC, 32, 32, "いいえ", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSPEECHEDIT												 */
/* 内容：ウィンドウの描画(自己紹介設定入力ウィンドウ)						 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
void DrawWindowSPEECHEDIT(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		InvalidateRect (g_hWndSpeech, NULL, FALSE);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "自己紹介メッセージ設定", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "自己紹介メッセージ設定", RGB (255, 255, 255));
	COLORREF clText = RGB (255, 255, 255);

	TextOut2 (hDC, 32, 16, "ESCキーでキャンセル", clText);
	TextOut2 (hDC, 32, 32, "改行は｢@1｣､入力待ちは｢@0｣か｢@2｣", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDSTATUS											 */
/* 内容：ウィンドウの描画(強さ)												 */
/* 日付：2003/09/28															 */
/* ========================================================================= */
void DrawWindowCOMMANDSTATUS(PWNDINFO pWnd)
{
	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PJOBINFO pInfo = NULL;

	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame2 (pWnd, 0, 0, 16 * 10, 16 * 7);
	DrawWindowFrame2 (pWnd, 0, 16 * 7, 16 * 10, 16 * 13);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	if (pChar->byJOB < g_JobInfo.GetCount ()) {
		pInfo = g_JobInfo.GetPtr (pChar->byJOB);
	}

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, pChar->szName, RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 7, "強さ", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, pChar->szName, RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 7, "強さ", RGB (255, 255, 255));

	wsprintf (szTmp, "職業:");
	TextOut2 (hDC, 16, 16, szTmp, RGB (200, 200, 200));
	if (pInfo) {
		TextOut2 (hDC, 16 + 8 * 6, 16, pInfo->pszName, RGB (150, 255, 200));
	}

	TextOut2 (hDC, 16, 16 + 16 * 1, "性別:", RGB (200, 200, 200));
	if (pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE) {
		TextOut2 (hDC, 16 + 8 * 6, 16 + 16 * 1, "女", RGB (150, 255, 200));
	} else {
		TextOut2 (hDC, 16 + 8 * 6, 16 + 16 * 1, "男", RGB (150, 255, 200));
	}
	TextOut2 (hDC, 16, 16 + 16 * 2, "レベル:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byLevel);
	TextOut2 (hDC, 16 + 8 * 8, 16 + 16 * 2, szTmp, RGB (150, 255, 200));
	TextOut2 (hDC, 16, 16 + 16 * 3, "HP    :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wHP);
	TextOut2 (hDC, 16 + 8 * 8, 16 + 16 * 3, szTmp, RGB (150, 255, 200));
	TextOut2 (hDC, 16, 16 + 16 * 4, "Mana  :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wMana);
	TextOut2 (hDC, 16 + 8 * 8, 16 + 16 * 4, szTmp, RGB (150, 255, 200));

	RECT rect;
	int x, y;

	x = 32 + 16 * 0;
	y = 16 + 16 * 7;
	TextOut2 (hDC, x, y, "力      :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byPW);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_PW, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "素早さ  :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byDEX);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_DEX, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "体力    :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byST);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ST, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "賢さ    :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byIT);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_IT, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "運のよさ:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->byLK);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_LK, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "最大HP  :", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wMaxHP);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_MAXHP, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "最大Mana:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wMaxMana);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_MAXMANA, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "攻撃力　:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wAT);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_AT, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "守備力　:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wDF);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_DF, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "魔法防御:", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pChar->wDFMagic);
	TextOut2 (hDC, x + 8 * 10, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_DFMAGIC, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "Exp:", RGB (200, 200, 200));
	wsprintf (szTmp, "%u", pChar->dwExp);
	TextOut2 (hDC, x + 8 * 5, y, szTmp, RGB (150, 255, 200));
//	GetSystemGrpRectStatus (STATUSTYPE_DFMAGIC, &rect);
//	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDEQU												 */
/* 内容：ウィンドウの描画(装備)												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void DrawWindowCOMMANDEQU(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame2 (pWnd, 0, 0, 16 * 12, 16 * 8);
	DrawWindowFrame2 (pWnd, 0, 16 * 8, 16 * 12, 16 * 11);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	int i;
	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szTmp, "　　攻撃力:%3d", pChar->wAT);
	TextOut2 (hDC, 32, 16 + 16 * 0, szTmp, RGB (255, 255, 255));
	wsprintf (szTmp, "　守備力:%3d", pChar->wDF);
	TextOut2 (hDC, 48, 16 + 16 * 3, szTmp, RGB (255, 255, 255));

	PITEMINFO pItem;
	PWNDINFO pWndSub = (PWNDINFO)pWnd->pChild;
	/* 武器 */
	if (pWndSub->byData1 == 0) {
		if (pWndSub->nData1 == pWndSub->nData2 - 1) {
			wsprintf (szTmp, "%3d", pChar->byPW);
		} else {
			pItem = g_aItemInfo[pChar->waItem[GetItemCountTypeNoEqu (g_nMyCharID, ITEMTYPE_WEAPON, pWndSub->nData1)]];
			wsprintf (szTmp, "%3d", pChar->byPW + pItem->byData);
		}
		TextOut2 (hDC, 124 + 2, 16 + 16 * 2, szTmp, RGB (255, 255, 255));

	/* 武器以外 */
	} else {
		int nTmp = pChar->byST / 2, nCount = GetItemCount (g_nMyCharID);

		for (i = 0; i < nCount; i ++) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			BYTE byType;
			switch (pItem->byType) {
			case ITEMTYPE_ARMOR:
			case ITEMTYPE_SHILD:
			case ITEMTYPE_HELMET:
			case ITEMTYPE_EQU_HAND:
			case ITEMTYPE_EQU_LEG:
			case ITEMTYPE_EQU_ACC1:
			case ITEMTYPE_EQU_ACC2:
			case ITEMTYPE_EQU_ACC3:
				break;
			default:
				continue;
			}
			switch (pWndSub->byData1) {
			case 1:	byType = ITEMTYPE_ARMOR;	break;
			case 2:	byType = ITEMTYPE_SHILD;	break;
			case 3:	byType = ITEMTYPE_HELMET;	break;
			case 4:	byType = ITEMTYPE_EQU_HAND;	break;
			case 5:	byType = ITEMTYPE_EQU_LEG;	break;
			case 6:	byType = ITEMTYPE_EQU_ACC1;	break;
			case 7:	byType = ITEMTYPE_EQU_ACC2;	break;
			case 8:	byType = ITEMTYPE_EQU_ACC3;	break;
			default:
				continue;
			}
			if (i == GetItemCountTypeNoEqu (g_nMyCharID, byType, pWndSub->nData1)) {
				nTmp += pItem->byData;
				continue;
			}
			if (byType == pItem->byType) {
				continue;
			}
			for (int j = 0; j < 9; j ++) {
				if (i == pChar->waEqu[j]) {
					if (byType != pItem->byType) {
						nTmp += pItem->byData;
						break;
					}
				}
			}
		}
		wsprintf (szTmp, "%3d", nTmp);
		TextOut2 (hDC, 124 + 2, 16 + 16 * 5, szTmp, RGB (255, 255, 255));
	}

	/* 装備品の表示 */
	for (i = 0; i < 9; i ++) {
		if (pChar->waEqu[i] != 0xFFFF) {
			pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
			wsprintf (szTmp, "%s", pItem->szName);
			TextOut2 (hDC, 32, 16 * (9 + i), szTmp, RGB (255, 255, 255));

			RECT rect;
			GetSystemGrpRect (pItem->byType, &rect);
			pWnd->pDib->Blt (16, 16 * (9 + i), 16, 16, g_pDibSystem,
					rect.left, rect.top, TRUE);
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();

	int nY = 32;
	if (pWndSub->byData1 >= 1) {
		nY = 80;
	}
	pWnd->pDib->Blt (134, nY, 16, 9, g_pDibSystem, 48, 48, TRUE);

	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDEQUSUB											 */
/* 内容：ウィンドウの描画(装備(種類別))										 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void DrawWindowCOMMANDEQUSUB(PWNDINFO pWnd)
{
#if 0
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	char szTmp[64];
	RECT rect;
	int nX = 0;

	switch (pWnd->byData1) {
	case 0:		strcpy (szTmp, "武器");
				GetSystemGrpRect (ITEMTYPE_WEAPON, &rect);
				nX = 16 * 4 + 8;
				break;
	case 1:		strcpy (szTmp, "鎧");
				GetSystemGrpRect (ITEMTYPE_ARMOR, &rect);
				nX = 16 * 5;
				break;
	case 2:		strcpy (szTmp, "盾");
				GetSystemGrpRect (ITEMTYPE_SHILD, &rect);
				nX = 16 * 5;
				break;
	case 3:		strcpy (szTmp, "兜");
				GetSystemGrpRect (ITEMTYPE_HELMET, &rect);
				nX = 16 * 5;
				break;
	case 4:		strcpy (szTmp, "小手");
				GetSystemGrpRect (ITEMTYPE_EQU_HAND, &rect);
				nX = 16 * 4 + 8;
				break;
	case 5:		strcpy (szTmp, "靴");
				GetSystemGrpRect (ITEMTYPE_EQU_LEG, &rect);
				nX = 16 * 5;
				break;
	case 6:		strcpy (szTmp, "首飾り");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC1, &rect);
				nX = 16 * 4;
				break;
	case 7:		strcpy (szTmp, "指輪１");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC2, &rect);
				nX = 16 * 4;
				break;
	case 8:		strcpy (szTmp, "指輪２");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC3, &rect);
				nX = 16 * 4;
				break;
	}
	TextOut2 (hDC, nX + 16, 0, szTmp, RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, nX + 16, 0, szTmp, RGB (255, 255, 255));

	if (nX) {
		pWnd->pDib->Blt (nX, 0, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nY = 16, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		BYTE byType;
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		switch (pWnd->byData1) {
		case 0:	byType = ITEMTYPE_WEAPON;	break;
		case 1:	byType = ITEMTYPE_ARMOR;	break;
		case 2:	byType = ITEMTYPE_SHILD;	break;
		case 3:	byType = ITEMTYPE_HELMET;	break;
		case 4:	byType = ITEMTYPE_EQU_HAND;	break;
		case 5:	byType = ITEMTYPE_EQU_LEG;	break;
		case 6:	byType = ITEMTYPE_EQU_ACC1;	break;
		case 7:	byType = ITEMTYPE_EQU_ACC2;	break;
		case 8:	byType = ITEMTYPE_EQU_ACC3;	break;
		default:
			continue;
		}
		if (pItem->byType != byType) {
			continue;
		}

		/* 性別による使えるかチェック */
		if (CheckItemSexDistinction (pItem, pChar) == FALSE) {
			continue;
		}

		WORD wNo;
		BOOL bEqu;

		bEqu = FALSE;
		switch (pItem->byType) {
		case ITEMTYPE_WEAPON:	/* 武器 */
			{
				PITEMINFO_WEAPON pInfo;

				pInfo = (PITEMINFO_WEAPON)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_ARMOR:	/* 鎧 */
			{
				PITEMINFO_ARMOR pInfo;

				pInfo = (PITEMINFO_ARMOR)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_SHILD:	/* 盾 */
			{
				PITEMINFO_SHILD pInfo;

				pInfo = (PITEMINFO_SHILD)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_HELMET:	/* 兜 */
			{
				PITEMINFO_HELMET pInfo;

				pInfo = (PITEMINFO_HELMET)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
			{
				PITEMINFO_EQU_HAND pInfo;

				pInfo = (PITEMINFO_EQU_HAND)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
			{
				PITEMINFO_EQU_LEG pInfo;

				pInfo = (PITEMINFO_EQU_LEG)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
			{
				PITEMINFO_EQU_ACC1 pInfo;

				pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
			{
				PITEMINFO_EQU_ACC2 pInfo;

				pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
			{
				PITEMINFO_EQU_ACC3 pInfo;

				pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;
		}

		bEqu = CheckEquUserInfo (wNo, pChar->byJOB);
		if (bEqu == FALSE) {
			continue;
		}

		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, nY, szTmp, RGB (255, 255, 255));

		if (pChar->waEqu[pWnd->byData1] == i) {
			pWnd->pDib->Blt (16, nY, 8, 16, g_pDibSystem, 64, 16, TRUE);
			nEquNo = nY;
		}
			
		nY += 16;
	}

	TextOut2 (hDC, 16 * 2, nY, "装備しない", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16 * 2, nY, "装備しない", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	
	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
	pWnd->dwTimeDrawStart = timeGetTime ();
#else
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	char szTmp[128];

	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szTmp, "攻撃力:%d 守備力:%d", pWnd->nData1, pWnd->nData2);
	TextOut2 (hDC, 16, 16, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	
	pWnd->dwTimeDrawStart = timeGetTime ();
#endif
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDEQUSELECT										 */
/* 内容：ウィンドウの描画(装備(選択))										 */
/* 日付：2005/04/12															 */
/* ========================================================================= */
void DrawWindowCOMMANDEQUSELECT(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	char szTmp[64];
	RECT rect;
	int nX = 0;

	switch (pWnd->byData1) {
	case ITEMTYPE_WEAPON:		strcpy (szTmp, "武器");
				GetSystemGrpRect (ITEMTYPE_WEAPON, &rect);
				nX = 16 * 4 + 8;
				break;
	case ITEMTYPE_ARMOR:		strcpy (szTmp, "鎧");
				GetSystemGrpRect (ITEMTYPE_ARMOR, &rect);
				nX = 16 * 5;
				break;
	case ITEMTYPE_SHILD:		strcpy (szTmp, "盾");
				GetSystemGrpRect (ITEMTYPE_SHILD, &rect);
				nX = 16 * 5;
				break;
	case ITEMTYPE_HELMET:		strcpy (szTmp, "兜");
				GetSystemGrpRect (ITEMTYPE_HELMET, &rect);
				nX = 16 * 5;
				break;
	case ITEMTYPE_EQU_HAND:		strcpy (szTmp, "小手");
				GetSystemGrpRect (ITEMTYPE_EQU_HAND, &rect);
				nX = 16 * 4 + 8;
				break;
	case ITEMTYPE_EQU_LEG:		strcpy (szTmp, "靴");
				GetSystemGrpRect (ITEMTYPE_EQU_LEG, &rect);
				nX = 16 * 5;
				break;
	case ITEMTYPE_EQU_ACC1:		strcpy (szTmp, "首飾り");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC1, &rect);
				nX = 16 * 4;
				break;
	case ITEMTYPE_EQU_ACC2:		strcpy (szTmp, "指輪１");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC2, &rect);
				nX = 16 * 4;
				break;
	case ITEMTYPE_EQU_ACC3:		strcpy (szTmp, "指輪２");
				GetSystemGrpRect (ITEMTYPE_EQU_ACC3, &rect);
				nX = 16 * 4;
				break;
	}
	TextOut2 (hDC, nX + 16, 0, szTmp, RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, nX + 16, 0, szTmp, RGB (255, 255, 255));

	if (nX) {
		pWnd->pDib->Blt (nX, 0, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nY = 16, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		if (pItem->byType != pWnd->byData1) {
			continue;
		}

		/* 性別による使えるかチェック */
		if (CheckItemSexDistinction (pItem, pChar) == FALSE) {
			continue;
		}

		WORD wNo;
		BOOL bEqu;

		bEqu = FALSE;
		switch (pItem->byType) {
		case ITEMTYPE_WEAPON:	/* 武器 */
			{
				PITEMINFO_WEAPON pInfo;

				pInfo = (PITEMINFO_WEAPON)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_ARMOR:	/* 鎧 */
			{
				PITEMINFO_ARMOR pInfo;

				pInfo = (PITEMINFO_ARMOR)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_SHILD:	/* 盾 */
			{
				PITEMINFO_SHILD pInfo;

				pInfo = (PITEMINFO_SHILD)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_HELMET:	/* 兜 */
			{
				PITEMINFO_HELMET pInfo;

				pInfo = (PITEMINFO_HELMET)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
			{
				PITEMINFO_EQU_HAND pInfo;

				pInfo = (PITEMINFO_EQU_HAND)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
			{
				PITEMINFO_EQU_LEG pInfo;

				pInfo = (PITEMINFO_EQU_LEG)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
			{
				PITEMINFO_EQU_ACC1 pInfo;

				pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
			{
				PITEMINFO_EQU_ACC2 pInfo;

				pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;

		case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
			{
				PITEMINFO_EQU_ACC3 pInfo;

				pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
				wNo = pInfo->wEqu;
			}
			break;
		}

		bEqu = CheckEquUserInfo (wNo, pChar->byJOB);
		if (bEqu == FALSE) {
			continue;
		}

		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, nY, szTmp, RGB (255, 255, 255));

		BYTE byTmp;
		switch (pWnd->byData1) {
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
		if (pChar->waEqu[byTmp] == i) {
			pWnd->pDib->Blt (16, nY, 8, 16, g_pDibSystem, 64, 16, TRUE);
			nEquNo = nY;
		}
			
		nY += 16;
	}

	TextOut2 (hDC, 16 * 2, nY, "装備しない", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16 * 2, nY, "装備しない", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	
	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDEQU												 */
/* 内容：ウィンドウの描画(装備)												 */
/* 日付：2005/04/11															 */
/* ========================================================================= */
void DrawWindowCOMMANDEQU2(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	int i;
	char szTmp[128], szTmp2[128];
	RECT rect;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PITEMINFO pItem;
	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	for (i = 0; i < 9; i ++) {
		switch (i) {
		case 0:		strcpy (szTmp, "武器　　");
					GetSystemGrpRect (ITEMTYPE_WEAPON, &rect);
					break;
		case 1:		strcpy (szTmp, "鎧　　　");
					GetSystemGrpRect (ITEMTYPE_ARMOR, &rect);
					break;
		case 2:		strcpy (szTmp, "盾　　　");
					GetSystemGrpRect (ITEMTYPE_SHILD, &rect);
					break;
		case 3:		strcpy (szTmp, "兜　　　");
					GetSystemGrpRect (ITEMTYPE_HELMET, &rect);
					break;
		case 4:		strcpy (szTmp, "小手　　");
					GetSystemGrpRect (ITEMTYPE_EQU_HAND, &rect);
					break;
		case 5:		strcpy (szTmp, "靴　　　");
					GetSystemGrpRect (ITEMTYPE_EQU_LEG, &rect);
					break;
		case 6:		strcpy (szTmp, "首飾り　");
					GetSystemGrpRect (ITEMTYPE_EQU_ACC1, &rect);
					break;
		case 7:		strcpy (szTmp, "指輪１　");
					GetSystemGrpRect (ITEMTYPE_EQU_ACC2, &rect);
					break;
		case 8:		strcpy (szTmp, "指輪２　");
					GetSystemGrpRect (ITEMTYPE_EQU_ACC3, &rect);
					break;
		}
		if (pChar->waEqu[i] != 0xFFFF) {
			pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
			wsprintf (szTmp2, "%s%s", szTmp, pItem->szName);
		} else {
			strcpy (szTmp2, szTmp);
		}
		TextOut2 (hDC, 48, 16 + i * 16, szTmp2, RGB (255, 255, 255));
		SetBkMode (hDC, TRANSPARENT);
		TextOut2 (hDC, 48, 16 + i * 16, szTmp2, RGB (255, 255, 255));
		pWnd->pDib->Blt (32, 16 + i * 16, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);
	}


	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDPARTY											 */
/* 内容：ウィンドウの描画(仲間ウィンドウ)									 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void DrawWindowCOMMANDPARTY(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, "仲間", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, "仲間", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 * 1, "仲間に誘う", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 2, "抜ける", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 3, "分配方法", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 4, "解散", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANPARTYCHAR										 */
/* 内容：ウィンドウの描画(誘う相手選択)										 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void DrawWindowCOMMANDPARTYCHAR(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "誰を？", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "誰を？", RGB (255, 255, 255));
	int i;
	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		TextOut2 (hDC, 32, 16 + 16 * i, g_aUseCharInfo[i]->szName, RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	if (pWnd->byAnime) {
		pWnd->pDib->Blt (16, 16 + pWnd->nData1 * 16, 16, 16, g_pDibSystem, 48, 0, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowLEAVE													 */
/* 内容：ウィンドウの描画(預かり所)											 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
void DrawWindowLEAVE(PWNDINFO pWnd)
{
	if (g_bProcShopConfirm) {
		return;

	} else {
		switch (g_nProcShopState) {
		case LEAVESTATE_SELECT:
		case LEAVESTATE_SELECTGET:		DrawWindowLEAVE_SELECT (pWnd);		return;
		case LEAVESTATE_ITEM:			DrawWindowLEAVE_ITEM (pWnd);		return;
		case LEAVESTATE_ITEMSELECTGET:	DrawWindowLEAVE_ITEMGET (pWnd);		return;
		case LEAVESTATE_GOLD:
		case LEAVESTATE_GOLDGET:		DrawWindowLEAVE_GOLD (pWnd);		return;
		}
	}

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "預ける", clText);
	TextOut2 (hDC, 32, 32, "引き取る", clText);
	TextOut2 (hDC, 32, 48, "やめる", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowLEAVE_SELECT											 */
/* 内容：ウィンドウの描画(預かり所：どちらか選択)							 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
void DrawWindowLEAVE_SELECT(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "持ち物", clText);
	TextOut2 (hDC, 32, 32, "お金", clText);
	TextOut2 (hDC, 32, 48, "やめる", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowLEAVE_ITEM												 */
/* 内容：ウィンドウの描画(預かり所：アイテム選択)							 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
void DrawWindowLEAVE_ITEM(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));

		for (int j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				nEquNo = 16 + i * 16;
				pWnd->pDib->Blt (16, nEquNo, 8, 16, g_pDibSystem, 64, 16, TRUE);
			}
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowLEAVE_ITEMGET											 */
/* 内容：ウィンドウの描画(預かり所：引き取るアイテム選択)					 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void DrawWindowLEAVE_ITEMGET(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	char szTmp[128];
	int i, nNo = (int)pWnd->byData1, nLen = 0;
	for (i = 0; i < 10; i ++) {
		if (i + nNo >= g_pLeaveInfo->byCount) {
			break;
		}
		if (g_pLeaveInfo->waItem[i + nNo] >= g_aItemInfo.GetSize ()) {
			break;
		}
		PITEMINFO pItem = g_aItemInfo[g_pLeaveInfo->waItem[i + nNo]];
		if (g_pLeaveInfo->byItemCount[i + nNo] > 1) {
			wsprintf (szTmp, "%s(%d)", pItem->szName, g_pLeaveInfo->byItemCount[i + nNo]);
		} else {
			wsprintf (szTmp, "%s", pItem->szName);
		}
		if ((int)strlen (szTmp) > nLen) {
			nLen = strlen (szTmp);
		}
	}
	nLen += 2;
	if (nLen % 2) {
		nLen ++;
	}
	SAFE_DELETE (pWnd->pDib);
	pWnd->pDib = new CImg32;
	pWnd->pDib->Create (nLen * 8 + 48, 16 * i + 32);
	pWnd->pDib->SetColorKey (RGB (40, 40, 40));
	pWnd->nData2 = i;
	DrawWindowFrame2 (pWnd, 0, 0, nLen * 8 + 48, 16 * i + 32);

	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);
	for (i = 0; i < 10; i ++) {
		if (i + nNo >= g_pLeaveInfo->byCount) {
			break;
		}
		PITEMINFO pItem = g_aItemInfo[g_pLeaveInfo->waItem[i + nNo]];
		if (g_pLeaveInfo->byItemCount[i + nNo] > 1) {
			wsprintf (szTmp, "%s(%d)", pItem->szName, g_pLeaveInfo->byItemCount[i + nNo]);
		} else {
			wsprintf (szTmp, "%s", pItem->szName);
		}
		if ((int)strlen (szTmp) > nLen) {
			nLen = strlen (szTmp);
		}
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowLEAVE_GOLD												 */
/* 内容：ウィンドウの描画(預かり所：預ける金額設定)							 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void DrawWindowLEAVE_GOLD(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	char szTmp[128];
	int nNo = (int)pWnd->byData1, nLen = 0;

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);
	SetBkColor (hDC, RGB (40, 40, 40));

	int nTmp = pWnd->nData1;
	if (nTmp >= 1000000) {
		nTmp -= 1000000;
	}
	wsprintf (szTmp, "%06dG", nTmp);
	if (pWnd->byAnime == 1) {
		szTmp[pWnd->byData1] = '*';
	}
	TextOut2 (hDC, 16, 16, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDTRADE											 */
/* 内容：ウィンドウの描画(取り引きコマンド)									 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void DrawWindowCOMMANDTRADE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "誰と？", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "誰と？", RGB (255, 255, 255));
	int i;
	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		TextOut2 (hDC, 32, 16 + 16 * i, g_aUseCharInfo[i]->szName, RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowTRADE													 */
/* 内容：ウィンドウの描画(取り引き)											 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void DrawWindowTRADE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		int x = 80, y = 32 + pWnd->nData1 * 16;
		if (pWnd->nData1 >= 2) {
			x = 16;
			y += 16;
		}
		DrawCursor (pWnd, x, y);
		return;
	}

	if (g_pTradeInfo == NULL) {
		return;
	}

	char szTmp[128];
	int i, nNo = GetCharNo2 (g_pTradeInfo->dwTblID);
	if (nNo < 0) {
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "取り引き", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "取り引き", RGB (255, 255, 255));

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID],
			  pCharTrade = g_aCharInfo[nNo];

	TextOut2 (hDC, 16, 16, pChar->szName, RGB (255, 255, 255));
	if (g_pTradeInfo->byMode1 == 0) {
		TextOut2 (hDC, 16 + 16 * 5, 32, "設定中", RGB (255, 255, 255));
	} else {
		TextOut2 (hDC, 16 + 16 * 5, 32, "決定", RGB (255, 255, 255));
	}
	TextOut2 (hDC, 16, 32, "状態：", RGB (255, 255, 255));
	TextOut2 (hDC, 16, 48, "お金：", RGB (255, 255, 255));
	TextOut2 (hDC, 16, 64, "持ち物：", RGB (255, 255, 255));
	wsprintf (szTmp, "%06dG", g_pTradeInfo->dwGold1);
	if ((pWnd->byAnime == 0) && (pWnd->nData1 == 1) && (pWnd->byData1 > 0)) {
		szTmp[pWnd->byData1 - 1] = '*';
	}
	TextOut2 (hDC, 16 + 16 * 5, 48, szTmp, RGB (255, 255, 255));
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (g_pTradeInfo->waItem1[i] == 0xFFFF) {
			break;
		}
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[g_pTradeInfo->waItem1[i]]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32, 80 + i * 16, szTmp, RGB (255, 255, 255));
	}
	if (i < MAX_MYITEM) {
		TextOut2 (hDC, 32, 80 + i * 16, "追加", RGB (255, 255, 255));
	}

	if (g_pTradeInfo->byMode2 == 0) {
		TextOut2 (hDC, 16 + 16 * 5 + 16 * 12, 32, "設定中", RGB (255, 255, 255));
	} else {
		TextOut2 (hDC, 16 + 16 * 5 + 16 * 12, 32, "決定", RGB (255, 255, 255));
	}
	TextOut2 (hDC, 16 + 16 * 12, 16, pCharTrade->szName, RGB (255, 255, 255));
	TextOut2 (hDC, 16 + 16 * 12, 32, "状態：", RGB (255, 255, 255));
	TextOut2 (hDC, 16 + 16 * 12, 48, "お金：", RGB (255, 255, 255));
	TextOut2 (hDC, 16 + 16 * 12, 64, "持ち物：", RGB (255, 255, 255));
	wsprintf (szTmp, "%06dG", g_pTradeInfo->dwGold2);
	TextOut2 (hDC, 16 + 16 * 17, 48, szTmp, RGB (255, 255, 255));
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (g_pTradeInfo->waItem2[i] == 0xFFFF) {
			break;
		}
		PITEMINFO pItem = g_aItemInfo[pCharTrade->waItem[g_pTradeInfo->waItem2[i]]];
		wsprintf (szTmp, "%s", pItem->szName);
		TextOut2 (hDC, 32 + 16 * 12, 80 + i * 16, szTmp, RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	{
		int x = 80, y = 32 + pWnd->nData1 * 16;
		if (pWnd->nData1 >= 2) {
			x = 16;
			y += 16;
		}
		DrawCursor (pWnd, x, y);
	}
}

/* ========================================================================= */
/* 関数名：DrawWindowTRADEITEM												 */
/* 内容：ウィンドウの描画(取り引き：アイテム選択)							 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void DrawWindowTRADEITEM(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int i, j, nEquNo = -1, nCount = GetItemCount (g_nMyCharID);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
		wsprintf (szTmp, "%s", pItem->szName);
		COLORREF cl = RGB (255, 255, 255);
		for (j = 0; j < MAX_MYITEM; j ++) {
			if (i == g_pTradeInfo->waItem1[j]) {
				cl = RGB (128, 128, 128);
			}
		}
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, cl);

		for (int j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				nEquNo = 16 + i * 16;
				pWnd->pDib->Blt (16, nEquNo, 8, 16, g_pDibSystem, 64, 16, TRUE);
			}
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowTRADEGOLD												 */
/* 内容：ウィンドウの描画(取り引き：金額設定)								 */
/* 日付：2004/01/03															 */
/* ========================================================================= */
void DrawWindowTRADEGOLD(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	char szTmp[128];

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);
	SetBkColor (hDC, RGB (40, 40, 40));

	int nTmp = pWnd->nData1;
	if (nTmp >= 1000000) {
		nTmp -= 1000000;
	}
	wsprintf (szTmp, "%06dG", nTmp);
	if (pWnd->byAnime == 1) {
		szTmp[pWnd->byData1] = '*';
	}
	TextOut2 (hDC, 16, 16, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowGOLD													 */
/* 内容：ウィンドウの描画(所持金表示)										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void DrawWindowGOLD(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame (pWnd);
	char szTmp[10];
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "所持金", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "所持金", RGB (255, 255, 255));

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	int nTmp = pChar->dwGold;
	if (nTmp >= 1000000) {
		nTmp -= 1000000;
	}
	wsprintf (szTmp, "%6dG", nTmp);
	TextOut2 (hDC, 16, 16, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBGAME_BOMBINFO										 */
/* 内容：ウィンドウの描画(サブゲーム：爆弾ゲーム情報)						 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void DrawWindowSUBGAME_BOMBINFO(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		return;
	}

	/* 情報が無い？ */
	if (g_pSubGameInfoPlay == NULL) {
		return;
	}
	PSUBGAMECLIENT_BOMBINFO pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;

	DrawWindowFrame (pWnd);
	char szTmp[10];
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 16, "マスター:", RGB (255, 255, 255));
	TextOut2 (hDC, 16, 32, "残り時間:", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 16, "マスター:", RGB (255, 255, 255));
	TextOut2 (hDC, 16, 32, "残り時間:", RGB (255, 255, 255));

	PCHARINFO pChar = NULL;
	if (pInfo->dwTblIDGameMaster != -1) {
		int nNo = GetCharNo2 (pInfo->dwTblIDGameMaster);
		if (nNo >= 0) {
			pChar = g_aCharInfo[nNo];
		}
	}
	if (pChar) {
		TextOut2 (hDC, 16 + 16 * 5, 16, pChar->szName, RGB (255, 255, 255));
	}

	if (pInfo->byGameTime == 0) {
		wsprintf (szTmp, "無制限");
	} else if (pInfo->byGameTimeNow == pInfo->byGameTime) {
		wsprintf (szTmp, "開始前");
	} else {
		wsprintf (szTmp, "%d 秒", pInfo->byGameTimeNow);
	}
	TextOut2 (hDC, 16 + 16 * 5, 32, szTmp, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBGAME_BOMBSET										 */
/* 内容：ウィンドウの描画(サブゲーム：爆弾ゲーム設定)						 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void DrawWindowSUBGAME_BOMBSET(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	/* 情報が無い？ */
	if (g_pSubGameInfoPlay == NULL) {
		return;
	}
	PSUBGAMECLIENT_BOMBINFO pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;

	DrawWindowFrame (pWnd);
	char szTmp[10];
	COLORREF cl = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 16 * 0, "爆弾ゲーム設定", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 16 * 0, "爆弾ゲーム設定", RGB (255, 255, 255));
	cl = RGB (255, 255, 255);
	if (pInfo->byFlgGameNow) {
		cl = RGB (128, 128, 128);
	}
	TextOut2 (hDC, 32, 16 * 1, "試合開始", cl);
	TextOut2 (hDC, 32, 16 * 2, "制限時間:", cl);
	if (pInfo->byGameTime == 0) {
		wsprintf (szTmp, "無制限");
	} else {
		wsprintf (szTmp, "%d 秒", pInfo->byGameTime);
	}
	TextOut2 (hDC, 32 + 16 * 7, 16 * 2, szTmp, cl);

	TextOut2 (hDC, 32, 16 * 3, "最大設置数:", cl);
	wsprintf (szTmp, "%2d 個", pInfo->byMaxBomb);
	TextOut2 (hDC, 32 + 16 * 7, 16 * 3, szTmp, cl);

	TextOut2 (hDC, 32, 16 * 4, "最大火力:", cl);
	if (pInfo->byMaxFire == 0) {
		wsprintf (szTmp, "ランダム");
	} else {
		wsprintf (szTmp, "%d", pInfo->byMaxFire);
	}
	TextOut2 (hDC, 32 + 16 * 7, 16 * 4, szTmp, cl);

	TextOut2 (hDC, 32, 16 * 5, "壊れる壁:", cl);
	if (pInfo->byFlgUseBlock) {
		wsprintf (szTmp, "あり");
	} else {
		wsprintf (szTmp, "なし");
	}
	TextOut2 (hDC, 32 + 16 * 7, 16 * 5, szTmp, cl);

	TextOut2 (hDC, 32, 16 * 6, "アイテム:", cl);
	if (pInfo->byFlgUseItem) {
		wsprintf (szTmp, "あり");
	} else {
		wsprintf (szTmp, "なし");
	}
	TextOut2 (hDC, 32 + 16 * 7, 16 * 6, szTmp, cl);

	TextOut2 (hDC, 32, 16 * 7, "途中参加:", cl);
	if (pInfo->byFlgProgress) {
		wsprintf (szTmp, "できない");
	} else {
		wsprintf (szTmp, "できる");
	}
	TextOut2 (hDC, 32 + 16 * 7, 16 * 7, szTmp, cl);

	cl = RGB (128, 128, 128);
	if (pInfo->byFlgGameNow) {
		cl = RGB (255, 255, 255);
	}
	TextOut2 (hDC, 32, 16 * 8, "試合中止", cl);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCHGPASSWORD											 */
/* 内容：ウィンドウの描画(パスワードの変更)									 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
void DrawWindowCHGPASSWORD(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		InvalidateRect (g_hWndSpeech, NULL, FALSE);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "パスワードの変更", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "パスワードの変更", RGB (255, 255, 255));
	COLORREF clText = RGB (255, 255, 255);

	TextOut2 (hDC, 16, 16, "ESCキーでキャンセル", clText);
	TextOut2 (hDC, 16, 32, "最大13文字（半角）", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowCONF													 */
/* 内容：ウィンドウの描画(確認)												 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void DrawWindowCONF(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "はい", clText);
	TextOut2 (hDC, 32, 32, "いいえ", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowJOB													 */
/* 内容：ウィンドウの描画(冒険者登録所)										 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void DrawWindowJOB(PWNDINFO pWnd)
{
	if (g_bProcShopConfirm) {
		return;

	} else {
		switch (g_nProcShopState) {
		case JOBSTATE_JOBSELECT:		DrawWindowJOB_JOBSELECT (pWnd);		return;
		}
	}

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "職業の変更", clText);
	TextOut2 (hDC, 32, 32, "やめる", clText);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowJOB_JOBSELECT											 */
/* 内容：ウィンドウの描画(冒険者登録所：職業の選択)							 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void DrawWindowJOB_JOBSELECT(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	COLORREF clText = RGB (255, 255, 255);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	int i, y, nCount;
	char szTmp[128];

	y = 16;
	nCount = g_JobInfo.GetCount ();
	for (i = 0; i < nCount; i ++) {
		PJOBINFO pInfo;

		pInfo = g_JobInfo.GetPtr (i);
		wsprintf (szTmp, "%s", pInfo->pszName);
		TextOut2 (hDC, 32, y, szTmp, clText);
		y += 16;
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDMAGIC											 */
/* 内容：ウィンドウの描画(魔法)												 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void DrawWindowCOMMANDMAGIC(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "特技", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "特技", RGB (255, 255, 255));

	char szTmp[128];
	int i, nCount = g_aMagicInfoWalk.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[g_aMagicInfoWalk[i]];
		wsprintf (szTmp, "%s", pInfo->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowCOMMANDMAGICBATTLE										 */
/* 内容：ウィンドウの描画(魔法(戦闘マップ))									 */
/* 日付：2004/09/25															 */
/* ========================================================================= */
void DrawWindowCOMMANDMAGICBATTLE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 48, 0, "特技", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 48, 0, "特技", RGB (255, 255, 255));

	char szTmp[128];
	int i, nCount = g_aMagicInfoBattle.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[g_aMagicInfoBattle[i]];
		wsprintf (szTmp, "%s", pInfo->szName);
		TextOut2 (hDC, 32, 16 + i * 16, szTmp, RGB (255, 255, 255));
	}
	pWnd->pDib->Blt (16, 16 + g_byComSelectMagic * 16, 8, 16, g_pDibSystem, 64, 16, TRUE);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_SELECTTEAM										 */
/* 内容：ウィンドウの描画(戦場ゲーム：チーム選択)							 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void DrawWindowSUBWAR_SELECTTEAM(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "チーム選択", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "チーム選択", RGB (255, 255, 255));

	int nCount = g_aMagicInfoWalk.GetSize ();

	TextOut2 (hDC, 32, 16 + 0 * 16, "赤チーム", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "青チーム", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_WAIT											 */
/* 内容：ウィンドウの描画(戦場ゲーム：他のプレイヤー待ち)					 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void DrawWindowSUBWAR_WAIT(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	int nCount = g_aMagicInfoWalk.GetSize ();

	TextOut2 (hDC, 32, 16 + 0 * 16, "戦場ゲームから抜ける", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_SELECTRULE										 */
/* 内容：ウィンドウの描画(戦場ゲーム：ルール選択)							 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void DrawWindowSUBWAR_SELECTRULE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "勝利条件", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "勝利条件", RGB (255, 255, 255));

	int nCount = g_aMagicInfoWalk.GetSize ();

	TextOut2 (hDC, 32, 16 + 0 * 16, "全滅させる", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "城へ到達", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_SELECTJOB										 */
/* 内容：ウィンドウの描画(戦場ゲーム：職業選択)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void DrawWindowSUBWAR_SELECTJOB(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "職業", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "職業", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "戦士", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "武道家", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 2 * 16, "魔法使い", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 3 * 16, "僧侶", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_SELECTTURN										 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動順序)								 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
void DrawWindowSUBWAR_SELECTTURN(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	if (g_pWarSelectTurn == NULL) {
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "行動順序", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "行動順序", RGB (255, 255, 255));

	int i;

	for (i = 0; i < MAX_SUBGAME_WARMEMBER; i ++) {
		TextOut2 (hDC, 32, 16 + i * 16, g_pWarSelectTurn->aszName[i], RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN											 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動選択)								 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}
	if (g_pWarInfo == NULL) {
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "行動選択", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "行動選択", RGB (255, 255, 255));

	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;
	COLORREF cl;

	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	cl = RGB (255, 255, 255);
	if (g_pWarInfo->byMove == 0) {
		cl = RGB (128, 128, 128);
	}
	TextOut2 (hDC, 32, 16 + 0 * 16, "移動", cl);
	TextOut2 (hDC, 32, 16 + 1 * 16, "攻撃", cl);

	cl = RGB (128, 128, 128);
	switch (pSubStatus->byJob) {
	case SUBGAME_WAR_JOB_WITCH:		/* 魔法使い */
	case SUBGAME_WAR_JOB_PRIEST:	/* 僧侶 */
		if (g_pWarInfo->byMove > 0) {
			cl = RGB (255, 255, 255);
		}
		break;
	}

	TextOut2 (hDC, 32, 16 + 2 * 16, "魔法", cl);
	TextOut2 (hDC, 32, 16 + 3 * 16, "待機", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_MOVE										 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(移動))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_MOVE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "移動", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "移動", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "↑", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "↓", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 2 * 16, "←", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 3 * 16, "→", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_ATACK										 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(攻撃))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_ATACK(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "攻撃", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "攻撃", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "↑", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "↓", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 2 * 16, "←", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 3 * 16, "→", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_DIFENCE_FIGHTER							 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(防御：戦士))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_DIFENCE_FIGHTER(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "通常防御", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "見切り", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_DIFENCE_MARTIAL							 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(防御：武道家))					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_DIFENCE_MARTIAL(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "通常防御", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "受け身", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 2 * 16, "カウンター", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_DIFENCE_WITCH								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(防御：魔法使い))					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_DIFENCE_WITCH(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "通常防御", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_DIFENCE_PRIEST								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(防御：僧侶))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_DIFENCE_PRIEST(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "通常防御", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_DIFENCE_WITCH2								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(魔法防御：魔法使い))				 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_DIFENCE_WITCH2(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "防御", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "跳ね返し", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_MAGIC_WITCH								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(魔法：魔法使い))					 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_MAGIC_WITCH(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "魔法", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "魔法", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "炎", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_MAGIC_TARGET								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(魔法対象者選択))					 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_MAGIC_TARGET(PWNDINFO pWnd)
{
	int i;

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "誰に？", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "誰に？", RGB (255, 255, 255));

	for (i = 0; i < g_pWarMagicTarget->byCount; i ++) {
		TextOut2 (hDC, 32, 16 + i * 16, g_pWarMagicTarget->aszName[i], RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBWAR_TURN_MAGIC_PRIEST								 */
/* 内容：ウィンドウの描画(戦場ゲーム：行動(魔法：僧侶))						 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void DrawWindowSUBWAR_TURN_MAGIC_PRIEST(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 16, 0, "魔法", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 16, 0, "魔法", RGB (255, 255, 255));

	TextOut2 (hDC, 32, 16 + 0 * 16, "回復", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 + 1 * 16, "蘇生", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowITEMEQU												 */
/* 内容：ウィンドウの描画(装備アイテム)										 */
/* 日付：2004/09/18															 */
/* ========================================================================= */
void DrawWindowITEMEQU(PWNDINFO pWnd)
{
	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	RECT rect;

	if (pWnd->dwTimeDrawStart) {
		return;
	}

	int i, nCount = GetItemCount (g_nMyCharID), nLen = 0, nLenTmp;
	for (i = 0; i < MAX_EQUTYPE; i ++) {
		if (pChar->waEqu[i] == 0xFFFF) {
			continue;
		}
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
		nLenTmp = strlen (pItem->szName);
		nLen = max (nLen, nLenTmp);
	}
	if (nLen % 2) {
		nLen ++;
	}
	if (nLen < 2) {
		nLen = 4;
	}

	DrawWindowFrame2 (pWnd, 0, 0, 16 * 2 + (nLen * 8) + 32, 16 * 12);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, "装備", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, "装備", RGB (255, 255, 255));

	/* 装備品の表示 */
	PITEMINFO pItem;
	for (i = 0; i < MAX_EQUTYPE; i ++) {
		if (pChar->waEqu[i] != 0xFFFF) {
			pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
			wsprintf (szTmp, "%s", pItem->szName);
			TextOut2 (hDC, 32, 16 * (1 + i), szTmp, RGB (255, 255, 255));

			GetSystemGrpRect (pItem->byType, &rect);
			pWnd->pDib->Blt (16, 16 * (1 + i), 16, 16, g_pDibSystem,
					rect.left, rect.top, TRUE);
		}
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowSTATUS_ATTR											 */
/* 内容：ウィンドウの描画(ステータス表示(属性))								 */
/* 日付：2004/09/19															 */
/* ========================================================================= */
void DrawWindowSTATUS_ATTR(PWNDINFO pWnd)
{
	char szTmp[128];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	RECT rect;

	if (pWnd->dwTimeDrawStart) {
		return;
	}

	DrawWindowFrame2 (pWnd, 0, 0, 16 * 2 + 16 * 18, 16 * 2 + 16 * 2);

	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, "属性", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, "属性", RGB (255, 255, 255));

	int x, y;

	x = 32;
	y = 16;
	TextOut2 (hDC, x, y, "火:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrFire);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_FIRE, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "水:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrWater);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_WATER, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	x += (12 * 8);
	y = 16;
	TextOut2 (hDC, x, y, "風:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrWind);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_WIND, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "土:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrEarth);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_EARTH, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	x += (12 * 8);
	y = 16;
	TextOut2 (hDC, x, y, "光:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrLight);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_LIGHT, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	y += 16;
	TextOut2 (hDC, x, y, "闇:", RGB (200, 200, 200));
	wsprintf (szTmp, "%4d%%", pChar->chAttrDark);
	TextOut2 (hDC, x + 8 * 3, y, szTmp, RGB (150, 255, 200));
	GetSystemGrpRectStatus (STATUSTYPE_ATTR_DARK, &rect);
	pWnd->pDib->Blt (x - 16, y, 16, 16, g_pDibSystem, rect.left, rect.top, TRUE);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：DrawWindowBATTLE_COMMAND											 */
/* 内容：ウィンドウの描画(戦闘：コマンド)									 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
void DrawWindowBATTLE_COMMAND(PWNDINFO pWnd)
{
	HDC hDC;
	HFONT hFontOld;
	PCHARINFO pChar;
	COLORREF cl;

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	hDC			= pWnd->pDib->Lock ();
	hFontOld	= (HFONT)SelectObject (hDC, g_hFontBasic);
	pChar		= g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
//	TextOut2 (hDC, 48, 0, "コマンド", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);

	cl = RGB (255, 255, 255);
//	if (pChar->byTurnLV < 100) {
//		cl = RGB (127, 127, 127);
//	}

	TextOut2 (hDC, 16 * 2, 16, "攻撃",	cl);
	TextOut2 (hDC, 16 * 2, 32, "特技",	cl);
	TextOut2 (hDC, 16 * 2, 48, "道具",	cl);

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowBATTLE_TARGET											 */
/* 内容：ウィンドウの描画(戦闘：ターゲット選択)								 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
void DrawWindowBATTLE_TARGET(PWNDINFO pWnd)
{
	int i, nCount;

	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	TextOut2 (hDC, 32, 0, "対象", RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut2 (hDC, 32, 0, "対象", RGB (255, 255, 255));

	nCount = g_aUseCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		TextOut2 (hDC, 32, 16 + 16 * i, g_aUseCharInfo[i]->szName, RGB (255, 255, 255));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBSUGOROKU_JOINMODE									 */
/* 内容：ウィンドウの描画(すごろく：参加モード選択)							 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void DrawWindowSUBSUGOROKU_JOINMODE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "参加する", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 32, "観戦する", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBSUGOROKU_COMMAND									 */
/* 内容：ウィンドウの描画(すごろく：行動選択)								 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void DrawWindowSUBSUGOROKU_COMMAND(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16 * 1, "サイコロを振る", RGB (255, 255, 255));
	TextOut2 (hDC, 32, 16 * 2, "すごろくから抜ける", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowSUBSUGOROKU_IDLE										 */
/* 内容：ウィンドウの描画(すごろく：平常状態)								 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void DrawWindowSUBSUGOROKU_IDLE(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 32, 16, "すごろくから抜ける", RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}

/* ========================================================================= */
/* 関数名：DrawWindowITEMINFO												 */
/* 内容：ウィンドウの描画(アイテム情報)										 */
/* 日付：2005/03/12															 */
/* ========================================================================= */
void DrawWindowITEMINFO(PWNDINFO pWnd)
{
	PITEMINFO pItem;
	WORD wEqu;
	int nLen;
	char szTmp[256];

	if (pWnd->dwTimeDrawStart) {
		return;
	}

	if (pWnd->nData1 == 0) {
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	pItem = g_aItemInfo[pWnd->nData1];

	SetBkColor (hDC, RGB (40, 40, 40));
	wsprintf (szTmp, "%s", pItem->szName);
	TextOut2 (hDC, 16, 0, szTmp, RGB (255, 255, 255));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 16, 0, szTmp, RGB (255, 255, 255));
	TextOut2 (hDC, 16, 16, "種類：", RGB (200, 200, 200));
	wsprintf (szTmp, "%s", aItemeTypeNameTbl[pItem->byType]);
	TextOut2 (hDC, 16 + 12 * 3, 16, szTmp, RGB (150, 255, 200));

	TextOut2 (hDC, 16, 16 + 12 * 1, "価値：", RGB (200, 200, 200));
	wsprintf (szTmp, "%d", pItem->dwGold);
	TextOut2 (hDC, 16 + 12 * 3, 16 + 12 * 1, szTmp, RGB (150, 255, 200));

	wEqu = 0xFFFF;
	switch (pItem->byType) {
	case ITEMTYPE_WEAPON:		/* 武器 */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_WEAPON)pItem->pData)->wEqu;
			strcpy (szTmp, "攻撃力：");
		}
		break;
	case ITEMTYPE_ARMOR:		/* 鎧 */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_ARMOR)pItem->pData)->wEqu;
			strcpy (szTmp, "守備力：");
		}
		break;
	case ITEMTYPE_SHILD:		/* 盾 */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_SHILD)pItem->pData)->wEqu;
			strcpy (szTmp, "守備力：");
		}
		break;
	case ITEMTYPE_HELMET:		/* 兜 */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_HELMET)pItem->pData)->wEqu;
			strcpy (szTmp, "守備力：");
		}
		break;
	case ITEMTYPE_EQU_HAND:		/* 装備(小手) */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_EQU_HAND)pItem->pData)->wEqu;
			strcpy (szTmp, "守備力：");
		}
		break;
	case ITEMTYPE_EQU_LEG:		/* 装備(靴) */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_EQU_LEG)pItem->pData)->wEqu;
			strcpy (szTmp, "守備力：");
		}
		break;
	case ITEMTYPE_EQU_ACC1:		/* 装備(首飾り) */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_EQU_ACC1)pItem->pData)->wEqu;
			strcpy (szTmp, "効果：");
		}
		break;
	case ITEMTYPE_EQU_ACC2:		/* 装備(指輪1) */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_EQU_ACC2)pItem->pData)->wEqu;
			strcpy (szTmp, "効果：");
		}
		break;
	case ITEMTYPE_EQU_ACC3:		/* 装備(指輪2) */
		if (pItem->pData) {
			wEqu = ((PITEMINFO_EQU_ACC3)pItem->pData)->wEqu;
			strcpy (szTmp, "効果：");
		}
		break;
	}

	if (wEqu != 0xFFFF) {
		nLen = strlen (szTmp) / 2;
		TextOut2 (hDC, 16, 16 + 12 * 2, szTmp, RGB (200, 200, 200));
		wsprintf (szTmp, "%d", pItem->byData);
		TextOut2 (hDC, 16 + 12 * nLen, 16 + 12 * 2, szTmp, RGB (150, 255, 200));
		TextOut2 (hDC, 16, 16 + 12 * 3, "装備：", RGB (200, 200, 200));
		wsprintf (szTmp, "%s", g_aEquUserInfo[wEqu]);
		TextOut2 (hDC, 16 + 12 * 3, 16 + 12 * 3, szTmp, RGB (150, 255, 200));
	}

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();
}


/* ========================================================================= */
/* 関数名：	DrawWindowDEAD_COMMAND											 */
/* 内容：	ウィンドウの描画(死亡中：コマンド)								 */
/* 日付：	2005/11/01														 */
/* ========================================================================= */

void DrawWindowDEAD_COMMAND(PWNDINFO pWnd)
{
	if (pWnd->dwTimeDrawStart) {
		DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
		return;
	}

	DrawWindowFrame (pWnd);
	HDC hDC = pWnd->pDib->Lock ();
	HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	SetBkColor (hDC, RGB (40, 40, 40));
	SetBkMode (hDC, TRANSPARENT);

	TextOut2 (hDC, 16 * 2, 16, "この場で助けを待つ",		RGB (255, 255, 255));
	TextOut2 (hDC, 16 * 2, 32, "記録した場所で復活する",	RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	pWnd->pDib->Unlock ();
	pWnd->dwTimeDrawStart = timeGetTime ();

	DrawCursor (pWnd, 16, 16 + pWnd->nData1 * 16);
}


/* ========================================================================= */
/* 関数名：EraseWindow														 */
/* 内容：ウィンドウの消去													 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void EraseWindow(PWNDINFO pWnd)
{
	int i;

	if (pWnd == NULL) {
		return;
	}

	if (pWnd->pChild) {
		EraseWindow ((PWNDINFO)pWnd->pChild);
	}

	if (pWnd->pWndTmp) {
		EraseWindow ((PWNDINFO)pWnd->pWndTmp);
		pWnd->pWndTmp = NULL;
	}

	switch (pWnd->nType) {
	case WNDTYPE_COMMAND:
	case WNDTYPE_BATTLE_COMMAND:
	case WNDTYPE_SUBGAME_BOMBSET:
	case WNDTYPE_DEAD_COMMAND:
		g_dwMsgIDCommand = 0;
		break;

	case WNDTYPE_MSG:
	case WNDTYPE_MSGVIEW:
	case WNDTYPE_MSG3SEC:
		SAFE_DELETE_ARRAY (pWnd->pszMsg);
		break;

	case WNDTYPE_LOGIN:
		if (g_hWndAcc) {
			SetWindowLong (g_hWndAcc, GWL_WNDPROC, (LONG)g_OrgWndProcAcc);
			DestroyWindow (g_hWndAcc);
			g_hWndAcc = NULL;
		}
		if (g_hWndPas) {
			SetWindowLong (g_hWndPas, GWL_WNDPROC, (LONG)g_OrgWndProcPas);
			DestroyWindow (g_hWndPas);
			g_hWndPas = NULL;
		}
		if (g_hWndCon) {
			SetWindowLong (g_hWndCon, GWL_WNDPROC, (LONG)g_OrgWndProcCon);
			DestroyWindow (g_hWndCon);
			g_hWndCon = NULL;
		}
		SetFocus (g_hWndMain);
		break;

	case WNDTYPE_LOGINMENUCHARMAKE:
		SetWindowLong (g_hWndName, GWL_WNDPROC, (LONG)g_OrgWndProcName);
		DestroyWindow (g_hWndName);
		SetWindowLong (g_hWndMake, GWL_WNDPROC, (LONG)g_OrgWndProcMake);
		DestroyWindow (g_hWndMake);
		break;

	case WNDTYPE_SPEECH:
	case WNDTYPE_SPEECHEDIT:
	case WNDTYPE_CHGPASSWORD:
		if (g_hWndSpeech) {
			/* IMEをオフにする */
			HIMC hImc = ImmGetContext (g_hWndSpeech);
			ImmSetOpenStatus (hImc, FALSE);
			ImmReleaseContext (g_hWndSpeech, hImc);

			SetWindowLong (g_hWndSpeech, GWL_WNDPROC, (LONG)g_OrgWndProcSpeech);
			DestroyWindow (g_hWndSpeech);
			g_hWndSpeech = NULL;
			g_bSpeech = FALSE;
		}
		break;

	case WNDTYPE_COMMANDITEMCHAR:
	case WNDTYPE_COMMANDITEMBATTLE:
	case WNDTYPE_COMMANDTRADE:
	case WNDTYPE_BATTLE_TARGET:
	case WNDTYPE_COMMANDPARTYCHAR:
		for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
			SAFE_DELETE (g_aUseCharInfo[i]);
		}
		g_aUseCharInfo.RemoveAll ();
		break;

	case WNDTYPE_TRADE:
		SAFE_DELETE (g_pTradeInfo);
		g_dwMsgIDTRADE = 0;
		break;

	case WNDTYPE_GOLD:
		g_dwMsgIDGold = 0;
		break;
	}

	/* アクティブウィンドウの設定 */
	g_dwActiveMsgID = 0;
	if (pWnd->pParent) {
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
		pWndParent->pChild = NULL;
		g_dwActiveMsgID = pWndParent->dwID;
	}

	int nCount = g_WndInfo.GetSize ();
	for (i = 0; i < g_WndInfo.GetSize (); i ++) {
		if (g_WndInfo[i]->dwID == pWnd->dwID) {
			g_WndInfo.RemoveAt (i);
			break;
		}
	}
	SAFE_DELETE (pWnd->pDib);
	SAFE_DELETE (pWnd);
	g_nProcMsg = 0;
}


/* ========================================================================= */
/* 関数名：	AllEraseWindow													 */
/* 内容：	全てのウィンドウ消去											 */
/* 日付：	2003/11/02														 */
/* ========================================================================= */

void AllEraseWindow(void)
{
	int nCount;
	PWNDINFO pWnd;

	while (1) {
		nCount = g_WndInfo.GetSize ();
		if (nCount == 0) {
			break;
		}
		pWnd = g_WndInfo[0];
		EraseWindow (pWnd);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
