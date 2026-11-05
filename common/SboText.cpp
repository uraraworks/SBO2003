/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboText.cpp													 */
/* 内容：		文字列の文字単位の扱い（UTF-8 版）							 */
/* ========================================================================= */

#include "StdAfx.h"
#include <string>
#include "SboText.h"

/* platform/w32_sjis.cpp（古いデータ用の Shift_JIS 変換） */
std::string W32SjisToUtf8Str(const char *pszSjis, int nLen);
std::string W32Utf8ToSjisStr(const char *pszUtf8);

/* ------------------------------------------------------------------------- */
/* UTF-8 の 1 文字のバイト数を先頭バイトから求める（正しくない並びは 0）		 */
/* ------------------------------------------------------------------------- */
static int Utf8SeqLen(const BYTE *p)
{
	int i, nLen;

	if (p[0] < 0x80) {
		return 1;
	} else if ((p[0] >= 0xC2) && (p[0] <= 0xDF)) {
		nLen = 2;
	} else if ((p[0] >= 0xE0) && (p[0] <= 0xEF)) {
		nLen = 3;
	} else if ((p[0] >= 0xF0) && (p[0] <= 0xF4)) {
		nLen = 4;
	} else {
		return 0;
	}
	/* 続きのバイトは 0x80〜0xBF（終端が来たら正しくない） */
	for (i = 1; i < nLen; i ++) {
		if ((p[i] & 0xC0) != 0x80) {
			return 0;
		}
	}
	/* 短く書ける文字を長く書いたもの・サロゲート・範囲外は正しくない */
	if ((p[0] == 0xE0) && (p[1] < 0xA0)) {
		return 0;
	}
	if ((p[0] == 0xED) && (p[1] >= 0xA0)) {
		return 0;
	}
	if ((p[0] == 0xF0) && (p[1] < 0x90)) {
		return 0;
	}
	if ((p[0] == 0xF4) && (p[1] >= 0x90)) {
		return 0;
	}
	return nLen;
}

