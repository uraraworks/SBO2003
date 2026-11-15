/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		会話データダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgTalk.cpp											 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * NPC の台詞を "@t番号" にすると使われる会話セットを、管理者ウィンドウで
 * 見て直すための一覧画面。リソースは IDD_TALKINFO。
 * 会話データはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * 届いたものを AdminFlagTalk.cpp が覚えている。届くたびに更新番号が変わるので、
 * 変わっていたら一覧を描き直す。
 * ・編集／追加：編集画面（AdminDlgTalkEdit.cpp）を出す
 * ・削除：選んでいる会話セットを消す（サーバー側は「削除済みの枠」として残る）
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminListFilter.h"
#include "AdminFlagTalk.h"
#include "AdminDlgTalk.h"
#include "AdminDlgTalkEdit.h"
#include <vector>
#include <algorithm>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define TALK_TITLE		"会話データ"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static DWORD s_dwTalkSeqSeen = 0;								/* 描き直した時の会話データの更新番号 */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */
static BOOL s_bWait = FALSE;									/* 削除の結果を待っている */
static std::vector<int> s_anRow;								/* 一覧の行 → 会話セットの配列の番号 */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号(@t)", 60}, {"名前", 210}, {"行数", 40},
};

/* 並べ方（会話セット番号の順） */
static bool RowLess(int a, int b)
{
	return AdminTalkGet (a)->wID < AdminTalkGet (b)->wID;
}

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnChanged(void);													/* 会話データ・書き込みの結果が届いた */
static void RenewList(void);													/* リスト更新 */
static int GetSelIndex(HWND hWnd);												/* 一覧で選んでいる会話セットの配列の番号（無ければ -1） */

