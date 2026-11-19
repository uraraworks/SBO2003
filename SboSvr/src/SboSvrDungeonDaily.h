/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrDungeonDaily.h										 */
/* 概要：		日替わりダンジョンの版と最短記録(定義ファイル)				 */
/* 作成日：		2026/11/19													 */
/* ========================================================================= */

#pragma once

#include <string>
#include <vector>
#include <time.h>

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* 最短記録の 1 件（組が入ってから、誰かが最初にゴールのワープに乗るまで） */
typedef struct _DGDAILYREC {
	std::vector<std::string>	aName;								/* 組のキャラ名（入った時の全員） */
	std::vector<std::string>	aForm;								/* 選んだ姿の名前（aName と同じ順） */
	std::vector<DWORD>	aTbl;										/* 組のキャラのテーブルID（小さい順。同じ顔ぶれの組は 1 件だけ残す印。空なら比べない） */
	int			nCount;												/* 人数 */
	DWORD		dwMs;												/* クリア時間（ミリ秒） */
	std::string	strAt;												/* 日時（日本時間 "YYYY-MM-DD HH:MM:SS"） */
	_DGDAILYREC() : nCount(0), dwMs(0) {}
} DGDAILYREC;

/* 版（日付・作り直し番号・シード・テーマの写し・生成器の版・生成設定）。記録は版ごと */
typedef struct _DGDAILYVER {
	DWORD		dwNo;												/* 版の通し番号（ダンジョンごと。1 から） */
	std::string	strDate;											/* 日付（日本時間 "YYYY-MM-DD"） */
	int			nRebuild;											/* その日の作り直し番号（0=日付から決めたシード。管理者の作り直しで 1, 2, ...） */
	DWORD		dwSeed;												/* 生成のシード */
	int			nGenVer;											/* 生成器の版番号（DungeonGen::GEN_VERSION） */
	DWORD		dwThemeID;											/* テーマ番号 */
	int			nW, nH;												/* 生成の大きさ（範囲の大きさ） */
	int			nThemeRef;											/* テーマの写しの番号（DungeonDailyThemeText） */
	std::string	strCreated;											/* 版を作った日時（日本時間） */
	std::vector<DGDAILYREC>	aRec;									/* 最短記録（時間の短い順。DGDAILY_RECMAX 件まで） */
	_DGDAILYVER() : dwNo(0), nRebuild(0), dwSeed(0), nGenVer(0), dwThemeID(0), nW(0), nH(0), nThemeRef(-1) {}
} DGDAILYVER;

#define DGDAILY_RECMAX		20										/* 版ごとに残す記録の数 */

/* 版の決め方 */
enum {
	DGDAILY_NORMAL = 0,												/* 今日の版があればそれを使う（再起動・日付が変わった時） */
	DGDAILY_ADMIN,													/* 管理者の「今すぐ作り直す」（新しいランダムなシードの版を足す） */
};

/* ========================================================================= */
/* 変数・関数プロトタイプ宣言												 */
/* ========================================================================= */
extern time_t g_tDungeonDailyNow;									/* 0 以外ならこれを「今の時刻」にする（テスト用） */

extern time_t DungeonDailyNow(void);								/* 今の時刻 */
extern std::string DungeonDailyDate(time_t t);						/* 日本時間の日付 "YYYY-MM-DD" */
extern std::string DungeonDailyDateTime(time_t t);					/* 日本時間の日時 "YYYY-MM-DD HH:MM:SS" */
extern DWORD DungeonDailySeed(const std::string &strDate, int nDungeon);	/* その日のシード（日付とダンジョン番号から） */
extern DWORD DungeonDailyPutSeed(DWORD dwVerSeed, DWORD dwParentTblID, DWORD dwTry);	/* 敵の出る位置の乱数のシード（版のシード・発生元の番号・何回目の試しか） */

/* 版を決める（pszFileTheme は今のテーマファイルの該当テーマ 1 件の写し。無い時は NULL か空）。使える版を out に入れて TRUE。
   NORMAL: 今日の版があって、生成器の版・テーマ番号・大きさが今と同じならそれ。違えば新しい版（シード・作り直し番号は今日の版から引き継ぐ）。
   ADMIN: 新しいランダムなシードの版を足す。新しい版にはテーマの写しが要る（pszFileTheme が無く足せない時は FALSE） */
extern BOOL DungeonDailyAcquire(int nDungeon, DWORD dwThemeID, int nW, int nH, int nHow, const char *pszFileTheme, DGDAILYVER &out);
extern BOOL DungeonDailyFind(int nDungeon, DWORD dwNo, DGDAILYVER &out);		/* 版を探す */
extern std::string DungeonDailyThemeText(int nRef);					/* テーマの写し（正規化した JSON。無ければ ""） */
extern BOOL DungeonDailyStale(int nDungeon, DWORD dwVerNo, DWORD dwThemeID, int nW, int nH);	/* 今の版を作り直す必要があるか（版が無い・日付が変わった・生成器の版/テーマ番号/大きさが違う） */
extern int DungeonDailyAddRecord(int nDungeon, DWORD dwVerNo, const DGDAILYREC &Rec, BOOL *pbBetter = NULL);	/* 記録を足す。順位（1〜DGDAILY_RECMAX）か、入らなければ 0。同じ顔ぶれ（aTbl が同じ）の組が同じ版で記録済みなら、速い方だけ残す（遅い・同じ時間なら足さず、今ある記録の順位を返して *pbBetter を FALSE に） */
extern std::string DungeonDailyTopText(int nDungeon, DWORD dwVerNo);	/* 入口で知らせる「今日の 1〜3 位」の文（版が無ければ ""） */
extern std::string DungeonDailyTimeText(DWORD dwMs);				/* "X分Y秒" */
extern std::string DungeonDailyVersionsJson(int nDungeon, DWORD dwVerNo, BOOL bTheme);	/* 版の配列の JSON（dwVerNo が 0 以外ならその版だけ。bTheme ならテーマの写しも入れる） */
extern int DungeonDailyGenVersion(void);							/* 今の生成器の版番号（DungeonGen::GEN_VERSION） */
extern int DungeonDailyVersionCount(int nDungeon);					/* 版の数 */
extern void DungeonDailyReset(BOOL bDeleteFile);					/* 覚えているものを全部捨てる（bDeleteFile ならファイルも消す。次に使う時はファイルから読む。テスト用） */
extern void DungeonDailyReload(void);								/* 覚えているものを捨てて、次に使う時にファイルから読み直す（テスト用） */
