/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoEfc.cpp													 */
/* 内容：		エフェクト情報クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/02													 */
/* ========================================================================= */

#include "stdafx.h"
#include "InfoEfc.h"

/* ========================================================================= */
/* 関数名：	CInfoEfc::CInfoEfc												 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

CInfoEfc::CInfoEfc()
{
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::~CInfoEfc												 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

CInfoEfc::~CInfoEfc()
{
	DeleteAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetCount												 */
/* 内容：	データ数を取得													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

int CInfoEfc::GetCount(void)
{
	return m_aEfcInfo.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetCountAnime											 */
/* 内容：	データ数を取得													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

int CInfoEfc::GetCountAnime(int nNo)
{
	int nRet;
	PEFCINFO pInfo;

	pInfo = GetPtr (nNo);
	if (pInfo == NULL) {
		goto Exit;
	}

	nRet = pInfo->aAnime.GetCount ();
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetNew												 */
/* 内容：	新規データを取得												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCINFO CInfoEfc::GetNew(void)
{
	PEFCINFO pEfcInfo;

	pEfcInfo = new EFCINFO;
	pEfcInfo->byType	= 0;
	pEfcInfo->bySound	= 0;

	return pEfcInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::AddNew												 */
/* 内容：	空のデータを追加												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::AddNew(void)
{
	PEFCINFO pEfcInfo;

	pEfcInfo = GetNew ();
	m_aEfcInfo.Add (pEfcInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::AddNewAnime											 */
/* 内容：	空のアニメーション情報を追加									 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::AddNewAnime(int nNo)
{
	PEFCINFO pEfcInfo;

	pEfcInfo = GetPtr (nNo);
	if (pEfcInfo == NULL) {
		return;
	}

	pEfcInfo->aAnime.AddNew ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::Add													 */
/* 内容：	データを追加													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::Add(PEFCINFO pSrc)
{
	m_aEfcInfo.Add (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::Delete												 */
/* 内容：	削除															 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::Delete(PEFCINFO pSrc)
{
	SAFE_DELETE (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::DeleteAll												 */
/* 内容：	全て削除														 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::DeleteAll(void)
{
	int i, nCount;
	PEFCINFO pEfcInfo;

	nCount = m_aEfcInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pEfcInfo = m_aEfcInfo[i];
		Delete (pEfcInfo);
	}
	m_aEfcInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::DeleteAllAnime										 */
/* 内容：	削除															 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::DeleteAllAnime(int nNo)
{
	PEFCINFO pEfcInfo;

	pEfcInfo = GetPtr (nNo);
	if (pEfcInfo == NULL) {
		return;
	}

	pEfcInfo->aAnime.DeleteAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetPtr												 */
/* 内容：	エフェクト情報を取得											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCINFO CInfoEfc::GetPtr(int nNo)
{
	PEFCINFO pRet;

	pRet = NULL;

	if (nNo >= m_aEfcInfo.GetSize ()) {
		goto Exit;
	}

	pRet = m_aEfcInfo[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetPtrAnime											 */
