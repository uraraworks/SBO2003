/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSaveLoadChar.cpp										 */
/* 概要：		キャラ情報読み書き処理(実装ファイル)						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "../../Common/Info/InfoJob.h"
#include "../CharProc/IncludeCharProc.h"
#include "SboSvrRecvCmd.h"
#include <set>

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define DATACOUNTCOLUMN		(77)									/* 保存データ数 */

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
	"byAdminLV",				/* 管理権限レベル */
	"byCharState",				/* キャラ状態 */
	"byAccState",				/* アカウント状態 */
	"byDirection",				/* 向き */
	"byRoofLV",					/* 屋根レベル */
	"byNpcType",				/* NPC移動種別 */
	"bySaveFlg",				/* 保存用フラグ */
	"byPW",						/* 力 */
	"byDEX",					/* 素早さ */
	"byST",						/* 体力 */
	"byIT",						/* 賢さ */
	"byLK",						/* 運のよさ */
	"byJOB",					/* 職業 */
	"byLevel",					/* レベル */
	"byPutCount",				/* 最大NPC発生数 */
	"byAreaX",					/* 移動範囲（横） */
	"byAreaY",					/* 移動範囲（縦） */
	"byDropProbability",		/* ドロップ率 */
	"byMoveProbabilityX",		/* 移動確率X(NPC) */
	"byMoveProbabilityY",		/* 移動確率Y(NPC) */
	"bySearchCx",				/* 検索範囲X */
	"bySearchCy",				/* 検索範囲Y */
	"chAttrFire",				/* 属性(火) */
	"chAttrWater",				/* 属性(水) */
	"chAttrWind",				/* 属性(風) */
	"chAttrEarth",				/* 属性(土) */
	"chAttrLight",				/* 属性(光) */
	"chAttrDark",				/* 属性(闇) */
	"wGrpMain",					/* グラフィック種別(メイン) */
	"wGrpSub",					/* グラフィック種別(サブ) */
	"wAT",						/* 攻撃力 */
	"wDF",						/* 守備力 */
	"wMaxHP",					/* 最大HP */
	"wHP",						/* HP */
	"wMaxMana",					/* 最大Mana */
	"wMana",					/* Mana */
	"wGrpID",					/* グラフィックID */
	"wGrpIDBack",				/* グラフィックID記録用 */
	"wMakeYear",				/* キャラ作成年 */
	"wMakeMonth",				/* キャラ作成月 */
	"wMakeDay",					/* キャラ作成日 */
	"wTimePutInterval",			/* 発生周期 */
	"dwMapID",					/* マップID */
	"dwX",						/* X座標 */
	"dwY",						/* Y座標 */
	"dwStartMapID",				/* 開始マップID */
	"dwStartX",					/* 開始X座標 */
	"dwStartY",					/* 開始Y座標 */
	"dwLoginCount",				/* ログイン数 */
	"dwPlayTime",				/* 総プレイ時間 */
	"dwStTmplateID",			/* ステータステンプレートID */
	"dwGold",					/* 所持金 */
	"dwExp",					/* 経験値 */
	"dwLeaveID",				/* 預かり所ID */
	"clName",					/* 名前の色 */
	"clSpeech",					/* 発言の色 */
	"abyMagicCount",			/* 魔法数 */
	"abyMagic",					/* 魔法 */
	"waEquCount",				/* 装備品数 */
	"waEqu",					/* 装備品 */
	"waItemCount",				/* 所持アイテム数 */
	"waItem",					/* 所持アイテム */
	"szMacAdr",					/* MACアドレス */
	"szNameLen",				/* キャラ名長 */
	"szName",					/* キャラ名 */
	"szAccLen",					/* アカウント名長 */
	"szAcc",					/* アカウント名 */
	"szPasLen",					/* パスワード長 */
	"szPas",					/* パスワード */
	"szNpcSpeechLen",			/* NPC発言長 */
	"pszNpcSpeech",				/* NPC発言 */
	"dwLastIPAddress",			/* 最終IPアドレス */
	"abyJobMagic",				/* 転職前の職業で覚えていた呪文（職業ID別） */
	"byOptBgm",					/* 音楽ON/OFFのアカウント設定 */
	"abyNpcRoute",				/* NPCの巡回ルート */
	"dwNpcID",					/* NPC番号 */
	"abyTownWarp",				/* 町転送の行き先帳 */
	"z"
};


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
static BOOL CheckSaveChar(PCHARINFO);								/* キャラ情報を保存するかチェック */
static DWORD GetJobMagicSaveCount(PCHARINFO);						/* 職業別の呪文を何職業分保存するか */
BOOL SaveCharInfo(void);											/* キャラ情報保存 */
BOOL LoadCharInfo(void);											/* キャラ情報読み込み */
void UpdatePlayTime(PCHARINFO pChar);								/* ログイン中のプレイ時間を加算 */

static DWORD s_dwNpcIDMax = 0;									/* これまでに振った一番大きい NPC番号 */

/* ========================================================================= */
/* 関数名：CharIsSaveNpc													 */
/* 内容：NPC番号を持つ NPC か（保存される NPC。プレイヤー・発生した敵は除く）	 */
/* ========================================================================= */
static BOOL CharIsSaveNpc(PCHARINFO pChar)
{
	if ((pChar == NULL) || (pChar->byNpcType == NPCTYPE_INIT) || pChar->szAcc[0]) {
		return FALSE;
	}
	return (pChar->dwParentTblID == (DWORD)-1);
}

/* ========================================================================= */
/* 関数名：CharNpcIdEnsure													 */
/* 内容：NPC番号がまだ無ければ新しく振る										 */
/* ========================================================================= */
void CharNpcIdEnsure(PCHARINFO pChar)
{
	if (!CharIsSaveNpc (pChar)) {
		return;
	}
	/* 番号を指定して足し直した時（元に戻す）は、次に振る番号をその先へ */
	if (pChar->dwNpcID) {
		if (pChar->dwNpcID > s_dwNpcIDMax) {
			s_dwNpcIDMax = pChar->dwNpcID;
		}
		return;
	}
	pChar->dwNpcID = ++ s_dwNpcIDMax;
}

/* ========================================================================= */
/* 関数名：CharNpcIdRenumberAll												 */
/* 内容：読み込んだ後に、無い・重なった NPC番号を振り直す						 */
/* ========================================================================= */
void CharNpcIdRenumberAll(void)
{
	int i, nCount = g_aCharInfo.GetSize ();
	std::set<DWORD> aUsed;

	s_dwNpcIDMax = 0;
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (CharIsSaveNpc (pChar) && pChar->dwNpcID > s_dwNpcIDMax) {
			s_dwNpcIDMax = pChar->dwNpcID;
		}
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (!CharIsSaveNpc (pChar)) {
			if (pChar) {
				pChar->dwNpcID = 0;
			}
			continue;
		}
		if (pChar->dwNpcID && (aUsed.find (pChar->dwNpcID) == aUsed.end ())) {
			aUsed.insert (pChar->dwNpcID);
			continue;
		}
		pChar->dwNpcID = ++ s_dwNpcIDMax;
		aUsed.insert (pChar->dwNpcID);
	}
}

