/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			サブゲーム(すごろく)の土地取りルール（周回型）			 */
/* 作成日：		2026/11/26（段階 1）										 */
/* ========================================================================= */
/*
 * 既存のすごろく（SUBGAME_SUGOROKUINFO。盤＝マップのマス・駒＝実キャラ・参加 60 秒募集・観戦）の
 * 新しいルール種別 SUBGAME_SUGOROKU_RULE_LAND。仕様・遊び方・データ投入手順は docs/land-sugoroku.md。
 *
 * ルール（お金・土地・手番・勝敗）は common/LandSugoroku.h の LandGame が持ち、ここは
 *   - 駒（実キャラ）を盤のコマへ歩かせる（1 コマ＝マップ 3×3 マス。コマの中央から次のコマの中央まで、普通の歩き速度で 3 歩。1 歩 LAND_WALK_MS）
 *   - 起きたこと（LandGame の aEvent）をシステムメッセージと CMD_LANDSTATE にする
 *   - 手番の制限時間（30 秒で自動サイコロ・購入は「やめる」）を見張る
 *   - 切断・退出・マップ移動で脱落させる
 * を受け持つ。既存の状態機械（SboSvrSubGameProc.cpp の SubGameProc_SUGOROKU）は、土地取りの時だけ
 * ゲーム開始後の処理をここ（SubLand_Proc）に渡す。
 *
 * 盤のマップ座標は、すごろくの「スタート」イベント（管理画面で置く START イベント）の座標＝盤の左上。
 * 盤は 8×8 コマの外周の輪（コマ 0=スタート、7=右上、14=右下、21=左下）で、1 コマは 3×3 マスなので全体は 24×24 マス。
 * コマの代表位置は 3×3 の中央のマス。駒は通過中は中央を通って歩き、同じコマに複数人が止まる時だけ、
 * 後から止まった人が空いている四隅へ寄る（LandStandSlot）。
 * 土地表は標準の表（LandBoard::SetDefault）。SBODATA/SboDataLandBoard.tsv があれば、ゲームの開始ごとに読んで差し替える。
 *
 * 脱落（通行料が払えない）した人は、受付の位置へ戻って観戦者になり、最後まで盤の状態を見られる。
 * 切断・退出・マップ移動の人は既存のすごろくと同じく受付へ戻る（SubSugoroku_RemoveAndMove）。
 * 終わったら結果を LAND_RESULT_MS 見せてから、全員を受付へ戻す（SubSugoroku_Quit）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSubGameProcSugoroku.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define LAND_RESULT_MS		(8000)				/* 終わってから全員を受付へ戻すまで（結果を読む時間）(ms) */
#define LAND_BOARD_FILE		"SboDataLandBoard.tsv"	/* 土地表の差し替えファイル（SBODATA の中） */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */
/* 土地取りの進行（SUBGAME_SUGOROKUINFO::pLand。ゲームが始まってから終わるまでだけ持つ） */
typedef struct _LANDSVR {
	LandGame	Game;												/* ルール */
	DWORD		adwTbl[LAND_MAX_PLAYERS];							/* 参加者番号 → キャラのテーブルID */
	char		aszName[LAND_MAX_PLAYERS][LANDSTATE_NAME_LEN];		/* 参加者の名前（抜けた後も結果に使う） */
	DWORD		dwLastStep;											/* 最後に駒を進めた時刻 */
	DWORD		dwFinishTime;										/* 終わった時刻 */
	BOOL		bPlaying;											/* 遊んでいるか（終わったら FALSE。結果を見せる間は pLand だけ残る） */
	DWORD		dwOriginX;											/* 盤の左上のマップ座標 */
	DWORD		dwOriginY;
	BOOL		bLeg;												/* 手番の駒が今コマ 1 つぶん（3 歩）歩いている途中か */
	int			anTileX[LAND_MAX_PLAYERS];							/* 参加者ごとの駒のマップ座標（サーバーが決めた立ち位置） */
	int			anTileY[LAND_MAX_PLAYERS];
	DWORD		adwWalkNext[LAND_MAX_PLAYERS];						/* 次の 1 歩を送ってよい時刻 */
	std::vector<int>	aWalkX[LAND_MAX_PLAYERS];					/* これから歩くマス（先頭から 1 歩ずつ） */
	std::vector<int>	aWalkY[LAND_MAX_PLAYERS];
} LANDSVR, *PLANDSVR;

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static int LandIdx(PLANDSVR pL, DWORD dwTblID);												/* テーブルID → 参加者番号（いなければ -1） */
static void LandCellPos(PLANDSVR pL, int nCell, int nSlot, DWORD *pdwX, DWORD *pdwY);		/* コマ → マップ座標（nSlot<0 は中央、0〜3 は四隅） */
static int LandStandSlot(PLANDSVR pL, int nIdx, int nCell);									/* 止まった時の立ち位置（-1=中央 0〜3=四隅） */
static void LandPathAdd(PLANDSVR pL, int nIdx, int nToX, int nToY);							/* 今の位置から目的のマスまで（横→縦）の歩きを足す */
static void LandPump(PLANDSVR pL, DWORD dwTime);
static void LandBeginLeg(PLANDSVR pL, int nTurn, DWORD dwTime);								/* 次のコマへの 3 歩を予定に入れる */											/* 歩く予定のマスを、時刻が来た人から 1 歩ずつ進める */
static void LandBuildState(PSUBGAME_SUGOROKUINFO pSugoroku, PLANDSVR pL, int nMe, CMDPARA_LANDSTATE *pSt);	/* 通信の構造体を作る */
static void LandBroadcast(PSUBGAME_SUGOROKUINFO pSugoroku);									/* 遊んでいる人・観戦者全員へ盤の状態を送る */
static void LandWalkTile(PLANDSVR pL, int nIdx, int nX, int nY);							/* 駒を隣のマップのマスへ 1 歩歩かせる */
static void LandJump(PLANDSVR pL, int nIdx, int nCell, int nSlot);							/* 駒を指定のコマへフェードで移す */

