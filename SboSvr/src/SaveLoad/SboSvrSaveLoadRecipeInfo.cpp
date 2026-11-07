/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadRecipeInfo.cpp								 */
/* 概要：		作り手情報・レシピ情報読み書き処理(実装ファイル)			 */
/* 作成日：		2026/11/07													 */
/* ========================================================================= */
/*
 * 保存先は SBODATA/SboDataMakerInfo.dat・SboDataRecipeInfo.dat。
 * ほかの SboData*.dat と同じ「カラム単位」の形式（[カラム数][データ数]、
 * 続けて カラム名\0＋開始位置、その後にカラムごとの全件分のデータ）で、
 * 項目の表（s_aMakerCol・s_aRecipeCol）に従って書き読みする。
 * ファイルが無い時は 0 件で始める（初めて保存した時に作られる）。
 * 後から項目を足しても、知らないカラムは読み飛ばし、無いカラムは 0 のまま読める。
 */
#include "include.h"
#include "Glbldata.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <vector>

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ヘッダ情報 */
typedef struct _HEADERINFO {
	char		szName[64];							/* カラム名 */
	DWORD		dwOffset;							/* オフセット */
} HEADERINFO, *LPHEADERINFO;

/* カラムの種類 */
enum {
	COL_U8 = 0,							/* BYTE */
	COL_U16,							/* WORD */
	COL_U32,							/* DWORD */
	COL_STR,							/* 文字列（nSize は終端込みのバイト数。保存は終端まで） */
	COL_U8ARR,							/* BYTE の配列（nSize は要素数） */
	COL_U16ARR,							/* WORD の配列（nSize は要素数） */
};

/* カラムの定義 */
typedef struct _COLINFO {
	const char	*pszName;							/* カラム名 */
	int			nType;								/* 種類(COL_*) */
	size_t		nOffset;							/* 構造体の中の位置 */
	size_t		nSize;								/* 大きさ */
} COLINFO, *PCOLINFO;

#define COL(st, name, type)			{ #name, type, offsetof(st, name), 0 }
#define COLS(st, name, type, size)	{ #name, type, offsetof(st, name), size }

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* 作り手情報のカラム */
static const COLINFO s_aMakerCol[] = {
	COL (MAKERINFO, byState,	COL_U8),
	COLS(MAKERINFO, szName,		COL_STR, MAX_MAKERNAME),
	COLS(MAKERINFO, szMsgTalk,	COL_STR, MAX_MAKERMSG),
	COLS(MAKERINFO, szMsgDone,	COL_STR, MAX_MAKERMSG),
	COLS(MAKERINFO, szMsgLack,	COL_STR, MAX_MAKERMSG),
};

