/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ギルドの依頼ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgQuestInfo.cpp										 */
/* 作成日：		2026/11/12													 */
/* ========================================================================= */
/*
 * ギルドの依頼（探し物などのふつうの依頼・討伐チャレンジ）を、管理者ウィンドウで
 * 見て直すための一覧画面。リソースは IDD_QUESTINFO。
 * 依頼のデータはサーバーだけが持つので、開いた時（と「取得」）にサーバーへ問い合わせ、
 * CMD_ADMINQUEST の ITEM で 1 件ずつ届いたものを手元に覚えておく。
 * ・取得：サーバーから一覧を取り直す
 * ・編集／追加：編集画面を出す（OK でサーバーへ送り、データAPIと同じ確認を通る）
 * 消すことはできない（データAPIと同じ。出したくない時は状態を「止める」にする）。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgQuestInfo.h"
#include "AdminDlgQuestInfoEdit.h"
#include <vector>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static std::vector<QUESTINFO> s_aQuest;							/* サーバーから届いた依頼（配列の番号はサーバーと同じ） */
static std::vector<BYTE> s_abyHave;								/* その番号が届いているか */
static BOOL s_bAdminPosBack = FALSE;							/* 開く前の「座標を拾う」 */

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号", 40}, {"状態", 50}, {"ギルド", 45}, {"並び", 40}, {"名前", 160}, {"種類", 130},
	{"段", 30}, {"お金", 60}, {"経験値", 60}, {"NPC", 35},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */
static void RequestList(void);													/* サーバーへ一覧を問い合わせる */

/* 討伐チャレンジの種類の名前（QUESTCHALLENGE_*。足りない分は番号で出す） */
LPCSTR AdminQuestChallengeName(int nType, char *pszBuf, int nBufSize)
{
	static const LPCSTR s_apszName[] = {
		"ふつうの依頼", "討伐チャレンジ（異空間）", "討伐チャレンジ（闘技場）",
	};
	if ((nType >= 0) && (nType < (int)(sizeof (s_apszName) / sizeof (s_apszName[0])))) {
		return s_apszName[nType];
	}
	snprintf (pszBuf, nBufSize, "討伐チャレンジ（種類%d）", nType);
	return pszBuf;
}

