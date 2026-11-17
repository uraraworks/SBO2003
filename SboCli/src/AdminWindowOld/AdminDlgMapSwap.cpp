/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		印で変わるマス・宝箱ダイアログ処理実装ファイル				 */
/* ファイル名：	AdminDlgMapSwap.cpp											 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 管理者ウィンドウの「マップ」→「印で変わるマス・宝箱」。今いるマップの差し替え表
 * （印で変わるマス・宝箱・リサイクル箱。MAPINFO の aSwap）を一覧で見て、1 行ずつ足す・直す・消す。
 * リソースは IDD_ADMINSWAP。通信は CMD_ADMINSWAP（サーバーは SboSvrRecvCmd.cpp の RecvDataProcADMINSWAP）。
 * 確かめと番号の自動振りはサーバーが行う（データ API と同じ関数）ので、ここでは形を整えて送るだけ。
 *
 * ・開いた時・「取得」・自分のマップが変わった時に、サーバーへ表を問い合わせる。
 * ・一覧の行を選ぶと、下の入力欄に中身が入る。種類に合わない欄は隠す。
 * ・番号は空なら自動で振られる（消した番号は使い回さない）。行を選ぶと今の番号が入る。
 * ・ゲーム画面をクリックすると、そのマスが X・Y に入る（そのマスの行があればその行を選ぶ）。
 *   「自分の位置」で今いるマスも入れられる。「選択中のパーツ」はパーツ画面で選んでいるパーツ。
 * ・足す・直す・消すの結果はサーバーから届き、表は届いた物に入れ替わる。
 * ・開いている間、ゲーム画面に各行のマスの枠を描く（SboCliDrawScreen.cpp の DrawMapSwapEdit）。
 */
#include "stdafx.h"
#include <vector>
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgMapSwap.h"

/* ========================================================================= */
/* 定数・構造体の定義														 */
/* ========================================================================= */

#define	MAPSWAP_TICKTIME	500							/* マップ変更を調べる間隔(ms) */
#define	MAPSWAP_TITLE		"印で変わるマス・宝箱"

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"種類", 62}, {"X", 30}, {"Y", 30}, {"パーツ", 44}, {"印/番号", 80}, {"品", 36}, {"メモ", 90},
};

static LPCSTR s_apszKind[] = {"印", "宝箱", "リサイクル箱"};				/* MAPSWAPKIND_* の順 */
static LPCSTR s_apszCmp[] = {"立っている時", "立っていない時"};			/* MAPSWAPCMP_* の順 */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static std::vector<ADMINSWAPROW> s_aRow;						/* 届いている表 */
static DWORD s_dwMapID = (DWORD)-1;								/* 表のマップID（要求済みを含む） */
static BOOL s_bHave = FALSE;									/* 表が届いているか */
static WORD s_wNextChest = 0, s_wNextBox = 0;					/* 次に自動で振る番号 */
static int s_nSel = -1;											/* 選んでいる行 */
static DWORD s_dwTickLast = 0;									/* 最後に調べた時刻 */
static BOOL s_bLoading = FALSE;									/* 入力欄に入れている最中 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static LRESULT OnNotify(HWND hWnd, int nID, NMHDR *pHdr);						/* メッセージハンドラ(WM_NOTIFY) */
static void SendGet(void);														/* 表を要求 */
static void SendEdit(BYTE bySub);												/* 足す・直す・消すを送る */
static BOOL MakeRow(ADMINSWAPROW &Row);											/* 入力欄から 1 行を作る(足りなければ FALSE) */
static void LoadEdit(int nIndex);												/* 行を入力欄へ */
static void RenewList(void);													/* 一覧を作り直す */
static void RenewVisible(void);													/* 種類に合わない欄を隠す */
static void SetMsg(LPCSTR pszMsg);												/* 一言メッセージ */
static BOOL GetNum(int nID, int &nValue);										/* 入力欄の数値(空なら FALSE) */
static void SetNum(int nID, int nValue);										/* 入力欄に数値を入れる */
static int  GetKind(void);														/* 種類の選択 */

