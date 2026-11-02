/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoJob.cpp													 */
/* 内容：		職業情報クラス 実装ファイル									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/02/01													 */
/* ========================================================================= */

#include "stdafx.h"
#include "InfoJob.h"

/* ========================================================================= */
/* 関数名：	CInfoJob::CInfoJob												 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

CInfoJob::CInfoJob()
{
}


/* ========================================================================= */
/* 関数名：	CInfoJob::~CInfoJob												 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

CInfoJob::~CInfoJob()
{
	int i, nCount;
	PJOBINFO pJobInfo;

	nCount = m_aJobInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pJobInfo = m_aJobInfo[i];
		SAFE_DELETE (pJobInfo);
	}
	m_aJobInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetCount												 */
/* 内容：	データ数を取得													 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

int CInfoJob::GetCount(void)
{
	return m_aJobInfo.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetNo													 */
/* 内容：	配列番号を取得													 */
/* 日付：	2006/02/07														 */
/* ========================================================================= */

int CInfoJob::GetNo(DWORD dwJobID)
{
	int i, nCount, nRet;
	PJOBINFO pJobInfo;

	nRet 	= -1;
	nCount	= GetCount ();

	for (i = 0; i < nCount; i ++) {
		pJobInfo = m_aJobInfo[i];
		if (pJobInfo->dwJobID == dwJobID) {
			nRet = i;
			break;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetNew												 */
/* 内容：	新規データを取得												 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PJOBINFO CInfoJob::GetNew(void)
{
	PJOBINFO pJobInfo;

	pJobInfo = new JOBINFO;
	pJobInfo->bySkillType	= 0;
	pJobInfo->byMagicType	= 0;
	pJobInfo->byEquType		= 0;
	pJobInfo->dwJobID		= 0;
	pJobInfo->dwFlg			= 0;
	pJobInfo->pszName		= NULL;

	return pJobInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetNewLvUpInfo										 */
/* 内容：	新規データを取得(レベルアップ情報)								 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PLVUPINFO CInfoJob::GetNewLvUpInfo(void)
{
	PLVUPINFO pLvUpInfo;

	pLvUpInfo = new LVUPINFO;
	pLvUpInfo->byPW		= 0;
	pLvUpInfo->byDEX	= 0;
	pLvUpInfo->byST		= 0;
	pLvUpInfo->byIT		= 0;
	pLvUpInfo->byLK		= 0;
	pLvUpInfo->wMAXHP	= 0;
	pLvUpInfo->wMAXMANA	= 0;
	pLvUpInfo->dwExp	= 0;

	return pLvUpInfo;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetNewID												 */
/* 内容：	新しい職業IDを取得												 */
/* 日付：	2006/02/07														 */
/* ========================================================================= */

DWORD CInfoJob::GetNewID(void)
{
	int i, nCount;
	DWORD dwRet;
	PJOBINFO pJobInfo;

	dwRet	= 1;
	nCount	= GetCount ();

	for (i = 0; i < nCount; i ++) {
		pJobInfo = GetPtr (i);
		if (dwRet <= pJobInfo->dwJobID) {
			dwRet = pJobInfo->dwJobID + 1;
		}
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::Add													 */
/* 内容：	データを追加													 */
/* 日付：	2006/02/01														 */
/* 戻り値：	職業ID															 */
/* ========================================================================= */

DWORD CInfoJob::Add(PJOBINFO pSrc)
{
	DWORD dwRet;

	dwRet = 0;
	if (pSrc->dwJobID == 0) {
		pSrc->dwJobID = GetNewID ();
	}
	m_aJobInfo.Add (pSrc);

	return pSrc->dwJobID;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::Set													 */
/* 内容：	既存データを更新												 */
/* 日付：	2006/02/08														 */
/* ========================================================================= */

void CInfoJob::Set(PJOBINFO pSrc)
{
	PJOBINFO pDst;

	pDst = GetPtr (pSrc->dwJobID);
	if (pDst == NULL) {
		return;
	}

	Copy (pDst, pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::Delete												 */
/* 内容：	削除															 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

void CInfoJob::Delete(PJOBINFO pSrc)
{
	DeleteAllLvUpInfo (pSrc);
	SAFE_DELETE (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::DeleteAll												 */
/* 内容：	全て削除														 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

void CInfoJob::DeleteAll(void)
{
	int i, nCount;
	PJOBINFO pJobInfo;

	nCount = m_aJobInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pJobInfo = m_aJobInfo[i];
		Delete (pJobInfo);
	}
	m_aJobInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoJob::DeleteLvUpInfo										 */
/* 内容：	削除(レベルアップ情報)											 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

void CInfoJob::DeleteLvUpInfo(PLVUPINFO pSrc)
{
	SAFE_DELETE (pSrc);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::DeleteAllLvUpInfo										 */
/* 内容：	全て削除(レベルアップ情報)										 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

void CInfoJob::DeleteAllLvUpInfo(LPCMYARRAYLVUPINFO pSrc)
{
	int i, nCount;
	PLVUPINFO pLvUpInfo;

	if (pSrc == NULL) {
		return;
	}

	nCount = pSrc->GetSize ();
	for (i = 0; i < nCount; i ++) {
		pLvUpInfo = pSrc->GetAt (i);
		DeleteLvUpInfo (pLvUpInfo);
	}
	pSrc->RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoJob::DeleteAllLvUpInfo										 */
/* 内容：	全て削除(レベルアップ情報)										 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

void CInfoJob::DeleteAllLvUpInfo(PJOBINFO pSrc)
{
	DeleteAllLvUpInfo (&pSrc->aLevelUpInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetPtr												 */
/* 内容：	職業情報を取得													 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PJOBINFO CInfoJob::GetPtr(int nNo)
{
	PJOBINFO pRet;

	pRet = NULL;

	if (nNo >= m_aJobInfo.GetSize ()) {
		goto Exit;
	}

	pRet = m_aJobInfo[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetPtr												 */
/* 内容：	職業情報を取得													 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

PJOBINFO CInfoJob::GetPtr(DWORD dwJobID)
{
	int i, nCount;
	PJOBINFO pInfo, pRet;

	pRet	= NULL;
	nCount	= m_aJobInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pInfo = m_aJobInfo[i];
		if (pInfo->dwJobID == dwJobID) {
			pRet = pInfo;
			break;
		}
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::SetJobName											 */
/* 内容：	職業名を設定													 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

void CInfoJob::SetJobName(PJOBINFO pDst, LPCSTR pszJobName)
{
	if (pDst == NULL) {
		return;
	}

	SAFE_DELETE_ARRAY (pDst->pszName);
	if ((pszJobName == NULL) || (strlen (pszJobName) <= 0)) {
		return;
	}

	pDst->pszName = new char[strlen (pszJobName) + 1];
	strcpy (pDst->pszName, pszJobName);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::Copy													 */
/* 内容：	コピー															 */
/* 日付：	2006/02/06														 */
/* ========================================================================= */

void CInfoJob::Copy(PJOBINFO pDst, PJOBINFO pSrc)
{
	pDst->bySkillType	= pSrc->bySkillType;
	pDst->byMagicType	= pSrc->byMagicType;
	pDst->byEquType		= pSrc->byEquType;
	pDst->dwFlg			= pSrc->dwFlg;

	SetJobName (pDst, pSrc->pszName);
	CopyLvUpInfo (&pDst->aLevelUpInfo, &pSrc->aLevelUpInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoJob::CopyLvUpInfo											 */
/* 内容：	コピー(レベルアップ情報)										 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

void CInfoJob::CopyLvUpInfo(PLVUPINFO pDst, PLVUPINFO pSrc)
{
	if ((pDst == NULL) || (pSrc == NULL)) {
		return;
	}

	pDst->byPW		= pSrc->byPW;
	pDst->byDEX		= pSrc->byDEX;
	pDst->byST		= pSrc->byST;
	pDst->byIT		= pSrc->byIT;
	pDst->byLK		= pSrc->byLK;
	pDst->wMAXHP	= pSrc->wMAXHP;
	pDst->wMAXMANA	= pSrc->wMAXMANA;
	pDst->dwExp		= pSrc->dwExp;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::CopyLvUpInfo											 */
/* 内容：	コピー(レベルアップ情報)										 */
/* 日付：	2006/02/02														 */
/* ========================================================================= */

void CInfoJob::CopyLvUpInfo(LPCMYARRAYLVUPINFO pDst, LPCMYARRAYLVUPINFO pSrc)
{
	int i, nCount;
	PLVUPINFO pInfoSrc, pInfoNew;

	if ((pDst == NULL) || (pSrc == NULL)) {
		return;
	}

	DeleteAllLvUpInfo (pDst);

	nCount = pSrc->GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfoSrc	= pSrc->GetAt (i);
		pInfoNew	= GetNewLvUpInfo ();
		CopyLvUpInfo (pInfoNew, pInfoSrc);
		pDst->Add (pInfoNew);
	}
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetSendDataSize										 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

DWORD CInfoJob::GetSendDataSize(int nNo)
{
	int i, nCount;
	DWORD dwRet;
	PJOBINFO pJobInfo;

	dwRet = 0;

	if (nNo >= m_aJobInfo.GetSize ()) {
		goto Exit;
	}

	pJobInfo = m_aJobInfo[nNo];
	dwRet += sizeof (pJobInfo->bySkillType);
	dwRet += sizeof (pJobInfo->byMagicType);
	dwRet += sizeof (pJobInfo->byEquType);
	dwRet += sizeof (pJobInfo->dwJobID);
	dwRet += sizeof (pJobInfo->dwFlg);
	dwRet += (strlen (pJobInfo->pszName) + 1);

	dwRet += 1;	/* データ数用 */
	nCount = pJobInfo->aLevelUpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		dwRet += sizeof (LVUPINFO);
	}

Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetSendDataSize										 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/02/07														 */
/* ========================================================================= */

DWORD CInfoJob::GetSendDataSize(DWORD dwJobID)
{
	int nNo;
	DWORD dwRet;

	dwRet	= 0;
	nNo		= GetNo (dwJobID);

	if (nNo == -1) {
		goto Exit;
	}

	dwRet = GetSendDataSize (nNo);
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetAllSendDataSize									 */
/* 内容：	全送信用データサイズを取得										 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

DWORD CInfoJob::GetAllSendDataSize(void)
{
	int i, nCount;
	DWORD dwRet;

	dwRet	= 0;
	nCount	= m_aJobInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		dwRet += GetSendDataSize (i);
	}
	/* 終端用 */
	dwRet ++;

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetSendData											 */
/* 内容：	送信用データを取得												 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PBYTE CInfoJob::GetSendData(int nNo)
{
	int i, nCount;
	BYTE byTmp;
	PBYTE pRet, pData;
	DWORD dwSize;
	PJOBINFO pJobInfo;
	PLVUPINFO pLvUpInfo;

	pRet	 = NULL;
	pJobInfo = GetPtr (nNo);
	if (pJobInfo == NULL) {
		goto Exit;
	}

	dwSize	= GetSendDataSize (nNo);
	pRet	= new BYTE[dwSize];

	pData = pRet;
	CopyMemory (pData, &pJobInfo->dwJobID, sizeof (pJobInfo->dwJobID));
	pData += sizeof (pJobInfo->dwJobID);
	CopyMemory (pData, &pJobInfo->bySkillType, sizeof (pJobInfo->bySkillType));
	pData += sizeof (pJobInfo->bySkillType);
	CopyMemory (pData, &pJobInfo->byMagicType, sizeof (pJobInfo->byMagicType));
	pData += sizeof (pJobInfo->byMagicType);
	CopyMemory (pData, &pJobInfo->byEquType, sizeof (pJobInfo->byEquType));
	pData += sizeof (pJobInfo->byEquType);
	CopyMemory (pData, &pJobInfo->dwFlg, sizeof (pJobInfo->dwFlg));
	pData += sizeof (pJobInfo->dwFlg);
	strcpy ((LPSTR)pData, pJobInfo->pszName);
	pData += (strlen (pJobInfo->pszName) + 1);

	byTmp = (BYTE)pJobInfo->aLevelUpInfo.GetSize ();
	CopyMemory (pData, &byTmp, sizeof (byTmp));
	pData += sizeof (byTmp);

	nCount = pJobInfo->aLevelUpInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pLvUpInfo = pJobInfo->aLevelUpInfo[i];

		CopyMemory (pData, &pLvUpInfo->byPW, sizeof (pLvUpInfo->byPW));
		pData += sizeof (pLvUpInfo->byPW);
		CopyMemory (pData, &pLvUpInfo->byDEX, sizeof (pLvUpInfo->byDEX));
		pData += sizeof (pLvUpInfo->byDEX);
		CopyMemory (pData, &pLvUpInfo->byST, sizeof (pLvUpInfo->byST));
		pData += sizeof (pLvUpInfo->byST);
		CopyMemory (pData, &pLvUpInfo->byIT, sizeof (pLvUpInfo->byIT));
		pData += sizeof (pLvUpInfo->byIT);
		CopyMemory (pData, &pLvUpInfo->byLK, sizeof (pLvUpInfo->byLK));
		pData += sizeof (pLvUpInfo->byLK);
		CopyMemory (pData, &pLvUpInfo->wMAXHP, sizeof (pLvUpInfo->wMAXHP));
		pData += sizeof (pLvUpInfo->wMAXHP);
		CopyMemory (pData, &pLvUpInfo->wMAXMANA, sizeof (pLvUpInfo->wMAXMANA));
		pData += sizeof (pLvUpInfo->wMAXMANA);
		CopyMemory (pData, &pLvUpInfo->dwExp, sizeof (pLvUpInfo->dwExp));
		pData += sizeof (pLvUpInfo->dwExp);
	}

Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetSendData											 */
/* 内容：	送信用データを取得												 */
/* 日付：	2006/02/07														 */
/* ========================================================================= */

PBYTE CInfoJob::GetSendData(DWORD dwJobID)
{
	int nNo;
	PBYTE pRet;

	pRet	= NULL;
	nNo		= GetNo (dwJobID);

	if (nNo == -1) {
		goto Exit;
	}

	pRet = GetSendData (nNo);
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::GetAllSendData										 */
/* 内容：	全送信用データを取得											 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PBYTE CInfoJob::GetAllSendData(void)
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
	wTmp = 0;
	CopyMemory (pData, &wTmp, sizeof (wTmp));

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoJob::SetSendData											 */
/* 内容：	送信用データから設定											 */
/* 日付：	2006/02/01														 */
/* ========================================================================= */

PJOBINFO CInfoJob::SetSendData(PBYTE pSrc, DWORD &dwSize)
{
	int i, nCount;
	BYTE byTmp;
	PBYTE pData;
	PJOBINFO pJobInfo;
	PLVUPINFO pLvUpInfo;

	pJobInfo = GetNew ();

	pData = pSrc;
	CopyMemory (&pJobInfo->dwJobID, pData, sizeof (pJobInfo->dwJobID));
	pData += sizeof (pJobInfo->dwJobID);
	CopyMemory (&pJobInfo->bySkillType, pData, sizeof (pJobInfo->bySkillType));
	pData += sizeof (pJobInfo->bySkillType);
	CopyMemory (&pJobInfo->byMagicType, pData, sizeof (pJobInfo->byMagicType));
	pData += sizeof (pJobInfo->byMagicType);
	CopyMemory (&pJobInfo->byEquType, pData, sizeof (pJobInfo->byEquType));
	pData += sizeof (pJobInfo->byEquType);
	CopyMemory (&pJobInfo->dwFlg, pData, sizeof (pJobInfo->dwFlg));
	pData += sizeof (pJobInfo->dwFlg);
	SetJobName (pJobInfo, (LPCSTR)pData);
	pData += (strlen (pJobInfo->pszName) + 1);

	CopyMemory (&byTmp, pData, sizeof (byTmp));
	pData += sizeof (byTmp);

	nCount = byTmp;
	for (i = 0; i < nCount; i ++) {
		pLvUpInfo = GetNewLvUpInfo ();

		CopyMemory (&pLvUpInfo->byPW, pData, sizeof (pLvUpInfo->byPW));
		pData += sizeof (pLvUpInfo->byPW);
		CopyMemory (&pLvUpInfo->byDEX, pData, sizeof (pLvUpInfo->byDEX));
		pData += sizeof (pLvUpInfo->byDEX);
		CopyMemory (&pLvUpInfo->byST, pData, sizeof (pLvUpInfo->byST));
		pData += sizeof (pLvUpInfo->byST);
		CopyMemory (&pLvUpInfo->byIT, pData, sizeof (pLvUpInfo->byIT));
		pData += sizeof (pLvUpInfo->byIT);
		CopyMemory (&pLvUpInfo->byLK, pData, sizeof (pLvUpInfo->byLK));
		pData += sizeof (pLvUpInfo->byLK);
		CopyMemory (&pLvUpInfo->wMAXHP, pData, sizeof (pLvUpInfo->wMAXHP));
		pData += sizeof (pLvUpInfo->wMAXHP);
		CopyMemory (&pLvUpInfo->wMAXMANA, pData, sizeof (pLvUpInfo->wMAXMANA));
		pData += sizeof (pLvUpInfo->wMAXMANA);
		CopyMemory (&pLvUpInfo->dwExp, pData, sizeof (pLvUpInfo->dwExp));
		pData += sizeof (pLvUpInfo->dwExp);

		pJobInfo->aLevelUpInfo.Add (pLvUpInfo);
	}

	dwSize = pData - pSrc;
	return pJobInfo;
}

/* Copyright(C)URARA-works 2006 */
