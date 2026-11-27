/* ========================================================================= */
/* ファイル名：	SboSvrMaze.h												 */
/* 概要：		迷路対戦の台（台NPC・席・ゲームの進行）の処理(定義ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL MazeIsTableNpc(PCHARINFO pNpc);									/* 台詞が "@k<番号>" で、番号がサブゲーム情報の迷路対戦の台（SUBGAMEID_MAZETABLE）を指す台NPCか（依頼NPCの "@q" とは別。番号が無い・種別違いなら FALSE） */
extern BOOL MazeOnSeatCell(PCHARINFO pChar, PCHARINFO pNpc);				/* pChar が台NPCの「座る場所」（席 1＝サブゲーム情報の座る場所1・既定は台NPCの 1 つ下／席 2＝座る場所2・既定は (0,-10)）のマスの上にいるか（同じマップ・空間） */
extern void MazeSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bTicketUsed = FALSE, int nChoice = 0);	/* 台の座る場所のマスで席に座る（座る場所以外では何もしない）。Sキー：無料の台だけ座る（チケットの台は案内だけ）。bTicketUsed=TRUE（道具のチケットから）：チケットを使って座る。
																				   もう一方の席でひとりが遊んでいれば「乱入しますか？」、CPU強さ>0 の台にひとりで座るなら「CPUと対戦しますか？」、CPU対戦中のCPU側の席なら「交代して対戦しますか？」を聞く
																				   （nChoice: 0=まだ聞いていない 1=はい 2=いいえ）。チケットは答えの「はい」を受けて座る時に取り上げる */
extern BOOL MazeUseItem(PCHARINFO pChar, WORD wItem);						/* 道具を使った時の処理。座る場所のマスでその台のチケットなら座る（TRUE。足りない時も伝えて TRUE）。当てはまらなければ FALSE */
extern BOOL MazeAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes);		/* 「乱入しますか？」への答え（CMD_TALKANSWER）。この問いへの答えなら TRUE（30 秒で無効）。違えば FALSE で会話の処理へ */
extern BOOL MazeIsSeated(PCHARINFO pChar);									/* 席に座っているか（座っている間は歩けない） */
extern void MazeKey(PCHARINFO pChar, BYTE byAct, BYTE byDir, BYTE byX, BYTE byY, BYTE byOrient);	/* 座っている人の操作（MAZEACT_*）。不正なら無視して状態を送り直す */
extern void MazeProc(void);													/* 周期処理（勝ち表示の時間・席を離れた人の片付け・近くの人の見直し。メインループから毎回呼ぶ） */
extern void MazeOnLeave(PCHARINFO pChar);									/* 切断などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  MazeIdleCount(void);											/* 表に載っている待機中の台の数（テスト用） */
extern int  MazeSeatCount(void);											/* 今座っている人の数（テスト用） */
struct MazeGame;
extern MazeGame *MazeGetGame(PCHARINFO pChar);								/* 座っている人のゲーム（座っていなければ NULL。テスト用） */
