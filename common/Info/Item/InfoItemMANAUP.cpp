/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CInfoItemMANAUP.cpp											 */
/* 内容：		アイテム情報(Mana回復)クラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/16													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemMANAUP.h"


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
/* 関数名：	CInfoItemMANAUP::CInfoItemMANAUP								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemMANAUP::CInfoItemMANAUP()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMANAUP::~CInfoItemMANAUP								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

CInfoItemMANAUP::~CInfoItemMANAUP()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemMANAUP::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemMANAUP::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_MANAUP;
	m_bRate			= FALSE;
	m_wMin			= 0;
	m_wMax			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMANAUP::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

void CInfoItemMANAUP::Copy(CInfoItemMANAUP *pSrc)
{
	CInfoItemBase::Init ();

	m_bRate	= pSrc->GetRate ();
	m_wMin	= pSrc->GetMin ();
	m_wMax	= pSrc->GetMax ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMANAUP::GetDataSize									 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/16														 */
/* ========================================================================= */

DWORD CInfoItemMANAUP::GetDataSize(void)
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
/* 関数名：	CInfoItemMANAUP::GetData										 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/16														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemMANAUP::GetData(void)
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
/* 関数名：	CInfoItemMANAUP::SetData										 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/16														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemMANAUP::SetData(PBYTE pData)
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
/* 関数名：	CInfoItemMANAUP::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMANAUP::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemMANAUP::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemMANAUP::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemMANAUP::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMANAUP::GetKeyData(int nNo, CmyString &strDst)
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
/* 関数名：	CInfoItemMANAUP::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMANAUP::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemMANAUP::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMANAUP::SetKeyData(int nNo, LPCSTR pszSrc)
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
