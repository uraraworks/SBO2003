/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadJobInfo.cpp									 */
/* 概要：		職業情報読み書き処理(実装ファイル)							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/10													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "../../../Common/Info/InfoJob.h"
#include "SboSvrRecvCmd.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

#define DATACOUNTJOBINFO		(14)				/* 保存データ数 */

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
	"bySkillType",			/* 特技種別 */
	"byMagicType",			/* 魔法種別 */
	"byEquType",			/* 装備種別 */
	"dwFlg",				/* フラグ */
	"byCountLevelUpInfo",	/* レベルアップ情報数 */
	"byPW",					/* 最大値(力) */
	"byDEX",				/* 最大値(素早さ) */
	"byST",					/* 最大値(体力) */
	"byIT",					/* 最大値(賢さ) */
	"byLK",					/* 最大値(運のよさ) */
	"wMAXHP",				/* 最大値(最大HP) */
	"wMAXMANA",				/* 最大値(最大Mana) */
	"dwExp",				/* レベルアップに必要な経験値 */
	NULL
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveJobInfo(void);											/* 職業情報情報保存 */
BOOL LoadJobInfo(void);											/* 職業情報読み込み */


/* ========================================================================= */
/* 関数名：SaveJobInfo														 */
/* 内容：職業情報情報保存													 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
BOOL SaveJobInfo(void)
{
	BYTE byTmp;
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	char szFileName[MAX_PATH + 1];
	WORD wTmp;
	DWORD i, j, k, dwCount, dwColumnCount, dwDataCount, dwHeaderSize,
			dwDataSize, dwOffset, dwBytes, dwTmp, dwCountLevel;
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
	strcat (szFileName, "\\SboDataJobInfo.dat");


	/*************************
		データの初期設定
	*************************/

	for (dwColumnCount = 0; ; dwColumnCount ++) {
		if (s_aszName[dwColumnCount] == NULL) {
			break;
		}
	}
	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwDataCount = g_JobInfo.GetCount ();


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;

		ZeroMemory (&Info, sizeof (Info));
		if (s_aszName[i] == NULL) {
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
			PJOBINFO pInfo;

			pInfo			= g_JobInfo.GetPtr ((int)j);
			dwCountLevel	= pInfo->aLevelUpInfo.GetSize ();
			if ((i >= 6) && (i <= 13)) {
				if (dwCountLevel == 0) {
					continue;
				}
			}
			switch (i) {
			case 0:		dwDataSizeTmp += (strlen (pInfo->pszName) + 1);		break;	/* 名前 */
			case 1:		dwDataSizeTmp += sizeof (BYTE);						break;	/* 特技種別 */
			case 2:		dwDataSizeTmp += sizeof (BYTE);						break;	/* 魔法種別 */
			case 3:		dwDataSizeTmp += sizeof (BYTE);						break;	/* 装備種別 */
			case 4:		dwDataSizeTmp += sizeof (DWORD);					break;	/* フラグ */
			case 5:		dwDataSizeTmp += sizeof (BYTE);						break;	/* レベルアップ情報数 */
			case 6:		dwDataSizeTmp += (sizeof (BYTE) * dwCountLevel);	break;	/* 最大値(力) */
			case 7:		dwDataSizeTmp += (sizeof (BYTE) * dwCountLevel);	break;	/* 最大値(素早さ) */
			case 8:		dwDataSizeTmp += (sizeof (BYTE) * dwCountLevel);	break;	/* 最大値(体力) */
			case 9:		dwDataSizeTmp += (sizeof (BYTE) * dwCountLevel);	break;	/* 最大値(賢さ) */
			case 10:	dwDataSizeTmp += (sizeof (BYTE) * dwCountLevel);	break;	/* 最大値(運のよさ) */
			case 11:	dwDataSizeTmp += (sizeof (WORD) * dwCountLevel);	break;	/* 最大値(最大HP) */
			case 12:	dwDataSizeTmp += (sizeof (WORD) * dwCountLevel);	break;	/* 最大値(最大Mana) */
			case 13:	dwDataSizeTmp += (sizeof (DWORD) * dwCountLevel);	break;	/* レベルアップに必要な経験値 */
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
			PJOBINFO pInfo;
			PLVUPINFO pLvUpInfo;

			pInfo = g_JobInfo.GetPtr ((int)j);
			switch (i) {
			case 0:		/* 名前 */
				strcpy ((LPSTR)&pData[dwOffset], pInfo->pszName);
				dwOffset += (strlen (pInfo->pszName) + 1);
				break;
			case 1:		/* 特技種別 */
				CopyMemory (&pData[dwOffset], &pInfo->bySkillType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 2:		/* 魔法種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byMagicType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 3:		/* 装備種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byEquType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 4:		/* フラグ */
				CopyMemory (&pData[dwOffset], &pInfo->dwFlg, sizeof (DWORD));
				dwOffset += sizeof (DWORD);
				break;
			case 5:		/* レベルアップ情報数 */
				byTmp = (BYTE)pInfo->aLevelUpInfo.GetSize ();
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 6:		/* 最大値(力) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					byTmp		= pLvUpInfo->byPW;
					CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
					dwOffset += sizeof (byTmp);
				}
				break;
			case 7:		/* 最大値(素早さ) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					byTmp		= pLvUpInfo->byDEX;
					CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
					dwOffset += sizeof (byTmp);
				}
				break;
			case 8:		/* 最大値(体力) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					byTmp		= pLvUpInfo->byST;
					CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
					dwOffset += sizeof (byTmp);
				}
				break;
			case 9:		/* 最大値(賢さ) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					byTmp		= pLvUpInfo->byIT;
					CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
					dwOffset += sizeof (byTmp);
				}
				break;
			case 10:	/* 最大値(運のよさ) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					byTmp		= pLvUpInfo->byLK;
					CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
					dwOffset += sizeof (byTmp);
				}
				break;
			case 11:	/* 最大値(最大HP) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					wTmp		= pLvUpInfo->wMAXHP;
					CopyMemory (&pData[dwOffset], &wTmp, sizeof (wTmp));
					dwOffset += sizeof (wTmp);
				}
				break;
			case 12:	/* 最大値(最大Mana) */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					wTmp		= pLvUpInfo->wMAXMANA;
					CopyMemory (&pData[dwOffset], &wTmp, sizeof (wTmp));
					dwOffset += sizeof (wTmp);
				}
				break;
			case 13:	/* レベルアップに必要な経験値 */
				dwCount = (DWORD)pInfo->aLevelUpInfo.GetSize ();
				for (k = 0; k < dwCount; k ++) {
					pLvUpInfo	= pInfo->aLevelUpInfo[k];
					dwTmp		= pLvUpInfo->dwExp;
					CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
					dwOffset += sizeof (dwTmp);
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
/* 関数名：LoadJobInfo														 */
/* 内容：職業情報読み込み													 */
/* 日付：2004/07/10															 */
/* ========================================================================= */
BOOL LoadJobInfo(void)
{
	int k;
	BYTE byTmp;
	char szFileName[MAX_PATH + 1], szTmp[128];
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	WORD wTmp;
	DWORD i, j, l, dwSize, dwBytes, dwOffset, dwDataCount, dwColumnCount, dwCountTmp, dwTmp;
	PLVUPINFO pLvUpInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	for (k = strlen (szFileName) - 1; k > 0; k --) {
		if (szFileName[k] == '\\') {
			szFileName[k + 1] = 0;
			break;
		}
	}
	strcat (szFileName, "SBODATA\\SboDataJobInfo.dat");


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

	g_JobInfo.DeleteAll ();

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
			if (s_aszName[j] == NULL) {
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
		PJOBINFO pInfo;

		pInfo = g_JobInfo.GetNew ();
		g_JobInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PJOBINFO pInfo;

			pInfo = g_JobInfo.GetPtr ((int)j);
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {					/* 名前 */
				strcpy (szTmp, (LPSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen (szTmp) + 1);
				g_JobInfo.SetJobName (pInfo, szTmp);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {				/* 特技種別 */
				CopyMemory (&pInfo->bySkillType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {				/* 魔法種別 */
				CopyMemory (&pInfo->byMagicType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {				/* 装備種別 */
				CopyMemory (&pInfo->byEquType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {				/* フラグ */
				CopyMemory (&pInfo->dwFlg, &pData[dwOffsetTmp], sizeof (DWORD));
				dwOffsetTmp += sizeof (DWORD);

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {				/* レベルアップ情報数 */
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
				for (l = 0; l < byTmp; l ++) {
					pLvUpInfo = g_JobInfo.GetNewLvUpInfo ();
					pInfo->aLevelUpInfo.Add (pLvUpInfo);
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[6]) == 0) {				/* 最大値(力) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
					dwOffsetTmp += sizeof (byTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->byPW = byTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[7]) == 0) {				/* 最大値(素早さ) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
					dwOffsetTmp += sizeof (byTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->byDEX = byTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[8]) == 0) {				/* 最大値(体力) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
					dwOffsetTmp += sizeof (byTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->byST = byTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[9]) == 0) {				/* 最大値(賢さ) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
					dwOffsetTmp += sizeof (byTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->byIT = byTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[10]) == 0) {			/* 最大値(運のよさ) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
					dwOffsetTmp += sizeof (byTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->byLK = byTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[11]) == 0) {			/* 最大値(最大HP) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&wTmp, &pData[dwOffsetTmp], sizeof (wTmp));
					dwOffsetTmp += sizeof (wTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->wMAXHP = wTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[12]) == 0) {			/* 最大値(最大Mana) */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&wTmp, &pData[dwOffsetTmp], sizeof (wTmp));
					dwOffsetTmp += sizeof (wTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->wMAXMANA = wTmp;
				}

			} else if (strcmp (aHeaderInfo[i].szName, s_aszName[13]) == 0) {			/* レベルアップに必要な経験値 */
				dwCountTmp = pInfo->aLevelUpInfo.GetSize ();
				for (l = 0; l < dwCountTmp; l ++) {
					CopyMemory (&dwTmp, &pData[dwOffsetTmp], sizeof (dwTmp));
					dwOffsetTmp += sizeof (dwTmp);

					pLvUpInfo = pInfo->aLevelUpInfo[l];
					pLvUpInfo->dwExp = dwTmp;
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
