/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		NPCの巡回ダイアログ処理実装ファイル							 */
/* ファイル名：	AdminDlgNpcRoute.cpp										 */
/* 作成日：		2026/11/06													 */
/* ========================================================================= */
/*
 * 管理ページ（web/account/admin-npcroute.js）の「NPCの巡回」と同じことを、
 * ブラウザ版で使う旧版の管理者ウィンドウからできるようにしたもの。
 * リソースは IDD_NPCROUTE を使う。
 *
 * ・開いた時・「読み直す」・自分のマップが変わった時に、サーバーへ CMD_NPCROUTE(REQ) を送り、
 *   そのマップの「動かない」「ランダム移動」のNPCの巡回ルートを INFO で受け取って一覧にする。
 *   （名前・位置・種類は、クライアントが知っているキャラ情報から引く。
 *     まだ画面に入っていなくて知らないNPCは「未取得」と出す）
 * ・ゲーム画面の左クリックで道しるべを足し、右クリックでそのマスのNPCを選ぶ。
 * ・「保存」で CMD_NPCROUTE(SET) を送る。保存すると、サーバーですぐ歩き出す。
 * ・警告（保存は止めない）は管理ページと同じ内容。通れるかの判定はサーバーの
 *   CharProcMOVERAND.cpp の IsMovePos に合わせている。
 */
#include "stdafx.h"
#include <string>
#include <vector>
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../SboCliRecvCmd.h"
#include "AdminAutoLayout.h"
#include "AdminDlgNpcRoute.h"

/* ========================================================================= */
/* 定数・構造体の定義														 */
/* ========================================================================= */

#define	NPCROUTE_DEFAULTWAIT	3						/* 止まる秒数の初期値 */
#define	NPCROUTE_TICKTIME		500						/* マップ変更などを調べる間隔(ms) */
#define	NPCROUTE_NOSEL			((DWORD)-1)				/* 選んでいない */

/* 1体ぶんの巡回ルート（サーバーからの通知） */
struct NPCROUTE_ENTRY {
	DWORD	dwTblID;									/* テーブルID */
	BYTE	byCount;									/* 道しるべ数 */
	BYTE	byMode;										/* 回り方(NPCROUTEMODE_*) */
	BYTE	byWait;										/* 止まる秒数 */
	BYTE	abyX[MAX_NPCROUTE];							/* 道しるべ(横) */
	BYTE	abyY[MAX_NPCROUTE];							/* 道しるべ(縦) */
};

/* 一覧の列（名前と幅） */
static const struct {
	LPCSTR	pszName;
	int		nWidth;
} s_aColumn[] = {
	{"番号", 40}, {"名前", 100}, {"種類", 80}, {"位置", 55}, {"巡回", 60},
};

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */

static std::vector<NPCROUTE_ENTRY>	s_aEntry;					/* 一覧（サーバーから届いた巡回できるNPC） */
static std::vector<NPCROUTE_ENTRY>	s_aRecv;					/* 受信中の一覧（INFOEND で s_aEntry に入れ替える） */
static BOOL		s_bWaitList = FALSE;							/* 一覧(REQ)の応答を待っている */
static DWORD	s_dwMapID = (DWORD)-1;							/* 一覧のマップID（要求済みを含む） */
static DWORD	s_dwTickLast = 0;								/* 最後に調べた時刻 */
static int		s_nKnownCount = -1;								/* 一覧を出した時に名前が分かっていたNPC数 */

