/* ========================================================================= */
/* ファイル名：	FallingPuzzle.h												 */
/* 概要：		落ち物パズル（テトリス型）の盤面ロジック（サーバーが使う。通信・時計を知らない） */
/*				盤面・7種の落ち物・回転・衝突・ライン消し・得点・落下の速さだけ。	 */
/*				単体テスト: tests/server/falling_puzzle_test.cpp				 */
/* 日付：		2026/11/24													 */
/* ========================================================================= */

#pragma once

#include <string.h>

/* ========================================================================= */
/* 調整しやすい定数（遊びながら変える）										 */
/* ========================================================================= */
#define PUZZLE_COLS			(10)		/* 盤面の横（ブロック数） */
#define PUZZLE_ROWS			(14)		/* 盤面の縦（ブロック数。地図7マス分。台ごとに 18 などへ変えられる） */
#define PUZZLE_MAX_COLS		(16)		/* 通信で運べる盤面の横の上限 */
#define PUZZLE_MAX_ROWS		(24)		/* 通信で運べる盤面の縦の上限 */
#define PUZZLE_KINDS		(7)			/* 落ち物の種類（I O T S Z J L） */
#define PUZZLE_DROP_MS_BASE	(1000)		/* レベル 0 の落下間隔(ms)。1 秒に 1 段 */
#define PUZZLE_DROP_MS_STEP	(80)		/* レベルが 1 上がるごとに短くなる量(ms) */
#define PUZZLE_DROP_MS_MIN	(100)		/* 落下間隔の下限(ms) */
#define PUZZLE_LINES_PER_LEVEL	(10)	/* このライン数ごとにレベルが上がる */
#define PUZZLE_SOFTDROP_SCORE	(1)		/* 下キーで 1 段落とすごとの得点 */

/* 対戦（台を 2 台ペアにした時）の調整用定数 */
#define PUZZLE_VS_COUNTDOWN_SEC	(3)		/* 対戦開始前のカウントダウン(秒) */
#define PUZZLE_PENDING_MAX		(20)	/* 待ちお邪魔（自分宛てでまだ積まれていない段数）の上限。これ以上は捨てる */
#define PUZZLE_GARBAGE_APPLY_MAX	(8)	/* ブロックが固定された時に、いっぺんに積まれるお邪魔の段数の上限（残りは次の固定へ持ち越し） */
#define PUZZLE_VIEW_MAX			(16)	/* クライアントが同時に覚える盤面の数（自分・相手・近くの台。待機中の台も数える） */
#define PUZZLE_CELL_GARBAGE		(PUZZLE_KINDS + 1)	/* お邪魔ブロックのセルの値（8。灰色で描く） */

/* フィールドの置き場所（席の台NPCからの相対位置。台ごとに変えられる）
   フィールドの左上は、台のマスの左上から (dx, dy) マス（負なら左・上）。縦横の段数も台ごとに指定できる。
   dx, dy を省略した時（既定）は、席がフィールドの真下：フィールドの一番下のマスの行が席の 1 マス上で、
   横はフィールドの中央が席のマスの中央に来る（盤面 10×14 なら 横 5 マス×縦 7 マスで dx=-2, dy=-6）。
   縦は、フィールドの一番下のマス行が台NPC のマスと重なる（実プレイで dy=-7 だと上 2 段が画面の上にはみ出したため 1 つ下げた） */
#define PUZZLE_BLOCK_DOT		(16)	/* ブロック 1 つの大きさ(ドット) */
#define PUZZLE_TILE_DOT			(32)	/* 地図 1 マスの大きさ(ドット) */
#define PUZZLE_DEFAULT_SIDE		(0)		/* 横の帯（次の落ち物・得点）をフィールドのどちら側に出すか（0=右 1=左。PUZZLESIDE_*） */

/* ライン消しの得点（1〜4 列。レベルが上がっても同じ） */
static const int g_anPuzzleLineScore[5] = { 0, 100, 300, 500, 800 };

/* 対戦で相手に送るお邪魔の段数（1〜4 列。1 列は送らない） */
static const int g_anPuzzleGarbage[5] = { 0, 0, 1, 2, 4 };
static inline int PuzzleGarbageLines(int nCleared)
{
	return (nCleared <= 0) ? 0 : g_anPuzzleGarbage[(nCleared > 4) ? 4 : nCleared];
}

