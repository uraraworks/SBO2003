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
#include "../../common/DungeonText.h"
#include "CharProc/IncludeCharProc.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrItemProc.h"
#include "SboSvrEventProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrRecycle.h"
#include "SboSvrDungeonDaily.h"
#include "CharProc/CharProcPUT.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <set>
#include <string>

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
	BYTE		abyMagicKey[MAX_MAGICKEY];							/* 特技ショートカット（中で登録し直しても、出たら元に戻す） */
	WORD		awKeyItem[MAX_MAGICKEY];							/* ショートカットの装備品・道具 */
	WORD		wMaxHP, wHP, wMaxMana, wMana;
	DWORD		dwGold, dwExp;
	char		chAttrFire, chAttrWater, chAttrWind, chAttrEarth, chAttrLight, chAttrDark;
	DWORD		dwMapID, dwX, dwY;									/* 入る前にいた場所（戻る場所・保存する場所） */
	BYTE		byDirection;
	WORD		wDgTmpl;											/* 番外編ダンジョン：入った時のステータステンプレートの番号（中で増えた物を数えるのに使う） */
} LOANBAK;

/* 空間の種類（CHALLENGE::byKind） */
#define CHALLENGEKIND_QUEST		(0)									/* 討伐チャレンジ */
#define DG_STATUS_KEEP			(3000)								/* 状況パネル（CMD_DGSTATUS）は、変わらなくてもこの間隔（ミリ秒）で送り直す */
#define CHALLENGEKIND_DUNGEON	(1)									/* 番外編ダンジョン（気まぐれダンジョンのミニゲーム） */

/* 番外編ダンジョンの、NPC発生の発生元ごとの数（組の空間ごとに数える） */
typedef struct _DGPUT {
	int			nNow;												/* いま生きている数 */
	int			nTotal;												/* この組で出した数（補充しない印の数え用） */
	DWORD		dwLast;												/* 最後に出した・倒された時刻 */
	DWORD		dwTry;												/* 出そうとした回数（日替わりの位置の乱数に混ぜる。出せなかった試しも数える） */
	DWORD		dwDefLast;											/* 拠点防衛：最後に出した時刻（周期処理の時刻。0=まだ出していない） */
	_DGPUT() : nNow(0), nTotal(0), dwLast(0), dwTry(0), dwDefLast(0) {}
} DGPUT;

/* 拠点防衛の進み具合（CHALLENGE::byDefPhase） */
#define DGDEF_PHASE_INIT	(0)									/* まだ始めていない（最初の周期処理で準備に入る） */
#define DGDEF_PHASE_PREP	(1)									/* 準備時間（次の波を待っている） */
#define DGDEF_PHASE_WAVE	(2)									/* 波の最中 */
/* 拠点防衛の結果（CHALLENGE::byDefResult） */
#define DGDEF_RESULT_NONE	(0)
#define DGDEF_RESULT_WIN	(1)									/* 最後の波をしのいだ */
#define DGDEF_RESULT_LOSE	(2)									/* 拠点が落ちた */

/* 拠点防衛：倒れた人の復活待ち */
typedef struct _DGDEFDEAD {
	int			nCount;												/* この組で倒れた回数（復活までの秒数＝回数×5、最大30） */
	DWORD		dwReviveAt;											/* 復活する時刻（0=いま倒れていない） */
	DWORD		dwSpringFx;											/* 泉でキラッとしたエフェクトを最後に出した時刻（0=まだ・泉の外に出たら 0 に戻す） */
	_DGDEFDEAD() : nCount(0), dwReviveAt(0), dwSpringFx(0) {}
} DGDEFDEAD;

/* 1 回の挑戦（一緒に入った組） */
typedef struct _CHALLENGE {
	BYTE		byKind;												/* CHALLENGEKIND_*（番外編ダンジョンは wQuestID・敵の一覧を使わない） */
	BYTE		byDungeonID;										/* 番外編：サブゲームの番号 */
	DWORD		dwBuildNo;											/* 番外編：入った時のダンジョンの作り直し番号（変わったら全員出す） */
	std::map<DWORD, DGPUT>	mapPut;									/* 番外編：NPC発生の発生元のテーブルID → この組の数 */
	BOOL		bDaily;												/* 番外編：日替わりのダンジョンに入った（記録をつける・敵の位置を版のシードで決める） */
	BOOL		bRanked;											/* 番外編：全員がその版で初めての挑戦（クリアしたら順位の記録に入れる。一発勝負） */
	DWORD		dwDailyVer;											/* 番外編：入った時の版の通し番号 */
	DWORD		dwDailySeed;										/* 番外編：その版のシード */
	std::map<DWORD, std::pair<std::string, std::string> >	mapWho;	/* 番外編：組の人のテーブルID → (キャラ名, 選んだ姿の名前)。記録には、クリアした時まで残っていた人だけを残す */
	BOOL		bKillAll;											/* 番外編：クリア条件が「敵を全部倒す」（補充しない印の敵がいる時だけ） */
	BYTE		byCountDown;										/* 番外編：戻る前の 3・2・1 の数えで、最後に知らせた数（4=まだ） */
	WORD		wQuestID;											/* 依頼番号 */
	DWORD		dwMapNo;											/* 戦う場所のマップ番号 */
	DWORD		dwSpaceID;											/* この挑戦の空間番号（挑戦の番号と同じ。0 にはならない。組ごとに別の空間で戦う） */
	BOOL		bArena;												/* 闘技場（ふつうの世界の人から見える＝観戦できる） */
	DWORD		dwPartyID;											/* 入った時のパーティ（変わった人は中断） */
	DWORD		dwStart;											/* 始めた時刻 */
	BOOL		bCleared;											/* 敵を全部倒した */
	DWORD		dwClear;											/* 全部倒したと知らせた時刻（0=まだ） */
	DWORD		dwClearTime;										/* かかった時間（ミリ秒） */
	std::vector<DWORD>	aMember;									/* 挑戦中の人（テーブルID） */
	BOOL bMapShared = FALSE;
	std::set<WORD> setMapRecord;
	std::vector<BYTE> aMapPositions;
	std::vector<DWORD>	aEnemy;										/* 残っている敵（テーブルID） */
	BOOL		bDefense;											/* 番外編：拠点防衛（組の空間に拠点キャラがいて、波ごとに敵が来る） */
	DWORD		dwBaseTbl;											/* 拠点防衛：拠点キャラのテーブルID（(DWORD)-1=まだ・いない） */
	int			nWave;												/* 拠点防衛：今の波（準備中は次に来る波。1〜） */
	BYTE		byDefPhase;											/* 拠点防衛：DGDEF_PHASE_* */
	BYTE		byDefResult;										/* 拠点防衛：DGDEF_RESULT_* */
	DWORD		dwDefPhaseStart;									/* 拠点防衛：今の準備に入った時刻（周期処理の時刻） */
	DWORD		dwDefWaveStart;										/* 拠点防衛：今の波が始まった時刻 */
	DWORD		dwDefEnd;											/* 拠点防衛：結果を知らせた時刻（この 10 秒後に全員帰還） */
	DWORD		dwDefSpring;										/* 拠点防衛：最後に泉で Mana を戻した時刻 */
	int			nEnemyLeft;											/* 拠点防衛：最後に確かめた「残り敵」の数（-1=波が始まったばかりでまだ確かめていない） */
	BYTE		byBaseBracket;										/* 拠点防衛：最後に知らせた拠点HPの段（0〜4。4=満タン） */
	std::map<DWORD, DGDEFDEAD>	mapDead;							/* 拠点防衛：組の人のテーブルID → 倒れた回数・復活の時刻 */
	CMDPARA_DGDEFSTATUS	stDefSent;									/* 拠点防衛：最後に組へ送った画面の帯の状態（変わった時だけ送るため） */
	DWORD		dwDefSentAt;										/* 拠点防衛：その状態を送った時刻（周期処理の時刻。一定時間ごとに同じ内容も送り直す） */
	CMDPARA_DGSTATUS	stStSent;									/* 拠点防衛以外：最後に組へ送った状況パネルの状態（変わった時だけ送るため） */
	DWORD		dwStSentAt;											/* 拠点防衛以外：その状態を送った時刻（周期処理の時刻。数秒ごとに送り直して経過秒を合わせる） */
	_CHALLENGE() : byKind(0), byDungeonID(0), dwBuildNo(0), bDaily(FALSE), bRanked(FALSE), dwDailyVer(0), dwDailySeed(0), bKillAll(FALSE), byCountDown(4), wQuestID(0), dwMapNo(0), dwSpaceID(0), bArena(FALSE), dwPartyID(0), dwStart(0), bCleared(FALSE), dwClear(0), dwClearTime(0), bDefense(FALSE), dwBaseTbl((DWORD)-1), nWave(0), byDefPhase(DGDEF_PHASE_INIT), byDefResult(DGDEF_RESULT_NONE), dwDefPhaseStart(0), dwDefWaveStart(0), dwDefEnd(0), dwDefSpring(0), nEnemyLeft(-1), byBaseBracket(4), dwDefSentAt(0), dwStSentAt(0) { ZeroMemory (&stDefSent, sizeof (stDefSent)); ZeroMemory (&stStSent, sizeof (stStSent)); }
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
static void DgBagCollect(PCHARINFO p, WORD wTmpl, std::vector<WORD> &aBag);
static void DgLeaveMsg(PCHARINFO p, int nReason, int nDungeonID);
static void DgLeaveAskClose(PCHARINFO p, BOOL bSend);
static void DgProc(DWORD dwNow);
static CHALLENGE *DgRunOf(PCHARINFO p);
static void DgDefCreateBase(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon);
static void DgDefCreateSpringMark(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon);
static void DgDefPutProc(CHALLENGE *pCh, DWORD dwNow);
static void DgDefProc(CHALLENGE &Ch, PSUBGAME_DUNGEONINFO pDungeon, DWORD dwNow);
static void DgDefFieldDrop(DWORD dwSpaceID);
static void DgSelOnDec(PCHARINFO pChar);
static void DgReceptionForget(PCHARINFO pChar);
static BOOL HasMercenary(PCHARINFO pChar);
static void ChallengeLeave(PCHARINFO p, int nReason);

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

/* 報酬をもうもらっている印（思い出の本でやり直してクリア済みの印が消えても残る。報酬は初回だけ） */
BOOL QuestIsRewarded(PCHARINFO pChar, WORD wQuestID)
{
	if (wQuestID >= MAX_QUEST) {
		return FALSE;
	}
	return (pChar->abyQuestRewarded[wQuestID / 8] & (1 << (wQuestID % 8))) ? TRUE : FALSE;
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

extern PCHARINFO GetMercenaryOwner(PCHARINFO pMerc);			/* 傭兵の雇い主（CharProcMERCENARY.cpp） */

/* 印の条件（wShowFlag・wHideFlag）で、pSrc が pViewer に見えるか。空間は見ない。
   管理者も一般プレイヤーと同じく印どおりに見える（「印で隠れたNPCも表示」をオンにした管理者にだけ、常に見える）。プレイヤーでない相手（敵・町の人）には見える（傭兵だけは雇い主の目で見る） */
BOOL CharViewCondOk(PCHARINFO pSrc, PCHARINFO pViewer)
{
	if (pViewer == NULL) {
		return FALSE;
	}
	if (pViewer->byNpcType != NPCTYPE_INIT) {
		pViewer = GetMercenaryOwner (pViewer);
		if (pViewer == NULL) {
			return TRUE;
		}
	}
	if ((pViewer->byAdminLV != ADMINLV_USER) && pViewer->byViewAllNpc) {
		return TRUE;
	}
	/* wShowFlag・wHideFlag は「番号+1（0=無し）」なので、番号に戻してから見る */
	WORD wShow = CharFlagFromWire (pSrc->wShowFlag);
	WORD wHide = CharFlagFromWire (pSrc->wHideFlag);
	if ((wShow < MAX_FLAGBIT) && !FlagBitGet (pViewer->abyFlag, wShow)) {
		return FALSE;
	}
	if ((wHide < MAX_FLAGBIT) && FlagBitGet (pViewer->abyFlag, wHide)) {
		return FALSE;
	}
	return TRUE;
}

/* pSrc の情報を pDst に送ってよいか */
BOOL IsCharVisibleTo(PCHARINFO pSrc, PCHARINFO pDst)
{
	/* 空間が違えば見えない（管理者も、いま居る空間のものしか見えない）。
	   ただし闘技場の空間とふつうの世界は、おたがいに見える（観客は戦いを観戦し、挑戦者からも観客が見える。
	   見えるだけで、ぶつかり・攻撃・魔法・会話などは届かない） */
	if (pDst != NULL) {
		if (pSrc->dwSpaceID != pDst->dwSpaceID) {
			if (!(((pDst->dwSpaceID == SPACE_NORMAL) && SpaceIsArena (pSrc->dwSpaceID)) ||
				  ((pSrc->dwSpaceID == SPACE_NORMAL) && SpaceIsArena (pDst->dwSpaceID)))) {
				return FALSE;
			}
		}
	} else if (pSrc->dwSpaceID != 0) {
		if (!SpaceIsArena (pSrc->dwSpaceID)) {
			return FALSE;
		}
	}
	if (!IsQuestNpc (pSrc)) {
		/* 印で見えるNPCは、受け取る人の印で決める（受け取る人が分からない一斉送信には送らない） */
		if (!CharHasViewCond (pSrc)) {
			return TRUE;
		}
		if (pDst == NULL) {
			return FALSE;
		}
		return CharViewCondOk (pSrc, pDst);
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

	/* ふつうの世界にいる、見えるNPC・印で見えるNPC以外は誰にでも送ってよい（人の多い所で毎回通るので先に返す） */
	if (!IsQuestNpc (pSrc) && !CharHasViewCond (pSrc) && (pSrc->dwSpaceID == 0)) {
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

/* 条件付きNPC 1 体の見え方を、1 人にだけ送り直す（見えるなら CHARINFO、見えないなら消す通知）。
   見えるものは、画面の近くにいる時だけ送る（遠くのものを送ると、クライアントが「遠いキャラ」として消してしまうため）。
   消す通知は、遠くても送る（前に見えていたものが残らないように） */
static void ViewRefreshOne(PCHARINFO pNpc, PCHARINFO pDst)
{
	if ((pDst->pID == 0) || (pDst->byGameState < GAMESTATE_MOVE) || (pNpc->byGameState < GAMESTATE_MOVE)) {
		return;
	}
	if (!IsSameSpace (pNpc, pDst)) {
		return;
	}
	if (IsCharVisibleTo (pNpc, pDst)) {
		if ((abs ((int)pNpc->dwX - (int)pDst->dwX) <= DRAW_PARTS_X + 2) &&
			(abs ((int)pNpc->dwY - (int)pDst->dwY) <= DRAW_PARTS_Y + 2)) {
			SendCmdCHARINFO (pDst->pID, (int)pNpc->dwTblID);
		}
	} else {
		SendCmdCHARHIDE (pDst->pID, (int)pNpc->dwTblID);
	}
}

/* ========================================================================= */
/* 印で変わるマス（MAPSWAP）													 */
/*   見た目と通れるか（当たり判定）を、その人の印で差し替える。				 */
/*   差し替えるのは本人（プレイヤー）の目だけ。NPC・敵・傭兵・宝箱の敵などは	 */
/*   ふつうにベースのパーツで動く。「印で隠れたNPCも表示」をオンにした管理者は	 */
/*   ベースのパーツのまま見える（印を見ずに全部見る設定なので）。			 */
/* ========================================================================= */

/* 差し替えを見る人か（プレイヤーのキャラで、印どおりに見る人）。違えば NULL */
static PCHARINFO MapSwapViewer(PCHARINFO pChar)
{
	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return NULL;
	}
	if ((pChar->byAdminLV != ADMINLV_USER) && pChar->byViewAllNpc) {
		return NULL;
	}
	return pChar;
}

/* 開けた宝箱の記録（キャラごとのビット。番号は全マップ通し 0〜2047） */
BOOL ChestOpenedGet(const PCHARINFO pChar, int nChestNo)
{
	if ((pChar == NULL) || (nChestNo < 0) || (nChestNo >= MAX_CHESTNO)) {
		return FALSE;
	}
	return (pChar->abyChestOpened[nChestNo / 8] & (1 << (nChestNo % 8))) ? TRUE : FALSE;
}

void ChestOpenedSet(PCHARINFO pChar, int nChestNo, BOOL bOn)
{
	if ((pChar == NULL) || (nChestNo < 0) || (nChestNo >= MAX_CHESTNO)) {
		return;
	}
	if (bOn) {
		pChar->abyChestOpened[nChestNo / 8] |= (BYTE)(1 << (nChestNo % 8));
	} else {
		pChar->abyChestOpened[nChestNo / 8] &= (BYTE)~(1 << (nChestNo % 8));
	}
}

/* この行の差し替えが、いま効いているか（宝箱は開けた記録、リサイクル箱は窓を開いている間、印で変わるは印を見る） */
static BOOL MapSwapIsActive(const MAPSWAP *pSwap, const PCHARINFO pViewer)
{
	BOOL bOn;

	if (pSwap->byKind == MAPSWAPKIND_RECYCLE) {
		return (pSwap->wBoxNo != 0) && (pViewer->wRecycleBox == pSwap->wBoxNo);
	}
	if (pSwap->byKind == MAPSWAPKIND_CHEST) {
		return ChestOpenedGet (pViewer, pSwap->wChestNo);
	}
	if (pSwap->wFlag >= MAX_FLAGBIT) {
		return FALSE;
	}
	bOn = FlagBitGet (pViewer->abyFlag, pSwap->wFlag) ? TRUE : FALSE;
	return (pSwap->byCmp == MAPSWAPCMP_OFF) ? !bOn : bOn;
}

/* 調べると開く行か（宝箱の行：まだ開けていない人が目の前で調べると、品を渡して記録を立てて文を出す。
   リサイクル箱の行：調べると、その人の画面だけ開いた絵になって窓が開く） */
static BOOL MapSwapIsExamineRow(const MAPSWAP *pSwap)
{
	if (pSwap->byKind == MAPSWAPKIND_RECYCLE) {
		return (pSwap->wBoxNo >= 1) && (pSwap->wBoxNo <= MAX_RECYCLEBOX);
	}
	return (pSwap->byKind == MAPSWAPKIND_CHEST) && (pSwap->wChestNo < MAX_CHESTNO);
}

/* (x,y) のパーツ。pViewer の印で差し替えた後の番号（pViewer が NULL・プレイヤー以外ならベースのまま） */
WORD MapSwapParts(PMAPINFO pMap, PCHARINFO pViewer, int x, int y)
{
	WORD wBase;
	int i;

	wBase = pMap->pwMap[x + y * pMap->nCX];
	if (pMap->bySwapCount == 0) {
		return wBase;
	}
	pViewer = MapSwapViewer (pViewer);
	if (pViewer == NULL) {
		return wBase;
	}
	for (i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
		const MAPSWAP *pSwap = &pMap->aSwap[i];

		if ((pSwap->wX != x) || (pSwap->wY != y) || (pSwap->wParts >= g_wCountMapParts)) {
			continue;
		}
		if (MapSwapIsActive (pSwap, pViewer)) {
			return pSwap->wParts;
		}
	}
	return wBase;
}

/* pChar の差し替え一覧を pMap について本人に送る。bForce=FALSE の時、送るものが無ければ何も送らない */
void MapSwapSend(PCHARINFO pChar, PMAPINFO pMap, BOOL bForce)
{
	MAPSWAPSEND aSend[MAX_MAPSWAP];
	PCHARINFO pViewer;
	int i, nCount = 0;

	if ((pChar == NULL) || (pChar->pID == 0) || (pMap == NULL)) {
		return;
	}
	/* 開いていたリサイクル箱がこのマップに無ければ（マップを移った・箱を消した）、開いた記憶を捨てる */
	if ((pChar->wRecycleBox != 0) && (RecycleFindRow (pMap, pChar->wRecycleBox) == NULL)) {
		pChar->wRecycleBox = 0;
	}
	pViewer = MapSwapViewer (pChar);
	if (pViewer != NULL) {
		for (i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
			const MAPSWAP *pSwap = &pMap->aSwap[i];
			BYTE byFlg = 0;

			if ((pSwap->wX >= pMap->nCX) || (pSwap->wY >= pMap->nCY)) {
				continue;
			}
			if ((pSwap->wParts < g_wCountMapParts) && MapSwapIsActive (pSwap, pViewer)) {
				byFlg |= MAPSWAPSEND_APPLY;
			} else if (MapSwapIsExamineRow (pSwap) && !MapSwapIsActive (pSwap, pViewer)) {
				byFlg |= MAPSWAPSEND_EXAMINE;
			}
			if (byFlg == 0) {
				continue;
			}
			aSend[nCount].wX = pSwap->wX;
			aSend[nCount].wY = pSwap->wY;
			aSend[nCount].wParts = pSwap->wParts;
			aSend[nCount].byFlg = byFlg;
			nCount ++;
		}
	}
	if ((nCount == 0) && !bForce && (pMap->bySwapCount == 0)) {
		return;
	}
	SendCmdMAPSWAP (pChar->pID, pMap->dwID, aSend, nCount);
}

/* マップ pMap の中にいるプレイヤー全員に、差し替え一覧を送り直す（マップの表やパーツを書き換えた時） */
void MapSwapSendMap(PMAPINFO pMap)
{
	int i;

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];

		if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT) ||
			(pChar->byGameState < GAMESTATE_MOVE) || (pChar->dwMapID != pMap->dwID)) {
			continue;
		}
		MapSwapSend (pChar, pMap, TRUE);
	}
}

/*
 * マップにいるプレイヤーへ、マップに入った時と同じ順で情報を送り直す
 * （SubProcMapMove と同じ: マップ情報→イベント→キャラ→自分の位置・ステータス・持ち物）。
 * クライアントはマップ情報を受け取るとキャラ情報を全部消すので、キャラも必ず送る。
 * データ API のマス目書き込み（tiles/set）とダンジョンの作り直しで使う（MAPINFO だけ送るとクライアントが固まる）
 */
void ResendMapToPlayers(PMAPINFO pMap, int nID)
{
	int i, j;

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar && pChar->pID && pChar->szAcc[0] && pChar->dwMapID == pMap->dwID && pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdMAPINFO (pChar->pID, pMap->dwID);
			SendCmdEVENTCOUNT (pChar->pID, (DWORD)nID);
			for (j = 0; j < pMap->nEventCount; j ++) {
				SendCmdEVENTINFO (pChar->pID, nID, j);
			}
			SendCmdEVENTPOS (pChar->pID, (DWORD)nID);
			SendCmdCHARINFO (pChar->pID, -1);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 2);
			SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
			SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
			SendCmdSYSMSG (pChar->pID, "SYSTEM:マップの情報が更新されたので読み込み直しました。");
		}
	}
}

/* 目の前（byData=2 の調べる）のマスが「調べると開く」マス（宝箱・リサイクル箱）なら開ける。処理した時 TRUE。
   印がまだ立っていない人が調べると、品を渡して（受け取れなければ何も動かさない）印を立てて文を出す */
BOOL MapSwapExamine(PCHARINFO pChar)
{
	PMAPINFO pMap;
	int i, nNo, x, y;

	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->byDirection >= 4)) {
		return FALSE;
	}
	nNo = GetMapNo (pChar->dwMapID);
	if (nNo < 0) {
		return FALSE;
	}
	pMap = g_aMapInfo[nNo];
	if (pMap->bySwapCount == 0) {
		return FALSE;
	}
	x = (int)pChar->dwX + anPosAddTblX[pChar->byDirection];
	y = (int)pChar->dwY + anPosAddTblY[pChar->byDirection];
	for (i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
		const MAPSWAP *pSwap = &pMap->aSwap[i];
		char szMsg[MAX_MAPSWAPMSG + 8];

		if ((pSwap->wX != x) || (pSwap->wY != y) || !MapSwapIsExamineRow (pSwap)) {
			continue;
		}
		/* リサイクル箱：その人の画面だけ開いた絵にして、窓を開かせる（討伐チャレンジ中は使えないので何も起きない） */
		if (pSwap->byKind == MAPSWAPKIND_RECYCLE) {
			if ((pChar->szAcc[0] == 0) || QuestInChallenge (pChar)) {
				if (QuestInChallenge (pChar)) {
					SendCmdSYSMSG (pChar->pID, QuestInDungeon (pChar) ? DungeonBlockedText (pChar).c_str () : "討伐チャレンジ中はできません。", SPEECHCMD_BATTLE1);
				}
				SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			} else {
				RecycleOpen (pChar, pMap, (PMAPSWAP)pSwap);
			}
			return TRUE;
		}
		if (ChestOpenedGet (pChar, pSwap->wChestNo)) {
			continue;			/* もう開けた人は何も起きない（ほかのイベントに任せる） */
		}
		/* 品を渡す（宝箱・会話データと同じ渡し方と知らせ）。受け取れない時は記録を立てず、同じ知らせだけ出る */
		/* 知らせの白文字は、文の窓を送った後に流す（窓が開いたまま重ならないように）。失敗の知らせは窓が無いのでその場で送る */
		QUESTNOTICEVEC vLater;
		if ((pSwap->wGiveItem != 0) && !QuestTalkExchange (pChar, 0, pSwap->wGiveItem, &vLater)) {
			QuestNoticeFlush (pChar, &vLater);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			return TRUE;
		}
		ChestOpenedSet (pChar, pSwap->wChestNo, TRUE);
		if (pSwap->szMsg[0]) {
			SboTextCopy (szMsg, MAX_MAPSWAPMSG + 4, pSwap->szMsg);
			ItemMsgAddWait (szMsg, sizeof (szMsg));
			SendCmdMAPMSG (pChar->pID, szMsg);
			QuestNoticeFlush (pChar, &vLater);
		} else {
			/* クライアントは返事が来るまで歩けないので、位置を送って待ちを解く（窓は無いので知らせはそのまま流す） */
			QuestNoticeFlush (pChar, &vLater);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		}
		MapSwapSend (pChar, pMap, FALSE);		/* 記録が動いたので、差し替えを本人にだけ送り直す */
		return TRUE;
	}
	return FALSE;
}

/* 印が変わった時、その人がいるマップの条件付きNPCの見え方を、その人にだけ送り直す */
void CharViewRefresh(PCHARINFO pChar)
{
	PMAPINFO pMap;
	PDWORD pData;
	int i;

	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	/* 印で変わるマスの差し替えも、その人にだけ送り直す（印が変わった全部の所からここを通る） */
	if ((pChar->byGameState >= GAMESTATE_MOVE) && (pChar->byNpcType == NPCTYPE_INIT)) {
		MapSwapSend (pChar, pMap, FALSE);
	}
	pData = pMap->paTblNoNPC;
	if (pData == NULL) {
		return;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pNpc = g_aCharInfo[pData[i]];
		if (pNpc->byNpcType == NPCTYPE_QUESTNPC) {
			continue;
		}
		if (CharHasViewCond (pNpc)) {
			ViewRefreshOne (pNpc, pChar);
		}
	}
}