/* ========================================================================= */
/* 関数名：ShowDlgTalk														 */
/* 内容：ダイアログ表示（開いたらサーバーへ会話データを問い合わせる）		 */
/* ========================================================================= */
void ShowDlgTalk(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_bWait = FALSE;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_TALKINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	AdminFlagNameLoadIfEmpty ();							/* 編集画面で印・数値の名前を出すため */
	AdminTalkRequest ();
	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgTalk														 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgTalk(void)
{
	CloseDlgTalkEdit ();
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
/* 内容：会話データ・書き込みの結果が届いた（更新番号が変わっていたら描き直す）	 */
/* ========================================================================= */
static void OnChanged(void)
{
	const ADMINFLAGTALKRESULT *pRes = AdminFlagTalkGetResult ();
	char szMsg[MAX_ADMINQUESTMSG + 64];

	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwTalkSeqSeen != AdminTalkGetSeq ()) {
		RenewList ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bWait && (pRes->byCmd == CMD_ADMINTALK)) {
			s_bWait = FALSE;
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "消せませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, TALK_TITLE, MB_OK);
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

	/* 枠を引っぱって大きさを変えた時は、一覧を広げる */
	case WM_SIZE:
		AdminAutoLayout (hWnd, FALSE);
		return 0;

	case WM_NOTIFY:
		/* 一覧をダブルクリックしたら編集 */
		{
			LPNMHDR pHdr = (LPNMHDR)lParam;
			if (pHdr && (pHdr->idFrom == IDC_LIST) && (pHdr->code == NM_DBLCLK)) {
				PostMessage (hWnd, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
			}
		}
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

	return FALSE;
}

/* ========================================================================= */
/* 関数名：GetSelIndex														 */
/* 内容：一覧で選んでいる会話セットの配列の番号（無ければ -1）				 */
/* ========================================================================= */
static int GetSelIndex(HWND hWnd)
{
	int nRow = ListView_GetNextItem (GetDlgItem (hWnd, IDC_LIST), -1, LVNI_ALL | LVNI_SELECTED);

	if ((nRow < 0) || (nRow >= (int)s_anRow.size ())) {
		return -1;
	}
	return s_anRow[nRow];
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	int nIndex = GetSelIndex (hWnd);

	switch (nID) {
	case IDC_TK_FIND:	/* 名前・番号で絞り込み（部分一致） */
		if (nCodeNotify == EN_CHANGE) {
			RenewList ();
		}
		break;

	case IDC_GET:	/* 取得（サーバーから取り直す） */
		if (IsDlgTalkEditOpen ()) {
			MessageBox (hWnd, "編集画面を閉じてから取得してください。", TALK_TITLE, MB_OK);
			break;
		}
		AdminFlagNameRequest ();
		AdminTalkRequest ();
		RenewList ();
		break;

	case IDC_EDIT:	/* 編集 */
		if (AdminTalkGet (nIndex) == NULL) {
			MessageBox (hWnd, "編集する会話セットを一覧から選んでください。", TALK_TITLE, MB_OK);
			break;
		}
		ShowDlgTalkEdit (hWnd, nIndex);
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgTalkEdit (hWnd, -1);
		break;

	case IDC_DELETE:	/* 削除 */
		{
			const TALKINFO *pOld = AdminTalkGet (nIndex);
			char szMsg[MAX_TALKNAME + 96];
			TALKINFO Info;

			if (s_bWait) {
				break;
			}
			if (IsDlgTalkEditOpen ()) {
				MessageBox (hWnd, "編集画面を閉じてから削除してください。", TALK_TITLE, MB_OK);
				break;
			}
			if (pOld == NULL) {
				MessageBox (hWnd, "消す会話セットを一覧から選んでください。", TALK_TITLE, MB_OK);
				break;
			}
			snprintf (szMsg, sizeof (szMsg), "会話セット %u「%s」を消します。\nこのセットを使っているNPCは、話しかけても何も言わなくなります。\nよろしいですか？",
					(unsigned)pOld->wID, pOld->szName);
			if (MessageBox (hWnd, szMsg, "削除確認", MB_YESNO | MB_ICONQUESTION) != IDYES) {
				break;
			}
			Info = *pOld;
			Info.byState = TALKSTATE_NONE;
			s_bWait = TRUE;
			s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
			AdminTalkSend (nIndex, &Info);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新（有効な会話セットを番号順に。名前・番号で絞り込み）		 */
/* ========================================================================= */
static void RenewList(void)
{
	HWND hWndTmp;
	char szTmp[160], szKey[128];
	int i, nCount, nSel, nSelRow = -1, nAll = 0;

	if (s_hWndDlg == NULL) {
		return;
	}
	s_dwTalkSeqSeen = AdminTalkGetSeq ();
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nSel = GetSelIndex (s_hWndDlg);
	szKey[0] = 0;
	GetDlgItemText (s_hWndDlg, IDC_TK_FIND, szKey, sizeof (szKey));

	/* 有効な会話セットを集め、番号の順に並べる */
	s_anRow.clear ();
	nCount = AdminTalkGetCount ();
	for (i = 0; i < nCount; i ++) {
		const TALKINFO *pInfo = AdminTalkGet (i);

		if ((pInfo == NULL) || (pInfo->byState != TALKSTATE_ON)) {
			continue;
		}
		nAll ++;
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wID);
		if (!AdminFilterMatch (pInfo->szName, szKey) && (strstr (szTmp, szKey) == NULL)) {
			continue;
		}
		s_anRow.push_back (i);
	}
	std::sort (s_anRow.begin (), s_anRow.end (), RowLess);

	ListView_DeleteAllItems (hWndTmp);
	for (i = 0; i < (int)s_anRow.size (); i ++) {
		const TALKINFO *pInfo = AdminTalkGet (s_anRow[i]);
		LV_ITEM item;

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wID);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);
		ListView_SetItemText (hWndTmp, i, 1, (LPSTR)pInfo->szName);
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->byLineCount);
		ListView_SetItemText (hWndTmp, i, 2, szTmp);
		if (s_anRow[i] == nSel) {
			nSelRow = i;
		}
	}
	if (nSelRow >= 0) {
		ListView_SetItemState (hWndTmp, nSelRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}

	snprintf (szTmp, sizeof (szTmp), "会話セット %d 件（いま %d 件を表示）", nAll, (int)s_anRow.size ());
	SetDlgItemText (s_hWndDlg, IDC_TK_MSG, szTmp);
}

/* Copyright(C)URARA-works 2026 */
