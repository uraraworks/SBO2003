/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH.cpp											 */
/* 内容：		キャラ処理(検索してついていく)クラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */
/*
	動き方（2026/11 改修。「検索して追尾」「戦闘２」の敵が使う）
	・サーバー負荷を抑えるため、これまでどおり 0.5 秒に1回の自分の順番で
	  「探す」「1歩進む」「向きを変える」のどれか1つだけをする。経路探索はしない。
	・探し方：以前は探索範囲のマスを1回に1マスずつ順番に調べていた（探索3で一巡約25秒）。
	  1マス調べるたびにマップのプレイヤー一覧を1回なめていたので、同じ1回のなめで
	  「探索範囲の中で一番近いプレイヤー」を選ぶようにした。手間は同じで、すぐ気づく。
	  （見つけても 30% は見逃すのは従来どおり）
	・見つける前：開始位置の近く（行動範囲、ただし最大3マス。未設定は2マス）をゆっくりうろつく。
	・追いかける：1回の順番で、移動確率(未設定は20%)の確率で目標へ近い方へ1歩。
	  ふさがっていたらもう一方の軸、それもだめなら横へよける。
	  探索範囲(+2マス)から3秒外れる、または10秒進めなければあきらめる。
	・あきらめたら：以前は開始位置へ瞬間移動していたが、歩いて戻る。
	  戻る途中で10秒進めなかった時だけ、最後の手段として瞬間移動する。
*/

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH.h"

/* 判定の間隔(ms)。従来と同じ */
#define SEARCH_PROCTIME			(500)
/* 1歩動いてから次に動けるまでの時間(ms)。クライアントが1マス歩ききる時間 */
#define SEARCH_STEPTIME			(320)
/* 見つけても見逃す確率(%)。従来と同じ */
#define SEARCH_MISSRATE			(30)
/* 追いかける時の歩く確率が未設定(0)の時の値(%)。従来と同じ */
#define SEARCH_DEFAULTCHASE		(20)
/* 探索範囲から外れてからあきらめるまで(ms) */
#define SEARCH_LOSTTIME			(3000)
/* 進めなくなってからあきらめるまで(ms) */
#define SEARCH_STUCKTIME		(10000)
/* 探索範囲から外れたとみなす余裕(マス) */
#define SEARCH_LOSTMARGIN		(2)

