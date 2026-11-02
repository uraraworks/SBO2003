/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminSubGame.h											 */
/* 内容：		サブゲーム情報ダイアログクラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/04/13													 */
/* ========================================================================= */

#pragma once

class CLayoutHelper;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CDlgAdminSubGame : public CDialog
{
public:
			CDlgAdminSubGame(CWnd* pParent = NULL);			/* コンストラクタ */
	virtual ~CDlgAdminSubGame();							/* デストラクタ */

	void	Init	(void);											/* 初期化 */
	void	Renew	(void);											/* 更新 */


protected:
	CLayoutHelper	*m_pLayoutHelper;		/* 配置補助 */
	CWnd			*m_pWndParent;			/* 親ウィンドウ */



public:
	//{{AFX_DATA(CDlgAdminSubGame)
	enum { IDD = IDD_ETC_SUBGAME };
	CListCtrl	m_List;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminSubGame)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CDlgAdminSubGame)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
} CDlgAdminSubGame, *PCDlgAdminSubGame;

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
