/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrQuest.cpp												 */
/* 概要：		ギルドの依頼の処理(実装ファイル)							 */
/* 作成日：		2026/11/10													 */
/* ========================================================================= */
/*
 * 流れ：ギルドの人（台詞 "@gNN"）に話す → 一覧 → 受ける。
 * 受けた人とそのパーティだけに「見えるNPC」（台詞 "@q"）が出て、話すと
 * ポータルで移る・宝箱でアイテムをもらう、などができる。ギルドに戻ると報酬。
 * 見えるNPC は保存せず、約 1 秒ごとの同期（QuestSyncProc）で作る・消す。
 * 見えるNPC の情報は IsCharVisibleTo を通った相手にだけ送る。
 *
 * 討伐チャレンジ（QUESTINFO の byChallenge）：入口の見えるNPC に乗ると、近くで同じ依頼を
 * 受けた仲間と一緒に行き先へ移り、全員が貸し出しセット（ステータステンプレート）の状態になる。
 * 本物の状態はメモリの控え（s_mapLoan）に置き、保存する時は本物を書く（SaveCharInfo が
 * QuestChallengeSaveSwap を前後で呼ぶ）。倒れる・パーティを抜ける・切断・行き先から出ると、
 * その人だけ中断して元に戻る。出ている敵を全部倒すと全員元に戻って最後の段へ。
 */
#include "include.h"
#include "Glbldata.h"
#include "CharProc/IncludeCharProc.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrEventProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>

extern void SendCmdMAPCHARINFO(DWORD);

/* 見えるNPC 1 体の持ち主・出どころ */
typedef struct _QNPCENTRY {
	DWORD		dwOwner;											/* 持ち主（依頼を受けた人）のテーブルID */
	WORD		wQuestID;											/* 依頼番号 */
	int			nIdx;												/* 依頼の見えるNPC の何番目か */
	DWORD		dwKey;												/* 見える人の組み合わせ（変わったら作り直す） */
} QNPCENTRY;

static std::map<DWORD, QNPCENTRY> s_mapNpc;						/* NPC のテーブルID → 出どころ */
static int s_nRemoving = 0;										/* 消している間は送る相手を絞らない */
/* 光に乗って、移るのを待っている人 */
typedef struct _STEPWAIT {
	DWORD		dwNpcTblID;											/* 乗った光（見えるNPC）のテーブルID */
	DWORD		dwTime;												/* 乗った時刻 */
	DWORD		dwMapID, dwX, dwY;									/* 乗った場所（動いたらやめる） */
} STEPWAIT;
static std::map<DWORD, STEPWAIT> s_mapStepWait;					/* 乗った人のテーブルID → 待ち */
static DWORD s_dwLastSync = 0;									/* 最後に同期した時刻 */

#define QUEST_SYNC_INTERVAL		1000								/* 同期の間隔（ミリ秒） */
#define QUEST_STEP_WAIT			1000								/* 光（ポータル・入口）に乗ってから移るまでの間（ミリ秒。歩き終わる前に移らないように） */
#define QUEST_GOLD_MAX			999999								/* お金の上限 */
#define QUEST_FLG_CLEARED		16									/* 討伐チャレンジに成功した印の旗（ギルドで渡せる条件） */
#define CHALLENGE_NEAR			8									/* 一緒に受ける・入る仲間の距離（マス） */
#define CHALLENGE_ARRIVE_WAIT	10000								/* 入ってから行き先に着くまで待つ時間（ミリ秒） */
#define CHALLENGE_CLEAR_WAIT	3000								/* 全部倒してから元の場所へ戻すまでの間（ミリ秒。何が起きたか分かるように） */

/* 討伐チャレンジ中の人の、本物の状態の控え */
typedef struct _LOANBAK {
	DWORD		dwSession;											/* 挑戦の番号 */
	BOOL		bArrived;											/* 行き先のマップに着いたか */
	BOOL		bWarped;											/* 着いた後にマップの中のワープで動いた（部屋を出た） */
	BYTE		byLevel, byPW, byDEX, byST, byIT, byLK;
	BYTE		abyMagic[MAX_USEMAGIC];
	WORD		waItem[MAX_MYITEM];
	WORD		waEqu[MAX_EQUTYPE];
	WORD		wMaxHP, wHP, wMaxMana, wMana;
	DWORD		dwGold, dwExp;
	char		chAttrFire, chAttrWater, chAttrWind, chAttrEarth, chAttrLight, chAttrDark;
	DWORD		dwMapID, dwX, dwY;									/* 入る前にいた場所（戻る場所・保存する場所） */
	BYTE		byDirection;
} LOANBAK;

/* 1 回の挑戦（一緒に入った組） */
typedef struct _CHALLENGE {
	WORD		wQuestID;											/* 依頼番号 */
	DWORD		dwMapNo;											/* 戦う場所のマップ番号 */
	DWORD		dwSpaceID;											/* この挑戦の空間番号（挑戦の番号と同じ。0 にはならない。組ごとに別の空間で戦う） */
	DWORD		dwPartyID;											/* 入った時のパーティ（変わった人は中断） */
	DWORD		dwStart;											/* 始めた時刻 */
	BOOL		bCleared;											/* 敵を全部倒した */
	DWORD		dwClear;											/* 全部倒したと知らせた時刻（0=まだ） */
	DWORD		dwClearTime;										/* かかった時間（ミリ秒） */
	std::vector<DWORD>	aMember;									/* 挑戦中の人（テーブルID） */
	std::vector<DWORD>	aEnemy;										/* 残っている敵（テーブルID） */
} CHALLENGE;

/* 戻す場所（切断した人は、マップの表から外れてから動かす） */
typedef struct _LOANPOS {
	DWORD		dwMapID, dwX, dwY;
	BYTE		byDirection;
} LOANPOS;

static std::map<DWORD, LOANBAK> s_mapLoan;						/* 挑戦中の人のテーブルID → 本物の控え */
static std::map<DWORD, CHALLENGE> s_mapChallenge;				/* 挑戦の番号 → 挑戦 */
static std::map<DWORD, DWORD> s_mapEnemy;						/* 敵のテーブルID → 挑戦の番号 */
static std::map<DWORD, LOANPOS> s_mapPendingPos;				/* 切断した人のテーブルID → 戻す場所 */
static DWORD s_dwChallengeSeq = 0;								/* 挑戦の番号の採番 */

static void ChallengeProc(void);
static void StepWaitProc(DWORD dwNow);
static void ChallengeOnDecPlayer(PCHARINFO pChar);
static void ChallengeClearMember(PCHARINFO p, struct _CHALLENGE *pCh, int nReason);
static void ChallengeStart(PCHARINFO pChar, PQUESTINFO pQuest, PQUESTNPCINFO pDef);
static void GetChallengeMembers(PCHARINFO pChar, WORD wQuestID, std::vector<PCHARINFO> &aOut);

/* ========================================================================= */
/* 小さな道具																 */
/* ========================================================================= */

/* 依頼を番号から探す */
static PQUESTINFO FindQuest(WORD wID)
{
	int i;
	if (wID == 0) {
		return NULL;
	}
	for (i = 0; i < g_aQuestInfo.GetSize (); i ++) {
		if (g_aQuestInfo[i]->wID == wID) {
			return g_aQuestInfo[i];
		}
	}
	return NULL;
}

/* 文字列を欄の大きさに収めてコピー（終端を必ず付ける） */
static void CopyStr(char *pszDst, size_t nSize, const char *pszSrc)
{
	strncpy (pszDst, pszSrc ? pszSrc : "", nSize - 1);
	pszDst[nSize - 1] = 0;
}

/* 遊んでいる人（プレイヤー）か */
static BOOL IsPlayerOnline(PCHARINFO pChar)
{
	return pChar && (pChar->byNpcType == NPCTYPE_INIT) && (pChar->pID != 0) &&
		(pChar->byGameState >= GAMESTATE_MOVE) && (pChar->byGameState != GAMESTATE_NPC);
}

/* パーティの仲間（本人も入る）。本人を先頭にしてオンラインの人だけ集める */
static void GetPartyMembers(PCHARINFO pChar, std::vector<PCHARINFO> &aOut)
{
	int i;
	aOut.clear ();
	aOut.push_back (pChar);
	if ((pChar->dwPartyID == 0) || (pChar->dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return;
	}
	PPARTYINFO pParty = g_aPartyInfo[pChar->dwPartyID];
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		DWORD dwTbl = pParty->adwTblID[i];
		if (dwTbl == 0) {
			break;
		}
		if ((dwTbl >= (DWORD)g_aCharInfo.GetSize ()) || (dwTbl == pChar->dwTblID)) {
			continue;
		}
		if (IsPlayerOnline (g_aCharInfo[dwTbl]) && (g_aCharInfo[dwTbl]->dwPartyID == pChar->dwPartyID)) {
			aOut.push_back (g_aCharInfo[dwTbl]);
		}
	}
}

/* 旗(1〜16)が立っているか */
int QuestFlagCheck(PCHARINFO pChar, int nFlg)
{
	if ((nFlg < 1) || (nFlg > 16)) {
		return 0;
	}
	return (pChar->wQuestFlg & (1 << (nFlg - 1))) ? 1 : 0;
}

/* クリア済みの印 */
BOOL QuestIsDone(PCHARINFO pChar, WORD wQuestID)
{
	if (wQuestID >= MAX_QUEST) {
		return FALSE;
	}
	return (pChar->abyQuestDone[wQuestID / 8] & (1 << (wQuestID % 8))) ? TRUE : FALSE;
}

/* 受けている依頼を空にする（クリア済みの印は残す） */
static void ClearQuest(PCHARINFO pChar)
{
	pChar->wQuestID = 0;
	pChar->byQuestStep = 0;
	pChar->wQuestFlg = 0;
	ZeroMemory (pChar->awQuestCount, sizeof (pChar->awQuestCount));
	pChar->dwQuestAcceptTime = 0;
}

/* 段の「いまの目的」 */
static const char *GetGoal(PQUESTINFO pQuest, BYTE byStep)
{
	if ((pQuest == NULL) || (byStep >= MAX_QUESTSTEP)) {
		return "";
	}
	return pQuest->aszGoal[byStep];
}

/* ========================================================================= */
/* 見えるかどうか															 */
/* ========================================================================= */

/* pSrc（見えるNPC）が、依頼を受けた人・その仲間として pDst のものか（管理者の特別扱いなし） */
static BOOL IsQuestNpcFor(PCHARINFO pSrc, PCHARINFO pDst)
{
	std::map<DWORD, QNPCENTRY>::iterator it;
	PCHARINFO pOwner;

	if (pDst == NULL) {
		return FALSE;
	}
	it = s_mapNpc.find (pSrc->dwTblID);
	if ((it == s_mapNpc.end ()) || (it->second.dwOwner >= (DWORD)g_aCharInfo.GetSize ())) {
		return FALSE;
	}
	pOwner = g_aCharInfo[it->second.dwOwner];
	if (pDst == pOwner) {
		return TRUE;
	}
	/* 同じ依頼を受けている仲間には、その人自身の分だけ見える */
	if ((pDst->wQuestID != 0) && (pDst->wQuestID == it->second.wQuestID)) {
		return FALSE;
	}
	/* 依頼を受けていない仲間には、パーティの誰かの分が見える */
	return (pDst->dwPartyID != 0) && (pDst->dwPartyID == pOwner->dwPartyID);
}

/* pSrc の情報を pDst に送ってよいか */
BOOL IsCharVisibleTo(PCHARINFO pSrc, PCHARINFO pDst)
{
	/* 空間が違えば見えない（管理者も、いま居る空間のものしか見えない） */
	if (pDst != NULL) {
		if (pSrc->dwSpaceID != pDst->dwSpaceID) {
			return FALSE;
		}
	} else if (pSrc->dwSpaceID != 0) {
		return FALSE;
	}
	if (!IsQuestNpc (pSrc)) {
		return TRUE;
	}
	if (s_nRemoving) {
		return TRUE;
	}
	if (pDst == NULL) {
		return FALSE;
	}
	/* 管理者には、同じ空間の分が全部見える（見えるだけ。光で移る・宝箱を開けるのは受けた人と仲間だけ） */
	if (pDst->byAdminLV != ADMINLV_USER) {
		return TRUE;
	}
	return IsQuestNpcFor (pSrc, pDst);
}

/* 依頼の品か（どれかの依頼の「持ち帰る品」か、見える宝箱の中身）
   依頼の品は預かり所に預けられない・売れない・捨てられない */