/* 内容：	アニメーション情報を取得										 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCANIMEINFO CInfoEfc::GetPtrAnime(int nNo, int nNoAnime)
{
	PEFCINFO pInfo;
	PEFCANIMEINFO pRet;

	pRet  = NULL;
	pInfo = GetPtr (nNo);

	if (pInfo == NULL) {
		goto Exit;
	}

	pRet = pInfo->aAnime.GetPtr (nNoAnime);
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetSendDataSize										 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

DWORD CInfoEfc::GetSendDataSize(int nNo)
{
	DWORD dwRet;
	PEFCINFO pEfcInfo;

	dwRet = 0;

	if (nNo >= m_aEfcInfo.GetSize ()) {
		goto Exit;
	}

	pEfcInfo = m_aEfcInfo[nNo];
	dwRet += sizeof (pEfcInfo->byType);
	dwRet += sizeof (pEfcInfo->bySound);
	dwRet += pEfcInfo->aAnime.GetAllSendDataSize ();

Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetAllSendDataSize									 */
/* 内容：	全送信用データサイズを取得										 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

DWORD CInfoEfc::GetAllSendDataSize(void)
{
	int i, nCount;
	DWORD dwRet;

	dwRet	= 0;
	nCount	= m_aEfcInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		dwRet += GetSendDataSize (i);
	}
	/* 終端用 */
	dwRet += 2;

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetSendData											 */
/* 内容：	送信用データを取得												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PBYTE CInfoEfc::GetSendData(int nNo)
{
	PBYTE pRet, pData, pDataTmp;
	DWORD dwSize, dwSizeTmp;
	PEFCINFO pEfcInfo;

	pRet	 = NULL;
	pEfcInfo = GetPtr (nNo);
	if (pEfcInfo == NULL) {
		goto Exit;
	}

	dwSize	= GetSendDataSize (nNo);
	pRet	= new BYTE[dwSize];

	pData = pRet;
	CopyMemory (pData, &pEfcInfo->byType, sizeof (pEfcInfo->byType));
	pData += sizeof (pEfcInfo->byType);
	CopyMemory (pData, &pEfcInfo->bySound, sizeof (pEfcInfo->bySound));
	pData += sizeof (pEfcInfo->bySound);

	dwSizeTmp	= pEfcInfo->aAnime.GetAllSendDataSize ();
	pDataTmp	= pEfcInfo->aAnime.GetAllSendData ();
	CopyMemory (pData, pDataTmp, dwSizeTmp);

	SAFE_DELETE_ARRAY (pDataTmp);
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::GetAllSendData										 */
/* 内容：	全送信用データを取得											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PBYTE CInfoEfc::GetAllSendData(void)
{
	int i, nCount;
	PBYTE pRet, pData, pDataTmp;
	WORD wTmp;
	DWORD dwSize, dwSizeTmp;

	dwSize	= GetAllSendDataSize ();
	pRet	= new BYTE[dwSize];
	pData	= pRet;

	nCount = GetCount ();
	for (i = 0; i < nCount; i ++) {
		pDataTmp	= GetSendData (i);
		dwSizeTmp	= GetSendDataSize (i);
		CopyMemory (pData, pDataTmp, dwSizeTmp);
		pData += dwSizeTmp;
		SAFE_DELETE_ARRAY (pDataTmp);
	}
	wTmp = 0xFFFF;
	CopyMemory (pData, &wTmp, sizeof (wTmp));

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::SetSendData											 */
/* 内容：	送信用データから設定											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCINFO CInfoEfc::SetSendData(PBYTE pSrc, DWORD &dwSize)
{
	PBYTE pData;
	WORD wTmp;
	DWORD dwSizeTmp;
	PEFCINFO pEfcInfo;
	PEFCANIMEINFO pEfcAnimeInfo;

	pEfcInfo = GetNew ();

	pData = pSrc;
	CopyMemory (&pEfcInfo->byType, pData, sizeof (pEfcInfo->byType));
	pData += sizeof (pEfcInfo->byType);
	CopyMemory (&pEfcInfo->bySound, pData, sizeof (pEfcInfo->bySound));
	pData += sizeof (pEfcInfo->bySound);

	while (1) {
		CopyMemory (&wTmp, pData, sizeof (wTmp));
		if (wTmp == 0xFFFF) {
			pData += 2;
			break;
		}

		dwSizeTmp		= 0;
		pEfcAnimeInfo	= pEfcInfo->aAnime.SetSendData (pData, dwSizeTmp);
		pEfcInfo->aAnime.Add (pEfcAnimeInfo);
		pData += dwSizeTmp;
	}

	dwSize = pData - pSrc;
	return pEfcInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::SetAllSendData										 */
/* 内容：	送信用データから設定											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::SetAllSendData(PBYTE pSrc)
{
	PBYTE pData;
	WORD wTmp;
	DWORD dwSizeTmp;
	PEFCINFO pEfcInfo;

	DeleteAll ();
	pData = pSrc;

	while (1) {
		CopyMemory (&wTmp, pData, sizeof (wTmp));
		if (wTmp == 0xFFFF) {
			pData += 2;
			break;
		}

		dwSizeTmp	= 0;
		pEfcInfo	= SetSendData (pData, dwSizeTmp);
		pData += dwSizeTmp;
		Add (pEfcInfo);
	}
}


/* ========================================================================= */
/* 関数名：	CInfoEfc::Renew													 */
/* 内容：	更新															 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfc::Renew(int nNo, PEFCINFO pSrc)
{
	PEFCINFO pEfcInfo;

	pEfcInfo = GetPtr (nNo);
	if (pEfcInfo == NULL) {
		return;
	}

	Delete (pEfcInfo);
	m_aEfcInfo[nNo] = pSrc;
}

/* Copyright(C)URARA-works 2006 */
