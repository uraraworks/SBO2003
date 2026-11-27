/* ピンボールの物理・進行ロジック（common/Pinball.h）の native テスト
   g++ -std=c++11 -O1 -I. tests/server/pinball_test.cpp -o /tmp/pinball_test && /tmp/pinball_test */
#include <stdio.h>
#include "common/Pinball.h"
/* サブゲーム情報の既定値・丸めは、SUBGAME_PINBALLINFO と同じ項目名の構造体なら使える（TypedefCommon.h は要らない） */
#define PINBALLINFO_VERSION		(2)
#define PINBALLINFO_RESERVED	(13)
#include "common/PinballInfo.h"
struct TestInfo {
	unsigned short wCoinItem; unsigned char byCoinCount, byBalls, bySaveSec, byBandSide;
	unsigned short wScoreBumper, wScoreTarget, wScoreBonus, wComboMs;
	signed char cDx, cDy, cSeatDx, cSeatDy;
	unsigned short wVsSec; unsigned char byPair;
	unsigned char byVersion, abyReserved[PINBALLINFO_RESERVED];
};

static int g_nNg = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG %d: %s\n", __LINE__, #c); g_nNg ++; } } while (0)

/* 決まった乱数（テストを毎回同じにする） */
static unsigned int s_dwRnd = 12345;
static unsigned int Rnd(void) { s_dwRnd = s_dwRnd * 1664525u + 1013904223u; return s_dwRnd >> 8; }

/* ボールと線分の距離の 2 乗が (半径-許容)^2 以上か（壁にめり込んでいないか） */
static int FarFromSeg(const PBSEG &S, int x, int y, int nR)
{
	int qx, qy, t;
	long long dx, dy;
	PbClosest (S.x1, S.y1, S.x2, S.y2, x, y, &qx, &qy, &t);
	dx = x - qx; dy = y - qy;
	return dx * dx + dy * dy >= (long long)nR * nR;
}

static unsigned int g_dwFlashAll = 0, g_dwEvAll = 0;	/* ランダム遊びで当たったバンパー・ターゲット（nFlash の OR）と、起きた出来事（Step の戻り値の OR） */
static int g_nUpperSteps = 0;		/* ランダム遊びで、ボールが盤の上側（アーチの下の上のバンパー域）にいたステップ数 */

/* 打ち出しから、ランダムなフリッパー操作で長く遊ばせる。毎ステップ、不変条件を調べる */
static void RandomPlay(unsigned int dwSeed, int nSteps, int *pnDrains)
{
	const PinballTable &T = PinballGetTable ();
	PinballGame g;
	int i, j, nHold = 0;

	s_dwRnd = dwSeed;
	g.Start ();
	for (i = 0; i < nSteps; i ++) {
		int nEv, nPrevX = g.nX;
		if ((Rnd () % 90) == 0) g.SetFlipper (0, Rnd () & 1);
		if ((Rnd () % 90) == 0) g.SetFlipper (1, Rnd () & 1);
		if (g.nPhase == PB_PHASE_READY) {
			nHold ++;
			if (nHold > (int)(Rnd () % 150)) {
				g.Launch ((int)(Rnd () % 1100));
				nHold = 0;
			}
		}
		nEv = g.Step ();
		g_dwEvAll |= nEv; g_dwFlashAll |= g.nFlash; g.nFlash = 0;
		if (nEv & PB_EV_DRAIN) {
			(*pnDrains) ++;
			/* 穴に落ちるのは、フリッパーの軸の間だけ（壁・斜め壁の外へ抜けて落ちていない） */
			CHECK (nPrevX >= T.nPivotLx && nPrevX <= T.nPivotRx);
		}
		if (g.nPhase == PB_PHASE_OVER) {
			g.Start ();
			continue;
		}
		if (g.nPhase != PB_PHASE_PLAY) {
			continue;
		}
		if (g.nY < PbY (PB_H / 4)) g_nUpperSteps ++;
		/* 壁の外へ出ていない */
		CHECK (g.nX >= PbX (PB_WALL_L) && g.nX <= PbX (PB_WALL_R));
		CHECK (g.nY >= PbY (0) && g.nY <= T.nDrainY + PB_BALL_R);
		/* 速さの上限 */
		{
			long long nCap = (g.nX > T.nLaneL) ? PB_VMAX_LANE : PB_VMAX;
			CHECK ((long long)g.nVx * g.nVx + (long long)g.nVy * g.nVy <= (nCap + 2) * (nCap + 2));
		}
		/* 壁にめり込んでいない（反発の後は半径の 3/4 まで許す） */
		for (j = 0; j < T.nSeg; j ++) {
			if (T.aSeg[j].nTarget >= 0) continue;
			CHECK (FarFromSeg (T.aSeg[j], g.nX, g.nY, PB_BALL_R * 3 / 4));
		}
		/* 斜め壁の下（壁の外側）へ回り込んでいない */
		if (g.nX >= PbX (PB_WALL_L) && g.nX <= T.nPivotLx) {
			int nLineY = PbY (PB_DIAG_TOP) + (int)((long long)(g.nX - PbX (PB_WALL_L)) * (T.nPivotY - PbY (PB_DIAG_TOP)) / (T.nPivotLx - PbX (PB_WALL_L)));
			CHECK (g.nY <= nLineY + PB_FP);
		}
		if (g.nX >= T.nPivotRx && g.nX <= T.nLaneL) {
			int nLineY = T.nPivotY + (int)((long long)(g.nX - T.nPivotRx) * (PbY (PB_DIAG_TOP) - T.nPivotY) / (T.nLaneL - T.nPivotRx));
			CHECK (g.nY <= nLineY + PB_FP);
		}
		/* 仕切り・斜め壁の反対側へ回り込んでいない：レーンの外（x > レーン左端）にいる間は、仕切りの高さの間は右側だけ */
		if (g.nY > PbY (PB_LANE_SEPA_TOP) && g.nY < PbY (PB_LANE_FLOOR)) {
			/* 球は仕切りの左（遊ぶ場所）か右（レーン）のどちらかにいて、仕切りの上を跨ぐのは仕切りの上端より上を通った時だけ */
			CHECK (g.nX < T.nLaneL - PB_BALL_R / 2 || g.nX > T.nLaneL + PB_BALL_R / 2);
		}
	}
}

