/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			サブゲーム(すごろく)用処理								 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2004/12/29												 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
int SubSugoroku_CheckJoin(PSUBGAME_SUGOROKUINFO, DWORD);				/* 参加しているかチェック */
void SubSugoroku_Remove(PSUBGAME_SUGOROKUINFO, DWORD);					/* ゲームから離脱 */
void SubSugoroku_RemoveAndMove(PSUBGAME_SUGOROKUINFO, DWORD);			/* ゲームから離脱して受付位置に移動 */
int SubSugoroku_AddMember(PSUBGAME_SUGOROKUINFO, BYTE, DWORD);			/* 参加 */
void SubSugoroku_Msg(PSUBGAME_SUGOROKUINFO, LPCSTR);					/* メッセージ */
void SubSugoroku_GetStartPos(PSUBGAME_SUGOROKUINFO, PDWORD, PDWORD);	/* スタート地点を取得 */
void SubSugoroku_GetGoalPos(PSUBGAME_SUGOROKUINFO, PDWORD, PDWORD);		/* ゴール地点を取得 */
void SubSugoroku_Quit(PSUBGAME_SUGOROKUINFO);							/* ゲーム終了 */
int SubSugoroku_GetNo(PSUBGAME_SUGOROKUINFO, DWORD);					/* テーブル番号を取得 */
int SubSugoroku_GetNextDirection(PSUBGAME_SUGOROKUINFO, BYTE);			/* 次の進行方向を取得 */
BOOL SubSugoroku_CheckBlock(PSUBGAME_SUGOROKUINFO, DWORD, DWORD);		/* ぶつかるかチェック */
void SubSugoroku_Move(PSUBGAME_SUGOROKUINFO, BYTE);						/* 移動 */
BOOL SubSugoroku_CheckParts(PSUBGAME_SUGOROKUINFO, BYTE);				/* パーツのチェック */
void SubSugoroku_NextPlayer(PSUBGAME_SUGOROKUINFO);						/* 次のプレイヤーに交替 */
BOOL SubSugoroku_CheckEvent(PSUBGAME_SUGOROKUINFO, BYTE);				/* イベントのチェック */
BOOL SubSugoroku_IsAllGoal(PSUBGAME_SUGOROKUINFO);						/* 全員ゴールしているか判定 */
void SubSugoroku_GoalProc(PSUBGAME_SUGOROKUINFO, BYTE);					/* ゴール処理 */

extern void sgenrand(unsigned long seed);							/* 乱数初期化 */
extern long genrand();													/* 乱数取得 */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */


/* ========================================================================= */
/* 関数名：SubSugoroku_CheckJoin											 */
/* 内容：参加しているかチェック												 */
/* 戻り値：-1:不参加 0:参加者 1:観戦者										 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
int SubSugoroku_CheckJoin(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	int i, nRet, nCount;

	nRet = -1;

	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pSugoroku->adwPlayerTblID[i] == dwTblID) {
			nRet = 0;
			goto Exit;
		}
	}
	nCount = pSugoroku->adwViewerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pSugoroku->adwViewerTblID[i] == dwTblID) {
			nRet = 1;
			goto Exit;
		}
	}

Exit:
	return nRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_Remove												 */
