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
#include <map>

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
/* 「はい・いいえ」で聞く行（byAsk=1）の保留中の問い							 */
/*   キャラごと（キー=キャラのテーブルID）に 1 つ覚える。サーバーのメモリだけ	 */
/*   （保存しない）。別の NPC に話した時・答えた時・ログアウトで消える。		 */
/*   マップ移動・空間移動・時間切れ・NPC から離れた時は、答えが来ても捨てる	 */
/* ========================================================================= */
static std::map<DWORD, TALKPEND> s_mapTalkPend;

/* 保留中の問い（無ければ NULL） */
TALKPEND *TalkPendGet(DWORD dwTblID)
{
	std::map<DWORD, TALKPEND>::iterator it = s_mapTalkPend.find (dwTblID);

	return (it == s_mapTalkPend.end ()) ? NULL : &it->second;
}

/* 保留中の問いを消す（無ければ何もしない） */
void TalkPendClear(DWORD dwTblID)
{
	s_mapTalkPend.erase (dwTblID);
}

/* ========================================================================= */
/* 関数名：TalkLineApply													 */
/* 内容：行の「話した後」の動きを反映する									 */
/*       取り上げる→渡す（QuestTalkExchange）が通った時だけ印・数値を動かす。	 */
/*       渡す品を受け取れない・取り上げる品を持っていない時は何も動かさない	 */
/*       印が動いたら、印で見える・消えるNPCを本人にだけ見え直す			 */
/* ========================================================================= */
static void TalkLineApply(PCHARINFO pChar, const TALKLINEINFO *pLine)
{
	BYTE abyFlagBak[sizeof (pChar->abyFlag)];

	CopyMemory (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak));
	if (QuestTalkExchange (pChar, pLine->wTakeItem, pLine->wGiveItem)) {
		TalkApplyLine (pLine, pChar->abyFlag, pChar->abyFlagNum);
	}
	if (memcmp (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak)) != 0) {
		CharViewRefresh (pChar);
	}
}

/* ========================================================================= */
/* 関数名：TalkNpcTalk														 */
/* 内容：会話データの NPC に話しかけた										 */
/*       選んだ行の印・数値を反映して、台詞を話しかけた本人にだけ送る		 */
/*       「はい・いいえ」で聞く行（byAsk=1）は、台詞（の最後に @y）だけ送り、	 */
/*       印・数値・渡す物はまだ反映しない。答え（TalkAnswer）が来てから反映する*/
/* ========================================================================= */
void TalkNpcTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	PTALKINFO pTalk;
	int nLine;
	char szSpeech[MAX_TALKMSG + 4];	/* 最後に @2 を足せるよう少し余分に */
	WORD wGiveItem = 0;
	BYTE abyFlagBak[sizeof (pChar->abyFlag)];
	BOOL bAsk = FALSE;
	QUESTNOTICEVEC vLater;			/* 品を渡す知らせ。窓を送った後に流す */

	if ((pChar == NULL) || (pNpc == NULL) || (pChar->pID == 0)) {
		return;
	}
	/* 別の NPC（や同じ NPC）にもう一度話したら、前の問いは消える */
	TalkPendClear (pChar->dwTblID);
	CopyMemory (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak));
	SboTextCopy (szSpeech, sizeof (szSpeech), TALK_NOANSWER);
	pTalk = TalkFind (TalkGetNo (pNpc));
	if (pTalk) {
		nLine = TalkPickLine (pTalk, pChar->abyFlag, pChar->abyFlagNum, pChar->byLevel, pChar->wQuestID, pChar->byQuestStep, pChar->waItem, pChar->waImportant, (DWORD)genrand ());
		if (nLine >= 0) {
			const TALKLINEINFO *pLine = &pTalk->aLine[nLine];

			if (pLine->byAsk) {
				/* 聞く行：台詞のあとに「はい・いいえ」を出させる（@y をクライアントが読む）。
				   @y が切れないよう、台詞は 2 文字ぶん短く収める。印・数値・渡す物は、はいの答えが来るまで動かさない */
				bAsk = TRUE;
				SboTextCopy (szSpeech, MAX_TALKMSG - 2, pLine->szSpeech[0] ? pLine->szSpeech : TALK_NOANSWER);
				strcat (szSpeech, "@y");
				{
					TALKPEND Pend;

					ZeroMemory (&Pend, sizeof (Pend));
					Pend.wTalkNo = (WORD)TalkGetNo (pNpc);
					Pend.byLine = (BYTE)nLine;
					Pend.dwNpcTblID = pNpc->dwTblID;
					Pend.dwTime = timeGetTime ();
					Pend.dwMapID = pChar->dwMapID;
					Pend.dwSpaceID = pChar->dwSpaceID;
					s_mapTalkPend[pChar->dwTblID] = Pend;
				}
			} else {
				if (pLine->szSpeech[0]) {
					SboTextCopy (szSpeech, sizeof (szSpeech), pLine->szSpeech);
				}
				/* アイテムを渡す（宝箱・依頼の報酬と同じ渡し方と知らせ）。
				   受け取れなかった時は印・数値を動かさない（一度きりのお礼が消えないように。台詞はそのまま言う） */
				wGiveItem = pLine->wGiveItem;
				/* 取り上げる品（wTakeItem）があれば、先に持っているか・渡す品を受け取れるかを確かめ、両方よければ取り上げて渡す。
				   どちらかがダメなら何も変えず印・数値も動かさない（引換券だけ取られることが無いように） */
				if (QuestTalkExchange (pChar, pLine->wTakeItem, wGiveItem, &vLater)) {
					TalkApplyLine (pLine, pChar->abyFlag, pChar->abyFlagNum);
				}
			}
		}
	}
	/* 最後が入力待ちで終わっていないと、窓がすぐ閉じて読めないので @2 を足す（道具の説明文と同じ）。
	   聞く行は最後の @y が「はい・いいえ」の待ちになるので足さない */
	if (!bAsk) {
		ItemMsgAddWait (szSpeech, sizeof (szSpeech));
	}
	SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szSpeech);
	/* 品を渡す知らせは、窓を開かせた後に送る（窓が開いたまま「手に入れました」が重ならないように） */
	QuestNoticeFlush (pChar, &vLater);
	/* 印が動いたら、印で見える・消えるNPCを、話した本人にだけ見え直す */
	if (memcmp (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak)) != 0) {
		CharViewRefresh (pChar);
	}
}

