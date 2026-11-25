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
	CHECK (PuzzleDefaultDx (10) == -2 && PuzzleDefaultDy (14) == -6);
	CHECK (fx == 18 * 32 && fy == 24 * 32);
	CHECK (fx + 80 == 20 * 32 + 16);						/* 横の中央が席のマスの中央 */
	CHECK (fy + 14 * 16 == 30 * 32 + 32);					/* 下端は台のマスの下端（最下行が台のマスと重なる） */
	CHECK (PuzzleDefaultDy (18) == -8);						/* 18 段（9 マス）は -8（マス数−1） */
	PuzzleFieldPos (20, 30, 2, -3, &fx, &fy);				/* 席の横に置く指定 */
	CHECK (fx == 22 * 32 && fy == 27 * 32);

	/* ---- 対戦：お邪魔ブロック ---- */

	/* 消した列数に応じた段数：1 列は送らない・2→1・3→2・4→4 */
	CHECK (PuzzleGarbageLines (0) == 0 && PuzzleGarbageLines (1) == 0 && PuzzleGarbageLines (2) == 1 &&
		PuzzleGarbageLines (3) == 2 && PuzzleGarbageLines (4) == 4);

	/* 盤面にお邪魔を積む：下から nLines 段・全体が上へずれる・穴は 1 段に 1 か所 */
	PuzzleRand rnd;
	rnd.Seed (77);
	bd.Init (10, 18);
	bd.Set (3, 17, 2);							/* 一番下に普通のブロック */
	CHECK (bd.AddGarbage (3, rnd));
	CHECK (bd.Get (3, 14) == 2);				/* 3 段上へずれた */
	for (r = 15; r < 18; r ++) {
		int nHole = 0, nGarb = 0;
		for (i = 0; i < 10; i ++) {
			if (bd.Get (i, r) == 0) nHole ++;
			if (bd.Get (i, r) == PUZZLE_CELL_GARBAGE) nGarb ++;
		}
		CHECK (nHole == 1 && nGarb == 9);
	}
	CHECK (bd.ClearLines () == 0);				/* 穴があるので消えない */
	/* 穴の位置は段ごとにばらつく（20 段積んで全部同じ列にはならない） */
	bd.Init (10, 24);
	bd.AddGarbage (20, rnd);
	int anHoleSeen[10] = {0}, nKinds = 0;
	for (r = 4; r < 24; r ++) {
		for (i = 0; i < 10; i ++) { if (bd.Get (i, r) == 0) anHoleSeen[i] ++; }
	}
	for (i = 0; i < 10; i ++) { if (anHoleSeen[i]) nKinds ++; }
	CHECK (nKinds >= 4);
	/* 上にはみ出す時は false（負け）。空の盤面なら true */
	bd.Init (10, 18);
	bd.Set (0, 0, 1);							/* 一番上の行にブロック */
	CHECK (!bd.AddGarbage (1, rnd));
	bd.Init (10, 18);
	bd.Set (0, 1, 1);
	CHECK (bd.AddGarbage (1, rnd));				/* 上から 2 行目は 1 段ずれても残る */
	CHECK (bd.AddGarbage (0, rnd));

	/* 待ちお邪魔：足す（上限あり）・自分が消したラインで相殺・残りが相手へ・相殺しきれない分は固定で積まれる */
	PuzzleGame a;
	a.Start (10, 18, 5, 0);
	CHECK (a.nPending == 0 && a.nAttack == 0);
	CHECK (a.AddPending (3) == PUZZLE_CHG_GARBAGE && a.nPending == 3);
	CHECK (a.AddPending (1000) == PUZZLE_CHG_GARBAGE && a.nPending == PUZZLE_PENDING_MAX);
	CHECK (a.AddPending (1) == 0);				/* 上限なので変わらない */

	/* 4 列消し（攻撃 4）：待ち 3 を相殺して残り 1 を相手へ。待ちは 0 になり積まれない */
	a.Start (10, 18, 5, 0);
	a.AddPending (3);
	for (r = 0; r < 4; r ++) for (i = 0; i < 10; i ++) { if (!(i == 4 && r < 4)) a.Board.Set (i, 14 + r, 1); }
	a.nKind = 0; a.nRot = 1; a.nX = 2; a.nY = 14;			/* 縦の I を 4 列目の穴（x=4）に入れる */
	CHECK (a.LockAndNext () & PUZZLE_CHG_BOARD);
	CHECK (a.nLines == 4);
	CHECK (a.nPending == 0);
	CHECK (a.nAttack == 1);
	CHECK (a.Board.Get (0, 17) == 0);			/* お邪魔は積まれていない（空の盤面に戻った） */

	/* 4 列消し・待ち 0：4 段をそのまま相手へ */
	a.Start (10, 18, 5, 0);
	for (r = 0; r < 4; r ++) for (i = 0; i < 10; i ++) { if (i != 4) a.Board.Set (i, 14 + r, 1); }
	a.nKind = 0; a.nRot = 1; a.nX = 2; a.nY = 14;
	a.LockAndNext ();
	CHECK (a.nAttack == 4);

	/* 1 列消しは送らない */
	a.Start (10, 18, 5, 0);
	for (i = 0; i < 10; i ++) { if (i < 4 || i > 5) a.Board.Set (i, 17, 1); }
	a.nKind = 1; a.nRot = 0; a.nX = 4; a.nY = 16;
	a.LockAndNext ();
	CHECK (a.nLines == 1 && a.nAttack == 0);

	/* ラインを消さずに固定すると待ちお邪魔が下から積まれる（1 回に PUZZLE_GARBAGE_APPLY_MAX 段まで。残りは持ち越し） */
	a.Start (10, 18, 5, 0);
	a.AddPending (PUZZLE_GARBAGE_APPLY_MAX + 2);
	a.nKind = 1; a.nRot = 0; a.nX = 0; a.nY = 16;			/* 左下に O を置く */
	int nChgG = a.LockAndNext ();
	CHECK (nChgG & PUZZLE_CHG_GARBAGE);
	CHECK (a.nPending == 2);
	CHECK (!a.bOver);
	for (r = 18 - PUZZLE_GARBAGE_APPLY_MAX; r < 18; r ++) {
		int nGarb = 0;
		for (i = 0; i < 10; i ++) { if (a.Board.Get (i, r) == PUZZLE_CELL_GARBAGE) nGarb ++; }
		CHECK (nGarb >= 8);						/* 穴 1 か所（O の上に重なった行は 8 のこともある） */
	}
	CHECK (a.Board.Get (0, 18 - PUZZLE_GARBAGE_APPLY_MAX - 1) == 2);	/* O（値 2）は上へずれた */

	/* 勝敗判定：お邪魔で積み上がって上にはみ出す・次の落ち物が置けない → 置けなくなった側が負け（OVER） */
	a.Start (10, 18, 5, 0);
	for (r = 0; r < 18; r ++) { a.Board.Set (9, r, 1); }	/* 右端を全段埋める（消えない：他の列が空） */
	for (i = 0; i < 9; i ++) { a.Board.Set (i, 0, 1); }		/* 一番上の行に 9 個（左右の壁際は空でも、お邪魔でずれて出る） */
	a.Board.Set (9, 0, 0);
	a.AddPending (1);
	a.nKind = 1; a.nRot = 0; a.nX = 0; a.nY = 16;
	int nChgO = a.LockAndNext ();
	CHECK ((nChgO & PUZZLE_CHG_OVER) && a.bOver);
	PuzzleGame b;											/* お邪魔なしで、置けなくなった時も OVER */
	b.Start (10, 18, 5, 0);
	for (r = 0; r < 18; r ++) for (i = 0; i < 10; i ++) { if (i != 9) b.Board.Set (i, r, 1); }
	b.nKind = 1; b.nRot = 0; b.nX = 4; b.nY = 16;
	CHECK ((b.LockAndNext () & PUZZLE_CHG_OVER) && b.bOver);
	/* 逆に、お邪魔が積まれても余裕があれば続く */
	b.Start (10, 18, 5, 0);
	b.AddPending (2);
	b.nKind = 1; b.nRot = 0; b.nX = 0; b.nY = 16;
	CHECK (!(b.LockAndNext () & PUZZLE_CHG_OVER) && !b.bOver);

	/* お邪魔の穴の乱数は落ち物の順番に影響しない（同じ種の 2 人は、お邪魔の有無にかかわらず同じ順番） */
	PuzzleGame c1, c2;
	c1.Start (10, 18, 4242, 0);
	c2.Start (10, 18, 4242, 0);
	c1.AddPending (2);
	c1.Board.AddGarbage (2, c1.GRand);
	for (i = 0; i < 20; i ++) {
		CHECK (c1.nKind == c2.nKind && c1.nNext == c2.nNext);
		c1.Spawn (); c2.Spawn ();
	}

	/* 台詞の座る場所（9・10 番目）。省略は台NPCの 1 つ下、負の値・空欄・範囲外の丸め。既存の 8 項目の台詞はそのまま読める */
	{
		int nSx, nSy, anVal[PUZZLE_PARAM_COUNT];

		PuzzleSeatOffset ("@p314,1,,,,,1,1", &nSx, &nSy);
		CHECK (nSx == 0 && nSy == 1);
		PuzzleSeatOffset ("@p", &nSx, &nSy);
		CHECK (nSx == 0 && nSy == 1);
		PuzzleSeatOffset ("@p1,1,,,,,,,3,-2", &nSx, &nSy);
		CHECK (nSx == 3 && nSy == -2);
		PuzzleSeatOffset ("@p1,1,,,,,,,-4", &nSx, &nSy);
		CHECK (nSx == -4 && nSy == 1);
		PuzzleSeatOffset ("@p1,1,,,,,,,,5", &nSx, &nSy);
		CHECK (nSx == 0 && nSy == 5);
		PuzzleSeatOffset ("@p1,1,,,,,,,999,-999", &nSx, &nSy);
		CHECK (nSx == PUZZLE_SEAT_LIMIT && nSy == -PUZZLE_SEAT_LIMIT);
		PuzzleParseSpeech ("@p314,1,,,,,1,1", anVal);
		CHECK (anVal[0] == 314 && anVal[1] == 1 && anVal[2] == PUZZLE_PARAM_NONE && anVal[6] == 1 && anVal[7] == 1 && anVal[8] == PUZZLE_PARAM_NONE);
	}

	/* 先読み（クライアントが同じ判定で先に動かす）：PuzzleTry* はサーバーの PuzzleGame と同じ結果になる */
	{
		PuzzleBoard pb;
		int nX, nRot, nY;

		/* 動けない時は動かさない（左の壁・積んだブロック・床） */
		pb.Init (10, 14);
		nX = -0; nRot = 0; nY = 3;
		CHECK (!PuzzleTryMove (pb, 1, 0, &nX, nY, -1) && nX == 0);		/* O が左の壁 */
		nX = 4;
		pb.Set (6, 4, 3);
		CHECK (!PuzzleTryMove (pb, 1, 0, &nX, nY, 1) && nX == 4);		/* O の右が積んだブロック */
		CHECK (PuzzleTryMove (pb, 1, 0, &nX, nY, -1) && nX == 3);
		nY = 12;
		CHECK (!PuzzleTryDown (pb, 1, 0, 0, &nY) && nY == 12);			/* O が床 */
		nY = 3;
		CHECK (PuzzleTryDown (pb, 1, 0, 0, &nY) && nY == 4);
		/* O は回らない */
		nRot = 0; nX = 4;
		CHECK (!PuzzleTryRotate (pb, 1, &nRot, &nX, 3) && nRot == 0 && nX == 4);
		/* 壁けり：I を縦（回転 1）にして右の壁際で回すと、左にずらして入る（サーバーの Rotate と同じ） */
		pb.Init (10, 14);
		PuzzleGame g1;
		g1.Start (10, 14, 1, 0);
		g1.nKind = 0; g1.nRot = 1; g1.nX = 8; g1.nY = 2;
		nRot = g1.nRot; nX = g1.nX; nY = g1.nY;
		bool bClient = PuzzleTryRotate (g1.Board, 0, &nRot, &nX, nY);
		int nSrv = g1.Rotate ();
		CHECK (bClient == (nSrv != 0));
		CHECK (nRot == g1.nRot && nX == g1.nX);
		CHECK (g1.nX != 8);												/* 壁けりでずれた */
	}
	/* ランダムなキー列を、サーバー（PuzzleGame）とクライアントの先読み（PuzzleTry*）で同じに動かすと、毎回同じ位置・向きになる */
	{
		PuzzleGame sv;
		PuzzleRand rk;
		int nSeed, nStep;
		int nSame = 0, nTotal = 0;

		for (nSeed = 1; nSeed <= 20; nSeed ++) {
			sv.Start (10, 14, (unsigned int)nSeed, 0);
			rk.Seed ((unsigned int)nSeed * 7919u);
			/* 盤面にでこぼこを作る */
			for (i = 0; i < 25; i ++) {
				sv.Board.Set ((int)(rk.Next () % 10), 8 + (int)(rk.Next () % 6), 4);
			}
			for (nStep = 0; nStep < 200 && !sv.bOver; nStep ++) {
				int nKey = (int)(rk.Next () % 4);
				int nKind = sv.nKind, nRot = sv.nRot, nX = sv.nX, nY = sv.nY;
				bool bPred = false;
				int nChg = 0;

				switch (nKey) {
				case 0:	bPred = PuzzleTryMove (sv.Board, nKind, nRot, &nX, nY, -1);	nChg = sv.Move (-1);	break;
				case 1:	bPred = PuzzleTryMove (sv.Board, nKind, nRot, &nX, nY, 1);	nChg = sv.Move (1);		break;
				case 2:	bPred = PuzzleTryRotate (sv.Board, nKind, &nRot, &nX, nY);	nChg = sv.Rotate ();	break;
				default:
					bPred = PuzzleTryDown (sv.Board, nKind, nRot, nX, &nY);
					nChg = sv.Step (true, 0);
					if (!bPred) {		/* 詰まれば固定（サーバー任せ）。次の落ち物から */
						continue;
					}
					break;
				}
				nTotal ++;
				CHECK (bPred == (nChg != 0));
				if (bPred && nChg && sv.nKind == nKind && sv.nRot == nRot && sv.nX == nX && sv.nY == nY) {
					nSame ++;
				}
			}
		}
		CHECK (nSame > 100 && nSame <= nTotal);
	}
	/* GAME OVER を出す時間（定数）：本人の帯の表示時間は 0 でも 3 秒 */
	CHECK (PUZZLE_OVER_SHOW_MS >= 0 && PUZZLE_OVER_SELF_MS > 0);

	/* ハイスコア・連勝記録の並び：上位 10 件・1 キャラ 1 件（最高だけ）・同点は先に達成した方が上 */
	{
		PuzzleRecordList L;
		char szName[PUZZLE_RECORD_NAMEBUF];

		L.Clear ();
		CHECK (L.nCount == 0);
		CHECK (L.Add ("", 100, 1) == 0 && L.Add (NULL, 100, 1) == 0 && L.Add ("A", 0, 1) == 0);		/* 名前なし・0 点は載せない */
		CHECK (L.Add ("A", 500, 20261101) == 1);
		CHECK (L.Add ("B", 800, 20261102) == 1);					/* 上に入る */
		CHECK (L.Add ("C", 500, 20261103) == 3);					/* 同点は先の A の後ろ */
		CHECK (L.nCount == 3);
		CHECK (strcmp (L.aRec[0].szName, "B") == 0 && strcmp (L.aRec[1].szName, "A") == 0 && strcmp (L.aRec[2].szName, "C") == 0);
		CHECK (L.aRec[1].dwDate == 20261101 && L.aRec[2].dwDate == 20261103);
		/* 1 キャラ 1 件：同じか低い得点は足さない（先着の日付も変わらない）・高ければ前の行を消して入れ直す */
		CHECK (L.Add ("A", 500, 20261110) == 0 && L.Add ("A", 100, 20261110) == 0);
		CHECK (L.nCount == 3 && L.aRec[1].dwDate == 20261101);
		CHECK (L.Add ("A", 900, 20261111) == 1);
		CHECK (L.nCount == 3);
		CHECK (strcmp (L.aRec[0].szName, "A") == 0 && L.aRec[0].dwValue == 900 && L.aRec[0].dwDate == 20261111);
		CHECK (strcmp (L.aRec[1].szName, "B") == 0 && strcmp (L.aRec[2].szName, "C") == 0);
		/* 10 件で打ち止め：満杯の時、一番下より高ければ押し出す・同点や低いのは入らない */
		L.Clear ();
		for (i = 1; i <= 12; i ++) {
			sprintf (szName, "P%d", i);
			L.Add (szName, (unsigned int)(i * 100), 20261120);
		}
		CHECK (L.nCount == PUZZLE_RECORD_MAX);
		CHECK (L.aRec[0].dwValue == 1200 && L.aRec[9].dwValue == 300);		/* 100・200 は押し出された */
		CHECK (L.Add ("Q", 300, 20261121) == 0);						/* 最下位と同点は入らない（先着が上） */
		CHECK (L.Add ("Q", 250, 20261121) == 0);
		CHECK (L.Add ("Q", 301, 20261121) == 10);
		CHECK (L.nCount == PUZZLE_RECORD_MAX && L.aRec[9].dwValue == 301 && strcmp (L.aRec[9].szName, "Q") == 0);
		CHECK (strcmp (L.aRec[8].szName, "P4") == 0);					/* 300 の P3 は押し出された */
		/* 満杯でも、載っているキャラが上がる時は件数が増えない */
		CHECK (L.Add ("P4", 5000, 20261122) == 1);
		CHECK (L.nCount == PUZZLE_RECORD_MAX && strcmp (L.aRec[0].szName, "P4") == 0);
		{
			int n = 0;
			for (i = 0; i < L.nCount; i ++) {
				if (strcmp (L.aRec[i].szName, "P4") == 0) n ++;
			}
			CHECK (n == 1);
		}
		/* 並びは常に降順 */
		for (i = 1; i < L.nCount; i ++) {
			CHECK (L.aRec[i - 1].dwValue >= L.aRec[i].dwValue);
		}
		/* 欄より長い名前は欄に収まるところまで（終端つき） */
		{
			char szLong[200];
			memset (szLong, 'x', sizeof (szLong));
			szLong[199] = 0;
			L.Clear ();
			CHECK (L.Add (szLong, 10, 1) == 1);
			CHECK (strlen (L.aRec[0].szName) == PUZZLE_RECORD_NAMEBUF - 1);
			CHECK (L.Add (szLong, 20, 1) == 1 && L.nCount == 1);		/* 同じ名前とみなす */
		}
	}

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
