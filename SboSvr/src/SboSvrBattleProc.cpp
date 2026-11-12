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
#include "SboSvrCharInfoTimerProc.h"
#include "CharProc/CharProcMERCENARY.h"
#include <map>
#include <vector>

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
static	int		GetEnemyTurnTime	(PCHARINFO);					/* 敵の行動の間隔(ms) */
static	BOOL	IsEnemy				(PCHARINFO);					/* 戦闘１・戦闘２の敵か */
static	BYTE	GetDirectionTo		(PCHARINFO, PCHARINFO, BYTE);	/* 相手のいる方の向き */
static	BOOL	IsHealMagic			(PMAGICINFO);					/* 味方(自分側)のHPを増やす特技か */
static	BOOL	IsNeedHeal			(PCHARINFO, PMAGICINFO);		/* 回復の特技を使う意味があるか */
static	BOOL	CanReachMagic		(PCHARINFO, PCHARINFO, PMAGICINFO, BYTE *);	/* 離れた相手に届く特技か */
static	int		ChooseEnemyMagic	(PCHARINFO, PCHARINFO, BOOL);	/* 敵が使う特技の枠を選ぶ */
static	BOOL	BattleProcRangeMagic(PCHARINFO, PMAPINFO);			/* 離れた相手へ特技を使う */
static	int		GetWeponMin			(PCHARINFO);					/* 武器の減少値を取得 */
static	int		DamageProc			(PCHARINFO, PCHARINFO, BOOL, BOOL, int);	/* ダメージ量計算 */
		int		RenkeiCheck			(PCHARINFO, PCHARINFO);			/* 連携ボーナスの倍率(%) */

#define RENKEI_TIME		2000		/* 別の人が殴ってからこの時間(ms)以内に殴ると連携 */
#define ENEMYHEAL_RATE	70			/* 敵はHPがこの割合(%)以上なら回復の特技を使わない */
#define ENEMYHEAL_PINCH	30			/* 敵はHPがこの割合(%)以下なら回復の特技を優先する */
#define ENEMYHEAL_PINCHRATE	50		/* HPが少ない時に回復を優先する確率(%) */
#define RENKEI_RATE		130			/* 連携の時のダメージ(%) */
		void	AddBattleDamage		(PCHARINFO, PCHARINFO, int);	/* 敵に与えたダメージを覚える(報酬を分ける用) */
		void	ClearBattleDamage	(DWORD);						/* 覚えたダメージを消す(キャラ削除時) */
