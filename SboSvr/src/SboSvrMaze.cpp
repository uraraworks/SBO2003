/* ========================================================================= */
/* ファイル名：	SboSvrMaze.cpp												 */
/* 概要：		迷路対戦の台（台NPC・席・ゲームの進行）の処理(実装ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 盤面ロジックは common/MazeAttack.h、台詞の読み方は common/MazeTable.h。台の作りは落ち物パズル（SboSvrPuzzle.cpp）にならう。
 * 台は、台詞が "@k" で始まる NPC（"@q" は依頼NPCの印なので使わない）:
 *     @k<dx>,<dy>,<sdx>,<sdy>,<床A>,<床B>,<隙間>,<sdx2>,<sdy2>      後ろは省略でき、空欄も既定になる
 *   dx, dy   … 盤（10×10 タイル＝320 ドット）の左上が、台NPCのマスの左上から何タイルか。省略は -5,-9
 *   sdx, sdy … 席 1（プレイヤー 0＝下側）の座る場所：台NPCのマスからの相対位置（マス）。省略は (0,+1)
 *   sdx2, sdy2 … 席 2（プレイヤー 1＝上側）の座る場所。省略は (0,-10)（盤の真上の 1 つ上）
 * 台は席が 2 つで盤は 1 枚を共有する。コインは無く、座る場所のマスに立って S キーで座る（無料の台）。
 *   片方だけが座っている間はひとり練習：座るとゲームを初期化し、相手（不在）の手番は 1 手指すたびに自動で飛ばす
 *   （MazeGame::Pass は「相手が動けない時」しか成立しないので、手番は直接座っている人に戻す）。
 *   ひとり練習中のところへもう一方が座ろうとすると「乱入しますか？」を聞く（会話の「はい・いいえ」@y。答えは CMD_TALKANSWER → MazeAnswer。
 *   MAZE_ASK_TIMEOUT_MS で無効）。はい＝盤を初期化して 2 人対戦（MAZE_VS_COUNTDOWN_SEC 秒のカウントダウンのあと開始。その間は操作無効）、いいえ＝座らない。
 *   対戦：手番はプレイヤー 0 から始まり 1 手ごとに交代。手番でない人の操作は無視。1 手 MAZE_TURN_MS で、時間切れはその手番を飛ばし、
 *   連続 MAZE_TIMEOUT_MAX 回で不戦敗。勝敗が決まると両者へ「○○の勝ち！」、盤を MAZE_OVER_SHOW_MS 見せて初期化し、
 *   続けて対戦（先手を交代）。席を立つ・切断・マップ移動・倒れた時は残った側の勝ちで、残った人はひとり練習に戻る。
 *   ひとり練習で勝ったら盤をそのまま MAZE_OVER_SHOW_MS だけ見せ（その間は操作を受けない）、過ぎたら席に座ったまま盤を初期状態に戻して続けられる。
 * ゲームの正はサーバー。盤・コマ・壁は変わるたびに全部送る（小さいので差分は作らない）。
 * 送る相手（台ごと）: 座っている人と台の近くにいる人（観戦。画面に入る距離。MAZE_WATCH_MS ごとに見直し、
 *   近づいた時に盤全体を 1 回送り、離れた時に NONE を送る）。席が空の待機中の台も、近くの人に初期配置の盤を送る。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrMaze.h"
#include "../../common/MazeAttack.h"
#include "../../common/MazeTable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>

#define MAZE_NOSEAT				((DWORD)-1)
#define MAZE_NEAR				(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define MAZE_WATCH_MS			(250)		/* 台の近くにいる人（観戦）を見直す間隔(ms) */
#define MAZE_OVER_SHOW_MS		(3000)		/* 勝ちを見せている時間(ms)。過ぎると盤を戻して続けられる */
#define MAZE_ASK_TIMEOUT_MS		(30000)		/* 「乱入しますか？」の問いが無効になるまで(ms) */
/* 盤を送る距離（横・縦それぞれ。盤の中心から）。盤の一部でも画面（地図 15×15 マス）にかかる距離は、画面の半分 7.5 マス＋歩きのずれ 1 マス
   ＋盤の半分（10 タイルなので 5 マス）＝13.5 マス。MARGIN＝9 に盤の半分 MAZE_VIEW_HALF（5）を足した 14 マスでかかる範囲は足りている。
   送り始めは MARGIN＋3、消すのは＋6 で、画面にかかっている間に届かない・先に消えることはない（境目で送る・消すを繰り返さない） */
#define MAZE_VIEW_MARGIN		(9)
#define MAZE_VIEW_HALF			((MAZE_TABLE_TILES + 1) / 2)	/* 盤の半分（マス。切り上げ。10 タイルなので 5） */
#define MAZE_VIEW_ENTER			(MAZE_VIEW_MARGIN + 3)
#define MAZE_VIEW_LEAVE			(MAZE_VIEW_MARGIN + 6)

/* 盤を送ってある人 */
typedef struct _MAZEWATCH {
	DWORD		dwChar;						/* キャラのテーブルID */
	DWORD		dwPID;						/* プレイヤーID */
} MAZEWATCH;

