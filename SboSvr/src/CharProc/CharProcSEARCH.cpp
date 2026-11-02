/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH.cpp											 */
/* 内容：		キャラ処理(検索してついていく)クラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH.h"

/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::CCharProcSEARCH								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcSEARCH::CCharProcSEARCH()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcSEARCH::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcSEARCH::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;
	PDWORD pData;
	BOOL bFlg;
	DWORD dwX, dwY;
	int nAreaX, nAreaY, nCountTmpX, nCountTmpY, nX, nY;
	PCHARINFO pCharTmp = NULL;
	PMAPINFO pMap;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 500) {
		goto Exit;
	}

	bResult = PartsMoveProc (m_pChar);
	if (bResult) {
		m_pChar->dwLastDotMove = timeGetTime ();
		goto Exit;
	}
	pMap = g_aMapInfo[m_pChar->dwMapID];
	/* 誰もいない？ */
	if (pMap->dwCountTblNoPC == 0) {
		goto Exit;
	}

	/* ターゲットがいる？ */
	if (m_pChar->dwTargetTblID != -1) {
		pCharTmp = g_aCharInfo[m_pChar->dwTargetTblID];
		if (pCharTmp->byGameState == GAMESTATE_LOGOFF) {
			m_pChar->dwTargetTblID = -1;
			goto Exit;
		}
		dwX = m_pChar->dwX;
		dwY = m_pChar->dwY;
		dwX += anPosAddTblX[m_pChar->byDirection];
		dwY += anPosAddTblY[m_pChar->byDirection];
		/* 目の前にターゲットキャラがいる？ */
		if ((CheckCharX (m_pChar, dwX, pCharTmp->dwX)) && (dwY == pCharTmp->dwY)) {
			m_pChar->dwLastDotMove = g_dwTime;
			goto Exit;
		}
		bFlg = FALSE;
		if (m_pChar->dwY == pCharTmp->dwY - 1) {
			if (m_pChar->dwX == pCharTmp->dwX) {
				m_pChar->byDirection = 1;
				bFlg = TRUE;
			}
		} else if (m_pChar->dwY == pCharTmp->dwY + 1) {
			if (m_pChar->dwX == pCharTmp->dwX) {
				m_pChar->byDirection = 0;
				bFlg = TRUE;
			}
		} else if (CheckCharX (m_pChar, m_pChar->dwX, pCharTmp->dwX + 1)) {
			if (m_pChar->dwY == pCharTmp->dwY) {
				m_pChar->byDirection = 2;
				bFlg = TRUE;
			}
		} else if (CheckCharX (m_pChar, m_pChar->dwX, pCharTmp->dwX - 1)) {
			if (m_pChar->dwY == pCharTmp->dwY) {
				m_pChar->byDirection = 3;
				bFlg = TRUE;
			}
		}
		if (bFlg) {
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
			m_pChar->dwLastDotMove = g_dwTime;
			goto Exit;
		}
	}

	nX = (int)m_pChar->dwX;
	nY = (int)m_pChar->dwY;
	nAreaX = m_pChar->bySearchCx, nAreaY = m_pChar->bySearchCy;

	nCountTmpX = nAreaX * 2 + 1;
	nCountTmpY = nAreaY * 2 + 1;
	nX += ((nAreaX * -1) + (m_pChar->wSearchTmp % nCountTmpX));
	nY += ((nAreaY * -1) + (m_pChar->wSearchTmp / nCountTmpX));

	nX = max (nX, 0);
	nX = min (nX, pMap->nCX - 1);
	nY = max (nY, 0);
	nY = min (nY, pMap->nCY - 1);

	/* キャラチェック */
	pData = pMap->paTblNoPC;
	bFlg = FALSE;
	pCharTmp = NULL;

	/* ターゲットキャラがいない？ */
	if (m_pChar->dwTargetTblID == -1) {
		for (int j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
			if (pData[j] == -1) {
				break;
			}
			pCharTmp = g_aCharInfo[pData[j]];
			/* キャラを発見？ */
			if ((pCharTmp->dwX == (DWORD)nX) && (pCharTmp->dwY == (DWORD)nY) && (pCharTmp->wHP > 0)) {
				/* そのキャラをターゲットする？ */
				if (genrand () % 100 >= 30) {
					if (m_pChar->byNpcType == NPCTYPE_BATTLE2) {
						if (m_pChar->dwLastBattle == 0) {
							m_pChar->dwTargetTblID = pCharTmp->dwTblID;
							m_pChar->byDirection = abyDirectionTbl[pCharTmp->byDirection];
							m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
							/* 初回は即攻撃させる */
							m_pChar->dwLastBattle = 1;
						}
					} else {
						m_pChar->dwTargetTblID = pCharTmp->dwTblID;
					}
					goto Exit;
				}
			}
		}
	} else {
		pCharTmp = g_aCharInfo[m_pChar->dwTargetTblID];
		if ((pCharTmp->dwMapID == m_pChar->dwMapID) &&
			(abs (pCharTmp->dwX - m_pChar->dwX) <= nAreaX) &&
			(abs (pCharTmp->dwY - m_pChar->dwY) <= nAreaY)) {
			bFlg = TRUE;
		}
		int nTmp = genrand () % 100, nTmpCheck;

		if (m_pChar->byDirection >= 2) {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityX;
			if (m_pChar->byMoveProbabilityX == 0) {
				nTmpCheck = 80;
			}
		} else {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityY;
			if (m_pChar->byMoveProbabilityY == 0) {
				nTmpCheck = 80;
			}
		}

		if (nTmp < nTmpCheck) {
			if (bFlg) {
				m_pChar->dwLastDotMove = timeGetTime ();
			}
			goto Exit;
		}
	}

	nX = (int)m_pChar->dwX;
	nY = (int)m_pChar->dwY;
	/* キャラを発見？ */
	if (bFlg) {
		BOOL bProc = FALSE;
		for (int j=0;j<2;j++){
		/* 北にいる？ */
		if (nY > (int)pCharTmp->dwY) {
			bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX, nY - 1);
			if (bProc) {
				nY --;
				continue;
			}
		}
		/* 南にいる？ */
		if (nY < (int)pCharTmp->dwY) {
			bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX, nY + 1);
			if (bProc) {
				nY ++;
				continue;
			}
		}
		/* 西にいる？ */
		if (nX > (int)pCharTmp->dwX) {
			bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX - 1, nY);
			if (bProc) {
				nX --;
				continue;
			}
		}
		/* 東にいる？ */
		if (nX < (int)pCharTmp->dwX) {
			bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX + 1, nY);
			if (bProc) {
				nX ++;
				continue;
			}
		}
		if (bProc) {
			break;
		}
		}
		if (bProc) {
			nX = (int)m_pChar->dwX;
			nY = (int)m_pChar->dwY;
			bProc = FALSE;
			/* 北にいる？ */
			if (m_pChar->dwY > pCharTmp->dwY) {
				bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX, nY - 1);
				if (bProc) {
					nY --;
					m_pChar->byDirection = 0;
					m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
				}
			}
			/* 南にいる？ */
			if (bProc == FALSE && m_pChar->dwY < pCharTmp->dwY) {
				bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX, nY + 1);
				if (bProc) {
					nY ++;
					m_pChar->byDirection = 1;
					m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
				}
			}
			/* 西にいる？ */
			if (bProc == FALSE && m_pChar->dwX > pCharTmp->dwX) {
				bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX - 1, nY);
				if (bProc) {
					nX --;
					m_pChar->byDirection = 2;
					m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
				}
			}
			/* 東にいる？ */
			if (bProc == FALSE && m_pChar->dwX < pCharTmp->dwX) {
				bProc = NpcProcSEARCH_Check (m_pChar, pCharTmp, nX + 1, nY);
				if (bProc) {
					nX ++;
					m_pChar->byDirection = 3;
					m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
				}
			}
		} else {
			int nXTmp, nYTmp, nDirection;

			nX = (int)m_pChar->dwX;
			nY = (int)m_pChar->dwY;

			nDirection = genrand () % 4;
			nXTmp = nX + anPosAddTblX[nDirection];
			nYTmp = nY + anPosAddTblY[nDirection];
			if (NpcProcSEARCH_Check (m_pChar, pCharTmp, nXTmp, nYTmp)) {
				if (!(abs (nX - (int)m_pChar->dwX) > abs (nXTmp - (int)m_pChar->dwX) ||
				abs (nY - (int)m_pChar->dwY) > abs (nYTmp - (int)m_pChar->dwY))) {
					m_pChar->byDirection = (BYTE)nDirection;
					nX = nXTmp;
					nY = nYTmp;
					bProc = TRUE;
				}
			}
		}

		if (bProc) {
			/* 移動範囲が設定されている？ */
			if (m_pChar->byAreaX) {
				if ((abs (m_pChar->dwStartX - nX) > m_pChar->byAreaX) ||
					(abs (m_pChar->dwStartY - nY) > m_pChar->byAreaY)) {
					/* ターゲットがいない？ */
					if (m_pChar->dwTargetTblID == -1) {
						goto Exit;
					}
				}
			}
			/* 重なる？ */
			if ((pCharTmp->dwX == (DWORD)nX) && (pCharTmp->dwY == (DWORD)nY)) {
				m_pChar->wSearchTmp = 0;
				goto Exit;
			}

			DWORD dwTblID;

			dwTblID = -1;
			if (m_pChar->byNpcType == NPCTYPE_BATTLE2) {
				dwTblID = CheckCharBlockPC (m_pChar->dwMapID, nX, nY);
			}

			/* 誰もいない？ */
			if (dwTblID == -1) {
				if (!((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255))) {
					m_pChar->dwX = (DWORD)nX;
					m_pChar->dwY = (DWORD)nY;
					m_pChar->byFlg |= CHARINFOFLG_MOVE;
				}
				m_pChar->dwLastDotMove = g_dwTime;
			}

		} else {
			/* タゲ相手と重なってる？ */
			if ((m_pChar->dwX == pCharTmp->dwX) && (m_pChar->dwY == pCharTmp->dwY)) {
				/* タゲ解除。 */
				m_pChar->dwTargetTblID = -1;
				m_pChar->dwLastBattle = 0;
			}
		}
		goto Exit;
	}

	m_pChar->wSearchTmp ++;
	if (m_pChar->wSearchTmp >= nCountTmpX * nCountTmpY) {
		m_pChar->wSearchTmp = 0;
		/* ターゲットがいる？ */
		if (m_pChar->dwTargetTblID != -1) {
			/* みつからなかったのであきらめる */
			m_pChar->dwTargetTblID = -1;
			m_pChar->dwLastBattle = 0;
		}
	}
	if (m_pChar->byNpcType == NPCTYPE_BATTLE2) {
		/* 10秒以上移動してない？ */
		if (m_pChar->dwLastDotMove && g_dwTime >= m_pChar->dwLastDotMove + 10000) {
			m_pChar->dwLastDotMove = g_dwTime;
			m_pChar->dwLastBattle = 0;
			m_pChar->dwTargetTblID = -1;
			if ((m_pChar->dwX != m_pChar->dwStartX) || (m_pChar->dwY != m_pChar->dwStartY)) {
				/* あきらめて開始地点に戻る */
				m_pChar->dwX = m_pChar->dwStartX;
				m_pChar->dwY = m_pChar->dwStartY;
				SendCmdCHARPOS (m_pChar->pID, m_pChar->dwTblID, 1, 2);
			}
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
