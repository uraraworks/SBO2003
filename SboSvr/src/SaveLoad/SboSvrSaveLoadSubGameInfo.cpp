/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadSubGameInfo.cpp								 */
/* 概要：		サブゲーム情報読み書き処理(実装ファイル)					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "../../../Common/Info/SubGame/InfoSubGameWAR.h"
#include "../Proc/SubGame/ProcSubGameWAR.h"
#include "../Proc/ProcSubGame.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define DATACOUNT			(1)										/* 保存データ数 */
#define DATACOUNT_DUNGEON	(15)									/* 保存データ数(ダンジョン) */
#define DATACOUNT_MAZE		(23)									/* 保存データ数(迷路) */
#define DATACOUNT_BOMB		(9)										/* 保存データ数(爆弾ゲーム) */
#define DATACOUNT_WAR		(22)									/* 保存データ数(戦場ゲーム) */

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
	"byType",			/* 種別 */
	"pInfo",			/* 詳細情報 */
	"z"
};

/* ラベル名テーブル(ダンジョン) */
static const char *s_aszNameDUNGEON[] = {
	"dwMapID",			/* マップID */
	"byX1",				/* エリア情報（開始X） */
	"byY1",				/* エリア情報（開始Y） */
	"byX2",				/* エリア情報（終了X） */
	"byY2",				/* エリア情報（終了Y） */
	"byCharX",			/* 管理人キャラ座標(X) */
	"byCharY",			/* 管理人キャラ座標(Y) */
	"byStartX",			/* スタート地点(X) */
	"byStartY",			/* スタート地点(Y) */
	"byGoalX",			/* ゴール地点(移動元X) */
	"byGoalY",			/* ゴール地点(移動元Y) */
	"wChipIDStart",		/* 開始地点へのマップチップID */
	"wChipIDGoal",		/* ゴール地点のマップチップID */
	"wChipIDRoad",		/* 道のマップチップID */
	"wChipIDBlock",		/* 壁のマップチップID */
	"z"
};

/* ラベル名テーブル(迷路) */
static const char *s_aszNameMAZE[] = {
	"dwMapID",			/* マップID */
	"byX1",				/* エリア情報（開始X） */
	"byY1",				/* エリア情報（開始Y） */
	"byX2",				/* エリア情報（終了X） */
	"byY2",				/* エリア情報（終了Y） */
	"byCharX",			/* 管理人キャラ座標(X) */
	"byCharY",			/* 管理人キャラ座標(Y) */
	"byStartX",			/* スタート地点(X) */
	"byStartY",			/* スタート地点(Y) */
	"byGoalFromX",		/* ゴール地点(移動元X) */
	"byGoalFromY",		/* ゴール地点(移動元Y) */
	"byGoalToX",		/* ゴール地点(移動先X) */
	"byGoalToY",		/* ゴール地点(移動先Y) */
	"wChipIDStart",		/* 開始地点へのマップチップID */
	"wChipIDGoal",		/* ゴール地点のマップチップID */
	"wChipIDRoad",		/* 道のマップチップID */
	"wChipIDBlock",		/* 壁のマップチップID */
	"wChipIDRoad2",		/* 道に混ぜるマップチップID */
	"byRoad2Rate",		/* 道に混ぜる割合(%) */
	"wChipIDBlock2",		/* 壁に混ぜるマップチップID */
	"byBlock2Rate",		/* 壁に混ぜる割合(%) */
	"wChipIDBlockFace",		/* 壁の手前の面のマップチップID */
	"byPathWidth",		/* 通路の太さ */
	"z"
};

/* ラベル名テーブル(爆弾ゲーム) */
static const char *s_aszNameBOMB[] = {
	"dwMapID",			/* マップID */
	"dwX",				/* 参加受付X座標 */
	"dwY",				/* 参加受付Y座標 */
	"dwStartX",			/* 参加開始X座標 */
	"dwStartY",			/* 参加開始Y座標 */
	"byX1",				/* エリア情報（開始X） */
	"byY1",				/* エリア情報（開始Y） */
	"byX2",				/* エリア情報（終了X） */
	"byY2",				/* エリア情報（終了Y） */
	"z"
};

/* ラベル名テーブル(戦場ゲーム) */
static const char *s_aszNameWAR[] = {
	"dwMapID",			/* マップID */
	"byMember",			/* １チームの参加人数 */
	"byX",				/* 参加受付X座標 */
	"byY",				/* 参加受付Y座標 */
	"byX1",				/* エリア情報（開始X） */
	"byY1",				/* エリア情報（開始Y） */
	"byX2",				/* エリア情報（終了X） */
	"byY2",				/* エリア情報（終了Y） */
	"byHP1X",			/* チーム１のHP(X) */
	"byHP2X",			/* チーム２のHP(X) */
	"byHPY",			/* チームのHP(Y) */
	"byJudgeX",			/* 審判キャラX座標 */
	"byJudgeY",			/* 審判キャラY座標 */
	"byRes1X",			/* チーム１の蘇生カウンタX座標 */
	"byRes2X",			/* チーム２の蘇生カウンタX座標 */
	"byResY",			/* チームの蘇生カウンタY座標 */
	"byStart1X",		/* チーム１の開始X座標 */
	"byStart2X",		/* チーム２の開始X座標 */
	"byStartY",			/* チームの開始Y座標 */
	"byCastle1X",		/* チーム１の城X座標 */
	"byCastle2X",		/* チーム２の城X座標 */
	"byCastleY",		/* チームの城Y座標 */
	"z"
};

/* ラベル名テーブル(すごろく) */
static const char *s_aszNameSUGOROKU[] = {
	"dwMapID",				/* マップID */
	"byX",					/* 参加受付X座標 */
	"byY",					/* 参加受付Y座標 */
	"byRule",				/* ルール */
	"byMaxCount",			/* 最大参加人数 */
	"Event_Count",			/* イベント数 */
	"Event_byX",			/* X座標 */
	"Event_byY",			/* Y座標 */
	"Event_byEventType",	/* イベント種別 */
	"Event_byPara1",		/* パラメータ */
	"Parts_Count",			/* パーツ数 */
	"Parts_byType",			/* 種別 */
	"Parts_wPartsID",		/* パーツID */
	"z"
};

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveSubGameInfo(void);										/* サブゲーム情報保存 */
BOOL LoadSubGameInfo(void);										/* サブゲーム情報読み込み */

