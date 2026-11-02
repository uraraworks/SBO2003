/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminLvUpInfoEdit.cpp									 */
/* 内容：		レベルアップ情報編集ダイアログクラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/27													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../../Common/Info/InfoJob.h"
#include "../Glbldata.h"
#include "DlgAdminLvUpInfoEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminLvUpInfoEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminLvUpInfoEdit)
	DDX_Text(pDX, IDC_DEX, m_byDEX);
	DDX_Text(pDX, IDC_IT, m_byIT);
	DDX_Text(pDX, IDC_LK, m_byLK);
	DDX_Text(pDX, IDC_MAXHP, m_wMaxHP);
	DDX_Text(pDX, IDC_MAXMANA, m_wMaxMana);
	DDX_Text(pDX, IDC_PW, m_byPw);
	DDX_Text(pDX, IDC_ST, m_byST);
	DDX_Text(pDX, IDC_EXP, m_dwEXP);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminLvUpInfoEdit, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminLvUpInfoEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::CDlgAdminLvUpInfoEdit					 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminLvUpInfoEdit::CDlgAdminLvUpInfoEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminLvUpInfoEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminLvUpInfoEdit)
	m_byDEX = 0;
	m_byIT = 0;
	m_byLK = 0;
	m_wMaxHP = 0;
	m_wMaxMana = 0;
	m_byPw = 0;
	m_byST = 0;
	m_dwEXP = 0;
	//}}AFX_DATA_INIT

	m_pLvUpInfo = g_JobInfo.GetNewLvUpInfo ();
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::~CDlgAdminLvUpInfoEdit					 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

CDlgAdminLvUpInfoEdit::~CDlgAdminLvUpInfoEdit()
{
	if (m_pLvUpInfo) {
		g_JobInfo.DeleteLvUpInfo (m_pLvUpInfo);
		m_pLvUpInfo = NULL;
	}
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::Set										 */
/* 内容：	入力内容に設定													 */
/* 日付：	2006/01/30														 */
/* ========================================================================= */

void CDlgAdminLvUpInfoEdit::Set(PLVUPINFO pSrc)
{
	if (pSrc == NULL) {
		return;
	}

	m_byPw		= pSrc->byPW;
	m_byDEX		= pSrc->byDEX;
	m_byST		= pSrc->byST;
	m_byIT		= pSrc->byIT;
	m_byLK		= pSrc->byLK;
	m_wMaxHP	= pSrc->wMAXHP;
	m_wMaxMana	= pSrc->wMAXMANA;
	m_dwEXP		= pSrc->dwExp;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::Get										 */
/* 内容：	入力内容を取得													 */
/* 日付：	2006/01/30														 */
/* ========================================================================= */

void CDlgAdminLvUpInfoEdit::Get(PLVUPINFO pDst)
{
	if (pDst == NULL) {
		return;
	}

	pDst->byPW		= m_byPw;
	pDst->byDEX		= m_byDEX;
	pDst->byST		= m_byST;
	pDst->byIT		= m_byIT;
	pDst->byLK		= m_byLK;
	pDst->wMAXHP	= m_wMaxHP;
	pDst->wMAXMANA	= m_wMaxMana;
	pDst->dwExp		= m_dwEXP;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::OnInitDialog								 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

BOOL CDlgAdminLvUpInfoEdit::OnInitDialog()
{
	CDialog::OnInitDialog ();

	Set (m_pLvUpInfo);
	UpdateData (FALSE);

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminLvUpInfoEdit::OnOK										 */
/* 内容：	ボタンハンドラ(OK)												 */
/* 日付：	2006/01/27														 */
/* ========================================================================= */

void CDlgAdminLvUpInfoEdit::OnOK()
{
	UpdateData ();

	Get (m_pLvUpInfo);
	CDialog::OnOK ();
}

/* Copyright(C)URARA-works 2006 */
