/* ========================================================================= */
/* ファイル：	MazeAttack.h												 */
/* 概要：		迷路対戦（7×7・2 人）の盤面ロジック（純粋ロジック）			 */
/*				コマを十字に 1 マス進めるか、壁を 1 枚置くかで、相手より先に	 */
/*				向こう側の行へ着いた方が勝ち。通信・描画は含まない。			 */
/*				単体テスト: tests/server/maze_attack_test.cpp					 */
/* 日付：		2026/11/26													 */
/* ========================================================================= */

#pragma once

#include <string.h>

/* ========================================================================= */
/* 調整しやすい定数															 */
/* ========================================================================= */
#define MAZE_SIZE		(7)		/* 盤の一辺のマス数 */
#define MAZE_WALLS		(6)		/* 1 人が置ける壁の枚数 */
#define MAZE_WALL_MAX	(MAZE_WALLS * 2)	/* 盤に置かれる壁の最大数（2 人分） */
#define MAZE_H			(0)		/* 横壁：y 行と y+1 行の境目を x〜x+1 列にわたって塞ぐ */
#define MAZE_V			(1)		/* 縦壁：x 列と x+1 列の境目を y〜y+1 行にわたって塞ぐ */
#define MAZE_UP			(0)		/* 進む向き：上（y が減る） */
#define MAZE_DOWN		(1)		/* 下（y が増える） */
#define MAZE_LEFT		(2)		/* 左（x が減る） */
#define MAZE_RIGHT		(3)		/* 右（x が増える） */
#define MAZE_NO_WINNER	(-1)	/* 勝負がまだついていない */
#define MAZE_TURN_MS	(30000)	/* 2 人対戦：1 手の制限時間(ms)。過ぎるとその手番は飛ばされる */
#define MAZE_TIMEOUT_MAX	(3)	/* 2 人対戦：連続でこの回数、時間切れにした人は不戦敗 */
#define MAZE_VS_COUNTDOWN_SEC	(3)	/* 2 人対戦：始まる前のカウントダウン（秒。3・2・1。その間は操作できない） */

static const int g_anMazeDx[4] = { 0, 0, -1, 1 };
static const int g_anMazeDy[4] = { -1, 1, 0, 0 };

/* 壁 1 枚（交点 (x, y) は 0..MAZE_SIZE-2、nOrient は MAZE_H / MAZE_V） */
struct MazeWall {
	int nX, nY, nOrient;
	int nOwner;						/* 置いたプレイヤー（0 / 1。壁の絵の色分けに使う） */
};

/* ========================================================================= */
/* 盤面（1 試合分）															 */
/* ========================================================================= */
struct MazeGame {
	int nPosX[2], nPosY[2];			/* 各プレイヤーのコマ位置 */
	int nWallLeft[2];				/* 各プレイヤーの壁の残り枚数 */
	int nWallCount;					/* 置かれた壁の数 */
	MazeWall aWall[MAZE_WALL_MAX];	/* 置かれた壁のリスト（置いた順） */
	int byTurn;						/* 今の手番（0 / 1） */
	int nWinner;					/* 勝者（MAZE_NO_WINNER なら未決着） */

	/* 初期化：P0 は (3,6)、P1 は (3,0)。先手は P0 */
	void Init ()
	{
		nPosX[0] = MAZE_SIZE / 2;	nPosY[0] = MAZE_SIZE - 1;
		nPosX[1] = MAZE_SIZE / 2;	nPosY[1] = 0;
		nWallLeft[0] = nWallLeft[1] = MAZE_WALLS;
		nWallCount = 0;
		memset (aWall, 0, sizeof (aWall));
		byTurn = 0;
		nWinner = MAZE_NO_WINNER;
	}

	int GetWinner () const { return nWinner; }

	/* プレイヤー who のゴール行 */
	static int GoalRow (int who) { return who == 0 ? 0 : MAZE_SIZE - 1; }

