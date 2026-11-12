/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：		魔法情報編集ダイアログ処理実装ファイル						 */
/* ファイル名：	AdminDlgMagicInfoEdit.cpp									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/12													 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../resource.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "../SboCliMagicExplain.h"

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND s_hWndDlg = NULL;									/* ダイアログのウィンドウハンドル */
static HWND s_hWndParent = NULL;								/* 親ウィンドウ */
static int s_nNo = -1;											/* テーブル番号 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ShowDlgMagicInfoEdit(HWND hWndParent, int nNo);							/* ダイアログ表示(魔法情報) */
void CloseDlgMagicInfoEdit(void);												/* ダイアログ終了(魔法情報) */
static LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);					/* ダイアログ処理 */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);				/* メッセージハンドラ(WM_INITDIALOG) */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify);		/* メッセージハンドラ(WM_COMMAND) */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam);					/* メッセージハンドラ(WM_INITEND) */
static void GetDlgData(HWND hWnd, PMAGICINFO pInfo);							/* 入力欄の内容を取り出す（名前と説明文以外） */
static void SetExplainText(HWND hWnd, LPCSTR pszSrc);							/* 説明文の欄に文字を入れる */
static void GetExplainText(HWND hWnd, LPSTR pszDst);							/* 説明文の欄から文字を取り出す */

/* ========================================================================= */
/* 関数名：ShowDlgMagicInfoEdit												 */
/* 内容：ダイアログ表示(魔法情報編集)										 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void ShowDlgMagicInfoEdit(HWND hWndParent, int nNo)
{
	if (s_hWndDlg) {
		return;
	}

	s_nNo = nNo;
	s_hWndParent = hWndParent;
	s_hWndDlg = CreateDialog (NULL, (LPCTSTR)IDD_MAGICINFO_EDIT, hWndParent, (DLGPROC)DlgProc);
	if (s_hWndDlg == NULL) {
		return;
	}

	EnableWindow (hWndParent, FALSE);
	ShowWindow (s_hWndDlg, SW_SHOW);
}

/* ========================================================================= */
/* 関数名：CloseDlgMagicInfoEdit											 */
/* 内容：ダイアログ終了(魔法情報編集)										 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void CloseDlgMagicInfoEdit(void)
{
	EnableWindow (s_hWndParent, TRUE);
	DestroyWindow (s_hWndDlg);
	s_hWndDlg = NULL;
	s_hWndParent = NULL;
}

/* ========================================================================= */
/* 関数名：DlgProc															 */
/* 内容：ダイアログ処理														 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG (hWnd, WM_COMMAND, OnCommand);

	case WM_INITEND:
		OnInitEnd (hWnd, wParam, lParam);
		break;

	default:
		return 0;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：OnInitDialog														 */
/* 内容：メッセージハンドラ(WM_INITDIALOG)									 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
static BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	int i, nCount;
	HWND hWndTmp;

	/* 「効果」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPE);
	for (i = 0; ; i ++) {
		if (g_aStrType[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrType[i]);
	}

	/* 「種別」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TYPEINFO);
	for (i = 0; ; i ++) {
		if (g_aStrTypeInfo[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrTypeInfo[i]);
	}

	/* 「利用者」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_USERINFO);
	nCount = g_aMagicUserInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PMAGICUSERINFO pInfo;

		pInfo = g_aMagicUserInfo[i];
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
	}

	/* 「対象」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TARGET);
	for (i = 0; ; i ++) {
		if (g_aStrTarget[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrTarget[i]);
	}

	/* 「人数」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TARGETCOUNT);
	for (i = 0; ; i ++) {
		if (g_aStrTargetCount[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrTargetCount[i]);
	}

	/* 「発動」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TARGETTYPE);
	for (i = 0; ; i ++) {
		if (g_aStrTargeType[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrTargeType[i]);
	}

	/* 「基本詠唱時間」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_TIME);
	for (i = 0; i <= 10; i ++) {
		char szTmp[64];
		wsprintf (szTmp, "%d秒", i);
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)szTmp);
	}

	/* 「属性」コンボボックスに項目追加 */
	hWndTmp = GetDlgItem (hWnd, IDC_ATTR);
	for (i = 0; ; i ++) {
		if (g_aStrAttrType[i][0] == 'z') { break; }
		SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)g_aStrAttrType[i]);
	}

	lParam = 0;

	/* 編集？ */
	if (s_nNo >= 0) {
		PMAGICINFO pInfo;

		pInfo = g_aMagicInfo[s_nNo];
		lParam = (LPARAM)pInfo->byType;
	}

	SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_SETCURSEL, lParam, 0);
	SendMessage (hWnd, WM_COMMAND, MAKEWPARAM(IDC_TYPE, 0), 0);
	SendMessage (hWnd, WM_INITEND, 0, 0);
	return FALSE;
}