/* 内容：ゲームから離脱														 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SubSugoroku_Remove(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	int i, nTmp, nCount;

	nTmp = SubSugoroku_CheckJoin (pSugoroku, dwTblID);
	if (nTmp < 0) {
		return;
	}

	switch (nTmp) {
	case 0:		/* 参加者 */
		nCount = pSugoroku->adwPlayerTblID.GetSize ();
		for (i = 0; i < nCount; i ++) {
			if (pSugoroku->adwPlayerTblID[i] == dwTblID) {
				pSugoroku->adwPlayerTblID.RemoveAt (i);
				pSugoroku->aPlayerInfo.RemoveAt (i);
				/* 行動中のキャラ？ */
				if (pSugoroku->byProcNo == i) {
					pSugoroku->byProcNo --;
					SubSugoroku_NextPlayer (pSugoroku);

				} else if (pSugoroku->byProcNo > i) {
					pSugoroku->byProcNo --;
				}
				break;
			}
		}
		break;
	case 1:		/* 観戦者 */
		nCount = pSugoroku->adwViewerTblID.GetSize ();
		for (i = 0; i < nCount; i ++) {
			if (pSugoroku->adwViewerTblID[i] == dwTblID) {
				pSugoroku->adwViewerTblID.RemoveAt (i);
				break;
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SubSugoroku_RemoveAndMove										 */
/* 内容：ゲームから離脱して受付位置に移動									 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SubSugoroku_RemoveAndMove(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	int nTeam;
	char szMsg[256];
	PCHARINFO pChar;
	PSUBSTATUS_SUGOROKU pSubStatus;

	nTeam = SubSugoroku_CheckJoin (pSugoroku, dwTblID);
	/* 行動中チームのメンバー？ */

	SubSugoroku_Remove (pSugoroku, dwTblID);

	pChar = g_aCharInfo[dwTblID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	if ((nTeam == 0) && (pSugoroku->byState != SUBGAME_SUGOROKU_STATE_INIT)) {
		wsprintf (szMsg, "%sはゲームを抜けました。", pChar->szName);
		SubSugoroku_Msg (pSugoroku, szMsg);
	}
	if (nTeam != 1) {
		/* 受け付け地点へ飛ばす */
		SubProcMove (pChar, (DWORD)pSugoroku->byX, (DWORD)pSugoroku->byY);
	}

	/* 状態遷移 */
	pSubStatus->byState = SUBGAME_SUGOROKU_STATE_INIT;
	SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);

	/* 参加者がいなくなった？ */
	if (pSugoroku->adwPlayerTblID.GetSize () < 2) {
		SubSugoroku_Quit (pSugoroku);
	}

	ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));
}

/* ========================================================================= */
/* 関数名：SubSugoroku_AddMember											 */
/* 内容：参加																 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
int SubSugoroku_AddMember(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byMode, DWORD dwTblID)
{
	SUGOROKUPLAYERINFO Info;
	int nNo, nTmp;

	/* 参加？ */
	if (byMode == 0) {
		pSugoroku->adwPlayerTblID.Add (dwTblID);

		ZeroMemory (&Info, sizeof (Info));
		Info.dwTblID = dwTblID;
		SubSugoroku_GetStartPos (pSugoroku, &Info.dwX, &Info.dwY);
		nNo = pSugoroku->aPlayerInfo.Add (Info);
		nTmp = SubSugoroku_GetNextDirection (pSugoroku, pSugoroku->aPlayerInfo.GetSize () - 1);
		pSugoroku->aPlayerInfo[nNo].byDirection = (BYTE)nTmp;

	} else {
		pSugoroku->adwViewerTblID.Add (dwTblID);
	}

	return nNo;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_Msg													 */
/* 内容：メッセージ															 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SubSugoroku_Msg(PSUBGAME_SUGOROKUINFO pSugoroku, LPCSTR pszMsg)
{
	int i, nCount;
	char szTmp[256];
	PCHARINFO pChar;

	strcpy (szTmp, "SYSTEM:");
	strcat (szTmp, pszMsg);

	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[pSugoroku->adwPlayerTblID[i]];
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	}
	nCount = pSugoroku->adwViewerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[pSugoroku->adwViewerTblID[i]];
		SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	}
}

/* ========================================================================= */
/* 関数名：SubSugoroku_GetStartPos											 */
/* 内容：スタート地点を取得													 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SubSugoroku_GetStartPos(PSUBGAME_SUGOROKUINFO pSugoroku, PDWORD pdwX, PDWORD pdwY)
{
	int i, nCount;

	nCount = pSugoroku->aEventInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PSUGOROKUEVENTINFO pInfo;

		pInfo = &pSugoroku->aEventInfo[i];
		if (pInfo->byEventType != SUBGAME_SUGOROKU_EVENT_START) {
			continue;
		}
		*pdwX = pInfo->byX;
		*pdwY = pInfo->byY;
	}
}

/* ========================================================================= */
/* 関数名：SubSugoroku_GetGoalPos											 */
/* 内容：ゴール地点を取得													 */
/* 日付：2004/12/29															 */
/* ========================================================================= */
void SubSugoroku_GetGoalPos(PSUBGAME_SUGOROKUINFO pSugoroku, PDWORD pdwX, PDWORD pdwY)
{
	int i, nCount;

	nCount = pSugoroku->aEventInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PSUGOROKUEVENTINFO pInfo;

		pInfo = &pSugoroku->aEventInfo[i];
		if (pInfo->byEventType != SUBGAME_SUGOROKU_EVENT_GOAL) {
			continue;
		}
		*pdwX = pInfo->byX;
		*pdwY = pInfo->byY;
	}
}

