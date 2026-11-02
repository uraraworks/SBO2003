/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	DlgAdminStTmpInfo.cpp										 */
/* 内容：		ステータステンプレート情報ダイアログクラス 実装ファイル		 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/04													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../Lib/LayoutHelper.h"
#include "../Glbldata.h"
#include "DlgAdminStTmpInfoEdit.h"
#include "DlgAdminStTmpInfo.h"
#include "DlgModal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ========================================================================= */
/* クラスの設定																 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgAdminStTmpInfo)
	DDX_Control(pDX, IDC_LIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgAdminStTmpInfo, CDialog)
	//{{AFX_MSG_MAP(CDlgAdminStTmpInfo)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::CDlgAdminStTmpInfo							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

CDlgAdminStTmpInfo::CDlgAdminStTmpInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAdminStTmpInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAdminStTmpInfo)
	//}}AFX_DATA_INIT

	m_pWndParent	= pParent;
	m_pLayoutHelper = new CLayoutHelper;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::~CDlgAdminStTmpInfo							 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

CDlgAdminStTmpInfo::~CDlgAdminStTmpInfo()
{
	SAFE_DELETE (m_pLayoutHelper);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::Init										 */
/* 内容：	初期化															 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::Init(void)
{
	/* ウィンドウ作成 */
	Create (IDD_STTMPINFO, m_pWndParent);
	ShowWindow (SW_SHOW);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::Renew										 */
/* 内容：	更新															 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::Renew(void)
{
	int i, nCount, nSelect;
	CString strTmp;

	nSelect	= m_List.GetNextItem (-1, LVNI_SELECTED);
	m_List.DeleteAllItems ();

	nCount = g_aStTmpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		Renew (i, TRUE);
	}

	m_List.SetItemState (nSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::Renew										 */
/* 内容：	更新															 */
/* 日付：	2006/07/14														 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::Renew(int nNo, BOOL bAdd)
{
	PSTATUSTEMPLATEINFO pInfo;
	CString strTmp;

	pInfo = g_aStTmpInfo[nNo];

	if (bAdd) {
		m_List.InsertItem (nNo, pInfo->szName);
	} else {
		m_List.SetItemText (nNo, 0, pInfo->szName);
	}
	m_List.SetItemText (nNo, 1, aNpcMoveTypeTbl[pInfo->byNpcType]);
	strTmp.Format ("%d", pInfo->byPW);
	m_List.SetItemText (nNo, 2, strTmp);
	strTmp.Format ("%d", pInfo->byDEX);
	m_List.SetItemText (nNo, 3, strTmp);
	strTmp.Format ("%d", pInfo->byST);
	m_List.SetItemText (nNo, 4, strTmp);
	strTmp.Format ("%d", pInfo->byIT);
	m_List.SetItemText (nNo, 5, strTmp);
	strTmp.Format ("%d", pInfo->byLK);
	m_List.SetItemText (nNo, 6, strTmp);
	strTmp.Format ("%d", pInfo->wMaxHP);
	m_List.SetItemText (nNo, 7, strTmp);
	strTmp.Format ("%d", pInfo->wMaxMana);
	m_List.SetItemText (nNo, 8, strTmp);
	strTmp.Format ("%u", pInfo->dwGold);
	m_List.SetItemText (nNo, 9, strTmp);
	strTmp.Format ("%d", pInfo->byDropProbability);
	m_List.SetItemText (nNo, 10, strTmp);
	strTmp.Format ("%d", pInfo->byMoveProbabilityX);
	m_List.SetItemText (nNo, 11, strTmp);
	strTmp.Format ("%d", pInfo->byMoveProbabilityY);
	m_List.SetItemText (nNo, 12, strTmp);
	strTmp.Format ("%d", pInfo->bySearchCx);
	m_List.SetItemText (nNo, 13, strTmp);
	strTmp.Format ("%d", pInfo->bySearchCy);
	m_List.SetItemText (nNo, 14, strTmp);
	strTmp.Format ("%d", pInfo->chAttrFire);
	m_List.SetItemText (nNo, 15, strTmp);
	strTmp.Format ("%d", pInfo->chAttrWater);
	m_List.SetItemText (nNo, 16, strTmp);
	strTmp.Format ("%d", pInfo->chAttrWind);
	m_List.SetItemText (nNo, 17, strTmp);
	strTmp.Format ("%d", pInfo->chAttrEarth);
	m_List.SetItemText (nNo, 18, strTmp);
	strTmp.Format ("%d", pInfo->chAttrLight);
	m_List.SetItemText (nNo, 19, strTmp);
	strTmp.Format ("%d", pInfo->chAttrDark);
	m_List.SetItemText (nNo, 20, strTmp);
	strTmp.Format ("%d", pInfo->wDFMagic);
	m_List.SetItemText (nNo, 21, strTmp);
	strTmp.Format ("%d", pInfo->byLevel);
	m_List.SetItemText (nNo, 22, strTmp);
	strTmp.Format ("%u", pInfo->dwExp);
	m_List.SetItemText (nNo, 23, strTmp);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::OnInitDialog								 */