BOOL QuestIsQuestItem(WORD wItem)
{
	int i, j;

	if (wItem == 0) {
		return FALSE;
	}
	for (i = 0; i < g_aQuestInfo.GetSize (); i ++) {
		PQUESTINFO pQuest = g_aQuestInfo[i];
		if (pQuest->wQuestItem == wItem) {
			return TRUE;
		}
		for (j = 0; j < MAX_QUESTNPC; j ++) {
			if ((pQuest->aNpc[j].byKind == QUESTNPCKIND_CHEST) && (pQuest->aNpc[j].wItem == wItem)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* 同上（相手をプレイヤーIDで指定） */
BOOL IsCharVisibleToPID(PCHARINFO pSrc, DWORD pIDDst)
{
	int nNo;

	/* ふつうの世界にいる、見えるNPC以外は誰にでも送ってよい（人の多い所で毎回通るので先に返す） */
	if (!IsQuestNpc (pSrc) && (pSrc->dwSpaceID == 0)) {
		return TRUE;
	}
	if (IsQuestNpc (pSrc) && s_nRemoving) {
		return TRUE;
	}
	nNo = GetCharNo (pIDDst);
	if (nNo < 0) {
		return FALSE;
	}
	return IsCharVisibleTo (pSrc, g_aCharInfo[nNo]);
}

void QuestRemovingBegin(void)	{ s_nRemoving ++; }
void QuestRemovingEnd(void)		{ if (s_nRemoving > 0) s_nRemoving --; }
BOOL QuestIsRemoving(void)		{ return s_nRemoving > 0; }

/* ========================================================================= */
/* 進み具合の通知															 */
/* ========================================================================= */

/* 1 人分の進み具合を詰める */
static void FillEntry(QUESTSTATEENTRY *pEntry, PCHARINFO pChar)
{
	PQUESTINFO pQuest = FindQuest (pChar->wQuestID);

	ZeroMemory (pEntry, sizeof (*pEntry));
	pEntry->dwTblID = pChar->dwTblID;
	CopyStr (pEntry->szCharName, sizeof (pEntry->szCharName), pChar->szName);
	if (pQuest) {
		pEntry->wQuestID = pChar->wQuestID;
		pEntry->byStep = pChar->byQuestStep;
		CopyStr (pEntry->szQuestName, sizeof (pEntry->szQuestName), pQuest->szName);
		CopyStr (pEntry->szGoal, sizeof (pEntry->szGoal), GetGoal (pQuest, pChar->byQuestStep));
		memcpy (pEntry->awCount, pChar->awQuestCount, sizeof (pEntry->awCount));
	}
}

/* 1 人に、その人とパーティの進み具合を送る */
static void SendStateTo(PCHARINFO pTo)
{
	CMDPARA_QUESTSTATE Para;
	std::vector<PCHARINFO> aMember;
	size_t i;

	if (!IsPlayerOnline (pTo)) {
		return;
	}
	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_QUESTSTATE;
	GetPartyMembers (pTo, aMember);
	for (i = 0; i < aMember.size () && Para.byCount < MAX_PARTYMEMBER; i ++) {
		/* 本人は受けていなくても必ず入れる。仲間は受けている人だけ */
		if ((i > 0) && (FindQuest (aMember[i]->wQuestID) == NULL)) {
			continue;
		}
		FillEntry (&Para.aEntry[Para.byCount], aMember[i]);
		Para.byCount ++;
	}
	svrSock.SendTo (pTo->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* 本人とパーティに送る */
void QuestSendState(PCHARINFO pChar)
{
	std::vector<PCHARINFO> aMember;
	size_t i;

	GetPartyMembers (pChar, aMember);
	for (i = 0; i < aMember.size (); i ++) {
		SendStateTo (aMember[i]);
	}
}

/* パーティ全員（と pExtra）に送る（抜けた人・解散した人にも届ける） */
void QuestSendStateParty(DWORD dwPartyID, PCHARINFO pExtra)
{
	int i;

	if (pExtra) {
		SendStateTo (pExtra);
	}
	if ((dwPartyID == 0) || (dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return;
	}
	PPARTYINFO pParty = g_aPartyInfo[dwPartyID];
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		DWORD dwTbl = pParty->adwTblID[i];
		if ((dwTbl == 0) || (dwTbl >= (DWORD)g_aCharInfo.GetSize ())) {
			break;
		}
		if (g_aCharInfo[dwTbl] != pExtra) {
			SendStateTo (g_aCharInfo[dwTbl]);
		}
	}
}

/* ========================================================================= */
/* 見えるNPC の作成・消去（同期）											 */
/* ========================================================================= */

/* 見える人の組み合わせ（持ち主とそのパーティのメンバー） */
static DWORD MakeKey(PCHARINFO pOwner)
{
	std::vector<PCHARINFO> aMember;
	size_t i;
	DWORD dwKey = pOwner->dwPartyID * 7919 + 1;

	/* 持ち主がいる空間が変わったら作り直す（見えるNPC は持ち主の空間に出る） */
	dwKey = dwKey * 31 + pOwner->dwSpaceID;
	GetPartyMembers (pOwner, aMember);
	for (i = 0; i < aMember.size (); i ++) {
		dwKey = dwKey * 31 + aMember[i]->dwTblID + 1;
		dwKey = dwKey * 31 + aMember[i]->wQuestID;
	}
	return dwKey;
}

/* この見えるNPC が今の進み具合で出るか */
static BOOL IsNpcWanted(PCHARINFO pOwner, PQUESTINFO pQuest, int nIdx)
{
	PQUESTNPCINFO pNpc = &pQuest->aNpc[nIdx];

	if ((pNpc->byKind == QUESTNPCKIND_NONE) || (pNpc->byKind >= QUESTNPCKIND_MAX) || (pNpc->byKind == QUESTNPCKIND_ENEMY)) {
		return FALSE;
	}
	if (GetMapNo (pNpc->dwMapID) < 0) {
		return FALSE;
	}
	if (pOwner->byQuestStep < pNpc->byStepMin) {
		return FALSE;
	}
	if ((pNpc->byStepMax != QUESTNPC_NOSTEP) && (pOwner->byQuestStep > pNpc->byStepMax)) {
		return FALSE;
	}
	if (pNpc->byFlgNeed && !QuestFlagCheck (pOwner, pNpc->byFlgNeed)) {
		return FALSE;
	}
	if (pNpc->byFlgHide && QuestFlagCheck (pOwner, pNpc->byFlgHide)) {
		return FALSE;
	}
	return TRUE;
}

/* 持ち主かその仲間が、この見えるNPC のマップにいるか */
static BOOL IsAnyoneOnMap(PCHARINFO pOwner, int nMapNo)
{
	std::vector<PCHARINFO> aMember;
	size_t i;

	GetPartyMembers (pOwner, aMember);
	for (i = 0; i < aMember.size (); i ++) {
		if ((int)aMember[i]->dwMapID == nMapNo) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 見えるNPC を消す */
static void RemoveNpc(DWORD dwTblID)
{
	PCHARINFO pNpc;

	s_mapNpc.erase (dwTblID);
	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pNpc = g_aCharInfo[dwTblID];
	if (!IsQuestNpc (pNpc)) {
		return;
	}
	/* 見えていた人みんなに消えたことが届くよう、消す間は絞り込みを止める */
	QuestRemovingBegin ();
	SAFE_DELETE_ARRAY (pNpc->pszNpcSpeech);
	DeleteChar (dwTblID);
	QuestRemovingEnd ();
}

/* 見えるNPC を作る */
static void CreateNpc(PCHARINFO pOwner, PQUESTINFO pQuest, int nIdx, DWORD dwKey)
{
	PQUESTNPCINFO pDef = &pQuest->aNpc[nIdx];
	PCHARINFO pNew;
	int nMapNo = GetMapNo (pDef->dwMapID);
	CCharProcBase CharProcBase;
	QNPCENTRY Entry;

	if (nMapNo < 0) {
		return;
	}
	pNew = new CHARINFO;
	ZeroMemory ((PBYTE)pNew, sizeof (CHARINFO));
	pNew->byNpcType = NPCTYPE_QUESTNPC;
	SetInitCharData (pNew);
	pNew->dwSpaceID		= pOwner->dwSpaceID;				/* 持ち主と同じ空間に出す */
	pNew->byGameState	= GAMESTATE_NPC;
	pNew->byCharState	= CHARSTATE_MOVE;
	pNew->dwMapID		= (DWORD)nMapNo;
	pNew->dwStartMapID	= (DWORD)nMapNo;
	pNew->wGrpID		= pDef->wGrpID;
	pNew->byDirection	= (pDef->byDir < 4) ? pDef->byDir : 1;
	pNew->wHP = pNew->wMaxHP = 1;
	CopyStr (pNew->szName, sizeof (pNew->szName), pDef->szName);
	pNew->dwXBack = pNew->dwX = pNew->dwStartX = pDef->byX;
	pNew->dwYBack = pNew->dwY = pNew->dwStartY = pDef->byY;
	/* 話しかけた時にクライアントがサーバーへ知らせる目印（台詞の中身は話した時にサーバーが送る） */
	pNew->pszNpcSpeech = new char[3];
	strcpy (pNew->pszNpcSpeech, "@q");

	pNew = AddChar (pNew);
	pNew->pID = pNew->dwTblID + 1;
	if (pNew->pProc == NULL) {
		CharProcBase.SetProc (pNew);
	}

	Entry.dwOwner = pOwner->dwTblID;
	Entry.wQuestID = pQuest->wID;
	Entry.nIdx = nIdx;
	Entry.dwKey = dwKey;
	s_mapNpc[pNew->dwTblID] = Entry;

	/* 見える人（持ち主・仲間・管理者）にだけ届く */
	SendCmdCHARINFO3 (pNew->dwTblID);
}

/* 今すぐ見えるNPC を作る・消す */
void QuestSyncAll(void)
{
	std::map<DWORD, QNPCENTRY>::iterator it;
	std::map<std::pair<DWORD, int>, DWORD> mapWanted;			/* (持ち主, 何番目) → 組み合わせ */
	std::vector<DWORD> aRemove;
	std::vector<std::pair<std::pair<DWORD, int>, DWORD> > aCreate;
	int i, j, nCount = g_aCharInfo.GetSize ();

	/* いま出すべきもの */
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pOwner = g_aCharInfo[i];
		PQUESTINFO pQuest;

		if ((pOwner->wQuestID == 0) || !IsPlayerOnline (pOwner)) {
			continue;
		}
		pQuest = FindQuest (pOwner->wQuestID);
		if (pQuest == NULL) {
			continue;
		}
		DWORD dwKey = MakeKey (pOwner);
		for (j = 0; j < MAX_QUESTNPC; j ++) {
			if (IsNpcWanted (pOwner, pQuest, j) && IsAnyoneOnMap (pOwner, GetMapNo (pQuest->aNpc[j].dwMapID))) {
				mapWanted[std::make_pair ((DWORD)i, j)] = dwKey;
			}
		}
	}

	/* 要らなくなったもの・組み合わせが変わったものを消す */
	for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
		std::map<std::pair<DWORD, int>, DWORD>::iterator itW = mapWanted.find (std::make_pair (it->second.dwOwner, it->second.nIdx));
		if ((itW == mapWanted.end ()) || (itW->second != it->second.dwKey) ||
				(it->second.dwOwner >= (DWORD)nCount) || (g_aCharInfo[it->second.dwOwner]->wQuestID != it->second.wQuestID)) {
			aRemove.push_back (it->first);
		}
	}
	for (i = 0; i < (int)aRemove.size (); i ++) {
		RemoveNpc (aRemove[i]);
	}

	/* まだ無いものを作る */
	{
		std::map<std::pair<DWORD, int>, DWORD>::iterator itW;
		std::map<std::pair<DWORD, int>, BOOL> mapHave;
		for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
			mapHave[std::make_pair (it->second.dwOwner, it->second.nIdx)] = TRUE;
		}
		for (itW = mapWanted.begin (); itW != mapWanted.end (); itW ++) {
			if (mapHave.find (itW->first) != mapHave.end ()) {
				continue;
			}
			PCHARINFO pOwner = g_aCharInfo[itW->first.first];
			CreateNpc (pOwner, FindQuest (pOwner->wQuestID), itW->first.second, itW->second);
		}
	}
}

/* 周期処理 */
void QuestSyncProc(void)
{
	DWORD dwNow = timeGetTime ();

	/* 光に乗って少し待った人を移す（毎回見る） */
	StepWaitProc (dwNow);
	if ((s_dwLastSync != 0) && (dwNow - s_dwLastSync < QUEST_SYNC_INTERVAL)) {
		return;
	}
	s_dwLastSync = dwNow;
	/* 討伐チャレンジ（倒れた・抜けた人を戻す、全部倒したら成功） */
	ChallengeProc ();
	/* 依頼を受けている人も、出ている見えるNPC も無ければ何もしない */
	if (s_mapNpc.empty ()) {
		int i, nCount = g_aCharInfo.GetSize ();
		BOOL bAny = FALSE;
		for (i = 0; i < nCount; i ++) {
			if (g_aCharInfo[i]->wQuestID != 0) {
				bAny = TRUE;
				break;
			}
		}
		if (!bAny) {
			return;
		}
	}
	QuestSyncAll ();
}

/* キャラが消える・ログオフする時 */
void QuestOnDecPlayer(PCHARINFO pChar)
{
	std::vector<DWORD> aRemove;
	std::map<DWORD, QNPCENTRY>::iterator it;

	if (IsQuestNpc (pChar)) {
		s_mapNpc.erase (pChar->dwTblID);
		return;
	}
	s_mapStepWait.erase (pChar->dwTblID);
	/* 討伐チャレンジの敵が倒された・挑戦中の人が切断した */
	ChallengeOnDecPlayer (pChar);
	for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
		if (it->second.dwOwner == pChar->dwTblID) {
			aRemove.push_back (it->first);
		}
	}
	for (size_t i = 0; i < aRemove.size (); i ++) {
		RemoveNpc (aRemove[i]);
	}
}

/* ログインした時 */
void QuestOnLogin(PCHARINFO pChar)
{
	if (pChar->byNpcType != NPCTYPE_INIT) {
		return;
	}
	/* 空間はログインのたびにふつうの世界から（切断した時に残っていても戻す） */
	pChar->dwSpaceID = SPACE_NORMAL;
	/* 討伐チャレンジ中に切断した人は、入る前の場所へ（まだマップの表に入る前なので直接書き換える） */
	{
		std::map<DWORD, LOANPOS>::iterator itPos = s_mapPendingPos.find (pChar->dwTblID);
		if (itPos != s_mapPendingPos.end ()) {
			pChar->dwMapID = itPos->second.dwMapID;
			pChar->dwX = pChar->dwXBack = itPos->second.dwX;
			pChar->dwY = pChar->dwYBack = itPos->second.dwY;
			pChar->byDirection = itPos->second.byDirection;
			s_mapPendingPos.erase (itPos);
		}
	}
	/* なくなった依頼や、段が範囲外の保存データは受けていないことにする */
	if (pChar->wQuestID != 0) {
		PQUESTINFO pQuest = FindQuest (pChar->wQuestID);
		if (pQuest == NULL) {
			ClearQuest (pChar);
		} else if (pChar->byQuestStep >= max ((BYTE)1, min (pQuest->byStepCount, (BYTE)MAX_QUESTSTEP))) {
			pChar->byQuestStep = (BYTE)(max ((BYTE)1, min (pQuest->byStepCount, (BYTE)MAX_QUESTSTEP)) - 1);
		}
	}
	SendStateTo (pChar);
}

/* ========================================================================= */
/* 依頼の品・報酬の持ち物の扱い												 */
/* ========================================================================= */

/* 持ち物と預かり所にある数 */
static int CountItem(PCHARINFO pChar, WORD wItem, int *pnInv, int *pnLeave)
{
	int i, nInv = 0, nLeave = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == wItem) {
			nInv ++;
		}
	}
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		PLEAVEINFO pLeave = g_aLeaveInfo[pChar->dwLeaveID];
		for (i = 0; (i < pLeave->byCount) && (i < MAX_LEAVEITEM); i ++) {
			if (pLeave->waItem[i] == wItem) {
				nLeave += pLeave->byItemCount[i];
			}
		}
	}
	if (pnInv) {
		*pnInv = nInv;
	}
	if (pnLeave) {
		*pnLeave = nLeave;
	}
	return nInv + nLeave;
}

/* 持ち物か預かり所から 1 個取り除く（持ち物を先に）。取ったら TRUE */
static BOOL TakeItem(PCHARINFO pChar, WORD wItem)
{
	int i;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == wItem) {
			DelCharItem (pChar->dwTblID, (BYTE)i);
			return TRUE;
		}
	}
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		PLEAVEINFO pLeave = g_aLeaveInfo[pChar->dwLeaveID];
		for (i = 0; (i < pLeave->byCount) && (i < MAX_LEAVEITEM); i ++) {
			if ((pLeave->waItem[i] == wItem) && (pLeave->byItemCount[i] > 0)) {
				pLeave->byItemCount[i] --;
				if (pLeave->byItemCount[i] == 0) {
					for (int k = i; k < pLeave->byCount - 1; k ++) {
						pLeave->waItem[k] = pLeave->waItem[k + 1];
						pLeave->byItemCount[k] = pLeave->byItemCount[k + 1];
					}
					pLeave->byCount --;
					pLeave->waItem[pLeave->byCount] = 0;
					pLeave->byItemCount[pLeave->byCount] = 0;
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* アイテムを 1 個渡す（持ち物へ、いっぱいなら預かり所へ）。渡せたら TRUE
   pbToLeave が NULL の時は持ち物にだけ入れる（依頼の品は預かり所に入れない） */
static BOOL GiveItem(PCHARINFO pChar, WORD wItem, BOOL *pbToLeave)
{
	if (pbToLeave) {
		*pbToLeave = FALSE;
	}
	if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
		return FALSE;
	}
	if (GetItemCount (pChar->dwTblID) < MAX_MYITEM) {
		AddCharItem (pChar->dwTblID, wItem);
		return TRUE;
	}
	if (pbToLeave == NULL) {
		return FALSE;
	}
	if (AddLeaveItem (pChar, wItem)) {
		*pbToLeave = TRUE;
		return TRUE;
	}
	return FALSE;
}

/* 持ち物・お金・預かり所の変化を本人に送る */
static void SendBagState(PCHARINFO pChar)
{
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
	SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		SendCmdLEAVEITEMGET (pChar->dwTblID);
	}
}

/* アイテムの名前 */
static const char *ItemName(WORD wItem)
{
	if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
		return "";
	}
	return g_aItemInfo[wItem]->szName;
}

