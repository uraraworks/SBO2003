/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoSubGameBase.h											 */
/* 内容：		サブゲーム情報基底クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/14													 */
/* ========================================================================= */

#pragma once


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoSubGameBase
{
public:
			CInfoSubGameBase();								/* コンストラクタ */
	virtual ~CInfoSubGameBase();							/* デストラクタ */

	virtual void	Init				(void);						/* ゲーム情報を初期化 */
	virtual DWORD	GetSendDataSize		(void);						/* 送信用データサイズを取得 */
	virtual PBYTE	GetSendData			(void);						/* 送信用データを取得 */
	virtual DWORD	SetSendData			(PBYTE pSrc);				/* 送信用データから設定 */
	virtual LPCSTR	GetName				(void);						/* サブゲーム名を取得 */


public:
	DWORD		m_dwID,								/* サブゲームID */
				m_dwMapID;							/* マップID */
	RECT		m_rcArea;							/* ゲームエリア */
} CInfoSubGameBase, *PCInfoSubGameBase;

/* Copyright(C)URARA-works 2006 */
