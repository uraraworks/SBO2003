/* 土地取りすごろくのルール（common/LandSugoroku.h）の native テスト
   g++ -std=c++11 -O1 -I. tests/server/land_sugoroku_test.cpp -o /tmp/land_sugoroku_test && /tmp/land_sugoroku_test */
#include <stdio.h>
#include "common/LandSugoroku.h"

static int g_nNg = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG %d: %s\n", __LINE__, #c); g_nNg ++; } } while (0)

/* 起きたことの中から種類 nType を数える */
static int CountEv(const LandGame &G, int nType)
{
	int i, n = 0;
	for (i = 0; i < (int)G.aEvent.size (); i ++) {
		if (G.aEvent[i].nType == nType) n ++;
	}
	return n;
}

/* 手番の人 p が合計 nSum（2〜12）だけ進む。最後のマスの処理まで済ませる。時刻は dwNow */
static void Move(LandGame &G, int p, int nSum, unsigned int dwNow)
{
	int d1 = (nSum > 6) ? 6 : nSum - 1, d2 = nSum - d1;
	int i;

	CHECK (G.Roll (p, d1, d2));
	for (i = 0; i < nSum; i ++) {
		CHECK (G.Step (dwNow));
	}
}

/* 手番の人を p のまま、いるマスだけ nPos にそろえる（テスト用） */
static void Put(LandGame &G, int p, int nPos)
{
	G.aPlayer[p].nPos = nPos;
}

/* 土地のマス番号を n 番目（0〜23）から引く */
static int TownCell(int n)
{
	int i, k = 0;
	for (i = 0; i < LAND_CELLS; i ++) {
		if (LandCornerKind (i) == LANDCELL_TOWN) {
			if (k == n) return i;
			k ++;
		}
	}
	return -1;
}

