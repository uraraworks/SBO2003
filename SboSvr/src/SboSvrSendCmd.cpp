/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			コマンド送信処理										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#include <vector>
#include "include.h"
#include "Glbldata.h"
#include "../Common/Command/Command.h"
#include "../Common/Info/InfoJob.h"
#include "../Common/Info/SubGame/InfoSubGameWAR.h"
#include "Proc/ProcSubGame.h"
#include "Proc/SubGame/ProcSubGameBase.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrNpcRoute.h"
#include "SboSvrMagicProc.h"


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void SendCmdInside2				(PCHARINFO, LPBYTE, DWORD, DWORD, BYTE, BOOL, BOOL, BYTE);	/* 同画面内のプレイヤーにコマンド送信 */
void SendCmdSubGame				(DWORD, DWORD, LPBYTE, DWORD);				/* 指定サブゲーム参加者にコマンド送信 */
void CheckCharPosSend			(PCHARINFO);								/* 画面内にキャラが見えたら送信 */
void SendCmdInsideMap			(DWORD, LPBYTE, DWORD, BYTE);				/* 指定マップ内のプレイヤーにコマンド送信 */
void SendCmdAdmin				(PBYTE, DWORD);								/* 管理人プレイヤーにコマンド送信 */
void SendCmdCONNECT				(DWORD, BYTE);								/* コマンド送信(接続応答) */
void SendCmdLOGINRES			(DWORD, DWORD, BYTE);						/* コマンド送信(ログイン応答) */
void SendCmdCHGSTATE			(DWORD, DWORD, BYTE);						/* コマンド送信(状態遷移) */
void SendCmdMAPINFO				(DWORD, DWORD);								/* コマンド送信(マップ情報通知) */
void SendCmdMAPPARTSINFO		(DWORD);									/* コマンド送信(マップパーツ情報通知) */
void SendCmdCHARINFO			(DWORD, int);								/* コマンド送信(キャラ情報通知) */
void SendCmdCHARINFO2			(DWORD, int);								/* コマンド送信(キャラ情報通知２) */
void SendCmdCHARINFO3			(int);										/* コマンド送信(キャラ情報通知) */
void SendCmdCHGDIRECTION		(DWORD, int);								/* コマンド送信(向き変更要求) */
void SendCmdCHARPOS				(DWORD, int, BYTE, BYTE);					/* コマンド送信(座標通知) */
void SendCmdSPEECH				(DWORD, int, BYTE);							/* コマンド送信(発言通知) */
void SendCmdONLINE				(DWORD);									/* コマンド送信(オンライン数通知) */
void SendCmdMAPPARTS			(DWORD, int);								/* コマンド送信(マップパーツ更新) */
void SendCmdCHGMAP				(DWORD, WORD, int, int);					/* コマンド送信(マップ書き換え) */
void SendCmdADDDELMAP			(DWORD, BYTE, BYTE);						/* コマンド送信(マップ幅増減) */
void SendCmdEVENTINFO			(DWORD, int, int);							/* コマンド送信(イベント情報更新) */
void SendCmdEVENTCOUNT			(DWORD, DWORD);								/* コマンド送信(イベント数通知) */
void SendCmdEVENTPOS			(DWORD, DWORD);								/* コマンド送信(イベント座標通知) */
void SendCmdHARTBEAT			(DWORD);									/* コマンド送信(生存確認応答) */
void SendCmdCHGBGM				(DWORD, DWORD, BYTE);						/* コマンド送信(BGM変更) */
void SendCmdMSG					(DWORD, DWORD);								/* コマンド送信(メッセージ更新) */
void SendCmdVIEWMSG				(DWORD, DWORD);								/* コマンド送信(メッセージ表示) */
void SendCmdPLAYSOUND			(DWORD, BYTE, BYTE);						/* コマンド送信(効果音再生) */
void SendCmdPLAYSOUNDinside		(DWORD, BYTE);								/* 画面内のキャラにコマンド送信(効果音再生) */
void SendCmdMAPMSG				(DWORD, LPSTR);								/* コマンド送信(メッセージ通知) */
extern void RecvCmdOnSendMsg	(DWORD pID, LPCSTR pszMsg);					/* 送るメッセージの制御コードを覚える（SboSvrRecvCmd.cpp） */
void SendCmdMAPMSGREQ			(DWORD, LPSTR);								/* コマンド送信(メッセージ通知) */
void SendCmdSYSMSG				(DWORD, LPTSTR, BYTE);						/* コマンド送信(システムメッセージ) */
void SendCmdSYSMSGMAP			(DWORD, LPSTR, BYTE);						/* コマンド送信(マップ内システムメッセージ) */
void SendCmdEFECTCHG			(DWORD, DWORD, BYTE, BOOL);					/* コマンド送信(エフェクト変更) */
void SendCmdCOMEFC				(PCHARINFO);								/* コマンド送信(吹き出し通知) */
void SendCmdERR					(DWORD, BYTE);								/* コマンド送信(エラー通知) */
void SendCmdGETACCINFO			(DWORD, DWORD);								/* コマンド送信(アカウント情報要求) */
void SendCmdCHGCEF				(DWORD, WORD);								/* コマンド送信(吹き出し情報更新) */
void SendCmdCEFINFO				(DWORD);									/* コマンド送信(吹き出し情報通知) */
void SendCmdFADEINOUT			(DWORD, BYTE);								/* コマンド送信(フェードイン・アウト) */
void SendCmdCHGITEM				(DWORD, DWORD);								/* コマンド送信(アイテム情報更新) */
void SendCmdITEMINFO			(DWORD);									/* コマンド送信(アイテム情報送信) */
void SendCmdCHARITEM			(DWORD, DWORD);								/* コマンド送信(所持アイテム情報) */
void SendCmdSVRINFO				(DWORD);									/* コマンド送信(サーバー情報) */
void SendCmdMAPCHARINFO			(DWORD);									/* コマンド送信(マップ内キャラ数通知) */
void SendCmdNAMECOLOR			(DWORD);									/* コマンド送信(名前の色設定) */
void SendCmdSHOP				(DWORD, BYTE);								/* コマンド送信(店情報) */
void SendCmdSHOPMap				(DWORD, BYTE);								/* コマンド送信(店情報・マップ指定) */
void SendCmdEFC					(BYTE);										/* コマンド送信(エフェクト情報更新) */
void SendCmdEFCINFO				(DWORD);									/* コマンド送信(エフェクト情報通知) */
void SendCmdCHARSTATUS			(DWORD, DWORD);								/* コマンド送信(キャラステータス通知) */
void SendCmdCHARSTATUSHP		(DWORD, DWORD);								/* コマンド送信(キャラステータス(HPのみ)通知) */
void SendCmdCHARSTATUSHPMANA	(DWORD);									/* コマンド送信(キャラステータス(HPとMANA)通知) */
void SendCmdCHARSTATUSGOLDEXP	(DWORD, DWORD);								/* コマンド送信(キャラステータス(GOLDのみ)通知) */
void SendCmdTARGETCHARPC		(DWORD, DWORD);								/* コマンド送信(ついていく通知) */
void SendCmdADDCHAREFC			(DWORD, COLORREF, LPSTR);					/* コマンド送信(文字エフェクト追加) */
void SendCmdCHGCOLOR			(DWORD);									/* コマンド送信(色変換) */
void SendCmdSTTMPLATE			(DWORD, BYTE);								/* コマンド送信(ステータステンプレート情報) */
void SendCmdLEAVEITEMGET		(DWORD);									/* コマンド送信(預けてるアイテム取得) */
void SendCmdSUBGAMECOUNT		(DWORD);									/* コマンド送信(サブゲーム数通知) */
void SendCmdSUBGAMEINFO			(DWORD, int);								/* コマンド送信(サブゲーム情報通知) */
void SendCmdSUBGAMEINFOALL		(DWORD);									/* コマンド送信(全サブゲーム情報通知) */
void SendCmdSUBSTATUS			(DWORD);									/* コマンド送信(サブステータス情報) */
void SendCmdTRADE				(DWORD, DWORD, BYTE);						/* コマンド送信(取り引きコマンド) */
void SendCmdSYSMSG2				(DWORD, DWORD, DWORD, DWORD);				/* コマンド送信(ID指定システムメッセージ) */
void SendCmdLEAVEGOLDGET		(DWORD);									/* コマンド送信(預けてる金額取得) */
void SendCmdLEAVEINFO			(DWORD, DWORD);								/* コマンド送信(預かり所情報) */
void SendCmdACCSEARCH			(DWORD, DWORD);								/* コマンド送信(アカウント検索) */
void SendCmdADMINWINDOW			(DWORD, BYTE);								/* コマンド送信(管理者ウィンドウ表示) */
void SendCmdPARTY				(DWORD, DWORD, BYTE, BYTE);					/* コマンド送信(仲間コマンド) */
void SendCmdJOBINFO				(DWORD, BYTE);								/* コマンド送信(職業情報) */
void SendCmdMAGICINFO			(DWORD, BYTE);								/* コマンド送信(魔法情報) */
void SendCmdMAGICUSERINFO		(DWORD, BYTE);								/* コマンド送信(魔法利用者情報) */
void SendCmdEQUUSERINFO			(DWORD, BYTE);								/* コマンド送信(装備職業情報) */
void SendCmdTARGETCHAR			(DWORD, DWORD);								/* コマンド送信(ターゲット要求) */
void SendCmdSubBombTime			(BYTE, PSUBGAME_BOMBINFO);					/* サブゲーム用コマンド送信(爆弾ゲーム：残り時間通知) */
void SendCmdSubBOMBINFO			(DWORD, BYTE, PSUBGAME_BOMBINFO);			/* サブゲーム用コマンド送信(爆弾ゲーム：設定) */
void SendCmdSubBOMBSTOP			(DWORD, BYTE, PSUBGAME_BOMBINFO);			/* サブゲーム用コマンド送信(爆弾ゲーム：試合中止) */
void SendCmdSubBOMBSETBOMB		(DWORD);									/* サブゲーム用コマンド送信(爆弾ゲーム：爆弾設置) */
void SendCmdSubWarCHGSTATE		(DWORD, BYTE);								/* サブゲーム用コマンド送信(戦場ゲーム：状態遷移) */
void SendCmdSubWarSELECTTURN	(CInfoSubGameWAR *, BYTE);					/* サブゲーム用コマンド送信(戦場ゲーム：行動順序通知) */
void SendCmdSubWarMAGIC_TARGET	(DWORD, PCMDPARA_WARSELECTMAGICTARGET);		/* サブゲーム用コマンド送信(戦場ゲーム：魔法対象者一覧) */
void SendCmdSubSugorokuCHGSTATE	(DWORD, BYTE);								/* サブゲーム用コマンド送信(すごろく：状態遷移) */
void SendCmdCHAR_STATEINFO		(DWORD pIDSend, DWORD dwTblID);				/* コマンド送信(キャラ系:状態通知) */


/* ========================================================================= */
/* 関数名：SendCmdInside2													 */
/* 内容：同画面内のプレイヤーにコマンド送信									 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void SendCmdInside2(PCHARINFO pChar, LPBYTE pSendData, DWORD dwSize,
		DWORD dwTimeOut, BYTE byPosCheck, BOOL bFlg, BOOL bSend, BYTE byPriority)
{
	int i, x, y;
	PCHARINFO pCharDst;
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	DWORD dwNumMsgs = 0;
	std::vector<DWORD> aIDDst;

	if (pData == NULL) {
		return;
	}
	for (i = 0; ; i ++) {
		if (pData[i] == -1) {
			break;
		}
		pCharDst = g_aCharInfo[pData[i]];
		if ((byPosCheck & SENDINSIDE_INSIDENOME) && (pCharDst->pID == pChar->pID)) {
			continue;
		}
		if (pCharDst->byGameState < GAMESTATE_MOVE) {
			continue;
		}

		/* 画面内にいるかチェック */
		x = abs (pCharDst->dwX - pChar->dwX);
		if (x <= DRAW_PARTS_X + 3) {
			y = abs (pCharDst->dwY - pChar->dwY);
			if (y <= DRAW_PARTS_Y + 3) {
				aIDDst.push_back (pCharDst->pID);
			}
		}
	}
	/* 圧縮とCRCを1回で済ませるため、まとめて送る */
	if (!aIDDst.empty ()) {
		svrSock.SendToMulti (&aIDDst[0], (DWORD)aIDDst.size (), pSendData, dwSize, dwTimeOut, byPriority);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdSubGame													 */
/* 内容：指定サブゲーム参加者にコマンド送信									 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void SendCmdSubGame(BYTE byID, DWORD dwMapID, LPBYTE pSendData, DWORD dwSize)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PCHARINFO pChar;
	std::vector<DWORD> aIDDst;

	if (pData == NULL) {
		return;
	}
	for (int i = 0; ; i ++) {
		if (pData[i] == -1) {
			break;
		}
		pChar = g_aCharInfo[pData[i]];
		/* 種別が一致？ */
		if (pChar->abySubStatus[0] != pInfo->byType) {
			continue;
		}
		/* IDが一致？ */
		if (pChar->abySubStatus[1] != byID) {
			continue;
		}
		aIDDst.push_back (pChar->pID);
	}
	if (!aIDDst.empty ()) {
		svrSock.SendToMulti (&aIDDst[0], (DWORD)aIDDst.size (), pSendData, dwSize, 0, URARASOCK_SENDPRIORITY_MIDDLE);
	}
}

/* ========================================================================= */
/* 関数名：CheckCharPosSend													 */
/* 内容：同画面内にキャラが見えたら送信										 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void CheckCharPosSend(PCHARINFO pChar)
{
	int i, x, y;
	PCHARINFO pCharDst;
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	DWORD dwNumMsgs = 0;

	if (pData == NULL) {
		return;
	}

	/* NPC？ */
	if (pChar->byGameState == GAMESTATE_NPC) {
		return;
	}

	for (i = 0; ; i ++) {
		if (pData[i] == -1) {
			if (pData == pMap->paTblNoPC) {
				/* 今度はNPCのチェック */
				pData = pMap->paTblNoNPC;
				i = -1;
				/* NPCはいない？ */
				if (pData == NULL) {
					break;
				}
				continue;
			}
			break;
		}
		pCharDst = g_aCharInfo[pData[i]];
		if (pCharDst->pID == pChar->pID) {
			continue;
		}

		BOOL bSend = FALSE;
		x = abs (pCharDst->dwX - pChar->dwX);
		y = abs (pCharDst->dwY - pChar->dwY);
		switch (pChar->byDirection) {
		case 0:
		case 1:
			if (x <= DRAW_PARTS_X + 3) {
				if ((y <= DRAW_PARTS_Y + 3) && (y >= DRAW_PARTS_Y + 1)) {
					bSend = TRUE;
				}
			}
			break;

		case 2:
		case 3:
			if (y <= DRAW_PARTS_Y + 3) {
				if ((x <= DRAW_PARTS_X + 3) && (x >= DRAW_PARTS_X + 1)) {
					bSend = TRUE;
				}
			}
			break;
		}
		if (bSend) {
			SendCmdCHARPOS (pChar->pID, pCharDst->dwTblID, 0, 0);
		}
	}
}

