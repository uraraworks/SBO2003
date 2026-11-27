/* ========================================================================= */
/* ファイル名：	Pinball.h													 */
/* 概要：		ピンボール（縦長の台1つ）の物理・ゲーム進行ロジック				 */
/*				サーバー・クライアント共通。STL（string.h）だけに依存し、通信・時計を知らない。 */
/*				整数の固定小数点だけで計算する（浮動小数点は使わない）ので、		 */
/*				同じ入力からは必ず同じ結果になる（サーバーが正、クライアントは先読み用）。	 */
/*				単体テスト: tests/server/pinball_test.cpp						 */
/* 日付：		2026/11/26													 */
/* ========================================================================= */
/*
 * 単位：位置・長さは 1/256 ドット（PB_FP）。速度は「1 ステップあたりの 1/256 ドット」。
 *       時間は固定ステップ 1/120 秒（PB_STEP_HZ）。Update に経過ミリ秒を渡すと、その分だけステップを回す。
 * 台の形（設計は 160×224 の座標。PB_W・PB_H を変えると配置が比例して伸び縮みする）：
 *   左右の壁・上部のアーチ（円の内側）・左右の下の斜め壁（フリッパー溝）・左右フリッパー（回る線分）・
 *   中央下の穴（落ちると球が減る）・右端の打ち出しレーン（プランジャー）・バンパー 3 個・ターゲット 3 個。
 * 衝突：ボール（円）対 壁（線分）・アーチ（内側の円）・バンパー（円）・ターゲット（線分）・フリッパー（回る線分＋太さ）。
 *       1 ステップの移動が小さくなるようにステップをさらに分けて（サブステップ）調べるので、すり抜けない。
 * 通信するもの：ボール位置・速度・フリッパーの角度（段階）・ターゲットの倒れた印・得点・残り球・状態。
 */

#pragma once

#include <string.h>
#include <stdlib.h>

/* ========================================================================= */
/* 調整しやすい定数															 */
/* ========================================================================= */
#define PB_W				(160)		/* 盤の幅(ドット)。落ち物の 10×14 ブロック(16 ドット)と同じ場所取り */
#define PB_VIEW_H			(224)		/* 見える窓の高さ(ドット)。マップ上の場所取り（160×224）はこれ。盤が縦に長くても変わらない */
#ifndef PB_H
#define PB_H				(448)		/* 盤全体の高さ(ドット)。PB_VIEW_H 以上なら何でもよい（既定は 2 画面分）。物理は盤全体で動き、窓がボールを追って上下に動く */
#endif
#define PB_YOFS				(PB_H - PB_VIEW_H)	/* 窓 1 つ分の下の部分（フリッパー・穴・レーン）を盤の下端に置くための下げ幅(ドット) */
#define PB_UPPER			(PB_YOFS >= 120)	/* 盤が十分に高い時だけ、上側・中ほどのバンパー・ターゲットを足す */
#define PB_DESIGN_W			(160)		/* 配置を決めた時の幅（PB_W がこれと違うと配置を比例させる） */
#define PB_FP				(256)		/* 1 ドット＝256 */
#define PB_STEP_HZ			(120)		/* 固定ステップ（1 秒あたり） */
#define PB_MAX_CATCHUP		(60)		/* 1 回の Update で回すステップの上限（これを超える遅れは捨てる） */
#define PB_BALLS			(3)			/* ひとり練習の球数 */
#define PB_BALL_R			(4 * PB_FP)	/* ボールの半径 */
#define PB_GRAVITY			(5)			/* 重力（1 ステップあたり、下向き） */
#define PB_VMAX				(900)		/* ボールの速さの上限（1 ステップあたり。約 420 ドット/秒） */
#define PB_VMAX_LANE		(1700)		/* 打ち出しレーンの中だけの上限（盤が高いと、上のアーチまで届く強い打ち出しが要る） */
#define PB_REST_V			(14)		/* これより小さい面に垂直な速さは跳ね返さずに止める（がたつき防止） */
#define PB_E_WALL			(150)		/* 壁の反発（/256） */
#define PB_E_ARCH			(90)		/* アーチの反発（/256。ここは小さくして、沿って回りやすくする） */
#define PB_E_TARGET			(150)
#define PB_E_FLIPPER		(110)
#define PB_BUMPER_R			(9 * PB_FP)	/* バンパーの半径 */
#define PB_BUMPER_KICK		(640)		/* バンパーが返す速さの下限 */
#define PB_TARGET_R			(2 * PB_FP)	/* ターゲット（線分）の太さの半分 */
#define PB_FLIP_LEN			(28)		/* フリッパーの長さ(ドット) */
#define PB_FLIP_R			(3 * PB_FP)	/* フリッパーの太さの半分 */
#define PB_FLIP_STEPS		(12)		/* フリッパーが休み位置から上がり切るまでの段階数（1 ステップで 1 段階動く＝約 0.1 秒） */
#define PB_FLIP_DEG			(5)			/* 1 段階の角度(度) */
#define PB_FLIP_REST_DEG	(30)		/* 休み位置の角度（水平から下向き。上がり切ると逆に同じだけ上向き） */
#define PB_LAUNCH_MIN		(350)		/* 打ち出しの強さ（1 ステップあたりの速さ）の最小。最大は盤の高さから決まる（PinballTable::nLaunchMax） */
#define PB_LAUNCH_REACH		(17)		/* 打ち出しの最大の強さ＝レーンの床からアーチまで届くのに要る強さの PB_LAUNCH_REACH/10 倍（二乗の比。1.7 倍） */
#define PB_CHARGE_MS		(1000)		/* ↓を押してこの時間で最大の強さ（それ以上は上限） */
#define PB_SAVE_MS			(8000)		/* 球を打ち出してからこの間は、穴に落ちても同じ球（ボールセーブ。1 球につき 1 回） */
#define PB_COMBO_MS			(2000)		/* この間に続けてヒットするとコンボ */
#define PB_SCORE_BUMPER		(100)
#define PB_SCORE_TARGET		(300)
#define PB_SCORE_BONUS		(1000)		/* ターゲットを全部倒した時のボーナス */
#define PB_STALL_MS			(5000)		/* 球がほとんど動かないままこの時間たつと、軽く弾いて救う */
#define PB_TARGETS			(PB_UPPER ? 6 : 3)	/* 0〜2 が下側、3〜5 が上側 */
#define PB_BUMPERS			(PB_UPPER ? 8 : 3)	/* 0〜2 が下側、3〜5 が上側（アーチの下）、6・7 が中ほど */
#define PB_TRIG				(1024)		/* sin/cos の表の 1.0 */
#define PB_NORM				(1024)		/* 法線ベクトルの 1.0 */

