/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			コマンド送信処理										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

#include "stdafx.h"
#include "common/BowFlight.h"
#include "../Common/Info/InfoJob.h"
#include "../Common/Command/Command.h"
#include "Glbldata.h"
#include "SboCliRecvCmd.h"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* コマンド送信 */
void SendCmdLOGINREQ		(void);									/* ログイン要求 */
void SendCmdWEBACCOUNT		(BYTE, BYTE);								/* ブラウザ版のアカウント関係 */
void SendCmdCHGSTATE		(BYTE);									/* 状態遷移 */
void SendCmdCHGDIRECTION	(BYTE);									/* 向き変更要求 */
void SendCmdCHARPOS			(BYTE, BOOL);							/* 移動要求 */
void SendCmdCHARINFOREQ		(DWORD);								/* キャラ情報要求 */
void SendCmdSPEECH			(LPSTR, BYTE);							/* 発言通知 */
void SendCmdHARTBEAT		(void);									/* 生存確認通知 */
void SendCmdMAPPARTSADD		(void);									/* マップパーツ追加 */
void SendCmdMAPPARTSADDANIME(DWORD);								/* マップパーツアニメ追加 */
void SendCmdCHGMAP			(WORD, int, int);						/* マップ書き換え */
void SendCmdMAPPARTS		(int);									/* マップパーツ更新 */
void SendCmdRELOADPARTS		(int);									/* マップパーツ更新要求 */
void SendCmdADDDELMAP		(BYTE, BYTE);							/* マップ幅増減要求 */
void SendCmdSAVEMAP			(void);									/* マップ保存 */
void SendCmdEVENTADD		(void);									/* イベント情報追加 */
void SendCmdEVENTGET		(int, DWORD, DWORD);					/* イベント情報取得 */
void SendCmdEVENTINFO		(void);									/* イベント情報更新 */
void SendCmdCHGMAPINFO		(void);									/* マップ情報更新 */
BOOL SendCmdEVENTCHECK		(BYTE);									/* イベントチェック要求 */
void SendCmdADDCHAR			(PCMDPARA_ADDCHARINFO);					/* キャラ追加要求 */
void SendCmdBANCHAR			(DWORD);								/* キャラ強制ログオフ要求 */
void SendCmdCHARINFO		(PCMDPARA_CHARINFO);					/* キャラ情報 */
void SendCmdADDMAP			(void);									/* マップ追加 */
void SendCmdADDMSG			(LPSTR);								/* メッセージ追加 */
void SendCmdMAPMSGREQ		(DWORD);								/* メッセージ要求 */
void SendCmdCHGMSG			(LPSTR, DWORD);							/* メッセージ更新 */
void SendCmdOPENDOOR		(void);									/* 扉開け要求 */
void SendCmdCHGCHARGRP		(DWORD);								/* 着せ替え要求 */
void SendCmdCHGCHARGRPBACK	(DWORD);								/* キャラ画像記録 */
void SendCmdNAMESEX			(PCHARINFO);							/* 名前と性別更新 */
void SendCmdGMCALL			(void);									/* 管理人呼び出し */
void SendCmdMAPMOVE			(DWORD, DWORD, DWORD, DWORD, BYTE);		/* マップ移動 */
void SendCmdCHARPUSH		(DWORD);								/* 押して要求 */
void SendCmdCOMEFC			(BYTE);									/* 吹き出し要求 */
void SendCmdADDSCORE		(DWORD);								/* 得点追加要求 */
void SendCmdDELCHAR			(DWORD);								/* キャラ削除要求 */
void SendCmdACCADD			(LPSTR, LPSTR);							/* アカウント追加要求 */
void SendCmdGETACC			(DWORD);								/* アカウント・パスワード取得 */
void SendCmdCHGACC			(DWORD, LPSTR, LPSTR);					/* アカウント・パスワード更新 */
void SendCmdTALKRAND		(DWORD);								/* 話してランダム要求 */
void SendCmdSTOPRAND		(DWORD);								/* 止まるかランダム切り替え */
void SendCmdGETACCINFO		(DWORD);								/* アカウント情報要求 */
void SendCmdCHGACCSTATE		(DWORD, BYTE);							/* アカウント状態更新 */
void SendCmdCLIVERSION		(void);									/* クライアントバージョン通知 */
void SendCmdCHGCEF			(int);									/* 吹き出し情報更新 */
void SendCmdCHGADMINON		(void);									/* 管理人ON/OFF通知 */
void SendCmdADDITEM			(void);									/* アイテム追加 */
void SendCmdCHGITEM			(DWORD);								/* アイテム情報更新 */
void SendCmdGETITEM			(DWORD, DWORD);							/* アイテム取得 */
void SendCmdDROPITEM		(BYTE, DWORD, DWORD);					/* アイテム捨てる */
void SendCmdREQCHARITEM		(DWORD);								/* 所持アイテム情報要求 */
void SendCmdCHARITEM		(DWORD);								/* 所持アイテム情報 */
void SendCmdPLAYSOUNDALL	(DWORD);								/* 効果音放送 */
void SendCmdUSEITEM			(BYTE, DWORD);							/* アイテム使う */
void SendCmdSVRINFOREQ		(void);									/* サーバー情報要求 */
void SendCmdNAMECOLOR		(DWORD, COLORREF, COLORREF);			/* 名前の色設定 */
void SendCmdADDSHOP			(void);									/* 店追加要求 */
void SendCmdCHGSHOP			(BYTE);									/* 店情報更新 */
void SendCmdADDEFC			(void);									/* エフェクト追加 */
void SendCmdCHGEFC			(void);									/* エフェクト更新 */
void SendCmdEFCREQ			(BYTE, BYTE);							/* エフェクト発動 */
void SendCmdREQCHARSTATUS	(DWORD);								/* ステータス情報要求 */
void SendCmdCHARSTATUS		(DWORD);								/* キャラステータス情報 */
void SendCmdTARGETCHAR		(int);									/* ターゲット要求 */
void SendCmdTARGETCHAR		(DWORD);								/* ターゲット要求 */
void SendCmdCHGEQU			(DWORD, BYTE);							/* 装備変更 */
void SendCmdSTTMPLATE		(DWORD, BYTE);							/* ステータステンプレート情報 */
void SendCmdLEAVEITEM		(BYTE);									/* アイテム預ける */
void SendCmdLEAVEITEMGET	(DWORD);								/* 預けてるアイテム取得 */
void SendCmdSUBGAMEINFOGET	(DWORD);								/* サブゲーム情報取得 */
void SendCmdSUBGAMEINFOADD	(void);									/* サブゲーム情報追加 */
void SendCmdSUBGAMEINFO		(int);									/* サブゲーム情報通知 */
void SendCmdTRADE			(DWORD, BYTE);							/* 取り引きコマンド */
void SendCmdTRADEINFO		(void);									/* 取り引き情報 */
void SendCmdTRADEPROC		(void);									/* 取り引き決定 */
void SendCmdLEAVEGOLD		(DWORD);								/* お金預ける */
void SendCmdLEAVEGOLDGET	(DWORD);								/* 預けてるお金取得 */
void SendCmdSETSTARTPOS		(BYTE, BYTE, BYTE, int nForgetMapID);	/* 復活座標設定 */
void SendCmdCHGPASSWORD		(DWORD, LPSTR);							/* パスワード更新 */
void SendCmdLEAVEINFOGET	(DWORD);								/* 預かり所情報取得 */
void SendCmdLEAVEINFO		(DWORD);								/* 預かり所情報 */
void SendCmdACCSEARCH		(BYTE, DWORD, LPSTR);					/* アカウント検索 */
void SendCmdMSGCMD_0		(BYTE);									/* メッセージコマンド:パラメータ無し */
void SendCmdMSGCMD_1		(BYTE, int);							/* メッセージコマンド:パラメータ１つ */
void SendCMDPARTY			(BYTE, DWORD, BYTE);					/* 仲間コマンド */
void SendCMDJOBINFO			(BYTE);									/* 職業情報 */
void SendCMDJOBINFOREQ		(BYTE);									/* 職業情報要求 */
void SendCMDJOBCHG			(BYTE);									/* 職業変更 */
void SendCMDMAGICINFO		(BYTE);									/* 魔法情報 */
void SendCMDMAGICINFOREQ	(BYTE);									/* 魔法情報要求 */
void SendCMDUSEMAGIC		(BYTE byID, DWORD dwTblID);				/* 魔法使用 */
void SendCMDMAGICUSERINFO	(BYTE);									/* 魔法利用者情報 */
void SendCMDMAGICUSERINFOREQ(BYTE);									/* 魔法利用者情報要求 */
void SendCMDTARGETCHG		(DWORD);								/* ターゲット変更要求 */
void SendCMDEQUUSERINFO		(BYTE);									/* 装備職業情報 */
void SendCMDEQUUSERINFOREQ	(BYTE);									/* 装備職業情報要求 */
void SendCmdADMINUNDO		(BYTE, BYTE, DWORD);					/* 管理者画面の「元に戻す」 */
void SendCmdADMINSPACE		(BYTE, DWORD);							/* 管理者の空間の切り替え */
void SendCmdADMINQUEST		(BYTE, int, const QUESTINFO *);			/* 管理者ウィンドウのギルドの依頼の編集 */
void SendCmdADMINRESURRECT	(DWORD);								/* 管理者ウィンドウからキャラをHP満タンで蘇生 */
void SendCmdMAGICKEY		(BYTE, BYTE);							/* 特技ショートカットの登録 */
void SendCMDSHOPBUY			(DWORD, DWORD);							/* アイテム購入 */
void SendCMDDIFENCE			(void);									/* 防御 */
void SendCMDTHROWSNOW		(void);									/* 雪を投げる */
void SendCmdEFECTCHG		(BYTE, BOOL);							/* エフェクト変更 */
void SendCmdRESURRECTION	(void);									/* 蘇生 */
void SendCmdREQ_ATTACK		(DWORD dwTblID, BYTE byTurnLV);			/* 攻撃要求 */
void SendCmdREQ_GUARD		(BOOL bGuard);							/* 構え(防御)の開始・終了 */
void SendCmdRECIPEMAKE		(BYTE, WORD);							/* レシピで作ってもらう要求 */
void SendCmdQUEST			(BYTE, WORD, DWORD);					/* ギルドの仕事依頼 */


/* サブゲーム用コマンド送信 */
void SendCmdSubSETBOMB		(void);									/* 爆弾設置 */
void SendCmdSubBOMBINFO		(void);									/* 爆弾ゲーム設定 */
void SendCmdSubBOMBSTART	(void);									/* 試合開始 */
void SendCmdSubBOMBSTOP		(void);									/* 試合中止 */
void SendCmdSubBOMBGETITEM	(DWORD);								/* アイテム取得 */
void SendCmdSubBOMBREMOCON	(void);									/* リモコン使用 */
void SendCmdSubMAZERENEW	(BYTE);									/* 迷路作成 */
void SendCmdSubDUNGEONRENEW	(BYTE);									/* ダンジョン作成 */
void SendCmdSubWARSELECTTEAM	(BYTE);								/* 戦場ゲーム：チーム選択 */
void SendCmdSubWARQUIT			(void);								/* 戦場ゲーム：ゲームから抜ける */
void SendCmdSubWARSELECTRULE	(BYTE);								/* 戦場ゲーム：ルール選択 */
void SendCmdSubWARSELECTJOB		(BYTE);								/* 戦場ゲーム：職業選択 */
void SendCmdSubWARSELECTTURN	(BYTE);								/* 戦場ゲーム：行動順序 */
void SendCmdSubWARCOMMAND		(BYTE, BYTE);						/* 戦場ゲーム：コマンド通知 */
void SendCmdSubWARUSEMAGIC		(BYTE, BYTE);						/* 戦場ゲーム：魔法 */
void SendCmdSubSUGOROKUJOINMODE	(BYTE);								/* すごろく：参加モード選択 */
void SendCmdSubSUGOROKUQUIT		(void);								/* すごろく：ゲームから抜ける */
void SendCmdSubSUGOROKUDICE		(void);								/* すごろく：サイコロを振る */

void SendCmdCHAR_TAIL			(DWORD dwTblIDTarget);				/* キャラ系:ついていく要求 */


/* ========================================================================= */
/* 関数名：SendCmdLOGINREQ													 */
/* 内容：コマンド送信(ログイン要求)											 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void SendCmdLOGINREQ(void)
{
	PCMDPARA_LOGINREQ pSendData;

	pSendData = (PCMDPARA_LOGINREQ)new CMDPARA_LOGINREQ;
	ZeroMemory (pSendData, sizeof (CMDPARA_LOGINREQ));
	pSendData->byCmd = CMD_LOGINREQ;
	pSendData->pID = g_pMyID;
	strcat (pSendData->szAcc, g_szAcc);
	strcat (pSendData->szPas, g_szPas);
	CopyMemory (pSendData->szMacAdr, g_szMacAdr, 6);

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_LOGINREQ));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdWEBACCOUNT												 */
/* 内容：コマンド送信(ブラウザ版のアカウント関係)							 */
/*       WEBACC_TOKENLOGIN … g_szWebToken の端末トークンでログイン要求		 */
/*       WEBACC_TOKENREQ   … ログインできたので端末トークンを発行してもらう	 */
/*       WEBACC_OPTBGM     … 音楽ON/OFFをアカウントに覚えてもらう(byData)	 */
/*       WEBACC_OPTMAGICBAR … 特技ショートカットの帯の表示をアカウントに覚えてもらう */
/* ========================================================================= */
void SendCmdWEBACCOUNT(BYTE byCmdSub, BYTE byData)
{
	CMDPARA_WEBACCOUNT SendData;

	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd = CMD_WEBACCOUNT;
	SendData.byCmdSub = byCmdSub;
	SendData.pID = g_pMyID;
	SendData.byData = byData;
	if (byCmdSub == WEBACC_TOKENLOGIN) {
		strncpy (SendData.szToken, g_szWebToken, MAX_WEBTOKEN);
		CopyMemory (SendData.szMacAdr, g_szMacAdr, 6);
	}
	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}

