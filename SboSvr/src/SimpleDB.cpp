/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	SimpleDB.cpp												 */
/* 内容：		簡易データベースクラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/25													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "SimpleDB.h"


/* ========================================================================= */
/* 定数の定義																 */
/* ========================================================================= */

#define SIMPLEDB_DATAVERSION		(1)					/* データバージョン */


/* ========================================================================= */
/* 関数名：	CSimpleDB::CSimpleDB											 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

CSimpleDB::CSimpleDB()
{
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::~CSimpleDB											 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

CSimpleDB::~CSimpleDB()
{
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Create												 */
/* 内容：	作成															 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

BOOL CSimpleDB::Create(LPCSTR pszFileName)
{
	BOOL bRet;

	bRet = FALSE;

	/* 作成済み？ */
	if (m_strFileName.IsEmpty () == FALSE) {
		goto Exit;
	}

	m_strFileName = pszFileName;

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Destroy												 */
/* 内容：	破棄															 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

void CSimpleDB::Destroy(void)
{
	m_strFileName.Empty ();
	DeleteAllKeyInfo ();
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Save													 */
/* 内容：	保存															 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

BOOL CSimpleDB::Save(void)
{
	int i, nCount;
	BOOL bRet;
	DWORD dwSize, dwBytes, dwTmp;
	PBYTE pData, pDataTmp, pTmp;
	HANDLE hFile;
	PSIMPLEDB_KEYINFO pKeyInfo;

	bRet	= FALSE;
	pData	= NULL;
	hFile	= INVALID_HANDLE_VALUE;

	if (m_strFileName.IsEmpty ()) {
		goto Exit;
	}

	/* ファイルの作成 */
	hFile = CreateFile (m_strFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	/* データサイズの計算 */
	dwSize = dwBytes = 0;

	/* データバージョンサイズ */
	dwSize += sizeof (DWORD);
	/* キー情報数サイズ */
	dwSize += sizeof (DWORD);

	nCount = m_aKeyInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pKeyInfo = m_aKeyInfo[i];

		dwSize += GetKeyInfoSize (pKeyInfo);
		/* データ情報数サイズ */
		dwSize += sizeof (DWORD);
	}

	/* キャッシュデータの作成 */
	pData		= new BYTE[dwSize];
	pDataTmp	= pData;
	ZeroMemory (pData, dwSize);

	/* データバージョン */
	dwTmp = SIMPLEDB_DATAVERSION;
	CopyMemory (pDataTmp, &dwTmp, sizeof (dwTmp));
	pDataTmp += sizeof (dwTmp);
	/* キー情報数 */
	dwTmp = nCount;
	CopyMemory (pDataTmp, &dwTmp, sizeof (dwTmp));
	pDataTmp += sizeof (dwTmp);

	/* キー情報とデータ情報 */
	for (i = 0; i < nCount; i ++) {
		pKeyInfo = m_aKeyInfo[i];

		pTmp	= GetKeyInfo (pKeyInfo);
		dwTmp	= GetKeyInfoSize (pKeyInfo);
		CopyMemory (pDataTmp, pTmp, dwTmp);
		pDataTmp += dwTmp;
		SAFE_DELETE_ARRAY (pTmp);
	}

	/* キャッシュデータを書き込み */
	WriteFile (hFile, pData, dwSize, &dwBytes, NULL);
	if (dwBytes == 0) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if (pData) {
		delete [] pData;
		pData = NULL;
	}
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Load													 */
/* 内容：	読み込み														 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

BOOL CSimpleDB::Load(void)
{
	int i, nCount;
	BOOL bRet;
	DWORD dwSize, dwBytes, dwTmp;
	PBYTE pData, pDataTmp;
	HANDLE hFile;
	PSIMPLEDB_KEYINFO pKeyInfo;

	bRet	= FALSE;
	pData	= NULL;
	hFile	= INVALID_HANDLE_VALUE;

	if (m_strFileName.IsEmpty ()) {
		goto Exit;
	}

	hFile = CreateFile (m_strFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	/* ファイルサイズ取得 */
	dwSize = GetFileSize (hFile, NULL);
	if (dwSize == -1) {
		goto Exit;
	}

	DeleteAllKeyInfo ();

	pData		= new BYTE[dwSize];
	pDataTmp	= pData;
	/* ファイル内容を全部読み込む */
	ReadFile (hFile, pData, dwSize, &dwBytes, NULL);

	/* データバージョン */
	CopyMemory (&dwTmp, pDataTmp, sizeof (dwTmp));
	pDataTmp += sizeof (dwTmp);
	/* 読み込めないバージョン？ */
	if (dwTmp != SIMPLEDB_DATAVERSION) {
		goto Exit;
	}

	/* キー情報数 */
	CopyMemory (&dwTmp, pDataTmp, sizeof (dwTmp));
	pDataTmp += sizeof (dwTmp);

	/* キー情報とデータ情報 */
	nCount = dwTmp;
	for (i = 0; i < nCount; i ++) {
		pKeyInfo = new SIMPLEDB_KEYINFO;

		pDataTmp = SetKeyInfo (pDataTmp, pKeyInfo);
		m_aKeyInfo.Add (pKeyInfo);
	}

	bRet = TRUE;