/* ========================================================================= */
/* 見えるNPC に話す															 */
/* ========================================================================= */

/* 段と旗を進める（変わったら TRUE） */
static BOOL AdvanceQuest(PCHARINFO pChar, PQUESTNPCINFO pDef)
{
	BOOL bChg = FALSE;

	if (pDef->bySetFlg >= 1 && pDef->bySetFlg <= 16 && !QuestFlagCheck (pChar, pDef->bySetFlg)) {
		pChar->wQuestFlg |= (WORD)(1 << (pDef->bySetFlg - 1));
		bChg = TRUE;
	}
	if ((pDef->byNextStep != QUESTNPC_NOSTEP) && (pDef->byNextStep < MAX_QUESTSTEP) && (pChar->byQuestStep != pDef->byNextStep)) {
		pChar->byQuestStep = pDef->byNextStep;
		bChg = TRUE;
	}
	return bChg;
}

/* ポータルで移る */
static void DoPortal(PCHARINFO pChar, PQUESTNPCINFO pDef)
{
	EVENTINFO Event;
	EVENTINFO_MOVE Move;
	EVENTINFO_MAPCHG MapChg;
	int nDestMapNo = GetMapNo (pDef->dwDestMapID);

	if (nDestMapNo < 0) {
		return;
	}
	ZeroMemory (&Event, sizeof (Event));
	if ((DWORD)nDestMapNo == pChar->dwMapID) {
		ZeroMemory (&Move, sizeof (Move));
		Move.dwX = pDef->byDestX;
		Move.dwY = pDef->byDestY;
		Move.byEfect = EVENTMOVEEFECT_MOVE;
		Event.byType = EVENTTYPE_MOVE;
		Event.pData = &Move;
		EventProc (pChar, &Event, 0);
	} else {
		ZeroMemory (&MapChg, sizeof (MapChg));
		MapChg.dwX = pDef->byDestX;
		MapChg.dwY = pDef->byDestY;
		MapChg.dwMapID = pDef->dwDestMapID;
		Event.byType = EVENTTYPE_MAPCHG;
		Event.pData = &MapChg;
		EventProc (pChar, &Event, 0);
	}
}

/* 見えるNPC に話しかけた */
void QuestNpcTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	std::map<DWORD, QNPCENTRY>::iterator it = s_mapNpc.find (pNpc->dwTblID);
	PQUESTINFO pQuest;
	PQUESTNPCINFO pDef;
	char szTmp[512];
	BOOL bChg = FALSE;

	if ((it == s_mapNpc.end ()) || !IsCharVisibleTo (pNpc, pChar)) {
		return;
	}
	pQuest = FindQuest (it->second.wQuestID);
	if ((pQuest == NULL) || (it->second.nIdx >= MAX_QUESTNPC)) {
		return;
	}
	pDef = &pQuest->aNpc[it->second.nIdx];
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}

	/* 管理者は見えるだけ（依頼を受けた人・仲間でなければ、光で移ったり宝箱を開けたりはできない） */
	if (!IsQuestNpcFor (pNpc, pChar)) {
		SendCmdSYSMSG (pChar->pID, "これは、依頼を受けた人のためのものです。（管理者なので見えています）", SPEECHCMD_BATTLE1);
		return;
	}

	/* 宝箱は、受けた人が中身を受け取れた時だけ台詞を出す（受け取れないのに「見つけた」と出ないように） */
	if (pDef->byKind == QUESTNPCKIND_CHEST) {
		if (pChar->wQuestID != it->second.wQuestID) {
			SendCmdSYSMSG (pChar->pID, "これは、依頼を受けた人のためのものです。", SPEECHCMD_BATTLE1);
			return;
		}
	} else if (pDef->szSpeech[0]) {
		/* 台詞は話した人にだけ出す */
		snprintf (szTmp, sizeof (szTmp), "%s「%s」", pNpc->szName, pDef->szSpeech);
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	}

	/* ポータルは、見えている人（受けた人・仲間）なら誰でも通れる。段を進めるのは受けた人だけ */
	if ((pDef->byKind == QUESTNPCKIND_PORTAL) && (pChar->wQuestID != it->second.wQuestID)) {
		DoPortal (pChar, pDef);
		QuestSyncAll ();
		return;
	}

	/* その依頼を受けている人だけが、進める・受け取る（見ているだけの仲間は台詞だけ） */
	if (pChar->wQuestID != it->second.wQuestID) {
		if (pDef->szSpeech[0] == 0) {
			SendCmdSYSMSG (pChar->pID, "これは、依頼を受けた人のためのものです。", SPEECHCMD_BATTLE1);
		}
		return;
	}

	switch (pDef->byKind) {
	case QUESTNPCKIND_GATE:
		/* 討伐チャレンジの入口：近くの仲間と一緒に入る（段・旗は入った全員が進む） */
		ChallengeStart (pChar, pQuest, pDef);
		QuestSyncAll ();
		return;

	case QUESTNPCKIND_PORTAL:
		bChg = AdvanceQuest (pChar, pDef);
		if (bChg) {
			QuestSendState (pChar);
		}
		DoPortal (pChar, pDef);
		QuestSyncAll ();
		return;

	case QUESTNPCKIND_CHEST:
		/* 旗で「もう開けた」を表す（旗が立っていたら 2 度目はもらえない） */
		if ((pDef->bySetFlg != 0) && QuestFlagCheck (pChar, pDef->bySetFlg)) {
			SendCmdSYSMSG (pChar->pID, "もう中身は空っぽのようだ。", SPEECHCMD_BATTLE1);
			return;
		}
		/* 依頼の品は持ち物にだけ入れる（預かり所には入れない）。持てない時は受け取れない */
		if ((pDef->wItem != 0) && !GiveItem (pChar, pDef->wItem, NULL)) {
			SendCmdSYSMSG (pChar->pID, "持ち物がいっぱいで受け取れません。持ち物をあけてからもう一度どうぞ。", SPEECHCMD_BATTLE1);
			return;
		}
		if (pDef->szSpeech[0]) {
			snprintf (szTmp, sizeof (szTmp), "%s「%s」", pNpc->szName, pDef->szSpeech);
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		}
		if (pDef->wItem != 0) {
			SendBagState (pChar);
			SendCmdPLAYSOUND (pChar->pID, SOUND_PI52);
			snprintf (szTmp, sizeof (szTmp), "%sを手に入れました。", ItemName (pDef->wItem));
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		}
		break;

	default:
		break;
	}
	bChg = AdvanceQuest (pChar, pDef);
	if (bChg) {
		QuestSendState (pChar);
		QuestSyncAll ();
	}
}

/* 歩いて見えるポータルに重なったら、話しかけた時と同じく移す
   （同じマスに乗った時だけ。となりを通っただけでは移らない） */
void QuestOnStep(PCHARINFO pChar)
{
	std::map<DWORD, QNPCENTRY>::iterator it;
	PCHARINFO pNpc = NULL;

	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}
	for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
		if (it->first >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		PCHARINFO pTmp = g_aCharInfo[it->first];
		if ((pTmp == NULL) || (pTmp->dwMapID != pChar->dwMapID)) {
			continue;
		}
		if ((pTmp->dwX != pChar->dwX) || (pTmp->dwY != pChar->dwY)) {
			continue;
		}
		PQUESTINFO pQuest = FindQuest (it->second.wQuestID);
		if ((pQuest == NULL) || (it->second.nIdx >= MAX_QUESTNPC) ||
			((pQuest->aNpc[it->second.nIdx].byKind != QUESTNPCKIND_PORTAL) &&
			 (pQuest->aNpc[it->second.nIdx].byKind != QUESTNPCKIND_GATE))) {
			continue;
		}
		if (!IsCharVisibleTo (pTmp, pChar)) {
			continue;
		}
		pNpc = pTmp;
		break;
	}
	/* 乗った瞬間に移すと、歩き（スクロール）が終わる前に画面が変わって何が起きたか分からない。
	 * 少しその場で待ってから移す（待つ間に降りたらやめる）。話しかけた時はすぐ移る */
	if (pNpc == NULL) {
		s_mapStepWait.erase (pChar->dwTblID);
		return;
	}
	{
		std::map<DWORD, STEPWAIT>::iterator itW = s_mapStepWait.find (pChar->dwTblID);
		STEPWAIT Wait;
		if ((itW != s_mapStepWait.end ()) && (itW->second.dwNpcTblID == pNpc->dwTblID) &&
			(itW->second.dwX == pChar->dwX) && (itW->second.dwY == pChar->dwY)) {
			return;
		}
		Wait.dwNpcTblID = pNpc->dwTblID;
		Wait.dwTime = timeGetTime ();
		Wait.dwMapID = pChar->dwMapID;
		Wait.dwX = pChar->dwX;
		Wait.dwY = pChar->dwY;
		s_mapStepWait[pChar->dwTblID] = Wait;
	}
}