/* ========================================================================= */
/* 関数名：SendCmdCHGSTATE													 */
/* 内容：コマンド送信(状態遷移)												 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void SendCmdCHGSTATE(BYTE byState)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_CHGSTATE;
	pSendData->pID = g_pMyID;
	pSendData->byData = byState;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGDIRECTION												 */
/* 内容：コマンド送信(向き変更要求)											 */
/* 日付：2003/05/14															 */
/* ========================================================================= */
void SendCmdCHGDIRECTION(BYTE byDirection)
{
	PCMDPARA_DATA1 pSendData;
	PCHARINFO pChar;

	pChar = g_aCharInfo[g_nMyCharID];
	if (g_bReqestPos || pChar->byDotMove || !g_bEnableMove) {
		return;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		return;
	}
	if (pChar->dwTargetTblID3 != -1) {
		return;
	}
	if (pChar->dwTailTblID != -1) {
		return;
	}
	/* 送信キューの数を取得 */
//	IDirectPlay8Client *pCli = cli.GetPtr ();
//	DWORD dwNumMsgs;
//	HRESULT hr;
//	hr = pCli->GetSendQueueInfo (&dwNumMsgs, NULL, DPNGETSENDQUEUEINFO_PRIORITY_NORMAL);
//	if (hr != S_OK) {
//		return;
//	}
//	if (dwNumMsgs > MAX_NUMMSGS) {
//		/* 溜まりすぎなので中止 */
//		return;
//	}

    if (GetBowWeapon(pChar) && pChar->byDirection != byDirection) PauseBowTurnGage(pChar);

	g_bReqestPos = TRUE;
	pSendData = (PCMDPARA_DATA1)new BYTE[sizeof (CMDPARA_DATA1)];
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd	= CMD_CHGDIRECTION;
	pSendData->pID		= g_pMyID;
	pSendData->dwTblID	= pChar->dwTblID;
	pSendData->byData	= byDirection;

	PostMessage (g_hWndMain, WM_RCVDATA, (WPARAM)pSendData, (LPARAM)sizeof (CMDPARA_CHARPOS));
	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
}

/* ========================================================================= */
/* 関数名：SendCmdCHARPOS													 */
/* 内容：コマンド送信(移動要求)												 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
BOOL g_bSendCharPosDone = FALSE;

void SendCmdCHARPOS(BYTE byDirection, BOOL bMovingFlg)
{
	PCMDPARA_CHARPOS pSendData;
	PCHARINFO pChar;
	DWORD dwX, dwY;
	BYTE byHitType;

	if (g_nMyCharID < 0) {
		return;
	}

	/* フェードイン・アウト中？ */
	if (g_aFadeInOutTbl.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];

	if (pChar->dwTargetTblID3 != -1) {
		return;
	}
	if (pChar->dwTailTblID != -1) {
		return;
	}

	if (((pChar->dwX == 0) && (byDirection == 2)) ||
		((pChar->dwX == (DWORD)g_pMapInfo->nCX - 1) && (byDirection == 3)) ||
		((pChar->dwY == 0) && (byDirection == 0)) ||
		((pChar->dwY == (DWORD)g_pMapInfo->nCY - 1) && (byDirection == 1))) {
		return;
	}

	if (g_bReqestPos || pChar->byDotMove || !g_bEnableMove || g_dwTimeMoveDisable) {
		return;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		return;
	}
	if (GetKeyState (VK_SHIFT) & 0x8000) {
		return;
	}

	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	/* 橋の上から横へ落ちる・橋の下から橋の端へ出る？ */
	if (IsBridgeMoveBlock (pChar, pChar->byDirection)) {
		g_dwTimeMoveDisable = timeGetTime () + 250;
		return;
	}
	if (g_bBlock) {
		byHitType = (&g_pMapPartsInfo[g_pMapInfo->pwMap[dwX + dwY * g_pMapInfo->nCX]])->byHitType;
		switch (byHitType) {
		case PARTSHIT_BLOCK1:
		case PARTSHIT_HIDE2:
		case PARTSHIT_HIDE4:
		case PARTSHIT_COUNTER:
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_BLOCK]);
			g_dwTimeMoveDisable = timeGetTime () + 250;
			return;

		case PARTSHIT_BLOCK2:
		case PARTSHIT_HIDE6:
			g_dwTimeMoveDisable = timeGetTime () + 250;
			return;
		}
	}

	/* 前回位置から2歩以上移動する？ */
	if (pChar->dwXBack != -1) {
		if ((abs (pChar->dwXBack - pChar->dwX) > 1) ||
			(abs (pChar->dwYBack - pChar->dwY) > 1)) {
			/* あやしいのでそっと終了。 */
			PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
			return;
		}
	}

	pSendData = (PCMDPARA_CHARPOS)new BYTE[sizeof (CMDPARA_CHARPOS)];
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARPOS));
	pSendData->byCmd		= CMD_CHARPOS;
	pSendData->pID			= g_pMyID;
	pSendData->dwTblID		= pChar->dwTblID;
	pSendData->dwMapID		= pChar->dwMapID;
	pSendData->byX			= (BYTE)(pChar->dwX + anPosAddTblX[pChar->byDirection]);
	pSendData->byY			= (BYTE)(pChar->dwY + anPosAddTblY[pChar->byDirection]);
	pSendData->byDirection	= pChar->byDirection;

	DWORD dwItemGet = -1;
	if (g_bBlock) {// && g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pSendData->byY + pSendData->byX]) {
		int i, nBridgeLayer;
		/* 行き先が橋なら、自分が橋の上を渡るか(1)下をくぐるか(2) */
		nBridgeLayer = GetBridgeLayerPos (pChar, pSendData->byX, pSendData->byY);
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			PCHARINFO pCharTmp = g_aCharInfo[i];
			if ((CheckCharX (pCharTmp, pCharTmp->dwX, pSendData->byX)) && (pCharTmp->dwY == pSendData->byY)) {
				/* 橋の上と下ですれちがう人とはぶつからない */
				if (nBridgeLayer && (GetCharBridgeLayer (pCharTmp) != nBridgeLayer)) {
					continue;
				}
				/* 戦場ゲームに参加中？ */
				if (pChar->abySubStatus[0] == SUBGAMEID_WAR) {
					return;
				}
				/* 壊せる壁？ */
				if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
					/* 爆弾ゲームに参加中？ */
					if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
						PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
						/* 壁すり抜け可？ */
						if (pSubStatus->byFlgBlockThrough == 1) {
							goto SendCmdCHARPOSEND;
						}
						/* 試合中ではない？ */
						if (!pSubStatus->byFlgEntry) {
							continue;
						}
					}
				/* 爆弾？ */
				} else if (pCharTmp->byNpcType == NPCTYPE_BOMB) {
					/* 爆弾ゲームに参加中？ */
					if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
						PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
						/* 爆弾すり抜け可？ */
						if (pSubStatus->byFlgBombThrough == 1) {
							goto SendCmdCHARPOSEND;
						}
					}

				/* 爆弾ゲームアイテム？ */
				} else if (pCharTmp->byNpcType == NPCTYPE_BOMBITEM) {
					dwItemGet = pCharTmp->dwTblID;
					goto SendCmdCHARPOSEND;
				}
				if ((pCharTmp->byNpcType) &&
					((pCharTmp->byNpcType != NPCTYPE_PUT) &&
					 (pCharTmp->byNpcType != NPCTYPE_USEMAGIC) &&
					 (pCharTmp->byNpcType != NPCTYPE_USEMAGICPARENT) &&
					 (pCharTmp->byNpcType != NPCTYPE_ATACK) &&
					 (pCharTmp->byNpcType != NPCTYPE_NOBLOCK))) {
					if ((pCharTmp->byNpcType != NPCTYPE_SEARCH) &&
						(pCharTmp->byNpcType != NPCTYPE_SEARCH2) &&
						(pCharTmp->byNpcType != NPCTYPE_MERCENARY) &&
						(pCharTmp->byNpcType != NPCTYPE_QUESTNPC)) {
						/* NPCとはぶつかる */
						delete [] pSendData;
						/* 宝箱の敵（正体前）にぶつかった？ 壁にぶつかった時（ぶつかる(音付き)）と同じ音を鳴らす */
						if (pCharTmp->byNpcType == NPCTYPE_MIMIC) {
							DXAudio.PlaySecoundary (&aSoundTbl[SOUND_BLOCK]);
							g_dwTimeMoveDisable = timeGetTime () + 250;
						}
						if ((pCharTmp->byNpcType == NPCTYPE_BLOCK) ||
							(pCharTmp->byNpcType == NPCTYPE_BLOCK2) ||
							(pCharTmp->byNpcType == NPCTYPE_BLOCK3) ||
							(pCharTmp->byNpcType == NPCTYPE_BLOCK4)	||
							(pCharTmp->byNpcType == NPCTYPE_CHGPUSH)) {
							/* 押して要求送信 */
							SendCmdCHARPUSH (pCharTmp->dwTblID);
							g_bReqestPos = TRUE;
						}
						return;
					}
				}
			}
		}
	}

SendCmdCHARPOSEND:
    if (GetBowWeapon(pChar)) PauseBowTurnGage(pChar);
	g_bSendCharPosDone = TRUE;
	PostMessage (g_hWndMain, WM_RCVDATA, (WPARAM)pSendData, (LPARAM)sizeof (CMDPARA_CHARPOS));
	g_bReqestPos = TRUE;
	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHARPOS));

	if (bMovingFlg) {
		g_bMoving = TRUE;
	}
	if (dwItemGet != -1) {
		SendCmdSubBOMBGETITEM (dwItemGet);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdCHARINFOREQ												 */
/* 内容：コマンド送信(キャラ情報要求)										 */
/* 日付：2003/05/21															 */
/* ========================================================================= */
void SendCmdCHARINFOREQ(DWORD dwTblID)
{
	PCMDPARA_CHARINFOREQ pSendData;

	int i;
	for (i = 0; i < g_aRecTbl.GetSize (); i ++) {
		DWORD dwTmp = g_aRecTbl[i];
		/* すでにキューを出してる？ */
		if (dwTmp == dwTblID) {
			g_aRecTbl.RemoveAt (i);
			return;
		}
	}

	pSendData = new CMDPARA_CHARINFOREQ;
	if (!pSendData) {
		return;
	}
	g_aRecTbl.Add (dwTblID);
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARINFOREQ));
	pSendData->byCmd = CMD_CHARINFOREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwTblIDReq = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHARINFOREQ));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSPEECH													 */
/* 内容：コマンド送信(発言通知)												 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void SendCmdSPEECH(LPSTR pszStr, BYTE byType)
{
	BYTE *pData;
	PCMDPARA_SPEECH pSendData;
	int nSize = strlen (pszStr) + 1 + sizeof (CMDPARA_SPEECH);

	pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);
	pSendData = (PCMDPARA_SPEECH)pData;
	pSendData->byCmd = CMD_SPEECH;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->wSize = (WORD)(strlen (pszStr) + 1);
	pSendData->byType = byType;
	memcpy (pData + sizeof (CMDPARA_SPEECH), pszStr, strlen (pszStr) + 1);

	cliSock.Send ((PBYTE)pSendData, nSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdHARTBEAT													 */
/* 内容：コマンド送信(生存確認通知)											 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void SendCmdHARTBEAT(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_HARTBEAT;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = 0;

	g_dwTimeHartbeatReq = timeGetTime ();
	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPPARTSADD												 */
/* 内容：コマンド送信(マップパーツ追加)										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdMAPPARTSADD(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_MAPPARTSADD;
	pSendData->pID = g_pMyID;
	pSendData->byData = 0;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPPARTSADDANIME											 */
/* 内容：コマンド送信(マップパーツアニメ追加)								 */
/* 日付：2003/07/03															 */
/* ========================================================================= */
void SendCmdMAPPARTSADDANIME(DWORD dwNo)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_MAPPARTSADDANIME;
	pSendData->dwData = dwNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGMAP													 */
/* 内容：コマンド送信(マップ書き換え)										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdCHGMAP(WORD wMap, int nX, int nY)
{
	PCMDPARA_CHGMAP pSendData;

	pSendData = new CMDPARA_CHGMAP;
	pSendData->byCmd = CMD_CHGMAP;
	pSendData->dwID = g_pMapInfo->dwID;
	pSendData->wMap = wMap;
	pSendData->nX = nX;
	pSendData->nY = nY;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHGMAP));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPPARTS													 */
