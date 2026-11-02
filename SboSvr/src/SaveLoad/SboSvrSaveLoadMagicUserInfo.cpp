/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadMagicUserInfo.cpp								 */
/* 概要：		魔法利用者情報読み書き処理(実装ファイル)					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/15													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define DATACOUNT					(3)						/* 保存データ数 */

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ヘッダ情報 */
typedef struct _HEADERINFO {
	char		szName[64];							/* カラム名 */
	DWORD		dwOffset;							/* オフセット */
	DWORD		dwDataSize;							/* データサイズ */
} HEADERINFO, *LPHEADERINFO;


/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* ラベル名テーブル */
static const char *s_aszName[] = {
	"szName",				/* キャラ名 */
	"byCount",				/* 保存している職業数 */
	"paJobIDTbl",			/* 職業一覧 */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveMagicUserInfo(void);									/* 魔法利用者情報情報保存 */
BOOL LoadMagicUserInfo(void);									/* 魔法利用者情報読み込み */


/* ========================================================================= */
/* 関数名：SaveMagicUserInfo												 */
/* 内容：魔法利用者情報情報保存												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
BOOL SaveMagicUserInfo(void)
{
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	char szFileName[MAX_PATH + 1];
	DWORD i, j, dwCount, dwColumnCount, dwDataCount, dwHeaderSize,
			dwDataSize, dwOffset, dwBytes;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	for (i = strlen (szFileName) - 1; i > 0; i --) {
		if (szFileName[i] == '\\') {
			szFileName[i + 1] = 0;
			break;
		}
	}
	strcat (szFileName, "SBODATA");
	CreateDirectory (szFileName, NULL);
	strcat (szFileName, "\\SboDataMagicUserInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = DATACOUNT;
	dwDataCount = (DWORD)g_aMagicUserInfo.GetSize ();


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;

		ZeroMemory (&Info, sizeof (Info));
		if (s_aszName[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszName[i]);

		dwHeaderSize += (strlen (Info.szName) + 1);
		dwHeaderSize += sizeof (DWORD);

		Info.dwOffset = 0;
		Info.dwDataSize = 0;
		aHeaderInfo.Add (Info);
	}


	/***********************
		データ部分の作成
	***********************/

	dwDataSize = 0;
	/* データサイズを計算 */
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;
		for (j = 0; j < dwDataCount; j ++) {
			PMAGICUSERINFO pInfo;

			pInfo = g_aMagicUserInfo[j];
			switch (i) {
			case 0:		dwDataSizeTmp += (strlen (pInfo->szName) + 1);			break;	/* キャラ名 */
			case 1:		dwDataSizeTmp += sizeof (BYTE);							break;	/* 保存している職業数 */
			case 2:		dwDataSizeTmp += (pInfo->byCount * sizeof (BYTE));		break;	/* 職業一覧 */
			}
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* データ数を書き込み */
	CopyMemory (&pData[dwOffset], &dwDataCount, sizeof (dwDataCount));
	dwOffset += sizeof (dwDataCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfo;
		DWORD dwTmp;

		pInfo = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfo->szName);
		dwOffset += (strlen (pInfo->szName) + 1);

		dwTmp = dwHeaderSize + pInfo->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		for (j = 0; j < dwDataCount; j ++) {
			PMAGICUSERINFO pInfo;

			pInfo = g_aMagicUserInfo[j];
			switch (i) {
			case 0:		/* キャラ名 */
				strcpy ((LPSTR)&pData[dwOffset], pInfo->szName);
				dwOffset += (strlen (pInfo->szName) + 1);
				break;
			case 1:		/* 保存している職業数 */
				CopyMemory (&pData[dwOffset], &pInfo->byCount, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 2:		/* 職業一覧 */
				CopyMemory (&pData[dwOffset], pInfo->paJobIDTbl, pInfo->byCount * sizeof (BYTE));
				dwOffset += (pInfo->byCount * sizeof (BYTE));
				break;
			}
		}
	}


	/***************************
		ファイルへの書き込み
	****************************/

	hFile = CreateFile (szFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	WriteFile (hFile, pData, dwHeaderSize + dwDataSize, &dwBytes, NULL);
	CloseHandle (hFile);

	bRet = TRUE;
Exit:
	SAFE_DELETE_ARRAY (pData);
	return bRet;
}


/* ========================================================================= */
/* 関数名：LoadMagicUserInfo												 */
/* 内容：魔法利用者情報読み込み												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
BOOL LoadMagicUserInfo(void)
{
	int k;
	char szFileName[MAX_PATH + 1];
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	DWORD i, j, dwSize, dwBytes, dwOffset, dwDataCount, dwColumnCount;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	for (k = strlen (szFileName) - 1; k > 0; k --) {
		if (szFileName[k] == '\\') {
			szFileName[k + 1] = 0;
			break;
		}
	}
	strcat (szFileName, "SBODATA\\SboDataMagicUserInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;


	/***********************
		ファイル読み込み
	***********************/

	hFile = CreateFile (szFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	/* ファイルサイズ取得 */
	dwSize = GetFileSize (hFile, NULL);
	if ((int)dwSize == -1) {
		goto Exit;
	}

	pData = new BYTE[dwSize];
	/* ファイル内容を全部読み込む */
	ReadFile (hFile, pData, dwSize, &dwBytes, NULL);


	/*************************
		ヘッダ情報の読み込み
	*************************/

	dwOffset = 0;
	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* データ数を読み込み */
	CopyMemory (&dwDataCount, &pData[dwOffset], sizeof (dwDataCount));
	dwOffset += sizeof (dwDataCount);

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0;  ; j ++) {
			if (s_aszName[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszName[j]) == 0) {
				bTmp = TRUE;
				break;
			}
		}
		if (bTmp == FALSE) {
			continue;
		}

		Info.dwDataSize = 0;
		aHeaderInfo.Add (Info);
	}


	/***************************
		データ部分の読み込み
	***************************/

	for (i = 0; i < dwDataCount; i ++) {
		PMAGICUSERINFO pInfo;

		pInfo = new MAGICUSERINFO;
		ZeroMemory (pInfo, sizeof (MAGICUSERINFO));
		g_aMagicUserInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PMAGICUSERINFO pInfo;

			pInfo = g_aMagicUserInfo[j];
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {					/* キャラ名 */
				strcpy (pInfo->szName, (LPSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen ((LPSTR)&pData[dwOffsetTmp]) + 1);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {				/* 保存している職業数 */
				CopyMemory (&pInfo->byCount, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {				/* 職業一覧 */
				pInfo->paJobIDTbl = new BYTE[pInfo->byCount];
				CopyMemory (pInfo->paJobIDTbl, &pData[dwOffsetTmp], pInfo->byCount * sizeof (BYTE));
				dwOffsetTmp += (pInfo->byCount * sizeof (BYTE));
			}
		}
	}


	bRet = TRUE;
Exit:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle (hFile);
	}
	SAFE_DELETE_ARRAY (pData);
	return bRet;
}

/* Copyright(C)URARA-works 2003-2005 */
