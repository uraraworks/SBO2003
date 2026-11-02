/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadEfc.cpp										 */
/* 概要：		エフェクト情報読み書き処理(実装ファイル)					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "../../../Common/Info/InfoEfc.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

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

/* エフェクト情報 */
static const char *s_aszName[] = {
	"byType",			/* グラフィック種別 */
	"bySound",			/* 使った時に鳴らす音 */
	"byAnimeCount",		/* アニメーションコマ数 */
	"pAnime_byWait",	/* アニメーション情報:待ち時間(×１０ミリ秒) */
	"pAnime_byGrpID",	/* アニメーション情報:グラフィックID */
	"pAnime_byLevel",	/* アニメーション情報:透明度 */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

BOOL SaveEfcInfo(void);										/* エフェクト情報情報保存 */
BOOL LoadEfcInfo(void);										/* エフェクト情報情報読み込み */

/* ========================================================================= */
/* 関数名：	SaveEfcInfo														 */
/* 内容：	エフェクト情報情報保存											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

BOOL SaveEfcInfo(void)
{
	int i, j, k, nCount, nAnimeCount;
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	BYTE byTmp;
	char szFileName[MAX_PATH + 1];
	DWORD dwCount, dwColumnCount, dwDataCount, dwHeaderSize,
			dwDataSize, dwOffset, dwBytes;
	PEFCINFO pEfcInfo;
	PEFCANIMEINFO pEfcAnimeInfo;
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
	strcat (szFileName, "\\SboDataEfcInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwDataCount = g_EfcInfo.GetCount ();


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
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

	dwColumnCount = aHeaderInfo.GetSize ();
	dwDataSize = 0;

	/* データサイズを計算 */
	for (i = 0; i < (int)dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;
		for (j = 0; j < (int)dwDataCount; j ++) {
			pEfcInfo	= g_EfcInfo.GetPtr (j);
			nCount		= pEfcInfo->aAnime.GetCount ();
			switch (i) {
			case 0:		dwDataSizeTmp += sizeof (BYTE);					break;	/* グラフィック種別 */
			case 1:		dwDataSizeTmp += sizeof (BYTE);					break;	/* 使った時に鳴らす音 */
			case 2:		dwDataSizeTmp += sizeof (BYTE);					break;	/* アニメーションコマ数 */
			case 3:		dwDataSizeTmp += (sizeof (BYTE) * nCount);		break;	/* アニメーション情報:待ち時間(×１０ミリ秒) */
			case 4:		dwDataSizeTmp += (sizeof (WORD) * nCount);		break;	/* アニメーション情報:グラフィックID */
			case 5:		dwDataSizeTmp += (sizeof (BYTE) * nCount);		break;	/* アニメーション情報:透明度 */
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
	for (i = 0; i < (int)dwCount; i ++) {
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
	for (i = 0; i < (int)dwColumnCount; i ++) {
		for (j = 0; j < (int)dwDataCount; j ++) {
			pEfcInfo	= g_EfcInfo.GetPtr (j);
			nAnimeCount	= pEfcInfo->aAnime.GetCount ();
			switch (i) {
			case 0:		/* グラフィック種別 */
				byTmp = nAnimeCount;
				CopyMemory (&pData[dwOffset], &pEfcInfo->byType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 1:		/* 使った時に鳴らす音 */
				CopyMemory (&pData[dwOffset], &pEfcInfo->bySound, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 2:		/* アニメーションコマ数 */
				byTmp = nAnimeCount;
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 3:		/* アニメーション情報:待ち時間(×１０ミリ秒) */
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pData[dwOffset], &pEfcAnimeInfo->byWait, sizeof (BYTE));
					dwOffset += sizeof (BYTE);
				}
				break;
			case 4:		/* アニメーション情報:グラフィックID */
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pData[dwOffset], &pEfcAnimeInfo->wGrpID, sizeof (WORD));
					dwOffset += sizeof (WORD);
				}
				break;

			case 5:		/* アニメーション情報:透明度 */
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pData[dwOffset], &pEfcAnimeInfo->byLevel, sizeof (BYTE));
					dwOffset += sizeof (BYTE);
				}
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
/* 関数名：	LoadEfcInfo														 */
/* 内容：	エフェクト情報読み込み											 */
/* 日付：	2006/01/02														 */
/* ========================================================================= */

BOOL LoadEfcInfo(void)
{
	int k, nAnimeCount;
	char szFileName[MAX_PATH + 1];
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	DWORD i, j, dwSize, dwBytes, dwOffset, dwDataCount, dwColumnCount;
	PEFCINFO pEfcInfo;
	PEFCANIMEINFO pEfcAnimeInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	for (k = strlen (szFileName) - 1; k > 0; k --) {
		if (szFileName[k] == '\\') {
			szFileName[k + 1] = 0;
			break;
		}
	}
	strcat (szFileName, "SBODATA\\SboDataEfcInfo.dat");


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

	/* データの準備 */
	for (i = 0; i < dwDataCount; i ++) {
		g_EfcInfo.AddNew ();
	}

	/***************************
		データ部分の読み込み
	***************************/

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			pEfcInfo = g_EfcInfo.GetPtr (j);
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {						/* グラフィック種別 */
				CopyMemory (&pEfcInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {					/* 使った時に鳴らす音 */
				CopyMemory (&pEfcInfo->bySound, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {					/* アニメーションコマ数 */
				nAnimeCount = 0;
				CopyMemory (&nAnimeCount, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

				if (nAnimeCount > 0) {
					for (k = 0; k < nAnimeCount; k ++) {
						pEfcInfo->aAnime.AddNew ();
					}
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {					/* アニメーション情報:待ち時間(×１０ミリ秒) */
				nAnimeCount = pEfcInfo->aAnime.GetCount ();
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pEfcAnimeInfo->byWait, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {					/* アニメーション情報:グラフィックID */
				nAnimeCount = pEfcInfo->aAnime.GetCount ();
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pEfcAnimeInfo->wGrpID, &pData[dwOffsetTmp], sizeof (WORD));
					dwOffsetTmp += sizeof (WORD);
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {					/* アニメーション情報:透明度 */
				nAnimeCount = pEfcInfo->aAnime.GetCount ();
				for (k = 0; k < nAnimeCount; k ++) {
					pEfcAnimeInfo = pEfcInfo->aAnime.GetPtr (k);
					CopyMemory (&pEfcAnimeInfo->byLevel, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
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
