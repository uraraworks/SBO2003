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
#include <set>
#include <unordered_map>
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
/* 検査モード（URL に ?verify=1）: 差分だけ転送した後の canvas が、全画面を転送した場合と
   同じ画素になっているかを毎回確かめる（テスト用。通常は何もしない） */
EM_JS(int, W32JsVerifyEnabled, (void), {
	try {
		return new URLSearchParams(location.search).get('verify') === '1' ? 1 : 0;
	} catch (e) {
		return 0;
	}
});

static BOOL s_bSkipCandidate;		/* 次の転送は前と同じ画素のはず（省いてよいと判断した描画の検査用） */

BOOL W32VerifyMode(void)
{
	static int s_nMode = -1;
	if (s_nMode < 0) {
		s_nMode = W32JsVerifyEnabled();
	}
	return s_nMode == 1;
}

void W32VerifySkipCandidate(void)
{
	s_bSkipCandidate = TRUE;
}

/* 変わった範囲（dx, dy, dw, dh）だけ canvas へ出す。canvas の大きさが変わった時は全部出す */
EM_JS(void, W32JsPresent, (const unsigned char *pRgba, int cx, int cy, int dx, int dy, int dw, int dh, int bVerify), {
	var canvas = Module['canvas'];
	if (!canvas) {
		return;
	}
	if (canvas.width !== cx || canvas.height !== cy) {
		canvas.width = cx;
		canvas.height = cy;
		dx = 0; dy = 0; dw = cx; dh = cy;
	}
	var ctx = Module['w32Ctx'];
	if (!ctx || Module['w32CtxCanvas'] !== canvas) {
		/* 画面は不透明なので alpha:false にして、ブラウザの合成の手間を減らす */
		ctx = canvas.getContext('2d', { alpha: false });
		Module['w32Ctx'] = ctx;
		Module['w32CtxCanvas'] = canvas;
		dx = 0; dy = 0; dw = cx; dh = cy;
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
		dx = 0; dy = 0; dw = cx; dh = cy;
	}
	if (dw > 0 && dh > 0) {
		ctx.putImageData(img, 0, 0, dx, dy, dw, dh);
	}
	if (bVerify) {
		var got = ctx.getImageData(0, 0, cx, cy).data;
		var want = HEAPU8.subarray(pRgba, pRgba + cx * cy * 4);
		var bad = 0;
		for (var i = 0; i < want.length; i++) {
			if (got[i] !== want[i]) { bad++; }
		}
		if (bad) {
			Module['sboVerifyFail'] = (Module['sboVerifyFail'] || 0) + 1;
			console.warn('[verify] canvas が全画面転送と違います: ' + bad + ' バイト');
		}
	}
	/* 表示側（シェル）が拡大表示用の後処理をしたい時のための呼び出し口 */
	if (Module['w32OnPresent']) {
		Module['w32OnPresent']();
	}
});

EM_JS(void, W32JsVerifyFail, (const char *pszText), {
	Module['sboVerifyFail'] = (Module['sboVerifyFail'] || 0) + 1;
	console.warn('[verify] ' + UTF8ToString(pszText));
});

/* B,G,R,X の並びを R,G,B,A(255) にして、1 行のうち [x0, x1) を変換する */
static inline void ConvertRow(const DWORD *pSrc, DWORD *pDst, int x0, int x1)
{
	for (int x = x0; x < x1; x ++) {
		DWORD dw = pSrc[x];
		pDst[x] = 0xFF000000 | ((dw & 0xFF) << 16) | (dw & 0xFF00) | ((dw >> 16) & 0xFF);
	}
}

