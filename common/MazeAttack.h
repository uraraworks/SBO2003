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
