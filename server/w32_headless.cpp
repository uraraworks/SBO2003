/* ========================================================================= */
/* ファイル名：	w32_headless.cpp											 */
/* 概要：		Win32 互換レイヤ：画面なし（Node.js 版サーバー）用の部品		 */
/* ========================================================================= */
/*
 * ブラウザ版では EDIT / BUTTON を HTML の部品で代用しているが、
 * サーバーは画面を持たないので何もしない。文字列は W32Wnd::strText に
 * 持ったままなので、GetWindowText などはそのまま動く。
 *
 * リソース DLL も使わないので、リソース表は空にしておく。
 */
#include "../platform/w32_internal.h"
#include "w32_resources.h"

void W32HtmlCreate(W32Wnd *)	{}
void W32HtmlSetNoIme(W32Wnd *, BOOL)	{}
void W32HtmlDestroy(W32Wnd *)	{}
void W32HtmlUpdate(W32Wnd *)	{}
void W32HtmlSetText(W32Wnd *)	{}
void W32HtmlGetText(W32Wnd *)	{}
void W32HtmlFocus(W32Wnd *)		{}
void W32HtmlItems(W32Wnd *, int, int, const char *)	{}
void W32HtmlScroll(W32Wnd *)	{}
void W32HtmlListView(W32Wnd *, int, int, int)	{}
void W32HtmlPaint(W32Wnd *, W32GdiObj *)	{}
void W32HtmlMenu(W32Wnd *)		{}

/* リソース表（空。最後の印だけ置く） */
const W32ResEntry g_aW32ResEntry[] = {
	{NULL, NULL, 0, NULL, NULL, 0}
};
const W32StrEntry g_aW32StrEntry[] = {
	{NULL, 0, NULL}
};
const W32DlgEntry g_aW32DlgEntry[] = {
	{NULL, 0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL}
};
const W32MenuEntry g_aW32MenuEntry[] = {
	{NULL, 0, NULL}
};
