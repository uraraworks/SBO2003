/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrBattleProc.cpp										 */
/* 概要：		戦闘処理													 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/13													 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "../../Common/Info/InfoJob.h"
#include "ProcFunc/ProcFunc.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrMagicProc.h"
#include <map>

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
		void	BattleProcCheck		(PCHARINFO);					/* 戦闘行動チェック */
		void	BattleProcCheckPC	(PCHARINFO);					/* 戦闘行動チェック(PC) */
		BOOL	DamageAfterProcPC	(PCHARINFO, PCHARINFO);			/* ダメージ受けた後の処理(PC) */
		BOOL	DamageAfterProcNPC	(PCHARINFO, PCHARINFO);			/* ダメージ受けた後の処理(NPC) */
static	BOOL	BattleProc			(PCHARINFO, PCHARINFO);			/* 戦闘処理 */
		BOOL	BattleProcWeapon	(PCHARINFO);					/* 武器処理 */
		BOOL	BattleProcAtack		(PCHARINFO, PCHARINFO);			/* 戦闘処理(直接攻撃) */
static	void	BattleProcAtackArea	(PCHARINFO, DWORD);				/* なぎ払い・貫通で周りの敵にも当てる */
static	BOOL	BattleProcTell		(PCHARINFO, PMAPINFO, DWORD, BOOL);	/* 敵の攻撃の予兆「！」 */
		void	ResurrectionBack	(PCHARINFO);					/* 復活位置で蘇生 */
static	void	BattleProcMagic		(PCHARINFO, PCHARINFO, BYTE);	/* 戦闘処理(魔法) */
static	int		GetWeponMin			(PCHARINFO);					/* 武器の減少値を取得 */
static	int		DamageProc			(PCHARINFO, PCHARINFO, BOOL);	/* ダメージ量計算 */
static	DWORD	GetAcquisitionGold	(PCHARINFO, PCHARINFO);			/* 獲得金額を計算 */
static	DWORD	GetAcquisitionExp	(PCHARINFO, PCHARINFO);			/* 獲得経験値を計算 */
static	void	AddGold				(PCHARINFO, DWORD, BOOL);		/* 所持金を追加 */
static	void	AddExp				(PCHARINFO, DWORD, BOOL);		/* 経験値を追加 */
		void	AddStatus			(PCHARINFO pChar, BYTE byType, int nData);	/* パラメータの追加 */

extern long genrand();												/* 乱数取得 */


