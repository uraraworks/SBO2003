/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminStTmpInfoEdit.h										 */
/* 内容：		ステータステンプレート情報編集ダイアログクラス 定義ファイル	 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/12													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CDlgAdminStTmpInfoEdit : public CDialog
{
public:
	CDlgAdminStTmpInfoEdit(CWnd* pParent = NULL);		/* コンストラクタ */

	void Set	(int nNo);										/* 指定番号の情報を設定 */
	void Get	(int nNo);										/* 指定番号の情報へ設定 */



protected:
	int		m_nSelect;						/* 選択中のリスト番号 */


public:
	//{{AFX_DATA(CDlgAdminStTmpInfoEdit)
	enum { IDD = IDD_STTMPINFO_EDIT };
	CComboBox	m_cbMoveType;
	int		m_nDark;
	int		m_nDex;
	int		m_nDrop;
	int		m_nEarth;
	int		m_nExp;
	int		m_nFire;
	int		m_nGold;
	int		m_nIT;
	int		m_nLight;
	int		m_nLK;
	int		m_nLV;
	int		m_nMaxHP;
	int		m_nMaxMana;
	int		m_nMoveType;
	int		m_nMoveX;
	int		m_nMoveY;
	CString	m_strName;
	int		m_nPW;
	int		m_nRegMagic;
	int		m_nST;
	int		m_nWater;
	int		m_nWind;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CDlgAdminStTmpInfoEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CDlgAdminStTmpInfoEdit)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
