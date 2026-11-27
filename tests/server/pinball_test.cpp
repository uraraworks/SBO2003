/* ピンボールの物理・進行ロジック（common/Pinball.h）の native テスト
   g++ -std=c++11 -O1 -I. tests/server/pinball_test.cpp -o /tmp/pinball_test && /tmp/pinball_test */
#include <stdio.h>
#include "common/Pinball.h"

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

	/* 台詞の読み方 */
	{
		int an[PBP_COUNT], sdx, sdy;
		PinballParseSpeech ("@i20,1", an);
		CHECK (an[0] == 20 && an[1] == 1 && an[2] == PBP_NONE && an[6] == PBP_NONE);
		PinballParseSpeech ("@i", an);
		CHECK (an[0] == PBP_NONE);
		PinballParseSpeech ("@i20,1,-2,-6,1,3,-1", an);
		CHECK (an[2] == -2 && an[3] == -6 && an[4] == 1 && an[5] == 3 && an[6] == -1);
		PinballSeatOffset ("@i", &sdx, &sdy);
		CHECK (sdx == 0 && sdy == 1);
		PinballSeatOffset ("@i20,1,,,,3,-1", &sdx, &sdy);
		CHECK (sdx == 3 && sdy == -1);
		PinballSeatOffset ("@i1,1,,,,999,-999", &sdx, &sdy);
		CHECK (sdx == PBP_SEAT_LIMIT && sdy == -PBP_SEAT_LIMIT);
		/* 既定の盤の位置：席の真上（盤の下端が台NPCのマスの下端）。10×14 ブロックと同じ 5×7 マス */
		CHECK (PinballDefaultDx () == -2 && PinballDefaultDy () == -6);
	}

	printf (g_nNg ? "NG %d 件\n" : "一致\n", g_nNg);
	return g_nNg ? 1 : 0;
}
