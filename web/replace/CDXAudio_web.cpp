/* ========================================================================= */
/* CDXAudio_web.cpp															 */
/* DirectX Audio を使うためのクラス（Web版）：SDL_mixer で鳴らす				 */
/* ========================================================================= */
/*
 * 効果音は WAVE リソースなので、SDL_mixer の Mix_Chunk にして鳴らす。
 * ゲーム側は IDirectMusicSegment8 * として持っているが、中身は Mix_Chunk *。
 * 音量は DirectMusic と同じく dB 単位（0 が最大、-100 で無音）で受け取って換算する。
 * プライマリ（BGM 用）はチャンネル 0 を専用に使う。
 */
#include "StdAfx.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <math.h>
#include <map>

#define PRIMARY_CHANNEL		0			/* プライマリセグメント用のチャンネル */
#define CHANNEL_COUNT		16

static BOOL						s_bOpen;
static std::map<Mix_Chunk *, int>	s_mapRepeat;	/* セグメントごとの繰り返し回数 */

static Mix_Chunk *ToChunk(IDirectMusicSegment8 **pSeg)
{
	return pSeg ? (Mix_Chunk *)*pSeg : NULL;
}

/* dB → SDL_mixer の音量(0〜128) */
static int DbToVolume(long lDb)
{
	if (lDb <= -100) {
		return 0;
	}
	double fGain = pow(10.0, (double)lDb / 20.0);
	int nVol = (int)(fGain * MIX_MAX_VOLUME + 0.5);
	if (nVol > MIX_MAX_VOLUME) {
		nVol = MIX_MAX_VOLUME;
	}
	return nVol;
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
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		printf("[audio] SDL_INIT_AUDIO に失敗しました: %s\n", SDL_GetError());
		/* 音が出なくてもゲームは続ける */
		return TRUE;
	}
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) {
		printf("[audio] Mix_OpenAudio に失敗しました: %s\n", Mix_GetError());
		return TRUE;
	}
	Mix_AllocateChannels(CHANNEL_COUNT);
	Mix_ReserveChannels(1);
	s_bOpen = TRUE;
	return TRUE;
}

void CDXAudio::Destroy(void)
{
	if (!s_bOpen) {
		return;
	}
	Mix_HaltChannel(-1);
	Mix_CloseAudio();
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
	/* リソースのデータは残り続けるので、コピーせずにそのまま読ませる */
	Mix_Chunk *pChunk = Mix_LoadWAV_RW(SDL_RWFromConstMem(pData, (int)dwSize), 1);
	if (pChunk == NULL) {
		printf("[audio] 効果音を読み込めませんでした: %s\n", Mix_GetError());
		return FALSE;
	}
	*pSeg = (IDirectMusicSegment8 *)pChunk;
	return TRUE;
}

void CDXAudio::ReleaseSeg(IDirectMusicSegment8 **pSeg)
{
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (pChunk == NULL) {
		return;
	}
	Stop(pSeg, 0);
	s_mapRepeat.erase(pChunk);
	Mix_FreeChunk(pChunk);
	*pSeg = NULL;
}

static int GetLoops(Mix_Chunk *pChunk)
{
	std::map<Mix_Chunk *, int>::iterator it = s_mapRepeat.find(pChunk);
	return (it == s_mapRepeat.end()) ? 0 : it->second;
}

BOOL CDXAudio::PlayPrimary(IDirectMusicSegment8 **pSeg)
{
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (!s_bOpen || pChunk == NULL) {
		return FALSE;
	}
	return (Mix_PlayChannel(PRIMARY_CHANNEL, pChunk, GetLoops(pChunk)) >= 0) ? TRUE : FALSE;
}

BOOL CDXAudio::PlaySecoundary(IDirectMusicSegment8 **pSeg)
{
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (!s_bOpen || pChunk == NULL) {
		return FALSE;
	}
	/* 空いているチャンネルで鳴らす（同じ音を重ねて鳴らせる） */
	return (Mix_PlayChannel(-1, pChunk, GetLoops(pChunk)) >= 0) ? TRUE : FALSE;
}

void CDXAudio::SetVolPrimary(long lVol)
{
	if (!s_bOpen) {
		return;
	}
	lVol = max(lVol, -96L);
	lVol = min(lVol, 0L);
	Mix_Volume(PRIMARY_CHANNEL, DbToVolume(lVol));
}

void CDXAudio::SetVolSecoundary(long lVol)
{
	if (!s_bOpen) {
		return;
	}
	lVol = max(lVol, -100L);
	lVol = min(lVol, 10L);
	int nVol = DbToVolume(lVol);
	for (int i = 0; i < CHANNEL_COUNT; i ++) {
		if (i != PRIMARY_CHANNEL) {
			Mix_Volume(i, nVol);
		}
	}
}

void CDXAudio::Stop(IDirectMusicSegment8 **pSeg, DWORD dwFlg)
{
	if (!s_bOpen) {
		return;
	}
	if (dwFlg == 1) {
		Mix_HaltChannel(PRIMARY_CHANNEL);
	}
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (pChunk == NULL) {
		if (pSeg == NULL) {
			Mix_HaltChannel(-1);
		}
		return;
	}
	for (int i = 0; i < CHANNEL_COUNT; i ++) {
		if (Mix_Playing(i) && Mix_GetChunk(i) == pChunk) {
			Mix_HaltChannel(i);
		}
	}
}

void CDXAudio::SetLoopPoints(IDirectMusicSegment8 **pSeg, DWORD dwFlg)
{
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (pChunk == NULL) {
		return;
	}
	/* DirectMusic の繰り返し回数 → SDL_mixer のループ回数（-1 は無限） */
	s_mapRepeat[pChunk] = (dwFlg == DMUS_SEG_REPEAT_INFINITE) ? -1 : (int)dwFlg;
}

BOOL CDXAudio::IsPlaying(IDirectMusicSegment8 **pSeg)
{
	Mix_Chunk *pChunk = ToChunk(pSeg);
	if (!s_bOpen || pChunk == NULL) {
		return FALSE;
	}
	for (int i = 0; i < CHANNEL_COUNT; i ++) {
		if (Mix_Playing(i) && Mix_GetChunk(i) == pChunk) {
			return TRUE;
		}
	}
	return FALSE;
}
