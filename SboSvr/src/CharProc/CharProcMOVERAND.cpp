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
	・巡回ルート（2026/11 追加）：「動かない」「ランダム移動」の村人に道しるべが2つ以上あると、
	  道しるべを順にたどって歩く。最後まで行ったら折り返すか最初に戻る（byRouteMode）。
	  各道しるべで byRouteWait 秒止まる。道しるべの間は1歩ずつ近い方へ進むだけで、経路探索はしない
	  （管理ページで、道しるべの間をまっすぐ歩けるように置く）。10秒進めなければ次の道しるべへ。
	  行動範囲は見ない。向かい合ったプレイヤーの方を向いて止まるのは同じ。
	・戦闘１の敵（2026/11/12 見直し）：攻撃されるとその場で止まって戦う（追いかけないのは従来どおり）。
	  相手がとなりにいればそちらを向く（横や後ろに回られても殴り返す）。
	  相手が近く（探索範囲＋2マス）から15秒離れていたら戦いをやめる。相手が倒れた・いなくなったらすぐやめる。
	  以前は「最後に歩いてから15秒」でやめていたため、戦闘中は歩かないので15秒を超える戦いで必ず離れていた。
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
/* 巡回ルートを歩く時の1歩の間隔(ms) */
#define MOVERAND_ROUTESTEPTIME	(400)
/* 巡回ルートで進めない時に次の道しるべへ移るまで(ms) */
#define MOVERAND_ROUTESTUCK	(10000)
/* 戦闘１：相手が近くにいなくなってから戦いをやめるまで(ms) */
#define MOVERAND_BATTLELOST		(15000)
/* 戦闘１：相手が近くにいるとみなす余裕(探索範囲＋これ。マス) */
#define MOVERAND_BATTLEMARGIN	(2)

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
	m_nRouteNo		= 0;
	m_nRouteDir		= 1;
	m_dwRouteWaitEnd	= 0;
	m_dwRouteStuck	= 0;
	m_dwBattleTarget	= (DWORD)-1;
	m_dwBattleTime		= 0;
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
	if (IsRoute ()) {
		dwInterval = MOVERAND_ROUTESTEPTIME;
	}
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
		/* 戦っている間はその場で止まる */
		m_nWalkLeft = 0;
		bRet = ProcBattle (dwTime);
		goto Exit;
	}
	m_dwBattleTarget = (DWORD)-1;

	/* 巡回ルートを歩く？ */
	if (IsRoute ()) {
		bRet = ProcRoute (dwTime);
		goto Exit;
	}

	/* 動かない設定？ */
	if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
		goto Exit;
	}

	/* 移動範囲の外にいる？（開始位置を変えた・押された等）開始位置へ戻して範囲の中だけを歩く。
	   範囲の外から歩いて戻らせると、戻る途中は範囲の外を歩き回って見えるため */
	if (m_pChar->byAreaX &&
		((abs ((int)m_pChar->dwStartX - (int)m_pChar->dwX) > m_pChar->byAreaX) ||
		 (abs ((int)m_pChar->dwStartY - (int)m_pChar->dwY) > m_pChar->byAreaY))) {
		m_nWalkLeft = 0;
		m_pChar->dwX = m_pChar->dwStartX;
		m_pChar->dwY = m_pChar->dwStartY;
		m_pChar->dwLastDotMove = dwTime;
		SendCmdCHARPOS (m_pChar->pID, m_pChar->dwTblID, 1, 2);
		StartRest (dwTime, FALSE);
		bRet = TRUE;
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

	/* 障害物チェック（平地・移動パーツ・隠れる1・足跡(雪/砂)・水の波紋だけ歩ける。足跡と波紋は以前は抜けていた） */
	byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;
	if (!((byHitType == PARTSHIT_NONE) ||
		  (byHitType == PARTSHIT_MOVEFRONT) ||
		  (byHitType == PARTSHIT_MOVEUP) ||
		  (byHitType == PARTSHIT_MOVEDOWN) ||
		  (byHitType == PARTSHIT_MOVERIGHT) ||
		  (byHitType == PARTSHIT_MOVELEFT) ||
		  (byHitType == PARTSHIT_HIDE1) ||
		  (byHitType == PARTSHIT_FOOTMARK_SNOW) ||
		  (byHitType == PARTSHIT_FOOTMARK_SAND) ||
		  (byHitType == PARTSHIT_WATERRIPPLE))) {
		return FALSE;
	}

	/* 移動範囲が設定されている？（巡回ルートを歩く時は見ない） */
	if (m_pChar->byAreaX && (IsRoute () == FALSE)) {
		/* 開始位置から移動範囲の外へは出ない（範囲の外にいる時は Proc で開始位置へ戻す） */
		if ((abs ((int)m_pChar->dwStartX - x) > m_pChar->byAreaX) ||
			(abs ((int)m_pChar->dwStartY - y) > m_pChar->byAreaY)) {
			return FALSE;
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
		/* 別の空間にいる人とは重ならない */
		if (!IsSameSpace (m_pChar, pCharTmp)) {
			continue;
		}
		if ((pCharTmp->dwX == (DWORD)x) && (pCharTmp->dwY == (DWORD)y)) {
			return FALSE;
		}
	}

	/* ほかのNPCと重なる？（以前はNPC同士は重なれた） */
	if (CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y, m_pChar->dwSpaceID) != -1) {
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
	if ((m_pChar->byNpcType != NPCTYPE_MOVERAND) && (m_pChar->byNpcType != NPCTYPE_NOMOVE)) {
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
		/* 別の空間にいる人には気づかない */
		if (!IsSameSpace (m_pChar, pCharTmp)) {
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

/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::IsRoute										 */
/* 内容：	巡回ルートを歩くか												 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::IsRoute(void)
{
	if (m_pChar->byRouteCount < 2) {
		return FALSE;
	}
	if ((m_pChar->byNpcType != NPCTYPE_MOVERAND) && (m_pChar->byNpcType != NPCTYPE_NOMOVE)) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::ProcRoute									 */
/* 内容：	巡回ルートを歩く(1回の順番で1歩だけ)							 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::ProcRoute(DWORD dwTime)
{
	int nX, nY, nDX, nDY, anDirection[2], i;

	/* 目の前でプレイヤーと向かい合っている間は止まる */
	if (CheckTalkPC ()) {
		m_dwRouteStuck = 0;
		return TRUE;
	}
	/* 道しるべで止まっている？ */
	if ((int)(dwTime - m_dwRouteWaitEnd) < 0) {
		return FALSE;
	}

	if ((m_nRouteNo < 0) || (m_nRouteNo >= m_pChar->byRouteCount)) {
		m_nRouteNo	= 0;
		m_nRouteDir	= 1;
	}
	nX = m_pChar->abyRouteX[m_nRouteNo];
	nY = m_pChar->abyRouteY[m_nRouteNo];
	nDX = nX - (int)m_pChar->dwX;
	nDY = nY - (int)m_pChar->dwY;

	/* 道しるべに着いた？ */
	if ((nDX == 0) && (nDY == 0)) {
		NextRoutePoint ();
		m_dwRouteStuck = 0;
		if (m_pChar->byRouteWait) {
			m_dwRouteWaitEnd = dwTime + (DWORD)m_pChar->byRouteWait * 1000;
		}
		return FALSE;
	}

	/* 遠い方の軸を先に、だめならもう一方の軸 */
	anDirection[0] = -1;
	anDirection[1] = -1;
	if (abs (nDX) >= abs (nDY)) {
		anDirection[0] = (nDX < 0) ? 2 : 3;
		if (nDY) {
			anDirection[1] = (nDY < 0) ? 0 : 1;
		}
	} else {
		anDirection[0] = (nDY < 0) ? 0 : 1;
		if (nDX) {
			anDirection[1] = (nDX < 0) ? 2 : 3;
		}
	}
	for (i = 0; i < 2; i ++) {
		if (anDirection[i] < 0) {
			continue;
		}
		if (IsMovePos ((int)m_pChar->dwX + anPosAddTblX[anDirection[i]], (int)m_pChar->dwY + anPosAddTblY[anDirection[i]])) {
			if (m_pChar->byDirection != (BYTE)anDirection[i]) {
				m_pChar->byDirection = (BYTE)anDirection[i];
				m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			}
			StepFront ();
			m_dwRouteStuck = 0;
			return TRUE;
		}
	}

	/* 進めない（人が立っている等）。しばらく待っても進めなければ次の道しるべへ */
	if (m_dwRouteStuck == 0) {
		m_dwRouteStuck = dwTime;
		if (m_dwRouteStuck == 0) {
			m_dwRouteStuck = 1;
		}
	} else if ((DWORD)(dwTime - m_dwRouteStuck) >= MOVERAND_ROUTESTUCK) {
		NextRoutePoint ();
		m_dwRouteStuck = 0;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::NextRoutePoint								 */
/* 内容：	次の道しるべへ													 */
/* 日付：	2026/11/06														 */
/* ========================================================================= */

void CCharProcMOVERAND::NextRoutePoint(void)
{
	int nCount = m_pChar->byRouteCount;

	if (m_pChar->byRouteMode == NPCROUTEMODE_LOOP) {
		/* 最後まで行ったら最初に戻る */
		m_nRouteNo = (m_nRouteNo + 1) % nCount;
		m_nRouteDir = 1;
		return;
	}

	/* 最後まで行ったら折り返す */
	if ((m_nRouteNo + m_nRouteDir < 0) || (m_nRouteNo + m_nRouteDir >= nCount)) {
		m_nRouteDir = -m_nRouteDir;
	}
	m_nRouteNo += m_nRouteDir;
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::ProcBattle									 */
/* 内容：	戦闘１の敵が戦っている間（その場で止まって相手の方を向く）		 */
/* 日付：	2026/11/12														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::ProcBattle(DWORD dwTime)
{
	int nDX, nDY, nRange;
	BYTE byDirection;
	BOOL bEnd;
	PCHARINFO pCharTmp;

	bEnd = FALSE;
	pCharTmp = NULL;
	if (m_pChar->dwTargetTblID == -1) {
		bEnd = TRUE;
	} else {
		pCharTmp = g_aCharInfo[m_pChar->dwTargetTblID];
		/* 相手が倒れた・いなくなった？ */
		if ((pCharTmp == NULL) || (pCharTmp->byGameState == GAMESTATE_LOGOFF) ||
			(pCharTmp->dwMapID != m_pChar->dwMapID) || !IsSameSpace (m_pChar, pCharTmp) || (pCharTmp->wHP == 0)) {
			bEnd = TRUE;
		}
	}

	if (bEnd == FALSE) {
		/* 新しい相手？ */
		if (m_dwBattleTarget != m_pChar->dwTargetTblID) {
			m_dwBattleTarget	= m_pChar->dwTargetTblID;
			m_dwBattleTime		= dwTime;
		}
		nDX = GetCharDistanceX (m_pChar, pCharTmp);
		nDY = (int)pCharTmp->dwY - (int)m_pChar->dwY;
		nRange = max ((int)max (m_pChar->bySearchCx, m_pChar->bySearchCy), 1) + MOVERAND_BATTLEMARGIN;
		if ((abs (nDX) <= nRange) && (abs (nDY) <= nRange)) {
			m_dwBattleTime = dwTime;
		} else if ((DWORD)(dwTime - m_dwBattleTime) >= MOVERAND_BATTLELOST) {
			bEnd = TRUE;
		}
	}

	if (bEnd) {
		/* 戦いをやめて、少し休んでから歩き出す */
		m_pChar->dwLastBattle	= 0;
		m_pChar->dwTargetTblID	= -1;
		m_dwBattleTarget		= (DWORD)-1;
		StartRest (dwTime, FALSE);
		return FALSE;
	}

	/* となりにいればそちらを向く（横や後ろに回られても殴り返す） */
	if (abs (nDX) + abs (nDY) == 1) {
		if (nDX) {
			byDirection = (nDX < 0) ? 2 : 3;
		} else {
			byDirection = (nDY < 0) ? 0 : 1;
		}
		if (m_pChar->byDirection != byDirection) {
			m_pChar->byDirection = byDirection;
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			return TRUE;
		}
	}
	return FALSE;
}

/* Copyright(C)URARA-works 2006 */
