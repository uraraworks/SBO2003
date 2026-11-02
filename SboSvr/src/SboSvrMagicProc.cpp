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


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);			/* 魔法処理 */
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
};

/* ========================================================================= */
/* 関数名：MagicProc														 */
/* 内容：魔法処理															 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget)
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


	/*******************
		不正チェック
	*******************/

	for (i = 0; i < MAX_USEMAGIC; i ++) {
		if (pChar->abyMagic[i] == byID) {
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

	if (pChar->byNpcType == NPCTYPE_INIT) {
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

	if (pChar->byNpcType == NPCTYPE_INIT) {
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
	if ((pChar->byNpcType == NPCTYPE_USEMAGIC) || (pChar->byNpcType == NPCTYPE_USEMAGICPARENT)) {
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

Exit:
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
	if (MagicProcTbl[pInfo->byType] == NULL) {
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

	if (pMap->byLightLevel == 0) {
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
	DWORD dwX, dwY;
	BOOL bRet;

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

	pMap = g_aMapInfo[pChar->dwStartMapID];
	dwX = pChar->dwStartX;
	dwY = pChar->dwStartY;

	/* マップに入り口が設定されている？ */
	if (!((pMap->nEntranceX == 0) && (pMap->nEntranceY == 0))) {
		dwX = pMap->nEntranceX;
		dwY = pMap->nEntranceY;
	}

	/* マップ移動 */
	SendCmdCHGBGM (pCharTarget->pID, pCharTarget->dwMapID, 0);
	CharTargetCancelPC (pCharTarget);
	SendCmdFADEINOUT (pCharTarget->pID, 0);
	SubProcMapMove (pCharTarget, pChar->dwStartMapID, dwX, dwY);
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

/* Copyright(C)URARA-works 2003-2005 */