/* 台（座っている人がいる間と、待機中の台の近くに人がいる間、表に載せる） */
typedef struct _MAZETABLE {
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	DWORD		dwMapID;					/* 台NPCのいるマップ */
	int			nDx, nDy;					/* 盤の左上：台のマスの左上から（タイル） */
	int			anSdx[2], anSdy[2];			/* 座る場所：台のマスから（マス）。[0]=席 1（プレイヤー 0）・[1]=席 2（プレイヤー 1） */
	WORD		wFloorA, wFloorB, wGap;		/* 見た目のマップパーツID（床の市松 A・B、隙間。0 ならクライアントがコード描画） */
	DWORD		dwChar[2];					/* 席ごとの座っているキャラのテーブルID（いなければ MAZE_NOSEAT） */
	DWORD		dwPID[2];					/* そのキャラのプレイヤーID（テーブルIDの使い回しを見分ける） */
	MazeGame	Game;						/* ゲーム（待機中は初期配置） */
	BOOL		bIdle;						/* 待機中（席に誰もいない） */
	BOOL		bVs;						/* 2 人対戦中（両方の席に人がいて、乱入で始めた） */
	BOOL		bCount;						/* 対戦開始前のカウントダウン中 */
	DWORD		dwStartTime;				/* カウントダウンが終わる（対戦が始まる）時刻 */
	int			nCountSent;					/* 最後に知らせたカウントダウンの残り秒 */
	DWORD		dwTurnStart;				/* 今の手番が始まった時刻（対戦中） */
	int			nLeftSent;					/* 最後に知らせた手番の残り秒 */
	int			anTimeout[2];				/* 各プレイヤーの連続時間切れ回数 */
	int			nFirst;						/* 今の対戦（次に始める対戦）の先手（0 / 1。1 局ごとに交代） */
	BOOL		bWin;						/* 勝ちを見せている間 */
	DWORD		dwWinTime;					/* 勝った時刻 */
	DWORD		dwLastWatch;				/* 最後に観戦の人を見直した時刻 */
	std::vector<MAZEWATCH>	aWatch;			/* 盤を送ってある人（座っている人・近くにいる人） */
} MAZETABLE;

