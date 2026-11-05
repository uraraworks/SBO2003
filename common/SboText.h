/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboText.h													 */
/* 内容：		文字列の文字単位の扱い（文字コードの違いをここに閉じ込める）	 */
/* ========================================================================= */
/*
 * ゲーム内の文字列を「1 文字ずつ進める」「表示幅を数える」「文字の途中で
 * 切らずに詰める」処理は、必ずここの関数を通すこと。
 * いまの中身は Shift_JIS(CP932) 用。UTF-8 へ切り替える時はこのファイルの
 * 実装だけを差し替えれば、呼び出し側は直さなくて済むようにしてある。
 *
 * 表示幅は「半角 1 文字 = 1、全角 1 文字 = 2」で数える
 * （画面上は半角 1 文字が 6px または 8px、全角はその倍）。
 */

#pragma once

int		SboTextCharLen		(LPCSTR psz);							/* 先頭 1 文字のバイト数（終端なら 0） */
int		SboTextCharWidth	(LPCSTR psz);							/* 先頭 1 文字の表示幅（終端なら 0） */
BOOL	SboTextIsWide		(LPCSTR psz);							/* 先頭 1 文字が全角か */
int		SboTextWidth		(LPCSTR psz, int nLen = -1);			/* 文字列の表示幅（nLen はバイト数、-1 で終端まで） */
int		SboTextFitBytes		(LPCSTR psz, int nMaxBytes);			/* nMaxBytes バイト以内で文字の途中で切れないバイト数 */
int		SboTextFitWidth		(LPCSTR psz, int nMaxWidth);			/* 表示幅 nMaxWidth 以内に入るバイト数 */
int		SboTextCopy			(LPSTR pszDst, int nDstSize, LPCSTR pszSrc);	/* 文字の途中で切らずにコピー（戻り値はバイト数） */
BOOL	SboTextIsSingleByteOnly	(LPCSTR psz);						/* 1 バイト文字だけでできているか */
