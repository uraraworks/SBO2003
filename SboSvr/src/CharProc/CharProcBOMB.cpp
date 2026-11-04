/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBOMB.cpp											 */
/* 内容：		キャラ処理(爆弾)クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcBOMB.h"

/* ========================================================================= */
/* 関数名：	CCharProcBOMB::CCharProcBOMB									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcBOMB::CCharProcBOMB()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcBOMB::Proc												 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcBOMB::Proc(DWORD dwTime)
{
	PMAPINFO pMap = g_aMapInfo[m_pChar->dwMapID];
	PDWORD pData = NULL;
	BOOL bRet, bResult;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 150) {
		goto Exit;
	}

	bResult = PartsMoveProc (m_pChar);
	if (bResult) {
		m_pChar->dwLastDotMove = dwTime;
	}

	if (m_pChar->dwLastDotMove == 0) {
		m_pChar->dwLastDotMove = dwTime;
		goto Exit;
	}
	if (m_pChar->dwLastAnime == 0) {
		m_pChar->dwLastAnime = dwTime;
		goto Exit;
	}

	switch (m_pChar->byEfect) {
	case 0:	/* 爆発前 */
		if (dwTime >= m_pChar->dwLastAnime + 3000) {
			m_pChar->dwLastAnime = dwTime;

			PCHARINFO pCharParent = NULL;
			if (m_pChar->dwParentTblID != -1) {
				pCharParent = g_aCharInfo[m_pChar->dwParentTblID];
				PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pCharParent->abySubStatus;
				/* リモコン中？ */
				if (pSubStatus->byFlgRemocon) {
					break;
				}
			}

			m_pChar->byEfect ++;
			SendCmdCHARSTATUSHP (URARASOCK_SENDALL, m_pChar->dwTblID);
			SendCmdEFECTCHG (m_pChar->pID, m_pChar->dwTblID, m_pChar->byEfect);
			SendCmdPLAYSOUNDinside (m_pChar->dwTblID, SOUND_BOMBBOMBER);
		}
		break;

	default:	/* 爆発中 */
		int i, j;
		BYTE byUp = 0, byDown = 0, byLeft = 0, byRight = 0;
		/* 火力分ループ */
		for (j = 0; j < m_pChar->wHP; j ++) {
			/* 誘爆チェック */
			pData = pMap->paTblNoNPC;
			for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
				if (pData[i] == -1) {
					break;
				}
				PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
				if ((pCharTmp->byNpcType != NPCTYPE_BOMB) &&
					(pCharTmp->byNpcType != NPCTYPE_BOMBBLOCK) &&
					(pCharTmp->byNpcType != NPCTYPE_BOMBITEM)) {
					continue;
				}
				if (m_pChar == pCharTmp) {
					continue;
				}
				if ((pCharTmp->byNpcType == NPCTYPE_BOMB) && (pCharTmp->byEfect != 0)) {
					continue;
				}
				BOOL bHit = FALSE;
				if ((m_pChar->dwX == pCharTmp->dwX) || (m_pChar->dwY == pCharTmp->dwY)) {
					if (abs (m_pChar->dwX - pCharTmp->dwX) < m_pChar->wHP) {
						if (abs (m_pChar->dwY - pCharTmp->dwY) < m_pChar->wHP) {
							if (m_pChar->dwY > pCharTmp->dwY) {
								if ((int)m_pChar->dwY - j > 0) {
									if (byUp > 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[m_pChar->dwX + (m_pChar->dwY - j) * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										byUp = j;
										continue;
									}
									if (pCharTmp->dwY == (m_pChar->dwY - j)) {
										if (pCharTmp->byNpcType == NPCTYPE_BOMBITEM) {
											bHit = TRUE;
										} else if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
											/* 破壊されてない？ */
											if (pCharTmp->byEfect == 0) {
												pCharTmp->byEfect = 1;
												SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
												bHit = TRUE;
											}
											byUp = j;
											continue;
										}
										bHit = TRUE;
									}
								}
							}
							if (m_pChar->dwY < pCharTmp->dwY) {
								if ((int)m_pChar->dwY + j < pMap->nCY) {
									if (byDown > 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[m_pChar->dwX + (m_pChar->dwY + j) * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										byDown = j;
										continue;
									}
									if (pCharTmp->dwY == (m_pChar->dwY + j)) {
										if (pCharTmp->byNpcType == NPCTYPE_BOMBITEM) {
											bHit = TRUE;
										} else if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
											/* 破壊されてない？ */
											if (pCharTmp->byEfect == 0) {
												pCharTmp->byEfect = 1;
												SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
												bHit = TRUE;
											}
											byDown = j;
											continue;
										}
										bHit = TRUE;
									}
								}
							}
							if (m_pChar->dwX > pCharTmp->dwX) {
								if ((int)m_pChar->dwX - j > 0) {
									if (byLeft > 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[(m_pChar->dwX - j) + m_pChar->dwY * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										byLeft = j;
										continue;
									}
									if (pCharTmp->dwX == (m_pChar->dwX - j)) {
										if (pCharTmp->byNpcType == NPCTYPE_BOMBITEM) {
											bHit = TRUE;
										} else if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
											/* 破壊されてない？ */
											if (pCharTmp->byEfect == 0) {
												pCharTmp->byEfect = 1;
												SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
												bHit = TRUE;
											}
											byLeft = j;
											continue;
										}
										bHit = TRUE;
									}
								}
							}
							if (m_pChar->dwX < pCharTmp->dwX) {
								if ((int)m_pChar->dwX + j < pMap->nCX) {
									if (byRight > 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[(m_pChar->dwX + j) + m_pChar->dwY * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										byRight = j;
										continue;
									}
									if (pCharTmp->dwX == (m_pChar->dwX + j)) {
										if (pCharTmp->byNpcType == NPCTYPE_BOMBITEM) {
											bHit = TRUE;
										} else if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
											/* 破壊されてない？ */
											if (pCharTmp->byEfect == 0) {
												pCharTmp->byEfect = 1;
												SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
												bHit = TRUE;
											}
											byRight = j;
											continue;
										}
										bHit = TRUE;
									}
								}
							}

							if (bHit && (pCharTmp->byNpcType == NPCTYPE_BOMBITEM)) {
								DeleteChar (pCharTmp->dwTblID);
								break;

							} else if (bHit && (pCharTmp->byNpcType == NPCTYPE_BOMB)) {
								pCharTmp->byEfect = 1;
								pCharTmp->dwLastAnime = dwTime;
								SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTmp->dwTblID);
								SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
								SendCmdPLAYSOUNDinside (pCharTmp->dwTblID, SOUND_BOMBBOMBER);
							}
						}
					}
				}
			}
		}
		/* プレイヤー当たり判定 */
		pData = pMap->paTblNoPC;
		for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
			if (pData[i] == -1) {
				break;
			}
			PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
			if (pCharTmp->abySubStatus[0] != SUBGAMEID_BOMB) {
				continue;
			}
			PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pCharTmp->abySubStatus;
			if (pSubStatus->byFlgEntry == 0) {
				continue;
			}
			/* 耐火？ */
			if (pSubStatus->byFlgRegistFire == 1) {
				continue;
			}
			BOOL bHit = FALSE;
			if ((m_pChar->dwX == pCharTmp->dwX) || (m_pChar->dwY == pCharTmp->dwY)) {
				if (abs (m_pChar->dwX - pCharTmp->dwX) < m_pChar->wHP) {
					if (abs (m_pChar->dwY - pCharTmp->dwY) < m_pChar->wHP) {
						/* 横軸が同じ？ */
						if (m_pChar->dwX == pCharTmp->dwX) {
							if ((m_pChar->dwY > pCharTmp->dwY) && ((byUp == 0) || (m_pChar->dwY - pCharTmp->dwY) <= byUp)) {
								for (j = 0; j < (int)(m_pChar->dwY - pCharTmp->dwY); j ++) {
									if ((int)m_pChar->dwY - j < 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[m_pChar->dwX + (m_pChar->dwY - j) * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										break;
									}
								}
								if (j < (int)(m_pChar->dwY - pCharTmp->dwY)) {
									continue;
								}
								bHit = TRUE;
							}
							if ((m_pChar->dwY <= pCharTmp->dwY) && ((byDown == 0) || (pCharTmp->dwY - m_pChar->dwY) <= byDown)) {
								for (j = 0; j < (int)(pCharTmp->dwY - m_pChar->dwY); j ++) {
									if ((int)m_pChar->dwY + j >= pMap->nCY) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[m_pChar->dwX + (m_pChar->dwY + j) * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										break;
									}
								}
								if (j < (int)(pCharTmp->dwY - m_pChar->dwY)) {
									continue;
								}
								bHit = TRUE;
							}
						} else {
							if ((m_pChar->dwX > pCharTmp->dwX) && ((byLeft == 0) || (m_pChar->dwX - pCharTmp->dwX) <= byLeft)) {
								for (j = 0; j < (int)(m_pChar->dwX - pCharTmp->dwX); j ++) {
									if ((int)m_pChar->dwX - j < 0) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[(m_pChar->dwX - j) + m_pChar->dwY * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										break;
									}
								}
								if (j < (int)(m_pChar->dwX - pCharTmp->dwX)) {
									continue;
								}
								bHit = TRUE;
							}
							if ((m_pChar->dwX <= pCharTmp->dwX) && ((byRight == 0) || (pCharTmp->dwX - m_pChar->dwX) <= byRight)) {
								for (j = 0; j < (int)(pCharTmp->dwX - m_pChar->dwX); j ++) {
									if ((int)m_pChar->dwX + j >= pMap->nCX) {
										continue;
									}
									BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[(m_pChar->dwX + j) + m_pChar->dwY * pMap->nCX]])->byHitType;
									if (((byHitType != PARTSHIT_NONE) &&
										 (byHitType != PARTSHIT_MOVEFRONT) &&
										 (byHitType != PARTSHIT_MOVEUP) &&
										 (byHitType != PARTSHIT_MOVEDOWN) &&
										 (byHitType != PARTSHIT_MOVELEFT) &&
										 (byHitType != PARTSHIT_MOVERIGHT) &&
										 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
										 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
										break;
									}
								}
								if (j < (int)(pCharTmp->dwX - m_pChar->dwX)) {
									continue;
								}
								bHit = TRUE;
							}
						}
						if (bHit == FALSE) {
							continue;
						}
						BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[pCharTmp->dwX + pCharTmp->dwY * pMap->nCX]])->byHitType;
						if (((byHitType != PARTSHIT_NONE) &&
							 (byHitType != PARTSHIT_MOVEFRONT) &&
							 (byHitType != PARTSHIT_MOVEUP) &&
							 (byHitType != PARTSHIT_MOVEDOWN) &&
							 (byHitType != PARTSHIT_MOVELEFT) &&
							 (byHitType != PARTSHIT_MOVERIGHT) &&
							 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
							 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
							continue;
						}
						PSUBGAMEINFO pInfo = g_aSubaGameInfo[pCharTmp->abySubStatus[1]];
						PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
						/* やられた。 */
						CharTargetCancelPC (m_pChar);
						SendCmdSYSMSG (pCharTmp->pID, "爆風に当たってアウト！", SPEECHCMD_BATTLE1);
						if (pSubStatus->byFlgGameMaster) {
							pSubStatus->byFlgEntry = 0;
							pSubStatus->byFlgBlockThrough = 0;
							pSubStatus->byFlgBombThrough = 0;
							pSubStatus->byFlgRegistFire = 0;
							pSubStatus->byFlgRemocon = 0;
						} else {
							SendCmdSubBOMBSTOP (pCharTmp->pID, pSubStatus->byID, pBomb);
							ZeroMemory (pCharTmp->abySubStatus, sizeof (pCharTmp->abySubStatus));
						}
						SendCmdSUBSTATUS (pCharTmp->dwTblID);
						SendCmdFADEINOUT (pCharTmp->pID, 0);
						pCharTmp->byDirection = 1;
						pCharTmp->byDirectionBack2 = pCharTmp->byDirection;
						pCharTmp->byDirectionBack = pCharTmp->byDirection;
						pCharTmp->dwX = pBomb->dwX;
						pCharTmp->dwY = pBomb->dwY;
						pCharTmp->dwXBack = m_pChar->dwX;
						pCharTmp->dwYBack = m_pChar->dwY;
						SendCmdCHARINFO (pCharTmp->pID, -1);
						SendCmdCHARPOS (pCharTmp->pID, pCharTmp->dwTblID, 1, 2);
						SendCmdFADEINOUT (pCharTmp->pID, 1);
					}
				}
			}
		}
		if (dwTime >= m_pChar->dwLastAnime + 800) {
			if (m_pChar->dwParentTblID != -1) {
				PCHARINFO pCharParent = g_aCharInfo[m_pChar->dwParentTblID];
				PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pCharParent->abySubStatus;
				pSubStatus->byCount --;
			}
			/* 消滅 */
			DeleteChar (m_pChar->dwTblID);
			break;
		}
		break;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