/* 条件付きNPCの条件を変えた時、そのマップの全員に見え直しを送る */
void CharViewRefreshNpc(PCHARINFO pNpc)
{
	PMAPINFO pMap;
	PDWORD pData;
	int i;

	if ((pNpc == NULL) || (pNpc->byNpcType == NPCTYPE_INIT) || (pNpc->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return;
	}
	pMap = g_aMapInfo[pNpc->dwMapID];
	pData = pMap->paTblNoPC;
	if (pData == NULL) {
		return;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		ViewRefreshOne (pNpc, g_aCharInfo[pData[i]]);
	}
}

/* 闘技場の空間か（ふつうの世界から観戦できる） */
BOOL SpaceIsArena(DWORD dwSpaceID)
{
	std::map<DWORD, CHALLENGE>::iterator itC;

	if (dwSpaceID == SPACE_NORMAL) {
		return FALSE;
	}
	itC = s_mapChallenge.find (dwSpaceID);			/* 空間番号は挑戦の番号と同じ */
	return (itC != s_mapChallenge.end ()) && itC->second.bArena && (itC->second.dwSpaceID == dwSpaceID);
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

static int FillStoryGoals(PCHARINFO pChar, STORYGOALENTRY *paOut, int nMax);	/* 物語の「いまの目的」を集める（思い出の本の節で定義） */

/* 1 人に、その人とパーティの進み具合を送る（物語の目的は本人の分だけ） */
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
	Para.byStoryCount = (BYTE)FillStoryGoals (pTo, Para.aStory, MAX_STORYGOAL);
	/* 物語の目的は byStoryCount 件ぶんだけ送る（可変長。目的が無ければ byStoryCount の 1 バイトまで） */
	svrSock.SendTo (pTo->pID, (LPBYTE)&Para, (DWORD)(offsetof (CMDPARA_QUESTSTATE, aStory) + sizeof (STORYGOALENTRY) * Para.byStoryCount), SENDTIMEOUT);
}

/* 本人だけに、依頼と物語の「いまの目的」を送り直す（印・数値が動いた時） */
void QuestSendStateSelf(PCHARINFO pChar)
{
	SendStateTo (pChar);
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

/* 持ち物（大事な物の欄を含む）と預かり所にある数 */
static int CountItem(PCHARINFO pChar, WORD wItem, int *pnInv, int *pnLeave)
{
	int i, nInv = 0, nLeave = 0;

	nInv += CountImportantItem (pChar, wItem);

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

/* 大事な物の欄か持ち物か預かり所から 1 個取り除く（大事な物の欄、持ち物の順）。取ったら TRUE */
static BOOL TakeItem(PCHARINFO pChar, WORD wItem)
{
	int i;

	if (DelImportantItem (pChar, wItem)) {
		return TRUE;
	}
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

/* テスト用：持ち物（大事な物の欄を含む）と預かり所にある数・1 個取り除く */
int QuestCountItemAll(PCHARINFO pChar, WORD wItem)
{
	return CountItem (pChar, wItem, NULL, NULL);
}

BOOL QuestTakeItemOne(PCHARINFO pChar, WORD wItem)
{
	return TakeItem (pChar, wItem);
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
	if (CanAddCharItem (pChar->dwTblID, wItem)) {
		AddCharItem (pChar->dwTblID, wItem);
		return TRUE;
	}
	/* 大事な物は預かり所へ送らない */
	if ((pbToLeave == NULL) || IsImportantItem (wItem)) {
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

/* 品を渡す知らせの白文字を送る。pvLater があれば送らずに溜める（窓が先に開くよう、窓を送った後でまとめて送るため） */
static void QuestNoticeSend(PCHARINFO pChar, QUESTNOTICEVEC *pvLater, const char *pszMsg)
{
	if (pvLater) {
		pvLater->push_back (pszMsg);
		return;
	}
	SendCmdSYSMSG (pChar->pID, (LPSTR)pszMsg, SPEECHCMD_BATTLE1);
}

/* 溜めた知らせを、溜めた順に送って空にする */
void QuestNoticeFlush(PCHARINFO pChar, QUESTNOTICEVEC *pvLater)
{
	if (pvLater == NULL) {
		return;
	}
	for (size_t i = 0; i < pvLater->size (); i ++) {
		SendCmdSYSMSG (pChar->pID, (LPSTR)(*pvLater)[i].c_str (), SPEECHCMD_BATTLE1);
	}
	pvLater->clear ();
}

/* 会話データからアイテムを 1 つ渡して知らせる
   持ち物へ、いっぱいなら預かり所へ（大事な物の印のある品は大事な物の欄へ入る）。渡せたら TRUE。
   pvLater があれば、知らせは送らずに溜める（セリフの窓の後に流すため） */
BOOL QuestGiveItemNotice(PCHARINFO pChar, WORD wItem, QUESTNOTICEVEC *pvLater)
{
	BOOL bToLeave = FALSE;
	char szTmp[256];

	if (!GiveItem (pChar, wItem, &bToLeave)) {
		QuestNoticeSend (pChar, pvLater, "持ち物がいっぱいで受け取れませんでした。");
		return FALSE;
	}
	SendBagState (pChar);
	SendCmdPLAYSOUND (pChar->pID, SOUND_PI52);
	snprintf (szTmp, sizeof (szTmp), "%sを手に入れました。", ItemName (wItem));
	QuestNoticeSend (pChar, pvLater, szTmp);
	if (bToLeave) {
		snprintf (szTmp, sizeof (szTmp), "持ちきれないので、預かり所の使い魔が%sの預かり所へ運んでいきました", pChar->szName);
		QuestNoticeSend (pChar, pvLater, szTmp);
	}
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* 会話データの引き換え（取り上げる品 wTakeItem と渡す品 wGiveItem）				 */
/* ------------------------------------------------------------------------- */

/* 取り上げられる品を探す。戻り値：QUESTTAKE_NONE=持っていない、QUESTTAKE_BAG=手持ち（*pnIdx に位置）、
   QUESTTAKE_IMPORTANT=大事な物の欄。手持ちを先に見て、装備中の品は飛ばす（装備していない同じ品があればそれを取る） */
int QuestTakeFind(PCHARINFO pChar, WORD wItem, int *pnIdx)
{
	int i, j;

	if ((pChar == NULL) || (wItem == 0)) {
		return QUESTTAKE_NONE;
	}
	for (i = 0; i < MAX_MYITEM; i ++) {
		BOOL bEqu = FALSE;

		if (pChar->waItem[i] != wItem) {
			continue;
		}
		for (j = 0; j < MAX_EQUTYPE; j ++) {
			if (pChar->waEqu[j] == i) {
				bEqu = TRUE;
			}
		}
		if (!bEqu) {
			if (pnIdx) {
				*pnIdx = i;
			}
			return QUESTTAKE_BAG;
		}
	}
	if (CountImportantItem (pChar, wItem) > 0) {
		return QUESTTAKE_IMPORTANT;
	}
	return QUESTTAKE_NONE;
}

/* 預かり所へ 1 つ足せるか（足さずに確かめるだけ。AddLeaveItem と同じ条件） */
static BOOL LeaveCanAdd(PCHARINFO pChar, WORD wItem)
{
	int i;

	if (g_aLeaveInfo.GetSize () == 0) {
		return FALSE;
	}
	if ((pChar->dwLeaveID == 0) || (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ())) {
		return TRUE;	/* 口座が無ければ作れる */
	}
	PLEAVEINFO pLeave = g_aLeaveInfo[pChar->dwLeaveID];
	for (i = 0; (i < pLeave->byCount) && (i < MAX_LEAVEITEM); i ++) {
		if ((pLeave->waItem[i] == wItem) && (pLeave->byItemCount[i] < 255)) {
			return TRUE;
		}
	}
	return pLeave->byCount < MAX_LEAVEITEM;
}

/* nTakeKind の品を 1 つ取り上げた後で、wGive を受け取れるか（持ち物へ、いっぱいなら預かり所へ。大事な物は預かり所へ送らない）。
   取り上げて空く枠も数える（引換券で持ち物がいっぱいでも、券と引き換えなら入る） */
BOOL QuestCanGiveAfterTake(PCHARINFO pChar, WORD wGive, int nTakeKind)
{
	int i, nBag = 0, nImp = 0;

	if ((wGive == 0) || (wGive >= (WORD)g_aItemInfo.GetSize ())) {
		return FALSE;
	}
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			nBag ++;
		}
	}
	for (i = 0; i < MAX_IMPORTANT; i ++) {
		if (pChar->waImportant[i]) {
			nImp ++;
		}
	}
	if (nTakeKind == QUESTTAKE_BAG) {
		nBag --;
	} else if (nTakeKind == QUESTTAKE_IMPORTANT) {
		nImp --;
	}
	if (IsImportantItem (wGive)) {
		return nImp < MAX_IMPORTANT;
	}
	if (nBag < MAX_MYITEM) {
		return TRUE;
	}
	return LeaveCanAdd (pChar, wGive);
}

/* 会話の行のアイテムの動き（取り上げる → 渡す）。印・数値を動かしてよい時 TRUE
   順番：取り上げる品を持っているか → 渡す品を受け取れるか → 両方よければ取り上げて渡す。
   どちらかがダメなら何も変えない（持っていない時は知らせも出さない。受け取れない時は今までと同じ知らせ）。
   pvLater があれば、白文字の知らせは送らずに溜める（呼んだ側が窓を送った後に QuestNoticeFlush で送る） */
BOOL QuestTalkExchange(PCHARINFO pChar, WORD wTake, WORD wGive, QUESTNOTICEVEC *pvLater)
{
	int nKind, nIdx = 0;
	char szTmp[256];

	if (wTake == 0) {
		return (wGive == 0) || QuestGiveItemNotice (pChar, wGive, pvLater);
	}
	nKind = QuestTakeFind (pChar, wTake, &nIdx);
	if (nKind == QUESTTAKE_NONE) {
		return FALSE;
	}
	if ((wGive != 0) && !QuestCanGiveAfterTake (pChar, wGive, nKind)) {
		QuestNoticeSend (pChar, pvLater, "持ち物がいっぱいで受け取れませんでした。");
		return FALSE;
	}
	if (nKind == QUESTTAKE_BAG) {
		DelCharItem (pChar->dwTblID, (BYTE)nIdx);
	} else {
		DelImportantItem (pChar, wTake);
	}
	SendBagState (pChar);
	snprintf (szTmp, sizeof (szTmp), "%sを渡しました。", ItemName (wTake));
	QuestNoticeSend (pChar, pvLater, szTmp);
	if (wGive != 0) {
		QuestGiveItemNotice (pChar, wGive, pvLater);
	}
	return TRUE;
}

/* 依頼を受ける条件の印に合うか（FLAG_NOBIT=条件なし） */
BOOL QuestFlagOk(PCHARINFO pChar, const QUESTINFO *pQuest)
{
	if ((pQuest->wNeedFlag < MAX_FLAGBIT) && !FlagBitGet (pChar->abyFlag, pQuest->wNeedFlag)) {
		return FALSE;
	}
	if ((pQuest->wNotFlag < MAX_FLAGBIT) && FlagBitGet (pChar->abyFlag, pQuest->wNotFlag)) {
		return FALSE;
	}
	return TRUE;
}

/* 依頼を受ける最低レベルに合うか（0=制限なし。管理者は常に合う） */
BOOL QuestLevelOk(PCHARINFO pChar, const QUESTINFO *pQuest)
{
	if ((pQuest->byLevelMin == 0) || (pChar->byAdminLV != ADMINLV_USER)) {
		return TRUE;
	}
	return (pChar->byLevel >= pQuest->byLevelMin) ? TRUE : FALSE;
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

/* 同じマスに重なっている見えるNPC のうち、pChar が受けた依頼（または仲間の依頼）の分を探す
   管理者にはほかの人の分の光・宝箱も同じマスに重なって見えるので、
   先に見つかったほかの人の分に乗ると「依頼を受けた人のためのもの」と言われて入れなかった */
static PCHARINFO FindOwnQuestNpcAt(PCHARINFO pChar, PCHARINFO pNpc)
{
	std::map<DWORD, QNPCENTRY>::iterator it;

	if (IsQuestNpcFor (pNpc, pChar)) {
		return pNpc;
	}
	for (it = s_mapNpc.begin (); it != s_mapNpc.end (); it ++) {
		if (it->first >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		PCHARINFO pTmp = g_aCharInfo[it->first];
		if ((pTmp == NULL) || (pTmp == pNpc) || (pTmp->dwMapID != pNpc->dwMapID) ||
			(pTmp->dwX != pNpc->dwX) || (pTmp->dwY != pNpc->dwY)) {
			continue;
		}
		if (IsQuestNpcFor (pTmp, pChar)) {
			return pTmp;
		}
	}
	return pNpc;
}

/* 見えるNPC に話しかけた */
void QuestNpcTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	std::map<DWORD, QNPCENTRY>::iterator it;
	PQUESTINFO pQuest;
	PQUESTNPCINFO pDef;
	char szTmp[512];
	BOOL bChg = FALSE;

	if ((pChar == NULL) || (pNpc == NULL)) {
		return;
	}
	pNpc = FindOwnQuestNpcAt (pChar, pNpc);
	it = s_mapNpc.find (pNpc->dwTblID);

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

/* NPC の台詞に、番号の後ろの挨拶（「@g10【受付】@1いらっしゃい…」の【受付】以降）があるか */
static BOOL GuildHasGreeting(PCHARINFO pNpc)
{
	const char *p = pNpc->pszNpcSpeech + 2;
	while ((*p >= '0') && (*p <= '9')) {
		p ++;
	}
	return (*p != 0) ? TRUE : FALSE;
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
	return (QuestPartyConflictMsg (pA, pB) != NULL) ? TRUE : FALSE;
}

/* 仲間になれない理由（なれるなら NULL）
   討伐チャレンジは「パーティで受けた組」で挑むので、別々に受けた人同士は同じ依頼でも仲間になれない
   （1 つのパーティに別々に受けた人がいると、入る組と残る組に分かれてしまう） */
const char *QuestPartyConflictMsg(PCHARINFO pA, PCHARINFO pB)
{
	WORD wA = GetPartyQuest (pA, NULL);
	WORD wB = GetPartyQuest (pB, NULL);
	PQUESTINFO pQuest;

	if ((wA == 0) || (wB == 0)) {
		return NULL;
	}
	if (wA != wB) {
		return "受けている依頼が違うので仲間になれません";
	}
	pQuest = FindQuest (wA);
	if (pQuest && (pQuest->byChallenge != QUESTCHALLENGE_NONE)) {
		return "討伐チャレンジを別々に受けているので仲間になれません（どちらかが取り消すか、パーティを組んでから受けてください）";
	}
	return NULL;
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
static void MakeRewardText(PCHARINFO pChar, PQUESTINFO pQuest, char *pszOut, size_t nSize)
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
	} else if ((pQuest->byChallenge != QUESTCHALLENGE_NONE) || (pQuest->byOnce && QuestIsRewarded (pChar, pQuest->wID))) {
		/* 討伐チャレンジは毎回。一度きりの依頼は、思い出の本でやり直す時（もうもらっている時）に出す */
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
	int nHideLv = 0;												/* 最低レベルに足りなくて出さなかった依頼の数 */

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
		/* 受ける条件の印に合わないものは出さない（受けている人には出す） */
		if ((pChar->wQuestID != pQuest->wID) && !QuestFlagOk (pChar, pQuest)) {
			continue;
		}
		/* 最低レベルに足りないものも出さない（受けている人・管理者には出す）。出さなかった数を覚えておく */
		if ((pChar->wQuestID != pQuest->wID) && !QuestLevelOk (pChar, pQuest)) {
			nHideLv ++;
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
		/* レベルで絞って 1 件も出ない時だけ、窓口の一言を添える（NPC に自前の台詞があればそちらが先なので添えない） */
		if ((nHideLv > 0) && !GuildHasGreeting (pNpc)) {
			CopyStr (Para.szMsg, sizeof (Para.szMsg), "いまのレベルに合う依頼はありません。");
		}
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
		/* レベルで依頼を絞った人にだけ、最初の 1 件に窓口の一言を添える（NPC に自前の台詞があればそちらが先なので添えない） */
		if ((i == 0) && (nHideLv > 0) && !GuildHasGreeting (pNpc)) {
			CopyStr (Para.szMsg, sizeof (Para.szMsg), "今のレベルに合った依頼は、こんな感じです。");
		} else {
			Para.szMsg[0] = 0;
		}
		CopyStr (Para.szName, sizeof (Para.szName), pQuest->szName);
		CopyStr (Para.szExplain, sizeof (Para.szExplain), pQuest->szExplain);
		Para.byIntro = pQuest->szIntro[0] ? 1 : 0;	/* 導入のお話がある（本文は決定を押した時に INTROREQ で送る） */
		CopyStr (Para.szGoal, sizeof (Para.szGoal), (pChar->wQuestID == pQuest->wID) ? GetGoal (pQuest, pChar->byQuestStep) : "");
		MakeRewardText (pChar, pQuest, Para.szReward, sizeof (Para.szReward));
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

/* 導入のお話を送る（本文は構造体の後ろに終端付きで続ける） */
static void SendIntro(PCHARINFO pChar, PQUESTINFO pQuest)
{
	std::vector<BYTE> aBuf (sizeof (CMDPARA_QUEST) + MAX_QUESTINTRO, 0);
	PCMDPARA_QUEST pPara = (PCMDPARA_QUEST)&aBuf[0];
	size_t nLen = strnlen (pQuest->szIntro, MAX_QUESTINTRO - 1);

	pPara->byCmd = CMD_QUEST;
	pPara->byType = QUESTCMD_INTRO;
	pPara->wQuestID = pQuest->wID;
	pPara->dwTblID = pChar->dwTblID;
	memcpy (&aBuf[sizeof (CMDPARA_QUEST)], pQuest->szIntro, nLen);
	svrSock.SendTo (pChar->pID, &aBuf[0], (DWORD)(sizeof (CMDPARA_QUEST) + nLen + 1), SENDTIMEOUT);
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
	if (!QuestFlagOk (pChar, pQuest)) {
		SendResult (pChar, QUESTRESULT_NG, 0, "この依頼は、いまは受けられません。");
		return;
	}
	if (!QuestLevelOk (pChar, pQuest)) {
		char szLv[96];
		snprintf (szLv, sizeof (szLv), "この依頼を受けるには、レベル%uが必要です。", (unsigned)pQuest->byLevelMin);
		SendResult (pChar, QUESTRESULT_NG, 0, szLv);
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
		/* 仲間の中に受ける条件の印に合わない人がいたら、みんな受けられない */
		for (i = 0; i < aMember.size (); i ++) {
			if (!QuestFlagOk (aMember[i], pQuest)) {
				snprintf (szTmp, sizeof (szTmp), "%sさんは、この依頼をいまは受けられません。", aMember[i]->szName);
				SendResult (pChar, QUESTRESULT_NG, 0, szTmp);
				return;
			}
			if (!QuestLevelOk (aMember[i], pQuest)) {
				snprintf (szTmp, sizeof (szTmp), "%sさんは、レベルが足りないので、この依頼を受けられません。", aMember[i]->szName);
				SendResult (pChar, QUESTRESULT_NG, 0, szTmp);
				return;
			}
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

/* パーティを抜けた・解散した時（dwPartyID を 0 にした後に呼ぶ）。
   討伐チャレンジはパーティで受けるものなので、まだ倒していない討伐チャレンジの依頼は取り消す
   （倒し終えて報告を待つだけの人は、そのまま報告できる）。挑戦中の人は中断の処理で取り消す */
static int s_nPartyLeaveHold = 0;								/* 0 より大きい間は取り消さない（切断でパーティから外れる時） */
void QuestPartyLeaveHold(BOOL bHold)
{
	if (bHold) {
		s_nPartyLeaveHold ++;
	} else if (s_nPartyLeaveHold > 0) {
		s_nPartyLeaveHold --;
	}
}

void QuestOnPartyLeave(PCHARINFO pChar)
{
	DgReceptionForget (pChar);
	PQUESTINFO pQuest;

	/* 切断した人・それで解散した仲間は取り消さない（つなぎ直したらまた組めるように） */
	if (s_nPartyLeaveHold > 0) {
		return;
	}
	if ((pChar == NULL) || (pChar->wQuestID == 0) || QuestInChallenge (pChar)) {
		return;
	}
	pQuest = FindQuest (pChar->wQuestID);
	if ((pQuest == NULL) || (pQuest->byChallenge == QUESTCHALLENGE_NONE) || QuestFlagCheck (pChar, QUEST_FLG_CLEARED)) {
		return;
	}
	if (pQuest->wQuestItem) {
		while (TakeItem (pChar, pQuest->wQuestItem)) {
		}
		SendBagState (pChar);
	}
	ClearQuest (pChar);
	SendCmdSYSMSG (pChar->pID, "パーティを抜けたので、討伐チャレンジの依頼は取り消しになりました。挑むときは、またパーティで受けてください。", SPEECHCMD_BATTLE1);
	QuestSendState (pChar);
}

/* 渡す（依頼の品を回収して、報酬を渡す） */
static void DoHandIn(PCHARINFO pChar, PCHARINFO pNpc)
{
	PQUESTINFO pQuest = FindQuest (pChar->wQuestID);
	WORD awItemBak[MAX_MYITEM], awEquBak[10], awImpBak[MAX_IMPORTANT];
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
	memcpy (awImpBak, pChar->waImportant, sizeof (awImpBak));
	dwLeaveIDBak = pChar->dwLeaveID;
	ZeroMemory (&LeaveBak, sizeof (LeaveBak));
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		pLeave = g_aLeaveInfo[pChar->dwLeaveID];
		LeaveBak = *pLeave;
	}

	/* 一度きりの依頼・討伐チャレンジの報酬は初めてクリアした時だけ（思い出の本でやり直した 2 回目からは、
	   クリア済みの印が消えていても「もらい済みの印」で分かる）。何度でも受けられる依頼は毎回報酬 */
	BOOL bRewarded = QuestIsRewarded (pChar, pQuest->wID);
	BOOL bReward = !((pQuest->byOnce || (pQuest->byChallenge != QUESTCHALLENGE_NONE)) && bRewarded);
	/* 完了した時の印・数値の動きは、ふつうの依頼なら 2 回目も必ず動かす（物語が進むように）。
	   討伐チャレンジの 2 回目は今まで通り動かさない */
	BOOL bFlagMove = (pQuest->byChallenge == QUESTCHALLENGE_NONE) ? TRUE : bReward;

	if (pQuest->wQuestItem) {
		TakeItem (pChar, pQuest->wQuestItem);
	}
	if (bReward && pQuest->wRewardItem && !GiveItem (pChar, pQuest->wRewardItem, &bToLeave)) {
		memcpy (pChar->waItem, awItemBak, sizeof (awItemBak));
		memcpy (pChar->waEqu, awEquBak, sizeof (awEquBak));
		memcpy (pChar->waImportant, awImpBak, sizeof (awImpBak));
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
	/* 報酬を渡したら、もうもらっている印を立てる（思い出の本でやり直しても消えない） */
	if (bReward && (pQuest->wID < MAX_QUEST)) {
		pChar->abyQuestRewarded[pQuest->wID / 8] |= (BYTE)(1 << (pQuest->wID % 8));
	}
	/* 完了した時に立てる印・数値の動き（討伐チャレンジの 2 回目からは記録だけなので動かさない） */
	if (bFlagMove) {
		BYTE abyFlagBak[sizeof (pChar->abyFlag)];
		CopyMemory (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak));
		FlagApplyQuestDone (pChar, pQuest);
		/* 印が動いたら、印で見える・消えるNPCを見え直す（依頼で助けた人が、助けた後に町にいる、など） */
		if (memcmp (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak)) != 0) {
			CharViewRefresh (pChar);
		}
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

/* ========================================================================= */
/* 思い出の本（語り部。台詞が "@r"）										 */
/* ========================================================================= */

/* 台詞が "@r" で始まるか（語り部。思い出の本の窓口） */
BOOL StoryIsNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'r');
}

/* シナリオを番号から探す（止めているものも探す） */
static PSTORYINFO FindStory(WORD wID)
{
	int i;
	if (wID == 0) {
		return NULL;
	}
	for (i = 0; i < g_aStoryInfo.GetSize (); i ++) {
		if (g_aStoryInfo[i]->wID == wID) {
			return g_aStoryInfo[i];
		}
	}
	return NULL;
}

/* 本に出るシナリオか（出す状態で、クリアした印が立っている） */
static BOOL StoryIsShown(PCHARINFO pChar, const STORYINFO *pStory)
{
	return (pStory->byState == STORYSTATE_ON) && (pStory->wClearFlag < MAX_FLAGBIT) && FlagBitGet (pChar->abyFlag, pStory->wClearFlag);
}

/* 一覧の並べ替え */
static bool StoryLess(PSTORYINFO pA, PSTORYINFO pB)
{
	if (pA->wSort != pB->wSort) {
		return pA->wSort < pB->wSort;
	}
	return pA->wID < pB->wID;
}

/* 「いまの目的」の行を選ぶ：aGoal を上から見て、szGoal が空でなく、条件（会話データの行と同じ判定）に全部合う最初の行。無ければ -1 */
int StoryGoalPick(const STORYINFO *pStory, const BYTE *pbyFlag, const BYTE *pbyFlagNum)
{
	int i;

	for (i = 0; i < MAX_STORYGOAL; i ++) {
		const STORYGOAL *pGoal = &pStory->aGoal[i];

		if (pGoal->szGoal[0] == 0) {
			continue;
		}
		if (FlagCondMatch (pGoal->awNeed, pGoal->awNot, pGoal->abyCondNo, pGoal->abyCondCmp, pGoal->abyCondVal, pbyFlag, pbyFlagNum)) {
			return i;
		}
	}
	return -1;
}

/* そのキャラの物語の「いまの目的」を集める（出す状態で、クリアの印が立っていないシナリオのうち、合う行があるもの。wSort 順）。
   戻り値は件数（nMax まで） */
static int FillStoryGoals(PCHARINFO pChar, STORYGOALENTRY *paOut, int nMax)
{
	std::vector<PSTORYINFO> aList;
	size_t i;
	int nCount = 0;

	for (i = 0; i < (size_t)g_aStoryInfo.GetSize (); i ++) {
		PSTORYINFO pStory = g_aStoryInfo[i];

		/* クリアの印は見ない（第1話の最後の行き先のように、クリアした後も出したい目的があるため。
		   クリアで消したい時は、目的の行の awNot にクリアの印を入れる） */
		if (pStory->byState != STORYSTATE_ON) {
			continue;
		}
		aList.push_back (pStory);
	}
	std::stable_sort (aList.begin (), aList.end (), StoryLess);
	for (i = 0; (i < aList.size ()) && (nCount < nMax); i ++) {
		int nGoal = StoryGoalPick (aList[i], pChar->abyFlag, pChar->abyFlagNum);

		if (nGoal < 0) {
			continue;
		}
		ZeroMemory (&paOut[nCount], sizeof (STORYGOALENTRY));
		CopyStr (paOut[nCount].szName, sizeof (paOut[nCount].szName), aList[i]->szName);
		CopyStr (paOut[nCount].szGoal, sizeof (paOut[nCount].szGoal), aList[i]->aGoal[nGoal].szGoal);
		nCount ++;
	}
	return nCount;
}

/* wID のシナリオと、wPrevStory をたどってそれにつながる後のシナリオ（子・孫…）を集める。
   先頭は wID。つながりがループしていても、同じシナリオは 1 回しか入れない */
static void StoryCollect(WORD wID, std::vector<PSTORYINFO> &aOut)
{
	size_t nPos;
	int i;
	PSTORYINFO pFirst = FindStory (wID);

	aOut.clear ();
	if (pFirst == NULL) {
		return;
	}
	aOut.push_back (pFirst);
	for (nPos = 0; nPos < aOut.size (); nPos ++) {
		for (i = 0; i < g_aStoryInfo.GetSize (); i ++) {
			PSTORYINFO pTmp = g_aStoryInfo[i];
			if ((pTmp->wPrevStory != aOut[nPos]->wID) || (pTmp->wPrevStory == 0)) {
				continue;
			}
			if (std::find (aOut.begin (), aOut.end (), pTmp) == aOut.end ()) {
				aOut.push_back (pTmp);
			}
		}
	}
}

/* aTree（先頭は選んだシナリオ）の、続きのお話のうち本に出ているものを『名前』の並びにする（nMaxByte に収まる分だけ） */
static std::string StoryNextNames(PCHARINFO pChar, const std::vector<PSTORYINFO> &aTree, size_t nMaxByte)
{
	std::string strNext;
	size_t i;

	for (i = 1; i < aTree.size (); i ++) {
		if (!StoryIsShown (pChar, aTree[i])) {
			continue;
		}
		if (strNext.size () + strlen (aTree[i]->szName) + 6 >= nMaxByte) {
			break;
		}
		strNext += "『";
		strNext += aTree[i]->szName;
		strNext += "』";
	}
	return strNext;
}

/* 近くに、語り部がいるか */
static PCHARINFO FindNearStoryNpc(PCHARINFO pChar, DWORD dwNpcTblID)
{
	PCHARINFO pNpc;

	if (dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pNpc = g_aCharInfo[dwNpcTblID];
	if (!StoryIsNpc (pNpc) || (pNpc->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pNpc)) {
		return NULL;
	}
	if ((abs ((int)pChar->dwX - (int)pNpc->dwX) > 2) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > 2)) {
		return NULL;
	}
	return pNpc;
}

/* 思い出の本の一覧を送る（語り部に話した時）。本に出るシナリオだけ、並び順で。
   szGoal には、やり直すといっしょに最初に戻る「続きのお話」（本に出ているもの）の名前を入れる */
void StoryTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	std::vector<PSTORYINFO> aList, aTree;
	CMDPARA_QUEST Para;
	int i;

	for (i = 0; i < g_aStoryInfo.GetSize (); i ++) {
		PSTORYINFO pStory = g_aStoryInfo[i];
		if (!StoryIsShown (pChar, pStory)) {
			continue;
		}
		if ((int)aList.size () >= MAX_QUESTLIST) {
			break;
		}
		aList.push_back (pStory);
	}
	std::sort (aList.begin (), aList.end (), StoryLess);

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_QUEST;
	Para.byType = QUESTCMD_STORYITEM;
	Para.dwTblID = pChar->dwTblID;
	Para.dwNpcTblID = pNpc->dwTblID;
	Para.byCount = (BYTE)aList.size ();
	if (aList.empty ()) {
		svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
		return;
	}
	for (i = 0; i < (int)aList.size (); i ++) {
		PSTORYINFO pStory = aList[i];
		Para.byIndex = (BYTE)i;
		Para.wQuestID = pStory->wID;
		CopyStr (Para.szName, sizeof (Para.szName), pStory->szName);
		CopyStr (Para.szExplain, sizeof (Para.szExplain), pStory->szExplain);
		/* いっしょに最初に戻る続きのお話（まだ本に出ていないお話は教えない） */
		StoryCollect (pStory->wID, aTree);
		SboTextCopy (Para.szGoal, sizeof (Para.szGoal), StoryNextNames (pChar, aTree, sizeof (Para.szGoal) - 1).c_str ());
		svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
	}
}

/* やり直しで取り上げる品を持ち物から消す（大事な物の欄の全部と、手持ちの装備していない分）。消した個数を返す */
static int StoryTakeItems(PCHARINFO pChar, WORD wItem)
{
	int i, j, nRet = 0;

	if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
		return 0;
	}
	while (DelImportantItem (pChar, wItem)) {
		nRet ++;
	}
	for (i = 0; i < MAX_MYITEM; ) {
		BOOL bEqu = FALSE;

		if (pChar->waItem[i] != wItem) {
			i ++;
			continue;
		}
		for (j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == (WORD)i) {
				bEqu = TRUE;
			}
		}
		if (bEqu) {
			i ++;			/* 装備中は触らない */
			continue;
		}
		DelCharItem (pChar->dwTblID, (BYTE)i);		/* 後ろがつまるので i は進めない */
		nRet ++;
	}
	return nRet;
}

/* 思い出をやり直す（選んだシナリオと、それにつながる後のシナリオの印・数値・依頼のクリア済みの印を戻す。
   開けた宝箱の記録は、各シナリオの awTakeChest に指定した宝箱だけ戻す（指定の無い宝箱は戻さない＝宝は一度きり）） */
static void DoStoryReset(PCHARINFO pChar, WORD wStoryID)
{
	std::vector<PSTORYINFO> aTree;
	PSTORYINFO pStory = FindStory (wStoryID);
	BYTE abyFlagBak[sizeof (pChar->abyFlag)];
	char szMsg[MAX_QUESTMSG];
	std::string strNext;
	size_t i;
	int j, k, nTake = 0, nChest = 0;

	if ((pStory == NULL) || !StoryIsShown (pChar, pStory)) {
		SendResult (pChar, QUESTRESULT_NG, 0, "その思い出は、いまは開けません。");
		return;
	}
	StoryCollect (wStoryID, aTree);

	/* 受けている依頼が戻す依頼に入っている間は断る（受けたまま印だけ戻ると、話がつながらなくなる） */
	if (pChar->wQuestID != 0) {
		for (i = 0; i < aTree.size (); i ++) {
			for (j = 0; j < MAX_STORYQUEST; j ++) {
				if (aTree[i]->awQuest[j] == pChar->wQuestID) {
					SendResult (pChar, QUESTRESULT_NG, wStoryID, "いま受けている依頼を終えるか取り消してから、思い出をやり直してください。");
					return;
				}
			}
		}
	}

	/* 巻き込む続きのお話の名前（印を消す前の、本に出ているもの） */
	strNext = StoryNextNames (pChar, aTree, MAX_QUESTMSG / 2);

	CopyMemory (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak));
	for (i = 0; i < aTree.size (); i ++) {
		const STORYINFO *p = aTree[i];

		/* 印の範囲（先頭〜最後。どちらも印の番号で、先頭が最後より大きい範囲は使わない） */
		for (j = 0; j < MAX_STORYFLAGRANGE; j ++) {
			if ((p->awFlagFrom[j] >= MAX_FLAGBIT) || (p->awFlagTo[j] >= MAX_FLAGBIT) || (p->awFlagFrom[j] > p->awFlagTo[j])) {
				continue;
			}
			for (k = p->awFlagFrom[j]; k <= p->awFlagTo[j]; k ++) {
				FlagBitSet (pChar->abyFlag, k, FALSE);
			}
		}
		/* 数値は 0 に */
		for (j = 0; j < MAX_STORYNUM; j ++) {
			if (p->abyNum[j] < MAX_FLAGNUM) {
				FlagNumSet (pChar->abyFlagNum, p->abyNum[j], 0);
			}
		}
		/* 依頼のクリア済みの印を消す（報酬をもらい済みの印は残す＝報酬は初回だけ） */
		for (j = 0; j < MAX_STORYQUEST; j ++) {
			WORD wQuest = p->awQuest[j];
			if ((wQuest >= 1) && (wQuest < MAX_QUEST)) {
				pChar->abyQuestDone[wQuest / 8] &= (BYTE)~(1 << (wQuest % 8));
			}
		}
		/* 取り上げる持ち物 */
		for (j = 0; j < MAX_STORYTAKEITEM; j ++) {
			nTake += StoryTakeItems (pChar, p->awTakeItem[j]);
		}
		/* 開けた記録を戻す宝箱（開けていた分だけ数える。「無し」は FLAG_NOBIT） */
		for (j = 0; j < MAX_STORYCHEST; j ++) {
			WORD wChest = p->awTakeChest[j];
			if ((wChest < MAX_CHESTNO) && ChestOpenedGet (pChar, wChest)) {
				ChestOpenedSet (pChar, wChest, FALSE);
				nChest ++;
			}
		}
	}
	/* やり直した回数（選んだシナリオだけ。消す数値と重なっても最後に +1） */
	if (pStory->byCountNum < MAX_FLAGNUM) {
		FlagNumAdd (pChar->abyFlagNum, pStory->byCountNum, 1);
	}
	/* 印が動いたら、印で見える・消えるNPCを見え直す */
	/* 宝箱の記録が戻った時も、本人の「印で変わるマス」（宝箱の見た目）を送り直す（CharViewRefresh が差し替えも送る） */
	if ((memcmp (abyFlagBak, pChar->abyFlag, sizeof (abyFlagBak)) != 0) || (nChest > 0)) {
		CharViewRefresh (pChar);
	}

	snprintf (szMsg, sizeof (szMsg), "「%s」を最初からやり直せるようになりました。", pStory->szName);
	if (!strNext.empty ()) {
		snprintf (szMsg + strlen (szMsg), sizeof (szMsg) - strlen (szMsg), "%sもいっしょに最初に戻りました。", strNext.c_str ());
	}
	if (nTake > 0) {
		snprintf (szMsg + strlen (szMsg), sizeof (szMsg) - strlen (szMsg), "（%d個の大事な物を手放しました）", nTake);
		SendBagState (pChar);		/* 持ち物の更新をクライアントへ */
	}
	if (nChest > 0) {
		snprintf (szMsg + strlen (szMsg), sizeof (szMsg) - strlen (szMsg), "（宝箱を%d個戻しました）", nChest);
	}
	SendResult (pChar, QUESTRESULT_STORY, wStoryID, szMsg);
	/* 印・数値が戻ったので、物語の「いまの目的」も送り直す */
	QuestSendStateSelf (pChar);
}

/* CMD_QUEST の受信 */
void QuestRecv(PCHARINFO pChar, PCMDPARA_QUEST pPara)
{
	PCHARINFO pNpc;
	PQUESTINFO pQuest;

	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}
	if (QuestInChallenge (pChar) && (pPara->byType != QUESTCMD_LISTREQ) && (pPara->byType != QUESTCMD_STORYREQ)) {
		SendResult (pChar, QUESTRESULT_NG, 0, QuestInDungeon (pChar) ? DungeonBlockedText (pChar).c_str () : "討伐チャレンジ中はできません。");
		return;
	}
	switch (pPara->byType) {
	case QUESTCMD_LISTREQ:
		pNpc = FindNearGuild (pChar, -1, pPara->dwNpcTblID);
		if (pNpc) {
			QuestGuildTalk (pChar, pNpc);
		}
		break;

	case QUESTCMD_STORYREQ:
		pNpc = FindNearStoryNpc (pChar, pPara->dwNpcTblID);
		if (pNpc) {
			if (QuestInChallenge (pChar)) {
				SendResult (pChar, QUESTRESULT_NG, 0, QuestInDungeon (pChar) ? DungeonBlockedText (pChar).c_str () : "討伐チャレンジ中はできません。");
			} else {
				StoryTalk (pChar, pNpc);
			}
		}
		break;

	case QUESTCMD_STORYRESET:
		if (FindNearStoryNpc (pChar, pPara->dwNpcTblID) == NULL) {
			SendResult (pChar, QUESTRESULT_NG, 0, "語り部の近くで頼んでください。");
			break;
		}
		DoStoryReset (pChar, pPara->wQuestID);
		break;

	case QUESTCMD_INTROREQ:
		/* 導入のお話は読み物なので、受ける条件は見ない（受ける時にもう一度見る）。ギルドの人の近くで、お話がある依頼だけ */
		pNpc = FindNearGuild (pChar, -1, pPara->dwNpcTblID);
		pQuest = FindQuest (pPara->wQuestID);
		if ((pNpc != NULL) && (pQuest != NULL) && pQuest->szIntro[0]) {
			SendIntro (pChar, pQuest);
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
	LOANA (abyMagicKey);	LOANA (awKeyItem);
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
	/* ショートカットは空から（元の登録は控えにあり、出る時に戻す） */
	ZeroMemory (p->abyMagicKey, sizeof (p->abyMagicKey));
	ZeroMemory (p->awKeyItem, sizeof (p->awKeyItem));
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
	/* 入る前（出る前）に出した、ダメージを出し続ける魔法などのキャラを消す（行き先まで持ち込まない）。
	   DeleteChildChar はプレイヤーの子を探す表が違うので、ここで全部のキャラから探す */
	{
		int i, nCount = g_aCharInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PCHARINFO pTmp = g_aCharInfo[i];
			if ((pTmp->dwParentTblID != p->dwTblID) || (pTmp == p)) {
				continue;
			}
			if ((pTmp->byNpcType == NPCTYPE_USEMAGIC) || (pTmp->byNpcType == NPCTYPE_ATACK) ||
				(pTmp->byNpcType == NPCTYPE_USEMAGICPARENT)) {
				DeleteChar (pTmp->dwTblID);
			}
		}
	}
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
		SendMagicKeySetting (p);		/* 入れ替えたショートカットを知らせる */
	}
	/* 攻撃アップ・守りアップの表示も消す（入る前にかけた効果を持ち込まない・持ち出さない） */
	BuffSendState (p);
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
	CHALLENGELEAVE_BUILD,			/* 番外編ダンジョンが作り直された */
	CHALLENGELEAVE_RETIRE,			/* 番外編ダンジョン：中の入口・「敵を全部倒す」条件のゴールのワープから途中で帰った（記録なし） */
	CHALLENGELEAVE_ALLDEAD,		/* 番外編の全滅 */
};

/* 挑戦から外して元の状態に戻す */
static void ChallengeLeave(PCHARINFO p, int nReason)
{
	std::map<DWORD, LOANBAK>::iterator it = s_mapLoan.find (p->dwTblID);
	LOANBAK Bak;
	BOOL bMove, bDungeon = FALSE, bDefense = FALSE;
	int nDungeonID = -1;

	if (it == s_mapLoan.end ()) {
		return;
	}
	Bak = it->second;
	s_mapLoan.erase (it);
	/* 出た後は「抜けますか？」の確認は無効（切断では送らない） */
	DgLeaveAskClose (p, nReason != CHALLENGELEAVE_LOGOUT);
	{
		std::map<DWORD, CHALLENGE>::iterator itC = s_mapChallenge.find (Bak.dwSession);
		if (itC != s_mapChallenge.end ()) {
			std::vector<DWORD> &aM = itC->second.aMember;
			aM.erase (std::remove (aM.begin (), aM.end (), p->dwTblID), aM.end ());
			bDungeon = (itC->second.byKind == CHALLENGEKIND_DUNGEON);
			nDungeonID = itC->second.byDungeonID;
			bDefense = itC->second.bDefense;
		}
	}
	/* 拠点防衛：外へ出る人の画面から帯を消す（切断では送れない。つなぎ直した画面は新しいので帯はない） */
	if (bDefense && (nReason != CHALLENGELEAVE_LOGOUT) && IsPlayerOnline (p)) {
		CMDPARA_DGDEFSTATUS Para;

		ZeroMemory (&Para, sizeof (Para));
		SendCmdDGDEFSTATUS (p->pID, &Para);
	}
	/* 拠点防衛以外の番外編：外へ出る人の画面から状況パネルを消す（切断では送れない） */
	if (bDungeon && !bDefense && (nReason != CHALLENGELEAVE_LOGOUT) && IsPlayerOnline (p)) {
		CMDPARA_DGSTATUS Para;

		ZeroMemory (&Para, sizeof (Para));
		SendCmdDGSTATUS (p->pID, &Para);
	}
	/* 番外編：中で手に入れた物は中だけの物。どの出方でも持ち帰らず、元の持ち物に戻すだけ（2026-11-19 方針） */

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
	if (bDungeon) {
		DgLeaveMsg (p, nReason, nDungeonID);
	}
	switch (bDungeon ? -1 : nReason) {
	case CHALLENGELEAVE_DEAD:
		SendCmdSYSMSG (p->pID, "力尽きてしまった…。挑戦は中断して、元の姿に戻りました。依頼は受けたままなので、また挑めます。", SPEECHCMD_BATTLE1);
		break;
	case CHALLENGELEAVE_PARTY:
		SendCmdSYSMSG (p->pID, "パーティを抜けたので、挑戦は中断して元の姿に戻りました。", SPEECHCMD_BATTLE1);
		/* 討伐チャレンジはパーティで受けるものなので、抜けたら依頼も取り消し */
		QuestOnPartyLeave (p);
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
	pNew->byGhost		= t->byGhost;
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
	/* 闘技場は観客から見えるので、同じ依頼は一度に 1 組だけ（重なって見えないように） */
	if (pQuest->byChallenge == QUESTCHALLENGE_ARENA) {
		std::map<DWORD, CHALLENGE>::iterator itA;
		for (itA = s_mapChallenge.begin (); itA != s_mapChallenge.end (); itA ++) {
			if (itA->second.wQuestID == pQuest->wID) {
				SendCmdSYSMSG (pChar->pID, "いま別の組が闘技場で戦っています。終わるまで観戦して待ちましょう。", SPEECHCMD_BATTLE1);
				return;
			}
		}
	}
	/* 組ごとに別の空間で戦うので、同じ依頼を何組でも同時に遊べる。
	   ただし同じパーティの仲間がまだ中で戦っている間は、倒れて出てきた人は入り直せない */
	{
		std::vector<PCHARINFO> aParty;
		GetPartyMembers (pChar, aParty);
		for (i = 0; i < (int)aParty.size (); i ++) {
			if ((aParty[i] != pChar) && QuestInChallenge (aParty[i]) && (aParty[i]->wQuestID == pQuest->wID)) {
				SendCmdSYSMSG (pChar->pID, "仲間がまだ討伐チャレンジ中です。終わるまで待ってから、もう一度挑んでください。", SPEECHCMD_BATTLE1);
				return;
			}
		}
	}
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
	Ch.bArena = (pQuest->byChallenge == QUESTCHALLENGE_ARENA);
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
			if ((wItem == 0) || !ItemIDValid (wItem)) {
				continue;
			}
			snprintf (szEqu + strlen (szEqu), sizeof (szEqu) - strlen (szEqu), "%s%s", szEqu[0] ? "、" : "持ち物：", ItemNameOf (wItem));
		}
		for (i = 0; i < (int)aMember.size (); i ++) {
			SendCmdSYSMSG (aMember[i]->pID, szTmp, SPEECHCMD_BATTLE1);
			if (szEqu[0]) {
				SendCmdSYSMSG (aMember[i]->pID, szEqu, SPEECHCMD_BATTLE1);
			}
			SendCmdSYSMSG (aMember[i]->pID, "倒れる・パーティを抜ける・切断すると、その人は中断して元の姿に戻ります。", SPEECHCMD_BATTLE1);
		}
	}

	/* 戻る場所は、仲間も全員、光に入った人の入る前の場所にする
	   （各自の場所に戻すと、光から離れていた仲間がばらばらの場所へ戻ってしまう） */
	DWORD dwBackMapID = pChar->dwMapID, dwBackX = pChar->dwX, dwBackY = pChar->dwY;
	BYTE byBackDirection = pChar->byDirection;

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
		Bak.dwMapID = dwBackMapID;
		Bak.dwX = dwBackX;
		Bak.dwY = dwBackY;
		Bak.byDirection = byBackDirection;
		s_mapLoan[pTmp->dwTblID] = Bak;
		pCh->aMember.push_back (pTmp->dwTblID);

		LoanApply (pTmp, t);
		LoanSendAll (pTmp);
		AdvanceQuest (pTmp, pDef);
		if (!Ch.bArena && ((DWORD)nDestMapNo == pTmp->dwMapID)) {
			/* 同じマップの中のワープ（座標変更）ではマップ情報が届かないので、
			   クライアントにふつうの世界のキャラが残り、その場に止まったまま見えてしまう。
			   マップを移る時と同じ流れで見え方を作り直す
			   （闘技場はふつうの世界のキャラも見えたまま動きが届くので、今までどおりのワープ） */
			SendCmdPLAYSOUND (pTmp->pID, SOUND_MOVE);
			SendCmdFADEINOUT (pTmp->pID, 0);
			SubProcMapMove (pTmp, (DWORD)nDestMapNo, pDef->byDestX, pDef->byDestY);
			/* 入る時のワープは「着いた」印にする（座標変更イベントで移った時と同じ） */
			QuestChallengeOnWarp (pTmp);
		} else {
			DoPortal (pTmp, pDef);
		}
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

	/* 番外編ダンジョンの姿を選んでいる最中の切断 */
	DgSelOnDec (pChar);
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

		/* 番外編ダンジョンの組は別（DgProc） */
		if (itC->second.byKind == CHALLENGEKIND_DUNGEON) {
			continue;
		}
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
	/* 番外編ダンジョン（姿を選ぶ締め切り・中の人が出た判定・敵の補充） */
	DgProc (dwNow);
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
	/* 拠点防衛：倒れても中断させない（復活は拠点の横で時間がたつと自動で起きる。押されても何もしない） */
	{
		CHALLENGE *pRun = DgRunOf (pChar);
		if (pRun && pRun->bDefense) {
			return TRUE;
		}
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
/* 番外編ダンジョン（気まぐれダンジョンのミニゲーム）						 */
/* ========================================================================= */
/*
 * サブゲーム「ダンジョン」にテンプレート（SUBGAME_DUNGEONINFO::byTmplCount）を決めると、番外編のミニゲームになる。
 * 受付で近くの仲間全員に姿の一覧を出し、全員が選び終えるか 30 秒たつと、選んだ人を受付済みにする。
 * 受付済みの誰かが外の入口に乗ると、近くの組全員で新しい空間へ一緒に入る。
 * 中は討伐チャレンジと同じ貸し出しの姿（s_mapLoan。経験値・お金は戻る・預かり所などは使えない）。
 * ダンジョンの形（マップ）は 1 つを共有し、組ごとに空間番号を割り当てる（s_mapChallenge の byKind が 1 の組）。
 * 敵（NPC発生の出し分）・扉の開閉は空間ごと。一緒に入った全員が出たら、その空間を空きにする。
 * 中で手に入れた物は中だけの物。どの出方でも持ち帰らず、元の持ち物に戻す（持ち帰り袋は 2026-11-19 にやめた）。
 * DgBagCollect は「中で増えた物」を数えるだけ（テスト用 DungeonBagCount）。
 */
#define DGSEL_WAIT				30000								/* 姿を選ぶ締め切り（ミリ秒） */

/* 姿を選んでいる組（申し込んだ人＝リーダーのテーブルIDが鍵） */
typedef struct _DGSEL {
	DWORD		dwNpcTblID;										/* 申し込んだ受付 */
	int			nDungeonID;											/* サブゲームの番号 */
	DWORD		dwStart;											/* 申し込んだ時刻 */
	DWORD		dwMapID, dwX, dwY;									/* 申し込んだ場所（仲間との距離の基準） */
	BYTE		byDirection;
	std::vector<DWORD>		aMember;								/* 選ぶ人（テーブルID。リーダーが先頭） */
	std::map<DWORD, int>	mapPick;								/* テーブルID → 選んだ番号（-1=まだ -2=やめた） */
	std::vector<WORD>		aTmpl;									/* 選べるテンプレート番号 */
	std::vector<std::string>	aName;								/* その表示名 */
	DWORD dwPartyID, dwBuildNo;
} DGSEL;

static std::map<DWORD, DGSEL> s_mapDgSel;						/* リーダーのテーブルID → 選んでいる組 */
static std::map<DWORD, DWORD> s_mapDgSelOf;						/* 選んでいる人のテーブルID → リーダーのテーブルID */

/* 選び終えた組。入口に乗るまでは貸し出し・空間の作成を行わない。 */
#define DGPREPARED_WAIT 120000
static std::map<DWORD, DGSEL> s_mapDgPrepared;
static std::map<DWORD, DWORD> s_mapDgPreparedOf;

/* サブゲームの番号からダンジョンの情報（無ければ NULL） */
static PSUBGAME_DUNGEONINFO DgInfo(int nID)
{
	PSUBGAMEINFO pInfo;

	if ((nID < 0) || (nID >= g_aSubaGameInfo.GetSize ())) {
		return NULL;
	}
	pInfo = g_aSubaGameInfo[nID];
	if ((pInfo == NULL) || (pInfo->byType != SUBGAMEID_DUNGEON) || (pInfo->pInfo == NULL)) {
		return NULL;
	}
	return (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
}

/* 番外編のダンジョンか（テーマで作るか、作り直さない固定の形で、テンプレートが 1 つ以上ある） */
static BOOL DgIsGame(PSUBGAME_DUNGEONINFO p)
{
	return (p != NULL) && ((p->dwThemeID != 0) || (p->byRebuildMode == DGREBUILD_FIXED)) && (p->byTmplCount > 0);
}

/* 遊べる状態か（作り終えている。固定の形は範囲がマップに収まっていれば、いつでも遊べる） */
static BOOL DgReady(PSUBGAME_DUNGEONINFO p)
{
	if (p->byRebuildMode == DGREBUILD_FIXED) {
		DWORD dwInX, dwInY;
		if (!((p->dwMapID < (DWORD)g_aMapInfo.GetSize ()) && (g_aMapInfo[p->dwMapID] != NULL) &&
			(p->byX2 > p->byX1) && (p->byY2 > p->byY1) &&
			((int)p->byX2 < g_aMapInfo[p->dwMapID]->nCX) && ((int)p->byY2 < g_aMapInfo[p->dwMapID]->nCY))) {
			return FALSE;
		}
		/* 中の開始位置を設定している時は、それもマップに収まっていること */
		return !DungeonFixedInside (p, &dwInX, &dwInY) || (((int)dwInX < g_aMapInfo[p->dwMapID]->nCX) && ((int)dwInY < g_aMapInfo[p->dwMapID]->nCY));
	}
	return (p->dwGenMapID == p->dwMapID) && (p->byGenX1 == p->byX1) && (p->byGenY1 == p->byY1) &&
		(p->byGenX2 == p->byX2) && (p->byGenY2 == p->byY2) && (p->pwGenParts != NULL) && (p->dwBuildNo > 0) && (p->pMapTmp == NULL) && (p->wGenCX > 0);
}

static BOOL DgInArea(PSUBGAME_DUNGEONINFO p, DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	return (dwMapID == p->dwMapID) && (dwX >= (DWORD)p->byX1) && (dwX <= (DWORD)p->byX2) && (dwY >= (DWORD)p->byY1) && (dwY <= (DWORD)p->byY2);
}

/* 生成時と入場時で、全敵撃破として扱う条件をそろえる。発生済みの数は問わない。 */
BOOL DungeonUsesKillAll(PSUBGAME_DUNGEONINFO pDungeon)
{
	if (!pDungeon || pDungeon->byClearMode != DGCLEAR_KILLALL) return FALSE;
	for (int k = 0; k < g_aCharInfo.GetSize (); k ++) {
		PCHARINFO pPut = g_aCharInfo[k];
		if (pPut && pPut->byNpcType == NPCTYPE_PUT && pPut->byPutOnce && pPut->byPutCount > 0 && pPut->dwSpaceID == SPACE_NORMAL &&
			DgInArea (pDungeon, pPut->dwMapID, pPut->dwX, pPut->dwY)) return TRUE;
	}
	return FALSE;
}

/* その場所を範囲に含む番外編のダンジョンの番号（無ければ -1） */
static int DgFindByArea(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	int i;

	for (i = 0; i < g_aSubaGameInfo.GetSize (); i ++) {
		PSUBGAME_DUNGEONINFO p = DgInfo (i);
		if (DgIsGame (p) && DgInArea (p, dwMapID, dwX, dwY)) {
			return i;
		}
	}
	return -1;
}

/* 入口（外の入口のマス）の番号。マスが同じ番外編のダンジョンが無ければ -1 */
static int DgFindByEntrance(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	int i;

	for (i = 0; i < g_aSubaGameInfo.GetSize (); i ++) {
		PSUBGAME_DUNGEONINFO p = DgInfo (i);
		if (DgIsGame (p) && (p->dwMapID == dwMapID) && (dwX == (DWORD)p->byStartX) && (dwY == (DWORD)p->byStartY)) {
			return i;
		}
	}
	return -1;
}

/* 固定の形の中の開始位置（byFixStartX・byFixStartY）を使うか。使う時だけ TRUE で、その位置を返す。
   両方 0・固定の形でない・外の入口と同じマスなら FALSE（外の入口＝開始位置のまま、今まで通り） */
BOOL DungeonFixedInside(PSUBGAME_DUNGEONINFO p, DWORD *pdwX, DWORD *pdwY)
{
	if ((p == NULL) || (p->byRebuildMode != DGREBUILD_FIXED) || (!p->byFixStartX && !p->byFixStartY) ||
		((p->byFixStartX == p->byStartX) && (p->byFixStartY == p->byStartY))) {
		return FALSE;
	}
	if (pdwX) {
		*pdwX = (DWORD)p->byFixStartX;
	}
	if (pdwY) {
		*pdwY = (DWORD)p->byFixStartY;
	}
	return TRUE;
}

/* 固定の形の中の開始位置のマスの番号。外の入口と違うマスを設定している番外編が無ければ -1 */
static int DgFindByFixInside(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	int i;

	for (i = 0; i < g_aSubaGameInfo.GetSize (); i ++) {
		PSUBGAME_DUNGEONINFO p = DgInfo (i);
		DWORD dwInX, dwInY;
		if (DgIsGame (p) && (p->dwMapID == dwMapID) && DungeonFixedInside (p, &dwInX, &dwInY) && (dwX == dwInX) && (dwY == dwInY)) {
			return i;
		}
	}
	return -1;
}

/* 選べるテンプレートの一覧（存在しない番号は外す。名前が空ならテンプレートの名前） */
static void DgMakeList(PSUBGAME_DUNGEONINFO p, std::vector<WORD> &aTmpl, std::vector<std::string> &aName)
{
	int i, nMax = min ((int)p->byTmplCount, (int)MAX_DGTMPL);

	aTmpl.clear ();
	aName.clear ();
	for (i = 0; i < nMax; i ++) {
		WORD wID = p->wTmplID[i];
		char szName[MAX_DGTMPLNAME];

		if (wID >= (WORD)g_aStTmpInfo.GetSize ()) {
			continue;
		}
		CopyStr (szName, sizeof (szName), p->szTmplName[i][0] ? p->szTmplName[i] : g_aStTmpInfo[wID]->szName);
		aTmpl.push_back (wID);
		aName.push_back (szName);
	}
}

/* 番外編の中の組（空間）を探す。無ければ NULL */
static CHALLENGE *DgRunOf(PCHARINFO p)
{
	std::map<DWORD, LOANBAK>::iterator itL;
	std::map<DWORD, CHALLENGE>::iterator itC;

	if (p == NULL) {
		return NULL;
	}
	itL = s_mapLoan.find (p->dwTblID);
	if (itL == s_mapLoan.end ()) {
		return NULL;
	}
	itC = s_mapChallenge.find (itL->second.dwSession);
	if ((itC == s_mapChallenge.end ()) || (itC->second.byKind != CHALLENGEKIND_DUNGEON)) {
		return NULL;
	}
	return &itC->second;
}

/* 組の地図は空間に所属する間だけ有効。ソロは従来どおり通信しない。 */
DWORD DungeonMapSpace(PCHARINFO p)
{
	CHALLENGE *c = DgRunOf (p);
	PSUBGAME_DUNGEONINFO d = c ? DgInfo (c->byDungeonID) : NULL;
	return c && c->bMapShared && d && d->pbyAutoMap && d->dwAutoMapSize && d->dwBuildNo == c->dwBuildNo &&
		p->dwSpaceID == c->dwSpaceID && p->dwMapID == c->dwMapNo ? c->dwSpaceID : 0;
}

static std::vector<BYTE> DgMapPositions(CHALLENGE *c)
{
	std::vector<BYTE> out;
	for (size_t i = 0; i < c->aMember.size (); i ++) {
		PCHARINFO p = g_aCharInfo[c->aMember[i]];
		if (!p || !p->pID || p->dwSpaceID != c->dwSpaceID || p->dwMapID != c->dwMapNo) continue;
		DUNGEONMAPPOS pos;
		ZeroMemory (&pos, sizeof (pos));
		pos.dwTblID = p->dwTblID; pos.wX = (WORD)p->dwX; pos.wY = (WORD)p->dwY;
		out.insert (out.end (), (PBYTE)&pos, (PBYTE)(&pos + 1));
	}
	return out;
}

static void DgMapSend(CHALLENGE *c, PCHARINFO to, const std::vector<WORD> &cells)
{
	if (cells.size () > 512) {
		for (size_t i = 0; i < cells.size (); i += 512) {
			std::vector<WORD> part (cells.begin () + i, cells.begin () + min (i + 512, cells.size ()));
			DgMapSend (c, to, part);
		}
		return;
	}
	CMDPARA_DUNGEONMAPDIFF h;
	ZeroMemory (&h, sizeof (h));
	h.byCmd = CMD_DUNGEONMAP; h.byValid = 2;
	h.dwMapID = c->dwMapNo; h.dwBuildNo = c->dwBuildNo; h.dwSpaceID = c->dwSpaceID;
	h.wCount = (WORD)cells.size ();
	std::vector<BYTE> pos = DgMapPositions (c);
	h.byMember = (BYTE)(pos.size () / sizeof (DUNGEONMAPPOS));
	std::vector<BYTE> buf (sizeof (h) + cells.size () * sizeof (WORD) + pos.size ());
	CopyMemory (&buf[0], &h, sizeof (h));
	if (!cells.empty ()) CopyMemory (&buf[sizeof (h)], &cells[0], cells.size () * sizeof (WORD));
	if (!pos.empty ()) CopyMemory (&buf[sizeof (h) + cells.size () * sizeof (WORD)], &pos[0], pos.size ());
	if (to) {
		svrSock.SendTo (to->pID, &buf[0], (DWORD)buf.size (), SENDTIMEOUT);
	} else {
		for (size_t i = 0; i < c->aMember.size (); i ++) {
			PCHARINFO p = g_aCharInfo[c->aMember[i]];
			if (p && p->pID && DungeonMapSpace (p) == c->dwSpaceID)
				svrSock.SendTo (p->pID, &buf[0], (DWORD)buf.size (), SENDTIMEOUT);
		}
	}
}

/* 地形の再送直後だけ全記録を返す。通常は新しく増えたマスだけ。 */
void DungeonMapSend(PCHARINFO p)
{
	if (!DungeonMapSpace (p)) return;
	CHALLENGE *c = DgRunOf (p);
	std::vector<WORD> cells (c->setMapRecord.begin (), c->setMapRecord.end ());
	DgMapSend (c, p, cells);
}

void DungeonMapRecv(PCHARINFO p, PCMDPARA_DUNGEONMAPDIFF h, DWORD size)
{
	if (!DungeonMapSpace (p)) return;
	CHALLENGE *c = DgRunOf (p);
	PSUBGAME_DUNGEONINFO d = DgInfo (c->byDungeonID);
	if (!d || !d->pbyAutoMap || h->byValid != 2 || h->byMember || h->wCount > 512 ||
		h->dwSpaceID != c->dwSpaceID || h->dwMapID != c->dwMapNo || h->dwBuildNo != c->dwBuildNo ||
		size != sizeof (*h) + h->wCount * sizeof (WORD)) return;
	PCMDPARA_DUNGEONMAP m = (PCMDPARA_DUNGEONMAP)d->pbyAutoMap;
	DWORD count = (m->wX2 - m->wX1 + 1) * (m->wY2 - m->wY1 + 1);
	PWORD cells = (PWORD)(h + 1);
	for (int i = 0; i < h->wCount; i ++) if ((cells[i] & 0x7fff) >= count) return;
	std::vector<WORD> added;
	for (int i = 0; i < h->wCount; i ++) {
		if (c->setMapRecord.insert (cells[i]).second) added.push_back (cells[i]);
	}
	if (!added.empty ()) DgMapSend (c, NULL, added);
}

/* 番外編ダンジョンの中か */
BOOL QuestInDungeon(PCHARINFO pChar)
{
	return DgRunOf (pChar) != NULL;
}

/* 中で増えた品（今の持ち物のうち、入った時のテンプレートの持ち物より増えた分）を集める */
static void DgBagCollect(PCHARINFO p, WORD wTmpl, std::vector<WORD> &aBag)
{
	std::map<WORD, int> mapNow, mapTmpl;
	std::map<WORD, int>::iterator it;
	int i;

	aBag.clear ();
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (p->waItem[i] != 0) {
			mapNow[p->waItem[i]] ++;
		}
	}
	if (wTmpl < (WORD)g_aStTmpInfo.GetSize ()) {
		PSTATUSTEMPLATEINFO t = g_aStTmpInfo[wTmpl];
		for (i = 0; i < MAX_MYITEM; i ++) {
			/* LoanApply と同じく、無いアイテムの番号は持たせていない */
			if ((t->waItem[i] != 0) && (t->waItem[i] < (WORD)g_aItemInfo.GetSize ())) {
				mapTmpl[t->waItem[i]] ++;
			}
		}
	}
	for (it = mapNow.begin (); it != mapNow.end (); it ++) {
		int nExtra = it->second - mapTmpl[it->first];
		while (nExtra -- > 0) {
			aBag.push_back (it->first);
		}
	}
}

/* 進行中の通知は白文字、受付への返答は会話窓に出す。 */
static void DgMessage(PCHARINFO p, int id, const char *key, BOOL talk = FALSE,
    const char *k1 = "", const std::string &v1 = "", const char *k2 = "", const std::string &v2 = "")
{
    std::string msg = DungeonText (DgInfo (id), key, k1, v1, k2, v2);
    if (talk) {
        char bounded[MAX_TALKMSG - 2];
        SboTextCopy (bounded, sizeof (bounded), msg.c_str ());
        msg = bounded; msg += "@2";
        SendCmdTALKMSG (p->pID, 0, msg.c_str ());
    }
    else SendCmdSYSMSG (p->pID, (LPSTR)msg.c_str (), SPEECHCMD_BATTLE1);
}

static void DgSendSel(PCHARINFO p, BYTE byType, const DGSEL *pSel, DWORD dwWait);

/* 選択中・受付済みとも組全員を取り消し、切断した本人以外にも同じ理由を知らせる。 */
static void DgReceptionCancel(DWORD key, const char *reason, BOOL selecting = FALSE)
{
    std::map<DWORD, DGSEL> &groups = selecting ? s_mapDgSel : s_mapDgPrepared;
    std::map<DWORD, DWORD> &members = selecting ? s_mapDgSelOf : s_mapDgPreparedOf;
    std::map<DWORD, DGSEL>::iterator it = groups.find (key);
    if (it == groups.end ()) return;
    DGSEL sel = it->second;
    groups.erase (it);
    for (size_t i = 0; i < sel.aMember.size (); i ++) {
        DWORD id = sel.aMember[i];
        members.erase (id);
        PCHARINFO p = id < (DWORD)g_aCharInfo.GetSize () ? g_aCharInfo[id] : NULL;
        if (p && IsPlayerOnline (p)) {
            if (selecting) DgSendSel (p, DUNGEONSEL_CLOSE, NULL, 0);
            DgMessage (p, sel.nDungeonID, reason);
        }
    }
}

static void DgReceptionForget(PCHARINFO pChar)
{
    if (!pChar) return;
    std::map<DWORD, DWORD>::iterator it = s_mapDgPreparedOf.find (pChar->dwTblID);
    if (it != s_mapDgPreparedOf.end ()) DgReceptionCancel (it->second, "ready_cancelled");
    it = s_mapDgSelOf.find (pChar->dwTblID);
    if (it != s_mapDgSelOf.end ()) DgReceptionCancel (it->second, "ready_cancelled", TRUE);
}

/* マップへの登録時と周期処理で受付場所を確認する。正規入場は先に受付を消費する。 */
void DungeonReceptionCheck(PCHARINFO p)
{
    if (!p) return;
    for (int selecting = 0; selecting < 2; selecting ++) {
        std::map<DWORD, DWORD> &members = selecting ? s_mapDgSelOf : s_mapDgPreparedOf;
        std::map<DWORD, DGSEL> &groups = selecting ? s_mapDgSel : s_mapDgPrepared;
        std::map<DWORD, DWORD>::iterator member = members.find (p->dwTblID);
        if (member == members.end ()) continue;
        std::map<DWORD, DGSEL>::iterator group = groups.find (member->second);
        if (group != groups.end () && (!IsPlayerOnline (p) ||
            p->dwMapID != group->second.dwMapID || p->dwSpaceID != SPACE_NORMAL))
            DgReceptionCancel (member->second, "ready_cancelled", selecting);
    }
}

/* 周期処理と入場直前の両方で確認し、期限や離脱の判定をすり抜けさせない。 */
static BOOL DgPreparedValid(DWORD key, DWORD now)
{
    std::map<DWORD, DGSEL>::iterator it = s_mapDgPrepared.find (key);
    if (it == s_mapDgPrepared.end ()) return FALSE;
    const DGSEL &sel = it->second;
    const char *reason = NULL;
    PSUBGAME_DUNGEONINFO dg = DgInfo (sel.nDungeonID);
    if (now - sel.dwStart >= DGPREPARED_WAIT) reason = "ready_timeout";
    else if (!DgIsGame (dg) || !DgReady (dg) || dg->dwBuildNo != sel.dwBuildNo) reason = "ready_cancelled";
    for (size_t i = 0; !reason && i < sel.aMember.size (); i ++) {
        DWORD id = sel.aMember[i];
        PCHARINFO p = id < (DWORD)g_aCharInfo.GetSize () ? g_aCharInfo[id] : NULL;
        if (!p || !IsPlayerOnline (p) || p->dwPartyID != sel.dwPartyID ||
            !p->wHP || p->byCharState == CHARSTATE_DEAD || QuestInChallenge (p) || HasMercenary (p) ||
            p->dwMapID != sel.dwMapID || p->dwSpaceID != SPACE_NORMAL ||
            abs ((int)p->dwX - (int)sel.dwX) > CHALLENGE_NEAR ||
            abs ((int)p->dwY - (int)sel.dwY) > CHALLENGE_NEAR) reason = "ready_cancelled";
    }
    if (reason) { DgReceptionCancel (key, reason); return FALSE; }
    return TRUE;
}

std::string DungeonBlockedText(PCHARINFO p)
{
    CHALLENGE *run = DgRunOf (p);
    return DungeonText (run ? DgInfo (run->byDungeonID) : NULL, "blocked");
}

/* 出た理由の知らせ（番外編） */
static void DgLeaveMsg(PCHARINFO p, int nReason, int nDungeonID)
{
	std::string strMsg;

	switch (nReason) {
	case CHALLENGELEAVE_CLEAR:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_clear");
		break;
	case CHALLENGELEAVE_DEAD:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_dead");
		break;
	case CHALLENGELEAVE_PARTY:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_party");
		break;
	case CHALLENGELEAVE_OUT:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_out");
		break;
	case CHALLENGELEAVE_RETIRE:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_retire");
		break;
	case CHALLENGELEAVE_BUILD:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_build");
		break;
	case CHALLENGELEAVE_QUEST:
		strMsg = DungeonText (DgInfo (nDungeonID), "leave_end");
		break;
	}
	if (nReason == CHALLENGELEAVE_ALLDEAD) strMsg = DungeonText (DgInfo (nDungeonID), "all_dead");
	if (!strMsg.empty ()) {
		SendCmdSYSMSG (p->pID, (LPSTR)strMsg.c_str (), SPEECHCMD_BATTLE1);
	}
}

/* 開始マスを踏んで「抜けますか？」を聞いている人（テーブルID → 聞いた時の場所）。
   はいが届いた時に、まだその場にいて挑戦中なら抜ける。離れていたら無効 */
typedef struct _DGLEAVEASK {
	DWORD dwMapID, dwX, dwY;
} DGLEAVEASK;
static std::map<DWORD, DGLEAVEASK> s_mapDgLeaveAsk;

/* 抜ける確認を出す・閉じる（番外編の全モード共通） */
static void DgSendLeave(PCHARINFO p, BYTE byType, const std::string &strText)
{
	CMDPARA_DUNGEONSEL Para;

	if ((p == NULL) || (p->pID == 0)) {
		return;
	}
	ZeroMemory (&Para, sizeof (Para));
	Para.byType = byType;
	Para.dwTblID = p->dwTblID;
	SboTextCopy (Para.szTitle, sizeof (Para.szTitle), strText.c_str ());
	SendCmdDUNGEONSEL (p->pID, &Para);
}

/* 開始マスを踏んだ：すぐには抜けず、はい・いいえを聞く（防衛の復活待ち・波の準備中も同じ） */
static void DgLeaveAsk(PCHARINFO p, int nDungeonID)
{
	DGLEAVEASK Ask;

	Ask.dwMapID = p->dwMapID;
	Ask.dwX = p->dwX;
	Ask.dwY = p->dwY;
	s_mapDgLeaveAsk[p->dwTblID] = Ask;
	DgSendLeave (p, DUNGEONSEL_LEAVEASK, DungeonText (DgInfo (nDungeonID), "leave_ask"));
}

/* 確認を無効にする（挑戦から外れた時など）。bSend なら窓も閉じさせる */
static void DgLeaveAskClose(PCHARINFO p, BOOL bSend)
{
	std::map<DWORD, DGLEAVEASK>::iterator it = s_mapDgLeaveAsk.find (p->dwTblID);

	if (it == s_mapDgLeaveAsk.end ()) {
		return;
	}
	s_mapDgLeaveAsk.erase (it);
	if (bSend) {
		DgSendLeave (p, DUNGEONSEL_LEAVECLOSE, "");
	}
}

/* 抜ける確認への返事。はいなら、聞いた場所にまだいて挑戦中の時だけ途中で帰る（記録なし） */
static void DgLeaveAnswer(PCHARINFO p, BOOL bYes)
{
	std::map<DWORD, DGLEAVEASK>::iterator it = s_mapDgLeaveAsk.find (p->dwTblID);
	DGLEAVEASK Ask;

	if (it == s_mapDgLeaveAsk.end ()) {
		return;
	}
	Ask = it->second;
	s_mapDgLeaveAsk.erase (it);
	if (!bYes || !DgRunOf (p) || (p->dwMapID != Ask.dwMapID) || (p->dwX != Ask.dwX) || (p->dwY != Ask.dwY)) {
		return;
	}
	ChallengeLeave (p, CHALLENGELEAVE_RETIRE);
}

/* 姿を選ぶ窓を開く・閉じる */
static void DgSendSel(PCHARINFO p, BYTE byType, const DGSEL *pSel, DWORD dwWait)
{
	CMDPARA_DUNGEONSEL Para;
	size_t i;

	if ((p == NULL) || (p->pID == 0)) {
		return;
	}
	ZeroMemory (&Para, sizeof (Para));
	Para.byType = byType;
	Para.dwTblID = p->dwTblID;
	if ((byType == DUNGEONSEL_OPEN) && pSel) {
		/* 仲間には選択窓と同時に要点を通知する。申し込んだ本人は受付の会話で読んでいる */
		if (!pSel->aMember.empty () && (p->dwTblID != pSel->aMember[0])) {
			DgMessage (p, pSel->nDungeonID, "member_guide", FALSE);
		}
		Para.byCount = (BYTE)min (pSel->aName.size (), (size_t)MAX_DGTMPL);
		Para.dwWait = dwWait;
		SboTextCopy (Para.szTitle, sizeof (Para.szTitle), DungeonText (DgInfo (pSel->nDungeonID), "select_title").c_str ());
		for (i = 0; i < (size_t)Para.byCount; i ++) {
			CopyStr (Para.aszName[i], sizeof (Para.aszName[i]), pSel->aName[i].c_str ());
		}
	}
	SendCmdDUNGEONSEL (p->pID, &Para);
}

/* 一緒に入れる仲間（pChar の近くにいる同じパーティの人。本人が先頭） */
static void DgGetMembers(PCHARINFO pChar, std::vector<PCHARINFO> &aOut)
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
		if ((pTmp->wHP == 0) || (pTmp->byCharState == CHARSTATE_DEAD) || QuestInChallenge (pTmp) || (s_mapDgSelOf.find (pTmp->dwTblID) != s_mapDgSelOf.end ()) || s_mapDgPreparedOf.count (pTmp->dwTblID)) {
			continue;
		}
		aOut.push_back (pTmp);
	}
}

/* パーティのリーダー（adwTblID の先頭）。近くにそろっていない・倒れている・挑戦中なら、乗った人が代わりに始める */
static PCHARINFO DgLeaderOf(PCHARINFO pChar)
{
	PPARTYINFO pParty;
	PCHARINFO pLeader;
	DWORD dwTbl;

	if ((pChar->dwPartyID == 0) || (pChar->dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return pChar;
	}
	pParty = g_aPartyInfo[pChar->dwPartyID];
	dwTbl = pParty->adwTblID[0];
	if ((dwTbl == 0) || (dwTbl >= (DWORD)g_aCharInfo.GetSize ()) || (dwTbl == pChar->dwTblID)) {
		return pChar;
	}
	pLeader = g_aCharInfo[dwTbl];
	if (!IsPlayerOnline (pLeader) || (pLeader->dwPartyID != pChar->dwPartyID) || (pLeader->dwMapID != pChar->dwMapID) ||
		!IsSameSpace (pLeader, pChar) || (pLeader->wHP == 0) || (pLeader->byCharState == CHARSTATE_DEAD) || QuestInChallenge (pLeader) ||
		(abs ((int)pLeader->dwX - (int)pChar->dwX) > CHALLENGE_NEAR) || (abs ((int)pLeader->dwY - (int)pChar->dwY) > CHALLENGE_NEAR)) {
		return pChar;
	}
	return pLeader;
}

/* 同じパーティの誰かが姿を選んでいる最中なら、その組の鍵（選び始めた人のテーブルID）。無ければ 0 */
static DWORD DgSelOfParty(PCHARINFO pChar)
{
	std::map<DWORD, DGSEL>::iterator it;

	if ((pChar->dwPartyID == 0) || (pChar->dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return 0;
	}
	for (it = s_mapDgSel.begin (); it != s_mapDgSel.end (); it ++) {
		PCHARINFO pKey = (it->first < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[it->first] : NULL;
		if (pKey && (pKey != pChar) && (pKey->dwPartyID == pChar->dwPartyID)) {
			return it->first;
		}
	}
	return 0;
}

/* 同じパーティの人が、番外編の中の組（空間）に入っているか */
static BOOL DgPartyRunning(PCHARINFO pChar)
{
	std::map<DWORD, CHALLENGE>::iterator it;

	if (pChar->dwPartyID == 0) {
		return FALSE;
	}
	for (it = s_mapChallenge.begin (); it != s_mapChallenge.end (); it ++) {
		if ((it->second.byKind == CHALLENGEKIND_DUNGEON) && (it->second.dwPartyID == pChar->dwPartyID) && !it->second.aMember.empty ()) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 姿の一覧と同時に出す説明。受付の独自台詞、設定、既定文の順に選ぶ。 */
static void DgSendReception(PCHARINFO pChar, int nID, DWORD dwNpcTblID)
{
	PCHARINFO pNpc = (dwNpcTblID < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[dwNpcTblID] : NULL;
	const char *p = DungeonIsReceptionNpc (pNpc) ? pNpc->pszNpcSpeech + 2 : "";
	while (*p >= '0' && *p <= '9') p ++;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p ++;
	std::string strMsg = *p ? p : DungeonText (DgInfo (nID), "reception");
	/* 旧クライアント用の入力待ちと、説明を識別する印を通信上限内に収める。 */
	char szMsg[MAX_TALKMSG - 4];
	SboTextCopy (szMsg, sizeof (szMsg), strMsg.c_str ());
	strMsg = szMsg;
	while (strMsg.size () >= 2 && (strMsg.substr (strMsg.size () - 2) == "@2" || strMsg.substr (strMsg.size () - 2) == "@0")) strMsg.resize (strMsg.size () - 2);
	strMsg += "@2@D";
	SendCmdTALKMSG (pChar->pID, dwNpcTblID, (LPSTR)strMsg.c_str ());
}

/* 選んでいる最中の組に、遅れて来た仲間を足して窓を出す（締め切りは延ばさない。残り 10 秒を切っていたら 10 秒までだけ延ばす）。
   足せたら TRUE。bTell は足せない理由を本人に知らせる（受付に話した時） */
static BOOL DgSelJoin(DWORD dwKey, PCHARINFO pChar, DWORD dwNow, BOOL bTell)
{
	std::map<DWORD, DGSEL>::iterator it = s_mapDgSel.find (dwKey);
	PCHARINFO pKey;
	DWORD dwPass, dwRest;
	size_t i;
	char szTmp[256];

	if ((it == s_mapDgSel.end ()) || (pChar == NULL) || !IsPlayerOnline (pChar)) {
		return FALSE;
	}
	DGSEL &Sel = it->second;
	if ((Sel.mapPick.find (pChar->dwTblID) != Sel.mapPick.end ()) || (s_mapDgSelOf.find (pChar->dwTblID) != s_mapDgSelOf.end ()) || s_mapDgPreparedOf.count (pChar->dwTblID)) {
		return FALSE;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD) || QuestInChallenge (pChar) || (pChar->dwMapID != Sel.dwMapID) ||
		(pChar->dwSpaceID != SPACE_NORMAL) || (abs ((int)pChar->dwX - (int)Sel.dwX) > CHALLENGE_NEAR) || (abs ((int)pChar->dwY - (int)Sel.dwY) > CHALLENGE_NEAR)) {
		return FALSE;
	}
	if (HasMercenary (pChar)) {
		if (bTell) {
			DgMessage (pChar, Sel.nDungeonID, "mercenary_self", TRUE);
		}
		return FALSE;
	}
	pKey = (dwKey < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[dwKey] : NULL;
	dwPass = dwNow - Sel.dwStart;
	if (dwPass + 10000 > DGSEL_WAIT) {
		/* 残りが少ない：10 秒までだけ延ばし、まだ選んでいない人の窓の残り時間もそろえる */
		Sel.dwStart = dwNow + 10000 - DGSEL_WAIT;
		for (i = 0; i < Sel.aMember.size (); i ++) {
			PCHARINFO p = (Sel.aMember[i] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Sel.aMember[i]] : NULL;
			if (p && IsPlayerOnline (p) && (Sel.mapPick[Sel.aMember[i]] == -1)) {
				DgSendSel (p, DUNGEONSEL_OPEN, &Sel, 10000);
			}
		}
		dwRest = 10000;
	} else {
		dwRest = DGSEL_WAIT - dwPass;
	}
	Sel.aMember.push_back (pChar->dwTblID);
	Sel.mapPick[pChar->dwTblID] = -1;
	s_mapDgSelOf[pChar->dwTblID] = dwKey;
	DgMessage (pChar, Sel.nDungeonID, "joined", FALSE, "name", pKey ? pKey->szName : "", "seconds", std::to_string ((dwRest + 999) / 1000));
	for (i = 0; i + 1 < Sel.aMember.size (); i ++) {
		PCHARINFO p = (Sel.aMember[i] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Sel.aMember[i]] : NULL;
		if (p && IsPlayerOnline (p)) {
			DgMessage (p, Sel.nDungeonID, "member_joined", FALSE, "name", pChar->szName);
		}
	}
	DgSendReception (pChar, Sel.nDungeonID, Sel.dwNpcTblID);
	DgSendSel (pChar, DUNGEONSEL_OPEN, &Sel, dwRest);
	return TRUE;
}

/* 空間の中の敵（NPC発生から出たもの）をその組の分だけ出す・補充する。bFirst は入った時に一度にそろえる */
static void DgPutProc(CHALLENGE *pCh, DWORD dwNow, BOOL bFirst)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	int i, nCount = g_aCharInfo.GetSize ();

	if ((pDungeon == NULL) || pCh->bCleared) {
		return;
	}
	/* 拠点防衛は波ごとに出す（DgDefPutProc。入った時に一度にそろえることもしない） */
	if (pCh->bDefense) {
		return;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];
		int nTry = 0, nTryMax;

		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || (pParent->pProc == NULL) || (pParent->byPutCount == 0) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		DGPUT &d = pCh->mapPut[pParent->dwTblID];
		nTryMax = bFirst ? (int)pParent->byPutCount * 50 : 1;
		while ((d.nNow < (int)pParent->byPutCount) && (nTry < nTryMax)) {
			/* 補充しない印：決めた数を出したら、倒されても出さない（数えるのはこの組だけ） */
			if (pParent->byPutOnce && (d.nTotal >= (int)pParent->byPutCount)) {
				break;
			}
			if (!bFirst && (dwNow < d.dwLast + (DWORD)pParent->wTimePutInterval * 1000)) {
				break;
			}
			nTry ++;
			/* 日替わりは、位置を版のシード・発生元の番号・何回目の試しかで決める（どの組も同じ位置） */
			DWORD dwSeed = 0;
			const DWORD *pdwSeed = NULL;
			if (pCh->bDaily) {
				dwSeed = DungeonDailyPutSeed (pCh->dwDailySeed, pParent->dwTblID, d.dwTry);
				pdwSeed = &dwSeed;
			}
			d.dwTry ++;
			if (((CCharProcPUT *)pParent->pProc)->PutOneSpace (dwNow, pCh->dwSpaceID, pdwSeed)) {
				d.nNow ++;
				d.nTotal ++;
				d.dwLast = dwNow;
			}
		}
	}
}

/* その空間の敵を全部消して、空間を空きにする */
static void DgEnd(DWORD dwSession)
{
	std::vector<DWORD> aDel;
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if ((p != NULL) && (p->dwSpaceID == dwSession) && (p->byNpcType != NPCTYPE_INIT) && (p->byGameState == GAMESTATE_NPC)) {
			aDel.push_back (p->dwTblID);
		}
	}
	for (i = 0; i < (int)aDel.size (); i ++) {
		DeleteChar (aDel[i]);
	}
	DgDefFieldDrop (dwSession);
	s_mapChallenge.erase (dwSession);
}

/* 選んだ人だけで、新しい空間へ入る */
static void DgStart(int nDungeonID, const DGSEL &Sel, const std::vector<std::pair<PCHARINFO, WORD> > &aPart, const std::vector<std::string> &aForm)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (nDungeonID);
	CHALLENGE Ch;
	DWORD dwSession;
	size_t i;
	char szTmp[512];

	dwSession = ++ s_dwChallengeSeq;
	Ch.byKind = CHALLENGEKIND_DUNGEON;
	Ch.byDungeonID = (BYTE)nDungeonID;
	Ch.dwBuildNo = pDungeon->dwBuildNo;
	Ch.wQuestID = 0;
	Ch.dwSpaceID = dwSession;						/* 組の番号をそのまま空間番号にする（0 にはならない） */
	Ch.bArena = FALSE;
	Ch.dwMapNo = pDungeon->dwMapID;
	Ch.dwPartyID = aPart[0].first->dwPartyID;
	Ch.bMapShared = aPart.size () > 1;
	Ch.dwStart = timeGetTime ();
	Ch.bCleared = FALSE;
	/* 拠点防衛：拠点を立てて、波ごとに敵を出す（周期処理の DgDefProc が進める） */
	Ch.bDefense = (pDungeon->byClearMode == DGCLEAR_DEFENSE);
	/* 日替わり：入った時の版を覚える（記録は版ごと。敵の位置の乱数にも使う）。組のキャラ名・姿の名前も記録用に控える */
	if ((pDungeon->byRebuildMode == DGREBUILD_DAILY) && pDungeon->dwDailyVer) {
		DGDAILYVER Ver;
		if (DungeonDailyFind (nDungeonID, pDungeon->dwDailyVer, Ver)) {
			Ch.bDaily = TRUE;
			Ch.dwDailyVer = Ver.dwNo;
			Ch.dwDailySeed = Ver.dwSeed;
			/* 一発勝負：全員がこの版で初めての挑戦の時だけ順位に入る。入った時点で、全員「挑戦した」ことにする（倒れても帰っても使ったことになる） */
			Ch.bRanked = TRUE;
			for (i = 0; i < aPart.size (); i ++) {
				if (DungeonDailyTried (nDungeonID, Ver.dwNo, aPart[i].first->dwTblID)) {
					Ch.bRanked = FALSE;
				}
			}
			{
				std::vector<DWORD> aTbl;
				for (i = 0; i < aPart.size (); i ++) {
					aTbl.push_back (aPart[i].first->dwTblID);
				}
				DungeonDailyMarkTried (nDungeonID, Ver.dwNo, aTbl);
			}
		}
	}
	for (i = 0; i < aPart.size (); i ++) {
		Ch.mapWho[aPart[i].first->dwTblID] = std::make_pair (std::string (aPart[i].first->szName), (i < aForm.size ()) ? aForm[i] : std::string ());
	}
	/* クリア条件「敵を全部倒す」：補充しない印の敵（範囲の中の NPC発生）がいる時だけ。いなければゴールと同じ */
	Ch.bKillAll = DungeonUsesKillAll (pDungeon);

	s_mapChallenge[dwSession] = Ch;
	CHALLENGE *pCh = &s_mapChallenge[dwSession];

	for (i = 0; i < aPart.size (); i ++) {
		PCHARINFO pTmp = aPart[i].first;
		PSTATUSTEMPLATEINFO t = g_aStTmpInfo[aPart[i].second];
		LOANBAK Bak;

		CharTargetCancelPC (pTmp);
		/* この組の空間へ（先に移る。見え方の作り直しはマップ移動のほうでされる。
		   固定の形で中の開始位置が無い時は、入口に立ったままの人が開始位置と同じマスなので、移動で何も届かない分、ここで作り直す。
		   中の開始位置を設定している時は、気まぐれと同じく別のマスへ移るので、移動のほうで届く） */
		BOOL bFixed = (pDungeon->byRebuildMode == DGREBUILD_FIXED);
		DWORD dwInX = bFixed ? (DWORD)pDungeon->byStartX : (DWORD)pDungeon->byGenX1 + pDungeon->wGenStartX;
		DWORD dwInY = bFixed ? (DWORD)pDungeon->byStartY : (DWORD)pDungeon->byGenY1 + pDungeon->wGenStartY;
		if (bFixed) {
			DungeonFixedInside (pDungeon, &dwInX, &dwInY);
		}
		SpaceSet (pTmp, Ch.dwSpaceID, (pTmp->dwMapID == pDungeon->dwMapID) && (pTmp->dwX == dwInX) && (pTmp->dwY == dwInY));
		ZeroMemory (&Bak, sizeof (Bak));
		LoanXfer (pTmp, &Bak, FALSE);
		Bak.dwSession = dwSession;
		Bak.bArrived = TRUE;
		Bak.wDgTmpl = aPart[i].second;
		/* 戻る場所は、仲間も全員、受付に申し込んだ人の場所 */
		Bak.dwMapID = Sel.dwMapID;
		Bak.dwX = Sel.dwX;
		Bak.dwY = Sel.dwY;
		Bak.byDirection = Sel.byDirection;
		s_mapLoan[pTmp->dwTblID] = Bak;
		pCh->aMember.push_back (pTmp->dwTblID);

		LoanApply (pTmp, t);
		LoanSendAll (pTmp);

		SendCmdPLAYSOUND (pTmp->pID, SOUND_MOVE);
		SendCmdFADEINOUT (pTmp->pID, 0);
		SubProcMapMove (pTmp, pDungeon->dwMapID, dwInX, dwInY);
		/* 移動で届くマップ情報は扉が閉じたままなので、この空間の扉などの状態を送る */
		SpaceSendMapState (pTmp);

		DgMessage (pTmp, nDungeonID, "entered", FALSE, "name", t->szName);
	}
	/* 拠点防衛：この組の空間に拠点を立てる */
	if (pCh->bDefense) {
		DgDefCreateBase (pCh, pDungeon);
		DgDefCreateSpringMark (pCh, pDungeon);
	}
	/* この組の敵を、決めた数まで一度に出す */
	DgPutProc (pCh, timeGetTime (), TRUE);
}

/* 選び終えた（全員が決めた・締め切り）：選んだ人だけ受付済みにする */
static void DgSelFinish(DWORD dwLeader)
{
	std::map<DWORD, DGSEL>::iterator it = s_mapDgSel.find (dwLeader);
	std::vector<std::pair<PCHARINFO, WORD> > aPart;
	std::vector<std::string> aForm;
	PSUBGAME_DUNGEONINFO pDungeon;
	DGSEL Sel;
	size_t i;

	if (it == s_mapDgSel.end ()) {
		return;
	}
	Sel = it->second;
	s_mapDgSel.erase (it);
	for (i = 0; i < Sel.aMember.size (); i ++) {
		s_mapDgSelOf.erase (Sel.aMember[i]);
	}
	pDungeon = DgInfo (Sel.nDungeonID);

	for (i = 0; i < Sel.aMember.size (); i ++) {
		DWORD dwTbl = Sel.aMember[i];
		PCHARINFO p = (dwTbl < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[dwTbl] : NULL;
		int nPick = Sel.mapPick[dwTbl];

		/* まだ開いたままの窓は閉じる（選ばなかった人は置いていく） */
		if (p && IsPlayerOnline (p) && (nPick == -1)) {
			DgSendSel (p, DUNGEONSEL_CLOSE, NULL, 0);
			DgMessage (p, Sel.nDungeonID, "selection_timeout", FALSE);
		}
		if ((nPick < 0) || (nPick >= (int)Sel.aTmpl.size ()) || (p == NULL) || !IsPlayerOnline (p)) {
			continue;
		}
		/* まだ入口の近くにいて、倒れておらず、ほかで挑戦中でない人だけ */
		if ((p->wHP == 0) || (p->byCharState == CHARSTATE_DEAD) || QuestInChallenge (p) || (p->dwPartyID != Sel.dwPartyID) || (p->dwMapID != Sel.dwMapID) || (p->dwSpaceID != SPACE_NORMAL) ||
			(abs ((int)p->dwX - (int)Sel.dwX) > CHALLENGE_NEAR) || (abs ((int)p->dwY - (int)Sel.dwY) > CHALLENGE_NEAR)) {
			DgMessage (p, Sel.nDungeonID, "moved_away", FALSE);
			continue;
		}
		aPart.push_back (std::make_pair (p, Sel.aTmpl[nPick]));
		aForm.push_back ((nPick < (int)Sel.aName.size ()) ? Sel.aName[nPick] : std::string ());
	}
	if (aPart.empty ()) {
		return;
	}
	if ((pDungeon == NULL) || !DgIsGame (pDungeon) || !DgReady (pDungeon)) {
		for (i = 0; i < aPart.size (); i ++) {
			DgMessage (aPart[i].first, Sel.nDungeonID, "not_ready", FALSE);
		}
		return;
	}
    Sel.aMember.clear ();
    Sel.dwStart = timeGetTime ();
    Sel.dwBuildNo = pDungeon->dwBuildNo;
    for (i = 0; i < aPart.size (); i ++) {
        Sel.aMember.push_back (aPart[i].first->dwTblID);
        s_mapDgPreparedOf[aPart[i].first->dwTblID] = dwLeader;
        DgMessage (aPart[i].first, Sel.nDungeonID, "ready", TRUE);
    }
    s_mapDgPrepared[dwLeader] = Sel;
}

/* 決めていない人がいなければ、受付済みにする */
static void DgSelCheck(DWORD dwLeader)
{
	std::map<DWORD, DGSEL>::iterator it = s_mapDgSel.find (dwLeader);
	std::map<DWORD, int>::iterator itP;

	if (it == s_mapDgSel.end ()) {
		return;
	}
	for (itP = it->second.mapPick.begin (); itP != it->second.mapPick.end (); itP ++) {
		if (itP->second == -1) {
			return;
		}
	}
	DgSelFinish (dwLeader);
}

/* 組の敵（補充しない印）の残りの数。出す前の分も数える */
static int DgRemain(CHALLENGE *pCh)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	int i, nRemain = 0, nCount = g_aCharInfo.GetSize ();

	if (pDungeon == NULL) {
		return 0;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];
		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || !pParent->byPutOnce || (pParent->byPutCount == 0) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		std::map<DWORD, DGPUT>::iterator it = pCh->mapPut.find (pParent->dwTblID);
		int nKilled = (it == pCh->mapPut.end ()) ? 0 : (it->second.nTotal - it->second.nNow);
		nRemain += max ((int)pParent->byPutCount - nKilled, 0);
	}
	return nRemain;
}

/* 敵の総数（クリア条件「敵を全部倒す」の、範囲の中の補充しない印の発生元が出す数の合計） */
static int DgEnemyTotal(CHALLENGE *pCh)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	int i, nTotal = 0, nCount = g_aCharInfo.GetSize ();

	if (pDungeon == NULL) {
		return 0;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];
		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || !pParent->byPutOnce || (pParent->byPutCount == 0) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		nTotal += pParent->byPutCount;
	}
	return nTotal;
}

/* 状況パネル（CMD_DGSTATUS）の今の状態を作る */
static void DgStatusMake(CHALLENGE *pCh, CMDPARA_DGSTATUS &Para, DWORD dwNow)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	std::string strTitle;
	size_t nPos;
	int nTotal;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_DGSTATUS;
	Para.byState = (pCh->bKillAll && pCh->bCleared) ? DGSTATUS_CLEAR : DGSTATUS_RUN;
	if (pCh->bKillAll) {
		Para.byGoal = DGSTATUS_GOAL_KILLALL;
		nTotal = DgEnemyTotal (pCh);
		Para.wEnemyMax = (WORD)min (nTotal, 65535);
		Para.wEnemy = (Para.byState == DGSTATUS_CLEAR) ? (WORD)0 : (WORD)min (DgRemain (pCh), 65535);
	}
	if (Para.byState == DGSTATUS_CLEAR) {
		Para.dwElapsedSec = pCh->dwClearTime / 1000;
	} else {
		Para.dwElapsedSec = ((int)(dwNow - pCh->dwStart) > 0) ? (dwNow - pCh->dwStart) / 1000 : 0;
	}
	/* 題：テーマ名（日替わりの「（今日のダンジョン）」は付けない）。無ければ「気まぐれダンジョン」 */
	if (pDungeon && pDungeon->dwThemeID && pDungeon->pbyAutoMap && (pDungeon->dwAutoMapSize >= sizeof (CMDPARA_DUNGEONMAP))) {
		PCMDPARA_DUNGEONMAP pMap = (PCMDPARA_DUNGEONMAP)pDungeon->pbyAutoMap;
		strTitle.assign (pMap->szName, strnlen (pMap->szName, sizeof (pMap->szName)));
	}
	nPos = strTitle.find ("（今日のダンジョン）");
	if (nPos != std::string::npos) {
		strTitle.erase (nPos);
	}
	if (strTitle.empty ()) {
		strTitle = "気まぐれダンジョン";
	}
	SboTextCopy (Para.szTitle, sizeof (Para.szTitle), strTitle.c_str ());
}

/* 状況パネルを組の全員に送る。変わった時と一定時間ごと（経過秒の合わせ直しも兼ねる） */
static void DgStatusSend(CHALLENGE *pCh, DWORD dwNow)
{
	CMDPARA_DGSTATUS Para, A, B;
	size_t k;

	DgStatusMake (pCh, Para, dwNow);
	A = Para;
	B = pCh->stStSent;
	A.dwElapsedSec = B.dwElapsedSec = 0;
	if ((memcmp (&A, &B, sizeof (A)) == 0) && ((int)(dwNow - pCh->dwStSentAt) < DG_STATUS_KEEP) && (pCh->dwStSentAt != 0)) {
		return;
	}
	pCh->stStSent = Para;
	pCh->dwStSentAt = dwNow;
	for (k = 0; k < pCh->aMember.size (); k ++) {
		PCHARINFO pTmp = (pCh->aMember[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[k]] : NULL;

		if (pTmp && IsPlayerOnline (pTmp) && (s_mapLoan.find (pCh->aMember[k]) != s_mapLoan.end ())) {
			SendCmdDGSTATUS (pTmp->pID, &Para);
		}
	}
}

/* クリア時間を決める（最初の 1 回だけ）。日替わりだけ、その版の最短記録に足す。記録には、今も組に残っている人だけを残す。
   組に知らせる「クリア時間 X分Y秒（今日の○位）」を返す。日替わりでなく bAlways も偽なら空 */
static std::string DgRecordClear(CHALLENGE *pRun, BOOL bAlways)
{
	DGDAILYREC Rec;
	std::string strMsg;
	size_t i;
	int nRank = 0;
	BOOL bBetter = TRUE;

	if (pRun->bCleared) {
		return std::string ();
	}
	pRun->bCleared = TRUE;
	pRun->dwClear = timeGetTime ();
	pRun->dwClearTime = pRun->dwClear - pRun->dwStart;
	if (!pRun->bDaily && !bAlways) {
		return std::string ();
	}
	if (pRun->bDaily && pRun->bRanked) {
		for (i = 0; i < pRun->aMember.size (); i ++) {
			std::map<DWORD, std::pair<std::string, std::string> >::iterator it = pRun->mapWho.find (pRun->aMember[i]);
			if (it != pRun->mapWho.end ()) {
				Rec.aName.push_back (it->second.first);
				Rec.aForm.push_back (it->second.second);
			}
		}
		Rec.aTbl = pRun->aMember;
		std::sort (Rec.aTbl.begin (), Rec.aTbl.end ());
		Rec.nCount = (int)Rec.aName.size ();
		Rec.dwMs = pRun->dwClearTime;
		Rec.strAt = DungeonDailyDateTime (DungeonDailyNow ());
		nRank = DungeonDailyAddRecord ((int)pRun->byDungeonID, pRun->dwDailyVer, Rec, &bBetter);
	}
    PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pRun->byDungeonID);
    strMsg = DungeonText (pDungeon, "clear_time", "time", DungeonDailyTimeText (pRun->dwClearTime));
    if (pRun->bDaily) {
        strMsg += DungeonText (pDungeon, !pRun->bRanked ? "unranked" :
            (nRank > 0 ? (bBetter ? "rank_best" : "rank_unchanged") : "rank_out"),
            "rank", std::to_string (nRank), "limit", std::to_string (DGDAILY_RECMAX));
    }
	return strMsg;
}

/* 組の全員に知らせる */
static void DgNoticeMembers(CHALLENGE *pCh, const char *pszMsg)
{
	size_t k;

	for (k = 0; k < pCh->aMember.size (); k ++) {
		PCHARINFO pTmp = (pCh->aMember[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[k]] : NULL;
		if (pTmp && IsPlayerOnline (pTmp)) {
			SendCmdSYSMSG (pTmp->pID, (LPSTR)pszMsg, SPEECHCMD_BATTLE1);
		}
	}
}

/* 受付から姿選びを始める（パーティの確認・遅れて来た仲間の参加も共通） */
static BOOL DgSelStart(PCHARINFO pChar, int nID, DWORD dwNpcTblID)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (nID);
	DGSEL Sel;
	std::vector<PCHARINFO> aMember;
	size_t i;
	char szTmp[256];

	if (!pChar || !DgIsGame (pDungeon) || (pChar->byNpcType != NPCTYPE_INIT) ||
		(pChar->dwSpaceID != SPACE_NORMAL) || QuestInChallenge (pChar)) {
		return TRUE;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD) || (s_mapDgSelOf.find (pChar->dwTblID) != s_mapDgSelOf.end ()) || s_mapDgPreparedOf.count (pChar->dwTblID)) {
		return TRUE;
	}
	if (!DgReady (pDungeon)) {
		DgMessage (pChar, nID, "not_ready", TRUE);
		return TRUE;
	}
	DgMakeList (pDungeon, Sel.aTmpl, Sel.aName);
	if (Sel.aTmpl.empty ()) {
		DgMessage (pChar, nID, "no_forms", TRUE);
		return TRUE;
	}
    /* 未選択だったリーダーが再受付して、同じ鍵の準備済み組を上書きしない。 */
    for (std::map<DWORD, DGSEL>::iterator it = s_mapDgPrepared.begin (); it != s_mapDgPrepared.end (); it ++) {
        if (it->first == pChar->dwTblID || (pChar->dwPartyID && it->second.dwPartyID == pChar->dwPartyID)) {
            DgMessage (pChar, nID, "party_ready", TRUE);
            return TRUE;
        }
    }
	/* 同じパーティの人がすでに番外編の中にいる時は、新しい組を作らない（同じパーティ番号の組が 2 つできて、
	   片方が終わった時にもう片方の人まで巻き込んで操作できなくなるため。遅れて来た人は組が終わってから申し込む） */
	if (DgPartyRunning (pChar)) {
		DgMessage (pChar, nID, "party_running", TRUE);
		return TRUE;
	}
	/* 同じパーティの組が姿を選んでいる最中なら、その組に加わる（リーダーより遅れて来た仲間・代わりに始めた組へ遅れて来たリーダー） */
	{
		DWORD dwKey = DgSelOfParty (pChar);
		if (dwKey != 0) {
			if ((s_mapDgSel[dwKey].nDungeonID != nID) || (s_mapDgSel[dwKey].dwNpcTblID != dwNpcTblID) ||
				!DgSelJoin (dwKey, pChar, timeGetTime (), TRUE)) {
				PCHARINFO pKey = (dwKey < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[dwKey] : NULL;
				DgMessage (pChar, nID, "party_selecting", TRUE, "name", pKey ? pKey->szName : "");
			}
			return TRUE;
		}
	}
	/* パーティのリーダー（ソロならその人）が申し込んだ時に始める */
	if (DgLeaderOf (pChar) != pChar) {
		DgMessage (pChar, nID, "leader_only", TRUE);
		return TRUE;
	}
	DgGetMembers (pChar, aMember);
	for (i = 0; i < aMember.size (); i ++) {
		if (HasMercenary (aMember[i])) {
			DgMessage (pChar, nID, "mercenary", TRUE, "name", aMember[i]->szName);
			return TRUE;
		}
	}
	Sel.dwPartyID = pChar->dwPartyID;
	Sel.nDungeonID = nID;
	Sel.dwNpcTblID = dwNpcTblID;
	Sel.dwStart = timeGetTime ();
	Sel.dwMapID = pChar->dwMapID;
	Sel.dwX = pChar->dwX;
	Sel.dwY = pChar->dwY;
	Sel.byDirection = pChar->byDirection;
	for (i = 0; i < aMember.size (); i ++) {
		Sel.aMember.push_back (aMember[i]->dwTblID);
		Sel.mapPick[aMember[i]->dwTblID] = -1;
	}
	s_mapDgSel[pChar->dwTblID] = Sel;
	/* 今このダンジョンに入っている組の数を知らせる（入っている組がいれば同じ形、いなければ新しい形） */
	{
		std::map<DWORD, CHALLENGE>::iterator itC;
		int nRun = 0;
		char szRun[128];
		for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
			if ((itC->second.byKind == CHALLENGEKIND_DUNGEON) && (itC->second.byDungeonID == (BYTE)nID)) {
				nRun ++;
			}
		}
        for (i = 0; i < aMember.size (); i ++) {
            DgMessage (aMember[i], nID, nRun > 0 ? "running" : "empty", FALSE, "count", std::to_string (nRun));
        }
	}
	/* 日替わり：日付が変わっていたら知らせる。今日の 1〜3 位も知らせる */
	if ((pDungeon->byRebuildMode == DGREBUILD_DAILY) && pDungeon->dwDailyVer) {
		DGDAILYVER Ver;
		std::string strTop = DungeonDailyTopText (nID, pDungeon->dwDailyVer);
		std::string strFirst = DungeonText (pDungeon, "daily_first");
		std::string strAgain = DungeonText (pDungeon, "daily_again");
		BOOL bOld = DungeonDailyFind (nID, pDungeon->dwDailyVer, Ver) && (Ver.strDate != DungeonDailyDate (DungeonDailyNow ()));
		for (i = 0; i < aMember.size (); i ++) {
			if (bOld) {
				DgMessage (aMember[i], nID, "daily_changed", FALSE);
			}
			if (!strTop.empty ()) {
				SendCmdSYSMSG (aMember[i]->pID, (LPSTR)strTop.c_str (), SPEECHCMD_BATTLE1);
			}
			SendCmdSYSMSG (aMember[i]->pID, (LPSTR)(DungeonDailyTried (nID, pDungeon->dwDailyVer, aMember[i]->dwTblID) ? strAgain.c_str () : strFirst.c_str ()), SPEECHCMD_BATTLE1);
		}
	}
	/* 近くにいない仲間を知らせる（締め切りまでに受付の近くへ来れば組に加わる） */
	{
		std::vector<PCHARINFO> aParty;
		std::string strFar;
		GetPartyMembers (pChar, aParty);
		for (i = 0; i < aParty.size (); i ++) {
			if ((Sel.mapPick.find (aParty[i]->dwTblID) == Sel.mapPick.end ()) && !QuestInChallenge (aParty[i])) {
				if (!strFar.empty ()) {
					strFar += DungeonText (pDungeon, "name_separator");
				}
				strFar += DungeonText (pDungeon, "far_name", "name", aParty[i]->szName);
			}
		}
		if (!strFar.empty ()) {
			strFar = DungeonText (pDungeon, "far_members", "names", strFar);
			for (i = 0; i < aMember.size (); i ++) {
				SendCmdSYSMSG (aMember[i]->pID, (LPSTR)strFar.c_str (), SPEECHCMD_BATTLE1);
			}
		}
	}
	for (i = 0; i < aMember.size (); i ++) {
		s_mapDgSelOf[aMember[i]->dwTblID] = pChar->dwTblID;
		DgSendReception (aMember[i], nID, dwNpcTblID);
		DgSendSel (aMember[i], DUNGEONSEL_OPEN, &Sel, DGSEL_WAIT);
	}
	return TRUE;
}

/* 台詞が「@d番号」のダンジョン受付か */
BOOL DungeonIsReceptionNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'd');
}

