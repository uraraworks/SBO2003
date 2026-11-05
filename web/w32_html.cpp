/* ========================================================================= */
/* ファイル名：	w32_html.cpp												 */
/* 概要：		Win32 互換レイヤ：コントロールと小窓を HTML の部品で代用する	 */
/* ========================================================================= */
/*
 * 仕組み
 *  - メインウィンドウの子の EDIT / BUTTON は、canvas の上に重ねた
 *    <input> / <button> / チェックボックスで表示する。
 *  - 位置は画面サイズに対する割合（%）で指定するので、canvas が拡大されても
 *    ずれない。文字の大きさは --w32-scale（実際の表示幅 ÷ 画面の幅）で合わせる。
 *  - 入力欄でのキー操作は W32JsOnKey に入力欄の番号付きで渡すので、
 *    ゲーム側のサブクラス化したプロシージャで Enter などを受け取れる。
 *  - 文字列は C 側も HTML 側も UTF-8（C 側から渡す時は W32TextToUtf8Str で整える）。
 *
 * 小窓（メニュー付きのウィンドウ・ダイアログ）
 *  - ページの上に、タイトル・メニューバー付きの枠（div）を出す。中のウィンドウは
 *    入れ子の div、コントロールは HTML の部品にして、位置はピクセルで指定する。
 *  - GDI で描いた内容は、ウィンドウごとの <canvas> に写す（W32HtmlPaint）。
 *  - 部品の中身（項目・選択・スクロール位置など）は C 側が持っていて、
 *    操作されたら W32JsOn* で C 側へ知らせ、C 側から表示を直す。
 */
#include "../platform/w32_internal.h"

#include "../platform/include/commctrl.h"

#include <emscripten.h>
#include <stdlib.h>
#include <stdio.h>

static int s_nHtmlIDNext = 1;

/* 部品を置く場所（canvas に重ねた div）を用意する */
EM_JS(void, W32JsHtmlInit, (void), {
	if (Module['w32Overlay']) {
		return;
	}
	var canvas = Module['canvas'];
	var overlay = document.getElementById('sbo-overlay');
	if (!overlay) {
		overlay = document.createElement('div');
		overlay.id = 'sbo-overlay';
		overlay.style.cssText = 'position:absolute;left:0;top:0;right:0;bottom:0;pointer-events:none;';
		if (canvas && canvas.parentNode) {
			canvas.parentNode.style.position = 'relative';
			canvas.parentNode.appendChild(overlay);
		} else {
			document.body.appendChild(overlay);
		}
	}
	Module['w32Overlay'] = overlay;
	Module['w32Html'] = {};

	/* 表示倍率を CSS 変数に入れておく */
	var updateScale = function () {
		if (!canvas || !canvas.width) {
			return;
		}
		var r = canvas.getBoundingClientRect();
		overlay.style.setProperty('--w32-scale', String(r.width / canvas.width));
	};
	Module['w32UpdateScale'] = updateScale;
	if (window.ResizeObserver && canvas) {
		new ResizeObserver(updateScale).observe(canvas);
	}
	window.addEventListener('resize', updateScale);
	updateScale();
});

