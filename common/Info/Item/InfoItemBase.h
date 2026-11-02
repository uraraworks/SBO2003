/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemBase.h												 */
/* 内容：		アイテム情報基底クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/23													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoItemBase
{
public:
			CInfoItemBase();							/* コンストラクタ */
	virtual ~CInfoItemBase();							/* デストラクタ */

	virtual void	Init			(void);						/* 初期化 */
	virtual void	Copy			(CInfoItemBase *pSrc);		/* コピー */
	virtual DWORD	GetDataSize		(void);						/* データサイズを取得 */
	virtual PBYTE	GetData			(void);						/* データを取得 */
	virtual PBYTE	SetData			(PBYTE pData);				/* データを設定 */

	/* 情報の設定 */
	void	SetName			(LPCSTR pszName)		{ m_strName			= pszName;		}	/* 名前 */
	void	SetMsg			(LPCSTR pszMsg)			{ m_strMsg			= pszMsg;		}	/* 使った時のメッセージ */
	void	SetComment		(LPCSTR pszComment)		{ m_strComment		= pszComment;	}	/* 説明 */
	void	SetTypeBase		(BYTE byTypeBase)		{ m_byTypeBase		= byTypeBase;	}	/* 種別 */
	void	SetFlg			(BYTE byFlg)			{ m_byFlg			= byFlg;		}	/* フラグ */
	void	SetSound		(BYTE bySound)			{ m_bySound			= bySound;		}	/* 効果音 */
	void	SetSex			(BYTE bySex)			{ m_bySex			= bySex;		}	/* 性別 */
	void	SetEraseRate	(BYTE byEraseRate)		{ m_byEraseRate		= byEraseRate;	}	/* 破損率 */
	void	SetItemID		(DWORD dwItemID)		{ m_dwItemID		= dwItemID;		}	/* アイテムID */
	void	SetItemTypeID	(DWORD dwItemTypeID)	{ m_dwItemTypeID	= dwItemTypeID;	}	/* アイテム種別ID */
	void	SetGold			(DWORD dwGold)			{ m_dwGold			= dwGold;		}	/* 価値 */

	/* 情報の取得 */
	LPCSTR	GetName			(void)	{ return m_strName;			}	/* 名前 */
	LPCSTR	GetMsg			(void)	{ return m_strMsg;			}	/* 使った時のメッセージ */
	LPCSTR	GetComment		(void)	{ return m_strComment;		}	/* 説明 */
	BYTE	GetTypeBase		(void)	{ return m_byTypeBase;		}	/* 種別 */
	BYTE	GetFlg			(void)	{ return m_byFlg;			}	/* フラグ */
	BYTE	GetSound		(void)	{ return m_bySound;			}	/* 効果音 */
	BYTE	GetSex			(void)	{ return m_bySex;			}	/* 性別 */
	BYTE	GetEraseRate	(void)	{ return m_byEraseRate;		}	/* 破損率 */
	DWORD	GetItemID		(void)	{ return m_dwItemID;		}	/* アイテムID */
	DWORD	GetItemTypeID	(void)	{ return m_dwItemTypeID;	}	/* アイテム種別ID */
	DWORD	GetGold			(void)	{ return m_dwGold;			}	/* 価値 */

	virtual int		GetKeyCount	(void)	{ return m_nKeyCountBase;	}	/* キーの数を取得 */

	virtual LPCSTR	GetKeyName	(int nNo);							/* キー名を取得 */
	virtual void	GetKeyData	(int nNo, CmyString &strDst);		/* キーに対応するデータを取得 */
	virtual int		GetKeyNo	(LPCSTR pszKey);					/* キー番号を取得 */
	virtual void	SetKeyData	(int nNo, LPCSTR pszSrc);			/* キーに対応するデータを設定 */


protected:
	void	CopyMemoryRenew	(PVOID pDst, CONST PVOID pSrc, DWORD dwSize, PBYTE &pPos);	/* メモリコピーしてポインタを進める */
	void	strcpyRenew		(LPSTR pszDst, LPCSTR pszSrc, PBYTE &pPos);					/* 文字列コピーしてポインタを進める */


protected:
	int			m_nKeyCountBase,		/* キーの数(基底) */
				m_nKeyCount;			/* キーの数(派生先) */
	CmyString	m_strName,				/* 名前 */
				m_strMsg,				/* 使った時のメッセージ */
				m_strComment;			/* 説明 */
	BYTE		m_byTypeBase,			/* 種別 */
				m_byFlg,				/* フラグ */
				m_bySound,				/* 効果音 */
				m_bySex,				/* 性別 */
				m_byEraseRate;			/* 破損率 */
	DWORD		m_dwItemID,				/* アイテムID */
				m_dwItemTypeID,			/* アイテム種別ID */
				m_dwGold;				/* 価値 */
} CInfoItemBase, *PCInfoItemBase;
typedef CmyArray <PCInfoItemBase, PCInfoItemBase>		  ARRAYINFOITEM;
typedef CmyArray <PCInfoItemBase, PCInfoItemBase>		*PARRAYINFOITEM;

/* Copyright(C)URARA-works 2006 */
