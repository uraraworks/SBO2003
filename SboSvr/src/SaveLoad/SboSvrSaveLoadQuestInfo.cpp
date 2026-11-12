/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadQuestInfo.cpp									 */
/* 概要：		依頼情報読み書き処理(実装ファイル)							 */
/* 作成日：		2026/11/10													 */
/* ========================================================================= */
/*
 * 保存先は SBODATA/SboDataQuestInfo.dat。作り手・レシピと同じ「カラム単位」の形式。
 * 見えるNPC（8 件）は 1 件ぶんが大きいので、項目ごとに 8 件まとめた配列のカラム
 * （adwNpcMapID・abyNpcX …）にして保存する（カラム数の上限 64 に収めるため）。
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

/* ファイルに書く 1 件ぶん（見えるNPC は項目ごとの配列にしてある） */
typedef struct _QUESTFILE {
	WORD		wID;
	BYTE		byState;
	BYTE		byGuild;
	WORD		wSort;
	BYTE		byOnce;
	char		szName[MAX_QUESTNAME];
	char		szExplain[MAX_QUESTMSG];
	char		szMsgAccept[MAX_QUESTMSG];
	char		szMsgDone[MAX_QUESTMSG];
	DWORD		dwGold;
	DWORD		dwExp;
	WORD		wRewardItem;
	WORD		wQuestItem;
	BYTE		byStepCount;
	char		aszGoal[MAX_QUESTSTEP * MAX_QUESTGOAL];
	DWORD		adwNpcMapID[MAX_QUESTNPC];
	BYTE		abyNpcX[MAX_QUESTNPC];
	BYTE		abyNpcY[MAX_QUESTNPC];
	BYTE		abyNpcDir[MAX_QUESTNPC];
	BYTE		abyNpcKind[MAX_QUESTNPC];
	WORD		awNpcGrpID[MAX_QUESTNPC];
	char		aszNpcName[MAX_QUESTNPC * MAX_CHARNAMEBUF];
	char		aszNpcSpeech[MAX_QUESTNPC * MAX_QUESTMSG];
	BYTE		abyNpcStepMin[MAX_QUESTNPC];
	BYTE		abyNpcStepMax[MAX_QUESTNPC];
	BYTE		abyNpcFlgNeed[MAX_QUESTNPC];
	BYTE		abyNpcFlgHide[MAX_QUESTNPC];
	DWORD		adwNpcDestMapID[MAX_QUESTNPC];
	BYTE		abyNpcDestX[MAX_QUESTNPC];
	BYTE		abyNpcDestY[MAX_QUESTNPC];
	WORD		awNpcItem[MAX_QUESTNPC];
	BYTE		abyNpcSetFlg[MAX_QUESTNPC];
	BYTE		abyNpcNextStep[MAX_QUESTNPC];
	WORD		awNpcStTmp[MAX_QUESTNPC];
	BYTE		byChallenge;
	WORD		wLoanStTmp;
	BYTE		byMemberMin;
} QUESTFILE;

/* 依頼情報のカラム */
static const COLINFO s_aQuestCol[] = {
	COL (QUESTFILE, wID,			COL_U16),
	COL (QUESTFILE, byState,		COL_U8),
	COL (QUESTFILE, byGuild,		COL_U8),
	COL (QUESTFILE, wSort,			COL_U16),
	COL (QUESTFILE, byOnce,			COL_U8),
	COLS(QUESTFILE, szName,			COL_STR, MAX_QUESTNAME),
	COLS(QUESTFILE, szExplain,		COL_STR, MAX_QUESTMSG),
	COLS(QUESTFILE, szMsgAccept,	COL_STR, MAX_QUESTMSG),
	COLS(QUESTFILE, szMsgDone,		COL_STR, MAX_QUESTMSG),
	COL (QUESTFILE, dwGold,			COL_U32),
	COL (QUESTFILE, dwExp,			COL_U32),
	COL (QUESTFILE, wRewardItem,	COL_U16),
	COL (QUESTFILE, wQuestItem,		COL_U16),
	COL (QUESTFILE, byStepCount,	COL_U8),
	COLS(QUESTFILE, aszGoal,		COL_U8ARR, MAX_QUESTSTEP * MAX_QUESTGOAL),
	COLS(QUESTFILE, adwNpcMapID,	COL_U32ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcX,		COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcY,		COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcDir,		COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcKind,		COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, awNpcGrpID,		COL_U16ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, aszNpcName,		COL_U8ARR, MAX_QUESTNPC * MAX_CHARNAMEBUF),
	COLS(QUESTFILE, aszNpcSpeech,	COL_U8ARR, MAX_QUESTNPC * MAX_QUESTMSG),
	COLS(QUESTFILE, abyNpcStepMin,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcStepMax,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcFlgNeed,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcFlgHide,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, adwNpcDestMapID,COL_U32ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcDestX,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcDestY,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, awNpcItem,		COL_U16ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcSetFlg,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, abyNpcNextStep,	COL_U8ARR, MAX_QUESTNPC),
	COLS(QUESTFILE, awNpcStTmp,		COL_U16ARR, MAX_QUESTNPC),
	COL (QUESTFILE, byChallenge,	COL_U8),
	COL (QUESTFILE, wLoanStTmp,		COL_U16),
	COL (QUESTFILE, byMemberMin,	COL_U8),
};

