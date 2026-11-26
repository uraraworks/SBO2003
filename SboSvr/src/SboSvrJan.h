/* ========================================================================= */
/* ファイル名：	SboSvrJan.h													 */
/* 概要：		SBOじゃんの卓（台NPC・4 席・CPU・ゲームの進行）の処理(定義ファイル) */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */

#pragma once

/* 進行の時間(ms) */
#define JAN_START_MS		(1500)		/* 座ってから配牌まで */
#define JAN_CPU_MS			(700)		/* CPU の 1 動作（引く・捨てる・鳴きの確定）の間隔 */
#define JAN_TURN_MS			(20000)		/* 人間の手番（捨てる）の制限時間。時間切れは引いた牌を捨てる */
#define JAN_CLAIM_MS		(8000)		/* 人間がロン・鳴きできる時の待ち。時間切れは見送り */
#define JAN_OVER_MS			(6000)		/* 局が終わってから席を立たせるまで */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL JanIsTableNpc(PCHARINFO pNpc);									/* 台詞が "@j" で始まる台NPCか */
extern BOOL JanOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc, int *pnSeat);	/* pChar が台NPCの 4 つの座る場所のどれかのマスの上にいるか（同じマップ・空間）。いれば席番号 0〜3 を *pnSeat に入れる（NULL 可） */
extern void JanSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bCoinUsed = FALSE);	/* 座る場所のマスで席に座る（座る場所以外では何もしない）。Sキー：無料の台だけ始まる。bCoinUsed=TRUE（道具のコインから）：コインを使って始まる */
extern BOOL JanUseItem(PCHARINFO pChar, WORD wItem);						/* 道具を使った時の処理。座る場所のマスでその台のコインなら座って始める（TRUE）。当てはまらなければ FALSE */
extern BOOL JanIsSeated(PCHARINFO pChar);									/* 席に座っているか（座っている間は歩けない） */
extern void JanKey(PCHARINFO pChar, BYTE byAct, BYTE byParam);				/* 座っている人の操作（JAN_ACT_*。反則・手番外は黙って捨てる） */
extern void JanProc(void);													/* 周期処理（g_dwTime の経過時間で卓を進める。メインループから毎回呼ぶ） */
extern void JanOnLeave(PCHARINFO pChar);									/* 切断などで席を立たせて卓を片付ける（本人へは何も送らない） */
extern int  JanIdleCount(void);												/* 表に載っている待機中の卓の数（テスト用） */
extern int  JanSeatCount(void);												/* 人が座っている卓の数（テスト用） */
struct JanjanGame;
extern JanjanGame *JanGetGame(PCHARINFO pChar);								/* 座っている人の卓のゲーム（座っていなければ NULL。テスト用） */
extern int  JanGetSeat(PCHARINFO pChar);									/* 座っている席の番号（座っていなければ -1。テスト用） */
extern DWORD g_dwJanSeedOverride;											/* 0 以外ならこれを乱数の種にする（テスト用） */
