/* ========================================================================= */
/* LibMusicLoader_web.cpp													 */
/* AflMusic ライブラリ読み込みクラス（Web版）								 */
/* ========================================================================= */
/*
 * BGM は MIDI を LibMusic.dll で鳴らしていたが、ブラウザでは使えないので、
 * ページ側の JS（web/bgm/sbo_bgm.js の window.SboBgm）に MIDI データを渡して鳴らす。
 * シンセは spessasynth_lib、音色はサウンドフォント GeneralUser GS。
 */
#include "StdAfx.h"

#include <emscripten.h>

/* ページ側の BGM プレイヤーを呼ぶ（読み込みに失敗していたら何もしない） */
EM_JS(int, SboBgmOpen, (const void *pData, int nSize), {
	if (!window.SboBgm) {
		return 0;
	}
	return window.SboBgm.open(HEAPU8.subarray(pData, pData + nSize)) ? 1 : 0;
});
/* 音声ファイル(.m4a)の BGM を開く（名前は bgm/audio/ の中のファイル名） */
EM_JS(int, SboBgmOpenAudio, (const char *pszName), {
	if (!window.SboBgm || !window.SboBgm.openAudio) {
		return 0;
	}
	return window.SboBgm.openAudio(UTF8ToString(pszName)) ? 1 : 0;
});
EM_JS(void, SboBgmClose, (void), {
	window.SboBgm && window.SboBgm.close();
});
EM_JS(int, SboBgmPlay, (void), {
	return (window.SboBgm && window.SboBgm.play()) ? 1 : 0;
});
EM_JS(void, SboBgmStop, (void), {
	window.SboBgm && window.SboBgm.stop();
});
EM_JS(void, SboBgmSetLoop, (int bLoop), {
	window.SboBgm && window.SboBgm.setLoop(bLoop != 0);
});
EM_JS(void, SboBgmSetVolume, (double fVol), {
	window.SboBgm && window.SboBgm.setVolume(fVol);
});
EM_JS(int, SboBgmIsPlay, (void), {
	return (window.SboBgm && window.SboBgm.isPlaying()) ? 1 : 0;
});

CLibMusicLoader::CLibMusicLoader()
{
	m_hLib = NULL;
	m_pOpen1 = NULL;
	m_pOpen2 = NULL;
	m_pClose = NULL;
	m_pFadeIn = NULL;
	m_pFadeOut = NULL;
	m_pPlayTime = NULL;
	m_pPlay1 = NULL;
	m_pPlay2 = NULL;
	m_pCont = NULL;
	m_pStop = NULL;
	m_pIsPlay = NULL;
	m_pSetLoop = NULL;
	m_pSetRelativeVolume = NULL;
	m_pGetTitle = NULL;
	m_pGetAllTime = NULL;
	m_pGetPlayTime = NULL;
}

CLibMusicLoader::~CLibMusicLoader()
{
}

void CLibMusicLoader::Load(void)
{
}

void CLibMusicLoader::Free(void)
{
	SboBgmClose();
}

BOOL CLibMusicLoader::Open(LPCSTR, LPCSTR)
{
	/* ファイルからは使っていない */
	return FALSE;
}

BOOL CLibMusicLoader::Open(HMODULE hResource, HRSRC hSrc, LPCSTR)
{
	if (hSrc == NULL) {
		return FALSE;
	}
	HGLOBAL hRes = LoadResource(hResource, hSrc);
	LPVOID pData = LockResource(hRes);
	DWORD dwSize = SizeofResource(hResource, hSrc);
	if (pData == NULL || dwSize == 0) {
		return FALSE;
	}
	return SboBgmOpen(pData, (int)dwSize) ? TRUE : FALSE;
}

BOOL CLibMusicLoader::OpenAudio(LPCSTR pszFile)
{
	if (pszFile == NULL || pszFile[0] == 0) {
		return FALSE;
	}
	return SboBgmOpenAudio(pszFile) ? TRUE : FALSE;
}

BOOL CLibMusicLoader::Close(void)
{
	SboBgmClose();
	return TRUE;
}

/* フェードは使われていないので、すぐに鳴らす・止める */
BOOL CLibMusicLoader::FadeIn(INT, INT)
{
	return Play();
}

BOOL CLibMusicLoader::FadeOut(INT)
{
	return Stop();
}

/* 途中からの演奏は使われていないので、頭から鳴らす */
BOOL CLibMusicLoader::PlayTime(DWORD)
{
	return Play();
}

BOOL CLibMusicLoader::Play(DWORDLONG)
{
	return Play();
}

BOOL CLibMusicLoader::Play(void)
{
	return SboBgmPlay() ? TRUE : FALSE;
}

BOOL CLibMusicLoader::Cont(void)
{
	return Play();
}

BOOL CLibMusicLoader::Stop(void)
{
	SboBgmStop();
	return TRUE;
}

BOOL CLibMusicLoader::IsPlay(void)
{
	return SboBgmIsPlay() ? TRUE : FALSE;
}

void CLibMusicLoader::SetLoop(BOOL bLoop)
{
	SboBgmSetLoop(bLoop);
}

/*
 * 音量は anVolTblMIDI の値（-127 で無音、0 で標準、+ で大きく）で来るので、
 * 100 を標準とした割合にする
 */
void CLibMusicLoader::SetRelativeVolume(INT nVolume)
{
	double fVol = (100.0 + nVolume) / 100.0;
	if (fVol < 0.0) {
		fVol = 0.0;
	}
	SboBgmSetVolume(fVol);
}

LPCSTR CLibMusicLoader::GetTitle(void)
{
	return "";
}

DWORDLONG CLibMusicLoader::GetAllTime(void)
{
	return 0;
}

DWORDLONG CLibMusicLoader::GetPlayTime(void)
{
	return 0;
}