/* ゲームの状態 */
enum {
	PB_PHASE_READY = 0,		/* 球が打ち出しレーンで待っている */
	PB_PHASE_PLAY,			/* 球が動いている */
	PB_PHASE_WAIT,			/* （先読み用）穴に落ちた。サーバーの返事を待つ */
	PB_PHASE_OVER,			/* 終了 */
};
/* Update / Step が返す出来事のビット */
#define PB_EV_BUMPER		(0x001)		/* バンパーに当たった */
#define PB_EV_TARGET		(0x002)		/* ターゲットを倒した */
#define PB_EV_BONUS			(0x004)		/* ターゲットを全部倒した（ターゲット復活） */
#define PB_EV_DRAIN			(0x008)		/* 穴に落ちた（球が減った。ボールセーブなら同じ球に戻った） */
#define PB_EV_SAVE			(0x010)		/* ボールセーブで同じ球に戻った */
#define PB_EV_NEXT			(0x020)		/* 次の球が打ち出しレーンに来た */
#define PB_EV_OVER			(0x040)		/* 3 球目が落ちて終了 */
#define PB_EV_LAUNCH		(0x080)		/* 打ち出した */
#define PB_EV_FLIPHIT		(0x100)		/* フリッパーで打ち返した */
#define PB_EV_WALL			(0x200)		/* 壁・アーチに当たった */
#define PB_EV_STALL			(0x400)		/* 動かない球を救った */
/* 当たった印（nFlash）のビット。0〜PB_BUMPERS-1 がバンパー、PB_BUMPERS〜 がターゲット */

/* ========================================================================= */
/* 整数の計算																 */
/* ========================================================================= */

/* 整数の平方根（切り捨て） */
static inline unsigned int PbIsqrt(unsigned long long n)
{
	unsigned long long x = 0, bit = 1ULL << 62;

	while (bit > n) {
		bit >>= 2;
	}
	while (bit != 0) {
		if (n >= x + bit) {
			n -= x + bit;
			x = (x >> 1) + bit;
		} else {
			x >>= 1;
		}
		bit >>= 2;
	}
	return (unsigned int)x;
}

/* sin の表（0〜90 度を 5 度刻み。PB_TRIG が 1.0） */
static const short g_asPbSin90[19] = {
	0, 89, 178, 265, 350, 433, 512, 587, 658, 724, 784, 839, 887, 928, 962, 989, 1008, 1020, 1024
};

/* 度（5 の倍数。負も可）の sin。y が下向きの座標でそのまま使える */
static inline int PbSin(int nDeg)
{
	int nSign = 1;

	nDeg %= 360;
	if (nDeg < 0) {
		nDeg += 360;
	}
	if (nDeg >= 180) {
		nSign = -1;
		nDeg -= 180;
	}
	if (nDeg > 90) {
		nDeg = 180 - nDeg;
	}
	return nSign * g_asPbSin90[nDeg / 5];
}
static inline int PbCos(int nDeg)
{
	return PbSin (nDeg + 90);
}

/* 設計座標 → 実際の固定小数点。横は 160 を PB_W に比例させる。縦は盤の上端からのドット数そのまま（盤が高いぶんは PB_YOFS で下側に足す） */
static inline int PbX(int nDesign) { return (int)((long long)nDesign * PB_W * PB_FP / PB_DESIGN_W); }
static inline int PbY(int nDesign) { return nDesign * PB_FP; }