/* 内容：コマンド送信(マップパーツ更新)										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdMAPPARTS(int nNo)
{
	PBYTE pSendData;
	PMAPPARTSINFO pParts = &g_pMapPartsInfo[nNo];
	PMAPPARTSINFOSAVE pSend;
	DWORD dwSendSize = sizeof (CMDPARA_MAPPARTSINFO) + sizeof (MAPPARTSINFOSAVE);
	dwSendSize += (pParts->byAnimeCount * sizeof (ANIMEINFO));

	pSendData = new BYTE[dwSendSize];
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_MAPPARTSINFO)pSendData)->byCmd = CMD_MAPPARTS;
	((PCMDPARA_MAPPARTSINFO)pSendData)->wCount = nNo;
	pSend = (PMAPPARTSINFOSAVE)(pSendData + sizeof (CMDPARA_MAPPARTSINFO));
	pSend->byViewType = pParts->byViewType;
	pSend->byHitType = pParts->byHitType;
	pSend->byAnimeType = pParts->byAnimeType;
	pSend->byAnimeCount = pParts->byAnimeCount;
	pSend->byLevel = pParts->byLevel;
	pSend->byLight = pParts->byLight;
	pSend->byDamage = pParts->byDamage;
	pSend->byPoison = pParts->byPoison;
	pSend->wGrpID = pParts->wGrpID;
	pSend->wBackID = pParts->wBackID;
	pSend->pAnimeInfo = pParts->pAnimeInfo;
	if (pParts->pAnimeInfo) {
		CopyMemory (pSendData + sizeof (CMDPARA_MAPPARTSINFO) + sizeof (MAPPARTSINFOSAVE),
				 pParts->pAnimeInfo, pParts->byAnimeCount * sizeof (ANIMEINFO));
	}

	cliSock.Send ((PBYTE)pSendData, dwSendSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdRELOADPARTS												 */
/* 内容：コマンド送信(マップパーツ更新要求)									 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdRELOADPARTS(int nNo)
{
	PCMDPARA_DATA2 pSendData;

	pSendData = new CMDPARA_DATA2;
	pSendData->byCmd = CMD_MAPPARTSRELOAD;
	pSendData->pID = g_pMyID;
	pSendData->nData = nNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA2));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDDELMAP													 */
/* 内容：コマンド送信(マップ幅増減要求)										 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void SendCmdADDDELMAP(BYTE byType, BYTE byCount)
{
	PCMDPARA_ADDDELMAP pSendData;

	pSendData = new CMDPARA_ADDDELMAP;
	pSendData->byCmd = CMD_ADDDELMAP;
	pSendData->dwID = g_pMapInfo->dwID;
	pSendData->byType = byType;
	pSendData->byCount = byCount;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_ADDDELMAP));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSAVEMAP													 */
/* 内容：コマンド送信(マップ保存)											 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void SendCmdSAVEMAP(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_SAVEMAP;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = 0;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTADD													 */
/* 内容：コマンド送信(イベント情報追加)										 */
/* 日付：2003/06/02															 */
/* ========================================================================= */
void SendCmdEVENTADD(void)
{
	PCMDPARA_EVENT pSendData;

	pSendData = new CMDPARA_EVENT;
	pSendData->byCmd = CMD_EVENTADD;
	pSendData->pID = g_pMyID;
	pSendData->dwID = g_pMapInfo->dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_EVENT));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTGET													 */
/* 内容：コマンド送信(イベント情報取得)										 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void SendCmdEVENTGET(int nNo, DWORD dwX, DWORD dwY)
{
	PCMDPARA_EVENT pSendData;

	pSendData = new CMDPARA_EVENT;
	pSendData->byCmd = CMD_EVENTGET;
	pSendData->pID = g_pMyID;
	pSendData->nData = nNo;
	pSendData->dwID = g_pMapInfo->dwID;
	pSendData->dwX = dwX;
	pSendData->dwY = dwY;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_EVENT));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTINFO													 */
/* 内容：コマンド送信(イベント情報更新)										 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void SendCmdEVENTINFO(void)
{
	PCMDPARA_CHGEVENT pSendData;
	int nSize = sizeof (CMDPARA_CHGEVENT),
		nDataSize = 0;

	if (g_pEventInfo->pData) {
		nDataSize = GetEventDataSize (g_pEventInfo->byType);
		nSize += nDataSize;
	}

	pSendData = (PCMDPARA_CHGEVENT)new BYTE [nSize];
	pSendData->byCmd = CMD_CHGEVENT;
	pSendData->dwID = g_pMapInfo->dwID;
	pSendData->dwDataID = (DWORD)g_nSelectEventID;
	pSendData->dwX = g_pEventInfo->dwX;
	pSendData->dwY = g_pEventInfo->dwY;
	pSendData->byType = g_pEventInfo->byType;
	pSendData->pData = g_pEventInfo->pData;
	if (pSendData->pData) {
		CopyMemory ((PBYTE)pSendData + sizeof (CMDPARA_CHGEVENT), (PBYTE)g_pEventInfo->pData, nDataSize);
	}

	cliSock.Send ((PBYTE)pSendData, nSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGMAPINFO												 */
/* 内容：コマンド送信(マップ情報更新)										 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void SendCmdCHGMAPINFO(void)
{
	PCMDPARA_CHGMAPINFO pSendData;
	PBYTE pData;
	DWORD dwSize = sizeof (CMDPARA_CHGMAPINFO);

	/* マップ名がある？ */
	if (g_pMapInfo->pszName) {
		dwSize += (strlen ((LPSTR)g_pMapInfo->pszName) + 1);
	}

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_CHGMAPINFO)pData;
	pSendData->byCmd = CMD_CHGMAPINFO;
	pSendData->dwID = g_pMapInfo->dwID;
	pSendData->nStartX = g_pMapInfo->nStartX;
	pSendData->nStartY = g_pMapInfo->nStartY;
	pSendData->nEntranceX = g_pMapInfo->nEntranceX;
	pSendData->nEntranceY = g_pMapInfo->nEntranceY;
	pSendData->byFlg = g_pMapInfo->byFlg;
	pSendData->wBGM = g_pMapInfo->wBGM;
	pSendData->byLightLevel = g_pMapInfo->byLightLevel;
	pSendData->byWeather	= g_pMapInfo->byWeather;
	pSendData->clChg = g_pMapInfo->clChg;
	pSendData->byDarkAreaCount = g_pMapInfo->byDarkAreaCount;
	pSendData->byViewRange = g_pMapInfo->byViewRange;
	pSendData->wViewHideParts = g_pMapInfo->wViewHideParts;
	CopyMemory (pSendData->aDarkArea, g_pMapInfo->aDarkArea, sizeof (pSendData->aDarkArea));
	pSendData->byLightCount = g_pMapInfo->byLightCount;
	CopyMemory (pSendData->aLight, g_pMapInfo->aLight, sizeof (pSendData->aLight));

	pSendData ++;
	strcat ((LPSTR)pSendData, (LPSTR)g_pMapInfo->pszName);

	cliSock.Send (pData, dwSize);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTCHECK												 */
