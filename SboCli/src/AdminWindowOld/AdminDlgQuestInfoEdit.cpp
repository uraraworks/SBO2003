/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		ギルドの依頼編集ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgQuestInfoEdit.cpp									 */
/* 作成日：		2026/11/12													 */
/* ========================================================================= */
/*
 * ギルドの依頼 1 件の編集画面。リソースは IDD_QUESTINFO_EDIT。
 * 左が依頼そのもの（名前・台詞・報酬・段ごとの目的・討伐チャレンジの設定）、
 * 右が「見えるNPC」（8 つの欄を上の一覧で切り替えて直す）。
 * 画面は枠（GROUPBOX）ごとのかたまりに分けてあり、依頼の種類・見えるNPCの種類で
 * 使わない枠は隠して下の枠を上へ詰める（s_aSectionRule）。高さが足りなくなったら
 * 右にスクロールバーを出す。種類が増えた時は、リソースに枠を足して
 * s_aSectionRule に「どの種類で出すか」を 1 行足せばよい。
 * ゲーム画面を左クリックすると選んでいる見えるNPCの出る場所、右クリックすると
 * 行き先（ポータル・討伐チャレンジの入口）に、いま居るマップの座標が入る。
 * OK でサーバーへ送り、サーバーはデータAPIと同じ確認を通して書き込む。
 * うまくいけば閉じ、だめならサーバーからの理由を出して開いたままにする。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminDlgQuestInfo.h"
#include "AdminDlgQuestInfoEdit.h"
#include <string>
#include <vector>
#include <algorithm>
#include <climits>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* 依頼の配列の番号（-1 なら追加） */
static int s_nNpcSel = 0;										/* いま右側に出している見えるNPCの欄 */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static QUESTINFO s_Info;										/* 編集中の内容 */

#define QUESTEDIT_TITLE		"ギルドの依頼"
#define QUESTEDIT_MAXCY		380		/* 画面の高さの上限（ダイアログ単位）。越えたらスクロール */
#define QUESTEDIT_GAP		2		/* 枠どうしのすき間（ダイアログ単位） */
#define QUESTEDIT_SCROLLCX	16		/* スクロールバーの幅 */
#define QUESTEDIT_WHEEL		36		/* ホイール 1 目盛りで動かすドット数 */

/* 枠（かたまり）ごとの部品と元の位置（クライアント座標） */
struct QE_CTRL {
	HWND	hWnd;
	RECT	rc;
};
struct QE_SECTION {
	int		nGroupID;			/* 枠の GROUPBOX の ID */
	int		nCol;				/* 0=左（依頼） 1=右（見えるNPC） */
	RECT	rc;					/* 枠の元の位置 */
	std::vector<QE_CTRL> aCtrl;	/* 枠の中の部品（枠そのものも含む） */
};
static std::vector<QE_SECTION>	s_aSection;						/* 枠の一覧（列・上から順） */
static std::vector<QE_CTRL>		s_aButton;						/* OK・キャンセル（中身の下に付いていく） */
static HWND s_hWndScroll = NULL;								/* 縦スクロールバー */
static int s_nScroll = 0;										/* スクロールの位置（ドット） */
static int s_nScrollMax = 0;									/* スクロールの最大 */
static int s_nScrollPage = 0;									/* 見えている高さ */
static int s_nBaseCX = 0;										/* 元のクライアントの幅 */
static int s_nMaxCY = 0;										/* クライアントの高さの上限 */
static int s_nGap = 0;											/* 枠どうしのすき間（ドット） */
static SIZE s_sizeFrame;										/* 窓の枠・タイトルの分の大きさ */

/* 見えるNPCの種類の名前（QUESTNPCKIND_*） */
static LPCSTR NpcKindName(int nKind)
{
	static const LPCSTR s_apszName[] = {
		"なし（使わない）", "ワープポータル", "宝箱", "人（台詞だけ）", "討伐チャレンジの入口", "討伐チャレンジの敵",
	};
	if ((nKind >= 0) && (nKind < (int)(sizeof (s_apszName) / sizeof (s_apszName[0])))) {
		return s_apszName[nKind];
	}
	return "？";
}

/* 見えるNPCの種類の説明（種類の下に 1 行で出す） */
static LPCSTR NpcKindHelp(int nKind)
{
	switch (nKind) {
	case QUESTNPCKIND_NONE:		return "この欄は使いません。種類を選ぶと設定が出ます。";
	case QUESTNPCKIND_PORTAL:	return "話すと行き先へ移ります。";
	case QUESTNPCKIND_CHEST:	return "話すと品がもらえます（依頼の品を渡す時など）。";
	case QUESTNPCKIND_PERSON:	return "話すと台詞を言うだけの人です。";
	case QUESTNPCKIND_GATE:		return "乗ると討伐チャレンジが始まり、仲間と行き先へ移ります。";
	case QUESTNPCKIND_ENEMY:	return "討伐チャレンジが始まると出ます。全部倒すとクリア。";
	}
	return "";
}

/* 依頼の種類（byChallenge）で使える見えるNPCの種類か（入口・敵は討伐チャレンジだけ） */
static BOOL NpcKindAllowed(int nType, int nKind)
{
	if ((nKind == QUESTNPCKIND_GATE) || (nKind == QUESTNPCKIND_ENEMY)) {
		return (nType != QUESTCHALLENGE_NONE) ? TRUE : FALSE;
	}
	return TRUE;
}

/* 見えるNPCの種類ごとに使う欄 */
static BOOL NpcKindHasDest(int nKind)	{ return ((nKind == QUESTNPCKIND_PORTAL) || (nKind == QUESTNPCKIND_GATE)) ? TRUE : FALSE; }
static BOOL NpcKindHasItem(int nKind)	{ return (nKind == QUESTNPCKIND_CHEST) ? TRUE : FALSE; }
static BOOL NpcKindHasStTmp(int nKind)	{ return (nKind == QUESTNPCKIND_ENEMY) ? TRUE : FALSE; }

/* 枠を出すか（nType=依頼の種類 byChallenge、nKind=選んでいる見えるNPCの種類）
   種類が増えたら、リソースに枠を足してここに 1 行足す */
