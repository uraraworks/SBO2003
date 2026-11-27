/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrTrialRoom.h											 */
/* 概要：		お試し個室(定義ファイル)									 */
/*				試し中の新機能を、部屋専用のステータステンプレートの姿で遊べる部屋 */
/*				（貸し出しの部品 SboSvrLoan を使う。討伐チャレンジ・番外編とは独立） */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数定義																	 */
/* ========================================================================= */

BOOL		TrialRoomIsNpc(PCHARINFO pNpc);												/* 入口のNPC（台詞が "@x" と数字）か */
void		TrialRoomTalk(PCHARINFO pChar, PCHARINFO pNpc);								/* 入口に話しかけた：説明を出して「入りますか？」を聞く */
BOOL		TrialRoomAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes);				/* 「入りますか？」への答え。この問いへの答えなら TRUE */
void		TrialRoomProc(void);														/* 周期処理：個室の範囲の外に出た人を元の姿・元の場所へ戻す */
BOOL		TrialRoomEventExit(PCHARINFO pChar);										/* 個室の中でワープのイベントに乗った：行き先は使わず、元の姿・元の場所へ直接戻して TRUE */
BOOL		TrialRoomIs(PCHARINFO pChar);												/* お試し個室の中（借りている最中）か */
BOOL		TrialRoomResurrection(PCHARINFO pChar);										/* 倒れて「復活」を選んだ時。個室の中なら出して TRUE（お金は減らさない） */
BOOL		TrialRoomOnDecPlayer(PCHARINFO pChar);										/* キャラが消える・切断した時。個室の中なら元の姿に戻して TRUE（戻り位置は LoanPendingSet） */
const char	*TrialRoomBlockedText(PCHARINFO pChar);										/* 貸し出し中の制限（店・預かり所・取り引きなど）で出す文 */
