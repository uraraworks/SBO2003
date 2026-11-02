/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CHAR_STATEINFO.h											 */
/* 内容：		コマンド(キャラ系:状態通知) 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/25													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCHAR_STATEINFO
{
public:
			CCHAR_STATEINFO();							/* コンストラクタ */
	virtual ~CCHAR_STATEINFO();							/* デストラクタ */

	void	Make		(PCHARINFO pChar);						/* パケットを作成 */
	void	Set			(PBYTE pPacket);						/* パケットを設定 */
	DWORD	GetSize		(void);									/* パケットサイズを取得 */
	PBYTE	Get			(void);									/* パケットを取得 */


public:
	PBYTE	m_pPacket;							/* パケット */
	DWORD	m_dwPacketSize,						/* パケットサイズ */
			m_dwTblID,							/* 対象キャラのテーブルID */
			m_dwTailTblID;						/* ついていっているキャラのテーブルID */
} CCHAR_STATEINFO, *PCCHAR_STATEINFO;

/* Copyright(C)URARA-works 2006 */
