/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrTalk.cpp												 */
/* 概要：		キャラのフラグと会話データの処理(実装ファイル)				 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * NPC の台詞を "@tNNN"（NNN=会話セット番号）にすると、その NPC は会話データを使う。
 * 話しかけると、話した人のフラグ（印・数値）とレベルで会話セットの行を選び、
 * 選んだ行の「話した後の印・数値の動き」を反映して、その台詞を本人にだけ送る。
 *
 * 行の選び方：条件に全部合う行を集め、その中で順位（byRank）が一番小さい行たちから、
 * 同じ順位が複数ならくじ（乱数）で 1 行。合う行が無ければ「……」とだけ言う。
 * 会話データ（台詞）はクライアントへ送らない。送るのは選ばれた 1 行の台詞だけ。
 */
#include "include.h"
#include "Glbldata.h"
#include <stdio.h>
#include <string.h>

/* 合う行が無い時（会話セットが無い・削除済みも）の台詞 */
extern void ItemMsgAddWait (LPSTR, int);		/* 最後に入力待ちを足す(SboSvrItemProc.cpp) */
#define TALK_NOANSWER		"……"

/* ========================================================================= */
/* 関数名：FlagBitGet / FlagBitSet / FlagNumGet / FlagNumSet / FlagNumAdd		 */
/* 内容：フラグ（印・数値）の読み書き										 */
/* ========================================================================= */
BOOL FlagBitGet(const BYTE *pbyFlag, int nNo)
{
	if ((pbyFlag == NULL) || (nNo < 0) || (nNo >= MAX_FLAGBIT)) {
		return FALSE;
	}
	return (pbyFlag[nNo / 8] & (1 << (nNo % 8))) ? TRUE : FALSE;
}

void FlagBitSet(BYTE *pbyFlag, int nNo, BOOL bOn)
{
	if ((pbyFlag == NULL) || (nNo < 0) || (nNo >= MAX_FLAGBIT)) {
		return;
	}
	if (bOn) {
		pbyFlag[nNo / 8] |= (BYTE)(1 << (nNo % 8));
	} else {
		pbyFlag[nNo / 8] &= (BYTE)~(1 << (nNo % 8));
	}
}

BYTE FlagNumGet(const BYTE *pbyFlagNum, int nNo)
{
	if ((pbyFlagNum == NULL) || (nNo < 0) || (nNo >= MAX_FLAGNUM)) {
		return 0;
	}
	return pbyFlagNum[nNo];
}

void FlagNumSet(BYTE *pbyFlagNum, int nNo, int nVal)
{
	if ((pbyFlagNum == NULL) || (nNo < 0) || (nNo >= MAX_FLAGNUM)) {
		return;
	}
	pbyFlagNum[nNo] = (BYTE)((nVal < 0) ? 0 : (nVal > 255) ? 255 : nVal);
}

void FlagNumAdd(BYTE *pbyFlagNum, int nNo, int nVal)
{
	if ((pbyFlagNum == NULL) || (nNo < 0) || (nNo >= MAX_FLAGNUM)) {
		return;
	}
	FlagNumSet (pbyFlagNum, nNo, (int)pbyFlagNum[nNo] + nVal);
}

/* 数値を動かす（番号が「無し」・範囲の外なら何もしない） */
void FlagNumAct(BYTE *pbyFlagNum, int nNo, BYTE byOp, BYTE byVal)
{
	if ((nNo < 0) || (nNo >= MAX_FLAGNUM)) {
		return;
	}
	if (byOp == TALKNUMOP_ADD) {
		FlagNumAdd (pbyFlagNum, nNo, byVal);
	} else if (byOp == TALKNUMOP_SET) {
		FlagNumSet (pbyFlagNum, nNo, byVal);
	}
}

/* ========================================================================= */
/* 関数名：FlagApplyQuestDone												 */
/* 内容：依頼を完了した時に立てる印・数値の動きを反映する					 */
/* ========================================================================= */
void FlagApplyQuestDone(PCHARINFO pChar, const QUESTINFO *pQuest)
{
	int i;

	for (i = 0; i < MAX_QUESTDONESET; i ++) {
		if (pQuest->awDoneSetFlag[i] < MAX_FLAGBIT) {
			FlagBitSet (pChar->abyFlag, pQuest->awDoneSetFlag[i], TRUE);
		}
	}
	if (pQuest->byDoneNumNo < MAX_FLAGNUM) {
		FlagNumAct (pChar->abyFlagNum, pQuest->byDoneNumNo, pQuest->byDoneNumOp, pQuest->byDoneNumVal);
	}
}

