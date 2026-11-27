/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		お試し個室編集ダイアログ処理定義ファイル					 */
/* ファイル名：	AdminDlgTrialRoomEdit.h										 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgTrialRoomEdit(HWND hWndParent, int nNo);				/* ダイアログ表示(-1 なら追加) */
extern void CloseDlgTrialRoomEdit(void);								/* ダイアログ終了 */
extern BOOL IsDlgTrialRoomEditOpen(void);								/* 開いているか */
extern void TrialRoomEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg);	/* 書き込みの結果が届いた */
