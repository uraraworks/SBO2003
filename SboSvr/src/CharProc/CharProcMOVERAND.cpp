/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcMOVERAND.cpp										 */
/* 内容：		キャラ処理(ランダム移動)クラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcMOVERAND.h"

/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::CCharProcMOVERAND							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcMOVERAND::CCharProcMOVERAND()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcMOVERAND::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcMOVERAND::Proc(DWORD dwTime)
{
	int j, x, y;
	BOOL bRet, bResult, bFlg;
	BYTE byHitType;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 500) {
		goto Exit;
	}

	bResult = PartsMoveProc (m_pChar);
	if (bResult) {
		m_pChar->dwLastDotMove = dwTime;
		goto Exit;
	}
	pMap = g_aMapInfo[GetMapNo (m_pChar->dwMapID)];
	/* 誰もいない？ */
	if (pMap->dwCountTblNoPC == 0) {
		goto Exit;
	}

	if ((m_pChar->byNpcType == NPCTYPE_BATTLE1) && (m_pChar->dwLastBattle)) {
		/* 15秒以上移動してない？ */
		if (g_dwTime >= m_pChar->dwLastDotMove + 15000) {
			if ((m_pChar->dwX != m_pChar->dwStartX) || (m_pChar->dwY != m_pChar->dwStartY)) {
				/* ターゲット解除 */
				m_pChar->dwLastBattle = 0;
				m_pChar->dwTargetTblID = -1;
			}
		} else {
			goto Exit;
		}
	}

	if (genrand () % 10 > 7) {
		m_pChar->byDirection = (BYTE)genrand () % 4;
		m_pChar->byFlg |= CHARINFOFLG_DIRECTION;

	} else {
		int nTmp = genrand () % 100, nTmpCheck;

		if (m_pChar->byDirection >= 2) {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityX;
			if (m_pChar->byMoveProbabilityX == 0) {
				nTmpCheck = 70;
			}
		} else {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityY;
			if (m_pChar->byMoveProbabilityY == 0) {
				nTmpCheck = 70;
			}
		}
		if (nTmp >= nTmpCheck) {
			x = m_pChar->dwX + anPosAddTblX[m_pChar->byDirection];
			y = m_pChar->dwY + anPosAddTblY[m_pChar->byDirection];
			if (x >= 0 && x < pMap->nCX && y >= 0 && y < pMap->nCY) {
				byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;

				bFlg = TRUE;
				/* 障害物チェック */
				if (!((byHitType == PARTSHIT_NONE) ||
					  (byHitType == PARTSHIT_MOVEFRONT) ||
					  (byHitType == PARTSHIT_MOVEUP) ||
					  (byHitType == PARTSHIT_MOVEDOWN) ||
					  (byHitType == PARTSHIT_MOVERIGHT) ||
					  (byHitType == PARTSHIT_MOVELEFT) ||
					  (byHitType == PARTSHIT_HIDE1))) {
					bFlg = FALSE;

				/* 移動範囲が設定されている？ */
				} else if (m_pChar->byAreaX) {
					if ((abs (m_pChar->dwStartX - x) > m_pChar->byAreaX) ||
						(abs (m_pChar->dwStartY - y) > m_pChar->byAreaY)) {
						bFlg = FALSE;
					}

				/* イベントが配置されている？ */
				} else if (CheckEventBlock (m_pChar->dwMapID, (DWORD)x, (DWORD)y)) {
					bFlg = FALSE;
				}

				if (bFlg) {
					/* キャラチェック */
					pMap = g_aMapInfo[m_pChar->dwMapID];
					pData = pMap->paTblNoPC;
					for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
						if (pData[j] == -1) {
							break;
						}
						pCharTmp = g_aCharInfo[pData[j]];
						/* キャラと重なる？ */
						if ((pCharTmp->dwX == (DWORD)x) && (pCharTmp->dwY == (DWORD)y)) {
							bFlg = FALSE;
							break;
						}
					}
				}
				if ((m_pChar->byAreaX == 255) || (m_pChar->byAreaY == 255)) {
					bFlg = FALSE;
				}

				if (bFlg) {
					m_pChar->dwX = x;
					m_pChar->dwY = y;
					m_pChar->byFlg |= CHARINFOFLG_MOVE;
				}
			}
		}
	}
	m_pChar->dwLastDotMove = g_dwTime;

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
