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
extern void PuzzleKey(PCHARINFO pChar, BYTE byKey, DWORD dwSerial = 0);							/* 座っている人のキー入力（PUZZLEKEY_*） */
extern void PuzzleProc(void);												/* 周期処理（g_dwTime の経過時間で落下を進める。メインループから毎回呼ぶ） */
extern void PuzzleOnLeave(PCHARINFO pChar);									/* 切断・マップ移動などで席を立たせて台の状態を片付ける（本人へは何も送らない） */
extern int  PuzzleIdleCount(void);											/* 表に載っている待機中の台の数（近くに人がいる台と、最後の盤面を残している台。テスト用） */
extern int  PuzzleSeatCount(void);											/* 今座っている人の数（テスト用） */
extern BOOL PuzzleIsVersus(PCHARINFO pChar);								/* 対戦中か（テスト用） */
struct PuzzleGame;
extern PuzzleGame *PuzzleGetGame(PCHARINFO pChar);							/* 座っている人のゲーム（座っていなければ NULL。テスト用） */
extern int  PuzzleWinStreak(PCHARINFO pChar);								/* 座っている人の連勝数（座っていなければ 0。テスト用） */

/* ハイスコア・連勝記録（落ち物パズル全体で 1 つずつ。上位 PUZZLE_RECORD_MAX 件・1 キャラ 1 件・同点は先着。保存は SaveLoad/SboSvrSaveLoadPuzzleRecord.cpp） */
struct PuzzleRecordList;
extern PuzzleRecordList g_PuzzleScoreRec;									/* ハイスコア */
extern PuzzleRecordList g_PuzzleStreakRec;									/* 連勝記録 */
extern DWORD g_dwPuzzleNowOverride;											/* 0 以外ならこれを「今の時刻（UNIX 秒）」にする（テスト用） */
extern void PuzzleRecordClear(void);										/* 記録を全部消す（終了時・読み込み前） */
extern DWORD PuzzleDateJst(void);											/* 今日の日付（日本時間の年月日。20261125 の形） */
extern int  PuzzleRecordScore(LPCSTR pszName, DWORD dwScore);				/* ハイスコアに足す（順位 1〜10 か、入らなければ 0） */
extern int  PuzzleRecordStreak(LPCSTR pszName, DWORD dwStreak);				/* 連勝記録に足す（2 連勝以上だけ。順位 1〜10 か 0） */
extern BOOL PuzzleIsRecordNpc(PCHARINFO pNpc);								/* 台詞が "@h" で始まる掲示板NPCか（ハイスコア・連勝記録を見せる） */
extern void PuzzleRecordTalk(PCHARINFO pChar, PCHARINFO pNpc);				/* 掲示板NPCに話しかけた（本人にだけ、ハイスコアと連勝記録をメッセージで出す） */
extern void PuzzleRecordMessage(char *pszOut, int nSize);					/* 掲示板に出す文を作る（テスト用に外へ出している。@1=改行・@0=入力待ち） */