void W32PresentScreen(void)
{
	/* s_aRgba … いま canvas にある絵（R,G,B,A）。s_aPrev … 同じ絵の元（B,G,R,X） */
	static std::vector<BYTE>	s_aRgba;
	static std::vector<DWORD>	s_aPrev;
	static int					s_nVerify = -1;
	W32GdiObj *p = W32GetGdi(s_hBmpScreen);
	if (p == NULL) {
		return;
	}
	double dStart = W32PerfNow();
	if (s_nVerify < 0) {
		s_nVerify = W32JsVerifyEnabled();
	}
	size_t nPixels = (size_t)p->cx * p->cy;
	BOOL bFull = FALSE;
	if (s_aPrev.size() != nPixels || s_aRgba.size() != nPixels * 4) {
		s_aRgba.assign(nPixels * 4, 0);
		s_aPrev.assign(nPixels, 0);
		bFull = TRUE;
	}
	/* 前回から変わった範囲を求める（行ごとに比べ、変わった行だけ変換する） */
	int nMinX = p->cx, nMaxX = -1, nMinY = p->cy, nMaxY = -1;
	DWORD *pRgba = (DWORD *)&s_aRgba[0];
	for (int y = 0; y < p->cy; y ++) {
		const DWORD *pSrc = (const DWORD *)PixelPtr(p, 0, y);
		DWORD *pPrev = &s_aPrev[(size_t)y * p->cx];
		int x0 = 0, x1 = p->cx;
		if (!bFull) {
			if (memcmp(pSrc, pPrev, (size_t)p->cx * 4) == 0) {
				continue;
			}
			while (x0 < x1 && pSrc[x0] == pPrev[x0]) {
				x0 ++;
			}
			while (x1 > x0 && pSrc[x1 - 1] == pPrev[x1 - 1]) {
				x1 --;
			}
		}
		ConvertRow(pSrc, pRgba + (size_t)y * p->cx, x0, x1);
		memcpy(pPrev + x0, pSrc + x0, (size_t)(x1 - x0) * 4);
		nMinX = std::min(nMinX, x0);
		nMaxX = std::max(nMaxX, x1 - 1);
		nMinY = std::min(nMinY, y);
		nMaxY = std::max(nMaxY, y);
	}
	if (s_bSkipCandidate) {
		s_bSkipCandidate = FALSE;
		if (nMaxX >= 0 && !bFull) {
			W32JsVerifyFail("省いてよいと判断した描画で画素が変わりました");
		}
	}
	if (s_nVerify == 1) {
		/* 全画面を変換し直して、差分で更新した絵と同じか確かめる */
		int nBad = 0;
		for (int y = 0; y < p->cy; y ++) {
			const DWORD *pSrc = (const DWORD *)PixelPtr(p, 0, y);
			for (int x = 0; x < p->cx; x ++) {
				DWORD dw = pSrc[x];
				DWORD dwWant = 0xFF000000 | ((dw & 0xFF) << 16) | (dw & 0xFF00) | ((dw >> 16) & 0xFF);
				if (pRgba[(size_t)y * p->cx + x] != dwWant) {
					nBad ++;
				}
			}
		}
		if (nBad) {
			char szMsg[64];
			snprintf(szMsg, sizeof(szMsg), "差分の変換が全画面と違います: %d 画素", nBad);
			W32JsVerifyFail(szMsg);
		}
	}
	if (nMaxX >= 0 || bFull || s_nVerify == 1) {
		if (bFull) {
			nMinX = 0; nMinY = 0; nMaxX = p->cx - 1; nMaxY = p->cy - 1;
		}
		int dw = (nMaxX >= 0) ? nMaxX - nMinX + 1 : 0;
		int dh = (nMaxX >= 0) ? nMaxY - nMinY + 1 : 0;
		W32JsPresent(&s_aRgba[0], p->cx, p->cy, (nMaxX >= 0) ? nMinX : 0, (nMaxX >= 0) ? nMinY : 0, dw, dh,
			s_nVerify == 1 ? 1 : 0);
	}
	s_bScreenDirty = FALSE;
	if (dStart) {
		W32PerfAdd(W32PERF_PRESENT, W32PerfNow() - dStart);
	}
}
#else
BOOL W32VerifyMode(void)
{
	return FALSE;
}

void W32VerifySkipCandidate(void)
{
}

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

/* 横 1 行を半透明の色で塗る（文字の後ろの帯用）。
   x から cx ドットは nAlpha（0〜255）で塗り、その右の nFade ドットはだんだん薄くして 0 にする */
