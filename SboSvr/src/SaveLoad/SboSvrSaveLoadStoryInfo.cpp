/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadStoryInfo.cpp									 */
/* 概要：		思い出の本のシナリオ情報読み書き処理(実装ファイル)			 */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */
/*
 * 保存先は SBODATA/SboDataStoryInfo.dat。依頼・作り手・レシピと同じ「カラム単位」の形式。
 * ファイルが無い時は 0 件で始める。知らないカラムは読み飛ばし、無いカラムは 0 のまま読める。
 * 印・数値の番号は「番号+1（0=無し）」で保存する（0 のまま読んだデータが「印 0」「数値 0」にならないように）。
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

/* ファイルに書く 1 件ぶん（印・数値の番号は「番号+1」。0=無し） */
typedef struct _STORYFILE {
	WORD		wID;
	BYTE		byState;
	WORD		wSort;
	char		szName[MAX_QUESTNAME];
	char		szExplain[MAX_QUESTMSG];
	WORD		wClearFlag;								/* クリアした印（番号+1。0=無し） */
	WORD		wPrevStory;								/* 前のお話（0=無し） */
	WORD		awFlagFrom[MAX_STORYFLAGRANGE];			/* 消す印の範囲の先頭（番号+1。0=無し） */
	WORD		awFlagTo[MAX_STORYFLAGRANGE];			/* 消す印の範囲の最後（番号+1。0=無し） */
	BYTE		abyNum[MAX_STORYNUM];					/* 0 に戻す数値の番号（番号+1。0=無し） */
	WORD		awQuest[MAX_STORYQUEST];				/* クリア済みの印を消す依頼の番号（0=無し） */
	BYTE		byCountNum;								/* やり直した回数を数える数値の番号（番号+1。0=数えない） */
	WORD		awTakeItem[MAX_STORYTAKEITEM];			/* やり直す時に取り上げるアイテム番号（0=無し。古いファイルには無く、全部 0 で読める） */
	/* 「いまの目的」（古いファイルには無く、目的なしで読める）。行ごとの項目は MAX_STORYGOAL 行まとめた配列のカラムにする */
	WORD		awGoalNeed[MAX_STORYGOAL * MAX_TALKNEED];	/* 立っている必要のある印（番号+1。0=無し） */
	WORD		awGoalNot[MAX_STORYGOAL * MAX_TALKNEED];	/* 立っていてはいけない印（番号+1。0=無し） */
	BYTE		abyGoalCondNo[MAX_STORYGOAL * MAX_TALKCOND];	/* 数値の条件の数値番号（番号+1。0=無し） */
	BYTE		abyGoalCondCmp[MAX_STORYGOAL * MAX_TALKCOND];	/* 比べ方 */
	BYTE		abyGoalCondVal[MAX_STORYGOAL * MAX_TALKCOND];	/* 比べる値 */
	char		aszGoal[MAX_STORYGOAL][MAX_QUESTGOAL];	/* 目的の文（カラムは行ごとに szGoal0〜） */
} STORYFILE;

/* シナリオ情報のカラム */
static const COLINFO s_aStoryCol[] = {
	COL (STORYFILE, wID,			COL_U16),
	COL (STORYFILE, byState,		COL_U8),
	COL (STORYFILE, wSort,			COL_U16),
	COLS(STORYFILE, szName,			COL_STR, MAX_QUESTNAME),
	COLS(STORYFILE, szExplain,		COL_STR, MAX_QUESTMSG),
	COL (STORYFILE, wClearFlag,		COL_U16),
	COL (STORYFILE, wPrevStory,		COL_U16),
	COLS(STORYFILE, awFlagFrom,		COL_U16ARR, MAX_STORYFLAGRANGE),
	COLS(STORYFILE, awFlagTo,		COL_U16ARR, MAX_STORYFLAGRANGE),
	COLS(STORYFILE, abyNum,			COL_U8ARR, MAX_STORYNUM),
	COLS(STORYFILE, awQuest,		COL_U16ARR, MAX_STORYQUEST),
	COL (STORYFILE, byCountNum,		COL_U8),
	COLS(STORYFILE, awTakeItem,		COL_U16ARR, MAX_STORYTAKEITEM),
	COLS(STORYFILE, awGoalNeed,		COL_U16ARR, MAX_STORYGOAL * MAX_TALKNEED),
	COLS(STORYFILE, awGoalNot,		COL_U16ARR, MAX_STORYGOAL * MAX_TALKNEED),
	COLS(STORYFILE, abyGoalCondNo,	COL_U8ARR, MAX_STORYGOAL * MAX_TALKCOND),
	COLS(STORYFILE, abyGoalCondCmp,	COL_U8ARR, MAX_STORYGOAL * MAX_TALKCOND),
	COLS(STORYFILE, abyGoalCondVal,	COL_U8ARR, MAX_STORYGOAL * MAX_TALKCOND),
	/* 目的の文は 1 行 1 カラム（8 行ぶん） */
	{ "szGoal0", COL_STR, offsetof (STORYFILE, aszGoal[0]), MAX_QUESTGOAL },
	{ "szGoal1", COL_STR, offsetof (STORYFILE, aszGoal[1]), MAX_QUESTGOAL },
	{ "szGoal2", COL_STR, offsetof (STORYFILE, aszGoal[2]), MAX_QUESTGOAL },
	{ "szGoal3", COL_STR, offsetof (STORYFILE, aszGoal[3]), MAX_QUESTGOAL },
	{ "szGoal4", COL_STR, offsetof (STORYFILE, aszGoal[4]), MAX_QUESTGOAL },
	{ "szGoal5", COL_STR, offsetof (STORYFILE, aszGoal[5]), MAX_QUESTGOAL },
	{ "szGoal6", COL_STR, offsetof (STORYFILE, aszGoal[6]), MAX_QUESTGOAL },
	{ "szGoal7", COL_STR, offsetof (STORYFILE, aszGoal[7]), MAX_QUESTGOAL },
};