/* ========================================================================= */
/* 関数名：BattleProcCheck													 */
/* 内容：戦闘行動チェック													 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
void BattleProcCheck(PCHARINFO pChar)
{
	int j, nCount, nTmp;
	PDWORD pData;
	DWORD dwX, dwY;
	PMAPINFO pMap;
	BOOL bResult;

	pMap = g_aMapInfo[pChar->dwMapID];

	/* 戦闘許可マップではない？ */
	if ((pMap->byFlg & MAPFLG_BATTLE) == 0) {
		return;
	}

	/* ターゲットがいない？ */
	if (pChar->dwLastBattle == 0) {
		return;
	}

	/* 死んでいる？(倒れたPCは攻撃しない。矢などはwHPを射程に使うのでPCだけ見る) */
	if ((pChar->byNpcType == NPCTYPE_INIT) &&
		((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0))) {
		pChar->dwLastBattle	 = 0;
		pChar->dwTargetTblID = -1;
		return;
	}

	/* PCの行動？ */
	if (pChar->byNpcType == NPCTYPE_INIT) {
		BattleProcCheckPC (pChar);
		return;
	}


	/***************************
		行動タイミングの判定
	***************************/

	/* 攻撃タイミングになっているかを計算。最大4秒で255を100%とする。 */
	nTmp = pChar->byDEX;
	nTmp = nTmp * 100 / 255;
	nTmp = max (nTmp, 1);
	nTmp = 1000 + (4000 - ((4000 * nTmp) / 100));

	if (pChar->dwLastBattle == 1) {
		pChar->dwLastBattle = timeGetTime ();
		return;
	}

	/* 規定時間以上経っている？ */
	if (abs (timeGetTime () - pChar->dwLastBattle) < nTmp) {
		/* 攻撃の少し前に、目の前のPCへ予兆「！」を出す */
		BattleProcTell (pChar, pMap, (DWORD)nTmp, FALSE);
		return;
	}
	/* 攻撃できる時間になってからPCが目の前に来た？ 予兆を出してから少し待って殴る */
	if (BattleProcTell (pChar, pMap, (DWORD)nTmp, TRUE)) {
		return;
	}

	bResult = BattleProcWeapon (pChar);
	if (bResult) {
		pChar->dwLastBattle = timeGetTime ();
		return;
	}

	/* PCの行動？ */
	if (pChar->byNpcType == NPCTYPE_INIT) {
		pData = pMap->paTblNoNPC;
		nCount = (int)pMap->dwCountTblNoNPC;

	} else {
		pData = pMap->paTblNoPC;
		nCount = (int)pMap->dwCountTblNoPC;
	}

	/* 一歩前の座標 */
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	/* マップ内のキャラを処理 */
	for (j = 0; j < nCount; j ++) {
		if (pData[j] == -1) {
			break;
		}

		BOOL bResult;
		PCHARINFO pCharTmp;

		bResult = TRUE;
		pCharTmp = g_aCharInfo[pData[j]];

		/* 目の前にはいない？ */
		if (((CheckCharX (pChar, dwX, pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, dwX) == FALSE))) ||
			(dwY != pCharTmp->dwY)) {
			continue;
		}

		/* PCの行動？ */
		if (pChar->byNpcType == NPCTYPE_INIT) {
			/* ターゲット相手ではない？ */
			if (pChar->dwTargetTblID != pCharTmp->dwTblID) {
				continue;
			}
		}

		/* 戦闘処理 */
		bResult = BattleProc (pChar, pCharTmp);
		if (bResult == FALSE) {
			pChar->dwLastBattle = timeGetTime ();
		}


		/***********************
			 戦闘後の後処理
		***********************/

		/* PCの行動？ */
		if (pChar->byNpcType == NPCTYPE_INIT) {
			/* ターゲットは毎回解除 */
			pChar->dwLastBattle	 = 0;
			pChar->dwTargetTblID = -1;
			break;

		} else {
			/* 他のキャラへの行動はしない？ */
			if (bResult == FALSE) {
				break;
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	BattleProcCheckPC												 */
/* 内容：	戦闘行動チェック(PC)											 */
/* 日付：	2006/03/06														 */
/* ========================================================================= */

void BattleProcCheckPC(PCHARINFO pChar)
{
	int j, nCount;
	PDWORD pData;
	DWORD dwX, dwY;
	PMAPINFO pMap;
	BOOL bResult;

	if (pChar->dwLastBattle != 1) {
		return;
	}

	pMap = g_aMapInfo[pChar->dwMapID];

	/* 武器処理 */
	bResult = BattleProcWeapon (pChar);
	if (bResult) {
		goto Exit;
	}

	pData = pMap->paTblNoNPC;
	nCount = (int)pMap->dwCountTblNoNPC;

	/* 一歩前の座標 */
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	/* マップ内のキャラを処理 */
	for (j = 0; j < nCount; j ++) {
		if (pData[j] == -1) {
			break;
		}

		BOOL bResult;
		PCHARINFO pCharTmp;

		bResult = TRUE;
		pCharTmp = g_aCharInfo[pData[j]];

		/* 目の前にはいない？ */
		if (((CheckCharX (pChar, dwX, pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, dwX) == FALSE))) ||
			(dwY != pCharTmp->dwY)) {
			continue;
		}

		/* ターゲット相手ではない？ */
		if (pChar->dwTargetTblID != pCharTmp->dwTblID) {
			continue;
		}
		pChar->dwLastBattle = timeGetTime ();

		/* 戦闘処理 */
		bResult = BattleProc (pChar, pCharTmp);
		/* なぎ払い・貫通の武器なら、周りの敵にも当てる */
		BattleProcAtackArea (pChar, pCharTmp->dwTblID);
		break;
	}

Exit:
	/* ターゲットは毎回解除 */
	pChar->dwLastBattle	 = 0;
	pChar->dwTargetTblID = -1;
}


/* ========================================================================= */
/* 関数名：DamageAfterProcPC												 */
/* 内容：ダメージ受けた後の処理(PC)											 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
BOOL DamageAfterProcPC(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int nTmp;

	nTmp = pCharTarget->wHP;
	if (nTmp > 0) {
		return FALSE;
	}

	TailOff (pCharTarget);
	TailOff (pCharTarget);

	pCharTarget->byCharState = CHARSTATE_DEAD;
	SendCmdCHARSTATUSHP (pCharTarget->pID, pCharTarget->dwTblID);

	/* ターゲット解除 */
	pChar->dwLastBattle		= 0;
	pChar->dwTargetTblID	= -1;

	pCharTarget->dwLastSake			= 0;
	pCharTarget->dwLastSakeHikku	= 0;
	pCharTarget->byDrunkPoint		= 0;
	pCharTarget->byDrunkLV			= CHARSTATEDRUNK_NONE;
	pCharTarget->dwTargetTblID = -1;
	if (pCharTarget->dwTargetTblID2 != -1) {
		PCHARINFO pCharTmp = g_aCharInfo[pCharTarget->dwTargetTblID2];
		pCharTmp->dwTargetTblID3 = -1;
		SendCmdTARGETCHARPC (pCharTmp->pID, pCharTmp->dwTblID);
		pCharTarget->dwTargetTblID2 = -1;
	}
	if (pCharTarget->dwTargetTblID3 != -1) {
		PCHARINFO pCharTmp = g_aCharInfo[pCharTarget->dwTargetTblID3];
		pCharTmp->dwTargetTblID2 = -1;
		pCharTarget->dwTargetTblID3 = -1;
		SendCmdTARGETCHARPC (pCharTarget->pID, pCharTarget->dwTblID);
	}

	SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_DEAD, pCharTarget->pID, 0);
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTarget->dwTblID);
	/* ターゲット解除 */
	pCharTarget->dwLastBattle = 0;
	pCharTarget->dwTargetTblID = -1;
	SendCmdTARGETCHAR (pCharTarget->dwTblID, -1);

	return TRUE;
}

