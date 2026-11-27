/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		お試し個室編集ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgTrialRoomEdit.cpp									 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
/*
 * お試し個室 1 件の編集画面。リソースは IDD_TRIALROOM_EDIT。
 * OK でサーバーへ送り、サーバーはデータAPIと同じ確認を通して書き込む。
 * うまくいけば閉じ、だめならサーバーからの理由を出して開いたままにする。
 * 説明文は台詞と同じ書き方（改行は @1）のまま。画面で Enter で改行した所は @1 にして送る。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgTrialRoom.h"
#include "AdminDlgTrialRoomEdit.h"
#include <string>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define TRIALROOMEDIT_TITLE		"お試し個室"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* お試し個室の配列の番号（-1 なら追加） */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static TRIALROOMINFO s_Info;									/* 編集中の内容 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static BOOL SaveAll(HWND hWnd);													/* 画面の内容を覚える */
static BOOL GetNumber(HWND hWnd, int nID, LPCSTR pszName, unsigned long ulMax, unsigned long &ulOut);	/* 数字の欄を読む */

/* ========================================================================= */
/* 関数名：ShowDlgTrialRoomEdit												 */
/* 内容：ダイアログ表示(nNo=-1 なら追加)									 */
/* ========================================================================= */
void ShowDlgTrialRoomEdit(HWND hWndParent, int nNo)
{
	const TRIALROOMINFO *pInfo;

	if (s_hWndDlg) {
		return;
	}

	ZeroMemory (&s_Info, sizeof (s_Info));
	pInfo = (nNo >= 0) ? AdminTrialRoomGet (nNo) : NULL;
	if (pInfo) {
		s_Info = *pInfo;
		s_nNo = nNo;
	} else {
		/* 追加：設定が済むまで入れないよう、閉じておく */
		s_nNo = -1;
		s_Info.byState = TRIALROOMSTATE_CLOSED;
	}
	s_bWait = FALSE;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_TRIALROOM_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	/* 管理者ウィンドウに重ねて出す */
	{
		RECT rc;
		GetWindowRect (g_hWndAdmin ? g_hWndAdmin : hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgTrialRoomEdit											 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgTrialRoomEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_bWait = FALSE;
}

/* ========================================================================= */
/* 関数名：IsDlgTrialRoomEditOpen											 */
/* 内容：開いているか														 */
/* ========================================================================= */
BOOL IsDlgTrialRoomEditOpen(void)
{
	return (s_hWndDlg != NULL) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：TrialRoomEditOnResult											 */
/* 内容：書き込みの結果が届いた												 */
/* ========================================================================= */
void TrialRoomEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg)
{
	char szMsg[MAX_ADMINTRIALROOMMSG + 64];

	if ((s_hWndDlg == NULL) || !s_bWait) {
		return;
	}
	s_bWait = FALSE;
	EnableWindow (GetDlgItem (s_hWndDlg, IDOK), TRUE);
	if (!bOK) {
		snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", (pszMsg && pszMsg[0]) ? pszMsg : "");
		MessageBox (s_hWndDlg, szMsg, TRIALROOMEDIT_TITLE, MB_OK);
		return;
	}
	CloseDlgTrialRoomEdit ();
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_CLOSE:
		CloseDlgTrialRoomEdit ();
		return 1;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：SetNumber														 */
/* 内容：数字の欄に値を入れる												 */
/* ========================================================================= */
static void SetNumber(HWND hWnd, int nID, unsigned long ulVal)
{
	char szTmp[32];

	snprintf (szTmp, sizeof (szTmp), "%lu", ulVal);
	SetDlgItemText (hWnd, nID, szTmp);
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	char szTmp[160];
	HWND hWndCombo;

	if (s_nNo >= 0) {
		snprintf (szTmp, sizeof (szTmp), "番号 %02d（入口のNPCの台詞は @x%02d。番号は変えられません）", s_nNo, s_nNo);
	} else {
		snprintf (szTmp, sizeof (szTmp), "新しいお試し個室（番号は自動で決まります）");
	}
	SetDlgItemText (hWnd, IDC_TR_ID, szTmp);
	SendDlgItemMessage (hWnd, IDC_TR_NAME, EM_LIMITTEXT, MAX_TRIALROOMNAME - 1, 0);
	SendDlgItemMessage (hWnd, IDC_TR_DESC, EM_LIMITTEXT, MAX_TRIALROOMDESC - 1, 0);
	SetDlgItemText (hWnd, IDC_TR_NAME, s_Info.szName);
	SetDlgItemText (hWnd, IDC_TR_DESC, s_Info.szDesc);

	/* 状態（0=閉じている 1=開いている） */
	hWndCombo = GetDlgItem (hWnd, IDC_TR_STATE);
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"閉じている");
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"開いている");
	SendMessage (hWndCombo, CB_SETCURSEL, (s_Info.byState == TRIALROOMSTATE_OPEN) ? 1 : 0, 0);

	SetNumber (hWnd, IDC_TR_STTMP, s_Info.wLoanStTmp);
	SetNumber (hWnd, IDC_TR_MAPID, s_Info.dwMapID);
	SetNumber (hWnd, IDC_TR_X, s_Info.dwX);
	SetNumber (hWnd, IDC_TR_Y, s_Info.dwY);
	SetNumber (hWnd, IDC_TR_X1, s_Info.dwX1);
	SetNumber (hWnd, IDC_TR_Y1, s_Info.dwY1);
	SetNumber (hWnd, IDC_TR_X2, s_Info.dwX2);
	SetNumber (hWnd, IDC_TR_Y2, s_Info.dwY2);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		if (s_bWait) {
			break;
		}
		if (!SaveAll (hWnd)) {
			break;
		}
		s_bWait = TRUE;
		EnableWindow (GetDlgItem (hWnd, IDOK), FALSE);
		SendCmdADMINTRIALROOM (ADMINTRIALROOM_SET, s_nNo, &s_Info);
		break;

	case IDCANCEL:
		CloseDlgTrialRoomEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：GetNumber														 */
/* 内容：数字の欄を読む（空・数字以外・上限超えなら理由を出して FALSE）		 */
/* ========================================================================= */
static BOOL GetNumber(HWND hWnd, int nID, LPCSTR pszName, unsigned long ulMax, unsigned long &ulOut)
{
	char szText[32], *pEnd = NULL;
	char szMsg[160];

	ZeroMemory (szText, sizeof (szText));
	GetDlgItemText (hWnd, nID, szText, sizeof (szText) - 1);
	ulOut = strtoul (szText, &pEnd, 10);
	if ((szText[0] == 0) || (pEnd == NULL) || (*pEnd != 0) || (ulOut > ulMax)) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%lu の数字で入れてください。", pszName, ulMax);
		MessageBox (hWnd, szMsg, TRIALROOMEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SaveAll															 */
/* 内容：画面の内容を s_Info に覚える（おかしければ理由を出して FALSE）		 */
/*       ここでは数字の読み取りだけ。範囲・番号の確認はサーバーがする		 */
/* ========================================================================= */
static BOOL SaveAll(HWND hWnd)
{
	TRIALROOMINFO Info = s_Info;
	char szName[MAX_TRIALROOMNAME + 1], szDesc[MAX_TRIALROOMDESC + 1];
	std::string strDesc;
	unsigned long ul;

	ZeroMemory (szName, sizeof (szName));
	GetDlgItemText (hWnd, IDC_TR_NAME, szName, sizeof (szName) - 1);
	if (szName[0] == 0) {
		MessageBox (hWnd, "名前を入れてください。", TRIALROOMEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, IDC_TR_NAME));
		return FALSE;
	}
	ZeroMemory (Info.szName, sizeof (Info.szName));
	strncpy (Info.szName, szName, sizeof (Info.szName) - 1);

	/* 説明文：画面の改行は台詞と同じ @1 にする */
	ZeroMemory (szDesc, sizeof (szDesc));
	GetDlgItemText (hWnd, IDC_TR_DESC, szDesc, sizeof (szDesc) - 1);
	for (const char *p = szDesc; *p; p ++) {
		if (*p == '\r') {
			continue;
		}
		if (*p == '\n') {
			strDesc += "@1";
		} else {
			strDesc += *p;
		}
	}
	if (strDesc.size () >= sizeof (Info.szDesc)) {
		MessageBox (hWnd, "説明文が長すぎます。\n（UTF-8 で 511 バイトまで。全角ならおよそ 170 文字まで。改行は @1 で 2 バイト）", TRIALROOMEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, IDC_TR_DESC));
		return FALSE;
	}
	ZeroMemory (Info.szDesc, sizeof (Info.szDesc));
	strncpy (Info.szDesc, strDesc.c_str (), sizeof (Info.szDesc) - 1);

	Info.byState = ((int)SendDlgItemMessage (hWnd, IDC_TR_STATE, CB_GETCURSEL, 0, 0) == 1) ? TRIALROOMSTATE_OPEN : TRIALROOMSTATE_CLOSED;
	if (!GetNumber (hWnd, IDC_TR_STTMP, "貸すテンプレート", 65535, ul)) return FALSE;
	Info.wLoanStTmp = (WORD)ul;
	if (!GetNumber (hWnd, IDC_TR_MAPID, "入るマップ", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwMapID = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_X, "入るX", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwX = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_Y, "入るY", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwY = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_X1, "範囲の左", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwX1 = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_Y1, "範囲の上", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwY1 = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_X2, "範囲の右", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwX2 = (DWORD)ul;
	if (!GetNumber (hWnd, IDC_TR_Y2, "範囲の下", 0xFFFFFFFFUL, ul)) return FALSE;
	Info.dwY2 = (DWORD)ul;

	s_Info = Info;
	return TRUE;
}
