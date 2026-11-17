/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrRecycle.cpp											 */
/* 概要：		リサイクルボックス（おすそわけの共有の箱）の処理(実装ファイル) */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 仕様は docs/recycle.md。箱は台詞が "@d" のNPC。クライアントが話しかけると画面が開き、
 * 中身・入れる・もらうをサーバーへ頼む（CMD_RECYCLE*）。取引所（SboSvrMarket.cpp）と同じ作りで、
 * 頼みごとのたびにサーバーが「箱の近くにいるか」「手持ち・中身・今日の残りが合うか」を確かめる。
 * 中身は箱の NPC番号（dwNpcID）ごとに持つ。1 日にもらえる数はアカウントごと（日本時間の 0 時に戻る）。
 * 誰がいつ何を入れた・もらったかは記録に残す（新しい RECYCLE_LOGMAX 件）。
 * 保存は SaveLoad/SboSvrSaveLoadRecycle.cpp（SBODATA/SboDataRecycle.dat）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrRecycle.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
std::vector<RECYCLESTOCK> g_aRecycleStock;						/* 箱の中身（入れた順） */
std::vector<RECYCLEDAILY> g_aRecycleDaily;						/* 今日もらった個数 */
std::vector<RECYCLELOG> g_aRecycleLog;							/* 記録（古い順） */
DWORD g_dwRecycleNowOverride = 0;								/* 0 以外ならこれを「今の時刻」にする（テスト用） */

#define RECYCLE_JST_SEC		(9 * 60 * 60)						/* 日本時間は UTC より 9 時間進んでいる */

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* アカウント名が同じか（欄の長さまで比べる） */
static BOOL SameAcc(LPCSTR pszA, LPCSTR pszB)
{
	return (pszA[0] != 0) && (strncmp (pszA, pszB, MAX_ACCNAME) == 0);
}

/* 結果を送る（要求の欄はそのまま返す） */
static void SendResult(PCHARINFO pChar, BYTE byCmd, BYTE byResult, DWORD dwNpcTblID, WORD wItemID, BYTE byCount)
{
	CMDPARA_RECYCLE Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= byCmd;
	Para.byResult	= byResult;
	Para.byCount	= byCount;
	Para.byLeft		= (BYTE)RecycleLeft (pChar->szAcc);
	Para.wItemID	= wItemID;
	Para.dwTblID	= pChar->dwTblID;
	Para.dwNpcTblID	= dwNpcTblID;
	svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
}

/* 箱の中身の欄を探す（無ければ -1） */
static int FindStock(DWORD dwNpcID, WORD wItemID)
{
	size_t i;

	for (i = 0; i < g_aRecycleStock.size (); i ++) {
		if ((g_aRecycleStock[i].dwNpcID == dwNpcID) && (g_aRecycleStock[i].wItemID == wItemID)) {
			return (int)i;
		}
	}
	return -1;
}

/* 今日の分の欄を探す（無ければ bAdd のとき作る。作らないなら -1）。古い日付の欄は今日の 0 個にして使う */
static int FindDaily(LPCSTR pszAcc, BOOL bAdd)
{
	size_t i;
	DWORD dwDay = RecycleDay (RecycleNowSec ());
	RECYCLEDAILY Daily;

	for (i = 0; i < g_aRecycleDaily.size (); i ++) {
		if (SameAcc (g_aRecycleDaily[i].szAcc, pszAcc)) {
			if (g_aRecycleDaily[i].dwDay != dwDay) {
				g_aRecycleDaily[i].dwDay = dwDay;
				g_aRecycleDaily[i].byCount = 0;
			}
			return (int)i;
		}
	}
	if (!bAdd) {
		return -1;
	}
	ZeroMemory (&Daily, sizeof (Daily));
	strncpy (Daily.szAcc, pszAcc, MAX_ACCNAME - 1);
	Daily.dwDay = dwDay;
	g_aRecycleDaily.push_back (Daily);
	return (int)g_aRecycleDaily.size () - 1;
}

