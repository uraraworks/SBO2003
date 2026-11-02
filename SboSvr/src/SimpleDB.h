/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	SimpleDB.h													 */
/* 内容：		簡易データベースクラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/25													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* データ情報 */
typedef struct _SIMPLEDB_DATAINFO {
	CmyString		strData;					/* データ */
} SIMPLEDB_DATAINFO, *PSIMPLEDB_DATAINFO;
typedef CmyArray <PSIMPLEDB_DATAINFO, PSIMPLEDB_DATAINFO>   ARRAYSIMPLEDBDATAINFO;
typedef CmyArray <PSIMPLEDB_DATAINFO, PSIMPLEDB_DATAINFO> *PARRAYSIMPLEDBDATAINFO;

/* キー情報 */
typedef struct _SIMPLEDB_KEYINFO {
	CmyString				strKey;				/* キー */
	ARRAYSIMPLEDBDATAINFO	aDataInfo;			/* データ情報 */
} SIMPLEDB_KEYINFO, *PSIMPLEDB_KEYINFO;
typedef CmyArray <PSIMPLEDB_KEYINFO, PSIMPLEDB_KEYINFO>   ARRAYSIMPLEDBKEYINFO;
typedef CmyArray <PSIMPLEDB_KEYINFO, PSIMPLEDB_KEYINFO> *PARRAYSIMPLEDBKEYINFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CSimpleDB
{
public:
			CSimpleDB();								/* コンストラクタ */
	virtual ~CSimpleDB();								/* デストラクタ */

	BOOL	Create		(LPCSTR pszFileName);					/* 作成 */
	void	Destroy		(void);									/* 破棄 */
	BOOL	Save		(void);									/* 保存 */
	BOOL	Load		(void);									/* 読み込み */
	void	Add			(LPCSTR pszKey, LPCSTR pszData);		/* 追加 */
	int		GetCount	(LPCSTR pszKey);						/* データ数を取得 */
	BOOL	Get			(LPCSTR pszKey, int nNo, LPSTR pszDst);	/* 取得 */

	PSIMPLEDB_KEYINFO	GetKeyInfoPtr	(LPCSTR pszKey);		/* キー情報を取得 */

	DWORD	GetDataInfoSize		(PSIMPLEDB_DATAINFO pDataInfo);			/* データ情報サイズを取得 */
	PBYTE	GetDataInfo			(PSIMPLEDB_DATAINFO pDataInfo);			/* データ情報を取得 */
	PBYTE	SetDataInfo			(PBYTE pSrc, PSIMPLEDB_DATAINFO pDst);	/* データ情報を取得 */
	void	DeleteDataInfo		(PSIMPLEDB_DATAINFO pDataInfo);			/* データ情報を削除 */
	void	DeleteAllDataInfo	(PARRAYSIMPLEDBDATAINFO paDataInfo);	/* データ情報を全て削除 */

	DWORD	GetKeyInfoSize		(PSIMPLEDB_KEYINFO pKeyInfo);			/* キー情報サイズを取得 */
	PBYTE	GetKeyInfo			(PSIMPLEDB_KEYINFO pKeyInfo);			/* キー情報を取得 */
	PBYTE	SetKeyInfo			(PBYTE pSrc, PSIMPLEDB_KEYINFO pDst);	/* キー情報を取得 */
	void	DeleteKeyInfo		(PSIMPLEDB_KEYINFO pKeyInfo);			/* キー情報を削除 */
	void	DeleteAllKeyInfo	(void);									/* キー情報を全て削除 */


protected:
	CmyString				m_strFileName;				/* ファイル名 */
	ARRAYSIMPLEDBKEYINFO	m_aKeyInfo;					/* キー情報 */
} CSimpleDB, *PCSimpleDB;

/* Copyright(C)URARA-works 2006 */
