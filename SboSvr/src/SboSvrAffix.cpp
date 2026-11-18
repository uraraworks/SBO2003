/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	SboSvrAffix.cpp												 */
/* 概要：		付与（接頭辞・接尾辞）の処理								 */
/* 作成日：		2026/11/18													 */
/* ========================================================================= */
/*
 * 派生アイテムの番号の引き方・名前・強さ・売値は common/ItemAffix.h。
 * ここはサーバーだけの処理（派生表への登録・ドロップのくじ・鑑定）。詳しくは docs/item-affix.md
 */
#include "include.h"
#include "Glbldata.h"
#include "SaveLoad/SaveLoad.h"
#include "SboSvrAffix.h"

/* ========================================================================= */
/* 関数名：AffixDeriveFindOrCreate											 */
/* 内容：{元・前・後ろ・未鑑定} が同じ派生アイテムの番号を探し、無ければ作る	 */
/*		 作ったら派生表を保存し、ログイン中の全員へその行を送る				 */
/*		 付与が何も無い組（前後とも 0）は元の番号を返す。表がいっぱいでも元の番号 */
/* ========================================================================= */
WORD AffixDeriveFindOrCreate(WORD wBase, WORD wPre, WORD wSuf, BYTE byUnknown)
{
	PITEMDERIVEINFO pDerive;
	int i, nCount;

	if ((wBase >= ITEMID_DERIVE_BASE) || (wBase >= (WORD)g_aItemInfo.GetSize ())) {
		return wBase;
	}
	if ((wPre == 0) && (wSuf == 0)) {
		return wBase;
	}
	byUnknown = byUnknown ? 1 : 0;

	nCount = g_aItemDerive.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pDerive = g_aItemDerive[i];
		if ((pDerive->wBase == wBase) && (pDerive->wPre == wPre) && (pDerive->wSuf == wSuf) && (pDerive->byUnknown == byUnknown)) {
			return (WORD)(ITEMID_DERIVE_BASE + i);
		}
	}
	if (nCount >= MAX_ITEMDERIVE) {
		return wBase;
	}
	pDerive = new ITEMDERIVEINFO;
	ZeroMemory (pDerive, sizeof (ITEMDERIVEINFO));
	pDerive->wBase		= wBase;
	pDerive->wPre		= wPre;
	pDerive->wSuf		= wSuf;
	pDerive->byUnknown	= byUnknown;
	g_aItemDerive.Add (pDerive);
	SaveItemDerive ();
	SendCmdITEMDERIVE (URARASOCK_SENDALL, (WORD)nCount, 1);
	return (WORD)(ITEMID_DERIVE_BASE + nCount);
}

/* ========================================================================= */
/* 関数名：AffixCanDrop														 */
/* 内容：付与が付く装備か（武器〜指輪の種別で、大事な物ではない）			 */
/* ========================================================================= */
BOOL AffixCanDrop(WORD wItem)
{
	PITEMINFO pItem;

	if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
		return FALSE;
	}
	pItem = g_aItemInfo[wItem];
	if (pItem->byFlg & ITEMFLG_IMPORTANT) {
		return FALSE;
	}
	return (ItemAffixEquBit (pItem->byType) != 0) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：AffixPick														 */
/* 内容：位置 byPos の付与を、種別が合い格が byMaxRank 以下の行から重み付きで選ぶ	 */
/*		 候補が無ければ 0													 */
/* ========================================================================= */
static WORD AffixPick(BYTE byPos, WORD wBit, BYTE byMaxRank)
{
	PITEMAFFIXINFO pAffix;
	DWORD dwTotal = 0, dwLot;
	int i, nCount = g_aAffixInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pAffix = g_aAffixInfo[i];
		if ((pAffix->byPos != byPos) || !(pAffix->wEquMask & wBit) || (pAffix->byRank > byMaxRank) || (pAffix->wWeight == 0)) {
			continue;
		}
		dwTotal += pAffix->wWeight;
	}
	if (dwTotal == 0) {
		return 0;
	}
	dwLot = (DWORD)genrand () % dwTotal;
	for (i = 0; i < nCount; i ++) {
		pAffix = g_aAffixInfo[i];
		if ((pAffix->byPos != byPos) || !(pAffix->wEquMask & wBit) || (pAffix->byRank > byMaxRank) || (pAffix->wWeight == 0)) {
			continue;
		}
		if (dwLot < pAffix->wWeight) {
			return pAffix->wID;
		}
		dwLot -= pAffix->wWeight;
	}
	return 0;
}

