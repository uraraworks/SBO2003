/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadTalkInfo.cpp									 */
/* 概要：		フラグの名前表・会話データ読み書き処理(実装ファイル)		 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
/*
 * 保存先は SBODATA/SboDataFlagNameInfo.dat（フラグの名前表）と SBODATA/SboDataTalkInfo.dat（会話データ）。
 * どちらも依頼情報（SboSvrSaveLoadQuestInfo.cpp）と同じ「カラム単位」の形式
 * （カラムの読み書きの部分は同じ作り）。
 * 会話セットは 16 行ぶんあるので、行ごとの項目は 16 行まとめた配列のカラム（awNeed・abyCondNo …）にして保存する。
 * 台詞は行ごとに別のカラム（szSpeech0〜szSpeech15）にして、使った長さだけを保存する。
 * ファイルが無い時は 0 件で始める。知らないカラムは読み飛ばし、無いカラムは 0 のまま読める。
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

/* ========================================================================= */
/* フラグの名前表															 */
/* ========================================================================= */

/* ファイルに書く 1 件ぶん */
typedef struct _FLAGNAMEFILE {
	BYTE		byState;
	BYTE		byKind;
	WORD		wNo;
	char		szName[MAX_FLAGNAME];
} FLAGNAMEFILE;

/* フラグの名前表のカラム */
static const COLINFO s_aFlagNameCol[] = {
	COL (FLAGNAMEFILE, byState,		COL_U8),
	COL (FLAGNAMEFILE, byKind,		COL_U8),
	COL (FLAGNAMEFILE, wNo,			COL_U16),
	COLS(FLAGNAMEFILE, szName,		COL_STR, MAX_FLAGNAME),
};

/* ========================================================================= */
/* 会話データ																 */
/* ========================================================================= */

/* ファイルに書く 1 件ぶん（行ごとの項目は 16 行ぶんまとめた配列にしてある） */
typedef struct _TALKFILE {
	WORD		wID;
	BYTE		byState;
	BYTE		byLineCount;
	char		szName[MAX_TALKNAME];
	BYTE		abyRank[MAX_TALKLINE];
	BYTE		abyLvMin[MAX_TALKLINE];
	BYTE		abyLvMax[MAX_TALKLINE];
	WORD		awNeed[MAX_TALKLINE * MAX_TALKNEED];
	WORD		awNot[MAX_TALKLINE * MAX_TALKNEED];
	BYTE		abyCondNo[MAX_TALKLINE * MAX_TALKCOND];
	BYTE		abyCondCmp[MAX_TALKLINE * MAX_TALKCOND];
	BYTE		abyCondVal[MAX_TALKLINE * MAX_TALKCOND];
	WORD		awSetOn[MAX_TALKLINE * MAX_TALKSET];
	WORD		awSetOff[MAX_TALKLINE * MAX_TALKSET];
	BYTE		abyActNo[MAX_TALKLINE * MAX_TALKACT];
	BYTE		abyActOp[MAX_TALKLINE * MAX_TALKACT];
	BYTE		abyActVal[MAX_TALKLINE * MAX_TALKACT];
	char		aszSpeech[MAX_TALKLINE][MAX_TALKMSG];
	WORD		awGiveItem[MAX_TALKLINE];			/* 渡すアイテム（0=渡さない。古いファイルには無く、0 で読める） */
	WORD		awQuestID[MAX_TALKLINE];			/* 受けている依頼の条件（0=条件なし。古いファイルには無く、0 で読める） */
	BYTE		abyQuestStepMin[MAX_TALKLINE];
	BYTE		abyQuestStepMax[MAX_TALKLINE];		/* 段の上限+1 で保存する（0=どの段でも。古いファイルには無く、0 =どの段でも で読める） */
	WORD		awNeedItem[MAX_TALKLINE];			/* 持っている物の条件（0=条件なし。古いファイルには無く、0 で読める） */
} TALKFILE;

/* 台詞は行ごとに別のカラム（szSpeech0〜szSpeech15）。名前は 1 つずつ書く */
#define TALKCOL_SPEECH(n)	{ "szSpeech" #n, COL_STR, offsetof (TALKFILE, aszSpeech[n]), MAX_TALKMSG }

