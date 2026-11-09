/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		明るさ・灯り・見え方の設定ダイアログ処理実装ファイル		 */
/* ファイル名：	AdminDlgMapLight.cpp										 */
/* 作成日：		2026/11/09													 */
/* ========================================================================= */
/*
 * 管理者ウィンドウの「マップ情報」から開く小窓。リソースは IDD_MAP_LIGHT。
 * マップ情報のうち、次の3つを一覧で編集する。
 *   ・明るさの範囲（暗くする／暗くしない／明るく壁で隠す。最大 MAX_MAPDARKAREA 個、下ほど優先）
 *   ・光源（最大 MAX_MAPLIGHT 個）
 *   ・見える範囲・見えない所のパーツ
 *
 * ・編集した内容はすぐ g_pMapInfo に入れるので、ゲーム画面でそのまま見た目を確かめられる。
 *   （サーバーへ送るのは「OK」の時。「キャンセル」なら開いた時の内容に戻す）
 * ・ゲーム画面のクリックで座標を拾う。範囲は1回目で左上、2回目で右下。
 *   光源はクリックした所へ動かす。何も選んでいなければ、クリックした所に新しく足す。
 * ・開いている間は、範囲の枠と光源の印をゲーム画面に描く（SboCliDrawScreen.cpp の DrawMapLightEdit）。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../SboCliRecvCmd.h"
#include "AdminDlgMapLight.h"

/* ========================================================================= */
/* 定数・構造体の定義														 */
/* ========================================================================= */

#define	MAPLIGHT_TARGET_AREA	0						/* クリックで範囲を決める */
#define	MAPLIGHT_TARGET_LIGHT	1						/* クリックで光源を置く */

/* 開いた時の内容（キャンセルで戻す） */
struct MAPLIGHT_BACKUP {
	BYTE			byDarkAreaCount;
	MAPDARKAREA		aDarkArea[MAX_MAPDARKAREA];
	BYTE			byLightCount;
	MAPLIGHT		aLight[MAX_MAPLIGHT];
	BYTE			byViewRange;
	WORD			wViewHideParts;
};

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static DWORD s_dwMapID = 0;										/* 開いた時のマップID */
static MAPLIGHT_BACKUP s_stBackup;								/* 開いた時の内容 */
static BOOL s_bAdminPosBack = FALSE;							/* 開く前の「座標を拾う」 */
static BOOL s_bLoading = FALSE;									/* 入力欄に入れている最中(変更通知を無視) */
static int s_nTarget = MAPLIGHT_TARGET_AREA;					/* ゲーム画面のクリックで決めるもの */
static int s_nCorner = 0;										/* 範囲のクリック 0=次は左上 1=次は右下 */
static int s_nCornerX = 0, s_nCornerY = 0;						/* 1回目にクリックした所 */
static BYTE s_byNewAreaType = MAPDARK_DARK;						/* 新しく足す範囲の種類 */

static LPCSTR s_apszAreaType[] = {"暗くしない", "暗くする", "明るく壁で隠す"};	/* MAPDARK_* の順 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

extern void AdminMapInfoRefresh(void);							/* マップ情報画面の表示を直す(SboAdminWindow.cpp) */

static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);				/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);			/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);	/* メッセージハンドラ(WM_COMMAND) */
static void SaveBackup(void);												/* 今の内容を覚える */
static void LoadBackup(void);												/* 覚えた内容に戻す */
static BOOL IsChanged(void);												/* 開いた時から変わったか */
static void Finish(BOOL bSend);												/* 閉じる(bSend なら送る、でなければ戻す) */
static void RenewMap(void);													/* 内容が変わったので見た目を直す */
static void RenewAreaList(void);											/* 範囲の一覧を作り直す */
static void RenewLightList(void);											/* 光源の一覧を作り直す */
static void LoadAreaEdit(void);												/* 選んだ範囲を入力欄へ */
static void LoadLightEdit(void);											/* 選んだ光源を入力欄へ */
static void SaveAreaEdit(void);												/* 入力欄を選んだ範囲へ */
static void SaveLightEdit(void);											/* 入力欄を選んだ光源へ */
static void SaveViewEdit(void);												/* 見え方の入力欄をマップ情報へ */
static int AddArea(int x1, int y1, int x2, int y2);							/* 範囲を足す */
static int AddLight(int x, int y);											/* 光源を足す */
static void SetTarget(int nTarget);											/* クリックで決めるものを変える */
static void RenewClickText(void);											/* クリックの案内を直す */
static int GetSelArea(void);												/* 選んでいる範囲(無ければ -1) */
static int GetSelLight(void);												/* 選んでいる光源(無ければ -1) */
static int GetNum(int nID, int nMin, int nMax);								/* 入力欄の数値(範囲に収める) */
static void SetNum(int nID, int nValue);									/* 入力欄に数値を入れる */
static int ClampX(int x);													/* マップの中に収める */
static int ClampY(int y);

