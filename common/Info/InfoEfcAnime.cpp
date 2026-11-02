/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoEfcAnime.cpp											 */
/* 内容：		エフェクトアニメーション情報クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/02													 */
/* ========================================================================= */

#include "stdafx.h"
#include "InfoEfcAnime.h"

/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::CInfoEfcAnime									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

CInfoEfcAnime::CInfoEfcAnime()
{
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::~CInfoEfcAnime									 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

CInfoEfcAnime::~CInfoEfcAnime()
{
	int i, nCount;
	PEFCANIMEINFO pEfcAnimeInfo;

	nCount = m_aEfcAnimeInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pEfcAnimeInfo = m_aEfcAnimeInfo[i];
		SAFE_DELETE (pEfcAnimeInfo);
	}
	m_aEfcAnimeInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetCount											 */
/* 内容：	データ数を取得													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

int CInfoEfcAnime::GetCount(void)
{
	return m_aEfcAnimeInfo.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetNew											 */
/* 内容：	新規データを取得												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCANIMEINFO CInfoEfcAnime::GetNew(void)
{
	PEFCANIMEINFO pEfcAnimeInfo;

	pEfcAnimeInfo = new EFCANIMEINFO;
	pEfcAnimeInfo->byWait	= 0;
	pEfcAnimeInfo->byLevel	= 0;
	pEfcAnimeInfo->wGrpID	= 0;

	return pEfcAnimeInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::AddNew											 */
/* 内容：	空のデータを追加												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfcAnime::AddNew(void)
{
	PEFCANIMEINFO pEfcAnimeInfo;

	pEfcAnimeInfo = new EFCANIMEINFO;
	pEfcAnimeInfo->byWait	= 0;
	pEfcAnimeInfo->byLevel	= 0;
	pEfcAnimeInfo->wGrpID	= 0;

	m_aEfcAnimeInfo.Add (pEfcAnimeInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::Add												 */
/* 内容：	データを追加													 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfcAnime::Add(PEFCANIMEINFO pSrc)
{
	m_aEfcAnimeInfo.Add (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::Delete											 */
/* 内容：	削除															 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfcAnime::Delete(PEFCANIMEINFO pSrc)
{
	SAFE_DELETE (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::DeleteAll										 */
/* 内容：	全て削除														 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfcAnime::DeleteAll(void)
{
	int i, nCount;
	PEFCANIMEINFO pEfcAnimeInfo;

	nCount = m_aEfcAnimeInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pEfcAnimeInfo = m_aEfcAnimeInfo[i];
		Delete (pEfcAnimeInfo);
	}
	m_aEfcAnimeInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetPtr											 */
/* 内容：	エフェクトアニメーション情報を取得								 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCANIMEINFO CInfoEfcAnime::GetPtr(int nNo)
{
	PEFCANIMEINFO pRet;

	pRet = NULL;

	if (nNo >= m_aEfcAnimeInfo.GetSize ()) {
		goto Exit;
	}

	pRet = m_aEfcAnimeInfo[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetSendDataSize									 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

DWORD CInfoEfcAnime::GetSendDataSize(int nNo)
{
	DWORD dwRet;
	PEFCANIMEINFO pEfcAnimeInfo;

	dwRet = 0;

	if (nNo >= m_aEfcAnimeInfo.GetSize ()) {
		goto Exit;
	}

	pEfcAnimeInfo = m_aEfcAnimeInfo[nNo];
	dwRet += sizeof (pEfcAnimeInfo->byWait);
	dwRet += sizeof (pEfcAnimeInfo->byLevel);
	dwRet += sizeof (pEfcAnimeInfo->wGrpID);

Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetAllSendDataSize								 */
/* 内容：	全送信用データサイズを取得										 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

DWORD CInfoEfcAnime::GetAllSendDataSize(void)
{
	int i, nCount;
	DWORD dwRet;

	dwRet	= 0;
	nCount	= m_aEfcAnimeInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		dwRet += GetSendDataSize (i);
	}
	/* 終端用 */
	dwRet += 2;

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetSendData										 */
/* 内容：	送信用データを取得												 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PBYTE CInfoEfcAnime::GetSendData(int nNo)
{
	PBYTE pRet, pData;
	DWORD dwSize;
	PEFCANIMEINFO pEfcAnimeInfo;

	pRet		  = NULL;
	pEfcAnimeInfo = GetPtr (nNo);
	if (pEfcAnimeInfo == NULL) {
		goto Exit;
	}

	dwSize	= GetSendDataSize (nNo);
	pRet	= new BYTE[dwSize];

	pData = pRet;
	CopyMemory (pData, &pEfcAnimeInfo->byWait, sizeof (pEfcAnimeInfo->byWait));
	pData += sizeof (pEfcAnimeInfo->byWait);
	CopyMemory (pData, &pEfcAnimeInfo->byLevel, sizeof (pEfcAnimeInfo->byLevel));
	pData += sizeof (pEfcAnimeInfo->byLevel);
	CopyMemory (pData, &pEfcAnimeInfo->wGrpID, sizeof (pEfcAnimeInfo->wGrpID));
	pData += sizeof (pEfcAnimeInfo->wGrpID);

Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::GetAllSendData									 */
/* 内容：	全送信用データを取得											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PBYTE CInfoEfcAnime::GetAllSendData(void)
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
/* 関数名：	CInfoEfcAnime::SetSendData										 */
/* 内容：	送信用データから設定											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

PEFCANIMEINFO CInfoEfcAnime::SetSendData(PBYTE pSrc, DWORD &dwSize)
{
	PBYTE pData;
	PEFCANIMEINFO pEfcAnimeInfo;

	pEfcAnimeInfo = GetNew ();

	pData = pSrc;
	CopyMemory (&pEfcAnimeInfo->byWait, pData, sizeof (pEfcAnimeInfo->byWait));
	pData += sizeof (pEfcAnimeInfo->byWait);
	CopyMemory (&pEfcAnimeInfo->byLevel, pData, sizeof (pEfcAnimeInfo->byLevel));
	pData += sizeof (pEfcAnimeInfo->byLevel);
	CopyMemory (&pEfcAnimeInfo->wGrpID, pData, sizeof (pEfcAnimeInfo->wGrpID));
	pData += sizeof (pEfcAnimeInfo->wGrpID);

	dwSize = pData - pSrc;
	return pEfcAnimeInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoEfcAnime::Renew											 */
/* 内容：	更新															 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

void CInfoEfcAnime::Renew(int nNo, PEFCANIMEINFO pSrc)
{
	PEFCANIMEINFO pEfcAnimeInfo;

	pEfcAnimeInfo = GetPtr (nNo);
	if (pEfcAnimeInfo == NULL) {
		return;
	}

	Delete (pEfcAnimeInfo);
	m_aEfcAnimeInfo[nNo] = pSrc;
}

/* Copyright(C)URARA-works 2006 */
