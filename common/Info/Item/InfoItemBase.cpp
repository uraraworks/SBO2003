/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoItemBase.cpp											 */
/* 内容：		アイテム情報基底クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/23													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "InfoItemBase.h"

/* ========================================================================= */
/* ローカル変数の定義														 */
/* ========================================================================= */

static LPCSTR m_aszKey[] = {
	"strName",				/* 名前 */
	"strMsg",				/* 使った時のメッセージ */
	"strComment",			/* 説明 */
	"byTypeBase",				/* 種別 */
	"byFlg",				/* フラグ */
	"bySound",				/* 効果音 */
	"bySex",				/* 性別 */
	"byEraseRate",			/* 破損率 */
	"dwItemID",				/* アイテムID */
	"dwItemTypeID",			/* アイテム種別ID */
	"dwGold",				/* 価値 */
	NULL
};


/* ========================================================================= */
/* 関数名：	CInfoItemBase::CInfoItemBase									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/05/23														 */
/* ========================================================================= */

CInfoItemBase::CInfoItemBase()
{
	int i;

	m_nKeyCountBase	= 0;
	m_nKeyCount		= 0;

	for (i = 0; m_aszKey[i] != NULL; i ++) {
		m_nKeyCountBase ++;
	}

	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::~CInfoItemBase									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/05/23														 */
/* ========================================================================= */

CInfoItemBase::~CInfoItemBase()
{
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::Init												 */
/* 内容：	初期化															 */
/* 日付：	2006/05/24														 */
/* ========================================================================= */

void CInfoItemBase::Init(void)
{
	m_byTypeBase		= 0;
	m_byFlg			= 0;
	m_bySound		= 0;
	m_bySex			= 0;
	m_byEraseRate	= 0;
	m_dwItemID		= 0;
	m_dwItemTypeID	= 0;
	m_dwGold		= 0;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::Copy												 */
/* 内容：	コピー															 */
/* 日付：	2006/05/24														 */
/* ========================================================================= */

void CInfoItemBase::Copy(CInfoItemBase *pSrc)
{
	Init ();

	SetName		(pSrc->GetName ());
	SetMsg		(pSrc->GetMsg ());
	SetComment	(pSrc->GetComment ());
	m_byTypeBase	= pSrc->GetTypeBase ();
	m_byFlg			= pSrc->GetFlg ();
	m_bySound		= pSrc->GetSound ();
	m_bySex			= pSrc->GetSex ();
	m_byEraseRate	= pSrc->GetEraseRate ();
	m_dwItemTypeID	= pSrc->GetItemTypeID ();
	m_dwGold		= pSrc->GetGold ();
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::GetDataSize										 */
/* 内容：	データサイズを取得												 */
/* 日付：	2006/05/24														 */
/* ========================================================================= */

DWORD CInfoItemBase::GetDataSize(void)
{
	DWORD dwRet, dwTmp;

	dwRet	= 0;
	dwTmp	= 0;

	dwTmp += m_strName.GetLength ();
	dwTmp ++;
	dwTmp += m_strMsg.GetLength ();
	dwTmp ++;
	dwTmp += m_strComment.GetLength ();
	dwTmp += sizeof (m_byTypeBase);
	dwTmp += sizeof (m_byFlg);
	dwTmp += sizeof (m_bySound);
	dwTmp += sizeof (m_bySex);
	dwTmp += sizeof (m_byEraseRate);
	dwTmp += sizeof (m_dwItemTypeID);
	dwTmp += sizeof (m_dwGold);

	dwRet = dwTmp;
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/05/24														 */
/* メモ：	開放は取得側で行うこと											 */
/* ========================================================================= */

PBYTE CInfoItemBase::GetData(void)
{
	PBYTE pRet, pRetTmp, pTmp;
	DWORD dwSize;

	pRet	= NULL;
	dwSize	= GetDataSize ();
	pRetTmp	= new BYTE[dwSize];
	pTmp	= pRetTmp;

	ZeroMemory (pTmp, dwSize);

	strcpyRenew ((LPSTR)pTmp, m_strName,	pTmp);
	strcpyRenew ((LPSTR)pTmp, m_strMsg,		pTmp);
	strcpyRenew ((LPSTR)pTmp, m_strComment,	pTmp);
	CopyMemoryRenew (pTmp, &m_byTypeBase,	sizeof (m_byTypeBase),		pTmp);
	CopyMemoryRenew (pTmp, &m_byFlg,		sizeof (m_byFlg),			pTmp);
	CopyMemoryRenew (pTmp, &m_bySound,		sizeof (m_bySound),			pTmp);
	CopyMemoryRenew (pTmp, &m_bySex,		sizeof (m_bySex),			pTmp);
	CopyMemoryRenew (pTmp, &m_byEraseRate,	sizeof (m_byEraseRate),		pTmp);
	CopyMemoryRenew (pTmp, &m_dwItemTypeID,	sizeof (m_dwItemTypeID),	pTmp);
	CopyMemoryRenew (pTmp, &m_dwGold,		sizeof (m_dwGold),			pTmp);

	pRet = pRetTmp;

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::SetData											 */
/* 内容：	データを設定													 */
/* 日付：	2006/05/24														 */
/* 戻り値：	コピー後のアドレス												 */
/* ========================================================================= */

PBYTE CInfoItemBase::SetData(PBYTE pData)
{
	PBYTE pTmp, pRet;

	Init ();
	pTmp = pData;

	SetName ((LPCSTR)pTmp);
	pTmp += m_strName.GetLength ();
	SetMsg ((LPCSTR)pTmp);
	pTmp += m_strMsg.GetLength ();
	SetComment ((LPCSTR)pTmp);
	pTmp += m_strComment.GetLength ();
	CopyMemoryRenew (&m_byTypeBase,		pTmp, sizeof (m_byTypeBase),	pTmp);
	CopyMemoryRenew (&m_byFlg,			pTmp, sizeof (m_byFlg),			pTmp);
	CopyMemoryRenew (&m_bySound,		pTmp, sizeof (m_bySound),		pTmp);
	CopyMemoryRenew (&m_bySex,			pTmp, sizeof (m_bySex),			pTmp);
	CopyMemoryRenew (&m_byEraseRate,	pTmp, sizeof (m_byEraseRate),	pTmp);
	CopyMemoryRenew (&m_dwItemTypeID,	pTmp, sizeof (m_dwItemTypeID),	pTmp);
	CopyMemoryRenew (&m_dwGold,			pTmp, sizeof (m_dwGold),		pTmp);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::GetKeyName										 */
/* 内容：	キー名を取得													 */
/* 日付：	2006/07/07														 */
/* ========================================================================= */

LPCSTR CInfoItemBase::GetKeyName(int nNo)
{
	LPCSTR pRet;

	pRet = NULL;

	if (nNo >= m_nKeyCountBase) {
		goto Exit;
	}

	pRet = m_aszKey[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::GetKeyData										 */
/* 内容：	キーに対応するデータを取得										 */
/* 日付：	2006/07/07														 */
/* ========================================================================= */

void CInfoItemBase::GetKeyData(int nNo, CmyString &strDst)
{
	CmyString strTmp;

	switch (nNo) {
	case 0:			/* 名前 */
		strTmp = m_strName;
		break;
	case 1:			/* 使った時のメッセージ */
		strTmp = m_strMsg;
		break;
	case 2:			/* 説明 */
		strTmp = m_strComment;
		break;
	case 3:			/* 種別 */
		strTmp.Format ("%d", m_byTypeBase);
		break;
	case 4:			/* フラグ */
		strTmp.Format ("%d", m_byFlg);
		break;
	case 5:			/* 効果音 */
		strTmp.Format ("%d", m_bySound);
		break;
	case 6:			/* 性別 */
		strTmp.Format ("%d", m_bySex);
		break;
	case 7:			/* 破損率 */
		strTmp.Format ("%d", m_byEraseRate);
		break;
	case 8:			/* アイテムID */
		strTmp.Format ("%u", m_dwItemID);
		break;
	case 9:			/* アイテム種別ID */
		strTmp.Format ("%u", m_dwItemTypeID);
		break;
	case 10:		/* 価値 */
		strTmp.Format ("%u", m_dwGold);
		break;
	default:
		goto Exit;
	}

Exit:
	strDst = strTmp;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::GetKeyNo											 */
/* 内容：	キー番号を取得													 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

int CInfoItemBase::GetKeyNo(LPCSTR pszKey)
{
	int i, nRet;

	nRet = -1;

	for (i = 0; i < m_nKeyCountBase; i ++) {
		if (strcmp (m_aszKey[i], pszKey) != 0) {
			continue;
		}
		nRet = i;
		break;
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::SetKeyData										 */
/* 内容：	キーに対応するデータを設定										 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void CInfoItemBase::SetKeyData(int nNo, LPCSTR pszSrc)
{
	switch (nNo) {
	case 0:			/* 名前 */
		m_strName = pszSrc;
		break;
	case 1:			/* 使った時のメッセージ */
		m_strMsg = pszSrc;
		break;
	case 2:			/* 説明 */
		m_strComment = pszSrc;
		break;
	case 3:			/* 種別 */
		m_byTypeBase = (BYTE)atoi (pszSrc);
		break;
	case 4:			/* フラグ */
		m_byFlg = (BYTE)atoi (pszSrc);
		break;
	case 5:			/* 効果音 */
		m_bySound = (BYTE)atoi (pszSrc);
		break;
	case 6:			/* 性別 */
		m_bySex = (BYTE)atoi (pszSrc);
		break;
	case 7:			/* 破損率 */
		m_byEraseRate = (BYTE)atoi (pszSrc);
		break;
	case 8:			/* アイテムID */
		m_dwItemID = (DWORD)atoi (pszSrc);
		break;
	case 9:			/* アイテム種別ID */
		m_dwItemTypeID = (DWORD)atoi (pszSrc);
		break;
	case 10:		/* 価値 */
		m_dwGold = (DWORD)atoi (pszSrc);
		break;
	}
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::CopyMemoryRenew									 */
/* 内容：	メモリコピーしてポインタを進める								 */
/* 日付：	2006/06/08														 */
/* ========================================================================= */

void CInfoItemBase::CopyMemoryRenew(PVOID pDst, CONST PVOID pSrc, DWORD dwSize, PBYTE &pPos)
{
	CopyMemory (pDst, pSrc, dwSize);
	pPos += dwSize;
}


/* ========================================================================= */
/* 関数名：	CInfoItemBase::strcpyRenew										 */
/* 内容：	文字列コピーしてポインタを進める								 */
/* 日付：	2006/06/08														 */
/* ========================================================================= */

void CInfoItemBase::strcpyRenew(LPSTR pszDst, LPCSTR pszSrc, PBYTE &pPos)
{
	if ((pszSrc == NULL) || (strlen (pszSrc) <= 0)) {
		pPos ++;
		return;
	}
	strcpy (pszDst, pszSrc);
	pPos += (strlen (pszSrc) + 1);
}

/* Copyright(C)URARA-works 2006 */