/* ========================================================================= */
/* 関数名：SendCmdInsideMap													 */
/* 内容：指定マップ内のプレイヤーにコマンド送信								 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdInsideMap(DWORD dwID, LPBYTE pSendData, DWORD dwSize)
{
	int i;
	PMAPINFO pMap = g_aMapInfo[dwID];
	PDWORD pData = pMap->paTblNoPC;
	PCHARINFO pChar;
	std::vector<DWORD> aIDDst;

	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		pChar = g_aCharInfo[pData[i]];
		if (pChar->pID) {
			aIDDst.push_back (pChar->pID);
		}
	}
	if (!aIDDst.empty ()) {
		svrSock.SendToMulti (&aIDDst[0], (DWORD)aIDDst.size (), pSendData, dwSize, 0, URARASOCK_SENDPRIORITY_MIDDLE);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdAdmin														 */
/* 内容：管理人プレイヤーにコマンド送信										 */
/* 日付：2003/10/25															 */
/* ========================================================================= */
void SendCmdAdmin(PBYTE pData, DWORD dwSize)
{
	int i;
	std::vector<DWORD> aIDDst;

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar->byAdminLV < ADMINLV_ADMINISTRATOR) {
			continue;
		}
		if (pChar->pID == 0) {
			continue;
		}
		aIDDst.push_back (pChar->pID);
	}
	if (!aIDDst.empty ()) {
		svrSock.SendToMulti (&aIDDst[0], (DWORD)aIDDst.size (), pData, dwSize, 0, URARASOCK_SENDPRIORITY_MIDDLE);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdCONNECT													 */
/* 内容：コマンド送信(接続応答)												 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void SendCmdCONNECT(DWORD pIDSend, BYTE byData)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_CONNECT;
	pSendData->pID = pIDSend;
	pSendData->byData = byData;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLOGINRES													 */
/* 内容：コマンド送信(ログイン応答)											 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void SendCmdLOGINRES(DWORD pIDSend, DWORD dwTblID, BYTE byData)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_LOGINRES;
	pSendData->dwTblID = dwTblID;
	pSendData->byData = byData;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGSTATE													 */
/* 内容：コマンド送信(状態遷移)												 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void SendCmdCHGSTATE(DWORD pIDSend, DWORD dwTblID, BYTE byData)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_CHGSTATE;
	pSendData->pID = pIDSend;
	pSendData->dwTblID = dwTblID;
	pSendData->byData = byData;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPINFO													 */
/* 内容：コマンド送信(マップ情報通知)										 */
/* 日付：2003/05/04															 */
/* ========================================================================= */
void SendCmdMAPINFO(DWORD pIDSend, DWORD dwID)
{
	int i, j, nNo;
	LPBYTE pSendData, pSendTmp;
	PMAPINFO pMapSend;
	PMAPINFO pMap;
	DWORD dwSendSize;

	nNo = GetMapNo (dwID);
	if (nNo < 0) {
		return;
	}
	pMap = (PMAPINFO)g_aMapInfo[nNo];
	dwSendSize = sizeof (CMDPARA_MAPINFO) + (pMap->nCX * pMap->nCY * sizeof (WORD));

	/* 店がある？ */
	if (pMap->byCountShop) {
		dwSendSize += (sizeof (SHOPINFO) * pMap->byCountShop);
		for (i = 0; i < pMap->byCountShop; i ++) {
			PSHOPINFO pShop = &pMap->pShopInfo[i];
			for (j = 0; j < MAX_SHOPMSG; j ++) {
				if (pShop->apMsg[j]) {
					dwSendSize += (strlen (pShop->apMsg[j]) + 1);
				} else {
					dwSendSize ++;
				}
			}
		}
	}

	if (pMap->pszName) {
		dwSendSize += (strlen ((LPSTR)pMap->pszName) + 1);
	}

	dwSendSize ++;
	pSendData = new BYTE[dwSendSize];
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_MAPINFO)pSendData)->byCmd = CMD_MAPINFO;
	pMapSend = &((PCMDPARA_MAPINFO)pSendData)->MapInfo;
	CopyMemory (pMapSend, pMap, sizeof (MAPINFO));
	CopyMemory (pSendData + sizeof (CMDPARA_MAPINFO), pMap->pwMap, (pMap->nCX * pMap->nCY) * sizeof (WORD));

	pSendTmp = pSendData + sizeof (CMDPARA_MAPINFO) + (pMap->nCX * pMap->nCY) * sizeof (WORD);
	if (pMap->byCountShop) {
		/* 店情報をコピー */
		for (i = 0; i < pMap->byCountShop; i ++) {
			PSHOPINFO pShop = &pMap->pShopInfo[i];
			CopyMemory (pSendTmp, pShop, sizeof (SHOPINFO));
			pSendTmp += sizeof (SHOPINFO);
			for (j = 0; j < MAX_SHOPMSG; j ++) {
				if (pShop->apMsg[j]) {
					strcat ((LPSTR)pSendTmp, pShop->apMsg[j]);
					pSendTmp += (strlen (pShop->apMsg[j]) + 1);
				} else {
					pSendTmp ++;
				}
			}
		}
	}

	/* マップ名あり？ */
	if (pMap->pszName) {
		strcat ((LPSTR)pSendTmp, (LPSTR)pMap->pszName);
	}

	svrSock.SendTo (pIDSend, pSendData, dwSendSize, SENDTIMEOUT);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPPARTSINFO												 */
/* 内容：コマンド送信(マップパーツ情報通知)									 */
/* 日付：2003/05/11															 */
/* ========================================================================= */
void SendCmdMAPPARTSINFO(DWORD pIDSend)
{
	LPBYTE pSendData, pData;
	PMAPPARTSINFO pParts;
	PMAPPARTSINFOSAVE pSend;
	DWORD i, dwSendSize = sizeof (CMDPARA_MAPPARTSINFO) + (sizeof (MAPPARTSINFOSAVE) * g_wCountMapParts);
	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_aMapPartsInfo[i];
		dwSendSize += (pParts->byAnimeCount * sizeof (ANIMEINFO));
	}

	pSendData = new BYTE[dwSendSize];
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_MAPPARTSINFO)pSendData)->byCmd = CMD_MAPPARTSINFO;
	((PCMDPARA_MAPPARTSINFO)pSendData)->wCount = g_wCountMapParts;
	pData = pSendData + sizeof (CMDPARA_MAPPARTSINFO);

	for (i = 0; i < g_wCountMapParts; i ++) {
		pSend = (PMAPPARTSINFOSAVE)pData;
		pParts = &g_aMapPartsInfo[i];
		pSend->byViewType = pParts->byViewType;
		pSend->byHitType = pParts->byHitType;
		pSend->byAnimeType = pParts->byAnimeType;
		pSend->byAnimeCount = pParts->byAnimeCount;
		pSend->wGrpID = pParts->wGrpID;
		pSend->wBackID = pParts->wBackID;
		pSend->pAnimeInfo = pParts->pAnimeInfo;
		pSend->byLevel = pParts->byLevel;
		pSend->byLight = pParts->byLight;
		pData += sizeof (MAPPARTSINFOSAVE);

		if (pParts->pAnimeInfo) {
			CopyMemory (pData, pParts->pAnimeInfo, pParts->byAnimeCount * sizeof (ANIMEINFO));
			pData += (pParts->byAnimeCount * sizeof (ANIMEINFO));
		}
	}

//	svr.SetPriority (1);
	svrSock.SendTo (pIDSend, pSendData, dwSendSize, SENDTIMEOUT);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：IsCharInfoFullRecv												 */
