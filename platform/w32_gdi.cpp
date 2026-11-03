/* ========================================================================= */
/* ファイル名：	w32_gdi.cpp													 */
/* 概要：		Win32 互換レイヤ：GDI（DC・ビットマップ・ブラシ・文字描画）	 */
/* ========================================================================= */
/*
 * 仕組み
 *  - ビットマップはすべて 32bit/画素（BGRX）。CreateDIBSection で作ったものは
 *    Win32 と同じメモリ配置（biHeight が正ならボトムアップ）なので、
 *    ゲーム側が GetBits() で直接いじっても同じ結果になる。
 *  - メインウィンドウの DC は「画面用ビットマップ」に描き、
 *    EndPaint / ReleaseDC の時にブラウザの canvas へ転送する。
 *  - 文字は SDL_ttf で 1bit（アンチエイリアス無し）に描く。
 *    元の MS ゴシックのビットマップフォントに近い見た目で、
 *    カラーキー抜きの合成でも縁がにじまない。
 *  - サーバー（画面なし）では、画面への転送と文字の描画を省く。
 */
#include "w32_internal.h"

/* ブラウザ版だけ画面と文字を描く。サーバー（画面なし）は画面を持たないので、
 * DC・ビットマップの管理だけ行い、画面への転送と文字の描画はしない */
#ifdef W32_BROWSER
#define W32_USE_TTF
#include <emscripten.h>
#include <SDL_ttf.h>
#endif
#include <map>
#include <list>
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* オブジェクト管理															 */
/* ------------------------------------------------------------------------- */
static std::vector<W32Dc *>		s_apDc;
static std::vector<W32GdiObj *>	s_apGdi;
static HBITMAP					s_hBmpScreen;
static BOOL						s_bScreenDirty;
/* ReleaseDC されないまま描かれた小窓の面（あとで W32FlushSurfaces でまとめて出す） */
static std::vector<HWND>	s_ahWndDirtySurface;

W32Dc *W32GetDc(HDC hDC)
{
	W32Dc *p = (W32Dc *)hDC;
	if (p == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < s_apDc.size(); i ++) {
		if (s_apDc[i] == p) {
			return p;
		}
	}
	return NULL;
}

W32GdiObj *W32GetGdi(HGDIOBJ hObj)
{
	W32GdiObj *p = (W32GdiObj *)hObj;
	if (p == NULL) {
		return NULL;
	}
	if (p->dwMagic == W32GDI_MAGIC) {
		return p;
	}
	return NULL;
}

static W32GdiObj *NewGdi(W32GdiType nType)
{
	W32GdiObj *p = new W32GdiObj;
	memset(p, 0, sizeof(W32GdiObj));
	p->dwMagic = W32GDI_MAGIC;
	p->nType = nType;
	s_apGdi.push_back(p);
	return p;
}

static W32GdiObj *NewBitmap(int cx, int cy, BOOL bTopDown, BYTE *pBits)
{
	W32GdiObj *p = NewGdi(W32GDI_BITMAP);
	p->cx = std::max(cx, 1);
	p->cy = std::max(cy, 1);
	p->bTopDown = bTopDown;
	if (pBits) {
		p->pBits = pBits;
		p->bOwnBits = FALSE;
	} else {
		p->pBits = new BYTE[(size_t)p->cx * p->cy * 4];
		memset(p->pBits, 0, (size_t)p->cx * p->cy * 4);
		p->bOwnBits = TRUE;
	}
	return p;
}

/* 画素の先頭アドレス（y は上から数える） */
static inline BYTE *PixelPtr(W32GdiObj *pBmp, int x, int y)
{
	int nRow = pBmp->bTopDown ? y : (pBmp->cy - 1 - y);
	return pBmp->pBits + ((size_t)nRow * pBmp->cx + x) * 4;
}

static inline DWORD ColorToPixel(COLORREF cl)
{
	/* COLORREF(0x00BBGGRR) → メモリ上 B,G,R,X */
	return (DWORD)(GetBValue(cl) | (GetGValue(cl) << 8) | (GetRValue(cl) << 16));
}

static inline COLORREF PixelToColor(DWORD dw)
{
	return RGB((dw >> 16) & 0xFF, (dw >> 8) & 0xFF, dw & 0xFF);
}

static W32GdiObj *DcBitmap(W32Dc *pDc)
{
	if (pDc == NULL) {
		return NULL;
	}
	return W32GetGdi(pDc->hBmp);
}

HBITMAP W32GetScreenBitmap(void)
{
	int cx, cy;
	W32GetScreenSize(&cx, &cy);
	W32GdiObj *p = W32GetGdi(s_hBmpScreen);
	if (p == NULL || p->cx != cx || p->cy != cy) {
		if (p) {
			DeleteObject(s_hBmpScreen);
		}
		p = NewBitmap(cx, cy, TRUE, NULL);
		p->bStock = TRUE;
		s_hBmpScreen = (HBITMAP)p;
	}
	return s_hBmpScreen;
}

/* ------------------------------------------------------------------------- */
/* 画面への転送																 */
/* ------------------------------------------------------------------------- */
#ifdef W32_BROWSER
EM_JS(void, W32JsPresent, (const unsigned char *pRgba, int cx, int cy), {
	var canvas = Module['canvas'];
	if (!canvas) {
		return;
	}
	if (canvas.width !== cx || canvas.height !== cy) {
		canvas.width = cx;
		canvas.height = cy;
	}
	var ctx = Module['w32Ctx'];
	if (!ctx || Module['w32CtxCanvas'] !== canvas) {
		/* 画面は不透明なので alpha:false にして、ブラウザの合成の手間を減らす */
		ctx = canvas.getContext('2d', { alpha: false });
		Module['w32Ctx'] = ctx;
		Module['w32CtxCanvas'] = canvas;
	}
	/* wasm のメモリをそのまま ImageData の中身として使い、毎回のコピーを省く。
	   メモリが増えると古い buffer は使えなくなるので、その時は作り直す */
	var img = Module['w32Img'];
	if (!img || img.width !== cx || img.height !== cy || Module['w32ImgPtr'] !== pRgba ||
		Module['w32ImgBuf'] !== HEAPU8.buffer) {
		img = new ImageData(new Uint8ClampedArray(HEAPU8.buffer, pRgba, cx * cy * 4), cx, cy);
		Module['w32Img'] = img;
		Module['w32ImgPtr'] = pRgba;
		Module['w32ImgBuf'] = HEAPU8.buffer;
	}
	ctx.putImageData(img, 0, 0);
	/* 表示側（シェル）が拡大表示用の後処理をしたい時のための呼び出し口 */
	if (Module['w32OnPresent']) {
		Module['w32OnPresent']();
	}
});

