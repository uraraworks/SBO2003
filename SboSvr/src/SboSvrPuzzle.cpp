/* ========================================================================= */
/* ファイル名：	SboSvrPuzzle.cpp											 */
/* 概要：		落ち物パズルの台（台NPC・席・ゲームの進行・対戦）の処理(実装ファイル) */
/* 作成日：		2026/11/24													 */
/* ========================================================================= */
/*
 * 仕様は docs/falling-puzzle.md。盤面ロジックは common/FallingPuzzle.h。
 * 台は、台詞が "@p" で始まる NPC。台詞のパラメータ（コンマ区切り。後ろは省略でき、空欄も既定になる）:
 *     @p<コインのアイテム番号>,<必要な枚数>,<dx>,<dy>,<横の段数>,<縦の段数>,<帯の側 0=右 1=左>,<ペア番号>,<sdx>,<sdy>
 *   dx, dy … フィールドの左上が、台のマスの左上から何マスか（負は左・上）。省略はフィールドの真下に席（10×14 なら -2,-6）
 *   横・縦の段数 … 省略は PUZZLE_COLS × PUZZLE_ROWS（10×14）。台ごとに 10×18 なども指定できる
 *   帯の側 … 次の落ち物・得点の帯をフィールドのどちら側に出すか（省略は右）
 *   ペア番号 … 同じマップで同じ番号の台 2 台が 1 組（対戦）。省略・0 はペアなし（ひとり用）
 *   sdx, sdy … 座る場所：台NPCのマスからの相対位置（マス）。省略は (0,+1)＝台NPCの 1 つ下（既定の盤面の真下）。
 *             台NPCは見えない（体も名前も描かない。CMD_CHARINFO に載せる byGhost を CHARGHOST_NAMEONLY にして送り、
 *             クライアントは台詞が @p のそのキャラの名前も描かない。データは書き換えない）ので、
 *             座る場所のマスに立って S キーを押すと座る（向きは問わない。台NPCが目の前でなくてもよい）
 *   例  @p20,1            … アイテム 20 を 1 枚使って遊ぶ。フィールドは席の真上（10×14）
 *       @p20,1,,,10,18    … 10×18 にする（位置は既定＝席の真上に合わせて決まる）
 *       @p20,1,2,-3       … フィールドの左上を席の右へ 2・上へ 3 マスにする（席の横に置く）
 *       @p20,1,,,,,,1     … ペア番号 1（もう 1 台も ,,,,,,1 にすると対戦台になる）
 *       @p20,1,,,,,,,3,0  … 座る場所を台NPCの右 3 マスにする
 *       @p                … 無料
 *   枚数 0 かアイテム番号 0 なら無料。
 * ゲームの正はサーバー。盤面・操作中の落ち物・次の落ち物・得点はここで持ち、変わった時だけ送る
 * （盤面が変わった時は全体、動いただけの時は位置だけ）。
 * 落下は周期処理（PuzzleProc）で g_dwTime の経過時間で進める（呼ばれる間隔は不定なので）。
 *
 * 台は 1 台＝1 フィールド＝1 席。対戦は「ペアの 2 台」で行う:
 *   ひとりが座って遊んでいる所に、ペアのもう 1 台の席へ 2 人目が座る（コインを使う）と対戦開始。
 *   両方の盤面を空に戻し、PUZZLE_VS_COUNTDOWN_SEC 秒のカウントダウンのあと同時スタート（落ち物の順番は共通の種）。
 *   ライン消しでお邪魔ブロックを相手へ送る（FallingPuzzle.h）。置けなくなった側が負け。
 *   負けた側は席を立ち、勝った側はそのままひとり用に続ける。ESC・切断・マップ移動で席を立った場合は残った側の勝ち。
 * 送る相手（台ごと）: 座っている人・対戦相手の台の人・台の近くにいる人（観戦。画面に入る距離。PUZZLE_WATCH_MS ごとに見直し、
 *   近づいた時に盤面全体を 1 回送り、離れた時に NONE を送る）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrPuzzle.h"
#include "../../common/FallingPuzzle.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#define PUZZLE_NOCHAR			((DWORD)-1)
#define PUZZLE_KEY_MIN_MS		(25)		/* キー入力の最短間隔（これより速い入力は捨てる。連打・改造対策） */
#define PUZZLE_NEAR				(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define PUZZLE_WATCH_MS			(250)		/* 台の近くにいる人（観戦）を見直す間隔(ms) */
/* 盤面を送る距離（横・縦それぞれ。盤面の中心から）。盤面の一部でも画面（地図 15×15 マス）にかかる距離は、
   画面の半分 7.5 マス＋歩きのずれ 1 マス＋盤面の半分（横 cols/4・縦 rows/4 マス。10×14 で 2.5・3.5）＋帯（横に約 3.2 マス）。
   横は最大 7.5+1+2.5+3.2＝14.2、縦は 7.5+1+3.5＝12 なので、MARGIN＝12 に盤面の半分を足した距離（横 14.5・縦 15.5）でもかかる範囲は足りている。
   送り始めは MARGIN＋3（横 17・縦 18）、消すのは＋6（横 20・縦 21）で、画面にかかっている間に届かない・先に消えることはない */