/* レシピ情報のカラム */
static const COLINFO s_aRecipeCol[] = {
	COL (RECIPEINFO, byState,		COL_U8),
	COL (RECIPEINFO, byMaker,		COL_U8),
	COL (RECIPEINFO, wSort,			COL_U16),
	COL (RECIPEINFO, wItem,			COL_U16),
	COL (RECIPEINFO, byCount,		COL_U8),
	COLS(RECIPEINFO, awMatItem,		COL_U16ARR, MAX_RECIPEMAT),
	COLS(RECIPEINFO, abyMatCount,	COL_U8ARR, MAX_RECIPEMAT),
	COL (RECIPEINFO, dwGold,		COL_U32),
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveMakerInfo(void);										/* 作り手情報保存 */
BOOL LoadMakerInfo(void);										/* 作り手情報読み込み */
BOOL SaveRecipeInfo(void);										/* レシピ情報保存 */
BOOL LoadRecipeInfo(void);										/* レシピ情報読み込み */
static void MakeFileName(char *pszOut, const char *pszName);	/* 保存先のファイル名を作る */
static size_t ColDataSize(const COLINFO *pCol, const BYTE *pRec);	/* 1 件ぶんのデータの大きさ */
static BOOL SaveColumns(const char *pszFile, const COLINFO *paCol, int nColCount, const BYTE *pData, size_t nRecSize, int nCount);	/* カラム形式で保存 */
static BOOL LoadColumns(const char *pszFile, const COLINFO *paCol, int nColCount, size_t nRecSize, int nMax, std::vector<BYTE> &aOut, int *pnCount);	/* カラム形式で読み込み */

/* ========================================================================= */
/* 関数名：MakeFileName														 */
/* 内容：保存先のファイル名を作る											 */
/* ========================================================================= */
static void MakeFileName(char *pszOut, const char *pszName)
{
	int i;

	GetModuleFileName (NULL, pszOut, MAX_PATH);
	for (i = strlen (pszOut) - 1; i > 0; i --) {
		if (pszOut[i] == '\\') {
			pszOut[i + 1] = 0;
			break;
		}
	}
	strcat (pszOut, "SBODATA");
	CreateDirectory (pszOut, NULL);
	strcat (pszOut, "\\");
	strcat (pszOut, pszName);
}

/* ========================================================================= */
/* 関数名：ColDataSize														 */
/* 内容：1 件ぶんのカラムのデータの大きさ									 */
/* ========================================================================= */
static size_t ColDataSize(const COLINFO *pCol, const BYTE *pRec)
{
	switch (pCol->nType) {
	case COL_U8:		return 1;
	case COL_U16:		return 2;
	case COL_U32:		return 4;
	case COL_STR:
		{
			size_t n = 0;
			const char *psz = (const char *)(pRec + pCol->nOffset);
			while ((n < pCol->nSize - 1) && psz[n]) {
				n ++;
			}
			return n + 1;
		}
	case COL_U8ARR:		return pCol->nSize;
	case COL_U16ARR:	return pCol->nSize * 2;
	}
	return 0;
}

/* ========================================================================= */
/* 関数名：SaveColumns														 */
/* 内容：カラム形式で保存（別名で書いてから置き換える）					 */
/* ========================================================================= */
static BOOL SaveColumns(const char *pszFile, const COLINFO *paCol, int nColCount, const BYTE *pData, size_t nRecSize, int nCount)
{
	std::vector<BYTE> aBody, aHead;
	std::vector<DWORD> aOffset;
	DWORD dwHeaderSize, dwTmp;
	int i, j;
	char szTmp[MAX_PATH + 8];
	HANDLE hFile;
	DWORD dwBytes;

	/* データ部分（カラムごとに全件分） */
	for (i = 0; i < nColCount; i ++) {
		aOffset.push_back ((DWORD)aBody.size ());
		for (j = 0; j < nCount; j ++) {
			const BYTE *pRec = pData + nRecSize * j;
			size_t nSize = ColDataSize (&paCol[i], pRec);
			if (paCol[i].nType == COL_STR) {
				/* 文字列は終端まで書き、終端の 0 を付ける */
				aBody.insert (aBody.end (), pRec + paCol[i].nOffset, pRec + paCol[i].nOffset + nSize - 1);
				aBody.push_back (0);
			} else {
				aBody.insert (aBody.end (), pRec + paCol[i].nOffset, pRec + paCol[i].nOffset + nSize);
			}
		}
	}

	/* ヘッダ部分 */
	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; i < nColCount; i ++) {
		dwHeaderSize += (DWORD)(strlen (paCol[i].pszName) + 1 + sizeof (DWORD));
	}
	dwTmp = (DWORD)nColCount;
	aHead.insert (aHead.end (), (BYTE *)&dwTmp, (BYTE *)&dwTmp + sizeof (dwTmp));
	dwTmp = (DWORD)nCount;
	aHead.insert (aHead.end (), (BYTE *)&dwTmp, (BYTE *)&dwTmp + sizeof (dwTmp));
	for (i = 0; i < nColCount; i ++) {
		aHead.insert (aHead.end (), (const BYTE *)paCol[i].pszName, (const BYTE *)paCol[i].pszName + strlen (paCol[i].pszName) + 1);
		dwTmp = dwHeaderSize + aOffset[i];
		aHead.insert (aHead.end (), (BYTE *)&dwTmp, (BYTE *)&dwTmp + sizeof (dwTmp));
	}

	/* 途中で止まっても元のファイルが壊れないよう、別名で書いてから置き換える */
	strcpy (szTmp, pszFile);
	strcat (szTmp, ".tmp");
	hFile = CreateFile (szTmp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	WriteFile (hFile, &aHead[0], (DWORD)aHead.size (), &dwBytes, NULL);
	if (!aBody.empty ()) {
		WriteFile (hFile, &aBody[0], (DWORD)aBody.size (), &dwBytes, NULL);
	}
	CloseHandle (hFile);
	DeleteFile (pszFile);
	if (!MoveFile (szTmp, pszFile)) {
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：LoadColumns														 */
/* 内容：カラム形式で読み込み（ファイルが無い・壊れている時は FALSE で 0 件）	 */
/* ========================================================================= */
static BOOL LoadColumns(const char *pszFile, const COLINFO *paCol, int nColCount, size_t nRecSize, int nMax, std::vector<BYTE> &aOut, int *pnCount)
{
	HANDLE hFile;
	DWORD dwSize, dwBytes, dwOffset, dwColumnCount, dwDataCount, i, j;
	std::vector<BYTE> aFile;
	BOOL bRet = FALSE;

	*pnCount = 0;
	aOut.clear ();
	hFile = CreateFile (pszFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	dwSize = GetFileSize (hFile, NULL);
	if (((int)dwSize == -1) || (dwSize < sizeof (DWORD) * 2)) {
		CloseHandle (hFile);
		return FALSE;
	}
	aFile.resize (dwSize);
	ReadFile (hFile, &aFile[0], dwSize, &dwBytes, NULL);
	CloseHandle (hFile);
	if (dwBytes != dwSize) {
		return FALSE;
	}

	dwOffset = 0;
	memcpy (&dwColumnCount, &aFile[dwOffset], sizeof (DWORD));
	dwOffset += sizeof (DWORD);
	memcpy (&dwDataCount, &aFile[dwOffset], sizeof (DWORD));
	dwOffset += sizeof (DWORD);
	if ((dwColumnCount > 64) || (dwDataCount > (DWORD)nMax)) {
		return FALSE;
	}

	aOut.assign (nRecSize * dwDataCount, 0);

	for (i = 0; i < dwColumnCount; i ++) {
		char szName[64];
		DWORD dwColOffset;
		int nCol = -1;
		size_t nLen;

		/* カラム名と開始位置 */
		for (nLen = 0; (dwOffset + nLen < dwSize) && aFile[dwOffset + nLen]; nLen ++) {
		}
		if ((dwOffset + nLen + 1 + sizeof (DWORD) > dwSize) || (nLen >= sizeof (szName))) {
			aOut.clear ();
			return FALSE;
		}
		memcpy (szName, &aFile[dwOffset], nLen + 1);
		dwOffset += (DWORD)(nLen + 1);
		memcpy (&dwColOffset, &aFile[dwOffset], sizeof (DWORD));
		dwOffset += sizeof (DWORD);

		for (j = 0; j < (DWORD)nColCount; j ++) {
			if (strcmp (szName, paCol[j].pszName) == 0) {
				nCol = (int)j;
				break;
			}
		}
		if (nCol < 0) {
			continue;		/* 知らないカラムは読み飛ばす */
		}

		/* 全件分を読む */
		for (j = 0; j < dwDataCount; j ++) {
			BYTE *pRec = &aOut[nRecSize * j];
			const COLINFO *pCol = &paCol[nCol];
			size_t nSizeOne = 0;

			switch (pCol->nType) {
			case COL_U8:		nSizeOne = 1;						break;
			case COL_U16:		nSizeOne = 2;						break;
			case COL_U32:		nSizeOne = 4;						break;
			case COL_U8ARR:		nSizeOne = pCol->nSize;				break;
			case COL_U16ARR:	nSizeOne = pCol->nSize * 2;			break;
			case COL_STR:
				for (nSizeOne = 0; (dwColOffset + nSizeOne < dwSize) && aFile[dwColOffset + nSizeOne]; nSizeOne ++) {
				}
				if (dwColOffset + nSizeOne >= dwSize) {
					aOut.clear ();
					return FALSE;
				}
				break;
			}
			if ((size_t)dwColOffset + nSizeOne + ((pCol->nType == COL_STR) ? 1 : 0) > dwSize) {
				aOut.clear ();
				return FALSE;
			}
			if (pCol->nType == COL_STR) {
				/* 欄より長い文字列は欄に収まるところまで（終端は必ず付ける） */
				size_t nCopy = min (nSizeOne, pCol->nSize - 1);
				memcpy (pRec + pCol->nOffset, &aFile[dwColOffset], nCopy);
				pRec[pCol->nOffset + nCopy] = 0;
				dwColOffset += (DWORD)(nSizeOne + 1);
			} else {
				memcpy (pRec + pCol->nOffset, &aFile[dwColOffset], nSizeOne);
				dwColOffset += (DWORD)nSizeOne;
			}
		}
	}

	*pnCount = (int)dwDataCount;
	bRet = TRUE;
	return bRet;
}

/* ========================================================================= */
/* 関数名：SaveMakerInfo													 */
/* 内容：作り手情報保存														 */
/* ========================================================================= */
BOOL SaveMakerInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = g_aMakerInfo.GetSize ();

	MakeFileName (szFile, "SboDataMakerInfo.dat");
	/* 1 件も無く、ファイルも無い時は作らない（公開リポジトリにはデータを置かない） */
	if (nCount == 0) {
		HANDLE hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return TRUE;
		}
		CloseHandle (hFile);
	}
	aData.assign (sizeof (MAKERINFO) * (nCount ? nCount : 1), 0);
	for (i = 0; i < nCount; i ++) {
		memcpy (&aData[sizeof (MAKERINFO) * i], g_aMakerInfo[i], sizeof (MAKERINFO));
	}
	return SaveColumns (szFile, s_aMakerCol, sizeof (s_aMakerCol) / sizeof (s_aMakerCol[0]), &aData[0], sizeof (MAKERINFO), nCount);
}

/* ========================================================================= */
/* 関数名：LoadMakerInfo													 */
/* 内容：作り手情報読み込み													 */
/* ========================================================================= */
BOOL LoadMakerInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataMakerInfo.dat");
	if (!LoadColumns (szFile, s_aMakerCol, sizeof (s_aMakerCol) / sizeof (s_aMakerCol[0]), sizeof (MAKERINFO), MAX_MAKER, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		PMAKERINFO pInfo = new MAKERINFO;
		memcpy (pInfo, &aData[sizeof (MAKERINFO) * i], sizeof (MAKERINFO));
		g_aMakerInfo.Add (pInfo);
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SaveRecipeInfo													 */
/* 内容：レシピ情報保存														 */
/* ========================================================================= */
BOOL SaveRecipeInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = g_aRecipeInfo.GetSize ();

	MakeFileName (szFile, "SboDataRecipeInfo.dat");
	/* 1 件も無く、ファイルも無い時は作らない（公開リポジトリにはデータを置かない） */
	if (nCount == 0) {
		HANDLE hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return TRUE;
		}
		CloseHandle (hFile);
	}
	aData.assign (sizeof (RECIPEINFO) * (nCount ? nCount : 1), 0);
	for (i = 0; i < nCount; i ++) {
		memcpy (&aData[sizeof (RECIPEINFO) * i], g_aRecipeInfo[i], sizeof (RECIPEINFO));
	}
	return SaveColumns (szFile, s_aRecipeCol, sizeof (s_aRecipeCol) / sizeof (s_aRecipeCol[0]), &aData[0], sizeof (RECIPEINFO), nCount);
}

/* ========================================================================= */
/* 関数名：LoadRecipeInfo													 */
/* 内容：レシピ情報読み込み													 */
/* ========================================================================= */
BOOL LoadRecipeInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataRecipeInfo.dat");
	if (!LoadColumns (szFile, s_aRecipeCol, sizeof (s_aRecipeCol) / sizeof (s_aRecipeCol[0]), sizeof (RECIPEINFO), MAX_RECIPE, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		PRECIPEINFO pInfo = new RECIPEINFO;
		memcpy (pInfo, &aData[sizeof (RECIPEINFO) * i], sizeof (RECIPEINFO));
		g_aRecipeInfo.Add (pInfo);
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
