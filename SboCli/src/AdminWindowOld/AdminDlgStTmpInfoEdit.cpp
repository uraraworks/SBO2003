/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ステータステンプレート情報編集ダイアログ処理実装ファイル	 */
/* ファイル名：	AdminDlgStTmpInfoEdit.cpp									 */
/* 作成日：		2026/11/05													 */
/* ========================================================================= */
/*
 * MFC 版（AdminWindow/DlgAdminStTmpInfoEdit.cpp）と同じ編集画面。
 * リソースは IDD_STTMPINFO_EDIT。OK を押すとサーバーへ送る（追加なら追加として送る）。
 * 画面に無い項目（検索範囲など）は今の値のまま残す。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* テンプレート番号（-1 なら追加） */
static WORD s_waItem[MAX_MYITEM];								/* 編集中の持ち物 */
static BYTE s_abyMagic[MAX_USEMAGIC];							/* 編集中の特技 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgStTmpInfoEdit(HWND hWndParent, int nNo);							/* ダイアログ表示 */
void CloseDlgStTmpInfoEdit(void);												/* ダイアログ終了 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */

/* 数字の欄 */
static void SetNum(HWND hWnd, int nID, long lValue)
{
	char szTmp[32];
	snprintf (szTmp, sizeof (szTmp), "%ld", lValue);
	SetDlgItemText (hWnd, nID, szTmp);
}

static long GetNum(HWND hWnd, int nID)
{
	char szTmp[32];
	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	return atol (szTmp);
}

/* 持ち物・特技の一覧の表示文字列（枠番号：番号 名前） */
static void MakeSlotText(BOOL bItem, int nSlot, char *pszDst, int nSize)
{
	int nID = bItem ? (int)s_waItem[nSlot] : (int)s_abyMagic[nSlot];
	const char *pszName = "";

	if (nID) {
		if (bItem) {
			if (ItemIDValid ((WORD)nID) && g_aItemInfo[nID]) {
				pszName = g_aItemInfo[nID]->szName;
			}
		} else if (nID < g_aMagicInfo.GetSize () && g_aMagicInfo[nID]) {
			pszName = g_aMagicInfo[nID]->szName;
		}
	}
	if (nID == 0) {
		snprintf (pszDst, nSize, "%2d：", nSlot + 1);
	} else {
		snprintf (pszDst, nSize, "%2d：%d %s", nSlot + 1, nID, pszName);
	}
}