#define PUZZLE_VIEW_MARGIN		(12)		/* 盤面が画面に入り始める距離：フィールドの中心から、画面の半分（7 マス）＋帯・余白ぶん（マス）。盤面の大きさを足す */
#define PUZZLE_VIEW_ENTER		(PUZZLE_VIEW_MARGIN + 3)	/* 盤面を送り始める距離（画面に入り始める距離より 3 マス手前。受信済みで端からスクロールインして見える） */
#define PUZZLE_VIEW_LEAVE		(PUZZLE_VIEW_MARGIN + 6)	/* 盤面を消す（NONE を送る）距離（送り始める距離より 3 マス外。境目で送る・消すを繰り返さない） */

/* 台の設定（台NPCの台詞から読む） */
typedef struct _PUZZLEPARAM {
	WORD	wCoinItem;						/* コインのアイテム番号（0=無料） */
	int		nCoinCount;						/* 必要な枚数（0=無料） */
	int		nDx;							/* フィールドの左上：台のマスの左上から（マス） */
	int		nDy;
	int		nCols;							/* 盤面の横（段数） */
	int		nRows;							/* 盤面の縦（段数） */
	int		nSide;							/* 帯の側（PUZZLESIDE_*） */
	int		nPair;							/* ペア番号（0=ペアなし） */
	int		nSdx;							/* 座る場所：台のマスから（マス。既定 0,+1） */
	int		nSdy;
} PUZZLEPARAM;

/* 席 */
typedef struct _PUZZLESEAT {
	DWORD		dwChar;						/* 座っているキャラのテーブルID（いなければ PUZZLE_NOCHAR） */
	DWORD		dwPID;						/* そのキャラのプレイヤーID（テーブルIDの使い回しを見分ける） */
	DWORD		dwLastKey;					/* 最後にキーを受けた時刻 */
	PuzzleGame	Game;						/* ゲーム */
} PUZZLESEAT;

/* 盤面を送ってある人 */
typedef struct _PUZZLEWATCH {
	DWORD		dwChar;						/* キャラのテーブルID */
	DWORD		dwPID;						/* プレイヤーID */
} PUZZLEWATCH;

/* 台（座っている人がいる間と、待機中の台の近くに人がいる間・最後の盤面を残している間、表に載せる） */
typedef struct _PUZZLETABLE {
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	DWORD		dwMapID;					/* 台NPCのいるマップ */
	PUZZLEPARAM	Param;
	PUZZLESEAT	Seat;						/* 席は 1 つ */
	BOOL		bVs;						/* 対戦中か */
	DWORD		dwOppNpc;					/* 対戦相手の台NPCのテーブルID（bVs の時） */
	BOOL		bCount;						/* カウントダウン中か（この間は落ちず、キーも効かない） */
	DWORD		dwStartTime;				/* カウントダウンが終わる時刻(ms) */
	int			nCountSent;					/* 最後に知らせたカウントダウンの残り秒 */
	DWORD		dwLastWatch;				/* 最後に観戦の人を見直した時刻 */
	BOOL		bIdle;						/* 待機中（席に誰もいない。Seat.Game は空の盤面か、bLast なら最後の盤面） */
	BOOL		bLast;						/* 待機中で、ゲームオーバーの最後の盤面を残している（次に誰かが座るまで） */
	std::vector<PUZZLEWATCH>	aWatch;		/* 盤面を送ってある人（座っている人・対戦相手・近くにいる人） */
} PUZZLETABLE;

static std::vector<PUZZLETABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 台詞 "@p..." のパラメータを読む（コンマ区切り。空欄・足りない欄は既定。読み方は common/FallingPuzzle.h の PuzzleParseSpeech） */
static int Clamp(int n, int nMin, int nMax) { return (n < nMin) ? nMin : (n > nMax) ? nMax : n; }
static void ParseParam(LPCSTR pszSpeech, PUZZLEPARAM *pParam)
{
	int anVal[PUZZLE_PARAM_COUNT];

	PuzzleParseSpeech (pszSpeech, anVal);
	pParam->wCoinItem	= ((anVal[0] < 0) || (anVal[0] > 65535)) ? 0 : (WORD)anVal[0];
	pParam->nCoinCount	= (anVal[1] < 0) ? 0 : (anVal[1] > 99) ? 99 : anVal[1];
	pParam->nCols		= (anVal[4] == PUZZLE_PARAM_NONE) ? PUZZLE_COLS : Clamp (anVal[4], 4, PUZZLE_MAX_COLS);
	pParam->nRows		= (anVal[5] == PUZZLE_PARAM_NONE) ? PUZZLE_ROWS : Clamp (anVal[5], 4, PUZZLE_MAX_ROWS);
	pParam->nSide		= (anVal[6] == PUZZLESIDE_LEFT) ? PUZZLESIDE_LEFT : PUZZLESIDE_RIGHT;
	pParam->nPair		= (anVal[7] == PUZZLE_PARAM_NONE) ? 0 : Clamp (anVal[7], 0, 9999);
	/* dx, dy の省略時は、席がフィールドの真下（横は中央） */
	pParam->nDx = (anVal[2] == PUZZLE_PARAM_NONE) ? PuzzleDefaultDx (pParam->nCols) : Clamp (anVal[2], -60, 60);
	pParam->nDy = (anVal[3] == PUZZLE_PARAM_NONE) ? PuzzleDefaultDy (pParam->nRows) : Clamp (anVal[3], -60, 60);
	/* 座る場所（台NPCのマスから。省略は 1 つ下） */
	PuzzleSeatOffset (pszSpeech, &pParam->nSdx, &pParam->nSdy);
}

