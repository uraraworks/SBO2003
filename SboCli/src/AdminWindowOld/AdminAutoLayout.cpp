/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		管理者ウィンドウの大きさに合わせて一覧などを広げる 実装ファイル	 */
/* ファイル名：	AdminAutoLayout.cpp											 */
/* ========================================================================= */
/*
 * 管理者ウィンドウの画面は、部品の位置を数字で決め打ちしているものがほとんどなので、
 * 部品ごとに「元の位置」を覚えておき、増えた幅・高さだけ一覧を広げる・部品をずらす。
 *
 * ・元の位置は、その部品を初めて見た時の位置。
 *   ここで動かした位置と今の位置が違えば、画面側が動かしたものとして覚え直す。
 * ・元の大きさより小さくした時は、元の配置のまま（はみ出た所は隠れる）。
 */
#include "stdafx.h"
#include <map>
#include <vector>
#include <climits>
#include "AdminAutoLayout.h"

/* ========================================================================= */
/* 定数・構造体の定義														 */
/* ========================================================================= */

#define	AAL_ROWGAP		24		/* 上下の段で「右寄せの並び」とみなすボタンどうしの間 */

/* 覚えている部品の位置 */
struct AAL_ITEM {
	RECT	rcBase;				/* 元の位置 */
	RECT	rcSet;				/* 最後にここで動かした位置 */
};

/* 文字を描く画面のずらし方 */
struct AAL_SHIFT {
	BOOL	bValid;				/* ずらす一覧がある */
	SIZE	sizeBase;			/* 元の大きさ */
	RECT	rcMain;				/* 広げる一覧の元の位置（帯の境目） */
	int		dx;					/* 右横の帯をずらす幅 */
	int		dy;					/* 下の帯をずらす高さ */
};

/* 並べ直す時の作業用 */
struct AAL_WORK {
	HWND	hWnd;
	RECT	rcBase;				/* 元の位置 */
	RECT	rcNow;				/* 今の位置 */
	RECT	rcNew;				/* 動かす先 */
	BOOL	bList;				/* 一覧（リストビュー・リストボックス） */
	BOOL	bFill;				/* 親いっぱいの子ダイアログ */
};

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

static std::map<HWND, AAL_ITEM>	s_mapItem;		/* 部品ごとの元の位置 */
static std::map<HWND, SIZE>		s_mapBase;		/* 文字を描く画面の元の大きさ */
static std::map<HWND, AAL_SHIFT>	s_mapShift;		/* 文字を描く画面のずらし方 */

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* max・min はマクロのことがあるので自前で持つ */
static int AalMax(int a, int b)
{
	return (a > b) ? a : b;
}

static int AalMin(int a, int b)
{
	return (a < b) ? a : b;
}

static BOOL IsSameRect(const RECT &a, const RECT &b)
{
	return (a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom);
}

/* 縦の範囲が重なっているか */
static BOOL IsOverlapV(const RECT &a, const RECT &b)
{
	return (a.top < b.bottom && b.top < a.bottom);
}

/* 横の範囲が重なっているか */
static BOOL IsOverlapH(const RECT &a, const RECT &b)
{
	return (a.left < b.right && b.left < a.right);
}

/* 子ウィンドウを集める */
struct AAL_ENUM {
	HWND					hParent;
	std::vector<HWND>		*paChild;
};

static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
	AAL_ENUM *pEnum = (AAL_ENUM *)lParam;
	if (GetParent (hWnd) == pEnum->hParent) {
		pEnum->paChild->push_back (hWnd);
	}
	return TRUE;
}

/* 消えた部品の記録を捨てる */
static void CleanUp(void)
{
	std::map<HWND, AAL_ITEM>::iterator it = s_mapItem.begin ();
	while (it != s_mapItem.end ()) {
		if (IsWindow (it->first)) {
			++ it;
		} else {
			it = s_mapItem.erase (it);
		}
	}
	std::map<HWND, AAL_SHIFT>::iterator itShift = s_mapShift.begin ();
	while (itShift != s_mapShift.end ()) {
		if (IsWindow (itShift->first)) {
			++ itShift;
		} else {
			itShift = s_mapShift.erase (itShift);
		}
	}
	std::map<HWND, SIZE>::iterator itBase = s_mapBase.begin ();
	while (itBase != s_mapBase.end ()) {
		if (IsWindow (itBase->first)) {
			++ itBase;
		} else {
			itBase = s_mapBase.erase (itBase);
		}
	}
}

