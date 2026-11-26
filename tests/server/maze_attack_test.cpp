/* 迷路対戦の盤面ロジック（common/MazeAttack.h）の native テスト
   g++ -std=c++17 -O1 -I. tests/server/maze_attack_test.cpp -o /tmp/maze_attack_test && /tmp/maze_attack_test */
#include <stdio.h>
#include "common/MazeAttack.h"

static int g_nNg = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG %d: %s\n", __LINE__, #c); g_nNg ++; } } while (0)

/* 盤に壁を直接並べる（ルール判定を通さない。道塞ぎの検証用） */
static void AddWall (MazeGame &g, int x, int y, int o)
{
	g.aWall[g.nWallCount].nX = x;  g.aWall[g.nWallCount].nY = y;  g.aWall[g.nWallCount].nOrient = o;
	g.nWallCount ++;
}

int main()
{
	MazeGame g;
	int i;

	/* 初期配置 */
	g.Init ();
	CHECK (g.nPosX[0] == 3 && g.nPosY[0] == 6);
	CHECK (g.nPosX[1] == 3 && g.nPosY[1] == 0);
	CHECK (g.byTurn == 0);
	CHECK (g.GetWinner () == MAZE_NO_WINNER);
	CHECK (g.nWallLeft[0] == 6 && g.nWallLeft[1] == 6 && g.nWallCount == 0);
	CHECK (MazeShortestDist (g, 0) == 6 && MazeShortestDist (g, 1) == 6);
	CHECK (g.HasPath (0) && g.HasPath (1));
	CHECK (!MazeNeedPass (g));

	/* 手番：相手の番には動けない。動くと交代 */
	CHECK (!g.Move (1, MAZE_DOWN));
	CHECK (g.Move (0, MAZE_UP) && g.nPosY[0] == 5 && g.byTurn == 1);
	CHECK (!g.Move (0, MAZE_UP));
	CHECK (g.Move (1, MAZE_LEFT) && g.nPosX[1] == 2 && g.byTurn == 0);
	CHECK (MazeShortestDist (g, 0) == 5);

	/* 盤の端で止まる */
	g.Init ();
	g.nPosX[0] = 0;  g.nPosY[0] = 3;
	CHECK (!MazeCanMove (g, 0, MAZE_LEFT));
	g.nPosX[0] = 6;
	CHECK (!MazeCanMove (g, 0, MAZE_RIGHT));
	CHECK (MazeCanMove (g, 0, MAZE_LEFT));
	CHECK (!g.Move (0, 9) && !g.Move (0, -1));

	/* 壁で止まる：横壁 (3,2) は (3,2)/(4,2) と (3,3)/(4,3) の間 */
	g.Init ();
	g.nPosX[0] = 3;  g.nPosY[0] = 3;
	CHECK (g.PlaceWall (0, 3, 2, MAZE_H));
	CHECK (g.byTurn == 1 && g.nWallLeft[0] == 5);
	CHECK (!g.CanMove (0, MAZE_UP));
	g.nPosX[0] = 4;
	CHECK (!g.CanMove (0, MAZE_UP));
	g.nPosX[0] = 5;
	CHECK (g.CanMove (0, MAZE_UP));
	g.nPosX[0] = 2;
	CHECK (g.CanMove (0, MAZE_UP));
	g.nPosX[0] = 3;  g.nPosY[0] = 2;
	CHECK (!g.CanMove (0, MAZE_DOWN));		/* 反対側からも通れない */
	/* 縦壁 (1,4) は x=1 と x=2 の境目を y=4,5 にわたって塞ぐ */
	CHECK (g.PlaceWall (1, 1, 4, MAZE_V));
	g.nPosX[0] = 1;  g.nPosY[0] = 5;
	CHECK (!g.CanMove (0, MAZE_RIGHT));
	g.nPosX[0] = 2;
	CHECK (!g.CanMove (0, MAZE_LEFT));
	g.nPosY[0] = 4;
	CHECK (!g.CanMove (0, MAZE_LEFT));
	g.nPosY[0] = 6;
	CHECK (g.CanMove (0, MAZE_LEFT));
	g.nPosY[0] = 3;
	CHECK (g.CanMove (0, MAZE_LEFT));

	/* 相手のコマのマスには進めない（飛び越えもしない） */
	g.Init ();
	g.nPosX[0] = 3;  g.nPosY[0] = 4;
	g.nPosX[1] = 3;  g.nPosY[1] = 3;
	CHECK (!g.CanMove (0, MAZE_UP));
	CHECK (!g.Move (0, MAZE_UP));
	CHECK (g.byTurn == 0 && g.nPosY[0] == 4);
	CHECK (!g.CanMove (1, MAZE_DOWN));
	CHECK (g.CanMove (0, MAZE_LEFT) && g.CanMove (0, MAZE_RIGHT));
	CHECK (MazeShortestDist (g, 0) == 4);	/* 最短手数：壁で回り道になる */
	g.Init ();
	AddWall (g, 2, 2, MAZE_H);
	AddWall (g, 4, 2, MAZE_H);			/* x=2..5 が y=2|3 で塞がる。P0 は (3,6) から x=1 まで 2 歩寄る */
	CHECK (MazeShortestDist (g, 0) == 8);
	CHECK (MazeShortestDist (g, 1) == 6 + 0 + 2);	/* P1 は (3,0) から x=1 へ 2 歩寄って下る */
	g.nPosX[0] = 1;  g.nPosY[0] = 3;
	CHECK (MazeShortestDist (g, 0) == 3);
	CHECK (g.nPosX[1] == 3 && MazeShortestDist (g, 1) == 8);

	/* 全体で道が絶対に残る：ランダムに置き続けても両者の経路は残る */
	{
		unsigned s = 12345;
		int n;
		for (n = 0; n < 200; n ++) {
			g.Init ();
			int turns;
			for (turns = 0; turns < 40 && g.GetWinner () == MAZE_NO_WINNER; turns ++) {
				s = s * 1103515245u + 12345u;
				int r = (s >> 16) % 3, who = g.byTurn;
				bool ok = false;
				if (r == 0) {
					ok = g.Move (who, (s >> 8) & 3);
				} else {
					ok = g.PlaceWall (who, (s >> 8) % 6, (s >> 12) % 6, (s >> 5) & 1);
				}
				if (!ok && MazeNeedPass (g)) MazePass (g, who);
				CHECK (g.HasPath (0) && g.HasPath (1));
				CHECK (g.nWallCount <= MAZE_WALL_MAX);
				if (g_nNg > 20) return 1;
			}
		}
	}

	if (g_nNg == 0) {
		printf ("OK maze_attack\n");
		return 0;
	}
	printf ("%d NG\n", g_nNg);
	return 1;
}
