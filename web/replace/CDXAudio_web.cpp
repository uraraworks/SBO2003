/* ========================================================================= */
/* CDXAudio_web.cpp															 */
/* DirectX Audio を使うためのクラス（Web版）：Web Audio で直接鳴らす		 */
/* ========================================================================= */
/*
 * 効果音は WAVE リソースなので、PCM を取り出して Web Audio の AudioBuffer にしておき、
 * 鳴らす時は AudioBufferSourceNode をその場で作って鳴らす。
 *
 * 以前は SDL_mixer で鳴らしていたが、Emscripten 版の SDL2 は ScriptProcessorNode
 * （メインスレッドで音を作って渡す古い仕組み）で出力するため、スマホでは
 * 鳴らしてから聞こえるまでが PC より大きく遅れていた（1 秒近く遅れることがある）。
 * AudioBufferSourceNode はブラウザの音声スレッドがそのまま鳴らすので、
 * 遅れは端末の出力の遅れだけになる。
 *
 * ゲーム側は IDirectMusicSegment8 * として持っているが、中身は JS 側の効果音の番号（1〜）。
 * 音量は DirectMusic と同じく dB 単位（0 が最大、-100 で無音）で受け取って換算する。
 * プライマリは 1 つだけ鳴らす（新しく鳴らすと前のは止める）。
 */
#include "StdAfx.h"

#include <SDL.h>
#include <emscripten.h>
#include <math.h>
#include <map>

/* ページ側に効果音の仕組みを用意する（何度呼んでもよい） */
EM_JS(int, SboSeInit, (void), {
	if (Module.SboSe) {
		return Module.SboSe.ctx ? 1 : 0;
	}
	var se = Module.SboSe = {
		ctx: null,
		bufs: {},			/* 番号 → AudioBuffer */
		nodes: {},			/* 番号 → 鳴っている AudioBufferSourceNode の一覧 */
		primary: null,		/* プライマリで鳴っているもの { id, node } */
		gainPri: null,
		gainSec: null
	};
	var AC = window.AudioContext || window.webkitAudioContext;
	if (!AC) {
		console.log('[se] この環境では AudioContext が使えないので効果音は鳴りません');
		return 0;
	}
	try {
		se.ctx = new AC({ latencyHint: 'interactive' });
	} catch (e) {
		try {
			se.ctx = new AC();
		} catch (e2) {
			console.log('[se] AudioContext を作れませんでした: ' + e2);
			return 0;
		}
	}
	var ctx = se.ctx;
	se.gainPri = ctx.createGain();
	se.gainPri.connect(ctx.destination);
	se.gainSec = ctx.createGain();
	se.gainSec.connect(ctx.destination);

	/* 操作されたら音を出せるようにする（resume はイベントの中ですぐに呼ばないと
	   Safari などでは断られる） */
	var silentDone = false;
	var unlock = function () {
		if (ctx.state === 'running') {
			return;
		}
		ctx.resume().catch(function () {});
		/* iPhone などは操作の中で何か鳴らすと動き出すので、無音を一瞬鳴らしておく */
		if (!silentDone) {
			silentDone = true;
			try {
				var src = ctx.createBufferSource();
				src.buffer = ctx.createBuffer(1, 1, ctx.sampleRate);
				src.connect(ctx.destination);
				src.start(0);
			} catch (e) {
			}
		}
	};
	/* タッチでは touchstart・pointerdown は「操作」と見なされないので、離した時のイベントも見る */
	['keydown', 'mousedown', 'pointerdown', 'pointerup', 'touchend', 'click'].forEach(function (type) {
		window.addEventListener(type, unlock, true);
	});
	return 1;
});