/* 光に乗って待っている人を、時間が来たら移す */
static void StepWaitProc(DWORD dwNow)
{
	std::map<DWORD, STEPWAIT>::iterator it;
	std::vector<std::pair<DWORD, DWORD> > aGo;
	size_t i;

	if (s_mapStepWait.empty ()) {
		return;
	}
	for (it = s_mapStepWait.begin (); it != s_mapStepWait.end (); ) {
		PCHARINFO pChar = (it->first < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[it->first] : NULL;
		/* 降りた・マップが変わった・いなくなった */
		if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->dwMapID != it->second.dwMapID) ||
			(pChar->dwX != it->second.dwX) || (pChar->dwY != it->second.dwY)) {
			s_mapStepWait.erase (it ++);
			continue;
		}
		if (dwNow - it->second.dwTime < QUEST_STEP_WAIT) {
			it ++;
			continue;
		}
		aGo.push_back (std::make_pair (it->first, it->second.dwNpcTblID));
		s_mapStepWait.erase (it ++);
	}
	/* 移すと見えるNPC の一覧が変わるので、待ちの一覧を見終えてから */
	for (i = 0; i < aGo.size (); i ++) {
		PCHARINFO pChar = g_aCharInfo[aGo[i].first];
		if ((aGo[i].second >= (DWORD)g_aCharInfo.GetSize ()) || (s_mapNpc.find (aGo[i].second) == s_mapNpc.end ())) {
			continue;
		}
		PCHARINFO pNpc = g_aCharInfo[aGo[i].second];
		if ((pNpc->dwMapID != pChar->dwMapID) || (pNpc->dwX != pChar->dwX) || (pNpc->dwY != pChar->dwY)) {
			continue;
		}
		QuestNpcTalk (pChar, pNpc);
	}
}

/* 足元を調べた時、同じマスに見える宝箱などがあれば話しかけたことにする
   （マップの調べるイベントと重なっていると、そちらが先になって開けられなかった）
   戻り値：見つけて処理した時 TRUE */
BOOL QuestExamineHere(PCHARINFO pChar)
{
	std::map<DWORD, QNPCENTRY>::iterator it;
	PCHARINFO pNpc = NULL;

	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return FALSE;
	}
	for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
		if (it->first >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		PCHARINFO pTmp = g_aCharInfo[it->first];
		if ((pTmp == NULL) || (pTmp->dwMapID != pChar->dwMapID)) {
			continue;
		}
		if ((pTmp->dwX != pChar->dwX) || (pTmp->dwY != pChar->dwY)) {
			continue;
		}
		if (!IsCharVisibleTo (pTmp, pChar)) {
			continue;
		}
		pNpc = pTmp;
		break;
	}
	if (pNpc == NULL) {
		return FALSE;
	}
	QuestNpcTalk (pChar, pNpc);
	return TRUE;
}

/* ========================================================================= */
/* ギルド																	 */
/* ========================================================================= */

/* 台詞が "@g" で始まるか */
BOOL QuestIsGuildNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'g');
}

/* ギルド番号 */
static int GetGuildNo(PCHARINFO pNpc)
{
	const char *p = pNpc->pszNpcSpeech + 2;
	int n = 0;
	while ((*p >= '0') && (*p <= '9') && (n < 1000)) {
		n = n * 10 + (*p - '0');
		p ++;
	}
	return n;
}

/* 仲間が受けている依頼の番号（誰も受けていなければ 0。食い違っていれば 0xFFFF） */
static WORD GetPartyQuest(PCHARINFO pChar, PCHARINFO pExclude)
{
	std::vector<PCHARINFO> aMember;
	size_t i;
	WORD wRet = 0;

	GetPartyMembers (pChar, aMember);
	for (i = 0; i < aMember.size (); i ++) {
		if (aMember[i] == pExclude || aMember[i]->wQuestID == 0) {
			continue;
		}
		if ((wRet != 0) && (wRet != aMember[i]->wQuestID)) {
			return 0xFFFF;
		}
		wRet = aMember[i]->wQuestID;
	}
	return wRet;
}

/* 別の依頼を受けていて、同じパーティになれないか（A 側・B 側それぞれのパーティ全員を見る） */
BOOL QuestPartyConflict(PCHARINFO pA, PCHARINFO pB)
{
	WORD wA = GetPartyQuest (pA, NULL);
	WORD wB = GetPartyQuest (pB, NULL);

	if ((wA == 0) || (wB == 0)) {
		return FALSE;
	}
	return (wA != wB) ? TRUE : FALSE;
}

/* 一覧の並べ替え */
static bool QuestLess(PQUESTINFO pA, PQUESTINFO pB)
{
	if (pA->wSort != pB->wSort) {
		return pA->wSort < pB->wSort;
	}
	return pA->wID < pB->wID;
}

/* 最後の段にいるか */
static BOOL IsLastStep(PCHARINFO pChar, PQUESTINFO pQuest)
{
	int nCount = max ((int)pQuest->byStepCount, 1);
	return ((int)pChar->byQuestStep + 1 >= nCount) ? TRUE : FALSE;
}

/* 報酬の文 */
static void MakeRewardText(PQUESTINFO pQuest, char *pszOut, size_t nSize)
{
	char szTmp[128] = "";

	if (pQuest->dwGold) {
		snprintf (szTmp + strlen (szTmp), sizeof (szTmp) - strlen (szTmp), "お金 %uG ", (unsigned)pQuest->dwGold);
	}
	if (pQuest->dwExp) {
		snprintf (szTmp + strlen (szTmp), sizeof (szTmp) - strlen (szTmp), "経験値 %u ", (unsigned)pQuest->dwExp);
	}
	if (pQuest->wRewardItem) {
		snprintf (szTmp + strlen (szTmp), sizeof (szTmp) - strlen (szTmp), "%s", ItemName (pQuest->wRewardItem));
	}
	if (szTmp[0] == 0) {
		strcpy (szTmp, "なし");
	} else if (pQuest->byChallenge != QUESTCHALLENGE_NONE) {
		snprintf (szTmp + strlen (szTmp), sizeof (szTmp) - strlen (szTmp), "（初回のみ）");
	}
	CopyStr (pszOut, nSize, szTmp);
}

/* 一覧を送る（ギルドの人に話した時） */
void QuestGuildTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	std::vector<PQUESTINFO> aList;
	CMDPARA_QUEST Para;
	int i, nGuild = GetGuildNo (pNpc);
	WORD wParty = GetPartyQuest (pChar, pChar);

	for (i = 0; i < g_aQuestInfo.GetSize (); i ++) {
		PQUESTINFO pQuest = g_aQuestInfo[i];
		if (pQuest->byGuild != nGuild) {
			continue;
		}
		/* 止めている依頼は、受けている人にだけ出る */
		if ((pQuest->byState != QUESTSTATE_ON) && (pChar->wQuestID != pQuest->wID)) {
			continue;
		}
		/* 一度きりでクリア済みのものは出さない */
		if (pQuest->byOnce && QuestIsDone (pChar, pQuest->wID)) {
			continue;
		}
		if ((int)aList.size () >= MAX_QUESTLIST) {
			break;
		}
		aList.push_back (pQuest);
	}
	std::sort (aList.begin (), aList.end (), QuestLess);

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_QUEST;
	Para.byType = QUESTCMD_LISTITEM;
	Para.dwTblID = pChar->dwTblID;
	Para.dwNpcTblID = pNpc->dwTblID;
	Para.byCount = (BYTE)aList.size ();
	if (aList.empty ()) {
		svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
		return;
	}
	for (i = 0; i < (int)aList.size (); i ++) {
		PQUESTINFO pQuest = aList[i];
		Para.byIndex = (BYTE)i;
		Para.wQuestID = pQuest->wID;
		if (pChar->wQuestID == pQuest->wID) {
			Para.byMark = (IsLastStep (pChar, pQuest) &&
				((pQuest->byChallenge == QUESTCHALLENGE_NONE) || QuestFlagCheck (pChar, QUEST_FLG_CLEARED)) &&
				((pQuest->wQuestItem == 0) || (CountItem (pChar, pQuest->wQuestItem, NULL, NULL) > 0))) ? QUESTMARK_READY : QUESTMARK_MINE;
		} else if ((pChar->wQuestID != 0) || (wParty == 0xFFFF) || ((wParty != 0) && (wParty != pQuest->wID))) {
			Para.byMark = QUESTMARK_LOCKED;
		} else {
			Para.byMark = QUESTMARK_FREE;
		}
		CopyStr (Para.szName, sizeof (Para.szName), pQuest->szName);
		CopyStr (Para.szExplain, sizeof (Para.szExplain), pQuest->szExplain);
		CopyStr (Para.szGoal, sizeof (Para.szGoal), (pChar->wQuestID == pQuest->wID) ? GetGoal (pQuest, pChar->byQuestStep) : "");
		MakeRewardText (pQuest, Para.szReward, sizeof (Para.szReward));
		svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
	}
}

