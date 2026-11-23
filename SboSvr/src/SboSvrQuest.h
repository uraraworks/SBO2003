/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrQuest.h												 */
/* 概要：		ギルドの依頼の処理(定義ファイル)							 */
/* 作成日：		2026/11/10													 */
/* ========================================================================= */

#pragma once

#include <string>
#include <vector>

/* 品を渡す知らせ（白文字）を、窓を送った後まで溜めておく入れ物 */
typedef std::vector<std::string> QUESTNOTICEVEC;

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

/* 印で見える・見えないが決まるNPCか（wShowFlag・wHideFlag のどちらかに印がある）。
   人の多い所で毎回通るので、条件の無いキャラは 2 回の比べだけで返す。
   プレイヤー（byNpcType = 0）は対象外 */
inline BOOL CharHasViewCond(PCHARINFO pChar)
{
	return ((pChar->wShowFlag != 0) || (pChar->wHideFlag != 0)) && (pChar->byNpcType != 0);
}

extern BOOL CharViewCondOk(PCHARINFO pSrc, PCHARINFO pViewer);	/* 印の条件で pSrc が pViewer に見えるか（空間などは見ない） */

/* pSrc が pViewer に見えるか（印の条件だけ。条件が無ければ常に TRUE）。
   見えない人にとっては、そのNPCは居ないのと同じ（ぶつからない・話せない・攻撃できない） */
inline BOOL IsCharSeenBy(PCHARINFO pSrc, PCHARINFO pViewer)
{
	return !CharHasViewCond (pSrc) || CharViewCondOk (pSrc, pViewer);
}

/* 相手が「いまの相手にしてよいキャラ」か（依頼の見えるNPCではなく、同じ空間にいて、おたがいに見える）。
   攻撃・会話・取り引き・ついていくなど、キャラ同士の働きかけの前に確かめる */
