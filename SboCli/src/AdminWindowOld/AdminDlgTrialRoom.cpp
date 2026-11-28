/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		お試し個室ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgTrialRoom.cpp										 */
/* 作成日：		2026/11/27													 */
/* ========================================================================= */
/*
 * お試し個室（試し中の新機能を、テンプレートのキャラで遊べる部屋）を、管理者ウィンドウで
 * 見て直すための一覧画面。リソースは IDD_TRIALROOM。
 * お試し個室のデータはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * CMD_ADMINTRIALROOM の ITEM で 1 件ずつ届いたものを手元に覚えておく。
 * ・取得：サーバーから一覧を取り直す
 * ・編集／追加：編集画面を出す（OK でサーバーへ送り、データAPIと同じ確認を通る）
 * 消すことはできない（データAPIと同じ。入れなくしたい時は状態を「閉じている」にする）。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgTrialRoom.h"
#include "AdminDlgTrialRoomEdit.h"
#include <vector>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define TRIALROOM_TITLE		"お試し個室"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static std::vector<TRIALROOMINFO> s_aRoom;						/* サーバーから届いたお試し個室（配列の番号はサーバーと同じ） */
static std::vector<BYTE> s_abyHave;								/* その番号が届いているか */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号", 40}, {"状態", 70}, {"名前", 160}, {"貸すテンプレート", 90}, {"マップ", 50},
	{"入るX,Y", 60}, {"範囲", 120},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */
static void RequestList(void);													/* サーバーへ一覧を問い合わせる */

/* ========================================================================= */
/* 関数名：ShowDlgTrialRoom													 */
/* 内容：ダイアログ表示（開いたらサーバーへ一覧を問い合わせる）				 */
/* ========================================================================= */
void ShowDlgTrialRoom(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_TRIALROOM, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RequestList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgTrialRoom												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgTrialRoom(void)
{
	CloseDlgTrialRoomEdit ();
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：AdminTrialRoomGetCount / AdminTrialRoomGet						 */
/* 内容：届いているお試し個室												 */
/* ========================================================================= */
int AdminTrialRoomGetCount(void)
{
	return (int)s_aRoom.size ();
}

const TRIALROOMINFO *AdminTrialRoomGet(int nNo)
{
	if ((nNo < 0) || (nNo >= (int)s_aRoom.size ()) || !s_abyHave[nNo]) {
		return NULL;
	}
	return &s_aRoom[nNo];
}

/* ========================================================================= */
/* 関数名：AdminTrialRoomOnRecv												 */
/* 内容：サーバーから届いた（一覧の 1 件・書き込みの結果）					 */
/* ========================================================================= */
void AdminTrialRoomOnRecv(const void *pParaIn)
{
	const CMDPARA_ADMINTRIALROOM *pPara = (const CMDPARA_ADMINTRIALROOM *)pParaIn;
	int nCount = pPara->wCount;

	if (nCount > MAX_TRIALROOM) {
		nCount = MAX_TRIALROOM;
	}

	switch (pPara->byCmdSub) {
	case ADMINTRIALROOM_ITEM:
		/* 件数が変わっていたら合わせる（増えた分はまだ届いていない） */
		if ((int)s_aRoom.size () != nCount) {
			TRIALROOMINFO Empty;
			ZeroMemory (&Empty, sizeof (Empty));
			s_aRoom.resize (nCount, Empty);
			s_abyHave.resize (nCount, 0);
		}
		if ((pPara->nNo >= 0) && (pPara->nNo < nCount)) {
			TRIALROOMINFO *pInfo = &s_aRoom[pPara->nNo];
			*pInfo = pPara->Info;
			/* 文字の欄は必ず終端させる */
			pInfo->szName[sizeof (pInfo->szName) - 1] = 0;
			pInfo->szDesc[sizeof (pInfo->szDesc) - 1] = 0;
			s_abyHave[pPara->nNo] = 1;
		}
		RenewList ();
		break;

	case ADMINTRIALROOM_RESULT:
		TrialRoomEditOnResult (pPara->byResult ? TRUE : FALSE, pPara->nNo, pPara->szMsg);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RequestList														 */
/* 内容：サーバーへ一覧を問い合わせる（届くまでは空にしておく）				 */
/* ========================================================================= */
static void RequestList(void)
{
	s_aRoom.clear ();
	s_abyHave.clear ();
	RenewList ();
	SendCmdADMINTRIALROOM (ADMINTRIALROOM_LISTREQ, 0, NULL);
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
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	HWND hWndList = GetDlgItem (hWnd, IDC_LIST);
	int nNo = ListView_GetNextItem (hWndList, -1, LVNI_ALL | LVNI_SELECTED);

	switch (nID) {
	case IDC_GET:	/* 取得（サーバーから取り直す） */
		if (IsDlgTrialRoomEditOpen ()) {
			MessageBox (hWnd, "編集画面を閉じてから取得してください。", TRIALROOM_TITLE, MB_OK);
			break;
		}
		RequestList ();
		break;

	case IDC_EDIT:	/* 編集 */
		if (AdminTrialRoomGet (nNo) == NULL) {
			MessageBox (hWnd, "編集するお試し個室を一覧から選んでください。", TRIALROOM_TITLE, MB_OK);
			break;
		}
		ShowDlgTrialRoomEdit (hWnd, nNo);
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgTrialRoomEdit (hWnd, -1);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* ========================================================================= */
static void RenewList(void)
{
	int i, nNo, nCount;
	char szTmp[96];
	HWND hWndTmp;
	LV_ITEM item;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = (int)s_aRoom.size ();
	for (i = 0; i < nCount; i ++) {
		const TRIALROOMINFO *pInfo = &s_aRoom[i];
		int nCol = 0;

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = nCol ++;
		if (!s_abyHave[i]) {
			item.pszText = (LPSTR)"…";
			ListView_InsertItem (hWndTmp, &item);
			continue;
		}
		/* 番号は配列の番号（入口のNPCの台詞 @xNN の NN） */
		snprintf (szTmp, sizeof (szTmp), "%02d", i);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);

		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)((pInfo->byState == TRIALROOMSTATE_OPEN) ? "開いている" : "閉じている"));
		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)pInfo->szName);
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wLoanStTmp);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%lu", (unsigned long)pInfo->dwMapID);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%lu,%lu", (unsigned long)pInfo->dwX, (unsigned long)pInfo->dwY);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "(%lu,%lu)-(%lu,%lu)", (unsigned long)pInfo->dwX1, (unsigned long)pInfo->dwY1,
				(unsigned long)pInfo->dwX2, (unsigned long)pInfo->dwY2);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
	}

	if ((nNo >= 0) && (nNo < nCount)) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

/* Copyright(C)URARA-works 2026 */