static PBYTE GetInfoDungeon(PSUBGAME_DUNGEONINFO, PDWORD);		/* 保存用サブゲーム情報取得(ダンジョン) */
static PBYTE GetInfoMaze(PSUBGAME_MAZEINFO, PDWORD);			/* 保存用サブゲーム情報取得(迷路) */
static PBYTE GetInfoBomb(PSUBGAME_BOMBINFO, PDWORD);			/* 保存用サブゲーム情報取得(爆弾ゲーム) */
static PBYTE GetInfoWar(PSUBGAME_WARINFO, PDWORD);				/* 保存用サブゲーム情報取得(戦場ゲーム) */
static PBYTE GetInfoSugoroku(PSUBGAME_SUGOROKUINFO, PDWORD);	/* 保存用サブゲーム情報取得(すごろく) */
static PBYTE PetInfoDungeon(PBYTE, PDWORD);						/* サブゲーム情報作成(ダンジョン) */
static PBYTE PetInfoMaze(PBYTE, PDWORD);						/* サブゲーム情報作成(迷路) */
static PBYTE PetInfoWar(PBYTE, PDWORD);							/* サブゲーム情報作成(戦場ゲーム) */
static PBYTE PetInfoBomb(PBYTE, PDWORD);						/* サブゲーム情報作成(爆弾ゲーム) */
static PBYTE PetInfoSugoroku(PBYTE, PDWORD);					/* サブゲーム情報作成(すごろく) */


