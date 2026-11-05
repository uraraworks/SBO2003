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
 *                    描いた文字とずれないよう、右端・下端に接している一覧だけを広げる。
 * bPainted = FALSE : ダイアログ（文字もスタティック部品）。
 *                    一番大きな一覧を広げ、その右・下にある部品はずらす。
 * どちらも、中にある全面のダイアログ（子ダイアログ）は親いっぱいに広げて、その中も並べ直す。
 */

void AdminAutoLayoutInit	(HWND hWnd);					/* 元の大きさを覚える */
void AdminAutoLayout		(HWND hWnd, BOOL bPainted);		/* 並べ直す */

/* Copyright(C)URARA-works 2026 */
