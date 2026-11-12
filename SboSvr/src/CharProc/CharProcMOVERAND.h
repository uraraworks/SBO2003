/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcMOVERAND.h											 */
/* 内容：		キャラ処理(ランダム移動)クラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcMOVERAND : public CCharProcBase
{
public:
	CCharProcMOVERAND();								/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */

protected:
	BOOL	IsMovePos		(int x, int y);							/* そのマスへ歩けるか判定 */
	int		GetMoveWeight	(int nDirection);						/* 向きごとの歩きやすさ(移動確率)を取得 */
	int		ChooseDirection	(void);									/* 歩き出す向きを選ぶ */
	BOOL	CheckTalkPC		(void);									/* 目の前で向かい合っているプレイヤーがいれば振り向く */
	void	StartRest		(DWORD dwTime, BOOL bShort);			/* 立ち止まって休み始める */
	BOOL	StepFront		(void);									/* 向いている方向へ1歩進む */
	BOOL	IsRoute			(void);									/* 巡回ルートを歩くか */
	BOOL	ProcRoute		(DWORD dwTime);							/* 巡回ルートを歩く */
	void	NextRoutePoint	(void);									/* 次の道しるべへ */
	BOOL	ProcBattle		(DWORD dwTime);							/* 戦闘１の敵が戦っている間 */

protected:
	BOOL	m_bInit;									/* 初回処理済み */
	BOOL	m_bLooked;									/* この休みの間に見回したか */
	int		m_nWalkLeft;								/* まとめ歩きの残り歩数 */
	DWORD	m_dwRestStart;								/* 休み始めた時間 */
	DWORD	m_dwRestEnd;								/* 休みが終わる時間 */
	int		m_nRouteNo;									/* 巡回ルートで次に向かう道しるべ */
	int		m_nRouteDir;								/* 巡回ルートを進む向き(1:先へ -1:戻る) */
	DWORD	m_dwRouteWaitEnd;							/* 道しるべで止まり終わる時間 */
	DWORD	m_dwRouteStuck;								/* 巡回ルートで進めなくなった時間 */
	DWORD	m_dwBattleTarget;							/* 戦闘１：戦っている相手 */
	DWORD	m_dwBattleTime;								/* 戦闘１：最後に相手が近くにいた時間 */
} CCharProcMOVERAND, *PCCharProcMOVERAND;

/* Copyright(C)URARA-works 2006 */