/* 会話データのカラム */
static const COLINFO s_aTalkCol[] = {
	COL (TALKFILE, wID,				COL_U16),
	COL (TALKFILE, byState,			COL_U8),
	COL (TALKFILE, byLineCount,		COL_U8),
	COLS(TALKFILE, szName,			COL_STR, MAX_TALKNAME),
	COLS(TALKFILE, abyRank,			COL_U8ARR, MAX_TALKLINE),
	COLS(TALKFILE, abyLvMin,		COL_U8ARR, MAX_TALKLINE),
	COLS(TALKFILE, abyLvMax,		COL_U8ARR, MAX_TALKLINE),
	COLS(TALKFILE, awNeed,			COL_U16ARR, MAX_TALKLINE * MAX_TALKNEED),
	COLS(TALKFILE, awNot,			COL_U16ARR, MAX_TALKLINE * MAX_TALKNEED),
	COLS(TALKFILE, abyCondNo,		COL_U8ARR, MAX_TALKLINE * MAX_TALKCOND),
	COLS(TALKFILE, abyCondCmp,		COL_U8ARR, MAX_TALKLINE * MAX_TALKCOND),
	COLS(TALKFILE, abyCondVal,		COL_U8ARR, MAX_TALKLINE * MAX_TALKCOND),
	COLS(TALKFILE, awSetOn,			COL_U16ARR, MAX_TALKLINE * MAX_TALKSET),
	COLS(TALKFILE, awSetOff,		COL_U16ARR, MAX_TALKLINE * MAX_TALKSET),
	COLS(TALKFILE, abyActNo,		COL_U8ARR, MAX_TALKLINE * MAX_TALKACT),
	COLS(TALKFILE, abyActOp,		COL_U8ARR, MAX_TALKLINE * MAX_TALKACT),
	COLS(TALKFILE, abyActVal,		COL_U8ARR, MAX_TALKLINE * MAX_TALKACT),
	TALKCOL_SPEECH (0),  TALKCOL_SPEECH (1),  TALKCOL_SPEECH (2),  TALKCOL_SPEECH (3),
	TALKCOL_SPEECH (4),  TALKCOL_SPEECH (5),  TALKCOL_SPEECH (6),  TALKCOL_SPEECH (7),
	TALKCOL_SPEECH (8),  TALKCOL_SPEECH (9),  TALKCOL_SPEECH (10), TALKCOL_SPEECH (11),
	TALKCOL_SPEECH (12), TALKCOL_SPEECH (13), TALKCOL_SPEECH (14), TALKCOL_SPEECH (15),
	COLS(TALKFILE, awGiveItem,		COL_U16ARR, MAX_TALKLINE),
	COLS(TALKFILE, awQuestID,		COL_U16ARR, MAX_TALKLINE),
	COLS(TALKFILE, abyQuestStepMin,	COL_U8ARR, MAX_TALKLINE),
	COLS(TALKFILE, abyQuestStepMax,	COL_U8ARR, MAX_TALKLINE),
	COLS(TALKFILE, awNeedItem,		COL_U16ARR, MAX_TALKLINE),
};

BOOL SaveFlagNameInfo(void);									/* フラグの名前表保存 */
BOOL LoadFlagNameInfo(void);									/* フラグの名前表読み込み */
BOOL SaveTalkInfo(void);										/* 会話データ保存 */
BOOL LoadTalkInfo(void);										/* 会話データ読み込み */

