/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			スクラップブックオンライン（サーバー）					 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */
#include "include.h"
#include <wininet.h>
#include <objbase.h>
#include "../resource.h"
#include "CharProc/IncludeCharProc.h"
#include "Proc/SubGame/ProcSubGameWAR.h"
#include "Proc/ProcSubGame.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrDrawWindow.h"
#include <vector>
#include "SboSvrDlgLeaveInfo.h"
#include "SaveLoad/SaveLoad.h"
#include "SboSvrBattleProc.h"
#include "SboSvrMagicProc.h"
#include "SboSvrItemProc.h"
#include "SboSvrEventProc.h"
#include "SboSvrSubGameProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrSubGameProcSugoroku.h"
#include "SboSvrDataUtf8.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define CLNAME "SBOSVR"												/* 登録クラス名 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
HANDLE g_mutexMain = NULL;											/* 二重起動防止用ミューテックス */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);					/* メインウィンドウメッセージ処理 */
BOOL OnCreate			(HWND);										/* メッセージハンドラ(WM_CREATE) */
void OnDestroy			(HWND);										/* メッセージハンドラ(WM_DESTROY) */
void OnDecPlayer		(int nNo);									/* メッセージハンドラ(WM_DECPLAYER) */
void TimerProc			(void);										/* タイマー処理 */
BOOL TimerProcChar		(PCHARINFO pChar);							/* タイマー処理(キャラ) */
void MakeAccList		(void);										/* アカウント一覧作成 */
void WriteData			(HANDLE, LPSTR, DWORD, PBYTE);				/* データ書き込み */
BOOL SaveMapPartsInfo	(void);										/* マップパーツ情報保存 */
BOOL LoadMapPartsInfo	(void);										/* マップパーツ情報読み込み */
BOOL SaveMapInfo		(void);										/* マップ情報保存 */
BOOL LoadMapInfo		(void);										/* マップ情報読み込み */
BOOL SaveLeaveInfo		(void);										/* 預かり所情報保存 */
BOOL LoadLeaveInfo		(void);										/* 預かり所情報読み込み */

#ifdef __EMSCRIPTEN__
extern void SvrLogSettingWarning	(int nKind);						/* 設定の注意をログに出す（server/server_main.cpp） */
#endif
extern void RecvCmdOnDecPlayer	(int nNo);							/* 受信処理の状態を消す（SboSvrRecvCmd.cpp） */
extern void WebThreadInit		(void);								/* Web更新スレッド開始 */
extern void WebThreadRenew		(void);								/* Web更新 */
extern void WebThreadDestroy	(void);								/* Web更新スレッド終了 */

extern void sgenrand(unsigned long);								/* 乱数初期化 */
extern long genrand();												/* 乱数取得 */

#ifdef __EMSCRIPTEN__
/* ========================================================================= */
/* Node.js 版のメインループ													 */
/*   Node.js ではずっと回り続けるループを書けないので、WinMain のループを		 */
/*   1 回分ずつ呼ぶ。TimerProc の「ちょっと休憩」（MsgWaitForMultipleObjects）	 */
/*   を頼まれたら、その時間だけ Node.js に戻して通信などを受け取る。			 */
/* ========================================================================= */
static HINSTANCE	s_hInstMain;
static UINT			s_uPeriodMin;

/* 誰もつながっていない時の休憩時間（ミリ秒）。元の 10ms のままだと、誰もいなくても
   キャラ処理を 1 秒に 100 周ほど回して CPU を使い続けるので、その間だけ長めに休む */
#define IDLE_WAIT_MS	100
extern "C" int SvrNetConnCount (void);					/* いまつながっている数（server/CUraraSockTCP_svr.cpp） */

static void MainLoopStep(void)
{
	MSG msg;
	int nLoop, nWait;
	BOOL bRenewTime;

	bRenewTime = TRUE;
	for (nLoop = 0; nLoop < 100000; nLoop ++) {
		/* 時刻を取るのは重いので（キャラ 1 体の処理で何度も呼ばれる）、キャラ 32 体分か
		   メッセージを 1 つ処理するまでは同じ値を使う */
		if (bRenewTime || ((nLoop % 32) == 0)) {
			W32BeginTimeCache ();
			bRenewTime = FALSE;
		}
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				W32EndTimeCache ();
				emscripten_cancel_main_loop ();
				timeEndPeriod (s_uPeriodMin);
				UnregisterClass (CLNAME, s_hInstMain);
				ReleaseMutex (g_mutexMain);
				CloseHandle (g_mutexMain);
				emscripten_force_exit ((int)msg.wParam);
				return;
			}
			TranslateMessage (&msg);
			DispatchMessage (&msg);
			bRenewTime = TRUE;

		} else {
			TimerProc ();
		}
		nWait = W32TakeYieldRequest ();
		if (nWait >= 0) {
			W32EndTimeCache ();
			if ((nWait < IDLE_WAIT_MS) && (SvrNetConnCount () == 0)) {
				nWait = IDLE_WAIT_MS;
			}
			emscripten_set_main_loop_timing (EM_TIMING_SETTIMEOUT, nWait);
			return;
		}
	}
	W32EndTimeCache ();
	emscripten_set_main_loop_timing (EM_TIMING_SETTIMEOUT, 0);
}
#endif

/* ========================================================================= */
/* 関数名：WinMain															 */
/* 内容：ウィンドウのメイン関数												 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode)
{
	HWND hWnd;
	MSG msg;
	WNDCLASS wc;

	g_mutexMain = CreateMutex (NULL, TRUE, "SboSvr");
	if (!g_mutexMain || GetLastError () == ERROR_ALREADY_EXISTS) {
//		return FALSE;
	}

	g_hInst = hInstance;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLNAME;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.style = 0;
	wc.hIcon = LoadIcon (hInstance, (char *)IDI_ICON1);
	wc.hCursor = LoadCursor ((HINSTANCE)NULL, IDC_ARROW);
	wc.lpszMenuName = "IDR_MENU";
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);

	/* ウィンドウのクラスを登録 */
	if (!RegisterClass (&wc)) {
		return (FALSE);
	}

	/* ウィンドウ作成 */
	char szBuf[128];
	wsprintf (szBuf, "%s%s", WNDTITLE, VERTEXT);
	hWnd = CreateWindow (CLNAME, szBuf, WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT,
				strlen (WNDTITLE) * 12, 150,
				NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}

	g_hWndMain = hWnd;

	/* ウィンドウの外枠を描画 */
	ShowWindow (hWnd, nWinMode);
	/* ウィンドウのクライアント領域を描画 */
	UpdateWindow (hWnd);

	TIMECAPS tc;
	timeGetDevCaps (&tc, sizeof (TIMECAPS));
	/* マルチメディアタイマーのサービス精度を最大に */
	timeBeginPeriod (tc.wPeriodMin);

#ifdef __EMSCRIPTEN__
	/* ここから先は MainLoopStep が繰り返し呼ばれる（この関数には戻らない） */
	s_hInstMain = hInstance;
	s_uPeriodMin = tc.wPeriodMin;
	emscripten_set_main_loop (MainLoopStep, 1000, 1);
#endif

	while (1) {
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage (&msg);
			DispatchMessage (&msg);

		} else {
			TimerProc ();
		}
	}

	timeEndPeriod (tc.wPeriodMin);
	UnregisterClass (CLNAME, hInstance);

	ReleaseMutex (g_mutexMain);
	CloseHandle (g_mutexMain);

	/* 終了メッセージによりプログラム終了 */
	return (msg.wParam);
}


/* ========================================================================= */
/* 関数名：WndProc															 */
/* 内容：メッセージの処理													 */
/* 日付：2003/05/02															 */
/* ========================================================================= */

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
		return OnCreate (hWnd);

	/* データ受信 */
	case WM_URARASOCK_RECV:
	case WM_RCVDATA:
		if (wParam) {
			RecvDataProc ((BYTE *)wParam, (DWORD)lParam);
		}
		break;

	/* プレイヤー追加 */
	case WM_URARASOCK_ADDCLIENT:
		wParam = lParam;
	case WM_ADDPLAYER:
		{
			DWORD pID = (DWORD)wParam;

			/* 接続応答 */
			SendCmdCONNECT (pID, VERSIONVAL);
		}
		break;

	/* プレイヤー削除 */
	case WM_URARASOCK_DECCLIENT:
		lParam = GetCharNo (lParam);
		if (lParam < 0) {
			break;
		}
	case WM_DECPLAYER:
		OnDecPlayer ((int)lParam);
		break;

	case WM_TIMER:
		/* 1秒毎の描画更新 */
		if (wParam == TIMERID_REDRAW) {
			SYSTEMTIME sysTime;
			GetLocalTime (&sysTime);

			if (sysTime.wMinute == 0) {
				if (sysTime.wHour != g_byLastSendClock) {
					g_byLastSendClock = (BYTE)sysTime.wHour;
					char szTmp[128];
					wsprintf (szTmp, "SYSTEM:サーバーが%d時頃をお知らせします。", g_byLastSendClock);
					SendCmdSYSMSG (URARASOCK_SENDALL, szTmp);

					if (g_dwOnline > g_dwOnlineBack[sysTime.wHour]) {
						g_dwOnlineBack[sysTime.wHour] = g_dwOnline;
						if (g_dwOnline > g_dwOnlineBackMax[sysTime.wHour]) {
							g_dwOnlineBackMax[sysTime.wHour] = g_dwOnline;
						}
					}
				}
			}
			InvalidateRect (hWnd, NULL, TRUE);

		/* 定期保存 */
		} else if (wParam == TIMERID_SAVEINFO) {
			SendCmdSYSMSG (URARASOCK_SENDALL, "SYSTEM:サーバー情報を保存します。");
			SaveCharInfo ();
			SaveMapPartsInfo ();
			SaveMapInfo ();
			SaveComEfcInfo ();
			SaveEfcInfo ();
			SaveItemInfo ();
			SaveStTmpInfo ();
			SaveLeaveInfo ();
			SaveSubGameInfo ();
			SaveJobInfo ();
			SaveMagicUserInfo ();
			SaveMagicInfo ();
			SaveEquUserInfo ();
			SaveMakerInfo ();
			SaveRecipeInfo ();
			SendCmdSYSMSG (URARASOCK_SENDALL, "SYSTEM:サーバー情報を保存しました。");
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC;

			hDC = BeginPaint (hWnd, &ps);
			DrawWindow (hDC);
			EndPaint (hWnd, &ps);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD (wParam)) {
		case IDM_QUIT:
			PostMessage (hWnd, WM_CLOSE, 0, 0);
			break;

		case IDM_CHAR_LEAVE:		/* 預かり所 */
			{
				HWND hWndTmp = CreateDialog (NULL, (LPCTSTR)IDD_LEAVEINFO, hWnd, (DLGPROC)DlgProcLeaveInfo);
				if (hWndTmp) {
					ShowWindow (hWndTmp, SW_SHOW);
				}
			}
			break;

		case IDM_CHAR_MAKEACCLIST:	/* アカウント一覧作成 */
			MakeAccList ();
			break;
		}
		break;

	/* ウィンドウを閉じる */
	case WM_DESTROY:
		OnDestroy (hWnd);
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}
	return (FALSE);
}