static std::vector<MAZETABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 席 nSeat に座っているキャラ（いなければ NULL） */
static PCHARINFO SeatChar(const MAZETABLE &Table, int nSeat)
{
	PCHARINFO pChar;

	if ((Table.dwChar[nSeat] == MAZE_NOSEAT) || (Table.dwChar[nSeat] >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	pChar = g_aCharInfo[Table.dwChar[nSeat]];
	if ((pChar == NULL) || (pChar->pID != Table.dwPID[nSeat])) {
		return NULL;
	}
	return pChar;
}

/* pChar が座っている席（座っていなければ -1） */
static int SeatOf(const MAZETABLE &Table, PCHARINFO pChar)
{
	int i;

	for (i = 0; i < 2; i ++) {
		if ((Table.dwChar[i] == pChar->dwTblID) && (Table.dwPID[i] == pChar->pID)) {
			return i;
		}
	}
	return -1;
}

/* ひとり練習で座っている側の席（誰もいなければ -1。2 人いる時は 0） */
static int SoleSeat(const MAZETABLE &Table)
{
	return (Table.dwChar[0] != MAZE_NOSEAT) ? 0 : (Table.dwChar[1] != MAZE_NOSEAT) ? 1 : -1;
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

/* 座っている台と席を探す（座っていなければ FALSE。pnSeat は省略可） */
static BOOL FindSeat(PCHARINFO pChar, int *pnTable, int *pnSeat = NULL)
{
	int i, nSeat;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		nSeat = SeatOf (s_aTable[i], pChar);
		if (nSeat >= 0) {
			*pnTable = i;
			if (pnSeat) {
				*pnSeat = nSeat;
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* pChar が台NPCの座る場所のマスに立っている席（0=席 1・1=席 2。どちらでもなければ -1）。台が使う座る場所の位置は台詞から読む */
static int SeatIndexAt(PCHARINFO pChar, PCHARINFO pNpc)
{
	int nDx, nDy, anSdx[2], anSdy[2], i;

	if (!MazeIsTableNpc (pNpc) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return -1;
	}
	MazeParseTable (pNpc->pszNpcSpeech, &nDx, &nDy, &anSdx[0], &anSdy[0]);
	MazeParseSeat2 (pNpc->pszNpcSpeech, &anSdx[1], &anSdy[1]);
	for (i = 0; i < 2; i ++) {
		if (((int)pChar->dwX == (int)pNpc->dwX + anSdx[i]) && ((int)pChar->dwY == (int)pNpc->dwY + anSdy[i])) {
			return i;
		}
	}
	return -1;
}

static PCHARINFO WatchChar(const MAZEWATCH &w)
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

static PCHARINFO NpcOf(const MAZETABLE &Table)
{
	return (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
}

/* 席が空になった台を待機中にする（表からは消さない）。盤は初期配置に戻す */
static void ToIdle(int nTable)
{
	MAZETABLE &Table = s_aTable[nTable];

	Table.dwChar[0] = Table.dwChar[1] = MAZE_NOSEAT;
	Table.dwPID[0] = Table.dwPID[1] = 0;
	Table.bIdle = TRUE;
	Table.bVs = FALSE;
	Table.bCount = FALSE;
	Table.bWin = FALSE;
	Table.nFirst = 0;
	Table.Game.Init ();
}

/* 対戦の手番の残り秒（対戦中で、カウントダウン・勝ち表示中・決着後でない時だけ。それ以外は 0） */
static int TurnLeft(const MAZETABLE &Table)
{
	DWORD dwEl;

	if (!Table.bVs || Table.bCount || Table.bWin || (Table.Game.GetWinner () != MAZE_NO_WINNER)) {
		return 0;
	}
	dwEl = g_dwTime - Table.dwTurnStart;
	if (dwEl >= (DWORD)MAZE_TURN_MS) {
		return 0;
	}
	return (int)(((DWORD)MAZE_TURN_MS - dwEl + 999) / 1000);
}

/* カウントダウンの残り秒（カウントダウン中でなければ 0） */
static int CountLeft(const MAZETABLE &Table)
{
	int n;

	if (!Table.bCount || (Table.dwStartTime <= g_dwTime)) {
		return 0;
	}
	n = (int)((Table.dwStartTime - g_dwTime + 999) / 1000);
	return (n > MAZE_VS_COUNTDOWN_SEC) ? MAZE_VS_COUNTDOWN_SEC : n;
}

/* 対戦（またはひとり練習）の盤を初期化する。nFirst は先手（ひとり練習は座っている側） */
static void ResetBoard(MAZETABLE &Table, int nTurn)
{
	Table.Game.Init ();
	Table.Game.byTurn = nTurn;
	Table.bWin = FALSE;
	Table.anTimeout[0] = Table.anTimeout[1] = 0;
}

/* 対戦の始め（または続き）：盤を初期化して、カウントダウンのあと nFirst から始める */
static void StartVs(MAZETABLE &Table)
{
	ResetBoard (Table, Table.nFirst);
	Table.bVs = TRUE;
	Table.bCount = TRUE;
	Table.dwStartTime = g_dwTime + MAZE_VS_COUNTDOWN_SEC * 1000;
	Table.dwTurnStart = Table.dwStartTime;
	Table.nCountSent = MAZE_VS_COUNTDOWN_SEC;
}

/* 台の状態のパケットを作る。byKind: FULL=盤全体 NONE=消す。byState の SEAT は送る相手ごとに付け替える */
static void BuildState(const MAZETABLE &Table, BYTE byKind, CMDPARA_MAZESTATE *pOut)
{
	const MazeGame &g = Table.Game;
	PCHARINFO pNpc = NpcOf (Table);
	int i, nX, nY;

	ZeroMemory (pOut, sizeof (*pOut));
	pOut->byCmd = CMD_MAZESTATE;
	pOut->byKind = byKind;
	pOut->dwNpc = Table.dwNpc;
	pOut->dwChar[0] = pOut->dwChar[1] = MAZE_NOCHAR;
	pOut->cWinner = -1;
	if (byKind == MAZEKIND_NONE) {
		return;
	}
	pOut->byState = (BYTE)((Table.bIdle ? MAZESTATE_IDLE : 0) | (Table.bWin ? MAZESTATE_WIN : 0));
	pOut->byTurn = (BYTE)g.byTurn;
	pOut->cWinner = (signed char)g.nWinner;
	for (i = 0; i < 2; i ++) {
		pOut->byPosX[i] = (BYTE)g.nPosX[i];
		pOut->byPosY[i] = (BYTE)g.nPosY[i];
		pOut->byWallLeft[i] = (BYTE)g.nWallLeft[i];
	}
	pOut->byWallCount = (BYTE)g.nWallCount;
	for (i = 0; i < g.nWallCount && i < MAZEPARA_WALL_MAX; i ++) {
		pOut->aWall[i].byX = (BYTE)g.aWall[i].nX;
		pOut->aWall[i].byY = (BYTE)g.aWall[i].nY;
		pOut->aWall[i].byOrient = (BYTE)g.aWall[i].nOrient;
		pOut->aWall[i].byOwner = (BYTE)g.aWall[i].nOwner;
	}
	pOut->byState |= (BYTE)((Table.bVs ? MAZESTATE_VS : 0) | (Table.bCount ? MAZESTATE_COUNT : 0));
	for (i = 0; i < 2; i ++) {
		pOut->dwChar[i] = Table.dwChar[i];		/* 席 1=プレイヤー 0・席 2=プレイヤー 1。空席は MAZE_NOSEAT（＝MAZE_NOCHAR） */
		pOut->byHasName[i] = (BYTE)((SeatChar (Table, i) != NULL) ? 1 : 0);
		pOut->byTimeouts[i] = (BYTE)Table.anTimeout[i];
	}
	pOut->dwTurnChar = (g.byTurn == 0 || g.byTurn == 1) ? Table.dwChar[g.byTurn] : MAZE_NOCHAR;
	pOut->byCount = (BYTE)CountLeft (Table);
	pOut->byTimeLeft = (BYTE)TurnLeft (Table);
	pOut->wFloor[0] = Table.wFloorA;
	pOut->wFloor[1] = Table.wFloorB;
	pOut->wGap = Table.wGap;
	if (pNpc) {
		MazeBoardPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.nDx, Table.nDy, &nX, &nY);
		pOut->sBoardX = (short)nX;
		pOut->sBoardY = (short)nY;
	}
}

/* 1 人に送る（SEAT はその人が席に座っているかで決める） */
static void SendStateTo(const MAZETABLE &Table, PCHARINFO pChar, CMDPARA_MAZESTATE Para)
{
	if ((Para.byKind == MAZEKIND_FULL) && (SeatOf (Table, pChar) >= 0)) {
		Para.byState |= MAZESTATE_SEAT;
	}
	SendCmdMAZESTATE (pChar->pID, &Para);
}

/* 台の状態を、盤を送ってある全員に送る。pSkip には送らない */
static void SendTable(int nTable, BYTE byKind, PCHARINFO pSkip = NULL)
{
	MAZETABLE &Table = s_aTable[nTable];
	CMDPARA_MAZESTATE Para;
	int i;

	Table.nLeftSent = TurnLeft (Table);
	BuildState (Table, byKind, &Para);
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar = WatchChar (Table.aWatch[i]);
		if ((pChar == NULL) || (pChar == pSkip)) {
			continue;
		}
		SendStateTo (Table, pChar, Para);
	}
}

/* 盤を送る距離にいるか（盤の中心から nMargin マス＋盤の半分の範囲） */
static BOOL WatchEligible(const MAZETABLE &Table, PCHARINFO pNpc, PCHARINFO pChar, int nMargin)
{
	int nFx, nFy, nCx, nCy;

	if ((pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) || (pChar->dwMapID != Table.dwMapID) ||
		!IsSameSpace (pNpc, pChar) || !IsCharSeenBy (pNpc, pChar)) {
		return FALSE;
	}
	MazeBoardPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.nDx, Table.nDy, &nFx, &nFy);
	nCx = (nFx + MAZE_TABLE_TILES * MAZE_TABLE_TILE_DOT / 2) / MAZE_TABLE_TILE_DOT;
	nCy = (nFy + MAZE_TABLE_TILES * MAZE_TABLE_TILE_DOT / 2) / MAZE_TABLE_TILE_DOT;
	return (abs ((int)pChar->dwX - nCx) <= nMargin + MAZE_VIEW_HALF) &&
		(abs ((int)pChar->dwY - nCy) <= nMargin + MAZE_VIEW_HALF);
}

static int FindWatch(const std::vector<MAZEWATCH> &aWatch, const MAZEWATCH &w)
{
	int i;

	for (i = 0; i < (int)aWatch.size (); i ++) {
		if ((aWatch[i].dwChar == w.dwChar) && (aWatch[i].dwPID == w.dwPID)) {
			return i;
		}
	}
	return -1;
}

/* 盤が見える人を見直す。新しく見える人には盤全体を 1 回送り、見えなくなった人には NONE を送って消してもらう */
static void WatchRefresh(int nTable)
{
	MAZETABLE &Table = s_aTable[nTable];
	PCHARINFO pNpc = NpcOf (Table);
	std::vector<MAZEWATCH> aNew;
	CMDPARA_MAZESTATE Full, None;
	PCHARINFO apSeat[2];
	PMAPINFO pMap;
	BOOL bFull = FALSE, bNone = FALSE;
	int i;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	/* 座っている人（2 席）は距離に関係なく入れる */
	for (i = 0; i < 2; i ++) {
		apSeat[i] = SeatChar (Table, i);
		if (apSeat[i]) {
			MAZEWATCH w;
			w.dwChar = apSeat[i]->dwTblID;
			w.dwPID = apSeat[i]->pID;
			aNew.push_back (w);
		}
	}
	/* 同じマップの近くにいる人 */
	pMap = (Table.dwMapID < (DWORD)g_aMapInfo.GetSize ()) ? g_aMapInfo[Table.dwMapID] : NULL;
	if (pMap && pMap->paTblNoPC) {
		for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
			PCHARINFO pChar;
			MAZEWATCH w;

			if (pMap->paTblNoPC[i] == (DWORD)-1) {
				break;
			}
			pChar = g_aCharInfo[pMap->paTblNoPC[i]];
			if ((pChar == apSeat[0]) || (pChar == apSeat[1])) {
				continue;
			}
			w.dwChar = pChar->dwTblID;
			w.dwPID = pChar->pID;
			/* すでに送ってある人は遠い距離（LEAVE）まで、新しい人は近い距離（ENTER）から */
			if (!WatchEligible (Table, pNpc, pChar, (FindWatch (Table.aWatch, w) >= 0) ? MAZE_VIEW_LEAVE : MAZE_VIEW_ENTER)) {
				continue;
			}
			aNew.push_back (w);
		}
	}
	/* 見えなくなった人へ NONE */
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar;
		if (FindWatch (aNew, Table.aWatch[i]) >= 0) {
			continue;
		}
		pChar = WatchChar (Table.aWatch[i]);
		if (pChar) {
			if (!bNone) {
				BuildState (Table, MAZEKIND_NONE, &None);
				bNone = TRUE;
			}
			SendStateTo (Table, pChar, None);
		}
	}
	/* 新しく見える人へ盤全体（すでに見ている人へは送り直さない） */
	for (i = 0; i < (int)aNew.size (); i ++) {
		PCHARINFO pChar;

		if (FindWatch (Table.aWatch, aNew[i]) >= 0) {
			continue;
		}
		pChar = WatchChar (aNew[i]);
		if (pChar == NULL) {
			continue;
		}
		if (!bFull) {
			BuildState (Table, MAZEKIND_FULL, &Full);
			bFull = TRUE;
		}
		SendStateTo (Table, pChar, Full);
	}
	Table.aWatch = aNew;
}

/* 2 人対戦の勝敗が決まった時（g.nWinner は設定済み）：勝ちを見せ始める。対戦中なら両者へ「○○の勝ち！」を送り、次の先手を交代する */
static void Decided(int nTable)
{
	MAZETABLE &Table = s_aTable[nTable];
	PCHARINFO pWin = SeatChar (Table, Table.Game.nWinner);
	char szMsg[96];
	int i;

	Table.bWin = TRUE;
	Table.dwWinTime = g_dwTime;
	if (!Table.bVs) {
		return;
	}
	sprintf (szMsg, "%sの勝ち！", pWin ? pWin->szName : "だれか");
	for (i = 0; i < 2; i ++) {
		PCHARINFO pChar = SeatChar (Table, i);
		if (pChar) {
			SendCmdSYSMSG (pChar->pID, szMsg);
		}
	}
	Table.nFirst = 1 - Table.nFirst;		/* 次の対戦は先手を交代 */
}

/* 席 nSeat の人を立たせる。bNotifySelf でなければ本人へは何も送らない（切断）。
   相手が残れば、対戦中だった時は残った側の勝ち（両者へ「○○の勝ち！」）で、残った人はひとり練習に戻る（盤は初期化）。
   誰もいなくなれば台は待機中（初期配置）に戻る。見ている人へは盤を送る */
static void Leave(int nTable, int nSeat, BOOL bNotifySelf)
{
	MAZETABLE &Table = s_aTable[nTable];
	PCHARINFO pSelf = SeatChar (Table, nSeat);
	PCHARINFO pOther = SeatChar (Table, 1 - nSeat);
	BOOL bDecide = Table.bVs && !Table.bWin;		/* 勝ちを見せている間に立った時は、もう勝敗を伝えてある */
	char szMsg[96];

	Table.dwChar[nSeat] = MAZE_NOSEAT;
	Table.dwPID[nSeat] = 0;
	if (Table.dwChar[1 - nSeat] == MAZE_NOSEAT) {
		ToIdle (nTable);
	} else {
		Table.bVs = FALSE;
		Table.bCount = FALSE;
		ResetBoard (Table, 1 - nSeat);
		if (bDecide) {
			sprintf (szMsg, "%sの勝ち！", pOther ? pOther->szName : "だれか");
			if (pOther) {
				SendCmdSYSMSG (pOther->pID, szMsg);
			}
			if (pSelf && bNotifySelf) {
				SendCmdSYSMSG (pSelf->pID, szMsg);
			}
		}
	}
	SendTable (nTable, MAZEKIND_FULL, bNotifySelf ? NULL : pSelf);
}

/* 席 nSeat の本人に今の状態を送り直す（不正な操作を無視した時。クライアントの見た目をサーバーにそろえさせる） */
static void SendBack(int nTable, int nSeat)
{
	MAZETABLE &Table = s_aTable[nTable];
	PCHARINFO pChar = SeatChar (Table, nSeat);
	CMDPARA_MAZESTATE Para;

	if (pChar == NULL) {
		return;
	}
	BuildState (Table, MAZEKIND_FULL, &Para);
	SendStateTo (Table, pChar, Para);
}

/* 待機中の台を表に作る（盤は初期配置） */
static void InitTable(MAZETABLE *pTable, PCHARINFO pNpc, DWORD dwMapID)
{
	pTable->dwNpc = pNpc->dwTblID;
	pTable->dwMapID = dwMapID;
	MazeParseTable (pNpc->pszNpcSpeech, &pTable->nDx, &pTable->nDy, &pTable->anSdx[0], &pTable->anSdy[0]);
	MazeParseSeat2 (pNpc->pszNpcSpeech, &pTable->anSdx[1], &pTable->anSdy[1]);
	MazeParseParts (pNpc->pszNpcSpeech, &pTable->wFloorA, &pTable->wFloorB, &pTable->wGap);
	pTable->dwChar[0] = pTable->dwChar[1] = MAZE_NOSEAT;
	pTable->dwPID[0] = pTable->dwPID[1] = 0;
	pTable->bIdle = TRUE;
	pTable->bVs = FALSE;
	pTable->bCount = FALSE;
	pTable->dwStartTime = 0;
	pTable->nCountSent = 0;
	pTable->dwTurnStart = 0;
	pTable->nLeftSent = 0;
	pTable->anTimeout[0] = pTable->anTimeout[1] = 0;
	pTable->nFirst = 0;
	pTable->bWin = FALSE;
	pTable->dwWinTime = 0;
	pTable->dwLastWatch = 0;
	pTable->Game.Init ();
}

/* ========================================================================= */
/* 外から呼ぶ関数															 */
/* ========================================================================= */

BOOL MazeIsTableNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && (pNpc->byNpcType != NPCTYPE_QUESTNPC) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'k');
}

