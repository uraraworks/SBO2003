/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			ウィンドウ表示の外部参照用定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void GetSystemGrpRect	(BYTE, PRECT);									/* システムグラフィックの座標を取得 */
extern void DrawWindow			(CImg32 *pDib = NULL, int x = 0, int y = 0);	/* ウィンドウの描画 */
extern void EraseWindow			(PWNDINFO);										/* ウィンドウの消去 */
extern void AllEraseWindow		(void);											/* 全てのウィンドウ消去 */
extern COLORREF HpWarnTextColor	(COLORREF cl);									/* HPが減った時のウィンドウ文字色に変える */

/* Copyright(C)URARA-works 2003-2005 */
