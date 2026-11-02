/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminSubGame.cpp											 */
/* 内容：		サブゲーム情報ダイアログクラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/04/13													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../Common/Info/SubGame/InfoSubGameBase.h"
#include "../Lib/LayoutHelper.h"
#include "../Proc/ProcSubGame.h"
#include "../Glbldata.h"
#include "DlgAdminSubGame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminSubGame::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminSubGame)
	DDX_Control(pDX, IDC_LIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminSubGame, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminSubGame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::CDlgAdminSubGame								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/04/13														 */
/* ========================================================================= */

CDlgAdminSubGame::CDlgAdminSubGame(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminSubGame::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminSubGame)
	//}}AFX_DATA_INIT

	m_pWndParent	= pParent;
	m_pLayoutHelper = new CLayoutHelper;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::~CDlgAdminSubGame								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/04/13														 */
/* ========================================================================= */

CDlgAdminSubGame::~CDlgAdminSubGame()
{
	SAFE_DELETE (m_pLayoutHelper);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/04/13														 */
/* ========================================================================= */

void CDlgAdminSubGame::Init(void)
{
	/* ウィンドウ作成 */
	Create (IDD_ETC_SUBGAME, m_pWndParent);
	ShowWindow (SW_SHOW);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::Renew											 */
/* 内容：	更新															 */
/* 日付：	2006/04/13														 */
/* ========================================================================= */

void CDlgAdminSubGame::Renew(void)
{
	int i, nCount;
	PCInfoSubGameBase pInfo;
	CString strTmp;

	m_List.DeleteAllItems ();

	nCount = g_ProcSubGame.GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfo = g_ProcSubGame.Get (i);

		strTmp.Format ("%u", pInfo->m_dwID);
		m_List.InsertItem (i, strTmp);
		strTmp.Format ("%u", pInfo->m_dwMapID);
		m_List.SetItemText (i, 1, strTmp);
		m_List.SetItemText (i, 2, pInfo->GetName ());
	}
}


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::OnInitDialog									 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

BOOL CDlgAdminSubGame::OnInitDialog()
{
	CDialog::OnInitDialog ();

	m_List.InsertColumn (0, "ID", LVCFMT_LEFT, 50);
	m_List.InsertColumn (1, "マップID", LVCFMT_LEFT, 70);
	m_List.InsertColumn (2, "種別", LVCFMT_LEFT, 100);
	m_List.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_pLayoutHelper->Initialize (m_hWnd);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDC_LIST)->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);

	Renew ();

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminSubGame::OnSize										 */
/* 内容：	メッセージハンドラ(WM_SIZE)										 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminSubGame::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize (nType, cx, cy);

	m_pLayoutHelper->DoLayout (cx, cy);
}

/* Copyright(C)URARA-works 2006 */
