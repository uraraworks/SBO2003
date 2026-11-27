/* ========================================================================= */
/* ファイル名：	SboSvrJan.cpp												 */
/* 概要：		SBOじゃんの卓（台NPC・4 席・CPU・ゲームの進行）の処理(実装ファイル) */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 仕様は docs/janjan.md。盤面ロジックは common/Janjan.h（通信・時計を知らない）。落ち物パズル（SboSvrPuzzle.cpp）の作りを写している。
 * 台は、台詞が "@j<サブゲーム情報の番号>" の NPC。設定（チケットのアイテム番号・枚数・CPU強さ）はサブゲーム情報 SUBGAME_JANJANINFO（SUBGAMEID_JANJAN）から読む。
 *   番号の指す情報が無い・種別が違う時は、無料・強さ 2 の台。旧形式 "@j<コイン番号>,<枚数>,<CPUの強さ>" も移行のため当面は読む（枚数 0・コイン番号 0 は無料。強さの省略は 2）。
 *   チケット（枚数 1 以上）の台は S キーでは座れず、座る場所のマスでコマンド→道具→チケットを使うと「始めますか？」と聞き（CMD_TALKANSWER。JanAnswer）、
 *   「はい」を受けてから必要枚数を取り上げて座る（いいえ・30 秒切れ・使用中・足りない時は減らさない）。
 *   台NPCのマスが卓の中心 (cx,cy)。盤は 7×7 マス。座る場所は 席0=(cx,cy+4) 席1=(cx+4,cy) 席2=(cx,cy-4) 席3=(cx-4,cy)。
 *   台NPCは見えない（CMD_CHARINFO の byGhost を送る時だけ CHARGHOST_NAMEONLY にする。PuzzleGhostForSend）。
 *   どの席かは、立っているマスで決まる。座るとキャラは歩かない。
 * 人間 2〜4 人＋CPU（空いた席）。最初の人が座ると「参加募集中」（JANMODE_RECRUIT。待ち時間は Param.nWaitSec 秒。サブゲーム情報の wWaitSec）になり、
 *   その間に別の人が空いている席（座る場所のマス）に座れる。4 席が人間で埋まれば待たずに開始。待ち時間が終われば、座っている人（1 人でも）で開始し、空き席は CPU が埋める。
 *   開始後（進行中・終局表示中）の卓には座れない（途中参加なし。観戦はできる）。人が抜ければその席は CPU に戻って局は続く。人間が全員いなくなれば卓を片付ける。
 * 進行（JanProc。g_dwTime の経過時間で進める。1 回の呼び出しで進めるのは 1 動作だけ）:
 *   募集（待ち時間・全席が埋まる）→ 配牌（親は席 0）→ 引く→捨てる→（他席のロン・鳴きの申し出）→次の席 … 局が終わったら結果を
 *   システムメッセージで出し、JAN_OVER_MS 見せてから全員の席を立たせる。
 *   CPU は JAN_CPU_MS ごとに 1 動作。人間の手番は JAN_TURN_MS（時間切れは引いた牌を捨てる。3 回続けたら CPU に戻す）。
 *   人間がロン・そろい・組そろいできる時だけ JAN_CLAIM_MS 待つ（できる人間が複数いれば同時に待つ。全員が返事するか時間切れで、返事なしは見送り）。
 *   CPU の申し出と優先順位は Janjan.h（JanjanCpuClaim・JanjanResolveClaim）。
 * 状態（CMD_JANSTATE）は受取人ごとに作る：座っている人には自分の手牌だけ表（他席は、ほかの人間の席も 0xFF=裏）。座っていない観戦者には全員の手牌を表。
 *   局が終わった時は全員に全手牌を表。変わった時だけ送る（同じ内容は送り直さない。待ち残り dwLimitMs の違いだけでは送り直さない）。
 *   dwLimitMs は受取人ごと：募集中は開始までの残り（全員）・自分の手番／自分の申し出の待ちは本人の残り。
 * 送る相手（卓ごと）: 座っている人・卓の近くにいる人（観戦。JAN_WATCH_MS ごとに見直し、近づいた時に状態を 1 回送り、離れた時に NONE を送る）。
 * 席を立つ・切断・マップ移動・座る場所から離れる（JAN_NEAR マス超）で、その人の席は空き（募集中）／CPU（開始後）に戻る。人間が 0 人になれば卓を待機（空卓）に戻す。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrJan.h"
#include "../../common/Janjan.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#define JAN_NOCHAR			((DWORD)-1)
#define JAN_NEAR			(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define JAN_WATCH_MS		(250)		/* 卓の近くにいる人（観戦）を見直す間隔(ms) */
#define JAN_TILE_DOT		(32)		/* 地図 1 マスの大きさ(ドット) */
/* 状態を送る距離（卓の中心から。横・縦それぞれ。画面は 15×15 マス：半分 7.5＋歩きのずれ 1＋卓の半分と席 4＝12.5 マスに余裕を足した） */
#define JAN_VIEW_ENTER		(15)		/* 送り始める距離（マス） */
#define JAN_VIEW_LEAVE		(18)		/* 消す（NONE を送る）距離（送り始める距離より 3 マス外。境目で送る・消すを繰り返さない） */

/* 卓の進行 */
#define JANMODE_IDLE		(0)			/* 待機（空卓） */
#define JANMODE_RECRUIT		(1)			/* 参加募集中（最初の人が座ってから、待ち時間が終わる・4 席が埋まるまで。牌は配らない） */
#define JANMODE_PLAY		(2)			/* 進行中 */
#define JANMODE_OVER		(3)			/* 終局表示 */
/* 進行中の段 */
#define JANSTEP_DRAW		(0)			/* 手番の席が引く（dwNext） */
#define JANSTEP_DISCARD		(1)			/* 手番の席が捨てる（CPU は dwNext・人間は dwLimit まで） */
#define JANSTEP_CLAIM		(2)			/* 直前の捨て牌への申し出を決める（人間が申し出できる時は abClaimAsk の全員が返事するか dwLimit まで待つ。そうでなければ dwNext） */

/* 台の設定（台NPCの台詞から読む） */
typedef struct _JANPARAM {
	WORD	wCoinItem;						/* コインのアイテム番号（0=無料） */
	int		nCoinCount;						/* 必要な枚数（0=無料） */
	int		nStrength;						/* CPU の強さ（1〜3） */
	int		nWaitSec;						/* 参加募集の待ち時間（秒） */
} JANPARAM;

/* 状態を送ってある人 */
typedef struct _JANWATCH {
	DWORD				dwChar;				/* キャラのテーブルID */
	DWORD				dwPID;				/* プレイヤーID */
	BOOL				bSent;				/* 最後に送った内容を持っているか */
	CMDPARA_JANSTATE	Last;				/* 最後に送った内容（dwLimitMs は 0 にして持つ。同じ内容を送り直さないための比べ用） */
} JANWATCH;

