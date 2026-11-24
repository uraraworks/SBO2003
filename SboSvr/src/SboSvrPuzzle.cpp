/* ========================================================================= */
/* ファイル名：	SboSvrPuzzle.cpp											 */
/* 概要：		落ち物パズルの台（台NPC・席・ゲームの進行）の処理(実装ファイル) */
/* 作成日：		2026/11/24													 */
/* ========================================================================= */
/*
 * 仕様は docs/falling-puzzle.md。盤面ロジックは common/FallingPuzzle.h。
 * 台は、台詞が "@p" で始まる NPC。台詞のパラメータ（コンマ区切り。後ろは省略でき、空欄も既定になる）:
 *     @p<コインのアイテム番号>,<必要な枚数>,<dx>,<dy>,<横の段数>,<縦の段数>,<帯の側 0=右 1=左>
 *   dx, dy … フィールドの左上が、台のマスの左上から何マスか（負は左・上）。省略はフィールドの真下に席（10×14 なら -2,-7）
 *   横・縦の段数 … 省略は PUZZLE_COLS × PUZZLE_ROWS（10×18）。台ごとに 10×14 なども指定できる
 *   帯の側 … 次の落ち物・得点の帯をフィールドのどちら側に出すか（省略は右）
 *   例  @p20,1            … アイテム 20 を 1 枚使って遊ぶ。フィールドは席の真上（10×14）
 *       @p20,1,,,10,18    … 10×18 にする（位置は既定＝席の真上に合わせて決まる）
 *       @p20,1,2,-3       … フィールドの左上を席の右へ 2・上へ 3 マスにする（席の横に置く）
 *       @p                … 無料
 *   枚数 0 かアイテム番号 0 なら無料。
 * ゲームの正はサーバー。盤面・操作中の落ち物・次の落ち物・得点はここで持ち、
 * 変わった時だけ座っている人に送る（盤面が変わった時は全体、動いただけの時は位置だけ）。
 * 落下は周期処理（PuzzleProc）で g_dwTime の経過時間で進める（呼ばれる間隔は不定なので）。
 * 席は 1 つの台に PUZZLE_SEATS 個まで（段階2で 2 人目を割り込ませるための配列。段階1は席 0 だけ使う）。
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

#define PUZZLE_SEATS			(2)			/* 1 つの台の席の数 */
#define PUZZLE_SEATS_USED		(1)			/* 今使う席の数（段階1はひとり用。段階2で 2 にする） */
#define PUZZLE_NOCHAR			((DWORD)-1)
#define PUZZLE_KEY_MIN_MS		(25)		/* キー入力の最短間隔（これより速い入力は捨てる。連打・改造対策） */
#define PUZZLE_NEAR				(3)			/* 席の台NPCからこれ以上離れたら席を立たせる（マス） */

/* 台の設定（台NPCの台詞から読む） */
typedef struct _PUZZLEPARAM {
	WORD	wCoinItem;						/* コインのアイテム番号（0=無料） */
	int		nCoinCount;						/* 必要な枚数（0=無料） */
	int		nDx;							/* フィールドの左上：台のマスの左上から（マス） */
	int		nDy;
	int		nCols;							/* 盤面の横（段数） */
	int		nRows;							/* 盤面の縦（段数） */
	int		nSide;							/* 帯の側（PUZZLESIDE_*） */
} PUZZLEPARAM;

/* 席 */
typedef struct _PUZZLESEAT {
	DWORD		dwChar;						/* 座っているキャラのテーブルID（いなければ PUZZLE_NOCHAR） */
	DWORD		dwPID;						/* そのキャラのプレイヤーID（テーブルIDの使い回しを見分ける） */
	DWORD		dwLastKey;					/* 最後にキーを受けた時刻 */
	PuzzleGame	Game;						/* ゲーム */
} PUZZLESEAT;

/* 台 */
typedef struct _PUZZLETABLE {
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	DWORD		dwMapID;					/* 台NPCのいるマップ */
	PUZZLEPARAM	Param;
	PUZZLESEAT	aSeat[PUZZLE_SEATS];
} PUZZLETABLE;