/* ========================================================================= */
/* 関数名：OnCommand														 */
/* 内容：メッセージハンドラ(WM_COMMAND)										 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
static void OnCommand(HWND hWnd, int nID, HWND hWndCtl, UINT nCodeNotify)
{
	switch (nID) {
	case IDOK:
		{
			PMAGICINFO pInfo;
			char szTmp[128];
			HWND hWndTmp;

			/* 新規登録？ */
			if (s_nNo < 0) {
				pInfo = new MAGICINFO;
				ZeroMemory (pInfo, sizeof (MAGICINFO));
				g_aMagicInfo.Add (pInfo);

			} else {
				pInfo = g_aMagicInfo[s_nNo];
			}
			hWndTmp = GetDlgItem (hWnd, IDC_NAME);
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (hWndTmp, szTmp, sizeof (szTmp) - 1);
			szTmp[SboTextFitWidth (szTmp, MAX_MAGICNAME_WIDTH)] = 0;		/* 画面に収まる長さまで */
			strcpy (pInfo->szName, szTmp);

			GetDlgData (hWnd, pInfo);
			GetExplainText (hWnd, pInfo->szExplain);

			if (s_nNo < 0) {
				SendCMDMAGICINFO ((BYTE)(g_aMagicInfo.GetSize () - 1));
			}

			PostMessage (s_hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, IDD_MAGICINFO_EDIT), 0);
			CloseDlgMagicInfoEdit ();
		}
		break;

	case IDCANCEL:
		CloseDlgMagicInfoEdit ();
		break;

	case IDC_EXPLAIN_AUTO:	/* 説明文を今の入力内容から自動で作る */
		{
			MAGICINFO Info;
			char szTmp[MAX_MAGICEXPLAIN];

			ZeroMemory (&Info, sizeof (Info));
			GetDlgData (hWnd, &Info);
			MakeMagicExplainAuto (&Info, szTmp, sizeof (szTmp));
			SetExplainText (hWnd, szTmp);
		}
		break;

	case IDC_TYPE:
		{
			HWND hWndTmp;
			int i, nCount, nType;
			PMAGICINFO pInfo;

			SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "");
			SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "");
			SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA3), "");
			SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA4), "");
			ShowWindow (GetDlgItem (hWnd, IDC_DATA1), FALSE);
			ShowWindow (GetDlgItem (hWnd, IDC_DATA2), FALSE);
			ShowWindow (GetDlgItem (hWnd, IDC_DATA3), FALSE);
			ShowWindow (GetDlgItem (hWnd, IDC_DATA4), FALSE);
			ShowWindow (GetDlgItem (hWnd, IDC_DATACOMBO1), FALSE);
			SendMessage (GetDlgItem (hWnd, IDC_DATACOMBO1), CB_RESETCONTENT, 0, 0);

			nType = SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_GETCURSEL, 0, 0);
			/* 付加データのラベルと入力欄の設定 */
			switch (nType) {
			case MAGICTYPE_HPDOWN:		/* HP減少 */
			case MAGICTYPE_HPDOWNAT:	/* HP減少(攻撃力) */
			case MAGICTYPE_HPDOWNHPCOST:	/* HP減少(HP消費) */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "最低値：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "最大値：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				break;

			case MAGICTYPE_HPUP:		/* HP増加 */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "最低値：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "最大値：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				break;

			case MAGICTYPE_LIGHT:		/* 明かり */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "明るさﾚﾍﾞﾙ：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "維持時間：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				break;

			case MAGICTYPE_PUT:			/* 魔法発生 */
			case MAGICTYPE_PUTSELF:		/* 魔法発生(自分) */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "発生魔法：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA3), "発生期間(秒)：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA4), "移動確率(%)：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA3), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA4), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATACOMBO1), TRUE);

				/* 「魔法発生」コンボボックスに項目追加 */
				hWndTmp = GetDlgItem (hWnd, IDC_DATACOMBO1);
				nCount = g_aMagicInfo.GetSize ();

				for (i = 0; i < nCount; i ++) {
					pInfo = g_aMagicInfo[i];

					SendMessage (hWndTmp, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
				}
				break;

			case MAGICTYPE_RESURRECTION:	/* 蘇生 */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "回復率(%)：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				break;
			case MAGICTYPE_HPDOWNRATE:		/* HP減少(率) */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "減少率(%)：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				break;
			case MAGICTYPE_HASTE:			/* 加速 */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "速くなる率(%)：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "続く秒数：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				break;
			case MAGICTYPE_POISON:			/* 毒 */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "2秒ごとのﾀﾞﾒｰｼﾞ：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "続く秒数：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA3), "成功率(%)0=100：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA3), TRUE);
				break;
			case MAGICTYPE_SLEEP:			/* 眠り */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "眠る秒数：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "成功率(%)0=100：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				break;
			case MAGICTYPE_ATUP:			/* 攻撃力アップ */
			case MAGICTYPE_DFUP:			/* 守備力アップ */
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA1), "上がる率(%)：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA2), "続く秒数：");
				SetWindowText (GetDlgItem (hWnd, IDC_STATIC_DATA3), "最低でも上がる量：");
				ShowWindow (GetDlgItem (hWnd, IDC_DATA1), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA2), TRUE);
				ShowWindow (GetDlgItem (hWnd, IDC_DATA3), TRUE);
				break;
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：OnInitEnd														 */
/* 内容：メッセージハンドラ(WM_INITEND)										 */
/* 日付：2004/07/31															 */
/* ========================================================================= */
static void OnInitEnd(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int nCount;
	HWND hWndTmp;
	char szTmp[128];
	PMAGICINFO pInfo;
	BOOL bDelete = FALSE;

	/* 編集？ */
	if (s_nNo >= 0) {
		pInfo = g_aMagicInfo[s_nNo];

	} else {
		bDelete = TRUE;
		pInfo = new MAGICINFO;
		ZeroMemory (pInfo, sizeof (MAGICINFO));
		/* 各コンボボックスの初期値を設定 */
		pInfo->byType = MAGICTYPE_NONE;
		pInfo->byTypeInfoID = MAGICUSETYPE_WALK;
		pInfo->byTarget = MAGICTARGET_PC;
		pInfo->byTargetCount = MAGICTARGETCOUNT_ONE;
		pInfo->byTargetType = MAGICTARGETTYPE_SELF;
		pInfo->byUserInfo = 0;
		pInfo->byTime = 0;
	}

	hWndTmp = GetDlgItem (hWnd, IDC_NAME);
	SetWindowText (hWndTmp, pInfo->szName);
	SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_SETCURSEL, pInfo->byType, 0);
	SendMessage (GetDlgItem (hWnd, IDC_TYPEINFO), CB_SETCURSEL, pInfo->byTypeInfoID, 0);
	SendMessage (GetDlgItem (hWnd, IDC_TARGET), CB_SETCURSEL, pInfo->byTarget, 0);
	SendMessage (GetDlgItem (hWnd, IDC_TARGETCOUNT), CB_SETCURSEL, pInfo->byTargetCount, 0);
	SendMessage (GetDlgItem (hWnd, IDC_TARGETTYPE), CB_SETCURSEL, pInfo->byTargetType, 0);
	SendMessage (GetDlgItem (hWnd, IDC_USERINFO), CB_SETCURSEL, pInfo->byUserInfo, 0);
	SendMessage (GetDlgItem (hWnd, IDC_TIME), CB_SETCURSEL, pInfo->byTime, 0);
	SendMessage (GetDlgItem (hWnd, IDC_ATTR), CB_SETCURSEL, pInfo->byAttr, 0);

	wsprintf (szTmp, "%d", pInfo->byTargetArea);
	SetWindowText (GetDlgItem (hWnd, IDC_TARGETAREA), szTmp);
	wsprintf (szTmp, "%d", pInfo->byTargetEfectID);
	SetWindowText (GetDlgItem (hWnd, IDC_TARGETEFECTID), szTmp);
	wsprintf (szTmp, "%d", pInfo->byUserEfectID);
	SetWindowText (GetDlgItem (hWnd, IDC_USEREFECTID), szTmp);
	wsprintf (szTmp, "%d", pInfo->wData1);
	SetWindowText (GetDlgItem (hWnd, IDC_DATA1), szTmp);
	wsprintf (szTmp, "%d", pInfo->wData2);
	SetWindowText (GetDlgItem (hWnd, IDC_DATA2), szTmp);
	wsprintf (szTmp, "%d", pInfo->wData3);
	SetWindowText (GetDlgItem (hWnd, IDC_DATA3), szTmp);
	wsprintf (szTmp, "%d", pInfo->wData4);
	SetWindowText (GetDlgItem (hWnd, IDC_DATA4), szTmp);
	wsprintf (szTmp, "%d", pInfo->wMana);
	SetWindowText (GetDlgItem (hWnd, IDC_MANA), szTmp);
	SetExplainText (hWnd, pInfo->szExplain);

	if ((pInfo->byType == MAGICTYPE_PUT) || (pInfo->byType == MAGICTYPE_PUTSELF)) {
		/* 「魔法発生」コンボボックスに項目追加 */
		hWndTmp = GetDlgItem (hWnd, IDC_DATACOMBO1);
		nCount = g_aMagicInfo.GetSize ();

		SendMessage (GetDlgItem (hWnd, IDC_DATACOMBO1), CB_SETCURSEL, pInfo->wData1, 0);
		wsprintf (szTmp, "%d", pInfo->wData2);
		SetWindowText (GetDlgItem (hWnd, IDC_DATA3), szTmp);
	}

	if (bDelete) {
		SAFE_DELETE (pInfo);
	}
}