/* 卓（座っている人がいる間と、待機中の卓の近くに人がいる間、表に載せる） */
typedef struct _JANTABLE {
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	DWORD		dwMapID;					/* 台NPCのいるマップ */
	JANPARAM	Param;
	DWORD		adwChar[JANJAN_SEATS];		/* 席に座っている人のテーブルID（人間がいない席は JAN_NOCHAR） */
	DWORD		adwPID[JANJAN_SEATS];		/* そのキャラのプレイヤーID（テーブルIDの使い回しを見分ける） */
	int			anTimeout[JANJAN_SEATS];	/* 手番の時間切れの連続回数（3 回で CPU に戻す。自分で操作すれば 0） */
	int			nMode;						/* JANMODE_* */
	int			nStep;						/* JANSTEP_*（JANMODE_PLAY の時） */
	DWORD		dwNext;						/* 次の動作の時刻（PLAY=CPU の動作・OVER=席を立たせる） */
	DWORD		dwLimit;					/* 待ちの期限（RECRUIT=開始・人間の手番・申し出の待ち） */
	BOOL		abClaimAsk[JANJAN_SEATS];	/* この席の人間に申し出を聞いているか（JANSTEP_CLAIM の時。ロン・そろい・組そろいができる人間だけ） */
	BOOL		abClaimDone[JANJAN_SEATS];	/* その返事（申し出か見送り）が来たか */
	int			anClaim[JANJAN_SEATS];		/* 人間の申し出（JANJAN_CLAIM_*） */
	int			anClaimA[JANJAN_SEATS];		/* 組そろいで使う手牌の 2 枚 */
	int			anClaimB[JANJAN_SEATS];
	DWORD		dwLastWatch;				/* 最後に観戦の人を見直した時刻 */
	JanjanGame	Game;						/* ゲーム（募集中は空） */
	char		aszName[JANJAN_SEATS][JAN_NAME_LEN];	/* 席名（UTF-8。人間=キャラ名・CPU=その席の CPU 名） */
	char		aszCpuName[JANJAN_SEATS][JAN_NAME_LEN];	/* 席の CPU 名（開始時に決める。人間が抜けて CPU に戻る時にこの名前に戻る） */
	std::vector<JANWATCH>	aWatch;			/* 状態を送ってある人（座っている人・近くにいる人） */
} JANTABLE;

static std::vector<JANTABLE> s_aTable;
DWORD g_dwJanSeedOverride = 0;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 「チケットを使って始めますか？」の問い（座ろうとした人ごとに 1 つ。答えは JanAnswer。時間がたったら捨てる） */
typedef struct _JANASK {
	DWORD	dwNpc;							/* 座ろうとした台NPCのテーブルID */
	DWORD	dwTime;							/* 問いを出した時刻 */
} JANASK;
static std::map<DWORD, JANASK> s_mapAsk;

/* 台の設定を読む。台詞が "@j<番号>"（数字だけ）ならサブゲーム情報から（無い・種別が違えば既定=無料・強さ 2）。
   それ以外（"@j" だけ・旧形式 "@j<コイン番号>,<枚数>,<強さ>"）は台詞の数値から */
static void ParseParam(LPCSTR pszSpeech, JANPARAM *pParam)
{
	int anVal[JANJAN_PARAM_COUNT];
	int nNo = JanjanSpeechInfoNo (pszSpeech);

	if (nNo >= 0) {
		SUBGAME_JANJANINFO Info;
		PSUBGAMEINFO pSub = (nNo < g_aSubaGameInfo.GetSize ()) ? g_aSubaGameInfo[nNo] : NULL;

		memset (&Info, 0, sizeof (Info));
		JanjanInfoSetDefault (&Info);
		if (pSub && (pSub->byType == SUBGAMEID_JANJAN) && pSub->pInfo) {
			memcpy (&Info, pSub->pInfo, sizeof (Info));
		}
		JanjanInfoClamp (&Info);
		pParam->wCoinItem	= Info.wTicketItemID;
		pParam->nCoinCount	= Info.byTicketNum;
		pParam->nStrength	= Info.byCpuLevel;
		pParam->nWaitSec	= Info.wWaitSec;
		return;
	}
	JanjanParseSpeech (pszSpeech, anVal);
	pParam->wCoinItem	= ((anVal[0] < 0) || (anVal[0] > 65535)) ? 0 : (WORD)anVal[0];
	pParam->nCoinCount	= (anVal[1] < 0) ? 0 : (anVal[1] > 99) ? 99 : anVal[1];
	pParam->nWaitSec	= JANJAN_WAIT_DEFAULT;
	pParam->nStrength	= (anVal[2] == JANJAN_PARAM_NONE) ? JANJAN_STRENGTH_DEFAULT : ((anVal[2] < 1) ? 1 : (anVal[2] > 3) ? 3 : anVal[2]);
}

static PCHARINFO NpcOf(const JANTABLE &Table)
{
	return (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
}

/* その席に人間が座っているか */
static BOOL IsHumanSeat(const JANTABLE &Table, int s)
{
	return Table.adwChar[s] != JAN_NOCHAR;
}

/* 席 s に座っている人（いなければ・テーブルIDが使い回されていれば NULL） */
static PCHARINFO HumanAt(const JANTABLE &Table, int s)
{
	PCHARINFO pChar;

	if ((Table.adwChar[s] == JAN_NOCHAR) || (Table.adwChar[s] >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	pChar = g_aCharInfo[Table.adwChar[s]];
	if ((pChar == NULL) || (pChar->pID != Table.adwPID[s])) {
		return NULL;
	}
	return pChar;
}

static int HumanCount(const JANTABLE &Table)
{
	int s, n = 0;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		n += IsHumanSeat (Table, s) ? 1 : 0;
	}
	return n;
}

/* pChar が座っている席（座っていなければ -1） */
static int SeatOfChar(const JANTABLE &Table, PCHARINFO pChar)
{
	int s;

	if (pChar == NULL) {
		return -1;
	}
	for (s = 0; s < JANJAN_SEATS; s ++) {
		if ((Table.adwChar[s] == pChar->dwTblID) && (Table.adwPID[s] == pChar->pID)) {
			return s;
		}
	}
	return -1;
}

/* 人間に聞いている申し出の、返事待ちの人がいるか／聞いている人がいるか */
static BOOL AnyClaimPending(const JANTABLE &Table)
{
	int s;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		if (Table.abClaimAsk[s] && !Table.abClaimDone[s]) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL AnyClaimAsk(const JANTABLE &Table)
{
	int s;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		if (Table.abClaimAsk[s]) {
			return TRUE;
		}
	}
	return FALSE;
}

static PCHARINFO WatchChar(const JANWATCH &w)
{
	PCHARINFO pChar;

	if (w.dwChar >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pChar = g_aCharInfo[w.dwChar];
	if ((pChar == NULL) || (pChar->pID != w.dwPID) || (pChar->byGameState < GAMESTATE_MOVE)) {
		return NULL;
	}
	return pChar;
}

static int FindTable(DWORD dwNpc, DWORD dwMapID)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwNpc == dwNpc) && (s_aTable[i].dwMapID == dwMapID)) {
			return i;
		}
	}
	return -1;
}