void W32PresentScreen(void)
{
	static std::vector<BYTE> s_aRgba;
	W32GdiObj *p = W32GetGdi(s_hBmpScreen);
	if (p == NULL) {
		return;
	}
	double dStart = W32PerfNow();
	size_t nSize = (size_t)p->cx * p->cy * 4;
	if (s_aRgba.size() != nSize) {
		s_aRgba.resize(nSize);
	}
	/* B,G,R,X の並びを R,G,B,A(255) にする。2 画素（64bit）ずつまとめて入れ替える
	   （wasm はリトルエンディアンなので、1 画素は 0xXXRRGGBB として読める） */
	const unsigned long long qwMaskRB = 0x000000FF000000FFULL;
	const unsigned long long qwMaskG  = 0x0000FF000000FF00ULL;
	const unsigned long long qwAlpha  = 0xFF000000FF000000ULL;
	DWORD *pDst = (DWORD *)&s_aRgba[0];
	for (int y = 0; y < p->cy; y ++) {
		const DWORD *pSrc = (const DWORD *)PixelPtr(p, 0, y);
		int x = 0;
		for (; x + 1 < p->cx; x += 2) {
			unsigned long long qw;
			memcpy(&qw, &pSrc[x], sizeof(qw));
			qw = qwAlpha | ((qw & qwMaskRB) << 16) | (qw & qwMaskG) | ((qw >> 16) & qwMaskRB);
			memcpy(&pDst[x], &qw, sizeof(qw));
		}
		if (x < p->cx) {
			DWORD dw = pSrc[x];
			pDst[x] = 0xFF000000 | ((dw & 0xFF) << 16) | (dw & 0xFF00) | ((dw >> 16) & 0xFF);
		}
		pDst += p->cx;
	}
	W32JsPresent(&s_aRgba[0], p->cx, p->cy);
	s_bScreenDirty = FALSE;
	if (dStart) {
		W32PerfAdd(W32PERF_PRESENT, W32PerfNow() - dStart);
	}
}
#else
void W32PresentScreen(void)
{
	/* 画面なし */
}
#endif

/* ------------------------------------------------------------------------- */
/* DC																		 */
/* ------------------------------------------------------------------------- */
static HFONT GetDefaultFont(void);

static W32Dc *NewDc(HWND hWnd)
{
	W32Dc *p = new W32Dc;
	memset(p, 0, sizeof(W32Dc));
	p->dwMagic		= W32DC_MAGIC;
	p->hWnd			= hWnd;
	p->hBmpDefault	= (HBITMAP)NewBitmap(1, 1, TRUE, NULL);
	p->hBmp			= p->hBmpDefault;
	p->hFont		= GetDefaultFont();
	p->hBrush		= GetStockObject(WHITE_BRUSH);
	p->hPen			= GetStockObject(BLACK_PEN);
	p->clText		= RGB(0, 0, 0);
	p->clBk			= RGB(255, 255, 255);
	p->nBkMode		= OPAQUE;
	s_apDc.push_back(p);
	return p;
}

static void FillBits(W32GdiObj *pBmp, int x, int y, int cx, int cy, DWORD dwPixel);

/* 背景の色（クラスのブラシ。無ければダイアログと同じ色） */
static COLORREF WndBackColor(W32Wnd *pWnd)
{
	W32GdiObj *pBrush = W32GetGdi(pWnd->hbrBackground);
	if (pBrush && pBrush->nType == W32GDI_BRUSH && !pBrush->bNull) {
		return pBrush->cl;
	}
	return RGB(240, 240, 240);
}

void W32FreeWndSurface(W32Wnd *pWnd)
{
	W32GdiObj *pBmp = W32GetGdi(pWnd->hBmpSurface);
	if (pBmp) {
		pBmp->bStock = FALSE;
		DeleteObject(pWnd->hBmpSurface);
	}
	pWnd->hBmpSurface = NULL;
}

/* 小窓の中のウィンドウは、描いた内容を自分のビットマップに残す */
static W32GdiObj *WndSurface(W32Wnd *pWnd)
{
	RECT rc;
	GetClientRect((HWND)pWnd, &rc);
	int cx = std::max(1, (int)rc.right);
	int cy = std::max(1, (int)rc.bottom);
	W32GdiObj *pBmp = W32GetGdi(pWnd->hBmpSurface);
	if (pBmp && pBmp->cx == cx && pBmp->cy == cy) {
		return pBmp;
	}
	W32FreeWndSurface(pWnd);
	pBmp = NewBitmap(cx, cy, TRUE, NULL);
	pBmp->bStock = TRUE;		/* SelectObject 先で消されないように */
	FillBits(pBmp, 0, 0, cx, cy, ColorToPixel(WndBackColor(pWnd)));
	pWnd->hBmpSurface = (HBITMAP)pBmp;
	return pBmp;
}

/* ウィンドウ用 DC の描き先：メインウィンドウは画面、小窓の中は自分のビットマップ、
   それ以外は使い捨てのビットマップ */
