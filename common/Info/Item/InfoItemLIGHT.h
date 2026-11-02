/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemLIGHT.h												 */
/* 内容：		アイテム情報(明かり)クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemLIGHT : public CInfoItemBase
{
public:
			CInfoItemLIGHT();					/* コンストラクタ */
	virtual ~CInfoItemLIGHT();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemLIGHT *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetLevel		(BYTE byLevel)	{ m_byLevel = byLevel;	}	/* 明るさレベル */
	void	SetTime			(WORD wTime)	{ m_wTime = wTime;		}	/* 維持時間 */

	/* 情報の取得 */
	BYTE	GetLevel		(void)	{ return m_byLevel;	}		/* 明るさレベル */
	WORD	GetTime			(void)	{ return m_wTime;	}		/* 維持時間 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BYTE	m_byLevel;					/* 明るさレベル */
	WORD	m_wTime;					/* 維持時間 */
} CInfoItemLIGHT, *PCInfoItemLIGHT;

/* Copyright(C)URARA-works 2006 */