/* 座っている卓を探す（座っていなければ FALSE） */
static BOOL FindSeat(PCHARINFO pChar, int *pnTable, int *pnSeat = NULL)
{
	int i, s;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if (s_aTable[i].nMode == JANMODE_IDLE) {
			continue;
		}
		s = SeatOfChar (s_aTable[i], pChar);
		if (s >= 0) {
			*pnTable = i;
			if (pnSeat) {
				*pnSeat = s;
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* UTF-8 の文字の途中で切らずに nSize バイト（終端含む）の欄へ写す */
static void CopyName(char *pszDst, int nSize, LPCSTR pszSrc)
{
	int n = (int)strlen (pszSrc);

	if (n > nSize - 1) {
		n = nSize - 1;
		while ((n > 0) && ((((BYTE)pszSrc[n]) & 0xC0) == 0x80)) {
			n --;
		}
	}
	memcpy (pszDst, pszSrc, n);
	pszDst[n] = '\0';
}

/* 席名を決める（開始時。人間=キャラ名・CPU=CPU1〜。席の順に数える） */
static void SetNames(JANTABLE &Table)
{
	int s, nCpu = 0;

	/* CPU の席から CPU1〜、人間の席には（CPU に戻る時の名前として）続きの番号。人間の席の aszName はキャラ名のまま */
	for (s = 0; s < JANJAN_SEATS; s ++) {
		if (!IsHumanSeat (Table, s)) {
			nCpu ++;
			sprintf (Table.aszCpuName[s], "CPU%d", nCpu);
			strcpy (Table.aszName[s], Table.aszCpuName[s]);
		}
	}
	for (s = 0; s < JANJAN_SEATS; s ++) {
		if (IsHumanSeat (Table, s)) {
			nCpu ++;
			sprintf (Table.aszCpuName[s], "CPU%d", nCpu);
		}
	}
}

/* 卓を待機（空卓）に戻す（表からは消さない。近くの人へは待機状態で送り続ける） */
static void ToIdle(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	int s;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		Table.adwChar[s] = JAN_NOCHAR;
		Table.adwPID[s] = 0;
		Table.anTimeout[s] = 0;
		Table.abClaimAsk[s] = FALSE;
		Table.abClaimDone[s] = FALSE;
		Table.anClaim[s] = JANJAN_CLAIM_NONE;
		Table.anClaimA[s] = Table.anClaimB[s] = 0;
	}
	Table.nMode = JANMODE_IDLE;
	Table.nStep = JANSTEP_DRAW;
	Table.dwNext = 0;
	Table.dwLimit = 0;
	ZeroMemory (&Table.Game, sizeof (Table.Game));
	ZeroMemory (Table.aszName, sizeof (Table.aszName));
	ZeroMemory (Table.aszCpuName, sizeof (Table.aszCpuName));
}

/* 座れる席の座る場所に立っている人の案内（JANIDLE_SEAT。必要な枚数が 0 なら JANIDLE_FREE も）。空卓、または募集中の卓の空いている席。
   座る場所のマスに立っていて、倒れておらず、ほかの卓に座っていない人だけ（JanSit で座れる状態） */
static BYTE SeatFlag(const JANTABLE &Table, PCHARINFO pChar)
{
	PCHARINFO pNpc = NpcOf (Table);
	int nSeat = -1;

	if (((Table.nMode != JANMODE_IDLE) && (Table.nMode != JANMODE_RECRUIT)) || (pNpc == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) ||
		(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0) || JanIsSeated (pChar) ||
		PuzzleIsSeated (pChar) || (pChar->dwMapID != Table.dwMapID) || !IsSameSpace (pNpc, pChar) || !JanOnSeatCell (pChar, pNpc, &nSeat)) {
		return 0;
	}
	if ((Table.nMode == JANMODE_RECRUIT) && IsHumanSeat (Table, nSeat)) {
		return 0;			/* 募集中でも、人が座っている席には座れない */
	}
	return (BYTE)(JANIDLE_SEAT | (((Table.Param.wCoinItem == 0) || (Table.Param.nCoinCount <= 0)) ? JANIDLE_FREE : 0));
}

/* 席 s の人間が今、できる操作（JAN_CAN_*）と、組そろいの選び方（aGumi。最大 JAN_GUMI_WIRE）を調べる */
static BYTE CanAct(const JANTABLE &Table, int s, int *pnGumi, BYTE aGumi[JAN_GUMI_WIRE][2])
{
	const JanjanGame &g = Table.Game;
	int n, i;
	BYTE by = 0;

	*pnGumi = 0;
	if ((Table.nMode != JANMODE_PLAY) || (s < 0)) {
		return 0;
	}
	if ((Table.nStep == JANSTEP_DISCARD) && (g.nTurn == s)) {
		if (JanjanCanTsumo (g, s)) {
			by |= JAN_CAN_TSUMO;
		}
	} else if ((Table.nStep == JANSTEP_CLAIM) && Table.abClaimAsk[s] && !Table.abClaimDone[s]) {
		int aOut[JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE][2];

		if (JanjanCanRon (g, s)) {
			by |= JAN_CAN_RON;
		}
		if (JanjanCanSoroi (g, s)) {
			by |= JAN_CAN_SOROI;
		}
		n = JanjanGumiOptions (g, s, aOut, JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE);
		if (n > 0) {
			by |= JAN_CAN_GUMI;
			*pnGumi = (n > JAN_GUMI_WIRE) ? JAN_GUMI_WIRE : n;
			for (i = 0; i < *pnGumi; i ++) {
				aGumi[i][0] = (BYTE)aOut[i][0];
				aGumi[i][1] = (BYTE)aOut[i][1];
			}
		}
		by |= JAN_CAN_PASS;
	}
	return by;
}

/* 席 s の席名・手牌を欄へ書く（空卓は空席・募集中は人間の席名だけ）。bOpen=FALSE なら牌は 0xFF（裏）。引いた牌は最後に置く */
static void FillSeat(const JANTABLE &Table, int s, BOOL bOpen, JANSEATWIRE *pSeat)
{
	const JanjanGame &g = Table.Game;
	int i, n, nSkip = -1;

	ZeroMemory (pSeat, sizeof (*pSeat));
	if (Table.nMode == JANMODE_IDLE) {
		return;			/* 空席 */
	}
	if ((Table.nMode == JANMODE_RECRUIT) && !IsHumanSeat (Table, s)) {
		return;			/* 募集中の空き席（開始すると CPU が座る） */
	}
	pSeat->byState = (BYTE)(IsHumanSeat (Table, s) ? JANSEAT_HUMAN : JANSEAT_CPU);
	memcpy (pSeat->szName, Table.aszName[s], JAN_NAME_LEN);
	pSeat->szName[JAN_NAME_LEN - 1] = '\0';
	if (Table.nMode == JANMODE_RECRUIT) {
		return;			/* 募集中は牌なし */
	}
	pSeat->sScore = (short)g.anScore[s];
	n = g.anHand[s];
	if (n > JAN_HAND_WIRE) {
		n = JAN_HAND_WIRE;
	}
	pSeat->byHandNum = (BYTE)n;
	/* 引いた牌（引いた席の捨てる番。ツモ上がりで終わった時も）は最後に置く */
	if ((s == g.nTurn) && g.bDrew && (n > 0) &&
		((g.nPhase == JANJAN_PHASE_DISCARD) || ((g.nPhase == JANJAN_PHASE_END) && (g.nResult == JANJAN_RESULT_WIN) && (g.nWinHow == JANJAN_HOW_TSUMO)))) {
		int nDrawn = g.aPile[g.nPileTop - 1];
		for (i = 0; i < n; i ++) {
			if (g.aHand[s][i] == nDrawn) {
				nSkip = i;
				break;
			}
		}
	}
	{
		int nPos = 0;
		for (i = 0; i < n; i ++) {
			if (i == nSkip) {
				continue;
			}
			pSeat->abyHand[nPos ++] = bOpen ? (BYTE)g.aHand[s][i] : (BYTE)JAN_TILE_HIDDEN;
		}
		if (nSkip >= 0) {
			pSeat->abyHand[nPos ++] = bOpen ? (BYTE)g.aHand[s][nSkip] : (BYTE)JAN_TILE_HIDDEN;
		}
	}
	n = g.anMeld[s];
	if (n > JAN_MELD_WIRE) {
		n = JAN_MELD_WIRE;
	}
	pSeat->byMeldNum = (BYTE)n;
	for (i = 0; i < n; i ++) {
		pSeat->aMeld[i].byKind = (BYTE)g.aMeld[s][i].nKind;
		pSeat->aMeld[i].abyTile[0] = (BYTE)g.aMeld[s][i].aTile[0];
		pSeat->aMeld[i].abyTile[1] = (BYTE)g.aMeld[s][i].aTile[1];
		pSeat->aMeld[i].abyTile[2] = (BYTE)g.aMeld[s][i].aTile[2];
	}
	n = g.anRiver[s];
	if (n > JAN_RIVER_WIRE) {
		n = JAN_RIVER_WIRE;
	}
	pSeat->byRiverNum = (BYTE)n;
	for (i = 0; i < n; i ++) {
		pSeat->abyRiver[i] = (BYTE)g.aRiver[s][i];
	}
}

/* 卓の状態を受取人 pRecv（NULL=誰でもない観戦者）向けに作る。bNone なら「消す」 */
static void BuildState(const JANTABLE &Table, PCHARINFO pRecv, BOOL bNone, CMDPARA_JANSTATE *pOut)
{
	const JanjanGame &g = Table.Game;
	PCHARINFO pNpc = NpcOf (Table);
	int nMy = (Table.nMode != JANMODE_IDLE) ? SeatOfChar (Table, pRecv) : -1;		/* 受取人が座っている席（座っていなければ -1） */
	BOOL bMine = (nMy >= 0);
	BOOL bAll = (Table.nMode == JANMODE_OVER);			/* 局が終わった時は全員に全手牌 */
	int s, nGumi = 0;
	BYTE aGumi[JAN_GUMI_WIRE][2];

	ZeroMemory (pOut, sizeof (*pOut));
	pOut->byCmd = CMD_JANSTATE;
	pOut->dwNpc = Table.dwNpc;
	pOut->byKind = (BYTE)(bNone ? JANSTATE_NONE : JANSTATE_STATE);
	pOut->byMySeat = (BYTE)(bMine ? nMy : 0xFF);
	pOut->byLastTile = JAN_TILE_HIDDEN;
	if (pNpc) {
		pOut->nFieldX = ((int)pNpc->dwX - JANJAN_TABLE_HALF) * JAN_TILE_DOT;
		pOut->nFieldY = ((int)pNpc->dwY - JANJAN_TABLE_HALF) * JAN_TILE_DOT;
	}
	if (bNone) {
		return;
	}
	if ((Table.nMode == JANMODE_IDLE) || (Table.nMode == JANMODE_RECRUIT)) {
		BOOL bRecruit = (Table.nMode == JANMODE_RECRUIT);

		/* 空卓：待機（JANIDLE_ON）。募集中：JANPHASE_RECRUIT（dwLimitMs=開始までの残り。受取人全員に）。どちらも座れる席の座る場所に立つ人には SEAT */
		pOut->byIdle = (BYTE)((bRecruit ? 0 : JANIDLE_ON) | ((pRecv != NULL) ? SeatFlag (Table, pRecv) : 0));
		pOut->byPhase = (BYTE)(bRecruit ? JANPHASE_RECRUIT : JANPHASE_IDLE);
		if (bRecruit) {
			int nMs = (int)(Table.dwLimit - g_dwTime);
			pOut->dwLimitMs = (DWORD)((nMs < 0) ? 0 : nMs);
		}
		for (s = 0; s < JANJAN_SEATS; s ++) {
			FillSeat (Table, s, FALSE, &pOut->aSeat[s]);
		}
		return;
	}
	pOut->byPhase = (BYTE)((Table.nMode == JANMODE_OVER) ? JANPHASE_OVER : JANPHASE_PLAY);
	pOut->byTurn = (BYTE)g.nTurn;
	pOut->byLogicPhase = (BYTE)g.nPhase;
	pOut->byPileLeft = (BYTE)JanjanPileLeft (g);
	pOut->byResult = (BYTE)g.nResult;
	pOut->byWinner = (BYTE)((g.nWinner < 0) ? 0 : g.nWinner);
	pOut->byWinHow = (BYTE)g.nWinHow;
	pOut->byLastSeat = (BYTE)g.nLastSeat;
	if (g.bLastLive || ((g.nResult == JANJAN_RESULT_WIN) && (g.nWinHow == JANJAN_HOW_RON))) {
		pOut->byLastTile = (BYTE)g.nLastTile;			/* 続行中は鳴き・ロンの対象の牌。ロンで終わった時は上がり牌 */
	}
	if (bMine) {
		pOut->byCanAct = CanAct (Table, nMy, &nGumi, aGumi);
		pOut->byGumiNum = (BYTE)nGumi;
		memcpy (pOut->abyGumi, aGumi, sizeof (pOut->abyGumi));
		if ((Table.nMode == JANMODE_PLAY) && (((Table.nStep == JANSTEP_DISCARD) && (g.nTurn == nMy)) ||
			((Table.nStep == JANSTEP_CLAIM) && Table.abClaimAsk[nMy] && !Table.abClaimDone[nMy]))) {
			int nMs = (int)(Table.dwLimit - g_dwTime);
			pOut->dwLimitMs = (DWORD)((nMs < 0) ? 0 : nMs);		/* 本人の待ち残り（受取人ごと） */
		}
	}
	/* 手牌：座っている人は自分だけ表（ほかの人間の席も裏）。座っていない観戦者は全員表。局が終わったら全員表 */
	for (s = 0; s < JANJAN_SEATS; s ++) {
		BOOL bOpen = bAll || !bMine || (s == nMy);
		FillSeat (Table, s, bOpen, &pOut->aSeat[s]);
	}
}

/* 1 人に送る。前に送った内容と同じ（待ち残りの違いだけ）なら送らない */
static void SendStateTo(const JANTABLE &Table, PCHARINFO pChar, JANWATCH &w, BOOL bNone)
{
	CMDPARA_JANSTATE St, Cmp;

	BuildState (Table, pChar, bNone, &St);
	Cmp = St;
	Cmp.dwLimitMs = 0;
	if (w.bSent && (memcmp (&w.Last, &Cmp, sizeof (Cmp)) == 0)) {
		return;
	}
	w.Last = Cmp;
	w.bSent = TRUE;
	SendCmdJANSTATE (pChar->pID, (PBYTE)&St, sizeof (St));
}

/* 卓の状態を、送ってある全員に送る（受取人ごとに作る）。pSkip には送らない */
static void SendTable(int nTable, PCHARINFO pSkip = NULL)
{
	JANTABLE &Table = s_aTable[nTable];
	int i;

	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar = WatchChar (Table.aWatch[i]);
		if ((pChar == NULL) || (pChar == pSkip)) {
			continue;
		}
		SendStateTo (Table, pChar, Table.aWatch[i], FALSE);
	}
}

/* 送ってある全員へシステムメッセージ（pSkip には送らない） */
static void SendMsgAll(const JANTABLE &Table, LPCSTR pszMsg, PCHARINFO pSkip = NULL)
{
	int i;

	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar = WatchChar (Table.aWatch[i]);
		if (pChar && (pChar != pSkip)) {
			SendCmdSYSMSG (pChar->pID, pszMsg);
		}
	}
}