EM_JS(void, W32JsHtmlCreate, (int nID, int nType, int bPassword, int bLower, int bBorder), {
	/* nType 1:入力欄 2:押しボタン 3:チェックボックス */
	var overlay = Module['w32Overlay'];
	var el;
	var font = 'calc(12px * var(--w32-scale, 1)) "BIZ UDGothic", "MS Gothic", monospace';
	if (nType === 1) {
		el = document.createElement('input');
		el.type = bPassword ? 'password' : 'text';
		el.autocomplete = bPassword ? 'current-password' : 'off';
		el.spellcheck = false;
		el.style.cssText = 'position:absolute;box-sizing:border-box;margin:0;padding:0 2px;outline:none;' +
			'background:#fff;color:#000;border:' + (bBorder ? '1px solid #808080' : 'none') + ';';
		if (bLower) {
			el.addEventListener('input', function () {
				var s = el.selectionStart, e = el.selectionEnd;
				el.value = el.value.toLowerCase();
				el.setSelectionRange(s, e);
			});
		}
		el.addEventListener('keydown', function (ev) {
			var vk = Module['w32KeyToVk'] ? Module['w32KeyToVk'](ev) : ev.keyCode;
			if (vk === 13 || vk === 9 || vk === 27) {
				ev.preventDefault();
			}
			/* IME で変換中のキーはゲームに渡さない */
			if (ev.isComposing || ev.keyCode === 229) {
				return;
			}
			_W32JsOnKey(vk, 1, nID);
			ev.stopPropagation();
		});
		el.addEventListener('keyup', function (ev) {
			var vk = Module['w32KeyToVk'] ? Module['w32KeyToVk'](ev) : ev.keyCode;
			_W32JsOnKey(vk, 0, nID);
			ev.stopPropagation();
		});
	} else if (nType === 2) {
		el = document.createElement('button');
		el.type = 'button';
		el.style.cssText = 'position:absolute;box-sizing:border-box;margin:0;padding:0;';
		el.addEventListener('click', function () {
			_W32JsOnButton(nID);
		});
	} else {
		el = document.createElement('label');
		el.style.cssText = 'position:absolute;box-sizing:border-box;margin:0;padding:0;display:flex;align-items:center;white-space:nowrap;color:#000;';
		var cb = document.createElement('input');
		cb.type = 'checkbox';
		cb.style.margin = '0 4px 0 0';
		var span = document.createElement('span');
		el.appendChild(cb);
		el.appendChild(span);
		el.w32Check = cb;
		el.w32Label = span;
		cb.addEventListener('click', function () {
			_W32JsOnButton(nID);
		});
	}
	el.style.font = font;
	el.style.pointerEvents = 'auto';
	el.style.display = 'none';
	el.dataset.w32Id = String(nID);
	overlay.appendChild(el);
	Module['w32Html'][nID] = el;
});

EM_JS(void, W32JsHtmlDestroy, (int nID), {
	var el = Module['w32Html'] && Module['w32Html'][nID];
	if (!el) {
		return;
	}
	if (el.contains(document.activeElement) && Module['canvas']) {
		Module['canvas'].focus();
	}
	if (el.w32Frame) {
		if (Module['w32Pop'] && Module['w32Pop'].menuOwner === el) {
			Module['w32Pop'].closeMenu();
		}
		el.w32Frame.remove();
	}
	el.remove();
	delete Module['w32Html'][nID];
});

EM_JS(void, W32JsHtmlUpdate, (int nID, double fLeft, double fTop, double fWidth, double fHeight, int bVisible, int bEnabled, int nCheck), {
	var el = Module['w32Html'] && Module['w32Html'][nID];
	if (!el) {
		return;
	}
	el.style.left = fLeft + '%';
	el.style.top = fTop + '%';
	el.style.width = fWidth + '%';
	el.style.height = fHeight + '%';
	el.style.display = bVisible ? '' : 'none';
	if (el.w32Check) {
		el.w32Check.disabled = !bEnabled;
		el.w32Check.checked = !!nCheck;
	} else {
		el.disabled = !bEnabled;
	}
});

EM_JS(void, W32JsHtmlSetText, (int nID, const char *pszUtf8), {
	var el = Module['w32Html'] && Module['w32Html'][nID];
	if (!el) {
		return;
	}
	var text = UTF8ToString(pszUtf8);
	if (el.w32Label) {
		el.w32Label.textContent = text;
	} else if (el.tagName === 'BUTTON') {
		el.textContent = text;
	} else if (el.value !== text) {
		el.value = text;
	}
});

/* 入力欄の内容を UTF-8 で返す（呼んだ側で free する） */
EM_JS(char *, W32JsHtmlGetText, (int nID), {
	var el = Module['w32Html'] && Module['w32Html'][nID];
	if (!el || el.tagName !== 'INPUT') {
		return 0;
	}
	return stringToNewUTF8(el.value);
});