int main()
{
	int i, r;
	unsigned int t = 1000;

	/* ---- 盤：標準の土地表 ---- */
	{
		LandBoard B;
		std::string strWhy;
		B.SetDefault ();
		CHECK (B.Validate (&strWhy));
		CHECK (B.aCell[0].nKind == LANDCELL_START);
		CHECK (B.aCell[7].nKind == LANDCELL_FERRY);
		CHECK (B.aCell[14].nKind == LANDCELL_TAVERN);
		CHECK (B.aCell[21].nKind == LANDCELL_FOREST);
		int nTown = 0, nPrev = 0;
		for (i = 0; i < LAND_CELLS; i ++) {
			if (B.aCell[i].nKind == LANDCELL_TOWN) {
				nTown ++;
				CHECK (B.aCell[i].nPrice >= nPrev);			/* 地方が進むほど安くならない */
				nPrev = B.aCell[i].nPrice;
				CHECK (B.aCell[i].nToll > 0 && B.aCell[i].nToll < B.aCell[i].nPrice);
				CHECK (strlen (B.aCell[i].szShort) > 0 && strlen (B.aCell[i].szShort) < LAND_SHORT_BUF);
				CHECK (strlen (B.aCell[i].szName) > 0 && strlen (B.aCell[i].szName) < LAND_NAME_BUF);
			}
		}
		CHECK (nTown == 24);
		/* 壊れた表は弾く */
		B.aCell[5].nPrice = 0;
		CHECK (!B.Validate (&strWhy));
		B.SetDefault ();
		B.aCell[3].nKind = LANDCELL_FOREST;
		CHECK (!B.Validate (&strWhy));
	}

	/* ---- マス → 盤の中の位置：外周を 1 周つながる ---- */
	{
		int x0, y0, x1, y1;
		bool bSeen[LAND_SIDE][LAND_SIDE];
		memset (bSeen, 0, sizeof (bSeen));
		LandCellOffset (0, &x0, &y0);
		CHECK (x0 == 0 && y0 == 0);
		LandCellOffset (7, &x0, &y0);
		CHECK (x0 == 7 && y0 == 0);
		LandCellOffset (14, &x0, &y0);
		CHECK (x0 == 7 && y0 == 7);
		LandCellOffset (21, &x0, &y0);
		CHECK (x0 == 0 && y0 == 7);
		for (i = 0; i < LAND_CELLS; i ++) {
			LandCellOffset (i, &x0, &y0);
			LandCellOffset (i + 1, &x1, &y1);
			CHECK (abs (x1 - x0) + abs (y1 - y0) == 1);			/* となりのマスとは 1 マスだけ離れる（27→0 も） */
			CHECK (x0 == 0 || x0 == LAND_SIDE - 1 || y0 == 0 || y0 == LAND_SIDE - 1);	/* 外周の上 */
			CHECK (!bSeen[y0][x0]);								/* 重ならない */
			bSeen[y0][x0] = true;
		}
		LandCellOffset (-1, &x0, &y0);							/* 負の番号も輪の中 */
		CHECK (x0 == 0 && y0 == 1);
	}

	/* ---- 1 コマ＝3×3 マス：コマ番号 → マップのマス位置（盤は 24×24 マス） ---- */
	{
		int x0, y0, x1, y1, gx, gy, nSlot;
		bool bSeen[LAND_SIDE * LAND_CELL_TILES][LAND_SIDE * LAND_CELL_TILES];
		memset (bSeen, 0, sizeof (bSeen));
		CHECK (LAND_CELL_TILES == 3);
		LandCellTile (0, -1, &x0, &y0);							/* スタートの中央は (1, 1) */
		CHECK (x0 == 1 && y0 == 1);
		LandCellTile (7, -1, &x0, &y0);							/* 右上の中央は (22, 1) */
		CHECK (x0 == 22 && y0 == 1);
		LandCellTile (14, -1, &x0, &y0);
		CHECK (x0 == 22 && y0 == 22);
		LandCellTile (21, -1, &x0, &y0);
		CHECK (x0 == 1 && y0 == 22);
		for (i = 0; i < LAND_CELLS; i ++) {
			LandCellOffset (i, &gx, &gy);
			LandCellTile (i, -1, &x0, &y0);
			CHECK (x0 == gx * 3 + 1 && y0 == gy * 3 + 1);		/* 代表位置は 3×3 の中央 */
			CHECK (!bSeen[y0][x0]);
			bSeen[y0][x0] = true;
			LandCellTile (i + 1, -1, &x1, &y1);
			CHECK (abs (x1 - x0) + abs (y1 - y0) == 3);			/* となりのコマの中央とは 3 歩（曲がり角も 27→0 も） */
			for (nSlot = 0; nSlot < 4; nSlot ++) {				/* 四隅は同じコマの中で、中央と重ならず、互いにも重ならない */
				int k, xs, ys;
				LandCellTile (i, nSlot, &xs, &ys);
				CHECK (xs / 3 == gx && ys / 3 == gy);
				CHECK (!(xs == x0 && ys == y0));
				CHECK ((xs % 3) != 1 && (ys % 3) != 1);
				for (k = 0; k < nSlot; k ++) {
					int xk, yk;
					LandCellTile (i, k, &xk, &yk);
					CHECK (!(xk == xs && yk == ys));
				}
			}
		}
	}

	/* ---- TSV の読み込み ---- */
	{
		LandBoard B;
		std::string strWhy;
		B.SetDefault ();
		CHECK (B.LoadTsv ("# コメント\n\n1\ttown\t0\tテスト町\tテス\t77\t11\r\n21\tforest\t0\t森\t森\t0\t0\n", &strWhy));
		CHECK (B.aCell[1].nPrice == 77 && B.aCell[1].nToll == 11 && strcmp (B.aCell[1].szShort, "テス") == 0);
		CHECK (B.Validate (&strWhy));
		CHECK (!B.LoadTsv ("1\ttown\t0\tだけ\n", &strWhy));			/* 列が足りない */
		CHECK (!B.LoadTsv ("99\ttown\t0\tN\tN\t10\t2\n", &strWhy));		/* マス番号の範囲外 */
		CHECK (!B.LoadTsv ("2\tcastle\t0\tN\tN\t10\t2\n", &strWhy));	/* 知らない種類 */
		B.SetDefault ();
		/* 長すぎる名前は欄に収まるところまで（文字の途中で切らない） */
		B.LoadTsv ("1\ttown\t0\tあいうえおかきくけこさしすせそ\tあいうえお\t10\t2\n", NULL);
		CHECK (strlen (B.aCell[1].szName) < LAND_NAME_BUF && strlen (B.aCell[1].szName) % 3 == 0);
		CHECK (strlen (B.aCell[1].szShort) < LAND_SHORT_BUF && strlen (B.aCell[1].szShort) % 3 == 0);
	}

	/* ---- 開始：初期状態 ---- */
	LandGame G;
	G.Board.SetDefault ();
	G.Start (2, t);
	CHECK (G.nPlayers == 2 && G.nTurn == 0 && G.nPhase == LANDPHASE_ROLL);
	CHECK (G.aPlayer[0].nGold == LAND_START_GOLD && G.aPlayer[1].nGold == LAND_START_GOLD);
	CHECK (G.aPlayer[0].nPos == 0 && G.aPlayer[0].nLaps == 0 && G.aPlayer[0].bAlive);
	CHECK (CountEv (G, LANDEV_TURN) == 1);
	for (i = 0; i < LAND_CELLS; i ++) CHECK (G.anOwner[i] == -1);
	CHECK (G.Assets (0) == LAND_START_GOLD);

	/* 手番でない人・段階違い・不正な出目は受け付けない */
	CHECK (!G.Roll (1, 3, 3));
	CHECK (!G.Roll (0, 0, 3));
	CHECK (!G.Roll (0, 3, 7));
	CHECK (!G.Step (t));								/* サイコロ前は進まない */
	CHECK (!G.Buy (0, true, t));						/* 購入待ちでもない */

	/* ---- 制限時間：サイコロ待ちは 30 秒で自動 ---- */
	CHECK (G.TimeoutAction (t) == LANDTIMEOUT_NONE);
	CHECK (G.TimeoutAction (t + LAND_TURN_SEC * 1000 - 1) == LANDTIMEOUT_NONE);
	CHECK (G.TimeoutAction (t + LAND_TURN_SEC * 1000) == LANDTIMEOUT_ROLL);
	CHECK (G.SecondsLeft (t) == LAND_TURN_SEC);
	CHECK (G.SecondsLeft (t + 1500) == LAND_TURN_SEC - 1);
	CHECK (G.SecondsLeft (t + 99999) == 0);
	/* 時計が一周しても正しく比べる */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, 0xFFFFFFFFU - 5000);
		CHECK (H.TimeoutAction (0xFFFFFFFFU - 4000) == LANDTIMEOUT_NONE);
		CHECK (H.TimeoutAction (0xFFFFFFFFU - 5000 + LAND_TURN_SEC * 1000) == LANDTIMEOUT_ROLL);
	}

	/* ---- 移動・購入 ---- */
	G.ClearEvents ();
	CHECK (G.Roll (0, 1, 2, true));
	CHECK (G.nPhase == LANDPHASE_MOVE && G.nStepsLeft == 3);
	CHECK (G.aEvent.back ().nType == LANDEV_ROLL && G.aEvent.back ().nValue == 3 && G.aEvent.back ().nCell == 1 && G.aEvent.back ().nOther == 12);
	CHECK (!G.Roll (0, 1, 2));							/* 動いている間はもう振れない */
	CHECK (G.Step (t) && G.aPlayer[0].nPos == 1 && G.nPhase == LANDPHASE_MOVE);
	CHECK (G.Step (t) && G.aPlayer[0].nPos == 2);
	CHECK (G.Step (t) && G.aPlayer[0].nPos == 3);
	/* 3 マス目 = 土地(3 番目の町)：空き地なので買うか聞く */
	CHECK (G.nPhase == LANDPHASE_BUY && G.nAskCell == 3 && G.nTurn == 0);
	CHECK (G.aEvent.back ().nType == LANDEV_ASK && G.aEvent.back ().nValue == G.Board.aCell[3].nPrice);
	CHECK (G.TimeoutAction (t + LAND_TURN_SEC * 1000) == LANDTIMEOUT_DECLINE);
	CHECK (!G.Buy (1, true, t));						/* 手番でない人は答えられない */
	CHECK (!G.Roll (0, 1, 1));
	{
		int nPrice = G.Board.aCell[3].nPrice;
		CHECK (G.Buy (0, true, t + 5000));
		CHECK (G.anOwner[3] == 0 && G.aPlayer[0].nGold == LAND_START_GOLD - nPrice);
		CHECK (G.Assets (0) == LAND_START_GOLD);		/* 買っても総資産は変わらない（現金→土地） */
		CHECK (CountEv (G, LANDEV_BOUGHT) == 1);
		/* 手番は 1 に移り、制限時間は新しく数え直す */
		CHECK (G.nTurn == 1 && G.nPhase == LANDPHASE_ROLL);
		CHECK (G.SecondsLeft (t + 5000) == LAND_TURN_SEC);
	}

	/* ---- 通行料：他人の土地に止まる ---- */
	G.ClearEvents ();
	{
		int nToll = G.Board.aCell[3].nToll;
		int g0 = G.aPlayer[0].nGold, g1 = G.aPlayer[1].nGold;
		Move (G, 1, 3, t + 6000);						/* 0 → 3 */
		CHECK (G.aPlayer[1].nPos == 3);
		CHECK (G.aPlayer[1].nGold == g1 - nToll && G.aPlayer[0].nGold == g0 + nToll);
		CHECK (CountEv (G, LANDEV_TOLL) == 1);
		CHECK (G.nTurn == 0 && G.nPhase == LANDPHASE_ROLL);	/* 払ったら手番交代（買えない） */
	}

	/* ---- 自分の土地に止まっても何もない ---- */
	G.ClearEvents ();
	{
		int g0 = G.aPlayer[0].nGold;
		Put (G, 0, 0);
		Move (G, 0, 3, t + 7000);
		CHECK (G.aPlayer[0].nGold == g0);
		CHECK (CountEv (G, LANDEV_OWN) == 1);
		CHECK (G.nTurn == 1);
	}

	/* ---- 買わない：「やめる」と時間切れ ---- */
	{
		int c = TownCell (1);
		G.ClearEvents ();
		Put (G, 1, c - 2);
		Move (G, 1, 2, t + 8000);						/* 1 の手番のはず */
		CHECK (G.nPhase == LANDPHASE_BUY && G.nAskCell == c);
		int g1 = G.aPlayer[1].nGold;
		CHECK (G.Buy (1, false, t + 9000));
		CHECK (G.anOwner[c] == -1 && G.aPlayer[1].nGold == g1);
		CHECK (CountEv (G, LANDEV_DECLINE) == 1 && G.aEvent.back ().nType != LANDEV_BOUGHT);
		CHECK (G.nTurn == 0);

		/* 時間切れの「やめる」は印がつく */
		G.ClearEvents ();
		Put (G, 0, c - 2);
		Move (G, 0, 2, t + 10000);
		CHECK (G.nPhase == LANDPHASE_BUY);
		CHECK (G.TimeoutAction (t + 10000 + LAND_TURN_SEC * 1000) == LANDTIMEOUT_DECLINE);
		CHECK (G.Buy (0, false, t + 10000 + LAND_TURN_SEC * 1000, true));
		{
			int k, nFlag = 0;
			for (k = 0; k < (int)G.aEvent.size (); k ++) {
				if (G.aEvent[k].nType == LANDEV_DECLINE && G.aEvent[k].nOther == 1) nFlag ++;
			}
			CHECK (nFlag == 1);
		}
		CHECK (G.anOwner[c] == -1);
	}

	/* ---- お金が足りない空き地：聞かずに次へ ---- */
	{
		LandGame H;
		int c = TownCell (23);							/* 一番高い */
		H.Board.SetDefault ();
		H.Start (2, t);
		H.aPlayer[0].nGold = H.Board.aCell[c].nPrice - 1;
		Put (H, 0, c - 2);
		Move (H, 0, 2, t);
		CHECK (CountEv (H, LANDEV_CANTBUY) == 1 && H.nPhase == LANDPHASE_ROLL && H.nTurn == 1);
		CHECK (H.anOwner[c] == -1);
		/* ちょうどの額なら買える */
		H.aPlayer[1].nGold = H.Board.aCell[c].nPrice;
		Put (H, 1, c - 2);
		Move (H, 1, 2, t);
		CHECK (H.nPhase == LANDPHASE_BUY);
		CHECK (H.Buy (1, true, t));
		CHECK (H.aPlayer[1].nGold == 0 && H.anOwner[c] == 1);
	}

	/* ---- 周回ボーナス：スタートを通る・止まる ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, t);
		Put (H, 0, LAND_CELLS - 2);
		H.Roll (0, 2, 2);								/* 26 → 27 → 0(+200) → 1 → 2 */
		H.Step (t); H.Step (t);
		CHECK (H.aPlayer[0].nLaps == 1 && H.aPlayer[0].nGold == LAND_START_GOLD + LAND_LAP_BONUS);
		H.Step (t);
		CHECK (H.aPlayer[0].nLaps == 1);				/* 通り過ぎたあとは増えない */
		H.Step (t);
		CHECK (H.aPlayer[0].nLaps == 1);
		CHECK (CountEv (H, LANDEV_BONUS) == 1);
		/* 止まっても同じボーナス（1 回だけ） */
		LandGame J;
		J.Board.SetDefault ();
		J.Start (2, t);
		Put (J, 0, LAND_CELLS - 2);
		Move (J, 0, 2, t);
		CHECK (J.aPlayer[0].nPos == 0 && J.aPlayer[0].nLaps == 1 && J.aPlayer[0].nGold == LAND_START_GOLD + LAND_LAP_BONUS);
		CHECK (J.nTurn == 1);
		/* 迷いの森で戻る時はスタートを通らない（ボーナスなし） */
		LandGame K;
		K.Board.SetDefault ();
		K.Start (2, t);
		Put (K, 0, 19);
		Move (K, 0, 2, t);								/* 19 → 21(迷いの森) → 18 */
		CHECK (K.aPlayer[0].nPos == 21 - LAND_BACK_STEPS && K.aPlayer[0].nLaps == 0 && K.aPlayer[0].nGold == LAND_START_GOLD);
		CHECK (CountEv (K, LANDEV_FOREST) == 1 && K.nTurn == 1);
		/* スタートの近くから戻っても、輪の中で負にならない */
		LandGame L;
		L.Board.SetDefault ();
		L.Start (2, t);
		L.Board.aCell[2].nKind = LANDCELL_FOREST;		/* テスト用に森を置く（Validate はしない） */
		Put (L, 0, 0);
		Move (L, 0, 2, t);
		CHECK (L.aPlayer[0].nPos == LAND_CELLS - 1 && L.aPlayer[0].nLaps == 0);
	}

	/* ---- 渡し舟小屋：1 回休み ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, t);
		Put (H, 0, 5);
		Move (H, 0, 2, t);								/* 7 = 渡し舟小屋 */
		CHECK (CountEv (H, LANDEV_FERRY) == 1 && H.aPlayer[0].nSkip == 1 && H.nTurn == 1);
		H.ClearEvents ();
		Move (H, 1, 2, t);								/* 1 の手番（2 マス先は土地：買うか聞かれる） */
		CHECK (H.nPhase == LANDPHASE_BUY);
		CHECK (H.Buy (1, false, t));
		/* 0 の番は飛ばされて 1 にもどる */
		CHECK (CountEv (H, LANDEV_SKIP) == 1);
		CHECK (H.nTurn == 1 && H.aPlayer[0].nSkip == 0 && H.nPhase == LANDPHASE_ROLL);
		/* 次の周の 0 の番は普通に来る */
		H.ClearEvents ();
		Move (H, 1, 2, t);
		if (H.nPhase == LANDPHASE_BUY) H.Buy (1, false, t);
		CHECK (H.nTurn == 0 && H.nPhase == LANDPHASE_ROLL);
	}

	/* ---- ギルド酒場：何も起きない ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, t);
		Put (H, 0, 12);
		Move (H, 0, 2, t);								/* 14 */
		CHECK (CountEv (H, LANDEV_TAVERN) == 1 && H.aPlayer[0].nPos == 14 && H.aPlayer[0].nGold == LAND_START_GOLD && H.nTurn == 1);
	}

	/* ---- 払えなければ脱落（土地は銀行へ）・1 人になれば勝ち ---- */
	{
		LandGame H;
		int c = TownCell (4), c2 = TownCell (5);
		H.Board.SetDefault ();
		H.Start (2, t);
		H.anOwner[c] = 0;								/* 0 が 2 つの土地を持ち、1 は 1 つ持つ */
		H.anOwner[c2] = 1;
		H.aPlayer[1].nGold = H.Board.aCell[c].nToll - 1;
		H.nTurn = 1;
		Put (H, 1, c - 3);
		int g0 = H.aPlayer[0].nGold;
		Move (H, 1, 3, t);
		CHECK (!H.aPlayer[1].bAlive);
		CHECK (H.aPlayer[0].nGold == g0);				/* 払えない時は受け取れない */
		CHECK (H.anOwner[c2] == -1);					/* 脱落者の土地は銀行へ */
		CHECK (H.anOwner[c] == 0);
		CHECK (CountEv (H, LANDEV_OUT) == 1);
		CHECK (H.IsOver () && H.nOverReason == LANDOVER_LAST && H.nWinMask == 1);
		CHECK (H.aEvent.back ().nType == LANDEV_OVER);
		CHECK (H.Assets (1) == 0);
		/* 終わったら何も受け付けない */
		CHECK (!H.Roll (0, 1, 1) && !H.Step (t) && !H.Buy (0, true, t));
		H.Eliminate (0, LANDOUT_LEFT, t);				/* 終わったあとの脱落は無視 */
		CHECK (H.aPlayer[0].bAlive);
	}

	/* ちょうど払える額なら払って 0G で続く */
	{
		LandGame H;
		int c = TownCell (4);
		H.Board.SetDefault ();
		H.Start (2, t);
		H.anOwner[c] = 0;
		H.aPlayer[1].nGold = H.Board.aCell[c].nToll;
		H.nTurn = 1;
		Put (H, 1, c - 3);
		Move (H, 1, 3, t);
		CHECK (H.aPlayer[1].bAlive && H.aPlayer[1].nGold == 0 && !H.IsOver ());
	}

	/* ---- 切断・退出・マップ移動：その場で脱落 ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, t);
		H.anOwner[3] = 1;
		H.anOwner[5] = 1;
		/* 手番でない人が抜けた：残りの人の勝ち */
		H.Eliminate (1, LANDOUT_LEFT, t);
		CHECK (H.IsOver () && H.nWinMask == 1 && H.anOwner[3] == -1 && H.anOwner[5] == -1);
		CHECK (H.aEvent[H.aEvent.size () - 2].nOther == LANDOUT_LEFT && H.aEvent[H.aEvent.size () - 2].nValue == 2);

		/* 手番の人が動いている途中で抜けた：残りの人の勝ち（進行は止まる） */
		LandGame J;
		J.Board.SetDefault ();
		J.Start (2, t);
		J.Roll (0, 3, 3);
		J.Step (t);
		J.Eliminate (0, LANDOUT_LEFT, t);
		CHECK (J.IsOver () && J.nWinMask == 2 && !J.Step (t));

		/* 3 人のうち手番の人が抜けた：次の人の手番になり、ゲームは続く */
		LandGame K;
		K.Board.SetDefault ();
		K.Start (3, t);
		K.Roll (0, 3, 3);
		K.Eliminate (0, LANDOUT_LEFT, t + 100);
		CHECK (!K.IsOver () && K.nTurn == 1 && K.nPhase == LANDPHASE_ROLL && K.AliveCount () == 2);
		CHECK (K.SecondsLeft (t + 100) == LAND_TURN_SEC);
		/* 購入の返事待ちの人が抜けた */
		LandGame M;
		M.Board.SetDefault ();
		M.Start (3, t);
		Put (M, 0, 1);
		Move (M, 0, 2, t);
		CHECK (M.nPhase == LANDPHASE_BUY);
		M.Eliminate (0, LANDOUT_LEFT, t);
		CHECK (M.nPhase == LANDPHASE_ROLL && M.nTurn == 1 && M.nAskCell == -1);
		CHECK (!M.Buy (0, true, t) && !M.Buy (1, true, t));
		/* 抜けた人は手番が回ってこない */
		M.Roll (1, 1, 1);
		M.Step (t); M.Step (t);
		if (M.nPhase == LANDPHASE_BUY) M.Buy (1, false, t);
		CHECK (M.nTurn == 2);
		CHECK (M.Roll (2, 1, 1));
		M.Step (t); M.Step (t);
		if (M.nPhase == LANDPHASE_BUY) M.Buy (2, false, t);
		CHECK (M.nTurn == 1);
		/* 全員いなくなった */
		M.Eliminate (1, LANDOUT_LEFT, t);
		M.Eliminate (2, LANDOUT_LEFT, t);
		CHECK (M.IsOver ());
	}

	/* ---- 12 周で終了：総資産（現金＋土地の値段）最大が勝ち ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (2, t);
		H.aPlayer[0].nLaps = LAND_LAPS - 1;
		H.aPlayer[1].nLaps = 0;
		Put (H, 0, LAND_CELLS - 2);
		Move (H, 0, 2, t);								/* 0 が 12 周に届く */
		CHECK (H.aPlayer[0].nLaps == LAND_LAPS && !H.IsOver () && H.nTurn == 1);	/* 1 の番がまだ残っている */
		Put (H, 1, 5);
		Move (H, 1, 2, t);								/* 7 渡し舟 */
		CHECK (H.IsOver () && H.nOverReason == LANDOVER_LAPS);
		/* 0 は 1700G・1 は 1500G → 0 の勝ち */
		CHECK (H.Assets (0) == LAND_START_GOLD + LAND_LAP_BONUS && H.Assets (1) == LAND_START_GOLD);
		CHECK (H.nWinMask == 1);
	}
	/* 土地の値段も総資産に入る・同点は両方の勝ち */
	{
		LandGame H;
		int c = TownCell (22);
		H.Board.SetDefault ();
		H.Start (2, t);
		H.nMaxLaps = 1;
		H.anOwner[c] = 1;
		H.aPlayer[1].nGold = 100;						/* 1 は現金少なめでも土地がある */
		H.aPlayer[0].nGold = 100 + H.Board.aCell[c].nPrice;
		CHECK (H.Assets (0) == H.Assets (1));
		H.aPlayer[0].nLaps = 1;
		/* 0 の番を回して 1 の番を終わらせると終了 */
		Put (H, 0, 14);
		Move (H, 0, 2, t);								/* 16 の土地（現金は足りるので聞かれる） */
		if (H.nPhase == LANDPHASE_BUY) H.Buy (0, false, t);
		Put (H, 1, 14);
		Move (H, 1, 2, t);
		if (H.nPhase == LANDPHASE_BUY) H.Buy (1, false, t);
		CHECK (H.IsOver () && H.nOverReason == LANDOVER_LAPS && H.nWinMask == 3);
		CHECK (H.Assets (0) == H.Assets (1));
	}
	/* 脱落者は総資産の比べに入らない */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (3, t);
		H.nMaxLaps = 1;
		H.aPlayer[2].nGold = 99999;
		H.Eliminate (2, LANDOUT_LEFT, t);
		CHECK (H.Assets (2) == 0 && !H.IsOver ());
		H.aPlayer[0].nLaps = 1;
		H.aPlayer[0].nGold = 10;
		H.aPlayer[1].nGold = 5;
		Put (H, 0, 14);
		Move (H, 0, 2, t);
		if (H.nPhase == LANDPHASE_BUY) H.Buy (0, false, t);
		Put (H, 1, 14);
		Move (H, 1, 2, t);
		if (H.nPhase == LANDPHASE_BUY) H.Buy (1, false, t);
		CHECK (H.IsOver () && H.nWinMask == 1);
	}

	/* ---- 一人だけで始めた・誰もいない ---- */
	{
		LandGame H;
		H.Board.SetDefault ();
		H.Start (1, t);
		CHECK (H.IsOver () && H.nWinMask == 1);
		H.Start (0, t);
		CHECK (H.IsOver () && H.nOverReason == LANDOVER_EMPTY);
		H.Start (9, t);
		CHECK (H.nPlayers == LAND_MAX_PLAYERS);
	}

	/* ---- 乱数でたくさん遊んでも、お金と土地の整合が崩れない（総額は周回ボーナスぶんだけ増える） ---- */
	for (r = 0; r < 200; r ++) {
		LandGame H;
		unsigned int s = 12345 + r * 7919;
		int nSteps = 0, p;
		H.Board.SetDefault ();
		H.Start (2 + r % 3, t);
		while (!H.IsOver () && nSteps < 20000) {
			s = s * 1103515245U + 12345U;
			if (H.nPhase == LANDPHASE_ROLL) {
				CHECK (H.Roll (H.nTurn, 1 + (s >> 16) % 6, 1 + (s >> 20) % 6));
			} else if (H.nPhase == LANDPHASE_MOVE) {
				H.Step (t);
			} else if (H.nPhase == LANDPHASE_BUY) {
				CHECK (H.Buy (H.nTurn, ((s >> 24) & 3) != 0, t));
			}
			/* 持ち主のいる土地は、生きている人のものだけ */
			for (i = 0; i < LAND_CELLS; i ++) {
				if (H.anOwner[i] >= 0) CHECK (H.aPlayer[(int)H.anOwner[i]].bAlive);
			}
			for (p = 0; p < H.nPlayers; p ++) {
				CHECK (H.aPlayer[p].nGold >= 0);
			}
			nSteps ++;
		}
		CHECK (H.IsOver ());
		CHECK (H.nOverReason == LANDOVER_LAPS || H.nOverReason == LANDOVER_LAST);
		CHECK (H.nWinMask != 0);
		/* 終了した時、生きている人のうち誰かは規定の周回に届いている（周回終了の時） */
		if (H.nOverReason == LANDOVER_LAPS) {
			bool bReach = false;
			for (p = 0; p < H.nPlayers; p ++) {
				if (H.aPlayer[p].bAlive && H.aPlayer[p].nLaps >= LAND_LAPS) bReach = true;
			}
			CHECK (bReach);
		}
	}

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
