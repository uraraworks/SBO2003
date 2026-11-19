/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	SboCliAutoMap.h												 */
/* 内容：		探索マップ（オートマップ）									 */
/* ========================================================================= */

#pragma once

extern void	RecvDataProcDUNGEONMAP	(PBYTE, DWORD);					/* 受信データ処理(探索マップ用の情報) */
extern void	AutoMapUpdate			(void);							/* 自分の位置・見えたマスを記録する（キャラ座標情報の更新のたび） */
extern void	AutoMapReset			(void);							/* 全部消す（ログイン画面へ戻る時など） */
extern void	AutoMapOnMapInfo		(DWORD);						/* マップ情報を受け取った（別のマップなら消す） */
extern BOOL	AutoMapIsAvailable		(void);							/* 今、探索マップが使える場所（ダンジョンの範囲の中）か */
extern BOOL	AutoMapToggleKey		(void);							/* Tab・M キー：使える場所なら出し入れして TRUE */
extern BOOL	AutoMapIsShown			(void);							/* 地図を出しているか */
extern void	AutoMapDraw				(int, int);						/* 地図を描く（画面の左上のドット位置） */
