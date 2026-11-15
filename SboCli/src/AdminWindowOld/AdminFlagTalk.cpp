/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		フラグの名前表・会話データ・キャラのフラグ(管理者用)実装ファイル	 */
/* ファイル名：	AdminFlagTalk.cpp											 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * 画面を持たない、管理者ウィンドウ用の通信と手元のデータの置き場（使い方は AdminFlagTalk.h）。
 * ギルドの依頼（AdminDlgQuestInfo.cpp）と同じ形で、サーバーから 1 件ずつ・または
 * まとめて届いたものを、配列の番号をサーバーと同じにして覚えておく。
 */
#include "stdafx.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminFlagTalk.h"
#include <vector>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static std::vector<FLAGNAMEINFO> s_aFlagName;							/* サーバーから届いた名前表（配列の番号はサーバーと同じ） */
static std::vector<BYTE> s_abyFlagNameHave;								/* その番号が届いているか */
static DWORD s_dwFlagNameSeq = 0;

static std::vector<TALKINFO> s_aTalk;									/* サーバーから届いた会話セット */
static std::vector<BYTE> s_abyTalkHave;
static DWORD s_dwTalkSeq = 0;
static std::vector<CMDPARA_ADMINTALKUSE> s_aTalkMap;					/* サーバーから届いたマップの名前（届いた順＝サーバーのマップの並び） */
static std::vector<CMDPARA_ADMINTALKUSE> s_aTalkUse;					/* サーバーから届いた会話セットの使用先 */
static BOOL s_bTalkLoaded = FALSE;										/* 取り直しの返事が最後の 1 件まで届いたか */

static BOOL s_bCharFlag = FALSE;										/* キャラのフラグが届いているか */
static DWORD s_dwCharFlagTblID = 0;
static BYTE s_abyCharFlag[MAX_FLAGBITBYTE];
static BYTE s_abyCharFlagNum[MAX_FLAGNUM];
static DWORD s_dwCharFlagSeq = 0;

static ADMINFLAGTALKRESULT s_Result;									/* 直近の書き込みの結果 */
static BOOL s_bFlagNameAsked = FALSE;									/* 名前表を一度取り寄せたか */
static std::vector<void (*)(void)> s_apfnListener;						/* 届いた時にお知らせする先（画面） */

/* ========================================================================= */
/* 関数名：Notify															 */
/* 内容：届いたことを画面に知らせる（画面は更新番号を見て描き直す）			 */
/*       お知らせの中で画面が外れてもよいよう、写しを回す					 */
/* ========================================================================= */
static void Notify(void)
{
	std::vector<void (*)(void)> apfn = s_apfnListener;

	for (size_t i = 0; i < apfn.size (); i ++) {
		/* 呼ぶ前に外されていないか見る */
		if (std::find (s_apfnListener.begin (), s_apfnListener.end (), apfn[i]) != s_apfnListener.end ()) {
			apfn[i] ();
		}
	}
}

void AdminFlagTalkAddListener(void (*pfnListener)(void))
{
	if (std::find (s_apfnListener.begin (), s_apfnListener.end (), pfnListener) == s_apfnListener.end ()) {
		s_apfnListener.push_back (pfnListener);
	}
}

void AdminFlagTalkRemoveListener(void (*pfnListener)(void))
{
	s_apfnListener.erase (std::remove (s_apfnListener.begin (), s_apfnListener.end (), pfnListener), s_apfnListener.end ());
}

/* ========================================================================= */
/* 関数名：SetResult														 */
/* 内容：書き込みの結果を覚える												 */
/* ========================================================================= */
static void SetResult(BYTE byCmd, BYTE byResult, int nNo, const char *pszMsg)
{
	s_Result.dwSeq ++;
	s_Result.byCmd = byCmd;
	s_Result.byResult = byResult;
	s_Result.nNo = nNo;
	strncpy (s_Result.szMsg, pszMsg ? pszMsg : "", sizeof (s_Result.szMsg) - 1);
	s_Result.szMsg[sizeof (s_Result.szMsg) - 1] = 0;
}