/* ========================================================================= */
/* 関数名：	OnCreate														 */
/* 内容：	メッセージハンドラ(WM_CREATE)									 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

BOOL OnCreate(HWND hWnd)
{
	char szBuf[MAX_PATH], szLoginLogFileName[MAX_PATH];
	int i;

	CoInitialize (NULL);

	g_pServerInfoUploadInfo = NULL;
	g_dwOnline = 0;
	g_dwNewMapID = 0;
	g_byAdminCount = 0;
	g_nProcChar = 0;
	g_nProcMap = 0;
	g_nProcSubGame = 0;
	g_byLastSendClock = 255;
	g_dwTime = g_dwServerStartTime = timeGetTime ();

	ZeroMemory (g_dwOnlineBack, sizeof (g_dwOnlineBack));
	ZeroMemory (g_dwOnlineBackMax, sizeof (g_dwOnlineBackMax));
	ZeroMemory (szBuf, sizeof (szBuf));
	GetModuleFileName (NULL, szBuf, MAX_PATH);
	szBuf[strlen (szBuf) - 10] = 0;
	strcpy (szLoginLogFileName, szBuf);
	strcat (szBuf, "\\\\SBODATA");
	CreateDirectory (szBuf, NULL);

	strcat (szLoginLogFileName, "\\SboSvrLoginLog.txt");
	g_LogLogin.Create (szLoginLogFileName, TRUE, TRUE);

	/* iniファイル読み込み */
	ZeroMemory (&szBuf, sizeof (szBuf));
	GetModuleFileName (NULL, szBuf, MAX_PATH);
	szBuf[strlen (szBuf) - 3] = 0;
	strcat (szBuf, "ini");
	/* ポート番号読み込み */
	g_wPort = GetPrivateProfileInt ("Port", "PortNo", 2004, szBuf);
	/* アカウント自動追加モードの読み込み */
	g_bFreeAcount = GetPrivateProfileInt ("Setting", "FreeAcount", TRUE, szBuf);

	/* 管理者アカウント名/パスワード読み込み */
	ZeroMemory (g_szAdminAcc, sizeof (g_szAdminAcc));
	ZeroMemory (g_szAdminPas, sizeof (g_szAdminPas));
	GetPrivateProfileString ("Administrator", "Account", "Administrator", g_szAdminAcc, MAX_ACCNAME, szBuf);
	GetPrivateProfileString ("Administrator", "Password", "", g_szAdminPas, MAX_ACCNAME, szBuf);
	if (g_szAdminPas[0] == 0) {
		/* 空のままだと誰でも管理者になれてしまうので、管理者ログインは受け付けない */
#ifdef __EMSCRIPTEN__
		SvrLogSettingWarning (0);
#endif
	}

	ZeroMemory (g_szCliVer, sizeof (g_szCliVer));
	GetPrivateProfileString ("Version", "Client", VERTEXT, g_szCliVer, sizeof (g_szCliVer), szBuf);

	/* サーバー情報アップロード情報 */
	g_pServerInfoUploadInfo = new SERVERINFOUPLOADINFO;
	ZeroMemory (g_pServerInfoUploadInfo, sizeof (SERVERINFOUPLOADINFO));

	GetPrivateProfileString ("Upload", "ServerName", "",
			g_pServerInfoUploadInfo->szServerName, sizeof (g_pServerInfoUploadInfo->szServerName), szBuf);
	GetPrivateProfileString ("Upload", "Account", "",
			g_pServerInfoUploadInfo->szAccount, sizeof (g_pServerInfoUploadInfo->szAccount), szBuf);
	GetPrivateProfileString ("Upload", "Password", "",
			g_pServerInfoUploadInfo->szPassword, sizeof (g_pServerInfoUploadInfo->szPassword), szBuf);
	GetPrivateProfileString ("Upload", "Directory", "",
			g_pServerInfoUploadInfo->szDirectory, sizeof (g_pServerInfoUploadInfo->szDirectory), szBuf);

	if (strlen (g_pServerInfoUploadInfo->szServerName) == 0) {
		SAFE_DELETE (g_pServerInfoUploadInfo);
	}

	/* 待ち受け開始 */
	svrSock.Host (hWnd, g_wPort, MAX_CONNECTCOUNT + 100);	/* ログイン前の接続や管理者の分も見込んで定員より多めにとる */

	for (i = 0; i < 24; i ++) {
		char szTmp[10];
		wsprintf (szTmp, "%02d", i);
		g_dwOnlineBackMax[i] = (DWORD)GetPrivateProfileInt ("OnlineCount", szTmp, 0, szBuf);
	}

	if (!LoadMapPartsInfo ()) {
		/* 初期マップパーツ作成 */
		g_wCountMapParts = 2;
		g_aMapPartsInfo = new MAPPARTSINFO [g_wCountMapParts];
		ZeroMemory (g_aMapPartsInfo, sizeof (MAPPARTSINFO) * g_wCountMapParts);
		for (i = 0; i < g_wCountMapParts; i ++) {
			(&g_aMapPartsInfo[i])->wGrpID = i;
		}
	}

	if (!LoadMapInfo ()) {
		/* 初期マップ作成 */
		PMAPINFO pMap = new MAPINFO;
		ZeroMemory (pMap, sizeof (MAPINFO));
		pMap->dwID = g_dwNewMapID;
		g_dwNewMapID ++;
		pMap->nCX = 10;
		pMap->nCY = 10;
		pMap->nStartX = 0;
		pMap->nStartY = 0;
		pMap->pwMap = new WORD [pMap->nCX * pMap->nCY];
		PBYTE pMapData = (PBYTE)pMap->pwMap;
		for (i = 0; i < pMap->nCX * pMap->nCY; i ++) {
			pMap->pwMap[i] = 1;
		}
		pMap->pdwMapChar = new DWORD [pMap->nCX * pMap->nCY];
		ZeroMemory ((PBYTE)pMap->pdwMapChar, pMap->nCX * pMap->nCY * sizeof (DWORD));
		g_aMapInfo.Add (pMap);
	}

	LoadJobInfo ();
	LoadEquUserInfo ();
	LoadMagicUserInfo ();
	LoadMagicInfo ();
	LoadItemInfo ();
	LoadCharInfo ();
	LoadSubGameInfo ();
	LoadMakerInfo ();
	LoadRecipeInfo ();

	g_pComEfcInfo = new COMEFCINFO[99];
	ZeroMemory (g_pComEfcInfo, sizeof (COMEFCINFO) * 99);
	if (!LoadComEfcInfo ()) {
		for (i = 0; i < 99; i ++) {
			PCOMEFCINFO pComEfcInfo = &g_pComEfcInfo[i];
			PCOMEFCANIMEINFO pAnime = new COMEFCANIMEINFO[1];
			ZeroMemory (pAnime, sizeof (COMEFCANIMEINFO));
			pAnime->byGrpID = i;
			pAnime->byWait = 30;
			pComEfcInfo->pAnime = pAnime;
			pComEfcInfo->byAnimeCount = 1;
		}
	}

	if (!LoadLeaveInfo ()) {
		PLEAVEINFO pInfo = new LEAVEINFO;
		ZeroMemory (pInfo, sizeof (LEAVEINFO));
		g_aLeaveInfo.Add (pInfo);
	}

	LoadEfcInfo ();
	LoadStTmpInfo ();

	/* 古い Shift_JIS のデータなら、バックアップを取ってから UTF-8 に読み替えて保存し直す
	   （バックアップが取れない時は、データを書き換えずにサーバーを止める） */
	if (!DataUtf8Migrate ()) {
		exit (1);
	}

	PPARTYINFO pPartyInfo;
	pPartyInfo = new PARTYINFO;
	ZeroMemory (pPartyInfo, sizeof (PARTYINFO));
	g_aPartyInfo.Add (pPartyInfo);

	SetTimer (hWnd, TIMERID_REDRAW, TIMER_REDRAW, NULL);
	SetTimer (hWnd, TIMERID_SAVEINFO, TIMER_SAVEINFO, NULL);
	sgenrand (timeGetTime ());

	WebThreadInit ();

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	OnDestroy														 */
/* 内容：	メッセージハンドラ(WM_DESTROY)									 */
/* 日付：	2003/05/17														 */
/* ========================================================================= */