static	void	GetRewardMember		(PCHARINFO, PCHARINFO, std::vector<std::pair<PCHARINFO, DWORD> > &);	/* 討伐報酬をもらえるプレイヤー */
static	void	AddGold				(PCHARINFO, DWORD, BOOL);		/* 所持金を追加 */
void	AddExp						(PCHARINFO, DWORD, BOOL);		/* 経験値を追加 */

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

	/* 傭兵は自分の処理(CharProcMERCENARY)で殴る */
	if (IsMercenary (pChar)) {
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

	/* 眠っている？(PCは攻撃要求を取り消し、敵は起きてから1回分待つ) */
	if (IsSleep (pChar)) {
		if (pChar->byNpcType == NPCTYPE_INIT) {
			pChar->dwLastBattle	 = 0;
			pChar->dwTargetTblID = -1;
		} else if (pChar->dwLastBattle > 1) {
			pChar->dwLastBattle = timeGetTime ();
		}
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

	/* 攻撃タイミングになっているかを計算。最大4秒で255を100%とする。加速していれば間隔が短くなる */
	nTmp = GetEnemyTurnTime (pChar);

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

	BOOL bAtack = FALSE;
	/* マップ内のキャラを処理 */
	for (j = 0; j < nCount; j ++) {
		if (pData[j] == -1) {
			break;
		}

		BOOL bResult;
		PCHARINFO pCharTmp;

		bResult = TRUE;
		pCharTmp = g_aCharInfo[pData[j]];
		/* 依頼の見えるNPC・別の空間のキャラは攻撃の相手にしない */
		if (!IsInteractable (pChar, pCharTmp)) {
			continue;
		}

		/* 目の前にはいない？ */
		if (((CheckCharX (pChar, dwX, pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, dwX) == FALSE))) ||
			(dwY != pCharTmp->dwY)) {
			continue;
		}
		/* 敵は狙っている相手にだけ行動する（以前は特技だけ、目の前の誰にでも使っていた） */
		if (IsEnemy (pChar) && (pChar->dwTargetTblID != pCharTmp->dwTblID)) {
			continue;
		}
		bAtack = TRUE;

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

	/* 敵の目の前にプレイヤーがいなくて、傭兵がいる？ 傭兵を殴る */
	if (!bAtack && (pChar->byNpcType != NPCTYPE_INIT)) {
		PCHARINFO pMerc = GetMercenaryFront (pChar);
		if (pMerc) {
			BattleProc (pChar, pMerc);
			pChar->dwLastBattle = timeGetTime ();
			bAtack = TRUE;
		}
	}
	/* 狙っている相手が目の前にいない？ 届く特技があれば離れたところから使う */
	if (!bAtack && IsEnemy (pChar)) {
		BattleProcRangeMagic (pChar, pMap);
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
		/* 依頼の見えるNPC・別の空間のキャラは攻撃の相手にしない */
		if (!IsInteractable (pChar, pCharTmp)) {
			continue;
		}

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

		/* 傭兵・プレイヤー以外(敵どうしの魔法など)からの攻撃では狙いを変えない */
		if ((pChar->byNpcType != NPCTYPE_INIT) && !IsMercenary (pChar)) {
			return FALSE;
		}
		if (pCharTarget->dwLastBattle == 0) {
			/* 最初に攻撃してきたキャラをターゲットにする（向きは相手のいる方。離れた所から撃たれた時も） */
			pCharTarget->dwTargetTblID = pChar->dwTblID;
			pCharTarget->byDirection = GetDirectionTo (pCharTarget, pChar, abyDirectionTbl[pChar->byDirection]);
			pCharTarget->byFlg |= CHARINFOFLG_DIRECTION;
			pCharTarget->dwLastBattle = 1;

		} else {
			/* ターゲット以外からの攻撃？ */
			if (pChar->dwTblID != pCharTarget->dwTargetTblID) {
				/* 敵自身の行動の間隔で数えて（以前は殴った人の素早さで数えていた）、
				   2ターン以上行動していない？ 今の相手とは戦えていないので、殴ってきた人に乗り換える */
				nTmp = GetEnemyTurnTime (pCharTarget);
				if ((pCharTarget->dwLastBattle <= 1) ||
					((DWORD)(timeGetTime () - pCharTarget->dwLastBattle) / (DWORD)nTmp >= 2)) {
					pCharTarget->dwTargetTblID = pChar->dwTblID;
					if (abs (GetCharDistanceX (pCharTarget, pChar)) + abs ((int)pChar->dwY - (int)pCharTarget->dwY) == 1) {
						pCharTarget->byDirection = GetDirectionTo (pCharTarget, pChar, pCharTarget->byDirection);
						pCharTarget->byFlg |= CHARINFOFLG_DIRECTION;
					}
				}
			}
		}
		return FALSE;
	}

	/* 傭兵が倒した？ 討伐報酬は雇い主がもらう */
	if (IsMercenary (pChar)) {
		PCHARINFO pOwner = GetMercenaryOwner (pChar);
		if (pOwner == NULL) {
			DeleteChar (pCharTarget->dwTblID);
			return TRUE;
		}
		pChar = pOwner;
	}

	SendCmdSYSMSG2 (pChar->pID, SYSMSG_BATTLEEND1, pCharTarget->pID, 0);

	/* 討伐報酬をもらえるプレイヤー(ダメージを与えた人＋「全員で分配」パーティの仲間)。先頭はとどめを刺した人 */
	std::vector<std::pair<PCHARINFO, DWORD> > aMember;
	GetRewardMember (pChar, pCharTarget, aMember);
	int nMember = (int)aMember.size ();
	int i;

	/* 経験値はみんな満額 */
	dwTmp = pCharTarget->dwExp;
	if (dwTmp > 0) {
		for (i = 0; i < nMember; i ++) {
			AddExp (aMember[i].first, dwTmp, TRUE);
		}
	}

	/* お金は人数で割る(1人1ゴールドは残す) */
	dwGoldTmp = pCharTarget->dwGold;
	if (dwGoldTmp > 0) {
		dwGoldTmp = max (dwGoldTmp / (DWORD)nMember, (DWORD)1);
		for (i = 0; i < nMember; i ++) {
			PCHARINFO pCharTmp = aMember[i].first;
			AddGold (pCharTmp, dwGoldTmp, TRUE);
			SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_BATTLEEND3, pCharTmp->pID, dwGoldTmp);
		}
	}

	BYTE byGetItem = 0;
	int nGetItemNo = 0;
	PCHARINFO pCharGet = pChar;

	/* アイテム落とす？ */
	if (pCharTarget->byDropProbability) {
		int nCount = GetItemCount (pCharTarget->dwTblID);
		if (nCount && ((genrand () % 100) + pCharTarget->byDropProbability >= 100)) {
			nGetItemNo = pCharTarget->waItem[genrand () % nCount];

			/* 誰がもらうかは、与えたダメージが多い人ほど当たりやすいくじ引き */
			DWORD dwTotal = 0;
			for (i = 0; i < nMember; i ++) {
				dwTotal += aMember[i].second;
			}
			if (dwTotal > 0) {
				DWORD dwLot = (DWORD)genrand () % dwTotal;
				for (i = 0; i < nMember; i ++) {
					if (dwLot < aMember[i].second) {
						pCharGet = aMember[i].first;
						break;
					}
					dwLot -= aMember[i].second;
				}
			}

			/* もう持てない？ */
			if (GetItemCount (pCharGet->dwTblID) >= MAX_MYITEM) {
				byGetItem = 1;
				/* 手持ちがいっぱいなら預かり所へ送る（預かり所もいっぱいならあきらめる） */
				if (AddLeaveItem (pCharGet, (WORD)nGetItemNo)) {
					byGetItem = 3;
				}

			} else {
				byGetItem = 2;
				AddCharItem (pCharGet->dwTblID, nGetItemNo);
				SendCmdCHARITEM (pCharGet->pID, pCharGet->dwTblID);
			}
		}
	}

	/* 報酬をもらった人と、ターゲットにしていた他のプレイヤーへ通知 */
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	std::vector<PCHARINFO> aNotify;

	for (i = 0; i < nMember; i ++) {
		aNotify.push_back (aMember[i].first);
	}
	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		if (pCharTmp->dwTargetTblID != pCharTarget->dwTblID) {
			continue;
		}
		if (pCharTmp->dwTblID != pChar->dwTblID) {
			/* ターゲット解除 */
			pCharTmp->dwLastBattle = 0;
			pCharTmp->dwTargetTblID = -1;
			SendCmdTARGETCHAR (pCharTmp->dwTblID, -1);
		}
		int j;
		for (j = 0; j < (int)aNotify.size (); j ++) {
			if (aNotify[j] == pCharTmp) {
				break;
			}
		}
		if (j >= (int)aNotify.size ()) {
			aNotify.push_back (pCharTmp);
		}
	}

	for (i = 0; i < (int)aNotify.size (); i ++) {
		PCHARINFO pCharTmp = aNotify[i];

		/* 倒したプレイヤー以外にメッセージ通知 */
		if (pCharTmp->dwTblID != pChar->dwTblID) {
			SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_BATTLEEND2, pCharTarget->pID, pChar->pID);
		}

		/* アイテム落とした？ */
		if (byGetItem) {
			SendCmdPLAYSOUND (pCharTmp->pID, SOUND_PI52);
			SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM1, pCharTarget->pID, nGetItemNo);
			if (byGetItem == 1) {
				SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM2, 0, 0);
			} else if (byGetItem == 3) {
				/* 預かり所へ送った（預かり所と契約した冒険者には使い魔が付いていて運んでくれる、という設定） */
				char szTmp[256];
				snprintf (szTmp, sizeof (szTmp), "持ちきれないので、預かり所の使い魔が%sの預かり所へ運んでいきました", pCharGet->szName);
				SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
			} else {
				SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_GETITEM3, pCharGet->pID, nGetItemNo);
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

	/* 傭兵には特技を使わない(特技の相手はプレイヤーと敵だけの作りのため) */
	byNo = 0;
	if (pChar->byNpcType && !IsMercenary (pCharTarget)) {
		int nNo = ChooseEnemyMagic (pChar, pCharTarget, FALSE);
		if (nNo >= 0) {
			byNo = (BYTE)nNo;
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

			pCharNew->dwSpaceID = pChar->dwSpaceID;		/* 矢は撃った人と同じ空間 */
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
/* 関数名：	SetGuard / IsGuard												 */
/* 内容：	構え(防御)の状態。キャラ番号ごとに、構えた時のプレイヤーIDを持つ	 */
/*			(ログインし直して同じ番号を別の人が使っても引き継がないように)	 */
/* ========================================================================= */
static std::map<DWORD, DWORD> s_mapGuard;

void SetGuard(PCHARINFO pChar, BOOL bGuard)
{
	if (bGuard) {
		s_mapGuard[pChar->dwTblID] = pChar->pID;
	} else {
		s_mapGuard.erase (pChar->dwTblID);
	}
}

BOOL IsGuard(PCHARINFO pChar)
{
	std::map<DWORD, DWORD>::iterator it;

	it = s_mapGuard.find (pChar->dwTblID);
	if (it == s_mapGuard.end ()) {
		return FALSE;
	}
	if ((it->second != pChar->pID) || (pChar->wHP == 0)) {
		s_mapGuard.erase (it);
		return FALSE;
	}
	return TRUE;
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
		/* 別の空間にいる人には予兆を出さない */
		if (!IsSameSpace (pChar, pCharTmp)) {
			continue;
		}
		if ((pCharTmp->wHP == 0) || (pCharTmp->byCharState == CHARSTATE_DEAD)) {
			continue;
		}
		/* 狙っている相手にだけ殴るので、予兆もその相手にだけ */
		if (pCharTmp->dwTblID != pChar->dwTargetTblID) {
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
		/* ダメージの数字と見分けやすいように、吹き出しの「!」で知らせる */
		pChar->byComEfc = COMEFC_TELL;
		SendCmdCOMEFC (pChar);
		pChar->byComEfc = 0;
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
		/* 別の空間にいるキャラには当たらない */
		if (!IsSameSpace (pChar, pCharTmp)) {
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

/* 攻撃のメッセージを送る相手（傭兵の攻撃は雇い主に、ほかは攻撃された側に） */
static DWORD AtackMsgPID(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	if (IsMercenary (pChar)) {
		PCHARINFO pOwner = GetMercenaryOwner (pChar);
		if (pOwner) {
			return pOwner->pID;
		}
	}
	return pCharTarget->pID;
}

/* ========================================================================= */
/* 関数名：BattleProcAtack													 */
/* 内容：戦闘処理(直接攻撃)													 */
/* 日付：2003/10/02															 */
/* ========================================================================= */
BOOL BattleProcAtack(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	PCHARINFO pCharTmp1, pCharTmp2;
	BOOL bRet, bSPAtack, bMiss, bPC, bJust, bGuard;
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
	case NPCTYPE_MERCENARY:	/* 傭兵 */
		break;
	default:
		goto Exit;
	}


	bSPAtack	= FALSE;
	bMiss		= FALSE;
	nDamage		= 0;
	/* ジャスト（ゲージ満タンの瞬間に押した）？ 空振りせず、会心が出やすい */
	bJust		= (pChar->byAtackRate == ATACKRATE_JUST);
	/* 構えているPC？ 受けるダメージが半分になり、会心を受けない */
	bGuard		= ((pCharTarget->byNpcType == NPCTYPE_INIT) && IsGuard (pCharTarget));
	if (bJust) {
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 255, 64) | CHAREFC_BIG, "JUST!");
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
		SendCmdSYSMSG2 ((pChar->byNpcType == 0) ? pCharTmp1->pID : AtackMsgPID (pChar, pCharTarget), SYSMSG_ATACK1, pChar->pID, 0);

	/* 会心の一撃？(普段は 2%、ジャストなら 30%) */
	} else if (!bGuard && ((genrand () % 100) >= (DWORD)(bJust ? 70 : 98))) {
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

		SendCmdSYSMSG2 ((pChar->byNpcType == 0) ? pCharTmp1->pID : AtackMsgPID (pChar, pCharTarget), dwID, pCharTmp2->pID, 0);
		SendCmdPLAYSOUNDinside (pCharTmp1->dwTblID, byNo);

	/* 通常の攻撃 */
	} else {
		if (pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdSYSMSG2 (pChar->pID, SYSMSG_ATACK1, pChar->pID, 0);

		} else {
			SendCmdSYSMSG2 (AtackMsgPID (pChar, pCharTarget), SYSMSG_ATACK1, pChar->pID, 0);
		}
	}

	/* 命中？ */
	if (!bMiss) {
		/* ダメージ量の計算 */
		nDamage = DamageProc (pChar, pCharTarget, bSPAtack, bGuard, RenkeiCheck (pChar, pCharTarget));
		if (bGuard) {
			SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (120, 200, 255), "ｶﾞｰﾄﾞ");
		}
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
				dwSend = AtackMsgPID (pChar, pCharTarget);
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
				dwSend = AtackMsgPID (pChar, pCharTarget);
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
				dwSend = AtackMsgPID (pChar, pCharTarget);
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

	/* 傭兵の攻撃はプレイヤーの攻撃と同じ扱い */
	} else if (IsMercenary (pChar)) {
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
		BOOL bMerc = IsMercenary (pChar);
		bRet = DamageAfterProcNPC (pChar, pCharTarget);
		/* 攻撃命中でダメージあり？(傭兵は能力値が上がらない) */
		if (!bMiss && (nDamage > 0) && !bMerc) {
			AddStatus (pChar, STATUSTYPE_PW, 0);
			AddStatus (pChar, STATUSTYPE_DEX, 0);
		}

	/* 傭兵が攻撃を受けた */
	} else if (IsMercenary (pCharTarget)) {
		bRet = MercenaryDamaged (pChar, pCharTarget);

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
	BYTE byID = pChar->abyMagic[byNo];

	/* 敵が「選んだ相手」に使う回復は、戦っている相手ではなく自分に使う */
	if ((pChar->byNpcType != NPCTYPE_INIT) && (byID < g_aMagicInfo.GetSize ()) && IsHealMagic (g_aMagicInfo[byID])) {
		pCharTarget = pChar;
	}
	MagicProc (pChar->dwTblID, byID, pCharTarget->dwTblID);
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
static int DamageProc(PCHARINFO pChar, PCHARINFO pCharTarget, BOOL bSPAtack, BOOL bGuard, int nRate)
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
	/* 構えている相手には半分 */
	if (bGuard) {
		nTmp /= 2;
	}
	/* 連携ボーナス */
	nTmp = nTmp * nRate / 100;

	/* ノーダメージ？(おまけの1ダメージはゲージ満タンの攻撃だけ。早撃ちの連打で削れないように) */
	if ((nTmp == 0) && ((pChar->byAtackRate == 0) || (pChar->byAtackRate >= 100))) {
		if ((genrand () % 100) > 66) {
			nTmp = 1;
		}
	}

	AddBattleDamage (pChar, pCharTarget, nTmp);
	nTmp2 = pCharTarget->wHP;
	nTmp2 -= nTmp;
	pCharTarget->wHP = (WORD)max (nTmp2, 0);
	if (pCharTarget->wHP == 0) {
		pCharTarget->byCharState = CHARSTATE_DEAD;
	}

	return nTmp;
}

/* ========================================================================= */
/* 討伐報酬を分けるために、敵ごとに「誰がどれだけダメージを与えたか」を覚えておく */
/* 敵のtblID → (プレイヤーのtblID → (pID, ダメージ合計))					 */
/* pID も覚えるのは、ログアウトした人の席に別の人が入った時に間違えないため	 */
/* ========================================================================= */
static std::map<DWORD, std::map<DWORD, std::pair<DWORD, DWORD> > > s_mapBattleDamage;

/* ========================================================================= */
/* 関数名：	AddBattleDamage													 */
/* 内容：	プレイヤーが敵に与えたダメージを覚える。HPを減らす直前に呼ぶ	 */
/*			nDamage は実際に減ったHP(残りHPより多い分は数えない)			 */
/* ========================================================================= */
void AddBattleDamage(PCHARINFO pChar, PCHARINFO pCharTarget, int nDamage)
{
	if ((pChar == NULL) || (pCharTarget == NULL) || (nDamage <= 0)) {
		return;
	}
	/* 矢などは撃ったキャラ */
	if (pChar->byNpcType == NPCTYPE_ATACK) {
		if ((pChar->dwParentTblID == (DWORD)-1) || (pChar->dwParentTblID >= (DWORD)g_aCharInfo.GetSize ())) {
			return;
		}
		pChar = g_aCharInfo[pChar->dwParentTblID];
	}
	/* 傭兵が与えたダメージは雇い主の分 */
	if (IsMercenary (pChar)) {
		pChar = GetMercenaryOwner (pChar);
		if (pChar == NULL) {
			return;
		}
	}
	/* プレイヤーが敵に与えた時だけ */
	if ((pChar->byNpcType != NPCTYPE_INIT) || (pCharTarget->byNpcType == NPCTYPE_INIT) || IsMercenary (pCharTarget)) {
		return;
	}
	nDamage = min (nDamage, (int)pCharTarget->wHP);
	if (nDamage <= 0) {
		return;
	}

	std::pair<DWORD, DWORD> &info = s_mapBattleDamage[pCharTarget->dwTblID][pChar->dwTblID];
	if (info.first != pChar->pID) {
		/* 同じ席の前の人の分は捨てる */
		info.first	= pChar->pID;
		info.second	= 0;
	}
	info.second += (DWORD)nDamage;
}

/* ========================================================================= */
/* 関数名：	ClearBattleDamage												 */
/* 内容：	覚えたダメージを消す(キャラ削除時に呼ぶ)						 */
/* ========================================================================= */
void ClearBattleDamage(DWORD dwTblID)
{
	s_mapBattleDamage.erase (dwTblID);
}

/* ========================================================================= */
/* 関数名：	GetRewardMember													 */
/* 内容：	討伐報酬をもらえるプレイヤーと、与えたダメージを集める			 */
/*			・とどめを刺した人(先頭)										 */
/*			・ダメージを与えた人											 */
/*			・上の人が「全員で分配」のパーティなら、その仲間				 */
/*			  (敵から画面内の範囲＝縦横10マス以内にいる仲間だけ)			 */
/*			どれも敵と同じマップにいて、倒れていない人だけ					 */
/* ========================================================================= */
static void GetRewardMember(PCHARINFO pChar, PCHARINFO pCharTarget, std::vector<std::pair<PCHARINFO, DWORD> > &aMember)
{
	std::map<DWORD, std::map<DWORD, std::pair<DWORD, DWORD> > >::iterator itTarget;
	std::map<DWORD, std::pair<DWORD, DWORD> >::iterator it;
	int i, j, nCount;

	aMember.clear ();
	nCount = g_aCharInfo.GetSize ();

	/* とどめを刺した人 */
	aMember.push_back (std::make_pair (pChar, (DWORD)0));

	/* ダメージを与えた人 */
	itTarget = s_mapBattleDamage.find (pCharTarget->dwTblID);
	if (itTarget != s_mapBattleDamage.end ()) {
		for (it = itTarget->second.begin (); it != itTarget->second.end (); it ++) {
			if (it->first >= (DWORD)nCount) {
				continue;
			}
			PCHARINFO pCharTmp = g_aCharInfo[it->first];
			if ((pCharTmp->pID == 0) || (pCharTmp->pID != it->second.first)) {
				continue;		/* もういない */
			}
			if (pCharTmp == pChar) {
				aMember[0].second = it->second.second;
				continue;
			}
			if ((pCharTmp->byNpcType != NPCTYPE_INIT) || (pCharTmp->dwMapID != pCharTarget->dwMapID) || !IsSameSpace (pCharTmp, pCharTarget) ||
				(pCharTmp->byCharState == CHARSTATE_DEAD) || (pCharTmp->wHP == 0)) {
				continue;
			}
			aMember.push_back (std::make_pair (pCharTmp, it->second.second));
		}
	}

	/* 「全員で分配」のパーティの仲間 */
	int nDamageMember = (int)aMember.size ();
	for (j = 0; j < nDamageMember; j ++) {
		DWORD dwPartyID = aMember[j].first->dwPartyID;
		if (dwPartyID == 0) {
			continue;
		}
		PPARTYINFO pPartyInfo = g_aPartyInfo[dwPartyID];
		if ((pPartyInfo == NULL) || (pPartyInfo->byGold != PARTYGOLD_ALL)) {
			continue;
		}
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}
			if (pPartyInfo->adwTblID[i] >= (DWORD)nCount) {
				continue;
			}
			PCHARINFO pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			if ((pCharTmp->pID == 0) || (pCharTmp->byNpcType != NPCTYPE_INIT) ||
				(pCharTmp->dwMapID != pCharTarget->dwMapID) || !IsSameSpace (pCharTmp, pCharTarget) ||
				(pCharTmp->byCharState == CHARSTATE_DEAD) || (pCharTmp->wHP == 0)) {
				continue;
			}
			/* 敵が画面内に見えない(HPバーも見えない)ほど離れた仲間はもらえない。
			   範囲は周りへキャラ情報を送る範囲(SENDINSIDE_INSIDE)と同じ */
			if ((abs ((int)pCharTmp->dwX - (int)pCharTarget->dwX) > DRAW_PARTS_X + 3) ||
				(abs ((int)pCharTmp->dwY - (int)pCharTarget->dwY) > DRAW_PARTS_Y + 3)) {
				continue;
			}
			int k;
			for (k = 0; k < (int)aMember.size (); k ++) {
				if (aMember[k].first == pCharTmp) {
					break;
				}
			}
			if (k >= (int)aMember.size ()) {
				aMember.push_back (std::make_pair (pCharTmp, (DWORD)0));
			}
		}
	}

	/* 一度も当てずに倒した時(念のため)はとどめの人がドロップをもらえるように */
	if (aMember[0].second == 0) {
		DWORD dwTotal = 0;
		for (i = 0; i < (int)aMember.size (); i ++) {
			dwTotal += aMember[i].second;
		}
		if (dwTotal == 0) {
			aMember[0].second = 1;
		}
	}
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

/* ========================================================================= */
/* 関数名：	RenkeiCheck														 */
/* 内容：	連携ボーナス。同じ敵を、別のプレイヤーが殴って(特技を当てて)から	 */
/*			RENKEI_TIME 以内に殴るとダメージが RENKEI_RATE% になり「連携!」	 */
/*			戻り値はダメージの倍率(%)。当たった時だけ呼ぶこと				 */
/* ========================================================================= */
int RenkeiCheck(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	static std::map<DWORD, std::pair<DWORD, DWORD> > s_mapLastHit;	/* 敵ごとの 最後に当てたPC・時間 */
	std::map<DWORD, std::pair<DWORD, DWORD> >::iterator it;
	DWORD dwNow;
	int nRet;

	/* 矢などは撃ったキャラ */
	if ((pChar->byNpcType == NPCTYPE_ATACK) && (pChar->dwParentTblID != (DWORD)-1) &&
		(pChar->dwParentTblID < (DWORD)g_aCharInfo.GetSize ())) {
		pChar = g_aCharInfo[pChar->dwParentTblID];
	}
	/* プレイヤー(と傭兵)が敵に当てた時だけ */
	if (((pChar->byNpcType != NPCTYPE_INIT) && !IsMercenary (pChar)) ||
		(pCharTarget->byNpcType == NPCTYPE_INIT) || IsMercenary (pCharTarget)) {
		return 100;
	}
	nRet	= 100;
	dwNow	= timeGetTime ();
	it		= s_mapLastHit.find (pCharTarget->dwTblID);
	if ((it != s_mapLastHit.end ()) && (it->second.first != pChar->dwTblID) &&
		(dwNow - it->second.second < RENKEI_TIME)) {
		nRet = RENKEI_RATE;
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 160, 40), "連携!");
	}
	s_mapLastHit[pCharTarget->dwTblID] = std::make_pair (pChar->dwTblID, dwNow);
	return nRet;
}

