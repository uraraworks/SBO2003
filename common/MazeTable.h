/* ========================================================================= */
/* ファイル：	MazeTable.h													 */
/* 概要：		迷路対戦の台の台詞 "@k..." の読み方・盤の置き場所			 */
/*				（サーバーとクライアントで同じ読み方をする。通信・描画は含まない）	 */
/* 日付：		2026/11/26													 */
/* ========================================================================= */
/*
 * 台詞:  @k<dx>,<dy>,<sdx>,<sdy>,<床A>,<床B>,<隙間>,<sdx2>,<sdy2>   （後ろは省略でき、空欄も既定になる。落ち物の @p と同じ流儀）
 *   盤は 320×320 ドット（10×10 タイル分。1 タイル=32 ドット）。マスは 32 ドット（キャラの実寸）、マスの間の
 *   壁の通り道は 16 ドット（半キャラ分）で、7*32+6*16=320。マス (x, y) の左上は盤ドット (48x, 48y)。
 *   横壁 (x, y) は盤ドットで縦 48y+32〜+48・横 48x〜48x+80（2 マス＋隙間 1 つ分）、縦壁はその縦横を入れ替えた形。
 *   dx, dy … 盤の左上が、台NPCのマスの左上から何タイルか（負は左・上）。
 *            既定は -5, -9（盤の中央のマスの中心＝盤ドット(160,160)が席の列の左端に合い、盤の下端が台NPCのマスの下端と重なる）
 *   sdx, sdy … 席 1（プレイヤー 0＝下側）の座る場所：台NPCのマスからの相対位置（既定 0, +1＝台NPCの 1 つ下）
 *   床A, 床B … マスに使うマップパーツID（市松に交互。Bを省略（0）したらAだけ＝単色の床。両方 0・省略ならコード描画の床）
 *   sdx2, sdy2 … 席 2（プレイヤー 1＝上側）の座る場所：台NPCのマスからの相対位置（既定 0, -10＝盤の真上の 1 つ上。
 *            既定の盤（dy=-9）の上端の 1 行上）。盤の外にはみ出す場合の扱いは決めていない（台を置く側が収まる所に置く）
 *   隙間     … 壁の通り道（16 ドット幅の溝と交差点）の地に使うマップパーツID（0・省略ならコード描画の暗い単色）。
 *              パーツ（32×32）の中央 16×16 を敷き詰める。パーツIDが範囲外（g_wCountMapParts 以上）なら、その指定はコード描画に戻る
 *   例  @k            … 全部既定（"@q" は依頼NPCの印なので使わない）
 *       @k-5,-9,0,1   … 上と同じ
 *       @k,,3,0       … 座る場所を台NPCの右 3 マスにする
 *       @k,,,,5,6,7   … 床にパーツ5と6（市松）、隙間にパーツ7を使う
 *       @k,,,,,,,3,-10 … 席 2 を台NPCの右 3 マス・上 10 マスにする
 */

#pragma once

#define MAZE_TABLE_TILES		(10)		/* 盤の一辺（タイル数。320 ドット） */
#define MAZE_TABLE_TILE_DOT		(32)		/* 1 タイルのドット数 */
#define MAZE_CELL_DOT			(32)		/* マスの一辺（ドット。キャラの実寸） */
#define MAZE_GAP_DOT			(16)		/* マスの間の壁の通り道・壁の太さ（ドット） */
#define MAZE_PITCH_DOT			(MAZE_CELL_DOT + MAZE_GAP_DOT)	/* マスの間隔（48 ドット） */
#define MAZE_BOARD_CELLS		(7)			/* 盤の一辺のマス数（common/MazeAttack.h の MAZE_SIZE と同じ） */
#define MAZE_BOARD_DOTS			(MAZE_BOARD_CELLS * MAZE_CELL_DOT + (MAZE_BOARD_CELLS - 1) * MAZE_GAP_DOT)	/* 盤の一辺（ドット。320） */
#define MAZE_PARAM_NONE			(-99999)	/* 省略・空欄 */
#define MAZE_PARAM_COUNT		(9)			/* 台詞のパラメータの数（dx, dy, sdx, sdy, 床A, 床B, 隙間, sdx2, sdy2） */
#define MAZE_PARAM_SEAT2		(7)			/* そのうち席 2 の位置（sdx2, sdy2）の最初の番号 */
#define MAZE_PARAM_POS_COUNT	(4)			/* そのうち位置の 4 つ（dx, dy, sdx, sdy。範囲は ±MAZE_PARAM_LIMIT） */
#define MAZE_PART_MAX			(65535)		/* パーツIDの上限（WORD） */
#define MAZE_DEFAULT_DX			(-5)		/* 盤の左上の既定（台NPCのマスの左上から。タイル） */
#define MAZE_DEFAULT_DY			(-9)
#define MAZE_SEAT_DEFAULT_DX	(0)			/* 座る場所の既定（台NPCのマスから） */
#define MAZE_SEAT_DEFAULT_DY	(1)
#define MAZE_SEAT2_DEFAULT_DX	(0)			/* 席 2 の座る場所の既定（台NPCのマスから。盤の真上の 1 つ上） */
#define MAZE_SEAT2_DEFAULT_DY	(-10)
#define MAZE_PARAM_LIMIT		(60)		/* 位置の値（dx, dy, sdx, sdy, sdx2, sdy2）の範囲（±） */

