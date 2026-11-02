/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			情報ウィンドウ表示										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void MyTextOut(HDC, int, int, LPCSTR);								/* 文字列表示 */
void DrawWindow(HDC);												/* 各ウィンドウ表示 */
void DrawWindowInfo(HDC);											/* サーバー情報表示 */

/* ========================================================================= */
/* 関数名：MyTextOut														 */
/* 内容：文字列表示															 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void MyTextOut(HDC hDC, int x, int y, LPCSTR pStr)
{
	TextOut (hDC, x, y, pStr, strlen (pStr));
}

/* ========================================================================= */
/* 関数名：DrawWindow														 */
/* 内容：各ウィンドウ表示													 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void DrawWindow(HDC hDC)
{
	DrawWindowInfo (hDC);
}

/* ========================================================================= */
/* 関数名：DrawWindowInfo													 */
/* 内容：サーバー情報表示													 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void DrawWindowInfo(HDC hDC)
{
	HFONT hFont, hFontOld;
	char szTmp[100];
	DWORD dwTime = g_dwTime - g_dwServerStartTime;

	hFont = CreateFont (12, 0, 0, 0, FW_NORMAL,
			FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "ＭＳ ゴシック");
	SetBkMode (hDC, TRANSPARENT);
	hFontOld = (HFONT)SelectObject (hDC, hFont);

	SetTextColor (hDC, RGB(0, 255, 0));
	MyTextOut (hDC, 0, 0, 		"サーバー稼動時間");
	MyTextOut (hDC, 0, 12 * 1, 	"接続数");
	MyTextOut (hDC, 0, 12 * 2, 	"処理キャラ数");
	MyTextOut (hDC, 0, 12 * 3, 	"処理マップ数");

	SetTextColor (hDC, RGB(255, 255, 255));

	wsprintf (szTmp, "%04d:%02d:%02d",
			 dwTime / 3600000,
			 (dwTime % 3600000 - ((dwTime % 60000) / 1000)) / 60000,
			 (dwTime % 60000) / 1000);
	MyTextOut (hDC, 120, 0, szTmp);

	wsprintf (szTmp, "%d", g_dwOnline);
	MyTextOut (hDC, 120, 12 * 1, szTmp);

	wsprintf (szTmp, "%d", g_aCharInfo.GetSize ());
	MyTextOut (hDC, 120, 12 * 2, szTmp);

	wsprintf (szTmp, "%d", g_aMapInfo.GetSize ());
	MyTextOut (hDC, 120, 12 * 3, szTmp);

	SelectObject (hDC, hFontOld);
	DeleteObject (hFont);
}

/* Copyright(C)URARA-works 2003-2005 */
