/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		印・数値の番号の欄の小物（管理者ウィンドウ用）定義ファイル		 */
/* ファイル名：	AdminFlagUi.h												 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
#pragma once

#include "AdminFlagTalk.h"
#include <string>

/*
 * 会話データの編集・ギルドの依頼の編集・キャラのフラグの画面で使う。
 * 印・数値の番号は番号の欄に入れ、横の文字に名前表の名前を出す。
 * 「無し」は空欄で表し、内部では FLAG_NOBIT / FLAG_NONUM にする。
 */

/* 番号の欄に出す（bNone なら空欄） */
static inline void AdminFlagSetNo(HWND hWnd, int nID, unsigned long ulNo, BOOL bNone)
{
	char szTmp[32];

	if (bNone) {
		szTmp[0] = 0;
	} else {
		snprintf (szTmp, sizeof (szTmp), "%lu", ulNo);
	}
	SetDlgItemText (hWnd, nID, szTmp);
}

/* 番号の欄を読む。空欄なら bBlank=TRUE。数字以外が入っていたら FALSE */
static inline BOOL AdminFlagReadNo(HWND hWnd, int nID, unsigned long &ulNo, BOOL &bBlank)
{
	char szTmp[32];
	const char *p;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	p = szTmp;
	while (*p == ' ') {
		p ++;
	}
	bBlank = (*p == 0) ? TRUE : FALSE;
	ulNo = 0;
	if (bBlank) {
		return TRUE;
	}
	for (const char *q = p; *q; q ++) {
		if ((*q < '0') || (*q > '9')) {
			return FALSE;
		}
	}
	ulNo = strtoul (p, NULL, 10);
	return TRUE;
}

/* 番号の横の名前の文字を作り直す（番号が空欄なら何も出さない。名前表に無ければ「（名前なし）」） */
static inline void AdminFlagUpdateName(HWND hWnd, int nIDEdit, int nIDLabel, BYTE byKind)
{
	unsigned long ulNo;
	BOOL bBlank;
	LPCSTR pszName;
	WORD wMax = (byKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;

	if (!AdminFlagReadNo (hWnd, nIDEdit, ulNo, bBlank) || bBlank) {
		SetDlgItemText (hWnd, nIDLabel, "");
		return;
	}
	if (ulNo >= wMax) {
		SetDlgItemText (hWnd, nIDLabel, "（範囲外）");
		return;
	}
	pszName = AdminFlagNameFind (byKind, (WORD)ulNo);
	SetDlgItemText (hWnd, nIDLabel, pszName ? pszName : "（名前なし）");
}

/* 印の番号の欄（空欄=FLAG_NOBIT）。おかしければ理由を出して FALSE */
static inline BOOL AdminFlagGetBit(HWND hWnd, HWND hWndMsg, int nID, const std::string &strName, LPCSTR pszTitle, WORD &wOut)
{
	unsigned long ul;
	BOOL bBlank;
	char szMsg[200];

	if (!AdminFlagReadNo (hWnd, nID, ul, bBlank) || (!bBlank && (ul >= MAX_FLAGBIT))) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%d の印の番号を入れてください（空欄=無し）。", strName.c_str (), MAX_FLAGBIT - 1);
		MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	wOut = bBlank ? (WORD)FLAG_NOBIT : (WORD)ul;
	return TRUE;
}

/* 数値の番号の欄（空欄=FLAG_NONUM）。おかしければ理由を出して FALSE */
static inline BOOL AdminFlagGetNum(HWND hWnd, HWND hWndMsg, int nID, const std::string &strName, LPCSTR pszTitle, BYTE &byOut)
{
	unsigned long ul;
	BOOL bBlank;
	char szMsg[200];

	if (!AdminFlagReadNo (hWnd, nID, ul, bBlank) || (!bBlank && (ul >= MAX_FLAGNUM))) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%d の数値の番号を入れてください（空欄=無し）。", strName.c_str (), MAX_FLAGNUM - 1);
		MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	byOut = bBlank ? (BYTE)FLAG_NONUM : (BYTE)ul;
	return TRUE;
}

/* 0〜ulMax の数の欄（空欄は 0）。おかしければ理由を出して FALSE */
static inline BOOL AdminFlagGetValue(HWND hWnd, HWND hWndMsg, int nID, unsigned long ulMax, const std::string &strName, LPCSTR pszTitle, unsigned long &ulOut)
{
	BOOL bBlank;
	char szMsg[200];

	if (!AdminFlagReadNo (hWnd, nID, ulOut, bBlank) || (ulOut > ulMax)) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%lu で入れてください。", strName.c_str (), ulMax);
		MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	return TRUE;
}

/* 複数行の欄に出す（データの "\n" を画面の "\r\n" にする） */
static inline void AdminSetMultiText(HWND hWnd, int nID, LPCSTR pszText)
{
	std::string str;

	for (LPCSTR p = pszText; *p; p ++) {
		if (*p == '\r') {
			continue;
		}
		if (*p == '\n') {
			str += '\r';
		}
		str += *p;
	}
	SetDlgItemText (hWnd, nID, str.c_str ());
}

/* 文字の欄を読む（改行は "\n" だけにする。収まらなければ理由を出して FALSE） */
static inline BOOL AdminGetText(HWND hWnd, HWND hWndMsg, int nID, char *pszOut, int nOutSize, BOOL bMultiLine, LPCSTR pszName, LPCSTR pszTitle)
{
	char szTmp[2048], szMsg[200];
	int nLen = 0;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	for (LPCSTR p = szTmp; *p; p ++) {
		if (*p == '\r') {
			continue;
		}
		if ((*p == '\n') && !bMultiLine) {
			continue;
		}
		if (nLen >= nOutSize - 1) {
			snprintf (szMsg, sizeof (szMsg), "%sが長すぎます。\n（UTF-8 で %d バイトまで。全角ならおよそ %d 文字まで）",
					pszName, nOutSize - 1, (nOutSize - 1) / 3);
			MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
			SetFocus (GetDlgItem (hWnd, nID));
			return FALSE;
		}
		pszOut[nLen ++] = *p;
	}
	pszOut[nLen] = 0;
	return TRUE;
}

/* Copyright(C)URARA-works 2026 */
