/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ギルドの依頼ダイアログ処理定義ファイル						 */
/* ファイル名：	AdminDlgQuestInfo.h											 */
/* 作成日：		2026/11/12													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgQuestInfo(HWND hWndParent);							/* ダイアログ表示(ギルドの依頼) */
extern void CloseDlgQuestInfo(void);									/* ダイアログ終了(ギルドの依頼) */
extern void AdminQuestOnRecv(const void *pPara);						/* サーバーから届いた（CMDPARA_ADMINQUEST） */
extern int AdminQuestGetCount(void);									/* 届いている依頼の数 */
extern const QUESTINFO *AdminQuestGet(int nNo);							/* 届いている依頼（無ければ NULL） */
extern BOOL AdminQuestOnPos(int x, int y, BOOL bRight);					/* ゲーム画面がクリックされた(使ったら TRUE) */
extern LPCSTR AdminQuestChallengeName(int nType, char *pszBuf, int nBufSize);	/* 討伐チャレンジの種類の名前 */
/* Copyright(C)URARA-works 2026 */
