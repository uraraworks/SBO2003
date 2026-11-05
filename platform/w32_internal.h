/* ========================================================================= */
/* ファイル名：	w32_internal.h												 */
/* 概要：		Win32 互換レイヤの内部定義（互換レイヤの .cpp 同士で共有）	 */
/* ========================================================================= */
#pragma once

#include "include/win32compat.h"

/* 互換層の中では std::min / std::max を使うので、ゲーム用のマクロは外す */
#undef min
#undef max
#include <string>
#include <vector>

/* ------------------------------------------------------------------------- */
/* ウィンドウ																 */
/* ------------------------------------------------------------------------- */

/* ウィンドウの種類（コントロールは HTML の部品で代用する） */
enum W32WndKind {
	W32WND_NORMAL = 0,			/* 普通のウィンドウ */
	W32WND_EDIT,				/* エディット → <input> / <textarea> */
	W32WND_BUTTON,				/* ボタン → <button> / チェックボックス */
	W32WND_RICHEDIT,			/* リッチエディット（ログ表示。画面には出さない） */
	W32WND_STATIC,				/* スタティック */
	W32WND_COMBOBOX,			/* コンボボックス → <select> */
	W32WND_LISTBOX,				/* リストボックス → <select size> */
	W32WND_SCROLLBAR,			/* スクロールバー */
	W32WND_LISTVIEW,			/* リストビュー → <table> */
};

/* リストビューの 1 行 */
struct W32LvRow {
	std::vector<std::string>	aText;		/* 列ごとの文字（SJIS） */
	LPARAM						lParam;
	UINT						uState;		/* LVIS_SELECTED / チェック状態(LVIS_STATEIMAGEMASK) */
};

/* メニュー */
struct W32Menu;
struct W32MenuItem {
	UINT		uFlags;				/* MF_POPUP / MF_SEPARATOR / MF_CHECKED / MF_GRAYED */
	UINT		uID;
	std::string	strText;			/* SJIS */
	W32Menu		*pSub;
};
struct W32Menu {
	DWORD						dwMagic;
	std::vector<W32MenuItem>	aItem;
	HWND						hWnd;		/* メニューバーとして付いているウィンドウ */
};
#define W32MENU_MAGIC	0x57334D4E		/* 'W3MN' */
W32Menu	*W32GetMenu(HMENU hMenu);

struct W32Wnd {
	DWORD			dwMagic;			/* 正しいハンドルか判定する印 */
	std::string		strClass;			/* クラス名 */
	std::string		strText;			/* ウィンドウテキスト */
	WNDPROC			pfnWndProc;			/* ウィンドウプロシージャ */
	HWND			hWndParent;			/* 親ウィンドウ */
	DWORD			dwStyle;			/* スタイル */
	DWORD			dwExStyle;			/* 拡張スタイル */
	int				nID;				/* コントロールID */
	RECT			rc;					/* 位置（子は親のクライアント座標、トップレベルは画面座標） */
	BOOL			bVisible;			/* 表示中 */
	BOOL			bEnabled;			/* 有効 */
	BOOL			bInvalid;			/* 再描画が必要 */
	BOOL			bDestroyed;			/* 破棄済み */
	LONG			lUserData;			/* GWL_USERDATA */
	W32WndKind		nKind;				/* 種類 */
	int				nCheck;				/* チェック状態(ボタン) */
	int				nHtmlID;			/* 対応する HTML 部品の番号（0 なら無し） */
	BOOL			bScreen;			/* ブラウザの画面(canvas)に対応するメインウィンドウ */

	/* ---- ここから小窓（管理者ウィンドウ・ダイアログ）用 ---- */
	HWND			hWndOwner = NULL;	/* オーナー（WS_CHILD でないウィンドウの親） */
	HBRUSH			hbrBackground = NULL;	/* クラスの背景ブラシ */
	BOOL			bPopup = FALSE;		/* ブラウザに小窓として出すトップレベルウィンドウ */
	HMENU			hMenu = NULL;		/* メニューバー */
	BOOL			bDialog = FALSE;	/* ダイアログ */
	DLGPROC			pfnDlgProc = NULL;	/* ダイアログプロシージャ */
	LONG_PTR		lDlgMsgResult = 0;	/* DWL_MSGRESULT */
	LONG_PTR		lDlgUser = 0;		/* DWL_USER */
	HBITMAP			hBmpSurface = NULL;	/* GDI で描いた内容（小窓の中のウィンドウだけ） */
	/* コンボボックス・リストボックス */
	std::vector<std::string>	aItems;	/* 項目（SJIS） */
	std::vector<LPARAM>			aItemData;
	int				nCurSel = -1;
	/* スクロールバー（SB_CTL。普通のウィンドウでは SB_HORZ/SB_VERT 用に覚えるだけ） */
	int				nScrMin = 0, nScrMax = 100, nScrPos = 0;
	UINT			nScrPage = 0;
	/* リストビュー */
	std::vector<std::string>	aColText;
	std::vector<int>			aColWidth;
	std::vector<W32LvRow>		aRows;
	DWORD			dwLvExStyle = 0;
};

#define W32WND_MAGIC	0x57334E44		/* 'W3ND' */

