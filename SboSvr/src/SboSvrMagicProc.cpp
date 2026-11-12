/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrMagicProc.cpp											 */
/* 概要：		魔法処理													 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/13													 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrCharInfoTimerProc.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef CmyArray <POSINFO, POSINFO>	*PCMYARRAYPOS;
typedef BOOL (*MAGICPROC)(PMAGICINFO, PCHARINFO, PCHARINFO);

/* 重なったキャラに効果を出している魔法キャラ（床など）。効果の処理の間だけ入る。
   町転送の床が覚えている行き先を帰還の処理で使う */
static PCHARINFO s_pCharMagicSrc = NULL;


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);			/* 魔法処理 */
BOOL MagicProcItem(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);		/* 魔法処理(アイテムから発動) */
BOOL IsTownWarpMagic(BYTE byID);										/* 町転送の特技か */
static BOOL IsTownWarpMagicInfo(PMAGICINFO pInfo);						/* 町転送の特技か(魔法情報) */
static void TownWarpGetDest(PCHARINFO pChar, BYTE bySel, PDWORD pdwMapID, PDWORD pdwX, PDWORD pdwY);	/* 選んだ行き先を取り出す */
int  TownWarpMakeList(PCHARINFO pChar, PDWORD padwMapID, PDWORD padwX, PDWORD padwY);	/* 町転送の行き先一覧を作る */
void TownWarpRecord(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY, int nForgetMapID);	/* 教会で復活位置を記録し、行き先帳に書く */
static BOOL MagicProcSub(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget, BOOL bItem);	/* 魔法処理(本体) */
static BYTE GetTarget						(PCHARINFO, BYTE);			/* キャラ種別の取得 */
static void MakeCharTblTarget_SELF			(PARRAYDWORD, PCHARINFO, PMAGICINFO);				/* ターゲットキャラの設定(自分) */
static BOOL MakeCharTblTargetSub_SELF		(PARRAYDWORD, int, PDWORD, PCHARINFO, PMAGICINFO);	/* ターゲットキャラの設定処理(自分) */
static void MakeCharTblTarget_FRONT			(PARRAYDWORD, PCHARINFO, PMAGICINFO);				/* ターゲットキャラの設定(前方) */
static BOOL MakeCharTblTargetSub_FRONT		(PARRAYDWORD, int, PDWORD, PCHARINFO, PMAGICINFO);	/* ターゲットキャラの設定処理(前方) */
static void MakeCharTblTarget_CIRCLE		(PARRAYDWORD, PCHARINFO, PMAGICINFO);				/* ターゲットキャラの設定(周囲) */
static BOOL MakeCharTblTargetSub_CIRCLE		(PARRAYDWORD, int, PDWORD, PCHARINFO, PMAGICINFO);	/* ターゲットキャラの設定処理(周囲) */
static void MakeCharTblTarget_SCREEN		(PARRAYDWORD, PCHARINFO, PMAGICINFO);				/* ターゲットキャラの設定(画面内) */
static BOOL MakeCharTblTargetSub_SCREEN		(PARRAYDWORD, int, PDWORD, PCHARINFO, PMAGICINFO);	/* ターゲットキャラの設定処理(画面内) */
static void MakeCharTblTarget_FRONTSIDE		(PARRAYDWORD, PCHARINFO, PMAGICINFO);				/* ターゲットキャラの設定(前方横一列) */
static BOOL MakeCharTblTargetSub_FRONTSIDE	(PARRAYDWORD, int, PDWORD, PCHARINFO, PMAGICINFO);	/* ターゲットキャラの設定処理(前方横一列) */
static void MakePosTbl						(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定 */
static void MakePosTbl_SELF					(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定(自分) */
static void MakePosTbl_FRONT				(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定(前方) */
static void MakePosTbl_CIRCLE				(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定(周囲) */
static void MakePosTbl_SCREEN				(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定(画面内) */
static void MakePosTbl_FRONTSIDE			(PCMYARRAYPOS, PCHARINFO, PMAGICINFO);				/* 座標情報の設定(前方横一列) */
static BOOL MagicProcUse					(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理) */
static BOOL MagicProcUse_HPDOWN				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:HP減少) */
static BOOL MagicProcUse_HPUP				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:HP増加) */
static BOOL MagicProcUse_LIGHT				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:明かり) */
static BOOL MagicProcUse_PUT				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:魔法発生) */
static BOOL MagicProcUse_RETURN				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:帰還) */
static BOOL MagicProcUse_HPDOWNAT			(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:HP減少(攻撃力)) */
static BOOL MagicProcUse_PUTSELF			(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:魔法発生(自分)) */
static BOOL MagicProcUse_RESURRECTION		(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:蘇生) */
static BOOL MagicProcUse_HPDOWNRATE			(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:HP減少(率)) */
static BOOL MagicProcUse_HASTE				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:加速) */
static BOOL MagicProcUse_POISON				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:毒) */
static BOOL MagicProcUse_SLEEP				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:眠り) */
static BOOL MagicProcUse_ATUP				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:攻撃力アップ) */
static BOOL MagicProcUse_DFUP				(PMAGICINFO, PCHARINFO, PCHARINFO);					/* 魔法処理(使用処理:守備力アップ) */

extern long genrand();													/* 乱数取得 */

/* 関数ジャンプテーブル */
static const MAGICPROC MagicProcTbl[] = {
	NULL,							/* 効果無し */
	MagicProcUse_HPDOWN,			/* HP減少 */
	MagicProcUse_HPUP,				/* HP増加 */
	MagicProcUse_LIGHT,				/* 明かり */
	MagicProcUse_PUT,				/* 魔法発生 */
	MagicProcUse_RETURN,			/* 帰還 */
	MagicProcUse_HPDOWNAT,			/* HP減少(攻撃力) */
	MagicProcUse_PUTSELF,			/* 魔法発生(自分) */
	MagicProcUse_RESURRECTION,		/* 蘇生 */
	MagicProcUse_HPDOWN,			/* HP減少(HP消費) */
	MagicProcUse_HPDOWNRATE,		/* HP減少(率) */
	MagicProcUse_HASTE,				/* 加速 */
	MagicProcUse_POISON,			/* 毒 */
	MagicProcUse_SLEEP,				/* 眠り */
	MagicProcUse_ATUP,				/* 攻撃力アップ */
	MagicProcUse_DFUP,				/* 守備力アップ */
};

/* ========================================================================= */
/* 関数名：MagicProc														 */
/* 内容：魔法処理															 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget)
{
	return MagicProcSub (dwTblID, byID, dwTblIDTarget, FALSE);
}

/* ========================================================================= */
/* 関数名：MagicProcItem													 */
/* 内容：魔法処理(アイテムから発動)。覚えていなくても使え、Manaも減らない	 */
/*		 戻り値：発動した(対象がいた)ら TRUE（アイテムを減らすかの判定用）	 */
/* ========================================================================= */
BOOL MagicProcItem(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget)
{
	return MagicProcSub (dwTblID, byID, dwTblIDTarget, TRUE);
}

