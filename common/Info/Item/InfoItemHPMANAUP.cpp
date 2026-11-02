/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemHPMANAUP.cpp										 */
/* 内容：		アイテム情報(HP・Mana回復)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/16													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemHPMANAUP.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"bRateHP",				/* 確立で回復か判定(HP) */
	"bRateMana",			/* 確立で回復か判定(Mana) */
	"wMinHP",				/* 最低回復値(HP) */
	"wMinMana",				/* 最低回復値(Mana) */
	"wMaxHP",				/* 最大回復値(HP) */
	"wMaxMana",				/* 最大回復値(Mana) */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::CInfoItemHPMANAUP							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemHPMANAUP::CInfoItemHPMANAUP()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::~CInfoItemHPMANAUP							 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemHPMANAUP::~CInfoItemHPMANAUP()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemHPMANAUP::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_HPMANAUP;
	m_bRateHP		= FALSE;
	m_bRateMana		= FALSE;
	m_wMinHP		= 0;
	m_wMinMana		= 0;
	m_wMaxHP		= 0;
	m_wMaxMana		= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemHPMANAUP::Copy(CInfoItemHPMANAUP *pSrc)
{
	CInfoItemBase::Init ();

	m_bRateHP	= pSrc->GetRateHP ();
	m_bRateMana	= pSrc->GetRateMana ();
	m_wMinHP	= pSrc->GetMinHP ();
	m_wMinMana	= pSrc->GetMinMana ();
	m_wMaxHP	= pSrc->GetMaxHP ();
	m_wMaxMana	= pSrc->GetMaxMana ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::GetDataSize									 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

DWORD CInfoItemHPMANAUP::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_bRateHP);
	dwTmp += sizeof (m_bRateMana);
	dwTmp += sizeof (m_wMinHP);
	dwTmp += sizeof (m_wMinMana);
	dwTmp += sizeof (m_wMaxHP);
	dwTmp += sizeof (m_wMaxMana);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::GetData										 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/16														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemHPMANAUP::GetData(void)
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

	CopyMemoryRenew (pTmp, pDataBase,		dwSizeBase,				pTmp);
	CopyMemoryRenew (pTmp, &m_bRateHP,		sizeof (m_bRateHP),		pTmp);
	CopyMemoryRenew (pTmp, &m_bRateMana,	sizeof (m_bRateMana),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMinHP,		sizeof (m_wMinHP),		pTmp);
	CopyMemoryRenew (pTmp, &m_wMinMana,		sizeof (m_wMinMana),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMaxHP,		sizeof (m_wMaxHP),		pTmp);
	CopyMemoryRenew (pTmp, &m_wMaxMana,		sizeof (m_wMaxMana),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::SetData										 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/16														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemHPMANAUP::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_bRateHP,	pTmp, sizeof (m_bRateHP),	pTmp);
	CopyMemoryRenew (&m_bRateMana,	pTmp, sizeof (m_bRateMana),	pTmp);
	CopyMemoryRenew (&m_wMinHP,		pTmp, sizeof (m_wMinHP),	pTmp);
	CopyMemoryRenew (&m_wMinMana,	pTmp, sizeof (m_wMinMana),	pTmp);
	CopyMemoryRenew (&m_wMaxHP,		pTmp, sizeof (m_wMaxHP),	pTmp);
	CopyMemoryRenew (&m_wMaxMana,	pTmp, sizeof (m_wMaxMana),	pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::GetKeyCount									 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPMANAUP::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::GetKeyName									 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemHPMANAUP::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemHPMANAUP::GetKeyData									 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPMANAUP::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 確立で回復か判定(HP) */
		strTmp.Format ("%d", m_bRateHP);
		break;
	case 1:			/* 確立で回復か判定(Mana) */
		strTmp.Format ("%d", m_bRateMana);
		break;
	case 2:			/* 最低回復値(HP) */
		strTmp.Format ("%d", m_wMinHP);
		break;
	case 3:			/* 最低回復値(Mana) */
		strTmp.Format ("%d", m_wMinMana);
		break;
	case 4:			/* 最大回復値(HP) */
		strTmp.Format ("%d", m_wMaxHP);
		break;
	case 5:			/* 最大回復値(Mana) */
		strTmp.Format ("%d", m_wMaxMana);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPMANAUP::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPMANAUP::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemHPMANAUP::SetKeyData									 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPMANAUP::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 確立で回復か判定(HP) */
		m_bRateHP = (BOOL)atoi (pszSrc);
		break;
	case 1:			/* 確立で回復か判定(Mana) */
		m_bRateMana = (BOOL)atoi (pszSrc);
		break;
	case 2:			/* 最低回復値(HP) */
		m_wMinHP = (WORD)atoi (pszSrc);
		break;
	case 3:			/* 最低回復値(Mana) */
		m_wMinMana = (WORD)atoi (pszSrc);
		break;
	case 4:			/* 最大回復値(HP) */
		m_wMaxHP = (WORD)atoi (pszSrc);
		break;
	case 5:			/* 最大回復値(Mana) */
		m_wMaxMana = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
