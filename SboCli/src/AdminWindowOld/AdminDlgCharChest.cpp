/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		キャラの開けた宝箱ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgCharChest.cpp										 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 管理者ウィンドウで選んでいるキャラ（プレイヤーだけ）の「開けた宝箱の記録」を、
 * 全宝箱の一覧で見て、直すための画面。リソースは IDD_CHARCHEST。
 * キャラのフラグ画面（AdminDlgCharFlag.cpp）の「宝箱」ボタンから開く。
 * 宝箱の記録は印（フラグ）とは別で、サーバーだけが持つ。開いた時（と「取得」）にサーバーへ
 * 問い合わせ、届いたものを AdminFlagTalk.cpp が覚えている。届くたびに更新番号が変わるので、
 * 変わっていたら一覧を描き直す。
 * ・一覧：宝箱番号の順に、番号・マップ・座標・品・メモと、そのキャラが開けたか（○）を出す。
 *         消した宝箱は出ない。一覧が長ければスクロールする（印の一覧と同じ操作）
 * ・行を選んで「開けた」「開けていない」で、その宝箱の記録を変える
 * ・「全部開けていない」は、確認を出してから全宝箱の記録を戻す
 * ・「開けていない宝箱だけ」にチェックすると、まだ開けていない宝箱だけに絞る
 * 変えると、そのキャラがログイン中ならサーバーが差し替えを送り直し、宝箱の見た目がすぐ変わる。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminFlagTalk.h"
#include "AdminDlgCharChest.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define CHARCHEST_TITLE		"キャラの開けた宝箱"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static DWORD s_dwTblID = 0;										/* 見ているキャラのテーブルID */
static char s_szCharName[64];									/* 見ているキャラの名前 */
static BOOL s_bHave = FALSE;									/* このキャラの記録が届いているか */
static BYTE s_abyOpened[MAX_CHESTBYTE];							/* 届いている記録 */
static DWORD s_dwSeqSeen = 0;									/* 描き直した時の宝箱の更新番号 */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */
static BOOL s_bPending = FALSE;									/* 変更の結果を待っている */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnChanged(void);													/* 宝箱の一覧・記録・結果が届いた */
static void RenewList(void);													/* 一覧を作り直す */
static BOOL IsOpened(int nChestNo);												/* 開けているか */
static int  GetSelectedChestNo(void);											/* 一覧で選んでいる宝箱の番号（無ければ -1） */