static void BindWindowBitmap(W32Dc *pDc, HWND hWnd)
{
	W32Wnd *pWnd = W32GetWnd(hWnd);
	if (pWnd && pWnd->bScreen) {
		pDc->hBmp = W32GetScreenBitmap();
		return;
	}
	if (pWnd && W32GetPopupRoot(pWnd)) {
		pDc->hBmp = (HBITMAP)WndSurface(pWnd);
		return;
	}
	if (pWnd) {
		int cx = std::max(1, (int)(pWnd->rc.right - pWnd->rc.left));
		int cy = std::max(1, (int)(pWnd->rc.bottom - pWnd->rc.top));
		W32GdiObj *pBmp = W32GetGdi(pDc->hBmpDefault);
		if (pBmp && (pBmp->cx != cx || pBmp->cy != cy)) {
			DeleteObject(pDc->hBmpDefault);
			pDc->hBmpDefault = (HBITMAP)NewBitmap(cx, cy, TRUE, NULL);
		}
		pDc->hBmp = pDc->hBmpDefault;
	}
}

HDC GetDC(HWND hWnd)
{
	W32Dc *p = NewDc(hWnd);
	if (hWnd) {
		BindWindowBitmap(p, hWnd);
	}
	return (HDC)p;
}

HDC GetWindowDC(HWND hWnd)
{
	return GetDC(hWnd);
}

int ReleaseDC(HWND hWnd, HDC hDC)
{
	W32Dc *p = W32GetDc(hDC);
	if (p == NULL) {
		return 0;
	}
	W32Wnd *pWnd = W32GetWnd(hWnd);
	if (pWnd && pWnd->bScreen && s_bScreenDirty) {
		W32PresentScreen();
	}
	if (pWnd && pWnd->hBmpSurface && p->hBmp == pWnd->hBmpSurface) {
		W32HtmlPaint(pWnd, W32GetGdi(pWnd->hBmpSurface));
		s_ahWndDirtySurface.erase(std::remove(s_ahWndDirtySurface.begin(), s_ahWndDirtySurface.end(), hWnd), s_ahWndDirtySurface.end());
	}
	DeleteDC(hDC);
	return 1;
}

HDC BeginPaint(HWND hWnd, LPPAINTSTRUCT pPs)
{
	W32Wnd *pWnd = W32GetWnd(hWnd);
	if (pWnd) {
		pWnd->bInvalid = FALSE;
	}
	HDC hDC = GetDC(hWnd);
	/* 小窓の中は背景で塗り直す（WM_ERASEBKGND の代わり） */
	if (pWnd && pWnd->hBmpSurface) {
		W32GdiObj *pBmp = W32GetGdi(pWnd->hBmpSurface);
		if (pBmp) {
			FillBits(pBmp, 0, 0, pBmp->cx, pBmp->cy, ColorToPixel(WndBackColor(pWnd)));
		}
	}
	if (pPs) {
		memset(pPs, 0, sizeof(PAINTSTRUCT));
		pPs->hdc = hDC;
		GetClientRect(hWnd, &pPs->rcPaint);
	}
	return hDC;
}

BOOL EndPaint(HWND hWnd, const PAINTSTRUCT *pPs)
{
	if (pPs) {
		ReleaseDC(hWnd, pPs->hdc);
	}
	return TRUE;
}

HDC CreateCompatibleDC(HDC)
{
	return (HDC)NewDc(NULL);
}

