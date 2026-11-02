/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGameBase.h											 */
/* 内容：		サブゲーム処理基底クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/18													 */
/* ========================================================================= */

#pragma once

class CInfoSubGameBase;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CProcSubGameBase
{
public:
			CProcSubGameBase();										/* コンストラクタ */
	virtual ~CProcSubGameBase();									/* デストラクタ */

	virtual void	Init				(BYTE byID, CInfoSubGameBase *pInfo);	/* 初期化 */
	virtual void	Destroy				(void);									/* 破棄 */
	virtual void	Proc				(void);									/* 処理 */
	virtual BOOL	IsInformationPos	(DWORD dwMapID, BYTE byX, BYTE byY);	/* 指定座標が受付位置か判定 */
	virtual DWORD	GetSendDataSize		(void);									/* 送信用データサイズを取得 */
	virtual PBYTE	GetSendData			(void);									/* 送信用データを取得 */
	virtual void	SetSendData			(PBYTE pSrc);							/* 送信用データから設定 */
	virtual int		GetType				(void);									/* サブゲーム種別を取得 */

			BYTE	GetID		(void)	{ return m_byID; }					/* サブゲームIDを取得 */


private:
	BYTE	m_byID;									/* サブゲームID */
	CInfoSubGameBase	*m_pInfoSubGame;			/* サブゲーム情報 */
} CProcSubGameBase, *PCProcSubGameBase;

/* Copyright(C)URARA-works 2006 */