/* 卓の状態を送る距離にいるか（卓の中心から nMargin マス） */
static BOOL WatchEligible(DWORD dwMapID, PCHARINFO pNpc, PCHARINFO pChar, int nMargin)
{
	if ((pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) || (pChar->dwMapID != dwMapID) ||
		!IsSameSpace (pNpc, pChar) || !IsCharSeenBy (pNpc, pChar)) {
		return FALSE;
	}
	/* 座る場所のマスに立っている人は、遠くても届ける（案内を出すため） */
	if (JanOnSeatCell (pChar, pNpc, NULL)) {
		return TRUE;
	}
	return (abs ((int)pChar->dwX - (int)pNpc->dwX) <= nMargin) && (abs ((int)pChar->dwY - (int)pNpc->dwY) <= nMargin);
}

static int FindWatch(const std::vector<JANWATCH> &aWatch, DWORD dwChar, DWORD dwPID)
{
	int i;

	for (i = 0; i < (int)aWatch.size (); i ++) {
		if ((aWatch[i].dwChar == dwChar) && (aWatch[i].dwPID == dwPID)) {
			return i;
		}
	}
	return -1;
}

/* 状態が見える人を見直す。新しく見える人には状態を送り、見えなくなった人には NONE を送って消してもらう。
   すでに見ている人へは、内容が変わっていた時（座る場所に立った・離れたなど）だけ送る */