inline BOOL IsInteractable(PCHARINFO pChar, PCHARINFO pTarget)
{
	return (pTarget != NULL) && !IsQuestNpc (pTarget) && IsSameSpace (pChar, pTarget) &&
			IsCharSeenBy (pTarget, pChar) && IsCharSeenBy (pChar, pTarget);
}

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL IsCharVisibleTo(PCHARINFO pSrc, PCHARINFO pDst);	/* pSrc の情報を pDst に送ってよいか（見えるNPC以外は常に TRUE） */
extern BOOL IsCharVisibleToPID(PCHARINFO pSrc, DWORD pIDDst);	/* 同上（受け取る人をプレイヤーIDで指定。SENDALL など該当者なしは見えるNPCだけ FALSE） */
extern void CharViewRefresh(PCHARINFO pChar);					/* 印が変わった時、その人がいる所の条件付きNPCの見え方を、その人にだけ送り直す */
extern WORD MapSwapParts(PMAPINFO pMap, PCHARINFO pViewer, int x, int y);	/* (x,y) のパーツ（印で変わるマスを pViewer の印で差し替えた後。pViewer が NULL・プレイヤー以外はベースのまま） */
extern void MapSwapSend(PCHARINFO pChar, PMAPINFO pMap, BOOL bForce);	/* 印で変わるマスの差し替え一覧を本人にだけ送る（bForce=FALSE は送るものが無ければ送らない） */
extern BOOL ChestOpenedGet(const PCHARINFO pChar, int nChestNo);	/* 開けた宝箱の記録を見る（番号 0〜2047） */
extern void ChestOpenedSet(PCHARINFO pChar, int nChestNo, BOOL bOn);	/* 開けた宝箱の記録を立てる・消す */
extern void ResendMapToPlayers(PMAPINFO pMap, int nID);			/* そのマップにいるプレイヤー全員へ、マップ情報・イベント・キャラ等をマップに入った時と同じ順で送り直す */
extern void MapSwapSendMap(PMAPINFO pMap);						/* そのマップにいるプレイヤー全員に差し替え一覧を送り直す */
extern BOOL MapSwapExamine(PCHARINFO pChar);					/* 目の前の「調べると開く」マスを調べた（品を渡して印を立てて文を出す）。処理した時 TRUE */
extern void CharViewRefreshNpc(PCHARINFO pNpc);					/* 条件付きNPCの条件を変えた時、そのマップの全員に見え直しを送る */
extern BOOL SpaceIsArena(DWORD dwSpaceID);						/* 闘技場の空間か（ふつうの世界から観戦できる） */
extern void QuestRemovingBegin(void);							/* 見えるNPCを消す間、送る相手の絞り込みを止める */
extern void QuestRemovingEnd(void);
extern BOOL QuestIsRemoving(void);
extern void QuestSyncProc(void);								/* 周期処理（約1秒ごと）。見えるNPCを作る・消す */
extern void QuestSyncAll(void);									/* 今すぐ見えるNPCを作る・消す */
extern void QuestOnDecPlayer(PCHARINFO pChar);					/* キャラが消える・ログオフする時 */
extern void QuestOnLogin(PCHARINFO pChar);						/* ログインした時（保存データの確認と進み具合の通知） */
extern void QuestSendState(PCHARINFO pChar);					/* 本人とパーティに依頼の進み具合を送る */
extern void QuestSendStateSelf(PCHARINFO pChar);				/* 本人だけに依頼と物語の「いまの目的」を送り直す（印・数値が動いた時） */
extern int  StoryGoalPick(const STORYINFO *pStory, const BYTE *pbyFlag, const BYTE *pbyFlagNum);	/* シナリオの「いまの目的」の行を選ぶ（合う行が無ければ -1） */
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
extern BOOL QuestIsRewarded(PCHARINFO pChar, WORD wQuestID);	/* 報酬をもうもらっているか（思い出の本でやり直してもクリア済みの印と違って消えない） */
extern void StoryTalk(PCHARINFO pChar, PCHARINFO pNpc);			/* 語り部（台詞 "@r"）に話しかけた（思い出の本の一覧を送る） */
extern BOOL StoryIsNpc(PCHARINFO pNpc);							/* 台詞が "@r" で始まるか */
extern BOOL QuestFlagOk(PCHARINFO pChar, const QUESTINFO *pQuest);	/* 依頼を受ける条件の印（wNeedFlag・wNotFlag）に合うか */
extern BOOL QuestLevelOk(PCHARINFO pChar, const QUESTINFO *pQuest);	/* 依頼を受ける最低レベル（byLevelMin）に合うか（管理者は常に TRUE） */
extern BOOL QuestGiveItemNotice(PCHARINFO pChar, WORD wItem, QUESTNOTICEVEC *pvLater = NULL);	/* 会話データからアイテムを 1 つ渡して知らせる（持ち物がいっぱいなら預かり所へ。渡せたら TRUE。pvLater があれば知らせは送らず溜める） */
extern void QuestNoticeFlush(PCHARINFO pChar, QUESTNOTICEVEC *pvLater);	/* 溜めた知らせを、溜めた順に送って空にする（セリフの窓を送った直後に呼ぶ） */
enum { QUESTTAKE_NONE = 0, QUESTTAKE_BAG, QUESTTAKE_IMPORTANT };	/* QuestTakeFind の戻り値（持っていない・手持ち・大事な物の欄） */
extern int  QuestTakeFind(PCHARINFO pChar, WORD wItem, int *pnIdx);	/* 会話で取り上げる品を探す（手持ち→大事な物の欄。装備中は除く） */
extern BOOL QuestCanGiveAfterTake(PCHARINFO pChar, WORD wGive, int nTakeKind);	/* nTakeKind の品を 1 つ取り上げた後で wGive を受け取れるか */
extern BOOL QuestTalkExchange(PCHARINFO pChar, WORD wTake, WORD wGive, QUESTNOTICEVEC *pvLater = NULL);	/* 会話の行の取り上げる→渡す（印・数値を動かしてよい時 TRUE。pvLater があれば知らせは送らず溜める） */
extern BOOL QuestIsQuestItem(WORD wItem);						/* 依頼の品か（預けられない・売れない・捨てられない） */
extern int  QuestCountItemAll(PCHARINFO pChar, WORD wItem);		/* 持ち物（大事な物の欄を含む）と預かり所にある数（テスト用） */
extern BOOL QuestTakeItemOne(PCHARINFO pChar, WORD wItem);			/* 1 個取り除く（大事な物の欄→持ち物→預かり所の順。テスト用） */
extern BOOL QuestInChallenge(PCHARINFO pChar);					/* 討伐チャレンジ中か（貸し出しの状態になっているか） */
extern BOOL QuestIsChallengeEnemy(PCHARINFO pChar);				/* 討伐チャレンジの敵か */
extern BOOL QuestChallengeResurrection(PCHARINFO pChar);		/* 倒れて「復活」を選んだ時。挑戦中なら中断して入る前の場所へ戻し TRUE */
extern void QuestChallengeSaveSwap(BOOL bBegin);				/* キャラ情報を保存する前(TRUE)と後(FALSE)に呼ぶ。挑戦中の人は本物の状態を保存させる */
extern void QuestChallengeProcNow(void);						/* 討伐チャレンジの周期処理を今すぐ（テスト用） */
extern void QuestChallengeOnWarp(PCHARINFO pChar);				/* マップの中のワープで動いた時（挑戦中で着いた後なら部屋を出たとして中断） */