	/* マス (x, y) から dir へ 1 歩の辺が壁で塞がれているか（盤の外は見ない） */
	bool EdgeBlocked (int x, int y, int dir) const
	{
		int i;
		for (i = 0; i < nWallCount; i ++) {
			const MazeWall &w = aWall[i];
			if (dir == MAZE_UP || dir == MAZE_DOWN) {
				int row = (dir == MAZE_UP) ? y - 1 : y;		/* 境目の上側の行 */
				if (w.nOrient == MAZE_H && w.nY == row && (w.nX == x || w.nX + 1 == x)) {
					return true;
				}
			} else {
				int col = (dir == MAZE_LEFT) ? x - 1 : x;	/* 境目の左側の列 */
				if (w.nOrient == MAZE_V && w.nX == col && (w.nY == y || w.nY + 1 == y)) {
					return true;
				}
			}
		}
		return false;
	}

	/* 壁 1 枚だけを追加した状態で who のコマ位置からゴール行へ着けるか（コマは無視） */
	bool PathExists (int sx, int sy, int who) const
	{
		bool seen[MAZE_SIZE][MAZE_SIZE];
		int qx[MAZE_SIZE * MAZE_SIZE], qy[MAZE_SIZE * MAZE_SIZE];
		int head = 0, tail = 0, d;
		memset (seen, 0, sizeof (seen));
		seen[sy][sx] = true;
		qx[tail] = sx;  qy[tail ++] = sy;
		while (head < tail) {
			int x = qx[head], y = qy[head ++];
			if (y == GoalRow (who)) {
				return true;
			}
			for (d = 0; d < 4; d ++) {
				int nx = x + g_anMazeDx[d], ny = y + g_anMazeDy[d];
				if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
				if (seen[ny][nx] || EdgeBlocked (x, y, d)) continue;
				seen[ny][nx] = true;
				qx[tail] = nx;  qy[tail ++] = ny;
			}
		}
		return false;
	}

	/* who がゴール行へ着く経路が（壁だけで）あるか */
	bool HasPath (int who) const
	{
		return PathExists (nPosX[who], nPosY[who], who);
	}

	/* (x, y, orient) に壁を置けるか（残り枚数と手番は見ない。盤面の条件だけ） */
	bool CanPlaceWall (int x, int y, int orient)
	{
		int i;
		if (orient != MAZE_H && orient != MAZE_V) return false;
		if (x < 0 || x > MAZE_SIZE - 2 || y < 0 || y > MAZE_SIZE - 2) return false;	/* (a) */
		if (nWallCount >= MAZE_WALL_MAX) return false;
		for (i = 0; i < nWallCount; i ++) {
			const MazeWall &w = aWall[i];
			if (w.nOrient == orient) {
				if (orient == MAZE_H && w.nY == y && w.nX - x >= -1 && w.nX - x <= 1) return false;	/* (b) */
				if (orient == MAZE_V && w.nX == x && w.nY - y >= -1 && w.nY - y <= 1) return false;
			} else if (w.nX == x && w.nY == y) {
				return false;											/* (c) 十字 */
			}
		}
		/* (d) 置いてみて、両者の経路が残るか */
		aWall[nWallCount].nX = x;  aWall[nWallCount].nY = y;  aWall[nWallCount].nOrient = orient;
		nWallCount ++;
		bool ok = HasPath (0) && HasPath (1);
		nWallCount --;
		return ok;
	}

