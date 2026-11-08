/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrEventProc.cpp											 */
/* 概要：		イベント処理												 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/20													 */
/* ========================================================================= */
#include "include.h"
#include "../../Common/Info/InfoJob.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "CharProc/CharProcMERCENARY.h"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void EventProc(PCHARINFO, PEVENTINFO, BYTE);							/* イベント処理 */
static void EventProc_MOVE			(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理(移動) */
static void EventProc_MAPCHG		(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理(マップ間移動) */
static void EventProc_MSG			(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理(メッセージ) */
static void EventProc_HPMANA		(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理(HP/Mana回復) */
static void EventProc_STATUSINIT	(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理(ステータス初期化) */

extern long genrand();													/* 乱数取得 */

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef void (*EVENTPROC)(PCHARINFO, PEVENTINFO, BYTE);

/* 関数ジャンプテーブル */
static const EVENTPROC EventProcTbl[] = {
	EventProc_MOVE,					/* 移動 */
	EventProc_MAPCHG,				/* マップ間移動 */
	EventProc_MSG,					/* メッセージ */
	NULL,							/* 扉 */
	EventProc_HPMANA,				/* HP/Mana回復 */
	EventProc_STATUSINIT,			/* ステータス初期化 */
	NULL,							/* キャラが乗っている間マップ変更 */
};

/* ========================================================================= */
/* 関数名：EventProc														 */
/* 内容：イベント処理														 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void EventProc(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	if ((pEvent->byType != EVENTTYPE_MSG) && byData) {
		SendCmdMAPMSG (pChar->pID, "何も見つかりませんでした。@2");
		return;
	}

	/* 種類別に処理 */
	if (EventProcTbl[pEvent->byType]) {
		EventProcTbl[pEvent->byType] (pChar, pEvent, byData);

	} else {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	}
}

/* ========================================================================= */
/* 関数名：EventProc_MOVE													 */
/* 内容：イベント処理(移動)													 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
void EventProc_MOVE(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	PEVENTINFO_MOVE pMove;
	PCHARINFO pCharTmp;

	pMove = (PEVENTINFO_MOVE)pEvent->pData;
	if (!pMove) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}

	svrSock.SendCancel (pChar->pID);

	/* 誰かついてきている？ */
	if (pChar->dwTailBackTblID != -1) {
		pCharTmp = g_aCharInfo[pChar->dwTailBackTblID];
		/* ついてきているキャラを移動させる */
		EventProc_MOVE (pCharTmp, pEvent, byData);
	}

	pChar->byDirectionBack2 = pChar->byDirection;
	pChar->byDirectionBack = pChar->byDirection;
	pChar->dwX = pMove->dwX;
	pChar->dwY = pMove->dwY;
	pChar->dwXBack = pChar->dwX;
	pChar->dwYBack = pChar->dwY;

	switch (pMove->byEfect) {
	case EVENTMOVEEFECT_MOVE:
		SendCmdPLAYSOUND (pChar->pID, SOUND_MOVE);
		break;
	case EVENTMOVEEFECT_DOWN:
		SendCmdPLAYSOUND (pChar->pID, SOUND_DOWN);
		break;
	case EVENTMOVEEFECT_SEE:
		SendCmdPLAYSOUND (pChar->pID, SOUND_SEE);
		break;
	}

	SendCmdFADEINOUT (pChar->pID, 0);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdFADEINOUT (pChar->pID, 1);
}

/* ========================================================================= */
/* 関数名：EventProc_MAPCHG													 */
/* 内容：イベント処理(マップ間移動)											 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
void EventProc_MAPCHG(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	PEVENTINFO_MAPCHG pMap = (PEVENTINFO_MAPCHG)pEvent->pData;
	PCHARINFO pCharTmp;

	if (!pMap) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}

	svrSock.SendCancel (pChar->pID);

	/* 誰かついてきている？ */
	if (pChar->dwTailBackTblID != -1) {
		pCharTmp = g_aCharInfo[pChar->dwTailBackTblID];
		/* 傭兵は別のマップへは連れて行かない（列から外し、傭兵の処理でお別れ） */
		if (IsMercenary (pCharTmp)) {
			MercenaryTailOff (pCharTmp);
		} else {
			/* ついてきているキャラを移動させる */
			EventProc_MAPCHG (pCharTmp, pEvent, byData);
		}
	}

	SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);

	switch (pMap->byEfect) {
	case EVENTMOVEEFECT_MOVE:
		SendCmdPLAYSOUND (pChar->pID, SOUND_MOVE);
		break;
	case EVENTMOVEEFECT_DOWN:
		SendCmdPLAYSOUND (pChar->pID, SOUND_DOWN);
		break;
	case EVENTMOVEEFECT_SEE:
		SendCmdPLAYSOUND (pChar->pID, SOUND_SEE);
		break;
	}

	SendCmdFADEINOUT (pChar->pID, 0);
	SubProcMapMove (pChar, pMap->dwMapID, pMap->dwX, pMap->dwY);
}

/* ========================================================================= */
/* 関数名：EventProc_MSG													 */
/* 内容：イベント処理(メッセージ)											 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
void EventProc_MSG(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	PEVENTINFO_MSG pMsg = (PEVENTINFO_MSG)pEvent->pData;
	if (!pMsg) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}

	if (byData != pMsg->byType) {
		if (byData) {
			SendCmdMAPMSG (pChar->pID, "何も見つかりませんでした。@2");
		} else {
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		}
		return;
	}

	switch (pMsg->byType) {
	case 0:
	case 1:
	case 2:
		{
			PMAPINFO pMap = g_aMapInfo[GetMapNo (pChar->dwMapID)];
			if (pMap->nMsgCount <= (int)pMsg->dwNo) {
				break;
			}
			DWORD dwNo = pMsg->dwNo;
			LPSTR pMsgTmp = pMap->pMsg[dwNo];
			WORD wMap = pMap->pwMap[pMsg->dwX + pMsg->dwY * pMap->nCX];
			/* マップ書き換えあり？ */
			if (pMsg->wGrpID) {
				if (pMsg->wGrpID != wMap) {
					pMap->pwMap[pMsg->dwX + pMsg->dwY * pMap->nCX] = pMsg->wGrpID;
					SendCmdCHGMAP (pMap->dwID, pMsg->wGrpID, pMsg->dwX, pMsg->dwY);
				} else {
					SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
					break;
				}
			}
			SendCmdMAPMSG (pChar->pID, pMsgTmp);
		}
		break;

	default:
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		break;
	}
}