/* 受付の番号はサブゲーム番号（0 から）。設定違いの受付では始めない */
static void DgReceptionTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	if (!pChar || !DungeonIsReceptionNpc (pNpc)) {
		return;
	}
	const char *p = pNpc->pszNpcSpeech + 2;
	int nID = 0;
	BOOL bDigit = (*p >= '0') && (*p <= '9');
	while ((*p >= '0') && (*p <= '9') && (nID < 1000)) {
		nID = nID * 10 + (*p ++ - '0');
	}
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (nID);
	if (!bDigit || !DgIsGame (pDungeon) || (pNpc->dwMapID != pDungeon->dwMapID) ||
		(pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pChar, pNpc) ||
		DgInArea (pDungeon, pNpc->dwMapID, pNpc->dwX, pNpc->dwY) ||
		(abs ((int)pChar->dwX - (int)pNpc->dwX) > 2) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > 2)) {
		DgMessage (pChar, nID, "invalid_reception", TRUE);
		return;
	}
	DgSelStart (pChar, nID, pNpc->dwTblID);
}

void DungeonReceptionTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	DgReceptionTalk (pChar, pNpc);
}

/* 入口に乗った時（EventProc_MAPCHG の最初）：番外編の外の入口なら受付済みの組を入れる。出口なら出す。処理したら TRUE */
BOOL DungeonEventStep(PCHARINFO pChar, PEVENTINFO pEvent)
{
	PEVENTINFO_MAPCHG pMapChg;
	PSUBGAME_DUNGEONINFO pDungeon;
	CHALLENGE *pRun;
	int nID;

	if ((pChar == NULL) || (pEvent == NULL) || (pEvent->byType != EVENTTYPE_MAPCHG) || (pEvent->pData == NULL) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return FALSE;
	}
	pMapChg = (PEVENTINFO_MAPCHG)pEvent->pData;

	/* 外の入口は受付済みの組を一緒に入れる。未受付なら受付へ案内する。 */
	nID = DgFindByEntrance (pChar->dwMapID, pEvent->dwX, pEvent->dwY);
	/* 固定の形は入口が開始位置と同じマス：中の人が一度離れて戻ると、途中で帰る（記録なし） */
	if (nID >= 0) {
		pRun = DgRunOf (pChar);
		pDungeon = DgInfo (nID);
		if (pRun && pDungeon && (pDungeon->byRebuildMode == DGREBUILD_FIXED) && (pRun->byDungeonID == (BYTE)nID)) {
			DgLeaveAsk (pChar, nID);
			return TRUE;
		}
	}
	/* 固定の形で中の開始位置を設定している時：そのマスに中の人が乗ったら（一度離れて戻った時）、途中で帰る（記録なし）。
	   組の外の人には何もしない（自動で置いたワープで外の入口へ飛ばされないように） */
	if (nID < 0) {
		int nFix = DgFindByFixInside (pChar->dwMapID, pEvent->dwX, pEvent->dwY);
		if (nFix >= 0) {
			pRun = DgRunOf (pChar);
			if (pRun && (pRun->byDungeonID == (BYTE)nFix)) {
				DgLeaveAsk (pChar, nFix);
				return TRUE;
			}
			pDungeon = DgInfo (nFix);
			if (!pRun && (pMapChg->dwMapID == pDungeon->dwMapID) && (pMapChg->dwX == (DWORD)pDungeon->byStartX) && (pMapChg->dwY == (DWORD)pDungeon->byStartY)) {
				return TRUE;
			}
		}
	}
	if (nID >= 0) {
        std::map<DWORD, DWORD>::iterator ready = s_mapDgPreparedOf.find (pChar->dwTblID);
        if (ready != s_mapDgPreparedOf.end ()) {
            DWORD key = ready->second;
            if (DgPreparedValid (key, timeGetTime ()) && s_mapDgPrepared[key].nDungeonID == nID) {
                DGSEL sel = s_mapDgPrepared[key];
                std::vector<std::pair<PCHARINFO, WORD> > members;
                std::vector<std::string> forms;
                pDungeon = DgInfo (nID);
                for (size_t i = 0; i < sel.aMember.size (); i ++) {
                    PCHARINFO p = g_aCharInfo[sel.aMember[i]];
                    if (abs ((int)p->dwX - pDungeon->byStartX) > CHALLENGE_NEAR ||
                        abs ((int)p->dwY - pDungeon->byStartY) > CHALLENGE_NEAR) {
                        for (size_t j = 0; j < sel.aMember.size (); j ++)
                            DgMessage (g_aCharInfo[sel.aMember[j]], nID, "ready_gather", FALSE, "name", p->szName);
                        return TRUE;
                    }
                    int pick = sel.mapPick[p->dwTblID];
                    members.push_back (std::make_pair (p, sel.aTmpl[pick]));
                    forms.push_back (sel.aName[pick]);
                }
                s_mapDgPrepared.erase (key);
                for (size_t i = 0; i < sel.aMember.size (); i ++) s_mapDgPreparedOf.erase (sel.aMember[i]);
                DgStart (nID, sel, members, forms);
                return TRUE;
            }
        }
		DgMessage (pChar, nID, "entrance", TRUE);
		return TRUE;
	}

	/* ゴール：番外編の中の人が、外の入口へ戻るワープに乗った。元の姿に戻して外へ出す */
	pRun = DgRunOf (pChar);
	if (pRun) {
		pDungeon = DgInfo (pRun->byDungeonID);
		if (pDungeon && (pMapChg->dwMapID == pDungeon->dwMapID) && (pMapChg->dwX == (DWORD)pDungeon->byStartX) && (pMapChg->dwY == (DWORD)pDungeon->byStartY)) {
			/* 中の入口（入った時に立つ所）は途中で帰る出口（記録なし） */
			if ((pEvent->dwX == (DWORD)pDungeon->byGenX1 + pDungeon->wGenStartX) && (pEvent->dwY == (DWORD)pDungeon->byGenY1 + pDungeon->wGenStartY)) {
				DgLeaveAsk (pChar, pRun->byDungeonID);
				return TRUE;
			}
			/* 設定変更前から残ったワープでも、全敵撃破では途中帰還として扱う（記録なし） */
			/* 拠点防衛はゴールのワープで勝つことは無い（勝ち負けはウェーブと拠点で決まる）。
			   中の開始位置を動かした後に残った古いワープなども、途中で帰るかを聞くだけにする */
			if (pRun->bKillAll || pRun->bDefense) {
				DgLeaveAsk (pChar, pRun->byDungeonID);
				return TRUE;
			}
			/* 組の誰かが最初にゴールした：クリア時間を記録する（まだ中にいる全員に知らせる文は、出した後に送る） */
			std::string strClear = DgRecordClear (pRun, TRUE);
			std::vector<DWORD> aNotice = pRun->aMember;
			ChallengeLeave (pChar, CHALLENGELEAVE_CLEAR);
			if (!strClear.empty ()) {
				for (size_t k = 0; k < aNotice.size (); k ++) {
					PCHARINFO pTmp = (aNotice[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[aNotice[k]] : NULL;
					if (pTmp && IsPlayerOnline (pTmp)) {
						SendCmdSYSMSG (pTmp->pID, (LPSTR)strClear.c_str (), SPEECHCMD_BATTLE1);
					}
				}
			}
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}

/* 姿を選ぶ・やめる（CMD_DUNGEONSEL の受信） */
void DungeonSelRecv(PCHARINFO pChar, PCMDPARA_DUNGEONSEL pPara)
{
	/* 旧クライアントの読み終えた通知では再開・再送・締め切り延長をしない。 */
	if (pPara->byType == DUNGEONSEL_BEGIN) return;
	/* 開始マスを踏んだ時の「抜けますか？」への返事（姿選びとは別） */
	if ((pPara->byType == DUNGEONSEL_LEAVEYES) || (pPara->byType == DUNGEONSEL_LEAVENO)) {
		DgLeaveAnswer (pChar, pPara->byType == DUNGEONSEL_LEAVEYES);
		return;
	}

	std::map<DWORD, DWORD>::iterator itOf = s_mapDgSelOf.find (pChar->dwTblID);
	std::map<DWORD, DGSEL>::iterator itS;
	DWORD dwLeader;

	if (itOf == s_mapDgSelOf.end ()) {
		return;
	}
	dwLeader = itOf->second;
	itS = s_mapDgSel.find (dwLeader);
	if (itS == s_mapDgSel.end ()) {
		s_mapDgSelOf.erase (itOf);
		return;
	}
	if (pPara->byType == DUNGEONSEL_PICK) {
		if (pPara->byIndex >= (BYTE)itS->second.aTmpl.size ()) {
			return;
		}
		itS->second.mapPick[pChar->dwTblID] = (int)pPara->byIndex;
	} else if (pPara->byType == DUNGEONSEL_CANCEL) {
		itS->second.mapPick[pChar->dwTblID] = -2;
		DgMessage (pChar, itS->second.nDungeonID, "cancel", FALSE);
	} else {
		return;
	}
	DgSendSel (pChar, DUNGEONSEL_CLOSE, NULL, 0);
	DgSelCheck (dwLeader);
}

/* 選んでいる最中の人が切断した */
static void DgSelOnDec(PCHARINFO pChar)
{
	DgReceptionForget (pChar);
}

/* ========================================================================= */
/* 拠点防衛																	 */
/* ========================================================================= */
/*
 * サブゲーム「ダンジョン」の byClearMode を DGCLEAR_DEFENSE（2）にすると、番外編の組の空間に拠点キャラが立ち、
 * NPC発生の byPutWave に合わせて波ごとに敵が来る。拠点のHPが 0 になると負け、最後の波をしのぐと勝ち。
 * どちらも結果を知らせて約 10 秒後に全員が外へ戻る（普通の番外編のクリアと同じ）。
 * 倒れた人はその場に体を残さず拠点の横へ移り、倒れた回数×5 秒（最大 30 秒）で HP/Mana 満タンで起き上がる。
 * 敵の行進・拠点への攻撃は DgIsDefenseEnemy / DgDefenseBaseOf を使って別に作る。
 */
#define DGDEF_BASE_HP			(1000)								/* 拠点の最大HP（設定が 0 の時） */
#define DGDEF_PREP_SEC			(20)								/* 準備時間・秒（設定が 0 の時） */
#define DGDEF_WAVEMAX_SEC		(180)								/* 1つの波の最長時間・秒（設定が 0 の時） */
#define DGDEF_RESULT_WAIT		(10000)								/* 結果を知らせてから帰るまで（ミリ秒） */
#define DGDEF_REVIVE_STEP		(5)									/* 復活までの秒数＝倒れた回数×この秒数 */
#define DGDEF_REVIVE_MAX		(30)								/* 復活までの最大秒数 */
#define DGDEF_STATUS_KEEP		(5000)								/* 画面の帯の状態は、変わらなくてもこの間隔（ミリ秒）で送り直す（つなぎ直した・入り直した人にも届く） */
#define DGDEF_SPRING_PERCENT	(10)								/* 泉で 1 回に戻る Mana の割合の既定（最大 Mana の %。最低 1。設定 byDefSpringPct が 0 の時） */
#define DGDEF_SPRING_FX_INTERVAL	(3000)							/* 泉のキラッとしたエフェクトを出す間隔（ミリ秒。入って最初に戻った時は必ず出す） */
#define DGDEF_SPRING_INTERVAL	(900)								/* 泉で Mana が戻る間隔（ミリ秒。周期処理が約 1 秒ごとなので少し短く） */

static int DgDefPrepSec(PSUBGAME_DUNGEONINFO p)
{
	return p->byDefPrepSec ? (int)p->byDefPrepSec : DGDEF_PREP_SEC;
}

static int DgDefWaveMaxSec(PSUBGAME_DUNGEONINFO p)
{
	return p->wDefWaveMaxSec ? (int)p->wDefWaveMaxSec : DGDEF_WAVEMAX_SEC;
}

static int DgDefWaveCount(PSUBGAME_DUNGEONINFO p)
{
	return max ((int)p->byDefWaveCount, 1);
}

/* 組の拠点キャラ（いなければ NULL） */
static PCHARINFO DgDefBaseChar(CHALLENGE *pCh)
{
	PCHARINFO p;

	if (!pCh->bDefense || (pCh->dwBaseTbl >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	p = g_aCharInfo[pCh->dwBaseTbl];
	if ((p == NULL) || (p->byGameState != GAMESTATE_NPC) || (p->dwSpaceID != pCh->dwSpaceID) || (p->byNpcType != NPCTYPE_NOMOVE)) {
		return NULL;
	}
	return p;
}

/* 組の全員に、szMessages のキーの文を知らせる */
static void DgDefSay(CHALLENGE *pCh, const char *pszKey, const char *k1 = "", const std::string &v1 = "", const char *k2 = "", const std::string &v2 = "", const char *k3 = "", const std::string &v3 = "")
{
	DgNoticeMembers (pCh, DungeonText (DgInfo (pCh->byDungeonID), pszKey, k1, v1, k2, v2, k3, v3).c_str ());
}

/* 拠点キャラを作る（ゴール地点に立つ。動かない・攻撃しない・プレイヤーからは攻撃できない NPC） */
static void DgDefCreateBase(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon)
{
	PCHARINFO pNew;
	CCharProcBase CharProcBase;
	std::string strName = DungeonText (pDungeon, "def_base_name");

	if (pDungeon->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	pNew = new CHARINFO;
	ZeroMemory ((PBYTE)pNew, sizeof (CHARINFO));
	pNew->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData (pNew);
	pNew->dwSpaceID			= pCh->dwSpaceID;					/* 組の空間にだけ立つ */
	pNew->byGameState		= GAMESTATE_NPC;
	pNew->byCharState		= CHARSTATE_MOVE;
	pNew->dwMapID			= pDungeon->dwMapID;
	pNew->dwStartMapID		= pDungeon->dwMapID;
	pNew->dwXBack = pNew->dwX = pNew->dwStartX = (DWORD)pDungeon->byGoalX;
	pNew->dwYBack = pNew->dwY = pNew->dwStartY = (DWORD)pDungeon->byGoalY;
	pNew->byDirection		= 1;
	/* 見た目のテンプレート（強さではなく、名前以外の見た目まわりだけ使う） */
	if ((pDungeon->wDefBaseStTmp != 0) && (pDungeon->wDefBaseStTmp < (WORD)g_aStTmpInfo.GetSize ())) {
		PSTATUSTEMPLATEINFO t = g_aStTmpInfo[pDungeon->wDefBaseStTmp];
		pNew->byLevel		= t->byLevel;
		pNew->chAttrFire	= t->chAttrFire;
		pNew->chAttrWater	= t->chAttrWater;
		pNew->chAttrWind	= t->chAttrWind;
		pNew->chAttrEarth	= t->chAttrEarth;
		pNew->chAttrLight	= t->chAttrLight;
		pNew->chAttrDark	= t->chAttrDark;
		pNew->byGhost		= t->byGhost;
	}
	pNew->wMaxHP			= pDungeon->wDefBaseHP ? pDungeon->wDefBaseHP : (WORD)DGDEF_BASE_HP;
	pNew->wHP				= pNew->wMaxHP;
	pNew->dwGold			= 0;
	pNew->dwExp				= 0;
	pNew->byDropProbability	= 0;
	CopyStr (pNew->szName, sizeof (pNew->szName), strName.c_str ());

	pNew = AddChar (pNew);
	pNew->pID = pNew->dwTblID + 1;
	/* 発生元のない NPC は保存される NPC とみなされるので、自分自身を発生元にして保存の対象から外す */
	pNew->dwParentTblID = pNew->dwTblID;
	if (pNew->pProc == NULL) {
		CharProcBase.SetProc (pNew);
	}
	pCh->dwBaseTbl = pNew->dwTblID;
	SendCmdCHARINFO3 (pNew->dwTblID);
}

/* 泉の目印を作る：泉の位置に名前だけ見える透明な NPC（通り抜けられる・攻撃や特技の対象にならない）。
   泉の位置を決めていない時（拠点のまわりが泉）は、拠点の名前で分かるので作らない。
   組の空間に立つので、組が終わる時（DgEnd）に一緒に消える */
static void DgDefCreateSpringMark(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon)
{
	PCHARINFO pNew;
	CCharProcBase CharProcBase;
	std::string strName = DungeonText (pDungeon, "def_spring_name");

	if ((pDungeon->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) || ((pDungeon->byDefSpringX == 0) && (pDungeon->byDefSpringY == 0)) || strName.empty ()) {
		return;
	}
	pNew = new CHARINFO;
	ZeroMemory ((PBYTE)pNew, sizeof (CHARINFO));
	pNew->byNpcType = NPCTYPE_NOBLOCK;
	SetInitCharData (pNew);
	pNew->dwSpaceID			= pCh->dwSpaceID;
	pNew->byGameState		= GAMESTATE_NPC;
	pNew->byCharState		= CHARSTATE_MOVE;
	pNew->dwMapID			= pDungeon->dwMapID;
	pNew->dwStartMapID		= pDungeon->dwMapID;
	pNew->dwXBack = pNew->dwX = pNew->dwStartX = (DWORD)pDungeon->byDefSpringX;
	pNew->dwYBack = pNew->dwY = pNew->dwStartY = (DWORD)pDungeon->byDefSpringY;
	pNew->byDirection		= 1;
	pNew->byGhost			= CHARGHOST_NAMEONLY;				/* 体を描かず名前だけ見せる */
	pNew->wMaxHP			= 0;
	pNew->wHP				= 0;
	pNew->dwGold			= 0;
	pNew->dwExp				= 0;
	pNew->byDropProbability	= 0;
	pNew->clName			= RGB (139, 213, 255);				/* 泉で戻る時の数字と同じ水色 */
	CopyStr (pNew->szName, sizeof (pNew->szName), strName.c_str ());

	pNew = AddChar (pNew);
	pNew->pID = pNew->dwTblID + 1;
	/* 保存される NPC とみなされないように、自分自身を発生元にする */
	pNew->dwParentTblID = pNew->dwTblID;
	if (pNew->pProc == NULL) {
		CharProcBase.SetProc (pNew);
	}
	SendCmdCHARINFO3 (pNew->dwTblID);
}

/* 組の空間で出た敵か（NPC発生から出たもの。拠点・傭兵・矢などは除く） */
static BOOL DgDefIsEnemyOf(CHALLENGE *pCh, PCHARINFO p)
{
	PCHARINFO pParent;

	if ((p == NULL) || (p->dwSpaceID != pCh->dwSpaceID) || (p->byGameState != GAMESTATE_NPC) || (p->dwTblID == pCh->dwBaseTbl) ||
		(p->dwParentTblID >= (DWORD)g_aCharInfo.GetSize ()) || (p->dwParentTblID == p->dwTblID)) {
		return FALSE;
	}
	pParent = g_aCharInfo[p->dwParentTblID];
	return (pParent != NULL) && (pParent->byNpcType == NPCTYPE_PUT);
}

/* 組の空間にいる生きている敵の数 */
static int DgDefEnemyCount(CHALLENGE *pCh)
{
	int i, nCount = g_aCharInfo.GetSize (), nEnemy = 0;

	for (i = 0; i < nCount; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (DgDefIsEnemyOf (pCh, p) && (p->wHP > 0)) {
			nEnemy ++;
		}
	}
	return nEnemy;
}

/* 発生元が今の組で出す数：設定の最大発生数は DGDEF_FULL_MEMBERS 人で遊ぶ時の数。人数が少ない時はその割合に減らす（切り上げ・最低 1 体） */
#define DGDEF_FULL_MEMBERS	(4)
static int DgDefPutLimit(CHALLENGE *pCh, PCHARINFO pParent)
{
	int nMember = (int)pCh->aMember.size ();
	int nCount = (int)pParent->byPutCount;

	if (nMember >= DGDEF_FULL_MEMBERS) {
		return nCount;
	}
	nMember = max (nMember, 1);
	return max ((nCount * nMember + DGDEF_FULL_MEMBERS - 1) / DGDEF_FULL_MEMBERS, 1);
}

/* 今の波の「残り敵」：まだ出ていない数（発生元の最大発生数の残り）＋今いる生きている敵（画面の帯と知らせで共通） */
static int DgDefEnemyLeft(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon)
{
	int i, nCount = g_aCharInfo.GetSize (), nLeft = DgDefEnemyCount (pCh);

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];

		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || (pParent->byPutCount == 0) || (pParent->byPutWave != (BYTE)pCh->nWave) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		std::map<DWORD, DGPUT>::iterator it = pCh->mapPut.find (pParent->dwTblID);
		int nTotal = (it == pCh->mapPut.end ()) ? 0 : it->second.nTotal;
		nLeft += max (DgDefPutLimit (pCh, pParent) - nTotal, 0);
	}
	return nLeft;
}

/* 組の敵を全部消す（結果が出た時） */
static void DgDefRemoveEnemies(CHALLENGE *pCh)
{
	std::vector<DWORD> aDel;
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		if (DgDefIsEnemyOf (pCh, g_aCharInfo[i])) {
			aDel.push_back ((DWORD)i);
		}
	}
	for (i = 0; i < (int)aDel.size (); i ++) {
		DeleteChar (aDel[i]);
	}
}

/* 今の波の発生元が、決めた数を出し切ったか（今の波の発生元が 1 つも無ければ出し切ったことにする） */
static BOOL DgDefWaveExhausted(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon)
{
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];

		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || (pParent->byPutCount == 0) || (pParent->byPutWave != (BYTE)pCh->nWave) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		std::map<DWORD, DGPUT>::iterator it = pCh->mapPut.find (pParent->dwTblID);
		if ((it == pCh->mapPut.end ()) || (it->second.nTotal < DgDefPutLimit (pCh, pParent))) {
			return FALSE;
		}
	}
	return TRUE;
}

/* 波の敵を出す：今の波の発生元（byPutWave が今の波）だけが、最大発生数の数を発生周期（秒。0 なら 1 秒）おきに 1 体ずつ出す */
static void DgDefPutProc(CHALLENGE *pCh, DWORD dwNow)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	int i, nCount = g_aCharInfo.GetSize ();

	if ((pDungeon == NULL) || (pCh->byDefPhase != DGDEF_PHASE_WAVE) || (pCh->byDefResult != DGDEF_RESULT_NONE)) {
		return;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];
		DWORD dwInterval;
		int nTry;

		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || (pParent->pProc == NULL) || (pParent->byPutCount == 0) ||
			(pParent->byPutWave != (BYTE)pCh->nWave) ||
			(pParent->dwSpaceID != SPACE_NORMAL) || !DgInArea (pDungeon, pParent->dwMapID, pParent->dwX, pParent->dwY)) {
			continue;
		}
		DGPUT &d = pCh->mapPut[pParent->dwTblID];
		if (d.nTotal >= DgDefPutLimit (pCh, pParent)) {
			continue;
		}
		dwInterval = pParent->wTimePutInterval ? (DWORD)pParent->wTimePutInterval * 1000 : 1000;
		if ((d.dwDefLast != 0) && ((int)(dwNow - d.dwDefLast) < (int)dwInterval)) {
			continue;
		}
		/* 置けない所に当たった時は、別の所で数回やり直す（出せたら 1 体で終わり） */
		for (nTry = 0; nTry < 5; nTry ++) {
			d.dwTry ++;
			if (((CCharProcPUT *)pParent->pProc)->PutOneSpace (dwNow, pCh->dwSpaceID, NULL)) {
				d.nNow ++;
				d.nTotal ++;
				d.dwDefLast = dwNow ? dwNow : 1;
				break;
			}
		}
	}
}

