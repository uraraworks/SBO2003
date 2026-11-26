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
} PINBALLDRAW, *PPINBALLDRAW;

extern void RecvDataProcPINBALLSTATE(PBYTE pData, DWORD dwDataSize);	/* 受信データ処理(ピンボールの状態) */
extern BOOL IsPinballSeated(void);								/* ピンボールの席に座って遊んでいるか（ゲームオーバー直後の少しの間も TRUE） */
extern void PinballKeyPoll(const BYTE *abyKeyState);			/* 座っている間のキー処理（KeyProc から毎回。押した時・離した時だけ送る） */
extern BOOL GetPinballView(int nIdx, PPINBALLDRAW pOut);		/* 重ね描きの状態（nIdx は 0〜PINBALL_VIEW_MAX-1。見えている台だけ TRUE） */
