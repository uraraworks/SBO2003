/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadMagicInfo.cpp									 */
/* 概要：		魔法情報読み書き処理(実装ファイル)							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/12													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define DATACOUNTMAGICINFO		(18)					/* 保存データ数 */

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
	"szName",				/* 名前 */
	"byType",				/* 効果種別 */
	"byTypeInfoID",			/* 種別情報ID */
	"byUserInfo",			/* 利用情報 */
	"byTarget",				/* 使用対象 */
	"byTargetType",			/* 発動種別 */
	"byTargetArea",			/* 発動範囲 */
	"byTargetCount",		/* 発動人数 */
	"byTargetEfectID",		/* エフェクト番号（相手） */
	"byUserEfectID",		/* エフェクト番号（利用者） */
	"byTime",				/* 詠唱時間(1で1秒10で10秒) */
	"byAttr",				/* 属性 */
	"wData1",				/* 付加データ */
	"wData2",				/* 付加データ */
	"wData3",				/* 付加データ */
	"wData4",				/* 付加データ */
	"wMana",				/* 消費Mana */
	"szExplain",			/* 説明文（後から追加。古いデータには無いので空になる） */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveMagicInfo(void);										/* 魔法情報情報保存 */
BOOL LoadMagicInfo(void);										/* 魔法情報読み込み */