/* 効果音を登録する（pData は -1.0〜1.0 の float を チャンネル交互に並べたもの） */
EM_JS(int, SboSeCreate, (int nId, const float *pData, int nFrames, int nChannels, int nRate), {
	var se = Module.SboSe;
	if (!se || !se.ctx || nFrames <= 0) {
		return 0;
	}
	var ctx = se.ctx;
	var src = HEAPF32.subarray(pData >> 2, (pData >> 2) + nFrames * nChannels);
	var rate = nRate;
	var frames = nFrames;
	var buf = null;
	try {
		buf = ctx.createBuffer(nChannels, frames, rate);
	} catch (e) {
		buf = null;
	}
	var step = 1;
	if (!buf) {
		/* 古いブラウザは低いサンプリング周波数の AudioBuffer を作れないので、こちらで引き伸ばす */
		rate = ctx.sampleRate;
		frames = Math.max(1, Math.floor(nFrames * rate / nRate));
		step = nRate / rate;
		buf = ctx.createBuffer(nChannels, frames, rate);
	}
	for (var c = 0; c < nChannels; c++) {
		var dst = buf.getChannelData(c);
		if (step === 1) {
			for (var i = 0; i < frames; i++) {
				dst[i] = src[i * nChannels + c];
			}
		} else {
			for (var i = 0; i < frames; i++) {
				var p = i * step;
				var i0 = Math.floor(p);
				var i1 = Math.min(i0 + 1, nFrames - 1);
				var t = p - i0;
				dst[i] = src[i0 * nChannels + c] * (1 - t) + src[i1 * nChannels + c] * t;
			}
		}
	}
	se.bufs[nId] = buf;
	se.nodes[nId] = [];
	return 1;
});

EM_JS(void, SboSeStopId, (int nId), {
	var se = Module.SboSe;
	if (!se) {
		return;
	}
	var list = se.nodes[nId];
	if (list) {
		se.nodes[nId] = [];
		list.forEach(function (n) {
			try {
				n.onended = null;
				n.stop();
			} catch (e) {
			}
			n.disconnect();
		});
	}
	if (se.primary && se.primary.id === nId) {
		se.primary = null;
	}
});

EM_JS(void, SboSeDelete, (int nId), {
	var se = Module.SboSe;
	if (!se) {
		return;
	}
	delete se.bufs[nId];
	delete se.nodes[nId];
});

/* 鳴らす（nLoops: 0=1回、n=n回繰り返す、-1=ずっと。bPrimary: プライマリで鳴らす。nDelayMs: この時間だけ後に鳴らす） */
EM_JS(int, SboSePlay, (int nId, int nLoops, int bPrimary, int nDelayMs), {
	var se = Module.SboSe;
	if (!se || !se.ctx || !se.bufs[nId]) {
		return 0;
	}
	var ctx = se.ctx;
	/* 音を出せない（操作される前など）間は鳴らさない。止まっている間に鳴らすと、
	   動き出した時にまとめて遅れて聞こえてしまう */
	if (ctx.state !== 'running') {
		return 0;
	}
	if (bPrimary && se.primary) {
		var old = se.primary;
		se.primary = null;
		var list = se.nodes[old.id] || [];
		var idx = list.indexOf(old.node);
		if (idx >= 0) {
			list.splice(idx, 1);
		}
		try {
			old.node.onended = null;
			old.node.stop();
		} catch (e) {
		}
		old.node.disconnect();
	}
	var buf = se.bufs[nId];
	var node = ctx.createBufferSource();
	node.buffer = buf;
	node.connect(bPrimary ? se.gainPri : se.gainSec);
	var now = ctx.currentTime + Math.max(nDelayMs, 0) / 1000;
	if (nLoops != 0) {
		node.loop = true;
	}
	node.onended = function () {
		var list = se.nodes[nId];
		if (list) {
			var i = list.indexOf(node);
			if (i >= 0) {
				list.splice(i, 1);
			}
		}
		if (se.primary && se.primary.node === node) {
			se.primary = null;
		}
		node.disconnect();
	};
	node.start(now);
	if (nLoops > 0) {
		node.stop(now + buf.duration * (nLoops + 1));
	}
	se.nodes[nId].push(node);
	if (bPrimary) {
		se.primary = { id: nId, node: node };
	}
	return 1;
});

EM_JS(void, SboSeStopPrimary, (void), {
	var se = Module.SboSe;
	if (se && se.primary) {
		var old = se.primary;
		se.primary = null;
		var list = se.nodes[old.id] || [];
		var idx = list.indexOf(old.node);
		if (idx >= 0) {
			list.splice(idx, 1);
		}
		try {
			old.node.onended = null;
			old.node.stop();
		} catch (e) {
		}
		old.node.disconnect();
	}
});

