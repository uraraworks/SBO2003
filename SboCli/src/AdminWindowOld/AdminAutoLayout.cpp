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

#define	AAL_EDGE		48		/* 文字を描く画面で「右端・下端に接している」とみなす余白 */
#define	AAL_ROWGAP		24		/* 上下の段で「右寄せの並び」とみなすボタンどうしの間 */

/* 覚えている部品の位置 */
struct AAL_ITEM {
	RECT	rcBase;				/* 元の位置 */
	RECT	rcSet;				/* 最後にここで動かした位置 */
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

/* 文字を描く画面：右端・下端に接していて、その先に部品の無い一覧だけ広げる */
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
	int dx = AalMax (0, cx - (int)sizeBase.cx);
	int dy = AalMax (0, cy - (int)sizeBase.cy);

	for (size_t i = 0; i < aWork.size (); i ++) {
		AAL_WORK &stList = aWork[i];
		if (!stList.bList) {
			continue;
		}
		const RECT &rcL = stList.rcBase;
		BOOL bWidth  = (sizeBase.cx - rcL.right  <= AAL_EDGE);
		BOOL bHeight = (sizeBase.cy - rcL.bottom <= AAL_EDGE);
		for (size_t j = 0; j < aWork.size (); j ++) {
			if (j == i || aWork[j].bFill) {
				continue;
			}
			const RECT &rcO = aWork[j].rcBase;
			/* 右に部品がある */
			if (rcO.left >= rcL.right - 2 && IsOverlapV (rcO, rcL)) {
				bWidth = FALSE;
			}
			/* 下に部品がある */
			if (rcO.top >= rcL.bottom - 2 && IsOverlapH (rcO, rcL)) {
				bHeight = FALSE;
			}
		}
		if (bWidth) {
			stList.rcNew.right += dx;
		}
		if (bHeight) {
			stList.rcNew.bottom += dy;
		}
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

/* Copyright(C)URARA-works 2026 */
