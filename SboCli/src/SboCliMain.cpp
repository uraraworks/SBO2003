/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboCliMain.cpp												 */
/* 概要：		スクラップブックオンライン（クライアント）					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#include "stdafx.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "../Common/Info/InfoJob.h"
#include "Lib/png.h"
#include "Lib/MacAddr.h"
#include "Glbldata.h"
#include "SboCliMakeWindow.h"
#include "SboCliDrawWindow.h"
#include "SboCliDrawScreen.h"
#include "SboCliKeyProc.h"
#include "SboCliRecvCmd.h"
#include "SboCliSendCmd.h"
#include "SboAdminWindow.h"
#include "SboCliMsgLogWindow.h"
#include "SboCliAdminWindow.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define CLNAME "SBOCLI"											/* 登録クラス名 */


/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

typedef struct{
    PBYTE	Buffer;
    int		Pos;
} TPngFileBuffer;


/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

BOOL g_bLeftClick	= FALSE,						/* 左クリック中？ */
	 g_bDrawStop	= TRUE;							/* 描画停止 */
HANDLE g_mutexMain	= NULL;							/* 二重起動防止用ミューテックス */
HINSTANCE m_hRtLib	= NULL;							/* リッチテキストエディットＤＬＬハンドル */
HMODULE g_hModule	= NULL;							/* KERNEL32.DLLのモジュールハンドル */
FARPROC g_proc		= NULL;							/* IsDebuggerPresent関数 */


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);						/* メインウィンドウメッセージ処理 */
BOOL	OnCreate					(HWND);								/* メッセージハンドラ(WM_CREATE) */
void	OnInitEnd					(HWND hWnd);						/* メッセージハンドラ(WM_INITEND) */
void	OnPaint						(HWND hWnd);						/* メッセージハンドラ(WM_PAINT) */
void	OnCommand					(HWND hWnd, int nID);				/* メッセージハンドラ(WM_COMMAND) */
void	OnLbuttonDown				(HWND hWnd, WPARAM wParam, LPARAM lParam);	/* メッセージハンドラ(WM_LBUTTONDOWN) */
void	ChgState					(int nState);						/* 状態変更 */
BOOL	ReadData					();									/* 各データの読み込み */
void	ReadGrpData					(LPSTR, CImg32**, int);				/* グラフィックデータの読み込み */
void	ReadGrpData2				(LPSTR, CImg32**, int);				/* グラフィックデータの読み込み */
void	WriteGrpData				(LPSTR, CImg32 *);					/* 画像をファイルに出力 */
void	WriteGrpData2				(LPSTR, CImg32 *);					/* 画像をファイルに出力 */
void	DeleteChar					(int);								/* キャラ削除 */
void	ReleaseData					(void);								/* 各データの開放 */
void	TimerProc					(void);								/* タイマー処理 */
void	TimerProcObj				(void);								/* タイマー処理(オブジェクト) */
void	TimerProcChar				(PCHARINFO pChar);					/* タイマー処理(キャラ) */
void	TimerProcCharDotMove		(PCHARINFO pChar);					/* タイマー処理(キャラ移動処理) */
void	TimerProcCharEfect			(PCHARINFO pChar);					/* タイマー処理(キャラエフェクト処理) */
void	TimerProcCharEfectIdle		(PCHARINFO pChar);					/* タイマー処理(キャラエフェクト処理(アイドル中)) */
void	TimerProcCharComEfc			(PCHARINFO pChar);					/* タイマー処理(キャラ吹き出し処理) */
void	TimerProcCharAnime			(PCHARINFO pChar);					/* タイマー処理(キャラアニメーション処理) */
void	TimerProcCharAnimeBOMB		(PCHARINFO pChar);					/* タイマー処理(キャラアニメーション処理(爆弾)) */
void	TimerProcCharAnimeBOMBBLOCK	(PCHARINFO pChar);					/* タイマー処理(キャラアニメーション処理(爆弾ゲームの壁)) */
void	TimerProcCharHartBeat		(PCHARINFO pChar);					/* タイマー処理(キャラハートビート処理) */
void	TimerProcCharTurnLV			(PCHARINFO pChar);					/* タイマー処理(キャラターンゲージ処理) */
static void	TimerProcAutoTurnGage	(PCHARINFO pChar, DWORD dwTime);	/* 目の前の敵で自動でゲージを溜める */
void	TimerProcCharMagicLV		(PCHARINFO pChar);					/* タイマー処理(キャラ魔法ターンゲージ処理) */
BOOL	MsgProc						(PWNDINFO);							/* メッセージ処理 */
void	StartPosConfirmOpen			(PWNDINFO, int, int, int, int);		/* 復活位置の確認を出す */
int		GetWindowID					(DWORD);							/* ウィンドウ番号を取得 */
int		GetCharID					(DWORD);							/* キャラ番号を取得 */
void	StopBGM						();									/* BGM停止 */
void	PlayBGM						(DWORD, BOOL bPlay = FALSE);		/* BGM演奏 */
BOOL	SaveAdminRura				(void);								/* 管理人ルーラ情報保存 */
void	SaveIniOption				(void);								/* オプション設定をiniへ保存 */
BOOL	LoadAdminRura				(void);								/* 管理人ルーラ情報読み込み */
static void	MainLoopInit			(void);								/* メインループの初期化 */
static int	MainLoopStep			(void);								/* メインループ１回分の処理 */
#ifdef __EMSCRIPTEN__
static void	MainLoopWeb				(void);								/* ブラウザ版のメインループ */
#endif

/* MainLoopStep の戻り値 */
enum {
	MAINLOOP_QUIT = 0,							/* WM_QUIT を受け取った */
	MAINLOOP_MESSAGE,							/* メッセージを１つ処理した */
	MAINLOOP_IDLE,								/* メッセージが無かったのでタイマー処理をした */
};


/* ========================================================================= */
/* 関数名：	MainLoopInit													 */
/* 内容：	メインループの初期化											 */
/* ========================================================================= */

static DWORD s_dwTimeStart, s_dwTimeFps, s_dwTimeCount, s_dwFpsCount;
static BYTE s_byFps, s_byFrameBack;

static void MainLoopInit(void)
{
	g_byFps = 0;
	s_byFps = 0;
	s_byFrameBack = 0;
	s_dwFpsCount = 0;
	g_dwFpsCount = 0;
	s_dwTimeStart = s_dwTimeFps = s_dwTimeCount = timeGetTime ();
}


/* ========================================================================= */
/* 関数名：	MainLoopStep													 */
/* 内容：	メインループ１回分の処理										 */
/*			メッセージがあれば１つ処理し、無ければタイマー処理と描画判定を行う	 */
/* 戻り値：	MAINLOOP_QUIT / MAINLOOP_MESSAGE / MAINLOOP_IDLE				 */
/* ========================================================================= */

static int MainLoopStep(void)
{
	MSG msg;
	DWORD dwTmp, dwTimeTmp;
	BYTE byNowFrame;

	/* メッセージがきてるなら処理する */
	if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return MAINLOOP_QUIT;
		}
		TranslateMessage (&msg);
		DispatchMessage (&msg);
		return MAINLOOP_MESSAGE;
	}

	TimerProc ();
	dwTimeTmp = timeGetTime ();
	s_dwFpsCount ++;
	dwTmp = 33;

	/* 時間的に描画するだろうフレーム番号を求める */
	byNowFrame = (BYTE)((dwTimeTmp - s_dwTimeStart) / dwTmp);
	if ((s_byFps == 0) || s_byFrameBack != byNowFrame) {
		if (s_byFps && (abs (s_byFrameBack - byNowFrame) != 1)) {
			g_bDraw = FALSE;
		}
		if (s_byFps >= 30) {
			g_bDraw = FALSE;
		}

		s_byFrameBack = byNowFrame;
		if (g_bWindowActive && byNowFrame % 2) {
			/* ウィンドウがアクティブの時だけキー処理をする */
			if (g_FadeInOutInfo.dwLastTime == 0) {
				KeyProc ();
			}
		}

		if (g_bDraw) {
			InvalidateRect (g_hWndMain, NULL, FALSE);
//			UpdateWindow (g_hWndMain);
			s_byFps ++;
		}
	}

	if (timeGetTime () > s_dwTimeStart + 1000) {
		/* FPSの更新 */
		s_dwTimeStart = s_dwTimeFps = timeGetTime ();
		g_byFps = s_byFps;
		s_byFps = 0;
		g_dwFpsCount = s_dwFpsCount;
		s_dwFpsCount = 0;
	}

	return MAINLOOP_IDLE;
}


#ifdef __EMSCRIPTEN__
/* ========================================================================= */
/* 関数名：	MainLoopWeb														 */
/* 内容：	ブラウザ版のメインループ（描画フレームごとに呼ばれる）			 */
/* ========================================================================= */

static void MainLoopWeb(void)
{
	int i, nRet;

	/* 処理時間の計測（URL に ?perf=1 を付けた時だけ） */
	W32PerfBeginFrame ();

	/* 溜まっているメッセージを全部処理してから、タイマー処理を１回行う */
	for (i = 0; i < 1000; i ++) {
		nRet = MainLoopStep ();
		if (nRet == MAINLOOP_QUIT) {
			emscripten_cancel_main_loop ();
			return;
		}
		if (nRet == MAINLOOP_IDLE) {
			break;
		}
	}

	/* タイマー処理で出た再描画をこのフレームのうちに済ませておく。
	   Windows では InvalidateRect のすぐ後に WM_PAINT が処理されるが、
	   ここで描かずに次のフレームへ持ち越すと、先に届いた受信データ
	   （マップ切り替えなど）を処理した直後、フェードアウト開始前に
	   次のマップが一度描かれてしまう */
	UpdateWindow (g_hWndMain);

	W32PerfEndFrame ();
}
#endif


/* ========================================================================= */
/* 関数名：WinMain															 */
/* 内容：ウィンドウのメイン関数												 */
/* 日付：2003/04/29															 */
/* ========================================================================= */

void SboCliMain(LPSTR lpszArgs)
{
	HWND hWnd;
	WNDCLASS wc;
	HINSTANCE hInstance;

#if 1
	if (strcmp (lpszArgs, "-multiboot")) {
		g_mutexMain = CreateMutex (NULL, TRUE, "SboCli");
		if (!g_mutexMain || GetLastError () == ERROR_ALREADY_EXISTS) {
			return;
		}
	}

	g_hModule = LoadLibrary ("KERNEL32.DLL");
	if (g_hModule) {
		g_proc = GetProcAddress (g_hModule, "IsDebuggerPresent");
	}
#endif

	hInstance			= GetModuleHandle (NULL);
	g_hInstance			= hInstance;
	wc.hInstance		= hInstance;
	wc.lpszClassName	= CLNAME;
	wc.lpfnWndProc		= (WNDPROC)WndProc;
	wc.style			= CS_DBLCLKS;
	wc.hIcon			= LoadIcon (hInstance, (char *)IDI_ICON1);
	wc.hCursor			= LoadCursor ((HINSTANCE)NULL, IDC_ARROW);
	wc.lpszMenuName		= 0;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hbrBackground	= (HBRUSH)GetStockObject (LTGRAY_BRUSH);

	/* ウィンドウのクラスを登録 */
	if (!RegisterClass (&wc)) {
		return;
	}

	/* ウィンドウ作成 */
	char szBuf[128];
	wsprintf (szBuf, "%s%s", WNDTITLE, VERTEXT);
	hWnd = CreateWindow (CLNAME, szBuf,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_MINIMIZEBOX,
				CW_USEDEFAULT, CW_USEDEFAULT,
				SCRSIZEX + GetSystemMetrics (SM_CXEDGE) * 2,
				SCRSIZEY + GetSystemMetrics (SM_CYEDGE) * 2 + GetSystemMetrics (SM_CYCAPTION),
				NULL, NULL,
				hInstance, NULL);

	if (!hWnd) {
		return;
	}
	g_hWndMain = hWnd;

	/* ウィンドウ表示 */
	ShowWindow (hWnd, SW_HIDE);
	UpdateWindow (hWnd);

	/* メインループ */
	TIMECAPS tc;
	timeGetDevCaps (&tc, sizeof (TIMECAPS));
	/* マルチメディアタイマーのサービス精度を最大に */
	timeBeginPeriod (tc.wPeriodMin);

	MainLoopInit ();
#ifdef __EMSCRIPTEN__
	/* ブラウザでは無限ループできないので、描画フレームごとに呼んでもらう */
	emscripten_set_main_loop (MainLoopWeb, 0, 0);
	return;
#else
	while (MainLoopStep () != MAINLOOP_QUIT) {
	}
#endif

	timeEndPeriod (tc.wPeriodMin);
	UnregisterClass (CLNAME, hInstance);

	ReleaseMutex (g_mutexMain);
	CloseHandle (g_mutexMain);
}