void OnDestroy(HWND hWnd)
{
	char szBuf[16], szName[MAX_PATH + 1];
	int i;
	PCHARINFO pChar;

	/* サーバー落ちる前にキャラデータを編集 */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		pChar = g_aCharInfo[i];
		if (pChar->byNpcType == 0) {
			OnDecPlayer (i);
		}
	}

	g_LogLogin.Destroy ();
	WebThreadDestroy ();
	KillTimer (hWnd, TIMERID_REDRAW);
	KillTimer (hWnd, TIMERID_SAVEINFO);
	ZeroMemory (szName, sizeof (szName));
	GetModuleFileName (NULL, szName, MAX_PATH);
	szName[strlen (szName) - 3] = 0;
	strcat (szName, "ini");

	wsprintf (szBuf, "%d", g_wPort);
	WritePrivateProfileString ("Port", "PortNo", szBuf, szName);
	wsprintf (szBuf, "%d", g_bFreeAcount);
	WritePrivateProfileString ("Setting", "FreeAcount", szBuf, szName);
	/* 昔の「全員管理人権限」（AllAdmin）は廃止したので、設定ファイルに残っていれば消す */
	WritePrivateProfileString ("Setting", "AllAdmin", NULL, szName);
	WritePrivateProfileString ("Administrator", "Account", g_szAdminAcc, szName);
	WritePrivateProfileString ("Administrator", "Password", g_szAdminPas, szName);
	WritePrivateProfileString ("Version", "Client", g_szCliVer, szName);

	for (i = 0; i < 24; i ++) {
		char szTmp[10], szTmp2[10];
		wsprintf (szTmp, "%02d", i);
		wsprintf (szTmp2, "%d", g_dwOnlineBackMax[i]);
		WritePrivateProfileString ("OnlineCount", szTmp, szTmp2, szName);
	}

	SaveCharInfo ();			/* キャラ情報の保存 */
	SaveMapPartsInfo ();		/* パーツ情報の保存 */
	SaveMapInfo ();				/* マップ情報の保存 */
	SaveComEfcInfo ();			/* 吹き出し情報の保存 */
	SaveEfcInfo ();				/* エフェクト情報の保存 */
	SaveItemInfo ();			/* アイテム情報の保存 */
	SaveStTmpInfo ();			/* ステータステンプレート情報の保存 */
	SaveLeaveInfo ();			/* 預かり所情報の保存 */
	SaveSubGameInfo ();			/* サブゲーム情報の保存 */
	SaveJobInfo ();				/* 職業情報の保存 */
	SaveMagicUserInfo ();		/* 魔法利用者情報の保存 */
	SaveMagicInfo ();			/* 魔法情報の保存 */
	SaveEquUserInfo ();			/* 装備職業情報の保存 */
	SaveMakerInfo ();			/* 作り手情報の保存 */
	SaveRecipeInfo ();			/* レシピ情報の保存 */

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		SAFE_DELETE_ARRAY (g_aCharInfo[i]->pszSpeech);
		SAFE_DELETE_ARRAY (g_aCharInfo[i]->pszNpcSpeech);
		delete g_aCharInfo[i];
	}
	g_aCharInfo.RemoveAll ();

	PMAPPARTSINFO pParts = NULL;
	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_aMapPartsInfo[i];
		if (pParts->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pParts->pAnimeInfo);
		}
	}
	SAFE_DELETE_ARRAY (g_aMapPartsInfo);

	for (i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		if (pMap->pShopInfo) {
			for (int j = 0; j < pMap->byCountShop; j ++) {
				PSHOPINFO pShop = &pMap->pShopInfo[j];
				for (int k = 0; k < MAX_SHOPMSG; k ++) {
					SAFE_DELETE_ARRAY (pShop->apMsg[k]);
				}
			}
		}
		SAFE_DELETE_ARRAY (pMap->pszName);
		SAFE_DELETE_ARRAY (pMap->pShopInfo);
		SAFE_DELETE_ARRAY (pMap->pszName);
		SAFE_DELETE_ARRAY (pMap->pwMap);
		SAFE_DELETE_ARRAY (pMap->pdwMapChar);
		SAFE_DELETE_ARRAY (pMap->paTblNoPC);
		SAFE_DELETE_ARRAY (pMap->paTblNoNPC);
		SAFE_DELETE_ARRAY (pMap);
	}

	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pData);
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pMsg);
		SAFE_DELETE (g_aItemInfo[i]);
	}
	g_aItemInfo.RemoveAll ();
	g_aMapInfo.RemoveAll ();

	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aStTmpInfo[i]);
	}
	g_aStTmpInfo.RemoveAll ();

	for (i = 0; i < 99; i ++) {
		PCOMEFCINFO pComefc = &g_pComEfcInfo[i];
		SAFE_DELETE_ARRAY (pComefc->pAnime);
	}
	SAFE_DELETE_ARRAY (g_pComEfcInfo);

	for (i = 0; i < g_aPartyInfo.GetSize (); i ++) {
		if (g_aPartyInfo[i]) {
			SAFE_DELETE (g_aPartyInfo[i]);
		}
	}
	g_aPartyInfo.RemoveAll ();

	for (i = 0; i < g_aMagicInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aMagicInfo[i]);
	}
	g_aMagicInfo.RemoveAll ();

	for (i = 0; i < g_aMagicUserInfo.GetSize (); i ++) {
		PMAGICUSERINFO pInfo;

		pInfo = g_aMagicUserInfo[i];
		if (pInfo) {
			SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
			SAFE_DELETE (pInfo);
		}
	}
	g_aMagicUserInfo.RemoveAll ();

	for (i = 0; i < g_aEquUserInfo.GetSize (); i ++) {
		PEQUUSERINFO pInfo;

		pInfo = g_aEquUserInfo[i];
		if (pInfo) {
			SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
			SAFE_DELETE (pInfo);
		}
	}
	g_aEquUserInfo.RemoveAll ();

	for (i = 0; i < g_aMakerInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aMakerInfo[i]);
	}
	g_aMakerInfo.RemoveAll ();

	for (i = 0; i < g_aRecipeInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aRecipeInfo[i]);
	}
	g_aRecipeInfo.RemoveAll ();

	SAFE_DELETE (g_pServerInfoUploadInfo);

	CoUninitialize ();
	PostQuitMessage (0);
}


/* ========================================================================= */
/* 関数名：	OnDecPlayer														 */
/* 内容：	メッセージハンドラ(WM_DECPLAYER)								 */
/* 日付：	2006/01/04														 */
/* ========================================================================= */

void OnDecPlayer(int nNo)
{
	PCHARINFO pChar;

	if (nNo >= g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[nNo];

	RecvCmdOnDecPlayer (nNo);		/* 取り引きなど受信処理で覚えている状態を消す */

	pChar->dwLastBattle		= 0;
	pChar->dwTargetTblID	= -1;

	DeleteChildChar (pChar);		/* 自分が親のキャラを削除 */
	ProcBreakParty (pChar);			/* パーティから抜ける */
	CharTargetCancelPC (pChar);		/* ついてく解除 */
	DeleteTblNoPC (nNo);			/* PC番号テーブルから削除 */
	DeleteAllTimerEvent (pChar);	/* タイマーイベントを削除 */

	if (pChar->byNpcType == 0) {
		svrSock.DeleteClient (pChar->pID);
	}

	if (pChar->dwParentTblID != -1) {
		g_aCharInfo[pChar->dwParentTblID]->byPutCountNow --;
		g_aCharInfo[pChar->dwParentTblID]->dwLastPut = timeGetTime ();
	}

	if ((pChar->byGameState >= GAMESTATE_MOVE) &&
		(pChar->byGameState != GAMESTATE_NPC)) {
		g_dwOnline --;
		if (pChar->byAdminLV > ADMINLV_USER) {
			if (pChar->byAccState & ACCSTATE_ADMINON) {
				g_byAdminCount --;
			}
		}
		SendCmdONLINE (URARASOCK_SENDALL);
		g_LogLogin.Write ("ログアウト\t[%s][%s] pID:%ld TblID:%ld", pChar->szName, pChar->szAcc, pChar->pID, pChar->dwTblID);
	}

	/* 爆弾ゲームに参加中？ */
	if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
		PSUBSTATUS_BOMB pSubStatus;
		PSUBGAMEINFO pInfo;
		PSUBGAME_BOMBINFO pBomb;

		pSubStatus	= (PSUBSTATUS_BOMB)pChar->abySubStatus;
		pInfo		= g_aSubaGameInfo[pSubStatus->byID];
		pBomb		= (PSUBGAME_BOMBINFO)pInfo->pInfo;
		/* ゲームマスター？ */
		if (pSubStatus->byFlgGameMaster) {
			SubGameProcBombQuitMsg (pSubStatus->byID, "マスターが抜けたので爆弾ゲーム終了");
		}
		pChar->dwX = pBomb->dwX;
		pChar->dwY = pBomb->dwY;

	/* 戦場ゲームに参加中？ */
	} else if (pChar->abySubStatus[0] == SUBGAMEID_WAR) {
		PSUBSTATUS_WAR pSubStatus;
		PCProcSubGameWAR pProcSubGameWAR;

		pSubStatus		= (PSUBSTATUS_WAR)pChar->abySubStatus;
		pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
		if (pProcSubGameWAR) {
			pProcSubGameWAR->RemoveAndMove (pChar->dwTblID);
		}

	/* すごろくに参加中？ */
	} else if (pChar->abySubStatus[0] == SUBGAMEID_SUGOROKU) {
		PSUBGAMEINFO pInfo;
		PSUBSTATUS_SUGOROKU pSubStatus;
		PSUBGAME_SUGOROKUINFO pSugoroku;

		pSubStatus	= (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
		pInfo		= g_aSubaGameInfo[pSubStatus->byID];
		pSugoroku	= (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
		SubSugoroku_RemoveAndMove (pSugoroku, pChar->dwTblID);
	}
	/* サブゲーム情報をクリア */
	ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));

	/* ログオフ処理 */
	pChar->byDirection = 99;
	if (pChar->byGameState != GAMESTATE_LOGOFF) {
		SendCmdCHARPOS (0, nNo, 0, 1);
	}

	pChar->pID				= 0;
	pChar->byGameState		= GAMESTATE_LOGOFF;
	pChar->byCharState		= CHARSTATE_LOGOFF;
	pChar->byDirection		= 1;
	pChar->byDrunkLV		= CHARSTATEDRUNK_NONE;
	pChar->byDrunkPoint		= 0;
	pChar->dwLastSake		= 0;
	pChar->dwLastSakeHikku	= 0;
	pChar->byEfect			= EFECTTYPE_NONE;

	/* プレイ時間の更新 */
	UpdatePlayTime (pChar);
	pChar->dwTimeLogin = 0;
	g_nProcChar = 0;
}


