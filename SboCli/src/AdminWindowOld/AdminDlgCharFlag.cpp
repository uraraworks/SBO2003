/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		キャラのフラグダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgCharFlag.cpp										 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * 管理者ウィンドウのキャラ情報で選んでいるキャラ（プレイヤーだけ）の、
 * 印（ON/OFF）と数値（0〜255）を名前付きで見て、直すための画面。リソースは IDD_CHARFLAG。
 * キャラのフラグはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * 届いたものを AdminFlagTalk.cpp が覚えている。届くたびに更新番号が変わるので、
 * 変わっていたら一覧を描き直す。
 * ・印：名前の付いた印は全部（立っていない物も）並べ、チェックで立てる・消す。
 *       名前の無い印は立っている物だけが出る。番号を入れて「立てる」「消す」もできる
 * ・数値：名前の付いた数値は全部、名前の無い数値は 0 でない物だけが出る。
 *         選ぶと番号と値が下の欄に入る。値を直して「設定」で変える
 * ・「宝箱」ボタン：開けた宝箱の記録（印とは別）を見て直す画面（AdminDlgCharChest.cpp）を開く
 * ・「立っている印・0でない数値だけ」にチェックすると、立っている物だけに絞る
 * ・フラグのグループ（メイン-サブ）：一覧に「グループ」列（ルシー-3 の形）を出す。
 *   一覧の上の欄でグループを選ぶと、そのグループの番号の帯の全部（名前が無くて立っていない物も）に絞れる
 *   （すべて／各グループ／どこにも入らない）。番号の欄は、数字のほかに「グループ番号-サブ」（例 3-2）も受け付ける
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminFlagTalk.h"
#include "AdminFlagUi.h"
#include "AdminDlgCharFlag.h"
#include "AdminDlgCharChest.h"
#include <string>
#include <vector>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define CHARFLAG_TITLE		"キャラのフラグ"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static DWORD s_dwTblID = 0;										/* 見ているキャラのテーブルID */
static char s_szCharName[64];									/* 見ているキャラの名前 */
static BOOL s_bHave = FALSE;									/* このキャラのフラグが届いているか */
static BYTE s_abyFlag[MAX_FLAGBITBYTE];							/* 届いている印 */
static BYTE s_abyFlagNum[MAX_FLAGNUM];							/* 届いている数値 */
static DWORD s_dwFlagSeqSeen = 0;								/* 描き直した時のキャラのフラグの更新番号 */
static DWORD s_dwNameSeqSeen = 0;								/* 描き直した時の名前表の更新番号 */
static DWORD s_dwGroupSeqSeen = 0;								/* グループの選択肢を作った時の更新番号 */

#define CFGROUP_ALL			(-1)								/* グループの絞り込み: すべて */
#define CFGROUP_NONE		(-2)								/* グループの絞り込み: どこにも入らない */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */
static BOOL s_bPending = FALSE;									/* 問い合わせ・変更の結果を待っている */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnNotify(HWND hWnd, LPARAM lParam);									/* メッセージハンドラ(WM_NOTIFY) */
static void OnChanged(void);													/* キャラのフラグ・名前表・結果が届いた */
static void RenewLists(void);													/* 一覧を作り直す */
static void LoadFlag(void);														/* 届いているフラグを覚える */
static BOOL IsBitOn(int nNo);													/* 印が立っているか */
static void RenewGroupCombo(int nIDCombo, BYTE byKind);							/* グループの絞り込みの選択肢を作り直す */
static BOOL InGroupFilter(int nIDCombo, BYTE byKind, int nNo);					/* グループの絞り込みに合う番号か */
static BOOL IsGroupPicked(int nIDCombo);										/* 特定のグループを選んでいるか */

