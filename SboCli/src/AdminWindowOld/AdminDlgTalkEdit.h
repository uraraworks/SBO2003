/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		会話セット編集ダイアログ処理定義ファイル					 */
/* ファイル名：	AdminDlgTalkEdit.h											 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgTalkEdit(HWND hWndParent, int nNo);					/* ダイアログ表示(-1 なら追加) */
extern void ShowDlgTalkEditByNo(HWND hWndParent, int nTalkNo);			/* 会話セット番号(@t の数字)で開く（無ければその番号で新規作成。届いている一覧から探す） */
extern void CloseDlgTalkEdit(void);										/* ダイアログ終了 */
extern BOOL IsDlgTalkEditOpen(void);									/* 開いているか */
/* Copyright(C)URARA-works 2026 */