/* ========================================================================= */
/* 関数名：WndProc															 */
/* 内容：メッセージの処理													 */
/* 日付：2003/04/29															 */
/* ========================================================================= */
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:			return OnCreate (hWnd);
	case WM_INITEND:		OnInitEnd		(hWnd);			break;
	case WM_PAINT:			OnPaint			(hWnd);			break;
	case WM_COMMAND:		OnCommand		(hWnd, wParam);	break;
	case WM_LBUTTONDOWN:	OnLbuttonDown	(hWnd, wParam, lParam);	break;

	case WM_ACTIVATE:
		/* 非アクティブになる？ */
		if (LOWORD (wParam) == WA_INACTIVE) {
			g_bWindowActive = FALSE;
		} else {
			g_bWindowActive = TRUE;
		}
		break;

	case WM_LBUTTONUP:
		{
			/* ロゴ表示画面？ */
			if (g_byGameState == GAMESTATE_LOGO) {
				if (g_byStage < 2) {
					g_byStage = 2;
					g_dwTimeLastLogo = 0;
					g_FadeInOutInfo.byFadeOut = 100;
					g_FadeInOutInfo.byFadeIn = 0;
					g_FadeInOutInfo.dwLastTime = timeGetTime ();
				}
				break;
			/* タイトル画面？ */
			} else if (g_byGameState == GAMESTATE_TITLE) {
				if (g_byStage == 0) {
					g_byStage = 1;
					g_FadeInOutInfo.byFadeOut = 100;
					g_FadeInOutInfo.byFadeIn = 0;
					g_FadeInOutInfo.dwLastTime = timeGetTime ();
				}
				break;
			}
			if (!g_bLeftClick) {
				break;
			}
			g_bLeftClick = FALSE;
			ReleaseCapture ();
		}
		break;

	case WM_MOUSEMOVE:
		{
			if (g_byGameState == GAMESTATE_CONNECTLOST) {
				break;
			}
			if (g_hWndAdmin == NULL) {
				break;
			}
			if (!g_bLeftClick) {
				break;
			}
			POINT pt;
			pt.x = LOWORD(lParam) / 32;
			pt.y = HIWORD(lParam) / 32;
			if (g_nAdminWindowType == IDM_MAP_PARTS) {
				if (!g_hWndAdmin || !g_pMapInfo->pwMap || !g_pMapPartsInfo) {
					break;
				}
				pt.x += (g_aCharInfo[g_nMyCharID]->dwX - DRAW_PARTS_X);
				pt.y += (g_aCharInfo[g_nMyCharID]->dwY - DRAW_PARTS_Y);
				if (pt.x < 0 || pt.x >= g_pMapInfo->nCX || pt.y < 0 || pt.y >= g_pMapInfo->nCY) {
					break;
				}
				if (g_nSelectPartsID != g_pMapInfo->pwMap[pt.x + pt.y * g_pMapInfo->nCX]) {
					g_pMapInfo->pwMap[pt.x + pt.y * g_pMapInfo->nCX] = (WORD)g_nSelectPartsID;
					SendCmdCHGMAP (g_nSelectPartsID, pt.x, pt.y);
				}
			}
		}
		break;

	case WM_RBUTTONDBLCLK:
		{
			if (g_byGameState == GAMESTATE_CONNECTLOST) {
				break;
			}
			if (g_hWndAdmin == NULL) {
				break;
			}
			POINT pt;
			pt.x = LOWORD(lParam) / 32;
			pt.y = HIWORD(lParam) / 32;
			if (!g_hWndAdmin || !g_pMapInfo->pwMap || !g_pMapPartsInfo) {
				break;
			}
			pt.x += (g_aCharInfo[g_nMyCharID]->dwX - DRAW_PARTS_X);
			pt.y += (g_aCharInfo[g_nMyCharID]->dwY - DRAW_PARTS_Y);
			if (pt.x < 0 || pt.x >= g_pMapInfo->nCX || pt.y < 0 || pt.y >= g_pMapInfo->nCY) {
				break;
			}
			PostMessage (g_hWndAdmin, WM_ADMINRBUTTONDBLCLK, (WPARAM)pt.x, (LPARAM)pt.y);
		}
		break;

	case WM_RBUTTONDOWN:
		{
			if (g_byGameState == GAMESTATE_CONNECTLOST) {
				break;
			}
			if (g_hWndAdmin == NULL) {
				break;
			}
			POINT pt;
			pt.x = LOWORD(lParam) / 32;
			pt.y = HIWORD(lParam) / 32;
			if (!g_hWndAdmin || !g_pMapInfo->pwMap || !g_pMapPartsInfo) {
				break;
			}
			pt.x += (g_aCharInfo[g_nMyCharID]->dwX - DRAW_PARTS_X);
			pt.y += (g_aCharInfo[g_nMyCharID]->dwY - DRAW_PARTS_Y);
			if (pt.x < 0 || pt.x >= g_pMapInfo->nCX || pt.y < 0 || pt.y >= g_pMapInfo->nCY) {
				break;
			}
			if (g_nAdminWindowType == IDM_MAP_PARTS) {
				g_nSelectPartsID = g_pMapInfo->pwMap[pt.x + pt.y * g_pMapInfo->nCX];
				g_byAdminAnimeNo = 0;
				InvalidateRect (g_hWndAdmin, NULL, TRUE);

			} else if (g_bAdminPos) {
				PostMessage (g_hWndAdmin, WM_ADMINPOSRIGHT, (WPARAM)pt.x, (LPARAM)pt.y);
			}
		}
		break;

	case WM_TIMER:
		{
			if (wParam == 0) {
				PWNDINFO pWnd;
				KillTimer (hWnd, 0);
				int nNo = GetWindowID (g_dwMsgIDConnect);
				if (nNo < 0) {
					break;
				}
				EraseWindow (g_WndInfo[nNo]);
				g_dwMsgIDConnect = 0;

				pWnd = MakeWindowMSG3SEC ("サーバーに接続できませんでした");
				pWnd->nData1 = MSG3SEC_ACC;
				g_WndInfo.Add (pWnd);
				cliSock.Destroy ();
				InvalidateRect (g_hWndMain, NULL, FALSE);
			}
		}
		break;

	/* メッセージ表示後３秒経った */
	case WM_MSG3SEC:
		{
			int nNo = GetWindowID ((DWORD)wParam);
			if (nNo < 0) {
				break;
			}

			switch (g_WndInfo[nNo]->nData1) {
			case MSG3SEC_ACC:
				EnableWindow (g_hWndCon, TRUE);
				break;
			}
			EraseWindow (g_WndInfo[nNo]);
			InvalidateRect (g_hWndMain, NULL, FALSE);
		}
		break;

	case WM_KEYUP:
		/* ロゴ表示画面？ */
		if (g_byGameState == GAMESTATE_LOGO) {
			if (g_byStage < 2) {
				g_byStage = 2;
				g_dwTimeLastLogo = 0;
				g_FadeInOutInfo.byFadeOut = 100;
				g_FadeInOutInfo.byFadeIn = 0;
				g_FadeInOutInfo.dwLastTime = timeGetTime ();
			}

		/* タイトル画面？ */
		} else if (g_byGameState == GAMESTATE_TITLE) {
			if (g_byStage == 0) {
				g_byStage = 1;
				g_aFadeInOutTbl.Add (0);
			}
		} 
	case WM_KEYDOWN:
		if (wParam == VK_SNAPSHOT) {
			if (g_byGameState < GAMESTATE_MOVE) {
				WriteGrpData2 ("", g_pDibPrimary);
			} else {
				CImg32 DibTmp;
				DibTmp.CreateWithoutGdi (g_pDibDrawMap->Width () - 64, g_pDibDrawMap->Height () - 64);
				DibTmp.Blt (0, 0, DibTmp.Width (), DibTmp.Height (), g_pDibDrawMap, 32, 32);
				WriteGrpData2 ("", &DibTmp);
			}
		} else {
			KeyProc ();
		}
		break;

	/* データ受信 */
	case WM_URARASOCK_RECV:
	case WM_RCVDATA:
		if (wParam) {
			RecvDataProc ((PBYTE)wParam, (DWORD)lParam);
		}
		break;

	/* 接続が途切れた */
	case WM_URARASOCK_DISCONNECT:
	case WM_CONNECTLOST:
		ChgState (GAMESTATE_CONNECTLOST);
		break;

	case WM_ERASEBKGND:
		break;

	case WM_CLOSE:
		{
			/* 移動直後にログオフでイベントが動作しない対応のため、終了前にイベントチェックしておく */
			if (g_nMyCharID >= 0) {
				PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
				if (g_pMapInfo && g_pMapInfo->pbyFlg && g_pMapInfo->pbyFlg[pChar->dwX + g_pMapInfo->nCX * pChar->dwY] & MAPFLG_EVENT) {
					/* サーバーにイベント問い合わせ */
					SendCmdEVENTCHECK (0);
				}
			}

			RECT rc;
			char szBuf[16], szName[MAX_PATH + 1];
			ZeroMemory (szName, sizeof (szName));
			GetModuleFileName (NULL, szName, MAX_PATH);
			szName[strlen (szName) - 3] = 0;
			strcat (szName, "ini");
			if (g_hWndMain && (IsIconic (g_hWndMain) == FALSE) && IsWindowVisible (g_hWndMain)) {
				GetWindowRect (g_hWndMain, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "MainX1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "MainY1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.right);
				WritePrivateProfileString ("Pos", "MainX2", szBuf, szName);
				wsprintf (szBuf, "%d", rc.bottom);
				WritePrivateProfileString ("Pos", "MainY2", szBuf, szName);
			}

			if (g_hWndDebug && IsWindowVisible (g_hWndDebug)) {
				GetWindowRect (g_hWndDebug, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "DbgX1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "DbgY1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.right);
				WritePrivateProfileString ("Pos", "DbgX2", szBuf, szName);
				wsprintf (szBuf, "%d", rc.bottom);
				WritePrivateProfileString ("Pos", "DbgY2", szBuf, szName);
			}

			if (g_hWndMsgLog && IsWindowVisible (g_hWndMsgLog)) {
				GetWindowRect (g_hWndMsgLog, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "LogX1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "LogY1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.right);
				WritePrivateProfileString ("Pos", "LogX2", szBuf, szName);
				wsprintf (szBuf, "%d", rc.bottom);
				WritePrivateProfileString ("Pos", "LogY2", szBuf, szName);
			}

			if (g_hWndDebug && IsWindowVisible (g_hWndDebug)) {
				GetWindowRect (g_hWndDebug, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "DbgX1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "DbgY1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.right);
				WritePrivateProfileString ("Pos", "DbgX2", szBuf, szName);
				wsprintf (szBuf, "%d", rc.bottom);
				WritePrivateProfileString ("Pos", "DbgY2", szBuf, szName);
			}

			if (g_hWndAdmin && IsWindowVisible (g_hWndAdmin)) {
				GetWindowRect (g_hWndAdmin, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "AdminX1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "AdminY1", szBuf, szName);
				wsprintf (szBuf, "%d", rc.right);
				WritePrivateProfileString ("Pos", "AdminX2", szBuf, szName);
				wsprintf (szBuf, "%d", rc.bottom);
				WritePrivateProfileString ("Pos", "AdminY2", szBuf, szName);
				SaveAdminRura ();
			}

			if (g_hWndBalloon && IsWindowVisible (g_hWndBalloon)) {
				GetWindowRect (g_hWndBalloon, &rc);
				wsprintf (szBuf, "%d", rc.left);
				WritePrivateProfileString ("Pos", "BalloonX", szBuf, szName);
				wsprintf (szBuf, "%d", rc.top);
				WritePrivateProfileString ("Pos", "BalloonY", szBuf, szName);
			}

			SaveMsgLog ();

			cliSock.SendCancel ();
			DestroyWindow (hWnd);
		}
		return TRUE;

	/* ウィンドウを閉じる */
	case WM_DESTROY:
		{
			if (g_pAdminWindow) {
				g_pAdminWindow->Destroy ();
			}
			KillTimer (hWnd, 0);
			FreeLibrary (m_hRtLib);

			char szBuf[16], szName[MAX_PATH + 1];
			ZeroMemory (szName, sizeof (szName));
			GetModuleFileName (NULL, szName, MAX_PATH);
			szName[strlen (szName) - 3] = 0;
			strcat (szName, "ini");
			wsprintf (szBuf, "%d", g_wPort);
			WritePrivateProfileString ("Port", "PortNo", szBuf, szName);
			WritePrivateProfileString ("Server", "Address", g_pszServer, szName);
			WritePrivateProfileString ("Account", "Name", g_szAcc, szName);
			SaveIniOption ();

			ReleaseData ();
			CoUninitialize ();
			PostQuitMessage (0);
		}
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}

	return FALSE;
}


/* ========================================================================= */
/* 関数名：	OnCreate														 */
/* 内容：	メッセージハンドラ(WM_CREATE)									 */
/* 日付：	2003/05/11														 */
/* ========================================================================= */

