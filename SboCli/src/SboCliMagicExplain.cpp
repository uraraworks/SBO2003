/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboCliMagicExplain.cpp										 */
/* 内容：		特技（魔法）の説明文 実装ファイル							 */
/*				店で特技の書を買う時などに、効果が分かるように説明を出す。	 */
/*				管理者が説明文を書いていない特技は、効果種別や数値から		 */
/*				自動で説明文を作って出す。									 */
/* ========================================================================= */
#include "stdafx.h"
#include "Glbldata.h"
#include "SboCliMagicExplain.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define MAGICEXPLAIN_MAXDEPTH	(4)				/* 魔法発生をたどる最大の深さ */

/* 属性名（g_aStrAttrType と同じ並び。「無し」は出さない） */
static LPCSTR s_aszAttrName[] = {"", "火", "水", "風", "土", "光", "闇"};


/* ========================================================================= */
/* 関数名：	AppendStr														 */
/* 内容：	文字列を後ろに足す（入りきらない分は全角の途中で切らずに捨てる） */
/* ========================================================================= */
static void AppendStr(LPSTR pszDst, int nSize, LPCSTR pszSrc)
{
	int nPos, i, nChar;

	nPos = (int)strlen (pszDst);
	for (i = 0; pszSrc[i]; i += nChar) {
		nChar = (IsDBCSLeadByte ((BYTE)pszSrc[i]) && pszSrc[i + 1]) ? 2 : 1;
		if (nPos + nChar > nSize - 1) {
			break;
		}
		CopyMemory (&pszDst[nPos], &pszSrc[i], nChar);
		nPos += nChar;
	}
	pszDst[nPos] = 0;
}

/* ========================================================================= */
/* 関数名：	GetAttrStr														 */
/* 内容：	属性の表記を取得（属性が無ければ空）							 */
/* ========================================================================= */
static void GetAttrStr(PMAGICINFO pInfo, LPSTR pszDst)
{
	pszDst[0] = 0;
	if ((pInfo->byAttr > 0) && (pInfo->byAttr < sizeof (s_aszAttrName) / sizeof (s_aszAttrName[0]))) {
		wsprintf (pszDst, "（%s属性）", s_aszAttrName[pInfo->byAttr]);
	}
}

/* ========================================================================= */
/* 関数名：	GetRangeStr														 */
/* 内容：	最低値～最大値の表記を取得										 */
/* ========================================================================= */
static void GetRangeStr(WORD wMin, WORD wMax, LPSTR pszDst)
{
	WORD wTmp;

	if (wMin > wMax) {
		wTmp = wMin;
		wMin = wMax;
		wMax = wTmp;
	}
	if (wMin == wMax) {
		wsprintf (pszDst, "%d", wMin);
	} else {
		wsprintf (pszDst, "%d～%d", wMin, wMax);
	}
}

/* ========================================================================= */
/* 関数名：	GetTargetStr													 */
/* 内容：	効果の相手の表記を取得（敵/味方）								 */
/* ========================================================================= */
static LPCSTR GetTargetStr(PMAGICINFO pInfo)
{
	switch (pInfo->byTarget) {
	case MAGICTARGET_PC:	return "味方";
	case MAGICTARGET_NPC:	return "敵";
	}
	return "相手";
}

