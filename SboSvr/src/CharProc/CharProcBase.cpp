/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBase.cpp											 */
/* 内容：		キャラ処理基底クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/16													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "IncludeCharProc.h"

/* ========================================================================= */
/* 関数名：	CCharProcBase::CCharProcBase									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

CCharProcBase::CCharProcBase()
{
	m_nPosX				= 0;
	m_nPosY				= 0;
	m_dwLastTimeProc	= 0;
	m_pChar				= NULL;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::~CCharProcBase									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

CCharProcBase::~CCharProcBase()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::Create											 */
/* 内容：	作成															 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

void CCharProcBase::Create(PCHARINFO pChar)
{
	m_pChar = pChar;
	m_nPosX = (int)m_pChar->dwX;
	m_nPosY = (int)m_pChar->dwY;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::Proc												 */
/* 内容：	処理															 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

BOOL CCharProcBase::Proc(DWORD dwTime)
{
	BOOL bRet;
	DWORD dwTmp;

	bRet	= FALSE;
	dwTmp	= dwTime - m_dwLastTimeProc;

	if (dwTmp < 100) {
		goto Exit;
	}
	m_dwLastTimeProc = dwTime;

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::GetPos											 */
/* 内容：	座標を取得														 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

void CCharProcBase::GetPos(BYTE &byPosX, BYTE &byPosY)
{
	byPosX = (BYTE)m_nPosX;
	byPosY = (BYTE)m_nPosY;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::SetProc											 */
/* 内容：	処理を設定														 */
/* 日付：	2006/01/18														 */
/* ========================================================================= */