void W32ShadeSpan(HDC hDC, int x, int y, int cx, int nFade, COLORREF cl, int nAlpha)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || y < 0 || y >= pBmp->cy || nAlpha <= 0) {
		return;
	}
	MarkDirty(pDc);
	int nR = GetRValue(cl), nG = GetGValue(cl), nB = GetBValue(cl);
	int x2 = std::min(x + cx + nFade, pBmp->cx);
	for (int xx = std::max(x, 0); xx < x2; xx ++) {
		int a = nAlpha;
		if (xx >= x + cx) {
			/* ぼかし：右へ行くほど薄く */
			a = nAlpha * (x + cx + nFade - xx) / (nFade + 1);
		}
		BYTE *p = PixelPtr(pBmp, xx, y);	/* B,G,R,X */
		p[0] = (BYTE)((p[0] * (255 - a) + nB * a) / 255);
		p[1] = (BYTE)((p[1] * (255 - a) + nG * a) / 255);
		p[2] = (BYTE)((p[2] * (255 - a) + nR * a) / 255);
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
#define FONT_PATH_EMOJI		"/font/NotoEmoji-Regular.ttf"	/* 白黒の絵文字（文字色・縁取りがそのまま効く） */
#define FONT_PATH_DOT16		"/font/DotGothic16-Regular.ttf"	/* 16 ドットのマス用のドット絵フォント */

/* 大きさごとに TTF_Font を共有する */
static std::map<int, TTF_Font *> s_mapTtf;
static std::map<TTF_Font *, int> s_mapTtfSize;		/* TTF_Font → 1 文字のマスの大きさ（CreateFont の高さ） */
static std::map<int, TTF_Font *> s_mapEmoji;		/* マスの大きさごとの絵文字フォント */
static std::map<TTF_Font *, TTF_Font *> s_mapTtfFallback;	/* ドット絵フォント → 無い字を描く BIZ UDゴシック */
static std::map<TTF_Font *, int> s_mapTtfAscent;	/* 行の中の字の位置をそろえるための高さ（BIZ UDゴシックの値） */
static std::map<TTF_Font *, int> s_mapTtfHeight;

static TTF_Font *OpenBizTtf(int nSize)
{
	/* 字は 1 ドット小さく開き、マス（nSize）の中に置く。
	   BIZ UDゴシックは字がマスいっぱいに描かれるので、行を nSize ドットおきに並べると
	   下の行の黒縁取りが上の行の最下段を塗りつぶしてしまう。ＭＳ ゴシックと同じく
	   字の右と下に 1 ドットのすき間を空ける */
	int nGlyph = (nSize >= 10) ? nSize - 1 : nSize;
	TTF_Font *pFont = TTF_OpenFont(FONT_PATH_REGULAR, nGlyph);
	if (pFont == NULL) {
		printf("[w32] フォントを開けません: %s\n", TTF_GetError());
	} else {
		/* MS ゴシックのビットマップに近づけるため、ヒンティングを強めにかける */
		TTF_SetFontHinting(pFont, TTF_HINTING_MONO);
	}
	return pFont;
}

/* 字の位置（ベースライン）と行の高さ。ドット絵フォントも BIZ UDゴシックと同じ所に字が来るようにする */
static int FontAscentOf(TTF_Font *pTtf)
{
	std::map<TTF_Font *, int>::iterator it = s_mapTtfAscent.find(pTtf);
	return (it != s_mapTtfAscent.end()) ? it->second : TTF_FontAscent(pTtf);
}
static int FontHeightOf(TTF_Font *pTtf)
{
	std::map<TTF_Font *, int>::iterator it = s_mapTtfHeight.find(pTtf);
	return (it != s_mapTtfHeight.end()) ? it->second : TTF_FontHeight(pTtf);
}

static TTF_Font *GetTtf(int nSize)
{
	if (!TTF_WasInit() && TTF_Init() != 0) {
		return NULL;
	}
	nSize = std::max(nSize, 6);
	std::map<int, TTF_Font *>::iterator it = s_mapTtf.find(nSize);
	if (it != s_mapTtf.end()) {
		return it->second;
	}
	/* 太字の指定があっても標準の太さで描く。
	   元の Windows 版は太字の無い「ＭＳ ゴシック」を太字指定で使っていて、Windows が 1 ドット右へ
	   重ねる疑似太字を作っていた。同じように重ねると字がマス（nSize）の横幅いっぱいになり、
	   黒縁取りで隣の字とくっついて読みにくい（特にスマホで縮めて表示した時）ので、重ねない */
	TTF_Font *pBiz = OpenBizTtf(nSize);
	TTF_Font *pFont = pBiz;
	if (nSize == 16 && pBiz) {
		/* 16 ドットのマス（ウィンドウ・メッセージなど、ほとんどの字）はドット絵フォントで描く。
		   BIZ UDゴシックを 15 ドットに縮めると、線が 1 ドットの所と 2 ドットの所が混ざって
		   字の一部が太く・伸びたように見える。DotGothic16 は 16 ドットのマスに 15 ドットの字が
		   ドットで描いてあるので、線がどこも 1 ドットでそろう（元の ＭＳ ゴシックの見た目にも近い）。
		   ドット絵フォントに無い字は BIZ UDゴシックで描く */
		TTF_Font *pDot = TTF_OpenFont(FONT_PATH_DOT16, nSize);
		if (pDot == NULL) {
			printf("[w32] フォントを開けません: %s\n", TTF_GetError());
		} else {
			TTF_SetFontHinting(pDot, TTF_HINTING_MONO);
			s_mapTtfFallback[pDot] = pBiz;
			s_mapTtfSize[pBiz] = nSize;
			s_mapTtfAscent[pDot] = TTF_FontAscent(pBiz);
			s_mapTtfHeight[pDot] = TTF_FontHeight(pBiz);
			pFont = pDot;
		}
	}
	if (pFont) {
		s_mapTtfSize[pFont] = nSize;
	}
	s_mapTtf[nSize] = pFont;
	return pFont;
}

/* 本文のフォントと同じ大きさの絵文字フォント（無ければ NULL） */
static TTF_Font *GetEmojiTtf(TTF_Font *pMain)
{
	std::map<TTF_Font *, int>::iterator itSize = s_mapTtfSize.find(pMain);
	if (itSize == s_mapTtfSize.end()) {
		return NULL;
	}
	int nSize = itSize->second;
	std::map<int, TTF_Font *>::iterator it = s_mapEmoji.find(nSize);
	if (it != s_mapEmoji.end()) {
		return it->second;
	}
	/* Noto Emoji は同じ大きさだと本文の字より縦横とも大きく、マスからはみ出して切れるので小さめに開く */
	TTF_Font *pFont = TTF_OpenFont(FONT_PATH_EMOJI, std::max(nSize * 13 / 16, 6));
	if (pFont == NULL) {
		printf("[w32] 絵文字フォントを開けません: %s\n", TTF_GetError());
	} else {
		TTF_SetFontHinting(pFont, TTF_HINTING_MONO);
	}
	s_mapEmoji[nSize] = pFont;
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
	TTF_Font *pTtf = GetTtf(pFont->nHeight);
	if (pTtf) {
		TTF_SetFontStyle(pTtf, pFont->bItalic ? TTF_STYLE_ITALIC : TTF_STYLE_NORMAL);
	}
	return pTtf;
}

/* 同じフォントで続けて描く文字のまとまり */
struct W32TextRun {
	TTF_Font		*pTtf;
	std::string		str;
};

/* UTF-8 の 1 文字を取り出す（正しい UTF-8 を渡すこと。戻り値はバイト数） */
static int Utf8Next(const std::string &str, size_t i, Uint32 &dwCode)
{
	BYTE c = (BYTE)str[i];
	int n = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
	if (i + n > str.size()) {
		n = (int)(str.size() - i);
	}
	dwCode = (n == 1) ? c : (n == 2) ? (c & 0x1F) : (n == 3) ? (c & 0x0F) : (c & 0x07);
	for (int k = 1; k < n; k ++) {
		dwCode = (dwCode << 6) | ((BYTE)str[i + k] & 0x3F);
	}
	return std::max(n, 1);
}

/* 絵文字の見た目を変えるだけの文字（ZWJ・異体字セレクタ・肌の色・タグ文字） */
static BOOL IsEmojiModifier(Uint32 dwCode)
{
	return ((dwCode == 0x200D) ||
			(dwCode >= 0xFE00 && dwCode <= 0xFE0F) ||
			(dwCode >= 0x1F3FB && dwCode <= 0x1F3FF) ||
			(dwCode >= 0xE0020 && dwCode <= 0xE007F)) ? TRUE : FALSE;
}

/* 文字列を「本文のフォント」と「絵文字フォント」で描く部分に分ける。
   本文のフォントに無い文字と、絵文字として描く指定（U+FE0F）が付いた文字・U+1F000 以降は絵文字フォントで描く */
static void SplitTextRuns(TTF_Font *pMain, const std::string &strUtf8, std::vector<W32TextRun> &aRun)
{
	TTF_Font *pEmoji = NULL;
	BOOL bEmojiChecked = FALSE;
	aRun.clear();
	for (size_t i = 0; i < strUtf8.size(); ) {
		Uint32 dwCode;
		int n = Utf8Next(strUtf8, i, dwCode);
		TTF_Font *pUse = pMain;
		if (IsEmojiModifier(dwCode)) {
			/* 絵文字の続きならくっつける。本文のフォントでは描けないので捨てる */
			if (!aRun.empty() && aRun.back().pTtf != pMain) {
				aRun.back().str.append(strUtf8, i, n);
			}
			i += n;
			continue;
		}
		if (dwCode >= 0x80) {
			Uint32 dwNext = 0;
			if (i + n < strUtf8.size()) {
				Utf8Next(strUtf8, i + n, dwNext);
			}
			BOOL bWantEmoji = (dwCode >= 0x1F000 || dwNext == 0xFE0F) ? TRUE : FALSE;
			if (!bWantEmoji && !TTF_GlyphIsProvided32(pMain, dwCode)) {
				/* ドット絵フォントに無い字は BIZ UDゴシックで */
				std::map<TTF_Font *, TTF_Font *>::iterator itFb = s_mapTtfFallback.find(pMain);
				if (itFb != s_mapTtfFallback.end() && TTF_GlyphIsProvided32(itFb->second, dwCode)) {
					pUse = itFb->second;
				}
			}
			if (pUse == pMain && (bWantEmoji || !TTF_GlyphIsProvided32(pMain, dwCode))) {
				if (!bEmojiChecked) {
					pEmoji = GetEmojiTtf(pMain);
					bEmojiChecked = TRUE;
				}
				if (pEmoji && TTF_GlyphIsProvided32(pEmoji, dwCode)) {
					pUse = pEmoji;
				}
			}
		}
		if (aRun.empty() || aRun.back().pTtf != pUse) {
			W32TextRun run;
			run.pTtf = pUse;
			aRun.push_back(run);
		}
		aRun.back().str.append(strUtf8, i, n);
		i += n;
	}
}

/* マスに置く 1 文字 */
struct W32TextCell {
	TTF_Font		*pTtf;
	std::string		str;
	int				nX;			/* 置く位置（文字列の左端から） */
};

/* 文字列を 1 文字ずつマスに並べる。ＭＳ ゴシックと同じく、半角はマスの半分・全角と絵文字はマス 1 つ分ずつ進む。
   戻り値は全体の送り幅 */
static int LayoutTextCells(TTF_Font *pMain, const std::string &strUtf8, std::vector<W32TextCell> &aCell)
{
	aCell.clear();
	std::map<TTF_Font *, int>::iterator itSize = s_mapTtfSize.find(pMain);
	int nCell = (itSize != s_mapTtfSize.end()) ? itSize->second : TTF_FontHeight(pMain);
	std::map<TTF_Font *, TTF_Font *>::iterator itFb = s_mapTtfFallback.find(pMain);
	TTF_Font *pFallback = (itFb != s_mapTtfFallback.end()) ? itFb->second : NULL;
	std::vector<W32TextRun> aRun;
	SplitTextRuns(pMain, strUtf8, aRun);
	int nX = 0;
	for (size_t r = 0; r < aRun.size(); r ++) {
		const std::string &str = aRun[r].str;
		for (size_t i = 0; i < str.size(); ) {
			Uint32 dwCode;
			int n = Utf8Next(str, i, dwCode);
			if (aRun[r].pTtf != pMain && IsEmojiModifier(dwCode)) {
				/* 絵文字の見た目を変えるだけの文字は直前の絵文字にくっつける（白黒の絵文字フォントでは描かれない） */
				if (!aCell.empty()) {
					aCell.back().str.append(str, i, n);
				}
				i += n;
				continue;
			}
			W32TextCell cell;
			cell.pTtf = aRun[r].pTtf;
			cell.str.assign(str, i, n);
			cell.nX = nX;
			aCell.push_back(cell);
			int nStep = nCell;
			if (aRun[r].pTtf == pMain || aRun[r].pTtf == pFallback) {
				int nMinX, nMaxX, nMinY, nMaxY, nAdv = 0;
				if (dwCode < 0x80 || (dwCode >= 0xFF61 && dwCode <= 0xFF9F)) {
					nStep = nCell / 2;	/* 半角（英数字・半角カナ） */
				} else if (TTF_GlyphMetrics32(aRun[r].pTtf, dwCode, &nMinX, &nMaxX, &nMinY, &nMaxY, &nAdv) == 0 &&
						nAdv * 2 <= nCell) {
					nStep = nCell / 2;	/* 半角 */
				}
			}
			nX += nStep;
			i += n;
		}
	}
	return nX;
}

/* 描いた文字の形（1bit マスク）を覚えておく。TextOut2 は同じ文字列を 5 回描くので効く */
struct W32TextMask {
	std::string			strKey;
	int					cx, cy;
	std::vector<BYTE>	aMask;
};
static std::list<W32TextMask>	s_listMask;
static std::unordered_map<std::string, std::list<W32TextMask>::iterator>	s_mapMask;	/* 文字列 → s_listMask の位置 */
#define TEXTMASK_CACHE_MAX		256

static const W32TextMask *GetTextMask(TTF_Font *pTtf, const std::string &strUtf8)
{
	char szKey[32];
	snprintf(szKey, sizeof(szKey), "%p:", (void *)pTtf);
	std::string strKey = std::string(szKey) + strUtf8;
	std::unordered_map<std::string, std::list<W32TextMask>::iterator>::iterator itFind = s_mapMask.find(strKey);
	if (itFind != s_mapMask.end()) {
		std::list<W32TextMask>::iterator it = itFind->second;
		if (it != s_listMask.begin()) {
			s_listMask.splice(s_listMask.begin(), s_listMask, it);	/* splice しても iterator は無効にならない */
		}
		return &s_listMask.front();
	}
	SDL_Color clWhite = {255, 255, 255, 255};
	std::vector<W32TextCell> aCell;
	int nAdvance = LayoutTextCells(pTtf, strUtf8, aCell);

	/* 1 文字ずつ描いてマスに置く。絵文字フォントはベースラインを本文に合わせる */
	std::vector<SDL_Surface *> aSurf(aCell.size(), (SDL_Surface *)NULL);
	W32TextMask mask;
	mask.strKey = strKey;
	mask.cx = nAdvance;
	mask.cy = FontHeightOf(pTtf);
	for (size_t c = 0; c < aCell.size(); c ++) {
		aSurf[c] = TTF_RenderUTF8_Solid(aCell[c].pTtf, aCell[c].str.c_str(), clWhite);
		if (aSurf[c]) {
			mask.cx = std::max(mask.cx, aCell[c].nX + aSurf[c]->w);
		}
	}
	if (mask.cx <= 0) {
		for (size_t c = 0; c < aSurf.size(); c ++) {
			if (aSurf[c]) {
				SDL_FreeSurface(aSurf[c]);
			}
		}
		return NULL;
	}
	mask.aMask.assign((size_t)mask.cx * mask.cy, 0);
	int nAscent = FontAscentOf(pTtf);
	for (size_t c = 0; c < aCell.size(); c ++) {
		SDL_Surface *pSurf = aSurf[c];
		if (pSurf == NULL) {
			continue;
		}
		int nOffY = nAscent - TTF_FontAscent(aCell[c].pTtf);
		SDL_LockSurface(pSurf);
		for (int y = 0; y < pSurf->h; y ++) {
			int dy = y + nOffY;
			if (dy < 0 || dy >= mask.cy) {
				continue;
			}
			const BYTE *pSrc = (const BYTE *)pSurf->pixels + (size_t)y * pSurf->pitch;
			for (int x = 0; x < pSurf->w; x ++) {
				if (pSrc[x]) {
					mask.aMask[(size_t)dy * mask.cx + aCell[c].nX + x] = 1;
				}
			}
		}
		SDL_UnlockSurface(pSurf);
		SDL_FreeSurface(pSurf);
	}
	s_listMask.push_front(mask);
	s_mapMask[strKey] = s_listMask.begin();
	if (s_listMask.size() > TEXTMASK_CACHE_MAX) {
		s_mapMask.erase(s_listMask.back().strKey);
		s_listMask.pop_back();
	}
	return &s_listMask.front();
}

/* 文字の形（マスク）を bitmap へ描く */
static void DrawTextMask(W32Dc *pDc, W32GdiObj *pBmp, const W32TextMask *pMask, int x, int y, COLORREF clText)
{
	MarkDirty(pDc);
	if (pDc->nBkMode == OPAQUE) {
		FillBits(pBmp, x, y, pMask->cx, pMask->cy, ColorToPixel(pDc->clBk));
	}
	DWORD dwText = ColorToPixel(clText);
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
}

/* 文字列の形を取り出す（空文字列の時は *pbEmpty = TRUE で NULL） */
static const W32TextMask *PrepareTextMask(W32Dc *pDc, LPCSTR pszText, int nLen, BOOL *pbEmpty)
{
	*pbEmpty = FALSE;
	TTF_Font *pTtf = DcTtf(pDc);
	if (pTtf == NULL) {
		return NULL;
	}
	std::string strUtf8 = W32TextToUtf8Str(pszText, nLen);
	if (strUtf8.empty()) {
		*pbEmpty = TRUE;
		return NULL;
	}
	return GetTextMask(pTtf, strUtf8);
}

BOOL TextOut(HDC hDC, int x, int y, LPCSTR pszText, int nLen)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || pszText == NULL || nLen <= 0) {
		return FALSE;
	}
	BOOL bEmpty;
	const W32TextMask *pMask = PrepareTextMask(pDc, pszText, nLen, &bEmpty);
	if (bEmpty) {
		return TRUE;
	}
	if (pMask == NULL) {
		return FALSE;
	}
	DrawTextMask(pDc, pBmp, pMask, x, y, pDc->clText);
	return TRUE;
}

