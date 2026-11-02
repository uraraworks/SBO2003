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


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

void		DeleteAllTimerEvent			(PCHARINFO pChar);								/* タイマーイベントを削除 */
void		CharInfoTimerProc			(DWORD dwTblID);								/* キャラ情報タイマーイベント処理 */
BOOL		CheckCharInfoTimer			(DWORD dwTblID, BYTE byType);					/* 指定したイベントを持っているかチェック */
static BOOL	CharInfoTimerProc_PW		(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(力) */
static BOOL	CharInfoTimerProc_DIFENCE	(PCHARINFO pChar, PCHARINFOTIMEREVENT pInfo);	/* キャラ情報タイマーイベント処理(防御力倍) */

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

/* Copyright(C)URARA-works 2003-2006 */