/* 内容：キャラ情報を全部見せてよい相手か（本人か管理者）					 */
/* ========================================================================= */
static BOOL IsCharInfoFullRecv(DWORD pIDRecv, PCHARINFO pChar)
{
	if (pIDRecv == pChar->pID) {
		return TRUE;
	}
	int nRecv = GetCharNo (pIDRecv);
	if ((nRecv >= 0) && (g_aCharInfo[nRecv]->byAdminLV != ADMINLV_USER)) {
		return TRUE;
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：MaskCharInfoForOther												 */
/* 内容：他人に送るキャラ情報から、画面に要らない項目を消す					 */
/*       （ログイン回数・プレイ時間・作成日・復活地点、NPC の発生範囲・		 */
/*         発生数・発生間隔・移動確率・索敵範囲・ドロップ率など）。		 */
/*       クライアントでこれらを使うのは、本人のログイン情報と管理者画面だけ */
/* ========================================================================= */
static void MaskCharInfoForOther(PCMDPARA_CHARINFO pSendData)
{
	pSendData->wMakeYear			= 0;
	pSendData->wMakeMonth			= 0;
	pSendData->wMakeDay				= 0;
	pSendData->dwStartMapID			= 0;
	pSendData->dwStartX				= 0;
	pSendData->dwStartY				= 0;
	pSendData->dwLoginCount			= 0;
	pSendData->dwPlayTime			= 0;
	pSendData->dwStTmplateID		= (DWORD)-1;
	pSendData->byAreaX				= 0;
	pSendData->byAreaY				= 0;
	pSendData->byPutCount			= 0;
	pSendData->wTimePutInterval		= 0;
	pSendData->byDropProbability	= 0;
	pSendData->byMoveProbabilityX	= 0;
	pSendData->byMoveProbabilityY	= 0;
	pSendData->bySearchCx			= 0;
	pSendData->bySearchCy			= 0;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARINFO													 */
/* 内容：コマンド送信(キャラ情報通知)										 */
/* 日付：2003/05/12															 */
/* ========================================================================= */
void SendCmdCHARINFO(DWORD pIDSend, int nNo)
{
	PCMDPARA_CHARINFO pSendData;
	PBYTE pSendData2 = NULL;
	PCHARINFO pChar;
	int i, nCount = 1, nTmp = 0;
	BOOL bSendAll = FALSE;

	pSendData = new CMDPARA_CHARINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARINFO));

	pSendData->byCmd = CMD_CHARINFO;

	/* 全キャラ分の情報を送信 */
	if (nNo < 0) {
		nCount = g_aCharInfo.GetSize ();
		nNo = GetCharNo (pIDSend);
		if (nNo < 0) {
			delete pSendData;
			return;
		}
		bSendAll = TRUE;

	} else {
		nTmp = nNo;
		if (nTmp >= g_aCharInfo.GetSize ()) {
			delete pSendData;
			return;
		}
	}

	/* 全キャラ分を送る時は、受け取る本人の情報を先頭で送る。
	   クライアントはキャラ情報を受け取るたびに「自分から遠いキャラ」を消すので、
	   ワープ直後に本人より前の番号のキャラ（NPC など）が先に届くと、
	   ワープ前の自分の位置で判定されて、近くにいるのに消されてしまうため */
	for (int j = (bSendAll ? -1 : 0); j < nCount; j ++) {
		i = j;
		if (bSendAll) {
			if (j < 0) {
				i = nNo;				/* 本人 */
			} else if (j == nNo) {
				continue;				/* 本人は先に送った */
			}
		}
		if (g_aCharInfo[i + nTmp]->dwMapID == g_aCharInfo[nNo]->dwMapID) {
			if (abs (g_aCharInfo[i + nTmp]->dwX - g_aCharInfo[nNo]->dwX) <= DRAW_PARTS_X + 2) {
				if (abs (g_aCharInfo[i + nTmp]->dwY - g_aCharInfo[nNo]->dwY) <= DRAW_PARTS_Y + 2) {
					if (bSendAll) {
						pChar = g_aCharInfo[i + nTmp];

					} else {
						pChar = g_aCharInfo[nNo];
					}
					if ((pChar->byGameState < GAMESTATE_MOVE) && (pIDSend != pChar->pID)) {
						/* ログオフ中のキャラ情報は送らない */
						continue;
					}

					pSendData->pID					= pChar->pID;
					pSendData->dwTblID				= pChar->dwTblID;
					pSendData->byAdminLV			= pChar->byAdminLV;
					pSendData->byCharState			= pChar->byCharState;
					pSendData->byDirection			= pChar->byDirection;
					pSendData->byRoofLV				= pChar->byRoofLV;
					pSendData->byEfect				= pChar->byEfect;
					pSendData->byEfectIdle			= pChar->byEfectIdle;
					pSendData->byNpcType			= pChar->byNpcType;
					pSendData->byDropProbability	= pChar->byDropProbability;
					pSendData->byMoveProbabilityX	= pChar->byMoveProbabilityX;
					pSendData->byMoveProbabilityY	= pChar->byMoveProbabilityY;
					pSendData->bySearchCx			= pChar->bySearchCx;
					pSendData->bySearchCy			= pChar->bySearchCy;
					pSendData->byLightLevel			= pChar->byLightLevel;
					pSendData->wGrpID				= pChar->wGrpID;
					pSendData->wGrpMain				= pChar->wGrpMain;
					pSendData->wGrpSub				= pChar->wGrpSub;
					pSendData->wGrpTmpID			= pChar->wGrpTmpID;
					pSendData->wGrpTmpMain			= pChar->wGrpTmpMain;
					pSendData->wGrpTmpSub			= pChar->wGrpTmpSub;
					pSendData->wMaxHP				= pChar->wMaxHP;
					pSendData->wHP					= pChar->wHP;
					pSendData->wMaxMana				= pChar->wMaxMana;
					pSendData->wMana				= pChar->wMana;
					pSendData->wMakeYear			= pChar->wMakeYear;
					pSendData->wMakeMonth			= pChar->wMakeMonth;
					pSendData->wMakeDay				= pChar->wMakeDay;
					pSendData->dwMapID				= pChar->dwMapID;
					pSendData->dwStartMapID			= pChar->dwStartMapID;
					pSendData->dwStartX				= pChar->dwStartX;
					pSendData->dwStartY				= pChar->dwStartY;
					pSendData->dwX					= pChar->dwX;
					pSendData->dwY					= pChar->dwY;
					pSendData->dwLoginCount			= pChar->dwLoginCount;
					pSendData->dwPlayTime			= pChar->dwPlayTime;
					pSendData->dwStTmplateID		= pChar->dwStTmplateID;
					pSendData->dwTailTblID			= pChar->dwTailTblID;
					pSendData->dwTailBackTblID		= pChar->dwTailBackTblID;
					pSendData->byAreaX				= pChar->byAreaX;
					pSendData->byAreaY				= pChar->byAreaY;
					pSendData->byPutCount			= pChar->byPutCount;
					pSendData->wTimePutInterval		= pChar->wTimePutInterval;
					pSendData->pszNpcSpeech			= pChar->pszNpcSpeech;
					pSendData->clName				= pChar->clName;
					pSendData->clSpeech				= pChar->clSpeech;
					CopyMemory (pSendData->szName, pChar->szName, sizeof (pSendData->szName));

					/* 本人と管理者以外には、画面に要らない項目を送らない */
					if (!IsCharInfoFullRecv (pIDSend, pChar)) {
						MaskCharInfoForOther (pSendData);
					}

					/* 名前は可変長に詰めて送る（後ろに NPC 発言） */
					{
						int nSpeech = pChar->pszNpcSpeech ? (int)strlen (pChar->pszNpcSpeech) + 1 : 0;
						int nSize = (int)SboPackSizeCHARINFO (pSendData) + nSpeech;
						pSendData2 = new BYTE[nSize];
						PBYTE pPos = SboPackPutCHARINFO (pSendData2, pSendData);
						if (nSpeech) {
							CopyMemory (pPos, pChar->pszNpcSpeech, nSpeech);
						}
						svrSock.SendTo (pIDSend, pSendData2, nSize, SENDTIMEOUT);
						SAFE_DELETE_ARRAY (pSendData2);
					}
				}
			}
		}
	}

	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARINFO2													 */
/* 内容：コマンド送信(キャラ情報通知2)										 */
/* 日付：2003/06/17															 */
/* ========================================================================= */
void SendCmdCHARINFO2(DWORD pIDSend, DWORD dwID, int nNo)
{
	PCMDPARA_CHARINFO2 pSendData;
	PBYTE pSendData2 = NULL;
	PCHARINFO pChar;
	int i, nCount = 1, nTmp = 0;
	BOOL bSendAll = FALSE;

	pSendData = new CMDPARA_CHARINFO2;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARINFO2));

	pSendData->byCmd = CMD_CHARINFO2;

	if (nNo < 0) {
		nCount = g_aCharInfo.GetSize ();
		nNo = dwID;
		bSendAll = TRUE;

	} else {
		nTmp = nNo;
		if (nTmp >= g_aCharInfo.GetSize ()) {
			delete pSendData;
			return;
		}
	}

	for (i = 0; i < nCount; i ++) {
		if (i + nTmp >= g_aCharInfo.GetSize ()) {
			continue;
		}
		if (g_aCharInfo[i + nTmp]->dwMapID == g_aCharInfo[nNo]->dwMapID) {
			if (abs (g_aCharInfo[i + nTmp]->dwX - g_aCharInfo[nNo]->dwX) <= DRAW_PARTS_X + 2) {
				if (abs (g_aCharInfo[i + nTmp]->dwY - g_aCharInfo[nNo]->dwY) <= DRAW_PARTS_Y + 2) {
					if (bSendAll) {
						pChar = g_aCharInfo[i + nTmp];

					} else {
						pChar = g_aCharInfo[nNo];
					}
					if ((pChar->byGameState < GAMESTATE_MOVE) && (pIDSend != pChar->pID)) {
						/* ログオフ中のキャラ情報は送らない */
						continue;
					}

					pSendData->pID				= pChar->pID;
					pSendData->dwTblID			= pChar->dwTblID;
					pSendData->dwMapID			= pChar->dwMapID;
					pSendData->byAdminLV		= pChar->byAdminLV;
					pSendData->byCharState		= pChar->byCharState;
					pSendData->byDirection		= pChar->byDirection;
					pSendData->byRoofLV			= pChar->byRoofLV;
					pSendData->byEfect			= pChar->byEfect;
					pSendData->byEfectIdle		= pChar->byEfectIdle;
					pSendData->byNpcType		= pChar->byNpcType;
					pSendData->byLightLevel		= pChar->byLightLevel;
					pSendData->wGrpID			= pChar->wGrpID;
					pSendData->wGrpMain			= pChar->wGrpMain;
					pSendData->wGrpSub			= pChar->wGrpSub;
					pSendData->wGrpTmpID		= pChar->wGrpTmpID;
					pSendData->wGrpTmpMain		= pChar->wGrpTmpMain;
					pSendData->wGrpTmpSub		= pChar->wGrpTmpSub;
					pSendData->wMaxHP			= pChar->wMaxHP;
					pSendData->wHP				= pChar->wHP;
					pSendData->wMaxMana			= pChar->wMaxMana;
					pSendData->wMana			= pChar->wMana;
					pSendData->dwX				= pChar->dwX;
					pSendData->dwY				= pChar->dwY;
					pSendData->pszNpcSpeech		= pChar->pszNpcSpeech;
					pSendData->clName			= pChar->clName;
					pSendData->clSpeech			= pChar->clSpeech;
					CopyMemory (pSendData->szName, pChar->szName, sizeof (pSendData->szName));

					/* 名前は可変長に詰めて送る（後ろに NPC 発言） */
					{
						int nSpeech = pChar->pszNpcSpeech ? (int)strlen (pChar->pszNpcSpeech) + 1 : 0;
						int nSize = (int)SboPackSizeCHARINFO2 (pSendData) + nSpeech;
						pSendData2 = new BYTE[nSize];
						PBYTE pPos = SboPackPutCHARINFO2 (pSendData2, pSendData);
						if (nSpeech) {
							CopyMemory (pPos, pChar->pszNpcSpeech, nSpeech);
						}
						svrSock.SendTo (pIDSend, pSendData2, nSize, SENDTIMEOUT);
						SAFE_DELETE_ARRAY (pSendData2);
					}
				}
			}
		}
	}

	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdInsideCHARINFO											 */
/* 内容：同画面内のプレイヤーにキャラ情報を送る							 */
/*       （本人と管理者には全部入り、他の人には項目を消したもの）			 */
/*       画面内の判定は SendCmdInside2 (SENDINSIDE_INSIDE) と同じ			 */
/* ========================================================================= */
static void SendCmdInsideCHARINFO(PCHARINFO pChar, PBYTE pFull, int nFull, PBYTE pMask, int nMask)
{
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	std::vector<DWORD> aIDFull, aIDMask;

	if (pData == NULL) {
		return;
	}
	for (int i = 0; pData[i] != -1; i ++) {
		PCHARINFO pCharDst = g_aCharInfo[pData[i]];
		if (pCharDst->byGameState < GAMESTATE_MOVE) {
			continue;
		}
		int x = abs (pCharDst->dwX - pChar->dwX);
		int y = abs (pCharDst->dwY - pChar->dwY);
		if (x > DRAW_PARTS_X + 3) {
			continue;
		}
		if (y > DRAW_PARTS_Y + 3) {
			continue;
		}
		if ((pCharDst->pID == pChar->pID) || (pCharDst->byAdminLV != ADMINLV_USER)) {
			aIDFull.push_back (pCharDst->pID);
		} else {
			aIDMask.push_back (pCharDst->pID);
		}
	}
	/* 圧縮とCRCを1回で済ませるため、まとめて送る */
	if (!aIDFull.empty ()) {
		svrSock.SendToMulti (&aIDFull[0], (DWORD)aIDFull.size (), pFull, nFull, SENDTIMEOUT, 0);
	}
	if (!aIDMask.empty ()) {
		svrSock.SendToMulti (&aIDMask[0], (DWORD)aIDMask.size (), pMask, nMask, SENDTIMEOUT, 0);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdCHARINFO3													 */
/* 内容：コマンド送信(キャラ情報通知)										 */
/* 日付：2003/10/17															 */
/* ========================================================================= */
void SendCmdCHARINFO3(int nNo)
{
	PCMDPARA_CHARINFO pSendData;
	PCHARINFO pChar;
	PBYTE pSendData2 = NULL;

	pChar = g_aCharInfo[nNo];
	if (pChar->byGameState < GAMESTATE_MOVE) {
		/* ログオフ中のキャラ情報は送らない */
		return;
	}

	pSendData = new CMDPARA_CHARINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARINFO));

	pSendData->byCmd				= CMD_CHARINFO;
	pSendData->pID					= pChar->pID;
	pSendData->dwTblID				= pChar->dwTblID;
	pSendData->byAdminLV			= pChar->byAdminLV;
	pSendData->byCharState			= pChar->byCharState;
	pSendData->byDirection			= pChar->byDirection;
	pSendData->byRoofLV				= pChar->byRoofLV;
	pSendData->byEfect				= pChar->byEfect;
	pSendData->byEfectIdle			= pChar->byEfectIdle;
	pSendData->byNpcType			= pChar->byNpcType;
	pSendData->byDropProbability	= pChar->byDropProbability;
	pSendData->byMoveProbabilityX	= pChar->byMoveProbabilityX;
	pSendData->byMoveProbabilityY	= pChar->byMoveProbabilityY;
	pSendData->bySearchCx			= pChar->bySearchCx;
	pSendData->bySearchCy			= pChar->bySearchCy;
	pSendData->byLightLevel			= pChar->byLightLevel;
	pSendData->wGrpID				= pChar->wGrpID;
	pSendData->wGrpMain				= pChar->wGrpMain;
	pSendData->wGrpSub				= pChar->wGrpSub;
	pSendData->wGrpTmpID			= pChar->wGrpTmpID;
	pSendData->wGrpTmpMain			= pChar->wGrpTmpMain;
	pSendData->wGrpTmpSub			= pChar->wGrpTmpSub;
	pSendData->wMaxHP				= pChar->wMaxHP;
	pSendData->wHP					= pChar->wHP;
	pSendData->wMaxMana				= pChar->wMaxMana;
	pSendData->wMana				= pChar->wMana;
	pSendData->dwMapID				= pChar->dwMapID;
	pSendData->dwStartMapID			= pChar->dwStartMapID;
	pSendData->dwStartX				= pChar->dwStartX;
	pSendData->dwStartY				= pChar->dwStartY;
	pSendData->dwX					= pChar->dwX;
	pSendData->dwY					= pChar->dwY;
	pSendData->dwLoginCount			= pChar->dwLoginCount;
	pSendData->dwPlayTime			= pChar->dwPlayTime;
	pSendData->dwStTmplateID		= pChar->dwStTmplateID;
	pSendData->dwTailTblID			= pChar->dwTailTblID;
	pSendData->dwTailBackTblID		= pChar->dwTailBackTblID;
	pSendData->byAreaX				= pChar->byAreaX;
	pSendData->byAreaY				= pChar->byAreaY;
	pSendData->byPutCount			= pChar->byPutCount;
	pSendData->wTimePutInterval		= pChar->wTimePutInterval;
	pSendData->pszNpcSpeech			= pChar->pszNpcSpeech;
	pSendData->clName				= pChar->clName;
	pSendData->clSpeech				= pChar->clSpeech;
	CopyMemory (pSendData->szName, pChar->szName, sizeof (pSendData->szName));

	/* 名前は可変長に詰めて送る（後ろに NPC 発言） */
	{
		int nSpeech = pChar->pszNpcSpeech ? (int)strlen (pChar->pszNpcSpeech) + 1 : 0;
		int nSize = (int)SboPackSizeCHARINFO (pSendData) + nSpeech;
		pSendData2 = new BYTE[nSize];
		PBYTE pPos = SboPackPutCHARINFO (pSendData2, pSendData);
		if (nSpeech) {
			CopyMemory (pPos, pChar->pszNpcSpeech, nSpeech);
		}
		/* 本人と管理者には全部、それ以外の人には要らない項目を消したものを送る */
		CMDPARA_CHARINFO ParaMask = *pSendData;
		MaskCharInfoForOther (&ParaMask);
		int nSizeMask = (int)SboPackSizeCHARINFO (&ParaMask) + nSpeech;
		PBYTE pSendMask = new BYTE[nSizeMask];
		PBYTE pPosMask = SboPackPutCHARINFO (pSendMask, &ParaMask);
		if (nSpeech) {
			CopyMemory (pPosMask, pChar->pszNpcSpeech, nSpeech);
		}
		SendCmdInsideCHARINFO (pChar, pSendData2, nSize, pSendMask, nSizeMask);
		SAFE_DELETE_ARRAY (pSendMask);
		SAFE_DELETE_ARRAY (pSendData2);
	}

	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGDIRECTION												 */
/* 内容：コマンド送信(向き変更要求)											 */
/* 日付：2003/05/14															 */
/* ========================================================================= */
void SendCmdCHGDIRECTION(DWORD pIDSend, int nNo)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_CHGDIRECTION;
	pSendData->pID = g_aCharInfo[nNo]->pID;
	pSendData->dwTblID = g_aCharInfo[nNo]->dwTblID;
	pSendData->byData = g_aCharInfo[nNo]->byDirection;

	if (pIDSend == URARASOCK_SENDALL) {
		svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);

	} else {
		SendCmdInside2 (g_aCharInfo[nNo], (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT, SENDINSIDE_INSIDENOME, TRUE, TRUE, 2);
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARPOS													 */
/* 内容：コマンド送信(座標通知)												 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void SendCmdCHARPOS(DWORD pIDSend, int nNo, BYTE byFlg, BYTE byInside)
{
	PCMDPARA_CHARPOS pSendData;

	pSendData = (PCMDPARA_CHARPOS)new CMDPARA_CHARPOS;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARPOS));
	pSendData->byCmd = CMD_CHARPOS;
	pSendData->pID = g_aCharInfo[nNo]->pID;
	pSendData->dwTblID = g_aCharInfo[nNo]->dwTblID;
	pSendData->dwMapID = g_aCharInfo[nNo]->dwMapID;
	pSendData->byX = (BYTE)g_aCharInfo[nNo]->dwX;
	pSendData->byY = (BYTE)g_aCharInfo[nNo]->dwY;
	pSendData->byDirection = g_aCharInfo[nNo]->byDirection;
	pSendData->byFlg = byFlg;

//	char szTmp[128];
//	wsprintf (szTmp, "→CHARPOS送信 ID:%d\r\n", pSendData->dwTblID);
//	OutputDebugString (szTmp);

	if (byInside == 3) {
//		DWORD dwNumMsgs;
//		IDirectPlay8Server *pSvr = svr.GetPtr ();
//		pSvr->GetSendQueueInfo (g_aCharInfo[nNo]->pID, &dwNumMsgs, NULL, DPNGETSENDQUEUEINFO_PRIORITY_NORMAL);
//		if (dwNumMsgs < 10) {
			svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_CHARPOS), SENDTIMEOUT);
//		}

	} else if (pIDSend == URARASOCK_SENDALL || !byInside) {
		svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_CHARPOS), SENDTIMEOUT);

	} else if (byInside == 2) {
		SendCmdInsideMap (g_aCharInfo[nNo]->dwMapID, (LPBYTE)pSendData, sizeof (CMDPARA_CHARPOS));

	} else if (byInside == 4) {
		SendCmdInside2 (g_aCharInfo[nNo], (LPBYTE)pSendData, sizeof (CMDPARA_CHARPOS), 0, SENDINSIDE_INSIDENOME, TRUE, TRUE, 2);

	} else {
		SendCmdInside2 (g_aCharInfo[nNo], (LPBYTE)pSendData, sizeof (CMDPARA_CHARPOS), SENDTIMEOUT, SENDINSIDE_INSIDENOME, TRUE, TRUE, 2);
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSPEECH													 */
/* 内容：コマンド送信(発言通知)												 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void SendCmdSPEECH(DWORD pIDSend, int nNo, BYTE byType)
{
	BYTE *pData;
	PCMDPARA_SPEECH pSendData;
	int nSize = strlen (g_aCharInfo[nNo]->pszSpeech) + 1 + sizeof (CMDPARA_SPEECH);

	pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);
	pSendData = (PCMDPARA_SPEECH)pData;
	pSendData->byCmd = CMD_SPEECH;
	pSendData->pID = g_aCharInfo[nNo]->pID;
	pSendData->dwTblID = g_aCharInfo[nNo]->dwTblID;
	pSendData->byType = byType;
	pSendData->wSize = (WORD)(strlen (g_aCharInfo[nNo]->pszSpeech) + 1);
	memcpy (pData + sizeof (CMDPARA_SPEECH), g_aCharInfo[nNo]->pszSpeech,
				strlen (g_aCharInfo[nNo]->pszSpeech) + 1);

	/* 全員に配信？ */
	if (byType == SPEECHCMD_ALL) {
		byType = SENDINSIDE_ALL;
		if (!CheckAdminBan (g_aCharInfo[nNo]->dwTblID)) {
			delete [] pData;
			return;
		}

	/* 管理者用内線？ */
	} else if (byType == SPEECHCMD_GM) {
		if (!CheckAdminBan (g_aCharInfo[nNo]->dwTblID)) {
			delete [] pData;
			return;
		}

		int i;
		PCHARINFO pChar;
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			pChar = g_aCharInfo[i];
			if (pChar->byAdminLV != ADMINLV_USER) {
				if (pChar->byGameState >= GAMESTATE_MOVE) {
					svrSock.SendTo (pChar->pID, (LPBYTE)pSendData, nSize, SENDTIMEOUT, URARASOCK_SENDPRIORITY_HIGH);
				}
			}
		}
		delete [] pData;
		return;

	/* マップ内放送？ */
	} else if (byType == SPEECHCMD_MAP) {
		PMAPINFO pMap = g_aMapInfo[g_aCharInfo[nNo]->dwMapID];
		/* マップ内放送が許可されている？ */
		if (pMap->byFlg & MAPFLG_SPEECH) {
			byType = SENDINSIDE_INSIDEMAP;
			pSendData->color = g_aCharInfo[nNo]->clSpeech;
		} else {
			byType = SENDINSIDE_INSIDE;
		}

	/* 画面内放送？ */
	} else if (byType == SPEECHCMD_SCREENCHAR) {
		PMAPINFO pMap = g_aMapInfo[g_aCharInfo[nNo]->dwMapID];
		byType = SENDINSIDE_INSIDE;
		/* マップ内放送が許可されている？ */
		if (pMap->byFlg & MAPFLG_SPEECH) {
			pSendData->color = g_aCharInfo[nNo]->clSpeech;
		}

	/* 画面内のプレイヤーにだけ配信 */
	} else {
		byType = SENDINSIDE_INSIDE;
	}
	if (byType == SENDINSIDE_ALL) {
		svrSock.SendTo (URARASOCK_SENDALL, (LPBYTE)pSendData, nSize, SENDTIMEOUT, URARASOCK_SENDPRIORITY_HIGH);

	} else if (byType == SENDINSIDE_INSIDEMAP) {
		SendCmdInsideMap (g_aCharInfo[nNo]->dwMapID, (LPBYTE)pSendData, nSize);

	} else {
		SendCmdInside2 (g_aCharInfo[nNo], (LPBYTE)pSendData, nSize, SENDTIMEOUT, byType, 1);
	}
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdONLINE													 */
/* 内容：コマンド送信(オンライン数通知)										 */
/* 日付：2003/05/25															 */
/* ========================================================================= */
void SendCmdONLINE(DWORD pIDSend)
{
	PCMDPARA_ONLINE pSendData;

	pSendData = new CMDPARA_ONLINE;
	pSendData->byCmd = CMD_ONLINE;
	pSendData->byAdmin = g_byAdminCount;
	pSendData->dwData = g_dwOnline;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_ONLINE), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPPARTS													 */
/* 内容：コマンド送信(マップパーツ更新)										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdMAPPARTS(DWORD pIDSend, int nNo)
{
	LPBYTE pSendData;
	PMAPPARTSINFO pParts = &g_aMapPartsInfo[nNo];
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
	pSend->wGrpID = pParts->wGrpID;
	pSend->wBackID = pParts->wBackID;
	pSend->pAnimeInfo = pParts->pAnimeInfo;
	pSend->byLevel = pParts->byLevel;
	pSend->byLight = pParts->byLight;
	if (pParts->pAnimeInfo) {
		CopyMemory (pSendData + sizeof (CMDPARA_MAPPARTSINFO) + sizeof (MAPPARTSINFOSAVE),
				 pParts->pAnimeInfo, pParts->byAnimeCount * sizeof (ANIMEINFO));
	}

	svrSock.SendTo (pIDSend, pSendData, dwSendSize, SENDTIMEOUT);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGMAP													 */
/* 内容：コマンド送信(マップ書き換え)										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void SendCmdCHGMAP(DWORD dwID, WORD wMap, int nX, int nY)
{
	PCMDPARA_CHGMAP pSendData;

	pSendData = new CMDPARA_CHGMAP;
	pSendData->byCmd = CMD_CHGMAP;
	pSendData->dwID = dwID;
	pSendData->wMap = wMap;
	pSendData->nX = nX;
	pSendData->nY = nY;
	SendCmdInsideMap (dwID, (LPBYTE)pSendData, sizeof (CMDPARA_CHGMAP));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDDELMAP													 */
/* 内容：コマンド送信(マップ幅増減要求)										 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void SendCmdADDDELMAP(DWORD dwID, BYTE byType, BYTE byCount)
{
	PCMDPARA_ADDDELMAP pSendData;

	pSendData = new CMDPARA_ADDDELMAP;
	pSendData->byCmd = CMD_ADDDELMAP;
	pSendData->dwID = dwID;
	pSendData->byType = byType;
	pSendData->byCount = byCount;

	SendCmdInsideMap (dwID, (LPBYTE)pSendData, sizeof (CMDPARA_ADDDELMAP));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTINFO													 */
/* 内容：コマンド送信(イベント情報更新)										 */
/* 日付：2003/06/02															 */
/* ========================================================================= */
void SendCmdEVENTINFO(DWORD pIDSend, int nMapNo, int nDataNo)
{
	PCMDPARA_CHGEVENT pSendData;
	PMAPINFO pMap;
	PEVENTINFO pEvent;

	/* マップが存在しない？ */
	if (nMapNo >= g_aMapInfo.GetSize ()) {
		return;
	}
	pMap = g_aMapInfo[nMapNo];

	/* イベントが存在しない？ */
	if (nDataNo >= pMap->nEventCount) {
		return;
	}
	pEvent = &pMap->pEvent[nDataNo];

	/* イベント情報が無い？ */
	if (pEvent == NULL) {
		return;
	}

	int nSize, nDataSize;
	nSize = sizeof (CMDPARA_CHGEVENT);
	nDataSize = 0;

	if (pEvent->pData) {
		nDataSize = GetEventDataSize (pEvent->byType);
		nSize += nDataSize;
	}

	pSendData = (PCMDPARA_CHGEVENT)new BYTE [nSize];
	pSendData->byCmd = CMD_CHGEVENT;
	pSendData->dwID = g_aMapInfo[nMapNo]->dwID;
	pSendData->dwX = pEvent->dwX;
	pSendData->dwY = pEvent->dwY;
	pSendData->byType = pEvent->byType;
	pSendData->pData = pEvent->pData;
	pSendData->dwDataID = (DWORD)nDataNo;
	if (pEvent->pData) {
		CopyMemory (pSendData + 1, pEvent->pData, nDataSize);
	}

	if (!pIDSend) {
		SendCmdInsideMap (g_aMapInfo[nMapNo]->dwID, (LPBYTE)pSendData, nSize);

	} else {
		svrSock.SendTo (pIDSend, (PBYTE)pSendData, nSize, SENDTIMEOUT);
	}
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTCOUNT												 */
/* 内容：コマンド送信(イベント数通知)										 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void SendCmdEVENTCOUNT(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_DATA2 pSendData;

	pSendData = new CMDPARA_DATA2;
	pSendData->byCmd = CMD_COUNTEVENT;
	pSendData->nData = g_aMapInfo[dwID]->nEventCount;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA2), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEVENTPOS													 */
/* 内容：コマンド送信(イベント座標通知)										 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void SendCmdEVENTPOS(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_EVENTPOS pSendData;
	int i, nSize = g_aMapInfo[dwID]->nEventCount * sizeof (XYINFO);
	PXYINFO pData;
	PEVENTINFO pEvent;
	if (!nSize) {
		return;
	}

	nSize += sizeof (CMDPARA_EVENTPOS);
	pSendData = (PCMDPARA_EVENTPOS)new BYTE[nSize];
	pSendData->byCmd = CMD_EVENTPOS;
	pSendData->dwCount = g_aMapInfo[dwID]->nEventCount;
	pData = (PXYINFO)((PBYTE)pSendData + sizeof (CMDPARA_EVENTPOS));

	for (i = 0; i < g_aMapInfo[dwID]->nEventCount; i ++) {
		pEvent = &g_aMapInfo[dwID]->pEvent[i];
		pData->dwX = pEvent->dwX;
		pData->dwY = pEvent->dwY;
		pData ++;
	}

	if (pIDSend == URARASOCK_SENDALL) {
		SendCmdInsideMap (g_aMapInfo[dwID]->dwID, (LPBYTE)pSendData, nSize);

	} else {
		svrSock.SendTo (pIDSend, (PBYTE)pSendData, nSize, SENDTIMEOUT);
	}
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdHARTBEAT													 */
/* 内容：コマンド送信(生存確認応答)											 */
/* 日付：2003/06/17															 */
/* ========================================================================= */
void SendCmdHARTBEAT(DWORD pIDSend)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_HARTBEAT;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT, URARASOCK_SENDPRIORITY_HIGH);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGBGM													 */
/* 内容：コマンド送信(BGM変更)												 */
/* 日付：2003/07/13															 */
/* ========================================================================= */
void SendCmdCHGBGM(DWORD pIDSend, DWORD dwMapID, BYTE byNo)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_CHGBGM;
	pSendData->byData = byNo;

	if (pIDSend == URARASOCK_SENDALL) {
		SendCmdInsideMap (dwMapID, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1));

	} else {
//		svr.SetPriority (1);
		svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMSG														 */
/* 内容：コマンド送信(メッセージ更新)										 */
/* 日付：2003/07/15															 */
/* ========================================================================= */
void SendCmdMSG(DWORD dwMapID, DWORD dwNo)
{
	PCMDPARA_DATA1 pSendData;
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	LPSTR pszMsg = NULL;
	CopyMemory (&pszMsg, &pMap->pMsg[dwNo], sizeof (LPSTR));
	int nSize = sizeof (CMDPARA_DATA1) + strlen (pszMsg) + 1;

	PBYTE pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);

	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_MSG;
	pSendData->dwTblID = dwNo;
	CopyMemory (pData + sizeof (CMDPARA_DATA1), pszMsg, strlen (pszMsg) + 1);
	SendCmdInsideMap (dwMapID, pData, nSize);

	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdVIEWMSG													 */
/* 内容：コマンド送信(メッセージ表示)										 */
/* 日付：2003/07/16															 */
/* ========================================================================= */
void SendCmdVIEWMSG(DWORD pIDSend, DWORD dwNo)
{
	PCMDPARA_DATA3 pSendData;

	pSendData = (PCMDPARA_DATA3)new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_VIEWMSG;
	pSendData->dwData = dwNo;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA3), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdPLAYSOUND													 */
/* 内容：コマンド送信(効果音再生)											 */
/* 日付：2003/07/20															 */
/* ========================================================================= */
void SendCmdPLAYSOUND(DWORD pIDSend, BYTE byNo)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_PLAYSOUND;
	pSendData->byData = byNo;
	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdPLAYSOUNDinside											 */
/* 内容：画面内のキャラにコマンド送信(効果音再生)							 */
/* 日付：2003/11/10															 */
/* ========================================================================= */
void SendCmdPLAYSOUNDinside(DWORD dwID, BYTE byNo)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_PLAYSOUND;
	pSendData->byData = byNo;
	SendCmdInside2 (g_aCharInfo[dwID], (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT, SENDINSIDE_INSIDE);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPMSG													 */
/* 内容：コマンド送信(メッセージ通知)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void SendCmdMAPMSG(DWORD pIDSend, LPSTR lpStr)
{
	PCMDPARA_DATA3 pSendData;
	PBYTE pData;
	DWORD dwSize = sizeof (CMDPARA_DATA3) + strlen (lpStr) + 1;

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_DATA3)pData;
	pSendData->byCmd = CMD_MAPMSG;
	pSendData->dwData = strlen (lpStr);
	CopyMemory (pSendData + 1, lpStr, pSendData->dwData);

	/* アイテム取得などの制御コードが入っていたら、頼まれた時のために覚えておく */
	RecvCmdOnSendMsg (pIDSend, lpStr);

	svrSock.SendTo (pIDSend, pData, dwSize, SENDTIMEOUT);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPMSGREQ													 */
/* 内容：コマンド送信(メッセージ通知)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void SendCmdMAPMSGREQ(DWORD pIDSend, LPSTR lpStr)
{
	PCMDPARA_DATA3 pSendData;
	PBYTE pData;
	DWORD dwSize = sizeof (CMDPARA_DATA3) + strlen (lpStr) + 1;

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_DATA3)pData;
	pSendData->byCmd = CMD_MAPMSGREQ;
	pSendData->dwData = strlen (lpStr);
	CopyMemory (pSendData + 1, lpStr, pSendData->dwData);

	svrSock.SendTo (pIDSend, pData, dwSize, SENDTIMEOUT);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdSYSMSG													 */
/* 内容：コマンド送信(システムメッセージ)									 */
/* 日付：2003/08/03															 */
/* ========================================================================= */
void SendCmdSYSMSG(DWORD pIDSend, LPCTSTR pszStr, BYTE byType)
{
	BYTE *pData;
	PCMDPARA_SPEECH pSendData;
	int nSize = strlen (pszStr) + 1 + sizeof (CMDPARA_SPEECH);

	pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);
	pSendData = (PCMDPARA_SPEECH)pData;
	pSendData->byCmd = CMD_SYSMSG;
	pSendData->wSize = strlen (pszStr) + 1;
	pSendData->byType = byType;
	memcpy (pData + sizeof (CMDPARA_SPEECH), pszStr, strlen (pszStr) + 1);

	if (byType == SPEECHCMD_SCREEN) {
		SendCmdInside2 (g_aCharInfo[pIDSend], pData, nSize, SENDTIMEOUT, SENDINSIDE_INSIDE);
	} else {
		svrSock.SendTo (pIDSend, pData, nSize, SENDTIMEOUT);
	}
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdSYSMSGMAP													 */
/* 内容：コマンド送信(マップ内システムメッセージ)							 */
/* 日付：2004/01/31															 */
/* ========================================================================= */
void SendCmdSYSMSGMAP(DWORD dwMapID, LPSTR pszStr, BYTE byType)
{
	BYTE *pData;
	PCMDPARA_SPEECH pSendData;
	int nSize = strlen (pszStr) + 1 + sizeof (CMDPARA_SPEECH);

	pData = new BYTE[nSize];
	ZeroMemory (pData, nSize);
	pSendData = (PCMDPARA_SPEECH)pData;
	pSendData->byCmd = CMD_SYSMSG;
	pSendData->wSize = strlen (pszStr) + 1;
	pSendData->byType = byType;
	memcpy (pData + sizeof (CMDPARA_SPEECH), pszStr, strlen (pszStr) + 1);

	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pTbl = pMap->paTblNoPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pTbl[i] == -1) {
			break;
		}
		PCHARINFO pChar = g_aCharInfo[pTbl[i]];
		svrSock.SendTo (pChar->pID, pData, nSize, SENDTIMEOUT);
	}
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdEFECTCHG													 */
/* 内容：コマンド送信(エフェクト変更)										 */
/* 日付：2003/08/11															 */
/* ========================================================================= */
void SendCmdEFECTCHG(DWORD pIDSend, DWORD dwTblID, BYTE byNo, BOOL bIdle)
{
	PCMDPARA_DATA4 pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];

	pSendData = (PCMDPARA_DATA4)new CMDPARA_DATA4;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA4));
	pSendData->byCmd = CMD_EFECTCHG;
	pSendData->pID = pChar->pID;
	pSendData->byData = byNo;
	pSendData->dwData = (DWORD)bIdle;
	SendCmdInside2 (pChar, (LPBYTE)pSendData, sizeof (CMDPARA_DATA4), SENDTIMEOUT, SENDINSIDE_INSIDE);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCOMEFC													 */
