/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemMOVE.h												 */
/* 内容：		アイテム情報(指定座標へ移動)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/07													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemMOVE : public CInfoItemBase
{
public:
			CInfoItemMOVE();					/* コンストラクタ */
	virtual ~CInfoItemMOVE();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemMOVE *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetEnableMapID	(DWORD dwEnableMapID)	{ m_dwEnableMapID	= dwEnableMapID;	}	/* 使用限定するマップID */
	void	SetMapID		(DWORD dwMapID)			{ m_dwMapID			= dwMapID;			}	/* 移動先マップID */
	void	SetPos			(POINT ptPos)			{ m_ptPos			= ptPos;			}	/* 移動先座標 */

	/* 情報の取得 */
	DWORD	GetEnableMapID	(void)	{ return m_dwEnableMapID;	}	/* 使用限定するマップID */
	DWORD	GetMapID		(void)	{ return m_dwMapID;			}	/* 移動先マップID */
	POINT	GetPos			(void)	{ return m_ptPos;			}	/* 移動先座標 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	DWORD	m_dwEnableMapID,	/* 使用限定するマップID(-1:限定しない) */
			m_dwMapID;			/* 移動先マップID */
	POINT	m_ptPos;			/* 移動先座標 */
} CInfoItemMOVE, *PCInfoItemMOVE;

/* Copyright(C)URARA-works 2006 */
