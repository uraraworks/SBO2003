/* ========================================================================= */
/* ファイル名：	w32_resources.h												 */
/* 概要：		リソース表の型（中身は gen_resources.py が生成する）			 */
/* ========================================================================= */
#pragma once

#include <stddef.h>

/* リソース 1 件 */
struct W32ResEntry {
	const char	*pszModule;		/* モジュール名（例: "SboGrpData.dll"） */
	const char	*pszType;		/* 種類（大文字。例: "PNG", "WAVE"） */
	int			nID;			/* 数値ID（名前で引くものは 0） */
	const char	*pszName;		/* 名前（数値IDのものは NULL） */
	const char	*pszPath;		/* 仮想ファイルシステム上のパス */
	int			bNoFile;		/* ファイルを preload していない（MIDI など） */
};

/* 文字列リソース 1 件 */
struct W32StrEntry {
	const char	*pszModule;		/* モジュール名 */
	int			nID;			/* 文字列ID */
	const char	*pszText;		/* 内容（SJIS） */
};

extern const W32ResEntry g_aW32ResEntry[];
extern const W32StrEntry g_aW32StrEntry[];

/* ダイアログのコントロール 1 個（.rc の 1 行） */
struct W32DlgItem {
	const char	*pszClass;		/* クラス名（"BUTTON" "SysListView32" など） */
	const char	*pszText;		/* 表示する文字（SJIS） */
	int			nID;			/* コントロールID（IDC_STATIC は -1） */
	int			x, y, cx, cy;	/* 位置と大きさ（ダイアログ単位） */
	unsigned long	dwStyle;
	unsigned long	dwExStyle;
};

/* ダイアログ 1 個 */
struct W32DlgEntry {
	const char	*pszName;		/* 名前（"IDD_MAP_PARTS"） */
	int			nID;			/* 数値ID */
	int			x, y, cx, cy;	/* 位置と大きさ（ダイアログ単位） */
	unsigned long	dwStyle;
	unsigned long	dwExStyle;
	const char	*pszCaption;	/* タイトル（SJIS） */
	int			nFontSize;		/* フォントの大きさ（ポイント） */
	const W32DlgItem *pItems;	/* コントロール（pszClass が NULL で終わり） */
};

/* メニューの 1 行 */
enum {
	W32MENU_ITEM = 1,			/* 項目 */
	W32MENU_POPUP,				/* サブメニューの始まり */
	W32MENU_SEPARATOR,			/* 区切り線 */
	W32MENU_END,				/* サブメニューの終わり（表全体の終わりにも使う） */
};
struct W32MenuTplItem {
	int			nKind;
	int			nID;
	const char	*pszText;		/* SJIS */
};

/* メニュー 1 個 */
struct W32MenuEntry {
	const char	*pszName;
	int			nID;
	const W32MenuTplItem *pItems;	/* pszText が NULL の W32MENU_END で終わり */
};

extern const W32DlgEntry g_aW32DlgEntry[];
extern const W32MenuEntry g_aW32MenuEntry[];
