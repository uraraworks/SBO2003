/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcMOVERAND.cpp										 */
/* 内容：		キャラ処理(ランダム移動)クラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */
/*
	歩き方（2026/11 改修）
	・以前は 0.5 秒ごとに「向きだけ変える」「今の向きへ1歩」をくじ引きしていたため、
	  1歩ごとに棒立ちになり、壁の方を向くとしばらく固まっていた。
	・今は「何歩かまとめて歩く → 少し立ち止まって休む」をくり返す。
	  - 歩き出す時は、歩ける向きの中から選ぶ（壁・行動範囲の外・イベントのマスは選ばない）
	  - まとめ歩きは 2〜4 歩。途中で進めなくなったらそこで休む
	  - 休む長さは 1〜4 秒が基本で、移動確率(byMoveProbabilityX/Y)が高いほど短く、低いほど長い
	    （未設定の 0 は従来どおり 30% 扱い。横と縦の確率は歩き出す向きの選ばれやすさにも使う）
	  - 休みの途中で、ときどき一度だけ見回す
	・ランダム移動の村人は、目の前でプレイヤーと向かい合ったら立ち止まってそちらを向く
	  （話しかける時は必ず向かい合うので、話している途中に歩き去らない）
	・サーバー負荷を増やさないよう、1回の順番でやることは1つだけ（1歩・向き変え・休み判定のどれか）。
*/

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcMOVERAND.h"

/* 1歩ごとの間隔(ms)。クライアントは1マスを約 0.28 秒で歩くので、それより少し長くする */
#define MOVERAND_STEPTIME		(320)
/* 休み中に判定する間隔(ms) */
#define MOVERAND_RESTTIME		(500)
/* 移動確率が未設定(0)の時の値(%)。従来と同じ */
#define MOVERAND_DEFAULTPROB	(30)
/* 休みの長さ(ms)の上限。移動確率がとても低い時用 */
#define MOVERAND_RESTMAX		(60000)
/* プレイヤーと向かい合った時に立ち止まる長さ(ms) */
#define MOVERAND_TALKREST		(3000)