int main()
{
	int i;

	/* 整数の平方根・三角関数の表 */
	CHECK (PbIsqrt (0) == 0 && PbIsqrt (1) == 1 && PbIsqrt (15) == 3 && PbIsqrt (16) == 4 && PbIsqrt (1000000) == 1000);
	CHECK (PbIsqrt (4000000000000ULL) == 2000000);
	CHECK (PbSin (0) == 0 && PbSin (90) == PB_TRIG && PbSin (180) == 0 && PbSin (270) == -PB_TRIG && PbSin (-30) == -PbSin (30));
	CHECK (PbCos (0) == PB_TRIG && PbCos (90) == 0 && PbCos (60) == PbSin (30));
	CHECK (PbFlipDeg (0) == 30 && PbFlipDeg (PB_FLIP_STEPS) == -30);
	/* 先端：休みは下がっていて、上がると上がる。左右は鏡 */
	{
		const PinballTable &T = PinballGetTable ();
		int lx0, ly0, lx1, ly1, rx0, ry0;
		PinballFlipperTip (T, 0, 0, &lx0, &ly0);
		PinballFlipperTip (T, 0, PB_FLIP_STEPS, &lx1, &ly1);
		PinballFlipperTip (T, 1, 0, &rx0, &ry0);
		CHECK (ly0 > T.nPivotY && ly1 < T.nPivotY);
		CHECK (lx0 - T.nPivotLx == T.nPivotRx - rx0 && ly0 == ry0);
		/* 休み位置の先端の間（穴）は、球が通れる広さ */
		CHECK (rx0 - lx0 > 2 * (PB_BALL_R + PB_FLIP_R));
		/* 上がり切っても、斜め壁と軸でつながって隙間がない（球が通れる隙間がない） */
		CHECK (T.nPivotLx == PbX (PB_PIVOT_LX));
		/* 打ち出しレーンの幅は、ボールの直径より広い（詰まらない） */
		CHECK (PbX (PB_WALL_R) - T.nLaneL > 2 * PB_BALL_R);
		/* バンパーどうし・バンパーと壁の隙間は、ボールの直径より広いか、完全に閉じている（半端に狭くて詰まらない） */
		for (int a = 0; a < PB_BUMPERS; a ++) {
			for (int b = a + 1; b < PB_BUMPERS; b ++) {
				long long dx = T.aBumper[a].x - T.aBumper[b].x, dy = T.aBumper[a].y - T.aBumper[b].y;
				long long nGap = (long long)PbIsqrt ((unsigned long long)(dx * dx + dy * dy)) - 2 * PB_BUMPER_R;
				CHECK (nGap > 2 * PB_BALL_R + PB_FP);
			}
		}
	}

	/* 経過時間のステップ：1 秒で 120 ステップ。細かく分けても同じ。遅れすぎは捨てる */
	{
		PinballGame a, b;
		a.Start (); b.Start ();
		a.Update (500); a.Update (500);
		for (i = 0; i < 1000; i ++) b.Update (1);
		CHECK (a.dwStep == PB_STEP_HZ && b.dwStep == PB_STEP_HZ);
		a.Start ();
		a.Update (5000);
		CHECK (a.dwStep == PB_MAX_CATCHUP);
		a.Start ();
		a.Update (25); a.Update (25); a.Update (25); a.Update (25);		/* 100ms＝12.0 ステップ（端数は持ち越す） */
		CHECK (a.dwStep == 12);
	}

	/* 打ち出しの強さ：押した時間で強く・上限あり。READY 以外では打てない */
	{
		PinballGame a, b, c;
		a.Start (); b.Start (); c.Start ();
		CHECK (a.Launch (0) && a.nVy == -PB_LAUNCH_MIN && a.nPhase == PB_PHASE_PLAY);
		CHECK (!a.Launch (500));
		CHECK (b.Launch (PB_CHARGE_MS) && b.nVy == -PinballGetTable ().nLaunchMax);
		CHECK (c.Launch (PB_CHARGE_MS * 5) && c.nVy == -PinballGetTable ().nLaunchMax);
		CHECK (PinballGame::ChargePercent (PB_CHARGE_MS / 2) == 50 && PinballGame::ChargePercent (99999) == 100);
	}

	/* 決定的：同じ入力列からは、ステップごとに全く同じ状態になる */
	{
		PinballGame a, b;
		int nSame = 1;
		a.Start (); b.Start ();
		s_dwRnd = 777;
		for (i = 0; i < 120 * 60; i ++) {
			int nL = (int)(Rnd () % 60) == 0, nR = (int)(Rnd () % 60) == 1, nLaunch = (int)(Rnd () % 100) == 0;
			if (nL) { a.SetFlipper (0, (i / 60) & 1); b.SetFlipper (0, (i / 60) & 1); }
			if (nR) { a.SetFlipper (1, (i / 90) & 1); b.SetFlipper (1, (i / 90) & 1); }
			if (nLaunch) { a.Launch (400 + (i % 600)); b.Launch (400 + (i % 600)); }
			int ea = a.Step (), eb = b.Step ();
			if ((ea != eb) || memcmp (&a, &b, sizeof (a)) != 0) { nSame = 0; break; }
		}
		CHECK (nSame);
		CHECK (a.dwStep == 120 * 60);
	}

	/* ボールのコピーから続けても同じ（クライアントの巻き直しの前提） */
	{
		PinballGame a, b;
		a.Start ();
		a.Launch (800);
		for (i = 0; i < 400; i ++) a.Step ();
		memcpy (&b, &a, sizeof (a));
		a.SetFlipper (0, 1); b.SetFlipper (0, 1);
		for (i = 0; i < 400; i ++) { a.Step (); b.Step (); }
		CHECK (memcmp (&a, &b, sizeof (a)) == 0);
	}

	/* すり抜けない・壁の外へ出ない：ランダムな遊びを何局も */
	{
		int nDrains = 0;
		for (i = 0; i < 40; i ++) {
			RandomPlay (1000 + i * 7919, 120 * 90, &nDrains);
		}
		CHECK (nDrains > 20);
		printf ("ランダム遊び: 穴に落ちた回数 %d、上側にいたステップ %d\n", nDrains, g_nUpperSteps);
		/* 半径 5 のボールでも、打ち出し→レーンを出る→フリッパーで打ち返す→穴まで、全バンパー・全ターゲットに当たる */
		CHECK ((g_dwEvAll & (PB_EV_FLIPHIT | PB_EV_DRAIN | PB_EV_BUMPER | PB_EV_TARGET)) == (PB_EV_FLIPHIT | PB_EV_DRAIN | PB_EV_BUMPER | PB_EV_TARGET));
		CHECK (g_dwFlashAll == ((1u << (PB_BUMPERS + PB_TARGETS)) - 1));
		printf ("当たった印 0x%x\n", g_dwFlashAll);
		CHECK (!PB_UPPER || g_nUpperSteps > 1000);		/* 上のバンパー域も通っている */
	}

	/* 最大の速さで壁に向けて何度撃っても出ない（上・左・右・斜め壁・レーン） */
	{
		const PinballTable &T = PinballGetTable ();
		int nDir, nBad = 0;
		for (nDir = 0; nDir < 72; nDir ++) {
			int nDeg = nDir * 5, k;
			for (k = 0; k < 8; k ++) {
				PinballGame g;
				int nStep;
				g.Start ();
				g.nPhase = PB_PHASE_PLAY;
				g.nX = PbX (30 + k * 12); g.nY = PbY (20 + k * (PB_H - 60) / 8);		/* 盤の上から下まで散らす */
				g.nVx = PbCos (nDeg) * PB_VMAX / PB_TRIG;
				g.nVy = PbSin (nDeg) * PB_VMAX / PB_TRIG;
				for (nStep = 0; nStep < 500; nStep ++) {
					g.Step ();
					if (g.nPhase != PB_PHASE_PLAY) break;
					if (g.nX < PbX (PB_WALL_L) || g.nX > PbX (PB_WALL_R) || g.nY < PbY (0)) { nBad ++; break; }
				}
			}
		}
		CHECK (nBad == 0);
		(void)T;
	}

	/* 穴に落ちる：何もしなければ、いつか穴に落ちて球が減る（ボールセーブが切れた後） */
	{
		PinballGame g;
		int nEv = 0;
		g.Start ();
		g.nPhase = PB_PHASE_PLAY;
		g.nX = PbX (73); g.nY = PbY (150 + PB_YOFS);		/* 2 つのフリッパーの真ん中の上 */
		g.nVx = 0; g.nVy = 0;
		for (i = 0; i < 120 * 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
		CHECK (nEv & PB_EV_DRAIN);
		CHECK (g.nBallsLeft == PB_BALLS - 1 && g.nPhase == PB_PHASE_READY && (nEv & PB_EV_NEXT));
		CHECK (g.nX == PinballGetTable ().nStartX);		/* 次の球は打ち出しレーン */
	}

	/* ボールセーブ：打ち出して約 8 秒の間は落ちても同じ球（1 球 1 回）。あとは減る */
	{
		PinballGame g;
		int nEv;
		g.Start ();
		g.Launch (0);
		g.nPhase = PB_PHASE_PLAY;
		g.nX = PbX (73); g.nY = PbY (215 + PB_YOFS); g.nVx = 0; g.nVy = 300;
		nEv = g.Step ();
		for (i = 0; i < 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
		CHECK ((nEv & PB_EV_DRAIN) && (nEv & PB_EV_SAVE));
		CHECK (g.nBallsLeft == PB_BALLS && g.nPhase == PB_PHASE_READY && g.bSaved);
		/* 同じ球をもう一度打って、また落ちる：今度は減る */
		g.Launch (0);
		g.nPhase = PB_PHASE_PLAY;
		g.nX = PbX (73); g.nY = PbY (215 + PB_YOFS); g.nVx = 0; g.nVy = 300;
		nEv = 0;
		for (i = 0; i < 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
		CHECK ((nEv & PB_EV_DRAIN) && !(nEv & PB_EV_SAVE));
		CHECK (g.nBallsLeft == PB_BALLS - 1 && !g.bSaved);
		/* 8 秒以上たってから落ちる：減る */
		g.Launch (0);
		g.nPhase = PB_PHASE_PLAY;
		g.nSaveSteps = 1;
		g.nX = PbX (73); g.nY = PbY (150 + PB_YOFS); g.nVx = 0; g.nVy = 0;
		nEv = 0;
		for (i = 0; i < 120 * 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
		CHECK ((nEv & PB_EV_DRAIN) && !(nEv & PB_EV_SAVE) && g.nBallsLeft == PB_BALLS - 2);
	}

	/* 3 球で終了：3 球目が落ちたら OVER。終了後は動かず、打ち出せない */
	{
		PinballGame g;
		int nEv = 0, nBall;
		g.Start ();
		for (nBall = 0; nBall < PB_BALLS; nBall ++) {
			g.nPhase = PB_PHASE_PLAY;
			g.nSaveSteps = 0;
			g.nX = PbX (73); g.nY = PbY (150 + PB_YOFS); g.nVx = 0; g.nVy = 0;
			nEv = 0;
			for (i = 0; i < 120 * 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
			CHECK (nEv & PB_EV_DRAIN);
			CHECK (g.nBallsLeft == PB_BALLS - 1 - nBall);
		}
		CHECK ((nEv & PB_EV_OVER) && g.nPhase == PB_PHASE_OVER && g.nBallsLeft == 0);
		CHECK (!g.Launch (500));
		{
			unsigned int dwScore = g.dwScore;
			for (i = 0; i < 100; i ++) g.Step ();
			CHECK (g.nPhase == PB_PHASE_OVER && g.dwScore == dwScore);
		}
	}

	/* フリッパーで打ち返せる：軸の近くの球を、上げたフリッパーが上へ弾く（上げなければ弾かない） */
	{
		const PinballTable &T = PinballGetTable ();
		int nSide, nHeld;
		for (nSide = 0; nSide < 2; nSide++) {
			for (nHeld = 0; nHeld < 2; nHeld ++) {
				PinballGame g;
				int nMinVy = 0;
				g.Start ();
				g.nPhase = PB_PHASE_PLAY;
				g.nX = nSide ? (T.nPivotRx - 14 * PB_FP) : (T.nPivotLx + 14 * PB_FP);		/* フリッパーの上 */
				g.nY = T.nPivotY - 14 * PB_FP;
				g.nVx = 0; g.nVy = 0;
				g.nSaveSteps = 0;
				/* 休み位置のフリッパーに球を乗せる（転がって落ちる前に、少し待ってから上げる） */
				for (i = 0; i < 60; i ++) g.Step ();
				if (nHeld) g.SetFlipper (nSide, 1);
				for (i = 0; i < 30; i ++) {
					g.Step ();
					if (g.nVy < nMinVy) nMinVy = g.nVy;
					if (g.nPhase != PB_PHASE_PLAY) break;
				}
				if (nHeld) {
					CHECK (nMinVy < -200);		/* 強く上へ */
				}
			}
		}
		/* 上げ切ったフリッパーに上から球を落としても、すり抜けず上に乗る・跳ねる */
		{
			PinballGame g;
			g.Start ();
			g.nPhase = PB_PHASE_PLAY;
			g.SetFlipper (0, 1);
			for (i = 0; i < 20; i ++) g.Step ();
			g.nX = T.nPivotLx + 10 * PB_FP; g.nY = T.nPivotY - 40 * PB_FP; g.nVx = 0; g.nVy = 0;
			for (i = 0; i < 60; i ++) {
				g.Step ();
				CHECK (g.nY < T.nPivotY + 6 * PB_FP);		/* 軸の高さより下へすり抜けない */
			}
		}
	}

	/* 得点：バンパー 100・ターゲット 300・全部倒すとボーナス 1000 でターゲット復活・コンボ */
	{
		const PinballTable &T = PinballGetTable ();
		PinballGame g;
		int nEv = 0;

		g.Start ();
		g.nPhase = PB_PHASE_PLAY;
		g.nSaveSteps = 1000;
		g.nX = T.aBumper[0].x + 2 * PB_FP; g.nY = T.aBumper[0].y - 22 * PB_FP;
		g.nVx = 0; g.nVy = 200;
		for (i = 0; i < 40 && !(nEv & PB_EV_BUMPER); i ++) nEv |= g.Step ();
		CHECK (nEv & PB_EV_BUMPER);
		CHECK (g.dwScore == PB_SCORE_BUMPER && g.nCombo == 1 && (g.nFlash & 1));
		CHECK (g.nVy < 0 || g.nVx != 0);		/* 弾かれた */

		/* コンボ：続けて当てると増え、間があくと 0 に戻る */
		g.AddHit (PB_SCORE_BUMPER, PB_EV_BUMPER, 1);
		CHECK (g.nCombo == 2 && g.dwScore == 2 * PB_SCORE_BUMPER);
		g.nPhase = PB_PHASE_READY;			/* 動かない状態でステップだけ進める */
		for (i = 0; i < PB_COMBO_MS * PB_STEP_HZ / 1000 + 2; i ++) g.Step ();
		CHECK (g.nCombo == 0);

		/* ターゲット：下から当てて +300、倒れたものは通り抜ける。3 つ倒すと +1000 で全部復活 */
		g.Start ();
		g.nPhase = PB_PHASE_PLAY;
		g.nSaveSteps = 100000;
		for (int t = 0; t < PB_TARGETS; t ++) {
			const PBSEG &S = T.aSeg[T.nSeg - PB_TARGETS + t];
			unsigned int dwBefore = g.dwScore;
			g.nPhase = PB_PHASE_PLAY;
			g.nX = (S.x1 + S.x2) / 2; g.nY = S.y1 + 16 * PB_FP;
			g.nVx = 0; g.nVy = -300;
			nEv = 0;
			for (i = 0; i < 40 && !(nEv & PB_EV_TARGET); i ++) nEv |= g.Step ();
			CHECK (nEv & PB_EV_TARGET);
			if (t < PB_TARGETS - 1) {
				CHECK (g.dwScore == dwBefore + PB_SCORE_TARGET);
				CHECK (g.nTargetMask & (1u << t));
				/* 倒れたターゲットは通り抜ける */
				g.nX = (S.x1 + S.x2) / 2; g.nY = S.y1 + 16 * PB_FP; g.nVx = 0; g.nVy = -300;
				nEv = 0;
				for (i = 0; i < 20; i ++) nEv |= g.Step ();
				CHECK (!(nEv & PB_EV_TARGET) || (nEv & PB_EV_BUMPER) == 0);
				CHECK (g.nY < S.y1 || g.nPhase != PB_PHASE_PLAY || g.nY > S.y1);
			} else {
				CHECK (nEv & PB_EV_BONUS);
				CHECK (g.dwScore == dwBefore + PB_SCORE_TARGET + PB_SCORE_BONUS);
				CHECK (g.nTargetMask == 0);
			}
		}
	}

	/* 縦に長い盤：最大の打ち出しで上のアーチまで届き、盤の中（上のバンパー域）を通って、下の穴まで戻ってくる */
	{
		const PinballTable &T = PinballGetTable ();
		PinballGame g;
		int nMinY = T.nStartY, nBack = 0, nDrain = 0, nHitUpper = 0;

		CHECK (PB_H >= PB_VIEW_H);
		CHECK (T.nStartY > PbY (PB_H - 40) && T.nArchCy == PbY (PB_ARCH_CY));		/* レーンの床は盤の下端・アーチは盤の最上部 */
		g.Start ();
		CHECK (g.Launch (PB_CHARGE_MS));
		for (i = 0; i < 120 * 40 && !nDrain; i ++) {
			int nEv = g.Step ();
			if (g.nY < nMinY) nMinY = g.nY;
			if (nMinY < PbY (PB_ARCH_CY) && g.nX < T.nLaneL) nBack = 1;		/* 上のアーチを回って、遊ぶ場所へ戻った */
			if (nEv & PB_EV_BUMPER) nHitUpper |= 1;
			if (nEv & PB_EV_DRAIN) nDrain = 1;
		}
		CHECK (nMinY < PbY (PB_ARCH_CY + 40));		/* 盤の最上部のアーチの所まで届く */
		CHECK (nBack);
		CHECK (nDrain);
		/* 最小の打ち出しでは上まで行かず、レーンへ戻る */
		g.Start ();
		g.Launch (0);
		nMinY = T.nStartY;
		for (i = 0; i < 120 * 10; i ++) {
			g.Step ();
			if (g.nY < nMinY) nMinY = g.nY;
		}
		CHECK (nMinY > PbY (PB_H / 2));
		CHECK (g.nPhase == PB_PHASE_READY);
#if PB_UPPER
		/* 上側・中ほどのバンパーに当てると +100（下側と同じ） */
		for (int b = 3; b < PB_BUMPERS; b ++) {
			PinballGame h;
			int nEv = 0;
			h.Start ();
			h.nPhase = PB_PHASE_PLAY;
			h.nSaveSteps = 1000;
			h.nX = T.aBumper[b].x + 2 * PB_FP; h.nY = T.aBumper[b].y - 22 * PB_FP;
			h.nVx = 0; h.nVy = 200;
			for (i = 0; i < 40 && !(nEv & PB_EV_BUMPER); i ++) nEv |= h.Step ();
			CHECK ((nEv & PB_EV_BUMPER) && h.dwScore == PB_SCORE_BUMPER && (h.nFlash & (1u << b)));
		}
#endif
		(void)nHitUpper;
	}

	/* 台詞の読み方：@i<サブゲーム情報の番号> だけ。それ以外は台ではない（-1） */
	{
		CHECK (PinballSpeechInfoNo ("@i0") == 0);
		CHECK (PinballSpeechInfoNo ("@i3") == 3);
		CHECK (PinballSpeechInfoNo ("@i65535") == 65535);
		CHECK (PinballSpeechInfoNo ("@i65536") == -1);
		CHECK (PinballSpeechInfoNo ("@i99999999999") == -1);
		CHECK (PinballSpeechInfoNo ("@i") == -1);
		CHECK (PinballSpeechInfoNo ("@i20,1") == -1);		/* 古い形（台詞にパラメータを並べる）は台にならない */
		CHECK (PinballSpeechInfoNo ("@i3a") == -1);
		CHECK (PinballSpeechInfoNo ("@i-1") == -1);
		CHECK (PinballSpeechInfoNo ("@p3") == -1);
		CHECK (PinballSpeechInfoNo ("i3") == -1);
		CHECK (PinballSpeechInfoNo ("") == -1);
		CHECK (PinballSpeechInfoNo (NULL) == -1);
		/* 既定の盤の位置：席の真上（盤の下端が台NPCのマスの下端）。10×14 ブロックと同じ 5×7 マス */
		CHECK (PinballDefaultDx () == -2 && PinballDefaultDy () == -6);
	}

	/* サブゲーム情報（既定値・範囲の丸め・ゲームの設定）。型は TypedefCommon.h を使わず同じ項目名の構造体で確かめる */
	{
		TestInfo Info;
		PinballConfig Cfg, Def;

		PinballInfoSetDefault (&Info);
		CHECK (Info.wCoinItem == 0 && Info.byCoinCount == 0 && Info.byBalls == PB_BALLS && Info.bySaveSec == PB_SAVE_MS / 1000);
		CHECK (Info.wScoreBumper == PB_SCORE_BUMPER && Info.wScoreTarget == PB_SCORE_TARGET && Info.wScoreBonus == PB_SCORE_BONUS && Info.wComboMs == PB_COMBO_MS);
		CHECK (Info.cDx == -2 && Info.cDy == -6 && Info.cSeatDx == 0 && Info.cSeatDy == 1 && Info.byBandSide == 0 && Info.byVersion == PINBALLINFO_VERSION);
		for (i = 0; i < PINBALLINFO_RESERVED; i ++) CHECK (Info.abyReserved[i] == 0);
		CHECK (Info.wVsSec == PB_VS_SEC_DEFAULT && Info.byPair == 0);
		/* 既定値から作った設定は、設定を渡さない Start と同じ */
		PinballInfoToConfig (&Info, &Cfg);
		Def.SetDefault ();
		CHECK (Cfg.nBalls == Def.nBalls && Cfg.nSaveMs == Def.nSaveMs && Cfg.nComboMs == Def.nComboMs);
		CHECK (Cfg.nScoreBumper == Def.nScoreBumper && Cfg.nScoreTarget == Def.nScoreTarget && Cfg.nScoreBonus == Def.nScoreBonus);
		/* 範囲外は丸める（0 球・席が盤の遠く・得点が大きすぎ・帯の側が 2） */
		memset (&Info, 0, sizeof (Info));
		Info.byBalls = 0; Info.byCoinCount = 200; Info.bySaveSec = 255; Info.byBandSide = 2;
		Info.wScoreBumper = 65535; Info.wComboMs = 65535; Info.cDx = 127; Info.cDy = -128; Info.cSeatDx = 100; Info.cSeatDy = -100;
		PinballInfoClamp (&Info);
		CHECK (Info.byBalls == 1 && Info.byCoinCount == 99 && Info.bySaveSec == 60 && Info.byBandSide == 0);
		CHECK (Info.wScoreBumper == PB_CFG_SCORE_MAX && Info.wComboMs == PB_CFG_MS_MAX);
		CHECK (Info.cDx == PBP_SEAT_LIMIT && Info.cDy == -PBP_SEAT_LIMIT && Info.cSeatDx == PBP_SEAT_LIMIT && Info.cSeatDy == -PBP_SEAT_LIMIT);
		Info.byBalls = 99;
		PinballInfoClamp (&Info);
		CHECK (Info.byBalls == PB_BALLS_MAX);
		/* 対戦の時間：0（未設定・古い保存データ）は既定。短すぎ・長すぎは範囲に丸める */
		Info.wVsSec = 0; PinballInfoClamp (&Info); CHECK (Info.wVsSec == PB_VS_SEC_DEFAULT);
		Info.wVsSec = 3; PinballInfoClamp (&Info); CHECK (Info.wVsSec == PB_VS_SEC_MIN);
		Info.wVsSec = 65535; PinballInfoClamp (&Info); CHECK (Info.wVsSec == PB_VS_SEC_MAX);
		Info.wVsSec = 45; PinballInfoClamp (&Info); CHECK (Info.wVsSec == 45);
	}

	/* 台ごとの設定：球数・得点・ボールセーブ・コンボの時間がゲームに効く */
	{
		PinballGame g;
		PinballConfig Cfg;
		int nEv = 0, nBall;

		Cfg.SetDefault ();
		Cfg.nBalls = 5;
		Cfg.nScoreBumper = 250;
		Cfg.nScoreTarget = 777;
		Cfg.nScoreBonus = 4000;
		Cfg.nSaveMs = 3000;
		Cfg.nComboMs = 500;
		g.Start (&Cfg);
		CHECK (g.nBallsLeft == 5 && g.Cfg.nBalls == 5);
		/* 5 球で終わる */
		for (nBall = 0; nBall < 5; nBall ++) {
			g.nPhase = PB_PHASE_PLAY;
			g.nSaveSteps = 0;
			g.nX = PbX (73); g.nY = PbY (150 + PB_YOFS); g.nVx = 0; g.nVy = 0;
			nEv = 0;
			for (i = 0; i < 120 * 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= g.Step ();
			CHECK (nEv & PB_EV_DRAIN);
			CHECK (g.nBallsLeft == 4 - nBall);
		}
		CHECK ((nEv & PB_EV_OVER) && g.nPhase == PB_PHASE_OVER);
		/* ボールセーブの長さ：打ち出した時に 3 秒ぶん。0 秒ならボールセーブなし */
		g.Start (&Cfg);
		g.Launch (0);
		CHECK (g.nSaveSteps == 3 * PB_STEP_HZ);
		Cfg.nSaveMs = 0;
		g.Start (&Cfg);
		g.Launch (0);
		CHECK (g.nSaveSteps == 0);
		/* 得点（バンパー 250）。コンボは 500ms で切れる */
		g.Start (&Cfg);
		g.AddHit (g.Cfg.nScoreBumper, PB_EV_BUMPER, 0);
		g.AddHit (g.Cfg.nScoreBumper, PB_EV_BUMPER, 0);
		CHECK (g.dwScore == 500 && g.nCombo == 2);
		for (i = 0; i < 500 * PB_STEP_HZ / 1000 + 2; i ++) g.Step ();
		g.AddHit (g.Cfg.nScoreBumper, PB_EV_BUMPER, 0);
		CHECK (g.nCombo == 1);
		/* 設定を渡さない Start は既定（これまでと同じ） */
		g.Start ();
		CHECK (g.nBallsLeft == PB_BALLS && g.Cfg.nScoreBumper == PB_SCORE_BUMPER && g.Cfg.nSaveMs == PB_SAVE_MS);
		/* 範囲外の設定で始めても壊れない（球 0 → 1 球。得点・時間は上限） */
		Cfg.nBalls = 0; Cfg.nScoreBumper = 999999; Cfg.nComboMs = -5;
		g.Start (&Cfg);
		CHECK (g.nBallsLeft == 1 && g.Cfg.nScoreBumper == PB_CFG_SCORE_MAX && g.Cfg.nComboMs == 0);
	}

	/* 対戦：勝敗・同点・時間切れの判定・連勝・乱数列の共有・球の数の上限なし・ボールセーブは最初の球だけ */
	{
		PinballGame a, b, s;
		PinballConfig Cfg;
		int nEv, nBall, nSaveFirst;

		/* 時間切れの勝敗：高い方の勝ち・同点は引き分け */
		CHECK (PinballVsJudge (500, 300) == PB_VS_WIN_A);
		CHECK (PinballVsJudge (300, 500) == PB_VS_WIN_B);
		CHECK (PinballVsJudge (0, 0) == PB_VS_DRAW);
		CHECK (PinballVsJudge (1234, 1234) == PB_VS_DRAW);
		/* 終わりの時刻の判定（時刻が 1 周しても合う）。残り時間 */
		CHECK (!PinballVsTimeUp (1000, 1001) && PinballVsTimeUp (1001, 1001) && PinballVsTimeUp (2000, 1001));
		CHECK (!PinballVsTimeUp (0xFFFFFF00u, 0x100u) && PinballVsTimeUp (0x100u, 0x100u) && PinballVsTimeUp (0x200u, 0x100u));
		CHECK (PinballVsRemainMs (1000, 91000) == 90000 && PinballVsRemainMs (91000, 91000) == 0 && PinballVsRemainMs (95000, 91000) == 0);
		/* 連勝：勝てば +1・引き分けは変わらない・負け/席を立つ/切断は 0 */
		CHECK (PinballStreakNext (0, 1) == 1 && PinballStreakNext (1, 1) == 2 && PinballStreakNext (5, 1) == 6);
		CHECK (PinballStreakNext (3, -1) == 3 && PinballStreakNext (0, -1) == 0);
		CHECK (PinballStreakNext (3, 0) == 0);
		/* 乱数列の共有：同じ種は同じ列・番号は下位 8 ビット・範囲内・種が違えば違う列 */
		{
			int nDiff = 0, nMin = 999, nMax = -999;
			for (i = 0; i < 512; i ++) {
				int j1 = PinballLaunchJitter (777, i), j2 = PinballLaunchJitter (777, i), j3 = PinballLaunchJitter (778, i);
				CHECK (j1 == j2);
				CHECK (j1 >= -PB_VS_JITTER && j1 <= PB_VS_JITTER);
				if (j1 != j3) nDiff ++;
				if (j1 < nMin) nMin = j1;
				if (j1 > nMax) nMax = j1;
			}
			CHECK (PinballLaunchJitter (777, 5) == PinballLaunchJitter (777, 5 + 256));
			CHECK (nDiff > 400);
			CHECK (nMin < -PB_VS_JITTER / 2 && nMax > PB_VS_JITTER / 2);		/* 振れ幅を使い切っている */
		}
		/* ペアの 2 台に同じ種を渡すと、同じ番号の打ち出しは同じ横の速さ。ひとり用は乱数なし（今まで通り真上） */
		Cfg.SetDefault ();
		a.StartVs (&Cfg, 4242);
		b.StartVs (&Cfg, 4242);
		for (i = 0; i < 6; i ++) {
			a.SetReady (); b.SetReady ();
			a.Launch (300); b.Launch (700);			/* 押していた時間が違っても、横の速さの乱数は同じ */
			CHECK (a.nVx == b.nVx);
			CHECK (a.nVx == PinballLaunchJitter (4242, i));
		}
		CHECK (a.nLaunchNo == 6 && b.nLaunchNo == 6);
		s.Start (&Cfg);
		s.Launch (300);
		CHECK (s.nVx == 0 && !s.bVs);
		/* 球の数に上限がない：何度落ちても終わらず、次の球が出る。球数は減らない */
		a.StartVs (&Cfg, 99);
		CHECK (a.bVs && a.nSeed == 99);
		for (nBall = 0; nBall < Cfg.nBalls * 3 + 2; nBall ++) {
			a.nPhase = PB_PHASE_PLAY;
			a.nSaveSteps = 0;
			a.nX = PbX (73); a.nY = PbY (150 + PB_YOFS); a.nVx = 0; a.nVy = 0;
			nEv = 0;
			for (i = 0; i < 120 * 20 && !(nEv & PB_EV_DRAIN); i ++) nEv |= a.Step ();
			CHECK (nEv & PB_EV_DRAIN);
			CHECK (!(nEv & PB_EV_OVER));
			CHECK (a.nPhase == PB_PHASE_READY && (nEv & PB_EV_NEXT));
			CHECK (a.nBallsLeft == Cfg.nBalls);
			CHECK (a.nBallNo == nBall + 1);
		}
		/* ボールセーブは最初の球だけ（最初の球では効き、落ちて次の球になるともう効かない） */
		a.StartVs (&Cfg, 99);
		a.Launch (0);
		nSaveFirst = a.nSaveSteps;
		CHECK (nSaveFirst == Cfg.nSaveMs * PB_STEP_HZ / 1000 && nSaveFirst > 0);
		a.nSaveSteps = 0;
		a.nX = PbX (73); a.nY = PbY (150 + PB_YOFS); a.nVx = 0; a.nVy = 0;
		for (i = 0; i < 120 * 20 && a.nPhase == PB_PHASE_PLAY; i ++) a.Step ();
		CHECK (a.nBallNo == 1 && a.nPhase == PB_PHASE_READY);
		a.Launch (0);
		CHECK (a.nSaveSteps == 0);
		/* ひとり用は今まで通り（球が減って 3 球で終わる・毎回ボールセーブ） */
		s.Start (&Cfg);
		s.Launch (0);
		CHECK (s.nSaveSteps > 0 && s.nBallsLeft == Cfg.nBalls);
	}

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
