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
extern BOOL QuestExamineHere(PCHARINFO pChar);					/* 足元を調べた時、同じマスの見える宝箱などに話しかけたことにする */
extern void QuestGuildTalk(PCHARINFO pChar, PCHARINFO pNpc);	/* ギルドの人（台詞 "@gNN"）に話しかけた（一覧を送る） */
extern BOOL QuestIsGuildNpc(PCHARINFO pNpc);					/* 台詞が "@g" で始まるか */
extern void QuestRecv(PCHARINFO pChar, PCMDPARA_QUEST pPara);	/* CMD_QUEST の受信 */
extern BOOL QuestPartyConflict(PCHARINFO pA, PCHARINFO pB);		/* A 側と B 側が別の依頼を受けていて、同じパーティになれないか */
extern int  QuestFlagCheck(PCHARINFO pChar, int nFlg);			/* 旗が立っているか(1〜16) */
extern BOOL QuestIsDone(PCHARINFO pChar, WORD wQuestID);		/* クリア済みの印があるか */
extern BOOL QuestInChallenge(PCHARINFO pChar);					/* 討伐チャレンジ中か（貸し出しの状態になっているか） */
extern BOOL QuestIsChallengeEnemy(PCHARINFO pChar);				/* 討伐チャレンジの敵か */
extern BOOL QuestChallengeResurrection(PCHARINFO pChar);		/* 倒れて「復活」を選んだ時。挑戦中なら中断して入る前の場所へ戻し TRUE */
extern void QuestChallengeSaveSwap(BOOL bBegin);				/* キャラ情報を保存する前(TRUE)と後(FALSE)に呼ぶ。挑戦中の人は本物の状態を保存させる */
extern void QuestChallengeProcNow(void);						/* 討伐チャレンジの周期処理を今すぐ（テスト用） */
extern void QuestChallengeOnWarp(PCHARINFO pChar);				/* マップの中のワープで動いた時（挑戦中で着いた後なら部屋を出たとして中断） */

/* Copyright(C)URARA-works 2003-2005 */