	/* who が dir に 1 マス進めるか（盤の外・壁・相手のコマは不可。手番は見ない） */
	bool CanMove (int who, int dir) const
	{
		if (dir < 0 || dir > 3) return false;
		int nx = nPosX[who] + g_anMazeDx[dir], ny = nPosY[who] + g_anMazeDy[dir];
		if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) return false;
		if (EdgeBlocked (nPosX[who], nPosY[who], dir)) return false;
		if (nx == nPosX[1 - who] && ny == nPosY[1 - who]) return false;
		return true;
	}

	/* 手番のプレイヤーが動けず壁も置けない（パスするしかない）か */
	bool NeedPass ()
	{
		int d, x, y, o;
		if (nWinner != MAZE_NO_WINNER) return false;
		for (d = 0; d < 4; d ++) {
			if (CanMove (byTurn, d)) return false;
		}
		if (nWallLeft[byTurn] > 0) {
			for (y = 0; y < MAZE_SIZE - 1; y ++) {
				for (x = 0; x < MAZE_SIZE - 1; x ++) {
					for (o = 0; o < 2; o ++) {
						if (CanPlaceWall (x, y, o)) return false;
					}
				}
			}
		}
		return true;
	}

	/* 手番を相手に渡す */
	bool Pass (int who)
	{
		if (nWinner != MAZE_NO_WINNER || who != byTurn || !NeedPass ()) return false;
		byTurn = 1 - byTurn;
		return true;
	}

	/* コマを進める。成功したら手番と勝敗を更新して true */
	bool Move (int who, int dir)
	{
		if (nWinner != MAZE_NO_WINNER || who != byTurn || !CanMove (who, dir)) return false;
		nPosX[who] += g_anMazeDx[dir];
		nPosY[who] += g_anMazeDy[dir];
		if (nPosY[who] == GoalRow (who)) {
			nWinner = who;			/* 着いた時点で確定（手番はそのまま） */
		} else {
			byTurn = 1 - who;
		}
		return true;
	}

	/* 壁を置く。成功したら手番を更新して true */
	bool PlaceWall (int who, int x, int y, int orient)
	{
		if (nWinner != MAZE_NO_WINNER || who != byTurn || nWallLeft[who] <= 0) return false;
		if (!CanPlaceWall (x, y, orient)) return false;
		aWall[nWallCount].nX = x;  aWall[nWallCount].nY = y;  aWall[nWallCount].nOrient = orient;
		aWall[nWallCount].nOwner = who;
		nWallCount ++;
		nWallLeft[who] --;
		byTurn = 1 - who;
		return true;
	}
};

/* 手番のプレイヤーが動けず壁も置けないか（MazeGame::NeedPass の外向き版） */
static inline bool MazeNeedPass (MazeGame &g) { return g.NeedPass (); }

/* 手番を飛ばす（MazeNeedPass の時だけ成功） */
static inline bool MazePass (MazeGame &g, int who) { return g.Pass (who); }

/* who が dir に進めるか */
static inline bool MazeCanMove (const MazeGame &g, int who, int dir) { return g.CanMove (who, dir); }

/* who のコマ位置からゴール行までの BFS 最短手数（壁だけ考慮・コマは無視）。着けないなら -1 */
static inline int MazeShortestDist (const MazeGame &g, int who)
{
	int dist[MAZE_SIZE][MAZE_SIZE];
	int qx[MAZE_SIZE * MAZE_SIZE], qy[MAZE_SIZE * MAZE_SIZE];
	int head = 0, tail = 0, d, x, y;
	for (y = 0; y < MAZE_SIZE; y ++) for (x = 0; x < MAZE_SIZE; x ++) dist[y][x] = -1;
	dist[g.nPosY[who]][g.nPosX[who]] = 0;
	qx[tail] = g.nPosX[who];  qy[tail ++] = g.nPosY[who];
	while (head < tail) {
		x = qx[head];  y = qy[head ++];
		if (y == MazeGame::GoalRow (who)) {
			return dist[y][x];
		}
		for (d = 0; d < 4; d ++) {
			int nx = x + g_anMazeDx[d], ny = y + g_anMazeDy[d];
			if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
			if (dist[ny][nx] >= 0 || g.EdgeBlocked (x, y, d)) continue;
			dist[ny][nx] = dist[y][x] + 1;
			qx[tail] = nx;  qy[tail ++] = ny;
		}
	}
	return -1;
}

