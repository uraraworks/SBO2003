/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		思い出の本ダイアログ処理定義ファイル						 */
/* ファイル名：	AdminDlgStoryInfo.h											 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgStoryInfo(HWND hWndParent);							/* ダイアログ表示(思い出の本) */
extern void CloseDlgStoryInfo(void);									/* ダイアログ終了(思い出の本) */
extern void AdminStoryOnRecv(const void *pPara);						/* サーバーから届いた（CMDPARA_ADMINSTORY） */
extern int AdminStoryGetCount(void);									/* 届いているシナリオの数 */
extern const STORYINFO *AdminStoryGet(int nNo);							/* 届いているシナリオ（無ければ NULL） */
/* Copyright(C)URARA-works 2026 */