BOOL MazeOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc)
{
	return SeatIndexAt (pChar, pNpc) >= 0;
}

BOOL MazeIsSeated(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable);
}

MazeGame *MazeGetGame(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable) ? &s_aTable[nTable].Game : NULL;
}

int MazeSeatCount(void)
{
	int i, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		nRet += (s_aTable[i].dwChar[0] != MAZE_NOSEAT) + (s_aTable[i].dwChar[1] != MAZE_NOSEAT);
	}
	return nRet;
}

int MazeIdleCount(void)
{
	int i, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		nRet += s_aTable[i].bIdle ? 1 : 0;
	}
	return nRet;
}

/* 「乱入しますか？」の問い（座ろうとした人ごとに 1 つ。答えは MazeAnswer。MAZE_ASK_TIMEOUT_MS たったら捨てる） */
typedef struct _MAZEASK {
	DWORD	dwNpc;				/* 座ろうとした台NPCのテーブルID */
	DWORD	dwTime;				/* 問いを出した時刻 */
} MAZEASK;
static std::map<DWORD, MAZEASK> s_mapAsk;

BOOL MazeAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes)
{
	std::map<DWORD, MAZEASK>::iterator it;
	MAZEASK Ask;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return FALSE;
	}
	it = s_mapAsk.find (pChar->dwTblID);
	if ((it == s_mapAsk.end ()) || (it->second.dwNpc != dwNpcTblID)) {
		return FALSE;
	}
	Ask = it->second;
	s_mapAsk.erase (it);
	if (((DWORD)(g_dwTime - Ask.dwTime) > (DWORD)MAZE_ASK_TIMEOUT_MS) || (dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return TRUE;
	}
	/* 座る条件は MazeSit が改めて確かめる（その間に席が埋まった・立つ場所を離れた時は座らない）。はい=乱入、いいえ=座らない */
	MazeSit (pChar, g_aCharInfo[dwNpcTblID], bYes ? 1 : 2);
	return TRUE;
}