/* ========================================================================= */
/* 関数名：DamageAfterProcNPC												 */
/* 内容：ダメージ受けた後の処理(NPC)										 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
BOOL DamageAfterProcNPC(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int nTmp;
	DWORD dwGoldTmp, dwTmp;
	DWORD dwSend;

	dwSend = pChar->pID;
	if (pChar->byNpcType == NPCTYPE_ATACK) {
		pChar = g_aCharInfo[pChar->dwParentTblID];
	} 

	nTmp = pCharTarget->wHP;
	if (nTmp > 0) {
		switch (pCharTarget->byNpcType) {
		case NPCTYPE_BATTLE1:
		case NPCTYPE_BATTLE2:
			break;
		default:
			return FALSE;
		}

		if (pCharTarget->dwLastBattle == 0) {
			/* 最初に攻撃してきたキャラをターゲットにする */
			pCharTarget->dwTargetTblID = pChar->dwTblID;
			pCharTarget->byDirection = abyDirectionTbl[pChar->byDirection];
			pCharTarget->byFlg |= CHARINFOFLG_DIRECTION;
			pCharTarget->dwLastBattle = 1;

		} else {
			/* ターゲット以外からの攻撃？ */
			if (pChar->dwTblID != pCharTarget->dwTargetTblID) {
				/* 攻撃タイミングになっているかを計算。最大4秒で255を100%とする。 */
				nTmp = pChar->byDEX;
				nTmp = nTmp * 100 / 255;
				nTmp = max (nTmp, 1);
				nTmp = 1000 + (4000 - ((4000 * nTmp) / 100));

				/* 2ターン以上行動していない？ */
				if (abs (timeGetTime () - pCharTarget->dwLastBattle) / nTmp >= 2) {
					/* 目の前にはいないと判断してターゲット解除 */
					pCharTarget->dwLastBattle = 0;
					pCharTarget->dwTargetTblID = -1;
				}
			}
		}
		return FALSE;
	}

	SendCmdSYSMSG2 (pChar->pID, SYSMSG_BATTLEEND1, pCharTarget->pID, 0);

	/* 経験値取得？ */
	dwTmp = GetAcquisitionExp (pChar, pCharTarget);
	if (dwTmp > 0) {
		/* パーティに所属している？ */
		if (pChar->dwPartyID) {
			PPARTYINFO pPartyInfo = g_aPartyInfo[pChar->dwPartyID];

			/* 全員で分配？ */
			if (pPartyInfo->byGold == PARTYGOLD_ALL) {
				int i;

				for (i = 0; i < MAX_PARTYMEMBER; i ++) {
					PCHARINFO pCharTmp;

					if (pPartyInfo->adwTblID[i] == 0) {
						break;
					}
					pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
					if (pCharTmp->byCharState == CHARSTATE_DEAD) {
						continue;
					}

					AddExp (pCharTmp, dwTmp, TRUE);
				}

			} else {
				AddExp (pChar, dwTmp, TRUE);
			}

		} else {
			AddExp (pChar, dwTmp, TRUE);
		}
	}

	dwGoldTmp = GetAcquisitionGold (pChar, pCharTarget);
	/* お金持ってる？ */
	if (dwGoldTmp > 0) {
		/* パーティに所属している？ */
		if (pChar->dwPartyID) {
			PPARTYINFO pPartyInfo = g_aPartyInfo[pChar->dwPartyID];

			/* 全員で分配？ */
			if (pPartyInfo->byGold == PARTYGOLD_ALL) {
				int i;
				char szTmp[128];

				wsprintf (szTmp, "%sから%ldゴールド分配されました。", pChar->szName, dwGoldTmp);
				for (i = 0; i < MAX_PARTYMEMBER; i ++) {
					PCHARINFO pCharTmp;

					if (pPartyInfo->adwTblID[i] == 0) {
						break;
					}
					pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
					if (pCharTmp->byCharState == CHARSTATE_DEAD) {
						continue;
					}

					AddGold (pCharTmp, dwGoldTmp, TRUE);
					if (pCharTmp != pChar) {
						SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
					}
				}
				SendCmdSYSMSG2 (pChar->pID, SYSMSG_BATTLEEND3, pChar->pID, dwGoldTmp);

			} else {
				AddGold (pChar, dwGoldTmp, TRUE);
				SendCmdSYSMSG2 (pChar->pID, SYSMSG_BATTLEEND3, pChar->pID, dwGoldTmp);
			}

		} else {
			AddGold (pChar, dwGoldTmp, TRUE);
			SendCmdSYSMSG2 (pChar->pID, SYSMSG_BATTLEEND3, pChar->pID, dwGoldTmp);
		}
	}

	BYTE byGetItem = 0;
	int nGetItemNo = 0;

	/* アイテム落とす？ */
	if (pCharTarget->byDropProbability) {
		int nCount = GetItemCount (pCharTarget->dwTblID);
		if (nCount && ((genrand () % 100) + pCharTarget->byDropProbability >= 100)) {
			nGetItemNo = pCharTarget->waItem[genrand () % nCount];

			/* もう持てない？ */
			if (GetItemCount (pChar->dwTblID) >= MAX_MYITEM) {
				byGetItem = 1;

			} else {
				byGetItem = 2;
				AddCharItem (pChar->dwTblID, nGetItemNo);
				SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
			}
		}
	}

	/* ターゲットにしていた他のプレイヤーへ通知 */
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	int i;

	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		if (pCharTmp->dwTargetTblID == pCharTarget->dwTblID) {
			if (pCharTmp->dwTblID != pChar->dwTblID) {
				/* ターゲット解除 */
				pCharTmp->dwLastBattle = 0;
				pCharTmp->dwTargetTblID = -1;
				SendCmdTARGETCHAR (pCharTmp->dwTblID, -1);
				/* 倒したプレイヤー以外にメッセージ通知 */
				SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_BATTLEEND2, pCharTarget->pID, pChar->pID);
				if (dwGoldTmp > 0) {
					SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_BATTLEEND3, pChar->pID, dwGoldTmp);
				}
			}

			/* アイテム落とした？ */
			if (byGetItem) {
				SendCmdPLAYSOUND (pCharTmp->pID, SOUND_PI52);
				SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM1, pCharTarget->pID, nGetItemNo);
				if (byGetItem == 1) {
					SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM2, 0, 0);
				} else {
					SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM3, pChar->pID, nGetItemNo);
				}
			}
		}
	}

	/* ターゲットしているキャラ？ */
	if (pChar->dwTargetTblID == pCharTarget->dwTblID) {
		/* ターゲット解除 */
		pChar->dwLastBattle = 0;
		pChar->dwTargetTblID = -1;
		SendCmdTARGETCHAR (pChar->dwTblID, -1);
	}

	/* 削除 */
	DeleteChar (pCharTarget->dwTblID);

	return TRUE;
}

