/* ========================================================================= */
/* ファイル名：	SboSvrMaze.cpp												 */
/* 概要：		迷路対戦の台（台NPC・席・ゲームの進行）の処理(実装ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 盤面ロジックは common/MazeAttack.h、台詞の読み方は common/MazeTable.h。台の作りは落ち物パズル（SboSvrPuzzle.cpp）にならう。
 * 台は、台詞が "@k" で始まる NPC（"@q" は依頼NPCの印なので使わない）:
 *     @k<dx>,<dy>,<sdx>,<sdy>      後ろは省略でき、空欄も既定になる
 *   dx, dy   … 盤（10×10 タイル＝320 ドット）の左上が、台NPCのマスの左上から何タイルか。省略は -5,-9
 *   sdx, sdy … 座る場所：台NPCのマスからの相対位置（マス）。省略は (0,+1)
 * 今は段階 1（ひとり練習）。コインは無く、座る場所のマスに立って S キーで座る（無料の台）。
 *   座るとゲームを初期化し、座った人がプレイヤー 0（下側スタート）。プレイヤー 1 は不在で、プレイヤー 0 が 1 手指すたびに手番を戻す
 *   （MazeGame::Pass は「相手が動けない時」しか成立しないので、手番は直接 0 に戻す）。
 *   勝ったら盤をそのまま MAZE_OVER_SHOW_MS だけ見せ（その間は操作を受けない）、過ぎたら席に座ったまま盤を初期状態に戻して続けられる。
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

#define MAZE_NOSEAT				((DWORD)-1)
#define MAZE_NEAR				(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define MAZE_WATCH_MS			(250)		/* 台の近くにいる人（観戦）を見直す間隔(ms) */
#define MAZE_OVER_SHOW_MS		(3000)		/* 勝ちを見せている時間(ms)。過ぎると盤を戻して続けられる */
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
	int			nSdx, nSdy;					/* 座る場所：台のマスから（マス） */
	DWORD		dwChar;						/* 座っているキャラのテーブルID（いなければ MAZE_NOSEAT） */
	DWORD		dwPID;						/* そのキャラのプレイヤーID（テーブルIDの使い回しを見分ける） */
	MazeGame	Game;						/* ゲーム（待機中は初期配置） */
	BOOL		bIdle;						/* 待機中（席に誰もいない） */
	BOOL		bWin;						/* 勝ちを見せている間 */
	DWORD		dwWinTime;					/* 勝った時刻 */
	DWORD		dwLastWatch;				/* 最後に観戦の人を見直した時刻 */
	std::vector<MAZEWATCH>	aWatch;			/* 盤を送ってある人（座っている人・近くにいる人） */
} MAZETABLE;

static std::vector<MAZETABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

static PCHARINFO SeatChar(const MAZETABLE &Table)
{
	PCHARINFO pChar;

	if ((Table.dwChar == MAZE_NOSEAT) || (Table.dwChar >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	pChar = g_aCharInfo[Table.dwChar];
	if ((pChar == NULL) || (pChar->pID != Table.dwPID)) {
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

/* 座っている台を探す（座っていなければ FALSE） */
static BOOL FindSeat(PCHARINFO pChar, int *pnTable)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwChar == pChar->dwTblID) && (s_aTable[i].dwPID == pChar->pID)) {
			*pnTable = i;
			return TRUE;
		}
	}
	return FALSE;
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

	Table.dwChar = MAZE_NOSEAT;
	Table.dwPID = 0;
	Table.bIdle = TRUE;
	Table.bWin = FALSE;
	Table.Game.Init ();
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
	}
	if (!Table.bIdle) {
		pOut->dwChar[0] = Table.dwChar;			/* 座った人がプレイヤー 0。プレイヤー 1 は不在（MAZE_NOCHAR） */
	}
	if (pNpc) {
		MazeBoardPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.nDx, Table.nDy, &nX, &nY);
		pOut->sBoardX = (short)nX;
		pOut->sBoardY = (short)nY;
	}
}