BOOL OnCreate(HWND hWnd)
{
	char szBuf[MAX_PATH + 1];
	CMacAddr MacAddr;

	CoInitialize (NULL);
	g_bWindowActive = TRUE;
	g_byGameState = GAMESTATE_LOGO;
	g_hFontBasic = CreateFont (16, 0, 0, 0, FW_ULTRABOLD,
			FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "ＭＳ ゴシック");
	g_hFont12 = CreateFont (12, 0, 0, 0, FW_NORMAL,
			FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "ＭＳ ゴシック");
	g_hFont14 = CreateFont (14, 0, 0, 0, FW_ULTRABOLD,
			FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "ＭＳ ゴシック");
	g_hFont32 = CreateFont (32, 0, 0, 0, FW_NORMAL,
			TRUE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "ＭＳ Ｐ明朝");

	g_wLastProcSecond		= 0xFFFF;			/* 自キャラ最終処理秒 */
	g_byStage				= 0;				/* 状態管理用 */
	g_byViewMode			= 3;				/* 表示モード */
	g_byAnimeNo				= 0;				/* アニメーション番号 */
	g_byPosReqNo			= 0;				/* 座標通知シーケンス番号 */
	g_dwPlayBgmNo			= 0;				/* 演奏中BGM番号 */
	g_nProcMsg				= 0;				/* 処理ID(メッセージ) */
	g_nProcChar				= 0;				/* 処理ID(キャラ) */
	g_nProcSysMsg			= 0;				/* 処理ID(システムメッセージ) */
	g_nProcMapParts			= 0;				/* 処理ID(マップパーツ) */
	g_nProcObj				= 0;
	g_hWndAdmin				= NULL;
	g_hWndDebug				= NULL;
	g_hWndBalloon			= NULL;
	g_nLoginAnimeX			= -32;
	g_pMyID					= -1;
	g_nMyCharID				= -1;
	g_dwNewID				= 1;
	g_dwTimeDrawTime		= 0;
	g_dwActiveMsgID			= 0;
	g_dwMsgIDCommand		= 0;
	g_dwMsgIDTRADE			= 0;
	g_dwMsgIDGold			= 0;
	g_dwMsgIDSubGame		= 0;
	g_dwTimePingLast		= 0;
	g_dwTimeDataCount		= 0;
	g_dwTimeLastMapParts	= 0;
	g_dwTimeLastAnime		= 0;
	g_dwTimeTextFadeInOut	= 0;
	g_dwTimeLastCursor		= 0;
	g_byCursorAnime			= 0;
	g_dwOnilne				= 0;
	g_bReqestPos			= FALSE;
	g_bMoving				= FALSE;
	g_bEnableMove			= TRUE;
	g_bSpeech				= FALSE;
	g_dwTimeMoveDisable		= 0;
	g_dwTimeHartbeat		= 0;
	g_dwTimeHartbeatReq		= 0;
	g_bBGMONOFF				= TRUE;
	g_bCpuMax				= FALSE;
	g_bComEfcONOFF			= TRUE;
	g_bChatSound			= TRUE;
	g_byAdminCount			= 0;
	g_dwCountPC				= 0;
	g_dwCountNPC			= 0;
	g_nProcShopID			= -1;
	g_nProcShopState		= 0;
	g_bProcShopConfirm		= FALSE;
	g_nProcShopSelect		= 0;
	g_bBlock				= TRUE;
	g_byUseEfcID			= 0;
	g_byTextFadeInOut		= 0;
	g_bTitleSkip			= FALSE;
	g_pLeaveInfo			= NULL;
	g_pTradeInfo			= NULL;
	g_pSubGameInfo			= NULL;
	g_pSubGameInfoPlay		= NULL;
	g_bMainWindowView		= TRUE;
	g_bBalloonWindowView	= TRUE;
	g_bSpeechView			= TRUE;
	g_bBattleMode			= FALSE;
	g_bSnowCharge			= FALSE;
	g_pWarSelectTurn		= NULL;
	g_pWarInfo				= NULL;
	g_pWarMagicTarget		= NULL;
	g_byComSelectBattle		= 0;
	g_byComSelectMagic		= 0;
	g_pTargetSelectInfo		= NULL;
	g_bySpeechMode			= 0;
	g_bySoundCount			= 0;
	g_bHartbeatCheck		= FALSE;
	g_bMapMarkView			= TRUE;
	g_bWeatherDraw			= TRUE;
	ZeroMemory (g_szAcc, sizeof (g_szAcc));
	ZeroMemory (g_szPas, sizeof (g_szPas));
	ZeroMemory (&g_FadeInOutInfo, sizeof (g_FadeInOutInfo));

	g_pMapDarkTmp = new char[SCRMARTSSIZE];
	ZeroMemory (g_pMapDarkTmp, SCRMARTSSIZE);
	g_pMapDarkTmpPos = new char[SCRMARTSSIZE];
	ZeroMemory (g_pMapDarkTmpPos, SCRMARTSSIZE);

	g_pMapInfo = new MAPINFO;
	ZeroMemory (g_pMapInfo, sizeof (MAPINFO));
	g_pMapPartsInfo = NULL;

	g_pComEfcInfo = new COMEFCINFO[99];
	ZeroMemory (g_pComEfcInfo, sizeof (COMEFCINFO) * 99);

	g_pszServer = (LPSTR)new BYTE[MAX_PATH + 1];
	ZeroMemory ((LPVOID)g_pszServer, sizeof (g_pszServer));

	ZeroMemory (szBuf, sizeof (szBuf));
	GetModuleFileName (NULL, szBuf, MAX_PATH);
	szBuf[strlen (szBuf) - 10] = 0;
	strcat (szBuf, "\\\\log");
	CreateDirectory (szBuf, NULL);

	ZeroMemory (szBuf, sizeof (szBuf));
	GetModuleFileName (NULL, szBuf, MAX_PATH);
	szBuf[strlen (szBuf) - 10] = 0;
	strcat (szBuf, "\\\\ss");
	CreateDirectory (szBuf, NULL);

	ZeroMemory (szBuf, sizeof (szBuf));
	GetModuleFileName (NULL, szBuf, MAX_PATH);
	szBuf[strlen (szBuf) - 3] = 0;
	strcat (szBuf, "ini");
	g_wPort = GetPrivateProfileInt ("Port", "PortNo", 2004, szBuf);
	GetPrivateProfileString ("Server", "Address", "127.0.0.1", g_pszServer, MAX_PATH, szBuf);
	GetPrivateProfileString ("Account", "Name", "", g_szAcc, sizeof (g_szAcc), szBuf);
	g_bCpuMax				= GetPrivateProfileInt ("Option", "CpuMax", 0, szBuf);
	g_bComEfcONOFF			= GetPrivateProfileInt ("Option", "ComEfc", 1, szBuf);
	g_bChatSound			= GetPrivateProfileInt ("Option", "ChatSound", 1, szBuf);
	g_bBGMONOFF				= GetPrivateProfileInt ("Option", "Bgm", 1, szBuf);
	g_bTitleSkip			= GetPrivateProfileInt ("Option", "SkipTitle", 0, szBuf);
	g_bTitleSkip = FALSE;
	g_byVolSound			= GetPrivateProfileInt ("Option", "VolSound", 2, szBuf);
	g_byVolMusic			= GetPrivateProfileInt ("Option", "VolMusic", 2, szBuf);
	g_bBalloonWindowView	= GetPrivateProfileInt ("Option", "BalloonWindow", 1, szBuf);
	g_bSpeechView			= GetPrivateProfileInt ("Option", "SpeechView", 1, szBuf);
	g_byViewMode			= GetPrivateProfileInt ("Option", "ViewMode", 3, szBuf);
	g_bWeatherDraw			= GetPrivateProfileInt ("Option", "WeatherDraw", 1, szBuf);

	rcMain.left		= GetPrivateProfileInt ("Pos", "MainX1", 0, szBuf);
	rcMain.top		= GetPrivateProfileInt ("Pos", "MainY1", 0, szBuf);
	rcMain.right	= GetPrivateProfileInt ("Pos", "MainX2", 0, szBuf);
	rcMain.bottom	= GetPrivateProfileInt ("Pos", "MainY2", 0, szBuf);

	rcDebug.left	= GetPrivateProfileInt ("Pos", "DbgX1", 0, szBuf);
	rcDebug.top		= GetPrivateProfileInt ("Pos", "DbgY1", 0, szBuf);
	rcDebug.right	= GetPrivateProfileInt ("Pos", "DbgX2", 0, szBuf);
	rcDebug.bottom	= GetPrivateProfileInt ("Pos", "DbgY2", 0, szBuf);

	rcMsgLog.left	= GetPrivateProfileInt ("Pos", "LogX1", 0, szBuf);
	rcMsgLog.top	= GetPrivateProfileInt ("Pos", "LogY1", 0, szBuf);
	rcMsgLog.right	= GetPrivateProfileInt ("Pos", "LogX2", 0, szBuf);
	rcMsgLog.bottom	= GetPrivateProfileInt ("Pos", "LogY2", 0, szBuf);

	rcBalloon.left	= GetPrivateProfileInt ("Pos", "BalloonX", 0, szBuf);
	rcBalloon.top	= GetPrivateProfileInt ("Pos", "BalloonY", 0, szBuf);

	rcAdmin.left	= GetPrivateProfileInt ("Pos", "AdminX1", 0, szBuf);
	rcAdmin.top		= GetPrivateProfileInt ("Pos", "AdminY1", 0, szBuf);
	rcAdmin.right	= GetPrivateProfileInt ("Pos", "AdminX2", 0, szBuf);
	rcAdmin.bottom	= GetPrivateProfileInt ("Pos", "AdminY2", 0, szBuf);

	if (!LoadAdminRura ()) {
		PADMINRURAINFO pInfoNew = new ADMINRURAINFO;
		ZeroMemory (pInfoNew, sizeof (ADMINRURAINFO));
		g_AdminRuraInfo.Add (pInfoNew);
	}

	if (!DXAudio.Create ()) {
		MessageBox (hWnd, "DirectMusicの初期化に失敗しました。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}

	g_hDll = LoadLibrary ("SboSoundData.dll");
	if (g_hDll == NULL) {
		MessageBox (hWnd, "サウンドデータDLLの読み込みに失敗しました。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}
	g_hDllGrpData = LoadLibrary ("SboGrpData.dll");
	if (g_hDllGrpData == NULL) {
		MessageBox (hWnd, "グラフィックデータDLLの読み込みに失敗しました。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}

	MacAddr.Get ((PBYTE)g_szMacAdr);

	ZeroMemory (szBuf, sizeof (szBuf));
	LoadString (g_hDll, IDS_DLLVER, szBuf, sizeof (szBuf));
	if (strcmp (szBuf, DLLVER) != 0) {
		MessageBox (hWnd, "リソースDLLのバージョンが合いません。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}
	LoadString (g_hDllGrpData, IDS_DLLVER, szBuf, sizeof (szBuf));
	if (strcmp (szBuf, GRPDATADLLVER) != 0) {
		MessageBox (hWnd, "グラフィックデータDLLのバージョンが合いません。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}

	m_hRtLib = LoadLibrary ("RICHED32.DLL");
	if (m_hRtLib == NULL) {
		MessageBox (hWnd, "RICHED32.DLLが読み込めませんでした。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return FALSE;
	}

	int i;
	DXAudio.SetResourceHandle (g_hDll);
	ZeroMemory (aSoundTbl, sizeof (aSoundTbl));
	/* サウンドの読み込み */
	for (i = 1; ; i ++) {
		if (adwSoundNoTbl[i] == 0xFFFFFFFF) {
			break;
		}
		DXAudio.GetSegFromRes (FindResource (g_hDll,
				MAKEINTRESOURCE (adwBgmIDTbl[adwSoundNoTbl[i]]), "WAVE"),
				&aSoundTbl[adwSoundNoTbl[i]]);
	}
	g_byVolSound = min (5, g_byVolSound);
	DXAudio.SetVolSecoundary ((long)anVolTbl[g_byVolSound]);

	PostMessage (hWnd, WM_INITEND, 0, 0);

	SetWindowPos (hWnd, 0, rcMain.left, rcMain.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	OnInitEnd														 */
/* 内容：	メッセージハンドラ(WM_INITEND)									 */
/* 日付：	2005/04/04														 */
/* ========================================================================= */

void OnInitEnd(HWND hWnd)
{
	if (!ReadData ()) {
		MessageBox (hWnd, "グラフィックデータの読み込みに失敗しました。", "エラー", MB_OK);
		PostMessage (hWnd, WM_CLOSE, 0, 0);
		return;
	}

	/* タイトルスキップモード？ */
	if (g_bTitleSkip) {
		ChgState (GAMESTATE_LOGIN);

	} else {
		ChgState (GAMESTATE_LOGO);
	}
	g_bDrawStop = FALSE;
	ShowWindow (hWnd, SW_SHOW);
}


/* ========================================================================= */
/* 関数名：	OnPaint															 */
/* 内容：	メッセージハンドラ(WM_PAINT)									 */
/* 日付：	2006/02/17														 */
/* ========================================================================= */

void OnPaint(HWND hWnd)
{
	DWORD dwTmp;
	HDC hDC;
	PAINTSTRUCT ps;

	dwTmp	= timeGetTime ();
	hDC		= BeginPaint (hWnd, &ps);

	DrawScreen (hWnd, hDC);
	EndPaint (hWnd, &ps);

	g_dwTimeDrawTime = timeGetTime () - dwTmp;
}


/* ========================================================================= */
/* 関数名：	OnCommand														 */
/* 内容：	メッセージハンドラ(WM_COMMAND)									 */
/* 日付：	2006/04/05														 */
/* ========================================================================= */

void OnCommand(HWND hWnd, int nID)
{
	int j;
	LPSTR pszTmp;
	PWNDINFO pWnd, pWndNew;
	PCHARINFO pChar;

	switch (nID) {
	case IDC_CONNECT:		/* 接続ボタン */
		/* 接続中？ */
		if (g_dwMsgIDConnect) {
			break;
		}
		/* ID・パスワードで入るので、端末トークンは使わない */
		ZeroMemory (g_szWebToken, sizeof (g_szWebToken));
		{
			char szAccTmp[MAX_ACCNAME];
			ZeroMemory (szAccTmp, sizeof (szAccTmp));
			GetWindowText (g_hWndAcc, szAccTmp, MAX_ACCNAME);
			/* アカウント名は 1 バイト文字だけ（日本語入力のまま打った時など）。
			   黙って何もしないと押しても反応しないように見えるので、知らせる */
			if (!SboTextIsSingleByteOnly (szAccTmp)) {
				EnableWindow (g_hWndCon, FALSE);
				pWnd = MakeWindowMSG3SEC ("アカウント名は半角英数字で入力してください");
				pWnd->nData1 = MSG3SEC_ACC;
				g_WndInfo.Add (pWnd);
				InvalidateRect (g_hWndMain, NULL, FALSE);
				break;
			}
			ZeroMemory (g_szAcc, sizeof (g_szAcc));
			strcpy (g_szAcc, szAccTmp);
		}
		EnableWindow (g_hWndCon, FALSE);

		ZeroMemory (g_szPas, sizeof (g_szPas));
		GetWindowText (g_hWndPas, g_szPas, MAX_ACCNAME);
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

		/* 未入力？ */
		if (!strlen (g_szAcc)) {
			pWnd = MakeWindowMSG3SEC ("アカウント名を入力してください");
			pWnd->nData1 = MSG3SEC_ACC;
			g_WndInfo.Add (pWnd);
			InvalidateRect (g_hWndMain, NULL, FALSE);

		} else {
			pWnd = MakeWindowMSGVIEW ("サーバーに接続中です...");
			g_WndInfo.Add (pWnd);
			InvalidateRect (g_hWndMain, NULL, FALSE);
			g_dwMsgIDConnect = pWnd->dwID;
			SetTimer (hWnd, 0, 10000, NULL);
			SetFocus (g_hWndMain);
			cliSock.Connect (g_hWndMain, g_wPort, g_pszServer);
		}
		break;

	case IDC_CONNECTTOKEN:	/* 端末トークンで接続（ブラウザ版のログイン画面のボタン） */
	case IDC_CONNECTNEW:	/* 自動で作ったアカウントで接続（ブラウザ版の「すぐにはじめる」） */
		/* 接続中？ */
		if (g_dwMsgIDConnect) {
			break;
		}
		if (((nID == IDC_CONNECTTOKEN) && (g_szWebToken[0] == 0)) ||
				((nID == IDC_CONNECTNEW) && (g_szAcc[0] == 0))) {
			break;
		}
		EnableWindow (g_hWndCon, FALSE);
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd = MakeWindowMSGVIEW ("サーバーに接続中です...");
		g_WndInfo.Add (pWnd);
		InvalidateRect (g_hWndMain, NULL, FALSE);
		g_dwMsgIDConnect = pWnd->dwID;
		SetTimer (hWnd, 0, 10000, NULL);
		SetFocus (g_hWndMain);
		cliSock.Connect (g_hWndMain, g_wPort, g_pszServer);
		break;

	case IDC_MAKE:		/* 作成ボタン */
		pChar = g_aCharInfo[g_nMyCharID];
		pszTmp = new char [MAX_CHARNAMEBUF];
		ZeroMemory (pszTmp, MAX_CHARNAMEBUF);
		ZeroMemory (pChar->szName, sizeof (pChar->szName));
		GetWindowText (g_hWndName, pszTmp, MAX_CHARNAMEBUF);
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

		/* 表示幅 14（全角 7 文字）まで、文字の途中で切らずに写す。空白・制御文字・全角空白・
		   名前に使えない文字（絵文字など Shift_JIS に無い文字）が入っていたら名前なし */
		{
			int i, nLen = min (SboTextFitWidth (pszTmp, MAX_CHARNAME_WIDTH), SboTextFitBytes (pszTmp, sizeof (pChar->szName) - 1)), nChar;
			BOOL bNG = !SboTextIsNameSafe (pszTmp);
			for (i = 0; i < nLen; i += nChar) {
				nChar = SboTextCharLen (&pszTmp[i]);
				if ((nChar == 1) && ((BYTE)pszTmp[i] <= ' ')) {
					bNG = TRUE;
				}
				if ((nChar == (int)strlen ("　")) && (memcmp (&pszTmp[i], "　", nChar) == 0)) {
					bNG = TRUE;
				}
			}
			if (!bNG) {
				CopyMemory (pChar->szName, pszTmp, nLen);
			}
		}

		if (pChar->szName[0]) {
			pWndNew = MakeWindowMALEFAMALE ();
			pWndNew->pParent = g_WndInfo[GetWindowID (g_dwActiveMsgID)];
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;
			EnableWindow (g_hWndMake, FALSE);
			SetFocus (g_hWndMain);
		}
		break;
	}
}


/* ========================================================================= */
/* 関数名：	OnLbuttonDown													 */
/* 内容：	メッセージハンドラ(WM_LBUTTONDOWN)								 */
/* 日付：	2006/07/26														 */
/* ========================================================================= */

void OnLbuttonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT pt;

	if (g_byGameState == GAMESTATE_CONNECTLOST) {
		return;
	}
	if (g_hWndAdmin == NULL) {
		return;
	}

	pt.x = LOWORD(lParam) / 32;
	pt.y = HIWORD(lParam) / 32;
	if (!g_hWndAdmin || !g_pMapInfo->pwMap || !g_pMapPartsInfo) {
		return;
	}
	pt.x += (g_aCharInfo[g_nMyCharID]->dwX - DRAW_PARTS_X);
	pt.y += (g_aCharInfo[g_nMyCharID]->dwY - DRAW_PARTS_Y);
	if (pt.x < 0 || pt.x >= g_pMapInfo->nCX || pt.y < 0 || pt.y >= g_pMapInfo->nCY) {
		return;
	}

	if (g_nAdminWindowType == IDM_MAP_PARTS) {
		if (g_nSelectPartsID != g_pMapInfo->pwMap[pt.x + pt.y * g_pMapInfo->nCX]) {
			g_pMapInfo->pwMap[pt.x + pt.y * g_pMapInfo->nCX] = (WORD)g_nSelectPartsID;
			SendCmdCHGMAP (g_nSelectPartsID, pt.x, pt.y);
		}
		g_bLeftClick = TRUE;
		SetCapture (hWnd);

	} else if (g_bAdminPos) {
		PostMessage (g_hWndAdmin, WM_ADMINPOSLEFT, (WPARAM)pt.x, (LPARAM)pt.y);
	}
}


/* ========================================================================= */
/* 関数名：	ChgState														 */
/* 内容：	状態変更														 */
/* 日付：	2005/04/04														 */
/* ========================================================================= */

void ChgState(int nState)
{
	PWNDINFO pWnd;

	switch (nState) {
	case GAMESTATE_LOGO:			/* URARA-worksロゴ */
		g_byGameState = GAMESTATE_LOGO;
		g_aFadeInOutTbl.Add (1);
		g_dwTimeLastLogo = 0;
		break;

	case GAMESTATE_TITLE:			/* タイトル画面 */
		g_byGameState = GAMESTATE_TITLE;
		g_dwTimeLastLogo = 0;
		g_FadeInOutInfo.byFadeIn = 0;
		g_aFadeInOutTbl.Add (1);
		g_byStage = 0;
		g_bDraw = TRUE;
		break;

	case GAMESTATE_LOGIN:			/* ログイン画面 */
		g_byGameState = GAMESTATE_LOGIN;
		pWnd = MakeWindowLOGIN ();
		g_WndInfo.Add (pWnd);
		DXAudio.SetLoopPoints (&aSoundTbl[SOUND_MIDI_W0081], DMUS_SEG_REPEAT_INFINITE);
		PlayBGM (SOUND_MIDI_W0081, TRUE);
		g_FadeInOutInfo.byFadeIn = 0;
		g_FadeInOutInfo.byFadeOut = 0;
		/* タイトルのフェードイン中にキーを押すと、予約したフェードアウトが残ったまま
		   ログイン画面になり、そのあと暗くなって戻らなくなるので予約を消しておく */
		g_aFadeInOutTbl.RemoveAll ();
		g_bDraw = TRUE;
		g_nLoginAnimeX = -32;
		InvalidateRect (g_hWndMain, NULL, FALSE);
		break;

	case GAMESTATE_CONNECTLOST:		/* 切断 */
		g_byGameState = GAMESTATE_CONNECTLOST;
		SaveMsgLog ();
		/* 表示中のウィンドウを全部閉じる */
		while (1) {
			int nCount = g_WndInfo.GetSize ();
			if (nCount == 0) {
				break;
			}
			/* EraseWindow で配列から取り除かれて詰められるので、常に先頭を消す */
			EraseWindow (g_WndInfo[0]);
		}
		RecvDataHoldClear ();
		pWnd = MakeWindowCONNECTLOST ("サーバーとの接続が途切れました");
		g_WndInfo.Add (pWnd);
		InvalidateRect (g_hWndMain, NULL, FALSE);
		UpdateWindow (g_hWndMain);
		StopBGM ();
		break;
	}
}


/* ========================================================================= */
/* 関数名：	ReadData														 */
/* 内容：	各データの読み込み												 */
/* 日付：	2003/04/30														 */
/* ========================================================================= */

BOOL ReadData(void)
{
	int i;
	char szBuf[32];

	/* 描画テンポラリ領域の作成 */
	pDibDrawTmp.Create (SCRSIZEX, SCRSIZEY);

	/* プライマリビットマップ作成 */
	g_pDibPrimary = new CImg32;
	g_pDibPrimary->Create (SCRSIZEX, SCRSIZEY);

	/* システムグラフィックの読み込み */
	ReadGrpData2 ("IDP_SYSTEM", &g_pDibSystem, 1);

	/* ロゴグラフィックの読み込み */
	ReadGrpData2 ("IDP_LOGO", &g_pDibLogo, 2);

	/* タイトルグラフィックの読み込み */
	ReadGrpData2 ("IDP_TITLE", &g_pDibTitle, 1);

	/* タイトル背景グラフィックの読み込み */
	ReadGrpData2 ("IDP_TITLEBACK", &g_pDibTitleBack, 1);

	/* アニメーション用ビットマップの読み込み */
	ReadGrpData2 ("IDP_ANIME", &g_pDibAnime, 1);

	/* サブゲーム（爆弾ゲーム）グラフィックの読み込み */
	ReadGrpData2 ("IDP_SUBG_BOMB", &g_pDibSubgBomb, 2);

	/* サブゲーム（戦場ゲーム）グラフィックの読み込み */
	ReadGrpData2 ("IDP_WCHAR01", &g_pDibWChar, 2);

	/* 吹き出しグラフィックの読み込み */
	ReadGrpData2 ("IDP_EFC01", &g_pDibComEfc, 2);

	/* 吹き出しウィンドウ用グラフィックの読み込み */
	ReadGrpData2 ("IDP_EFC01", &g_pDibComEfcWindow, 1);

	/* マップグラフィックの読み込み */
	g_wCountMapPartsGrp = 5;
	g_apDibMap		= new CImg32 * [g_wCountMapPartsGrp];
	g_apDibMapBack	= new CImg32 * [g_wCountMapPartsGrp];
	g_pDibDrawMap	= new CImg32;
	g_pDibMapParts	= new CImg32;
	g_pDibDrawMap->Create (SCRSIZEX + 64, SCRSIZEY + 64);

	g_byMapGrpCountX = 16;
	g_byMapGrpCountY = 16;

	for (i = 0; i < g_wCountMapPartsGrp; i ++) {
		wsprintf (szBuf, "IDP_MAP%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibMap[i], 2);
		ReadGrpData2 (szBuf, &g_apDibMapBack[i], 2);
	}

	/* キャラグラフィックの読み込み */
	g_wCountCharGrp	= 16;
	g_apDibChar		= new CImg32 * [g_wCountCharGrp];
	g_apDibCharBack	= new CImg32 * [g_wCountCharGrp];

	for (i = 0; i < g_wCountCharGrp; i ++) {
		wsprintf (szBuf, "IDP_CHAR%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibChar[i], 2);
		ReadGrpData2 (szBuf, &g_apDibCharBack[i], 2);
	}

	/* キャラグラフィック(16x32)の読み込み */
	g_wCountCharGrp1632	= 1;
	g_apDibChar1632		= new CImg32 * [g_wCountCharGrp1632];
	g_apDibChar1632Back	= new CImg32 * [g_wCountCharGrp1632];

	for (i = 0; i < g_wCountCharGrp1632; i ++) {
		wsprintf (szBuf, "IDP_CHAR1632_%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibChar1632[i], 2);
		ReadGrpData2 (szBuf, &g_apDibChar1632Back[i], 2);
	}

	/* キャラグラフィック(32x32)の読み込み */
	g_wCountCharGrp3232	= 1;
	g_apDibChar3232		= new CImg32 * [g_wCountCharGrp3232];
	g_apDibChar3232Back	= new CImg32 * [g_wCountCharGrp3232];

	for (i = 0; i < g_wCountCharGrp3232; i ++) {
		wsprintf (szBuf, "IDP_CHAR3232_%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibChar3232[i], 2);
		ReadGrpData2 (szBuf, &g_apDibChar3232Back[i], 2);
	}

	/* キャラグラフィック(48x48)の読み込み */
	g_wCountCharGrp4848	= 1;
	g_apDibChar4848		= new CImg32 * [g_wCountCharGrp4848];
	g_apDibChar4848Back	= new CImg32 * [g_wCountCharGrp4848];

	for (i = 0; i < g_wCountCharGrp4848; i ++) {
		wsprintf (szBuf, "IDP_CHAR4848_%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibChar4848[i], 2);
		ReadGrpData2 (szBuf, &g_apDibChar4848Back[i], 2);
	}

	/* エフェクトグラフィックの読み込み */
	g_wCountEfcGrp	= 5;
	g_apDibEfect	= new CImg32 * [g_wCountEfcGrp];
	for (i = 0; i < g_wCountEfcGrp; i ++) {
		wsprintf (szBuf, "IDP_EFC%02d", i + 2);
		ReadGrpData2 (szBuf, &g_apDibEfect[i], 2);
	}

	/* エフェクトグラフィック(16x32)の読み込み */
	g_wCountEfcGrp1632	= 1;
	g_apDibEfect1632	= new CImg32 * [g_wCountEfcGrp1632];
	for (i = 0; i < g_wCountEfcGrp1632; i ++) {
		wsprintf (szBuf, "IDP_EFC1632_%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibEfect1632[i], 2);
	}

	/* オブジェクト(32x32)グラフィックの読み込み */
	g_wCountObjGrp	= 1;
	g_apDibObj3232	= new CImg32 * [g_wCountObjGrp];

	for (i = 0; i < g_wCountObjGrp; i ++) {
		wsprintf (szBuf, "IDP_OBJ3232_%02d", i + 1);
		ReadGrpData2 (szBuf, &g_apDibObj3232[i], 2);
	}

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	ReadGrpData														 */
/* 内容：	グラフィックデータの読み込み									 */
/* 日付：	2003/11/22														 */
/* ========================================================================= */

void ReadGrpData(LPSTR pszName, CImg32 **pDib, int nSize)
{
	HBITMAP hBmp;
	BITMAP Bmp;
	SIZE size;
	CImg32 *pDibTmp = new CImg32;

	hBmp = (HBITMAP)LoadImage(g_hInstance, pszName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	GetObject (hBmp, sizeof (Bmp), &Bmp);
	size.cx = Bmp.bmWidth;
	size.cy = Bmp.bmHeight;

	*pDib = new CImg32;
	CImg32 *pDibNew = *pDib;
	pDibNew->CreateWithoutGdi (size.cx * nSize, size.cy * nSize);
	pDibTmp->Create (size.cx * nSize, size.cy * nSize);

	HDC hDCBmp, hDC = GetDC (NULL), hDCTmp = CreateCompatibleDC (hDC);
	ReleaseDC (NULL, hDC);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hDCTmp, hBmp);

	hDCBmp = pDibTmp->Lock ();
		StretchBlt (hDCBmp, 0, 0, size.cx * nSize, size.cy * nSize,
					hDCTmp, 0, 0, size.cx, size.cy, SRCCOPY);
	pDibTmp->Unlock ();

	pDibNew->Blt (0, 0, size.cx * nSize, size.cy * nSize, pDibTmp, 0, 0);

	SelectObject (hDCTmp, hBmpOld);
	DeleteObject (hBmp);
	DeleteDC (hDCTmp);
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：PngReadFunc														 */
/* 内容：PNG読み込み用コールバック関数										 */
/* 日付：2003/11/22															 */
/* ========================================================================= */
static void PngReadFunc(png_struct *Png,png_bytep buf,png_size_t size)
{
    TPngFileBuffer *PngFileBuffer = (TPngFileBuffer*)png_get_io_ptr (Png);
    memcpy (buf, PngFileBuffer->Buffer+PngFileBuffer->Pos, size);
    PngFileBuffer->Pos += size;
}

/* ========================================================================= */
/* 関数名：ReadGrpData2														 */
/* 内容：グラフィックデータの読み込み										 */
/* 日付：2003/11/22															 */
/* ========================================================================= */
void ReadGrpData2(LPSTR pszName, CImg32 **pDib, int nSize)
{
	*pDib = new CImg32;
	CImg32 *pDibNew = *pDib;
	CImg32 *pDibTmp = new CImg32;

	/* 指定されたリソースを読み込み */
	HGLOBAL hRes = LoadResource (g_hDllGrpData, FindResource (g_hDllGrpData, pszName, "PNG"));
	if (hRes == NULL) {
		return;
	}

	png_struct *Png = NULL;
	png_info *Info = NULL;
	TPngFileBuffer PngFileBuffer = {NULL, 0};

	Png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	Info = png_create_info_struct (Png);
	PngFileBuffer.Buffer = (BYTE *)LockResource (hRes);
	PngFileBuffer.Pos = 0;
	png_set_read_fn (Png, (png_voidp)&PngFileBuffer, (png_rw_ptr)PngReadFunc);

	png_read_info (Png, Info);
	png_uint_32 Width, Height;
	int BitDepth, ColorType;
	int InterlaceType;
	int CompressionType;
	int FilterType;
	png_get_IHDR (Png, Info, &Width, &Height, &BitDepth, &ColorType, &InterlaceType, &CompressionType, &FilterType);
	BitDepth = 8;

	png_set_bgr(Png);

	png_colorp Palette;
	int PaletteCount = 0;
	if (BitDepth == 8) {
		png_get_PLTE (Png, Info, &Palette, &PaletteCount);
	}

	png_byte *LineBuf = new png_byte[Width * BitDepth / 8];

	pDibTmp->Create (Width, Height);
	pDibNew->CreateWithoutGdi (Width * nSize, Height * nSize);
	LPBYTE pData = pDibTmp->GetBits ();
	PBYTE pTmp = pData;

	/* １ラインずつPNGデータを読み込む */
	for (int y = 0; y < (int)Height; y ++) {
		png_read_row (Png, LineBuf, NULL);
		pTmp = pData + (Height - y - 1) * Width * 4;
		   for (int x = 0; x < (int)Width; x ++) {
			BYTE *Src = LineBuf + x * BitDepth / 8;
			if (BitDepth == 8) {
				pTmp[1] = Palette[*Src].green;
				pTmp[0] = Palette[*Src].blue;
				pTmp[2] = Palette[*Src].red;
			} else {
				CopyMemory (pTmp, Src, 3);
			}
			pTmp += 4;
		}
	}
	png_destroy_read_struct (&Png, &Info, NULL);
	delete [] LineBuf;

	CImg32 *pDibTmp2 = new CImg32;
	pDibTmp2->Create (Width * nSize, Height * nSize);
	HDC hDCBmp2 = pDibTmp2->Lock ();
	HDC hDCBmp = pDibTmp->Lock ();

	/* 指定サイズに拡大してDIBにコピー */
	StretchBlt (hDCBmp2, 0, 0, Width * nSize, Height * nSize,
			hDCBmp, 0, 0, Width, Height, SRCCOPY);

	pDibTmp->Unlock ();
	pDibTmp2->Unlock ();

	pDibNew->Blt (0, 0, Width * nSize, Height * nSize, pDibTmp2, 0, 0);

	delete pDibTmp;
	delete pDibTmp2;
}

/* ========================================================================= */
/* 関数名：WriteGrpData														 */
/* 内容：画像をファイルに出力												 */
/* 日付：2004/02/29															 */
/* ========================================================================= */
void WriteGrpData(LPSTR pszName, CImg32 *pDib)
{
	HANDLE hFile;
	SYSTEMTIME sysTime;
	GetLocalTime (&sysTime);
	char szTmp[MAX_PATH + 10], szTmp2[128];
	DWORD dwFileSize;
	BITMAPFILEHEADER bmpFile;
	BITMAPINFOHEADER bmpInfo;
	PBYTE pSaveData, pTmp;

	dwFileSize = sizeof (bmpFile) + sizeof (bmpInfo) + pDib->GetImageSize ();
	ZeroMemory (&bmpFile, sizeof (bmpFile));
	ZeroMemory (&bmpInfo, sizeof (bmpInfo));
	ZeroMemory (szTmp, sizeof (szTmp));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szTmp, MAX_PATH);
	szTmp[strlen (szTmp) - 10] = 0;
	strcat (szTmp, "ss\\sbo");
	wsprintf (szTmp2, "%02d%02d%02d%02d%02d",
			sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	strcat (szTmp, szTmp2);
	strcat (szTmp, ".bmp");

	bmpFile.bfType = 0x4D42;
	bmpFile.bfOffBits = sizeof (bmpFile) + sizeof (bmpInfo);
	bmpInfo.biSize = sizeof (bmpInfo);
	bmpInfo.biWidth = pDib->Width ();
	bmpInfo.biHeight = pDib->Height ();
	bmpInfo.biPlanes = 1;
	bmpInfo.biBitCount = 24;
	bmpInfo.biCompression = BI_RGB;


	pSaveData = new BYTE[dwFileSize];
	pTmp = pSaveData;
	ZeroMemory (pSaveData, dwFileSize);

	bmpFile.bfSize = dwFileSize;
	memcpy (pTmp, &bmpFile, sizeof (bmpFile));
	pTmp += sizeof (bmpFile);
	memcpy (pTmp, &bmpInfo, sizeof (bmpInfo));
	pTmp += sizeof (bmpInfo);
	LPBYTE pData = pDib->GetBits ();
	for (int y = 0; y < pDib->Height (); y ++) {
		pData = pDib->GetBits () + y * (pDib->Width () * 4);
		for (int x = 0; x < pDib->Width (); x ++) {
			CopyMemory (pTmp, pData, 3);
			pData += 4;
			pTmp += 3;
		}
	}

	/* ファイルを開く */
	hFile = CreateFile (szTmp, GENERIC_WRITE
			, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD dwAccBytes;
	WriteFile (hFile, pSaveData, dwFileSize, &dwAccBytes, NULL);
	delete [] pSaveData;

	CloseHandle (hFile);
}

/* ========================================================================= */
/* 関数名：WriteGrpData2													 */
/* 内容：画像をファイルに出力												 */
/* 日付：2004/03/01															 */
/* ========================================================================= */
void WriteGrpData2(LPSTR pszName, CImg32 *pDib)
{
	SYSTEMTIME sysTime;
	GetLocalTime (&sysTime);
	char szTmp[MAX_PATH + 10], szTmp2[128], szTmp3[20];
	DWORD dwFileSize;
	PBYTE *pSaveData;
	FILE *fp;
	int x, y;
	png_structp png_ptr;
	png_infop info_ptr;

	ZeroMemory (szTmp, sizeof (szTmp));

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szTmp, MAX_PATH);
	szTmp[strlen (szTmp) - 10] = 0;
	strcat (szTmp, "ss\\sbo");
	wsprintf (szTmp3, "%04d", sysTime.wYear);
	wsprintf (szTmp2, "%s%02d%02d%02d%02d%02d",
			&szTmp3[2],
			sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	strcat (szTmp, szTmp2);
	strcat (szTmp, ".png");

	fp = fopen (szTmp, "wb");
	if (fp == NULL) {
		return;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct (png_ptr);
	png_init_io (png_ptr, fp);
	png_set_IHDR (png_ptr, info_ptr, pDib->Width (), pDib->Width (),
			8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info (png_ptr, info_ptr);


	dwFileSize = pDib->GetImageSize ();
	pSaveData = new PBYTE[pDib->Height ()];
	ZeroMemory (pSaveData, sizeof (PBYTE) * pDib->Height ());

	LPBYTE pData = pDib->GetBits ();
	for (y = 0; y < pDib->Height (); y ++) {
		pData = pDib->GetBits () + y * (pDib->Width () * 4);
		PBYTE pDataTmp = new BYTE[pDib->Width () * 3];
		for (x = 0; x < pDib->Width (); x ++) {
			pDataTmp[x * 3 + 0] = pData[2];
			pDataTmp[x * 3 + 1] = pData[1];
			pDataTmp[x * 3 + 2] = pData[0];
			pData += 4;
		}
		pSaveData[pDib->Height () - 1 - y] = pDataTmp;
	}

	png_write_image (png_ptr, pSaveData);

	png_write_end (png_ptr, info_ptr); 
	png_destroy_write_struct (&png_ptr, &info_ptr); 
	fclose(fp);

	for (y = 0; y < pDib->Height (); y ++) {
		delete [] pSaveData[y];
	}
	delete [] pSaveData;
}

/* ========================================================================= */
/* 関数名：DeleteChar														 */
/* 内容：キャラ削除															 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void DeleteChar(int nNo)
{
	if (g_aCharInfo.GetSize () == 0) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[nNo];

	/* 取り引き中？ */
	if (g_pTradeInfo) {
		/* 取引中の相手がいなくなる？ */
		if (g_pTradeInfo->dwTblID == pChar->dwTblID) {
			/* 取り引き中止 */
			if (g_nMyCharID >= 0) {
				SendCmdTRADE (g_aCharInfo[g_nMyCharID]->dwTblID, CMDTRADE_OUT);
			}
		}
	}

	for (int i = 0; i < g_aRecTbl.GetSize (); i ++) {
		if (g_aRecTbl[i] == pChar->dwTblID) {
			g_aRecTbl.RemoveAt (i);
			break;
		}
	}
	if (g_pMapInfo->pdwMapChar) {
		if ((pChar->dwX <= (DWORD)g_pMapInfo->nCX) && (pChar->dwY <= (DWORD)g_pMapInfo->nCY)) {
			g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pChar->dwY + pChar->dwX] --;
		}
	}
	SAFE_DELETE_ARRAY (pChar->pszSpeech);
	SAFE_DELETE_ARRAY (pChar->pszNpcSpeech);
	SAFE_DELETE (pChar);
	g_aCharInfo.RemoveAt (nNo);
	g_nMyCharID = GetCharID (g_pMyID);
	g_nProcChar = 0;

	/* キャラ座標情報更新 */
	RenewCharPosInfo ();
}

/* ========================================================================= */
/* 関数名：ReleaseData														 */
/* 内容：各データの開放														 */
/* 日付：2003/04/29															 */
/* ========================================================================= */
void ReleaseData(void)
{
	int i;

	if (g_hDll) {
		FreeLibrary (g_hDll);
	}
	if (g_hDllGrpData) {
		FreeLibrary (g_hDllGrpData);
	}

	SAFE_DELETE (g_pTargetSelectInfo);
	SAFE_DELETE (g_pWarSelectTurn);
	SAFE_DELETE (g_pWarInfo);
	SAFE_DELETE (g_pWarMagicTarget);
	SAFE_DELETE_ARRAY (g_pszServer);
	SAFE_DELETE (g_pLeaveInfo);
	SAFE_DELETE (g_pTradeInfo);
	if (g_pSubGameInfoPlay) {
		SAFE_DELETE_ARRAY (g_pSubGameInfoPlay->pInfo);
	}
	SAFE_DELETE (g_pSubGameInfoPlay);
	g_aFadeInOutTbl.RemoveAll ();
	RecvDataHoldClear ();

	if (g_nMyCharID > 0) {
		PCHARINFO pCharMyChar = g_aCharInfo[g_nMyCharID];
		PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pCharMyChar->abySubStatus;
		ZeroMemory (pCharMyChar->abySubStatus, sizeof (pCharMyChar->abySubStatus));
	}

	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		DeleteChar (i);
	}
	g_aCharInfo.RemoveAll ();

	if (g_pMapInfo) {
		if (g_pMapInfo->pShopInfo) {
			for (int j = 0; j < g_pMapInfo->byCountShop; j ++) {
				PSHOPINFO pShop = &g_pMapInfo->pShopInfo[j];
				for (int k = 0; k < MAX_SHOPMSG; k ++) {
					SAFE_DELETE_ARRAY (pShop->apMsg[k]);
				}
			}
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pszName);
		SAFE_DELETE_ARRAY (g_pMapInfo->pShopInfo);
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
		SAFE_DELETE (g_pMapInfo);
	}

	PMAPPARTSINFO pParts;
	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_pMapPartsInfo[i];
		if (pParts->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pParts->pAnimeInfo);
		}
	}
	SAFE_DELETE_ARRAY (g_pMapPartsInfo);

	delete [] g_apDibChar;

	StopBGM ();
	for (i = 0; i < SOUND_MAX; i ++) {
		if (aSoundTbl[i]) {
			DXAudio.Stop (&aSoundTbl[i], 0);
			while (DXAudio.IsPlaying (&aSoundTbl[i])) {
				Sleep (1);
			}
			DXAudio.ReleaseSeg (&aSoundTbl[i]);
		}
	}
	DXAudio.Destroy ();

	for (i = 0; i < g_WndInfo.GetSize (); i ++) {
		EraseWindow (g_WndInfo[i]);
	}
	g_WndInfo.RemoveAll ();

	for (i = 0; i < g_SysMsgInfo.GetSize (); i ++) {
		SAFE_DELETE_ARRAY (g_SysMsgInfo[i]->pszMsg);
		SAFE_DELETE (g_SysMsgInfo[i]);
	}
	g_SysMsgInfo.RemoveAll ();
	g_aRecTbl.RemoveAll ();

	for (i = 0; i < g_AdminRuraInfo.GetSize (); i ++) {
		SAFE_DELETE (g_AdminRuraInfo[i]);
	}
	g_AdminRuraInfo.RemoveAll ();

	for (i = 0; i < g_aUseCharInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aUseCharInfo[i]);
	}
	g_aUseCharInfo.RemoveAll ();

	/* アイテム情報を開放 */
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pData);
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pMsg);
		SAFE_DELETE (g_aItemInfo[i]);
	}
	g_aItemInfo.RemoveAll ();

	/* 吹き出し情報を開放 */
	for (i = 0; i < 99; i ++) {
		PCOMEFCINFO pComefc = &g_pComEfcInfo[i];
		SAFE_DELETE_ARRAY (pComefc->pAnime);
	}

	/* ステータステンプレート情報 */
	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		SAFE_DELETE (g_aStTmpInfo[i]);
	}
	g_aStTmpInfo.RemoveAll ();

	/* オブジェクト情報を開放 */
	for (i = 0; i < g_aObjInfo.GetSize (); i ++) {
		POBJINFO pObj = g_aObjInfo[i];
		if (pObj->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pObj->pAnimeInfo);
		}
	}
	g_aObjInfo.RemoveAll ();
	g_JobInfo.DeleteAll ();

	for (i = 0; i < DRAW_PARTS_Y; i ++) {
		g_aCharPos[i].RemoveAll ();
	}

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
	g_aMagicUserInfo.RemoveAll ();

	g_aMagicInfoWalk.RemoveAll ();
	g_aMagicInfoBattle.RemoveAll ();

	DeleteObject (g_hFontBasic);
	DeleteObject (g_hFont12);
	DeleteObject (g_hFont14);
	DeleteObject (g_hFont32);
	pDibDrawTmp.Destroy ();
}

/* ========================================================================= */
/* 関数名：TimerProc														 */
/* 内容：タイマー処理														 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void TimerProc(void)
{
	int i, nCount;
	DWORD dwTime = timeGetTime ();

	if (g_bDrawStop) {
		return;
	}

	if (g_FadeInOutInfo.dwLastTime == 0) {
		if (!g_bCpuMax) {
			MsgWaitForMultipleObjects (0, NULL, FALSE, 1, QS_ALLINPUT);
		}
		if (g_bMainWindowView == FALSE) {
			MsgWaitForMultipleObjects (0, NULL, FALSE, 1, QS_ALLINPUT);
		}
	}

	if (dwTime > g_dwTimeLastAnime + 50) {
		g_dwTimeLastAnime = dwTime;
		g_byAnimeNo ++;
		g_bDraw = TRUE;

		g_nLoginAnimeX -= 2;
		if (g_nLoginAnimeX < -32) {
			g_nLoginAnimeX = SCRSIZEX + strlen ("Presented By URARA-works 2003-2007") * 6;
		}
	}

	/* メッセージ */
	nCount = g_WndInfo.GetSize ();
	if (nCount) {
		PWNDINFO pWnd = g_WndInfo[g_nProcMsg];
		switch (pWnd->nType) {
		case WNDTYPE_MSG:
			if (pWnd && pWnd->nData2 == 0) {
				if (dwTime > pWnd->dwTime + 25) {
					DWORD j, dwTmp;
					if (pWnd->dwTime == 0) {
						dwTmp = 1;
					} else {
						dwTmp = (dwTime - pWnd->dwTime) / 25;
						dwTmp = max (dwTmp, 1);
					}
					pWnd->dwTime = timeGetTime ();
					if (pWnd->pszMsg) {
						int nLen = strlen (pWnd->pszMsg);
						for (j = 0; j < dwTmp; j ++) {
							if (pWnd->nData1 < nLen) {
								pWnd->nData1 += max (SboTextCharLen (&pWnd->pszMsg[pWnd->nData1]), 1);
								BOOL bRet = MsgProc (pWnd);
								pWnd->dwTimeDrawStart = 0;
								g_bDraw = TRUE;
								if (!bRet) {
									break;
								}

							/* 最後まで処理した */
							} else if (g_nProcShopID < 0) {
								EraseWindow (pWnd);

								/* コマンドウィンドウを閉じる */
								int nNo = GetWindowID (g_dwMsgIDCommand);
								if (nNo >= 0) {
									EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
									g_dwMsgIDCommand = 0;
									g_bDraw = TRUE;
								}
								break;
							}
						}
					}
				}
			} else if (pWnd->nData2 == 1) {
				if (dwTime > pWnd->dwTime + 250) {
					pWnd->dwTime = dwTime;
					pWnd->byAnime = 1 - pWnd->byAnime;
					pWnd->dwTimeDrawStart = 0;
					g_bDraw = TRUE;
				}
			}
			break;

		case WNDTYPE_MSG3SEC:
			if (pWnd->dwTimeDrawStart) {
				/* ３秒経過した？ */
				if (dwTime > pWnd->dwTimeDrawStart + 3000) {
					PostMessage (g_hWndMain, WM_MSG3SEC, (WPARAM)pWnd->dwID, 0);
				}
			}
			break;

		case WNDTYPE_LOGINMENU:
		case WNDTYPE_COMMAND:
		case WNDTYPE_COMMANDETC:
		case WNDTYPE_COMMANDETC_CONF:
		case WNDTYPE_COMMANDITEM:
		case WNDTYPE_COMMANDITEMBATTLE:
		case WNDTYPE_COMMANDITEMPROC:
		case WNDTYPE_COMMANDITEMCHAR:
		case WNDTYPE_COMMANDITEMCHARBATTLE:
		case WNDTYPE_COMMANDOPTION:
		case WNDTYPE_COMMANDOPTIONVOLSOUND:
		case WNDTYPE_COMMANDOPTIONVOLMUSIC:
		case WNDTYPE_COMMANDEQU:
		case WNDTYPE_COMMANDEQUSUB:
		case WNDTYPE_COMMANDEQUSELECT:
		case WNDTYPE_MALEFAMALE:
		case WNDTYPE_DELETECONFIRM:
		case WNDTYPE_NAMECOLOR:
		case WNDTYPE_SHOP:
		case WNDTYPE_SHOPSELL:
		case WNDTYPE_LEAVE:
		case WNDTYPE_COMMANDTRADE:
		case WNDTYPE_TRADE:
		case WNDTYPE_TRADEITEM:
		case WNDTYPE_TRADEGOLD:
		case WNDTYPE_SUBGAME_BOMBSET:
		case WNDTYPE_COMMANDPARTY:
		case WNDTYPE_COMMANDPARTYCHAR:
		case WNDTYPE_CONF:
		case WNDTYPE_JOB:
		case WNDTYPE_COMMANDMAGIC:
		case WNDTYPE_COMMANDMAGICBATTLE:
		case WNDTYPE_SUBWAR_SELECTTEAM:
		case WNDTYPE_SUBWAR_WAIT:
		case WNDTYPE_SUBWAR_SELECTRULE:
		case WNDTYPE_SUBWAR_SELECTJOB:
		case WNDTYPE_SUBWAR_SELECTTURN:
		case WNDTYPE_SUBWAR_TURN:
		case WNDTYPE_SUBWAR_TURN_MOVE:
		case WNDTYPE_SUBWAR_TURN_ATACK:
		case WNDTYPE_SUBWAR_TURN_DIFENCE_FIGHTER:
		case WNDTYPE_SUBWAR_TURN_DIFENCE_MARTIAL:
		case WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH:
		case WNDTYPE_SUBWAR_TURN_DIFENCE_PRIEST:
		case WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH2:
		case WNDTYPE_SUBWAR_TURN_MAGIC_WITCH:
		case WNDTYPE_SUBWAR_TURN_MAGIC_TARGET:
		case WNDTYPE_SUBWAR_TURN_MAGIC_PRIEST:
		case WNDTYPE_ITEMEQU:
		case WNDTYPE_STATUS_ATTR:
		case WNDTYPE_BATTLE_COMMAND:
		case WNDTYPE_BATTLE_TARGET:
		case WNDTYPE_SUBSUGOROKU_JOINMODE:
		case WNDTYPE_SUBSUGOROKU_COMMAND:
		case WNDTYPE_SUBSUGOROKU_IDLE:
		case WNDTYPE_DEAD_COMMAND:
			if (dwTime > pWnd->dwTime + 250) {
				pWnd->dwTime = dwTime;
				if (g_dwActiveMsgID == pWnd->dwID) {
					pWnd->byAnime = 1 - pWnd->byAnime;
					pWnd->dwTimeDrawStart = 0;
					g_bDraw = TRUE;
				}
			}
			break;
		}
		g_nProcMsg ++;
		nCount = g_WndInfo.GetSize ();
		g_nProcMsg = (g_nProcMsg >= nCount) ? 0 : g_nProcMsg;
	}

	/* フェードインかフェードアウト中？ */
	if (g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut > 1) {
		/* フェードアウト中？ */
		if (g_FadeInOutInfo.byFadeOut > 1 && g_FadeInOutInfo.byFadeOut < 255) {
			if (g_FadeInOutInfo.dwLastTime) {
				int nTmp = (int)g_FadeInOutInfo.byFadeOut;
				nTmp -= ((abs (dwTime - g_FadeInOutInfo.dwLastTime)) / 4);
				nTmp = max (1, nTmp);
				g_FadeInOutInfo.byFadeOut = (BYTE)nTmp;
				if (nTmp == 1) {
					g_FadeInOutInfo.dwLastTime = 0;

				} else {
					g_FadeInOutInfo.dwLastTime = timeGetTime ();
				}
				g_bDraw = TRUE;
			} else {
				g_FadeInOutInfo.dwLastTime = timeGetTime ();
			}

		/* フェードイン中？ */
		} else if (g_FadeInOutInfo.byFadeIn && g_FadeInOutInfo.byFadeIn < 255) {
			if (g_FadeInOutInfo.dwLastTime) {
				int nTmp = (int)g_FadeInOutInfo.byFadeIn;
				nTmp += ((abs (dwTime - g_FadeInOutInfo.dwLastTime)) / 4);
				nTmp = min (100, nTmp);
				if (nTmp == 100) {
					g_FadeInOutInfo.byFadeIn = 0;
					g_FadeInOutInfo.dwLastTime = 0;
					if (g_byTextFadeInOut == 1) {
						g_dwTimeTextFadeInOut = timeGetTime ();
					}

				} else {
					g_FadeInOutInfo.byFadeIn = (BYTE)nTmp;
					g_FadeInOutInfo.dwLastTime = timeGetTime ();
				}
				g_bDraw = TRUE;

			} else {
				g_FadeInOutInfo.dwLastTime = timeGetTime ();
			}
		}
	} else {
		if (g_aFadeInOutTbl.GetSize ()) {
			if (g_aFadeInOutTbl[0] == 0) {
				g_FadeInOutInfo.byFadeIn = 0;
				g_FadeInOutInfo.byFadeOut = 100;
			} else {
				g_FadeInOutInfo.byFadeOut = 0;
				g_FadeInOutInfo.byFadeIn = 1;
			}
			g_FadeInOutInfo.dwLastTime = 0;
			g_aFadeInOutTbl.RemoveAt (0);
		}
	}

	/* フェードアウトが終わったら、取っておいた受信データ(次のマップ情報など)を処理する */
	RecvDataHoldProc ();

	/* カーソル */
	if (abs (timeGetTime () - g_dwTimeLastCursor) > 100) {
		g_dwTimeLastCursor = timeGetTime ();
		g_byCursorAnime ++;
		g_bDraw = TRUE;
		if (g_byCursorAnime >= 4) {
			g_byCursorAnime = 0;
		}
	}

	if (g_byGameState < GAMESTATE_MOVE) {
		if (g_byGameState == GAMESTATE_LOGO) {
			if (g_byStage == 0) {
				/* フェードイン完了？ */
				if (g_FadeInOutInfo.byFadeIn == 0) {
					g_byStage = 1;
					g_dwTimeLastLogo = dwTime;
				}
			}
			if (g_byStage == 1) {
				if (dwTime >= g_dwTimeLastLogo + 2000) {
					g_byStage = 2;
					g_dwTimeLastLogo = 0;
					g_FadeInOutInfo.byFadeOut = 100;
					g_FadeInOutInfo.byFadeIn = 0;
					g_FadeInOutInfo.dwLastTime = timeGetTime ();
				}
			}
			if (g_byStage == 2) {
				if (g_FadeInOutInfo.byFadeOut <= 2) {
					ChgState (GAMESTATE_TITLE);
				}
			}
		} else if (g_byGameState == GAMESTATE_TITLE) {
			if ((g_byStage == 1) && (g_FadeInOutInfo.byFadeOut <= 2)) {
				ChgState (GAMESTATE_LOGIN);
			}
		}
		if (!g_bCpuMax) {
			/* CPU負荷下げるためにちょっと休憩 */
			MsgWaitForMultipleObjects (0, NULL, FALSE, 1, QS_ALLINPUT);
		}

		/* 歩き中以外の時はこれ以下の処理をしない */
		return;
	}

	/* マップ名表示 */
	if (g_dwTimeTextFadeInOut) {
		if (timeGetTime () > g_dwTimeTextFadeInOut + 100) {
			g_dwTimeTextFadeInOut = timeGetTime ();
			g_byTextFadeInOut ++;
			if (g_byTextFadeInOut > 41) {
				g_byTextFadeInOut = 0;
				g_dwTimeTextFadeInOut = 0;
			}
		}
	}

	/* キャラ */
	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		g_nProcChar = i;
		PCHARINFO pChar = g_aCharInfo[g_nProcChar];
		TimerProcChar (pChar);
		nCount = g_aCharInfo.GetSize ();
	}

	/* システムメッセージ */
	nCount = g_SysMsgInfo.GetSize ();
	if (nCount) {
		PSYSMSGINFO pSysMsgInfo = g_SysMsgInfo[g_nProcSysMsg];
		if (dwTime > pSysMsgInfo->dwTime + 70) {
			DWORD dwTmp;

			dwTmp = (dwTime - pSysMsgInfo->dwTime) / 70;
			pSysMsgInfo->dwTime = dwTime;
			pSysMsgInfo->nY -= (int)dwTmp;

			if (g_nProcSysMsg == nCount - 1) {
				/* 再描画しすぎるので、更新タイミングは最後のメッセージだけにする */
				g_bDraw = TRUE;
			}

			/* 上段まで表示した？ */
			if (pSysMsgInfo->nY < SYSMSG_EY) {
				SAFE_DELETE_ARRAY (pSysMsgInfo->pszMsg);
				SAFE_DELETE (pSysMsgInfo);
				g_SysMsgInfo.RemoveAt (g_nProcSysMsg);
				nCount = g_SysMsgInfo.GetSize ();
				g_nProcSysMsg = -1;
			}
		}

		g_nProcSysMsg ++;
		g_nProcSysMsg = (g_nProcSysMsg >= nCount) ? 0 : g_nProcSysMsg;
	}

	/* マップパーツ */
	if (g_wCountMapParts) {
		if (g_dwTimeLastMapParts == 0) {
			g_dwTimeLastMapParts = timeGetTime ();
		}
		DWORD dwTimeTmp = timeGetTime ();
		if (dwTimeTmp >= g_dwTimeLastMapParts + 10) {
			int j;
			dwTimeTmp = g_dwTimeLastMapParts + 10;

			for (j = 0; j < g_wCountMapParts; j ++) {
				PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[j];

				if (pParts->byAnimeCount) {
					PANIMEINFO pAnime = &pParts->pAnimeInfo[pParts->byAnimeNo];
					if (pAnime && dwTimeTmp >= pParts->dwLastAnime + ((DWORD)pAnime->byWait * 10)) {
						pParts->byAnimeNo ++;
						pParts->byAnimeNo = (pParts->byAnimeNo >= pParts->byAnimeCount) ? 0 : pParts->byAnimeNo;
						pParts->dwLastAnime = dwTimeTmp;
						g_bDraw = TRUE;
					}
				}
			}
			g_dwTimeLastMapParts = dwTimeTmp;
		}
	}

	/* ターゲット選択情報 */
	if (g_pTargetSelectInfo) {
		if (abs (g_pTargetSelectInfo->dwLastTime - dwTime) > 100) {
			g_pTargetSelectInfo->dwLastTime = timeGetTime ();
			g_pTargetSelectInfo->byAnime ++;
			if (g_pTargetSelectInfo->byAnime >= 4) {
				g_pTargetSelectInfo->byAnime = 0;
			}
			g_bDraw = TRUE;
		}
	}

	/* オブジェクト */
	TimerProcObj ();

	/* 移動禁止タイマー発動中？ */
	if (g_dwTimeMoveDisable) {
		if (dwTime > g_dwTimeMoveDisable) {
			g_dwTimeMoveDisable = 0;
		}
	}
}

/* ========================================================================= */
/* 関数名：TimerProcObj														 */
/* 内容：タイマー処理(オブジェクト)											 */
/* 日付：2004/01/16															 */
/* ========================================================================= */
void TimerProcObj(void)
{
	int i, nCount;
	DWORD dwTime = timeGetTime ();

	nCount = g_aObjInfo.GetSize ();
	if (nCount == 0) {
		return;
	}

	for (i = 0; i < nCount; i ++) {
		POBJINFO pObj = g_aObjInfo[i];

		switch (pObj->byTypeMain) {
		case OBJTYPE_FOOTMARK_SNOW:		/* 足跡(雪) */
		case OBJTYPE_FOOTMARK_SAND:		/* 足跡(砂) */
			if (dwTime > pObj->dwLastAnime + 500) {
				int nTmp;
				/* 消滅？ */
				if (pObj->byLevel >= 100) {
					SAFE_DELETE (pObj);
					g_aObjInfo.RemoveAt (i);
					nCount = g_aObjInfo.GetSize ();
					i = -1;
					break;
				}
				if (pObj->dwLastAnime > 0) {
					nTmp = abs (dwTime - pObj->dwLastAnime) / 500 * 10;
					if (nTmp + pObj->byLevel > 100) {
						nTmp = 100 - pObj->byLevel;
					}
					pObj->byLevel += (BYTE)nTmp;
				}
				pObj->dwLastAnime = dwTime;
			}
			break;

		case OBJTYPE_WATERRIPPLE:		/* 水の波紋 */
			if (dwTime > pObj->dwLastAnime + 100) {
				/* 消滅？ */
				if (pObj->byAnimeNo >= 10) {
					SAFE_DELETE (pObj);
					g_aObjInfo.RemoveAt (i);
					nCount = g_aObjInfo.GetSize ();
					i = -1;
					break;
				}
				if (pObj->dwLastAnime > 0) {
					pObj->byAnimeNo ++;
				}
				pObj->dwLastAnime = dwTime;
			}
			break;
		}
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcChar													 */
/* 内容：	タイマー処理(キャラ)											 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcChar(PCHARINFO pChar)
{
	DWORD dwTime = timeGetTime ();

	TimerProcCharDotMove	(pChar);		/* 移動処理 */
	TimerProcCharEfect		(pChar);		/* エフェクト処理 */
	TimerProcCharEfectIdle	(pChar);		/* エフェクト処理(アイドル中) */
	TimerProcCharComEfc		(pChar);		/* 吹き出し処理 */
	TimerProcCharAnime		(pChar);		/* アニメーション処理 */
	TimerProcCharHartBeat	(pChar);		/* キャラハートビート処理 */
	TimerProcCharTurnLV		(pChar);		/* ターンゲージ処理 */
	TimerProcCharMagicLV	(pChar);		/* 魔法ターンゲージ処理 */

	/* 発言内容のクリア判定 */
	if (pChar->dwLastSpeech) {
		if (dwTime > pChar->dwLastSpeech + 10000) {
			pChar->dwLastSpeech = 0;
			SAFE_DELETE_ARRAY (pChar->pszSpeech);
			g_bDraw = TRUE;
		}
	}

	/* 文字エフェクト */
	PCHAREFCINFO pCharEfc = &pChar->aCharEfcInfo[pChar->byCharEfcProc];
	if (pCharEfc->szMsg[0] != 0) {
		if (dwTime >= pCharEfc->dwLastProc + 75) {
			pCharEfc->dwLastProc = dwTime;
			pCharEfc->chY += achCharEfcAddTbl[pCharEfc->byStage];
			/* 終わり？ */
			if (achCharEfcAddTbl[pCharEfc->byStage] == 99) {
				ZeroMemory (pCharEfc, sizeof (CHAREFCINFO));
			} else {
				pCharEfc->byStage ++;
			}
			g_bDraw = TRUE;
		}
	}
	pChar->byCharEfcProc ++;
	if (pChar->byCharEfcProc >= MAX_COUNTCHAREFC) {
		pChar->byCharEfcProc = 0;
	}

	/* 削除中 */
	if (pChar->dwTimeDelete) {
		switch (pChar->byNpcType) {
		case NPCTYPE_INIT:			/* 未設定 */
		case NPCTYPE_NOMOVE:		/* 移動しない */
		case NPCTYPE_MOVERAND:		/* ランダムに移動 */
		case NPCTYPE_BATTLE1:		/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
		case NPCTYPE_SEARCH:		/* 周り２キャラの範囲で検索してついていく */
		case NPCTYPE_BATTLE2:		/* 周り２キャラの範囲で検索してついていって攻撃 */
		case NPCTYPE_NOBLOCK:		/* 移動しない(通り抜け可) */
			break;
		default:
			pChar->dwTimeDelete = 0;
			break;
		}
		int nTmp = abs (dwTime - pChar->dwTimeDelete);
		if (nTmp >= 1500) {
			DeleteChar (g_nProcChar);
			g_nProcChar = 0;
			return;
		}
		nTmp = min (100, nTmp / 15);
		pChar->byFadeLevel = (BYTE)nTmp;
		g_bDraw = TRUE;

	/* 表示開始中 */
	} else if (pChar->dwTimeAdd) {
		switch (pChar->byNpcType) {
		case NPCTYPE_INIT:			/* 未設定 */
		case NPCTYPE_NOMOVE:		/* 移動しない */
		case NPCTYPE_MOVERAND:		/* ランダムに移動 */
		case NPCTYPE_BATTLE1:		/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
		case NPCTYPE_SEARCH:		/* 周り２キャラの範囲で検索してついていく */
		case NPCTYPE_BATTLE2:		/* 周り２キャラの範囲で検索してついていって攻撃 */
		case NPCTYPE_NOBLOCK:		/* 移動しない(通り抜け可) */
			break;
		default:
			pChar->dwTimeAdd = 0;
			break;
		}
		int nTmp = abs (dwTime - pChar->dwTimeAdd);
		if (nTmp >= 1500) {
			pChar->dwTimeAdd = 0;
			pChar->byFadeLevel = 0;

		} else {
			nTmp = max (0, 100 - (nTmp / 15));
			if (pChar->byFadeLevel != (BYTE)nTmp) {
				pChar->byFadeLevel = (BYTE)nTmp;
				g_bDraw = TRUE;
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharDotMove											 */
/* 内容：	タイマー処理(キャラ移動処理)									 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharDotMove(PCHARINFO pChar)
{
	int nTime1, nTime2, nTmp;

	if (pChar->byDotMove == 0) {
		return;
	}

	/* 歩き始めに先に進めておく時間(ms)。一歩目を受け付けてから動き出すまでの遅れを埋める */
	nTime1 = 27;
	/* 1ドット進むのにかける時間(ms)。1マス(31ドット)はおよそ 31 × nTime2 ms で歩く */
	nTime2 = 9;
	/* 投げた雪玉は歩くより速く飛ぶ（サーバーは 1 マス 180ms で進める。CharProcATACK.cpp） */
	if ((pChar->byNpcType == NPCTYPE_ATACK) && (pChar->wGrpID == 213)) {
		nTime2 = 5;
	}
	if (pChar->dwLastDotMove == 0) {
		if (g_nProcChar == g_nMyCharID) {
			if (g_bEnableMove) {
				pChar->dwLastDotMove = timeGetTime () - nTime1;
			}
		} else {
			pChar->dwLastDotMove = timeGetTime () - nTime1;
		}
	}

	/* 以前は nTime1(27ms) ごとにまとめて進めていたが、描画のタイミングとずれて
	   ガクガクして見えるため、1ドット分の時間が過ぎていれば毎回進める */
	if (timeGetTime () >= pChar->dwLastDotMove + nTime2) {
		/* 前回移動時から過ぎた分を一気に移動させる */
		nTmp = (timeGetTime () - pChar->dwLastDotMove) / nTime2;
		pChar->byDotMove		= (BYTE)max ((int)pChar->byDotMove - nTmp, 0);
		/* 端数の時間は次回に持ち越す（切り捨てると画面の更新間隔で速さが変わってしまうため） */
		pChar->dwLastDotMove	+= nTmp * nTime2;
		g_bDraw = TRUE;
	}

	/* 歩ききった？ */
	if ((g_nProcChar == g_nMyCharID) && !pChar->byDotMove && g_bEnableMove) {
		BYTE byHitType;

		g_bMoving = FALSE;
		if (g_pMapInfo->pbyFlg[pChar->dwX + g_pMapInfo->nCX * pChar->dwY] & MAPFLG_EVENT) {
			/* サーバーにイベント問い合わせ */
			SendCmdEVENTCHECK (0);
			g_bDraw = TRUE;
		} else {
			BOOL bMove;

			bMove = TRUE;
			byHitType = (&g_pMapPartsInfo[g_pMapInfo->pwMap[pChar->dwX + pChar->dwY * g_pMapInfo->nCX]])->byHitType;
			switch (byHitType) {
			case PARTSHIT_MOVEFRONT:		/* 一歩前へ進む */
				break;
			case PARTSHIT_MOVEUP:			/* 一歩上へ進む */
				pChar->byDirection = 0;
				break;
			case PARTSHIT_MOVEDOWN:			/* 一歩下へ進む */
				pChar->byDirection = 1;
				break;
			case PARTSHIT_MOVERIGHT:		/* 一歩左へ進む */
				pChar->byDirection = 2;
				break;
			case PARTSHIT_MOVELEFT:			/* 一歩右へ進む */
				pChar->byDirection = 3;
				break;
			default:
				bMove = FALSE;
				break;
			}
			if (bMove) {
				SendCmdCHARPOS (pChar->byDirection, FALSE);
			} else {
				/* 方向キーが押されたままなら、待たずに次の一歩を出す */
				KeyProcMoveNext ();
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharEfect												 */
/* 内容：	タイマー処理(キャラエフェクト処理)								 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharEfect(PCHARINFO pChar)
{
	int nCount;
	BYTE byID;
	PEFCANIMEINFO pAnime;
	DWORD dwTime;

	if ((pChar->byEfect == 0) || (pChar->dwTimeEfc == 0)) {
		return;
	}

	dwTime	= timeGetTime ();
	byID	= pChar->byEfect - 1;
	pAnime	= g_EfcInfo.GetPtrAnime (byID, pChar->byEfcAnime);

	if ((pAnime == NULL) || (dwTime - pChar->dwTimeEfc < (DWORD)(pAnime->byWait * 10))) {
		return;
	}

	pChar->byEfcAnime ++;
	nCount = g_EfcInfo.GetCountAnime (byID);
	if (pChar->byEfcAnime >= nCount) {
		switch (pChar->byNpcType) {
		case NPCTYPE_USEMAGIC:			/* NPC属性が重なったキャラに魔法発動 */
		case NPCTYPE_USEMAGICPARENT:	/* 魔法発動し続ける */
		case NPCTYPE_ATACK:				/* NPC属性が重なったキャラに攻撃 */
			/* エフェクト処理し続ける */
			pChar->dwTimeEfc = dwTime;
			break;

		default:
			pChar->byEfect	 = 0;
			pChar->dwTimeEfc = 0;
			break;
		}
		pChar->byEfcAnime = 0;

	} else {
		pChar->dwTimeEfc = dwTime;
	}
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：	TimerProcCharEfectIdle											 */
/* 内容：	タイマー処理(キャラエフェクト処理(アイドル中))					 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharEfectIdle(PCHARINFO pChar)
{
	int nCount;
	BYTE byID;
	PEFCANIMEINFO pAnime;
	DWORD dwTime;

	if ((pChar->byEfect > 0) || (pChar->byEfectIdle == 0)) {
		return;
	}

	dwTime	= timeGetTime ();
	byID	= pChar->byEfectIdle - 1;
	pAnime	= g_EfcInfo.GetPtrAnime (byID, pChar->byEfcAnime);

	if ((pAnime == NULL) || (dwTime - pChar->dwTimeEfc < (DWORD)(pAnime->byWait * 10))) {
		return;
	}

	pChar->byEfcAnime ++;
	nCount = g_EfcInfo.GetCountAnime (byID);
	if (pChar->byEfcAnime >= nCount) {
		/* エフェクト処理し続ける */
		pChar->dwTimeEfc	= dwTime;
		pChar->byEfcAnime	= 0;

	} else {
		pChar->dwTimeEfc = dwTime;
	}
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：	TimerProcCharComEfc												 */
/* 内容：	タイマー処理(キャラ吹き出し処理)								 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharComEfc(PCHARINFO pChar)
{
	DWORD dwTime;
	PCOMEFCINFO pComEfc;
	PCOMEFCANIMEINFO pAnime;

	if (pChar->byComEfc == 0) {
		return;
	}

	dwTime	= timeGetTime ();
	/* 敵の攻撃の予兆の「!」は、攻撃までの間(TELL_VIEWTIME)だけ出して消す */
	if ((pChar->byComEfc == COMEFC_TELL) &&
		((pChar->byNpcType == NPCTYPE_BATTLE1) || (pChar->byNpcType == NPCTYPE_BATTLE2))) {
		if (dwTime - pChar->dwTimeComEfc >= TELL_VIEWTIME) {
			pChar->byComEfc			= 0;
			pChar->byComEfcAnime	= 0;
			pChar->dwTimeComEfc		= 0;
			g_bDraw = TRUE;
		}
		return;
	}
	pComEfc	= &g_pComEfcInfo[pChar->byComEfc - 1];
	pAnime	= &pComEfc->pAnime[pChar->byComEfcAnime];

	if ((pAnime == NULL) || (dwTime - pChar->dwTimeComEfc < (DWORD)(pAnime->byWait * 100))) {
		return;
	}

	pChar->byComEfcAnime ++;
	if (pChar->byComEfcAnime >= pComEfc->byAnimeCount) {
		pChar->byComEfc			= 0;
		pChar->byComEfcAnime	= 0;
		pChar->dwTimeComEfc		= 0;

	} else {
		pChar->dwTimeComEfc = dwTime;
	}
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：	TimerProcCharAnime												 */
/* 内容：	タイマー処理(キャラアニメーション処理)							 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharAnime(PCHARINFO pChar)
{
	DWORD dwTime;

	dwTime = timeGetTime ();
	switch (pChar->byNpcType) {
	case NPCTYPE_BOMB:							/* 爆弾 */
		TimerProcCharAnimeBOMB (pChar);
		break;

	case NPCTYPE_BOMBBLOCK:						/* 爆弾ゲームの壁 */
		TimerProcCharAnimeBOMBBLOCK (pChar);
		break;

	default:
		if (dwTime - pChar->dwLastAnime < 300) {
			break;
		}
		pChar->dwLastAnime	= dwTime;
		pChar->byAnime		= 1 - pChar->byAnime;
		g_bDraw = TRUE;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharAnimeBOMB											 */
/* 内容：	タイマー処理(キャラアニメーション処理(爆弾))					 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharAnimeBOMB(PCHARINFO pChar)
{
	DWORD dwTime;

	dwTime = timeGetTime ();
	switch (pChar->byEfect) {
	case 0:		/* 爆発前 */
		if (dwTime - pChar->dwLastAnime < 300) {
			break;
		}
		pChar->byAnime ++;
		if (pChar->byAnime >= 4) {
			pChar->byAnime = 0;
		}
		pChar->dwLastAnime = dwTime;
		g_bDraw = TRUE;
		break;

	default:	/* 爆発中 */
		if (dwTime - pChar->dwLastAnime < 100) {
			break;
		}
		pChar->byEfect ++;
		/* 消滅？ */
		if (pChar->byEfect >= 8) {
			pChar->dwTimeDelete = 1;
			break;
		}
		pChar->dwLastAnime = dwTime;
		g_bDraw = TRUE;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharAnimeBOMBBLOCK										 */
/* 内容：	タイマー処理(キャラアニメーション処理(爆弾ゲームの壁))			 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharAnimeBOMBBLOCK(PCHARINFO pChar)
{
	DWORD dwTime;

	dwTime = timeGetTime ();
	switch (pChar->byEfect) {
	case 0:
		break;
	case 1:			/* 溶け始め */
		if (dwTime - pChar->dwLastAnime < 1000) {
			break;
		}
		pChar->byEfect ++;
		pChar->dwLastAnime = dwTime;
		g_bDraw = TRUE;
		break;

	default:		/* 溶け中 */
		if (dwTime - pChar->dwLastAnime < 250) {
			break;
		}
		pChar->byEfect ++;
		/* 消滅？ */
		if (pChar->byEfect >= 7) {
			pChar->dwTimeDelete = 1;
			return;
		}
		pChar->dwLastAnime = dwTime;
		g_bDraw = TRUE;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharHartBeat											 */
/* 内容：	タイマー処理(キャラハートビート処理)							 */
/* 日付：	2006/01/09														 */
/* ========================================================================= */

void TimerProcCharHartBeat(PCHARINFO pChar)
{
	DWORD dwTime;
	SYSTEMTIME sysTime;

	dwTime = timeGetTime ();

	if (g_nProcChar != g_nMyCharID) {
		return;
	}

	g_dwTimeLastProc = timeGetTime ();
	if (dwTime - pChar->dwLastHartBeat < SENDHARTBEATTIME) {
		return;
	}
	GetLocalTime (&sysTime);

	if (pChar->dwLastHartBeat) {
		SendCmdHARTBEAT ();
		/* デバッグモードで動作中？ */
		if (g_proc && g_proc ()) {
			/* 強制終了 */
			PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
			return;
		}
	}

	pChar->dwLastHartBeat = timeGetTime ();
	if (g_wLastProcSecond != 0xFFFF) {
		WORD wLastProcSecondBack, wTmp;

		wLastProcSecondBack = g_wLastProcSecond;
		wTmp = sysTime.wSecond;
		if (wLastProcSecondBack > wTmp) {
			wTmp += 60;
		}

		if (abs (wLastProcSecondBack - wTmp) * 1000 < SENDHARTBEATTIME - 1000) {
			if (g_bHartbeatCheck == FALSE) {
				g_bHartbeatCheck = TRUE;
			} else {
				PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
				return;
			}
		} else {
			g_bHartbeatCheck = FALSE;
		}
	}
	g_wLastProcSecond = sysTime.wSecond;
}


/* ========================================================================= */
/* 関数名：	TimerProcAutoTurnGage											 */
/* 内容：	戦闘モードで目の前に敵がいれば、自動でゲージを溜め始める		 */
/*			（コマンドから攻撃→相手を選ばなくても殴れるように）			 */
/*			向きを変えて別の敵が目の前に来たら相手だけ切り替え、			 */
/*			目の前から敵がいなくなって AUTOGAGE_LOSTTIME たったら消す		 */
/*			弓は離れた敵を狙うので、今までどおりコマンドから相手を選ぶ		 */
/* ========================================================================= */

static void TimerProcAutoTurnGage(PCHARINFO pChar, DWORD dwTime)
{
	int nFront;
	DWORD dwFront;

	if ((g_bBattleMode == FALSE) || (g_pMapInfo == NULL) || ((g_pMapInfo->byFlg & MAPFLG_BATTLE) == 0)) {
		return;
	}
	if (g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut) {
		return;
	}
	/* 倒れている・特技の詠唱中・構えている？ */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || pChar->dwTimeTurnMagicLV || g_bGuard) {
		return;
	}
	/* 相手を選んで溜めているゲージ・弓はそのまま */
	if ((pChar->dwTimeTurnLV && (g_bAutoGage == FALSE)) || (GetAttackType (pChar) == ATACKTYPE_ARROW)) {
		return;
	}

	nFront = FindFrontAttackTarget (pChar);
	if (nFront == -1) {
		g_dwAutoGageSuppress = (DWORD)-1;
		/* 目の前から敵がいなくなってしばらくたったらゲージを消す(一瞬ずれただけでは消さない) */
		if (pChar->dwTimeTurnLV && (dwTime - g_dwAutoGageLastFront > AUTOGAGE_LOSTTIME)) {
			CancelTurnGage (pChar);
			g_bAutoGage = FALSE;
			SendCmdTARGETCHAR ((DWORD)-1);
		}
		return;
	}
	dwFront = g_aCharInfo[nFront]->dwTblID;
	g_dwAutoGageLastFront = dwTime;

	if (pChar->dwTimeTurnLV == 0) {
		/* Z でやめた相手のまま？ */
		if (dwFront == g_dwAutoGageSuppress) {
			return;
		}
		g_dwAutoGageSuppress = (DWORD)-1;
		StartTurnGage (pChar, dwFront);
		g_bAutoGage = TRUE;
		SendCmdTARGETCHAR (dwFront);
		g_bDraw = TRUE;

	} else if (pChar->dwAtackTblID != dwFront) {
		/* 向きを変えて別の敵が目の前に来た：ゲージはそのままで相手だけ切り替え */
		pChar->dwAtackTblID = dwFront;
		SendCmdTARGETCHAR (dwFront);
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharTurnLV												 */
/* 内容：	タイマー処理(キャラターンゲージ処理)							 */
/* 日付：	2006/02/14														 */
/* ========================================================================= */

void TimerProcCharTurnLV(PCHARINFO pChar)
{
	int nTmp;
	DWORD dwTime, dwTmp;
	PCHARINFO pCharTarget;

	dwTime = timeGetTime ();
	/* 戦闘モードで目の前に敵がいれば、相手を選ばなくてもゲージを溜める */
	if ((g_nMyCharID >= 0) && (pChar == g_aCharInfo[g_nMyCharID])) {
		TimerProcAutoTurnGage (pChar, dwTime);
	}
	/* ターンゲージ溜まり中 */
	if (pChar->dwTimeTurnLV == 0) {
		return;
	}
	/* 死んでいる？(倒れた後にゲージが溜まっても攻撃しない) */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		CancelTurnGage (pChar);
		return;
	}
	/* 構えている間はゲージを止める(離すと続きから溜まる) */
	if (g_bGuard && (pChar == g_aCharInfo[g_nMyCharID])) {
		pChar->dwTimeTurnLV = dwTime - g_dwGuardGageElapse;
		return;
	}
	/* 特技の詠唱中・戦闘モードを解除した？(連続攻撃もここで止める) */
	if (pChar->dwTimeTurnMagicLV || (g_bBattleMode == FALSE) ||
		(g_pMapInfo == NULL) || ((g_pMapInfo->byFlg & MAPFLG_BATTLE) == 0)) {
		CancelTurnGage (pChar);
		return;
	}

	if (!(g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut)) {
		/* 攻撃タイミングになっているかを計算。最大3秒で255を100%とする。 */
		dwTmp = GetTurnGageTime (pChar);

		if (dwTime >= pChar->dwTimeTurnLV + dwTmp / 100) {
			pChar->dwLastTurnLV = dwTime;
			pChar->byTurnLV = (BYTE)((dwTime - pChar->dwTimeTurnLV) * 10000 / dwTmp / 100);
			pChar->byTurnLV = min (pChar->byTurnLV, 100);
			g_bDraw = TRUE;
			/* 満タン？ */
			if (pChar->byTurnLV == 100) {
				/* 満タン直後はジャストで押せる時間なので、自動では殴らずに待つ */
				if (IsTurnGageJust (pChar)) {
					return;
				}
				/* 移動中ではない？ */
				if (pChar->byDotMove == 0) {
					/* 弓など、相手を決めずに撃つ？ */
					if (pChar->dwAtackTblID == (DWORD)-1) {
						/* 攻撃要求 */
						SendCmdREQ_ATTACK (-1, pChar->byTurnLV);
						CancelTurnGage (pChar);
						return;
					}
					/* 目の前に敵がいれば、選んだ相手でなくてもそちらを殴る(連続攻撃) */
					nTmp = FindFrontAttackTarget (pChar);
					if (nTmp != -1) {
						if (g_aCharInfo[nTmp]->dwTblID != pChar->dwAtackTblID) {
							pChar->dwAtackTblID = g_aCharInfo[nTmp]->dwTblID;
							SendCmdTARGETCHAR (pChar->dwAtackTblID);
						}
						nTmp = 1;
					} else {
						nTmp = GetCharNo2 (pChar->dwAtackTblID);
						/* 相手が倒れて消えた？(周りの次の敵に切り替える。向きを変えれば殴れる) */
						if ((nTmp == -1) || (g_aCharInfo[nTmp]->byCharState == CHARSTATE_DEAD)) {
							nTmp = FindNextAttackTarget (pChar);
							if (nTmp == -1) {
								CancelTurnGage (pChar);
								return;
							}
							pChar->dwAtackTblID = g_aCharInfo[nTmp]->dwTblID;
							SendCmdTARGETCHAR (pChar->dwAtackTblID);
						}
						pCharTarget = g_aCharInfo[nTmp];
						/* 相手から離れすぎた？(逃げたら連続攻撃をやめる) */
						if ((pCharTarget->dwMapID != pChar->dwMapID) ||
							(abs ((int)pCharTarget->dwX - (int)pChar->dwX) + abs ((int)pCharTarget->dwY - (int)pChar->dwY) > 5)) {
							CancelTurnGage (pChar);
							SendCmdTARGETCHAR ((DWORD)-1);
							return;
						}
						nTmp = GetCharDistance (pCharTarget, pChar->dwMapID, pChar->dwX, pChar->dwY, pChar->byDirection);
					}
					if (nTmp == 1) {
						/* 攻撃要求 */
						SendCmdREQ_ATTACK (pChar->dwAtackTblID, pChar->byTurnLV);
						/* 連続攻撃：同じ相手に向けて次のゲージを溜め始める */
						ContinueTurnGage (pChar);
					}
					//Todo:遠距離攻撃用の処理
				}
			}
		}

	} else {
		pChar->byTurnLV = 1;
		pChar->dwTimeTurnLV = timeGetTime ();
		pChar->dwLastTurnLV = pChar->dwTimeTurnLV;
	}
}


/* ========================================================================= */
/* 関数名：	TimerProcCharMagicLV											 */
/* 内容：	タイマー処理(キャラ魔法ターンゲージ処理)						 */
/* 日付：	2006/01/10														 */
/* ========================================================================= */

void TimerProcCharMagicLV(PCHARINFO pChar)
{
	DWORD dwTime;
	PMAGICINFO pInfo;
	DWORD dwTmp, dwTblID;

	dwTime = timeGetTime ();
	/* 魔法ターンゲージ溜まり中 */
	if ((pChar->dwTimeTurnMagicLV == 0) || (pChar->byTurnMagicLV >= 100)) {
		return;
	}
	/* 死んでいる？(倒れた後に詠唱が終わっても特技は出さない) */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		pChar->dwTimeTurnMagicLV	= 0;
		pChar->byTurnMagicLV		= 0;
		CancelTurnGage (pChar);
		SAFE_DELETE (g_pTargetSelectInfo);
		return;
	}

	if (!(g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut)) {
		pInfo = g_aMagicInfo[g_byComSelectMagic];
		dwTmp = 1;
		dwTmp = 1000 + ((pInfo->byTime * 1000) - ((pInfo->byTime * 1000) * dwTmp) / 100);

		if (dwTime >= pChar->dwLastTurnMagicLV + dwTmp / 100) {
			pChar->dwLastTurnMagicLV = dwTime;
			pChar->byTurnMagicLV = (BYTE)((dwTime - pChar->dwTimeTurnMagicLV) * 10000 / dwTmp / 100);
			pChar->byTurnMagicLV = min (pChar->byTurnMagicLV, 100);
			/* 満タン？ */
			if (pChar->byTurnMagicLV == 100) {
				/* Manaチェック */
				if (pChar->wMana < pInfo->wMana) {
					AddSysMsg ("Manaが足りません。", RGB (255, 255, 255));

				} else {
					/* 発動 */
					dwTblID = pChar->dwTblID;
					if (g_pTargetSelectInfo) {
						dwTblID = g_pTargetSelectInfo->dwTblID;
					}
					SendCmdEFECTCHG (0, TRUE);
					SendCMDUSEMAGIC (g_byComSelectMagic, dwTblID);
					SAFE_DELETE (g_pTargetSelectInfo);
				}
				pChar->dwTimeTurnMagicLV	= 0;
				pChar->byTurnMagicLV		= 0;
				CancelTurnGage (pChar);
			}
			g_bDraw = TRUE;
		}

	} else {
		pChar->byTurnMagicLV = 1;
		pChar->dwTimeTurnMagicLV = timeGetTime ();
		pChar->dwLastTurnMagicLV = pChar->dwTimeTurnMagicLV;
	}
}


/* ========================================================================= */
/* 関数名：StartPosConfirmOpen												 */
/* 内容：教会などの「@8」で、復活位置を記録するか「はい・いいえ」で聞く		 */
/*		 メッセージを「(ここまでのセリフ)@1復活位置を記録しますか？@k(続き)」 */
/*		 に組み替え、続きは「はい」を選ぶまで止めておく						 */
/*		 nPosAt：元のメッセージでの "@8" の '@' の位置						 */
/* ========================================================================= */
void StartPosConfirmOpen(PWNDINFO pWnd, int nPosAt, int nMapID, int nX, int nY)
{
	static const char *pszAsk = "@1復活位置を記録しますか？@k";
	LPCSTR pszTail = "";
	if ((int)strlen (pWnd->pszMsg) >= nPosAt + 11) {
		pszTail = &pWnd->pszMsg[nPosAt + 11];		/* "@8" + 9 桁の後ろ */
	}

	/* 続きの先頭の入力待ちは、選んだ後に余計なので外す */
	if ((pszTail[0] == '@') && (pszTail[1] == '0')) {
		pszTail += 2;
	}

	int nLenHead = nPosAt;
	int nLenAsk  = (int)strlen (pszAsk);
	int nLenTail = (int)strlen (pszTail);
	LPSTR pszNew = new char[nLenHead + nLenAsk + nLenTail + 1];
	CopyMemory (pszNew, pWnd->pszMsg, nLenHead);
	CopyMemory (&pszNew[nLenHead], pszAsk, nLenAsk);
	CopyMemory (&pszNew[nLenHead + nLenAsk], pszTail, nLenTail);
	pszNew[nLenHead + nLenAsk + nLenTail] = 0;

	SAFE_DELETE_ARRAY (pWnd->pszMsg);
	pWnd->pszMsg = pszNew;
	/* 「@k」の 'k' まで処理済みにする（問いかけまで一度に出す） */
	pWnd->nData1 = nLenHead + nLenAsk - 1;
	pWnd->nData2 = MSGSTATE_STARTPOSCONF;
	pWnd->dwTimeDrawStart = 0;
	/* 待っている間は dwTime を使わないので、確認を出した時刻を入れておく */
	pWnd->dwTime = timeGetTime ();

	/* はい・いいえのウィンドウ（メッセージ欄の右上に出す） */
	PWNDINFO pWndConf = MakeWindowCONF (pWnd->nX + pWnd->nCx - 16 * 6, pWnd->nY - 16 * 4, IDCONFIRMTYPE_STARTPOS);
	pWndConf->nData1 = 0;
	pWndConf->nData2 = ((nMapID & 0xFF) << 16) | ((nX & 0xFF) << 8) | (nY & 0xFF);
	pWndConf->pParent = pWnd;
	pWnd->pChild = pWndConf;
	g_WndInfo.Add (pWndConf);
	g_dwActiveMsgID = pWndConf->dwID;
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：MsgProc															 */
/* 内容：メッセージ処理														 */
/* 日付：2003/06/11															 */
/* ========================================================================= */
BOOL MsgProc(PWNDINFO pWnd)
{
	BOOL bRet = TRUE;

	/* 制御コードじゃない？ */
	if (pWnd->pszMsg[pWnd->nData1] != '@') {
		/* 音出す指定？ */
		if (pWnd->byData1 && pWnd->pszMsg[pWnd->nData1] != ' ') {
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_MSG]);
		}
		return TRUE;
	}

	pWnd->nData1 ++;
	switch (pWnd->pszMsg[pWnd->nData1]) {
	/* 入力待ち */
	case '0':
		pWnd->nData2 = 1;
		pWnd->nData1 ++;
		bRet = FALSE;
		break;

	/* 点滅無しで入力待ち */
	case '2':
		pWnd->nData2 = 2;
		pWnd->nData1 ++;
		bRet = FALSE;
		break;

	/* 効果音 */
	case '3':
		{
			char szTmp[3];
			szTmp[0] = pWnd->pszMsg[pWnd->nData1 + 1];
			szTmp[1] = pWnd->pszMsg[pWnd->nData1 + 2];
			szTmp[2] = 0;
			int nNo = atoi (szTmp);
			if (nNo < SOUND_MAX) {
				DXAudio.PlaySecoundary (&aSoundTbl[nNo]);
			}
			pWnd->nData1 += 2;
			bRet = FALSE;
		}
		break;

	/* アイテム取得 */
	case '4':
		{
			char szTmp[6];
			int i;
			for (i = 0; i < 5; i ++) {
				szTmp[i] = pWnd->pszMsg[pWnd->nData1 + i + 1];
			}
			szTmp[i] = 0;
			SendCmdGETITEM ((DWORD)atoi (szTmp), 0);
			pWnd->nData1 += 5;
			bRet = FALSE;
		}
		break;

	/* 復活座標設定 */
	case '8':
		{
			int i;
			char szMspID[4], szX[4], szY[4];
			int nPosAt = pWnd->nData1 - 1;		/* "@8" の '@' の位置 */
			for (i = 0; i < 3; i ++) {
				szMspID[i] = pWnd->pszMsg[pWnd->nData1 + i + 1];
			}
			szMspID[i] = 0;
			pWnd->nData1 += 3;

			for (i = 0; i < 3; i ++) {
				szX[i] = pWnd->pszMsg[pWnd->nData1 + i + 1];
			}
			szX[i] = 0;
			pWnd->nData1 += 3;

			for (i = 0; i < 3; i ++) {
				szY[i] = pWnd->pszMsg[pWnd->nData1 + i + 1];
			}
			szY[i] = 0;
			pWnd->nData1 += 3;

			/* 以前はここですぐ記録していたが、「はい・いいえ」で選んでもらう */
			StartPosConfirmOpen (pWnd, nPosAt, atoi (szMspID), atoi (szX), atoi (szY));
			bRet = FALSE;
		}
		break;

	/* キャラ画像記録 */
	case '9':
		{
			char szTmp[6];
			szTmp[0] = pWnd->pszMsg[pWnd->nData1 + 1];
			szTmp[1] = pWnd->pszMsg[pWnd->nData1 + 2];
			szTmp[2] = pWnd->pszMsg[pWnd->nData1 + 3];
			szTmp[3] = pWnd->pszMsg[pWnd->nData1 + 4];
			szTmp[4] = pWnd->pszMsg[pWnd->nData1 + 5];
			szTmp[5] = 0;
			int nNo = atoi (szTmp);
			if (nNo >= 0 && nNo < g_wCountCharGrp * 16) {
				SendCmdCHGCHARGRPBACK (nNo);
			}
			pWnd->nData1 += 5;
			bRet = FALSE;
		}
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：GetWindowID														 */
/* 内容：ウィンドウ番号を取得												 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
int GetWindowID(DWORD dwID)
{
	int i;

	for (i = 0; i < (int)g_WndInfo.GetSize (); i ++) {
		if (g_WndInfo[i]->dwID == dwID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetCharID														 */
/* 内容：キャラ番号を取得													 */
/* 日付：2003/05/12															 */
/* ========================================================================= */
int GetCharID(DWORD pID)
{
	int i;

	for (i = 0; i < (int)g_aCharInfo.GetSize (); i ++) {
		if (g_aCharInfo[i]->pID == pID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：StopBGM															 */
/* 内容：BGM停止															 */
/* 日付：2003/08/28															 */
/* ========================================================================= */
void StopBGM(void)
{
	if (g_dwPlayBgmNo == 0) {
		return;
	}

//	g_AflMusic.fadeOut (500);
	g_LibMusic.Stop ();

	g_dwPlayBgmNo = 0;
}

/* ========================================================================= */
/* 関数名：PlayBGM															 */
/* 内容：BGM演奏															 */
/* 日付：2003/08/28															 */
/* ========================================================================= */
void PlayBGM(DWORD dwNo, BOOL bPlay)
{
	DWORD dwBGM = 0;

	StopBGM ();
	g_LibMusic.Close ();
	if (dwNo) {
		dwBGM = dwNo;

	} else {
		dwBGM = g_pMapInfo->byBGM;
	}

	g_dwPlayBgmNo = dwBGM;

	BOOL bFlg = g_bBGMONOFF;
	if (bPlay) {
		bFlg = TRUE;
	}
	if (adwBgmIDTbl[dwBGM] == 0) {
		bFlg = FALSE;
	}

	if (bFlg) {
		g_LibMusic.SetRelativeVolume (anVolTblMIDI[g_byVolMusic]);
		g_LibMusic.SetLoop (TRUE);
		g_LibMusic.Open (g_hDll, FindResource (g_hDll,
				MAKEINTRESOURCE (adwBgmIDTbl[dwBGM]), "WAVE"));
		g_LibMusic.Play ();
		g_LibMusic.SetRelativeVolume (anVolTblMIDI[g_byVolMusic]);
	}
}

/* ========================================================================= */
/* 関数名：SaveIniOption													 */
/* 内容：オプション設定をiniへ保存											 */
/* ========================================================================= */
/*
 * 終了時（WM_DESTROY）に加えて、オプションを変えた時にも呼ぶ。
 * ブラウザ版はページを閉じても WM_DESTROY が来ないので、その場で保存しないと残らない。
 */
void SaveIniOption(void)
{
	char szBuf[16], szName[MAX_PATH + 1];
	ZeroMemory (szName, sizeof (szName));
	GetModuleFileName (NULL, szName, MAX_PATH);
	szName[strlen (szName) - 3] = 0;
	strcat (szName, "ini");

	ZeroMemory (szBuf, sizeof (szBuf));
	wsprintf (szBuf, "%d", g_bCpuMax);
	WritePrivateProfileString ("Option", "CpuMax", szBuf, szName);
	ZeroMemory (szBuf, sizeof (szBuf));
	wsprintf (szBuf, "%d", g_bComEfcONOFF);
	WritePrivateProfileString ("Option", "ComEfc", szBuf, szName);
	wsprintf (szBuf, "%d", g_bChatSound);
	WritePrivateProfileString ("Option", "ChatSound", szBuf, szName);
	wsprintf (szBuf, "%d", g_bBGMONOFF);
	WritePrivateProfileString ("Option", "Bgm", szBuf, szName);
	wsprintf (szBuf, "%d", g_bTitleSkip);
	WritePrivateProfileString ("Option", "SkipTitle", szBuf, szName);
	wsprintf (szBuf, "%d", g_byVolSound);
	WritePrivateProfileString ("Option", "VolSound", szBuf, szName);
	wsprintf (szBuf, "%d", g_byVolMusic);
	WritePrivateProfileString ("Option", "VolMusic", szBuf, szName);
	wsprintf (szBuf, "%d", g_bBalloonWindowView);
	WritePrivateProfileString ("Option", "BalloonWindow", szBuf, szName);
	wsprintf (szBuf, "%d", g_bSpeechView);
	WritePrivateProfileString ("Option", "SpeechView", szBuf, szName);
	wsprintf (szBuf, "%d", g_byViewMode);
	WritePrivateProfileString ("Option", "ViewMode", szBuf, szName);
	wsprintf (szBuf, "%d", g_bWeatherDraw);
	WritePrivateProfileString ("Option", "WeatherDraw", szBuf, szName);
}

/* ========================================================================= */
/* 関数名：SaveAdminRura													 */
/* 内容：管理人ルーラ情報保存												 */
/* 日付：2003/08/16															 */
/* ========================================================================= */
BOOL SaveAdminRura(void)
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
	strcat (szName, "SboCliPosInfo.dat");

	hFile = CreateFile (szName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);

	PADMINRURAINFO pInfo;
	for (i = 0; i < g_AdminRuraInfo.GetSize (); i ++) {
		pInfo = (PADMINRURAINFO)g_AdminRuraInfo[i];
		WriteFile (hFile, pInfo, sizeof (ADMINRURAINFO), &dwBytes, NULL);
	}
	CloseHandle (hFile);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：LoadAdminRura													 */
/* 内容：管理人ルーラ情報読み込み											 */
/* 日付：2003/08/16															 */
/* ========================================================================= */
BOOL LoadAdminRura(void)
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
	strcat (szName, "SboCliPosInfo.dat");

	hFile = CreateFile (szName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	SetFilePointer (hFile, 0, 0, FILE_BEGIN);
	
	PADMINRURAINFO pInfoNew;
	for (i = 0; ; i ++) {
		pInfoNew = new ADMINRURAINFO;
		ZeroMemory (pInfoNew, sizeof (ADMINRURAINFO));
		ReadFile (hFile, pInfoNew, sizeof (ADMINRURAINFO), &dwBytes, NULL);
		if (dwBytes == 0) {
			SAFE_DELETE (pInfoNew);
			break;
		}
		/* 文字コードを UTF-8 にする前（Shift_JIS）に保存したコメントは読み替える */
		pInfoNew->szComment[sizeof (pInfoNew->szComment) - 1] = 0;
		if (!SboTextIsValid (pInfoNew->szComment)) {
			char szTmp[sizeof (pInfoNew->szComment) * 2];
			W32SjisToUtf8 (pInfoNew->szComment, -1, szTmp, sizeof (szTmp));
			SboTextCopy (pInfoNew->szComment, sizeof (pInfoNew->szComment), szTmp);
		}
		g_AdminRuraInfo.Add (pInfoNew);
	}
	CloseHandle (hFile);
	if (i == 0) {
		return FALSE;
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2006 */