#ifdef SBO_LAND_TEST
/* ========================================================================= */
/* テスト用の仕掛け（-DSBO_LAND_TEST でビルドした時だけ有効。本番のビルドには入らない）	 */
/*		 データフォルダの LandTest.txt を読む（無ければ普通の乱数・設定）。行の書き方:		 */
/*		   D 出目1 出目2   … サイコロを振るたびに先頭の 1 行を使って消す（足りなければ乱数）	 */
/*		   LAPS n          … 終了する周回数をゲーム開始時に n にする						 */
/*		   GOLD 番号 G     … ゲーム開始時に、その番号の人の所持金を G にする				 */
/* ========================================================================= */
static void LandTestRead(std::vector<std::string> &aLine)
{
	FILE *fp = fopen ("LandTest.txt", "rb");
	char szBuf[256];

	if (fp == NULL) {
		return;
	}
	while (fgets (szBuf, sizeof (szBuf), fp)) {
		std::string str = szBuf;

		while (!str.empty () && ((str[str.size () - 1] == '\n') || (str[str.size () - 1] == '\r'))) {
			str.erase (str.size () - 1);
		}
		if (!str.empty ()) {
			aLine.push_back (str);
		}
	}
	fclose (fp);
}

static void LandTestDice(int *pn1, int *pn2)
{
	std::vector<std::string> aLine;
	size_t i;

	*pn1 = (int)(genrand () % 6 + 1);
	*pn2 = (int)(genrand () % 6 + 1);
	LandTestRead (aLine);
	for (i = 0; i < aLine.size (); i ++) {
		int a, b;

		if (sscanf (aLine[i].c_str (), "D %d %d", &a, &b) == 2) {
			FILE *fp;
			size_t j;

			*pn1 = a;
			*pn2 = b;
			fp = fopen ("LandTest.txt", "wb");
			if (fp) {
				for (j = 0; j < aLine.size (); j ++) {
					if (j != i) {
						fprintf (fp, "%s\n", aLine[j].c_str ());
					}
				}
				fclose (fp);
			}
			return;
		}
	}
}

static void LandTestSetup(LandGame *pG)
{
	std::vector<std::string> aLine;
	size_t i;

	LandTestRead (aLine);
	for (i = 0; i < aLine.size (); i ++) {
		int a, b;

		if (sscanf (aLine[i].c_str (), "LAPS %d", &a) == 1) {
			pG->nMaxLaps = a;
		} else if (sscanf (aLine[i].c_str (), "GOLD %d %d", &a, &b) == 2) {
			if ((a >= 0) && (a < LAND_MAX_PLAYERS)) {
				pG->aPlayer[a].nGold = b;
			}
		}
	}
}
#define LAND_DICE(n1, n2)	LandTestDice (&(n1), &(n2))
#else
#define LAND_DICE(n1, n2)	do { (n1) = (int)(genrand () % 6 + 1); (n2) = (int)(genrand () % 6 + 1); } while (0)
#endif
static void LandMakeViewer(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID);					/* 脱落した人を観戦者にする */
static void LandDrain(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTime);						/* 起きたことをメッセージ・通信にする */
static void LandLoadBoard(LandBoard *pBoard);												/* 土地表を用意（差し替えファイルがあれば読む） */