/* ========================================================================= */
/* 落ち物の形																 */
/* ========================================================================= */
/* 基本の形（回転 0）。箱の大きさと、4 つのブロックの (x, y)。I は 4×4・O は 2×2・ほかは 3×3 */
struct PuzzleShape {
	int nSize;
	int anCell[4][2];
};
static const PuzzleShape g_aPuzzleShape[PUZZLE_KINDS] = {
	{ 4, { {0, 1}, {1, 1}, {2, 1}, {3, 1} } },		/* 0 I */
	{ 2, { {0, 0}, {1, 0}, {0, 1}, {1, 1} } },		/* 1 O */
	{ 3, { {1, 0}, {0, 1}, {1, 1}, {2, 1} } },		/* 2 T */
	{ 3, { {1, 0}, {2, 0}, {0, 1}, {1, 1} } },		/* 3 S */
	{ 3, { {0, 0}, {1, 0}, {1, 1}, {2, 1} } },		/* 4 Z */
	{ 3, { {0, 0}, {0, 1}, {1, 1}, {2, 1} } },		/* 5 J */
	{ 3, { {2, 0}, {0, 1}, {1, 1}, {2, 1} } },		/* 6 L */
};

/* 落ち物 kind を右に nRot 回（0〜3）回した 4 つのブロックの箱の中の位置を返す */
static inline void PuzzleCells(int nKind, int nRot, int anOut[4][2])
{
	const PuzzleShape &s = g_aPuzzleShape[nKind];
	int i, r;

	for (i = 0; i < 4; i ++) {
		int x = s.anCell[i][0], y = s.anCell[i][1];
		for (r = 0; r < (nRot & 3); r ++) {
			int nx = s.nSize - 1 - y;
			int ny = x;
			x = nx;
			y = ny;
		}
		anOut[i][0] = x;
		anOut[i][1] = y;
	}
}

/* ========================================================================= */
/* 乱数（同じ種から同じ順番が出る。対戦で参加者に同じ順番で出すため、盤面と別に持つ） */
/* ========================================================================= */
struct PuzzleRand {
	unsigned int dwState;

	void Seed(unsigned int dwSeed) { dwState = dwSeed ? dwSeed : 0x9E3779B9u; }
	unsigned int Next(void)
	{
		/* xorshift32 */
		dwState ^= dwState << 13;
		dwState ^= dwState >> 17;
		dwState ^= dwState << 5;
		return dwState;
	}
};

/* ========================================================================= */
/* 盤面（セルは 0=空 1〜7=落ち物の種類+1＝色）								 */
/* ========================================================================= */
struct PuzzleBoard {
	int nCols;
	int nRows;
	unsigned char abyCell[PUZZLE_MAX_COLS * PUZZLE_MAX_ROWS];

	void Init(int nC, int nR)
	{
		nCols = (nC < 4) ? 4 : (nC > PUZZLE_MAX_COLS) ? PUZZLE_MAX_COLS : nC;
		nRows = (nR < 4) ? 4 : (nR > PUZZLE_MAX_ROWS) ? PUZZLE_MAX_ROWS : nR;
		memset (abyCell, 0, sizeof (abyCell));
	}
	int Get(int x, int y) const
	{
		/* 左右と下の外は壁（埋まっている）。上の外は空き */
		if ((x < 0) || (x >= nCols) || (y >= nRows)) {
			return 1;
		}
		if (y < 0) {
			return 0;
		}
		return abyCell[x + y * nCols];
	}
	void Set(int x, int y, int nVal)
	{
		if ((x >= 0) && (x < nCols) && (y >= 0) && (y < nRows)) {
			abyCell[x + y * nCols] = (unsigned char)nVal;
		}
	}

	/* 落ち物（左上が (x, y)）が壁・積んだブロックに重なるか */
	bool Collide(int nKind, int nRot, int x, int y) const
	{
		int anC[4][2], i;
		PuzzleCells (nKind, nRot, anC);
		for (i = 0; i < 4; i ++) {
			if (Get (x + anC[i][0], y + anC[i][1])) {
				return true;
			}
		}
		return false;
	}

	/* 落ち物を盤面に固定する（盤面の上の外にはみ出した分は捨てる） */
	void Lock(int nKind, int nRot, int x, int y)
	{
		int anC[4][2], i;
		PuzzleCells (nKind, nRot, anC);
		for (i = 0; i < 4; i ++) {
			Set (x + anC[i][0], y + anC[i][1], nKind + 1);
		}
	}

