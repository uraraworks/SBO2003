/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		フラグの名前ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgFlagName.cpp										 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * キャラごとのフラグ（印 0〜1023・数値 0〜127）に付ける名前の表を、管理者ウィンドウで
 * 見て直すための画面。リソースは IDD_FLAGNAME。
 * 名前表はサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * 届いたものを AdminFlagTalk.cpp が覚えている。届くたびに更新番号が変わるので、
 * 変わっていたら一覧を描き直す。
 * ・一覧：種類（印・数値）と名前の部分一致で絞り込める。選ぶと下の欄に出る
 * ・追加：下の欄の種類・番号・名前で足す（削除済みの枠に同じ番号があれば、その枠を使う）
 * ・更新：選んでいる名前を、下の欄の内容に直す
 * ・削除：選んでいる名前を消す（サーバー側は「削除済みの枠」として残る）
 * 書き込みはデータAPIと同じ確認を通り、だめならサーバーからの理由を出す。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminListFilter.h"
#include "AdminFlagTalk.h"
#include "AdminDlgFlagName.h"
#include <algorithm>
#include <vector>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define FLAGNAME_TITLE		"フラグの名前"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static DWORD s_dwNameSeqSeen = 0;								/* 描き直した時の名前表の更新番号 */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static std::vector<int> s_anRow;								/* 一覧の行 → 名前表の配列の番号 */
static int s_nSelIndex = -1;									/* 選んでいる名前表の配列の番号 */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"種類", 50}, {"番号", 45}, {"名前", 220},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnNotify(HWND hWnd, LPARAM lParam);									/* メッセージハンドラ(WM_NOTIFY) */
static void OnChanged(void);													/* 名前表・書き込みの結果が届いた */
static void RenewList(void);													/* リスト更新 */
static void LoadFields(int nIndex);												/* 選んだ名前を下の欄に出す */
static BOOL ReadFields(FLAGNAMEINFO *pInfo);									/* 下の欄を読む */
static void SendSet(int nNo, const FLAGNAMEINFO *pInfo);						/* 書き込みを送る */

/* 並べ方（種類・番号の順） */
static bool RowLess(int a, int b)
{
	const FLAGNAMEINFO *pA = AdminFlagNameGet (a), *pB = AdminFlagNameGet (b);

	if (pA->byKind != pB->byKind) {
		return pA->byKind < pB->byKind;
	}
	return pA->wNo < pB->wNo;
}

/* 種類の名前 */
static LPCSTR KindName(int nKind)
{
	return (nKind == FLAGKIND_BIT) ? "印" : "数値";
}