/* ========================================================================= */
/* 人のいないマップ・動かないNPCの処理を減らす（2026/11/08）				 */
/*	キャラは1回に1体ずつ順に処理し、全員回ると1周（人がいる時は1秒に約100周）。	 */
/*	・人のいないマップのNPCは、0.1秒に1周だけ処理する							 */
/*	・その場から動かない村人などは、何も起きていない間は処理しない				 */
/* ========================================================================= */

#define IDLEMAP_INTERVAL	(100)		/* 人のいないマップのNPCを処理する間隔(ミリ秒) */

static std::vector<BYTE>	s_abyMapRun;	/* この周でそのマップのNPCを処理するか */
static std::vector<DWORD>	s_adwMapRunTime;	/* 人のいないマップのNPCを最後に処理した時間 */

/* 周の始めに、マップごとにNPCを処理するか決める */
static void RenewMapRun(void)
{
	int i, j, nCount;

	nCount = g_aMapInfo.GetSize ();
	if ((int)s_abyMapRun.size () != nCount) {
		s_abyMapRun.assign (nCount, 1);
		s_adwMapRunTime.assign (nCount, 0);
	}
	for (i = 0; i < nCount; i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		BOOL bPC = FALSE;

		if (pMap && pMap->paTblNoPC) {
			for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
				if (pMap->paTblNoPC[j] == -1) {
					break;
				}
				if (g_aCharInfo[pMap->paTblNoPC[j]]->byGameState >= GAMESTATE_MOVE) {
					bPC = TRUE;
					break;
				}
			}
		}
		if (bPC || ((DWORD)(g_dwTime - s_adwMapRunTime[i]) >= IDLEMAP_INTERVAL)) {
			s_abyMapRun[i] = 1;
			s_adwMapRunTime[i] = g_dwTime;
		} else {
			s_abyMapRun[i] = 0;
		}
	}
}

/* 何も起きていない、動かないNPCか */
static BOOL IsIdleStaticNPC(PCHARINFO pChar)
{
	PMAPINFO pMap;
	BYTE byHitType;

	if ((pChar->byNpcType != NPCTYPE_NOMOVE) && (pChar->byNpcType != NPCTYPE_NOBLOCK)) {
		return FALSE;
	}
	/* 巡回ルートのある村人は歩く */
	if (pChar->byRouteCount >= 2) {
		return FALSE;
	}
	/* 送ることが残っている・戦闘中・酒・明かり・効果の時間切れ待ち */
	if (pChar->byFlg || pChar->dwLastBattle || pChar->dwLastSake || pChar->dwTimeLight ||
		(pChar->aTimerEvent.GetSize () > 0)) {
		return FALSE;
	}
	/* コンベアなど、勝手に動くマスの上にいる */
	if (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return FALSE;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	if ((pChar->dwX >= (DWORD)pMap->nCX) || (pChar->dwY >= (DWORD)pMap->nCY)) {
		return FALSE;
	}
	WORD wParts = pMap->pwMap[pChar->dwX + pChar->dwY * pMap->nCX];
	if (wParts >= g_wCountMapParts) {
		return FALSE;
	}
	byHitType = g_aMapPartsInfo[wParts].byHitType;
	switch (byHitType) {
	case PARTSHIT_MOVEFRONT:
	case PARTSHIT_MOVEUP:
	case PARTSHIT_MOVEDOWN:
	case PARTSHIT_MOVERIGHT:
	case PARTSHIT_MOVELEFT:
		return FALSE;
	}
	return TRUE;
}

/* この周は処理しなくてよいキャラか */
static BOOL IsSkipChar(PCHARINFO pChar)
{
	if (pChar->byNpcType == NPCTYPE_INIT) {
		return FALSE;
	}
	if ((pChar->dwMapID < (DWORD)s_abyMapRun.size ()) && (s_abyMapRun[pChar->dwMapID] == 0)) {
		return TRUE;
	}
	return IsIdleStaticNPC (pChar);
}


/* ========================================================================= */
/* 関数名：TimerProc														 */
/* 内容：タイマー処理														 */
/* 日付：2003/05/02															 */
/* ========================================================================= */

void TimerProc(void)
{
	BOOL bResult;
	int nCount;
	PCHARINFO pChar;

	g_dwTime = timeGetTime ();

	nCount = g_aCharInfo.GetSize ();
	if (nCount > 0) {
		if (g_nProcChar == 0) {
			RenewMapRun ();
		}
		pChar	= g_aCharInfo[g_nProcChar];
		bResult = FALSE;
		if (IsSkipChar (pChar) == FALSE) {
			bResult = TimerProcChar (pChar);
		}
		nCount	= g_aCharInfo.GetSize ();
		g_nProcChar ++;
		g_nProcChar = (g_nProcChar >= nCount) ? 0 : g_nProcChar;
		if (g_nProcChar == 0) {
			/* ちょっと休憩。 */
			MsgWaitForMultipleObjects (0, NULL, FALSE, 10, QS_ALLINPUT);
		}

	} else {
		/* ちょっと休憩。 */
		MsgWaitForMultipleObjects (0, NULL, FALSE, 1, QS_ALLINPUT);
	}

	/* イベント情報 */
	nCount = g_aMapInfo.GetSize ();
	if (nCount > 0) {
		PMAPINFO pMap = g_aMapInfo[g_nProcMap];

		if (pMap->nEventCount) {
			PEVENTINFO pEvent = &pMap->pEvent[pMap->nProcEvent];
			switch (pEvent->byType) {
			case EVENTTYPE_DOOR:
				{
					PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEvent->pData;
					/* 開いてる？ */
					if (pDoor && pDoor->dwTimeOpen) {
						/* 閉める時間が過ぎた？ */
						if (g_dwTime > pDoor->dwTimeOpen + (pDoor->byTimeClose * 100)) {
							pDoor->dwTimeOpen = 0;
							SendCmdCHGMAP (pMap->dwID, pDoor->wGrpID, pEvent->dwX, pEvent->dwY);
						}
					}
				}
				break;

			case EVENTTYPE_ONCHARMAP:	/* キャラが乗っている間マップ変更 */
				{
					if (g_aCharInfo.GetSize () == 0) {
						break;
					}
					PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)pEvent->pData;
					/* 前回のチェックから500ミリ秒以上経過している？ */
					if (g_dwTime > pInfo->dwCheckTime + 500) {
						pInfo->dwCheckTime = g_dwTime;
						WORD wMap = pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX];
						/* 誰も乗ってない？ */
						if ((CheckCharBlockPC (pMap->dwID, pEvent->dwX, pEvent->dwY) == -1) &&
							(CheckCharBlockNPC (pMap->dwID, pEvent->dwX, pEvent->dwY) == -1)) {
							if (wMap != pInfo->wGrpIDOff) {
								pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX] = pInfo->wGrpIDOff;
								SendCmdCHGMAP (pMap->dwID, pInfo->wGrpIDOff, pInfo->dwX, pInfo->dwY);
							}
						/* 誰か乗っている */
						} else {
							if (wMap != pInfo->wGrpIDOn) {
								pMap->pwMap[pInfo->dwX + pInfo->dwY * pMap->nCX] = pInfo->wGrpIDOn;
								SendCmdCHGMAP (pMap->dwID, pInfo->wGrpIDOn, pInfo->dwX, pInfo->dwY);
							}
						}
					}
				}
				break;
			}

			pMap->nProcEvent ++;
			pMap->nProcEvent = (pMap->nProcEvent >= pMap->nEventCount) ? 0 : pMap->nProcEvent;
		}

		g_nProcMap ++;
		g_nProcMap = (g_nProcMap >= nCount) ? 0 : g_nProcMap;
	}

	/* サブゲーム */
	nCount = g_aSubaGameInfo.GetSize ();
	if (nCount > 0) {
		SubGameProc (g_nProcSubGame);

		g_nProcSubGame ++;
		g_nProcSubGame = (g_nProcSubGame >= nCount) ? 0 : g_nProcSubGame;
	}
	g_ProcSubGame.Proc ();
}


/* ========================================================================= */
/* 関数名：	TimerProcChar													 */
/* 内容：	タイマー処理(キャラ)											 */
/* 日付：	2006/01/04														 */
/* ========================================================================= */