/* 有効なウィンドウハンドルなら構造体を返す */
W32Wnd	*W32GetWnd(HWND hWnd);
/* 小窓の中にあるウィンドウなら、その小窓（トップレベル）を返す */
W32Wnd	*W32GetPopupRoot(W32Wnd *pWnd);
/* HTML 部品の番号からウィンドウを探す */
W32Wnd	*W32FindWndByHtmlID(int nHtmlID);
/* 小窓の枠（タイトル・メニュー・縁）の幅。クライアント領域 = rc からこれを除いたところ */
void	W32GetFrameInsets(const W32Wnd *pWnd, RECT *pInsets);
/* 次に作るダイアログのプロシージャ（CreateDialogParam から） */
void	W32SetNextDlgProc(DLGPROC pfn);
/* コンボボックス・リストボックス・スクロールバー・リストビューのプロシージャ（w32_ctrl.cpp） */
LRESULT CALLBACK W32CtrlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/* ダイアログクラス(#32770)のプロシージャ（w32_dlg.cpp） */
LRESULT CALLBACK W32DialogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/* 一覧（w32_core.cpp） */
const std::vector<W32Wnd *> &W32GetWndList(void);
/* ダイアログ単位 → ピクセル */
#define W32DLU_X(v)		((v) * 6 / 4)
#define W32DLU_Y(v)		((v) * 12 / 8)
/* ブラウザの画面に対応するメインウィンドウ */
HWND	W32GetScreenWnd(void);
/* 画面サイズ（メインウィンドウのクライアント領域） */
void	W32GetScreenSize(int *pcx, int *pcy);

/* ------------------------------------------------------------------------- */
/* GDI																		 */
/* ------------------------------------------------------------------------- */

enum W32GdiType {
	W32GDI_BITMAP = 1,
	W32GDI_FONT,
	W32GDI_BRUSH,
	W32GDI_PEN,
};

struct W32GdiObj {
	DWORD		dwMagic;
	W32GdiType	nType;
	BOOL		bStock;				/* ストックオブジェクト（削除しない） */
	/* ビットマップ（32bit ボトムアップ DIB に統一） */
	int			cx, cy;
	BYTE		*pBits;
	BOOL		bOwnBits;
	BOOL		bTopDown;			/* biHeight が負で作られた */
	/* フォント */
	int			nHeight;
	int			nWeight;
	BOOL		bItalic;
	/* ブラシ・ペン */
	COLORREF	cl;
	BOOL		bNull;
};

#define W32GDI_MAGIC	0x57334744		/* 'W3GD' */

struct W32Dc {
	DWORD		dwMagic;
	HWND		hWnd;				/* ウィンドウの DC ならそのウィンドウ */
	HBITMAP		hBmp;				/* 選択中のビットマップ */
	HBITMAP		hBmpDefault;		/* 作成時の 1x1 ビットマップ */
	HFONT		hFont;				/* 選択中のフォント */
	HBRUSH		hBrush;				/* 選択中のブラシ */
	HPEN		hPen;				/* 選択中のペン */
	COLORREF	clText;
	COLORREF	clBk;
	int			nBkMode;
	POINT		ptPen;
};

#define W32DC_MAGIC		0x57334443		/* 'W3DC' */

W32Dc		*W32GetDc(HDC hDC);
W32GdiObj	*W32GetGdi(HGDIOBJ hObj);
/* 小窓の中のウィンドウが持つビットマップを捨てる */
void		W32FreeWndSurface(W32Wnd *pWnd);
/* ReleaseDC されずに描かれた小窓の面を画面へ出す（PeekMessage のたびに呼ぶ） */
void		W32FlushSurfaces(void);
/* 画面用のビットマップ（メインウィンドウの DC はここに描く） */
HBITMAP		W32GetScreenBitmap(void);
/* 画面用のビットマップをブラウザの canvas へ転送する */
void		W32PresentScreen(void);

/* ------------------------------------------------------------------------- */
/* 文字コード（w32_sjis.cpp）												 */
/* ------------------------------------------------------------------------- */
/* SJIS → UTF-16 のコードポイント列（サロゲートは使わない） */
void		W32SjisToUcs(const char *pszSjis, int nLen, std::vector<unsigned short> &aOut);
std::string	W32SjisToUtf8Str(const char *pszSjis, int nLen = -1);
std::string	W32Utf8ToSjisStr(const char *pszUtf8);
/* 内部の文字列（UTF-8）を正しい UTF-8 にする（ブラウザ・SDL_ttf に渡す時） */
std::string	W32TextToUtf8Str(const char *psz, int nLen = -1);
/* ブラウザから受け取った UTF-8 を内部の文字列にする */
std::string	W32Utf8ToTextStr(const char *pszUtf8);

/* ------------------------------------------------------------------------- */
/* HTML 部品（w32_html.cpp）												 */
/* ------------------------------------------------------------------------- */
void		W32HtmlCreate(W32Wnd *pWnd);
void		W32HtmlDestroy(W32Wnd *pWnd);
void		W32HtmlUpdate(W32Wnd *pWnd);			/* 位置・表示状態・有効状態を反映 */
void		W32HtmlSetText(W32Wnd *pWnd);			/* strText を部品へ反映 */
void		W32HtmlGetText(W32Wnd *pWnd);			/* 部品の内容を strText へ取り込む */
void		W32HtmlFocus(W32Wnd *pWnd);			/* 部品にフォーカスを移す(NULL ならゲーム画面へ) */
/* コンボボックス・リストボックスの項目 nOp 0:全消し 1:挿入 2:削除 3:選択 */
void		W32HtmlItems(W32Wnd *pWnd, int nOp, int nIndex, const char *pszText);
void		W32HtmlScroll(W32Wnd *pWnd);			/* スクロールバーの範囲と位置 */
/* リストビュー nOp 0:行を全消し 1:列を挿入 2:行を挿入 3:文字を設定 4:行を削除 5:行の状態 6:拡張スタイル 7:見える所まで動かす */
void		W32HtmlListView(W32Wnd *pWnd, int nOp, int nRow, int nCol);
void		W32HtmlPaint(W32Wnd *pWnd, W32GdiObj *pBmp);	/* GDI で描いた内容を表示 */
void		W32HtmlMenu(W32Wnd *pWnd);				/* メニューバー */