/* ========================================================================= */
/* 関数名：EventProc_HPMANA													 */
/* 内容：イベント処理(HP/Mana回復)											 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
void EventProc_HPMANA(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	PEVENTINFO_HPMANA pHpMana = (PEVENTINFO_HPMANA)pEvent->pData;
	if (pHpMana == NULL) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}
	int nTmp = 0;
	char szTmp[64];

	switch (pHpMana->byType) {
	case EVENTHPMANATYPE_HP:
		nTmp = pChar->wMaxHP - pChar->wHP;
		if (nTmp == 0) {
			break;
		}
		pChar->wHP = pChar->wMaxHP;
		SendCmdCHARSTATUSHPMANA (pChar->dwTblID, pChar->dwTblID);
		wsprintf (szTmp, "%sのHPは完全に回復しました。", pChar->szName);
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		wsprintf (szTmp, "%d", nTmp);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);
		pChar->byEfect = EFECTTYPE_HOIMI;
		SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
		pChar->byEfect = 0;
		break;
	case EVENTHPMANATYPE_MANA:
		nTmp = pChar->wMaxMana - pChar->wMana;
		if (nTmp == 0) {
			break;
		}
		pChar->wMana = pChar->wMaxMana;
		SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
		wsprintf (szTmp, "%sのManaは完全に回復しました。", pChar->szName);
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		wsprintf (szTmp, "%d", nTmp);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);
		pChar->byEfect = EFECTTYPE_HOIMI;
		SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
		pChar->byEfect = 0;
		break;
	case EVENTHPMANATYPE_HPMANA:
		nTmp = pChar->wMaxHP - pChar->wHP;
		if (nTmp) {
			pChar->wHP = pChar->wMaxHP;
			SendCmdCHARSTATUSHPMANA (pChar->dwTblID, pChar->dwTblID);
			wsprintf (szTmp, "%sのHPは完全に回復しました。", pChar->szName);
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
			wsprintf (szTmp, "%d", nTmp);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);
			pChar->byEfect = EFECTTYPE_HOIMI;
			SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
			pChar->byEfect = 0;
		}
		nTmp = pChar->wMaxMana - pChar->wMana;
		if (nTmp) {
			pChar->wMana = pChar->wMaxMana;
			SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
			wsprintf (szTmp, "%sのManaは完全に回復しました。", pChar->szName);
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
			wsprintf (szTmp, "%d", nTmp);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);
			pChar->byEfect = EFECTTYPE_HOIMI;
			SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
			pChar->byEfect = 0;
		}
		break;
	}
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
}

/* ========================================================================= */
/* 関数名：EventProc_STATUSINIT												 */
/* 内容：イベント処理(ステータス初期化)										 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
void EventProc_STATUSINIT(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData)
{
	PJOBINFO pJobInfo;
	PLVUPINFO pLvUpInfo;

	pChar->byLevel	= 1;
	pChar->byPW		= 1;
	pChar->byDEX	= 1;
	pChar->byST		= 1;
	pChar->byIT		= 1;
	pChar->byLK		= 1;
	pChar->wMaxHP	= 20;
	pChar->wHP		= 20;
	pChar->wMaxMana	= 10;
	pChar->wMana	= 10;
	pChar->dwExp	= 0;

	pJobInfo = g_JobInfo.GetPtr ((int)pChar->byJOB);
	if (pJobInfo) {
		if (pJobInfo->aLevelUpInfo.GetSize () > 0) {
			pLvUpInfo = pJobInfo->aLevelUpInfo[0];
			pChar->byPW		= pLvUpInfo->byPW;
			pChar->byDEX	= pLvUpInfo->byDEX;
			pChar->byST		= pLvUpInfo->byST;
			pChar->byIT		= pLvUpInfo->byIT;
			pChar->byLK		= pLvUpInfo->byLK;
			pChar->wMaxHP	= pLvUpInfo->wMAXHP;
			pChar->wHP		= pLvUpInfo->wMAXHP;
			pChar->wMaxMana	= pLvUpInfo->wMAXMANA;
			pChar->wMana	= pLvUpInfo->wMAXMANA;
		}
	}

	RenewATDF (pChar->dwTblID);
	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
	pChar->byEfect = EFECTTYPE_STSTUS;
	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
	pChar->byEfect = 0;
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
}
/* Copyright(C)URARA-works 2003-2005 */