/* ========================================================================= */
/* 関数名：	MakeEffectStr													 */
/* 内容：	効果の部分の文を作る（魔法発生は発生させる魔法をたどる）		 */
/*			bInner は魔法発生の先（「当たると」の後ろに続く文）の時 TRUE	 */
/* ========================================================================= */
static void MakeEffectStr(PMAGICINFO pInfo, BOOL bInner, int nDepth, LPSTR pszDst, int nSize)
{
	char szRange[64], szAttr[64], szTmp[256];
	PMAGICINFO pInfoChild;

	pszDst[0] = 0;
	GetAttrStr (pInfo, szAttr);
	GetRangeStr (pInfo->wData1, pInfo->wData2, szRange);

	switch (pInfo->byType) {
	case MAGICTYPE_HPDOWN:			/* HP減少 */
	case MAGICTYPE_HPDOWNHPCOST:	/* HP減少(HP消費) */
		if (bInner) {
			wsprintf (szTmp, "%sの魔法ダメージ%s", szRange, szAttr);
		} else {
			wsprintf (szTmp, "%sに%sの魔法ダメージ%s", GetTargetStr (pInfo), szRange, szAttr);
		}
		break;

	case MAGICTYPE_HPDOWNAT:		/* HP減少(攻撃力) */
		if (bInner) {
			wsprintf (szTmp, "攻撃力＋%sのダメージ%s", szRange, szAttr);
		} else {
			wsprintf (szTmp, "%sに攻撃力＋%sのダメージ%s", GetTargetStr (pInfo), szRange, szAttr);
		}
		break;

	case MAGICTYPE_HPDOWNRATE:		/* HP減少(率) */
		if (bInner) {
			wsprintf (szTmp, "残りHPの%d％のダメージ%s", pInfo->wData1, szAttr);
		} else {
			wsprintf (szTmp, "%sに残りHPの%d％のダメージ%s", GetTargetStr (pInfo), pInfo->wData1, szAttr);
		}
		break;

	case MAGICTYPE_HPUP:			/* HP増加 */
		wsprintf (szTmp, "HPを%s回復", szRange);
		break;

	case MAGICTYPE_LIGHT:			/* 明かり */
		strcpy (szTmp, "暗い場所で周りを明るくする");
		break;

	case MAGICTYPE_RETURN:			/* 帰還 */
		strcpy (szTmp, "記録した町へ戻る");
		break;

	case MAGICTYPE_RESURRECTION:	/* 蘇生 */
		wsprintf (szTmp, "倒れた仲間を生き返らせる（最大HPの%d％で復活）", pInfo->wData1);
		break;

	case MAGICTYPE_PUT:				/* 魔法発生 */
	case MAGICTYPE_PUTSELF:			/* 魔法発生(自分) */
		szTmp[0] = 0;
		if ((nDepth >= MAGICEXPLAIN_MAXDEPTH) || (pInfo->wData1 >= g_aMagicInfo.GetSize ())) {
			strcpy (szTmp, "魔法を発生させる");
			break;
		}
		pInfoChild = g_aMagicInfo[pInfo->wData1];
		/* 魔法発生の中の魔法発生はそのまま中身をたどる */
		if (bInner || (pInfoChild->byType == MAGICTYPE_PUT) || (pInfoChild->byType == MAGICTYPE_PUTSELF)) {
			if (bInner == FALSE) {
				AppendStr (szTmp, sizeof (szTmp), (pInfo->wData4) ? "前へ飛ぶ魔法を放ち、当たると" : "魔法を発生させ、触れると");
			}
			MakeEffectStr (pInfoChild, TRUE, nDepth + 1, &szTmp[strlen (szTmp)], sizeof (szTmp) - (int)strlen (szTmp));
			break;
		}
		if (pInfo->wData4) {
			strcpy (szTmp, "前へ飛ぶ魔法を放ち、当たると");
		} else if (pInfo->wData2 >= 2) {
			wsprintf (szTmp, "%d秒間残る魔法を置き、触れると", pInfo->wData2);
		} else {
			strcpy (szTmp, "魔法を発生させ、触れると");
		}
		{
			char szChild[256];
			MakeEffectStr (pInfoChild, TRUE, nDepth + 1, szChild, sizeof (szChild));
			AppendStr (szTmp, sizeof (szTmp), szChild);
		}
		break;

	default:
		szTmp[0] = 0;
		break;
	}
	AppendStr (pszDst, nSize, szTmp);
}