/* ========================================================================= */
/* CPU の思考（純粋ロジック。乱数は呼び出し側が持つ状態 pdwRand を更新する）	 */
/* ========================================================================= */
#define MAZE_CPU_PASS		(3)		/* MazeCpuAction::nAct：パス */
#define MAZE_CPU_MOVE		(1)		/* 進む */
#define MAZE_CPU_WALL		(2)		/* 壁を置く */
#define MAZE_CPU_WIN		(100)	/* 評価値：勝ち（負けは -100） */
#define MAZE_CPU_CAND_MAX	(MAZE_SIZE * MAZE_SIZE * 2 + 8)	/* 候補手の最大数 */

struct MazeCpuAction {
	int nAct;				/* 1=進む、2=壁を置く、3=パス */
	int nDir;				/* 進む向き（nAct==1） */
	int nX, nY, nOrient;	/* 壁（nAct==2） */
};

/* xorshift32：状態を更新して乱数を返す */
static inline unsigned int MazeCpuRand (unsigned int *pdwRand)
{
	unsigned int x = *pdwRand;
	if (x == 0) x = 2463534242u;
	x ^= x << 13;  x ^= x >> 17;  x ^= x << 5;
	*pdwRand = x;
	return x;
}

/* 0..n-1 の乱数 */
static inline int MazeCpuRandN (unsigned int *pdwRand, int n)
{
	return (int)((MazeCpuRand (pdwRand) >> 8) % (unsigned int)n);
}

/* 手を打った後の盤を作る。打てなければ false */
static inline bool MazeCpuApply (MazeGame &g, int who, const MazeCpuAction &a)
{
	if (a.nAct == MAZE_CPU_MOVE) return g.Move (who, a.nDir);
	if (a.nAct == MAZE_CPU_WALL) return g.PlaceWall (who, a.nX, a.nY, a.nOrient);
	return false;
}

/* 評価値：(相手の最短手数 − 自分の最短手数)。決着済みなら ±MAZE_CPU_WIN */
static inline int MazeCpuEval (const MazeGame &g, int who)
{
	if (g.nWinner == who) return MAZE_CPU_WIN;
	if (g.nWinner != MAZE_NO_WINNER) return -MAZE_CPU_WIN;
	int dm = MazeShortestDist (g, who), dop = MazeShortestDist (g, 1 - who);
	if (dm < 0 || dop < 0) return 0;
	return dop - dm;
}

/* who の最短路の 1 つを塞ぐ壁の候補を cand[orient][y][x] に立てる */
static inline void MazeCpuMarkPathWalls (const MazeGame &g, int who, bool cand[2][MAZE_SIZE - 1][MAZE_SIZE - 1])
{
	int dist[MAZE_SIZE][MAZE_SIZE], px[MAZE_SIZE][MAZE_SIZE], py[MAZE_SIZE][MAZE_SIZE];
	int qx[MAZE_SIZE * MAZE_SIZE], qy[MAZE_SIZE * MAZE_SIZE];
	int head = 0, tail = 0, d, x, y, ex = -1, ey = -1;
	for (y = 0; y < MAZE_SIZE; y ++) for (x = 0; x < MAZE_SIZE; x ++) dist[y][x] = -1;
	dist[g.nPosY[who]][g.nPosX[who]] = 0;
	qx[tail] = g.nPosX[who];  qy[tail ++] = g.nPosY[who];
	while (head < tail) {
		x = qx[head];  y = qy[head ++];
		if (y == MazeGame::GoalRow (who)) { ex = x;  ey = y;  break; }
		for (d = 0; d < 4; d ++) {
			int nx = x + g_anMazeDx[d], ny = y + g_anMazeDy[d];
			if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
			if (dist[ny][nx] >= 0 || g.EdgeBlocked (x, y, d)) continue;
			dist[ny][nx] = dist[y][x] + 1;
			px[ny][nx] = x;  py[ny][nx] = y;
			qx[tail] = nx;  qy[tail ++] = ny;
		}
	}
	while (ex >= 0 && (ex != g.nPosX[who] || ey != g.nPosY[who])) {
		int fx = px[ey][ex], fy = py[ey][ex], k;
		if (fx == ex) {		/* 縦に動く辺：横壁が塞ぐ */
			int row = fy < ey ? fy : ey;
			for (k = ex - 1; k <= ex; k ++) if (k >= 0 && k <= MAZE_SIZE - 2 && row <= MAZE_SIZE - 2) cand[MAZE_H][row][k] = true;
		} else {			/* 横に動く辺：縦壁が塞ぐ */
			int col = fx < ex ? fx : ex;
			for (k = ey - 1; k <= ey; k ++) if (k >= 0 && k <= MAZE_SIZE - 2 && col <= MAZE_SIZE - 2) cand[MAZE_V][k][col] = true;
		}
		ex = fx;  ey = fy;
	}
}

