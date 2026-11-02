/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemGRPCHG.h											 */
/* 内容：		アイテム情報(キャラ画像変更)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemGRPCHG : public CInfoItemBase
{
public:
			CInfoItemGRPCHG();					/* コンストラクタ */
	virtual ~CInfoItemGRPCHG();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemGRPCHG *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetGrpID		(WORD wGrpID)	{ m_wGrpID = wGrpID;	}	/* 変更するグラフィックID（０は記録した画像） */

	/* 情報の取得 */
	WORD	GetGrpID		(void)	{ return m_wGrpID;	}		/* 変更するグラフィックID（０は記録した画像） */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	WORD	m_wGrpID;					/* 変更するグラフィックID（０は記録した画像） */
} CInfoItemGRPCHG, *PCInfoItemGRPCHG;

/* Copyright(C)URARA-works 2006 */
