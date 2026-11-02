/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CHAR_TAIL.h													 */
/* 内容：		コマンド(キャラ系:ついていく要求) 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/08/01													 */
/* ========================================================================= */

#pragma once

#include "PacketBase.h"

/* ========================================================================= */
/* 定数の定義																 */
/* ========================================================================= */

enum {
	CHAR_TAIL_REQ = 0,								/* ついていく要求 */
	CHAR_TAIL_BACK,									/* ○○がついてきます */
	CHAR_TAIL_ON,									/* ○○についていきます */
	CHAR_TAIL_OFF,									/* ついていくのを解除しました */
	CHAR_TAIL_OFF2,									/* ついていくのが解除されました */
	CHAR_TAIL_NG,									/* ○○は最後尾ではありません */
	CHAR_TAIL_NG2,									/* ○○にはついていけません */
};


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCHAR_TAIL : public CPacketBase
{
public:
			CCHAR_TAIL();								/* コンストラクタ */
	virtual ~CCHAR_TAIL();								/* デストラクタ */

	void	Make		(DWORD dwTblID, DWORD dwTblIDTarget, DWORD dwResult);	/* パケットを作成 */
	void	Set			(PBYTE pPacket);						/* パケットを設定 */


public:
	DWORD	m_dwTblID,							/* 対象キャラのテーブルID */
			m_dwTailTblID,						/* ついていこうとしているキャラのテーブルID */
			m_dwResult;							/* 結果 */
} CCHAR_TAIL, *PCCHAR_TAIL;

/* Copyright(C)URARA-works 2006 */