static PCHARINFO SeatChar(const PUZZLESEAT &Seat)
{
	PCHARINFO pChar;

	if ((Seat.dwChar == PUZZLE_NOCHAR) || (Seat.dwChar >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	pChar = g_aCharInfo[Seat.dwChar];
	if ((pChar == NULL) || (pChar->pID != Seat.dwPID)) {
		return NULL;
	}
	return pChar;
}

/* 台を探す（無ければ -1） */
static int FindTable(DWORD dwNpc, DWORD dwMapID)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwNpc == dwNpc) && (s_aTable[i].dwMapID == dwMapID)) {
			return i;
		}
	}
	return -1;
}

/* 座っている台を探す（座っていなければ FALSE） */
static BOOL FindSeat(PCHARINFO pChar, int *pnTable)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].Seat.dwChar == pChar->dwTblID) && (s_aTable[i].Seat.dwPID == pChar->pID)) {
			*pnTable = i;
			return TRUE;
		}
	}
	return FALSE;
}

/* 席が空になった台を待機中にする（表からは消さない。近くの人への盤面は待機状態で送り続ける）。
   bLast なら最後の盤面をそのまま残す。そうでなければ空の盤面に戻す */
static void ToIdle(int nTable, BOOL bLast)
{
	PUZZLETABLE &Table = s_aTable[nTable];
	int nCols = Table.Param.nCols, nRows = Table.Param.nRows;

	Table.Seat.dwChar = PUZZLE_NOCHAR;
	Table.Seat.dwPID = 0;
	Table.bIdle = TRUE;
	Table.bLast = bLast;
	Table.bVs = FALSE;
	Table.dwOppNpc = (DWORD)-1;
	Table.bCount = FALSE;
	if (!bLast) {
		ZeroMemory (&Table.Seat.Game, sizeof (Table.Seat.Game));
		Table.Seat.Game.Board.Init (nCols, nRows);
	} else {
		Table.Seat.Game.nPending = 0;
	}
}

static PCHARINFO WatchChar(const PUZZLEWATCH &w)
{
	PCHARINFO pChar;

	if (w.dwChar >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pChar = g_aCharInfo[w.dwChar];
	if ((pChar == NULL) || (pChar->pID != w.dwPID) || (pChar->byGameState < GAMESTATE_MOVE)) {
		return NULL;
	}
	return pChar;
}

/* カウントダウンの残り秒（0=始まっている） */
static int CountRemain(const PUZZLETABLE &Table)
{
	int nMs;

	if (!Table.bCount) {
		return 0;
	}
	nMs = (int)(Table.dwStartTime - g_dwTime);
	return (nMs <= 0) ? 0 : (nMs + 999) / 1000;
}

/* 台の状態のパケットを作る。byKind: FULL=盤面全体 MOVE=位置だけ OVER=終わり（盤面付き） NONE=消す。
   byMine は送る相手ごとに書き換える */
static void BuildState(const PUZZLETABLE &Table, BYTE byKind, std::vector<BYTE> &aBuf)
{
	const PuzzleGame &g = Table.Seat.Game;
	CMDPARA_PUZZLESTATE Head;
	int nCells = g.Board.nCols * g.Board.nRows, nX, nY;
	BOOL bBoard = (byKind == PUZZLESTATE_FULL) || (byKind == PUZZLESTATE_OVER);
	BOOL bIdle = Table.bIdle && (byKind != PUZZLESTATE_OVER);
	PCHARINFO pNpc = (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;

	ZeroMemory (&Head, sizeof (Head));
	Head.byCmd		= CMD_PUZZLESTATE;
	Head.byKind		= byKind;
	Head.bySeat		= 0;
	Head.bySide		= (BYTE)Table.Param.nSide;
	Head.byCols		= (BYTE)g.Board.nCols;
	Head.byRows		= (BYTE)g.Board.nRows;
	Head.byPiece	= (BYTE)g.nKind;
	Head.byRot		= (BYTE)g.nRot;
	Head.byNext		= (BYTE)g.nNext;
	Head.byLevel	= (BYTE)((g.nLevel > 255) ? 255 : g.nLevel);
	Head.cX			= (signed char)g.nX;
	Head.cY			= (signed char)g.nY;
	Head.wLines		= (WORD)((g.nLines > 65535) ? 65535 : g.nLines);
	Head.dwScore	= g.dwScore;
	Head.dwNpc		= Table.dwNpc;
	Head.byVs		= (BYTE)(Table.bVs ? 1 : 0);
	Head.byGarbage	= (BYTE)((g.nPending > 255) ? 255 : g.nPending);
	Head.byCount	= (BYTE)CountRemain (Table);
	Head.byIdle		= (BYTE)(bIdle ? 1 : 0);
	if (bIdle) {
		/* 待機中は落ち物・次の落ち物・得点・待ちお邪魔を持たない（盤面の枠と井戸だけ） */
		Head.byPiece = Head.byNext = 255;
		Head.byLevel = 0;
		Head.wLines = 0;
		Head.dwScore = 0;
		Head.byVs = 0;
		Head.byGarbage = 0;
		Head.byCount = 0;
	}
	if (pNpc) {
		PuzzleFieldPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.Param.nDx, Table.Param.nDy, &nX, &nY);
		Head.sFieldX = (short)nX;
		Head.sFieldY = (short)nY;
	}
	aBuf.resize (sizeof (Head) + (bBoard ? nCells : 0));
	memcpy (&aBuf[0], &Head, sizeof (Head));
	if (bBoard) {
		memcpy (&aBuf[sizeof (Head)], g.Board.abyCell, nCells);
	}
}

/* 1 人に送る（byMine はその人が席に座っているかで決める） */
static void SendStateTo(const PUZZLETABLE &Table, PCHARINFO pChar, std::vector<BYTE> &aBuf)
{
	((PCMDPARA_PUZZLESTATE)&aBuf[0])->byMine = (SeatChar (Table.Seat) == pChar) ? 1 : 0;
	SendCmdPUZZLESTATE (pChar->pID, &aBuf[0], (DWORD)aBuf.size ());
}

/* 台の状態を、盤面を送ってある全員（座っている人・対戦相手・近くにいる人）に送る。pSkip には送らない */
static void SendTable(int nTable, BYTE byKind, PCHARINFO pSkip = NULL)
{
	const PUZZLETABLE &Table = s_aTable[nTable];
	std::vector<BYTE> aBuf;
	int i;

	BuildState (Table, byKind, aBuf);
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar = WatchChar (Table.aWatch[i]);
		if ((pChar == NULL) || (pChar == pSkip)) {
			continue;
		}
		SendStateTo (Table, pChar, aBuf);
	}
}

