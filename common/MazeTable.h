/* ========================================================================= */
/* ファイル：	MazeTable.h													 */
/* 概要：		迷路対戦の台の設定（サブゲーム情報 SUBGAMEID_MAZETABLE）の読み方・盤の置き場所	 */
/*				（サーバーとクライアントで同じ読み方をする。通信・描画は含まない）	 */
/* 日付：		2026/11/27													 */
/* ========================================================================= */
/*
 * 台NPCの台詞:  @k<サブゲーム情報の番号>      （ダンジョン受付の @d<番号> と同じ。番号は 0 から。旧書式 @k<dx>,<dy>,... は廃止）
 *   番号の指すサブゲーム情報が SUBGAMEID_MAZETABLE でない・存在しない時は、台ではないものとして無視する。
 *   設定は SUBGAME_MAZETABLEINFO（common/TypedefCommon.h。管理者が「サブゲーム情報」で設定する）:
 *   盤は 320×320 ドット（10×10 タイル分。1 タイル=32 ドット）。マスは 32 ドット（キャラの実寸）、マスの間の
 *   壁の通り道は 16 ドット（半キャラ分）で、7*32+6*16=320。マス (x, y) の左上は盤ドット (48x, 48y)。
 *   横壁 (x, y) は盤ドットで縦 48y+32〜+48・横 48x〜48x+80（2 マス＋隙間 1 つ分）、縦壁はその縦横を入れ替えた形。
 *   chDx, chDy   … 盤の左上が、台NPCのマスの左上から何タイルか（負は左・上）。
 *                  既定は -5, -9（盤の中央のマスの中心＝盤ドット(160,160)が席の列の左端に合い、盤の下端が台NPCのマスの下端と重なる）
 *   chSdx, chSdy … 席 1（プレイヤー 0＝下側）の座る場所：台NPCのマスからの相対位置（既定 0, +1＝台NPCの 1 つ下）
 *   chSdx2, chSdy2 … 席 2（プレイヤー 1＝上側）の座る場所：台NPCのマスからの相対位置（既定 0, -10＝盤の真上の 1 つ上）
 *   wFloorA, wFloorB … マスに使うマップパーツID（市松に交互。Bが 0 なら A だけ＝単色の床。両方 0 ならコード描画の床）
 *   wGap         … 壁の通り道（16 ドット幅の溝と交差点）の地に使うマップパーツID（0 ならコード描画の暗い単色）。
 *                  パーツ（32×32）の中央 16×16 を敷き詰める。パーツIDが範囲外（g_wCountMapParts 以上）なら、その指定はコード描画に戻る
 *   byCpuLevel   … 0＝CPUなし（従来のひとり練習）、1=よわい・2=ふつう・3=つよい。1 以上の台は、人がひとりで座った時に
 *                  「CPUと対戦しますか？」と聞き、はいなら空いている側の席をCPUが受け持つ
 *   wCpuGrpID    … CPUのコマに使うキャラ画像（NPC の見た目 wGrpID と同じ意味。0＝色付きの丸）
 *   szCpuName    … CPUの名前表示（空なら「CPU」）
 *   wTicketItemID, byTicketNum … 参加チケット。byTicketNum が 1 以上の台は有料で、席のマスに立ってコマンド→道具→そのチケットを使うと座る
 *                  （Sキーでは座れず案内だけ。落ち物パズルのコインと同じ流れ）。チケットは座る時（CPU・乱入の確認で「はい」を受けてから）に取り上げる。0 なら無料でSキー
 *   クライアントは台の設定を持たない。盤の位置・席の位置・CPUの見た目は、サーバーが送る CMD_MAZESTATE から読む。
 */

#pragma once

#include <string.h>		/* 使う前に TypedefCommon.h（SUBGAME_MAZETABLEINFO）を読み込んでおくこと。サーバー・クライアント・テストは include.h 経由で読み込み済み */

