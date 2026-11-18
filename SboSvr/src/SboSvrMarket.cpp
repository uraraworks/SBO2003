/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrMarket.cpp											 */
/* 概要：		取引所（委託販売）の処理(実装ファイル)						 */
/* 作成日：		2026/11/16													 */
/* ========================================================================= */
/*
 * 仕様は docs/market.md。受付は台詞が "@b" のNPC。クライアントが話しかけると画面が開き、
 * 一覧・出品・買う・引き取り・売上の受け取りをサーバーへ頼む（CMD_MARKET*）。
 * 頼みごとは、そのたびにサーバーが「受付の近くにいるか」「手持ち・所持金・件数が合うか」を
 * 確かめる。結果は CMDPARA_MARKET で返し、続けて手持ち（CMD_CHARITEM）と所持金を本人へ送る。
 * 出品者は同じアカウントかどうかで見分ける（キャラ名ではなくアカウント名）。
 * 保存は SaveLoad/SboSvrSaveLoadMarket.cpp（SBODATA/SboDataMarket.dat）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrMarket.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
std::vector<MARKETITEM> g_aMarketItem;							/* 出品の一覧 */
std::vector<MARKETSALE> g_aMarketSale;							/* 売上の一覧 */
DWORD g_dwMarketNextID = 1;										/* 次の出品ID */

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* アカウント名が同じか（欄の長さまで比べる） */
static BOOL SameAcc(LPCSTR pszA, LPCSTR pszB)
{
	return (pszA[0] != 0) && (strncmp (pszA, pszB, MAX_ACCNAME) == 0);
}

/* アカウント名を欄にコピー（終端を必ず付ける） */
static void CopyAcc(char *pszDst, LPCSTR pszSrc)
{
	ZeroMemory (pszDst, MAX_ACCNAME);
	strncpy (pszDst, pszSrc, MAX_ACCNAME - 1);
}

/* 現在時刻（秒） */
static DWORD NowSec(void)
{
	return (DWORD)time (NULL);
}

/* 手数料（値段の 5%。切り捨て） */
static DWORD CalcFee(DWORD dwPrice)
{
	return (DWORD)((unsigned long long)dwPrice * MARKET_FEE_PERCENT / 100);
}

/* 値段の下限（品の元の売値。店と同じ「価値の 1/4・最低 1」） */
static DWORD CalcPriceMin(WORD wItemID)
{
	return max (ItemGoldOf (wItemID) / 4, (DWORD)1);
}