/* 台の盤面を送る距離にいるか（フィールドの中心から nMargin マス＋盤面の大きさの範囲。
   送り始める距離は PUZZLE_VIEW_ENTER、すでに送ってある人が消える距離は PUZZLE_VIEW_LEAVE） */
static BOOL WatchEligible(const PUZZLEPARAM &Param, DWORD dwMapID, PCHARINFO pNpc, PCHARINFO pChar, int nMargin)
{
	int nFx, nFy, nCx, nCy;

	if ((pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) || (pChar->dwMapID != dwMapID) ||
		!IsSameSpace (pNpc, pChar) || !IsCharSeenBy (pNpc, pChar)) {
		return FALSE;
	}
	PuzzleFieldPos ((int)pNpc->dwX, (int)pNpc->dwY, Param.nDx, Param.nDy, &nFx, &nFy);
	nCx = (nFx + Param.nCols * PUZZLE_BLOCK_DOT / 2) / PUZZLE_TILE_DOT;
	nCy = (nFy + Param.nRows * PUZZLE_BLOCK_DOT / 2) / PUZZLE_TILE_DOT;
	return (abs ((int)pChar->dwX - nCx) <= nMargin + Param.nCols / 4) &&
		(abs ((int)pChar->dwY - nCy) <= nMargin + Param.nRows / 4);
}

