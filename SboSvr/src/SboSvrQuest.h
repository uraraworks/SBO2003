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

/* 同じ空間にいるか（空間番号が同じか。どちらかが NULL なら FALSE）。
   空間 0 がふつうの世界。空間が違うキャラ同士は、見えない・ぶつからない・攻撃や会話ができない */
inline BOOL IsSameSpace(PCHARINFO pA, PCHARINFO pB)
{
	return (pA != NULL) && (pB != NULL) && (pA->dwSpaceID == pB->dwSpaceID);
}

/* 相手が「いまの相手にしてよいキャラ」か（依頼の見えるNPCではなく、同じ空間にいる）。
   攻撃・会話・取り引き・ついていくなど、キャラ同士の働きかけの前に確かめる */
inline BOOL IsInteractable(PCHARINFO pChar, PCHARINFO pTarget)
{
	return (pTarget != NULL) && !IsQuestNpc (pTarget) && IsSameSpace (pChar, pTarget);
}

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL IsCharVisibleTo(PCHARINFO pSrc, PCHARINFO pDst);	/* pSrc の情報を pDst に送ってよいか（見えるNPC以外は常に TRUE） */
extern BOOL IsCharVisibleToPID(PCHARINFO pSrc, DWORD pIDDst);	/* 同上（受け取る人をプレイヤーIDで指定。SENDALL など該当者なしは見えるNPCだけ FALSE） */
extern BOOL SpaceIsArena(DWORD dwSpaceID);						/* 闘技場の空間か（ふつうの世界から観戦できる） */
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
extern const char *QuestPartyConflictMsg(PCHARINFO pA, PCHARINFO pB);	/* 仲間になれない理由（なれるなら NULL） */
extern void QuestOnPartyLeave(PCHARINFO pChar);					/* パーティを抜けた・解散した時（討伐チャレンジの依頼を取り消す） */
extern void QuestPartyLeaveHold(BOOL bHold);					/* TRUE の間は QuestOnPartyLeave で取り消さない（切断の時） */
extern int  QuestFlagCheck(PCHARINFO pChar, int nFlg);			/* 旗が立っているか(1〜16) */
extern BOOL QuestIsDone(PCHARINFO pChar, WORD wQuestID);		/* クリア済みの印があるか */
extern BOOL QuestIsQuestItem(WORD wItem);						/* 依頼の品か（預けられない・売れない・捨てられない） */
extern BOOL QuestInChallenge(PCHARINFO pChar);					/* 討伐チャレンジ中か（貸し出しの状態になっているか） */
extern BOOL QuestIsChallengeEnemy(PCHARINFO pChar);				/* 討伐チャレンジの敵か */
extern BOOL QuestChallengeResurrection(PCHARINFO pChar);		/* 倒れて「復活」を選んだ時。挑戦中なら中断して入る前の場所へ戻し TRUE */
extern void QuestChallengeSaveSwap(BOOL bBegin);				/* キャラ情報を保存する前(TRUE)と後(FALSE)に呼ぶ。挑戦中の人は本物の状態を保存させる */
extern void QuestChallengeProcNow(void);						/* 討伐チャレンジの周期処理を今すぐ（テスト用） */
extern void QuestChallengeOnWarp(PCHARINFO pChar);				/* マップの中のワープで動いた時（挑戦中で着いた後なら部屋を出たとして中断） */

/* 空間（同じマップの同じ場所でも、別の空間の人とは出会わない。討伐チャレンジの組ごとに 1 つ作る） */
extern void SpaceSet(PCHARINFO pChar, DWORD dwSpaceID, BOOL bRefresh = TRUE);	/* 空間を移す（遊んでいる人は、前の空間の人から消える。bRefresh なら新しい空間の見え方を作り直して送る。この後マップ移動をするなら FALSE で、移動の後に SpaceSendMapState を呼ぶ） */
extern void SpaceSetQuiet(PCHARINFO pChar, DWORD dwSpaceID);	/* 空間番号だけ書き換える（まだ誰にも見えていない作りたてのキャラ・マップ移動の途中用） */
extern BOOL SpaceExists(DWORD dwSpaceID);						/* 切り替えられる空間か（0 か、いま挑戦が行われている空間） */
extern int  SpaceGetList(PADMINSPACEENTRY pEntry, int nMax);	/* いま在る空間（0 を除く）の一覧を作る。件数を返す */
extern void SpaceSendMapState(PCHARINFO pChar);					/* その人がいる空間の、扉・「乗っている間変わるマス」の状態を本人に送る */
extern BOOL SpaceDoorIsOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 扉がその空間で開いているか */
extern BOOL SpaceDoorOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 扉をその空間で開ける（開けたら TRUE。クライアントへもその空間にだけ知らせる） */
extern void SpaceDoorProc(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor);	/* 扉が閉まる時間を見る（ふつうの世界と、ほかの空間の分） */
extern void SpaceOnCharMapProc(PMAPINFO pMap, PEVENTINFO pEvent, PEVENTINFO_ONCHARMAP pInfo);	/* 「乗っている間変わるマス」の処理（ふつうの世界と、ほかの空間の分） */

/* Copyright(C)URARA-works 2003-2005 */
