/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	LibInfoItem.h												 */
/* 内容：		アイテム情報ライブラリ クラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/21													 */
/* ========================================================================= */

#pragma once

#include "../Info/Item/InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CLibInfoItem
{
public:
			CLibInfoItem();								/* コンストラクタ */
	virtual ~CLibInfoItem();							/* デストラクタ */

	int		GetCount	(void);									/* 登録数を取得 */
	DWORD	GetType		(DWORD dwItemID);						/* 種別を取得 */
	void	Add			(CInfoItemBase *pSrc);					/* 追加 */
	void	DeleteAll	(void);									/* 全て削除 */

	CInfoItemBase	*Get	(DWORD dwItemID);					/* 取得 */
	CInfoItemBase	*Get	(int nNo);							/* 取得 */
	CInfoItemBase	*GetNew	(DWORD dwItemTypeID);				/* 指定種別の新規データを取得 */

	DWORD	GetDataSize		(DWORD dwItemID);					/* データサイズを取得 */
	PBYTE	GetData			(DWORD dwItemID);					/* データを取得 */
	PBYTE	SetData			(DWORD dwItemID, PBYTE pSrc);		/* データから設定 */

	DWORD	GetAllDataSize	(void);								/* 全データサイズを取得 */
	PBYTE	GetAllData		(void);								/* 全データを取得 */
	PBYTE	SetAllData		(int nCount, PBYTE pSrc);			/* 全データを設定 */


protected:
	DWORD	GetNewID	(void);									/* 追加用の新しいIDを取得 */


protected:
	ARRAYINFOITEM		m_aInfoItem;					/* アイテム情報 */
} CLibInfoItem, *PCLibInfoItem;

/* Copyright(C)URARA-works 2006 */
