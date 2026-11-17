/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		思い出の本編集ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgStoryInfoEdit.cpp									 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 思い出の本のシナリオ 1 件の編集画面。リソースは IDD_STORYINFO_EDIT。
 * OK でサーバーへ送り、サーバーはデータAPIと同じ確認を通して書き込む。
 * うまくいけば閉じ、だめならサーバーからの理由を出して開いたままにする。
 * 印・数値・依頼の欄は空欄が「無し」（内部では FLAG_NOBIT / FLAG_NONUM / 0）。
 * 印・数値の番号は数字のほかに「グループ番号-サブ」（例 3-2）でも入れられ、横にメイン-サブの名前が出る。
 * 「グループから範囲を入れる」は、選んだ印のグループの先頭〜最後を、消す印の範囲の 1 つ目に入れる。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgStoryInfo.h"
#include "AdminDlgStoryInfoEdit.h"
#include "AdminFlagUi.h"
#include <string>
#include <vector>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* シナリオの配列の番号（-1 なら追加） */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static STORYINFO s_Info;										/* 編集中の内容 */
static DWORD s_dwGroupSeqSeen = 0;								/* グループの選択肢を作った時の更新番号 */

#define STORYEDIT_TITLE		"思い出の本"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnFlagNameChanged(void);											/* フラグの名前表が届いた */
static void UpdateNames(HWND hWnd);												/* 番号の横の名前を作り直す */
static void RenewGroupCombo(HWND hWnd);											/* 「グループから範囲を入れる」の選択肢（印のグループ）を作り直す */
static BOOL SaveAll(HWND hWnd);													/* 画面の内容を覚える */