BOOL DeleteDC(HDC hDC)
{
	W32Dc *p = W32GetDc(hDC);
	if (p == NULL) {
		return FALSE;
	}
	s_apDc.erase(std::remove(s_apDc.begin(), s_apDc.end(), p), s_apDc.end());
	DeleteObject(p->hBmpDefault);
	p->dwMagic = 0;
	delete p;
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* ビットマップ																 */
/* ------------------------------------------------------------------------- */
HBITMAP CreateCompatibleBitmap(HDC, int cx, int cy)
{
	return (HBITMAP)NewBitmap(cx, cy, FALSE, NULL);
}

HBITMAP CreateBitmap(int cx, int cy, UINT, UINT, const void *pBits)
{
	W32GdiObj *p = NewBitmap(cx, cy, TRUE, NULL);
	if (pBits) {
		memcpy(p->pBits, pBits, (size_t)p->cx * p->cy * 4);
	}
	return (HBITMAP)p;
}

HBITMAP CreateDIBSection(HDC, const BITMAPINFO *pBmi, UINT, void **ppBits, HANDLE, DWORD)
{
	if (pBmi == NULL) {
		return NULL;
	}
	if (pBmi->bmiHeader.biBitCount != 32) {
		printf("[w32] CreateDIBSection: %d bit は未対応です\n", pBmi->bmiHeader.biBitCount);
		return NULL;
	}
	int cx = pBmi->bmiHeader.biWidth;
	int cy = pBmi->bmiHeader.biHeight;
	BOOL bTopDown = FALSE;
	if (cy < 0) {
		cy = -cy;
		bTopDown = TRUE;
	}
	W32GdiObj *p = NewBitmap(cx, cy, bTopDown, NULL);
	if (ppBits) {
		*ppBits = p->pBits;
	}
	return (HBITMAP)p;
}

int GetDIBits(HDC, HBITMAP hBmp, UINT uStart, UINT cLines, LPVOID pBits, LPBITMAPINFO pBmi, UINT)
{
	W32GdiObj *p = W32GetGdi(hBmp);
	if (p == NULL || p->nType != W32GDI_BITMAP || pBmi == NULL) {
		return 0;
	}
	if (pBits == NULL) {
		pBmi->bmiHeader.biWidth = p->cx;
		pBmi->bmiHeader.biHeight = p->cy;
		pBmi->bmiHeader.biPlanes = 1;
		pBmi->bmiHeader.biBitCount = 32;
		pBmi->bmiHeader.biCompression = BI_RGB;
		pBmi->bmiHeader.biSizeImage = p->cx * p->cy * 4;
		return p->cy;
	}
	int nBpp = pBmi->bmiHeader.biBitCount;
	int nStride = ((p->cx * nBpp / 8) + 3) & ~3;
	BOOL bDstTopDown = pBmi->bmiHeader.biHeight < 0;
	UINT n = 0;
	for (UINT i = 0; i < cLines; i ++) {
		int nLine = (int)(uStart + i);		/* 下から数えた行 */
		if (nLine >= p->cy) {
			break;
		}
		int ySrc = p->cy - 1 - nLine;
		BYTE *pDst = (BYTE *)pBits + (size_t)(bDstTopDown ? (cLines - 1 - i) : i) * nStride;
		const BYTE *pSrc = PixelPtr(p, 0, ySrc);
		for (int x = 0; x < p->cx; x ++) {
			if (nBpp == 32) {
				memcpy(pDst + x * 4, pSrc + x * 4, 4);
			} else if (nBpp == 24) {
				memcpy(pDst + x * 3, pSrc + x * 4, 3);
			}
		}
		n ++;
	}
	return (int)n;
}

int SetDIBitsToDevice(HDC hDC, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc,
	UINT, UINT, const void *pBits, const BITMAPINFO *pBmi, UINT)
{
	return StretchDIBits(hDC, xDest, yDest, (int)w, (int)h, xSrc, ySrc, (int)w, (int)h, pBits, pBmi, 0, SRCCOPY);
}

int StretchDIBits(HDC hDC, int xDest, int yDest, int wDest, int hDest, int xSrc, int ySrc, int wSrc, int hSrc,
	const void *pBits, const BITMAPINFO *pBmi, UINT, DWORD dwRop)
{
	if (pBmi == NULL || pBits == NULL || pBmi->bmiHeader.biBitCount != 32) {
		return 0;
	}
	int cy = pBmi->bmiHeader.biHeight;
	BOOL bTopDown = cy < 0;
	W32GdiObj tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.dwMagic = W32GDI_MAGIC;
	tmp.nType = W32GDI_BITMAP;
	tmp.cx = pBmi->bmiHeader.biWidth;
	tmp.cy = bTopDown ? -cy : cy;
	tmp.bTopDown = bTopDown;
	tmp.pBits = (BYTE *)pBits;
	/* 一時的に DC を作って StretchBlt に任せる */
	W32Dc dc;
	memset(&dc, 0, sizeof(dc));
	dc.dwMagic = W32DC_MAGIC;
	dc.hBmp = (HBITMAP)&tmp;
	s_apDc.push_back(&dc);
	s_apGdi.push_back(&tmp);
	StretchBlt(hDC, xDest, yDest, wDest, hDest, (HDC)&dc, xSrc, ySrc, wSrc, hSrc, dwRop);
	s_apDc.erase(std::remove(s_apDc.begin(), s_apDc.end(), &dc), s_apDc.end());
	s_apGdi.erase(std::remove(s_apGdi.begin(), s_apGdi.end(), &tmp), s_apGdi.end());
	tmp.dwMagic = 0;
	return hDest;
}

HGDIOBJ SelectObject(HDC hDC, HGDIOBJ hObj)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *p = W32GetGdi(hObj);
	if (pDc == NULL || p == NULL) {
		return NULL;
	}
	HGDIOBJ hOld = NULL;
	switch (p->nType) {
	case W32GDI_BITMAP:
		hOld = pDc->hBmp;
		pDc->hBmp = (HBITMAP)hObj;
		break;
	case W32GDI_FONT:
		hOld = pDc->hFont;
		pDc->hFont = (HFONT)hObj;
		break;
	case W32GDI_BRUSH:
		hOld = pDc->hBrush;
		pDc->hBrush = (HBRUSH)hObj;
		break;
	case W32GDI_PEN:
		hOld = pDc->hPen;
		pDc->hPen = (HPEN)hObj;
		break;
	}
	return hOld;
}

static void FontRelease(W32GdiObj *p);

BOOL DeleteObject(HGDIOBJ hObj)
{
	W32GdiObj *p = W32GetGdi(hObj);
	if (p == NULL || p->bStock) {
		return FALSE;
	}
	/* DC で選択中のものは外しておく（Win32 では未定義動作だが落ちないように） */
	for (size_t i = 0; i < s_apDc.size(); i ++) {
		if (s_apDc[i]->hBmp == hObj) {
			s_apDc[i]->hBmp = s_apDc[i]->hBmpDefault;
		}
	}
	if (p->nType == W32GDI_BITMAP && p->bOwnBits) {
		delete [] p->pBits;
	}
	if (p->nType == W32GDI_FONT) {
		FontRelease(p);
	}
	s_apGdi.erase(std::remove(s_apGdi.begin(), s_apGdi.end(), p), s_apGdi.end());
	p->dwMagic = 0;
	delete p;
	return TRUE;
}

HGDIOBJ GetStockObject(int nObject)
{
	static W32GdiObj *s_apStock[32];
	if (nObject < 0 || nObject >= 32) {
		return NULL;
	}
	if (s_apStock[nObject]) {
		return (HGDIOBJ)s_apStock[nObject];
	}
	W32GdiObj *p = NULL;
	switch (nObject) {
	case WHITE_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->cl = RGB(255, 255, 255);	break;
	case LTGRAY_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->cl = RGB(192, 192, 192);	break;
	case GRAY_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->cl = RGB(128, 128, 128);	break;
	case DKGRAY_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->cl = RGB(64, 64, 64);	break;
	case BLACK_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->cl = RGB(0, 0, 0);		break;
	case NULL_BRUSH:	p = NewGdi(W32GDI_BRUSH);	p->bNull = TRUE;			break;
	case WHITE_PEN:		p = NewGdi(W32GDI_PEN);		p->cl = RGB(255, 255, 255);	break;
	case BLACK_PEN:		p = NewGdi(W32GDI_PEN);		p->cl = RGB(0, 0, 0);		break;
	case NULL_PEN:		p = NewGdi(W32GDI_PEN);		p->bNull = TRUE;			break;
	case SYSTEM_FONT:
	case DEFAULT_GUI_FONT:
		return (HGDIOBJ)GetDefaultFont();
	default:
		return NULL;
	}
	p->bStock = TRUE;
	s_apStock[nObject] = p;
	return (HGDIOBJ)p;
}