static void WatchRefresh(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	PCHARINFO pNpc = NpcOf (Table);
	std::vector<JANWATCH> aNew;
	PMAPINFO pMap;
	int i, s, nOld;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	/* 座っている人は距離に関係なく入れる */
	for (s = 0; s < JANJAN_SEATS; s ++) {
		PCHARINFO pSeat = HumanAt (Table, s);
		JANWATCH w;

		if (pSeat == NULL) {
			continue;
		}
		nOld = FindWatch (Table.aWatch, pSeat->dwTblID, pSeat->pID);
		if (nOld >= 0) {
			w = Table.aWatch[nOld];
		} else {
			ZeroMemory (&w, sizeof (w));
			w.dwChar = pSeat->dwTblID;
			w.dwPID = pSeat->pID;
		}
		aNew.push_back (w);
	}
	/* 同じマップの近くにいる人 */
	pMap = (Table.dwMapID < (DWORD)g_aMapInfo.GetSize ()) ? g_aMapInfo[Table.dwMapID] : NULL;
	if (pMap && pMap->paTblNoPC) {
		for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
			PCHARINFO pChar;
			JANWATCH w;

			if (pMap->paTblNoPC[i] == (DWORD)-1) {
				break;
			}
			pChar = g_aCharInfo[pMap->paTblNoPC[i]];
			if (SeatOfChar (Table, pChar) >= 0) {
				continue;
			}
			nOld = FindWatch (Table.aWatch, pChar->dwTblID, pChar->pID);
			/* すでに送ってある人は遠い距離（LEAVE）まで、新しい人は近い距離（ENTER）から。境目で送る・消すを繰り返さない */
			if (!WatchEligible (Table.dwMapID, pNpc, pChar, (nOld >= 0) ? JAN_VIEW_LEAVE : JAN_VIEW_ENTER)) {
				continue;
			}
			if (nOld >= 0) {
				w = Table.aWatch[nOld];
			} else {
				ZeroMemory (&w, sizeof (w));
				w.dwChar = pChar->dwTblID;
				w.dwPID = pChar->pID;
			}
			aNew.push_back (w);
		}
	}
	/* 見えなくなった人へ NONE */
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar;
		if (FindWatch (aNew, Table.aWatch[i].dwChar, Table.aWatch[i].dwPID) >= 0) {
			continue;
		}
		pChar = WatchChar (Table.aWatch[i]);
		if (pChar) {
			SendStateTo (Table, pChar, Table.aWatch[i], TRUE);
		}
	}
	/* 新しく見える人へ状態。すでに見ている人へは変わった時だけ */
	for (i = 0; i < (int)aNew.size (); i ++) {
		PCHARINFO pChar = WatchChar (aNew[i]);
		if (pChar) {
			SendStateTo (Table, pChar, aNew[i], FALSE);
		}
	}
	Table.aWatch = aNew;
}

/* 席を立たせる。bNotifySelf でなければ本人へは送らない（切断）。卓は待機（空卓）に戻る */
static void CloseTable(int nTable, PCHARINFO pSkip = NULL)
{
	ToIdle (nTable);
	SendTable (nTable, pSkip);
}

/* 席 s の人を席から外す。bNotifySelf でなければ本人へは送らない（切断）。
   開始後は席が CPU（元の CPU 名）に戻って局は続く。募集中は席が空く。人間が 0 人になれば卓を待機（空卓）に戻す */
static void RemoveHuman(int nTable, int s, BOOL bNotifySelf)
{
	JANTABLE &Table = s_aTable[nTable];
	PCHARINFO pSelf = HumanAt (Table, s);

	Table.adwChar[s] = JAN_NOCHAR;
	Table.adwPID[s] = 0;
	Table.anTimeout[s] = 0;
	Table.abClaimAsk[s] = FALSE;
	Table.abClaimDone[s] = FALSE;
	Table.anClaim[s] = JANJAN_CLAIM_NONE;
	if (HumanCount (Table) == 0) {
		CloseTable (nTable, bNotifySelf ? NULL : pSelf);
		return;
	}
	if (Table.nMode == JANMODE_RECRUIT) {
		ZeroMemory (Table.aszName[s], sizeof (Table.aszName[s]));
	} else {
		memcpy (Table.aszName[s], Table.aszCpuName[s], JAN_NAME_LEN);
	}
	SendTable (nTable, bNotifySelf ? NULL : pSelf);
}

/* 局を終える（結果をシステムメッセージで出し、JAN_OVER_MS 見せてから席を立たせる） */
static void EndRound(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	const JanjanGame &g = Table.Game;
	char szMsg[96];

	Table.nMode = JANMODE_OVER;
	Table.dwNext = g_dwTime + JAN_OVER_MS;
	if ((g.nResult == JANJAN_RESULT_WIN) && (g.nWinner >= 0) && (g.nWinner < JANJAN_SEATS)) {
		sprintf (szMsg, "%sの上がり！", Table.aszName[g.nWinner]);
	} else {
		strcpy (szMsg, "流局");
	}
	SendMsgAll (Table, szMsg);
	SendTable (nTable);
}

/* 人間の手番（捨てる）に入る時の待ちの期限 */
static void SetHumanTurn(JANTABLE &Table)
{
	if (IsHumanSeat (Table, Table.Game.nTurn)) {
		Table.dwLimit = g_dwTime + JAN_TURN_MS;
	}
	Table.dwNext = g_dwTime + JAN_CPU_MS;
}

/* 捨てた直後：人間がロン・鳴きできるならその返事を（できる人間みんなで同時に）待ち、できなければ CPU の申し出を決める段へ */
static void GoClaim(JANTABLE &Table)
{
	const JanjanGame &g = Table.Game;
	int s;
	BOOL bAsk = FALSE;

	Table.nStep = JANSTEP_CLAIM;
	for (s = 0; s < JANJAN_SEATS; s ++) {
		Table.anClaim[s] = JANJAN_CLAIM_NONE;
		Table.abClaimDone[s] = FALSE;
		Table.abClaimAsk[s] = (IsHumanSeat (Table, s) && (JanjanCanRon (g, s) || JanjanCanSoroi (g, s) || JanjanCanGumi (g, s))) ? TRUE : FALSE;
		bAsk = bAsk || Table.abClaimAsk[s];
	}
	if (bAsk) {
		Table.dwLimit = g_dwTime + JAN_CLAIM_MS;		/* 申し出られる人間が複数いても、同時に待つ */
	}
	Table.dwNext = g_dwTime + JAN_CPU_MS;
}

/* 直前の捨て牌への申し出を決める（CPU は JanjanCpuClaim・人間は anClaim。優先は JanjanResolveClaim）。
   上がり→局を終える／鳴き→その席が捨てる番／誰もなし→次の席が引く */