void MazeSit(PCHARINFO pChar, PCHARINFO pNpc, int nChoice)
{
	int nTable, nSeat;
	BOOL bOther;
	PCHARINFO pOther = NULL;

	if (!MazeIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || MazeIsSeated (pChar) || PuzzleIsSeated (pChar)) {
		return;
	}
	/* 座れるのは、台の座る場所（席 1・席 2）のマスの上だけ（同じマップ・同じ空間。向きは問わない） */
	nSeat = SeatIndexAt (pChar, pNpc);
	if (nSeat < 0) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}
	/* その席が使われていれば座れない（待機中の台は座れる） */
	nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
	if ((nTable >= 0) && (s_aTable[nTable].dwChar[nSeat] != MAZE_NOSEAT)) {
		SendCmdSYSMSG (pChar->pID, "この台は今、使われています。");
		return;
	}
	/* もう一方の席でひとりが遊んでいれば、乱入するかを聞く */
	if (nTable >= 0) {
		pOther = SeatChar (s_aTable[nTable], 1 - nSeat);
	}
	bOther = (pOther != NULL);
	if (bOther && (nChoice == 0)) {
		MAZEASK Ask;
		char szAsk[160];

		Ask.dwNpc = pNpc->dwTblID;
		Ask.dwTime = g_dwTime;
		s_mapAsk[pChar->dwTblID] = Ask;
		sprintf (szAsk, "%sさんが遊んでいます。乱入して対戦しますか？（「いいえ」で座りません）@y", pOther->szName);
		SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szAsk);
		return;
	}
	s_mapAsk.erase (pChar->dwTblID);
	if (nChoice == 2) {
		return;			/* いいえ：座らない */
	}
	if (nTable < 0) {
		MAZETABLE Table;
		InitTable (&Table, pNpc, pNpc->dwMapID);
		s_aTable.push_back (Table);
		nTable = (int)s_aTable.size () - 1;
	}
	{
		MAZETABLE &Table = s_aTable[nTable];

		Table.dwChar[nSeat] = pChar->dwTblID;
		Table.dwPID[nSeat] = pChar->pID;
		Table.bIdle = FALSE;
		if (bOther) {
			/* 乱入：盤を初期化して、カウントダウンのあと対戦（先手はプレイヤー 0） */
			char szMsg[96];

			Table.nFirst = 0;
			StartVs (Table);
			sprintf (szMsg, "対戦開始！ 相手は%sです。", pOther->szName);
			SendCmdSYSMSG (pChar->pID, szMsg);
			sprintf (szMsg, "対戦開始！ 相手は%sです。", pChar->szName);
			SendCmdSYSMSG (pOther->pID, szMsg);
		} else {
			ResetBoard (Table, nSeat);		/* ひとり練習：座った人の側から（相手の手番は自動で飛ばす） */
			Table.bVs = FALSE;
			Table.bCount = FALSE;
		}
	}
	/* 待機中から見ていた人（座った本人を含む）へ始まった盤。新しく見える人へは見直しで届く */
	SendTable (nTable, MAZEKIND_FULL);
	WatchRefresh (nTable);
}

