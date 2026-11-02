/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemGRPCHG.cpp											 */
/* 内容：		アイテム情報(キャラ画像変更)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/15													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemGRPCHG.h"


/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"wGrpID",			/* 変更するグラフィックID（０は記録した画像） */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::CInfoItemGRPCHG								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemGRPCHG::CInfoItemGRPCHG()
{
	int i;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCount ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::~CInfoItemGRPCHG								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

CInfoItemGRPCHG::~CInfoItemGRPCHG()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::Init											 */
/* 内容：	初期化															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemGRPCHG::Init(void)
{
	CInfoItemBase::Init ();

	m_dwItemTypeID	= ITEMTYPE_GRPCHG;
	m_wGrpID		= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::Copy											 */
/* 内容：	コピー															 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

void CInfoItemGRPCHG::Copy(CInfoItemGRPCHG *pSrc)
{
	CInfoItemBase::Init ();

	m_wGrpID = pSrc->GetGrpID ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::GetDataSize									 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/06/15														 */
/* ========================================================================= */

DWORD CInfoItemGRPCHG::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += CInfoItemBase::GetDataSize ();
	dwTmp += sizeof (m_wGrpID);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::GetData										 */
/* 内容：	データを取得													 */
/* 日付：	2006/06/15														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemGRPCHG::GetData(void)
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
	CopyMemoryRenew (pTmp, &m_wGrpID,	sizeof (m_wGrpID),	pTmp);

	pRet = pRetTmp;
	SAFE_DELETE_ARRAY (pDataBase);

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::SetData										 */
/* 内容：	データを設定													 */
/* 日付：	2006/06/15														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemGRPCHG::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	pTmp = CInfoItemBase::SetData (pData);

	CopyMemoryRenew (&m_wGrpID, pTmp, sizeof (m_wGrpID), pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::GetKeyCount									 */
/* 内容：	キーの数を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemGRPCHG::GetKeyCount(void)
{
	return (m_nKeyCountBase + m_nKeyCount);
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

LPCSTR CInfoItemGRPCHG::GetKeyName(int nNo)
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
/* 関数名：	CInfoItemGRPCHG::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemGRPCHG::GetKeyData(int nNo, CmyString &strDst)
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
	case 0:			/* 変更するグラフィックID（０は記録した画像） */
		strTmp.Format ("%d", m_wGrpID);
		break;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemGRPCHG::GetKeyNo										 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemGRPCHG::GetKeyNo(LPCSTR pszKey)
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
/* 関数名：	CInfoItemGRPCHG::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemGRPCHG::SetKeyData(int nNo, LPCSTR pszSrc)
{
	if (nNo < m_nKeyCountBase) {
		CInfoItemBase::SetKeyData (nNo, pszSrc);
		return;
	}

	nNo -= m_nKeyCountBase;

	switch (nNo) {
	case 0:			/* 変更するグラフィックID（０は記録した画像） */
		m_wGrpID = (WORD)atoi (pszSrc);
		break;
	}
}

/* Copyright(C)URARA-works 2006 */