/* 子部品の一覧と元の位置を作る */
static void MakeWork(HWND hParent, std::vector<AAL_WORK> &aWork)
{
	std::vector<HWND> aChild;
	AAL_ENUM stEnum;
	stEnum.hParent	= hParent;
	stEnum.paChild	= &aChild;
	EnumChildWindows (hParent, EnumChildProc, (LPARAM)&stEnum);

	for (size_t i = 0; i < aChild.size (); i ++) {
		HWND hWnd = aChild[i];
		if ((GetWindowLong (hWnd, GWL_STYLE) & WS_CHILD) == 0) {
			continue;
		}
		AAL_WORK stWork;
		ZeroMemory (&stWork, sizeof (stWork));
		stWork.hWnd = hWnd;

		/* 親のクライアント座標にする */
		RECT rc;
		GetWindowRect (hWnd, &rc);
		POINT pt1 = {rc.left, rc.top}, pt2 = {rc.right, rc.bottom};
		ScreenToClient (hParent, &pt1);
		ScreenToClient (hParent, &pt2);
		SetRect (&stWork.rcNow, pt1.x, pt1.y, pt2.x, pt2.y);

		std::map<HWND, AAL_ITEM>::iterator it = s_mapItem.find (hWnd);
		if (it == s_mapItem.end () || !IsSameRect (it->second.rcSet, stWork.rcNow)) {
			/* 初めて見た・画面側が動かした → 今の位置を元の位置にする */
			AAL_ITEM stItem;
			stItem.rcBase	= stWork.rcNow;
			stItem.rcSet	= stWork.rcNow;
			s_mapItem[hWnd] = stItem;
			stWork.rcBase = stWork.rcNow;
		} else {
			stWork.rcBase = it->second.rcBase;
		}
		stWork.rcNew = stWork.rcBase;

		char szClass[64];
		ZeroMemory (szClass, sizeof (szClass));
		GetClassName (hWnd, szClass, sizeof (szClass) - 1);
		if (lstrcmpi (szClass, WC_LISTVIEW) == 0 || lstrcmpi (szClass, "ListBox") == 0) {
			stWork.bList = TRUE;
		} else if (lstrcmpi (szClass, "#32770") == 0 && stWork.rcBase.left <= 1 && stWork.rcBase.top <= 1) {
			stWork.bFill = TRUE;
		}
		aWork.push_back (stWork);
	}
}

/* 文字を描く画面：一番大きな一覧を広げ、一覧の下の部品は下へ、一覧の右横の部品は右へずらす。
   描いた文字も AdminAutoLayoutPaint で同じ帯ごとにずらすので、見出しと部品はずれない */
static void LayoutPainted(HWND hParent, std::vector<AAL_WORK> &aWork, int cx, int cy)
{
	SIZE sizeBase;
	std::map<HWND, SIZE>::iterator it = s_mapBase.find (hParent);
	if (it == s_mapBase.end ()) {
		sizeBase.cx = cx;
		sizeBase.cy = cy;
		s_mapBase[hParent] = sizeBase;
	} else {
		sizeBase = it->second;
	}

	AAL_SHIFT stShift;
	ZeroMemory (&stShift, sizeof (stShift));
	stShift.sizeBase = sizeBase;

	int nMain = -1, nArea = 0;
	for (size_t i = 0; i < aWork.size (); i ++) {
		if (!aWork[i].bList) {
			continue;
		}
		const RECT &rc = aWork[i].rcBase;
		int n = (rc.right - rc.left) * (rc.bottom - rc.top);
		if (n > nArea) {
			nArea = n;
			nMain = (int)i;
		}
	}
	if (nMain >= 0) {
		stShift.bValid	= TRUE;
		stShift.rcMain	= aWork[nMain].rcBase;
		stShift.dx		= AalMax (0, cx - (int)sizeBase.cx);
		stShift.dy		= AalMax (0, cy - (int)sizeBase.cy);
		/* 一覧の右端・下端は、帯の境目として 2 ドット内側に取る（枠線ぶんの誤差を許す） */
		stShift.rcMain.right	-= 2;
		stShift.rcMain.bottom	-= 2;
	}

	if (stShift.bValid) {
		const RECT &rcMain = stShift.rcMain;
		for (size_t i = 0; i < aWork.size (); i ++) {
			AAL_WORK &stWork = aWork[i];
			const RECT &rc = stWork.rcBase;
			if (stWork.bFill) {
				continue;
			}
			if ((int)i == nMain) {
				stWork.rcNew.right	+= stShift.dx;
				stWork.rcNew.bottom	+= stShift.dy;
			} else if (rc.top >= rcMain.bottom) {
				OffsetRect (&stWork.rcNew, 0, stShift.dy);			/* 一覧の下 */
			} else if (rc.left >= rcMain.right && rc.top >= rcMain.top) {
				OffsetRect (&stWork.rcNew, stShift.dx, 0);			/* 一覧の右横 */
			}
		}
	}

	/* ずらし方が変わったら、描いた文字も描き直す */
	std::map<HWND, AAL_SHIFT>::iterator itShift = s_mapShift.find (hParent);
	if (itShift == s_mapShift.end () || memcmp (&itShift->second, &stShift, sizeof (stShift)) != 0) {
		s_mapShift[hParent] = stShift;
		InvalidateRect (hParent, NULL, TRUE);
	}
}