/* 内容：コマンド送信(吹き出し通知)											 */
/* 日付：2003/08/12															 */
/* ========================================================================= */
void SendCmdCOMEFC(PCHARINFO pChar)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_COMEFC;
	pSendData->pID = pChar->pID;
	pSendData->byData = pChar->byComEfc;
	SendCmdInside2 (pChar, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT, SENDINSIDE_INSIDE);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCOMEFC													 */
/* 内容：コマンド送信(エラー通知)											 */
/* 日付：2003/08/14															 */
/* ========================================================================= */
void SendCmdERR(DWORD pIDSend, BYTE byErr)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_ERR;
	pSendData->pID = pIDSend;
	pSendData->byData = byErr;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdGETACCINFO												 */
/* 内容：コマンド送信(アカウント情報要求)									 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void SendCmdGETACCINFO(DWORD pIDSend, DWORD dwTblID)
{
	PCMDPARA_CHARINFO pSendData;
	PCHARINFO pChar;

	pSendData = new CMDPARA_CHARINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARINFO));
	pSendData->byCmd = CMD_GETACCINFO;
	pSendData->pID = pIDSend;

	pChar = g_aCharInfo[dwTblID];

	pSendData->byAccState			= pChar->byAccState;
	pSendData->byCharState			= pChar->byCharState;
	pSendData->byAdminLV			= pChar->byAdminLV;
	pSendData->byDirection			= pChar->byDirection;
	pSendData->byNpcType			= pChar->byNpcType;
	pSendData->byAreaX				= pChar->byAreaX;
	pSendData->byAreaY				= pChar->byAreaY;
	pSendData->byPutCount			= pChar->byPutCount;
	pSendData->byDropProbability	= pChar->byDropProbability;
	pSendData->byMoveProbabilityX	= pChar->byMoveProbabilityX;
	pSendData->byMoveProbabilityY	= pChar->byMoveProbabilityY;
	pSendData->bySearchCx			= pChar->bySearchCx;
	pSendData->bySearchCy			= pChar->bySearchCy;
	pSendData->dwTblID				= pChar->dwTblID;
	pSendData->wGrpID				= pChar->wGrpID;
	pSendData->wGrpMain				= pChar->wGrpMain;
	pSendData->wGrpSub				= pChar->wGrpSub;
	pSendData->wGrpTmpID			= pChar->wGrpTmpID;
	pSendData->wGrpTmpMain			= pChar->wGrpTmpMain;
	pSendData->wGrpTmpSub			= pChar->wGrpTmpSub;
	pSendData->wTimePutInterval		= pChar->wTimePutInterval;
	pSendData->dwMapID				= pChar->dwMapID;
	pSendData->dwStartMapID			= pChar->dwStartMapID;
	pSendData->dwStartX				= pChar->dwStartX;
	pSendData->dwStartY				= pChar->dwStartY;
	pSendData->dwX					= pChar->dwX;
	pSendData->dwY					= pChar->dwY;
	pSendData->dwLoginCount			= pChar->dwLoginCount;
	pSendData->dwPlayTime			= pChar->dwPlayTime;
	pSendData->dwStTmplateID		= pChar->dwStTmplateID;
	pSendData->dwLastIPAddress		= pChar->dwLastIPAddress;
	pSendData->clName				= pChar->clName;
	pSendData->clSpeech				= pChar->clSpeech;

	CopyMemory (pSendData->szName, pChar->szName, sizeof (pSendData->szName));
	CopyMemory (pSendData->szAcc, pChar->szAcc, MAX_ACCNAME);
	CopyMemory (pSendData->szPas, pChar->szPas, MAX_ACCNAME);
	CopyMemory (pSendData->szMacAdr, pChar->szMacAdr, 6);

	/* 名前は可変長に詰めて送る */
	{
		DWORD dwSize = SboPackSizeCHARINFO (pSendData);
		PBYTE pPack = new BYTE[dwSize];
		SboPackPutCHARINFO (pPack, pSendData);
		svrSock.SendTo (pIDSend, pPack, dwSize, SENDTIMEOUT);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGCEF													 */
/* 内容：コマンド送信(吹き出し情報更新)										 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void SendCmdCHGCEF(DWORD pIDSend, WORD wNo)
{
	PBYTE pSendData;
	PCOMEFCINFO pComEfc = &g_pComEfcInfo[wNo];
	PCOMEFCINFO pSend;
	DWORD dwSendSize = sizeof (CMDPARA_CHGCEF) + sizeof (COMEFCINFO);
	dwSendSize += (pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));

	pSendData = new BYTE[dwSendSize];
	/* 構造体のすき間も 0 にしておく（「元に戻す」が送信データで同じ状態かを比べるので） */
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_CHGCEF)pSendData)->byCmd = CMD_CHGCEF;
	((PCMDPARA_CHGCEF)pSendData)->wCount = wNo;
	pSend = (PCOMEFCINFO)(pSendData + sizeof (CMDPARA_CHGCEF));
	pSend->byAnimeCount = pComEfc->byAnimeCount;
	pSend->pAnime = pComEfc->pAnime;
	pSend->bySound = pComEfc->bySound;
	if (pSend->pAnime) {
		CopyMemory (pSendData + sizeof (CMDPARA_CHGCEF) + sizeof (COMEFCINFO),
				 pComEfc->pAnime, pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
	}

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, dwSendSize, SENDTIMEOUT);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCEFINFO													 */
/* 内容：コマンド送信(吹き出し情報通知)										 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void SendCmdCEFINFO(DWORD pIDSend)
{
	PBYTE pSendData, pSendTmp;
	PCOMEFCINFO pComEfc;
	PCOMEFCINFO pSend;
	int i;
	DWORD dwSendSize = sizeof (CMDPARA_CHGCEF) + (sizeof (COMEFCINFO) * 99);
	for (i = 0; i < 99; i ++) {
		pComEfc = &g_pComEfcInfo[i];
		dwSendSize += (pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
	}

	pSendData = new BYTE[dwSendSize];
	ZeroMemory (pSendData, dwSendSize);
	((PCMDPARA_CHGCEF)pSendData)->byCmd = CMD_CEFINFO;
	((PCMDPARA_CHGCEF)pSendData)->wCount = 99;
	pSendTmp = pSendData + sizeof (CMDPARA_CHGCEF);

	for (i = 0; i < 99; i ++) {
		pComEfc = &g_pComEfcInfo[i];
		pSend = (PCOMEFCINFO)pSendTmp;
		pSend->byAnimeCount = pComEfc->byAnimeCount;
		pSend->pAnime = pComEfc->pAnime;
		pSend->bySound = pComEfc->bySound;
		if (pSend->byAnimeCount) {
			CopyMemory (pSendTmp + sizeof (COMEFCINFO),
					pComEfc->pAnime, pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
			pSendTmp += (pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
		}
		pSendTmp += sizeof (COMEFCINFO);
	}

//	svr.SetPriority (1);
	svrSock.SendTo (pIDSend, (PBYTE)pSendData, dwSendSize, SENDTIMEOUT);
	delete [] pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdFADEINOUT													 */
