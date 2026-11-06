/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadStTmpInfo.cpp									 */
/* 概要：		ステータステンプレート情報読み書き処理(実装ファイル)		 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/12/01													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define DATACOUNT		(27)									/* 保存データ数 */

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
	"byLevel",				/* レベル */
	"byPW",					/* 力 */
	"byDEX",				/* 素早さ */
	"byST",					/* 体力 */
	"byIT",					/* 賢さ */
	"byLK",					/* 運のよさ */
	"byNpcType",			/* 職業 */
	"byDropProbability",	/* ドロップ率 */
	"byMoveProbabilityX",	/* 移動確率X */
	"byMoveProbabilityY",	/* 移動確率Y */
	"bySearchCx",			/* 検索範囲X */
	"bySearchCy",			/* 検索範囲Y */
	"abyMagic",				/* 呪文 */
	"chAttrFire",			/* 属性(火) */
	"chAttrWater",			/* 属性(水) */
	"chAttrWind",			/* 属性(風) */
	"chAttrEarth",			/* 属性(土) */
	"chAttrLight",			/* 属性(光) */
	"chAttrDark",			/* 属性(闇) */
	"wMaxHP",				/* 最大HP */
	"wMaxMana",				/* 最大Mana */
	"wDFMagic",				/* 魔法防御 */
	"waItem",				/* 所持アイテム */
	"dwGold",				/* 所持金 */
	"dwExp",				/* 経験値 */
	"byStatusResist",		/* 毒・眠りの効きにくさ(%) */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveStTmpInfo(void);										/* ステータステンプレート情報情報保存 */
BOOL LoadStTmpInfo(void);										/* ステータステンプレート情報読み込み */

