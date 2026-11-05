/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadItemInfo.cpp									 */
/* 概要：		アイテム情報読み書き処理(実装ファイル)						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* ========================================================================= */
#include "include.h"
#include "../../../Common/Info/Item/InfoItem.h"
#include "../../../Common/LibInfo/LibInfoItem.h"
#include "SimpleDB.h"
#include "Glbldata.h"

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
/* ラベル名テーブル */
static const char *s_aszName[] = {
	"szName",				/* アイテム名 */
	"byType",				/* 種別 */
	"byData",				/* 付加データ */
	"byFlg",				/* フラグ */
	"bySound",				/* 使った時に鳴らす音 */
	"bySexDistinction",		/* 性別による種別 */
	"byEraseProbability",	/* 無くなる確率 */
	"wGrpID",				/* グラフィックID */
	"dwGold",				/* 値段 */
	"pData",				/* 詳細情報 */
	"pMsg",					/* 使った時のメッセージ */
	"szNameV",				/* アイテム名（可変長。szName は古い版のために 31 バイトで残す） */
	"z"
};

/* ラベル名テーブル(特定位置へ移動) */
static const char *s_aszNameRURA[] = {
	"dwEnableMapID",	/* 使用限定するマップID(先頭ビットはフラグ) */
	"dwMapID",			/* 移動先マップID */
	"dwX",				/* 移動先X座標 */
	"dwY",				/* 移動先Y座標 */
	"z"
};

/* ラベル名テーブル(HP回復) */
static const char *s_aszNameHPUP[] = {
	"byRate",			/* 確立で回復？ */
	"wMin",				/* 最低回復値 */
	"wMax",				/* 最大回復値 */
	"z"
};