/* 内容：コマンド送信(フェードイン・アウト)									 */
/* 日付：2003/09/11															 */
/* ========================================================================= */
void SendCmdFADEINOUT(DWORD pIDSend, BYTE byFade)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_FADEINOUT;
	pSendData->pID = pIDSend;
	pSendData->byData = byFade;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGITEM													 */
/* 内容：コマンド送信(アイテム情報更新)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdCHGITEM(DWORD pIDSend, DWORD dwID)
{
	CMDPARA_CHGITEM Para;
	PCMDPARA_CHGITEM pSendData;
	PBYTE pData;
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
	PBYTE pDataTmp = pData;
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

	svrSock.SendTo (pIDSend, pData, (DWORD)(pDataTmp - pData), SENDTIMEOUT);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdITEMINFO													 */
/* 内容：コマンド送信(アイテム情報送信)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdITEMINFO(DWORD pIDSend)
{
	PCMDPARA_DATA3 pSendData;
	PBYTE pData, pSendDataTmp;
	int i, nCount = g_aItemInfo.GetSize ();

	if (nCount == 0) {
		return;
	}
	DWORD dwSize = sizeof (CMDPARA_DATA3);
	PITEMINFO pItem;

	/* 送信データサイズを計算 */
	for (i = 0; i < nCount; i ++) {
		pItem = g_aItemInfo[i];
		dwSize += sizeof (ITEMINFO);
		if (pItem->pMsg) {
			dwSize += (strlen ((LPSTR)pItem->pMsg) + 1);
		}
		if (pItem->pData) {
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
		}
	}

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_DATA3)pData;
	pSendData->byCmd = CMD_ITEMINFO;
	pSendData->dwData = (DWORD)nCount;

	pSendDataTmp = pData + sizeof (CMDPARA_DATA3);
	for (i = 0; i < nCount; i ++) {
		pItem = g_aItemInfo[i];
		/* 名前は可変長に詰める */
		pSendDataTmp = SboPackPutITEMINFO (pSendDataTmp, pItem);
		/* 使った時のメッセージ */
		if (pItem->pMsg) {
			strcat ((LPSTR)pSendDataTmp, (LPSTR)pItem->pMsg);
			pSendDataTmp += (strlen ((LPSTR)pItem->pMsg) + 1);
		}
		/* 詳細データ */
		if (pItem->pData) {
			pSendDataTmp = SboPackPutItemData (pSendDataTmp, pItem->byType, pItem->pData, GetItemDataSize (pItem->byType));
			if (pItem->byType == ITEMTYPE_MSG) {
				PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
				if (pInfo) {
					if (pInfo->pMsgMe) {
						strcpy ((LPSTR)pSendDataTmp, (LPSTR)pInfo->pMsgMe);
						pSendDataTmp += (strlen ((LPSTR)pInfo->pMsgMe) + 1);
					}
					if (pInfo->pMsgTo) {
						strcpy ((LPSTR)pSendDataTmp, (LPSTR)pInfo->pMsgTo);
						pSendDataTmp += (strlen ((LPSTR)pInfo->pMsgTo) + 1);
					}
				}
			}
		}
	}

//	svr.SetPriority (1);
	svrSock.SendTo (pIDSend, pData, (DWORD)(pSendDataTmp - pData), SENDTIMEOUT);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARITEM													 */
/* 内容：コマンド送信(所持アイテム情報)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void SendCmdCHARITEM(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_CHARITEM pSendData;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSendData = (PCMDPARA_CHARITEM)new CMDPARA_CHARITEM;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHARITEM));
	pSendData->byCmd = CMD_CHARITEM;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = dwID;
	CopyMemory (pSendData->waItem, pChar->waItem, sizeof (pChar->waItem));
	CopyMemory (pSendData->waEqu, pChar->waEqu, sizeof (pChar->waEqu));

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_CHARITEM), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSVRINFO													 */
/* 内容：コマンド送信(サーバー情報)											 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void SendCmdSVRINFO(DWORD pIDSend)
{
	PCMDPARA_SVRINFO pSendData;

	pSendData = (PCMDPARA_SVRINFO)new CMDPARA_SVRINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_SVRINFO));
	pSendData->byCmd = CMD_SVRINFO;
	pSendData->dwCharCount = (DWORD)g_aCharInfo.GetSize ();
	pSendData->dwMapCount = (DWORD)g_aMapInfo.GetSize ();
	pSendData->dwTime = timeGetTime () - g_dwServerStartTime;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_SVRINFO), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAPCHARINFO												 */