/* ========================================================================= */
/* 関数名：BattleProc														 */
/* 内容：戦闘処理															 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
static BOOL BattleProc(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	BOOL bRet, bResult;
	BYTE byNo;
	int nType = 0;

	bRet = TRUE;

	if (pChar->byNpcType) {
		byNo = (BYTE)(genrand () % MAX_USEMAGIC);
		if (pChar->abyMagic[byNo]) {
			nType = 1;
		}
	}

	switch (nType) {
	case 0:		/* 直接攻撃 */
		bResult = BattleProcAtack (pChar, pCharTarget);
		if (bResult) {
			bRet = FALSE;
		}
		break;
	case 1:		/* 魔法 */
		BattleProcMagic (pChar, pCharTarget, byNo);
		bRet = FALSE;
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：BattleProcWeapon													 */
/* 内容：武器処理															 */
/* 日付：2004/10/14															 */
/* ========================================================================= */
BOOL BattleProcWeapon(PCHARINFO pChar)
{
	BOOL bRet;
	PITEMINFO pItem;
	PITEMINFO_WEAPON pWeapon;

	bRet = FALSE;

	/* 武器を装備していない？ */
	if (pChar->waEqu[0] == 0xFFFF) {
		goto Exit;
	}

	pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
	if (pItem->pData == NULL) {
		goto Exit;
	}

	pWeapon = (PITEMINFO_WEAPON)pItem->pData;
	switch (pWeapon->byAtackType) {
	case ATACKTYPE_ARROW:		/* 矢 */
		{
			PCHARINFO pCharNew = new CHARINFO;
			ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));
			pCharNew->byNpcType = NPCTYPE_ATACK;
			SetInitCharData (pCharNew);

			pCharNew->byGameState = GAMESTATE_NPC;
			pCharNew->byAtackRate = pChar->byAtackRate;
			pCharNew->dwXBack = pCharNew->dwX = pChar->dwX;
			pCharNew->dwYBack = pCharNew->dwY = pChar->dwY;
			pCharNew->dwMapID = pCharNew->dwMapID = pChar->dwMapID;
			pCharNew->dwTimeLogin = timeGetTime ();
			pCharNew->dwParentTblID = pChar->dwTblID;
			pCharNew->wAT = pChar->wAT;
			pCharNew->wDF = pChar->wDF;
			pCharNew->wMaxHP = 0;
			pCharNew->waEqu[0] = 0;
			pCharNew->waItem[0] = pChar->waItem[pChar->waEqu[0]];
			pCharNew->byDirection = pChar->byDirection;
			pCharNew->byMoveProbabilityX = 100;
			pCharNew->byMoveProbabilityY = 100;
			pCharNew->wHP = pWeapon->byData1;
			pCharNew->wMaxHP = 1;

			switch (pChar->byDirection) {
			case 0:
				pCharNew->byEfect = pWeapon->byData2;
				break;
			case 1:
				pCharNew->byEfect = pWeapon->byData3;
				break;
			case 2:
				pCharNew->byEfect = pWeapon->byData4;
				break;
			case 3:
				pCharNew->byEfect = pWeapon->byData5;
				break;
			}

			pCharNew = AddChar (pCharNew);

			/* 属性が火？ */
//			if (pInfo->byAttr == ATTRTYPE_FIRE) {
//				pCharNew->byLightLevel = 5;
//			}

			pCharNew->pID = pCharNew->dwTblID + 1;
//			RenewATDF (pCharNew->dwTblID);
			SendCmdCHARINFO3 (pCharNew->dwTblID);
		}
		break;

	default:
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：	BattleProcTell													 */
/* 内容：	敵の攻撃の予兆：攻撃する ENEMYTELL_TIME 前に、頭の上へ「！」を出す	 */
/*			目の前にPCがいる時だけ。1回の攻撃につき1回だけ出す				 */
/* ========================================================================= */
static BOOL BattleProcTell(PCHARINFO pChar, PMAPINFO pMap, DWORD dwWait, BOOL bLate)
{
	static std::map<DWORD, DWORD> s_mapTell;		/* キャラごとに「！」を出した時の dwLastBattle */
	DWORD dwElapse, dwTell, dwX, dwY;
	PDWORD pData;
	int j, nCount;
	PCHARINFO pCharTmp;
	std::map<DWORD, DWORD>::iterator it;

	/* 敵だけ */
	if ((pChar->byNpcType != NPCTYPE_BATTLE1) && (pChar->byNpcType != NPCTYPE_BATTLE2)) {
		return FALSE;
	}
	/* 待ち時間が短い敵は、半分経ったところで出す */
	dwTell = ENEMYTELL_TIME;
	if (dwTell > dwWait / 2) {
		dwTell = dwWait / 2;
	}
	dwElapse = timeGetTime () - pChar->dwLastBattle;
	if (!bLate && (dwElapse + dwTell < dwWait)) {
		return FALSE;
	}
	/* この攻撃ではもう出した？ */
	it = s_mapTell.find (pChar->dwTblID);
	if ((it != s_mapTell.end ()) && (it->second == pChar->dwLastBattle)) {
		return FALSE;
	}

	/* 目の前にPCがいる？ */
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	pData = pMap->paTblNoPC;
	nCount = (int)pMap->dwCountTblNoPC;
	for (j = 0; j < nCount; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if (((CheckCharX (pChar, dwX, pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, dwX) == FALSE))) ||
			(dwY != pCharTmp->dwY)) {
			continue;
		}
		if ((pCharTmp->wHP == 0) || (pCharTmp->byCharState == CHARSTATE_DEAD)) {
			continue;
		}
		/* 攻撃できる時間を過ぎてから出した時は、ここから dwTell 待ってから殴る */
		if (bLate) {
			pChar->dwLastBattle = timeGetTime () - dwWait + dwTell;
			if (pChar->dwLastBattle <= 1) {
				pChar->dwLastBattle = 2;
			}
		}
		s_mapTell[pChar->dwTblID] = pChar->dwLastBattle;
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 64, 64), "！");
		return TRUE;
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：	BattleProcAtackArea												 */
/* 内容：	なぎ払い・貫通：狙った相手のほかに、武器の届くマスの敵にも当てる	 */
/*			なぎ払い＝自分の左右のマス(目の前と合わせて3方向)				 */
/*			貫通＝目の前のさらに1マス先										 */
/*			狙った相手以外には威力 ATACKRATE_AREA(%) で当たる				 */
/* ========================================================================= */
static void BattleProcAtackArea(PCHARINFO pChar, DWORD dwMainTblID)
{
	int i, j, nCount, nPos;
	PDWORD pData;
	PMAPINFO pMap;
	PITEMINFO pItem;
	PITEMINFO_WEAPON pWeapon;
	PCHARINFO pCharTmp;
	DWORD adwX[2], adwY[2], dwX, dwY, dwTargetBack;
	BYTE byRateBack;
	CmyArray<DWORD, DWORD> adwTblID;

	/* 武器を装備していない？ */
	if (pChar->waEqu[0] == 0xFFFF) {
		return;
	}
	pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
	if (pItem->pData == NULL) {
		return;
	}
	pWeapon = (PITEMINFO_WEAPON)pItem->pData;

	/* 一歩前の座標 */
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	/* 当たるマスを決める */
	switch (pWeapon->byAtackType) {
	case ATACKTYPE_SWEEP:		/* なぎ払い：自分の左右(向きと直角の方向)。敵は自分の上下左右に寄ってくるので */
		nPos = 2;
		adwX[0] = pChar->dwX + anPosAddTblY[pChar->byDirection];
		adwY[0] = pChar->dwY + anPosAddTblX[pChar->byDirection];
		adwX[1] = pChar->dwX - anPosAddTblY[pChar->byDirection];
		adwY[1] = pChar->dwY - anPosAddTblX[pChar->byDirection];
		break;
	case ATACKTYPE_PIERCE:		/* 貫通：目の前のさらに1マス先 */
		nPos = 1;
		adwX[0] = dwX + anPosAddTblX[pChar->byDirection];
		adwY[0] = dwY + anPosAddTblY[pChar->byDirection];
		break;
	default:
		return;
	}

	pMap = g_aMapInfo[pChar->dwMapID];
	pData = pMap->paTblNoNPC;
	nCount = (int)pMap->dwCountTblNoNPC;

	/* 先に当たる敵を集める(倒すとマップ内の一覧が変わるため) */
	for (j = 0; j < nCount; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if (pCharTmp->dwTblID == dwMainTblID) {
			continue;
		}
		/* 敵だけ */
		if ((pCharTmp->byNpcType != NPCTYPE_BATTLE1) && (pCharTmp->byNpcType != NPCTYPE_BATTLE2)) {
			continue;
		}
		if (pCharTmp->wHP == 0) {
			continue;
		}
		for (i = 0; i < nPos; i ++) {
			if (((CheckCharX (pChar, adwX[i], pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, adwX[i]) == FALSE))) ||
				(adwY[i] != pCharTmp->dwY)) {
				continue;
			}
			adwTblID.Add (pCharTmp->dwTblID);
			break;
		}
	}

	/* 狙った相手以外には弱めに当てる(薄い色のダメージ表示になる) */
	byRateBack		= pChar->byAtackRate;
	dwTargetBack	= pChar->dwTargetTblID;
	nCount = adwTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[adwTblID[i]];
		/* 途中で消えた？ */
		if ((pCharTmp->dwTblID != adwTblID[i]) || (pCharTmp->wHP == 0)) {
			continue;
		}
		pChar->byAtackRate		= ATACKRATE_AREA;
		pChar->dwTargetTblID	= pCharTmp->dwTblID;
		BattleProcAtack (pChar, pCharTmp);
	}
	pChar->byAtackRate		= byRateBack;
	pChar->dwTargetTblID	= dwTargetBack;
}

