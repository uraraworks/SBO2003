/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		会話データダイアログ処理定義ファイル						 */
/* ファイル名：	AdminDlgTalk.h												 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgTalk(HWND hWndParent);								/* ダイアログ表示(会話データの一覧) */
extern void OpenTalkEditFromNpc(HWND hWndParent, int nTalkNo);			/* キャラ情報から、NPC の台詞 "@t番号" の会話セットの編集画面を開く（一覧を取り直して届いてから開く） */
extern void CloseDlgTalk(void);											/* ダイアログ終了(会話データの一覧) */
/* Copyright(C)URARA-works 2026 */