/* 結果を送る（要求の欄はそのまま返す） */
static void SendResult(PCHARINFO pChar, BYTE byCmd, BYTE byResult, DWORD dwID, WORD wItemID, DWORD dwPrice, DWORD dwGold)
{
	CMDPARA_MARKET Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= byCmd;
	Para.byResult	= byResult;
	Para.wItemID	= wItemID;
	Para.dwTblID	= pChar->dwTblID;
	Para.dwID		= dwID;
	Para.dwPrice	= dwPrice;
	Para.dwGold		= dwGold;
	svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* 持ち物・所持金の変化を本人に送る */
static void SendBagState(PCHARINFO pChar)
{
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
	SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
}

/* 出品を探す（無ければ -1） */
static int FindItem(DWORD dwID)
{
	size_t i;

	for (i = 0; i < g_aMarketItem.size (); i ++) {
		if (g_aMarketItem[i].dwID == dwID) {
			return (int)i;
		}
	}
	return -1;
}

/* アカウントの出品数（期限切れを含む） */
static int CountItem(LPCSTR pszAcc)
{
	size_t i;
	int nCount = 0;

	for (i = 0; i < g_aMarketItem.size (); i ++) {
		if (SameAcc (g_aMarketItem[i].szAcc, pszAcc)) {
			nCount ++;
		}
	}
	return nCount;
}

/* 売上の欄を探す（無ければ bAdd のとき作る。作らないなら -1） */
static int FindSale(LPCSTR pszAcc, BOOL bAdd)
{
	size_t i;
	MARKETSALE Sale;

	for (i = 0; i < g_aMarketSale.size (); i ++) {
		if (SameAcc (g_aMarketSale[i].szAcc, pszAcc)) {
			return (int)i;
		}
	}
	if (!bAdd) {
		return -1;
	}
	ZeroMemory (&Sale, sizeof (Sale));
	CopyAcc (Sale.szAcc, pszAcc);
	g_aMarketSale.push_back (Sale);
	return (int)g_aMarketSale.size () - 1;
}

/* 残り秒 */
static DWORD RemainSec(const MARKETITEM &Item, DWORD dwNow)
{
	DWORD dwLimit = Item.dwTime + MARKET_LIMITSEC;

	if (Item.byState != MARKETSTATE_ON) {
		return 0;
	}
	if (dwNow < Item.dwTime) {
		return MARKET_LIMITSEC;					/* 時計が戻っていても期限より長くはしない */
	}
	return (dwNow >= dwLimit) ? 0 : (dwLimit - dwNow);
}

/* ========================================================================= */
/* 関数名：MarketClear														 */
/* 内容：全部消す															 */
/* ========================================================================= */
void MarketClear(void)
{
	g_aMarketItem.clear ();
	g_aMarketSale.clear ();
	g_dwMarketNextID = 1;
}

/* ========================================================================= */
/* 関数名：MarketFixNextID													 */
/* 内容：読み込み後に次の出品IDを決める（一番大きい ID の次）				 */
/* ========================================================================= */
void MarketFixNextID(void)
{
	size_t i;

	g_dwMarketNextID = 1;
	for (i = 0; i < g_aMarketItem.size (); i ++) {
		if (g_aMarketItem[i].dwID >= g_dwMarketNextID) {
			g_dwMarketNextID = g_aMarketItem[i].dwID + 1;
		}
	}
}

/* ========================================================================= */
/* 関数名：MarketGetSale													 */
/* 内容：アカウントの売上													 */
/* ========================================================================= */
DWORD MarketGetSale(LPCSTR pszAcc)
{
	int nNo = FindSale (pszAcc, FALSE);

	return (nNo < 0) ? 0 : g_aMarketSale[nNo].dwGold;
}

/* ========================================================================= */
/* 関数名：MarketIsNearReception											 */
/* 内容：受付（台詞が "@b" のNPC）の近く（同じマップ・同じ空間で MARKET_RANGE マス以内）にいるか */
/* ========================================================================= */
BOOL MarketIsNearReception(PCHARINFO pChar)
{
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pNpc = g_aCharInfo[i];
		if ((pNpc == NULL) || (pNpc->szAcc[0] != 0) || (pNpc->byNpcType == NPCTYPE_INIT) ||
				(pNpc->pszNpcSpeech == NULL)) {
			continue;
		}
		if ((pNpc->pszNpcSpeech[0] != '@') || (pNpc->pszNpcSpeech[1] != 'b')) {
			continue;
		}
		if ((pNpc->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pNpc)) {
			continue;
		}
		if ((abs ((int)pChar->dwX - (int)pNpc->dwX) <= MARKET_RANGE) &&
				(abs ((int)pChar->dwY - (int)pNpc->dwY) <= MARKET_RANGE)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：MarketExpire														 */
/* 内容：期限を過ぎた出品を期限切れにする（一覧を作る時・買う時に呼ぶ）		 */
/* ========================================================================= */
void MarketExpire(DWORD dwNow)
{
	size_t i;

	for (i = 0; i < g_aMarketItem.size (); i ++) {
		MARKETITEM &Item = g_aMarketItem[i];
		if ((Item.byState == MARKETSTATE_ON) && (dwNow >= Item.dwTime) && (dwNow - Item.dwTime >= MARKET_LIMITSEC)) {
			Item.byState = MARKETSTATE_EXPIRED;
		}
	}
}

/* ========================================================================= */
/* 関数名：MarketList														 */
/* 内容：一覧を送る（新しい順に MARKET_LISTMAX 件まで）						 */
/* ========================================================================= */
void MarketList(PCHARINFO pChar, BYTE byType)
{
	PCMDPARA_MARKETLIST pSend;
	DWORD dwNow = NowSec ();
	int i, nCount = 0;
	DWORD dwSize;

	if (!MarketIsNearReception (pChar) || (pChar->szAcc[0] == 0)) {
		return;
	}
	if ((byType != MARKETLIST_BUY) && (byType != MARKETLIST_MINE)) {
		return;
	}
	MarketExpire (dwNow);

	pSend = new CMDPARA_MARKETLIST;
	ZeroMemory (pSend, sizeof (CMDPARA_MARKETLIST));
	pSend->byCmd	= CMD_MARKETLIST;
	pSend->byType	= byType;
	pSend->dwTblID	= pChar->dwTblID;
	pSend->dwSale	= MarketGetSale (pChar->szAcc);

	/* 出品IDが大きい（新しい）ほうから */
	for (i = (int)g_aMarketItem.size () - 1; (i >= 0) && (nCount < MARKET_LISTMAX); i --) {
		const MARKETITEM &Item = g_aMarketItem[i];
		BOOL bMine = SameAcc (Item.szAcc, pChar->szAcc);
		PMARKETENTRY pEntry;

		if (byType == MARKETLIST_BUY) {
			if (bMine || (Item.byState != MARKETSTATE_ON)) {
				continue;
			}
		} else if (!bMine) {
			continue;
		}
		pEntry = &pSend->aEntry[nCount];
		pEntry->dwID		= Item.dwID;
		pEntry->wItemID		= Item.wItemID;
		pEntry->byState		= Item.byState;
		pEntry->dwPrice		= Item.dwPrice;
		pEntry->dwRemain	= RemainSec (Item, dwNow);
		strncpy (pEntry->szSeller, Item.szSeller, MAX_CHARNAMEBUF - 1);
		nCount ++;
	}
	pSend->byCount = (BYTE)nCount;

	dwSize = (DWORD)(offsetof (CMDPARA_MARKETLIST, aEntry) + sizeof (MARKETENTRY) * nCount);
	svrSock.SendTo (pChar->pID, (LPBYTE)pSend, dwSize, SENDTIMEOUT);
	delete pSend;
}

/* ========================================================================= */
/* 関数名：MarketPut														 */
/* 内容：出品する（手持ちの byNo の品を値段 dwPrice で）						 */
/* ========================================================================= */
void MarketPut(PCHARINFO pChar, BYTE byNo, DWORD dwPrice)
{
	PITEMINFO pItem;
	WORD wItemID;
	MARKETITEM Item;
	int i;

	if (!MarketIsNearReception (pChar) || (pChar->szAcc[0] == 0)) {
		SendResult (pChar, CMD_MARKETPUT, MARKETRES_FAR, 0, 0, dwPrice, 0);
		return;
	}
	/* 出せる品か（手持ちにある・装備中でない・取引できる・依頼の品でない） */
	if ((byNo >= MAX_MYITEM) || (pChar->waItem[byNo] == 0) ||
			!ItemIDValid (pChar->waItem[byNo])) {
		SendResult (pChar, CMD_MARKETPUT, MARKETRES_NGITEM, 0, 0, dwPrice, 0);
		return;
	}
	wItemID = pChar->waItem[byNo];
	pItem = g_aItemInfo[wItemID];
	for (i = 0; i < 10; i ++) {
		if (pChar->waEqu[i] == byNo) {
			SendResult (pChar, CMD_MARKETPUT, MARKETRES_NGITEM, 0, wItemID, dwPrice, 0);
			return;
		}
	}
	if ((pItem == NULL) || (pItem->byFlg & ITEMFLG_TRADE) || QuestIsQuestItem (wItemID)) {
		SendResult (pChar, CMD_MARKETPUT, MARKETRES_NGITEM, 0, wItemID, dwPrice, 0);
		return;
	}
	if (CountItem (pChar->szAcc) >= MARKET_MAXPUT) {
		SendResult (pChar, CMD_MARKETPUT, MARKETRES_LIMIT, 0, wItemID, dwPrice, 0);
		return;
	}
	if ((dwPrice < CalcPriceMin (wItemID)) || (dwPrice > MARKET_PRICEMAX)) {
		SendResult (pChar, CMD_MARKETPUT, MARKETRES_PRICE, 0, wItemID, dwPrice, 0);
		return;
	}

	/* 手持ちから外して出品にする */
	DelCharItem (pChar->dwTblID, byNo);
	ZeroMemory (&Item, sizeof (Item));
	Item.dwID		= g_dwMarketNextID ++;
	CopyAcc (Item.szAcc, pChar->szAcc);
	Item.dwCharID	= pChar->dwTblID;
	strncpy (Item.szSeller, pChar->szName, MAX_CHARNAMEBUF - 1);
	Item.wItemID	= wItemID;
	Item.dwPrice	= dwPrice;
	Item.dwTime		= NowSec ();
	Item.byState	= MARKETSTATE_ON;
	g_aMarketItem.push_back (Item);

	SendResult (pChar, CMD_MARKETPUT, MARKETRES_OK, Item.dwID, wItemID, dwPrice, dwPrice - CalcFee (dwPrice));
	SendBagState (pChar);
}

/* ========================================================================= */
/* 関数名：MarketBuy														 */
/* 内容：買う（見えていた出品ID・品・値段が今も同じなら）					 */
/* ========================================================================= */
void MarketBuy(PCHARINFO pChar, DWORD dwID, WORD wItemID, DWORD dwPrice)
{
	int nNo;
	DWORD dwFee;
	int nSale;

	if (!MarketIsNearReception (pChar) || (pChar->szAcc[0] == 0)) {
		SendResult (pChar, CMD_MARKETBUY, MARKETRES_FAR, dwID, wItemID, dwPrice, 0);
		return;
	}
	MarketExpire (NowSec ());
	nNo = FindItem (dwID);
	/* もう無い・期限切れ・品や値段が変わった */
	if ((nNo < 0) || (g_aMarketItem[nNo].byState != MARKETSTATE_ON) ||
			(g_aMarketItem[nNo].wItemID != wItemID) || (g_aMarketItem[nNo].dwPrice != dwPrice)) {
		SendResult (pChar, CMD_MARKETBUY, MARKETRES_GONE, dwID, wItemID, dwPrice, 0);
		return;
	}
	MARKETITEM Item = g_aMarketItem[nNo];
	if (SameAcc (Item.szAcc, pChar->szAcc)) {
		SendResult (pChar, CMD_MARKETBUY, MARKETRES_OWN, dwID, wItemID, dwPrice, 0);
		return;
	}
	if (pChar->dwGold < Item.dwPrice) {
		SendResult (pChar, CMD_MARKETBUY, MARKETRES_NOGOLD, dwID, wItemID, dwPrice, 0);
		return;
	}
	if (!CanAddCharItem (pChar->dwTblID, wItemID)) {
		SendResult (pChar, CMD_MARKETBUY, MARKETRES_ITEMFULL, dwID, wItemID, dwPrice, 0);
		return;
	}

	/* お金を払って品を受け取り、手取りを出品者のアカウントの売上へ。出品はその場で消す */
	pChar->dwGold -= Item.dwPrice;
	AddCharItem (pChar->dwTblID, Item.wItemID);
	dwFee = CalcFee (Item.dwPrice);
	nSale = FindSale (Item.szAcc, TRUE);
	g_aMarketSale[nSale].dwGold += Item.dwPrice - dwFee;
	g_aMarketItem.erase (g_aMarketItem.begin () + nNo);

	SendResult (pChar, CMD_MARKETBUY, MARKETRES_OK, dwID, wItemID, Item.dwPrice, 0);
	SendBagState (pChar);
}

/* ========================================================================= */
/* 関数名：MarketTakeBack													 */
/* 内容：自分のアカウントの出品を手持ちへ戻す（出品中でも期限切れでも）		 */
/* ========================================================================= */
void MarketTakeBack(PCHARINFO pChar, DWORD dwID)
{
	int nNo;
	WORD wItemID;

	if (!MarketIsNearReception (pChar) || (pChar->szAcc[0] == 0)) {
		SendResult (pChar, CMD_MARKETTAKEBACK, MARKETRES_FAR, dwID, 0, 0, 0);
		return;
	}
	nNo = FindItem (dwID);
	if ((nNo < 0) || !SameAcc (g_aMarketItem[nNo].szAcc, pChar->szAcc)) {
		SendResult (pChar, CMD_MARKETTAKEBACK, MARKETRES_GONE, dwID, 0, 0, 0);
		return;
	}
	wItemID = g_aMarketItem[nNo].wItemID;
	if (!CanAddCharItem (pChar->dwTblID, wItemID)) {
		SendResult (pChar, CMD_MARKETTAKEBACK, MARKETRES_ITEMFULL, dwID, wItemID, g_aMarketItem[nNo].dwPrice, 0);
		return;
	}
	AddCharItem (pChar->dwTblID, wItemID);
	g_aMarketItem.erase (g_aMarketItem.begin () + nNo);

	SendResult (pChar, CMD_MARKETTAKEBACK, MARKETRES_OK, dwID, wItemID, 0, 0);
	SendBagState (pChar);
}

/* ========================================================================= */
/* 関数名：MarketGoldGet													 */
/* 内容：売上を所持金へ受け取る（所持金の上限 999999 を超える分は受付に残す）	 */
/* ========================================================================= */
void MarketGoldGet(PCHARINFO pChar)
{
	int nNo;
	DWORD dwSale, dwRoom, dwGet;

	if (!MarketIsNearReception (pChar) || (pChar->szAcc[0] == 0)) {
		SendResult (pChar, CMD_MARKETGOLDGET, MARKETRES_FAR, 0, 0, 0, 0);
		return;
	}
	nNo = FindSale (pChar->szAcc, FALSE);
	dwSale = (nNo < 0) ? 0 : g_aMarketSale[nNo].dwGold;
	if (dwSale == 0) {
		SendResult (pChar, CMD_MARKETGOLDGET, MARKETRES_NOGOLDSALE, 0, 0, 0, 0);
		return;
	}
	dwRoom = (pChar->dwGold >= MARKET_PRICEMAX) ? 0 : (MARKET_PRICEMAX - pChar->dwGold);
	if (dwRoom == 0) {
		SendResult (pChar, CMD_MARKETGOLDGET, MARKETRES_GOLDMAX, 0, 0, dwSale, 0);
		return;
	}
	dwGet = min (dwSale, dwRoom);
	pChar->dwGold += dwGet;
	g_aMarketSale[nNo].dwGold -= dwGet;
	dwSale -= dwGet;
	if (dwSale == 0) {
		g_aMarketSale.erase (g_aMarketSale.begin () + nNo);
	}

	SendResult (pChar, CMD_MARKETGOLDGET, MARKETRES_OK, 0, 0, dwSale, dwGet);
	SendBagState (pChar);
}

/* Copyright(C)URARA-works 2003-2005 */
