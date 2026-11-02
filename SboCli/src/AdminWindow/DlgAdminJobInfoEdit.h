/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminJobInfoEdit.h										 */
/* 内容：		職業情報編集ダイアログクラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/26													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CDlgAdminJobInfoEdit : public CDialog
{
public:
			CDlgAdminJobInfoEdit(CWnd* pParent = NULL);		/* コンストラクタ */
	virtual ~CDlgAdminJobInfoEdit();						/* デストラクタ */


public:
	PJOBINFO	m_pJobInfoEdit;				/* 編集中の職業情報 */


public:
	//{{AFX_DATA(CDlgAdminJobInfoEdit)
	enum { IDD = IDD_JOBINFO_EDIT };
	CString	m_strName;
	BOOL	m_bViewItem;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminJobInfoEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

protected:
	void	OnOK();
	//{{AFX_MSG(CDlgAdminJobInfoEdit)
	virtual BOOL OnInitDialog();
	afx_msg void OnLvUpInfo();
	void OnLvUpInfoEnd(CDialog *pDlg, int nResult, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