/* 内容：コマンド送信(マップ内キャラ数通知)									 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void SendCmdMAPCHARINFO(DWORD dwID)
{
	PCMDPARA_MAPCHARINFO pSendData;
	PCHARINFO pChar = g_aCharInfo[dwID];
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];

	pSendData = (PCMDPARA_MAPCHARINFO)new CMDPARA_MAPCHARINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_MAPCHARINFO));
	pSendData->byCmd = CMD_MAPCHARINFO;
	pSendData->dwCountPC = pMap->dwCountTblNoPC;
	pSendData->dwCountNPC = pMap->dwCountTblNoNPC;

	SendCmdInsideMap (pChar->dwMapID, (PBYTE)pSendData, sizeof (CMDPARA_MAPCHARINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdNAMECOLOR													 */
/* 内容：コマンド送信(名前の色設定)											 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void SendCmdNAMECOLOR(DWORD dwID)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	PCMDPARA_NAMECOLOR pSendData;

	pSendData = new CMDPARA_NAMECOLOR;
	ZeroMemory (pSendData, sizeof (CMDPARA_NAMECOLOR));
	pSendData->byCmd = CMD_NAMECOLOR;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->clName = pChar->clName;
	pSendData->clSpeech = pChar->clSpeech;

	SendCmdInsideMap (pChar->dwMapID, (PBYTE)pSendData, sizeof (CMDPARA_NAMECOLOR));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSHOP														 */
/* 内容：コマンド送信(店情報)												 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void SendCmdSHOP(DWORD dwID, BYTE byNo)
{
	SendCmdSHOPMap (g_aCharInfo[dwID]->dwMapID, byNo);
}

/* ========================================================================= */
/* 関数名：SendCmdSHOPMap													 */
/* 内容：コマンド送信(店情報・マップ指定)									 */
/*       そのマップにいるプレイヤーに送る									 */
/* ========================================================================= */
void SendCmdSHOPMap(DWORD dwMapID, BYTE byNo)
{
	PCMDPARA_SHOP pSendData;
	DWORD dwSize = sizeof (CMDPARA_SHOP);
	PMAPINFO pMap;
	PSHOPINFO pShop;
	int i;

	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	pMap = g_aMapInfo[dwMapID];
	if ((pMap->pShopInfo == NULL) || (byNo >= pMap->byCountShop)) {
		return;
	}
	pShop = &pMap->pShopInfo[byNo];

	/* 送信データサイズ計算 */
	for (i = 0; i < MAX_SHOPMSG; i ++) {
		if (pShop->apMsg[i]) {
			dwSize += (strlen (pShop->apMsg[i]) + 1);
		}
	}
	
	PBYTE pData = new BYTE[dwSize], ptrDataTmp;
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_SHOP)pData;
	pSendData->byCmd = CMD_SHOP;
	pSendData->byID = byNo;
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

	SendCmdInsideMap (dwMapID, pData, dwSize);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdEFC														 */
/* 内容：コマンド送信(エフェクト情報更新)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void SendCmdEFC(BYTE byNo)
{
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
	pSend->byCmd	= CMD_EFC;
	pSend->wData	= byNo;

	pDataTmp = g_EfcInfo.GetSendData (byNo);
	CopyMemory ((PBYTE)(pSend + 1), pDataTmp, dwSizeTmp);

	svrSock.SendTo (URARASOCK_SENDALL, pData, dwSize, SENDTIMEOUT);
	SAFE_DELETE_ARRAY (pData);
	SAFE_DELETE_ARRAY (pDataTmp);
}

/* ========================================================================= */
/* 関数名：SendCmdEFCINFO													 */
/* 内容：コマンド送信(エフェクト情報通知)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void SendCmdEFCINFO(DWORD pIDSend)
{
	PBYTE pData, pDataTmp;
	DWORD dwSize, dwSizeTmp;
	PCMDPARA_CHGEFC pSend;

	/* サイズの計算 */
	dwSize = 0;
	dwSize += sizeof (CMDPARA_CHGEFC);
	dwSizeTmp = g_EfcInfo.GetAllSendDataSize ();
	dwSize += dwSizeTmp;

	/* 送信データの作成 */
	pData = new BYTE[dwSize];
	pSend = (PCMDPARA_CHGEFC)pData;
	pSend->byCmd	= CMD_EFCINFO;
	pSend->wData	= (WORD)g_EfcInfo.GetCount ();

	pDataTmp = g_EfcInfo.GetAllSendData ();
	CopyMemory ((PBYTE)(pSend + 1), pDataTmp, dwSizeTmp);

	svrSock.SendTo (pIDSend, pData, dwSize, SENDTIMEOUT);
	SAFE_DELETE_ARRAY (pData);
	SAFE_DELETE_ARRAY (pDataTmp);
}