/* 記録を足す（RECYCLE_LOGMAX 件を超えたら古いほうから捨てる） */
static void AddLog(PCHARINFO pChar, PCHARINFO pNpc, BYTE byKind, WORD wItemID, BYTE byCount)
{
	RECYCLELOG Log;

	ZeroMemory (&Log, sizeof (Log));
	Log.dwTime = RecycleNowSec ();
	strncpy (Log.szAcc, pChar->szAcc, MAX_ACCNAME - 1);
	strncpy (Log.szName, pChar->szName, MAX_CHARNAMEBUF - 1);
	Log.dwNpcID = pNpc->dwNpcID;
	Log.byKind = byKind;
	Log.wItemID = wItemID;
	Log.byCount = byCount;
	g_aRecycleLog.push_back (Log);
	RecycleTrimLog ();
}

/* 持ち物の変化を本人に送る */
static void SendBagState(PCHARINFO pChar)
{
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
}

/* 手持ちの位置 nNo が装備中か */
static BOOL IsEquipped(PCHARINFO pChar, int nNo)
{
	int i;

	for (i = 0; i < 10; i ++) {
		if (pChar->waEqu[i] == nNo) {
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：RecycleClear														 */
/* 内容：全部消す															 */
/* ========================================================================= */
void RecycleClear(void)
{
	g_aRecycleStock.clear ();
	g_aRecycleDaily.clear ();
	g_aRecycleLog.clear ();
}

/* ========================================================================= */
/* 関数名：RecycleNowSec / RecycleDay										 */
/* 内容：今の時刻（秒）／時刻から日本時間の日付（日数）を求める				 */
/* ========================================================================= */
DWORD RecycleNowSec(void)
{
	return g_dwRecycleNowOverride ? g_dwRecycleNowOverride : (DWORD)time (NULL);
}

DWORD RecycleDay(DWORD dwSec)
{
	return (dwSec + RECYCLE_JST_SEC) / (24 * 60 * 60);
}

/* ========================================================================= */
/* 関数名：RecycleLeft														 */
/* 内容：アカウントが今日あと何個もらえるか									 */
/* ========================================================================= */
int RecycleLeft(LPCSTR pszAcc)
{
	int nNo;

	if (pszAcc[0] == 0) {
		return 0;
	}
	nNo = FindDaily (pszAcc, FALSE);
	if (nNo < 0) {
		return RECYCLE_DAYLIMIT;
	}
	return max (RECYCLE_DAYLIMIT - (int)g_aRecycleDaily[nNo].byCount, 0);
}

/* ========================================================================= */
/* 関数名：RecycleKindCount / RecycleStockCount								 */
/* 内容：箱に入っている種類の数／箱にあるその品の個数						 */
/* ========================================================================= */
int RecycleKindCount(DWORD dwNpcID)
{
	size_t i;
	int nCount = 0;

	for (i = 0; i < g_aRecycleStock.size (); i ++) {
		if (g_aRecycleStock[i].dwNpcID == dwNpcID) {
			nCount ++;
		}
	}
	return nCount;
}

int RecycleStockCount(DWORD dwNpcID, WORD wItemID)
{
	int nNo = FindStock (dwNpcID, wItemID);

	return (nNo < 0) ? 0 : g_aRecycleStock[nNo].byCount;
}

/* ========================================================================= */
/* 関数名：RecycleTrimLog													 */
/* 内容：記録を RECYCLE_LOGMAX 件に切る（古いほうから捨てる）				 */
/* ========================================================================= */
void RecycleTrimLog(void)
{
	if (g_aRecycleLog.size () > RECYCLE_LOGMAX) {
		g_aRecycleLog.erase (g_aRecycleLog.begin (), g_aRecycleLog.end () - RECYCLE_LOGMAX);
	}
}

/* ========================================================================= */
/* 関数名：RecycleIsBox														 */
/* 内容：箱（台詞が "@d"・NPC番号がある保存NPC）か							 */
/* ========================================================================= */
BOOL RecycleIsBox(PCHARINFO pNpc)
{
	if ((pNpc == NULL) || (pNpc->szAcc[0] != 0) || (pNpc->byNpcType == NPCTYPE_INIT) ||
			(pNpc->pszNpcSpeech == NULL) || (pNpc->dwNpcID == 0)) {
		return FALSE;
	}
	return (pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'd');
}

/* ========================================================================= */
/* 関数名：RecycleFindBox													 */
/* 内容：話しかけた箱を探す（箱で、同じマップ・同じ空間で RECYCLE_RANGE マス以内）*/
/* ========================================================================= */
PCHARINFO RecycleFindBox(PCHARINFO pChar, DWORD dwNpcTblID)
{
	PCHARINFO pNpc;

	if ((dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ()) || (pChar->szAcc[0] == 0)) {
		return NULL;
	}
	pNpc = g_aCharInfo[dwNpcTblID];
	if (!RecycleIsBox (pNpc)) {
		return NULL;
	}
	if ((pNpc->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pNpc)) {
		return NULL;
	}
	if ((abs ((int)pChar->dwX - (int)pNpc->dwX) > RECYCLE_RANGE) ||
			(abs ((int)pChar->dwY - (int)pNpc->dwY) > RECYCLE_RANGE)) {
		return NULL;
	}
	return pNpc;
}

/* ========================================================================= */
/* 関数名：RecycleList														 */
/* 内容：箱の中身（入れた順）と、今日あと何個もらえるかを送る				 */
/* ========================================================================= */
void RecycleList(PCHARINFO pChar, DWORD dwNpcTblID)
{
	CMDPARA_RECYCLELIST Send;
	PCHARINFO pNpc = RecycleFindBox (pChar, dwNpcTblID);
	size_t i;
	int nCount = 0;
	DWORD dwSize;

	if (pNpc == NULL) {
		return;
	}
	ZeroMemory (&Send, sizeof (Send));
	Send.byCmd			= CMD_RECYCLELIST;
	Send.byLeft			= (BYTE)RecycleLeft (pChar->szAcc);
	Send.dwTblID		= pChar->dwTblID;
	Send.dwNpcTblID		= dwNpcTblID;
	for (i = 0; (i < g_aRecycleStock.size ()) && (nCount < RECYCLE_MAXKIND); i ++) {
		if (g_aRecycleStock[i].dwNpcID != pNpc->dwNpcID) {
			continue;
		}
		Send.aEntry[nCount].wItemID	= g_aRecycleStock[i].wItemID;
		Send.aEntry[nCount].byCount	= g_aRecycleStock[i].byCount;
		nCount ++;
	}
	Send.byKind = (BYTE)nCount;

	dwSize = (DWORD)(offsetof (CMDPARA_RECYCLELIST, aEntry) + sizeof (RECYCLEENTRY) * nCount);
	svrSock.SendTo (pChar->pID, (LPBYTE)&Send, dwSize, SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：RecyclePut														 */
/* 内容：入れる（手持ちの wItemID を byCount 個。装備中の分は数えない）		 */
/* ========================================================================= */
void RecyclePut(PCHARINFO pChar, DWORD dwNpcTblID, WORD wItemID, BYTE byCount)
{
	PCHARINFO pNpc = RecycleFindBox (pChar, dwNpcTblID);
	PITEMINFO pItem;
	std::vector<int> aSlot;
	int i, nStock, nNow;

	if (pNpc == NULL) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_FAR, dwNpcTblID, wItemID, byCount);
		return;
	}
	/* 入れられる品か（手持ちにある・装備中でない・依頼の品や大事な物でない・取引できる） */
	if ((wItemID == 0) || (wItemID >= (WORD)g_aItemInfo.GetSize ())) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_NGITEM, dwNpcTblID, wItemID, byCount);
		return;
	}
	pItem = g_aItemInfo[wItemID];
	if ((pItem == NULL) || (pItem->byFlg & ITEMFLG_TRADE) || IsImportantItem (wItemID) || QuestIsQuestItem (wItemID)) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_NGITEM, dwNpcTblID, wItemID, byCount);
		return;
	}
	for (i = 0; i < MAX_MYITEM; i ++) {
		if ((pChar->waItem[i] == wItemID) && !IsEquipped (pChar, i)) {
			aSlot.push_back (i);
		}
	}
	if (aSlot.empty ()) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_NGITEM, dwNpcTblID, wItemID, byCount);
		return;
	}
	if ((byCount == 0) || ((int)aSlot.size () < (int)byCount)) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_NOHAVE, dwNpcTblID, wItemID, byCount);
		return;
	}
	/* 箱に入るか（20 種類・1 種類 255 個まで。同じ品は個数でまとめる） */
	nStock = FindStock (pNpc->dwNpcID, wItemID);
	if ((nStock < 0) && (RecycleKindCount (pNpc->dwNpcID) >= RECYCLE_MAXKIND)) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_KINDFULL, dwNpcTblID, wItemID, byCount);
		return;
	}
	nNow = (nStock < 0) ? 0 : (int)g_aRecycleStock[nStock].byCount;
	if (nNow + (int)byCount > RECYCLE_MAXCOUNT) {
		SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_COUNTFULL, dwNpcTblID, wItemID, byCount);
		return;
	}

	/* 手持ちから外して箱へ（後ろの枠から外す。外すたびに前へ詰まるので、大きい番号から） */
	for (i = 0; i < (int)byCount; i ++) {
		DelCharItem (pChar->dwTblID, (BYTE)aSlot[aSlot.size () - 1 - i]);
	}
	if (nStock < 0) {
		RECYCLESTOCK Stock;
		ZeroMemory (&Stock, sizeof (Stock));
		Stock.dwNpcID = pNpc->dwNpcID;
		Stock.wItemID = wItemID;
		Stock.byCount = byCount;
		g_aRecycleStock.push_back (Stock);
	} else {
		g_aRecycleStock[nStock].byCount = (BYTE)(nNow + (int)byCount);
	}
	AddLog (pChar, pNpc, RECYCLELOG_PUT, wItemID, byCount);

	SendResult (pChar, CMD_RECYCLEPUT, RECYCLERES_OK, dwNpcTblID, wItemID, byCount);
	SendBagState (pChar);
}