#define MAZE_TABLE_TILES		(10)		/* 盤の一辺（タイル数。320 ドット） */
#define MAZE_TABLE_TILE_DOT		(32)		/* 1 タイルのドット数 */
#define MAZE_CELL_DOT			(32)		/* マスの一辺（ドット。キャラの実寸） */
#define MAZE_GAP_DOT			(16)		/* マスの間の壁の通り道・壁の太さ（ドット） */
#define MAZE_PITCH_DOT			(MAZE_CELL_DOT + MAZE_GAP_DOT)	/* マスの間隔（48 ドット） */
#define MAZE_BOARD_CELLS		(7)			/* 盤の一辺のマス数（common/MazeAttack.h の MAZE_SIZE と同じ） */
#define MAZE_BOARD_DOTS			(MAZE_BOARD_CELLS * MAZE_CELL_DOT + (MAZE_BOARD_CELLS - 1) * MAZE_GAP_DOT)	/* 盤の一辺（ドット。320） */
#define MAZE_CPU_LEVEL_MAX		(3)			/* CPU強さの最大（1=よわい・2=ふつう・3=つよい。0=CPUなし） */
#define MAZE_DEFAULT_DX			(-5)		/* 盤の左上の既定（台NPCのマスの左上から。タイル） */
#define MAZE_DEFAULT_DY			(-9)
#define MAZE_SEAT_DEFAULT_DX	(0)			/* 座る場所の既定（台NPCのマスから） */
#define MAZE_SEAT_DEFAULT_DY	(1)
#define MAZE_SEAT2_DEFAULT_DX	(0)			/* 席 2 の座る場所の既定（台NPCのマスから。盤の真上の 1 つ上） */
#define MAZE_SEAT2_DEFAULT_DY	(-10)
#define MAZE_SPEECH_NO_MAX		(100000)	/* 台詞の番号の上限（これを超えたら台ではない） */
#define MAZE_TICKET_MAX			(99)		/* チケット必要枚数の上限 */
#define MAZE_PARAM_LIMIT		(60)		/* 位置の値（dx, dy, sdx, sdy, sdx2, sdy2）の範囲（±） */

/* 盤の一辺は 10 タイル分ちょうど */
typedef char MazeBoardSizeCheck[(MAZE_BOARD_DOTS == MAZE_TABLE_TILES * MAZE_TABLE_TILE_DOT) ? 1 : -1];

/* 台詞 "@k<番号>" の番号（サブゲーム情報の番号）。"@k" の後ろが数字だけの時の番号・そうでなければ -1 */
static inline int MazeSpeechNo(const char *pszSpeech)
{
	const char *p;
	int nNo = 0;

	if (!pszSpeech || (pszSpeech[0] != '@') || (pszSpeech[1] != 'k')) {
		return -1;
	}
	p = pszSpeech + 2;
	if ((*p < '0') || (*p > '9')) {
		return -1;
	}
	while ((*p >= '0') && (*p <= '9')) {
		nNo = nNo * 10 + (*p - '0');
		if (nNo > MAZE_SPEECH_NO_MAX) {
			return -1;
		}
		p ++;
	}
	return (*p == '\0') ? nNo : -1;
}

/* 台の設定を範囲内に直す（管理者の入力・保存データを受け取った時。位置は ±MAZE_PARAM_LIMIT、CPU強さは 0〜3、文字列は終端を保証） */
static inline void MazeTableInfoFix(SUBGAME_MAZETABLEINFO *p)
{
	char *apch[6] = { &p->chDx, &p->chDy, &p->chSdx, &p->chSdy, &p->chSdx2, &p->chSdy2 };
	int i;

	for (i = 0; i < 6; i ++) {
		int n = *apch[i];
		*apch[i] = (char)((n < -MAZE_PARAM_LIMIT) ? -MAZE_PARAM_LIMIT : (n > MAZE_PARAM_LIMIT) ? MAZE_PARAM_LIMIT : n);
	}
	if (p->byCpuLevel > MAZE_CPU_LEVEL_MAX) {
		p->byCpuLevel = MAZE_CPU_LEVEL_MAX;
	}
	if (p->byTicketNum > MAZE_TICKET_MAX) {
		p->byTicketNum = MAZE_TICKET_MAX;
	}
	p->szName[sizeof (p->szName) - 1] = '\0';
	p->szCpuName[sizeof (p->szCpuName) - 1] = '\0';
}

/* 新しい台の既定値（盤の位置 -5,-9・座る場所 0,1 と 0,-10・CPU名「CPU」。ほかは 0） */
static inline void MazeTableInfoDefault(SUBGAME_MAZETABLEINFO *p)
{
	memset (p, 0, sizeof (*p));
	p->chDx = MAZE_DEFAULT_DX;
	p->chDy = MAZE_DEFAULT_DY;
	p->chSdx = MAZE_SEAT_DEFAULT_DX;
	p->chSdy = MAZE_SEAT_DEFAULT_DY;
	p->chSdx2 = MAZE_SEAT2_DEFAULT_DX;
	p->chSdy2 = MAZE_SEAT2_DEFAULT_DY;
	strcpy (p->szCpuName, "CPU");
}

/* 盤の左上（マップのドット座標）。台NPCのマス (nTileX, nTileY) と dx, dy から */
static inline void MazeBoardPos(int nTileX, int nTileY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nTileX + nDx) * MAZE_TABLE_TILE_DOT;
	*pnY = (nTileY + nDy) * MAZE_TABLE_TILE_DOT;
}