static BOOL InWatch(const std::vector<PUZZLEWATCH> &aWatch, const PUZZLEWATCH &w)
{
	int i;

	for (i = 0; i < (int)aWatch.size (); i ++) {
		if ((aWatch[i].dwChar == w.dwChar) && (aWatch[i].dwPID == w.dwPID)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 盤面が見える人を見直す。新しく見える人には盤面全体を 1 回送り、見えなくなった人には NONE を送って消してもらう */
static void WatchRefresh(int nTable)
{
	PUZZLETABLE &Table = s_aTable[nTable];
	PCHARINFO pNpc = (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
	std::vector<PUZZLEWATCH> aNew;
	std::vector<BYTE> aFull, aNone;
	PCHARINFO pSeat, pOpp = NULL;
	PMAPINFO pMap;
	int i;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	/* 座っている人と、対戦相手の台の人は距離に関係なく入れる */
	pSeat = SeatChar (Table.Seat);
	if (Table.bVs) {
		int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);
		if (nOpp >= 0) {
			pOpp = SeatChar (s_aTable[nOpp].Seat);
		}
	}
	if (pSeat) {
		PUZZLEWATCH w;
		w.dwChar = pSeat->dwTblID;
		w.dwPID = pSeat->pID;
		aNew.push_back (w);
	}
	if (pOpp && (pOpp != pSeat)) {
		PUZZLEWATCH w;
		w.dwChar = pOpp->dwTblID;
		w.dwPID = pOpp->pID;
		aNew.push_back (w);
	}
	/* 同じマップの近くにいる人 */
	pMap = (Table.dwMapID < (DWORD)g_aMapInfo.GetSize ()) ? g_aMapInfo[Table.dwMapID] : NULL;
	if (pMap && pMap->paTblNoPC) {
		for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
			PCHARINFO pChar;
			PUZZLEWATCH w;

			if (pMap->paTblNoPC[i] == (DWORD)-1) {
				break;
			}
			pChar = g_aCharInfo[pMap->paTblNoPC[i]];
			if ((pChar == pSeat) || (pChar == pOpp)) {
				continue;
			}
			w.dwChar = pChar->dwTblID;
			w.dwPID = pChar->pID;
			/* すでに送ってある人は遠い距離（LEAVE）まで、新しい人は近い距離（ENTER）から。境目で送る・消すを繰り返さない */
			if (!WatchEligible (Table.Param, Table.dwMapID, pNpc, pChar, InWatch (Table.aWatch, w) ? PUZZLE_VIEW_LEAVE : PUZZLE_VIEW_ENTER)) {
				continue;
			}
			aNew.push_back (w);
		}
	}
	/* 見えなくなった人へ NONE */
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar;
		if (InWatch (aNew, Table.aWatch[i])) {
			continue;
		}
		pChar = WatchChar (Table.aWatch[i]);
		if (pChar) {
			if (aNone.empty ()) {
				BuildState (Table, PUZZLESTATE_NONE, aNone);
			}
			SendStateTo (Table, pChar, aNone);
		}
	}
	/* 新しく見える人へ盤面全体 */
	for (i = 0; i < (int)aNew.size (); i ++) {
		PCHARINFO pChar;
		if (InWatch (Table.aWatch, aNew[i])) {
			continue;
		}
		pChar = WatchChar (aNew[i]);
		if (pChar) {
			if (aFull.empty ()) {
				BuildState (Table, PUZZLESTATE_FULL, aFull);
			}
			SendStateTo (Table, pChar, aFull);
		}
	}
	Table.aWatch = aNew;
}

/* 対戦を終える（残った側。ひとり用に戻って続ける）。待ちお邪魔は捨て、カウントダウン中なら今から始める */
static void EndVs(int nTable, PCHARINFO pSkip)
{
	PUZZLETABLE &Table = s_aTable[nTable];

	Table.bVs = FALSE;
	Table.dwOppNpc = (DWORD)-1;
	Table.Seat.Game.nPending = 0;
	if (Table.bCount) {
		Table.bCount = FALSE;
		Table.Seat.Game.dwLastDrop = g_dwTime;
	}
	SendTable (nTable, PUZZLESTATE_FULL, pSkip);
}

/* 席を立たせる。byKind は周りへ知らせる内容（NONE=消す・OVER=終わりの盤面を少し見せる）。
   bNotifySelf でなければ本人へは送らない（切断）。bLost なら得点を知らせる。
   対戦中なら残った側の勝ち（両者へ「○○の勝ち！」）。台は空になって表から消える */
static void Leave(int nTable, BYTE byKind, BOOL bNotifySelf, BOOL bLost)
{
	PUZZLETABLE &Table = s_aTable[nTable];
	PCHARINFO pSelf = SeatChar (Table.Seat);
	char szMsg[128];

	if (Table.bVs) {
		int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);
		PCHARINFO pWin = (nOpp >= 0) ? SeatChar (s_aTable[nOpp].Seat) : NULL;

		Table.bVs = FALSE;
		if (nOpp >= 0) {
			EndVs (nOpp, bNotifySelf ? NULL : pSelf);
		}
		if (pWin) {
			sprintf (szMsg, "%sの勝ち！", pWin->szName);
			SendCmdSYSMSG (pWin->pID, szMsg);
			if (pSelf && bNotifySelf) {
				SendCmdSYSMSG (pSelf->pID, szMsg);
			}
		}
	}
	if (bLost && pSelf) {
		sprintf (szMsg, "ゲームオーバー！ 得点 %u（%dライン・レベル%d）", Table.Seat.Game.dwScore, Table.Seat.Game.nLines, Table.Seat.Game.nLevel);
		SendCmdSYSMSG (pSelf->pID, szMsg);
	}
	/* 本人へ：ゲームオーバーは OVER（3 秒ほど見せる）。ほかの人（と、席を立っただけの本人）へは、
	   待機中の台の状態（最後の盤面か空の井戸。byIdle=1）を送る。切断・マップ移動の本人へは送らない */
	if ((byKind == PUZZLESTATE_OVER) && pSelf && bNotifySelf) {
		std::vector<BYTE> aOver;
		BuildState (Table, PUZZLESTATE_OVER, aOver);
		SendStateTo (Table, pSelf, aOver);
	}
	ToIdle (nTable, byKind == PUZZLESTATE_OVER);
	SendTable (nTable, PUZZLESTATE_FULL, ((byKind == PUZZLESTATE_OVER) || !bNotifySelf) ? pSelf : NULL);
}

/* 入力・周期処理の結果を送る（変わった所だけ）。置けなくなったら負け／ゲームオーバー。
   対戦中は、固定で消したラインのお邪魔を相手の待ちに足す */
static void SendChanged(int nTable, int nChg)
{
	PUZZLETABLE &Table = s_aTable[nTable];
	PuzzleGame &g = Table.Seat.Game;
	PCHARINFO pChar = SeatChar (Table.Seat);
	int nAttack = g.nAttack;

	if (pChar == NULL) {
		return;
	}
	g.nAttack = 0;
	if (nChg & PUZZLE_CHG_OVER) {
		Leave (nTable, PUZZLESTATE_OVER, TRUE, TRUE);
		return;
	}
	if (nChg & (PUZZLE_CHG_BOARD | PUZZLE_CHG_NEXT | PUZZLE_CHG_SCORE | PUZZLE_CHG_GARBAGE)) {
		SendTable (nTable, PUZZLESTATE_FULL);
	} else if (nChg & PUZZLE_CHG_PIECE) {
		SendTable (nTable, PUZZLESTATE_MOVE);
	}
	if (Table.bVs && (nAttack > 0) && (nChg & PUZZLE_CHG_BOARD)) {
		int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);
		if ((nOpp >= 0) && s_aTable[nOpp].Seat.Game.AddPending (nAttack)) {
			SendTable (nOpp, PUZZLESTATE_FULL);
		}
	}
}

