/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemARMOR.cpp											 */
/* 内容：		アイテム情報(鎧)クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/14													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemARMOR.h"

/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"wEqu",				/* 装備できるキャラ情報 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::CInfoItemARMOR									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/14														 */
/* ========================================================================= */

CInfoItemARMOR::CInfoItemARMOR()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::~CInfoItemARMOR									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/14														 */
/* ========================================================================= */

CInfoItemARMOR::~CInfoItemARMOR()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/14														 */
/* ========================================================================= */

void CInfoItemARMOR::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_ARMOR;
	m_wEqu			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/14														 */
/* ========================================================================= */

void CInfoItemARMOR::Copy(CInfoItemARMOR *pSrc)
{
	CInfoItemBase::Init ();

	m_wEqu = pSrc->GetEqu ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/14														 */
/* ========================================================================= */

DWORD CInfoItemARMOR::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_wEqu);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/14														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemARMOR::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_wEqu,		sizeof (m_wEqu),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/14														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemARMOR::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_wEqu, pTmp, sizeof (m_wEqu), pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/10														 */
/* ========================================================================= */

int CInfoItemARMOR::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/10														 */
/* ========================================================================= */

LPCSTR CInfoItemARMOR::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemARMOR::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/10														 */
/* ========================================================================= */

void CInfoItemARMOR::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 装備できるキャラ情報 */
		strTmp.Format ("%d", m_wEqu);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemARMOR::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemARMOR::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemARMOR::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemARMOR::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 名前 */
		m_wEqu = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
