/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemMSG.h												 */
/* 内容：		アイテム情報(メッセージ表示)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#pragma once

#include "InfoItemBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemMSG : public CInfoItemBase
{
public:
			CInfoItemMSG();						/* コンストラクタ */
	virtual ~CInfoItemMSG();					/* デストラクタ */

	void	Init			(void);						/* 初期化 */
	void	Copy			(CInfoItemMSG *pSrc);		/* コピー */
	DWORD	GetDataSize		(void);						/* データサイズを取得 */
	PBYTE	GetData			(void);						/* データを取得 */
	PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetType			(BYTE byType)		{ m_byType		= byType;		}	/* 攻撃力減少幅 */
	void	SetEfcID		(BYTE byEfcID)		{ m_byEfcID		= byEfcID;		}	/* エフェクト番号 */
	void	SetCharEfc		(LPCSTR pszCharEfc)	{ m_strCharEfc	= pszCharEfc;	}	/* 文字エフェクト */
	void	SetMsgTo		(LPCSTR strMsgTo)	{ m_strMsgTo	= strMsgTo;		}	/* 他のキャラに使った時のメッセージ */
	void	SetMsgMe		(LPCSTR strMsgMe)	{ m_strMsgMe	= strMsgMe;		}	/* 他のキャラに使われた時のメッセージ */


	/* 情報の取得 */
	BYTE	GetType			(void)	{ return m_byType;		}		/* 表示種別 */
	BYTE	GetEfcID		(void)	{ return m_byEfcID;		}		/* エフェクト番号 */
	LPCSTR	GetCharEfc		(void)	{ return m_strCharEfc;	}		/* 文字エフェクト */
	LPCSTR	GetMsgTo		(void)	{ return m_strMsgTo;	}		/* 他のキャラに使った時のメッセージ */
	LPCSTR	GetMsgMe		(void)	{ return m_strMsgMe;	}		/* 他のキャラに使われた時のメッセージ */

	int		GetKeyCount	(void);							/* キーの数を取得 */
	LPCSTR	GetKeyName	(int nNo);						/* キー名を取得 */
	void	GetKeyData	(int nNo, CmyString &strDst);	/* キーに対応するデータを取得 */
	int		GetKeyNo	(LPCSTR pszKey);				/* キー番号を取得 */
	void	SetKeyData	(int nNo, LPCSTR pszSrc);		/* キーに対応するデータを設定 */


protected:
	BYTE		m_byType,				/* 表示種別 */
				m_byEfcID;				/* エフェクト番号 */
	CmyString	m_strCharEfc,			/* 文字エフェクト */
				m_strMsgTo,				/* 他のキャラに使った時のメッセージ */
				m_strMsgMe;				/* 他のキャラに使われた時のメッセージ */
} CInfoItemMSG, *PCInfoItemMSG;

/* Copyright(C)URARA-works 2006 */