static DWORD	s_dwSelTblID = NPCROUTE_NOSEL;					/* 選んでいるNPCのテーブルID */
static int		s_nEditCount = 0;								/* 編集中の道しるべ数 */
static int		s_anEditX[MAX_NPCROUTE];						/* 編集中の道しるべ(横) */
static int		s_anEditY[MAX_NPCROUTE];						/* 編集中の道しるべ(縦) */
static int		s_nEditMode = NPCROUTEMODE_BACK;				/* 編集中の回り方 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgNpcRoute(HWND hWndParent);											/* ダイアログ表示 */
void CloseDlgNpcRoute(void);													/* ダイアログ終了 */
void NpcRouteOnRecv(const void *pPara);											/* サーバーからの通知 */
void NpcRouteOnPosLeft(int x, int y);											/* 左クリック */
void NpcRouteOnPosRight(int x, int y);											/* 右クリック */
void NpcRouteReload(void);														/* 取り直す */
void NpcRouteTick(void);														/* マップ変更の確認 */
BOOL NpcRouteIsListed(DWORD dwTblID);											/* 一覧にあるNPCか */
DWORD NpcRouteGetSelTblID(void);												/* 選んでいるNPC */
int NpcRouteGetEditCount(void);													/* 編集中の道しるべ数 */
void NpcRouteGetEditPoint(int nNo, int &x, int &y);								/* 編集中の道しるべ */
int NpcRouteGetEditMode(void);													/* 編集中の回り方 */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static LRESULT OnNotify(HWND hWnd, int nID, NMHDR *pHdr);						/* メッセージハンドラ(WM_NOTIFY) */
static void SendReq(void);														/* 一覧を要求 */
static void SendSet(int nCount);												/* 保存を送る */
static int FindEntry(DWORD dwTblID);											/* 一覧から探す */
static PCHARINFO GetClientChar(DWORD dwTblID);									/* クライアントが知っているキャラ */
static void SelectNpc(DWORD dwTblID, BOOL bListSelect);							/* NPCを選ぶ */
static void LoadEdit(const NPCROUTE_ENTRY *pEntry);								/* 一覧の内容を編集用に読み込む */
static void MakeNpcLabel(PCHARINFO pChar, char *pszDst, int nSize);				/* 名前（空なら台詞の出だし） */
static void RenewList(void);													/* 一覧を更新 */
static void RenewEdit(void);													/* 選択・道しるべ・警告の表示を更新 */
static void SetMsg(LPCSTR pszMsg);												/* 一言メッセージ */
static LPCSTR GetBlockReason(int x, int y);										/* 通れない理由（通れれば NULL） */
static void AddSegmentWarn(int ax, int ay, int bx, int by, LPCSTR pszLabel, std::string &strDst);	/* 2点の間の警告 */
static void MakeWarnings(std::string &strDst);									/* 警告を作る */

/* ========================================================================= */
/* 関数名：ShowDlgNpcRoute													 */
/* 内容：ダイアログ表示														 */
/* ========================================================================= */
void ShowDlgNpcRoute(HWND hWndParent)
{
	if (s_hWndDlg) {
		return;
	}

	s_aEntry.clear ();
	s_aRecv.clear ();
	s_bWaitList = FALSE;
	s_dwMapID = (DWORD)-1;
	s_nKnownCount = -1;
	s_dwSelTblID = NPCROUTE_NOSEL;
	s_nEditCount = 0;
	s_nEditMode = NPCROUTEMODE_BACK;

	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_NPCROUTE, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	RenewList ();
	RenewEdit ();
	ShowWindow (s_hWndDlg, SW_SHOW);
	SendReq ();
}

/* ========================================================================= */
/* 関数名：CloseDlgNpcRoute													 */
/* 内容：ダイアログ終了														 */
/* ========================================================================= */
void CloseDlgNpcRoute(void)
{
	if (s_hWndDlg) {
		DestroyWindow (s_hWndDlg);
	}
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
	s_aEntry.clear ();
	s_aRecv.clear ();
	s_bWaitList = FALSE;
	s_dwSelTblID = NPCROUTE_NOSEL;
	s_nEditCount = 0;
}

/* ========================================================================= */
/* 関数名：NpcRouteReload													 */
/* 内容：巡回ルートを取り直す（元に戻した後など）							 */
/* ========================================================================= */
void NpcRouteReload(void)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	SendReq ();
}

/* ========================================================================= */
/* 関数名：NpcRouteTick														 */
/* 内容：自分のマップが変わったか・名前の分かるNPCが増えたかを調べる			 */
/*       （ゲーム画面を描くたびに呼ぶ。間引いて調べる）						 */
/* ========================================================================= */
void NpcRouteTick(void)
{
	int i, nKnown;
	DWORD dwTime;

	if (s_hWndDlg == NULL || g_pMapInfo == NULL) {
		return;
	}
	dwTime = timeGetTime ();
	if ((DWORD)(dwTime - s_dwTickLast) < NPCROUTE_TICKTIME) {
		return;
	}
	s_dwTickLast = dwTime;

	/* マップが変わった？ */
	if (g_pMapInfo->dwID != s_dwMapID) {
		SendReq ();
		return;
	}

	/* 画面に入って名前が分かるようになったNPCが増えた？ */
	nKnown = 0;
	for (i = 0; i < (int)s_aEntry.size (); i ++) {
		if (GetClientChar (s_aEntry[i].dwTblID)) {
			nKnown ++;
		}
	}
	if (nKnown != s_nKnownCount) {
		RenewList ();
		RenewEdit ();
	}
}

/* ========================================================================= */
/* 関数名：NpcRouteIsListed													 */
/* 内容：一覧にあるNPCか（ゲーム画面に枠を描くため）						 */
/* ========================================================================= */
BOOL NpcRouteIsListed(DWORD dwTblID)
{
	return (FindEntry (dwTblID) >= 0);
}