/* 内容：コマンド送信(イベントチェック要求)									 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
BOOL SendCmdEVENTCHECK(BYTE byData)
{
	PCMDPARA_DATA1 pSendData;

	if (g_aCharInfo[g_nMyCharID]->dwTargetTblID3 != -1) {
		return FALSE;
	}

	g_bEnableMove = FALSE;
	cliSock.SendCancel ();

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_CHECKEVENT;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byData;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;

	return TRUE;
}

/* ========================================================================= */
/* 関数名：SendCmdADDCHAR													 */
/* 内容：コマンド送信(キャラ追加要求)										 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void SendCmdADDCHAR(PCMDPARA_ADDCHARINFO pChar)
{
	pChar->byCmd = CMD_ADDCHAR;
	pChar->dwMapID = g_pMapInfo->dwID;

	/* 名前は可変長に詰めて送る */
	DWORD dwSize = SboPackSizeADDCHARINFO (pChar);
	PBYTE pSendData = new BYTE[dwSize];
	SboPackPutADDCHARINFO (pSendData, pChar);
	cliSock.Send (pSendData, dwSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdBANCHAR													 */
/* 内容：コマンド送信(キャラ強制ログオフ要求)								 */
/* 日付：2003/06/09															 */
/* ========================================================================= */
void SendCmdBANCHAR(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_BANCHAR;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARINFO													 */
/* 内容：コマンド送信(キャラ情報)											 */
/* 日付：2003/06/10															 */
/* ========================================================================= */
void SendCmdCHARINFO(PCMDPARA_CHARINFO pCharInfo)
{
	PBYTE pSendData, pPos;
	int nSpeech, nSize;

	pCharInfo->byCmd = CMD_CHARINFO;

	/* 名前は可変長に詰めて送る（後ろに NPC 発言） */
	nSpeech = pCharInfo->pszNpcSpeech ? (int)strlen (pCharInfo->pszNpcSpeech) + 1 : 0;
	nSize = (int)SboPackSizeCHARINFO (pCharInfo) + nSpeech;
	pSendData = new BYTE[nSize];
	pPos = SboPackPutCHARINFO (pSendData, pCharInfo);
	if (nSpeech) {
		CopyMemory (pPos, pCharInfo->pszNpcSpeech, nSpeech);
	}

	cliSock.Send (pSendData, nSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDMAP													 */
/* 内容：コマンド送信(マップ追加)											 */
/* 日付：2003/07/06															 */
/* ========================================================================= */
void SendCmdADDMAP(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_ADDMAP;
	pSendData->pID = g_pMyID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDMSG													 */
/* 内容：コマンド送信(メッセージ追加)										 */
/* 日付：2003/07/15															 */
/* ========================================================================= */
void SendCmdADDMSG(LPSTR pszMsg)
{
	PCMDPARA_DATA1 pSendData;

	int nSize = sizeof (CMDPARA_DATA1) + strlen (pszMsg) + 1;
	PBYTE pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);

	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_ADDMSG;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = 0;
	CopyMemory (pData + sizeof (CMDPARA_DATA1), pszMsg, strlen (pszMsg));

	cliSock.Send (pData, nSize);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPMSGREQ													 */
/* 内容：コマンド送信(メッセージ要求)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void SendCmdMAPMSGREQ(DWORD dwNo)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));

	pSendData->byCmd = CMD_MAPMSGREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGMSG													 */
/* 内容：コマンド送信(メッセージ更新)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void SendCmdCHGMSG(LPSTR pszMsg, DWORD dwNo)
{
	PCMDPARA_DATA3 pSendData;

	int nSize = sizeof (CMDPARA_DATA3) + strlen (pszMsg) + 1;
	PBYTE pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);

	pSendData = (PCMDPARA_DATA3)pData;
	pSendData->byCmd = CMD_CHGMSG;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwNo;
	CopyMemory (pData + sizeof (CMDPARA_DATA3), pszMsg, strlen (pszMsg));

	cliSock.Send (pData, nSize);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdOPENDOOR													 */
/* 内容：コマンド送信(扉開け要求)											 */
/* 日付：2003/07/23															 */
/* ========================================================================= */
void SendCmdOPENDOOR(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_OPENDOOR;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGCHARGRP												 */
/* 内容：コマンド送信(着せ替え要求)											 */
/* 日付：2003/07/24															 */
/* ========================================================================= */
void SendCmdCHGCHARGRP(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_CHGCHARGRP;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGCHARGRPBACK											 */
/* 内容：コマンド送信(キャラ画像記録)										 */
/* 日付：2004/04/03															 */
/* ========================================================================= */
void SendCmdCHGCHARGRPBACK(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_CHGCHARGRPBACK;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdNAMESEX													 */
/* 内容：コマンド送信(名前と性別更新)										 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
void SendCmdNAMESEX(PCHARINFO pChar)
{
	PCMDPARA_NAMESEX pSendData;
	pSendData = new CMDPARA_NAMESEX;
	ZeroMemory (pSendData, sizeof (CMDPARA_NAMESEX));
	pSendData->byCmd = CMD_CHGNAMESEX;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	strcat (pSendData->szName, pChar->szName);
	pSendData->bySex = (pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE) ? 1 : 0;

	/* 文字列は可変長に詰めて送る */
	{
		DWORD dwPackSize = SboPackSizeNAMESEX (pSendData);
		PBYTE pPack = new BYTE[dwPackSize];
		SboPackPutNAMESEX (pPack, pSendData);
		cliSock.Send (pPack, dwPackSize);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdGMCALL													 */
/* 内容：コマンド送信(管理人呼び出し)										 */
/* 日付：2003/08/04															 */
/* ========================================================================= */
void SendCmdGMCALL(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_GMCALL;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPMOVE													 */
/* 内容：コマンド送信(マップ移動)											 */
/* 日付：2003/08/06															 */
/* ========================================================================= */
void SendCmdMAPMOVE(DWORD dwTblID, DWORD dwX, DWORD dwY, DWORD dwMapID, BYTE byEfect)
{
	PCMDPARA_MAPMOVE pSendData;
	pSendData = new CMDPARA_MAPMOVE;
	pSendData->byCmd = CMD_MAPMOVE;
	pSendData->dwTblID = dwTblID;
	pSendData->dwX = dwX;
	pSendData->dwY = dwY;
	pSendData->dwMapID = dwMapID;
	pSendData->byEfect = byEfect;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_MAPMOVE));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARPUSH													 */
/* 内容：コマンド送信(キャラ押して要求)										 */
/* 日付：2003/08/11															 */
/* ========================================================================= */
void SendCmdCHARPUSH(DWORD dwTblID)
{
	PCMDPARA_CHARPUSH pSendData;
	pSendData = new CMDPARA_CHARPUSH;
	pSendData->byCmd = CMD_CHARPUSH;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwTblIDPush = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHARPUSH));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCOMEFC													 */
/* 内容：コマンド送信(吹き出し要求)											 */
/* 日付：2003/08/12															 */
/* ========================================================================= */
void SendCmdCOMEFC(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_COMEFC;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDSCORE													 */
/* 内容：コマンド送信(得点追加要求)											 */
/* 日付：2003/08/22															 */
/* ========================================================================= */
void SendCmdADDSCORE(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_ADDSCORE;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdDELCHAR													 */
/* 内容：コマンド送信(キャラ削除要求)										 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void SendCmdDELCHAR(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_DELCHAR;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdACCADD													 */
/* 内容：コマンド送信(アカウント追加要求)									 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void SendCmdACCADD(LPSTR pszAcc, LPSTR pszPas)
{
	PCMDPARA_ACCADD pSendData;
	pSendData = new CMDPARA_ACCADD;
	ZeroMemory (pSendData, sizeof (CMDPARA_ACCADD));
	pSendData->byCmd = CMD_ACCADD;
	wsprintf (pSendData->szAcc, "%s", pszAcc);
	wsprintf (pSendData->szPas, "%s", pszPas);

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_ACCADD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdGETACC													 */
/* 内容：コマンド送信(アカウント・パスワード取得)							 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void SendCmdGETACC(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_GETACC;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGACC													 */
/* 内容：コマンド送信(アカウント・パスワード更新)							 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void SendCmdCHGACC(DWORD dwTblID, LPSTR pszAcc, LPSTR pszPas)
{
	PCMDPARA_ACCADD pSendData;
	pSendData = new CMDPARA_ACCADD;
	ZeroMemory (pSendData, sizeof (CMDPARA_ACCADD));
	pSendData->byCmd = CMD_CHGACC;
	pSendData->dwTblID = dwTblID;
	wsprintf (pSendData->szAcc, "%s", pszAcc);
	wsprintf (pSendData->szPas, "%s", pszPas);

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_ACCADD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdTALKRAND													 */
/* 内容：コマンド送信(話してランダム要求)									 */
/* 日付：2003/08/28															 */
/* ========================================================================= */
void SendCmdTALKRAND(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_TALKRAND;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSTOPRAND													 */
/* 内容：コマンド送信(話してランダム要求)									 */
/* 日付：2003/08/28															 */
/* ========================================================================= */
void SendCmdSTOPRAND(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_STOPRAND;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdGETACCINFO												 */
/* 内容：コマンド送信(アカウント情報要求)									 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void SendCmdGETACCINFO(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_GETACCINFO;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGACCSTATE												 */
/* 内容：コマンド送信(アカウント状態更新)									 */
/* 日付：2003/09/02															 */
/* ========================================================================= */
void SendCmdCHGACCSTATE(DWORD dwTblID, BYTE byData)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_CHGACCSTATE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;
	pSendData->byData = byData;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCLIVERSION												 */
/* 内容：コマンド送信(クライアントバージョン通知)												 */
/* 日付：2003/09/05															 */
/* ========================================================================= */
void SendCmdCLIVERSION(void)
{
	BYTE *pData;
	PCMDPARA_SPEECH pSendData;
	int nSize = strlen (VERTEXT) + 1 + sizeof (CMDPARA_SPEECH);

	pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);
	pSendData = (PCMDPARA_SPEECH)pData;
	pSendData->byCmd = CMD_CLIVERSION;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->wSize = strlen (VERTEXT) + 1;
	memcpy (pData + sizeof (CMDPARA_SPEECH), VERTEXT, strlen (VERTEXT) + 1);

	cliSock.Send ((PBYTE)pSendData, nSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGCEF													 */
/* 内容：コマンド送信(吹き出し情報更新)										 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void SendCmdCHGCEF(int nNo)
{
	PBYTE pSendData;
	PCOMEFCINFO pComEfc = &g_pComEfcInfo[nNo];
	PCOMEFCINFO pSend;
	DWORD dwSendSize = sizeof (CMDPARA_CHGCEF) + sizeof (COMEFCINFO);
	dwSendSize += (pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));

	pSendData = new BYTE[dwSendSize];
	((PCMDPARA_CHGCEF)pSendData)->byCmd = CMD_CHGCEF;
	((PCMDPARA_CHGCEF)pSendData)->wCount = nNo;
	pSend = (PCOMEFCINFO)(pSendData + sizeof (CMDPARA_CHGCEF));
	pSend->byAnimeCount = pComEfc->byAnimeCount;
	pSend->pAnime = pComEfc->pAnime;
	pSend->bySound = pComEfc->bySound;
	if (pSend->pAnime) {
		CopyMemory (pSendData + sizeof (CMDPARA_CHGCEF) + sizeof (COMEFCINFO),
				 pComEfc->pAnime, pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
	}

	cliSock.Send ((PBYTE)pSendData, dwSendSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGADMINON												 */
/* 内容：コマンド送信(管理人ON/OFF通知)										 */
/* 日付：2003/09/08															 */
/* ========================================================================= */
void SendCmdCHGADMINON(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_CHGADMINON;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDITEM													 */
/* 内容：コマンド送信(アイテム追加)											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdADDITEM(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_ADDITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGITEM													 */
/* 内容：コマンド送信(アイテム情報更新)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdCHGITEM(DWORD dwID)
{
	CMDPARA_CHGITEM Para;
	PCMDPARA_CHGITEM pSendData;
	PBYTE pData, pDataTmp;
	PITEMINFO pItem;

	if (g_aItemInfo.GetSize () <= (int)dwID) {
		return;
	}

	pItem = g_aItemInfo[dwID];
	int nLen = 0;

	DWORD dwSize = sizeof (CMDPARA_CHGITEM);
	dwSize += GetItemDataSize (pItem->byType);
	if (pItem->byType == ITEMTYPE_MSG) {
		PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
		if (pInfo) {
			if (pInfo->pMsgMe) {
				dwSize += (strlen ((LPSTR)pInfo->pMsgMe) + 1);
			}
			if (pInfo->pMsgTo) {
				dwSize += (strlen ((LPSTR)pInfo->pMsgTo) + 1);
			}
		}
	}
	if (pItem->pMsg) {
		nLen = strlen ((LPSTR)pItem->pMsg) + 1;
		dwSize += nLen;
	}

	pData = new BYTE[dwSize];
	pDataTmp = pData;
	ZeroMemory (pData, dwSize);

	/* 名前は可変長に詰めて送るので、いったん構造体に入れる */
	ZeroMemory (&Para, sizeof (Para));
	pSendData = &Para;
	pSendData->byCmd = CMD_CHGITEM;

	SboTextCopy (pSendData->szName, sizeof (pSendData->szName), pItem->szName);
	pSendData->dwID = dwID;
	pSendData->byType = pItem->byType;
	pSendData->byData = pItem->byData;
	pSendData->byFlg = pItem->byFlg;
	pSendData->bySound = pItem->bySound;
	pSendData->bySexDistinction = pItem->bySexDistinction;
	pSendData->byEraseProbability = pItem->byEraseProbability;
	pSendData->wGrpID = pItem->wGrpID;
	pSendData->dwGold = pItem->dwGold;
	pSendData->pData = pItem->pData;
	pSendData->pMsg = pItem->pMsg;

	pDataTmp = SboPackPutCHGITEM (pData, pSendData);
	/* 使った時のメッセージ */
	if (pItem->pMsg) {
		strcat ((LPSTR)pDataTmp, (LPSTR)pItem->pMsg);
		pDataTmp += nLen;
	}

	/* 詳細データ */
	if (pSendData->pData) {
		/* 文字エフェクトなどの文字列も可変長に詰める */
		pDataTmp = SboPackPutItemData (pDataTmp, pItem->byType, pSendData->pData, GetItemDataSize (pItem->byType));
		if (pItem->byType == ITEMTYPE_MSG) {
			PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
			if (pInfo) {
				if (pInfo->pMsgMe) {
					strcpy ((LPSTR)pDataTmp, (LPSTR)pInfo->pMsgMe);
					pDataTmp += (strlen ((LPSTR)pInfo->pMsgMe) + 1);
				}
				if (pInfo->pMsgTo) {
					strcpy ((LPSTR)pDataTmp, (LPSTR)pInfo->pMsgTo);
					pDataTmp += (strlen ((LPSTR)pInfo->pMsgTo) + 1);
				}
			}
		}
	}

	cliSock.Send (pData, (DWORD)(pDataTmp - pData));
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdGETITEM													 */
/* 内容：コマンド送信(アイテム取得)											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdGETITEM(DWORD dwID, DWORD dwGold)
{
	PCMDPARA_DATA5 pSendData;
	pSendData = new CMDPARA_DATA5;
	pSendData->byCmd = CMD_GETITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData1 = dwID;
	pSendData->dwData2 = dwGold;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA5));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdDROPITEM													 */
/* 内容：コマンド送信(アイテム捨てる)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdDROPITEM(BYTE byNo, DWORD dwGold, DWORD dwItemID)
{
	PCMDPARA_DATA5 pSendData;
	pSendData = new CMDPARA_DATA5;
	pSendData->byCmd = CMD_DROPITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byNo;
	pSendData->dwData1 = dwGold;
	pSendData->dwData2 = dwItemID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA5));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdREQCHARITEM												 */
/* 内容：コマンド送信(所持アイテム情報要求)									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdREQCHARITEM(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_REQCHARITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[dwID]->dwTblID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARITEM													 */
/* 内容：コマンド送信(所持アイテム情報)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdCHARITEM(DWORD dwID)
{
	PCMDPARA_CHARITEM pSendData;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSendData = (PCMDPARA_CHARITEM)new CMDPARA_CHARITEM;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARITEM));
	pSendData->byCmd = CMD_CHARITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = pChar->dwTblID;
	CopyMemory (pSendData->waItem, pChar->waItem, sizeof (pChar->waItem));
	CopyMemory (pSendData->waEqu, pChar->waEqu, sizeof (pChar->waEqu));

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHARITEM));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdPLAYSOUNDALL												 */
/* 内容：コマンド送信(効果音放送)											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdPLAYSOUNDALL(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_PLAYSOUNDALL;
	pSendData->pID = g_pMyID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdUSEITEM													 */
/* 内容：コマンド送信(アイテム使う)											 */
/* 日付：2003/09/15															 */
/* ========================================================================= */
void SendCmdUSEITEM(BYTE byNo, DWORD dwID)
{
	if (g_aCharInfo[g_nMyCharID]->wHP == 0) {
		return;
	}

	PCMDPARA_DATA4 pSendData;
	pSendData = new CMDPARA_DATA4;
	pSendData->byCmd = CMD_USEITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byNo;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA4));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSVRINFOREQ												 */
/* 内容：コマンド送信(サーバー情報要求)										 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void SendCmdSVRINFOREQ(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_SVRINFO;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = 0;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdNAMECOLOR													 */
/* 内容：コマンド送信(名前の色設定)											 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void SendCmdNAMECOLOR(DWORD dwID, COLORREF clName, COLORREF clSpeech)
{
	PCMDPARA_NAMECOLOR pSendData;
	pSendData = new CMDPARA_NAMECOLOR;
	ZeroMemory (pSendData, sizeof (CMDPARA_NAMECOLOR));
	pSendData->byCmd = CMD_NAMECOLOR;
	pSendData->dwTblID = dwID;
	pSendData->clName = clName;
	pSendData->clSpeech = clSpeech;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_NAMECOLOR));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDSHOP													 */
/* 内容：コマンド送信(店追加要求)											 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void SendCmdADDSHOP(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_ADDSHOP;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGSHOP													 */
/* 内容：コマンド送信(店情報更新)											 */
/* 日付：2003/09/22															 */
/* ========================================================================= */
void SendCmdCHGSHOP(BYTE byNo)
{
	PCMDPARA_SHOP pSendData;
	DWORD dwSize = sizeof (CMDPARA_SHOP);
	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[byNo];
	int i;

	if (pShop == NULL) {
		return;
	}

	/* 送信データサイズ計算 */
	for (i = 0; i < MAX_SHOPMSG; i ++) {
		if (pShop->apMsg[i]) {
			dwSize += (strlen (pShop->apMsg[i]) + 1);
		}
	}
	
	PBYTE pData = new BYTE[dwSize], ptrDataTmp;
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_SHOP)pData;
	pSendData->byCmd = CMD_CHGSHOP;
	pSendData->byID = byNo;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byCountItem = pShop->byCountItem;
	CopyMemory (pSendData->awItem, pShop->awItem, sizeof (pShop->awItem));
	CopyMemory (pSendData->apMsg, pShop->apMsg, sizeof (pShop->apMsg));

	ptrDataTmp = pData + sizeof (CMDPARA_SHOP);
	for (i = 0; i < MAX_SHOPMSG; i ++) {
		if (pShop->apMsg[i]) {
			strcat ((LPSTR)ptrDataTmp, pShop->apMsg[i]);
			ptrDataTmp += (strlen (pShop->apMsg[i]) + 1);
		}
	}

	cliSock.Send ((PBYTE)pData, dwSize);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDEFC													 */
/* 内容：コマンド送信(エフェクト追加)										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void SendCmdADDEFC(void)
{
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_ADDEFC;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGEFC													 */
/* 内容：コマンド送信(エフェクト情報更新)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void SendCmdCHGEFC(BYTE byNo)
{
#if 0
	PBYTE pSendData;
	PEFCINFO pEfc = g_EfcInfo.GetPtr (byNo);
	PEFCINFO pSend;
	DWORD dwSendSize = sizeof (CMDPARA_CHGCEF) + sizeof (EFCINFO);
	dwSendSize += (pEfc->byAnimeCount * sizeof (EFCANIMEINFO));

	pSendData = new BYTE[dwSendSize];
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_CHGCEF)pSendData)->byCmd = CMD_CHGEFC;
	((PCMDPARA_CHGCEF)pSendData)->wCount = byNo;
	pSend = (PEFCINFO)(pSendData + sizeof (CMDPARA_CHGCEF));
	pSend->byAnimeCount = pEfc->byAnimeCount;
	pSend->pAnime = pEfc->pAnime;
	pSend->byLevel = pEfc->byLevel;
	pSend->bySound = pEfc->bySound;
	if (pSend->pAnime) {
		CopyMemory (pSendData + sizeof (CMDPARA_CHGCEF) + sizeof (EFCINFO),
				 pEfc->pAnime, pEfc->byAnimeCount * sizeof (EFCANIMEINFO));
	}

	cliSock.Send ((PBYTE)pSendData, dwSendSize);
	delete [] pSendData;
#endif
	PBYTE pData, pDataTmp;
	DWORD dwSize, dwSizeTmp;
	PCMDPARA_CHGEFC pSend;
	PEFCINFO pEfcInfo;

	if (byNo >= g_EfcInfo.GetCount ()) {
		return;
	}

	pEfcInfo = g_EfcInfo.GetPtr (byNo);

	/* サイズの計算 */
	dwSize = 0;
	dwSize += sizeof (CMDPARA_CHGEFC);
	dwSizeTmp = g_EfcInfo.GetSendDataSize (byNo);
	dwSize += dwSizeTmp;

	/* 送信データの作成 */
	pData = new BYTE[dwSize];
	pSend = (PCMDPARA_CHGEFC)pData;
	pSend->byCmd	= CMD_CHGEFC;
	pSend->wData	= byNo;

	pDataTmp = g_EfcInfo.GetSendData (byNo);
	CopyMemory ((PBYTE)(pSend + 1), pDataTmp, dwSizeTmp);

	cliSock.Send ((PBYTE)pData, dwSize);
	SAFE_DELETE_ARRAY (pData);
	SAFE_DELETE_ARRAY (pDataTmp);
}

/* ========================================================================= */
/* 関数名：SendCmdEFCREQ													 */
/* 内容：コマンド送信(エフェクト発動)										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void SendCmdEFCREQ(BYTE byType, BYTE byID)
{
	int i, nCount = g_aCharInfo.GetSize ();
	DWORD dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	if (byID == 0) {
		return;
	}

	/* エフェクトが許可されてない？ */
	if ((g_pMapInfo->byFlg & MAPFLG_EFECT) == 0) {
		return;
	}

	if (byType != 0)  {
		DWORD dwX, dwY;
		PCHARINFO pCharMe = g_aCharInfo[g_nMyCharID];
		dwX = pCharMe->dwX + anPosAddTblX[pCharMe->byDirection];
		dwY = pCharMe->dwY + anPosAddTblY[pCharMe->byDirection];
		for (i = 0; i < nCount; i ++) {
			PCHARINFO pChar = g_aCharInfo[i];
			if (pCharMe->dwTblID == pChar->dwTblID) {
				continue;
			}
			/* 前にキャラがいる？ */
			if ((dwX == pChar->dwX) && (dwY == pChar->dwY)) {
				dwTblID = pChar->dwTblID;
				break;
			}
		}
		/* 見つからなかった */
		if (i == nCount) {
			return;
		}
	}
	
	PCMDPARA_DATA1 pSendData;
	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_EFCREQ;
	pSendData->pID = g_pMyID;
	pSendData->byData = byID;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdREQCHARSTATUS												 */
/* 内容：コマンド送信(ステータス情報要求)									 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void SendCmdREQCHARSTATUS(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_REQCHARSTATUS;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[dwID]->dwTblID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARSTATUS												 */
/* 内容：コマンド送信(キャラステータス情報)									 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void SendCmdCHARSTATUS(DWORD dwID)
{
	PCMDPARA_CHARSTATUS pSend;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSend = new CMDPARA_CHARSTATUS;
	ZeroMemory (pSend, sizeof (CMDPARA_CHARSTATUS));

	pSend->byCmd			= CMD_CHARSTATUS;
	pSend->pID				= pChar->pID;
	pSend->dwTblID			= g_aCharInfo[dwID]->dwTblID;
	pSend->bySaveFlg		= pChar->bySaveFlg;
	pSend->byPW				= pChar->byPW;
	pSend->byDEX			= pChar->byDEX;
	pSend->byST				= pChar->byST;
	pSend->byIT				= pChar->byIT;
	pSend->byLK				= pChar->byLK;
	pSend->byNpcType		= pChar->byNpcType;
	pSend->byJOB			= pChar->byJOB;
	pSend->byLevel			= pChar->byLevel;
	pSend->chAttrFire		= pChar->chAttrFire;
	pSend->chAttrWater		= pChar->chAttrWater;
	pSend->chAttrWind		= pChar->chAttrWind;
	pSend->chAttrEarth		= pChar->chAttrEarth;
	pSend->chAttrLight		= pChar->chAttrLight;
	pSend->chAttrDark		= pChar->chAttrDark;
	pSend->wAT				= pChar->wAT;
	pSend->wDF				= pChar->wDF;
	pSend->wMaxHP			= pChar->wMaxHP;
	pSend->wHP				= pChar->wHP;
	pSend->wMaxMana			= pChar->wMaxMana;
	pSend->wMana			= pChar->wMana;
	pSend->wDFMagic			= pChar->wDFMagic;
	pSend->dwGold			= pChar->dwGold;
	pSend->dwExp			= pChar->dwExp;
	pSend->byDrunkLV		= pChar->byDrunkLV;
	pSend->byDrunkPoint		= pChar->byDrunkPoint;
	pSend->byDropProbability = pChar->byDropProbability;
	CopyMemory (pSend->waEqu, pChar->waEqu, sizeof (pChar->waEqu));
	CopyMemory (pSend->abyMagic, pChar->abyMagic, sizeof (pChar->abyMagic));

	cliSock.Send ((PBYTE)pSend, sizeof (CMDPARA_CHARSTATUS));
	delete pSend;
}

/* ========================================================================= */
/* 関数名：SendCmdTARGETCHAR												 */
/* 内容：コマンド送信(ターゲット要求)										 */
/* 日付：2003/10/02															 */
/* ========================================================================= */
void SendCmdTARGETCHAR(int nMode)
{
	int i, nCount;
	DWORD dwX, dwY;
	PCMDPARA_DATA3 pSendData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID], pCharTmp;

	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));

	pSendData->byCmd	= CMD_TARGETCHAR;
	pSendData->pID		= g_pMyID;
	if (nMode == 0) {
		pSendData->pID = 0;
	}
	pSendData->dwTblID	= pChar->dwTblID;
	pSendData->dwData	= -1;

	/* 前にキャラがいるかチェック */
	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[i];
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
			if ((pCharTmp->byNpcType != NPCTYPE_PUT) && (pCharTmp->byNpcType != NPCTYPE_QUESTNPC)) {
				pSendData->dwData = pCharTmp->dwTblID;
				break;
			}
		}
	}

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	SAFE_DELETE (pSendData);
}

/* ========================================================================= */
/* 関数名：SendCmdTARGETCHAR												 */
/* 内容：コマンド送信(ターゲット要求)										 */
/* 日付：2004/11/06															 */
/* ========================================================================= */
void SendCmdTARGETCHAR(DWORD dwTblID)
{
	PCMDPARA_DATA3 pSendData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_TARGETCHAR;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->dwData = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGEQU													 */
/* 内容：コマンド送信(装備変更)												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void SendCmdCHGEQU(DWORD dwID, BYTE byType)
{
	PCMDPARA_DATA4 pSendData;

	pSendData = (PCMDPARA_DATA4)new CMDPARA_DATA4;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA4));
	pSendData->byCmd = CMD_CHGEQU;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[dwID]->dwTblID;
	pSendData->byData = byType;
	pSendData->dwData = g_aCharInfo[dwID]->waEqu[byType];

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA4));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSTTMPLATE													 */
/* 内容：コマンド送信(ステータステンプレート情報)							 */
/* 日付：2003/10/25															 */
/* ========================================================================= */
void SendCmdSTTMPLATE(DWORD dwNo, BYTE byType)
{
	PCMDPARA_DATA1 pSendData;
	PBYTE pData;

	pData = new BYTE [sizeof (CMDPARA_DATA1) + sizeof (STATUSTEMPLATEINFO)];
	ZeroMemory (pData, sizeof (CMDPARA_DATA1) + sizeof (STATUSTEMPLATEINFO));
	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_STTMPLATE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwNo;
	pSendData->byData = byType;
	/* 名前は可変長に詰める */
	PBYTE pEnd = SboPackPutSTATUSTEMPLATEINFO ((PBYTE)(pSendData + 1), g_aStTmpInfo[dwNo]);

	cliSock.Send (pData, (DWORD)(pEnd - pData));
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEITEM													 */
/* 内容：コマンド送信(アイテム預ける)										 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void SendCmdLEAVEITEM(BYTE byNo)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_LEAVEITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEITEMGET												 */
/* 内容：コマンド送信(預けてるアイテム取得)									 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void SendCmdLEAVEITEMGET(DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = (PCMDPARA_DATA3)new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_LEAVEITEMGET;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSUBGAMEINFOGET											 */
/* 内容：コマンド送信(サブゲーム情報取得)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void SendCmdSUBGAMEINFOGET(DWORD dwNo)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_SUBGAMEGET;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSUBGAMEINFOADD											 */
/* 内容：コマンド送信(サブゲーム情報追加)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void SendCmdSUBGAMEINFOADD(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAMEADD;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSUBGAMEINFO												 */
/* 内容：コマンド送信(サブゲーム情報通知)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void SendCmdSUBGAMEINFO(int nNo)
{
	PCMDPARA_DATA1 pSendData;
	PBYTE pData;
	DWORD dwSize = sizeof (CMDPARA_DATA1);

	if (g_pSubGameInfo == NULL) {
		return;
	}

	dwSize += GetSubGameDataSize (g_pSubGameInfo->byType);
	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_BOMB:
		{
			PSUBGAME_BOMBINFOSAVE pBombInfo = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
			PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)(pData + sizeof (CMDPARA_DATA1));
			pBomb->dwMapID = pBombInfo->dwMapID;
			pBomb->dwX = pBombInfo->dwX;
			pBomb->dwY = pBombInfo->dwY;
			pBomb->dwStartX = pBombInfo->dwStartX;
			pBomb->dwStartY = pBombInfo->dwStartY;
			pBomb->byX1 = pBombInfo->byX1;
			pBomb->byY1 = pBombInfo->byY1;
			pBomb->byX2 = pBombInfo->byX2;
			pBomb->byY2 = pBombInfo->byY2;
		}		
		break;

	case SUBGAMEID_MAZE:
		{
			PSUBGAME_MAZEINFOSAVE pMazeInfo = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
			PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)(pData + sizeof (CMDPARA_DATA1));
			pMaze->dwMapID = pMazeInfo->dwMapID;
			pMaze->byX1 = pMazeInfo->byX1;
			pMaze->byY1 = pMazeInfo->byY1;
			pMaze->byX2 = pMazeInfo->byX2;
			pMaze->byY2 = pMazeInfo->byY2;
			pMaze->byCharX = pMazeInfo->byCharX;
			pMaze->byCharY = pMazeInfo->byCharY;
			pMaze->byStartX = pMazeInfo->byStartX;
			pMaze->byStartY = pMazeInfo->byStartY;
			pMaze->byGoalToX = pMazeInfo->byGoalToX;
			pMaze->byGoalToY = pMazeInfo->byGoalToY;
			pMaze->wChipIDStart = pMazeInfo->wChipIDStart;
			pMaze->wChipIDGoal = pMazeInfo->wChipIDGoal;
			pMaze->wChipIDRoad = pMazeInfo->wChipIDRoad;
			pMaze->wChipIDBlock = pMazeInfo->wChipIDBlock;
		}		
		break;

	case SUBGAMEID_DUNGEON:
		{
			PSUBGAME_DUNGEONINFOSAVE pDungeonInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
			PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)(pData + sizeof (CMDPARA_DATA1));
			pDungeon->dwMapID = pDungeonInfo->dwMapID;
			pDungeon->byX1 = pDungeonInfo->byX1;
			pDungeon->byY1 = pDungeonInfo->byY1;
			pDungeon->byX2 = pDungeonInfo->byX2;
			pDungeon->byY2 = pDungeonInfo->byY2;
			pDungeon->byCharX = pDungeonInfo->byCharX;
			pDungeon->byCharY = pDungeonInfo->byCharY;
			pDungeon->byStartX = pDungeonInfo->byStartX;
			pDungeon->byStartY = pDungeonInfo->byStartY;
			pDungeon->byGoalX = pDungeonInfo->byGoalX;
			pDungeon->byGoalY = pDungeonInfo->byGoalY;
			pDungeon->wChipIDStart = pDungeonInfo->wChipIDStart;
			pDungeon->wChipIDGoal = pDungeonInfo->wChipIDGoal;
			pDungeon->wChipIDRoad = pDungeonInfo->wChipIDRoad;
			pDungeon->wChipIDBlock = pDungeonInfo->wChipIDBlock;
		}		
		break;

	case SUBGAMEID_WAR:			/* 戦場ゲーム */
		{
			PSUBGAME_WARINFOSAVE pInfo = (PSUBGAME_WARINFOSAVE)g_pSubGameInfo->pInfo;
			PSUBGAME_WARINFOSAVE pSend = (PSUBGAME_WARINFOSAVE)(pData + sizeof (CMDPARA_DATA1));
			CopyMemory (pSend, pInfo, sizeof (SUBGAME_WARINFOSAVE));
		}
		break;

	case SUBGAMEID_SUGOROKU:	/* すごろく */
		{
			PSUBGAME_SUGOROKUINFOSAVE pInfoTmp = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
			PSUBGAME_SUGOROKUINFOSAVE pInfoSend;
			DWORD i, dwEventCount, dwPartsCount;
			PBYTE pTmp;

			delete [] pData;
			dwEventCount = pInfoTmp->aEventInfo.GetSize ();
			dwPartsCount = pInfoTmp->aPartsInfo.GetSize ();
			dwSize = sizeof (CMDPARA_DATA1) + GetSubGameDataSize (g_pSubGameInfo->byType);
			dwSize += sizeof (DWORD);
			dwSize += (dwEventCount * sizeof (SUGOROKUEVENTINFO));
			dwSize += sizeof (DWORD);
			dwSize += (dwPartsCount * sizeof (SUGOROKUPARTSINFO));
			pData = new BYTE[dwSize];
			ZeroMemory (pData, dwSize);

			pInfoSend = (PSUBGAME_SUGOROKUINFOSAVE)(pData + sizeof (CMDPARA_DATA1));
			pInfoSend->dwMapID = pInfoTmp->dwMapID;
			pInfoSend->byX = pInfoTmp->byX;
			pInfoSend->byY = pInfoTmp->byY;
			pInfoSend->byRule = pInfoTmp->byRule;
			pInfoSend->byMaxCount = pInfoTmp->byMaxCount;

			pTmp = (PBYTE)(pInfoSend + 1);
			CopyMemory (pTmp, &dwEventCount, sizeof (dwEventCount));
			pTmp += sizeof (dwEventCount);
			for (i = 0; i < dwEventCount; i ++) {
				CopyMemory (pTmp, &pInfoTmp->aEventInfo[i], sizeof (SUGOROKUEVENTINFO));
				pTmp += sizeof (SUGOROKUEVENTINFO);
			}

			CopyMemory (pTmp, &dwPartsCount, sizeof (dwPartsCount));
			pTmp += sizeof (dwPartsCount);
			for (i = 0; i < dwPartsCount; i ++) {
				CopyMemory (pTmp, &pInfoTmp->aPartsInfo[i], sizeof (SUGOROKUPARTSINFO));
				pTmp += sizeof (SUGOROKUPARTSINFO);
			}
		}
		break;
	}

	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_SUBGAMEINFO;
	pSendData->byData = g_pSubGameInfo->byType;
	pSendData->dwTblID = nNo;

	cliSock.Send (pData, dwSize);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdTRADE														 */
/* 内容：コマンド送信(取り引きコマンド)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void SendCmdTRADE(DWORD dwID, BYTE byCmd)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = new CMDPARA_DATA3;
	pSendData->byCmd = CMD_TRADE;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwID;

	if (byCmd == CMDTRADE_OUT) {
		pSendData->pID = 0;
		g_bEnableMove = TRUE;
	} else {
		pSendData->pID = g_pMyID;
	}

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdTRADEINFO													 */
/* 内容：コマンド送信(取り引き情報)											 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void SendCmdTRADEINFO(void)
{
	PCMDPARA_TRADEINFO pSendData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	BYTE bySum = (BYTE)pChar->dwGold;
	for (int i = 0; i < MAX_MYITEM; i ++) {
		bySum += (BYTE)pChar->waItem[i];
	}

	pSendData = new CMDPARA_TRADEINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_TRADEINFO));
	pSendData->byCmd = CMD_TRADEINFO;
	pSendData->byMode = g_pTradeInfo->byMode1;
	pSendData->bySum = bySum;
	pSendData->dwGold = g_pTradeInfo->dwGold1;
	pSendData->dwTblID = g_pTradeInfo->dwTblID;
	CopyMemory (pSendData->waItem, g_pTradeInfo->waItem1, sizeof (g_pTradeInfo->waItem1));

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_TRADEINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdTRADEPROC													 */
/* 内容：コマンド送信(取り引き決定)											 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void SendCmdTRADEPROC(void)
{
	PCMDPARA_TRADEPROC pSendData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	BYTE bySum = (BYTE)pChar->dwGold;
	for (int i = 0; i < MAX_MYITEM; i ++) {
		bySum += (BYTE)pChar->waItem[i];
	}

	pSendData = new CMDPARA_TRADEPROC;
	ZeroMemory (pSendData, sizeof (CMDPARA_TRADEPROC));
	pSendData->byCmd = CMD_TRADEPROC;
	pSendData->bySum1 = bySum;
	pSendData->bySum2 = g_pTradeInfo->bySum2;
	pSendData->dwGold1 = g_pTradeInfo->dwGold1;
	pSendData->dwTblID1 = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwGold2 = g_pTradeInfo->dwGold2;
	pSendData->dwTblID2 = g_pTradeInfo->dwTblID;
	CopyMemory (pSendData->waItem1, g_pTradeInfo->waItem1, sizeof (g_pTradeInfo->waItem1));
	CopyMemory (pSendData->waItem2, g_pTradeInfo->waItem2, sizeof (g_pTradeInfo->waItem2));

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_TRADEPROC));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEGOLD													 */
/* 内容：コマンド送信(お金預ける)											 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void SendCmdLEAVEGOLD(DWORD dwGold)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = (PCMDPARA_DATA3)new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_LEAVEGOLD;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwGold;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEGOLDGET												 */
/* 内容：コマンド送信(預けてるお金取得)										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void SendCmdLEAVEGOLDGET(DWORD dwGold)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = (PCMDPARA_DATA3)new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_LEAVEGOLDGET;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwGold;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSETSTARTPOS												 */
/* 内容：コマンド送信(復活座標設定)											 */
/* 日付：2004/01/23															 */
/* ========================================================================= */
void SendCmdSETSTARTPOS(BYTE byMapID, BYTE byX, BYTE byY, int nForgetMapID)
{
	PCMDPARA_CHARPOS pSendData;

	pSendData = (PCMDPARA_CHARPOS)new CMDPARA_CHARPOS;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARPOS));
	pSendData->byCmd = CMD_SETSTARTPOS;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwMapID = (DWORD)byMapID;
	pSendData->byX = byX;
	pSendData->byY = byY;
	/* 町転送の行き先から忘れる町（byFlg=1 の時だけ。マップIDは byDirection に入れる） */
	if ((nForgetMapID >= 0) && (nForgetMapID <= 0xFF)) {
		pSendData->byFlg = 1;
		pSendData->byDirection = (BYTE)nForgetMapID;
	}

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_CHARPOS));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGPASSWORD												 */
/* 内容：コマンド送信(パスワード更新)										 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
void SendCmdCHGPASSWORD(DWORD dwTblID, LPSTR pszPas)
{
	PCMDPARA_ACCADD pSendData;
	pSendData = new CMDPARA_ACCADD;
	ZeroMemory (pSendData, sizeof (CMDPARA_ACCADD));
	pSendData->byCmd = CMD_CHGPASSWORD;
	pSendData->dwTblID = dwTblID;
	wsprintf (pSendData->szPas, "%s", pszPas);

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_ACCADD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEINFOGET												 */
/* 内容：コマンド送信(預かり所情報取得)										 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void SendCmdLEAVEINFOGET(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_LEAVEINFOGET;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEINFO													 */
/* 内容：コマンド送信(預かり所情報)											 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void SendCmdLEAVEINFO(DWORD dwTblID)
{
	PCMDPARA_LEAVEINFO pSendData;

	pSendData = (PCMDPARA_LEAVEINFO)new CMDPARA_LEAVEINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_LEAVEINFO));
	pSendData->byCmd = CMD_LEAVEINFO;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;
	pSendData->byCount = g_pLeaveInfo->byCount;
	pSendData->dwGold = g_pLeaveInfo->dwGold;
	CopyMemory (pSendData->waItem, g_pLeaveInfo->waItem, sizeof (pSendData->waItem));
	CopyMemory (pSendData->byItemCount, g_pLeaveInfo->byItemCount, sizeof (pSendData->byItemCount));

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_LEAVEINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdACCSEARCH													 */
/* 内容：コマンド送信(アカウント検索)										 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void SendCmdACCSEARCH(BYTE byMode, DWORD dwTblIDStart, LPSTR pszName)
{
	PCMDPARA_ACCSEARCH pSendData;

	pSendData = (PCMDPARA_ACCSEARCH)new CMDPARA_ACCSEARCH;
	ZeroMemory (pSendData, sizeof (CMDPARA_ACCSEARCH));
	pSendData->byCmd = CMD_ACC_SEARCH;
	pSendData->byMode = byMode;
	pSendData->pID = g_pMyID;
	pSendData->dwTblIDStart = dwTblIDStart;
	SboTextCopy (pSendData->szName, sizeof (pSendData->szName), pszName);

	/* 文字列は可変長に詰めて送る */
	{
		DWORD dwPackSize = SboPackSizeACCSEARCH (pSendData);
		PBYTE pPack = new BYTE[dwPackSize];
		SboPackPutACCSEARCH (pPack, pSendData);
		cliSock.Send (pPack, dwPackSize);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMSGCMD_0													 */
/* 内容：コマンド送信(メッセージコマンド:パラメータ無し)					 */
/* 日付：2004/05/08															 */
/* ========================================================================= */
void SendCmdMSGCMD_0(BYTE byCmd)
{
	PCMDPARA_MSGCMD pSendData;
	pSendData = new CMDPARA_MSGCMD;
	ZeroMemory (pSendData, sizeof (CMDPARA_MSGCMD));

	pSendData->byCmd = CMD_MSGCMD;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byCmd;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_MSGCMD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMSGCMD_1													 */
/* 内容：コマンド送信(メッセージコマンド:パラメータ１つ)					 */
/* 日付：2004/05/15															 */
/* ========================================================================= */
void SendCmdMSGCMD_1(BYTE byCmd, int nPara)
{
	PCMDPARA_MSGCMD pSendData;
	pSendData = new CMDPARA_MSGCMD;
	ZeroMemory (pSendData, sizeof (CMDPARA_MSGCMD));

	pSendData->byCmd = CMD_MSGCMD;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byCmd;
	pSendData->nData1 = nPara;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_MSGCMD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDPARTY														 */
/* 内容：コマンド送信(仲間コマンド)											 */
/* 日付：2004/06/13															 */
/* ========================================================================= */
void SendCMDPARTY(BYTE byCmdID, DWORD dwTblID, BYTE byData)
{
	PCMDPARA_PARTYCMD pSendData;
	pSendData = new CMDPARA_PARTYCMD;
	ZeroMemory (pSendData, sizeof (CMDPARA_PARTYCMD));

	pSendData->byCmd = CMD_PARTY;
	pSendData->byCmdSub = byCmdID;
	pSendData->byData = byData;
	pSendData->dwTblIDFrom = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwTblIDTo = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_PARTYCMD));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDJOBINFO													 */
/* 内容：コマンド送信(職業情報)												 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
void SendCMDJOBINFO(BYTE byID)
{
	DWORD dwSize, dwSizeInfo;
	PBYTE pData, pDataTmp, pDataInfo;
	PCMDPARA_JOBINFO pSendData;
	PJOBINFO pInfo;

	pInfo = g_JobInfo.GetPtr (byID);
	if (pInfo == NULL) {
		return;
	}

	dwSizeInfo	= g_JobInfo.GetSendDataSize (pInfo->dwJobID);
	pDataInfo	= g_JobInfo.GetSendData (pInfo->dwJobID);

	dwSize = sizeof (CMDPARA_JOBINFO);
	dwSize += dwSizeInfo;

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);
	pSendData	= (PCMDPARA_JOBINFO)pData;
	pDataTmp	= (PBYTE)(pSendData + 1);

	pSendData->byCmd	= CMD_JOBINFO;
	pSendData->byID		= byID;
	CopyMemory (pDataTmp, pDataInfo, dwSizeInfo);

	cliSock.Send (pData, dwSize);
	SAFE_DELETE_ARRAY (pData);
	SAFE_DELETE_ARRAY (pDataInfo);
}

/* ========================================================================= */
/* 関数名：SendCMDJOBINFOREQ												 */
/* 内容：コマンド送信(職業情報要求)											 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void SendCMDJOBINFOREQ(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_JOBINFOREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDJOBCHG													 */
/* 内容：コマンド送信(職業変更)												 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void SendCMDJOBCHG(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_JOBCHG;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDMAGICINFO													 */
/* 内容：コマンド送信(魔法情報)												 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void SendCMDMAGICINFO(BYTE byID)
{
	PCMDPARA_MAGICINFO pSendData;
	PMAGICINFO pInfo;

	if (byID >= g_aMagicInfo.GetSize ()) {
		return;
	}
	pInfo = g_aMagicInfo[byID];

	pSendData = (PCMDPARA_MAGICINFO)new CMDPARA_MAGICINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_MAGICINFO));
	pSendData->byCmd = CMD_MAGICINFO;
	pSendData->byID = byID;

	SboTextCopy (pSendData->szName, sizeof (pSendData->szName), pInfo->szName);
	pSendData->byType = pInfo->byType;
	pSendData->byTypeInfoID = pInfo->byTypeInfoID;
	pSendData->byUserInfo = pInfo->byUserInfo;
	pSendData->byTarget = pInfo->byTarget;
	pSendData->byTargetType = pInfo->byTargetType;
	pSendData->byTargetArea = pInfo->byTargetArea;
	pSendData->byTargetCount = pInfo->byTargetCount;
	pSendData->byTargetEfectID = pInfo->byTargetEfectID;
	pSendData->byUserEfectID = pInfo->byUserEfectID;
	pSendData->byTime = pInfo->byTime;
	pSendData->byAttr = pInfo->byAttr;
	pSendData->wData1 = pInfo->wData1;
	pSendData->wData2 = pInfo->wData2;
	pSendData->wData3 = pInfo->wData3;
	pSendData->wData4 = pInfo->wData4;
	pSendData->wMana = pInfo->wMana;
	SboTextCopy (pSendData->szExplain, sizeof (pSendData->szExplain), pInfo->szExplain);

	/* 名前と説明文は可変長に詰めて送る */
	{
		DWORD dwPackSize = SboPackSizeMAGICINFO (pSendData);
		PBYTE pPack = new BYTE[dwPackSize];
		SboPackPutMAGICINFO (pPack, pSendData);
		cliSock.Send (pPack, dwPackSize);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDMAGICINFOREQ												 */
/* 内容：コマンド送信(魔法情報要求)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void SendCMDMAGICINFOREQ(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_MAGICINFOREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDUSEMAGIC													 */
/* 内容：コマンド送信(魔法使用)												 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void SendCMDUSEMAGIC(BYTE byID, DWORD dwTblID)
{
	PCMDPARA_DATA5 pSendData;

	pSendData = (PCMDPARA_DATA5)new CMDPARA_DATA5;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA5));
	pSendData->byCmd = CMD_USEMAGIC;
	/* 町転送の特技なら、選んだ行き先の番号（0=復活位置）を入れる */
	pSendData->byData = IsTownWarpMagic (byID) ? g_byTownWarpSel : 0;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData1 = byID;
	pSendData->dwData2 = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA5));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDMAGICUSERINFO												 */
/* 内容：コマンド送信(魔法利用者情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void SendCMDMAGICUSERINFO(BYTE byID)
{
	CMDPARA_MAGICUSERINFO Para;
	PCMDPARA_MAGICUSERINFO pSendData;
	PBYTE pPos;
	PMAGICUSERINFO pInfo;
	PBYTE pSendDataTmp;
	DWORD dwSize;

	if (byID >= g_aMagicUserInfo.GetSize ()) {
		return;
	}
	pInfo = g_aMagicUserInfo[byID];

	dwSize = sizeof (CMDPARA_MAGICUSERINFO) + pInfo->byCount;
	pSendDataTmp = new BYTE[dwSize];
	ZeroMemory (pSendDataTmp, dwSize);

	/* 名前は可変長に詰めて送る（後ろに職業 ID の表） */
	ZeroMemory (&Para, sizeof (Para));
	pSendData = &Para;
	pSendData->byCmd = CMD_MAGICUSERINFO;
	pSendData->byID = byID;

	SboTextCopy (pSendData->szName, sizeof (pSendData->szName), pInfo->szName);
	pSendData->byCount = pInfo->byCount;
	pPos = SboPackPutMAGICUSERINFO (pSendDataTmp, pSendData);
	if (pInfo->byCount) {
		CopyMemory (pPos, pInfo->paJobIDTbl, pInfo->byCount);
	}
	dwSize = (DWORD)(pPos - pSendDataTmp) + pInfo->byCount;

	cliSock.Send (pSendDataTmp, dwSize);
	delete [] pSendDataTmp;
}

/* ========================================================================= */
/* 関数名：SendCMDMAGICUSERINFOREQ											 */
/* 内容：コマンド送信(魔法利用者情報要求)									 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void SendCMDMAGICUSERINFOREQ(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_MAGICUSERINFOREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDTARGETCHG													 */
/* 内容：コマンド送信(ターゲット変更要求)									 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
void SendCMDTARGETCHG(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_TARGETCHG;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDEQUUSERINFO												 */
/* 内容：コマンド送信(装備職業情報)											 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void SendCMDEQUUSERINFO(BYTE byID)
{
	CMDPARA_EQUUSERINFO Para;
	PCMDPARA_EQUUSERINFO pSendData;
	PBYTE pPos;
	PEQUUSERINFO pInfo;
	PBYTE pSendDataTmp;
	DWORD dwSize;

	if (byID >= g_aEquUserInfo.GetSize ()) {
		return;
	}
	pInfo = g_aEquUserInfo[byID];

	dwSize = sizeof (CMDPARA_EQUUSERINFO) + pInfo->byCount;
	pSendDataTmp = new BYTE[dwSize];
	ZeroMemory (pSendDataTmp, dwSize);

	/* 名前は可変長に詰めて送る（後ろに職業 ID の表） */
	ZeroMemory (&Para, sizeof (Para));
	pSendData = &Para;
	pSendData->byCmd = CMD_EQUUSERINFO;
	pSendData->byID = byID;

	SboTextCopy (pSendData->szName, sizeof (pSendData->szName), pInfo->szName);
	pSendData->byCount = pInfo->byCount;
	pPos = SboPackPutEQUUSERINFO (pSendDataTmp, pSendData);
	if (pInfo->byCount) {
		CopyMemory (pPos, pInfo->paJobIDTbl, pInfo->byCount);
	}
	dwSize = (DWORD)(pPos - pSendDataTmp) + pInfo->byCount;

	cliSock.Send (pSendDataTmp, dwSize);
	delete [] pSendDataTmp;
}

/* ========================================================================= */
/* 関数名：SendCMDEQUUSERINFOREQ											 */
/* 内容：コマンド送信(装備職業情報要求)										 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void SendCMDEQUUSERINFOREQ(BYTE byID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_EQUUSERINFOREQ;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDSHOPBUY													 */
/* 内容：コマンド送信(アイテム購入)											 */
/* 日付：2004/07/28															 */
/* ========================================================================= */
void SendCMDSHOPBUY(DWORD dwID, DWORD dwNo)
{
	PCMDPARA_DATA5 pSendData;

	pSendData = (PCMDPARA_DATA5)new CMDPARA_DATA5;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA5));
	pSendData->byCmd = CMD_SHOPBUY;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData1 = dwID;
	pSendData->dwData2 = dwNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA5));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDDIFENCE													 */
/* 内容：コマンド送信(防御)													 */
/* 日付：2004/10/17															 */
/* ========================================================================= */
void SendCMDDIFENCE(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_DIFENCE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCMDTHROWSNOW													 */
/* 内容：コマンド送信(雪を投げる)											 */
/* 日付：2004/10/30															 */
/* ========================================================================= */
void SendCMDTHROWSNOW(void)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_THROWSNOW;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEFECTCHG													 */
/* 内容：コマンド送信(エフェクト変更)										 */
/* 日付：2004/11/21															 */
/* ========================================================================= */
void SendCmdEFECTCHG(BYTE byNo, BOOL bIdle)
{
	PCMDPARA_DATA4 pSendData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	pSendData = (PCMDPARA_DATA4)new CMDPARA_DATA4;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA4));
	pSendData->byCmd = CMD_EFECTCHG;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byNo;
	pSendData->dwData = (DWORD)bIdle;
	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA4));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdRESURRECTION												 */