static BOOL SectionVisible(int nGroupID, int nType, int nKind)
{
	switch (nGroupID) {
	case IDC_Q_GRP_SEARCH:		return (nType == QUESTCHALLENGE_NONE) ? TRUE : FALSE;	/* 探し物（依頼の品） */
	case IDC_Q_GRP_CHALLENGE:	return (nType != QUESTCHALLENGE_NONE) ? TRUE : FALSE;	/* 討伐チャレンジ（人数・貸し出し） */
	case IDC_Q_GRP_NPCCOMMON:	return (nKind != QUESTNPCKIND_NONE) ? TRUE : FALSE;
	case IDC_Q_GRP_NPCDEST:		return NpcKindHasDest (nKind);
	case IDC_Q_GRP_NPCITEM:		return NpcKindHasItem (nKind);
	case IDC_Q_GRP_NPCENEMY:	return NpcKindHasStTmp (nKind);
	}
	return TRUE;	/* 依頼・目的・見えるNPCの選択はいつも出す */
}

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void LoadNpc(HWND hWnd, int nNo);										/* 見えるNPCの欄を画面に出す */
static BOOL SaveNpc(HWND hWnd, int nNo);										/* 画面の見えるNPCを覚える */
static BOOL SaveAll(HWND hWnd);													/* 画面の内容を全部覚える */
static void RenewNpcSel(HWND hWnd);												/* 見えるNPCの一覧の文字を作り直す */
static void LayoutInit(HWND hWnd);												/* 枠ごとの部品と元の位置を覚える */
static void RenewLayout(HWND hWnd);												/* 種類に合わせて使う枠だけ出して詰める */
static void OnScrollMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);		/* スクロールバー・ホイール */
static void FillKindCombo(HWND hWnd, int nKind);								/* 見えるNPCの種類の一覧を作る */
static int GetNpcKind(HWND hWnd);												/* 選んでいる見えるNPCの種類 */

/* ========================================================================= */
/* 小物																		 */
/* ========================================================================= */

/* 数字の欄 */
static void SetNum(HWND hWnd, int nID, unsigned long ulValue)
{
	char szTmp[32];
	snprintf (szTmp, sizeof (szTmp), "%lu", ulValue);
	SetDlgItemText (hWnd, nID, szTmp);
}

static unsigned long GetNum(HWND hWnd, int nID)
{
	char szTmp[32];
	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	return strtoul (szTmp, NULL, 10);
}

/* 数字の欄（範囲外なら理由を出して FALSE） */
static BOOL GetNumRange(HWND hWnd, int nID, unsigned long ulMax, LPCSTR pszName, unsigned long &ulOut)
{
	char szMsg[160];

	ulOut = GetNum (hWnd, nID);
	if (ulOut > ulMax) {
		snprintf (szMsg, sizeof (szMsg), "%sは 0〜%lu で入れてください。", pszName, ulMax);
		MessageBox (s_hWndDlg, szMsg, QUESTEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, nID));
		return FALSE;
	}
	return TRUE;
}

/* 文字の欄（複数行は データの "\n" と画面の "\r\n" を変換する） */
static void SetText(HWND hWnd, int nID, LPCSTR pszText, BOOL bMultiLine)
{
	std::string str;

	if (!bMultiLine) {
		SetDlgItemText (hWnd, nID, pszText);
		return;
	}
	for (LPCSTR p = pszText; *p; p ++) {
		if (*p == '\r') {
			continue;
		}
		if (*p == '\n') {
			str += '\r';
		}
		str += *p;
	}
	SetDlgItemText (hWnd, nID, str.c_str ());
}

/* 欄の大きさに収まらなければ理由を出して FALSE */
static BOOL GetText(HWND hWnd, int nID, char *pszOut, int nOutSize, BOOL bMultiLine, LPCSTR pszName)
{
	char szTmp[2048], szMsg[200];
	int nLen = 0;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetDlgItemText (hWnd, nID, szTmp, sizeof (szTmp) - 1);
	/* 改行は "\n" だけにする（1 行の欄は改行を消す） */
	for (LPCSTR p = szTmp; *p; p ++) {
		if (*p == '\r') {
			continue;
		}
		if ((*p == '\n') && !bMultiLine) {
			continue;
		}
		if (nLen >= nOutSize - 1) {
			snprintf (szMsg, sizeof (szMsg), "%sが長すぎます。\n（UTF-8 で %d バイトまで。全角ならおよそ %d 文字まで）",
					pszName, nOutSize - 1, (nOutSize - 1) / 3);
			MessageBox (s_hWndDlg, szMsg, QUESTEDIT_TITLE, MB_OK);
			SetFocus (GetDlgItem (hWnd, nID));
			return FALSE;
		}
		pszOut[nLen ++] = *p;
	}
	pszOut[nLen] = 0;
	return TRUE;
}

/* 番号で選ぶ一覧（アイテム・ステータステンプレート）を作る。
   一覧の何番目＝番号。手元に無い番号が入っていた時は「(不明)」で足して値を残す */
static void FillItemCombo(HWND hWnd, int nID, int nValue)
{
	HWND hWndCombo = GetDlgItem (hWnd, nID);
	char szTmp[160];
	int i, nCount = g_aItemInfo.GetSize ();

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)"0: なし");
	for (i = 1; (i < nCount) || (i <= nValue); i ++) {
		PITEMINFO pItem = (i < nCount) ? g_aItemInfo[i] : NULL;
		snprintf (szTmp, sizeof (szTmp), "%d: %s", i, pItem ? pItem->szName : "(不明)");
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nValue, 0);
}

static void FillStTmpCombo(HWND hWnd, int nID, int nValue)
{
	HWND hWndCombo = GetDlgItem (hWnd, nID);
	char szTmp[160];
	int i, nCount = g_aStTmpInfo.GetSize ();

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	for (i = 0; (i < nCount) || (i <= nValue); i ++) {
		PSTATUSTEMPLATEINFO pInfo = (i < nCount) ? g_aStTmpInfo[i] : NULL;
		snprintf (szTmp, sizeof (szTmp), "%d: %s", i, pInfo ? pInfo->szName : "(不明)");
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendMessage (hWndCombo, CB_SETCURSEL, nValue, 0);
}

static int GetSel(HWND hWnd, int nID)
{
	int nSel = (int)SendDlgItemMessage (hWnd, nID, CB_GETCURSEL, 0, 0);
	return (nSel < 0) ? 0 : nSel;
}

/* 段の一覧（bAny: 先頭に「どの段でも／変えない」(=255) を置く） */
static void FillStepCombo(HWND hWnd, int nID, LPCSTR pszAny)
{
	HWND hWndCombo = GetDlgItem (hWnd, nID);
	char szTmp[32];

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	if (pszAny) {
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)pszAny);
	}
	for (int i = 0; i < MAX_QUESTSTEP; i ++) {
		snprintf (szTmp, sizeof (szTmp), "%d段目", i);
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
}

