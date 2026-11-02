/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminLvUpInfo.cpp										 */
/* 内容：		レベルアップ情報ダイアログクラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/27													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../Common/Info/InfoJob.h"
#include "../Lib/LayoutHelper.h"
#include "../Glbldata.h"
#include "DlgAdminLvUpInfoEdit.h"
#include "DlgAdminLvUpInfo.h"
#include "DlgModal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminLvUpInfo)
	DDX_Control(pDX, IDC_LIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminLvUpInfo, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminLvUpInfo)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::CDlgAdminLvUpInfo							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminLvUpInfo::CDlgAdminLvUpInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminLvUpInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminLvUpInfo)
	//}}AFX_DATA_INIT

	m_pLayoutHelper	= new CLayoutHelper;
	m_pLvUpInfo		= new CMYARRAYLVUPINFO;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::~CDlgAdminLvUpInfo							 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminLvUpInfo::~CDlgAdminLvUpInfo()
{
	SAFE_DELETE (m_pLayoutHelper);
	if (m_pLvUpInfo) {
		g_JobInfo.DeleteAllLvUpInfo (m_pLvUpInfo);
		SAFE_DELETE (m_pLvUpInfo);
	}
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::Renew										 */
/* 内容：	一覧を更新														 */
/* 日付：	2006/01/30														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::Renew(void)
{
	int i, nCount;
	PLVUPINFO pLvUpInfo;
	CString strTmp;

	m_List.DeleteAllItems ();

	nCount = m_pLvUpInfo->GetSize ();
	for (i = 0; i < nCount; i ++) {
		pLvUpInfo = m_pLvUpInfo->GetAt (i);
		strTmp.Format ("%u", i + 1);
		m_List.InsertItem (i, strTmp);
		strTmp.Format ("%u", pLvUpInfo->byPW);
		m_List.SetItemText (i, 1, strTmp);
		strTmp.Format ("%u", pLvUpInfo->byDEX);
		m_List.SetItemText (i, 2, strTmp);
		strTmp.Format ("%u", pLvUpInfo->byST);
		m_List.SetItemText (i, 3, strTmp);
		strTmp.Format ("%u", pLvUpInfo->byIT);
		m_List.SetItemText (i, 4, strTmp);
		strTmp.Format ("%u", pLvUpInfo->byLK);
		m_List.SetItemText (i, 5, strTmp);
		strTmp.Format ("%u", pLvUpInfo->wMAXHP);
		m_List.SetItemText (i, 6, strTmp);
		strTmp.Format ("%u", pLvUpInfo->wMAXMANA);
		m_List.SetItemText (i, 7, strTmp);
		strTmp.Format ("%u", pLvUpInfo->dwExp);
		m_List.SetItemText (i, 8, strTmp);
	}
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnInitDialog									 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

BOOL CDlgAdminLvUpInfo::OnInitDialog()
{
	CDialog::OnInitDialog ();

	m_List.InsertColumn (0, "レベル",		LVCFMT_LEFT, 70);
	m_List.InsertColumn (1, "力",			LVCFMT_LEFT, 70);
	m_List.InsertColumn (2, "素早さ",		LVCFMT_LEFT, 70);
	m_List.InsertColumn (3, "体力",			LVCFMT_LEFT, 70);
	m_List.InsertColumn (4, "賢さ",			LVCFMT_LEFT, 70);
	m_List.InsertColumn (5, "運のよさ",		LVCFMT_LEFT, 70);
	m_List.InsertColumn (6, "最大HP",		LVCFMT_LEFT, 70);
	m_List.InsertColumn (7, "最大Mana",		LVCFMT_LEFT, 70);
	m_List.InsertColumn (8, "必要な経験値",	LVCFMT_LEFT, 100);
	m_List.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_pLayoutHelper->Initialize (m_hWnd);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDC_LIST)	->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDOK)		->m_hWnd, LH_CTRL_X | LH_CTRL_Y);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDCANCEL)	->m_hWnd, LH_CTRL_X | LH_CTRL_Y);

	Renew ();
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnSize										 */
/* 内容：	メッセージハンドラ(WM_SIZE)										 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize (nType, cx, cy);

	m_pLayoutHelper->DoLayout (cx, cy);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnOK											 */
/* 内容：	ボタンハンドラ(OK)												 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnOK()
{
	UpdateData ();

	CDialog::OnOK ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnAdd										 */
/* 内容：	ボタンハンドラ(追加)											 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnAdd()
{
	CDlgAdminLvUpInfoEdit *pDlg;

	pDlg = new CDlgAdminLvUpInfoEdit;
	DOMODAL_THEN (pDlg, this, &CDlgAdminLvUpInfo::OnAddEnd, 0);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnAddEnd */
/* 内容：	追加の画面が閉じた */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnAddEnd(CDialog *pDlg, int nResult, LPARAM lParam)
{
	PLVUPINFO pLvUpInfo;
	CDlgAdminLvUpInfoEdit *pDlgEdit = (CDlgAdminLvUpInfoEdit *)pDlg;

	if (nResult != IDOK) {
		return;
	}
	pLvUpInfo = g_JobInfo.GetNewLvUpInfo ();
	g_JobInfo.CopyLvUpInfo (pLvUpInfo, pDlgEdit->m_pLvUpInfo);
	m_pLvUpInfo->Add (pLvUpInfo);
	Renew ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnEdit										 */
/* 内容：	ボタンハンドラ(編集)											 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnEdit()
{
	int nSelect;
	PLVUPINFO pLvUpInfo;
	CDlgAdminLvUpInfoEdit *pDlg;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}
	pLvUpInfo = m_pLvUpInfo->GetAt (nSelect);

	pDlg = new CDlgAdminLvUpInfoEdit;
	g_JobInfo.CopyLvUpInfo (pDlg->m_pLvUpInfo, pLvUpInfo);
	DOMODAL_THEN (pDlg, this, &CDlgAdminLvUpInfo::OnEditEnd, nSelect);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnEditEnd */
/* 内容：	編集の画面が閉じた */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam)
{
	PLVUPINFO pLvUpInfo;
	CDlgAdminLvUpInfoEdit *pDlgEdit = (CDlgAdminLvUpInfoEdit *)pDlg;

	if (nResult != IDOK) {
		return;
	}
	if ((int)lParam >= m_pLvUpInfo->GetSize ()) {
		return;
	}
	pLvUpInfo = m_pLvUpInfo->GetAt ((int)lParam);
	g_JobInfo.CopyLvUpInfo (pLvUpInfo, pDlgEdit->m_pLvUpInfo);
	Renew ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfo::OnDelete										 */
/* 内容：	ボタンハンドラ(削除)											 */
/* 日付：	2006/02/11														 */
/* ========================================================================= */

void CDlgAdminLvUpInfo::OnDelete()
{
	int nSelect;
	PLVUPINFO pLvUpInfo;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}

	pLvUpInfo = m_pLvUpInfo->GetAt (nSelect);
	g_JobInfo.DeleteLvUpInfo (pLvUpInfo);
	m_pLvUpInfo->RemoveAt (nSelect);

	Renew ();
}

/* Copyright(C)URARA-works 2006 */