/* 内容：	コマンド送信(蘇生)												 */
/* 日付：	2005/11/01														 */
/* ========================================================================= */

void SendCmdRESURRECTION(void)
{
	PCMDPARA_DATA1 pSendData;

	g_bEnableMove = FALSE;
	cliSock.SendCancel ();

	pSendData = new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd	= CMD_RESURRECTION;
	pSendData->pID		= g_pMyID;
	pSendData->dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdREQ_ATTACK												 */
/* 内容：	コマンド送信(攻撃要求)											 */
/* 日付：	2006/02/26														 */
/* ========================================================================= */

void SendCmdREQ_ATTACK(
	DWORD dwTblID,		/* [in] 攻撃先テーブルID */
	BYTE byTurnLV)		/* [in] ターンレベル */
{
	PCMDPARA_DATA4 pSendData;

	pSendData = new CMDPARA_DATA4;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA4));
    PCHARINFO c = g_aCharInfo[g_nMyCharID];
    if (GetBowWeapon(c)) {
        int target = FindBowAttackTarget(c, byTurnLV);
        if (target < 0) { delete pSendData; return; }
        if (dwTblID != c->dwAtackTblID || c->byDotMove || g_bReqestPos) { delete pSendData; return; }
    }
	pSendData->byCmd	= CMD_REQ_ATTACK;
	pSendData->pID		= g_pMyID;
	pSendData->byData	= byTurnLV;
	pSendData->dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData	= dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (CMDPARA_DATA4));
	delete pSendData;
}