/* ========================================================================= */
/* 関数名：BattleProcAtack													 */
/* 内容：戦闘処理(直接攻撃)													 */
/* 日付：2003/10/02															 */
/* ========================================================================= */
BOOL BattleProcAtack(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	PCHARINFO pCharTmp1, pCharTmp2;
	BOOL bRet, bSPAtack, bMiss, bPC, bJust;
	int nDamage;
	DWORD dwSend;

	bRet = FALSE;

	/* ターゲットしているキャラではない？ */
	if (pChar->dwTargetTblID != pCharTarget->dwTblID) {
		goto Exit;
	}

	/* ログオフ中？ */
	if (pCharTarget->byGameState == GAMESTATE_LOGOFF) {
		pChar->dwLastBattle = 0;
		pChar->dwTargetTblID = -1;
		goto Exit;
	}

	/* 生きていない？ */
	if (pCharTarget->wHP == 0) {
		pChar->dwLastBattle = 0;
		pChar->dwTargetTblID = -1;
		goto Exit;
	}

	switch (pCharTarget->byNpcType) {
	case NPCTYPE_INIT:
	case NPCTYPE_BATTLE1:	/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
	case NPCTYPE_BATTLE2:	/* 周り２キャラの範囲で検索してついていって攻撃 */
		break;
	default:
		goto Exit;
	}


	bSPAtack	= FALSE;
	bMiss		= FALSE;
	nDamage		= 0;
	/* ジャスト（ゲージ満タンの瞬間に押した）？ 空振りせず、会心が出やすい */
	bJust		= (pChar->byAtackRate == ATACKRATE_JUST);
	if (bJust) {
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 255, 64), "ｼﾞｬｽﾄ!");
	}

	/* 空振り？ */
	if (!bJust && ((genrand () % 100) >= 95)) {
		BYTE byNo;

		bMiss		= TRUE;
		byNo		= SOUND_ATACK3;
		pCharTmp1	= pCharTarget;

		/* PCの攻撃？ */
		if (pChar->byNpcType == 0) {
			byNo		= SOUND_ATACK1;
			pCharTmp1	= pChar;
		}

		SendCmdPLAYSOUND (pCharTmp1->pID, byNo);
		SendCmdSYSMSG2 (pCharTmp1->pID, SYSMSG_ATACK1, pChar->pID, 0);

	/* 会心の一撃？(普段は 2%、ジャストなら 30%) */
	} else if ((genrand () % 100) >= (DWORD)(bJust ? 70 : 98)) {
		BYTE byNo;
		DWORD dwID;

		bSPAtack	= TRUE;
		byNo		= SOUND_ATACK2;
		pCharTmp1	= pCharTarget;
		pCharTmp2	= pChar;
		dwID		= SYSMSG_ATACK2;

		if (pChar->byNpcType == 0) {
			byNo = SOUND_ATACK2;
			pCharTmp1 = pChar;
			pCharTmp2 = pChar;
		}

		SendCmdSYSMSG2 (pCharTmp1->pID, dwID, pCharTmp2->pID, 0);
		SendCmdPLAYSOUNDinside (pCharTmp1->dwTblID, byNo);

	/* 通常の攻撃 */
	} else {
		if (pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdSYSMSG2 (pChar->pID, SYSMSG_ATACK1, pChar->pID, 0);

		} else {
			SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_ATACK1, pChar->pID, 0);
		}
	}

	/* 命中？ */
	if (!bMiss) {
		/* ダメージ量の計算 */
		nDamage = DamageProc (pChar, pCharTarget, bSPAtack);
	}

	/* 攻撃命中でダメージあり？ */
	if (!bMiss && (nDamage > 0)) {
		char szTmp2[16];
		COLORREF cl;

		pCharTarget->byEfect = EFECTTYPE_ATACK;

		/* 武器を装備している？ */
		if (pChar->waEqu[0] != 0xFFFF) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
			if (pItem->pData) {
				PITEMINFO_WEAPON pWeapon = (PITEMINFO_WEAPON)pItem->pData;
				/* 指定されたエフェクトを発動 */
				pCharTarget->byEfect = pWeapon->byEfcID;
			}
		}

		dwSend = pChar->pID;
		if (pChar->byNpcType) {
			if (pChar->byNpcType == NPCTYPE_ATACK) {
				PCHARINFO pCharParent;

				pCharParent = g_aCharInfo[pChar->dwParentTblID];
				dwSend = pCharParent->pID;

			} else {
				dwSend = pCharTarget->pID;
			}
		} 

		SendCmdSYSMSG2 (dwSend, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nDamage);
		SendCmdEFECTCHG (pChar->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;

		wsprintf (szTmp2, "%d", nDamage);
		cl = RGB (250, 60, 4);
		/* 早撃ち(ゲージ満タン前)のダメージは薄い色で出す */
		if ((pChar->byAtackRate > 0) && (pChar->byAtackRate < 100)) {
			cl = RGB (255, 180, 140);
		}

		if (bSPAtack) {
			cl = RGB (255, 255, 255);
		}
		SendCmdADDCHAREFC (pCharTarget->dwTblID, cl, szTmp2);

		if (pCharTarget->byNpcType == NPCTYPE_INIT) {
			/* クライアントにステータス更新通知 */
			SendCmdCHARSTATUSHPMANA (pCharTarget->dwTblID, pCharTarget->dwTblID);
		}

	/* 攻撃ミス？ */
	} else if (bMiss) {
		char szTmp2[16];

		dwSend = pChar->pID;
		if (pChar->byNpcType) {
			if (pChar->byNpcType == NPCTYPE_ATACK) {
				PCHARINFO pCharParent;

				pCharParent = g_aCharInfo[pChar->dwParentTblID];
				dwSend = pCharParent->pID;

			} else {
				dwSend = pCharTarget->pID;
			}
		} 

		SendCmdSYSMSG2 (dwSend, SYSMSG_MISS, 0, 0);
		if (pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdPLAYSOUND (pChar->pID, SOUND_ATACKMISS);
		}

		wsprintf (szTmp2, "%d", nDamage);
		SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 255, 255), "ﾐｽ!");

	/* 攻撃が効かなかった */
	} else {
		dwSend = pChar->pID;
		if (pChar->byNpcType) {
			if (pChar->byNpcType == NPCTYPE_ATACK) {
				PCHARINFO pCharParent;

				pCharParent = g_aCharInfo[pChar->dwParentTblID];
				dwSend = pCharParent->pID;

			} else {
				dwSend = pCharTarget->pID;
			}
		}

		SendCmdSYSMSG2 (dwSend, SYSMSG_NODAMAGE, 0, 0);
		if (pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdPLAYSOUND (pChar->pID, SOUND_DAMAGE0);
		}
	}


	bPC = FALSE;
	if (pChar->byNpcType == 0) {
		bPC = TRUE;

	} else if (pChar->byNpcType == NPCTYPE_ATACK) {
		PCHARINFO pCharParent;

		pCharParent = g_aCharInfo[pChar->dwParentTblID];
		if (pCharParent->byNpcType == 0) {
			bPC = TRUE;
		}
	}

	/* プレイヤーの攻撃？ */
	if (bPC) {
		bRet = DamageAfterProcNPC (pChar, pCharTarget);
		/* 攻撃命中でダメージあり？ */
		if (!bMiss && (nDamage > 0)) {
			AddStatus (pChar, STATUSTYPE_PW, 0);
			AddStatus (pChar, STATUSTYPE_DEX, 0);
		}

	/* NPCの攻撃 */
	} else {
		bRet = DamageAfterProcPC (pChar, pCharTarget);
		/* 攻撃命中でダメージあり？ */
		if (!bMiss && (nDamage > 0)) {
			AddStatus (pCharTarget, STATUSTYPE_ST, 0);
			AddStatus (pCharTarget, STATUSTYPE_MAXHP, nDamage);
		}
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	ResurrectionBack												 */
/* 内容：	復活位置で蘇生													 */
/* 日付：	2005/11/01														 */
/* ========================================================================= */

void ResurrectionBack(PCHARINFO pChar)
{
	SendCmdFADEINOUT (pChar->pID, 0);
	SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);

	/* 所持金半分 */
	pChar->dwGold /= 2;

	/* HPを回復して復帰ポイントへ移動 */
	pChar->wHP			= pChar->wMaxHP;
	pChar->wMana		= pChar->wMaxMana;
	pChar->byCharState	= CHARSTATE_MOVE;
	SubProcMapMove (
			pChar,
			pChar->dwStartMapID,
			pChar->dwStartX,
			pChar->dwStartY);
}


