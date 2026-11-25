/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadPuzzleRecord.cpp								 */
/* 概要：		落ち物パズルのハイスコア・連勝記録の読み書き処理(実装ファイル) */
/* 作成日：		2026/11/25													 */
/* ========================================================================= */
/*
 * 保存先は SBODATA/SboDataPuzzleRecord.dat。取引所・リサイクルボックスと同じ「カラム単位」の形式。
 * ハイスコアと連勝記録は 1 つの表に入れ、byKind で分ける（0=ハイスコア・1=連勝記録）。
 *   szName（キャラ名。UTF-8）・dwValue（得点または連勝数）・dwDate（日本時間の年月日。20261125 の形）
 *   並びは順位順（1 位から）。読み込む時も PuzzleRecordList::Add で入れ直すので、壊れた並びや重複は整う。
 * ファイルが無い時は 0 件で始める（1 件も無く、ファイルも無い時は保存しても作らない）。
 * 知らないカラムは読み飛ばし、無いカラムは 0 のまま読める。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrPuzzle.h"
#include "../../../common/FallingPuzzle.h"
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
	COL_U32ARR,							/* DWORD の配列（nSize は要素数） */
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

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
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
	case COL_U32ARR:	return pCol->nSize * 4;
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
			case COL_U32ARR:	nSizeOne = pCol->nSize * 4;			break;
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

/* ファイルに書く 1 件ぶん */
typedef struct _PUZZLERECFILE {
	BYTE		byKind;												/* 0=ハイスコア・1=連勝記録 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名（UTF-8） */
	DWORD		dwValue;											/* 得点または連勝数 */
	DWORD		dwDate;												/* 日付（日本時間の年月日） */
} PUZZLERECFILE;

static const COLINFO s_aPuzzleRecCol[] = {
	COL (PUZZLERECFILE, byKind,		COL_U8),
	COLS(PUZZLERECFILE, szName,		COL_STR, MAX_CHARNAMEBUF),
	COL (PUZZLERECFILE, dwValue,	COL_U32),
	COL (PUZZLERECFILE, dwDate,		COL_U32),
};

#define PUZZLERECFILE_MAX		(1000)								/* 読み込む件数の上限 */

BOOL SavePuzzleRecord(void);										/* 落ち物パズルの記録保存 */
BOOL LoadPuzzleRecord(void);										/* 落ち物パズルの記録読み込み */

/* ========================================================================= */
/* 関数名：SavePuzzleRecord													 */
/* 内容：落ち物パズルのハイスコア・連勝記録の保存							 */
/* ========================================================================= */
BOOL SavePuzzleRecord(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<PUZZLERECFILE> aData;
	int i, nCount = g_PuzzleScoreRec.nCount + g_PuzzleStreakRec.nCount;

	MakeFileName (szFile, "SboDataPuzzleRecord.dat");
	/* 1 件も無く、ファイルも無い時は作らない（公開リポジトリにはデータを置かない） */
	if (nCount == 0) {
		HANDLE hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return TRUE;
		}
		CloseHandle (hFile);
	}
	aData.resize (nCount ? nCount : 1);
	memset (&aData[0], 0, sizeof (PUZZLERECFILE) * aData.size ());
	for (i = 0; i < g_PuzzleScoreRec.nCount; i ++) {
		PUZZLERECFILE *p = &aData[i];
		p->byKind = 0;
		memcpy (p->szName, g_PuzzleScoreRec.aRec[i].szName, PUZZLE_RECORD_NAMEBUF);
		p->szName[MAX_CHARNAMEBUF - 1] = 0;
		p->dwValue = g_PuzzleScoreRec.aRec[i].dwValue;
		p->dwDate = g_PuzzleScoreRec.aRec[i].dwDate;
	}
	for (i = 0; i < g_PuzzleStreakRec.nCount; i ++) {
		PUZZLERECFILE *p = &aData[g_PuzzleScoreRec.nCount + i];
		p->byKind = 1;
		memcpy (p->szName, g_PuzzleStreakRec.aRec[i].szName, PUZZLE_RECORD_NAMEBUF);
		p->szName[MAX_CHARNAMEBUF - 1] = 0;
		p->dwValue = g_PuzzleStreakRec.aRec[i].dwValue;
		p->dwDate = g_PuzzleStreakRec.aRec[i].dwDate;
	}
	return SaveColumns (szFile, s_aPuzzleRecCol, sizeof (s_aPuzzleRecCol) / sizeof (s_aPuzzleRecCol[0]), (const BYTE *)&aData[0], sizeof (PUZZLERECFILE), nCount);
}

/* ========================================================================= */
/* 関数名：LoadPuzzleRecord													 */
/* 内容：落ち物パズルのハイスコア・連勝記録の読み込み（ファイルが無い時は 0 件）	 */
/* ========================================================================= */
BOOL LoadPuzzleRecord(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	PuzzleRecordClear ();
	MakeFileName (szFile, "SboDataPuzzleRecord.dat");
	if (!LoadColumns (szFile, s_aPuzzleRecCol, sizeof (s_aPuzzleRecCol) / sizeof (s_aPuzzleRecCol[0]), sizeof (PUZZLERECFILE), PUZZLERECFILE_MAX, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		const PUZZLERECFILE *p = (const PUZZLERECFILE *)&aData[sizeof (PUZZLERECFILE) * i];
		char szName[MAX_CHARNAMEBUF];

		memcpy (szName, p->szName, sizeof (szName));
		szName[MAX_CHARNAMEBUF - 1] = 0;
		/* 順位順に並んでいるので、同点は先に読んだ方が上になる（Add が整える） */
		if (p->byKind == 0) {
			g_PuzzleScoreRec.Add (szName, p->dwValue, p->dwDate);
		} else if (p->byKind == 1) {
			g_PuzzleStreakRec.Add (szName, p->dwValue, p->dwDate);
		}
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