/* ========================================================================= */
/* 関数名：SubSugoroku_Quit													 */
/* 内容：ゲーム終了															 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void SubSugoroku_Quit(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	int i, nCount;

	if (pSugoroku->byState == SUBGAME_SUGOROKU_STATE_INIT) {
		return;
	}

	/* 状態遷移 */
	pSugoroku->byState = SUBGAME_SUGOROKU_STATE_INIT;

	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SubSugoroku_RemoveAndMove (pSugoroku, pSugoroku->adwPlayerTblID[0]);
	}
	nCount = pSugoroku->adwViewerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SubSugoroku_RemoveAndMove (pSugoroku, pSugoroku->adwViewerTblID[0]);
	}
	pSugoroku->aPlayerInfo.RemoveAll ();
}

/* ========================================================================= */
/* 関数名：SubSugoroku_GetNo												 */
/* 内容：テーブル番号を取得													 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
int SubSugoroku_GetNo(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	int i, nCount, nRet;

	nRet = -1;
	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (dwTblID == pSugoroku->adwPlayerTblID[i]) {
			nRet = i;
			break;
		}
	}

	return nRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_GetNextDirection										 */
/* 内容：次の進行方向を取得													 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
int SubSugoroku_GetNextDirection(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byNo)
{
	PSUGOROKUPLAYERINFO pInfo;
	int i, nRet;

	nRet = 0;
	pInfo = &pSugoroku->aPlayerInfo[byNo];
	/* 4方向分ループ(分かれ道は未対応) */
	for (i = 0; i < 4; i ++) {
		BOOL bResult;

		/* 後ろ？ */
		if (i == abyDirectionTbl[pInfo->byDirection]) {
			/* ゴール地点？ */
			if ((pInfo->dwX == pSugoroku->dwGoalX) && (pInfo->dwY == pSugoroku->dwGoalY)) {
				switch (pSugoroku->byRule) {
				case SUBGAME_SUGOROKU_RULE_GOAL_STOP:			/* ゴール(ぴったり) */
				case SUBGAME_SUGOROKU_RULE_GOAL_NONSTOP:		/* ゴール(通過) */
					nRet = i;
					pInfo->byBack = 1;
					goto Exit;
				}
			}
			continue;
		}

		bResult = SubSugoroku_CheckBlock (
							pSugoroku,
							pInfo->dwX + anPosAddTblX[i],
							pInfo->dwY + anPosAddTblY[i]);
		if (bResult == FALSE) {
			continue;
		}
		nRet = i;
		break;
	}

Exit:
	return nRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_CheckBlock											 */