static std::vector<PUZZLETABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 台詞 "@p..." のパラメータを読む（コンマ区切り。空欄・足りない欄は既定） */
#define PUZZLE_PARAM_NONE	(-99999)
static int Clamp(int n, int nMin, int nMax) { return (n < nMin) ? nMin : (n > nMax) ? nMax : n; }
static void ParseParam(LPCSTR pszSpeech, PUZZLEPARAM *pParam)
{
	int anVal[7], i = 0;
	LPCSTR p = pszSpeech + 2;

	for (i = 0; i < 7; i ++) {
		anVal[i] = PUZZLE_PARAM_NONE;
	}
	for (i = 0; i < 7; i ++) {
		BOOL bMinus = FALSE;
		int n = 0;
		BOOL bNum = FALSE;

		if (*p == '-') {
			bMinus = TRUE;
			p ++;
		}
		while ((*p >= '0') && (*p <= '9')) {
			if (n < 100000) {
				n = n * 10 + (*p - '0');
			}
			bNum = TRUE;
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
	pParam->wCoinItem	= ((anVal[0] < 0) || (anVal[0] > 65535)) ? 0 : (WORD)anVal[0];
	pParam->nCoinCount	= (anVal[1] < 0) ? 0 : (anVal[1] > 99) ? 99 : anVal[1];
	pParam->nCols		= (anVal[4] == PUZZLE_PARAM_NONE) ? PUZZLE_COLS : Clamp (anVal[4], 4, PUZZLE_MAX_COLS);
	pParam->nRows		= (anVal[5] == PUZZLE_PARAM_NONE) ? PUZZLE_ROWS : Clamp (anVal[5], 4, PUZZLE_MAX_ROWS);
	pParam->nSide		= (anVal[6] == PUZZLESIDE_LEFT) ? PUZZLESIDE_LEFT : PUZZLESIDE_RIGHT;
	/* dx, dy の省略時は、席がフィールドの真下（横は中央） */
	pParam->nDx = (anVal[2] == PUZZLE_PARAM_NONE) ? PuzzleDefaultDx (pParam->nCols) : Clamp (anVal[2], -60, 60);
	pParam->nDy = (anVal[3] == PUZZLE_PARAM_NONE) ? PuzzleDefaultDy (pParam->nRows) : Clamp (anVal[3], -60, 60);
}

/* 席 nSeat の側（席 0 は台の設定、席 1 は反対側） */
static int SeatSide(const PUZZLEPARAM &Param, int nSeat)
{
	return (nSeat == 0) ? Param.nSide : (Param.nSide == PUZZLESIDE_LEFT) ? PUZZLESIDE_RIGHT : PUZZLESIDE_LEFT;
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

/* 席を探す（座っていなければ FALSE） */
static BOOL FindSeat(PCHARINFO pChar, int *pnTable, int *pnSeat)
{
	int i, j;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		for (j = 0; j < PUZZLE_SEATS; j ++) {
			if ((s_aTable[i].aSeat[j].dwChar == pChar->dwTblID) && (s_aTable[i].aSeat[j].dwPID == pChar->pID)) {
				*pnTable = i;
				*pnSeat = j;
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* 台が空（座っている人がいない）なら表から消す */
static void CleanTable(int nTable)
{
	int j;

	for (j = 0; j < PUZZLE_SEATS; j ++) {
		if (s_aTable[nTable].aSeat[j].dwChar != PUZZLE_NOCHAR) {
			return;
		}
	}
	s_aTable.erase (s_aTable.begin () + nTable);
}

/* 状態を座っている人に送る。byKind: FULL=盤面全体 MOVE=位置だけ OVER=終わり（盤面付き） NONE=消す */
static void SendState(const PUZZLETABLE &Table, int nSeat, PCHARINFO pChar, BYTE byKind)
{
	const PUZZLESEAT &Seat = Table.aSeat[nSeat];
	const PuzzleGame &g = Seat.Game;
	CMDPARA_PUZZLESTATE Head;
	int nCells = g.Board.nCols * g.Board.nRows, nX, nY;
	BOOL bBoard = (byKind == PUZZLESTATE_FULL) || (byKind == PUZZLESTATE_OVER);
	BYTE *pBuf;
	PCHARINFO pNpc = (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;

	ZeroMemory (&Head, sizeof (Head));
	Head.byCmd		= CMD_PUZZLESTATE;
	Head.byKind		= byKind;
	Head.bySeat		= (BYTE)nSeat;
	Head.bySide		= (BYTE)SeatSide (Table.Param, nSeat);
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
	if (pNpc) {
		PuzzleFieldPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.Param.nDx, Table.Param.nDy, &nX, &nY);
		Head.sFieldX = (short)nX;
		Head.sFieldY = (short)nY;
	}
	if (!bBoard) {
		SendCmdPUZZLESTATE (pChar->pID, (PBYTE)&Head, sizeof (Head));
		return;
	}
	pBuf = new BYTE[sizeof (Head) + nCells];
	memcpy (pBuf, &Head, sizeof (Head));
	memcpy (pBuf + sizeof (Head), g.Board.abyCell, nCells);
	SendCmdPUZZLESTATE (pChar->pID, pBuf, sizeof (Head) + nCells);
	delete [] pBuf;
}

/* 席を立たせる。bNotify なら本人に片付けを知らせる（切断では送らない） */
static void Stand(int nTable, int nSeat, BOOL bNotify, BYTE byKind)
{
	PUZZLESEAT &Seat = s_aTable[nTable].aSeat[nSeat];
	PCHARINFO pChar = SeatChar (Seat);

	if (bNotify && pChar) {
		SendState (s_aTable[nTable], nSeat, pChar, byKind);
	}
	Seat.dwChar = PUZZLE_NOCHAR;
	Seat.dwPID = 0;
	CleanTable (nTable);
}

/* ゲームオーバー：得点を知らせて席を立つ（最後の盤面は OVER で送って、クライアントがしばらく見せる） */
static void GameOver(int nTable, int nSeat)
{
	PUZZLESEAT &Seat = s_aTable[nTable].aSeat[nSeat];
	PCHARINFO pChar = SeatChar (Seat);
	char szMsg[128];

	if (pChar) {
		sprintf (szMsg, "ゲームオーバー！ 得点 %u（%dライン・レベル%d）", Seat.Game.dwScore, Seat.Game.nLines, Seat.Game.nLevel);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}
	Stand (nTable, nSeat, TRUE, PUZZLESTATE_OVER);
}

/* 入力・周期処理の結果を送る（変わった所だけ） */
static void SendChanged(int nTable, int nSeat, int nChg)
{
	PUZZLESEAT &Seat = s_aTable[nTable].aSeat[nSeat];
	PCHARINFO pChar = SeatChar (Seat);

	if (pChar == NULL) {
		return;
	}
	if (nChg & PUZZLE_CHG_OVER) {
		GameOver (nTable, nSeat);
		return;
	}
	if (nChg & (PUZZLE_CHG_BOARD | PUZZLE_CHG_NEXT | PUZZLE_CHG_SCORE)) {
		SendState (s_aTable[nTable], nSeat, pChar, PUZZLESTATE_FULL);
	} else if (nChg & PUZZLE_CHG_PIECE) {
		SendState (s_aTable[nTable], nSeat, pChar, PUZZLESTATE_MOVE);
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

/* ========================================================================= */
/* 外から呼ぶ関数															 */
/* ========================================================================= */

BOOL PuzzleIsTableNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'p');
}

BOOL PuzzleIsSeated(PCHARINFO pChar)
{
	int nTable, nSeat;

	return FindSeat (pChar, &nTable, &nSeat);
}

int PuzzleSeatCount(void)
{
	int i, j, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		for (j = 0; j < PUZZLE_SEATS; j ++) {
			if (s_aTable[i].aSeat[j].dwChar != PUZZLE_NOCHAR) {
				nRet ++;
			}
		}
	}
	return nRet;
}

void PuzzleSit(PCHARINFO pChar, PCHARINFO pNpc)
{
	PUZZLEPARAM Param;
	int i, j, nTable = -1, nSeat = -1;
	char szMsg[96];

	if (!PuzzleIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || PuzzleIsSeated (pChar)) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}
	ParseParam (pNpc->pszNpcSpeech, &Param);

	/* 台を探す（無ければ作る）。空いている席を取る */
	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwNpc == pNpc->dwTblID) && (s_aTable[i].dwMapID == pNpc->dwMapID)) {
			nTable = i;
			break;
		}
	}
	if (nTable >= 0) {
		for (j = 0; j < PUZZLE_SEATS_USED; j ++) {
			if (s_aTable[nTable].aSeat[j].dwChar == PUZZLE_NOCHAR) {
				nSeat = j;
				break;
			}
		}
		if (nSeat < 0) {
			SendCmdSYSMSG (pChar->pID, "この台は今、使われています。");
			return;
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

	if (nTable < 0) {
		PUZZLETABLE Table;
		ZeroMemory (&Table, sizeof (Table));
		Table.dwNpc = pNpc->dwTblID;
		Table.dwMapID = pNpc->dwMapID;
		for (j = 0; j < PUZZLE_SEATS; j ++) {
			Table.aSeat[j].dwChar = PUZZLE_NOCHAR;
		}
		s_aTable.push_back (Table);
		nTable = (int)s_aTable.size () - 1;
		nSeat = 0;
	}
	s_aTable[nTable].Param = Param;
	PUZZLESEAT &Seat = s_aTable[nTable].aSeat[nSeat];
	Seat.dwChar = pChar->dwTblID;
	Seat.dwPID = pChar->pID;
	Seat.dwLastKey = 0;
	Seat.Game.Start (Param.nCols, Param.nRows, (unsigned int)(g_dwTime * 2654435761u) ^ (pChar->dwTblID * 40503u + 1), g_dwTime);
	SendState (s_aTable[nTable], nSeat, pChar, PUZZLESTATE_FULL);
}

void PuzzleKey(PCHARINFO pChar, BYTE byKey)
{
	int nTable, nSeat, nChg = 0;

	if (!FindSeat (pChar, &nTable, &nSeat)) {
		return;
	}
	PUZZLESEAT &Seat = s_aTable[nTable].aSeat[nSeat];

	if (byKey == PUZZLEKEY_LEAVE) {
		Stand (nTable, nSeat, TRUE, PUZZLESTATE_NONE);
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
	SendChanged (nTable, nSeat, nChg);
}

void PuzzleProc(void)
{
	int i, j;

	if (s_aTable.empty ()) {
		return;
	}
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		PUZZLETABLE &Table = s_aTable[i];
		PCHARINFO pNpc = (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
		BOOL bNpcOK = PuzzleIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);

		for (j = 0; j < PUZZLE_SEATS; j ++) {
			PUZZLESEAT &Seat = Table.aSeat[j];
			PCHARINFO pChar;
			int nChg;

			if (Seat.dwChar == PUZZLE_NOCHAR) {
				continue;
			}
			pChar = SeatChar (Seat);
			/* 切断・マップ移動・席から離れた・倒れた・台が無くなった時は片付ける */
			if ((pChar == NULL) || !bNpcOK || (pChar->byGameState < GAMESTATE_MOVE) ||
				(pChar->dwMapID != Table.dwMapID) ||
				(abs ((int)pChar->dwX - (int)pNpc->dwX) > PUZZLE_NEAR) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > PUZZLE_NEAR) ||
				(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
				/* 台が消えると Table への参照が無効になるので、この台の残りの席は次の周期へ回す */
				Stand (i, j, TRUE, PUZZLESTATE_NONE);
				break;
			}
			nChg = Seat.Game.Tick (g_dwTime);
			if (nChg) {
				int nSize = (int)s_aTable.size ();
				SendChanged (i, j, nChg);
				if (nSize != (int)s_aTable.size ()) {
					break;
				}
			}
		}
	}
}

void PuzzleOnLeave(PCHARINFO pChar)
{
	int nTable, nSeat;

	if (FindSeat (pChar, &nTable, &nSeat)) {
		Stand (nTable, nSeat, FALSE, PUZZLESTATE_NONE);
	}
}
