/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemWEAPON.h											 */
/* 内容：		アイテム情報(武器)クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/13													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemWEAPON : public CInfoItemBase
{
public:
			CInfoItemWEAPON();					/* コンストラクタ */
	virtual ~CInfoItemWEAPON();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemWEAPON *pSrc);	/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetEfcID		(BYTE byEfcID)		{ m_byEfcID			= byEfcID;		}	/* 攻撃時に発動するエフェクト番号 */
	void	SetMin			(BYTE byMin)		{ m_byMin			= byMin;		}	/* 攻撃力減少幅 */
	void	SetAtackType	(BYTE byAtackType)	{ m_byAtackType		= byAtackType;	}	/* 攻撃種別 */
	void	SetData1		(BYTE byData1)		{ m_byData1			= byData1;		}	/* 付加データ1 */
	void	SetData2		(BYTE byData2)		{ m_byData2			= byData2;		}	/* 付加データ2 */
	void	SetData3		(BYTE byData3)		{ m_byData3			= byData3;		}	/* 付加データ3 */
	void	SetData4		(BYTE byData4)		{ m_byData4			= byData4;		}	/* 付加データ4 */
	void	SetData5		(BYTE byData5)		{ m_byData5			= byData5;		}	/* 付加データ5 */
	void	SetEqu			(WORD wEqu)			{ m_wEqu			= wEqu;			}	/* 装備できるキャラ情報 */


	/* 情報の取得 */
	BYTE	GetEfcID		(void)	{ return m_byEfcID;	}		/* 攻撃時に発動するエフェクト番号 */
	BYTE	GetMin			(void)	{ return m_byMin;	}		/* 攻撃力減少幅 */
	BYTE	GeAtackType		(void)	{ return m_byAtackType;	}	/* 攻撃種別 */
	BYTE	GetData1		(void)	{ return m_byData1;	}		/* 付加データ1 */
	BYTE	GetData2		(void)	{ return m_byData2;	}		/* 付加データ2 */
	BYTE	GetData3		(void)	{ return m_byData3;	}		/* 付加データ3 */
	BYTE	GetData4		(void)	{ return m_byData4;	}		/* 付加データ4 */
	BYTE	GetData5		(void)	{ return m_byData5;	}		/* 付加データ5 */
	WORD	GetEqu			(void)	{ return m_wEqu;	}		/* 装備できるキャラ情報 */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BYTE	m_byEfcID,					/* 攻撃時に発動するエフェクト番号 */
			m_byMin,					/* 攻撃力減少幅 */
			m_byAtackType,				/* 攻撃種別 */
			m_byData1,					/* 付加データ1 */
			m_byData2,					/* 付加データ2 */
			m_byData3,					/* 付加データ3 */
			m_byData4,					/* 付加データ4 */
			m_byData5;					/* 付加データ5 */
	WORD	m_wEqu;						/* 装備できるキャラ情報 */
} CInfoItemWEAPON, *PCInfoItemWEAPON;

/* Copyright(C)URARA-works 2006 */
