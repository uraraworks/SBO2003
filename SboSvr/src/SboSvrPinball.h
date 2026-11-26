/* ========================================================================= */
/* ファイル名：	SboSvrPinball.h												 */
/* 概要：		ピンボールの台（台NPC・席・ゲームの進行）の処理(定義ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL PinballIsTableNpc(PCHARINFO pNpc);								/* 台詞が "@i" で始まる台NPCか */
extern void PinballSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bCoinUsed = FALSE);	/* 台の座る場所のマスで席に座る（座る場所以外では何もしない）。Sキー：無料の台だけ始まる（有料の台は案内だけ）。bCoinUsed=TRUE（道具のコインから）：コインを使って始まる */
extern BOOL PinballUseItem(PCHARINFO pChar, WORD wItem);					/* 道具を使った時の処理。座る場所のマスでその台のコインなら席に座って始める（TRUE）。当てはまらなければ FALSE */
extern BOOL PinballIsSeated(PCHARINFO pChar);								/* 席に座っているか（座っている間は歩けない） */
extern void PinballKey(PCHARINFO pChar, BYTE byKey, DWORD dwSerial = 0);	/* 座っている人のキー入力（PINBALLKEY_*。押した時・離した時） */
extern void PinballProc(void);												/* 周期処理（g_dwTime の経過時間で固定ステップを進める。メインループから毎回呼ぶ） */
extern void PinballOnLeave(PCHARINFO pChar);								/* 切断・マップ移動などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  PinballSeatCount(void);											/* 今座っている人の数（テスト用） */
extern int  PinballIdleCount(void);											/* 表に載っている待機中の台の数（テスト用） */
struct PinballGame;
extern PinballGame *PinballGetGame(PCHARINFO pChar);						/* 座っている人のゲーム（座っていなければ NULL。テスト用） */