	/* 下から nLines 段のお邪魔ブロックを積み上げる（盤面全体が上へ nLines 段ずれる）。
	   穴は 1 段ごとに 1 か所でランダム（rnd で決める）。上にはみ出して消えるブロックがあれば false（負け） */
	bool AddGarbage(int nLines, PuzzleRand &rnd)
	{
		int y, x, nHole;
		bool bOk = true;

		if (nLines <= 0) {
			return true;
		}
		if (nLines > nRows) {
			nLines = nRows;
		}
		for (y = 0; y < nLines && bOk; y ++) {
			for (x = 0; x < nCols; x ++) {
				if (abyCell[x + y * nCols]) {
					bOk = false;
					break;
				}
			}
		}
		memmove (abyCell, abyCell + nLines * nCols, (nRows - nLines) * nCols);
		for (y = nRows - nLines; y < nRows; y ++) {
			nHole = (int)(rnd.Next () % (unsigned int)nCols);
			for (x = 0; x < nCols; x ++) {
				abyCell[x + y * nCols] = (x == nHole) ? 0 : (unsigned char)PUZZLE_CELL_GARBAGE;
			}
		}
		return bOk;
	}

	/* 横一列がそろった行を消して、上を詰める。消した列数を返す */
	int ClearLines(void)
	{
		int y, x, nSrc, nDst, nCleared = 0;

		nDst = nRows - 1;
		for (nSrc = nRows - 1; nSrc >= 0; nSrc --) {
			bool bFull = true;
			for (x = 0; x < nCols; x ++) {
				if (abyCell[x + nSrc * nCols] == 0) {
					bFull = false;
					break;
				}
			}
			if (bFull) {
				nCleared ++;
				continue;
			}
			if (nDst != nSrc) {
				memcpy (&abyCell[nDst * nCols], &abyCell[nSrc * nCols], nCols);
			}
			nDst --;
		}
		for (y = nDst; y >= 0; y --) {
			memset (&abyCell[y * nCols], 0, nCols);
		}
		return nCleared;
	}
};

/* ========================================================================= */
/* 1人ぶんのゲーム															 */
/* ========================================================================= */
/* 周期処理・入力の結果として、何が変わったか（送る内容を決めるのに使う） */
#define PUZZLE_CHG_BOARD	(0x01)		/* 盤面（固定・ライン消し）が変わった → 盤面全体を送る */
#define PUZZLE_CHG_PIECE	(0x02)		/* 操作中の落ち物の位置・向きが変わった → 位置だけ送る */
#define PUZZLE_CHG_NEXT		(0x04)		/* 次の落ち物が変わった（盤面と一緒に送る） */
#define PUZZLE_CHG_SCORE	(0x08)		/* 得点・ライン・レベルが変わった（盤面と一緒に送る） */
#define PUZZLE_CHG_OVER		(0x10)		/* ゲームオーバーになった */
#define PUZZLE_CHG_GARBAGE	(0x20)		/* 待ちお邪魔の段数が変わった（盤面と一緒に送る） */

struct PuzzleGame {
	PuzzleBoard	Board;
	PuzzleRand	Rand;
	int			nKind;						/* 操作中の落ち物 */
	int			nRot;
	int			nX;							/* 操作中の落ち物の箱の左上（盤面のブロック座標。上にはみ出すと負） */
	int			nY;
	int			nNext;						/* 次の落ち物 */
	int			nBagPos;					/* 7 個入りの袋（7 種を 1 回ずつ出す）の何個目か */
	int			anBag[PUZZLE_KINDS];
	unsigned int dwScore;
	int			nLines;
	int			nLevel;
	bool		bOver;
	unsigned int dwLastDrop;				/* 最後に 1 段落ちた時刻(ms) */
	PuzzleRand	GRand;						/* お邪魔の穴の位置を決める乱数（落ち物の順番とは別。落ち物は参加者で同じ順番にするため） */
	int			nPending;					/* 待ちお邪魔（自分宛てでまだ積まれていない段数） */
	int			nAttack;					/* 直前の固定で相手へ送る段数（相殺した残り。呼び出し側が読んで 0 に戻す） */