static void ResolveClaim(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	JanjanGame &g = Table.Game;
	int aClaim[JANJAN_SEATS], aA[JANJAN_SEATS], aB[JANJAN_SEATS], s, nSeat = -1, nType;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		aClaim[s] = JANJAN_CLAIM_NONE;
		aA[s] = aB[s] = 0;
		if (IsHumanSeat (Table, s)) {
			/* 人間：返事のあった申し出（返事なし・聞かれていない人は見送り） */
			if (Table.abClaimAsk[s]) {
				aClaim[s] = Table.anClaim[s];
				aA[s] = Table.anClaimA[s];
				aB[s] = Table.anClaimB[s];
			}
		} else if (s != g.nLastSeat) {
			aClaim[s] = JanjanCpuClaim (g, s, Table.Param.nStrength, &aA[s], &aB[s]);
		}
	}
	for (s = 0; s < JANJAN_SEATS; s ++) {
		Table.abClaimAsk[s] = FALSE;
		Table.abClaimDone[s] = FALSE;
	}
	nType = JanjanResolveClaim (g, aClaim, &nSeat);
	if (nType == JANJAN_CLAIM_RON) {
		/* 頭ハネは「申し出た席の中」で決める（人間が見送ったロンは、後ろの席のロンを止めない） */
		JanjanFinishWin (g, nSeat, JANJAN_HOW_RON);
		EndRound (nTable);
		return;
	}
	if (nType == JANJAN_CLAIM_SOROI) {
		JanjanSoroi (g, nSeat);
	} else if (nType == JANJAN_CLAIM_GUMI) {
		JanjanGumi (g, nSeat, aA[nSeat], aB[nSeat]);
	}
	if (nType != JANJAN_CLAIM_NONE) {
		Table.nStep = JANSTEP_DISCARD;
		SetHumanTurn (Table);
	} else {
		Table.nStep = JANSTEP_DRAW;
		Table.dwNext = g_dwTime + JAN_CPU_MS;
	}
	SendTable (nTable);
}

/* 手番の席が引く（山が尽きていれば流局） */
static void DoDraw(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	JanjanGame &g = Table.Game;

	if (JanjanPileLeft (g) <= 0) {
		JanjanFinishDraw (g);
		EndRound (nTable);
		return;
	}
	JanjanDraw (g, g.nTurn);
	Table.nStep = JANSTEP_DISCARD;
	SetHumanTurn (Table);
	SendTable (nTable);
}

/* 手番の席が捨てた後（共通） */
static void AfterDiscard(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];

	GoClaim (Table);
	SendTable (nTable);
}

/* CPU の捨てる番：上がれるなら上がる。そうでなければ捨てる */
static void DoCpuDiscard(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	JanjanGame &g = Table.Game;
	int s = g.nTurn, nTile;

	if (JanjanCanTsumo (g, s)) {
		JanjanTsumo (g, s);
		EndRound (nTable);
		return;
	}
	nTile = JanjanCpuDiscard (g, s, Table.Param.nStrength);
	if ((nTile < 0) || !JanjanDiscard (g, s, nTile)) {
		return;
	}
	AfterDiscard (nTable);
}

/* 人間の時間切れ：引いた牌（なければ手牌の最後）を捨てる。JAN_TIMEOUT_MAX 回続けたら CPU に戻す */
static void DoHumanTimeoutDiscard(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	JanjanGame &g = Table.Game;
	int s = g.nTurn, nTile;
	PCHARINFO pSelf = HumanAt (Table, s);

	if (g.anHand[s] <= 0) {
		return;
	}
	nTile = (g.bDrew && (g.nPileTop > 0)) ? g.aPile[g.nPileTop - 1] : g.aHand[s][g.anHand[s] - 1];
	if (!JanjanDiscard (g, s, nTile)) {
		return;
	}
	Table.anTimeout[s] ++;
	AfterDiscard (nTable);
	/* 3 回続けて時間切れなら離席扱い：その席は CPU に戻る */
	if (Table.anTimeout[s] >= JAN_TIMEOUT_MAX) {
		if (pSelf) {
			SendCmdSYSMSG (pSelf->pID, "離席扱いになりました。");
		}
		RemoveHuman (nTable, s, TRUE);
	}
}

/* 卓に座れるか確かめる（座れなければ理由を本人に伝えて FALSE）。座れるのは、空卓か、参加募集中の卓の空いている席だけ */
static BOOL SitCheck(const JANTABLE &Table, int nSeat, PCHARINFO pChar)
{
	if ((Table.nMode == JANMODE_PLAY) || (Table.nMode == JANMODE_OVER)) {
		SendCmdSYSMSG (pChar->pID, "ゲーム中です。");
		return FALSE;
	}
	if ((Table.nMode == JANMODE_RECRUIT) && IsHumanSeat (Table, nSeat)) {
		SendCmdSYSMSG (pChar->pID, "その席は今、使われています。");
		return FALSE;
	}
	return TRUE;
}

/* 手持ちにあるコインの数（装備品でない普通のアイテム） */
static int CountCoin(PCHARINFO pChar, WORD wItem)
{
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == wItem) {
			nRet ++;
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 外から呼ぶ関数															 */
/* ========================================================================= */

BOOL JanIsTableNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'j');
}

