/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrLoan.h												 */
/* 概要：		キャラ貸し出しの部品(定義ファイル)							 */
/*				本物の状態を控えに写し、ステータステンプレートの姿にして貸し出す */
/*				（依頼・ダンジョンとは独立。呼び出し側が挑戦の欄を設定する）	 */
/* ========================================================================= */

#pragma once

#include <map>

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* 貸し出し中の人の、本物の状態の控え */
typedef struct _LOANBAK {
	DWORD		dwSession;											/* 挑戦の番号（挑戦以外から借りる時は 0 のまま） */
	BOOL		bArrived;											/* 行き先のマップに着いたか */
	BOOL		bWarped;											/* 着いた後にマップの中のワープで動いた（部屋を出た） */
	BYTE		byLevel, byPW, byDEX, byST, byIT, byLK;
	BYTE		abyMagic[MAX_USEMAGIC];
	WORD		waItem[MAX_MYITEM];
	WORD		waEqu[MAX_EQUTYPE];
	BYTE		abyMagicKey[MAX_MAGICKEY];							/* 特技ショートカット（中で登録し直しても、出たら元に戻す） */
	WORD		awKeyItem[MAX_MAGICKEY];							/* ショートカットの装備品・道具 */
	WORD		wMaxHP, wHP, wMaxMana, wMana;
	DWORD		dwGold, dwExp;
	char		chAttrFire, chAttrWater, chAttrWind, chAttrEarth, chAttrLight, chAttrDark;
	DWORD		dwMapID, dwX, dwY;									/* 入る前にいた場所（戻る場所・保存する場所） */
	BYTE		byDirection;
	BOOL		bTrial;												/* お試し個室で借りている（SboSvrTrialRoom。討伐チャレンジ・番外編は FALSE） */
	WORD		wTrialRoom;											/* お試し個室の番号 */
	WORD		wDgTmpl;											/* 番外編ダンジョン：入った時のステータステンプレートの番号（中で増えた物を数えるのに使う） */
} LOANBAK;

/* ========================================================================= */
/* 関数定義																	 */
/* ========================================================================= */

BOOL		LoanIs(PCHARINFO p);														/* 貸し出し中か */
LOANBAK		*LoanFind(DWORD dwTblID);													/* 控えを探す（無ければ NULL） */
LOANBAK		*LoanBegin(PCHARINFO p, PSTATUSTEMPLATEINFO t, DWORD dwMapID, DWORD dwX, DWORD dwY, BYTE byDirection);	/* 借りる（控えに写し、テンプレートの姿にして知らせる。戻り先を控えに持つ） */
LOANBAK		LoanEnd(PCHARINFO p);														/* 返す（控えから戻して控えを消す。控えのコピーを返す。LoanSendAll は呼び出し側） */
void		LoanSendAll(PCHARINFO p);													/* 状態を変えた後：タイマー効果を消し、攻撃力・守備力を計算し直して知らせる */
void		LoanSaveSwap(BOOL bBegin);													/* 保存の前後で本物と入れ替える。切断した人の戻り位置もここで書く */
void		LoanPendingSet(DWORD dwTblID, DWORD dwMapID, DWORD dwX, DWORD dwY, BYTE byDirection);	/* 切断した人の戻り位置を登録 */
void		LoanPendingTake(PCHARINFO p);												/* ログイン時：登録があれば位置を戻して消す */