/* 手持ちにあるコインの数（装備品でない普通のアイテム） */
static int CountCoin(PCHARINFO pChar, WORD wItem)
{
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == wItem) {
			nRet ++;
		}
	}
	return nRet;
}

/* ペアの台（同じマップ・同じ空間で、ペア番号が同じ別の台NPC）のテーブルIDを探す（無ければ -1） */
static DWORD FindPartnerNpc(PCHARINFO pNpc, const PUZZLEPARAM &Param)
{
	PMAPINFO pMap;
	int i;

	if ((Param.nPair <= 0) || (pNpc->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return (DWORD)-1;
	}
	pMap = g_aMapInfo[pNpc->dwMapID];
	if (pMap == NULL || pMap->paTblNoNPC == NULL) {
		return (DWORD)-1;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		PCHARINFO pTmp;
		PUZZLEPARAM Tmp;

		if (pMap->paTblNoNPC[i] == (DWORD)-1) {
			break;
		}
		pTmp = g_aCharInfo[pMap->paTblNoNPC[i]];
		if ((pTmp == pNpc) || !PuzzleIsTableNpc (pTmp) || !IsSameSpace (pTmp, pNpc)) {
			continue;
		}
		ParseParam (pTmp->pszNpcSpeech, &Tmp);
		if (Tmp.nPair == Param.nPair) {
			return pTmp->dwTblID;
		}
	}
	return (DWORD)-1;
}

/* ========================================================================= */
/* 外から呼ぶ関数															 */
/* ========================================================================= */

BOOL PuzzleIsTableNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'p');
}

BOOL PuzzleOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc)
{
	PUZZLEPARAM Param;

	if (!PuzzleIsTableNpc (pNpc) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return FALSE;
	}
	ParseParam (pNpc->pszNpcSpeech, &Param);
	return ((int)pChar->dwX == (int)pNpc->dwX + Param.nSdx) && ((int)pChar->dwY == (int)pNpc->dwY + Param.nSdy);
}

BYTE PuzzleGhostForSend(PCHARINFO pChar)
{
	/* 台NPC（@p）は体を描かない（名前だけの目印と同じ扱い）。データは書き換えず、送る時だけそうする */
	return PuzzleIsTableNpc (pChar) ? (BYTE)CHARGHOST_NAMEONLY : pChar->byGhost;
}

BOOL PuzzleIsSeated(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable);
}

BOOL PuzzleIsVersus(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable) && s_aTable[nTable].bVs;
}

PuzzleGame *PuzzleGetGame(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable) ? &s_aTable[nTable].Seat.Game : NULL;
}

int PuzzleSeatCount(void)
{
	int i, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if (!s_aTable[i].bIdle) {
			nRet ++;
		}
	}
	return nRet;
}

int PuzzleIdleCount(void)
{
	return (int)s_aTable.size () - PuzzleSeatCount ();
}

