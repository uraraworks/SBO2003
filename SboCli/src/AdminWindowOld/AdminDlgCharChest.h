/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		キャラの開けた宝箱ダイアログ処理定義ファイル				 */
/* ファイル名：	AdminDlgCharChest.h											 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgCharChest(HWND hWndParent, DWORD dwTblID, LPCSTR pszCharName);	/* ダイアログ表示(そのキャラの開けた宝箱) */
extern void CloseDlgCharChest(void);												/* ダイアログ終了 */
