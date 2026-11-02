/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcFuncChar.cpp											 */
/* 概要：		キャラ関連の処理 実装ファイル								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2006/08/06													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "../../Common/Command/Command.h"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

void TailOn		(PCHARINFO pChar, PCHARINFO pCharTarget);		/* ついていく */
void TailOff	(PCHARINFO pChar);								/* ついていくのを解除 */


/* ========================================================================= */
/* 関数名：	TailOn															 */
/* 内容：	ついていく														 */
/* 日付：	2006/08/06														 */
/* ========================================================================= */

void TailOn(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	PCHARINFO pCharTmp;
	CCHAR_TAIL Packet;

	pCharTmp = pCharTarget;
	/* 先頭が自分かもしれないのでチェック */
	while (pCharTmp->dwTailTblID != -1) {
		if (pCharTmp->dwTailTblID == pChar->dwTblID) {
			Packet.Make (pChar->dwTblID, pCharTarget->dwTblID, CHAR_TAIL_NG2);
			svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
			return;
		}
		pCharTmp = g_aCharInfo[pCharTmp->dwTailTblID];
	}

	pChar->dwTailTblID				= pCharTarget->dwTblID;
	pCharTarget->dwTailBackTblID	= pChar->dwTblID;

	Packet.Make (pChar->dwTblID, pCharTarget->dwTblID, CHAR_TAIL_ON);
	svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());

	Packet.Make (pCharTarget->dwTblID, pChar->dwTblID, CHAR_TAIL_BACK);
	svrSock.SendTo (pCharTarget->pID, Packet.Get (), Packet.GetSize ());
}


/* ========================================================================= */
/* 関数名：	TailOff															 */
/* 内容：	ついていくのを解除												 */
/* 日付：	2006/08/06														 */
/* ========================================================================= */

void TailOff(PCHARINFO pChar)
{
	PCHARINFO pCharTarget;
	CCHAR_TAIL Packet;

	/* 誰かについていっている？ */
	if (pChar->dwTailTblID != -1) {
		pCharTarget = g_aCharInfo[pChar->dwTailTblID];

		Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_OFF);
		svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());

		pChar->dwTailTblID = -1;
		pCharTarget->dwTailBackTblID = -1;

	/* 誰かがついてきている？ */
	} else if (pChar->dwTailBackTblID != -1) {
		pCharTarget = g_aCharInfo[pChar->dwTailBackTblID];
		pCharTarget->dwTailTblID = -1;
		pChar->dwTailBackTblID = -1;

		Packet.Make (pCharTarget->dwTblID, -1, CHAR_TAIL_OFF2);
		svrSock.SendTo (pCharTarget->pID, Packet.Get (), Packet.GetSize ());
	}
}

/* Copyright(C)URARA-works 2006 */
