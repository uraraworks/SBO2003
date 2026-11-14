/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		所持魔法情報ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgCharMagicInfo.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/15													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../SboCliRecvCmd.h"
#include "AdminListFilter.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static PCHARINFO s_pChar = NULL;								/* 編集中のキャラ情報 */
static int s_nTblID = -1;										/* 編集中のキャラID */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgCharMagicInfo(HWND hWndParent);										/* ダイアログ表示(所持魔法情報) */
void CloseDlgCharMagicInfo(void);												/* ダイアログ終了(所持魔法情報) */
void RenewCharMagicInfo(void);													/* 所持魔法情報更新 */
void SetCharMagicInfo(PCHARINFO);												/* 編集するキャラの設定 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void RenewList(void);													/* リスト更新 */
static void RenewMagicCombo(void);												/* 追加する魔法の一覧を絞り込んで作り直す */
static int GetMagicComboID(void);												/* 一覧で選んでいる魔法ID */

/* ========================================================================= */
/* 関数名：ShowDlgCharMagicInfo												 */
/* 内容：ダイアログ表示(所持魔法情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void ShowDlgCharMagicInfo(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_CHARMAGICINFO, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgCharMagicInfo											 */
/* 内容：ダイアログ終了(所持魔法情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void CloseDlgCharMagicInfo(void)
{
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewCharMagicInfo												 */
/* 内容：所持魔法情報更新													 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void RenewCharMagicInfo(void)
{
	RenewList ();
}

/* ========================================================================= */
/* 関数名：SetCharMagicInfo													 */
/* 内容：編集するキャラの設定												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void SetCharMagicInfo(PCHARINFO pChar)
{
	if (pChar == NULL) {
		ZeroMemory (s_pChar, sizeof (CHARINFO));
		s_nTblID = -1;

	} else {
		if ((s_nTblID >= 0) && (s_pChar != pChar)) {
			return;
		}
		CopyMemory (s_pChar, pChar, sizeof (CHARINFO));
		s_nTblID = (int)pChar->dwTblID;
	}

	RenewList ();
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_DESTROY:
		SAFE_DELETE (s_pChar);
		break;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	RECT rt;
	int i, nCount;
	HWND hWndTmp;
	LV_COLUMN lvcol;

	s_pChar = new CHARINFO;
	ZeroMemory (s_pChar, sizeof (CHARINFO));

	hWndTmp = GetDlgItem (hWnd, IDC_LIST);

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 180;
	lvcol.pszText = "魔法名";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);

	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);


	/* 種別で絞り込み */
	hWndTmp = GetDlgItem (hWnd, IDC_MAGICTYPE);
	SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)"すべて");
	for (i = 0; g_aStrType[i][0] != 'z'; i ++) {
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrType[i]);
	}
	SendMessage (hWndTmp, CB_SETCURSEL, 0, 0);

	/* 魔法名（itemdata に魔法ID）。ダイアログ作成中なので s_hWndDlg ではなく hWnd で作る */
	s_hWndDlg = hWnd;
	RenewMagicCombo ();
	SendMessage (GetDlgItem (hWnd, IDC_MAGIC), CB_SETCURSEL, (WPARAM)-1, 0);


	hWndTmp = GetDlgItem (hWnd, IDC_STTMP);
	nCount = g_aStTmpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
		SendMessage (hWndTmp, CB_ADDSTRING, (WPARAM)i, (LPARAM)pInfo->szName);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_SEND:		/* 送信 */
		{
			PCHARINFO pChar;
			int nNo;

			if (s_nTblID < 0) {
				break;
			}
			nNo = GetCharNo2 (s_nTblID);
			if (nNo < 0) {
				break;
			}

			pChar = g_aCharInfo[nNo];
			CopyMemory (pChar->abyMagic, s_pChar->abyMagic, sizeof (pChar->abyMagic));
			SendCmdCHARSTATUS ((DWORD)nNo);
			SetCharMagicInfo (NULL);
		}
		break;

	case IDC_ADD:		/* 追加 */
		{
			int nNo, nMagicID;

			for (nNo = 0; nNo < MAX_USEMAGIC; nNo ++) {
				if (s_pChar->abyMagic[nNo] == 0) {
					break;
				}
			}
			if (nNo >= MAX_USEMAGIC) {
				break;
			}

			nMagicID = GetMagicComboID ();
			if (nMagicID <= 0) {
				break;
			}
			s_pChar->abyMagic[nNo] = (BYTE)nMagicID;
			RenewList ();
		}
		break;

	case IDC_MODIFY:	/* 更新 */
		{
			int nNo, nMagicID;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			nMagicID = GetMagicComboID ();
			if (nMagicID <= 0) {
				break;
			}
			s_pChar->abyMagic[nNo] = (BYTE)nMagicID;
			RenewList ();
		}
		break;

	case IDC_DELETE:	/* 消去 */
		{
			int nNo;
			HWND hWndTmp;

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);
			nNo = ListView_GetNextItem (hWndTmp, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			for (; nNo < MAX_USEMAGIC - 1; nNo ++) {
				s_pChar->abyMagic[nNo] = s_pChar->abyMagic[nNo + 1];
			}
			s_pChar->abyMagic[nNo] = 0;
			RenewList ();
		}
		break;

	case IDC_GET:		/* ←取得 */
		{
			int nNo;
			HWND hWndTmp;
			PSTATUSTEMPLATEINFO pInfo;

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_STTMP);
			nNo = (int)SendMessage (hWndTmp, CB_GETCURSEL, 0, 0);
			if (nNo < 0) {
				break;
			}
			pInfo = g_aStTmpInfo[nNo];
			CopyMemory (s_pChar->abyMagic, pInfo->abyMagic, sizeof (pInfo->abyMagic));
			RenewList ();
		}
		break;

	case IDC_PUT:		/* →反映 */
		{
			int nNo;
			HWND hWndTmp;
			PSTATUSTEMPLATEINFO pInfo;

			hWndTmp = GetDlgItem (s_hWndDlg, IDC_STTMP);
			nNo = (int)SendMessage (hWndTmp, CB_GETCURSEL, 0, 0);
			if (nNo < 0) {
				break;
			}
			pInfo = g_aStTmpInfo[nNo];
			CopyMemory (pInfo->abyMagic, s_pChar->abyMagic, sizeof (pInfo->abyMagic));
			SendCmdSTTMPLATE ((DWORD)nNo, CMDSTTMPLATETYPE_CHG);
		}
		break;

	case IDC_MAGICTYPE:	/* 種別で絞り込み */
		if (nCodeNotify == CBN_SELCHANGE) {
			RenewMagicCombo ();
		}
		break;

	case IDC_MAGICFIND:	/* 名前で絞り込み（部分一致） */
		if (nCodeNotify == EN_CHANGE) {
			RenewMagicCombo ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：GetMagicComboID													 */
/* 内容：一覧で選んでいる魔法ID（無ければ-1）								 */
/* ========================================================================= */
static int GetMagicComboID(void)
{
	HWND hWndTmp = GetDlgItem (s_hWndDlg, IDC_MAGIC);
	int nSel = (int)SendMessage (hWndTmp, CB_GETCURSEL, 0, 0);
	if (nSel < 0) {
		return -1;
	}
	return (int)SendMessage (hWndTmp, CB_GETITEMDATA, nSel, 0);
}

/* ========================================================================= */
/* 関数名：RenewMagicCombo													 */
/* 内容：追加する魔法の一覧を、種別と名前の部分一致で絞り込んで作り直す		 */
/* 		 選んでいた魔法が残っていればそのまま								 */
/* ========================================================================= */
static void RenewMagicCombo(void)
{
	HWND hWndCombo = GetDlgItem (s_hWndDlg, IDC_MAGIC);
	if (hWndCombo == NULL) {
		return;
	}
	int nOldID = GetMagicComboID ();

	/* 種別（0=すべて、1〜=種別+1） */
	int nType = (int)SendMessage (GetDlgItem (s_hWndDlg, IDC_MAGICTYPE), CB_GETCURSEL, 0, 0) - 1;
	char szKey[128];
	szKey[0] = 0;
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_MAGICFIND), szKey, sizeof (szKey));

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	int i, nSel = -1, nCount = g_aMagicInfo.GetSize ();
	/* 0番は「無し」なので出さない（所持魔法は BYTE なので 255 まで） */
	for (i = 1; (i < nCount) && (i <= 255); i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[i];
		if ((nType >= 0) && (pInfo->byType != nType)) {
			continue;
		}
		if (!AdminFilterMatch (pInfo->szName, szKey)) {
			continue;
		}
		int nIndex = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		SendMessage (hWndCombo, CB_SETITEMDATA, nIndex, (LPARAM)i);
		if (i == nOldID) {
			nSel = nIndex;
		}
	}
	/* 選んでいたものが消えたら先頭を選ぶ（すぐ「追加」できるように） */
	if ((nSel < 0) && (SendMessage (hWndCombo, CB_GETCOUNT, 0, 0) > 0)) {
		nSel = 0;
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nSel, 0);
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：リスト更新															 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static void RenewList(void)
{
	/* リストへ登録 */
	LV_ITEM item;
	char szTmp[128];
	int i, nCount;
	HWND hWndTmp;

	hWndTmp = GetDlgItem (s_hWndDlg, IDC_LIST);

	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	ListView_DeleteAllItems (hWndTmp);

	if (s_pChar == NULL) {
		SetWindowText (GetDlgItem (s_hWndDlg, IDC_NAME), "");
		return;
	}

	nCount = g_aMagicInfo.GetSize ();
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		PMAGICINFO pInfo;

		pInfo = g_aMagicInfo[s_pChar->abyMagic[i]];
		item.iItem = i;

		/* 魔法名 */
		item.pszText = pInfo->szName;
		if (s_pChar->abyMagic[i] == 0) {
			item.pszText = "";
		}
		item.iSubItem = 0;
		ListView_InsertItem (hWndTmp, &item);
	}

	wsprintf (szTmp, "[%s]の所持魔法：", s_pChar->szName);
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_NAME);
	SetWindowText (hWndTmp, szTmp);
}

/* Copyright(C)URARA-works 2003-2005 */
