/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemHPMANAUP.h											 */
/* 内容：		アイテム情報(HP・Mana回復)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/16													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemHPMANAUP : public CInfoItemBase
{
public:
			CInfoItemHPMANAUP();				/* コンストラクタ */
	virtual ~CInfoItemHPMANAUP();				/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemHPMANAUP *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetRateHP		(BOOL bRateHP)		{ m_bRateHP		= bRateHP;		}	/* 確立で回復か判定(HP) */
	void	SetRateMana		(BOOL bRateMana)	{ m_bRateMana	= bRateMana;	}	/* 確立で回復か判定(Mana) */
	void	SetMinHP		(WORD wMinHP)		{ m_wMinHP		= wMinHP;		}	/* 最低回復値(HP) */
	void	SetMinMana		(WORD wMinMana)		{ m_wMinMana	= wMinMana;		}	/* 最低回復値(Mana) */
	void	SetMaxHP		(WORD wMaxHP)		{ m_wMaxHP		= wMaxHP;		}	/* 最大回復値(HP) */
	void	SetMaxMana		(WORD wMaxMana)		{ m_wMaxMana	= wMaxMana;		}	/* 最大回復値(Mana) */

	/* 情報の取得 */
	BOOL	GetRateHP		(void)	{ return m_bRateHP;		}	/* 確立で回復か判定(HP) */
	BOOL	GetRateMana		(void)	{ return m_bRateMana;	}	/* 確立で回復か判定(Mana) */
	WORD	GetMinHP		(void)	{ return m_wMinHP;		}	/* 最低回復値(HP) */
	WORD	GetMinMana		(void)	{ return m_wMinMana;	}	/* 最低回復値(Mana) */
	WORD	GetMaxHP		(void)	{ return m_wMaxHP;		}	/* 最大回復値(HP) */
	WORD	GetMaxMana		(void)	{ return m_wMaxMana;	}	/* 最大回復値(Mana) */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BOOL	m_bRateHP,					/* 確立で回復か判定(HP) */
			m_bRateMana;				/* 確立で回復か判定(Mana) */
	WORD	m_wMinHP,					/* 最低回復値(HP) */
			m_wMinMana,					/* 最低回復値(Mana) */
			m_wMaxHP,					/* 最大回復値(HP) */
			m_wMaxMana;					/* 最大回復値(Mana) */
} CInfoItemHPMANAUP, *PCInfoItemHPMANAUP;

/* Copyright(C)URARA-works 2006 */
