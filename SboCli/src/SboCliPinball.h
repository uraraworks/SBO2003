/* ========================================================================= */
/* ファイル名：	SboCliPinball.h												 */
/* 内容：		ピンボール（台に座って遊ぶ）のクライアント側（受信・先読み・補間・キー）	 */
/* ========================================================================= */
#pragma once

#include "common/Pinball.h"

#define PINBALL_VIEW_MAX	(8)			/* 同時に覚える台の数 */
#define PINBALL_FLASH_MS	(180)		/* バンパー・ターゲットが光る長さ(ms) */

/* 描くための情報（GetPinballView が返す） */
typedef struct _PINBALLDRAW {
	CMDPARA_PINBALLSTATE	St;							/* 最後に届いた状態（byMine・byIdle・byOver・得点・残り球・コンボ・ターゲットなど） */
	int			nBallX, nBallY;						/* 描くボールの中心（1/256 ドット） */
	BOOL		bBallShow;							/* ボールを描くか（待機中・穴に落ちて返事待ち・終了では描かない） */
	int			nAngL, nAngR;						/* 描くフリッパーの段階 */
	unsigned int nTargetMask;						/* 倒れたターゲット（本人は先読みの分も含む） */
	int			nCharge;							/* 溜めている打ち出しの強さ（0〜100 ％。溜めていなければ 0） */
	DWORD		dwFlashAge[PB_BUMPERS + PB_TARGETS];	/* 当たってからの経過(ms)（PINBALL_FLASH_MS を超えたら光らない） */
	BOOL		bOver;								/* 「GAME OVER」を出す */
	int			nCamY;								/* 窓（高さ PB_VIEW_H）の上端が、盤の上端から何ドット下か（0〜PB_YOFS。待機中・打ち出し前は下端 PB_YOFS）。クライアントだけで決める */
} PINBALLDRAW, *PPINBALLDRAW;

/* 盤の差し替え絵（SboGrpData/grp/pinball_bg.png・pinball_parts.png。docs/pinball-art.md）。
   両方読めて大きさが合っている時だけ TRUE（それ以外は図形で描く）。初めて呼んだ時に読み込む */
extern BOOL GetPinballArt(CImg32 **ppBg, CImg32 **ppParts);

/* 部品の絵の位置（pinball_parts.png。16 ドット格子。左上 x,y と大きさ w,h）。当たり判定の中心が、各セルの (中心 x,中心 y) に重なる */
#define PBART_PARTS_W		(128)
#define PBART_PARTS_H		(96)
#define PBART_BALL_X		(0)		/* ボール 16×16。中心 (8,8) */
#define PBART_BALL_Y		(0)
#define PBART_BALL_S		(16)
#define PBART_TARGET_Y		(0)		/* ターゲット 32×16 を 3 つ：立っている(16,0)・倒れた(48,0)・当たって光る(80,0)。中心 (16,8) */
#define PBART_TARGET_UP_X	(16)
#define PBART_TARGET_DOWN_X	(48)
#define PBART_TARGET_HIT_X	(80)
#define PBART_TARGET_W		(32)
#define PBART_TARGET_H		(16)
#define PBART_BUMPER_Y		(16)	/* バンパー 32×32 を 2 つ：ふつう(0,16)・光る(32,16)。中心 (16,16) */
#define PBART_BUMPER_X		(0)
#define PBART_BUMPER_HIT_X	(32)
#define PBART_BUMPER_S		(32)
#define PBART_FLIP_Y		(48)	/* 左フリッパー 48×48 を 2 つ：下がり(0,48)・上がり(48,48)。軸 (12,24)。右は左右反転して使う */
#define PBART_FLIP_REST_X	(0)
#define PBART_FLIP_UP_X		(48)
#define PBART_FLIP_S		(48)
#define PBART_FLIP_PX		(12)
#define PBART_FLIP_PY		(24)

extern void RecvDataProcPINBALLSTATE(PBYTE pData, DWORD dwDataSize);	/* 受信データ処理(ピンボールの状態) */
extern BOOL IsPinballSeated(void);								/* ピンボールの席に座って遊んでいるか（ゲームオーバー直後の少しの間も TRUE） */
extern void PinballKeyPoll(const BYTE *abyKeyState);			/* 座っている間のキー処理（KeyProc から毎回。押した時・離した時だけ送る） */
extern BOOL GetPinballView(int nIdx, PPINBALLDRAW pOut);		/* 重ね描きの状態（nIdx は 0〜PINBALL_VIEW_MAX-1。見えている台だけ TRUE） */
