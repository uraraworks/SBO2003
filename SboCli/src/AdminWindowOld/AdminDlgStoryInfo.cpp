/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		思い出の本ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgStoryInfo.cpp										 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 思い出の本のシナリオ（クリア済みのミニシナリオをキャラごとにやり直す）を、
 * 管理者ウィンドウで見て直すための一覧画面。リソースは IDD_STORYINFO。
 * シナリオのデータはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * CMD_ADMINSTORY の ITEM で 1 件ずつ届いたものを手元に覚えておく。
 * ・取得：サーバーから一覧を取り直す
 * ・編集／追加：編集画面を出す（OK でサーバーへ送り、データAPIと同じ確認を通る）
 * 消すことはできない（データAPIと同じ。出したくない時は状態を「止める」にする）。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgStoryInfo.h"
#include "AdminDlgStoryInfoEdit.h"
#include <vector>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static std::vector<STORYINFO> s_aStory;							/* サーバーから届いたシナリオ（配列の番号はサーバーと同じ） */
static std::vector<BYTE> s_abyHave;								/* その番号が届いているか */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号", 40}, {"状態", 50}, {"並び", 40}, {"名前", 160}, {"クリアの印", 65}, {"前のお話", 130},
	{"印の範囲", 110}, {"依頼", 70},
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
/* 関数名：ShowDlgStoryInfo													 */
/* 内容：ダイアログ表示（開いたらサーバーへ一覧を問い合わせる）				 */
/* ========================================================================= */
void ShowDlgStoryInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_STORYINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RequestList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgStoryInfo												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgStoryInfo(void)
{
	CloseDlgStoryInfoEdit ();
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：AdminStoryGetCount / AdminStoryGet								 */
/* 内容：届いているシナリオ													 */
/* ========================================================================= */
int AdminStoryGetCount(void)
{
	return (int)s_aStory.size ();
}

const STORYINFO *AdminStoryGet(int nNo)
{
	if ((nNo < 0) || (nNo >= (int)s_aStory.size ()) || !s_abyHave[nNo]) {
		return NULL;
	}
	return &s_aStory[nNo];
}

/* ========================================================================= */
/* 関数名：AdminStoryOnRecv													 */
/* 内容：サーバーから届いた（一覧の 1 件・書き込みの結果）					 */
/* ========================================================================= */
void AdminStoryOnRecv(const void *pParaIn)
{
	const CMDPARA_ADMINSTORY *pPara = (const CMDPARA_ADMINSTORY *)pParaIn;
	int nCount = pPara->wCount;

	if (nCount > MAX_STORY) {
		nCount = MAX_STORY;
	}

	switch (pPara->byCmdSub) {
	case ADMINSTORY_ITEM:
		/* 件数が変わっていたら合わせる（増えた分はまだ届いていない） */
		if ((int)s_aStory.size () != nCount) {
			STORYINFO Empty;
			ZeroMemory (&Empty, sizeof (Empty));
			s_aStory.resize (nCount, Empty);
			s_abyHave.resize (nCount, 0);
		}
		if ((pPara->nNo >= 0) && (pPara->nNo < nCount)) {
			STORYINFO *pInfo = &s_aStory[pPara->nNo];
			*pInfo = pPara->Info;
			/* 文字の欄は必ず終端させる */
			pInfo->szName[sizeof (pInfo->szName) - 1] = 0;
			pInfo->szExplain[sizeof (pInfo->szExplain) - 1] = 0;
			s_abyHave[pPara->nNo] = 1;
		}
		RenewList ();
		break;

	case ADMINSTORY_RESULT:
		StoryEditOnResult (pPara->byResult ? TRUE : FALSE, pPara->nNo, pPara->szMsg);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RequestList														 */
/* 内容：サーバーへ一覧を問い合わせる（届くまでは空にしておく）				 */
/* ========================================================================= */
static void RequestList(void)
{
	s_aStory.clear ();
	s_abyHave.clear ();
	RenewList ();
	SendCmdADMINSTORY (ADMINSTORY_LISTREQ, 0, NULL);
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
		if (IsDlgStoryInfoEditOpen ()) {
			MessageBox (hWnd, "編集画面を閉じてから取得してください。", "思い出の本", MB_OK);
			break;
		}
		RequestList ();
		break;

	case IDC_EDIT:	/* 編集 */
		if (AdminStoryGet (nNo) == NULL) {
			MessageBox (hWnd, "編集するシナリオを一覧から選んでください。", "思い出の本", MB_OK);
			break;
		}
		ShowDlgStoryInfoEdit (hWnd, nNo);
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgStoryInfoEdit (hWnd, -1);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* ========================================================================= */
static void RenewList(void)
{
	int i, j, nNo, nCount;
	char szTmp[96];
	HWND hWndTmp;
	LV_ITEM item;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = (int)s_aStory.size ();
	for (i = 0; i < nCount; i ++) {
		const STORYINFO *pInfo = &s_aStory[i];
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
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wID);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);

		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)((pInfo->byState == STORYSTATE_ON) ? "出す" : "止める"));
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wSort);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)pInfo->szName);
		if (pInfo->wClearFlag < MAX_FLAGBIT) {
			snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wClearFlag);
		} else {
			snprintf (szTmp, sizeof (szTmp), "（無し）");
		}
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);

		/* 前のお話（番号と名前） */
		szTmp[0] = 0;
		if (pInfo->wPrevStory != 0) {
			snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wPrevStory);
			for (j = 0; j < nCount; j ++) {
				if (s_abyHave[j] && (s_aStory[j].wID == pInfo->wPrevStory)) {
					snprintf (szTmp, sizeof (szTmp), "%u: %s", (unsigned)pInfo->wPrevStory, s_aStory[j].szName);
					break;
				}
			}
		}
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);

		/* 消す印の範囲 */
		szTmp[0] = 0;
		for (j = 0; j < MAX_STORYFLAGRANGE; j ++) {
			char szOne[32];
			if ((pInfo->awFlagFrom[j] >= MAX_FLAGBIT) || (pInfo->awFlagTo[j] >= MAX_FLAGBIT)) {
				continue;
			}
			snprintf (szOne, sizeof (szOne), "%s%u-%u", szTmp[0] ? ", " : "", (unsigned)pInfo->awFlagFrom[j], (unsigned)pInfo->awFlagTo[j]);
			strncat (szTmp, szOne, sizeof (szTmp) - strlen (szTmp) - 1);
		}
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);

		/* 戻す依頼 */
		szTmp[0] = 0;
		for (j = 0; j < MAX_STORYQUEST; j ++) {
			char szOne[16];
			if (pInfo->awQuest[j] == 0) {
				continue;
			}
			snprintf (szOne, sizeof (szOne), "%s%u", szTmp[0] ? ", " : "", (unsigned)pInfo->awQuest[j]);
			strncat (szTmp, szOne, sizeof (szTmp) - strlen (szTmp) - 1);
		}
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
	}

	if ((nNo >= 0) && (nNo < nCount)) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

/* Copyright(C)URARA-works 2026 */