BOOL SaveStoryInfo(void);										/* シナリオ情報保存 */
BOOL LoadStoryInfo(void);										/* シナリオ情報読み込み */

/* STORYINFO → ファイル用（番号は「番号+1」にする） */
static void ToFile(STORYFILE *p, const STORYINFO *q)
{
	int i;
	memset (p, 0, sizeof (*p));
	p->wID = q->wID;	p->byState = q->byState;	p->wSort = q->wSort;
	memcpy (p->szName, q->szName, sizeof (p->szName));
	memcpy (p->szExplain, q->szExplain, sizeof (p->szExplain));
	p->wClearFlag = (q->wClearFlag < MAX_FLAGBIT) ? (WORD)(q->wClearFlag + 1) : 0;
	p->wPrevStory = q->wPrevStory;
	for (i = 0; i < MAX_STORYFLAGRANGE; i ++) {
		p->awFlagFrom[i] = (q->awFlagFrom[i] < MAX_FLAGBIT) ? (WORD)(q->awFlagFrom[i] + 1) : 0;
		p->awFlagTo[i] = (q->awFlagTo[i] < MAX_FLAGBIT) ? (WORD)(q->awFlagTo[i] + 1) : 0;
	}
	for (i = 0; i < MAX_STORYNUM; i ++) {
		p->abyNum[i] = (q->abyNum[i] < MAX_FLAGNUM) ? (BYTE)(q->abyNum[i] + 1) : 0;
	}
	for (i = 0; i < MAX_STORYQUEST; i ++) {
		p->awQuest[i] = q->awQuest[i];
	}
	p->byCountNum = (q->byCountNum < MAX_FLAGNUM) ? (BYTE)(q->byCountNum + 1) : 0;
	for (i = 0; i < MAX_STORYTAKEITEM; i ++) {
		p->awTakeItem[i] = q->awTakeItem[i];
	}
	for (i = 0; i < MAX_STORYGOAL; i ++) {
		const STORYGOAL *g = &q->aGoal[i];
		int j;

		for (j = 0; j < MAX_TALKNEED; j ++) {
			p->awGoalNeed[i * MAX_TALKNEED + j] = (g->awNeed[j] < MAX_FLAGBIT) ? (WORD)(g->awNeed[j] + 1) : 0;
			p->awGoalNot[i * MAX_TALKNEED + j] = (g->awNot[j] < MAX_FLAGBIT) ? (WORD)(g->awNot[j] + 1) : 0;
		}
		for (j = 0; j < MAX_TALKCOND; j ++) {
			p->abyGoalCondNo[i * MAX_TALKCOND + j] = (g->abyCondNo[j] < MAX_FLAGNUM) ? (BYTE)(g->abyCondNo[j] + 1) : 0;
			p->abyGoalCondCmp[i * MAX_TALKCOND + j] = g->abyCondCmp[j];
			p->abyGoalCondVal[i * MAX_TALKCOND + j] = g->abyCondVal[j];
		}
		memcpy (p->aszGoal[i], g->szGoal, sizeof (p->aszGoal[i]));
	}
}

