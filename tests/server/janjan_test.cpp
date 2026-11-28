/* SBOじゃんの盤面ロジック（common/Janjan.h）の native テスト
   g++ -std=c++11 -O1 -I. tests/server/janjan_test.cpp -o /tmp/jt && /tmp/jt */
#include <stdio.h>
#include <string.h>
#include "common/Janjan.h"

static int g_nNg = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG %d: %s\n", __LINE__, #c); g_nNg ++; } } while (0)

/* 席の隠した手牌を指定の並びにする（試験用） */
static void SetHand (JanjanGame &g, int nSeat, const int *p, int n)
{
	int i;
	g.anHand[nSeat] = 0;
	for (i = 0; i < n; i ++) JanjanHandAdd (g, nSeat, p[i]);
}

/* 席 nFrom が nTile を捨てた直後の状態にする（試験用。手牌は呼び出し側で決める） */
static void SetLast (JanjanGame &g, int nFrom, int nTile)
{
	g.nPhase = JANJAN_PHASE_DRAW;
	g.nTurn = (nFrom + 1) % JANJAN_SEATS;
	g.nLastSeat = nFrom;
	g.nLastTile = nTile;
	g.bLastLive = 1;
	g.aRiver[nFrom][g.anRiver[nFrom] ++] = nTile;
}

static bool Win (const int *p, int n) { return JanjanIsWinHand (p, n); }

/* CPU 4 人で 1 局を最後まで打つ。反則（false）があれば -1、手数を返す */
static int PlayOut (uint32_t dwSeed, int nStrength)
{
	JanjanGame g;
	int nSteps = 0;
	JanjanStart (g, dwSeed);
	while (g.nPhase != JANJAN_PHASE_END) {
		if (++ nSteps > 1000) return -1;
		if (g.nPhase == JANJAN_PHASE_DRAW) {
			int aClaim[JANJAN_SEATS] = {0}, aA[JANJAN_SEATS] = {0}, aB[JANJAN_SEATS] = {0}, s, nSeat = -1, nType;
			if (g.bLastLive) {
				for (s = 0; s < JANJAN_SEATS; s ++) {
					aClaim[s] = JanjanCpuClaim (g, s, nStrength, &aA[s], &aB[s]);
				}
				nType = JanjanResolveClaim (g, aClaim, &nSeat);
				if (nType == JANJAN_CLAIM_RON) { if (!JanjanRon (g, nSeat)) return -1; continue; }
				if (nType == JANJAN_CLAIM_SOROI) { if (!JanjanSoroi (g, nSeat)) return -1; }
				else if (nType == JANJAN_CLAIM_GUMI) { if (!JanjanGumi (g, nSeat, aA[nSeat], aB[nSeat])) return -1; }
				if (nType != JANJAN_CLAIM_NONE) continue;
			}
			if (JanjanIsExhausted (g)) { if (!JanjanFinishDraw (g)) return -1; continue; }
			if (!JanjanDraw (g, g.nTurn)) return -1;
		} else {
			if (JanjanCanTsumo (g, g.nTurn)) { if (!JanjanTsumo (g, g.nTurn)) return -1; continue; }
			int t = JanjanCpuDiscard (g, g.nTurn, nStrength);
			if (!JanjanDiscard (g, g.nTurn, t)) return -1;
		}
		/* 枚数の不変条件: 隠した牌 + 鳴き×3 は引いた直後・鳴いた直後が 9、それ以外が 8 */
		for (int s = 0; s < JANJAN_SEATS; s ++) {
			int nAll = g.anHand[s] + g.anMeld[s] * JANJAN_SET_SIZE;
			int nWant = (g.nPhase == JANJAN_PHASE_DISCARD && s == g.nTurn) ? JANJAN_HAND + 1 : JANJAN_HAND;
			if (nAll != nWant) return -1;
		}
	}
	return g.nResult == JANJAN_RESULT_WIN ? (g.anScore[g.nWinner] == 1 ? nSteps : -1) : nSteps;
}

