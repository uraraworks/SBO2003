/* ========================================================================= */
/* ファイル名：	afx_compat.h												 */
/* 概要：		Web版: MFC のうち、管理画面で使っている部分だけの互換クラス	 */
/* ========================================================================= */
/*
 * 仕組み
 *  - CWnd は HWND を包むだけの薄いクラス。中身は Win32 互換層の関数を呼ぶ。
 *  - メッセージマップは MFC と同じ書き方（BEGIN_MESSAGE_MAP / ON_xxx）で書ける。
 *    各項目は「メッセージを引数に直してメンバ関数を呼ぶ小さな関数」を持つ。
 *  - CDialog::DoModal は終わるまで待てないので、ブラウザ版では DoModalAsync で
 *    閉じた時に呼び出し元へ知らせる（AdminWindow/DlgModal.h を参照）。
 */
#pragma once

#ifdef __cplusplus

#include "win32compat.h"
#include "commctrl.h"
#include <type_traits>

#define afx_msg
#define AFXAPI
#ifndef PASCAL
#define PASCAL
#endif
#define DEBUG_NEW				new
#define AFX_WS_DEFAULT_VIEW		(WS_CHILD | WS_VISIBLE | WS_BORDER)

#define DECLARE_DYNAMIC(c)
#define IMPLEMENT_DYNAMIC(c, b)

class CWnd;
class CMenu;
class CDialog;

HINSTANCE	AfxGetInstanceHandle(void);
BOOL		AfxRegisterClass(WNDCLASS *pWc);
LPCSTR		AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = NULL, HBRUSH hbrBackground = NULL, HICON hIcon = NULL);

/* ------------------------------------------------------------------------- */
/* 文字列																	 */
/* ------------------------------------------------------------------------- */
class CString
{
public:
	CString();
	CString(const CString &src);
	CString(LPCSTR psz);
	~CString();

	const CString &operator=(const CString &src);
	const CString &operator=(LPCSTR psz);
	const CString &operator+=(LPCSTR psz);
	operator LPCSTR() const					{ return m_psz; }

	int		GetLength(void) const			{ return (int)strlen(m_psz); }
	BOOL	IsEmpty(void) const				{ return m_psz[0] == '\0'; }
	void	Empty(void)						{ *this = ""; }
	void	Format(LPCSTR pszFormat, ...);
	int		Compare(LPCSTR psz) const		{ return strcmp(m_psz, psz); }

	LPSTR	GetBuffer(int nMinLen);
	void	ReleaseBuffer(int nNewLen = -1);

protected:
	void	Set(LPCSTR psz);

	LPSTR	m_psz;
};

inline bool operator==(const CString &a, LPCSTR b)	{ return strcmp(a, b) == 0; }
inline bool operator!=(const CString &a, LPCSTR b)	{ return strcmp(a, b) != 0; }

/* ------------------------------------------------------------------------- */
/* 矩形																		 */
/* ------------------------------------------------------------------------- */
class CRect : public tagRECT
{
public:
	CRect()											{ left = top = right = bottom = 0; }
	CRect(int l, int t, int r, int b)				{ left = l; top = t; right = r; bottom = b; }
	CRect(const RECT &rc)							{ left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; }

	operator LPRECT()								{ return this; }
	operator LPCRECT() const						{ return this; }
	int		Width(void) const						{ return right - left; }
	int		Height(void) const						{ return bottom - top; }
};

/* ------------------------------------------------------------------------- */
/* メッセージマップ															 */
/* ------------------------------------------------------------------------- */
class CCmdTarget;

/* メッセージを引数に直して呼ぶ関数 */
typedef LRESULT (*AFX_THUNK)(CCmdTarget *pThis, WPARAM wParam, LPARAM lParam);

struct AFX_MSGMAP_ENTRY {
	UINT		nMessage;			/* メッセージ（0 で終わり） */
	UINT		nCode;				/* WM_COMMAND の通知コード */
	UINT		nID;				/* WM_COMMAND の ID */
	AFX_THUNK	pfn;
};