/* 結果を送る */
static void SendResult(PCHARINFO pChar, BYTE byResult, WORD wQuestID, const char *pszMsg)
{
	CMDPARA_QUEST Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_QUEST;
	Para.byType = QUESTCMD_RESULT;
	Para.byResult = byResult;
	Para.wQuestID = wQuestID;
	Para.dwTblID = pChar->dwTblID;
	CopyStr (Para.szMsg, sizeof (Para.szMsg), pszMsg);
	svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* 近くに、ギルドの人（nGuild が負なら誰でも・そうでなければその番号）がいるか */
static PCHARINFO FindNearGuild(PCHARINFO pChar, int nGuild, DWORD dwNpcTblID)
{
	PCHARINFO pNpc;

	if (dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pNpc = g_aCharInfo[dwNpcTblID];
	if (!QuestIsGuildNpc (pNpc) || (pNpc->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pNpc)) {
		return NULL;
	}
	if ((abs ((int)pChar->dwX - (int)pNpc->dwX) > 2) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > 2)) {
		return NULL;
	}
	if ((nGuild >= 0) && (GetGuildNo (pNpc) != nGuild)) {
		return NULL;
	}
	return pNpc;
}

/* 受ける */
static void DoAccept(PCHARINFO pChar, PCHARINFO pNpc, PQUESTINFO pQuest)
{
	WORD wParty;

	if (pChar->wQuestID != 0) {
		SendResult (pChar, QUESTRESULT_NG, 0, "いま受けている依頼があります。先に終わらせるか、取り消してください。");
		return;
	}
	if ((pQuest->byState != QUESTSTATE_ON) || (pQuest->byGuild != GetGuildNo (pNpc))) {
		SendResult (pChar, QUESTRESULT_NG, 0, "この依頼は、いまは受けられません。");
		return;
	}
	if (pQuest->byOnce && QuestIsDone (pChar, pQuest->wID)) {
		SendResult (pChar, QUESTRESULT_NG, 0, "この依頼は、もう終わっています。");
		return;
	}
	wParty = GetPartyQuest (pChar, pChar);
	if ((wParty != 0) && (wParty != pQuest->wID)) {
		SendResult (pChar, QUESTRESULT_NG, 0, "仲間が別の依頼を受けているので、同じ依頼しか受けられません。");
		return;
	}
	/* 討伐チャレンジは、近くの仲間と一緒に受ける（後から入った仲間は参加できない） */
	if (pQuest->byChallenge != QUESTCHALLENGE_NONE) {
		std::vector<PCHARINFO> aMember;
		int nNeed = max ((int)pQuest->byMemberMin, 1);
		char szTmp[256];
		size_t i;

		if (wParty == pQuest->wID) {
			SendResult (pChar, QUESTRESULT_NG, 0, "討伐チャレンジは、仲間と一緒に受けた時だけ参加できます。");
			return;
		}
		if ((nNeed >= 2) && (pChar->dwPartyID == 0)) {
			snprintf (szTmp, sizeof (szTmp), "この依頼は %d 人以上でパーティを組んでから受けてください。", nNeed);
			SendResult (pChar, QUESTRESULT_NG, 0, szTmp);
			return;
		}
		GetChallengeMembers (pChar, 0, aMember);
		if ((int)aMember.size () < nNeed) {
			snprintf (szTmp, sizeof (szTmp), "近くにいる仲間が足りません（%d 人必要です）。", nNeed);
			SendResult (pChar, QUESTRESULT_NG, 0, szTmp);
			return;
		}
		for (i = 0; i < aMember.size (); i ++) {
			PCHARINFO pTmp = aMember[i];
			pTmp->wQuestID = pQuest->wID;
			pTmp->byQuestStep = 0;
			pTmp->wQuestFlg = 0;
			ZeroMemory (pTmp->awQuestCount, sizeof (pTmp->awQuestCount));
			pTmp->dwQuestAcceptTime = (DWORD)time (NULL);
			if (pTmp == pChar) {
				SendResult (pTmp, QUESTRESULT_ACCEPT, pQuest->wID, pQuest->szMsgAccept[0] ? pQuest->szMsgAccept : "依頼を受けました。");
			} else {
				snprintf (szTmp, sizeof (szTmp), "%sさんと一緒に、依頼「%s」を受けました。", pChar->szName, pQuest->szName);
				SendCmdSYSMSG (pTmp->pID, szTmp, SPEECHCMD_BATTLE1);
			}
		}
		QuestSendState (pChar);
		QuestSyncAll ();
		return;
	}
	pChar->wQuestID = pQuest->wID;
	pChar->byQuestStep = 0;
	pChar->wQuestFlg = 0;
	ZeroMemory (pChar->awQuestCount, sizeof (pChar->awQuestCount));
	pChar->dwQuestAcceptTime = (DWORD)time (NULL);

	SendResult (pChar, QUESTRESULT_ACCEPT, pQuest->wID, pQuest->szMsgAccept[0] ? pQuest->szMsgAccept : "依頼を受けました。");
	QuestSendState (pChar);
	QuestSyncAll ();
}

/* 取り消す（依頼の品・旗・数を消す。罰は無い） */
static void DoCancel(PCHARINFO pChar)
{
	PQUESTINFO pQuest = FindQuest (pChar->wQuestID);

	if (pChar->wQuestID == 0) {
		SendResult (pChar, QUESTRESULT_NG, 0, "受けている依頼はありません。");
		return;
	}
	if (pQuest && pQuest->wQuestItem) {
		while (TakeItem (pChar, pQuest->wQuestItem)) {
		}
		SendBagState (pChar);
	}
	ClearQuest (pChar);
	SendResult (pChar, QUESTRESULT_CANCEL, 0, "依頼を取り消しました。");
	QuestSendState (pChar);
	QuestSyncAll ();
}

/* 渡す（依頼の品を回収して、報酬を渡す） */
static void DoHandIn(PCHARINFO pChar, PCHARINFO pNpc)
{
	PQUESTINFO pQuest = FindQuest (pChar->wQuestID);
	WORD awItemBak[MAX_MYITEM], awEquBak[10];
	DWORD dwLeaveIDBak;
	LEAVEINFO LeaveBak;
	PLEAVEINFO pLeave = NULL;
	BOOL bToLeave = FALSE;
	char szTmp[256];

	if ((pQuest == NULL) || (pQuest->byGuild != GetGuildNo (pNpc))) {
		SendResult (pChar, QUESTRESULT_NG, 0, "ここでは渡せません。依頼を受けたギルドへ行ってください。");
		return;
	}
	if (!IsLastStep (pChar, pQuest) ||
			((pQuest->byChallenge != QUESTCHALLENGE_NONE) && !QuestFlagCheck (pChar, QUEST_FLG_CLEARED))) {
		SendResult (pChar, QUESTRESULT_NG, pQuest->wID, "まだ終わっていないようです。");
		return;
	}
	if (pQuest->wQuestItem && (CountItem (pChar, pQuest->wQuestItem, NULL, NULL) == 0)) {
		SendResult (pChar, QUESTRESULT_NG, pQuest->wID, "依頼の品が見つかりません。持ち物か預かり所を確かめてください。");
		return;
	}

	/* 途中で失敗したら元に戻せるよう、今の状態を控える */
	memcpy (awItemBak, pChar->waItem, sizeof (awItemBak));
	memcpy (awEquBak, pChar->waEqu, sizeof (awEquBak));
	dwLeaveIDBak = pChar->dwLeaveID;
	ZeroMemory (&LeaveBak, sizeof (LeaveBak));
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		pLeave = g_aLeaveInfo[pChar->dwLeaveID];
		LeaveBak = *pLeave;
	}

	/* 討伐チャレンジの報酬は初めてクリアした時だけ（2 回目からは記録だけ） */
	BOOL bReward = !((pQuest->byChallenge != QUESTCHALLENGE_NONE) && QuestIsDone (pChar, pQuest->wID));

	if (pQuest->wQuestItem) {
		TakeItem (pChar, pQuest->wQuestItem);
	}
	if (bReward && pQuest->wRewardItem && !GiveItem (pChar, pQuest->wRewardItem, &bToLeave)) {
		memcpy (pChar->waItem, awItemBak, sizeof (awItemBak));
		memcpy (pChar->waEqu, awEquBak, sizeof (awEquBak));
		if ((dwLeaveIDBak == 0) && (pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
			ZeroMemory (g_aLeaveInfo[pChar->dwLeaveID], sizeof (LEAVEINFO));
			pChar->dwLeaveID = 0;
		} else if (pLeave) {
			*pLeave = LeaveBak;
		}
		SendResult (pChar, QUESTRESULT_NG, pQuest->wID, "持ち物がいっぱいで報酬を受け取れません。あけてからもう一度どうぞ。");
		return;
	}

	if (bReward) {
		pChar->dwGold = min (pChar->dwGold + pQuest->dwGold, (DWORD)QUEST_GOLD_MAX);
	}
	if ((pQuest->byOnce || (pQuest->byChallenge != QUESTCHALLENGE_NONE)) && (pQuest->wID < MAX_QUEST)) {
		pChar->abyQuestDone[pQuest->wID / 8] |= (BYTE)(1 << (pQuest->wID % 8));
	}
	WORD wID = pQuest->wID;
	char szMsgDone[MAX_QUESTMSG];
	CopyStr (szMsgDone, sizeof (szMsgDone), pQuest->szMsgDone[0] ? pQuest->szMsgDone : "ありがとう！ 依頼は完了です。");
	DWORD dwExp = bReward ? pQuest->dwExp : 0;
	if (!bReward) {
		snprintf (szMsgDone + strlen (szMsgDone), sizeof (szMsgDone) - strlen (szMsgDone), "（2 回目からは報酬はありません）");
	}
	ClearQuest (pChar);

	SendBagState (pChar);
	if (bToLeave) {
		snprintf (szTmp, sizeof (szTmp), "持ちきれないので、預かり所の使い魔が%sの預かり所へ運んでいきました", pChar->szName);
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	}
	if (dwExp) {
		AddExp (pChar, dwExp, TRUE);
	}
	SendResult (pChar, QUESTRESULT_DONE, wID, szMsgDone);
	QuestSendState (pChar);
	QuestSyncAll ();
}

/* CMD_QUEST の受信 */
void QuestRecv(PCHARINFO pChar, PCMDPARA_QUEST pPara)
{
	PCHARINFO pNpc;
	PQUESTINFO pQuest;

	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}
	if (QuestInChallenge (pChar) && (pPara->byType != QUESTCMD_LISTREQ)) {
		SendResult (pChar, QUESTRESULT_NG, 0, "討伐チャレンジ中はできません。");
		return;
	}
	switch (pPara->byType) {
	case QUESTCMD_LISTREQ:
		pNpc = FindNearGuild (pChar, -1, pPara->dwNpcTblID);
		if (pNpc) {
			QuestGuildTalk (pChar, pNpc);
		}
		break;

	case QUESTCMD_ACCEPT:
		pNpc = FindNearGuild (pChar, -1, pPara->dwNpcTblID);
		pQuest = FindQuest (pPara->wQuestID);
		if ((pNpc == NULL) || (pQuest == NULL)) {
			SendResult (pChar, QUESTRESULT_NG, 0, "ギルドの人の近くで頼んでください。");
			break;
		}
		DoAccept (pChar, pNpc, pQuest);
		break;

	case QUESTCMD_CANCEL:
		if (FindNearGuild (pChar, -1, pPara->dwNpcTblID) == NULL) {
			SendResult (pChar, QUESTRESULT_NG, 0, "依頼の取り消しは、ギルドの人の近くでしかできません。");
			break;
		}
		DoCancel (pChar);
		break;

	case QUESTCMD_HANDIN:
		pNpc = FindNearGuild (pChar, -1, pPara->dwNpcTblID);
		if (pNpc == NULL) {
			SendResult (pChar, QUESTRESULT_NG, 0, "ギルドの人の近くで渡してください。");
			break;
		}
		DoHandIn (pChar, pNpc);
		break;
	}
}

/* ========================================================================= */
/* 討伐チャレンジ															 */
/* ========================================================================= */

/* 挑戦中か（貸し出しの状態になっているか） */
BOOL QuestInChallenge(PCHARINFO pChar)
{
	return (pChar != NULL) && (s_mapLoan.find (pChar->dwTblID) != s_mapLoan.end ());
}

/* 討伐チャレンジの敵か */
BOOL QuestIsChallengeEnemy(PCHARINFO pChar)
{
	return (pChar != NULL) && (s_mapEnemy.find (pChar->dwTblID) != s_mapEnemy.end ());
}

/* 本人とパーティの、近くにいて一緒に受ける・入る人（本人を先頭に）
   wQuestID が 0 なら「依頼を受けていない人」、そうでなければ「その依頼を受けている人」 */
static void GetChallengeMembers(PCHARINFO pChar, WORD wQuestID, std::vector<PCHARINFO> &aOut)
{
	std::vector<PCHARINFO> aParty;
	size_t i;

	aOut.clear ();
	GetPartyMembers (pChar, aParty);
	for (i = 0; i < aParty.size (); i ++) {
		PCHARINFO pTmp = aParty[i];
		if (pTmp != pChar) {
			if ((pTmp->dwMapID != pChar->dwMapID) || !IsSameSpace (pTmp, pChar)) {
				continue;
			}
			if ((abs ((int)pTmp->dwX - (int)pChar->dwX) > CHALLENGE_NEAR) || (abs ((int)pTmp->dwY - (int)pChar->dwY) > CHALLENGE_NEAR)) {
				continue;
			}
		}
		if (pTmp->wQuestID != wQuestID) {
			continue;
		}
		if ((pTmp->wHP == 0) || (pTmp->byCharState == CHARSTATE_DEAD) || QuestInChallenge (pTmp)) {
			continue;
		}
		aOut.push_back (pTmp);
	}
}