/* ========================================================================= */
/* 関数名：NpcRouteGetSelTblID												 */
/* 内容：選んでいるNPCのテーブルID（無ければ -1）							 */
/* ========================================================================= */
DWORD NpcRouteGetSelTblID(void)
{
	return s_dwSelTblID;
}

/* ========================================================================= */
/* 関数名：NpcRouteGetEditCount												 */
/* 内容：編集中の道しるべ数													 */
/* ========================================================================= */
int NpcRouteGetEditCount(void)
{
	return (s_dwSelTblID == NPCROUTE_NOSEL) ? 0 : s_nEditCount;
}

/* ========================================================================= */
/* 関数名：NpcRouteGetEditPoint												 */
/* 内容：編集中の道しるべの座標												 */
/* ========================================================================= */
void NpcRouteGetEditPoint(int nNo, int &x, int &y)
{
	x = y = 0;
	if (nNo < 0 || nNo >= s_nEditCount) {
		return;
	}
	x = s_anEditX[nNo];
	y = s_anEditY[nNo];
}

/* ========================================================================= */
/* 関数名：NpcRouteGetEditMode												 */
/* 内容：編集中の回り方														 */
/* ========================================================================= */
int NpcRouteGetEditMode(void)
{
	return s_nEditMode;
}

/* ========================================================================= */
/* 関数名：NpcRouteOnRecv													 */
/* 内容：サーバーからの通知													 */
/*       INFO は INFOEND まで貯めて一覧を入れ替える。保存の後に届く INFO は	 */
/*       そのNPCだけ入れ替える												 */
/* ========================================================================= */
void NpcRouteOnRecv(const void *pParaIn)
{
	const CMDPARA_NPCROUTE *pPara = (const CMDPARA_NPCROUTE *)pParaIn;
	NPCROUTE_ENTRY Entry;
	int i, nNo;

	if (s_hWndDlg == NULL) {
		return;
	}

	switch (pPara->byCmdSub) {
	case NPCROUTE_INFO:			/* 1体ぶんのルート */
		if (pPara->dwMapID != s_dwMapID || pPara->byRouteCount > MAX_NPCROUTE) {
			break;
		}
		ZeroMemory (&Entry, sizeof (Entry));
		Entry.dwTblID	= pPara->dwTblID;
		Entry.byCount	= pPara->byRouteCount;
		Entry.byMode	= pPara->byRouteMode;
		Entry.byWait	= pPara->byRouteWait;
		for (i = 0; i < MAX_NPCROUTE; i ++) {
			Entry.abyX[i] = pPara->abyRouteX[i];
			Entry.abyY[i] = pPara->abyRouteY[i];
		}
		if (s_bWaitList) {
			s_aRecv.push_back (Entry);
			break;
		}
		/* 保存した後の通知：そのNPCだけ入れ替える */
		nNo = FindEntry (Entry.dwTblID);
		if (nNo < 0) {
			break;
		}
		s_aEntry[nNo] = Entry;
		RenewList ();
		if (Entry.dwTblID == s_dwSelTblID) {
			LoadEdit (&Entry);
		}
		RenewEdit ();
		break;

	case NPCROUTE_INFOEND:		/* 一覧の終わり */
		if (pPara->dwMapID != s_dwMapID || !s_bWaitList) {
			break;
		}
		s_bWaitList = FALSE;
		s_aEntry = s_aRecv;
		s_aRecv.clear ();
		/* 選んでいたNPCが居なくなっていたら選択を外す */
		nNo = FindEntry (s_dwSelTblID);
		if (nNo < 0) {
			s_dwSelTblID = NPCROUTE_NOSEL;
			s_nEditCount = 0;
		} else {
			LoadEdit (&s_aEntry[nNo]);
		}
		RenewList ();
		RenewEdit ();
		{
			char szTmp[64];
			snprintf (szTmp, sizeof (szTmp), "読み込みました（%d体）", (int)s_aEntry.size ());
			SetMsg (szTmp);
		}
		break;

	case NPCROUTE_RESULT:		/* 保存結果 */
		if (pPara->byResult) {
			SetMsg ("保存しました");
		} else {
			char szMsg[MAX_NPCROUTEMSG + 1];
			CopyMemory (szMsg, pPara->szMsg, MAX_NPCROUTEMSG);
			szMsg[MAX_NPCROUTEMSG] = 0;
			SetMsg ("保存できませんでした");
			MessageBox (s_hWndDlg, szMsg, "NPCの巡回", MB_OK);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：NpcRouteOnPosLeft												 */
/* 内容：左クリック：道しるべを足す（選択中のみ。16個まで。同じマスの連続は無視）	 */
/* ========================================================================= */
void NpcRouteOnPosLeft(int x, int y)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	if (s_dwSelTblID == NPCROUTE_NOSEL) {
		SetMsg ("先にNPCを選んでください（右クリックか一覧で）");
		return;
	}
	if (s_nEditCount >= MAX_NPCROUTE) {
		SetMsg ("道しるべは16個までです");
		return;
	}
	if (s_nEditCount > 0 && s_anEditX[s_nEditCount - 1] == x && s_anEditY[s_nEditCount - 1] == y) {
		return;
	}
	s_anEditX[s_nEditCount] = x;
	s_anEditY[s_nEditCount] = y;
	s_nEditCount ++;
	SetMsg ("");
	RenewEdit ();
}

/* ========================================================================= */
/* 関数名：NpcRouteOnPosRight												 */
/* 内容：右クリック：そのマスのNPCを選ぶ									 */
/* ========================================================================= */
void NpcRouteOnPosRight(int x, int y)
{
	int i;
	PCHARINFO pChar;

	if (s_hWndDlg == NULL) {
		return;
	}
	for (i = 0; i < (int)s_aEntry.size (); i ++) {
		pChar = GetClientChar (s_aEntry[i].dwTblID);
		if (pChar && (int)pChar->dwX == x && (int)pChar->dwY == y) {
			SelectNpc (s_aEntry[i].dwTblID, TRUE);
			return;
		}
	}
	SetMsg ("そのマスに巡回できるNPCはいません");
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
	char szTmp[16];

	/* ウィンドウサイズを親のクライアント領域に合わせる */
	GetClientRect (s_hWndParent, &rt);
	SetWindowPos (hWnd, NULL, 0, 0, rt.right, rt.bottom, SWP_NOZORDER | SWP_NOMOVE);

	hWndTmp = GetDlgItem (hWnd, IDC_NR_LIST);
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	for (i = 0; i < (int)(sizeof (s_aColumn) / sizeof (s_aColumn[0])); i ++) {
		lvcol.cx = s_aColumn[i].nWidth;
		lvcol.pszText = (LPSTR)s_aColumn[i].pszName;
		ListView_InsertColumn (hWndTmp, i, &lvcol);
	}
	ListView_SetExtendedListViewStyle (hWndTmp, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	/* 回り方・止まる秒数の初期値 */
	Button_SetCheck (GetDlgItem (hWnd, IDC_NR_MODE_BACK), BST_CHECKED);
	snprintf (szTmp, sizeof (szTmp), "%d", NPCROUTE_DEFAULTWAIT);
	SetWindowText (GetDlgItem (hWnd, IDC_NR_WAIT), szTmp);

	/* 操作の案内 */
	SetWindowText (GetDlgItem (hWnd, IDC_NR_MSG), "右クリック：そのマスの NPC を選ぶ／左クリック：道しるべを足す");

	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDC_NR_MODE_BACK:		/* 折り返す */
	case IDC_NR_MODE_LOOP:		/* 一周 */
		s_nEditMode = (nID == IDC_NR_MODE_LOOP) ? NPCROUTEMODE_LOOP : NPCROUTEMODE_BACK;
		RenewEdit ();
		break;

	case IDC_NR_UNDO:			/* 1つ戻す */
		if (s_nEditCount > 0) {
			s_nEditCount --;
		}
		SetMsg ("");
		RenewEdit ();
		break;

	case IDC_NR_CLEAR:			/* 全部消す */
		s_nEditCount = 0;
		SetMsg ("");
		RenewEdit ();
		break;

	case IDC_NR_SAVE:			/* 保存 */
		if (s_dwSelTblID == NPCROUTE_NOSEL) {
			MessageBox (hWnd, "保存するNPCを選んでください。", "NPCの巡回", MB_OK);
			break;
		}
		SendSet (s_nEditCount);
		break;

	case IDC_NR_STOP:			/* 巡回をやめる（道しるべ0個で保存） */
		if (s_dwSelTblID == NPCROUTE_NOSEL) {
			MessageBox (hWnd, "巡回をやめさせるNPCを選んでください。", "NPCの巡回", MB_OK);
			break;
		}
		SendSet (0);
		break;

	case IDC_NR_RELOAD:			/* 読み直す */
		SendReq ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnNotify															 */
/* 内容：メッセージハンドラ(WM_NOTIFY)。一覧の選択が変わったらNPCを選ぶ		 */
/* ========================================================================= */
static LRESULT OnNotify(HWND hWnd, int nID, NMHDR *pHdr)
{
	NMLISTVIEW *pNm = (NMLISTVIEW *)pHdr;

	if (pHdr == NULL || nID != IDC_NR_LIST || pHdr->code != LVN_ITEMCHANGED) {
		return 0;
	}
	if ((pNm->uNewState & LVIS_SELECTED) == 0 || (pNm->uOldState & LVIS_SELECTED)) {
		return 0;
	}
	/* 一覧の更新はこちらが選び直した時にも通知が来るので、もう選んでいるNPCなら何もしない */
	if (pNm->iItem < 0 || pNm->iItem >= (int)s_aEntry.size ()) {
		return 0;
	}
	if (s_aEntry[pNm->iItem].dwTblID == s_dwSelTblID) {
		return 0;
	}
	SelectNpc (s_aEntry[pNm->iItem].dwTblID, FALSE);
	return 0;
}

/* ========================================================================= */
/* 関数名：SendReq															 */
/* 内容：自分のいるマップの巡回ルートを要求する								 */
/* ========================================================================= */
static void SendReq(void)
{
	if (s_hWndDlg == NULL || g_pMapInfo == NULL) {
		return;
	}
	/* マップが変わっていたら、前のマップの一覧と選択は捨てる */
	if (g_pMapInfo->dwID != s_dwMapID) {
		s_aEntry.clear ();
		s_dwSelTblID = NPCROUTE_NOSEL;
		s_nEditCount = 0;
		RenewList ();
		RenewEdit ();
	}
	s_dwMapID = g_pMapInfo->dwID;
	s_aRecv.clear ();
	s_bWaitList = TRUE;
	SendCmdNPCROUTE (NPCROUTE_REQ, s_dwMapID, 0, 0, 0, 0, NULL, NULL);
	SetMsg ("読み込んでいます…");
}

/* ========================================================================= */
/* 関数名：SendSet															 */
/* 内容：選んでいるNPCの巡回ルートを保存する（nCount が 0 なら巡回をやめる）	 */
/* ========================================================================= */
static void SendSet(int nCount)
{
	int i, nWait;
	BYTE abyX[MAX_NPCROUTE], abyY[MAX_NPCROUTE];
	char szTmp[16];

	ZeroMemory (abyX, sizeof (abyX));
	ZeroMemory (abyY, sizeof (abyY));
	for (i = 0; i < nCount && i < MAX_NPCROUTE; i ++) {
		abyX[i] = (BYTE)s_anEditX[i];
		abyY[i] = (BYTE)s_anEditY[i];
	}

	/* 範囲外の秒数はサーバーが断る（0〜60） */
	ZeroMemory (szTmp, sizeof (szTmp));
	GetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_WAIT), szTmp, sizeof (szTmp));
	nWait = atoi (szTmp);
	nWait = max (0, min (nWait, 255));

	SendCmdNPCROUTE (NPCROUTE_SET, s_dwMapID, s_dwSelTblID, (BYTE)nCount, (BYTE)s_nEditMode, (BYTE)nWait, abyX, abyY);
	SetMsg ("保存しています…");
}

/* ========================================================================= */
/* 関数名：FindEntry														 */
/* 内容：一覧から探す（無ければ -1）										 */
/* ========================================================================= */
static int FindEntry(DWORD dwTblID)
{
	int i;

	for (i = 0; i < (int)s_aEntry.size (); i ++) {
		if (s_aEntry[i].dwTblID == dwTblID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetClientChar													 */
/* 内容：クライアントが知っているキャラを取得（まだ画面に入っていなければ NULL）	 */
/* ========================================================================= */
static PCHARINFO GetClientChar(DWORD dwTblID)
{
	int nNo = GetCharNo2 (dwTblID);

	if (nNo < 0) {
		return NULL;
	}
	return g_aCharInfo[nNo];
}

/* ========================================================================= */
/* 関数名：SelectNpc														 */
/* 内容：NPCを選ぶ（今の編集内容は捨てて、そのNPCの今のルートを読み込む）	 */
/* ========================================================================= */
static void SelectNpc(DWORD dwTblID, BOOL bListSelect)
{
	int nNo = FindEntry (dwTblID);

	if (nNo < 0) {
		return;
	}
	s_dwSelTblID = dwTblID;
	LoadEdit (&s_aEntry[nNo]);
	SetMsg ("");

	if (bListSelect && s_hWndDlg) {
		HWND hWndList = GetDlgItem (s_hWndDlg, IDC_NR_LIST);
		ListView_SetItemState (hWndList, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible (hWndList, nNo, FALSE);
	}
	RenewEdit ();
}

/* ========================================================================= */
/* 関数名：LoadEdit															 */
/* 内容：一覧の内容を編集用に読み込む（ルートが無ければ回り方は折り返す・秒数は初期値）	 */
/* ========================================================================= */
static void LoadEdit(const NPCROUTE_ENTRY *pEntry)
{
	int i;
	char szTmp[16];

	s_nEditCount = 0;
	if (pEntry->byCount >= 2) {
		s_nEditCount = min ((int)pEntry->byCount, (int)MAX_NPCROUTE);
		for (i = 0; i < s_nEditCount; i ++) {
			s_anEditX[i] = pEntry->abyX[i];
			s_anEditY[i] = pEntry->abyY[i];
		}
		s_nEditMode = (pEntry->byMode == NPCROUTEMODE_LOOP) ? NPCROUTEMODE_LOOP : NPCROUTEMODE_BACK;
		snprintf (szTmp, sizeof (szTmp), "%d", pEntry->byWait);
	} else {
		s_nEditMode = NPCROUTEMODE_BACK;
		snprintf (szTmp, sizeof (szTmp), "%d", NPCROUTE_DEFAULTWAIT);
	}
	if (s_hWndDlg) {
		SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_WAIT), szTmp);
		Button_SetCheck (GetDlgItem (s_hWndDlg, IDC_NR_MODE_BACK), (s_nEditMode == NPCROUTEMODE_BACK) ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck (GetDlgItem (s_hWndDlg, IDC_NR_MODE_LOOP), (s_nEditMode == NPCROUTEMODE_LOOP) ? BST_CHECKED : BST_UNCHECKED);
	}
}

/* ========================================================================= */
/* 関数名：MakeNpcLabel														 */
/* 内容：NPCの名前（名前が空なら台詞の出だしを目印にする）					 */
/* ========================================================================= */
static void MakeNpcLabel(PCHARINFO pChar, char *pszDst, int nSize)
{
	std::string strTmp;
	LPCSTR psz;
	char szHead[48];

	pszDst[0] = 0;
	if (pChar == NULL) {
		SboTextCopy (pszDst, nSize, "（未取得）");
		return;
	}
	if (pChar->szName[0]) {
		SboTextCopy (pszDst, nSize, pChar->szName);
		return;
	}

	/* 台詞から制御コード(@数字)と空白を除いた出だし */
	psz = pChar->pszNpcSpeech;
	while (psz && *psz && strTmp.size () < 80) {
		if (*psz == '@') {
			psz ++;
			while (*psz >= '0' && *psz <= '9') {
				psz ++;
			}
			continue;
		}
		if (*psz == ' ' || *psz == '\t' || *psz == '\r' || *psz == '\n') {
			psz ++;
			continue;
		}
		strTmp += *psz;
		psz ++;
	}
	if (strTmp.empty ()) {
		SboTextCopy (pszDst, nSize, "（名前なし）");
		return;
	}
	/* 文字の途中で切らないように、出だしを 14 文字ぶん（全角で 42 バイト）までにして「…）」を付ける */
	SboTextCopy (szHead, sizeof (szHead), strTmp.c_str ());
	strTmp = "（";
	strTmp += szHead;
	strTmp += "…）";
	SboTextCopy (pszDst, nSize, strTmp.c_str ());
}

/* ========================================================================= */
/* 関数名：RenewList														 */
/* 内容：一覧を更新															 */
/* ========================================================================= */
static void RenewList(void)
{
	int i, nCount, nNo;
	char szTmp[96];
	HWND hWndTmp;
	LV_ITEM item;
	PCHARINFO pChar;

	if (s_hWndDlg == NULL) {
		return;
	}
	hWndTmp = GetDlgItem (s_hWndDlg, IDC_NR_LIST);
	ListView_DeleteAllItems (hWndTmp);

	nCount = (int)s_aEntry.size ();
	s_nKnownCount = 0;
	for (i = 0; i < nCount; i ++) {
		const NPCROUTE_ENTRY *pEntry = &s_aEntry[i];
		int nCol = 0;

		pChar = GetClientChar (pEntry->dwTblID);
		if (pChar) {
			s_nKnownCount ++;
		}

		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = nCol ++;
		snprintf (szTmp, sizeof (szTmp), "#%u", (unsigned)pEntry->dwTblID);
		item.pszText = szTmp;
		ListView_InsertItem (hWndTmp, &item);

#define	SETCOL(pszCol)	{ item.iSubItem = nCol ++; item.pszText = (LPSTR)(pszCol); ListView_SetItem (hWndTmp, &item); }
		MakeNpcLabel (pChar, szTmp, sizeof (szTmp));
		SETCOL (szTmp);
		if (pChar) {
			SETCOL ((pChar->byNpcType < NPCTYPE_MAX) ? aNpcMoveTypeTbl[pChar->byNpcType] : "?");
			snprintf (szTmp, sizeof (szTmp), "(%d,%d)", (int)pChar->dwX, (int)pChar->dwY);
			SETCOL (szTmp);
		} else {
			SETCOL ("-");
			SETCOL ("-");
		}
		if (pEntry->byCount >= 2) {
			snprintf (szTmp, sizeof (szTmp), "あり(%d点)", (int)pEntry->byCount);
			SETCOL (szTmp);
		} else {
			SETCOL ("なし");
		}
#undef	SETCOL
	}

	/* 選んでいたNPCを選び直す */
	nNo = FindEntry (s_dwSelTblID);
	if (nNo >= 0) {
		ListView_SetItemState (hWndTmp, nNo, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}

	snprintf (szTmp, sizeof (szTmp), "このマップの巡回できるNPC：%d 体", nCount);
	SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_COUNT), szTmp);
}

/* ========================================================================= */
/* 関数名：RenewEdit														 */
/* 内容：選んだNPC・道しるべ一覧・警告の表示を更新							 */
/* ========================================================================= */
static void RenewEdit(void)
{
	int i, nNo;
	char szTmp[160], szName[96];
	std::string strTmp;
	PCHARINFO pChar;

	if (s_hWndDlg == NULL) {
		return;
	}

	/* 選んだNPC */
	nNo = FindEntry (s_dwSelTblID);
	if (nNo < 0) {
		SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_SELECT), "選んだNPC：（選んでいません）");
	} else {
		pChar = GetClientChar (s_dwSelTblID);
		MakeNpcLabel (pChar, szName, sizeof (szName));
		if (pChar) {
			snprintf (szTmp, sizeof (szTmp), "選んだNPC：#%u %s（%s）", (unsigned)s_dwSelTblID, szName,
					(pChar->byNpcType < NPCTYPE_MAX) ? aNpcMoveTypeTbl[pChar->byNpcType] : "?");
		} else {
			snprintf (szTmp, sizeof (szTmp), "選んだNPC：#%u %s", (unsigned)s_dwSelTblID, szName);
		}
		SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_SELECT), szTmp);
	}

	/* 回り方 */
	Button_SetCheck (GetDlgItem (s_hWndDlg, IDC_NR_MODE_BACK), (s_nEditMode == NPCROUTEMODE_BACK) ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck (GetDlgItem (s_hWndDlg, IDC_NR_MODE_LOOP), (s_nEditMode == NPCROUTEMODE_LOOP) ? BST_CHECKED : BST_UNCHECKED);

	/* 道しるべ一覧 */
	for (i = 0; nNo >= 0 && i < s_nEditCount; i ++) {
		snprintf (szTmp, sizeof (szTmp), "%s%d:(%d,%d)", i ? " → " : "", i + 1, s_anEditX[i], s_anEditY[i]);
		strTmp += szTmp;
	}
	if (strTmp.empty ()) {
		strTmp = (nNo < 0) ? "" : "（道しるべはありません。ゲーム画面を左クリックして足してください）";
	}
	SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_POINTS), strTmp.c_str ());

	/* 警告 */
	strTmp.clear ();
	if (nNo >= 0) {
		MakeWarnings (strTmp);
	}
	SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_WARN), strTmp.c_str ());
}