BOOL TimerProcChar(PCHARINFO pChar)
{
	BOOL bRet;
	PMAPINFO pMap;

	bRet = FALSE;
	pMap = g_aMapInfo[pChar->dwMapID];

	if (pChar->byNpcType == 0) {
		if (pChar->byGameState < GAMESTATE_MOVE) {
			goto Exit;
		}
	}
	if (pChar->byGameState != GAMESTATE_NPC) {
		if (pChar->byGameState >= GAMESTATE_MOVE) {
			/* 生存確認チェック */
			if (pChar->dwLastHartBeat && (abs (g_dwTime - pChar->dwLastHartBeat) > 90000)) {
				/* 90秒以上通知がなかったので切断する */
				PostMessage (g_hWndMain, WM_DECPLAYER, (WPARAM)pChar->pID, pChar->dwTblID);
			}
		} else {
			goto Exit;
		}
	}

	/* 向きが変わった？ */
	if (pChar->byFlg & CHARINFOFLG_DIRECTION) {
		if (!(pChar->byFlg & CHARINFOFLG_MOVE)) {
			SendCmdCHGDIRECTION (pChar->pID, pChar->dwTblID);
		}
		pChar->byFlg &= ~CHARINFOFLG_DIRECTION;
	}

	/* 座標が変わった？ */
	if (pChar->byFlg & CHARINFOFLG_MOVE) {
		pChar->byFlg &= ~CHARINFOFLG_MOVE;
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 1);
		CheckCharPosSend (pChar);
	}

	/* 前のキャラが動いた？（今は移動を受け取った時に列ごと動かすので、旗が残っていた時だけ） */
	if (pChar->byFlg & CHARINFOFLG_MOVEPARENT) {
		pChar->byFlg &= ~CHARINFOFLG_MOVEPARENT;
		if ((pChar->dwTailTblID != -1) && (pChar->dwTailTblID != pChar->dwTblID) &&
			(pChar->dwTailTblID < (DWORD)g_aCharInfo.GetSize ())) {
			TailMoveChain (g_aCharInfo[pChar->dwTailTblID]);
		}
	}

	/* 発言した？ */
	if (pChar->byFlg & CHARINFOFLG_SPEECH) {
		pChar->byFlg &= ~CHARINFOFLG_SPEECH;
		SendCmdSPEECH (pChar->pID, pChar->dwTblID, pChar->bySpeechFlg);
	}

	/* 吹き出しが出る？ */
	if (pChar->byFlg & CHARINFOFLG_COMEFC) {
		pChar->byFlg &= ~CHARINFOFLG_COMEFC;
		SendCmdCOMEFC (pChar);
		pChar->byComEfc = 0;
	}

	/* 戦闘行動チェック */
	BattleProcCheck (pChar);

	/* 酒を飲んでいる？ */
	if (pChar->dwLastSake) {
		/* 規定時間以上経っている？ */
		if (g_dwTime >= pChar->dwLastSake + 600000) {
			pChar->dwLastSake = g_dwTime;
			if (pChar->byDrunkLV > CHARSTATEDRUNK_NONE) {
				pChar->byDrunkLV --;
				pChar->byDrunkPoint = pChar->byDrunkLV * 10;
				if (pChar->byDrunkLV == CHARSTATEDRUNK_NONE) {
					pChar->dwLastSake = 0;
					pChar->dwLastSakeHikku = 0;
				}
				char szTmp[128];
				wsprintf (szTmp, "酔いがさめて%s状態になりました。", aDrunkTypeTbl[pChar->byDrunkLV]);
				SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);

				/* クライアントにステータス更新通知 */
				SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
			}
		} else {
			if (g_dwTime >= pChar->dwLastSakeHikku) {
				pChar->dwLastSakeHikku = g_dwTime + 15000 + ((genrand () % 6) * 1000);
				SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 140, 255), "*ﾋｯｸ*");
			}
		}
	}

	/* 明るさ変化の必要がある？ */
	if (pChar->dwTimeLight) {
		BOOL bSend = FALSE;
		/* 時間オーバー？ */
		if (g_dwTime >= pChar->dwTimeLight + (pChar->wLightTime * 1000)) {
			pChar->byLightLevel = 0;
			pChar->byLightLevelBack = 0;
			pChar->wLightTime = 0;
			pChar->dwTimeLight = 0;
			bSend = TRUE;
		}
		if (bSend) {
			SendCmdCHARINFO3 (pChar->dwTblID);
		}
	}

	/* タイマーイベント */
	CharInfoTimerProc (pChar->dwTblID);

	/* 眠っているNPCは動かない */
	if (pChar->pProc && !((pChar->byNpcType != NPCTYPE_INIT) && IsSleep (pChar))) {
		PCCharProcBase pProc;

		pProc = (PCCharProcBase)pChar->pProc;
		pProc->Proc (timeGetTime ());
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：MakeAccList														 */
/* 内容：アカウント一覧作成													 */
/* 日付：2005/03/19															 */
/* ========================================================================= */

void MakeAccList(void)
{
	char szTmp[1024];
	int i, nCount;
	HANDLE hFile;
	DWORD dwBytes;
	PCHARINFO pChar;
	IN_ADDR AddrTmp;

	ZeroMemory (szTmp, sizeof (szTmp));
	GetModuleFileName (NULL, szTmp, MAX_PATH);
	szTmp[strlen (szTmp) - 3] = 0;
	strcat (szTmp, "txt");

	hFile = CreateFile (szTmp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);

	wsprintf (szTmp, "TblID,キャラ名,アカウント名,パスワード,IPアドレス,MACアドレス,作成日,ログイン時間,プレイ時間\r\n");
	WriteFile (hFile, szTmp, strlen (szTmp), &dwBytes, NULL);

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[i];
		if (pChar->byNpcType) {
			continue;
		}
		AddrTmp.S_un.S_addr = pChar->dwLastIPAddress;
		wsprintf (szTmp, "%d,%s,%s,%s,%d.%d.%d.%d,%02X-%02X-%02X-%02X-%02X-%02X,%04d/%02d/%02d,%d,%04d:%02d\r\n",
			pChar->dwTblID,
			pChar->szName,
			pChar->szAcc,
			pChar->szPas,
			AddrTmp.S_un.S_un_b.s_b1, AddrTmp.S_un.S_un_b.s_b2, AddrTmp.S_un.S_un_b.s_b3, AddrTmp.S_un.S_un_b.s_b4,
			(BYTE)pChar->szMacAdr[0], (BYTE)pChar->szMacAdr[1], (BYTE)pChar->szMacAdr[2],
			(BYTE)pChar->szMacAdr[3], (BYTE)pChar->szMacAdr[4], (BYTE)pChar->szMacAdr[5],
			pChar->wMakeYear, pChar->wMakeMonth, pChar->wMakeDay,
			pChar->dwLoginCount,
			pChar->dwPlayTime / 60, pChar->dwPlayTime % 60
			);
		WriteFile (hFile, szTmp, strlen (szTmp), &dwBytes, NULL);
	}

	CloseHandle (hFile);
}


/* ========================================================================= */
/* 関数名：WriteData														 */
/* 内容：データ書き込み														 */
/* 日付：2004/03/21															 */
/* ========================================================================= */

void WriteData(HANDLE hFile, LPSTR pszLavel, DWORD dwSize, PBYTE pData)
{
	DWORD dwBytes;

	WriteFile (hFile, pszLavel, strlen (pszLavel) + 1, &dwBytes, NULL);
	WriteFile (hFile, &dwSize, sizeof (dwSize), &dwBytes, NULL);
	WriteFile (hFile, pData, dwSize, &dwBytes, NULL);
}


/* ========================================================================= */
/* 関数名：SaveMapPartsInfo													 */
/* 内容：マップパーツ情報保存												 */
/* 日付：2003/06/01															 */
/* ========================================================================= */

BOOL SaveMapPartsInfo(void)
{
	int i;
	HANDLE hFile;
	DWORD dwBytes;
	char szName[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}
	strcat (szName, "SBODATA/SboDataMapPartsInfo.dat");

	hFile = CreateFile (szName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);

	PMAPPARTSINFO pParts = NULL;
	PMAPPARTSINFOSAVE pSave = new MAPPARTSINFOSAVE;
	ZeroMemory (pSave, sizeof (MAPPARTSINFOSAVE));

	WriteFile (hFile, &g_wCountMapParts, sizeof (g_wCountMapParts), &dwBytes, NULL);
	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_aMapPartsInfo[i];
		pSave->byViewType = pParts->byViewType;
		pSave->byHitType = pParts->byHitType;
		pSave->byAnimeType = pParts->byAnimeType;
		pSave->byAnimeCount = pParts->byAnimeCount;
		pSave->byLevel = pParts->byLevel;
		pSave->byLight = pParts->byLight;
		pSave->wGrpID = pParts->wGrpID;
		pSave->wBackID = pParts->wBackID;
		pSave->pAnimeInfo = pParts->pAnimeInfo;
		WriteFile (hFile, pSave, sizeof (MAPPARTSINFOSAVE), &dwBytes, NULL);

		if (pSave->pAnimeInfo) {
			WriteFile (hFile, pParts->pAnimeInfo, sizeof (ANIMEINFO) * pSave->byAnimeCount, &dwBytes, NULL);
		}
	}

	delete pSave;

	CloseHandle (hFile);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：LoadMapPartsInfo													 */
