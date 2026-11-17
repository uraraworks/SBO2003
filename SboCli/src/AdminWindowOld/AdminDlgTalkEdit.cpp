/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		会話セット編集ダイアログ処理実装ファイル					 */
/* ファイル名：	AdminDlgTalkEdit.cpp										 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * 会話セット 1 件の編集画面。リソースは IDD_TALKINFO_EDIT。
 * 左が行の一覧（最大 16 行。追加・削除・上へ・下へ）、右が選んだ行 1 つの編集。
 * 行の条件（立っている必要のある印・立っていてはいけない印・数値の条件・レベル）、
 * 台詞、話した後の動き（立てる印・消す印・数値の動き・渡すアイテム）を直す。
 * 条件には「いま受けている依頼（依頼番号と段の範囲）」もある。
 * 印・数値は番号の欄に入れ、横に名前表（フラグの名前）の名前を出す。空欄は「無し」で、
 * 内部では FLAG_NOBIT / FLAG_NONUM。
 * 行を切り替える時・OK を押す時に、いま出している行を画面から読み取って覚える。
 * OK でセット丸ごとをサーバーへ送り、サーバーはデータAPIと同じ確認を通して書き込む。
 * うまくいけば閉じ、だめならサーバーからの理由を出して開いたままにする。
 */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "AdminFlagTalk.h"
#include "AdminFlagUi.h"
#include "AdminDlgTalk.h"
#include "AdminDlgTalkEdit.h"
#include <string>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
#define TALKEDIT_TITLE		"会話データ"

static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* 会話セットの配列の番号（-1 なら追加） */
static int s_nPresetID = 0;										/* 追加の時に番号の欄へ入れておく会話セット番号（0=空欄） */
static int s_nLineSel = -1;										/* 右に出している行（-1=行が無い） */
static BOOL s_bWait = FALSE;									/* 書き込みの結果を待っている */
static TALKINFO s_Info;											/* 編集中の内容 */
static DWORD s_dwNameSeqSeen = 0;								/* 名前を出し直した時の名前表の更新番号 */
static DWORD s_dwResultSeqSeen = 0;								/* 見た書き込みの結果の番号 */