/* ========================================================================= */
/* 関数名：AffixRollDrop													 */
/* 内容：ドロップした装備に付与をくじ引きする								 */
/*		 byRate(%)で当たり判定 → 前・後ろをそれぞれ 70% で選ぶ（どちらも外れたら	 */
/*		 前か後ろのどちらか1つ）→ 候補から重み付きで選ぶ。					 */
/*		 付与表が 0 件なら乱数も使わず元の番号を返す。結果は未鑑定の派生番号	 */
/* ========================================================================= */
WORD AffixRollDrop(WORD wBase, BYTE byRate, BYTE byMaxRank)
{
	WORD wBit, wPre = 0, wSuf = 0;
	BOOL bPre, bSuf;

	if ((g_aAffixInfo.GetSize () == 0) || !AffixCanDrop (wBase) || (byRate == 0) || (byMaxRank == 0)) {
		return wBase;
	}
	if ((genrand () % 100) >= byRate) {
		return wBase;
	}
	wBit = ItemAffixEquBit (g_aItemInfo[wBase]->byType);
	bPre = ((genrand () % 100) < 70) ? TRUE : FALSE;
	bSuf = ((genrand () % 100) < 70) ? TRUE : FALSE;
	if (!bPre && !bSuf) {
		if (genrand () % 2) {
			bPre = TRUE;
		} else {
			bSuf = TRUE;
		}
	}
	if (bPre) {
		wPre = AffixPick (AFFIXPOS_FRONT, wBit, byMaxRank);
	}
	if (bSuf) {
		wSuf = AffixPick (AFFIXPOS_BACK, wBit, byMaxRank);
	}
	return AffixDeriveFindOrCreate (wBase, wPre, wSuf, 1);
}

/* ========================================================================= */
/* 関数名：AffixGetStTmp													 */
/* 内容：敵のステータステンプレート（発生元から出た敵は発生元のもの）。無ければ NULL */
/* ========================================================================= */
PSTATUSTEMPLATEINFO AffixGetStTmp(PCHARINFO pChar)
{
	DWORD dwStTmp;

	if (pChar == NULL) {
		return NULL;
	}
	dwStTmp = pChar->dwStTmplateID;
	if ((dwStTmp == (DWORD)-1) && (pChar->dwParentTblID != (DWORD)-1) &&
		(pChar->dwParentTblID < (DWORD)g_aCharInfo.GetSize ()) && g_aCharInfo[pChar->dwParentTblID]) {
		dwStTmp = g_aCharInfo[pChar->dwParentTblID]->dwStTmplateID;
	}
	if ((dwStTmp == (DWORD)-1) || (dwStTmp >= (DWORD)g_aStTmpInfo.GetSize ())) {
		return NULL;
	}
	return g_aStTmpInfo[dwStTmp];
}

/* ========================================================================= */
/* 関数名：AffixDropItem													 */
/* 内容：敵 pCharTarget が装備 wItem を落とす時の番号を決める				 */
/*		 付与が付けば未鑑定の派生番号、付かなければ（テンプレートが無い敵も）wItem */
/* ========================================================================= */
WORD AffixDropItem(PCHARINFO pCharTarget, WORD wItem)
{
	PSTATUSTEMPLATEINFO pTmp = AffixGetStTmp (pCharTarget);

	if (pTmp == NULL) {
		return wItem;
	}
	return AffixRollDrop (wItem, pTmp->byAffixRate, pTmp->byAffixRank);
}

/* ========================================================================= */
/* 関数名：AffixAppraise													 */
/* 内容：未鑑定の番号を鑑定済みの番号にする（未鑑定でなければそのまま）		 */
/* ========================================================================= */
WORD AffixAppraise(WORD wID)
{
	PITEMDERIVEINFO pDerive = ItemDeriveGet (wID);

	if ((pDerive == NULL) || !pDerive->byUnknown) {
		return wID;
	}
	return AffixDeriveFindOrCreate (pDerive->wBase, pDerive->wPre, pDerive->wSuf, 0);
}

/* ========================================================================= */
/* 関数名：AffixAppraiseCost												 */
/* 内容：鑑定の代金（元の値段の 1/10。最低 10 ゴールド）					 */
/* ========================================================================= */
DWORD AffixAppraiseCost(WORD wID)
{
	DWORD dwGold = g_aItemInfo[wID]->dwGold / 10;

	return (dwGold < 10) ? 10 : dwGold;
}

/* Copyright(C)URARA-works 2003-2026 */
