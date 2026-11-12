/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ギルドの依頼編集ダイアログ処理定義ファイル					 */
/* ファイル名：	AdminDlgQuestInfoEdit.h										 */
/* 作成日：		2026/11/12													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgQuestInfoEdit(HWND hWndParent, int nNo);				/* ダイアログ表示(-1 なら追加) */
extern void CloseDlgQuestInfoEdit(void);								/* ダイアログ終了 */
extern BOOL IsDlgQuestInfoEditOpen(void);								/* 開いているか */
extern void QuestEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg);		/* 書き込みの結果が届いた */
extern BOOL QuestEditOnPos(int x, int y, BOOL bRight);					/* ゲーム画面がクリックされた(使ったら TRUE) */
/* Copyright(C)URARA-works 2026 */