/* 番号の欄と、その横に名前を出す文字（ID は欄ごとに続き番号） */
static const struct {
	int		nEdit;				/* 番号の欄の先頭のID */
	int		nLabel;				/* 名前の文字の先頭のID */
	int		nCount;				/* 欄の数 */
	BYTE	byKind;				/* 印か数値か(FLAGKIND_*) */
} s_aFlagCtrl[] = {
	{IDC_TE_NEED0,		IDC_TE_NEEDN0,		MAX_TALKNEED,	FLAGKIND_BIT},
	{IDC_TE_NOT0,		IDC_TE_NOTN0,		MAX_TALKNEED,	FLAGKIND_BIT},
	{IDC_TE_COND0,		IDC_TE_CONDN0,		MAX_TALKCOND,	FLAGKIND_NUM},
	{IDC_TE_SETON0,		IDC_TE_SETONN0,		MAX_TALKSET,	FLAGKIND_BIT},
	{IDC_TE_SETOFF0,	IDC_TE_SETOFFN0,	MAX_TALKSET,	FLAGKIND_BIT},
	{IDC_TE_ACT0,		IDC_TE_ACTN0,		MAX_TALKACT,	FLAGKIND_NUM},
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnNotify(HWND hWnd, LPARAM lParam);									/* メッセージハンドラ(WM_NOTIFY) */
static void OnChanged(void);													/* 名前表・書き込みの結果が届いた */
static void RenewLines(void);													/* 行の一覧を作り直す */
static void LoadLine(int nLine);												/* 行を画面に出す */
static BOOL SaveLine(int nLine);												/* 画面の行を覚える */
static void EnableLine(BOOL bEnable);											/* 行の欄を使えるか */
static void UpdateAllNames(void);												/* 番号の横の名前を全部作り直す */
static void InitLine(TALKLINEINFO *pLine);										/* 行を空にする */
static void UpdateItemName(void);												/* 渡すアイテムの横の名前を出し直す */

/* ========================================================================= */
/* 小物																		 */
/* ========================================================================= */

/* 行を空にする（印・数値は「無し」） */
static void InitLine(TALKLINEINFO *pLine)
{
	ZeroMemory (pLine, sizeof (TALKLINEINFO));
	for (int i = 0; i < MAX_TALKNEED; i ++) {
		pLine->awNeed[i] = FLAG_NOBIT;
		pLine->awNot[i] = FLAG_NOBIT;
	}
	for (int i = 0; i < MAX_TALKCOND; i ++) {
		pLine->abyCondNo[i] = FLAG_NONUM;
	}
	for (int i = 0; i < MAX_TALKSET; i ++) {
		pLine->awSetOn[i] = FLAG_NOBIT;
		pLine->awSetOff[i] = FLAG_NOBIT;
	}
	for (int i = 0; i < MAX_TALKACT; i ++) {
		pLine->abyActNo[i] = FLAG_NONUM;
	}
	pLine->byQuestStepMax = QUESTNPC_NOSTEP;							/* 依頼の段の上限は「どの段でも」 */
}

/* 渡すアイテムの横の名前を出し直す（空欄・0 なら何も出さない） */
static void UpdateItemNameOne(int nEdit, int nLabel)
{
	unsigned long ulNo;
	BOOL bBlank;

	if (!AdminFlagReadNo (s_hWndDlg, nEdit, ulNo, bBlank) || bBlank || (ulNo == 0)) {
		SetDlgItemText (s_hWndDlg, nLabel, "");
		return;
	}
	if ((ulNo >= (unsigned long)g_aItemInfo.GetSize ()) || (g_aItemInfo[ulNo] == NULL)) {
		SetDlgItemText (s_hWndDlg, nLabel, "（範囲外）");
		return;
	}
	SetDlgItemText (s_hWndDlg, nLabel, g_aItemInfo[ulNo]->szName);
}

/* 渡すアイテム・持っている物の横の名前を出し直す */
static void UpdateItemName(void)
{
	UpdateItemNameOne (IDC_TE_GIVEITEM, IDC_TE_GIVEITEMN);
	UpdateItemNameOne (IDC_TE_NEEDITEM, IDC_TE_NEEDITEMN);
	UpdateItemNameOne (IDC_TE_TAKEITEM, IDC_TE_TAKEITEMN);
}

/* 一覧に出す台詞の先頭（最初の行の先頭 16 文字ほど。UTF-8 の途中で切らない） */
static void SpeechHead(const char *pszSpeech, char *pszOut, int nOutSize)
{
	int nLen = 0, nChar = 0;
	const unsigned char *p = (const unsigned char *)pszSpeech;

	while (*p && (*p != '\n') && (*p != '\r') && (nChar < 16)) {
		int nByte = 1;
		if (*p >= 0xF0)			nByte = 4;
		else if (*p >= 0xE0)	nByte = 3;
		else if (*p >= 0xC0)	nByte = 2;
		if (nLen + nByte >= nOutSize) {
			break;
		}
		for (int i = 0; i < nByte; i ++) {
			if (p[i] == 0) {
				nByte = i;
				break;
			}
			pszOut[nLen ++] = (char)p[i];
		}
		p += nByte;
		nChar ++;
		if (nByte == 0) {
			break;
		}
	}
	pszOut[nLen] = 0;
}

/* ========================================================================= */
/* 関数名：ShowDlgTalkEdit													 */
/* 内容：ダイアログ表示（nNo=-1 なら追加）									 */
/* ========================================================================= */
void ShowDlgTalkEdit(HWND hWndParent, int nNo)
{
	const TALKINFO *pInfo;

	if (s_hWndDlg) {
		MessageBox (s_hWndDlg, "編集中の会話セットがあります。閉じてから開いてください。", TALKEDIT_TITLE, MB_OK);
		return;
	}

	pInfo = AdminTalkGet (nNo);
	if (pInfo) {
		s_nNo = nNo;
		s_Info = *pInfo;
	} else {
		/* 追加は行を 1 つだけ持った状態から始める */
		s_nNo = -1;
		ZeroMemory (&s_Info, sizeof (s_Info));
		s_Info.byState = TALKSTATE_ON;
		for (int i = 0; i < MAX_TALKLINE; i ++) {
			InitLine (&s_Info.aLine[i]);
		}
		s_Info.byLineCount = 1;
		s_Info.aLine[0].byRank = 1;
	}
	if (s_Info.byLineCount > MAX_TALKLINE) {
		s_Info.byLineCount = MAX_TALKLINE;
	}
	s_nLineSel = (s_Info.byLineCount > 0) ? 0 : -1;
	s_bWait = FALSE;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_TALKINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	AdminFlagTalkAddListener (OnChanged);
	s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
	AdminFlagNameLoadIfEmpty ();

	/* 管理者ウィンドウに重ねて出す */
	{
		RECT rc;
		GetWindowRect (g_hWndAdmin ? g_hWndAdmin : hWndParent, &rc);
		SetWindowPos (s_hWndDlg, NULL, rc.left + 24, rc.top + 24, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：ShowDlgTalkEditByNo												 */
/* 内容：会話セット番号（NPC の台詞 "@t番号" の数字）で開く					 */
/*       有効なセットがあればそれを直す。無い（削除済み含む）なら、その番号で追加の画面にする */
/* ========================================================================= */
void ShowDlgTalkEditByNo(HWND hWndParent, int nTalkNo)
{
	int nIndex = -1;

	if ((nTalkNo < 1) || (nTalkNo >= MAX_TALK)) {
		char szMsg[96];
		snprintf (szMsg, sizeof (szMsg), "会話セットの番号は 1〜%d です（@t の後ろの数字）。", MAX_TALK - 1);
		MessageBox (hWndParent, szMsg, TALKEDIT_TITLE, MB_OK);
		return;
	}
	for (int i = 0; i < AdminTalkGetCount (); i ++) {
		const TALKINFO *pInfo = AdminTalkGet (i);
		if (pInfo && (pInfo->wID == (WORD)nTalkNo) && (pInfo->byState == TALKSTATE_ON)) {
			nIndex = i;
			break;
		}
	}
	s_nPresetID = (nIndex < 0) ? nTalkNo : 0;
	ShowDlgTalkEdit (hWndParent, nIndex);
	s_nPresetID = 0;						/* 開く処理の中で使い終わっている */
}

/* ========================================================================= */
/* 関数名：CloseDlgTalkEdit													 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgTalkEdit(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	AdminFlagTalkRemoveListener (OnChanged);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_bWait = FALSE;
}

/* ========================================================================= */
/* 関数名：IsDlgTalkEditOpen												 */
/* 内容：開いているか														 */
/* ========================================================================= */
BOOL IsDlgTalkEditOpen(void)
{
	return (s_hWndDlg != NULL) ? TRUE : FALSE;
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
		UpdateAllNames ();
	}
	if (s_dwResultSeqSeen != pRes->dwSeq) {
		s_dwResultSeqSeen = pRes->dwSeq;
		if (s_bWait && (pRes->byCmd == CMD_ADMINTALK)) {
			s_bWait = FALSE;
			EnableWindow (GetDlgItem (s_hWndDlg, IDOK), TRUE);
			if (!pRes->byResult) {
				snprintf (szMsg, sizeof (szMsg), "書き込めませんでした。\n%s", pRes->szMsg);
				MessageBox (s_hWndDlg, szMsg, TALKEDIT_TITLE, MB_OK);
				return;
			}
			CloseDlgTalkEdit ();
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
		CloseDlgTalkEdit ();
		return 1;

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
	char szTmp[160];
	HWND hWndTmp;
	LV_COLUMN lvcol;
	int i;

	s_hWndDlg = hWnd;		/* この後の小物が使う（CreateDialog から戻る前に呼ばれる） */

	if (s_nNo >= 0) {
		snprintf (szTmp, sizeof (szTmp), "会話データの編集（%u %s）", (unsigned)s_Info.wID, s_Info.szName);
		SetWindowText (hWnd, szTmp);
		AdminFlagSetNo (hWnd, IDC_TE_ID, s_Info.wID, FALSE);
		EnableWindow (GetDlgItem (hWnd, IDC_TE_ID), FALSE);		/* 番号は足す時に決め、あとから変えられない */
	} else {
		SetWindowText (hWnd, "会話データの追加");
		AdminFlagSetNo (hWnd, IDC_TE_ID, (unsigned long)s_nPresetID, (s_nPresetID <= 0) ? TRUE : FALSE);
	}
	SetDlgItemText (hWnd, IDC_TE_NAME, s_Info.szName);
	SetDlgItemText (hWnd, IDC_TE_HELP,
			"行の選び方：話しかけた人の印・数値・レベルが、行の条件に全部合う行が候補になります。"
			"候補の中で順位が一番小さい行を話します（同じ順位が複数あればくじ）。"
			"どの行も合わなければ何も言いません。"
			"印・数値は、話した人のキャラのフラグです。番号の横の名前は「その他→フラグの名前」で付けます。"
			"渡すアイテムは、その行で話すたびに 1 つ渡します（回数は数えません）。"
			"一度きりにしたい時は、「話した後に立てる印」と「立っていてはいけない印」に同じ印を入れてください。"
			"「はい・いいえで聞く」を入れた行は、台詞のあとに「はい・いいえ」を選ばせます。「はい」の時だけ印・数値・渡す物・取り上げる物を動かし、「いいえ」は何も動かさず、いいえの返事（空なら既定の一言）を言います。");

	/* 行の一覧 */
	hWndTmp = GetDlgItem (hWnd, IDC_TE_LINES);
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = 36;
	lvcol.pszText = (LPSTR)"順位";
	ListView_InsertColumn (hWndTmp, 0, &lvcol);
	lvcol.cx = 100;
	lvcol.pszText = (LPSTR)"台詞";
	ListView_InsertColumn (hWndTmp, 1, &lvcol);
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	/* 数値の条件の比べ方・数値の動き */
	for (i = 0; i < MAX_TALKCOND; i ++) {
		SendDlgItemMessage (hWnd, IDC_TE_CONDCMP0 + i, CB_ADDSTRING, 0, (LPARAM)"以上");
		SendDlgItemMessage (hWnd, IDC_TE_CONDCMP0 + i, CB_ADDSTRING, 0, (LPARAM)"ちょうど");
		SendDlgItemMessage (hWnd, IDC_TE_CONDCMP0 + i, CB_ADDSTRING, 0, (LPARAM)"以下");
	}
	for (i = 0; i < MAX_TALKACT; i ++) {
		SendDlgItemMessage (hWnd, IDC_TE_ACTOP0 + i, CB_ADDSTRING, 0, (LPARAM)"にする");
		SendDlgItemMessage (hWnd, IDC_TE_ACTOP0 + i, CB_ADDSTRING, 0, (LPARAM)"足す");
	}

	RenewLines ();
	if (s_nLineSel >= 0) {
		LoadLine (s_nLineSel);
	} else {
		EnableLine (FALSE);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：EnableLine														 */
/* 内容：行の欄を使えるようにする・使えなくする（行が無い時）				 */
/* ========================================================================= */
static void EnableLine(BOOL bEnable)
{
	static const int s_anSingle[] = {IDC_TE_RANK, IDC_TE_LVMIN, IDC_TE_LVMAX, IDC_TE_SPEECH,
			IDC_TE_QID, IDC_TE_QSTEPMIN, IDC_TE_QSTEPMAX, IDC_TE_GIVEITEM, IDC_TE_NEEDITEM, IDC_TE_TAKEITEM,
			IDC_TE_ASK, IDC_TE_NOMSG};

	for (size_t i = 0; i < sizeof (s_anSingle) / sizeof (s_anSingle[0]); i ++) {
		EnableWindow (GetDlgItem (s_hWndDlg, s_anSingle[i]), bEnable);
	}
	for (size_t i = 0; i < sizeof (s_aFlagCtrl) / sizeof (s_aFlagCtrl[0]); i ++) {
		for (int j = 0; j < s_aFlagCtrl[i].nCount; j ++) {
			EnableWindow (GetDlgItem (s_hWndDlg, s_aFlagCtrl[i].nEdit + j), bEnable);
		}
	}
	for (int i = 0; i < MAX_TALKCOND; i ++) {
		EnableWindow (GetDlgItem (s_hWndDlg, IDC_TE_CONDCMP0 + i), bEnable);
		EnableWindow (GetDlgItem (s_hWndDlg, IDC_TE_CONDVAL0 + i), bEnable);
	}
	for (int i = 0; i < MAX_TALKACT; i ++) {
		EnableWindow (GetDlgItem (s_hWndDlg, IDC_TE_ACTOP0 + i), bEnable);
		EnableWindow (GetDlgItem (s_hWndDlg, IDC_TE_ACTVAL0 + i), bEnable);
	}
	if (!bEnable) {
		/* 行が無い時は欄を空に見せる */
		SetDlgItemText (s_hWndDlg, IDC_TE_GRP_LINE, "選んだ行（行がありません。左の「追加」で足します）");
		AdminFlagSetNo (s_hWndDlg, IDC_TE_RANK, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_LVMIN, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_LVMAX, 0, TRUE);
		SetDlgItemText (s_hWndDlg, IDC_TE_SPEECH, "");
		AdminFlagSetNo (s_hWndDlg, IDC_TE_QID, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_QSTEPMIN, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_QSTEPMAX, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_GIVEITEM, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_NEEDITEM, 0, TRUE);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_TAKEITEM, 0, TRUE);
		CheckDlgButton (s_hWndDlg, IDC_TE_ASK, BST_UNCHECKED);
		SetDlgItemText (s_hWndDlg, IDC_TE_NOMSG, "");
		for (size_t i = 0; i < sizeof (s_aFlagCtrl) / sizeof (s_aFlagCtrl[0]); i ++) {
			for (int j = 0; j < s_aFlagCtrl[i].nCount; j ++) {
				AdminFlagSetNo (s_hWndDlg, s_aFlagCtrl[i].nEdit + j, 0, TRUE);
			}
		}
		for (int i = 0; i < MAX_TALKCOND; i ++) {
			AdminFlagSetNo (s_hWndDlg, IDC_TE_CONDVAL0 + i, 0, TRUE);
		}
		for (int i = 0; i < MAX_TALKACT; i ++) {
			AdminFlagSetNo (s_hWndDlg, IDC_TE_ACTVAL0 + i, 0, TRUE);
		}
	}
}

/* ========================================================================= */
/* 関数名：UpdateAllNames													 */
/* 内容：番号の横の名前を全部作り直す（名前表が届いた時・行を出した時）		 */
/* ========================================================================= */
static void UpdateAllNames(void)
{
	s_dwNameSeqSeen = AdminFlagNameGetSeq ();
	for (size_t i = 0; i < sizeof (s_aFlagCtrl) / sizeof (s_aFlagCtrl[0]); i ++) {
		for (int j = 0; j < s_aFlagCtrl[i].nCount; j ++) {
			AdminFlagUpdateName (s_hWndDlg, s_aFlagCtrl[i].nEdit + j, s_aFlagCtrl[i].nLabel + j, s_aFlagCtrl[i].byKind);
		}
	}
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	/* 番号の欄が変わったら、横の名前を出し直す */
	if (nCodeNotify == EN_CHANGE) {
		if ((nID == IDC_TE_GIVEITEM) || (nID == IDC_TE_NEEDITEM) || (nID == IDC_TE_TAKEITEM)) {
			UpdateItemName ();
			return;
		}
		for (size_t i = 0; i < sizeof (s_aFlagCtrl) / sizeof (s_aFlagCtrl[0]); i ++) {
			if ((nID >= s_aFlagCtrl[i].nEdit) && (nID < s_aFlagCtrl[i].nEdit + s_aFlagCtrl[i].nCount)) {
				AdminFlagUpdateName (hWnd, nID, s_aFlagCtrl[i].nLabel + (nID - s_aFlagCtrl[i].nEdit), s_aFlagCtrl[i].byKind);
				return;
			}
		}
		return;
	}

	switch (nID) {
	case IDC_TE_LINEADD:	/* 行を足す（いま出している行の下の端に） */
		{
			int nNew;

			if (s_Info.byLineCount >= MAX_TALKLINE) {
				char szMsg[64];
				snprintf (szMsg, sizeof (szMsg), "行は %d 行までです。", MAX_TALKLINE);
				MessageBox (hWnd, szMsg, TALKEDIT_TITLE, MB_OK);
				break;
			}
			if ((s_nLineSel >= 0) && !SaveLine (s_nLineSel)) {
				break;
			}
			nNew = s_Info.byLineCount;
			InitLine (&s_Info.aLine[nNew]);
			/* 順位は、一つ前の行の次（同じ順位だとくじになるので） */
			if (nNew > 0) {
				BYTE byPrev = s_Info.aLine[nNew - 1].byRank;
				s_Info.aLine[nNew].byRank = (byPrev < 255) ? (BYTE)(byPrev + 1) : byPrev;
			} else {
				s_Info.aLine[nNew].byRank = 1;
			}
			s_Info.byLineCount ++;
			s_nLineSel = nNew;
			RenewLines ();
			LoadLine (s_nLineSel);
		}
		break;

	case IDC_TE_LINEDEL:	/* いま出している行を消す */
		if (s_nLineSel < 0) {
			break;
		}
		for (int i = s_nLineSel; i < (int)s_Info.byLineCount - 1; i ++) {
			s_Info.aLine[i] = s_Info.aLine[i + 1];
		}
		s_Info.byLineCount --;
		InitLine (&s_Info.aLine[s_Info.byLineCount]);
		if (s_nLineSel >= (int)s_Info.byLineCount) {
			s_nLineSel = (int)s_Info.byLineCount - 1;
		}
		RenewLines ();
		if (s_nLineSel >= 0) {
			LoadLine (s_nLineSel);
		} else {
			EnableLine (FALSE);
		}
		break;

	case IDC_TE_LINEUP:		/* 上へ */
	case IDC_TE_LINEDOWN:	/* 下へ */
		{
			int nTo = (nID == IDC_TE_LINEUP) ? s_nLineSel - 1 : s_nLineSel + 1;
			TALKLINEINFO Tmp;

			if ((s_nLineSel < 0) || (nTo < 0) || (nTo >= (int)s_Info.byLineCount)) {
				break;
			}
			if (!SaveLine (s_nLineSel)) {
				break;
			}
			Tmp = s_Info.aLine[nTo];
			s_Info.aLine[nTo] = s_Info.aLine[s_nLineSel];
			s_Info.aLine[s_nLineSel] = Tmp;
			s_nLineSel = nTo;
			RenewLines ();
			LoadLine (s_nLineSel);
		}
		break;

	case IDOK:
		{
			TALKINFO Info;
			unsigned long ul;
			BOOL bBlank;

			if (s_bWait) {
				break;
			}
			if ((s_nLineSel >= 0) && !SaveLine (s_nLineSel)) {
				break;
			}
			Info = s_Info;
			if (!AdminGetText (hWnd, hWnd, IDC_TE_NAME, Info.szName, sizeof (Info.szName), FALSE, "名前", TALKEDIT_TITLE)) {
				break;
			}
			if (s_nNo < 0) {
				/* 番号は足す時に決める（1〜999） */
				if (!AdminFlagReadNo (hWnd, IDC_TE_ID, ul, bBlank) || bBlank || (ul < 1) || (ul >= MAX_TALK)) {
					char szMsg[96];
					snprintf (szMsg, sizeof (szMsg), "会話セットの番号は 1〜%d で入れてください。", MAX_TALK - 1);
					MessageBox (hWnd, szMsg, TALKEDIT_TITLE, MB_OK);
					SetFocus (GetDlgItem (hWnd, IDC_TE_ID));
					break;
				}
				/* 同じ番号がもうあれば、サーバーに断られる前に伝える */
				for (int i = 0; i < AdminTalkGetCount (); i ++) {
					const TALKINFO *pOld = AdminTalkGet (i);
					if (pOld && (pOld->wID == (WORD)ul)) {
						char szMsg[MAX_TALKNAME + 128];
						if (pOld->byState == TALKSTATE_ON) {
							snprintf (szMsg, sizeof (szMsg), "番号 %lu の会話セット「%s」がもうあります。別の番号にするか、一覧から選んで直してください。", ul, pOld->szName);
							MessageBox (hWnd, szMsg, TALKEDIT_TITLE, MB_OK);
							SetFocus (GetDlgItem (hWnd, IDC_TE_ID));
							ul = 0;
						}
						break;
					}
				}
				if (ul == 0) {
					break;
				}
				Info.wID = (WORD)ul;
			}
			Info.byState = TALKSTATE_ON;
			s_Info = Info;
			/* 追加で、削除済みの同じ番号の枠があれば、その枠を直す（サーバーは重なりを断るため） */
			{
				int nSendNo = s_nNo;
				if (s_nNo < 0) {
					for (int i = 0; i < AdminTalkGetCount (); i ++) {
						const TALKINFO *pOld = AdminTalkGet (i);
						if (pOld && (pOld->wID == Info.wID)) {
							nSendNo = i;
							break;
						}
					}
				}
				/* 結果が届くまで OK は押せないようにする（届いたら閉じるか、理由を出す） */
				s_bWait = TRUE;
				s_dwResultSeqSeen = AdminFlagTalkGetResult ()->dwSeq;
				EnableWindow (GetDlgItem (hWnd, IDOK), FALSE);
				AdminTalkSend (nSendNo, &Info);
			}
		}
		break;

	case IDCANCEL:
		CloseDlgTalkEdit ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnNotify															 */
/* 内容：メッセージハンドラ(WM_NOTIFY)。行の一覧で選んだら右に出す			 */
/* ========================================================================= */
static void OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR pHdr = (LPNMHDR)lParam;
	NMLISTVIEW *pNm = (NMLISTVIEW *)lParam;
	int nSel;

	if ((pHdr == NULL) || (pHdr->idFrom != IDC_TE_LINES) || (pHdr->code != LVN_ITEMCHANGED)) {
		return;
	}
	if (((pNm->uNewState & LVIS_SELECTED) == 0) || (pNm->uOldState & LVIS_SELECTED)) {
		return;
	}
	nSel = pNm->iItem;
	if ((nSel < 0) || (nSel >= (int)s_Info.byLineCount) || (nSel == s_nLineSel)) {
		return;
	}
	if ((s_nLineSel >= 0) && !SaveLine (s_nLineSel)) {
		/* 直してもらうため、元の行に戻す */
		ListView_SetItemState (GetDlgItem (hWnd, IDC_TE_LINES), s_nLineSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		return;
	}
	s_nLineSel = nSel;
	RenewLines ();
	LoadLine (s_nLineSel);
}

/* ========================================================================= */
/* 関数名：RenewLines														 */
/* 内容：行の一覧を作り直す（「順位」と台詞の先頭）。選んでいる行は残す		 */
/* ========================================================================= */
static void RenewLines(void)
{
	HWND hWndTmp = GetDlgItem (s_hWndDlg, IDC_TE_LINES);
	char szTmp[160];
	LV_ITEM item;

	ListView_DeleteAllItems (hWndTmp);
	for (int i = 0; i < (int)s_Info.byLineCount; i ++) {
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)s_Info.aLine[i].byRank);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);
		SpeechHead (s_Info.aLine[i].szSpeech, szTmp, sizeof (szTmp));
		ListView_SetItemText (hWndTmp, i, 1, szTmp);
	}
	if ((s_nLineSel >= 0) && (s_nLineSel < (int)s_Info.byLineCount)) {
		ListView_SetItemState (hWndTmp, s_nLineSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
}

/* ========================================================================= */
/* 関数名：LoadLine															 */
/* 内容：行を画面に出す														 */
/* ========================================================================= */
static void LoadLine(int nLine)
{
	const TALKLINEINFO *pLine = &s_Info.aLine[nLine];
	char szTmp[96];
	int i;

	EnableLine (TRUE);
	snprintf (szTmp, sizeof (szTmp), "選んだ行（行 %d）", nLine + 1);
	SetDlgItemText (s_hWndDlg, IDC_TE_GRP_LINE, szTmp);

	AdminFlagSetNo (s_hWndDlg, IDC_TE_RANK, pLine->byRank, FALSE);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_LVMIN, pLine->byLvMin, FALSE);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_LVMAX, pLine->byLvMax, FALSE);
	for (i = 0; i < MAX_TALKNEED; i ++) {
		AdminFlagSetNo (s_hWndDlg, IDC_TE_NEED0 + i, pLine->awNeed[i], pLine->awNeed[i] == FLAG_NOBIT);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_NOT0 + i, pLine->awNot[i], pLine->awNot[i] == FLAG_NOBIT);
	}
	for (i = 0; i < MAX_TALKCOND; i ++) {
		BOOL bNone = (pLine->abyCondNo[i] == FLAG_NONUM) ? TRUE : FALSE;
		AdminFlagSetNo (s_hWndDlg, IDC_TE_COND0 + i, pLine->abyCondNo[i], bNone);
		SendDlgItemMessage (s_hWndDlg, IDC_TE_CONDCMP0 + i, CB_SETCURSEL, (pLine->abyCondCmp[i] < TALKCMP_MAX) ? pLine->abyCondCmp[i] : 0, 0);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_CONDVAL0 + i, pLine->abyCondVal[i], bNone);
	}
	AdminSetMultiText (s_hWndDlg, IDC_TE_SPEECH, pLine->szSpeech);
	for (i = 0; i < MAX_TALKSET; i ++) {
		AdminFlagSetNo (s_hWndDlg, IDC_TE_SETON0 + i, pLine->awSetOn[i], pLine->awSetOn[i] == FLAG_NOBIT);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_SETOFF0 + i, pLine->awSetOff[i], pLine->awSetOff[i] == FLAG_NOBIT);
	}
	for (i = 0; i < MAX_TALKACT; i ++) {
		BOOL bNone = (pLine->abyActNo[i] == FLAG_NONUM) ? TRUE : FALSE;
		AdminFlagSetNo (s_hWndDlg, IDC_TE_ACT0 + i, pLine->abyActNo[i], bNone);
		SendDlgItemMessage (s_hWndDlg, IDC_TE_ACTOP0 + i, CB_SETCURSEL, (pLine->abyActOp[i] < TALKNUMOP_MAX) ? pLine->abyActOp[i] : 0, 0);
		AdminFlagSetNo (s_hWndDlg, IDC_TE_ACTVAL0 + i, pLine->abyActVal[i], bNone);
	}
	/* 受けている依頼の条件（依頼番号が 0・上限が 255 は空欄） */
	AdminFlagSetNo (s_hWndDlg, IDC_TE_QID, pLine->wQuestID, pLine->wQuestID == 0);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_QSTEPMIN, pLine->byQuestStepMin, pLine->wQuestID == 0);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_QSTEPMAX, pLine->byQuestStepMax, (pLine->wQuestID == 0) || (pLine->byQuestStepMax == QUESTNPC_NOSTEP));
	AdminFlagSetNo (s_hWndDlg, IDC_TE_GIVEITEM, pLine->wGiveItem, pLine->wGiveItem == 0);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_NEEDITEM, pLine->wNeedItem, pLine->wNeedItem == 0);
	AdminFlagSetNo (s_hWndDlg, IDC_TE_TAKEITEM, pLine->wTakeItem, pLine->wTakeItem == 0);
	CheckDlgButton (s_hWndDlg, IDC_TE_ASK, pLine->byAsk ? BST_CHECKED : BST_UNCHECKED);
	AdminSetMultiText (s_hWndDlg, IDC_TE_NOMSG, pLine->szNoMsg);
	UpdateItemName ();
	UpdateAllNames ();
}