/* ========================================================================= */
/* 関数名：	GetDlgData														 */
/* 内容：	入力欄の内容を取り出す（名前と説明文以外）						 */
/*			OK の時と「自動作成」の時の両方で使う							 */
/* ========================================================================= */
static void GetDlgData(HWND hWnd, PMAGICINFO pInfo)
{
	char szTmp[128];

	pInfo->byType = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TYPE), CB_GETCURSEL, 0, 0);
	pInfo->byTypeInfoID = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TYPEINFO), CB_GETCURSEL, 0, 0);
	pInfo->byTarget = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TARGET), CB_GETCURSEL, 0, 0);
	pInfo->byTargetCount = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TARGETCOUNT), CB_GETCURSEL, 0, 0);
	pInfo->byTargetType = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TARGETTYPE), CB_GETCURSEL, 0, 0);
	pInfo->byUserInfo = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_USERINFO), CB_GETCURSEL, 0, 0);
	pInfo->byTime = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_TIME), CB_GETCURSEL, 0, 0);
	pInfo->byAttr = (BYTE)SendMessage (GetDlgItem (hWnd, IDC_ATTR), CB_GETCURSEL, 0, 0);

	GetWindowText (GetDlgItem (hWnd, IDC_TARGETAREA), szTmp, sizeof (szTmp) - 1);
	pInfo->byTargetArea = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_TARGETEFECTID), szTmp, sizeof (szTmp) - 1);
	pInfo->byTargetEfectID = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_USEREFECTID), szTmp, sizeof (szTmp) - 1);
	pInfo->byUserEfectID = (BYTE)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_DATA1), szTmp, sizeof (szTmp) - 1);
	pInfo->wData1 = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_DATA2), szTmp, sizeof (szTmp) - 1);
	pInfo->wData2 = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_DATA3), szTmp, sizeof (szTmp) - 1);
	pInfo->wData3 = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_DATA4), szTmp, sizeof (szTmp) - 1);
	pInfo->wData4 = (WORD)atoi (szTmp);
	GetWindowText (GetDlgItem (hWnd, IDC_MANA), szTmp, sizeof (szTmp) - 1);
	pInfo->wMana = (WORD)atoi (szTmp);

	if ((pInfo->byType == MAGICTYPE_PUT) || (pInfo->byType == MAGICTYPE_PUTSELF)) {
		pInfo->wData1 = (WORD)SendMessage (GetDlgItem (hWnd, IDC_DATACOMBO1), CB_GETCURSEL, 0, 0);
		GetWindowText (GetDlgItem (hWnd, IDC_DATA3), szTmp, sizeof (szTmp) - 1);
		pInfo->wData2 = (WORD)atoi (szTmp);
	}
}