int GetObject(HGDIOBJ hObj, int cbBuffer, LPVOID pv)
{
	W32GdiObj *p = W32GetGdi(hObj);
	if (p == NULL) {
		return 0;
	}
	if (p->nType == W32GDI_BITMAP) {
		if (pv == NULL) {
			return sizeof(BITMAP);
		}
		BITMAP bm;
		memset(&bm, 0, sizeof(bm));
		bm.bmWidth		= p->cx;
		bm.bmHeight		= p->cy;
		bm.bmWidthBytes	= p->cx * 4;
		bm.bmPlanes		= 1;
		bm.bmBitsPixel	= 32;
		bm.bmBits		= p->pBits;
		memcpy(pv, &bm, std::min(cbBuffer, (int)sizeof(bm)));
		return std::min(cbBuffer, (int)sizeof(bm));
	}
	return 0;
}

HBRUSH CreateSolidBrush(COLORREF cl)
{
	W32GdiObj *p = NewGdi(W32GDI_BRUSH);
	p->cl = cl;
	return (HBRUSH)p;
}

HPEN CreatePen(int nStyle, int, COLORREF cl)
{
	W32GdiObj *p = NewGdi(W32GDI_PEN);
	p->cl = cl;
	p->bNull = (nStyle == 5);	/* PS_NULL */
	return (HPEN)p;
}

/* ------------------------------------------------------------------------- */
/* 転送・塗りつぶし															 */
/* ------------------------------------------------------------------------- */
static void MarkDirty(W32Dc *pDc)
{
	if (pDc == NULL) {
		return;
	}
	if (pDc->hBmp == s_hBmpScreen) {
		s_bScreenDirty = TRUE;
		return;
	}
	if (pDc->hWnd) {
		W32Wnd *pWnd = W32GetWnd(pDc->hWnd);
		if (pWnd && pWnd->hBmpSurface && pDc->hBmp == pWnd->hBmpSurface &&
			std::find(s_ahWndDirtySurface.begin(), s_ahWndDirtySurface.end(), pDc->hWnd) == s_ahWndDirtySurface.end()) {
			s_ahWndDirtySurface.push_back(pDc->hWnd);
		}
	}
}

void W32FlushSurfaces(void)
{
	if (s_ahWndDirtySurface.empty()) {
		return;
	}
	std::vector<HWND> ahWnd;
	ahWnd.swap(s_ahWndDirtySurface);
	for (size_t i = 0; i < ahWnd.size(); i++) {
		W32Wnd *pWnd = W32GetWnd(ahWnd[i]);
		if (pWnd && pWnd->hBmpSurface) {
			W32HtmlPaint(pWnd, W32GetGdi(pWnd->hBmpSurface));
		}
	}
}

static void FillBits(W32GdiObj *pBmp, int x, int y, int cx, int cy, DWORD dwPixel)
{
	int x1 = std::max(x, 0);
	int y1 = std::max(y, 0);
	int x2 = std::min(x + cx, pBmp->cx);
	int y2 = std::min(y + cy, pBmp->cy);
	for (int yy = y1; yy < y2; yy ++) {
		DWORD *pDst = (DWORD *)PixelPtr(pBmp, x1, yy);
		for (int xx = x1; xx < x2; xx ++) {
			*pDst ++ = dwPixel;
		}
	}
}

BOOL BitBlt(HDC hDest, int x, int y, int cx, int cy, HDC hSrc, int sx, int sy, DWORD dwRop)
{
	W32Dc *pDcDst = W32GetDc(hDest);
	W32GdiObj *pDst = DcBitmap(pDcDst);
	if (pDst == NULL) {
		return FALSE;
	}
	MarkDirty(pDcDst);
	if (dwRop == BLACKNESS || dwRop == WHITENESS) {
		FillBits(pDst, x, y, cx, cy, dwRop == BLACKNESS ? 0 : 0x00FFFFFF);
		return TRUE;
	}
	W32GdiObj *pSrc = DcBitmap(W32GetDc(hSrc));
	if (pSrc == NULL) {
		return FALSE;
	}
	/* クリッピング */
	if (sx < 0) { x -= sx; cx += sx; sx = 0; }
	if (sy < 0) { y -= sy; cy += sy; sy = 0; }
	if (x < 0) { sx -= x; cx += x; x = 0; }
	if (y < 0) { sy -= y; cy += y; y = 0; }
	cx = std::min(cx, std::min(pDst->cx - x, pSrc->cx - sx));
	cy = std::min(cy, std::min(pDst->cy - y, pSrc->cy - sy));
	if (cx <= 0 || cy <= 0) {
		return TRUE;
	}
	for (int yy = 0; yy < cy; yy ++) {
		DWORD *pD = (DWORD *)PixelPtr(pDst, x, y + yy);
		const DWORD *pS = (const DWORD *)PixelPtr(pSrc, sx, sy + yy);
		switch (dwRop) {
		case SRCPAINT:	for (int i = 0; i < cx; i ++) pD[i] |= pS[i];	break;
		case SRCAND:	for (int i = 0; i < cx; i ++) pD[i] &= pS[i];	break;
		case SRCINVERT:	for (int i = 0; i < cx; i ++) pD[i] ^= pS[i];	break;
		default:		memmove(pD, pS, (size_t)cx * 4);				break;
		}
	}
	return TRUE;
}

BOOL StretchBlt(HDC hDest, int x, int y, int cx, int cy, HDC hSrc, int sx, int sy, int scx, int scy, DWORD dwRop)
{
	if (cx == scx && cy == scy) {
		return BitBlt(hDest, x, y, cx, cy, hSrc, sx, sy, dwRop);
	}
	W32Dc *pDcDst = W32GetDc(hDest);
	W32GdiObj *pDst = DcBitmap(pDcDst);
	W32GdiObj *pSrc = DcBitmap(W32GetDc(hSrc));
	if (pDst == NULL || pSrc == NULL || cx == 0 || cy == 0 || scx == 0 || scy == 0) {
		return FALSE;
	}
	MarkDirty(pDcDst);
	/* 最近傍で拡大縮小（負の幅は反転） */
	int nW = abs(cx), nH = abs(cy);
	for (int yy = 0; yy < nH; yy ++) {
		int dy = (cy > 0) ? (y + yy) : (y - yy - 1);
		if (dy < 0 || dy >= pDst->cy) {
			continue;
		}
		int ty = sy + (int)((long long)yy * scy / cy * (cy > 0 ? 1 : -1));
		if (scy < 0) {
			ty = sy + (int)((long long)yy * scy / nH) - 1;
		}
		if (ty < 0 || ty >= pSrc->cy) {
			continue;
		}
		for (int xx = 0; xx < nW; xx ++) {
			int dx = (cx > 0) ? (x + xx) : (x - xx - 1);
			if (dx < 0 || dx >= pDst->cx) {
				continue;
			}
			int tx = sx + (int)((long long)xx * scx / nW);
			if (scx < 0) {
				tx -= 1;
			}
			if (tx < 0 || tx >= pSrc->cx) {
				continue;
			}
			*(DWORD *)PixelPtr(pDst, dx, dy) = *(const DWORD *)PixelPtr(pSrc, tx, ty);
		}
	}
	return TRUE;
}