/* 雇っている傭兵がいるか */
static BOOL HasMercenary(PCHARINFO pChar)
{
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pTmp = g_aCharInfo[i];
		if (IsMercenary (pTmp) && (GetMercenaryOwner (pTmp) == pChar)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 控えとキャラの間で状態を写す（bToChar=TRUE で控え→キャラ）。位置は写さない */
static void LoanXfer(PCHARINFO p, LOANBAK *b, BOOL bToChar)
{
#define LOANX(f)	if (bToChar) { p->f = b->f; } else { b->f = p->f; }
#define LOANA(f)	if (bToChar) { memcpy (p->f, b->f, sizeof (p->f)); } else { memcpy (b->f, p->f, sizeof (p->f)); }
	LOANX (byLevel);	LOANX (byPW);	LOANX (byDEX);	LOANX (byST);	LOANX (byIT);	LOANX (byLK);
	LOANA (abyMagic);	LOANA (waItem);	LOANA (waEqu);
	LOANX (wMaxHP);	LOANX (wHP);	LOANX (wMaxMana);	LOANX (wMana);
	LOANX (dwGold);	LOANX (dwExp);
	LOANX (chAttrFire);	LOANX (chAttrWater);	LOANX (chAttrWind);
	LOANX (chAttrEarth);	LOANX (chAttrLight);	LOANX (chAttrDark);
#undef LOANX
#undef LOANA
}

/* 控えとキャラの状態・位置を入れ替える（保存の前後で使う） */
static void LoanSwap(PCHARINFO p, LOANBAK *b)
{
	LOANBAK Tmp = *b;
	DWORD dwMapID = p->dwMapID, dwX = p->dwX, dwY = p->dwY;
	BYTE byDirection = p->byDirection;

	LoanXfer (p, b, FALSE);			/* キャラ → 控え */
	LoanXfer (p, &Tmp, TRUE);		/* 元の控え → キャラ */
	p->dwMapID = Tmp.dwMapID;	p->dwX = Tmp.dwX;	p->dwY = Tmp.dwY;	p->byDirection = Tmp.byDirection;
	b->dwMapID = dwMapID;	b->dwX = dwX;	b->dwY = dwY;	b->byDirection = byDirection;
}

/* 貸し出しセット（ステータステンプレート）にする。持ち物の装備品は種類ごとに最初の 1 つを装備 */
static void LoanApply(PCHARINFO p, PSTATUSTEMPLATEINFO t)
{
	static const BYTE abyEquTypeTbl[MAX_EQUTYPE] = {
		ITEMTYPE_WEAPON, ITEMTYPE_ARMOR, ITEMTYPE_SHILD, ITEMTYPE_HELMET, ITEMTYPE_EQU_HAND,
		ITEMTYPE_EQU_LEG, ITEMTYPE_EQU_ACC1, ITEMTYPE_EQU_ACC2, ITEMTYPE_EQU_ACC3, 0
	};
	int i, j;

	p->byLevel	= max (t->byLevel, (BYTE)1);
	p->byPW		= t->byPW;
	p->byDEX	= t->byDEX;
	p->byST		= t->byST;
	p->byIT		= t->byIT;
	p->byLK		= t->byLK;
	p->wMaxHP	= max (t->wMaxHP, (WORD)1);
	p->wHP		= p->wMaxHP;
	p->wMaxMana	= t->wMaxMana;
	p->wMana	= t->wMaxMana;
	p->dwGold	= t->dwGold;
	p->dwExp	= 0;
	p->chAttrFire	= t->chAttrFire;
	p->chAttrWater	= t->chAttrWater;
	p->chAttrWind	= t->chAttrWind;
	p->chAttrEarth	= t->chAttrEarth;
	p->chAttrLight	= t->chAttrLight;
	p->chAttrDark	= t->chAttrDark;
	memcpy (p->abyMagic, t->abyMagic, sizeof (p->abyMagic));
	ZeroMemory (p->waItem, sizeof (p->waItem));
	for (i = 0, j = 0; i < MAX_MYITEM; i ++) {
		/* 無いアイテムの番号は持たせない */
		if ((t->waItem[i] == 0) || (t->waItem[i] >= (WORD)g_aItemInfo.GetSize ())) {
			continue;
		}
		p->waItem[j ++] = t->waItem[i];
	}
	for (i = 0; i < MAX_EQUTYPE; i ++) {
		p->waEqu[i] = 0xFFFF;
		if (abyEquTypeTbl[i] == 0) {
			continue;
		}
		for (j = 0; j < MAX_MYITEM; j ++) {
			if (p->waItem[j] && (g_aItemInfo[p->waItem[j]]->byType == abyEquTypeTbl[i])) {
				p->waEqu[i] = (WORD)j;
				break;
			}
		}
	}
}

/* 状態を変えた後：タイマーの効果（加速・毒など）を消し、攻撃力・守備力を計算し直して知らせる */
static void LoanSendAll(PCHARINFO p)
{
	DeleteAllTimerEvent (p);
	p->chTimerPW = 0;
	p->chTimerST = 0;
	p->chTimerDEX = 0;
	p->dwLastBattle = 0;
	p->dwTargetTblID = -1;
	RenewATDF (p->dwTblID);
	if (p->pID != 0) {
		SendCmdCHARITEM (p->pID, p->dwTblID);
		SendCmdCHARSTATUS (p->pID, p->dwTblID);
		SendCmdCHARSTATUSHP (URARASOCK_SENDALL, p->dwTblID);
	}
}

/* 経過時間の文（m 分 ss 秒） */
static void MakeTimeText(DWORD dwMs, char *pszOut, size_t nSize)
{
	DWORD dwSec = dwMs / 1000;
	snprintf (pszOut, nSize, "%u分%02u秒", (unsigned)(dwSec / 60), (unsigned)(dwSec % 60));
}

/* 中断・成功の理由 */
enum {
	CHALLENGELEAVE_CLEAR = 0,		/* 全部倒した */
	CHALLENGELEAVE_DEAD,			/* 倒れた */
	CHALLENGELEAVE_PARTY,			/* パーティを抜けた */
	CHALLENGELEAVE_OUT,				/* 戦う場所から出た（町転送など） */
	CHALLENGELEAVE_QUEST,			/* 依頼が無くなった */
	CHALLENGELEAVE_LOGOUT,			/* 切断した */
};

/* 挑戦から外して元の状態に戻す */
static void ChallengeLeave(PCHARINFO p, int nReason)
{
	std::map<DWORD, LOANBAK>::iterator it = s_mapLoan.find (p->dwTblID);
	LOANBAK Bak;
	BOOL bMove;

	if (it == s_mapLoan.end ()) {
		return;
	}
	Bak = it->second;
	s_mapLoan.erase (it);
	{
		std::map<DWORD, CHALLENGE>::iterator itC = s_mapChallenge.find (Bak.dwSession);
		if (itC != s_mapChallenge.end ()) {
			std::vector<DWORD> &aM = itC->second.aMember;
			aM.erase (std::remove (aM.begin (), aM.end (), p->dwTblID), aM.end ());
		}
	}

	LoanXfer (p, &Bak, TRUE);
	if (p->byCharState == CHARSTATE_DEAD) {
		p->byCharState = CHARSTATE_MOVE;
	}
	/* 入る前の HP が 0 のことはないはずだが、念のため */
	if (p->wHP == 0) {
		p->wHP = max (p->wMaxHP, (WORD)1);
	}

	if (nReason == CHALLENGELEAVE_LOGOUT) {
		/* 切断：マップの表から外れた後に、入る前の場所へ（ログインか保存の時に動かす） */
		LOANPOS Pos;
		Pos.dwMapID = Bak.dwMapID;
		Pos.dwX = Bak.dwX;
		Pos.dwY = Bak.dwY;
		Pos.byDirection = Bak.byDirection;
		s_mapPendingPos[p->dwTblID] = Pos;
		LoanSendAll (p);
		return;
	}

	LoanSendAll (p);
	switch (nReason) {
	case CHALLENGELEAVE_DEAD:
		SendCmdSYSMSG (p->pID, "力尽きてしまった…。挑戦は中断して、元の姿に戻りました。依頼は受けたままなので、また挑めます。", SPEECHCMD_BATTLE1);
		break;
	case CHALLENGELEAVE_PARTY:
		SendCmdSYSMSG (p->pID, "パーティを抜けたので、挑戦は中断して元の姿に戻りました。", SPEECHCMD_BATTLE1);
		break;
	case CHALLENGELEAVE_OUT:
		SendCmdSYSMSG (p->pID, "戦いの場から離れたので、挑戦は中断して元の姿に戻りました。", SPEECHCMD_BATTLE1);
		break;
	case CHALLENGELEAVE_QUEST:
		SendCmdSYSMSG (p->pID, "依頼が無くなったので、挑戦は中断して元の姿に戻りました。", SPEECHCMD_BATTLE1);
		break;
	}

	/* 戦う場所から出た人はその場で、ほかは入る前の場所へ戻す */
	bMove = (nReason != CHALLENGELEAVE_OUT) && (Bak.dwMapID < (DWORD)g_aMapInfo.GetSize ());
	/* ふつうの世界へ戻る。移る時は、移った先で見え方が作り直されるので、ここでは作り直さない */
	/* （すでにその場所に立っている時は移動で何も届かないので、ここで作り直す） */
	SpaceSet (p, SPACE_NORMAL, !bMove || ((p->dwMapID == Bak.dwMapID) && (p->dwX == Bak.dwX) && (p->dwY == Bak.dwY)));
	if (bMove) {
		SendCmdFADEINOUT (p->pID, 0);
		SendCmdCHGBGM (p->pID, p->dwMapID, 0);
		SubProcMapMove (p, Bak.dwMapID, Bak.dwX, Bak.dwY);
		/* 移動で届くマップ情報は扉が閉じたままなので、ふつうの世界の扉などの状態を送る */
		SpaceSendMapState (p);
	}
}

/* 敵を消す */
static void ChallengeRemoveEnemy(DWORD dwTblID)
{
	s_mapEnemy.erase (dwTblID);
	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	if (g_aCharInfo[dwTblID]->byNpcType == NPCTYPE_INIT) {
		return;
	}
	DeleteChar (dwTblID);
}

/* 挑戦を終える（残っている人は戻し、残っている敵は消す） */
static void ChallengeEnd(DWORD dwSession)
{
	std::map<DWORD, CHALLENGE>::iterator itC = s_mapChallenge.find (dwSession);
	std::vector<DWORD> aMember, aEnemy;
	size_t i;

	if (itC == s_mapChallenge.end ()) {
		return;
	}
	aMember = itC->second.aMember;
	aEnemy = itC->second.aEnemy;
	s_mapChallenge.erase (itC);
	for (i = 0; i < aMember.size (); i ++) {
		if (aMember[i] < (DWORD)g_aCharInfo.GetSize ()) {
			ChallengeLeave (g_aCharInfo[aMember[i]], CHALLENGELEAVE_QUEST);
		}
	}
	for (i = 0; i < aEnemy.size (); i ++) {
		ChallengeRemoveEnemy (aEnemy[i]);
	}
}

/* 敵を出す */
static void ChallengeCreateEnemy(DWORD dwSession, CHALLENGE *pCh, PQUESTNPCINFO pDef)
{
	PCHARINFO pNew;
	PSTATUSTEMPLATEINFO t;
	int nMapNo = GetMapNo (pDef->dwMapID);
	CCharProcBase CharProcBase;

	if ((nMapNo < 0) || (pDef->wStTmp >= (WORD)g_aStTmpInfo.GetSize ())) {
		return;
	}
	t = g_aStTmpInfo[pDef->wStTmp];

	pNew = new CHARINFO;
	ZeroMemory ((PBYTE)pNew, sizeof (CHARINFO));
	pNew->byNpcType = NPCTYPE_BATTLE2;
	SetInitCharData (pNew);
	pNew->dwSpaceID = pCh->dwSpaceID;						/* 敵は、その組の空間に出す */
	/* 倒した時の処理が戦闘 1・2 だけなので、それ以外の動き方は戦闘 2（探して攻撃）にする */
	pNew->byNpcType = ((t->byNpcType == NPCTYPE_BATTLE1) || (t->byNpcType == NPCTYPE_BATTLE2)) ? t->byNpcType : NPCTYPE_BATTLE2;
	pNew->byGameState	= GAMESTATE_NPC;
	pNew->byCharState	= CHARSTATE_MOVE;
	pNew->dwMapID		= (DWORD)nMapNo;
	pNew->dwStartMapID	= (DWORD)nMapNo;
	pNew->dwXBack = pNew->dwX = pNew->dwStartX = pDef->byX;
	pNew->dwYBack = pNew->dwY = pNew->dwStartY = pDef->byY;
	pNew->byDirection	= (pDef->byDir < 4) ? pDef->byDir : 1;
	pNew->wGrpID		= pDef->wGrpID;
	pNew->byLevel		= t->byLevel;
	pNew->byPW			= t->byPW;
	pNew->byDEX			= t->byDEX;
	pNew->byST			= t->byST;
	pNew->byIT			= t->byIT;
	pNew->byLK			= t->byLK;
	pNew->wMaxHP		= max (t->wMaxHP, (WORD)1);
	pNew->wHP			= pNew->wMaxHP;
	pNew->wMaxMana		= t->wMaxMana;
	pNew->wMana			= t->wMaxMana;
	pNew->wDFMagic		= t->wDFMagic;
	pNew->chAttrFire	= t->chAttrFire;
	pNew->chAttrWater	= t->chAttrWater;
	pNew->chAttrWind	= t->chAttrWind;
	pNew->chAttrEarth	= t->chAttrEarth;
	pNew->chAttrLight	= t->chAttrLight;
	pNew->chAttrDark	= t->chAttrDark;
	pNew->byMoveProbabilityX	= t->byMoveProbabilityX;
	pNew->byMoveProbabilityY	= t->byMoveProbabilityY;
	pNew->bySearchCx	= t->bySearchCx;
	pNew->bySearchCy	= t->bySearchCy;
	memcpy (pNew->abyMagic, t->abyMagic, sizeof (pNew->abyMagic));
	/* 経験値・お金・落とす物は無し（貸し出しの状態で増えても戻すので、はじめから出さない） */
	pNew->dwGold = 0;
	pNew->dwExp = 0;
	pNew->byDropProbability = 0;
	CopyStr (pNew->szName, sizeof (pNew->szName), pDef->szName[0] ? pDef->szName : t->szName);

	pNew = AddChar (pNew);
	pNew->pID = pNew->dwTblID + 1;
	if (pNew->pProc == NULL) {
		CharProcBase.SetProc (pNew);
	}
	RenewATDF (pNew->dwTblID);
	s_mapEnemy[pNew->dwTblID] = dwSession;
	pCh->aEnemy.push_back (pNew->dwTblID);
	SendCmdCHARINFO3 (pNew->dwTblID);
}

/* 入口に乗った：近くで同じ依頼を受けた仲間と一緒に入る */
static void ChallengeStart(PCHARINFO pChar, PQUESTINFO pQuest, PQUESTNPCINFO pDef)
{
	std::vector<PCHARINFO> aMember;
	PSTATUSTEMPLATEINFO t;
	CHALLENGE Ch;
	DWORD dwSession;
	int i, nNeed, nDestMapNo;
	char szTmp[512];

	if ((pQuest->byChallenge == QUESTCHALLENGE_NONE) || QuestInChallenge (pChar)) {
		return;
	}
	nDestMapNo = GetMapNo (pDef->dwDestMapID);
	if ((nDestMapNo < 0) || (pQuest->wLoanStTmp >= (WORD)g_aStTmpInfo.GetSize ())) {
		SendCmdSYSMSG (pChar->pID, "この討伐チャレンジは準備中です。", SPEECHCMD_BATTLE1);
		return;
	}
	if (QuestFlagCheck (pChar, QUEST_FLG_CLEARED)) {
		SendCmdSYSMSG (pChar->pID, "討伐はもう済んでいます。ギルドに報告しましょう。", SPEECHCMD_BATTLE1);
		return;
	}
	/* 組ごとに別の空間で戦うので、同じ依頼を何組でも同時に遊べる */
	nNeed = max ((int)pQuest->byMemberMin, 1);
	GetChallengeMembers (pChar, pQuest->wID, aMember);
	if ((int)aMember.size () < nNeed) {
		snprintf (szTmp, sizeof (szTmp), "一緒に挑む仲間が近くにそろっていません（%d 人必要です）。", nNeed);
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		return;
	}
	for (i = 0; i < (int)aMember.size (); i ++) {
		if (HasMercenary (aMember[i])) {
			snprintf (szTmp, sizeof (szTmp), "%sさんの傭兵は一緒に入れません。別れてから来てください。", aMember[i]->szName);
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
			return;
		}
	}

	t = g_aStTmpInfo[pQuest->wLoanStTmp];
	dwSession = ++ s_dwChallengeSeq;
	Ch.wQuestID = pQuest->wID;
	Ch.dwSpaceID = dwSession;						/* 挑戦の番号（1 から増える）をそのまま空間番号にする */
	Ch.dwMapNo = (DWORD)nDestMapNo;
	Ch.dwPartyID = pChar->dwPartyID;
	Ch.dwStart = timeGetTime ();
	Ch.bCleared = FALSE;
	s_mapChallenge[dwSession] = Ch;
	CHALLENGE *pCh = &s_mapChallenge[dwSession];

	/* 貸し出しの中身のお知らせ */
	{
		char szEqu[256] = "";
		snprintf (szTmp, sizeof (szTmp), "討伐チャレンジ開始！ みんな同じ状態です：Lv%d 力%d 素早さ%d 体力%d 賢さ%d 運%d HP%d Mana%d",
			max ((int)t->byLevel, 1), t->byPW, t->byDEX, t->byST, t->byIT, t->byLK, max ((int)t->wMaxHP, 1), t->wMaxMana);
		for (i = 0; i < MAX_MYITEM; i ++) {
			WORD wItem = t->waItem[i];
			if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
				continue;
			}
			snprintf (szEqu + strlen (szEqu), sizeof (szEqu) - strlen (szEqu), "%s%s", szEqu[0] ? "、" : "持ち物：", g_aItemInfo[wItem]->szName);
		}
		for (i = 0; i < (int)aMember.size (); i ++) {
			SendCmdSYSMSG (aMember[i]->pID, szTmp, SPEECHCMD_BATTLE1);
			if (szEqu[0]) {
				SendCmdSYSMSG (aMember[i]->pID, szEqu, SPEECHCMD_BATTLE1);
			}
			SendCmdSYSMSG (aMember[i]->pID, "倒れる・パーティを抜ける・切断すると、その人は中断して元の姿に戻ります。", SPEECHCMD_BATTLE1);
		}
	}

	for (i = 0; i < (int)aMember.size (); i ++) {
		PCHARINFO pTmp = aMember[i];
		LOANBAK Bak;

		CharTargetCancelPC (pTmp);
		/* この組の空間へ（先に移る。マップを移る時に、ふつうの世界の人へ「いなくなった」が届くように。
		   見え方の作り直しは移動のほうでされるので、ここでは送らない） */
		SpaceSet (pTmp, Ch.dwSpaceID, FALSE);
		ZeroMemory (&Bak, sizeof (Bak));
		LoanXfer (pTmp, &Bak, FALSE);
		Bak.dwSession = dwSession;
		Bak.bArrived = FALSE;
		Bak.dwMapID = pTmp->dwMapID;
		Bak.dwX = pTmp->dwX;
		Bak.dwY = pTmp->dwY;
		Bak.byDirection = pTmp->byDirection;
		s_mapLoan[pTmp->dwTblID] = Bak;
		pCh->aMember.push_back (pTmp->dwTblID);

		LoanApply (pTmp, t);
		LoanSendAll (pTmp);
		AdvanceQuest (pTmp, pDef);
		DoPortal (pTmp, pDef);
		/* 移動で届くマップ情報は扉が閉じたままなので、この空間の扉などの状態を送る */
		SpaceSendMapState (pTmp);
	}

	/* 敵を出す */
	for (i = 0; i < MAX_QUESTNPC; i ++) {
		if (pQuest->aNpc[i].byKind == QUESTNPCKIND_ENEMY) {
			ChallengeCreateEnemy (dwSession, pCh, &pQuest->aNpc[i]);
		}
	}
	for (i = 0; i < (int)aMember.size (); i ++) {
		QuestSendState (aMember[i]);
	}
}

/* キャラが消える・ログオフする時 */
static void ChallengeOnDecPlayer(PCHARINFO pChar)
{
	std::map<DWORD, DWORD>::iterator itE = s_mapEnemy.find (pChar->dwTblID);

	/* 敵が倒された */
	if (itE != s_mapEnemy.end ()) {
		std::map<DWORD, CHALLENGE>::iterator itC = s_mapChallenge.find (itE->second);
		s_mapEnemy.erase (itE);
		if (itC != s_mapChallenge.end ()) {
			std::vector<DWORD> &aE = itC->second.aEnemy;
			aE.erase (std::remove (aE.begin (), aE.end (), pChar->dwTblID), aE.end ());
			/* 全部倒した（人を動かすのは周期処理で） */
			if (aE.empty ()) {
				itC->second.bCleared = TRUE;
			}
		}
		return;
	}
	/* 挑戦中の人が切断した（全部倒した後の待ち時間なら成功にしてから） */
	if (QuestInChallenge (pChar)) {
		std::map<DWORD, LOANBAK>::iterator itL = s_mapLoan.find (pChar->dwTblID);
		std::map<DWORD, CHALLENGE>::iterator itC = s_mapChallenge.find (itL->second.dwSession);
		if ((itC != s_mapChallenge.end ()) && itC->second.bCleared) {
			ChallengeClearMember (pChar, &itC->second, CHALLENGELEAVE_LOGOUT);
		} else {
			ChallengeLeave (pChar, CHALLENGELEAVE_LOGOUT);
		}
	}
}

/* 全部倒した組の人を成功にして戻す（最後の段＋成功の印） */
static void ChallengeClearMember(PCHARINFO p, CHALLENGE *pCh, int nReason)
{
	PQUESTINFO pQuest = FindQuest (pCh->wQuestID);

	ChallengeLeave (p, nReason);
	if (pQuest) {
		p->byQuestStep = (BYTE)(max ((int)pQuest->byStepCount, 1) - 1);
	}
	p->wQuestFlg |= (WORD)(1 << (QUEST_FLG_CLEARED - 1));
	if (nReason == CHALLENGELEAVE_CLEAR) {
		SendCmdSYSMSG (p->pID, "元の姿に戻りました。ギルドに報告しましょう。", SPEECHCMD_BATTLE1);
		QuestSendState (p);
	}
}

/* 周期処理：倒れた・抜けた人を戻す、全部倒したら成功、誰もいなくなったら終わり */
static void ChallengeProc(void)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	std::vector<DWORD> aEnd;
	DWORD dwNow = timeGetTime ();
	size_t i;

	for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
		DWORD dwSession = itC->first;
		std::vector<DWORD> aMember = itC->second.aMember;

		if (itC->second.bCleared) {
			char szTime[64], szTmp[256];

			/* 全部倒した：まずその場で知らせて、少し間をおいてから戻す（倒した瞬間に戻すと何が起きたか分からない） */
			if (itC->second.dwClear == 0) {
				itC->second.dwClearTime = dwNow - itC->second.dwStart;
				itC->second.dwClear = dwNow;
				if (itC->second.dwClear == 0) {
					itC->second.dwClear = 1;
				}
				MakeTimeText (itC->second.dwClearTime, szTime, sizeof (szTime));
				snprintf (szTmp, sizeof (szTmp), "討伐成功！ タイム %s。まもなく元の場所へ戻ります…", szTime);
				for (i = 0; i < aMember.size (); i ++) {
					PCHARINFO p = g_aCharInfo[aMember[i]];
					SendCmdPLAYSOUND (p->pID, SOUND_PI52);
					SendCmdSYSMSG (p->pID, szTmp, SPEECHCMD_BATTLE1);
				}
				continue;
			}
			if (dwNow - itC->second.dwClear < CHALLENGE_CLEAR_WAIT) {
				continue;
			}
			for (i = 0; i < aMember.size (); i ++) {
				ChallengeClearMember (g_aCharInfo[aMember[i]], &itC->second, CHALLENGELEAVE_CLEAR);
			}
			aEnd.push_back (dwSession);
			continue;
		}

		for (i = 0; i < aMember.size (); i ++) {
			PCHARINFO p = g_aCharInfo[aMember[i]];
			std::map<DWORD, LOANBAK>::iterator itL = s_mapLoan.find (aMember[i]);
			int nReason = -1;

			if (itL == s_mapLoan.end ()) {
				continue;
			}
			if ((p->wHP == 0) || (p->byCharState == CHARSTATE_DEAD)) {
				nReason = CHALLENGELEAVE_DEAD;
			} else if (p->dwPartyID != itC->second.dwPartyID) {
				nReason = CHALLENGELEAVE_PARTY;
			} else if (p->wQuestID != itC->second.wQuestID) {
				nReason = CHALLENGELEAVE_QUEST;
			} else if (itL->second.bArrived) {
				if ((p->dwMapID != itC->second.dwMapNo) || itL->second.bWarped) {
					nReason = CHALLENGELEAVE_OUT;
				}
			} else if (p->dwMapID == itC->second.dwMapNo) {
				itL->second.bArrived = TRUE;
			} else if (dwNow - itC->second.dwStart > CHALLENGE_ARRIVE_WAIT) {
				nReason = CHALLENGELEAVE_OUT;
			}
			if (nReason >= 0) {
				ChallengeLeave (p, nReason);
			}
		}
		if (itC->second.aMember.empty ()) {
			aEnd.push_back (dwSession);
		}
	}
	for (i = 0; i < aEnd.size (); i ++) {
		ChallengeEnd (aEnd[i]);
	}
}