void PuzzleSit(PCHARINFO pChar, PCHARINFO pNpc)
{
	PUZZLEPARAM Param;
	int i, nTable, nPartner = -1;
	DWORD dwPartnerNpc;
	char szMsg[96];

	if (!PuzzleIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || PuzzleIsSeated (pChar)) {
		return;
	}
	/* 座れるのは、台の座る場所のマスの上だけ（同じマップ・同じ空間。向きは問わない。隣のマスから台NPC越しには座らない） */
	if (!PuzzleOnSeatCell (pChar, pNpc)) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}
	ParseParam (pNpc->pszNpcSpeech, &Param);

	/* この台は席が 1 つ。使われていれば座れない（待機中の台は座れる） */
	nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
	if ((nTable >= 0) && !s_aTable[nTable].bIdle) {
		SendCmdSYSMSG (pChar->pID, "この台は今、使われています。");
		return;
	}
	/* ペアの台でひとりが遊んでいれば、対戦になる */
	dwPartnerNpc = FindPartnerNpc (pNpc, Param);
	if (dwPartnerNpc != (DWORD)-1) {
		nPartner = FindTable (dwPartnerNpc, pNpc->dwMapID);
		if ((nPartner >= 0) && (s_aTable[nPartner].bVs || (SeatChar (s_aTable[nPartner].Seat) == NULL))) {
			nPartner = -1;
		}
	}

	/* コイン（無料の台は何も使わない）。足りなければ座らせない */
	if ((Param.wCoinItem != 0) && (Param.nCoinCount > 0)) {
		if (CountCoin (pChar, Param.wCoinItem) < Param.nCoinCount) {
			SendCmdSYSMSG (pChar->pID, "コインがありません。");
			return;
		}
		for (i = 0; i < Param.nCoinCount; i ++) {
			QuestTakeItemOne (pChar, Param.wCoinItem);
		}
		sprintf (szMsg, "コインを%d枚使いました。", Param.nCoinCount);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}

	{
		PUZZLETABLE Table;
		Table.dwNpc = pNpc->dwTblID;
		Table.dwMapID = pNpc->dwMapID;
		Table.Param = Param;
		Table.bVs = FALSE;
		Table.dwOppNpc = (DWORD)-1;
		Table.bCount = FALSE;
		Table.dwStartTime = 0;
		Table.nCountSent = 0;
		Table.dwLastWatch = 0;
		Table.bIdle = FALSE;
		Table.bLast = FALSE;
		Table.Seat.dwChar = pChar->dwTblID;
		Table.Seat.dwPID = pChar->pID;
		Table.Seat.dwLastKey = 0;
		if (nTable >= 0) {
			/* 待機中の台に座る：今見ている人の表を引き継ぐ（下で、見ている人全員に始まった盤面を送る） */
			Table.aWatch = s_aTable[nTable].aWatch;
			s_aTable[nTable] = Table;
		} else {
			s_aTable.push_back (Table);
			nTable = (int)s_aTable.size () - 1;
		}
	}
	{
		PUZZLETABLE &Table = s_aTable[nTable];
		unsigned int dwSeed = (unsigned int)(g_dwTime * 2654435761u) ^ (pChar->dwTblID * 40503u + 1);

		if (nPartner < 0) {
			Table.Seat.Game.Start (Param.nCols, Param.nRows, dwSeed, g_dwTime);
		} else {
			/* 対戦開始：両方の盤面を空に戻し、同じ種（同じ落ち物の順番）でカウントダウンのあと同時にスタート */
			PUZZLETABLE &Partner = s_aTable[nPartner];
			DWORD dwStart = g_dwTime + PUZZLE_VS_COUNTDOWN_SEC * 1000;
			PCHARINFO pOther = SeatChar (Partner.Seat);

			Table.Seat.Game.Start (Param.nCols, Param.nRows, dwSeed, dwStart);
			Partner.Seat.Game.Start (Partner.Param.nCols, Partner.Param.nRows, dwSeed, dwStart);
			Partner.Seat.dwLastKey = 0;
			Table.bVs = Partner.bVs = TRUE;
			Table.dwOppNpc = Partner.dwNpc;
			Partner.dwOppNpc = Table.dwNpc;
			Table.bCount = Partner.bCount = TRUE;
			Table.dwStartTime = Partner.dwStartTime = dwStart;
			Table.nCountSent = Partner.nCountSent = PUZZLE_VS_COUNTDOWN_SEC;
			if (pOther) {
				sprintf (szMsg, "対戦開始！ 相手は%sです。", pOther->szName);
				SendCmdSYSMSG (pChar->pID, szMsg);
				sprintf (szMsg, "対戦開始！ 相手は%sです。", pChar->szName);
				SendCmdSYSMSG (pOther->pID, szMsg);
			}
			/* 相手の盤面（空に戻った）を今の見ている人へ。新しく見える人（今座った人）へは見直しで届く */
			SendTable (nPartner, PUZZLESTATE_FULL);
			WatchRefresh (nPartner);
		}
	}
	/* 自分の台：待機中から見ていた人（座った本人を含む）へ始まった盤面全体。新しく見える人へは見直しで届く */
	SendTable (nTable, PUZZLESTATE_FULL);
	WatchRefresh (nTable);
}

void PuzzleKey(PCHARINFO pChar, BYTE byKey)
{
	int nTable, nChg = 0;

	if (!FindSeat (pChar, &nTable)) {
		return;
	}
	PUZZLESEAT &Seat = s_aTable[nTable].Seat;

	if (byKey == PUZZLEKEY_LEAVE) {
		Leave (nTable, PUZZLESTATE_NONE, TRUE, FALSE);
		return;
	}
	/* カウントダウン中は動かせない */
	if (s_aTable[nTable].bCount) {
		return;
	}
	/* 速すぎる入力は捨てる */
	if ((Seat.dwLastKey != 0) && ((DWORD)(g_dwTime - Seat.dwLastKey) < PUZZLE_KEY_MIN_MS)) {
		return;
	}
	Seat.dwLastKey = g_dwTime;
	switch (byKey) {
	case PUZZLEKEY_LEFT:	nChg = Seat.Game.Move (-1);						break;
	case PUZZLEKEY_RIGHT:	nChg = Seat.Game.Move (1);						break;
	case PUZZLEKEY_DOWN:	nChg = Seat.Game.Step (true, g_dwTime);			break;
	case PUZZLEKEY_ROTATE:	nChg = Seat.Game.Rotate ();						break;
	default:				return;
	}
	SendChanged (nTable, nChg);
}

/* 待機中の台を新しく表に載せる：人のいるマップの台NPC（席が空）のうち、近くに人がいるものだけ。
   近くに誰もいない台は表に載らない（何も送らない・覚えない） */