/* ファイル用 → STORYINFO（文字列の終端は必ず付ける。0・範囲外の番号は必ず「無し」にする） */
static void FromFile(STORYINFO *q, const STORYFILE *p)
{
	int i;
	memset (q, 0, sizeof (*q));
	q->wID = p->wID;	q->byState = p->byState;	q->wSort = p->wSort;
	memcpy (q->szName, p->szName, sizeof (q->szName));
	memcpy (q->szExplain, p->szExplain, sizeof (q->szExplain));
	q->szName[MAX_QUESTNAME - 1] = 0;
	q->szExplain[MAX_QUESTMSG - 1] = 0;
	q->wClearFlag = CharFlagFromWire (p->wClearFlag);
	q->wPrevStory = p->wPrevStory;
	for (i = 0; i < MAX_STORYFLAGRANGE; i ++) {
		q->awFlagFrom[i] = CharFlagFromWire (p->awFlagFrom[i]);
		q->awFlagTo[i] = CharFlagFromWire (p->awFlagTo[i]);
	}
	for (i = 0; i < MAX_STORYNUM; i ++) {
		q->abyNum[i] = ((p->abyNum[i] >= 1) && (p->abyNum[i] <= MAX_FLAGNUM)) ? (BYTE)(p->abyNum[i] - 1) : (BYTE)FLAG_NONUM;
	}
	for (i = 0; i < MAX_STORYQUEST; i ++) {
		q->awQuest[i] = p->awQuest[i];
	}
	q->byCountNum = ((p->byCountNum >= 1) && (p->byCountNum <= MAX_FLAGNUM)) ? (BYTE)(p->byCountNum - 1) : (BYTE)FLAG_NONUM;
	for (i = 0; i < MAX_STORYTAKEITEM; i ++) {
		q->awTakeItem[i] = p->awTakeItem[i];
	}
	/* 目的：古いファイルには無く 0 で読めるので、印・数値は「無し」、文は空になる */
	for (i = 0; i < MAX_STORYGOAL; i ++) {
		STORYGOAL *g = &q->aGoal[i];
		int j;

		for (j = 0; j < MAX_TALKNEED; j ++) {
			g->awNeed[j] = CharFlagFromWire (p->awGoalNeed[i * MAX_TALKNEED + j]);
			g->awNot[j] = CharFlagFromWire (p->awGoalNot[i * MAX_TALKNEED + j]);
		}
		for (j = 0; j < MAX_TALKCOND; j ++) {
			BYTE byNo = p->abyGoalCondNo[i * MAX_TALKCOND + j];

			g->abyCondNo[j] = ((byNo >= 1) && (byNo <= MAX_FLAGNUM)) ? (BYTE)(byNo - 1) : (BYTE)FLAG_NONUM;
			g->abyCondCmp[j] = (p->abyGoalCondCmp[i * MAX_TALKCOND + j] < TALKCMP_MAX) ? p->abyGoalCondCmp[i * MAX_TALKCOND + j] : (BYTE)TALKCMP_GE;
			g->abyCondVal[j] = p->abyGoalCondVal[i * MAX_TALKCOND + j];
		}
		memcpy (g->szGoal, p->aszGoal[i], sizeof (g->szGoal));
		g->szGoal[MAX_QUESTGOAL - 1] = 0;
	}
}

/* ========================================================================= */
/* 関数名：SaveStoryInfo													 */
/* 内容：シナリオ情報保存													 */
/* ========================================================================= */
BOOL SaveStoryInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<STORYFILE> aData;
	int i, nCount = g_aStoryInfo.GetSize ();

	MakeFileName (szFile, "SboDataStoryInfo.dat");
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
		ToFile (&aData[i], g_aStoryInfo[i]);
	}
	return SaveColumns (szFile, s_aStoryCol, sizeof (s_aStoryCol) / sizeof (s_aStoryCol[0]), (const BYTE *)&aData[0], sizeof (STORYFILE), nCount);
}

/* ========================================================================= */
/* 関数名：LoadStoryInfo													 */
/* 内容：シナリオ情報読み込み												 */
/* ========================================================================= */
BOOL LoadStoryInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataStoryInfo.dat");
	if (!LoadColumns (szFile, s_aStoryCol, sizeof (s_aStoryCol) / sizeof (s_aStoryCol[0]), sizeof (STORYFILE), MAX_STORY, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		PSTORYINFO pInfo = new STORYINFO;
		FromFile (pInfo, (const STORYFILE *)&aData[sizeof (STORYFILE) * i]);
		g_aStoryInfo.Add (pInfo);
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
