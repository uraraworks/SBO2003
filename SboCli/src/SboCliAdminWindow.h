/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	SboCliAdminWindow.h											 */
/* 内容：		管理者ウィンドウメインフレームクラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/25													 */
/* ========================================================================= */

#pragma once

class CLayoutHelper;
class CDlgAdminJobInfo;
class CDlgAdminSubGame;
class CDlgAdminStTmpInfo;

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

/* 画面ID */
enum {
	SCRIDADMIN_TOP = 0,
	SCRIDADMIN_JOBINFO,						/* 職業情報 */
	SCRIDADMIN_SUBGAME,						/* サブゲーム */
	SCRIDADMIN_STTMPINFO,					/* ステータステンプレート情報 */
};


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CAdminWindow : public CFrameWnd
{
public:
			CAdminWindow();							/* コンストラクタ */
	virtual ~CAdminWindow();						/* デストラクタ */


	BOOL	Create		(HWND hWndParent);					/* 作成 */
	void	Destroy		(void);								/* 破棄 */

	void	Show		(void);								/* 表示 */
	void	Hide		(void);								/* 非表示 */
	void	ChgScreen	(int nScrID);						/* 画面切り替え */


protected:
	static UINT __stdcall ThreadEntry(LPVOID lpParam);		/* メイン処理 */
	void	DeleteDlg	(void);								/* ダイアログを削除 */


protected:
	HWND		m_hWnd;							/* ウィンドウハンドル */
	int			m_nScrID;						/* 画面ID */
	HANDLE		m_hThread;						/* スレッドハンドル */
	HANDLE		m_hInitEvent;					/* 開始待ちイベント */
	HANDLE		m_hExitEvent;					/* 終了待ちイベント */
	CWnd		*m_pWndParent;					/* 親ウィンドウ */
	CMenu		m_Menu;							/* メニュー */

	CLayoutHelper		*m_pLayoutHelper;		/* 配置補助 */
	CDlgAdminJobInfo	*m_pDlgJobInfo;			/* 職業情報 */
	CDlgAdminSubGame	*m_pDlgSubGame;			/* サブゲーム */
	CDlgAdminStTmpInfo	*m_pDlgStTmpInfo;		/* ステータステンプレート情報 */



public:
	DECLARE_DYNAMIC(CAdminWindow)
	//{{AFX_VIRTUAL(CAdminWindow)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL


protected:
	//{{AFX_MSG(CAdminWindow)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCharStTmpInfo();
	afx_msg void OnEtcJobinfo();
	afx_msg void OnEtcSubGame();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnAdminJobInfo(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
} CAdminWindow, *PCAdminWindow;

//{{AFX_INSERT_LOCATION}}

/* Copyright(C)URARA-works 2006 */