/* 1 人に送る（SEAT はその人が席に座っているかで決める） */
static void SendStateTo(const MAZETABLE &Table, PCHARINFO pChar, CMDPARA_MAZESTATE Para)
{
	if ((Para.byKind == MAZEKIND_FULL) && (SeatChar (Table) == pChar)) {
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
	PCHARINFO pSeat;
	PMAPINFO pMap;
	BOOL bFull = FALSE, bNone = FALSE;
	int i;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	/* 座っている人は距離に関係なく入れる */
	pSeat = SeatChar (Table);
	if (pSeat) {
		MAZEWATCH w;
		w.dwChar = pSeat->dwTblID;
		w.dwPID = pSeat->pID;
		aNew.push_back (w);
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
			if (pChar == pSeat) {
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

/* 席を立たせる。bNotifySelf でなければ本人へは何も送らない（切断）。台は待機中（初期配置）に戻り、見ている人へ送る */
static void Leave(int nTable, BOOL bNotifySelf)
{
	PCHARINFO pSelf = SeatChar (s_aTable[nTable]);

	ToIdle (nTable);
	SendTable (nTable, MAZEKIND_FULL, bNotifySelf ? NULL : pSelf);
}

/* 本人に今の状態を送り直す（不正な操作を無視した時。クライアントの見た目をサーバーにそろえさせる） */
static void SendBack(int nTable)
{
	MAZETABLE &Table = s_aTable[nTable];
	PCHARINFO pChar = SeatChar (Table);
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
	MazeParseTable (pNpc->pszNpcSpeech, &pTable->nDx, &pTable->nDy, &pTable->nSdx, &pTable->nSdy);
	pTable->dwChar = MAZE_NOSEAT;
	pTable->dwPID = 0;
	pTable->bIdle = TRUE;
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
	int nDx, nDy, nSdx, nSdy;

	if (!MazeIsTableNpc (pNpc) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return FALSE;
	}
	MazeParseTable (pNpc->pszNpcSpeech, &nDx, &nDy, &nSdx, &nSdy);
	return ((int)pChar->dwX == (int)pNpc->dwX + nSdx) && ((int)pChar->dwY == (int)pNpc->dwY + nSdy);
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
		if (!s_aTable[i].bIdle) {
			nRet ++;
		}
	}
	return nRet;
}

int MazeIdleCount(void)
{
	return (int)s_aTable.size () - MazeSeatCount ();
}

void MazeSit(PCHARINFO pChar, PCHARINFO pNpc)
{
	int nTable;

	if (!MazeIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || MazeIsSeated (pChar) || PuzzleIsSeated (pChar)) {
		return;
	}
	/* 座れるのは、台の座る場所のマスの上だけ（同じマップ・同じ空間。向きは問わない） */
	if (!MazeOnSeatCell (pChar, pNpc)) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}
	/* この台は席が 1 つ。使われていれば座れない（待機中の台は座れる） */
	nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
	if ((nTable >= 0) && !s_aTable[nTable].bIdle) {
		SendCmdSYSMSG (pChar->pID, "この台は今、使われています。");
		return;
	}
	if (nTable < 0) {
		MAZETABLE Table;
		InitTable (&Table, pNpc, pNpc->dwMapID);
		s_aTable.push_back (Table);
		nTable = (int)s_aTable.size () - 1;
	}
	{
		MAZETABLE &Table = s_aTable[nTable];

		Table.dwChar = pChar->dwTblID;
		Table.dwPID = pChar->pID;
		Table.bIdle = FALSE;
		Table.bWin = FALSE;
		Table.Game.Init ();			/* 座った人がプレイヤー 0（下側スタート・先手） */
	}
	/* 待機中から見ていた人（座った本人を含む）へ始まった盤。新しく見える人へは見直しで届く */
	SendTable (nTable, MAZEKIND_FULL);
	WatchRefresh (nTable);
}

void MazeKey(PCHARINFO pChar, BYTE byAct, BYTE byDir, BYTE byX, BYTE byY, BYTE byOrient)
{
	int nTable;
	BOOL bOK = FALSE;

	if (!FindSeat (pChar, &nTable)) {
		return;
	}
	MAZETABLE &Table = s_aTable[nTable];
	MazeGame &g = Table.Game;

	if (byAct == MAZEACT_LEAVE) {
		Leave (nTable, TRUE);
		return;
	}
	/* 勝ちを見せている間は、どの操作も受けない */
	if (Table.bWin) {
		SendBack (nTable);
		return;
	}
	switch (byAct) {
	case MAZEACT_MOVE:
		bOK = g.Move (0, (int)byDir);
		break;
	case MAZEACT_WALL:
		bOK = g.PlaceWall (0, (int)byX, (int)byY, (int)byOrient);
		break;
	case MAZEACT_RESTART:
		g.Init ();
		SendTable (nTable, MAZEKIND_FULL);
		return;
	default:
		break;
	}
	if (!bOK) {
		/* 不正（動けない・壁を置けない・手番でない・知らない操作）は無視して、状態を送り直す */
		SendBack (nTable);
		return;
	}
	if (g.GetWinner () == 0) {
		Table.bWin = TRUE;
		Table.dwWinTime = g_dwTime;
	} else {
		g.byTurn = 0;				/* プレイヤー 1 は不在なので、その手番は飛ばす */
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
	int i;

	/* 台が消えると添字がずれるので、後ろから 1 つずつ */
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		if (i >= (int)s_aTable.size ()) {
			continue;
		}
		MAZETABLE &Table = s_aTable[i];
		PCHARINFO pNpc = NpcOf (Table);
		BOOL bNpcOK = MazeIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);
		PCHARINFO pChar = SeatChar (Table);

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

		/* 切断・マップ移動・席から離れた・倒れた・台が無くなった時は片付ける */
		if ((pChar == NULL) || !bNpcOK || (pChar->byGameState < GAMESTATE_MOVE) ||
			(pChar->dwMapID != Table.dwMapID) ||
			(abs ((int)pChar->dwX - ((int)pNpc->dwX + Table.nSdx)) > MAZE_NEAR) ||
			(abs ((int)pChar->dwY - ((int)pNpc->dwY + Table.nSdy)) > MAZE_NEAR) ||
			(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
			Leave (i, TRUE);
			continue;
		}
		/* 台の近くにいる人（観戦）の見直し */
		if ((DWORD)(g_dwTime - Table.dwLastWatch) >= MAZE_WATCH_MS) {
			WatchRefresh (i);
		}
		/* 勝ちを見せ終えたら、座ったまま盤を初期状態に戻して続けられるようにする */
		if (Table.bWin && ((DWORD)(g_dwTime - Table.dwWinTime) >= (DWORD)MAZE_OVER_SHOW_MS)) {
			Table.bWin = FALSE;
			Table.Game.Init ();
			SendTable (i, MAZEKIND_FULL);
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
	int nTable;

	if (FindSeat (pChar, &nTable)) {
		Leave (nTable, FALSE);
	}
}