/* ========================================================================= */
/* 関数名：	MakeMagicExplainAuto											 */
/* 内容：	数値から説明文を作る											 */
/*			例）「敵に50～60の魔法ダメージ（光属性）。				 */
/*			     詠唱1秒・消費Mana12・戦闘中・僧侶用」				 */
/* ========================================================================= */
void MakeMagicExplainAuto(PMAGICINFO pInfo, LPSTR pszDst, int nSize)
{
	char szTmp[256];
	BOOL bFirst;

	if ((pszDst == NULL) || (nSize <= 0)) {
		return;
	}
	pszDst[0] = 0;
	if (pInfo == NULL) {
		return;
	}

	/* 効果 */
	MakeEffectStr (pInfo, FALSE, 0, szTmp, sizeof (szTmp));
	if (szTmp[0]) {
		AppendStr (pszDst, nSize, szTmp);
		AppendStr (pszDst, nSize, "。\n");
	}

	/* 詠唱時間・消費・使える状態・使える職業 */
	bFirst = TRUE;
	if (pInfo->byTime > 0) {
		wsprintf (szTmp, "詠唱%d秒", pInfo->byTime);
		AppendStr (pszDst, nSize, szTmp);
		bFirst = FALSE;
	}
	if (pInfo->wMana > 0) {
		if (pInfo->byType == MAGICTYPE_HPDOWNHPCOST) {
			wsprintf (szTmp, "%s消費HP%d", bFirst ? "" : "・", pInfo->wMana);
		} else {
			wsprintf (szTmp, "%s消費Mana%d", bFirst ? "" : "・", pInfo->wMana);
		}
		AppendStr (pszDst, nSize, szTmp);
		bFirst = FALSE;
	}
	switch (pInfo->byTypeInfoID) {
	case MAGICUSETYPE_WALK:		/* 平常状態 */
		wsprintf (szTmp, "%s戦闘中以外", bFirst ? "" : "・");
		AppendStr (pszDst, nSize, szTmp);
		bFirst = FALSE;
		break;
	case MAGICUSETYPE_BATTLE:	/* 戦闘状態 */
		wsprintf (szTmp, "%s戦闘中", bFirst ? "" : "・");
		AppendStr (pszDst, nSize, szTmp);
		bFirst = FALSE;
		break;
	}
	if (pInfo->byUserInfo < g_aMagicUserInfo.GetSize ()) {
		PMAGICUSERINFO pUser = g_aMagicUserInfo[pInfo->byUserInfo];
		if (pUser && pUser->szName[0]) {
			wsprintf (szTmp, "%s%s", bFirst ? "" : "・", pUser->szName);
			AppendStr (pszDst, nSize, szTmp);
			bFirst = FALSE;
		}
	}

	/* 最後の改行は取る */
	{
		int nLen = (int)strlen (pszDst);
		while ((nLen > 0) && (pszDst[nLen - 1] == '\n')) {
			pszDst[-- nLen] = 0;
		}
	}
}

/* ========================================================================= */
/* 関数名：	GetMagicExplain													 */
/* 内容：	説明文を取得													 */
/* ========================================================================= */
void GetMagicExplain(PMAGICINFO pInfo, LPSTR pszDst, int nSize)
{
	if ((pszDst == NULL) || (nSize <= 0)) {
		return;
	}
	pszDst[0] = 0;
	if (pInfo == NULL) {
		return;
	}
	if (pInfo->szExplain[0]) {
		AppendStr (pszDst, nSize, pInfo->szExplain);
	} else {
		MakeMagicExplainAuto (pInfo, pszDst, nSize);
	}
}

/* ========================================================================= */
/* 関数名：	IsHangChar														 */
/* 内容：	行の頭に来ないよう、前の行にぶら下げる文字（。、）」）か判定	 */
/* ========================================================================= */
static BOOL IsHangChar(LPCSTR psz)
{
	static LPCSTR s_aszHang[] = {"。", "、", "）", "」", "！", "？"};
	int i;

	for (i = 0; i < sizeof (s_aszHang) / sizeof (s_aszHang[0]); i ++) {
		if ((psz[0] == s_aszHang[i][0]) && (psz[1] == s_aszHang[i][1])) {
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：	SplitMagicExplain												 */
/* 内容：	説明文を nLineBytes バイトごとに折り返して行に分ける			 */
/*			戻り値は行数。\r\n・\n で改行する								 */
/*			「。」などは行の頭に来ないよう、全角1文字分まではみ出してよい	 */
/*			（呼ぶ側はその分の余白を空けておく）							 */
/* ========================================================================= */
int SplitMagicExplain(LPCSTR pszSrc, int nLineBytes, char aszDst[][128], int nMaxLine)
{
	int i, nChar, nLine, nPos;

	nLine = 0;
	nPos = 0;
	if ((pszSrc == NULL) || (nMaxLine <= 0)) {
		return 0;
	}
	nLineBytes = min (nLineBytes, 125);
	aszDst[0][0] = 0;

	for (i = 0; pszSrc[i]; i += nChar) {
		nChar = (IsDBCSLeadByte ((BYTE)pszSrc[i]) && pszSrc[i + 1]) ? 2 : 1;

		if (pszSrc[i] == '\r') {
			continue;
		}
		/* 改行、または入りきらないので次の行へ */
		if ((pszSrc[i] == '\n') ||
			((nPos + nChar > nLineBytes) && !((nChar == 2) && IsHangChar (&pszSrc[i]) && (nPos + nChar <= nLineBytes + 2)))) {
			nLine ++;
			if (nLine >= nMaxLine) {
				return nLine;
			}
			nPos = 0;
			aszDst[nLine][0] = 0;
			if (pszSrc[i] == '\n') {
				continue;
			}
		}
		CopyMemory (&aszDst[nLine][nPos], &pszSrc[i], nChar);
		nPos += nChar;
		aszDst[nLine][nPos] = 0;
	}
	if (nPos > 0) {
		nLine ++;
	}
	return nLine;
}

/* Copyright(C)URARA-works 2026 */
