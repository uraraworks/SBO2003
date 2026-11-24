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
extern void PuzzleSit(PCHARINFO pChar, PCHARINFO pNpc);						/* 台NPCにSキーで話しかけた（コインを使って席に座る） */
extern BOOL PuzzleIsSeated(PCHARINFO pChar);								/* 席に座っているか（座っている間は歩けない） */
extern void PuzzleKey(PCHARINFO pChar, BYTE byKey);							/* 座っている人のキー入力（PUZZLEKEY_*） */
extern void PuzzleProc(void);												/* 周期処理（g_dwTime の経過時間で落下を進める。メインループから毎回呼ぶ） */
extern void PuzzleOnLeave(PCHARINFO pChar);									/* 切断・マップ移動などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  PuzzleSeatCount(void);											/* 今座っている人の数（テスト用） */