void CCharProcBase::SetProc(PCHARINFO pChar)
{
	PCCharProcBase pProc;

	pProc = (PCCharProcBase)pChar->pProc;
	SAFE_DELETE (pProc);

	switch (pChar->byNpcType) {
	case NPCTYPE_STOPRAND:			pProc = new CCharProcSTOPRAND;			break;	/* 止まるかランダム？ */
	case NPCTYPE_PUT:				pProc = new CCharProcPUT;				break;	/* NPC発生？ */
	case NPCTYPE_MOVERAND:
	case NPCTYPE_BATTLE1:			pProc = new CCharProcMOVERAND;			break;	/* ランダム移動？ */
	case NPCTYPE_SEARCH:
	case NPCTYPE_BATTLE2:			pProc = new CCharProcSEARCH;			break;	/* 検索してついていく？ */
	case NPCTYPE_BOMB:				pProc = new CCharProcBOMB;				break;	/* 爆弾 */
	case NPCTYPE_BOMBBLOCK:			pProc = new CCharProcBOMBBLOCK;			break;	/* 爆弾ゲームの壁 */
	case NPCTYPE_BLOCK:																/* 押せる */
	case NPCTYPE_BLOCK2:															/* 押せる(サイコロ) */
	case NPCTYPE_BLOCK3:															/* 押せる(サイコロ２) */
	case NPCTYPE_BLOCK4:															/* 押せる(サイコロ３) */
	case NPCTYPE_CHGPUSH:			pProc = new CCharProcBLOCK;				break;	/* 押せる(押せて話すと切り替え) */
	case NPCTYPE_SCORE:				pProc = new CCharProcSCORE;				break;	/* 得点板 */
	case NPCTYPE_NOMOVE:															/* 移動しない */
	case NPCTYPE_NOBLOCK:															/* 移動しない(通り抜け可) */
	case NPCTYPE_BOMBITEM:			pProc = new CCharProcNOMOVE;			break;	/* 爆弾ゲームのアイテム */
	case NPCTYPE_USEMAGIC:			pProc = new CCharProcUSEMAGIC;			break;	/* 重なったキャラに魔法発動 */
	case NPCTYPE_ATACK:				pProc = new CCharProcATACK;				break;	/* 重なったキャラに攻撃 */
	case NPCTYPE_USEMAGICPARENT:	pProc = new CCharProcUSEMAGICPARENT;	break;	/* 魔法発動し続ける */
	case NPCTYPE_SEARCH2:			pProc = new CCharProcSEARCH2;			break;	/* 検索してついていく2 */
	}

	if (pProc) {
		pProc->Create (pChar);
		pChar->pProc = pProc;
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::ChgTarget										 */
/* 内容：	ターゲット変更													 */
/* 日付：	2006/07/19														 */
/* ========================================================================= */

void CCharProcBase::ChgTarget(DWORD dwTblID, BYTE byDirection)
{
	m_pChar->dwTargetTblID = dwTblID;
	m_pChar->byDirection = byDirection;
	m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::NpcProcSEARCH_Check								 */
/* 内容：	検索処理チェック												 */
/* 日付：	2003/10/12														 */
/* ========================================================================= */
BOOL CCharProcBase::NpcProcSEARCH_Check(PCHARINFO pChar, PCHARINFO pCharTmp, int nX, int nY)
{
	BOOL bRet;
	DWORD dwTblID;

	bRet = FALSE;
	/* 障害物にぶるかる？ */
	if (CheckCharBlock (pChar, pChar->dwMapID, nX, nY) == FALSE) {
		goto Exit;
	}

	dwTblID = CheckCharBlockPC (pChar->dwMapID, nX, nY);
	/* プレイヤーにぶつかる？ */
	if (dwTblID != -1) {
		if (pChar->byNpcType == NPCTYPE_BATTLE2) {
			/* ぶつかるキャラにタゲ変更 */
			pChar->dwTargetTblID = dwTblID;
		}

	} else {
		/* イベントが配置されている？ */
		if (CheckEventBlock (pChar->dwMapID, (DWORD)nX, (DWORD)nY)) {
			goto Exit;
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::PartsMoveProc									 */
/* 内容：	移動パーツにのっていれば移動させる								 */
/* 日付：	2004/07/30														 */
/* ========================================================================= */

BOOL CCharProcBase::PartsMoveProc(PCHARINFO pChar)
{
	PMAPINFO pMap;
	BYTE byHitType;
	BOOL bMove, bRet;

	if (abs (timeGetTime () - pChar->dwLastDotMove) < 500) {
		return FALSE;
	}

	bRet = FALSE;
	pMap = g_aMapInfo[pChar->dwMapID];
	byHitType = (&g_aMapPartsInfo[pMap->pwMap[pChar->dwX + pChar->dwY * pMap->nCX]])->byHitType;

	bMove = TRUE;
	switch (byHitType) {
	case PARTSHIT_MOVEFRONT:	/* 一歩前へ進む */
		break;
	case PARTSHIT_MOVEUP:		/* 一歩上へ進む */
		pChar->byDirection = 0;
		break;
	case PARTSHIT_MOVEDOWN:		/* 一歩下へ進む */
		pChar->byDirection = 1;
		break;
	case PARTSHIT_MOVERIGHT:	/* 一歩左へ進む */
		pChar->byDirection = 2;
		break;
	case PARTSHIT_MOVELEFT:		/* 一歩右へ進む */
		pChar->byDirection = 3;
		break;
	default:
		bMove = FALSE;
		break;
	}
	if (bMove) {
		DWORD dwX, dwY, dwTblID;

		dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
		dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
		bRet = CheckCharBlock (pChar->dwMapID, dwX, dwY);

		/* NPCとぶつかる？ */
		dwTblID = CheckCharBlockNPC (pChar->dwMapID, dwX, dwY);
		if ((dwTblID != -1) && (pChar->dwTblID != dwTblID)) {
			bRet = FALSE;
		}

		if (bRet) {
			pChar->dwX = dwX;
			pChar->dwY = dwY;
			pChar->byFlg |= CHARINFOFLG_MOVE;
		}
		pChar->byFlg |= CHARINFOFLG_DIRECTION;
	}

	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcBase::MoveStartPosProc									 */
/* 内容：	開始位置に移動													 */
/* 日付：	2006/01/18														 */
/* ========================================================================= */

void CCharProcBase::MoveStartPosProc(PCHARINFO pChar)
{
	PMAPINFO pMap;

	pMap = g_aMapInfo[pChar->dwMapID];
	/* 誰かいる？ */
	if (pMap->dwCountTblNoPC > 0) {
		return;
	}

	/* 開始位置に移動 */
	pChar->dwMapID	= pChar->dwStartMapID;
	pChar->dwX		= pChar->dwStartX;
	pChar->dwY		= pChar->dwStartY;
}

/* Copyright(C)URARA-works 2006 */