struct AFX_MSGMAP {
	const AFX_MSGMAP *(*pfnGetBaseMap)(void);
	const AFX_MSGMAP_ENTRY *lpEntries;
};

#define DECLARE_MESSAGE_MAP() \
protected: \
	static const AFX_MSGMAP *GetThisMessageMap(void); \
	virtual const AFX_MSGMAP *GetMessageMap(void) const;

#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	const AFX_MSGMAP *theClass::GetMessageMap(void) const { return GetThisMessageMap(); } \
	const AFX_MSGMAP *theClass::GetThisMessageMap(void) \
	{ \
		typedef theClass ThisClass; \
		typedef baseClass TheBaseClass; \
		static const AFX_MSGMAP_ENTRY _messageEntries[] = {

#define END_MESSAGE_MAP() \
			{0, 0, 0, 0} \
		}; \
		static const AFX_MSGMAP messageMap = {&TheBaseClass::GetThisMessageMap, &_messageEntries[0]}; \
		return &messageMap; \
	}

/* メンバ関数の型からクラスを取り出す */
template<class T, class R, class... A> T *AfxClassOf(R (T::*)(A...));

/* メッセージごとの呼び出し方（F はメンバ関数） */
template<auto F> struct AfxThunk {
	typedef typename std::remove_pointer<decltype(AfxClassOf(F))>::type T;
	static T *This(CCmdTarget *p)		{ return static_cast<T *>(p); }

	/* 引数なし（ボタン・メニュー・WM_DESTROY） */
	static LRESULT Void(CCmdTarget *p, WPARAM, LPARAM)
	{
		(This(p)->*F)();
		return 0;
	}
	/* WM_SIZE */
	static LRESULT Size(CCmdTarget *p, WPARAM wParam, LPARAM lParam)
	{
		(This(p)->*F)((UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		return 0;
	}
	/* WM_CREATE（-1 で作成失敗） */
	static LRESULT Create(CCmdTarget *p, WPARAM, LPARAM lParam)
	{
		return (LRESULT)(This(p)->*F)((LPCREATESTRUCT)lParam);
	}
	/* WM_GETMINMAXINFO */
	static LRESULT MinMax(CCmdTarget *p, WPARAM, LPARAM lParam)
	{
		(This(p)->*F)((MINMAXINFO *)lParam);
		return 0;
	}
	/* WM_ERASEBKGND */
	static LRESULT EraseBkgnd(CCmdTarget *p, WPARAM, LPARAM)
	{
		return (LRESULT)(This(p)->*F)(NULL);
	}
	/* ON_MESSAGE（VC6 の頃は void を返すハンドラも書けたので両方受ける） */
	static LRESULT Message(CCmdTarget *p, WPARAM wParam, LPARAM lParam)
	{
		if constexpr (std::is_void<decltype((This(p)->*F)(wParam, lParam))>::value) {
			(This(p)->*F)(wParam, lParam);
			return 0;
		} else {
			return (LRESULT)(This(p)->*F)(wParam, lParam);
		}
	}
};

#define ON_COMMAND(id, fn)			{WM_COMMAND, 0, (UINT)(id), &AfxThunk<&ThisClass::fn>::Void},
#define ON_BN_CLICKED(id, fn)		{WM_COMMAND, BN_CLICKED, (UINT)(id), &AfxThunk<&ThisClass::fn>::Void},
#define ON_WM_SIZE()				{WM_SIZE, 0, 0, &AfxThunk<&ThisClass::OnSize>::Size},
#define ON_WM_CREATE()				{WM_CREATE, 0, 0, &AfxThunk<&ThisClass::OnCreate>::Create},
#define ON_WM_DESTROY()				{WM_DESTROY, 0, 0, &AfxThunk<&ThisClass::OnDestroy>::Void},
#define ON_WM_GETMINMAXINFO()		{WM_GETMINMAXINFO, 0, 0, &AfxThunk<&ThisClass::OnGetMinMaxInfo>::MinMax},
#define ON_WM_ERASEBKGND()			{WM_ERASEBKGND, 0, 0, &AfxThunk<&ThisClass::OnEraseBkgnd>::EraseBkgnd},
#define ON_MESSAGE(msg, fn)			{(UINT)(msg), 0, 0, &AfxThunk<&ThisClass::fn>::Message},

/* ------------------------------------------------------------------------- */
/* 基本クラス																 */
/* ------------------------------------------------------------------------- */
class CObject
{
public:
	virtual ~CObject() {}
};

class CCmdTarget : public CObject
{
public:
	static const AFX_MSGMAP *GetThisMessageMap(void);
	virtual const AFX_MSGMAP *GetMessageMap(void) const;
};

class CDC;

/* DDX の受け渡し */
class CDataExchange
{
public:
	CDataExchange(CWnd *pDlgWnd, BOOL bSaveAndValidate) : m_bSaveAndValidate(bSaveAndValidate), m_pDlgWnd(pDlgWnd) {}

	BOOL	m_bSaveAndValidate;			/* TRUE なら画面 → 変数 */
	CWnd	*m_pDlgWnd;
};

/* ------------------------------------------------------------------------- */
/* ウィンドウ																 */
/* ------------------------------------------------------------------------- */
class CWnd : public CCmdTarget
{
public:
			CWnd();
	virtual ~CWnd();

	operator HWND() const					{ return m_hWnd; }
	HWND	GetSafeHwnd(void) const			{ return this ? m_hWnd : NULL; }
	static CWnd *FromHandle(HWND hWnd);
	static CWnd *FromHandlePermanent(HWND hWnd);

	BOOL	Attach(HWND hWnd);
	HWND	Detach(void);

	BOOL	CreateEx(DWORD dwExStyle, LPCSTR pszClassName, LPCSTR pszWindowName, DWORD dwStyle,
				int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, LPVOID pParam = NULL);
	virtual BOOL DestroyWindow(void);

	BOOL	ShowWindow(int nCmdShow)		{ return ::ShowWindow(m_hWnd, nCmdShow); }
	BOOL	EnableWindow(BOOL bEnable = TRUE)	{ return ::EnableWindow(m_hWnd, bEnable); }
	BOOL	IsWindowVisible(void) const		{ return ::IsWindowVisible(m_hWnd); }
	BOOL	SetWindowPos(const CWnd *pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
	void	MoveWindow(int x, int y, int cx, int cy, BOOL bRepaint = TRUE)	{ ::MoveWindow(m_hWnd, x, y, cx, cy, bRepaint); }
	void	MoveWindow(LPCRECT pRect, BOOL bRepaint = TRUE);
	void	GetClientRect(LPRECT pRect) const	{ ::GetClientRect(m_hWnd, pRect); }
	void	GetWindowRect(LPRECT pRect) const	{ ::GetWindowRect(m_hWnd, pRect); }
	void	SetWindowText(LPCSTR psz)		{ ::SetWindowText(m_hWnd, psz); }
	void	GetWindowText(CString &str) const;
	int		GetWindowText(LPSTR psz, int nMax) const	{ return ::GetWindowText(m_hWnd, psz, nMax); }
	void	Invalidate(BOOL bErase = TRUE)	{ ::InvalidateRect(m_hWnd, NULL, bErase); }
	CWnd	*SetFocus(void)					{ return FromHandle(::SetFocus(m_hWnd)); }
	CWnd	*GetParent(void) const			{ return FromHandle(::GetParent(m_hWnd)); }
	CWnd	*GetDlgItem(int nID) const		{ return FromHandle(::GetDlgItem(m_hWnd, nID)); }
	BOOL	SetMenu(CMenu *pMenu);
	BOOL	PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0)	{ return ::PostMessage(m_hWnd, uMsg, wParam, lParam); }
	LRESULT	SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0)	{ return ::SendMessage(m_hWnd, uMsg, wParam, lParam); }
	BOOL	UpdateData(BOOL bSaveAndValidate = TRUE);

	virtual LRESULT	WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL	OnWndMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	virtual LRESULT	DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL	PreCreateWindow(CREATESTRUCT &cs);
	virtual void	DoDataExchange(CDataExchange *pDX)	{ (void)pDX; }
	virtual void	PostNcDestroy(void)	{}

	/* 既定のハンドラ（派生クラスから CWnd::OnXxx で呼ばれる） */
	int		OnCreate(LPCREATESTRUCT)			{ return (int)Default(); }
	void	OnDestroy(void)						{ Default(); }
	void	OnSize(UINT, int, int)				{ Default(); }
	void	OnGetMinMaxInfo(MINMAXINFO *)		{ Default(); }
	BOOL	OnEraseBkgnd(CDC *)					{ return (BOOL)Default(); }
	LRESULT	Default(void);

	HWND	m_hWnd;

	DECLARE_MESSAGE_MAP()
};

/* ------------------------------------------------------------------------- */
/* メニュー																	 */
/* ------------------------------------------------------------------------- */
class CMenu : public CObject
{
public:
	CMenu() : m_hMenu(NULL) {}
	virtual ~CMenu();

	BOOL	LoadMenu(LPCSTR pszName);
	BOOL	LoadMenu(UINT nID)				{ return LoadMenu(MAKEINTRESOURCE(nID)); }
	operator HMENU() const					{ return m_hMenu; }

	HMENU	m_hMenu;
};

/* ------------------------------------------------------------------------- */
/* フレーム・ダイアログ														 */
/* ------------------------------------------------------------------------- */
class CFrameWnd : public CWnd
{
public:
	BOOL	Create(LPCSTR pszClassName, LPCSTR pszWindowName, DWORD dwStyle, const RECT &rect,
				CWnd *pParentWnd = NULL, LPCSTR pszMenuName = NULL, DWORD dwExStyle = 0);
	virtual BOOL PreCreateWindow(CREATESTRUCT &cs);

	DECLARE_MESSAGE_MAP()
};

class CDialog;
/* モーダルが閉じた時の知らせ先（ブラウザ版の DoModalAsync 用） */
typedef void (CWnd::*AFX_PMODALEND)(CDialog *pDlg, int nResult, LPARAM lParam);

class CDialog : public CWnd
{
public:
			CDialog();
			CDialog(UINT nIDTemplate, CWnd *pParentWnd = NULL);
	virtual ~CDialog();

	BOOL	Create(UINT nIDTemplate, CWnd *pParentWnd = NULL);
	virtual int	DoModal(void);
	/* 閉じたら pTarget の pfnEnd を呼び、そのあとダイアログを delete する */
	void	DoModalAsync(CWnd *pTarget, AFX_PMODALEND pfnEnd, LPARAM lParam);
	void	EndDialog(int nResult);

	virtual BOOL	OnInitDialog(void);
	virtual void	OnOK(void);
	virtual void	OnCancel(void);
	virtual BOOL	OnWndMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	virtual LRESULT	DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void	PostNcDestroy(void);

public:
	UINT			m_nIDHelp;
	UINT			m_nIDTemplate;
	CWnd			*m_pParentWnd;
	int				m_nModalResult;
	BOOL			m_bModal;
	CWnd			*m_pModalTarget;
	AFX_PMODALEND	m_pfnModalEnd;
	LPARAM			m_lModalParam;

	DECLARE_MESSAGE_MAP()
};

/* ------------------------------------------------------------------------- */
/* コントロール																 */
/* ------------------------------------------------------------------------- */
class CListCtrl : public CWnd
{
public:
	int		InsertColumn(int nCol, LPCSTR pszHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
	int		InsertItem(int nItem, LPCSTR pszItem);
	BOOL	SetItemText(int nItem, int nSubItem, LPCSTR pszText);
	CString	GetItemText(int nItem, int nSubItem) const;
	BOOL	DeleteItem(int nItem)			{ return ListView_DeleteItem(m_hWnd, nItem); }
	BOOL	DeleteAllItems(void)			{ return ListView_DeleteAllItems(m_hWnd); }
	int		GetItemCount(void) const		{ return ListView_GetItemCount(m_hWnd); }
	int		GetNextItem(int nItem, int nFlags) const	{ return ListView_GetNextItem(m_hWnd, nItem, nFlags); }
	BOOL	SetItemState(int nItem, UINT nState, UINT nMask);
	UINT	GetItemState(int nItem, UINT nMask) const	{ return ListView_GetItemState(m_hWnd, nItem, nMask); }
	DWORD	SetExtendedStyle(DWORD dwNewStyle)	{ return ListView_SetExtendedListViewStyle(m_hWnd, dwNewStyle); }
	DWORD	GetExtendedStyle(void) const	{ return ListView_GetExtendedListViewStyle(m_hWnd); }
	BOOL	EnsureVisible(int nItem, BOOL bPartialOK)	{ return ListView_EnsureVisible(m_hWnd, nItem, bPartialOK); }
};

class CComboBox : public CWnd
{
public:
	int		AddString(LPCSTR psz)			{ return (int)SendMessage(CB_ADDSTRING, 0, (LPARAM)psz); }
	int		InsertString(int nIndex, LPCSTR psz)	{ return (int)SendMessage(CB_INSERTSTRING, (WPARAM)nIndex, (LPARAM)psz); }
	int		DeleteString(int nIndex)		{ return (int)SendMessage(CB_DELETESTRING, (WPARAM)nIndex, 0); }
	void	ResetContent(void)				{ SendMessage(CB_RESETCONTENT, 0, 0); }
	int		GetCount(void) const			{ return (int)::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0); }
	int		GetCurSel(void) const			{ return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0); }
	int		SetCurSel(int nSelect)			{ return (int)SendMessage(CB_SETCURSEL, (WPARAM)nSelect, 0); }
	DWORD_PTR	GetItemData(int nIndex) const	{ return (DWORD_PTR)::SendMessage(m_hWnd, CB_GETITEMDATA, (WPARAM)nIndex, 0); }
	int		SetItemData(int nIndex, DWORD_PTR dwData)	{ return (int)SendMessage(CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)dwData); }
};