BOOL SaveQuestInfo(void);										/* 依頼情報保存 */
BOOL LoadQuestInfo(void);										/* 依頼情報読み込み */

/* QUESTINFO → ファイル用 */
static void ToFile(QUESTFILE *p, const QUESTINFO *q)
{
	int i;
	memset (p, 0, sizeof (*p));
	p->wID = q->wID;	p->byState = q->byState;	p->byGuild = q->byGuild;
	p->wSort = q->wSort;	p->byOnce = q->byOnce;
	memcpy (p->szName, q->szName, sizeof (p->szName));
	memcpy (p->szExplain, q->szExplain, sizeof (p->szExplain));
	memcpy (p->szMsgAccept, q->szMsgAccept, sizeof (p->szMsgAccept));
	memcpy (p->szMsgDone, q->szMsgDone, sizeof (p->szMsgDone));
	p->dwGold = q->dwGold;	p->dwExp = q->dwExp;
	p->wRewardItem = q->wRewardItem;	p->wQuestItem = q->wQuestItem;
	p->byStepCount = q->byStepCount;
	memcpy (p->aszGoal, q->aszGoal, sizeof (p->aszGoal));
	p->byChallenge = q->byChallenge;	p->wLoanStTmp = q->wLoanStTmp;	p->byMemberMin = q->byMemberMin;
	for (i = 0; i < MAX_QUESTNPC; i ++) {
		const QUESTNPCINFO *n = &q->aNpc[i];
		p->adwNpcMapID[i] = n->dwMapID;	p->abyNpcX[i] = n->byX;	p->abyNpcY[i] = n->byY;
		p->abyNpcDir[i] = n->byDir;	p->abyNpcKind[i] = n->byKind;	p->awNpcGrpID[i] = n->wGrpID;
		memcpy (&p->aszNpcName[i * MAX_CHARNAMEBUF], n->szName, MAX_CHARNAMEBUF);
		memcpy (&p->aszNpcSpeech[i * MAX_QUESTMSG], n->szSpeech, MAX_QUESTMSG);
		p->abyNpcStepMin[i] = n->byStepMin;	p->abyNpcStepMax[i] = n->byStepMax;
		p->abyNpcFlgNeed[i] = n->byFlgNeed;	p->abyNpcFlgHide[i] = n->byFlgHide;
		p->adwNpcDestMapID[i] = n->dwDestMapID;	p->abyNpcDestX[i] = n->byDestX;	p->abyNpcDestY[i] = n->byDestY;
		p->awNpcItem[i] = n->wItem;	p->abyNpcSetFlg[i] = n->bySetFlg;	p->abyNpcNextStep[i] = n->byNextStep;
		p->awNpcStTmp[i] = n->wStTmp;
	}
}

