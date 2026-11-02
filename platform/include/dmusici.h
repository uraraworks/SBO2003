/* Web版: DirectMusic の代わり。型名だけ用意する（再生は CDXAudio の Web 実装が SDL_mixer で行う） */
#pragma once
#include "win32compat.h"
struct IDirectMusicSegment8;
struct IDirectMusicPerformance8;
struct IDirectMusicAudioPath8;
struct IDirectMusicLoader8;
#define DMUS_SEG_REPEAT_INFINITE	0xFFFFFFFF
