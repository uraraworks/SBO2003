/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		思い出の本編集ダイアログ処理定義ファイル					 */
/* ファイル名：	AdminDlgStoryInfoEdit.h										 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgStoryInfoEdit(HWND hWndParent, int nNo);				/* ダイアログ表示(-1 なら追加) */
extern void CloseDlgStoryInfoEdit(void);								/* ダイアログ終了 */
extern BOOL IsDlgStoryInfoEditOpen(void);								/* 開いているか */
extern void StoryEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg);		/* 書き込みの結果が届いた */
/* Copyright(C)URARA-works 2026 */
