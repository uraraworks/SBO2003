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
/* 関数名：	PutCharSetStatus												 */
/* 内容：	発生元の能力（テンプレート指定中はテンプレート）を出てくるキャラへ写す	 */
/*			位置・向き・処理クラスはさわらない（呼ぶ側で）					 */
/* 日付：	2026/11/08（発生時の処理から切り出し）							 */
/* ========================================================================= */

void PutCharSetStatus(PCHARINFO pParent, PCHARINFO pChar)
{
	pChar->wGrpID			= pParent->wGrpID;
	pChar->wGrpMain			= pParent->wGrpMain;
	pChar->wGrpSub			= pParent->wGrpSub;
	pChar->byAreaX			= pParent->byAreaX;
	pChar->byAreaY			= pParent->byAreaY;
	pChar->byNpcType		= NPCTYPE_BATTLE1;
	pChar->byLevel			= pParent->byLevel;
	pChar->byPW				= pParent->byPW;
	pChar->byDEX			= pParent->byDEX;
	pChar->byST				= pParent->byST;
	pChar->byIT				= pParent->byIT;
	pChar->byLK				= pParent->byLK;
	pChar->byJOB			= pParent->byJOB;
	pChar->byDropProbability	= pParent->byDropProbability;
	pChar->wAT				= pParent->wAT;
	pChar->wDF				= pParent->wDF;
	pChar->wMaxHP			= pParent->wMaxHP;
	pChar->wHP				= pParent->wHP;
	pChar->wMaxMana			= pParent->wMaxMana;
	pChar->wMana			= pParent->wMana;
	pChar->wDFMagic			= pParent->wDFMagic;
	pChar->dwGold			= pParent->dwGold;
	pChar->dwExp			= pParent->dwExp;
	pChar->clName			= pParent->clName;
	pChar->clSpeech			= pParent->clSpeech;
	CopyMemory (pChar->waItem,		pParent->waItem,	sizeof (pChar->waItem));
	CopyMemory (pChar->abyMagic,	pParent->abyMagic,	sizeof (pChar->abyMagic));
	CopyMemory (pChar->szName,		pParent->szName,	sizeof (pChar->szName));
	pChar->szName[sizeof (pChar->szName) - 1] = 0;

	/* テンプレートが指定されている？ */
	if ((pParent->dwStTmplateID != -1) && (pParent->dwStTmplateID < (DWORD)g_aStTmpInfo.GetSize ())) {
		PSTATUSTEMPLATEINFO pTmp;

		pTmp = g_aStTmpInfo[pParent->dwStTmplateID];
		pChar->byLevel				= pTmp->byLevel;
		pChar->byPW					= pTmp->byPW;
		pChar->byDEX				= pTmp->byDEX;
		pChar->byST					= pTmp->byST;
		pChar->byIT					= pTmp->byIT;
		pChar->byLK					= pTmp->byLK;
		pChar->byNpcType			= pTmp->byNpcType;
		pChar->wMaxHP				= pTmp->wMaxHP;
		pChar->wHP					= pTmp->wMaxHP;
		pChar->wMaxMana				= pTmp->wMaxMana;
		pChar->wMana				= pTmp->wMaxMana;
		pChar->wDFMagic				= pTmp->wDFMagic;
		pChar->chAttrFire			= pTmp->chAttrFire;
		pChar->chAttrWater			= pTmp->chAttrWater;
		pChar->chAttrWind			= pTmp->chAttrWind;
		pChar->chAttrEarth			= pTmp->chAttrEarth;
		pChar->chAttrLight			= pTmp->chAttrLight;
		pChar->chAttrDark			= pTmp->chAttrDark;
		pChar->dwGold				= pTmp->dwGold;
		pChar->dwExp				= pTmp->dwExp;
		pChar->byDropProbability	= pTmp->byDropProbability;
		pChar->byMoveProbabilityX	= pTmp->byMoveProbabilityX;
		pChar->byMoveProbabilityY	= pTmp->byMoveProbabilityY;
		pChar->bySearchCx			= pTmp->bySearchCx;
		pChar->bySearchCy			= pTmp->bySearchCy;
		CopyMemory (pChar->waItem,		pTmp->waItem,	sizeof (pChar->waItem));
		CopyMemory (pChar->abyMagic,	pTmp->abyMagic,	sizeof (pChar->abyMagic));
	}
}


/* ========================================================================= */
/* 関数名：	RenewPutChildChar												 */
/* 内容：	発生元から出ているキャラへ、今の発生元（テンプレート）の能力を反映	 */
/*			管理者ウィンドウ・データ API で発生元やテンプレートを変えた時に呼ぶ。 */
/*			（以前は出ているキャラが倒されて出直すまで古い能力のままだった）	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

void RenewPutChildChar(PCHARINFO pParent)
{
	int i, nCount;
	WORD wHP;
	BYTE byNpcType;

	if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT)) {
		return;
	}

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];

		if ((pChar == NULL) || (pChar == pParent)) {
			continue;
		}
		if (pChar->dwParentTblID != pParent->dwTblID) {
			continue;
		}
		if (pChar->byGameState != pParent->byGameState) {
			continue;
		}

		wHP			= pChar->wHP;
		byNpcType	= pChar->byNpcType;
		PutCharSetStatus (pParent, pChar);

		/* 倒れている途中のキャラは生き返らせない。ほかは満タンで出直したことにする */
		if (wHP == 0) {
			pChar->wHP = 0;
		}

		/* 種別（戦闘1・戦闘2など）が変わったら処理クラスを作り直す */
		if (pChar->byNpcType != byNpcType) {
			CCharProcBase CharProcBase;

			CharProcBase.SetProc (pChar);
		}

		RenewATDF (pChar->dwTblID);
		SendCmdCHARINFO3 (pChar->dwTblID);
	}
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
	pCharNew->dwSpaceID			= m_pChar->dwSpaceID;		/* 発生させる元と同じ空間に出す */
	pCharNew->byGameState		= m_pChar->byGameState;
	pCharNew->dwMapID			= m_pChar->dwMapID;
	pCharNew->dwX				= dwX;
	pCharNew->dwY				= dwY;
	pCharNew->dwStartMapID		= m_pChar->dwStartMapID;
	pCharNew->dwStartX			= dwX;
	pCharNew->dwStartY			= dwY;
	pCharNew->byDirection		= m_pChar->byDirection;
	pCharNew->dwParentTblID		= m_pChar->dwTblID;
	PutCharSetStatus (m_pChar, pCharNew);
	/* テンプレートが指定されている？（種別がテンプレートのものになる） */
	if (m_pChar->dwStTmplateID != -1) {
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
