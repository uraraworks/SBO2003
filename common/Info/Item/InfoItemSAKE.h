/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemSAKE.h												 */
/* 内容：		アイテム情報(酒)クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/14													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemSAKE : public CInfoItemBase
{
public:
			CInfoItemSAKE();						/* コンストラクタ */
	virtual ~CInfoItemSAKE();						/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemSAKE *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetPoint		(char chPoint)	{ m_chPoint = chPoint;	}	/* 度数 */

	/* 情報の取得 */
	BOOL	GetPoint		(void)	{ return m_chPoint;	}	/* 度数 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	char	m_chPoint;					/* 度数 */
} CInfoItemSAKE, *PCInfoItemSAKE;

/* Copyright(C)URARA-works 2006 */