/* ------------------------------------------------------------------------- */
/* 正しい UTF-8 の 1 文字（nLen バイト）からコードポイントを取り出す			 */
/* ------------------------------------------------------------------------- */
static DWORD Utf8Decode(const BYTE *p, int nLen)
{
	switch (nLen) {
	case 1:	return p[0];
	case 2:	return ((DWORD)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
	case 3:	return ((DWORD)(p[0] & 0x0F) << 12) | ((DWORD)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
	case 4:	return ((DWORD)(p[0] & 0x07) << 18) | ((DWORD)(p[1] & 0x3F) << 12) | ((DWORD)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
	}
	return 0;
}

/* ========================================================================= */
/* 関数名：	SboTextCharLen													 */
/* 内容：	先頭 1 文字のバイト数（終端なら 0）								 */
/*			UTF-8 として正しくないバイトは 1 バイトで 1 文字と数える		 */
/* ========================================================================= */
int SboTextCharLen(LPCSTR psz)
{
	int nLen;

	if ((psz == NULL) || (psz[0] == 0)) {
		return 0;
	}
	nLen = Utf8SeqLen ((const BYTE *)psz);
	return (nLen > 0) ? nLen : 1;
}

/* ========================================================================= */
/* 関数名：	SboTextCharWidth												 */
/* 内容：	先頭 1 文字の表示幅（終端なら 0）								 */
/*			ASCII と半角カナは半角（1）、それ以外は全角（2）				 */
/*			絵文字の見た目を変えるだけの文字は 0							 */
/* ========================================================================= */
int SboTextCharWidth(LPCSTR psz)
{
	const BYTE *p = (const BYTE *)psz;
	int nLen;
	DWORD dwCode;

	if ((p == NULL) || (p[0] == 0)) {
		return 0;
	}
	if (p[0] < 0x80) {
		return 1;
	}
	nLen = Utf8SeqLen (p);
	if (nLen == 0) {
		return 1;	/* 正しくないバイトは「?」相当の半角 */
	}
	/* 半角カナ（U+FF61〜U+FF9F = EF BD A1〜EF BE 9F） */
	if ((nLen == 3) && (p[0] == 0xEF) &&
			(((p[1] == 0xBD) && (p[2] >= 0xA1)) || ((p[1] == 0xBE) && (p[2] <= 0x9F)))) {
		return 1;
	}
	dwCode = Utf8Decode (p, nLen);
	/* ZWJ・異体字セレクタ・肌の色・タグ文字（前の絵文字とくっついて描かれる） */
	if ((dwCode == 0x200D) ||
			((dwCode >= 0xFE00) && (dwCode <= 0xFE0F)) ||
			((dwCode >= 0x1F3FB) && (dwCode <= 0x1F3FF)) ||
			((dwCode >= 0xE0020) && (dwCode <= 0xE007F))) {
		return 0;
	}
	return 2;
}

/* ========================================================================= */
/* 関数名：	SboTextIsWide													 */
/* 内容：	先頭 1 文字が全角か												 */
/* ========================================================================= */
BOOL SboTextIsWide(LPCSTR psz)
{
	return (SboTextCharWidth (psz) >= 2) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：	SboTextWidth													 */
/* 内容：	文字列の表示幅													 */
/* ========================================================================= */
int SboTextWidth(LPCSTR psz, int nLen)
{
	int i, nChar, nWidth;

	if (psz == NULL) {
		return 0;
	}
	nWidth = 0;
	for (i = 0; psz[i] && ((nLen < 0) || (i < nLen)); i += nChar) {
		nChar = SboTextCharLen (&psz[i]);
		if ((nLen >= 0) && (i + nChar > nLen)) {
			break;
		}
		nWidth += SboTextCharWidth (&psz[i]);
	}
	return nWidth;
}

/* ========================================================================= */
/* 関数名：	SboTextFitBytes													 */
/* 内容：	nMaxBytes バイト以内で文字の途中で切れないバイト数				 */
/* ========================================================================= */
int SboTextFitBytes(LPCSTR psz, int nMaxBytes)
{
	int i, nChar;

	if (psz == NULL) {
		return 0;
	}
	for (i = 0; psz[i]; i += nChar) {
		nChar = SboTextCharLen (&psz[i]);
		if (i + nChar > nMaxBytes) {
			break;
		}
	}
	return i;
}

/* ========================================================================= */
/* 関数名：	SboTextFitWidth													 */
/* 内容：	表示幅 nMaxWidth 以内に入るバイト数								 */
/* ========================================================================= */
int SboTextFitWidth(LPCSTR psz, int nMaxWidth)
{
	int i, nChar, nWidth;

	if (psz == NULL) {
		return 0;
	}
	nWidth = 0;
	for (i = 0; psz[i]; i += nChar) {
		nChar = SboTextCharLen (&psz[i]);
		if (nWidth + SboTextCharWidth (&psz[i]) > nMaxWidth) {
			break;
		}
		nWidth += SboTextCharWidth (&psz[i]);
	}
	return i;
}

/* ========================================================================= */
/* 関数名：	SboTextCopy														 */
/* 内容：	文字の途中で切らずにコピー（戻り値はコピーしたバイト数）		 */
/* ========================================================================= */
int SboTextCopy(LPSTR pszDst, int nDstSize, LPCSTR pszSrc)
{
	int nLen;

	if ((pszDst == NULL) || (nDstSize <= 0)) {
		return 0;
	}
	nLen = SboTextFitBytes (pszSrc, nDstSize - 1);
	if (nLen > 0) {
		memmove (pszDst, pszSrc, nLen);
	}
	pszDst[nLen] = 0;
	return nLen;
}

/* ========================================================================= */
/* 関数名：	SboTextIsSingleByteOnly											 */
/* 内容：	ASCII（1 バイト文字）だけでできているか							 */
/* ========================================================================= */
BOOL SboTextIsSingleByteOnly(LPCSTR psz)
{
	int i, nChar;

	if (psz == NULL) {
		return TRUE;
	}
	for (i = 0; psz[i]; i += nChar) {
		nChar = SboTextCharLen (&psz[i]);
		if ((nChar > 1) || ((BYTE)psz[i] >= 0x80)) {
			return FALSE;
		}
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：	SboTextIsValid													 */
/* 内容：	UTF-8 として正しい並びだけでできているか						 */
/* ========================================================================= */
BOOL SboTextIsValid(LPCSTR psz)
{
	const BYTE *p = (const BYTE *)psz;

	if (p == NULL) {
		return TRUE;
	}
	while (*p) {
		int nLen = Utf8SeqLen (p);
		if (nLen == 0) {
			return FALSE;
		}
		p += nLen;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：	SboTextSanitize													 */
/* 内容：	UTF-8 として正しくないバイトを「?」に置き換える（その場で）		 */
/*			戻り値は置き換えた数											 */
/* ========================================================================= */
int SboTextSanitize(LPSTR psz)
{
	BYTE *p = (BYTE *)psz;
	int nCount = 0;

	if (p == NULL) {
		return 0;
	}
	while (*p) {
		int nLen = Utf8SeqLen (p);
		if (nLen == 0) {
			*p = '?';
			nLen = 1;
			nCount ++;
		}
		p += nLen;
	}
	return nCount;
}

/* ========================================================================= */
/* 関数名：	SboTextIsNameSafe												 */
/* 内容：	キャラ名に使える文字だけでできているか							 */
/*			Shift_JIS にある文字だけを使える（絵文字などは使えない）		 */
/*			見た目が似た文字でのなりすましや、名前の探しにくさを防ぐため	 */
/* ========================================================================= */
BOOL SboTextIsNameSafe(LPCSTR psz)
{
	if (psz == NULL) {
		return TRUE;
	}
	if (!SboTextIsValid (psz)) {
		return FALSE;
	}
	std::string strSjis = W32Utf8ToSjisStr (psz);
	return (W32SjisToUtf8Str (strSjis.c_str (), -1) == psz) ? TRUE : FALSE;
}