/* フリッパーの向き（段階 a=0 が休み位置）。戻り値は水平から下向きの角度(度)。a が増えると上がる */
static inline int PbFlipDeg(int nAng)
{
	return PB_FLIP_REST_DEG - PB_FLIP_DEG * nAng;
}

/* ========================================================================= */
/* 台の形																	 */
/* ========================================================================= */
typedef struct _PBSEG {
	int		x1, y1, x2, y2;		/* 線分の両端（固定小数点） */
	int		nE;					/* 反発（/256） */
	int		nTarget;			/* -1=ただの壁、0 以上=ターゲットの番号 */
} PBSEG;
typedef struct _PBCIRCLE {
	int		x, y;
} PBCIRCLE;

/* 台の形の寸法（設計座標）。フリッパーの軸は左 (PB_PIVOT_LX, PB_PIVOT_Y)・右 (PB_PIVOT_RX, PB_PIVOT_Y) */
#define PB_WALL_L			(6)			/* 左の壁 */
#define PB_WALL_R			(156)		/* 右の壁（打ち出しレーンの外側） */
#define PB_LANE_L			(141)		/* 打ち出しレーンの仕切り（遊ぶ場所の右の壁） */
#define PB_ARCH_CY			(80)		/* アーチ（半円）の中心の高さ（盤の最上部）。ここより上がアーチ */
#define PB_DIAG_TOP			(172 + PB_YOFS)	/* 下の斜め壁の上の端の高さ（ここから下が盤の下側の 1 画面分） */
#define PB_PIVOT_LX			(38)
#define PB_PIVOT_RX			(109)
#define PB_PIVOT_Y			(200 + PB_YOFS)
#define PB_LANE_FLOOR		(216 + PB_YOFS)	/* 打ち出しレーンの床 */
#define PB_LANE_SEPA_TOP	(84)		/* 仕切りの上の端（盤の上の方まで続く） */
#define PB_SEG_MAX			(20)

struct PinballTable {
	PBSEG		aSeg[PB_SEG_MAX];
	int			nSeg;
	PBCIRCLE	aBumper[PB_BUMPERS];
	int			nArchCx, nArchCy, nArchR;		/* アーチ（内側の円） */
	int			nPivotLx, nPivotRx, nPivotY;	/* フリッパーの軸 */
	int			nLaneL;							/* 打ち出しレーンの左端（x がこれより大きい所がレーン） */
	int			nStartX, nStartY;				/* 球が打ち出しを待つ位置 */
	int			nDrainY;						/* これより下に球の中心が行くと穴に落ちた */
	int			nLaunchMax;						/* 打ち出しの最大の強さ */

