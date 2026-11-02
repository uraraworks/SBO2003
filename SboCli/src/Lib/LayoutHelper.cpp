/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	LayoutHelper.h												 */
/* 内容：		ウィンドウサイズ変更時の配置補助クラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/06/17													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "LayoutHelper.h"


/* ========================================================================= */
/* 関数名：	CLayoutHelper::CLayoutHelper									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

CLayoutHelper::CLayoutHelper()
{
}


/* ========================================================================= */
/* 関数名：	CLayoutHelper::~CLayoutHelper									 */
/* 内容：	デストラクタ													 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

CLayoutHelper::~CLayoutHelper()
{
	m_vLayout.clear();
}


/* ========================================================================= */
/* 関数名：	CLayoutHelper::Initialize										 */
/* 内容：	初期化															 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

void CLayoutHelper::Initialize(HWND hBase)
{
	RECT rc;

	m_hBase = hBase;

	GetClientRect (m_hBase, &rc);
	m_sizeBase.cx = rc.right - rc.left;
	m_sizeBase.cy = rc.bottom - rc.top;
}


/* ========================================================================= */
/* 関数名：	CLayoutHelper::RegisterControl									 */
/* 内容：	コントロールの登録												 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

BOOL CLayoutHelper::RegisterControl(HWND hWnd, DWORD dwFlags)
{
	RECT rc;
	LH_CTRL_LAYOUT lhLayout;

	GetWindowRect (hWnd, &rc);
	ScreenToClient (m_hBase, (LPPOINT)&rc);
	ScreenToClient (m_hBase, ((LPPOINT)&rc) + 1);

	lhLayout.hWnd		= hWnd;
	lhLayout.x			= rc.left;
	lhLayout.y			= rc.top;
	lhLayout.cx			= rc.right - rc.left;
	lhLayout.cy			= rc.bottom - rc.top;
	lhLayout.dwFlags	= dwFlags;

	m_vLayout.push_back (lhLayout);

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CLayoutHelper::UnregisterControl								 */
/* 内容：	コントロールの解除												 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

BOOL CLayoutHelper::UnregisterControl(HWND hWnd)
{
	for (std::vector<LH_CTRL_LAYOUT>::iterator it = m_vLayout.begin (); it != m_vLayout.end (); ++ it) {
		if ((*it).hWnd == hWnd) {
			m_vLayout.erase (it);
			return TRUE;
		}
	}

	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CLayoutHelper::DoLayout											 */
/* 内容：	更新															 */
/* 日付：	2005/06/17														 */
/* ========================================================================= */

void CLayoutHelper::DoLayout(int cx, int cy)
{
	cx -= m_sizeBase.cx;
	cy -= m_sizeBase.cy;

	HDWP hdwp = BeginDeferWindowPos (m_vLayout.size ());

	for (std::vector<LH_CTRL_LAYOUT>::iterator it = m_vLayout.begin (); it != m_vLayout.end (); ++ it) {
		RECT rc;
		SetRect (&rc, (*it).x, (*it).y, (*it).x + (*it).cx, (*it).y + (*it).cy);
		if (((*it).dwFlags & LH_CTRL_X) != 0) {
			rc.left += cx;
			rc.right += cx;
		}
		if (((*it).dwFlags & LH_CTRL_Y) != 0) {
			rc.top += cy;
			rc.bottom += cy;
		}
		if (((*it).dwFlags & LH_CTRL_WIDTH) != 0) {
			rc.right += cx;
		}
		if (((*it).dwFlags & LH_CTRL_HEIGHT) != 0) {
			rc.bottom += cy;
		}
		hdwp = DeferWindowPos (hdwp, (*it).hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOOWNERZORDER | SWP_NOZORDER);
	}

	EndDeferWindowPos (hdwp);
}

/* Copyright(C)URARA-works 2005 */