/* ========================================================================= */
/* 関数名：SaveSubGameInfo													 */
/* 内容：サブゲーム情報保存													 */
/* 日付：2004/05/01															 */
/* ========================================================================= */
BOOL SaveSubGameInfo(void)
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
	strcat (szFileName, "\\SboDataSubGameInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = DATACOUNT;
	dwDataCount = (DWORD)g_aSubaGameInfo.GetSize ();


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
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp, dwTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;
		for (j = 0; j < dwDataCount; j ++) {
			PSUBGAMEINFO pInfo;
			PBYTE pTmp;

			pInfo = g_aSubaGameInfo[j];
			switch (i) {
			case 0:		dwDataSizeTmp += sizeof (BYTE);			break;	/* 種別 */
			case 1:		/* 詳細情報 */
				dwTmp = 0;
				pTmp = NULL;
				switch (pInfo->byType) {
				case SUBGAMEID_BOMB:			/* 爆弾ゲーム */
					pTmp = GetInfoBomb ((PSUBGAME_BOMBINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_MAZE:			/* 迷路 */
					pTmp = GetInfoMaze ((PSUBGAME_MAZEINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_DUNGEON:			/* ダンジョン */
					pTmp = GetInfoDungeon ((PSUBGAME_DUNGEONINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_WAR:				/* 戦場ゲーム */
					pTmp = GetInfoWar ((PSUBGAME_WARINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_SUGOROKU:		/* すごろく */
					pTmp = GetInfoSugoroku ((PSUBGAME_SUGOROKUINFO)pInfo->pInfo, &dwTmp);
					break;
				}
				SAFE_DELETE_ARRAY (pTmp);
				dwDataSizeTmp += dwTmp;
				break;
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
			PSUBGAMEINFO pInfo;
			PBYTE pTmp;
			DWORD dwTmp;

			pInfo = g_aSubaGameInfo[j];
			switch (i) {
			case 0:		/* 種別 */
				{
					/* 詳細が無い枠は「無し(0)」で書く（詳細 0 バイトのまま種別だけ残すと、	*/
					/* 読み込みで次の枠の詳細を読んでしまいずれる）							*/
					BYTE byType = (pInfo->pInfo == NULL) ? (BYTE)SUBGAMEID_NONE : pInfo->byType;
					CopyMemory (&pData[dwOffset], &byType, sizeof (BYTE));
					dwOffset += sizeof (BYTE);
				}
				break;
			case 1:		/* 詳細情報 */
				pTmp = NULL;
				switch (pInfo->byType) {
				case SUBGAMEID_BOMB:			/* 爆弾ゲーム */
					pTmp = GetInfoBomb ((PSUBGAME_BOMBINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_MAZE:			/* 迷路 */
					pTmp = GetInfoMaze ((PSUBGAME_MAZEINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_DUNGEON:			/* ダンジョン */
					pTmp = GetInfoDungeon ((PSUBGAME_DUNGEONINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_WAR:				/* 戦場ゲーム */
					pTmp = GetInfoWar ((PSUBGAME_WARINFO)pInfo->pInfo, &dwTmp);
					break;
				case SUBGAMEID_SUGOROKU:		/* すごろく */
					pTmp = GetInfoSugoroku ((PSUBGAME_SUGOROKUINFO)pInfo->pInfo, &dwTmp);
					break;
				}
				if (pTmp) {
					CopyMemory (&pData[dwOffset], pTmp, dwTmp);
					dwOffset += dwTmp;
				}
				SAFE_DELETE_ARRAY (pTmp);
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
/* 関数名：LoadSubGameInfo													 */
/* 内容：サブゲーム情報読み込み												 */
/* 日付：2004/05/01															 */
/* ========================================================================= */
BOOL LoadSubGameInfo(void)
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
	strcat (szFileName, "SBODATA\\SboDataSubGameInfo.dat");


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
		PSUBGAMEINFO pInfo;

		pInfo = new SUBGAMEINFO;
		ZeroMemory (pInfo, sizeof (SUBGAMEINFO));
		g_aSubaGameInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PSUBGAMEINFO pInfo;

			if (dwOffsetTmp >= dwSize) {
				break;
			}

			pInfo = g_aSubaGameInfo[j];
			if (strcmp (aHeaderInfo[i].szName, "byType") == 0) {						/* 種別 */
				pInfo->byType = pData[dwOffsetTmp];
				dwOffsetTmp += sizeof (BYTE);

			} else if (strcmp (aHeaderInfo[i].szName, "pInfo") == 0) {					/* 詳細情報 */
				PBYTE pTmp;
				DWORD dwTmp;

				dwTmp = 0;		/* 種別が「無し」なら詳細は 0 バイト */
				pTmp = NULL;
				switch (pInfo->byType) {
				case SUBGAMEID_BOMB:			/* 爆弾ゲーム */
					pTmp = PetInfoBomb (&pData[dwOffsetTmp], &dwTmp);
					break;
				case SUBGAMEID_MAZE:			/* 迷路 */
					pTmp = PetInfoMaze (&pData[dwOffsetTmp], &dwTmp);
					break;
				case SUBGAMEID_DUNGEON:			/* ダンジョン */
					pTmp = PetInfoDungeon (&pData[dwOffsetTmp], &dwTmp);
					break;
				case SUBGAMEID_WAR:				/* 戦場ゲーム */
					pTmp = PetInfoWar (&pData[dwOffsetTmp], &dwTmp);
					break;
				case SUBGAMEID_SUGOROKU:		/* すごろく */
					pTmp = PetInfoSugoroku (&pData[dwOffsetTmp], &dwTmp);
					break;
				}

				pInfo->pInfo = pTmp;
				dwOffsetTmp += dwTmp;
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

/* ========================================================================= */
/* 関数名：GetInfoDungeon													 */
/* 内容：保存用サブゲーム情報取得(ダンジョン)								 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE GetInfoDungeon(PSUBGAME_DUNGEONINFO pInfo, PDWORD pdwSize)
{
	PBYTE pData;
	DWORD i, dwCount, dwColumnCount, dwHeaderSize,
			dwDataSize, dwOffset;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;


	/*************************
		データの初期設定
	*************************/

	pData = NULL;
	*pdwSize = 0;
	if (pInfo == NULL) {
		return NULL;
	}


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
		HEADERINFO Info;

		if (s_aszNameDUNGEON[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameDUNGEON[i]);

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
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;

		switch (i) {
		case 0:		dwDataSizeTmp = sizeof (DWORD);			break;	/* マップID */
		case 1:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始X） */
		case 2:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始Y） */
		case 3:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了X） */
		case 4:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了Y） */
		case 5:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 管理人キャラ座標(X) */
		case 6:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 管理人キャラ座標(Y) */
		case 7:		dwDataSizeTmp = sizeof (BYTE);			break;	/* スタート地点(X) */
		case 8:		dwDataSizeTmp = sizeof (BYTE);			break;	/* スタート地点(Y) */
		case 9:		dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動元X) */
		case 10:	dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動元Y) */
		case 11:	dwDataSizeTmp = sizeof (WORD);			break;	/* 開始地点へのマップチップID */
		case 12:	dwDataSizeTmp = sizeof (WORD);			break;	/* ゴール地点のマップチップID */
		case 13:	dwDataSizeTmp = sizeof (WORD);			break;	/* 道のマップチップID */
		case 14:	dwDataSizeTmp = sizeof (WORD);			break;	/* 壁のマップチップID */
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	*pdwSize = dwHeaderSize + dwDataSize;
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfoTmp;
		DWORD dwTmp;

		pInfoTmp = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfoTmp->szName);
		dwOffset += (strlen (pInfoTmp->szName) + 1);

		dwTmp = dwHeaderSize + pInfoTmp->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		switch (i) {
		case 0:		/* マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* エリア情報（開始X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* エリア情報（開始Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 3:		/* エリア情報（終了X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 4:		/* エリア情報（終了Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 5:		/* 管理人キャラ座標(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byCharX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 6:		/* 管理人キャラ座標(Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byCharY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 7:		/* スタート地点(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byStartX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 8:		/* スタート地点(Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byStartY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 9:		/* ゴール地点(移動元X) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 10:	/* ゴール地点(移動元Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 11:	/* 開始地点へのマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDStart, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 12:	/* ゴール地点のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDGoal, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 13:	/* 道のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDRoad, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 14:	/* 壁のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDBlock, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}

/* ========================================================================= */
/* 関数名：GetInfoMaze														 */
/* 内容：保存用サブゲーム情報取得(迷路)										 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE GetInfoMaze(PSUBGAME_MAZEINFO pInfo, PDWORD pdwSize)
{
	PBYTE pData;
	DWORD i, dwCount, dwColumnCount, dwHeaderSize,
			dwDataSize, dwOffset;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;


	/*************************
		データの初期設定
	*************************/

	pData = NULL;
	*pdwSize = 0;
	if (pInfo == NULL) {
		return NULL;
	}


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
		HEADERINFO Info;

		if (s_aszNameMAZE[i][0] == 'z') {
			break;
		}
		/* 足した項目（17番目から）は使っている時だけ書く（前の版のサーバーでも読めるように） */
		if ((i >= 17) && !(pInfo->wChipIDRoad2 || pInfo->byRoad2Rate || pInfo->wChipIDBlock2 || pInfo->byBlock2Rate || pInfo->wChipIDBlockFace || pInfo->byPathWidth)) {
			break;
		}
		strcpy (Info.szName, s_aszNameMAZE[i]);

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
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;

		switch (i) {
		case 0:		dwDataSizeTmp = sizeof (DWORD);			break;	/* マップID */
		case 1:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始X） */
		case 2:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始Y） */
		case 3:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了X） */
		case 4:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了Y） */
		case 5:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 管理人キャラ座標(X) */
		case 6:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 管理人キャラ座標(Y) */
		case 7:		dwDataSizeTmp = sizeof (BYTE);			break;	/* スタート地点(X) */
		case 8:		dwDataSizeTmp = sizeof (BYTE);			break;	/* スタート地点(Y) */
		case 9:		dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動元X) */
		case 10:	dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動元Y) */
		case 11:	dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動先X) */
		case 12:	dwDataSizeTmp = sizeof (BYTE);			break;	/* ゴール地点(移動先Y) */
		case 13:	dwDataSizeTmp = sizeof (WORD);			break;	/* 開始地点へのマップチップID */
		case 14:	dwDataSizeTmp = sizeof (WORD);			break;	/* ゴール地点のマップチップID */
		case 15:	dwDataSizeTmp = sizeof (WORD);			break;	/* 道のマップチップID */
		case 16:	dwDataSizeTmp = sizeof (WORD);			break;	/* 壁のマップチップID */
		case 17:	dwDataSizeTmp = sizeof (WORD);			break;	/* 道に混ぜるマップチップID */
		case 18:	dwDataSizeTmp = sizeof (BYTE);			break;	/* 道に混ぜる割合(%) */
		case 19:	dwDataSizeTmp = sizeof (WORD);			break;	/* 壁に混ぜるマップチップID */
		case 20:	dwDataSizeTmp = sizeof (BYTE);			break;	/* 壁に混ぜる割合(%) */
		case 21:	dwDataSizeTmp = sizeof (WORD);			break;	/* 壁の手前の面のマップチップID */
		case 22:	dwDataSizeTmp = sizeof (BYTE);			break;	/* 通路の太さ */
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	*pdwSize = dwHeaderSize + dwDataSize;
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfoTmp;
		DWORD dwTmp;

		pInfoTmp = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfoTmp->szName);
		dwOffset += (strlen (pInfoTmp->szName) + 1);

		dwTmp = dwHeaderSize + pInfoTmp->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		switch (i) {
		case 0:		/* マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* エリア情報（開始X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* エリア情報（開始Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 3:		/* エリア情報（終了X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 4:		/* エリア情報（終了Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 5:		/* 管理人キャラ座標(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byCharX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 6:		/* 管理人キャラ座標(Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byCharY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 7:		/* スタート地点(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byStartX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 8:		/* スタート地点(Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byStartY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 9:		/* ゴール地点(移動元X) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalFromX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 10:	/* ゴール地点(移動元Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalFromY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 11:	/* ゴール地点(移動先X) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalToX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 12:	/* ゴール地点(移動先Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byGoalToY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 13:	/* 開始地点へのマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDStart, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 14:	/* ゴール地点のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDGoal, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 15:	/* 道のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDRoad, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 16:	/* 壁のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDBlock, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 17:	/* 道に混ぜるマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDRoad2, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 18:	/* 道に混ぜる割合(%) */
			CopyMemory (&pData[dwOffset], &pInfo->byRoad2Rate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 19:	/* 壁に混ぜるマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDBlock2, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 20:	/* 壁に混ぜる割合(%) */
			CopyMemory (&pData[dwOffset], &pInfo->byBlock2Rate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 21:	/* 壁の手前の面のマップチップID */
			CopyMemory (&pData[dwOffset], &pInfo->wChipIDBlockFace, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 22:	/* 通路の太さ */
			CopyMemory (&pData[dwOffset], &pInfo->byPathWidth, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}

/* ========================================================================= */
/* 関数名：GetInfoBomb														 */
/* 内容：保存用サブゲーム情報取得(爆弾ゲーム)								 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE GetInfoBomb(PSUBGAME_BOMBINFO pInfo, PDWORD pdwSize)
{
	PBYTE pData;
	DWORD i, dwCount, dwColumnCount, dwHeaderSize,
			dwDataSize, dwOffset;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;


	/*************************
		データの初期設定
	*************************/

	pData = NULL;
	*pdwSize = 0;
	if (pInfo == NULL) {
		return NULL;
	}


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
		HEADERINFO Info;

		if (s_aszNameBOMB[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameBOMB[i]);

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
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;

		switch (i) {
		case 0:		dwDataSizeTmp = sizeof (DWORD);			break;	/* マップID */
		case 1:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 参加受付X座標 */
		case 2:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 参加受付Y座標 */
		case 3:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 参加開始X座標 */
		case 4:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 参加開始Y座標 */
		case 5:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始X） */
		case 6:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始Y） */
		case 7:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了X） */
		case 8:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了Y） */
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	*pdwSize = dwHeaderSize + dwDataSize;
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfoTmp;
		DWORD dwTmp;

		pInfoTmp = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfoTmp->szName);
		dwOffset += (strlen (pInfoTmp->szName) + 1);

		dwTmp = dwHeaderSize + pInfoTmp->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		switch (i) {
		case 0:		/* マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* 参加受付X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwX, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 2:		/* 参加受付Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwY, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 3:		/* 参加開始X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwStartX, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 4:		/* 参加開始Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwStartY, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 5:		/* エリア情報（開始X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 6:		/* エリア情報（開始Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 7:		/* エリア情報（終了X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 8:		/* エリア情報（終了Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}

/* ========================================================================= */
/* 関数名：GetInfoWar														 */
/* 内容：保存用サブゲーム情報取得(戦場ゲーム)								 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE GetInfoWar(PSUBGAME_WARINFO pInfo, PDWORD pdwSize)
{
	PBYTE pData;
	DWORD i, dwCount, dwColumnCount, dwHeaderSize,
			dwDataSize, dwOffset;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;


	/*************************
		データの初期設定
	*************************/

	pData = NULL;
	*pdwSize = 0;
	if (pInfo == NULL) {
		return NULL;
	}


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
		HEADERINFO Info;

		if (s_aszNameWAR[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameWAR[i]);

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
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;

		switch (i) {
		case 0:		dwDataSizeTmp = sizeof (DWORD);			break;	/* マップID */
		case 1:		dwDataSizeTmp = sizeof (BYTE);			break;	/* １チームの参加人数 */
		case 2:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 参加受付X座標 */
		case 3:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 参加受付Y座標 */
		case 4:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始X） */
		case 5:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（開始Y） */
		case 6:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了X） */
		case 7:		dwDataSizeTmp = sizeof (BYTE);			break;	/* エリア情報（終了Y） */
		case 8:		dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム１のHP(X) */
		case 9:		dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム２のHP(X) */
		case 10:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チームのHP(Y) */
		case 11:	dwDataSizeTmp = sizeof (BYTE);			break;	/* 審判キャラX座標 */
		case 12:	dwDataSizeTmp = sizeof (BYTE);			break;	/* 審判キャラY座標 */
		case 13:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム１の蘇生カウンタX座標 */
		case 14:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム２の蘇生カウンタX座標 */
		case 15:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チームの蘇生カウンタY座標 */
		case 16:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム１の開始X座標 */
		case 17:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム２の開始X座標 */
		case 18:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チームの開始Y座標 */
		case 19:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム１の城X座標 */
		case 20:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チーム２の城X座標 */
		case 21:	dwDataSizeTmp = sizeof (BYTE);			break;	/* チームの城Y座標 */
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	*pdwSize = dwHeaderSize + dwDataSize;
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfoTmp;
		DWORD dwTmp;

		pInfoTmp = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfoTmp->szName);
		dwOffset += (strlen (pInfoTmp->szName) + 1);

		dwTmp = dwHeaderSize + pInfoTmp->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		switch (i) {
		case 0:		/* マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* １チームの参加人数 */
			CopyMemory (&pData[dwOffset], &pInfo->byMember, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 参加受付X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 3:		/* 参加受付Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 4:		/* エリア情報（開始X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 5:		/* エリア情報（開始Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 6:		/* エリア情報（終了X） */
			CopyMemory (&pData[dwOffset], &pInfo->byX2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 7:		/* エリア情報（終了Y） */
			CopyMemory (&pData[dwOffset], &pInfo->byY2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 8:		/* チーム１のHP(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byHP1X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 9:		/* チーム２のHP(X) */
			CopyMemory (&pData[dwOffset], &pInfo->byHP2X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 10:	/* チームのHP(Y) */
			CopyMemory (&pData[dwOffset], &pInfo->byHPY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 11:	/* 審判キャラX座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byJudgeX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 12:	/* 審判キャラY座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byJudgeY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 13:	/* チーム１の蘇生カウンタX座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byRes1X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 14:	/* チーム２の蘇生カウンタX座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byRes2X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 15:	/* チームの蘇生カウンタY座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byResY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 16:	/* チーム１の開始X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byStart1X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 17:	/* チーム２の開始X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byStart2X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 18:	/* チームの開始Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byStartY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 19:	/* チーム１の城X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byCastle1X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 20:	/* チーム２の城X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byCastle2X, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 21:	/* チームの城Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byCastleY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}

/* ========================================================================= */
/* 関数名：GetInfoSugoroku													 */
/* 内容：保存用サブゲーム情報取得(すごろく)									 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE GetInfoSugoroku(PSUBGAME_SUGOROKUINFO pInfo, PDWORD pdwSize)
{
	PBYTE pData;
	DWORD i, j, dwCount, dwColumnCount, dwHeaderSize,
			dwDataSize, dwOffset, dwEventCount, dwPartsCount;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;


	/*************************
		データの初期設定
	*************************/

	pData = NULL;
	*pdwSize = 0;
	if (pInfo == NULL) {
		return NULL;
	}


	/***********************
		ヘッダ情報の作成
	***********************/

	dwHeaderSize = sizeof (DWORD) * 2;
	for (i = 0; ; i ++) {
		HEADERINFO Info;

		if (s_aszNameSUGOROKU[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameSUGOROKU[i]);

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
	dwEventCount = pInfo->aEventInfo.GetSize ();
	dwPartsCount = pInfo->aPartsInfo.GetSize ();
	dwDataSize = 0;

	/* データサイズを計算 */
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwDataSizeTmp;

		if (i > 0) {
			aHeaderInfo[i].dwOffset = dwDataSize;
		}
		dwDataSizeTmp = 0;

		switch (i) {
		case 0:		dwDataSizeTmp = sizeof (DWORD);						break;	/* マップID */
		case 1:		dwDataSizeTmp = sizeof (BYTE);						break;	/* 参加受付X座標 */
		case 2:		dwDataSizeTmp = sizeof (BYTE);						break;	/* 参加受付Y座標 */
		case 3:		dwDataSizeTmp = sizeof (BYTE);						break;	/* ルール */
		case 4:		dwDataSizeTmp = sizeof (BYTE);						break;	/* 最大参加人数 */
		case 5:		dwDataSizeTmp = sizeof (DWORD);						break;	/* イベント数 */
		case 6:		dwDataSizeTmp = (sizeof (BYTE) * dwEventCount);		break;	/* X座標 */
		case 7:		dwDataSizeTmp = (sizeof (BYTE) * dwEventCount);		break;	/* Y座標 */
		case 8:		dwDataSizeTmp = (sizeof (BYTE) * dwEventCount);		break;	/* イベント種別 */
		case 9:		dwDataSizeTmp = (sizeof (BYTE) * dwEventCount);		break;	/* パラメータ */
		case 10:	dwDataSizeTmp = sizeof (DWORD);						break;	/* パーツ数 */
		case 11:	dwDataSizeTmp = (sizeof (BYTE) * dwPartsCount);		break;	/* 種別 */
		case 12:	dwDataSizeTmp = (sizeof (WORD) * dwPartsCount);		break;	/* パーツID */
		}
		aHeaderInfo[i].dwDataSize = dwDataSizeTmp;
		dwDataSize += dwDataSizeTmp;
	}


	/*************************
		保存用データを作成
	*************************/

	pData = new BYTE[dwHeaderSize + dwDataSize];
	*pdwSize = dwHeaderSize + dwDataSize;
	ZeroMemory (pData, dwHeaderSize + dwDataSize);
	dwOffset = 0;

	/* カラム数を書き込み */
	CopyMemory (&pData[dwOffset], &dwColumnCount, sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	/* ヘッダ部分を書き込み */
	dwCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwCount; i ++) {
		LPHEADERINFO pInfoTmp;
		DWORD dwTmp;

		pInfoTmp = &aHeaderInfo[i];

		strcpy ((LPSTR)&pData[dwOffset], pInfoTmp->szName);
		dwOffset += (strlen (pInfoTmp->szName) + 1);

		dwTmp = dwHeaderSize + pInfoTmp->dwOffset;
		CopyMemory (&pData[dwOffset], &dwTmp, sizeof (dwTmp));
		dwOffset += sizeof (dwTmp);
	}

	/* データ部分を作成 */
	for (i = 0; i < dwColumnCount; i ++) {
		switch (i) {
		case 0:		/* マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* 参加受付X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byX, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 参加受付Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->byY, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 3:		/* ルール */
			CopyMemory (&pData[dwOffset], &pInfo->byRule, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 4:		/* 最大参加人数 */
			CopyMemory (&pData[dwOffset], &pInfo->byMaxCount, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 5:		/* イベント数 */
			CopyMemory (&pData[dwOffset], &dwEventCount, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 6:		/* X座標 */
			for (j = 0; j < dwEventCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aEventInfo[j].byX, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
			}
			break;
		case 7:		/* Y座標 */
			for (j = 0; j < dwEventCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aEventInfo[j].byY, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
			}
			break;
		case 8:		/* イベント種別 */
			for (j = 0; j < dwEventCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aEventInfo[j].byEventType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
			}
			break;
		case 9:		/* パラメータ */
			for (j = 0; j < dwEventCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aEventInfo[j].byPara1, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
			}
			break;
		case 10:	/* イベント数 */
			CopyMemory (&pData[dwOffset], &dwPartsCount, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 11:	/* 種別 */
			for (j = 0; j < dwPartsCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aPartsInfo[j].byType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
			}
			break;
		case 12:	/* パーツID */
			for (j = 0; j < dwPartsCount; j ++) {
				CopyMemory (&pData[dwOffset], &pInfo->aPartsInfo[j].wPartsID, sizeof (WORD));
				dwOffset += sizeof (WORD);
			}
			break;
		}
	}

	return pData;
}

/* ========================================================================= */
/* 関数名：PetInfoDungeon													 */
/* 内容：サブゲーム情報作成(ダンジョン)										 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE PetInfoDungeon(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PSUBGAME_DUNGEONINFO pInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	dwOffset = 0;

	/*************************
		ヘッダ情報の読み込み
	*************************/

	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	dwOffsetData = dwOffset;

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		Info.dwOffset -= dwOffsetData;
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0; ; j ++) {
			if (s_aszNameDUNGEON[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameDUNGEON[j]) == 0) {
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

	pInfo = new SUBGAME_DUNGEONINFO;
	ZeroMemory (pInfo, sizeof (SUBGAME_DUNGEONINFO));
	pInfo->dwTblID = -1;

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {						/* マップID */
			CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byX1") == 0) {					/* エリア情報（開始X） */
			CopyMemory (&pInfo->byX1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY1") == 0) {					/* エリア情報（開始Y） */
			CopyMemory (&pInfo->byY1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX2") == 0) {					/* エリア情報（終了X） */
			CopyMemory (&pInfo->byX2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY2") == 0) {					/* エリア情報（終了Y） */
			CopyMemory (&pInfo->byY2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCharX") == 0) {				/* 管理人キャラ座標(X) */
			CopyMemory (&pInfo->byCharX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCharY") == 0) {				/* 管理人キャラ座標(Y) */
			CopyMemory (&pInfo->byCharY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStartX") == 0) {				/* スタート地点(X) */
			CopyMemory (&pInfo->byStartX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStartY") == 0) {				/* スタート地点(Y) */
			CopyMemory (&pInfo->byStartY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalX") == 0) {				/* ゴール地点(移動先X) */
			CopyMemory (&pInfo->byGoalX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalY") == 0) {				/* ゴール地点(移動先Y) */
			CopyMemory (&pInfo->byGoalY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDStart") == 0) {			/* 開始地点へのマップチップID */
			CopyMemory (&pInfo->wChipIDStart, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDGoal") == 0) {			/* ゴール地点のマップチップID */
			CopyMemory (&pInfo->wChipIDGoal, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDRoad") == 0) {			/* 道のマップチップID */
			CopyMemory (&pInfo->wChipIDRoad, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDBlock") == 0) {			/* 壁のマップチップID */
			CopyMemory (&pInfo->wChipIDBlock, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}

/* ========================================================================= */
/* 関数名：PetInfoMaze														 */
/* 内容：サブゲーム情報作成(迷路)											 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE PetInfoMaze(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PSUBGAME_MAZEINFO pInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	dwOffset = 0;

	/*************************
		ヘッダ情報の読み込み
	*************************/

	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	dwOffsetData = dwOffset;

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		Info.dwOffset -= dwOffsetData;
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0; ; j ++) {
			if (s_aszNameMAZE[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameMAZE[j]) == 0) {
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

	pInfo = new SUBGAME_MAZEINFO;
	ZeroMemory (pInfo, sizeof (SUBGAME_MAZEINFO));
	pInfo->dwTblID = -1;

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {						/* マップID */
			CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byX1") == 0) {					/* エリア情報（開始X） */
			CopyMemory (&pInfo->byX1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY1") == 0) {					/* エリア情報（開始Y） */
			CopyMemory (&pInfo->byY1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX2") == 0) {					/* エリア情報（終了X） */
			CopyMemory (&pInfo->byX2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY2") == 0) {					/* エリア情報（終了Y） */
			CopyMemory (&pInfo->byY2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCharX") == 0) {				/* 管理人キャラ座標(X) */
			CopyMemory (&pInfo->byCharX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCharY") == 0) {				/* 管理人キャラ座標(Y) */
			CopyMemory (&pInfo->byCharY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStartX") == 0) {				/* スタート地点(X) */
			CopyMemory (&pInfo->byStartX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStartY") == 0) {				/* スタート地点(Y) */
			CopyMemory (&pInfo->byStartY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalFromX") == 0) {			/* ゴール地点(移動元X) */
			CopyMemory (&pInfo->byGoalFromX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalFromY") == 0) {			/* ゴール地点(移動元Y) */
			CopyMemory (&pInfo->byGoalFromY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalToX") == 0) {				/* ゴール地点(移動先X) */
			CopyMemory (&pInfo->byGoalToX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byGoalToY") == 0) {				/* ゴール地点(移動先Y) */
			CopyMemory (&pInfo->byGoalToY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDStart") == 0) {			/* 開始地点へのマップチップID */
			CopyMemory (&pInfo->wChipIDStart, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDGoal") == 0) {			/* ゴール地点のマップチップID */
			CopyMemory (&pInfo->wChipIDGoal, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDRoad") == 0) {			/* 道のマップチップID */
			CopyMemory (&pInfo->wChipIDRoad, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDBlock") == 0) {			/* 壁のマップチップID */
			CopyMemory (&pInfo->wChipIDBlock, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDRoad2") == 0) {		/* 道に混ぜるマップチップID */
			CopyMemory (&pInfo->wChipIDRoad2, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byRoad2Rate") == 0) {		/* 道に混ぜる割合(%) */
			CopyMemory (&pInfo->byRoad2Rate, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDBlock2") == 0) {		/* 壁に混ぜるマップチップID */
			CopyMemory (&pInfo->wChipIDBlock2, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byBlock2Rate") == 0) {		/* 壁に混ぜる割合(%) */
			CopyMemory (&pInfo->byBlock2Rate, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "wChipIDBlockFace") == 0) {		/* 壁の手前の面のマップチップID */
			CopyMemory (&pInfo->wChipIDBlockFace, &pData[dwOffsetTmp], sizeof (WORD));
			dwOffsetTmp += sizeof (WORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byPathWidth") == 0) {		/* 通路の太さ */
			CopyMemory (&pInfo->byPathWidth, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}

/* ========================================================================= */
/* 関数名：PetInfoWar														 */
/* 内容：サブゲーム情報作成(戦場ゲーム)										 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE PetInfoWar(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PSUBGAME_WARINFO pInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PCInfoSubGameWAR pInfoSubGameWAR;
	PCProcSubGameWAR pProcSubGameWAR;

	dwOffset = 0;

	/*************************
		ヘッダ情報の読み込み
	*************************/

	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	dwOffsetData = dwOffset;

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		Info.dwOffset -= dwOffsetData;
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0; ; j ++) {
			if (s_aszNameWAR[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameWAR[j]) == 0) {
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

	pInfo = new SUBGAME_WARINFO;
	ZeroMemory (pInfo, sizeof (SUBGAME_WARINFO));


	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {						/* マップID */
			CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byMember") == 0) {				/* １チームの参加人数 */
			CopyMemory (&pInfo->byMember, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX") == 0) {					/* 参加受付X座標 */
			CopyMemory (&pInfo->byX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY") == 0) {					/* 参加受付Y座標 */
			CopyMemory (&pInfo->byY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX1") == 0) {					/* エリア情報（開始X） */
			CopyMemory (&pInfo->byX1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY1") == 0) {					/* エリア情報（開始Y） */
			CopyMemory (&pInfo->byY1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX2") == 0) {					/* エリア情報（終了X） */
			CopyMemory (&pInfo->byX2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY2") == 0) {					/* エリア情報（終了Y） */
			CopyMemory (&pInfo->byY2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byHP1X") == 0) {					/* チーム１のHP(X) */
			CopyMemory (&pInfo->byHP1X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byHP2X") == 0) {					/* チーム２のHP(X) */
			CopyMemory (&pInfo->byHP2X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byHPY") == 0) {					/* チームのHP(Y) */
			CopyMemory (&pInfo->byHPY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byJudgeX") == 0) {				/* 審判キャラX座標 */
			CopyMemory (&pInfo->byJudgeX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byJudgeY") == 0) {				/* 審判キャラY座標 */
			CopyMemory (&pInfo->byJudgeY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byRes1X") == 0) {				/* チーム１の蘇生カウンタX座標 */
			CopyMemory (&pInfo->byRes1X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byRes2X") == 0) {				/* チーム２の蘇生カウンタX座標 */
			CopyMemory (&pInfo->byRes2X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byResY") == 0) {					/* チームの蘇生カウンタY座標 */
			CopyMemory (&pInfo->byResY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStart1X") == 0) {				/* チーム１の開始X座標 */
			CopyMemory (&pInfo->byStart1X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStart2X") == 0) {				/* チーム２の開始X座標 */
			CopyMemory (&pInfo->byStart2X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byStartY") == 0) {				/* チームの開始Y座標 */
			CopyMemory (&pInfo->byStartY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCastle1X") == 0) {				/* チーム１の城X座標 */
			CopyMemory (&pInfo->byCastle1X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCastle2X") == 0) {				/* チーム２の城X座標 */
			CopyMemory (&pInfo->byCastle2X, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byCastleY") == 0) {				/* チームの城Y座標 */
			CopyMemory (&pInfo->byCastleY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		}
		dwOffset = dwOffsetTmp;
	}


	/* ゲーム情報 */
	pInfoSubGameWAR = new CInfoSubGameWAR;
	pInfoSubGameWAR->m_dwMapID			= pInfo->dwMapID;	/* マップID */
	pInfoSubGameWAR->m_byMember			= pInfo->byMember;	/* １チームの参加人数 */
	pInfoSubGameWAR->m_ptInformation.x	= pInfo->byX;		/* 受付座標(横) */
	pInfoSubGameWAR->m_ptInformation.y	= pInfo->byY;		/* 受付座標(縦) */
	pInfoSubGameWAR->m_ptJudge.x		= pInfo->byJudgeX;	/* 審判キャラ座標(横) */
	pInfoSubGameWAR->m_ptJudge.y		= pInfo->byJudgeY;	/* 審判キャラ座標(縦) */
	SetRect (&pInfoSubGameWAR->m_rcArea,					/* ゲームエリア */
			pInfo->byX1,
			pInfo->byY1,
			pInfo->byX2,
			pInfo->byY2);

	/* チーム情報1 */
	pTeamInfo = pInfoSubGameWAR->GetNewTeamInfo ();
	pTeamInfo->ptHP.x			= pInfo->byHP1X;			/* HP座標(横) */
	pTeamInfo->ptHP.y			= pInfo->byHPY;				/* HP座標(縦) */
	pTeamInfo->ptResurrection.x	= pInfo->byRes1X;			/* 蘇生カウンタ座標(横) */
	pTeamInfo->ptResurrection.y	= pInfo->byResY;			/* 蘇生カウンタ座標(縦) */
	pTeamInfo->ptStart.x		= pInfo->byStart1X;			/* 開始座標(横) */
	pTeamInfo->ptStart.y		= pInfo->byStartY;			/* 開始座標(縦) */
	pTeamInfo->ptCastle.x		= pInfo->byCastle1X;		/* 城座標(横) */
	pTeamInfo->ptCastle.y		= pInfo->byCastleY;			/* 城座標(縦) */
	pInfoSubGameWAR->AddTeam (pTeamInfo);

	/* チーム情報2 */
	pTeamInfo = pInfoSubGameWAR->GetNewTeamInfo ();
	pTeamInfo->ptHP.x			= pInfo->byHP2X;			/* HP座標(横) */
	pTeamInfo->ptHP.y			= pInfo->byHPY;				/* HP座標(縦) */
	pTeamInfo->ptResurrection.x	= pInfo->byRes2X;			/* 蘇生カウンタ座標(横) */
	pTeamInfo->ptResurrection.y	= pInfo->byResY;			/* 蘇生カウンタ座標(縦) */
	pTeamInfo->ptStart.x		= pInfo->byStart2X;			/* 開始座標(横) */
	pTeamInfo->ptStart.y		= pInfo->byStartY;			/* 開始座標(縦) */
	pTeamInfo->ptCastle.x		= pInfo->byCastle2X;		/* 城座標(横) */
	pTeamInfo->ptCastle.y		= pInfo->byCastleY;			/* 城座標(縦) */
	pInfoSubGameWAR->AddTeam (pTeamInfo);

	pProcSubGameWAR = new CProcSubGameWAR;
	pProcSubGameWAR->Init (g_ProcSubGame.GetNewID (), pInfoSubGameWAR);
	g_ProcSubGame.Add (pProcSubGameWAR);

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}

/* ========================================================================= */
/* 関数名：PetInfoBomb														 */
/* 内容：サブゲーム情報作成(爆弾ゲーム)										 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE PetInfoBomb(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PSUBGAME_BOMBINFO pInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	dwOffset = 0;

	/*************************
		ヘッダ情報の読み込み
	*************************/

	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	dwOffsetData = dwOffset;

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		Info.dwOffset -= dwOffsetData;
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0; ; j ++) {
			if (s_aszNameBOMB[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameBOMB[j]) == 0) {
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

	pInfo = new SUBGAME_BOMBINFO;
	ZeroMemory (pInfo, sizeof (SUBGAME_BOMBINFO));
	pInfo->dwTblIDGameMaster = -1;

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {						/* マップID */
			CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "dwX") == 0) {					/* 参加受付X座標 */
			CopyMemory (&pInfo->dwX, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "dwY") == 0) {					/* 参加受付Y座標 */
			CopyMemory (&pInfo->dwY, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "dwStartX") == 0) {				/* 参加開始X座標 */
			CopyMemory (&pInfo->dwStartX, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "dwStartY") == 0) {				/* 参加開始Y座標 */
			CopyMemory (&pInfo->dwStartY, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byX1") == 0) {					/* エリア情報（開始X） */
			CopyMemory (&pInfo->byX1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY1") == 0) {					/* エリア情報（開始Y） */
			CopyMemory (&pInfo->byY1, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byX2") == 0) {					/* エリア情報（終了X） */
			CopyMemory (&pInfo->byX2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY2") == 0) {					/* エリア情報（終了Y） */
			CopyMemory (&pInfo->byY2, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}

/* ========================================================================= */
/* 関数名：PetInfoSugoroku													 */
/* 内容：サブゲーム情報作成(すごろく)										 */
/* 日付：2004/12/31															 */
/* ========================================================================= */
static PBYTE PetInfoSugoroku(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount, dwCount;
	PSUBGAME_SUGOROKUINFO pInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;

	dwOffset = 0;

	/*************************
		ヘッダ情報の読み込み
	*************************/

	/* カラム数を読み込み */
	CopyMemory (&dwColumnCount, &pData[dwOffset], sizeof (dwColumnCount));
	dwOffset += sizeof (dwColumnCount);
	dwOffsetData = dwOffset;

	for (i = 0; i < dwColumnCount; i ++) {
		HEADERINFO Info;
		BOOL bTmp;

		strcpy (Info.szName, (LPSTR)&pData[dwOffset]);
		dwOffset += (strlen (Info.szName) + 1);

		CopyMemory (&Info.dwOffset, &pData[dwOffset], sizeof (Info.dwOffset));
		Info.dwOffset -= dwOffsetData;
		dwOffset += sizeof (Info.dwOffset);

		bTmp = FALSE;
		for (j = 0; ; j ++) {
			if (s_aszNameSUGOROKU[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameSUGOROKU[j]) == 0) {
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

	pInfo = new SUBGAME_SUGOROKUINFO;
	ZeroMemory (pInfo, sizeof (SUBGAME_SUGOROKUINFO));


	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {						/* マップID */
			CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);
		} else if (strcmp (aHeaderInfo[i].szName, "byX") == 0) {					/* 参加受付X座標 */
			CopyMemory (&pInfo->byX, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byY") == 0) {					/* 参加受付Y座標 */
			CopyMemory (&pInfo->byY, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byRule") == 0) {					/* ルール */
			CopyMemory (&pInfo->byRule, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "byMaxCount") == 0) {				/* 最大参加人数 */
			CopyMemory (&pInfo->byMaxCount, &pData[dwOffsetTmp], sizeof (BYTE));
			dwOffsetTmp += sizeof (BYTE);
		} else if (strcmp (aHeaderInfo[i].szName, "Event_Count") == 0) {			/* イベント数 */
			DWORD dwTmp;
			SUGOROKUEVENTINFO Info;

			CopyMemory (&dwTmp, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);

			ZeroMemory (&Info, sizeof (Info));
			for (j = 0; j < dwTmp; j ++) {
				pInfo->aEventInfo.Add (Info);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Event_byX") == 0) {				/* X座標 */
			dwCount = pInfo->aEventInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aEventInfo[j].byX, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Event_byY") == 0) {				/* Y座標 */
			dwCount = pInfo->aEventInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aEventInfo[j].byY, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Event_byEventType") == 0) {		/* イベント種別 */
			dwCount = pInfo->aEventInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aEventInfo[j].byEventType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Event_byPara1") == 0) {		/* パラメータ */
			dwCount = pInfo->aEventInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aEventInfo[j].byPara1, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Parts_Count") == 0) {			/* パーツ数 */
			DWORD dwTmp;
			SUGOROKUPARTSINFO Info;

			CopyMemory (&dwTmp, &pData[dwOffsetTmp], sizeof (DWORD));
			dwOffsetTmp += sizeof (DWORD);

			ZeroMemory (&Info, sizeof (Info));
			for (j = 0; j < dwTmp; j ++) {
				pInfo->aPartsInfo.Add (Info);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Parts_byType") == 0) {		/* 種別 */
			dwCount = pInfo->aPartsInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aPartsInfo[j].byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
		} else if (strcmp (aHeaderInfo[i].szName, "Parts_wPartsID") == 0) {		/* パーツID */
			dwCount = pInfo->aPartsInfo.GetSize ();
			for (j = 0; j < dwCount; j ++) {
				CopyMemory (&pInfo->aPartsInfo[j].wPartsID, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}

/* Copyright(C)URARA-works 2003-2005 */