/* ========================================================================= */
/* 関数名：SendCmdCHARSTATUS												 */
/* 内容：コマンド送信(キャラステータス通知)									 */
/* 日付：2003/09/30															 */
/* ========================================================================= */
void SendCmdCHARSTATUS(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_CHARSTATUS pSend;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSend = new CMDPARA_CHARSTATUS;
	ZeroMemory (pSend, sizeof (CMDPARA_CHARSTATUS));

	pSend->byCmd			= CMD_CHARSTATUS;
	pSend->pID				= pChar->pID;
	pSend->dwTblID			= pChar->dwTblID;
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
	pSend->wDFMagic			= pChar->wDFMagic;
	pSend->wMaxHP			= pChar->wMaxHP;
	pSend->wHP				= pChar->wHP;
	pSend->wMaxMana			= pChar->wMaxMana;
	pSend->wMana			= pChar->wMana;
	pSend->dwGold			= pChar->dwGold;
	pSend->dwExp			= pChar->dwExp;
	pSend->byDrunkPoint		= pChar->byDrunkPoint;
	pSend->byDrunkLV		= pChar->byDrunkLV;
	pSend->byDropProbability = pChar->byDropProbability;
	CopyMemory (pSend->waEqu, pChar->waEqu, sizeof (pChar->waEqu));
	CopyMemory (pSend->abyMagic, pChar->abyMagic, sizeof (pChar->abyMagic));

	svrSock.SendTo (pIDSend, (PBYTE)pSend, sizeof (CMDPARA_CHARSTATUS), SENDTIMEOUT);

	/* パーティに参加している？ */
	if (pChar->dwPartyID) {
		PPARTYINFO pPartyInfo;
		int i;

		pPartyInfo = g_aPartyInfo[pChar->dwPartyID];
		/* 各メンバーに解散を通知 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			PCHARINFO pCharTmp;

			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}

			pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			svrSock.SendTo (pCharTmp->pID, (PBYTE)pSend, sizeof (CMDPARA_CHARSTATUS), SENDTIMEOUT);
		}
	}
	delete pSend;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARSTATUSHP												 */
/* 内容：コマンド送信(キャラステータス(HPのみ)通知)							 */
/* 日付：2003/10/05															 */
/* ========================================================================= */
void SendCmdCHARSTATUSHP(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_CHARSTATUSHP pSend;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSend = new CMDPARA_CHARSTATUSHP;
	ZeroMemory (pSend, sizeof (CMDPARA_CHARSTATUSHP));

	pSend->byCmd		= CMD_CHARSTATUSHP;
	pSend->byCharState	= pChar->byCharState;
	pSend->pID			= pChar->pID;
	pSend->dwTblID		= pChar->dwTblID;
	pSend->wHP			= pChar->wHP;

	if (pIDSend == URARASOCK_SENDALL) {
		SendCmdInside2 (pChar, (LPBYTE)pSend, sizeof (CMDPARA_CHARSTATUSHP), SENDTIMEOUT, SENDINSIDE_INSIDE);
	} else {
		svrSock.SendTo (pIDSend, (PBYTE)pSend, sizeof (CMDPARA_CHARSTATUSHP), SENDTIMEOUT);
	}
	delete pSend;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARSTATUSHPMANA											 */
/* 内容：コマンド送信(キャラステータス(HPとMANA)通知)						 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void SendCmdCHARSTATUSHPMANA(DWORD dwSend, DWORD dwID)
{
	PCMDPARA_CHARSTATUSHPMANA pSend;
	PCHARINFO pChar = g_aCharInfo[dwID],
			  pCharSend = g_aCharInfo[dwSend];
	int i;

	pSend = new CMDPARA_CHARSTATUSHPMANA;
	ZeroMemory (pSend, sizeof (CMDPARA_CHARSTATUSHPMANA));

	pSend->byCmd		= CMD_CHARSTATUSHPMANA;
	pSend->byCharState	= pChar->byCharState;
	pSend->pID			= pChar->pID;
	pSend->dwTblID		= pChar->dwTblID;
	pSend->wHP			= pChar->wHP;
	pSend->wMaxHP		= pChar->wMaxHP;
	pSend->wMana		= pChar->wMana;
	pSend->wMaxMana		= pChar->wMaxMana;

	/* パーティに参加している？ */
	if ((dwSend == dwID) && pChar->dwPartyID) {
		PPARTYINFO pPartyInfo;
		pPartyInfo = g_aPartyInfo[pChar->dwPartyID];
		/* 各メンバーに解散を通知 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			PCHARINFO pCharTmp;

			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}

			pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			svrSock.SendTo (pCharTmp->pID, (PBYTE)pSend, sizeof (CMDPARA_CHARSTATUSHPMANA), SENDTIMEOUT);
		}

	} else {
		svrSock.SendTo (pCharSend->pID, (PBYTE)pSend, sizeof (CMDPARA_CHARSTATUSHPMANA), SENDTIMEOUT);
	}
	delete pSend;
}

/* ========================================================================= */
/* 関数名：SendCmdCHARSTATUSGOLDEXP											 */
/* 内容：コマンド送信(キャラステータス(所持金と経験値)通知)					 */
/* 日付：2003/11/19															 */
/* ========================================================================= */
void SendCmdCHARSTATUSGOLDEXP(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_DATA5 pSend;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSend = new CMDPARA_DATA5;
	ZeroMemory (pSend, sizeof (CMDPARA_DATA5));

	pSend->byCmd	= CMD_CHARSTATUSGOLD;
	pSend->pID		= pChar->pID;
	pSend->dwTblID	= pChar->dwTblID;
	pSend->dwData1	= pChar->dwGold;
	pSend->dwData2	= pChar->dwExp;

	if (pIDSend == URARASOCK_SENDALL) {
		SendCmdInside2 (pChar, (LPBYTE)pSend, sizeof (CMDPARA_DATA5), SENDTIMEOUT, SENDINSIDE_INSIDE);
	} else {
		svrSock.SendTo (pIDSend, (PBYTE)pSend, sizeof (CMDPARA_DATA5), SENDTIMEOUT);
	}
	delete pSend;
}

/* ========================================================================= */
/* 関数名：SendCmdTARGETCHARPC												 */
/* 内容：コマンド送信(ついていく通知)										 */
/* 日付：2003/10/07															 */
/* ========================================================================= */
void SendCmdTARGETCHARPC(DWORD pIDSend, DWORD dwID)
{
	PCMDPARA_DATA3 pSendData;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_TARGETCHARPC;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->dwData = pChar->dwTargetTblID3;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA3), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADDCHAREFC												 */
/* 内容：コマンド送信(文字エフェクト追加)									 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void SendCmdADDCHAREFC(DWORD dwTblID, COLORREF color, LPSTR pszMsg)
{
	PCMDPARA_ADDCHAREFC pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];

	pSendData = new CMDPARA_ADDCHAREFC;
	ZeroMemory (pSendData, sizeof (CMDPARA_ADDCHAREFC));
	pSendData->byCmd = CMD_ADDCHAREFC;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->color = color;
	SboTextCopy (pSendData->szMsg, sizeof (pSendData->szMsg), pszMsg);

	/* 文字列は可変長に詰めて送る */
	{
		DWORD dwPackSize = SboPackSizeADDCHAREFC (pSendData);
		PBYTE pPack = new BYTE[dwPackSize];
		SboPackPutADDCHAREFC (pPack, pSendData);
		SendCmdInside2 (pChar, pPack, dwPackSize, SENDTIMEOUT, SENDINSIDE_INSIDE);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdCHGCOLOR													 */
/* 内容：コマンド送信(色変換)												 */
/* 日付：2003/10/18															 */
/* ========================================================================= */
void SendCmdCHGCOLOR(DWORD dwMapID)
{
	PCMDPARA_CHGMAPCOLOR pSendData;
	PMAPINFO pMap = g_aMapInfo[dwMapID];

	pSendData = new CMDPARA_CHGMAPCOLOR;
	ZeroMemory (pSendData, sizeof (CMDPARA_CHGMAPCOLOR));
	pSendData->byCmd = CMD_CHGMAPCOLOR;
	pSendData->dwMapID = dwMapID;
	pSendData->color = pMap->clChg;

	SendCmdInsideMap (dwMapID, (LPBYTE)pSendData, sizeof (CMDPARA_CHGMAPCOLOR));
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
	PBYTE pData = NULL, pTmp;
	DWORD dwSize = 0;
	int i;

	if (g_aStTmpInfo.GetSize () == 0) {
		return;
	}

	switch (byType) {
	case CMDSTTMPLATETYPE_ADD:	/* 追加 */
	case CMDSTTMPLATETYPE_CHG:	/* 変更 */
		dwSize = sizeof (CMDPARA_DATA1) + sizeof (STATUSTEMPLATEINFO);
		pData = new BYTE [dwSize];
		ZeroMemory (pData, dwSize);
		/* 名前は可変長に詰める */
		pTmp = SboPackPutSTATUSTEMPLATEINFO (pData + sizeof (CMDPARA_DATA1), g_aStTmpInfo[dwNo]);
		dwSize = (DWORD)(pTmp - pData);
		break;

	case CMDSTTMPLATETYPE_ALL:	/* 全て更新 */
		dwSize = sizeof (CMDPARA_DATA1) + (g_aStTmpInfo.GetSize () * sizeof (STATUSTEMPLATEINFO));
		pData = new BYTE [dwSize];
		ZeroMemory (pData, dwSize);
		pTmp = pData + sizeof (CMDPARA_DATA1);
		for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
			pTmp = SboPackPutSTATUSTEMPLATEINFO (pTmp, g_aStTmpInfo[i]);
		}
		dwSize = (DWORD)(pTmp - pData);
		break;

	default:
		return;
	}

	if (pData == NULL) {
		return;
	}

	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_STTMPLATE;
	pSendData->byData = byType;

	if (byType != CMDSTTMPLATETYPE_ALL) {
		pSendData->dwTblID = dwNo;
		SendCmdAdmin (pData, dwSize);
	} else {
		pSendData->dwTblID = g_aStTmpInfo.GetSize ();
		svrSock.SendTo (g_aCharInfo[dwNo]->pID, pData, dwSize);
	}
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEITEMGET												 */
/* 内容：コマンド送信(預けてるアイテム取得)									 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void SendCmdLEAVEITEMGET(DWORD dwTblID)
{
	PCMDPARA_LEAVEITEMGET pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];
	PLEAVEINFO pInfo = g_aLeaveInfo[pChar->dwLeaveID];

	pSendData = new CMDPARA_LEAVEITEMGET;
	ZeroMemory (pSendData, sizeof (CMDPARA_LEAVEITEMGET));
	pSendData->byCmd = CMD_LEAVEITEMGET;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->byCount = pInfo->byCount;
	CopyMemory (pSendData->waItem, pInfo->waItem, sizeof (pInfo->waItem));
	CopyMemory (pSendData->byItemCount, pInfo->byItemCount, sizeof (pInfo->byItemCount));

	svrSock.SendTo (pChar->pID, (LPBYTE)pSendData, sizeof (CMDPARA_LEAVEITEMGET));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSUBGAMECOUNT												 */
/* 内容：コマンド送信(サブゲーム数通知)										 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void SendCmdSUBGAMECOUNT(DWORD pIDSend)
{
	PCMDPARA_DATA2 pSendData;

	pSendData = new CMDPARA_DATA2;
	pSendData->byCmd = CMD_COUNTSUBGAME;
	pSendData->nData = g_aSubaGameInfo.GetSize ();

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA2), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSUBGAMEINFO												 */
/* 内容：コマンド送信(サブゲーム情報通知)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void SendCmdSUBGAMEINFO(DWORD pIDSend, int nNo)
{
	PCMDPARA_DATA1 pSendData;
	PBYTE pData;
	DWORD dwSize = sizeof (CMDPARA_DATA1);

	if ((g_aSubaGameInfo.GetSize () == 0) || (nNo >= g_aSubaGameInfo.GetSize ())) {
		return;
	}

	PSUBGAMEINFO pInfo = g_aSubaGameInfo[nNo];
	if (pInfo->pInfo == NULL) {
		return;
	}

	dwSize += GetSubGameDataSize (pInfo->byType);
	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	switch (pInfo->byType) {
	case SUBGAMEID_BOMB:
		{
			PSUBGAME_BOMBINFO pBombInfo = (PSUBGAME_BOMBINFO)pInfo->pInfo;
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
			PSUBGAME_MAZEINFO pMazeInfo = (PSUBGAME_MAZEINFO)pInfo->pInfo;
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
			PSUBGAME_DUNGEONINFO pDungeonInfo = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
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
			PSUBGAME_WARINFO pInfoTmp = (PSUBGAME_WARINFO)pInfo->pInfo;
			PSUBGAME_WARINFOSAVE pInfoSend = (PSUBGAME_WARINFOSAVE)(pData + sizeof (CMDPARA_DATA1));

			pInfoSend->dwMapID = pInfoTmp->dwMapID;
			pInfoSend->byMember = pInfoTmp->byMember;
			pInfoSend->byX = pInfoTmp->byX;
			pInfoSend->byY = pInfoTmp->byY;
			pInfoSend->byX1 = pInfoTmp->byX1;
			pInfoSend->byY1 = pInfoTmp->byY1;
			pInfoSend->byX2 = pInfoTmp->byX2;
			pInfoSend->byY2 = pInfoTmp->byY2;
			pInfoSend->byHP1X = pInfoTmp->byHP1X;
			pInfoSend->byHP2X = pInfoTmp->byHP2X;
			pInfoSend->byHPY = pInfoTmp->byHPY;
			pInfoSend->byJudgeX = pInfoTmp->byJudgeX;
			pInfoSend->byJudgeY = pInfoTmp->byJudgeY;
			pInfoSend->byRes1X = pInfoTmp->byRes1X;
			pInfoSend->byRes2X = pInfoTmp->byRes2X;
			pInfoSend->byResY = pInfoTmp->byResY;
			pInfoSend->byStart1X = pInfoTmp->byStart1X;
			pInfoSend->byStart2X = pInfoTmp->byStart2X;
			pInfoSend->byStartY = pInfoTmp->byStartY;
			pInfoSend->byCastle1X = pInfoTmp->byCastle1X;
			pInfoSend->byCastle2X = pInfoTmp->byCastle2X;
			pInfoSend->byCastleY = pInfoTmp->byCastleY;
		}
		break;

	case SUBGAMEID_SUGOROKU:	/* すごろく */
		{
			PSUBGAME_SUGOROKUINFO pInfoTmp = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
			PSUBGAME_SUGOROKUINFOSAVE pInfoSend;
			DWORD i, dwEventCount, dwPartsCount;
			PBYTE pTmp;

			delete [] pData;
			dwEventCount = pInfoTmp->aEventInfo.GetSize ();
			dwPartsCount = pInfoTmp->aPartsInfo.GetSize ();
			dwSize = sizeof (CMDPARA_DATA1) + GetSubGameDataSize (pInfo->byType);
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
	pSendData->byData = pInfo->byType;
	pSendData->dwTblID = nNo;

	svrSock.SendTo (pIDSend, pData, dwSize, SENDTIMEOUT);
	delete [] pData;
}


/* ========================================================================= */
/* 関数名：	SendCmdSUBGAMEINFOALL											 */
/* 内容：	コマンド送信(全サブゲーム情報通知)								 */
/* 日付：	2006/04/18														 */
/* ========================================================================= */

void SendCmdSUBGAMEINFOALL(DWORD pIDSend)
{
	int i, nCount;
	PCMDPARA_DATA1 pSendData;
	BYTE byTmp;
	PBYTE pData, pDataTmp, pSubGameData;
	DWORD dwSize, dwSizeTmp;
	PCProcSubGameBase pProc;

	pData	= NULL;
	nCount	= g_ProcSubGame.GetCount ();
	dwSize	= sizeof (CMDPARA_DATA1);

	if (nCount <= 0) {
		goto Exit;
	}

	/* 送信データサイズを求める */
	for (i = 0; i < nCount; i ++) {
		pProc = g_ProcSubGame.Get (i);
		dwSize += pProc->GetSendDataSize ();
	}
	/* 種別分のサイズ */
	dwSize += nCount;

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);

	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd	= CMD_SUBGAMEINFOALL;		/* コマンドID */
	pSendData->byData	= (BYTE)nCount;				/* データ数 */

	pDataTmp = (PBYTE)(pSendData + 1);
	/* サブゲーム情報を設定 */
	for (i = 0; i < nCount; i ++) {
		pProc			= g_ProcSubGame.Get (i);
		dwSizeTmp		= pProc->GetSendDataSize ();
		pSubGameData	= pProc->GetSendData ();
		byTmp			= (BYTE)pProc->GetType ();

		CopyMemory (pDataTmp, &byTmp, sizeof (byTmp));
		pDataTmp += sizeof (byTmp);
		CopyMemory (pDataTmp, pSubGameData, dwSizeTmp);
		pDataTmp += dwSizeTmp;

		SAFE_DELETE_ARRAY (pSubGameData);
	}

	svrSock.SendTo (pIDSend, pData, dwSize, SENDTIMEOUT);

Exit:
	SAFE_DELETE_ARRAY (pData);
}


/* ========================================================================= */
/* 関数名：SendCmdSUBSTATUS													 */
/* 内容：コマンド送信(サブステータス情報)									 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void SendCmdSUBSTATUS(DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;
	PBYTE pData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];
	DWORD dwSize = sizeof (CMDPARA_DATA1) + sizeof (pChar->abySubStatus);

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);
	pSendData = (PCMDPARA_DATA1)pData;
	pSendData->byCmd = CMD_SUBSTATUS;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = dwTblID;
	CopyMemory (pSendData + 1, pChar->abySubStatus, sizeof (pChar->abySubStatus));

	svrSock.SendTo (pChar->pID, pData, dwSize, SENDTIMEOUT);
	delete [] pData;
}

/* ========================================================================= */
/* 関数名：SendCmdTRADE														 */
/* 内容：コマンド送信(取り引きコマンド)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void SendCmdTRADE(DWORD pIDSend, DWORD dwTblID, BYTE byCmd)
{
	PCMDPARA_DATA1 pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];

	pSendData = new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_TRADE;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = dwTblID;
	pSendData->byData = byCmd;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA1), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSYSMSG2													 */
/* 内容：コマンド送信(ID指定システムメッセージ)								 */
/* 日付：2003/11/24															 */
/* ========================================================================= */
void SendCmdSYSMSG2(DWORD pIDSend, DWORD dwID, DWORD dwData1, DWORD dwData2)
{
	PCMDPARA_DATA6 pSendData;

	pSendData = new CMDPARA_DATA6;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA6));
	pSendData->byCmd = CMD_SYSMSG2;
	pSendData->pID = dwID;
	pSendData->dwData1 = dwData1;
	pSendData->dwData2 = dwData2;

	svrSock.SendTo (pIDSend, (PBYTE)pSendData, sizeof (CMDPARA_DATA6), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEGOLDGET												 */
/* 内容：コマンド送信(預けてる金額取得)										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void SendCmdLEAVEGOLDGET(DWORD dwTblID)
{
	PCMDPARA_DATA3 pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];
	PLEAVEINFO pInfo = g_aLeaveInfo[pChar->dwLeaveID];

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_LEAVEGOLDGET;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->dwData = pInfo->dwGold;

	svrSock.SendTo (pChar->pID, (LPBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdLEAVEINFO													 */
/* 内容：コマンド送信(預かり所情報)											 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void SendCmdLEAVEINFO(DWORD pIDSend, DWORD dwTblID)
{
	PCMDPARA_LEAVEINFO pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];
	if (pChar->dwLeaveID == 0) {
		return;
	}

	PLEAVEINFO pInfo = g_aLeaveInfo[pChar->dwLeaveID];

	pSendData = new CMDPARA_LEAVEINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_LEAVEINFO));
	pSendData->byCmd = CMD_LEAVEINFO;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->byCount = pInfo->byCount;
	pSendData->dwGold = pInfo->dwGold;
	CopyMemory (pSendData->waItem, pInfo->waItem, sizeof (pInfo->waItem));
	CopyMemory (pSendData->byItemCount, pInfo->byItemCount, sizeof (pInfo->byItemCount));

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_LEAVEINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdACCSEARCH													 */
/* 内容：コマンド送信(アカウント検索)										 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void SendCmdACCSEARCH(DWORD pIDSend, DWORD dwTblID)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_ACC_SEARCH;
	pSendData->pID = pIDSend;
	pSendData->dwTblID = dwTblID;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdADMINWINDOW												 */
/* 内容：コマンド送信(管理者ウィンドウ表示)									 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void SendCmdADMINWINDOW(DWORD pIDSend, BYTE byData)
{
	PCMDPARA_DATA1 pSendData;

	pSendData = (PCMDPARA_DATA1)new CMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA1));
	pSendData->byCmd = CMD_ADMINWINDOW;
	pSendData->byData = byData;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (CMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdPARTY														 */
/* 内容：コマンド送信(仲間コマンド)											 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void SendCmdPARTY(DWORD dwTblIDTo, DWORD dwTblIDFrom, BYTE byCmdID, BYTE byData)
{
	PCHARINFO pCharTo = g_aCharInfo[dwTblIDTo];

	PCMDPARA_PARTYCMD pSendData;
	pSendData = new CMDPARA_PARTYCMD;
	ZeroMemory (pSendData, sizeof (CMDPARA_PARTYCMD));

	pSendData->byCmd = CMD_PARTY;
	pSendData->byCmdSub = byCmdID;
	pSendData->byData = byData;
	pSendData->dwTblIDFrom = dwTblIDFrom;
	pSendData->dwTblIDTo = dwTblIDTo;

	svrSock.SendTo (pCharTo->pID, (LPBYTE)pSendData, sizeof (CMDPARA_PARTYCMD));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：SendCmdJOBINFO													 */
/* 内容：コマンド送信(職業情報)												 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void SendCmdJOBINFO(DWORD pIDSend, BYTE byID)
{
	DWORD dwSize, dwSizeInfo;
	PBYTE pData, pDataTmp, pDataInfo;
	PCMDPARA_JOBINFO pSendData;
	PJOBINFO pInfo;

	pInfo	= g_JobInfo.GetPtr (byID);
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

	svrSock.SendTo (pIDSend, pData, dwSize);
	SAFE_DELETE_ARRAY (pData);
	SAFE_DELETE_ARRAY (pDataInfo);
}


/* ========================================================================= */
/* 関数名：SendCmdMAGICINFO													 */
/* 内容：コマンド送信(魔法情報)												 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void SendCmdMAGICINFO(DWORD pIDSend, BYTE byID)
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
		svrSock.SendTo (pIDSend, pPack, dwPackSize);
		delete [] pPack;
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdMAGICUSERINFO												 */
/* 内容：コマンド送信(魔法利用者情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void SendCmdMAGICUSERINFO(DWORD pIDSend, BYTE byID)
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

	svrSock.SendTo (pIDSend, pSendDataTmp, dwSize);
	delete [] pSendDataTmp;
}

/* ========================================================================= */
/* 関数名：SendCmdEQUUSERINFO												 */
/* 内容：コマンド送信(装備職業情報)											 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void SendCmdEQUUSERINFO(DWORD pIDSend, BYTE byID)
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

	svrSock.SendTo (pIDSend, pSendDataTmp, dwSize);
	delete [] pSendDataTmp;
}

/* ========================================================================= */
/* 関数名：SendCmdTARGETCHAR												 */
/* 内容：コマンド送信(ターゲット要求)										 */
/* 日付：2004/11/21															 */
/* ========================================================================= */
void SendCmdTARGETCHAR(DWORD dwTblID, DWORD dwTblIDTarget)
{
	PCMDPARA_DATA3 pSendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];

	pSendData = new CMDPARA_DATA3;
	ZeroMemory (pSendData, sizeof (CMDPARA_DATA3));
	pSendData->byCmd = CMD_TARGETCHAR;
	pSendData->dwData = dwTblIDTarget;

	svrSock.SendTo (pChar->pID, (LPBYTE)pSendData, sizeof (CMDPARA_DATA3));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：SendCmdSubBombTime												 */
/* 内容：サブゲーム用コマンド送信(爆弾ゲーム：残り時間通知)					 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void SendCmdSubBombTime(BYTE byID, PSUBGAME_BOMBINFO pInfo)
{
	PSUBCMDPARA_DATA1 pSendData;

	pSendData = new SUBCMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (SUBCMDPARA_DATA1));
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_TIME;
	pSendData->byData = pInfo->byGameTimeNow;

	SendCmdSubGame (byID, pInfo->dwMapID, (LPBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBINFO												 */
/* 内容：サブゲーム用コマンド送信(爆弾ゲーム：設定)							 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void SendCmdSubBOMBINFO(DWORD pIDSend, BYTE byID, PSUBGAME_BOMBINFO pInfo)
{
	PSUBCMDPARA_BOMBINFO pSendData;
	pSendData = new SUBCMDPARA_BOMBINFO;
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_INFO;
	pSendData->byID = byID;
	pSendData->byGameTime = pInfo->byGameTime;
	pSendData->byMaxBomb = pInfo->byMaxBomb;
	pSendData->byMaxFire = pInfo->byMaxFire;
	pSendData->byFlgGameNow = pInfo->byFlgGameNow;
	pSendData->byFlgUseBlock = pInfo->byFlgUseBlock;
	pSendData->byFlgUseItem = pInfo->byFlgUseItem;
	pSendData->byFlgProgress = pInfo->byFlgProgress;
	pSendData->dwTblIDGameMaster = pInfo->dwTblIDGameMaster;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (SUBCMDPARA_BOMBINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBSTOP												 */
/* 内容：サブゲーム用コマンド送信(爆弾ゲーム：試合中止)						 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void SendCmdSubBOMBSTOP(DWORD pIDSend, BYTE byID, PSUBGAME_BOMBINFO pInfo)
{
	PSUBCMDPARA_DATA1 pSendData;

	pSendData = new SUBCMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (SUBCMDPARA_DATA1));
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_STOP;
	pSendData->byData = byID;

	if (pIDSend == URARASOCK_SENDALL) {
		SendCmdSubGame (byID, pInfo->dwMapID, (LPBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	} else {
		svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (SUBCMDPARA_BOMBINFO));
	}
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：SendCmdSubBOMBSETBOMB											 */
/* 内容：サブゲーム用コマンド送信(爆弾ゲーム：爆弾設置)						 */
/* 日付：2004/04/11															 */
/* ========================================================================= */
void SendCmdSubBOMBSETBOMB(DWORD dwID)
{
	PCMDPARA_BOMBSETBOMB pSendData;
	PCHARINFO pChar = g_aCharInfo[dwID];

	pSendData = new CMDPARA_BOMBSETBOMB;
	ZeroMemory (pSendData, sizeof (CMDPARA_BOMBSETBOMB));
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_BOMB;
	pSendData->byCmdSub = CMDSUBBOMB_SETBOMB;
	pSendData->pID = pChar->pID;
	pSendData->dwTblID = pChar->dwTblID;
	pSendData->byNpcType = pChar->byNpcType;
	pSendData->dwX = pChar->dwX;
	pSendData->dwY = pChar->dwY;

	SendCmdInside2 (pChar, (LPBYTE)pSendData, sizeof (CMDPARA_BOMBSETBOMB), SENDTIMEOUT, SENDINSIDE_INSIDE);
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdSubWarCHGSTATE											 */
/* 内容：	サブゲーム用コマンド送信(戦場ゲーム：状態遷移)					 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void SendCmdSubWarCHGSTATE(DWORD pIDSend, BYTE byState)
{
	PSUBCMDPARA_DATA1 pSendData;

	pSendData = new SUBCMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (SUBCMDPARA_DATA1));

	pSendData->byCmd		= CMD_SUBGAME;
	pSendData->bySubGameID	= SUBGAMEID_WAR;
	pSendData->byCmdSub		= CMDSUBWAR_CHGSTATE;
	pSendData->byData		= byState;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdSubWarSELECTTURN											 */
/* 内容：	サブゲーム用コマンド送信(戦場ゲーム：行動順序通知)				 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void SendCmdSubWarSELECTTURN(CInfoSubGameWAR *pInfo, BYTE byTeam)
{
	int i, nCount;
	PCMDPARA_WARSELECTTURN pSendData;
	PCHARINFO pChar;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	pTeamInfo = pInfo->GetTeamInfo (byTeam);

	pSendData = new CMDPARA_WARSELECTTURN;
	ZeroMemory (pSendData, sizeof (CMDPARA_WARSELECTTURN));
	pSendData->byCmd		= CMD_SUBGAME;
	pSendData->bySubGameID	= SUBGAMEID_WAR;
	pSendData->byCmdSub		= CMDSUBWAR_SELECTTURN;
	pSendData->byCount		= (BYTE)pInfo->GetMemberCount (byTeam);

	/* 一覧を作成 */
	nCount = pTeamInfo->aMemberInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];
		pChar = g_aCharInfo[pMemberInfo->dwTblID];
		SboTextCopy (pSendData->aszName[i], sizeof (pSendData->aszName[i]), pChar->szName);
	}

	/* チーム内メンバーに通知（名前は可変長に詰める） */
	DWORD dwPackSize = SboPackSizeWARSELECTTURN (pSendData);
	PBYTE pPack = new BYTE[dwPackSize];
	SboPackPutWARSELECTTURN (pPack, pSendData);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];
		pChar = g_aCharInfo[pMemberInfo->dwTblID];
		svrSock.SendTo (pChar->pID, pPack, dwPackSize);
	}
	delete [] pPack;

	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdSubWarMAGIC_TARGET										 */
/* 内容：	サブゲーム用コマンド送信(戦場ゲーム：魔法対象者一覧)			 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void SendCmdSubWarMAGIC_TARGET(DWORD pIDSend, PCMDPARA_WARSELECTMAGICTARGET pSendData)
{
	pSendData->byCmd		= CMD_SUBGAME;
	pSendData->bySubGameID	= SUBGAMEID_WAR;
	pSendData->byCmdSub		= CMDSUBWAR_MAGIC_TARGET;

	/* 名前は可変長に詰めて送る */
	DWORD dwPackSize = SboPackSizeWARSELECTMAGICTARGET (pSendData);
	PBYTE pPack = new BYTE[dwPackSize];
	SboPackPutWARSELECTMAGICTARGET (pPack, pSendData);
	svrSock.SendTo (pIDSend, pPack, dwPackSize);
	delete [] pPack;
}


