/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminStTmpInfoEdit.cpp									 */
/* 内容：		ステータステンプレート情報編集ダイアログクラス 実装ファイル	 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/12													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../Glbldata.h"
#include "DlgAdminStTmpInfoEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminStTmpInfoEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminStTmpInfoEdit)
	DDX_Control(pDX, IDC_MOVETYPE, m_cbMoveType);
	DDX_Text(pDX, IDC_DARK, m_nDark);
	DDX_Text(pDX, IDC_DEX, m_nDex);
	DDX_Text(pDX, IDC_DROP, m_nDrop);
	DDX_Text(pDX, IDC_EARTH, m_nEarth);
	DDX_Text(pDX, IDC_EXP, m_nExp);
	DDX_Text(pDX, IDC_FIRE, m_nFire);
	DDX_Text(pDX, IDC_GOLD, m_nGold);
	DDX_Text(pDX, IDC_IT, m_nIT);
	DDX_Text(pDX, IDC_LIGHT, m_nLight);
	DDX_Text(pDX, IDC_LK, m_nLK);
	DDX_Text(pDX, IDC_LV, m_nLV);
	DDX_Text(pDX, IDC_MAXHP, m_nMaxHP);
	DDX_Text(pDX, IDC_MAXMANA, m_nMaxMana);
	DDX_CBIndex(pDX, IDC_MOVETYPE, m_nMoveType);
	DDX_Text(pDX, IDC_MOVEX, m_nMoveX);
	DDX_Text(pDX, IDC_MOVEY, m_nMoveY);
	DDX_Text(pDX, IDC_NAME, m_strName);
	DDX_Text(pDX, IDC_PW, m_nPW);
	DDX_Text(pDX, IDC_REGMAGIC, m_nRegMagic);
	DDX_Text(pDX, IDC_ST, m_nST);
	DDX_Text(pDX, IDC_WATER, m_nWater);
	DDX_Text(pDX, IDC_WIND, m_nWind);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminStTmpInfoEdit, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminStTmpInfoEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfoEdit::CDlgAdminStTmpInfoEdit					 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/07/12														 */
/* ========================================================================= */

CDlgAdminStTmpInfoEdit::CDlgAdminStTmpInfoEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminStTmpInfoEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminStTmpInfoEdit)
	m_nDark = 0;
	m_nDex = 0;
	m_nDrop = 0;
	m_nEarth = 0;
	m_nExp = 0;
	m_nFire = 0;
	m_nGold = 0;
	m_nIT = 0;
	m_nLight = 0;
	m_nLK = 0;
	m_nLV = 0;
	m_nMaxHP = 0;
	m_nMaxMana = 0;
	m_nMoveType = -1;
	m_nMoveX = 0;
	m_nMoveY = 0;
	m_strName = _T("");
	m_nPW = 0;
	m_nRegMagic = 0;
	m_nST = 0;
	m_nWater = 0;
	m_nWind = 0;
	//}}AFX_DATA_INIT

	m_nSelect = -1;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfoEdit::Set										 */
/* 内容：	指定番号の情報を設定											 */
/* 日付：	2006/07/12														 */
/* ========================================================================= */

void CDlgAdminStTmpInfoEdit::Set(int nNo)
{
	m_nSelect = nNo;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfoEdit::Get										 */
/* 内容：	指定番号の情報へ設定											 */
/* 日付：	2006/07/14														 */
/* ========================================================================= */

void CDlgAdminStTmpInfoEdit::Get(int nNo)
{
	PSTATUSTEMPLATEINFO pInfo;

	pInfo = g_aStTmpInfo[nNo];

	strcpy (pInfo->szName, m_strName);
	pInfo->byNpcType			= m_nMoveType;
	pInfo->byPW					= m_nPW;
	pInfo->byDEX				= m_nDex;
	pInfo->byST					= m_nST;
	pInfo->byIT					= m_nIT;
	pInfo->byLK					= m_nLK;
	pInfo->wMaxHP				= m_nMaxHP;
	pInfo->wMaxMana				= m_nMaxMana;
	pInfo->dwGold				= m_nGold;
	pInfo->byDropProbability	= m_nDrop;
	pInfo->byMoveProbabilityX	= m_nMoveX;
	pInfo->byMoveProbabilityY	= m_nMoveY;
	pInfo->chAttrFire			= m_nFire;
	pInfo->chAttrWater			= m_nWater;
	pInfo->chAttrWind			= m_nWind;
	pInfo->chAttrEarth			= m_nEarth;
	pInfo->chAttrLight			= m_nLight;
	pInfo->chAttrDark			= m_nDark;
	pInfo->wDFMagic				= m_nRegMagic;
	pInfo->byLevel				= m_nLV;
	pInfo->dwExp				= m_nExp;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfoEdit::OnInitDialog							 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/07/12														 */
/* ========================================================================= */

BOOL CDlgAdminStTmpInfoEdit::OnInitDialog()
{
	int i;
	LPCSTR pszTmp;
	PSTATUSTEMPLATEINFO pInfo;

	CDialog::OnInitDialog ();

	for (i = 0; ; i ++) {
		pszTmp = aNpcMoveTypeTbl[i];

		if (strcmp (pszTmp, "z") == 0) {
			break;
		}
		m_cbMoveType.InsertString (i, pszTmp);
	}

	pInfo = g_aStTmpInfo[m_nSelect];

	m_strName	= pInfo->szName;
	m_nMoveType	= pInfo->byNpcType;
	m_nPW		= pInfo->byPW;
	m_nDex		= pInfo->byDEX;
	m_nST		= pInfo->byST;
	m_nIT		= pInfo->byIT;
	m_nLK		= pInfo->byLK;
	m_nMaxHP	= pInfo->wMaxHP;
	m_nMaxMana	= pInfo->wMaxMana;
	m_nGold		= pInfo->dwGold;
	m_nDrop		= pInfo->byDropProbability;
	m_nMoveX	= pInfo->byMoveProbabilityX;
	m_nMoveY	= pInfo->byMoveProbabilityY;
	m_nFire		= pInfo->chAttrFire;
	m_nWater	= pInfo->chAttrWater;
	m_nWind		= pInfo->chAttrWind;
	m_nEarth	= pInfo->chAttrEarth;
	m_nLight	= pInfo->chAttrLight;
	m_nDark		= pInfo->chAttrDark;
	m_nRegMagic	= pInfo->wDFMagic;
	m_nLV		= pInfo->byLevel;
	m_nExp		= pInfo->dwExp;

	UpdateData (FALSE);

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfoEdit::OnOK									 */
/* 内容：	ボタンハンドラ(OK)												 */
/* 日付：	2006/07/14														 */
/* ========================================================================= */

void CDlgAdminStTmpInfoEdit::OnOK()
{
	UpdateData ();

	CDialog::OnOK();
}

/* Copyright(C)URARA-works 2006 */