/* TALKINFO → ファイル用 */
static void TalkToFile(TALKFILE *p, const TALKINFO *t)
{
	int i, j;

	memset (p, 0, sizeof (*p));
	p->wID = t->wID;	p->byState = t->byState;	p->byLineCount = t->byLineCount;
	memcpy (p->szName, t->szName, sizeof (p->szName));
	for (i = 0; i < MAX_TALKLINE; i ++) {
		const TALKLINEINFO *l = &t->aLine[i];
		p->abyRank[i] = l->byRank;	p->abyLvMin[i] = l->byLvMin;	p->abyLvMax[i] = l->byLvMax;
		for (j = 0; j < MAX_TALKNEED; j ++) {
			p->awNeed[i * MAX_TALKNEED + j] = l->awNeed[j];
			p->awNot[i * MAX_TALKNEED + j] = l->awNot[j];
		}
		for (j = 0; j < MAX_TALKCOND; j ++) {
			p->abyCondNo[i * MAX_TALKCOND + j] = l->abyCondNo[j];
			p->abyCondCmp[i * MAX_TALKCOND + j] = l->abyCondCmp[j];
			p->abyCondVal[i * MAX_TALKCOND + j] = l->abyCondVal[j];
		}
		for (j = 0; j < MAX_TALKSET; j ++) {
			p->awSetOn[i * MAX_TALKSET + j] = l->awSetOn[j];
			p->awSetOff[i * MAX_TALKSET + j] = l->awSetOff[j];
		}
		for (j = 0; j < MAX_TALKACT; j ++) {
			p->abyActNo[i * MAX_TALKACT + j] = l->abyActNo[j];
			p->abyActOp[i * MAX_TALKACT + j] = l->abyActOp[j];
			p->abyActVal[i * MAX_TALKACT + j] = l->abyActVal[j];
		}
		memcpy (p->aszSpeech[i], l->szSpeech, MAX_TALKMSG);
		p->awGiveItem[i] = l->wGiveItem;	p->awQuestID[i] = l->wQuestID;
		p->abyQuestStepMin[i] = l->byQuestStepMin;	p->abyQuestStepMax[i] = (BYTE)(l->byQuestStepMax + 1);
		p->awNeedItem[i] = l->wNeedItem;
	}
}

/* ファイル用 → TALKINFO（文字列の終端は必ず付ける） */
static void TalkFromFile(TALKINFO *t, const TALKFILE *p)
{
	int i, j;

	memset (t, 0, sizeof (*t));
	t->wID = p->wID;	t->byState = p->byState;
	t->byLineCount = (p->byLineCount > MAX_TALKLINE) ? MAX_TALKLINE : p->byLineCount;
	memcpy (t->szName, p->szName, sizeof (t->szName));
	t->szName[MAX_TALKNAME - 1] = 0;
	for (i = 0; i < MAX_TALKLINE; i ++) {
		TALKLINEINFO *l = &t->aLine[i];
		l->byRank = p->abyRank[i];	l->byLvMin = p->abyLvMin[i];	l->byLvMax = p->abyLvMax[i];
		for (j = 0; j < MAX_TALKNEED; j ++) {
			l->awNeed[j] = p->awNeed[i * MAX_TALKNEED + j];
			l->awNot[j] = p->awNot[i * MAX_TALKNEED + j];
		}
		for (j = 0; j < MAX_TALKCOND; j ++) {
			l->abyCondNo[j] = p->abyCondNo[i * MAX_TALKCOND + j];
			l->abyCondCmp[j] = p->abyCondCmp[i * MAX_TALKCOND + j];
			l->abyCondVal[j] = p->abyCondVal[i * MAX_TALKCOND + j];
		}
		for (j = 0; j < MAX_TALKSET; j ++) {
			l->awSetOn[j] = p->awSetOn[i * MAX_TALKSET + j];
			l->awSetOff[j] = p->awSetOff[i * MAX_TALKSET + j];
		}
		for (j = 0; j < MAX_TALKACT; j ++) {
			l->abyActNo[j] = p->abyActNo[i * MAX_TALKACT + j];
			l->abyActOp[j] = p->abyActOp[i * MAX_TALKACT + j];
			l->abyActVal[j] = p->abyActVal[i * MAX_TALKACT + j];
		}
		memcpy (l->szSpeech, p->aszSpeech[i], MAX_TALKMSG);
		l->wGiveItem = p->awGiveItem[i];	l->wQuestID = p->awQuestID[i];
		l->byQuestStepMin = p->abyQuestStepMin[i];	l->byQuestStepMax = (BYTE)(p->abyQuestStepMax[i] - 1);
		l->wNeedItem = p->awNeedItem[i];
		l->szSpeech[MAX_TALKMSG - 1] = 0;
	}
}