/* ========================================================================= */
/* 関数名：ShowDlgMapLight													 */
/* 内容：ダイアログ表示(明るさ・灯り・見え方の設定)							 */
/* ========================================================================= */
void ShowDlgMapLight(HWND hWndParent)
{
	if (s_hWndDlg) {
		SetForegroundWindow (s_hWndDlg);
		return;
	}
	if (g_pMapInfo == NULL) {
		return;
	}

	s_hWndParent	= hWndParent;
	s_dwMapID		= g_pMapInfo->dwID;
	s_nTarget		= MAPLIGHT_TARGET_AREA;
	s_nCorner		= 0;
	SaveBackup ();

	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_MAP_LIGHT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	/* 管理者ウィンドウに重ねて出す（ゲーム画面はクリックに使うので隠さない） */
	{
		RECT rc;
		GetWindowRect (hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	/* 開いている間はゲーム画面のクリックで座標を拾う */
	s_bAdminPosBack = g_bAdminPos;
	g_bAdminPos = TRUE;

	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgMapLight													 */
/* 内容：ダイアログ終了(送っていない変更は送るか聞く)						 */
/* ========================================================================= */
void CloseDlgMapLight(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	if (IsChanged ()) {
		int nRet = MessageBox (s_hWndDlg, "明るさ・灯り・見え方の変更をサーバーに送りますか？\n（送らない時は元に戻します）",
				"明るさ・灯り・見え方の設定", MB_YESNO);
		Finish ((nRet == IDYES) ? TRUE : FALSE);
		return;
	}
	Finish (FALSE);
}

/* ========================================================================= */
/* 関数名：IsDlgMapLightOpen												 */
/* 内容：開いているか														 */
/* ========================================================================= */
BOOL IsDlgMapLightOpen(void)
{
	return (s_hWndDlg != NULL) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：MapLightDlgOnPos													 */
/* 内容：ゲーム画面がクリックされた(使ったら TRUE)							 */
/* ========================================================================= */
BOOL MapLightDlgOnPos(int x, int y)
{
	int nNo;

	if (s_hWndDlg == NULL) {
		return FALSE;
	}
	x = ClampX (x);
	y = ClampY (y);

	if (s_nTarget == MAPLIGHT_TARGET_LIGHT) {
		nNo = GetSelLight ();
		if (nNo < 0) {
			nNo = AddLight (x, y);
			if (nNo < 0) {
				MessageBox (s_hWndDlg, "光源はこれ以上置けません。", "明るさ・灯り・見え方の設定", MB_OK);
				return TRUE;
			}
		}
		g_pMapInfo->aLight[nNo].wX = (WORD)x;
		g_pMapInfo->aLight[nNo].wY = (WORD)y;
		RenewLightList ();
		SendDlgItemMessage (s_hWndDlg, IDC_ML_LIGHTLIST, LB_SETCURSEL, nNo, 0);
		LoadLightEdit ();
		RenewMap ();
		return TRUE;
	}

	/* 範囲：1回目は左上(1マスの範囲にする)、2回目は右下 */
	nNo = GetSelArea ();
	if (s_nCorner == 0 || nNo < 0) {
		if (nNo < 0) {
			nNo = AddArea (x, y, x, y);
			if (nNo < 0) {
				MessageBox (s_hWndDlg, "範囲はこれ以上足せません。", "明るさ・灯り・見え方の設定", MB_OK);
				return TRUE;
			}
		}
		s_nCornerX = x;
		s_nCornerY = y;
		s_nCorner = 1;
	} else {
		s_nCorner = 0;
	}
	{
		PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
		pArea->wX1 = (WORD)min (s_nCornerX, x);
		pArea->wY1 = (WORD)min (s_nCornerY, y);
		pArea->wX2 = (WORD)max (s_nCornerX, x);
		pArea->wY2 = (WORD)max (s_nCornerY, y);
	}
	RenewAreaList ();
	SendDlgItemMessage (s_hWndDlg, IDC_ML_AREALIST, LB_SETCURSEL, nNo, 0);
	LoadAreaEdit ();
	RenewClickText ();
	RenewMap ();
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MapLightDlgTick													 */
/* 内容：自分のマップが変わったら閉じる（描画のたびに呼ぶ）					 */
/* ========================================================================= */
void MapLightDlgTick(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	if ((g_pMapInfo == NULL) || (g_pMapInfo->dwID != s_dwMapID)) {
		/* 別のマップの情報になったので、戻さずに閉じる */
		SaveBackup ();
		Finish (FALSE);
	}
}

/* ========================================================================= */
/* 関数名：MapLightDlgOnSent												 */
/* 内容：マップ情報画面の「送信」で送った（今の内容を開いた時の内容にする）	 */
/* ========================================================================= */
void MapLightDlgOnSent(void)
{
	if (s_hWndDlg) {
		SaveBackup ();
	}
}

/* ========================================================================= */
/* 関数名：MapLightDlgGetSelArea / MapLightDlgGetSelLight					 */
/* 内容：ゲーム画面への描画用：選んでいる範囲・光源(無ければ -1)			 */
/* ========================================================================= */
int MapLightDlgGetSelArea(void)
{
	return (s_hWndDlg) ? GetSelArea () : -1;
}

int MapLightDlgGetSelLight(void)
{
	return (s_hWndDlg) ? GetSelLight () : -1;
}

/* ========================================================================= */
/* 関数名：MapLightDlgGetCorner												 */
/* 内容：範囲の左上だけクリックして右下を待っているなら TRUE とその場所		 */
/* ========================================================================= */
BOOL MapLightDlgGetCorner(int &x, int &y)
{
	if ((s_hWndDlg == NULL) || (s_nTarget != MAPLIGHT_TARGET_AREA) || (s_nCorner == 0)) {
		return FALSE;
	}
	x = s_nCornerX;
	y = s_nCornerY;
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MapDarkAreaLabel													 */
/* 内容：範囲 nNo を「暗くする 10,5 - 20,8」のような文字にする				 */
/* ========================================================================= */
void MapDarkAreaLabel(int nNo, LPSTR pszText, int nSize)
{
	PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
	BYTE byType = (pArea->byDark <= MAPDARK_HIDE) ? pArea->byDark : MAPDARK_DARK;

	_snprintf (pszText, nSize, "%d. %s  %d,%d - %d,%d", nNo + 1, s_apszAreaType[byType],
			pArea->wX1, pArea->wY1, pArea->wX2, pArea->wY2);
	pszText[nSize - 1] = 0;
}

/* ========================================================================= */
/* 関数名：MapLightLabel													 */
/* 内容：光源 nNo を「10,5  範囲3  強さ100%  暖色」のような文字にする		 */
/* ========================================================================= */
void MapLightLabel(int nNo, LPSTR pszText, int nSize)
{
	PMAPLIGHT pLight = &g_pMapInfo->aLight[nNo];
	int nPower = (pLight->byPower == 0) ? 100 : pLight->byPower;

	_snprintf (pszText, nSize, "%d. %d,%d  範囲%d  強さ%d%%%s%s", nNo + 1, pLight->wX, pLight->wY,
			pLight->byRange, nPower,
			(pLight->byFlg & MAPLIGHTFLG_NOFLICKER) ? "  ゆらめかない" : "",
			(pLight->byFlg & MAPLIGHTFLG_WARM) ? "  暖色" : "");
	pszText[nSize - 1] = 0;
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
	int i;
	char szTmp[256];

	s_hWndDlg = hWnd;
	s_bLoading = TRUE;

	if (g_pMapInfo->pszName && g_pMapInfo->pszName[0]) {
		_snprintf (szTmp, sizeof (szTmp), "明るさ・灯り・見え方の設定（%s）", (LPCSTR)g_pMapInfo->pszName);
		szTmp[sizeof (szTmp) - 1] = 0;
		SetWindowText (hWnd, szTmp);
	}

	for (i = 0; i < (int)(sizeof (s_apszAreaType) / sizeof (s_apszAreaType[0])); i ++) {
		SendDlgItemMessage (hWnd, IDC_ML_AREATYPE, CB_ADDSTRING, 0, (LPARAM)s_apszAreaType[i]);
	}
	SendDlgItemMessage (hWnd, IDC_ML_AREATYPE, CB_SETCURSEL, s_byNewAreaType, 0);

	SetNum (IDC_ML_VIEWRANGE, g_pMapInfo->byViewRange);
	SetNum (IDC_ML_HIDEPARTS, g_pMapInfo->wViewHideParts);
	s_bLoading = FALSE;

	RenewAreaList ();
	RenewLightList ();
	if (g_pMapInfo->byDarkAreaCount > 0) {
		SendDlgItemMessage (hWnd, IDC_ML_AREALIST, LB_SETCURSEL, 0, 0);
	}
	if (g_pMapInfo->byLightCount > 0) {
		SendDlgItemMessage (hWnd, IDC_ML_LIGHTLIST, LB_SETCURSEL, 0, 0);
	}
	LoadAreaEdit ();
	LoadLightEdit ();
	RenewClickText ();

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
	case IDOK:
		Finish (TRUE);
		break;

	case IDCANCEL:
		Finish (FALSE);
		break;

	/* ---- 明るさの範囲 ---- */
	case IDC_ML_AREALIST:
		if (nCodeNotify == LBN_SELCHANGE) {
			s_nCorner = 0;
			SetTarget (MAPLIGHT_TARGET_AREA);
			LoadAreaEdit ();
		}
		break;

	case IDC_ML_AREAADD:
		{
			/* 選んでいる範囲の右隣に同じ大きさで足す（無ければ自分の周り） */
			int x1, y1, x2, y2;
			nNo = GetSelArea ();
			if (nNo >= 0) {
				PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
				x1 = pArea->wX1;
				y1 = pArea->wY1;
				x2 = pArea->wX2;
				y2 = pArea->wY2;
			} else {
				PCHARINFO pChar = (g_nMyCharID >= 0) ? g_aCharInfo[g_nMyCharID] : NULL;
				int x = pChar ? (int)pChar->dwX : 0, y = pChar ? (int)pChar->dwY : 0;
				x1 = x - 2;
				y1 = y - 2;
				x2 = x + 2;
				y2 = y + 2;
			}
			nNo = AddArea (ClampX (x1), ClampY (y1), ClampX (x2), ClampY (y2));
			if (nNo < 0) {
				MessageBox (hWnd, "範囲はこれ以上足せません。", "明るさ・灯り・見え方の設定", MB_OK);
				break;
			}
			s_nCorner = 0;
			RenewAreaList ();
			SendDlgItemMessage (hWnd, IDC_ML_AREALIST, LB_SETCURSEL, nNo, 0);
			SetTarget (MAPLIGHT_TARGET_AREA);
			LoadAreaEdit ();
			RenewMap ();
		}
		break;

	case IDC_ML_AREADEL:
		nNo = GetSelArea ();
		if (nNo < 0) {
			break;
		}
		if (nNo < g_pMapInfo->byDarkAreaCount - 1) {
			MoveMemory (&g_pMapInfo->aDarkArea[nNo], &g_pMapInfo->aDarkArea[nNo + 1],
					sizeof (MAPDARKAREA) * (g_pMapInfo->byDarkAreaCount - 1 - nNo));
		}
		g_pMapInfo->byDarkAreaCount --;
		ZeroMemory (&g_pMapInfo->aDarkArea[g_pMapInfo->byDarkAreaCount], sizeof (MAPDARKAREA));
		s_nCorner = 0;
		RenewAreaList ();
		nNo = min (nNo, (int)g_pMapInfo->byDarkAreaCount - 1);
		SendDlgItemMessage (hWnd, IDC_ML_AREALIST, LB_SETCURSEL, nNo, 0);
		LoadAreaEdit ();
		RenewMap ();
		break;

	case IDC_ML_AREAUP:
	case IDC_ML_AREADOWN:
		{
			int nTo;
			nNo = GetSelArea ();
			if (nNo < 0) {
				break;
			}
			nTo = (nID == IDC_ML_AREAUP) ? nNo - 1 : nNo + 1;
			if (nTo < 0 || nTo >= g_pMapInfo->byDarkAreaCount) {
				break;
			}
			MAPDARKAREA stTmp = g_pMapInfo->aDarkArea[nNo];
			g_pMapInfo->aDarkArea[nNo] = g_pMapInfo->aDarkArea[nTo];
			g_pMapInfo->aDarkArea[nTo] = stTmp;
			RenewAreaList ();
			SendDlgItemMessage (hWnd, IDC_ML_AREALIST, LB_SETCURSEL, nTo, 0);
			LoadAreaEdit ();
			RenewMap ();
		}
		break;

	case IDC_ML_AREATYPE:
		if (nCodeNotify == CBN_SELCHANGE && !s_bLoading) {
			int nSel = (int)SendDlgItemMessage (hWnd, IDC_ML_AREATYPE, CB_GETCURSEL, 0, 0);
			if (nSel >= 0) {
				s_byNewAreaType = (BYTE)nSel;
			}
			SetTarget (MAPLIGHT_TARGET_AREA);
			SaveAreaEdit ();
		}
		break;

	case IDC_ML_AREAX1:
	case IDC_ML_AREAY1:
	case IDC_ML_AREAX2:
	case IDC_ML_AREAY2:
		if (nCodeNotify == EN_SETFOCUS) {
			SetTarget (MAPLIGHT_TARGET_AREA);
		} else if (nCodeNotify == EN_CHANGE && !s_bLoading) {
			s_nCorner = 0;
			SaveAreaEdit ();
		}
		break;

	/* ---- 光源 ---- */
	case IDC_ML_LIGHTLIST:
		if (nCodeNotify == LBN_SELCHANGE) {
			SetTarget (MAPLIGHT_TARGET_LIGHT);
			LoadLightEdit ();
		}
		break;

	case IDC_ML_LIGHTADD:
		{
			PCHARINFO pChar = (g_nMyCharID >= 0) ? g_aCharInfo[g_nMyCharID] : NULL;
			nNo = AddLight (ClampX (pChar ? (int)pChar->dwX : 0), ClampY (pChar ? (int)pChar->dwY : 0));
			if (nNo < 0) {
				MessageBox (hWnd, "光源はこれ以上置けません。", "明るさ・灯り・見え方の設定", MB_OK);
				break;
			}
			RenewLightList ();
			SendDlgItemMessage (hWnd, IDC_ML_LIGHTLIST, LB_SETCURSEL, nNo, 0);
			SetTarget (MAPLIGHT_TARGET_LIGHT);
			LoadLightEdit ();
			RenewMap ();
		}
		break;

	case IDC_ML_LIGHTDEL:
		nNo = GetSelLight ();
		if (nNo < 0) {
			break;
		}
		if (nNo < g_pMapInfo->byLightCount - 1) {
			MoveMemory (&g_pMapInfo->aLight[nNo], &g_pMapInfo->aLight[nNo + 1],
					sizeof (MAPLIGHT) * (g_pMapInfo->byLightCount - 1 - nNo));
		}
		g_pMapInfo->byLightCount --;
		ZeroMemory (&g_pMapInfo->aLight[g_pMapInfo->byLightCount], sizeof (MAPLIGHT));
		RenewLightList ();
		nNo = min (nNo, (int)g_pMapInfo->byLightCount - 1);
		SendDlgItemMessage (hWnd, IDC_ML_LIGHTLIST, LB_SETCURSEL, nNo, 0);
		LoadLightEdit ();
		RenewMap ();
		break;

	case IDC_ML_LIGHTX:
	case IDC_ML_LIGHTY:
	case IDC_ML_LIGHTRANGE:
	case IDC_ML_LIGHTPOWER:
		if (nCodeNotify == EN_SETFOCUS) {
			SetTarget (MAPLIGHT_TARGET_LIGHT);
		} else if (nCodeNotify == EN_CHANGE && !s_bLoading) {
			SaveLightEdit ();
		}
		break;

	case IDC_ML_NOFLICKER:
	case IDC_ML_WARM:
		if (!s_bLoading) {
			SetTarget (MAPLIGHT_TARGET_LIGHT);
			SaveLightEdit ();
		}
		break;

	/* ---- 見え方 ---- */
	case IDC_ML_VIEWRANGE:
	case IDC_ML_HIDEPARTS:
		if (nCodeNotify == EN_CHANGE && !s_bLoading) {
			SaveViewEdit ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SaveBackup / LoadBackup / IsChanged								 */
/* 内容：開いた時の内容を覚える・戻す・比べる								 */
/* ========================================================================= */
static void SaveBackup(void)
{
	if (g_pMapInfo == NULL) {
		return;
	}
	ZeroMemory (&s_stBackup, sizeof (s_stBackup));
	s_stBackup.byDarkAreaCount	= g_pMapInfo->byDarkAreaCount;
	CopyMemory (s_stBackup.aDarkArea, g_pMapInfo->aDarkArea, sizeof (s_stBackup.aDarkArea));
	s_stBackup.byLightCount		= g_pMapInfo->byLightCount;
	CopyMemory (s_stBackup.aLight, g_pMapInfo->aLight, sizeof (s_stBackup.aLight));
	s_stBackup.byViewRange		= g_pMapInfo->byViewRange;
	s_stBackup.wViewHideParts	= g_pMapInfo->wViewHideParts;
}

static void LoadBackup(void)
{
	if (g_pMapInfo == NULL) {
		return;
	}
	g_pMapInfo->byDarkAreaCount	= s_stBackup.byDarkAreaCount;
	CopyMemory (g_pMapInfo->aDarkArea, s_stBackup.aDarkArea, sizeof (s_stBackup.aDarkArea));
	g_pMapInfo->byLightCount	= s_stBackup.byLightCount;
	CopyMemory (g_pMapInfo->aLight, s_stBackup.aLight, sizeof (s_stBackup.aLight));
	g_pMapInfo->byViewRange		= s_stBackup.byViewRange;
	g_pMapInfo->wViewHideParts	= s_stBackup.wViewHideParts;
}

static BOOL IsChanged(void)
{
	if (g_pMapInfo == NULL) {
		return FALSE;
	}
	if ((g_pMapInfo->byDarkAreaCount != s_stBackup.byDarkAreaCount) ||
		(g_pMapInfo->byLightCount != s_stBackup.byLightCount) ||
		(g_pMapInfo->byViewRange != s_stBackup.byViewRange) ||
		(g_pMapInfo->wViewHideParts != s_stBackup.wViewHideParts)) {
		return TRUE;
	}
	if (memcmp (g_pMapInfo->aDarkArea, s_stBackup.aDarkArea, sizeof (s_stBackup.aDarkArea)) != 0) {
		return TRUE;
	}
	if (memcmp (g_pMapInfo->aLight, s_stBackup.aLight, sizeof (s_stBackup.aLight)) != 0) {
		return TRUE;
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：Finish															 */
/* 内容：閉じる(bSend なら送る、でなければ開いた時の内容に戻す)				 */
/* ========================================================================= */
static void Finish(BOOL bSend)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	if (bSend) {
		if (IsChanged ()) {
			SendCmdCHGMAPINFO ();
		}
	} else if (IsChanged ()) {
		LoadBackup ();
		RenewMap ();
	}

	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	g_bAdminPos = s_bAdminPosBack;
	s_nCorner = 0;
	AdminMapInfoRefresh ();
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：RenewMap															 */
/* 内容：内容が変わったので、ゲーム画面の暗さとマップ情報画面を直す			 */
/* ========================================================================= */
static void RenewMap(void)
{
	RenewCharPosInfo ();
	AdminMapInfoRefresh ();
}

/* ========================================================================= */
/* 関数名：RenewAreaList / RenewLightList									 */
/* 内容：一覧を作り直す（選んでいた行はそのまま）							 */
/* ========================================================================= */
static void RenewAreaList(void)
{
	int i, nSel;
	char szTmp[128];
	HWND hWndList = GetDlgItem (s_hWndDlg, IDC_ML_AREALIST);

	nSel = GetSelArea ();
	SendMessage (hWndList, LB_RESETCONTENT, 0, 0);
	for (i = 0; i < g_pMapInfo->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
		MapDarkAreaLabel (i, szTmp, sizeof (szTmp));
		SendMessage (hWndList, LB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	if (nSel >= 0 && nSel < g_pMapInfo->byDarkAreaCount) {
		SendMessage (hWndList, LB_SETCURSEL, nSel, 0);
	}

	_snprintf (szTmp, sizeof (szTmp), "%d / %d 個（下ほど優先）", g_pMapInfo->byDarkAreaCount, MAX_MAPDARKAREA);
	szTmp[sizeof (szTmp) - 1] = 0;
	SetDlgItemText (s_hWndDlg, IDC_ML_AREAINFO, szTmp);
}

static void RenewLightList(void)
{
	int i, nSel;
	char szTmp[128];
	HWND hWndList = GetDlgItem (s_hWndDlg, IDC_ML_LIGHTLIST);

	nSel = GetSelLight ();
	SendMessage (hWndList, LB_RESETCONTENT, 0, 0);
	for (i = 0; i < g_pMapInfo->byLightCount && i < MAX_MAPLIGHT; i ++) {
		MapLightLabel (i, szTmp, sizeof (szTmp));
		SendMessage (hWndList, LB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	if (nSel >= 0 && nSel < g_pMapInfo->byLightCount) {
		SendMessage (hWndList, LB_SETCURSEL, nSel, 0);
	}

	_snprintf (szTmp, sizeof (szTmp), "%d / %d 個", g_pMapInfo->byLightCount, MAX_MAPLIGHT);
	szTmp[sizeof (szTmp) - 1] = 0;
	SetDlgItemText (s_hWndDlg, IDC_ML_LIGHTINFO, szTmp);
}

/* ========================================================================= */
/* 関数名：LoadAreaEdit / LoadLightEdit										 */
/* 内容：選んだ範囲・光源を入力欄へ（選んでいなければ使えなくする）			 */
/* ========================================================================= */
static void LoadAreaEdit(void)
{
	static const int anID[] = {IDC_ML_AREAX1, IDC_ML_AREAY1, IDC_ML_AREAX2, IDC_ML_AREAY2,
			IDC_ML_AREADEL, IDC_ML_AREAUP, IDC_ML_AREADOWN};
	int i, nNo = GetSelArea ();
	BOOL bSel = (nNo >= 0) ? TRUE : FALSE;

	s_bLoading = TRUE;
	for (i = 0; i < (int)(sizeof (anID) / sizeof (anID[0])); i ++) {
		EnableWindow (GetDlgItem (s_hWndDlg, anID[i]), bSel);
	}
	if (bSel) {
		PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
		SendDlgItemMessage (s_hWndDlg, IDC_ML_AREATYPE, CB_SETCURSEL, (pArea->byDark <= MAPDARK_HIDE) ? pArea->byDark : MAPDARK_DARK, 0);
		SetNum (IDC_ML_AREAX1, pArea->wX1);
		SetNum (IDC_ML_AREAY1, pArea->wY1);
		SetNum (IDC_ML_AREAX2, pArea->wX2);
		SetNum (IDC_ML_AREAY2, pArea->wY2);
	} else {
		SendDlgItemMessage (s_hWndDlg, IDC_ML_AREATYPE, CB_SETCURSEL, s_byNewAreaType, 0);
		SetDlgItemText (s_hWndDlg, IDC_ML_AREAX1, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_AREAY1, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_AREAX2, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_AREAY2, "");
	}
	s_bLoading = FALSE;
}

static void LoadLightEdit(void)
{
	static const int anID[] = {IDC_ML_LIGHTX, IDC_ML_LIGHTY, IDC_ML_LIGHTRANGE, IDC_ML_LIGHTPOWER,
			IDC_ML_NOFLICKER, IDC_ML_WARM, IDC_ML_LIGHTDEL};
	int i, nNo = GetSelLight ();
	BOOL bSel = (nNo >= 0) ? TRUE : FALSE;

	s_bLoading = TRUE;
	for (i = 0; i < (int)(sizeof (anID) / sizeof (anID[0])); i ++) {
		EnableWindow (GetDlgItem (s_hWndDlg, anID[i]), bSel);
	}
	if (bSel) {
		PMAPLIGHT pLight = &g_pMapInfo->aLight[nNo];
		SetNum (IDC_ML_LIGHTX, pLight->wX);
		SetNum (IDC_ML_LIGHTY, pLight->wY);
		SetNum (IDC_ML_LIGHTRANGE, pLight->byRange);
		SetNum (IDC_ML_LIGHTPOWER, (pLight->byPower == 0) ? 100 : pLight->byPower);
		CheckDlgButton (s_hWndDlg, IDC_ML_NOFLICKER, (pLight->byFlg & MAPLIGHTFLG_NOFLICKER) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton (s_hWndDlg, IDC_ML_WARM, (pLight->byFlg & MAPLIGHTFLG_WARM) ? BST_CHECKED : BST_UNCHECKED);
	} else {
		SetDlgItemText (s_hWndDlg, IDC_ML_LIGHTX, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_LIGHTY, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_LIGHTRANGE, "");
		SetDlgItemText (s_hWndDlg, IDC_ML_LIGHTPOWER, "");
		CheckDlgButton (s_hWndDlg, IDC_ML_NOFLICKER, BST_UNCHECKED);
		CheckDlgButton (s_hWndDlg, IDC_ML_WARM, BST_UNCHECKED);
	}
	s_bLoading = FALSE;
}

/* ========================================================================= */
/* 関数名：SaveAreaEdit / SaveLightEdit / SaveViewEdit						 */
/* 内容：入力欄の内容をマップ情報へ入れる（入力中なので欄は書き換えない）	 */
/* ========================================================================= */
static void SaveAreaEdit(void)
{
	int nNo = GetSelArea (), x1, y1, x2, y2, nType;
	char szTmp[128];

	if (nNo < 0) {
		return;
	}
	PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
	nType = (int)SendDlgItemMessage (s_hWndDlg, IDC_ML_AREATYPE, CB_GETCURSEL, 0, 0);
	if (nType < MAPDARK_BRIGHT || nType > MAPDARK_HIDE) {
		nType = pArea->byDark;
	}
	x1 = ClampX (GetNum (IDC_ML_AREAX1, 0, 0xFFFF));
	y1 = ClampY (GetNum (IDC_ML_AREAY1, 0, 0xFFFF));
	x2 = ClampX (GetNum (IDC_ML_AREAX2, 0, 0xFFFF));
	y2 = ClampY (GetNum (IDC_ML_AREAY2, 0, 0xFFFF));
	pArea->byDark	= (BYTE)nType;
	pArea->wX1		= (WORD)min (x1, x2);
	pArea->wY1		= (WORD)min (y1, y2);
	pArea->wX2		= (WORD)max (x1, x2);
	pArea->wY2		= (WORD)max (y1, y2);

	/* 一覧のその行だけ直す */
	MapDarkAreaLabel (nNo, szTmp, sizeof (szTmp));
	SendDlgItemMessage (s_hWndDlg, IDC_ML_AREALIST, LB_DELETESTRING, nNo, 0);
	SendDlgItemMessage (s_hWndDlg, IDC_ML_AREALIST, LB_INSERTSTRING, nNo, (LPARAM)szTmp);
	SendDlgItemMessage (s_hWndDlg, IDC_ML_AREALIST, LB_SETCURSEL, nNo, 0);
	RenewMap ();
}

static void SaveLightEdit(void)
{
	int nNo = GetSelLight ();
	char szTmp[128];

	if (nNo < 0) {
		return;
	}
	PMAPLIGHT pLight = &g_pMapInfo->aLight[nNo];
	pLight->wX		= (WORD)ClampX (GetNum (IDC_ML_LIGHTX, 0, 0xFFFF));
	pLight->wY		= (WORD)ClampY (GetNum (IDC_ML_LIGHTY, 0, 0xFFFF));
	pLight->byRange	= (BYTE)GetNum (IDC_ML_LIGHTRANGE, 1, 7);
	pLight->byPower	= (BYTE)GetNum (IDC_ML_LIGHTPOWER, 1, 100);
	pLight->byFlg	= 0;
	if (IsDlgButtonChecked (s_hWndDlg, IDC_ML_NOFLICKER) == BST_CHECKED) {
		pLight->byFlg |= MAPLIGHTFLG_NOFLICKER;
	}
	if (IsDlgButtonChecked (s_hWndDlg, IDC_ML_WARM) == BST_CHECKED) {
		pLight->byFlg |= MAPLIGHTFLG_WARM;
	}

	MapLightLabel (nNo, szTmp, sizeof (szTmp));
	SendDlgItemMessage (s_hWndDlg, IDC_ML_LIGHTLIST, LB_DELETESTRING, nNo, 0);
	SendDlgItemMessage (s_hWndDlg, IDC_ML_LIGHTLIST, LB_INSERTSTRING, nNo, (LPARAM)szTmp);
	SendDlgItemMessage (s_hWndDlg, IDC_ML_LIGHTLIST, LB_SETCURSEL, nNo, 0);
	RenewMap ();
}

static void SaveViewEdit(void)
{
	int nParts;

	g_pMapInfo->byViewRange = (BYTE)GetNum (IDC_ML_VIEWRANGE, 0, 10);
	nParts = GetNum (IDC_ML_HIDEPARTS, 0, 0xFFFF);
	if (nParts >= g_wCountMapParts) {
		nParts = 0;
	}
	g_pMapInfo->wViewHideParts = (WORD)nParts;
	RenewMap ();
}

/* ========================================================================= */
/* 関数名：AddArea / AddLight												 */
/* 内容：後ろに足して番号を返す（いっぱいなら -1）							 */
/* ========================================================================= */
static int AddArea(int x1, int y1, int x2, int y2)
{
	int nNo = g_pMapInfo->byDarkAreaCount;

	if (nNo >= MAX_MAPDARKAREA) {
		return -1;
	}
	PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[nNo];
	ZeroMemory (pArea, sizeof (MAPDARKAREA));
	pArea->wX1		= (WORD)min (x1, x2);
	pArea->wY1		= (WORD)min (y1, y2);
	pArea->wX2		= (WORD)max (x1, x2);
	pArea->wY2		= (WORD)max (y1, y2);
	pArea->byDark	= s_byNewAreaType;
	g_pMapInfo->byDarkAreaCount ++;
	return nNo;
}

static int AddLight(int x, int y)
{
	int nNo = g_pMapInfo->byLightCount;

	if (nNo >= MAX_MAPLIGHT) {
		return -1;
	}
	PMAPLIGHT pLight = &g_pMapInfo->aLight[nNo];
	ZeroMemory (pLight, sizeof (MAPLIGHT));
	pLight->wX		= (WORD)x;
	pLight->wY		= (WORD)y;
	pLight->byRange	= 3;
	pLight->byPower	= 100;
	g_pMapInfo->byLightCount ++;
	return nNo;
}

/* ========================================================================= */
/* 関数名：SetTarget / RenewClickText										 */
/* 内容：ゲーム画面のクリックで決めるものと、その案内						 */
/* ========================================================================= */
static void SetTarget(int nTarget)
{
	if (s_nTarget == nTarget) {
		return;
	}
	s_nTarget = nTarget;
	s_nCorner = 0;
	RenewClickText ();
}

static void RenewClickText(void)
{
	char szTmp[160];
	int nNo;

	if (s_nTarget == MAPLIGHT_TARGET_LIGHT) {
		nNo = GetSelLight ();
		if (nNo >= 0) {
			_snprintf (szTmp, sizeof (szTmp), "ゲーム画面をクリック → 光源 %d をそこへ動かす", nNo + 1);
		} else {
			_snprintf (szTmp, sizeof (szTmp), "ゲーム画面をクリック → そこに光源を足す");
		}
	} else {
		nNo = GetSelArea ();
		if (nNo < 0) {
			_snprintf (szTmp, sizeof (szTmp), "ゲーム画面をクリック → そこを左上に範囲を足す（次で右下）");
		} else if (s_nCorner == 0) {
			_snprintf (szTmp, sizeof (szTmp), "ゲーム画面をクリック → 範囲 %d の左上（次で右下）", nNo + 1);
		} else {
			_snprintf (szTmp, sizeof (szTmp), "ゲーム画面をクリック → 範囲 %d の右下", nNo + 1);
		}
	}
	szTmp[sizeof (szTmp) - 1] = 0;
	SetDlgItemText (s_hWndDlg, IDC_ML_CLICK, szTmp);
}

/* ========================================================================= */
/* 関数名：GetSelArea / GetSelLight											 */
/* 内容：選んでいる行（無ければ -1）										 */
/* ========================================================================= */
static int GetSelArea(void)
{
	int nSel = (int)SendDlgItemMessage (s_hWndDlg, IDC_ML_AREALIST, LB_GETCURSEL, 0, 0);
	if (nSel < 0 || nSel >= g_pMapInfo->byDarkAreaCount) {
		return -1;
	}
	return nSel;
}

static int GetSelLight(void)
{
	int nSel = (int)SendDlgItemMessage (s_hWndDlg, IDC_ML_LIGHTLIST, LB_GETCURSEL, 0, 0);
	if (nSel < 0 || nSel >= g_pMapInfo->byLightCount) {
		return -1;
	}
	return nSel;
}

/* ========================================================================= */
/* 関数名：GetNum / SetNum													 */
/* 内容：入力欄の数値													 */
/* ========================================================================= */
static int GetNum(int nID, int nMin, int nMax)
{
	char szTmp[16];
	int nValue;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (s_hWndDlg, nID, szTmp, sizeof (szTmp) - 1);
	nValue = atoi (szTmp);
	nValue = max (nValue, nMin);
	nValue = min (nValue, nMax);
	return nValue;
}

static void SetNum(int nID, int nValue)
{
	char szTmp[16];

	wsprintf (szTmp, "%d", nValue);
	SetDlgItemText (s_hWndDlg, nID, szTmp);
}

/* ========================================================================= */
/* 関数名：ClampX / ClampY													 */
/* 内容：マップの中に収める													 */
/* ========================================================================= */
static int ClampX(int x)
{
	return max (0, min (x, max (0, g_pMapInfo->nCX - 1)));
}

static int ClampY(int y)
{
	return max (0, min (y, max (0, g_pMapInfo->nCY - 1)));
}

/* Copyright(C)URARA-works 2026 */