/* ========================================================================= */
/* 関数名：	SendCmdREQ_GUARD												 */
/* 内容：	コマンド送信(構え(防御)の開始・終了)							 */
/* ========================================================================= */

void SendCmdREQ_GUARD(
	BOOL bGuard)		/* [in] TRUE:構える FALSE:やめる */
{
	CMDPARA_DATA1 SendData;

	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_REQ_GUARD;
	SendData.pID		= g_pMyID;
	SendData.byData		= bGuard ? 1 : 0;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}



/* ========================================================================= */
/* 関数名：	SendCmdBATTLEMODE												 */
/* 内容：	コマンド送信(戦闘モードの切り替え通知)							 */
/*			サーバーは傭兵が戦うかどうかに使う								 */
/* ========================================================================= */

void SendCmdBATTLEMODE(
	BOOL bBattle)		/* [in] TRUE:戦闘モード FALSE:解除 */
{
	CMDPARA_DATA1 SendData;

	if (g_nMyCharID < 0) {
		return;
	}
	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_BATTLEMODE;
	SendData.pID		= g_pMyID;
	SendData.byData		= bBattle ? 1 : 0;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}



/* ========================================================================= */
/* 関数名：SendCmdQUEST														 */
/* 内容：コマンド送信(ギルドの仕事依頼：一覧・受ける・取り消す・渡す)		 */
/* ========================================================================= */
void SendCmdQUEST(BYTE byType, WORD wQuestID, DWORD dwNpcTblID)
{
	CMDPARA_QUEST SendData;

	if (g_nMyCharID < 0) {
		return;
	}
	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_QUEST;
	SendData.byType		= byType;
	SendData.wQuestID	= wQuestID;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;
	SendData.dwNpcTblID	= dwNpcTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}