/* ダイアログ：一番大きな一覧を広げ、右・下の部品をずらす */
static void LayoutDialog(std::vector<AAL_WORK> &aWork, int cx, int cy)
{
	int nMain = -1, nArea = 0;
	int nLeft = INT_MAX, nTop = INT_MAX, nRight = 0, nBottom = 0;
	for (size_t i = 0; i < aWork.size (); i ++) {
		const RECT &rc = aWork[i].rcBase;
		nLeft	= AalMin (nLeft,   (int)rc.left);
		nTop	= AalMin (nTop,    (int)rc.top);
		nRight	= AalMax (nRight,  (int)rc.right);
		nBottom	= AalMax (nBottom, (int)rc.bottom);
		if (aWork[i].bList) {
			int n = (rc.right - rc.left) * (rc.bottom - rc.top);
			if (n > nArea) {
				nArea = n;
				nMain = (int)i;
			}
		}
	}
	if (nMain < 0) {
		return;
	}
	/* 元の大きさは、部品の外側に左上と同じだけ余白を足した大きさ */
	nLeft = AalMax (0, nLeft);
	nTop  = AalMax (0, nTop);
	int dx = AalMax (0, cx - (nRight + nLeft));
	int dy = AalMax (0, cy - (nBottom + nTop));
	const RECT rcMain = aWork[nMain].rcBase;

	/* 上下の段（一覧より上・下）にある部品のうち、右端から並んでいるものは右寄せにする */
	std::vector<BOOL> abRight (aWork.size (), FALSE);
	for (BOOL bChange = TRUE; bChange; ) {
		bChange = FALSE;
		for (size_t i = 0; i < aWork.size (); i ++) {
			const RECT &rc = aWork[i].rcBase;
			if (abRight[i] || (int)i == nMain || aWork[i].bFill) {
				continue;
			}
			BOOL bRow = (rc.bottom <= rcMain.top + 2 || rc.top >= rcMain.bottom - 2);
			if (!bRow) {
				continue;
			}
			BOOL bAnchor = (nRight - rc.right <= AAL_ROWGAP);
			for (size_t j = 0; j < aWork.size () && !bAnchor; j ++) {
				const RECT &rcO = aWork[j].rcBase;
				if (abRight[j] && IsOverlapV (rc, rcO) && rcO.left >= rc.right &&
						rcO.left - rc.right <= AAL_ROWGAP) {
					bAnchor = TRUE;
				}
			}
			if (bAnchor) {
				abRight[i] = TRUE;
				bChange = TRUE;
			}
		}
	}

	for (size_t i = 0; i < aWork.size (); i ++) {
		AAL_WORK &stWork = aWork[i];
		const RECT &rc = stWork.rcBase;
		if ((int)i == nMain) {
			stWork.rcNew.right	+= dx;
			stWork.rcNew.bottom	+= dy;
			continue;
		}
		if (stWork.bFill) {
			continue;
		}
		int nOffX = 0, nOffY = 0;
		if (rc.top >= rcMain.bottom - 2) {
			nOffY = dy;					/* 一覧の下 */
		}
		if (rc.left >= rcMain.right - 2 || abRight[i]) {
			nOffX = dx;					/* 一覧の右・右寄せの並び */
		}
		OffsetRect (&stWork.rcNew, nOffX, nOffY);
	}
}

/* ========================================================================= */
/* 関数名：	AdminAutoLayoutInit												 */
/* 内容：	元の大きさを覚える												 */
/* ========================================================================= */