/* 手番 who の合法手を列挙する。nRadius>=0 なら壁は「相手の最短路を塞ぐもの＋自コマから nRadius マス以内」に絞る（-1 は全部） */
static inline int MazeCpuGenerate (const MazeGame &g, int who, int nRadius, MazeCpuAction *pOut)
{
	int n = 0, d, x, y, o;
	MazeGame w = g;		/* CanPlaceWall が非 const のため作業用コピー */
	for (d = 0; d < 4; d ++) {
		if (g.CanMove (who, d)) {
			pOut[n].nAct = MAZE_CPU_MOVE;  pOut[n].nDir = d;
			pOut[n].nX = pOut[n].nY = pOut[n].nOrient = 0;
			n ++;
		}
	}
	if (g.nWallLeft[who] > 0) {
		bool cand[2][MAZE_SIZE - 1][MAZE_SIZE - 1];
		memset (cand, 0, sizeof (cand));
		if (nRadius >= 0) MazeCpuMarkPathWalls (g, 1 - who, cand);
		for (y = 0; y < MAZE_SIZE - 1; y ++) {
			for (x = 0; x < MAZE_SIZE - 1; x ++) {
				for (o = 0; o < 2; o ++) {
					if (nRadius >= 0 && !cand[o][y][x]) {
						int dx = x - g.nPosX[who], dy = y - g.nPosY[who];
						if (dx < 0) dx = -dx;
						if (dy < 0) dy = -dy;
						if (dx > nRadius || dy > nRadius) continue;
					}
					if (!w.CanPlaceWall (x, y, o)) continue;
					pOut[n].nAct = MAZE_CPU_WALL;  pOut[n].nDir = 0;
					pOut[n].nX = x;  pOut[n].nY = y;  pOut[n].nOrient = o;
					n ++;
				}
			}
		}
	}
	return n;
}

/* 2 手先読み（自分→相手→評価）の相手の返し：相手が評価を最小にする手の値。bound を下回ったら打ち切り */
static inline int MazeCpuReply (const MazeGame &g, int who, int nBound)
{
	MazeCpuAction aA[MAZE_CPU_CAND_MAX];
	int n = MazeCpuGenerate (g, 1 - who, 1, aA), i, nMin = 1000;
	if (n == 0) return MazeCpuEval (g, who);	/* 相手はパス */
	for (i = 0; i < n; i ++) {
		MazeGame c = g;
		MazeCpuApply (c, 1 - who, aA[i]);
		int v = MazeCpuEval (c, who);
		if (v < nMin) nMin = v;
		if (nMin < nBound) break;		/* これ以上悪くなるだけ（同点は残す） */
	}
	return nMin;
}

