/* ========================================================================= */
/* ファイル名：	SboSvrPuzzle.h												 */
/* 概要：		落ち物パズルの台（台NPC・席・ゲームの進行）の処理(定義ファイル) */
/* 作成日：		2026/11/24													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL PuzzleIsTableNpc(PCHARINFO pNpc);								/* 台詞が "@p" で始まる台NPCか */
extern void PuzzleSit(PCHARINFO pChar, PCHARINFO pNpc);						/* 台の座る場所のマスでSキーを押した（コインを使って席に座る。座る場所以外では何もしない） */
extern BOOL PuzzleOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc);				/* pChar が台NPCの「座る場所」のマスの上にいるか（同じマップ・空間。台詞の 9・10 番目。既定は台NPCの 1 つ下） */
extern BYTE PuzzleGhostForSend(PCHARINFO pChar);							/* CMD_CHARINFO に載せる byGhost（台NPCは体を描かない CHARGHOST_NAMEONLY に上書き。ほかは pChar->byGhost） */
extern BOOL PuzzleIsSeated(PCHARINFO pChar);								/* 席に座っているか（座っている間は歩けない） */
extern void PuzzleKey(PCHARINFO pChar, BYTE byKey);							/* 座っている人のキー入力（PUZZLEKEY_*） */
extern void PuzzleProc(void);												/* 周期処理（g_dwTime の経過時間で落下を進める。メインループから毎回呼ぶ） */
extern void PuzzleOnLeave(PCHARINFO pChar);									/* 切断・マップ移動などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  PuzzleIdleCount(void);											/* 表に載っている待機中の台の数（近くに人がいる台と、最後の盤面を残している台。テスト用） */
extern int  PuzzleSeatCount(void);											/* 今座っている人の数（テスト用） */
extern BOOL PuzzleIsVersus(PCHARINFO pChar);								/* 対戦中か（テスト用） */
struct PuzzleGame;
extern PuzzleGame *PuzzleGetGame(PCHARINFO pChar);							/* 座っている人のゲーム（座っていなければ NULL。テスト用） */