/* ========================================================================= */
/* 関数名：RecycleTake														 */
/* 内容：もらう（1 個。1 アカウント 1 日 RECYCLE_DAYLIMIT 個まで）			 */
/*       自分が入れた品でももらえる（個数の上限には数える）					 */
/* ========================================================================= */
void RecycleTake(PCHARINFO pChar, DWORD dwNpcTblID, WORD wItemID)
{
	PCHARINFO pNpc = RecycleFindBox (pChar, dwNpcTblID);
	int nStock, nDaily;

	if (pNpc == NULL) {
		SendResult (pChar, CMD_RECYCLETAKE, RECYCLERES_FAR, dwNpcTblID, wItemID, 0);
		return;
	}
	nStock = FindStock (pNpc->dwNpcID, wItemID);
	if (nStock < 0) {
		SendResult (pChar, CMD_RECYCLETAKE, RECYCLERES_EMPTY, dwNpcTblID, wItemID, 0);
		return;
	}
	if (RecycleLeft (pChar->szAcc) <= 0) {
		SendResult (pChar, CMD_RECYCLETAKE, RECYCLERES_DAILY, dwNpcTblID, wItemID, 0);
		return;
	}
	if (!CanAddCharItem (pChar->dwTblID, wItemID)) {
		SendResult (pChar, CMD_RECYCLETAKE, RECYCLERES_ITEMFULL, dwNpcTblID, wItemID, 0);
		return;
	}

	/* 品を受け取り、箱から 1 個減らす（0 個になったら種類ごと消す）。今日の数を 1 つ使う */
	AddCharItem (pChar->dwTblID, wItemID);
	g_aRecycleStock[nStock].byCount --;
	if (g_aRecycleStock[nStock].byCount == 0) {
		g_aRecycleStock.erase (g_aRecycleStock.begin () + nStock);
	}
	nDaily = FindDaily (pChar->szAcc, TRUE);
	g_aRecycleDaily[nDaily].byCount ++;
	AddLog (pChar, pNpc, RECYCLELOG_TAKE, wItemID, 1);

	SendResult (pChar, CMD_RECYCLETAKE, RECYCLERES_OK, dwNpcTblID, wItemID, 1);
	SendBagState (pChar);
}

/* Copyright(C)URARA-works 2003-2005 */