/* ========================================================================= */
/* 関数名：SaveMagicInfo													 */
/* 内容：魔法情報情報保存													 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
BOOL SaveMagicInfo(void)
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
	strcat (szFileName, "\\SboDataMagicInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = DATACOUNTMAGICINFO;
	dwDataCount = (DWORD)g_aMagicInfo.GetSize ();


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
			PMAGICINFO pInfo;

			pInfo = g_aMagicInfo[j];
			switch (i) {
			case 0:		dwDataSizeTmp += (strlen (pInfo->szName) + 1);		break;		/* 名前 */
			case 1:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 効果種別 */
			case 2:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 種別情報ID */
			case 3:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 利用情報 */
			case 4:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 使用対象 */
			case 5:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 発動種別 */
			case 6:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 発動範囲 */
			case 7:		dwDataSizeTmp += sizeof (BYTE);						break;		/* 発動人数 */
			case 8:		dwDataSizeTmp += sizeof (BYTE);						break;		/* エフェクト番号（相手） */
			case 9:		dwDataSizeTmp += sizeof (BYTE);						break;		/* エフェクト番号（利用者） */
			case 10:	dwDataSizeTmp += sizeof (BYTE);						break;		/* 詠唱時間(1で1秒10で10秒) */
			case 11:	dwDataSizeTmp += sizeof (BYTE);						break;		/* 属性 */
			case 12:	dwDataSizeTmp += sizeof (WORD);						break;		/* 付加データ */
			case 13:	dwDataSizeTmp += sizeof (WORD);						break;		/* 付加データ */
			case 14:	dwDataSizeTmp += sizeof (WORD);						break;		/* 付加データ */
			case 15:	dwDataSizeTmp += sizeof (WORD);						break;		/* 付加データ */
			case 16:	dwDataSizeTmp += sizeof (WORD);						break;		/* 消費Mana */
			case 17:	dwDataSizeTmp += (strlen (pInfo->szExplain) + 1);	break;		/* 説明文 */
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
			PMAGICINFO pInfo;

			pInfo = g_aMagicInfo[j];
			switch (i) {
			case 0:		/* 名前 */
				strcpy ((LPSTR)&pData[dwOffset], pInfo->szName);
				dwOffset += (strlen (pInfo->szName) + 1);
				break;
			case 1:		/* 効果種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 2:		/* 種別情報ID */
				CopyMemory (&pData[dwOffset], &pInfo->byTypeInfoID, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 3:		/* 利用情報 */
				CopyMemory (&pData[dwOffset], &pInfo->byUserInfo, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 4:		/* 使用対象 */
				CopyMemory (&pData[dwOffset], &pInfo->byTarget, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 5:		/* 発動種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byTargetType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 6:		/* 発動範囲 */
				CopyMemory (&pData[dwOffset], &pInfo->byTargetArea, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 7:		/* 発動人数 */
				CopyMemory (&pData[dwOffset], &pInfo->byTargetCount, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 8:		/* エフェクト番号（相手） */
				CopyMemory (&pData[dwOffset], &pInfo->byTargetEfectID, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 9:		/* エフェクト番号（利用者） */
				CopyMemory (&pData[dwOffset], &pInfo->byUserEfectID, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 10:	/* 詠唱時間(1で1秒10で10秒) */
				CopyMemory (&pData[dwOffset], &pInfo->byTime, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 11:	/* 属性 */
				CopyMemory (&pData[dwOffset], &pInfo->byAttr, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 12:	/* 付加データ */
				CopyMemory (&pData[dwOffset], &pInfo->wData1, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 13:	/* 付加データ */
				CopyMemory (&pData[dwOffset], &pInfo->wData2, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 14:	/* 付加データ */
				CopyMemory (&pData[dwOffset], &pInfo->wData3, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 15:	/* 付加データ */
				CopyMemory (&pData[dwOffset], &pInfo->wData4, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 16:	/* 消費Mana */
				CopyMemory (&pData[dwOffset], &pInfo->wMana, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 17:	/* 説明文 */
				strcpy ((LPSTR)&pData[dwOffset], pInfo->szExplain);
				dwOffset += (strlen (pInfo->szExplain) + 1);
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
/* 関数名：LoadMagicInfo													 */
/* 内容：魔法情報読み込み													 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
BOOL LoadMagicInfo(void)
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
	strcat (szFileName, "SBODATA\\SboDataMagicInfo.dat");


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
		PMAGICINFO pInfo;

		pInfo = new MAGICINFO;
		ZeroMemory (pInfo, sizeof (MAGICINFO));
		g_aMagicInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PMAGICINFO pInfo;

			pInfo = g_aMagicInfo[j];
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {					/* 名前 */
				strcpy (pInfo->szName, (LPSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen ((LPSTR)&pData[dwOffsetTmp]) + 1);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {				/* 効果種別 */
				CopyMemory (&pInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {				/* 種別情報ID */
				CopyMemory (&pInfo->byTypeInfoID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {				/* 利用情報 */
				CopyMemory (&pInfo->byUserInfo, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {				/* 使用対象 */
				CopyMemory (&pInfo->byTarget, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {				/* 発動種別 */
				CopyMemory (&pInfo->byTargetType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[6]) == 0) {				/* 発動範囲 */
				CopyMemory (&pInfo->byTargetArea, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[7]) == 0) {				/* 発動人数 */
				CopyMemory (&pInfo->byTargetCount, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[8]) == 0) {				/* エフェクト番号（相手） */
				CopyMemory (&pInfo->byTargetEfectID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[9]) == 0) {				/* エフェクト番号（利用者） */
				CopyMemory (&pInfo->byUserEfectID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[10]) == 0) {			/* 詠唱時間(1で1秒10で10秒) */
				CopyMemory (&pInfo->byTime, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[11]) == 0) {			/* 属性 */
				CopyMemory (&pInfo->byAttr, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[12]) == 0) {			/* 付加データ */
				CopyMemory (&pInfo->wData1, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[13]) == 0) {			/* 付加データ */
				CopyMemory (&pInfo->wData2, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[14]) == 0) {			/* 付加データ */
				CopyMemory (&pInfo->wData3, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[15]) == 0) {			/* 付加データ */
				CopyMemory (&pInfo->wData4, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[16]) == 0) {			/* 消費Mana */
				CopyMemory (&pInfo->wMana, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[17]) == 0) {			/* 説明文 */
				/* 長すぎる時は切り詰める（全角の途中で切れないよう、入る分だけ1文字ずつ写す） */
				LPCSTR pszSrc = (LPCSTR)&pData[dwOffsetTmp];
				int nLen = (int)strlen (pszSrc), nPos = 0;
				while (nPos < nLen) {
					int nChar = (IsDBCSLeadByte ((BYTE)pszSrc[nPos]) && (nPos + 1 < nLen)) ? 2 : 1;
					if (nPos + nChar > MAX_MAGICEXPLAIN - 1) {
						break;
					}
					nPos += nChar;
				}
				CopyMemory (pInfo->szExplain, pszSrc, nPos);
				pInfo->szExplain[nPos] = 0;
				dwOffsetTmp += (nLen + 1);
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
