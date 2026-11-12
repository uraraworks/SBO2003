/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboSvrCharInfoTimerProc.cpp									 */
/* 概要：		キャラ情報タイマーイベント処理								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/17													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSendCmd.h"


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

void		DeleteAllTimerEvent			(PCHARINFO pChar);								/* タイマーイベントを削除 */
void		CharInfoTimerProc			(DWORD dwTblID);								/* キャラ情報タイマーイベント処理 */
BOOL		CheckCharInfoTimer			(DWORD dwTblID, BYTE byType);					/* 指定したイベントを持っているかチェック */
static BOOL	CharInfoTimerProc_PW		(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(力) */
static BOOL	CharInfoTimerProc_DIFENCE	(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(防御力倍) */
static BOOL	CharInfoTimerProc_HASTE		(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(加速) */
static BOOL	CharInfoTimerProc_POISON	(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(毒) */
static BOOL	CharInfoTimerProc_SLEEP		(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(眠り) */
static BOOL	CharInfoTimerProc_STATUSUP	(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(攻撃力・守備力アップ) */

#define POISON_INTERVAL		2000		/* 毒のダメージの間隔(ms) */
#define SLEEP_EFC_INTERVAL	2000		/* 眠っている間に「Zzz」を出す間隔(ms) */

extern long genrand();					/* 乱数取得 */


/* ========================================================================= */
/* 関数名：	DeleteAllTimerEvent												 */
/* 内容：	タイマーイベントを削除											 */
/* 日付：	2006/01/04														 */
/* ========================================================================= */

void DeleteAllTimerEvent(PCHARINFO pChar)
{
	int i, nCount;

	nCount = pChar->aTimerEvent.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SAFE_DELETE (pChar->aTimerEvent[i]);
	}
	pChar->aTimerEvent.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc												 */
/* 内容：	キャラ情報タイマーイベント処理									 */
/* 日付：	2004/10/17														 */
/* ========================================================================= */

void CharInfoTimerProc(DWORD dwTblID)
{
	int i, nCount;
	PCHARINFO pChar;

	pChar = g_aCharInfo[dwTblID];
	nCount = pChar->aTimerEvent.GetSize ();

	/* タイマーイベントは設定されていない？ */
	if (nCount <= 0) {
		goto Exit;
	}

	for (i = 0; i < nCount; i ++) {
		PCHARINFOTIMEREVENT pInfo;
		BOOL bResult;

		pInfo = pChar->aTimerEvent[i];
		bResult = FALSE;

		switch (pInfo->byType) {
		case CHARINFOTIMEREVENT_PW:			/* 力 */
			bResult = CharInfoTimerProc_PW (pChar, pInfo);
			break;
		case CHARINFOTIMEREVENT_ST:			/* 体力 */
			break;
		case CHARINFOTIMEREVENT_DEX:		/* 素早さ */
			break;
		case CHARINFOTIMEREVENT_HPUPDOWN:	/* HP増減 */
			break;
		case CHARINFOTIMEREVENT_DIFENCE:	/* 防御力倍 */
			bResult = CharInfoTimerProc_DIFENCE (pChar, pInfo);
			break;
		case CHARINFOTIMEREVENT_HASTE:		/* 加速 */
			bResult = CharInfoTimerProc_HASTE (pChar, pInfo);
			break;
		case CHARINFOTIMEREVENT_POISON:		/* 毒 */
			bResult = CharInfoTimerProc_POISON (pChar, pInfo);
			break;
		case CHARINFOTIMEREVENT_SLEEP:		/* 眠り */
			bResult = CharInfoTimerProc_SLEEP (pChar, pInfo);
			break;
		case CHARINFOTIMEREVENT_ATUP:		/* 攻撃力アップ */
		case CHARINFOTIMEREVENT_DFUP:		/* 守備力アップ */
			bResult = CharInfoTimerProc_STATUSUP (pChar, pInfo);
			break;
		}
		if (bResult) {
			SAFE_DELETE (pInfo);
			pChar->aTimerEvent.RemoveAt (i);
			RenewATDF (pChar->dwTblID);
			break;
		}
	}

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CheckCharInfoTimer												 */
/* 内容：	指定したイベントを持っているかチェック							 */
/* 日付：	2004/10/17														 */
/* ========================================================================= */

BOOL CheckCharInfoTimer(DWORD dwTblID, BYTE byType)
{
	int i, nCount;
	BOOL bRet;
	PCHARINFO pChar;

	bRet = FALSE;
	pChar = g_aCharInfo[dwTblID];
	nCount = pChar->aTimerEvent.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFOTIMEREVENT pInfo;

		pInfo = pChar->aTimerEvent[i];
		if (pInfo->byType == byType) {
			bRet = TRUE;
			goto Exit;
		}
	}

Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_PW											 */
/* 内容：	キャラ情報タイマーイベント処理(力)								 */
/* 日付：	2004/10/22														 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_PW(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	DWORD dwTime;
	BOOL bRet;

	bRet = FALSE;
	dwTime = timeGetTime ();

	/* 有効期間は過ぎていない？ */
	if (abs (dwTime - pInfo->dwTimeStart) < (int)pInfo->dwTimeValid) {
		goto Exit;
	}

	pChar->chTimerPW -= (char)pInfo->byData;

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_DIFENCE										 */
/* 内容：	キャラ情報タイマーイベント処理(防御力倍)						 */
/* 日付：	2004/10/17														 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_DIFENCE(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	DWORD dwTime;
	BOOL bRet;

	bRet = FALSE;
	dwTime = timeGetTime ();

	/* 有効期間は過ぎていない？ */
	if (abs (dwTime - pInfo->dwTimeStart) < (int)pInfo->dwTimeValid) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：	SetCharInfoTimerStatus											 */
/* 内容：	状態(加速・毒・眠り)をかける。同じ種類がかかっていればかけ直す	 */
/* ========================================================================= */

void SetCharInfoTimerStatus(PCHARINFO pChar, BYTE byType, BYTE byData, DWORD dwTimeValid)
{
	int i, nCount;
	PCHARINFOTIMEREVENT pInfo;

	pInfo = NULL;
	nCount = pChar->aTimerEvent.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pChar->aTimerEvent[i]->byType == byType) {
			pInfo = pChar->aTimerEvent[i];
			break;
		}
	}
	if (pInfo == NULL) {
		pInfo = new CHARINFOTIMEREVENT;
		pChar->aTimerEvent.Add (pInfo);
	}
	ZeroMemory (pInfo, sizeof (CHARINFOTIMEREVENT));
	pInfo->byType		= byType;
	pInfo->byData		= byData;
	pInfo->dwTimeStart	= timeGetTime ();
	pInfo->dwTimeValid	= dwTimeValid;
	pInfo->dwTimeLast	= pInfo->dwTimeStart;
	if (byType == CHARINFOTIMEREVENT_SLEEP) {
		/* 眠った時のHP(これより減ったら起きる) */
		pInfo->dwTimeLast = pChar->wHP;
		pInfo->dwTimeInterval = pInfo->dwTimeStart;
	}
}


/* ========================================================================= */
/* 関数名：	GetHasteRate / IsSleep											 */
/* 内容：	加速の率(%)・眠っているか										 */
/* ========================================================================= */

BYTE GetHasteRate(PCHARINFO pChar)
{
	int i, nCount;

	nCount = pChar->aTimerEvent.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pChar->aTimerEvent[i]->byType == CHARINFOTIMEREVENT_HASTE) {
			return pChar->aTimerEvent[i]->byData;
		}
	}
	return 0;
}

BOOL IsSleep(PCHARINFO pChar)
{
	return CheckCharInfoTimer (pChar->dwTblID, CHARINFOTIMEREVENT_SLEEP);
}


/* ========================================================================= */
/* 関数名：	SetCharInfoTimerStatusMin										 */
/* 内容：	攻撃力・守備力アップの「最低でも上がる量」を覚えておく			 */
/* ========================================================================= */

void SetCharInfoTimerStatusMin(PCHARINFO pChar, BYTE byType, DWORD dwMin)
{
	int i, nCount;

	nCount = pChar->aTimerEvent.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pChar->aTimerEvent[i]->byType == byType) {
			pChar->aTimerEvent[i]->dwTimeInterval = dwMin;
			break;
		}
	}
}


/* ========================================================================= */
/* 関数名：	GetStatusUpBonus												 */
/* 内容：	攻撃力・守備力アップで上がる量									 */
/*			nBase(アップ無しの値)の率(%)、ただし最低でも決めた量は上がる	 */
/*			かかっていなければ0												 */
/* ========================================================================= */

int GetStatusUpBonus(PCHARINFO pChar, BYTE byType, int nBase)
{
	int i, nCount;
	PCHARINFOTIMEREVENT pInfo;

	nCount = pChar->aTimerEvent.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = pChar->aTimerEvent[i];
		if (pInfo->byType == byType) {
			return max (nBase * (int)pInfo->byData / 100, (int)pInfo->dwTimeInterval);
		}
	}
	return 0;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_STATUSUP										 */
/* 内容：	キャラ情報タイマーイベント処理(攻撃力・守備力アップ)			 */
/*			時間が来たら終わり(戻した値は呼び出し元の RenewATDF で反映)		 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_STATUSUP(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	if (timeGetTime () - pInfo->dwTimeStart < pInfo->dwTimeValid) {
		return FALSE;
	}
	if (pChar->byNpcType == NPCTYPE_INIT) {
		SendCmdSYSMSG (pChar->pID,
				(pInfo->byType == CHARINFOTIMEREVENT_ATUP) ? "攻撃アップの効果が切れました" : "守りアップの効果が切れました",
				SPEECHCMD_BATTLE1);
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_HASTE											 */
/* 内容：	キャラ情報タイマーイベント処理(加速)：時間が来たら終わり		 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_HASTE(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	if (timeGetTime () - pInfo->dwTimeStart < pInfo->dwTimeValid) {
		return FALSE;
	}
	/* PCには加速が切れたことを知らせる(ゲージの速さを戻す) */
	if (pChar->byNpcType == NPCTYPE_INIT) {
		SendCmdHASTE (pChar->dwTblID, 0, 0);
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_POISON										 */
/* 内容：	キャラ情報タイマーイベント処理(毒)：2秒ごとにHPが減る			 */
/*			毒では倒れない(HPは1で止まる)									 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_POISON(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	char szTmp[16];
	DWORD dwTime;
	int nDamage;

	dwTime = timeGetTime ();
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return TRUE;
	}
	if (dwTime - pInfo->dwTimeLast >= POISON_INTERVAL) {
		pInfo->dwTimeLast = dwTime;
		nDamage = min ((int)pInfo->byData, (int)pChar->wHP - 1);
		if (nDamage > 0) {
			pChar->wHP -= (WORD)nDamage;
			wsprintf (szTmp, "%d", nDamage);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (190, 80, 255), szTmp);
			SendCmdCHARSTATUSHPMANA (pChar->dwTblID, pChar->dwTblID);
		}
	}
	if (dwTime - pInfo->dwTimeStart < pInfo->dwTimeValid) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CharInfoTimerProc_SLEEP											 */
/* 内容：	キャラ情報タイマーイベント処理(眠り)：時間が来るか、ダメージを	 */
/*			受けてHPが減ったら起きる										 */
/* ========================================================================= */

static BOOL CharInfoTimerProc_SLEEP(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo)
{
	DWORD dwTime;

	dwTime = timeGetTime ();
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return TRUE;
	}
	/* 回復したらその HP を基準にする(回復の後に殴られたら起きる) */
	if ((DWORD)pChar->wHP > pInfo->dwTimeLast) {
		pInfo->dwTimeLast = pChar->wHP;
	}
	/* 起きた？(時間切れ・殴られた) */
	if ((dwTime - pInfo->dwTimeStart >= pInfo->dwTimeValid) || ((DWORD)pChar->wHP < pInfo->dwTimeLast)) {
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 255, 255), "!?");
		return TRUE;
	}
	/* 眠っている間はときどき「Zzz」 */
	if (dwTime - pInfo->dwTimeInterval >= SLEEP_EFC_INTERVAL) {
		pInfo->dwTimeInterval = dwTime;
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (160, 200, 255), "Zzz");
	}
	return FALSE;
}

/* Copyright(C)URARA-works 2003-2006 */