/* 倒れた人の復活（HP/Mana 満タン。死亡ペナルティは通さない） */
static void DgDefRevive(PCHARINFO p)
{
	BOOL bDead = (p->byCharState == CHARSTATE_DEAD) || (p->wHP == 0);

	p->wHP = max (p->wMaxHP, (WORD)1);
	p->wMana = p->wMaxMana;
	if (p->byCharState == CHARSTATE_DEAD) {
		p->byCharState = CHARSTATE_MOVE;
	}
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, p->dwTblID);
	SendCmdCHARSTATUSHPMANA (p->dwTblID, p->dwTblID);
	if (bDead && IsPlayerOnline (p)) {
		SendCmdSYSMSG2 (p->pID, SYSMSG_RESURRECTION, p->pID, 0);
	}
}

/* 拠点の 1〜2 マス以内の空きマスを探す（通れる・他のキャラがいない所） */
static BOOL DgDefFindSeat(PCHARINFO pWho, PCHARINFO pBase, int &x, int &y)
{
	PMAPINFO pMap;
	int d, dx, dy;

	if (pBase->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return FALSE;
	}
	pMap = g_aMapInfo[pBase->dwMapID];
	for (d = 1; d <= 2; d ++) {
		for (dy = -d; dy <= d; dy ++) {
			for (dx = -d; dx <= d; dx ++) {
				int cx = (int)pBase->dwX + dx, cy = (int)pBase->dwY + dy;

				if ((max (abs (dx), abs (dy)) != d) || (cx < 0) || (cy < 0) || (cx >= pMap->nCX) || (cy >= pMap->nCY)) {
					continue;
				}
				if (!CheckCharBlockFor (pWho, pBase->dwMapID, (DWORD)cx, (DWORD)cy)) {
					continue;
				}
				if ((CheckCharBlockNPC2 (pWho->dwTblID, pBase->dwMapID, (DWORD)cx, (DWORD)cy, pBase->dwSpaceID, pWho) != (DWORD)-1) ||
					(CheckCharBlockPC (pBase->dwMapID, (DWORD)cx, (DWORD)cy, pBase->dwSpaceID) != (DWORD)-1)) {
					continue;
				}
				x = cx;
				y = cy;
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* 準備時間に入る時：倒れている人を起こす（HP/Mana の全回復はしない。回復は泉で） */
static void DgDefRecoverAll(CHALLENGE *pCh)
{
	size_t j;

	for (j = 0; j < pCh->aMember.size (); j ++) {
		PCHARINFO p = (pCh->aMember[j] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[j]] : NULL;

		if ((p == NULL) || (s_mapLoan.find (pCh->aMember[j]) == s_mapLoan.end ())) {
			continue;
		}
		/* 2026-11-21 全回復はやめた（回復は泉で）。倒れて復活待ちの人だけ、ここで復活させる */
		if ((p->byCharState != CHARSTATE_DEAD) && (p->wHP != 0)) {
			continue;
		}
		DgDefRevive (p);
		pCh->mapDead[pCh->aMember[j]].dwReviveAt = 0;
	}
}

/* 画面の帯（CMD_DGDEFSTATUS）の今の状態を作る */
static void DgDefStatusMake(CHALLENGE *pCh, CMDPARA_DGDEFSTATUS &Para, DWORD dwNow)
{
	PSUBGAME_DUNGEONINFO pDungeon = DgInfo (pCh->byDungeonID);
	PCHARINFO pBase = DgDefBaseChar (pCh);

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_DGDEFSTATUS;
	if (pCh->byDefResult == DGDEF_RESULT_WIN) {
		Para.byState = DGDEFSTATUS_WIN;
	} else if (pCh->byDefResult == DGDEF_RESULT_LOSE) {
		Para.byState = DGDEFSTATUS_LOSE;
	} else if (pCh->byDefPhase == DGDEF_PHASE_WAVE) {
		Para.byState = DGDEFSTATUS_WAVE;
	} else {
		Para.byState = DGDEFSTATUS_PREP;
	}
	Para.byWave = (BYTE)min (max (pCh->nWave, 0), 255);
	Para.byWaveMax = (BYTE)min (pDungeon ? DgDefWaveCount (pDungeon) : 1, 255);
	Para.wBaseHP = pBase ? pBase->wHP : (WORD)0;
	Para.wBaseMaxHP = pBase ? pBase->wMaxHP : (pDungeon && pDungeon->wDefBaseHP ? pDungeon->wDefBaseHP : (WORD)DGDEF_BASE_HP);
	if ((Para.byState == DGDEFSTATUS_PREP) && pDungeon) {
		/* 準備中：次の波までの残り秒（切り上げ。最初の周期処理の前は準備時間まるごと） */
		int nRest = DgDefPrepSec (pDungeon) * 1000;

		if (pCh->byDefPhase == DGDEF_PHASE_PREP) {
			nRest -= (int)(dwNow - pCh->dwDefPhaseStart);
		}
		Para.wRestSec = (WORD)min (max ((nRest + 999) / 1000, 0), 65535);
	}
	if (Para.byState == DGDEFSTATUS_WAVE) {
		Para.wEnemy = (WORD)min (pDungeon ? DgDefEnemyLeft (pCh, pDungeon) : DgDefEnemyCount (pCh), 65535);
	}
}

/* 画面の帯の状態を組の全員（pOnly があればその人だけ）に送る。変わった時と一定時間ごと、bForce なら必ず */
static void DgDefStatusSend(CHALLENGE *pCh, DWORD dwNow, PCHARINFO pOnly, BOOL bForce)
{
	CMDPARA_DGDEFSTATUS Para;
	size_t k;

	DgDefStatusMake (pCh, Para, dwNow);
	if (pOnly == NULL) {
		if (!bForce && (memcmp (&Para, &pCh->stDefSent, sizeof (Para)) == 0) && ((int)(dwNow - pCh->dwDefSentAt) < DGDEF_STATUS_KEEP)) {
			return;
		}
		pCh->stDefSent = Para;
		pCh->dwDefSentAt = dwNow;
	}
	for (k = 0; k < pCh->aMember.size (); k ++) {
		PCHARINFO pTmp = (pCh->aMember[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[k]] : NULL;

		if (pTmp && IsPlayerOnline (pTmp) && ((pOnly == NULL) || (pOnly == pTmp)) && (s_mapLoan.find (pCh->aMember[k]) != s_mapLoan.end ())) {
			SendCmdDGDEFSTATUS (pTmp->pID, &Para);
		}
	}
}

/* 全体地図（CMD_DGDEFMAP）を組の全員に送る（周期処理ごと＝約 1 秒）。敵は座標だけの詰めた形で上限 DGDEFMAP_MAXENEMY 体 */
static void DgDefMapSend(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon)
{
	CMDPARA_DGDEFMAP Para;
	std::vector<BYTE> aPos, aEnemy;
	PCHARINFO pBase = DgDefBaseChar (pCh);
	BOOL bSpringPos = (pDungeon->byDefSpringX || pDungeon->byDefSpringY);
	int i, nCount = g_aCharInfo.GetSize (), nMember = 0, nEnemy = 0;
	size_t k;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd = CMD_DGDEFMAP;
	Para.dwMapID = pDungeon->dwMapID;
	Para.wX1 = pDungeon->byX1;
	Para.wY1 = pDungeon->byY1;
	Para.wX2 = pDungeon->byX2;
	Para.wY2 = pDungeon->byY2;
	Para.wBaseX = pBase ? (WORD)pBase->dwX : (WORD)DGDEFMAP_NONE;
	Para.wBaseY = pBase ? (WORD)pBase->dwY : (WORD)DGDEFMAP_NONE;
	/* 泉の中心：設定が両方 0 なら拠点の位置（DgDefMembers と同じ） */
	if (bSpringPos) {
		Para.wSpringX = pDungeon->byDefSpringX;
		Para.wSpringY = pDungeon->byDefSpringY;
	} else {
		Para.wSpringX = Para.wBaseX;
		Para.wSpringY = Para.wBaseY;
	}
	Para.bySpringRange = bSpringPos ? 0 : 1;						/* DgDefMembers と同じ（位置を決めた時は 1 マス） */
	/* 仲間の位置（この組の空間・同じマップにいる人） */
	for (k = 0; k < pCh->aMember.size () && nMember < 255; k ++) {
		PCHARINFO p = (pCh->aMember[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[k]] : NULL;
		DUNGEONMAPPOS pos;

		if ((p == NULL) || (p->pID == 0) || (p->dwSpaceID != pCh->dwSpaceID) || (p->dwMapID != pDungeon->dwMapID)) {
			continue;
		}
		ZeroMemory (&pos, sizeof (pos));
		pos.dwTblID = p->dwTblID;
		pos.wX = (WORD)p->dwX;
		pos.wY = (WORD)p->dwY;
		aPos.insert (aPos.end (), (PBYTE)&pos, (PBYTE)(&pos + 1));
		nMember ++;
	}
	/* 敵の位置（生きている敵だけ。座標は 0〜255 なので 1 バイトずつ） */
	for (i = 0; (i < nCount) && (nEnemy < DGDEFMAP_MAXENEMY); i ++) {
		PCHARINFO p = g_aCharInfo[i];

		if (DgDefIsEnemyOf (pCh, p) && (p->wHP > 0) && (p->dwMapID == pDungeon->dwMapID) && (p->dwX < 256) && (p->dwY < 256)) {
			aEnemy.push_back ((BYTE)p->dwX);
			aEnemy.push_back ((BYTE)p->dwY);
			nEnemy ++;
		}
	}
	Para.byMember = (BYTE)nMember;
	Para.wEnemy = (WORD)nEnemy;
	{
		std::vector<BYTE> buf (sizeof (Para) + aPos.size () + aEnemy.size ());

		CopyMemory (&buf[0], &Para, sizeof (Para));
		if (!aPos.empty ()) CopyMemory (&buf[sizeof (Para)], &aPos[0], aPos.size ());
		if (!aEnemy.empty ()) CopyMemory (&buf[sizeof (Para) + aPos.size ()], &aEnemy[0], aEnemy.size ());
		for (k = 0; k < pCh->aMember.size (); k ++) {
			PCHARINFO pTmp = (pCh->aMember[k] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pCh->aMember[k]] : NULL;

			if (pTmp && IsPlayerOnline (pTmp) && (s_mapLoan.find (pCh->aMember[k]) != s_mapLoan.end ())) {
				SendCmdDGDEFMAP (pTmp->pID, &buf[0], (DWORD)buf.size ());
			}
		}
	}
}

/* 準備時間に入る（次に来る波の知らせ） */
static void DgDefEnterPrep(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon, DWORD dwNow)
{
	int nSec = DgDefPrepSec (pDungeon);

	pCh->byDefPhase = DGDEF_PHASE_PREP;
	pCh->nEnemyLeft = -1;
	pCh->dwDefPhaseStart = dwNow;
	DgDefRecoverAll (pCh);
	DgDefSay (pCh, "def_wave_coming", "wave", std::to_string (pCh->nWave), "total", std::to_string (DgDefWaveCount (pDungeon)), "seconds", std::to_string (nSec));
}

/* 勝ち負けが決まった：敵を消して結果を知らせる（約 10 秒後に全員帰還） */
static void DgDefFinish(CHALLENGE *pCh, BYTE byResult, DWORD dwNow)
{
	pCh->byDefResult = byResult;
	pCh->dwDefEnd = dwNow ? dwNow : 1;
	DgDefRemoveEnemies (pCh);
	DgDefSay (pCh, (byResult == DGDEF_RESULT_WIN) ? "def_win" : "def_base_fallen");
	DgDefStatusSend (pCh, dwNow, NULL, TRUE);
}

/* 倒れた人の復活待ちと、泉（泉の中心の近くで Mana が戻る。中心は設定で拠点から離せる） */
static void DgDefMembers(CHALLENGE *pCh, PSUBGAME_DUNGEONINFO pDungeon, PCHARINFO pBase, DWORD dwNow)
{
	BOOL bSpring = ((int)(dwNow - pCh->dwDefSpring) >= DGDEF_SPRING_INTERVAL);
	DWORD dwSpringPct = pDungeon->byDefSpringPct ? (DWORD)pDungeon->byDefSpringPct : (DWORD)DGDEF_SPRING_PERCENT;	/* 泉で戻る割合（%） */
	/* 泉の中心：設定が両方 0 なら拠点の位置（古いデータ互換） */
	BOOL bSpringPos = (pDungeon->byDefSpringX || pDungeon->byDefSpringY);
	int nSpringX = bSpringPos ? (int)pDungeon->byDefSpringX : (pBase ? (int)pBase->dwX : 0);
	int nSpringY = bSpringPos ? (int)pDungeon->byDefSpringY : (pBase ? (int)pBase->dwY : 0);
	/* 泉の広さ：位置を決めた時はそのマス 1 つだけ。決めていない時は拠点のまわり 1 マス（拠点のマスには乗れないため）。
	   範囲の設定（byDefSpringRange）は使わない（2026-11-21 泉は 1 マスで十分に） */
	int nRange = bSpringPos ? 0 : 1;
	size_t j;

	if (bSpring) {
		pCh->dwDefSpring = dwNow;
	}
	for (j = 0; j < pCh->aMember.size (); j ++) {
		DWORD dwTbl = pCh->aMember[j];
		PCHARINFO p = (dwTbl < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[dwTbl] : NULL;

		if ((p == NULL) || (s_mapLoan.find (dwTbl) == s_mapLoan.end ())) {
			continue;
		}
		DGDEFDEAD &dd = pCh->mapDead[dwTbl];
		if ((p->wHP == 0) || (p->byCharState == CHARSTATE_DEAD)) {
			if (dd.dwReviveAt == 0) {
				/* 倒れたばかり：すぐ「入ってすぐの位置」（中の開始位置。無ければ入口＝開始位置）へ移して、倒れたまま待つ。
				   2026-11-21 拠点の横だと戦っていた場所とほぼ同じで、起きてすぐ囲まれるため変えた（プレイヤーは重なれるので何人でも同じマス）。
				   その位置がマップの外などおかしい時だけ、前と同じく拠点の横 */
				int nSec, x, y;
				DWORD dwInX, dwInY;

				dd.nCount ++;
				nSec = min (dd.nCount * DGDEF_REVIVE_STEP, DGDEF_REVIVE_MAX);
				dd.dwReviveAt = (dwNow + (DWORD)nSec * 1000) ? (dwNow + (DWORD)nSec * 1000) : 1;
				if (!DungeonFixedInside (pDungeon, &dwInX, &dwInY)) {
					dwInX = (DWORD)pDungeon->byStartX;
					dwInY = (DWORD)pDungeon->byStartY;
				}
				if ((pDungeon->dwMapID < (DWORD)g_aMapInfo.GetSize ()) && g_aMapInfo[pDungeon->dwMapID] &&
					((int)dwInX < g_aMapInfo[pDungeon->dwMapID]->nCX) && ((int)dwInY < g_aMapInfo[pDungeon->dwMapID]->nCY)) {
					SendCmdFADEINOUT (p->pID, 0);
					SubProcMapMove (p, pDungeon->dwMapID, dwInX, dwInY);
					SpaceSendMapState (p);
				} else if (pBase && DgDefFindSeat (p, pBase, x, y)) {
					SendCmdFADEINOUT (p->pID, 0);
					SubProcMapMove (p, pBase->dwMapID, (DWORD)x, (DWORD)y);
					SpaceSendMapState (p);
				}
				DgMessage (p, pCh->byDungeonID, "def_down", FALSE, "seconds", std::to_string (nSec));
			} else if ((int)(dwNow - dd.dwReviveAt) >= 0) {
				DgDefRevive (p);
				dd.dwReviveAt = 0;
				DgMessage (p, pCh->byDungeonID, "def_revived", FALSE);
				DgDefStatusSend (pCh, dwNow, p, FALSE);			/* 復活した人に帯の状態を送り直す */
			}
			continue;
		}
		dd.dwReviveAt = 0;
		/* 泉：泉のマスにいる間だけ、HP と Mana が最大値の dwSpringPct %（最低 1）ずつ戻る（2026-11-21 HP も戻すことに） */
		if (bSpring && pBase) {
			BOOL bIn = (p->dwMapID == pBase->dwMapID) &&
				(abs ((int)p->dwX - nSpringX) <= nRange) && (abs ((int)p->dwY - nSpringY) <= nRange);

			if (!bIn) {
				dd.dwSpringFx = 0;								/* 泉の外：次に入った時は最初に戻った時にエフェクトを出す */
			} else if ((p->wHP < p->wMaxHP) || (p->wMana < p->wMaxMana)) {
				DWORD dwHP = (DWORD)p->wHP, dwMana = (DWORD)p->wMana;
				int nGainHP, nGainMana;
				char szTmp[32];

				if (p->wHP < p->wMaxHP) {
					dwHP = min (dwHP + max ((DWORD)p->wMaxHP * dwSpringPct / 100, (DWORD)1), (DWORD)p->wMaxHP);
				}
				if (p->wMana < p->wMaxMana) {
					dwMana = min (dwMana + max ((DWORD)p->wMaxMana * dwSpringPct / 100, (DWORD)1), (DWORD)p->wMaxMana);
				}
				nGainHP = (int)dwHP - (int)p->wHP;
				nGainMana = (int)dwMana - (int)p->wMana;
				p->wHP = (WORD)dwHP;
				p->wMana = (WORD)dwMana;
				if (nGainHP > 0) {
					SendCmdCHARSTATUSHP (URARASOCK_SENDALL, p->dwTblID);	/* HP は周りの人の表示にも */
				}
				SendCmdCHARSTATUSHPMANA (p->dwTblID, p->dwTblID);
				/* 宿の回復と同じ見た目：増えた数の数字は毎回（HP・Mana それぞれ）、キラッとしたエフェクトは最初と 3 秒おき */
				if (nGainHP > 0) {
					wsprintf (szTmp, "%d", nGainHP);
					SendCmdADDCHAREFC (p->dwTblID, RGB (139, 213, 255), szTmp);
				}
				if (nGainMana > 0) {
					wsprintf (szTmp, "%d", nGainMana);
					SendCmdADDCHAREFC (p->dwTblID, RGB (139, 213, 255), szTmp);
				}
				if ((dd.dwSpringFx == 0) || ((int)(dwNow - dd.dwSpringFx) >= DGDEF_SPRING_FX_INTERVAL)) {
					dd.dwSpringFx = dwNow ? dwNow : 1;
					p->byEfect = EFECTTYPE_HOIMI;
					SendCmdEFECTCHG (p->pID, p->dwTblID, p->byEfect);
					p->byEfect = 0;
				}
			}
		}
	}
}

/* 拠点防衛の周期処理（約 1 秒ごと）：拠点が落ちたか・波の進み・倒れた人の復活・泉 */
static void DgDefProc(CHALLENGE &Ch, PSUBGAME_DUNGEONINFO pDungeon, DWORD dwNow)
{
	PCHARINFO pBase = DgDefBaseChar (&Ch);
	int nWaves = DgDefWaveCount (pDungeon);

	if (Ch.byDefResult != DGDEF_RESULT_NONE) {
		return;
	}
	/* 最初の周期処理：最初の波の前の準備時間に入る */
	if (Ch.byDefPhase == DGDEF_PHASE_INIT) {
		Ch.nWave = 1;
		Ch.dwDefSpring = dwNow;
		Ch.byBaseBracket = 4;
		DgDefEnterPrep (&Ch, pDungeon, dwNow);
	}
	/* 拠点が落ちた：負け */
	if ((pBase == NULL) || (pBase->wHP == 0)) {
		DgDefFinish (&Ch, DGDEF_RESULT_LOSE, dwNow);
		return;
	}
	/* 拠点が攻撃されている知らせ（HP が 25% 刻みで下がった時だけ） */
	{
		int nPct = (int)pBase->wHP * 100 / max ((int)pBase->wMaxHP, 1);
		BYTE byBracket = (BYTE)((nPct + 24) / 25);

		if (byBracket < Ch.byBaseBracket) {
			Ch.byBaseBracket = byBracket;
			DgDefSay (&Ch, "def_base_attacked", "percent", std::to_string (nPct));
		}
	}
	DgDefMembers (&Ch, pDungeon, pBase, dwNow);

	/* 準備時間が終わったら波を始める */
	if ((Ch.byDefPhase == DGDEF_PHASE_PREP) && ((int)(dwNow - Ch.dwDefPhaseStart) >= DgDefPrepSec (pDungeon) * 1000)) {
		Ch.byDefPhase = DGDEF_PHASE_WAVE;
		Ch.dwDefWaveStart = dwNow;
		DgDefSay (&Ch, "def_wave_start", "wave", std::to_string (Ch.nWave));
	}
	if (Ch.byDefPhase == DGDEF_PHASE_WAVE) {
		BOOL bLast = (Ch.nWave >= nWaves);
		BOOL bTimeout;

		DgDefPutProc (&Ch, dwNow);
		/* 波の終わり：発生元が出し切り、かつ敵が 0。長引いたら（最後の波以外は）残りがいても次へ */
		bTimeout = !bLast && ((int)(dwNow - Ch.dwDefWaveStart) >= DgDefWaveMaxSec (pDungeon) * 1000);
		if ((DgDefWaveExhausted (&Ch, pDungeon) && (DgDefEnemyCount (&Ch) == 0)) || bTimeout) {
			DgDefSay (&Ch, "def_wave_clear", "wave", std::to_string (Ch.nWave));
			if (bLast) {
				DgDefFinish (&Ch, DGDEF_RESULT_WIN, dwNow);
			} else {
				Ch.nWave ++;
				DgDefEnterPrep (&Ch, pDungeon, dwNow);
			}
		} else {
			/* 敵が倒されて残りが減った：組の全員に残り敵を知らせる（1 秒の間に何体倒れても最後の数を 1 回。0 の時は上の「しのいだ」が出る） */
			int nLeft = DgDefEnemyLeft (&Ch, pDungeon);

			if ((Ch.nEnemyLeft >= 0) && (nLeft < Ch.nEnemyLeft) && (nLeft > 0)) {
				DgDefSay (&Ch, "def_enemy_left", "count", std::to_string (nLeft));
			}
			Ch.nEnemyLeft = nLeft;
		}
	}
	/* 画面の帯：変わった時（準備中は残り秒が変わるので1秒ごと）と、一定時間ごとに送る */
	DgDefStatusSend (&Ch, dwNow, NULL, FALSE);
	/* 全体地図：敵・仲間の位置を毎回（約 1 秒ごと）組の全員へ */
	DgDefMapSend (&Ch, pDungeon);
}

/* 周期処理（約 1 秒ごと）：選ぶ締め切り・中の人が出た判定・空間ごとの敵の補充・空になった空間を空きにする */
static void DgProc(DWORD dwNow)
{
	std::vector<DWORD> aKey, aEnd;
	std::map<DWORD, DGSEL>::iterator itS;
	std::map<DWORD, CHALLENGE>::iterator itC;
	size_t i, j;

    /* マップ登録を通らない状態変更・切断も、締め切りの確定より先に取り消す。 */
    std::vector<DWORD> selectingMembers;
    for (std::map<DWORD, DWORD>::iterator it = s_mapDgSelOf.begin (); it != s_mapDgSelOf.end (); it ++)
        selectingMembers.push_back (it->first);
    for (size_t k = 0; k < selectingMembers.size (); k ++) {
        DWORD id = selectingMembers[k];
        DungeonReceptionCheck (id < (DWORD)g_aCharInfo.GetSize () ? g_aCharInfo[id] : NULL);
    }

	/* 選んでいる最中の組に、入口の近くへ来た同じパーティの仲間を足す */
	for (itS = s_mapDgSel.begin (); itS != s_mapDgSel.end (); itS ++) {
		PCHARINFO pKey = (itS->first < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[itS->first] : NULL;
		std::vector<PCHARINFO> aParty;
		if ((pKey == NULL) || (dwNow - itS->second.dwStart >= DGSEL_WAIT)) {
			continue;
		}
		GetPartyMembers (pKey, aParty);
		for (i = 0; i < aParty.size (); i ++) {
			DgSelJoin (itS->first, aParty[i], dwNow, FALSE);
		}
	}
	/* 姿を選ぶ締め切り（選ばなかった人は置いていく） */
	for (itS = s_mapDgSel.begin (); itS != s_mapDgSel.end (); itS ++) {
		if (dwNow - itS->second.dwStart >= DGSEL_WAIT) {
			aKey.push_back (itS->first);
		}
	}
	for (i = 0; i < aKey.size (); i ++) {
		DgSelFinish (aKey[i]);
	}

    /* 受付済みの期限・距離・パーティ・生存状態を確認する。 */
    std::vector<DWORD> preparedKeys;
    for (std::map<DWORD, DGSEL>::iterator it = s_mapDgPrepared.begin (); it != s_mapDgPrepared.end (); it ++)
        preparedKeys.push_back (it->first);
    for (size_t k = 0; k < preparedKeys.size (); k ++) DgPreparedValid (preparedKeys[k], dwNow);

	/* 中の組 */
	for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
		CHALLENGE &Ch = itC->second;
		PSUBGAME_DUNGEONINFO pDungeon;
		std::vector<DWORD> aMember;

		if (Ch.byKind != CHALLENGEKIND_DUNGEON) {
			continue;
		}
		pDungeon = DgInfo (Ch.byDungeonID);
		aMember = Ch.aMember;
		/* 拠点防衛：結果を知らせてから 10 秒後に全員（倒れている人も）戻す。それまで途中の判定はしない */
		if (Ch.bDefense && (Ch.byDefResult != DGDEF_RESULT_NONE)) {
			if ((int)(dwNow - Ch.dwDefEnd) >= DGDEF_RESULT_WAIT) {
				for (j = 0; j < aMember.size (); j ++) {
					PCHARINFO p = g_aCharInfo[aMember[j]];
					if (s_mapLoan.find (aMember[j]) != s_mapLoan.end ()) {
						ChallengeLeave (p, (Ch.byDefResult == DGDEF_RESULT_WIN) ? CHALLENGELEAVE_CLEAR : CHALLENGELEAVE_QUEST);
					}
				}
			}
			if (Ch.aMember.empty ()) {
				aEnd.push_back (itC->first);
			}
			continue;
		}
		/* クリア済み（敵を全部倒した）：3・2・1 を数えて、10 秒後に全員（倒れている人も）戻す。それまで途中の判定はしない */
		if (Ch.bKillAll && Ch.bCleared) {
			DWORD dwEl = dwNow - Ch.dwClear;
			DgStatusSend (&Ch, dwNow);
			int nLeft = 10 - (int)(dwEl / 1000);
			if (dwEl >= 10000) {
				for (j = 0; j < aMember.size (); j ++) {
					PCHARINFO p = g_aCharInfo[aMember[j]];
					if (s_mapLoan.find (aMember[j]) != s_mapLoan.end ()) {
						ChallengeLeave (p, CHALLENGELEAVE_CLEAR);
					}
				}
			} else if ((nLeft <= 3) && (nLeft >= 1) && (nLeft < (int)Ch.byCountDown)) {
				char szCnt[16];
				Ch.byCountDown = (BYTE)nLeft;
				DgNoticeMembers (&Ch, DungeonText (pDungeon, "countdown", "seconds", std::to_string (nLeft)).c_str ());
			}
			if (Ch.aMember.empty ()) {
				aEnd.push_back (itC->first);
			}
			continue;
		}
		/* 倒れた人は、ふつうの世界と同じく「このまま待つ／帰る」を選べる（帰るは蘇生の受信で外へ）。
		   中の全員が倒れたら、もう誰も起こせないので全員まとめて外へ（全滅） */
		BOOL bAllDead = TRUE;
		for (j = 0; j < aMember.size (); j ++) {
			PCHARINFO p = g_aCharInfo[aMember[j]];
			if ((s_mapLoan.find (aMember[j]) != s_mapLoan.end ()) && (p->wHP > 0) && (p->byCharState != CHARSTATE_DEAD)) {
				bAllDead = FALSE;
				break;
			}
		}
		for (j = 0; j < aMember.size (); j ++) {
			PCHARINFO p = g_aCharInfo[aMember[j]];
			int nReason = -1;

			if (s_mapLoan.find (aMember[j]) == s_mapLoan.end ()) {
				continue;
			}
			if ((p->wHP == 0) || (p->byCharState == CHARSTATE_DEAD)) {
				/* 拠点防衛は全員倒れても拠点の横で復活を待つ（全滅では終わらない） */
				if (bAllDead && !Ch.bDefense) {
					nReason = CHALLENGELEAVE_ALLDEAD;
				}
			} else if (p->dwPartyID != Ch.dwPartyID) {
				nReason = CHALLENGELEAVE_PARTY;
			} else if ((pDungeon == NULL) || (pDungeon->dwBuildNo != Ch.dwBuildNo)) {
				nReason = CHALLENGELEAVE_BUILD;
			} else if (!DgInArea (pDungeon, p->dwMapID, p->dwX, p->dwY)) {
				nReason = CHALLENGELEAVE_OUT;
			}
			if (nReason >= 0) {
				ChallengeLeave (p, nReason);
			}
		}
		if (Ch.aMember.empty ()) {
			aEnd.push_back (itC->first);
		} else {
			if (Ch.bDefense) {
				if (pDungeon != NULL) {
					DgDefProc (Ch, pDungeon, dwNow);
				}
			} else {
				DgPutProc (&Ch, dwNow, FALSE);
				DgStatusSend (&Ch, dwNow);
			}
			if (Ch.bMapShared) {
				std::vector<BYTE> pos = DgMapPositions (&Ch);
				if (pos != Ch.aMapPositions) {
					Ch.aMapPositions = pos;
					DgMapSend (&Ch, NULL, std::vector<WORD> ());
				}
			}
		}
	}
	for (i = 0; i < aEnd.size (); i ++) {
		DgEnd (aEnd[i]);
	}
}

/* 今すぐ周期処理をする（テスト用。時刻を指定できる） */
void DungeonProcNow(DWORD dwNow)
{
	DgProc (dwNow);
}

/* 組の敵が発生できる範囲。生成前・範囲変更後の古い地形には出さない */
BOOL DungeonPutPositionAllowed(PCHARINFO pParent, DWORD dwX, DWORD dwY)
{
	std::map<DWORD, CHALLENGE>::iterator it = s_mapChallenge.find (pParent->dwSpaceID);
	if ((it == s_mapChallenge.end ()) || (it->second.byKind != CHALLENGEKIND_DUNGEON)) return TRUE;
	PSUBGAME_DUNGEONINFO p = DgInfo (it->second.byDungeonID);
	return p && DgReady (p) && DgInArea (p, pParent->dwMapID, dwX, dwY);
}

/* NPC発生の発生元が、番外編ダンジョンの範囲の中にあるか（ふつうの世界には出さず、組の空間ごとに出す） */
BOOL DungeonPutIsSpaced(PCHARINFO pParent)
{
	return (pParent != NULL) && (pParent->byNpcType == NPCTYPE_PUT) && (pParent->dwSpaceID == SPACE_NORMAL) &&
			(DgFindByArea (pParent->dwMapID, pParent->dwX, pParent->dwY) >= 0);
}

/* 拠点防衛の組の空間か、その空間の拠点キャラ（組が終わっていれば NULL。敵の行進を作る側が使う） */
PCHARINFO DgDefenseBaseOf(DWORD dwSpaceID)
{
	std::map<DWORD, CHALLENGE>::iterator it;

	if (dwSpaceID == SPACE_NORMAL) {
		return NULL;
	}
	it = s_mapChallenge.find (dwSpaceID);
	if ((it == s_mapChallenge.end ()) || (it->second.byKind != CHALLENGEKIND_DUNGEON) || !it->second.bDefense) {
		return NULL;
	}
	return DgDefBaseChar (&it->second);
}

/* 拠点防衛の組の空間で出た敵か（NPC発生から出たキャラ。拠点・傭兵・矢は含まない） */
BOOL DgIsDefenseEnemy(PCHARINFO pChar)
{
	std::map<DWORD, CHALLENGE>::iterator it;

	if ((pChar == NULL) || (pChar->dwSpaceID == SPACE_NORMAL)) {
		return FALSE;
	}
	it = s_mapChallenge.find (pChar->dwSpaceID);
	if ((it == s_mapChallenge.end ()) || (it->second.byKind != CHALLENGEKIND_DUNGEON) || !it->second.bDefense) {
		return FALSE;
	}
	return DgDefIsEnemyOf (&it->second, pChar);
}

/* 拠点防衛の組の空間にいる、その組の拠点キャラか */
BOOL DgIsDefenseBase(PCHARINFO pChar)
{
	return (pChar != NULL) && (pChar->byNpcType == NPCTYPE_NOMOVE) && (DgDefenseBaseOf (pChar->dwSpaceID) == pChar);
}

/*
 * 拠点防衛：敵の行進用の道しるべ（拠点までの歩数表）
 * 拠点から幅優先で、通れるマスの歩数を組の空間ごとに 1 つ作って全員で使う（敵ごとには作らない）。
 * 地形・イベント・開いた扉（組の空間ごとに開閉が違う）・ダメージ床だけを見る。人や敵が立っているマスは
 * 表には入れず、敵が 1 歩ずつ自分で確かめる。範囲は番外編の範囲の中だけ。数秒ごとに作り直す。
 */
#define DGDEF_FIELD_LIFE	(3000)		/* 表を作り直す間隔(ms) */
#define DGDEF_FIELD_MINGAP	(1000)		/* 詰まって引き直しを頼まれても、作り直さない最短の間隔(ms) */

typedef struct _DGDEFFIELD {
	DWORD				dwBuilt;			/* 作った時刻（0 なら次に使う時に作り直す） */
	int					nX1, nY1, nW, nH;	/* 表の範囲（マスの座標・大きさ） */
	std::vector<short>	aDist;				/* 拠点までの歩数（通れない・行けないマスは -1） */
} DGDEFFIELD;
static std::map<DWORD, DGDEFFIELD> s_mapDefField;

/* 組が終わった時に表を捨てる */
static void DgDefFieldDrop(DWORD dwSpaceID)
{
	s_mapDefField.erase (dwSpaceID);
}

/* そのマスが敵の通れるマスか（番外編の範囲の中の、地形・イベント。閉じていない扉は組の空間ごとの開閉で見る。
   pChar が NULL なら地形はベースの判定） */
BOOL DgDefCellOpen(PCHARINFO pChar, DWORD dwSpaceID, DWORD dwMapID, int x, int y)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	PSUBGAME_DUNGEONINFO pDungeon;
	PMAPINFO pMap;
	PEVENTINFO pEvent;
	BOOL bTerrain, bDoorOpen = FALSE;
	int i;

	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return FALSE;
	}
	/* 範囲の外へは行かない（範囲の外に床があっても、扉を避けて回り込まない） */
	itC = s_mapChallenge.find (dwSpaceID);
	if (itC != s_mapChallenge.end ()) {
		pDungeon = DgInfo (itC->second.byDungeonID);
		if ((pDungeon != NULL) && !DgInArea (pDungeon, dwMapID, (DWORD)x, (DWORD)y)) {
			return FALSE;
		}
	}
	pMap = g_aMapInfo[dwMapID];
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}
	bTerrain = pChar ? CheckCharBlock (pChar, dwMapID, (DWORD)x, (DWORD)y) : CheckCharBlock (dwMapID, (DWORD)x, (DWORD)y);
	for (i = 0; i < pMap->nEventCount; i ++) {
		pEvent = &pMap->pEvent[i];
		if ((pEvent->dwX != (DWORD)x) || (pEvent->dwY != (DWORD)y)) {
			continue;
		}
		/* 開いている扉は通れる（サーバーのマップは閉じた絵のままなので、扉の情報で見る）。それ以外のイベントは通れない */
		if ((pEvent->byType == EVENTTYPE_DOOR) && pEvent->pData && SpaceDoorIsOpen (dwMapID, pEvent, (PEVENTINFO_DOOR)pEvent->pData, dwSpaceID)) {
			bDoorOpen = TRUE;
			continue;
		}
		return FALSE;
	}
	return bTerrain || bDoorOpen;
}

/* 組の空間の歩数表（古ければ作り直す）。防衛でない・拠点がない・範囲の外なら NULL */
static DGDEFFIELD *DgDefField(PCHARINFO pEnemy, DWORD dwNow)
{
	std::map<DWORD, CHALLENGE>::iterator it;
	PSUBGAME_DUNGEONINFO pDungeon;
	PCHARINFO pBase;
	PMAPINFO pMap;
	DGDEFFIELD *pF;
	std::vector<int> aQueue;
	size_t nHead = 0;
	int nX2, nY2, nPos, i, x, y;

	if ((pEnemy == NULL) || (pEnemy->dwSpaceID == SPACE_NORMAL)) {
		return NULL;
	}
	it = s_mapChallenge.find (pEnemy->dwSpaceID);
	if ((it == s_mapChallenge.end ()) || (it->second.byKind != CHALLENGEKIND_DUNGEON) || !it->second.bDefense) {
		return NULL;
	}
	pDungeon = DgInfo (it->second.byDungeonID);
	pBase = DgDefBaseChar (&it->second);
	if ((pDungeon == NULL) || (pBase == NULL) || (pBase->dwMapID != pDungeon->dwMapID) || (pBase->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return NULL;
	}
	pF = &s_mapDefField[pEnemy->dwSpaceID];
	if ((pF->dwBuilt != 0) && ((int)(dwNow - pF->dwBuilt) < DGDEF_FIELD_LIFE) && (pF->nW > 0)) {
		return pF;
	}

	pMap = g_aMapInfo[pBase->dwMapID];
	pF->nX1 = max ((int)pDungeon->byX1, 0);
	pF->nY1 = max ((int)pDungeon->byY1, 0);
	nX2 = min ((int)pDungeon->byX2, pMap->nCX - 1);
	nY2 = min ((int)pDungeon->byY2, pMap->nCY - 1);
	pF->nW = max (nX2 - pF->nX1 + 1, 0);
	pF->nH = max (nY2 - pF->nY1 + 1, 0);
	pF->aDist.assign ((size_t)pF->nW * pF->nH, -1);
	pF->dwBuilt = dwNow ? dwNow : 1;
	x = (int)pBase->dwX - pF->nX1;
	y = (int)pBase->dwY - pF->nY1;
	if ((x < 0) || (x >= pF->nW) || (y < 0) || (y >= pF->nH)) {
		return pF;
	}
	/* 拠点のマスから幅優先。拠点のマス自身は歩数 0（人が立っていても出発点） */
	nPos = x + y * pF->nW;
	pF->aDist[nPos] = 0;
	aQueue.push_back (nPos);
	while (nHead < aQueue.size ()) {
		nPos = aQueue[nHead ++];
		for (i = 0; i < 4; i ++) {
			x = nPos % pF->nW + anPosAddTblX[i];
			y = nPos / pF->nW + anPosAddTblY[i];
			if ((x < 0) || (x >= pF->nW) || (y < 0) || (y >= pF->nH) || (pF->aDist[x + y * pF->nW] != -1)) {
				continue;
			}
			if (!DgDefCellOpen (NULL, pEnemy->dwSpaceID, pBase->dwMapID, pF->nX1 + x, pF->nY1 + y) ||
				IsDamageFloor (pMap, pF->nX1 + x, pF->nY1 + y)) {
				continue;
			}
			pF->aDist[x + y * pF->nW] = (short)(pF->aDist[nPos] + 1);
			aQueue.push_back (x + y * pF->nW);
		}
	}
	return pF;
}

/* 敵の今いるマスと上下左右（向き 0〜3 の順）の、拠点までの歩数。anDist[0..3] が上下左右、anDist[4] が今のマス。分からない・通れないマスは -1。
   表が無い（防衛でない・拠点がない）時は FALSE */
BOOL DgDefMarchDists(PCHARINFO pEnemy, int *panDist)
{
	DGDEFFIELD *pF = DgDefField (pEnemy, timeGetTime ());
	int i, x, y;

	if (pF == NULL) {
		return FALSE;
	}
	for (i = 0; i < 5; i ++) {
		x = (int)pEnemy->dwX - pF->nX1 + ((i < 4) ? anPosAddTblX[i] : 0);
		y = (int)pEnemy->dwY - pF->nY1 + ((i < 4) ? anPosAddTblY[i] : 0);
		panDist[i] = ((x >= 0) && (x < pF->nW) && (y >= 0) && (y < pF->nH)) ? (int)pF->aDist[x + y * pF->nW] : -1;
	}
	return TRUE;
}

/* 進めず詰まった敵が、歩数表の引き直しを頼む（1 秒に 1 回まで） */
void DgDefMarchRefresh(DWORD dwSpaceID)
{
	std::map<DWORD, DGDEFFIELD>::iterator it = s_mapDefField.find (dwSpaceID);

	if ((it != s_mapDefField.end ()) && ((int)(timeGetTime () - it->second.dwBuilt) >= DGDEF_FIELD_MINGAP)) {
		it->second.dwBuilt = 0;
	}
}

/* 空間の中で発生した敵が消えた（倒された）。その組の数を減らす。空間の敵なら TRUE（発生元の数は動かさない） */
BOOL DungeonPutOnChildDec(PCHARINFO pChild)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	PCHARINFO pParent;

	if ((pChild->dwSpaceID == SPACE_NORMAL) || (pChild->dwParentTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return FALSE;
	}
	pParent = g_aCharInfo[pChild->dwParentTblID];
	if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT)) {
		return FALSE;
	}
	itC = s_mapChallenge.find (pChild->dwSpaceID);
	if ((itC != s_mapChallenge.end ()) && (itC->second.byKind == CHALLENGEKIND_DUNGEON)) {
		std::map<DWORD, DGPUT>::iterator itP = itC->second.mapPut.find (pParent->dwTblID);
		if (itP != itC->second.mapPut.end ()) {
			if (itP->second.nNow > 0) {
				itP->second.nNow --;
			}
			itP->second.dwLast = timeGetTime ();
		}
		/* クリア条件「敵を全部倒す」：1 体倒すたびに残りを知らせる。0 になったらクリア（その瞬間にクリア時間を決める） */
		CHALLENGE &Ch = itC->second;
		if (Ch.bKillAll && !Ch.bCleared && pParent->byPutOnce && !Ch.aMember.empty ()) {
			int nRemain = DgRemain (&Ch);
			char szTmp[256];
			if (nRemain > 0) {
				DgNoticeMembers (&Ch, DungeonText (DgInfo (Ch.byDungeonID), "remaining", "count", std::to_string (nRemain)).c_str ());
			} else {
				std::string strClear = DgRecordClear (&Ch, TRUE);
				Ch.byCountDown = 4;
				DgNoticeMembers (&Ch, DungeonText (DgInfo (Ch.byDungeonID), "killall_clear", "result", strClear).c_str ());
			}
		}
	}
	return TRUE;
}

/* 番外編の中で増えた品の数（今の持ち物 − 入った時のテンプレートの持ち物。テスト用） */
int DungeonBagCount(PCHARINFO pChar)
{
	std::map<DWORD, LOANBAK>::iterator itL;
	std::vector<WORD> aBag;

	if (DgRunOf (pChar) == NULL) {
		return 0;
	}
	itL = s_mapLoan.find (pChar->dwTblID);
	DgBagCollect (pChar, itL->second.wDgTmpl, aBag);
	return (int)aBag.size ();
}

/* ダンジョンの空間（組）の数・姿を選んでいる組の数（テスト用） */
int DungeonRunCount(void)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	int nCount = 0;

	for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
		if (itC->second.byKind == CHALLENGEKIND_DUNGEON) {
			nCount ++;
		}
	}
	return nCount;
}

int DungeonSelCount(void)
{
	return (int)s_mapDgSel.size ();
}

/* 姿を選んでいる組の人数（pChar がいる組。いなければ 0。テスト用） */
int DungeonSelMemberCount(PCHARINFO pChar)
{
	std::map<DWORD, DWORD>::iterator itOf = s_mapDgSelOf.find (pChar->dwTblID);
	std::map<DWORD, DGSEL>::iterator itS;

	if (itOf == s_mapDgSelOf.end ()) {
		return 0;
	}
	itS = s_mapDgSel.find (itOf->second);
	return (itS == s_mapDgSel.end ()) ? 0 : (int)itS->second.aMember.size ();
}

/* そのダンジョンの中の組の全員に知らせて、外へ戻す（管理者の作り直し。CHALLENGELEAVE_BUILD）。戻した人数 */
int DungeonRunLeaveAll(int nDungeonID, const char *pszMsg)
{
	std::map<DWORD, CHALLENGE>::iterator itC;
	std::vector<DWORD> aTbl;
	size_t i;
	int nCount = 0;

	for (itC = s_mapChallenge.begin (); itC != s_mapChallenge.end (); itC ++) {
		if ((itC->second.byKind == CHALLENGEKIND_DUNGEON) && (itC->second.byDungeonID == (BYTE)nDungeonID)) {
			aTbl.insert (aTbl.end (), itC->second.aMember.begin (), itC->second.aMember.end ());
		}
	}
	for (i = 0; i < aTbl.size (); i ++) {
		PCHARINFO p = (aTbl[i] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[aTbl[i]] : NULL;
		if ((p == NULL) || (s_mapLoan.find (aTbl[i]) == s_mapLoan.end ())) {
			continue;
		}
		if (pszMsg && pszMsg[0] && IsPlayerOnline (p)) {
			SendCmdSYSMSG (p->pID, (LPSTR)pszMsg, SPEECHCMD_BATTLE1);
		}
		ChallengeLeave (p, CHALLENGELEAVE_BUILD);
		nCount ++;
	}
	return nCount;
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
		} else if (itC->second.byKind == CHALLENGEKIND_DUNGEON) {
			SboTextCopy (p->szQuest, sizeof (p->szQuest), "ダンジョン（番外編）");
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

/* 扉をその空間で開ける（種別は見ない） */
static BOOL SpaceDoorOpenRaw(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
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

/* 扉をその空間で開ける（プレイヤー・傭兵が開ける時。「敵を全部倒す」扉は手では開けられない。
   「鍵」の扉は鍵を持っているか確かめた後で SpaceDoorOpenKey を使う（傭兵は鍵を持たないので開けられない）） */
BOOL SpaceDoorOpen(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	if ((pDoor == NULL) || (pDoor->byType == EVENTDOOTTYPE_ENEMY) || (pDoor->byType == EVENTDOOTTYPE_KEY)) {
		return FALSE;
	}
	return SpaceDoorOpenRaw (dwMapID, pEvent, pDoor, dwSpaceID);
}

/* 「鍵」の扉を、鍵を持っている人が開ける（持っているかは呼ぶ側で確かめる。鍵は減らない） */
BOOL SpaceDoorOpenKey(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	if ((pDoor == NULL) || (pDoor->byType != EVENTDOOTTYPE_KEY)) {
		return FALSE;
	}
	return SpaceDoorOpenRaw (dwMapID, pEvent, pDoor, dwSpaceID);
}

/* 扉をその空間で閉める */
static void SpaceDoorClose(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	if (dwSpaceID == SPACE_NORMAL) {
		pDoor->dwTimeOpen = 0;
	} else {
		s_mapSpaceDoor.erase (SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, dwSpaceID));
	}
	SendCmdCHGMAPSpace (dwMapID, dwSpaceID, pDoor->wGrpID, (int)pEvent->dwX, (int)pEvent->dwY);
}

/* ========================================================================= */
/* 敵を全部倒すと開く扉															 */
/* ========================================================================= */
/*
 * 扉の「敵を数える範囲」（全部0ならマップ全体）に、その空間の生きている敵がいなくなったら開き、
 * 敵がいない間は開きっぱなし。敵がまた出てきたら閉まる（扉の上に誰か立っている間は閉めない）。
 * プレイヤーの扉コマンド・傭兵では開けられない。空間ごとに別々に開け閉めする。
 */

#define ENEMYDOOR_CHECKTIME		(500)							/* ふだんの見回りの間隔(ms) */
#define ENEMYDOOR_NOTIFYRANGE	(12)							/* 範囲の外でも扉からこのマス以内の人には知らせる */

/* 数える敵か（生きている戦闘1・戦闘2・宝箱の敵） */
static BOOL IsDoorEnemy(PCHARINFO pChar)
{
	if ((pChar == NULL) || IsQuestNpc (pChar)) {
		return FALSE;
	}
	if ((pChar->byNpcType != NPCTYPE_BATTLE1) && (pChar->byNpcType != NPCTYPE_BATTLE2) && (pChar->byNpcType != NPCTYPE_MIMIC)) {
		return FALSE;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return FALSE;
	}
	return TRUE;
}

/* 扉の数える範囲の中か */
static BOOL IsInDoorArea(PEVENTINFO_DOOR pDoor, DWORD dwX, DWORD dwY)
{
	DWORD dwX1, dwY1, dwX2, dwY2;

	if ((pDoor->wAreaX1 == 0) && (pDoor->wAreaY1 == 0) && (pDoor->wAreaX2 == 0) && (pDoor->wAreaY2 == 0)) {
		return TRUE;	/* マップ全体 */
	}
	dwX1 = min (pDoor->wAreaX1, pDoor->wAreaX2);
	dwX2 = max (pDoor->wAreaX1, pDoor->wAreaX2);
	dwY1 = min (pDoor->wAreaY1, pDoor->wAreaY2);
	dwY2 = max (pDoor->wAreaY1, pDoor->wAreaY2);
	return (dwX >= dwX1) && (dwX <= dwX2) && (dwY >= dwY1) && (dwY <= dwY2);
}

/* 扉の範囲にいる、その空間の生きている敵の数 */
int SpaceDoorEnemyCount(DWORD dwMapID, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID)
{
	PMAPINFO pMap;
	int i, nCount, nRet = 0;

	int nMapNo = GetMapNo (dwMapID);
	if ((pDoor == NULL) || (nMapNo < 0)) {
		return 0;
	}
	pMap = g_aMapInfo[nMapNo];
	if (pMap->paTblNoNPC == NULL) {
		return 0;
	}
	nCount = (int)pMap->dwCountTblNoNPC;
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pCharTmp;
		if (pMap->paTblNoNPC[i] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pMap->paTblNoNPC[i]];
		if ((pCharTmp->dwSpaceID != dwSpaceID) || !IsDoorEnemy (pCharTmp)) {
			continue;
		}
		if (!IsInDoorArea (pDoor, pCharTmp->dwX, pCharTmp->dwY)) {
			continue;
		}
		nRet ++;
	}
	return nRet;
}

/* 扉のマスに、その空間の誰か（倒れていない人・ぶつかるNPC）が立っているか */
static BOOL IsCharOnDoor(PMAPINFO pMap, PEVENTINFO pEvent, DWORD dwSpaceID)
{
	int t, i;

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
			if (pCharTmp->dwSpaceID != dwSpaceID) {
				continue;
			}
			if (t == 0) {
				if ((pCharTmp->wHP == 0) || (pCharTmp->byGameState < GAMESTATE_MOVE)) {
					continue;
				}
			} else {
				if (IsQuestNpc (pCharTmp) ||
					(pCharTmp->byNpcType == NPCTYPE_PUT) || (pCharTmp->byNpcType == NPCTYPE_USEMAGIC) ||
					(pCharTmp->byNpcType == NPCTYPE_ATACK) || (pCharTmp->byNpcType == NPCTYPE_USEMAGICPARENT)) {
					continue;
				}
			}
			if (CheckCharX (pCharTmp, pCharTmp->dwX, pEvent->dwX) && (pCharTmp->dwY == pEvent->dwY)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* 扉の近く（数える範囲の中か、扉から少しの所）にいる、その空間の人に知らせる
   隣り合う扉が同時に開閉する時、同じ文を何度も出さない（同じ人・同じ文は、ほかの扉から1秒以内に出したばかりなら出さない） */
static void EnemyDoorNotify(PMAPINFO pMap, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, DWORD dwSpaceID, LPCSTR pszMsg)
{
	int i, nCount;

	if (pMap->paTblNoPC == NULL) {
		return;
	}
	nCount = (int)pMap->dwCountTblNoPC;
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pCharTmp;
		if (pMap->paTblNoPC[i] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pMap->paTblNoPC[i]];
		if ((pCharTmp->pID == 0) || (pCharTmp->byGameState < GAMESTATE_MOVE) || (pCharTmp->dwSpaceID != dwSpaceID)) {
			continue;
		}
		if (!IsInDoorArea (pDoor, pCharTmp->dwX, pCharTmp->dwY) &&
			((abs ((int)pCharTmp->dwX - (int)pEvent->dwX) > ENEMYDOOR_NOTIFYRANGE) ||
			 (abs ((int)pCharTmp->dwY - (int)pEvent->dwY) > ENEMYDOOR_NOTIFYRANGE))) {
			continue;
		}
		{
			static std::map<std::pair<DWORD, std::string>, std::pair<DWORD, PEVENTINFO> > s_mapNotify;
			std::pair<DWORD, std::string> key (pCharTmp->pID, pszMsg);
			std::map<std::pair<DWORD, std::string>, std::pair<DWORD, PEVENTINFO> >::iterator itN = s_mapNotify.find (key);
			if ((itN != s_mapNotify.end ()) && (itN->second.second != pEvent) && (g_dwTime - itN->second.first < 1000)) {
				continue;
			}
			s_mapNotify[key] = std::make_pair (g_dwTime, pEvent);
			if (s_mapNotify.size () > 256) {
				s_mapNotify.clear ();
			}
		}
		SendCmdPLAYSOUND (pCharTmp->pID, SOUND_DOOR);
		SendCmdSYSMSG (pCharTmp->pID, (LPTSTR)pszMsg, SPEECHCMD_BATTLE1);
	}
}

/* 敵を全部倒すと開く扉の、開け閉めを見る（bNow なら間隔を待たずに今すぐ） */
static void EnemyDoorProc(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor, BOOL bNow)
{
	PMAPINFO pMap;
	std::set<DWORD> aSpace;
	std::set<DWORD>::iterator itS;
	int i, nCount;

	int nMapNo = GetMapNo (dwMapID);
	if (nMapNo < 0) {
		return;
	}
	if (!bNow && pDoor->dwCheckTime && (g_dwTime - pDoor->dwCheckTime < ENEMYDOOR_CHECKTIME)) {
		return;
	}
	pDoor->dwCheckTime = g_dwTime ? g_dwTime : 1;
	pMap = g_aMapInfo[nMapNo];

	/* 見る空間：ふつうの世界・このマップにいる人の空間・開いたままの空間 */
	aSpace.insert (SPACE_NORMAL);
	if (pMap->paTblNoPC) {
		nCount = (int)pMap->dwCountTblNoPC;
		for (i = 0; i < nCount; i ++) {
			if (pMap->paTblNoPC[i] == -1) {
				break;
			}
			aSpace.insert (g_aCharInfo[pMap->paTblNoPC[i]]->dwSpaceID);
		}
	}
	if (!s_mapSpaceDoor.empty ()) {
		std::map<SPACEKEY, DWORD>::iterator it = s_mapSpaceDoor.lower_bound (SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, 0));
		while ((it != s_mapSpaceDoor.end ()) &&
				(std::get<0> (it->first) == dwMapID) && (std::get<1> (it->first) == pEvent->dwX) && (std::get<2> (it->first) == pEvent->dwY)) {
			aSpace.insert (std::get<3> (it->first));
			it ++;
		}
	}

	for (itS = aSpace.begin (); itS != aSpace.end (); itS ++) {
		DWORD dwSpaceID = *itS;
		BOOL bOpen = SpaceDoorIsOpen (dwMapID, pEvent, pDoor, dwSpaceID);

		/* 終わった挑戦の空間は片付ける */
		if (!SpaceExists (dwSpaceID)) {
			if (bOpen) {
				s_mapSpaceDoor.erase (SPACEKEY (dwMapID, pEvent->dwX, pEvent->dwY, dwSpaceID));
			}
			continue;
		}
		int nEnemy = SpaceDoorEnemyCount (dwMapID, pDoor, dwSpaceID);
		if ((nEnemy == 0) && !bOpen) {
			/* 全部倒した */
			SpaceDoorOpenRaw (dwMapID, pEvent, pDoor, dwSpaceID);
			EnemyDoorNotify (pMap, pEvent, pDoor, dwSpaceID, "魔物の気配が消え、扉が開いた！");
		} else if ((nEnemy > 0) && bOpen) {
			/* 敵がまた出てきた（扉の上に誰かいる間は待つ） */
			if (IsCharOnDoor (pMap, pEvent, dwSpaceID)) {
				continue;
			}
			SpaceDoorClose (dwMapID, pEvent, pDoor, dwSpaceID);
			EnemyDoorNotify (pMap, pEvent, pDoor, dwSpaceID, "魔物の気配がして、扉が閉ざされた…");
		}
	}
}

/* そのマップの「敵を全部倒す」扉を今すぐ見直す（敵が倒れて消えた時） */
void SpaceEnemyDoorCheckMap(DWORD dwMapID)
{
	PMAPINFO pMap;
	int i;

	int nMapNo = GetMapNo (dwMapID);
	if (nMapNo < 0) {
		return;
	}
	pMap = g_aMapInfo[nMapNo];
	for (i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pEvent = &pMap->pEvent[i];
		if ((pEvent->byType != EVENTTYPE_DOOR) || (pEvent->pData == NULL)) {
			continue;
		}
		PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEvent->pData;
		if (pDoor->byType == EVENTDOOTTYPE_ENEMY) {
			EnemyDoorProc (dwMapID, pEvent, pDoor, TRUE);
		}
	}
}

/* 扉が閉まる時間を見る */
void SpaceDoorProc(DWORD dwMapID, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor)
{
	if (pDoor == NULL) {
		return;
	}
	/* 敵を全部倒すと開く扉は、時間では閉めない */
	if (pDoor->byType == EVENTDOOTTYPE_ENEMY) {
		EnemyDoorProc (dwMapID, pEvent, pDoor, FALSE);
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
	/* 戦闘で列を離れて戻る途中（追いかけ中）の覚えも消す。残ると別の空間の先頭を追いかけ続けて本人が動けなくなる */
	TailReturnForget (pChar->dwTblID);

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