/* ========================================================================= */
/* 関数名：ShowDlgCharFlag													 */
/* 内容：ダイアログ表示（開いたらサーバーへそのキャラのフラグを問い合わせる）*/
/* ========================================================================= */
void ShowDlgCharFlag(HWND hWndParent, DWORD dwTblID, LPCSTR pszCharName)
{
	if (s_hWndDlg) {
		CloseDlgCharFlag ();		/* 開いていたら、いまのキャラで開き直す */
	}

	s_dwTblID = dwTblID;
	strncpy (s_szCharName, pszCharName ? pszCharName : "", sizeof (s_szCharName) - 1);
	s_szCharName[sizeof (s_szCharName) - 1] = 0;
	s_bHave = FALSE;
	ZeroMemory (s_abyFlag, sizeof (s_abyFlag));
	ZeroMemory (s_abyFlagNum, sizeof (s_abyFlagNum));
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_CHARFLAG, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	s_dwFlagSeqSeen = AdminCharFlagGetSeq ();
	AdminFlagNameLoadIfEmpty ();
	s_bPending = TRUE;
	AdminCharFlagRequest (s_dwTblID);
	RenewLists ();

	/* 管理者ウィンドウに重ねて出す */
	{
		RECT rc;
		GetWindowRect (g_hWndAdmin ? g_hWndAdmin : hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgCharFlag													 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgCharFlag(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	CloseDlgCharChest ();
	AdminFlagTalkRemoveListener (OnChanged);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_bPending = FALSE;
}

/* ========================================================================= */
/* 関数名：IsBitOn															 */
/* 内容：印が立っているか													 */
/* ========================================================================= */
static BOOL IsBitOn(int nNo)
{
	return (s_abyFlag[nNo / 8] & (1 << (nNo % 8))) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：LoadFlag															 */
/* 内容：届いているフラグを覚える（見ているキャラの物だけ）					 */
/* ========================================================================= */
static void LoadFlag(void)
{
	DWORD dwTblID;
	BYTE abyFlag[MAX_FLAGBITBYTE], abyNum[MAX_FLAGNUM];

	if (AdminCharFlagGet (&dwTblID, abyFlag, abyNum) && (dwTblID == s_dwTblID)) {
		CopyMemory (s_abyFlag, abyFlag, sizeof (s_abyFlag));
		CopyMemory (s_abyFlagNum, abyNum, sizeof (s_abyFlagNum));
		s_bHave = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：OnChanged														 */
/* 内容：キャラのフラグ・名前表・書き込みの結果が届いた						 */
/*       （更新番号が変わっていたら描き直す）								 */
/* ========================================================================= */
static void OnChanged(void)
{
	const ADMINFLAGTALKRESULT *pRes = AdminFlagTalkGetResult ();
	BOOL bRenew = FALSE;
	char szMsg[MAX_ADMINQUESTMSG + 64];

	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwFlagSeqSeen != AdminCharFlagGetSeq ()) {
		s_dwFlagSeqSeen = AdminCharFlagGetSeq ();
		LoadFlag ();
		bRenew = TRUE;
	}
	if (s_dwNameSeqSeen != AdminFlagNameGetSeq ()) {
		bRenew = TRUE;
	}
	if (bRenew) {
		RenewLists ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bPending && (pRes->byCmd == CMD_ADMINCHARFLAG)) {
			s_bPending = FALSE;
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "変えられませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, CHARFLAG_TITLE, MB_OK);
			}
		}
	}
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
		CloseDlgCharFlag ();
		return 1;

	case WM_NOTIFY:
		OnNotify (hWnd, lParam);
		return 0;

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
	char szTmp[128];
	HWND hWndTmp;
	LV_COLUMN lvcol;

	s_hWndDlg = hWnd;		/* この後の小物が使う（CreateDialog から戻る前に呼ばれる） */

	snprintf (szTmp, sizeof (szTmp), "キャラのフラグ（%s）", s_szCharName);
	SetWindowText (hWnd, szTmp);
	snprintf (szTmp, sizeof (szTmp), "キャラ：%s（テーブルID %lu）", s_szCharName, (unsigned long)s_dwTblID);
	SetDlgItemText (hWnd, IDC_CF_WHO, szTmp);

	/* 印の一覧（チェックで立てる・消す） */
	hWndTmp = GetDlgItem (hWnd, IDC_CF_BITS);
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = 44;
	lvcol.pszText = (LPSTR)"番号";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 90;
	lvcol.pszText = (LPSTR)"グループ";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	lvcol.cx = 150;		/* 一覧の幅いっぱいに近く（列の境目をドラッグでも変えられる） */
	lvcol.pszText = (LPSTR)"名前";
	ListView_InsertColumn (hWndTmp, 2, &lvcol);
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

	/* 数値の一覧 */
	hWndTmp = GetDlgItem (hWnd, IDC_CF_NUMS);
	lvcol.cx = 40;
	lvcol.pszText = (LPSTR)"番号";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 80;
	lvcol.pszText = (LPSTR)"グループ";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	lvcol.cx = 90;
	lvcol.pszText = (LPSTR)"名前";
	ListView_InsertColumn (hWndTmp, 2, &lvcol);
	lvcol.cx = 34;
	lvcol.pszText = (LPSTR)"値";
	ListView_InsertColumn (hWndTmp, 3, &lvcol);
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	unsigned long ulNo, ulVal;
	BOOL bBlank;

	switch (nID) {
	case IDC_GET:	/* 取得（サーバーから取り直す） */
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminFlagNameRequest ();
		AdminFlagGroupRequest ();
		AdminCharFlagRequest (s_dwTblID);
		break;

	case IDC_CF_CHEST:	/* 開けた宝箱の記録の画面を開く（印とは別の記録） */
		ShowDlgCharChest (hWnd, s_dwTblID, s_szCharName);
		break;

	case IDC_CF_BITGROUP:	/* 印のグループで絞り込み */
	case IDC_CF_NUMGROUP:	/* 数値のグループで絞り込み */
		if (nCodeNotify == CBN_SELCHANGE) {
			RenewLists ();
		}
		break;

	case IDC_CF_ONLY:	/* 立っている物だけ */
		RenewLists ();
		break;

	case IDC_CF_BITON:	/* 番号を指定して印を立てる */
	case IDC_CF_BITOFF:	/* 番号を指定して印を消す */
		/* 数字のほかに「グループ番号-サブ」（例 3-2）も受け付ける */
		if (!AdminFlagGetNo (hWnd, hWnd, IDC_CF_BITNO, FLAGKIND_BIT, "印の番号", CHARFLAG_TITLE, ulNo, bBlank)) {
			break;
		}
		if (bBlank) {
			MessageBox (hWnd, "印の番号を入れてください。（例 103・3-2）", CHARFLAG_TITLE, MB_OK);
			SetFocus (GetDlgItem (hWnd, IDC_CF_BITNO));
			break;
		}
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminCharFlagSetBit (s_dwTblID, (WORD)ulNo, (nID == IDC_CF_BITON) ? TRUE : FALSE);
		break;

	case IDC_CF_NUMSET:	/* 数値を変える */
		if (!AdminFlagGetNo (hWnd, hWnd, IDC_CF_NUMNO, FLAGKIND_NUM, "数値の番号", CHARFLAG_TITLE, ulNo, bBlank)) {
			break;
		}
		if (bBlank) {
			MessageBox (hWnd, "数値の番号を入れてください。（例 5・2-3）", CHARFLAG_TITLE, MB_OK);
			SetFocus (GetDlgItem (hWnd, IDC_CF_NUMNO));
			break;
		}
		if (!AdminFlagGetValue (hWnd, hWnd, IDC_CF_NUMVAL, 255, "値", CHARFLAG_TITLE, ulVal)) {
			break;
		}
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminCharFlagSetNum (s_dwTblID, (WORD)ulNo, (BYTE)ulVal);
		break;

	case IDCANCEL:
		CloseDlgCharFlag ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnNotify															 */
/* 内容：メッセージハンドラ(WM_NOTIFY)										 */
/*       印のチェックが変わったら立てる・消す。数値を選んだら下の欄に出す	 */
/* ========================================================================= */
static void OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR pHdr = (LPNMHDR)lParam;
	NMLISTVIEW *pNm = (NMLISTVIEW *)lParam;
	int nNo;

	if ((pHdr == NULL) || (pHdr->code != LVN_ITEMCHANGED)) {
		return;
	}
	nNo = (int)pNm->lParam;

	if (pHdr->idFrom == IDC_CF_BITS) {
		BOOL bNew, bChanged;

		if ((nNo < 0) || (nNo >= MAX_FLAGBIT) || !s_bHave) {
			return;
		}
		bChanged = ((pNm->uNewState & LVIS_STATEIMAGEMASK) != (pNm->uOldState & LVIS_STATEIMAGEMASK)) ? TRUE : FALSE;
		bNew = (((pNm->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2) ? TRUE : FALSE;
		/* 一覧を作り直した時にも通知が来るので、手元の印と違う時だけ送る */
		if (!bChanged || (bNew == IsBitOn (nNo)) || ((pNm->uOldState & LVIS_STATEIMAGEMASK) == 0)) {
			return;
		}
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminCharFlagSetBit (s_dwTblID, (WORD)nNo, bNew);
	} else if (pHdr->idFrom == IDC_CF_NUMS) {
		if (((pNm->uNewState & LVIS_SELECTED) == 0) || (pNm->uOldState & LVIS_SELECTED)) {
			return;
		}
		if ((nNo < 0) || (nNo >= MAX_FLAGNUM)) {
			return;
		}
		AdminFlagSetNo (hWnd, IDC_CF_NUMNO, nNo, FALSE);
		AdminFlagSetNo (hWnd, IDC_CF_NUMVAL, s_abyFlagNum[nNo], FALSE);
	}
}

/* グループの並べ方（並び順、同じなら番号順） */
static bool GroupLess(int a, int b)
{
	const FLAGGROUPINFO *pA = AdminFlagGroupGet (a), *pB = AdminFlagGroupGet (b);

	return (pA->wSort != pB->wSort) ? (pA->wSort < pB->wSort) : (pA->wID < pB->wID);
}

/* ========================================================================= */
/* 関数名：RenewGroupCombo													 */
/* 内容：グループの絞り込みの選択肢を作り直す（すべて／各グループ／どこにも入らない）*/
/*       グループが変わった時だけ作り直し、選んでいた物は選んだままにする	 */
/* ========================================================================= */
static void RenewGroupCombo(int nIDCombo, BYTE byKind)
{
	HWND hWndCombo = GetDlgItem (s_hWndDlg, nIDCombo);
	std::vector<int> anIndex;
	int nSelData = CFGROUP_ALL, nSel, nIdx;
	char szTmp[MAX_FLAGGROUPNAME + 48];
	DWORD dwSeq = AdminFlagGroupGetSeq ();

	/* 作り直しは、グループが届いた時か、まだ作っていない時だけ（選択肢が 1 つも無い時は作る） */
	if ((s_dwGroupSeqSeen == dwSeq) && ((int)SendMessage (hWndCombo, CB_GETCOUNT, 0, 0) > 0)) {
		return;
	}
	nSel = (int)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0);
	if (nSel >= 0) {
		nSelData = (int)SendMessage (hWndCombo, CB_GETITEMDATA, nSel, 0);
	}
	for (int i = 0; i < AdminFlagGroupGetCount (); i ++) {
		const FLAGGROUPINFO *p = AdminFlagGroupGet (i);
		if (p && (p->byState == FLAGGROUPSTATE_ON) && (p->byKind == byKind)) {
			anIndex.push_back (i);
		}
	}
	std::sort (anIndex.begin (), anIndex.end (), GroupLess);

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	nSel = 0;
	nIdx = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"すべて");
	SendMessage (hWndCombo, CB_SETITEMDATA, nIdx, CFGROUP_ALL);
	for (size_t i = 0; i < anIndex.size (); i ++) {
		const FLAGGROUPINFO *p = AdminFlagGroupGet (anIndex[i]);

		char szBands[96];

		AdminFlagGroupBandsText (p, szBands, sizeof (szBands));
		snprintf (szTmp, sizeof (szTmp), "%s（%s）", p->szName, szBands);
		nIdx = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
		SendMessage (hWndCombo, CB_SETITEMDATA, nIdx, anIndex[i]);
		if (anIndex[i] == nSelData) {
			nSel = nIdx;
		}
	}
	nIdx = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"どこにも入らない");
	SendMessage (hWndCombo, CB_SETITEMDATA, nIdx, CFGROUP_NONE);
	if (nSelData == CFGROUP_NONE) {
		nSel = nIdx;
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nSel, 0);
	/* 2 つの選択肢（印・数値）を続けて作るので、更新番号は 2 つめを作ってから覚える */
	if (byKind == FLAGKIND_NUM) {
		s_dwGroupSeqSeen = dwSeq;
	}
}

/* 今選んでいる絞り込みの値（CFGROUP_ALL・CFGROUP_NONE・グループの配列の番号） */
static int GetGroupFilter(int nIDCombo)
{
	HWND hWndCombo = GetDlgItem (s_hWndDlg, nIDCombo);
	int nSel = (int)SendMessage (hWndCombo, CB_GETCURSEL, 0, 0);

	return (nSel >= 0) ? (int)SendMessage (hWndCombo, CB_GETITEMDATA, nSel, 0) : CFGROUP_ALL;
}

/* 特定のグループを選んでいるか */
static BOOL IsGroupPicked(int nIDCombo)
{
	return (GetGroupFilter (nIDCombo) >= 0) ? TRUE : FALSE;
}

/* グループの絞り込みに合う番号か */
static BOOL InGroupFilter(int nIDCombo, BYTE byKind, int nNo)
{
	int nFilter = GetGroupFilter (nIDCombo);

	if (nFilter == CFGROUP_ALL) {
		return TRUE;
	}
	if (nFilter == CFGROUP_NONE) {
		return (AdminFlagGroupFind (byKind, (WORD)nNo) == NULL) ? TRUE : FALSE;
	}
	return ((AdminFlagGroupGet (nFilter) != NULL) && (AdminFlagGroupFind (byKind, (WORD)nNo) == AdminFlagGroupGet (nFilter))) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：RenewLists														 */
/* 内容：印・数値の一覧を作り直す											 */
/*       名前の付いた物は全部、名前の無い物は立っている・0でない物だけ		 */
/* ========================================================================= */
static void RenewLists(void)
{
	HWND hWndBits, hWndNums;
	LV_ITEM item;
	char szTmp[64], szGroup[MAX_FLAGGROUPNAME + 16];
	BOOL bOnly;
	int i, nRow, nBitOn = 0, nNumOn = 0, nSelNum = -1, nSelRow;

	if (s_hWndDlg == NULL) {
		return;
	}
	s_dwNameSeqSeen = AdminFlagNameGetSeq ();
	RenewGroupCombo (IDC_CF_BITGROUP, FLAGKIND_BIT);
	RenewGroupCombo (IDC_CF_NUMGROUP, FLAGKIND_NUM);
	bOnly = (IsDlgButtonChecked (s_hWndDlg, IDC_CF_ONLY) == BST_CHECKED) ? TRUE : FALSE;
	hWndBits = GetDlgItem (s_hWndDlg, IDC_CF_BITS);
	hWndNums = GetDlgItem (s_hWndDlg, IDC_CF_NUMS);

	/* いま選んでいる数値の番号を覚えておく（作り直した後も選んだままにする） */
	nSelRow = ListView_GetNextItem (hWndNums, -1, LVNI_ALL | LVNI_SELECTED);
	if (nSelRow >= 0) {
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_PARAM;
		item.iItem = nSelRow;
		if (ListView_GetItem (hWndNums, &item)) {
			nSelNum = (int)item.lParam;
		}
	}

	/* 印 */
	ListView_DeleteAllItems (hWndBits);
	nRow = 0;
	for (i = 0; i < MAX_FLAGBIT; i ++) {
		LPCSTR pszName = AdminFlagNameFind (FLAGKIND_BIT, (WORD)i);
		BOOL bOn = s_bHave ? IsBitOn (i) : FALSE;

		if (bOn) {
			nBitOn ++;
		}
		if (!InGroupFilter (IDC_CF_BITGROUP, FLAGKIND_BIT, i)) {
			continue;
		}
		if ((pszName == NULL) && !bOn && !IsGroupPicked (IDC_CF_BITGROUP)) {
			continue;		/* 名前が無くて立っていない印は出さない（グループを選んだ時は帯の全部を出す） */
		}
		if (bOnly && !bOn) {
			continue;
		}
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = nRow;
		item.iSubItem = 0;
		item.lParam = i;
		snprintf (szTmp, sizeof (szTmp), "%d", i);
		item.pszText = szTmp;
		ListView_InsertItem (hWndBits, &item);
		AdminFlagGroupFormat (FLAGKIND_BIT, (WORD)i, szGroup, sizeof (szGroup));
		ListView_SetItemText (hWndBits, nRow, 1, szGroup);
		ListView_SetItemText (hWndBits, nRow, 2, (LPSTR)(pszName ? pszName : "（名前なし）"));
		ListView_SetCheckState (hWndBits, nRow, bOn ? 1 : 0);
		nRow ++;
	}

	/* 数値 */
	ListView_DeleteAllItems (hWndNums);
	nRow = 0;
	for (i = 0; i < MAX_FLAGNUM; i ++) {
		LPCSTR pszName = AdminFlagNameFind (FLAGKIND_NUM, (WORD)i);
		int nVal = s_bHave ? s_abyFlagNum[i] : 0;

		if (nVal != 0) {
			nNumOn ++;
		}
		if (!InGroupFilter (IDC_CF_NUMGROUP, FLAGKIND_NUM, i)) {
			continue;
		}
		if ((pszName == NULL) && (nVal == 0) && !IsGroupPicked (IDC_CF_NUMGROUP)) {
			continue;
		}
		if (bOnly && (nVal == 0)) {
			continue;
		}
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = nRow;
		item.iSubItem = 0;
		item.lParam = i;
		snprintf (szTmp, sizeof (szTmp), "%d", i);
		item.pszText = szTmp;
		ListView_InsertItem (hWndNums, &item);
		AdminFlagGroupFormat (FLAGKIND_NUM, (WORD)i, szGroup, sizeof (szGroup));
		ListView_SetItemText (hWndNums, nRow, 1, szGroup);
		ListView_SetItemText (hWndNums, nRow, 2, (LPSTR)(pszName ? pszName : "（名前なし）"));
		snprintf (szTmp, sizeof (szTmp), "%d", nVal);
		ListView_SetItemText (hWndNums, nRow, 3, szTmp);
		if (i == nSelNum) {
			ListView_SetItemState (hWndNums, nRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
		nRow ++;
	}

	if (s_bHave) {
		snprintf (szTmp, sizeof (szTmp), "立っている印 %d 個・数値 %d 個", nBitOn, nNumOn);
	} else {
		snprintf (szTmp, sizeof (szTmp), "取得しています…");
	}
	SetDlgItemText (s_hWndDlg, IDC_CF_MSG, szTmp);
	/* 選んでいる数値の欄の値も、届いた値に合わせる */
	if ((nSelNum >= 0) && s_bHave) {
		AdminFlagSetNo (s_hWndDlg, IDC_CF_NUMVAL, s_abyFlagNum[nSelNum], FALSE);
	}
}

/* Copyright(C)URARA-works 2026 */
