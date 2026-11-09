/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	AdminAutoLayout.h											 */
/* 内容：		管理者ウィンドウの大きさに合わせて一覧などを広げる 定義ファイル	 */
/* ========================================================================= */

#pragma once

/*
 * 使い方
 *   AdminAutoLayoutInit (hWnd);			… 作った直後の大きさを「元の大きさ」として覚える
 *   AdminAutoLayout (hWnd, bPainted);	… 今の大きさに合わせて中の部品を並べ直す
 *
 * bPainted = TRUE  : 文字を自分で描いている画面（管理者ウィンドウ本体）。
 *                    一番大きな一覧を広げ、一覧の下の部品は下へ、一覧の右横の部品は右へずらす。
 *                    描く処理は AdminAutoLayoutPaint を通すと、描いた文字も同じだけずれる。
 * bPainted = FALSE : ダイアログ（文字もスタティック部品）。
 *                    一番大きな一覧を広げ、その右・下にある部品はずらす。
 * どちらも、中にある全面のダイアログ（子ダイアログ）は親いっぱいに広げて、その中も並べ直す。
 */

void AdminAutoLayoutInit	(HWND hWnd);					/* 元の大きさを覚える */
void AdminAutoLayout		(HWND hWnd, BOOL bPainted);		/* 並べ直す */
void AdminAutoLayoutPaint	(HWND hWnd, HDC hDC, void (*pfnPaint)(HDC));	/* ずらして描く */

/*
 * 縦スクロール（bPainted = TRUE の画面だけ）
 *   中身（部品の下端・申告した高さ）が見えている高さより高いと、右端にスクロールバーが出て、
 *   ホイールでも動かせる。部品も描いた文字も同じだけ上へずれる。
 */
void AdminAutoLayoutSetContentHeight	(HWND hWnd, int nHeight);	/* 中身の高さを申告する(0=申告なし) */
void AdminAutoLayoutResetScroll			(HWND hWnd);				/* 画面を切り替えた時に戻す */
int  AdminAutoLayoutGetScrollY			(HWND hWnd);				/* 今のスクロール位置 */
BOOL AdminAutoLayoutOnScrollMsg			(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);	/* ホイール・スクロールバー */

/* Copyright(C)URARA-works 2026 */
