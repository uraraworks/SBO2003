/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemSUBGAME.h											 */
/* 内容：		アイテム情報(サブゲーム参加)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/14													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemSUBGAME : public CInfoItemBase
{
public:
			CInfoItemSUBGAME();						/* コンストラクタ */
	virtual ~CInfoItemSUBGAME();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemSUBGAME *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetType		(BYTE byType)	{ m_byType = byType;	}	/* 種類 */

	/* 情報の取得 */
	BOOL	GetType		(void)	{ return m_byType;	}	/* 種類 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BOOL	m_byType;				/* 種類 */
} CInfoItemSUBGAME, *PCInfoItemSUBGAME;

/* Copyright(C)URARA-works 2006 */