/* ========================================================================= */
/* 関数名：BattleProcMagic													 */
/* 内容：戦闘処理(魔法)														 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
static void BattleProcMagic(PCHARINFO pChar, PCHARINFO pCharTarget, BYTE byNo)
{
	MagicProc (pChar->dwTblID, pChar->abyMagic[byNo], pCharTarget->dwTblID);
}

/* ========================================================================= */
/* 関数名：GetWeponMin														 */
/* 内容：武器の減少値を取得													 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static int GetWeponMin(PCHARINFO pChar)
{
	int nTmp, nItemID;
	PITEMINFO pItem;
	PITEMINFO_WEAPON pInfo;

	nTmp = pChar->waEqu[0];
	/* 持っていないアイテム？ */
	if (nTmp >= MAX_MYITEM) {
		nTmp = 0;
		goto Exit;
	}

	nItemID = pChar->waItem[nTmp];
	nTmp = 0;
	/* 存在しないアイテム？ */
	if (nItemID >= g_aItemInfo.GetSize ()) {
		goto Exit;
	}

	pItem = g_aItemInfo[nItemID];
	/* 武器ではない？ */
	if (pItem->byType != ITEMTYPE_WEAPON) {
		goto Exit;
	}

	pInfo = (PITEMINFO_WEAPON)pItem->pData;
	/* 減少値が設定されている？ */
	if (pInfo && pInfo->byMin) {
		nTmp = ((genrand () % pInfo->byMin) + 1);
	}

Exit:
	return nTmp;
}