/* 内容：ぶつかるかチェック													 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
BOOL SubSugoroku_CheckBlock(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwX, DWORD dwY)
{
	int i, nCount;
	WORD wPartsID;
	BOOL bRet;
	PMAPINFO pMap;

	bRet = FALSE;
	pMap = g_aMapInfo[pSugoroku->dwMapID];
	nCount = pSugoroku->aPartsInfo.GetSize ();
	wPartsID = pMap->pwMap[dwX + dwY * pMap->nCX];

	for (i = 0; i < nCount; i ++) {
		PSUGOROKUPARTSINFO pInfo;

		pInfo = &pSugoroku->aPartsInfo[i];
		/* 登録されているパーツではない？ */
		if (wPartsID != pInfo->wPartsID) {
			continue;
		}
		bRet = TRUE;
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_Move													 */
/* 内容：移動																 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void SubSugoroku_Move(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byNo)
{
	PSUGOROKUPLAYERINFO pInfo;
	PCHARINFO pChar;
	DWORD dwX, dwY;
	BOOL bResult;

	pInfo = &pSugoroku->aPlayerInfo[byNo];
	pChar = g_aCharInfo[pInfo->dwTblID];

	dwX = pInfo->dwX + anPosAddTblX[pInfo->byDirection];
	dwY = pInfo->dwY + anPosAddTblY[pInfo->byDirection];
	bResult = SubSugoroku_CheckBlock (pSugoroku, dwX, dwY);
	if (bResult == FALSE) {
		return;
	}

	pInfo->dwX = dwX;
	pInfo->dwY = dwY;

	pChar->byDirection = pInfo->byDirection;
	pChar->dwX = pInfo->dwX;
	pChar->dwY = pInfo->dwY;
	pChar->byFlg |= CHARINFOFLG_MOVE;
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
}

/* ========================================================================= */
/* 関数名：SubSugoroku_CheckParts											 */
/* 内容：パーツのチェック													 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
BOOL SubSugoroku_CheckParts(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byNo)
{
	int i, nCount, nTmp;
	WORD wPartsID;
	BYTE byTmp;
	BOOL bRet;
	char szTmp[256];
	PSUGOROKUPLAYERINFO pPlayerInfo;
	PSUGOROKUPARTSINFO pInfo;
	PCHARINFO pChar;
	PMAPINFO pMap;

	bRet		= TRUE;
	pInfo		= NULL;
	pPlayerInfo	= &pSugoroku->aPlayerInfo[byNo];
	pChar		= g_aCharInfo[pPlayerInfo->dwTblID];
	pMap		= g_aMapInfo[pSugoroku->dwMapID];
	nCount		= pSugoroku->aPartsInfo.GetSize ();
	wPartsID	= pMap->pwMap[pPlayerInfo->dwX + pPlayerInfo->dwY * pMap->nCX];

	if (pPlayerInfo->byNoCheck) {
		goto Exit;
	}

	for (i = 0; i < nCount; i ++) {
		pInfo = &pSugoroku->aPartsInfo[i];
		/* 登録されているパーツではない？ */
		if (wPartsID != pInfo->wPartsID) {
			continue;
		}
		break;
	}
	/* 見つからなかった？ */
	if (i >= nCount) {
		goto Exit;
	}

	switch (pInfo->byType) {
	case SUBGAME_SUGOROKU_PARTS_EVENT:			/* イベント */
		bRet = SubSugoroku_CheckEvent (pSugoroku, byNo);
		break;

	case SUBGAME_SUGOROKU_PARTS_BACKSTART:		/* スタートに戻る */
		SubSugoroku_Msg (pSugoroku, "スタート地点に戻ります。");

		SubSugoroku_GetStartPos (pSugoroku, &pPlayerInfo->dwX, &pPlayerInfo->dwY);
		nTmp = SubSugoroku_GetNextDirection (pSugoroku, byNo);
		pPlayerInfo->byDirection = (BYTE)nTmp;

		/* 開始地点へ飛ばす */
		SendCmdFADEINOUT (pChar->pID, 0);
		pChar->byDirection = pPlayerInfo->byDirection;
		pChar->byDirectionBack2 = pChar->byDirection;
		pChar->byDirectionBack = pChar->byDirection;

		CharTargetCancelPC (pChar);
		pChar->dwX = pPlayerInfo->dwX;
		pChar->dwY = pPlayerInfo->dwY;
		pChar->dwXBack = pChar->dwX;
		pChar->dwYBack = pChar->dwY;
		SendCmdCHARINFO (pChar->pID, -1);
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
		SendCmdFADEINOUT (pChar->pID, 1);
		break;

	case SUBGAME_SUGOROKU_PARTS_GO_1:			/* 1マス進む */
	case SUBGAME_SUGOROKU_PARTS_GO_2:			/* 2マス進む */
	case SUBGAME_SUGOROKU_PARTS_GO_3:			/* 3マス進む */
	case SUBGAME_SUGOROKU_PARTS_GO_4:			/* 4マス進む */
	case SUBGAME_SUGOROKU_PARTS_GO_5:			/* 5マス進む */
	case SUBGAME_SUGOROKU_PARTS_GO_6:			/* 6マス進む */
		bRet = FALSE;
		pPlayerInfo->byNoCheck = 1;
		byTmp = pInfo->byType - SUBGAME_SUGOROKU_PARTS_GO_1 + 1;
		pSugoroku->byLastMove = byTmp;
		wsprintf (szTmp, "%dマス進みます。", byTmp);
		SubSugoroku_Msg (pSugoroku, szTmp);
		break;

	case SUBGAME_SUGOROKU_PARTS_BACK_1:			/* 1マス戻る */
	case SUBGAME_SUGOROKU_PARTS_BACK_2:			/* 2マス戻る */
	case SUBGAME_SUGOROKU_PARTS_BACK_3:			/* 3マス戻る */
	case SUBGAME_SUGOROKU_PARTS_BACK_4:			/* 4マス戻る */
	case SUBGAME_SUGOROKU_PARTS_BACK_5:			/* 5マス戻る */
	case SUBGAME_SUGOROKU_PARTS_BACK_6:			/* 6マス戻る */
		bRet = FALSE;
		pPlayerInfo->byBack = 1;
		pPlayerInfo->byNoCheck = 1;
		pPlayerInfo->byDirection = abyDirectionTbl[pPlayerInfo->byDirection];
		byTmp = pInfo->byType - SUBGAME_SUGOROKU_PARTS_BACK_1 + 1;
		pSugoroku->byLastMove = byTmp;
		wsprintf (szTmp, "%dマス戻ります。", byTmp);
		SubSugoroku_Msg (pSugoroku, szTmp);
		break;

	case SUBGAME_SUGOROKU_PARTS_WAIT:			/* 1回休み */
		pPlayerInfo->byWait = 1;
		SubSugoroku_Msg (pSugoroku, "１回休みです。");
		break;
	}

Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_NextPlayer											 */
/* 内容：次のプレイヤーに交替												 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void SubSugoroku_NextPlayer(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	BOOL bResult;

	pSugoroku->byProcNo ++;
	if (pSugoroku->byProcNo >= pSugoroku->adwPlayerTblID.GetSize ()) {
		pSugoroku->byProcNo = 0;
	}

	bResult = SubSugoroku_IsAllGoal (pSugoroku);
	if (bResult == FALSE) {
		/* 状態遷移 */
		pSugoroku->byState = SUBGAME_SUGOROKU_STATE_COMMAND;
	}
}

