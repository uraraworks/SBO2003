/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemHPUP.h												 */
/* 内容：		アイテム情報(HP回復)クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/12													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemHPUP : public CInfoItemBase
{
public:
			CInfoItemHPUP();					/* コンストラクタ */
	virtual ~CInfoItemHPUP();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemHPUP *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetRate			(BOOL bRate)	{ m_bRate	= bRate;	}	/* 確立で回復か判定 */
	void	SetMin			(WORD wMin)		{ m_wMin	= wMin;		}	/* 最低回復値 */
	void	SetMax			(WORD wMax)		{ m_wMax	= wMax;		}	/* 最大回復値 */

	/* 情報の取得 */
	BOOL	GetRate			(void)	{ return m_bRate;	}	/* 確立で回復か判定 */
	WORD	GetMin			(void)	{ return m_wMin;	}	/* 最低回復値 */
	WORD	GetMax			(void)	{ return m_wMax;	}	/* 最大回復値 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BOOL	m_bRate;					/* 確立で回復か判定 */
	WORD	m_wMin,						/* 最低回復値 */
			m_wMax;						/* 最大回復値 */
} CInfoItemHPUP, *PCInfoItemHPUP;

/* Copyright(C)URARA-works 2006 */