/* ========================================================================= */
/* 関数名：SetMsg															 */
/* 内容：一言メッセージ														 */
/* ========================================================================= */
static void SetMsg(LPCSTR pszMsg)
{
	if (s_hWndDlg == NULL) {
		return;
	}
	/* 何も知らせることが無い時は、操作の案内を出しておく */
	if ((pszMsg == NULL) || (pszMsg[0] == 0)) {
		pszMsg = "右クリック：そのマスの NPC を選ぶ／左クリック：道しるべを足す";
	}
	SetWindowText (GetDlgItem (s_hWndDlg, IDC_NR_MSG), pszMsg);
}

/* ========================================================================= */
/* 関数名：GetBlockReason													 */
/* 内容：そのマスを歩けない理由（歩ければ NULL）							 */
/*       サーバーの CharProcMOVERAND.cpp の IsMovePos に合わせている。		 */
/*       歩けるのは平地・移動パーツ・隠れる1だけ。イベントのマスも歩かない	 */
/* ========================================================================= */
static LPCSTR GetBlockReason(int x, int y)
{
	WORD wMap;
	BYTE byHitType;

	if (g_pMapInfo == NULL || g_pMapInfo->pwMap == NULL || g_pMapPartsInfo == NULL) {
		return NULL;
	}
	if (x < 0 || y < 0 || x >= g_pMapInfo->nCX || y >= g_pMapInfo->nCY) {
		return "マップの外";
	}

	wMap = g_pMapInfo->pwMap[x + y * g_pMapInfo->nCX];
	if (wMap >= g_wCountMapParts) {
		wMap = 0;
	}
	byHitType = g_pMapPartsInfo[wMap].byHitType;
	if (!((byHitType == PARTSHIT_NONE) ||
		  (byHitType == PARTSHIT_MOVEFRONT) ||
		  (byHitType == PARTSHIT_MOVEUP) ||
		  (byHitType == PARTSHIT_MOVEDOWN) ||
		  (byHitType == PARTSHIT_MOVERIGHT) ||
		  (byHitType == PARTSHIT_MOVELEFT) ||
		  (byHitType == PARTSHIT_HIDE1) ||
		  (byHitType == PARTSHIT_FOOTMARK_SNOW) ||
		  (byHitType == PARTSHIT_FOOTMARK_SAND) ||
		  (byHitType == PARTSHIT_WATERRIPPLE))) {
		return "壁などで通れない";
	}
	if (g_pMapInfo->pbyFlg && (g_pMapInfo->pbyFlg[x + y * g_pMapInfo->nCX] & MAPFLG_EVENT)) {
		return "扉・ワープなどのイベント";
	}
	return NULL;
}