	void AddSeg(int x1, int y1, int x2, int y2, int nE, int nTarget)
	{
		PBSEG &s = aSeg[nSeg ++];
		s.x1 = PbX (x1); s.y1 = PbY (y1); s.x2 = PbX (x2); s.y2 = PbY (y2);
		s.nE = nE; s.nTarget = nTarget;
	}
	PinballTable()
	{
		nSeg = 0;
		AddSeg (PB_WALL_L, PB_ARCH_CY, PB_WALL_L, PB_DIAG_TOP, PB_E_WALL, -1);				/* 左の壁 */
		AddSeg (PB_WALL_L, PB_DIAG_TOP, PB_PIVOT_LX, PB_PIVOT_Y, PB_E_WALL, -1);			/* 左の斜め壁 */
		AddSeg (PB_LANE_L, PB_DIAG_TOP, PB_PIVOT_RX, PB_PIVOT_Y, PB_E_WALL, -1);			/* 右の斜め壁 */
		AddSeg (PB_LANE_L, PB_LANE_SEPA_TOP, PB_LANE_L, PB_LANE_FLOOR, PB_E_WALL, -1);		/* レーンの仕切り */
		AddSeg (PB_WALL_R, PB_ARCH_CY, PB_WALL_R, PB_LANE_FLOOR, PB_E_WALL, -1);			/* 右の壁（レーンの外側） */
		AddSeg (PB_LANE_L, PB_LANE_FLOOR, PB_WALL_R, PB_LANE_FLOOR, PB_E_WALL, -1);		/* レーンの床 */
		AddSeg (PB_PIVOT_LX, PB_PIVOT_Y, PB_PIVOT_LX, PB_PIVOT_Y + 34, PB_E_WALL, -1);	/* 穴へ落ちる縦穴の壁（軸の下。軸の外側へ回り込めないようにする） */
		AddSeg (PB_PIVOT_RX, PB_PIVOT_Y, PB_PIVOT_RX, PB_PIVOT_Y + 34, PB_E_WALL, -1);
		/* 下側の窓の中のターゲット 3 つ・バンパー 3 つ（盤が 1 画面分の時の配置。盤が高い時は下端に寄せる） */
		AddSeg (28, 130 + PB_YOFS, 44, 130 + PB_YOFS, PB_E_TARGET, 0);
		AddSeg (66, 130 + PB_YOFS, 82, 130 + PB_YOFS, PB_E_TARGET, 1);
		AddSeg (104, 130 + PB_YOFS, 120, 130 + PB_YOFS, PB_E_TARGET, 2);
		aBumper[0].x = PbX (58);  aBumper[0].y = PbY (62 + PB_YOFS);
		aBumper[1].x = PbX (100); aBumper[1].y = PbY (62 + PB_YOFS);
		aBumper[2].x = PbX (79);  aBumper[2].y = PbY (92 + PB_YOFS);
#if PB_UPPER
		/* 上側（アーチの下）にターゲット 3 つ・バンパー 3 つ、中ほどにバンパー 2 つ */
		AddSeg (28, 150, 44, 150, PB_E_TARGET, 3);
		AddSeg (66, 150, 82, 150, PB_E_TARGET, 4);
		AddSeg (104, 150, 120, 150, PB_E_TARGET, 5);
		aBumper[3].x = PbX (58);  aBumper[3].y = PbY (62);
		aBumper[4].x = PbX (100); aBumper[4].y = PbY (62);
		aBumper[5].x = PbX (79);  aBumper[5].y = PbY (92);
		aBumper[6].x = PbX (34);  aBumper[6].y = PbY ((150 + 130 + PB_YOFS) / 2);
		aBumper[7].x = PbX (124); aBumper[7].y = PbY ((150 + 130 + PB_YOFS) / 2);
#endif
		nArchCx = (PbX (PB_WALL_L) + PbX (PB_WALL_R)) / 2;
		nArchCy = PbY (PB_ARCH_CY);
		nArchR = (PbX (PB_WALL_R) - PbX (PB_WALL_L)) / 2;
		nPivotLx = PbX (PB_PIVOT_LX);
		nPivotRx = PbX (PB_PIVOT_RX);
		nPivotY = PbY (PB_PIVOT_Y);
		nLaneL = PbX (PB_LANE_L);
		nStartX = (PbX (PB_LANE_L) + PbX (PB_WALL_R)) / 2;
		nStartY = PbY (PB_LANE_FLOOR) - PB_BALL_R - 1;
		nDrainY = PbY (PB_PIVOT_Y) + 22 * PB_FP;
		{
			/* 打ち出しの最大：レーンの床から、レーンの真上のアーチの縁まで届くのに要る速さの二乗に PB_LAUNCH_REACH/10 を掛けて平方根 */
			long long nLim = (long long)nArchR - PB_BALL_R, nDx = nStartX - nArchCx;
			long long nTop = nArchCy - (long long)PbIsqrt ((unsigned long long)(nLim * nLim - nDx * nDx));	/* 球の中心が行けるアーチの一番上（レーンの真上） */
			long long nRise = nStartY - nTop;		/* 1 ステップあたり重力 PB_GRAVITY：v^2 = 2*g*距離 */
			nLaunchMax = (int)PbIsqrt ((unsigned long long)(2LL * PB_GRAVITY * nRise * PB_LAUNCH_REACH / 10));
			if (nLaunchMax < PB_LAUNCH_MIN + 100) nLaunchMax = PB_LAUNCH_MIN + 100;
			if (nLaunchMax > PB_VMAX_LANE) nLaunchMax = PB_VMAX_LANE;
		}
	}
};

static inline const PinballTable &PinballGetTable(void)
{
	static const PinballTable s_Table;
	return s_Table;
}

/* フリッパーの先端（固定小数点）。bRight なら右のフリッパー（左右反転） */
static inline void PinballFlipperTip(const PinballTable &T, int bRight, int nAng, int *pnX, int *pnY)
{
	int nDeg = PbFlipDeg (nAng);
	int nLen = PB_FLIP_LEN * PB_FP;
	int dx = (int)((long long)nLen * PbCos (nDeg) / PB_TRIG);
	int dy = (int)((long long)nLen * PbSin (nDeg) / PB_TRIG);

	*pnX = bRight ? (T.nPivotRx - dx) : (T.nPivotLx + dx);
	*pnY = T.nPivotY + dy;
}

/* 点 (px,py) に一番近い線分上の点（固定小数点）と、その位置の割合（/1024） */
static inline void PbClosest(int x1, int y1, int x2, int y2, int px, int py, int *pnQx, int *pnQy, int *pnT)
{
	long long dx = x2 - x1, dy = y2 - y1;
	long long len2 = dx * dx + dy * dy, t;

	if (len2 == 0) {
		t = 0;
	} else {
		t = ((long long)(px - x1) * dx + (long long)(py - y1) * dy) * 1024 / len2;
		if (t < 0) t = 0;
		if (t > 1024) t = 1024;
	}
	*pnT = (int)t;
	*pnQx = (int)(x1 + dx * t / 1024);
	*pnQy = (int)(y1 + dy * t / 1024);
}