void MazeKey(PCHARINFO pChar, BYTE byAct, BYTE byDir, BYTE byX, BYTE byY, BYTE byOrient)
{
	int nTable, nSeat;
	BOOL bOK = FALSE;

	if (!FindSeat (pChar, &nTable, &nSeat)) {
		return;
	}
	MAZETABLE &Table = s_aTable[nTable];
	MazeGame &g = Table.Game;

	if (byAct == MAZEACT_LEAVE) {
		Leave (nTable, nSeat, TRUE);
		return;
	}
	/* 勝ちを見せている間・カウントダウン中・自分の手番でない時（対戦）は、どの操作も受けない */
	if (Table.bWin || Table.bCount || (Table.bVs && (g.byTurn != nSeat))) {
		SendBack (nTable, nSeat);
		return;
	}
	switch (byAct) {
	case MAZEACT_MOVE:
		bOK = g.Move (nSeat, (int)byDir);
		break;
	case MAZEACT_WALL:
		bOK = g.PlaceWall (nSeat, (int)byX, (int)byY, (int)byOrient);
		break;
	case MAZEACT_RESTART:
		if (!Table.bVs) {
			ResetBoard (Table, nSeat);			/* ひとり練習だけ。対戦中は片方の都合ではやり直せない */
			SendTable (nTable, MAZEKIND_FULL);
		} else {
			SendBack (nTable, nSeat);
		}
		return;
	default:
		break;
	}
	if (!bOK) {
		/* 不正（動けない・壁を置けない・手番でない・知らない操作）は無視して、状態を送り直す */
		SendBack (nTable, nSeat);
		return;
	}
	Table.anTimeout[nSeat] = 0;					/* 指したので連続時間切れは途切れる */
	if (g.GetWinner () == nSeat) {
		Decided (nTable);
	} else if (Table.bVs) {
		Table.dwTurnStart = g_dwTime;
		if (g.NeedPass ()) {
			g.Pass (g.byTurn);					/* 相手が動けず壁も置けない時は手番が戻る */
		}
	} else {
		g.byTurn = nSeat;						/* 相手は不在なので、その手番は飛ばす */
	}
	SendTable (nTable, MAZEKIND_FULL);
}