EM_JS(void, SboSeStopAll, (void), {
	var se = Module.SboSe;
	if (!se) {
		return;
	}
	Object.keys(se.nodes).forEach(function (id) {
		var list = se.nodes[id];
		se.nodes[id] = [];
		list.forEach(function (n) {
			try {
				n.onended = null;
				n.stop();
			} catch (e) {
			}
			n.disconnect();
		});
	});
	se.primary = null;
});

EM_JS(void, SboSeSetVolume, (int bPrimary, double fGain), {
	var se = Module.SboSe;
	if (se && se.ctx) {
		(bPrimary ? se.gainPri : se.gainSec).gain.value = fGain;
	}
});

EM_JS(int, SboSeIsPlaying, (int nId), {
	var se = Module.SboSe;
	return (se && se.nodes[nId] && se.nodes[nId].length > 0) ? 1 : 0;
});

static BOOL					s_bOpen;
static int					s_nNextId = 1;
static std::map<int, int>	s_mapRepeat;	/* 効果音ごとの繰り返し回数 */

static int ToId(IDirectMusicSegment8 **pSeg)
{
	return pSeg ? (int)(intptr_t)*pSeg : 0;
}

/* dB → 音量の倍率（0.0〜1.0） */
static double DbToGain(long lDb)
{
	if (lDb <= -100) {
		return 0.0;
	}
	double fGain = pow(10.0, (double)lDb / 20.0);
	/* SDL_mixer で鳴らしていた時と同じく、1.0 を超える分は切る */
	return (fGain > 1.0) ? 1.0 : fGain;
}

CDXAudio::CDXAudio(void)
{
	m_pPerformance		= NULL;
	m_pDefAudioPath		= NULL;
	m_pDefAudioPath2	= NULL;
	m_pLoader			= NULL;
	m_hResource			= NULL;
}

CDXAudio::~CDXAudio(void)
{
	Destroy();
}

BOOL CDXAudio::Create(void)
{
	if (s_bOpen) {
		return TRUE;
	}
	if (!SboSeInit()) {
		/* 音が出なくてもゲームは続ける */
		return TRUE;
	}
	s_bOpen = TRUE;
	return TRUE;
}

void CDXAudio::Destroy(void)
{
	if (!s_bOpen) {
		return;
	}
	SboSeStopAll();
	s_bOpen = FALSE;
}

void CDXAudio::SetResourceHandle(HMODULE hResource)
{
	m_hResource = hResource;
}

BOOL CDXAudio::GetSegFromRes(HRSRC hSrc, IDirectMusicSegment8 **pSeg, BOOL bMidi)
{
	if (!s_bOpen || pSeg == NULL || bMidi) {
		return FALSE;
	}
	HGLOBAL hRes = LoadResource(m_hResource, hSrc);
	LPVOID pData = LockResource(hRes);
	DWORD dwSize = SizeofResource(m_hResource, hSrc);
	if (pData == NULL || dwSize == 0) {
		return FALSE;
	}
	/* WAVE を PCM に展開する（音声デバイスは開かないので SDL の読み込みだけ使う） */
	SDL_AudioSpec spec;
	Uint8 *pBuf = NULL;
	Uint32 dwLen = 0;
	if (SDL_LoadWAV_RW(SDL_RWFromConstMem(pData, (int)dwSize), 1, &spec, &pBuf, &dwLen) == NULL) {
		printf("[audio] 効果音を読み込めませんでした: %s\n", SDL_GetError());
		return FALSE;
	}
	int nChannels = (spec.channels > 0) ? spec.channels : 1;
	int nBytes = SDL_AUDIO_BITSIZE(spec.format) / 8;
	if (nBytes <= 0) {
		SDL_FreeWAV(pBuf);
		return FALSE;
	}
	int nSamples = (int)(dwLen / nBytes);
	int nFrames = nSamples / nChannels;
	float *pFloat = new float[(nSamples > 0) ? nSamples : 1];
	for (int i = 0; i < nSamples; i ++) {
		const Uint8 *p = pBuf + i * nBytes;
		float f = 0.0f;
		switch (spec.format) {
		case AUDIO_U8:		f = ((int)p[0] - 128) / 128.0f;					break;
		case AUDIO_S8:		f = (Sint8)p[0] / 128.0f;						break;
		case AUDIO_S16LSB:	f = (Sint16)(p[0] | (p[1] << 8)) / 32768.0f;	break;
		case AUDIO_S16MSB:	f = (Sint16)(p[1] | (p[0] << 8)) / 32768.0f;	break;
		case AUDIO_S32LSB:	f = (float)((Sint32)(p[0] | (p[1] << 8) | (p[2] << 16) | ((Uint32)p[3] << 24)) / 2147483648.0);	break;
		case AUDIO_F32LSB:	SDL_memcpy(&f, p, 4);							break;
		default:			break;
		}
		pFloat[i] = f;
	}
	SDL_FreeWAV(pBuf);

	int nId = s_nNextId ++;
	int nRet = SboSeCreate(nId, pFloat, nFrames, nChannels, spec.freq);
	delete [] pFloat;
	if (!nRet) {
		return FALSE;
	}
	*pSeg = (IDirectMusicSegment8 *)(intptr_t)nId;
	return TRUE;
}