/* 番外編ダンジョン（気まぐれダンジョンのミニゲーム。入口で姿＝テンプレートを選び、組ごとの空間へ入る） */
extern BOOL DungeonIsReceptionNpc(PCHARINFO pNpc);             /* ダンジョン受付（台詞「@d番号」）か */
extern void DungeonReceptionTalk(PCHARINFO pChar, PCHARINFO pNpc); /* 受付から姿選びを始める */
extern BOOL DungeonUsesKillAll(PSUBGAME_DUNGEONINFO pDungeon); /* 補充しない発生元が範囲内にある全敵撃破モードか */
extern BOOL DungeonFixedInside(PSUBGAME_DUNGEONINFO pDungeon, DWORD *pdwX, DWORD *pdwY);	/* 固定の形の中の開始位置を使うなら TRUE（その位置を返す）。0,0・外の入口と同じマスなら FALSE */
extern BOOL DungeonEventStep(PCHARINFO pChar, PEVENTINFO pEvent);	/* マップ間移動のイベントに乗った時の最初に呼ぶ（外の入口なら受付へ案内・中の出口なら元の姿で出す）。処理したら TRUE */
extern void DungeonSelRecv(PCHARINFO pChar, PCMDPARA_DUNGEONSEL pPara);	/* CMD_DUNGEONSEL の受信（姿を選ぶ・やめる） */
extern BOOL QuestInDungeon(PCHARINFO pChar);					/* 番外編ダンジョンの中か（貸し出しの姿になっているか） */
extern BOOL DungeonPutIsSpaced(PCHARINFO pParent);				/* NPC発生の発生元が番外編ダンジョンの範囲の中か（ふつうの世界には出さず、組の空間ごとに出す） */
extern BOOL DungeonPutOnChildDec(PCHARINFO pChild);				/* 空間の中で発生した敵が消えた。空間の敵なら TRUE（発生元の数は動かさない） */
extern void DungeonReceptionCheck(PCHARINFO pChar);
extern void DungeonProcNow(DWORD dwNow);						/* 番外編ダンジョンの周期処理を今すぐ（テスト用。時刻を指定） */
extern int  DungeonBagCount(PCHARINFO pChar);					/* 持ち帰り袋に入っている品の数（テスト用） */
extern DWORD DungeonMapSpace(PCHARINFO);
extern void DungeonMapSend(PCHARINFO);
extern void DungeonMapRecv(PCHARINFO, PCMDPARA_DUNGEONMAPDIFF, DWORD);
extern int  DungeonRunCount(void);								/* 番外編ダンジョンの空間（組）の数（テスト用） */
extern int  DungeonSelCount(void);								/* 姿を選んでいる組の数（テスト用） */
extern int  DungeonSelMemberCount(PCHARINFO pChar);				/* その人がいる選び中の組の人数（テスト用） */
extern BOOL DgIsDefenseEnemy(PCHARINFO pChar);					/* 拠点防衛の組の空間で出た敵か（NPC発生から出たキャラ。拠点・傭兵・矢は含まない。敵の行進用） */
extern PCHARINFO DgDefenseBaseOf(DWORD dwSpaceID);				/* 拠点防衛の組の空間の拠点キャラ（その空間が防衛でない・終わっていれば NULL。敵の行進用） */
extern BOOL DgDefCellOpen(PCHARINFO pChar, DWORD dwSpaceID, DWORD dwMapID, int x, int y);	/* 敵の行進：そのマスが通れるか（地形・イベント。組の空間で開いている扉は通れる） */
extern BOOL DgDefMarchDists(PCHARINFO pEnemy, int *panDist);	/* 敵の行進：今のマスと上下左右の、拠点までの歩数表の値（[0..3]=上下左右 [4]=今のマス。通れない・不明は -1。防衛でなければ FALSE） */
extern void DgDefMarchRefresh(DWORD dwSpaceID);					/* 敵の行進：詰まった時の歩数表の引き直し要求（1秒に1回まで） */
extern BOOL DgIsDefenseBase(PCHARINFO pChar);					/* 拠点防衛の組の拠点キャラか（プレイヤーからは攻撃できない・自分では攻撃しない） */
extern int  DungeonRunLeaveAll(int nDungeonID, const char *pszMsg);	/* そのダンジョンの中の組の全員に pszMsg を知らせて、外へ戻す（作り直し）。戻した人数 */

