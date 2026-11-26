/* ========================================================================= */
/* ファイル：	MazeTable.h													 */
/* 概要：		迷路対戦の台の台詞 "@k..." の読み方・盤の置き場所			 */
/*				（サーバーとクライアントで同じ読み方をする。通信・描画は含まない）	 */
/* 日付：		2026/11/26													 */
/* ========================================================================= */
/*
 * 台詞:  @k<dx>,<dy>,<sdx>,<sdy>   （後ろは省略でき、空欄も既定になる。落ち物の @p と同じ流儀）
 *   盤は 13×13 タイル（1 タイル=32 ドット）。マスは偶数タイル（0,2,..,12）、壁の通り道は奇数タイル。
 *   横壁 (x, y) はタイル行 2y+1・タイル列 2x〜2x+2 の 3 タイル、縦壁は縦に 3 タイルを占める。
 *   dx, dy … 盤の左上が、台NPCのマスの左上から何タイルか（負は左・上）。
 *            既定は -6, -12（盤の中央列が席の列と同じ・盤の一番下のタイル行が台NPCのマスと重なる）
 *   sdx, sdy … 座る場所：台NPCのマスからの相対位置（既定 0, +1＝台NPCの 1 つ下）
 *   例  @k            … 全部既定（"@q" は依頼NPCの印なので使わない）
 *       @k-6,-12,0,1  … 上と同じ
 *       @k,,3,0       … 座る場所を台NPCの右 3 マスにする
 */

#pragma once

#define MAZE_TABLE_TILES		(13)		/* 盤の一辺（タイル数） */
#define MAZE_TABLE_TILE_DOT		(32)		/* 1 タイルのドット数 */
#define MAZE_PARAM_NONE			(-99999)	/* 省略・空欄 */
#define MAZE_PARAM_COUNT		(4)			/* 台詞のパラメータの数 */
#define MAZE_DEFAULT_DX			(-6)		/* 盤の左上の既定（台NPCのマスの左上から。タイル） */
#define MAZE_DEFAULT_DY			(-12)
#define MAZE_SEAT_DEFAULT_DX	(0)			/* 座る場所の既定（台NPCのマスから） */
#define MAZE_SEAT_DEFAULT_DY	(1)
#define MAZE_PARAM_LIMIT		(60)		/* 4 つの値の範囲（±） */

/* 台詞（"@k" で始まる）を読み、anVal[MAZE_PARAM_COUNT] に入れる（空欄・足りない欄は MAZE_PARAM_NONE） */
static inline void MazeParseSpeech(const char *pszSpeech, int anVal[MAZE_PARAM_COUNT])
{
	const char *p = pszSpeech + 2;
	int i;

	for (i = 0; i < MAZE_PARAM_COUNT; i ++) {
		anVal[i] = MAZE_PARAM_NONE;
	}
	for (i = 0; i < MAZE_PARAM_COUNT; i ++) {
		bool bMinus = false, bNum = false;
		int n = 0;

		if (*p == '-') {
			bMinus = true;
			p ++;
		}
		while ((*p >= '0') && (*p <= '9')) {
			if (n < 100000) {
				n = n * 10 + (*p - '0');
			}
			bNum = true;
			p ++;
		}
		if (bNum) {
			anVal[i] = bMinus ? -n : n;
		}
		if (*p != ',') {
			break;
		}
		p ++;
	}
}

/* 台詞から、盤の左上（dx,dy）と座る場所（sdx,sdy）を読む（既定と範囲の丸めを含む） */
static inline void MazeParseTable(const char *pszSpeech, int *pnDx, int *pnDy, int *pnSdx, int *pnSdy)
{
	int anVal[MAZE_PARAM_COUNT], i;
	int anDef[MAZE_PARAM_COUNT] = { MAZE_DEFAULT_DX, MAZE_DEFAULT_DY, MAZE_SEAT_DEFAULT_DX, MAZE_SEAT_DEFAULT_DY };
	int *apOut[MAZE_PARAM_COUNT] = { pnDx, pnDy, pnSdx, pnSdy };

	MazeParseSpeech (pszSpeech, anVal);
	for (i = 0; i < MAZE_PARAM_COUNT; i ++) {
		int n = (anVal[i] == MAZE_PARAM_NONE) ? anDef[i] : anVal[i];
		*apOut[i] = (n < -MAZE_PARAM_LIMIT) ? -MAZE_PARAM_LIMIT : (n > MAZE_PARAM_LIMIT) ? MAZE_PARAM_LIMIT : n;
	}
}

/* 盤の左上（マップのドット座標）。台NPCのマス (nTileX, nTileY) と dx, dy から */
static inline void MazeBoardPos(int nTileX, int nTileY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nTileX + nDx) * MAZE_TABLE_TILE_DOT;
	*pnY = (nTileY + nDy) * MAZE_TABLE_TILE_DOT;
}