/* CPU の 1 手を選ぶ（g の手番が who の時に呼ぶ）。nLevel：1=よわい 2=ふつう 3=つよい */
static inline MazeCpuAction MazeCpuChoose (const MazeGame &g, int who, int nLevel, unsigned int *pdwRand)
{
	MazeCpuAction act, aA[MAZE_CPU_CAND_MAX];
	int n, i, nBest, nCnt, aIdx[MAZE_CPU_CAND_MAX];
	memset (&act, 0, sizeof (act));
	act.nAct = MAZE_CPU_PASS;
	if (g.nWinner != MAZE_NO_WINNER) return act;

	if (nLevel <= 1) {
		/* 70%：最短路の 1 歩 */
		if (MazeCpuRandN (pdwRand, 100) < 70) {
			int nMinD = 1000;
			nCnt = 0;
			for (i = 0; i < 4; i ++) {
				if (!g.CanMove (who, i)) continue;
				MazeGame c = g;
				c.Move (who, i);
				int d = (c.nWinner == who) ? 0 : MazeShortestDist (c, who);
				if (d < 0) continue;
				if (d < nMinD) { nMinD = d;  nCnt = 0; }
				if (d == nMinD) aIdx[nCnt ++] = i;
			}
			if (nCnt > 0) {
				act.nAct = MAZE_CPU_MOVE;  act.nDir = aIdx[MazeCpuRandN (pdwRand, nCnt)];
				return act;
			}
		}
		/* 30%：ランダムな合法手（壁は残っていれば確率的に） */
		n = MazeCpuGenerate (g, who, -1, aA);
		if (n == 0) return act;
		int nWalls = 0, nMoves = 0;
		for (i = 0; i < n; i ++) { if (aA[i].nAct == MAZE_CPU_WALL) nWalls ++; else nMoves ++; }
		bool bWall = nWalls > 0 && (nMoves == 0 || MazeCpuRandN (pdwRand, 100) < 40);
		nCnt = 0;
		for (i = 0; i < n; i ++) if ((aA[i].nAct == MAZE_CPU_WALL) == bWall) aIdx[nCnt ++] = i;
		return aA[aIdx[MazeCpuRandN (pdwRand, nCnt)]];
	}

	if (nLevel == 2) {
		int dMe = MazeShortestDist (g, who), dOp = MazeShortestDist (g, 1 - who);
		bool bAhead = (dMe >= 0 && dMe < dOp);		/* 先に着ける：進む */
		n = MazeCpuGenerate (g, who, -1, aA);
		if (n == 0) return act;
		nBest = -100000;  nCnt = 0;
		for (i = 0; i < n; i ++) {
			MazeGame c = g;
			MazeCpuApply (c, who, aA[i]);
			int v = MazeCpuEval (c, who);
			if (aA[i].nAct == MAZE_CPU_WALL) {
				if (bAhead) continue;
				int dOp2 = MazeShortestDist (c, 1 - who);
				if (dOp2 <= dOp) continue;			/* 相手の手数が増えない壁は候補にしない */
			}
			int s = v * 2 + (aA[i].nAct == MAZE_CPU_MOVE ? 1 : 0);	/* 同評価なら進む手 */
			if (s > nBest) { nBest = s;  nCnt = 0; }
			if (s == nBest) aIdx[nCnt ++] = i;
		}
		if (nCnt == 0) return act;
		return aA[aIdx[MazeCpuRandN (pdwRand, nCnt)]];
	}

	/* つよい：2 手先読み＋アルファ・ベータ */
	n = MazeCpuGenerate (g, who, 2, aA);
	if (n == 0) {
		n = MazeCpuGenerate (g, who, -1, aA);		/* 絞り込みで尽きた時は全部から */
		if (n == 0) return act;
	}
	nBest = -100000;  nCnt = 0;
	for (i = 0; i < n; i ++) {
		MazeGame c = g;
		MazeCpuApply (c, who, aA[i]);
		int v;
		if (c.nWinner == who) {
			v = MAZE_CPU_WIN + 1;					/* 相手の番が来る前に着く */
		} else {
			v = MazeCpuReply (c, who, nBest);
		}
		if (v > nBest) { nBest = v;  nCnt = 0; }
		if (v == nBest) aIdx[nCnt ++] = i;
	}
	return aA[aIdx[MazeCpuRandN (pdwRand, nCnt)]];
}
