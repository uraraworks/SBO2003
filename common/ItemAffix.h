/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	ItemAffix.h													 */
/* 内容：		付与（接頭辞・接尾辞）と派生アイテムの引き方（サーバー・クライアント共通） */
/* 作成日：		2026/11/18													 */
/* ========================================================================= */
/*
 * アイテムの番号（WORD）は、0〜16383 が今までの普通のアイテム、
 * 16384（ITEMID_DERIVE_BASE）以降が派生表 g_aItemDerive の行番号 + 16384。
 * 派生表の行は {元のアイテム, 前の付与, 後ろの付与, 未鑑定} で、付与は g_aAffixInfo の wID で指す。
 *
 * g_aItemInfo は CItemInfoArray で、[] に派生番号が来たら元の番号に直して引く。
 * そのため「アイテムの種類・印・性別・職業・フラグ」の判定は、番号が派生でも元のアイテムのままでよい。
 * 範囲チェック（GetSize との比較）は派生番号を通さないので、ItemIDValid を使う。
 * 名前・強さ・売値だけは付与で変わるので ItemNameOf・ItemValueOf・ItemGoldOf を通す。
 */
#pragma once

extern CmyArray<PITEMAFFIXINFO, PITEMAFFIXINFO> g_aAffixInfo;			/* 付与表（行番号と wID は別。wID は 1 から） */
extern CmyArray<PITEMDERIVEINFO, PITEMDERIVEINFO> g_aItemDerive;		/* 派生表（行番号 + ITEMID_DERIVE_BASE が派生アイテムの番号） */

/* 番号が派生アイテムか */
inline BOOL ItemIsDerived(WORD wID)
{
	return (wID >= ITEMID_DERIVE_BASE) ? TRUE : FALSE;
}

/* 派生アイテムの行（派生でない・無い番号なら NULL） */
inline PITEMDERIVEINFO ItemDeriveGet(WORD wID)
{
	int nIndex;

	if (wID < ITEMID_DERIVE_BASE) {
		return NULL;
	}
	nIndex = (int)wID - ITEMID_DERIVE_BASE;
	if (nIndex >= g_aItemDerive.GetSize ()) {
		return NULL;
	}
	return g_aItemDerive[nIndex];
}

/* 元のアイテム番号（普通の番号はそのまま。無い派生番号は 0） */
inline WORD ItemBaseID(WORD wID)
{
	PITEMDERIVEINFO pDerive;

	if (wID < ITEMID_DERIVE_BASE) {
		return wID;
	}
	pDerive = ItemDeriveGet (wID);
	return pDerive ? pDerive->wBase : 0;
}

/* 付与の番号から付与情報を探す（0・無い番号なら NULL） */
inline PITEMAFFIXINFO ItemAffixFind(WORD wAffixID)
{
	int i, nCount;

	if (wAffixID == 0) {
		return NULL;
	}
	nCount = g_aAffixInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (g_aAffixInfo[i]->wID == wAffixID) {
			return g_aAffixInfo[i];
		}
	}
	return NULL;
}

/* g_aItemInfo：[] に派生番号が来たら元の番号で引く */
class CItemInfoArray : public CmyArray<PITEMINFO, PITEMINFO>
{
public:
	PITEMINFO &operator [](int nIndex) {
		if (nIndex >= ITEMID_DERIVE_BASE) {
			nIndex = ItemBaseID ((WORD)nIndex);
		}
		return CmyArray<PITEMINFO, PITEMINFO>::operator [] (nIndex);
	}
};
extern CItemInfoArray g_aItemInfo;										/* アイテム情報 */

/* アイテム番号として有効か（0 は「なし」なので含む。派生は元のアイテムが有効なら有効） */
inline BOOL ItemIDValid(WORD wID)
{
	if (wID < ITEMID_DERIVE_BASE) {
		return ((int)wID < g_aItemInfo.GetSize ()) ? TRUE : FALSE;
	}
	return (ItemDeriveGet (wID) && ((int)ItemBaseID (wID) < g_aItemInfo.GetSize ())) ? TRUE : FALSE;
}

/* 鑑定済みの派生アイテムか（付与の効果が出るもの） */
inline BOOL ItemIsAppraised(WORD wID)
{
	PITEMDERIVEINFO pDerive = ItemDeriveGet (wID);
	return (pDerive && !pDerive->byUnknown) ? TRUE : FALSE;
}

/* 未鑑定の派生アイテムか */
inline BOOL ItemIsUnknown(WORD wID)
{
	PITEMDERIVEINFO pDerive = ItemDeriveGet (wID);
	return (pDerive && pDerive->byUnknown) ? TRUE : FALSE;
}

/* アイテムの種別(ITEMTYPE_*)に当たる付与の装備種別ビット（付与が付かない種別は 0） */
inline WORD ItemAffixEquBit(BYTE byType)
{
	switch (byType) {
	case ITEMTYPE_WEAPON:		return AFFIXEQU_WEAPON;
	case ITEMTYPE_ARMOR:		return AFFIXEQU_ARMOR;
	case ITEMTYPE_SHILD:		return AFFIXEQU_SHILD;
	case ITEMTYPE_HELMET:		return AFFIXEQU_HELMET;
	case ITEMTYPE_EQU_HAND:		return AFFIXEQU_HAND;
	case ITEMTYPE_EQU_LEG:		return AFFIXEQU_LEG;
	case ITEMTYPE_EQU_ACC1:		return AFFIXEQU_NECK;
	case ITEMTYPE_EQU_ACC2:
	case ITEMTYPE_EQU_ACC3:		return AFFIXEQU_RING;
	}
	return 0;
}