/* ========================================================================= */
/* ゲーム																	 */
/* ========================================================================= */
struct PinballGame {
	int				nPhase;				/* PB_PHASE_* */
	int				nX, nY;				/* ボールの中心（固定小数点） */
	int				nVx, nVy;			/* ボールの速度（1 ステップあたり） */
	int				nAngL, nAngR;		/* フリッパーの段階（0=休み〜PB_FLIP_STEPS=上がり切り） */
	int				bHoldL, bHoldR;		/* 左右のフリッパーを上げている（キーを押している） */
	unsigned int	nTargetMask;		/* 倒れたターゲットの印（ビット i がターゲット i） */
	unsigned int	nFlash;				/* 当たった印（呼ぶ側が読んで 0 に戻す。見た目の光らせ用） */
	unsigned int	dwScore;			/* 得点 */
	int				nBallsLeft;			/* 残り球（今の球を含む） */
	int				nCombo;				/* 連続ヒット数 */
	int				nComboSteps;		/* コンボが切れるまでの残りステップ */
	int				nSaveSteps;			/* ボールセーブの残りステップ（0=なし） */
	int				bSaved;				/* この球はボールセーブを使った */
	int				nStall;				/* 球がほとんど動かない状態が続いたステップ数 */
	int				bPredict;			/* 先読み用（穴に落ちても球を減らさず、サーバーの返事を待つ） */
	unsigned int	dwStep;				/* 開始からの固定ステップ数 */
	unsigned int	nAcc;				/* Update の端数（ミリ秒×PB_STEP_HZ。1000 で 1 ステップ） */

	void Start(void)
	{
		memset (this, 0, sizeof (*this));
		nBallsLeft = PB_BALLS;
		SetReady ();
	}
	/* 打ち出しレーンで球を待たせる */
	void SetReady(void)
	{
		const PinballTable &T = PinballGetTable ();
		nPhase = PB_PHASE_READY;
		nX = T.nStartX;
		nY = T.nStartY;
		nVx = nVy = 0;
		nSaveSteps = 0;
		nStall = 0;
	}
	void SetFlipper(int bRight, int bHold)
	{
		if (bRight) bHoldR = bHold ? 1 : 0; else bHoldL = bHold ? 1 : 0;
	}
	/* 打ち出す。nHoldMs は↓を押していた時間。打ち出せたら TRUE */
	int Launch(int nHoldMs)
	{
		int nPower;

		if (nPhase != PB_PHASE_READY) {
			return 0;
		}
		if (nHoldMs < 0) nHoldMs = 0;
		if (nHoldMs > PB_CHARGE_MS) nHoldMs = PB_CHARGE_MS;
		nPower = PB_LAUNCH_MIN + (PinballGetTable ().nLaunchMax - PB_LAUNCH_MIN) * nHoldMs / PB_CHARGE_MS;
		nVx = 0;
		nVy = -nPower;
		nPhase = PB_PHASE_PLAY;
		nSaveSteps = bSaved ? 0 : (PB_SAVE_MS * PB_STEP_HZ / 1000);
		nStall = 0;
		return 1;
	}
	/* 打ち出しの強さ（0〜100 ％。見た目の目盛り用） */
	static int ChargePercent(int nHoldMs)
	{
		if (nHoldMs < 0) nHoldMs = 0;
		if (nHoldMs > PB_CHARGE_MS) nHoldMs = PB_CHARGE_MS;
		return nHoldMs * 100 / PB_CHARGE_MS;
	}
	/* 経過ミリ秒ぶんだけ固定ステップを進める。出来事のビットを返す */
	int Update(unsigned int dwMs)
	{
		int nEv = 0, nSteps = 0;

		if (dwMs > 5000) dwMs = 5000;
		nAcc += dwMs * PB_STEP_HZ;
		while (nAcc >= 1000) {
			nAcc -= 1000;
			if (nSteps >= PB_MAX_CATCHUP) {
				nAcc %= 1000;		/* 遅れすぎは捨てる */
				break;
			}
			nEv |= Step ();
			nSteps ++;
		}
		return nEv;
	}
	/* 1 ステップ（1/120 秒） */
	int Step(void);

	/* 球のドット座標（描画用。四捨五入） */
	int BallPxX(void) const { return (nX + PB_FP / 2) / PB_FP; }
	int BallPxY(void) const { return (nY + PB_FP / 2) / PB_FP; }

	/* 内部用 */
	void Reflect(int nNx, int nNy, int nE, int nSx, int nSy);
	int  CollideSeg(const PBSEG &S, int nExtraR);
	int  CollideFlipper(const PinballTable &T, int bRight, int nPrevAng);
	int  AddHit(int nScore, int nEvent, int nFlashBit);
	void LimitSpeed(void);
};