/* ========================================================================= */
/* 関数名：ShowDlgMapSwap													 */
/* 内容：ダイアログ表示（開いたらサーバーへ表を問い合わせる）				 */
/* ========================================================================= */
void ShowDlgMapSwap(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}
	s_aRow.clear ();
	s_dwMapID = (DWORD)-1;
	s_bHave = FALSE;
	s_nSel = -1;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_ADMINSWAP, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}
	RenewList ();
	RenewVisible ();
	ShowWindow (s_hWndDlg, SW_SHOW);
	SendGet ();
}

/* ========================================================================= */
/* 関数名：CloseDlgMapSwap													 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgMapSwap(void)
{
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_aRow.clear ();
	s_bHave = FALSE;
	s_nSel = -1;
}

BOOL IsDlgMapSwapOpen(void)
{
	return (s_hWndDlg != NULL);
}

int MapSwapDlgGetCount(void)
{
	return s_hWndDlg ? (int)s_aRow.size () : 0;
}

const ADMINSWAPROW *MapSwapDlgGetRow(int nIndex)
{
	if (s_hWndDlg == NULL || nIndex < 0 || nIndex >= (int)s_aRow.size ()) {
		return NULL;
	}
	return &s_aRow[nIndex];
}

int MapSwapDlgGetSel(void)
{
	return s_hWndDlg ? s_nSel : -1;
}

/* ========================================================================= */
/* 関数名：MapSwapDlgReload													 */
/* 内容：表を取り直す														 */
/* ========================================================================= */
void MapSwapDlgReload(void)
{
	SendGet ();
}

/* ========================================================================= */
/* 関数名：MapSwapDlgTick													 */
/* 内容：自分のマップが変わったら取り直す（ゲーム画面を描くたびに呼ぶ。間引く）*/
/* ========================================================================= */
void MapSwapDlgTick(void)
{
	DWORD dwTime;

	if (s_hWndDlg == NULL || g_pMapInfo == NULL) {
		return;
	}
	dwTime = timeGetTime ();
	if ((DWORD)(dwTime - s_dwTickLast) < MAPSWAP_TICKTIME) {
		return;
	}
	s_dwTickLast = dwTime;
	if (g_pMapInfo->dwID != s_dwMapID) {
		SendGet ();
	}
}

