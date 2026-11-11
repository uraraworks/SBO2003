/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* 概要：		音声ファイル(.m4a)で鳴らす BGM の番号表						 */
/* ========================================================================= */
/*
 * マップの BGM 番号（MAPINFO.wBGM）は、これまで SOUND_ 列挙（効果音と共用、0〜SOUND_CHAT）の
 * 番号だった。この列挙は 1 バイトでは足りなくなったので、音声ファイルで鳴る曲には
 * BGM_AUDIO_FIRST 以降の別の番号を振る。
 *
 *   ・MIDI の曲は今まで通り（番号は SOUND_ 列挙のまま）
 *   ・管理者がマップ情報の BGM 一覧から音声の曲を選ぶと、そのマップは音声ファイルで鳴る
 *   ・音声ファイルは非公開側の SboSoundData/Audio/ に置く（ビルドで dist/bgm/audio/ へコピー）
 *
 * 曲を足す時は g_aBgmAudio の末尾に 1 行足す（番号は変えない・詰めない）。
 * TypedefCommon.h の末尾から読み込まれる（WORD などの型を使うので、直接は読み込まない）。
 */

#pragma once

#define BGM_AUDIO_FIRST		1000									/* 音声 BGM の最初の番号 */

typedef struct _BGMAUDIOINFO {
	WORD			wNo;											/* BGM 番号 */
	const char		*pszFile;										/* ファイル名（SboSoundData/Audio/ 内） */
	const char		*pszName;										/* 管理者ウィンドウに出す名前(UTF-8) */
} BGMAUDIOINFO, *PBGMAUDIOINFO;

static const BGMAUDIOINFO g_aBgmAudio[] = {
	{1000,	"town_morning.m4a",	"始まりの町（新）"},
	{1001,	"pixel_duel.m4a",	"爆弾ゲーム場（新）"},
};
#define BGM_AUDIO_COUNT		((int)(sizeof (g_aBgmAudio) / sizeof (g_aBgmAudio[0])))

/* 音声 BGM の情報を返す（音声の番号でなければ NULL） */
static inline const BGMAUDIOINFO *FindBgmAudio(DWORD dwNo)
{
	for (int i = 0; i < BGM_AUDIO_COUNT; i ++) {
		if (g_aBgmAudio[i].wNo == dwNo) {
			return &g_aBgmAudio[i];
		}
	}
	return NULL;
}

/* マップに設定してよい BGM 番号か（今までの MIDI の番号 0〜255 または 音声 BGM の表にある番号）。
 * 256（SOUND_CHAT、発言の効果音）は BGM にはならないので含めない */
#define BGM_MIDI_MAX		256										/* MIDI の BGM 番号の上限（これ未満） */
static inline BOOL IsValidBgmNo(DWORD dwNo)
{
	return (dwNo < BGM_MIDI_MAX) || (FindBgmAudio (dwNo) != NULL);
}
