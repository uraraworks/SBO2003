/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrTalk.h												 */
/* 概要：		キャラのフラグと会話データの処理(定義ファイル)				 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* フラグ（印 0〜1023・数値 0〜127）。範囲の外の番号は何もしない（読むと 0） */
extern BOOL FlagBitGet(const BYTE *pbyFlag, int nNo);					/* 印が立っているか */
extern void FlagBitSet(BYTE *pbyFlag, int nNo, BOOL bOn);				/* 印を立てる・消す */
extern BYTE FlagNumGet(const BYTE *pbyFlagNum, int nNo);				/* 数値を読む */
extern void FlagNumSet(BYTE *pbyFlagNum, int nNo, int nVal);			/* 数値を ○にする（0〜255 に収める） */
extern void FlagNumAdd(BYTE *pbyFlagNum, int nNo, int nVal);			/* 数値に ○足す（255 で止まる） */
extern void FlagNumAct(BYTE *pbyFlagNum, int nNo, BYTE byOp, BYTE byVal);	/* 数値を動かす（byOp は TALKNUMOP_*。番号が「無し」なら何もしない） */
extern void FlagApplyQuestDone(PCHARINFO pChar, const QUESTINFO *pQuest);	/* 依頼を完了した時の印・数値の動きを反映する */

/* 会話データ */
extern BOOL TalkIsNpc(PCHARINFO pNpc);									/* 台詞が "@t" と数字で始まるか（会話データの NPC） */
extern int  TalkGetNo(PCHARINFO pNpc);									/* 会話セット番号（"@tNNN" の NNN） */
extern PTALKINFO TalkFind(int nNo);										/* 会話セット番号から探す（無い・削除済みは NULL） */
extern int  TalkPickLine(const TALKINFO *pTalk, const BYTE *pbyFlag, const BYTE *pbyFlagNum, int nLevel, DWORD dwRand);	/* 条件に合う行を選ぶ（無ければ -1。dwRand は同じ順位が複数ある時のくじ） */
extern void TalkApplyLine(const TALKLINEINFO *pLine, BYTE *pbyFlag, BYTE *pbyFlagNum);	/* 話した後の印・数値の動きを反映する */
extern void TalkNpcTalk(PCHARINFO pChar, PCHARINFO pNpc);				/* 会話データの NPC に話しかけた（行を選び、印を動かし、本人にだけ台詞を送る） */

/* Copyright(C)URARA-works 2003-2005 */