/* ========================================================================= */
/* 関数名：	GetEnemyTurnTime												 */
/* 内容：	敵の行動の間隔(ms)。素早さ0で5秒〜255で1秒。加速で短くなる		 */
/* ========================================================================= */
static int GetEnemyTurnTime(PCHARINFO pChar)
{
	int nTmp;

	nTmp = pChar->byDEX;
	nTmp = nTmp * 100 / 255;
	nTmp = max (nTmp, 1);
	nTmp = 1000 + (4000 - ((4000 * nTmp) / 100));
	nTmp = nTmp * 100 / (100 + GetHasteRate (pChar));
	nTmp = max (nTmp, 1);

	return nTmp;
}

/* ========================================================================= */
/* 関数名：	IsEnemy															 */
/* 内容：	戦闘１・戦闘２の敵か											 */
/* ========================================================================= */
static BOOL IsEnemy(PCHARINFO pChar)
{
	return (pChar->byNpcType == NPCTYPE_BATTLE1) || (pChar->byNpcType == NPCTYPE_BATTLE2);
}

/* ========================================================================= */
/* 関数名：	GetDirectionTo													 */
/* 内容：	pChar から見て pCharTo のいる方の向き(同じ場所なら byDefault)	 */
/* ========================================================================= */
static BYTE GetDirectionTo(PCHARINFO pChar, PCHARINFO pCharTo, BYTE byDefault)
{
	int nDX, nDY;

	nDX = GetCharDistanceX (pChar, pCharTo);
	nDY = (int)pCharTo->dwY - (int)pChar->dwY;
	if ((nDX == 0) && (nDY == 0)) {
		return byDefault;
	}
	if (abs (nDX) > abs (nDY)) {
		return (nDX < 0) ? 2 : 3;
	}
	return (nDY < 0) ? 0 : 1;
}

