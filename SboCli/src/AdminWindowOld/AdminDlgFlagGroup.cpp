/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		フラグのグループダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgFlagGroup.cpp										 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * キャラのフラグ（印 0〜1023・数値 0〜127）の番号の帯に「メイン名」を付けて、
 * 「ルシー-3」のように メイン-サブ で管理するためのグループを、管理者ウィンドウで見て直す画面。
 * リソースは IDD_FLAGGROUP。中で使う番号（会話データ・依頼・見える条件・思い出の本など）は変わらない。
 * グループはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * 届いたものを AdminFlagTalk.cpp が覚えている。届くたびに更新番号が変わるので、
 * 変わっていたら一覧を描き直す。
 * ・一覧：種類（印・数値）とメイン名の部分一致で絞り込める。選ぶと下の欄に出る
 * ・追加：下の欄の種類・メイン名・番号の帯・並びで足す（グループ番号は自動で振られる）
 *   番号の帯は 1 つの欄にカンマ区切りで 16 本まで（例「100-139, 300-319」。区切りは「,」「、」・範囲は「-」「〜」「~」・
 *   数字 1 つだけなら 1 個の帯・全角数字も可。読み取りは common の FlagGroupParseBands）。
 *   サブ番号は帯の順に続けて数える（例：帯 1 が 100〜139・帯 2 が 300〜319 なら、300 はサブ 41）
 * ・更新：選んでいるグループを、下の欄の内容に直す
 * ・削除：選んでいるグループを消す（サーバー側は「削除済みの枠」として残り、帯はほかのグループが使える）
 * 書き込みはデータAPIと同じ確認（同じ種類での帯の重なり・範囲）を通り、だめならサーバーからの理由を出す。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminListFilter.h"
#include "AdminFlagTalk.h"
#include "AdminFlagUi.h"
#include "AdminDlgFlagGroup.h"
#include <algorithm>
#include <vector>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define FLAGGROUP_TITLE		"フラグのグループ"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static DWORD s_dwGroupSeqSeen = 0;								/* 描き直した時のグループの更新番号 */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static std::vector<int> s_anRow;								/* 一覧の行 → グループの配列の番号 */
static int s_nSelIndex = -1;									/* 選んでいるグループの配列の番号 */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号", 45}, {"種類", 50}, {"メイン名", 130}, {"番号の帯", 130}, {"並び", 45},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnNotify(HWND hWnd, LPARAM lParam);									/* メッセージハンドラ(WM_NOTIFY) */
static void OnChanged(void);													/* グループ・書き込みの結果が届いた */
static void RenewList(void);													/* リスト更新 */
static void LoadFields(int nIndex);												/* 選んだグループを下の欄に出す */
static BOOL ReadFields(FLAGGROUPINFO *pInfo);									/* 下の欄を読む */
static void SendSet(int nNo, const FLAGGROUPINFO *pInfo);						/* 書き込みを送る */

/* 並べ方（種類・並び順・番号の順） */
static bool RowLess(int a, int b)
{
	const FLAGGROUPINFO *pA = AdminFlagGroupGet (a), *pB = AdminFlagGroupGet (b);

	if (pA->byKind != pB->byKind) {
		return pA->byKind < pB->byKind;
	}
	if (pA->wSort != pB->wSort) {
		return pA->wSort < pB->wSort;
	}
	return pA->wID < pB->wID;
}

/* 種類の名前 */
static LPCSTR KindName(int nKind)
{
	return (nKind == FLAGKIND_BIT) ? "印" : "数値";
}