/* ========================================================================= */
/* 関数名：SubSugoroku_CheckEvent											 */
/* 内容：イベントのチェック													 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
BOOL SubSugoroku_CheckEvent(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byNo)
{
	int i, nCount;
	BOOL bRet;
	PSUGOROKUPLAYERINFO pPlayerInfo;
	PSUGOROKUEVENTINFO pInfo;
	PCHARINFO pChar;
	PMAPINFO pMap;

	bRet		= FALSE;
	pPlayerInfo	= &pSugoroku->aPlayerInfo[byNo];
	pChar		= g_aCharInfo[pPlayerInfo->dwTblID];
	pMap		= g_aMapInfo[pSugoroku->dwMapID];
	nCount		= pSugoroku->aEventInfo.GetSize ();


	for (i = 0; i < nCount; i ++) {
		pInfo = &pSugoroku->aEventInfo[i];
		/* 座標が違う？ */
		if (!((pInfo->byX == (BYTE)pPlayerInfo->dwX) && (pInfo->byY == (BYTE)pPlayerInfo->dwY))) {
			continue;
		}
		break;
	}
	/* 見つからなかった？ */
	if (i >= nCount) {
		goto Exit;
	}

	switch (pInfo->byEventType) {
	case SUBGAME_SUGOROKU_EVENT_START:			/* スタート */
		break;

	case SUBGAME_SUGOROKU_EVENT_GOAL:			/* ゴール */
		bRet = TRUE;
		SubSugoroku_GoalProc (pSugoroku, byNo);
		break;

	case SUBGAME_SUGOROKU_EVENT_GO:				/* 進む */
	case SUBGAME_SUGOROKU_EVENT_BACK:			/* 戻る */
	case SUBGAME_SUGOROKU_EVENT_BACKSTART:		/* スタートに戻る */
	case SUBGAME_SUGOROKU_EVENT_WAIT:			/* 休み */
		break;
	}

Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_IsAllGoal											 */
/* 内容：全員ゴールしているか判定											 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
BOOL SubSugoroku_IsAllGoal(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	int i, nCount, nCountGoal;
	BOOL bRet;

	bRet = FALSE;
	nCountGoal = 0;
	nCount = pSugoroku->aPlayerInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pSugoroku->aPlayerInfo[i].byGoal) {
			nCountGoal ++;
		}
	}

	/* 全員ゴールしている？ */
	if (nCountGoal == nCount) {
		bRet = TRUE;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：SubSugoroku_GoalProc												 */
/* 内容：ゴール処理															 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void SubSugoroku_GoalProc(PSUBGAME_SUGOROKUINFO pSugoroku, BYTE byNo)
{
	char szTmp[256];
	BOOL bResult;
	PSUGOROKUPLAYERINFO pPlayerInfo;
	PCHARINFO pChar;
	PMAPINFO pMap;

	pPlayerInfo	= &pSugoroku->aPlayerInfo[byNo];
	pChar		= g_aCharInfo[pPlayerInfo->dwTblID];
	pMap		= g_aMapInfo[pSugoroku->dwMapID];

	pPlayerInfo->byGoal = 1;
	wsprintf (szTmp, "%sがゴールしました！", pChar->szName);
	SubSugoroku_Msg (pSugoroku, szTmp);
	bResult = SubSugoroku_IsAllGoal (pSugoroku);
	/* 全員ゴールした？ */
	if (bResult) {
		SubSugoroku_Msg (pSugoroku, "ゲーム終了！");
		SubSugoroku_Quit (pSugoroku);
	}
}

/* Copyright(C)URARA-works 2003-2005 */
