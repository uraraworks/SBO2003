/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcPUT.cpp												 */
/* 内容：		キャラ処理(NPC発生)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcPUT.h"

/* ========================================================================= */
/* 関数名：	CCharProcPUT::CCharProcPUT										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcPUT::CCharProcPUT()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcPUT::Proc												 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcPUT::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;
	DWORD dwTmpX, dwTmpY, dwX, dwY;
	PMAPINFO pMap;
	PCHARINFO pCharNew;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 1000) {
		goto Exit;
	}

	if ((m_pChar->byPutCount == 0) || (m_pChar->byPutCountNow >= m_pChar->byPutCount)) {
		goto Exit;
	}
	if (dwTime < m_pChar->dwLastPut + (DWORD)(m_pChar->wTimePutInterval * 1000)) {
		goto Exit;
	}

	dwTmpX = 0, dwTmpY = 0, dwX = m_pChar->dwX, dwY = m_pChar->dwY;
	if (m_pChar->byAreaX) {
		dwTmpX = genrand () % m_pChar->byAreaX;
	}
	if (m_pChar->byAreaY) {
		dwTmpY = genrand () % m_pChar->byAreaY;
	}
	if (genrand () % 2 != 0) {
		dwX += dwTmpX;
	} else {
		dwX -= dwTmpX;
	}
	if (genrand () % 2 != 0) {
		dwY += dwTmpY;
	} else {
		dwY -= dwTmpY;
	}

	pMap = g_aMapInfo[GetMapNo (m_pChar->dwMapID)];
	dwX = max (dwX, 0);
	dwX = min (dwX, (DWORD)pMap->nCX - 1);
	dwY = max (dwY, 0);
	dwY = min (dwY, (DWORD)pMap->nCY - 1);

	/* 通れない場所に発生する？ */
	bResult = CheckCharBlock (m_pChar, m_pChar->dwMapID, dwX, dwY);
	if (bResult == FALSE) {
		/* 今回はあきらめる */
		goto Exit;
	}

	/* イベントが配置されている？ */
	if (CheckEventBlock (m_pChar->dwMapID, dwX, dwY)) {
		/* 今回はあきらめる */
		goto Exit;
	}

	pCharNew = new CHARINFO;
	ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));
	pCharNew->byNpcType = NPCTYPE_BATTLE1;
	SetInitCharData (pCharNew);
	pCharNew->byGameState		= m_pChar->byGameState;
	pCharNew->dwMapID			= m_pChar->dwMapID;
	pCharNew->dwX				= dwX;
	pCharNew->dwY				= dwY;
	pCharNew->dwStartMapID		= m_pChar->dwStartMapID;
	pCharNew->dwStartX			= dwX;
	pCharNew->dwStartY			= dwY;
	pCharNew->wGrpID			= m_pChar->wGrpID;
	pCharNew->wGrpMain			= m_pChar->wGrpMain;
	pCharNew->wGrpSub			= m_pChar->wGrpSub;
	pCharNew->byDirection		= m_pChar->byDirection;
	pCharNew->dwParentTblID		= m_pChar->dwTblID;
	pCharNew->byAreaX			= m_pChar->byAreaX;
	pCharNew->byAreaY			= m_pChar->byAreaY;
	pCharNew->byLevel			= m_pChar->byLevel;
	pCharNew->byPW				= m_pChar->byPW;
	pCharNew->byDEX				= m_pChar->byDEX;
	pCharNew->byST				= m_pChar->byST;
	pCharNew->byIT				= m_pChar->byIT;
	pCharNew->byLK				= m_pChar->byLK;
	pCharNew->byJOB				= m_pChar->byJOB;
	pCharNew->byDropProbability	= m_pChar->byDropProbability;
	pCharNew->wAT				= m_pChar->wAT;
	pCharNew->wDF				= m_pChar->wDF;
	pCharNew->wMaxHP			= m_pChar->wMaxHP;
	pCharNew->wHP				= m_pChar->wHP;
	pCharNew->wMaxMana			= m_pChar->wMaxMana;
	pCharNew->wMana				= m_pChar->wMana;
	pCharNew->wDFMagic			= m_pChar->wDFMagic;
	pCharNew->dwGold			= m_pChar->dwGold;
	pCharNew->dwExp				= m_pChar->dwExp;
	pCharNew->clName			= m_pChar->clName;
	pCharNew->clSpeech			= m_pChar->clSpeech;
	CopyMemory (pCharNew->waItem,	m_pChar->waItem,	sizeof (m_pChar->waItem));
	CopyMemory (pCharNew->abyMagic,	m_pChar->abyMagic,	sizeof (m_pChar->abyMagic));
	strcat (pCharNew->szName, m_pChar->szName);

	/* テンプレートが指定されている？ */
	if (m_pChar->dwStTmplateID != -1) {
		PSTATUSTEMPLATEINFO pTmp;

		pTmp = g_aStTmpInfo[m_pChar->dwStTmplateID];
		pCharNew->byLevel				= pTmp->byLevel;
		pCharNew->byPW					= pTmp->byPW;
		pCharNew->byDEX					= pTmp->byDEX;
		pCharNew->byST					= pTmp->byST;
		pCharNew->byIT					= pTmp->byIT;
		pCharNew->byLK					= pTmp->byLK;
		pCharNew->byNpcType				= pTmp->byNpcType;
		pCharNew->wMaxHP				= pTmp->wMaxHP;
		pCharNew->wHP					= pTmp->wMaxHP;
		pCharNew->wMaxMana				= pTmp->wMaxMana;
		pCharNew->wMana					= pTmp->wMaxMana;
		pCharNew->wDFMagic				= pTmp->wDFMagic;
		pCharNew->chAttrFire			= pTmp->chAttrFire;
		pCharNew->chAttrWater			= pTmp->chAttrWater;
		pCharNew->chAttrWind			= pTmp->chAttrWind;
		pCharNew->chAttrEarth			= pTmp->chAttrEarth;
		pCharNew->chAttrLight			= pTmp->chAttrLight;
		pCharNew->chAttrDark			= pTmp->chAttrDark;
		pCharNew->dwGold				= pTmp->dwGold;
		pCharNew->dwExp					= pTmp->dwExp;
		pCharNew->byDropProbability		= pTmp->byDropProbability;
		pCharNew->byMoveProbabilityX	= pTmp->byMoveProbabilityX;
		pCharNew->byMoveProbabilityY	= pTmp->byMoveProbabilityY;
		pCharNew->bySearchCx			= pTmp->bySearchCx;
		pCharNew->bySearchCy			= pTmp->bySearchCy;
		CopyMemory (pCharNew->waItem,	pTmp->waItem,	sizeof (pCharNew->waItem));
		CopyMemory (pCharNew->abyMagic,	pTmp->abyMagic,	sizeof (pCharNew->abyMagic));
		SetProc (pCharNew);
	}

	if (pCharNew->byNpcType == NPCTYPE_BOMBITEM) {
		pCharNew->byEfect = (BYTE)genrand () % 7;
	}
	pCharNew = AddChar (pCharNew);
	pCharNew->pID = pCharNew->dwTblID + 1;

	RenewATDF (pCharNew->dwTblID);
	SendCmdCHARINFO3 (pCharNew->dwTblID);

	m_pChar->dwLastPut = dwTime;
	m_pChar->byPutCountNow ++;
	m_pChar->dwLastDotMove = dwTime;

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