/* 鑑定済みの wID に効く前・後ろの付与を取り出す（効く物が無ければ NULL。種別が合わない付与は効かない） */
inline void ItemAffixGet(WORD wID, PITEMAFFIXINFO *ppPre, PITEMAFFIXINFO *ppSuf)
{
	PITEMDERIVEINFO pDerive = ItemDeriveGet (wID);
	PITEMAFFIXINFO pPre = NULL, pSuf = NULL;

	if (pDerive && !pDerive->byUnknown && ((int)pDerive->wBase < g_aItemInfo.GetSize ())) {
		WORD wBit = ItemAffixEquBit (g_aItemInfo[pDerive->wBase]->byType);

		pPre = ItemAffixFind (pDerive->wPre);
		pSuf = ItemAffixFind (pDerive->wSuf);
		if (pPre && !(pPre->wEquMask & wBit)) {
			pPre = NULL;
		}
		if (pSuf && !(pSuf->wEquMask & wBit)) {
			pSuf = NULL;
		}
	}
	if (ppPre) {
		*ppPre = pPre;
	}
	if (ppSuf) {
		*ppSuf = pSuf;
	}
}

/* 主の値（武器は攻撃力・防具は守備力・魔防。つまり byData）。鑑定済みなら付与を反映して 0〜255 に収める。
   最終値 = byData + byData * 合計Rate / 100 + 合計Add（前と後ろを合算） */
inline int ItemValueOf(WORD wID)
{
	PITEMAFFIXINFO pPre, pSuf;
	int nBase = (int)g_aItemInfo[wID]->byData;
	int nRate = 0, nAdd = 0, nValue;

	ItemAffixGet (wID, &pPre, &pSuf);
	if (pPre) {
		nRate += pPre->chRate;
		nAdd += pPre->chAdd;
	}
	if (pSuf) {
		nRate += pSuf->chRate;
		nAdd += pSuf->chAdd;
	}
	nValue = nBase + nBase * nRate / 100 + nAdd;
	if (nValue < 0) {
		nValue = 0;
	}
	if (nValue > 255) {
		nValue = 255;
	}
	return nValue;
}

/* 付与による魔法防御の加算（鑑定済みの時だけ。無ければ 0） */
inline int ItemAffixDFMagic(WORD wID)
{
	PITEMAFFIXINFO pPre, pSuf;
	int nValue = 0;

	ItemAffixGet (wID, &pPre, &pSuf);
	if (pPre) {
		nValue += pPre->chDFMagic;
	}
	if (pSuf) {
		nValue += pSuf->chDFMagic;
	}
	return nValue;
}

/* 値段（鑑定済みの売値の基準 = 元の値段 × 前後の wGoldRate%。未鑑定・普通のアイテムは元の値段のまま） */
inline DWORD ItemGoldOf(WORD wID)
{
	PITEMAFFIXINFO pPre, pSuf;
	unsigned long long ull = (unsigned long long)g_aItemInfo[wID]->dwGold;

	ItemAffixGet (wID, &pPre, &pSuf);
	if (pPre) {
		ull = ull * pPre->wGoldRate / 100;
	}
	if (pSuf) {
		ull = ull * pSuf->wGoldRate / 100;
	}
	if (ull > 0xFFFFFFFFULL) {
		ull = 0xFFFFFFFFULL;
	}
	return (DWORD)ull;
}

/* 名前を作って out へ入れる。鑑定済み＝前の名＋元の名＋後ろの名（鋭い黒曜石の剣＋1）、未鑑定＝元の名＋「？」。
   付与の無い普通のアイテムは元の名のまま。戻り値は pszOut */
inline char *ItemNameCopy(WORD wID, char *pszOut, int nSize)
{
	PITEMDERIVEINFO pDerive = ItemDeriveGet (wID);
	PITEMAFFIXINFO pPre, pSuf;
	const char *pszBase = g_aItemInfo[wID]->szName;

	if (nSize <= 0) {
		return pszOut;
	}
	pszOut[0] = 0;
	if (pDerive && pDerive->byUnknown) {
		snprintf (pszOut, nSize, "%s？", pszBase);
		return pszOut;
	}
	ItemAffixGet (wID, &pPre, &pSuf);
	if (pDerive && !pPre && !pSuf) {
		/* 効く付与が無い（種別が合わない・表に無い）時も、前後の名前は付与の表にあれば付ける */
		pPre = ItemAffixFind (pDerive->wPre);
		pSuf = ItemAffixFind (pDerive->wSuf);
	}
	snprintf (pszOut, nSize, "%s%s%s", pPre ? pPre->szName : "", pszBase, pSuf ? pSuf->szName : "");
	return pszOut;
}

/* 名前（8 つまで同時に使える持ち回りの置き場。wsprintf の引数にそのまま渡せる） */
inline LPCSTR ItemNameOf(WORD wID)
{
	static char s_aszName[8][MAX_ITEMNAME + MAX_AFFIXNAME * 2 + 8];
	static int s_nNext = 0;
	char *psz = s_aszName[s_nNext];

	s_nNext = (s_nNext + 1) % 8;
	return ItemNameCopy (wID, psz, sizeof (s_aszName[0]));
}

/* Copyright(C)URARA-works 2003-2026 */