/* ========================================================================= */
/* 関数名：CharFindByNpcId													 */
/* 内容：NPC番号からキャラのテーブル番号を探す（無ければ -1）				 */
/* ========================================================================= */
int CharFindByNpcId(DWORD dwNpcID)
{
	int i, nCount = g_aCharInfo.GetSize ();

	if (dwNpcID == 0) {
		return -1;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar && (pChar->dwNpcID == dwNpcID) && CharIsSaveNpc (pChar)) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：UpdatePlayTime													 */
/* 内容：ログイン中のプレイ時間を総プレイ時間（分）へ加算する				 */
/*       1分に満たない端数は dwTimeLogin に残して次回へ持ち越す。			 */
/*       ログアウト時だけでなく保存の前にも呼ぶので、サーバーを止めたり		 */
/*       落ちたりしても、最後に保存した所までのプレイ時間は残る。			 */
/* ========================================================================= */
void UpdatePlayTime(PCHARINFO pChar)
{
	DWORD dwMinutes;

	if (pChar->dwTimeLogin == 0) {
		return;
	}
	dwMinutes = (timeGetTime () - pChar->dwTimeLogin) / 60000;
	if (dwMinutes == 0) {
		return;
	}
	pChar->dwPlayTime	+= dwMinutes;
	pChar->dwTimeLogin	+= dwMinutes * 60000;
	if (pChar->dwTimeLogin == 0) {
		/* 0 は「ログインしていない」の意味なので避ける */
		pChar->dwTimeLogin = 1;
	}
}

/* ========================================================================= */
/* 関数名：CheckSaveChar													 */
/* 内容：キャラ情報を保存するかチェック										 */
/* 日付：2004/04/18															 */
/* ========================================================================= */
/* TRUE なら NPC だけ保存する（公開用の初期データを作る時。server_main.cpp の --save-npc-only） */
BOOL g_bSaveNpcOnly = FALSE;

BOOL CheckSaveChar(PCHARINFO pChar)
{
	BOOL bRet = TRUE;
	if (g_bSaveNpcOnly && ((pChar->byNpcType == 0) || (pChar->byNpcType >= NPCTYPE_MAX))) {
		/* プレイヤーのキャラは保存しない */
		return FALSE;
	}
	if ((pChar->byNpcType == NPCTYPE_BATTLE1) ||
		(pChar->byNpcType == NPCTYPE_BATTLE2) ||
		(pChar->byNpcType == NPCTYPE_USEMAGIC) ||
		(pChar->byNpcType == NPCTYPE_USEMAGICPARENT) ||
		(pChar->byNpcType == NPCTYPE_BOMB) ||
		(pChar->byNpcType == NPCTYPE_BOMBBLOCK) ||
		(pChar->byNpcType == NPCTYPE_BOMBITEM) ||
		(pChar->byNpcType == NPCTYPE_MERCENARY)) {
		/* 保存しない（傭兵は雇っている間だけ） */
		bRet = FALSE;

	} else if ((pChar->byNpcType == 0) || (pChar->byNpcType >= NPCTYPE_MAX)) {
		/* アカウント名無し？ */
		if ((pChar->szAcc[0] == 0) || (strlen (pChar->szAcc) == 0)) {
			bRet = FALSE;
		}
	}
	return bRet;
}

/* ========================================================================= */
/* 関数名：GetJobMagicSaveCount												 */
/* 内容：職業別の呪文を何職業分保存するか（最後の空でない職業まで）			 */
/*       保存形式は キャラごとに [職業数(BYTE)][1職業の呪文数(BYTE)][呪文...]	 */
/*       古いデータにはこのカラムが無いので、読み込み時は全て空のまま			 */
/* ========================================================================= */
static DWORD GetJobMagicSaveCount(PCHARINFO pChar)
{
	int i, j;

	for (i = MAX_JOBMAGIC - 1; i >= 0; i --) {
		for (j = 0; j < MAX_USEMAGIC; j ++) {
			if (pChar->abyJobMagic[i][j]) {
				return (DWORD)(i + 1);
			}
		}
	}
	return 0;
}

/* ========================================================================= */
/* 関数名：SaveCharInfo														 */
/* 内容：キャラ情報保存														 */
/* 日付：2003/06/12															 */
/* ========================================================================= */
BOOL SaveCharInfo(void)
{
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	char szFileName[MAX_PATH + 1];
	BYTE byTmp;
	DWORD i, j, dwCount, dwColumnCount, dwDataCount, dwDataCountSave, dwHeaderSize,
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
	strcat (szFileName, "\\SboDataCharInfo.dat");


	/*************************
		データの初期設定
	*************************/

	bRet = FALSE;
	pData = NULL;
	hFile = INVALID_HANDLE_VALUE;
	dwColumnCount = DATACOUNTCOLUMN;
	dwDataCount = (DWORD)g_aCharInfo.GetSize ();

	dwDataCountSave = 0;
	for (j = 0; j < dwDataCount; j ++) {
		PCHARINFO pInfo;
		BOOL bResult;

		pInfo = g_aCharInfo[j];
		bResult = CheckSaveChar (pInfo);
		if (bResult == FALSE) {
			continue;
		}
		/* 管理者ウィンドウなどで足した NPC にも番号を付けてから保存する */
		CharNpcIdEnsure (pInfo);
		/* ログイン中のプレイヤーは、ここまでのプレイ時間を加算してから保存する */
		if ((pInfo->byNpcType == 0) && pInfo->pID) {
			UpdatePlayTime (pInfo);
		}
		dwDataCountSave ++;
	}


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
			PCHARINFO pInfo;
			BOOL bResult;

			pInfo = g_aCharInfo[j];
			bResult = CheckSaveChar (pInfo);
			if (bResult == FALSE) {
				continue;
			}
			switch (i) {
			case 0:		dwDataSizeTmp += sizeof (pInfo->byAdminLV);				break;	/* 管理権限レベル */
			case 1:		dwDataSizeTmp += sizeof (pInfo->byCharState);			break;	/* キャラ状態 */
			case 2:		dwDataSizeTmp += sizeof (pInfo->byAccState);			break;	/* アカウント状態 */
			case 3:		dwDataSizeTmp += sizeof (pInfo->byDirection);			break;	/* 向き */
			case 4:		dwDataSizeTmp += sizeof (pInfo->byRoofLV);				break;	/* 屋根レベル */
			case 5:		dwDataSizeTmp += sizeof (pInfo->byNpcType);				break;	/* NPC移動種別 */
			case 6:		dwDataSizeTmp += sizeof (pInfo->bySaveFlg);				break;	/* 保存用フラグ */
			case 7:		dwDataSizeTmp += sizeof (pInfo->byPW);					break;	/* 力 */
			case 8:		dwDataSizeTmp += sizeof (pInfo->byDEX);					break;	/* 素早さ */
			case 9:		dwDataSizeTmp += sizeof (pInfo->byST);					break;	/* 体力 */
			case 10:	dwDataSizeTmp += sizeof (pInfo->byIT);					break;	/* 賢さ */
			case 11:	dwDataSizeTmp += sizeof (pInfo->byLK);					break;	/* 運のよさ */
			case 12:	dwDataSizeTmp += sizeof (pInfo->byJOB);					break;	/* 職業 */
			case 13:	dwDataSizeTmp += sizeof (pInfo->byLevel);				break;	/* レベル */
			case 14:	dwDataSizeTmp += sizeof (pInfo->byPutCount);			break;	/* 最大NPC発生数 */
			case 15:	dwDataSizeTmp += sizeof (pInfo->byAreaX);				break;	/* 移動範囲（横） */
			case 16:	dwDataSizeTmp += sizeof (pInfo->byAreaY);				break;	/* 移動範囲（縦） */
			case 17:	dwDataSizeTmp += sizeof (pInfo->byDropProbability);		break;	/* ドロップ率 */
			case 18:	dwDataSizeTmp += sizeof (pInfo->byMoveProbabilityX);	break;	/* 移動確率X(NPC) */
			case 19:	dwDataSizeTmp += sizeof (pInfo->byMoveProbabilityY);	break;	/* 移動確率Y(NPC) */
			case 20:	dwDataSizeTmp += sizeof (pInfo->bySearchCx);			break;	/* 検索範囲X */
			case 21:	dwDataSizeTmp += sizeof (pInfo->bySearchCy);			break;	/* 検索範囲Y */
			case 22:	dwDataSizeTmp += sizeof (pInfo->chAttrFire);			break;	/* 属性(火) */
			case 23:	dwDataSizeTmp += sizeof (pInfo->chAttrWater);			break;	/* 属性(水) */
			case 24:	dwDataSizeTmp += sizeof (pInfo->chAttrWind);			break;	/* 属性(風) */
			case 25:	dwDataSizeTmp += sizeof (pInfo->chAttrEarth);			break;	/* 属性(土) */
			case 26:	dwDataSizeTmp += sizeof (pInfo->chAttrLight);			break;	/* 属性(光) */
			case 27:	dwDataSizeTmp += sizeof (pInfo->chAttrDark);			break;	/* 属性(闇) */
			case 28:	dwDataSizeTmp += sizeof (pInfo->wGrpMain);				break;	/* グラフィック種別(メイン) */
			case 29:	dwDataSizeTmp += sizeof (pInfo->wGrpSub);				break;	/* グラフィック種別(サブ) */
			case 30:	dwDataSizeTmp += sizeof (pInfo->wAT);					break;	/* 攻撃力 */
			case 31:	dwDataSizeTmp += sizeof (pInfo->wDF);					break;	/* 守備力 */
			case 32:	dwDataSizeTmp += sizeof (pInfo->wMaxHP);				break;	/* 最大HP */
			case 33:	dwDataSizeTmp += sizeof (pInfo->wHP);					break;	/* HP */
			case 34:	dwDataSizeTmp += sizeof (pInfo->wMaxMana);				break;	/* 最大Mana */
			case 35:	dwDataSizeTmp += sizeof (pInfo->wMana);					break;	/* Mana */
			case 36:	dwDataSizeTmp += sizeof (pInfo->wGrpID);				break;	/* グラフィックID */
			case 37:	dwDataSizeTmp += sizeof (pInfo->wGrpIDBack);			break;	/* グラフィックID記録用 */
			case 38:	dwDataSizeTmp += sizeof (pInfo->wMakeYear);				break;	/* キャラ作成年 */
			case 39:	dwDataSizeTmp += sizeof (pInfo->wMakeMonth);			break;	/* キャラ作成月 */
			case 40:	dwDataSizeTmp += sizeof (pInfo->wMakeDay);				break;	/* キャラ作成日 */
			case 41:	dwDataSizeTmp += sizeof (pInfo->wTimePutInterval);		break;	/* 発生周期 */
			case 42:	dwDataSizeTmp += sizeof (pInfo->dwMapID);				break;	/* マップID */
			case 43:	dwDataSizeTmp += sizeof (pInfo->dwX);					break;	/* X座標 */
			case 44:	dwDataSizeTmp += sizeof (pInfo->dwY);					break;	/* Y座標 */
			case 45:	dwDataSizeTmp += sizeof (pInfo->dwStartMapID);			break;	/* 開始マップID */
			case 46:	dwDataSizeTmp += sizeof (pInfo->dwStartX);				break;	/* 開始X座標 */
			case 47:	dwDataSizeTmp += sizeof (pInfo->dwStartY);				break;	/* 開始Y座標 */
			case 48:	dwDataSizeTmp += sizeof (pInfo->dwLoginCount);			break;	/* ログイン数 */
			case 49:	dwDataSizeTmp += sizeof (pInfo->dwPlayTime);			break;	/* 総プレイ時間 */
			case 50:	dwDataSizeTmp += sizeof (pInfo->dwStTmplateID);			break;	/* ステータステンプレートID */
			case 51:	dwDataSizeTmp += sizeof (pInfo->dwGold);				break;	/* 所持金 */
			case 52:	dwDataSizeTmp += sizeof (pInfo->dwExp);					break;	/* 経験値 */
			case 53:	dwDataSizeTmp += sizeof (pInfo->dwLeaveID);				break;	/* 預かり所ID */
			case 54:	dwDataSizeTmp += sizeof (pInfo->clName);				break;	/* 名前の色 */
			case 55:	dwDataSizeTmp += sizeof (pInfo->clSpeech);				break;	/* 発言の色 */
			case 56:	dwDataSizeTmp += sizeof (BYTE);							break;	/* 魔法数 */
			case 57:	dwDataSizeTmp += sizeof (pInfo->abyMagic);				break;	/* 魔法 */
			case 58:	dwDataSizeTmp += sizeof (BYTE);							break;	/* 装備品数 */
			case 59:	dwDataSizeTmp += sizeof (pInfo->waEqu);					break;	/* 装備品 */
			case 60:	dwDataSizeTmp += sizeof (BYTE);							break;	/* 所持アイテム数 */
			case 61:	dwDataSizeTmp += sizeof (pInfo->waItem);				break;	/* 所持アイテム */
			case 62:	dwDataSizeTmp += sizeof (pInfo->szMacAdr);				break;	/* MACアドレス */
			case 63:	dwDataSizeTmp += sizeof (BYTE);							break;	/* キャラ名長 */
			case 64:	dwDataSizeTmp += (strlen (pInfo->szName) + 1);			break;	/* キャラ名 */
			case 65:	dwDataSizeTmp += sizeof (BYTE);							break;	/* アカウント名長 */
			case 66:	dwDataSizeTmp += (strlen (pInfo->szAcc) + 1);			break;	/* アカウント名 */
			case 67:	dwDataSizeTmp += sizeof (BYTE);							break;	/* パスワード長 */
			case 68:	dwDataSizeTmp += (strlen (pInfo->szPas) + 1);			break;	/* パスワード */
			case 69:	dwDataSizeTmp += sizeof (BYTE);							break;	/* NPC発言長 */
			case 70:		/* NPC発言 */
				if (pInfo->pszNpcSpeech) {
					dwDataSizeTmp += (strlen (pInfo->pszNpcSpeech) + 1);
				} else {
					dwDataSizeTmp += 1;
				}
				break;
			case 71:	dwDataSizeTmp += sizeof (DWORD);						break;	/* 最終IPアドレス */
			case 72:	/* 転職前の職業で覚えていた呪文 */
				dwDataSizeTmp += sizeof (BYTE) * 2 + GetJobMagicSaveCount (pInfo) * MAX_USEMAGIC;
				break;
			case 73:	dwDataSizeTmp += sizeof (pInfo->byOptBgm);				break;	/* 音楽ON/OFFのアカウント設定 */
			case 74:	/* NPCの巡回ルート [数(BYTE)] 数が1以上なら続けて [回り方][止まる秒数][(横,縦)×数] */
				dwDataSizeTmp += 1;
				if (pInfo->byRouteCount) {
					dwDataSizeTmp += 2 + pInfo->byRouteCount * 2;
				}
				break;
			case 75:	dwDataSizeTmp += sizeof (pInfo->dwNpcID);				break;	/* NPC番号 */
			case 76:	/* 町転送の行き先帳 [数(BYTE)] 続けて [マップID(DWORD)][横(BYTE)][縦(BYTE)]×数 */
				dwDataSizeTmp += 1 + min ((DWORD)pInfo->byTownWarpCount, (DWORD)MAX_TOWNWARP) * (sizeof (DWORD) + 2);
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
	CopyMemory (&pData[dwOffset], &dwDataCountSave, sizeof (dwDataCountSave));
	dwOffset += sizeof (dwDataCountSave);
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
			PCHARINFO pInfo;
			BOOL bResult;

			pInfo = g_aCharInfo[j];
			bResult = CheckSaveChar (pInfo);
			if (bResult == FALSE) {
				continue;
			}
			switch (i) {
			case 0:		/* 管理権限レベル */
				CopyMemory (&pData[dwOffset], &pInfo->byAdminLV, sizeof (pInfo->byAdminLV));
				dwOffset += sizeof (pInfo->byAdminLV);
				break;
			case 1:		/* キャラ状態 */
				CopyMemory (&pData[dwOffset], &pInfo->byCharState, sizeof (pInfo->byCharState));
				dwOffset += sizeof (pInfo->byCharState);
				break;
			case 2:		/* アカウント状態 */
				CopyMemory (&pData[dwOffset], &pInfo->byAccState, sizeof (pInfo->byAccState));
				dwOffset += sizeof (pInfo->byAccState);
				break;
			case 3:		/* 向き */
				CopyMemory (&pData[dwOffset], &pInfo->byDirection, sizeof (pInfo->byDirection));
				dwOffset += sizeof (pInfo->byDirection);
				break;
			case 4:		/* 屋根レベル */
				CopyMemory (&pData[dwOffset], &pInfo->byRoofLV, sizeof (pInfo->byRoofLV));
				dwOffset += sizeof (pInfo->byRoofLV);
				break;
			case 5:		/* NPC移動種別 */
				CopyMemory (&pData[dwOffset], &pInfo->byNpcType, sizeof (pInfo->byNpcType));
				dwOffset += sizeof (pInfo->byNpcType);
				break;
			case 6:		/* 保存用フラグ */
				CopyMemory (&pData[dwOffset], &pInfo->bySaveFlg, sizeof (pInfo->bySaveFlg));
				dwOffset += sizeof (pInfo->bySaveFlg);
				break;
			case 7:		/* 力 */
				CopyMemory (&pData[dwOffset], &pInfo->byPW, sizeof (pInfo->byPW));
				dwOffset += sizeof (pInfo->byPW);
				break;
			case 8:		/* 素早さ */
				CopyMemory (&pData[dwOffset], &pInfo->byDEX, sizeof (pInfo->byDEX));
				dwOffset += sizeof (pInfo->byDEX);
				break;
			case 9:		/* 体力 */
				CopyMemory (&pData[dwOffset], &pInfo->byST, sizeof (pInfo->byST));
				dwOffset += sizeof (pInfo->byST);
				break;
			case 10:	/* 賢さ */
				CopyMemory (&pData[dwOffset], &pInfo->byIT, sizeof (pInfo->byIT));
				dwOffset += sizeof (pInfo->byIT);
				break;
			case 11:	/* 運のよさ */
				CopyMemory (&pData[dwOffset], &pInfo->byLK, sizeof (pInfo->byLK));
				dwOffset += sizeof (pInfo->byLK);
				break;
			case 12:	/* 職業 */
				CopyMemory (&pData[dwOffset], &pInfo->byJOB, sizeof (pInfo->byJOB));
				dwOffset += sizeof (pInfo->byJOB);
				break;
			case 13:	/* レベル */
				CopyMemory (&pData[dwOffset], &pInfo->byLevel, sizeof (pInfo->byLevel));
				dwOffset += sizeof (pInfo->byLevel);
				break;
			case 14:	/* 最大NPC発生数 */
				CopyMemory (&pData[dwOffset], &pInfo->byPutCount, sizeof (pInfo->byPutCount));
				dwOffset += sizeof (pInfo->byPutCount);
				break;
			case 15:	/* 移動範囲（横） */
				CopyMemory (&pData[dwOffset], &pInfo->byAreaX, sizeof (pInfo->byAreaX));
				dwOffset += sizeof (pInfo->byAreaX);
				break;
			case 16:	/* 移動範囲（縦） */
				CopyMemory (&pData[dwOffset], &pInfo->byAreaY, sizeof (pInfo->byAreaY));
				dwOffset += sizeof (pInfo->byAreaY);
				break;
			case 17:	/* ドロップ率 */
				CopyMemory (&pData[dwOffset], &pInfo->byDropProbability, sizeof (pInfo->byDropProbability));
				dwOffset += sizeof (pInfo->byDropProbability);
				break;
			case 18:	/* 移動確率X(NPC) */
				CopyMemory (&pData[dwOffset], &pInfo->byMoveProbabilityX, sizeof (pInfo->byMoveProbabilityX));
				dwOffset += sizeof (pInfo->byMoveProbabilityX);
				break;
			case 19:	/* 移動確率Y(NPC) */
				CopyMemory (&pData[dwOffset], &pInfo->byMoveProbabilityY, sizeof (pInfo->byMoveProbabilityY));
				dwOffset += sizeof (pInfo->byMoveProbabilityY);
				break;
			case 20:	/* 検索範囲X */
				CopyMemory (&pData[dwOffset], &pInfo->bySearchCx, sizeof (pInfo->bySearchCx));
				dwOffset += sizeof (pInfo->bySearchCx);
				break;
			case 21:	/* 検索範囲Y */
				CopyMemory (&pData[dwOffset], &pInfo->bySearchCy, sizeof (pInfo->bySearchCy));
				dwOffset += sizeof (pInfo->bySearchCy);
				break;
			case 22:	/* 属性(火) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrFire, sizeof (pInfo->chAttrFire));
				dwOffset += sizeof (pInfo->chAttrFire);
				break;
			case 23:	/* 属性(水) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrWater, sizeof (pInfo->chAttrWater));
				dwOffset += sizeof (pInfo->chAttrWater);
				break;
			case 24:	/* 属性(風) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrWind, sizeof (pInfo->chAttrWind));
				dwOffset += sizeof (pInfo->chAttrWind);
				break;
			case 25:	/* 属性(土) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrEarth, sizeof (pInfo->chAttrEarth));
				dwOffset += sizeof (pInfo->chAttrEarth);
				break;
			case 26:	/* 属性(光) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrLight, sizeof (pInfo->chAttrLight));
				dwOffset += sizeof (pInfo->chAttrLight);
				break;
			case 27:	/* 属性(闇) */
				CopyMemory (&pData[dwOffset], &pInfo->chAttrDark, sizeof (pInfo->chAttrDark));
				dwOffset += sizeof (pInfo->chAttrDark);
				break;
			case 28:	/* グラフィック種別(メイン) */
				CopyMemory (&pData[dwOffset], &pInfo->wGrpMain, sizeof (pInfo->wGrpMain));
				dwOffset += sizeof (pInfo->wGrpMain);
				break;
			case 29:	/* グラフィック種別(サブ) */
				CopyMemory (&pData[dwOffset], &pInfo->wGrpSub, sizeof (pInfo->wGrpSub));
				dwOffset += sizeof (pInfo->wGrpSub);
				break;
			case 30:	/* 攻撃力 */
				CopyMemory (&pData[dwOffset], &pInfo->wAT, sizeof (pInfo->wAT));
				dwOffset += sizeof (pInfo->wAT);
				break;
			case 31:	/* 守備力 */
				CopyMemory (&pData[dwOffset], &pInfo->wDF, sizeof (pInfo->wDF));
				dwOffset += sizeof (pInfo->wDF);
				break;
			case 32:	/* 最大HP */
				CopyMemory (&pData[dwOffset], &pInfo->wMaxHP, sizeof (pInfo->wMaxHP));
				dwOffset += sizeof (pInfo->wMaxHP);
				break;
			case 33:	/* HP */
				CopyMemory (&pData[dwOffset], &pInfo->wHP, sizeof (pInfo->wHP));
				dwOffset += sizeof (pInfo->wHP);
				break;
			case 34:	/* 最大Mana */
				CopyMemory (&pData[dwOffset], &pInfo->wMaxMana, sizeof (pInfo->wMaxMana));
				dwOffset += sizeof (pInfo->wMaxMana);
				break;
			case 35:	/* Mana */
				CopyMemory (&pData[dwOffset], &pInfo->wMana, sizeof (pInfo->wMana));
				dwOffset += sizeof (pInfo->wMana);
				break;
			case 36:	/* グラフィックID */
				CopyMemory (&pData[dwOffset], &pInfo->wGrpID, sizeof (pInfo->wGrpID));
				dwOffset += sizeof (pInfo->wGrpID);
				break;
			case 37:	/* グラフィックID記録用 */
				CopyMemory (&pData[dwOffset], &pInfo->wGrpIDBack, sizeof (pInfo->wGrpIDBack));
				dwOffset += sizeof (pInfo->wGrpIDBack);
				break;
			case 38:	/* キャラ作成年 */
				CopyMemory (&pData[dwOffset], &pInfo->wMakeYear, sizeof (pInfo->wMakeYear));
				dwOffset += sizeof (pInfo->wMakeYear);
				break;
			case 39:	/* キャラ作成月 */
				CopyMemory (&pData[dwOffset], &pInfo->wMakeMonth, sizeof (pInfo->wMakeMonth));
				dwOffset += sizeof (pInfo->wMakeMonth);
				break;
			case 40:	/* キャラ作成日 */
				CopyMemory (&pData[dwOffset], &pInfo->wMakeDay, sizeof (pInfo->wMakeDay));
				dwOffset += sizeof (pInfo->wMakeDay);
				break;
			case 41:	/* 発生周期 */
				CopyMemory (&pData[dwOffset], &pInfo->wTimePutInterval, sizeof (pInfo->wTimePutInterval));
				dwOffset += sizeof (pInfo->wTimePutInterval);
				break;
			case 42:	/* マップID */
				CopyMemory (&pData[dwOffset], &pInfo->dwMapID, sizeof (pInfo->dwMapID));
				dwOffset += sizeof (pInfo->dwMapID);
				break;
			case 43:	/* X座標 */
				CopyMemory (&pData[dwOffset], &pInfo->dwX, sizeof (pInfo->dwX));
				dwOffset += sizeof (pInfo->dwX);
				break;
			case 44:	/* Y座標 */
				CopyMemory (&pData[dwOffset], &pInfo->dwY, sizeof (pInfo->dwY));
				dwOffset += sizeof (pInfo->dwY);
				break;
			case 45:	/* 開始マップID */
				CopyMemory (&pData[dwOffset], &pInfo->dwStartMapID, sizeof (pInfo->dwStartMapID));
				dwOffset += sizeof (pInfo->dwStartMapID);
				break;
			case 46:	/* 開始X座標 */
				CopyMemory (&pData[dwOffset], &pInfo->dwStartX, sizeof (pInfo->dwStartX));
				dwOffset += sizeof (pInfo->dwStartX);
				break;
			case 47:	/* 開始Y座標 */
				CopyMemory (&pData[dwOffset], &pInfo->dwStartY, sizeof (pInfo->dwStartY));
				dwOffset += sizeof (pInfo->dwStartY);
				break;
			case 48:	/* ログイン数 */
				CopyMemory (&pData[dwOffset], &pInfo->dwLoginCount, sizeof (pInfo->dwLoginCount));
				dwOffset += sizeof (pInfo->dwLoginCount);
				break;
			case 49:	/* 総プレイ時間 */
				CopyMemory (&pData[dwOffset], &pInfo->dwPlayTime, sizeof (pInfo->dwPlayTime));
				dwOffset += sizeof (pInfo->dwPlayTime);
				break;
			case 50:	/* ステータステンプレートID */
				CopyMemory (&pData[dwOffset], &pInfo->dwStTmplateID, sizeof (pInfo->dwStTmplateID));
				dwOffset += sizeof (pInfo->dwStTmplateID);
				break;
			case 51:	/* 所持金 */
				CopyMemory (&pData[dwOffset], &pInfo->dwGold, sizeof (pInfo->dwGold));
				dwOffset += sizeof (pInfo->dwGold);
				break;
			case 52:	/* 経験値 */
				CopyMemory (&pData[dwOffset], &pInfo->dwExp, sizeof (pInfo->dwExp));
				dwOffset += sizeof (pInfo->dwExp);
				break;
			case 53:	/* 預かり所ID */
				CopyMemory (&pData[dwOffset], &pInfo->dwLeaveID, sizeof (pInfo->dwLeaveID));
				dwOffset += sizeof (pInfo->dwLeaveID);
				break;
			case 54:	/* 名前の色 */
				CopyMemory (&pData[dwOffset], &pInfo->clName, sizeof (pInfo->clName));
				dwOffset += sizeof (pInfo->clName);
				break;
			case 55:	/* 発言の色 */
				CopyMemory (&pData[dwOffset], &pInfo->clSpeech, sizeof (pInfo->clSpeech));
				dwOffset += sizeof (pInfo->clSpeech);
				break;
			case 56:	/* 魔法数 */
				byTmp = sizeof (pInfo->abyMagic);
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 57:	/* 魔法 */
				CopyMemory (&pData[dwOffset], &pInfo->abyMagic, sizeof (pInfo->abyMagic));
				dwOffset += sizeof (pInfo->abyMagic);
				break;
			case 58:	/* 装備品数 */
				byTmp = sizeof (pInfo->waEqu) / sizeof (WORD);
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 59:	/* 装備品 */
				CopyMemory (&pData[dwOffset], &pInfo->waEqu, sizeof (pInfo->waEqu));
				dwOffset += sizeof (pInfo->waEqu);
				break;
			case 60:	/* 所持アイテム数 */
				byTmp = sizeof (pInfo->waItem) / sizeof (WORD);
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 61:	/* 所持アイテム */
				CopyMemory (&pData[dwOffset], &pInfo->waItem, sizeof (pInfo->waItem));
				dwOffset += sizeof (pInfo->waItem);
				break;
			case 62:	/* MACアドレス */
				CopyMemory (&pData[dwOffset], &pInfo->szMacAdr, sizeof (pInfo->szMacAdr));
				dwOffset += sizeof (pInfo->szMacAdr);
				break;
			case 63:	/* キャラ名長 */
				byTmp = strlen (pInfo->szName) + 1;
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 64:	/* キャラ名 */
				CopyMemory (&pData[dwOffset], pInfo->szName, strlen (pInfo->szName) + 1);
				dwOffset += (strlen (pInfo->szName) + 1);
				break;
			case 65:	/* アカウント名長 */
				byTmp = strlen (pInfo->szAcc) + 1;
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 66:	/* アカウント名 */
				CopyMemory (&pData[dwOffset], pInfo->szAcc, strlen (pInfo->szAcc) + 1);
				dwOffset += (strlen (pInfo->szAcc) + 1);
				break;
			case 67:	/* パスワード長 */
				byTmp = strlen (pInfo->szPas) + 1;
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 68:	/* パスワード */
				CopyMemory (&pData[dwOffset], pInfo->szPas, strlen (pInfo->szPas) + 1);
				dwOffset += (strlen (pInfo->szPas) + 1);
				break;
			case 69:	/* NPC発言長 */
				if (pInfo->pszNpcSpeech) {
					byTmp = strlen (pInfo->pszNpcSpeech) + 1;
				} else {
					byTmp = 1;
				}
				CopyMemory (&pData[dwOffset], &byTmp, sizeof (byTmp));
				dwOffset += sizeof (byTmp);
				break;
			case 70:	/* NPC発言 */
				if (pInfo->pszNpcSpeech) {
					CopyMemory (&pData[dwOffset], pInfo->pszNpcSpeech, strlen (pInfo->pszNpcSpeech) + 1);
					dwOffset += (strlen (pInfo->pszNpcSpeech) + 1);
				} else {
					pData[dwOffset] = 0;
					dwOffset ++;
				}
				break;
			case 71:	/* 最終IPアドレス */
				CopyMemory (&pData[dwOffset], &pInfo->dwLastIPAddress, sizeof (pInfo->dwLastIPAddress));
				dwOffset += sizeof (pInfo->dwLastIPAddress);
				break;
			case 72:	/* 転職前の職業で覚えていた呪文 */
				{
					DWORD dwJobCount = GetJobMagicSaveCount (pInfo);

					pData[dwOffset ++] = (BYTE)dwJobCount;		/* 職業数 */
					pData[dwOffset ++] = (BYTE)MAX_USEMAGIC;	/* 1職業の呪文数 */
					CopyMemory (&pData[dwOffset], pInfo->abyJobMagic, dwJobCount * MAX_USEMAGIC);
					dwOffset += dwJobCount * MAX_USEMAGIC;
				}
				break;
			case 73:	/* 音楽ON/OFFのアカウント設定 */
				pData[dwOffset ++] = pInfo->byOptBgm;
				break;
			case 74:	/* NPCの巡回ルート */
				pData[dwOffset ++] = pInfo->byRouteCount;
				if (pInfo->byRouteCount) {
					pData[dwOffset ++] = pInfo->byRouteMode;
					pData[dwOffset ++] = pInfo->byRouteWait;
					for (DWORD k = 0; k < pInfo->byRouteCount; k ++) {
						pData[dwOffset ++] = pInfo->abyRouteX[k];
						pData[dwOffset ++] = pInfo->abyRouteY[k];
					}
				}
				break;
			case 75:	/* NPC番号 */
				CopyMemory (&pData[dwOffset], &pInfo->dwNpcID, sizeof (pInfo->dwNpcID));
				dwOffset += sizeof (pInfo->dwNpcID);
				break;
			case 76:	/* 町転送の行き先帳 */
				{
					DWORD k, dwCount = min ((DWORD)pInfo->byTownWarpCount, (DWORD)MAX_TOWNWARP);

					pData[dwOffset ++] = (BYTE)dwCount;
					for (k = 0; k < dwCount; k ++) {
						CopyMemory (&pData[dwOffset], &pInfo->adwTownWarpMapID[k], sizeof (DWORD));
						dwOffset += sizeof (DWORD);
						pData[dwOffset ++] = pInfo->abyTownWarpX[k];
						pData[dwOffset ++] = pInfo->abyTownWarpY[k];
					}
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
/* 関数名：LoadCharInfo														 */
/* 内容：キャラ情報読み込み													 */
/* 日付：2003/06/12															 */
/* ========================================================================= */
BOOL LoadCharInfo(void)
{
	int k, nCountJob;
	char szFileName[MAX_PATH + 1];
	BYTE byTmp;
	BOOL bRet;
	PBYTE pData;
	HANDLE hFile;
	DWORD i, j, dwSize, dwBytes, dwOffset, dwDataCount, dwColumnCount;
	PJOBINFO pJobInfo;
	CmyArray<HEADERINFO, HEADERINFO>aHeaderInfo;
	CCharProcBase CharProcBase;

	/* ファイル名の作成 */
	GetModuleFileName (NULL, szFileName, MAX_PATH);
	for (k = strlen (szFileName) - 1; k > 0; k --) {
		if (szFileName[k] == '\\') {
			szFileName[k + 1] = 0;
			break;
		}
	}
	strcat (szFileName, "SBODATA\\SboDataCharInfo.dat");


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
		PCHARINFO pInfo;

		pInfo = new CHARINFO;
		ZeroMemory (pInfo, sizeof (CHARINFO));
		SetInitCharData (pInfo);
		AddChar (pInfo, FALSE);
	}

	dwColumnCount = aHeaderInfo.GetSize ();
	for (i = 0; i < dwColumnCount; i ++) {
		DWORD dwOffsetTmp;

		dwOffsetTmp = aHeaderInfo[i].dwOffset;
		for (j = 0; j < dwDataCount; j ++) {
			PCHARINFO pInfo;

			pInfo = g_aCharInfo[j];
			/* 管理権限レベル */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[0]) == 0) {
				CopyMemory (&pInfo->byAdminLV, &pData[dwOffsetTmp], sizeof (pInfo->byAdminLV));
				dwOffsetTmp += sizeof (pInfo->byAdminLV);
			}

			/* キャラ状態 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[1]) == 0) {
				CopyMemory (&pInfo->byCharState, &pData[dwOffsetTmp], sizeof (pInfo->byCharState));
				dwOffsetTmp += sizeof (pInfo->byCharState);
			}

			/* アカウント状態 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[2]) == 0) {
				CopyMemory (&pInfo->byAccState, &pData[dwOffsetTmp], sizeof (pInfo->byAccState));
				dwOffsetTmp += sizeof (pInfo->byAccState);
			}

			/* 向き */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[3]) == 0) {
				CopyMemory (&pInfo->byDirection, &pData[dwOffsetTmp], sizeof (pInfo->byDirection));
				dwOffsetTmp += sizeof (pInfo->byDirection);
			}

			/* 屋根レベル */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[4]) == 0) {
				CopyMemory (&pInfo->byRoofLV, &pData[dwOffsetTmp], sizeof (pInfo->byRoofLV));
				dwOffsetTmp += sizeof (pInfo->byRoofLV);
			}

			/* NPC移動種別 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[5]) == 0) {
				CopyMemory (&pInfo->byNpcType, &pData[dwOffsetTmp], sizeof (pInfo->byNpcType));
				dwOffsetTmp += sizeof (pInfo->byNpcType);
			}

			/* 保存用フラグ */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[6]) == 0) {
				CopyMemory (&pInfo->bySaveFlg, &pData[dwOffsetTmp], sizeof (pInfo->bySaveFlg));
				dwOffsetTmp += sizeof (pInfo->bySaveFlg);
			}

			/* 力 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[7]) == 0) {
				CopyMemory (&pInfo->byPW, &pData[dwOffsetTmp], sizeof (pInfo->byPW));
				dwOffsetTmp += sizeof (pInfo->byPW);
			}

			/* 素早さ */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[8]) == 0) {
				CopyMemory (&pInfo->byDEX, &pData[dwOffsetTmp], sizeof (pInfo->byDEX));
				dwOffsetTmp += sizeof (pInfo->byDEX);
			}

			/* 体力 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[9]) == 0) {
				CopyMemory (&pInfo->byST, &pData[dwOffsetTmp], sizeof (pInfo->byST));
				dwOffsetTmp += sizeof (pInfo->byST);
			}

			/* 賢さ */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[10]) == 0) {
				CopyMemory (&pInfo->byIT, &pData[dwOffsetTmp], sizeof (pInfo->byIT));
				dwOffsetTmp += sizeof (pInfo->byIT);
			}

			/* 運のよさ */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[11]) == 0) {
				CopyMemory (&pInfo->byLK, &pData[dwOffsetTmp], sizeof (pInfo->byLK));
				dwOffsetTmp += sizeof (pInfo->byLK);
			}

			/* 職業 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[12]) == 0) {
				CopyMemory (&pInfo->byJOB, &pData[dwOffsetTmp], sizeof (pInfo->byJOB));
				dwOffsetTmp += sizeof (pInfo->byJOB);
			}

			/* レベル */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[13]) == 0) {
				CopyMemory (&pInfo->byLevel, &pData[dwOffsetTmp], sizeof (pInfo->byLevel));
				dwOffsetTmp += sizeof (pInfo->byLevel);
			}

			/* 最大NPC発生数 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[14]) == 0) {
				CopyMemory (&pInfo->byPutCount, &pData[dwOffsetTmp], sizeof (pInfo->byPutCount));
				dwOffsetTmp += sizeof (pInfo->byPutCount);
			}

			/* 移動範囲（横） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[15]) == 0) {
				CopyMemory (&pInfo->byAreaX, &pData[dwOffsetTmp], sizeof (pInfo->byAreaX));
				dwOffsetTmp += sizeof (pInfo->byAreaX);
			}

			/* 移動範囲（縦） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[16]) == 0) {
				CopyMemory (&pInfo->byAreaY, &pData[dwOffsetTmp], sizeof (pInfo->byAreaY));
				dwOffsetTmp += sizeof (pInfo->byAreaY);
			}

			/* ドロップ率 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[17]) == 0) {
				CopyMemory (&pInfo->byDropProbability, &pData[dwOffsetTmp], sizeof (pInfo->byDropProbability));
				dwOffsetTmp += sizeof (pInfo->byDropProbability);
			}

			/* 移動確率X(NPC) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[18]) == 0) {
				CopyMemory (&pInfo->byMoveProbabilityX, &pData[dwOffsetTmp], sizeof (pInfo->byMoveProbabilityX));
				dwOffsetTmp += sizeof (pInfo->byMoveProbabilityX);
			}

			/* 移動確率Y(NPC) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[19]) == 0) {
				CopyMemory (&pInfo->byMoveProbabilityY, &pData[dwOffsetTmp], sizeof (pInfo->byMoveProbabilityY));
				dwOffsetTmp += sizeof (pInfo->byMoveProbabilityY);
			}

			/* 検索範囲X */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[20]) == 0) {
				CopyMemory (&pInfo->bySearchCx, &pData[dwOffsetTmp], sizeof (pInfo->bySearchCx));
				dwOffsetTmp += sizeof (pInfo->bySearchCx);
			}

			/* 検索範囲Y */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[21]) == 0) {
				CopyMemory (&pInfo->bySearchCy, &pData[dwOffsetTmp], sizeof (pInfo->bySearchCy));
				dwOffsetTmp += sizeof (pInfo->bySearchCy);
			}

			/* 属性(火) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[22]) == 0) {
				CopyMemory (&pInfo->chAttrFire, &pData[dwOffsetTmp], sizeof (pInfo->chAttrFire));
				dwOffsetTmp += sizeof (pInfo->chAttrFire);
			}

			/* 属性(水) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[23]) == 0) {
				CopyMemory (&pInfo->chAttrWater, &pData[dwOffsetTmp], sizeof (pInfo->chAttrWater));
				dwOffsetTmp += sizeof (pInfo->chAttrWater);
			}

			/* 属性(風) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[24]) == 0) {
				CopyMemory (&pInfo->chAttrWind, &pData[dwOffsetTmp], sizeof (pInfo->chAttrWind));
				dwOffsetTmp += sizeof (pInfo->chAttrWind);
			}

			/* 属性(土) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[25]) == 0) {
				CopyMemory (&pInfo->chAttrEarth, &pData[dwOffsetTmp], sizeof (pInfo->chAttrEarth));
				dwOffsetTmp += sizeof (pInfo->chAttrEarth);
			}

			/* 属性(光) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[26]) == 0) {
				CopyMemory (&pInfo->chAttrLight, &pData[dwOffsetTmp], sizeof (pInfo->chAttrLight));
				dwOffsetTmp += sizeof (pInfo->chAttrLight);
			}

			/* 属性(闇) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[27]) == 0) {
				CopyMemory (&pInfo->chAttrDark, &pData[dwOffsetTmp], sizeof (pInfo->chAttrDark));
				dwOffsetTmp += sizeof (pInfo->chAttrDark);
			}

			/* グラフィック種別(メイン) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[28]) == 0) {
				CopyMemory (&pInfo->wGrpMain, &pData[dwOffsetTmp], sizeof (pInfo->wGrpMain));
				dwOffsetTmp += sizeof (pInfo->wGrpMain);
			}

			/* グラフィック種別(サブ) */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[29]) == 0) {
				CopyMemory (&pInfo->wGrpSub, &pData[dwOffsetTmp], sizeof (pInfo->wGrpSub));
				dwOffsetTmp += sizeof (pInfo->wGrpSub);
			}

			/* 攻撃力 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[30]) == 0) {
				CopyMemory (&pInfo->wAT, &pData[dwOffsetTmp], sizeof (pInfo->wAT));
				dwOffsetTmp += sizeof (pInfo->wAT);
			}

			/* 守備力 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[31]) == 0) {
				CopyMemory (&pInfo->wDF, &pData[dwOffsetTmp], sizeof (pInfo->wDF));
				dwOffsetTmp += sizeof (pInfo->wDF);
			}

			/* 最大HP */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[32]) == 0) {
				CopyMemory (&pInfo->wMaxHP, &pData[dwOffsetTmp], sizeof (pInfo->wMaxHP));
				dwOffsetTmp += sizeof (pInfo->wMaxHP);
			}

			/* HP */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[33]) == 0) {
				CopyMemory (&pInfo->wHP, &pData[dwOffsetTmp], sizeof (pInfo->wHP));
				dwOffsetTmp += sizeof (pInfo->wHP);
			}

			/* 最大Mana */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[34]) == 0) {
				CopyMemory (&pInfo->wMaxMana, &pData[dwOffsetTmp], sizeof (pInfo->wMaxMana));
				dwOffsetTmp += sizeof (pInfo->wMaxMana);
			}

			/* Mana */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[35]) == 0) {
				CopyMemory (&pInfo->wMana, &pData[dwOffsetTmp], sizeof (pInfo->wMana));
				dwOffsetTmp += sizeof (pInfo->wMana);
			}

			/* グラフィックID */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[36]) == 0) {
				CopyMemory (&pInfo->wGrpID, &pData[dwOffsetTmp], sizeof (pInfo->wGrpID));
				dwOffsetTmp += sizeof (pInfo->wGrpID);
			}

			/* グラフィックID記録用 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[37]) == 0) {
				CopyMemory (&pInfo->wGrpIDBack, &pData[dwOffsetTmp], sizeof (pInfo->wGrpIDBack));
				dwOffsetTmp += sizeof (pInfo->wGrpIDBack);
			}

			/* キャラ作成年 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[38]) == 0) {
				CopyMemory (&pInfo->wMakeYear, &pData[dwOffsetTmp], sizeof (pInfo->wMakeYear));
				dwOffsetTmp += sizeof (pInfo->wMakeYear);
			}

			/* キャラ作成月 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[39]) == 0) {
				CopyMemory (&pInfo->wMakeMonth, &pData[dwOffsetTmp], sizeof (pInfo->wMakeMonth));
				dwOffsetTmp += sizeof (pInfo->wMakeMonth);
			}

			/* キャラ作成日 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[40]) == 0) {
				CopyMemory (&pInfo->wMakeDay, &pData[dwOffsetTmp], sizeof (pInfo->wMakeDay));
				dwOffsetTmp += sizeof (pInfo->wMakeDay);
			}

			/* 発生周期 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[41]) == 0) {
				CopyMemory (&pInfo->wTimePutInterval, &pData[dwOffsetTmp], sizeof (pInfo->wTimePutInterval));
				dwOffsetTmp += sizeof (pInfo->wTimePutInterval);
			}

			/* マップID */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[42]) == 0) {
				CopyMemory (&pInfo->dwMapID, &pData[dwOffsetTmp], sizeof (pInfo->dwMapID));
				dwOffsetTmp += sizeof (pInfo->dwMapID);
			}

			/* X座標 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[43]) == 0) {
				CopyMemory (&pInfo->dwX, &pData[dwOffsetTmp], sizeof (pInfo->dwX));
				dwOffsetTmp += sizeof (pInfo->dwX);
			}

			/* Y座標 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[44]) == 0) {
				CopyMemory (&pInfo->dwY, &pData[dwOffsetTmp], sizeof (pInfo->dwY));
				dwOffsetTmp += sizeof (pInfo->dwY);
			}

			/* 開始マップID */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[45]) == 0) {
				CopyMemory (&pInfo->dwStartMapID, &pData[dwOffsetTmp], sizeof (pInfo->dwStartMapID));
				dwOffsetTmp += sizeof (pInfo->dwStartMapID);
			}

			/* 開始X座標 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[46]) == 0) {
				CopyMemory (&pInfo->dwStartX, &pData[dwOffsetTmp], sizeof (pInfo->dwStartX));
				dwOffsetTmp += sizeof (pInfo->dwStartX);
			}

			/* 開始Y座標 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[47]) == 0) {
				CopyMemory (&pInfo->dwStartY, &pData[dwOffsetTmp], sizeof (pInfo->dwStartY));
				dwOffsetTmp += sizeof (pInfo->dwStartY);
			}

			/* ログイン数 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[48]) == 0) {
				CopyMemory (&pInfo->dwLoginCount, &pData[dwOffsetTmp], sizeof (pInfo->dwLoginCount));
				dwOffsetTmp += sizeof (pInfo->dwLoginCount);
			}

			/* 総プレイ時間 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[49]) == 0) {
				CopyMemory (&pInfo->dwPlayTime, &pData[dwOffsetTmp], sizeof (pInfo->dwPlayTime));
				dwOffsetTmp += sizeof (pInfo->dwPlayTime);
			}

			/* ステータステンプレートID */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[50]) == 0) {
				CopyMemory (&pInfo->dwStTmplateID, &pData[dwOffsetTmp], sizeof (pInfo->dwStTmplateID));
				dwOffsetTmp += sizeof (pInfo->dwStTmplateID);
			}

			/* 所持金 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[51]) == 0) {
				CopyMemory (&pInfo->dwGold, &pData[dwOffsetTmp], sizeof (pInfo->dwGold));
				dwOffsetTmp += sizeof (pInfo->dwGold);
			}

			/* 経験値 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[52]) == 0) {
				CopyMemory (&pInfo->dwExp, &pData[dwOffsetTmp], sizeof (pInfo->dwExp));
				dwOffsetTmp += sizeof (pInfo->dwExp);
			}

			/* 預かり所ID */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[53]) == 0) {
				CopyMemory (&pInfo->dwLeaveID, &pData[dwOffsetTmp], sizeof (pInfo->dwLeaveID));
				dwOffsetTmp += sizeof (pInfo->dwLeaveID);
			}

			/* 名前の色 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[54]) == 0) {
				CopyMemory (&pInfo->clName, &pData[dwOffsetTmp], sizeof (pInfo->clName));
				dwOffsetTmp += sizeof (pInfo->clName);
			}

			/* 発言の色 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[55]) == 0) {
				CopyMemory (&pInfo->clSpeech, &pData[dwOffsetTmp], sizeof (pInfo->clSpeech));
				dwOffsetTmp += sizeof (pInfo->clSpeech);
			}

			/* 魔法数 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[56]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* 魔法 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[57]) == 0) {
				CopyMemory (&pInfo->abyMagic, &pData[dwOffsetTmp], sizeof (pInfo->abyMagic));
				dwOffsetTmp += sizeof (pInfo->abyMagic);
			}

			/* 装備品数 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[58]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* 装備品 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[59]) == 0) {
				CopyMemory (&pInfo->waEqu, &pData[dwOffsetTmp], sizeof (pInfo->waEqu));
				dwOffsetTmp += sizeof (pInfo->waEqu);
			}

			/* 所持アイテム数 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[60]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* 所持アイテム */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[61]) == 0) {
				CopyMemory (&pInfo->waItem, &pData[dwOffsetTmp], sizeof (pInfo->waItem));
				dwOffsetTmp += sizeof (pInfo->waItem);
			}

			/* MACアドレス */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[62]) == 0) {
				CopyMemory (&pInfo->szMacAdr, &pData[dwOffsetTmp], sizeof (pInfo->szMacAdr));
				dwOffsetTmp += sizeof (pInfo->szMacAdr);
			}

			/* キャラ名長 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[63]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* キャラ名 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[64]) == 0) {
				strcpy (pInfo->szName, (LPCSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen (pInfo->szName) + 1);
			}

			/* アカウント名長 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[65]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* アカウント名 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[66]) == 0) {
				strcpy (pInfo->szAcc, (LPCSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen (pInfo->szAcc) + 1);
				if (strcmp (pInfo->szAcc, "administrator") == 0) {
					int a=0;
				}
			}

			/* パスワード長 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[67]) == 0) {
				CopyMemory (&byTmp, &pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* パスワード */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[68]) == 0) {
				strcpy (pInfo->szPas, (LPCSTR)&pData[dwOffsetTmp]);
				dwOffsetTmp += (strlen (pInfo->szPas) + 1);
			}

			/* NPC発言長 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[69]) == 0) {
				CopyMemory (&byTmp, (LPCSTR)&pData[dwOffsetTmp], sizeof (byTmp));
				dwOffsetTmp += sizeof (byTmp);
			}

			/* NPC発言 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[70]) == 0) {
				int nLen;

				nLen = strlen ((LPCSTR)&pData[dwOffsetTmp]);
				if (nLen > 0) {
					pInfo->pszNpcSpeech = new char[nLen + 1];
					strcpy (pInfo->pszNpcSpeech, (LPCSTR)&pData[dwOffsetTmp]);
					dwOffsetTmp += (strlen (pInfo->pszNpcSpeech) + 1);
				} else {
					dwOffsetTmp ++;
				}
			}

			/* 最終IPアドレス */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[71]) == 0) {
				CopyMemory (&pInfo->dwLastIPAddress, &pData[dwOffsetTmp], sizeof (pInfo->dwLastIPAddress));
				dwOffsetTmp += sizeof (pInfo->dwLastIPAddress);
			}

			/* 転職前の職業で覚えていた呪文 */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[72]) == 0) {
				DWORD dwJobCount, dwMagicCount, dwJob;

				dwJobCount   = pData[dwOffsetTmp ++];
				dwMagicCount = pData[dwOffsetTmp ++];
				/* 保存時と定数が変わっていても読めるように、収まる分だけ取り込む */
				for (dwJob = 0; dwJob < dwJobCount; dwJob ++) {
					if (dwJob < MAX_JOBMAGIC) {
						CopyMemory (pInfo->abyJobMagic[dwJob], &pData[dwOffsetTmp], min (dwMagicCount, (DWORD)MAX_USEMAGIC));
					}
					dwOffsetTmp += dwMagicCount;
				}
			}

			/* 音楽ON/OFFのアカウント設定（無い古いデータは「まだ決めていない」のまま） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[73]) == 0) {
				pInfo->byOptBgm = pData[dwOffsetTmp ++];
				if (pInfo->byOptBgm > OPTBGM_OFF) {
					pInfo->byOptBgm = OPTBGM_NONE;
				}
			}

			/* NPCの巡回ルート（無い古いデータは巡回しない） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[74]) == 0) {
				DWORD dwCount = pData[dwOffsetTmp ++];
				if (dwCount) {
					pInfo->byRouteMode = pData[dwOffsetTmp ++];
					pInfo->byRouteWait = pData[dwOffsetTmp ++];
					if (pInfo->byRouteMode >= NPCROUTEMODE_MAX) {
						pInfo->byRouteMode = NPCROUTEMODE_BACK;
					}
					for (DWORD k = 0; k < dwCount; k ++) {
						if (k < MAX_NPCROUTE) {
							pInfo->abyRouteX[k] = pData[dwOffsetTmp];
							pInfo->abyRouteY[k] = pData[dwOffsetTmp + 1];
						}
						dwOffsetTmp += 2;
					}
					pInfo->byRouteCount = (BYTE)min (dwCount, (DWORD)MAX_NPCROUTE);
				}
			}

			/* NPC番号（無い古いデータは 0 のまま。下の補正で振る） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[75]) == 0) {
				CopyMemory (&pInfo->dwNpcID, &pData[dwOffsetTmp], sizeof (pInfo->dwNpcID));
				dwOffsetTmp += sizeof (pInfo->dwNpcID);
			}

			/* 町転送の行き先帳（無い古いデータは空のまま。定数が減っても読めるよう収まる分だけ） */
			if (strcmp (aHeaderInfo[i].szName, s_aszName[76]) == 0) {
				DWORD k, dwCount = pData[dwOffsetTmp ++];

				pInfo->byTownWarpCount = 0;
				for (k = 0; k < dwCount; k ++) {
					if (k < MAX_TOWNWARP) {
						CopyMemory (&pInfo->adwTownWarpMapID[k], &pData[dwOffsetTmp], sizeof (DWORD));
						pInfo->abyTownWarpX[k] = pData[dwOffsetTmp + sizeof (DWORD)];
						pInfo->abyTownWarpY[k] = pData[dwOffsetTmp + sizeof (DWORD) + 1];
						pInfo->byTownWarpCount ++;
					}
					dwOffsetTmp += sizeof (DWORD) + 2;
				}
			}
		}
	}

	/* NPC番号を振る（古いデータで 0 の NPC や、重なった番号を直す） */
	CharNpcIdRenumberAll ();

	/* 各情報の補正 */
	nCountJob = g_JobInfo.GetCount ();
	for (i = 0; i < dwDataCount; i ++) {
		PCHARINFO pInfo;

		pInfo = g_aCharInfo[i];
		CharProcBase.SetProc (pInfo);
		if (pInfo->byNpcType == 0) {
			if (pInfo->wMaxHP == 0) {
				SetInitCharData (pInfo);
			}
			/* 透明？ */
			if ((pInfo->wGrpMain == CHARGRP_MAIN_NORMAL) && (pInfo->wGrpID == 0)) {
				pInfo->wGrpID = 2;
				pInfo->wGrpIDBack = 2;
				pInfo->wGrpSub = 0;
			}
		}

		if (pInfo->byNpcType > 0) {
			pInfo->byGameState = GAMESTATE_NPC;
			pInfo->pID = i + 1;
		}

		if (pInfo->wGrpIDBack == 0) {
			pInfo->wGrpIDBack = pInfo->wGrpID;
		}
		/* 戦場用画像？ */
		if (pInfo->wGrpMain == CHARGRP_MAIN_SUBWAR) {
			pInfo->wGrpMain = CHARGRP_MAIN_NORMAL;
			pInfo->wGrpID = pInfo->wGrpIDBack;
			pInfo->wGrpSub = 0;
		}
		if (pInfo->wMana > pInfo->wMaxMana) {
			pInfo->wMana = pInfo->wMaxMana;
		}
		if (pInfo->wHP > pInfo->wMaxHP) {
			pInfo->wHP = pInfo->wMaxHP;
		}

		if (pInfo->byJOB >= nCountJob) {
			pInfo->byJOB = 0;
		}
		if (pInfo->byLevel == 0) {
			pInfo->byLevel = 1;
		}
#if 0	//強制変更用
		if (pInfo->byNpcType == 0) {
			if (pInfo->dwStartMapID == 98) {
				pInfo->dwStartMapID = 0;
				pInfo->dwStartX = 35;
				pInfo->dwStartY = 30;
			}
			if ((pInfo->dwMapID == 98)) {
					pInfo->dwMapID = 0;
					pInfo->dwX = 26;
					pInfo->dwY = 38;
			}
		}
#endif
		if (pInfo->byNpcType == 0) {
			pJobInfo = g_JobInfo.GetPtr ((int)pInfo->byJOB);
			if (pJobInfo) {
				int nMaxLevel;

				/* 能力値は「次のレベルの上限」で切り詰めない。
				   （元は起動のたびに切り詰めていて、種や管理者で上げた分が
				     再起動で消えていた。HP・Mana が最大を超えない補正は上で済み） */
				nMaxLevel = pJobInfo->aLevelUpInfo.GetSize ();
				if (!(nMaxLevel > 1 && pInfo->byLevel < nMaxLevel)) {
					pInfo->byLevel = nMaxLevel;
				}
			}
		}
		pInfo->dwXBack = pInfo->dwX;
		pInfo->dwYBack = pInfo->dwY;
//		MyOutputDebugString ("%s[%s]\r\n", pInfo->szAcc, pInfo->szPas);
		RenewATDF (pInfo->dwTblID);
	}
#if 0
	{
		int ii, nCount;
		PCHARINFO pInfo;

		nCount = g_aCharInfo.GetSize ();
		for (ii = nCount - 1; ii >= 0; ii --) {
			pInfo = g_aCharInfo[ii];
			if (pInfo->byNpcType == 0) {
				DeleteChar (pInfo->dwTblID);
			}
		}
	}
#endif
	RenewTblNoChar ();

	bRet = TRUE;
Exit:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle (hFile);
	}
	SAFE_DELETE_ARRAY (pData);
	return bRet;
}

/* Copyright(C)URARA-works 2003-2005 */
