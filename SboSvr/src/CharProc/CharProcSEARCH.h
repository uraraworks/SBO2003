/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH.h											 */
/* 内容：		キャラ処理(検索してついていく)クラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcSEARCH : public CCharProcBase
{
public:
	CCharProcSEARCH();									/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */

protected:
	BOOL	ProcTarget		(DWORD dwTime);							/* ターゲットを追いかける */
	BOOL	ProcWander		(DWORD dwTime);							/* 開始位置の近くをうろつく */
	BOOL	ProcGoHome		(DWORD dwTime);							/* 開始位置へ歩いて戻る */
	DWORD	FindTarget		(void);									/* 探索範囲で一番近いプレイヤーを探す */
	void	DropTarget		(BOOL bGiveUp = FALSE);					/* ターゲットを解除して帰り始める */
	BOOL	IsTargetValid	(PCHARINFO pCharTmp);					/* ターゲットがまだ追える状態か */
	BOOL	CanStep			(PCHARINFO pCharTmp, int x, int y, PDWORD pdwBlockPC = NULL);	/* そのマスへ1歩進めるか */
	BOOL	StepToward		(PCHARINFO pCharTmp, int nX, int nY);	/* 目標のマスへ1歩近づく */
	int		FindPathStep	(PCHARINFO pCharTmp, int nX, int nY);	/* 近くの範囲で道をさがして最初の1歩の向きを返す */
	BOOL	IsInSight		(PCHARINFO pCharTmp);					/* 間に通れないマスが無く見通せるか */
	BOOL	IsOverLeash		(void);									/* 開始位置から離れすぎたか */
	void	ResetAtHome		(DWORD dwTime);							/* 家に着いた：HPを戻す */
	BOOL	StepDirection	(PCHARINFO pCharTmp, int nDirection);	/* 指定の向きへ1歩進む */
	int		GetWanderArea	(BOOL bX);								/* うろつく範囲 */

protected:
	BOOL	m_bInit;									/* 初回処理済み */
	DWORD	m_dwTargetLast;								/* 前回の順番のターゲット */
	DWORD	m_dwRestEnd;								/* うろつきの休みが終わる時間 */
	DWORD	m_dwLostTime;								/* ターゲットが探索範囲から外れた時間 */
	DWORD	m_dwStuckTime;								/* 進めなくなった時間(帰る途中) */
	BOOL	m_bGoHome;									/* 開始位置へ戻る途中 */
	BOOL	m_bGiveUp;									/* 追うのをあきらめて帰る途中（着いたらHPを戻す） */
	DWORD	m_dwGiveUpEnd;								/* あきらめてから、新しく探さずに帰る間の終わり */
} CCharProcSEARCH, *PCCharProcSEARCH;

/* Copyright(C)URARA-works 2006 */