/* 内容：マップパーツ情報読み込み											 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
BOOL LoadMapPartsInfo(void)
{
	int i;
	HANDLE hFile;
	DWORD dwBytes;
	char szName[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}
	strcat (szName, "SBODATA/SboDataMapPartsInfo.dat");

	hFile = CreateFile (szName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);

	ReadFile (hFile, &g_wCountMapParts, sizeof (g_wCountMapParts), &dwBytes, NULL);
	SAFE_DELETE_ARRAY (g_aMapPartsInfo);
	g_aMapPartsInfo = new MAPPARTSINFO [g_wCountMapParts];

	PMAPPARTSINFOSAVE pLoad = new MAPPARTSINFOSAVE;
	ZeroMemory (pLoad, sizeof (MAPPARTSINFOSAVE));
	PANIMEINFO pAnime = NULL;
	PMAPPARTSINFO pParts = NULL;

	for (i = 0; i < g_wCountMapParts; i ++) {
		ReadFile (hFile, pLoad, sizeof (MAPPARTSINFOSAVE), &dwBytes, NULL);
		pParts = &g_aMapPartsInfo[i];
		pParts->byViewType = pLoad->byViewType;
		pParts->byHitType = pLoad->byHitType;
		pParts->byAnimeType = pLoad->byAnimeType;
		pParts->byAnimeCount = pLoad->byAnimeCount;
		pParts->byLevel = pLoad->byLevel;
		pParts->byLight = pLoad->byLight;
		pParts->wGrpID = pLoad->wGrpID;
		pParts->wBackID = pLoad->wBackID;
		pParts->pAnimeInfo = pLoad->pAnimeInfo;

		if (pParts->pAnimeInfo) {
#if 1
			pAnime = new ANIMEINFO[pLoad->byAnimeCount];
			ReadFile (hFile, pAnime, sizeof (ANIMEINFO) * pLoad->byAnimeCount, &dwBytes, NULL);
			pParts->pAnimeInfo = pAnime;
#else	/* データ追加時のコンバート用 */
			PANIMEINFO2 pAnime2 = new ANIMEINFO2[pLoad->byAnimeCount];
			pAnime = new ANIMEINFO[pLoad->byAnimeCount];
			ZeroMemory (pAnime, sizeof (ANIMEINFO) * pLoad->byAnimeCount);

			ReadFile (hFile, pAnime2, sizeof (ANIMEINFO2) * pLoad->byAnimeCount, &dwBytes, NULL);

			for (int j = 0; j < pLoad->byAnimeCount; j ++) {
				PANIMEINFO pTmp = &pAnime[j];
				PANIMEINFO2 pTmp2 = &pAnime2[j];
				pTmp->byWait = pTmp2->byWait;
				pTmp->wBackID = pTmp2->wBackID;
				pTmp->wGrpID = pTmp2->wGrpID;
			}
			delete [] pAnime2;
			pParts->pAnimeInfo = pAnime;
#endif
		}
	}

	delete pLoad;

	CloseHandle (hFile);
	return TRUE;
}


/* ========================================================================= */
/* 関数名：SaveMapInfo														 */
/* 内容：マップ情報保存														 */
/* 日付：2003/06/01															 */
/* ========================================================================= */

