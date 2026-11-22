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
void TailRelinkLeave	(PCHARINFO pChar);						/* 列の途中の人が抜ける時、後ろの人を前の人へつなぎ直す */


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
	/* 先頭が自分かもしれないのでチェック（列が輪になっていても止まるように回数を区切る） */
	for (int nLoop = 0; (pCharTmp->dwTailTblID != -1) && (pCharTmp->dwTailTblID < (DWORD)g_aCharInfo.GetSize ()); nLoop ++) {
		if ((pCharTmp->dwTailTblID == pChar->dwTblID) || (nLoop >= 100)) {
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
		/* 前の人の「後ろ」が自分の時だけ空ける（別の人を指していたら、その人が置いていかれる） */
		if (pCharTarget->dwTailBackTblID == pChar->dwTblID) {
			pCharTarget->dwTailBackTblID = -1;
		}

	/* 誰かがついてきている？ */
	} else if (pChar->dwTailBackTblID != -1) {
		pCharTarget = g_aCharInfo[pChar->dwTailBackTblID];
		pCharTarget->dwTailTblID = -1;
		pChar->dwTailBackTblID = -1;

		Packet.Make (pCharTarget->dwTblID, -1, CHAR_TAIL_OFF2);
		svrSock.SendTo (pCharTarget->pID, Packet.Get (), Packet.GetSize ());
	}
}

/* ========================================================================= */
/* 関数名：	TailRelinkLeave													 */
/* 内容：	列の途中の人がログアウトなどで抜ける時、後ろの人を前の人へ		 */
/*			つなぎ直して、列が切れないようにする（A←B←C←D で B が抜けたら	 */
/*			A←C←D）。先頭・最後尾・列に入っていない人は何もしない			 */
/*			（その時は呼び出し側の CharTargetCancelPC で外す）				 */
/*			つなぎ直しはメッセージ無し(CHAR_TAIL_ON/BACK は後ろがある時は出ない) */
/* 戻り値：	なし（つなぎ直した時は pChar の前後は -1 になる）				 */
/* ========================================================================= */

void TailRelinkLeave(PCHARINFO pChar)
{
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PCHARINFO pPrev, pNext;
	CCHAR_TAIL Packet;

	if ((pChar->dwTailTblID >= dwCount) || (pChar->dwTailBackTblID >= dwCount)) {
		return;
	}
	pPrev = g_aCharInfo[pChar->dwTailTblID];
	pNext = g_aCharInfo[pChar->dwTailBackTblID];
	/* 前後とも、ちゃんと自分とつながっている時だけ（食い違っていたら触らない） */
	if ((pPrev == pChar) || (pNext == pChar) || (pPrev == pNext)) {
		return;
	}
	if ((pPrev->dwTailBackTblID != pChar->dwTblID) || (pNext->dwTailTblID != pChar->dwTblID)) {
		return;
	}
	/* 前の人と後ろの人が別のマップ・別の空間なら、つなぎ直さず従来どおり外す */
	if ((pPrev->dwMapID != pNext->dwMapID) || !IsSameSpace (pPrev, pNext)) {
		return;
	}

	pPrev->dwTailBackTblID	= pNext->dwTblID;
	pNext->dwTailTblID		= pPrev->dwTblID;
	pChar->dwTailTblID		= (DWORD)-1;
	pChar->dwTailBackTblID	= (DWORD)-1;

	/* 後ろの人は前の人へ、前の人は後ろの人へ、画面の持ち物もつなぎ直す */
	if (pNext->pID != 0) {
		Packet.Make (pNext->dwTblID, pPrev->dwTblID, CHAR_TAIL_ON);
		svrSock.SendTo (pNext->pID, Packet.Get (), Packet.GetSize ());
	}
	if (pPrev->pID != 0) {
		Packet.Make (pPrev->dwTblID, pNext->dwTblID, CHAR_TAIL_BACK);
		svrSock.SendTo (pPrev->pID, Packet.Get (), Packet.GetSize ());
	}
}

/* Copyright(C)URARA-works 2006 */
