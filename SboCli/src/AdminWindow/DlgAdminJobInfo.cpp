/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminJobInfo.cpp											 */
/* 内容：		職業情報ダイアログクラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/26													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../Common/Info/InfoJob.h"
#include "../Lib/LayoutHelper.h"
#include "../Glbldata.h"
#include "../SboCliSendCmd.h"
#include "DlgAdminJobInfoEdit.h"
#include "DlgModal.h"
#include "DlgAdminJobInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminJobInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminJobInfo)
	DDX_Control(pDX, IDC_LIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminJobInfo, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminJobInfo)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_BN_CLICKED(IDC_GET, OnGet)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::CDlgAdminJobInfo								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/26														 */
/* ========================================================================= */

CDlgAdminJobInfo::CDlgAdminJobInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminJobInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminJobInfo)
	//}}AFX_DATA_INIT

	m_pWndParent	= pParent;
	m_pLayoutHelper = new CLayoutHelper;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::~CDlgAdminJobInfo								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminJobInfo::~CDlgAdminJobInfo()
{
	SAFE_DELETE (m_pLayoutHelper);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminJobInfo::Init(void)
{
	/* ウィンドウ作成 */
	Create (IDD_JOBINFO, m_pWndParent);
	ShowWindow (SW_SHOW);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::Renew											 */
/* 内容：	更新															 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminJobInfo::Renew(void)
{
	int i, nCount, nSelect;
	PJOBINFO pInfo;

	nSelect	= m_List.GetNextItem (-1, LVNI_SELECTED);
	m_List.DeleteAllItems ();

	nCount = g_JobInfo.GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfo = g_JobInfo.GetPtr (i);

		m_List.InsertItem (i, pInfo->pszName);
	}

	m_List.SetItemState (nSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnInitDialog									 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

BOOL CDlgAdminJobInfo::OnInitDialog()
{
	CDialog::OnInitDialog ();

	m_List.InsertColumn (0, "名前", LVCFMT_LEFT, 200);
	m_List.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_pLayoutHelper->Initialize (m_hWnd);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDC_LIST)->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
	Renew ();

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnSize										 */
/* 内容：	メッセージハンドラ(WM_SIZE)										 */
/* 日付：	2006/02/11														 */
/* ========================================================================= */

void CDlgAdminJobInfo::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize (nType, cx, cy);

	m_pLayoutHelper->DoLayout (cx, cy);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnGet											 */
/* 内容：	ボタンハンドラ(取得)											 */
/* 日付：	2006/01/31														 */
/* ========================================================================= */

void CDlgAdminJobInfo::OnGet()
{
	int nSelect;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}

	SendCMDJOBINFOREQ ((BYTE)nSelect);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnEdit										 */
/* 内容：	ボタンハンドラ(編集)											 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminJobInfo::OnEdit()
{
	int nSelect;
	PJOBINFO pInfo;
	CDlgAdminJobInfoEdit *pDlg;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}

	pInfo = g_JobInfo.GetPtr (nSelect);
	pDlg = new CDlgAdminJobInfoEdit;
	g_JobInfo.Copy (pDlg->m_pJobInfoEdit, pInfo);
	pDlg->m_strName = pInfo->pszName;
	if (pInfo->dwFlg & JOBINFOFLG_VIEWITEMINFO) {
		pDlg->m_bViewItem = TRUE;
	}

	DOMODAL_THEN (pDlg, this, &CDlgAdminJobInfo::OnEditEnd, nSelect);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnEditEnd */
/* 内容：	編集画面が閉じた */
/* ========================================================================= */

void CDlgAdminJobInfo::OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam)
{
	PJOBINFO pInfo;
	CDlgAdminJobInfoEdit *pDlgEdit = (CDlgAdminJobInfoEdit *)pDlg;

	if (nResult != IDOK) {
		return;
	}
	pInfo = g_JobInfo.GetPtr ((int)lParam);
	if (pInfo == NULL) {
		return;
	}
	g_JobInfo.Copy (pInfo, pDlgEdit->m_pJobInfoEdit);
	Renew ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfo::OnSend										 */
/* 内容：	ボタンハンドラ(送信)											 */
/* 日付：	2006/01/31														 */
/* ========================================================================= */

void CDlgAdminJobInfo::OnSend()
{
	int nSelect;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}

	SendCMDJOBINFO (nSelect);
}

/* Copyright(C)URARA-works 2006 */