/* 今すぐ周期処理をする（テスト用） */
void QuestChallengeProcNow(void)
{
	ChallengeProc ();
}

/* マップの中のワープ（座標変更イベント）で動いた時。
 * 戦う部屋が入口と同じマップにあると、部屋を出てもマップが変わらないので、
 * 着いた後のワープは「部屋を出た」として周期処理で中断する（入る時のワープは着いた印にする） */
void QuestChallengeOnWarp(PCHARINFO pChar)
{
	std::map<DWORD, LOANBAK>::iterator itL = s_mapLoan.find (pChar->dwTblID);
	std::map<DWORD, CHALLENGE>::iterator itC;

	if (itL == s_mapLoan.end ()) {
		return;
	}
	if (itL->second.bArrived) {
		itL->second.bWarped = TRUE;
		return;
	}
	itC = s_mapChallenge.find (itL->second.dwSession);
	if ((itC != s_mapChallenge.end ()) && (pChar->dwMapID == itC->second.dwMapNo)) {
		itL->second.bArrived = TRUE;
	}
}

/* 倒れた人が「復活」を選んだ：挑戦中ならすぐ中断して入る前の場所へ（お金は減らさない）。処理したら TRUE */
BOOL QuestChallengeResurrection(PCHARINFO pChar)
{
	if (!QuestInChallenge (pChar)) {
		return FALSE;
	}
	ChallengeLeave (pChar, CHALLENGELEAVE_DEAD);
	return TRUE;
}

/* 保存の前後で呼ぶ：挑戦中の人は本物の状態と入る前の場所を保存させる（前後で同じ入れ替えをする）。
   切断した人の戻す場所もここで書き込む */
void QuestChallengeSaveSwap(BOOL bBegin)
{
	std::map<DWORD, LOANBAK>::iterator it;

	for (it = s_mapLoan.begin (); it != s_mapLoan.end (); it ++) {
		if (it->first < (DWORD)g_aCharInfo.GetSize ()) {
			LoanSwap (g_aCharInfo[it->first], &it->second);
		}
	}
	if (bBegin) {
		std::map<DWORD, LOANPOS>::iterator itPos = s_mapPendingPos.begin ();
		while (itPos != s_mapPendingPos.end ()) {
			PCHARINFO p = (itPos->first < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[itPos->first] : NULL;
			/* もうログインし直している人は、ログインの時に動かしている */
			if ((p != NULL) && (p->pID == 0)) {
				p->dwMapID = itPos->second.dwMapID;
				p->dwX = p->dwXBack = itPos->second.dwX;
				p->dwY = p->dwYBack = itPos->second.dwY;
				p->byDirection = itPos->second.byDirection;
			}
			s_mapPendingPos.erase (itPos ++);
		}
	}
}

/* ========================================================================= */
/* 空間																		 */
/* ========================================================================= */
/*
 * 空間番号(CHARINFO::dwSpaceID)が違うキャラ同士は、同じマップの同じ場所にいても出会わない
 * （見えない・ぶつからない・攻撃や会話ができない）。0 がふつうの世界。
 * 討伐チャレンジの組ごとに 1 つ作る（番号は挑戦の番号）。管理者は一覧から選んで入れる。
 *
 * 扉・「乗っている間変わるマス」はサーバーのマップの中身を書き換えずに、
 * ふつうの世界は今までどおり（扉は dwTimeOpen・マスは pwMap）、ほかの空間は下の表で持つ。
 * クライアントへは、その空間にいる人にだけ CMD_CHGMAP を送る。
 */

/* 空間ごとの扉・マスの持ち主を決める鍵（マップ・イベントの座標・空間） */
typedef std::tuple<DWORD, DWORD, DWORD, DWORD> SPACEKEY;

static std::map<SPACEKEY, DWORD> s_mapSpaceDoor;				/* ふつうの世界以外で開いている扉 → 開けた時刻 */
static std::map<SPACEKEY, BOOL> s_mapSpaceOnChar;				/* ふつうの世界以外で、誰かが乗っているマス */

/* 空間番号だけ書き換える */
void SpaceSetQuiet(PCHARINFO pChar, DWORD dwSpaceID)
{
	pChar->dwSpaceID = dwSpaceID;
}

/* 切り替えられる空間か */
BOOL SpaceExists(DWORD dwSpaceID)
{
	std::map<DWORD, CHALLENGE>::iterator itC;

	if (dwSpaceID == SPACE_NORMAL) {
		return TRUE;
	}
	for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
		if (itC->second.dwSpaceID == dwSpaceID) {
			return TRUE;
		}
	}
	return FALSE;
}