BOOL PatBlt(HDC hDC, int x, int y, int cx, int cy, DWORD dwRop)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL) {
		return FALSE;
	}
	MarkDirty(pDc);
	DWORD dw = 0;
	if (dwRop == WHITENESS) {
		dw = 0x00FFFFFF;
	} else if (dwRop != BLACKNESS) {
		W32GdiObj *pBrush = W32GetGdi(pDc->hBrush);
		if (pBrush == NULL || pBrush->bNull) {
			return TRUE;
		}
		dw = ColorToPixel(pBrush->cl);
	}
	if (cx < 0) { x += cx; cx = -cx; }
	if (cy < 0) { y += cy; cy = -cy; }
	FillBits(pBmp, x, y, cx, cy, dw);
	return TRUE;
}

int FillRect(HDC hDC, const RECT *pRect, HBRUSH hBrush)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || pRect == NULL) {
		return 0;
	}
	COLORREF cl;
	W32GdiObj *pBrush = W32GetGdi(hBrush);
	if (pBrush) {
		if (pBrush->bNull) {
			return 1;
		}
		cl = pBrush->cl;
	} else {
		/* (HBRUSH)(COLOR_xxx + 1) の形式 */
		cl = RGB(212, 208, 200);
	}
	MarkDirty(pDc);
	FillBits(pBmp, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, ColorToPixel(cl));
	return 1;
}

int FrameRect(HDC hDC, const RECT *pRect, HBRUSH hBrush)
{
	if (pRect == NULL) {
		return 0;
	}
	RECT rc;
	SetRect(&rc, pRect->left, pRect->top, pRect->right, pRect->top + 1);			FillRect(hDC, &rc, hBrush);
	SetRect(&rc, pRect->left, pRect->bottom - 1, pRect->right, pRect->bottom);		FillRect(hDC, &rc, hBrush);
	SetRect(&rc, pRect->left, pRect->top, pRect->left + 1, pRect->bottom);			FillRect(hDC, &rc, hBrush);
	SetRect(&rc, pRect->right - 1, pRect->top, pRect->right, pRect->bottom);		FillRect(hDC, &rc, hBrush);
	return 1;
}

COLORREF SetPixel(HDC hDC, int x, int y, COLORREF cl)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || x < 0 || y < 0 || x >= pBmp->cx || y >= pBmp->cy) {
		return (COLORREF)-1;
	}
	MarkDirty(pDc);
	*(DWORD *)PixelPtr(pBmp, x, y) = ColorToPixel(cl);
	return cl;
}

COLORREF GetPixel(HDC hDC, int x, int y)
{
	W32GdiObj *pBmp = DcBitmap(W32GetDc(hDC));
	if (pBmp == NULL || x < 0 || y < 0 || x >= pBmp->cx || y >= pBmp->cy) {
		return (COLORREF)-1;
	}
	return PixelToColor(*(DWORD *)PixelPtr(pBmp, x, y));
}

BOOL MoveToEx(HDC hDC, int x, int y, LPPOINT pPt)
{
	W32Dc *pDc = W32GetDc(hDC);
	if (pDc == NULL) {
		return FALSE;
	}
	if (pPt) {
		*pPt = pDc->ptPen;
	}
	pDc->ptPen.x = x;
	pDc->ptPen.y = y;
	return TRUE;
}

BOOL LineTo(HDC hDC, int x, int y)
{
	W32Dc *pDc = W32GetDc(hDC);
	if (pDc == NULL) {
		return FALSE;
	}
	W32GdiObj *pPen = W32GetGdi(pDc->hPen);
	if (pPen && !pPen->bNull) {
		/* ブレゼンハム（終点は描かない） */
		int x0 = pDc->ptPen.x, y0 = pDc->ptPen.y;
		int dx = abs(x - x0), sx = (x0 < x) ? 1 : -1;
		int dy = -abs(y - y0), sy = (y0 < y) ? 1 : -1;
		int err = dx + dy;
		while (x0 != x || y0 != y) {
			SetPixel(hDC, x0, y0, pPen->cl);
			int e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
		}
	}
	pDc->ptPen.x = x;
	pDc->ptPen.y = y;
	return TRUE;
}

BOOL Rectangle(HDC hDC, int left, int top, int right, int bottom)
{
	W32Dc *pDc = W32GetDc(hDC);
	if (pDc == NULL) {
		return FALSE;
	}
	RECT rc;
	SetRect(&rc, left, top, right, bottom);
	FillRect(hDC, &rc, pDc->hBrush);
	W32GdiObj *pPen = W32GetGdi(pDc->hPen);
	if (pPen && !pPen->bNull) {
		HBRUSH hBrush = CreateSolidBrush(pPen->cl);
		FrameRect(hDC, &rc, hBrush);
		DeleteObject(hBrush);
	}
	return TRUE;
}

COLORREF SetTextColor(HDC hDC, COLORREF cl)
{
	W32Dc *p = W32GetDc(hDC);
	if (p == NULL) {
		return 0;
	}
	COLORREF clOld = p->clText;
	p->clText = cl;
	return clOld;
}

COLORREF GetTextColor(HDC hDC)
{
	W32Dc *p = W32GetDc(hDC);
	return p ? p->clText : 0;
}