EM_JS(void, W32JsHtmlFocus, (int nID), {
	var el = nID ? (Module['w32Html'] && Module['w32Html'][nID]) : null;
	if (el && el.w32Focus) {
		el = el.w32Focus;
	}
	if (el && (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA' || el.tagName === 'SELECT' || el.tagName === 'BUTTON')) {
		el.focus();
		return;
	}
	var active = document.activeElement;
	if (active && active.dataset && active.dataset.w32Id) {
		active.blur();
	}
	if (Module['canvas']) {
		Module['canvas'].focus();
	}
});

/* ========================================================================= */
/* 小窓																		 */
/* ========================================================================= */
#include "w32_popup_js.inc"

/* 小窓の中のウィンドウか */
static BOOL IsPopupWnd(W32Wnd *pWnd)
{
	return W32GetPopupRoot(pWnd) ? TRUE : FALSE;
}

/* 部品の種類（JavaScript 側の番号） */
static int PopupType(W32Wnd *pWnd)
{
	if (pWnd->bPopup) {
		return 11;
	}
	DWORD dwType = pWnd->dwStyle & 0x0F;
	switch (pWnd->nKind) {
	case W32WND_EDIT:
		return 1;
	case W32WND_RICHEDIT:
		return 1;
	case W32WND_BUTTON:
		switch (dwType) {
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_3STATE:
		case BS_AUTO3STATE:
			return 3;
		case BS_RADIOBUTTON:
		case BS_AUTORADIOBUTTON:
			return 4;
		case BS_GROUPBOX:
			return 5;
		}
		return 2;
	case W32WND_STATIC:
		return 6;
	case W32WND_COMBOBOX:
		return 7;
	case W32WND_LISTBOX:
		return 8;
	case W32WND_SCROLLBAR:
		return 9;
	case W32WND_LISTVIEW:
		return 10;
	default:
		break;
	}
	return 0;
}

static void PopupCreate(W32Wnd *pWnd)
{
	W32JsPopInit();
	W32Wnd *pParent = W32GetWnd(pWnd->hWndParent);
	if (pWnd->bPopup) {
		/* 小窓の枠には、持ち主の小窓を渡す（持ち主より手前に出し続けるため） */
		W32Wnd *pOwner = W32GetWnd(pWnd->hWndOwner);
		pParent = pOwner ? W32GetPopupRoot(pOwner) : NULL;
	}
	pWnd->nHtmlID = s_nHtmlIDNext ++;
	W32JsPopCreate(pWnd->nHtmlID, pParent ? pParent->nHtmlID : 0,
			PopupType(pWnd), (int)pWnd->dwStyle, (int)pWnd->dwExStyle, pWnd->bDialog ? 1 : 0);
	W32HtmlSetText(pWnd);
	if (pWnd->nKind == W32WND_LISTVIEW) {
		W32HtmlListView(pWnd, 6, 0, 0);
	}
}

static void PopupUpdate(W32Wnd *pWnd)
{
	if (pWnd->bPopup) {
		int nScrCx, nScrCy;
		W32GetScreenSize(&nScrCx, &nScrCy);
		W32JsPopPos(pWnd->nHtmlID, pWnd->rc.left, pWnd->rc.top,
				pWnd->rc.right - pWnd->rc.left, pWnd->rc.bottom - pWnd->rc.top,
				pWnd->bVisible, pWnd->bEnabled, 0, nScrCx);
		return;
	}
	W32JsPopPos(pWnd->nHtmlID, pWnd->rc.left, pWnd->rc.top,
			pWnd->rc.right - pWnd->rc.left, pWnd->rc.bottom - pWnd->rc.top,
			pWnd->bVisible, pWnd->bEnabled, pWnd->nCheck, 0);
}

void W32HtmlItems(W32Wnd *pWnd, int nOp, int nIndex, const char *pszText)
{
	if (pWnd->nHtmlID == 0) {
		return;
	}
	W32JsPopItems(pWnd->nHtmlID, nOp, nIndex, W32TextToUtf8Str(pszText ? pszText : "").c_str());
}

void W32HtmlScroll(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID == 0 || pWnd->nKind != W32WND_SCROLLBAR) {
		return;
	}
	W32JsPopScroll(pWnd->nHtmlID, pWnd->nScrMin, pWnd->nScrMax, pWnd->nScrPos, (int)pWnd->nScrPage);
}