static void IdleScan(void)
{
	int nMap, i, j;

	for (nMap = 0; nMap < (int)g_aMapInfo.GetSize (); nMap ++) {
		PMAPINFO pMap = g_aMapInfo[nMap];

		if ((pMap == NULL) || (pMap->paTblNoPC == NULL) || (pMap->paTblNoNPC == NULL) || (pMap->dwCountTblNoPC == 0) ||
			(pMap->paTblNoPC[0] == (DWORD)-1)) {
			continue;
		}
		for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
			PCHARINFO pNpc;
			PUZZLEPARAM Param;
			BOOL bNear = FALSE;

			if (pMap->paTblNoNPC[i] == (DWORD)-1) {
				break;
			}
			if (pMap->paTblNoNPC[i] >= (DWORD)g_aCharInfo.GetSize ()) {
				continue;
			}
			pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
			if (!PuzzleIsTableNpc (pNpc) || (FindTable (pNpc->dwTblID, (DWORD)nMap) >= 0)) {
				continue;
			}
			ParseParam (pNpc->pszNpcSpeech, &Param);
			for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
				PCHARINFO pChar;

				if (pMap->paTblNoPC[j] == (DWORD)-1) {
					break;
				}
				if (pMap->paTblNoPC[j] >= (DWORD)g_aCharInfo.GetSize ()) {
					continue;
				}
				pChar = g_aCharInfo[pMap->paTblNoPC[j]];
				if (WatchEligible (Param, (DWORD)nMap, pNpc, pChar, PUZZLE_VIEW_ENTER)) {
					bNear = TRUE;
					break;
				}
			}
			if (bNear) {
				PUZZLETABLE Table;

				Table.dwNpc = pNpc->dwTblID;
				Table.dwMapID = (DWORD)nMap;
				Table.Param = Param;
				Table.bVs = FALSE;
				Table.dwOppNpc = (DWORD)-1;
				Table.bCount = FALSE;
				Table.dwStartTime = 0;
				Table.nCountSent = 0;
				Table.dwLastWatch = 0;
				Table.bIdle = TRUE;
				Table.bLast = FALSE;
				Table.Seat.dwChar = PUZZLE_NOCHAR;
				Table.Seat.dwPID = 0;
				Table.Seat.dwLastKey = 0;
				ZeroMemory (&Table.Seat.Game, sizeof (Table.Seat.Game));
				Table.Seat.Game.Board.Init (Param.nCols, Param.nRows);
				s_aTable.push_back (Table);
				WatchRefresh ((int)s_aTable.size () - 1);
			}
		}
	}
}

void PuzzleProc(void)
{
	static DWORD s_dwLastScan = 0;
	int i;

	/* 台が消えると添字がずれるので、後ろから 1 つずつ（消えるのはその台だけ） */
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		if (i >= (int)s_aTable.size ()) {
			continue;
		}
		PUZZLETABLE &Table = s_aTable[i];
		PCHARINFO pNpc = (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
		BOOL bNpcOK = PuzzleIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);
		PCHARINFO pChar = SeatChar (Table.Seat);
		int nChg, nSize;

		/* 待機中の台：台が無くなれば見ている人へ NONE を送って表から消す。
		   近くの人の見直しをして、誰もいなくなって最後の盤面も残していなければ表から消す */
		if (Table.bIdle) {
			if (!bNpcOK) {
				std::vector<BYTE> aNone;
				int k;

				BuildState (Table, PUZZLESTATE_NONE, aNone);
				for (k = 0; k < (int)Table.aWatch.size (); k ++) {
					PCHARINFO pW = WatchChar (Table.aWatch[k]);
					if (pW) {
						SendStateTo (Table, pW, aNone);
					}
				}
				s_aTable.erase (s_aTable.begin () + i);
				continue;
			}
			if ((DWORD)(g_dwTime - Table.dwLastWatch) >= PUZZLE_WATCH_MS) {
				WatchRefresh (i);
			}
			if (Table.aWatch.empty () && !Table.bLast) {
				s_aTable.erase (s_aTable.begin () + i);
			}
			continue;
		}

		/* 切断・マップ移動・席から離れた・倒れた・台が無くなった時は片付ける（対戦中なら相手の勝ち） */
		if ((pChar == NULL) || !bNpcOK || (pChar->byGameState < GAMESTATE_MOVE) ||
			(pChar->dwMapID != Table.dwMapID) ||
			(abs ((int)pChar->dwX - ((int)pNpc->dwX + Table.Param.nSdx)) > PUZZLE_NEAR) ||
			(abs ((int)pChar->dwY - ((int)pNpc->dwY + Table.Param.nSdy)) > PUZZLE_NEAR) ||
			(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
			Leave (i, PUZZLESTATE_NONE, TRUE, FALSE);
			continue;
		}
		/* 台の近くにいる人（観戦）の見直し */
		if ((DWORD)(g_dwTime - Table.dwLastWatch) >= PUZZLE_WATCH_MS) {
			WatchRefresh (i);
		}
		/* 対戦開始前のカウントダウン（秒が変わった時と、始まる時に知らせる。それまで落ちない） */
		if (Table.bCount) {
			if ((int)(g_dwTime - Table.dwStartTime) >= 0) {
				Table.bCount = FALSE;
				Table.nCountSent = 0;
				Table.Seat.Game.dwLastDrop = g_dwTime;
				SendTable (i, PUZZLESTATE_MOVE);
			} else {
				int nRemain = CountRemain (Table);
				if (nRemain != Table.nCountSent) {
					Table.nCountSent = nRemain;
					SendTable (i, PUZZLESTATE_MOVE);
				}
				continue;
			}
		}
		nChg = Table.Seat.Game.Tick (g_dwTime);
		if (nChg) {
			nSize = (int)s_aTable.size ();
			SendChanged (i, nChg);
			if (nSize != (int)s_aTable.size ()) {
				continue;
			}
		}
	}
	/* 待機中の台を新しく見つける（PUZZLE_WATCH_MS ごと） */
	if ((DWORD)(g_dwTime - s_dwLastScan) >= PUZZLE_WATCH_MS) {
		s_dwLastScan = g_dwTime;
		IdleScan ();
	}
}

void PuzzleOnLeave(PCHARINFO pChar)
{
	int nTable;

	if (FindSeat (pChar, &nTable)) {
		Leave (nTable, PUZZLESTATE_NONE, FALSE, FALSE);
	}
}
