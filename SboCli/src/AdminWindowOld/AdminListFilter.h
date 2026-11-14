/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		管理者ウィンドウの選択リスト用の絞り込み（部分一致）			 */
/* ファイル名：	AdminListFilter.h											 */
/* ========================================================================= */
#pragma once

#include <stddef.h>
#include <vector>

/* 半角カナ(U+FF66〜U+FF9D)に対応するひらがな */
static const unsigned short s_awHalfKana[] = {
	0x3092, 0x3041, 0x3043, 0x3045, 0x3047, 0x3049, 0x3083, 0x3085, 0x3087, 0x3063,	/* ｦｧｨｩｪｫｬｭｮｯ */
	0x30FC, 0x3042, 0x3044, 0x3046, 0x3048, 0x304A, 0x304B, 0x304D, 0x304F, 0x3051,	/* ｰｱｲｳｴｵｶｷｸｹ */
	0x3053, 0x3055, 0x3057, 0x3059, 0x305B, 0x305D, 0x305F, 0x3061, 0x3064, 0x3066,	/* ｺｻｼｽｾｿﾀﾁﾂﾃ */
	0x3068, 0x306A, 0x306B, 0x306C, 0x306D, 0x306E, 0x306F, 0x3072, 0x3075, 0x3078,	/* ﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍ */
	0x307B, 0x307E, 0x307F, 0x3080, 0x3081, 0x3082, 0x3084, 0x3086, 0x3088, 0x3089,	/* ﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗ */
	0x308A, 0x308B, 0x308C, 0x308D, 0x308F, 0x3093,									/* ﾘﾙﾚﾛﾜﾝ */
};

/* ========================================================================= */
/* 文字列(UTF-8)を比べやすい形の文字コード列にする							 */
/* ・カタカナ（半角も）はひらがなに（「ぽーしょん」で「ポーション」） */
/* ・全角英数字は半角に、英字は小文字に										 */
/* ========================================================================= */
static inline void AdminFilterNormalize(const char *psz, std::vector<unsigned int> &aOut)
{
	const unsigned char *p = (const unsigned char *)(psz ? psz : "");

	aOut.clear ();
	while (*p) {
		unsigned int c = *p;
		int nLen = 1;

		if ((c >= 0xF0) && p[1] && p[2] && p[3]) {
			c = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
			nLen = 4;
		} else if ((c >= 0xE0) && p[1] && p[2]) {
			c = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
			nLen = 3;
		} else if ((c >= 0xC0) && p[1]) {
			c = ((c & 0x1F) << 6) | (p[1] & 0x3F);
			nLen = 2;
		}
		p += nLen;

		if ((c >= 0xFF66) && (c <= 0xFF9D)) {
			c = s_awHalfKana[c - 0xFF66];	/* 半角カナ→ひらがな */
		} else if ((c == 0xFF9E) || (c == 0xFF9F)) {
			/* 半角の濁点・半濁点は前の字にくっつける */
			unsigned int cPrev = aOut.empty () ? 0 : aOut.back ();
			BOOL bHa = (cPrev >= 0x306F) && (cPrev <= 0x307B) && ((cPrev - 0x306F) % 3 == 0);	/* は〜ほ */
			if (c == 0xFF9E) {
				if (((cPrev >= 0x304B) && (cPrev <= 0x3061) && (cPrev & 1)) ||	/* か〜ち */
					(cPrev == 0x3064) || (cPrev == 0x3066) || (cPrev == 0x3068) || bHa) {
					aOut.back () = cPrev + 1;
					continue;
				}
				if (cPrev == 0x3046) {			/* う→ゔ */
					aOut.back () = 0x3094;
					continue;
				}
			} else if (bHa) {
				aOut.back () = cPrev + 2;
				continue;
			}
			c = (c == 0xFF9E) ? 0x309B : 0x309C;
		} else if ((c >= 0x30A1) && (c <= 0x30F6)) {
			c -= 0x60;						/* カタカナ→ひらがな */
		} else if ((c >= 0xFF01) && (c <= 0xFF5E)) {
			c -= 0xFEE0;					/* 全角英数記号→半角 */
		} else if (c == 0x3000) {
			c = ' ';						/* 全角スペース→半角 */
		}
		if ((c >= 'A') && (c <= 'Z')) {
			c += 'a' - 'A';
		}
		aOut.push_back (c);
	}
}

/* ========================================================================= */
/* 名前が検索語に部分一致するか												 */
/* 空白で区切ると「全部含む」。検索語が空なら何でも一致						 */
/* ========================================================================= */
static inline BOOL AdminFilterMatch(const char *pszName, const char *pszKey)
{
	std::vector<unsigned int> aName, aKey;

	AdminFilterNormalize (pszKey, aKey);
	AdminFilterNormalize (pszName, aName);

	size_t nPos = 0;
	while (nPos < aKey.size ()) {
		/* 区切りの空白を飛ばして1語を取り出す */
		while ((nPos < aKey.size ()) && (aKey[nPos] == ' ')) {
			nPos ++;
		}
		size_t nStart = nPos;
		while ((nPos < aKey.size ()) && (aKey[nPos] != ' ')) {
			nPos ++;
		}
		size_t nWord = nPos - nStart;
		if (nWord == 0) {
			break;
		}

		/* 名前の中にこの語があるか */
		BOOL bFound = FALSE;
		for (size_t i = 0; (i + nWord <= aName.size ()) && !bFound; i ++) {
			size_t j;
			for (j = 0; j < nWord; j ++) {
				if (aName[i + j] != aKey[nStart + j]) {
					break;
				}
			}
			bFound = (j == nWord);
		}
		if (!bFound) {
			return FALSE;
		}
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2026 */
