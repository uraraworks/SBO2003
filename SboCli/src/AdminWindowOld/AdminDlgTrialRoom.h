/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		お試し個室ダイアログ処理定義ファイル						 */
/* ファイル名：	AdminDlgTrialRoom.h											 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
#pragma once

extern void ShowDlgTrialRoom(HWND hWndParent);							/* ダイアログ表示(お試し個室) */
extern void CloseDlgTrialRoom(void);									/* ダイアログ終了(お試し個室) */
extern void AdminTrialRoomOnRecv(const void *pPara);					/* サーバーから届いた（CMDPARA_ADMINTRIALROOM） */
extern int AdminTrialRoomGetCount(void);								/* 届いているお試し個室の数 */
extern const TRIALROOMINFO *AdminTrialRoomGet(int nNo);					/* 届いているお試し個室（無ければ NULL） */
