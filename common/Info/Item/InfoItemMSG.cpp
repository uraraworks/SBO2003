/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemMSG.cpp												 */
/* 内容：		アイテム情報(メッセージ表示)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemMSG.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"byType",				/* 確立で回復か判定 */
	"byEfcID",				/* エフェクト番号 */
	"strCharEfc",			/* 文字エフェクト */
	"strMsgTo",				/* 他のキャラに使った時のメッセージ */
	"strMsgMe",				/* 他のキャラに使われた時のメッセージ */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::CInfoItemMSG										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemMSG::CInfoItemMSG()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::~CInfoItemMSG										 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemMSG::~CInfoItemMSG()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::Init												 */
/* 内容：	初期化															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemMSG::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_MSG;
	m_byType		= 0;
	m_byEfcID		= 0;
	m_strCharEfc.	Empty ();
	m_strMsgTo.		Empty ();
	m_strMsgMe.		Empty ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::Copy												 */
/* 内容：	コピー															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemMSG::Copy(CInfoItemMSG *pSrc)
{
	CInfoItemBase::Init ();

	m_byType		= pSrc->GetType ();
	m_byEfcID		= pSrc->GetEfcID ();
	m_strCharEfc	= pSrc->GetCharEfc ();
	m_strMsgTo		= pSrc->GetMsgTo ();
	m_strMsgMe		= pSrc->GetMsgMe ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

DWORD CInfoItemMSG::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_byType);
	dwTmp += sizeof (m_byEfcID);
	dwTmp += (m_strCharEfc.GetLength () + 1);
	dwTmp += (m_strMsgTo.GetLength () + 1);
	dwTmp += (m_strMsgMe.GetLength () + 1);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/15														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemMSG::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_byType,	sizeof (m_byType),	pTmp);
	CopyMemoryRenew (pTmp, &m_byEfcID,	sizeof (m_byEfcID),	pTmp);
	strcpyRenew ((LPSTR)pTmp, m_strCharEfc,	pTmp);
	strcpyRenew ((LPSTR)pTmp, m_strMsgTo,	pTmp);
	strcpyRenew ((LPSTR)pTmp, m_strMsgMe,	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/15														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemMSG::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_byType,		pTmp, sizeof (m_byType),	pTmp);
	CopyMemoryRenew (&m_byEfcID,	pTmp, sizeof (m_byEfcID),	pTmp);
	SetCharEfc ((LPCSTR)pTmp);
	pTmp += m_strCharEfc.GetLength ();
	SetMsgTo ((LPCSTR)pTmp);
	pTmp += m_strMsgTo.GetLength ();
	SetMsgMe ((LPCSTR)pTmp);
	pTmp += m_strMsgMe.GetLength ();

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::GetKeyCount										 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMSG::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemMSG::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemMSG::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMSG::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 表示種別 */
		strTmp.Format ("%d", m_byType);
		break;
	case 1:			/* エフェクト番号 */
		strTmp.Format ("%d", m_byEfcID);
		break;
	case 2:			/* 文字エフェクト */
		strTmp = m_strCharEfc;
		break;
	case 3:			/* 他のキャラに使った時のメッセージ */
		strTmp = m_strMsgTo;
		break;
	case 4:			/* 他のキャラに使われた時のメッセージ */
		strTmp = m_strMsgMe;
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemMSG::GetKeyNo											 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemMSG::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemMSG::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemMSG::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 表示種別 */
		m_byType = (BYTE)atoi (pszSrc);
		break;
	case 1:			/* エフェクト番号 */
		m_byEfcID = (BYTE)atoi (pszSrc);
		break;
	case 2:			/* 文字エフェクト */
		m_strCharEfc = pszSrc;
		break;
	case 3:			/* 他のキャラに使った時のメッセージ */
		m_strMsgTo = pszSrc;
		break;
	case 4:			/* 他のキャラに使われた時のメッセージ */
		m_strMsgMe = pszSrc;
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
