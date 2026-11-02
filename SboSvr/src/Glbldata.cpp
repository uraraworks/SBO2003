/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			グローバル変数定義										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */
#include "include.h"
#include "../../Common/Info/InfoEfc.h"
#include "../../Common/Info/InfoJob.h"
#include "../../Common/LibInfo/LibInfoItem.h"
#include "Proc/ProcSubGame.h"

/* libファイルのパス */
#define LIBCMNPATH	"../Common/myZlib/"

/* コンパイルオプション別のリンク設定 */
#ifdef _DEBUG
	#pragma comment(lib, LIBCMNPATH "zlibD.lib")
#else
	#pragma comment(lib, LIBCMNPATH "zlib.lib")
#endif

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
CUraraSockTCP svrSock;												/* サーバーオブジェクト */
CTextOutput g_LogLogin;												/* ログインログ */

HINSTANCE g_hInst;													/* インスタンスハンドル */
HWND g_hWndMain;													/* メインウィンドウハンドル */

WORD g_wPort,														/* 待ち受けポート番号 */
	 g_wCountMapParts;												/* 使用マップパーツ数 */

DWORD g_dwTime,														/* 現在時刻 */
	  g_dwOnline,													/* オンライン数 */
	  g_dwOnlineBack[24],											/* 時間毎の最大人数(稼動中) */
	  g_dwOnlineBackMax[24],										/* 時間毎の最大人数(最大) */
	  g_dwServerStartTime,											/* 稼動開始時間 */
	  g_dwNewMapID,													/* 新ID(マップ) */
	  g_dwSendSizeBack,												/* 前回の送信サイズ */
	  g_dwRecvSizeBack;												/* 前回の受信サイズ */

char g_szAdminAcc[MAX_ACCNAME],										/* 管理者アカウント名 */
	 g_szAdminPas[MAX_ACCNAME],										/* 管理者パスワード */
	 g_szCliVer[64];												/* クライアント最新バージョン */

BYTE g_bySendDataCount,												/* 送信平均サイズ計算用 */
	 g_byRecvDataCount,												/* 受信平均サイズ計算用 */
	 g_byAdminCount,												/* 管理人数 */
	 g_byLastSendClock;												/* 最後に通知した時刻 */

BOOL g_bFreeAcount,													/* アカウント自動追加モード？ */
	 g_bAllAdmin;													/* 全員管理人権限 */

int g_nProcChar,													/* 処理ID(キャラ) */
	g_nProcMap,														/* 処理ID(マップ) */
	g_nProcSubGame;													/* 処理ID(サブゲーム) */

PSERVERINFOUPLOADINFO g_pServerInfoUploadInfo;						/* サーバー情報アップロード情報 */
PMAPPARTSINFO g_aMapPartsInfo;										/* マップパーツ情報 */
PCOMEFCINFO g_pComEfcInfo;											/* 吹き出し情報 */
CInfoEfc g_EfcInfo;													/* エフェクト情報 */
CInfoJob g_JobInfo;													/* 職業情報 */
CProcSubGame g_ProcSubGame;											/* サブゲーム処理 */
CLibInfoItem g_LibInfoItem;											/* アイテム情報 */
CmyArray<PCHARINFO, PCHARINFO> g_aCharInfo;							/* キャラ情報 */
CmyArray<PMAPINFO, PMAPINFO> g_aMapInfo;							/* マップ情報 */
CmyArray<PITEMINFO, PITEMINFO> g_aItemInfo;							/* アイテム情報 */
CmyArray<PSTATUSTEMPLATEINFO, PSTATUSTEMPLATEINFO> g_aStTmpInfo;	/* ステータステンプレート情報 */
CmyArray<PLEAVEINFO, PLEAVEINFO> g_aLeaveInfo;						/* 預かり所情報 */
CmyArray<PSUBGAMEINFO, PSUBGAMEINFO> g_aSubaGameInfo;				/* サブゲーム情報 */
CmyArray<PPARTYINFO, PPARTYINFO> g_aPartyInfo;						/* パーティ情報 */
CmyArray<PMAGICINFO, PMAGICINFO> g_aMagicInfo;						/* 魔法情報 */
CmyArray<PMAGICUSERINFO, PMAGICUSERINFO> g_aMagicUserInfo;			/* 魔法利用者情報 */
CmyArray<PEQUUSERINFO, PEQUUSERINFO> g_aEquUserInfo;				/* 装備職業情報 */

/* 向き別縦横増減テーブル */
int anPosAddTblX[] = {0, 0, -1, 1},
	anPosAddTblY[] = {-1, 1, 0, 0};
/* 向き反転テーブル */
BYTE abyDirectionTbl[] = {1, 0, 3, 2};

/* 酔っ払いレベルテーブル */
const char *aDrunkTypeTbl[] = {
	"平常",
	"陶酔",
	"泥酔",
	"狂酔",
	"z"
};

/* Copyright(C)URARA-works 2003-2005 */
