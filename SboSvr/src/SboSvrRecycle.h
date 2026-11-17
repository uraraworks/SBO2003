/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrRecycle.h												 */
/* 概要：		リサイクルボックス（おすそわけの共有の箱）の処理(定義ファイル) */
/* 作成日：		2026/11/17													 */
/* ========================================================================= */

#pragma once

#include <vector>

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* 箱の中身の 1 種類（箱は NPC番号 dwNpcID ごと） */
typedef struct _RECYCLESTOCK {
	DWORD		dwNpcID;											/* 箱の NPC番号（CHARINFO の dwNpcID。再起動しても変わらない） */
	WORD		wItemID;											/* 品の種類ID */
	BYTE		byCount;											/* 個数（1〜RECYCLE_MAXCOUNT） */
} RECYCLESTOCK, *PRECYCLESTOCK;

/* アカウントが今日もらった個数（日付は日本時間の日。古い日付の行は 0 個として扱う） */
typedef struct _RECYCLEDAILY {
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	DWORD		dwDay;												/* 日付（日本時間で 1970/1/1 から数えた日数） */
	BYTE		byCount;											/* その日にもらった個数 */
} RECYCLEDAILY, *PRECYCLEDAILY;

/* 記録の種類 */
enum {
	RECYCLELOG_PUT = 0,											/* 入れた */
	RECYCLELOG_TAKE,											/* もらった */
};

/* 記録の 1 件（誰が・いつ・どの箱で・何を何個入れた／もらった） */
typedef struct _RECYCLELOG {
	DWORD		dwTime;												/* 時刻（time()） */
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	DWORD		dwNpcID;											/* 箱の NPC番号 */
	BYTE		byKind;												/* 入れた／もらった(RECYCLELOG_*) */
	WORD		wItemID;											/* 品の種類ID */
	BYTE		byCount;											/* 個数 */
} RECYCLELOG, *PRECYCLELOG;

/* ========================================================================= */
/* 変数・関数プロトタイプ宣言												 */
/* ========================================================================= */
extern std::vector<RECYCLESTOCK> g_aRecycleStock;				/* 箱の中身（入れた順） */
extern std::vector<RECYCLEDAILY> g_aRecycleDaily;				/* 今日もらった個数 */
extern std::vector<RECYCLELOG> g_aRecycleLog;					/* 記録（古い順。RECYCLE_LOGMAX 件まで） */
extern DWORD g_dwRecycleNowOverride;							/* 0 以外ならこれを「今の時刻」にする（テスト用） */

extern void RecycleClear(void);									/* 全部消す（終了時・読み込み前） */
extern DWORD RecycleNowSec(void);								/* 今の時刻（秒） */
extern DWORD RecycleDay(DWORD dwSec);							/* 時刻から日本時間の日付（日数）を求める */
extern int RecycleLeft(LPCSTR pszAcc);							/* アカウントが今日あと何個もらえるか */
extern int RecycleKindCount(DWORD dwNpcID);						/* 箱に入っている種類の数 */
extern int RecycleStockCount(DWORD dwNpcID, WORD wItemID);		/* 箱にあるその品の個数 */
extern void RecycleTrimLog(void);								/* 記録を RECYCLE_LOGMAX 件に切る（読み込み後） */
extern BOOL RecycleIsBox(PCHARINFO pNpc);						/* 台詞が "@d" の箱（NPC番号あり）か */
extern PCHARINFO RecycleFindBox(PCHARINFO pChar, DWORD dwNpcTblID);	/* 話しかけた箱（近くにいる時だけ。無ければ NULL） */

extern void RecycleList(PCHARINFO pChar, DWORD dwNpcTblID);		/* 中身を送る */
extern void RecyclePut(PCHARINFO pChar, DWORD dwNpcTblID, WORD wItemID, BYTE byCount);	/* 入れる */
extern void RecycleTake(PCHARINFO pChar, DWORD dwNpcTblID, WORD wItemID);	/* もらう（1 個） */

/* Copyright(C)URARA-works 2003-2005 */