COLORREF SetBkColor(HDC hDC, COLORREF cl)
{
	W32Dc *p = W32GetDc(hDC);
	if (p == NULL) {
		return 0;
	}
	COLORREF clOld = p->clBk;
	p->clBk = cl;
	return clOld;
}

COLORREF GetBkColor(HDC hDC)
{
	W32Dc *p = W32GetDc(hDC);
	return p ? p->clBk : 0;
}

int SetBkMode(HDC hDC, int nMode)
{
	W32Dc *p = W32GetDc(hDC);
	if (p == NULL) {
		return 0;
	}
	int nOld = p->nBkMode;
	p->nBkMode = nMode;
	return nOld;
}

int GetBkMode(HDC hDC)
{
	W32Dc *p = W32GetDc(hDC);
	return p ? p->nBkMode : 0;
}

int GetDeviceCaps(HDC, int nIndex)
{
	if (nIndex == BITSPIXEL) {
		return 32;
	}
	return 0;
}

int SetStretchBltMode(HDC, int)
{
	return COLORONCOLOR;
}

/* ------------------------------------------------------------------------- */
/* フォント・文字描画														 */
/* ------------------------------------------------------------------------- */
#ifdef W32_USE_TTF
#define FONT_PATH_REGULAR	"/font/BIZUDGothic-Regular.ttf"
#define FONT_PATH_BOLD		"/font/BIZUDGothic-Bold.ttf"

/* 大きさ・太さごとに TTF_Font を共有する */
static std::map<int, TTF_Font *> s_mapTtf;

static TTF_Font *GetTtf(int nSize, BOOL bBold)
{
	if (!TTF_WasInit() && TTF_Init() != 0) {
		return NULL;
	}
	nSize = std::max(nSize, 6);
	int nKey = nSize * 2 + (bBold ? 1 : 0);
	std::map<int, TTF_Font *>::iterator it = s_mapTtf.find(nKey);
	if (it != s_mapTtf.end()) {
		return it->second;
	}
	TTF_Font *pFont = TTF_OpenFont(bBold ? FONT_PATH_BOLD : FONT_PATH_REGULAR, nSize);
	if (pFont == NULL) {
		printf("[w32] フォントを開けません: %s\n", TTF_GetError());
	} else {
		/* MS ゴシックのビットマップに近づけるため、ヒンティングを強めにかける */
		TTF_SetFontHinting(pFont, TTF_HINTING_MONO);
	}
	s_mapTtf[nKey] = pFont;
	return pFont;
}

#endif

static HFONT GetDefaultFont(void)
{
	static HFONT s_hFont = NULL;
	if (s_hFont == NULL) {
		s_hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "");
		W32GetGdi(s_hFont)->bStock = TRUE;
	}
	return s_hFont;
}

HFONT CreateFont(int nHeight, int, int, int, int fnWeight, DWORD fdwItalic, DWORD, DWORD, DWORD,
	DWORD, DWORD, DWORD, DWORD, LPCSTR)
{
	W32GdiObj *p = NewGdi(W32GDI_FONT);
	p->nHeight = (nHeight == 0) ? 12 : abs(nHeight);
	p->nWeight = fnWeight;
	p->bItalic = fdwItalic ? TRUE : FALSE;
	return (HFONT)p;
}

HFONT CreateFontIndirect(const LOGFONT *pLf)
{
	if (pLf == NULL) {
		return NULL;
	}
	return CreateFont(pLf->lfHeight, pLf->lfWidth, 0, 0, pLf->lfWeight, pLf->lfItalic, 0, 0, 0, 0, 0, 0, 0, pLf->lfFaceName);
}

static void FontRelease(W32GdiObj *)
{
	/* TTF_Font は大きさごとに共有しているので閉じない */
}

#ifdef W32_USE_TTF
static TTF_Font *DcTtf(W32Dc *pDc)
{
	W32GdiObj *pFont = W32GetGdi(pDc ? pDc->hFont : NULL);
	if (pFont == NULL || pFont->nType != W32GDI_FONT) {
		pFont = W32GetGdi(GetDefaultFont());
	}
	TTF_Font *pTtf = GetTtf(pFont->nHeight, pFont->nWeight >= FW_SEMIBOLD);
	if (pTtf) {
		TTF_SetFontStyle(pTtf, pFont->bItalic ? TTF_STYLE_ITALIC : TTF_STYLE_NORMAL);
	}
	return pTtf;
}

/* 描いた文字の形（1bit マスク）を覚えておく。TextOut2 は同じ文字列を 5 回描くので効く */
struct W32TextMask {
	std::string			strKey;
	int					cx, cy;
	std::vector<BYTE>	aMask;
};
static std::list<W32TextMask>	s_listMask;
#define TEXTMASK_CACHE_MAX		256

static const W32TextMask *GetTextMask(TTF_Font *pTtf, const std::string &strUtf8)
{
	char szKey[32];
	snprintf(szKey, sizeof(szKey), "%p:", (void *)pTtf);
	std::string strKey = std::string(szKey) + strUtf8;
	for (std::list<W32TextMask>::iterator it = s_listMask.begin(); it != s_listMask.end(); ++ it) {
		if (it->strKey == strKey) {
			if (it != s_listMask.begin()) {
				s_listMask.splice(s_listMask.begin(), s_listMask, it);
			}
			return &s_listMask.front();
		}
	}
	SDL_Color clWhite = {255, 255, 255, 255};
	SDL_Surface *pSurf = TTF_RenderUTF8_Solid(pTtf, strUtf8.c_str(), clWhite);
	if (pSurf == NULL) {
		return NULL;
	}
	W32TextMask mask;
	mask.strKey = strKey;
	mask.cx = pSurf->w;
	mask.cy = pSurf->h;
	mask.aMask.resize((size_t)mask.cx * mask.cy);
	SDL_LockSurface(pSurf);
	for (int y = 0; y < mask.cy; y ++) {
		const BYTE *pSrc = (const BYTE *)pSurf->pixels + (size_t)y * pSurf->pitch;
		for (int x = 0; x < mask.cx; x ++) {
			mask.aMask[(size_t)y * mask.cx + x] = pSrc[x] ? 1 : 0;
		}
	}
	SDL_UnlockSurface(pSurf);
	SDL_FreeSurface(pSurf);
	s_listMask.push_front(mask);
	if (s_listMask.size() > TEXTMASK_CACHE_MAX) {
		s_listMask.pop_back();
	}
	return &s_listMask.front();
}