/* ========================================================================= */
/* 関数名：	SendCmdSORTITEM													 */
/* 内容：	コマンド送信(道具欄・預かり所の並べ替え)						 */
/*			結果は CMD_CHARITEM（手持ち）・CMD_LEAVEITEMGET（預かり所）で返る */
/* 日付：	2026/11/11														 */
/* ========================================================================= */
void SendCmdSORTITEM(BYTE byTarget, BYTE byMode, BYTE byNo1, BYTE byNo2)
{
	CMDPARA_SORTITEM SendData;

	if (g_nMyCharID < 0) {
		return;
	}
	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_SORTITEM;
	SendData.byTarget	= byTarget;
	SendData.byMode		= byMode;
	SendData.byNo1		= byNo1;
	SendData.byNo2		= byNo2;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}

/* ========================================================================= */
/* 関数名：	SendCmdLEAVEITEMDISPOSE											 */
/* 内容：	コマンド送信(預かり所の品を引き取らずに売る・捨てる)			 */
/*			結果は CMD_LEAVEITEMGET（預かり所の一覧）で返る					 */
/* 日付：	2026/11/11														 */
/* ========================================================================= */
void SendCmdLEAVEITEMDISPOSE(BYTE byMode, BYTE byNo, BYTE byCount, WORD wItemID)
{
	CMDPARA_LEAVEITEMDISPOSE SendData;

	if (g_nMyCharID < 0) {
		return;
	}
	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_LEAVEITEMDISPOSE;
	SendData.byMode		= byMode;
	SendData.byNo		= byNo;
	SendData.byCount	= byCount;
	SendData.wItemID	= wItemID;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}