/* ========================================================================= */
/* 関数名：SaveLine															 */
/* 内容：画面の行を覚える（入れ方がおかしければ理由を出して FALSE）			 */
/*       中身の確かめ（レベルの大小など）はサーバーがデータAPIと同じ決まりで	 */
/*       行い、だめなら理由を返してくる										 */
/* ========================================================================= */
static BOOL SaveLine(int nLine)
{
	TALKLINEINFO L = s_Info.aLine[nLine];
	unsigned long ul;
	char szTmp[48];
	int i;

	snprintf (szTmp, sizeof (szTmp), "行 %d の", nLine + 1);
	std::string strName = szTmp;

	if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_RANK, 255, strName + "順位", TALKEDIT_TITLE, ul)) return FALSE;
	L.byRank = (BYTE)ul;
	if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_LVMIN, 255, strName + "レベル下限", TALKEDIT_TITLE, ul)) return FALSE;
	L.byLvMin = (BYTE)ul;
	if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_LVMAX, 255, strName + "レベル上限", TALKEDIT_TITLE, ul)) return FALSE;
	L.byLvMax = (BYTE)ul;
	for (i = 0; i < MAX_TALKNEED; i ++) {
		snprintf (szTmp, sizeof (szTmp), " %d 個目の", i + 1);
		if (!AdminFlagGetBit (s_hWndDlg, s_hWndDlg, IDC_TE_NEED0 + i, strName + "立っている必要のある印の" + szTmp + "番号", TALKEDIT_TITLE, L.awNeed[i])) return FALSE;
		if (!AdminFlagGetBit (s_hWndDlg, s_hWndDlg, IDC_TE_NOT0 + i, strName + "立っていてはいけない印の" + szTmp + "番号", TALKEDIT_TITLE, L.awNot[i])) return FALSE;
	}
	for (i = 0; i < MAX_TALKCOND; i ++) {
		snprintf (szTmp, sizeof (szTmp), "数値の条件%dの", i + 1);
		if (!AdminFlagGetNum (s_hWndDlg, s_hWndDlg, IDC_TE_COND0 + i, strName + szTmp + "数値番号", TALKEDIT_TITLE, L.abyCondNo[i])) return FALSE;
		L.abyCondCmp[i] = (BYTE)SendDlgItemMessage (s_hWndDlg, IDC_TE_CONDCMP0 + i, CB_GETCURSEL, 0, 0);
		if (L.abyCondCmp[i] >= TALKCMP_MAX) {
			L.abyCondCmp[i] = TALKCMP_GE;
		}
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_CONDVAL0 + i, 255, strName + szTmp + "値", TALKEDIT_TITLE, ul)) return FALSE;
		L.abyCondVal[i] = (BYTE)ul;
		if (L.abyCondNo[i] == FLAG_NONUM) {
			/* 使わない条件は空にしておく */
			L.abyCondCmp[i] = TALKCMP_GE;
			L.abyCondVal[i] = 0;
		}
	}
	if (!AdminGetText (s_hWndDlg, s_hWndDlg, IDC_TE_SPEECH, L.szSpeech, sizeof (L.szSpeech), TRUE, (strName + "台詞").c_str (), TALKEDIT_TITLE)) return FALSE;
	for (i = 0; i < MAX_TALKSET; i ++) {
		snprintf (szTmp, sizeof (szTmp), " %d 個目の", i + 1);
		if (!AdminFlagGetBit (s_hWndDlg, s_hWndDlg, IDC_TE_SETON0 + i, strName + "話した後に立てる印の" + szTmp + "番号", TALKEDIT_TITLE, L.awSetOn[i])) return FALSE;
		if (!AdminFlagGetBit (s_hWndDlg, s_hWndDlg, IDC_TE_SETOFF0 + i, strName + "話した後に消す印の" + szTmp + "番号", TALKEDIT_TITLE, L.awSetOff[i])) return FALSE;
	}
	for (i = 0; i < MAX_TALKACT; i ++) {
		snprintf (szTmp, sizeof (szTmp), "数値の動き%dの", i + 1);
		if (!AdminFlagGetNum (s_hWndDlg, s_hWndDlg, IDC_TE_ACT0 + i, strName + szTmp + "数値番号", TALKEDIT_TITLE, L.abyActNo[i])) return FALSE;
		L.abyActOp[i] = (BYTE)SendDlgItemMessage (s_hWndDlg, IDC_TE_ACTOP0 + i, CB_GETCURSEL, 0, 0);
		if (L.abyActOp[i] >= TALKNUMOP_MAX) {
			L.abyActOp[i] = TALKNUMOP_SET;
		}
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_ACTVAL0 + i, 255, strName + szTmp + "値", TALKEDIT_TITLE, ul)) return FALSE;
		L.abyActVal[i] = (BYTE)ul;
		if (L.abyActNo[i] == FLAG_NONUM) {
			L.abyActOp[i] = TALKNUMOP_SET;
			L.abyActVal[i] = 0;
		}
	}

	/* 受けている依頼の条件（依頼番号が空欄=条件なし。上限が空欄=どの段でも） */
	{
		BOOL bBlank;

		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_QID, 65535, strName + "受けている依頼の依頼番号", TALKEDIT_TITLE, ul)) return FALSE;
		L.wQuestID = (WORD)ul;
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_QSTEPMIN, 254, strName + "依頼の段の下限", TALKEDIT_TITLE, ul)) return FALSE;
		L.byQuestStepMin = (BYTE)ul;
		if (!AdminFlagReadNo (s_hWndDlg, IDC_TE_QSTEPMAX, ul, bBlank) || (!bBlank && (ul > 254))) {
			MessageBox (s_hWndDlg, (strName + "依頼の段の上限は 0〜254 で入れてください（空欄=どの段でも）。").c_str (), TALKEDIT_TITLE, MB_OK);
			SetFocus (GetDlgItem (s_hWndDlg, IDC_TE_QSTEPMAX));
			return FALSE;
		}
		L.byQuestStepMax = bBlank ? (BYTE)QUESTNPC_NOSTEP : (BYTE)ul;
		if (L.wQuestID == 0) {
			/* 条件を使わない時は、段も空にしておく */
			L.byQuestStepMin = 0;
			L.byQuestStepMax = QUESTNPC_NOSTEP;
		}
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_GIVEITEM, 65535, strName + "渡すアイテム", TALKEDIT_TITLE, ul)) return FALSE;
		L.wGiveItem = (WORD)ul;
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_NEEDITEM, 65535, strName + "持っている物", TALKEDIT_TITLE, ul)) return FALSE;
		L.wNeedItem = (WORD)ul;
		if (!AdminFlagGetValue (s_hWndDlg, s_hWndDlg, IDC_TE_TAKEITEM, 65535, strName + "取り上げる物", TALKEDIT_TITLE, ul)) return FALSE;
		L.wTakeItem = (WORD)ul;
	}
	/* 「はい・いいえ」で聞く行（いいえの返事は聞く行の時だけ使う） */
	L.byAsk = (IsDlgButtonChecked (s_hWndDlg, IDC_TE_ASK) == BST_CHECKED) ? 1 : 0;
	if (!AdminGetText (s_hWndDlg, s_hWndDlg, IDC_TE_NOMSG, L.szNoMsg, sizeof (L.szNoMsg), TRUE, (strName + "いいえの返事").c_str (), TALKEDIT_TITLE)) return FALSE;

	s_Info.aLine[nLine] = L;
	return TRUE;
}

/* Copyright(C)URARA-works 2026 */
