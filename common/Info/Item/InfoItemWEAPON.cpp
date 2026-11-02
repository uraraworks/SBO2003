/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemWEAPON.cpp											 */
/* 内容：		アイテム情報(武器)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/13													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemWEAPON.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"byEfcID",				/* 攻撃時に発動するエフェクト番号 */
	"Min",					/* 攻撃力減少幅 */
	"AtackType",			/* 攻撃種別 */
	"Data1",				/* 付加データ1 */
	"Data2",				/* 付加データ2 */
	"Data3",				/* 付加データ3 */
	"Data4",				/* 付加データ4 */
	"Data5",				/* 付加データ5 */
	"wEqu",					/* 装備できるキャラ情報 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::CInfoItemWEAPON								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/13														 */
/* ========================================================================= */

CInfoItemWEAPON::CInfoItemWEAPON()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::~CInfoItemWEAPON								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/13														 */
/* ========================================================================= */

CInfoItemWEAPON::~CInfoItemWEAPON()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/13														 */
/* ========================================================================= */

void CInfoItemWEAPON::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_WEAPON;
	m_byEfcID		= 0;
	m_byMin			= 0;
	m_byAtackType	= 0;
	m_byData1		= 0;
	m_byData2		= 0;
	m_byData3		= 0;
	m_byData4		= 0;
	m_byData5		= 0;
	m_wEqu			= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/13														 */
/* ========================================================================= */

void CInfoItemWEAPON::Copy(CInfoItemWEAPON *pSrc)
{
	CInfoItemBase::Init ();

	m_byEfcID		= pSrc->GetEfcID ();
	m_byMin			= pSrc->GetMin ();
	m_byAtackType	= pSrc->GeAtackType ();
	m_byData1		= pSrc->GetData1 ();
	m_byData2		= pSrc->GetData2 ();
	m_byData3		= pSrc->GetData3 ();
	m_byData4		= pSrc->GetData4 ();
	m_byData5		= pSrc->GetData5 ();
	m_wEqu			= pSrc->GetEqu ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::GetDataSize									 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/13														 */
/* ========================================================================= */

DWORD CInfoItemWEAPON::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_byEfcID);
	dwTmp += sizeof (m_byMin);
	dwTmp += sizeof (m_byAtackType);
	dwTmp += sizeof (m_byData1);
	dwTmp += sizeof (m_byData2);
	dwTmp += sizeof (m_byData3);
	dwTmp += sizeof (m_byData4);
	dwTmp += sizeof (m_byData5);
	dwTmp += sizeof (m_wEqu);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::GetData										 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/13														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemWEAPON::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_byEfcID,		sizeof (m_byEfcID),		pTmp);
	CopyMemoryRenew (pTmp, &m_byMin,		sizeof (m_byMin),		pTmp);
	CopyMemoryRenew (pTmp, &m_byAtackType,	sizeof (m_byAtackType),	pTmp);
	CopyMemoryRenew (pTmp, &m_byData1,		sizeof (m_byData1),		pTmp);
	CopyMemoryRenew (pTmp, &m_byData2,		sizeof (m_byData2),		pTmp);
	CopyMemoryRenew (pTmp, &m_byData3,		sizeof (m_byData3),		pTmp);
	CopyMemoryRenew (pTmp, &m_byData4,		sizeof (m_byData4),		pTmp);
	CopyMemoryRenew (pTmp, &m_byData5,		sizeof (m_byData5),		pTmp);
	CopyMemoryRenew (pTmp, &m_wEqu,			sizeof (m_wEqu),		pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::SetData										 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/13														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemWEAPON::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_byEfcID,		pTmp, sizeof (m_byEfcID),		pTmp);
	CopyMemoryRenew (&m_byMin,			pTmp, sizeof (m_byMin),			pTmp);
	CopyMemoryRenew (&m_byAtackType,	pTmp, sizeof (m_byAtackType),	pTmp);
	CopyMemoryRenew (&m_byData1,		pTmp, sizeof (m_byData1),		pTmp);
	CopyMemoryRenew (&m_byData2,		pTmp, sizeof (m_byData2),		pTmp);
	CopyMemoryRenew (&m_byData3,		pTmp, sizeof (m_byData3),		pTmp);
	CopyMemoryRenew (&m_byData4,		pTmp, sizeof (m_byData4),		pTmp);
	CopyMemoryRenew (&m_byData5,		pTmp, sizeof (m_byData5),		pTmp);
	CopyMemoryRenew (&m_wEqu,			pTmp, sizeof (m_wEqu),			pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemWEAPON::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemWEAPON::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemWEAPON::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemWEAPON::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 攻撃時に発動するエフェクト番号 */
		strTmp.Format ("%d", m_byEfcID);
		break;
	case 1:			/* 攻撃力減少幅 */
		strTmp.Format ("%d", m_byMin);
		break;
	case 2:			/* 攻撃種別 */
		strTmp.Format ("%d", m_byAtackType);
		break;
	case 3:			/* 付加データ1 */
		strTmp.Format ("%d", m_byData1);
		break;
	case 4:			/* 付加データ2 */
		strTmp.Format ("%d", m_byData2);
		break;
	case 5:			/* 付加データ3 */
		strTmp.Format ("%d", m_byData3);
		break;
	case 6:			/* 付加データ4 */
		strTmp.Format ("%d", m_byData4);
		break;
	case 7:			/* 付加データ5 */
		strTmp.Format ("%d", m_byData5);
		break;
	case 8:			/* 装備できるキャラ情報 */
		strTmp.Format ("%d", m_wEqu);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemWEAPON::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemWEAPON::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemWEAPON::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemWEAPON::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 攻撃時に発動するエフェクト番号 */
		m_byEfcID = (BYTE)atoi (pszSrc);
		break;
	case 1:			/* 攻撃力減少幅 */
		m_byMin = (BYTE)atoi (pszSrc);
		break;
	case 2:			/* 攻撃種別 */
		m_byAtackType = (BYTE)atoi (pszSrc);
		break;
	case 3:			/* 付加データ1 */
		m_byData1 = (BYTE)atoi (pszSrc);
		break;
	case 4:			/* 付加データ2 */
		m_byData2 = (BYTE)atoi (pszSrc);
		break;
	case 5:			/* 付加データ3 */
		m_byData3 = (BYTE)atoi (pszSrc);
		break;
	case 6:			/* 付加データ4 */
		m_byData4 = (BYTE)atoi (pszSrc);
		break;
	case 7:			/* 付加データ5 */
		m_byData5 = (BYTE)atoi (pszSrc);
		break;
	case 8:			/* 装備できるキャラ情報 */
		m_wEqu = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