/* ========================================================================= */
/* 関数名：ShowDlgFlagName													 */
/* 内容：ダイアログ表示（開いたらサーバーへ名前表を問い合わせる）			 */
/* ========================================================================= */
void ShowDlgFlagName(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_bWait = FALSE;
	s_nSelIndex = -1;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_FLAGNAME, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	AdminFlagNameRequest ();
	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgFlagName													 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgFlagName(void)
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
/* 内容：名前表・書き込みの結果が届いた（更新番号が変わっていたら描き直す）	 */
/* ========================================================================= */
static void OnChanged(void)
{
	const ADMINFLAGTALKRESULT *pRes = AdminFlagTalkGetResult ();
	char szMsg[MAX_ADMINQUESTMSG + 64];

	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwNameSeqSeen != AdminFlagNameGetSeq ()) {
		RenewList ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bWait && (pRes->byCmd == CMD_ADMINFLAGNAME)) {
			s_bWait = FALSE;
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, FLAGNAME_TITLE, MB_OK);
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
	SendDlgItemMessage (hWnd, IDC_FN_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"すべて");
	SendDlgItemMessage (hWnd, IDC_FN_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"印");
	SendDlgItemMessage (hWnd, IDC_FN_KINDFILTER, CB_ADDSTRING, 0, (LPARAM)"数値");
	SendDlgItemMessage (hWnd, IDC_FN_KINDFILTER, CB_SETCURSEL, 0, 0);
	SendDlgItemMessage (hWnd, IDC_FN_KIND, CB_ADDSTRING, 0, (LPARAM)"印");
	SendDlgItemMessage (hWnd, IDC_FN_KIND, CB_ADDSTRING, 0, (LPARAM)"数値");
	SendDlgItemMessage (hWnd, IDC_FN_KIND, CB_SETCURSEL, 0, 0);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	FLAGNAMEINFO Info;
	int nIndex;

	switch (nID) {
	case IDC_FN_KINDFILTER:		/* 種類で絞り込み */
		if (nCodeNotify == CBN_SELCHANGE) {
			RenewList ();
		}
		break;

	case IDC_FN_FIND:			/* 名前で絞り込み（部分一致） */
		if (nCodeNotify == EN_CHANGE) {
			RenewList ();
		}
		break;

	case IDC_GET:	/* 取得（サーバーから取り直す） */
		AdminFlagNameRequest ();
		RenewList ();
		break;

	case IDC_ADD:	/* 追加（同じ種類・番号が削除済みなら、その枠を使う） */
		if (s_bWait || !ReadFields (&Info)) {
			break;
		}
		nIndex = AdminFlagNameFindIndex (Info.byKind, Info.wNo);
		if (nIndex >= 0) {
			const FLAGNAMEINFO *pOld = AdminFlagNameGet (nIndex);
			if (pOld && (pOld->byState == FLAGNAMESTATE_ON)) {
				char szMsg[MAX_FLAGNAME + 96];
				snprintf (szMsg, sizeof (szMsg), "%s %d には、もう「%s」という名前があります。\n直す時は一覧から選んで「更新」を押してください。",
						KindName (Info.byKind), (int)Info.wNo, pOld->szName);
				MessageBox (hWnd, szMsg, FLAGNAME_TITLE, MB_OK);
				break;
			}
		}
		SendSet (nIndex, &Info);
		break;

	case IDC_FN_UPDATE:	/* 更新（選んでいる名前を直す） */
		if (s_bWait) {
			break;
		}
		if (AdminFlagNameGet (s_nSelIndex) == NULL) {
			MessageBox (hWnd, "直す名前を一覧から選んでください。", FLAGNAME_TITLE, MB_OK);
			break;
		}
		if (!ReadFields (&Info)) {
			break;
		}
		SendSet (s_nSelIndex, &Info);
		break;

	case IDC_DELETE:	/* 削除 */
		{
			const FLAGNAMEINFO *pOld = AdminFlagNameGet (s_nSelIndex);
			char szMsg[MAX_FLAGNAME + 96];

			if (s_bWait) {
				break;
			}
			if (pOld == NULL) {
				MessageBox (hWnd, "消す名前を一覧から選んでください。", FLAGNAME_TITLE, MB_OK);
				break;
			}
			snprintf (szMsg, sizeof (szMsg), "%s %d「%s」の名前を消します。\nよろしいですか？",
					KindName (pOld->byKind), (int)pOld->wNo, pOld->szName);
			if (MessageBox (hWnd, szMsg, "削除確認", MB_YESNO | MB_ICONQUESTION) != IDYES) {
				break;
			}
			Info = *pOld;
			Info.byState = FLAGNAMESTATE_NONE;
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
/* 内容：選んだ名前を下の欄に出す											 */
/* ========================================================================= */
static void LoadFields(int nIndex)
{
	const FLAGNAMEINFO *pInfo = AdminFlagNameGet (nIndex);
	char szTmp[32];

	if (pInfo == NULL) {
		return;
	}
	SendDlgItemMessage (s_hWndDlg, IDC_FN_KIND, CB_SETCURSEL, (pInfo->byKind == FLAGKIND_NUM) ? 1 : 0, 0);
	snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wNo);
	SetDlgItemText (s_hWndDlg, IDC_FN_NO, szTmp);
	SetDlgItemText (s_hWndDlg, IDC_FN_NAME, pInfo->szName);
}

/* ========================================================================= */
/* 関数名：ReadFields														 */
/* 内容：下の欄を読む（おかしければ理由を出して FALSE）						 */
/* ========================================================================= */
static BOOL ReadFields(FLAGNAMEINFO *pInfo)
{
	char szNo[32], szName[MAX_FLAGNAME + 64], szMsg[160];
	int nKind = (int)SendDlgItemMessage (s_hWndDlg, IDC_FN_KIND, CB_GETCURSEL, 0, 0);
	unsigned long ulNo;
	int nMax;

	if (nKind < 0) {
		nKind = FLAGKIND_BIT;
	}
	nMax = (nKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;

	ZeroMemory (szNo, sizeof (szNo));
	GetDlgItemText (s_hWndDlg, IDC_FN_NO, szNo, sizeof (szNo) - 1);
	ulNo = strtoul (szNo, NULL, 10);
	if ((szNo[0] == 0) || (ulNo >= (unsigned long)nMax)) {
		snprintf (szMsg, sizeof (szMsg), "%sの番号は 0〜%d で入れてください。", KindName (nKind), nMax - 1);
		MessageBox (s_hWndDlg, szMsg, FLAGNAME_TITLE, MB_OK);
		SetFocus (GetDlgItem (s_hWndDlg, IDC_FN_NO));
		return FALSE;
	}

	ZeroMemory (szName, sizeof (szName));
	GetDlgItemText (s_hWndDlg, IDC_FN_NAME, szName, sizeof (szName) - 1);
	if (szName[0] == 0) {
		MessageBox (s_hWndDlg, "名前を入れてください。", FLAGNAME_TITLE, MB_OK);
		SetFocus (GetDlgItem (s_hWndDlg, IDC_FN_NAME));
		return FALSE;
	}
	if (strlen (szName) >= MAX_FLAGNAME) {
		snprintf (szMsg, sizeof (szMsg), "名前が長すぎます。\n（UTF-8 で %d バイトまで。全角ならおよそ %d 文字まで）",
				MAX_FLAGNAME - 1, (MAX_FLAGNAME - 1) / 3);
		MessageBox (s_hWndDlg, szMsg, FLAGNAME_TITLE, MB_OK);
		SetFocus (GetDlgItem (s_hWndDlg, IDC_FN_NAME));
		return FALSE;
	}

	ZeroMemory (pInfo, sizeof (FLAGNAMEINFO));
	pInfo->byState = FLAGNAMESTATE_ON;
	pInfo->byKind = (BYTE)nKind;
	pInfo->wNo = (WORD)ulNo;
	strncpy (pInfo->szName, szName, sizeof (pInfo->szName) - 1);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SendSet															 */
/* 内容：書き込みを送る（結果が届くまでは次の書き込みをさせない）			 */
/* ========================================================================= */
static void SendSet(int nNo, const FLAGNAMEINFO *pInfo)
{
	s_bWait = TRUE;
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	SetDlgItemText (s_hWndDlg, IDC_FN_MSG, "書き込み中…");
	AdminFlagNameSend (nNo, pInfo);
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新（種類・名前で絞り込み、種類・番号の順に並べる）			 */
/* ========================================================================= */
static void RenewList(void)
{
	HWND hWndTmp;
	LV_ITEM item;
	char szTmp[160], szKey[128];
	int i, nKindSel, nCount, nSelRow = -1, nBit = 0, nNum = 0;
	std::vector<int> anAll;

	if (s_hWndDlg == NULL) {
		return;
	}
	s_dwNameSeqSeen = AdminFlagNameGetSeq ();
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nKindSel = (int)SendDlgItemMessage (s_hWndDlg, IDC_FN_KINDFILTER, CB_GETCURSEL, 0, 0);	/* 0=すべて 1=印 2=数値 */
	szKey[0] = 0;
	GetDlgItemText (s_hWndDlg, IDC_FN_FIND, szKey, sizeof (szKey));

	/* 有効な名前（削除済みの枠は出さない）を、全体の件数に数える */
	nCount = AdminFlagNameGetCount ();
	for (i = 0; i < nCount; i ++) {
		const FLAGNAMEINFO *pInfo = AdminFlagNameGet (i);
		if ((pInfo == NULL) || (pInfo->byState != FLAGNAMESTATE_ON)) {
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
		const FLAGNAMEINFO *pInfo = AdminFlagNameGet (s_anRow[i]);

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = (LPSTR)KindName (pInfo->byKind);
		ListView_InsertItem (hWndTmp, &item);
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wNo);
		ListView_SetItemText (hWndTmp, i, 1, szTmp);
		ListView_SetItemText (hWndTmp, i, 2, (LPSTR)pInfo->szName);
		if (s_anRow[i] == s_nSelIndex) {
			nSelRow = i;
		}
	}
	if (nSelRow >= 0) {
		ListView_SetItemState (hWndTmp, nSelRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}

	if (!s_bWait) {
		snprintf (szTmp, sizeof (szTmp), "名前の数：印 %d・数値 %d（いま %d 件を表示）", nBit, nNum, (int)s_anRow.size ());
		SetDlgItemText (s_hWndDlg, IDC_FN_MSG, szTmp);
	}
}

/* Copyright(C)URARA-works 2026 */
