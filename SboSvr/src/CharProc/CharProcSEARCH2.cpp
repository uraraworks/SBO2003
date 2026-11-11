/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH2.cpp											 */
/* 内容：		キャラ処理(検索してついていく)クラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/08													 */
/* ========================================================================= */
//Todo
//ターゲットが移動し続けると検索できないので、どこかの地点を目指すようにするといいかも

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH2.h"
#include <algorithm>
#include <functional>

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

enum {
	STATE_READY = 0,					/* 待機中 */
	STATE_SEARCHTARGETINIT,				/* ターゲット検索用初期化 */
	STATE_SEARCHTARGET,					/* ターゲット検索中 */
	STATE_SEARCHMOVETARGET,				/* ターゲット検索移動中 */
	STATE_SEARCHINIT,					/* 検索用初期化 */
	STATE_PREMOVE,						/* 検索前移動中 */
	STATE_SEARCH,						/* 検索中 */
	STATE_SEARCHMOVE,					/* 検索移動中 */
	STATE_MOVE,							/* 移動中 */
};


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::CCharProcSEARCH2								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

CCharProcSEARCH2::CCharProcSEARCH2()
{
	m_nState = STATE_READY;

	m_ptStart.x		= m_ptStart.y	= 0;
	m_ptEnd.x		= m_ptEnd.y		= 0;
	m_ptNow.x		= m_ptNow.y		= 0;
	m_ptSearch.x	= m_ptSearch.y	= 0;
	m_sizeArea.cx	= m_sizeArea.cy	= 0;

	m_nState			= STATE_READY;
	m_nDirection		= 0;
	m_nSearchDirection	= 0;
	m_nSearchTurn		= 0;
	m_dwSearchLastTime	= 0;
	m_pwSearchMap		= NULL;
	m_dwSearchMapCapacity = 0;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::~CCharProcSEARCH2								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/05/09														 */
/* ========================================================================= */

CCharProcSEARCH2::~CCharProcSEARCH2()
{
	SAFE_DELETE_ARRAY (m_pwSearchMap);
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

BOOL CCharProcSEARCH2::Proc(DWORD dwTime)
{
	BOOL bResult;

	switch (m_nState) {
	case STATE_READY:				ProcREADY ();				break;	/* 待機中 */
	case STATE_SEARCHTARGETINIT:	ProcSEARCHTARGETINIT ();	break;	/* ターゲット検索用初期化 */
	case STATE_SEARCHTARGET:		ProcSEARCHTARGET ();		break;	/* ターゲット検索中 */
	case STATE_SEARCHMOVETARGET:	ProcSEARCHMOVETARGET ();	break;	/* ターゲット検索移動中 */
	case STATE_SEARCHINIT:			ProcSEARCHINIT ();			break;	/* 検索用初期化 */
	case STATE_PREMOVE:				ProcPREMOVE ();				break;	/* 検索前移動中 */
	case STATE_SEARCH:				ProcSEARCH ();				break;	/* 検索中 */
	case STATE_SEARCHMOVE:			ProcSEARCHMOVE ();			break;	/* 検索移動中 */
	case STATE_MOVE:				ProcMOVE ();				break;	/* 移動中 */
	}

	if (m_nState >= STATE_PREMOVE) {
		bResult = IsTargetReady ();
		/* ターゲットが有効ではない？ */
		if (bResult == FALSE) {
			m_nState = STATE_READY;
		}
	}

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ChgTarget										 */
/* 内容：	ターゲット変更													 */
/* 日付：	2006/07/19														 */
/* ========================================================================= */

void CCharProcSEARCH2::ChgTarget(DWORD dwTblID, BYTE byDirection)
{
	CCharProcBase::ChgTarget (dwTblID, byDirection);

	m_ptEnd.x = m_ptEnd.y = -1;
	m_nState = STATE_SEARCHINIT;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::IsBlock										 */
/* 内容：	当たり判定														 */
/* 日付：	2006/05/09														 */
/* 戻り値：	TRUE:ぶつからない FALSE:ぶつかる								 */
/* ========================================================================= */

BOOL CCharProcSEARCH2::IsBlock(int x, int y)
{
	BOOL bRet, bResult;

	bRet = FALSE;

	bResult = CheckCharBlock (m_pChar->dwMapID, x, y);
	if (bResult == FALSE) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::IsMove										 */
/* 内容：	移動するか判定													 */
/* 日付：	2006/05/21														 */
/* 戻り値：	TRUE:移動する FALSE:移動しない									 */
/* ========================================================================= */

BOOL CCharProcSEARCH2::IsMove(int nDirection)
{
	BOOL bRet;
	DWORD dwTime, dwTmp;

	bRet = FALSE;

	dwTime = timeGetTime ();

	if (m_pChar->byDirection >= 2) {
		dwTmp = m_pChar->byMoveProbabilityX * 1000 / 100;
	} else {
		dwTmp = m_pChar->byMoveProbabilityY * 1000 / 100;
	}
	dwTmp += 400;

	if ((DWORD)abs (dwTime - m_pChar->dwLastDotMove) < dwTmp) {
		goto Exit;
	}
	m_pChar->dwLastDotMove = dwTime;

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::IsTargetReady									 */
/* 内容：	ターゲットが有効か判定											 */
/* 日付：	2006/05/21														 */
/* 戻り値：	TRUE:有効 FALSE:有効ではない									 */
/* ========================================================================= */

BOOL CCharProcSEARCH2::IsTargetReady(void)
{
	BOOL bRet;
	PCHARINFO pChar;

	bRet = FALSE;

	if (m_pChar->dwTargetTblID == -1) {
		goto Exit;
	}

	pChar = g_aCharInfo[m_pChar->dwTargetTblID];
	/* マップが違う？ */
	if (m_pChar->dwMapID != pChar->dwMapID) {
		goto Exit;
	}
	/* ログオフしている？ */
	if (pChar->byGameState == GAMESTATE_LOGOFF) {
		goto Exit;
	}
	/* 生きていない？ */
	if (pChar->wHP == 0) {
		goto Exit;
	}
	/* 範囲外？ */
	if (!((abs (m_pChar->dwX - pChar->dwX) <= m_pChar->bySearchCx) &&
		  (abs (m_pChar->dwY - pChar->dwY) <= m_pChar->bySearchCy))) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/05/11														 */
/* ========================================================================= */

void CCharProcSEARCH2::Init(void)
{
	DWORD dwSize;
	PMAPINFO pMap;

	pMap = g_aMapInfo[m_pChar->dwMapID];

	m_ptStart.x = m_ptNow.x = m_ptSearch.x = m_pChar->dwX;
	m_ptStart.y = m_ptNow.y = m_ptSearch.y = m_pChar->dwY;
	m_sizeArea.cx = pMap->nCX;
	m_sizeArea.cy = pMap->nCY;

	m_nDirection		= m_pChar->byDirection;
	m_nSearchDirection	= m_nDirection;
	m_nSearchTurn		= 0;
	m_dwSearchLastTime	= timeGetTime ();

	dwSize = (m_sizeArea.cx * 2 + 1) * (m_sizeArea.cy * 2 + 1);
	if (dwSize > m_dwSearchMapCapacity) {
		SAFE_DELETE_ARRAY (m_pwSearchMap);
		m_pwSearchMap = new WORD[dwSize];
		m_dwSearchMapCapacity = dwSize;
		ZeroMemory (m_pwSearchMap, dwSize * sizeof (WORD));
		m_anSearchMapUsed.clear ();
	} else {
		ClearSearchMap ();
	}

	m_aSearchOpen.clear ();
	m_aSearchInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ClearSearchMap								 */
/* 内容：	検索用マップの書き込んだ所だけ消去							 */
/* 日付：	2026/11/11														 */
/* ========================================================================= */

void CCharProcSEARCH2::ClearSearchMap(void)
{
	int i, nCount;

	nCount = (int)m_anSearchMapUsed.size ();
	for (i = 0; i < nCount; i ++) {
		m_pwSearchMap[m_anSearchMapUsed[i]] = 0;
	}
	m_anSearchMapUsed.clear ();
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::Search										 */
/* 内容：	検索															 */
/* 日付：	2006/05/11														 */
/* ========================================================================= */

DWORD CCharProcSEARCH2::Search(
	BOOL bPCCheck)		/* [in] PCとの当たり判定を行うか */
{
	DWORD dwRet, dwResult;
	int i, nNo, x, y,
		anDirection[] = {1, 0, 3, 2}, anPosX[] = {0, 0, -1, 1}, anPosY[] = {-1, 1, 0, 0};
	SEARCH2INFO SearchInfo;

	dwRet = -1;

	/* 各方向をチェック */
	for (i = 0; i < 4; i ++) {
		x = m_ptSearch.x + anPosX[i];
		y = m_ptSearch.y + anPosY[i];
		/* 範囲外？ */
		if (!((abs (m_ptStart.x - x) <= m_pChar->bySearchCx) &&
			  (abs (m_ptStart.y - y) <= m_pChar->bySearchCy))) {
			continue;
		}
		/* 開始位置？ */
		if ((x == m_ptStart.x) && (y == m_ptStart.y)) {
			continue;
		}
		/* 検索済み？ */
		if (m_pwSearchMap[y * m_sizeArea.cx + x] > 0) {
			continue;
		}
		/* ぶつかる？ */
		if (IsBlock (x, y) == FALSE) {
			continue;
		}
		if (bPCCheck) {
			dwResult = CheckCharBlockPC (m_pChar->dwMapID, x, y);
			if (dwResult != -1) {
				dwRet = dwResult;
				m_pChar->dwTargetTblID = dwResult;
				m_ptEnd.x = m_ptSearch.x;
				m_ptEnd.y = m_ptSearch.y;
				break;
			}
		}

		/* 移動できそうなので検索情報を追加 */
		SearchInfo.x			= x;
		SearchInfo.y			= y;
		SearchInfo.byDirection	= anDirection[i];
		SearchInfo.byState		= 0;
		SearchInfo.wScore		= abs (m_ptEnd.x - x) + abs (m_ptEnd.y - y) + abs (m_ptStart.x - x) + abs (m_ptStart.y - y);
		SearchInfo.wTurn		= (WORD)m_nSearchTurn;
		nNo = m_aSearchInfo.Add (SearchInfo);
		m_pwSearchMap[y * m_sizeArea.cx + x] = (WORD)(nNo + 1);
		m_anSearchMapUsed.push_back (y * m_sizeArea.cx + x);
		/* 元の走査と同じく、同点は追加順、0xFFFFは選択しない */
		if (SearchInfo.wScore < 0xFFFF) {
			m_aSearchOpen.push_back (std::make_pair (SearchInfo.wScore, nNo));
			std::push_heap (m_aSearchOpen.begin (), m_aSearchOpen.end (), std::greater<std::pair<WORD, int> > ());
		}
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::SearchMove									 */
/* 内容：	検索移動														 */
/* 日付：	2006/05/11														 */
/* ========================================================================= */

int CCharProcSEARCH2::SearchMove(void)
{
	int nNo, nProcCount, nRet;
	PSEARCH2INFO pInfo;

	nRet		= 0;
	nNo			= -1;
	nProcCount	= 0;

	/* スコア、追加順が最小の未処理候補を取り出す */
	if (!m_aSearchOpen.empty ()) {
		nNo = m_aSearchOpen.front ().second;
		std::pop_heap (m_aSearchOpen.begin (), m_aSearchOpen.end (), std::greater<std::pair<WORD, int> > ());
		m_aSearchOpen.pop_back ();
	}
	/* 見つかった？ */
	if (nNo >= 0) {
		pInfo = &m_aSearchInfo[nNo];
		nProcCount ++;
		pInfo->byState	= 1;
		m_ptSearch.x	= pInfo->x;
		m_ptSearch.y	= pInfo->y;
		m_nSearchTurn	= pInfo->wTurn + 1;

		/* 最終位置？ */
		if ((pInfo->x == m_ptEnd.x) && (pInfo->y == m_ptEnd.y)) {
			nRet = -1;
			goto Exit;
		}
	}

	nRet = nProcCount;
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::InfoCleanup									 */
/* 内容：	検索情報最適化													 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::InfoCleanup(
	int x,		/* [in] 最終位置(横) */
	int y)		/* [in] 最終位置(縦) */
{
	int i, nNo, nCount,
		anDirection[] = {1, 0, 3, 2}, anPosX[] = {0, 0, -1, 1}, anPosY[] = {-1, 1, 0, 0};
	PSEARCH2INFO pInfo;
	ARRAYSEARCH2INFO aSearchInfoTmp;

	/* 最短ルートのみ残した配列を作成 */
	nCount = m_aSearchInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		nNo		= m_pwSearchMap[m_sizeArea.cx * y + x] - 1;
		pInfo	= &m_aSearchInfo[nNo];

		aSearchInfoTmp.Add (*pInfo);
		x += anPosX[pInfo->byDirection];
		y += anPosY[pInfo->byDirection];
		/* 開始位置？ */
		if ((x == m_ptStart.x) && (y == m_ptStart.y)) {
			m_nSearchDirection = anDirection[pInfo->byDirection];
			break;
		}
	}

	/* 本データを更新 */
	ClearSearchMap ();
	m_aSearchOpen.clear ();
	m_aSearchInfo.RemoveAll ();
	m_aSearchInfo.Copy (&aSearchInfoTmp);

	nCount = m_aSearchInfo.GetSize ();
	for (i = nCount - 1; i >= 0; i --) {
		pInfo = &m_aSearchInfo[i];
		m_pwSearchMap[m_sizeArea.cx * pInfo->y + pInfo->x] = (WORD)(i + 1);
		m_anSearchMapUsed.push_back (m_sizeArea.cx * pInfo->y + pInfo->x);

		if (i > 0) {
			/* 次の位置への向きを設定 */
			pInfo->byDirection = anDirection[m_aSearchInfo[i - 1].byDirection];

		} else {
			pInfo->byDirection = anDirection[pInfo->byDirection];
		}
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcREADY										 */
/* 内容：	処理(待機中)													 */
/* 日付：	2006/05/11														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcREADY(void)
{
	PMAPINFO pMap;

	pMap = g_aMapInfo[m_pChar->dwMapID];
	/* 誰もいない？ */
	if (pMap->dwCountTblNoPC == 0) {
		return;
	}

	m_pChar->dwTargetTblID = -1;
	m_ptEnd.x = m_ptEnd.y = -1;
	m_nState = STATE_SEARCHTARGETINIT;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCHTARGETINIT							 */
/* 内容：	処理(ターゲット検索用初期化)									 */
/* 日付：	2006/05/10														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCHTARGETINIT(void)
{
	Init ();
	m_nState = STATE_SEARCHTARGET;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCHTARGET								 */
/* 内容：	処理(ターゲット検索中)											 */
/* 日付：	2006/05/10														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCHTARGET(void)
{
	DWORD dwResult;

	dwResult = Search (TRUE);
	if (dwResult != -1) {
		m_nState = STATE_SEARCHINIT;

	} else {
		m_nState = STATE_SEARCHMOVETARGET;
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCHMOVETARGET							 */
/* 内容：	処理(ターゲット検索移動中)										 */
/* 日付：	2006/05/11														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCHMOVETARGET(void)
{
	int nResult;

	nResult = SearchMove ();
	switch (nResult) {
	case 0:			/* 移動できなかった */
		m_nState = STATE_READY;
		break;

	default:		/* 移動中 */
		m_nState = STATE_SEARCHTARGET;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCHINIT								 */
/* 内容：	処理(検索用初期化)												 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCHINIT(void)
{
	Init ();
	m_nState = STATE_PREMOVE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcPREMOVE									 */
/* 内容：	処理(検索前移動中)												 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcPREMOVE(void)
{
	BOOL bResult;
	int x, y,
		anDirection[] = {1, 0, 3, 2}, anPosX[] = {0, 0, -1, 1}, anPosY[] = {-1, 1, 0, 0};
	PCHARINFO pChar;

	bResult = IsMove (m_nDirection);
	if (bResult == FALSE) {
		return;
	}

	pChar = g_aCharInfo[m_pChar->dwTargetTblID];
	/* ターゲットが移動した？ */
	if (!((m_ptEnd.x == (int)pChar->dwX) && (m_ptEnd.y == (int)pChar->dwY))) {
		m_ptEnd.x = pChar->dwX;
		m_ptEnd.y = pChar->dwY;
	}

	/* 最終位置？ */
	if ((m_ptNow.x == m_ptEnd.x) && (m_ptNow.y == m_ptEnd.y)) {
		goto Exit;
	}

	if (m_ptNow.x == m_ptEnd.x) {
		if (m_ptNow.y > m_ptEnd.y) {
			m_nDirection = 0;
		} else {
			m_nDirection = 1;
		}

	} else if (m_ptNow.y == m_ptEnd.y) {
		if (m_ptNow.x > m_ptEnd.x) {
			m_nDirection = 2;
		} else {
			m_nDirection = 3;
		}

	} else {
		if (abs (m_ptNow.x - m_ptEnd.x) < abs (m_ptNow.y - m_ptEnd.y)) {
			if (m_ptNow.y > m_ptEnd.y) {
				m_nDirection = 0;
			} else {
				m_nDirection = 1;
			}
		} else {
			if (m_ptNow.x > m_ptEnd.x) {
				m_nDirection = 2;
			} else {
				m_nDirection = 3;
			}
		}
	}

	x = m_ptNow.x + anPosX[m_nDirection];
	y = m_ptNow.y + anPosY[m_nDirection];
	/* ぶつかる？ */
	if (IsBlock (x, y) == FALSE) {
		/* 検索開始 */
		m_nState			= STATE_SEARCH;
		m_ptStart			= m_ptNow;
		m_ptSearch.x		= m_ptNow.x;
		m_ptSearch.y		= m_ptNow.y;
		m_nSearchDirection	= m_nDirection;
		return;
	}

	if ((x == m_ptEnd.x) && (y == m_ptEnd.y)) {
		m_ptEnd.x = m_ptNow.x;
		m_ptEnd.y = m_ptNow.y;
	} else {
		m_ptNow.x = x;
		m_ptNow.y = y;
	}

	if (m_pChar->byDirection != m_nDirection) {
		m_pChar->byDirection = m_nDirection;
		m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
	}

Exit:
	if (!((m_pChar->dwX == (DWORD)m_ptNow.x) && (m_pChar->dwY == (DWORD)m_ptNow.y))) {
		if (!((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255))) {
			m_pChar->dwX = (DWORD)m_ptNow.x;
			m_pChar->dwY = (DWORD)m_ptNow.y;
			m_pChar->byFlg |= CHARINFOFLG_MOVE;
		}
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCH									 */
/* 内容：	処理(検索中)													 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCH(void)
{
	PCHARINFO pChar;

	pChar = g_aCharInfo[m_pChar->dwTargetTblID];
	/* ターゲットが移動した？ */
	if (!((m_ptEnd.x == (int)pChar->dwX) && (m_ptEnd.y == (int)pChar->dwY))) {
		m_ptEnd.x	= pChar->dwX;
		m_ptEnd.y	= pChar->dwY;
		m_nState	= STATE_PREMOVE;
		Init ();
		return;
	}

	Search (FALSE);
	m_nState = STATE_SEARCHMOVE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcSEARCHMOVE								 */
/* 内容：	処理(検索移動中)												 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcSEARCHMOVE(void)
{
	int nResult;
	PCHARINFO pChar;

	pChar = g_aCharInfo[m_pChar->dwTargetTblID];
	/* ターゲットが移動した？ */
	if (!((m_ptEnd.x == (int)pChar->dwX) && (m_ptEnd.y == (int)pChar->dwY))) {
		m_ptEnd.x	= pChar->dwX;
		m_ptEnd.y	= pChar->dwY;
		m_nState	= STATE_PREMOVE;
		Init ();
		return;
	}

	nResult = SearchMove ();
	switch (nResult) {
	case -1:		/* 最終地点 */
		m_nState = STATE_MOVE;
		/* 集めたデータを整理 */
		InfoCleanup (m_ptEnd.x, m_ptEnd.y);
		break;

	default:		/* 移動中 */
		m_nState = STATE_SEARCH;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH2::ProcMOVE										 */
/* 内容：	処理(移動中)													 */
/* 日付：	2006/05/08														 */
/* ========================================================================= */

void CCharProcSEARCH2::ProcMOVE(void)
{
	BOOL bResult;
	int nNo,
		anPosX[] = {0, 0, -1, 1}, anPosY[] = {-1, 1, 0, 0};
	PSEARCH2INFO pInfo;
	PCHARINFO pChar;

	bResult = IsMove (m_pChar->byDirection);
	if (bResult == FALSE) {
		return;
	}

	pChar = g_aCharInfo[m_pChar->dwTargetTblID];
	/* ターゲットが移動した？ */
	if (!((m_ptEnd.x == (int)pChar->dwX) && (m_ptEnd.y == (int)pChar->dwY))) {
		m_ptEnd.x	= pChar->dwX;
		m_ptEnd.y	= pChar->dwY;
		m_nState	= STATE_PREMOVE;
		Init ();
		return;
	}

	/* 最終位置？ */
	if ((m_ptNow.x == m_ptEnd.x) && (m_ptNow.y == m_ptEnd.y)) {
		return;
	}

	nNo = m_pwSearchMap[m_sizeArea.cx * m_ptNow.y + m_ptNow.x] - 1;
	if (nNo >= 0) {
		pInfo = &m_aSearchInfo[nNo];
		m_nSearchDirection = pInfo->byDirection;
	}

	/* 次の位置へ移動 */
	m_ptNow.x		+= anPosX[m_nSearchDirection];
	m_ptNow.y		+= anPosY[m_nSearchDirection];
	m_nDirection	= m_nSearchDirection;

	if (m_pChar->byDirection != m_nDirection) {
		m_pChar->byDirection = m_nDirection;
		m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
	}

	/* 最終位置ではない？ */
	if (!((m_ptNow.x == m_ptEnd.x) && (m_ptNow.y == m_ptEnd.y))) {
		if (!((m_pChar->dwX == (DWORD)m_ptNow.x) && (m_pChar->dwY == (DWORD)m_ptNow.y))) {
			if (!((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255))) {
				m_pChar->dwX = (DWORD)m_ptNow.x;
				m_pChar->dwY = (DWORD)m_ptNow.y;
				m_pChar->byFlg |= CHARINFOFLG_MOVE;
			}
		}
	}
}

/* Copyright(C)URARA-works 2006 */