/* ========================================================================= */
/* 関数名：TalkAnswer														 */
/* 内容：「はい・いいえ」の答えを受けた（クライアントから CMD_TALKANSWER）	 */
/*       保留中の問いが有効な時だけ受ける。次のどれかなら、黙って捨てる		 */
/*         ・保留が無い・別の NPC への答え・時間切れ（TALK_ASK_TIMEOUT）		 */
/*         ・マップ・空間が変わった・NPC から TALK_ASK_RANGE マスより離れた	 */
/*         ・NPC が見えなくなった・会話セットや行が無くなった・行が聞く行で	 */
/*           なくなった														 */
/*       はい：その行がまだ条件に合う時だけ、今までと同じ反映（渡す物→成功した */
/*             時だけ印・数値）をする。合わなくなっていたら何も動かさない	 */
/*       いいえ：何も動かさず、szNoMsg（空なら TALK_NOMSG_DEFAULT）を返す	 */
/*       答えを受けた（有効だった）ら保留は消える							 */
/* ========================================================================= */
void TalkAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes)
{
	TALKPEND Pend;
	TALKPEND *pPend;
	PCHARINFO pNpc = NULL;
	PTALKINFO pTalk;
	const TALKLINEINFO *pLine;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return;
	}
	pPend = TalkPendGet (pChar->dwTblID);
	if ((pPend == NULL) || (pPend->dwNpcTblID != dwNpcTblID)) {
		return;		/* 問いが無い・別の NPC への答えは、問いを残したまま無視する */
	}
	Pend = *pPend;
	TalkPendClear (pChar->dwTblID);

	/* 時間切れ・マップや空間が違う */
	if ((DWORD)(timeGetTime () - Pend.dwTime) > (DWORD)TALK_ASK_TIMEOUT) {
		return;
	}
	if ((pChar->dwMapID != Pend.dwMapID) || (pChar->dwSpaceID != Pend.dwSpaceID)) {
		return;
	}
	/* 問いかけた NPC が、まだ同じ会話セットの NPC で、近くにいて見える */
	if (dwNpcTblID < (DWORD)g_aCharInfo.GetSize ()) {
		pNpc = g_aCharInfo[dwNpcTblID];
	}
	if ((pNpc == NULL) || !TalkIsNpc (pNpc) || (TalkGetNo (pNpc) != (int)Pend.wTalkNo)) {
		return;
	}
	if ((pNpc->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pNpc) || !IsCharSeenBy (pNpc, pChar)) {
		return;
	}
	if ((abs ((int)pChar->dwX - (int)pNpc->dwX) > TALK_ASK_RANGE) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > TALK_ASK_RANGE)) {
		return;
	}
	pTalk = TalkFind ((int)Pend.wTalkNo);
	if ((pTalk == NULL) || (Pend.byLine >= pTalk->byLineCount) || (Pend.byLine >= MAX_TALKLINE)) {
		return;
	}
	pLine = &pTalk->aLine[Pend.byLine];
	if (!pLine->byAsk) {
		return;		/* 問いの後で、管理者が聞く行をやめた */
	}

	if (!bYes) {
		char szMsg[MAX_TALKNOMSG + 4];

		SboTextCopy (szMsg, sizeof (szMsg), pLine->szNoMsg[0] ? pLine->szNoMsg : TALK_NOMSG_DEFAULT);
		ItemMsgAddWait (szMsg, sizeof (szMsg));
		SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szMsg);
		return;
	}
	/* はい：その行がまだ条件に合う時だけ（問いのあとで印・持ち物・依頼が変わっていたら何も起きない） */
	if (!TalkLineMatch (pLine, pChar->abyFlag, pChar->abyFlagNum, pChar->byLevel, pChar->wQuestID, pChar->byQuestStep, pChar->waItem, pChar->waImportant)) {
		return;
	}
	TalkLineApply (pChar, pLine);
}

/* Copyright(C)URARA-works 2003-2005 */