void CDXAudio::ReleaseSeg(IDirectMusicSegment8 **pSeg)
{
	int nId = ToId(pSeg);
	if (nId == 0) {
		return;
	}
	SboSeStopId(nId);
	SboSeDelete(nId);
	s_mapRepeat.erase(nId);
	*pSeg = NULL;
}

static int GetLoops(int nId)
{
	std::map<int, int>::iterator it = s_mapRepeat.find(nId);
	return (it == s_mapRepeat.end()) ? 0 : it->second;
}

BOOL CDXAudio::PlayPrimary(IDirectMusicSegment8 **pSeg)
{
	int nId = ToId(pSeg);
	if (!s_bOpen || nId == 0) {
		return FALSE;
	}
	return SboSePlay(nId, GetLoops(nId), 1, 0) ? TRUE : FALSE;
}

BOOL CDXAudio::PlaySecoundary(IDirectMusicSegment8 **pSeg, DWORD dwDelayMs)
{
	int nId = ToId(pSeg);
	if (!s_bOpen || nId == 0) {
		return FALSE;
	}
	/* 同じ音を重ねて鳴らせる */
	return SboSePlay(nId, GetLoops(nId), 0, (int)dwDelayMs) ? TRUE : FALSE;
}

void CDXAudio::SetVolPrimary(long lVol)
{
	if (!s_bOpen) {
		return;
	}
	lVol = max(lVol, -96L);
	lVol = min(lVol, 0L);
	SboSeSetVolume(1, DbToGain(lVol));
}

void CDXAudio::SetVolSecoundary(long lVol)
{
	if (!s_bOpen) {
		return;
	}
	lVol = max(lVol, -100L);
	lVol = min(lVol, 10L);
	SboSeSetVolume(0, DbToGain(lVol));
}

void CDXAudio::Stop(IDirectMusicSegment8 **pSeg, DWORD dwFlg)
{
	if (!s_bOpen) {
		return;
	}
	if (dwFlg == 1) {
		SboSeStopPrimary();
	}
	int nId = ToId(pSeg);
	if (nId == 0) {
		if (pSeg == NULL) {
			SboSeStopAll();
		}
		return;
	}
	SboSeStopId(nId);
}

void CDXAudio::SetLoopPoints(IDirectMusicSegment8 **pSeg, DWORD dwFlg)
{
	int nId = ToId(pSeg);
	if (nId == 0) {
		return;
	}
	/* DirectMusic の繰り返し回数 → ループ回数（-1 は無限） */
	s_mapRepeat[nId] = (dwFlg == DMUS_SEG_REPEAT_INFINITE) ? -1 : (int)dwFlg;
}

BOOL CDXAudio::IsPlaying(IDirectMusicSegment8 **pSeg)
{
	int nId = ToId(pSeg);
	if (!s_bOpen || nId == 0) {
		return FALSE;
	}
	return SboSeIsPlaying(nId) ? TRUE : FALSE;
}