/* ========================================================================= */
/* 関数名：AddSegmentWarn													 */
/* 内容：2点の間の警告を足す（斜め・間に通れないマスやイベントのマス）		 */
/* ========================================================================= */
static void AddSegmentWarn(int ax, int ay, int bx, int by, LPCSTR pszLabel, std::string &strDst)
{
	int x, y, dx, dy, nBad;
	char szTmp[64];
	std::string strBad;

	if (ax != bx && ay != by) {
		strDst += pszLabel;
		strDst += " が斜めです。同じ行か同じ列にまっすぐ置かないと、障害物にひっかかることがあります。\r\n";
		return;
	}

	dx = (bx > ax) - (bx < ax);
	dy = (by > ay) - (by < ay);
	x = ax;
	y = ay;
	nBad = 0;
	while (x != bx || y != by) {
		x += dx;
		y += dy;
		if (GetBlockReason (x, y)) {
			if (nBad < 5) {
				snprintf (szTmp, sizeof (szTmp), "%s(%d,%d)", nBad ? " " : "", x, y);
				strBad += szTmp;
			}
			nBad ++;
		}
	}
	if (nBad) {
		strDst += pszLabel;
		strDst += " の間に、通れないマスやイベントのマスがあります：";
		strDst += strBad;
		if (nBad > 5) {
			strDst += " ほか";
		}
		strDst += "\r\n";
	}
}

