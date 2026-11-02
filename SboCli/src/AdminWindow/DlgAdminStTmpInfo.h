/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminStTmpInfo.h											 */
/* 内容：		ステータステンプレート情報ダイアログクラス 定義ファイル		 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/04													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CDlgAdminStTmpInfo : public CDialog
{
public:
			CDlgAdminStTmpInfo(CWnd* pParent = NULL);			/* コンストラクタ */
	virtual ~CDlgAdminStTmpInfo();

	void	Init	(void);												/* 初期化 */
	void	Renew	(void);												/* 更新 */
	void	Renew	(int nNo, BOOL bAdd);								/* 更新 */


protected:
	CLayoutHelper	*m_pLayoutHelper;		/* 配置補助 */
	CWnd			*m_pWndParent;			/* 親ウィンドウ */



public:
	//{{AFX_DATA(CDlgAdminStTmpInfo)
	enum { IDD = IDD_STTMPINFO };
	CListCtrl	m_List;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminStTmpInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CDlgAdminStTmpInfo)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEdit();
	void OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
