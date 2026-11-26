/* ========================================================================= */
/* ファイル名：	SboSvrMaze.h												 */
/* 概要：		迷路対戦の台（台NPC・席・ゲームの進行）の処理(定義ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL MazeIsTableNpc(PCHARINFO pNpc);									/* 台詞が "@k" で始まる迷路の台NPCか（依頼NPCの "@q" とは別） */
extern BOOL MazeOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc);				/* pChar が台NPCの「座る場所」のマスの上にいるか（同じマップ・空間。台詞の 3・4 番目。既定は台NPCの 1 つ下） */
extern void MazeSit(PCHARINFO pChar, PCHARINFO pNpc);						/* 台の座る場所のマスで席に座る（座る場所以外では何もしない）。無料の台のみ。Sキー */
extern BOOL MazeIsSeated(PCHARINFO pChar);									/* 席に座っているか（座っている間は歩けない） */
extern void MazeKey(PCHARINFO pChar, BYTE byAct, BYTE byDir, BYTE byX, BYTE byY, BYTE byOrient);	/* 座っている人の操作（MAZEACT_*）。不正なら無視して状態を送り直す */
extern void MazeProc(void);													/* 周期処理（勝ち表示の時間・席を離れた人の片付け・近くの人の見直し。メインループから毎回呼ぶ） */
extern void MazeOnLeave(PCHARINFO pChar);									/* 切断などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  MazeIdleCount(void);											/* 表に載っている待機中の台の数（テスト用） */
extern int  MazeSeatCount(void);											/* 今座っている人の数（テスト用） */
struct MazeGame;
extern MazeGame *MazeGetGame(PCHARINFO pChar);								/* 座っている人のゲーム（座っていなければ NULL。テスト用） */