/* ========================================================================= */
/* 関数名：ShowDlgQuestInfo													 */
/* 内容：ダイアログ表示（開いたらサーバーへ一覧を問い合わせる）				 */
/* ========================================================================= */
void ShowDlgQuestInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_QUESTINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	/* 開いている間はゲーム画面のクリックで座標を拾う（編集画面の見えるNPCの場所に使う） */
	s_bAdminPosBack = g_bAdminPos;
	g_bAdminPos = TRUE;

	RequestList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgQuestInfo												 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgQuestInfo(void)
{
	CloseDlgQuestInfoEdit ();
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
		g_bAdminPos = s_bAdminPosBack;
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：AdminQuestGetCount / AdminQuestGet								 */
/* 内容：届いている依頼														 */
/* ========================================================================= */
int AdminQuestGetCount(void)
{
	return (int)s_aQuest.size ();
}

const QUESTINFO *AdminQuestGet(int nNo)
{
	if ((nNo < 0) || (nNo >= (int)s_aQuest.size ()) || !s_abyHave[nNo]) {
		return NULL;
	}
	return &s_aQuest[nNo];
}

/* ========================================================================= */
/* 関数名：AdminQuestOnPos													 */
/* 内容：ゲーム画面がクリックされた(使ったら TRUE)							 */
/* ========================================================================= */
BOOL AdminQuestOnPos(int x, int y, BOOL bRight)
{
	if (s_hWndDlg == NULL) {
		return FALSE;
	}
	return QuestEditOnPos (x, y, bRight);
}

/* ========================================================================= */
/* 関数名：AdminQuestOnRecv													 */
/* 内容：サーバーから届いた（一覧の 1 件・書き込みの結果）					 */
/* ========================================================================= */
void AdminQuestOnRecv(const void *pParaIn)
{
	const CMDPARA_ADMINQUEST *pPara = (const CMDPARA_ADMINQUEST *)pParaIn;
	int nCount = pPara->wCount;

	if (nCount > MAX_QUEST) {
		nCount = MAX_QUEST;
	}

	switch (pPara->byCmdSub) {
	case ADMINQUEST_ITEM:
		/* 件数が変わっていたら合わせる（増えた分はまだ届いていない） */
		if ((int)s_aQuest.size () != nCount) {
			QUESTINFO Empty;
			ZeroMemory (&Empty, sizeof (Empty));
			s_aQuest.resize (nCount, Empty);
			s_abyHave.resize (nCount, 0);
		}
		if ((pPara->nNo >= 0) && (pPara->nNo < nCount)) {
			QUESTINFO *pInfo = &s_aQuest[pPara->nNo];
			*pInfo = pPara->Info;
			/* 文字の欄は必ず終端させる */
			pInfo->szName[sizeof (pInfo->szName) - 1] = 0;
			pInfo->szExplain[sizeof (pInfo->szExplain) - 1] = 0;
			pInfo->szMsgAccept[sizeof (pInfo->szMsgAccept) - 1] = 0;
			pInfo->szMsgDone[sizeof (pInfo->szMsgDone) - 1] = 0;
			for (int i = 0; i < MAX_QUESTSTEP; i ++) {
				pInfo->aszGoal[i][MAX_QUESTGOAL - 1] = 0;
			}
			for (int i = 0; i < MAX_QUESTNPC; i ++) {
				pInfo->aNpc[i].szName[sizeof (pInfo->aNpc[i].szName) - 1] = 0;
				pInfo->aNpc[i].szSpeech[sizeof (pInfo->aNpc[i].szSpeech) - 1] = 0;
			}
			s_abyHave[pPara->nNo] = 1;
		}
		RenewList ();
		break;

	case ADMINQUEST_RESULT:
		QuestEditOnResult (pPara->byResult ? TRUE : FALSE, pPara->nNo, pPara->szMsg);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RequestList														 */
/* 内容：サーバーへ一覧を問い合わせる（届くまでは空にしておく）				 */
/* ========================================================================= */
static void RequestList(void)
{
	s_aQuest.clear ();
	s_abyHave.clear ();
	RenewList ();
	SendCmdADMINQUEST (ADMINQUEST_LISTREQ, 0, NULL);
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
		if (IsDlgQuestInfoEditOpen ()) {
			MessageBox (hWnd, "編集画面を閉じてから取得してください。", "ギルドの依頼", MB_OK);
			break;
		}
		RequestList ();
		break;

	case IDC_EDIT:	/* 編集 */
		if (AdminQuestGet (nNo) == NULL) {
			MessageBox (hWnd, "編集する依頼を一覧から選んでください。", "ギルドの依頼", MB_OK);
			break;
		}
		ShowDlgQuestInfoEdit (hWnd, nNo);
		break;

	case IDC_ADD:	/* 追加 */
		ShowDlgQuestInfoEdit (hWnd, -1);
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
	char szTmp[64], szBuf[64];
	HWND hWndTmp;
	LV_ITEM item;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
	ListView_DeleteAllItems (hWndTmp);

	nCount = (int)s_aQuest.size ();
	for (i = 0; i < nCount; i ++) {
		const QUESTINFO *pInfo = &s_aQuest[i];
		int nCol = 0, nNpc = 0;

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

		for (int j = 0; j < MAX_QUESTNPC; j ++) {
			if (pInfo->aNpc[j].byKind != QUESTNPCKIND_NONE) {
				nNpc ++;
			}
		}

		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)((pInfo->byState == QUESTSTATE_ON) ? "出す" : "止める"));
		snprintf (szTmp, sizeof (szTmp), "%02u", (unsigned)pInfo->byGuild);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->wSort);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)pInfo->szName);
		ListView_SetItemText (hWndTmp, i, nCol ++, (LPSTR)AdminQuestChallengeName (pInfo->byChallenge, szBuf, sizeof (szBuf)));
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pInfo->byStepCount);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%lu", (unsigned long)pInfo->dwGold);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%lu", (unsigned long)pInfo->dwExp);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
		snprintf (szTmp, sizeof (szTmp), "%d", nNpc);
		ListView_SetItemText (hWndTmp, i, nCol ++, szTmp);
	}

	if ((nNo >= 0) && (nNo < nCount)) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

/* Copyright(C)URARA-works 2026 */