BOOL SaveMapInfo(void)
{
	int i, j, k;
	HANDLE hFile;
	DWORD dwBytes;
	PMAPINFO pMap;
	PEVENTINFO pEvent;
	char szName[MAX_PATH + 1], szNameTmp[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}

	BYTE byTmp = 0;
	for (i = 0; i < g_aMapInfo.GetSize (); i ++) {
		pMap = g_aMapInfo[i];
		ZeroMemory (szNameTmp, sizeof (szNameTmp));
		wsprintf (szNameTmp, "%sSBODATA/SboDataMapInfo%03d.dat", szName, i);
		hFile = CreateFile (szNameTmp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return FALSE;
		}
		SetFilePointer (hFile, 0, 0, FILE_BEGIN);

		/* 基本情報の保存 */
		WriteData (hFile, "dwID", sizeof (pMap->dwID), (PBYTE)&pMap->dwID);							/* マップID */
		WriteData (hFile, "nCX", sizeof (pMap->nCX), (PBYTE)&pMap->nCX);							/* 横サイズ */
		WriteData (hFile, "nCY", sizeof (pMap->nCY), (PBYTE)&pMap->nCY);							/* 縦サイズ */
		WriteData (hFile, "nStartX", sizeof (pMap->nStartX), (PBYTE)&pMap->nStartX);				/* 初期位置(X) */
		WriteData (hFile, "nStartY", sizeof (pMap->nStartY), (PBYTE)&pMap->nStartY);				/* 初期位置(Y) */
		WriteData (hFile, "nEntranceX", sizeof (pMap->nEntranceX), (PBYTE)&pMap->nEntranceX);				/* 初期位置(X) */
		WriteData (hFile, "nEntranceY", sizeof (pMap->nEntranceY), (PBYTE)&pMap->nEntranceY);				/* 初期位置(Y) */
		WriteData (hFile, "byBGM", sizeof (pMap->byBGM), (PBYTE)&pMap->byBGM);						/* 音楽ID */
		WriteData (hFile, "byFlg", sizeof (pMap->byFlg), (PBYTE)&pMap->byFlg);						/* 各フラグ */
		WriteData (hFile, "byLightLevel", sizeof (pMap->byLightLevel), (PBYTE)&pMap->byLightLevel);						/* 各フラグ */
		WriteData (hFile, "byWeather", sizeof (pMap->byWeather), (PBYTE)&pMap->byWeather);						/* 天候 */
		/* 暗くする範囲（無いマップは書かない。古いサーバーでも読めるように） */
		if (pMap->byDarkAreaCount) {
			WriteData (hFile, "aDarkArea", sizeof (MAPDARKAREA) * pMap->byDarkAreaCount, (PBYTE)pMap->aDarkArea);
		}
		if (pMap->pszName) {
			WriteData (hFile, "pszName", strlen ((LPSTR)pMap->pszName) + 1, (PBYTE)pMap->pszName);	/* マップ名 */
		}
		if (pMap->pwMap) {
			WriteData (hFile, "pwMap", pMap->nCX * pMap->nCY * sizeof (WORD), (PBYTE)pMap->pwMap);	/* マップパーツ */
		}
		WriteData (hFile, "nEventCount", sizeof (pMap->nEventCount), (PBYTE)&pMap->nEventCount);	/* 登録イベント数 */
		/* イベント情報 */
		if (pMap->nEventCount) {
			for (j = 0; j < pMap->nEventCount; j ++) {
				pEvent = &pMap->pEvent[j];

				WriteData (hFile, "dwX", sizeof (pEvent->dwX), (PBYTE)&pEvent->dwX);				/* X座標 */
				WriteData (hFile, "dwY", sizeof (pEvent->dwY), (PBYTE)&pEvent->dwY);				/* Y座標 */
				WriteData (hFile, "byType", sizeof (pEvent->byType), (PBYTE)&pEvent->byType);		/* 種別 */
				WriteData (hFile, "pData", sizeof (pEvent->pData), (PBYTE)&pEvent->pData);			/* データ */
				if (pEvent->pData) {
					switch (pEvent->byType) {
					case EVENTTYPE_MOVE:			/* 座標変更 */
						{
							PEVENTINFO_MOVE pInfo = (PEVENTINFO_MOVE)pEvent->pData;
							WriteData (hFile, "dwX", sizeof (pInfo->dwX), (PBYTE)&pInfo->dwX);				/* 移動先X座標 */
							WriteData (hFile, "dwY", sizeof (pInfo->dwY), (PBYTE)&pInfo->dwY);				/* 移動先Y座標 */
							WriteData (hFile, "byEfect", sizeof (pInfo->byEfect), (PBYTE)&pInfo->byEfect);	/* エフェクト種類 */
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					case EVENTTYPE_MAPCHG:			/* マップ間移動 */
						{
							PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
							WriteData (hFile, "dwX", sizeof (pInfo->dwX), (PBYTE)&pInfo->dwX);				/* 移動先X座標 */
							WriteData (hFile, "dwY", sizeof (pInfo->dwY), (PBYTE)&pInfo->dwY);				/* 移動先Y座標 */
							WriteData (hFile, "dwMapID", sizeof (pInfo->dwMapID), (PBYTE)&pInfo->dwMapID);	/* マップID */
							WriteData (hFile, "byEfect", sizeof (pInfo->byEfect), (PBYTE)&pInfo->byEfect);	/* エフェクト種類 */
							if (pInfo->byRandomRange) {
								/* ばらけない時は書かない（古いサーバーでも読めるように） */
								WriteData (hFile, "byRandomRange", sizeof (pInfo->byRandomRange), (PBYTE)&pInfo->byRandomRange);	/* ばらける範囲 */
							}
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					case EVENTTYPE_MSG:				/* メッセージ */
						{
							PEVENTINFO_MSG pInfo = (PEVENTINFO_MSG)pEvent->pData;
							WriteData (hFile, "dwNo", sizeof (pInfo->dwNo), (PBYTE)&pInfo->dwNo);			/* メッセージID */
							WriteData (hFile, "byType", sizeof (pInfo->byType), (PBYTE)&pInfo->byType);		/* 種別 */
							WriteData (hFile, "wGrpID", sizeof (pInfo->wGrpID), (PBYTE)&pInfo->wGrpID);		/* 書き換えるグラフィックID */
							WriteData (hFile, "dwX", sizeof (pInfo->dwX), (PBYTE)&pInfo->dwX);				/* X座標 */
							WriteData (hFile, "dwY", sizeof (pInfo->dwY), (PBYTE)&pInfo->dwY);				/* Y座標 */
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					case EVENTTYPE_DOOR:			/* 扉 */
						{
							PEVENTINFO_DOOR pInfo = (PEVENTINFO_DOOR)pEvent->pData;
							WriteData (hFile, "wGrpID", sizeof (pInfo->wGrpID), (PBYTE)&pInfo->wGrpID);					/* 扉のグラフィックID */
							WriteData (hFile, "wBackGrpID", sizeof (pInfo->wBackGrpID), (PBYTE)&pInfo->wBackGrpID);		/* 背景のグラフィックID */
							WriteData (hFile, "byTimeClose", sizeof (pInfo->byTimeClose), (PBYTE)&pInfo->byTimeClose);	/* 閉まるまでの時間 */
							WriteData (hFile, "byType", sizeof (pInfo->byType), (PBYTE)&pInfo->byType);					/* 種別 */
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					case EVENTTYPE_HPMANA:			/* HP/MANA回復 */
						{
							PEVENTINFO_HPMANA pInfo = (PEVENTINFO_HPMANA)pEvent->pData;
							WriteData (hFile, "byType", sizeof (pInfo->byType), (PBYTE)&pInfo->byType);					/* 種別 */
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					case EVENTTYPE_STATUSINIT:		/* ステータス初期化 */
						WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						break;
					case EVENTTYPE_ONCHARMAP:		/* キャラが乗っている間マップ変更 */
						{
							PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)pEvent->pData;
							WriteData (hFile, "wGrpIDOn", sizeof (pInfo->wGrpIDOn), (PBYTE)&pInfo->wGrpIDOn);		/* グラフィックID（キャラが乗っている） */
							WriteData (hFile, "wGrpIDOff", sizeof (pInfo->wGrpIDOff), (PBYTE)&pInfo->wGrpIDOff);	/* グラフィックID（キャラが乗っていない） */
							WriteData (hFile, "dwX", sizeof (pInfo->dwX), (PBYTE)&pInfo->dwX);						/* X座標 */
							WriteData (hFile, "dwY", sizeof (pInfo->dwY), (PBYTE)&pInfo->dwY);						/* Y座標 */
							WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
						}
						break;
					}
				}
				WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
			}
		}
		WriteData (hFile, "nMsgCount", sizeof (pMap->nMsgCount), (PBYTE)&pMap->nMsgCount);			/* 登録メッセージ数 */
		if (pMap->nMsgCount) {
			for (j = 0; j < pMap->nMsgCount; j ++) {
				WriteData (hFile, "Msg", strlen ((LPSTR)pMap->pMsg[j]) + 1, (PBYTE)pMap->pMsg[j]);	/* メッセージ */
			}
			WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);
		}
		WriteData (hFile, "clChg", sizeof (pMap->clChg), (PBYTE)&pMap->clChg);						/* 色変換 */

		WriteData (hFile, "byCountShop", sizeof (pMap->byCountShop), (PBYTE)&pMap->byCountShop);	/* 店の数 */
		/* 店がある？ */
		if (pMap->byCountShop) {
			for (k = 0; k < pMap->byCountShop; k ++) {
				PSHOPINFO pShop = &pMap->pShopInfo[k];
				/* 売り物無し？ */
				if (pShop->byCountItem == 0) {
					WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
					continue;
				}
				WriteData (hFile, "byCountItem", sizeof (pShop->byCountItem), (PBYTE)&pShop->byCountItem);	/* 売り物数 */
				WriteData (hFile, "awItem", sizeof (pShop->awItem), (PBYTE)&pShop->awItem);					/* 売り物 */

				for (j = 0; j < MAX_SHOPMSG; j ++) {
					WriteData (hFile, "apMsg", strlen (pShop->apMsg[j]) + 1, (PBYTE)pShop->apMsg[j]);		/* メッセージ */
				}
				WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
			}
			WriteFile (hFile, &byTmp, 1, &dwBytes, NULL);	/* 区切り */
		}

		CloseHandle (hFile);
	}

	return TRUE;
}


/* ========================================================================= */
/* 関数名：LoadMapInfo														 */
/* 内容：マップ情報読み込み													 */
/* 日付：2003/06/01															 */
/* ========================================================================= */

BOOL LoadMapInfo(void)
{
	int i, j, k;
	HANDLE hFile;
	DWORD dwBytes;
	char szName[MAX_PATH + 1], szNameTmp[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));
	BOOL bRet;

	bRet = FALSE;
	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}


	for (i = 0; ; i ++) {
		ZeroMemory (szNameTmp, sizeof (szNameTmp));
		wsprintf (szNameTmp, "%sSBODATA/SboDataMapInfo%03d.dat", szName, i);
		hFile = CreateFile (szNameTmp, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			break;
		}
		SetFilePointer (hFile, 0, 0, FILE_BEGIN);

		/* ファイルサイズ取得 */
		DWORD dwFileSize = GetFileSize (hFile, NULL);
		if ((int)dwFileSize == -1) {
			CloseHandle (hFile);
			return FALSE;
		}

		PBYTE pData = new BYTE[dwFileSize],
			  pDataTmp = pData;
		/* ファイル内容を全部読み込む */
		ReadFile (hFile, pData, dwFileSize, &dwBytes, NULL);
		CloseHandle (hFile);
		bRet = TRUE;
	
		DWORD dwFilePtr = 0, dwSize;
		while (1) {
			if (dwFilePtr >= dwFileSize) {
				break;
			}
			PMAPINFO pMap = new MAPINFO;
			ZeroMemory (pMap, sizeof (MAPINFO));

			/* 基本情報の読み込み */
			while (1) {
				if (pData[dwFilePtr] == 0) {
					dwFilePtr ++;
					break;
				}
				LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
				dwFilePtr += (strlen (pszTmp) + 1);
				CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
				dwFilePtr += sizeof (DWORD);
				if (strcmp (pszTmp, "dwID") == 0) {							/* マップID */
					CopyMemory (&pMap->dwID, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nCX") == 0) {					/* 横サイズ */
					CopyMemory (&pMap->nCX, &pData[dwFilePtr], dwSize);		dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nCY") == 0) {					/* 縦サイズ */
					CopyMemory (&pMap->nCY, &pData[dwFilePtr], dwSize);		dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nStartX") == 0) {				/* 初期位置(X) */
					CopyMemory (&pMap->nStartX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nStartY") == 0) {				/* 初期位置(Y) */
					CopyMemory (&pMap->nStartY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nEntranceX") == 0) {			/* 入り口(X) */
					CopyMemory (&pMap->nEntranceX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "nEntranceY") == 0) {			/* 入り口(Y) */
					CopyMemory (&pMap->nEntranceY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "byBGM") == 0) {					/* 音楽ID */
					CopyMemory (&pMap->byBGM, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "byFlg") == 0) {					/* 各フラグ */
					CopyMemory (&pMap->byFlg, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "byLightLevel") == 0) {			/* 明るさレベル */
					CopyMemory (&pMap->byLightLevel, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "byWeather") == 0) {				/* 天候 */
					CopyMemory (&pMap->byWeather, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "aDarkArea") == 0) {				/* 暗くする範囲 */
					pMap->byDarkAreaCount = (BYTE)min (dwSize / sizeof (MAPDARKAREA), (DWORD)MAX_MAPDARKAREA);
					CopyMemory (pMap->aDarkArea, &pData[dwFilePtr], sizeof (MAPDARKAREA) * pMap->byDarkAreaCount);
					dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "pszName") == 0) {				/* マップ名 */
					pszTmp = (LPCTSTR)&pData[dwFilePtr];
					pMap->pszName = new char[strlen (pszTmp) + 1];
					strcpy (pMap->pszName, pszTmp);
					dwFilePtr += dwSize;
					continue;
				} else if (strcmp (pszTmp, "pwMap") == 0) {					/* マップパーツ */
					pMap->pwMap = new WORD[pMap->nCX * pMap->nCY * sizeof (WORD)];
					CopyMemory (pMap->pwMap, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
#if 0
					int tmpx, tmpy;
					for (tmpy = 0; tmpy < pMap->nCY; tmpy ++) {
						for (tmpx = 0; tmpx < pMap->nCX; tmpx ++) {
							if (pMap->pwMap[tmpx + tmpy * pMap->nCX] == 1233) {
								pMap->pwMap[tmpx + tmpy * pMap->nCX] = 1291;
							} else if (pMap->pwMap[tmpx + tmpy * pMap->nCX] == 1202) {
								pMap->pwMap[tmpx + tmpy * pMap->nCX] = 1295;
							}
						}
					}
					continue;
#endif
				} else if (strcmp (pszTmp, "nEventCount") == 0) {			/* 登録イベント数 */
					CopyMemory (&pMap->nEventCount, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;
					/* イベントあり？ */
					if (pMap->nEventCount) {
						pMap->pEvent = new EVENTINFO[pMap->nEventCount];
						ZeroMemory (pMap->pEvent, sizeof (EVENTINFO) * pMap->nEventCount);
						PEVENTINFO pEvent = &pMap->pEvent[0];
						j = 0;
						while (1) {
							if (pData[dwFilePtr] == 0) {
								dwFilePtr ++;
								j ++;
								if (j >= pMap->nEventCount) {
									break;
								}
								pEvent = &pMap->pEvent[j];
								continue;
							}
							LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
							dwFilePtr += (strlen (pszTmp) + 1);
							CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
							dwFilePtr += sizeof (DWORD);
							if (strcmp (pszTmp, "dwX") == 0) {							/* X座標 */
								CopyMemory (&pEvent->dwX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
							} else if (strcmp (pszTmp, "dwY") == 0) {					/* Y座標 */
								CopyMemory (&pEvent->dwY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
							} else if (strcmp (pszTmp, "byType") == 0) {				/* 種別 */
								CopyMemory (&pEvent->byType, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
							} else if (strcmp (pszTmp, "pData") == 0) {					/* データ */
								CopyMemory (&pEvent->pData, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;
								/* データあり？ */
								if (pEvent->pData) {
									switch (pEvent->byType) {
									case EVENTTYPE_MOVE:			/* 座標変更 */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_MOVE)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_MOVE));
											PEVENTINFO_MOVE pInfo = (PEVENTINFO_MOVE)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "dwX") == 0) {							/* 移動先X座標 */
													CopyMemory (&pInfo->dwX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwY") == 0) {					/* 移動先Y座標 */
													CopyMemory (&pInfo->dwY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byEfect") == 0) {				/* エフェクト種類 */
													CopyMemory (&pInfo->byEfect, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									case EVENTTYPE_MAPCHG:			/* マップ間移動 */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_MAPCHG)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_MAPCHG));
											PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "dwX") == 0) {							/* 移動先X座標 */
													CopyMemory (&pInfo->dwX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwY") == 0) {					/* 移動先Y座標 */
													CopyMemory (&pInfo->dwY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwMapID") == 0) {				/* マップID */
													CopyMemory (&pInfo->dwMapID, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byEfect") == 0) {				/* エフェクト種類 */
													CopyMemory (&pInfo->byEfect, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byRandomRange") == 0) {		/* ばらける範囲 */
													CopyMemory (&pInfo->byRandomRange, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									case EVENTTYPE_MSG:				/* メッセージ */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_MSG)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_MSG));
											PEVENTINFO_MSG pInfo = (PEVENTINFO_MSG)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "dwNo") == 0) {							/* メッセージID */
													CopyMemory (&pInfo->dwNo, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byType") == 0) {				/* 種別 */
													CopyMemory (&pInfo->byType, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "wGrpID") == 0) {				/* 書き換えるグラフィックID */
													CopyMemory (&pInfo->wGrpID, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwX") == 0) {					/* X座標 */
													CopyMemory (&pInfo->dwX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwY") == 0) {					/* Y座標 */
													CopyMemory (&pInfo->dwY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									case EVENTTYPE_DOOR:			/* 扉 */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_DOOR)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_DOOR));
											PEVENTINFO_DOOR pInfo = (PEVENTINFO_DOOR)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "wGrpID") == 0) {						/* 扉のグラフィックID */
													CopyMemory (&pInfo->wGrpID, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "wBackGrpID") == 0) {			/* 背景のグラフィックID */
													CopyMemory (&pInfo->wBackGrpID, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byTimeClose") == 0) {			/* 閉まるまでの時間 */
													CopyMemory (&pInfo->byTimeClose, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "byType") == 0) {				/* 種別 */
													CopyMemory (&pInfo->byType, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									case EVENTTYPE_HPMANA:			/* HP/MANA回復 */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_HPMANA)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_HPMANA));
											PEVENTINFO_HPMANA pInfo = (PEVENTINFO_HPMANA)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "byType") == 0) {						/* 種別 */
													CopyMemory (&pInfo->byType, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									case EVENTTYPE_STATUSINIT:		/* ステータス初期化 */
										dwFilePtr ++;
										break;
									case EVENTTYPE_ONCHARMAP:		/* キャラが乗っている間マップ変更 */
										{
											pEvent->pData = new BYTE[sizeof (EVENTINFO_ONCHARMAP)];
											ZeroMemory (pEvent->pData, sizeof (EVENTINFO_ONCHARMAP));
											PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)pEvent->pData;
											while (1) {
												if (pData[dwFilePtr] == 0) {
													dwFilePtr ++;
													break;
												}
												LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
												dwFilePtr += (strlen (pszTmp) + 1);
												CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
												dwFilePtr += sizeof (DWORD);
												if (strcmp (pszTmp, "wGrpIDOn") == 0) {						/* グラフィックID（キャラが乗っている） */
													CopyMemory (&pInfo->wGrpIDOn, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "wGrpIDOff") == 0) {				/* グラフィックID（キャラが乗っていない） */
													CopyMemory (&pInfo->wGrpIDOff, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwX") == 0) {					/* X座標 */
													CopyMemory (&pInfo->dwX, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												} else if (strcmp (pszTmp, "dwY") == 0) {					/* Y座標 */
													CopyMemory (&pInfo->dwY, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
												}
											}
										}
										break;
									}
								}
							}
						}
					}
				} else if (strcmp (pszTmp, "nMsgCount") == 0) {				/* 登録メッセージ数 */
					CopyMemory (&pMap->nMsgCount, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;
					/* メッセージあり？ */
					if (pMap->nMsgCount) {
						pMap->pMsg = new LPSTR[pMap->nMsgCount];
						for (j = 0; j < pMap->nMsgCount; j ++) {
							LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
							dwFilePtr += (strlen (pszTmp) + 1);
							CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
							dwFilePtr += sizeof (DWORD);
							if (strcmp (pszTmp, "Msg") == 0) {
								pMap->pMsg[j] = new char[dwSize];
								strcpy (pMap->pMsg[j], (LPCTSTR)&pData[dwFilePtr]);
								dwFilePtr += dwSize;
							}
						}
						dwFilePtr ++;
					}
				} else if (strcmp (pszTmp, "clChg") == 0) {					/* 色変換 */
					CopyMemory (&pMap->clChg, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
				} else if (strcmp (pszTmp, "byCountShop") == 0) {			/* 店の数 */
					CopyMemory (&pMap->byCountShop, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;
					/* 店がある？ */
					if (pMap->byCountShop) {
						pMap->pShopInfo = new SHOPINFO[pMap->byCountShop];
						int nMsgCount = 0;
						k = 0;
						PSHOPINFO pShop = &pMap->pShopInfo[0];
						ZeroMemory (pShop, sizeof (SHOPINFO));
						while (1) {
							if (pData[dwFilePtr] == 0) {
								dwFilePtr ++;
								k ++;
								nMsgCount = 0;
								if (k >= pMap->byCountShop) {
									break;
								}
								pShop = &pMap->pShopInfo[k];
								ZeroMemory (pShop, sizeof (SHOPINFO));
								continue;
							}
							LPCTSTR pszTmp = (LPCTSTR)&pData[dwFilePtr];						/* ラベル */
							dwFilePtr += (strlen (pszTmp) + 1);
							CopyMemory (&dwSize, &pData[dwFilePtr], sizeof (DWORD));	/* データサイズ */
							dwFilePtr += sizeof (DWORD);
							if (strcmp (pszTmp, "byCountItem") == 0) {				/* 売り物数 */
								CopyMemory (&pShop->byCountItem, &pData[dwFilePtr], dwSize);	dwFilePtr += dwSize;	continue;
							} else if (strcmp (pszTmp, "awItem") == 0) {			/* 売り物 */
								CopyMemory (&pShop->awItem, &pData[dwFilePtr], dwSize);			dwFilePtr += dwSize;	continue;
							} else if (strcmp (pszTmp, "apMsg") == 0) {				/* メッセージ */
								pShop->apMsg[nMsgCount] = new char[strlen ((LPCTSTR)&pData[dwFilePtr]) + 1];
								strcpy (pShop->apMsg[nMsgCount], (LPCTSTR)&pData[dwFilePtr]);
								nMsgCount ++;
								dwFilePtr += dwSize;	continue;
							}
						}
					}
				}
			}
			pMap->pdwMapChar = new DWORD [pMap->nCX * pMap->nCY];
			ZeroMemory ((BYTE *)pMap->pdwMapChar, pMap->nCX * pMap->nCY * sizeof (DWORD));
			g_aMapInfo.Add (pMap);
		}
	}
	g_dwNewMapID = i;

	return bRet;
}


/* ========================================================================= */
/* 関数名：SaveLeaveInfo													 */
/* 内容：預かり所情報保存													 */
/* 日付：2003/11/06															 */
/* ========================================================================= */

BOOL SaveLeaveInfo(void)
{
	int i;
	HANDLE hFile;
	DWORD dwBytes;
	char szName[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));

	if (g_aLeaveInfo.GetSize () == 0) {
		return FALSE;
	}

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}
	strcat (szName, "SBODATA/SboDataLeaveInfo.dat");

	hFile = CreateFile (szName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	SetFilePointer (hFile, 0, 0, FILE_BEGIN);

	for (i = 0; i < g_aLeaveInfo.GetSize (); i ++) {
		PLEAVEINFO pTmp = g_aLeaveInfo[i];

		WriteFile (hFile, pTmp, sizeof (LEAVEINFO), &dwBytes, NULL);
	}
	CloseHandle (hFile);

	return TRUE;
}


/* ========================================================================= */
/* 関数名：LoadLeaveInfo													 */
/* 内容：預かり所情報保存													 */
/* 日付：2003/11/06															 */
/* ========================================================================= */

BOOL LoadLeaveInfo(void)
{
	int i;
	HANDLE hFile;
	DWORD dwBytes;
	char szName[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szName, MAX_PATH);
	for (i = strlen (szName) - 1; i > 0; i --) {
		if (szName[i] == '\\') {
			szName[i + 1] = 0;
			break;
		}
	}
	strcat (szName, "SBODATA/SboDataLeaveInfo.dat");

	hFile = CreateFile (szName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);
	
	for (i = 0; i < g_aLeaveInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aLeaveInfo[i]);
	}
	g_aLeaveInfo.RemoveAll ();
	for (i = 0; ; i ++) {
#if 1
		PLEAVEINFO pTmp = new LEAVEINFO;
		ZeroMemory (pTmp, sizeof (LEAVEINFO));
		ReadFile (hFile, pTmp, sizeof (LEAVEINFO), &dwBytes, NULL);
		if (dwBytes == 0) {
			SAFE_DELETE (pTmp);
			break;
		}
#else
		PLEAVEINFO2 pTmp2 = new LEAVEINFO2;
		ZeroMemory (pTmp2, sizeof (LEAVEINFO2));

		ReadFile (hFile, pTmp2, sizeof (LEAVEINFO2), &dwBytes, NULL);
		if (dwBytes == 0) {
			SAFE_DELETE (pTmp2);
			break;
		}
		PLEAVEINFO pTmp = new LEAVEINFO;
		ZeroMemory (pTmp, sizeof (LEAVEINFO));
		pTmp->byCount = pTmp2->byCount;
		pTmp->dwGold = pTmp2->dwGold;
		CopyMemory (pTmp->waItem, pTmp2->waItem, sizeof (pTmp->waItem));
		for (int j = 0; j < MAX_LEAVEITEM; j ++) {
			if (pTmp->waItem[j]) {
				pTmp->byItemCount[j] = 1;
			}
		}
		SAFE_DELETE (pTmp2);
#endif
		g_aLeaveInfo.Add (pTmp);
	}
	CloseHandle (hFile);

	if (g_aLeaveInfo.GetSize () == 0) {
		return FALSE;
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2006 */