/* ========================================================================= */
/* 関数名：ShowDlgStoryInfoEdit												 */
/* 内容：ダイアログ表示(nNo=-1 なら追加)									 */
/* ========================================================================= */
void ShowDlgStoryInfoEdit(HWND hWndParent, int nNo)
{
	const STORYINFO *pInfo;

	if (s_hWndDlg) {
		return;
	}

	ZeroMemory (&s_Info, sizeof (s_Info));
	pInfo = (nNo >= 0) ? AdminStoryGet (nNo) : NULL;
	if (pInfo) {
		s_Info = *pInfo;
		s_nNo = nNo;
	} else {
		/* 追加：「無し」は 0 ではなく専用の値にしておく（0 は 0 番の印・数値になってしまう） */
		s_nNo = -1;
		s_Info.byState = STORYSTATE_ON;
		s_Info.wClearFlag = 0;
		for (int i = 0; i < MAX_STORYFLAGRANGE; i ++) {
			s_Info.awFlagFrom[i] = FLAG_NOBIT;
			s_Info.awFlagTo[i] = FLAG_NOBIT;
		}
		for (int i = 0; i < MAX_STORYNUM; i ++) {
			s_Info.abyNum[i] = FLAG_NONUM;
		}
		s_Info.byCountNum = FLAG_NONUM;
	}
	s_bWait = FALSE;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_STORYINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	/* 管理者ウィンドウに重ねて出す */
	{
		RECT rc;
		GetWindowRect (g_hWndAdmin ? g_hWndAdmin : hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	/* 印・数値の名前を出すため、フラグの名前表を取り寄せておく（届いたら出し直す） */
	AdminFlagTalkAddListener (OnFlagNameChanged);
	AdminFlagNameLoadIfEmpty ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgStoryInfoEdit											 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgStoryInfoEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	AdminFlagTalkRemoveListener (OnFlagNameChanged);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_bWait = FALSE;
}

/* ========================================================================= */
/* 関数名：IsDlgStoryInfoEditOpen											 */
/* 内容：開いているか														 */
/* ========================================================================= */
BOOL IsDlgStoryInfoEditOpen(void)
{
	return (s_hWndDlg != NULL) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：StoryEditOnResult												 */
/* 内容：書き込みの結果が届いた												 */
/* ========================================================================= */
void StoryEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg)
{
	char szMsg[MAX_ADMINSTORYMSG + 64];

	if ((s_hWndDlg == NULL) || !s_bWait) {
		return;
	}
	s_bWait = FALSE;
	EnableWindow (GetDlgItem (s_hWndDlg, IDOK), TRUE);
	if (!bOK) {
		snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", (pszMsg && pszMsg[0]) ? pszMsg : "");
		MessageBox (s_hWndDlg, szMsg, STORYEDIT_TITLE, MB_OK);
		return;
	}
	CloseDlgStoryInfoEdit ();
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
		CloseDlgStoryInfoEdit ();
		return 1;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	char szTmp[160];
	HWND hWndCombo;
	int nSel;

	if (s_nNo >= 0) {
		snprintf (szTmp, sizeof (szTmp), "番号 %u（番号は変えられません）", (unsigned)s_Info.wID);
	} else {
		snprintf (szTmp, sizeof (szTmp), "新しいお話（番号は自動で決まります）");
	}
	SetDlgItemText (hWnd, IDC_S_ID, szTmp);
	SetDlgItemText (hWnd, IDC_S_NAME, s_Info.szName);
	SetDlgItemText (hWnd, IDC_S_EXPLAIN, s_Info.szExplain);
	SendDlgItemMessage (hWnd, IDC_S_NAME, EM_LIMITTEXT, MAX_QUESTNAME - 1, 0);
	SendDlgItemMessage (hWnd, IDC_S_EXPLAIN, EM_LIMITTEXT, MAX_QUESTMSG - 1, 0);

	/* 状態（0=止める 1=出す） */
	hWndCombo = GetDlgItem (hWnd, IDC_S_STATE);
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"止める");
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"出す");
	SendMessage (hWndCombo, CB_SETCURSEL, (s_Info.byState == STORYSTATE_ON) ? 1 : 0, 0);

	snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)s_Info.wSort);
	SetDlgItemText (hWnd, IDC_S_SORT, szTmp);
	AdminFlagSetNo (hWnd, IDC_S_CLEARFLAG, s_Info.wClearFlag, FALSE);

	/* 前のお話（先頭は「なし」。自分自身は選べない） */
	hWndCombo = GetDlgItem (hWnd, IDC_S_PREV);
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"なし");
	SendMessage (hWndCombo, CB_SETITEMDATA, 0, 0);
	nSel = 0;
	for (int i = 0; i < AdminStoryGetCount (); i ++) {
		const STORYINFO *p = AdminStoryGet (i);
		int nIdx;
		if ((p == NULL) || (p->wID == 0) || (p->wID == s_Info.wID)) {
			continue;
		}
		snprintf (szTmp, sizeof (szTmp), "%u: %s", (unsigned)p->wID, p->szName);
		nIdx = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
		SendMessage (hWndCombo, CB_SETITEMDATA, nIdx, p->wID);
		if (p->wID == s_Info.wPrevStory) {
			nSel = nIdx;
		}
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nSel, 0);

	for (int i = 0; i < MAX_STORYFLAGRANGE; i ++) {
		AdminFlagSetNo (hWnd, IDC_S_FROM0 + i, s_Info.awFlagFrom[i], s_Info.awFlagFrom[i] == FLAG_NOBIT);
		AdminFlagSetNo (hWnd, IDC_S_TO0 + i, s_Info.awFlagTo[i], s_Info.awFlagTo[i] == FLAG_NOBIT);
	}
	for (int i = 0; i < MAX_STORYNUM; i ++) {
		AdminFlagSetNo (hWnd, IDC_S_NUM0 + i, s_Info.abyNum[i], s_Info.abyNum[i] == FLAG_NONUM);
		AdminFlagSetNo (hWnd, IDC_S_QUEST0 + i, s_Info.awQuest[i], s_Info.awQuest[i] == 0);
	}
	AdminFlagSetNo (hWnd, IDC_S_COUNTNUM, s_Info.byCountNum, s_Info.byCountNum == FLAG_NONUM);
	s_dwGroupSeqSeen = AdminFlagGroupGetSeq () - 1;
	UpdateNames (hWnd);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_S_CLEARFLAG:
	case IDC_S_COUNTNUM:
		if (nCodeNotify == EN_CHANGE) {
			UpdateNames (hWnd);
		}
		break;

	case IDC_S_FROMGROUP:	/* グループから範囲を入れる（選んだ印のグループの先頭〜最後を、消す範囲の 1 つ目へ） */
		{
			int nSel = (int)SendDlgItemMessage (hWnd, IDC_S_GROUPSEL, CB_GETCURSEL, 0, 0);
			const FLAGGROUPINFO *pGrp = (nSel >= 0) ? AdminFlagGroupGet ((int)SendDlgItemMessage (hWnd, IDC_S_GROUPSEL, CB_GETITEMDATA, nSel, 0)) : NULL;

			if ((pGrp == NULL) || (pGrp->byState != FLAGGROUPSTATE_ON) || (pGrp->byKind != FLAGKIND_BIT)) {
				MessageBox (hWnd, "範囲に入れる印のグループを、横の欄から選んでください。\n（グループは「その他」→「フラグのグループ」で作れます）", STORYEDIT_TITLE, MB_OK);
				break;
			}
			AdminFlagSetNo (hWnd, IDC_S_FROM0, pGrp->wFrom, FALSE);
			AdminFlagSetNo (hWnd, IDC_S_TO0, pGrp->wTo, FALSE);
		}
		break;

	case IDOK:
		if (s_bWait) {
			break;
		}
		if (!SaveAll (hWnd)) {
			break;
		}
		s_bWait = TRUE;
		EnableWindow (GetDlgItem (hWnd, IDOK), FALSE);
		SendCmdADMINSTORY (ADMINSTORY_SET, s_nNo, &s_Info);
		break;

	case IDCANCEL:
		CloseDlgStoryInfoEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnFlagNameChanged												 */