/* ========================================================================= */
/* 関数名：	IsHealMagic														 */
/* 内容：	味方(自分側)のHPを増やす特技か									 */
/* ========================================================================= */
static BOOL IsHealMagic(PMAGICINFO pInfo)
{
	return (pInfo->byType == MAGICTYPE_HPUP) && (pInfo->byTarget != MAGICTARGET_NPC);
}

/* ========================================================================= */
/* 関数名：	IsNeedHeal														 */
/* 内容：	回復の特技を使う意味があるか									 */
/*			自分か、届く範囲の仲間(敵)のHPが ENEMYHEAL_RATE(%) より少ない時	 */
/* ========================================================================= */
static BOOL IsNeedHeal(PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i, nAreaX, nAreaY;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	if ((pChar->wMaxHP == 0) || ((int)pChar->wHP * 100 < (int)pChar->wMaxHP * ENEMYHEAL_RATE)) {
		return TRUE;
	}
	switch (pInfo->byTargetType) {
	case MAGICTARGETTYPE_CIRCLE:	nAreaX = nAreaY = pInfo->byTargetArea;	break;
	case MAGICTARGETTYPE_SCREEN:	nAreaX = DRAW_PARTS_X; nAreaY = DRAW_PARTS_Y;	break;
	default:
		return FALSE;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	pData = pMap->paTblNoNPC;
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[i]];
		if ((pCharTmp == pChar) || !IsEnemy (pCharTmp) || (pCharTmp->wHP == 0) || (pCharTmp->wMaxHP == 0) || !IsSameSpace (pChar, pCharTmp)) {
			continue;
		}
		if ((abs (GetCharDistanceX (pChar, pCharTmp)) > nAreaX) || (abs ((int)pCharTmp->dwY - (int)pChar->dwY) > nAreaY)) {
			continue;
		}
		if ((int)pCharTmp->wHP * 100 < (int)pCharTmp->wMaxHP * ENEMYHEAL_RATE) {
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：	CanReachMagic													 */
/* 内容：	目の前にいない相手に届く攻撃の特技か。届くなら使う時の向きを返す	 */
/*			・前方の特技／前へ飛ぶ魔法：まっすぐ並んでいて、届く距離で、	 */
/*			  間に通れないマスが無い										 */
/*			・周囲の特技：範囲の中にいる	・画面内の特技：画面の中にいる	 */
/* ========================================================================= */
static BOOL CanReachMagic(PCHARINFO pChar, PCHARINFO pCharTarget, PMAGICINFO pInfo, BYTE *pbyDirection)
{
	int i, nDX, nDY, nDist, nArea;
	BOOL bLine;

	/* 相手側に効く特技だけ */
	if (pInfo->byTarget == MAGICTARGET_PC) {
		return FALSE;
	}
	nDX = GetCharDistanceX (pChar, pCharTarget);
	nDY = (int)pCharTarget->dwY - (int)pChar->dwY;
	*pbyDirection = GetDirectionTo (pChar, pCharTarget, pChar->byDirection);

	bLine = FALSE;
	nArea = pInfo->byTargetArea;
	switch (pInfo->byTargetType) {
	case MAGICTARGETTYPE_FRONT:			/* 前方 */
		bLine = TRUE;
		break;
	case MAGICTARGETTYPE_SELF:			/* 自分の場所から前へ飛んでいく魔法 */
		if (((pInfo->byType == MAGICTYPE_PUT) || (pInfo->byType == MAGICTYPE_PUTSELF)) && pInfo->wData4) {
			bLine = TRUE;
		}
		break;
	case MAGICTARGETTYPE_CIRCLE:		/* 周囲 */
		return (abs (nDX) <= nArea) && (abs (nDY) <= nArea);
	case MAGICTARGETTYPE_SCREEN:		/* 画面内 */
		return (abs (nDX) <= DRAW_PARTS_X) && (abs (nDY) <= DRAW_PARTS_Y);
	}
	if (bLine == FALSE) {
		return FALSE;
	}
	if ((nDX != 0) && (nDY != 0)) {
		return FALSE;
	}
	nDist = abs (nDX) + abs (nDY);
	if ((nDist == 0) || (nDist > nArea)) {
		return FALSE;
	}
	/* 間に壁などがある？ */
	for (i = 1; i < nDist; i ++) {
		if (CheckCharBlock (pChar->dwMapID,
				(DWORD)((int)pChar->dwX + anPosAddTblX[*pbyDirection] * i),
				(DWORD)((int)pChar->dwY + anPosAddTblY[*pbyDirection] * i)) == FALSE) {
			return FALSE;
		}
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：	ChooseEnemyMagic												 */
/* 内容：	敵が使う特技の枠を選ぶ(使わずに普通に殴るなら -1)				 */
/*			・これまでどおり16枠からくじを引き、空き枠なら普通の攻撃		 */
/*			  （同じ特技を何枠にも入れると出やすい、という作りはそのまま）	 */
/*			・回復の特技は、自分や近くの仲間のHPが減っていなければ使わない	 */
/*			・自分のHPが少ない時は、ENEMYHEAL_PINCHRATE(%)で回復を優先する	 */
/*			・bRange の時は、離れた相手に届く攻撃の特技だけ					 */
/* ========================================================================= */
static int ChooseEnemyMagic(PCHARINFO pChar, PCHARINFO pCharTarget, BOOL bRange)
{
	int i, nNo, nCount, anHeal[MAX_USEMAGIC];
	BYTE byID, byDirection;
	PMAGICINFO pInfo;

	/* HPが少ない？ 使える回復があれば優先する */
	if ((pChar->wMaxHP > 0) && ((int)pChar->wHP * 100 <= (int)pChar->wMaxHP * ENEMYHEAL_PINCH) &&
		((int)(genrand () % 100) < ENEMYHEAL_PINCHRATE)) {
		nCount = 0;
		for (i = 0; i < MAX_USEMAGIC; i ++) {
			byID = pChar->abyMagic[i];
			if ((byID == 0) || (byID >= g_aMagicInfo.GetSize ())) {
				continue;
			}
			if (IsHealMagic (g_aMagicInfo[byID])) {
				anHeal[nCount ++] = i;
			}
		}
		if (nCount > 0) {
			return anHeal[genrand () % nCount];
		}
	}

	nNo = (int)(genrand () % MAX_USEMAGIC);
	byID = pChar->abyMagic[nNo];
	if ((byID == 0) || (byID >= g_aMagicInfo.GetSize ())) {
		return -1;
	}
	pInfo = g_aMagicInfo[byID];
	if (IsHealMagic (pInfo)) {
		/* 離れた所からは回復しない（回復は目の前の相手と戦っている時の番で使う） */
		if (bRange || (IsNeedHeal (pChar, pInfo) == FALSE)) {
			return -1;
		}
		return nNo;
	}
	if (bRange && (CanReachMagic (pChar, pCharTarget, pInfo, &byDirection) == FALSE)) {
		return -1;
	}
	return nNo;
}

/* ========================================================================= */
/* 関数名：	BattleProcRangeMagic											 */
/* 内容：	狙っている相手が目の前にいない時、届く特技があれば使う			 */
/*			くじは1回の番につき1回だけ引く(外れたら次の番まで待つ。		 */
/*			その間に相手が目の前に来たら普通に戦う)							 */
/* ========================================================================= */
static BOOL BattleProcRangeMagic(PCHARINFO pChar, PMAPINFO pMap)
{
	static std::map<DWORD, DWORD> s_mapTry;		/* キャラごとに、くじを引いた時の dwLastBattle */
	int nNo;
	BYTE byDirection;
	PCHARINFO pCharTarget;
	PMAGICINFO pInfo;

	if ((pChar->dwTargetTblID == -1) || (pChar->dwTargetTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return FALSE;
	}
	pCharTarget = g_aCharInfo[pChar->dwTargetTblID];
	if ((pCharTarget == NULL) || (pCharTarget->byNpcType != NPCTYPE_INIT) ||
		(pCharTarget->byGameState == GAMESTATE_LOGOFF) || (pCharTarget->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pCharTarget) ||
		(pCharTarget->wHP == 0) || (pCharTarget->byCharState == CHARSTATE_DEAD)) {
		return FALSE;
	}
	/* 特技を使えないマップ？ */
	if (pMap->byFlg & MAPFLG_MAGIC) {
		return FALSE;
	}
	/* この番ではもうくじを引いた？ */
	std::map<DWORD, DWORD>::iterator it = s_mapTry.find (pChar->dwTblID);
	if ((it != s_mapTry.end ()) && (it->second == pChar->dwLastBattle)) {
		return FALSE;
	}
	s_mapTry[pChar->dwTblID] = pChar->dwLastBattle;

	nNo = ChooseEnemyMagic (pChar, pCharTarget, TRUE);
	if (nNo < 0) {
		return FALSE;
	}
	pInfo = g_aMagicInfo[pChar->abyMagic[nNo]];
	if (CanReachMagic (pChar, pCharTarget, pInfo, &byDirection) == FALSE) {
		return FALSE;
	}
	/* 相手の方を向いてから使う */
	if (pChar->byDirection != byDirection) {
		pChar->byDirection = byDirection;
		pChar->byFlg |= CHARINFOFLG_DIRECTION;
	}
	BattleProcMagic (pChar, pCharTarget, (BYTE)nNo);
	pChar->dwLastBattle = timeGetTime ();
	if (pChar->dwLastBattle <= 1) {
		pChar->dwLastBattle = 2;
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
