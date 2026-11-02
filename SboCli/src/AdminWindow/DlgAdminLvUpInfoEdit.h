/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminLvUpInfoEdit.h										 */
/* 内容：		レベルアップ情報編集ダイアログクラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/27													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CDlgAdminLvUpInfoEdit : public CDialog
{
public:
			CDlgAdminLvUpInfoEdit(CWnd* pParent = NULL);	/* コンストラクタ */
	virtual ~CDlgAdminLvUpInfoEdit();						/* デストラクタ */

	void	Init	(void);											/* 初期化 */
	void	Set		(PLVUPINFO pSrc);								/* 入力内容に設定 */
	void	Get		(PLVUPINFO pDst);								/* 入力内容を取得 */


public:
	PLVUPINFO		m_pLvUpInfo;				/* レベルアップ情報 */


public:
	//{{AFX_DATA(CDlgAdminLvUpInfoEdit)
	enum { IDD = IDD_LVUPINFO_EDIT };
	BYTE	m_byDEX;
	BYTE	m_byIT;
	BYTE	m_byLK;
	int		m_wMaxHP;
	int		m_wMaxMana;
	BYTE	m_byPw;
	BYTE	m_byST;
	DWORD	m_dwEXP;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminLvUpInfoEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL


protected:
	void	OnOK();
	//{{AFX_MSG(CDlgAdminLvUpInfoEdit)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