/* 空間（同じマップの同じ場所でも、別の空間の人とは出会わない。討伐チャレンジの組ごとに 1 つ作る） */
extern void SpaceSet(PCHARINFO pChar, DWORD dwSpaceID, BOOL bRefresh = TRUE);	/* 空間を移す（遊んでいる人は、前の空間の人から消える。bRefresh なら新しい空間の見え方を作り直して送る。この後マップ移動をするなら FALSE で、移動の後に SpaceSendMapState を呼ぶ） */
extern void SpaceSetQuiet(PCHARINFO pChar, DWORD dwSpaceID);	/* 空間番号だけ書き換える（まだ誰にも見えていない作りたてのキャラ・マップ移動の途中用） */
extern BOOL SpaceExists(DWORD dwSpaceID);						/* 切り替えられる空間か（0 か、いま挑戦が行われている空間） */
extern int  SpaceGetList(PADMINSPACEENTRY pEntry, int nMax);	/* いま在る空間（0 を除く）の一覧を作る。件数を返す */
extern void SpaceSendMapState(PCHARINFO pChar);					/* その人がいる空間の、扉・「乗っている間変わるマス」の状態を本人に送る */
extern BOOL SpaceDoorIsOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 扉がその空間で開いているか */
extern void SpaceDoorExtraRefresh(PMAPINFO pMap, const int *panOldXY, int nOld);	/* 扉の追加マスを外から変えた時、そのマップにいる人へ追加マスの今の絵を送り直す（panOldXY は変える前の追加マスの座標 x,y の並び。外れたマスはマップ本来の絵に戻す） */
extern BOOL SpaceDoorExtraParts(PMAPINFO pMap, DWORD dwSpaceID, int x, int y, WORD *pwParts);	/* (x,y) が扉の追加マスなら、その空間の扉の開閉に合ったパーツを返す（通れる判定用。追加マスでなければ FALSE） */
extern BOOL SpaceDoorOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 扉をその空間で開ける（開けたら TRUE。クライアントへもその空間にだけ知らせる） */
extern BOOL SpaceDoorOpenKey(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 「鍵」の扉をその空間で開ける（鍵を持っているかは呼ぶ側で確かめる） */
extern void SpaceDoorProc(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor);	/* 扉が閉まる時間を見る（ふつうの世界と、ほかの空間の分） */
extern int  SpaceDoorEnemyCount(DWORD dwMapID, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID);	/* 扉の数える範囲にいる、その空間の生きている敵の数（「敵を全部倒す」扉用） */
extern void SpaceEnemyDoorCheckMap(DWORD dwMapID);				/* そのマップの「敵を全部倒す」扉を今すぐ見直す（敵が倒れて消えた時） */
extern void SpaceOnCharMapProc(PMAPINFO pMap, PEVENTINFO pEvent, PEVENTINFO_ONCHARMAP pInfo);	/* 「乗っている間変わるマス」の処理（ふつうの世界と、ほかの空間の分） */

/* Copyright(C)URARA-works 2003-2005 */


extern BOOL DungeonPutPositionAllowed(PCHARINFO pParent, DWORD dwX, DWORD dwY); /* 組の敵は生成エリア内にだけ出す */

/* 番外編の操作制限の文（サブゲームの設定を使う） */
extern std::string DungeonBlockedText(PCHARINFO);