/* 盤の一辺は 10 タイル分ちょうど */
typedef char MazeBoardSizeCheck[(MAZE_BOARD_DOTS == MAZE_TABLE_TILES * MAZE_TABLE_TILE_DOT) ? 1 : -1];

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
	for (i = 0; i < MAZE_PARAM_POS_COUNT; i ++) {
		int n = (anVal[i] == MAZE_PARAM_NONE) ? anDef[i] : anVal[i];
		*apOut[i] = (n < -MAZE_PARAM_LIMIT) ? -MAZE_PARAM_LIMIT : (n > MAZE_PARAM_LIMIT) ? MAZE_PARAM_LIMIT : n;
	}
}

/* 台詞から、席 2（プレイヤー 1＝上側）の座る場所（sdx2,sdy2。台NPCのマスからの相対。既定と範囲の丸めを含む）を読む */
static inline void MazeParseSeat2(const char *pszSpeech, int *pnSdx2, int *pnSdy2)
{
	int anVal[MAZE_PARAM_COUNT], i;
	int anDef[2] = { MAZE_SEAT2_DEFAULT_DX, MAZE_SEAT2_DEFAULT_DY };
	int *apOut[2] = { pnSdx2, pnSdy2 };

	MazeParseSpeech (pszSpeech, anVal);
	for (i = 0; i < 2; i ++) {
		int n = (anVal[MAZE_PARAM_SEAT2 + i] == MAZE_PARAM_NONE) ? anDef[i] : anVal[MAZE_PARAM_SEAT2 + i];
		*apOut[i] = (n < -MAZE_PARAM_LIMIT) ? -MAZE_PARAM_LIMIT : (n > MAZE_PARAM_LIMIT) ? MAZE_PARAM_LIMIT : n;
	}
}

/* 台詞から、見た目に使うパーツID（床A・床B・隙間）を読む。省略・空欄・負の値・範囲外の数は 0（＝コード描画）。
   上限は WORD（65535）。パーツ数を超えるIDの扱いは描く側（クライアント）がコード描画に戻す */
static inline void MazeParseParts(const char *pszSpeech, unsigned short *pwFloorA, unsigned short *pwFloorB, unsigned short *pwGap)
{
	int anVal[MAZE_PARAM_COUNT], i;
	unsigned short awOut[3];

	MazeParseSpeech (pszSpeech, anVal);
	for (i = 0; i < 3; i ++) {
		int n = anVal[MAZE_PARAM_POS_COUNT + i];
		awOut[i] = (unsigned short)((n == MAZE_PARAM_NONE || n < 0) ? 0 : (n > MAZE_PART_MAX) ? MAZE_PART_MAX : n);
	}
	*pwFloorA = awOut[0];
	*pwFloorB = awOut[1];
	*pwGap = awOut[2];
}

/* 盤の左上（マップのドット座標）。台NPCのマス (nTileX, nTileY) と dx, dy から */
static inline void MazeBoardPos(int nTileX, int nTileY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nTileX + nDx) * MAZE_TABLE_TILE_DOT;
	*pnY = (nTileY + nDy) * MAZE_TABLE_TILE_DOT;
}