Exit:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if (pData) {
		delete [] pData;
		pData = NULL;
	}
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Add													 */
/* 内容：	追加															 */
/* 日付：	2006/05/25														 */
/* ========================================================================= */

void CSimpleDB::Add(
	LPCSTR pszKey,		/* [in] キー名 */
	LPCSTR pszData)		/* [in] データ */
{
	PSIMPLEDB_KEYINFO pKeyInfo;
	PSIMPLEDB_DATAINFO pDataInfo;

	pKeyInfo = GetKeyInfoPtr (pszKey);
	if (pKeyInfo == NULL) {
		pKeyInfo = new SIMPLEDB_KEYINFO;
		pKeyInfo->strKey = pszKey;
		m_aKeyInfo.Add (pKeyInfo);
	}

	pDataInfo = new SIMPLEDB_DATAINFO;
	pDataInfo->strData = pszData;

	pKeyInfo->aDataInfo.Add (pDataInfo);
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetCount												 */
/* 内容：	データ数を取得													 */
/* 日付：	2006/06/02														 */
/* ========================================================================= */

int CSimpleDB::GetCount(LPCSTR pszKey)
{
	int nRet;
	PSIMPLEDB_KEYINFO pKeyInfo;

	nRet = 0;

	/* キー情報を取得 */
	pKeyInfo = GetKeyInfoPtr (pszKey);
	if (pKeyInfo == NULL) {
		goto Exit;
	}

	nRet = pKeyInfo->aDataInfo.GetSize ();
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::Get													 */
/* 内容：	取得															 */
/* 日付：	2006/05/25														 */
/* メモ：	格納先のサイズをチェックしていないので十分なサイズが必要		 */
/* ========================================================================= */

BOOL CSimpleDB::Get(
	LPCSTR pszKey,		/* [in] キー名 */
	int nNo,			/* [in] データインデックス */
	LPSTR pszDst)		/* [ou] データ格納先 */
{
	BOOL bRet;
	PSIMPLEDB_KEYINFO pKeyInfo;
	PSIMPLEDB_DATAINFO pDataInfo;

	bRet = FALSE;
	/* キー情報を取得 */
	pKeyInfo = GetKeyInfoPtr (pszKey);
	if (pKeyInfo == NULL) {
		goto Exit;
	}

	if (nNo >= pKeyInfo->aDataInfo.GetSize ()) {
		goto Exit;
	}
	/* データ情報を取得 */
	pDataInfo = pKeyInfo->aDataInfo[nNo];
	strcpy (pszDst, pDataInfo->strData);

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetKeyInfoPtr										 */
/* 内容：	キー情報を取得													 */
/* 日付：	2006/05/26														 */
/* ========================================================================= */

PSIMPLEDB_KEYINFO CSimpleDB::GetKeyInfoPtr(LPCSTR pszKey)
{
	int i, nCount;
	PSIMPLEDB_KEYINFO pRet, pKeyInfo;

	pRet	= NULL;
	nCount	= m_aKeyInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pKeyInfo = m_aKeyInfo[i];

		if (pKeyInfo->strKey == pszKey) {
			pRet = pKeyInfo;
			break;
		}
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetDataInfoSize										 */
/* 内容：	データ情報サイズを取得											 */
/* 日付：	2006/05/30														 */
/* ========================================================================= */

DWORD CSimpleDB::GetDataInfoSize(PSIMPLEDB_DATAINFO pDataInfo)
{
	DWORD dwRet, dwTmp;

	dwRet = dwTmp = 0;

	if (pDataInfo == NULL) {
		goto Exit;
	}

	dwTmp += (pDataInfo->strData.GetLength () + 1);

	dwRet = dwTmp;
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetDataInfo											 */
/* 内容：	データ情報を取得												 */
/* 日付：	2006/05/30														 */
/* メモ：	開放は取得側でする必要あり										 */
/* ========================================================================= */

PBYTE CSimpleDB::GetDataInfo(PSIMPLEDB_DATAINFO pDataInfo)
{
	PBYTE pRet, pTmp;
	DWORD dwSize;

	pRet = NULL;

	if (pDataInfo == NULL) {
		goto Exit;
	}

	dwSize	= GetDataInfoSize (pDataInfo);
	pTmp	= new BYTE[dwSize];
	ZeroMemory (pTmp, dwSize);

	strcpy ((LPSTR)pTmp, pDataInfo->strData);

	pRet = pTmp;
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::SetDataInfo											 */
/* 内容：	データ情報を取得												 */
/* 日付：	2006/05/31														 */
/* 戻り値：	取得後のアドレス												 */
/* ========================================================================= */

PBYTE CSimpleDB::SetDataInfo(
	PBYTE pSrc,						/* [in] 取得元のアドレス */
	PSIMPLEDB_DATAINFO pDst)		/* [ou] データ情報の格納先 */
{
	PBYTE pRet, pTmp;

	pRet = NULL;
	pTmp = pSrc;

	pDst->strData = (LPCSTR)pTmp;
	pTmp += (pDst->strData.GetLength () + 1);

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::DeleteDataInfo										 */
/* 内容：	データ情報を削除												 */
/* 日付：	2006/05/26														 */
/* ========================================================================= */

void CSimpleDB::DeleteDataInfo(PSIMPLEDB_DATAINFO pDataInfo)
{
	SAFE_DELETE (pDataInfo);
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::DeleteAllDataInfo									 */
/* 内容：	データ情報を全て削除											 */
/* 日付：	2006/05/26														 */
/* ========================================================================= */

void CSimpleDB::DeleteAllDataInfo(PARRAYSIMPLEDBDATAINFO paDataInfo)
{
	int i, nCount;
	PSIMPLEDB_DATAINFO pDataInfo;

	if (paDataInfo == NULL) {
		return;
	}

	nCount = paDataInfo->GetSize ();
	for (i = 0; i < nCount; i ++) {
		pDataInfo = paDataInfo->GetAt (i);
		DeleteDataInfo (pDataInfo);
	}
	paDataInfo->RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetKeyInfoSize										 */
/* 内容：	キー情報サイズを取得											 */
/* 日付：	2006/05/30														 */
/* ========================================================================= */

DWORD CSimpleDB::GetKeyInfoSize(PSIMPLEDB_KEYINFO pKeyInfo)
{
	int i, nCount;
	DWORD dwRet, dwTmp;
	PSIMPLEDB_DATAINFO pDataInfo;

	dwRet = dwTmp = 0;

	if (pKeyInfo == NULL) {
		goto Exit;
	}

	dwTmp += (pKeyInfo->strKey.GetLength () + 1);	/* キー名 */
	dwTmp += sizeof (DWORD);						/* データ情報数 */

	nCount = pKeyInfo->aDataInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pDataInfo = pKeyInfo->aDataInfo[i];

		dwTmp += GetDataInfoSize (pDataInfo);
	}

	dwRet = dwTmp;
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::GetKeyInfo											 */
/* 内容：	キー情報を取得													 */
/* 日付：	2006/05/30														 */
/* メモ：	開放は取得側でする必要あり										 */
/* ========================================================================= */

PBYTE CSimpleDB::GetKeyInfo(PSIMPLEDB_KEYINFO pKeyInfo)
{
	int i, nCount;
	DWORD dwSize, dwSizeTmp, dwTmp;
	PBYTE pRet, pData, pDataTmp, pDataInfoTmp;
	PSIMPLEDB_DATAINFO pDataInfo;

	pRet = NULL;

	if (pKeyInfo == NULL) {
		goto Exit;
	}

	dwSize = 0;

	dwSize += (pKeyInfo->strKey.GetLength () + 1);

	/* データサイズを算出 */
	nCount = pKeyInfo->aDataInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pDataInfo = pKeyInfo->aDataInfo[i];

		dwSize += sizeof (DWORD);
		dwSize += GetDataInfoSize (pDataInfo);
	}

	pData		= new BYTE[dwSize];
	pDataTmp	= pData;
	ZeroMemory (pData, dwSize);

	strcpy ((LPSTR)pDataTmp, pKeyInfo->strKey);
	pDataTmp += (pKeyInfo->strKey.GetLength () + 1);

	/* データ情報数 */
	dwTmp = pKeyInfo->aDataInfo.GetSize ();
	CopyMemory (pDataTmp, &dwTmp, sizeof (dwTmp));
	pDataTmp += sizeof (dwTmp);

	for (i = 0; i < nCount; i ++) {
		pDataInfo = pKeyInfo->aDataInfo[i];

		dwSizeTmp		= GetDataInfoSize (pDataInfo);
		pDataInfoTmp	= GetDataInfo (pDataInfo);

		/* データ情報 */
		CopyMemory (pDataTmp, pDataInfoTmp, dwSizeTmp);
		pDataTmp += dwSizeTmp;

		SAFE_DELETE_ARRAY (pDataInfoTmp);
	}

	pRet = pData;
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::SetKeyInfo											 */
/* 内容：	キー情報を取得													 */
/* 日付：	2006/05/31														 */
/* 戻り値：	取得後のアドレス												 */
/* ========================================================================= */

PBYTE CSimpleDB::SetKeyInfo(
	PBYTE pSrc,					/* [in] 取得元のアドレス */
	PSIMPLEDB_KEYINFO pDst)		/* [ou] キー情報の格納先 */
{
	int i, nCount;
	PBYTE pRet, pTmp;
	DWORD dwTmp;
	PSIMPLEDB_DATAINFO pDataInfo;

	pRet = NULL;
	pTmp = pSrc;

	/* キー名 */
	pDst->strKey = (LPCSTR)pTmp;
	pTmp += (pDst->strKey.GetLength () + 1);

	/* データ情報数 */
	CopyMemory (&dwTmp, pTmp, sizeof (dwTmp));
	pTmp += sizeof (dwTmp);

	/* データ情報 */
	nCount = dwTmp;
	for (i = 0; i < nCount; i ++) {
		pDataInfo = new SIMPLEDB_DATAINFO;

		pTmp = SetDataInfo (pTmp, pDataInfo);
		pDst->aDataInfo.Add (pDataInfo);
	}

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::DeleteKeyInfo										 */
/* 内容：	キー情報を削除													 */
/* 日付：	2006/05/26														 */
/* ========================================================================= */

void CSimpleDB::DeleteKeyInfo(PSIMPLEDB_KEYINFO pKeyInfo)
{
	if (pKeyInfo == NULL) {
		return;
	}

	DeleteAllDataInfo (&pKeyInfo->aDataInfo);

	SAFE_DELETE (pKeyInfo);
}


/* ========================================================================= */
/* 関数名：	CSimpleDB::DeleteAllKeyInfo										 */
/* 内容：	キー情報を全て削除												 */
/* 日付：	2006/05/26														 */
/* ========================================================================= */

void CSimpleDB::DeleteAllKeyInfo(void)
{
	int i, nCount;
	PSIMPLEDB_KEYINFO pKeyInfo;

	nCount = m_aKeyInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pKeyInfo = m_aKeyInfo[i];
		DeleteKeyInfo (pKeyInfo);
	}
	m_aKeyInfo.RemoveAll ();
}

/* Copyright(C)URARA-works 2006 */
