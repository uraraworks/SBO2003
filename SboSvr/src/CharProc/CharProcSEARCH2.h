/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH2.h											 */
/* 内容：		キャラ処理(検索してついていく2)クラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/08													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"
#include <vector>
#include <utility>

/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* 検索情報 */
typedef struct _SEARCH2INFO {
	BYTE	x;					/* 横座標 */
	BYTE	y;					/* 縦座標 */
	BYTE	byDirection;		/* 向き */
	BYTE	byState;			/* 状態 */
	WORD	wScore;				/* スコア */
	WORD	wTurn;				/* ターン */
} SEARCH2INFO, *PSEARCH2INFO;
typedef CmyArray<SEARCH2INFO, SEARCH2INFO>		ARRAYSEARCH2INFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcSEARCH2 : public CCharProcBase
{
public:
			CCharProcSEARCH2();					/* コンストラクタ */
	virtual	~CCharProcSEARCH2();				/* デストラクタ */

	BOOL Proc		(DWORD dwTime);						/* 処理 */
	void ChgTarget	(DWORD dwTblID, BYTE byDirection);	/* ターゲット変更 */


protected:
	BOOL	IsBlock					(int x, int y);			/* 当たり判定 */
	BOOL	IsMove					(int nDirection);		/* 移動するか判定 */
	BOOL	IsTargetReady			(void);					/* ターゲットが有効か判定 */
	void	Init					(void);					/* 初期化 */
	void	ClearSearchMap			(void);					/* 検索用マップを消去 */
	DWORD	Search					(BOOL bPCCheck);		/* 検索 */
	int		SearchMove				(void);					/* 検索移動 */
	void	InfoCleanup				(int x, int y);			/* 情報最適化 */

	void	ProcREADY				(void);					/* 処理(待機中) */
	void	ProcSEARCHTARGETINIT	(void);					/* 処理(ターゲット検索用初期化) */
	void	ProcSEARCHTARGET		(void);					/* 処理(ターゲット検索中) */
	void	ProcSEARCHMOVETARGET	(void);					/* 処理(ターゲット検索移動中) */
	void	ProcSEARCHINIT			(void);					/* 処理(検索用初期化) */
	void	ProcPREMOVE				(void);					/* 処理(検索前移動中) */
	void	ProcSEARCH				(void);					/* 処理(検索中) */
	void	ProcSEARCHMOVE			(void);					/* 処理(検索移動中) */
	void	ProcMOVE				(void);					/* 処理(移動中) */


protected:
	POINT				m_ptStart,				/* 開始位置 */
						m_ptEnd,				/* 最終位置 */
						m_ptNow,				/* 現在位置 */
						m_ptSearch;				/* 検索中の座標 */
	SIZE				m_sizeArea;				/* 検索範囲 */
	int					m_nState,				/* 状態 */
						m_nDirection,			/* 向き */
						m_nSearchDirection,		/* 検索中の向き */
						m_nSearchTurn;			/* ターン */
	DWORD				m_dwSearchLastTime;		/* 前回の処理時間 */
	PWORD				m_pwSearchMap;			/* 検索用マップ */
	ARRAYSEARCH2INFO	m_aSearchInfo;			/* 検索情報 */
	DWORD				m_dwSearchMapCapacity;	/* 検索用マップの容量 */
	std::vector<int>	m_anSearchMapUsed;		/* 書き込んだマップの添字 */
	std::vector<std::pair<WORD, int> > m_aSearchOpen;	/* 未処理候補（スコア、追加順） */
} CCharProcSEARCH2, *PCCharProcSEARCH2;

/* Copyright(C)URARA-works 2006 */