/* ========================================================================= */
/* フラグの名前表															 */
/* ========================================================================= */
void AdminFlagNameOnRecv(const void *pData)
{
	const CMDPARA_ADMINFLAGNAME *pPara = (const CMDPARA_ADMINFLAGNAME *)pData;
	int nCount = pPara->wCount;

	if (nCount > MAX_FLAGNAMEINFO) {
		nCount = MAX_FLAGNAMEINFO;
	}
	switch (pPara->byCmdSub) {
	case ADMINFLAGNAME_ITEM:
		/* 件数が変わっていたら合わせる（増えた分はまだ届いていない） */
		if ((int)s_aFlagName.size () != nCount) {
			FLAGNAMEINFO Empty;
			ZeroMemory (&Empty, sizeof (Empty));
			s_aFlagName.resize (nCount, Empty);
			s_abyFlagNameHave.resize (nCount, 0);
		}
		if ((pPara->nNo >= 0) && (pPara->byCount <= MAX_ADMINFLAGNAMEBATCH)) {
			for (int i = 0; i < pPara->byCount; i ++) {
				int nNo = pPara->nNo + i;
				if (nNo >= nCount) {
					break;
				}
				s_aFlagName[nNo] = pPara->aInfo[i];
				s_aFlagName[nNo].szName[sizeof (s_aFlagName[nNo].szName) - 1] = 0;
				s_abyFlagNameHave[nNo] = 1;
			}
		}
		s_dwFlagNameSeq ++;
		break;

	case ADMINFLAGNAME_RESULT:
		SetResult (CMD_ADMINFLAGNAME, pPara->byResult, pPara->nNo, pPara->szMsg);
		break;
	}
	Notify ();
}

void AdminFlagNameRequest(void)
{
	s_bFlagNameAsked = TRUE;
	s_aFlagName.clear ();
	s_abyFlagNameHave.clear ();
	s_dwFlagNameSeq ++;
	SendCmdADMINFLAGNAME (ADMINFLAGNAME_LISTREQ, 0, NULL);
}

void AdminFlagNameSend(int nNo, const FLAGNAMEINFO *pInfo)
{
	SendCmdADMINFLAGNAME (ADMINFLAGNAME_SET, nNo, pInfo);
}

int AdminFlagNameGetCount(void)
{
	return (int)s_aFlagName.size ();
}

const FLAGNAMEINFO *AdminFlagNameGet(int nNo)
{
	if ((nNo < 0) || (nNo >= (int)s_aFlagName.size ()) || !s_abyFlagNameHave[nNo]) {
		return NULL;
	}
	return &s_aFlagName[nNo];
}

DWORD AdminFlagNameGetSeq(void)
{
	return s_dwFlagNameSeq;
}

/* 名前表の配列の番号（削除済みの枠も含む。無ければ -1） */
int AdminFlagNameFindIndex(BYTE byKind, WORD wNo)
{
	for (size_t i = 0; i < s_aFlagName.size (); i ++) {
		if (s_abyFlagNameHave[i] && (s_aFlagName[i].byKind == byKind) && (s_aFlagName[i].wNo == wNo)) {
			return (int)i;
		}
	}
	return -1;
}

/* 有効な名前（無ければ NULL） */
const char *AdminFlagNameFind(BYTE byKind, WORD wNo)
{
	int nIndex = AdminFlagNameFindIndex (byKind, wNo);

	if ((nIndex < 0) || (s_aFlagName[nIndex].byState != FLAGNAMESTATE_ON)) {
		return NULL;
	}
	return s_aFlagName[nIndex].szName;
}

/* まだ一度も取り寄せていなければ取り寄せる */
void AdminFlagNameLoadIfEmpty(void)
{
	if (!s_bFlagNameAsked) {
		AdminFlagNameRequest ();
	}
}