/* ========================================================================= */
/* 関数名：MapSwapDlgOnRecv													 */
/* 内容：サーバーからの通知（INFO は表を入れ替える。RESULT は結果を出す）	 */
/* ========================================================================= */
void MapSwapDlgOnRecv(const void *pParaIn)
{
	const CMDPARA_ADMINSWAP *pPara = (const CMDPARA_ADMINSWAP *)pParaIn;
	char szTmp[128];
	int i;

	if (s_hWndDlg == NULL) {
		return;
	}
	switch (pPara->byCmdSub) {
	case ADMINSWAP_INFO:
		s_aRow.clear ();
		for (i = 0; i < pPara->byCount && i < MAX_MAPSWAP; i ++) {
			s_aRow.push_back (pPara->aRow[i]);
		}
		s_dwMapID = pPara->dwMapID;
		s_bHave = TRUE;
		s_wNextChest = pPara->wNextChest;
		s_wNextBox = pPara->wNextBox;
		if (s_nSel >= (int)s_aRow.size ()) {
			s_nSel = -1;
		}
		RenewList ();
		if (s_nSel >= 0) {
			LoadEdit (s_nSel);
		}
		snprintf (szTmp, sizeof (szTmp), "%d 行（最大 %d）。次に振る番号：宝箱 %d・リサイクル箱 %d", (int)s_aRow.size (), MAX_MAPSWAP,
				(int)s_wNextChest, (int)s_wNextBox);
		SetDlgItemText (s_hWndDlg, IDC_AS_COUNT, szTmp);
		break;

	case ADMINSWAP_RESULT:
		if (pPara->byResult) {
			SetMsg ("反映しました");
		} else {
			char szMsg[MAX_ADMINQUESTMSG + 1];
			CopyMemory (szMsg, pPara->szMsg, MAX_ADMINQUESTMSG);
			szMsg[MAX_ADMINQUESTMSG] = 0;
			SetMsg ("反映できませんでした");
			MessageBox (s_hWndDlg, szMsg, MAPSWAP_TITLE, MB_OK);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：MapSwapDlgOnPos													 */
/* 内容：ゲーム画面がクリックされた：そのマスを X・Y に入れる。そのマスの行があれば選ぶ*/
/* ========================================================================= */
void MapSwapDlgOnPos(int x, int y)
{
	int i;

	if (s_hWndDlg == NULL) {
		return;
	}
	for (i = 0; i < (int)s_aRow.size (); i ++) {
		if ((int)s_aRow[i].wX == x && (int)s_aRow[i].wY == y) {
			LoadEdit (i);
			RenewList ();
			return;
		}
	}
	SetNum (IDC_AS_X, x);
	SetNum (IDC_AS_Y, y);
	SetMsg ("クリックしたマスを X・Y に入れました");
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
	HANDLE_MSG (hWnd, WM_NOTIFY, OnNotify);

	/* 枠を引っぱって大きさを変えた時は、一覧を広げる */
	case WM_SIZE:
		AdminAutoLayout (hWnd, FALSE);
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
	int i;
	RECT rt;
	HWND hWndTmp;
	LV_COLUMN lvcol;

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	hWndTmp = GetDlgItem (hWnd, IDC_AS_LIST);
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	for (i = 0; i < (int)(sizeof (s_aColumn) / sizeof (s_aColumn[0])); i ++) {
		lvcol.cx = s_aColumn[i].nWidth;
		lvcol.pszText = (LPSTR)s_aColumn[i].pszName;
		ListView_InsertColumn (hWndTmp, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	hWndTmp = GetDlgItem (hWnd, IDC_AS_KIND);
	for (i = 0; i < 3; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)s_apszKind[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, 0, 0);
	hWndTmp = GetDlgItem (hWnd, IDC_AS_CMP);
	for (i = 0; i < 2; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)s_apszCmp[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, 0, 0);

	SetMsg ("マップをクリック：そのマスを X・Y に入れる");
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	int nValue;

	switch (nID) {
	case IDC_AS_GET:			/* 取得 */
		SendGet ();
		break;

	case IDC_AS_KIND:			/* 種類を変えた */
		if (nCodeNotify == CBN_SELCHANGE) {
			RenewVisible ();
		}
		break;

	case IDC_AS_HERE:			/* 今の自分の位置 */
		if (g_nMyCharID >= 0 && g_nMyCharID < g_aCharInfo.GetSize ()) {
			SetNum (IDC_AS_X, (int)g_aCharInfo[g_nMyCharID]->dwX);
			SetNum (IDC_AS_Y, (int)g_aCharInfo[g_nMyCharID]->dwY);
		}
		break;

	case IDC_AS_PICKPARTS:		/* パーツ画面で選んでいるパーツ */
		SetNum (IDC_AS_PARTS, g_nSelectPartsID);
		break;

	case IDC_AS_ITEM:			/* 品の番号を入れた（案内の文を直す） */
		if (nCodeNotify == EN_CHANGE && !s_bLoading) {
			SetDlgItemText (hWnd, IDC_AS_ITEMNAME, GetNum (IDC_AS_ITEM, nValue) ? "" : "（空=無し。アイテム番号）");
		}
		break;

	case IDC_AS_ADD:			/* 足す */
		SendEdit (ADMINSWAP_ADD);
		break;

	case IDC_AS_SET:			/* 直す */
		if (s_nSel < 0) {
			MessageBox (hWnd, "直す行を一覧から選んでください。", MAPSWAP_TITLE, MB_OK);
			break;
		}
		SendEdit (ADMINSWAP_SET);
		break;

	case IDC_AS_DEL:			/* 消す（確認してから） */
		if (s_nSel < 0) {
			MessageBox (hWnd, "消す行を一覧から選んでください。", MAPSWAP_TITLE, MB_OK);
			break;
		}
		if (MessageBox (hWnd, "選んでいる行を消します。\n（宝箱・リサイクル箱の番号は、消しても使い回されません）\nよろしいですか？",
				MAPSWAP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) {
			break;
		}
		SendEdit (ADMINSWAP_DEL);
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnNotify															 */
/* 内容：メッセージハンドラ(WM_NOTIFY)。一覧の選択が変わったら入力欄へ入れる	 */
/* ========================================================================= */
static LRESULT OnNotify(HWND hWnd, int nID, NMHDR *pHdr)
{
	NMLISTVIEW *pNm = (NMLISTVIEW *)pHdr;

	if (pHdr == NULL || nID != IDC_AS_LIST || pHdr->code != LVN_ITEMCHANGED) {
		return 0;
	}
	if ((pNm->uNewState & LVIS_SELECTED) == 0 || (pNm->uOldState & LVIS_SELECTED)) {
		return 0;
	}
	if (pNm->iItem < 0 || pNm->iItem >= (int)s_aRow.size () || pNm->iItem == s_nSel) {
		return 0;
	}
	LoadEdit (pNm->iItem);
	return 0;
}

/* ========================================================================= */
/* 関数名：SendGet															 */
/* 内容：今いるマップの表を要求する											 */
/* ========================================================================= */
static void SendGet(void)
{
	if (s_hWndDlg == NULL || g_pMapInfo == NULL) {
		return;
	}
	/* マップが変わっていたら、前のマップの表と選択は捨てる */
	if (g_pMapInfo->dwID != s_dwMapID) {
		s_aRow.clear ();
		s_nSel = -1;
		s_bHave = FALSE;
		RenewList ();
	}
	s_dwMapID = g_pMapInfo->dwID;
	SendCmdADMINSWAP (ADMINSWAP_GET, s_dwMapID, 0, NULL);
}

/* ========================================================================= */
/* 関数名：SendEdit															 */
/* 内容：足す・直す・消すを送る（表のマップは、表を取ったマップ）			 */
/* ========================================================================= */
static void SendEdit(BYTE bySub)
{
	ADMINSWAPROW Row;

	if (!s_bHave) {
		MessageBox (s_hWndDlg, "表を取得してから操作してください。", MAPSWAP_TITLE, MB_OK);
		return;
	}
	ZeroMemory (&Row, sizeof (Row));
	if (bySub != ADMINSWAP_DEL && !MakeRow (Row)) {
		return;
	}
	SetMsg ("送信しました");
	SendCmdADMINSWAP (bySub, s_dwMapID, (WORD)((s_nSel >= 0) ? s_nSel : 0), &Row);
}

/* ========================================================================= */
/* 関数名：MakeRow															 */
/* 内容：入力欄から 1 行を作る。種類に合う欄だけ「指定した」にする			 */
/* ========================================================================= */
static BOOL MakeRow(ADMINSWAPROW &Row)
{
	int nKind = GetKind (), nValue;
	char szTmp[MAX_MAPSWAPMSG];

	ZeroMemory (&Row, sizeof (Row));
	Row.byKind = (BYTE)nKind;
	if (!GetNum (IDC_AS_X, nValue)) {
		MessageBox (s_hWndDlg, "X を入れてください。", MAPSWAP_TITLE, MB_OK);
		return FALSE;
	}
	Row.wX = (WORD)nValue;
	if (!GetNum (IDC_AS_Y, nValue)) {
		MessageBox (s_hWndDlg, "Y を入れてください。", MAPSWAP_TITLE, MB_OK);
		return FALSE;
	}
	Row.wY = (WORD)nValue;
	if (!GetNum (IDC_AS_PARTS, nValue)) {
		MessageBox (s_hWndDlg, "パーツを入れてください。", MAPSWAP_TITLE, MB_OK);
		return FALSE;
	}
	Row.wParts = (WORD)nValue;

	if (nKind == MAPSWAPKIND_FLAG) {
		if (!GetNum (IDC_AS_FLAG, nValue)) {
			MessageBox (s_hWndDlg, "印の番号を入れてください。", MAPSWAP_TITLE, MB_OK);
			return FALSE;
		}
		Row.byHas |= ADMINSWAPHAS_FLAG | ADMINSWAPHAS_CMP;
		Row.wFlag = (WORD)nValue;
		Row.byCmp = (BYTE)SendDlgItemMessage (s_hWndDlg, IDC_AS_CMP, CB_GETCURSEL, 0, 0);
		return TRUE;
	}
	/* 番号は空なら自動 */
	if (GetNum (IDC_AS_NO, nValue)) {
		Row.byHas |= ADMINSWAPHAS_NO;
		Row.wNo = (WORD)nValue;
	}
	GetDlgItemText (s_hWndDlg, IDC_AS_MEMO, szTmp, sizeof (Row.szMemo));
	Row.byHas |= ADMINSWAPHAS_MEMO;
	strncpy (Row.szMemo, szTmp, sizeof (Row.szMemo) - 1);
	if (nKind == MAPSWAPKIND_CHEST) {
		Row.byHas |= ADMINSWAPHAS_GIVEITEM | ADMINSWAPHAS_MSG;
		Row.wGiveItem = GetNum (IDC_AS_ITEM, nValue) ? (WORD)nValue : 0;
		GetDlgItemText (s_hWndDlg, IDC_AS_MSG, szTmp, sizeof (Row.szMsg));
		strncpy (Row.szMsg, szTmp, sizeof (Row.szMsg) - 1);
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：LoadEdit															 */
/* 内容：行を入力欄へ入れて選ぶ												 */
/* ========================================================================= */
static void LoadEdit(int nIndex)
{
	const ADMINSWAPROW *pRow;

	if (nIndex < 0 || nIndex >= (int)s_aRow.size ()) {
		return;
	}
	pRow = &s_aRow[nIndex];
	s_nSel = nIndex;
	s_bLoading = TRUE;
	SendDlgItemMessage (s_hWndDlg, IDC_AS_KIND, CB_SETCURSEL, (pRow->byKind <= MAPSWAPKIND_RECYCLE) ? pRow->byKind : 0, 0);
	SetNum (IDC_AS_X, pRow->wX);
	SetNum (IDC_AS_Y, pRow->wY);
	SetNum (IDC_AS_PARTS, pRow->wParts);
	SetDlgItemText (s_hWndDlg, IDC_AS_FLAG, "");
	SetDlgItemText (s_hWndDlg, IDC_AS_NO, "");
	SetDlgItemText (s_hWndDlg, IDC_AS_ITEM, "");
	SetDlgItemText (s_hWndDlg, IDC_AS_MSG, "");
	SetDlgItemText (s_hWndDlg, IDC_AS_MEMO, "");
	if (pRow->byKind == MAPSWAPKIND_FLAG) {
		SetNum (IDC_AS_FLAG, pRow->wFlag);
		SendDlgItemMessage (s_hWndDlg, IDC_AS_CMP, CB_SETCURSEL, pRow->byCmp ? 1 : 0, 0);
	} else {
		SetNum (IDC_AS_NO, pRow->wNo);
		SetDlgItemText (s_hWndDlg, IDC_AS_MEMO, pRow->szMemo);
		if (pRow->byKind == MAPSWAPKIND_CHEST) {
			SetNum (IDC_AS_ITEM, pRow->wGiveItem);
			SetDlgItemText (s_hWndDlg, IDC_AS_MSG, pRow->szMsg);
		}
	}
	s_bLoading = FALSE;
	RenewVisible ();
	SetDlgItemText (s_hWndDlg, IDC_AS_ITEMNAME, "（空=無し。アイテム番号）");
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：一覧を作り直す（選んでいた行は選んだままにする）					 */
/* ========================================================================= */
static void RenewList(void)
{
	HWND hWndList;
	LV_ITEM item;
	char szTmp[160];
	int i;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndList = GetDlgItem (s_hWndDlg, IDC_AS_LIST);
	ListView_DeleteAllItems (hWndList);
	for (i = 0; i < (int)s_aRow.size (); i ++) {
		const ADMINSWAPROW *pRow = &s_aRow[i];

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = i;
		item.lParam = i;
		item.pszText = (LPSTR)s_apszKind[(pRow->byKind <= MAPSWAPKIND_RECYCLE) ? pRow->byKind : 0];
		ListView_InsertItem (hWndList, &item);
		snprintf (szTmp, sizeof (szTmp), "%d", (int)pRow->wX);
		ListView_SetItemText (hWndList, i, 1, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%d", (int)pRow->wY);
		ListView_SetItemText (hWndList, i, 2, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%d", (int)pRow->wParts);
		ListView_SetItemText (hWndList, i, 3, szTmp);
		if (pRow->byKind == MAPSWAPKIND_FLAG) {
			snprintf (szTmp, sizeof (szTmp), "印%d（%s）", (int)pRow->wFlag, s_apszCmp[pRow->byCmp ? 1 : 0]);
		} else if (pRow->byKind == MAPSWAPKIND_RECYCLE) {
			snprintf (szTmp, sizeof (szTmp), "箱%d", (int)pRow->wNo);
		} else {
			snprintf (szTmp, sizeof (szTmp), "宝箱%d", (int)pRow->wNo);
		}
		ListView_SetItemText (hWndList, i, 4, szTmp);
		if (pRow->byKind == MAPSWAPKIND_CHEST && pRow->wGiveItem) {
			snprintf (szTmp, sizeof (szTmp), "%d", (int)pRow->wGiveItem);
		} else {
			szTmp[0] = 0;
		}
		ListView_SetItemText (hWndList, i, 5, szTmp);
		ListView_SetItemText (hWndList, i, 6, (LPSTR)pRow->szMemo);
		if (i == s_nSel) {
			ListView_SetItemState (hWndList, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
	}
}

/* ========================================================================= */
/* 関数名：RenewVisible														 */
/* 内容：種類に合わない欄を隠す（印=印番号と比べ方／宝箱=番号・品・文・メモ／リサイクル箱=番号・メモ）*/
/* ========================================================================= */
static void RenewVisible(void)
{
	static const int anFlag[] = {IDC_AS_L_FLAG, IDC_AS_FLAG, IDC_AS_L_CMP, IDC_AS_CMP};
	static const int anNo[] = {IDC_AS_L_NO, IDC_AS_NO, IDC_AS_L_AUTO};
	static const int anItem[] = {IDC_AS_L_ITEM, IDC_AS_ITEM, IDC_AS_ITEMNAME, IDC_AS_L_MSG, IDC_AS_MSG};
	static const int anMemo[] = {IDC_AS_L_MEMO, IDC_AS_MEMO};
	int i, nKind = GetKind ();

	if (s_hWndDlg == NULL) {
		return;
	}
	for (i = 0; i < (int)(sizeof (anFlag) / sizeof (anFlag[0])); i ++) {
		ShowWindow (GetDlgItem (s_hWndDlg, anFlag[i]), (nKind == MAPSWAPKIND_FLAG) ? SW_SHOW : SW_HIDE);
	}
	for (i = 0; i < (int)(sizeof (anNo) / sizeof (anNo[0])); i ++) {
		ShowWindow (GetDlgItem (s_hWndDlg, anNo[i]), (nKind != MAPSWAPKIND_FLAG) ? SW_SHOW : SW_HIDE);
	}
	for (i = 0; i < (int)(sizeof (anItem) / sizeof (anItem[0])); i ++) {
		ShowWindow (GetDlgItem (s_hWndDlg, anItem[i]), (nKind == MAPSWAPKIND_CHEST) ? SW_SHOW : SW_HIDE);
	}
	for (i = 0; i < (int)(sizeof (anMemo) / sizeof (anMemo[0])); i ++) {
		ShowWindow (GetDlgItem (s_hWndDlg, anMemo[i]), (nKind != MAPSWAPKIND_FLAG) ? SW_SHOW : SW_HIDE);
	}
}

static int GetKind(void)
{
	int n = (int)SendDlgItemMessage (s_hWndDlg, IDC_AS_KIND, CB_GETCURSEL, 0, 0);
	return (n >= 0 && n <= MAPSWAPKIND_RECYCLE) ? n : 0;
}

static void SetMsg(LPCSTR pszMsg)
{
	if (s_hWndDlg) {
		SetDlgItemText (s_hWndDlg, IDC_AS_STATUS, pszMsg);
	}
}

static BOOL GetNum(int nID, int &nValue)
{
	char szTmp[32];

	GetDlgItemText (s_hWndDlg, nID, szTmp, sizeof (szTmp));
	if (szTmp[0] == 0) {
		return FALSE;
	}
	nValue = atoi (szTmp);
	return TRUE;
}

static void SetNum(int nID, int nValue)
{
	char szTmp[32];

	snprintf (szTmp, sizeof (szTmp), "%d", nValue);
	SetDlgItemText (s_hWndDlg, nID, szTmp);
}
