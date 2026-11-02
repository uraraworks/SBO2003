/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBOMBBLOCK.cpp										 */
/* 内容：		キャラ処理(爆弾ゲームの壁)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcBOMBBLOCK.h"

/* ========================================================================= */
/* 関数名：	CCharProcBOMBBLOCK::CCharProcBOMBBLOCK							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcBOMBBLOCK::CCharProcBOMBBLOCK()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcBOMBBLOCK::Proc										 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcBOMBBLOCK::Proc(DWORD dwTime)
{
	PMAPINFO pMap = g_aMapInfo[m_pChar->dwMapID];
	PDWORD pData = NULL;
	BOOL bRet;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 250) {
		goto Exit;
	}
	
	if (m_pChar->dwLastDotMove == 0) {
		m_pChar->dwLastDotMove = dwTime;
		goto Exit;
	}

	if (m_pChar->byEfect == 1) {
		if (dwTime >= m_pChar->dwLastDotMove + 3000) {
			m_pChar->byEfect ++;
			m_pChar->dwLastDotMove = dwTime;
		}
	} else if (m_pChar->byEfect) {
		if (dwTime >= m_pChar->dwLastDotMove + 300) {
			m_pChar->byEfect ++;
			m_pChar->dwLastDotMove = dwTime;
			/* 消滅？ */
			if (m_pChar->byEfect >= 7) {
				PSUBGAMEINFO pInfo = g_aSubaGameInfo[m_pChar->abySubStatus[1]];
				PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
				/* アイテム使用？ */
				if (pBomb->byFlgUseItem) {
					if ((genrand () % 100) >= 65) {
						/* アイテムの追加 */
						PCHARINFO pCharTmp = new CHARINFO;
						ZeroMemory ((PBYTE)pCharTmp, sizeof (CHARINFO));
						pCharTmp->byNpcType = NPCTYPE_BOMBITEM;
						SetInitCharData (pCharTmp);
						pCharTmp->wGrpID = 0;
						pCharTmp->dwMapID = m_pChar->dwMapID;
						pCharTmp->dwX = m_pChar->dwX;
						pCharTmp->dwY = m_pChar->dwY;
						pCharTmp->byDirection = 1;
						pCharTmp->byGameState = GAMESTATE_NPC;

						BYTE byVal = (BYTE)genrand () % 100;
						if (byVal < 80) {
							/* 火力アップか個数追加 */
							pCharTmp->byEfect = (BYTE)genrand () % 2;
						} else {
							byVal = (BYTE)genrand () % 100;
							if (byVal < 15) {
								/* リモコン */
								pCharTmp->byEfect = 2;
							} else if (byVal < 30) {
								/* 壁抜け */
								pCharTmp->byEfect = 3;
							} else if (byVal < 45) {
								/* 爆弾抜け */
								pCharTmp->byEfect = 4;
							} else if (byVal < 60) {
								pCharTmp->byEfect = 0;
							} else if (byVal < 75) {
								/* 即爆発 */
								pCharTmp->byEfect = 5;
							} else if (byVal < 90) {
								/* ？ */
								pCharTmp->byEfect = 6;
							} else {
								/* 防火 */
								pCharTmp->byEfect = 0;
							}
						}

						pCharTmp = AddChar (pCharTmp);
						pCharTmp->pID = pCharTmp->dwTblID + 1;
						SendCmdCHARINFO3 (pCharTmp->dwTblID);
					}
				}
				DeleteChar (m_pChar->dwTblID);
			}
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