/* ========================================================================= */
/* 関数名：ShowDlgCharChest													 */
/* 内容：ダイアログ表示（開いたらサーバーへ全宝箱の一覧とそのキャラの記録を問い合わせる）*/
/* ========================================================================= */
void ShowDlgCharChest(HWND hWndParent, DWORD dwTblID, LPCSTR pszCharName)
{
	if (s_hWndDlg) {
		CloseDlgCharChest ();		/* 開いていたら、いまのキャラで開き直す */
	}

	s_dwTblID = dwTblID;
	strncpy (s_szCharName, pszCharName ? pszCharName : "", sizeof (s_szCharName) - 1);
	s_szCharName[sizeof (s_szCharName) - 1] = 0;
	s_bHave = FALSE;
	ZeroMemory (s_abyOpened, sizeof (s_abyOpened));
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_CHARCHEST, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	s_dwSeqSeen = AdminChestGetSeq ();
	s_bPending = TRUE;
	AdminChestRequest (s_dwTblID);
	RenewList ();

	/* キャラのフラグの画面に少し重ねて出す */
	{
		RECT rc;
		GetWindowRect (hWndParent ? hWndParent : g_hWndAdmin, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgCharChest												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgCharChest(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	AdminFlagTalkRemoveListener (OnChanged);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_bPending = FALSE;
}

/* ========================================================================= */
/* 関数名：IsOpened															 */
/* 内容：開けているか（届いた記録を見る）									 */
/* ========================================================================= */
static BOOL IsOpened(int nChestNo)
{
	if (!s_bHave || (nChestNo < 0) || (nChestNo >= MAX_CHESTNO)) {
		return FALSE;
	}
	return (s_abyOpened[nChestNo / 8] & (1 << (nChestNo % 8))) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：GetSelectedChestNo												 */
/* 内容：一覧で選んでいる宝箱の番号（選んでいなければ -1）					 */
/* ========================================================================= */
static int GetSelectedChestNo(void)
{
	HWND hWndList = GetDlgItem (s_hWndDlg, IDC_CC_LIST);
	int nRow = ListView_GetNextItem (hWndList, -1, LVNI_ALL | LVNI_SELECTED);
	LV_ITEM item;

	if (nRow < 0) {
		return -1;
	}
	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_PARAM;
	item.iItem = nRow;
	if (!ListView_GetItem (hWndList, &item)) {
		return -1;
	}
	return (int)item.lParam;
}

/* ========================================================================= */
/* 関数名：OnChanged														 */
/* 内容：宝箱の一覧・記録・書き込みの結果が届いた（更新番号が変わっていたら描き直す）*/
/* ========================================================================= */
static void OnChanged(void)
{
	const ADMINFLAGTALKRESULT *pRes = AdminFlagTalkGetResult ();
	char szMsg[MAX_ADMINQUESTMSG + 64];
	DWORD dwTblID;
	BYTE abyOpened[MAX_CHESTBYTE];

	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwSeqSeen != AdminChestGetSeq ()) {
		s_dwSeqSeen = AdminChestGetSeq ();
		if (AdminChestGetOpened (&dwTblID, abyOpened) && (dwTblID == s_dwTblID)) {
			CopyMemory (s_abyOpened, abyOpened, sizeof (s_abyOpened));
			s_bHave = TRUE;
		}
		RenewList ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bPending && (pRes->byCmd == CMD_ADMINCHEST)) {
			s_bPending = FALSE;
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "変えられませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, CHARCHEST_TITLE, MB_OK);
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
		CloseDlgCharChest ();
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
	char szTmp[128];
	HWND hWndList;
	LV_COLUMN lvcol;
	static const struct { int nWidth; LPCSTR pszText; } aCol[] = {
		{ 44, "番号" }, { 120, "マップ" }, { 64, "座標" }, { 120, "品" }, { 170, "メモ" }, { 44, "開けた" },
	};

	s_hWndDlg = hWnd;		/* この後の小物が使う（CreateDialog から戻る前に呼ばれる） */

	snprintf (szTmp, sizeof (szTmp), "キャラの開けた宝箱（%s）", s_szCharName);
	SetWindowText (hWnd, szTmp);
	snprintf (szTmp, sizeof (szTmp), "キャラ：%s（テーブルID %lu）", s_szCharName, (unsigned long)s_dwTblID);
	SetDlgItemText (hWnd, IDC_CC_WHO, szTmp);

	/* 宝箱の一覧（列の境目をドラッグで変えられる。長ければスクロールする） */
	hWndList = GetDlgItem (hWnd, IDC_CC_LIST);
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	for (int i = 0; i < (int)(sizeof (aCol) / sizeof (aCol[0])); i ++) {
		lvcol.cx = aCol[i].nWidth;
		lvcol.pszText = (LPSTR)aCol[i].pszText;
		ListView_InsertColumn (hWndList, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	int nNo;

	switch (nID) {
	case IDC_CC_GET:	/* 取得（サーバーから取り直す） */
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminChestRequest (s_dwTblID);
		break;

	case IDC_CC_ONLY:	/* 開けていない宝箱だけ */
		RenewList ();
		break;

	case IDC_CC_OPEN:	/* 選んだ宝箱を「開けた」にする */
	case IDC_CC_CLOSE:	/* 選んだ宝箱を「開けていない」にする */
		nNo = GetSelectedChestNo ();
		if (nNo < 0) {
			MessageBox (hWnd, "一覧から宝箱を選んでください。", CHARCHEST_TITLE, MB_OK);
			break;
		}
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminChestSet (s_dwTblID, (WORD)nNo, (nID == IDC_CC_OPEN) ? TRUE : FALSE);
		break;

	case IDC_CC_CLEAR:	/* 全部「開けていない」に戻す（確認してから） */
		if (MessageBox (hWnd, "このキャラの開けた宝箱の記録を、全部「開けていない」に戻します。\nよろしいですか？",
				CHARCHEST_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) {
			break;
		}
		s_bPending = TRUE;
		s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
		AdminChestClear (s_dwTblID);
		break;

	case IDCANCEL:
		CloseDlgCharChest ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：宝箱の一覧を作り直す（選んでいた宝箱は選んだままにする）			 */
/* ========================================================================= */
static void RenewList(void)
{
	HWND hWndList;
	LV_ITEM item;
	char szTmp[128];
	BOOL bOnly;
	int i, nRow = 0, nOpened = 0, nSel, nCount = AdminChestGetCount ();

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndList = GetDlgItem (s_hWndDlg, IDC_CC_LIST);
	bOnly = (IsDlgButtonChecked (s_hWndDlg, IDC_CC_ONLY) == BST_CHECKED) ? TRUE : FALSE;
	nSel = GetSelectedChestNo ();

	ListView_DeleteAllItems (hWndList);
	for (i = 0; i < nCount; i ++) {
		const ADMINCHESTROW *pRow = AdminChestGetRow (i);
		BOOL bOpened = IsOpened (pRow->wChestNo);

		if (bOpened) {
			nOpened ++;
		}
		if (bOnly && bOpened) {
			continue;
		}
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = nRow;
		item.iSubItem = 0;
		item.lParam = pRow->wChestNo;
		snprintf (szTmp, sizeof (szTmp), "%d", (int)pRow->wChestNo);
		item.pszText = szTmp;
		ListView_InsertItem (hWndList, &item);
		if (pRow->szMap[0]) {
			snprintf (szTmp, sizeof (szTmp), "%s（%d）", pRow->szMap, (int)pRow->wMapNo);
		} else {
			snprintf (szTmp, sizeof (szTmp), "マップ%d", (int)pRow->wMapNo);
		}
		ListView_SetItemText (hWndList, nRow, 1, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%d, %d", (int)pRow->wX, (int)pRow->wY);
		ListView_SetItemText (hWndList, nRow, 2, szTmp);
		ListView_SetItemText (hWndList, nRow, 3, (LPSTR)(pRow->szItem[0] ? pRow->szItem : (pRow->wGiveItem ? "（品の番号だけ）" : "（無し）")));
		ListView_SetItemText (hWndList, nRow, 4, (LPSTR)pRow->szMemo);
		ListView_SetItemText (hWndList, nRow, 5, (LPSTR)(bOpened ? "○" : ""));
		if (pRow->wChestNo == nSel) {
			ListView_SetItemState (hWndList, nRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
		nRow ++;
	}

	if (s_bHave) {
		snprintf (szTmp, sizeof (szTmp), "宝箱 %d 個のうち、開けた物 %d 個", nCount, nOpened);
	} else {
		snprintf (szTmp, sizeof (szTmp), "取得しています…");
	}
	SetDlgItemText (s_hWndDlg, IDC_CC_MSG, szTmp);
}

/* Copyright(C)URARA-works 2026 */