/* 速さを上限に収める（大きさは整数の平方根） */
inline void PinballGame::LimitSpeed(void)
{
	unsigned long long v2 = (unsigned long long)((long long)nVx * nVx + (long long)nVy * nVy);

	/* 打ち出しレーンの中は上限が高い（盤が高い時、強く打ち出して上のアーチまで届かせるため） */
	long long nMax = (nX > PinballGetTable ().nLaneL) ? PB_VMAX_LANE : PB_VMAX;

	if (v2 > (unsigned long long)(nMax * nMax)) {
		unsigned int v = PbIsqrt (v2);
		nVx = (int)((long long)nVx * nMax / (long long)v);
		nVy = (int)((long long)nVy * nMax / (long long)v);
	}
}

/* 面（法線 nN。長さ PB_NORM）で跳ね返る。面が速さ (nSx,nSy) で動いている時は相対速度で反発する。
   面に向かっていなければ何もしない */
inline void PinballGame::Reflect(int nNx, int nNy, int nE, int nSx, int nSy)
{
	long long rvx = nVx - nSx, rvy = nVy - nSy;
	long long vn = (rvx * nNx + rvy * nNy) / PB_NORM;

	if (vn >= 0) {
		return;
	}
	if (-vn < PB_REST_V) {
		/* ごく小さい衝突は跳ね返さず、面に垂直な成分だけ消す（がたつき防止） */
		rvx -= vn * nNx / PB_NORM;
		rvy -= vn * nNy / PB_NORM;
	} else {
		long long k = vn * (256 + nE) / 256;
		rvx -= k * nNx / PB_NORM;
		rvy -= k * nNy / PB_NORM;
	}
	nVx = (int)(rvx + nSx);
	nVy = (int)(rvy + nSy);
}

inline int PinballGame::AddHit(int nScore, int nEvent, int nFlashBit)
{
	if (nComboSteps > 0) {
		nCombo ++;
	} else {
		nCombo = 1;
	}
	nComboSteps = PB_COMBO_MS * PB_STEP_HZ / 1000;
	dwScore += (unsigned int)nScore;
	nFlash |= 1u << nFlashBit;
	return nEvent;
}

/* 線分（太さ nExtraR）との衝突。当たったら 1。ボールを面の外へ押し戻して反発する */
inline int PinballGame::CollideSeg(const PBSEG &S, int nExtraR)
{
	int qx, qy, t;
	long long dx, dy, d2, rr;
	unsigned int d;
	int nNx, nNy;

	PbClosest (S.x1, S.y1, S.x2, S.y2, nX, nY, &qx, &qy, &t);
	dx = nX - qx;
	dy = nY - qy;
	rr = PB_BALL_R + nExtraR;
	d2 = dx * dx + dy * dy;
	if (d2 >= rr * rr) {
		return 0;
	}
	d = PbIsqrt ((unsigned long long)d2);
	if (d == 0) {
		/* 中心が線分の上に乗った：線分の法線のうち上向きへ */
		nNx = 0; nNy = -PB_NORM;
		d = 0;
	} else {
		nNx = (int)(dx * PB_NORM / (long long)d);
		nNy = (int)(dy * PB_NORM / (long long)d);
	}
	nX = qx + (int)((long long)nNx * rr / PB_NORM);
	nY = qy + (int)((long long)nNy * rr / PB_NORM);
	Reflect (nNx, nNy, S.nE, 0, 0);
	return 1;
}

/* フリッパーとの衝突。フリッパーが動いている分（前の段階との先端の差）を面の速度として反発する */
inline int PinballGame::CollideFlipper(const PinballTable &T, int bRight, int nPrevAng)
{
	int nAng = bRight ? nAngR : nAngL;
	int px = bRight ? T.nPivotRx : T.nPivotLx, py = T.nPivotY;
	int tx, ty, ptx, pty, qx, qy, t, sx, sy;
	long long dx, dy, d2, rr;
	unsigned int d;
	int nNx, nNy;

	PinballFlipperTip (T, bRight, nAng, &tx, &ty);
	PbClosest (px, py, tx, ty, nX, nY, &qx, &qy, &t);
	dx = nX - qx;
	dy = nY - qy;
	rr = PB_BALL_R + PB_FLIP_R;
	d2 = dx * dx + dy * dy;
	if (d2 >= rr * rr) {
		return 0;
	}
	d = PbIsqrt ((unsigned long long)d2);
	if (d == 0) {
		nNx = 0; nNy = -PB_NORM;
	} else {
		nNx = (int)(dx * PB_NORM / (long long)d);
		nNy = (int)(dy * PB_NORM / (long long)d);
	}
	/* 当たった所の面の速度＝前の段階の同じ割合の位置との差 */
	PinballFlipperTip (T, bRight, nPrevAng, &ptx, &pty);
	sx = (int)((long long)(tx - ptx) * t / 1024);
	sy = (int)((long long)(ty - pty) * t / 1024);
	nX = qx + (int)((long long)nNx * rr / PB_NORM);
	nY = qy + (int)((long long)nNy * rr / PB_NORM);
	{
		int nVxBefore = nVx, nVyBefore = nVy;
		Reflect (nNx, nNy, PB_E_FLIPPER, sx, sy);
		return (nVx != nVxBefore) || (nVy != nVyBefore);
	}
}