	/* 袋から次の種類を取り出す（袋が空なら詰め直してかき混ぜる） */
	int BagNext(void)
	{
		if (nBagPos >= PUZZLE_KINDS) {
			int i;
			for (i = 0; i < PUZZLE_KINDS; i ++) {
				anBag[i] = i;
			}
			for (i = PUZZLE_KINDS - 1; i > 0; i --) {
				int j = (int)(Rand.Next () % (unsigned int)(i + 1));
				int t = anBag[i];
				anBag[i] = anBag[j];
				anBag[j] = t;
			}
			nBagPos = 0;
		}
		return anBag[nBagPos ++];
	}

	/* 始める（nCols×nRows の空の盤面・種 dwSeed・時刻 dwNow）。最初の落ち物まで出す */
	void Start(int nCols, int nRows, unsigned int dwSeed, unsigned int dwNow)
	{
		Board.Init (nCols, nRows);
		Rand.Seed (dwSeed);
		nBagPos = PUZZLE_KINDS;
		dwScore = 0;
		nLines = 0;
		nLevel = 0;
		bOver = false;
		GRand.Seed (dwSeed ^ 0x5BD1E995u);
		nPending = 0;
		nAttack = 0;
		nNext = BagNext ();
		dwLastDrop = dwNow;
		Spawn ();
	}

	/* 次の落ち物を出す。置けなければゲームオーバー */
	void Spawn(void)
	{
		int anC[4][2], i, nMinY = 99;

		nKind = nNext;
		nNext = BagNext ();
		nRot = 0;
		PuzzleCells (nKind, 0, anC);
		for (i = 0; i < 4; i ++) {
			if (anC[i][1] < nMinY) {
				nMinY = anC[i][1];
			}
		}
		nX = (Board.nCols - g_aPuzzleShape[nKind].nSize + 1) / 2;
		nY = -nMinY;		/* いちばん上のブロックが盤面の 1 行目に来る */
		if (Board.Collide (nKind, nRot, nX, nY)) {
			bOver = true;
		}
	}

	/* 今のレベルの落下間隔(ms) */
	static int DropMs(int nLv)
	{
		int n = PUZZLE_DROP_MS_BASE - nLv * PUZZLE_DROP_MS_STEP;
		return (n < PUZZLE_DROP_MS_MIN) ? PUZZLE_DROP_MS_MIN : n;
	}

	/* 左右に動かす（dx=-1/+1）。動けたら PUZZLE_CHG_PIECE */
	int Move(int dx)
	{
		if (bOver || Board.Collide (nKind, nRot, nX + dx, nY)) {
			return 0;
		}
		nX += dx;
		return PUZZLE_CHG_PIECE;
	}

	/* 右に回す。壁際は左右にずらして入れる（壁けり）。回せたら PUZZLE_CHG_PIECE */
	int Rotate(void)
	{
		static const int anKick[5] = { 0, -1, 1, -2, 2 };
		int nNewRot = (nRot + 1) & 3, i;

		if (bOver || (nKind == 1)) {		/* O は回しても同じ */
			return 0;
		}
		for (i = 0; i < 5; i ++) {
			if (!Board.Collide (nKind, nNewRot, nX + anKick[i], nY)) {
				nRot = nNewRot;
				nX += anKick[i];
				return PUZZLE_CHG_PIECE;
			}
		}
		return 0;
	}

	/* 相手から送られたお邪魔 nLines 段を待ちに足す（上限あり）。変わったら PUZZLE_CHG_GARBAGE */
	int AddPending(int nLines)
	{
		int nOld = nPending;

		nPending += nLines;
		if (nPending > PUZZLE_PENDING_MAX) {
			nPending = PUZZLE_PENDING_MAX;
		}
		return (nPending != nOld) ? PUZZLE_CHG_GARBAGE : 0;
	}