/* 待機中の台を新しく表に載せる：人のいるマップの台NPC（席が空）のうち、近くに人がいるものだけ */
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
			MAZETABLE Table;
			BOOL bNear = FALSE;

			if (pMap->paTblNoNPC[i] == (DWORD)-1) {
				break;
			}
			if (pMap->paTblNoNPC[i] >= (DWORD)g_aCharInfo.GetSize ()) {
				continue;
			}
			pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
			if (!MazeIsTableNpc (pNpc) || (FindTable (pNpc->dwTblID, (DWORD)nMap) >= 0)) {
				continue;
			}
			InitTable (&Table, pNpc, (DWORD)nMap);
			for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
				PCHARINFO pChar;

				if (pMap->paTblNoPC[j] == (DWORD)-1) {
					break;
				}
				if (pMap->paTblNoPC[j] >= (DWORD)g_aCharInfo.GetSize ()) {
					continue;
				}
				pChar = g_aCharInfo[pMap->paTblNoPC[j]];
				if (WatchEligible (Table, pNpc, pChar, MAZE_VIEW_ENTER)) {
					bNear = TRUE;
					break;
				}
			}
			if (bNear) {
				s_aTable.push_back (Table);
				WatchRefresh ((int)s_aTable.size () - 1);
			}
		}
	}
}

