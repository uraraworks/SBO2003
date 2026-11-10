/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrQuest.h												 */
/* 概要：		ギルドの依頼の処理(定義ファイル)							 */
/* 作成日：		2026/11/10													 */
/* ========================================================================= */

#pragma once

/* 依頼の見えるNPCか */
inline BOOL IsQuestNpc(PCHARINFO pChar)
{
	return (pChar != NULL) && (pChar->byNpcType == NPCTYPE_QUESTNPC);
}

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL IsCharVisibleTo(PCHARINFO pSrc, PCHARINFO pDst);	/* pSrc の情報を pDst に送ってよいか（見えるNPC以外は常に TRUE） */
extern BOOL IsCharVisibleToPID(PCHARINFO pSrc, DWORD pIDDst);	/* 同上（受け取る人をプレイヤーIDで指定。SENDALL など該当者なしは見えるNPCだけ FALSE） */
extern void QuestRemovingBegin(void);							/* 見えるNPCを消す間、送る相手の絞り込みを止める */
extern void QuestRemovingEnd(void);
extern BOOL QuestIsRemoving(void);
extern void QuestSyncProc(void);								/* 周期処理（約1秒ごと）。見えるNPCを作る・消す */
extern void QuestSyncAll(void);									/* 今すぐ見えるNPCを作る・消す */
extern void QuestOnDecPlayer(PCHARINFO pChar);					/* キャラが消える・ログオフする時 */
extern void QuestOnLogin(PCHARINFO pChar);						/* ログインした時（保存データの確認と進み具合の通知） */
extern void QuestSendState(PCHARINFO pChar);					/* 本人とパーティに依頼の進み具合を送る */
extern void QuestSendStateParty(DWORD dwPartyID, PCHARINFO pExtra);	/* パーティ全員（と pExtra）に進み具合を送る */
extern void QuestNpcTalk(PCHARINFO pChar, PCHARINFO pNpc);		/* 見えるNPC に話しかけた */
extern void QuestOnStep(PCHARINFO pChar);						/* 歩いて見えるポータルに乗ったら移す */
extern void QuestGuildTalk(PCHARINFO pChar, PCHARINFO pNpc);	/* ギルドの人（台詞 "@gNN"）に話しかけた（一覧を送る） */
extern BOOL QuestIsGuildNpc(PCHARINFO pNpc);					/* 台詞が "@g" で始まるか */
extern void QuestRecv(PCHARINFO pChar, PCMDPARA_QUEST pPara);	/* CMD_QUEST の受信 */
extern BOOL QuestPartyConflict(PCHARINFO pA, PCHARINFO pB);		/* A 側と B 側が別の依頼を受けていて、同じパーティになれないか */
extern int  QuestFlagCheck(PCHARINFO pChar, int nFlg);			/* 旗が立っているか(1〜16) */
extern BOOL QuestIsDone(PCHARINFO pChar, WORD wQuestID);		/* クリア済みの印があるか */

/* Copyright(C)URARA-works 2003-2005 */