class CEdit : public CWnd {};
class CButton : public CWnd
{
public:
	int		GetCheck(void) const			{ return (int)::SendMessage(m_hWnd, BM_GETCHECK, 0, 0); }
	void	SetCheck(int nCheck)			{ SendMessage(BM_SETCHECK, (WPARAM)nCheck, 0); }
};

/* ------------------------------------------------------------------------- */
/* DDX（画面とメンバ変数のやり取り）										 */
/* ------------------------------------------------------------------------- */
void AFXAPI DDX_Control(CDataExchange *pDX, int nIDC, CWnd &rControl);
void AFXAPI DDX_Text(CDataExchange *pDX, int nIDC, CString &value);
long long	AfxDDXGetNumber(CDataExchange *pDX, int nIDC, BOOL bSigned);
void		AfxDDXSetNumber(CDataExchange *pDX, int nIDC, long long value, BOOL bSigned);

/* 数値はどの整数型でも受ける（BYTE・WORD・int・DWORD など） */
template<class T>
inline typename std::enable_if<std::is_integral<T>::value>::type DDX_Text(CDataExchange *pDX, int nIDC, T &value)
{
	if (pDX->m_bSaveAndValidate) {
		value = (T)AfxDDXGetNumber(pDX, nIDC, std::is_signed<T>::value);
	} else {
		AfxDDXSetNumber(pDX, nIDC, (long long)value, std::is_signed<T>::value);
	}
}
void AFXAPI DDX_Check(CDataExchange *pDX, int nIDC, int &value);
void AFXAPI DDX_CBIndex(CDataExchange *pDX, int nIDC, int &index);

#endif /* __cplusplus */