/* ========================================================================= */
/* 関数名：	SendCmdRECIPEMAKE												 */
/* 内容：	コマンド送信(レシピで作ってもらう要求)							 */
/*			結果は CMD_RECIPEMAKE で返ってくる								 */
/* ========================================================================= */

void SendCmdRECIPEMAKE(
	BYTE byMaker,		/* [in] 作り手の番号 */
	WORD wRecipeID)		/* [in] レシピの番号 */
{
	CMDPARA_RECIPEMAKE SendData;

	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_RECIPEMAKE;
	SendData.byMaker	= byMaker;
	SendData.wRecipeID	= wRecipeID;
	SendData.pID		= g_pMyID;
	SendData.dwTblID	= g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)&SendData, sizeof (SendData));
}



/* ========================================================================= */
/* 関数名：SendCmdSubSETBOMB												 */
/* 内容：サブゲーム用コマンド送信(爆弾設置)									 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void SendCmdSubSETBOMB(void)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_SETBOMB;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBINFO												 */
/* 内容：サブゲーム用コマンド送信(爆弾ゲーム設定)							 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void SendCmdSubBOMBINFO(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
	PSUBGAMECLIENT_BOMBINFO pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;

	PSUBCMDPARA_BOMBINFO pSendData;
	pSendData = new SUBCMDPARA_BOMBINFO;
	ZeroMemory (pSendData, sizeof (SUBCMDPARA_BOMBINFO));
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_INFO;
	pSendData->byID = pSubStatus->byID;
	pSendData->byGameTime = pInfo->byGameTime;
	pSendData->byMaxBomb = pInfo->byMaxBomb;
	pSendData->byMaxFire = pInfo->byMaxFire;
	pSendData->byFlgUseBlock = pInfo->byFlgUseBlock;
	pSendData->byFlgUseItem = pInfo->byFlgUseItem;
	pSendData->byFlgProgress = pInfo->byFlgProgress;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_BOMBINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBSTART												 */
/* 内容：サブゲーム用コマンド送信(試合開始)									 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void SendCmdSubBOMBSTART(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_START;
	pSendData->byData = pSubStatus->byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBSTOP												 */
/* 内容：サブゲーム用コマンド送信(試合中止)									 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void SendCmdSubBOMBSTOP(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_STOP;
	pSendData->byData = pSubStatus->byID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBGETITEM											 */
/* 内容：サブゲーム用コマンド送信(アイテム取得)								 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void SendCmdSubBOMBGETITEM(DWORD dwTblID)
{
	PSUBCMDPARA_DATA2 pSendData;
	pSendData = new SUBCMDPARA_DATA2;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CNDSUBBOMB_GETITEM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->dwData = dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA2));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBREMOCON											 */
/* 内容：サブゲーム用コマンド送信(リモコン使用)								 */
/* 日付：2003/12/29															 */
/* ========================================================================= */
void SendCmdSubBOMBREMOCON(void)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CNDSUBBOMB_REMOCON;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubMAZERENEW												 */
/* 内容：サブゲーム用コマンド送信(迷路作成)									 */
/* 日付：2004/01/31															 */
/* ========================================================================= */
void SendCmdSubMAZERENEW(BYTE byData)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_MAZE;
	pSendData->byCmdSub = CMDSUBBOMB_MAZERENEW;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byData;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubDUNGEONRENEW											 */
/* 内容：サブゲーム用コマンド送信(ダンジョン作成)							 */
/* 日付：2004/02/26															 */
/* ========================================================================= */
void SendCmdSubDUNGEONRENEW(BYTE byData)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_DUNGEON;
	pSendData->byCmdSub = CMDSUBBOMB_DUNGEONRENEW;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byData;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：SendCmdSubWARSELECTTEAM											 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：チーム選択)					 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void SendCmdSubWARSELECTTEAM(BYTE byTeam)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_SELECTTEAM;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byTeam;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARQUIT												 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：ゲームから抜ける)				 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void SendCmdSubWARQUIT(void)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_QUIT;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARSELECTRULE											 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：ルール選択)					 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void SendCmdSubWARSELECTRULE(BYTE byTeam)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_SELECTRULE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byTeam;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARSELECTJOB											 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：職業選択)						 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void SendCmdSubWARSELECTJOB(BYTE byTeam)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_SELECTJOB;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byTeam;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARSELECTTURN											 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：行動順序)						 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
void SendCmdSubWARSELECTTURN(BYTE byTeam)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_SELECTTURN;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byTeam;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARCOMMAND												 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：コマンド通知)					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void SendCmdSubWARCOMMAND(BYTE byCommand, BYTE byData)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = byCommand;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byData;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubWARUSEMAGIC											 */
/* 内容：サブゲーム用コマンド送信(戦場ゲーム：魔法)							 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void SendCmdSubWARUSEMAGIC(BYTE byMagic, BYTE byNo)
{
	PSUBCMDPARA_DATA3 pSendData;
	pSendData = new SUBCMDPARA_DATA3;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_WAR;
	pSendData->byCmdSub = CMDSUBWAR_MAGIC;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData1 = byMagic;
	pSendData->byData2 = byNo;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA3));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：SendCmdSubSUGOROKUJOINMODE										 */
/* 内容：サブゲーム用コマンド送信(すごろく：モード選択)						 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SendCmdSubSUGOROKUJOINMODE(BYTE byMode)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_SUGOROKU;
	pSendData->byCmdSub = CMDSUBSUGOROKU_JOINMODE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;
	pSendData->byData = byMode;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubSUGOROKUQUIT											 */
/* 内容：サブゲーム用コマンド送信(すごろく：ゲームから抜ける)				 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SendCmdSubSUGOROKUQUIT(void)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_SUGOROKU;
	pSendData->byCmdSub = CMDSUBSUGOROKU_QUIT;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubSUGOROKUDICE											 */
/* 内容：サブゲーム用コマンド送信(すごろく：サイコロを振る)					 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void SendCmdSubSUGOROKUDICE(void)
{
	PSUBCMDPARA_DATA1 pSendData;
	pSendData = new SUBCMDPARA_DATA1;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_SUGOROKU;
	pSendData->byCmdSub = CMDSUBSUGOROKU_DICE;
	pSendData->pID = g_pMyID;
	pSendData->dwTblID = g_aCharInfo[g_nMyCharID]->dwTblID;

	cliSock.Send ((PBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdCHAR_TAIL												 */
/* 内容：	コマンド送信(キャラ系:ついていく要求)							 */
/* 日付：	2006/08/02														 */
/* ========================================================================= */

void SendCmdCHAR_TAIL(DWORD dwTblIDTarget)
{
	int i, nCount;
	DWORD dwX, dwY;
	PCHARINFO pChar, pCharTmp;
	CCHAR_TAIL Packet;

	pChar = g_aCharInfo[g_nMyCharID];

	if (dwTblIDTarget == -1) {
		dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
		dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

		/* 前にキャラがいるかチェック */
		nCount = g_aCharInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			pCharTmp = g_aCharInfo[i];
			if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
				if ((pCharTmp->byNpcType != NPCTYPE_PUT) && (pCharTmp->byNpcType != NPCTYPE_QUESTNPC)) {
					dwTblIDTarget = pCharTmp->dwTblID;
					break;
				}
			}
		}
	}

	/* とりあえずついていっていることにする */
	pChar->dwTailTblID = dwTblIDTarget;

	Packet.Make (pChar->dwTblID, dwTblIDTarget, CHAR_TAIL_REQ);
	cliSock.Send (Packet.Get (), Packet.GetSize ());
}

/* ========================================================================= */
/* 関数名：SendCmdNPCROUTE													 */
/* 内容：コマンド送信(NPCの巡回ルート)										 */
/*       byCmdSub: NPCROUTE_REQ（dwMapID のマップの一覧を要求）・			 */
/*                 NPCROUTE_SET（dwTblID のNPCのルートを保存。count 0 で巡回をやめる）	 */
/* ========================================================================= */
void SendCmdNPCROUTE(BYTE byCmdSub, DWORD dwMapID, DWORD dwTblID, BYTE byCount, BYTE byMode, BYTE byWait, const BYTE *pbyX, const BYTE *pbyY)
{
	CMDPARA_NPCROUTE Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd			= CMD_NPCROUTE;
	Para.byCmdSub		= byCmdSub;
	Para.dwMapID		= dwMapID;
	Para.dwTblID		= dwTblID;
	Para.byRouteCount	= byCount;
	Para.byRouteMode	= byMode;
	Para.byRouteWait	= byWait;
	if (pbyX && pbyY) {
		CopyMemory (Para.abyRouteX, pbyX, sizeof (Para.abyRouteX));
		CopyMemory (Para.abyRouteY, pbyY, sizeof (Para.abyRouteY));
	}

	cliSock.Send ((PBYTE)&Para, sizeof (Para));
}

/* ========================================================================= */
/* 関数名：SendCmdADMINUNDO													 */
/* 内容：コマンド送信(管理者画面の「元に戻す」)								 */
/*       byCmdSub: ADMINUNDO_SCREEN（開いている画面の通知）・				 */
/*                 ADMINUNDO_INFOREQ（戻せる変更の問い合わせ）・			 */
/*                 ADMINUNDO_EXEC（dwStepID の変更を元に戻す）				 */
/* ========================================================================= */
void SendCmdADMINUNDO(BYTE byCmdSub, BYTE byScreen, DWORD dwStepID)
{
	CMDPARA_ADMINUNDO Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_ADMINUNDO;
	Para.byCmdSub	= byCmdSub;
	Para.byScreen	= byScreen;
	Para.dwStepID	= dwStepID;

	/* 案内文の欄はサーバーからの通知だけで使うので送らない */
	cliSock.Send ((PBYTE)&Para, sizeof (Para) - sizeof (Para.szMsg));
}

/* ========================================================================= */
/* 関数名：SendCmdADMINSPACE												 */
/* 内容：コマンド送信(管理者の空間の切り替え)								 */
/*       byCmdSub: ADMINSPACE_LISTREQ（空間の一覧の問い合わせ）・			 */
/*                 ADMINSPACE_SET（dwSpaceID の空間へ切り替え）				 */
/*       サーバーは構造体の全体の大きさを調べるので、一覧の欄も含めて送る	 */
/* ========================================================================= */
void SendCmdADMINSPACE(BYTE byCmdSub, DWORD dwSpaceID)
{
	CMDPARA_ADMINSPACE Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_ADMINSPACE;
	Para.byCmdSub	= byCmdSub;
	Para.dwSpaceID	= dwSpaceID;

	cliSock.Send ((PBYTE)&Para, sizeof (Para));
}

/* ========================================================================= */
/* 関数名：SendCmdADMINQUEST												 */
/* 内容：コマンド送信(管理者ウィンドウのギルドの依頼の編集)					 */
/*       byCmdSub: ADMINQUEST_LISTREQ（一覧の問い合わせ）・					 */
/*                 ADMINQUEST_SET（pInfo を nNo 番目に書き込む。-1 は足す）	 */
/*       サーバーは構造体の全体の大きさを調べるので、全部の欄を送る			 */
/* ========================================================================= */
void SendCmdADMINQUEST(BYTE byCmdSub, int nNo, const QUESTINFO *pInfo)
{
	PCMDPARA_ADMINQUEST pPara = new CMDPARA_ADMINQUEST;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINQUEST));
	pPara->byCmd	= CMD_ADMINQUEST;
	pPara->byCmdSub	= byCmdSub;
	pPara->nNo		= nNo;
	if (pInfo) {
		pPara->Info = *pInfo;
	}
	cliSock.Send ((PBYTE)pPara, sizeof (CMDPARA_ADMINQUEST));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：SendCmdADMINRESURRECT											 */
/* 内容：コマンド送信(管理者ウィンドウからキャラをHP満タンで蘇生)			 */
/*       dwTblID: サーバーのキャラ番号（CHARINFO の dwTblID）				 */
/* ========================================================================= */
void SendCmdADMINRESURRECT(DWORD dwTblID)
{
	CMDPARA_DATA1 Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_ADMINRESURRECT;
	Para.pID		= g_pMyID;
	Para.dwTblID	= dwTblID;
	cliSock.Send ((PBYTE)&Para, sizeof (Para));
}

/* ========================================================================= */
/* 関数名：SendCmdMAGICKEY													 */
/* 内容：コマンド送信(特技ショートカットの登録)								 */
/*       byNo 番目の枠に byMagicID を登録する（0 なら外す）					 */
/* ========================================================================= */
void SendCmdMAGICKEY(BYTE byNo, BYTE byMagicID)
{
	CMDPARA_MAGICKEY Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_MAGICKEY;
	Para.byCmdSub	= MAGICKEY_SET;
	Para.byNo		= byNo;
	Para.byMagicID	= byMagicID;
	cliSock.Send ((PBYTE)&Para, sizeof (Para));
}

/* Copyright(C)URARA-works 2003-2005 */