void W32HtmlListView(W32Wnd *pWnd, int nOp, int nRow, int nCol)
{
	if (pWnd->nHtmlID == 0) {
		return;
	}
	std::string strText;
	int nValue = 0;
	switch (nOp) {
	case 1:		/* 列 */
		if (nCol >= 0 && nCol < (int)pWnd->aColText.size()) {
			strText = W32TextToUtf8Str(pWnd->aColText[nCol].c_str());
			nValue = pWnd->aColWidth[nCol];
		}
		break;
	case 3:		/* 文字 */
		if (nRow >= 0 && nRow < (int)pWnd->aRows.size() && nCol < (int)pWnd->aRows[nRow].aText.size()) {
			strText = W32TextToUtf8Str(pWnd->aRows[nRow].aText[nCol].c_str());
		}
		break;
	case 5:		/* 状態 */
		if (nRow >= 0 && nRow < (int)pWnd->aRows.size()) {
			UINT uState = pWnd->aRows[nRow].uState;
			nValue = ((uState & LVIS_SELECTED) ? 1 : 0) | ((((uState & LVIS_STATEIMAGEMASK) >> 12) == 2) ? 2 : 0);
		}
		break;
	case 6:		/* 拡張スタイル */
		nValue = (int)pWnd->dwLvExStyle;
		break;
	}
	W32JsPopListView(pWnd->nHtmlID, nOp, nRow, nCol, strText.c_str(), nValue);
}

void W32HtmlPaint(W32Wnd *pWnd, W32GdiObj *pBmp)
{
	if (pWnd->nHtmlID == 0 || pBmp == NULL) {
		return;
	}
	static std::vector<BYTE> s_aRgba;
	size_t nSize = (size_t)pBmp->cx * pBmp->cy * 4;
	if (s_aRgba.size() < nSize) {
		s_aRgba.resize(nSize);
	}
	BYTE *pDst = &s_aRgba[0];
	/* ボトムアップの 32bit DIB（B,G,R,X）→ 上から順の R,G,B,A */
	int nPitch = pBmp->cx * 4;
	for (int y = 0; y < pBmp->cy; y ++) {
		const BYTE *pSrc = pBmp->bTopDown ? pBmp->pBits + y * nPitch : pBmp->pBits + (pBmp->cy - 1 - y) * nPitch;
		for (int x = 0; x < pBmp->cx; x ++) {
			pDst[0] = pSrc[2];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[0];
			pDst[3] = 255;
			pDst += 4;
			pSrc += 4;
		}
	}
	W32JsPopPaint(pWnd->nHtmlID, &s_aRgba[0], pBmp->cx, pBmp->cy);
}

/* 文字を JSON の文字列にする */
static void JsonString(std::string &out, const std::string &strUtf8)
{
	out += '"';
	for (size_t i = 0; i < strUtf8.size(); i ++) {
		unsigned char c = (unsigned char)strUtf8[i];
		if (c == '"' || c == '\\') {
			out += '\\';
			out += (char)c;
		} else if (c < 0x20) {
			char sz[8];
			snprintf(sz, sizeof(sz), "\\u%04x", c);
			out += sz;
		} else {
			out += (char)c;
		}
	}
	out += '"';
}

static void MenuJson(std::string &out, W32Menu *pMenu)
{
	out += '[';
	for (size_t i = 0; pMenu && i < pMenu->aItem.size(); i ++) {
		const W32MenuItem &item = pMenu->aItem[i];
		if (i) {
			out += ',';
		}
		char sz[96];
		snprintf(sz, sizeof(sz), "{\"id\":%u,\"sep\":%d,\"chk\":%d,\"dis\":%d,\"t\":",
				item.uID, (item.uFlags & MF_SEPARATOR) ? 1 : 0, (item.uFlags & MF_CHECKED) ? 1 : 0,
				(item.uFlags & (MF_GRAYED | MF_DISABLED)) ? 1 : 0);
		out += sz;
		JsonString(out, W32TextToUtf8Str(item.strText.c_str()));
		if (item.pSub) {
			out += ",\"sub\":";
			MenuJson(out, item.pSub);
		}
		out += '}';
	}
	out += ']';
}

