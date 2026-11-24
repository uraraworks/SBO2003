/* 落ち物パズルの盤面ロジック（common/FallingPuzzle.h）の native テスト
   g++ -std=c++11 -O1 -I. tests/server/falling_puzzle_test.cpp -o /tmp/falling_puzzle_test && /tmp/falling_puzzle_test */
#include <stdio.h>
#include "common/FallingPuzzle.h"

static int g_nNg = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG %d: %s\n", __LINE__, #c); g_nNg ++; } } while (0)

int main()
{
	int k, r, i;

	/* どの形・向きでも 4 ブロックで、箱の中に収まる。同じ向きに 4 回回すと元に戻る */
	for (k = 0; k < PUZZLE_KINDS; k ++) {
		for (r = 0; r < 4; r ++) {
			int c[4][2];
			PuzzleCells (k, r, c);
			for (i = 0; i < 4; i ++) {
				CHECK (c[i][0] >= 0 && c[i][0] < g_aPuzzleShape[k].nSize);
				CHECK (c[i][1] >= 0 && c[i][1] < g_aPuzzleShape[k].nSize);
			}
		}
		int a[4][2], b[4][2];
		PuzzleCells (k, 0, a);
		PuzzleCells (k, 4, b);
		CHECK (memcmp (a, b, sizeof (a)) == 0);
	}

	/* 衝突：壁・床・積んだブロック */
	PuzzleBoard bd;
	bd.Init (10, 18);
	CHECK (!bd.Collide (2, 0, 3, 0));
	CHECK (bd.Collide (2, 0, -2, 0));			/* 左の壁 */
	CHECK (bd.Collide (2, 0, 9, 0));			/* 右の壁 */
	CHECK (!bd.Collide (1, 0, 0, 16));			/* O は最下段に置ける */
	CHECK (bd.Collide (1, 0, 0, 17));			/* 床の下 */
	bd.Set (4, 5, 3);
	CHECK (bd.Collide (1, 0, 3, 4));			/* O の右下(4,5)が積んだブロック */
	CHECK (!bd.Collide (1, 0, 5, 4));

	/* ライン消し：1〜4 列、間の行が残って詰まる */
	bd.Init (10, 18);
	for (r = 0; r < 4; r ++) {
		for (i = 0; i < 10; i ++) {
			bd.Set (i, 14 + r, 1);
		}
	}
	bd.Set (0, 13, 5);							/* 一番下の 4 列の上に 1 つ */
	CHECK (bd.ClearLines () == 4);
	CHECK (bd.Get (0, 17) == 5);				/* 上のブロックが落ちてきた */
	CHECK (bd.Get (0, 13) == 0);
	bd.Init (10, 18);
	for (i = 0; i < 10; i ++) { bd.Set (i, 17, 1); bd.Set (i, 15, 1); }
	bd.Set (3, 16, 2);							/* 穴のある行 */
	CHECK (bd.ClearLines () == 2);
	CHECK (bd.Get (3, 17) == 2);
	CHECK (bd.Get (0, 17) == 0);

	/* 得点 */
	CHECK (g_anPuzzleLineScore[1] == 100 && g_anPuzzleLineScore[2] == 300 &&
		g_anPuzzleLineScore[3] == 500 && g_anPuzzleLineScore[4] == 800);

	/* 回転：壁際で回せる（壁けり）。左右の壁にめり込まない */
	PuzzleGame g;
	g.Start (10, 18, 1, 0);
	g.nKind = 0; g.nRot = 0; g.nX = 0; g.nY = 5;			/* 横の I（箱の左端） */
	CHECK (g.Rotate () == PUZZLE_CHG_PIECE);				/* 縦になる */
	CHECK (!g.Board.Collide (g.nKind, g.nRot, g.nX, g.nY));
	g.nKind = 0; g.nRot = 1; g.nX = 7; g.nY = 5;			/* 縦の I が右端（箱の 3 列目が x=9） */
	CHECK (g.Rotate () == PUZZLE_CHG_PIECE);
	CHECK (!g.Board.Collide (g.nKind, g.nRot, g.nX, g.nY));
	g.nKind = 1;											/* O は回らない */
	CHECK (g.Rotate () == 0);

	/* 左右移動：壁で止まる */
	g.nKind = 2; g.nRot = 0; g.nX = 0; g.nY = 5;
	CHECK (g.Move (-1) == 0 || g.Board.Collide (2, 0, -1, 5) == false);
	g.nX = 0;
	CHECK (g.Move (1) == PUZZLE_CHG_PIECE && g.nX == 1);

	/* 落下の速さ：レベル 0 は 1 秒、レベルごとに短く、下限あり */
	CHECK (PuzzleGame::DropMs (0) == 1000);
	CHECK (PuzzleGame::DropMs (1) < PuzzleGame::DropMs (0));
	CHECK (PuzzleGame::DropMs (100) == PUZZLE_DROP_MS_MIN);

	/* 周期処理：経過時間で進む（呼ばれる間隔に関係なく 1 秒に 1 段） */
	g.Start (10, 18, 7, 1000);
	int nY0 = g.nY;
	g.Tick (1500);									/* まだ */
	CHECK (g.nY == nY0);
	g.Tick (2000);									/* 1 秒たった */
	CHECK (g.nY == nY0 + 1);
	g.Tick (2000);									/* 同じ時刻なら進まない */
	CHECK (g.nY == nY0 + 1);
	g.Tick (5000);									/* 3 秒ぶん遅れて呼ばれたら 3 段 */
	CHECK (g.nY == nY0 + 4);

	/* 固定→ライン消し→得点・レベル：下に落とし続けると必ず固定され、最後はゲームオーバー */
	g.Start (10, 18, 3, 0);
	unsigned int t = 0;
	int nLockCount = 0;
	for (i = 0; i < 100000 && !g.bOver; i ++) {
		t += 1000;
		int c = g.Step (true, t);
		if (c & PUZZLE_CHG_BOARD) {
			nLockCount ++;
		}
	}
	CHECK (g.bOver);								/* 真ん中に積み続ければいつか置けなくなる */
	CHECK (nLockCount > 5);
	CHECK (g.dwScore > 0);							/* ソフトドロップの得点 */

	/* 1 列そろう配置：O 5 個ではなく I で 1 列を作って得点 100 */
	g.Start (10, 18, 5, 0);
	for (i = 0; i < 10; i ++) {
		if (i < 4 || i > 5) { g.Board.Set (i, 17, 1); }
	}
	/* 4,5 列目の隙間を O で埋める */
	g.nKind = 1; g.nRot = 0; g.nX = 4; g.nY = 16;
	int nChg = g.LockAndNext ();
	CHECK (nChg & PUZZLE_CHG_SCORE);
	CHECK (g.dwScore == 100 && g.nLines == 1);

	/* 10 ライン消すとレベル 1 */
	g.Start (10, 18, 5, 0);
	g.nLines = 9;
	for (i = 0; i < 10; i ++) {
		if (i < 4 || i > 5) { g.Board.Set (i, 17, 1); }
	}
	g.nKind = 1; g.nRot = 0; g.nX = 4; g.nY = 16;
	g.LockAndNext ();
	CHECK (g.nLevel == 1);

	/* 同じ種なら同じ順番（対戦で同じ順番を出すため） */
	PuzzleGame g1, g2;
	g1.Start (10, 18, 99, 0);
	g2.Start (10, 18, 99, 0);
	for (i = 0; i < 30; i ++) {
		CHECK (g1.nKind == g2.nKind && g1.nNext == g2.nNext);
		g1.Spawn (); g2.Spawn ();
	}
	/* 7 個入りの袋：最初の 7 個は 7 種が 1 つずつ */
	PuzzleGame g3;
	g3.Start (10, 18, 12345, 0);
	int anSeen[PUZZLE_KINDS] = {0};
	anSeen[g3.nKind] ++;
	for (i = 0; i < 6; i ++) { g3.Spawn (); anSeen[g3.nKind] ++; }
	for (k = 0; k < PUZZLE_KINDS; k ++) { CHECK (anSeen[k] == 1); }

	/* フィールドの位置：席の相対位置。既定は席がフィールドの真下（下端のマス行が席の 1 マス上・横は中央） */
	int fx, fy;
	PuzzleFieldPos (20, 30, PuzzleDefaultDx (10), PuzzleDefaultDy (14), &fx, &fy);
	CHECK (PuzzleDefaultDx (10) == -2 && PuzzleDefaultDy (14) == -7);
	CHECK (fx == 18 * 32 && fy == 23 * 32);
	CHECK (fx + 80 == 20 * 32 + 16);						/* 横の中央が席のマスの中央 */
	CHECK (fy + 14 * 16 == 30 * 32);						/* 下端は席のマスの上端（最下行は席の 1 マス上） */
	CHECK (PuzzleDefaultDy (18) == -9);						/* 18 段（9 マス）は -9 */
	PuzzleFieldPos (20, 30, 2, -3, &fx, &fy);				/* 席の横に置く指定 */
	CHECK (fx == 22 * 32 && fy == 27 * 32);

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
