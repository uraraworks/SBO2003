/* ========================================================================= */
/* ファイル名：	PinballInfo.h												 */
/* 概要：		ピンボールのサブゲーム情報（SUBGAME_PINBALLINFO / INFOSAVE）の既定値・範囲の丸め */
/*				サーバー・管理者ウィンドウ・データ API で共通。ゲームの中身は common/Pinball.h	 */
/* 日付：		2026/11/27													 */
/* ========================================================================= */
/*
 * INFO と INFOSAVE は同じ並びの別の構造体なので、どちらにも使えるようテンプレートにしてある。
 * 既定値は、サブゲーム情報にする前のピンボールの定数と同じ（球 3・ボールセーブ 8 秒・100/300/1000 点・コンボ 2 秒・盤 -2,-6・席 0,+1・帯は右・無料）。
 */

#pragma once

#include "Pinball.h"

#define PBI_COIN_MAX		(99)		/* コインの必要枚数の上限 */
#define PBI_SAVE_SEC_MAX	(PB_CFG_MS_MAX / 1000)

static inline int PbiClampInt(int n, int nMin, int nMax)
{
	return (n < nMin) ? nMin : (n > nMax) ? nMax : n;
}

/* 既定値に戻す（新しく作る時・管理者ウィンドウで種別をピンボールにした時） */
template<class T> static inline void PinballInfoSetDefault(T *p)
{
	memset (p, 0, sizeof (*p));
	p->wCoinItem		= 0;
	p->byCoinCount		= 0;
	p->byBalls			= (unsigned char)PB_BALLS;
	p->bySaveSec		= (unsigned char)(PB_SAVE_MS / 1000);
	p->byBandSide		= 0;
	p->wScoreBumper		= (unsigned short)PB_SCORE_BUMPER;
	p->wScoreTarget		= (unsigned short)PB_SCORE_TARGET;
	p->wScoreBonus		= (unsigned short)PB_SCORE_BONUS;
	p->wComboMs			= (unsigned short)PB_COMBO_MS;
	p->cDx				= (signed char)PinballDefaultDx ();
	p->cDy				= (signed char)PinballDefaultDy ();
	p->cSeatDx			= 0;
	p->cSeatDy			= 1;
	p->wVsSec			= (unsigned short)PB_VS_SEC_DEFAULT;
	p->byPair			= 0;
	p->byVersion		= PINBALLINFO_VERSION;
}

/* 範囲に丸める（保存データ・管理者ウィンドウの入力が範囲外でも、遊べる値にする） */
template<class T> static inline void PinballInfoClamp(T *p)
{
	p->byCoinCount	= (unsigned char)PbiClampInt (p->byCoinCount, 0, PBI_COIN_MAX);
	p->byBalls		= (unsigned char)PbiClampInt (p->byBalls, 1, PB_BALLS_MAX);
	p->bySaveSec	= (unsigned char)PbiClampInt (p->bySaveSec, 0, PBI_SAVE_SEC_MAX);
	p->byBandSide	= (unsigned char)((p->byBandSide == 1) ? 1 : 0);
	p->wScoreBumper	= (unsigned short)PbiClampInt (p->wScoreBumper, 0, PB_CFG_SCORE_MAX);
	p->wScoreTarget	= (unsigned short)PbiClampInt (p->wScoreTarget, 0, PB_CFG_SCORE_MAX);
	p->wScoreBonus	= (unsigned short)PbiClampInt (p->wScoreBonus, 0, PB_CFG_SCORE_MAX);
	p->wComboMs		= (unsigned short)PbiClampInt (p->wComboMs, 0, PB_CFG_MS_MAX);
	p->cDx			= (signed char)PbiClampInt (p->cDx, -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	p->cDy			= (signed char)PbiClampInt (p->cDy, -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	p->cSeatDx		= (signed char)PbiClampInt (p->cSeatDx, -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	p->cSeatDy		= (signed char)PbiClampInt (p->cSeatDy, -PBP_SEAT_LIMIT, PBP_SEAT_LIMIT);
	/* 対戦の時間：0（古い保存データ・未設定）は既定の秒数。それ以外は範囲に丸める */
	p->wVsSec		= (unsigned short)((p->wVsSec == 0) ? PB_VS_SEC_DEFAULT : PbiClampInt (p->wVsSec, PB_VS_SEC_MIN, PB_VS_SEC_MAX));
}

/* ゲームの設定（球数・ボールセーブ・コンボ・得点）にする */
template<class T> static inline void PinballInfoToConfig(const T *p, PinballConfig *pCfg)
{
	pCfg->nBalls		= p->byBalls;
	pCfg->nSaveMs		= (int)p->bySaveSec * 1000;
	pCfg->nComboMs		= p->wComboMs;
	pCfg->nScoreBumper	= p->wScoreBumper;
	pCfg->nScoreTarget	= p->wScoreTarget;
	pCfg->nScoreBonus	= p->wScoreBonus;
	pCfg->Clamp ();
}
