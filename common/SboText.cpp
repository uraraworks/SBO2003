/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboText.cpp													 */
/* 内容：		文字列の文字単位の扱い（Shift_JIS 版）						 */
/* ========================================================================= */

#include "StdAfx.h"
#include "SboText.h"

/* ========================================================================= */
/* 関数名：	SboTextCharLen													 */
/* 内容：	先頭 1 文字のバイト数（終端なら 0）								 */
/* ========================================================================= */
int SboTextCharLen(LPCSTR psz)
{
	if ((psz == NULL) || (psz[0] == 0)) {
		return 0;
	}
	/* 2 バイト文字の 1 バイト目で、2 バイト目が終端でない */
	if (IsDBCSLeadByte ((BYTE)psz[0]) && psz[1]) {
		return 2;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：	SboTextCharWidth												 */
/* 内容：	先頭 1 文字の表示幅（終端なら 0）								 */
/* ========================================================================= */
int SboTextCharWidth(LPCSTR psz)
{
	/* Shift_JIS は「1 バイト文字 = 半角、2 バイト文字 = 全角」なのでバイト数と同じ */
	return SboTextCharLen (psz);
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
/* 内容：	1 バイト文字だけでできているか									 */
/* ========================================================================= */
BOOL SboTextIsSingleByteOnly(LPCSTR psz)
{
	int i, nChar;

	if (psz == NULL) {
		return TRUE;
	}
	for (i = 0; psz[i]; i += nChar) {
		nChar = SboTextCharLen (&psz[i]);
		if (nChar > 1) {
			return FALSE;
		}
	}
	return TRUE;
}