/* 内容：フラグの名前表が届いた												 */
/* ========================================================================= */
static void OnFlagNameChanged(void)
{
	if (s_hWndDlg) {
		UpdateNames (s_hWndDlg);
	}
}

/* ========================================================================= */
/* 関数名：UpdateNames														 */
/* 内容：番号の横の名前を作り直す											 */
/* ========================================================================= */
static void UpdateNames(HWND hWnd)
{
	AdminFlagUpdateName (hWnd, IDC_S_CLEARFLAG, IDC_S_CLEARFLAGN, FLAGKIND_BIT);
	AdminFlagUpdateName (hWnd, IDC_S_COUNTNUM, IDC_S_COUNTNUMN, FLAGKIND_NUM);
	RenewGroupCombo (hWnd);
}

/* グループの並べ方（並び順、同じなら番号順） */
static bool GroupLess(int a, int b)
{
	const FLAGGROUPINFO *pA = AdminFlagGroupGet (a), *pB = AdminFlagGroupGet (b);

	return (pA->wSort != pB->wSort) ? (pA->wSort < pB->wSort) : (pA->wID < pB->wID);
}

/* ========================================================================= */
/* 関数名：RenewGroupCombo													 */
/* 内容：印のグループの選択肢を作り直す（グループが変わった時だけ。選んでいた物は選んだまま）*/
/* ========================================================================= */
static void RenewGroupCombo(HWND hWnd)
{
	HWND hWndCombo = GetDlgItem (hWnd, IDC_S_GROUPSEL);
	std::vector<int> anIndex;
	int nSelData = -1, nSel;
	char szTmp[MAX_FLAGGROUPNAME + 48];

	if (s_dwGroupSeqSeen == AdminFlagGroupGetSeq ()) {
		return;
	}
	s_dwGroupSeqSeen = AdminFlagGroupGetSeq ();

	nSel = (int)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0);
	if (nSel >= 0) {
		nSelData = (int)SendMessage (hWndCombo, CB_GETITEMDATA, nSel, 0);
	}
	for (int i = 0; i < AdminFlagGroupGetCount (); i ++) {
		const FLAGGROUPINFO *p = AdminFlagGroupGet (i);
		if (p && (p->byState == FLAGGROUPSTATE_ON) && (p->byKind == FLAGKIND_BIT)) {
			anIndex.push_back (i);
		}
	}
	/* 並び順（同じなら番号順） */
	std::sort (anIndex.begin (), anIndex.end (), GroupLess);
	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	nSel = -1;
	for (size_t i = 0; i < anIndex.size (); i ++) {
		const FLAGGROUPINFO *p = AdminFlagGroupGet (anIndex[i]);
		int nIdx;

		snprintf (szTmp, sizeof (szTmp), "%s（%u〜%u）", p->szName, (unsigned)p->wFrom, (unsigned)p->wTo);
		nIdx = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
		SendMessage (hWndCombo, CB_SETITEMDATA, nIdx, anIndex[i]);
		if (anIndex[i] == nSelData) {
			nSel = nIdx;
		}
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nSel, 0);
}