BOOL JanOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc, int *pnSeat)
{
	int s;

	if (!JanIsTableNpc (pNpc) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return FALSE;
	}
	for (s = 0; s < JANJAN_SEATS; s ++) {
		int nDx, nDy;

		JanjanSeatOffset (s, &nDx, &nDy);
		if (((int)pChar->dwX == (int)pNpc->dwX + nDx) && ((int)pChar->dwY == (int)pNpc->dwY + nDy)) {
			if (pnSeat) {
				*pnSeat = s;
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL JanIsSeated(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable);
}

JanjanGame *JanGetGame(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable) ? &s_aTable[nTable].Game : NULL;
}

int JanGetSeat(PCHARINFO pChar)
{
	int nTable, nSeat;

	return FindSeat (pChar, &nTable, &nSeat) ? nSeat : -1;
}

int JanSeatCount(void)
{
	int i, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if (s_aTable[i].nMode != JANMODE_IDLE) {
			nRet ++;
		}
	}
	return nRet;
}

int JanIdleCount(void)
{
	return (int)s_aTable.size () - JanSeatCount ();
}

/* 募集を終えて開始する（座っている人の席は人間・空き席は CPU。配牌して親は席 0）。待ち時間が終わった時・4 席が埋まった時 */
static void StartGame(int nTable)
{
	JANTABLE &Table = s_aTable[nTable];
	PCHARINFO pFirst = NULL;
	unsigned int dwSeed;
	int s;

	for (s = 0; s < JANJAN_SEATS; s ++) {
		if (pFirst == NULL) {
			pFirst = HumanAt (Table, s);
		}
	}
	dwSeed = (g_dwJanSeedOverride != 0) ? g_dwJanSeedOverride : ((unsigned int)(g_dwTime * 2654435761u) ^ ((pFirst ? pFirst->dwTblID : 0) * 40503u + 1));
	SetNames (Table);
	ZeroMemory (&Table.Game, sizeof (Table.Game));
	JanjanStart (Table.Game, dwSeed);
	Table.nMode = JANMODE_PLAY;
	Table.nStep = JANSTEP_DRAW;
	Table.dwNext = g_dwTime + JAN_CPU_MS;
	SendTable (nTable);
}

void JanSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bCoinUsed)
{
	JANPARAM Param;
	int i, nTable, nSeat = 0;
	BOOL bNew;
	char szMsg[96];

	if (!JanIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || JanIsSeated (pChar) || PuzzleIsSeated (pChar)) {
		return;
	}
	/* 座れるのは、台の 4 つの座る場所のマスの上だけ（同じマップ・同じ空間。向きは問わない）。どの席かはマスで決まる */
	if (!JanOnSeatCell (pChar, pNpc, &nSeat)) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}
	ParseParam (pNpc->pszNpcSpeech, &Param);

	/* 座れるのは、空卓か、参加募集中の卓の空いている席だけ（開始後は座れない。観戦はできる） */
	nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
	if ((nTable >= 0) && !SitCheck (s_aTable[nTable], nSeat, pChar)) {
		return;
	}
	/* チケットの台（必要枚数 1 以上）は S キーでは始まらない。道具のチケットを使って始める（無料の台は S キーで始まる） */
	if (!bCoinUsed && (Param.wCoinItem != 0) && (Param.nCoinCount > 0)) {
		SendCmdSYSMSG (pChar->pID, "チケットを使って始めます。コマンドの道具からチケットを使ってください。");
		return;
	}
	/* チケット（無料の台は何も使わない）。足りなければ座らせない。ここまで来たら座る（確認の「はい」を受けた後） */
	if ((Param.wCoinItem != 0) && (Param.nCoinCount > 0)) {
		if (CountCoin (pChar, Param.wCoinItem) < Param.nCoinCount) {
			SendCmdSYSMSG (pChar->pID, "チケットがありません。");
			return;
		}
		for (i = 0; i < Param.nCoinCount; i ++) {
			QuestTakeItemOne (pChar, Param.wCoinItem);
		}
		/* 取り上げた分を本人の画面の持ち物に反映する（送らないと、座った後も画面にコインが残り、捨てると切断になる） */
		SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
		sprintf (szMsg, "チケットを%d枚使いました。", Param.nCoinCount);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}

	bNew = ((nTable < 0) || (s_aTable[nTable].nMode == JANMODE_IDLE));
	if (nTable < 0) {
		JANTABLE Table;

		Table.dwNpc = pNpc->dwTblID;
		Table.dwMapID = pNpc->dwMapID;
		Table.dwLastWatch = 0;
		s_aTable.push_back (Table);
		nTable = (int)s_aTable.size () - 1;
		ToIdle (nTable);
	}
	{
		JANTABLE &Table = s_aTable[nTable];

		if (bNew) {
			/* 最初の人：参加募集を始める（待ち時間のカウントダウン） */
			ToIdle (nTable);
			Table.Param = Param;
			Table.nMode = JANMODE_RECRUIT;
			Table.nStep = JANSTEP_DRAW;
			Table.dwLimit = g_dwTime + (DWORD)Param.nWaitSec * 1000;
			Table.Game.nWinner = -1;
		}
		Table.adwChar[nSeat] = pChar->dwTblID;
		Table.adwPID[nSeat] = pChar->pID;
		Table.anTimeout[nSeat] = 0;
		CopyName (Table.aszName[nSeat], JAN_NAME_LEN, pChar->szName);
		/* 先に座っている人・観戦者へ知らせる（座った本人へは出さない） */
		if (!bNew) {
			sprintf (szMsg, "%sさんが参加しました。", Table.aszName[nSeat]);
			SendMsgAll (Table, szMsg, pChar);
		}
	}
	/* 待機・募集中から見ていた人（座った本人を含む）へ状態。新しく見える人へは見直しで届く */
	SendTable (nTable);
	WatchRefresh (nTable);
	/* 4 席が人間で埋まったら待たずに開始 */
	if (HumanCount (s_aTable[nTable]) >= JANJAN_SEATS) {
		StartGame (nTable);
	}
}