/* 一覧を作り直す（nSel は選び直す枠。-1 なら選ばない） */
static void RefreshSlotList(HWND hWnd, BOOL bItem, int nSel)
{
	int nID = bItem ? IDC_STT_ITEMLIST : IDC_STT_MAGICLIST;
	int nMax = bItem ? MAX_MYITEM : MAX_USEMAGIC;
	HWND hWndList = GetDlgItem (hWnd, nID);
	char szTmp[128];

	SendMessage (hWndList, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < nMax; i ++) {
		MakeSlotText (bItem, i, szTmp, sizeof (szTmp));
		SendMessage (hWndList, LB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	if (nSel >= 0) {
		SendMessage (hWndList, LB_SETCURSEL, nSel, 0);
	}
}

/* 番号欄の番号に合う名前を表示 */
static void ShowIDName(HWND hWnd, BOOL bItem)
{
	long lID = GetNum (hWnd, bItem ? IDC_STT_ITEMID : IDC_STT_MAGICID);
	const char *pszName = "（該当なし）";

	if (lID == 0) {
		pszName = "";
	} else if (bItem) {
		if (lID > 0 && lID < 0x10000 && ItemIDValid ((WORD)lID) && g_aItemInfo[(int)lID]) {
			pszName = g_aItemInfo[(int)lID]->szName;
		}
	} else if (lID > 0 && lID < g_aMagicInfo.GetSize () && lID < 256 && g_aMagicInfo[(int)lID]) {
		pszName = g_aMagicInfo[(int)lID]->szName;
	}
	SetDlgItemText (hWnd, bItem ? IDC_STT_ITEMNAME : IDC_STT_MAGICNAME, pszName);
}

/* 選んだ枠の設定・消去 */
static void SetSlot(HWND hWnd, BOOL bItem, BOOL bClear)
{
	HWND hWndList = GetDlgItem (hWnd, bItem ? IDC_STT_ITEMLIST : IDC_STT_MAGICLIST);
	int nSel = (int)SendMessage (hWndList, LB_GETCURSEL, 0, 0);
	long lID = bClear ? 0 : GetNum (hWnd, bItem ? IDC_STT_ITEMID : IDC_STT_MAGICID);

	if (nSel < 0) {
		return;
	}
	if (lID < 0) {
		return;
	}
	if (bItem) {
		if (lID >= 0x10000 || !ItemIDValid ((WORD)lID)) {
			return;
		}
		s_waItem[nSel] = (WORD)lID;
	} else {
		if (lID >= 256 || lID >= g_aMagicInfo.GetSize ()) {
			return;
		}
		s_abyMagic[nSel] = (BYTE)lID;
	}
	RefreshSlotList (hWnd, bItem, nSel);
}

/* ========================================================================= */
/* 関数名：ShowDlgStTmpInfoEdit												 */
/* 内容：ダイアログ表示														 */
/* ========================================================================= */
void ShowDlgStTmpInfoEdit(HWND hWndParent, int nNo)
{
	if (s_hWndDlg) {
		return;
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_STTMPINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgStTmpInfoEdit											 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgStTmpInfoEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
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
	STATUSTEMPLATEINFO stInfo;
	HWND hWndCombo = GetDlgItem (hWnd, IDC_MOVETYPE);

	/* 移動種別 */
	for (int i = 0; strcmp (aNpcMoveTypeTbl[i], "z") != 0; i ++) {
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)aNpcMoveTypeTbl[i]);
	}

	ZeroMemory (&stInfo, sizeof (stInfo));
	if (s_nNo >= 0 && s_nNo < g_aStTmpInfo.GetSize ()) {
		stInfo = *g_aStTmpInfo[s_nNo];
		SetWindowText (hWnd, "ステータステンプレート情報の編集");
	} else {
		s_nNo = -1;
		SetWindowText (hWnd, "ステータステンプレート情報の追加");
	}

	SetDlgItemText (hWnd, IDC_NAME, stInfo.szName);
	SendMessage (hWndCombo, CB_SETCURSEL, stInfo.byNpcType, 0);
	SetNum (hWnd, IDC_PW,		stInfo.byPW);
	SetNum (hWnd, IDC_DEX,		stInfo.byDEX);
	SetNum (hWnd, IDC_ST,		stInfo.byST);
	SetNum (hWnd, IDC_IT,		stInfo.byIT);
	SetNum (hWnd, IDC_LK,		stInfo.byLK);
	SetNum (hWnd, IDC_MAXHP,	stInfo.wMaxHP);
	SetNum (hWnd, IDC_MAXMANA,	stInfo.wMaxMana);
	SetNum (hWnd, IDC_GOLD,		(long)stInfo.dwGold);
	SetNum (hWnd, IDC_DROP,		stInfo.byDropProbability);
	SetNum (hWnd, IDC_MOVEX,	stInfo.byMoveProbabilityX);
	SetNum (hWnd, IDC_MOVEY,	stInfo.byMoveProbabilityY);
	SetNum (hWnd, IDC_REGMAGIC,	stInfo.wDFMagic);
	SetNum (hWnd, IDC_LV,		stInfo.byLevel);
	SetNum (hWnd, IDC_EXP,		(long)stInfo.dwExp);
	SetNum (hWnd, IDC_STRESIST,	stInfo.byStatusResist);
	CheckDlgButton (hWnd, IDC_NOFLOORDMG, stInfo.byNoFloorDamage ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton (hWnd, IDC_GHOST, stInfo.byGhost ? BST_CHECKED : BST_UNCHECKED);
	SetNum (hWnd, IDC_FIRE,		stInfo.chAttrFire);
	SetNum (hWnd, IDC_WATER,	stInfo.chAttrWater);
	SetNum (hWnd, IDC_WIND,		stInfo.chAttrWind);
	SetNum (hWnd, IDC_EARTH,	stInfo.chAttrEarth);
	SetNum (hWnd, IDC_LIGHT,	stInfo.chAttrLight);
	SetNum (hWnd, IDC_DARK,		stInfo.chAttrDark);

	/* 持ち物・特技 */
	CopyMemory (s_waItem, stInfo.waItem, sizeof (s_waItem));
	CopyMemory (s_abyMagic, stInfo.abyMagic, sizeof (s_abyMagic));
	RefreshSlotList (hWnd, TRUE, 0);
	RefreshSlotList (hWnd, FALSE, 0);
	SetNum (hWnd, IDC_STT_ITEMID, s_waItem[0]);
	SetNum (hWnd, IDC_STT_MAGICID, s_abyMagic[0]);
	ShowIDName (hWnd, TRUE);
	ShowIDName (hWnd, FALSE);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_STT_ITEMLIST:
	case IDC_STT_MAGICLIST:
		/* 選んだ枠の番号を番号欄に出す */
		if (nCodeNotify == LBN_SELCHANGE) {
			BOOL bItem = (nID == IDC_STT_ITEMLIST);
			int nSel = (int)SendDlgItemMessage (hWnd, nID, LB_GETCURSEL, 0, 0);
			if (nSel >= 0) {
				SetNum (hWnd, bItem ? IDC_STT_ITEMID : IDC_STT_MAGICID, bItem ? s_waItem[nSel] : s_abyMagic[nSel]);
				ShowIDName (hWnd, bItem);
			}
		}
		break;

	case IDC_STT_ITEMID:
	case IDC_STT_MAGICID:
		if (nCodeNotify == EN_CHANGE) {
			ShowIDName (hWnd, (nID == IDC_STT_ITEMID));
		}
		break;

	case IDC_STT_ITEMSET:	SetSlot (hWnd, TRUE, FALSE);	break;
	case IDC_STT_ITEMCLR:	SetSlot (hWnd, TRUE, TRUE);		break;
	case IDC_STT_MAGICSET:	SetSlot (hWnd, FALSE, FALSE);	break;
	case IDC_STT_MAGICCLR:	SetSlot (hWnd, FALSE, TRUE);	break;

	case IDOK:
		{
			PSTATUSTEMPLATEINFO pInfo;
			char szTmp[MAX_CHARNAMEBUF];

			if (s_nNo >= g_aStTmpInfo.GetSize ()) {
				/* 編集中に一覧が入れ替わった */
				CloseDlgStTmpInfoEdit ();
				break;
			}
			if (s_nNo < 0) {
				pInfo = new STATUSTEMPLATEINFO;
				ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
			} else {
				pInfo = g_aStTmpInfo[s_nNo];
			}

			ZeroMemory (szTmp, sizeof (szTmp));
			GetDlgItemText (hWnd, IDC_NAME, szTmp, sizeof (szTmp) - 1);
			szTmp[SboTextFitWidth (szTmp, MAX_CHARNAME_WIDTH)] = 0;		/* 画面に収まる長さまで */
			ZeroMemory (pInfo->szName, sizeof (pInfo->szName));
			strcpy (pInfo->szName, szTmp);

			int nMoveType = (int)SendDlgItemMessage (hWnd, IDC_MOVETYPE, CB_GETCURSEL, 0, 0);
			pInfo->byNpcType			= (BYTE)((nMoveType < 0) ? 0 : nMoveType);
			pInfo->byPW					= (BYTE)GetNum (hWnd, IDC_PW);
			pInfo->byDEX				= (BYTE)GetNum (hWnd, IDC_DEX);
			pInfo->byST					= (BYTE)GetNum (hWnd, IDC_ST);
			pInfo->byIT					= (BYTE)GetNum (hWnd, IDC_IT);
			pInfo->byLK					= (BYTE)GetNum (hWnd, IDC_LK);
			pInfo->wMaxHP				= (WORD)GetNum (hWnd, IDC_MAXHP);
			pInfo->wMaxMana				= (WORD)GetNum (hWnd, IDC_MAXMANA);
			pInfo->dwGold				= (DWORD)GetNum (hWnd, IDC_GOLD);
			pInfo->byDropProbability	= (BYTE)GetNum (hWnd, IDC_DROP);
			pInfo->byMoveProbabilityX	= (BYTE)GetNum (hWnd, IDC_MOVEX);
			pInfo->byMoveProbabilityY	= (BYTE)GetNum (hWnd, IDC_MOVEY);
			pInfo->wDFMagic				= (WORD)GetNum (hWnd, IDC_REGMAGIC);
			pInfo->byLevel				= (BYTE)GetNum (hWnd, IDC_LV);
			pInfo->dwExp				= (DWORD)GetNum (hWnd, IDC_EXP);
			pInfo->byStatusResist		= (BYTE)min (100, max (0, (int)GetNum (hWnd, IDC_STRESIST)));
			pInfo->byNoFloorDamage		= (IsDlgButtonChecked (hWnd, IDC_NOFLOORDMG) == BST_CHECKED) ? 1 : 0;
			pInfo->byGhost				= (IsDlgButtonChecked (hWnd, IDC_GHOST) == BST_CHECKED) ? 1 : 0;
			pInfo->chAttrFire			= (char)GetNum (hWnd, IDC_FIRE);
			pInfo->chAttrWater			= (char)GetNum (hWnd, IDC_WATER);
			pInfo->chAttrWind			= (char)GetNum (hWnd, IDC_WIND);
			pInfo->chAttrEarth			= (char)GetNum (hWnd, IDC_EARTH);
			pInfo->chAttrLight			= (char)GetNum (hWnd, IDC_LIGHT);
			pInfo->chAttrDark			= (char)GetNum (hWnd, IDC_DARK);
			/* 持ち物・特技（一覧で編集した値） */
			CopyMemory (pInfo->waItem, s_waItem, sizeof (pInfo->waItem));
			CopyMemory (pInfo->abyMagic, s_abyMagic, sizeof (pInfo->abyMagic));

			if (s_nNo < 0) {
				/* 追加はサーバーが一覧に足して送り返してくるので、手元には残さない */
				g_aStTmpInfo.Add (pInfo);
				SendCmdSTTMPLATE (g_aStTmpInfo.GetSize () - 1, CMDSTTMPLATETYPE_ADD);
				g_aStTmpInfo.RemoveAt (g_aStTmpInfo.GetSize () - 1);
				SAFE_DELETE (pInfo);
			} else {
				SendCmdSTTMPLATE ((DWORD)s_nNo, CMDSTTMPLATETYPE_CHG);
			}

			PostMessage (s_hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, IDD_STTMPINFO_EDIT), 0);
			CloseDlgStTmpInfoEdit ();
		}
		break;

	case IDCANCEL:
		CloseDlgStTmpInfoEdit ();
		break;
	}
}

/* Copyright(C)URARA-works 2026 */