inline int PinballGame::Step(void)
{
	const PinballTable &T = PinballGetTable ();
	int nEv = 0, nPrevL = nAngL, nPrevR = nAngR, i, nSub, nMax;
	int nHitL = 0, nHitR = 0;

	dwStep ++;
	/* フリッパーは 1 ステップに 1 段階ずつ動く */
	if (bHoldL) { if (nAngL < PB_FLIP_STEPS) nAngL ++; } else { if (nAngL > 0) nAngL --; }
	if (bHoldR) { if (nAngR < PB_FLIP_STEPS) nAngR ++; } else { if (nAngR > 0) nAngR --; }
	if (nComboSteps > 0) {
		nComboSteps --;
		if (nComboSteps == 0) {
			nCombo = 0;
		}
	}
	if (nPhase != PB_PHASE_PLAY) {
		return 0;
	}
	if (nSaveSteps > 0) {
		nSaveSteps --;
	}
	nVy += PB_GRAVITY;
	LimitSpeed ();
	/* サブステップ：1 回の移動が球の半径の半分以下になるように分ける（すり抜け防止） */
	nMax = (abs (nVx) > abs (nVy)) ? abs (nVx) : abs (nVy);
	nSub = nMax * 2 / PB_BALL_R + 1;
	if (nSub > 8) nSub = 8;
	for (i = 0; i < nSub; i ++) {
		int j;

		nX += nVx / nSub;
		nY += nVy / nSub;
		/* アーチ（円の内側。中心より上だけ） */
		if (nY < T.nArchCy) {
			long long dx = nX - T.nArchCx, dy = nY - T.nArchCy;
			long long lim = (long long)T.nArchR - PB_BALL_R;
			if (dx * dx + dy * dy > lim * lim) {
				unsigned int d = PbIsqrt ((unsigned long long)(dx * dx + dy * dy));
				int nNx = (int)(-dx * PB_NORM / (long long)d), nNy = (int)(-dy * PB_NORM / (long long)d);	/* 内向き */
				nX = T.nArchCx + (int)(dx * lim / (long long)d);
				nY = T.nArchCy + (int)(dy * lim / (long long)d);
				Reflect (nNx, nNy, PB_E_ARCH, 0, 0);
				nEv |= PB_EV_WALL;
			}
		}
		/* 壁・ターゲット */
		for (j = 0; j < T.nSeg; j ++) {
			const PBSEG &S = T.aSeg[j];
			if (S.nTarget >= 0) {
				if (nTargetMask & (1u << S.nTarget)) {
					continue;		/* 倒れたターゲットは通り抜ける */
				}
				if (CollideSeg (S, PB_TARGET_R)) {
					nTargetMask |= 1u << S.nTarget;
					nEv |= AddHit (PB_SCORE_TARGET, PB_EV_TARGET, PB_BUMPERS + S.nTarget);
					if (nTargetMask == (1u << PB_TARGETS) - 1) {
						dwScore += PB_SCORE_BONUS;
						nTargetMask = 0;
						nEv |= PB_EV_BONUS;
					}
				}
			} else if (CollideSeg (S, 0)) {
				nEv |= PB_EV_WALL;
			}
		}
		/* バンパー（丸い反発。当たると決まった強さ以上で弾く） */
		for (j = 0; j < PB_BUMPERS; j ++) {
			long long dx = nX - T.aBumper[j].x, dy = nY - T.aBumper[j].y;
			long long rr = PB_BALL_R + PB_BUMPER_R;
			if (dx * dx + dy * dy < rr * rr) {
				unsigned int d = PbIsqrt ((unsigned long long)(dx * dx + dy * dy));
				int nNx, nNy;
				unsigned long long v2;
				if (d == 0) { nNx = 0; nNy = -PB_NORM; } else {
					nNx = (int)(dx * PB_NORM / (long long)d);
					nNy = (int)(dy * PB_NORM / (long long)d);
				}
				nX = T.aBumper[j].x + (int)((long long)nNx * rr / PB_NORM);
				nY = T.aBumper[j].y + (int)((long long)nNy * rr / PB_NORM);
				Reflect (nNx, nNy, 256, 0, 0);
				v2 = (unsigned long long)((long long)nVx * nVx + (long long)nVy * nVy);
				if (v2 < (unsigned long long)PB_BUMPER_KICK * PB_BUMPER_KICK) {
					/* 弱い時は、外向きに決まった強さで弾く */
					nVx = nNx * PB_BUMPER_KICK / PB_NORM;
					nVy = nNy * PB_BUMPER_KICK / PB_NORM;
				}
				nEv |= AddHit (PB_SCORE_BUMPER, PB_EV_BUMPER, j);
			}
		}
		/* フリッパー */
		if (CollideFlipper (T, 0, nPrevL)) nHitL = 1;
		if (CollideFlipper (T, 1, nPrevR)) nHitR = 1;
		LimitSpeed ();
	}
	if (nHitL || nHitR) {
		nEv |= PB_EV_FLIPHIT;
	}
	/* ほとんど動かない状態が続いたら救う（レーン以外） */
	{
		long long v2 = (long long)nVx * nVx + (long long)nVy * nVy;
		if ((v2 < 24 * 24) && (nX < T.nLaneL)) {
			nStall ++;
			if (nStall >= PB_STALL_MS * PB_STEP_HZ / 1000) {
				nStall = 0;
				nVy = -320;
				nVx = (dwStep & 1) ? 160 : -160;
				nEv |= PB_EV_STALL;
			}
		} else {
			nStall = 0;
		}
		/* 打ち出しレーンの床で止まった（弱い打ち出しで戻ってきた）：また打ち出せるようにする */
		if ((nX > T.nLaneL) && (v2 < 24 * 24) && (nY >= T.nStartY - 2 * PB_FP)) {
			nStall = 0;
			nSaveSteps = 0;
			if (!bPredict) {
				SetReady ();
			}
		}
	}
	/* 穴に落ちた */
	if (nY >= T.nDrainY) {
		nEv |= PB_EV_DRAIN;
		if (bPredict) {
			nPhase = PB_PHASE_WAIT;
			nVx = nVy = 0;
			nY = T.nDrainY;
		} else if (nSaveSteps > 0) {
			bSaved = 1;
			SetReady ();
			nEv |= PB_EV_SAVE | PB_EV_NEXT;
		} else {
			nBallsLeft --;
			bSaved = 0;
			nCombo = 0;
			nComboSteps = 0;
			if (nBallsLeft <= 0) {
				nBallsLeft = 0;
				nPhase = PB_PHASE_OVER;
				nVx = nVy = 0;
				nEv |= PB_EV_OVER;
			} else {
				SetReady ();
				nEv |= PB_EV_NEXT;
			}
		}
	}
	return nEv;
}