/* ========================================================================= */
/* 関数名：SaveStTmpInfo													 */
/* 内容：ステータステンプレート情報情報保存									 */
/* 日付：2004/12/15															 */
/* ========================================================================= */
BOOL SaveStTmpInfo(void)
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
	strcat (szFileName, "\\SboDataStTmpInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = DATACOUNT;
	dwDataCount = (DWORD)g_aStTmpInfo.GetSize ();


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
			PSTATUSTEMPLATEINFO pInfo;

			pInfo = g_aStTmpInfo[j];
			switch (i) {
			case 0:		dwDataSizeTmp += (strlen (pInfo->szName) + 1);			break;	/* キャラ名 */
			case 1:		dwDataSizeTmp += sizeof (pInfo->byLevel);				break;	/* レベル */
			case 2:		dwDataSizeTmp += sizeof (pInfo->byPW);					break;	/* 力 */
			case 3:		dwDataSizeTmp += sizeof (pInfo->byDEX);					break;	/* 素早さ */
			case 4:		dwDataSizeTmp += sizeof (pInfo->byST);					break;	/* 体力 */
			case 5:		dwDataSizeTmp += sizeof (pInfo->byIT);					break;	/* 賢さ */
			case 6:		dwDataSizeTmp += sizeof (pInfo->byLK);					break;	/* 運のよさ */
			case 7:		dwDataSizeTmp += sizeof (pInfo->byNpcType);				break;	/* 職業 */
			case 8:		dwDataSizeTmp += sizeof (pInfo->byDropProbability);		break;	/* ドロップ率 */
			case 9:		dwDataSizeTmp += sizeof (pInfo->byMoveProbabilityX);	break;	/* 移動確率X */
			case 10:	dwDataSizeTmp += sizeof (pInfo->byMoveProbabilityY);	break;	/* 移動確率Y */
			case 11:	dwDataSizeTmp += sizeof (pInfo->bySearchCx);			break;	/* 検索範囲X */
			case 12:	dwDataSizeTmp += sizeof (pInfo->bySearchCy);			break;	/* 検索範囲Y */
			case 13:	dwDataSizeTmp += sizeof (pInfo->abyMagic);				break;	/* 呪文 */
			case 14:	dwDataSizeTmp += sizeof (pInfo->chAttrFire);			break;	/* 属性(火) */
			case 15:	dwDataSizeTmp += sizeof (pInfo->chAttrWater);			break;	/* 属性(水) */
			case 16:	dwDataSizeTmp += sizeof (pInfo->chAttrWind);			break;	/* 属性(風) */
			case 17:	dwDataSizeTmp += sizeof (pInfo->chAttrEarth);			break;	/* 属性(土) */
			case 18:	dwDataSizeTmp += sizeof (pInfo->chAttrLight);			break;	/* 属性(光) */
			case 19:	dwDataSizeTmp += sizeof (pInfo->chAttrDark);			break;	/* 属性(闇) */
			case 20:	dwDataSizeTmp += sizeof (pInfo->wMaxHP);				break;	/* 最大HP */
			case 21:	dwDataSizeTmp += sizeof (pInfo->wMaxMana);				break;	/* 最大Mana */
			case 22:	dwDataSizeTmp += sizeof (pInfo->wDFMagic);				break;	/* 魔法防御 */
			case 23:	dwDataSizeTmp += sizeof (pInfo->waItem);				break;	/* 所持アイテム */
			case 24:	dwDataSizeTmp += sizeof (pInfo->dwGold);				break;	/* 所持金 */
			case 25:	dwDataSizeTmp += sizeof (pInfo->dwExp);					break;	/* 経験値 */
			case 26:	dwDataSizeTmp += sizeof (pInfo->byStatusResist);		break;	/* 毒・眠りの効きにくさ */
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
			PSTATUSTEMPLATEINFO pInfo;

			pInfo = g_aStTmpInfo[j];
			switch (i) {
			case 0:		/* キャラ名 */
				strcpy ((LPSTR)&pData[dwOffset], pInfo->szName);
				dwOffset += (strlen (pInfo->szName) + 1);
				break;
			case 1:		/* レベル */
				CopyMemory (&pData[dwOffset], &pInfo->byLevel, sizeof (pInfo->byLevel));
				dwOffset += sizeof (pInfo->byLevel);
				break;
			case 2:		/* 力 */
				CopyMemory (&pData[dwOffset], &pInfo->byPW, sizeof (pInfo->byPW));
				dwOffset += sizeof (pInfo->byPW);
				break;
			case 3:		/* 素早さ */
				CopyMemory (&pData[dwOffset], &pInfo->byDEX, sizeof (pInfo->byDEX));
				dwOffset += sizeof (pInfo->byDEX);
				break;
			case 4:		/* 体力 */
				CopyMemory (&pData[dwOffset], &pInfo->byST, sizeof (pInfo->byST));
				dwOffset += sizeof (pInfo->byST);
				break;
			case 5:		/* 賢さ */
				CopyMemory (&pData[dwOffset], &pInfo->byIT, sizeof (pInfo->byIT));
				dwOffset += sizeof (pInfo->byIT);
				break;
			case 6:		/* 運のよさ */
				CopyMemory (&pData[dwOffset], &pInfo->byLK, sizeof (pInfo->byLK));
				dwOffset += sizeof (pInfo->byLK);
				break;
			case 7:		/* 職業 */
				CopyMemory (&pData[dwOffset], &pInfo->byNpcType, sizeof (pInfo->byNpcType));
				dwOffset += sizeof (pInfo->byNpcType);
				break;
			case 8:		/* ドロップ率 */
				CopyMemory (&pData[dwOffset], &pInfo->byDropProbability, sizeof (pInfo->byDropProbability));
				dwOffset += sizeof (pInfo->byDropProbability);
				break;
			case 9:		/* 移動確率X */
				CopyMemory (&pData[dwOffset], &pInfo->byMoveProbabilityX, sizeof (pInfo->byMoveProbabilityX));
				dwOffset += sizeof (pInfo->byMoveProbabilityX);
				break;
			case 10:	/* 移動確率Y */
				CopyMemory (&pData[dwOffset], &pInfo->byMoveProbabilityY, sizeof (pInfo->byMoveProbabilityY));
				dwOffset += sizeof (pInfo->byMoveProbabilityY);
				break;
			case 11:	/* 検索範囲X */
				CopyMemory (&pData[dwOffset], &pInfo->bySearchCx, sizeof (pInfo->bySearchCx));
				dwOffset += sizeof (pInfo->bySearchCx);
				break;
			case 12:	/* 検索範囲Y */
				CopyMemory (&pData[dwOffset], &pInfo->bySearchCy, sizeof (pInfo->bySearchCy));
				dwOffset += sizeof (pInfo->bySearchCy);
				break;
			case 13:	/* 呪文 */
				CopyMemory (&pData[dwOffset], &pInfo->abyMagic, sizeof (pInfo->abyMagic));
				dwOffset += sizeof (pInfo->abyMagic);
				break;
			case 14:	/* 属性(火) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrFire, sizeof (pInfo->chAttrFire));
				dwOffset += sizeof (pInfo->chAttrFire);
				break;
			case 15:	/* 属性(水) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrWater, sizeof (pInfo->chAttrWater));
				dwOffset += sizeof (pInfo->chAttrWater);
				break;
			case 16:	/* 属性(風) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrWind, sizeof (pInfo->chAttrWind));
				dwOffset += sizeof (pInfo->chAttrWind);
				break;
			case 17:	/* 属性(土) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrEarth, sizeof (pInfo->chAttrEarth));
				dwOffset += sizeof (pInfo->chAttrEarth);
				break;
			case 18:	/* 属性(光) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrLight, sizeof (pInfo->chAttrLight));
				dwOffset += sizeof (pInfo->chAttrLight);
				break;
			case 19:	/* 属性(闇) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrDark, sizeof (pInfo->chAttrDark));
				dwOffset += sizeof (pInfo->chAttrDark);
				break;
			case 20:	/* 最大HP */
				CopyMemory (&pData[dwOffset], &pInfo->wMaxHP, sizeof (pInfo->wMaxHP));
				dwOffset += sizeof (pInfo->wMaxHP);
				break;
			case 21:	/* 最大Mana */
				CopyMemory (&pData[dwOffset], &pInfo->wMaxMana, sizeof (pInfo->wMaxMana));
				dwOffset += sizeof (pInfo->wMaxMana);
				break;
			case 22:	/* 魔法防御 */
				CopyMemory (&pData[dwOffset], &pInfo->wDFMagic, sizeof (pInfo->wDFMagic));
				dwOffset += sizeof (pInfo->wDFMagic);
				break;
			case 23:	/* 所持アイテム */
				CopyMemory (&pData[dwOffset], &pInfo->waItem, sizeof (pInfo->waItem));
				dwOffset += sizeof (pInfo->waItem);
				break;
			case 24:	/* 所持金 */
				CopyMemory (&pData[dwOffset], &pInfo->dwGold, sizeof (pInfo->dwGold));
				dwOffset += sizeof (pInfo->dwGold);
				break;
			case 25:	/* 経験値 */
				CopyMemory (&pData[dwOffset], &pInfo->dwExp, sizeof (pInfo->dwExp));
				dwOffset += sizeof (pInfo->dwExp);
				break;
			case 26:	/* 毒・眠りの効きにくさ */
				CopyMemory (&pData[dwOffset], &pInfo->byStatusResist, sizeof (pInfo->byStatusResist));
				dwOffset += sizeof (pInfo->byStatusResist);
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
/* 関数名：LoadStTmpInfo													 */
/* 内容：ステータステンプレート情報読み込み									 */
/* 日付：2004/12/01															 */
/* ========================================================================= */
BOOL LoadStTmpInfo(void)
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
	strcat (szFileName, "SBODATA\\SboDataStTmpInfo.dat");


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
		PSTATUSTEMPLATEINFO pInfo;

		pInfo = new STATUSTEMPLATEINFO;
		ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
		g_aStTmpInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PSTATUSTEMPLATEINFO pInfo;

			pInfo = g_aStTmpInfo[j];
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {					/* キャラ名 */
				strcpy (pInfo->szName, (LPSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen ((LPSTR)&pData[dwOffsetTmp]) + 1);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {				/* レベル */
				CopyMemory (&pInfo->byLevel, &pData[dwOffsetTmp], sizeof (pInfo->byLevel));
				dwOffsetTmp += sizeof (pInfo->byLevel);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {				/* 力 */
				CopyMemory (&pInfo->byPW, &pData[dwOffsetTmp], sizeof (pInfo->byPW));
				dwOffsetTmp += sizeof (pInfo->byPW);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {				/* 素早さ */
				CopyMemory (&pInfo->byDEX, &pData[dwOffsetTmp], sizeof (pInfo->byDEX));
				dwOffsetTmp += sizeof (pInfo->byDEX);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {				/* 体力 */
				CopyMemory (&pInfo->byST, &pData[dwOffsetTmp], sizeof (pInfo->byST));
				dwOffsetTmp += sizeof (pInfo->byST);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {				/* 賢さ */
				CopyMemory (&pInfo->byIT, &pData[dwOffsetTmp], sizeof (pInfo->byIT));
				dwOffsetTmp += sizeof (pInfo->byIT);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[6]) == 0) {				/* 運のよさ */
				CopyMemory (&pInfo->byLK, &pData[dwOffsetTmp], sizeof (pInfo->byLK));
				dwOffsetTmp += sizeof (pInfo->byLK);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[7]) == 0) {				/* 職業 */
				CopyMemory (&pInfo->byNpcType, &pData[dwOffsetTmp], sizeof (pInfo->byNpcType));
				dwOffsetTmp += sizeof (pInfo->byNpcType);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[8]) == 0) {				/* ドロップ率 */
				CopyMemory (&pInfo->byDropProbability, &pData[dwOffsetTmp], sizeof (pInfo->byDropProbability));
				dwOffsetTmp += sizeof (pInfo->byDropProbability);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[9]) == 0) {				/* 移動確率X */
				CopyMemory (&pInfo->byMoveProbabilityX, &pData[dwOffsetTmp], sizeof (pInfo->byMoveProbabilityX));
				dwOffsetTmp += sizeof (pInfo->byMoveProbabilityX);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[10]) == 0) {				/* 移動確率Y */
				CopyMemory (&pInfo->byMoveProbabilityY, &pData[dwOffsetTmp], sizeof (pInfo->byMoveProbabilityY));
				dwOffsetTmp += sizeof (pInfo->byMoveProbabilityY);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[11]) == 0) {			/* 検索範囲X */
				CopyMemory (&pInfo->bySearchCx, &pData[dwOffsetTmp], sizeof (pInfo->bySearchCx));
				dwOffsetTmp += sizeof (pInfo->bySearchCx);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[12]) == 0) {			/* 検索範囲Y */
				CopyMemory (&pInfo->bySearchCy, &pData[dwOffsetTmp], sizeof (pInfo->bySearchCy));
				dwOffsetTmp += sizeof (pInfo->bySearchCy);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[13]) == 0) {			/* 呪文 */
				CopyMemory (&pInfo->abyMagic, &pData[dwOffsetTmp], sizeof (pInfo->abyMagic));
				dwOffsetTmp += sizeof (pInfo->abyMagic);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[14]) == 0) {			/* 属性(火) */
				CopyMemory (&pInfo->chAttrFire, &pData[dwOffsetTmp], sizeof (pInfo->chAttrFire));
				dwOffsetTmp += sizeof (pInfo->chAttrFire);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[15]) == 0) {			/* 属性(水) */
				CopyMemory (&pInfo->chAttrWater, &pData[dwOffsetTmp], sizeof (pInfo->chAttrWater));
				dwOffsetTmp += sizeof (pInfo->chAttrWater);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[16]) == 0) {			/* 属性(風) */
				CopyMemory (&pInfo->chAttrWind, &pData[dwOffsetTmp], sizeof (pInfo->chAttrWind));
				dwOffsetTmp += sizeof (pInfo->chAttrWind);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[17]) == 0) {			/* 属性(土) */
				CopyMemory (&pInfo->chAttrEarth, &pData[dwOffsetTmp], sizeof (pInfo->chAttrEarth));
				dwOffsetTmp += sizeof (pInfo->chAttrEarth);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[18]) == 0) {			/* 属性(光) */
				CopyMemory (&pInfo->chAttrLight, &pData[dwOffsetTmp], sizeof (pInfo->chAttrLight));
				dwOffsetTmp += sizeof (pInfo->chAttrLight);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[19]) == 0) {			/* 属性(闇) */
				CopyMemory (&pInfo->chAttrDark, &pData[dwOffsetTmp], sizeof (pInfo->chAttrDark));
				dwOffsetTmp += sizeof (pInfo->chAttrDark);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[20]) == 0) {			/* 最大HP */
				CopyMemory (&pInfo->wMaxHP, &pData[dwOffsetTmp], sizeof (pInfo->wMaxHP));
				dwOffsetTmp += sizeof (pInfo->wMaxHP);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[21]) == 0) {			/* 最大Mana */
				CopyMemory (&pInfo->wMaxMana, &pData[dwOffsetTmp], sizeof (pInfo->wMaxMana));
				dwOffsetTmp += sizeof (pInfo->wMaxMana);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[22]) == 0) {			/* 魔法防御 */
				CopyMemory (&pInfo->wDFMagic, &pData[dwOffsetTmp], sizeof (pInfo->wDFMagic));
				dwOffsetTmp += sizeof (pInfo->wDFMagic);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[23]) == 0) {			/* 所持アイテム */
				CopyMemory (&pInfo->waItem, &pData[dwOffsetTmp], sizeof (pInfo->waItem));
				dwOffsetTmp += sizeof (pInfo->waItem);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[24]) == 0) {			/* 所持金 */
				CopyMemory (&pInfo->dwGold, &pData[dwOffsetTmp], sizeof (pInfo->dwGold));
				dwOffsetTmp += sizeof (pInfo->dwGold);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[25]) == 0) {			/* 経験値 */
				CopyMemory (&pInfo->dwExp, &pData[dwOffsetTmp], sizeof (pInfo->dwExp));
				dwOffsetTmp += sizeof (pInfo->dwExp);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[26]) == 0) {			/* 毒・眠りの効きにくさ(古いデータには無い) */
				CopyMemory (&pInfo->byStatusResist, &pData[dwOffsetTmp], sizeof (pInfo->byStatusResist));
				dwOffsetTmp += sizeof (pInfo->byStatusResist);
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