/* 黒縁取りの文字（上下左右 1 ドットに縁の色で描いてから、本体の色で重ねる）。
   TextOut を縁取り色で 4 回・本体色で 1 回呼ぶのと同じ画素になる（文字の形の検索は 1 回で済ます）。
   終わった時の文字色は本体色 */
BOOL TextOutOutline(HDC hDC, int x, int y, LPCSTR pszText, int nLen, COLORREF clOutline, COLORREF clText)
{
	W32Dc *pDc = W32GetDc(hDC);
	W32GdiObj *pBmp = DcBitmap(pDc);
	if (pBmp == NULL || pszText == NULL || nLen <= 0) {
		return FALSE;
	}
	pDc->clText = clOutline;
	BOOL bEmpty;
	const W32TextMask *pMask = PrepareTextMask(pDc, pszText, nLen, &bEmpty);
	pDc->clText = clText;
	if (bEmpty) {
		return TRUE;
	}
	if (pMask == NULL) {
		return FALSE;
	}
	DrawTextMask(pDc, pBmp, pMask, x - 1, y, clOutline);
	DrawTextMask(pDc, pBmp, pMask, x + 1, y, clOutline);
	DrawTextMask(pDc, pBmp, pMask, x, y - 1, clOutline);
	DrawTextMask(pDc, pBmp, pMask, x, y + 1, clOutline);
	DrawTextMask(pDc, pBmp, pMask, x, y, clText);
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
	pSize->cy = FontHeightOf(pTtf);
	if (pszText == NULL || nLen <= 0) {
		return TRUE;
	}
	std::string strUtf8 = W32TextToUtf8Str(pszText, nLen);
	std::vector<W32TextCell> aCell;
	pSize->cx = LayoutTextCells(pTtf, strUtf8, aCell);
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
	pTm->tmHeight = FontHeightOf(pTtf);
	pTm->tmAscent = FontAscentOf(pTtf);
	pTm->tmDescent = pTm->tmHeight - pTm->tmAscent;
	std::map<TTF_Font *, int>::iterator itSize = s_mapTtfSize.find(pTtf);
	int nCell = (itSize != s_mapTtfSize.end()) ? itSize->second : TTF_FontHeight(pTtf);
	pTm->tmAveCharWidth = nCell / 2;
	pTm->tmMaxCharWidth = nCell;
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

BOOL TextOutOutline(HDC, int, int, LPCSTR, int, COLORREF, COLORREF)
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
