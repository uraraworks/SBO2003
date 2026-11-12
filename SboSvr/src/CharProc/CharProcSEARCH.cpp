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

	2026/11/12 敵のアルゴリズム改善
	・見つける時は、間に通れないマス（壁など）が無いかを見る。壁の向こうの人には気づかない
	  （以前は壁越しに気づいて、壁に張り付いたまま「あきらめる→すぐ見つけ直す」をくり返していた）
	・あきらめたら、家に着くまで（最大10秒）は新しく探さない。家に着いたらHPを満タンに戻す
	・開始位置から離れすぎたら（探索範囲の2倍＋4マス）あきらめる（どこまでも引っぱって行けないように）
	・まっすぐ進めない時は、近くの範囲で道をさがして回り込む（横へよけるくじ引きはやめた）。
	  行き先は「相手のとなりの空いているマス」なので、何匹かいると囲むように動く
	・進めるかを調べるだけでは狙う相手を変えない（以前はよける候補のマスにいた人へ変わっていた）。
	  まっすぐ進む先を別の人がふさいでいる時だけ、その人に狙いを変える
	・マップから人がいなくなったら、開始位置に戻ってHPを満タンにする
*/

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSendCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrQuest.h"
#include "CharProcSEARCH.h"
#include <vector>

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
/* あきらめた後、新しく探さずに帰る長さ(ms) */
#define SEARCH_GIVEUPTIME		(10000)
/* 開始位置から離れてよい距離：探索範囲 × 2 ＋ これ(マス) */
#define SEARCH_LEASHMARGIN		(4)
/* 道さがしで見る範囲：自分と目標を囲む四角の外側にこれだけ広げる(マス) */
#define SEARCH_PATHMARGIN		(4)
/* 道さがしで見る範囲の一辺の最大(マス) */
#define SEARCH_PATHMAX			(32)

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
	m_bGiveUp		= FALSE;
	m_dwGiveUpEnd	= 0;
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
		/* 戦っていた・帰る途中なら、開始位置に戻ってHPを満タンにしておく */
		if ((m_pChar->dwTargetTblID != -1) || m_bGoHome) {
			m_pChar->dwTargetTblID	= -1;
			m_pChar->dwLastBattle	= 0;
			m_dwTargetLast	= (DWORD)-1;
			m_bGoHome		= FALSE;
			m_bGiveUp		= TRUE;
			if ((m_pChar->dwX != m_pChar->dwStartX) || (m_pChar->dwY != m_pChar->dwStartY)) {
				m_pChar->dwX = m_pChar->dwStartX;
				m_pChar->dwY = m_pChar->dwStartY;
				m_pChar->dwLastDotMove = dwTime;
				SendCmdCHARPOS (m_pChar->pID, m_pChar->dwTblID, 1, 2);
			}
			ResetAtHome (dwTime);
		}
		goto Exit;
	}

	/* ターゲットがいる？（攻撃されて設定された時もここへ来る） */
	if (m_pChar->dwTargetTblID != -1) {
		bRet = ProcTarget (dwTime);
		goto Exit;
	}
	m_dwTargetLast = (DWORD)-1;

	/* あきらめて帰る途中は、しばらく新しく探さない */
	if (m_bGoHome && m_bGiveUp && ((int)(dwTime - m_dwGiveUpEnd) < 0)) {
		bRet = ProcGoHome (dwTime);
		goto Exit;
	}

	/* 探索範囲で一番近いプレイヤーを探す */
	dwTblID = FindTarget ();
	if ((dwTblID != -1) && (genrand () % 100 >= SEARCH_MISSRATE)) {
		pCharTmp = g_aCharInfo[dwTblID];
		m_pChar->dwTargetTblID = dwTblID;
		m_bGoHome	= FALSE;
		m_bGiveUp	= FALSE;
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
		m_bGiveUp		= FALSE;
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
			DropTarget (TRUE);
			return FALSE;
		}
	} else {
		m_dwLostTime = 0;
	}

	/* 開始位置から離れすぎた？ */
	if (IsOverLeash ()) {
		DropTarget (TRUE);
		return FALSE;
	}

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		m_dwStuckTime = dwTime;
		return FALSE;
	}

	/* しばらく進めない？（くじに外れて止まっている間も数える） */
	if ((DWORD)(dwTime - m_dwStuckTime) >= SEARCH_STUCKTIME) {
		DropTarget (TRUE);
		return FALSE;
	}

	/* 近い方へ1歩（進む軸の移動確率で歩くかを決める。StepToward と同じく同じ距離なら横） */
	if (abs (nDX) >= abs (nDY)) {
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
	/* 相手のすぐ近く（2マス以内）で詰まっているのは、仲間の後ろで順番待ちなので数えない */
	if (abs (nDX) + abs (nDY) <= 2) {
		m_dwStuckTime = dwTime;
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
		ResetAtHome (dwTime);
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
		ResetAtHome (dwTime);
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
		/* 別の空間にいる人には気づかない */
		if (!IsSameSpace (m_pChar, pCharTmp)) {
			continue;
		}
		nDX = abs ((int)pCharTmp->dwX - (int)m_pChar->dwX);
		nDY = abs ((int)pCharTmp->dwY - (int)m_pChar->dwY);
		if ((nDX > m_pChar->bySearchCx) || (nDY > m_pChar->bySearchCy)) {
			continue;
		}
		nDistance = nDX + nDY;
		if ((dwRet != -1) && (nDistance >= nDistanceMin)) {
			continue;
		}
		/* 壁の向こうの人には気づかない（見通せるかは近い人だけ調べる） */
		if (IsInSight (pCharTmp) == FALSE) {
			continue;
		}
		dwRet = pCharTmp->dwTblID;
		nDistanceMin = nDistance;
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::DropTarget										 */
/* 内容：	ターゲットを解除して帰り始める									 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

void CCharProcSEARCH::DropTarget(BOOL bGiveUp)
{
	m_pChar->dwTargetTblID	= -1;
	m_pChar->dwLastBattle	= 0;
	m_dwTargetLast	= (DWORD)-1;
	m_dwLostTime	= 0;
	m_dwStuckTime	= m_dwLastTimeProc;
	m_bGoHome		= TRUE;
	/* 追うのをあきらめた時は、しばらく新しく探さずに帰り、着いたらHPを戻す。
	   相手が倒れた・いなくなった時は、帰りながら近くの人を探す */
	m_bGiveUp		= bGiveUp;
	m_dwGiveUpEnd	= m_dwLastTimeProc + SEARCH_GIVEUPTIME;
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
	if ((pCharTmp->dwMapID != m_pChar->dwMapID) || !IsSameSpace (m_pChar, pCharTmp)) {
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

BOOL CCharProcSEARCH::CanStep(PCHARINFO pCharTmp, int x, int y, PDWORD pdwBlockPC)
{
	PMAPINFO pMap;
	DWORD dwBlockPC, dwTargetBack;
	BOOL bResult;

	if (pdwBlockPC) {
		*pdwBlockPC = (DWORD)-1;
	}
	pMap = g_aMapInfo[m_pChar->dwMapID];
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}
	/* 障害物・イベント。NpcProcSEARCH_Check は戦闘２だとふさいでいるプレイヤーへ
	   狙いを変えるが、ここでは調べるだけなので狙いは戻す（変えるかは ProcTarget が決める） */
	dwBlockPC		= (DWORD)-1;
	dwTargetBack	= m_pChar->dwTargetTblID;
	bResult = NpcProcSEARCH_Check (m_pChar, pCharTmp, x, y, &dwBlockPC);
	m_pChar->dwTargetTblID = dwTargetBack;
	if (pdwBlockPC) {
		*pdwBlockPC = dwBlockPC;
	}
	if (bResult == FALSE) {
		return FALSE;
	}
	/* PCの配置・状態は変わっていないので、直前の判定を使う */
	if (dwBlockPC != -1) {
		return FALSE;
	}
	/* ほかのNPCと重なる？ */
	if (CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y, m_pChar->dwSpaceID) != -1) {
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
	int nDX, nDY, nDirMain, nDirSub, nDir;
	DWORD dwBlockPC;

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
	/* 追いかけている（戦闘２）時に、まっすぐ進む先を別のプレイヤーがふさいでいる？
	   目の前のその人に狙いを変える（従来の「ぶつかったキャラにタゲ変更」） */
	if (pCharTmp && (m_pChar->byNpcType == NPCTYPE_BATTLE2)) {
		CanStep (pCharTmp, (int)m_pChar->dwX + anPosAddTblX[nDirMain], (int)m_pChar->dwY + anPosAddTblY[nDirMain], &dwBlockPC);
		if ((dwBlockPC != -1) && (dwBlockPC != pCharTmp->dwTblID) && (g_aCharInfo[dwBlockPC]->wHP > 0)) {
			m_pChar->dwTargetTblID = dwBlockPC;
			if (m_pChar->byDirection != (BYTE)nDirMain) {
				m_pChar->byDirection = (BYTE)nDirMain;
				m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			}
			return TRUE;
		}
	}
	if ((nDirSub >= 0) && StepDirection (pCharTmp, nDirSub)) {
		return TRUE;
	}

	/* まっすぐ近づけない時は、近くの範囲で道をさがして回り込む */
	nDir = FindPathStep (pCharTmp, nX, nY);
	if ((nDir >= 0) && StepDirection (pCharTmp, nDir)) {
		return TRUE;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::FindPathStep									 */
/* 内容：	近くの範囲で道をさがして、最初の1歩の向きを返す(無ければ -1)	 */
/*			追いかける時の行き先は「相手のとなりの空いているマス」、		 */
/*			帰る時は開始位置。自分と行き先を囲む四角を少し広げた範囲だけを	 */
/*			幅優先でさがす（まっすぐ進めない時だけ呼ばれる）				 */
/* 日付：	2026/11/12														 */
/* ========================================================================= */

int CCharProcSEARCH::FindPathStep(PCHARINFO pCharTmp, int nX, int nY)
{
	int i, j, x, y, nX1, nY1, nX2, nY2, nW, nH, nPos, nStart, nHead, nTail, nFound, nRet;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharBlock;
	std::vector<BYTE> abyBlock, abyGoal;
	std::vector<short> anPrev;
	std::vector<int> anQueue;

	nRet = -1;
	pMap = g_aMapInfo[m_pChar->dwMapID];

	/* さがす範囲 */
	nX1 = max (min ((int)m_pChar->dwX, nX) - SEARCH_PATHMARGIN, 0);
	nY1 = max (min ((int)m_pChar->dwY, nY) - SEARCH_PATHMARGIN, 0);
	nX2 = min (max ((int)m_pChar->dwX, nX) + SEARCH_PATHMARGIN, pMap->nCX - 1);
	nY2 = min (max ((int)m_pChar->dwY, nY) + SEARCH_PATHMARGIN, pMap->nCY - 1);
	nW = nX2 - nX1 + 1;
	nH = nY2 - nY1 + 1;
	if ((nW <= 0) || (nH <= 0) || (nW > SEARCH_PATHMAX) || (nH > SEARCH_PATHMAX)) {
		return -1;
	}

	/* 通れないマス（地形・イベント・ほかのキャラ） */
	abyBlock.assign (nW * nH, 0);
	for (y = 0; y < nH; y ++) {
		for (x = 0; x < nW; x ++) {
			if ((CheckCharBlock (pMap->dwID, (DWORD)(nX1 + x), (DWORD)(nY1 + y)) == FALSE) ||
				CheckEventBlock (pMap->dwID, (DWORD)(nX1 + x), (DWORD)(nY1 + y))) {
				abyBlock[x + y * nW] = 1;
			}
		}
	}
	for (j = 0; j < 2; j ++) {
		int nCount = (j == 0) ? (int)pMap->dwCountTblNoPC : (int)pMap->dwCountTblNoNPC;
		pData = (j == 0) ? pMap->paTblNoPC : pMap->paTblNoNPC;
		for (i = 0; i < nCount; i ++) {
			if (pData[i] == -1) {
				break;
			}
			pCharBlock = g_aCharInfo[pData[i]];
			if ((pCharBlock == m_pChar) || (pCharBlock->byNpcType == NPCTYPE_PUT) || !IsInteractable (m_pChar, pCharBlock)) {
				continue;
			}
			x = (int)pCharBlock->dwX - nX1;
			y = (int)pCharBlock->dwY - nY1;
			if ((x >= 0) && (x < nW) && (y >= 0) && (y < nH)) {
				abyBlock[x + y * nW] = 1;
			}
		}
	}

	/* 行き先 */
	abyGoal.assign (nW * nH, 0);
	nFound = 0;
	if (pCharTmp) {
		for (i = 0; i < 4; i ++) {
			x = nX + anPosAddTblX[i] - nX1;
			y = nY + anPosAddTblY[i] - nY1;
			if ((x >= 0) && (x < nW) && (y >= 0) && (y < nH) && (abyBlock[x + y * nW] == 0)) {
				abyGoal[x + y * nW] = 1;
				nFound ++;
			}
		}
	} else {
		x = nX - nX1;
		y = nY - nY1;
		if ((x >= 0) && (x < nW) && (y >= 0) && (y < nH)) {
			abyGoal[x + y * nW] = 1;
			abyBlock[x + y * nW] = 0;
			nFound ++;
		}
	}
	if (nFound == 0) {
		return -1;
	}

	/* 自分の場所から幅優先でさがす */
	nStart = ((int)m_pChar->dwX - nX1) + ((int)m_pChar->dwY - nY1) * nW;
	anPrev.assign (nW * nH, -1);
	anQueue.assign (nW * nH, 0);
	anPrev[nStart] = (short)nStart;
	nHead = nTail = 0;
	anQueue[nTail ++] = nStart;
	nFound = -1;
	while (nHead < nTail) {
		nPos = anQueue[nHead ++];
		if (abyGoal[nPos]) {
			nFound = nPos;
			break;
		}
		for (i = 0; i < 4; i ++) {
			x = nPos % nW + anPosAddTblX[i];
			y = nPos / nW + anPosAddTblY[i];
			if ((x < 0) || (x >= nW) || (y < 0) || (y >= nH)) {
				continue;
			}
			j = x + y * nW;
			if (abyBlock[j] || (anPrev[j] != -1)) {
				continue;
			}
			anPrev[j] = (short)nPos;
			anQueue[nTail ++] = j;
		}
	}
	if ((nFound < 0) || (nFound == nStart)) {
		return -1;
	}

	/* 最初の1歩までたどって戻る */
	nPos = nFound;
	while (anPrev[nPos] != nStart) {
		nPos = anPrev[nPos];
	}
	x = nPos % nW - nStart % nW;
	y = nPos / nW - nStart / nW;
	for (i = 0; i < 4; i ++) {
		if ((anPosAddTblX[i] == x) && (anPosAddTblY[i] == y)) {
			nRet = i;
			break;
		}
	}
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::IsInSight										 */
/* 内容：	間に通れないマスが無く見通せるか（となりなら見える）			 */
/* 日付：	2026/11/12														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::IsInSight(PCHARINFO pCharTmp)
{
	int x0, y0, x1, y1, nDX, nDY, nSX, nSY, nErr, nErr2;

	x0 = (int)m_pChar->dwX;
	y0 = (int)m_pChar->dwY;
	x1 = (int)pCharTmp->dwX;
	y1 = (int)pCharTmp->dwY;
	nDX = abs (x1 - x0);
	nDY = -abs (y1 - y0);
	nSX = (x0 < x1) ? 1 : -1;
	nSY = (y0 < y1) ? 1 : -1;
	nErr = nDX + nDY;
	/* ブレゼンハムの線で、両端を除いたマスを調べる */
	for (;;) {
		nErr2 = nErr * 2;
		if (nErr2 >= nDY) {
			nErr += nDY;
			x0 += nSX;
		}
		if (nErr2 <= nDX) {
			nErr += nDX;
			y0 += nSY;
		}
		if ((x0 == x1) && (y0 == y1)) {
			break;
		}
		if (CheckCharBlock (m_pChar->dwMapID, (DWORD)x0, (DWORD)y0) == FALSE) {
			return FALSE;
		}
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::IsOverLeash									 */
/* 内容：	開始位置から離れすぎたか(探索範囲 × 2 ＋ 4 マス)				 */
/* 日付：	2026/11/12														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::IsOverLeash(void)
{
	int nLeashX, nLeashY;

	nLeashX = max ((int)m_pChar->bySearchCx, GetWanderArea (TRUE)) * 2 + SEARCH_LEASHMARGIN;
	nLeashY = max ((int)m_pChar->bySearchCy, GetWanderArea (FALSE)) * 2 + SEARCH_LEASHMARGIN;
	if ((abs ((int)m_pChar->dwX - (int)m_pChar->dwStartX) > nLeashX) ||
		(abs ((int)m_pChar->dwY - (int)m_pChar->dwStartY) > nLeashY)) {
		return TRUE;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::ResetAtHome									 */
/* 内容：	あきらめて家に着いた：HPを満タンに戻し、受けたダメージの記録を消す	 */
/* 日付：	2026/11/12														 */
/* ========================================================================= */

void CCharProcSEARCH::ResetAtHome(DWORD dwTime)
{
	if (m_bGiveUp == FALSE) {
		return;
	}
	m_bGiveUp = FALSE;
	if (m_pChar->wHP == 0) {
		return;
	}
	ClearBattleDamage (m_pChar->dwTblID);
	if (m_pChar->wHP < m_pChar->wMaxHP) {
		m_pChar->wHP = m_pChar->wMaxHP;
		SendCmdCHARSTATUSHPMANA (m_pChar->dwTblID, m_pChar->dwTblID);
	}
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