/* ========================================================================= */
/* 台詞（"@i" で始まる）の読み方												 */
/*   @i<コインのアイテム番号>,<必要な枚数>,<dx>,<dy>,<帯の側>,<sdx>,<sdy>			 */
/*   dx, dy … 盤の左上が、台NPCのマスの左上から何マスか（負は左・上）。省略は席が盤の真下	 */
/*   帯の側 … 0=盤の右・1=盤の左									 */
/*   sdx, sdy … 座る場所（台NPCのマスからの相対位置。省略は 0,+1）					 */
/* ========================================================================= */
#define PBP_NONE			(-99999)	/* 省略・空欄 */
#define PBP_COUNT			(7)
#define PBP_SEAT_LIMIT		(60)
#define PBP_TILE_DOT		(32)		/* 地図 1 マス(ドット) */

static inline void PinballParseSpeech(const char *pszSpeech, int anVal[PBP_COUNT])
{
	const char *p = pszSpeech + 2;
	int i;

	for (i = 0; i < PBP_COUNT; i ++) {
		anVal[i] = PBP_NONE;
	}
	for (i = 0; i < PBP_COUNT; i ++) {
		bool bMinus = false, bNum = false;
		int n = 0;

		if (*p == '-') {
			bMinus = true;
			p ++;
		}
		while ((*p >= '0') && (*p <= '9')) {
			if (n < 100000) {
				n = n * 10 + (*p - '0');
			}
			bNum = true;
			p ++;
		}
		if (bNum) {
			anVal[i] = bMinus ? -n : n;
		}
		if (*p != ',') {
			break;
		}
		p ++;
	}
}

/* 座る場所（台NPCのマスからの相対位置） */
static inline void PinballSeatOffset(const char *pszSpeech, int *pnSdx, int *pnSdy)
{
	int anVal[PBP_COUNT];
	int a, b;

	PinballParseSpeech (pszSpeech, anVal);
	a = (anVal[5] == PBP_NONE) ? 0 : anVal[5];
	b = (anVal[6] == PBP_NONE) ? 1 : anVal[6];
	*pnSdx = (a < -PBP_SEAT_LIMIT) ? -PBP_SEAT_LIMIT : (a > PBP_SEAT_LIMIT) ? PBP_SEAT_LIMIT : a;
	*pnSdy = (b < -PBP_SEAT_LIMIT) ? -PBP_SEAT_LIMIT : (b > PBP_SEAT_LIMIT) ? PBP_SEAT_LIMIT : b;
}

/* 盤の左上の既定（席が盤の真下の中央）。盤の中心の x が席の x になるよう、横は盤の幅の半分 */
static inline int PinballDefaultDx(void)
{
	return -((PB_W / 2 - PBP_TILE_DOT / 2) / PBP_TILE_DOT);
}
static inline int PinballDefaultDy(void)
{
	return -((PB_VIEW_H + PBP_TILE_DOT - 1) / PBP_TILE_DOT - 1);
}
/* 盤の左上のマップのドット座標 */
static inline void PinballFieldPos(int nTileX, int nTileY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nTileX + nDx) * PBP_TILE_DOT;
	*pnY = (nTileY + nDy) * PBP_TILE_DOT;
}