	/* 固定して、ライン消し・得点・お邪魔の相殺と反映・次の落ち物まで進める。
	   消した列数に応じたお邪魔は、まず自分宛ての待ちを相殺し、残りが nAttack（相手へ送る分）になる。
	   相殺しても残った待ちは、この固定で下から積まれる（1 回に PUZZLE_GARBAGE_APPLY_MAX 段まで） */
	int LockAndNext(void)
	{
		int nRet = PUZZLE_CHG_BOARD | PUZZLE_CHG_NEXT | PUZZLE_CHG_PIECE, nCleared, nSend;
		bool bOverflow = false;

		nAttack = 0;
		Board.Lock (nKind, nRot, nX, nY);
		nCleared = Board.ClearLines ();
		if (nCleared > 0) {
			dwScore += (unsigned int)g_anPuzzleLineScore[(nCleared > 4) ? 4 : nCleared];
			nLines += nCleared;
			nLevel = nLines / PUZZLE_LINES_PER_LEVEL;
			nRet |= PUZZLE_CHG_SCORE;
			nSend = PuzzleGarbageLines (nCleared);
			if (nSend > 0 && nPending > 0) {
				int nCancel = (nSend < nPending) ? nSend : nPending;
				nPending -= nCancel;
				nSend -= nCancel;
				nRet |= PUZZLE_CHG_GARBAGE;
			}
			nAttack = nSend;
		}
		if (nPending > 0) {
			int nApply = (nPending < PUZZLE_GARBAGE_APPLY_MAX) ? nPending : PUZZLE_GARBAGE_APPLY_MAX;
			nPending -= nApply;
			bOverflow = !Board.AddGarbage (nApply, GRand);
			nRet |= PUZZLE_CHG_GARBAGE;
		}
		Spawn ();
		if (bOverflow) {
			bOver = true;
		}
		if (bOver) {
			nRet |= PUZZLE_CHG_OVER;
		}
		return nRet;
	}

	/* 1 段落とす（下キー・時間経過）。下が空いていれば動く。詰まっていれば固定。bSoft なら得点が付く */
	int Step(bool bSoft, unsigned int dwNow)
	{
		if (bOver) {
			return 0;
		}
		dwLastDrop = dwNow;
		if (!Board.Collide (nKind, nRot, nX, nY + 1)) {
			nY ++;
			if (bSoft) {
				dwScore += PUZZLE_SOFTDROP_SCORE;
				return PUZZLE_CHG_PIECE | PUZZLE_CHG_SCORE;
			}
			return PUZZLE_CHG_PIECE;
		}
		return LockAndNext ();
	}

	/* 周期処理。前に落ちてから間隔が過ぎた分だけ落とす（呼ばれる間隔に左右されない。1 回の処理で落とすのは最大 nMaxSteps 段） */
	int Tick(unsigned int dwNow, int nMaxSteps = 4)
	{
		int nRet = 0, n = 0;

		while (!bOver && (n < nMaxSteps) && ((unsigned int)(dwNow - dwLastDrop) >= (unsigned int)DropMs (nLevel))) {
			unsigned int dwBase = dwLastDrop + (unsigned int)DropMs (nLevel);
			nRet |= Step (false, dwBase);		/* 経過時間で進める（遅れた分は次の段へ持ち越す） */
			n ++;
		}
		/* 遅れが大きすぎる時（止まっていた後など）は追いつかせず、いま落ちたことにする */
		if (!bOver && ((unsigned int)(dwNow - dwLastDrop) >= (unsigned int)DropMs (nLevel) * 4)) {
			dwLastDrop = dwNow;
		}
		return nRet;
	}
};

/* ========================================================================= */
/* フィールドの位置（マップのドット座標の左上）								 */
/*   dx, dy は台のマスの左上からのマス数。省略時の既定は PuzzleDefaultDx/Dy		 */
/* ========================================================================= */
static inline void PuzzleFieldPos(int nTileX, int nTileY, int nDx, int nDy, int *pnX, int *pnY)
{
	*pnX = (nTileX + nDx) * PUZZLE_TILE_DOT;
	*pnY = (nTileY + nDy) * PUZZLE_TILE_DOT;
}

/* dx の既定：フィールドの中央が席のマスの中央に来る */
static inline int PuzzleDefaultDx(int nCols)
{
	return -((nCols * PUZZLE_BLOCK_DOT / 2 - PUZZLE_TILE_DOT / 2) / PUZZLE_TILE_DOT);
}

/* dy の既定：フィールドの高さ（マス数）−1。一番下のマス行が台NPC のマスと重なる（盤面が 7 マス分の高さなら -6、18 段なら -8） */
static inline int PuzzleDefaultDy(int nRows)
{
	return -((nRows * PUZZLE_BLOCK_DOT + PUZZLE_TILE_DOT - 1) / PUZZLE_TILE_DOT - 1);
}
