/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBase.h												 */
/* 内容：		キャラ処理基底クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/16													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcBase
{
public:
			CCharProcBase();									/* コンストラクタ */
	virtual ~CCharProcBase();									/* デストラクタ */

			void Create		(PCHARINFO pChar);							/* 作成 */
	virtual BOOL Proc		(DWORD dwTime);								/* 処理 */
	virtual void GetPos		(BYTE &byPosX, BYTE &byPosY);				/* 座標を取得 */
			void SetProc	(PCHARINFO pChar);							/* 処理を設定 */
	virtual void ChgTarget	(DWORD dwTblID, BYTE byDirection);			/* ターゲット変更 */


protected:
	BOOL NpcProcSEARCH_Check(PCHARINFO pChar, PCHARINFO pCharTmp, int nX, int nY);	/* 検索処理チェック */
	BOOL PartsMoveProc		(PCHARINFO pChar);										/* 移動パーツにのっていれば移動させる */
	void MoveStartPosProc	(PCHARINFO pChar);										/* 開始位置に移動 */


protected:
	int				m_nPosX;							/* 座標(横) */
	int				m_nPosY;							/* 座標(縦) */
	DWORD			m_dwLastTimeProc;					/* 最後に処理した時間 */
	PCHARINFO		m_pChar;							/* キャラ情報 */
} CCharProcBase, *PCCharProcBase;

/* Copyright(C)URARA-works 2006 */