/* ========================================================================= */
/* 関数名：MagicProcSub														 */
/* 内容：魔法処理(本体)。bItem はアイテムから発動した時 TRUE				 */
/* ========================================================================= */
static BOOL MagicProcSub(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget, BOOL bItem)
{
	PCHARINFO pChar;
	PMAGICINFO pInfo;
	PMAPINFO pMap;
	int i, nCount;
	BOOL bRet, bResult;
	ARRAYDWORD aCharTblTarget;

	bRet = FALSE;
	if (byID >= g_aMagicInfo.GetSize ()) {
		goto Exit;
	}

	pChar = g_aCharInfo[dwTblID];
	pInfo = g_aMagicInfo[byID];

	/* 眠っている間は使えない */
	if (IsSleep (pChar)) {
		goto Exit;
	}


	/*******************
		不正チェック
	*******************/

	for (i = 0; i < MAX_USEMAGIC; i ++) {
		if (bItem || (pChar->abyMagic[i] == byID)) {
			break;
		}
	}
	if (i >= MAX_USEMAGIC) {
		/* 初期位置に戻す */
		SendCmdFADEINOUT (pChar->pID, 0);
		SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);
		SubProcMapMove (pChar, pChar->dwStartMapID, pChar->dwStartX, pChar->dwStartY);

		/* 強制ログアウト */
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, pChar->dwTblID);
		goto Exit;
	}


	/*******************
		Manaチェック
	*******************/

	if ((pChar->byNpcType == NPCTYPE_INIT) && (bItem == FALSE)) {
		if (pInfo->byType == MAGICTYPE_HPDOWNHPCOST) {
			if (pChar->wHP <= pInfo->wMana) {
				goto Exit;
			}
		} else if (pChar->wMana < pInfo->wMana) {
			goto Exit;
		}
	}


	/*********************
		マップチェック
	*********************/

	pMap = g_aMapInfo[pChar->dwMapID];
	if (pMap->byFlg & MAPFLG_MAGIC) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NOUSE, pChar->pID, 0);
		goto Exit;
	}


	/*******************
		対象者の設定
	*******************/

	if ((pInfo->byType == MAGICTYPE_PUT) || (pInfo->byType == MAGICTYPE_PUTSELF) ||
		(pChar->byNpcType == NPCTYPE_USEMAGIC) || (pChar->byNpcType == NPCTYPE_USEMAGICPARENT)) {
		MakeCharTblTarget_SELF (&aCharTblTarget, pChar, pInfo);

	} else {
		switch (pInfo->byTargetType) {
		case MAGICTARGETTYPE_SELF:			/* 自分 */
			MakeCharTblTarget_SELF (&aCharTblTarget, pChar, pInfo);
			break;

		case MAGICTARGETTYPE_FRONT:			/* 前方 */
			MakeCharTblTarget_FRONT (&aCharTblTarget, pChar, pInfo);
			break;

		case MAGICTARGETTYPE_CIRCLE:		/* 周囲 */
			MakeCharTblTarget_CIRCLE (&aCharTblTarget, pChar, pInfo);
			break;

		case MAGICTARGETTYPE_SCREEN:		/* 画面内 */
			MakeCharTblTarget_SCREEN (&aCharTblTarget, pChar, pInfo);
			break;

		case MAGICTARGETTYPE_FRONTSIDE:		/* 前方横一列 */
			MakeCharTblTarget_FRONTSIDE (&aCharTblTarget, pChar, pInfo);
			break;

		case MAGICTARGETTYPE_SELECT:		/* 選択 */
		case MAGICTARGETTYPE_SELECT2:		/* 選択(自分以外) */
			aCharTblTarget.Add (dwTblIDTarget);
			break;
		}
	}


	/*********************
		使用者側の処理
	*********************/

	if ((pChar->byNpcType == NPCTYPE_INIT) && (bItem == FALSE)) {
		if (pInfo->byType == MAGICTYPE_HPDOWNHPCOST) {
			pChar->wHP -= pInfo->wMana;
		} else {
			pChar->wMana -= pInfo->wMana;
		}

		SendCmdCHARSTATUSHPMANA (pChar->dwTblID, pChar->dwTblID);
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_USESPECIALTY, pChar->pID, (DWORD)byID);

		AddStatus (pChar, STATUSTYPE_IT, 0);
		AddStatus (pChar, STATUSTYPE_MAXMANA, pInfo->wMana);
	}

	/* エフェクト発動 */
	if (pInfo->byUserEfectID) {
		if ((pChar->byNpcType != NPCTYPE_USEMAGIC) && (pChar->byNpcType != NPCTYPE_USEMAGICPARENT)) {
			pChar->byEfect = pInfo->byUserEfectID;
			SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
			pChar->byEfect = 0;
		}
	}

	/* 重なったキャラに魔法発動？ */
	s_pCharMagicSrc = NULL;
	if ((pChar->byNpcType == NPCTYPE_USEMAGIC) || (pChar->byNpcType == NPCTYPE_USEMAGICPARENT)) {
		s_pCharMagicSrc = pChar;
		if (pChar->dwParentTblID != -1) {
			pChar = g_aCharInfo[pChar->dwParentTblID];
		}
	}

	/*****************
		効果の処理
	*****************/

	nCount = aCharTblTarget.GetSize ();
	/* 効果の処理 */
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[aCharTblTarget[i]];

		if (pChar != pCharTmp) {
			SendCmdSYSMSG2 (pCharTmp->pID, SYSMSG_USESPECIALTY, pChar->pID, (DWORD)byID);
		}

		bResult = MagicProcUse (pInfo, pChar, pCharTmp);
		if (bResult) {
			bRet = TRUE;
		}
	}
	/* アイテムは、相手がいて発動できたら使ったことにする(毒・眠りが外れても減る) */
	if (bItem && (nCount > 0)) {
		bRet = TRUE;
	}

Exit:
	s_pCharMagicSrc = NULL;
	return bRet;
}

/* ========================================================================= */
/* 関数名：GetTarget														 */
/* 内容：キャラ種別の取得													 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
static BYTE GetTarget(PCHARINFO pChar, BYTE byTarget)
{
	BYTE byRet;
	PCHARINFO pCharTmp;

	byRet = byTarget;
	/* PC？ */
	if (pChar->byNpcType == NPCTYPE_INIT) {
		/* PCの場合は変換なし */
		goto Exit;
	}

	/* 親が設定されている？ */
	if ((pChar->dwParentTblID != -1) && (byTarget != MAGICTARGET_ALL)) {
		pCharTmp = g_aCharInfo[pChar->dwParentTblID];
		if (pCharTmp->byNpcType == NPCTYPE_INIT) {
			/* 親がPCの場合は変換なし */
			goto Exit;
		}
	}

	/* キャラ種別毎に変換 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		byRet = MAGICTARGET_NPC;
		break;

	case MAGICTARGET_NPC:	/* NPC */
		byRet = MAGICTARGET_PC;
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		byRet = byTarget;
		break;

	default:
		byRet = byTarget;
		break;
	}

Exit:
	return byRet;
}