/* ========================================================================= */
/* 会話データ																 */
/* ========================================================================= */
void AdminTalkOnRecv(const void *pData)
{
	const CMDPARA_ADMINTALK *pPara = (const CMDPARA_ADMINTALK *)pData;
	int nCount;

	/* マップの名前・使用先は ITEM より先に届く。覚えるだけで、画面へのお知らせは ITEM の時にまとめて */
	if ((pPara->byCmdSub == ADMINTALK_MAP) || (pPara->byCmdSub == ADMINTALK_USE)) {
		CMDPARA_ADMINTALKUSE Use = *(const CMDPARA_ADMINTALKUSE *)pData;

		Use.szName[sizeof (Use.szName) - 1] = 0;
		if (Use.byCmdSub == ADMINTALK_MAP) {
			if ((int)s_aTalkMap.size () < 4096) {
				s_aTalkMap.push_back (Use);
			}
		} else {
			if ((int)s_aTalkUse.size () < 65536) {
				s_aTalkUse.push_back (Use);
			}
		}
		return;
	}
	nCount = pPara->wCount;

	if (nCount > MAX_TALK) {
		nCount = MAX_TALK;
	}
	switch (pPara->byCmdSub) {
	case ADMINTALK_ITEM:
		if ((int)s_aTalk.size () != nCount) {
			TALKINFO Empty;
			ZeroMemory (&Empty, sizeof (Empty));
			s_aTalk.resize (nCount, Empty);
			s_abyTalkHave.resize (nCount, 0);
		}
		if ((pPara->nNo >= 0) && (pPara->nNo < nCount)) {
			TALKINFO *pInfo = &s_aTalk[pPara->nNo];
			*pInfo = pPara->Info;
			/* 文字の欄は必ず終端させる。行の数は範囲に収める */
			pInfo->szName[sizeof (pInfo->szName) - 1] = 0;
			if (pInfo->byLineCount > MAX_TALKLINE) {
				pInfo->byLineCount = MAX_TALKLINE;
			}
			for (int i = 0; i < MAX_TALKLINE; i ++) {
				pInfo->aLine[i].szSpeech[sizeof (pInfo->aLine[i].szSpeech) - 1] = 0;
			}
			s_abyTalkHave[pPara->nNo] = 1;
		}
		/* 一覧の最後の 1 件（0 件なら件数だけの ITEM）まで届いた */
		if ((nCount == 0) || (pPara->nNo == nCount - 1)) {
			s_bTalkLoaded = TRUE;
		}
		s_dwTalkSeq ++;
		break;

	case ADMINTALK_RESULT:
		SetResult (CMD_ADMINTALK, pPara->byResult, pPara->nNo, pPara->szMsg);
		break;
	}
	Notify ();
}

void AdminTalkRequest(void)
{
	s_aTalk.clear ();
	s_abyTalkHave.clear ();
	s_aTalkMap.clear ();
	s_aTalkUse.clear ();
	s_bTalkLoaded = FALSE;
	s_dwTalkSeq ++;
	SendCmdADMINTALK (ADMINTALK_LISTREQ, 0, NULL);
}

BOOL AdminTalkIsLoaded(void)
{
	return s_bTalkLoaded;
}

int AdminTalkGetMapCount(void)
{
	return (int)s_aTalkMap.size ();
}

DWORD AdminTalkGetMapID(int nIndex)
{
	if ((nIndex < 0) || (nIndex >= (int)s_aTalkMap.size ())) {
		return (DWORD)-1;
	}
	return s_aTalkMap[nIndex].dwMapID;
}

const char *AdminTalkGetMapName(int nIndex)
{
	if ((nIndex < 0) || (nIndex >= (int)s_aTalkMap.size ())) {
		return "";
	}
	return s_aTalkMap[nIndex].szName;
}

/* マップIDから名前（名前が空・知らないマップなら「マップ番号」。pszOut は nSize バイト） */
void AdminTalkFormatMapName(DWORD dwMapID, char *pszOut, int nSize)
{
	for (size_t i = 0; i < s_aTalkMap.size (); i ++) {
		if ((s_aTalkMap[i].dwMapID == dwMapID) && s_aTalkMap[i].szName[0]) {
			snprintf (pszOut, nSize, "%s", s_aTalkMap[i].szName);
			return;
		}
	}
	snprintf (pszOut, nSize, "マップ%u", (unsigned)dwMapID);
}

