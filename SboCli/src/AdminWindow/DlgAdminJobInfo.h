/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminJobInfo.h											 */
/* 内容：		職業情報ダイアログクラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/26													 */
/* ========================================================================= */

#pragma once

class CLayoutHelper;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CDlgAdminJobInfo : public CDialog
{
public:
			CDlgAdminJobInfo(CWnd* pParent = NULL);		/* コンストラクタ */
	virtual ~CDlgAdminJobInfo();

	void	Init	(void);										/* 初期化 */
	void	Renew	(void);										/* 更新 */


protected:
	CLayoutHelper	*m_pLayoutHelper;		/* 配置補助 */
	CWnd			*m_pWndParent;			/* 親ウィンドウ */


public:
	//{{AFX_DATA(CDlgAdminJobInfo)
	enum { IDD = IDD_JOBINFO };
	CListCtrl	m_List;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminJobInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CDlgAdminJobInfo)
	virtual BOOL OnInitDialog();
	afx_msg void OnEdit();
	void OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam);
	afx_msg void OnSend();
	afx_msg void OnGet();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
} CDlgAdminJobInfo, *PCDlgAdminJobInfo;

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
