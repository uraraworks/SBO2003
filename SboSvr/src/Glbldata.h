/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			グローバル変数の外部参照用定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#pragma once

class CInfoEfc;
class CInfoJob;
class CProcSubGame;
class CLibInfoItem;

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
extern CUraraSockTCP svrSock;										/* サーバーオブジェクト */
extern CTextOutput g_LogLogin;										/* ログインログ */

extern HINSTANCE g_hInst;											/* インスタンスハンドル */

extern HWND g_hWndMain;												/* メインウィンドウハンドル */

extern WORD g_wPort,												/* 待ち受けポート番号 */
			g_wCountMapParts;										/* 使用マップパーツ数 */

extern DWORD g_dwTime,												/* 現在時刻 */
			 g_dwIDGM,												/* 管理人キャラID */
			 g_dwOnline,											/* オンライン数 */
			 g_dwOnlineBack[24],									/* 時間毎の最大人数(稼動中) */
			 g_dwOnlineBackMax[24],									/* 時間毎の最大人数(最大) */
			 g_dwServerStartTime,									/* 稼動開始時間 */
			 g_dwNewMapID,											/* 新ID(マップ) */
			 g_dwSendSizeBack,										/* 前回の送信サイズ */
			 g_dwRecvSizeBack;										/* 前回の受信サイズ */

extern char g_szAdminAcc[MAX_ACCNAME],								/* 管理者アカウント名 */
			g_szAdminPas[MAX_ACCNAME],								/* 管理者パスワード */
			g_szCliVer[64];											/* クライアント最新バージョン */

extern BYTE g_bySendDataCount,										/* 送信平均サイズ計算用 */
			g_byRecvDataCount,										/* 受信平均サイズ計算用 */
		    g_byAdminCount,											/* 管理人数 */
			g_byLastSendClock;										/* 最後に通知した時刻 */

extern BOOL g_bFreeAcount,											/* アカウント自動追加モード？ */
			g_bAllAdmin;											/* 全員管理人権限 */

extern int g_nProcChar,												/* 処理ID(キャラ) */
		   g_nProcMap,												/* 処理ID(マップ) */
		   g_nProcSubGame;											/* 処理ID(サブゲーム) */

extern PSERVERINFOUPLOADINFO g_pServerInfoUploadInfo;						/* サーバー情報アップロード情報 */
extern PMAPPARTSINFO g_aMapPartsInfo;										/* マップパーツ情報 */
extern PCOMEFCINFO g_pComEfcInfo;											/* 吹き出し情報 */
extern CInfoEfc g_EfcInfo;													/* エフェクト情報 */
extern CInfoJob g_JobInfo;													/* 職業情報 */
extern CProcSubGame g_ProcSubGame;											/* サブゲーム処理 */
extern CLibInfoItem g_LibInfoItem;											/* アイテム情報 */
extern CmyArray<PCHARINFO, PCHARINFO> g_aCharInfo;							/* キャラ情報 */
extern CmyArray<PMAPINFO, PMAPINFO> g_aMapInfo;								/* マップ情報 */
extern CmyArray<PITEMINFO, PITEMINFO> g_aItemInfo;							/* アイテム情報 */
extern CmyArray<PSTATUSTEMPLATEINFO, PSTATUSTEMPLATEINFO> g_aStTmpInfo;		/* ステータステンプレート情報 */
extern CmyArray<PLEAVEINFO, PLEAVEINFO> g_aLeaveInfo;						/* 預かり所情報 */
extern CmyArray<PSUBGAMEINFO, PSUBGAMEINFO> g_aSubaGameInfo;				/* サブゲーム情報 */
extern CmyArray<PPARTYINFO, PPARTYINFO> g_aPartyInfo;						/* パーティ情報 */
extern CmyArray<PMAGICINFO, PMAGICINFO> g_aMagicInfo;						/* 魔法情報 */
extern CmyArray<PMAGICUSERINFO, PMAGICUSERINFO> g_aMagicUserInfo;			/* 魔法利用者情報 */
extern CmyArray<PEQUUSERINFO, PEQUUSERINFO> g_aEquUserInfo;					/* 装備職業情報 */

/* 向き別縦横増減テーブル */
extern  int anPosAddTblX[],
			anPosAddTblY[];
extern BYTE abyDirectionTbl[];
/* 酔っ払いレベルテーブル */
extern const char *aDrunkTypeTbl[];

/* Copyright(C)URARA-works 2003-2005 */