/* ラベル名テーブル(武器) */
static const char *s_aszNameWEAPON[] = {
	"byEfcID",			/* 攻撃時に発動するエフェクト番号 */
	"byMin",			/* 攻撃力減少幅 */
	"byAtackType",		/* 攻撃種別 */
	"byData1",			/* 付加データ1 */
	"byData2",			/* 付加データ2 */
	"byData3",			/* 付加データ3 */
	"byData4",			/* 付加データ4 */
	"byData5",			/* 付加データ5 */
	"wEqu",				/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(鎧) */
static const char *s_aszNameARMOR[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(盾) */
static const char *s_aszNameSHILD[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(兜) */
static const char *s_aszNameHELMET[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(種) */
static const char *s_aszNameSEED[] = {
	"byType",			/* 種類 */
	"wMax",				/* 上限値 */
	"z"
};

/* ラベル名テーブル(サブゲーム参加) */
static const char *s_aszNameSUBGAME[] = {
	"byType",			/* 種類 */
	"z"
};

/* ラベル名テーブル(酒) */
static const char *s_aszNameSAKE[] = {
	"chPoint",			/* 度数 */
	"z"
};

/* ラベル名テーブル(メッセージ表示) */
static const char *s_aszNameMSG[] = {
	"byType",			/* 表示種別 */
	"byEfectID",		/* エフェクト番号 */
	"szCharEfc",		/* 文字エフェクト */
	"pMsgTo",			/* 他のキャラに使った時のメッセージ */
	"pMsgMe",			/* 他のキャラに使われた時のメッセージ */
	"szCharEfcV",		/* 文字エフェクト（可変長。szCharEfc は古い版のために 15 バイトで残す） */
	"z"
};

/* ラベル名テーブル(キャラ画像変更) */
static const char *s_aszNameGRPCHG[] = {
	"wGrpID",			/* 変更するグラフィックID */
	"z"
};

/* ラベル名テーブル(明かり) */
static const char *s_aszNameLIGHT[] = {
	"byLevel",			/* 明るさレベル */
	"wTime",			/* 維持時間 */
	"z"
};

/* ラベル名テーブル(HP減少) */
static const char *s_aszNameHPDOWN[] = {
	"byEfectID",		/* エフェクト番号 */
	"byRate",			/* 確立で減少？ */
	"wMin",				/* 最低減少値 */
	"wMax",				/* 最大減少値 */
	"z"
};

/* ラベル名テーブル(魔法習得) */
static const char *s_aszNameMAGIC[] = {
	"byID",				/* 習得する魔法 */
	"z"
};

/* ラベル名テーブル(Mana回復) */
static const char *s_aszNameMANAUP[] = {
	"byRate",			/* 確立で回復？ */
	"wMin",				/* 最低減少値 */
	"wMax",				/* 最大減少値 */
	"z"
};

/* ラベル名テーブル(HP・Mana回復) */
static const char *s_aszNameHPMANAUP[] = {
	"byRateHP",			/* 確立で回復？(HP) */
	"byRateMana",		/* 確立で回復？(Mana) */
	"wMinHP",			/* 最低減少値(HP) */
	"wMaxHP",			/* 最大減少値(HP) */
	"wMinMana",			/* 最低減少値(Mana) */
	"wMaxMana",			/* 最大減少値(Mana) */
	"z"
};

/* ラベル名テーブル(小手) */
static const char *s_aszNameEQU_HAND[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(靴) */
static const char *s_aszNameEQU_LEG[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"z"
};

/* ラベル名テーブル(首飾り) */
static const char *s_aszNameEQU_ACC1[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"byEquType",	/* 装備効果 */
	"z"
};

/* ラベル名テーブル(指輪1) */
static const char *s_aszNameEQU_ACC2[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"byEquType",	/* 装備効果 */
	"z"
};

/* ラベル名テーブル(指輪2) */
static const char *s_aszNameEQU_ACC3[] = {
	"wEqu",			/* 装備できるキャラ情報 */
	"byEquType",	/* 装備効果 */
	"z"
};

/* ラベル名テーブル(蘇生) */
static const char *s_aszNameRESURRECTION[] = {
	"byRate",		/* 回復率 */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
BOOL SaveItemInfo(void);										/* アイテム情報保存 */
BOOL LoadItemInfo(void);										/* アイテム情報読み込み */
static PBYTE GetInfoRURA(PITEMINFO_RURA, PDWORD);				/* 保存用アイテム情報取得(特定位置へ移動) */
static PBYTE GetInfoHPUP(PITEMINFO_HPUP, PDWORD);				/* 保存用アイテム情報取得(HP回復) */
static PBYTE GetInfoWEAPON(PITEMINFO_WEAPON, PDWORD);			/* 保存用アイテム情報取得(武器) */
static PBYTE GetInfoARMOR(PITEMINFO_ARMOR, PDWORD);				/* 保存用アイテム情報取得(鎧) */
static PBYTE GetInfoSHILD(PITEMINFO_SHILD, PDWORD);				/* 保存用アイテム情報取得(盾) */
static PBYTE GetInfoHELMET(PITEMINFO_HELMET, PDWORD);			/* 保存用アイテム情報取得(兜) */
static PBYTE GetInfoSEED(PITEMINFO_SEED, PDWORD);				/* 保存用アイテム情報取得(種) */
static PBYTE GetInfoSUBGAME(PITEMINFO_SUBGAME, PDWORD);			/* 保存用アイテム情報取得(サブゲーム参加) */
static PBYTE GetInfoSAKE(PITEMINFO_SAKE, PDWORD);				/* 保存用アイテム情報取得(酒) */
static PBYTE GetInfoMSG(PITEMINFO_MSG, PDWORD);					/* 保存用アイテム情報取得(メッセージ表示) */
static PBYTE GetInfoGRPCHG(PITEMINFO_GRPCHG, PDWORD);			/* 保存用アイテム情報取得(キャラ画像変更) */
static PBYTE GetInfoLIGHT(PITEMINFO_LIGHT, PDWORD);				/* 保存用アイテム情報取得(明かり) */
static PBYTE GetInfoHPDOWN(PITEMINFO_HPDOWN, PDWORD);			/* 保存用アイテム情報取得(HP減少) */
static PBYTE GetInfoMAGIC(PITEMINFO_MAGIC, PDWORD);				/* 保存用アイテム情報取得(魔法習得) */
static PBYTE GetInfoMANAUP(PITEMINFO_MANAUP, PDWORD);			/* 保存用アイテム情報取得(Mana回復) */
static PBYTE GetInfoHPMANAUP(PITEMINFO_HPMANAUP, PDWORD);		/* 保存用アイテム情報取得(HP・Mana回復) */
static PBYTE GetInfoEQU_HAND(PITEMINFO_EQU_HAND, PDWORD);		/* 保存用アイテム情報取得(装備(小手)) */
static PBYTE GetInfoEQU_LEG(PITEMINFO_EQU_LEG, PDWORD);			/* 保存用アイテム情報取得(装備(靴)) */
static PBYTE GetInfoEQU_ACC1(PITEMINFO_EQU_ACC1, PDWORD);		/* 保存用アイテム情報取得(装備(首飾り)) */
static PBYTE GetInfoEQU_ACC2(PITEMINFO_EQU_ACC2, PDWORD);		/* 保存用アイテム情報取得(装備(指輪1)) */
static PBYTE GetInfoEQU_ACC3(PITEMINFO_EQU_ACC3, PDWORD);		/* 保存用アイテム情報取得(装備(指輪2)) */
static PBYTE GetInfoRESURRECTION(PITEMINFO_RESURRECTION, PDWORD);/* 保存用アイテム情報取得(蘇生) */
static PBYTE PutInfoRURA(PBYTE, PDWORD);						/* アイテム情報作成(特定位置へ移動) */
static PBYTE PutInfoHPUP(PBYTE, PDWORD);						/* アイテム情報作成(HP回復) */
static PBYTE PutInfoWEAPON(PBYTE, PDWORD);						/* アイテム情報作成(武器) */
static PBYTE PutInfoARMOR(PBYTE, PDWORD);						/* アイテム情報作成(鎧) */
static PBYTE PutInfoSHILD(PBYTE, PDWORD);						/* アイテム情報作成(盾) */
static PBYTE PutInfoHELMET(PBYTE, PDWORD);						/* アイテム情報作成(兜) */
static PBYTE PutInfoSEED(PBYTE, PDWORD);						/* アイテム情報作成(種) */
static PBYTE PutInfoSUBGAME(PBYTE, PDWORD);						/* アイテム情報作成(サブゲーム参加) */
static PBYTE PutInfoSAKE(PBYTE, PDWORD);						/* アイテム情報作成(酒) */
static PBYTE PutInfoMSG(PBYTE, PDWORD);							/* アイテム情報作成(メッセージ表示) */
static PBYTE PutInfoGRPCHG(PBYTE, PDWORD);						/* アイテム情報作成(キャラ画像変更) */
static PBYTE PutInfoLIGHT(PBYTE, PDWORD);						/* アイテム情報作成(明かり) */
static PBYTE PutInfoHPDOWN(PBYTE, PDWORD);						/* アイテム情報作成(HP減少) */
static PBYTE PutInfoMAGIC(PBYTE, PDWORD);						/* アイテム情報作成(魔法習得) */
static PBYTE PutInfoMANAUP(PBYTE, PDWORD);						/* アイテム情報作成(Mana回復) */
static PBYTE PutInfoHPMANAUP(PBYTE, PDWORD);					/* アイテム情報作成(HP・Mana回復) */
static PBYTE PutInfoEQU_HAND(PBYTE, PDWORD);					/* アイテム情報作成(装備(小手)) */
static PBYTE PutInfoEQU_LEG(PBYTE, PDWORD);						/* アイテム情報作成(装備(靴)) */
static PBYTE PutInfoEQU_ACC1(PBYTE, PDWORD);					/* アイテム情報作成(装備(首飾り)) */
static PBYTE PutInfoEQU_ACC2(PBYTE, PDWORD);					/* アイテム情報作成(装備(指輪1)) */
static PBYTE PutInfoEQU_ACC3(PBYTE, PDWORD);					/* アイテム情報作成(装備(指輪2)) */
static PBYTE PutInfoRESURRECTION(PBYTE, PDWORD);				/* アイテム情報作成(蘇生) */

void ConvItemTest(void);
void SaveItemTest(void);			/* アイテム情報保存 */
void LoadItemInfoTest(void);		/* アイテム情報読み込み */


/* ========================================================================= */
/* 関数名：SaveItemInfo														 */
/* 内容：アイテム情報保存													 */
/* 日付：2005/02/17															 */
/* ========================================================================= */
BOOL SaveItemInfo(void)
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
	strcat (szFileName, "\\SboDataItemInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = aHeaderInfo.GetSize ();
	dwDataCount = (DWORD)g_aItemInfo.GetSize ();


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
			PITEMINFO pInfo;
			PBYTE pTmp;

			pInfo = g_aItemInfo[j];
			switch (i) {
			case 0:		dwDataSizeTmp += OLD_MAX_ITEMNAME;			break;	/* アイテム名（古い版の幅） */
			case 1:		dwDataSizeTmp += sizeof (BYTE);				break;	/* 種別 */
			case 2:		dwDataSizeTmp += sizeof (BYTE);				break;	/* 付加データ */
			case 3:		dwDataSizeTmp += sizeof (BYTE);				break;	/* フラグ */
			case 4:		dwDataSizeTmp += sizeof (BYTE);				break;	/* 使った時に鳴らす音 */
			case 5:		dwDataSizeTmp += sizeof (BYTE);				break;	/* 性別による種別 */
			case 6:		dwDataSizeTmp += sizeof (BYTE);				break;	/* 無くなる確率 */
			case 7:		dwDataSizeTmp += sizeof (WORD);				break;	/* グラフィックID */
			case 8:		dwDataSizeTmp += sizeof (DWORD);			break;	/* 値段 */
			case 9:		/* 詳細情報 */
				dwTmp = 0;
				pTmp = NULL;
				switch (pInfo->byType) {
				case ITEMTYPE_RURA:				/* 特定位置へ移動 */
					pTmp = GetInfoRURA ((PITEMINFO_RURA)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPUP:				/* HP回復 */
					pTmp = GetInfoHPUP ((PITEMINFO_HPUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_WEAPON:			/* 武器 */
					pTmp = GetInfoWEAPON ((PITEMINFO_WEAPON)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_ARMOR:			/* 鎧 */
					pTmp = GetInfoARMOR ((PITEMINFO_ARMOR)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SHILD:			/* 盾 */
					pTmp = GetInfoSHILD ((PITEMINFO_SHILD)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HELMET:			/* 兜 */
					pTmp = GetInfoHELMET ((PITEMINFO_HELMET)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SEED:				/* 種 */
					pTmp = GetInfoSEED ((PITEMINFO_SEED)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SUBGAME:			/* サブゲーム参加 */
					pTmp = GetInfoSUBGAME ((PITEMINFO_SUBGAME)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SAKE:				/* 酒 */
					pTmp = GetInfoSAKE ((PITEMINFO_SAKE)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MSG:				/* メッセージ表示 */
					pTmp = GetInfoMSG ((PITEMINFO_MSG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_GRPCHG:			/* キャラ画像変更 */
					pTmp = GetInfoGRPCHG ((PITEMINFO_GRPCHG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_LIGHT:			/* 明かり */
					pTmp = GetInfoLIGHT ((PITEMINFO_LIGHT)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPDOWN:			/* HP減少 */
					pTmp = GetInfoHPDOWN ((PITEMINFO_HPDOWN)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MAGIC:			/* 魔法習得 */
					pTmp = GetInfoMAGIC ((PITEMINFO_MAGIC)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MANAUP:			/* Mana回復 */
					pTmp = GetInfoMANAUP ((PITEMINFO_MANAUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPMANAUP:			/* HP・Mana回復 */
					pTmp = GetInfoHPMANAUP ((PITEMINFO_HPMANAUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_HAND:			/* 装備(小手) */
					pTmp = GetInfoEQU_HAND ((PITEMINFO_EQU_HAND)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_LEG:			/* 装備(靴) */
					pTmp = GetInfoEQU_LEG ((PITEMINFO_EQU_LEG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC1:			/* 装備(首飾り) */
					pTmp = GetInfoEQU_ACC1 ((PITEMINFO_EQU_ACC1)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC2:			/* 装備(指輪1) */
					pTmp = GetInfoEQU_ACC2 ((PITEMINFO_EQU_ACC2)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC3:			/* 装備(指輪2) */
					pTmp = GetInfoEQU_ACC3 ((PITEMINFO_EQU_ACC3)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_RESURRECTION:		/* 蘇生 */
					pTmp = GetInfoRESURRECTION ((PITEMINFO_RESURRECTION)pInfo->pData, &dwTmp);
					break;
				}
				SAFE_DELETE_ARRAY (pTmp);
				dwDataSizeTmp += dwTmp;
				break;
			case 10:		/* 使った時のメッセージ */
				if (pInfo->pMsg) {
					dwDataSizeTmp += (strlen (pInfo->pMsg) + 1);
				} else {
					dwDataSizeTmp ++;
				}
				break;
			case 11:	dwDataSizeTmp += (strlen (pInfo->szName) + 1);	break;	/* アイテム名（可変長） */
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
			PITEMINFO pInfo;
			PBYTE pTmp;
			DWORD dwTmp;

			pInfo = g_aItemInfo[j];
			switch (i) {
			case 0:		/* アイテム名（古い版の幅。入らない分は文字の途中で切らずに捨てる） */
				SboTextCopy ((LPSTR)&pData[dwOffset], OLD_MAX_ITEMNAME, pInfo->szName);
				dwOffset += OLD_MAX_ITEMNAME;
				break;
			case 1:		/* 種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byType, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 2:		/* 付加データ */
				CopyMemory (&pData[dwOffset], &pInfo->byData, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 3:		/* フラグ */
				CopyMemory (&pData[dwOffset], &pInfo->byFlg, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 4:		/* 使った時に鳴らす音 */
				CopyMemory (&pData[dwOffset], &pInfo->bySound, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 5:		/* 性別による種別 */
				CopyMemory (&pData[dwOffset], &pInfo->bySexDistinction, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 6:		/* 無くなる確率 */
				CopyMemory (&pData[dwOffset], &pInfo->byEraseProbability, sizeof (BYTE));
				dwOffset += sizeof (BYTE);
				break;
			case 7:		/* グラフィックID */
				CopyMemory (&pData[dwOffset], &pInfo->wGrpID, sizeof (WORD));
				dwOffset += sizeof (WORD);
				break;
			case 8:		/* 値段 */
				CopyMemory (&pData[dwOffset], &pInfo->dwGold, sizeof (DWORD));
				dwOffset += sizeof (DWORD);
				break;
			case 9:		/* 詳細情報 */
				pTmp = NULL;
				switch (pInfo->byType) {
				case ITEMTYPE_RURA:				/* 特定位置へ移動 */
					pTmp = GetInfoRURA ((PITEMINFO_RURA)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPUP:				/* HP回復 */
					pTmp = GetInfoHPUP ((PITEMINFO_HPUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_WEAPON:			/* 武器 */
					pTmp = GetInfoWEAPON ((PITEMINFO_WEAPON)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_ARMOR:			/* 鎧 */
					pTmp = GetInfoARMOR ((PITEMINFO_ARMOR)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SHILD:			/* 盾 */
					pTmp = GetInfoSHILD ((PITEMINFO_SHILD)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HELMET:			/* 兜 */
					pTmp = GetInfoHELMET ((PITEMINFO_HELMET)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SEED:				/* 種 */
					pTmp = GetInfoSEED ((PITEMINFO_SEED)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SUBGAME:			/* サブゲーム参加 */
					pTmp = GetInfoSUBGAME ((PITEMINFO_SUBGAME)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_SAKE:				/* 酒 */
					pTmp = GetInfoSAKE ((PITEMINFO_SAKE)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MSG:				/* メッセージ表示 */
					pTmp = GetInfoMSG ((PITEMINFO_MSG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_GRPCHG:			/* キャラ画像変更 */
					pTmp = GetInfoGRPCHG ((PITEMINFO_GRPCHG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_LIGHT:			/* 明かり */
					pTmp = GetInfoLIGHT ((PITEMINFO_LIGHT)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPDOWN:			/* HP減少 */
					pTmp = GetInfoHPDOWN ((PITEMINFO_HPDOWN)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MAGIC:			/* 魔法習得 */
					pTmp = GetInfoMAGIC ((PITEMINFO_MAGIC)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_MANAUP:			/* Mana回復 */
					pTmp = GetInfoMANAUP ((PITEMINFO_MANAUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_HPMANAUP:			/* HP・Mana回復 */
					pTmp = GetInfoHPMANAUP ((PITEMINFO_HPMANAUP)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_HAND:			/* 装備(小手) */
					pTmp = GetInfoEQU_HAND ((PITEMINFO_EQU_HAND)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_LEG:			/* 装備(靴) */
					pTmp = GetInfoEQU_LEG ((PITEMINFO_EQU_LEG)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC1:			/* 装備(首飾り) */
					pTmp = GetInfoEQU_ACC1 ((PITEMINFO_EQU_ACC1)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC2:			/* 装備(指輪1) */
					pTmp = GetInfoEQU_ACC2 ((PITEMINFO_EQU_ACC2)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_EQU_ACC3:			/* 装備(指輪2) */
					pTmp = GetInfoEQU_ACC3 ((PITEMINFO_EQU_ACC3)pInfo->pData, &dwTmp);
					break;
				case ITEMTYPE_RESURRECTION:		/* 蘇生 */
					pTmp = GetInfoRESURRECTION ((PITEMINFO_RESURRECTION)pInfo->pData, &dwTmp);
					break;
				}
				if (pTmp) {
					CopyMemory (&pData[dwOffset], pTmp, dwTmp);
					dwOffset += dwTmp;
				}
				SAFE_DELETE_ARRAY (pTmp);
				break;
			case 10:	/* 使った時のメッセージ */
				if (pInfo->pMsg) {
					CopyMemory (&pData[dwOffset], pInfo->pMsg, strlen (pInfo->pMsg) + 1);
					dwOffset += (strlen (pInfo->pMsg) + 1);
				} else {
					pData[dwOffset] = 0;
					dwOffset ++;
				}
				break;
			case 11:	/* アイテム名（可変長） */
				CopyMemory (&pData[dwOffset], pInfo->szName, strlen (pInfo->szName) + 1);
				dwOffset += (strlen (pInfo->szName) + 1);
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

	SaveItemTest ();
	return bRet;
}

/* ========================================================================= */
/* 関数名：LoadItemInfo														 */
/* 内容：アイテム情報読み込み												 */
/* 日付：2005/02/27															 */
/* ========================================================================= */
BOOL LoadItemInfo(void)
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
	strcat (szFileName, "SBODATA\\SboDataItemInfo.dat");


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
		PITEMINFO pInfo;

		pInfo = new ITEMINFO;
		ZeroMemory (pInfo, sizeof (ITEMINFO));
		g_aItemInfo.Add (pInfo);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PITEMINFO pInfo;

			if (dwOffsetTmp >= dwSize) {
				break;
			}

			pInfo = g_aItemInfo[j];
			switch (i) {
			case 0:		/* アイテム名 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {
					/* 古い版の幅。新しい版のデータは後ろの szNameV で上書きされる */
					CopyMemory (&pInfo->szName, &pData[dwOffsetTmp], OLD_MAX_ITEMNAME);
					pInfo->szName[OLD_MAX_ITEMNAME - 1] = 0;
					dwOffsetTmp += OLD_MAX_ITEMNAME;
				}
				break;

			case 1:		/* 種別 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {
					CopyMemory (&pInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 2:		/* 付加データ */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {
					CopyMemory (&pInfo->byData, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 3:		/* フラグ */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {
					CopyMemory (&pInfo->byFlg, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 4:		/* 使った時に鳴らす音 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {
					CopyMemory (&pInfo->bySound, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 5:		/* 性別による種別 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {
					CopyMemory (&pInfo->bySexDistinction, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 6:		/* 無くなる確率 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[6]) == 0) {
					CopyMemory (&pInfo->byEraseProbability, &pData[dwOffsetTmp], sizeof (BYTE));
					dwOffsetTmp += sizeof (BYTE);
				}
				break;

			case 7:		/* グラフィックID */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[7]) == 0) {
					CopyMemory (&pInfo->wGrpID, &pData[dwOffsetTmp], sizeof (WORD));
					dwOffsetTmp += sizeof (WORD);
				}
				break;

			case 8:		/* 値段 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[8]) == 0) {
					CopyMemory (&pInfo->dwGold, &pData[dwOffsetTmp], sizeof (DWORD));
					dwOffsetTmp += sizeof (DWORD);
				}
				break;

			case 9:		/* 詳細情報 */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[9]) == 0) {
					PBYTE pTmp;
					DWORD dwTmp;

					dwTmp = dwOffsetTmp;
					pTmp = NULL;

					switch (pInfo->byType) {
					case ITEMTYPE_RURA:				/* 特定位置へ移動 */
						pTmp = PutInfoRURA (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_HPUP:				/* HP回復 */
						pTmp = PutInfoHPUP (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_WEAPON:			/* 武器 */
						pTmp = PutInfoWEAPON (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_ARMOR:			/* 鎧 */
						pTmp = PutInfoARMOR (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_SHILD:			/* 盾 */
						pTmp = PutInfoSHILD (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_HELMET:			/* 兜 */
						pTmp = PutInfoHELMET (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_SEED:				/* 種 */
						pTmp = PutInfoSEED (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_SUBGAME:			/* サブゲーム参加 */
						pTmp = PutInfoSUBGAME (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_SAKE:				/* 酒 */
						pTmp = PutInfoSAKE (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_MSG:				/* メッセージ表示 */
						pTmp = PutInfoMSG (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_GRPCHG:			/* キャラ画像変更 */
						pTmp = PutInfoGRPCHG (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_LIGHT:			/* 明かり */
						pTmp = PutInfoLIGHT (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_HPDOWN:			/* HP減少 */
						pTmp = PutInfoHPDOWN (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_MAGIC:			/* 魔法習得 */
						pTmp = PutInfoMAGIC (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_MANAUP:			/* Mana回復 */
						pTmp = PutInfoMANAUP (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_HPMANAUP:			/* HP・Mana回復 */
						pTmp = PutInfoHPMANAUP (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_EQU_HAND:			/* 装備(小手) */
						pTmp = PutInfoEQU_HAND (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_EQU_LEG:			/* 装備(靴) */
						pTmp = PutInfoEQU_LEG (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_EQU_ACC1:			/* 装備(首飾り) */
						pTmp = PutInfoEQU_ACC1 (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_EQU_ACC2:			/* 装備(指輪1) */
						pTmp = PutInfoEQU_ACC2 (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_EQU_ACC3:			/* 装備(指輪2) */
						pTmp = PutInfoEQU_ACC3 (&pData[dwOffsetTmp], &dwTmp);
						break;
					case ITEMTYPE_RESURRECTION:		/* 蘇生 */
						pTmp = PutInfoRESURRECTION (&pData[dwOffsetTmp], &dwTmp);
						break;
					default:
						dwTmp = 0;
						break;
					}
					pInfo->pData = pTmp;
					dwOffsetTmp += dwTmp;
				}
				break;

			case 10:		/* 使った時のメッセージ */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[10]) == 0) {
					int nLen;

					nLen = strlen ((LPCSTR)&pData[dwOffsetTmp]) + 1;
					if (nLen > 1) {
						pInfo->pMsg = new char[nLen];
						strcpy (pInfo->pMsg, (LPCSTR)&pData[dwOffsetTmp]);
					}
					dwOffsetTmp += nLen;
				}
				break;

			case 11:		/* アイテム名（可変長） */
				if (strcmp (aHeaderInfo[i].szName, s_aszName[11]) == 0) {
					LPCSTR pszSrc = (LPCSTR)&pData[dwOffsetTmp];
					SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pszSrc);
					dwOffsetTmp += (strlen (pszSrc) + 1);
				}
				break;
			}
		}
	}


	bRet = TRUE;
//Todo:test
	ConvItemTest ();
Exit:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle (hFile);
	}
	SAFE_DELETE_ARRAY (pData);
	return bRet;
}

/* ========================================================================= */
/* 関数名：PutInfoRURA														 */
/* 内容：保存用アイテム情報取得(特定位置へ移動)								 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoRURA(PITEMINFO_RURA pInfo, PDWORD pdwSize)
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

		if (s_aszNameRURA[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameRURA[i]);

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
		case 0:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 使用限定するマップID(先頭ビットはフラグ) */
		case 1:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 移動先マップID */
		case 2:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 移動先X座標 */
		case 3:		dwDataSizeTmp = sizeof (DWORD);			break;	/* 移動先Y座標 */
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
		case 0:		/* 使用限定するマップID(先頭ビットはフラグ) */
			CopyMemory (&pData[dwOffset], &pInfo->dwEnableMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 1:		/* 移動先マップID */
			CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 2:		/* 移動先X座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwX, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		case 3:		/* 移動先Y座標 */
			CopyMemory (&pData[dwOffset], &pInfo->dwY, sizeof (DWORD));
			dwOffset += sizeof (DWORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoHPUP														 */
/* 内容：保存用アイテム情報取得(HP回復)										 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoHPUP(PITEMINFO_HPUP pInfo, PDWORD pdwSize)
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

		if (s_aszNameHPUP[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameHPUP[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 確立で回復？ */
		case 1:		dwDataSizeTmp = sizeof (WORD);			break;	/* 最低回復値 */
		case 2:		dwDataSizeTmp = sizeof (WORD);			break;	/* 最大回復値 */
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
		case 0:		/* 確立で回復？ */
			CopyMemory (&pData[dwOffset], &pInfo->byRate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 最低回復値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMin, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 2:		/* 最大回復値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMax, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoWEAPON													 */
/* 内容：保存用アイテム情報取得(武器)										 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoWEAPON(PITEMINFO_WEAPON pInfo, PDWORD pdwSize)
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

		if (s_aszNameWEAPON[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameWEAPON[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 攻撃時に発動するエフェクト番号 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 攻撃力減少幅 */
		case 2:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 攻撃種別 */
		case 3:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 付加データ1 */
		case 4:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 付加データ2 */
		case 5:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 付加データ3 */
		case 6:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 付加データ4 */
		case 7:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 付加データ5 */
		case 8:		dwDataSizeTmp = sizeof (WORD);			break;	/* 装備できるキャラ情報 */
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
		case 0:		/* 攻撃時に発動するエフェクト番号 */
			CopyMemory (&pData[dwOffset], &pInfo->byEfcID, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 攻撃力減少幅 */
			CopyMemory (&pData[dwOffset], &pInfo->byMin, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 攻撃種別 */
			CopyMemory (&pData[dwOffset], &pInfo->byAtackType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 3:		/* 付加データ1 */
			CopyMemory (&pData[dwOffset], &pInfo->byData1, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 4:		/* 付加データ2 */
			CopyMemory (&pData[dwOffset], &pInfo->byData2, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 5:		/* 付加データ3 */
			CopyMemory (&pData[dwOffset], &pInfo->byData3, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 6:		/* 付加データ4 */
			CopyMemory (&pData[dwOffset], &pInfo->byData4, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 7:		/* 付加データ5 */
			CopyMemory (&pData[dwOffset], &pInfo->byData5, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 8:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoARMOR														 */
/* 内容：保存用アイテム情報取得(鎧)											 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoARMOR(PITEMINFO_ARMOR pInfo, PDWORD pdwSize)
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

		if (s_aszNameARMOR[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameARMOR[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);			break;	/* 装備できるキャラ情報 */
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (DWORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoSHILD														 */
/* 内容：保存用アイテム情報取得(盾)											 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoSHILD(PITEMINFO_SHILD pInfo, PDWORD pdwSize)
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

		if (s_aszNameSHILD[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameSHILD[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);			break;	/* 装備できるキャラ情報 */
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (DWORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoHELMET													 */
/* 内容：保存用アイテム情報取得(兜)											 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoHELMET(PITEMINFO_HELMET pInfo, PDWORD pdwSize)
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

		if (s_aszNameHELMET[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameHELMET[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);			break;	/* 装備できるキャラ情報 */
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (DWORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoSEED														 */
/* 内容：保存用アイテム情報取得(種)											 */
/* 日付：2005/02/27															 */
/* ========================================================================= */
static PBYTE GetInfoSEED(PITEMINFO_SEED pInfo, PDWORD pdwSize)
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

		if (s_aszNameSEED[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameSEED[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 種類 */
		case 1:		dwDataSizeTmp = sizeof (WORD);			break;	/* 上限値 */
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
		case 0:		/* 種類 */
			CopyMemory (&pData[dwOffset], &pInfo->byType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 上限値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMax, sizeof (WORD));
			dwOffset += sizeof (DWORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoSUBGAME													 */
/* 内容：保存用アイテム情報取得(サブゲーム参加)								 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoSUBGAME(PITEMINFO_SUBGAME pInfo, PDWORD pdwSize)
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

		if (s_aszNameSUBGAME[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameSUBGAME[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);			break;	/* 種類 */
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
		case 0:		/* 種類 */
			CopyMemory (&pData[dwOffset], &pInfo->byType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoSAKE														 */
/* 内容：保存用アイテム情報取得(酒)											 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoSAKE(PITEMINFO_SAKE pInfo, PDWORD pdwSize)
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

		if (s_aszNameSAKE[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameSAKE[i]);

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
		case 0:		dwDataSizeTmp = sizeof (char);			break;	/* 度数 */
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
		case 0:		/* 度数 */
			CopyMemory (&pData[dwOffset], &pInfo->chPoint, sizeof (char));
			dwOffset += sizeof (char);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoMSG														 */
/* 内容：保存用アイテム情報取得(メッセージ表示)								 */
/* 日付：2005/02/22															 */
/* ========================================================================= */
static PBYTE GetInfoMSG(PITEMINFO_MSG pInfo, PDWORD pdwSize)
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

		if (s_aszNameMSG[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameMSG[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);					break;	/* 表示種別 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);					break;	/* エフェクト番号 */
		case 2:		dwDataSizeTmp = OLD_MAX_LENCHAREFC;				break;	/* 文字エフェクト（古い版の幅） */
		case 3:																/* 他のキャラに使った時のメッセージ */
			if (pInfo->pMsgTo) {
				dwDataSizeTmp = strlen (pInfo->pMsgTo) + 1;
			} else {
				dwDataSizeTmp = 1;
			}
			break;
		case 4:																/* 他のキャラに使われた時のメッセージ */
			if (pInfo->pMsgMe) {
				dwDataSizeTmp = strlen (pInfo->pMsgMe) + 1;
			} else {
				dwDataSizeTmp = 1;
			}
			break;
		case 5:		dwDataSizeTmp = strlen (pInfo->szCharEfc) + 1;	break;	/* 文字エフェクト（可変長） */
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
		case 0:		/* 表示種別 */
			CopyMemory (&pData[dwOffset], &pInfo->byType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* エフェクト番号 */
			CopyMemory (&pData[dwOffset], &pInfo->byEfectID, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 文字エフェクト（古い版の幅） */
			SboTextCopy ((LPSTR)&pData[dwOffset], OLD_MAX_LENCHAREFC, pInfo->szCharEfc);
			dwOffset += OLD_MAX_LENCHAREFC;
			break;
		case 3:		/* 他のキャラに使った時のメッセージ */
			if (pInfo->pMsgTo) {
				CopyMemory (&pData[dwOffset], pInfo->pMsgTo, strlen (pInfo->pMsgTo) + 1);
				dwOffset += (strlen (pInfo->pMsgTo) + 1);
			} else {
				pData[dwOffset] = 0;
				dwOffset ++;
			}
			break;
		case 4:		/* 他のキャラに使われた時のメッセージ */
			if (pInfo->pMsgMe) {
				CopyMemory (&pData[dwOffset], pInfo->pMsgMe, strlen (pInfo->pMsgMe) + 1);
				dwOffset += (strlen (pInfo->pMsgMe) + 1);
			} else {
				pData[dwOffset] = 0;
				dwOffset ++;
			}
			break;
		case 5:		/* 文字エフェクト（可変長） */
			CopyMemory (&pData[dwOffset], pInfo->szCharEfc, strlen (pInfo->szCharEfc) + 1);
			dwOffset += (strlen (pInfo->szCharEfc) + 1);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoGRPCHG													 */
/* 内容：保存用アイテム情報取得(キャラ画像変更)								 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoGRPCHG(PITEMINFO_GRPCHG pInfo, PDWORD pdwSize)
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

		if (s_aszNameGRPCHG[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameGRPCHG[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 変更するグラフィックID */
			break;
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
		case 0:		/* 変更するグラフィックID */
			CopyMemory (&pData[dwOffset], &pInfo->wGrpID, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoLIGHT														 */
/* 内容：保存用アイテム情報取得(明かり)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoLIGHT(PITEMINFO_LIGHT pInfo, PDWORD pdwSize)
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

		if (s_aszNameLIGHT[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameLIGHT[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 明るさレベル */
		case 1:		dwDataSizeTmp = sizeof (WORD);				break;	/* 維持時間 */
			break;
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
		case 0:		/* 明るさレベル */
			CopyMemory (&pData[dwOffset], &pInfo->byLevel, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 維持時間 */
			CopyMemory (&pData[dwOffset], &pInfo->wTime, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoHPDOWN													 */
/* 内容：保存用アイテム情報取得(HP減少)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoHPDOWN(PITEMINFO_HPDOWN pInfo, PDWORD pdwSize)
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

		if (s_aszNameHPDOWN[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameHPDOWN[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);				break;	/* エフェクト番号 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 確立で減少？ */
		case 2:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最低減少値 */
		case 3:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最大減少値 */
			break;
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
		case 0:		/* エフェクト番号 */
			CopyMemory (&pData[dwOffset], &pInfo->byEfectID, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 確立で減少？ */
			CopyMemory (&pData[dwOffset], &pInfo->byRate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 最低減少値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMin, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 3:		/* 最大減少値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMax, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoMAGIC														 */
/* 内容：保存用アイテム情報取得(魔法習得)									 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoMAGIC(PITEMINFO_MAGIC pInfo, PDWORD pdwSize)
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

		if (s_aszNameMAGIC[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameMAGIC[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 習得する魔法 */
			break;
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
		case 0:		/* 習得する魔法 */
			CopyMemory (&pData[dwOffset], &pInfo->byID, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoMANAUP													 */
/* 内容：保存用アイテム情報取得(Mana回復)									 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoMANAUP(PITEMINFO_MANAUP pInfo, PDWORD pdwSize)
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

		if (s_aszNameMANAUP[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameMANAUP[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 確立で回復？ */
		case 1:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最低減少値 */
		case 2:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最大減少値 */
			break;
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
		case 0:		/* 確立で回復？ */
			CopyMemory (&pData[dwOffset], &pInfo->byRate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 最低減少値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMin, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 2:		/* 最大減少値 */
			CopyMemory (&pData[dwOffset], &pInfo->wMax, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoHPMANAUP													 */
/* 内容：保存用アイテム情報取得(HP・Mana回復)								 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoHPMANAUP(PITEMINFO_HPMANAUP pInfo, PDWORD pdwSize)
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

		if (s_aszNameHPMANAUP[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameHPMANAUP[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 確立で回復？(HP) */
		case 1:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 確立で回復？(Mana) */
		case 2:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最低減少値(HP) */
		case 3:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最大減少値(HP) */
		case 4:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最低減少値(Mana) */
		case 5:		dwDataSizeTmp = sizeof (WORD);				break;	/* 最大減少値(Mana) */
			break;
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
		case 0:		/* 確立で回復？(HP) */
			CopyMemory (&pData[dwOffset], &pInfo->byRateHP, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 1:		/* 確立で回復？(Mana) */
			CopyMemory (&pData[dwOffset], &pInfo->byRateMana, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		case 2:		/* 最低減少値(HP) */
			CopyMemory (&pData[dwOffset], &pInfo->wMinHP, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 3:		/* 最大減少値(HP) */
			CopyMemory (&pData[dwOffset], &pInfo->wMaxHP, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 4:		/* 最低減少値(Mana) */
			CopyMemory (&pData[dwOffset], &pInfo->wMinMana, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 5:		/* 最低減少値(Mana) */
			CopyMemory (&pData[dwOffset], &pInfo->wMaxMana, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoEQU_HAND													 */
/* 内容：保存用アイテム情報取得(小手)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoEQU_HAND(PITEMINFO_EQU_HAND pInfo, PDWORD pdwSize)
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

		if (s_aszNameEQU_HAND[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameEQU_HAND[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 装備できるキャラ情報 */
			break;
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoEQU_LEG													 */
/* 内容：保存用アイテム情報取得(靴)											 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoEQU_LEG(PITEMINFO_EQU_LEG pInfo, PDWORD pdwSize)
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

		if (s_aszNameEQU_LEG[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameEQU_LEG[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 装備できるキャラ情報 */
			break;
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoEQU_ACC1													 */
/* 内容：保存用アイテム情報取得(首飾り)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoEQU_ACC1(PITEMINFO_EQU_ACC1 pInfo, PDWORD pdwSize)
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

		if (s_aszNameEQU_ACC1[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameEQU_ACC1[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 装備できるキャラ情報 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 装備効果 */
			break;
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 1:		/* 装備効果 */
			CopyMemory (&pData[dwOffset], &pInfo->byEquType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoEQU_ACC2													 */
/* 内容：保存用アイテム情報取得(指輪1)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoEQU_ACC2(PITEMINFO_EQU_ACC2 pInfo, PDWORD pdwSize)
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

		if (s_aszNameEQU_ACC2[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameEQU_ACC2[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 装備できるキャラ情報 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 装備効果 */
			break;
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 1:		/* 装備効果 */
			CopyMemory (&pData[dwOffset], &pInfo->byEquType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：GetInfoEQU_ACC3													 */
/* 内容：保存用アイテム情報取得(指輪2)										 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE GetInfoEQU_ACC3(PITEMINFO_EQU_ACC3 pInfo, PDWORD pdwSize)
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

		if (s_aszNameEQU_ACC3[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameEQU_ACC3[i]);

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
		case 0:		dwDataSizeTmp = sizeof (WORD);				break;	/* 装備できるキャラ情報 */
		case 1:		dwDataSizeTmp = sizeof (BYTE);				break;	/* 装備効果 */
			break;
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
		case 0:		/* 装備できるキャラ情報 */
			CopyMemory (&pData[dwOffset], &pInfo->wEqu, sizeof (WORD));
			dwOffset += sizeof (WORD);
			break;
		case 1:		/* 装備効果 */
			CopyMemory (&pData[dwOffset], &pInfo->byEquType, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：	GetInfoRESURRECTION												 */
/* 内容：	保存用アイテム情報取得(蘇生)									 */
/* 日付：	2005/11/03														 */
/* ========================================================================= */
static PBYTE GetInfoRESURRECTION(PITEMINFO_RESURRECTION pInfo, PDWORD pdwSize)
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

		if (s_aszNameRESURRECTION[i][0] == 'z') {
			break;
		}
		strcpy (Info.szName, s_aszNameRESURRECTION[i]);

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
		case 0:		dwDataSizeTmp = sizeof (BYTE);		break;	/* 回復率 */
			break;
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
		case 0:		/* 回復率 */
			CopyMemory (&pData[dwOffset], &pInfo->byRate, sizeof (BYTE));
			dwOffset += sizeof (BYTE);
			break;
		}
	}

	return pData;
}


/* ========================================================================= */
/* 関数名：PutInfoRURA														 */
/* 内容：アイテム情報作成(特定位置へ移動)									 */
/* 日付：2005/02/23															 */
/* ========================================================================= */
static PBYTE PutInfoRURA(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_RURA pInfo;
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
			if (s_aszNameRURA[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameRURA[j]) == 0) {
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

	pInfo = new ITEMINFO_RURA;
	ZeroMemory (pInfo, sizeof (ITEMINFO_RURA));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 使用限定するマップID(先頭ビットはフラグ) */
			if (strcmp (aHeaderInfo[i].szName, "dwEnableMapID") == 0) {
				CopyMemory (&pInfo->dwEnableMapID, &pData[dwOffsetTmp], sizeof (DWORD));
				dwOffsetTmp += sizeof (DWORD);
			}
			break;
		case 1:		/* 移動先マップID */
			if (strcmp (aHeaderInfo[i].szName, "dwMapID") == 0) {
				CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (DWORD));
				dwOffsetTmp += sizeof (DWORD);
			}
			break;
		case 2:		/* 移動先X座標 */
			if (strcmp (aHeaderInfo[i].szName, "dwX") == 0) {
				CopyMemory (&pInfo->dwX, &pData[dwOffsetTmp], sizeof (DWORD));
				dwOffsetTmp += sizeof (DWORD);
			}
			break;
		case 3:		/* 移動先Y座標 */
			if (strcmp (aHeaderInfo[i].szName, "dwY") == 0) {
				CopyMemory (&pInfo->dwY, &pData[dwOffsetTmp], sizeof (DWORD));
				dwOffsetTmp += sizeof (DWORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoHPUP														 */
/* 内容：アイテム情報作成(HP回復)											 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoHPUP(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_HPUP pInfo;
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
			if (s_aszNameHPUP[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameHPUP[j]) == 0) {
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

	pInfo = new ITEMINFO_HPUP;
	ZeroMemory (pInfo, sizeof (ITEMINFO_HPUP));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 確立で回復？ */
			if (strcmp (aHeaderInfo[i].szName, "byRate") == 0) {
				CopyMemory (&pInfo->byRate, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 最低回復値 */
			if (strcmp (aHeaderInfo[i].szName, "wMin") == 0) {
				CopyMemory (&pInfo->wMin, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 2:		/* 最大回復値 */
			if (strcmp (aHeaderInfo[i].szName, "wMax") == 0) {
				CopyMemory (&pInfo->wMax, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoWEAPON													 */
/* 内容：アイテム情報作成(武器)												 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoWEAPON(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_WEAPON pInfo;
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
			if (s_aszNameWEAPON[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameWEAPON[j]) == 0) {
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

	pInfo = new ITEMINFO_WEAPON;
	ZeroMemory (pInfo, sizeof (ITEMINFO_WEAPON));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 攻撃時に発動するエフェクト番号 */
			if (strcmp (aHeaderInfo[i].szName, "byEfcID") == 0) {
				CopyMemory (&pInfo->byEfcID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 攻撃力減少幅 */
			if (strcmp (aHeaderInfo[i].szName, "byMin") == 0) {
				CopyMemory (&pInfo->byMin, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 2:		/* 攻撃種別 */
			if (strcmp (aHeaderInfo[i].szName, "byAtackType") == 0) {
				CopyMemory (&pInfo->byAtackType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 3:		/* 付加データ1 */
			if (strcmp (aHeaderInfo[i].szName, "byData1") == 0) {
				CopyMemory (&pInfo->byData1, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 4:		/* 付加データ2 */
			if (strcmp (aHeaderInfo[i].szName, "byData2") == 0) {
				CopyMemory (&pInfo->byData2, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 5:		/* 付加データ3 */
			if (strcmp (aHeaderInfo[i].szName, "byData3") == 0) {
				CopyMemory (&pInfo->byData3, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 6:		/* 付加データ4 */
			if (strcmp (aHeaderInfo[i].szName, "byData4") == 0) {
				CopyMemory (&pInfo->byData4, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 7:		/* 付加データ5 */
			if (strcmp (aHeaderInfo[i].szName, "byData5") == 0) {
				CopyMemory (&pInfo->byData5, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 8:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoARMOR														 */
/* 内容：アイテム情報作成(鎧)												 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoARMOR(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_ARMOR pInfo;
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
			if (s_aszNameARMOR[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameARMOR[j]) == 0) {
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

	pInfo = new ITEMINFO_ARMOR;
	ZeroMemory (pInfo, sizeof (ITEMINFO_ARMOR));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoSHILD														 */
/* 内容：アイテム情報作成(盾)												 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoSHILD(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_SHILD pInfo;
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
			if (s_aszNameSHILD[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameSHILD[j]) == 0) {
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

	pInfo = new ITEMINFO_SHILD;
	ZeroMemory (pInfo, sizeof (ITEMINFO_SHILD));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoHELMET													 */
/* 内容：アイテム情報作成(兜)												 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoHELMET(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_HELMET pInfo;
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
			if (s_aszNameHELMET[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameHELMET[j]) == 0) {
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

	pInfo = new ITEMINFO_HELMET;
	ZeroMemory (pInfo, sizeof (ITEMINFO_HELMET));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoSEED														 */
/* 内容：アイテム情報作成(種)												 */
/* 日付：2005/02/27															 */
/* ========================================================================= */
static PBYTE PutInfoSEED(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_SEED pInfo;
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
			if (s_aszNameSEED[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameSEED[j]) == 0) {
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

	pInfo = new ITEMINFO_SEED;
	ZeroMemory (pInfo, sizeof (ITEMINFO_SEED));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 種類 */
			if (strcmp (aHeaderInfo[i].szName, "byType") == 0) {
				CopyMemory (&pInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 上限値 */
			if (strcmp (aHeaderInfo[i].szName, "wMax") == 0) {
				CopyMemory (&pInfo->wMax, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoSUBGAME													 */
/* 内容：アイテム情報作成(サブゲーム参加)									 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoSUBGAME(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_SUBGAME pInfo;
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
			if (s_aszNameSUBGAME[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameSUBGAME[j]) == 0) {
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

	pInfo = new ITEMINFO_SUBGAME;
	ZeroMemory (pInfo, sizeof (ITEMINFO_SUBGAME));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 種類 */
			if (strcmp (aHeaderInfo[i].szName, "byType") == 0) {
				CopyMemory (&pInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoSAKE														 */
/* 内容：アイテム情報作成(酒)												 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoSAKE(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_SAKE pInfo;
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
			if (s_aszNameSAKE[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameSAKE[j]) == 0) {
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

	pInfo = new ITEMINFO_SAKE;
	ZeroMemory (pInfo, sizeof (ITEMINFO_SAKE));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 種類 */
			if (strcmp (aHeaderInfo[i].szName, "chPoint") == 0) {
				CopyMemory (&pInfo->chPoint, &pData[dwOffsetTmp], sizeof (char));
				dwOffsetTmp += sizeof (char);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoMSG														 */
/* 内容：アイテム情報作成(メッセージ表示)									 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoMSG(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_MSG pInfo;
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
			if (s_aszNameMSG[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameMSG[j]) == 0) {
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

	pInfo = new ITEMINFO_MSG;
	ZeroMemory (pInfo, sizeof (ITEMINFO_MSG));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 表示種別 */
			if (strcmp (aHeaderInfo[i].szName, "byType") == 0) {
				CopyMemory (&pInfo->byType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* エフェクト番号 */
			if (strcmp (aHeaderInfo[i].szName, "byEfectID") == 0) {
				CopyMemory (&pInfo->byEfectID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 2:		/* 文字エフェクト */
			if (strcmp (aHeaderInfo[i].szName, "szCharEfc") == 0) {
				/* 古い版の幅。新しい版のデータは後ろの szCharEfcV で上書きされる */
				CopyMemory (&pInfo->szCharEfc, &pData[dwOffsetTmp], OLD_MAX_LENCHAREFC);
				pInfo->szCharEfc[OLD_MAX_LENCHAREFC - 1] = 0;
				dwOffsetTmp += OLD_MAX_LENCHAREFC;
			}
			break;
		case 3:		/* 他のキャラに使った時のメッセージ */
			if (strcmp (aHeaderInfo[i].szName, "pMsgTo") == 0) {
				int nLen;

				nLen = strlen ((LPCSTR)&pData[dwOffsetTmp]) + 1;
				if (nLen <= 1) {
					break;
				}
				pInfo->pMsgTo = new char[nLen];
				strcpy (pInfo->pMsgTo, (LPCSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += nLen;
			}
			break;
		case 4:		/* 他のキャラに使われた時のメッセージ */
			if (strcmp (aHeaderInfo[i].szName, "pMsgMe") == 0) {
				int nLen;

				nLen = strlen ((LPCSTR)&pData[dwOffsetTmp]) + 1;
				if (nLen <= 1) {
					break;
				}
				pInfo->pMsgMe = new char[nLen];
				strcpy (pInfo->pMsgMe, (LPCSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += nLen;
			}
			break;
		case 5:		/* 文字エフェクト（可変長） */
			if (strcmp (aHeaderInfo[i].szName, "szCharEfcV") == 0) {
				LPCSTR pszSrc = (LPCSTR)&pData[dwOffsetTmp];
				SboTextCopy (pInfo->szCharEfc, sizeof (pInfo->szCharEfc), pszSrc);
				dwOffsetTmp += (strlen (pszSrc) + 1);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoGRPCHG													 */
/* 内容：アイテム情報作成(キャラ画像変更)									 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoGRPCHG(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_GRPCHG pInfo;
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
			if (s_aszNameGRPCHG[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameGRPCHG[j]) == 0) {
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

	pInfo = new ITEMINFO_GRPCHG;
	ZeroMemory (pInfo, sizeof (ITEMINFO_GRPCHG));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 変更するグラフィックID */
			if (strcmp (aHeaderInfo[i].szName, "wGrpID") == 0) {
				CopyMemory (&pInfo->wGrpID, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoLIGHT														 */
/* 内容：アイテム情報作成(明かり)											 */
/* 日付：2005/02/24															 */
/* ========================================================================= */
static PBYTE PutInfoLIGHT(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_LIGHT pInfo;
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
			if (s_aszNameLIGHT[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameLIGHT[j]) == 0) {
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

	pInfo = new ITEMINFO_LIGHT;
	ZeroMemory (pInfo, sizeof (ITEMINFO_LIGHT));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 明るさレベル */
			if (strcmp (aHeaderInfo[i].szName, "byLevel") == 0) {
				CopyMemory (&pInfo->byLevel, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 維持時間 */
			if (strcmp (aHeaderInfo[i].szName, "wTime") == 0) {
				CopyMemory (&pInfo->wTime, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoHPDOWN													 */
/* 内容：アイテム情報作成(HP減少)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoHPDOWN(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_HPDOWN pInfo;
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
			if (s_aszNameHPDOWN[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameHPDOWN[j]) == 0) {
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

	pInfo = new ITEMINFO_HPDOWN;
	ZeroMemory (pInfo, sizeof (ITEMINFO_HPDOWN));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* エフェクト番号 */
			if (strcmp (aHeaderInfo[i].szName, "byEfectID") == 0) {
				CopyMemory (&pInfo->byEfectID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 確立で減少？ */
			if (strcmp (aHeaderInfo[i].szName, "byRate") == 0) {
				CopyMemory (&pInfo->byRate, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 2:		/* 最低減少値 */
			if (strcmp (aHeaderInfo[i].szName, "wMin") == 0) {
				CopyMemory (&pInfo->wMin, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 3:		/* 最大減少値 */
			if (strcmp (aHeaderInfo[i].szName, "wMax") == 0) {
				CopyMemory (&pInfo->wMax, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoMAGIC														 */
/* 内容：アイテム情報作成(魔法習得)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoMAGIC(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_MAGIC pInfo;
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
			if (s_aszNameMAGIC[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameMAGIC[j]) == 0) {
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

	pInfo = new ITEMINFO_MAGIC;
	ZeroMemory (pInfo, sizeof (ITEMINFO_MAGIC));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 習得する魔法 */
			if (strcmp (aHeaderInfo[i].szName, "byID") == 0) {
				CopyMemory (&pInfo->byID, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoMANAUP													 */
/* 内容：アイテム情報作成(Mana回復)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoMANAUP(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_MANAUP pInfo;
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
			if (s_aszNameMANAUP[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameMANAUP[j]) == 0) {
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

	pInfo = new ITEMINFO_MANAUP;
	ZeroMemory (pInfo, sizeof (ITEMINFO_MANAUP));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 確立で回復？ */
			if (strcmp (aHeaderInfo[i].szName, "byRate") == 0) {
				CopyMemory (&pInfo->byRate, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 最低減少値 */
			if (strcmp (aHeaderInfo[i].szName, "wMin") == 0) {
				CopyMemory (&pInfo->wMin, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 2:		/* 最大減少値 */
			if (strcmp (aHeaderInfo[i].szName, "wMax") == 0) {
				CopyMemory (&pInfo->wMax, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoHPMANAUP													 */
/* 内容：アイテム情報作成(HP・Mana回復)										 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoHPMANAUP(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_HPMANAUP pInfo;
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
			if (s_aszNameHPMANAUP[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameHPMANAUP[j]) == 0) {
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

	pInfo = new ITEMINFO_HPMANAUP;
	ZeroMemory (pInfo, sizeof (ITEMINFO_HPMANAUP));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 確立で回復？(HP) */
			if (strcmp (aHeaderInfo[i].szName, "byRateHP") == 0) {
				CopyMemory (&pInfo->byRateHP, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 1:		/* 確立で回復？(Mana) */
			if (strcmp (aHeaderInfo[i].szName, "byRateMana") == 0) {
				CopyMemory (&pInfo->byRateMana, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		case 2:		/* 最低減少値(HP) */
			if (strcmp (aHeaderInfo[i].szName, "wMinHP") == 0) {
				CopyMemory (&pInfo->wMinHP, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 3:		/* 最大減少値(HP) */
			if (strcmp (aHeaderInfo[i].szName, "wMaxHP") == 0) {
				CopyMemory (&pInfo->wMaxHP, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 4:		/* 最低減少値(Mana) */
			if (strcmp (aHeaderInfo[i].szName, "wMinMana") == 0) {
				CopyMemory (&pInfo->wMinMana, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 5:		/* 最大減少値(Mana) */
			if (strcmp (aHeaderInfo[i].szName, "wMaxMana") == 0) {
				CopyMemory (&pInfo->wMaxMana, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoEQU_HAND													 */
/* 内容：アイテム情報作成(小手)												 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoEQU_HAND(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_EQU_HAND pInfo;
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
			if (s_aszNameEQU_HAND[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameEQU_HAND[j]) == 0) {
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

	pInfo = new ITEMINFO_EQU_HAND;
	ZeroMemory (pInfo, sizeof (ITEMINFO_EQU_HAND));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoEQU_LEG													 */
/* 内容：アイテム情報作成(靴)												 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoEQU_LEG(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_EQU_LEG pInfo;
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
			if (s_aszNameEQU_LEG[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameEQU_LEG[j]) == 0) {
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

	pInfo = new ITEMINFO_EQU_LEG;
	ZeroMemory (pInfo, sizeof (ITEMINFO_EQU_LEG));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoEQU_ACC1													 */
/* 内容：アイテム情報作成(首飾り)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoEQU_ACC1(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_EQU_ACC1 pInfo;
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
			if (s_aszNameEQU_ACC1[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameEQU_ACC1[j]) == 0) {
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

	pInfo = new ITEMINFO_EQU_ACC1;
	ZeroMemory (pInfo, sizeof (ITEMINFO_EQU_ACC1));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 1:		/* 装備効果 */
			if (strcmp (aHeaderInfo[i].szName, "byEquType") == 0) {
				CopyMemory (&pInfo->byEquType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoEQU_ACC2													 */
/* 内容：アイテム情報作成(指輪1)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoEQU_ACC2(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_EQU_ACC2 pInfo;
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
			if (s_aszNameEQU_ACC2[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameEQU_ACC2[j]) == 0) {
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

	pInfo = new ITEMINFO_EQU_ACC2;
	ZeroMemory (pInfo, sizeof (ITEMINFO_EQU_ACC2));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 1:		/* 装備効果 */
			if (strcmp (aHeaderInfo[i].szName, "byEquType") == 0) {
				CopyMemory (&pInfo->byEquType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：PutInfoEQU_ACC3													 */
/* 内容：アイテム情報作成(指輪2)											 */
/* 日付：2005/02/25															 */
/* ========================================================================= */
static PBYTE PutInfoEQU_ACC3(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_EQU_ACC3 pInfo;
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
			if (s_aszNameEQU_ACC3[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameEQU_ACC3[j]) == 0) {
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

	pInfo = new ITEMINFO_EQU_ACC3;
	ZeroMemory (pInfo, sizeof (ITEMINFO_EQU_ACC3));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 装備できるキャラ情報 */
			if (strcmp (aHeaderInfo[i].szName, "wEqu") == 0) {
				CopyMemory (&pInfo->wEqu, &pData[dwOffsetTmp], sizeof (WORD));
				dwOffsetTmp += sizeof (WORD);
			}
			break;
		case 1:		/* 装備効果 */
			if (strcmp (aHeaderInfo[i].szName, "byEquType") == 0) {
				CopyMemory (&pInfo->byEquType, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* ========================================================================= */
/* 関数名：	PutInfoRESURRECTION												 */
/* 内容：	アイテム情報作成(蘇生)											 */
/* 日付：	2005/11/03														 */
/* ========================================================================= */
static PBYTE PutInfoRESURRECTION(PBYTE pData, PDWORD pdwSize)
{
	DWORD i, j, dwOffset, dwOffsetData, dwColumnCount;
	PITEMINFO_RESURRECTION pInfo;
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
			if (s_aszNameRESURRECTION[j][0] == 'z') {
				break;
			}
			if (strcmp (Info.szName, s_aszNameRESURRECTION[j]) == 0) {
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

	pInfo = new ITEMINFO_RESURRECTION;
	ZeroMemory (pInfo, sizeof (ITEMINFO_RESURRECTION));

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		switch (i) {
		case 0:		/* 回復率 */
			if (strcmp (aHeaderInfo[i].szName, "byRate") == 0) {
				CopyMemory (&pInfo->byRate, &pData[dwOffsetTmp], sizeof (BYTE));
				dwOffsetTmp += sizeof (BYTE);
			}
			break;
		}
		dwOffset = dwOffsetTmp;
	}

	*pdwSize = dwOffsetData + dwOffset;
	return (PBYTE)pInfo;
}


/* 2006/06/29 */
/* 旧形式からの変換 */
void ConvItemTest(void)
{
	int i, nCount;
	DWORD dwTmp;
	POINT ptTmp;
	PITEMINFO pItemInfo;
	PCInfoItemBase pInfoItem;

	nCount = g_aItemInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pItemInfo = g_aItemInfo[i];
		pInfoItem = NULL;

		switch (pItemInfo->byType) {
		case ITEMTYPE_RURA:				/* 特定位置へ移動 */
			{
				PITEMINFO_RURA pInfo;
				PCInfoItemMOVE pItem;

				pInfo		= (PITEMINFO_RURA)pItemInfo->pData;
				pInfoItem	= new CInfoItemMOVE;
				pItem		= (PCInfoItemMOVE)pInfoItem;

				dwTmp = pInfo->dwEnableMapID & ~0x80000000;
				if ((pInfo->dwEnableMapID & 0x80000000) == 0) {
					dwTmp = -1;
				}
				pItem->SetEnableMapID (dwTmp);
				pItem->SetMapID (pInfo->dwMapID);
				ptTmp.x = pInfo->dwX;
				ptTmp.y = pInfo->dwY;
				pItem->SetPos (ptTmp);
			}
			break;
		case ITEMTYPE_HPUP:				/* HP回復 */
			{
				PITEMINFO_HPUP pInfo;
				PCInfoItemHPUP pItem;

				pInfo		= (PITEMINFO_HPUP)pItemInfo->pData;
				pInfoItem	= new CInfoItemHPUP;
				pItem		= (PCInfoItemHPUP)pInfoItem;

				pItem->SetRate (pInfo->byRate);
				pItem->SetMin (pInfo->wMin);
				pItem->SetMax (pInfo->wMax);
			}
			break;
		case ITEMTYPE_WEAPON:			/* 武器 */
			{
				PITEMINFO_WEAPON pInfo;
				PCInfoItemWEAPON pItem;

				pInfo		= (PITEMINFO_WEAPON)pItemInfo->pData;
				pInfoItem	= new CInfoItemWEAPON;
				pItem		= (PCInfoItemWEAPON)pInfoItem;

				pItem->SetEfcID (pInfo->byEfcID);
				pItem->SetMin (pInfo->byMin);
				pItem->SetAtackType (pInfo->byAtackType);
				pItem->SetData1 (pInfo->byData1);
				pItem->SetData2 (pInfo->byData2);
				pItem->SetData3 (pInfo->byData3);
				pItem->SetData4 (pInfo->byData4);
				pItem->SetData5 (pInfo->byData5);
				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_ARMOR:			/* 鎧 */
			{
				PITEMINFO_ARMOR pInfo;
				PCInfoItemARMOR pItem;

				pInfo		= (PITEMINFO_ARMOR)pItemInfo->pData;
				pInfoItem	= new CInfoItemARMOR;
				pItem		= (PCInfoItemARMOR)pInfoItem;

				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_SHILD:			/* 盾 */
			{
				PITEMINFO_SHILD pInfo;
				PCInfoItemSHILD pItem;

				pInfo		= (PITEMINFO_SHILD)pItemInfo->pData;
				pInfoItem	= new CInfoItemSHILD;
				pItem		= (PCInfoItemSHILD)pInfoItem;

				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_HELMET:			/* 兜 */
			{
				PITEMINFO_HELMET pInfo;
				PCInfoItemHELMET pItem;

				pInfo		= (PITEMINFO_HELMET)pItemInfo->pData;
				pInfoItem	= new CInfoItemHELMET;
				pItem		= (PCInfoItemHELMET)pInfoItem;

				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_SEED:				/* 種 */
			{
				PITEMINFO_SEED pInfo;
				PCInfoItemSEED pItem;

				pInfo		= (PITEMINFO_SEED)pItemInfo->pData;
				pInfoItem	= new CInfoItemSEED;
				pItem		= (PCInfoItemSEED)pInfoItem;

				pItem->SetType (pInfo->byType);
				pItem->SetMax (pInfo->wMax);
			}
			break;
		case ITEMTYPE_SUBGAME:			/* サブゲーム参加 */
			{
				PITEMINFO_SUBGAME pInfo;
				PCInfoItemSUBGAME pItem;

				pInfo		= (PITEMINFO_SUBGAME)pItemInfo->pData;
				pInfoItem	= new CInfoItemSUBGAME;
				pItem		= (PCInfoItemSUBGAME)pInfoItem;

				pItem->SetType (pInfo->byType);
			}
			break;
		case ITEMTYPE_SAKE:				/* 酒 */
			{
				PITEMINFO_SAKE pInfo;
				PCInfoItemSAKE pItem;

				pInfo		= (PITEMINFO_SAKE)pItemInfo->pData;
				pInfoItem	= new CInfoItemSAKE;
				pItem		= (PCInfoItemSAKE)pInfoItem;

				pItem->SetPoint (pInfo->chPoint);
			}
			break;
		case ITEMTYPE_MSG:				/* メッセージ表示 */
			{
				PITEMINFO_MSG pInfo;
				PCInfoItemMSG pItem;

				pInfo		= (PITEMINFO_MSG)pItemInfo->pData;
				pInfoItem	= new CInfoItemMSG;
				pItem		= (PCInfoItemMSG)pInfoItem;

				pItem->SetType (pInfo->byType);
				pItem->SetEfcID (pInfo->byEfectID);
				pItem->SetCharEfc (pInfo->szCharEfc);
				pItem->SetMsgTo (pInfo->pMsgTo);
				pItem->SetMsgMe (pInfo->pMsgMe);
			}
			break;
		case ITEMTYPE_GRPCHG:			/* キャラ画像変更 */
			{
				PITEMINFO_GRPCHG pInfo;
				PCInfoItemGRPCHG pItem;

				pInfo		= (PITEMINFO_GRPCHG)pItemInfo->pData;
				pInfoItem	= new CInfoItemGRPCHG;
				pItem		= (PCInfoItemGRPCHG)pInfoItem;

				pItem->SetGrpID (pInfo->wGrpID);
			}
			break;
		case ITEMTYPE_LIGHT:			/* 明かり */
			{
				PITEMINFO_LIGHT pInfo;
				PCInfoItemLIGHT pItem;

				pInfo		= (PITEMINFO_LIGHT)pItemInfo->pData;
				pInfoItem	= new CInfoItemLIGHT;
				pItem		= (PCInfoItemLIGHT)pInfoItem;

				pItem->SetLevel (pInfo->byLevel);
				pItem->SetTime (pInfo->wTime);
			}
			break;
		case ITEMTYPE_HPDOWN:			/* HP減少 */
			{
				PITEMINFO_HPDOWN pInfo;
				PCInfoItemHPDOWN pItem;

				pInfo		= (PITEMINFO_HPDOWN)pItemInfo->pData;
				pInfoItem	= new CInfoItemHPDOWN;
				pItem		= (PCInfoItemHPDOWN)pInfoItem;

				pItem->SetEfcID (pInfo->byEfectID);
				pItem->SetRate (pInfo->byRate);
				pItem->SetMin (pInfo->wMin);
				pItem->SetMax (pInfo->wMax);
			}
			break;
		case ITEMTYPE_MAGIC:			/* 魔法習得 */
			{
				PITEMINFO_MAGIC pInfo;
				PCInfoItemSKILL pItem;

				pInfo		= (PITEMINFO_MAGIC)pItemInfo->pData;
				pInfoItem	= new CInfoItemSKILL;
				pItem		= (PCInfoItemSKILL)pInfoItem;

				pItem->SetSkillID (pInfo->byID);
			}
			break;
		case ITEMTYPE_MANAUP:			/* Mana回復 */
			{
				PITEMINFO_MANAUP pInfo;
				PCInfoItemMANAUP pItem;

				pInfo		= (PITEMINFO_MANAUP)pItemInfo->pData;
				pInfoItem	= new CInfoItemMANAUP;
				pItem		= (PCInfoItemMANAUP)pInfoItem;

				pItem->SetRate (pInfo->byRate);
				pItem->SetMin (pInfo->wMin);
				pItem->SetMax (pInfo->wMax);
			}
			break;
		case ITEMTYPE_HPMANAUP:			/* HP・Mana回復 */
			{
				PITEMINFO_HPMANAUP pInfo;
				PCInfoItemHPMANAUP pItem;

				pInfo		= (PITEMINFO_HPMANAUP)pItemInfo->pData;
				pInfoItem	= new CInfoItemHPMANAUP;
				pItem		= (PCInfoItemHPMANAUP)pInfoItem;

				pItem->SetRateHP (pInfo->byRateHP);
				pItem->SetRateMana (pInfo->byRateMana);
				pItem->SetMinHP (pInfo->wMinHP);
				pItem->SetMaxHP (pInfo->wMaxHP);
				pItem->SetMinMana (pInfo->wMinMana);
				pItem->SetMaxMana (pInfo->wMaxMana);
			}
			break;
		case ITEMTYPE_EQU_HAND:			/* 装備(小手) */
			{
				PITEMINFO_EQU_HAND pInfo;
				PCInfoItemEQU_HAND pItem;

				pInfo		= (PITEMINFO_EQU_HAND)pItemInfo->pData;
				pInfoItem	= new CInfoItemEQU_HAND;
				pItem		= (PCInfoItemEQU_HAND)pInfoItem;

				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_EQU_LEG:			/* 装備(靴) */
			{
				PITEMINFO_EQU_LEG pInfo;
				PCInfoItemEQU_LEG pItem;

				pInfo		= (PITEMINFO_EQU_LEG)pItemInfo->pData;
				pInfoItem	= new CInfoItemEQU_LEG;
				pItem		= (PCInfoItemEQU_LEG)pInfoItem;

				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_EQU_ACC1:			/* 装備(首飾り) */
			{
				PITEMINFO_EQU_ACC1 pInfo;
				PCInfoItemEQU_ACC1 pItem;

				pInfo		= (PITEMINFO_EQU_ACC1)pItemInfo->pData;
				pInfoItem	= new CInfoItemEQU_ACC1;
				pItem		= (PCInfoItemEQU_ACC1)pInfoItem;

				pItem->SetEquType (pInfo->byEquType);
				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_EQU_ACC2:			/* 装備(指輪1) */
			{
				PITEMINFO_EQU_ACC2 pInfo;
				PCInfoItemEQU_ACC2 pItem;

				pInfo		= (PITEMINFO_EQU_ACC2)pItemInfo->pData;
				pInfoItem	= new CInfoItemEQU_ACC2;
				pItem		= (PCInfoItemEQU_ACC2)pInfoItem;

				pItem->SetEquType (pInfo->byEquType);
				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_EQU_ACC3:			/* 装備(指輪2) */
			{
				PITEMINFO_EQU_ACC3 pInfo;
				PCInfoItemEQU_ACC3 pItem;

				pInfo		= (PITEMINFO_EQU_ACC3)pItemInfo->pData;
				pInfoItem	= new CInfoItemEQU_ACC3;
				pItem		= (PCInfoItemEQU_ACC3)pInfoItem;

				pItem->SetEquType (pInfo->byEquType);
				pItem->SetEqu (pInfo->wEqu);
			}
			break;
		case ITEMTYPE_RESURRECTION:		/* 蘇生 */
			{
				PITEMINFO_RESURRECTION pInfo;
				PCInfoItemRESURRECTION pItem;

				pInfo		= (PITEMINFO_RESURRECTION)pItemInfo->pData;
				pInfoItem	= new CInfoItemRESURRECTION;
				pItem		= (PCInfoItemRESURRECTION)pInfoItem;

				pItem->SetRate (pInfo->byRate);
			}
			break;
		default:
			pInfoItem = new CInfoItemBase;
			break;
		}
		/* 共通部分の設定 */
		pInfoItem->SetName (pItemInfo->szName);
		pInfoItem->SetMsg (pItemInfo->pMsg);
		pInfoItem->SetTypeBase (pItemInfo->byType);
		pInfoItem->SetFlg (pItemInfo->byFlg);
		pInfoItem->SetSound (pItemInfo->bySound);
		pInfoItem->SetSex (pItemInfo->bySexDistinction);
		pInfoItem->SetEraseRate (pItemInfo->byEraseProbability);
		pInfoItem->SetItemTypeID (pItemInfo->byType);
		pInfoItem->SetGold (pItemInfo->dwGold);
		g_LibInfoItem.Add (pInfoItem);
	}
//test
	LoadItemInfoTest ();
}


/* ========================================================================= */
/* 関数名：	SaveItemInfo													 */
/* 内容：	アイテム情報保存												 */
/* 日付：	2006/06/29														 */
/* ========================================================================= */

void SaveItemTest(void)
{
	int i, j, nCount, nCount2;
	char szFileName[MAX_PATH + 1];
	LPSTR pszTmp;
	LPCSTR pszKeyName;
	CSimpleDB SimpleDB;
	CmyString strTmp;
	PCInfoItemBase pInfoItemBase;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	pszTmp	= PathFindFileName (szFileName);
	*pszTmp	= 0;
	strcat (szFileName, "SBODATA");
	CreateDirectory (szFileName, NULL);
	strcat (szFileName, "\\SboDataItemInfoTest.dat");

	SimpleDB.Create (szFileName);

	nCount = g_LibInfoItem.GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfoItemBase = g_LibInfoItem.Get (i);

		nCount2 = pInfoItemBase->GetKeyCount ();
		for (j = 0; j < nCount2; j ++) {
			pszKeyName = pInfoItemBase->GetKeyName (j);
			pInfoItemBase->GetKeyData (j, strTmp);
			SimpleDB.Add (pszKeyName, strTmp);
		}
	}
	SimpleDB.Save ();
}


/* ========================================================================= */
/* 関数名：	LoadItemInfo													 */
/* 内容：	アイテム情報読み込み											 */
/* 日付：	2006/07/11														 */
/* ========================================================================= */

void LoadItemInfoTest(void)
{
	BOOL bResult;
	int i, j, nCount, nCount2, nNo;
	char szFileName[MAX_PATH + 1], szTmp[128];
	DWORD dwItemTypeID;
	LPSTR pszTmp;
	LPCSTR pszKeyName;
	CSimpleDB SimpleDB;
	CmyString strTmp;
	PCInfoItemBase pInfoItemBase;

	g_LibInfoItem.DeleteAll ();

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	pszTmp	= PathFindFileName (szFileName);
	*pszTmp	= 0;
	strcat (szFileName, "SBODATA");
	CreateDirectory (szFileName, NULL);
	strcat (szFileName, "\\SboDataItemInfoTest.dat");

	SimpleDB.Create (szFileName);
	SimpleDB.Load ();

	nCount = SimpleDB.GetCount ("dwItemTypeID");
	for (i = 0; i < nCount; i ++) {
		SimpleDB.Get ("dwItemTypeID", i, szTmp);
		dwItemTypeID	= (DWORD)atoi (szTmp);
		pInfoItemBase	= g_LibInfoItem.GetNew (dwItemTypeID);
		g_LibInfoItem.Add (pInfoItemBase);
	}

	nCount = g_LibInfoItem.GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfoItemBase = g_LibInfoItem.Get (i);

		nCount2 = pInfoItemBase->GetKeyCount ();
		for (j = 0; j < nCount2; j ++) {
			pszKeyName	= pInfoItemBase->GetKeyName (j);
			bResult = SimpleDB.Get (pszKeyName, 0, szTmp);
			if (bResult == FALSE) {
				continue;
			}
			SimpleDB.Delete (pszKeyName, 0);

			nNo = pInfoItemBase->GetKeyNo (pszKeyName);
			if (nNo < 0) {
				continue;
			}
			pInfoItemBase->SetKeyData (nNo, szTmp);
		}
	}
}

/* Copyright(C)URARA-works 2003-2005 */