void AdminAutoLayoutInit(HWND hWnd)
{
	RECT rc;
	CleanUp ();
	GetClientRect (hWnd, &rc);
	SIZE size;
	size.cx = rc.right;
	size.cy = rc.bottom;
	s_mapBase[hWnd] = size;
}

/* ========================================================================= */
/* 関数名：	AdminAutoLayout													 */
/* 内容：	今の大きさに合わせて中の部品を並べ直す							 */
/* ========================================================================= */

void AdminAutoLayout(HWND hWnd, BOOL bPainted)
{
	if (!IsWindow (hWnd)) {
		return;
	}
	RECT rcClient;
	GetClientRect (hWnd, &rcClient);

	std::vector<AAL_WORK> aWork;
	MakeWork (hWnd, aWork);
	if (bPainted) {
		LayoutPainted (hWnd, aWork, rcClient.right, rcClient.bottom);
	} else {
		LayoutDialog (aWork, rcClient.right, rcClient.bottom);
	}

	for (size_t i = 0; i < aWork.size (); i ++) {
		AAL_WORK &stWork = aWork[i];
		if (stWork.bFill) {
			SetRect (&stWork.rcNew, 0, 0, rcClient.right, rcClient.bottom);
		}
		if (!IsSameRect (stWork.rcNew, stWork.rcNow)) {
			SetWindowPos (stWork.hWnd, NULL, stWork.rcNew.left, stWork.rcNew.top,
					stWork.rcNew.right - stWork.rcNew.left, stWork.rcNew.bottom - stWork.rcNew.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
		}
		std::map<HWND, AAL_ITEM>::iterator it = s_mapItem.find (stWork.hWnd);
		if (it != s_mapItem.end ()) {
			it->second.rcSet = stWork.rcNew;
		}
		/* 子ダイアログの中も並べ直す */
		if (stWork.bFill) {
			AdminAutoLayout (stWork.hWnd, FALSE);
		}
	}

	if (bPainted) {
		CleanUp ();
	}
}

/* ========================================================================= */
/* 関数名：	AdminAutoLayoutPaint											 */
/* 内容：	文字を描く画面を、部品と同じずらし方で描く						 */
/* ========================================================================= */

void AdminAutoLayoutPaint(HWND hWnd, HDC hDC, void (*pfnPaint)(HDC))
{
	std::map<HWND, AAL_SHIFT>::iterator it = s_mapShift.find (hWnd);
	if (it == s_mapShift.end () || !it->second.bValid || (it->second.dx == 0 && it->second.dy == 0)) {
		/* ずらさない時は、そのまま描く */
		pfnPaint (hDC);
		return;
	}
	const AAL_SHIFT &stShift = it->second;
	int cx = stShift.sizeBase.cx, cy = stShift.sizeBase.cy;
	const RECT &rcMain = stShift.rcMain;

	/* 元の大きさの裏画面に元の位置で描いてから、帯ごとにずらして写す */
	HDC hDCMem = CreateCompatibleDC (hDC);
	HBITMAP hBmp = CreateCompatibleBitmap (hDC, cx, cy);
	HGDIOBJ hBmpOld = SelectObject (hDCMem, hBmp);
	RECT rcAll = {0, 0, cx, cy};
	FillRect (hDCMem, &rcAll, (HBRUSH)GetStockObject (BLACK_BRUSH));
	pfnPaint (hDCMem);

	int nTop = AalMax (0, AalMin (cy, (int)rcMain.top));
	int nBottom = AalMax (nTop, AalMin (cy, (int)rcMain.bottom));
	int nRight = AalMax (0, AalMin (cx, (int)rcMain.right));
	/* 一覧より上 */
	BitBlt (hDC, 0, 0, cx, nTop, hDCMem, 0, 0, SRCCOPY);
	/* 一覧の段の左側（一覧そのものの場所を含む） */
	BitBlt (hDC, 0, nTop, nRight, nBottom - nTop, hDCMem, 0, nTop, SRCCOPY);
	/* 一覧の段の右横 → 右へ */
	BitBlt (hDC, nRight + stShift.dx, nTop, cx - nRight, nBottom - nTop, hDCMem, nRight, nTop, SRCCOPY);
	/* 一覧より下 → 下へ */
	BitBlt (hDC, 0, nBottom + stShift.dy, cx, cy - nBottom, hDCMem, 0, nBottom, SRCCOPY);

	SelectObject (hDCMem, hBmpOld);
	DeleteObject (hBmp);
	DeleteDC (hDCMem);
}

/* Copyright(C)URARA-works 2026 */