/* いま在る空間の一覧 */
int SpaceGetList(PADMINSPACEENTRY pEntry, int nMax)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	int i, nCount = 0, nChars = g_aCharInfo.GetSize ();

	for (itC = s_mapChallenge.begin (); (itC != s_mapChallenge.end ()) && (nCount < nMax); itC ++) {
		PADMINSPACEENTRY p = &pEntry[nCount];
		PQUESTINFO pQuest = FindQuest (itC->second.wQuestID);

		ZeroMemory (p, sizeof (ADMINSPACEENTRY));
		p->dwSpaceID = itC->second.dwSpaceID;
		p->wQuestID = itC->second.wQuestID;
		p->dwMapID = itC->second.dwMapNo;
		for (i = 0; i < nChars; i ++) {
			if (IsPlayerOnline (g_aCharInfo[i]) && (g_aCharInfo[i]->dwSpaceID == p->dwSpaceID)) {
				p->wMember ++;
			}
		}
		if (pQuest) {
			SboTextCopy (p->szQuest, sizeof (p->szQuest), pQuest->szName);
		}
		if ((itC->second.dwMapNo < (DWORD)g_aMapInfo.GetSize ()) && g_aMapInfo[itC->second.dwMapNo]->pszName) {
			SboTextCopy (p->szMap, sizeof (p->szMap), g_aMapInfo[itC->second.dwMapNo]->pszName);
		}
		nCount ++;
	}
	return nCount;
}

/* 扉がその空間で開いた時刻（0=閉まっている） */
static DWORD SpaceDoorTime(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	std::map<SPACEKEY, DWORD>::iterator it;

	if (dwSpaceID == SPACE_NORMAL) {
		return pDoor->dwTimeOpen;
	}
	it = s_mapSpaceDoor.find (SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, dwSpaceID));
	return (it == s_mapSpaceDoor.end ()) ? 0 : it->second;
}

/* 扉がその空間で開いているか */
BOOL SpaceDoorIsOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	return (pDoor != NULL) && (SpaceDoorTime (dwMapID, pEvent, pDoor, dwSpaceID) != 0);
}

/* 扉をその空間で開ける */
BOOL SpaceDoorOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	if ((pDoor == NULL) || SpaceDoorIsOpen (dwMapID, pEvent, pDoor, dwSpaceID)) {
		return FALSE;
	}
	if (dwSpaceID == SPACE_NORMAL) {
		pDoor->dwTimeOpen = timeGetTime ();
	} else {
		DWORD dwTime = timeGetTime ();
		s_mapSpaceDoor[SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, dwSpaceID)] = dwTime ? dwTime : 1;
	}
	SendCmdCHGMAPSpace (dwMapID, dwSpaceID, pDoor->wBackGrpID, (int)pEvent->dwX, (int)pEvent->dwY);
	return TRUE;
}

/* 扉が閉まる時間を見る */
void SpaceDoorProc(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor)
{
	if (pDoor == NULL) {
		return;
	}
	/* ふつうの世界 */
	if (pDoor->dwTimeOpen) {
		/* 閉める時間が過ぎた？ */
		if (g_dwTime > pDoor->dwTimeOpen + (pDoor->byTimeClose * 100)) {
			pDoor->dwTimeOpen = 0;
			SendCmdCHGMAPSpace (dwMapID, SPACE_NORMAL, pDoor->wGrpID, (int)pEvent->dwX, (int)pEvent->dwY);
		}
	}
	/* ほかの空間（開けた扉がある時だけ見る） */
	if (!s_mapSpaceDoor.empty ()) {
		std::map<SPACEKEY, DWORD>::iterator it = s_mapSpaceDoor.lower_bound (SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, 0));
		while ((it != s_mapSpaceDoor.end ()) &&
				(std::get<0> (it->first) == dwMapID) && (std::get<1> (it->first) == pEvent->dwX) && (std::get<2> (it->first) == pEvent->dwY)) {
			if (g_dwTime > it->second + (pDoor->byTimeClose * 100)) {
				DWORD dwSpaceID = std::get<3> (it->first);
				s_mapSpaceDoor.erase (it ++);
				SendCmdCHGMAPSpace (dwMapID, dwSpaceID, pDoor->wGrpID, (int)pEvent->dwX, (int)pEvent->dwY);
			} else {
				it ++;
			}
		}
	}
}

/* マスの上に、ふつうの世界以外のキャラが誰か乗っているか調べて、乗っている空間を集める */
static void SpaceCollectOnTile(DWORD dwMapID, DWORD dwX, DWORD dwY, std::vector<DWORD> &aSpace)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	int t, i;

	aSpace.clear ();
	for (t = 0; t < 2; t ++) {
		PDWORD pData = (t == 0) ? pMap->paTblNoPC : pMap->paTblNoNPC;
		int nCount = (int)((t == 0) ? pMap->dwCountTblNoPC : pMap->dwCountTblNoNPC);
		if (pData == NULL) {
			continue;
		}
		for (i = 0; i < nCount; i ++) {
			PCHARINFO pCharTmp;
			if (pData[i] == -1) {
				break;
			}
			pCharTmp = g_aCharInfo[pData[i]];
			if (pCharTmp->dwSpaceID == SPACE_NORMAL) {
				continue;
			}
			if (t == 0) {
				/* PC（倒れている人・ログイン中でない人は乗っていない） */
				if ((pCharTmp->wHP == 0) || (pCharTmp->byGameState < GAMESTATE_MOVE)) {
					continue;
				}
			} else {
				/* NPC（見えるNPC・置かれた魔法などは乗っていないことにする。CheckCharBlockNPC と同じ） */
				if (IsQuestNpc (pCharTmp) ||
					(pCharTmp->byNpcType == NPCTYPE_PUT) || (pCharTmp->byNpcType == NPCTYPE_USEMAGIC) ||
					(pCharTmp->byNpcType == NPCTYPE_ATACK) || (pCharTmp->byNpcType == NPCTYPE_USEMAGICPARENT)) {
					continue;
				}
			}
			if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
				if (std::find (aSpace.begin (), aSpace.end (), pCharTmp->dwSpaceID) == aSpace.end ()) {
					aSpace.push_back (pCharTmp->dwSpaceID);
				}
			}
		}
	}
}

/* 「乗っている間変わるマス」の処理 */
void SpaceOnCharMapProc(PMAPINFO pMap, PEVENTINFO pEvent, PEVENTINFO_ONCHARMAP pInfo)
{
	WORD wMap = pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX];

	/* ふつうの世界（マップの中身を書き換える） */
	/* 誰も乗ってない？ */
	if ((CheckCharBlockPC (pMap->dwID, pEvent->dwX, pEvent->dwY, SPACE_NORMAL) == -1) &&
		(CheckCharBlockNPC (pMap->dwID, pEvent->dwX, pEvent->dwY, SPACE_NORMAL) == -1)) {
		if (wMap != pInfo->wGrpIDOff) {
			pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX] = pInfo->wGrpIDOff;
			SendCmdCHGMAPSpace (pMap->dwID, SPACE_NORMAL, pInfo->wGrpIDOff, pInfo->dwX, pInfo->dwY);
		}
	/* 誰か乗っている */
	} else {
		if (wMap != pInfo->wGrpIDOn) {
			pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX] = pInfo->wGrpIDOn;
			SendCmdCHGMAPSpace (pMap->dwID, SPACE_NORMAL, pInfo->wGrpIDOn, pInfo->dwX, pInfo->dwY);
		}
	}

	/* ほかの空間（人がいる空間か、乗っている印が残っている空間だけ見る） */
	if (!s_mapSpaceOnChar.empty () || (pMap->dwCountTblNoPC + pMap->dwCountTblNoNPC > 0)) {
		std::vector<DWORD> aOn;
		size_t i;

		SpaceCollectOnTile (pMap->dwID, pEvent->dwX, pEvent->dwY, aOn);
		/* 新しく乗った空間 */
		for (i = 0; i < aOn.size (); i ++) {
			SPACEKEY Key (pMap->dwID, pEvent->dwX, pEvent->dwY, aOn[i]);
			if (s_mapSpaceOnChar.find (Key) == s_mapSpaceOnChar.end ()) {
				s_mapSpaceOnChar[Key] = TRUE;
				SendCmdCHGMAPSpace (pMap->dwID, aOn[i], pInfo->wGrpIDOn, pInfo->dwX, pInfo->dwY);
			}
		}
		/* 降りた空間 */
		if (!s_mapSpaceOnChar.empty ()) {
			std::map<SPACEKEY, BOOL>::iterator it = s_mapSpaceOnChar.lower_bound (SPACEKEY (pMap->dwID, pEvent->dwX, pEvent->dwY, 0));
			while ((it != s_mapSpaceOnChar.end ()) &&
					(std::get<0> (it->first) == pMap->dwID) && (std::get<1> (it->first) == pEvent->dwX) && (std::get<2> (it->first) == pEvent->dwY)) {
				DWORD dwSpaceID = std::get<3> (it->first);
				if (std::find (aOn.begin (), aOn.end (), dwSpaceID) == aOn.end ()) {
					s_mapSpaceOnChar.erase (it ++);
					SendCmdCHGMAPSpace (pMap->dwID, dwSpaceID, pInfo->wGrpIDOff, pInfo->dwX, pInfo->dwY);
				} else {
					it ++;
				}
			}
		}
	}
}

/* その人がいる空間の、扉・「乗っている間変わるマス」の状態を本人に送る。
   マップ情報(CMD_MAPINFO)にはサーバーのマップの中身（扉は閉じたまま）が入っているので、
   空間を移った人には、扉と乗っている間変わるマスの全部について、いまの状態を送り直す */
void SpaceSendMapState(PCHARINFO pChar)
{
	PMAPINFO pMap;
	int i;

	if ((pChar->pID == 0) || (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	for (i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pEvent = &pMap->pEvent[i];

		if (pEvent->pData == NULL) {
			continue;
		}
		switch (pEvent->byType) {
		case EVENTTYPE_DOOR:
			{
				PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEvent->pData;
				BOOL bOpen = SpaceDoorIsOpen (pMap->dwID, pEvent, pDoor, pChar->dwSpaceID);
				SendCmdCHGMAPTo (pChar->pID, pMap->dwID, bOpen ? pDoor->wBackGrpID : pDoor->wGrpID, (int)pEvent->dwX, (int)pEvent->dwY);
			}
			break;

		case EVENTTYPE_ONCHARMAP:
			{
				PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)pEvent->pData;
				BOOL bOn;
				if (pChar->dwSpaceID == SPACE_NORMAL) {
					bOn = (pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX] == pInfo->wGrpIDOn);
				} else {
					bOn = (s_mapSpaceOnChar.find (SPACEKEY (pMap->dwID, pEvent->dwX, pEvent->dwY, pChar->dwSpaceID)) != s_mapSpaceOnChar.end ());
				}
				SendCmdCHGMAPTo (pChar->pID, pMap->dwID, bOn ? pInfo->wGrpIDOn : pInfo->wGrpIDOff, (int)pInfo->dwX, (int)pInfo->dwY);
			}
			break;
		}
	}
}

/* 空間を移す（傭兵も一緒に） */
void SpaceSet(PCHARINFO pChar, DWORD dwSpaceID, BOOL bRefresh)
{
	int i, nCount;

	if (pChar->dwSpaceID == dwSpaceID) {
		return;
	}
	/* まだ遊んでいない・NPC・マップの表に入っていない時は、番号だけ */
	if (!IsPlayerOnline (pChar) || (GetMapNo (pChar->dwMapID) < 0)) {
		pChar->dwSpaceID = dwSpaceID;
		return;
	}

	/* ついていく・ターゲットは切る（空間をまたいでは続けられない） */
	CharTargetCancelPC (pChar);
	pChar->dwTargetTblID = -1;

	/* 前の空間の人から消える（向きを 99 にして座標を送るのが「いなくなった」の合図） */
	{
		BYTE byDirection = pChar->byDirection;
		pChar->byDirection = 99;
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 4);
		pChar->byDirection = byDirection;
	}
	/* 傭兵は一緒に移る */
	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pMerc = g_aCharInfo[i];
		if (!IsMercenary (pMerc) || (GetMercenaryOwner (pMerc) != pChar) || (pMerc->dwSpaceID == dwSpaceID)) {
			continue;
		}
		BYTE byDirection = pMerc->byDirection;
		pMerc->byDirection = 99;
		SendCmdCHARPOS (pMerc->pID, pMerc->dwTblID, 0, 4);
		pMerc->byDirection = byDirection;
		pMerc->dwSpaceID = dwSpaceID;
		SendCmdCHARINFO3 (pMerc->dwTblID);
	}

	pChar->dwSpaceID = dwSpaceID;

	if (bRefresh) {
		/* 見え方を作り直す（マップを移った時と同じ。クライアントはマップ情報を受け取ると、知っているキャラを全部忘れる） */
		SendCmdMAPINFO (pChar->pID, pChar->dwMapID);
		SendCmdEVENTPOS (pChar->pID, pChar->dwMapID);
		SpaceSendMapState (pChar);
		SendCmdCHARINFO (pChar->pID, -1);
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 2);
		SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
		SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
		/* 新しい空間の近くの人に自分を知らせる */
		SendCmdCHARINFO3 (pChar->dwTblID);
	}
	/* 依頼の見えるNPCは持ち主の空間に出すので、作り直す */
	QuestSyncAll ();
}

/* Copyright(C)URARA-works 2003-2005 */
