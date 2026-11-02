/* ========================================================================= */
/* ファイル名：	w32_perf.cpp												 */
/* 概要：		ブラウザ版の処理時間の計測									 */
/* ========================================================================= */
/*
 * ページの URL に ?perf=1 を付けた時だけ、1 秒ごとに処理時間を画面の左上へ出す。
 * 付けていない時は W32PerfAdd などは何もしない（時刻も取らない）。
 *
 *   フレーム … MainLoopWeb 1 回分（メッセージ処理・タイマー処理・描画の合計）
 *   処理     … フレームのうち描画以外（キー・通信・ゲームの計算）
 *   描画     … メインウィンドウの WM_PAINT 1 回分（画面への転送を含む）
 *   転送     … 画面用ビットマップを canvas へ出す 1 回分（W32PresentScreen）
 *
 * 同じ内容は Module.sboPerf にも入れるので、開発者ツールのコンソールからも見られる。
 */
#include "../platform/w32_internal.h"

#include <emscripten.h>
#include <stdio.h>
#include <algorithm>

static const char *s_apszName[W32PERF_COUNT] = {
	"フレーム", "処理　　", "描画　　", "転送　　",
};

struct W32PerfStat {
	int		nCount;
	double	dSum;
	double	dMax;
};

static int			s_nEnabled = -1;		/* -1:まだ調べていない 0:無効 1:有効 */
static W32PerfStat	s_aStat[W32PERF_COUNT];
static double		s_dLastReport;
static double		s_dFrameStart;			/* このフレームの始まり */
static double		s_dPaintInFrame;		/* このフレームの中で描画に使った時間 */

EM_JS(int, W32JsPerfEnabled, (void), {
	try {
		return new URLSearchParams(location.search).get('perf') === '1' ? 1 : 0;
	} catch (e) {
		return 0;
	}
});

EM_JS(void, W32JsPerfShow, (const char *pszText), {
	var text = UTF8ToString(pszText);
	var el = document.getElementById('sbo-perf');
	if (!el) {
		el = document.createElement('div');
		el.id = 'sbo-perf';
		el.style.cssText = 'position:fixed;left:4px;top:4px;z-index:10000;pointer-events:none;' +
			'background:rgba(0,0,0,0.6);color:#fff;font:12px/1.4 monospace;padding:4px 6px;' +
			'white-space:pre;border-radius:4px';
		document.body.appendChild(el);
	}
	el.textContent = text;
	Module['sboPerf'] = text;
});

BOOL W32PerfEnabled(void)
{
	if (s_nEnabled < 0) {
		s_nEnabled = W32JsPerfEnabled();
		s_dLastReport = emscripten_get_now();
	}
	return s_nEnabled ? TRUE : FALSE;
}

double W32PerfNow(void)
{
	return W32PerfEnabled() ? emscripten_get_now() : 0;
}

void W32PerfAdd(int nKind, double dMs)
{
	if (!W32PerfEnabled() || nKind < 0 || nKind >= W32PERF_COUNT) {
		return;
	}
	W32PerfStat *p = &s_aStat[nKind];
	p->nCount ++;
	p->dSum += dMs;
	if (dMs > p->dMax) {
		p->dMax = dMs;
	}
	if (nKind == W32PERF_PAINT) {
		s_dPaintInFrame += dMs;
	}
}

void W32PerfBeginFrame(void)
{
	if (!W32PerfEnabled()) {
		return;
	}
	s_dFrameStart = emscripten_get_now();
	s_dPaintInFrame = 0;
}

void W32PerfEndFrame(void)
{
	if (!W32PerfEnabled()) {
		return;
	}
	double dNow = emscripten_get_now();
	double dFrame = dNow - s_dFrameStart;
	W32PerfAdd(W32PERF_FRAME, dFrame);
	W32PerfAdd(W32PERF_LOGIC, std::max(dFrame - s_dPaintInFrame, 0.0));
	double dElapsed = dNow - s_dLastReport;
	if (dElapsed < 1000) {
		return;
	}
	char szText[512];
	int nPos = 0;
	for (int i = 0; i < W32PERF_COUNT; i ++) {
		const W32PerfStat *p = &s_aStat[i];
		double dAvg = p->nCount ? p->dSum / p->nCount : 0;
		nPos += snprintf(szText + nPos, sizeof(szText) - nPos, "%s%s %4.0f回/秒 平均%6.2fms 最大%6.2fms",
			i ? "\n" : "", s_apszName[i], p->nCount * 1000.0 / dElapsed, dAvg, p->dMax);
		if (nPos >= (int)sizeof(szText)) {
			break;
		}
	}
	W32JsPerfShow(szText);
	memset(s_aStat, 0, sizeof(s_aStat));
	s_dLastReport = dNow;
}
