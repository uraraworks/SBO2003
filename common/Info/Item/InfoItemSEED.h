/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemSEED.h												 */
/* 内容：		アイテム情報(種)クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/14													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemSEED : public CInfoItemBase
{
public:
			CInfoItemSEED();					/* コンストラクタ */
	virtual ~CInfoItemSEED();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemSEED *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetType			(BYTE byType)	{ m_byType	= byType;	}	/* 種類 */
	void	SetMax			(WORD wMax)		{ m_wMax	= wMax;		}	/* 上限値 */

	/* 情報の取得 */
	BOOL	GetType			(void)	{ return m_byType;	}	/* 種類 */
	WORD	GetMax			(void)	{ return m_wMax;	}	/* 上限値 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BOOL	m_byType;					/* 種類 */
	WORD	m_wMax;						/* 上限値 */
} CInfoItemSEED, *PCInfoItemSEED;

/* Copyright(C)URARA-works 2006 */