void W32HtmlMenu(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID == 0 || !pWnd->bPopup) {
		return;
	}
	std::string strJson;
	MenuJson(strJson, W32GetMenu(pWnd->hMenu));
	W32JsPopMenu(pWnd->nHtmlID, strJson.c_str());
}

/* ========================================================================= */
/* 共通の入口																 */
/* ========================================================================= */
void W32HtmlCreate(W32Wnd *pWnd)
{
	if (IsPopupWnd(pWnd)) {
		PopupCreate(pWnd);
		return;
	}
	W32JsHtmlInit();
	int nType = 1;
	if (pWnd->nKind == W32WND_BUTTON) {
		DWORD dwType = pWnd->dwStyle & 0x0F;
		nType = (dwType == BS_CHECKBOX || dwType == BS_AUTOCHECKBOX) ? 3 : 2;
	}
	pWnd->nHtmlID = s_nHtmlIDNext ++;
	W32JsHtmlCreate(pWnd->nHtmlID, nType,
			(pWnd->dwStyle & ES_PASSWORD) ? 1 : 0,
			(pWnd->dwStyle & ES_LOWERCASE) ? 1 : 0,
			((pWnd->dwStyle & WS_BORDER) || (pWnd->dwExStyle & WS_EX_CLIENTEDGE)) ? 1 : 0);
	W32HtmlSetText(pWnd);
}

void W32HtmlDestroy(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID) {
		W32JsHtmlDestroy(pWnd->nHtmlID);
		pWnd->nHtmlID = 0;
	}
}

void W32HtmlUpdate(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID == 0) {
		return;
	}
	if (IsPopupWnd(pWnd)) {
		PopupUpdate(pWnd);
		return;
	}
	int cx, cy;
	W32GetScreenSize(&cx, &cy);
	if (cx <= 0 || cy <= 0) {
		return;
	}
	/* 親が隠れていたら一緒に隠す */
	BOOL bVisible = pWnd->bVisible;
	W32Wnd *pParent = W32GetWnd(pWnd->hWndParent);
	if (pParent && !pParent->bVisible) {
		bVisible = FALSE;
	}
	W32JsHtmlUpdate(pWnd->nHtmlID,
			pWnd->rc.left * 100.0 / cx, pWnd->rc.top * 100.0 / cy,
			(pWnd->rc.right - pWnd->rc.left) * 100.0 / cx, (pWnd->rc.bottom - pWnd->rc.top) * 100.0 / cy,
			bVisible, pWnd->bEnabled, pWnd->nCheck);
}

void W32HtmlSetText(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID == 0) {
		return;
	}
	if (IsPopupWnd(pWnd)) {
		W32JsPopText(pWnd->nHtmlID, W32TextToUtf8Str(pWnd->strText.c_str()).c_str());
		return;
	}
	W32JsHtmlSetText(pWnd->nHtmlID, W32TextToUtf8Str(pWnd->strText.c_str()).c_str());
}

void W32HtmlGetText(W32Wnd *pWnd)
{
	if (pWnd->nHtmlID == 0) {
		return;
	}
	char *pszUtf8 = IsPopupWnd(pWnd) ? W32JsPopGetText(pWnd->nHtmlID) : W32JsHtmlGetText(pWnd->nHtmlID);
	if (pszUtf8 == NULL) {
		return;
	}
	pWnd->strText = W32Utf8ToTextStr(pszUtf8);
	free(pszUtf8);
}

void W32HtmlFocus(W32Wnd *pWnd)
{
	W32JsHtmlFocus(pWnd ? pWnd->nHtmlID : 0);
}