/* ========================================================================= */
/* 関数名：DamageProc														 */
/* 内容：ダメージ量計算														 */
/* 日付：2003/10/02															 */
/* ========================================================================= */
static int DamageProc(PCHARINFO pChar, PCHARINFO pCharTarget, BOOL bSPAtack)
{
	int nTmp, nTmp2, nAT, nDF;

	/* PCの行動？ */
	if (pChar->byAtackRate > 0) {
		/* ジャスト(101)も威力は 100% */
		nAT = pChar->wAT * min (pChar->byAtackRate, 100) / 100;
	} else {
		nAT = pChar->wAT;
	}
	nDF = pCharTarget->wDF;

	if (bSPAtack == FALSE) {
		/* 武器の減少値を取得 */
		nTmp = GetWeponMin (pChar);
		/* 減少幅を補正する */
		nAT -= nTmp;
	}

	/* 割り算用に補正 */
	nAT = max (nAT, 2);

	/* 会心の一撃 */
	if (bSPAtack) {
		nAT += (nAT / 2);
	}

	/* 直接攻撃のダメージ＝{ 攻撃力 - ( 守備力/2 ) }　+-16％ */
	nTmp = nAT - (nDF / 2);

	/* ＋－の補正 */
	if (genrand () % 2) {
		nTmp += (nTmp * ((genrand () % 16) + 1) / 100);
	} else {
		nTmp -= (nTmp * ((genrand () % 16) + 1) / 100);
	}
	nTmp = max (nTmp, 0);

	/* ノーダメージ？(おまけの1ダメージはゲージ満タンの攻撃だけ。早撃ちの連打で削れないように) */
	if ((nTmp == 0) && ((pChar->byAtackRate == 0) || (pChar->byAtackRate >= 100))) {
		if ((genrand () % 100) > 66) {
			nTmp = 1;
		}
	}

	nTmp2 = pCharTarget->wHP;
	nTmp2 -= nTmp;
	pCharTarget->wHP = (WORD)max (nTmp2, 0);
	if (pCharTarget->wHP == 0) {
		pCharTarget->byCharState = CHARSTATE_DEAD;
	}

	return nTmp;
}

/* ========================================================================= */
/* 関数名：GetAcquisitionGold												 */
/* 内容：獲得金額を計算														 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
static DWORD GetAcquisitionGold(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	DWORD dwRet;

	dwRet = pCharTarget->dwGold;

	/* お金持ってる？ */
	if (dwRet > 0) {
		/* パーティに所属している？ */
		if (pChar->dwPartyID) {
			PPARTYINFO pPartyInfo;

			pPartyInfo = g_aPartyInfo[pChar->dwPartyID];
			/* 全員で分配？ */
			if (pPartyInfo->byGold == PARTYGOLD_ALL) {
				int i;

				/* パーティ人数の計算 */
				for (i = 0; i < MAX_PARTYMEMBER; i ++) {
					if (pPartyInfo->adwTblID[i] == 0) {
						break;
					}
				}

				/* 人数分で割る */
				dwRet /= i;

				/* なくなっちゃう？ */
				if (dwRet == 0) {
					dwRet = pCharTarget->dwGold;
				}
			}
		}
	}

	return dwRet;
}

/* ========================================================================= */
/* 関数名：GetAcquisitionExp												 */
/* 内容：獲得経験値を計算													 */
/* 日付：2007/03/02															 */
/* ========================================================================= */
static DWORD GetAcquisitionExp(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	DWORD dwRet;

	dwRet = pCharTarget->dwExp;

	/* 経験値持ってる？ */
	if (pCharTarget->dwExp > 0) {
		/* パーティに所属している？ */
		if (pChar->dwPartyID) {
			PPARTYINFO pPartyInfo;

			pPartyInfo = g_aPartyInfo[pChar->dwPartyID];
			/* 全員で分配？ */
			if (pPartyInfo->byGold == PARTYGOLD_ALL) {
				int i;

				/* パーティ人数の計算 */
				for (i = 0; i < MAX_PARTYMEMBER; i ++) {
					if (pPartyInfo->adwTblID[i] == 0) {
						break;
					}
				}

				/* 人数分で割る */
				dwRet /= i;

				/* なくなっちゃう？ */
				if (dwRet == 0) {
					dwRet = pCharTarget->dwExp;
				}
			}
		}
	}

	return dwRet;
}

/* ========================================================================= */
/* 関数名：AddGold															 */
/* 内容：所持金を追加														 */
/* 日付：2004/07/20															 */
/* ========================================================================= */
static void AddGold(PCHARINFO pChar, DWORD dwGold, BOOL bSend)
{
	if (pChar->dwGold + dwGold <= 999999) {
		pChar->dwGold += dwGold;

	} else {
		pChar->dwGold = 999999;
	}

	pChar->dwGold = min (pChar->dwGold, 999999);

	/* 送信する？ */
	if (bSend) {
		SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
	}
}


/* ========================================================================= */
/* 関数名：	AddExp															 */
/* 内容：	経験値の追加													 */
/* 日付：	2005/12/19														 */
/* ========================================================================= */