int AdminTalkGetUseCount(void)
{
	return (int)s_aTalkUse.size ();
}

const CMDPARA_ADMINTALKUSE *AdminTalkGetUse(int nIndex)
{
	if ((nIndex < 0) || (nIndex >= (int)s_aTalkUse.size ())) {
		return NULL;
	}
	return &s_aTalkUse[nIndex];
}

void AdminTalkSend(int nNo, const TALKINFO *pInfo)
{
	SendCmdADMINTALK (ADMINTALK_SET, nNo, pInfo);
}

int AdminTalkGetCount(void)
{
	return (int)s_aTalk.size ();
}

const TALKINFO *AdminTalkGet(int nNo)
{
	if ((nNo < 0) || (nNo >= (int)s_aTalk.size ()) || !s_abyTalkHave[nNo]) {
		return NULL;
	}
	return &s_aTalk[nNo];
}

DWORD AdminTalkGetSeq(void)
{
	return s_dwTalkSeq;
}

/* ========================================================================= */
/* キャラのフラグ															 */
/* ========================================================================= */
void AdminCharFlagOnRecv(const void *pData)
{
	const CMDPARA_ADMINCHARFLAG *pPara = (const CMDPARA_ADMINCHARFLAG *)pData;

	switch (pPara->byCmdSub) {
	case ADMINCHARFLAG_INFO:
		s_bCharFlag = TRUE;
		s_dwCharFlagTblID = pPara->dwTblID;
		CopyMemory (s_abyCharFlag, pPara->abyFlag, sizeof (s_abyCharFlag));
		CopyMemory (s_abyCharFlagNum, pPara->abyFlagNum, sizeof (s_abyCharFlagNum));
		s_dwCharFlagSeq ++;
		break;

	case ADMINCHARFLAG_RESULT:
		SetResult (CMD_ADMINCHARFLAG, pPara->byResult, -1, pPara->szMsg);
		break;
	}
	Notify ();
}

void AdminCharFlagRequest(DWORD dwTblID)
{
	SendCmdADMINCHARFLAG (ADMINCHARFLAG_GET, dwTblID, FLAGKIND_BIT, 0, 0);
}

void AdminCharFlagSetBit(DWORD dwTblID, WORD wNo, BOOL bOn)
{
	SendCmdADMINCHARFLAG (ADMINCHARFLAG_SET, dwTblID, FLAGKIND_BIT, wNo, bOn ? 1 : 0);
}

void AdminCharFlagSetNum(DWORD dwTblID, WORD wNo, BYTE byVal)
{
	SendCmdADMINCHARFLAG (ADMINCHARFLAG_SET, dwTblID, FLAGKIND_NUM, wNo, byVal);
}

BOOL AdminCharFlagGet(DWORD *pdwTblID, BYTE *pbyFlag, BYTE *pbyFlagNum)
{
	if (!s_bCharFlag) {
		return FALSE;
	}
	if (pdwTblID) {
		*pdwTblID = s_dwCharFlagTblID;
	}
	if (pbyFlag) {
		CopyMemory (pbyFlag, s_abyCharFlag, sizeof (s_abyCharFlag));
	}
	if (pbyFlagNum) {
		CopyMemory (pbyFlagNum, s_abyCharFlagNum, sizeof (s_abyCharFlagNum));
	}
	return TRUE;
}

DWORD AdminCharFlagGetSeq(void)
{
	return s_dwCharFlagSeq;
}

/* ========================================================================= */
/* 書き込みの結果															 */
/* ========================================================================= */
const ADMINFLAGTALKRESULT *AdminFlagTalkGetResult(void)
{
	return &s_Result;
}

/* Copyright(C)URARA-works 2026 */
