/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemRESURRECTION.h										 */
/* 内容：		アイテム情報(蘇生)クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/16													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemRESURRECTION : public CInfoItemBase
{
public:
			CInfoItemRESURRECTION();			/* コンストラクタ */
	virtual ~CInfoItemRESURRECTION();			/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemRESURRECTION *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetRate			(BYTE byRate)	{ m_byRate	= byRate;	}	/* 回復率 */

	/* 情報の取得 */
	BYTE	GetRate			(void)	{ return m_byRate;	}	/* 回復率 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BYTE	m_byRate;					/* 回復率 */
} CInfoItemRESURRECTION, *PCInfoItemRESURRECTION;

/* Copyright(C)URARA-works 2006 */