/* ファイル用 → QUESTINFO（文字列の終端は必ず付ける） */
static void FromFile(QUESTINFO *q, const QUESTFILE *p)
{
	int i;
	memset (q, 0, sizeof (*q));
	q->wID = p->wID;	q->byState = p->byState;	q->byGuild = p->byGuild;
	q->wSort = p->wSort;	q->byOnce = p->byOnce;
	memcpy (q->szName, p->szName, sizeof (q->szName));
	memcpy (q->szExplain, p->szExplain, sizeof (q->szExplain));
	memcpy (q->szMsgAccept, p->szMsgAccept, sizeof (q->szMsgAccept));
	memcpy (q->szMsgDone, p->szMsgDone, sizeof (q->szMsgDone));
	q->dwGold = p->dwGold;	q->dwExp = p->dwExp;
	q->wRewardItem = p->wRewardItem;	q->wQuestItem = p->wQuestItem;
	q->byStepCount = p->byStepCount;
	memcpy (q->aszGoal, p->aszGoal, sizeof (q->aszGoal));
	q->byChallenge = p->byChallenge;	q->wLoanStTmp = p->wLoanStTmp;	q->byMemberMin = p->byMemberMin;
	q->szName[MAX_QUESTNAME - 1] = 0;
	q->szExplain[MAX_QUESTMSG - 1] = 0;
	q->szMsgAccept[MAX_QUESTMSG - 1] = 0;
	q->szMsgDone[MAX_QUESTMSG - 1] = 0;
	for (i = 0; i < MAX_QUESTSTEP; i ++) {
		q->aszGoal[i][MAX_QUESTGOAL - 1] = 0;
	}
	for (i = 0; i < MAX_QUESTNPC; i ++) {
		QUESTNPCINFO *n = &q->aNpc[i];
		n->dwMapID = p->adwNpcMapID[i];	n->byX = p->abyNpcX[i];	n->byY = p->abyNpcY[i];
		n->byDir = p->abyNpcDir[i];	n->byKind = p->abyNpcKind[i];	n->wGrpID = p->awNpcGrpID[i];
		memcpy (n->szName, &p->aszNpcName[i * MAX_CHARNAMEBUF], MAX_CHARNAMEBUF);
		memcpy (n->szSpeech, &p->aszNpcSpeech[i * MAX_QUESTMSG], MAX_QUESTMSG);
		n->szName[MAX_CHARNAMEBUF - 1] = 0;
		n->szSpeech[MAX_QUESTMSG - 1] = 0;
		n->byStepMin = p->abyNpcStepMin[i];	n->byStepMax = p->abyNpcStepMax[i];
		n->byFlgNeed = p->abyNpcFlgNeed[i];	n->byFlgHide = p->abyNpcFlgHide[i];
		n->dwDestMapID = p->adwNpcDestMapID[i];	n->byDestX = p->abyNpcDestX[i];	n->byDestY = p->abyNpcDestY[i];
		n->wItem = p->awNpcItem[i];	n->bySetFlg = p->abyNpcSetFlg[i];	n->byNextStep = p->abyNpcNextStep[i];
		n->wStTmp = p->awNpcStTmp[i];
	}
}

/* ========================================================================= */
/* 関数名：SaveQuestInfo													 */
/* 内容：依頼情報保存														 */
/* ========================================================================= */
BOOL SaveQuestInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<QUESTFILE> aData;
	int i, nCount = g_aQuestInfo.GetSize ();

	MakeFileName (szFile, "SboDataQuestInfo.dat");
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
		ToFile (&aData[i], g_aQuestInfo[i]);
	}
	return SaveColumns (szFile, s_aQuestCol, sizeof (s_aQuestCol) / sizeof (s_aQuestCol[0]), (const BYTE *)&aData[0], sizeof (QUESTFILE), nCount);
}

/* ========================================================================= */
/* 関数名：LoadQuestInfo													 */
/* 内容：依頼情報読み込み													 */
/* ========================================================================= */
BOOL LoadQuestInfo(void)
{
	char szFile[MAX_PATH + 1];
	std::vector<BYTE> aData;
	int i, nCount = 0;

	MakeFileName (szFile, "SboDataQuestInfo.dat");
	if (!LoadColumns (szFile, s_aQuestCol, sizeof (s_aQuestCol) / sizeof (s_aQuestCol[0]), sizeof (QUESTFILE), MAX_QUEST, aData, &nCount)) {
		return FALSE;
	}
	for (i = 0; i < nCount; i ++) {
		PQUESTINFO pInfo = new QUESTINFO;
		FromFile (pInfo, (const QUESTFILE *)&aData[sizeof (QUESTFILE) * i]);
		g_aQuestInfo.Add (pInfo);
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2005 */
