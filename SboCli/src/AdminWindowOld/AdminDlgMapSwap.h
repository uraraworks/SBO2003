/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		印で変わるマス・宝箱ダイアログ処理定義ファイル				 */
/* ファイル名：	AdminDlgMapSwap.h											 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgMapSwap(HWND hWndParent);							/* ダイアログ表示(印で変わるマス・宝箱) */
extern void CloseDlgMapSwap(void);										/* ダイアログ終了 */
extern BOOL IsDlgMapSwapOpen(void);										/* 開いているか */
extern void MapSwapDlgOnRecv(const void *pPara);						/* サーバーからの通知(CMDPARA_ADMINSWAP) */
extern void MapSwapDlgOnPos(int x, int y);								/* ゲーム画面のクリック(X・Y に入れる) */
extern void MapSwapDlgReload(void);										/* 表を取り直す（元に戻した後など） */
extern void MapSwapDlgTick(void);										/* 自分のマップが変わったら取り直す（描画のたびに呼ぶ） */
extern int  MapSwapDlgGetCount(void);									/* 届いている表の行数 */
extern const ADMINSWAPROW *MapSwapDlgGetRow(int nIndex);				/* 表の 1 行（範囲外は NULL） */
extern int  MapSwapDlgGetSel(void);										/* 選んでいる行（無ければ -1） */