/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::CCharProcMOVERAND							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcMOVERAND::CCharProcMOVERAND()
{
	m_bInit			= FALSE;
	m_bLooked		= FALSE;
	m_nWalkLeft		= 0;
	m_dwRestStart	= 0;
	m_dwRestEnd		= 0;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;
	DWORD dwInterval;
	PMAPINFO pMap;

	bRet = FALSE;

	/* 歩いている間は1歩の間隔、休んでいる間は休みの判定間隔で処理する */
	dwInterval = (m_nWalkLeft > 0) ? MOVERAND_STEPTIME : MOVERAND_RESTTIME;
	if ((DWORD)(dwTime - m_dwLastTimeProc) < dwInterval) {
		goto Exit;
	}
	/* 最後に動いてから1マス歩ききる時間がたっていない（押された・移動パーツ等） */
	if ((DWORD)(dwTime - m_pChar->dwLastDotMove) < MOVERAND_STEPTIME) {
		goto Exit;
	}
	m_dwLastTimeProc = dwTime;

	if (m_bInit == FALSE) {
		/* みんなが同時に歩き出さないよう、最初の休みをばらけさせる */
		m_bInit = TRUE;
		m_dwRestStart	= dwTime;
		m_dwRestEnd		= dwTime + (DWORD)(genrand () % 3000);
	}

	bResult = PartsMoveProc (m_pChar);
	if (bResult) {
		m_pChar->dwLastDotMove = dwTime;
		m_nWalkLeft = 0;
		StartRest (dwTime, TRUE);
		goto Exit;
	}

	pMap = g_aMapInfo[m_pChar->dwMapID];
	/* 誰もいない？ */
	if (pMap->dwCountTblNoPC == 0) {
		goto Exit;
	}

	if ((m_pChar->byNpcType == NPCTYPE_BATTLE1) && (m_pChar->dwLastBattle)) {
		/* 15秒以上移動してない？ */
		if (dwTime >= m_pChar->dwLastDotMove + 15000) {
			if ((m_pChar->dwX != m_pChar->dwStartX) || (m_pChar->dwY != m_pChar->dwStartY)) {
				/* ターゲット解除 */
				m_pChar->dwLastBattle = 0;
				m_pChar->dwTargetTblID = -1;
			}
		} else {
			m_nWalkLeft = 0;
			goto Exit;
		}
	}

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		goto Exit;
	}

	/* 目の前でプレイヤーと向かい合っている？ */
	if (CheckTalkPC ()) {
		m_nWalkLeft = 0;
		m_dwRestStart	= dwTime;
		m_dwRestEnd		= dwTime + MOVERAND_TALKREST;
		m_bLooked		= TRUE;
		bRet = TRUE;
		goto Exit;
	}

	/* まとめ歩きの途中？ */
	if (m_nWalkLeft > 0) {
		if (StepFront ()) {
			m_nWalkLeft --;
			if (m_nWalkLeft <= 0) {
				StartRest (dwTime, FALSE);
			}
		} else {
			/* 進めなくなったのでここで休む */
			m_nWalkLeft = 0;
			StartRest (dwTime, TRUE);
		}
		bRet = TRUE;
		goto Exit;
	}

	/* 休み中？ */
	if ((int)(dwTime - m_dwRestEnd) < 0) {
		/* 休みの半分を過ぎたら、ときどき一度だけ見回す */
		if ((m_bLooked == FALSE) && ((DWORD)(dwTime - m_dwRestStart) >= (m_dwRestEnd - m_dwRestStart) / 2)) {
			m_bLooked = TRUE;
			if (genrand () % 100 < 35) {
				BYTE byDirection = (BYTE)(genrand () % 4);
				if (byDirection != m_pChar->byDirection) {
					m_pChar->byDirection = byDirection;
					m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
				}
			}
		}
		goto Exit;
	}

	/* 休み終わり：歩き出す向きを選ぶ */
	{
		int nDirection = ChooseDirection ();
		if (nDirection < 0) {
			/* どこにも行けないので向きだけ変えてまた休む */
			m_pChar->byDirection = (BYTE)(genrand () % 4);
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			StartRest (dwTime, FALSE);
			goto Exit;
		}
		if (m_pChar->byDirection != (BYTE)nDirection) {
			m_pChar->byDirection = (BYTE)nDirection;
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
		}
		/* 2〜4歩まとめて歩く（1歩目は今すぐ） */
		m_nWalkLeft = 2 + (int)(genrand () % 3);
		if (StepFront ()) {
			m_nWalkLeft --;
		} else {
			m_nWalkLeft = 0;
			StartRest (dwTime, TRUE);
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::IsMovePos									 */
/* 内容：	そのマスへ歩けるか判定											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::IsMovePos(int x, int y)
{
	int j;
	BYTE byHitType;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	pMap = g_aMapInfo[m_pChar->dwMapID];
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}

	/* 障害物チェック（従来と同じく、平地・移動パーツ・隠れる1だけ歩ける） */
	byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;
	if (!((byHitType == PARTSHIT_NONE) ||
		  (byHitType == PARTSHIT_MOVEFRONT) ||
		  (byHitType == PARTSHIT_MOVEUP) ||
		  (byHitType == PARTSHIT_MOVEDOWN) ||
		  (byHitType == PARTSHIT_MOVERIGHT) ||
		  (byHitType == PARTSHIT_MOVELEFT) ||
		  (byHitType == PARTSHIT_HIDE1))) {
		return FALSE;
	}

	/* 移動範囲が設定されている？ */
	if (m_pChar->byAreaX) {
		if ((abs ((int)m_pChar->dwStartX - x) > m_pChar->byAreaX) ||
			(abs ((int)m_pChar->dwStartY - y) > m_pChar->byAreaY)) {
			/* 範囲の外にいる時（押された等）は、開始位置に近づく向きだけ歩ける。
			   以前は範囲外に出るとどこへも歩けず固まっていた */
			if ((abs ((int)m_pChar->dwStartX - x) + abs ((int)m_pChar->dwStartY - y)) >=
				(abs ((int)m_pChar->dwStartX - (int)m_pChar->dwX) + abs ((int)m_pChar->dwStartY - (int)m_pChar->dwY))) {
				return FALSE;
			}
		}
	}

	/* イベントが配置されている？（以前は移動範囲を設定すると判定されていなかった） */
	if (CheckEventBlock (m_pChar->dwMapID, (DWORD)x, (DWORD)y)) {
		return FALSE;
	}

	/* プレイヤーと重なる？ */
	pData = pMap->paTblNoPC;
	for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if ((pCharTmp->dwX == (DWORD)x) && (pCharTmp->dwY == (DWORD)y)) {
			return FALSE;
		}
	}

	/* ほかのNPCと重なる？（以前はNPC同士は重なれた） */
	if (CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) {
		return FALSE;
	}

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::GetMoveWeight								 */
/* 内容：	向きごとの歩きやすさ(移動確率)を取得							 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

int CCharProcMOVERAND::GetMoveWeight(int nDirection)
{
	int nRet;

	/* 0:上 1:下 は縦、2:左 3:右 は横 */
	nRet = (nDirection >= 2) ? m_pChar->byMoveProbabilityX : m_pChar->byMoveProbabilityY;
	if (nRet == 0) {
		nRet = MOVERAND_DEFAULTPROB;
	}
	nRet = min (nRet, 100);

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::ChooseDirection								 */
/* 内容：	歩き出す向きを選ぶ(歩ける向きが無ければ -1)						 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

int CCharProcMOVERAND::ChooseDirection(void)
{
	int i, nTotal, nTmp, anWeight[4];

	nTotal = 0;
	for (i = 0; i < 4; i ++) {
		anWeight[i] = 0;
		if (IsMovePos ((int)m_pChar->dwX + anPosAddTblX[i], (int)m_pChar->dwY + anPosAddTblY[i])) {
			anWeight[i] = GetMoveWeight (i);
			/* 今向いている方へは少し歩き出しやすくして、行ったり来たりを減らす */
			if (i == (int)m_pChar->byDirection) {
				anWeight[i] += anWeight[i] / 2;
			}
		}
		nTotal += anWeight[i];
	}
	if (nTotal <= 0) {
		return -1;
	}

	nTmp = (int)(genrand () % nTotal);
	for (i = 0; i < 4; i ++) {
		if (nTmp < anWeight[i]) {
			return i;
		}
		nTmp -= anWeight[i];
	}
	return -1;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::CheckTalkPC									 */
