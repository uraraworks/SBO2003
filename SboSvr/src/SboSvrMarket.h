/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrMarket.h												 */
/* 概要：		取引所（委託販売）の処理(定義ファイル)						 */
/* 作成日：		2026/11/16													 */
/* ========================================================================= */

#pragma once

#include <vector>

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* 出品 1 件（品 1 個）。売れた出品はその場で消す */
typedef struct _MARKETITEM {
	DWORD		dwID;												/* 出品ID（1 から。消しても詰めない） */
	char		szAcc[MAX_ACCNAME];									/* 出品者のアカウント名 */
	DWORD		dwCharID;											/* 出品したキャラのテーブルID（参考） */
	char		szSeller[MAX_CHARNAMEBUF];							/* 出品者のキャラ名 */
	WORD		wItemID;											/* 品の種類ID */
	DWORD		dwPrice;											/* 値段 */
	DWORD		dwTime;												/* 出品した時刻（time()） */
	BYTE		byState;											/* 状態(MARKETSTATE_*) */
} MARKETITEM, *PMARKETITEM;

/* 売上（アカウントごと） */
typedef struct _MARKETSALE {
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	DWORD		dwGold;												/* 貯まっている金額 */
} MARKETSALE, *PMARKETSALE;

/* ========================================================================= */
/* 変数・関数プロトタイプ宣言												 */
/* ========================================================================= */
extern std::vector<MARKETITEM> g_aMarketItem;					/* 出品の一覧 */
extern std::vector<MARKETSALE> g_aMarketSale;					/* 売上の一覧 */
extern DWORD g_dwMarketNextID;									/* 次の出品ID */

extern void MarketClear(void);									/* 全部消す（終了時・読み込み前） */
extern void MarketFixNextID(void);								/* 読み込み後に次の出品IDを決める */
extern DWORD MarketGetSale(LPCSTR pszAcc);						/* アカウントの売上 */
extern BOOL MarketIsNearReception(PCHARINFO pChar);				/* 受付（台詞 "@b" のNPC）の近くにいるか */
extern void MarketExpire(DWORD dwNow);							/* 期限を過ぎた出品を期限切れにする */

extern void MarketList(PCHARINFO pChar, BYTE byType);			/* 一覧を送る */
extern void MarketPut(PCHARINFO pChar, BYTE byNo, DWORD dwPrice);	/* 出品する */
extern void MarketBuy(PCHARINFO pChar, DWORD dwID, WORD wItemID, DWORD dwPrice);	/* 買う */
extern void MarketTakeBack(PCHARINFO pChar, DWORD dwID);		/* 自分の出品を引き取る */
extern void MarketGoldGet(PCHARINFO pChar);						/* 売上を所持金へ受け取る */

/* Copyright(C)URARA-works 2003-2005 */