/* ========================================================================= */
/* 関数名：ShowDlgFlagGroup													 */
/* 内容：ダイアログ表示（開いたらサーバーへグループを問い合わせる）			 */
/* ========================================================================= */
void ShowDlgFlagGroup(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_bWait = FALSE;
	s_nSelIndex = -1;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_FLAGGROUP, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	AdminFlagGroupRequest ();
	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgFlagGroup												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgFlagGroup(void)
{
	if (s_hWndDlg) {
		AdminFlagTalkRemoveListener (OnChanged);
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_anRow.clear ();
	s_bWait = FALSE;
}

/* ========================================================================= */
/* 関数名：OnChanged														 */
/* 内容：グループ・書き込みの結果が届いた（更新番号が変わっていたら描き直す）*/
/* ========================================================================= */
static void OnChanged(void)
{
	const ADMINFLAGTALKRESULT *pRes = AdminFlagTalkGetResult ();
	char szMsg[MAX_ADMINFLAGGROUPMSG + 64];

	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwGroupSeqSeen != AdminFlagGroupGetSeq ()) {
		RenewList ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bWait && (pRes->byCmd == CMD_ADMINFLAGGROUP)) {
			s_bWait = FALSE;
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, FLAGGROUP_TITLE, MB_OK);
			}
			RenewList ();
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

	/* 枠を引っぱって大きさを変えた時は、一覧を広げる */
	case WM_SIZE:
		AdminAutoLayout (hWnd, FALSE);
		return 0;

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
	RECT rt;
	HWND hWndTmp;
	LV_COLUMN lvcol;

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	for (int i = 0; i < (int)(sizeof (s_aColumn) / sizeof (s_aColumn[0])); i ++) {
		lvcol.cx = s_aColumn[i].nWidth;
		lvcol.pszText = (LPSTR)s_aColumn[i].pszName;
		ListView_InsertColumn (hWndTmp, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	/* 絞り込み（種類）と入れる欄の種類 */
	SendDlgItemMessage (hWnd, IDC_FG_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"すべて");
	SendDlgItemMessage (hWnd, IDC_FG_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"印");
	SendDlgItemMessage (hWnd, IDC_FG_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"数値");
	SendDlgItemMessage (hWnd, IDC_FG_KINDFILTER, CB_SETCURSEL, 0, 0);
	SendDlgItemMessage (hWnd, IDC_FG_KIND, CB_ADDSTRING, 0, (LPARAM)"印");
	SendDlgItemMessage (hWnd, IDC_FG_KIND, CB_ADDSTRING, 0, (LPARAM)"数値");
	SendDlgItemMessage (hWnd, IDC_FG_KIND, CB_SETCURSEL, 0, 0);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	FLAGGROUPINFO Info;

	switch (nID) {
	case IDC_FG_KINDFILTER:		/* 種類で絞り込み */
		if (nCodeNotify == CBN_SELCHANGE) {
			RenewList ();
		}
		break;

	case IDC_FG_FIND:			/* メイン名で絞り込み（部分一致） */
		if (nCodeNotify == EN_CHANGE) {
			RenewList ();
		}
		break;

	case IDC_GET:	/* 取得（サーバーから取り直す） */
		AdminFlagGroupRequest ();
		RenewList ();
		break;

	case IDC_ADD:	/* 追加（グループ番号は自動） */
		if (s_bWait || !ReadFields (&Info)) {
			break;
		}
		SendSet (-1, &Info);
		break;

	case IDC_FG_UPDATE:	/* 更新（選んでいるグループを直す） */
		if (s_bWait) {
			break;
		}
		if (AdminFlagGroupGet (s_nSelIndex) == NULL) {
			MessageBox (hWnd, "直すグループを一覧から選んでください。", FLAGGROUP_TITLE, MB_OK);
			break;
		}
		if (!ReadFields (&Info)) {
			break;
		}
		SendSet (s_nSelIndex, &Info);
		break;

	case IDC_DELETE:	/* 削除 */
		{
			const FLAGGROUPINFO *pOld = AdminFlagGroupGet (s_nSelIndex);
			char szMsg[MAX_FLAGGROUPNAME + 256], szBands[160];

			if (s_bWait) {
				break;
			}
			if (pOld == NULL) {
				MessageBox (hWnd, "消すグループを一覧から選んでください。", FLAGGROUP_TITLE, MB_OK);
				break;
			}
			AdminFlagGroupBandsText (pOld, szBands, sizeof (szBands));
			snprintf (szMsg, sizeof (szMsg), "%s「%s」（%s）のグループを消します。\n番号そのものは変わりません。よろしいですか？",
					KindName (pOld->byKind), pOld->szName, szBands);
			if (MessageBox (hWnd, szMsg, "削除確認", MB_YESNO | MB_ICONQUESTION) != IDYES) {
				break;
			}
			Info = *pOld;
			Info.byState = FLAGGROUPSTATE_NONE;
			SendSet (s_nSelIndex, &Info);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnNotify															 */
/* 内容：メッセージハンドラ(WM_NOTIFY)。一覧で選んだら下の欄に出す			 */
/* ========================================================================= */
static void OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR pHdr = (LPNMHDR)lParam;
	NMLISTVIEW *pNm = (NMLISTVIEW *)lParam;

	if ((pHdr == NULL) || (pHdr->idFrom != IDC_LIST) || (pHdr->code != LVN_ITEMCHANGED)) {
		return;
	}
	if (((pNm->uNewState & LVIS_SELECTED) == 0) || (pNm->uOldState & LVIS_SELECTED)) {
		return;
	}
	if ((pNm->iItem < 0) || (pNm->iItem >= (int)s_anRow.size ())) {
		return;
	}
	/* 一覧を作り直した時にも通知が来るので、もう選んでいる物なら何もしない */
	if (s_anRow[pNm->iItem] == s_nSelIndex) {
		return;
	}
	s_nSelIndex = s_anRow[pNm->iItem];
	LoadFields (s_nSelIndex);
}

/* ========================================================================= */
/* 関数名：LoadFields														 */
/* 内容：選んだグループを下の欄に出す										 */
/* ========================================================================= */
static void LoadFields(int nIndex)
{
	const FLAGGROUPINFO *pInfo = AdminFlagGroupGet (nIndex);
	char szTmp[32];

	if (pInfo == NULL) {
		return;
	}
	SendDlgItemMessage (s_hWndDlg, IDC_FG_KIND, CB_SETCURSEL, (pInfo->byKind == FLAGKIND_NUM) ? 1 : 0, 0);
	SetDlgItemText (s_hWndDlg, IDC_FG_NAME, pInfo->szName);
	{
		char szBands[256];
		int nLen = 0;

		szBands[0] = 0;
		for (int b = 0; b < MAX_FLAGGROUPBAND; b ++) {
			if (FlagGroupBandUsed (pInfo, b) && (nLen < (int)sizeof (szBands) - 16)) {
				nLen += snprintf (szBands + nLen, sizeof (szBands) - nLen, "%s%u-%u", (nLen > 0) ? ", " : "", (unsigned)pInfo->awFrom[b], (unsigned)pInfo->awTo[b]);
			}
		}
		SetDlgItemText (s_hWndDlg, IDC_FG_BANDS, szBands);
	}
	snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wSort);
	SetDlgItemText (s_hWndDlg, IDC_FG_SORT, szTmp);
}

/* ========================================================================= */
/* 関数名：ReadFields														 */
/* 内容：下の欄を読む（おかしければ理由を出して FALSE）						 */
/*       選んでいるグループがあればグループ番号 wID は今の番号を引き継ぐ	 */
/*       （サーバー側は番号を書き戻す形で扱うので、ここでは使われない）		 */
/* ========================================================================= */
static BOOL ReadFields(FLAGGROUPINFO *pInfo)
{
	char szName[MAX_FLAGGROUPNAME + 64], szMsg[400];
	int nKind = (int)SendDlgItemMessage (s_hWndDlg, IDC_FG_KIND, CB_GETCURSEL, 0, 0);
	WORD awFrom[MAX_FLAGGROUPBAND], awTo[MAX_FLAGGROUPBAND];
	unsigned long ulSort;
	int nMax, nBandCount = 0;

	if (nKind < 0) {
		nKind = FLAGKIND_BIT;
	}
	nMax = (nKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;

	ZeroMemory (szName, sizeof (szName));
	GetDlgItemText (s_hWndDlg, IDC_FG_NAME, szName, sizeof (szName) - 1);
	if (szName[0] == 0) {
		MessageBox (s_hWndDlg, "メイン名を入れてください。（例：ルシー）", FLAGGROUP_TITLE, MB_OK);
		SetFocus (GetDlgItem (s_hWndDlg, IDC_FG_NAME));
		return FALSE;
	}
	if (strlen (szName) >= MAX_FLAGGROUPNAME) {
		snprintf (szMsg, sizeof (szMsg), "メイン名が長すぎます。\n（UTF-8 で %d バイトまで。全角ならおよそ %d 文字まで）",
				MAX_FLAGGROUPNAME - 1, (MAX_FLAGGROUPNAME - 1) / 3);
		MessageBox (s_hWndDlg, szMsg, FLAGGROUP_TITLE, MB_OK);
		SetFocus (GetDlgItem (s_hWndDlg, IDC_FG_NAME));
		return FALSE;
	}

	/* 番号の帯（端も含む）。印は 0〜1023・数値は 0〜127。カンマ区切りで 16 本まで（同じグループの帯同士の重なりもここで確かめる。
	   ほかのグループとの重なりはサーバーが確かめる） */
	{
		char szText[512];

		GetDlgItemText (s_hWndDlg, IDC_FG_BANDS, szText, sizeof (szText));
		if (!FlagGroupParseBands (szText, nMax, awFrom, awTo, nBandCount, szMsg, sizeof (szMsg))) {
			MessageBox (s_hWndDlg, szMsg, FLAGGROUP_TITLE, MB_OK);
			SetFocus (GetDlgItem (s_hWndDlg, IDC_FG_BANDS));
			return FALSE;
		}
	}
	if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_FG_SORT, 65535, "並び", FLAGGROUP_TITLE, ulSort)) {
		return FALSE;
	}

	ZeroMemory (pInfo, sizeof (FLAGGROUPINFO));
	FlagGroupClearBands (pInfo);
	pInfo->byState = FLAGGROUPSTATE_ON;
	pInfo->byKind = (BYTE)nKind;
	strncpy (pInfo->szName, szName, sizeof (pInfo->szName) - 1);
	for (int b = 0; b < nBandCount; b ++) {
		pInfo->awFrom[b] = awFrom[b];
		pInfo->awTo[b] = awTo[b];
	}
	pInfo->wSort = (WORD)ulSort;
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SendSet															 */
/* 内容：書き込みを送る（結果が届くまでは次の書き込みをさせない）			 */
/* ========================================================================= */
static void SendSet(int nNo, const FLAGGROUPINFO *pInfo)
{
	s_bWait = TRUE;
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	SetDlgItemText (s_hWndDlg, IDC_FG_MSG, "書き込み中…");
	AdminFlagGroupSend (nNo, pInfo);
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新（種類・メイン名で絞り込み、種類・並び順・番号の順に並べる）*/
/* ========================================================================= */
static void RenewList(void)
{
	HWND hWndTmp;
	LV_ITEM item;
	char szTmp[200], szKey[128], szBands[160];
	int i, nKindSel, nCount, nSelRow = -1, nBit = 0, nNum = 0;
	std::vector<int> anAll;

	if (s_hWndDlg == NULL) {
		return;
	}
	s_dwGroupSeqSeen = AdminFlagGroupGetSeq ();
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nKindSel = (int)SendDlgItemMessage (s_hWndDlg, IDC_FG_KINDFILTER, CB_GETCURSEL, 0, 0);	/* 0=すべて 1=印 2=数値 */
	szKey[0] = 0;
	GetDlgItemText (s_hWndDlg, IDC_FG_FIND, szKey, sizeof (szKey));

	/* 有効なグループ（削除済みの枠は出さない）を、全体の件数に数える */
	nCount = AdminFlagGroupGetCount ();
	for (i = 0; i < nCount; i ++) {
		const FLAGGROUPINFO *pInfo = AdminFlagGroupGet (i);
		if ((pInfo == NULL) || (pInfo->byState != FLAGGROUPSTATE_ON)) {
			continue;
		}
		if (pInfo->byKind == FLAGKIND_BIT) {
			nBit ++;
		} else {
			nNum ++;
		}
		if ((nKindSel > 0) && (pInfo->byKind != nKindSel - 1)) {
			continue;
		}
		if (!AdminFilterMatch (pInfo->szName, szKey)) {
			continue;
		}
		anAll.push_back (i);
	}
	std::sort (anAll.begin (), anAll.end (), RowLess);
	s_anRow = anAll;

	ListView_DeleteAllItems (hWndTmp);
	for (i = 0; i < (int)s_anRow.size (); i ++) {
		const FLAGGROUPINFO *pInfo = AdminFlagGroupGet (s_anRow[i]);

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wID);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);
		ListView_SetItemText (hWndTmp, i, 1, (LPSTR)KindName (pInfo->byKind));
		ListView_SetItemText (hWndTmp, i, 2, (LPSTR)pInfo->szName);
		AdminFlagGroupBandsText (pInfo, szBands, sizeof (szBands));
		snprintf (szTmp, sizeof (szTmp), "%s（%d 個）", szBands, FlagGroupSubCount (pInfo));
		ListView_SetItemText (hWndTmp, i, 3, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wSort);
		ListView_SetItemText (hWndTmp, i, 4, szTmp);
		if (s_anRow[i] == s_nSelIndex) {
			nSelRow = i;
		}
	}
	if (nSelRow >= 0) {
		ListView_SetItemState (hWndTmp, nSelRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}

	if (!s_bWait) {
		snprintf (szTmp, sizeof (szTmp), "グループの数：印 %d・数値 %d（いま %d 件を表示）", nBit, nNum, (int)s_anRow.size ());
		SetDlgItemText (s_hWndDlg, IDC_FG_MSG, szTmp);
	}
}

/* Copyright(C)URARA-works 2026 */