int main()
{
	int i, s;
	JanjanGame g;

	/* 配牌: 各席 8 枚・山は 64-32=32 枚・親は席 0 で引く番 */
	JanjanStart (g, 12345);
	for (s = 0; s < JANJAN_SEATS; s ++) CHECK (g.anHand[s] == JANJAN_HAND);
	CHECK (JanjanPileLeft (g) == JANJAN_TILES - JANJAN_HAND * JANJAN_SEATS);
	CHECK (g.nTurn == 0 && g.nPhase == JANJAN_PHASE_DRAW && g.nResult == JANJAN_RESULT_NONE);
	{
		/* 山と手牌を合わせて 16 種×4 枚 */
		int cnt[JANJAN_KINDS] = {0};
		for (i = g.nPileTop; i < JANJAN_TILES; i ++) cnt[g.aPile[i]] ++;
		for (s = 0; s < JANJAN_SEATS; s ++) for (i = 0; i < g.anHand[s]; i ++) cnt[g.aHand[s][i]] ++;
		for (i = 0; i < JANJAN_KINDS; i ++) CHECK (cnt[i] == JANJAN_COPIES);
		CHECK (JanjanGroupOf (15) == 3 && JanjanGroupOf (4) == 1);
	}

	/* 同じ種なら同じ山、違う種なら違う山 */
	{
		JanjanGame a, b, c;
		JanjanStart (a, 777); JanjanStart (b, 777); JanjanStart (c, 778);
		CHECK (memcmp (a.aPile, b.aPile, sizeof (a.aPile)) == 0);
		CHECK (memcmp (a.aPile, c.aPile, sizeof (a.aPile)) != 0);
	}

	/* 上がり判定 */
	{
		int soroi3[9] = {0,0,0, 4,4,4, 8,8,8};
		int gumi3[9]  = {0,1,2, 4,5,6, 8,9,10};
		int mixed[9]  = {0,0,0, 4,5,6, 12,13,15};
		int gumiAlt[9] = {0,1,2, 0,1,3, 4,4,4};		/* 同じ組の中で 0,1,2 と 0,1,3 */
		int dupBad[9] = {0,0,1, 4,4,4, 8,8,8};		/* 0,0,1 は組そろいにならない（同じ絵を含む） */
		int diffGrp[9] = {0,1,2, 4,5,6, 8,9,14};		/* 8,9 と別の組の 14 は不可 */
		int tight[9] = {0,0,1, 1,2,2, 4,4,4};		/* 0,1,2 が 2 つ */
		int open6[6] = {0,0,0, 5,6,7};				/* 鳴き 1 つ: 隠すのは 6 枚 */
		int open3[3] = {8,9,10};					/* 鳴き 2 つ: 隠すのは 3 枚 */
		int short8[8] = {0,0,0, 4,4,4, 8,8};
		CHECK (Win (soroi3, 9));
		CHECK (Win (gumi3, 9));
		CHECK (Win (mixed, 9));
		CHECK (Win (gumiAlt, 9));
		CHECK (Win (tight, 9));
		CHECK (!Win (dupBad, 9));
		CHECK (!Win (diffGrp, 9));
		CHECK (Win (open6, 6));
		CHECK (Win (open3, 3));
		CHECK (!Win (short8, 8));					/* 1 枚足りない */
		{
			int bad[9] = {0,0,0, 4,4,4, 8,8,12};
			CHECK (!Win (bad, 9));
		}
		/* 隠した牌 + 鳴きで判定: 席に鳴きを持たせ、6 枚で上がり */
		JanjanStart (g, 1);
		g.anMeld[1] = 1;
		SetHand (g, 1, open6, 6);
		CHECK (JanjanIsWin (g, 1, -1));
		SetHand (g, 1, short8, 5);
		CHECK (!JanjanIsWin (g, 1, -1));
	}

	/* 引く・ツモ・捨てる・反則 */
	{
		int h[8] = {0,0,0, 4,5,6, 8,9};
		JanjanStart (g, 5);
		SetHand (g, 0, h, 8);
		CHECK (!JanjanDraw (g, 1));					/* 手番でない席は引けない */
		CHECK (!JanjanDiscard (g, 0, 0));			/* 引く前は捨てられない */
		CHECK (JanjanDraw (g, 0) && g.anHand[0] == 9);
		CHECK (!JanjanDraw (g, 0));					/* 2 回は引けない */
		CHECK (!JanjanDiscard (g, 1, 0));
		{
			int nHave = JanjanHandCount (g, 0, 15);
			int nPile = g.nPileTop;
			if (nHave == 0) {
				CHECK (!JanjanDiscard (g, 0, 15));	/* 持っていない牌は捨てられない */
				CHECK (g.anHand[0] == 9 && g.nPileTop == nPile && g.anRiver[0] == 0);
			}
		}
		CHECK (JanjanDiscard (g, 0, 0));
		CHECK (g.anHand[0] == 8 && g.anRiver[0] == 1 && g.aRiver[0][0] == 0);
		CHECK (g.nTurn == 1 && g.nPhase == JANJAN_PHASE_DRAW && g.bLastLive);
		/* ツモ */
		JanjanStart (g, 5);
		int w[8] = {0,0,0, 4,4,4, 8,8};
		SetHand (g, 0, w, 8);
		g.aPile[g.nPileTop] = 8;
		CHECK (!JanjanTsumo (g, 0));				/* 引く前は上がれない */
		JanjanDraw (g, 0);
		CHECK (JanjanCanTsumo (g, 0) && !JanjanCanTsumo (g, 1));
		CHECK (!JanjanTsumo (g, 1));
		CHECK (JanjanTsumo (g, 0));
		CHECK (g.nResult == JANJAN_RESULT_WIN && g.nWinner == 0 && g.nWinHow == JANJAN_HOW_TSUMO);
		CHECK (g.anScore[0] == 1 && g.anScore[1] == 0 && g.nPhase == JANJAN_PHASE_END);
		CHECK (!JanjanDraw (g, 0));
	}

	/* ロン: 自分の捨て牌では不可・頭ハネ（捨てた人の次の席から順で最初） */
	{
		int w[8] = {0,0,0, 4,4,4, 8,8};
		JanjanStart (g, 9);
		SetHand (g, 0, w, 8); SetHand (g, 2, w, 8); SetHand (g, 3, w, 8);
		int o[8] = {1,5,9,13, 2,6,10,14};
		SetHand (g, 1, o, 8);
		SetLast (g, 3, 8);							/* 席 3 が 8 を捨てた: 次は席 0 */
		CHECK (!JanjanCanRon (g, 3) && !JanjanCanRon (g, 1));
		CHECK (JanjanCanRon (g, 0) && JanjanCanRon (g, 2));
		CHECK (JanjanRonSeat (g) == 0);
		CHECK (!JanjanRon (g, 2));					/* 席 0 に頭ハネされる */
		CHECK (g.nResult == JANJAN_RESULT_NONE && g.anScore[2] == 0);
		CHECK (JanjanRon (g, 0));
		CHECK (g.nResult == JANJAN_RESULT_WIN && g.nWinner == 0 && g.nWinHow == JANJAN_HOW_RON && g.anScore[0] == 1);
		/* 捨てたのが席 0 なら、席 2 より席 1 の次... 席 2 が最初 */
		JanjanStart (g, 9);
		SetHand (g, 0, w, 8); SetHand (g, 2, w, 8); SetHand (g, 3, w, 8);
		SetLast (g, 0, 8);
		CHECK (JanjanRonSeat (g) == 2);
		/* 引いたあとはもうロンできない */
		JanjanDraw (g, g.nTurn);
		CHECK (JanjanRonSeat (g) == -1);
	}

	/* そろい（ポン相当）: 同じ絵 2 枚で誰の捨て牌でも可・川から取って手番が移る */
	{
		int h[8] = {4,4, 0,1, 8,9, 12,13};
		JanjanStart (g, 3);
		SetHand (g, 2, h, 8);
		SetLast (g, 0, 4);							/* 席 0 の捨て牌（席 2 は左隣ではない） */
		CHECK (JanjanCanSoroi (g, 2));
		CHECK (!JanjanCanSoroi (g, 0));				/* 自分の捨て牌 */
		CHECK (!JanjanSoroi (g, 3));				/* 同じ絵を持っていない */
		CHECK (JanjanSoroi (g, 2));
		CHECK (g.anMeld[2] == 1 && g.aMeld[2][0].nKind == JANJAN_MELD_SOROI && g.aMeld[2][0].nFrom == 0);
		CHECK (g.anHand[2] == 6 && g.anRiver[0] == 0);
		CHECK (g.nTurn == 2 && g.nPhase == JANJAN_PHASE_DISCARD && !g.bDrew);
		CHECK (!JanjanCanTsumo (g, 2));				/* 鳴いただけではツモにならない */
		CHECK (!JanjanDraw (g, 2));					/* 引かずに捨てる */
		CHECK (JanjanDiscard (g, 2, 13) && g.anHand[2] == 5 && g.nTurn == 3);
		/* 1 枚しか持っていなければ鳴けない */
		int h1[8] = {4, 0,1, 8,9, 12,13,14};
		JanjanStart (g, 3);
		SetHand (g, 2, h1, 8);
		SetLast (g, 0, 4);
		CHECK (!JanjanCanSoroi (g, 2) && !JanjanSoroi (g, 2) && g.anHand[2] == 8 && g.anRiver[0] == 1);
	}

	/* 組そろい（チー相当）: 左隣（席番号+1）だけ・違う絵 2 枚・選べる */
	{
		int h[8] = {5,6,7, 5, 8,9, 12,13};			/* 捨て牌 4 に対して (5,6)(5,7)(6,7) が作れる */
		int aOut[16][2];
		JanjanStart (g, 3);
		SetHand (g, 1, h, 8); SetHand (g, 2, h, 8); SetHand (g, 0, h, 8);
		SetLast (g, 0, 4);
		CHECK (JanjanCanGumi (g, 1));
		CHECK (!JanjanCanGumi (g, 2) && !JanjanCanGumi (g, 3) && !JanjanCanGumi (g, 0));
		CHECK (JanjanGumiOptions (g, 1, aOut, 16) == 3);
		CHECK (!JanjanGumi (g, 2, 5, 6));			/* 左隣ではない */
		CHECK (!JanjanGumi (g, 1, 5, 5));			/* 同じ絵は不可 */
		CHECK (!JanjanGumi (g, 1, 5, 8));			/* 別の組は不可 */
		CHECK (!JanjanGumi (g, 1, 4, 5));			/* 捨て牌と同じ絵は不可 */
		CHECK (g.anHand[1] == 8 && g.bLastLive);
		CHECK (JanjanGumi (g, 1, 7, 6));			/* 選んだ 2 枚（順不同） */
		CHECK (g.aMeld[1][0].nKind == JANJAN_MELD_GUMI);
		CHECK (g.aMeld[1][0].aTile[0] == 4 && g.aMeld[1][0].aTile[1] == 6 && g.aMeld[1][0].aTile[2] == 7);
		CHECK (g.anHand[1] == 6 && JanjanHandCount (g, 1, 5) == 2 && g.anRiver[0] == 0);
		CHECK (g.nTurn == 1 && g.nPhase == JANJAN_PHASE_DISCARD);
		/* 席 3 が捨てた場合の左隣は席 0（循環） */
		JanjanStart (g, 3);
		SetHand (g, 0, h, 8); SetHand (g, 1, h, 8);
		SetLast (g, 3, 4);
		CHECK (JanjanCanGumi (g, 0) && !JanjanCanGumi (g, 1));
		/* 鳴いたあと 1 枚捨てられる枚数が要る・鳴き上限 */
		int tiny[3] = {5, 6, 12};
		SetHand (g, 0, tiny, 3);
		CHECK (JanjanCanGumi (g, 0));
		g.anMeld[0] = JANJAN_MELD_MAX;
		CHECK (!JanjanCanGumi (g, 0));
	}

	/* 優先順位: ロン > そろい > 組そろい */
	{
		int aClaim[JANJAN_SEATS], nSeat = -1;
		int win[8] = {5,5,5, 8,8,8, 12,12};			/* 席 2 は 12 でロン */
		int pon[8] = {4,4, 0,1, 8,9, 12,13};
		int chi[8] = {5,6, 0,1, 8,9, 12,13};
		JanjanStart (g, 3);
		/* 席 0 が 12 を捨てた。席 1=チー相当は不可（別の組）、席 2=ロン、席 3=そろい */
		SetHand (g, 1, chi, 8); SetHand (g, 2, win, 8);
		int pon12[8] = {12,12, 0,1, 8,9, 4,5};
		SetHand (g, 3, pon12, 8);
		SetLast (g, 0, 12);
		aClaim[0] = JANJAN_CLAIM_NONE; aClaim[1] = JANJAN_CLAIM_GUMI; aClaim[2] = JANJAN_CLAIM_RON; aClaim[3] = JANJAN_CLAIM_SOROI;
		CHECK (JanjanResolveClaim (g, aClaim, &nSeat) == JANJAN_CLAIM_RON && nSeat == 2);
		/* ロンできる席が申し出なければ、そろいが勝つ */
		aClaim[2] = JANJAN_CLAIM_NONE;
		CHECK (JanjanResolveClaim (g, aClaim, &nSeat) == JANJAN_CLAIM_SOROI && nSeat == 3);
		/* できない申し出は無視される（席 1 のチーは 12 では作れない） */
		aClaim[3] = JANJAN_CLAIM_NONE;
		CHECK (JanjanResolveClaim (g, aClaim, &nSeat) == JANJAN_CLAIM_NONE);
		/* そろい > 組そろい: 捨て牌 4 を、席 1 がチー・席 3 がポンで取り合う */
		JanjanStart (g, 3);
		int chi4[8] = {5,6, 0,1, 8,9, 12,13};
		SetHand (g, 1, chi4, 8); SetHand (g, 3, pon, 8);
		SetLast (g, 0, 4);
		aClaim[0] = JANJAN_CLAIM_NONE; aClaim[1] = JANJAN_CLAIM_GUMI; aClaim[2] = JANJAN_CLAIM_NONE; aClaim[3] = JANJAN_CLAIM_SOROI;
		CHECK (JanjanResolveClaim (g, aClaim, &nSeat) == JANJAN_CLAIM_SOROI && nSeat == 3);
		aClaim[3] = JANJAN_CLAIM_NONE;
		CHECK (JanjanResolveClaim (g, aClaim, &nSeat) == JANJAN_CLAIM_GUMI && nSeat == 1);
	}

	/* 鳴きありで上がる: 1 鳴き + 隠し 6 枚をツモで完成 */
	{
		int h[8] = {0,0, 5,6,7, 12,13,14};
		JanjanStart (g, 3);
		SetHand (g, 1, h, 8);
		SetLast (g, 0, 4);
		CHECK (!JanjanCanSoroi (g, 1) && JanjanCanGumi (g, 1));
		JanjanGumi (g, 1, 5, 6);					/* 4,5,6 を公開。隠し 6 枚: 0,0,7,12,13,14 */
		CHECK (g.anHand[1] == 6);
		JanjanDiscard (g, 1, 7);					/* 1 枚捨てて隠し 5 枚 */
		CHECK (g.anHand[1] == 5);
		g.nTurn = 1; g.nPhase = JANJAN_PHASE_DRAW; g.bLastLive = 0;
		g.aPile[g.nPileTop] = 0;
		JanjanDraw (g, 1);							/* 0,0,0,12,13,14 */
		CHECK (JanjanTsumo (g, 1));
	}

	/* 流局: 山が尽きたら引けず、流局にできる。尽きる前は流局にできない */
	{
		JanjanStart (g, 11);
		CHECK (!JanjanIsExhausted (g) && !JanjanFinishDraw (g));
		g.nPileTop = JANJAN_TILES;
		CHECK (JanjanIsExhausted (g));
		CHECK (!JanjanDraw (g, 0));
		CHECK (JanjanFinishDraw (g));
		CHECK (g.nResult == JANJAN_RESULT_DRAW && g.nPhase == JANJAN_PHASE_END && g.nWinner == -1);
		CHECK (g.anScore[0] + g.anScore[1] + g.anScore[2] + g.anScore[3] == 0);
	}

	/* CPU: 捨てる牌は必ず手牌にある。強さ 2 は孤立牌を捨てる。強さ 3 は待ちを保つ */
	{
		for (int st = 1; st <= 3; st ++) {
			for (uint32_t sd = 1; sd <= 20; sd ++) {
				JanjanStart (g, sd);
				JanjanDraw (g, 0);
				int t = JanjanCpuDiscard (g, 0, st);
				CHECK (t >= 0 && JanjanHandCount (g, 0, t) > 0);
			}
		}
		JanjanStart (g, 1);
		int h2[9] = {0,0,0, 4,4, 8,9, 12, 13};
		SetHand (g, 0, h2, 9);
		g.nPhase = JANJAN_PHASE_DISCARD; g.bDrew = 1;
		int t = JanjanCpuDiscard (g, 0, 2);
		CHECK (t == 8 || t == 9 || t == 12 || t == 13);			/* 同じ組の相方が 1 枚だけの牌（価値が低い）から捨てる */
		CHECK (t != 0);
		/* 強さ 3: 8 枚が「あと 1 枚」の形になる捨て方を選ぶ。{0,0,0,4,5,6,8,8,12}: 12 を捨てると 8 待ち */
		int h3[9] = {0,0,0, 4,5,6, 8,8, 12};
		JanjanStart (g, 1);
		SetHand (g, 0, h3, 9);
		g.nPhase = JANJAN_PHASE_DISCARD; g.bDrew = 1;
		CHECK (JanjanWaitCount (g, 0, 12) > 0 && JanjanWaitCount (g, 0, 0) == 0);
		for (uint32_t sd = 1; sd <= 10; sd ++) {
			g.dwRng = sd;
			CHECK (JanjanCpuDiscard (g, 0, 3) == 12);
		}
		/* CPU の申し出 */
		int pon[8] = {4,4, 0,1, 8,9, 12,13};
		int chi[8] = {5,6, 0,1, 8,9, 12,13};
		int a = -1, b = -1;
		JanjanStart (g, 3);
		SetHand (g, 2, pon, 8); SetHand (g, 1, chi, 8);
		SetLast (g, 0, 4);
		CHECK (JanjanCpuClaim (g, 2, 1, &a, &b) == JANJAN_CLAIM_SOROI);
		CHECK (JanjanCpuClaim (g, 1, 1, &a, &b) == JANJAN_CLAIM_NONE);		/* 強さ 1 は組そろいを鳴かない */
		CHECK (JanjanCpuClaim (g, 1, 2, &a, &b) == JANJAN_CLAIM_GUMI && a == 5 && b == 6);
	}

	/* CPU が 1 局を最後まで打ち切れる（100 種・強さ 1〜3 で無限ループ・反則なし） */
	{
		for (int st = 1; st <= 3; st ++) {
			for (uint32_t sd = 1; sd <= 100; sd ++) {
				int n = PlayOut (sd * 7919u + st, st);
				CHECK (n > 0);
			}
		}
	}

	/* 台詞：数字だけなら番号（サブゲーム情報）・カンマがあれば旧形式・"@j" だけや数字以外は番号なし */
	{
		int an[JANJAN_PARAM_COUNT];
		CHECK (JanjanSpeechInfoNo ("@j3") == 3);
		CHECK (JanjanSpeechInfoNo ("@j0") == 0);
		CHECK (JanjanSpeechInfoNo ("@j314,1") == -1);
		CHECK (JanjanSpeechInfoNo ("@j314,1,3") == -1);
		CHECK (JanjanSpeechInfoNo ("@j") == -1);
		CHECK (JanjanSpeechInfoNo ("@jx") == -1);
		CHECK (JanjanSpeechInfoNo ("@j3x") == -1);
		CHECK (JanjanSpeechInfoNo ("@j3,") == -1);
		CHECK (JanjanSpeechInfoNo ("@j999999") == -1);
		CHECK (JanjanSpeechInfoNo ("@i3") == -1);
		CHECK (JanjanSpeechInfoNo (NULL) == -1);
		JanjanParseSpeech ("@j314,1,3", an);		/* 旧形式はこれまでどおり */
		CHECK (an[0] == 314 && an[1] == 1 && an[2] == 3);
	}
	/* サブゲーム情報の既定値と丸め（CPU 強さ 0 は 2・上限・チケット枚数の上限） */
	{
		struct { unsigned char byCpuLevel; unsigned short wTicketItemID; unsigned char byTicketNum; unsigned short wWaitSec; } Info;		/* SUBGAME_JANJANINFO と同じ欄（TypedefCommon.h は入れない） */
		memset (&Info, 0xFF, sizeof (Info));
		JanjanInfoSetDefault (&Info);
		CHECK (Info.byCpuLevel == JANJAN_STRENGTH_DEFAULT && Info.wTicketItemID == 0 && Info.byTicketNum == 0);
		Info.byCpuLevel = 0; Info.byTicketNum = 200;
		JanjanInfoClamp (&Info);
		CHECK (Info.byCpuLevel == JANJAN_STRENGTH_DEFAULT && Info.byTicketNum == JANJAN_TICKET_MAX);
		Info.byCpuLevel = 9;
		JanjanInfoClamp (&Info);
		CHECK (Info.byCpuLevel == JANJAN_CPU_LEVEL_MAX);
		/* 参加募集の待ち時間：既定 60・0 は 60・下限 5・上限 600 */
		JanjanInfoSetDefault (&Info);
		CHECK (Info.wWaitSec == JANJAN_WAIT_DEFAULT);
		Info.wWaitSec = 0; JanjanInfoClamp (&Info); CHECK (Info.wWaitSec == JANJAN_WAIT_DEFAULT);
		Info.wWaitSec = 2; JanjanInfoClamp (&Info); CHECK (Info.wWaitSec == JANJAN_WAIT_MIN);
		Info.wWaitSec = 9999; JanjanInfoClamp (&Info); CHECK (Info.wWaitSec == JANJAN_WAIT_MAX);
		Info.wWaitSec = 30; JanjanInfoClamp (&Info); CHECK (Info.wWaitSec == 30);
	}

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