/* 内容：	目の前で向かい合っているプレイヤーがいれば振り向く				 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::CheckTalkPC(void)
{
	int i, j, x, y;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	/* 村人だけ（戦闘１の敵はこれまでどおり） */
	if (m_pChar->byNpcType != NPCTYPE_MOVERAND) {
		return FALSE;
	}

	pMap = g_aMapInfo[m_pChar->dwMapID];
	pData = pMap->paTblNoPC;
	for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if (pCharTmp->byDirection >= 4) {
			continue;
		}
		/* プレイヤーの目の前が自分？ */
		x = (int)pCharTmp->dwX + anPosAddTblX[pCharTmp->byDirection];
		y = (int)pCharTmp->dwY + anPosAddTblY[pCharTmp->byDirection];
		if ((x != (int)m_pChar->dwX) || (y != (int)m_pChar->dwY)) {
			continue;
		}
		/* プレイヤーの方を向く */
		i = abyDirectionTbl[pCharTmp->byDirection];
		if (m_pChar->byDirection != (BYTE)i) {
			m_pChar->byDirection = (BYTE)i;
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
		}
		return TRUE;
	}

	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::StartRest									 */
/* 内容：	立ち止まって休み始める											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

void CCharProcMOVERAND::StartRest(DWORD dwTime, BOOL bShort)
{
	int nProb;
	DWORD dwRest;

	/* 基本は 1〜4 秒（進めなくて止まった時は 0.5〜1.5 秒） */
	if (bShort) {
		dwRest = 500 + (DWORD)(genrand () % 1000);
	} else {
		dwRest = 1000 + (DWORD)(genrand () % 3000);
	}
	/* 移動確率が高いほど短く、低いほど長く休む（30% でそのまま） */
	nProb = max (GetMoveWeight (0), GetMoveWeight (2));
	dwRest = dwRest * MOVERAND_DEFAULTPROB / nProb;
	dwRest = min (dwRest, (DWORD)MOVERAND_RESTMAX);

	m_dwRestStart	= dwTime;
	m_dwRestEnd		= dwTime + dwRest;
	m_bLooked		= FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::StepFront									 */
/* 内容：	向いている方向へ1歩進む											 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::StepFront(void)
{
	int x, y;

	x = (int)m_pChar->dwX + anPosAddTblX[m_pChar->byDirection];
	y = (int)m_pChar->dwY + anPosAddTblY[m_pChar->byDirection];
	if (IsMovePos (x, y) == FALSE) {
		return FALSE;
	}

	m_pChar->dwX = (DWORD)x;
	m_pChar->dwY = (DWORD)y;
	m_pChar->byFlg |= CHARINFOFLG_MOVE;
	m_pChar->dwLastDotMove = g_dwTime;

	return TRUE;
}

/* Copyright(C)URARA-works 2006 */