/* ========================================================================= */
/* 関数名：MakeWarnings														 */
/* 内容：警告を作る（保存は止めない。管理ページの collectWarnings と同じ内容）	 */
/* ========================================================================= */
static void MakeWarnings(std::string &strDst)
{
	int i;
	char szLabel[96];
	LPCSTR pszWhy;
	PCHARINFO pChar;

	if (s_dwSelTblID == NPCROUTE_NOSEL || s_nEditCount < 1) {
		return;
	}

	/* NPCの今の位置→道しるべ1（NPCをまだ知らない時は調べられない） */
	pChar = GetClientChar (s_dwSelTblID);
	if (pChar) {
		AddSegmentWarn ((int)pChar->dwX, (int)pChar->dwY, s_anEditX[0], s_anEditY[0],
				"NPCの今の位置→道しるべ1", strDst);
	}
	for (i = 1; i < s_nEditCount; i ++) {
		snprintf (szLabel, sizeof (szLabel), "道しるべ%d→%d", i, i + 1);
		AddSegmentWarn (s_anEditX[i - 1], s_anEditY[i - 1], s_anEditX[i], s_anEditY[i], szLabel, strDst);
	}
	if (s_nEditMode == NPCROUTEMODE_LOOP && s_nEditCount >= 3) {
		snprintf (szLabel, sizeof (szLabel), "道しるべ%d→1（一周の戻り）", s_nEditCount);
		AddSegmentWarn (s_anEditX[s_nEditCount - 1], s_anEditY[s_nEditCount - 1], s_anEditX[0], s_anEditY[0], szLabel, strDst);
	}
	for (i = 0; i < s_nEditCount; i ++) {
		pszWhy = GetBlockReason (s_anEditX[i], s_anEditY[i]);
		if (pszWhy) {
			snprintf (szLabel, sizeof (szLabel), "道しるべ%d (%d,%d) は通れないマスです（", i + 1, s_anEditX[i], s_anEditY[i]);
			strDst += szLabel;
			strDst += pszWhy;
			strDst += "）。\r\n";
		}
	}
	if (s_nEditCount == 1) {
		strDst += "道しるべは 2 つ以上置かないと保存できません。\r\n";
	}
}

/* Copyright(C)URARA-works 2026 */