/* ========================================================================= */
/* 関数名：MakeCharTblTarget_SELF											 */
/* 内容：ターゲットキャラの設定(自分)										 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static void MakeCharTblTarget_SELF(PARRAYDWORD aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	PMAPINFO pMap;
	BYTE byTarget;
	BOOL bRet;
	int nMaxPC, nMaxNPC;

	pMap = g_aMapInfo[pChar->dwMapID];
	nMaxPC = (int)pMap->dwCountTblNoPC;
	nMaxNPC = (int)pMap->dwCountTblNoNPC;

	/* 魔法発生？ */
	if ((pInfo->byType == MAGICTYPE_PUT) || (pInfo->byType == MAGICTYPE_PUTSELF)) {
		aTarget->Add (pChar->dwTblID);
		return;
	}
	/* 対象が一人？ */
	if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
		if ((pChar->byNpcType != NPCTYPE_USEMAGIC) && (pChar->byNpcType != NPCTYPE_USEMAGICPARENT)) {
			/* 一人で自分の時は自分のIDを加えて処理完了 */
			aTarget->Add (pChar->dwTblID);
			return;
		}
	}

	byTarget = GetTarget (pChar, pInfo->byTarget);

	/* チェックするキャラ種別で分岐 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		MakeCharTblTargetSub_SELF (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		break;

	case MAGICTARGET_NPC:	/* NPC */
		MakeCharTblTargetSub_SELF (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		bRet = MakeCharTblTargetSub_SELF (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		/* 対象が一人でヒットしなかった？ */
		if (bRet && (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE)) {
			MakeCharTblTargetSub_SELF (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeCharTblTargetSub_SELF										 */
/* 内容：ターゲットキャラの設定処理(自分)									 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static BOOL MakeCharTblTargetSub_SELF(PARRAYDWORD aTarget,
					int nMax, PDWORD pData, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i;
	BOOL bRet = TRUE;

	/* マップ内のキャラ分ループ */
	for (i = 0; i < nMax; i ++) {
		if (pData[i] == -1) {
			break;
		}
		int x, y;
		BOOL bCheck;
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPCは魔法の対象にしない */
		if (IsQuestNpc (pCharTmp)) {
			continue;
		}
		x = GetCharDistanceX (pChar, pCharTmp);
		y = abs (pChar->dwY - pCharTmp->dwY);

		bCheck = FALSE;
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:		/* 未設定 */
		case NPCTYPE_BATTLE1:	/* 戦闘１ */
		case NPCTYPE_BATTLE2:	/* 戦闘２ */
			bCheck = TRUE;
			break;
		}

		if (bCheck == FALSE) {
			continue;
		}

		/* 当たっている？ */
		if ((x == 0) && (y == 0)) {
			aTarget->Add (pCharTmp->dwTblID);
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：MakeCharTblTarget_FRONT											 */
/* 内容：ターゲットキャラの設定(前方)										 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static void MakeCharTblTarget_FRONT(PARRAYDWORD aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	PMAPINFO pMap;
	BOOL bRet;
	BYTE byTarget;
	int nMaxPC, nMaxNPC;

	pMap = g_aMapInfo[pChar->dwMapID];
	nMaxPC = (int)pMap->dwCountTblNoPC;
	nMaxNPC = (int)pMap->dwCountTblNoNPC;

	byTarget = GetTarget (pChar, pInfo->byTarget);

	/* チェックするキャラ種別で分岐 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		MakeCharTblTargetSub_FRONT (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		break;

	case MAGICTARGET_NPC:	/* NPC */
		MakeCharTblTargetSub_FRONT (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		bRet = MakeCharTblTargetSub_FRONT (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		/* 対象が一人でヒットしなかった？ */
		if (bRet && (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE)) {
			MakeCharTblTargetSub_FRONT (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeCharTblTargetSub_FRONT										 */
/* 内容：ターゲットキャラの設定処理(前方)									 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static BOOL MakeCharTblTargetSub_FRONT(PARRAYDWORD aTarget,
					int nMax, PDWORD pData, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i;
	BOOL bRet = TRUE;

	/* マップ内のキャラ分ループ */
	for (i = 0; i < nMax; i ++) {
		if (pData[i] == -1) {
			break;
		}
		int x, y;
		BOOL bCheck;
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPCは魔法の対象にしない */
		if (IsQuestNpc (pCharTmp)) {
			continue;
		}
		x = GetCharDistanceX (pChar, pCharTmp);
		y = (int)pCharTmp->dwY - (int)pChar->dwY;

		bCheck = FALSE;
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:		/* 未設定 */
		case NPCTYPE_BATTLE1:	/* 戦闘１ */
		case NPCTYPE_BATTLE2:	/* 戦闘２ */
			bCheck = TRUE;
			break;
		}

		if (bCheck == FALSE) {
			continue;
		}

		bCheck = FALSE;
		/* 向き別に前方辺り判定 */
		switch (pChar->byDirection) {
		case 0:		/* 上 */
			if (!(x == 0 && y < 0 && abs (y) <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;

		case 1:		/* 下 */
			if (!(x == 0 && y > 0 && y <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;

		case 2:		/* 左 */
			if (!(y == 0 && x < 0 && (abs (x) <= pInfo->byTargetArea))) {
				break;
			}
			bCheck = TRUE;
			break;

		case 3:		/* 右 */
			if (!(y == 0 && x > 0 && (x <= pInfo->byTargetArea))) {
				break;
			}
			bCheck = TRUE;
			break;
		}

		/* 当たっている？ */
		if (bCheck) {
			aTarget->Add (pCharTmp->dwTblID);
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				bRet = FALSE;
				break;
			}
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：MakeCharTblTarget_CIRCLE											 */
/* 内容：ターゲットキャラの設定(周囲)										 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static void MakeCharTblTarget_CIRCLE(PARRAYDWORD aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	PMAPINFO pMap;
	BOOL bRet;
	BYTE byTarget;
	int nMaxPC, nMaxNPC;

	pMap = g_aMapInfo[pChar->dwMapID];
	nMaxPC = (int)pMap->dwCountTblNoPC;
	nMaxNPC = (int)pMap->dwCountTblNoNPC;

	byTarget = GetTarget (pChar, pInfo->byTarget);

	/* チェックするキャラ種別で分岐 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		MakeCharTblTargetSub_CIRCLE (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		break;

	case MAGICTARGET_NPC:	/* NPC */
		MakeCharTblTargetSub_CIRCLE (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		bRet = MakeCharTblTargetSub_CIRCLE (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		/* 対象が一人でヒットしなかった時以外？ */
		if (!(bRet == FALSE && (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE))) {
			MakeCharTblTargetSub_CIRCLE (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeCharTblTargetSub_CIRCLE										 */
/* 内容：ターゲットキャラの設定処理(周囲)									 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static BOOL MakeCharTblTargetSub_CIRCLE(PARRAYDWORD aTarget,
					int nMax, PDWORD pData, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i;
	BOOL bRet = TRUE;

	/* マップ内のキャラ分ループ */
	for (i = 0; i < nMax; i ++) {
		if (pData[i] == -1) {
			break;
		}
		int x, y;
		BOOL bCheck;
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPCは魔法の対象にしない */
		if (IsQuestNpc (pCharTmp)) {
			continue;
		}
		x = abs (GetCharDistanceX (pChar, pCharTmp));
		y = abs (pChar->dwY - pCharTmp->dwY);

		bCheck = FALSE;
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:		/* 未設定 */
		case NPCTYPE_BATTLE1:	/* 戦闘１ */
		case NPCTYPE_BATTLE2:	/* 戦闘２ */
			bCheck = TRUE;
			break;
		}

		if (bCheck == FALSE) {
			continue;
		}

		if ((x <= pInfo->byTargetArea) && (y <= pInfo->byTargetArea)) {
			aTarget->Add (pCharTmp->dwTblID);
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				bRet = FALSE;
				break;
			}
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：MakeCharTblTarget_SCREEN											 */
/* 内容：ターゲットキャラの設定(画面内)										 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static void MakeCharTblTarget_SCREEN(PARRAYDWORD aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	PMAPINFO pMap;
	BOOL bRet;
	BYTE byTarget;
	int nMaxPC, nMaxNPC;

	pMap = g_aMapInfo[pChar->dwMapID];
	nMaxPC = (int)pMap->dwCountTblNoPC;
	nMaxNPC = (int)pMap->dwCountTblNoNPC;

	byTarget = GetTarget (pChar, pInfo->byTarget);

	/* チェックするキャラ種別で分岐 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		MakeCharTblTargetSub_SCREEN (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		break;

	case MAGICTARGET_NPC:	/* NPC */
		MakeCharTblTargetSub_SCREEN (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		bRet = MakeCharTblTargetSub_SCREEN (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		/* 対象が一人でヒットしなかった時以外？ */
		if (!(bRet == FALSE && (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE))) {
			MakeCharTblTargetSub_SCREEN (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeCharTblTargetSub_SCREEN										 */
/* 内容：ターゲットキャラの設定処理(画面内)									 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
static BOOL MakeCharTblTargetSub_SCREEN(PARRAYDWORD aTarget,
					int nMax, PDWORD pData, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i;
	BOOL bRet = TRUE;

	/* マップ内のキャラ分ループ */
	for (i = 0; i < nMax; i ++) {
		if (pData[i] == -1) {
			break;
		}
		int x, y;
		BOOL bCheck;
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPCは魔法の対象にしない */
		if (IsQuestNpc (pCharTmp)) {
			continue;
		}
		x = abs (GetCharDistanceX (pChar, pCharTmp));
		y = abs (pChar->dwY - pCharTmp->dwY);

		bCheck = FALSE;
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:		/* 未設定 */
		case NPCTYPE_BATTLE1:	/* 戦闘１ */
		case NPCTYPE_BATTLE2:	/* 戦闘２ */
			bCheck = TRUE;
			break;
		}

		if (bCheck == FALSE) {
			continue;
		}

		if ((x <= DRAW_PARTS_X) && (y <= DRAW_PARTS_Y)) {
			aTarget->Add (pCharTmp->dwTblID);
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				bRet = FALSE;
				break;
			}
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：MakeCharTblTarget_FRONTSIDE										 */
/* 内容：ターゲットキャラの設定(前方横一列)									 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakeCharTblTarget_FRONTSIDE(PARRAYDWORD aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	PMAPINFO pMap;
	BOOL bRet;
	BYTE byTarget;
	int nMaxPC, nMaxNPC;

	pMap = g_aMapInfo[pChar->dwMapID];
	nMaxPC = (int)pMap->dwCountTblNoPC;
	nMaxNPC = (int)pMap->dwCountTblNoNPC;

	byTarget = GetTarget (pChar, pInfo->byTarget);

	/* チェックするキャラ種別で分岐 */
	switch (byTarget) {
	case MAGICTARGET_PC:	/* PC */
		MakeCharTblTargetSub_FRONTSIDE (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		break;

	case MAGICTARGET_NPC:	/* NPC */
		MakeCharTblTargetSub_FRONTSIDE (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		break;

	case MAGICTARGET_ALL:	/* 両方 */
		bRet = MakeCharTblTargetSub_FRONTSIDE (aTarget, nMaxPC, pMap->paTblNoPC, pChar, pInfo);
		/* 対象が一人でヒットしなかった？ */
		if (bRet && (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE)) {
			MakeCharTblTargetSub_FRONTSIDE (aTarget, nMaxNPC, pMap->paTblNoNPC, pChar, pInfo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakeCharTblTargetSub_FRONTSIDE									 */
/* 内容：ターゲットキャラの設定処理(前方横一列)								 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static BOOL MakeCharTblTargetSub_FRONTSIDE(PARRAYDWORD aTarget,
					int nMax, PDWORD pData, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i;
	BOOL bRet = TRUE;

	/* マップ内のキャラ分ループ */
	for (i = 0; i < nMax; i ++) {
		if (pData[i] == -1) {
			break;
		}
		int x, y;
		BOOL bCheck;
		PCHARINFO pCharTmp;

		pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPCは魔法の対象にしない */
		if (IsQuestNpc (pCharTmp)) {
			continue;
		}
		x = GetCharDistanceX (pChar, pCharTmp);
		y = (int)pCharTmp->dwY - (int)pChar->dwY;

		bCheck = FALSE;
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:		/* 未設定 */
		case NPCTYPE_BATTLE1:	/* 戦闘１ */
		case NPCTYPE_BATTLE2:	/* 戦闘２ */
			bCheck = TRUE;
			break;
		}

		if (bCheck == FALSE) {
			continue;
		}

		bCheck = FALSE;
		/* 向き別に前方辺り判定 */
		switch (pChar->byDirection) {
		case 0:		/* 上 */
			if (!((y == -1) && abs (x) <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;

		case 1:		/* 下 */
			if (!((y == 1) && abs (x) <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;

		case 2:		/* 左 */
			if (!(x == -1 && abs (y) <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;

		case 3:		/* 右 */
			if (!(x == 1 && abs (y) <= pInfo->byTargetArea)) {
				break;
			}
			bCheck = TRUE;
			break;
		}

		/* 当たっている？ */
		if (bCheck) {
			aTarget->Add (pCharTmp->dwTblID);
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				bRet = FALSE;
				break;
			}
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：MakePosTbl														 */
/* 内容：座標情報の設定														 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	switch (pInfo->byTargetType) {
	case MAGICTARGETTYPE_SELF:			/* 自分 */
		MakePosTbl_SELF (aTarget, pChar, pInfo);
		break;

	case MAGICTARGETTYPE_FRONT:			/* 前方 */
		MakePosTbl_FRONT (aTarget, pChar, pInfo);
		break;

	case MAGICTARGETTYPE_CIRCLE:		/* 周囲 */
		MakePosTbl_CIRCLE (aTarget, pChar, pInfo);
		break;

	case MAGICTARGETTYPE_SCREEN:		/* 画面内 */
		MakePosTbl_SCREEN (aTarget, pChar, pInfo);
		break;

	case MAGICTARGETTYPE_FRONTSIDE:		/* 前方横一列 */
		MakePosTbl_FRONTSIDE (aTarget, pChar, pInfo);
		break;
	}
}

/* ========================================================================= */
/* 関数名：MakePosTbl_SELF													 */
/* 内容：座標情報の設定(自分)												 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl_SELF(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	POSINFO PosInfo;

	PosInfo.dwMapID = pChar->dwMapID;
	PosInfo.dwX = pChar->dwX;
	PosInfo.dwY = pChar->dwY;
	aTarget->Add (PosInfo);
}

/* ========================================================================= */
/* 関数名：MakePosTbl_FRONT													 */
/* 内容：座標情報の設定(前方)												 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl_FRONT(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i, nCount, nPosTmpX, nPosTmpY;
	BOOL bResult;
	PMAPINFO pMap;
	POSINFO PosInfo;

	pMap = g_aMapInfo[pChar->dwMapID];
	nPosTmpX = (int)GetCharXPos (pChar, pChar->dwX, pChar->byDirection);
	nPosTmpY = (int)pChar->dwY;
	nPosTmpX += anPosAddTblX[pChar->byDirection];
	nPosTmpY += anPosAddTblY[pChar->byDirection];

	nCount = pInfo->byTargetArea;
	for (i = 0; i < nCount - 1; i ++) {
		bResult = CheckCharBlock (pChar->dwMapID, (DWORD)nPosTmpX, (DWORD)nPosTmpY);
		if (bResult) {
			PosInfo.dwMapID = pChar->dwMapID;
			PosInfo.dwX = (DWORD)nPosTmpX;
			PosInfo.dwY = (DWORD)nPosTmpY;
			aTarget->Add (PosInfo);
		}

		nPosTmpX += anPosAddTblX[pChar->byDirection];
		nPosTmpY += anPosAddTblY[pChar->byDirection];
	}
}

/* ========================================================================= */
/* 関数名：MakePosTbl_CIRCLE												 */
/* 内容：座標情報の設定(周囲)												 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl_CIRCLE(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i, j, nCount;
	BOOL bResult;
	PMAPINFO pMap;
	POSINFO PosInfo;
	DWORD dwX;

	pMap = g_aMapInfo[pChar->dwMapID];
	dwX = GetCharXPos (pChar, pChar->dwX, pChar->byDirection);

	nCount = pInfo->byTargetArea;
	for (j = (int)pChar->dwY - nCount; j <= (int)pChar->dwY + nCount; j ++) {
		for (i = (int)dwX - nCount; i <= (int)dwX + nCount; i ++) {
			bResult = CheckCharBlock (pChar->dwMapID, (DWORD)i, (DWORD)j);
			if (bResult) {
				PosInfo.dwMapID = pChar->dwMapID;
				PosInfo.dwX = (DWORD)i;
				PosInfo.dwY = (DWORD)j;
				aTarget->Add (PosInfo);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：MakePosTbl_SCREEN												 */
/* 内容：座標情報の設定(画面内)												 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl_SCREEN(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int i, j, nCount;
	BOOL bResult;
	PMAPINFO pMap;
	POSINFO PosInfo;
	DWORD dwX;

	pMap = g_aMapInfo[pChar->dwMapID];
	dwX = GetCharXPos (pChar, pChar->dwX, pChar->byDirection);

	nCount = pInfo->byTargetArea;
	for (j = (int)pChar->dwY - DRAW_PARTS_Y; j <= (int)pChar->dwY + DRAW_PARTS_Y; j ++) {
		for (i = (int)dwX - DRAW_PARTS_X; i <= (int)dwX + DRAW_PARTS_X; i ++) {
			bResult = CheckCharBlock (pChar->dwMapID, (DWORD)i, (DWORD)j);
			if (bResult) {
				PosInfo.dwMapID = pChar->dwMapID;
				PosInfo.dwX = (DWORD)i;
				PosInfo.dwY = (DWORD)j;
				aTarget->Add (PosInfo);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：MakePosTbl_FRONTSIDE												 */
/* 内容：座標情報の設定(前方横一列)											 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
static void MakePosTbl_FRONTSIDE(PCMYARRAYPOS aTarget, PCHARINFO pChar, PMAGICINFO pInfo)
{
	int x, y, xMax, yMax, nPosTmpX, nPosTmpY;
	BOOL bResult;
	PMAPINFO pMap;
	POSINFO PosInfo;
	DWORD dwX;

	pMap = g_aMapInfo[pChar->dwMapID];
	dwX = GetCharXPos (pChar, pChar->dwX, pChar->byDirection);
	nPosTmpX = (int)dwX;
	nPosTmpY = (int)pChar->dwY;
	nPosTmpX += anPosAddTblX[pChar->byDirection];
	nPosTmpY += anPosAddTblY[pChar->byDirection];

	switch (pChar->byDirection) {
	case 0:
		x = (int)dwX - pInfo->byTargetArea;
		xMax = (int)dwX + pInfo->byTargetArea;
		y = yMax = (int)pChar->dwY - 1;
		break;

	case 1:
		x = (int)dwX - pInfo->byTargetArea;
		xMax = (int)dwX + pInfo->byTargetArea;
		y = yMax = (int)pChar->dwY + 1;
		break;

	case 2:
		x = xMax = (int)dwX - 1;
		y = (int)pChar->dwY - pInfo->byTargetArea;
		yMax = (int)pChar->dwY + pInfo->byTargetArea;
		break;

	case 3:
		x = xMax = (int)dwX + 1;
		y = (int)pChar->dwY - pInfo->byTargetArea;
		yMax = (int)pChar->dwY + pInfo->byTargetArea;
		break;
	}

	if (pChar->byDirection <= 1) {
		for (; y <= yMax; y ++) {
			for (; x <= xMax; x ++) {
				bResult = CheckCharBlock (pChar->dwMapID, (DWORD)x, (DWORD)y);
				if (bResult) {
					PosInfo.dwMapID = pChar->dwMapID;
					PosInfo.dwX = (DWORD)x;
					PosInfo.dwY = (DWORD)y;
					aTarget->Add (PosInfo);
				}
			}
		}
	} else {
		for (; x <= xMax; x ++) {
			for (; y <= yMax; y ++) {
				bResult = CheckCharBlock (pChar->dwMapID, (DWORD)x, (DWORD)y);
				if (bResult) {
					PosInfo.dwMapID = pChar->dwMapID;
					PosInfo.dwX = (DWORD)x;
					PosInfo.dwY = (DWORD)y;
					aTarget->Add (PosInfo);
				}
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：MagicProcUse														 */
/* 内容：魔法処理(使用処理)													 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
BOOL MagicProcUse(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	BOOL bRet;

	bRet = FALSE;
	if ((pInfo->byType >= sizeof (MagicProcTbl) / sizeof (MagicProcTbl[0])) || (MagicProcTbl[pInfo->byType] == NULL)) {
		goto Exit;
	}

	/* 効果の処理 */
	bRet = MagicProcTbl[pInfo->byType](pInfo, pChar, pCharTarget);

Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_HPDOWN												 */
/* 内容：魔法処理(使用処理:HP減少)											 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
BOOL MagicProcUse_HPDOWN(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	char szTmp[128];
	DWORD dwTargetBack;
	int nTmp, nTmp2, nTmp3;
	BOOL bRet, bResult;

	bRet = FALSE;

	if (pChar->pID == pCharTarget->pID) {
		/* 自分にHP減少効果は不可とする */
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* 基本ダメージ量 */
	nTmp = abs (pInfo->wData2 - pInfo->wData1 + 1);

	nTmp = max (1, nTmp);
	nTmp2 = (pInfo->wData1 + (genrand () % nTmp));

	nTmp3 = pCharTarget->wDFMagic;
	/* 防御力倍？ */
	bResult = CheckCharInfoTimer (pChar->dwTblID, CHARINFOTIMEREVENT_DIFENCE);
	if (bResult) {
		nTmp3 = pCharTarget->wDFMagic * 2;
	}

	/* 魔法防御値を反映 */
	nTmp2 -= (nTmp3 / 2);

	/* 属性を反映 */
	switch (pInfo->byAttr) {
	case ATTRTYPE_FIRE:			/* 火 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrFire / 100);
		break;
	case ATTRTYPE_WATER:		/* 水 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWater / 100);
		break;
	case ATTRTYPE_WIND:			/* 風 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWind / 100);
		break;
	case ATTRTYPE_EARTH:		/* 土 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrEarth / 100);
		break;
	case ATTRTYPE_LIGHT:		/* 光 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrLight / 100);
		break;
	case ATTRTYPE_DARK:			/* 闇 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrDark / 100);
		break;
	}

	nTmp2 = max (0, nTmp2);
	/* 連携ボーナス(別のプレイヤーが直前に当てていたら増える) */
	if (nTmp2 > 0) {
		nTmp2 = nTmp2 * RenkeiCheck (pChar, pCharTarget) / 100;
	}

	AddBattleDamage (pChar, pCharTarget, nTmp2);
	nTmp = pCharTarget->wHP;
	nTmp -= nTmp2;
	nTmp = max (nTmp, 0);
	pCharTarget->wHP = (WORD)nTmp;

	/* メッセージ送信 */
	if (nTmp2 <= 0) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NODAMAGE, 0, 0);
	} else {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
		SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
	}

	/* 文字エフェクト発動 */
	wsprintf (szTmp, "%d", nTmp2);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (250, 60, 4), szTmp);

	/* HPとManaを通知 */
	SendCmdCHARSTATUSHPMANA (pCharTarget->dwTblID, pCharTarget->dwTblID);

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	dwTargetBack = pChar->dwTargetTblID;
	pChar->dwTargetTblID = pCharTarget->dwTblID;

	/* ダメージ後の処理 */
	if (pCharTarget->byNpcType) {
		DamageAfterProcNPC (pChar, pCharTarget);
	} else {
		DamageAfterProcPC (pChar, pCharTarget);
		/* ダメージあり？ */
		if (nTmp > 0) {
			AddStatus (pCharTarget, STATUSTYPE_ST, 0);
			AddStatus (pCharTarget, STATUSTYPE_MAXHP, nTmp);
		}
	}

	pChar->dwTargetTblID = dwTargetBack;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_HPUP												 */
/* 内容：魔法処理(使用処理:HP増加)											 */
/* 日付：2004/07/13															 */
/* ========================================================================= */
BOOL MagicProcUse_HPUP(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	char szTmp[128];
	WORD wTmp;
	DWORD dwID, dwData1, dwData2;
	int nTmp;
	BOOL bRet;

	bRet = FALSE;

	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}
	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	/* すでに満たん？ */
	if (pCharTarget->wHP == pCharTarget->wMaxHP) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NOPROC, pCharTarget->pID, 0);
		if (pChar != pCharTarget) {
			SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_NOPROC, pCharTarget->pID, 0);
		}
		goto Exit;
	}

	nTmp = abs (pInfo->wData2 - pInfo->wData1 + 1);
	nTmp = max (1, nTmp);
	wTmp = (pInfo->wData1 + ((WORD)genrand () % nTmp));

	if (wTmp + pCharTarget->wHP > pCharTarget->wMaxHP) {
		wTmp = pCharTarget->wMaxHP - pCharTarget->wHP;
	}
	pCharTarget->wHP += wTmp;
	/* 満たんになった？ */
	if (pCharTarget->wHP == pCharTarget->wMaxHP) {
		dwID = SYSMSG_HPMAX;
		dwData1 = (DWORD)pCharTarget->pID;
		dwData2 = 0;
	} else {
		dwID = SYSMSG_HPADD;
		dwData1 = (DWORD)pCharTarget->pID;
		dwData2 = (DWORD)wTmp;
	}

	SendCmdSYSMSG2 (pChar->pID, dwID, dwData1, dwData2);
	if (pChar != pCharTarget) {
		SendCmdSYSMSG2 (pCharTarget->pID, dwID, dwData1, dwData2);
	}

	/* 文字エフェクト発動 */
	wsprintf (szTmp, "%d", wTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp);

	/* HPとManaを通知 */
	SendCmdCHARSTATUSHPMANA (pCharTarget->dwTblID, pCharTarget->dwTblID);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_LIGHT												 */
/* 内容：魔法処理(使用処理:明かり)											 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
BOOL MagicProcUse_LIGHT(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	PMAPINFO pMap;
	BOOL bRet;

	bRet = FALSE;

	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}
	pMap = g_aMapInfo[pChar->dwMapID];

	if ((pMap->byLightLevel == 0) || !IsMapDarkPos (pMap, pCharTarget->dwX, pCharTarget->dwY)) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NOPROC2, 0, 0);
		goto Exit;
	}

	pCharTarget->byLightLevel		= (BYTE)pInfo->wData1;	/* 明るさレベル */
	pCharTarget->byLightLevelBack	= (BYTE)pInfo->wData1;
	pCharTarget->wLightTime			= pInfo->wData2;		/* 維持時間 */

	if (pCharTarget->wLightTime == 0) {
		pCharTarget->dwTimeLight = 0;				/* 明かりつけた時間 */

	} else {
		pCharTarget->dwTimeLight = timeGetTime ();	/* 明かりつけた時間 */
	}

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	SendCmdCHARINFO3 (pCharTarget->dwTblID);
	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：PlayPutEfcSound													 */
/* 内容：置いた魔法キャラのエフェクトの効果音を周りに鳴らす					 */
/*       (置いたキャラはキャラ情報としてエフェクト付きで届くので、			 */
/*        エフェクト変更の時のようにはクライアントで音が鳴らない)			 */
/* ========================================================================= */
static void PlayPutEfcSound(PCHARINFO pCharNew)
{
	PEFCINFO pEfc;

	if ((pCharNew == NULL) || (pCharNew->byEfect == 0)) {
		return;
	}
	pEfc = g_EfcInfo.GetPtr (pCharNew->byEfect - 1);
	if ((pEfc == NULL) || (pEfc->bySound == 0)) {
		return;
	}
	SendCmdPLAYSOUNDinside (pCharNew->dwTblID, pEfc->bySound);
}

/* ========================================================================= */
/* 関数名：MagicProcUse_PUT													 */
/* 内容：魔法処理(使用処理:魔法発生)										 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
BOOL MagicProcUse_PUT(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int i, j, nCount;
	PCHARINFO pCharNew;
	BOOL bRet;

	bRet = FALSE;

	CmyArray<POSINFO, POSINFO> aTblTarget;
	MakePosTbl (&aTblTarget, pChar, pInfo);

	nCount = aTblTarget.GetSize ();
	for (i = 0; i < nCount; i ++) {
		POSINFO Pos;

		Pos = aTblTarget[i];
		pCharNew = new CHARINFO;
		ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));
		pCharNew->byNpcType		= NPCTYPE_USEMAGIC;
		SetInitCharData (pCharNew);

		pCharNew->byGameState	= GAMESTATE_NPC;
		pCharNew->dwXBack		= pCharNew->dwX = Pos.dwX;
		pCharNew->dwYBack		= pCharNew->dwY = Pos.dwY;
		pCharNew->dwMapID		= pCharNew->dwMapID = Pos.dwMapID;
		pCharNew->dwTimeDelete	= pInfo->wData2;
		pCharNew->dwTimeLogin	= timeGetTime ();
		pCharNew->byEfect		= pInfo->byTargetEfectID;
		pCharNew->dwParentTblID	= pChar->dwTblID;
		pCharNew->wMaxHP		= 0;
		for (j = 0; j < MAX_USEMAGIC; j ++) {
			pCharNew->abyMagic[j] = (BYTE)pInfo->wData1;
		}
		/* 町転送の床は、置いた時に選んでいた行き先を覚えておく（帰還の処理で使う） */
		if (IsTownWarpMagicInfo (pInfo) && (pChar->byNpcType == NPCTYPE_INIT)) {
			TownWarpGetDest (pChar, pChar->byTownWarpSel, &pCharNew->dwStartMapID, &pCharNew->dwStartX, &pCharNew->dwStartY);
			pCharNew->byTownWarpDest = 1;
		}
		/* 移動確率が設定されている？ */
		if (pInfo->wData4) {
			pCharNew->byDirection			= pChar->byDirection;
			pCharNew->byMoveProbabilityX	= (BYTE)pInfo->wData4;
			pCharNew->byMoveProbabilityY	= (BYTE)pInfo->wData4;
			pCharNew->wHP					= pInfo->byTargetArea;
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				pCharNew->wMaxHP = 1;
			}
		}

		pCharNew = AddChar (pCharNew);

		/* 属性が火？ */
		if (pInfo->byAttr == ATTRTYPE_FIRE) {
			pCharNew->byLightLevel = 5;
		}

		pCharNew->pID = pCharNew->dwTblID + 1;
		RenewATDF (pCharNew->dwTblID);
		SendCmdCHARINFO3 (pCharNew->dwTblID);
		/* 置いた時にエフェクトの効果音を鳴らす(何マスにも置く時は最初の1回だけ。
		   置いた魔法がさらに置く(広がる炎など)時は、鳴りっぱなしにならないよう鳴らさない) */
		if ((i == 0) && (s_pCharMagicSrc == NULL)) {
			PlayPutEfcSound (pCharNew);
		}
	}
	bRet = TRUE;

	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_RETURN												 */
/* 内容：魔法処理(使用処理:帰還)											 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
BOOL MagicProcUse_RETURN(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	PMAPINFO pMap;
	DWORD dwMapID, dwX, dwY;
	BOOL bRet;

	bRet = FALSE;
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}
	if (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		goto Exit;
	}

	pMap = g_aMapInfo[pChar->dwMapID];

	/* 禁止されているマップ？ */
	if (pMap->byFlg & MAPFLG_MOVE) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NOUSE, 0, 0);
		goto Exit;
	}

	pChar->dwLastDotMove = timeGetTime ();
	/* 2秒以上止まっている？ */
	if (abs (pCharTarget->dwLastDotMove - timeGetTime ()) >= 2000) {
		goto Exit;
	}

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	/* 行き先は使った人の復活位置。町転送の床なら置いた時に選んだ町 */
	dwMapID = pChar->dwStartMapID;
	dwX = pChar->dwStartX;
	dwY = pChar->dwStartY;
	if (s_pCharMagicSrc && s_pCharMagicSrc->byTownWarpDest) {
		dwMapID = s_pCharMagicSrc->dwStartMapID;
		dwX = s_pCharMagicSrc->dwStartX;
		dwY = s_pCharMagicSrc->dwStartY;
	}
	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		goto Exit;
	}
	pMap = g_aMapInfo[dwMapID];

	/* マップに入り口が設定されている？ */
	if (!((pMap->nEntranceX == 0) && (pMap->nEntranceY == 0))) {
		dwX = pMap->nEntranceX;
		dwY = pMap->nEntranceY;
	}

	/* マップ移動 */
	SendCmdCHGBGM (pCharTarget->pID, pCharTarget->dwMapID, 0);
	CharTargetCancelPC (pCharTarget);
	SendCmdFADEINOUT (pCharTarget->pID, 0);
	SubProcMapMove (pCharTarget, dwMapID, dwX, dwY);
	pCharTarget->dwLastDotMove = timeGetTime () - 2000;

	SendCmdCHARINFO3 (pCharTarget->dwTblID);
	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_HPDOWNAT											 */
/* 内容：魔法処理(使用処理:HP減少(攻撃力))									 */
/* 日付：2004/11/28															 */
/* ========================================================================= */
BOOL MagicProcUse_HPDOWNAT(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	char szTmp[128];
	DWORD dwTargetBack;
	int nTmp, nTmp2, nTmp3;
	BOOL bRet, bResult;

	bRet = FALSE;

	if (pChar->pID == pCharTarget->pID) {
		/* 自分にHP減少効果は不可とする */
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* 基本ダメージ量 */
	nTmp = abs (pInfo->wData2 - pInfo->wData1 + 1);

	nTmp = max (1, nTmp);
	nTmp2 = (pInfo->wData1 + (genrand () % nTmp));
	nTmp2 += pChar->wAT;

	nTmp3 = pCharTarget->wDF;
	/* 防御力倍？ */
	bResult = CheckCharInfoTimer (pChar->dwTblID, CHARINFOTIMEREVENT_DIFENCE);
	if (bResult) {
		nTmp3 = pCharTarget->wDF * 2;
	}

	/* 防御値を反映 */
	nTmp2 -= (nTmp3 / 2);

	/* 属性を反映 */
	switch (pInfo->byAttr) {
	case ATTRTYPE_FIRE:			/* 火 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrFire / 100);
		break;
	case ATTRTYPE_WATER:		/* 水 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWater / 100);
		break;
	case ATTRTYPE_WIND:			/* 風 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWind / 100);
		break;
	case ATTRTYPE_EARTH:		/* 土 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrEarth / 100);
		break;
	case ATTRTYPE_LIGHT:		/* 光 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrLight / 100);
		break;
	case ATTRTYPE_DARK:			/* 闇 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrDark / 100);
		break;
	}

	nTmp2 = max (0, nTmp2);
	/* 連携ボーナス(別のプレイヤーが直前に当てていたら増える) */
	if (nTmp2 > 0) {
		nTmp2 = nTmp2 * RenkeiCheck (pChar, pCharTarget) / 100;
	}

	AddBattleDamage (pChar, pCharTarget, nTmp2);
	nTmp = pCharTarget->wHP;
	nTmp -= nTmp2;
	nTmp = max (nTmp, 0);
	pCharTarget->wHP = (WORD)nTmp;

	/* メッセージ送信 */
	if (nTmp2 <= 0) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NODAMAGE, 0, 0);
	} else {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
		SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
	}

	/* 文字エフェクト発動 */
	wsprintf (szTmp, "%d", nTmp2);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (250, 60, 4), szTmp);

	/* HPとManaを通知 */
	SendCmdCHARSTATUSHPMANA (pCharTarget->dwTblID, pCharTarget->dwTblID);

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	dwTargetBack = pChar->dwTargetTblID;
	pChar->dwTargetTblID = pCharTarget->dwTblID;

	/* ダメージ後の処理 */
	if (pCharTarget->byNpcType) {
		DamageAfterProcNPC (pChar, pCharTarget);
	} else {
		DamageAfterProcPC (pChar, pCharTarget);
		/* ダメージあり？ */
		if (nTmp > 0) {
			AddStatus (pCharTarget, STATUSTYPE_ST, 0);
			AddStatus (pCharTarget, STATUSTYPE_MAXHP, nTmp);
		}
	}

	pChar->dwTargetTblID = dwTargetBack;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_PUTSELF												 */
/* 内容：魔法処理(使用処理:魔法発生(自分))									 */
/* 日付：2005/01/19															 */
/* ========================================================================= */
BOOL MagicProcUse_PUTSELF(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int i, j, nCount;
	PCHARINFO pCharNew;
	BOOL bRet;

	bRet = FALSE;

	CmyArray<POSINFO, POSINFO> aTblTarget;
	MakePosTbl (&aTblTarget, pChar, pInfo);

	nCount = aTblTarget.GetSize ();
	for (i = 0; i < nCount; i ++) {
		POSINFO Pos;

		Pos = aTblTarget[i];
		pCharNew = new CHARINFO;
		ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));
		pCharNew->byNpcType		= NPCTYPE_USEMAGICPARENT;
		SetInitCharData (pCharNew);

		pCharNew->byGameState	= GAMESTATE_NPC;
		pCharNew->dwXBack		= pCharNew->dwX = Pos.dwX;
		pCharNew->dwYBack		= pCharNew->dwY = Pos.dwY;
		pCharNew->dwMapID		= pCharNew->dwMapID = Pos.dwMapID;
		pCharNew->dwTimeDelete	= pInfo->wData2;
		pCharNew->dwTimeLogin	= timeGetTime ();
		pCharNew->byEfect		= pInfo->byTargetEfectID;
		pCharNew->dwParentTblID	= pChar->dwTblID;
		pCharNew->wMaxHP = 0;
		for (j = 0; j < MAX_USEMAGIC; j ++) {
			pCharNew->abyMagic[j] = (BYTE)pInfo->wData1;
		}
		/* 移動確率が設定されている？ */
		if (pInfo->wData4) {
			pCharNew->byDirection			= pChar->byDirection;
			pCharNew->byMoveProbabilityX	= (BYTE)pInfo->wData4;
			pCharNew->byMoveProbabilityY	= (BYTE)pInfo->wData4;
			pCharNew->wHP					= pInfo->byTargetArea;
			if (pInfo->byTargetCount == MAGICTARGETCOUNT_ONE) {
				pCharNew->wMaxHP = 1;
			}
		}

		pCharNew = AddChar (pCharNew);

		/* 属性が火？ */
		if (pInfo->byAttr == ATTRTYPE_FIRE) {
			pCharNew->byLightLevel = 5;
		}

		pCharNew->pID = pCharNew->dwTblID + 1;
		RenewATDF (pCharNew->dwTblID);
		SendCmdCHARINFO3 (pCharNew->dwTblID);
		/* 置いた時にエフェクトの効果音を鳴らす(何マスにも置く時は最初の1回だけ。
		   置いた魔法がさらに置く(広がる炎など)時は、鳴りっぱなしにならないよう鳴らさない) */
		if ((i == 0) && (s_pCharMagicSrc == NULL)) {
			PlayPutEfcSound (pCharNew);
		}
	}
	bRet = TRUE;

	return bRet;
}


/* ========================================================================= */
/* 関数名：	MagicProcUse_RESURRECTION										 */
/* 内容：	魔法処理(使用処理:蘇生)											 */
/* 日付：	2005/11/03														 */
/* ========================================================================= */

BOOL MagicProcUse_RESURRECTION(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	BOOL bRet;
	char szTmp[128];
	long lTmp;

	bRet = FALSE;

	if (pCharTarget->byCharState != CHARSTATE_DEAD) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NOPROC, pCharTarget->pID, 0);
		goto Exit;
	}

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	lTmp = pCharTarget->wMaxHP * pInfo->wData1 / 100;
	lTmp = max (lTmp, 1);
	if (pCharTarget->wHP + lTmp > pCharTarget->wMaxHP) {
		lTmp = pCharTarget->wMaxHP - pCharTarget->wHP;
	}
	pCharTarget->wHP += (WORD)lTmp;
	pCharTarget->byCharState = CHARSTATE_MOVE;

	SendCmdSYSMSG2 (pChar->pID, SYSMSG_RESURRECTION, pCharTarget->pID, 0);
	if (pChar != pCharTarget) {
		SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_RESURRECTION, pCharTarget->pID, 0);
	}

	/* 文字エフェクト発動 */
	wsprintf (szTmp, "%d", lTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp);

	/* HPとManaを通知 */
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTarget->dwTblID);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_HPDOWNRATE											 */
/* 内容：魔法処理(使用処理:HP減少(率))										 */
/* 日付：2007/02/21															 */
/* ========================================================================= */
BOOL MagicProcUse_HPDOWNRATE(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	char szTmp[128];
	DWORD dwTargetBack;
	int nTmp, nTmp2, nTmp3;
	BOOL bRet, bResult;

	bRet = FALSE;

	if (pChar->pID == pCharTarget->pID) {
		/* 自分にHP減少効果は不可とする */
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* 基本ダメージ量 */
	nTmp = (pCharTarget->wHP * 10 * pInfo->wData1 / 1000);

	nTmp2 = max (1, nTmp);

	nTmp3 = pCharTarget->wDFMagic;
	/* 防御力倍？ */
	bResult = CheckCharInfoTimer (pChar->dwTblID, CHARINFOTIMEREVENT_DIFENCE);
	if (bResult) {
		nTmp3 = pCharTarget->wDFMagic * 2;
	}

	/* 魔法防御値を反映 */
	nTmp2 -= (nTmp3 / 2);

	/* 属性を反映 */
	switch (pInfo->byAttr) {
	case ATTRTYPE_FIRE:			/* 火 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrFire / 100);
		break;
	case ATTRTYPE_WATER:		/* 水 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWater / 100);
		break;
	case ATTRTYPE_WIND:			/* 風 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrWind / 100);
		break;
	case ATTRTYPE_EARTH:		/* 土 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrEarth / 100);
		break;
	case ATTRTYPE_LIGHT:		/* 光 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrLight / 100);
		break;
	case ATTRTYPE_DARK:			/* 闇 */
		nTmp2 -= (nTmp2 * pCharTarget->chAttrDark / 100);
		break;
	}

	nTmp2 = max (0, nTmp2);
	/* 連携ボーナス(別のプレイヤーが直前に当てていたら増える) */
	if (nTmp2 > 0) {
		nTmp2 = nTmp2 * RenkeiCheck (pChar, pCharTarget) / 100;
	}

	AddBattleDamage (pChar, pCharTarget, nTmp2);
	nTmp = pCharTarget->wHP;
	nTmp -= nTmp2;
	nTmp = max (nTmp, 0);
	pCharTarget->wHP = (WORD)nTmp;

	/* メッセージ送信 */
	if (nTmp2 <= 0) {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_NODAMAGE, 0, 0);
	} else {
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
		SendCmdSYSMSG2 (pCharTarget->pID, SYSMSG_DAMAGE1, pCharTarget->pID, (DWORD)nTmp2);
	}

	/* 文字エフェクト発動 */
	wsprintf (szTmp, "%d", nTmp2);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (250, 60, 4), szTmp);

	/* HPとManaを通知 */
	SendCmdCHARSTATUSHPMANA (pCharTarget->dwTblID, pCharTarget->dwTblID);

	/* エフェクト発動 */
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	dwTargetBack = pChar->dwTargetTblID;
	pChar->dwTargetTblID = pCharTarget->dwTblID;

	/* ダメージ後の処理 */
	if (pCharTarget->byNpcType) {
		DamageAfterProcNPC (pChar, pCharTarget);
	} else {
		DamageAfterProcPC (pChar, pCharTarget);
		/* ダメージあり？ */
		if (nTmp > 0) {
			AddStatus (pCharTarget, STATUSTYPE_ST, 0);
			AddStatus (pCharTarget, STATUSTYPE_MAXHP, nTmp);
		}
	}

	pChar->dwTargetTblID = dwTargetBack;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_HASTE												 */
/* 内容：魔法処理(使用処理:加速)											 */
/*		 付加データ1:速くなる率(%) 2:秒数									 */
/*		 攻撃ゲージ(PC)・攻撃の間隔(敵)が短くなる							 */
/* ========================================================================= */
BOOL MagicProcUse_HASTE(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int nRate, nSec;

	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		return FALSE;
	}
	nRate = min (200, max (1, (int)pInfo->wData1));
	nSec  = max (1, (int)pInfo->wData2);

	SetCharInfoTimerStatus (pCharTarget, CHARINFOTIMEREVENT_HASTE, (BYTE)nRate, (DWORD)nSec * 1000);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 230, 80), "加速");
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}
	/* PCには攻撃ゲージを速くするよう知らせる */
	if (pCharTarget->byNpcType == NPCTYPE_INIT) {
		SendCmdHASTE (pCharTarget->dwTblID, (BYTE)nRate, (DWORD)nSec * 1000);
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：GetStatusResist													 */
/* 内容：毒・眠りの効きにくさ(%)。ステータステンプレートの値を見る			 */
/*		 発生元から出た敵はテンプレートを持たないので、発生元のものを見る	 */
/* ========================================================================= */
static int GetStatusResist(PCHARINFO pChar)
{
	DWORD dwStTmp;

	if (pChar->byNpcType == NPCTYPE_INIT) {
		return 0;
	}
	dwStTmp = pChar->dwStTmplateID;
	if ((dwStTmp == (DWORD)-1) && (pChar->dwParentTblID != (DWORD)-1) &&
		(pChar->dwParentTblID < (DWORD)g_aCharInfo.GetSize ())) {
		dwStTmp = g_aCharInfo[pChar->dwParentTblID]->dwStTmplateID;
	}
	if ((dwStTmp == (DWORD)-1) || (dwStTmp >= (DWORD)g_aStTmpInfo.GetSize ())) {
		return 0;
	}
	return min (100, (int)g_aStTmpInfo[dwStTmp]->byStatusResist);
}

/* ========================================================================= */
/* 関数名：MagicProcUseStatusCheck											 */
/* 内容：毒・眠りをかけられる相手か・成功したか								 */
/* ========================================================================= */
static BOOL MagicProcUseStatusCheck(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget, int nRate)
{
	if (pChar->dwTblID == pCharTarget->dwTblID) {
		/* 自分にはかけない */
		return FALSE;
	}
	if ((pCharTarget->byCharState == CHARSTATE_DEAD) || (pCharTarget->wHP == 0)) {
		return FALSE;
	}
	if (nRate <= 0) {
		nRate = 100;
	}
	/* 敵の効きにくさ(ステータステンプレート)の分だけ成功率を下げる。100 なら効かない */
	nRate = nRate * (100 - GetStatusResist (pCharTarget)) / 100;
	if ((int)(genrand () % 100) >= nRate) {
		SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 255, 255), "ﾐｽ");
		return FALSE;
	}
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_POISON												 */
/* 内容：魔法処理(使用処理:毒)												 */
/*		 付加データ1:2秒ごとのダメージ 2:秒数 3:成功率(%、0なら100)			 */
/*		 毒では倒れない(HPは1で止まる)										 */
/* ========================================================================= */
BOOL MagicProcUse_POISON(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	if (MagicProcUseStatusCheck (pInfo, pChar, pCharTarget, (int)pInfo->wData3) == FALSE) {
		return FALSE;
	}
	SetCharInfoTimerStatus (pCharTarget, CHARINFOTIMEREVENT_POISON,
			(BYTE)min (255, max (1, (int)pInfo->wData1)), (DWORD)max (1, (int)pInfo->wData2) * 1000);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (190, 80, 255), "毒");
	/* 敵はかけた相手を狙う */
	if (pCharTarget->byNpcType != NPCTYPE_INIT) {
		DWORD dwTargetBack = pChar->dwTargetTblID;
		pChar->dwTargetTblID = pCharTarget->dwTblID;
		DamageAfterProcNPC (pChar, pCharTarget);
		pChar->dwTargetTblID = dwTargetBack;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_SLEEP												 */
/* 内容：魔法処理(使用処理:眠り)											 */
/*		 付加データ1:秒数 2:成功率(%、0なら100)								 */
/*		 眠っている間は攻撃も特技もできない。殴られると起きる				 */
/* ========================================================================= */
BOOL MagicProcUse_SLEEP(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	if (MagicProcUseStatusCheck (pInfo, pChar, pCharTarget, (int)pInfo->wData2) == FALSE) {
		return FALSE;
	}
	SetCharInfoTimerStatus (pCharTarget, CHARINFOTIMEREVENT_SLEEP, 0, (DWORD)max (1, (int)pInfo->wData1) * 1000);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (160, 200, 255), "Zzz");
	return TRUE;
}

/* ========================================================================= */
/* 関数名：IsTownWarpMagicInfo												 */
/* 内容：町転送の特技か（床を置く魔法で、床の魔法が帰還）					 */
/* ========================================================================= */
static BOOL IsTownWarpMagicInfo(PMAGICINFO pInfo)
{
	PMAGICINFO pInfoChild;

	if ((pInfo == NULL) || (pInfo->byType != MAGICTYPE_PUT)) {
		return FALSE;
	}
	if (pInfo->wData1 >= (WORD)g_aMagicInfo.GetSize ()) {
		return FALSE;
	}
	pInfoChild = g_aMagicInfo[pInfo->wData1];
	if ((pInfoChild == NULL) || (pInfoChild->byType != MAGICTYPE_RETURN)) {
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：IsTownWarpMagic													 */
/* 内容：町転送の特技か														 */
/* ========================================================================= */
BOOL IsTownWarpMagic(BYTE byID)
{
	if (byID >= g_aMagicInfo.GetSize ()) {
		return FALSE;
	}
	return IsTownWarpMagicInfo (g_aMagicInfo[byID]);
}

/* ========================================================================= */
/* 関数名：TownWarpMakeList													 */
/* 内容：町転送の行き先一覧を作る（戻り値は数。最大 MAX_TOWNWARP + 1）		 */
/*		 0 番目は今の復活位置。1 番目からは行き先帳の町を新しく記録した順に	 */
/*		 （復活位置と同じ町は除く）。選べるのは先頭から TOWNWARP_SLOT 個	 */
/*		 クライアントへ送る一覧（SendCmdTOWNWARP）もこれで作る				 */
/* ========================================================================= */
int TownWarpMakeList(PCHARINFO pChar, PDWORD padwMapID, PDWORD padwX, PDWORD padwY)
{
	int i, nCount;

	nCount = 0;
	padwMapID[nCount]	= pChar->dwStartMapID;
	padwX[nCount]		= pChar->dwStartX;
	padwY[nCount]		= pChar->dwStartY;
	nCount ++;

	for (i = 0; (i < pChar->byTownWarpCount) && (i < MAX_TOWNWARP); i ++) {
		if (pChar->adwTownWarpMapID[i] == pChar->dwStartMapID) {
			continue;
		}
		if (pChar->adwTownWarpMapID[i] >= (DWORD)g_aMapInfo.GetSize ()) {
			continue;
		}
		padwMapID[nCount]	= pChar->adwTownWarpMapID[i];
		padwX[nCount]		= pChar->abyTownWarpX[i];
		padwY[nCount]		= pChar->abyTownWarpY[i];
		nCount ++;
		if (nCount >= MAX_TOWNWARP + 1) {
			break;
		}
	}
	return nCount;
}

/* ========================================================================= */
/* 関数名：TownWarpGetDest													 */
/* 内容：町転送で選んだ行き先を取り出す（選べない番号なら復活位置）			 */
/* ========================================================================= */
static void TownWarpGetDest(PCHARINFO pChar, BYTE bySel, PDWORD pdwMapID, PDWORD pdwX, PDWORD pdwY)
{
	DWORD adwMapID[MAX_TOWNWARP + 1], adwX[MAX_TOWNWARP + 1], adwY[MAX_TOWNWARP + 1];
	int nCount, nSlot;

	nCount = TownWarpMakeList (pChar, adwMapID, adwX, adwY);
	nSlot = TOWNWARP_SLOT (pChar->byLevel);
	if ((bySel >= nCount) || (bySel >= nSlot)) {
		bySel = 0;
	}
	*pdwMapID	= adwMapID[bySel];
	*pdwX		= adwX[bySel];
	*pdwY		= adwY[bySel];
}

/* ========================================================================= */
/* 関数名：TownWarpRecord													 */
/* 内容：教会で復活位置を記録する。行き先帳のいちばん上にその町を書き、		 */
/*		 選べる数（TOWNWARP_SLOT）がいっぱいなら nForgetMapID の町を忘れる	 */
/*		 （-1 や一覧に無い町なら一番古い町）。すでに入っている町なら上へ移す	 */
/*		 職業に関係なくキャラに 1 冊												 */
/* ========================================================================= */
void TownWarpRecord(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY, int nForgetMapID)
{
	DWORD adwMapID[MAX_TOWNWARP + 1], adwX[MAX_TOWNWARP + 1], adwY[MAX_TOWNWARP + 1];
	int i, nCount, nSlot, nNew;
	BOOL bFound;

	/* 記録する前の一覧（選べる分だけ） */
	nCount = TownWarpMakeList (pChar, adwMapID, adwX, adwY);
	nSlot = TOWNWARP_SLOT (pChar->byLevel);
	if (nCount > nSlot) {
		nCount = nSlot;
	}
	bFound = FALSE;
	for (i = 0; i < nCount; i ++) {
		if (adwMapID[i] == dwMapID) {
			bFound = TRUE;
		}
	}
	/* 新しい町でいっぱいなら、忘れる町を決める */
	if (!bFound && (nCount >= nSlot)) {
		BOOL bOK = FALSE;
		for (i = 0; i < nCount; i ++) {
			if ((nForgetMapID >= 0) && (adwMapID[i] == (DWORD)nForgetMapID)) {
				bOK = TRUE;
			}
		}
		if (!bOK) {
			nForgetMapID = (int)adwMapID[nCount - 1];
		}
	} else {
		nForgetMapID = -1;
	}

	/* いちばん上に記録する町、続けて残す町 */
	pChar->dwStartMapID = dwMapID;
	pChar->dwStartX = dwX;
	pChar->dwStartY = dwY;
	pChar->adwTownWarpMapID[0]	= dwMapID;
	pChar->abyTownWarpX[0]		= (BYTE)dwX;
	pChar->abyTownWarpY[0]		= (BYTE)dwY;
	nNew = 1;
	for (i = 0; (i < nCount) && (nNew < MAX_TOWNWARP); i ++) {
		if ((adwMapID[i] == dwMapID) || ((nForgetMapID >= 0) && (adwMapID[i] == (DWORD)nForgetMapID))) {
			continue;
		}
		pChar->adwTownWarpMapID[nNew]	= adwMapID[i];
		pChar->abyTownWarpX[nNew]		= (BYTE)adwX[i];
		pChar->abyTownWarpY[nNew]		= (BYTE)adwY[i];
		nNew ++;
	}
	pChar->byTownWarpCount = (BYTE)nNew;
}

/* Copyright(C)URARA-works 2003-2005 */

/* ========================================================================= */
/* 関数名：MagicProcUseStatusUp												 */
/* 内容：攻撃力・守備力アップをかける(プレイヤーだけ)						 */
/*		 付加データ1:上がる率(%) 2:秒数 3:最低でも上がる量					 */
/*		 同じ種類はかけ直し(時間が延びるだけで重ならない)					 */
/* ========================================================================= */
static BOOL MagicProcUseStatusUp(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget,
		BYTE byType, LPCSTR pszName, COLORREF clText)
{
	char szTmp[256];
	int nRate, nSec, nMin;

	/* 敵や傭兵の攻撃力・守備力は作りが違うのでプレイヤーだけ */
	if (pCharTarget->byNpcType != NPCTYPE_INIT) {
		return FALSE;
	}
	if ((pCharTarget->byCharState == CHARSTATE_DEAD) || (pCharTarget->wHP == 0)) {
		return FALSE;
	}
	nRate = min (255, max (0, (int)pInfo->wData1));
	nSec  = max (1, (int)pInfo->wData2);
	nMin  = min (255, max (0, (int)pInfo->wData3));

	SetCharInfoTimerStatus (pCharTarget, byType, (BYTE)nRate, (DWORD)nSec * 1000);
	SetCharInfoTimerStatusMin (pCharTarget, byType, (DWORD)nMin);
	RenewATDF (pCharTarget->dwTblID);

	snprintf (szTmp, sizeof (szTmp), "%s↑", pszName);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, clText, szTmp);
	if (pInfo->byTargetEfectID) {
		pCharTarget->byEfect = pInfo->byTargetEfectID;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}
	/* かけてもらった人のログに続く時間を出す(切れた時もログに出る) */
	if (nSec % 60 == 0) {
		snprintf (szTmp, sizeof (szTmp), "%sアップがかかりました（%d分）", pszName, nSec / 60);
	} else {
		snprintf (szTmp, sizeof (szTmp), "%sアップがかかりました（%d秒）", pszName, nSec);
	}
	SendCmdSYSMSG (pCharTarget->pID, szTmp, SPEECHCMD_BATTLE1);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MagicProcUse_ATUP / MagicProcUse_DFUP							 */
/* 内容：魔法処理(使用処理:攻撃力アップ・守備力アップ)						 */
/* ========================================================================= */
BOOL MagicProcUse_ATUP(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	return MagicProcUseStatusUp (pInfo, pChar, pCharTarget, CHARINFOTIMEREVENT_ATUP, "攻撃", RGB (255, 120, 80));
}

BOOL MagicProcUse_DFUP(PMAGICINFO pInfo, PCHARINFO pChar, PCHARINFO pCharTarget)
{
	return MagicProcUseStatusUp (pInfo, pChar, pCharTarget, CHARINFOTIMEREVENT_DFUP, "守り", RGB (100, 180, 255));
}