static void SetStepSel(HWND hWnd, int nID, BYTE byStep, BOOL bAny)
{
	int nSel;

	if (bAny) {
		nSel = (byStep == QUESTNPC_NOSTEP || byStep >= MAX_QUESTSTEP) ? 0 : byStep + 1;
	} else {
		nSel = (byStep >= MAX_QUESTSTEP) ? 0 : byStep;
	}
	SendDlgItemMessage (hWnd, nID, CB_SETCURSEL, nSel, 0);
}

static BYTE GetStepSel(HWND hWnd, int nID, BOOL bAny)
{
	int nSel = GetSel (hWnd, nID);

	if (bAny) {
		return (nSel == 0) ? QUESTNPC_NOSTEP : (BYTE)(nSel - 1);
	}
	return (BYTE)nSel;
}

/* 見えるNPCの欄を空にする（段は「どの段でも」「変えない」） */
static void ClearNpc(QUESTNPCINFO *pNpc)
{
	ZeroMemory (pNpc, sizeof (QUESTNPCINFO));
	pNpc->byStepMax = QUESTNPC_NOSTEP;
	pNpc->byNextStep = QUESTNPC_NOSTEP;
}

/* ========================================================================= */
/* 関数名：ShowDlgQuestInfoEdit												 */
/* 内容：ダイアログ表示（nNo=-1 なら追加）									 */
/* ========================================================================= */
void ShowDlgQuestInfoEdit(HWND hWndParent, int nNo)
{
	const QUESTINFO *pInfo;

	if (s_hWndDlg) {
		MessageBox (s_hWndDlg, "編集中の依頼があります。閉じてから開いてください。", QUESTEDIT_TITLE, MB_OK);
		return;
	}

	pInfo = AdminQuestGet (nNo);
	if (pInfo) {
		s_nNo = nNo;
		s_Info = *pInfo;
	} else {
		/* 追加はデータAPIと同じ初めの値（出す・段は 1 つ） */
		s_nNo = -1;
		ZeroMemory (&s_Info, sizeof (s_Info));
		s_Info.byState = QUESTSTATE_ON;
		s_Info.byStepCount = 1;
		for (int i = 0; i < MAX_QUESTNPC; i ++) {
			ClearNpc (&s_Info.aNpc[i]);
		}
	}
	s_nNpcSel = 0;
	s_bWait = FALSE;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_QUESTINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	/* 管理者ウィンドウに重ねて出す（ゲーム画面はクリックに使うので隠さない） */
	{
		RECT rc;
		GetWindowRect (g_hWndAdmin ? g_hWndAdmin : hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgQuestInfoEdit											 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgQuestInfoEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_bWait = FALSE;
	s_aSection.clear ();
	s_aButton.clear ();
	s_hWndScroll = NULL;
}

/* ========================================================================= */
/* 関数名：IsDlgQuestInfoEditOpen											 */
/* 内容：開いているか														 */
/* ========================================================================= */
BOOL IsDlgQuestInfoEditOpen(void)
{
	return (s_hWndDlg != NULL) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：QuestEditOnResult												 */
/* 内容：書き込みの結果が届いた												 */
/* ========================================================================= */
void QuestEditOnResult(BOOL bOK, int nNo, LPCSTR pszMsg)
{
	char szMsg[MAX_ADMINQUESTMSG + 64];

	if ((s_hWndDlg == NULL) || !s_bWait) {
		return;
	}
	s_bWait = FALSE;
	EnableWindow (GetDlgItem (s_hWndDlg, IDOK), TRUE);
	if (!bOK) {
		snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", (pszMsg && pszMsg[0]) ? pszMsg : "");
		MessageBox (s_hWndDlg, szMsg, QUESTEDIT_TITLE, MB_OK);
		return;
	}
	CloseDlgQuestInfoEdit ();
}

/* ========================================================================= */
/* 関数名：QuestEditOnPos													 */
/* 内容：ゲーム画面がクリックされた(使ったら TRUE)							 */
/*       左クリック＝出る場所、右クリック＝行き先							 */
/* ========================================================================= */
BOOL QuestEditOnPos(int x, int y, BOOL bRight)
{
	char szTmp[160];
	DWORD dwMapID;

	if ((s_hWndDlg == NULL) || (g_pMapInfo == NULL)) {
		return FALSE;
	}
	dwMapID = g_pMapInfo->dwID;
	{
		int nKind = GetNpcKind (s_hWndDlg);
		if ((nKind == QUESTNPCKIND_NONE) || (bRight && !NpcKindHasDest (nKind))) {
			/* その欄が出ていない種類の時は何もしない（見えない所に値を入れない） */
			return TRUE;
		}
	}
	if (bRight) {
		SetNum (s_hWndDlg, IDC_Q_NPCDESTMAP, dwMapID);
		SetNum (s_hWndDlg, IDC_Q_NPCDESTX, (unsigned long)x);
		SetNum (s_hWndDlg, IDC_Q_NPCDESTY, (unsigned long)y);
		snprintf (szTmp, sizeof (szTmp), "→ %d番目の行き先：マップ%lu (%d, %d)",
				s_nNpcSel + 1, (unsigned long)dwMapID, x, y);
	} else {
		SetNum (s_hWndDlg, IDC_Q_NPCMAP, dwMapID);
		SetNum (s_hWndDlg, IDC_Q_NPCX, (unsigned long)x);
		SetNum (s_hWndDlg, IDC_Q_NPCY, (unsigned long)y);
		snprintf (szTmp, sizeof (szTmp), "→ %d番目の出る場所：マップ%lu (%d, %d)",
				s_nNpcSel + 1, (unsigned long)dwMapID, x, y);
	}
	SetDlgItemText (s_hWndDlg, IDC_Q_CLICK, szTmp);
	return TRUE;
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
		CloseDlgQuestInfoEdit ();
		return 1;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		OnScrollMsg (hWnd, msg, wParam, lParam);
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
	char szTmp[128], szBuf[64];
	int i;

	LayoutInit (hWnd);
	if (s_nNo >= 0) {
		snprintf (szTmp, sizeof (szTmp), "ギルドの依頼の編集（%s）", s_Info.szName);
		SetWindowText (hWnd, szTmp);
		snprintf (szTmp, sizeof (szTmp), "依頼番号：%u", (unsigned)s_Info.wID);
	} else {
		SetWindowText (hWnd, "ギルドの依頼の追加");
		snprintf (szTmp, sizeof (szTmp), "依頼番号：足す時に自動");
	}
	SetDlgItemText (hWnd, IDC_Q_ID, szTmp);

	/* 依頼 */
	SetText (hWnd, IDC_Q_NAME, s_Info.szName, FALSE);
	SendDlgItemMessage (hWnd, IDC_Q_STATE, CB_ADDSTRING, 0, (LPARAM)"止める");
	SendDlgItemMessage (hWnd, IDC_Q_STATE, CB_ADDSTRING, 0, (LPARAM)"出す");
	SendDlgItemMessage (hWnd, IDC_Q_STATE, CB_SETCURSEL, (s_Info.byState == QUESTSTATE_ON) ? 1 : 0, 0);
	SetNum (hWnd, IDC_Q_GUILD, s_Info.byGuild);
	SetNum (hWnd, IDC_Q_SORT, s_Info.wSort);
	for (i = 1; i <= MAX_QUESTSTEP; i ++) {
		snprintf (szTmp, sizeof (szTmp), "%d", i);
		SendDlgItemMessage (hWnd, IDC_Q_STEPCOUNT, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendDlgItemMessage (hWnd, IDC_Q_STEPCOUNT, CB_SETCURSEL,
			((s_Info.byStepCount >= 1) && (s_Info.byStepCount <= MAX_QUESTSTEP)) ? s_Info.byStepCount - 1 : 0, 0);
	CheckDlgButton (hWnd, IDC_Q_ONCE, s_Info.byOnce ? BST_CHECKED : BST_UNCHECKED);
	SetText (hWnd, IDC_Q_EXPLAIN, s_Info.szExplain, TRUE);
	SetText (hWnd, IDC_Q_MSGACCEPT, s_Info.szMsgAccept, TRUE);
	SetText (hWnd, IDC_Q_MSGDONE, s_Info.szMsgDone, TRUE);
	SetNum (hWnd, IDC_Q_GOLD, s_Info.dwGold);
	SetNum (hWnd, IDC_Q_EXP, s_Info.dwExp);
	FillItemCombo (hWnd, IDC_Q_REWARDITEM, s_Info.wRewardItem);
	FillItemCombo (hWnd, IDC_Q_QUESTITEM, s_Info.wQuestItem);
	SetText (hWnd, IDC_Q_GOAL0, s_Info.aszGoal[0], FALSE);
	SetText (hWnd, IDC_Q_GOAL1, s_Info.aszGoal[1], FALSE);
	SetText (hWnd, IDC_Q_GOAL2, s_Info.aszGoal[2], FALSE);
	SetText (hWnd, IDC_Q_GOAL3, s_Info.aszGoal[3], FALSE);

	/* 依頼の種類（ふつう・討伐チャレンジ）と討伐チャレンジ */
	for (i = 0; (i < QUESTCHALLENGE_MAX) || (i <= s_Info.byChallenge); i ++) {
		if (i == QUESTCHALLENGE_NONE) {
			snprintf (szTmp, sizeof (szTmp), "ふつうの依頼（探し物など）");
		} else {
			snprintf (szTmp, sizeof (szTmp), "%s", AdminQuestChallengeName (i, szBuf, sizeof (szBuf)));
		}
		SendDlgItemMessage (hWnd, IDC_Q_CHALLENGE, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendDlgItemMessage (hWnd, IDC_Q_CHALLENGE, CB_SETCURSEL, s_Info.byChallenge, 0);
	SetNum (hWnd, IDC_Q_MEMBERMIN, s_Info.byMemberMin);
	FillStTmpCombo (hWnd, IDC_Q_LOANSTTMP, s_Info.wLoanStTmp);

	/* 見えるNPC（種類の一覧は LoadNpc で依頼の種類に合わせて作る） */
	SendDlgItemMessage (hWnd, IDC_Q_NPCDIR, CB_ADDSTRING, 0, (LPARAM)"上");
	SendDlgItemMessage (hWnd, IDC_Q_NPCDIR, CB_ADDSTRING, 0, (LPARAM)"下");
	SendDlgItemMessage (hWnd, IDC_Q_NPCDIR, CB_ADDSTRING, 0, (LPARAM)"左");
	SendDlgItemMessage (hWnd, IDC_Q_NPCDIR, CB_ADDSTRING, 0, (LPARAM)"右");
	FillStepCombo (hWnd, IDC_Q_NPCSTEPMIN, NULL);
	FillStepCombo (hWnd, IDC_Q_NPCSTEPMAX, "どの段でも");
	FillStepCombo (hWnd, IDC_Q_NPCNEXTSTEP, "変えない");
	RenewNpcSel (hWnd);
	LoadNpc (hWnd, s_nNpcSel);

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_Q_NPCSEL:		/* 見えるNPCの欄を切り替える */
		if (nCodeNotify == CBN_SELCHANGE) {
			int nSel = GetSel (hWnd, IDC_Q_NPCSEL);
			if (nSel == s_nNpcSel) {
				break;
			}
			if (!SaveNpc (hWnd, s_nNpcSel)) {
				/* 直してもらうため、元の欄に戻す */
				SendDlgItemMessage (hWnd, IDC_Q_NPCSEL, CB_SETCURSEL, s_nNpcSel, 0);
				break;
			}
			s_nNpcSel = nSel;
			RenewNpcSel (hWnd);
			LoadNpc (hWnd, s_nNpcSel);
		}
		break;

	case IDC_Q_CHALLENGE:	/* 依頼の種類が変わったら、使う枠と見えるNPCの種類の一覧を切り替える */
		if (nCodeNotify == CBN_SELCHANGE) {
			FillKindCombo (hWnd, GetNpcKind (hWnd));
			RenewLayout (hWnd);
		}
		break;

	case IDC_Q_NPCKIND:		/* 種類が変わったら使う枠を切り替える */
		if (nCodeNotify == CBN_SELCHANGE) {
			s_Info.aNpc[s_nNpcSel].byKind = (BYTE)GetNpcKind (hWnd);
			RenewNpcSel (hWnd);
			RenewLayout (hWnd);
		}
		break;

	case IDC_Q_NPCCLEAR:	/* この欄を空にする */
		ClearNpc (&s_Info.aNpc[s_nNpcSel]);
		RenewNpcSel (hWnd);
		LoadNpc (hWnd, s_nNpcSel);
		break;

	case IDOK:
		if (s_bWait) {
			break;
		}
		if (!SaveAll (hWnd)) {
			break;
		}
		/* 結果が届くまで OK は押せないようにする（届いたら閉じるか、理由を出す） */
		s_bWait = TRUE;
		EnableWindow (GetDlgItem (hWnd, IDOK), FALSE);
		SendCmdADMINQUEST (ADMINQUEST_SET, s_nNo, &s_Info);
		break;

	case IDCANCEL:
		CloseDlgQuestInfoEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：RenewNpcSel														 */
/* 内容：見えるNPCの一覧の文字を作り直す（「1: 宝箱 名前」など）			 */
/* ========================================================================= */
static void RenewNpcSel(HWND hWnd)
{
	HWND hWndCombo = GetDlgItem (hWnd, IDC_Q_NPCSEL);
	char szTmp[160];

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < MAX_QUESTNPC; i ++) {
		const QUESTNPCINFO *pNpc = &s_Info.aNpc[i];
		if (pNpc->byKind == QUESTNPCKIND_NONE) {
			snprintf (szTmp, sizeof (szTmp), "%d: （空き）", i + 1);
		} else {
			snprintf (szTmp, sizeof (szTmp), "%d: %s %s", i + 1, NpcKindName (pNpc->byKind), pNpc->szName);
		}
		SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}
	SendMessage (hWndCombo, CB_SETCURSEL, s_nNpcSel, 0);
}

/* ========================================================================= */
/* 関数名：FillKindCombo													 */
/* 内容：見えるNPCの種類の一覧を作る										 */
/*       いまの依頼の種類で使える種類だけ並べる。使えない種類が入っていた	 */
/*       時は、値を残すため「（使えません）」を付けて足す					 */
/* ========================================================================= */
static void FillKindCombo(HWND hWnd, int nKind)
{
	HWND hWndCombo = GetDlgItem (hWnd, IDC_Q_NPCKIND);
	int nType = GetSel (hWnd, IDC_Q_CHALLENGE);
	char szTmp[128];

	SendMessage (hWndCombo, CB_RESETCONTENT, 0, 0);
	for (int i = 0; (i < QUESTNPCKIND_MAX) || (i <= nKind); i ++) {
		BOOL bAllowed = NpcKindAllowed (nType, i);
		if (!bAllowed && (i != nKind)) {
			continue;
		}
		snprintf (szTmp, sizeof (szTmp), "%d: %s%s", i, NpcKindName (i), bAllowed ? "" : "（この依頼では使えません）");
		int nIndex = (int)SendMessage (hWndCombo, CB_ADDSTRING, 0, (LPARAM)szTmp);
		SendMessage (hWndCombo, CB_SETITEMDATA, nIndex, (LPARAM)i);
		if (i == nKind) {
			SendMessage (hWndCombo, CB_SETCURSEL, nIndex, 0);
		}
	}
}

/* ========================================================================= */
/* 関数名：GetNpcKind														 */
/* 内容：選んでいる見えるNPCの種類											 */
/* ========================================================================= */
static int GetNpcKind(HWND hWnd)
{
	int nSel = (int)SendDlgItemMessage (hWnd, IDC_Q_NPCKIND, CB_GETCURSEL, 0, 0);

	if (nSel < 0) {
		return QUESTNPCKIND_NONE;
	}
	return (int)SendDlgItemMessage (hWnd, IDC_Q_NPCKIND, CB_GETITEMDATA, nSel, 0);
}

/* 子の部品を集める */
static BOOL CALLBACK CollectChild(HWND hWnd, LPARAM lParam)
{
	((std::vector<HWND> *)lParam)->push_back (hWnd);
	return TRUE;
}

/* 部品の位置（ダイアログのクライアント座標） */
static void GetCtrlRect(HWND hWndDlg, HWND hWndCtrl, RECT *pRect)
{
	POINT pt1, pt2;

	GetWindowRect (hWndCtrl, pRect);
	pt1.x = pRect->left;
	pt1.y = pRect->top;
	pt2.x = pRect->right;
	pt2.y = pRect->bottom;
	ScreenToClient (hWndDlg, &pt1);
	ScreenToClient (hWndDlg, &pt2);
	SetRect (pRect, pt1.x, pt1.y, pt2.x, pt2.y);
}

static bool SectionLess(const QE_SECTION &a, const QE_SECTION &b)
{
	if (a.nCol != b.nCol) {
		return a.nCol < b.nCol;
	}
	return a.rc.top < b.rc.top;
}

/* ========================================================================= */
/* 関数名：LayoutInit														 */
/* 内容：枠ごとの部品と元の位置を覚える										 */
/*       枠の中（左上の角が枠の中にある）の部品をその枠のかたまりにする		 */
/* ========================================================================= */
static void LayoutInit(HWND hWnd)
{
	static const int s_anGroup[] = {
		IDC_Q_GRP_BASE, IDC_Q_GRP_SEARCH, IDC_Q_GRP_CHALLENGE, IDC_Q_GRP_GOAL,
		IDC_Q_GRP_NPC, IDC_Q_GRP_NPCCOMMON, IDC_Q_GRP_NPCDEST, IDC_Q_GRP_NPCITEM, IDC_Q_GRP_NPCENEMY,
	};
	std::vector<HWND> ahWnd;
	RECT rcClient, rcWindow, rc;
	size_t i, j;

	s_aSection.clear ();
	s_aButton.clear ();
	s_nScroll = 0;
	s_nScrollMax = 0;

	GetClientRect (hWnd, &rcClient);
	GetWindowRect (hWnd, &rcWindow);
	s_nBaseCX = rcClient.right;
	s_sizeFrame.cx = (rcWindow.right - rcWindow.left) - rcClient.right;
	s_sizeFrame.cy = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	SetRect (&rc, 0, 0, QUESTEDIT_GAP, QUESTEDIT_MAXCY);
	MapDialogRect (hWnd, &rc);
	s_nGap = rc.right;
	s_nMaxCY = rc.bottom;

	for (i = 0; i < sizeof (s_anGroup) / sizeof (s_anGroup[0]); i ++) {
		QE_SECTION Section;
		HWND hWndGroup = GetDlgItem (hWnd, s_anGroup[i]);
		if (hWndGroup == NULL) {
			continue;
		}
		Section.nGroupID = s_anGroup[i];
		GetCtrlRect (hWnd, hWndGroup, &Section.rc);
		Section.nCol = (Section.rc.left < s_nBaseCX / 2) ? 0 : 1;
		s_aSection.push_back (Section);
	}

	EnumChildWindows (hWnd, CollectChild, (LPARAM)&ahWnd);
	for (i = 0; i < ahWnd.size (); i ++) {
		QE_CTRL Ctrl;
		POINT pt;
		if (GetParent (ahWnd[i]) != hWnd) {
			continue;
		}
		Ctrl.hWnd = ahWnd[i];
		GetCtrlRect (hWnd, Ctrl.hWnd, &Ctrl.rc);
		int nID = GetDlgCtrlID (Ctrl.hWnd);
		if ((nID == IDOK) || (nID == IDCANCEL)) {
			s_aButton.push_back (Ctrl);
			continue;
		}
		pt.x = Ctrl.rc.left + 1;
		pt.y = Ctrl.rc.top + 1;
		for (j = 0; j < s_aSection.size (); j ++) {
			if (PtInRect (&s_aSection[j].rc, pt)) {
				s_aSection[j].aCtrl.push_back (Ctrl);
				break;
			}
		}
	}
	std::sort (s_aSection.begin (), s_aSection.end (), SectionLess);

	s_hWndScroll = CreateWindow ("SCROLLBAR", "", WS_CHILD | SBS_VERT,
			s_nBaseCX, 0, QUESTEDIT_SCROLLCX, 32, hWnd, (HMENU)(INT_PTR)IDC_Q_SCROLL, NULL, NULL);
}

/* 部品を動かす（変わった時だけ） */
static void PlaceCtrl(const QE_CTRL &Ctrl, int dy, BOOL bShow)
{
	if (bShow) {
		MoveWindow (Ctrl.hWnd, Ctrl.rc.left, Ctrl.rc.top + dy,
				Ctrl.rc.right - Ctrl.rc.left, Ctrl.rc.bottom - Ctrl.rc.top, TRUE);
	}
	if ((IsWindowVisible (Ctrl.hWnd) ? TRUE : FALSE) != bShow) {
		ShowWindow (Ctrl.hWnd, bShow ? SW_SHOWNA : SW_HIDE);
	}
}

/* ========================================================================= */
/* 関数名：RenewLayout														 */
/* 内容：種類に合わせて使う枠だけ出し、左右の列ごとに上へ詰める				 */
/*       高さが上限を越えたら右にスクロールバーを出す						 */
/* ========================================================================= */
static void RenewLayout(HWND hWnd)
{
	int nType = GetSel (hWnd, IDC_Q_CHALLENGE);
	int nKind = GetNpcKind (hWnd);
	int anCursor[2] = {-1, -1};
	int nBottom = 0, nContent, nCX, nCY, nBtnTop = INT_MAX, nBtnBottom = 0;
	BOOL bScroll;
	size_t i, j;

	if (s_aSection.empty ()) {
		return;
	}
	if (!NpcKindAllowed (nType, nKind)) {
		SetDlgItemText (hWnd, IDC_Q_NPCKINDHELP, "討伐チャレンジの依頼でだけ使えます。種類を変えてください。");
	} else {
		SetDlgItemText (hWnd, IDC_Q_NPCKINDHELP, NpcKindHelp (nKind));
	}

	/* どこへ置くかを決める（まだ動かさない） */
	std::vector<int> anDY (s_aSection.size (), 0);
	std::vector<BOOL> abShow (s_aSection.size (), FALSE);
	for (i = 0; i < s_aSection.size (); i ++) {
		QE_SECTION &Section = s_aSection[i];
		int &nCursor = anCursor[Section.nCol];
		if (nCursor < 0) {
			nCursor = Section.rc.top;
		}
		abShow[i] = SectionVisible (Section.nGroupID, nType, nKind);
		if (!abShow[i]) {
			continue;
		}
		anDY[i] = nCursor - Section.rc.top;
		nCursor = Section.rc.bottom + anDY[i] + s_nGap;
		nBottom = max (nBottom, Section.rc.bottom + anDY[i]);
	}
	for (i = 0; i < s_aButton.size (); i ++) {
		nBtnTop = min (nBtnTop, (int)s_aButton[i].rc.top);
		nBtnBottom = max (nBtnBottom, (int)s_aButton[i].rc.bottom);
	}
	if (s_aButton.empty ()) {
		nBtnTop = nBtnBottom = 0;
	}
	int nBtnDY = nBottom + s_nGap * 3 - nBtnTop;
	nContent = nBtnBottom + nBtnDY + s_nGap * 3;

	bScroll = (nContent > s_nMaxCY) ? TRUE : FALSE;
	nCY = bScroll ? s_nMaxCY : nContent;
	nCX = s_nBaseCX + (bScroll ? QUESTEDIT_SCROLLCX : 0);
	s_nScrollPage = nCY;
	s_nScrollMax = bScroll ? (nContent - nCY) : 0;
	s_nScroll = max (0, min (s_nScroll, s_nScrollMax));

	/* 動かす */
	for (i = 0; i < s_aSection.size (); i ++) {
		for (j = 0; j < s_aSection[i].aCtrl.size (); j ++) {
			PlaceCtrl (s_aSection[i].aCtrl[j], anDY[i] - s_nScroll, abShow[i]);
		}
	}
	for (i = 0; i < s_aButton.size (); i ++) {
		PlaceCtrl (s_aButton[i], nBtnDY - s_nScroll, TRUE);
	}

	if (s_hWndScroll) {
		if (bScroll) {
			SCROLLINFO si;
			ZeroMemory (&si, sizeof (si));
			si.cbSize	= sizeof (si);
			si.fMask	= SIF_RANGE | SIF_PAGE | SIF_POS;
			si.nMin		= 0;
			si.nMax		= nContent - 1;
			si.nPage	= (UINT)nCY;
			si.nPos		= s_nScroll;
			SetScrollInfo (s_hWndScroll, SB_CTL, &si, TRUE);
			SetWindowPos (s_hWndScroll, HWND_TOP, s_nBaseCX, 0, QUESTEDIT_SCROLLCX, nCY, SWP_NOACTIVATE);
			ShowWindow (s_hWndScroll, SW_SHOWNA);
		} else {
			ShowWindow (s_hWndScroll, SW_HIDE);
		}
	}

	/* 窓の大きさを中身に合わせる */
	{
		RECT rcClient;
		GetClientRect (hWnd, &rcClient);
		if ((rcClient.right != nCX) || (rcClient.bottom != nCY)) {
			SetWindowPos (hWnd, NULL, 0, 0, nCX + s_sizeFrame.cx, nCY + s_sizeFrame.cy,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
	InvalidateRect (hWnd, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：OnScrollMsg														 */
/* 内容：スクロールバー・ホイールで中身を上下に動かす						 */
/* ========================================================================= */
static void OnScrollMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int nPos = s_nScroll;

	if (s_nScrollMax <= 0) {
		return;
	}
	if (msg == WM_MOUSEWHEEL) {
		short nDelta = (short)HIWORD (wParam);
		int nMove = nDelta * QUESTEDIT_WHEEL / 120;
		if ((nMove == 0) && (nDelta != 0)) {
			nMove = (nDelta > 0) ? 1 : -1;
		}
		nPos -= nMove;
	} else {
		int nPage = max (12, s_nScrollPage - 24);
		if ((HWND)lParam != s_hWndScroll) {
			return;
		}
		switch (LOWORD (wParam)) {
		case SB_LINEUP:			nPos -= 12;						break;
		case SB_LINEDOWN:		nPos += 12;						break;
		case SB_PAGEUP:			nPos -= nPage;					break;
		case SB_PAGEDOWN:		nPos += nPage;					break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:	nPos = (int)HIWORD (wParam);	break;
		case SB_TOP:			nPos = 0;						break;
		case SB_BOTTOM:			nPos = s_nScrollMax;			break;
		default:				return;
		}
	}
	nPos = max (0, min (nPos, s_nScrollMax));
	if (nPos != s_nScroll) {
		s_nScroll = nPos;
		RenewLayout (hWnd);
	}
}

/* ========================================================================= */
/* 関数名：LoadNpc															 */
/* 内容：見えるNPCの欄を画面に出す											 */
/* ========================================================================= */
static void LoadNpc(HWND hWnd, int nNo)
{
	const QUESTNPCINFO *pNpc = &s_Info.aNpc[nNo];

	char szTmp[64];

	FillKindCombo (hWnd, pNpc->byKind);
	snprintf (szTmp, sizeof (szTmp), "見えるNPC %d の設定", nNo + 1);
	SetDlgItemText (hWnd, IDC_Q_GRP_NPCCOMMON, szTmp);
	SendDlgItemMessage (hWnd, IDC_Q_NPCDIR, CB_SETCURSEL, (pNpc->byDir <= 3) ? pNpc->byDir : 1, 0);
	SetText (hWnd, IDC_Q_NPCNAME, pNpc->szName, FALSE);
	SetNum (hWnd, IDC_Q_NPCGRP, pNpc->wGrpID);
	SetNum (hWnd, IDC_Q_NPCMAP, pNpc->dwMapID);
	SetNum (hWnd, IDC_Q_NPCX, pNpc->byX);
	SetNum (hWnd, IDC_Q_NPCY, pNpc->byY);
	SetText (hWnd, IDC_Q_NPCSPEECH, pNpc->szSpeech, TRUE);
	SetStepSel (hWnd, IDC_Q_NPCSTEPMIN, pNpc->byStepMin, FALSE);
	SetStepSel (hWnd, IDC_Q_NPCSTEPMAX, pNpc->byStepMax, TRUE);
	SetNum (hWnd, IDC_Q_NPCFLGNEED, pNpc->byFlgNeed);
	SetNum (hWnd, IDC_Q_NPCFLGHIDE, pNpc->byFlgHide);
	SetNum (hWnd, IDC_Q_NPCSETFLG, pNpc->bySetFlg);
	SetStepSel (hWnd, IDC_Q_NPCNEXTSTEP, pNpc->byNextStep, TRUE);
	SetNum (hWnd, IDC_Q_NPCDESTMAP, pNpc->dwDestMapID);
	SetNum (hWnd, IDC_Q_NPCDESTX, pNpc->byDestX);
	SetNum (hWnd, IDC_Q_NPCDESTY, pNpc->byDestY);
	FillItemCombo (hWnd, IDC_Q_NPCITEM, pNpc->wItem);
	FillStTmpCombo (hWnd, IDC_Q_NPCSTTMP, pNpc->wStTmp);
	SetDlgItemText (hWnd, IDC_Q_CLICK, "");
	RenewLayout (hWnd);
}

/* ========================================================================= */
/* 関数名：SaveNpc															 */
/* 内容：画面の見えるNPCを覚える（入れ方がおかしければ理由を出して FALSE）	 */
/* ========================================================================= */
static BOOL SaveNpc(HWND hWnd, int nNo)
{
	QUESTNPCINFO Npc = s_Info.aNpc[nNo];
	unsigned long ul;
	char szTmp[64];

	Npc.byKind = (BYTE)GetNpcKind (hWnd);
	if (Npc.byKind == QUESTNPCKIND_NONE) {
		/* 使わない欄は空にしておく */
		ClearNpc (&s_Info.aNpc[nNo]);
		return TRUE;
	}
	snprintf (szTmp, sizeof (szTmp), "見えるNPC %d の", nNo + 1);
	std::string strName = szTmp;

	Npc.byDir = (BYTE)GetSel (hWnd, IDC_Q_NPCDIR);
	if (!GetText (hWnd, IDC_Q_NPCNAME, Npc.szName, sizeof (Npc.szName), FALSE, (strName + "名前").c_str ())) return FALSE;
	if (!GetNumRange (hWnd, IDC_Q_NPCGRP, 0xFFFF, (strName + "画像ID").c_str (), ul)) return FALSE;
	Npc.wGrpID = (WORD)ul;
	Npc.dwMapID = (DWORD)GetNum (hWnd, IDC_Q_NPCMAP);
	if (!GetNumRange (hWnd, IDC_Q_NPCX, 0xFF, (strName + "X座標").c_str (), ul)) return FALSE;
	Npc.byX = (BYTE)ul;
	if (!GetNumRange (hWnd, IDC_Q_NPCY, 0xFF, (strName + "Y座標").c_str (), ul)) return FALSE;
	Npc.byY = (BYTE)ul;
	if (!GetText (hWnd, IDC_Q_NPCSPEECH, Npc.szSpeech, sizeof (Npc.szSpeech), TRUE, (strName + "台詞").c_str ())) return FALSE;
	Npc.byStepMin = GetStepSel (hWnd, IDC_Q_NPCSTEPMIN, FALSE);
	Npc.byStepMax = GetStepSel (hWnd, IDC_Q_NPCSTEPMAX, TRUE);
	if (!GetNumRange (hWnd, IDC_Q_NPCFLGNEED, 16, (strName + "出る条件の旗").c_str (), ul)) return FALSE;
	Npc.byFlgNeed = (BYTE)ul;
	if (!GetNumRange (hWnd, IDC_Q_NPCFLGHIDE, 16, (strName + "消える条件の旗").c_str (), ul)) return FALSE;
	Npc.byFlgHide = (BYTE)ul;
	if (!GetNumRange (hWnd, IDC_Q_NPCSETFLG, 16, (strName + "話したら立てる旗").c_str (), ul)) return FALSE;
	Npc.bySetFlg = (BYTE)ul;
	Npc.byNextStep = GetStepSel (hWnd, IDC_Q_NPCNEXTSTEP, TRUE);
	Npc.dwDestMapID = (DWORD)GetNum (hWnd, IDC_Q_NPCDESTMAP);
	if (!GetNumRange (hWnd, IDC_Q_NPCDESTX, 0xFF, (strName + "行き先X").c_str (), ul)) return FALSE;
	Npc.byDestX = (BYTE)ul;
	if (!GetNumRange (hWnd, IDC_Q_NPCDESTY, 0xFF, (strName + "行き先Y").c_str (), ul)) return FALSE;
	Npc.byDestY = (BYTE)ul;
	Npc.wItem = (WORD)GetSel (hWnd, IDC_Q_NPCITEM);
	Npc.wStTmp = (WORD)GetSel (hWnd, IDC_Q_NPCSTTMP);
	/* 隠れている欄（この種類では使わない）は空にしておく */
	if (!NpcKindHasDest (Npc.byKind)) {
		Npc.dwDestMapID = 0;
		Npc.byDestX = Npc.byDestY = 0;
	}
	if (!NpcKindHasItem (Npc.byKind)) {
		Npc.wItem = 0;
	}
	if (!NpcKindHasStTmp (Npc.byKind)) {
		Npc.wStTmp = 0;
	}

	s_Info.aNpc[nNo] = Npc;
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SaveAll															 */
/* 内容：画面の内容を全部覚える（入れ方がおかしければ理由を出して FALSE）	 */
/*       中身の確かめ（マップの外・ありえない組み合わせなど）はサーバーが	 */
/*       データAPIと同じ決まりで行い、だめなら理由を返してくる				 */
/* ========================================================================= */
static BOOL SaveAll(HWND hWnd)
{
	QUESTINFO Info;
	unsigned long ul;

	if (!SaveNpc (hWnd, s_nNpcSel)) {
		return FALSE;
	}
	Info = s_Info;
	if (!GetText (hWnd, IDC_Q_NAME, Info.szName, sizeof (Info.szName), FALSE, "名前")) return FALSE;
	if (Info.szName[0] == 0) {
		MessageBox (s_hWndDlg, "名前を入れてください。", QUESTEDIT_TITLE, MB_OK);
		SetFocus (GetDlgItem (hWnd, IDC_Q_NAME));
		return FALSE;
	}
	Info.byState = (GetSel (hWnd, IDC_Q_STATE) == 1) ? QUESTSTATE_ON : QUESTSTATE_OFF;
	if (!GetNumRange (hWnd, IDC_Q_GUILD, 99, "ギルド番号", ul)) return FALSE;
	Info.byGuild = (BYTE)ul;
	if (!GetNumRange (hWnd, IDC_Q_SORT, 0xFFFF, "並び順", ul)) return FALSE;
	Info.wSort = (WORD)ul;
	Info.byStepCount = (BYTE)(GetSel (hWnd, IDC_Q_STEPCOUNT) + 1);
	Info.byOnce = (IsDlgButtonChecked (hWnd, IDC_Q_ONCE) == BST_CHECKED) ? 1 : 0;
	if (!GetText (hWnd, IDC_Q_EXPLAIN, Info.szExplain, sizeof (Info.szExplain), TRUE, "説明")) return FALSE;
	if (!GetText (hWnd, IDC_Q_MSGACCEPT, Info.szMsgAccept, sizeof (Info.szMsgAccept), TRUE, "受けた時の台詞")) return FALSE;
	if (!GetText (hWnd, IDC_Q_MSGDONE, Info.szMsgDone, sizeof (Info.szMsgDone), TRUE, "達成した時の台詞")) return FALSE;
	if (!GetNumRange (hWnd, IDC_Q_GOLD, 999999, "報酬のお金", ul)) return FALSE;
	Info.dwGold = (DWORD)ul;
	if (!GetNumRange (hWnd, IDC_Q_EXP, 0xFFFFFFFFUL, "報酬の経験値", ul)) return FALSE;
	Info.dwExp = (DWORD)ul;
	Info.wRewardItem = (WORD)GetSel (hWnd, IDC_Q_REWARDITEM);
	Info.wQuestItem = (WORD)GetSel (hWnd, IDC_Q_QUESTITEM);
	if (!GetText (hWnd, IDC_Q_GOAL0, Info.aszGoal[0], MAX_QUESTGOAL, FALSE, "0段目の目的")) return FALSE;
	if (!GetText (hWnd, IDC_Q_GOAL1, Info.aszGoal[1], MAX_QUESTGOAL, FALSE, "1段目の目的")) return FALSE;
	if (!GetText (hWnd, IDC_Q_GOAL2, Info.aszGoal[2], MAX_QUESTGOAL, FALSE, "2段目の目的")) return FALSE;
	if (!GetText (hWnd, IDC_Q_GOAL3, Info.aszGoal[3], MAX_QUESTGOAL, FALSE, "3段目の目的")) return FALSE;
	Info.byChallenge = (BYTE)GetSel (hWnd, IDC_Q_CHALLENGE);
	if (!GetNumRange (hWnd, IDC_Q_MEMBERMIN, MAX_PARTYMEMBER, "必要な人数", ul)) return FALSE;
	Info.byMemberMin = (BYTE)ul;
	Info.wLoanStTmp = (WORD)GetSel (hWnd, IDC_Q_LOANSTTMP);
	/* 隠れている枠（この依頼の種類では使わない）は空にしておく */
	if (!SectionVisible (IDC_Q_GRP_SEARCH, Info.byChallenge, QUESTNPCKIND_NONE)) {
		Info.wQuestItem = 0;
	}
	if (!SectionVisible (IDC_Q_GRP_CHALLENGE, Info.byChallenge, QUESTNPCKIND_NONE)) {
		Info.byMemberMin = 0;
		Info.wLoanStTmp = 0;
	}

	s_Info = Info;
	return TRUE;
}

/* Copyright(C)URARA-works 2026 */
