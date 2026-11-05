/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGameWAR.cpp											 */
/* 内容：		サブゲーム処理(戦場ゲーム)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/19													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../../../Common/Info/SubGame/InfoSubGameWAR.h"
#include "../../SboSvrRecvCmd.h"
#include "ProcSubGameWAR.h"


/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define SUBWAR_WAITTIME			(10)								/* 待ち時間 */
#define SUBWAR_WAITTIME_JOIN	(60)								/* 待ち時間(参加受付) */


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

extern void sgenrand(unsigned long seed);				/* 乱数初期化 */
extern long genrand();									/* 乱数取得 */


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::CProcSubGameWAR								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

CProcSubGameWAR::CProcSubGameWAR()
{
	m_pInfo = NULL;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::~CProcSubGameWAR								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

CProcSubGameWAR::~CProcSubGameWAR()
{
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

void CProcSubGameWAR::Init(
	BYTE byID,						/* [in] サブゲームID */
	CInfoSubGameBase *pInfo)		/* [in] サブゲーム情報 */
{
	CProcSubGameBase::Init (byID, pInfo);

	m_pInfo = (PCInfoSubGameWAR)pInfo;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::Proc(void)
{
	switch (m_pInfo->m_byState) {
	case SUBGAME_WAR_STATE_INIT:		ProcINIT		();		break;	/* 初期状態 */
	case SUBGAME_WAR_STATE_CLEAN:		ProcCLEAN		();		break;	/* 初期化 */
	case SUBGAME_WAR_STATE_READY:		ProcREADY		();		break;	/* 準備完了 */
	case SUBGAME_WAR_STATE_JOIN:		ProcJOIN		();		break;	/* 参加者募集中 */
	case SUBGAME_WAR_STATE_SELECTRULE:	ProcSELECTRULE	();		break;	/* ルール選択 */
	case SUBGAME_WAR_STATE_SELECTJOB:	ProcSELECTJOB	();		break;	/* 職業選択 */
	case SUBGAME_WAR_STATE_SELECTTURN:	ProcSELECTTURN	();		break;	/* 順番設定 */
	case SUBGAME_WAR_STATE_GAMEPREPARE:	ProcGAMEPREPARE	();		break;	/* 試合準備 */
	case SUBGAME_WAR_STATE_GAME:		ProcGAME		();		break;	/* 試合中 */
	case SUBGAME_WAR_STATE_QUIT:		ProcQUIT		();		break;	/* 終了 */
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::IsInformationPos								 */
/* 内容：	指定座標が受付位置か判定										 */
/* 日付：	2006/04/07														 */
/* ========================================================================= */

BOOL CProcSubGameWAR::IsInformationPos(
	DWORD dwMapID,		/* [in] マップID */
	BYTE byX,			/* [in] 座標(横) */
	BYTE byY)			/* [in] 座標(縦) */
{
	BOOL bRet;

	bRet = FALSE;

	/* 受付座標が一致？ */
	if ((m_pInfo->m_dwMapID			== dwMapID)	&&
		(m_pInfo->m_ptInformation.x	== byX)		&&
		(m_pInfo->m_ptInformation.y	== byY)) {
		bRet = TRUE;
	}

	return bRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetType										 */
/* 内容：	サブゲーム種別を取得											 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

int CProcSubGameWAR::GetType(void)
{
	return SUBGAMEID_WAR;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Rand											 */
/* 内容：	乱数発生														 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

int CProcSubGameWAR::Rand(void)
{
	int nRet;

	nRet = 0;

	switch (genrand () % 7) {
	case 0:	nRet = 3;	break;
	case 1:
	case 2:	nRet = 2;	break;
	case 3:
	case 4:	nRet = 1;	break;
	case 5:	nRet = 0;	break;
	case 6:	nRet = 4;	break;
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetMemberCount									 */
/* 内容：	参加人数を取得													 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

int CProcSubGameWAR::GetMemberCount(int nTeam)
{
	return m_pInfo->GetMemberCount (nTeam);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetAllMemberCount								 */
/* 内容：	全参加人数を取得												 */
/* 日付：	2006/04/24														 */
/* ========================================================================= */

int CProcSubGameWAR::GetAllMemberCount(void)
{
	return m_pInfo->GetAllMemberCount ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetMemberType									 */
/* 内容：	1チームの最大参加人数を取得										 */
/* 日付：	2006/04/24														 */
/* ========================================================================= */

int CProcSubGameWAR::GetMemberType(void)
{
	int nRet;

	nRet = 3;
	switch (m_pInfo->m_byMember) {
	case SUBGAME_WAR_MEMBER_3:				/* ３人 */
		nRet = 3;
		break;
	case SUBGAME_WAR_MEMBER_4:				/* ４人 */
		nRet = 4;
		break;
	}
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::IsJoin											 */
/* 内容：	参加しているかチェック											 */
/* 戻り値：	-1:不参加 0:チーム１ 1:チーム２									 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

int CProcSubGameWAR::IsJoin(DWORD dwTblID)
{
	return m_pInfo->GetTeamNo (dwTblID);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetState										 */
/* 内容：	状態を取得														 */
/* 日付：	2006/04/24														 */
/* ========================================================================= */

int CProcSubGameWAR::GetState(void)
{
	return m_pInfo->m_byState;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ChgState										 */
/* 内容：	状態を変更														 */
/* 日付：	2006/04/26														 */
/* ========================================================================= */

void CProcSubGameWAR::ChgState(int nState)
{
	m_pInfo->m_byState = nState;

	switch (m_pInfo->m_byState) {
	case SUBGAME_WAR_STATE_INIT:		break;	/* 初期状態 */
	case SUBGAME_WAR_STATE_CLEAN:		break;	/* 初期化 */
	case SUBGAME_WAR_STATE_READY:		break;	/* 準備完了 */
	case SUBGAME_WAR_STATE_JOIN:				/* 参加者募集中 */
		m_pInfo->m_dwLastTimeProc	= 0;
		m_pInfo->m_dwFirstTimeProc	= timeGetTime ();
		m_pInfo->m_dwNextTimeProc	= m_pInfo->m_dwFirstTimeProc + 10 * 1000;
		break;
	case SUBGAME_WAR_STATE_SELECTRULE:	break;	/* ルール選択 */
	case SUBGAME_WAR_STATE_SELECTJOB:	break;	/* 職業選択 */
	case SUBGAME_WAR_STATE_SELECTTURN:	break;	/* 順番設定 */
	case SUBGAME_WAR_STATE_GAMEPREPARE:	break;	/* 試合準備 */
	case SUBGAME_WAR_STATE_GAME:		break;	/* 試合中 */
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetRule										 */
/* 内容：	ルールを取得													 */
/* 日付：	2006/04/27														 */
/* ========================================================================= */

BYTE CProcSubGameWAR::GetRule(void)
{
	return m_pInfo->m_byRule;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::SetRule										 */
/* 内容：	ルールを設定													 */
/* 日付：	2006/04/27														 */
/* ========================================================================= */

void CProcSubGameWAR::SetRule(BYTE byRule)
{
	m_pInfo->m_byRule = byRule;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetStartPos									 */
/* 内容：	開始座標を取得													 */
/* 日付：	2006/04/26														 */
/* ========================================================================= */

void CProcSubGameWAR::GetStartPos(BYTE byTeam, POINT &ptDst)
{
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo = m_pInfo->GetTeamInfo (byTeam);
	if (pTeamInfo == NULL) {
		return;
	}

	ptDst = pTeamInfo->ptStart;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Remove											 */
/* 内容：	ゲームから離脱													 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void CProcSubGameWAR::Remove(DWORD dwTblID)
{
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PCHARINFO pChar;

	pMemberInfo = m_pInfo->GetMemberInfo (dwTblID);
	/* 不参加？ */
	if (pMemberInfo == NULL) {
		return;
	}

	if (pMemberInfo->dwTblIDHP != -1) {
		/* HPゲージの名前を消去 */
		pChar = g_aCharInfo[pMemberInfo->dwTblIDHP];
		pChar->byEfect = 5;
		ZeroMemory (pChar->szName, sizeof (pChar->szName));
		SendCmdCHARINFO3 (pChar->dwTblID);
	}

	m_pInfo->DeleteMember (dwTblID);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::RemoveAndMove									 */
/* 内容：	ゲームから離脱して受付位置に移動								 */
/* 日付：	2004/08/11														 */
/* ========================================================================= */

void CProcSubGameWAR::RemoveAndMove(DWORD dwTblID)
{
	int nTeam, nCount;
	char szMsg[256];
	DWORD dwTurnTblID;
	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;

	nTeam = m_pInfo->GetTeamNo (dwTblID);
	if (nTeam != -1) {
		dwTurnTblID	= m_pInfo->GetTurnTblID (nTeam);
		/* 行動中チームのメンバー？ */
		if (m_pInfo->m_byTurn == nTeam) {
			if (dwTurnTblID == dwTblID) {
				NextTurn ();
			}
		}
	}

	Remove (dwTblID);

	pChar		= g_aCharInfo[dwTblID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	if (nTeam != -1) {
		if (m_pInfo->m_byState != SUBGAME_WAR_STATE_INIT) {
			wsprintf (szMsg, "%sは戦場ゲームを抜けました。", pChar->szName);
			JudgeMsg (szMsg);
		}
	}

	pChar->wGrpTmpID	= 0;
	pChar->wGrpTmpMain	= 0;
	pChar->wGrpTmpSub	= 0;

	/* 状態遷移 */
	pSubStatus->byState = SUBGAME_WAR_STATE_INIT;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	if (nTeam != -1) {
		/* 受け付け地点へ飛ばす */
		SubProcMove (pChar, (DWORD)m_pInfo->m_ptInformation.x, (DWORD)m_pInfo->m_ptInformation.y);

		nCount = GetMemberCount (nTeam);
		/* 参加者がいなくなった？ */
		if ((nCount == 0) || (m_pInfo->GetAllMemberCount () <= 1)) {
			Quit ();
		}
	}

	ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::AddMember										 */
/* 内容：	参加															 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void CProcSubGameWAR::AddMember(BYTE byTeam, DWORD dwTblID)
{
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	pMemberInfo = m_pInfo->GetNewMemberInfo ();
	pMemberInfo->dwTblID = dwTblID;

	m_pInfo->AddMember (byTeam, pMemberInfo);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::JudgeMsg										 */
/* 内容：	審判キャラ発言													 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void CProcSubGameWAR::JudgeMsg(LPCSTR pszMsg)
{
	char szTmp[256];

	strcpy (szTmp, "SYSTEM:");
	strcat (szTmp, pszMsg);

	SendCmdSYSMSG (m_pInfo->m_dwTblIDJudge, szTmp, SPEECHCMD_SCREEN);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::AllChgState									 */
/* 内容：	参加者全員の状態変更											 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void CProcSubGameWAR::AllChgState(BYTE byState)
{
	int i, j, nCountMember, nCountTeam;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBSTATUS_WAR pSubStatus;
	PCHARINFO pChar;

	nCountTeam = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCountTeam; i ++) {
		nCountMember = m_pInfo->GetMemberCount (i);
		for (j = 0; j < nCountMember; j ++) {
			pMemberInfo = m_pInfo->GetMemberInfo (i, j);
			if (pMemberInfo == NULL) {
				continue;
			}

			pChar = g_aCharInfo[pMemberInfo->dwTblID];
			pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;
			pSubStatus->byState = byState;
			SendCmdSubWarCHGSTATE (pChar->pID, byState);
		}
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::AllChgStateAndQuit								 */
/* 内容：	参加者全員の状態変更(条件付)									 */
/* 日付：	2004/08/11														 */
/* ========================================================================= */

void CProcSubGameWAR::AllChgStateAndQuit(BYTE byState, BYTE byCheckState)
{
	int i, j, nCountMember, nCountTeam;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBSTATUS_WAR pSubStatus;
	PCHARINFO pChar;

	nCountTeam = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCountTeam; i ++) {
		nCountMember = m_pInfo->GetMemberCount (i);
		for (j = 0; j < nCountMember; j ++) {
			pMemberInfo = m_pInfo->GetMemberInfo (i, j);
			if (pMemberInfo == NULL) {
				continue;
			}

			pChar = g_aCharInfo[pMemberInfo->dwTblID];
			pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;
			if (pSubStatus->byState != byCheckState) {
				RemoveAndMove (pChar->dwTblID);
				j --;
			} else {
				pSubStatus->byState = byState;
				SendCmdSubWarCHGSTATE (pChar->pID, byState);
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::UpTurn											 */
/* 内容：	順番を一つ手前にずらす											 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void CProcSubGameWAR::UpTurn(BYTE byTeam, BYTE byNo)
{
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	/* 一番上？ */
	if (byNo == 0) {
		return;
	}

	pTeamInfo = m_pInfo->GetTeamInfo (byTeam);
	if (pTeamInfo == NULL) {
		return;
	}

	/* 1つ上と入れ替え */
	pMemberInfo = pTeamInfo->aMemberInfo[byNo - 1];
	pTeamInfo->aMemberInfo[byNo - 1] = pTeamInfo->aMemberInfo[byNo];
	pTeamInfo->aMemberInfo[byNo] = pMemberInfo;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::PrePare										 */
/* 内容：	準備処理														 */
/* 日付：	2004/08/11														 */
/* ========================================================================= */

void CProcSubGameWAR::PrePare(void)
{
	char szTmp[MAX_CHARNAME + 1];
	int i, j, k, nCountMember, nCountTeam;
	DWORD dwTblID;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBSTATUS_WAR pSubStatus;
	PCHARINFO pChar, pCharTmp;

	nCountTeam = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCountTeam; i ++) {
		pTeamInfo = m_pInfo->GetTeamInfo (i);

		/* HPと蘇生カウンタキャラの情報を初期化 */
		nCountMember = GetMemberType ();
		for (j = 0; j < nCountMember; j ++) {
			dwTblID = CheckCharBlockNPC (
							m_pInfo->m_dwMapID,
							pTeamInfo->ptHP.x + j,
							pTeamInfo->ptHP.y);
			if (dwTblID != -1) {
				pCharTmp = g_aCharInfo[dwTblID];
				ZeroMemory (pCharTmp->szName, sizeof (pCharTmp->szName));
				pCharTmp->byEfect = 5;
				SendCmdCHARINFO3 (pCharTmp->dwTblID);
			}
			dwTblID = CheckCharBlockNPC (
							m_pInfo->m_dwMapID,
							pTeamInfo->ptResurrection.x + j,
							pTeamInfo->ptResurrection.y);
			if (dwTblID != -1) {
				pCharTmp = g_aCharInfo[dwTblID];
				ZeroMemory (pCharTmp->szName, sizeof (pCharTmp->szName));
				pCharTmp->byEfect = 0;
				SendCmdCHARINFO3 (pCharTmp->dwTblID);
			}
		}

		/* 参加メンバー用の情報を初期化 */
		nCountMember = m_pInfo->GetMemberCount (i);
		for (j = 0; j < nCountMember; j ++) {
			pMemberInfo = m_pInfo->GetMemberInfo (i, j);
			if (pMemberInfo == NULL) {
				continue;
			}

			if (j == 0) {
				pTeamInfo->dwTurnTblID = pMemberInfo->dwTblID;
			}

			pChar = g_aCharInfo[pMemberInfo->dwTblID];
			pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;
			pChar->wGrpTmpMain	= CHARGRP_MAIN_SUBWAR;
			pChar->wGrpTmpSub	= 3;
			pChar->wGrpTmpID	= pSubStatus->byJob * 2;
			/* 女性？ */
			if (pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE) {
				pChar->wGrpTmpID ++;
			}
			pChar->wGrpTmpID += (i * 8);

			/* HPキャラのテーブルIDを取得 */
			pMemberInfo->dwTblIDHP = CheckCharBlockNPC (
					m_pInfo->m_dwMapID,
					pTeamInfo->ptHP.x + j,
					pTeamInfo->ptHP.y);

			/* 名前と色のコピー */
			pCharTmp = g_aCharInfo[pMemberInfo->dwTblIDHP];
			/* 名前の先頭を表示幅 4（全角 2 文字）まで */
			ZeroMemory (szTmp, sizeof (szTmp));
			CopyMemory (szTmp, pChar->szName, min (SboTextFitWidth (pChar->szName, 4), (int)sizeof (szTmp) - 1));
			pCharTmp->byEfect = 0;
			wsprintf (pCharTmp->szName, "[%s]", szTmp);
			pCharTmp->clName	= pChar->clName;
			pCharTmp->clSpeech	= pChar->clSpeech;
			SendCmdCHARINFO3 (pCharTmp->dwTblID);

			/* 初期位置へ移動 */
			SubProcMove (pChar, pTeamInfo->ptStart.x, pTeamInfo->ptStart.y + j);
		}
	}

	/* 順番待ちへ状態遷移 */
	AllChgState (SUBGAME_WAR_STATE_WAIT);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ChgTurn										 */
/* 内容：	ターン変更														 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void CProcSubGameWAR::ChgTurn(void)
{
	char szTmp[128];
	PSUBSTATUS_WAR pSubStatus;
	PCHARINFO pChar, pCharHP;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	if (m_pInfo->m_byState != SUBGAME_WAR_STATE_GAME) {
		return;
	}

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	pMemberInfo	= m_pInfo->GetMemberInfo (pTeamInfo->dwTurnTblID);
	if (pMemberInfo == NULL) {
		return;
	}

	pChar		= g_aCharInfo[pMemberInfo->dwTblID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* やられ中？ */
	if (pMemberInfo->dwTblIDRes != -1) {
		pCharHP = g_aCharInfo[pMemberInfo->dwTblIDHP];
		Resuscitation (pChar->dwTblID, FALSE);
		NextTurn ();

	} else {
		if (m_pInfo->m_byTurn == 0) {
			wsprintf (szTmp, "赤チーム[%s]の行動です。", pChar->szName);
		} else {
			wsprintf (szTmp, "青チーム[%s]の行動です。", pChar->szName);
		}
		JudgeMsg (szTmp);

		pChar->wGrpTmpSub = 1;
		SendCmdCHARINFO3 (pChar->dwTblID);

		/* 行動選択に状態遷移 */
		pSubStatus->byState = SUBGAME_WAR_STATE_TURN;
		SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

		SendCmdPLAYSOUND (pChar->pID, SOUND_SHOOT15);
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::NextTurn										 */
/* 内容：	次のキャラへ行動権限変更										 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void CProcSubGameWAR::NextTurn(void)
{
	int nCount;
	DWORD dwTblID;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PCHARINFO pChar;

	if (m_pInfo->m_byState != SUBGAME_WAR_STATE_GAME) {
		return;
	}

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	pMemberInfo	= m_pInfo->GetMemberInfo (m_pInfo->m_byTurn, pTeamInfo->byTurn);
	nCount		= m_pInfo->GetMemberCount (m_pInfo->m_byTurn);
	dwTblID		= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	pTeamInfo->byTurn ++;
	if (pTeamInfo->byTurn >= nCount) {
		pTeamInfo->byTurn = 0;

		if (pMemberInfo == NULL) {
			pMemberInfo	= m_pInfo->GetMemberInfo (m_pInfo->m_byTurn, pTeamInfo->byTurn);
			dwTblID	= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);
		}

		/* 行動チーム入れ替え */
		m_pInfo->m_byTurn ++;
		if (m_pInfo->m_byTurn >= m_pInfo->GetTeamCount ()) {
			m_pInfo->m_byTurn = 0;
		}
	}
	if (pMemberInfo) {
		/* 次のキャラへ移る前に画像を変更しておく */
		pChar = g_aCharInfo[dwTblID];
		if (pMemberInfo->dwTblIDRes == -1) {
			pChar->wGrpTmpSub = 3;
		}
		SendCmdCHARINFO3 (pChar->dwTblID);
	}

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	pMemberInfo = m_pInfo->GetMemberInfo (m_pInfo->m_byTurn, pTeamInfo->byTurn);
	if (pMemberInfo == NULL) {
		return;
	}

	pTeamInfo->dwTurnTblID = pMemberInfo->dwTblID;
	ChgTurn ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::HPUp											 */
/* 内容：	HP回復															 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void CProcSubGameWAR::HPUp(DWORD dwTblID, BYTE byPoint)
{
	char szMsg[256];
	PCHARINFO pCharTmp;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	pMemberInfo = m_pInfo->GetMemberInfo (dwTblID);
	if (pMemberInfo == NULL) {
		return;
	}

	pCharTmp = g_aCharInfo[pMemberInfo->dwTblIDHP];
	if (pCharTmp->byEfect == 0) {
		return;
	}

	if (pCharTmp->byEfect >= byPoint) {
		pCharTmp->byEfect -= byPoint;
	} else {
		pCharTmp->byEfect = 0;
	}
	SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);

	pCharTmp = g_aCharInfo[dwTblID];
	pCharTmp->byEfect = EFECTTYPE_HOIMI;
	SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);

	if (byPoint == 5) {
		wsprintf (szMsg, "%sのHPは完全回復した！", pCharTmp->szName, byPoint);
	} else {
		wsprintf (szMsg, "%sのHPが %d ポイント回復した！", pCharTmp->szName, byPoint);
	}
	JudgeMsg (szMsg);

	wsprintf (szMsg, "%d", byPoint);
	SendCmdADDCHAREFC (pCharTmp->dwTblID, RGB (139, 213, 255), szMsg);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Atack											 */
/* 内容：	攻撃															 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void CProcSubGameWAR::Atack(DWORD dwTblIDTarget, BYTE byDifence)
{
	char szMsg[256];
	int i, nTmp, nTeamTarget, nPoint, nPointTarget, nCount;
	DWORD dwTblIDCounter;
	PCHARINFO pChar, pCharTarget, pCharJudge, pCharTmp;
	PSUBSTATUS_WAR pSubStatus, pSubStatusTarget;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo, pMemberInfoTarget;
	PSUBGAME_WAR_TEAMINFO pTeamInfo, pTeamInfoTarget;

	pTeamInfo			= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	pMemberInfo			= m_pInfo->GetMemberInfo (pTeamInfo->dwTurnTblID);
	pMemberInfoTarget	= m_pInfo->GetMemberInfo (dwTblIDTarget);

	nPointTarget = -1;
	pChar				= g_aCharInfo[pMemberInfo->dwTblID];
	pSubStatus			= (PSUBSTATUS_WAR)pChar->abySubStatus;
	pCharTarget			= g_aCharInfo[pMemberInfoTarget->dwTblID];
	pSubStatusTarget	= (PSUBSTATUS_WAR)pCharTarget->abySubStatus;
	pCharJudge			= g_aCharInfo[m_pInfo->m_dwTblIDJudge];

	/* 攻撃側判定 */
	nPoint			= Rand ();
	nTeamTarget		= m_pInfo->GetTeamNo (pMemberInfoTarget->dwTblID);
	pTeamInfoTarget	= m_pInfo->GetTeamInfo (nTeamTarget);

	pCharJudge->byEfect = nPoint;
	SendCmdEFECTCHG (pCharJudge->pID, pCharJudge->dwTblID, pCharJudge->byEfect);

	switch (pSubStatus->byJob) {
	case SUBGAME_WAR_JOB_FIGHTER:			/* 戦士 */
		switch (nPoint) {
		case 4:		/* 全力攻撃 */
			wsprintf (szMsg, "%sの全力攻撃！", pChar->szName);
			JudgeMsg (szMsg);
			break;
		}
		break;

	case SUBGAME_WAR_JOB_MARTIAL:			/* 武道家 */
		switch (nPoint) {
		case 4:		/* 回し蹴り */
			wsprintf (szMsg, "%sの回し蹴り！", pChar->szName);
			JudgeMsg (szMsg);

			nCount = m_pInfo->GetMemberCount (nTeamTarget);
			for (i = 0; i < nCount; i ++) {
				pMemberInfo = m_pInfo->GetMemberInfo (nTeamTarget, i);
				if (pMemberInfo == NULL) {
					continue;
				}

				pCharTmp = g_aCharInfo[pMemberInfo->dwTblIDHP];
				if (pCharTmp->byEfect >= 5) {
					continue;
				}
				pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
				/* 周囲１マス以内にいる？ */
				if (abs (pCharTmp->dwX - pChar->dwX) <= 1) {
					if (abs (pCharTmp->dwY - pChar->dwY) <= 1) {
						Damage (pCharTmp->dwTblID, 2, 1);
					}
				}
			}
			goto Exit;
		}
		break;

	case SUBGAME_WAR_JOB_WITCH:				/* 魔法使い */
	case SUBGAME_WAR_JOB_PRIEST:			/* 僧侶 */
		switch (nPoint) {
		case 4:		/* ミス */
			nPoint = 0;
			break;
		}
		break;
	}

	switch (nPoint) {
	case 0:			/* ミス */
		wsprintf (szMsg, "ミス！%sはダメージを受けない", pCharTarget->szName);
		JudgeMsg (szMsg);
		goto Exit;
	case 1:
	case 2:
	case 3:
		break;
	}

	switch (byDifence) {
	case SUBGAME_WAR_DIFENCE_NORMAL:		/* 通常防御 */
		nPoint --;
		wsprintf (szMsg, "%sの通常防御", pCharTarget->szName);
		JudgeMsg (szMsg);
		break;

	case SUBGAME_WAR_DIFENCE_AVOID:			/* 見切り */
		{
			int nTmp;

			nTmp = Rand ();
			switch (nTmp) {
			case 4:
				nPoint = 0;
				wsprintf (szMsg, "%sの見切り！", pCharTarget->szName);
				JudgeMsg (szMsg);
				break;
			case 1:
			case 2:
			case 3:
				wsprintf (szMsg, "%sの反撃！", pCharTarget->szName);
				JudgeMsg (szMsg);
				Damage (pChar->dwTblID, (BYTE)nTmp, 1);
				break;
			}
		}
		break;

	case SUBGAME_WAR_DIFENCE_PASSIVE:		/* 受け身 */
		{
			int nTmp;

			nTmp = Rand ();
			switch (nTmp) {
			case 1:
			case 2:
			case 3:
				wsprintf (szMsg, "%sの受け身！", pCharTarget->szName);
				JudgeMsg (szMsg);
				nPoint -= nTmp;
				break;
			}
		}
		break;

	case SUBGAME_WAR_DIFENCE_COUNTER:		/* カウンター */
		nTmp = Rand ();
		if (nTmp >= 2) {
			if (nPoint == nTmp) {
				break;
			}
			wsprintf (szMsg, "%sのカウンター！", pCharTarget->szName);
			JudgeMsg (szMsg);

			if (nTmp == 4) {
				nPointTarget = nPoint;
			} else {
				nPoint -= nTmp;
				nPointTarget = nTmp;
			}
			dwTblIDCounter = pChar->dwTblID;
		}
		break;
	}

	/* ダメージなし？ */
	if (nPoint <= 0) {
		wsprintf (szMsg, "%sの攻撃は効かなかった", pChar->szName);
		JudgeMsg (szMsg);

	/* ダメージあり？ */
	} else if (nPoint > 0) {
		Damage (dwTblIDTarget, (BYTE)nPoint, 1);
	}

	if (nPointTarget > 0) {
		Damage (dwTblIDCounter, (BYTE)nPointTarget, 1);
	}

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Damage											 */
/* 内容：	ダメージ処理													 */
/* 日付：	2004/08/14														 */
/* ========================================================================= */

void CProcSubGameWAR::Damage(DWORD dwTblIDTarget, BYTE byDamage, BYTE byEfect)
{
	char szMsg[256];
	PCHARINFO pCharTmp, pCharHP;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	pMemberInfo = m_pInfo->GetMemberInfo (dwTblIDTarget);
	if (pMemberInfo == NULL) {
		return;
	}

	pCharHP = g_aCharInfo[pMemberInfo->dwTblIDHP];
	pCharHP->byEfect += byDamage;
	pCharHP->byEfect = min (pCharHP->byEfect, 5);
	SendCmdEFECTCHG (pCharHP->pID, pCharHP->dwTblID, pCharHP->byEfect);

	pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
	pCharTmp->byEfect = byEfect;
	SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);

	wsprintf (szMsg, "%sに%dのダメージ！", pCharTmp->szName, byDamage);
	JudgeMsg (szMsg);

	wsprintf (szMsg, "%d", byDamage);
	SendCmdADDCHAREFC (pCharTmp->dwTblID, RGB (250, 60, 4), szMsg);

	/* やられた？ */
	if (pCharHP->byEfect >= 5) {
		Dead (dwTblIDTarget);
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Dead											 */
/* 内容：	やられた処理													 */
/* 日付：	2004/08/14														 */
/* ========================================================================= */

void CProcSubGameWAR::Dead(DWORD dwTblIDTarget)
{
	int i, nNo, nTeam, nTmp, nCount;
	char szMsg[256];
	PCHARINFO pCharTmp, pCharHP;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo, pMemberInfoTmp;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pMemberInfo = m_pInfo->GetMemberInfo (dwTblIDTarget);
	if (pMemberInfo == NULL) {
		return;
	}

	pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
	wsprintf (szMsg, "%sはやられてしまった！", pCharTmp->szName);
	JudgeMsg (szMsg);

	nTeam		= m_pInfo->GetTeamNo (dwTblIDTarget);
	nNo			= GetDeadPosNo (nTeam);
	nCount		= m_pInfo->GetMemberCount (nTeam);
	pTeamInfo	= m_pInfo->GetTeamInfo (nTeam);

	/* 全滅判定 */
	nTmp = 0;
	for (i = 0; i < nCount; i ++) {
		pMemberInfoTmp = m_pInfo->GetMemberInfo (nTeam, i);
		if (pMemberInfo == NULL) {
			continue;
		}
		pCharHP = g_aCharInfo[pMemberInfoTmp->dwTblIDHP];
		if (pCharHP->byEfect >= 5) {
			nTmp ++;
		}
	}

	/* 全滅？ */
	if (nTmp >= nCount) {
		JudgeMsg ("試合終了！");
		/* チーム１ */
		if (nTeam == 0) {
			JudgeMsg ("青チームの勝利！");
		} else {
			JudgeMsg ("赤チームの勝利！");
		}
		Quit ();

	} else {
		pCharTmp->wGrpTmpSub = 0;
		pMemberInfo->dwTblIDRes = CheckCharBlockNPC (
					m_pInfo->m_dwMapID,
					pTeamInfo->ptResurrection.x + nNo,
					pTeamInfo->ptResurrection.y);
		SubProcMove (pCharTmp, pTeamInfo->ptResurrection.x + nNo, pTeamInfo->ptResurrection.y - 1);
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetDeadPosNo									 */
/* 内容：	空いている待機場所を取得										 */
/* 日付：	2006/05/04														 */
/* ========================================================================= */

int CProcSubGameWAR::GetDeadPosNo(BYTE byTeam)
{
	int i, j, nCount, nCount2, nRet;
	DWORD dwTblIDRes;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	nRet = -1;
	pTeamInfo = m_pInfo->GetTeamInfo (byTeam);
	if (pTeamInfo == NULL) {
		goto Exit;
	}

	nCount = GetMemberType () - 1;
	for (i = 0; i < nCount; i ++) {
		dwTblIDRes = CheckCharBlockNPC (
						m_pInfo->m_dwMapID,
						pTeamInfo->ptResurrection.x + i,
						pTeamInfo->ptResurrection.y);
		if (dwTblIDRes == -1) {
			continue;
		}
		nCount2 = GetMemberCount (byTeam);
		for (j = 0; j < nCount2; j ++) {
			pMemberInfo = m_pInfo->GetMemberInfo (byTeam, j);
			if (pMemberInfo == NULL) {
				continue;
			}
			/* 使用中？ */
			if (pMemberInfo->dwTblIDRes == dwTblIDRes) {
				break;
			}
		}
		if (j < nCount2) {
			continue;
		}
		nRet = i;
		break;
	}

Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Resuscitation									 */
/* 内容：	蘇生判定														 */
/* 日付：	2004/08/14														 */
/* ========================================================================= */

void CProcSubGameWAR::Resuscitation(DWORD dwTblID, BOOL bProc)
{
	int nNo, nTeam;
	char szMsg[256];
	PCHARINFO pCharTmp, pCharHP, pCharRes;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	/* 全滅ルールの時は自動復活なし */
	if ((bProc == FALSE) && (m_pInfo->m_byRule == 0)) {
		return;
	}

	pMemberInfo = m_pInfo->GetMemberInfo (dwTblID);
	if (pMemberInfo == NULL) {
		return;
	}

	nTeam		= m_pInfo->GetTeamNo (dwTblID);
	pTeamInfo	= m_pInfo->GetTeamInfo (nTeam);
	nNo			= m_pInfo->GetMemberNo (nTeam, dwTblID);
	pCharRes	= g_aCharInfo[pMemberInfo->dwTblIDRes];
	pCharHP		= g_aCharInfo[pMemberInfo->dwTblIDHP];

	pCharRes->byEfect ++;
	pCharTmp = g_aCharInfo[dwTblID];

	if (bProc || (pCharRes->byEfect >= 3)) {
		wsprintf (szMsg, "%sは復活した！", pCharTmp->szName);
		pCharRes->byEfect = 0;

		pCharTmp->wGrpTmpSub = 3;
		SubProcMove (pCharTmp, pTeamInfo->ptStart.x, pTeamInfo->ptStart.y + nNo);
		pMemberInfo->dwTblIDRes = -1;

		/* HP3で復活 */
		pCharHP->byEfect = 2;
		SendCmdEFECTCHG (pCharHP->pID, pCharHP->dwTblID, pCharHP->byEfect);

	} else {
		wsprintf (szMsg, "%sの復活まで残り %d ターン", pCharTmp->szName, 3 - pCharRes->byEfect);
	}

	SendCmdEFECTCHG (pCharRes->pID, pCharRes->dwTblID, pCharRes->byEfect);
	JudgeMsg (szMsg);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::Quit											 */
/* 内容：	試合終了														 */
/* 日付：	2004/08/14														 */
/* ========================================================================= */

void CProcSubGameWAR::Quit(void)
{
	if (m_pInfo->m_byState == SUBGAME_WAR_STATE_INIT) {
		return;
	}

	/* 状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_QUIT;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::MakeTargetFire									 */
/* 内容：	魔法対象者一覧作成(炎)											 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::MakeTargetFire(
	PCMDPARA_WARSELECTMAGICTARGET pTarget)
{
	int i, nCount, nTmp;
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTmp;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	/* チーム１ */
	if (m_pInfo->m_byTurn == 0) {
		pTeamInfo = m_pInfo->GetTeamInfo (1);
	} else {
		pTeamInfo = m_pInfo->GetTeamInfo (0);
	}
	dwTblIDChar = m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}
	pChar = g_aCharInfo[dwTblIDChar];
	pTarget->byMagic = SUBGAME_WAR_MAGIC_FIRE;

	nCount = m_pInfo->GetMemberCount (pTeamInfo);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];

		pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
		nTmp = abs (pChar->dwX - pCharTmp->dwX) + abs (pChar->dwY - pCharTmp->dwY);
		/* 距離は２歩以外？ */
		if (nTmp != 2) {
			continue;
		}
		if (pMemberInfo->dwTblIDRes != -1) {
			continue;
		}
		strcpy (pTarget->aszName[pTarget->byCount], pCharTmp->szName);
		pTarget->abyNo[pTarget->byCount] = i;
		pTarget->byCount ++;
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::MakeTargetQure									 */
/* 内容：	魔法対象者一覧作成(回復)										 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::MakeTargetQure(
	PCMDPARA_WARSELECTMAGICTARGET pTarget)
{
	int i, nCount, nTmp;
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTmp;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	dwTblIDChar	= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}
	pChar = g_aCharInfo[dwTblIDChar];

	pTarget->byMagic = SUBGAME_WAR_MAGIC_QURE;
	nCount = m_pInfo->GetMemberCount (pTeamInfo);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];

		pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
		nTmp = abs (pChar->dwX - pCharTmp->dwX) + abs (pChar->dwY - pCharTmp->dwY);
		/* 距離は２歩以内？ */
		if (nTmp > 2) {
			continue;
		}
		strcpy (pTarget->aszName[pTarget->byCount], pCharTmp->szName);
		pTarget->abyNo[pTarget->byCount] = i;
		pTarget->byCount ++;
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::MakeTargetResuscitation						 */
/* 内容：	魔法対象者一覧作成(蘇生)										 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::MakeTargetResuscitation(
	PCMDPARA_WARSELECTMAGICTARGET pTarget)
{
	int i, nCount;
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTmp;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	dwTblIDChar	= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}
	pChar = g_aCharInfo[dwTblIDChar];

	pTarget->byMagic = SUBGAME_WAR_MAGIC_RESUSCITATION;
	nCount = m_pInfo->GetMemberCount (pTeamInfo);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];
		if (pMemberInfo->dwTblIDRes == -1) {
			continue;
		}

		pCharTmp = g_aCharInfo[pMemberInfo->dwTblID];
		strcpy (pTarget->aszName[pTarget->byCount], pCharTmp->szName);
		pTarget->abyNo[pTarget->byCount] = i;
		pTarget->byCount ++;
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::UseMagicFire									 */
/* 内容：	魔法使用(炎)													 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::UseMagicFire(BYTE byNo)
{
	int nAtack, nTarget;
	char szMsg[256];
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTarget;
	PSUBSTATUS_WAR pSubStatus;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	/* チーム１ */
	if (m_pInfo->m_byTurn == 0) {
		pTeamInfo = m_pInfo->GetTeamInfo (1);
	} else {
		pTeamInfo = m_pInfo->GetTeamInfo (0);
	}
	dwTblIDChar = m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}
	pMemberInfo	= pTeamInfo->aMemberInfo[byNo];
	pChar		= g_aCharInfo[dwTblIDChar];
	pCharTarget	= g_aCharInfo[pMemberInfo->dwTblID];
	pSubStatus	= (PSUBSTATUS_WAR)pCharTarget->abySubStatus;

	wsprintf (szMsg, "%sの魔法「炎」！", pChar->szName);
	JudgeMsg (szMsg);

	nAtack = Rand ();
	if (nAtack == 0) {
		JudgeMsg ("失敗！");
		return;
	}

	/* 相手は魔法使い？ */
	if ((nAtack < 4) && (pSubStatus->byJob == SUBGAME_WAR_JOB_WITCH)) {
		/* 跳ね返し判定 */
		nTarget = Rand ();
		if ((nTarget >= 1) && (nTarget <= 3)) {
			/* 相手より大きい？ */
			if (nTarget - nAtack > 0) {
				nAtack = nTarget - nAtack;

				wsprintf (szMsg, "%sの魔法跳ね返し！", pCharTarget->szName);
				JudgeMsg (szMsg);
				Damage (pChar->dwTblID, nAtack, 32);
				return;

			} else if (nTarget == nAtack) {
				wsprintf (szMsg, "%sの魔法跳ね返しにより相殺された！", pCharTarget->szName);
				JudgeMsg (szMsg);
				return;
			}
		}
	}

	nAtack = min (nAtack, 3);
	Damage (pCharTarget->dwTblID, nAtack, 32);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::UseMagicQure									 */
/* 内容：	魔法使用(回復)													 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::UseMagicQure(BYTE byNo)
{
	int nTmp;
	char szMsg[256];
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTarget;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	dwTblIDChar	= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}

	pMemberInfo	= pTeamInfo->aMemberInfo[byNo];
	pChar		= g_aCharInfo[dwTblIDChar];
	pCharTarget	= g_aCharInfo[pMemberInfo->dwTblID];

	wsprintf (szMsg, "%sの魔法[回復]！", pChar->szName);
	JudgeMsg (szMsg);

	nTmp = Rand ();
	if (nTmp == 0) {
		JudgeMsg ("失敗！");
		return;
	}

	if (nTmp == 4) {
		nTmp = 5;
	}
	HPUp (pCharTarget->dwTblID, (BYTE)nTmp);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::UseMagicResuscitation							 */
/* 内容：	魔法使用(蘇生)													 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void CProcSubGameWAR::UseMagicResuscitation(BYTE byNo)
{
	int nTmp;
	char szMsg[256];
	DWORD dwTblIDChar;
	PCHARINFO pChar, pCharTarget;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo	= m_pInfo->GetTeamInfo (m_pInfo->m_byTurn);
	dwTblIDChar	= m_pInfo->GetTurnTblID (m_pInfo->m_byTurn);

	if (dwTblIDChar == -1) {
		return;
	}

	pMemberInfo	= pTeamInfo->aMemberInfo[byNo];
	pChar		= g_aCharInfo[dwTblIDChar];
	pCharTarget	= g_aCharInfo[pMemberInfo->dwTblID];

	wsprintf (szMsg, "%sの魔法[蘇生]！", pChar->szName);
	JudgeMsg (szMsg);

	nTmp = Rand ();
	if ((nTmp >= 1) && (nTmp <= 3)) {
		JudgeMsg ("失敗！");
		return;
	}

	Resuscitation (pCharTarget->dwTblID, TRUE);
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::GetInfo										 */
/* 内容：	戦場ゲーム情報を取得											 */
/* 日付：	2006/04/28														 */
/* ========================================================================= */

CInfoSubGameWAR *CProcSubGameWAR::GetInfo(void)
{
	return m_pInfo;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcINIT										 */
/* 内容：	処理(初期状態)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcINIT(void)
{
	m_pInfo->m_dwTblIDJudge	= -1;
	m_pInfo->m_byState		= SUBGAME_WAR_STATE_CLEAN;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcCLEAN										 */
/* 内容：	処理(初期化)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcCLEAN(void)
{
	int i, nCount;

	m_pInfo->m_byRule			= 0;
	m_pInfo->m_byTurn			= 0;
	m_pInfo->m_dwWaitTblID		= -1;
	m_pInfo->m_dwFirstTimeProc	= 0;
	m_pInfo->m_dwLastTimeProc	= 0;
	m_pInfo->m_dwNextTimeProc	= 0;

	if (m_pInfo->m_dwTblIDJudge == -1) {
		m_pInfo->m_dwTblIDJudge = CheckCharBlockNPC (
				m_pInfo->m_dwMapID,
				m_pInfo->m_ptJudge.x,
				m_pInfo->m_ptJudge.y);
	}

	m_pInfo->m_byState = SUBGAME_WAR_STATE_READY;

	nCount = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		m_pInfo->DeleteAllMemberInfo (i);
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcREADY										 */
/* 内容：	処理(準備完了)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcREADY(void)
{
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcJOIN										 */
/* 内容：	処理(参加者募集中)												 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcJOIN(void)
{
	char szMsg[256];
	int nTmp, i, nCount;
	DWORD dwTime;

	dwTime = timeGetTime ();
	if (dwTime < m_pInfo->m_dwNextTimeProc) {
		return;
	}

	m_pInfo->m_dwLastTimeProc = dwTime;
	m_pInfo->m_dwNextTimeProc = dwTime + 10 * 1000;

	nTmp = abs (m_pInfo->m_dwFirstTimeProc - dwTime) / 1000;
	/* 規定時間経過していない？ */
	if (nTmp < SUBWAR_WAITTIME_JOIN) {
		/* 残り時間表示 */
		wsprintf (szMsg, "参加者募集中。残り %d 秒", SUBWAR_WAITTIME_JOIN - nTmp);
		JudgeMsg (szMsg);
		goto Exit;
	}

	nCount = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		nTmp = GetMemberCount (i);
		/* メンバーがいない？ */
		if (nTmp == 0) {
			JudgeMsg ("相手チームがいません。無効試合となりました。");
			Quit ();
			goto Exit;
		}
	}

	JudgeMsg ("参加者募集終了しました。");
	JudgeMsg ("参加者の方はルールを選択してください。");

	m_pInfo->m_dwLastTimeProc	= 0;
	m_pInfo->m_dwFirstTimeProc	= dwTime;
	m_pInfo->m_dwNextTimeProc	= m_pInfo->m_dwFirstTimeProc + 10 * 1000;

	/* ルール選択に状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_SELECTRULE;
	AllChgStateAndQuit (SUBGAME_WAR_STATE_SELECTRULE, SUBGAME_WAR_STATE_WAIT);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcSELECTRULE									 */
/* 内容：	処理(ルール選択)												 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcSELECTRULE(void)
{
	int nTmp;
	char szMsg[256];
	DWORD dwTime;

	dwTime = timeGetTime ();
	if (dwTime < m_pInfo->m_dwNextTimeProc) {
		return;
	}

	m_pInfo->m_dwLastTimeProc = dwTime;
	m_pInfo->m_dwNextTimeProc = dwTime + 10 * 1000;

	nTmp = abs (m_pInfo->m_dwFirstTimeProc - dwTime) / 1000;
	/* 規定時間経過していない？ */
	if (nTmp < SUBWAR_WAITTIME) {
		/* 残り時間表示 */
		wsprintf (szMsg, "ルール選択中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
		JudgeMsg (szMsg);
		goto Exit;
	}

	JudgeMsg ("ルール選択終了しました。");
	if (m_pInfo->m_byRule == 0) {
		strcpy (szMsg, "[全滅させる]で試合を行います。");
	} else {
		strcpy (szMsg, "[城へ到達]で試合を行います。");
	}
	JudgeMsg (szMsg);
	JudgeMsg ("参加者の方は職業を選択してください。");

	m_pInfo->m_dwLastTimeProc	= 0;
	m_pInfo->m_dwFirstTimeProc	= dwTime;
	m_pInfo->m_dwNextTimeProc	= m_pInfo->m_dwFirstTimeProc + 10 * 1000;

	/* 職業選択に状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_SELECTJOB;
	AllChgState (SUBGAME_WAR_STATE_SELECTJOB);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcSELECTJOB									 */
/* 内容：	処理(職業選択)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcSELECTJOB(void)
{
	int nTmp;
	char szMsg[256];
	DWORD dwTime;

	dwTime = timeGetTime ();
	if (dwTime < m_pInfo->m_dwNextTimeProc) {
		return;
	}

	m_pInfo->m_dwLastTimeProc = dwTime;
	m_pInfo->m_dwNextTimeProc = dwTime + 10 * 1000;

	nTmp = abs (m_pInfo->m_dwFirstTimeProc - dwTime) / 1000;
	/* 規定時間経過していない？ */
	if (nTmp < SUBWAR_WAITTIME) {
		/* 残り時間表示 */
		wsprintf (szMsg, "職業選択中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
		JudgeMsg (szMsg);
		goto Exit;
	}

	JudgeMsg ("職業選択終了しました。");
	JudgeMsg ("参加者の方は行動順を設定してください。");

	m_pInfo->m_dwLastTimeProc	= 0;
	m_pInfo->m_dwFirstTimeProc	= dwTime;
	m_pInfo->m_dwNextTimeProc	= m_pInfo->m_dwFirstTimeProc + 10 * 1000;

	SendCmdSubWarSELECTTURN (m_pInfo, 0);
	SendCmdSubWarSELECTTURN (m_pInfo, 1);

	/* 順番設定に状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_SELECTTURN;
	AllChgState (SUBGAME_WAR_STATE_SELECTTURN);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcSELECTTURN									 */
/* 内容：	処理(順番設定)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcSELECTTURN(void)
{
	int nTmp;
	char szMsg[256];
	DWORD dwTime;

	dwTime = timeGetTime ();
	if (dwTime < m_pInfo->m_dwNextTimeProc) {
		return;
	}

	m_pInfo->m_dwLastTimeProc = dwTime;
	m_pInfo->m_dwNextTimeProc = dwTime + 10 * 1000;

	nTmp = abs (m_pInfo->m_dwFirstTimeProc - dwTime) / 1000;
	/* 規定時間経過していない？ */
	if (nTmp < SUBWAR_WAITTIME) {
		/* 残り時間表示 */
		wsprintf (szMsg, "行動順設定中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
		JudgeMsg (szMsg);
		goto Exit;
	}

	JudgeMsg ("順番設定終了しました。");

	m_pInfo->m_dwLastTimeProc	= 0;
	m_pInfo->m_dwFirstTimeProc	= 0;
	m_pInfo->m_dwNextTimeProc	= 0;

	/* 試合準備に状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_GAMEPREPARE;

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcGAMEPREPARE								 */
/* 内容：	処理(試合準備)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcGAMEPREPARE(void)
{
	JudgeMsg ("戦場ゲーム戦闘開始！");
	SendCmdPLAYSOUNDinside (m_pInfo->m_dwTblIDJudge, SOUND_WHISTLE);

	m_pInfo->m_byTurn = (BYTE)(genrand () % 2);

	if (m_pInfo->m_byTurn == 0) {
		JudgeMsg ("先攻は赤チームです！");
	} else {
		JudgeMsg ("先攻は青チームです！");
	}

	/* 準備処理 */
	PrePare ();

	/* 試合中に状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_GAME;

	ChgTurn ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcGAME										 */
/* 内容：	処理(試合中)													 */
/* 日付：	2006/03/31														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcGAME(void)
{
}


/* ========================================================================= */
/* 関数名：	CProcSubGameWAR::ProcQUIT										 */
/* 内容：	処理(終了)														 */
/* 日付：	2006/05/04														 */
/* ========================================================================= */

void CProcSubGameWAR::ProcQUIT(void)
{
	int i, j, nCount, nCount2;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	/* 状態遷移 */
	m_pInfo->m_byState = SUBGAME_WAR_STATE_INIT;

	nCount = m_pInfo->GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		nCount2 = m_pInfo->GetMemberCount (i);
		for (j = 0; j < nCount2; j ++) {
			pMemberInfo = m_pInfo->GetMemberInfo (i, 0);
			if (pMemberInfo == NULL) {
				continue;
			}
			RemoveAndMove (pMemberInfo->dwTblID);
		}
	}
}

/* Copyright(C)URARA-works 2006 */