/* ========================================================================= */
/* 関数名：LandIdx															 */
/* ========================================================================= */
static int LandIdx(PLANDSVR pL, DWORD dwTblID)
{
	int i;

	for (i = 0; i < pL->Game.nPlayers; i ++) {
		if (pL->adwTbl[i] == dwTblID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：LandCellPos														 */
/* 内容：コマ → マップ座標（盤の左上＝スタートの座標＋コマの格子位置×3＋コマの中の位置）	 */
/*		 nSlot < 0 はコマの中央（代表位置）、0〜3 は四隅（左上・右上・左下・右下）	 */
/* ========================================================================= */
static void LandCellPos(PLANDSVR pL, int nCell, int nSlot, DWORD *pdwX, DWORD *pdwY)
{
	int nDx, nDy;

	LandCellTile (nCell, nSlot, &nDx, &nDy);
	*pdwX = pL->dwOriginX + nDx;
	*pdwY = pL->dwOriginY + nDy;
}

/* ========================================================================= */
/* 関数名：LandStandSlot													 */
/* 内容：nIdx が nCell に止まった時の立ち位置。ほかに誰もいなければ中央（-1）。	 */
/*		 同じコマに生きている人がいれば、その人たちのいないマスの四隅を割り当てる	 */
/* ========================================================================= */
static int LandStandSlot(PLANDSVR pL, int nIdx, int nCell)
{
	int i, nSlot, nX, nY, nOthers = 0;

	for (i = 0; i < pL->Game.nPlayers; i ++) {
		if ((i != nIdx) && pL->Game.aPlayer[i].bAlive && (pL->Game.aPlayer[i].nPos == nCell)) {
			nOthers ++;
		}
	}
	if (nOthers == 0) {
		return -1;
	}
	for (nSlot = 0; nSlot < 4; nSlot ++) {
		BOOL bUsed = FALSE;

		LandCellTile (nCell, nSlot, &nX, &nY);
		for (i = 0; i < pL->Game.nPlayers; i ++) {
			if ((i != nIdx) && pL->Game.aPlayer[i].bAlive && (pL->Game.aPlayer[i].nPos == nCell) &&
				(pL->anTileX[i] == (int)pL->dwOriginX + nX) && (pL->anTileY[i] == (int)pL->dwOriginY + nY)) {
				bUsed = TRUE;
			}
		}
		if (!bUsed) {
			return nSlot;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：LandPathAdd														 */
/* 内容：駒 nIdx の予定の最後の位置から (nToX, nToY) まで、横→縦の順に 1 マスずつの歩きを足す	 */
/* ========================================================================= */
static void LandPathAdd(PLANDSVR pL, int nIdx, int nToX, int nToY)
{
	int x = pL->anTileX[nIdx], y = pL->anTileY[nIdx];

	if (!pL->aWalkX[nIdx].empty ()) {
		x = pL->aWalkX[nIdx].back ();
		y = pL->aWalkY[nIdx].back ();
	}
	while (x != nToX) {
		x += (nToX > x) ? 1 : -1;
		pL->aWalkX[nIdx].push_back (x);
		pL->aWalkY[nIdx].push_back (y);
	}
	while (y != nToY) {
		y += (nToY > y) ? 1 : -1;
		pL->aWalkX[nIdx].push_back (x);
		pL->aWalkY[nIdx].push_back (y);
	}
}

/* ========================================================================= */
/* 関数名：LandPump															 */
/* 内容：歩く予定のある駒を、LAND_WALK_MS ごとに 1 マスずつ進める				 */
/* ========================================================================= */
static void LandPump(PLANDSVR pL, DWORD dwTime)
{
	int i;

	for (i = 0; i < pL->Game.nPlayers; i ++) {
		if (pL->aWalkX[i].empty () || ((int)(dwTime - pL->adwWalkNext[i]) < 0)) {
			continue;
		}
		LandWalkTile (pL, i, pL->aWalkX[i].front (), pL->aWalkY[i].front ());
		pL->aWalkX[i].erase (pL->aWalkX[i].begin ());
		pL->aWalkY[i].erase (pL->aWalkY[i].begin ());
		pL->adwWalkNext[i] = dwTime + LAND_WALK_MS;
	}
}

/* ========================================================================= */
/* 関数名：LandBuildState													 */
/* 内容：通信の構造体を作る（nMe = 受け取る人の参加者番号。観戦者は -1）	 */
/* ========================================================================= */
static void LandBuildState(PSUBGAME_SUGOROKUINFO pSugoroku, PLANDSVR pL, int nMe, CMDPARA_LANDSTATE *pSt)
{
	LandGame &G = pL->Game;
	int i;

	ZeroMemory (pSt, sizeof (CMDPARA_LANDSTATE));
	pSt->byCmd = CMD_LANDSTATE;
	pSt->byKind = LANDSTATE_FULL;
	pSt->byPlayers = (BYTE)G.nPlayers;
	pSt->byMe = (nMe < 0) ? LANDSTATE_ME_VIEWER : (BYTE)nMe;
	pSt->byTurn = (BYTE)G.nTurn;
	pSt->byPhase = (BYTE)G.nPhase;
	pSt->byDice1 = (BYTE)G.nDice1;
	pSt->byDice2 = (BYTE)G.nDice2;
	pSt->byWinMask = (BYTE)G.nWinMask;
	pSt->byOverReason = (BYTE)G.nOverReason;
	pSt->wOriginX = (WORD)pL->dwOriginX;
	pSt->wOriginY = (WORD)pL->dwOriginY;
	pSt->wSecLeft = (WORD)G.SecondsLeft (timeGetTime ());
	pSt->wRound = (WORD)(G.nRound + 1);
	pSt->wMaxLaps = (WORD)G.nMaxLaps;
	/* 購入の問い合わせは、手番の本人にだけ付ける */
	if ((G.nPhase == LANDPHASE_BUY) && (nMe >= 0) && (nMe == G.nTurn) && (G.nAskCell >= 0)) {
		const LandCellDef &C = G.Board.aCell[G.nAskCell];
		pSt->byAsk = 1;
		pSt->byAskCell = (BYTE)G.nAskCell;
		pSt->wAskPrice = (WORD)C.nPrice;
		SboTextCopy (pSt->szAskName, sizeof (pSt->szAskName), C.szName);
	}
	for (i = 0; i < G.nPlayers; i ++) {
		CMDPARA_LANDSTATE_PLAYER &P = pSt->aPlayer[i];
		SboTextCopy (P.szName, sizeof (P.szName), pL->aszName[i]);
		P.nGold = G.aPlayer[i].nGold;
		P.nAssets = G.Assets (i);
		P.byLaps = (BYTE)G.aPlayer[i].nLaps;
		P.byAlive = G.aPlayer[i].bAlive ? 1 : 0;
		P.bySkip = (BYTE)G.aPlayer[i].nSkip;
		P.byLands = (BYTE)G.OwnedCount (i);
		P.byPos = (BYTE)G.aPlayer[i].nPos;
	}
	for (i = 0; i < LAND_CELLS; i ++) {
		const LandCellDef &C = G.Board.aCell[i];
		pSt->aCell[i].byKind = (BYTE)C.nKind;
		pSt->aCell[i].byOwner = (G.anOwner[i] < 0) ? LANDSTATE_OWNER_NONE : (BYTE)G.anOwner[i];
		pSt->aCell[i].wPrice = (WORD)C.nPrice;
		SboTextCopy (pSt->aCell[i].szShort, sizeof (pSt->aCell[i].szShort), C.szShort);
		SboTextCopy (pSt->aCell[i].szName, sizeof (pSt->aCell[i].szName), C.szName);
	}
}

/* ========================================================================= */
/* 関数名：SubLand_SendTo													 */
/* 内容：1 人へ盤の状態を送る（観戦に途中参加した人・状態が変わった時）		 */
/* ========================================================================= */
void SubLand_SendTo(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	PCHARINFO pChar;
	CMDPARA_LANDSTATE St;

	if ((pL == NULL) || (dwTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return;
	}
	pChar = g_aCharInfo[dwTblID];
	if (pChar == NULL) {
		return;
	}
	LandBuildState (pSugoroku, pL, LandIdx (pL, dwTblID), &St);
	SendCmdLANDSTATE (pChar->pID, &St);
}

/* ========================================================================= */
/* 関数名：SubLand_SendNone													 */
/* 内容：盤の重ね描きを消させる（ゲームを抜けた・終わった）					 */
/* ========================================================================= */
void SubLand_SendNone(PCHARINFO pChar)
{
	CMDPARA_LANDSTATE St;

	ZeroMemory (&St, sizeof (St));
	St.byCmd = CMD_LANDSTATE;
	St.byKind = LANDSTATE_NONE;
	SendCmdLANDSTATE (pChar->pID, &St);
}

/* ========================================================================= */
/* 関数名：LandBroadcast													 */
/* ========================================================================= */
static void LandBroadcast(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	int i, nCount;

	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SubLand_SendTo (pSugoroku, pSugoroku->adwPlayerTblID[i]);
	}
	nCount = pSugoroku->adwViewerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SubLand_SendTo (pSugoroku, pSugoroku->adwViewerTblID[i]);
	}
}

/* ========================================================================= */
/* 関数名：LandWalkTile														 */
/* 内容：駒を隣のマップのマスへ 1 歩歩かせる（既存のすごろく SubSugoroku_Move と同じ送り方）	 */
/* ========================================================================= */
static void LandWalkTile(PLANDSVR pL, int nIdx, int nX, int nY)
{
	PCHARINFO pChar = NULL;

	if (pL->adwTbl[nIdx] < (DWORD)g_aCharInfo.GetSize ()) {
		pChar = g_aCharInfo[pL->adwTbl[nIdx]];
	}
	if (pChar != NULL) {
		if (nY < (int)pChar->dwY) {
			pChar->byDirection = 0;
		} else if (nY > (int)pChar->dwY) {
			pChar->byDirection = 1;
		} else if (nX < (int)pChar->dwX) {
			pChar->byDirection = 2;
		} else if (nX > (int)pChar->dwX) {
			pChar->byDirection = 3;
		}
		pChar->dwX = (DWORD)nX;
		pChar->dwY = (DWORD)nY;
		pChar->byFlg |= CHARINFOFLG_MOVE;
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	}
	pL->anTileX[nIdx] = nX;
	pL->anTileY[nIdx] = nY;
}

/* ========================================================================= */
/* 関数名：LandJump															 */
/* 内容：駒を指定のコマ（nSlot<0 は中央、0〜3 は四隅）へフェードで移す（開始時・迷いの森で戻る時。	 */
/*		 SubSugoroku_CheckParts のスタートに戻ると同じ送り方）。歩く予定は捨てる	 */
/* ========================================================================= */
static void LandJump(PLANDSVR pL, int nIdx, int nCell, int nSlot)
{
	PCHARINFO pChar;
	DWORD dwX, dwY;

	LandCellPos (pL, nCell, nSlot, &dwX, &dwY);
	pL->anTileX[nIdx] = (int)dwX;
	pL->anTileY[nIdx] = (int)dwY;
	pL->aWalkX[nIdx].clear ();
	pL->aWalkY[nIdx].clear ();
	if (pL->adwTbl[nIdx] >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pChar = g_aCharInfo[pL->adwTbl[nIdx]];
	if (pChar == NULL) {
		return;
	}
	SendCmdFADEINOUT (pChar->pID, 0);
	pChar->byDirectionBack2 = pChar->byDirection;
	pChar->byDirectionBack = pChar->byDirection;
	CharTargetCancelPC (pChar);
	pChar->dwX = dwX;
	pChar->dwY = dwY;
	pChar->dwXBack = pChar->dwX;
	pChar->dwYBack = pChar->dwY;
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
	SendCmdFADEINOUT (pChar->pID, 1);
}

/* ========================================================================= */
/* 関数名：LandMakeViewer													 */
/* 内容：脱落した人を観戦者にする（受付へ戻して、盤の状態は引き続き届く）	 */
/* ========================================================================= */
static void LandMakeViewer(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	int i, nCount;
	PCHARINFO pChar;
	PSUBSTATUS_SUGOROKU pSubStatus;

	if (SubSugoroku_CheckJoin (pSugoroku, dwTblID) != 0) {
		return;
	}
	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	for (i = 0; i < nCount; i ++) {
		if (pSugoroku->adwPlayerTblID[i] == dwTblID) {
			pSugoroku->adwPlayerTblID.RemoveAt (i);
			pSugoroku->aPlayerInfo.RemoveAt (i);
			break;
		}
	}
	pSugoroku->adwViewerTblID.Add (dwTblID);
	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pChar = g_aCharInfo[dwTblID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
	pSubStatus->byFlgViewer = 1;
	pSubStatus->byState = SUBGAME_SUGOROKU_STATE_IDLE;
	SendCmdSUBSTATUS (pChar->dwTblID);
	SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);
	SubProcMove (pChar, (DWORD)pSugoroku->byX, (DWORD)pSugoroku->byY);
}

/* ========================================================================= */
/* 関数名：LandLoadBoard													 */
/* 内容：土地表を用意（標準。SBODATA/SboDataLandBoard.tsv があれば上書き。壊れていれば標準に戻す）	 */
/* ========================================================================= */
static void LandLoadBoard(LandBoard *pBoard)
{
	char szFile[MAX_PATH + 1];
	FILE *fp;
	std::string strText, strWhy;
	char szBuf[1024];
	size_t nRead;
	int i;

	pBoard->SetDefault ();

	GetModuleFileName (NULL, szFile, MAX_PATH);
	for (i = (int)strlen (szFile) - 1; i > 0; i --) {
		if (szFile[i] == '\\') {
			szFile[i + 1] = 0;
			break;
		}
	}
	strcat (szFile, "SBODATA/" LAND_BOARD_FILE);
	fp = fopen (szFile, "rb");
	if (fp == NULL) {
		return;
	}
	while ((nRead = fread (szBuf, 1, sizeof (szBuf), fp)) > 0) {
		strText.append (szBuf, nRead);
	}
	fclose (fp);
	/* UTF-8 の BOM は読み飛ばす */
	if ((strText.size () >= 3) && ((unsigned char)strText[0] == 0xEF) && ((unsigned char)strText[1] == 0xBB) && ((unsigned char)strText[2] == 0xBF)) {
		strText.erase (0, 3);
	}
	if (!pBoard->LoadTsv (strText.c_str (), &strWhy) || !pBoard->Validate (&strWhy)) {
		printf ("[land] %s が使えません（%s）。標準の土地表で遊びます\n", LAND_BOARD_FILE, strWhy.c_str ());
		pBoard->SetDefault ();
	}
}

/* ========================================================================= */
/* 関数名：SubLand_Start													 */
/* 内容：土地取りを始める（参加者募集が終わった時。参加者は 2 人以上）		 */
/* ========================================================================= */
void SubLand_Start(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	PLANDSVR pL;
	char szMsg[256];
	DWORD dwTime;
	int i, nCount;

	SubLand_Release (pSugoroku);
	pL = new LANDSVR;
	ZeroMemory (pL->adwTbl, sizeof (pL->adwTbl));
	ZeroMemory (pL->aszName, sizeof (pL->aszName));
	pSugoroku->pLand = pL;

	dwTime = timeGetTime ();
	pL->dwLastStep = dwTime;
	pL->dwFinishTime = 0;
	pL->bPlaying = TRUE;
	pL->dwOriginX = pSugoroku->dwStartX;
	pL->dwOriginY = pSugoroku->dwStartY;
	pL->bLeg = FALSE;
	for (i = 0; i < LAND_MAX_PLAYERS; i ++) {
		pL->anTileX[i] = (int)pL->dwOriginX;
		pL->anTileY[i] = (int)pL->dwOriginY;
		pL->adwWalkNext[i] = dwTime;
	}

	nCount = pSugoroku->adwPlayerTblID.GetSize ();
	if (nCount > LAND_MAX_PLAYERS) {
		nCount = LAND_MAX_PLAYERS;
	}
	for (i = 0; i < nCount; i ++) {
		DWORD dwTblID = pSugoroku->adwPlayerTblID[i];

		pL->adwTbl[i] = dwTblID;
		SboTextCopy (pL->aszName[i], sizeof (pL->aszName[i]), g_aCharInfo[dwTblID]->szName);
	}
	LandLoadBoard (&pL->Game.Board);
	pL->Game.Start (nCount, dwTime);
#ifdef SBO_LAND_TEST
	LandTestSetup (&pL->Game);
#endif
	/* 全員がスタートのコマに立つ。全員同じコマなので四隅に分かれる（参加者の番号順） */
	for (i = 0; i < nCount; i ++) {
		LandJump (pL, i, 0, i);
	}

	wsprintf (szMsg, "土地取りすごろく！お金は%dG。空き地を買って、他人の土地では通行料を払います。", LAND_START_GOLD);
	SubSugoroku_Msg (pSugoroku, szMsg);
	wsprintf (szMsg, "%d周で総資産（現金＋土地）が一番多い人の勝ち。通行料が払えないと脱落です。", pL->Game.nMaxLaps);
	SubSugoroku_Msg (pSugoroku, szMsg);

	LandDrain (pSugoroku, dwTime);
}

/* ========================================================================= */
/* 関数名：SubLand_Stop / SubLand_Release									 */
/* 内容：進行を止める（終了処理の最初）・進行の情報を捨てる（終了処理の最後）	 */
/* ========================================================================= */
void SubLand_Stop(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;

	if (pL) {
		pL->bPlaying = FALSE;
	}
}

void SubLand_Release(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;

	if (pL) {
		pSugoroku->pLand = NULL;
		delete pL;
	}
}

/* ========================================================================= */
/* 関数名：SubLand_Active													 */
/* 内容：土地取りの進行を持っているか（RemoveAndMove の「2 人未満なら終了」を土地取りでは使わない判定）	 */
/* ========================================================================= */
BOOL SubLand_Active(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	return (pSugoroku->byRule == SUBGAME_SUGOROKU_RULE_LAND) && (pSugoroku->pLand != NULL);
}

/* ========================================================================= */
/* 関数名：SubLand_GetGame													 */
/* 内容：ルールの状態を返す（テスト・調べる用。進行の情報が無ければ NULL）	 */
/* ========================================================================= */
LandGame *SubLand_GetGame(PSUBGAME_SUGOROKUINFO pSugoroku)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;

	return pL ? &pL->Game : NULL;
}

/* ========================================================================= */
/* 関数名：SubLand_Dice														 */
/* 内容：サイコロを振る（クライアントの要求）								 */
/* ========================================================================= */
void SubLand_Dice(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	int nIdx, nD1, nD2;
	DWORD dwTime = timeGetTime ();

	if ((pL == NULL) || !pL->bPlaying) {
		return;
	}
	nIdx = LandIdx (pL, dwTblID);
	if (nIdx < 0) {
		return;
	}
	LAND_DICE (nD1, nD2);
	if (pL->Game.Roll (nIdx, nD1, nD2, false)) {
		pL->dwLastStep = dwTime;
		LandDrain (pSugoroku, dwTime);
	}
}

/* ========================================================================= */
/* 関数名：SubLand_Buy														 */
/* 内容：空き地を買うかの返事（クライアントの要求。bYes=買う）				 */
/* ========================================================================= */
void SubLand_Buy(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID, BOOL bYes)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	int nIdx;
	DWORD dwTime = timeGetTime ();

	if ((pL == NULL) || !pL->bPlaying) {
		return;
	}
	nIdx = LandIdx (pL, dwTblID);
	if (nIdx < 0) {
		return;
	}
	if (pL->Game.Buy (nIdx, bYes ? true : false, dwTime)) {
		LandDrain (pSugoroku, dwTime);
	}
}

/* ========================================================================= */
/* 関数名：SubLand_OnLeave													 */
/* 内容：参加者が抜けた（切断・退出・マップ移動）。その場で脱落（土地は銀行へ）。	 */
/*		 SubSugoroku_Remove が参加者の表から外した後に呼ぶ					 */
/* ========================================================================= */
void SubLand_OnLeave(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTblID)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	int nIdx;
	DWORD dwTime = timeGetTime ();

	if ((pL == NULL) || !pL->bPlaying) {
		return;
	}
	nIdx = LandIdx (pL, dwTblID);
	if ((nIdx < 0) || !pL->Game.aPlayer[nIdx].bAlive) {
		return;
	}
	pL->Game.Eliminate (nIdx, LANDOUT_LEFT, dwTime);
	LandDrain (pSugoroku, dwTime);
}

/* ========================================================================= */
/* 関数名：LandBeginLeg														 */
/* 内容：手番の駒が次のコマの中央まで歩く 3 歩（四隅にいる時は先に中央へ）を予定に入れ、最初の 1 歩を送る	 */
/* ========================================================================= */
static void LandBeginLeg(PLANDSVR pL, int nTurn, DWORD dwTime)
{
	LandGame *pG = &pL->Game;
	PCHARINFO pChar = (pL->adwTbl[nTurn] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[pL->adwTbl[nTurn]] : NULL;
	int nCur = pG->aPlayer[nTurn].nPos, nNext = (nCur + 1) % LAND_CELLS;
	DWORD dwX, dwY, dwNX, dwNY;

	if ((pL->aWalkX[nTurn].size () != 0) || ((int)(dwTime - pL->adwWalkNext[nTurn]) < 0)) {
		return;							/* 前のコマの四隅への寄りがまだ終わっていない */
	}
	if (pChar != NULL) {
		pL->anTileX[nTurn] = (int)pChar->dwX;
		pL->anTileY[nTurn] = (int)pChar->dwY;
	}
	LandCellPos (pL, nCur, -1, &dwX, &dwY);
	LandCellPos (pL, nNext, -1, &dwNX, &dwNY);
	if ((abs (pL->anTileX[nTurn] - (int)dwX) > LAND_CELL_TILES) || (abs (pL->anTileY[nTurn] - (int)dwY) > LAND_CELL_TILES)) {
		/* 立ち位置がコマから大きくずれている（想定外）：中央へ移してから歩く */
		LandJump (pL, nTurn, nCur, -1);
	}
	LandPathAdd (pL, nTurn, (int)dwX, (int)dwY);		/* 四隅にいたら、まず中央へ */
	LandPathAdd (pL, nTurn, (int)dwNX, (int)dwNY);		/* 中央から次のコマの中央へ（角のコマでも中央を通るので正しく曲がる） */
	pL->bLeg = TRUE;
	LandPump (pL, dwTime);
}

/* ========================================================================= */
/* 関数名：SubLand_Proc														 */
/* 内容：ゲーム開始後の処理（SubGameProc_SUGOROKU から、土地取りで COMMAND 以降の状態の時に呼ばれる）	 */
/*		 制限時間の見張り・駒を 1 マスずつ進める・終わった後の片付け			 */
/* ========================================================================= */
void SubLand_Proc(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTime)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	LandGame *pG;
	int nTurn;

	if (pL == NULL) {
		/* 進行の情報が無い（管理画面で設定を変えられた等）：終わらせて受付へ戻す */
		SubSugoroku_Quit (pSugoroku);
		return;
	}
	pG = &pL->Game;

	/* 終わった：結果を見せてから全員を受付へ戻す */
	if (!pL->bPlaying) {
		if ((int)(dwTime - pL->dwFinishTime) >= LAND_RESULT_MS) {
			SubSugoroku_Quit (pSugoroku);
		}
		return;
	}

	nTurn = pG->nTurn;
	if (pG->nPhase != LANDPHASE_MOVE) {
		pL->bLeg = FALSE;
	}
	LandPump (pL, dwTime);
	switch (pG->nPhase) {
	case LANDPHASE_ROLL:
		/* 制限時間切れ：自動でサイコロを振る */
		if (pG->TimeoutAction (dwTime) == LANDTIMEOUT_ROLL) {
			int nD1, nD2;

			LAND_DICE (nD1, nD2);
			if (pG->Roll (nTurn, nD1, nD2, true)) {
				pL->dwLastStep = dwTime;
				LandDrain (pSugoroku, dwTime);
			}
		}
		break;

	case LANDPHASE_MOVE:
		/* 駒を 1 コマずつ進める。コマ 1 つは、中央から次のコマの中央まで 3 歩（LandPump が 1 歩ずつ送る）。
		   3 歩目の分の時間が過ぎてから LandGame::Step を呼び、そのコマの処理（購入・通行料・四隅）を行う。
		   移動中は制限時間を見ない（最大 12 コマ＝36 歩＝約 11 秒。サイコロ・購入の制限時間はそのあとから数え直される） */
		if (!pL->bLeg) {
			LandBeginLeg (pL, nTurn, dwTime);
			break;
		}
		if ((pL->aWalkX[nTurn].size () != 0) || ((int)(dwTime - pL->adwWalkNext[nTurn]) < 0)) {
			break;
		}
		pL->bLeg = FALSE;
		pL->dwLastStep = dwTime;
		pG->Step (dwTime);
		/* 最後のコマに着いたら、同じコマに他の人がいる時は空いている四隅へ寄る（迷いの森で戻される時は寄らない） */
		{
			int i, nArrive = -1;
			BOOL bForest = FALSE;

			for (i = 0; i < (int)pG->aEvent.size (); i ++) {
				if (pG->aEvent[i].nType == LANDEV_ARRIVE) {
					nArrive = pG->aEvent[i].nCell;
				} else if (pG->aEvent[i].nType == LANDEV_FOREST) {
					bForest = TRUE;
				}
			}
			if ((nArrive >= 0) && !bForest && pG->aPlayer[nTurn].bAlive) {
				int nSlot = LandStandSlot (pL, nTurn, nArrive);

				if (nSlot >= 0) {
					DWORD dwX, dwY;

					LandCellPos (pL, nArrive, nSlot, &dwX, &dwY);
					LandPathAdd (pL, nTurn, (int)dwX, (int)dwY);
				}
			}
		}
		LandDrain (pSugoroku, dwTime);
		/* まだ歩く歩数があれば、間をあけずに次のコマの最初の 1 歩へ（止まらず続けて歩いて見える） */
		if (pG->nPhase == LANDPHASE_MOVE) {
			LandBeginLeg (pL, pG->nTurn, dwTime);
		}
		break;

	case LANDPHASE_BUY:
		/* 制限時間切れ：「やめる」にする */
		if (pG->TimeoutAction (dwTime) == LANDTIMEOUT_DECLINE) {
			if (pG->Buy (nTurn, false, dwTime, true)) {
				LandDrain (pSugoroku, dwTime);
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：LandDrain														 */
/* 内容：LandGame に積まれた「起きたこと」をメッセージにして全員へ送り、	 */
/*		 盤の状態（CMD_LANDSTATE）を送り直し、既存の状態機械の状態を合わせる	 */
/* ========================================================================= */
static void LandDrain(PSUBGAME_SUGOROKUINFO pSugoroku, DWORD dwTime)
{
	PLANDSVR pL = (PLANDSVR)pSugoroku->pLand;
	LandGame &G = pL->Game;
	std::vector<LandEvent> aEvent;
	char szMsg[512];
	int i, j;

	aEvent = G.aEvent;
	G.ClearEvents ();
	if (aEvent.empty () && pL->bPlaying) {
		return;
	}

	for (i = 0; i < (int)aEvent.size (); i ++) {
		const LandEvent &E = aEvent[i];
		const char *pszWho = (E.nPlayer >= 0) ? pL->aszName[E.nPlayer] : "";
		const char *pszCell = G.Board.aCell[(E.nCell >= 0) && (E.nCell < LAND_CELLS) ? E.nCell : 0].szName;

		szMsg[0] = 0;
		switch (E.nType) {
		case LANDEV_TURN:
			wsprintf (szMsg, "%sの番です。（%d秒以内にサイコロを振ってください）", pszWho, LAND_TURN_SEC);
			/* 本人のクライアントに「行動選択」を知らせる（コマンドを開くとサイコロ） */
			if (pL->adwTbl[E.nPlayer] < (DWORD)g_aCharInfo.GetSize ()) {
				SendCmdSubSugorokuCHGSTATE (g_aCharInfo[pL->adwTbl[E.nPlayer]]->pID, SUBGAME_SUGOROKU_STATE_COMMAND);
			}
			break;
		case LANDEV_SKIP:
			wsprintf (szMsg, "%sは休みです。", pszWho);
			break;
		case LANDEV_ROLL:
			if (E.nCell) {
				wsprintf (szMsg, "%sのサイコロ！[%d][%d]で%dが出ました。（時間切れで自動）", pszWho, E.nOther / 10, E.nOther % 10, E.nValue);
				/* 自動で振った人のクライアントは、まだ「行動選択」のままなので戻す */
				if (pL->adwTbl[E.nPlayer] < (DWORD)g_aCharInfo.GetSize ()) {
					SendCmdSubSugorokuCHGSTATE (g_aCharInfo[pL->adwTbl[E.nPlayer]]->pID, SUBGAME_SUGOROKU_STATE_IDLE);
				}
			} else {
				wsprintf (szMsg, "%sのサイコロ！[%d][%d]で%dが出ました。", pszWho, E.nOther / 10, E.nOther % 10, E.nValue);
			}
			break;
		case LANDEV_ARRIVE:
			/* 止まったマスの名前を全員へ。町のマスは、続く「空き地です」「自分の土地」「〜の土地。通行料」が
			   土地名を出すのでここでは出さない（重複を避ける）。四隅・渡し舟・酒場・迷いの森は名前が出ないので出す */
			if (G.Board.aCell[(E.nCell >= 0) && (E.nCell < LAND_CELLS) ? E.nCell : 0].nKind != LANDCELL_TOWN) {
				wsprintf (szMsg, "%sは「%s」に着きました。", pszWho, pszCell);
			}
			break;
		case LANDEV_BONUS:
			wsprintf (szMsg, "%sは%d周目。周回ボーナス%dGをもらいました。", pszWho, E.nOther, E.nValue);
			break;
		case LANDEV_OWN:
			wsprintf (szMsg, "%sは自分の土地「%s」に止まりました。", pszWho, pszCell);
			break;
		case LANDEV_ASK:
			wsprintf (szMsg, "「%s」は空き地です。%sは%dGで買いますか？", pszCell, pszWho, E.nValue);
			break;
		case LANDEV_CANTBUY:
			wsprintf (szMsg, "「%s」は空き地ですが、%sはお金が足りません。（%dG）", pszCell, pszWho, E.nValue);
			break;
		case LANDEV_BOUGHT:
			wsprintf (szMsg, "%sは「%s」を%dGで買いました。", pszWho, pszCell, E.nValue);
			break;
		case LANDEV_DECLINE:
			wsprintf (szMsg, "%sは「%s」を買いませんでした。%s", pszWho, pszCell, E.nOther ? "（時間切れ）" : "");
			break;
		case LANDEV_TOLL:
			if (E.nValue > 0) {
				wsprintf (szMsg, "「%s」は%sの土地。%sは通行料%dGを払いました。", pszCell, pL->aszName[E.nOther], pszWho, E.nValue);
			} else {
				wsprintf (szMsg, "「%s」は%sの土地。%sは通行料を払えません！", pszCell, pL->aszName[E.nOther], pszWho);
			}
			break;
		case LANDEV_FERRY:
			wsprintf (szMsg, "%sは渡し舟を待ちます。次は1回休みです。", pszWho);
			break;
		case LANDEV_TAVERN:
			wsprintf (szMsg, "%sはギルド酒場で一休みしました。", pszWho);
			break;
		case LANDEV_FOREST:
			wsprintf (szMsg, "%sは迷いの森で迷い、%dマス戻りました。", pszWho, E.nValue);
			LandJump (pL, E.nPlayer, E.nCell, LandStandSlot (pL, E.nPlayer, E.nCell));
			break;
		case LANDEV_OUT:
			/* 歩く予定は捨てる（受付へ戻される人の駒を盤へ歩かせない） */
			pL->aWalkX[E.nPlayer].clear ();
			pL->aWalkY[E.nPlayer].clear ();
			if (E.nOther == LANDOUT_BANKRUPT) {
				wsprintf (szMsg, "%sは破産して脱落しました。（土地%d件は銀行へ）", pszWho, E.nValue);
				/* 受付へ戻して観戦者にする（盤の状態は引き続き届く） */
				LandMakeViewer (pSugoroku, pL->adwTbl[E.nPlayer]);
			}
			/* 抜けた人は SubSugoroku_RemoveAndMove が「ゲームを抜けました。」を出す */
			break;
		case LANDEV_OVER:
			if (E.nValue == LANDOVER_LAPS) {
				wsprintf (szMsg, "%d周に達したので終了です。総資産（現金＋土地）を数えます。", G.nMaxLaps);
				SubSugoroku_Msg (pSugoroku, szMsg);
				for (j = 0; j < G.nPlayers; j ++) {
					if (!G.aPlayer[j].bAlive) {
						continue;
					}
					wsprintf (szMsg, "%s：%dG（現金%dG・土地%d件）", pL->aszName[j], G.Assets (j), G.aPlayer[j].nGold, G.OwnedCount (j));
					SubSugoroku_Msg (pSugoroku, szMsg);
				}
			}
			szMsg[0] = 0;
			if (E.nOther == 0) {
				strcpy (szMsg, "ゲーム終了！勝者はいません。");
			} else {
				std::string strWin;
				int nWin = 0;

				for (j = 0; j < G.nPlayers; j ++) {
					if (E.nOther & (1 << j)) {
						if (nWin) {
							strWin += "と";
						}
						strWin += pL->aszName[j];
						nWin ++;
					}
				}
				if (nWin > 1) {
					wsprintf (szMsg, "ゲーム終了！%sは同点で引き分けです。", strWin.c_str ());
				} else {
					wsprintf (szMsg, "ゲーム終了！%sの勝ちです！", strWin.c_str ());
				}
			}
			break;
		}
		if (szMsg[0]) {
			SubSugoroku_Msg (pSugoroku, szMsg);
#ifdef SBO_LAND_TEST
			printf ("[landtest] %s\n", szMsg);		/* テスト用：進行をサーバーのログに出す */
			fflush (stdout);
#endif
		}
	}

	/* 既存の状態機械の状態を、LandGame の段階に合わせる */
	switch (G.nPhase) {
	case LANDPHASE_ROLL:	pSugoroku->byState = SUBGAME_SUGOROKU_STATE_WAIT;		break;
	case LANDPHASE_MOVE:	pSugoroku->byState = SUBGAME_SUGOROKU_STATE_MOVE;		break;
	case LANDPHASE_BUY:		pSugoroku->byState = SUBGAME_SUGOROKU_STATE_LANDBUY;	break;
	default:				pSugoroku->byState = SUBGAME_SUGOROKU_STATE_GOAL;		break;
	}
	if ((G.nPhase == LANDPHASE_OVER) && pL->bPlaying) {
		pL->bPlaying = FALSE;
		pL->dwFinishTime = dwTime;
	}

	LandBroadcast (pSugoroku);
}
