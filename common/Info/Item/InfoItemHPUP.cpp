/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemHPUP.cpp											 */
/* 内容：		アイテム情報(HP回復)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/12													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemHPUP.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"bRate",				/* 確立で回復か判定 */
	"wMin",					/* 最低回復値 */
	"wMax",					/* 最大回復値 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::CInfoItemHPUP									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/12														 */
/* ========================================================================= */

CInfoItemHPUP::CInfoItemHPUP()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::~CInfoItemHPUP									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/12														 */
/* ========================================================================= */

CInfoItemHPUP::~CInfoItemHPUP()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::Init												 */
/* 内容：	初期化															 */
/* 日付：	2006/06/12														 */
/* ========================================================================= */

void CInfoItemHPUP::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_HPUP;
	m_bRate			= FALSE;
	m_wMin			= 0;
	m_wMax			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::Copy												 */
/* 内容：	コピー															 */
/* 日付：	2006/06/12														 */
/* ========================================================================= */

void CInfoItemHPUP::Copy(CInfoItemHPUP *pSrc)
{
	CInfoItemBase::Init ();

	m_bRate	= pSrc->GetRate ();
	m_wMin	= pSrc->GetMin ();
	m_wMax	= pSrc->GetMax ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/12														 */
/* ========================================================================= */

DWORD CInfoItemHPUP::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_bRate);
	dwTmp += sizeof (m_wMin);
	dwTmp += sizeof (m_wMax);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/12														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemHPUP::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_bRate,	sizeof (m_bRate),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMin,		sizeof (m_wMin),	pTmp);
	CopyMemoryRenew (pTmp, &m_wMax,		sizeof (m_wMax),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/12														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemHPUP::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_bRate,	pTmp, sizeof (m_bRate),	pTmp);
	CopyMemoryRenew (&m_wMin,	pTmp, sizeof (m_wMin),	pTmp);
	CopyMemoryRenew (&m_wMax,	pTmp, sizeof (m_wMax),	pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPUP::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemHPUP::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemHPUP::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPUP::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 確立で回復か判定 */
		strTmp.Format ("%d", m_bRate);
		break;
	case 1:			/* 最低回復値 */
		strTmp.Format ("%d", m_wMin);
		break;
	case 2:			/* 最大回復値 */
		strTmp.Format ("%d", m_wMax);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemHPUP::GetKeyNo											 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemHPUP::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemHPUP::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemHPUP::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 確立で回復か判定 */
		m_bRate = (BOOL)atoi (pszSrc);
		break;
	case 1:			/* 最低回復値 */
		m_wMin = (WORD)atoi (pszSrc);
		break;
	case 2:			/* 最大回復値 */
		m_wMax = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