/* ========================================================================= */
/* 関数名：	SetExplainText													 */
/* 内容：	説明文の欄に文字を入れる（改行は \r\n にして入れる）			 */
/* ========================================================================= */
static void SetExplainText(HWND hWnd, LPCSTR pszSrc)
{
	char szTmp[MAX_MAGICEXPLAIN * 2];
	int i, nPos;

	nPos = 0;
	for (i = 0; pszSrc[i] && (nPos < (int)sizeof (szTmp) - 3); i ++) {
		if (pszSrc[i] == '\r') {
			continue;
		}
		if (pszSrc[i] == '\n') {
			szTmp[nPos ++] = '\r';
		}
		szTmp[nPos ++] = pszSrc[i];
	}
	szTmp[nPos] = 0;
	SetWindowText (GetDlgItem (hWnd, IDC_EXPLAIN), szTmp);
}

/* ========================================================================= */
/* 関数名：	GetExplainText													 */
/* 内容：	説明文の欄から文字を取り出す									 */
/*			改行は \n にし、入りきらない分は全角の途中で切らずに捨てる。	 */
/*			後ろの改行と空白は取る											 */
/* ========================================================================= */
static void GetExplainText(HWND hWnd, LPSTR pszDst)
{
	char szTmp[2048];
	int i, nChar, nPos, nWidth;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetWindowText (GetDlgItem (hWnd, IDC_EXPLAIN), szTmp, sizeof (szTmp) - 1);

	nPos = 0;
	nWidth = 0;
	for (i = 0; szTmp[i]; i += nChar) {
		nChar = SboTextCharLen (&szTmp[i]);
		if (szTmp[i] == '\r') {
			continue;
		}
		/* 表示幅で上限を決める。欄のバイト数からもはみ出さないように */
		if ((nWidth + SboTextCharWidth (&szTmp[i]) > MAX_MAGICEXPLAIN_WIDTH) ||
			(nPos + nChar > MAX_MAGICEXPLAIN - 1)) {
			break;
		}
		CopyMemory (&pszDst[nPos], &szTmp[i], nChar);
		nPos += nChar;
		nWidth += SboTextCharWidth (&szTmp[i]);
	}
	while ((nPos > 0) && ((pszDst[nPos - 1] == '\n') || (pszDst[nPos - 1] == ' '))) {
		nPos --;
	}
	pszDst[nPos] = 0;
}

/* Copyright(C)URARA-works 2003-2005 */