/* ========================================================================= */
/* 関数名：SaveFlagNameInfo													 */
/* 内容：フラグの名前表保存													 */
/* ========================================================================= */
BOOL SaveFlagNameInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<FLAGNAMEFILE> aData;
	int i, nCount = g_aFlagNameInfo.GetSize ();

	MakeFileName (szFile, "SboDataFlagNameInfo.dat");
	/* 1 件も無く、ファイルも無い時は作らない（公開リポジトリにはデータを置かない） */
	if (nCount == 0) {
		HANDLE hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return TRUE;
		}
		CloseHandle (hFile);
	}
	aData.resize (nCount ? nCount : 1);
	for (i = 0; i < nCount; i ++) {
		memset (&aData[i], 0, sizeof (FLAGNAMEFILE));
		aData[i].byState = g_aFlagNameInfo[i]->byState;
		aData[i].byKind = g_aFlagNameInfo[i]->byKind;
		aData[i].wNo = g_aFlagNameInfo[i]->wNo;
		memcpy (aData[i].szName, g_aFlagNameInfo[i]->szName, sizeof (aData[i].szName));
	}
	return SaveColumns (szFile, s_aFlagNameCol, sizeof (s_aFlagNameCol) / sizeof (s_aFlagNameCol[0]), (const BYTE *)&aData[0], sizeof (FLAGNAMEFILE), nCount);
}

/* ========================================================================= */
/* 関数名：LoadFlagNameInfo													 */
/* 内容：フラグの名前表読み込み												 */
/* ========================================================================= */
BOOL LoadFlagNameInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataFlagNameInfo.dat");
	if (!LoadColumns (szFile, s_aFlagNameCol, sizeof (s_aFlagNameCol) / sizeof (s_aFlagNameCol[0]), sizeof (FLAGNAMEFILE), MAX_FLAGNAMEINFO, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		const FLAGNAMEFILE *p = (const FLAGNAMEFILE *)&aData[sizeof (FLAGNAMEFILE) * i];
		PFLAGNAMEINFO pInfo = new FLAGNAMEINFO;
		memset (pInfo, 0, sizeof (*pInfo));
		pInfo->byState = p->byState;
		pInfo->byKind = p->byKind;
		pInfo->wNo = p->wNo;
		memcpy (pInfo->szName, p->szName, sizeof (pInfo->szName));
		pInfo->szName[MAX_FLAGNAME - 1] = 0;
		g_aFlagNameInfo.Add (pInfo);
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SaveTalkInfo														 */
/* 内容：会話データ保存														 */
/* ========================================================================= */
BOOL SaveTalkInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<TALKFILE> aData;
	int i, nCount = g_aTalkInfo.GetSize ();

	MakeFileName (szFile, "SboDataTalkInfo.dat");
	/* 1 件も無く、ファイルも無い時は作らない（公開リポジトリにはデータを置かない） */
	if (nCount == 0) {
		HANDLE hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return TRUE;
		}
		CloseHandle (hFile);
	}
	aData.resize (nCount ? nCount : 1);
	for (i = 0; i < nCount; i ++) {
		TalkToFile (&aData[i], g_aTalkInfo[i]);
	}
	return SaveColumns (szFile, s_aTalkCol, sizeof (s_aTalkCol) / sizeof (s_aTalkCol[0]), (const BYTE *)&aData[0], sizeof (TALKFILE), nCount);
}

/* ========================================================================= */
/* 関数名：LoadTalkInfo														 */
/* 内容：会話データ読み込み													 */
/* ========================================================================= */
BOOL LoadTalkInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataTalkInfo.dat");
	if (!LoadColumns (szFile, s_aTalkCol, sizeof (s_aTalkCol) / sizeof (s_aTalkCol[0]), sizeof (TALKFILE), MAX_TALK, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		PTALKINFO pInfo = new TALKINFO;
		TalkFromFile (pInfo, (const TALKFILE *)&aData[sizeof (TALKFILE) * i]);
		g_aTalkInfo.Add (pInfo);
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
