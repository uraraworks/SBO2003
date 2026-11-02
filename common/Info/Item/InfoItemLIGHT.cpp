/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemLIGHT.cpp											 */
/* 内容：		アイテム情報(キャラ画像変更)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemLIGHT.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"byLevel",				/* 明るさレベル */
	"wTime",				/* 維持時間 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::CInfoItemLIGHT									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemLIGHT::CInfoItemLIGHT()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::~CInfoItemLIGHT									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemLIGHT::~CInfoItemLIGHT()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemLIGHT::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_LIGHT;
	m_byLevel		= 0;
	m_wTime			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemLIGHT::Copy(CInfoItemLIGHT *pSrc)
{
	CInfoItemBase::Init ();

	m_byLevel	= pSrc->GetLevel ();
	m_wTime		= pSrc->GetTime ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

DWORD CInfoItemLIGHT::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_byLevel);
	dwTmp += sizeof (m_wTime);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/15														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemLIGHT::GetData(void)
{
	PBYTE pRet, pRetTmp, pTmp, pDataBase;
	DWORD dwSize, dwSizeBase;

	pRet		= NULL;
	pDataBase	= NULL;

	dwSizeBase	= CInfoItemBase::GetDataSize ();
	dwSize		= GetDataSize ();
	dwSize		+= dwSizeBase;
	pRetTmp		= new BYTE[dwSize];
	pTmp		= pRetTmp;
	ZeroMemory (pTmp, dwSize);

	/* 規定クラスの情報をコピー */
	pDataBase = CInfoItemBase::GetData ();

	CopyMemoryRenew (pTmp, pDataBase,	dwSizeBase,			pTmp);
	CopyMemoryRenew (pTmp, &m_byLevel,	sizeof (m_byLevel),	pTmp);
	CopyMemoryRenew (pTmp, &m_wTime,	sizeof (m_wTime),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/15														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemLIGHT::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_byLevel, pTmp, sizeof (m_byLevel), pTmp);
	CopyMemoryRenew (&m_wTime, pTmp, sizeof (m_wTime), pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemLIGHT::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemLIGHT::GetKeyName(int nNo)
{
	LPCSTR pRet;

	pRet = NULL;

	if (nNo >= m_nKeyCountBase + m_nKeyCount) {
		goto Exit;
	}

	if (nNo < m_nKeyCountBase) {
		pRet = CInfoItemBase::GetKeyName (nNo);
		goto Exit;
	}

	pRet = m_aszKey[nNo - m_nKeyCountBase];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemLIGHT::GetKeyData(int nNo, CmyString &strDst)
{
	CmyString strTmp;

	if (nNo >= m_nKeyCountBase + m_nKeyCount) {
		goto Exit;
	}

	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::GetKeyData (nNo, strTmp);
		goto Exit;
	}

	switch (nNo - m_nKeyCountBase) {
	case 0:			/* 明るさレベル */
		strTmp.Format ("%d", m_byLevel);
		break;
	case 1:			/* 維持時間 */
		strTmp.Format ("%d", m_wTime);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemLIGHT::GetKeyNo(LPCSTR pszKey)
{
	int i, nRet;

	nRet = CInfoItemBase::GetKeyNo (pszKey);
	if (nRet >= 0) {
		goto Exit;
	}

	for (i = 0; i < m_nKeyCount; i ++) {
		if (strcmp (m_aszKey[i], pszKey) != 0) {
			continue;
		}
		nRet = m_nKeyCountBase + i;
		break;
	}

Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemLIGHT::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemLIGHT::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 明るさレベル */
		m_byLevel = (BYTE)atoi (pszSrc);
		break;
	case 1:			/* 維持時間 */
		m_wTime = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
