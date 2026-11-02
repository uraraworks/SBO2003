/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemHPDOWN.cpp											 */
/* 内容：		アイテム情報(HP減少)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/16													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemHPDOWN.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"byEfcID",				/* エフェクト番号 */
	"bRate",				/* 確立で減少か判定 */
	"wMin",					/* 最低減少値 */
	"wMax",					/* 最大減少値 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::CInfoItemHPDOWN								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemHPDOWN::CInfoItemHPDOWN()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::~CInfoItemHPDOWN								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemHPDOWN::~CInfoItemHPDOWN()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemHPDOWN::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_HPDOWN;
	m_byEfcID		= 0;
	m_bRate			= FALSE;
	m_wMin			= 0;
	m_wMax			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemHPDOWN::Copy(CInfoItemHPDOWN *pSrc)
{
	CInfoItemBase::Init ();

	m_byEfcID	= pSrc->GetEfcID ();
	m_bRate		= pSrc->GetRate ();
	m_wMin		= pSrc->GetMin ();
	m_wMax		= pSrc->GetMax ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::GetDataSize									 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

DWORD CInfoItemHPDOWN::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_byEfcID);
	dwTmp += sizeof (m_bRate);
	dwTmp += sizeof (m_wMin);
	dwTmp += sizeof (m_wMax);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::GetData										 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/16														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemHPDOWN::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_byEfcID,	sizeof (m_byEfcID),	pTmp);
	CopyMemoryRenew (pTmp, &m_bRate,	sizeof (m_bRate),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMin,		sizeof (m_wMin),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMax,		sizeof (m_wMax),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::SetData										 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/16														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemHPDOWN::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_byEfcID,	pTmp, sizeof (m_byEfcID),	pTmp);
	CopyMemoryRenew (&m_bRate,		pTmp, sizeof (m_bRate),		pTmp);
	CopyMemoryRenew (&m_wMin,		pTmp, sizeof (m_wMin),		pTmp);
	CopyMemoryRenew (&m_wMax,		pTmp, sizeof (m_wMax),		pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::GetKeyCount									 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPDOWN::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemHPDOWN::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemHPDOWN::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPDOWN::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* エフェクト番号 */
		strTmp.Format ("%d", m_byEfcID);
		break;
	case 1:			/* 確立で減少か判定 */
		strTmp.Format ("%d",m_bRate);
		break;
	case 2:			/* 最低減少値 */
		strTmp.Format ("%d", m_wMin);
		break;
	case 3:			/* 最大減少値 */
		strTmp.Format ("%d", m_wMax);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPDOWN::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPDOWN::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemHPDOWN::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPDOWN::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* エフェクト番号 */
		m_byEfcID = (BYTE)atoi (pszSrc);
		break;
	case 1:			/* 確立で減少か判定 */
		m_bRate = (BOOL)atoi (pszSrc);
		break;
	case 2:			/* 最低減少値 */
		m_wMin = (WORD)atoi (pszSrc);
		break;
	case 3:			/* 最大減少値 */
		m_wMax = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
