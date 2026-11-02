/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminLvUpInfo.h											 */
/* 内容：		レベルアップ情報ダイアログクラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/27													 */
/* ========================================================================= */

#pragma once

class CLayoutHelper;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CDlgAdminLvUpInfo : public CDialog
{
public:
			CDlgAdminLvUpInfo(CWnd* pParent = NULL);		/* コンストラクタ */
	virtual ~CDlgAdminLvUpInfo();							/* デストラクタ */

	void	Init			(void);									/* 初期化 */


protected:
	void	Renew			(void);									/* 一覧を更新 */
	void	OnAdminOK		(WPARAM wParam, LPARAM lParam);
	void	OnAdminCANCEL	(WPARAM wParam, LPARAM lParam);


public:
	CLayoutHelper			*m_pLayoutHelper;		/* 配置補助 */
	LPCMYARRAYLVUPINFO		m_pLvUpInfo;			/* 編集中のレベルアップ情報 */


public:
	//{{AFX_DATA(CDlgAdminLvUpInfo)
	enum { IDD = IDD_LVUPINFO };
	CListCtrl	m_List;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminLvUpInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CDlgAdminLvUpInfo)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	void OnAddEnd(CDialog *pDlg, int nResult, LPARAM lParam);
	void OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam);
	afx_msg void OnDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