void AddExp(PCHARINFO pChar, DWORD dwExp, BOOL bSend)
{
	BOOL bLevelUp;
	int nTmp, nCountLevel;
	char szTmp[128], szMsg[128];
	PJOBINFO pJobInfo;
	PLVUPINFO pLvUpInfo;

	if (dwExp == 0) {
		return;
	}

	pJobInfo = g_JobInfo.GetPtr ((int)pChar->byJOB);
	if (pJobInfo == NULL) {
		return;
	}
	nCountLevel = pJobInfo->aLevelUpInfo.GetSize ();
	/* 最高レベル？ */
	if (pChar->byLevel >= nCountLevel) {
		return;
	}

	bLevelUp		= FALSE;
	pChar->dwExp	+= dwExp;
	SendCmdSYSMSG2 (pChar->pID, SYSMSG_GETEXP, pChar->pID, dwExp);

	while (1) {
		if (pChar->byLevel >= nCountLevel) {
			break;
		}
		pLvUpInfo = pJobInfo->aLevelUpInfo[pChar->byLevel];
		if (pChar->dwExp < pLvUpInfo->dwExp) {
			break;
		}
		pChar->byEfect = 91;
		SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
		pChar->byEfect = 0;

		wsprintf (szMsg, "レベルアップしました！");
		SendCmdSYSMSG (pChar->pID, szMsg, SPEECHCMD_BATTLE1);

		ZeroMemory (szMsg, sizeof (szMsg));
		if (pChar->byPW < pLvUpInfo->byPW) {
			nTmp = (rand () % (pLvUpInfo->byPW - pChar->byPW)) + 1;
			wsprintf (szTmp, "力+%d ", nTmp);
			pChar->byPW += nTmp;
			strcat (szMsg, szTmp);
		}

		if (pChar->byDEX < pLvUpInfo->byDEX) {
			nTmp = (rand () % (pLvUpInfo->byDEX - pChar->byDEX)) + 1;
			wsprintf (szTmp, "素早さ+%d ", nTmp);
			pChar->byDEX += nTmp;
			strcat (szMsg, szTmp);
		}

		if (pChar->byST < pLvUpInfo->byST) {
			nTmp = (rand () % (pLvUpInfo->byST - pChar->byST)) + 1;
			wsprintf (szTmp, "体力+%d ", nTmp);
			pChar->byST += nTmp;
			strcat (szMsg, szTmp);
		}

		if (pChar->byIT < pLvUpInfo->byIT) {
			nTmp = (rand () % (pLvUpInfo->byIT - pChar->byIT)) + 1;
			wsprintf (szTmp, "賢さ+%d ", nTmp);
			pChar->byIT += nTmp;
			strcat (szMsg, szTmp);
		}

		if (pChar->wMaxHP < pLvUpInfo->wMAXHP) {
			nTmp = (rand () % (pLvUpInfo->wMAXHP - pChar->wMaxHP)) + 1;
			wsprintf (szTmp, "最大HP+%d ", nTmp);
			pChar->wMaxHP += nTmp;
			strcat (szMsg, szTmp);
		}

		if (pChar->wMaxMana < pLvUpInfo->wMAXMANA) {
			nTmp = (rand () % (pLvUpInfo->wMAXMANA - pChar->wMaxMana)) + 1;
			wsprintf (szTmp, "最大Mana+%d ", nTmp);
			pChar->wMaxMana += nTmp;
			strcat (szMsg, szTmp);
		}
		pChar->byLevel ++;
		SendCmdSYSMSG (pChar->pID, szMsg, SPEECHCMD_BATTLE1);

		/* クライアントにステータス更新通知 */
		SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
		bLevelUp = TRUE;
	}

	if (bLevelUp == FALSE) {
		/* 送信する？ */
		if (bSend) {
			SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
		}
	}
}


/* ========================================================================= */
/* 関数名：	AddStatus														 */
/* 内容：	パラメータの追加												 */
/* 日付：	2005/11/10														 */
/* ========================================================================= */

void AddStatus(PCHARINFO pChar, BYTE byType, int nData)
{
	int nTmp, nRate1, nRate2;
	char szTmp[16], szMsg[128];
	ULONGLONG ulTmp, ulNow, ulRate, ulData;

return;
	nRate1 = nRate2 = 1;

	switch (byType) {
	case STATUSTYPE_PW:				/* 力 */
		ulNow	= pChar->byPW;
		ulData	= 255;
		nRate2	= 2;
		break;
	case STATUSTYPE_DEX:			/* 素早さ */
		ulNow	= pChar->byDEX;
		ulData	= 255;
		nRate2	= 2;
		break;
	case STATUSTYPE_ST:				/* 体力 */
		ulNow	= pChar->byST;
		ulData	= 255;
		nRate2	= 2;
		break;
	case STATUSTYPE_IT:				/* 賢さ */
		ulNow	= pChar->byIT;
		ulData	= 255;
		nRate2	= 2;
		break;
	case STATUSTYPE_LK:				/* 運のよさ */
		ulNow	= pChar->byLK;
		ulData	= 255;
		nRate2	= 2;
		break;
	case STATUSTYPE_MAXHP:			/* 最大HP */
		nTmp = pChar->wMaxHP / 10;
		nTmp = max (nTmp, 1);
		if (!((nData >= nTmp) || (pChar->wHP < nTmp))) {
			return;
		}
		ulNow	= pChar->wMaxHP;
		ulData	= 65535;
		nRate1	= 2;
		break;
	case STATUSTYPE_MAXMANA:		/* 最大MANA */
		nTmp = pChar->wMaxMana / 10;
		nTmp = max (nTmp, 1);
		if (!((nData >= nTmp) || (pChar->wMana < nTmp))) {
			return;
		}
		ulNow	= pChar->wMaxMana;
		ulData	= 65535;
		nRate1	= 2;
		break;
	default:
		return;
	}

	if (ulNow >= ulData) {
		return;
	}

	ulTmp	= ulNow * 100 / ulData;
	ulRate	= (100 - ulTmp) / 10;
	ulRate	*= nRate1;
	ulRate	/= nRate2;
	ulRate	= max (ulRate, 1);
	if (ulRate < genrand () % 100) {
		return;
	}

	switch (byType) {
	case STATUSTYPE_PW:				/* 力 */
		pChar->byPW ++;
		wsprintf (szMsg, "%sの力が1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_DEX:			/* 素早さ */
		pChar->byDEX ++;
		wsprintf (szMsg, "%sの素早さが1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_ST:				/* 体力 */
		pChar->byST ++;
		wsprintf (szMsg, "%sの体力が1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_IT:				/* 賢さ */
		pChar->byIT ++;
		wsprintf (szMsg, "%sの賢さが1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_LK:				/* 運のよさ */
		pChar->byLK ++;
		wsprintf (szMsg, "%sの運のよさが1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_MAXHP:			/* 最大HP */
		pChar->wMaxHP ++;
		wsprintf (szMsg, "%sの最大HPが1ポイント増えました！", pChar->szName);
		break;
	case STATUSTYPE_MAXMANA:		/* 最大Mana */
		pChar->wMaxMana ++;
		wsprintf (szMsg, "%sの最大Manaが1ポイント増えました！", pChar->szName);
		break;
	}

	SendCmdSYSMSG (pChar->pID, szMsg, SPEECHCMD_BATTLE1);

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);

	wsprintf (szTmp, "%d", 1);
	SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);

	/* パラメータを更新 */
	RenewATDF (pChar->dwTblID);
}

/* Copyright(C)URARA-works 2003-2005 */
