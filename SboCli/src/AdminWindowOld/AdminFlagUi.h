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
 * 番号がフラグのグループ（メイン-サブ）の帯に入る時は、名前の前に「ルシー-3」を出す。
 * 番号の欄には、数字のほかに「グループ番号-サブ」（例 3-2。グループ名でも可）も入れられる
 * （AdminFlagGetBit・AdminFlagGetNum が番号にする。中で使う番号は今まで通り）。
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

/* 番号の欄を、byKind（FLAGKIND_*）の番号として読む。数字のほかに「グループ番号-サブ」（例 3-2）も受け付ける。
   空欄なら bBlank=TRUE。読めなければ理由を pszErr に入れて FALSE（範囲は見ない） */
static inline BOOL AdminFlagReadNoKind(HWND hWnd, int nID, BYTE byKind, unsigned long &ulNo, BOOL &bBlank, char *pszErr, int nErrSize)
{
	char szTmp[96];
	WORD wNo = 0;

	pszErr[0] = 0;
	if (AdminFlagReadNo (hWnd, nID, ulNo, bBlank)) {
		return TRUE;
	}
	/* 数字だけではない（「3-2」など） */
	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	if (!AdminFlagParseNo (byKind, szTmp, wNo, pszErr, nErrSize)) {
		return FALSE;
	}
	ulNo = wNo;
	bBlank = FALSE;
	return TRUE;
}

/* 番号の横の名前の文字を作り直す（番号が空欄なら何も出さない。名前表に無ければ「（名前なし）」）。
   番号がグループの帯に入る時は、名前の前に「ルシー-3」を出す（「3-2」の形で入れた時は「=103」で元の番号も） */
static inline void AdminFlagUpdateName(HWND hWnd, int nIDEdit, int nIDLabel, BYTE byKind)
{
	unsigned long ulNo;
	BOOL bBlank, bDigit;
	LPCSTR pszName;
	WORD wMax = (byKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;
	char szErr[200], szGroup[MAX_FLAGGROUPNAME + 16], szText[MAX_FLAGNAME + MAX_FLAGGROUPNAME + 48];

	if (!AdminFlagReadNo (hWnd, nIDEdit, ulNo, bBlank)) {
		bDigit = FALSE;
		if (!AdminFlagReadNoKind (hWnd, nIDEdit, byKind, ulNo, bBlank, szErr, sizeof (szErr))) {
			SetDlgItemText (hWnd, nIDLabel, "（読めません）");
			return;
		}
	} else {
		bDigit = TRUE;
	}
	if (bBlank) {
		SetDlgItemText (hWnd, nIDLabel, "");
		return;
	}
	if (ulNo >= wMax) {
		SetDlgItemText (hWnd, nIDLabel, "（範囲外）");
		return;
	}
	pszName = AdminFlagNameFind (byKind, (WORD)ulNo);
	szText[0] = 0;
	if (!bDigit) {
		snprintf (szText, sizeof (szText), "=%lu ", ulNo);
	}
	if (AdminFlagGroupFormat (byKind, (WORD)ulNo, szGroup, sizeof (szGroup))) {
		strncat (szText, szGroup, sizeof (szText) - strlen (szText) - 1);
		if (pszName) {
			strncat (szText, " ", sizeof (szText) - strlen (szText) - 1);
		}
	} else if (pszName == NULL) {
		strncat (szText, "（名前なし）", sizeof (szText) - strlen (szText) - 1);
	}
	if (pszName) {
		strncat (szText, pszName, sizeof (szText) - strlen (szText) - 1);
	}
	SetDlgItemText (hWnd, nIDLabel, szText);
}

/* 印か数値の番号の欄（空欄は bBlank=TRUE）。数字か「グループ番号-サブ」。おかしければ理由を出して FALSE */
static inline BOOL AdminFlagGetNo(HWND hWnd, HWND hWndMsg, int nID, BYTE byKind, const std::string &strName, LPCSTR pszTitle, unsigned long &ulOut, BOOL &bBlank)
{
	char szMsg[400], szErr[200];
	int nMax = (byKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;

	if (!AdminFlagReadNoKind (hWnd, nID, byKind, ulOut, bBlank, szErr, sizeof (szErr))) {
		snprintf (szMsg, sizeof (szMsg), "%sを読めません。\n%s", strName.c_str (), szErr);
		MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	if (!bBlank && (ulOut >= (unsigned long)nMax)) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%d の%sの番号を入れてください（「グループ番号-サブ」でも可）。",
				strName.c_str (), nMax - 1, (byKind == FLAGKIND_BIT) ? "印" : "数値");
		MessageBox (hWndMsg, szMsg, pszTitle, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	return TRUE;
}

/* 印の番号の欄（空欄=FLAG_NOBIT）。おかしければ理由を出して FALSE */
static inline BOOL AdminFlagGetBit(HWND hWnd, HWND hWndMsg, int nID, const std::string &strName, LPCSTR pszTitle, WORD &wOut)
{
	unsigned long ul;
	BOOL bBlank;

	if (!AdminFlagGetNo (hWnd, hWndMsg, nID, FLAGKIND_BIT, strName, pszTitle, ul, bBlank)) {
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

	if (!AdminFlagGetNo (hWnd, hWndMsg, nID, FLAGKIND_NUM, strName, pszTitle, ul, bBlank)) {
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