void MazeProc(void)
{
	static DWORD s_dwLastScan = 0;
	int i, nSeat;

	/* 台が消えると添字がずれるので、後ろから 1 つずつ */
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		if (i >= (int)s_aTable.size ()) {
			continue;
		}
		MAZETABLE &Table = s_aTable[i];
		PCHARINFO pNpc = NpcOf (Table);
		BOOL bNpcOK = MazeIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);

		/* 待機中の台：台が無くなれば見ている人へ NONE を送って表から消す。近くの人の見直しをして、誰もいなくなれば表から消す */
		if (Table.bIdle) {
			if (!bNpcOK) {
				CMDPARA_MAZESTATE None;
				int k;

				BuildState (Table, MAZEKIND_NONE, &None);
				for (k = 0; k < (int)Table.aWatch.size (); k ++) {
					PCHARINFO pW = WatchChar (Table.aWatch[k]);
					if (pW) {
						SendStateTo (Table, pW, None);
					}
				}
				s_aTable.erase (s_aTable.begin () + i);
				continue;
			}
			if ((DWORD)(g_dwTime - Table.dwLastWatch) >= MAZE_WATCH_MS) {
				WatchRefresh (i);
			}
			if (Table.aWatch.empty ()) {
				s_aTable.erase (s_aTable.begin () + i);
			}
			continue;
		}

		/* 切断・マップ移動・席から離れた・倒れた・台が無くなった時は片付ける（対戦中なら残った側の勝ち） */
		for (nSeat = 0; nSeat < 2; nSeat ++) {
			PCHARINFO pChar;

			if (Table.dwChar[nSeat] == MAZE_NOSEAT) {
				continue;
			}
			pChar = SeatChar (Table, nSeat);
			if ((pChar == NULL) || !bNpcOK || (pChar->byGameState < GAMESTATE_MOVE) ||
				(pChar->dwMapID != Table.dwMapID) ||
				(abs ((int)pChar->dwX - ((int)pNpc->dwX + Table.anSdx[nSeat])) > MAZE_NEAR) ||
				(abs ((int)pChar->dwY - ((int)pNpc->dwY + Table.anSdy[nSeat])) > MAZE_NEAR) ||
				(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
				Leave (i, nSeat, TRUE);
			}
		}
		if (Table.bIdle) {
			continue;
		}
		/* 台の近くにいる人（観戦）の見直し */
		if ((DWORD)(g_dwTime - Table.dwLastWatch) >= MAZE_WATCH_MS) {
			WatchRefresh (i);
		}
		/* 勝ちを見せ終えたら、座ったまま盤を初期状態に戻して続けられるようにする（対戦中は、先手を替えてカウントダウンから続ける） */
		if (Table.bWin && ((DWORD)(g_dwTime - Table.dwWinTime) >= (DWORD)MAZE_OVER_SHOW_MS)) {
			if (Table.bVs) {
				StartVs (Table);
			} else {
				ResetBoard (Table, SoleSeat (Table));
			}
			SendTable (i, MAZEKIND_FULL);
			continue;
		}
		/* 2 人対戦：カウントダウン（残り秒が変わるたびに知らせ、0 で開始）・手番の時間切れ・残り秒の更新 */
		if (Table.bVs && !Table.bWin) {
			if (Table.bCount) {
				if (g_dwTime >= Table.dwStartTime) {
					Table.bCount = FALSE;
					Table.dwTurnStart = g_dwTime;
					SendTable (i, MAZEKIND_FULL);
				} else if (CountLeft (Table) != Table.nCountSent) {
					Table.nCountSent = CountLeft (Table);
					SendTable (i, MAZEKIND_FULL);
				}
			} else if ((DWORD)(g_dwTime - Table.dwTurnStart) >= (DWORD)MAZE_TURN_MS) {
				int nTurn = Table.Game.byTurn;
				PCHARINFO pLate = SeatChar (Table, nTurn);

				Table.anTimeout[nTurn] ++;
				if (Table.anTimeout[nTurn] >= MAZE_TIMEOUT_MAX) {
					/* 連続で時間切れ：不戦敗（勝敗が決まる。勝ちの表示のあと対戦が続く） */
					if (pLate) {
						SendCmdSYSMSG (pLate->pID, "時間切れが続いたので、負けになりました。");
					}
					Table.Game.nWinner = 1 - nTurn;
					Decided (i);
				} else {
					char szMsg[96];

					sprintf (szMsg, "時間切れです。手番を飛ばします（連続%d回）。", Table.anTimeout[nTurn]);
					if (pLate) {
						SendCmdSYSMSG (pLate->pID, szMsg);
					}
					Table.Game.byTurn = 1 - nTurn;
					Table.dwTurnStart = g_dwTime;
				}
				SendTable (i, MAZEKIND_FULL);
			} else if (TurnLeft (Table) != Table.nLeftSent) {
				SendTable (i, MAZEKIND_FULL);			/* 手番の残り秒が変わった */
			}
		}
	}
	/* 待機中の台を新しく見つける（MAZE_WATCH_MS ごと） */
	if ((DWORD)(g_dwTime - s_dwLastScan) >= MAZE_WATCH_MS) {
		s_dwLastScan = g_dwTime;
		IdleScan ();
	}
}

void MazeOnLeave(PCHARINFO pChar)
{
	int nTable, nSeat;

	if (FindSeat (pChar, &nTable, &nSeat)) {
		Leave (nTable, nSeat, FALSE);
	}
}