/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::CCharProcSEARCH								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcSEARCH::CCharProcSEARCH()
{
	m_bInit			= FALSE;
	m_bGoHome		= FALSE;
	m_dwRestEnd		= 0;
	m_dwLostTime	= 0;
	m_dwStuckTime	= 0;
	m_dwTargetLast	= (DWORD)-1;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;
	DWORD dwTblID;
	PMAPINFO pMap;
	PCHARINFO pCharTmp;

	bRet = FALSE;

	if ((DWORD)(dwTime - m_dwLastTimeProc) < SEARCH_PROCTIME) {
		goto Exit;
	}
	/* 最後に動いてから1マス歩ききる時間がたっていない */
	if ((DWORD)(dwTime - m_pChar->dwLastDotMove) < SEARCH_STEPTIME) {
		goto Exit;
	}
	m_dwLastTimeProc = dwTime;

	if (m_bInit == FALSE) {
		/* みんなが同時に動き出さないよう、最初の休みをばらけさせる */
		m_bInit		= TRUE;
		m_dwRestEnd	= dwTime + (DWORD)(genrand () % 4000);
	}

	bResult = PartsMoveProc (m_pChar);
	if (bResult) {
		m_pChar->dwLastDotMove = dwTime;
		goto Exit;
	}

	pMap = g_aMapInfo[m_pChar->dwMapID];
	/* 誰もいない？ */
	if (pMap->dwCountTblNoPC == 0) {
		goto Exit;
	}

	/* ターゲットがいる？（攻撃されて設定された時もここへ来る） */
	if (m_pChar->dwTargetTblID != -1) {
		bRet = ProcTarget (dwTime);
		goto Exit;
	}
	m_dwTargetLast = (DWORD)-1;

	/* 探索範囲で一番近いプレイヤーを探す */
	dwTblID = FindTarget ();
	if ((dwTblID != -1) && (genrand () % 100 >= SEARCH_MISSRATE)) {
		pCharTmp = g_aCharInfo[dwTblID];
		m_pChar->dwTargetTblID = dwTblID;
		if (m_pChar->byNpcType == NPCTYPE_BATTLE2) {
			if (m_pChar->dwLastBattle == 0) {
				/* 初回は即攻撃させる */
				m_pChar->dwLastBattle = 1;
			}
		}
		/* 見つけたプレイヤーの方を向く */
		{
			int nDX = (int)pCharTmp->dwX - (int)m_pChar->dwX;
			int nDY = (int)pCharTmp->dwY - (int)m_pChar->dwY;
			BYTE byDirection;
			if (abs (nDX) > abs (nDY)) {
				byDirection = (nDX < 0) ? 2 : 3;
			} else {
				byDirection = (nDY < 0) ? 0 : 1;
			}
			if ((nDX || nDY) && (m_pChar->byDirection != byDirection)) {
				m_pChar->byDirection = byDirection;
				m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			}
		}
		bRet = TRUE;
		goto Exit;
	}

	if (m_bGoHome) {
		bRet = ProcGoHome (dwTime);
	} else {
		bRet = ProcWander (dwTime);
	}

Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::ProcTarget										 */
/* 内容：	ターゲットを追いかける											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::ProcTarget(DWORD dwTime)
{
	int nX, nY, nDX, nDY, nRate;
	BYTE byDirection;
	BOOL bFlg;
	PCHARINFO pCharTmp;

	pCharTmp = g_aCharInfo[m_pChar->dwTargetTblID];

	/* 新しいターゲット？（自分で見つけた・攻撃された・ぶつかった） */
	if (m_dwTargetLast != m_pChar->dwTargetTblID) {
		m_dwTargetLast	= m_pChar->dwTargetTblID;
		m_dwStuckTime	= dwTime;
		m_dwLostTime	= 0;
		m_bGoHome		= FALSE;
	}
	if ((m_pChar->byNpcType == NPCTYPE_BATTLE2) && (m_pChar->dwLastBattle == 0)) {
		m_pChar->dwLastBattle = 1;
	}

	if (IsTargetValid (pCharTmp) == FALSE) {
		DropTarget ();
		return FALSE;
	}

	nX = (int)m_pChar->dwX + anPosAddTblX[m_pChar->byDirection];
	nY = (int)m_pChar->dwY + anPosAddTblY[m_pChar->byDirection];
	/* 目の前にターゲットキャラがいる？（攻撃は戦闘処理がする） */
	if ((CheckCharX (m_pChar, (DWORD)nX, pCharTmp->dwX)) && ((DWORD)nY == pCharTmp->dwY)) {
		m_dwStuckTime	= dwTime;
		m_dwLostTime	= 0;
		return FALSE;
	}

	/* となりにいるならそちらを向く */
	bFlg = FALSE;
	byDirection = m_pChar->byDirection;
	if (m_pChar->dwY == pCharTmp->dwY - 1) {
		if (m_pChar->dwX == pCharTmp->dwX) {
			byDirection = 1;
			bFlg = TRUE;
		}
	} else if (m_pChar->dwY == pCharTmp->dwY + 1) {
		if (m_pChar->dwX == pCharTmp->dwX) {
			byDirection = 0;
			bFlg = TRUE;
		}
	} else if (CheckCharX (m_pChar, m_pChar->dwX, pCharTmp->dwX + 1)) {
		if (m_pChar->dwY == pCharTmp->dwY) {
			byDirection = 2;
			bFlg = TRUE;
		}
	} else if (CheckCharX (m_pChar, m_pChar->dwX, pCharTmp->dwX - 1)) {
		if (m_pChar->dwY == pCharTmp->dwY) {
			byDirection = 3;
			bFlg = TRUE;
		}
	}
	if (bFlg) {
		m_pChar->byDirection = byDirection;
		m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
		m_dwStuckTime	= dwTime;
		m_dwLostTime	= 0;
		return TRUE;
	}

	/* 探索範囲から外れた？ */
	nDX = (int)pCharTmp->dwX - (int)m_pChar->dwX;
	nDY = (int)pCharTmp->dwY - (int)m_pChar->dwY;
	if ((abs (nDX) > m_pChar->bySearchCx + SEARCH_LOSTMARGIN) ||
		(abs (nDY) > m_pChar->bySearchCy + SEARCH_LOSTMARGIN)) {
		if (m_dwLostTime == 0) {
			m_dwLostTime = dwTime;
		} else if ((DWORD)(dwTime - m_dwLostTime) >= SEARCH_LOSTTIME) {
			/* 見失ったのであきらめる */
			DropTarget ();
			return FALSE;
		}
	} else {
		m_dwLostTime = 0;
	}

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		m_dwStuckTime = dwTime;
		return FALSE;
	}

	/* 近い方へ1歩（遠い方の軸の移動確率で歩くかを決める） */
	if (abs (nDX) > abs (nDY)) {
		nRate = m_pChar->byMoveProbabilityX;
	} else {
		nRate = m_pChar->byMoveProbabilityY;
	}
	if (nRate == 0) {
		nRate = SEARCH_DEFAULTCHASE;
	}
	if ((int)(genrand () % 100) >= nRate) {
		return FALSE;
	}

	if (StepToward (pCharTmp, (int)pCharTmp->dwX, (int)pCharTmp->dwY)) {
		m_dwStuckTime = dwTime;
		return TRUE;
	}

	/* しばらく進めない？ */
	if ((DWORD)(dwTime - m_dwStuckTime) >= SEARCH_STUCKTIME) {
		DropTarget ();
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::ProcWander										 */
/* 内容：	開始位置の近くをうろつく										 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::ProcWander(DWORD dwTime)
{
	int nDirection, x, y;

	if ((int)(dwTime - m_dwRestEnd) < 0) {
		return FALSE;
	}
	/* 次の休み（2〜6秒） */
	m_dwRestEnd = dwTime + 2000 + (DWORD)(genrand () % 4000);

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		return FALSE;
	}

	nDirection = (int)(genrand () % 4);
	x = (int)m_pChar->dwX + anPosAddTblX[nDirection];
	y = (int)m_pChar->dwY + anPosAddTblY[nDirection];
	/* うろつく範囲の外へは出ない */
	if ((abs (x - (int)m_pChar->dwStartX) > GetWanderArea (TRUE)) ||
		(abs (y - (int)m_pChar->dwStartY) > GetWanderArea (FALSE))) {
		/* 範囲の外にいる（押された等）なら戻る */
		if ((abs ((int)m_pChar->dwX - (int)m_pChar->dwStartX) > GetWanderArea (TRUE)) ||
			(abs ((int)m_pChar->dwY - (int)m_pChar->dwStartY) > GetWanderArea (FALSE))) {
			m_bGoHome		= TRUE;
			m_dwStuckTime	= dwTime;
		}
		return FALSE;
	}

	return StepDirection (NULL, nDirection);
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::ProcGoHome										 */
/* 内容：	開始位置へ歩いて戻る											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::ProcGoHome(DWORD dwTime)
{
	/* うろつく範囲まで戻った？ */
	if ((abs ((int)m_pChar->dwX - (int)m_pChar->dwStartX) <= GetWanderArea (TRUE)) &&
		(abs ((int)m_pChar->dwY - (int)m_pChar->dwStartY) <= GetWanderArea (FALSE))) {
		m_bGoHome	= FALSE;
		m_dwRestEnd	= dwTime + 1000 + (DWORD)(genrand () % 2000);
		return FALSE;
	}

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		m_bGoHome = FALSE;
		return FALSE;
	}

	if (StepToward (NULL, (int)m_pChar->dwStartX, (int)m_pChar->dwStartY)) {
		m_dwStuckTime = dwTime;
		return TRUE;
	}

	/* 10秒以上進めない？ 最後の手段として開始地点へ移動する（従来の動き） */
	if ((DWORD)(dwTime - m_dwStuckTime) >= SEARCH_STUCKTIME) {
		m_bGoHome = FALSE;
		m_pChar->dwX = m_pChar->dwStartX;
		m_pChar->dwY = m_pChar->dwStartY;
		m_pChar->dwLastDotMove = dwTime;
		SendCmdCHARPOS (m_pChar->pID, m_pChar->dwTblID, 1, 2);
		return TRUE;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::FindTarget										 */
/* 内容：	探索範囲で一番近いプレイヤーを探す(いなければ -1)				 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

DWORD CCharProcSEARCH::FindTarget(void)
{
	int j, nDX, nDY, nDistance, nDistanceMin;
	DWORD dwRet;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	dwRet = (DWORD)-1;
	nDistanceMin = 0;

	pMap = g_aMapInfo[m_pChar->dwMapID];
	pData = pMap->paTblNoPC;
	for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if ((pCharTmp->wHP == 0) || (pCharTmp->byGameState < GAMESTATE_MOVE)) {
			continue;
		}
		nDX = abs ((int)pCharTmp->dwX - (int)m_pChar->dwX);
		nDY = abs ((int)pCharTmp->dwY - (int)m_pChar->dwY);
		if ((nDX > m_pChar->bySearchCx) || (nDY > m_pChar->bySearchCy)) {
			continue;
		}
		nDistance = nDX + nDY;
		if ((dwRet == -1) || (nDistance < nDistanceMin)) {
			dwRet = pCharTmp->dwTblID;
			nDistanceMin = nDistance;
		}
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::DropTarget										 */
/* 内容：	ターゲットを解除して帰り始める									 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

void CCharProcSEARCH::DropTarget(void)
{
	m_pChar->dwTargetTblID	= -1;
	m_pChar->dwLastBattle	= 0;
	m_dwTargetLast	= (DWORD)-1;
	m_dwLostTime	= 0;
	m_dwStuckTime	= m_dwLastTimeProc;
	m_bGoHome		= TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::IsTargetValid									 */
/* 内容：	ターゲットがまだ追える状態か									 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::IsTargetValid(PCHARINFO pCharTmp)
{
	if (pCharTmp == NULL) {
		return FALSE;
	}
	if (pCharTmp->byGameState == GAMESTATE_LOGOFF) {
		return FALSE;
	}
	if (pCharTmp->dwMapID != m_pChar->dwMapID) {
		return FALSE;
	}
	/* 倒れたプレイヤーは追いかけない */
	if ((m_pChar->byNpcType == NPCTYPE_BATTLE2) && (pCharTmp->wHP == 0)) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::CanStep										 */
/* 内容：	そのマスへ1歩進めるか											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::CanStep(PCHARINFO pCharTmp, int x, int y)
{
	PMAPINFO pMap;

	pMap = g_aMapInfo[m_pChar->dwMapID];
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}
	/* 障害物・イベント（戦闘２はぶつかったプレイヤーにターゲットを変える） */
	if (NpcProcSEARCH_Check (m_pChar, pCharTmp, x, y) == FALSE) {
		return FALSE;
	}
	/* プレイヤーと重なる？ */
	if (CheckCharBlockPC (m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) {
		return FALSE;
	}
	/* ほかのNPCと重なる？ */
	if (CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::StepToward										 */
/* 内容：	目標のマスへ1歩近づく											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::StepToward(PCHARINFO pCharTmp, int nX, int nY)
{
	int nDX, nDY, nDirMain, nDirSub, nSide;

	nDX = nX - (int)m_pChar->dwX;
	nDY = nY - (int)m_pChar->dwY;
	if ((nDX == 0) && (nDY == 0)) {
		return FALSE;
	}

	/* 遠い方の軸を先に、だめならもう一方の軸 */
	nDirMain = -1;
	nDirSub = -1;
	if (abs (nDX) >= abs (nDY)) {
		nDirMain = (nDX < 0) ? 2 : 3;
		if (nDY) {
			nDirSub = (nDY < 0) ? 0 : 1;
		}
	} else {
		nDirMain = (nDY < 0) ? 0 : 1;
		if (nDX) {
			nDirSub = (nDX < 0) ? 2 : 3;
		}
	}
	if (StepDirection (pCharTmp, nDirMain)) {
		return TRUE;
	}
	if ((nDirSub >= 0) && StepDirection (pCharTmp, nDirSub)) {
		return TRUE;
	}

	/* まっすぐ前がふさがっている時は横へよける */
	if (nDirSub < 0) {
		nSide = (nDirMain >= 2) ? 0 : 2;
		if (genrand () % 2) {
			nSide ++;
		}
		if (StepDirection (pCharTmp, nSide)) {
			return TRUE;
		}
		if (StepDirection (pCharTmp, abyDirectionTbl[nSide])) {
			return TRUE;
		}
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::StepDirection									 */
/* 内容：	指定の向きへ1歩進む												 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::StepDirection(PCHARINFO pCharTmp, int nDirection)
{
	int x, y;

	x = (int)m_pChar->dwX + anPosAddTblX[nDirection];
	y = (int)m_pChar->dwY + anPosAddTblY[nDirection];
	if (CanStep (pCharTmp, x, y) == FALSE) {
		return FALSE;
	}

	m_pChar->byDirection = (BYTE)nDirection;
	m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
	m_pChar->dwX = (DWORD)x;
	m_pChar->dwY = (DWORD)y;
	m_pChar->byFlg |= CHARINFOFLG_MOVE;
	m_pChar->dwLastDotMove = g_dwTime;

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::GetWanderArea									 */
/* 内容：	うろつく範囲(行動範囲、ただし最大3マス。未設定は2マス)			 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

int CCharProcSEARCH::GetWanderArea(BOOL bX)
{
	int nRet;

	if (m_pChar->byAreaX == 0) {
		return 2;
	}
	nRet = bX ? m_pChar->byAreaX : m_pChar->byAreaY;
	nRet = min (nRet, 3);

	return nRet;
}

/* Copyright(C)URARA-works 2006 */