BOOL JanUseItem(PCHARINFO pChar, WORD wItem)
{
	PMAPINFO pMap;
	int i;

	if ((wItem == 0) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return FALSE;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	if (pMap == NULL || pMap->paTblNoNPC == NULL) {
		return FALSE;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		PCHARINFO pNpc;
		JANPARAM Param;
		int nSeatCell;

		if (pMap->paTblNoNPC[i] == (DWORD)-1) {
			break;
		}
		pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
		if (!JanIsTableNpc (pNpc) || !JanOnSeatCell (pChar, pNpc, NULL)) {
			continue;
		}
		ParseParam (pNpc->pszNpcSpeech, &Param);
		if ((Param.wCoinItem != wItem) || (Param.nCoinCount <= 0)) {
			continue;
		}
		nSeatCell = 0;
		JanOnSeatCell (pChar, pNpc, &nSeatCell);
		/* 使えない時（座っている・倒れている・卓が使用中・足りない）は何もしない・案内だけで、問いは出さずチケットは減らさない */
		if (JanIsSeated (pChar) || PuzzleIsSeated (pChar) || (pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
			return TRUE;
		}
		{
			int nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
			JANASK Ask;
			char szAsk[96];

			if ((nTable >= 0) && !SitCheck (s_aTable[nTable], nSeatCell, pChar)) {
				return TRUE;
			}
			if (CountCoin (pChar, Param.wCoinItem) < Param.nCoinCount) {
				SendCmdSYSMSG (pChar->pID, "チケットがありません。");
				return TRUE;
			}
			/* 確認（はい／いいえ）。取り上げは「はい」を受けてから（JanAnswer → JanSit） */
			Ask.dwNpc = pNpc->dwTblID;
			Ask.dwTime = g_dwTime;
			s_mapAsk[pChar->dwTblID] = Ask;
			sprintf (szAsk, "チケットを%d枚使って始めますか？@y", Param.nCoinCount);
			SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szAsk);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL JanAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes)
{
	std::map<DWORD, JANASK>::iterator it;
	JANASK Ask;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return FALSE;
	}
	it = s_mapAsk.find (pChar->dwTblID);
	if ((it == s_mapAsk.end ()) || (it->second.dwNpc != dwNpcTblID)) {
		return FALSE;
	}
	Ask = it->second;
	s_mapAsk.erase (it);
	/* 30 秒たった問い・いいえ・台が消えた時は何もしない（チケットは減らさない） */
	if (((DWORD)(g_dwTime - Ask.dwTime) > (DWORD)JAN_ASK_TIMEOUT_MS) || !bYes || (dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return TRUE;
	}
	/* 座る条件（座る場所のマス・使用中でない・チケットが足りる）は JanSit が改めて確かめる */
	JanSit (pChar, g_aCharInfo[dwNpcTblID], TRUE);
	return TRUE;
}

void JanKey(PCHARINFO pChar, BYTE byAct, BYTE byParam)
{
	int nTable, s;

	if (!FindSeat (pChar, &nTable, &s)) {
		return;
	}
	JANTABLE &Table = s_aTable[nTable];
	JanjanGame &g = Table.Game;

	if (byAct == JAN_ACT_LEAVE) {
		RemoveHuman (nTable, s, TRUE);
		return;
	}
	if (Table.nMode != JANMODE_PLAY) {
		return;
	}
	if ((Table.nStep == JANSTEP_DISCARD) && (g.nTurn == s)) {
		if (byAct == JAN_ACT_DISCARD) {
			if (JanjanDiscard (g, s, (int)byParam)) {
				Table.anTimeout[s] = 0;
				AfterDiscard (nTable);
			}
		} else if (byAct == JAN_ACT_TSUMO) {
			if (JanjanTsumo (g, s)) {
				EndRound (nTable);
			}
		}
		return;
	}
	if ((Table.nStep == JANSTEP_CLAIM) && Table.abClaimAsk[s] && !Table.abClaimDone[s]) {
		switch (byAct) {
		case JAN_ACT_RON:
			if (!JanjanCanRon (g, s)) {
				return;
			}
			Table.anClaim[s] = JANJAN_CLAIM_RON;
			break;
		case JAN_ACT_SOROI:
			if (!JanjanCanSoroi (g, s)) {
				return;
			}
			Table.anClaim[s] = JANJAN_CLAIM_SOROI;
			break;
		case JAN_ACT_GUMI:
			{
				int aOut[JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE][2];
				int n = JanjanGumiOptions (g, s, aOut, JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE);

				if ((int)byParam >= n || (int)byParam >= JAN_GUMI_WIRE) {
					return;
				}
				Table.anClaim[s] = JANJAN_CLAIM_GUMI;
				Table.anClaimA[s] = aOut[byParam][0];
				Table.anClaimB[s] = aOut[byParam][1];
			}
			break;
		case JAN_ACT_PASS:
			Table.anClaim[s] = JANJAN_CLAIM_NONE;
			break;
		default:
			return;
		}
		Table.abClaimDone[s] = TRUE;
		Table.anTimeout[s] = 0;
		if (AnyClaimPending (Table)) {
			SendTable (nTable);		/* 返事をした本人の操作の行を消す（ほかの人の返事を待つ） */
			return;
		}
		ResolveClaim (nTable);
	}
}

/* 待機中の卓を新しく表に載せる：人のいるマップの台NPC（席が空）のうち、近くに人がいるものだけ。
   近くに誰もいない卓は表に載らない（何も送らない・覚えない） */
static void IdleScan(void)
{
	int nMap, i, j;

	for (nMap = 0; nMap < (int)g_aMapInfo.GetSize (); nMap ++) {
		PMAPINFO pMap = g_aMapInfo[nMap];

		if ((pMap == NULL) || (pMap->paTblNoPC == NULL) || (pMap->paTblNoNPC == NULL) || (pMap->dwCountTblNoPC == 0) ||
			(pMap->paTblNoPC[0] == (DWORD)-1)) {
			continue;
		}
		for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
			PCHARINFO pNpc;
			BOOL bNear = FALSE;

			if (pMap->paTblNoNPC[i] == (DWORD)-1) {
				break;
			}
			if (pMap->paTblNoNPC[i] >= (DWORD)g_aCharInfo.GetSize ()) {
				continue;
			}
			pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
			if (!JanIsTableNpc (pNpc) || (FindTable (pNpc->dwTblID, (DWORD)nMap) >= 0)) {
				continue;
			}
			for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
				PCHARINFO pChar;

				if (pMap->paTblNoPC[j] == (DWORD)-1) {
					break;
				}
				if (pMap->paTblNoPC[j] >= (DWORD)g_aCharInfo.GetSize ()) {
					continue;
				}
				pChar = g_aCharInfo[pMap->paTblNoPC[j]];
				if (WatchEligible ((DWORD)nMap, pNpc, pChar, JAN_VIEW_ENTER)) {
					bNear = TRUE;
					break;
				}
			}
			if (bNear) {
				JANTABLE Table;
				int nTable;

				Table.dwNpc = pNpc->dwTblID;
				Table.dwMapID = (DWORD)nMap;
				Table.dwLastWatch = 0;
				ParseParam (pNpc->pszNpcSpeech, &Table.Param);
				s_aTable.push_back (Table);
				nTable = (int)s_aTable.size () - 1;
				ToIdle (nTable);
				WatchRefresh (nTable);
			}
		}
	}
}

void JanProc(void)
{
	static DWORD s_dwLastScan = 0;
	int i;

	/* 卓が消えると添字がずれるので、後ろから 1 つずつ（消えるのはその卓だけ） */
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		if (i >= (int)s_aTable.size ()) {
			continue;
		}
		JANTABLE &Table = s_aTable[i];
		PCHARINFO pNpc = NpcOf (Table);
		BOOL bNpcOK = JanIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);
		int s;

		/* 待機中の卓（募集中は人間がいるので下の処理）：台が無くなれば見ている人へ NONE を送って表から消す。
		   近くの人の見直しをして、誰もいなくなれば表から消す */
		if (Table.nMode == JANMODE_IDLE) {
			if (!bNpcOK) {
				int k;

				for (k = 0; k < (int)Table.aWatch.size (); k ++) {
					PCHARINFO pW = WatchChar (Table.aWatch[k]);
					if (pW) {
						SendStateTo (Table, pW, Table.aWatch[k], TRUE);
					}
				}
				s_aTable.erase (s_aTable.begin () + i);
				continue;
			}
			if ((DWORD)(g_dwTime - Table.dwLastWatch) >= JAN_WATCH_MS) {
				WatchRefresh (i);
			}
			if (Table.aWatch.empty ()) {
				s_aTable.erase (s_aTable.begin () + i);
			}
			continue;
		}

		/* 台が無くなった時は片付ける */
		if (!bNpcOK) {
			CloseTable (i);
			continue;
		}
		/* 切断・マップ移動・席から離れた・倒れた人は席から外す（開始後は CPU に戻る。人間が 0 人になれば卓は空卓に戻る） */
		for (s = 0; s < JANJAN_SEATS; s ++) {
			PCHARINFO pChar;
			int nSdx = 0, nSdy = 0;

			if (!IsHumanSeat (Table, s)) {
				continue;
			}
			pChar = HumanAt (Table, s);
			JanjanSeatOffset (s, &nSdx, &nSdy);
			if ((pChar == NULL) || (pChar->byGameState < GAMESTATE_MOVE) ||
				(pChar->dwMapID != Table.dwMapID) || !IsSameSpace (pNpc, pChar) ||
				(abs ((int)pChar->dwX - ((int)pNpc->dwX + nSdx)) > JAN_NEAR) ||
				(abs ((int)pChar->dwY - ((int)pNpc->dwY + nSdy)) > JAN_NEAR) ||
				(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
				RemoveHuman (i, s, TRUE);
				if (Table.nMode == JANMODE_IDLE) {
					break;
				}
			}
		}
		if (Table.nMode == JANMODE_IDLE) {
			continue;
		}
		/* 卓の近くにいる人（観戦）の見直し */
		if ((DWORD)(g_dwTime - Table.dwLastWatch) >= JAN_WATCH_MS) {
			WatchRefresh (i);
		}
		if (Table.nMode == JANMODE_RECRUIT) {
			if ((int)(g_dwTime - Table.dwLimit) >= 0) {
				StartGame (i);		/* 待ち時間が終わった：座っている人（1 人でも）で開始。空き席は CPU */
			}
			continue;
		}
		if (Table.nMode == JANMODE_OVER) {
			if ((int)(g_dwTime - Table.dwNext) >= 0) {
				CloseTable (i);		/* 全員が席を立つ */
			}
			continue;
		}
		/* 進行中：1 回に 1 動作だけ進める */
		if (Table.nStep == JANSTEP_DISCARD) {
			if (IsHumanSeat (Table, Table.Game.nTurn)) {
				if ((int)(g_dwTime - Table.dwLimit) >= 0) {
					DoHumanTimeoutDiscard (i);
				}
			} else if ((int)(g_dwTime - Table.dwNext) >= 0) {
				DoCpuDiscard (i);
			}
		} else if (Table.nStep == JANSTEP_CLAIM) {
			if (AnyClaimAsk (Table)) {
				/* 全員が返事するか時間切れ（返事なしは見送り）で、CPU の申し出と合わせて決める */
				if (!AnyClaimPending (Table) || ((int)(g_dwTime - Table.dwLimit) >= 0)) {
					ResolveClaim (i);
				}
			} else if ((int)(g_dwTime - Table.dwNext) >= 0) {
				ResolveClaim (i);
			}
		} else if ((int)(g_dwTime - Table.dwNext) >= 0) {
			DoDraw (i);
		}
	}
	/* 待機中の卓を新しく見つける（JAN_WATCH_MS ごと） */
	if ((DWORD)(g_dwTime - s_dwLastScan) >= JAN_WATCH_MS) {
		s_dwLastScan = g_dwTime;
		IdleScan ();
	}
}

void JanOnLeave(PCHARINFO pChar)
{
	int nTable, nSeat;

	s_mapAsk.erase (pChar->dwTblID);
	if (FindSeat (pChar, &nTable, &nSeat)) {
		RemoveHuman (nTable, nSeat, FALSE);
	}
}