/* ========================================================================= */
/* 関数名：TalkIsNpc / TalkGetNo / TalkFind									 */
/* 内容：会話データの NPC か・会話セット番号・会話セットを探す				 */
/* ========================================================================= */
BOOL TalkIsNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 't') &&
		(pNpc->pszNpcSpeech[2] >= '0') && (pNpc->pszNpcSpeech[2] <= '9');
}

int TalkGetNo(PCHARINFO pNpc)
{
	const char *p = pNpc->pszNpcSpeech + 2;
	int n = 0;

	while ((*p >= '0') && (*p <= '9') && (n < 10000)) {
		n = n * 10 + (*p - '0');
		p ++;
	}
	return n;
}

PTALKINFO TalkFind(int nNo)
{
	int i;

	for (i = 0; i < g_aTalkInfo.GetSize (); i ++) {
		PTALKINFO pTalk = g_aTalkInfo[i];
		if (pTalk && (pTalk->wID == nNo) && (pTalk->byState == TALKSTATE_ON)) {
			return pTalk;
		}
	}
	return NULL;
}

/* ========================================================================= */
/* 関数名：TalkLineMatch													 */
/* 内容：行の条件に合うか（印・数値・レベル・受けている依頼・持っている物）				 */
/*       範囲の外の番号（FLAG_NOBIT など）は「条件なし」として飛ばす			 */
/* ========================================================================= */
static BOOL TalkLineMatch(const TALKLINEINFO *pLine, const BYTE *pbyFlag, const BYTE *pbyFlagNum, int nLevel, WORD wQuestID, BYTE byQuestStep, const WORD *pwItem, const WORD *pwImportant)
{
	int i;

	if ((pLine->byLvMin != 0) && (nLevel < pLine->byLvMin)) {
		return FALSE;
	}
	if ((pLine->byLvMax != 0) && (nLevel > pLine->byLvMax)) {
		return FALSE;
	}
	/* 受けている依頼の条件（段の数え方・255=どの段でも は見えるNPC の byStepMin/byStepMax と同じ） */
	if (pLine->wQuestID != 0) {
		if (wQuestID != pLine->wQuestID) {
			return FALSE;
		}
		if (byQuestStep < pLine->byQuestStepMin) {
			return FALSE;
		}
		if ((pLine->byQuestStepMax != QUESTNPC_NOSTEP) && (byQuestStep > pLine->byQuestStepMax)) {
			return FALSE;
		}
	}
	/* 持っている物の条件（手持ちか大事な物の欄に 1 つ以上。預かり所は見ない） */
	if (pLine->wNeedItem != 0) {
		BOOL bHave = FALSE;

		for (i = 0; (i < MAX_MYITEM) && pwItem && !bHave; i ++) {
			bHave = (pwItem[i] == pLine->wNeedItem);
		}
		for (i = 0; (i < MAX_IMPORTANT) && pwImportant && !bHave; i ++) {
			bHave = (pwImportant[i] == pLine->wNeedItem);
		}
		if (!bHave) {
			return FALSE;
		}
	}
	for (i = 0; i < MAX_TALKNEED; i ++) {
		if ((pLine->awNeed[i] < MAX_FLAGBIT) && !FlagBitGet (pbyFlag, pLine->awNeed[i])) {
			return FALSE;
		}
		if ((pLine->awNot[i] < MAX_FLAGBIT) && FlagBitGet (pbyFlag, pLine->awNot[i])) {
			return FALSE;
		}
	}
	for (i = 0; i < MAX_TALKCOND; i ++) {
		int nVal;

		if (pLine->abyCondNo[i] >= MAX_FLAGNUM) {
			continue;
		}
		nVal = FlagNumGet (pbyFlagNum, pLine->abyCondNo[i]);
		switch (pLine->abyCondCmp[i]) {
		case TALKCMP_GE:	if (nVal < pLine->abyCondVal[i])	return FALSE;	break;
		case TALKCMP_EQ:	if (nVal != pLine->abyCondVal[i])	return FALSE;	break;
		case TALKCMP_LE:	if (nVal > pLine->abyCondVal[i])	return FALSE;	break;
		default:			return FALSE;		/* 知らない比べ方は合わないことにする */
		}
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：TalkPickLine														 */
/* 内容：条件に合う行を選ぶ（順位が一番小さい行たちから、同じ順位ならくじで 1 行）*/
/*       戻り値：行の番号（合う行が無ければ -1）							 */
/* ========================================================================= */
int TalkPickLine(const TALKINFO *pTalk, const BYTE *pbyFlag, const BYTE *pbyFlagNum, int nLevel, WORD wQuestID, BYTE byQuestStep, const WORD *pwItem, const WORD *pwImportant, DWORD dwRand)
{
	int i, nCount, nMatch = 0, nRank = 256, anMatch[MAX_TALKLINE];

	nCount = (pTalk->byLineCount > MAX_TALKLINE) ? MAX_TALKLINE : pTalk->byLineCount;
	for (i = 0; i < nCount; i ++) {
		const TALKLINEINFO *pLine = &pTalk->aLine[i];

		if (!TalkLineMatch (pLine, pbyFlag, pbyFlagNum, nLevel, wQuestID, byQuestStep, pwItem, pwImportant)) {
			continue;
		}
		if (pLine->byRank < nRank) {
			nRank = pLine->byRank;
			nMatch = 0;
		}
		if (pLine->byRank == nRank) {
			anMatch[nMatch ++] = i;
		}
	}
	if (nMatch == 0) {
		return -1;
	}
	return anMatch[dwRand % (DWORD)nMatch];
}

/* ========================================================================= */
/* 関数名：TalkApplyLine													 */
/* 内容：話した後の印・数値の動きを反映する（立てる → 消す → 数値の順）		 */
/* ========================================================================= */
void TalkApplyLine(const TALKLINEINFO *pLine, BYTE *pbyFlag, BYTE *pbyFlagNum)
{
	int i;

	for (i = 0; i < MAX_TALKSET; i ++) {
		if (pLine->awSetOn[i] < MAX_FLAGBIT) {
			FlagBitSet (pbyFlag, pLine->awSetOn[i], TRUE);
		}
	}
	for (i = 0; i < MAX_TALKSET; i ++) {
		if (pLine->awSetOff[i] < MAX_FLAGBIT) {
			FlagBitSet (pbyFlag, pLine->awSetOff[i], FALSE);
		}
	}
	for (i = 0; i < MAX_TALKACT; i ++) {
		FlagNumAct (pbyFlagNum, pLine->abyActNo[i], pLine->abyActOp[i], pLine->abyActVal[i]);
	}
}

/* ========================================================================= */
/* 関数名：TalkNpcTalk														 */
/* 内容：会話データの NPC に話しかけた										 */
/*       選んだ行の印・数値を反映して、台詞を話しかけた本人にだけ送る		 */
/* ========================================================================= */
void TalkNpcTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	PTALKINFO pTalk;
	int nLine;
	char szSpeech[MAX_TALKMSG + 4];	/* 最後に @2 を足せるよう少し余分に */
	WORD wGiveItem = 0;
	BYTE abyFlagBak[sizeof (pChar->abyFlag)];

	if ((pChar == NULL) || (pNpc == NULL) || (pChar->pID == 0)) {
		return;
	}
	CopyMemory (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak));
	SboTextCopy (szSpeech, sizeof (szSpeech), TALK_NOANSWER);
	pTalk = TalkFind (TalkGetNo (pNpc));
	if (pTalk) {
		nLine = TalkPickLine (pTalk, pChar->abyFlag, pChar->abyFlagNum, pChar->byLevel, pChar->wQuestID, pChar->byQuestStep, pChar->waItem, pChar->waImportant, (DWORD)genrand ());
		if (nLine >= 0) {
			const TALKLINEINFO *pLine = &pTalk->aLine[nLine];

			if (pLine->szSpeech[0]) {
				SboTextCopy (szSpeech, sizeof (szSpeech), pLine->szSpeech);
			}
			/* アイテムを渡す（宝箱・依頼の報酬と同じ渡し方と知らせ）。
			   受け取れなかった時は印・数値を動かさない（一度きりのお礼が消えないように。台詞はそのまま言う） */
			wGiveItem = pLine->wGiveItem;
			/* 取り上げる品（wTakeItem）があれば、先に持っているか・渡す品を受け取れるかを確かめ、両方よければ取り上げて渡す。
			   どちらかがダメなら何も変えず印・数値も動かさない（引換券だけ取られることが無いように） */
			if (QuestTalkExchange (pChar, pLine->wTakeItem, wGiveItem)) {
				TalkApplyLine (pLine, pChar->abyFlag, pChar->abyFlagNum);
			}
		}
	}
	/* 最後が入力待ちで終わっていないと、窓がすぐ閉じて読めないので @2 を足す（道具の説明文と同じ） */
	ItemMsgAddWait (szSpeech, sizeof (szSpeech));
	SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szSpeech);
	/* 印が動いたら、印で見える・消えるNPCを、話した本人にだけ見え直す */
	if (memcmp (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak)) != 0) {
		CharViewRefresh (pChar);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
