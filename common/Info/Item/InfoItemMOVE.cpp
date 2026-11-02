/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemMOVE.cpp											 */
/* 内容：		アイテム情報(指定座標へ移動)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/07													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemMOVE.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"dwEnableMapID",			/* 使用限定するマップID(-1:限定しない) */
	"dwMapID",					/* 移動先マップID */
	"ptPosX",					/* 移動先座標X */
	"ptPosY",					/* 移動先座標Y */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::CInfoItemMOVE									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/07														 */
/* ========================================================================= */

CInfoItemMOVE::CInfoItemMOVE()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::~CInfoItemMOVE									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/07														 */
/* ========================================================================= */

CInfoItemMOVE::~CInfoItemMOVE()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::Init												 */
/* 内容：	初期化															 */
/* 日付：	2006/06/07														 */
/* ========================================================================= */

void CInfoItemMOVE::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_RURA;
	m_dwEnableMapID	= -1;
	m_dwMapID		= -1;

	m_ptPos.x = m_ptPos.y = -1;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::Copy												 */
/* 内容：	コピー															 */
/* 日付：	2006/06/07														 */
/* ========================================================================= */

void CInfoItemMOVE::Copy(CInfoItemMOVE *pSrc)
{
	CInfoItemBase::Init ();

	m_dwEnableMapID	= pSrc->GetEnableMapID ();
	m_dwMapID		= pSrc->GetMapID ();
	m_ptPos			= pSrc->GetPos ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/07														 */
/* ========================================================================= */

DWORD CInfoItemMOVE::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_dwEnableMapID);
	dwTmp += sizeof (m_dwMapID);
	dwTmp += sizeof (m_ptPos);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/07														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemMOVE::GetData(void)
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

	CopyMemoryRenew (pTmp, pDataBase,			dwSizeBase,					pTmp);
	CopyMemoryRenew (pTmp, &m_dwEnableMapID,	sizeof (m_dwEnableMapID),	pTmp);
	CopyMemoryRenew (pTmp, &m_dwMapID,			sizeof (m_dwMapID),			pTmp);
	CopyMemoryRenew (pTmp, &m_ptPos,			sizeof (m_ptPos),			pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/07														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemMOVE::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_dwEnableMapID,	pTmp, sizeof (m_dwEnableMapID),	pTmp);
	CopyMemoryRenew (&m_dwMapID,		pTmp, sizeof (m_dwMapID),		pTmp);
	CopyMemoryRenew (&m_ptPos,			pTmp, sizeof (m_ptPos),			pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMOVE::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemMOVE::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemMOVE::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMOVE::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 使用限定するマップID(-1:限定しない) */
		strTmp.Format ("%u", m_dwEnableMapID);
		break;
	case 1:			/* 移動先マップID */
		strTmp.Format ("%d", m_dwMapID);
		break;
	case 2:			/* 移動先座標X */
		strTmp.Format ("%d", m_ptPos.x);
		break;
	case 3:			/* 移動先座標Y */
		strTmp.Format ("%d", m_ptPos.y);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMOVE::GetKeyNo											 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMOVE::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemMOVE::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMOVE::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 使用限定するマップID(-1:限定しない) */
		m_dwEnableMapID = (DWORD)atoi (pszSrc);
		break;
	case 1:			/* 移動先マップID */
		m_dwMapID = (DWORD)atoi (pszSrc);
		break;
	case 2:			/* 移動先座標X */
		m_ptPos.x = atoi (pszSrc);
		break;
	case 3:			/* 移動先座標Y */
		m_ptPos.y = atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
