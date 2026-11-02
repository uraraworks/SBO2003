/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemEQU_LEG.h											 */
/* 内容：		アイテム情報(装備(靴))クラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/14													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemEQU_LEG : public CInfoItemBase
{
public:
			CInfoItemEQU_LEG();					/* コンストラクタ */
	virtual ~CInfoItemEQU_LEG();				/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemEQU_LEG *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetEqu			(WORD wEqu)	{ m_wEqu = wEqu;	}	/* 装備できるキャラ情報 */

	/* 情報の取得 */
	WORD	GetEqu			(void)	{ return m_wEqu;	}		/* 装備できるキャラ情報 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	WORD	m_wEqu;						/* 装備できるキャラ情報 */
} CInfoItemEQU_LEG, *PCInfoItemEQU_LEG;

/* Copyright(C)URARA-works 2006 */