/* 内容：	メッセージハンドラ(WM_INITDIALOG)								 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

BOOL CDlgAdminStTmpInfo::OnInitDialog()
{
	CDialog::OnInitDialog ();

	m_List.InsertColumn (0, "名前", LVCFMT_LEFT, 100);
	m_List.InsertColumn (1, "移動種別", LVCFMT_LEFT, 100);
	m_List.InsertColumn (2, "力", LVCFMT_LEFT, 70);
	m_List.InsertColumn (3, "素早さ", LVCFMT_LEFT, 70);
	m_List.InsertColumn (4, "体力", LVCFMT_LEFT, 70);
	m_List.InsertColumn (5, "賢さ", LVCFMT_LEFT, 70);
	m_List.InsertColumn (6, "運のよさ", LVCFMT_LEFT, 70);
	m_List.InsertColumn (7, "最大HP", LVCFMT_LEFT, 70);
	m_List.InsertColumn (8, "最大Mana", LVCFMT_LEFT, 70);
	m_List.InsertColumn (9, "所持金", LVCFMT_LEFT, 70);
	m_List.InsertColumn (10, "ドロップ率", LVCFMT_LEFT, 70);
	m_List.InsertColumn (11, "移動確率X", LVCFMT_LEFT, 70);
	m_List.InsertColumn (12, "移動確率Y", LVCFMT_LEFT, 70);
	m_List.InsertColumn (13, "検索範囲X", LVCFMT_LEFT, 70);
	m_List.InsertColumn (14, "検索範囲Y", LVCFMT_LEFT, 70);
	m_List.InsertColumn (15, "火", LVCFMT_LEFT, 70);
	m_List.InsertColumn (16, "水", LVCFMT_LEFT, 70);
	m_List.InsertColumn (17, "風", LVCFMT_LEFT, 70);
	m_List.InsertColumn (18, "土", LVCFMT_LEFT, 70);
	m_List.InsertColumn (19, "光", LVCFMT_LEFT, 70);
	m_List.InsertColumn (20, "闇", LVCFMT_LEFT, 70);
	m_List.InsertColumn (21, "魔法防御", LVCFMT_LEFT, 70);
	m_List.InsertColumn (22, "レベル", LVCFMT_LEFT, 70);
	m_List.InsertColumn (23, "経験値", LVCFMT_LEFT, 70);
	m_List.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_pLayoutHelper->Initialize (m_hWnd);
	m_pLayoutHelper->RegisterControl (GetDlgItem (IDC_LIST)->m_hWnd, LH_CTRL_WIDTH | LH_CTRL_HEIGHT);
	Renew ();

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::OnSize										 */
/* 内容：	メッセージハンドラ(WM_SIZE)										 */
/* 日付：	2006/07/04														 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize (nType, cx, cy);

	m_pLayoutHelper->DoLayout (cx, cy);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::OnEdit										 */
/* 内容：	ボタンハンドラ(編集)											 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CDlgAdminStTmpInfo::OnEdit()
{
	int nSelect;
	CDlgAdminStTmpInfoEdit *pDlg;

	nSelect = m_List.GetNextItem (-1, LVNI_SELECTED);
	if (nSelect < 0) {
		return;
	}

	pDlg = new CDlgAdminStTmpInfoEdit;
	pDlg->Set (nSelect);
	DOMODAL_THEN (pDlg, this, &CDlgAdminStTmpInfo::OnEditEnd, nSelect);
}


/* ========================================================================= */
/* 関数名：	CDlgAdminStTmpInfo::OnEditEnd */
/* 内容：	編集画面が閉じた */
/* ========================================================================= */

void CDlgAdminStTmpInfo::OnEditEnd(CDialog *pDlg, int nResult, LPARAM lParam)
{
	CDlgAdminStTmpInfoEdit *pDlgEdit = (CDlgAdminStTmpInfoEdit *)pDlg;

	if (nResult != IDOK) {
		return;
	}

	pDlgEdit->Get ((int)lParam);
	Renew ((int)lParam, FALSE);
}

/* Copyright(C)URARA-works 2006 */
