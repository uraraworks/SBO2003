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
#include "SboSvrQuest.h"

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
	pChar->byGhost				= pParent->byGhost;
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
		pChar->byGhost				= pTmp->byGhost;	/* 霊（半透明）はテンプレートの設定にする */
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
	BOOL bRet;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 1000) {
		goto Exit;
	}

	if ((m_pChar->byPutCount == 0) || (m_pChar->byPutCountNow >= m_pChar->byPutCount)) {
		goto Exit;
	}
	/* 補充しない印：決めた数を出したら、倒されても出さない */
	if (m_pChar->byPutOnce && (m_pChar->byPutDone >= m_pChar->byPutCount)) {
		goto Exit;
	}
	/* 番外編ダンジョンの発生元は、組ごとの空間に出す（SboSvrQuest.cpp の周期処理）。ふつうの世界には出さない */
	if (DungeonPutIsSpaced (m_pChar)) {
		goto Exit;
	}
	if (dwTime < m_pChar->dwLastPut + (DWORD)(m_pChar->wTimePutInterval * 1000)) {
		goto Exit;
	}

	bRet = PutOne (dwTime);
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcPUT::PutOne											 */
/* 内容：	発生範囲からマスを1つくじで選び、キャラを1体出す				 */
/*			選んだマスが通れない・イベント・ダメージ床なら今回はあきらめる	 */
/* 戻り値：	出したら TRUE													 */
/* 日付：	2026/11/17（Proc から切り出し）									 */
/* ========================================================================= */

BOOL CCharProcPUT::PutOne(DWORD dwTime, BOOL bSpaced)
{
	BOOL bRet, bResult;
	DWORD dwTmpX, dwTmpY, dwX, dwY;
	PMAPINFO pMap;
	PCHARINFO pCharNew;

	bRet = FALSE;
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

	/* ダメージ床（毒沼・溶岩など）？ 敵は出さない */
	if (IsDamageFloor (pMap, (int)dwX, (int)dwY)) {
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
	/* 番外編ダンジョンの中の敵は経験値・お金を持たない（中では入らない。落とす物はそのまま） */
	if (bSpaced) {
		pCharNew->dwExp = 0;
		pCharNew->dwGold = 0;
	}

	if (pCharNew->byNpcType == NPCTYPE_BOMBITEM) {
		pCharNew->byEfect = (BYTE)genrand () % 7;
	}
	pCharNew = AddChar (pCharNew);
	pCharNew->pID = pCharNew->dwTblID + 1;

	RenewATDF (pCharNew->dwTblID);
	SendCmdCHARINFO3 (pCharNew->dwTblID);

	if (!bSpaced) {
		m_pChar->dwLastPut = dwTime;
		m_pChar->byPutCountNow ++;
		m_pChar->dwLastDotMove = dwTime;
		if (m_pChar->byPutOnce && (m_pChar->byPutDone < 255)) {
			m_pChar->byPutDone ++;
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：	CCharProcPUT::PutOneSpace										 */
/* 内容：	指定した空間に 1体出す（番外編ダンジョン）						 */
/*			マスの空き・ぶつかりは、その空間で見る（発生元の空間を一時的に替える）。 */
/*			発生元の数・時刻は動かさない（組ごとの数は呼ぶ側が持つ）			 */
/* 日付：	2026/11/19														 */
/* ========================================================================= */

BOOL CCharProcPUT::PutOneSpace(DWORD dwTime, DWORD dwSpaceID)
{
	DWORD dwBak = m_pChar->dwSpaceID;
	BOOL bRet;

	m_pChar->dwSpaceID = dwSpaceID;
	bRet = PutOne (dwTime, TRUE);
	m_pChar->dwSpaceID = dwBak;
	return bRet;
}

/* ========================================================================= */
/* 関数名：	PutCharFillAll													 */
/* 内容：	サーバー起動時に、各NPC発生を発生数まですぐ出す					 */
/*			発生から出たキャラは保存しないので、起動直後は 0 体。			 */
/*			（Node 版の timeGetTime はプロセス起動からの時間なので、		 */
/*			  ふだんの処理だと発生周期が過ぎるまで 1 体目すら出なかった）	 */
/*			すでに出ている数（byPutCountNow）からの不足分だけ出すので、		 */
/*			二度呼んでも発生数を超えない。									 */
/*			選んだマスが通れない時もあるので、1体あたり何回かまで引き直す。	 */
/*			それでも出なかった分は、ふだんの処理（発生周期ごと）に任せる。	 */
/* 日付：	2026/11/17														 */
/* ========================================================================= */

void PutCharFillAll(void)
{
	int i, nCount, nTry, nTryMax;
	DWORD dwTime;

	dwTime = timeGetTime ();
	/* 出したキャラは後ろに足されるので、起動時にいたキャラだけを見る */
	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		CCharProcPUT *pProc;

		if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_PUT) || (pChar->pProc == NULL)) {
			continue;
		}
		if (pChar->byPutCountNow >= pChar->byPutCount) {
			continue;
		}
		/* 番外編ダンジョンの発生元は、組が入った時にその空間へ出す */
		if (DungeonPutIsSpaced (pChar)) {
			continue;
		}
		pProc = (CCharProcPUT *)pChar->pProc;
		nTryMax = (pChar->byPutCount - pChar->byPutCountNow) * 50;
		for (nTry = 0; (nTry < nTryMax) && (pChar->byPutCountNow < pChar->byPutCount); nTry ++) {
			pProc->PutOne (dwTime);
		}
	}
}

/* Copyright(C)URARA-works 2006 */
