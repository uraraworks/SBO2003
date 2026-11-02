/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	SboCliAdminWindow.cpp										 */
/* 内容：		管理者ウィンドウメインフレームクラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/25													 */
/* ========================================================================= */

#include "stdafx.h"
#include <process.h>
#include "Lib/LayoutHelper.h"
#include "AdminWindow/DlgAdminJobInfo.h"
#include "AdminWindow/DlgAdminSubGame.h"
#include "AdminWindow/DlgAdminStTmpInfo.h"
#include "SboCliAdminWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

IMPLEMENT_DYNAMIC(CAdminWindow, CFrameWnd)
BEGIN_MESSAGE_MAP(CAdminWindow, CFrameWnd)
	//{{AFX_MSG_MAP(CAdminWindow)
	ON_WM_CREATE()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_COMMAND(IDM_CHAR_STTMP, OnCharStTmpInfo)
	ON_COMMAND(IDM_ETC_JOBINFO, OnEtcJobinfo)
	ON_COMMAND(IDM_ETC_SUBGAME, OnEtcSubGame)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ADMINJOBINFO, OnAdminJobInfo)
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CAdminWindow::CAdminWindow										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

CAdminWindow::CAdminWindow()
{
	m_hWnd			= NULL;
	m_nScrID		= SCRIDADMIN_TOP;
	m_hThread		= NULL;
	m_hInitEvent	= NULL;
	m_hExitEvent	= NULL;
	m_pDlgJobInfo	= NULL;
	m_pDlgSubGame	= NULL;
	m_pDlgStTmpInfo	= NULL;

	m_pWndParent	= new CWnd;
	m_pLayoutHelper = new CLayoutHelper;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::~CAdminWindow										 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

CAdminWindow::~CAdminWindow()
{
	SAFE_DELETE (m_pLayoutHelper);
	Destroy ();
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::Create											 */
/* 内容：	作成															 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

BOOL CAdminWindow::Create(HWND hWndParent)
{
	BOOL bRet;
	UINT threadID;

	bRet = FALSE;

	/* すでに作成されている */
	if(m_hThread != NULL){
		goto Exit;
	}

	/* イベントを作成 */
	m_hInitEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (m_hInitEvent == NULL) {
		Destroy ();
		goto Exit;
	}
	m_hExitEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (m_hExitEvent == NULL) {
		Destroy ();
		goto Exit;
	}

	m_pWndParent->Attach (hWndParent);

#ifdef __EMSCRIPTEN__
	/* ブラウザ版はスレッドが無いので、ウィンドウだけ作ってゲームと同じメッセージループで動かす */
	m_hThread = (HANDLE)1;
	ThreadEntry ((PVOID)this);
	bRet = TRUE;
	goto Exit;
#endif

	/* スレッドを起動 */
	threadID = 0;
	m_hThread = (HANDLE)_beginthreadex (NULL, 0, ThreadEntry, (PVOID)this, 0, &threadID);
	if (m_hThread == NULL) {
		Destroy ();
		goto Exit;
	}

	/* 初期化完了を待機 */
	WaitForSingleObject (m_hInitEvent, INFINITE);

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::Destroy											 */
/* 内容：	破棄															 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

void CAdminWindow::Destroy(void)
{
	/* スレッドを停止 */
	if (m_hThread != NULL) {
		::PostMessage (m_hWnd, WM_CLOSE, 0, 0);
//		WaitForSingleObject (m_hThread, INFINITE);
	}

	/* イベントを破棄 */
	if (m_hExitEvent) {
		CloseHandle (m_hExitEvent);
	}
	if (m_hInitEvent) {
		CloseHandle (m_hInitEvent);
	}
#ifndef __EMSCRIPTEN__
	if (m_hThread) {
		CloseHandle (m_hThread);
	}
#endif

	/* 変数を初期化 */
	m_hWnd			= NULL;
	m_hThread		= NULL;
	m_hInitEvent	= NULL;
	m_hExitEvent	= NULL;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::Show												 */
/* 内容：	表示															 */
/* 日付：	2006/03/12														 */
/* ========================================================================= */

void CAdminWindow::Show(void)
{
	if (m_hWnd == NULL) {
		return;
	}

	ShowWindow (SW_SHOW);
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::Hide												 */
/* 内容：	非表示															 */
/* 日付：	2006/03/12														 */
/* ========================================================================= */

void CAdminWindow::Hide(void)
{
	if (m_hWnd == NULL) {
		return;
	}

	ShowWindow (SW_HIDE);
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::ChgScreen											 */
/* 内容：	画面切り替え													 */
/* 日付：	2006/01/26														 */
/* ========================================================================= */

void CAdminWindow::ChgScreen(int nScrID)
{
	CRect rc;

	DeleteDlg ();
	GetClientRect (rc);
		
	SAFE_DELETE (m_pLayoutHelper);
	m_pLayoutHelper = new CLayoutHelper;
	m_pLayoutHelper->Initialize (m_hWnd);

	switch (nScrID) {
	case SCRIDADMIN_JOBINFO:			/* 職業情報 */
		m_pDlgJobInfo = new CDlgAdminJobInfo(this);
		m_pDlgJobInfo->Init ();
		m_pDlgJobInfo->SetWindowPos (NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);
		m_pLayoutHelper->RegisterControl (m_pDlgJobInfo->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
		break;

	case SCRIDADMIN_SUBGAME:			/* サブゲーム */
		m_pDlgSubGame = new CDlgAdminSubGame(this);
		m_pDlgSubGame->Init ();
		m_pDlgSubGame->SetWindowPos (NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);
		m_pLayoutHelper->RegisterControl (m_pDlgSubGame->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
		break;

	case SCRIDADMIN_STTMPINFO:			/* ステータステンプレート情報 */
		m_pDlgStTmpInfo = new CDlgAdminStTmpInfo(this);
		m_pDlgStTmpInfo->Init ();
		m_pDlgStTmpInfo->SetWindowPos (NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);
		m_pLayoutHelper->RegisterControl (m_pDlgStTmpInfo->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
		break;
	}
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::DeleteDlg											 */
/* 内容：	ダイアログを削除												 */
/* 日付：	2006/01/26														 */
/* ========================================================================= */

void CAdminWindow::DeleteDlg(void)
{
	SAFE_DELETE (m_pDlgJobInfo);
	SAFE_DELETE (m_pDlgSubGame);
	SAFE_DELETE (m_pDlgStTmpInfo);
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::ThreadEntry										 */
/* 内容：	メイン処理														 */
/* 日付：	2006/02/11														 */
/* ========================================================================= */

UINT __stdcall CAdminWindow::ThreadEntry(LPVOID lpParam)
{
	BOOL bResult;
	MSG msg;
	PCAdminWindow pThis = (PCAdminWindow)lpParam;
	HANDLE aHandle[] = {pThis->m_hExitEvent};
	CRect rc;

	/* 開始完了 */
	SetEvent (pThis->m_hInitEvent);

	pThis->m_pWndParent->GetWindowRect (rc);
	rc.left		= rc.right;
	rc.right	= rc.left + 400;
	rc.bottom	= rc.top + 300;
#ifdef __EMSCRIPTEN__
	/* 同じ場所に出る「管理者用ウィンドウ」と重ならないよう、その下に出す */
	rc.top		+= 350;
	rc.bottom	+= 350;
#endif

	/* ウィンドウを作成 */
	bResult = pThis->CFrameWnd::Create (
					NULL, NULL,
					AFX_WS_DEFAULT_VIEW,
					rc,
					pThis->m_pWndParent);
	if (bResult == FALSE) {
		goto Exit;
	}
#ifdef __EMSCRIPTEN__
	/* メッセージはゲームのメッセージループが配る */
	goto Exit;
#endif

	while (GetMessage (&msg, NULL, 0, 0)) {
		if (msg.message == WM_QUIT) {
			break;
		}
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

Exit:
	return 0;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::PreCreateWindow									 */
/* 内容：	ウィンドウ作成前処理											 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

BOOL CAdminWindow::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wc;

	if (!CFrameWnd::PreCreateWindow (cs)) {
		return FALSE;
	}

	GetClassInfo (AfxGetInstanceHandle (), cs.lpszClass, &wc);
	wc.style &= ~(CS_HREDRAW | CS_VREDRAW);

	AfxRegisterClass (&wc);
	cs.dwExStyle	&= ~WS_EX_CLIENTEDGE;
	cs.style		= WS_OVERLAPPEDWINDOW & ~WS_SYSMENU;
	cs.lpszName		= "管理者ウィンドウ";
	cs.lpszClass	= AfxRegisterWndClass (wc.style, wc.hCursor, wc.hbrBackground, wc.hIcon);

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnCreate											 */
/* 内容：	メッセージハンドラ(WM_CREATE)									 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

int CAdminWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate (lpCreateStruct) == -1) {
		return -1;
	}

	m_hWnd = GetSafeHwnd ();
	/* メニューの設定 */
	m_Menu.LoadMenu ("IDR_MENU1");
	SetMenu (&m_Menu);

	ShowWindow (SW_SHOW);

	return 0;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnDestroy											 */
/* 内容：	メッセージハンドラ(WM_DESTROY)									 */
/* 日付：	2006/02/11														 */
/* ========================================================================= */

void CAdminWindow::OnDestroy()
{
	CFrameWnd::OnDestroy ();

	DeleteDlg ();

	if (m_pWndParent) {
		m_pWndParent->Detach ();
	}
	SAFE_DELETE (m_pWndParent);

#ifndef __EMSCRIPTEN__
	/* 専用スレッドのメッセージループを終わらせる（ブラウザ版はゲーム本体と共用なので送らない） */
	PostQuitMessage (0);
#endif
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnEraseBkgnd										 */
/* 内容：	メッセージハンドラ(WM_ERASEBKGND)								 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

BOOL CAdminWindow::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnGetMinMaxInfo									 */
/* 内容：	メッセージハンドラ(WM_GETMINMAXINFO)							 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

void CAdminWindow::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 400;
	lpMMI->ptMinTrackSize.y = 300;
	
	CFrameWnd::OnGetMinMaxInfo (lpMMI);
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnSize											 */
/* 内容：	メッセージハンドラ(WM_SIZE)										 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

void CAdminWindow::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize (nType, cx, cy);

	if (m_pLayoutHelper) {
		m_pLayoutHelper->DoLayout (cx, cy);
	}
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnAdminJobInfo									 */
/* 内容：	メッセージハンドラ(WM_ADMINJOBINFO)								 */
/* 日付：	2006/02/11														 */
/* ========================================================================= */

void CAdminWindow::OnAdminJobInfo(WPARAM wParam, LPARAM lParam)
{
	if (m_pDlgJobInfo == NULL) {
		return;
	}
	m_pDlgJobInfo->Renew ();
}


void CAdminWindow::OnCharStTmpInfo()
{
	ChgScreen (SCRIDADMIN_STTMPINFO);
}

/* ========================================================================= */
/* 関数名：	CAdminWindow::OnEtcJobinfo										 */
/* 内容：	メニューハンドラ(職業情報)										 */
/* 日付：	2006/01/25														 */
/* ========================================================================= */

void CAdminWindow::OnEtcJobinfo()
{
	ChgScreen (SCRIDADMIN_JOBINFO);
}


/* ========================================================================= */
/* 関数名：	CAdminWindow::OnEtcSubGame										 */
/* 内容：	メニューハンドラ(サブゲーム情報)								 */
/* 日付：	2006/04/13														 */
/* ========================================================================= */

void CAdminWindow::OnEtcSubGame()
{
	ChgScreen (SCRIDADMIN_SUBGAME);
}

/* Copyright(C)URARA-works 2006 */