/* ========================================================================= */
/* 関数名：SendCmdSubSugorokuCHGSTAT										 */
/* 内容：サブゲーム用コマンド送信(すごろく：状態遷移)						 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void SendCmdSubSugorokuCHGSTATE(DWORD pIDSend, BYTE byState)
{
	PSUBCMDPARA_DATA1 pSendData;

	pSendData = new SUBCMDPARA_DATA1;
	ZeroMemory (pSendData, sizeof (SUBCMDPARA_DATA1));
	pSendData->byCmd = CMD_SUBGAME;
	pSendData->bySubGameID = SUBGAMEID_SUGOROKU;
	pSendData->byCmdSub = CMDSUBSUGOROKU_CHGSTATE;
	pSendData->byData = byState;

	svrSock.SendTo (pIDSend, (LPBYTE)pSendData, sizeof (SUBCMDPARA_DATA1));
	delete pSendData;
}


/* ========================================================================= */
/* 関数名：	SendCmdCHAR_STATEINFO											 */
/* 内容：	コマンド送信(キャラ系:状態通知)									 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

void SendCmdCHAR_STATEINFO(DWORD pIDSend, DWORD dwTblID)
{
	CCHAR_STATEINFO Packet;
	PCHARINFO pChar;

	pChar = g_aCharInfo[dwTblID];

	Packet.Make (pChar);
	svrSock.SendTo (pIDSend, Packet.Get (), Packet.GetSize (), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：SendCmdADMINUNDO													 */
/* 内容：コマンド送信(管理者画面の「元に戻す」)								 */
/* ========================================================================= */
void SendCmdADMINUNDO(DWORD pIDSend, BYTE byCmdSub, BYTE byScreen, BYTE byResult, WORD wCount, DWORD dwStepID, LPCSTR pszMsg)
{
	CMDPARA_ADMINUNDO Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_ADMINUNDO;
	Para.byCmdSub	= byCmdSub;
	Para.byScreen	= byScreen;
	Para.byResult	= byResult;
	Para.wCount		= wCount;
	Para.dwStepID	= dwStepID;
	if (pszMsg) {
		SboTextCopy (Para.szMsg, sizeof (Para.szMsg), pszMsg);
	}

	svrSock.SendTo (pIDSend, (PBYTE)&Para, sizeof (Para));
}

/* ========================================================================= */
/* 関数名：SendCmdNPCROUTE													 */
/* 内容：コマンド送信(NPCの巡回ルート)										 */
/*       dwTblID のキャラが居れば、そのルートを入れて送る					 */
/*       pszMsg はエラー文（RESULT 用）。pIDSend が 0 なら送らない			 */
/* ========================================================================= */
void SendCmdNPCROUTE(DWORD pIDSend, BYTE byCmdSub, BYTE byResult, DWORD dwMapID, DWORD dwTblID, LPCSTR pszMsg)
{
	CMDPARA_NPCROUTE Para;
	PCHARINFO pChar = NULL;

	if (pIDSend == 0) {
		return;
	}
	if (dwTblID < (DWORD)g_aCharInfo.GetSize ()) {
		pChar = g_aCharInfo[dwTblID];
	}
	NpcRouteMakePara (Para, byCmdSub, pChar);
	Para.byResult	= byResult;
	Para.dwMapID	= dwMapID;
	Para.dwTblID	= dwTblID;
	if (pszMsg) {
		SboTextCopy (Para.szMsg, sizeof (Para.szMsg), pszMsg);
	}

	svrSock.SendTo (pIDSend, (PBYTE)&Para, sizeof (Para));
}

/* ========================================================================= */
/* 関数名：SendCmdHASTE														 */
/* 内容：コマンド送信(加速の状態を本人へ。byRate=0で終わり)				 */
/* ========================================================================= */
void SendCmdHASTE(DWORD dwTblID, BYTE byRate, DWORD dwMs)
{
	CMDPARA_DATA4 SendData;
	PCHARINFO pChar = g_aCharInfo[dwTblID];

	if (pChar->byNpcType != NPCTYPE_INIT) {
		return;
	}
	ZeroMemory (&SendData, sizeof (CMDPARA_DATA4));
	SendData.byCmd		= CMD_HASTE;
	SendData.byData		= byRate;
	SendData.pID		= pChar->pID;
	SendData.dwTblID	= pChar->dwTblID;
	SendData.dwData		= dwMs;
	svrSock.SendTo (pChar->pID, (LPBYTE)&SendData, sizeof (CMDPARA_DATA4), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：SendCmdMAKERINFO													 */
/* 内容：コマンド送信(作り手情報。pIDSend は URARASOCK_SENDALL でも可)		 */
/* ========================================================================= */
void SendCmdMAKERINFO(DWORD pIDSend, WORD wID)
{
	CMDPARA_MAKERINFO Para;

	if (wID >= (WORD)g_aMakerInfo.GetSize ()) {
		return;
	}
	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd	= CMD_MAKERINFO;
	Para.wID	= wID;
	Para.wCount	= (WORD)g_aMakerInfo.GetSize ();
	Para.Info	= *g_aMakerInfo[wID];

	svrSock.SendTo (pIDSend, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：SendCmdRECIPEINFO												 */
/* 内容：コマンド送信(レシピ情報。pIDSend は URARASOCK_SENDALL でも可)		 */
/* ========================================================================= */
void SendCmdRECIPEINFO(DWORD pIDSend, WORD wID)
{
	CMDPARA_RECIPEINFO Para;

	if (wID >= (WORD)g_aRecipeInfo.GetSize ()) {
		return;
	}
	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd	= CMD_RECIPEINFO;
	Para.wID	= wID;
	Para.wCount	= (WORD)g_aRecipeInfo.GetSize ();
	Para.Info	= *g_aRecipeInfo[wID];

	svrSock.SendTo (pIDSend, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：SendCmdRECIPEINFOALL												 */
/* 内容：コマンド送信(作り手とレシピを全部。ログイン時)					 */
/* ========================================================================= */
void SendCmdRECIPEINFOALL(DWORD pIDSend)
{
	int i;

	for (i = 0; i < g_aMakerInfo.GetSize (); i ++) {
		SendCmdMAKERINFO (pIDSend, (WORD)i);
	}
	for (i = 0; i < g_aRecipeInfo.GetSize (); i ++) {
		SendCmdRECIPEINFO (pIDSend, (WORD)i);
	}
}

/* ========================================================================= */
/* 関数名：SendCmdRECIPEMAKE												 */
/* 内容：コマンド送信(作ってもらった結果を本人へ)							 */
/* ========================================================================= */
void SendCmdRECIPEMAKE(DWORD pIDSend, BYTE byResult, BYTE byMaker, WORD wRecipeID)
{
	CMDPARA_RECIPEMAKE Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_RECIPEMAKE;
	Para.byResult	= byResult;
	Para.byMaker	= byMaker;
	Para.wRecipeID	= wRecipeID;
	Para.pID		= pIDSend;

	svrSock.SendTo (pIDSend, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：SendCmdTOWNWARP													 */
/* 内容：コマンド送信(町転送の行き先一覧を本人へ。ログイン時と復活位置を	 */
/*		 記録した時)。並びは TownWarpMakeList と同じ						 */
/* ========================================================================= */
void SendCmdTOWNWARP(DWORD dwTblID)
{
	CMDPARA_TOWNWARP Para;
	PCHARINFO pChar;
	PMAPINFO pMap;
	DWORD adwMapID[MAX_TOWNWARP + 1], adwX[MAX_TOWNWARP + 1], adwY[MAX_TOWNWARP + 1];
	int i, nCount;

	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pChar = g_aCharInfo[dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0)) {
		return;
	}

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_TOWNWARP;
	nCount = TownWarpMakeList (pChar, adwMapID, adwX, adwY);
	for (i = 0; i < nCount; i ++) {
		Para.adwMapID[i] = adwMapID[i];
		if (adwMapID[i] < (DWORD)g_aMapInfo.GetSize ()) {
			pMap = g_aMapInfo[adwMapID[i]];
			if (pMap && pMap->pszName) {
				SboTextCopy (Para.aszName[i], sizeof (Para.aszName[i]), pMap->pszName);
			}
		}
	}
	Para.byCount = (BYTE)nCount;

	svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* Copyright(C)URARA-works 2003-2005 */