BOOL TextOut(HDC hDC, int x, int y, LPCSTR pszText, int nLen)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || pszText == NULL || nLen <= 0) {
		return FALSE;
	}
	TTF_Font *pTtf = DcTtf(pDc);
	if (pTtf == NULL) {
		return FALSE;
	}
	std::string strUtf8 = W32SjisToUtf8Str(pszText, nLen);
	if (strUtf8.empty()) {
		return TRUE;
	}
	const W32TextMask *pMask = GetTextMask(pTtf, strUtf8);
	if (pMask == NULL) {
		return FALSE;
	}
	MarkDirty(pDc);
	if (pDc->nBkMode == OPAQUE) {
		FillBits(pBmp, x, y, pMask->cx, pMask->cy, ColorToPixel(pDc->clBk));
	}
	DWORD dwText = ColorToPixel(pDc->clText);
	for (int yy = 0; yy < pMask->cy; yy ++) {
		int dy = y + yy;
		if (dy < 0 || dy >= pBmp->cy) {
			continue;
		}
		const BYTE *pM = &pMask->aMask[(size_t)yy * pMask->cx];
		for (int xx = 0; xx < pMask->cx; xx ++) {
			int dx = x + xx;
			if (pM[xx] && dx >= 0 && dx < pBmp->cx) {
				*(DWORD *)PixelPtr(pBmp, dx, dy) = dwText;
			}
		}
	}
	return TRUE;
}

BOOL GetTextExtentPoint32(HDC hDC, LPCSTR pszText, int nLen, LPSIZE pSize)
{
	if (pSize == NULL) {
		return FALSE;
	}
	pSize->cx = 0;
	pSize->cy = 0;
	TTF_Font *pTtf = DcTtf(W32GetDc(hDC));
	if (pTtf == NULL) {
		return FALSE;
	}
	pSize->cy = TTF_FontHeight(pTtf);
	if (pszText == NULL || nLen <= 0) {
		return TRUE;
	}
	std::string strUtf8 = W32SjisToUtf8Str(pszText, nLen);
	int w = 0, h = 0;
	TTF_SizeUTF8(pTtf, strUtf8.c_str(), &w, &h);
	pSize->cx = w;
	pSize->cy = h;
	return TRUE;
}

BOOL GetTextMetrics(HDC hDC, LPTEXTMETRIC pTm)
{
	if (pTm == NULL) {
		return FALSE;
	}
	memset(pTm, 0, sizeof(TEXTMETRIC));
	TTF_Font *pTtf = DcTtf(W32GetDc(hDC));
	if (pTtf == NULL) {
		return FALSE;
	}
	pTm->tmHeight = TTF_FontHeight(pTtf);
	pTm->tmAscent = TTF_FontAscent(pTtf);
	pTm->tmDescent = -TTF_FontDescent(pTtf);
	int w = 0, h = 0;
	TTF_SizeUTF8(pTtf, "0", &w, &h);
	pTm->tmAveCharWidth = w;
	pTm->tmMaxCharWidth = w * 2;
	return TRUE;
}

#else	/* W32_USE_TTF */

/* 画面なしの時は文字を描かない。大きさだけ「半角＝高さの半分」として返す */
static W32GdiObj *DcFont(W32Dc *pDc)
{
	W32GdiObj *pFont = W32GetGdi(pDc ? pDc->hFont : NULL);
	if (pFont == NULL || pFont->nType != W32GDI_FONT) {
		pFont = W32GetGdi(GetDefaultFont());
	}
	return pFont;
}

BOOL TextOut(HDC, int, int, LPCSTR, int)
{
	return TRUE;
}

BOOL GetTextExtentPoint32(HDC hDC, LPCSTR pszText, int nLen, LPSIZE pSize)
{
	if (pSize == NULL) {
		return FALSE;
	}
	int nHeight = DcFont(W32GetDc(hDC))->nHeight;
	pSize->cx = (pszText && nLen > 0) ? nLen * nHeight / 2 : 0;
	pSize->cy = nHeight;
	return TRUE;
}

BOOL GetTextMetrics(HDC hDC, LPTEXTMETRIC pTm)
{
	if (pTm == NULL) {
		return FALSE;
	}
	memset(pTm, 0, sizeof(TEXTMETRIC));
	int nHeight = DcFont(W32GetDc(hDC))->nHeight;
	pTm->tmHeight = nHeight;
	pTm->tmAscent = nHeight * 7 / 8;
	pTm->tmDescent = nHeight - pTm->tmAscent;
	pTm->tmAveCharWidth = nHeight / 2;
	pTm->tmMaxCharWidth = nHeight;
	return TRUE;
}

#endif	/* W32_USE_TTF */

int DrawText(HDC hDC, LPCSTR pszText, int nLen, LPRECT pRect, UINT uFormat)
{
	if (pszText == NULL || pRect == NULL) {
		return 0;
	}
	if (nLen < 0) {
		nLen = (int)strlen(pszText);
	}
	SIZE size;
	GetTextExtentPoint32(hDC, pszText, nLen, &size);
	if (uFormat & DT_CALCRECT) {
		pRect->right = pRect->left + size.cx;
		pRect->bottom = pRect->top + size.cy;
		return size.cy;
	}
	int x = pRect->left;
	int y = pRect->top;
	if (uFormat & DT_CENTER) {
		x = (pRect->left + pRect->right - size.cx) / 2;
	} else if (uFormat & DT_RIGHT) {
		x = pRect->right - size.cx;
	}
	if (uFormat & DT_VCENTER) {
		y = (pRect->top + pRect->bottom - size.cy) / 2;
	} else if (uFormat & DT_BOTTOM) {
		y = pRect->bottom - size.cy;
	}
	TextOut(hDC, x, y, pszText, nLen);
	return size.cy;
}