/* ========================================================================= */
/* 関数名：SaveAll															 */
/* 内容：画面の内容を s_Info に覚える（おかしければ理由を出して FALSE）		 */
/* ========================================================================= */
static BOOL SaveAll(HWND hWnd)
{
	STORYINFO Info = s_Info;
	char szTmp[MAX_QUESTMSG + 1];
	unsigned long ul;
	int nSel;
	HWND hWndCombo;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, IDC_S_NAME, szTmp, sizeof (szTmp) - 1);
	ZeroMemory (Info.szName, sizeof (Info.szName));
	strncpy (Info.szName, szTmp, sizeof (Info.szName) - 1);
	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, IDC_S_EXPLAIN, szTmp, sizeof (szTmp) - 1);
	ZeroMemory (Info.szExplain, sizeof (Info.szExplain));
	strncpy (Info.szExplain, szTmp, sizeof (Info.szExplain) - 1);
	if (Info.szName[0] == 0) {
		MessageBox (hWnd, "名前を入れてください。", STORYEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, IDC_S_NAME));
		return FALSE;
	}

	nSel = (int)SendDlgItemMessage (hWnd, IDC_S_STATE, CB_GETCURSEL, 0, 0);
	Info.byState = (nSel == 1) ? STORYSTATE_ON : STORYSTATE_OFF;
	if (!AdminFlagGetValue (hWnd, hWnd, IDC_S_SORT, 65535, "並び順", STORYEDIT_TITLE, ul)) return FALSE;
	Info.wSort = (WORD)ul;
	if (!AdminFlagGetBit (hWnd, hWnd, IDC_S_CLEARFLAG, "クリアした印の番号", STORYEDIT_TITLE, Info.wClearFlag)) return FALSE;
	if (Info.wClearFlag == FLAG_NOBIT) {
		MessageBox (hWnd, "クリアした印の番号を入れてください。", STORYEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, IDC_S_CLEARFLAG));
		return FALSE;
	}

	hWndCombo = GetDlgItem (hWnd, IDC_S_PREV);
	nSel = (int)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0);
	Info.wPrevStory = (nSel >= 0) ? (WORD)SendMessage (hWndCombo, CB_GETITEMDATA, nSel, 0) : 0;

	for (int i = 0; i < MAX_STORYFLAGRANGE; i ++) {
		if (!AdminFlagGetBit (hWnd, hWnd, IDC_S_FROM0 + i, "消す印の範囲の先頭", STORYEDIT_TITLE, Info.awFlagFrom[i])) return FALSE;
		if (!AdminFlagGetBit (hWnd, hWnd, IDC_S_TO0 + i, "消す印の範囲の最後", STORYEDIT_TITLE, Info.awFlagTo[i])) return FALSE;
	}
	for (int i = 0; i < MAX_STORYNUM; i ++) {
		if (!AdminFlagGetNum (hWnd, hWnd, IDC_S_NUM0 + i, "戻す数値の番号", STORYEDIT_TITLE, Info.abyNum[i])) return FALSE;
		if (!AdminFlagGetValue (hWnd, hWnd, IDC_S_QUEST0 + i, 65535, "依頼の番号", STORYEDIT_TITLE, ul)) return FALSE;
		Info.awQuest[i] = (WORD)ul;
	}
	if (!AdminFlagGetNum (hWnd, hWnd, IDC_S_COUNTNUM, "やり直した回数を数える数値の番号", STORYEDIT_TITLE, Info.byCountNum)) return FALSE;

	s_Info = Info;
	return TRUE;
}
