/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminJobInfoEdit.cpp										 */
/* 内容：		職業情報編集ダイアログクラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/27													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../Glbldata.h"
#include "DlgAdminLvUpInfo.h"
#include "DlgAdminJobInfoEdit.h"
#include "DlgModal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminJobInfoEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminJobInfoEdit)
	DDX_Text(pDX, IDC_NAME, m_strName);
	DDX_Check(pDX, IDC_VIEWITEMINFO, m_bViewItem);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminJobInfoEdit, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminJobInfoEdit)
	ON_BN_CLICKED(IDC_LVUPINFO, OnLvUpInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::CDlgAdminJobInfoEdit						 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminJobInfoEdit::CDlgAdminJobInfoEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminJobInfoEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminJobInfoEdit)
	m_strName = _T("");
	m_bViewItem = FALSE;
	//}}AFX_DATA_INIT

	m_pJobInfoEdit = g_JobInfo.GetNew ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::~CDlgAdminJobInfoEdit						 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminJobInfoEdit::~CDlgAdminJobInfoEdit()
{
	g_JobInfo.Delete (m_pJobInfoEdit);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::OnInitDialog								 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

BOOL CDlgAdminJobInfoEdit::OnInitDialog()
{
	CDialog::OnInitDialog ();

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::OnOK										 */
/* 内容：	ボタンハンドラ(OK)												 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminJobInfoEdit::OnOK()
{
	UpdateData ();

	g_JobInfo.SetJobName (m_pJobInfoEdit, m_strName);
	m_pJobInfoEdit->dwFlg &= ~JOBINFOFLG_VIEWITEMINFO;
	if (m_bViewItem) {
		m_pJobInfoEdit->dwFlg |= JOBINFOFLG_VIEWITEMINFO;
	}

	CDialog::OnOK ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::OnLvUpInfo								 */
/* 内容：	ボタンハンドラ(レベルアップ情報の設定)							 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminJobInfoEdit::OnLvUpInfo()
{
	CDlgAdminLvUpInfo *pDlg;

	pDlg = new CDlgAdminLvUpInfo;
	g_JobInfo.CopyLvUpInfo (pDlg->m_pLvUpInfo, &m_pJobInfoEdit->aLevelUpInfo);

	DOMODAL_THEN (pDlg, this, &CDlgAdminJobInfoEdit::OnLvUpInfoEnd, 0);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminJobInfoEdit::OnLvUpInfoEnd */
/* 内容：	レベルアップ情報の画面が閉じた */
/* ========================================================================= */

void CDlgAdminJobInfoEdit::OnLvUpInfoEnd(CDialog *pDlg, int nResult, LPARAM lParam)
{
	CDlgAdminLvUpInfo *pDlgLvUp = (CDlgAdminLvUpInfo *)pDlg;

	if (nResult == IDOK) {
		g_JobInfo.CopyLvUpInfo (&m_pJobInfoEdit->aLevelUpInfo, pDlgLvUp->m_pLvUpInfo);
	}
}

/* Copyright(C)URARA-works 2006 */
