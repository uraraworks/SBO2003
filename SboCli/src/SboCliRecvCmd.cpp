/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboCliRecvCmd.cpp											 */
/* 内容：		コマンド受信処理											 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../Common/Info/InfoJob.h"
#include "../Common/Info/SubGame/InfoSubGameBase.h"
#include "../Common/Command/Command.h"
#include "Proc/ProcSubGame.h"
#include "Glbldata.h"
#include "SboCliAdminWindow.h"
#include "SboCliMakeWindow.h"
#include "SboCliDrawWindow.h"
#include "SboCliSendCmd.h"
#include "SboCliMain.h"
#include "SboCliDrawScreen.h"
#include "SboAdminWindow.h"
#include "AdminWindowOld/AdminDlgNpcRoute.h"
#include "SboCliDebugWindow.h"
#include "SboCliMsgLogWindow.h"
#include "SboCliBalloonWindow.h"
#ifdef __EMSCRIPTEN__
extern void SboWebChatNotify(void);								/* 発言が来たことをシェルに知らせる（タブの題名で新着を知らせる） */
#endif


/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */

typedef void (*RECVDATAPROC)(PBYTE, DWORD);


/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
int		GetCharNo							(DWORD);												/* キャラ番号取得 */
int		GetCharNo2							(DWORD);												/* キャラ番号取得 */
int		GetEventDataSize					(BYTE);													/* イベント種別毎のデータサイズを取得 */
int		GetSubGameDataSize					(BYTE);													/* サブゲーム種別毎のデータサイズを取得 */
int		GetItemDataSize						(BYTE);													/* アイテム種別毎のデータサイズを取得 */
int		GetItemCount						(DWORD);												/* アイテム数を取得 */
int		GetItemCountType					(DWORD, BYTE);											/* 種類指定でアイテム数を取得 */
int		GetItemCountTypeEqu					(DWORD, BYTE);											/* 種類指定でアイテム数を取得(装備不可は除く) */
int		GetItemCountTypeID					(DWORD, BYTE, BYTE);									/* 種類指定でアイテムIDを取得 */
int		GetItemCountTypeIDEqu				(DWORD, BYTE, BYTE);									/* 種類指定でアイテムIDを取得(装備不可は除く) */
int		GetItemCountTypeNo					(DWORD, BYTE, BYTE);									/* 種類指定でアイテム番号を取得 */
int		GetItemCountTypeNoEqu				(DWORD, BYTE, BYTE);									/* 種類指定でアイテム番号を取得(装備不可は除く) */
int		GetCharDistance						(PCHARINFO pCharTarget, DWORD dwMapID, int x, int y, BYTE byDirection);		/* 指定キャラとの距離を求める */
void	RenewATDF							(DWORD);												/* 攻撃力・守備力の更新 */
void	RenewCharPosInfo					(void);													/* キャラ座標情報更新 */
void	RenewUseMagicInfo					(void);													/* 使用できる魔法一覧更新 */
void	RenewMapDark						(PCHARINFO pChar);										/* 暗闇描画情報を更新 */
void	RenewMapDarkWork					(PCHARINFO, int x, int y, int nCount);					/* 暗闇描画情報を更新処理 */
int		GetMapDarkLevel						(PCHARINFO pChar, int x, int y);						/* 灯りの明るさを取得 */
void	RenewMapDarkCorner					(PCHARINFO pChar);										/* 灯りの角の壁を照らす */
void	RenewMapDarkParts					(void);													/* 光るマップパーツで照らす */
void	RenewMapDarkHideArea				(void);													/* 明るく壁で隠す範囲の中を全部見せる */
BOOL	IsMapHideArea						(void);													/* 明るいが壁の向こうは見えない所か */
static BOOL IsHideAreaCross				(int nFromX, int nFromY, int x, int y);		/* 灯りが隠す範囲の境目を越えたか */
static BOOL IsMapHasHideArea				(void);												/* 明るく壁で隠す範囲があるか */
BOOL	IsLightOtherRoom					(int x, int y);										/* 自分と違う部屋(隠す範囲)にある灯りか */
static void RenewMapViewLights			(void);												/* 見える範囲のマップで光源の所も見せる */
static void RenewMapDarkLightFlood		(int mx, int my, int nRange);							/* 灯りの位置から照らす */
int	GetMapPosLight						(int mx, int my, BYTE *pbyPower, BYTE *pbyFlg);										/* マスの灯りの範囲(パーツ・光源) */

/* 光るマップパーツの灯り(GetMapDarkLevel に pChar=NULL で渡した時に使う) */
static int s_nPartsLightX = 0;			/* 灯りのマップ座標(X) */
static int s_nPartsLightY = 0;			/* 灯りのマップ座標(Y) */
static int s_nPartsLightRange = 0;		/* 灯りの範囲(マス) */
BOOL	CheckEquUserInfo					(WORD, BYTE);											/* 装備職業情報にあるか判定 */
BOOL	CheckItemSexDistinction				(PITEMINFO, PCHARINFO);									/* 性別による使えるかチェック */
BOOL	CheckCharBlock						(DWORD, DWORD);											/* キャラのぶつかり判定 */
BOOL	IsSightWall							(int, int);												/* 灯りを止めるマス？ */
int		CheckCharBlockPC					(DWORD, DWORD);											/* キャラのキャラとのぶつかり判定 */
BOOL	CheckBlockEventFront				(PCHARINFO pChar, int x, int y, BYTE byDirection);		/* 1歩前にイベントがあるか判定 */
BOOL	IsBlock								(int x, int y, BOOL bHide);								/* ぶつかり判定 */
BOOL	CheckCharX							(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget);			/* キャラサイズに合わせた当たり判定(横) */
int		GetCharCenterX						(PCHARINFO pChar);										/* キャラの中心位置を取得 */
BOOL	CheckTargetPos						(int nXFrom, int nYFrom, int nXTo, int nYTo, int nCount);					/* 指定座標までたどり着けるかチェック */
void	CheckTargetPosWork					(int nXFrom, int nYFrom, int nXTo, int nYTo, int nCount, BOOL *pbCheck);	/* 指定座標までたどり着けるかチェック処理 */
int		GetUseCharInfo						(int nArea, PCHARINFO pMyChar, BOOL bSelf, int nType, ARRAYUSECHARINFO &aDst, int &nLen);	/* 誰に使うか一覧を取得 */
int		GetAttackType						(PCHARINFO pChar);										/* 攻撃種別を取得 */
void	StartTurnGage						(PCHARINFO pChar);										/* ターンゲージ開始 */
void	CancelTurnGage						(PCHARINFO pChar);										/* ターンゲージをキャンセル */
int		FindNextAttackTarget				(PCHARINFO pChar);										/* 連続攻撃の次の相手を探す */
void	ContinueTurnGage					(PCHARINFO pChar);										/* 連続攻撃：次のターンゲージを溜め始める */
int		FindFrontAttackTarget				(PCHARINFO pChar);										/* 目の前にいる敵を探す */
void	RecvDataProc						(PBYTE, DWORD);			/* 受信データ処理 */
void	RecvDataProcCONNECT					(PBYTE, DWORD);			/* 受信データ処理(接続応答) */
void	RecvDataProcLOGINRES				(PBYTE, DWORD);			/* 受信データ処理(ログイン応答) */
void	RecvDataProcWEBACCOUNT				(PBYTE, DWORD);			/* 受信データ処理(ブラウザ版のアカウント関係) */
void	RecvDataProcADMINUNDO				(PBYTE, DWORD);			/* 受信データ処理(管理者画面の「元に戻す」) */
void	RecvDataProcHASTE					(PBYTE, DWORD);			/* 受信データ処理(加速の状態) */
void	RecvDataProcMAKERINFO				(PBYTE, DWORD);			/* 受信データ処理(作り手情報) */
void	RecvDataProcRECIPEINFO				(PBYTE, DWORD);			/* 受信データ処理(レシピ情報) */
void	RecvDataProcRECIPEMAKE				(PBYTE, DWORD);			/* 受信データ処理(作ってもらった結果) */
void	RecvDataProcNPCROUTE				(PBYTE, DWORD);			/* 受信データ処理(NPCの巡回ルート) */
void	RecvDataProcTOWNWARP				(PBYTE, DWORD);			/* 受信データ処理(町転送の行き先一覧) */
void	RecvDataProcQUEST					(PBYTE, DWORD);			/* 受信データ処理(ギルドの仕事依頼：一覧・結果) */
void	RecvDataProcQUESTSTATE				(PBYTE, DWORD);			/* 受信データ処理(依頼の進み具合) */
/* ブラウザ版のアカウント関係（web/replace/SboCliWebAccount_web.cpp） */
void	WebAccSaveToken						(LPCSTR);				/* 端末トークンをページへ渡す */
void	WebAccOnTokenNG						(void);					/* 端末トークンが無効だった */
void	RecvDataProcCHGSTATE				(PBYTE, DWORD);			/* 受信データ処理(状態遷移) */
void	RecvDataProcMAPINFO					(PBYTE, DWORD);			/* 受信データ処理(マップ情報通知) */
void	RecvDataProcMAPPARTSINFO			(PBYTE, DWORD);			/* 受信データ処理(マップパーツ情報通知) */
void	RecvDataProcCHARINFO				(PBYTE, DWORD);			/* 受信データ処理(キャラ情報通知) */
void	RecvDataProcCHARINFO2				(PBYTE, DWORD);			/* 受信データ処理(キャラ情報通知2) */
void	RecvDataProcCHGDIRECTION			(PBYTE, DWORD);			/* 受信データ処理(向き変更通知) */
void	RecvDataProcCHARPOS					(PBYTE, DWORD);			/* 受信データ処理(座標通知) */
void	RecvDataProcSPEECH					(PBYTE, DWORD);			/* 受信データ処理(発言通知) */
void	RecvDataProcONLINE					(PBYTE, DWORD);			/* 受信データ処理(オンライン数通知) */
void	RecvDataProcMAPPARTS				(PBYTE, DWORD);			/* 受信データ処理(マップパーツ更新) */
void	RecvDataProcCHGMAP					(PBYTE, DWORD);			/* 受信データ処理(マップ更新) */
void	RecvDataProcADDDELMAP				(PBYTE, DWORD);			/* 受信データ処理(マップ幅増減) */
void	RecvDataProcCHGEVENT				(PBYTE, DWORD);			/* 受信データ処理(イベント情報更新) */
void	RecvDataProcCOUNTEVENT				(PBYTE, DWORD);			/* 受信データ処理(イベント数通知) */
void	RecvDataProcEVENTPOS				(PBYTE, DWORD);			/* 受信データ処理(イベント座標通知) */
void	RecvDataProcHARTBEAT				(PBYTE, DWORD);			/* 受信データ処理(生存確認応答) */
void	RecvDataProcCHGBGM					(PBYTE, DWORD);			/* 受信データ処理(BGM変更) */
void	RecvDataProcMSG						(PBYTE, DWORD);			/* 受信データ処理(メッセージ変更) */
void	RecvDataProcVIEWMSG					(PBYTE, DWORD);			/* 受信データ処理(メッセージ表示) */
void	RecvDataProcPLAYSOUND				(PBYTE, DWORD);			/* 受信データ処理(効果音再生) */
void	RecvDataProcMAPMSG					(PBYTE, DWORD);			/* 受信データ処理(メッセージ通知) */
void	RecvDataProcMAPMSGREQ				(PBYTE, DWORD);			/* 受信データ処理(メッセージ通知) */
void	RecvDataProcSYSMSG					(PBYTE, DWORD);			/* 受信データ処理(システムメッセージ) */
void	RecvDataProcEFECTCHG				(PBYTE, DWORD);			/* 受信データ処理(エフェクト変更) */
void	RecvDataProcMAIN_CHAR				(PBYTE, DWORD);			/* 受信データ処理(キャラ系コマンド) */
void	RecvDataProcCOMEFC					(PBYTE, DWORD);			/* 受信データ処理(吹き出し通知) */
void	RecvDataProcERR						(PBYTE, DWORD);			/* 受信データ処理(エラー通知) */
void	RecvDataProcGETACC					(PBYTE, DWORD);			/* 受信データ処理(アカウント・パスワード取得) */
void	RecvDataProcGETACCINFO				(PBYTE, DWORD);			/* 受信データ処理(アカウント情報要求) */
void	RecvDataProcCEFINFO					(PBYTE, DWORD);			/* 受信データ処理(吹き出し情報) */
void	RecvDataProcCHGCEF					(PBYTE, DWORD);			/* 受信データ処理(吹き出し情報更新) */
void	RecvDataProcFADEINOUT				(PBYTE, DWORD);			/* 受信データ処理(フェードイン・アウト) */
void	RecvDataProcCHGITEM					(PBYTE, DWORD);			/* 受信データ処理(アイテム情報更新) */
void	RecvDataProcITEMINFO				(PBYTE, DWORD);			/* 受信データ処理(アイテム情報) */
void	RecvDataProcCHARITEM				(PBYTE, DWORD);			/* 受信データ処理(所持アイテム情報) */
void	RecvDataProcSVRINFO					(PBYTE, DWORD);			/* 受信データ処理(サーバー情報) */
void	RecvDataProcMAPCHARINFO				(PBYTE, DWORD);			/* 受信データ処理(マップ内キャラ数通知) */
void	RecvDataProcNAMECOLOR				(PBYTE, DWORD);			/* 受信データ処理(名前の色設定) */
void	RecvDataProcSHOP					(PBYTE, DWORD);			/* 受信データ処理(店情報) */
void	RecvDataProcEFC						(PBYTE, DWORD);			/* 受信データ処理(エフェクト情報更新) */
void	RecvDataProcEFCINFO					(PBYTE, DWORD);			/* 受信データ処理(エフェクト情報通知) */
void	RecvDataProcCHARSTATUS				(PBYTE, DWORD);			/* 受信データ処理(キャラステータス通知) */
void	RecvDataProcTARGETCHAR				(PBYTE, DWORD);			/* 受信データ処理(ターゲット要求) */
void	RecvDataProcCHARSTATUSHP			(PBYTE, DWORD);			/* 受信データ処理(キャラステータス(HPのみ)通知) */
void	RecvDataProcCHARSTATUSGOLD			(PBYTE, DWORD);			/* 受信データ処理(キャラステータス(GOLDのみ)通知) */
void	RecvDataProcTARGETCHARPC			(PBYTE, DWORD);			/* 受信データ処理(ついていく通知) */
void	RecvDataProcADDCHAREFC				(PBYTE, DWORD);			/* 受信データ処理(文字エフェクト追加) */
void	RecvDataProcCHGMAPCOLOR				(PBYTE, DWORD);			/* 受信データ処理(マップ色変換) */
void	RecvDataProcSTTMPLATE				(PBYTE, DWORD);			/* 受信データ処理(ステータステンプレート情報) */
void	RecvDataProcLEAVEITEMGET			(PBYTE, DWORD);			/* 受信データ処理(預けてるアイテム一覧) */
void	RecvDataProcCOUNTSUBGAME			(PBYTE, DWORD);			/* 受信データ処理(サブゲーム数通知) */
void	RecvDataProcSUBGAMEINFO				(PBYTE, DWORD);			/* 受信データ処理(サブゲーム情報更新) */
void	RecvDataProcSUBGAMEINFOALL			(PBYTE, DWORD);			/* 受信データ処理(全サブゲーム情報通知) */
void	RecvDataProcSUBSTATUS				(PBYTE, DWORD);			/* 受信データ処理(サブステータス情報) */
void	RecvDataProcTRADE					(PBYTE, DWORD);			/* 受信データ処理(取り引きコマンド) */
void	RecvDataProcTRADEINFO				(PBYTE, DWORD);			/* 受信データ処理(取り引き情報) */
void	RecvDataProcSYSMSG2					(PBYTE, DWORD);			/* 受信データ処理(ID指定システムメッセージ) */
void	RecvDataProcLEAVEGOLDGET			(PBYTE, DWORD);			/* 受信データ処理(預けてる金額) */
void	RecvDataProcLEAVEINFO				(PBYTE, DWORD);			/* 受信データ処理(預かり所情報) */
void	RecvDataProcACCSEARCH				(PBYTE, DWORD);			/* 受信データ処理(アカウント検索) */
void	RecvDataProcADMINWINDOW				(PBYTE, DWORD);			/* 受信データ処理(管理者ウィンドウ表示) */
void	RecvDataProcPARTY					(PBYTE, DWORD);			/* 受信データ処理(仲間コマンド) */
void	RecvDataProcCHARSTATUSHPMANA		(PBYTE, DWORD);			/* 受信データ処理(キャラステータス(HPとMANA)通知) */
void	RecvDataProcJOBINFO					(PBYTE, DWORD);			/* 受信データ処理(職業情報) */
void	RecvDataProcMAGICINFO				(PBYTE, DWORD);			/* 受信データ処理(魔法情報) */
void	RecvDataProcMAGICUSERINFO			(PBYTE, DWORD);			/* 受信データ処理(魔法利用者情報) */
void	RecvDataProcEQUUSERINFO				(PBYTE, DWORD);			/* 受信データ処理(装備職業情報) */
void	RecvDataProcSUBGAME					(PBYTE, DWORD);			/* 受信データ処理(サブゲーム用コマンド) */
void	RecvDataProcSubGameBOMB_TIME		(PBYTE, DWORD);			/* 受信データ処理(残り時間) */
void	RecvDataProcSubGameBOMB_INFO		(PBYTE, DWORD);			/* 受信データ処理(設定) */
void	RecvDataProcSubGameBOMB_STOP		(PBYTE, DWORD);			/* 受信データ処理(試合中止) */
void	RecvDataProcSubGameBOMB_SETBOMB		(PBYTE, DWORD);			/* 受信データ処理(爆弾設置) */
void	RecvDataProcSubGameWAR_CHGSTATE		(PBYTE, DWORD);			/* 受信データ処理(戦場ゲーム：状態遷移) */
void	RecvDataProcSubGameWAR_SELECTTURN	(PBYTE, DWORD);			/* 受信データ処理(戦場ゲーム：順番選択) */
void	RecvDataProcSubGameWAR_MAGIC_TARGET	(PBYTE, DWORD);			/* 受信データ処理(戦場ゲーム：魔法対象者一覧) */
void	RecvDataProcSubGameSUGOROKU_CHGSTATE(PBYTE, DWORD);			/* 受信データ処理(すごろく：状態遷移) */
void	RecvDataProcMAIN_CHAR_STATEINFO		(PBYTE, DWORD);			/* 受信データ処理(キャラ系:状態通知) */
void	RecvDataProcMAIN_CHAR_TAIL			(PBYTE, DWORD);			/* 受信データ処理(キャラ系:ついていく要求) */

extern int	GetWindowID	(DWORD);									/* ウィンドウ番号を取得 */
extern void	DeleteChar	(int);										/* キャラ削除 */
extern int	GetCharID	(DWORD);									/* キャラ番号を取得 */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
/* 関数ジャンプテーブル */
const RECVDATAPROC RecvDataProcTbl[] = {
	NULL,
	RecvDataProcCONNECT,				/* CMD_CONNECT */
	NULL,								/* CMD_LOGINREQ */
	RecvDataProcLOGINRES,				/* CMD_LOGINRES */
	RecvDataProcCHGSTATE,				/* CMD_CHGSTATE */
	RecvDataProcCHARINFO2,				/* CMD_CHARINFO2 */
	RecvDataProcMAPINFO,				/* CMD_MAPINFO */
	RecvDataProcMAPPARTSINFO,			/* CMD_MAPPARTSINFO */
	RecvDataProcCHGDIRECTION,			/* CMD_CHGDIRECTION */
	RecvDataProcCHARPOS,				/* CMD_CHARPOS */
	NULL,								/* CMD_CHARINFOREQ */
	RecvDataProcSPEECH,					/* CMD_SPEECH */
	RecvDataProcONLINE,					/* CMD_ONLINE */
	RecvDataProcHARTBEAT,				/* CMD_HARTBEAT */
	RecvDataProcCOUNTEVENT,				/* CMD_COUNTEVENT */
	RecvDataProcEVENTPOS,				/* CMD_EVENTPOS */
	NULL,								/* CMD_CHECKEVENT */
	RecvDataProcCHGBGM,					/* CMD_CHGBGM */
	RecvDataProcVIEWMSG,				/* CMD_VIEWMSG */
	RecvDataProcPLAYSOUND,				/* CMD_PLAYSOUND */
	RecvDataProcMAPMSG,					/* CMD_MAPMSG */
	NULL,								/* CMD_OPENDOOR */
	NULL,								/* CMD_CHGCHARGRP */
	NULL,								/* CMD_CHGNAMESEX */
	RecvDataProcSYSMSG,					/* CMD_SYSMSG */
	NULL,								/* CMD_GMCALL */
	NULL,								/* CMD_CHARPUSH */
	RecvDataProcCOMEFC,					/* CMD_COMEFC */
	RecvDataProcERR,					/* CMD_ERR */
	NULL,								/* CMD_ADDSCORE */
	NULL,								/* CMD_TALKRAND */
	NULL,								/* CMD_STOPRAND */
	RecvDataProcCEFINFO,				/* CMD_CEFINFO */
	RecvDataProcFADEINOUT,				/* CMD_FADEINOUT */
	RecvDataProcITEMINFO,				/* CMD_ITEMINFO */
	NULL,								/* CMD_GETITEM */
	RecvDataProcCHARITEM,				/* CMD_CHARITEM */
	NULL,								/* CMD_DROPITEM */
	NULL,								/* CMD_USEITEM */
	RecvDataProcMAPCHARINFO,			/* CMD_MAPCHARINFO */
	RecvDataProcNAMECOLOR,				/* CMD_NAMECOLOR */
	RecvDataProcSHOP,					/* CMD_SHOP */
	RecvDataProcEFC,					/* CMD_EFC */
	RecvDataProcEFCINFO,				/* CMD_EFCINFO */
	NULL,								/* CMD_EFCREQ */
	RecvDataProcTARGETCHAR,				/* CMD_TARGETCHAR */
	RecvDataProcCHARSTATUSHP,			/* CMD_CHARSTATUSHP */
	RecvDataProcCHARSTATUSGOLD,			/* CMD_CHARSTATUSGOLD */
	RecvDataProcTARGETCHARPC,			/* CMD_TARGETCHARPC */
	RecvDataProcADDCHAREFC,				/* CMD_ADDCHAREFC */
	NULL,								/* CMD_CHGEQU */
	RecvDataProcCHGMAPCOLOR,			/* CMD_CHGMAPCOLOR */
	NULL,								/* CMD_LEAVEITEM */
	RecvDataProcLEAVEITEMGET,			/* CMD_LEAVEITEMGET */
	RecvDataProcCOUNTSUBGAME,			/* CMD_COUNTSUBGAME */
	RecvDataProcSUBSTATUS,				/* CMD_SUBSTATUS */
	RecvDataProcSUBGAME,				/* CMD_SUBGAME */
	RecvDataProcTRADE,					/* CMD_TRADE */
	RecvDataProcTRADEINFO,				/* CMD_TRADEINFO */
	NULL,								/* CMD_TRADEPROC */
	RecvDataProcSYSMSG2,				/* CMD_SYSMSG2 */
	NULL,								/* CMD_LEAVEGOLD */
	RecvDataProcLEAVEGOLDGET,			/* CMD_LEAVEGOLDGET */
	NULL,								/* CMD_SETSTARTPOS */
	NULL,								/* CMD_CHGPASSWORD */
	NULL,								/* CMD_CHGCHARGRPBACK */
	NULL,								/* CMD_MSGCMD */
	RecvDataProcPARTY,					/* CMD_PARTY */
	RecvDataProcCHARSTATUSHPMANA,		/* CMD_CHARSTATUSHPMANA */
	NULL,								/* CMD_JOBCHG */
	NULL,								/* CMD_USEMAGIC */
	NULL,								/* CMD_TARGETCHG */
	NULL,								/* CMD_SHOPBUY */
	NULL,								/* CMD_DIFENCE */
	NULL,								/* CMD_THROWSNOW */
	RecvDataProcEFECTCHG,				/* CMD_EFECTCHG */
	NULL,								/* CMD_RESURRECTION */
	NULL,								/* CMD_REQ_ATTACK */
	RecvDataProcMAIN_CHAR,				/* CMD_MAIN_CHAR */
/* 以下、管理者用コマンド CMD_MAPPARTSADD より上には置かない事。判別に使ってるので。 */
	NULL,								/* CMD_MAPPARTSADD */
	RecvDataProcCHARSTATUS,				/* CMD_CHARSTATUS */
	RecvDataProcMAPPARTS,				/* CMD_MAPPARTS */
	NULL,								/* CMD_MAPPARTSRELOAD */
	NULL,								/* CMD_MAPPARTSADDANIME */
	NULL,								/* CMD_ADDMAP */
	RecvDataProcMAPMSGREQ,				/* CMD_MAPMSGREQ */
	NULL,								/* CMD_CHGMSG */
	RecvDataProcCHARINFO,				/* CMD_CHARINFO */
	RecvDataProcCHGMAP,					/* CMD_CHGMAP */
	RecvDataProcADDDELMAP,				/* CMD_ADDDELMAP */
	NULL,								/* CMD_SAVEMAP */
	NULL,								/* CMD_CHGMAPINFO */
	NULL,								/* CMD_EVENTADD */
	NULL,								/* CMD_EVENTGET */
	RecvDataProcCHGEVENT,				/* CMD_CHGEVENT */
	NULL,								/* CMD_ADDCHAR */
	NULL,								/* CMD_BANCHAR */
	NULL,								/* CMD_ADDMSG */
	RecvDataProcMSG,					/* CMD_MSG */
	NULL,								/* CMD_MAPMOVE */
	NULL,								/* CMD_DELCHAR */
	NULL,								/* CMD_ACCADD */
	RecvDataProcGETACC,					/* CMD_GETACC */
	NULL,								/* CMD_CHGACC */
	RecvDataProcGETACCINFO,				/* CMD_GETACCINFO */
	NULL,								/* CMD_CHGACCSTATE */
	NULL,								/* CMD_CLIVERSION */
	RecvDataProcCHGCEF,					/* CMD_CHGCEF */
	NULL,								/* CMD_CHGADMINON */
	NULL,								/* CMD_ADDITEM */
	RecvDataProcCHGITEM,				/* CMD_CHGITEM */
	NULL,								/* CMD_REQCHARITEM */
	NULL,								/* CMD_PLAYSOUNDALL */
	RecvDataProcSVRINFO,				/* CMD_SVRINFO */
	NULL,								/* CMD_ADDSHOP */
	NULL,								/* CMD_CHGSHOP */
	NULL,								/* CMD_ADDEFC */
	NULL,								/* CMD_CHGEFC */
	NULL,								/* CMD_REQCHARSTATUS */
	RecvDataProcSTTMPLATE,				/* CMD_STTMPLATE */
	NULL,								/* CMD_SUBGAMEGET */
	NULL,								/* CMD_SUBGAMEADD */
	RecvDataProcSUBGAMEINFO,			/* CMD_SUBGAMEINFO */
	RecvDataProcSUBGAMEINFOALL,			/* CMD_SUBGAMEINFOALL */
	NULL,								/* CMD_LEAVEINFOGET */
	RecvDataProcLEAVEINFO,				/* CMD_LEAVEINFO */
	RecvDataProcACCSEARCH,				/* CMD_ACCSEARCH */
	RecvDataProcADMINWINDOW,			/* CMD_ADMINWINDOW */
	RecvDataProcJOBINFO,				/* CMD_JOBINFO */
	NULL,								/* CMD_JOBINFOREQ */
	RecvDataProcMAGICINFO,				/* CMD_MAGICINFO */
	NULL,								/* CMD_MAGICINFOREQ */
	RecvDataProcMAGICUSERINFO,			/* CMD_MAGICUSERINFO */
	NULL,								/* CMD_MAGICUSERINFOREQ */
	RecvDataProcEQUUSERINFO,			/* CMD_EQUUSERINFO */
	NULL,								/* CMD_EQUUSERINFOREQ */
	RecvDataProcWEBACCOUNT,				/* CMD_WEBACCOUNT */
	RecvDataProcADMINUNDO,				/* CMD_ADMINUNDO */
	NULL,								/* CMD_REQ_GUARD */
	RecvDataProcHASTE,					/* CMD_HASTE */
	RecvDataProcMAKERINFO,				/* CMD_MAKERINFO */
	RecvDataProcRECIPEINFO,				/* CMD_RECIPEINFO */
	RecvDataProcRECIPEMAKE,				/* CMD_RECIPEMAKE */
	NULL,								/* CMD_BATTLEMODE */
	RecvDataProcNPCROUTE,				/* CMD_NPCROUTE */
	RecvDataProcTOWNWARP,				/* CMD_TOWNWARP */
	RecvDataProcQUEST,					/* CMD_QUEST */
	RecvDataProcQUESTSTATE,				/* CMD_QUESTSTATE */
	NULL,								/* CMD_SORTITEM(クライアント→サーバーのみ) */
	NULL
};
/* 番号とテーブルの並びがずれていないか */
static_assert (sizeof (RecvDataProcTbl) / sizeof (RecvDataProcTbl[0]) == CMD_MAX + 1, "RecvDataProcTbl");

/* 関数ジャンプテーブル(爆弾ゲーム) */
const RECVDATAPROC RecvDataProcSubGameTblBomb[] = {
	NULL,
	RecvDataProcSubGameBOMB_SETBOMB,	/* CMDSUBBOMB_SETBOMB */
	RecvDataProcSubGameBOMB_TIME,		/* CMDSUBBOMB_TIME */
	RecvDataProcSubGameBOMB_INFO,		/* CMDSUBBOMB_INFO */
	NULL,								/* CMDSUBBOMB_START */
	RecvDataProcSubGameBOMB_STOP,		/* CMDSUBBOMB_STOP */
	NULL,								/* CNDSUBBOMB_GETITEM */
	NULL,								/* CNDSUBBOMB_REMOCON */
	NULL
};

/* 関数ジャンプテーブル(迷路) */
const RECVDATAPROC RecvDataProcSubGameTblMaze[] = {
	NULL,
	NULL,								/* CMDSUBBOMB_MAZERENEW */
	NULL,								/* CMDSUBBOMB_DUNGEONRENEW */
};

/* 関数ジャンプテーブル(戦場ゲーム) */
const RECVDATAPROC RecvDataProcSubGameTblWar[] = {
	NULL,
	RecvDataProcSubGameWAR_CHGSTATE,		/* CMDSUBWAR_CHGSTATE */
	NULL,									/* CMDSUBWAR_SELECTTEAM */
	NULL,									/* CMDSUBWAR_QUIT */
	NULL,									/* CMDSUBWAR_SELECTRULE */
	NULL,									/* CMDSUBWAR_SELECTJOB */
	RecvDataProcSubGameWAR_SELECTTURN,		/* CMDSUBWAR_SELECTTURN */
	NULL,									/* CMDSUBWAR_WAIT */
	NULL,									/* CMDSUBWAR_ATACK */
	NULL,									/* CMDSUBWAR_DIFENCE */
	NULL,									/* CMDSUBWAR_MAGIC */
	RecvDataProcSubGameWAR_MAGIC_TARGET,	/* CMDSUBWAR_MAGIC_TARGET */
};

/* 関数ジャンプテーブル(すごろく) */
const RECVDATAPROC RecvDataProcSubGameTblSugoroku[] = {
	NULL,
	RecvDataProcSubGameSUGOROKU_CHGSTATE,	/* CMDSUBSUGOROKU_CHGSTATE */
	NULL,									/* CMDSUBSUGOROKU_JOINMODE */
	NULL,									/* CMDSUBSUGOROKU_QUIT */
	NULL,									/* CMDSUBSUGOROKU_DICE */
};

/* 関数ジャンプテーブル(キャラ系コマンド) */
const RECVDATAPROC RecvDataProcTblCHAR[] = {
	NULL,
	RecvDataProcMAIN_CHAR_STATEINFO,			/* CMDCHAR_STATEINFO */
	RecvDataProcMAIN_CHAR_TAIL,					/* CMDCHAR_TAIL */
};


/* ========================================================================= */
/* 関数名：GetCharNo														 */
/* 内容：キャラ番号取得														 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
int GetCharNo(DWORD pID)
{
	int i;

	for (i = 0; i < (int)g_aCharInfo.GetSize (); i ++) {
		if (g_aCharInfo[i]->pID == pID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetCharNo2														 */
/* 内容：キャラ番号取得2													 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
int GetCharNo2(DWORD dwTblID)
{
	int i;

	for (i = 0; i < (int)g_aCharInfo.GetSize (); i ++) {
		if (g_aCharInfo[i]->dwTblID == dwTblID) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetEventDataSize													 */
/* 内容：イベント種別毎のデータサイズを取得									 */
/* 日付：2003/06/02															 */
/* ========================================================================= */
int GetEventDataSize(BYTE byType)
{
	int nSize = 0;

	switch (byType) {
	case EVENTTYPE_MOVE:		nSize = sizeof (EVENTINFO_MOVE);		break;
	case EVENTTYPE_MAPCHG:		nSize = sizeof (EVENTINFO_MAPCHG);		break;
	case EVENTTYPE_MSG:			nSize = sizeof (EVENTINFO_MSG);			break;
	case EVENTTYPE_DOOR:		nSize = sizeof (EVENTINFO_DOOR);		break;
	case EVENTTYPE_HPMANA:		nSize = sizeof (EVENTINFO_HPMANA);		break;
	case EVENTTYPE_ONCHARMAP:	nSize = sizeof (EVENTINFO_ONCHARMAP);	break;
	}
	return nSize;
}

/* ========================================================================= */
/* 関数名：GetSubGameDataSize												 */
/* 内容：サブゲーム種別毎のデータサイズを取得								 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
int GetSubGameDataSize(BYTE byType)
{
	int nSize = 0;

	switch (byType) {
	case SUBGAMEID_BOMB:		nSize = sizeof (SUBGAME_BOMBINFOSAVE);		break;
	case SUBGAMEID_MAZE:		nSize = sizeof (SUBGAME_MAZEINFOSAVE);		break;
	case SUBGAMEID_DUNGEON:		nSize = sizeof (SUBGAME_DUNGEONINFOSAVE);	break;
	case SUBGAMEID_WAR:			nSize = sizeof (SUBGAME_WARINFOSAVE);		break;
	case SUBGAMEID_SUGOROKU:	nSize = sizeof (SUBGAME_SUGOROKUINFOSAVE);	break;
	}
	return nSize;
}

/* ========================================================================= */
/* 関数名：GetItemDataSize													 */
/* 内容：アイテム種別毎のデータサイズを取得									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
int GetItemDataSize(BYTE byType)
{
	int nSize = 0;

	switch (byType) {
	case ITEMTYPE_NONE:			nSize = 0;								break;
	case ITEMTYPE_RURA:			nSize = sizeof (ITEMINFO_RURA);			break;
	case ITEMTYPE_HPUP:			nSize = sizeof (ITEMINFO_HPUP);			break;
	case ITEMTYPE_WEAPON:		nSize = sizeof (ITEMINFO_WEAPON);		break;
	case ITEMTYPE_ARMOR:		nSize = sizeof (ITEMINFO_ARMOR);		break;
	case ITEMTYPE_SHILD:		nSize = sizeof (ITEMINFO_SHILD);		break;
	case ITEMTYPE_HELMET:		nSize = sizeof (ITEMINFO_HELMET);		break;
	case ITEMTYPE_SEED:			nSize = sizeof (ITEMINFO_SEED);			break;
	case ITEMTYPE_SUBGAME:		nSize = sizeof (ITEMINFO_SUBGAME);		break;
	case ITEMTYPE_SAKE:			nSize = sizeof (ITEMINFO_SAKE);			break;
	case ITEMTYPE_MSG:			nSize = sizeof (ITEMINFO_MSG);			break;
	case ITEMTYPE_GRPCHG:		nSize = sizeof (ITEMINFO_GRPCHG);		break;
	case ITEMTYPE_LIGHT:		nSize = sizeof (ITEMINFO_LIGHT);		break;
	case ITEMTYPE_HPDOWN:		nSize = sizeof (ITEMINFO_HPDOWN);		break;
	case ITEMTYPE_MAGIC:		nSize = sizeof (ITEMINFO_MAGIC);		break;
	case ITEMTYPE_USEMAGIC:		nSize = sizeof (ITEMINFO_MAGIC);		break;
	case ITEMTYPE_MANAUP:		nSize = sizeof (ITEMINFO_MANAUP);		break;
	case ITEMTYPE_HPMANAUP:		nSize = sizeof (ITEMINFO_HPMANAUP);		break;
	case ITEMTYPE_EQU_HAND:		nSize = sizeof (ITEMTYPE_EQU_HAND);		break;
	case ITEMTYPE_EQU_LEG:		nSize = sizeof (ITEMTYPE_EQU_LEG);		break;
	case ITEMTYPE_EQU_ACC1:		nSize = sizeof (ITEMTYPE_EQU_ACC1);		break;
	case ITEMTYPE_EQU_ACC2:		nSize = sizeof (ITEMTYPE_EQU_ACC2);		break;
	case ITEMTYPE_EQU_ACC3:		nSize = sizeof (ITEMTYPE_EQU_ACC3);		break;
	case ITEMTYPE_RESURRECTION:	nSize = sizeof (ITEMTYPE_RESURRECTION);	break;
	default:					nSize = 0;								break;
	}
	return nSize;
}

/* ========================================================================= */
/* 関数名：GetItemCount														 */
/* 内容：アイテム数を取得													 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
int GetItemCount(DWORD dwID)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			nRet ++;
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetItemCountType													 */
/* 内容：種類指定でアイテム数を取得											 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
int GetItemCountType(DWORD dwID, BYTE byType)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			if (pItem->byType == byType) {
				nRet ++;
			}
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetItemCountTypeEqu												 */
/* 内容：種類指定でアイテム数を取得(装備不可は除く)							 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
int GetItemCountTypeEqu(DWORD dwID, BYTE byType)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			WORD wNo;

			/* 性別による使えるかチェック */
			if (CheckItemSexDistinction (pItem, pChar) == FALSE) {
				continue;
			}

			wNo = 0xFFFF;
			switch (pItem->byType) {
			case ITEMTYPE_WEAPON:	/* 武器 */
				{
					PITEMINFO_WEAPON pInfo;

					pInfo = (PITEMINFO_WEAPON)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_ARMOR:	/* 鎧 */
				{
					PITEMINFO_ARMOR pInfo;

					pInfo = (PITEMINFO_ARMOR)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_SHILD:	/* 盾 */
				{
					PITEMINFO_SHILD pInfo;

					pInfo = (PITEMINFO_SHILD)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_HELMET:	/* 兜 */
				{
					PITEMINFO_HELMET pInfo;

					pInfo = (PITEMINFO_HELMET)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;
			}
			if (wNo != 0xFFFF) {
				BOOL bEqu;
				bEqu = CheckEquUserInfo (wNo, pChar->byJOB);
				if (bEqu == FALSE) {
					continue;
				}
			}
			if (pItem->byType == byType) {
				nRet ++;
			}
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetItemCountTypeID												 */
/* 内容：種類指定でアイテムIDを取得											 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
int GetItemCountTypeID(DWORD dwID, BYTE byType, BYTE byNo)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			if (pItem->byType == byType) {
				if (nRet == (int)byNo) {
					return ((int)pChar->waItem[i]);
				}
				nRet ++;
			}
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetItemCountTypeIDEqu											 */
/* 内容：種類指定でアイテムIDを取得(装備不可は除く)							 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
int GetItemCountTypeIDEqu(DWORD dwID, BYTE byType, BYTE byNo)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			WORD wNo;
			PITEMINFO pItem;

			wNo = 0xFFFF;
			pItem = g_aItemInfo[pChar->waItem[i]];

			/* 性別による使えるかチェック */
			if (CheckItemSexDistinction (pItem, pChar) == FALSE) {
				continue;
			}

			switch (pItem->byType) {
			case ITEMTYPE_WEAPON:	/* 武器 */
				{
					PITEMINFO_WEAPON pInfo;

					pInfo = (PITEMINFO_WEAPON)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_ARMOR:	/* 鎧 */
				{
					PITEMINFO_ARMOR pInfo;

					pInfo = (PITEMINFO_ARMOR)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_SHILD:	/* 盾 */
				{
					PITEMINFO_SHILD pInfo;

					pInfo = (PITEMINFO_SHILD)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_HELMET:	/* 兜 */
				{
					PITEMINFO_HELMET pInfo;

					pInfo = (PITEMINFO_HELMET)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;
			}

			if (wNo != 0xFFFF) {
				BOOL bEqu;
				bEqu = CheckEquUserInfo (wNo, pChar->byJOB);
				if (bEqu == FALSE) {
					continue;
				}
			}
			if (pItem->byType == byType) {
				if (nRet == (int)byNo) {
					return ((int)pChar->waItem[i]);
				}
				nRet ++;
			}
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetItemCountTypeNo												 */
/* 内容：種類指定でアイテム番号を取得										 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
int GetItemCountTypeNo(DWORD dwID, BYTE byType, BYTE byNo)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			if (pItem->byType == byType) {
				if (nRet == (int)byNo) {
					return i;
				}
				nRet ++;
			}
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetItemCountTypeNoEqu											 */
/* 内容：種類指定でアイテム番号を取得(装備不可は除く)						 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
int GetItemCountTypeNoEqu(DWORD dwID, BYTE byType, BYTE byNo)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[i]];
			WORD wNo;

			/* 性別による使えるかチェック */
			if (CheckItemSexDistinction (pItem, pChar) == FALSE) {
				continue;
			}

			wNo = 0xFFFF;
			switch (pItem->byType) {
			case ITEMTYPE_WEAPON:	/* 武器 */
				{
					PITEMINFO_WEAPON pInfo;

					pInfo = (PITEMINFO_WEAPON)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_ARMOR:	/* 鎧 */
				{
					PITEMINFO_ARMOR pInfo;

					pInfo = (PITEMINFO_ARMOR)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_SHILD:	/* 盾 */
				{
					PITEMINFO_SHILD pInfo;

					pInfo = (PITEMINFO_SHILD)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_HELMET:	/* 兜 */
				{
					PITEMINFO_HELMET pInfo;

					pInfo = (PITEMINFO_HELMET)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;
			}
			if (wNo != 0xFFFF) {
				BOOL bEqu;
				bEqu = CheckEquUserInfo (wNo, pChar->byJOB);
				if (bEqu == FALSE) {
					continue;
				}
			}
			if (pItem->byType == byType) {
				if (nRet == (int)byNo) {
					return i;
				}
				nRet ++;
			}
		}
	}
	return -1;
}


/* ========================================================================= */
/* 関数名：	GetCharDistance													 */
/* 内容：	指定キャラとの距離を求める										 */
/* 日付：	2006/07/23														 */
/* ========================================================================= */

int GetCharDistance(
	PCHARINFO pCharTarget,		/* [in] ターゲットとなるキャラ */
	DWORD dwMapID,				/* [in] 元の位置(マップID) */
	int x,						/* [in] 元の位置(X座標) */
	int y,						/* [in] 元の位置(Y座標) */
	BYTE byDirection)			/* [in] 向き(-1で向き関係無し) */
{
	BOOL bResult;
	int nRet, nXtmp, nYtmp;

	nRet = -1;

	if (pCharTarget->dwMapID != dwMapID) {
		goto Exit;
	}

	nXtmp = abs (x - pCharTarget->dwX);
	nYtmp = abs (y - pCharTarget->dwY);

	if (nXtmp <= 3) {
		bResult = CheckCharX (pCharTarget, pCharTarget->dwX, x);
		if (bResult) {
			nXtmp = 0;
		}
	}

	/* 向きを指定した時は、サーバー(BattleProcCheckPC)と同じく「向いている方向の1マス先」に
	   いる時だけ距離1にする。以前は前後・左右の区別をしていなかったので、背中側の敵も
	   目の前と見なしてしまい、そちらを狙った攻撃要求がサーバーで空振りしていた */
	if (byDirection < 4) {
		int nFrontX = x + anPosAddTblX[byDirection];
		int nFrontY = y + anPosAddTblY[byDirection];

		if ((nFrontX >= 0) && (nFrontY == (int)pCharTarget->dwY) &&
				CheckCharX (pCharTarget, pCharTarget->dwX, (DWORD)nFrontX)) {
			nRet = 1;
			goto Exit;
		}
	}

	if (byDirection != -1) {
		if (byDirection == 2) {
			bResult = CheckCharX (pCharTarget, pCharTarget->dwX, x - 1);
			if (bResult) {
				nXtmp = 1;
			}
		}
		if (nXtmp == 0) {
			if (!((byDirection <= 1) && (nYtmp == 1))) {
				goto Exit;
			}
		} else if (nYtmp == 0) {
			if (!((byDirection >= 2) && (nXtmp == 1))) {
				goto Exit;
			}
		}
	}

	nRet = nXtmp + nYtmp;
	/* 向き指定で隣にいても目の前ではない(背中側など)なら、距離1とは言わない */
	if ((byDirection < 4) && (nRet == 1)) {
		nRet = -1;
	}
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：RenewATDF														 */
/* 内容：攻撃力・守備力の更新												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void RenewATDF(DWORD dwID)
{
	PCHARINFO pChar = g_aCharInfo[dwID];
	PITEMINFO pItem;
	pChar->wAT = pChar->byPW;
	if (pChar->waEqu[0] != 0xFFFF) {
		pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
		pChar->wAT += pItem->byData;
	}

	if (pChar->byNpcType == 0) {
		pChar->wDFMagic = 0;
	}
	pChar->wDF = pChar->byST / 2;
	for (int i = 0; i < MAX_EQUTYPE; i ++) {
		if (pChar->waEqu[i] != 0xFFFF) {
			pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[i]]];
			switch (pItem->byType) {
//			case ITEMTYPE_WEAPON:		/* 武器 */
//				break;
			case ITEMTYPE_ARMOR:		/* 鎧 */
			case ITEMTYPE_SHILD:		/* 盾 */
			case ITEMTYPE_HELMET:		/* 兜 */
			case ITEMTYPE_SEED:			/* 種 */
			case ITEMTYPE_EQU_HAND:		/* 装備(小手) */
			case ITEMTYPE_EQU_LEG:		/* 装備(靴) */
				pChar->wDF += pItem->byData;
				break;
			case ITEMTYPE_EQU_ACC1:		/* 装備(首飾り) */
				{
					PITEMINFO_EQU_ACC1 pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += pItem->byData;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += pItem->byData;
						break;
					}
				}
				break;
			case ITEMTYPE_EQU_ACC2:		/* 装備(指輪1) */
				{
					PITEMINFO_EQU_ACC2 pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += pItem->byData;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += pItem->byData;
						break;
					}
				}
				break;
			case ITEMTYPE_EQU_ACC3:		/* 装備(指輪2) */
				{
					PITEMINFO_EQU_ACC3 pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += pItem->byData;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += pItem->byData;
						break;
					}
				}
				break;
			}
		}
	}

	/* 酔いレベルの調整 */
	pChar->byDrunkPoint = min (pChar->byDrunkPoint, (CHARSTATEDRUNK_MAX - 1) * 10);
	pChar->byDrunkLV = pChar->byDrunkPoint / 10;
}

/* ========================================================================= */
/* 関数名：RenewCharPosInfo													 */
/* 内容：キャラ座標情報更新													 */
/* 日付：2004/07/03															 */
/* ========================================================================= */
void RenewCharPosInfo(void)
{
	int i, nNo, nCount;

	/* まずは消去 */
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		g_aCharPos[i].RemoveAll ();
	}

	if (g_nMyCharID < 0) {
		return;
	}

	PCHARINFO pMyChar = g_aCharInfo[g_nMyCharID];

	nCount = g_aCharInfo.GetSize ();

	ZeroMemory (g_pMapDarkTmp, SCRMARTSSIZE);

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar;

		pChar = g_aCharInfo[i];
		nNo = DRAW_PARTS_Y + 1;
		nNo += (pChar->dwY - pMyChar->dwY);

		if (nNo < 0 || nNo >= DRAW_PARTS_Y * 2 + 3) {
			continue;
		}

		/* 依頼の見える物（宝箱・光）は同じ行の中で先に描く（上に乗った人が宝箱に隠れていた） */
		if (pChar->byNpcType == NPCTYPE_QUESTNPC) {
			g_aCharPos[nNo].InsertAt (0, i);
		} else {
			g_aCharPos[nNo].Add (i);
		}
		if (pMyChar != pChar) {
			RenewMapDark (pChar);
		}
	}
	RenewMapDark (NULL);
	RenewMapDarkParts ();
	RenewMapDarkHideArea ();
}

/* ========================================================================= */
/* 関数名：RenewMapDarkHideArea												 */
/* 内容：「明るく壁で隠す」範囲にいる時は、範囲の中を壁や家具まで全部見せる	 */
/* ========================================================================= */
static BYTE s_abyHideShow[SCRMARTSSIZE];								/* 隠す範囲の中で全部見せるマス(画面のマス) */

/* 隠す範囲の中で全部見せるマス(DrawMapLight で全部明るく描く)。範囲の外にいる時は使わない */
BOOL IsMapHideShowTile(int nPos)
{
	if ((nPos < 0) || (nPos >= SCRMARTSSIZE)) {
		return FALSE;
	}
	return s_abyHideShow[nPos] ? TRUE : FALSE;
}

void RenewMapDarkHideArea(void)
{
	int x, y, mx, my, nPos, nMyNo, nWidth;
	const MAPDARKAREA *pArea;
	PCHARINFO pMyChar;

	ZeroMemory (s_abyHideShow, sizeof (s_abyHideShow));
	if ((g_nMyCharID < 0) || !IsMapHideArea ()) {
		return;
	}
	pMyChar = g_aCharInfo[g_nMyCharID];
	nMyNo = GetMapHideAreaNo (g_pMapInfo, pMyChar->dwX, pMyChar->dwY);
	nWidth = DRAW_PARTS_X * 2 + 3;
	/* 自分のいる範囲の四角の中のマス(重なっている所は優先されない範囲でも見せる。
	   隣の部屋との間の壁を両方の範囲に入れておけば、どちらの部屋からも見える) */
	pArea = &g_pMapInfo->aDarkArea[nMyNo - 1];
	for (y = 0; y < DRAW_PARTS_Y * 2 + 3; y ++) {
		my = (int)pMyChar->dwY + y - (1 + DRAW_PARTS_Y);
		for (x = 0; x < nWidth; x ++) {
			mx = (int)pMyChar->dwX + x - (1 + DRAW_PARTS_X);
			if ((mx < 0) || (my < 0) || (mx >= g_pMapInfo->nCX) || (my >= g_pMapInfo->nCY)) {
				continue;
			}
			if ((mx >= pArea->wX1) && (mx <= pArea->wX2) && (my >= pArea->wY1) && (my <= pArea->wY2)) {
				s_abyHideShow[x + nWidth * y] = 1;
			}
		}
	}
	for (nPos = 0; nPos < SCRMARTSSIZE; nPos ++) {
		if (s_abyHideShow[nPos]) {
			g_pMapDarkTmp[nPos] = 100;
			g_pMapDarkTmpPos[nPos] = 1;
		}
	}
}


/* ========================================================================= */
/* 関数名：	RenewUseMagicInfo												 */
/* 内容：	使用できる魔法一覧更新											 */
/* 日付：	2004/07/12														 */
/* ========================================================================= */

void RenewUseMagicInfo(void)
{
	int i, nCount;
	PCHARINFO pChar;

	if (g_nMyCharID < 0) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	g_aMagicInfoWalk.RemoveAll ();
	g_aMagicInfoBattle.RemoveAll ();

	nCount = g_aMagicInfo.GetSize ();
	if (nCount <= 0) {
		return;
	}

	/* 平常状態で使える魔法一覧作成 */
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		PMAGICINFO pInfo;

		if (pChar->abyMagic[i] == 0) {
			break;
		}

		pInfo = g_aMagicInfo[pChar->abyMagic[i]];
		switch (pInfo->byTypeInfoID) {
		case MAGICUSETYPE_WALK:			/* 平常状態 */
		case MAGICUSETYPE_WALKBATTLE:	/* 平常状態と戦闘状態 */
			g_aMagicInfoWalk.Add (pChar->abyMagic[i]);
			break;
		}
	}

	/* 戦闘状態で使える魔法一覧作成 */
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		PMAGICINFO pInfo;

		if (pChar->abyMagic[i] == 0) {
			break;
		}

		pInfo = g_aMagicInfo[pChar->abyMagic[i]];
		switch (pInfo->byTypeInfoID) {
		case MAGICUSETYPE_BATTLE:		/* 戦闘状態 */
		case MAGICUSETYPE_WALKBATTLE:	/* 平常状態と戦闘状態 */
			g_aMagicInfoBattle.Add (pChar->abyMagic[i]);
			break;
		}
	}
}

/* ========================================================================= */
/* 関数名：RenewMapDark														 */
/* 内容：暗闇描画情報を更新													 */
/* 日付：2004/10/08															 */
/* ========================================================================= */
/* 関数名：GetMyPosDarkType													 */
/* 内容：自分のいる所の範囲の種類(MAPDARK_*)								 */
/*       範囲の無いマップはどこでも暗い(マップ全体が明るさの設定どおり)		 */
/* ========================================================================= */
static BYTE GetMyPosDarkType(void)
{
	PCHARINFO pChar;

	if (g_pMapInfo->byDarkAreaCount == 0) {
		return MAPDARK_DARK;
	}
	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return MAPDARK_DARK;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	return GetMapDarkType (g_pMapInfo, pChar->dwX, pChar->dwY);
}


/* 灯りの元(nFromX,nFromY)から見て (x,y) が別の「明るく壁で隠す」範囲(または範囲の外)か
   灯りは境目を越えた1マス目(境目の壁・カーテン・扉)までは照らし、その先へは広がらない */
static BOOL IsHideAreaCross(int nFromX, int nFromY, int x, int y)
{
	if (!IsMapHasHideArea ()) {
		return FALSE;
	}
	return (GetMapHideAreaNo (g_pMapInfo, nFromX, nFromY) != GetMapHideAreaNo (g_pMapInfo, x, y)) ? TRUE : FALSE;
}

/* マップに「明るく壁で隠す」範囲があるか */
static BOOL IsMapHasHideArea(void)
{
	int i;

	for (i = 0; (i < g_pMapInfo->byDarkAreaCount) && (i < MAX_MAPDARKAREA); i ++) {
		if (g_pMapInfo->aDarkArea[i].byDark == MAPDARK_HIDE) {
			return TRUE;
		}
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：IsMapHideArea													 */
/* 内容：自分のいる所が「明るい。ただし壁の向こうは見えない」範囲か			 */
/* ========================================================================= */
BOOL IsMapHideArea(void)
{
	return (GetMyPosDarkType () == MAPDARK_HIDE) ? TRUE : FALSE;
}


/* ========================================================================= */
/* 関数名：GetMapLightLevel													 */
/* 内容：今の場所で使うマップの明るさレベル(暗くしない場所なら 0)			 */
/* ========================================================================= */
BYTE GetMapLightLevel(void)
{
	switch (GetMyPosDarkType ()) {
	case MAPDARK_BRIGHT:
		return 0;
	case MAPDARK_HIDE:
		/* 範囲の中は RenewMapDarkHideArea・DrawMapLight で全部明るくする。
		   範囲の外はマップの暗さどおり(明るいマップなら見えない) */
		return g_pMapInfo->byLightLevel ? g_pMapInfo->byLightLevel : 100;
	}
	return g_pMapInfo->byLightLevel;
}


/* ========================================================================= */
/* 関数名：IsMapDarkFlg														 */
/* 内容：今の場所で「暗闇」(壁の向こうが見えない)が効くか					 */
/* ========================================================================= */
BOOL IsMapDarkFlg(void)
{
	switch (GetMyPosDarkType ()) {
	case MAPDARK_BRIGHT:
		return FALSE;
	case MAPDARK_HIDE:
		return TRUE;		/* 壁や扉の向こうは見えない */
	}
	return (g_pMapInfo->byFlg & MAPFLG_DARK) ? TRUE : FALSE;
}


/* ========================================================================= */
/* 見える範囲(視界)のマップ													 */
/*   暗くはならないが、自分から見える範囲の外と壁の向こうは見えない。		 */
/*   一度見たマスは覚えておき、見えていない間は薄暗く描く					 */
/*   (そこにいるキャラは見えない)											 */
/* ========================================================================= */
#define VIEW_SEEN_LEVEL		40											/* 一度見た所の明るさ */
#define VIEW_SEEN_LEVEL_PARTS	70										/* 一度見た所の明るさ(見えない所をパーツで描くマップ) */
#define VIEW_RANGE_MAX		DRAW_PARTS_X								/* 見える範囲の上限(画面の端まで) */

static PBYTE	s_pbyViewSeen = NULL;									/* 一度見たマス(1マス1バイト) */
static DWORD	s_dwViewSeenMapID = (DWORD)-1;							/* 記録しているマップ */
static int		s_nViewSeenCX = 0;										/* 記録しているマップの大きさ */
static int		s_nViewSeenCY = 0;

/* 今の場所で見える範囲(視界)が効くか(暗いマップでは灯りのほうを使う) */
BOOL IsMapViewMode(void)
{
	if ((g_pMapInfo == NULL) || (g_pMapInfo->byViewRange == 0)) {
		return FALSE;
	}
	if (GetMapLightLevel ()) {
		return FALSE;
	}
	return (GetMyPosDarkType () != MAPDARK_BRIGHT) ? TRUE : FALSE;
}

/* 見える範囲(マス) */
static int GetMapViewRange(void)
{
	return min ((int)g_pMapInfo->byViewRange, VIEW_RANGE_MAX);
}

/* 壁で視界(灯り)が止まるか */
static BOOL IsSightBlock(void)
{
	/* 「明るく壁で隠す」範囲があるマップ(建物の中)も、暗闇OFFのまま壁で灯りを止める
	   (止めないと壁の外の何もない所を通って、別の部屋の壁まで回り込んでいた) */
	return (IsMapDarkFlg () || IsMapViewMode () || IsMapHasHideArea ());
}

static BOOL s_bLightIgnoreViewRange = FALSE;							/* マップの光源で照らし中(見える範囲の外でも照らす) */

/* 自分から (x,y) が見える範囲の中か(丸く見えるように半マス足す) */
static BOOL IsInViewRange(int x, int y)
{
	PCHARINFO pMyChar;
	int dx, dy, nRange;

	if (!IsMapViewMode () || s_bLightIgnoreViewRange) {
		return TRUE;
	}
	pMyChar = g_aCharInfo[g_nMyCharID];
	dx = x - (int)pMyChar->dwX;
	dy = y - (int)pMyChar->dwY;
	nRange = GetMapViewRange ();
	return ((dx * dx + dy * dy) <= (nRange * nRange + nRange));
}

/* 自分からずれたマス(xTmp=自分のX-そのX)が暗くて(見えなくて)隠すか */
BOOL IsMapDarkHidePos(int xTmp, int yTmp)
{
	int nPos;
	char nDark;

	if (!GetMapLightLevel () && !IsMapViewMode ()) {
		return FALSE;
	}
	if ((abs (xTmp) > DRAW_PARTS_X + 1) || (abs (yTmp) > DRAW_PARTS_Y + 1)) {
		return TRUE;
	}
	nPos = (DRAW_PARTS_X + 1 - xTmp) + (DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp);
	nDark = g_pMapDarkTmp[nPos];
	if (IsMapViewMode ()) {
		/* 一度見ただけの所(薄暗い)にいるキャラも見えない */
		return (nDark < 100);
	}
	return (nDark <= 1);
}

/* ========================================================================= */
/* 橋(立体交差)																 */
/*   地面から橋の向きに乗った人は橋の上、横切って入った人は橋の下を通る。	 */
/*   上か下かは橋から降りるまで覚えておく(広い橋の上で横にずれられる)		 */
/* ========================================================================= */

/* マス(x,y)の橋の向き(1=縦に渡る 2=横に渡る 0=橋ではない) */
static int GetMapBridgeAxis(int x, int y)
{
	WORD wParts;

	if ((g_pMapInfo == NULL) || (g_pMapInfo->pwMap == NULL) || (g_pMapPartsInfo == NULL)) {
		return 0;
	}
	if ((x < 0) || (x >= g_pMapInfo->nCX) || (y < 0) || (y >= g_pMapInfo->nCY)) {
		return 0;
	}
	wParts = g_pMapInfo->pwMap[x + y * g_pMapInfo->nCX];
	if (wParts >= g_wCountMapParts) {
		return 0;
	}
	return GetBridgeAxis (g_pMapPartsInfo[wParts].byHitType);
}

/* 今いるマスで橋の上(1)か下(2)か(0=橋ではない)
   覚えている時はそれを、覚えていない時(画面に入ってきた・ログインした)は向きで決める */
static int GetCharBridgeLayerNow(PCHARINFO pChar)
{
	int nAxis;

	nAxis = GetMapBridgeAxis ((int)pChar->dwX, (int)pChar->dwY);
	if (nAxis == 0) {
		return 0;
	}
	if (pChar->byBridgeLayer && (g_pMapInfo->dwID == pChar->dwBridgeMapID) &&
		(pChar->dwX == pChar->dwBridgeX) && (pChar->dwY == pChar->dwBridgeY)) {
		return pChar->byBridgeLayer;
	}
	/* 向きで決めたらすぐ覚える(自分のキャラはキーを押すと先に向きが変わるので、
	   そのまま向きで決め直すと、橋の下でログインして縦に歩くと上に変わってしまう) */
	pChar->byBridgeLayer	= (BYTE)GetBridgeLayerByDirection (nAxis, pChar->byDirection);
	pChar->byBridgeLayerBack = 0;
	pChar->dwBridgeMapID	= g_pMapInfo->dwID;
	pChar->dwBridgeX		= pChar->dwX;
	pChar->dwBridgeY		= pChar->dwY;
	return pChar->byBridgeLayer;
}

/* キャラが (x,y) へ動いた後に橋の上(1)か下(2)か */
int GetBridgeLayerPos(PCHARINFO pChar, int x, int y)
{
	int nAxisFrom, nAxisTo, nMoveAxis, dx, dy;

	if (pChar == NULL) {
		return 0;
	}
	nAxisTo = GetMapBridgeAxis (x, y);
	if (nAxisTo == 0) {
		return 0;
	}
	dx = x - (int)pChar->dwX;
	dy = y - (int)pChar->dwY;
	if (abs (dx) + abs (dy) != 1) {
		/* となりではない(ワープ・初めて見た)時は向きで決める */
		return GetBridgeLayerByDirection (nAxisTo, pChar->byDirection);
	}
	nAxisFrom = GetMapBridgeAxis ((int)pChar->dwX, (int)pChar->dwY);
	nMoveAxis = (dy != 0) ? 1 : 2;
	return GetBridgeLayerNext (GetCharBridgeLayerNow (pChar), nAxisFrom, nAxisTo, nMoveAxis);
}

/* キャラが (x,y) へ動く時に、橋の上か下かを覚え直す(位置を変える前に呼ぶ) */
void RenewCharBridgeLayer(PCHARINFO pChar, int x, int y)
{
	if ((pChar == NULL) || (g_pMapInfo == NULL)) {
		return;
	}
	pChar->byBridgeLayerBack = (BYTE)GetCharBridgeLayerNow (pChar);
	pChar->byBridgeLayer	= (BYTE)GetBridgeLayerPos (pChar, x, y);
	pChar->dwBridgeMapID	= g_pMapInfo->dwID;
	pChar->dwBridgeX		= (DWORD)x;
	pChar->dwBridgeY		= (DWORD)y;
}

/* キャラが橋の上(1)か下(2)か(0=橋とは関係ない)
   橋のマスにいる時と、橋のマスから出ていく途中(歩いている間)は橋の扱い */
int GetCharBridgeLayer(PCHARINFO pChar)
{
	int nLayer;

	if ((pChar == NULL) || (g_pMapInfo == NULL)) {
		return 0;
	}
	nLayer = GetCharBridgeLayerNow (pChar);
	if ((nLayer == 0) && pChar->byDotMove && pChar->byBridgeLayerBack &&
		(g_pMapInfo->dwID == pChar->dwBridgeMapID) &&
		(pChar->dwX == pChar->dwBridgeX) && (pChar->dwY == pChar->dwBridgeY)) {
		nLayer = pChar->byBridgeLayerBack;
	}
	return nLayer;
}

/* 橋のマスからその向きへ進めないか
   (上を渡っている人は橋から横へ落ちない・下をくぐっている人は橋の端から上へ出ない) */
BOOL IsBridgeMoveBlock(PCHARINFO pChar, BYTE byDirection)
{
	int x, y, nAxisFrom;

	if ((pChar == NULL) || (byDirection >= 4) || (g_pMapInfo == NULL)) {
		return FALSE;
	}
	nAxisFrom = GetMapBridgeAxis ((int)pChar->dwX, (int)pChar->dwY);
	if (nAxisFrom == 0) {
		return FALSE;
	}
	x = (int)pChar->dwX + anPosAddTblX[byDirection];
	y = (int)pChar->dwY + anPosAddTblY[byDirection];
	return IsBridgeMoveBlock (GetCharBridgeLayerNow (pChar), nAxisFrom,
			GetMapBridgeAxis (x, y), GetDirectionAxis (byDirection));
}

/* 一度見た所の記録を消す(マップを受け取った時。迷路の作り直しで形が変わるので) */
void ResetMapViewSeen(void)
{
	SAFE_DELETE_ARRAY (s_pbyViewSeen);
	s_dwViewSeenMapID = (DWORD)-1;
	s_nViewSeenCX = s_nViewSeenCY = 0;
}

/* 見えているマスを覚え、見えていないが一度見たマスを薄暗くする */
static void RenewMapViewSeen(void)
{
	int x, y, mx, my, nPos, nWidth;
	PCHARINFO pMyChar;

	pMyChar = g_aCharInfo[g_nMyCharID];
	if ((s_pbyViewSeen == NULL) || (s_dwViewSeenMapID != g_pMapInfo->dwID) ||
		(s_nViewSeenCX != g_pMapInfo->nCX) || (s_nViewSeenCY != g_pMapInfo->nCY)) {
		ResetMapViewSeen ();
		if ((g_pMapInfo->nCX <= 0) || (g_pMapInfo->nCY <= 0)) {
			return;
		}
		s_pbyViewSeen = new BYTE[g_pMapInfo->nCX * g_pMapInfo->nCY];
		ZeroMemory (s_pbyViewSeen, g_pMapInfo->nCX * g_pMapInfo->nCY);
		s_dwViewSeenMapID = g_pMapInfo->dwID;
		s_nViewSeenCX = g_pMapInfo->nCX;
		s_nViewSeenCY = g_pMapInfo->nCY;
	}

	nWidth = DRAW_PARTS_X * 2 + 3;
	for (y = 0; y < DRAW_PARTS_Y * 2 + 3; y ++) {
		my = (int)pMyChar->dwY + y - (1 + DRAW_PARTS_Y);
		if ((my < 0) || (my >= g_pMapInfo->nCY)) {
			continue;
		}
		for (x = 0; x < nWidth; x ++) {
			mx = (int)pMyChar->dwX + x - (1 + DRAW_PARTS_X);
			if ((mx < 0) || (mx >= g_pMapInfo->nCX)) {
				continue;
			}
			nPos = x + nWidth * y;
			if (g_pMapDarkTmp[nPos] > 1) {
				g_pMapDarkTmp[nPos] = 100;
				s_pbyViewSeen[mx + my * g_pMapInfo->nCX] = 1;
			} else if (s_pbyViewSeen[mx + my * g_pMapInfo->nCX]) {
				/* 見えない所をパーツで描くマップは、一度見た所を少しだけ暗く(暗くしすぎると穴のように見える) */
				g_pMapDarkTmp[nPos] = g_pMapInfo->wViewHideParts ? VIEW_SEEN_LEVEL_PARTS : VIEW_SEEN_LEVEL;
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：RenewMapViewLights												 */
/* 内容：見える範囲(視界)のマップで、マップの光源が照らす所も見えるようにする */
/*       (カウンターの中など、壁や家具の向こうでも灯りがあれば見える)		 */
/* ========================================================================= */
static void RenewMapViewLights(void)
{
	int i, x, y, nRange;
	PCHARINFO pMyChar;

	pMyChar = g_aCharInfo[g_nMyCharID];
	for (i = 0; (i < g_pMapInfo->byLightCount) && (i < MAX_MAPLIGHT); i ++) {
		PMAPLIGHT pLight = &g_pMapInfo->aLight[i];

		nRange = min ((int)pLight->byRange, MAPLIGHT_RANGE_MAX);
		if (nRange <= 0) {
			continue;
		}
		if ((pLight->wX >= g_pMapInfo->nCX) || (pLight->wY >= g_pMapInfo->nCY)) {
			continue;
		}
		if (IsLightOtherRoom (pLight->wX, pLight->wY)) {
			continue;
		}
		x = (int)pLight->wX - (int)pMyChar->dwX;
		y = (int)pLight->wY - (int)pMyChar->dwY;
		/* 灯りが画面に届かない？ */
		if ((abs (x) > DRAW_PARTS_X + 1 + nRange) || (abs (y) > DRAW_PARTS_Y + 1 + nRange)) {
			continue;
		}
		s_nPartsLightX = pLight->wX;
		s_nPartsLightY = pLight->wY;
		s_nPartsLightRange = nRange;
		s_bLightIgnoreViewRange = TRUE;
		ZeroMemory (g_pMapDarkTmpPos, SCRMARTSSIZE);
		RenewMapDarkLightFlood (pLight->wX, pLight->wY, nRange);
		RenewMapDarkCorner (NULL);
		s_bLightIgnoreViewRange = FALSE;
	}
}


/* ========================================================================= */
void RenewMapDark(PCHARINFO pChar)
{
	if (g_nMyCharID < 0) {
		return;
	}

	if (pChar == NULL) {
		pChar = g_aCharInfo[g_nMyCharID];
	}
	/* 自分と違う部屋(隠す範囲)にいる人の灯りは見えない */
	if ((pChar != g_aCharInfo[g_nMyCharID]) && IsLightOtherRoom ((int)pChar->dwX, (int)pChar->dwY)) {
		return;
	}

	/* 見える範囲(視界)のマップは自分から見える所だけ(ほかの人の視界は使わない) */
	if (IsMapViewMode ()) {
		if (pChar != g_aCharInfo[g_nMyCharID]) {
			return;
		}
		ZeroMemory (g_pMapDarkTmpPos, SCRMARTSSIZE);
		RenewMapDarkWork (pChar, pChar->dwX, pChar->dwY, DRAW_PARTS_X);
		RenewMapDarkCorner (pChar);
		RenewMapViewLights ();
		RenewMapViewSeen ();
		return;
	}

	/* 暗闇表示ではない？ */
	if ((!(IsMapDarkFlg ())) && (GetMapLightLevel () == 0)) {
		return;
	}
	ZeroMemory (g_pMapDarkTmpPos, SCRMARTSSIZE);

	RenewMapDarkWork (pChar, pChar->dwX, pChar->dwY, DRAW_PARTS_X);
	RenewMapDarkCorner (pChar);
}

/* ========================================================================= */
/* 関数名：RenewMapDarkParts												 */
/* 内容：光るマップパーツ(たいまつ・かがり火など)で照らす					 */
/*       パーツの「灯りの範囲」が 1 以上なら、そのマスから範囲のマスまで	 */
/*       キャラの灯りと同じように照らす(壁や閉まった扉の向こうは暗いまま)	 */
/* ========================================================================= */
void RenewMapDarkParts(void)
{
	int x, y, mx, my;
	PCHARINFO pMyChar;

	if (g_nMyCharID < 0) {
		return;
	}
	/* 明るさレベルのある暗いマップだけ */
	if (GetMapLightLevel () == 0) {
		return;
	}
	if ((g_pMapInfo->pwMap == NULL) && (g_pMapInfo->byLightCount == 0)) {
		return;
	}
	pMyChar = g_aCharInfo[g_nMyCharID];

	/* 画面の外にある灯りでも、照らす範囲が画面に入っていれば使う */
	for (y = -(DRAW_PARTS_Y + 1 + MAPLIGHT_RANGE_MAX); y <= DRAW_PARTS_Y + 1 + MAPLIGHT_RANGE_MAX; y ++) {
		my = (int)pMyChar->dwY + y;
		if ((my < 0) || (my >= g_pMapInfo->nCY)) {
			continue;
		}
		for (x = -(DRAW_PARTS_X + 1 + MAPLIGHT_RANGE_MAX); x <= DRAW_PARTS_X + 1 + MAPLIGHT_RANGE_MAX; x ++) {
			int nRange;

			mx = (int)pMyChar->dwX + x;
			if ((mx < 0) || (mx >= g_pMapInfo->nCX)) {
				continue;
			}
			nRange = GetMapPosLight (mx, my, NULL, NULL);
			if (nRange == 0) {
				continue;
			}
			if (IsLightOtherRoom (mx, my)) {
				continue;
			}
			s_nPartsLightX = mx;
			s_nPartsLightY = my;
			s_nPartsLightRange = nRange;
			/* 灯りが画面に届かない？ */
			if ((abs (x) > DRAW_PARTS_X + 1 + nRange) || (abs (y) > DRAW_PARTS_Y + 1 + nRange)) {
				continue;
			}
			ZeroMemory (g_pMapDarkTmpPos, SCRMARTSSIZE);
			RenewMapDarkLightFlood (mx, my, nRange);
			RenewMapDarkCorner (NULL);
		}
	}
}

/* ========================================================================= */
/* 関数名：IsLightOtherRoom													 */
/* 内容：マス(x,y)の灯りが自分と違う部屋(「明るく壁で隠す」範囲)にあるか	 */
/*       部屋の中の灯りは外から見えない・外の灯りは部屋の中から見えない		 */
/*       (範囲の外どうしは同じ部屋あつかい)									 */
/* ========================================================================= */
BOOL IsLightOtherRoom(int x, int y)
{
	PCHARINFO pMyChar;

	if ((g_nMyCharID < 0) || !IsMapHasHideArea ()) {
		return FALSE;
	}
	pMyChar = g_aCharInfo[g_nMyCharID];
	return (GetMapHideAreaNo (g_pMapInfo, x, y) != GetMapHideAreaNo (g_pMapInfo, pMyChar->dwX, pMyChar->dwY));
}

/* ========================================================================= */
/* 関数名：RenewMapDarkLightFlood											 */
/* 内容：灯り(パーツ・光源)の位置から、通れるマスを伝って照らす			 */
/*       灯りが画面の外にあっても、画面の外のマスを通って画面の中まで届く	 */
/*       (壁で止まる・範囲のマスまで、は RenewMapDarkWork と同じ)			 */
/* ========================================================================= */
static void RenewMapDarkLightFlood(int mx, int my, int nRange)
{
	enum { W = MAPLIGHT_RANGE_MAX * 2 + 1 };
	BYTE abyDone[W * W];
	int anQX[W * W], anQY[W * W];
	int nHead, nTail, i, nSize;
	PCHARINFO pMyChar;
	static const int anDX[4] = { 0, 0, -1, 1 }, anDY[4] = { -1, 1, 0, 0 };

	nRange = min (nRange, MAPLIGHT_RANGE_MAX);
	nSize = nRange * 2 + 1;
	ZeroMemory (abyDone, sizeof (abyDone));
	pMyChar = g_aCharInfo[g_nMyCharID];

	nHead = nTail = 0;
	anQX[nTail] = mx;
	anQY[nTail] = my;
	nTail ++;
	abyDone[nRange + nRange * nSize] = 1;

	while (nHead < nTail) {
		int x, y, dx, dy, nTmp, nPos;
		BOOL bWall;

		x = anQX[nHead];
		y = anQY[nHead];
		nHead ++;

		/* 見える範囲(視界)の外？ */
		if (!IsInViewRange (x, y)) {
			continue;
		}
		nTmp = GetMapDarkLevel (NULL, x, y);
		if (nTmp <= 1) {
			continue;
		}
		/* 壁(灯りのマス自身は壁に付いたたいまつでも照らす) */
		bWall = FALSE;
		if (((x != mx) || (y != my)) && IsSightBlock ()) {
			bWall = IsSightWall (x, y);
		}
		/* 隠す範囲の境目を越えたマスは照らすが、その先へは広げない */
		if (!bWall && IsHideAreaCross (mx, my, x, y)) {
			bWall = TRUE;
		}

		/* 画面の中なら明るさを足す */
		dx = x - (int)pMyChar->dwX;
		dy = y - (int)pMyChar->dwY;
		if ((abs (dx) <= DRAW_PARTS_X + 1) && (abs (dy) <= DRAW_PARTS_Y + 1)) {
			nPos = (1 + DRAW_PARTS_X + dx) + ((DRAW_PARTS_X * 2 + 3) * (1 + DRAW_PARTS_Y + dy));
			if (g_pMapDarkTmpPos[nPos] == 0) {
				if (g_pMapDarkTmp[nPos] + nTmp > 100) {
					nTmp = 100 - g_pMapDarkTmp[nPos];
				}
				g_pMapDarkTmp[nPos] += (char)nTmp;
			}
			/* 灯りが通ったマスは 2 (斜め隣の壁を照らす目印) */
			g_pMapDarkTmpPos[nPos] = bWall ? 1 : 2;
		}
		if (bWall) {
			continue;
		}

		for (i = 0; i < 4; i ++) {
			int nx = x + anDX[i], ny = y + anDY[i];
			int bx = nx - mx + nRange, by = ny - my + nRange;
			if ((bx < 0) || (by < 0) || (bx >= nSize) || (by >= nSize)) {
				continue;
			}
			if ((nx < 0) || (ny < 0) || (nx >= g_pMapInfo->nCX) || (ny >= g_pMapInfo->nCY)) {
				continue;
			}
			if (abyDone[bx + by * nSize]) {
				continue;
			}
			abyDone[bx + by * nSize] = 1;
			anQX[nTail] = nx;
			anQY[nTail] = ny;
			nTail ++;
		}
	}
}

/* ========================================================================= */
/* 関数名：GetMapPosLight													 */
/* 内容：マス(mx,my)の灯りの範囲(光るパーツとマップの光源の大きい方。0=無し) */
/*       pbyPower・pbyFlg に強さ(%)とフラグ(マップの光源のもの)を返す		 */
/* ========================================================================= */
int GetMapPosLight(int mx, int my, BYTE *pbyPower, BYTE *pbyFlg)
{
	int i, nRange;

	nRange = 0;
	if (pbyPower) {
		*pbyPower = 100;
	}
	if (pbyFlg) {
		*pbyFlg = 0;
	}
	if ((mx < 0) || (my < 0) || (mx >= g_pMapInfo->nCX) || (my >= g_pMapInfo->nCY)) {
		return 0;
	}
	if (g_pMapInfo->pwMap && g_pMapPartsInfo) {
		WORD wMap = g_pMapInfo->pwMap[mx + my * g_pMapInfo->nCX];
		if (wMap < g_wCountMapParts) {
			nRange = g_pMapPartsInfo[wMap].byLight;
		}
	}
	for (i = 0; (i < g_pMapInfo->byLightCount) && (i < MAX_MAPLIGHT); i ++) {
		PMAPLIGHT pLight = &g_pMapInfo->aLight[i];
		if ((pLight->wX == mx) && (pLight->wY == my)) {
			nRange = max (nRange, (int)pLight->byRange);
			if (pbyPower && pLight->byPower) {
				*pbyPower = min (pLight->byPower, 100);
			}
			if (pbyFlg) {
				*pbyFlg = pLight->byFlg;
			}
		}
	}
	return min (nRange, DRAW_PARTS_X);
}

/* ========================================================================= */
/* 関数名：GetMapDarkLevel													 */
/* 内容：灯り(pChar)から見たマス(x,y)の明るさを取得							 */
/*       1 以下なら暗くて見えない											 */
/* ========================================================================= */
int GetMapDarkLevel(PCHARINFO pChar, int x, int y)
{
	int nTmp, nLevel;
	PCHARINFO pMyChar = g_aCharInfo[g_nMyCharID];

	nTmp = 100;
	/* 「明るく壁で隠す」範囲の中にいる時は範囲の外は見えない(外の光源で照らされていても)。
	   境目を越えた灯りは IsHideAreaCross で1マス目(境目の壁・カーテン・扉)で止める */
	if (IsMapHasHideArea ()) {
		int nHere;

		nHere = GetMapHideAreaNo (g_pMapInfo, x, y);
		if (IsMapHideArea () && (nHere != GetMapHideAreaNo (g_pMapInfo, pMyChar->dwX, pMyChar->dwY))) {
			return 1;
		}
	}
	/* 光るマップパーツ？(範囲のマスまで見え、その先は暗い) */
	if (pChar == NULL) {
		nLevel = max (abs (x - s_nPartsLightX), abs (y - s_nPartsLightY));
		nTmp = 100 - 100 * nLevel / (s_nPartsLightRange + 1);
		nTmp = max (1, nTmp);
		nTmp = min (100, nTmp);
		return nTmp;
	}
	if (GetMapLightLevel ()) {
		nLevel = max (abs (x - (int)pChar->dwX), abs (y - (int)pChar->dwY));
		if (pChar != pMyChar) {
			nTmp = 0;
		} else if (g_pMapInfo->byFlg & MAPFLG_NOCHARLIGHT) {
			nTmp = (nLevel == 0) ? 60 : 0;	/* キャラの灯り無しのマップでは自分の足元だけ見える */
		} else {
			nTmp = 100 - GetMapLightLevel () * nLevel;
		}
		if (pChar->byLightLevel) {
			nTmp = 100 - (GetMapLightLevel () - pChar->byLightLevel) * nLevel;
		}
		nTmp = max (1, nTmp);
		nTmp = min (100, nTmp);
	}
	return nTmp;
}

/* ========================================================================= */
/* 関数名：RenewMapDarkCorner												 */
/* 内容：灯りの角の壁を照らす												 */
/*       灯りは通れるマスを伝って広がるので、斜めにある壁の角や				 */
/*       灯りの四隅の壁が真っ黒になり、壁の続きが見えなかった。				 */
/*       灯りが通ったマスの斜め隣までは照らして、壁の形が分かるようにする	 */
/* ========================================================================= */
void RenewMapDarkCorner(PCHARINFO pChar)
{
	int x, y, dx, dy, mx, my, nPos, nTmp, nWidth;
	PCHARINFO pMyChar;
	BOOL bLight;
	char abyAdd[SCRMARTSSIZE];

	/* 壁で灯りが止まるのは暗闇マップ・見える範囲(視界)のマップだけ。
	   「明るく壁で隠す」範囲があるマップは、範囲の境目で灯りが止まるので角も照らす */
	if (!(IsSightBlock ()) && !IsMapHasHideArea ()) {
		return;
	}

	pMyChar = g_aCharInfo[g_nMyCharID];
	nWidth = DRAW_PARTS_X * 2 + 3;
	ZeroMemory (abyAdd, sizeof (abyAdd));

	/* 画面の端の1マス(歩いてずれる時に見える所)も照らす */
	for (y = 0; y < DRAW_PARTS_Y * 2 + 3; y ++) {
		for (x = 0; x < nWidth; x ++) {
			nPos = x + nWidth * y;
			/* もう照らしたマス？ */
			if (g_pMapDarkTmpPos[nPos]) {
				continue;
			}
			/* 灯りが通ったマス(2)の斜め隣か調べる */
			bLight = FALSE;
			for (dy = -1; (dy <= 1) && (bLight == FALSE); dy ++) {
				for (dx = -1; dx <= 1; dx ++) {
					if ((x + dx < 0) || (x + dx >= nWidth) || (y + dy < 0) || (y + dy >= DRAW_PARTS_Y * 2 + 3)) {
						continue;
					}
					if (g_pMapDarkTmpPos[(x + dx) + nWidth * (y + dy)] == 2) {
						bLight = TRUE;
						break;
					}
				}
			}
			if (bLight == FALSE) {
				continue;
			}
			mx = (int)pMyChar->dwX + x - (1 + DRAW_PARTS_X);
			my = (int)pMyChar->dwY + y - (1 + DRAW_PARTS_Y);
			if (!IsInViewRange (mx, my)) {
				continue;
			}
			nTmp = GetMapDarkLevel (pChar, mx, my);
			if (GetMapLightLevel () && (nTmp <= 1)) {
				continue;
			}
			abyAdd[nPos] = (char)nTmp;
		}
	}

	/* 照らした結果を足す(足しながらだと照らした範囲が広がってしまうので後で) */
	for (nPos = 0; nPos < SCRMARTSSIZE; nPos ++) {
		if (abyAdd[nPos] == 0) {
			continue;
		}
		nTmp = abyAdd[nPos];
		if (g_pMapDarkTmp[nPos] + nTmp > 100) {
			nTmp = 100 - g_pMapDarkTmp[nPos];
		}
		g_pMapDarkTmp[nPos] += (char)nTmp;
		g_pMapDarkTmpPos[nPos] = 1;
	}

	/* 壁は「上のマス=天井・下のマス=側面」の縦2マスで描かれていることが多い。
	   灯りは側面(下)で止まるので天井(上)が真っ黒になり不自然だった。
	   灯りが当たった壁(斜めの角として照らした壁も)のすぐ上も壁なら、その1マスだけ照らす */
	ZeroMemory (abyAdd, sizeof (abyAdd));
	for (y = 1; y < DRAW_PARTS_Y * 2 + 3; y ++) {
		for (x = 0; x < nWidth; x ++) {
			int nPosUp;

			nPos = x + nWidth * y;
			/* 照らしていないマス・灯りが通った(壁ではない)マス？ */
			if ((g_pMapDarkTmpPos[nPos] == 0) || (g_pMapDarkTmpPos[nPos] == 2)) {
				continue;
			}
			nPosUp = nPos - nWidth;
			if (g_pMapDarkTmpPos[nPosUp]) {
				continue;
			}
			mx = (int)pMyChar->dwX + x - (1 + DRAW_PARTS_X);
			my = (int)pMyChar->dwY + y - (1 + DRAW_PARTS_Y);
			/* 自分が壁で、上も壁の時だけ */
			if (!IsSightWall (mx, my) || !IsSightWall (mx, my - 1)) {
				continue;
			}
			if (!IsInViewRange (mx, my - 1)) {
				continue;
			}
			nTmp = GetMapDarkLevel (pChar, mx, my - 1);
			if (GetMapLightLevel () && (nTmp <= 1)) {
				continue;
			}
			abyAdd[nPosUp] = (char)nTmp;
		}
	}
	for (nPos = 0; nPos < SCRMARTSSIZE; nPos ++) {
		if (abyAdd[nPos] == 0) {
			continue;
		}
		nTmp = abyAdd[nPos];
		if (g_pMapDarkTmp[nPos] + nTmp > 100) {
			nTmp = 100 - g_pMapDarkTmp[nPos];
		}
		g_pMapDarkTmp[nPos] += (char)nTmp;
		g_pMapDarkTmpPos[nPos] = 1;
	}
}

/* ========================================================================= */
/* 関数名：RenewMapDarkWork													 */
/* 内容：暗闇描画情報を更新処理												 */
/* 日付：2004/10/08															 */
/* ========================================================================= */
void RenewMapDarkWork(PCHARINFO pChar, int x, int y, int nCount)
{
	BOOL bResult;
	PCHARINFO pMyChar;
	int nTmp, nPos, nMapDarkTmpPos;

	if (nCount <= 0) {
		return;
	}

	pMyChar = g_aCharInfo[g_nMyCharID];
	/* 画面の端の1マス(歩いてずれる時に見える所)まで */
	if ((abs ((int)pMyChar->dwX - (x)) > DRAW_PARTS_X + 1) ||
		(abs ((int)pMyChar->dwY - (y)) > DRAW_PARTS_Y + 1)) {
		return;
	}

	/* 見える範囲(視界)の外？ */
	if (!IsInViewRange (x, y)) {
		return;
	}
	nTmp = GetMapDarkLevel (pChar, x, y);
	if (GetMapLightLevel () && (nTmp == 1)) {
		return;
	}

	nPos = (1 + DRAW_PARTS_X + (x - pMyChar->dwX)) +
		   ((DRAW_PARTS_X * 2 + 3) * (1 + DRAW_PARTS_Y + (y - pMyChar->dwY)));
	nMapDarkTmpPos = g_pMapDarkTmpPos[nPos];

	if (g_pMapDarkTmp[nPos] + (char)nTmp > 100) {
		nTmp = 100 - g_pMapDarkTmp[nPos];
	}

	bResult = TRUE;
	if ((nCount < DRAW_PARTS_X) && (IsSightBlock ())) {
		bResult = !IsSightWall (x, y);
	}
	/* 隠す範囲の境目を越えたマスは照らすが、その先へは広げない */
	if (bResult && IsHideAreaCross ((int)pChar->dwX, (int)pChar->dwY, x, y)) {
		bResult = FALSE;
	}
	if (bResult == FALSE) {
		if (nMapDarkTmpPos == 0) {
			g_pMapDarkTmp[nPos] += nTmp;
		}
		g_pMapDarkTmpPos[nPos] = 1;
		return;
	}

	if (g_pMapDarkTmp[nPos] < 0) {
		g_pMapDarkTmp[nPos] = 1;
		return;
	}

	if (nMapDarkTmpPos == 0) {
		g_pMapDarkTmp[nPos] += (char)nTmp;
	}

	/* 灯りが通ったマスは 2 (斜め隣の壁を照らす目印) */
	g_pMapDarkTmpPos[nPos] = 2;

	RenewMapDarkWork (pChar, x, y - 1, nCount - 1);
	RenewMapDarkWork (pChar, x, y + 1, nCount - 1);
	RenewMapDarkWork (pChar, x - 1, y, nCount - 1);
	RenewMapDarkWork (pChar, x + 1, y, nCount - 1);
}

/* ========================================================================= */
/* 関数名：CheckEquUserInfo													 */
/* 内容：装備職業情報にあるか判定											 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
BOOL CheckEquUserInfo(WORD wNo, BYTE byJobID)
{
	BOOL bRet;
	int i, nCount;
	PEQUUSERINFO pInfo;

	bRet = FALSE;
	if (wNo >= g_aEquUserInfo.GetSize ()) {
		goto Exit;
	}

	pInfo = g_aEquUserInfo[wNo];
	nCount = pInfo->byCount;

	for (i = 0; i < nCount; i ++) {
		if (pInfo->paJobIDTbl[i] == byJobID) {
			break;
		}
	}

	/* 見つからなかった？ */
	if (i >= nCount) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckItemSexDistinction											 */
/* 内容：性別による使えるかチェック											 */
/* 日付：2004/07/18															 */
/* ========================================================================= */
BOOL CheckItemSexDistinction(PITEMINFO pItem, PCHARINFO pChar)
{
	BOOL bRet;

	bRet = FALSE;
	if (pItem->bySexDistinction == SEXDISTINCTION_ALL) {
		bRet = TRUE;

	/* 女性？ */
	} else if (pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE) {
		if (pItem->bySexDistinction == SEXDISTINCTION_FEMALE) {
			bRet = TRUE;
		}

	/* 男性 */
	} else {
		if (pItem->bySexDistinction == SEXDISTINCTION_MALE) {
			bRet = TRUE;
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckCharBlock													 */
/* 内容：キャラのぶつかり判定												 */
/* 日付：2003/10/11															 */
/* ========================================================================= */
BOOL CheckCharBlock(DWORD dwX, DWORD dwY)
{
	BYTE byHitType;
	int nPartsID, nX, nY;
	PMAPINFO pMap;
	PMAPPARTSINFO pInfo;

	nX = (int)dwX;
	nY = (int)dwY;

	pMap = g_pMapInfo;
	if ((nX < 0) || (nX >= pMap->nCX) || (nY < 0) || (nY >= pMap->nCY)) {
		return FALSE;
	}

	nPartsID = pMap->pwMap[nX + nY * pMap->nCX];
	if (nPartsID >= g_wCountMapParts) {
		return FALSE;
	}

	pInfo = &g_pMapPartsInfo[nPartsID];
	byHitType = pInfo->byHitType;

	switch (byHitType) {
	case PARTSHIT_BLOCK1:
	case PARTSHIT_HIDE2:
	case PARTSHIT_HIDE4:
	case PARTSHIT_HIDE6:
	case PARTSHIT_COUNTER:
	case PARTSHIT_BLOCK2:
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：IsSightWall														 */
/* 内容：灯りを止めるマス？(ぶつかるマスのうちカウンターは灯りを通す)		 */
/* 日付：2026/11/09															 */
/* ========================================================================= */
BOOL IsSightWall(int x, int y)
{
	int nPartsID;
	PMAPINFO pMap;

	if (CheckCharBlock ((DWORD)x, (DWORD)y)) {
		return FALSE;
	}
	pMap = g_pMapInfo;
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return TRUE;
	}
	nPartsID = pMap->pwMap[x + y * pMap->nCX];
	if (nPartsID >= g_wCountMapParts) {
		return TRUE;
	}
	/* カウンターは低い家具なので灯りが越えていく */
	if (g_pMapPartsInfo[nPartsID].byHitType == PARTSHIT_COUNTER) {
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：CheckCharBlockPC													 */
/* 内容：キャラのキャラとのぶつかり判定										 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
int CheckCharBlockPC(DWORD dwX, DWORD dwY)
{
	int i, nCount;

	nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		/* キャラと重なる？ */
		if ((pCharTmp->dwX == dwX) && (pCharTmp->dwY == dwY)) {
			return (int)pCharTmp->dwTblID;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：CheckBlockEventFront												 */
/* 内容：1歩前にイベントがあるか判定										 */
/* 日付：2005/03/13															 */
/* 戻り値：TRUE:イベントあり FALSE:イベント無し								 */
/* ========================================================================= */
BOOL CheckBlockEventFront(PCHARINFO pChar, int x, int y, BYTE byDirection)
{
	BOOL bRet;
	int xTmp, yTmp;

	bRet = FALSE;

	xTmp = x + anPosAddTblX[byDirection];
	yTmp = y + anPosAddTblY[byDirection];

	/* はみ出しチェック */
	if ((xTmp < 0) || (xTmp >= g_pMapInfo->nCX) ||
		(yTmp < 0) || (yTmp >= g_pMapInfo->nCY)) {
		goto Exit;
	}

	/* イベントのチェック */
	if ((g_pMapInfo->pbyFlg[xTmp + g_pMapInfo->nCX * yTmp] & MAPFLG_EVENT) == 0) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	IsBlock															 */
/* 内容：	ぶつかり判定													 */
/* 日付：	2006/01/19														 */
/* 戻り値：	TRUE:ぶつかる FALSE:ぶつからない								 */
/* ========================================================================= */

BOOL IsBlock(
	int x,			/* [in] 座標(横) */
	int y,			/* [in] 座標(縦) */
	BOOL bHide)		/* [in] TRUE:隠れるパーツをぶつかり判定にする */
{
	BOOL bRet;
	BYTE byHitType;
	int nPartsID;
	PMAPINFO pMap;
	PMAPPARTSINFO pInfo;

	bRet = TRUE;
	pMap = g_pMapInfo;

	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		goto Exit;
	}

	nPartsID = pMap->pwMap[x + y * pMap->nCX];
	if (nPartsID >= g_wCountMapParts) {
		goto Exit;
	}

	pInfo		= &g_pMapPartsInfo[nPartsID];
	byHitType	= pInfo->byHitType;

	switch (byHitType) {
	case PARTSHIT_BLOCK1:						/* ぶつかる(音付き) */
	case PARTSHIT_BLOCK2:						/* ぶつかる(音無し) */
	case PARTSHIT_COUNTER:						/* カウンター */
	case PARTSHIT_HIDE2:						/* 隠れる(ぶつかる) */
	case PARTSHIT_HIDE4:						/* 隠れる4(ぶつかる) */
	case PARTSHIT_HIDE6:						/* 隠れる6(ぶつかる/音無し) */
		goto Exit;

	case PARTSHIT_HIDE1:						/* 隠れる */
	case PARTSHIT_HIDE3:						/* 隠れる3 */
	case PARTSHIT_HIDE5:						/* 隠れる5(背景ごと隠れる) */
	case PARTSHIT_HIDE7:						/* 隠れる7(隠れる3で背景ごと隠れる) */
		if (bHide) {
			goto Exit;
		}
		break;

	case PARTSHIT_NONE:							/* ぶつからない */
	case PARTSHIT_FOOTMARK_SNOW:				/* 足跡(雪) */
	case PARTSHIT_FOOTMARK_SAND:				/* 足跡(砂) */
	case PARTSHIT_MOVEFRONT:					/* 一歩前へ進む */
	case PARTSHIT_MOVEUP:						/* 一歩上へ進む */
	case PARTSHIT_MOVEDOWN:						/* 一歩下へ進む */
	case PARTSHIT_MOVERIGHT:					/* 一歩左へ進む */
	case PARTSHIT_MOVELEFT:						/* 一歩右へ進む */
	case PARTSHIT_WATERRIPPLE:					/* 水の波紋 */
		break;
	}

	bRet = FALSE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：CheckCharX														 */
/* 内容：キャラサイズに合わせた当たり判定(横)								 */
/* 日付：2004/09/27															 */
/* ========================================================================= */
BOOL CheckCharX(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget)
{
	BOOL bRet;

	bRet = FALSE;

	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		if (dwX != dwXTarget) {
			goto Exit;
		}
		break;

	case CHARGRP_MAIN_3232:
		if (!((dwX <= dwXTarget) && (dwXTarget <= dwX + 1))) {
			goto Exit;
		}
		break;

	case CHARGRP_MAIN_4848:
		if (!((dwX <= dwXTarget) && (dwXTarget <= dwX + 2))) {
			goto Exit;
		}
		break;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：GetCharCenterX													 */
/* 内容：キャラの中心位置を取得												 */
/* 日付：2004/09/29															 */
/* ========================================================================= */
int GetCharCenterX(PCHARINFO pChar)
{
	int nRet;

	nRet = 16;

	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		break;

	case CHARGRP_MAIN_3232:
		nRet = 32;
		break;

	case CHARGRP_MAIN_4848:
		nRet = 48;
		break;
	}

	return nRet;
}

/* ========================================================================= */
/* 関数名：CheckTargetPos													 */
/* 内容：指定座標までたどり着けるかチェック									 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
BOOL CheckTargetPos(int nXFrom, int nYFrom, int nXTo, int nYTo, int nCount)
{
	BOOL bRet;

	bRet = FALSE;

	CheckTargetPosWork (nXFrom, nYFrom, nXTo, nYTo, nCount, &bRet);
	if (bRet == FALSE) {
		CheckTargetPosWork (nXFrom - 1, nYFrom, nXTo, nYTo, nCount, &bRet);
	}
	if (bRet == FALSE) {
		CheckTargetPosWork (nXFrom + 1, nYFrom, nXTo, nYTo, nCount, &bRet);
	}
	if (bRet == FALSE) {
		CheckTargetPosWork (nXFrom, nYFrom - 1, nXTo, nYTo, nCount, &bRet);
	}
	if (bRet == FALSE) {
		CheckTargetPosWork (nXFrom, nYFrom + 1, nXTo, nYTo, nCount, &bRet);
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckTargetPosWork												 */
/* 内容：指定座標までたどり着けるかチェック処理								 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
void CheckTargetPosWork(int nXFrom, int nYFrom, int nXTo, int nYTo, int nCount, BOOL *pbCheck)
{
	BYTE byHitType;
	int nPartsID;
	PMAPPARTSINFO pInfo;

	if (*pbCheck == TRUE) {
		goto Exit;
	}
	if (nCount <= 0) {
		goto Exit;
	}

	/* 到達？ */
	if ((nXFrom == nXTo) & (nYFrom == nYTo)) {
		*pbCheck = TRUE;
		goto Exit;
	}

	if (nXFrom < 0 || nXFrom >= g_pMapInfo->nCX ||
		nYFrom < 0 || nYFrom >= g_pMapInfo->nCY) {
		goto Exit;
	}

	nPartsID = g_pMapInfo->pwMap[nXFrom + nYFrom * g_pMapInfo->nCX];
	pInfo = &g_pMapPartsInfo[nPartsID];
	byHitType = pInfo->byHitType;

	switch (byHitType) {
	case PARTSHIT_BLOCK1:
	case PARTSHIT_HIDE2:
	case PARTSHIT_HIDE4:
	case PARTSHIT_HIDE6:
	case PARTSHIT_COUNTER:
	case PARTSHIT_BLOCK2:
		goto Exit;
	}

	CheckTargetPosWork (nXFrom - 1, nYFrom, nXTo, nYTo, nCount - 1, pbCheck);
	CheckTargetPosWork (nXFrom + 1, nYFrom, nXTo, nYTo, nCount - 1, pbCheck);
	CheckTargetPosWork (nXFrom, nYFrom - 1, nXTo, nYTo, nCount - 1, pbCheck);
	CheckTargetPosWork (nXFrom, nYFrom + 1, nXTo, nYTo, nCount - 1, pbCheck);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	GetUseCharInfo													 */
/* 内容：	誰に使うか一覧を取得											 */
/* 日付：	2006/02/20														 */
/* 戻り値：	一覧に登録されたキャラ数										 */
/* ========================================================================= */

int GetUseCharInfo(
	int nArea,					/* [in] 範囲 */
	PCHARINFO pMyChar,			/* [in] 一覧に含めないキャラ情報 */
	BOOL bSelf,					/* [in] 自分を含めるか */
	int nType,					/* [in] 種別 */
	ARRAYUSECHARINFO &aDst,		/* [ou] 作成した一覧 */
	int &nLen)					/* [ou] 最大文字列長 */
{
	BOOL bResult;
	int i, nCount, nRet;
	PCHARINFO pCharTmp;
	PUSECHARINFO pInfo;

	nRet = 0;
	nLen = 0;

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[i];
		if ((bSelf == FALSE) && (pMyChar == pCharTmp)) {
			continue;
		}
		switch (pCharTmp->byNpcType) {
		case NPCTYPE_INIT:
			if (!((nType == MAGICTARGET_PC) || (nType == MAGICTARGET_ALL))) {
				continue;
			}
			break;
		case NPCTYPE_BATTLE1:				/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
		case NPCTYPE_BATTLE2:				/* 周り２キャラの範囲で検索してついていって攻撃 */
			if (!((nType == MAGICTARGET_NPC) || (nType == MAGICTARGET_ALL))) {
				continue;
			}
			break;
		default:
			continue;
		}
		bResult = CheckTargetPos ((int)pMyChar->dwX, (int)pMyChar->dwY,
				(int)pCharTmp->dwX, (int)pCharTmp->dwY, nArea);
		if (bResult == FALSE) {
			continue;
		}

		pInfo = new USECHARINFO;
		ZeroMemory (pInfo, sizeof (USECHARINFO));
		pInfo->dwTblID = pCharTmp->dwTblID;
		strcat (pInfo->szName, pCharTmp->szName);
		nLen = max (SboTextWidth (pInfo->szName), nLen);
		aDst.Add (pInfo);
	}

	nRet = aDst.GetSize ();
	return nRet;
}


/* ========================================================================= */
/* 関数名：	DeleteUseCharInfo												 */
/* 内容：	誰に使うか一覧を削除											 */
/* 日付：	2006/02/21														 */
/* ========================================================================= */

void DeleteUseCharInfo(PARRAYUSECHARINFO pSrc)
{
	int i, nCount;
	PUSECHARINFO pUseCharInfo;

	nCount = pSrc->GetSize ();
	for (i = 0; i < nCount; i ++) {
		pUseCharInfo = pSrc->GetAt (i);
		SAFE_DELETE (pUseCharInfo);
	}
	pSrc->RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	GetAttackType													 */
/* 内容：	攻撃種別を取得													 */
/* 日付：	2006/02/26														 */
/* ========================================================================= */

int GetAttackType(PCHARINFO pChar)
{
	int nRet;
	PITEMINFO pItem;
	PITEMINFO_WEAPON pWeapon;

	nRet = ATACKTYPE_NORMAL;

	/* 武器を装備していない？ */
	if (pChar->waEqu[0] == 0xFFFF) {
		goto Exit;
	}

	pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
	if (pItem->pData == NULL) {
		goto Exit;
	}

	pWeapon = (PITEMINFO_WEAPON)pItem->pData;

	switch (pWeapon->byAtackType) {
	case ATACKTYPE_ARROW:		/* 矢 */
		nRet = ATACKTYPE_ARROW;
		break;
	}

Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	StartTurnGage													 */
/* 内容：	ターンゲージ開始												 */
/* 日付：	2006/03/12														 */
/* ========================================================================= */

BOOL	g_bAutoGage				= FALSE;		/* 目の前の敵で自動で溜め始めたゲージ(相手を選んだゲージは FALSE) */
DWORD	g_dwAutoGageLastFront	= 0;			/* 自動ゲージで最後に目の前に敵がいた時間 */
DWORD	g_dwAutoGageSuppress	= (DWORD)-1;	/* Z でやめた相手(目の前から離れるまで自動で溜め直さない) */

void StartTurnGage(PCHARINFO pChar, DWORD dwAtackTblID)
{
	pChar->byTurnLV		= 0;
	pChar->dwAtackTblID	= dwAtackTblID;
	pChar->dwTimeTurnLV = timeGetTime ();
	g_bAutoGage			= FALSE;
}


/* ========================================================================= */
/* 関数名：	CancelTurnGage													 */
/* 内容：	ターンゲージをキャンセル										 */
/* 日付：	2006/03/12														 */
/* ========================================================================= */

void CancelTurnGage(PCHARINFO pChar)
{
	pChar->byTurnLV		= 0;
	pChar->dwAtackTblID	= 0;
	pChar->dwTimeTurnLV = 0;
}


/* ========================================================================= */
/* 関数名：	FindNextAttackTarget											 */
/* 内容：	連続攻撃の次の相手を探す										 */
/*			周り2マス以内にいる敵のうち、一番近いもの						 */
/*			戻り値：キャラ番号（いなければ -1）								 */
/* ========================================================================= */

int FindNextAttackTarget(PCHARINFO pChar)
{
	int i, nCount, nDist, nDistMin, nRet;
	PCHARINFO pCharTmp;

	nRet		= -1;
	nDistMin	= 3;
	nCount		= g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[i];
		if (pCharTmp == pChar) {
			continue;
		}
		if (!((pCharTmp->byNpcType == NPCTYPE_BATTLE1) || (pCharTmp->byNpcType == NPCTYPE_BATTLE2))) {
			continue;
		}
		if (pCharTmp->dwMapID != pChar->dwMapID) {
			continue;
		}
		if (pCharTmp->byCharState == CHARSTATE_DEAD) {
			continue;
		}
		nDist = abs ((int)pCharTmp->dwX - (int)pChar->dwX) + abs ((int)pCharTmp->dwY - (int)pChar->dwY);
		if (nDist < nDistMin) {
			nDistMin	= nDist;
			nRet		= i;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	FindFrontAttackTarget											 */
/* 内容：	目の前（向いている方向の1マス先）にいる敵を探す					 */
/*			戻り値：キャラ番号（いなければ -1）								 */
/* ========================================================================= */

int FindFrontAttackTarget(PCHARINFO pChar)
{
	int i, nCount;
	PCHARINFO pCharTmp;

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[i];
		if (pCharTmp == pChar) {
			continue;
		}
		if (!((pCharTmp->byNpcType == NPCTYPE_BATTLE1) || (pCharTmp->byNpcType == NPCTYPE_BATTLE2))) {
			continue;
		}
		if (pCharTmp->byCharState == CHARSTATE_DEAD) {
			continue;
		}
		if (GetCharDistance (pCharTmp, pChar->dwMapID, pChar->dwX, pChar->dwY, pChar->byDirection) == 1) {
			return i;
		}
	}
	return -1;
}


BOOL	g_bGuard			= FALSE;		/* Z を押し続けて構えている */
DWORD	g_dwGuardGageElapse	= 0;			/* 構えた時に溜まっていたゲージの時間(構えている間は止める) */
BYTE	g_byHasteRate		= 0;			/* 加速の率(%)。0なら加速していない */
DWORD	g_dwHasteEnd		= 0;			/* 加速が切れる時間(サーバーからの通知が届かなくても止まるように) */

/* ========================================================================= */
/* 関数名：	GetTurnGageTime													 */
/* 内容：	ターンゲージが満タンになるまでの時間(ms)						 */
/*			素早さ 0 で 3 秒～255 で 1 秒（サーバーも同じ計算）				 */
/* ========================================================================= */

DWORD GetTurnGageTime(PCHARINFO pChar)
{
	DWORD dwTmp;

	dwTmp = pChar->byDEX;
	dwTmp = dwTmp * 100 / 255;
	dwTmp = max (dwTmp, 1);
	dwTmp = 1000 + (2000 - ((2000 * dwTmp) / 100));
	/* 自分が加速していれば速く溜まる(サーバーも同じ計算) */
	if (g_byHasteRate && (g_nMyCharID >= 0) && (pChar == g_aCharInfo[g_nMyCharID])) {
		if ((int)(timeGetTime () - g_dwHasteEnd) < 0) {
			dwTmp = dwTmp * 100 / (100 + g_byHasteRate);
		} else {
			g_byHasteRate = 0;
		}
	}
	return dwTmp;
}


/* ========================================================================= */
/* 関数名：	IsTurnGageJust													 */
/* 内容：	ゲージが満タンになってすぐ（ジャストで押せる時間）か			 */
/* ========================================================================= */

BOOL IsTurnGageJust(PCHARINFO pChar)
{
	DWORD dwTime, dwFull;

	if ((pChar->dwTimeTurnLV == 0) || (pChar->byTurnLV < 100)) {
		return FALSE;
	}
	dwTime = timeGetTime ();
	dwFull = pChar->dwTimeTurnLV + GetTurnGageTime (pChar);
	return (dwTime < dwFull + TURNGAGE_JUSTTIME) ? TRUE : FALSE;
}


/* ========================================================================= */
/* 関数名：	ContinueTurnGage												 */
/* 内容：	連続攻撃：殴った後、同じ相手に向けて次のゲージを溜め始める		 */
/*			（毎回コマンド→攻撃→相手選択をしなくて済むように）			 */
/*			止める時は B キー（今までどおりゲージのキャンセル）				 */
/* ========================================================================= */

DWORD	g_dwTurnGageContinue = 0;		/* 殴って次のゲージを溜め始めた時間(押すのが遅れた X でコマンドを開かないように) */

void ContinueTurnGage(PCHARINFO pChar)
{
	DWORD dwAtackTblID;
	BOOL bAutoGage;

	dwAtackTblID = pChar->dwAtackTblID;
	bAutoGage = g_bAutoGage;
	CancelTurnGage (pChar);
	g_dwTurnGageContinue = timeGetTime ();

	/* 弓（相手を決めずに撃つ）は続けない */
	if (dwAtackTblID == (DWORD)-1) {
		return;
	}
	/* 戦闘モードではない・戦闘マップではない？ */
	if ((g_bBattleMode == FALSE) || (g_pMapInfo == NULL) || ((g_pMapInfo->byFlg & MAPFLG_BATTLE) == 0)) {
		return;
	}
	/* 倒れている・特技の詠唱中？ */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || pChar->dwTimeTurnMagicLV) {
		return;
	}
	StartTurnGage (pChar, dwAtackTblID);
	g_bAutoGage = bAutoGage;
}


/* ========================================================================= */
/* 受信データの保留															 */
/* マップ切り替えなどでフェードアウトの指示が来たら、画面が暗くなりきる	 */
/* までそれ以降の受信データを処理せずに取っておく。						 */
/* 先に次のマップを読み込んでしまうと、読み込み中は移動前の画面で止まり、*/
/* 読み込みが終わってからフェードアウトが始まってしまうため。			 */
/* ========================================================================= */
#define RECVHOLD_TIMEOUT	3000		/* 念のため、これ以上は待たない(ms) */

static BOOL s_bRecvHold = FALSE;						/* 受信データを保留中 */
static DWORD s_dwTimeRecvHold = 0;						/* 保留を始めた時間 */
static CmyArray<PBYTE, PBYTE> s_aRecvHoldData;			/* 保留中の受信データ */
static CmyArray<DWORD, DWORD> s_aRecvHoldSize;			/* 保留中の受信データのサイズ */

/* ========================================================================= */
/* 関数名：RecvDataHoldStart												 */
/* 内容：受信データの保留を始める											 */
/* ========================================================================= */
static void RecvDataHoldStart(void)
{
	s_bRecvHold = TRUE;
	s_dwTimeRecvHold = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：RecvDataHoldProc													 */
/* 内容：フェードアウトが終わっていたら保留中の受信データを処理する			 */
/*		 TimerProc から毎回呼ばれる											 */
/* ========================================================================= */
void RecvDataHoldProc(void)
{
	int i, nCount;

	if (!s_bRecvHold) {
		return;
	}

	if (timeGetTime () - s_dwTimeRecvHold < RECVHOLD_TIMEOUT) {
		/* まだ始まっていないフェードアウトが予約されている？ */
		nCount = g_aFadeInOutTbl.GetSize ();
		for (i = 0; i < nCount; i ++) {
			if (g_aFadeInOutTbl[i] == 0) {
				return;
			}
		}
		/* フェードイン中か、フェードアウトの途中？ */
		if (g_FadeInOutInfo.byFadeIn || (g_FadeInOutInfo.byFadeOut > 1)) {
			return;
		}
	}

	/* 画面が暗くなりきったので、取っておいた分を届いた順に処理する。
	   途中でまたフェードアウトの指示が来たら、残りは次のフェードアウト後に回す */
	s_bRecvHold = FALSE;
	while (!s_bRecvHold && s_aRecvHoldData.GetSize ()) {
		PBYTE pData = s_aRecvHoldData[0];
		DWORD dwDataSize = s_aRecvHoldSize[0];
		s_aRecvHoldData.RemoveAt (0);
		s_aRecvHoldSize.RemoveAt (0);
		RecvDataProc (pData, dwDataSize);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataHoldClear												 */
/* 内容：保留中の受信データを捨てる(切断時など)								 */
/* ========================================================================= */
void RecvDataHoldClear(void)
{
	int i, nCount;

	nCount = s_aRecvHoldData.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PBYTE pData = s_aRecvHoldData[i];
		SAFE_DELETE_ARRAY (pData);
	}
	s_aRecvHoldData.RemoveAll ();
	s_aRecvHoldSize.RemoveAll ();
	s_bRecvHold = FALSE;
}


/* ========================================================================= */
/* 関数名：RecvDataProc														 */
/* 内容：データ受信処理														 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void RecvDataProc(PBYTE pData, DWORD dwDataSize)
{
	/* フェードアウトが終わるまでは処理せずに取っておく */
	if (s_bRecvHold) {
		s_aRecvHoldData.Add (pData);
		s_aRecvHoldSize.Add (dwDataSize);
		return;
	}

	if ((*pData >= CMD_MAX) || (RecvDataProcTbl[*pData] == NULL)) {
		return;
	}

	RecvDataProcTbl[*pData](pData, dwDataSize);
	SAFE_DELETE_ARRAY (pData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCONNECT												 */
/* 内容：受信データ処理(接続応答)											 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void RecvDataProcCONNECT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	KillTimer (g_hWndMain, 0);

	int nNo = GetWindowID (g_dwMsgIDConnect);
	if (nNo < 0) {
		return;
	}
	PWNDINFO pWnd = (PWNDINFO)g_WndInfo[nNo];
	EraseWindow (pWnd);
	g_dwMsgIDConnect = 0;

	/* バージョンが一致しない？ */
	if (pPara->byData != VERSIONVAL) {
		pWnd = MakeWindowMSG3SEC ("バージョンが違います。再読み込みしてください");
		pWnd->nData1 = MSG3SEC_ACC;
		g_WndInfo.Add (pWnd);
		g_bDraw = TRUE;
		InvalidateRect (g_hWndMain, NULL, FALSE);
		UpdateWindow (g_hWndMain);
		return;
	}
	g_pMyID = pPara->pID;
	/* ログイン要求（端末トークンがあればそれで、無ければ ID・パスワードで） */
	if (g_szWebToken[0]) {
		SendCmdWEBACCOUNT (WEBACC_TOKENLOGIN);
	} else {
		SendCmdLOGINREQ ();
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLOGINRES												 */
/* 内容：受信データ処理(ログイン応答)										 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void RecvDataProcLOGINRES(PBYTE pData, DWORD dwDataSize)
{
	PWNDINFO pWnd;
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	switch (pPara->byData) {
	case 0:
		pWnd = MakeWindowMSG3SEC ("一致するアカウントが見つかりません");
		break;

	case 1:
		pWnd = MakeWindowMSG3SEC ("パスワードが間違っています");
		break;

	/* ログイン成功 */
	case 2:
		{
			int nNo = GetWindowID (g_dwMsgIDLogin);
			PWNDINFO pWnd = (PWNDINFO)g_WndInfo[nNo];
			EraseWindow (pWnd);
			g_dwMsgIDLogin = 0;

			pWnd = MakeWindowLOGINMENU ();
			g_WndInfo.Add (pWnd);
			g_dwActiveMsgID = pWnd->dwID;
			pWnd = MakeWindowLOGINMENUCHARINFO ();
			g_WndInfo.Add (pWnd);
			/* ID・パスワードで入れたら、次からボタンで入れるよう端末トークンをもらう */
			if (g_szWebToken[0] == 0) {
				SendCmdWEBACCOUNT (WEBACC_TOKENREQ);
			}
			return;
		}
		break;

	case 3:
		pWnd = MakeWindowMSG3SEC ("すでにログインしています");
		break;

	case 4:
		pWnd = MakeWindowMSG3SEC ("サーバーの定員オーバーです");
		break;

	case 5:
		pWnd = MakeWindowMSG3SEC ("アカウント作成が拒否されました");
		break;

	case 6:
		pWnd = MakeWindowMSG3SEC ("ログイン要求が拒否されました");
		break;

	case 7:
		pWnd = MakeWindowMSG3SEC ("この端末の記憶が無効です");
		WebAccOnTokenNG ();
		break;

	default:
		pWnd = MakeWindowMSG3SEC ("ログインできませんでした");
		break;
	}
	ZeroMemory (g_szWebToken, sizeof (g_szWebToken));
	g_bDraw = TRUE;
	pWnd->nData1 = MSG3SEC_ACC;
	g_WndInfo.Add (pWnd);
	InvalidateRect (g_hWndMain, NULL, FALSE);
	UpdateWindow (g_hWndMain);
	cliSock.Destroy ();
}

/* ========================================================================= */
/* 関数名：RecvDataProcHASTE												 */
/* 内容：受信データ処理(加速の状態。byData=率%、dwData=続く時間ms)			 */
/* ========================================================================= */
void RecvDataProcHASTE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;

	if (dwDataSize < sizeof (CMDPARA_DATA4)) {
		return;
	}
	g_byHasteRate	= pPara->byData;
	g_dwHasteEnd	= timeGetTime () + pPara->dwData;
	if (pPara->dwData == 0) {
		g_byHasteRate = 0;
	}
}


/* ========================================================================= */
/* 関数名：RecvDataProcMAKERINFO											 */
/* 内容：受信データ処理(作り手情報。ログイン時と管理画面での変更時)			 */
/* ========================================================================= */
void RecvDataProcMAKERINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MAKERINFO pPara = (PCMDPARA_MAKERINFO)pData;

	if (dwDataSize < sizeof (CMDPARA_MAKERINFO)) {
		return;
	}
	if ((pPara->wID >= MAX_MAKER) || (pPara->wCount > MAX_MAKER)) {
		return;
	}
	g_aMakerInfo[pPara->wID] = pPara->Info;
	/* 文字列は必ず終端させる */
	g_aMakerInfo[pPara->wID].szName[MAX_MAKERNAME - 1] = 0;
	g_aMakerInfo[pPara->wID].szMsgTalk[MAX_MAKERMSG - 1] = 0;
	g_aMakerInfo[pPara->wID].szMsgDone[MAX_MAKERMSG - 1] = 0;
	g_aMakerInfo[pPara->wID].szMsgLack[MAX_MAKERMSG - 1] = 0;
	g_nMakerCount = pPara->wCount;
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：RecvDataProcRECIPEINFO											 */
/* 内容：受信データ処理(レシピ情報。ログイン時と管理画面での変更時)			 */
/* ========================================================================= */
void RecvDataProcRECIPEINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_RECIPEINFO pPara = (PCMDPARA_RECIPEINFO)pData;

	if (dwDataSize < sizeof (CMDPARA_RECIPEINFO)) {
		return;
	}
	if ((pPara->wID >= MAX_RECIPE) || (pPara->wCount > MAX_RECIPE)) {
		return;
	}
	g_aRecipeInfo[pPara->wID] = pPara->Info;
	g_nRecipeCount = pPara->wCount;
	/* 開いている画面は描画の時に内容の変化を見つけて作り直す */
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：RecvDataProcRECIPEMAKE											 */
/* 内容：受信データ処理(作ってもらった結果)									 */
/*		 作り手の画面を開いていれば、結果に合わせた台詞を出す				 */
/* ========================================================================= */
void RecvDataProcRECIPEMAKE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_RECIPEMAKE pPara = (PCMDPARA_RECIPEMAKE)pData;
	PWNDINFO pWndList = NULL;
	LPCSTR pszMsg;
	int i;

	if (dwDataSize < sizeof (CMDPARA_RECIPEMAKE)) {
		return;
	}
	g_bProcMakerWait = FALSE;
	if ((g_nProcMakerID < 0) || (g_nProcMakerID >= MAX_MAKER)) {
		return;
	}
	for (i = 0; i < g_WndInfo.GetSize (); i ++) {
		if ((g_WndInfo[i]->nType == WNDTYPE_MAKER) && (g_WndInfo[i]->nType2 == MAKERWND_LIST)) {
			pWndList = g_WndInfo[i];
			break;
		}
	}
	if (pWndList == NULL) {
		return;
	}

	switch (pPara->byResult) {
	case RECIPERESULT_OK:
		pszMsg = g_aMakerInfo[g_nProcMakerID].szMsgDone;
		if (pszMsg[0] == 0) {
			pszMsg = "できました。@2";
		}
		break;
	case RECIPERESULT_LACKMAT:
		pszMsg = g_aMakerInfo[g_nProcMakerID].szMsgLack;
		if (pszMsg[0] == 0) {
			pszMsg = "材料が足りないようです。@2";
		}
		break;
	case RECIPERESULT_LACKGOLD:
		pszMsg = "手数料が足りないようです。@2";
		break;
	case RECIPERESULT_NOROOM:
		pszMsg = "持ち物も預かり所もいっぱいで、お渡しできません。@2";
		break;
	default:
		pszMsg = "それは作れないようです。@2";
		break;
	}
	MakerSetMsg (pWndList, pszMsg);
	g_dwActiveMsgID = pWndList->dwID;
	pWndList->dwTimeDrawStart = 0;
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：RecvDataProcNPCROUTE												 */
/* 内容：受信データ処理(NPCの巡回ルート)									 */
/* ========================================================================= */
void RecvDataProcNPCROUTE(PBYTE pData, DWORD dwDataSize)
{
	if (dwDataSize < sizeof (CMDPARA_NPCROUTE)) {
		return;
	}
	NpcRouteOnRecv (pData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTOWNWARP												 */
/* 内容：受信データ処理(町転送の行き先一覧)									 */
/* ========================================================================= */
void RecvDataProcTOWNWARP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_TOWNWARP pPara = (PCMDPARA_TOWNWARP)pData;
	int i;

	if (dwDataSize < sizeof (CMDPARA_TOWNWARP)) {
		return;
	}
	CopyMemory (&g_TownWarpInfo, pPara, sizeof (CMDPARA_TOWNWARP));
	if (g_TownWarpInfo.byCount > MAX_TOWNWARP + 1) {
		g_TownWarpInfo.byCount = MAX_TOWNWARP + 1;
	}
	/* 名前は必ず終端させる */
	for (i = 0; i <= MAX_TOWNWARP; i ++) {
		g_TownWarpInfo.aszName[i][MAX_TOWNWARPNAME - 1] = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcQUEST												 */
/* 内容：受信データ処理(ギルドの仕事依頼：一覧・結果)						 */
/* ========================================================================= */
void RecvDataProcQUEST(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_QUEST pPara = (PCMDPARA_QUEST)pData;
	PWNDINFO pWndList;

	if (dwDataSize < sizeof (CMDPARA_QUEST)) {
		return;
	}
	/* 文字列は必ず終端させる */
	pPara->szName[MAX_QUESTNAME - 1] = 0;
	pPara->szExplain[MAX_QUESTMSG - 1] = 0;
	pPara->szGoal[MAX_QUESTGOAL - 1] = 0;
	pPara->szReward[MAX_QUESTGOAL - 1] = 0;
	pPara->szMsg[MAX_QUESTMSG - 1] = 0;

	pWndList = QuestFindWindow (QUESTWND_LIST);

	switch (pPara->byType) {
	case QUESTCMD_LISTITEM:
		/* 頼んでいない（開いていない・返事待ちでもない）のに届いた一覧は捨てる */
		if ((pWndList == NULL) &&
			((g_dwQuestWaitTime == 0) || (timeGetTime () - g_dwQuestWaitTime > QUEST_WAITMAX))) {
			g_dwQuestWaitTime = 0;
			return;
		}
		g_dwQuestGuildTblID = pPara->dwNpcTblID;
		if (pPara->byCount == 0) {
			g_nQuestListCount = 0;
		} else {
			if ((pPara->byIndex >= MAX_QUESTLIST) || (pPara->byIndex >= pPara->byCount)) {
				return;
			}
			/* 一覧の先頭が来たら作り直し */
			if (pPara->byIndex == 0) {
				g_nQuestListCount = 0;
			}
			PQUESTLISTITEM pItem = &g_aQuestList[pPara->byIndex];
			pItem->byMark = pPara->byMark;
			pItem->wQuestID = pPara->wQuestID;
			strcpy (pItem->szName, pPara->szName);
			strcpy (pItem->szExplain, pPara->szExplain);
			strcpy (pItem->szGoal, pPara->szGoal);
			strcpy (pItem->szReward, pPara->szReward);
			if ((int)pPara->byIndex + 1 > g_nQuestListCount) {
				g_nQuestListCount = (int)pPara->byIndex + 1;
			}
		}
		if (pWndList == NULL) {
			/* ギルドの人に話した返事：一覧の窓を開く */
			g_dwQuestWaitTime = 0;
			g_szQuestMsg[0] = 0;
			pWndList = MakeWindowQUEST (QUESTWND_LIST);
			g_WndInfo.Add (pWndList);
			g_dwActiveMsgID = pWndList->dwID;
		}
		/* カーソルと表示位置を一覧の中に収める */
		if (pWndList->nData1 >= g_nQuestListCount) {
			pWndList->nData1 = max (g_nQuestListCount - 1, 0);
		}
		if (pWndList->nData2 > pWndList->nData1) {
			pWndList->nData2 = pWndList->nData1;
		}
		if (pWndList->nData1 >= pWndList->nData2 + QUEST_LISTROWS) {
			pWndList->nData2 = pWndList->nData1 - QUEST_LISTROWS + 1;
		}
		pWndList->dwTimeDrawStart = 0;
		g_bDraw = TRUE;
		break;

	case QUESTCMD_RESULT:
		/* 結果はシステムメッセージにも出す */
		if (pPara->szMsg[0]) {
			AddSysMsg (pPara->szMsg, (pPara->byResult == QUESTRESULT_NG) ? RGB (255, 200, 200) : RGB (255, 255, 255));
		}
		if (pWndList) {
			/* 窓の詳細欄にも出して、一覧を取り直す（印が変わるため） */
			_snprintf (g_szQuestMsg, sizeof (g_szQuestMsg) - 1, "%s", pPara->szMsg);
			g_szQuestMsg[sizeof (g_szQuestMsg) - 1] = 0;
			pWndList->dwTimeDrawStart = 0;
			SendCmdQUEST (QUESTCMD_LISTREQ, 0, g_dwQuestGuildTblID);
		}
		g_bDraw = TRUE;
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcQUESTSTATE											 */
/* 内容：受信データ処理(依頼の進み具合。自分と仲間の分)						 */
/* ========================================================================= */
void RecvDataProcQUESTSTATE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_QUESTSTATE pPara = (PCMDPARA_QUESTSTATE)pData;
	PWNDINFO pWnd;
	int i;

	if (dwDataSize < sizeof (CMDPARA_QUESTSTATE)) {
		return;
	}
	CopyMemory (&g_QuestState, pPara, sizeof (CMDPARA_QUESTSTATE));
	if (g_QuestState.byCount > MAX_PARTYMEMBER) {
		g_QuestState.byCount = MAX_PARTYMEMBER;
	}
	/* 文字列は必ず終端させる */
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		g_QuestState.aEntry[i].szCharName[MAX_CHARNAMEBUF - 1] = 0;
		g_QuestState.aEntry[i].szQuestName[MAX_QUESTNAME - 1] = 0;
		g_QuestState.aEntry[i].szGoal[MAX_QUESTGOAL - 1] = 0;
	}
	pWnd = QuestFindWindow (QUESTWND_STATE);
	if (pWnd) {
		pWnd->dwTimeDrawStart = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINUNDO											 */

/* 内容：受信データ処理(管理者画面の「元に戻す」)							 */
/* ========================================================================= */
void RecvDataProcADMINUNDO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINUNDO pPara = (PCMDPARA_ADMINUNDO)pData;

	/* 案内文を必ず終端させる */
	pPara->szMsg[sizeof (pPara->szMsg) - 1] = 0;
	AdminUndoOnRecv (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcWEBACCOUNT											 */
/* 内容：受信データ処理(ブラウザ版のアカウント関係)							 */
/* ========================================================================= */
void RecvDataProcWEBACCOUNT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_WEBACCOUNT pPara = (PCMDPARA_WEBACCOUNT)pData;

	switch (pPara->byCmdSub) {
	case WEBACC_TOKEN:			/* 端末トークン通知 */
		pPara->szToken[MAX_WEBTOKEN] = 0;
		if (strlen (pPara->szToken) == MAX_WEBTOKEN) {
			WebAccSaveToken (pPara->szToken);
		}
		break;

	case WEBACC_OPTBGM:			/* 音楽ON/OFFのアカウント設定 */
		/*
		 * ログインした直後に届く。ログイン画面の曲は設定に関係なく鳴らしているので、
		 * ここでは覚えるだけで、マップに入った時の PlayBGM (0) から効く。
		 * 端末の ini にも書いておき、次のログイン画面までの既定にする。
		 */
		if ((pPara->byData == OPTBGM_ON) || (pPara->byData == OPTBGM_OFF)) {
			g_bBGMONOFF = (pPara->byData == OPTBGM_ON) ? TRUE : FALSE;
			if (!g_bBGMONOFF && (g_byGameState != GAMESTATE_LOGIN)) {
				StopBGM ();
			}
			SaveIniOption ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGSTATE												 */
/* 内容：受信データ処理(状態遷移)											 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void RecvDataProcCHGSTATE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	g_byGameState = pPara->byData;
	if (g_byGameState == GAMESTATE_MOVE) {
		MakeMsgLogWindow ();
		MakeDebugWindow ();
		MakeBalloonWindow ();
	}
	int i;
	/* マップパーツのアニメーションカウンタを初期化 */
	DWORD dwTmp = timeGetTime ();
	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i];
		g_dwTimeLastMapParts = timeGetTime ();
		pParts->dwLastAnime = g_dwTimeLastMapParts;
		pParts->byAnimeNo = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPINFO												 */
/* 内容：受信データ処理(マップ情報通知)										 */
/* 日付：2003/05/04															 */
/* ========================================================================= */
void RecvDataProcMAPINFO(PBYTE pData, DWORD dwDataSize)
{
	/* 取り引き中？ */
	if (g_pTradeInfo) {
		SendCmdTRADE (g_pTradeInfo->dwTblID, CMDTRADE_OUT);
		if (g_nMyCharID >= 0) {
			SendCmdTRADE (g_aCharInfo[g_nMyCharID]->dwTblID, CMDTRADE_OUT);
		}
	}

	BOOL bClChg = FALSE;
	if (g_pMapInfo->clChg) {
		bClChg = TRUE;
	}

	int i, j;
	/* キャラ情報を全て消去 */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		DeleteChar (i);
	}
	g_aCharInfo.RemoveAll ();
	g_nMyCharID = -1;
	g_wLastProcSecond = 0xFFFF;
	/* 参加中のサブゲーム情報をクリア */
	if (g_pSubGameInfoPlay) {
		SAFE_DELETE_ARRAY (g_pSubGameInfoPlay->pInfo);
	}
	SAFE_DELETE (g_pSubGameInfoPlay);

	i = GetWindowID (g_dwMsgIDSubGame);
	if (i >= 0) {
		EraseWindow ((PWNDINFO)g_WndInfo[i]);
	}
	if (g_dwMsgIDCommand > 0) {
		i = GetWindowID (g_dwMsgIDCommand);
		if (i >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[i]);
		}
	}
	g_nProcMsg = 0;
	SAFE_DELETE (g_pTargetSelectInfo);

	/* オブジェクト情報を開放 */
	for (i = 0; i < g_aObjInfo.GetSize (); i ++) {
		POBJINFO pObj = g_aObjInfo[i];
		if (pObj->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pObj->pAnimeInfo);
		}
	}
	g_aObjInfo.RemoveAll ();
	g_nProcObj = 0;
	/* 放送はマップごとに許可が違うので普通に戻す。パーティ発言はマップを移っても続ける */
	if (g_bySpeechMode != 3) {
		g_bySpeechMode = 0;
	}
	g_bMoving = FALSE;

	StopBGM ();

	/* 現在のマップ情報を開放 */
	if (g_pMapInfo) {
		if (g_pMapInfo->pShopInfo) {
			for (int j = 0; j < g_pMapInfo->byCountShop; j ++) {
				PSHOPINFO pShop = &g_pMapInfo->pShopInfo[j];
				for (int k = 0; k < MAX_SHOPMSG; k ++) {
					SAFE_DELETE_ARRAY (pShop->apMsg[k]);
				}
			}
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pszName);
		SAFE_DELETE_ARRAY (g_pMapInfo->pShopInfo);
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
	}

	/* 一度見た所の記録を消す(迷路の作り直しなどで形が変わるので) */
	ResetMapViewSeen ();

	PMAPINFO pMap = &((PCMDPARA_MAPINFO)pData)->MapInfo;
	CopyMemory (g_pMapInfo, pMap, sizeof (MAPINFO));
	g_pMapInfo->pszName = NULL;
	g_pMapInfo->pbyRoofLV = NULL;

	g_pMapInfo->pwMap = new WORD[pMap->nCX * pMap->nCY];
	CopyMemory ((PBYTE)g_pMapInfo->pwMap, (PBYTE)pData + sizeof (CMDPARA_MAPINFO),
			pMap->nCX * pMap->nCY * sizeof (WORD));

	g_pMapInfo->pdwMapChar = new DWORD[pMap->nCX * pMap->nCY];
	ZeroMemory ((PBYTE)g_pMapInfo->pdwMapChar, pMap->nCX * pMap->nCY * sizeof (DWORD));

	g_pMapInfo->pbyFlg = new BYTE[pMap->nCX * pMap->nCY];
	ZeroMemory ((PBYTE)g_pMapInfo->pbyFlg, pMap->nCX * pMap->nCY);

	PBYTE pDataTmp = pData + sizeof (CMDPARA_MAPINFO) + (pMap->nCX * pMap->nCY * sizeof (WORD));

	/* 店がある？ */
	if (g_pMapInfo->byCountShop) {
		g_pMapInfo->pShopInfo = new SHOPINFO[g_pMapInfo->byCountShop];
		ZeroMemory (g_pMapInfo->pShopInfo, sizeof (SHOPINFO) * g_pMapInfo->byCountShop);

		for (i = 0; i < g_pMapInfo->byCountShop; i ++) {
			PSHOPINFO pShop = &g_pMapInfo->pShopInfo[i];
			CopyMemory (pShop, pDataTmp, sizeof (SHOPINFO));
			pShop->dwItemCRC = g_crc.GetCRC ((PBYTE)pShop->awItem, sizeof (pShop->awItem));
			pDataTmp += sizeof (SHOPINFO);
			for (j = 0; j < MAX_SHOPMSG; j ++) {
				int nLen = strlen ((LPSTR)pDataTmp);
				pShop->apMsg[j] = new char[nLen + 1];
				ZeroMemory (pShop->apMsg[j], nLen + 1);
				strcat (pShop->apMsg[j], (LPSTR)pDataTmp);
				pDataTmp += (nLen + 1);
				pShop->dwMsgCRC[j] = g_crc.GetCRC ((PBYTE)pShop->apMsg[j], nLen + 1);
			}
		}
	}

	/* マップ名あり？ */
	if (pDataTmp) {
		g_pMapInfo->pszName = new char[strlen ((LPSTR)pDataTmp) + 1];
		ZeroMemory (g_pMapInfo->pszName, strlen ((LPSTR)pDataTmp) + 1);
		strcat ((LPSTR)g_pMapInfo->pszName, (LPSTR)pDataTmp);
	}

	/* マップパーツのアニメーションカウンタを初期化 */
	DWORD dwTmp = timeGetTime ();
	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i];
		g_dwTimeLastMapParts = timeGetTime ();
		pParts->dwLastAnime = g_dwTimeLastMapParts;
		pParts->byAnimeNo = 0;
	}

	delete g_pDibMapParts;
	g_pDibMapParts = new CImg32;
	g_pDibMapParts->CreateWithoutGdi (16 * 32, (g_wCountMapParts / 16 + 1) * 32);
	g_pDibMapParts->Clear ();

	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = &g_pMapPartsInfo[i];
		int no, xx, yy;
		DWORD dwID;
		if (pParts->wBackID) {
			no = pParts->wBackID / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = pParts->wBackID % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy);
		}
		no = pParts->wGrpID / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = pParts->wGrpID % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = (dwID % g_byMapGrpCountX) * 32;
		yy = dwID / g_byMapGrpCountX * 32;
		if (pParts->byLevel) {
			g_pDibMapParts->BltAlpha ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, pParts->byLevel, TRUE);
		} else {
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, TRUE);
		}
	}
	if (g_pMapInfo->clChg) {
		g_pDibMapParts->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
	}

	for (i = 0; i < g_wCountMapPartsGrp; i ++) {
		g_apDibMap[i]->Blt (0, 0, 0, 0, g_apDibMapBack[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibMap[i]->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
		}
	}
	for (i = 0; i < g_wCountCharGrp; i ++) {
		g_apDibChar[i]->Blt (0, 0, 0, 0, g_apDibCharBack[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar[i]->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
		}
	}
	for (i = 0; i < g_wCountCharGrp1632; i ++) {
		g_apDibChar1632[i]->Blt (0, 0, 0, 0, g_apDibChar1632Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar1632[i]->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
		}
	}
	for (i = 0; i < g_wCountCharGrp3232; i ++) {
		g_apDibChar3232[i]->Blt (0, 0, 0, 0, g_apDibChar3232Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar3232[i]->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
		}
	}
	for (i = 0; i < g_wCountCharGrp4848; i ++) {
		g_apDibChar4848[i]->Blt (0, 0, 0, 0, g_apDibChar4848Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar4848[i]->ChgColor (0, 0, 0, 0, g_pMapInfo->clChg);
		}
	}

	if (g_pMapInfo->byBGM && g_pMapInfo->byBGM < SOUND_MAX) {
		PlayBGM (0);
	}

	char szTmp[256] = {0};
	if (g_pMapInfo->pszName && strlen ((LPSTR)g_pMapInfo->pszName)) {
		wsprintf (szTmp, "%s - %s%s", g_pMapInfo->pszName, WNDTITLE, VERTEXT);
	} else {
		wsprintf (szTmp, "%s%s", WNDTITLE, VERTEXT);
	}
	SetWindowText (g_hWndMain, szTmp);

	ZeroMemory (g_pMapDarkTmp, SCRMARTSSIZE);

	g_aFadeInOutTbl.Add (1);
	g_byTextFadeInOut = 1;
	g_dwTimeTextFadeInOut = 0;
	g_FadeInOutInfo.dwLastTime = timeGetTime ();
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPPARTSINFO											 */
/* 内容：受信データ処理(マップパーツ情報通知)								 */
/* 日付：2003/05/11															 */
/* ========================================================================= */
void RecvDataProcMAPPARTSINFO(PBYTE pData, DWORD dwDataSize)
{
	PMAPPARTSINFO pParts;
	PMAPPARTSINFOSAVE pRecv;
	PBYTE pRead;
	DWORD i, no, xx, yy, dwID;
	PANIMEINFO pAnime;

	/* 現在のマップ情報を開放 */
	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_pMapPartsInfo[i];
		if (pParts->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pParts->pAnimeInfo);
		}
	}
	SAFE_DELETE_ARRAY (g_pMapPartsInfo);

	g_wCountMapParts = ((PCMDPARA_MAPPARTSINFO)pData)->wCount;
	g_pMapPartsInfo = new MAPPARTSINFO [g_wCountMapParts];
	ZeroMemory (g_pMapPartsInfo, g_wCountMapParts * sizeof (MAPPARTSINFO));
	pRead = pData + sizeof (CMDPARA_MAPPARTSINFO);

	for (i = 0; i < g_wCountMapParts; i ++) {
		pRecv = (PMAPPARTSINFOSAVE)pRead;
		pParts = &g_pMapPartsInfo[i];
		pParts->byViewType = pRecv->byViewType;
		pParts->byHitType = pRecv->byHitType;
		pParts->byAnimeType = pRecv->byAnimeType;
		pParts->byAnimeCount = pRecv->byAnimeCount;
		pParts->wGrpID = pRecv->wGrpID;
		pParts->wBackID = pRecv->wBackID;
		pParts->pAnimeInfo = pRecv->pAnimeInfo;
		pParts->byLevel = pRecv->byLevel;
		pParts->byLight = pRecv->byLight;
		pRead += sizeof (MAPPARTSINFOSAVE);

		if (pRecv->pAnimeInfo) {
			pAnime = new ANIMEINFO[pRecv->byAnimeCount];
			CopyMemory (pAnime, pRead, pRecv->byAnimeCount * sizeof (ANIMEINFO));
			pParts->pAnimeInfo = pAnime;
			pRead += (pRecv->byAnimeCount * sizeof (ANIMEINFO));
		}
	}
	delete g_pDibMapParts;
	g_pDibMapParts = new CImg32;
	g_pDibMapParts->CreateWithoutGdi (16 * 32, (g_wCountMapParts / 16 + 1) * 32);
	g_pDibMapParts->Clear ();

	for (i = 0; i < g_wCountMapParts; i ++) {
		pParts = &g_pMapPartsInfo[i];
		if (pParts->wBackID) {
			no = pParts->wBackID / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = pParts->wBackID % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy);
		}
		no = pParts->wGrpID / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = pParts->wGrpID % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = (dwID % g_byMapGrpCountX) * 32;
		yy = dwID / g_byMapGrpCountX * 32;
		if (pParts->byLevel) {
			g_pDibMapParts->BltAlpha ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, pParts->byLevel, TRUE);
		} else {
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, TRUE);
		}
	}

	/* マップパーツのアニメーションカウンタを初期化 */
	DWORD dwTmp = timeGetTime ();
	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i];
		g_dwTimeLastMapParts = timeGetTime ();
		pParts->dwLastAnime = g_dwTimeLastMapParts;
		pParts->byAnimeNo = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARINFO												 */
/* 内容：受信データ処理(キャラ情報通知)										 */
/* 日付：2003/05/12															 */
/* ========================================================================= */
void RecvDataProcCHARINFO(PBYTE pData, DWORD dwDataSize)
{
	int i, x, y, nNo, nCount = g_aCharInfo.GetSize ();
	CMDPARA_CHARINFO Para;
	PCMDPARA_CHARINFO pChar = &Para;
	PCHARINFO pCharDst;
	/* 名前は可変長で届くので構造体に戻す（後ろに NPC 発言が続く） */
	PBYTE pDataNext = SboPackGetCHARINFO (&Para, pData, NULL);

	for (i = 0; i < g_aRecTbl.GetSize (); i ++) {
		DWORD dwTmp = g_aRecTbl[i];
		if (dwTmp == pChar->dwTblID) {
			g_aRecTbl.RemoveAt (i);
			break;
		}
	}

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		return;
	}

	nNo = GetCharNo (pChar->pID);

	/* 登録済み？ */
	if (nNo >= 0) {
		pCharDst = g_aCharInfo[nNo];
		if (g_pMapInfo->pdwMapChar) {
			if ((pCharDst->dwX <= (DWORD)g_pMapInfo->nCX) && (pCharDst->dwY <= (DWORD)g_pMapInfo->nCY)) {
				g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] --;
			}
		}
		/* 削除中、または別のキャラに入れ替わった（倒れて消えかけの敵の番号を、
		   雇ったばかりの傭兵などがもらった）？ 前のキャラのダメージ表示などは引き継がない */
		if (pCharDst->dwTimeDelete || (strcmp (pCharDst->szName, pChar->szName) != 0)) {
			ZeroMemory (pCharDst->aCharEfcInfo, sizeof (pCharDst->aCharEfcInfo));
		}
		/* 削除中？ */
		if (pCharDst->dwTimeDelete) {
			pCharDst->dwTimeDelete = 0;
			pCharDst->dwTimeAdd = timeGetTime ();
		}

	} else {
		pCharDst = new CHARINFO;
		ZeroMemory ((PBYTE)pCharDst, sizeof (CHARINFO));
		pCharDst->dwTimeAdd = timeGetTime ();
		pCharDst->dwTargetTblID3	= -1;
		pCharDst->dwTailTblID		= -1;
		pCharDst->dwTailBackTblID	= -1;
	}

	pCharDst->pID					= pChar->pID;
	pCharDst->dwTblID				= pChar->dwTblID;
	pCharDst->byAdminLV				= pChar->byAdminLV;
	pCharDst->byCharState			= pChar->byCharState;
	pCharDst->byDirection			= pChar->byDirection;
	pCharDst->byRoofLV				= pChar->byRoofLV;
	pCharDst->byEfect				= pChar->byEfect;
	pCharDst->byEfectIdle			= pChar->byEfectIdle;
	pCharDst->byNpcType				= pChar->byNpcType;
	pCharDst->wGrpID				= pChar->wGrpID;
	pCharDst->wGrpMain				= pChar->wGrpMain;
	pCharDst->wGrpSub				= pChar->wGrpSub;
	pCharDst->wGrpTmpID				= pChar->wGrpTmpID;
	pCharDst->wGrpTmpMain			= pChar->wGrpTmpMain;
	pCharDst->wGrpTmpSub			= pChar->wGrpTmpSub;
	pCharDst->wMaxHP				= pChar->wMaxHP;
	pCharDst->wHP					= pChar->wHP;
	pCharDst->wMaxMana				= pChar->wMaxMana;
	pCharDst->wMana					= pChar->wMana;
	pCharDst->wMakeYear				= pChar->wMakeYear;
	pCharDst->wMakeMonth			= pChar->wMakeMonth;
	pCharDst->wMakeDay				= pChar->wMakeDay;
	pCharDst->dwMapID				= pChar->dwMapID;
	pCharDst->dwStartMapID			= pChar->dwStartMapID;
	pCharDst->dwStartX				= pChar->dwStartX;
	pCharDst->dwStartY				= pChar->dwStartY;
	pCharDst->dwX					= pChar->dwX;
	pCharDst->dwY					= pChar->dwY;
	pCharDst->dwXBack				= pChar->dwX;
	pCharDst->dwYBack				= pChar->dwY;
	pCharDst->dwLoginCount			= pChar->dwLoginCount;
	pCharDst->dwPlayTime			= pChar->dwPlayTime;
	pCharDst->dwStTmplateID			= pChar->dwStTmplateID;
	pCharDst->dwTailTblID			= pChar->dwTailTblID;
	pCharDst->dwTailBackTblID		= pChar->dwTailBackTblID;
	pCharDst->dwNpcID				= pChar->dwNpcID;
	pCharDst->byDotMove				= 0;
	pCharDst->byAreaX				= pChar->byAreaX;
	pCharDst->byAreaY				= pChar->byAreaY;
	pCharDst->byPutCount			= pChar->byPutCount;
	pCharDst->byDropProbability		= pChar->byDropProbability;
	pCharDst->byMoveProbabilityX	= pChar->byMoveProbabilityX;
	pCharDst->byMoveProbabilityY	= pChar->byMoveProbabilityY;
	pCharDst->byLightLevel			= pChar->byLightLevel;
	pCharDst->bySearchCx			= pChar->bySearchCx;
	pCharDst->bySearchCy			= pChar->bySearchCy;
	pCharDst->wTimePutInterval		= pChar->wTimePutInterval;
	pCharDst->clName				= pChar->clName;
	pCharDst->clSpeech				= pChar->clSpeech;
	pCharDst->byEfcAnime			= 0;
	pCharDst->dwTimeEfc				= 0;
	if (pCharDst->byEfectIdle) {
		pCharDst->dwTimeEfc = timeGetTime ();

	} else if (pCharDst->byEfect) {
		switch (pCharDst->byNpcType) {
		case NPCTYPE_USEMAGIC:
		case NPCTYPE_USEMAGICPARENT:
		case NPCTYPE_ATACK:
			pCharDst->dwTimeEfc = timeGetTime ();
			break;
		}
	} else if (pCharDst->byEfectIdle) {
		pCharDst->dwTimeEfc = timeGetTime ();
	}

	pCharDst->dwStTmplateID = pChar->dwStTmplateID;

	pCharDst->dwSpeechCRC = 0;
	SAFE_DELETE_ARRAY (pCharDst->pszNpcSpeech);
	pCharDst->pszNpcSpeech = pChar->pszNpcSpeech;
	CopyMemory (pCharDst->szName, pChar->szName, sizeof (pCharDst->szName));
	CopyMemory (pCharDst->szAcc, pChar->szAcc, MAX_ACCNAME);
	CopyMemory (pCharDst->szPas, pChar->szPas, MAX_ACCNAME);
	if (pChar->pszNpcSpeech) {
		LPSTR pszTmp = (LPSTR)pDataNext;
		int nSize = strlen (pszTmp) + 1;

		pCharDst->pszNpcSpeech = new char[nSize];
		CopyMemory (pCharDst->pszNpcSpeech, pszTmp, nSize);
		pCharDst->dwSpeechCRC = g_crc.GetCRC ((PBYTE)pCharDst->pszNpcSpeech, nSize);
	}

	if (nNo < 0) {
		g_aCharInfo.Add (pCharDst);
	}

	if (g_pMapInfo->pdwMapChar) {
		if ((pCharDst->dwX < (DWORD)g_pMapInfo->nCX) && (pCharDst->dwY < (DWORD)g_pMapInfo->nCY)) {
			g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] ++;
		}
	}

	g_nMyCharID = GetCharID (g_pMyID);

	if (g_nMyCharID >= 0) {
		/* 画面内にいるキャラ情報のチェック */
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			if (i == g_nMyCharID) {
				continue;
			}
			if (g_aCharInfo[i]->dwTimeDelete) {
				continue;
			}
			x = abs (g_aCharInfo[g_nMyCharID]->dwX - g_aCharInfo[i]->dwX);
			y = abs (g_aCharInfo[g_nMyCharID]->dwY - g_aCharInfo[i]->dwY);
			if (g_aCharInfo[i]->byDirection == 99) {
				g_aCharInfo[i]->dwTimeDelete = timeGetTime ();

			} else if ((g_aCharInfo[g_nMyCharID]->dwMapID != g_aCharInfo[i]->dwMapID) ||
				(x > DRAW_PARTS_X + 2) || (y > DRAW_PARTS_Y + 2)) {
				DeleteChar (i);
				i = -1;
			}
		}
	}

	/* キャラ座標情報更新 */
	RenewCharPosInfo ();
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARINFO2											 */
/* 内容：受信データ処理(キャラ情報通知2)									 */
/* 日付：2003/06/17															 */
/* ========================================================================= */
void RecvDataProcCHARINFO2(PBYTE pData, DWORD dwDataSize)
{
	int i, x, y, nNo, nCount = g_aCharInfo.GetSize ();
	CMDPARA_CHARINFO2 Para;
	PCMDPARA_CHARINFO2 pChar = &Para;
	PCHARINFO pCharDst;
	/* 名前は可変長で届くので構造体に戻す（後ろに NPC 発言が続く） */
	PBYTE pDataNext = SboPackGetCHARINFO2 (&Para, pData, NULL);

	for (i = 0; i < g_aRecTbl.GetSize (); i ++) {
		DWORD dwTmp = g_aRecTbl[i];
		if (dwTmp == pChar->dwTblID) {
			g_aRecTbl.RemoveAt (i);
			break;
		}
	}

	if (g_nMyCharID < 0) {
		return;
	}

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		return;
	}

	if (g_aCharInfo[g_nMyCharID]->dwMapID != pChar->dwMapID) {
		return;
	}

	nNo = GetCharNo (pChar->pID);
	/* 登録済み？ */
	if (nNo >= 0) {
		pCharDst = g_aCharInfo[nNo];
		if (g_pMapInfo->pdwMapChar) {
			if ((pCharDst->dwX <= (DWORD)g_pMapInfo->nCX) && (pCharDst->dwY <= (DWORD)g_pMapInfo->nCY)) {
				g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] --;
			}
		}
		/* 削除中、または別のキャラに入れ替わった（倒れて消えかけの敵の番号を、
		   雇ったばかりの傭兵などがもらった）？ 前のキャラのダメージ表示などは引き継がない */
		if (pCharDst->dwTimeDelete || (strcmp (pCharDst->szName, pChar->szName) != 0)) {
			ZeroMemory (pCharDst->aCharEfcInfo, sizeof (pCharDst->aCharEfcInfo));
		}
		/* 削除中？ */
		if (pCharDst->dwTimeDelete) {
			pCharDst->dwTimeDelete = 0;
			pCharDst->dwTimeAdd = timeGetTime ();
		}

	} else {
		pCharDst = new CHARINFO;
		ZeroMemory ((PBYTE)pCharDst, sizeof (CHARINFO));
		pCharDst->dwTimeAdd = timeGetTime ();
		pCharDst->dwTargetTblID3	= -1;
		pCharDst->dwTailTblID		= -1;
		pCharDst->dwTailBackTblID	= -1;
	}

	pCharDst->pID			= pChar->pID;
	pCharDst->dwTblID		= pChar->dwTblID;
	pCharDst->dwMapID		= pChar->dwMapID;
	pCharDst->byAdminLV		= pChar->byAdminLV;
	pCharDst->byCharState	= pChar->byCharState;
	pCharDst->byDirection	= pChar->byDirection;
	pCharDst->byRoofLV		= pChar->byRoofLV;
	pCharDst->byEfect		= pChar->byEfect;
	pCharDst->byEfectIdle	= pChar->byEfectIdle;
	pCharDst->byNpcType		= pChar->byNpcType;
	pCharDst->byLightLevel	= pChar->byLightLevel;
	pCharDst->wGrpID		= pChar->wGrpID;
	pCharDst->wGrpMain		= pChar->wGrpMain;
	pCharDst->wGrpSub		= pChar->wGrpSub;
	pCharDst->wGrpTmpID		= pChar->wGrpTmpID;
	pCharDst->wGrpTmpMain	= pChar->wGrpTmpMain;
	pCharDst->wGrpTmpSub	= pChar->wGrpTmpSub;
	pCharDst->wMaxHP		= pChar->wMaxHP;
	pCharDst->wHP			= pChar->wHP;
	pCharDst->wMaxMana		= pChar->wMaxMana;
	pCharDst->wMana			= pChar->wMana;
	pCharDst->dwXBack		= pChar->dwX;
	pCharDst->dwYBack		= pChar->dwY;
	pCharDst->dwX			= pChar->dwX;
	pCharDst->dwY			= pChar->dwY;
	pCharDst->byDotMove		= 0;
	pCharDst->clName		= pChar->clName;
	pCharDst->clSpeech		= pChar->clSpeech;
	pCharDst->byEfcAnime	= 0;
	pCharDst->dwTimeEfc		= 0;
	pCharDst->dwStTmplateID	= -1;

	pCharDst->dwSpeechCRC = 0;
	SAFE_DELETE_ARRAY (pCharDst->pszNpcSpeech);
	pCharDst->pszNpcSpeech = pChar->pszNpcSpeech;
	CopyMemory (pCharDst->szName, pChar->szName, sizeof (pCharDst->szName));
	if (pChar->pszNpcSpeech) {
		LPSTR pszTmp = (LPSTR)pDataNext;
		int nSize = strlen (pszTmp) + 1;

		pCharDst->pszNpcSpeech = new char[nSize];
		CopyMemory (pCharDst->pszNpcSpeech, pszTmp, nSize);
		pCharDst->dwSpeechCRC = g_crc.GetCRC ((PBYTE)pCharDst->pszNpcSpeech, nSize);
	}
	if (pCharDst->byEfectIdle) {
		pCharDst->dwTimeEfc = timeGetTime ();

	} else if (pCharDst->byEfect) {
		switch (pCharDst->byNpcType) {
		case NPCTYPE_USEMAGIC:
		case NPCTYPE_USEMAGICPARENT:
		case NPCTYPE_ATACK:
			pCharDst->dwTimeEfc = timeGetTime ();
			break;
		}
	}

	if (nNo < 0) {
		g_aCharInfo.Add (pCharDst);
	}

	if (g_pMapInfo->pdwMapChar) {
		if ((pCharDst->dwX <= (DWORD)g_pMapInfo->nCX) && (pCharDst->dwY <= (DWORD)g_pMapInfo->nCY)) {
			g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] ++;
		}
	}

	g_nMyCharID = GetCharID (g_pMyID);

	if (g_nMyCharID >= 0) {
		/* 画面内にいるキャラ情報のチェック */
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			if (i == g_nMyCharID) {
				continue;
			}
			x = abs (g_aCharInfo[g_nMyCharID]->dwX - g_aCharInfo[i]->dwX);
			y = abs (g_aCharInfo[g_nMyCharID]->dwY - g_aCharInfo[i]->dwY);
			/* 画面外へ出てる？ */
			if (g_aCharInfo[i]->byDirection == 99) {
				g_aCharInfo[i]->dwTimeDelete = timeGetTime ();

			} else if ((x > DRAW_PARTS_X + 2) || (y > DRAW_PARTS_Y + 2)) {
				DeleteChar (i);
			}
		}
	}

	/* キャラ座標情報更新 */
	RenewCharPosInfo ();
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGDIRECTION											 */
/* 内容：受信データ処理(向き変更通知)										 */
/* 日付：2003/05/14															 */
/* ========================================================================= */
void RecvDataProcCHGDIRECTION(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo = GetCharNo (pPara->pID);

	if (g_nMyCharID < 0) {
		return;
	}

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		return;
	}

	if (g_aCharInfo.GetSize () == 0) {
		return;
	}

	if (nNo < 0) {
		SendCmdCHARINFOREQ (pPara->dwTblID);
		return;
	}

	int i;
	for (i = 0; i < g_aRecTbl.GetSize (); i ++) {
		DWORD dwTmp = g_aRecTbl[i];
		if (dwTmp == pPara->dwTblID) {
			g_aRecTbl.RemoveAt (i);
			break;
		}
	}

	if (nNo == g_nMyCharID) {
		g_bReqestPos = FALSE;
	}
	g_aCharInfo[nNo]->byDirection = ((PCMDPARA_DATA1)pData)->byData;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARPOS												 */
/* 内容：受信データ処理(座標通知)											 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void RecvDataProcCHARPOS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARPOS pChar = (PCMDPARA_CHARPOS)pData;
	int x = 0, y = 0, nNo = GetCharNo (pChar->pID);
	PCHARINFO pMyChar;

	if (nNo == g_nMyCharID) {
		g_bEnableMove = TRUE;
	}

	if (g_nMyCharID < 0) {
		return;
	}

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		return;
	}

	if (g_aCharInfo.GetSize () == 0) {
		return;
	}

	pMyChar = g_aCharInfo[g_nMyCharID];
	if (pMyChar->dwMapID != pChar->dwMapID) {
		return;
	}

	/* 知らないキャラ？ */
	if (nNo < 0) {
		SendCmdCHARINFOREQ (pChar->dwTblID);
		return;
	}

	int i;
	for (i = 0; i < g_aRecTbl.GetSize (); i ++) {
		DWORD dwTmp = g_aRecTbl[i];
		if (dwTmp == pChar->dwTblID) {
			g_aRecTbl.RemoveAt (i);
			break;
		}
	}

	if ((pChar->byX >= (BYTE)g_pMapInfo->nCX) || (pChar->byY >= (BYTE)g_pMapInfo->nCY)) {
		DeleteChar (nNo);
		return;
	}

	if (nNo == g_nMyCharID) {
		g_bReqestPos = FALSE;
		if (pMyChar->byTurnMagicLV) {
			/* 詠唱中止 */
			pMyChar->dwTimeTurnMagicLV = 0;
			pMyChar->byTurnMagicLV = 0;
			pMyChar->byTurnLV		= 0;
			pMyChar->dwTimeTurnLV = 0;
			SendCmdEFECTCHG (0, TRUE);
			SAFE_DELETE (g_pTargetSelectInfo);
			AddSysMsg ("集中力が途切れた為、特技の実行は中断されました。", RGB (255, 255, 255));
		}

	} else {
		x = abs (pMyChar->dwX - pChar->byX);
		y = abs (pMyChar->dwY - pChar->byY);
		/* 画面外へ出た？ */
		if ((x > DRAW_PARTS_X + 2) || (y > DRAW_PARTS_Y + 2)) {
			DeleteChar (nNo);
			return;
		}
	}

	if (pChar->byDirection == 99) {
		g_aCharInfo[nNo]->dwTimeDelete = timeGetTime ();
		return;
	}

	PCHARINFO pCharDst = g_aCharInfo[nNo];

	/* 削除中？ */
	if (pCharDst->dwTimeDelete) {
		pCharDst->dwTimeDelete = 0;
		pCharDst->dwTimeAdd = timeGetTime ();
	}

	if (pCharDst->dwX != pChar->byX || pCharDst->dwY != pChar->byY) {
		pCharDst->byDotMove = 0;
		if (pChar->byFlg != 1) {
			pCharDst->byDotMove = 31;
		}
	}
	/* 座標が変わった？ */
	if ((pCharDst->dwX != (DWORD)pChar->byX) || (pCharDst->dwY != (DWORD)pChar->byY)) {
		int nMapParts = g_pMapInfo->pwMap[pCharDst->dwX + pCharDst->dwY * g_pMapInfo->nCX];
		BYTE byHitType = (&g_pMapPartsInfo[nMapParts])->byHitType;
		/* 足跡の座標？ */
//Todo:画像ID設定
		if ((
			(pCharDst->byNpcType != NPCTYPE_USEMAGIC) &&
			(pCharDst->byNpcType != NPCTYPE_USEMAGICPARENT) &&
			(pCharDst->byNpcType != NPCTYPE_ATACK)) && (
			(byHitType == PARTSHIT_FOOTMARK_SNOW) ||
			(byHitType == PARTSHIT_FOOTMARK_SAND) ||
			(byHitType == PARTSHIT_WATERRIPPLE))) {
			POBJINFO pObj = new OBJINFO;
			ZeroMemory (pObj, sizeof (OBJINFO));

			switch (byHitType) {
			case PARTSHIT_FOOTMARK_SNOW:	/* 雪 */
				pObj->byTypeMain = OBJTYPE_FOOTMARK_SNOW;
				pObj->byLevel = 10;
				if (pCharDst->byDirection >= 2) {
					pObj->wGrpID = 1;
				}
				break;

			case PARTSHIT_FOOTMARK_SAND:	/* 砂 */
				pObj->byTypeMain = OBJTYPE_FOOTMARK_SAND;
				pObj->wGrpID = 2;
				pObj->byLevel = 10;
				if (pCharDst->byDirection >= 2) {
					pObj->wGrpID = 3;
				}
				break;

			case PARTSHIT_WATERRIPPLE:		/* 水の波紋 */
				pObj->byTypeMain = OBJTYPE_WATERRIPPLE;
				pObj->byLevel = 30;
				break;
			}
			pObj->byX = (BYTE)pCharDst->dwX;
			pObj->byY = (BYTE)pCharDst->dwY;

			/* 足跡用オブジェクトを追加 */
			g_aObjInfo.Add (pObj);
		}

		if (nNo == g_nMyCharID) {
			/* 戦場ゲーム中？ */
			if (pCharDst->abySubStatus[0] == SUBGAMEID_WAR) {
				g_pWarInfo->byMove --;
			}
		}
	}

	g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] --;
	/* 立体交差の橋の上か下かを覚え直す(動く前の位置と向きから) */
	if ((pCharDst->dwX != (DWORD)pChar->byX) || (pCharDst->dwY != (DWORD)pChar->byY)) {
		RenewCharBridgeLayer (pCharDst, (int)pChar->byX, (int)pChar->byY);
	}
	pCharDst->byDirection = pChar->byDirection;
	if (pCharDst->dwXBack == -1) {
		pCharDst->dwXBack = (DWORD)pChar->byX;
		pCharDst->dwYBack = (DWORD)pChar->byY;
	} else {
		pCharDst->dwXBack = pCharDst->dwX;
		pCharDst->dwYBack = pCharDst->dwY;
	}
	pCharDst->dwX = (DWORD)pChar->byX;
	pCharDst->dwY = (DWORD)pChar->byY;
	pCharDst->dwLastDotMove = 0;
	g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] ++;
	g_bDraw = TRUE;

	/* キャラ座標情報更新 */
	RenewCharPosInfo ();
}

/* ========================================================================= */
/* 関数名：RecvDataProcSPEECH												 */
/* 内容：受信データ処理(発言通知)											 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void RecvDataProcSPEECH(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SPEECH pPara = (PCMDPARA_SPEECH)pData;
	int nNo = GetCharNo (((PCMDPARA_DATA1)pData)->pID);
	BOOL bFlg = FALSE;
	BOOL bSpeech = FALSE;		/* プレイヤーの発言（発言の音を鳴らす） */

	LPSTR pszTmp = (LPSTR)new BYTE[pPara->wSize + 1], pszTmp2;
	ZeroMemory (pszTmp, pPara->wSize + 1);
	memcpy (pszTmp, pData + sizeof (CMDPARA_SPEECH), pPara->wSize);

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		SAFE_DELETE_ARRAY (pszTmp);
		return;
	}

	if (g_aCharInfo.GetSize () == 0) {
		SAFE_DELETE_ARRAY (pszTmp);
		return;
	}

	COLORREF cl = RGB(255, 255, 255);
	/* 全員に配信メッセージ？ */
	if (pPara->byType == SPEECHCMD_ALL) {
		cl = RGB (0, 200, 255);
		bFlg = TRUE;
		bSpeech = TRUE;

	} else if (pPara->byType == SPEECHCMD_GM) {
		cl = RGB (255, 255, 255);
		bFlg = TRUE;

	/* 自己紹介文更新？ */
	} else if (pPara->byType == SPEECHCMD_SPEECHEDIT) {
		if (nNo >= 0) {
			g_aCharInfo[nNo]->dwSpeechCRC = 0;
			SAFE_DELETE_ARRAY (g_aCharInfo[nNo]->pszNpcSpeech);
			g_aCharInfo[nNo]->pszNpcSpeech = pszTmp;
			g_aCharInfo[nNo]->dwSpeechCRC = g_crc.GetCRC ((PBYTE)pszTmp, strlen (pszTmp) + 1);
		}
		return;

	/* マップ内放送？ */
	} else if (pPara->byType == SPEECHCMD_MAP) {
		bFlg = TRUE;
		cl = pPara->color;

	/* 画面内放送？ */
	} else if (pPara->byType == SPEECHCMD_SCREENCHAR) {
		bFlg = TRUE;
		cl = pPara->color;

	/* 仲間のパーティ発言？（文字は「[PT]名前：発言」で届く。仲間にはどこにいても届く） */
	} else if (pPara->byType == SPEECHCMD_PARTY) {
		bFlg = TRUE;
		bSpeech = TRUE;
		cl = pPara->color;
		/* 見えている仲間なら、普通の発言と同じく吹き出しも出す（画面に文字は流さない） */
		if ((nNo >= 0) && (g_nMyCharID >= 0)) {
			PCHARINFO pCharSpeech = g_aCharInfo[nNo];
			int nName = strlen (pCharSpeech->szName);
			int nHead = 4 + nName;
			int nSep = strlen ("：");
			if ((strncmp (pszTmp, "[PT]", 4) == 0) &&
				(strncmp (&pszTmp[4], pCharSpeech->szName, nName) == 0) &&
				(strncmp (&pszTmp[nHead], "：", nSep) == 0) &&
				(abs ((int)g_aCharInfo[g_nMyCharID]->dwX - (int)pCharSpeech->dwX) <= DRAW_PARTS_X) &&
				(abs ((int)g_aCharInfo[g_nMyCharID]->dwY - (int)pCharSpeech->dwY) <= DRAW_PARTS_Y)) {
				LPCSTR pszBody = &pszTmp[nHead + nSep];
				SAFE_DELETE_ARRAY (pCharSpeech->pszSpeech);
				pCharSpeech->pszSpeech = new char[strlen (pszBody) + 1];
				strcpy (pCharSpeech->pszSpeech, pszBody);
				pCharSpeech->dwLastSpeech = timeGetTime ();
				bFlg = FALSE;
			}
		}

	} else if (nNo >= 0) {

		int x, y;
		x = abs (g_aCharInfo[g_nMyCharID]->dwX - g_aCharInfo[nNo]->dwX);
		y = abs (g_aCharInfo[g_nMyCharID]->dwY - g_aCharInfo[nNo]->dwY);

		SAFE_DELETE_ARRAY (g_aCharInfo[nNo]->pszSpeech);
		g_aCharInfo[nNo]->pszSpeech = pszTmp;
		g_aCharInfo[nNo]->dwLastSpeech = timeGetTime ();
		int nSize = pPara->wSize + strlen (g_aCharInfo[nNo]->szName) + 3;
		pszTmp2 = (LPSTR)new BYTE[nSize];
		ZeroMemory (pszTmp2, nSize);
		wsprintf (pszTmp2, "%s：%s", g_aCharInfo[nNo]->szName, pszTmp);
		pszTmp = pszTmp2;
		cl = g_aCharInfo[nNo]->clSpeech;
		if (cl == 0) {
			cl = RGB (255, 255, 255);
		}
		bSpeech = TRUE;

	} else {
		goto Exit;
	}

	/* 画面に文字を流す？ */
	if (bFlg) {
		AddSysMsg (pszTmp, cl);
	}

	AddMsgLog (pszTmp, cl);

	/* 発言の音（SBOP2 と同じく、キャラの発言と全体発言で鳴らす。お知らせ類は鳴らさない） */
	if (bSpeech) {
		if (g_bChatSound) {
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CHAT]);
		}
#ifdef __EMSCRIPTEN__
		SboWebChatNotify ();
#endif
	}
Exit:
	SAFE_DELETE_ARRAY (pszTmp);
}

/* ========================================================================= */
/* 関数名：RecvDataProcONLINE												 */
/* 内容：受信データ処理(オンライン数通知)									 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void RecvDataProcONLINE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ONLINE pPara = (PCMDPARA_ONLINE)pData;

	g_byAdminCount = pPara->byAdmin;
	g_dwOnilne = pPara->dwData;
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPPARTS												 */
/* 内容：受信データ処理(マップパーツ更新)									 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void RecvDataProcMAPPARTS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MAPPARTSINFO pParts = (PCMDPARA_MAPPARTSINFO)pData;
	PMAPPARTSINFO pPartsTmp;
	PMAPPARTSINFOSAVE pRecv;

	/* 追加？ */
	if (pParts->wCount >= g_wCountMapParts) {
		if (g_byGameState < GAMESTATE_MOVE) {
			return;
		}
		PMAPPARTSINFO pParts = new MAPPARTSINFO [g_wCountMapParts + 1];
		ZeroMemory (pParts, sizeof (MAPPARTSINFO) * (g_wCountMapParts + 1));
		CopyMemory (pParts, g_pMapPartsInfo, sizeof (MAPPARTSINFO) * (g_wCountMapParts + 1));

		pPartsTmp = &pParts[g_wCountMapParts];
		pRecv = (PMAPPARTSINFOSAVE)(pData + sizeof (CMDPARA_MAPPARTSINFO));
		pPartsTmp->byViewType = pRecv->byViewType;
		pPartsTmp->byHitType = pRecv->byHitType;
		pPartsTmp->byAnimeType = pRecv->byAnimeType;
		pPartsTmp->byAnimeCount = pRecv->byAnimeCount;
		pPartsTmp->wGrpID = pRecv->wGrpID;
		pPartsTmp->wBackID = pRecv->wBackID;
		pPartsTmp->pAnimeInfo = pRecv->pAnimeInfo;
		pPartsTmp->byLevel = pRecv->byLevel;
		pPartsTmp->byLight = pRecv->byLight;
		pRecv += sizeof (MAPPARTSINFOSAVE);
		if (pPartsTmp->pAnimeInfo) {
			pPartsTmp->pAnimeInfo = new ANIMEINFO[pPartsTmp->byAnimeCount];
			CopyMemory (pPartsTmp->pAnimeInfo, pRecv, pPartsTmp->byAnimeCount * sizeof (ANIMEINFO));
		}

		SAFE_DELETE_ARRAY (g_pMapPartsInfo);
		g_pMapPartsInfo = pParts;
		g_wCountMapParts ++;

	} else {
		pPartsTmp = &g_pMapPartsInfo[pParts->wCount];
		if (pPartsTmp->pAnimeInfo) {
			SAFE_DELETE_ARRAY (pPartsTmp->pAnimeInfo);
		}

		pRecv = (PMAPPARTSINFOSAVE)(pData + sizeof (CMDPARA_MAPPARTSINFO));
		pPartsTmp->byViewType = pRecv->byViewType;
		pPartsTmp->byHitType = pRecv->byHitType;
		pPartsTmp->byAnimeType = pRecv->byAnimeType;
		pPartsTmp->byAnimeCount = pRecv->byAnimeCount;
		pPartsTmp->wGrpID = pRecv->wGrpID;
		pPartsTmp->wBackID = pRecv->wBackID;
		pPartsTmp->pAnimeInfo = pRecv->pAnimeInfo;
		pPartsTmp->byLevel = pRecv->byLevel;
		pPartsTmp->byLight = pRecv->byLight;
		pRecv ++;
		if (pPartsTmp->pAnimeInfo) {
			pPartsTmp->pAnimeInfo = new ANIMEINFO[pPartsTmp->byAnimeCount];
			CopyMemory (pPartsTmp->pAnimeInfo, pRecv, pPartsTmp->byAnimeCount * sizeof (ANIMEINFO));
		}
	}

	delete g_pDibMapParts;
	g_pDibMapParts = new CImg32;
	g_pDibMapParts->CreateWithoutGdi (16 * 32, (g_wCountMapParts / 16 + 1) * 32);
	g_pDibMapParts->Clear ();

	int i, no, xx, yy;
	DWORD dwID;
	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = &g_pMapPartsInfo[i];
		if (pParts->wBackID) {
			no = pParts->wBackID / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = pParts->wBackID % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy);
		}
		no = pParts->wGrpID / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = pParts->wGrpID % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = (dwID % g_byMapGrpCountX) * 32;
		yy = dwID / g_byMapGrpCountX * 32;
		if (pParts->byLevel) {
			g_pDibMapParts->BltAlpha ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, pParts->byLevel, TRUE);
		} else {
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, TRUE);
		}
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_MAPPARTSCOUNT, 0, 0);
	}

	/* マップパーツのアニメーションカウンタを初期化 */
	DWORD dwTmp = timeGetTime ();
	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i];
		g_dwTimeLastMapParts = timeGetTime ();
		pParts->dwLastAnime = g_dwTimeLastMapParts;
		pParts->byAnimeNo = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGMAP												 */
/* 内容：受信データ処理(マップ更新)											 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void RecvDataProcCHGMAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGMAP pMap = (PCMDPARA_CHGMAP)pData;
	if (g_pMapInfo->dwID != pMap->dwID) {
		return;
	}

	g_pMapInfo->pwMap[pMap->nX + pMap->nY * g_pMapInfo->nCX] = pMap->wMap;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDDELMAP											 */
/* 内容：受信データ処理(マップ幅増減)										 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void RecvDataProcADDDELMAP(PBYTE pData, DWORD dwDataSize)
{
	int i, nSize;
	WORD *pNewMap;
	DWORD *pdwNewMap;
	PBYTE pbyNewMap;
	PCMDPARA_ADDDELMAP pMap = (PCMDPARA_ADDDELMAP)pData;

	if (g_pMapInfo->dwID != pMap->dwID) {
		return;
	}

	switch (pMap->byType) {
	case 0:
		nSize = g_pMapInfo->nCX * (g_pMapInfo->nCY + pMap->byCount);
		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		CopyMemory (pNewMap + g_pMapInfo->nCX * pMap->byCount,
				g_pMapInfo->pwMap,
				(g_pMapInfo->nCX * g_pMapInfo->nCY) * sizeof (WORD));
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		g_pMapInfo->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		CopyMemory (pdwNewMap + (g_pMapInfo->nCX * pMap->byCount),
				g_pMapInfo->pdwMapChar,
				(g_pMapInfo->nCX * g_pMapInfo->nCY) * sizeof (DWORD));
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
		g_pMapInfo->pdwMapChar = pdwNewMap;

		pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		CopyMemory (pbyNewMap + (g_pMapInfo->nCX * pMap->byCount),
				g_pMapInfo->pbyFlg,
				g_pMapInfo->nCX * g_pMapInfo->nCY);
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		g_pMapInfo->pbyFlg = pbyNewMap;

/*
			BYTE *pbyNewMap = new BYTE[nSize];
			ZeroMemory (pbyNewMap, nSize);
			CopyMemory (pbyNewMap + g_pMapInfo->nCX * pMap->byCount, g_pMapInfo->pbyRoofLV,
					g_pMapInfo->nCX * g_pMapInfo->nCY);
			SAFE_DELETE_ARRAY (g_pMapInfo->pbyRoofLV);
			g_pMapInfo->pbyRoofLV = pbyNewMap;
*/
		g_pMapInfo->nCY += pMap->byCount;
		break;

	case 1:
		nSize = g_pMapInfo->nCX * (g_pMapInfo->nCY + pMap->byCount);
		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		CopyMemory (pNewMap, g_pMapInfo->pwMap,
				(g_pMapInfo->nCX * g_pMapInfo->nCY) * sizeof (WORD));
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		g_pMapInfo->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		CopyMemory (pdwNewMap, g_pMapInfo->pdwMapChar,
				(g_pMapInfo->nCX * g_pMapInfo->nCY) * sizeof (DWORD));
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
		g_pMapInfo->pdwMapChar = pdwNewMap;

		pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		CopyMemory (pbyNewMap, g_pMapInfo->pbyFlg, g_pMapInfo->nCX * g_pMapInfo->nCY);
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		g_pMapInfo->pbyFlg = pbyNewMap;
/*
			BYTE *pbyNewMap = new BYTE[nSize];
			ZeroMemory (pbyNewMap, nSize);
			CopyMemory (pbyNewMap, g_pMapInfo->pbyRoofLV,
					g_pMapInfo->nCX * g_pMapInfo->nCY);
			SAFE_DELETE_ARRAY (g_pMapInfo->pbyRoofLV);
			g_pMapInfo->pbyRoofLV = pbyNewMap;
*/
		g_pMapInfo->nCY += pMap->byCount;
		break;

	case 2:
		nSize = (g_pMapInfo->nCX + pMap->byCount) * g_pMapInfo->nCY;
		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pNewMap + pMap->byCount + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pwMap + g_pMapInfo->nCX * i,
					g_pMapInfo->nCX * sizeof (WORD));
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		g_pMapInfo->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pdwNewMap + pMap->byCount + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pdwMapChar + g_pMapInfo->nCX * i,
					g_pMapInfo->nCX * sizeof (DWORD));
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
		g_pMapInfo->pdwMapChar = pdwNewMap;

		pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pbyNewMap + pMap->byCount + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pbyFlg + g_pMapInfo->nCX * i, g_pMapInfo->nCX);
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		g_pMapInfo->pbyFlg = pbyNewMap;

		g_pMapInfo->nCX += pMap->byCount;
		break;

	case 3:
		nSize = (g_pMapInfo->nCX + pMap->byCount) * g_pMapInfo->nCY;
		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pNewMap + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pwMap + g_pMapInfo->nCX * i,
					g_pMapInfo->nCX * sizeof (WORD));
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pwMap);
		g_pMapInfo->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pdwNewMap + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pdwMapChar + g_pMapInfo->nCX * i,
					g_pMapInfo->nCX * sizeof (DWORD));
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pdwMapChar);
		g_pMapInfo->pdwMapChar = pdwNewMap;

		pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		for (i = 0; i < g_pMapInfo->nCY; i ++) {
			CopyMemory (pbyNewMap + (g_pMapInfo->nCX + pMap->byCount) * i,
					g_pMapInfo->pbyFlg + g_pMapInfo->nCX * i,
					g_pMapInfo->nCX);
		}
		SAFE_DELETE_ARRAY (g_pMapInfo->pbyFlg);
		g_pMapInfo->pbyFlg = pbyNewMap;

		g_pMapInfo->nCX += pMap->byCount;
		break;
	}

	if (g_hWndAdmin) {
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}

	g_aCharInfo[g_nMyCharID]->dwXBack = -1;
	g_aCharInfo[g_nMyCharID]->dwYBack = -1;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGEVENT												 */
/* 内容：受信データ処理(イベント情報更新)									 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void RecvDataProcCHGEVENT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGEVENT pEvent = (PCMDPARA_CHGEVENT)pData;
	int nSize;

	if (g_hWndAdmin == NULL) {
		return;
	}

	if (g_pEventInfo) {
		SAFE_DELETE_ARRAY (g_pEventInfo->pData);
		SAFE_DELETE (g_pEventInfo);
	}
	g_pEventInfo = new EVENTINFO;
	g_pEventInfo->dwX = pEvent->dwX;
	g_pEventInfo->dwY = pEvent->dwY;
	g_pEventInfo->byType = pEvent->byType;
	g_pEventInfo->pData = pEvent->pData;
	if (pEvent->pData) {
		nSize = GetEventDataSize (pEvent->byType);
		g_pEventInfo->pData = new BYTE[nSize];
		CopyMemory (g_pEventInfo->pData, pEvent + 1, nSize);
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHGEVENT, pEvent->dwDataID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCOUNTEVENT											 */
/* 内容：受信データ処理(イベント数通知)										 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void RecvDataProcCOUNTEVENT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA2 pEvent = (PCMDPARA_DATA2)pData;

	g_nEventCount = pEvent->nData;

	if (g_hWndAdmin) {
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcEVENTPOS												 */
/* 内容：受信データ処理(イベント座標通知)									 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void RecvDataProcEVENTPOS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_EVENTPOS pEventPos = (PCMDPARA_EVENTPOS)pData;
	PXYINFO pEvent;
	int i;
	DWORD j;

	if (!g_pMapInfo->pbyFlg) {
		return;
	}

	/* 現在のイベント座標データをクリア */
	for (j = 0; j < (DWORD)(g_pMapInfo->nCX * g_pMapInfo->nCY); j ++) {
		g_pMapInfo->pbyFlg[j] &= ~MAPFLG_EVENT;
	}

	pEvent = (PXYINFO)((PBYTE)pEventPos + sizeof (CMDPARA_EVENTPOS));
	for (i = 0; i < (int)pEventPos->dwCount; i ++) {
		g_pMapInfo->pbyFlg[pEvent->dwX + g_pMapInfo->nCX * pEvent->dwY] |= MAPFLG_EVENT;
		pEvent ++;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcHARTBEAT												 */
/* 内容：受信データ処理(生存確認応答)										 */
/* 日付：2003/06/17															 */
/* ========================================================================= */
void RecvDataProcHARTBEAT(PBYTE pData, DWORD dwDataSize)
{
	if (g_nMyCharID >= 0) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		pChar->dwLastHartBeat = timeGetTime ();
	}
	g_dwTimeHartbeat = timeGetTime () - g_dwTimeHartbeatReq;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGBGM												 */
/* 内容：受信データ処理(BGM変更)											 */
/* 日付：2003/07/13															 */
/* ========================================================================= */
void RecvDataProcCHGBGM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	StopBGM ();
	g_pMapInfo->byBGM = pPara->byData;

	if (pPara->byData && pPara->byData < SOUND_MAX) {
		PlayBGM (0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcMSG													 */
/* 内容：受信データ処理(メッセージ変更)										 */
/* 日付：2003/07/15															 */
/* ========================================================================= */
void RecvDataProcMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PMAPINFO pMap = g_pMapInfo;

	LPSTR *pNew = new LPSTR[pMap->nMsgCount + 1];
	ZeroMemory (pNew, sizeof (LPSTR) * (pMap->nMsgCount + 1));
	if (pMap->nMsgCount) {
		CopyMemory (pNew, pMap->pMsg, sizeof (LPSTR) * pMap->nMsgCount);
		SAFE_DELETE_ARRAY (pMap->pMsg);
	}

	LPSTR pMsg, pNewMsg;
	pMsg = (LPSTR)pData + sizeof (CMDPARA_DATA1);
	pNewMsg = new char[strlen ((LPSTR)pMsg) + 1];
	ZeroMemory (pNewMsg, strlen ((LPSTR)pMsg) + 1);
	strcat ((LPSTR)pNewMsg, (LPSTR)pMsg);

	pNew[pMap->nMsgCount] = pNewMsg;
	pMap->pMsg = pNew;
	pMap->nMsgCount ++;

	if (g_hWndAdmin) {
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcVIEWMSG												 */
/* 内容：受信データ処理(メッセージ表示)										 */
/* 日付：2003/07/16															 */
/* ========================================================================= */
void RecvDataProcVIEWMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	PWNDINFO pWndTmp;
	pWndTmp = MakeWindowMSG ((LPSTR)g_pMapInfo->pMsg[pPara->dwData], FALSE);
	g_WndInfo.Add (pWndTmp);
	g_dwActiveMsgID = pWndTmp->dwID;
	g_bEnableMove = TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcPLAYSOUND											 */
/* 内容：受信データ処理(効果音再生)											 */
/* 日付：2003/07/20															 */
/* ========================================================================= */
void RecvDataProcPLAYSOUND(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	DXAudio.PlaySecoundary (&aSoundTbl[pPara->byData]);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPMSG												 */
/* 内容：受信データ処理(メッセージ通知)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void RecvDataProcMAPMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	PWNDINFO pWndTmp;
	pWndTmp = MakeWindowMSG ((LPSTR)(pPara + 1), FALSE);
	g_WndInfo.Add (pWndTmp);
	g_dwActiveMsgID = pWndTmp->dwID;
	g_bEnableMove = TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPMSGREQ											 */
/* 内容：受信データ処理(メッセージ通知)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void RecvDataProcMAPMSGREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	if (g_hWndAdmin) {
		DWORD dwSize = strlen ((LPSTR)(pPara + 1)) + 1;
		PBYTE pMsg = new BYTE[dwSize];
		ZeroMemory (pMsg, dwSize);
		strcat ((LPSTR)pMsg, (LPSTR)(pPara + 1));
		PostMessage (g_hWndAdmin, WM_ADMINMSGREQ, (WPARAM)pMsg, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSYSMSG												 */
/* 内容：受信データ処理(システムメッセージ)									 */
/* 日付：2003/08/03															 */
/* ========================================================================= */
void RecvDataProcSYSMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SPEECH pPara = (PCMDPARA_SPEECH)pData;

	if (g_byGameState < GAMESTATE_MOVE) {
		return;
	}

	LPSTR pszTmp = (LPSTR)new BYTE[pPara->wSize + 1];
	ZeroMemory (pszTmp, pPara->wSize + 1);
	memcpy (pszTmp, pData + sizeof (CMDPARA_SPEECH), pPara->wSize);

	switch (pPara->byType) {
	case SPEECHCMD_BATTLE1:	/* 戦闘関連(ログ保存しない) */
		AddSysMsg (pszTmp, RGB (255, 255, 255));
		break;

	case SPEECHCMD_SCREEN:	/* 画面内放送 */
		AddSysMsg (pszTmp, RGB (255, 255, 255));
		AddMsgLog (pszTmp, RGB (255, 255, 255));
		break;

	default:
		AddSysMsg (pszTmp, RGB (0, 200, 255));
		AddMsgLog (pszTmp, RGB (0, 200, 255));
		break;
	}
	SAFE_DELETE_ARRAY (pszTmp);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEFECTCHG												 */
/* 内容：受信データ処理(エフェクト変更)										 */
/* 日付：2003/08/11															 */
/* ========================================================================= */
void RecvDataProcEFECTCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;
	int nNo = GetCharNo (((PCMDPARA_DATA4)pData)->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	if (pPara->dwData) {
		pChar->byEfectIdle = pPara->byData;
		if (pChar->byEfectIdle == 0) {
			pChar->dwTimeEfc = 0;
			pChar->byEfcAnime = 0;
		}
		return;
	} else {
		pChar->byEfect = pPara->byData;
	}
	pChar->dwTimeEfc = 0;
	pChar->byEfcAnime = 0;

	if (pChar->byNpcType) {
		switch (pChar->byNpcType) {
		case NPCTYPE_NOMOVE:
		case NPCTYPE_MOVERAND:
		case NPCTYPE_BLOCK:
		case NPCTYPE_BATTLE1:
		case NPCTYPE_BATTLE2:
		case NPCTYPE_USEMAGIC:
		case NPCTYPE_USEMAGICPARENT:
		case NPCTYPE_ATACK:
			break;
		case NPCTYPE_BOMB:
		case NPCTYPE_BOMBBLOCK:
		case NPCTYPE_BOMBITEM:
			pChar->dwLastAnime = timeGetTime ();
			if (pChar->byEfect != 0) {
				pChar->byAnime = 0;
				g_bDraw = TRUE;
			}
			return;
		default:
			return;
		}
	}
	pChar->dwTimeEfc = timeGetTime ();
	PEFCINFO pEfc = g_EfcInfo.GetPtr (pChar->byEfect - 1);
	/* 音付き？ */
	if (pEfc && pEfc->bySound) {
		if (g_bComEfcONOFF) {
			BYTE byNo = 0;
			for (int i = 0; ; i ++) {
				if (adwSoundNoTbl[i] == -1) {
					break;
				}
				if (pEfc->bySound == adwSoundNoTbl[i]) {
					byNo = i;
					break;
				}
			}
			DXAudio.PlaySecoundary (&aSoundTbl[adwSoundNoTbl[byNo]]);
		}
	}
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCOMEFC												 */
/* 内容：受信データ処理(吹き出し通知)										 */
/* 日付：2003/08/12															 */
/* ========================================================================= */
void RecvDataProcCOMEFC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo = GetCharNo (((PCMDPARA_DATA1)pData)->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	pChar->byComEfc = pPara->byData;
	pChar->byComEfcAnime = 0;
	pChar->dwTimeComEfc = timeGetTime ();

	PCOMEFCINFO pComEfc = &g_pComEfcInfo[pPara->byData - 1];

	/* 効果音あり？ */
	if (pComEfc->bySound) {
		if (g_bComEfcONOFF) {
			BYTE byNo = 0;
			for (int i = 0; ; i ++) {
				if (adwSoundNoTbl[i] == -1) {
					break;
				}
				if (pComEfc->bySound == adwSoundNoTbl[i]) {
					byNo = i;
					break;
				}
			}
			DXAudio.PlaySecoundary (&aSoundTbl[adwSoundNoTbl[byNo]]);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcERR													 */
/* 内容：受信データ処理(エラー通知)											 */
/* 日付：2003/08/14															 */
/* ========================================================================= */
void RecvDataProcERR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PWNDINFO pWnd = NULL;

	switch (pPara->byData) {
	case SBOERR_NAME:
		if (g_byGameState < GAMESTATE_MOVE) {
			pWnd = MakeWindowMSG3SEC ("その名前はすでに使われています。");
			pWnd->pParent = g_WndInfo[GetWindowID (g_dwActiveMsgID)];
			g_dwActiveMsgID = pWnd->dwID;
			PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
			ZeroMemory (pChar->szName, sizeof (pChar->szName));
			int nNo = GetWindowID (g_dwMsgIDMENUCHARINFO);
			g_WndInfo[nNo]->dwTimeDrawStart = 0;
		}
		break;
	}
	if (pWnd) {
		g_bDraw = TRUE;
		pWnd->nData1 = MSG3SEC_ACC;
		g_WndInfo.Add (pWnd);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcGETACC												 */
/* 内容：受信データ処理(アカウント・パスワード取得)							 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void RecvDataProcGETACC(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_ACCADD pPara = (PCMDPARA_ACCADD)pData;

	if (!g_hWndAdmin) {
		return;
	}

	LPSTR pszAcc, pszPas;
	pszAcc = new char[MAX_ACCNAME];
	pszPas = new char[MAX_ACCNAME];
	ZeroMemory (pszAcc, MAX_ACCNAME);
	ZeroMemory (pszPas, MAX_ACCNAME);
	wsprintf (pszAcc, "%s", pPara->szAcc);
	wsprintf (pszPas, "%s", pPara->szPas);
	PostMessage (g_hWndAdmin, WM_ADMINGETACC, (WPARAM)pszAcc, (WPARAM)pszPas);
}

/* ========================================================================= */
/* 関数名：RecvDataProcGETACCINFO											 */
/* 内容：受信データ処理(アカウント情報要求)									 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void RecvDataProcGETACCINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_CHARINFO pTmp;

	if (!g_hWndAdmin) {
		return;
	}

	pTmp = new CMDPARA_CHARINFO;
	ZeroMemory (pTmp, sizeof (CMDPARA_CHARINFO));
	SboPackGetCHARINFO (pTmp, pData, NULL);

	PostMessage (g_hWndAdmin, WM_ADMINGETACCINFO, (WPARAM)pTmp, 0);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCEFINFO												 */
/* 内容：受信データ処理(吹き出し情報)										 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void RecvDataProcCEFINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_CHGCEF pPara = (PCMDPARA_CHGCEF)pData;
	PCOMEFCINFO pComEfcNew;
	PBYTE pRead = (PBYTE)((PBYTE)pPara + sizeof (PCMDPARA_CHGCEF));

	for (int i = 0; i < 99; i ++) {
		PCOMEFCINFO pComEfc = &g_pComEfcInfo[i];
		SAFE_DELETE_ARRAY (pComEfc->pAnime);
		pComEfcNew = (PCOMEFCINFO)pRead;

		pComEfc->byAnimeCount = pComEfcNew->byAnimeCount;
		pComEfc->pAnime = pComEfcNew->pAnime;
		pComEfc->bySound = pComEfcNew->bySound;
		pRead += sizeof (COMEFCINFO);
		if (pComEfc->pAnime) {
			pComEfc->pAnime = new COMEFCANIMEINFO[pComEfc->byAnimeCount];
			CopyMemory (pComEfc->pAnime, pRead, sizeof (COMEFCANIMEINFO) * pComEfc->byAnimeCount);
			pRead += sizeof (COMEFCANIMEINFO) * pComEfc->byAnimeCount;
		}
	}
	if (g_hWndBalloon) {
		InvalidateRect (g_hWndBalloon, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGCEF												 */
/* 内容：受信データ処理(吹き出し情報更新)									 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void RecvDataProcCHGCEF(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGCEF pRecv = (PCMDPARA_CHGCEF)pData;
	PCOMEFCINFO pComEfc = &g_pComEfcInfo[pRecv->wCount], pComEfcNew;

	if (pComEfc->pAnime) {
		if (pComEfc->byAnimeCount == 1) {
			SAFE_DELETE (pComEfc->pAnime);
		} else {
			SAFE_DELETE_ARRAY (pComEfc->pAnime);
		}
	}

	pComEfcNew = (PCOMEFCINFO)(pData + sizeof (PCMDPARA_CHGCEF));
	pComEfc->byAnimeCount = pComEfcNew->byAnimeCount;
	pComEfc->pAnime = pComEfcNew->pAnime;
	pComEfcNew ++;
	if (pComEfc->pAnime) {
		pComEfc->pAnime = new COMEFCANIMEINFO[pComEfc->byAnimeCount];
		CopyMemory (pComEfc->pAnime, pComEfcNew, pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
	}
	if (g_hWndBalloon) {
		InvalidateRect (g_hWndBalloon, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcFADEINOUT											 */
/* 内容：受信データ処理(フェードインアウト)									 */
/* 日付：2003/09/11															 */
/* ========================================================================= */
void RecvDataProcFADEINOUT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	/* フェードアウト？ */
	if (pPara->byData == 0) {
		g_aFadeInOutTbl.Add (0);
		/* 暗くなりきるまで、続きの受信データ(次のマップ情報など)は処理しない */
		RecvDataHoldStart ();

	} else {
		g_aFadeInOutTbl.Add (1);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGITEM												 */
/* 内容：受信データ処理(アイテム情報更新)									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcCHGITEM(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_CHGITEM Para;
	PCMDPARA_CHGITEM pPara = &Para;
	PBYTE pDataPtr;

	/* 名前は可変長で届くので構造体に戻す（後ろにメッセージ・詳細データが続く） */
	pDataPtr = SboPackGetCHGITEM (&Para, pData, NULL);
	if (pDataPtr == NULL) {
		return;
	}

	DWORD dwCount = g_aItemInfo.GetSize ();
	PITEMINFO pItem = NULL;
	/* 追加？ */
	if (dwCount <= pPara->dwID) {
		if (dwCount + 2 <= pPara->dwID) {
			return;
		}
		PITEMINFO pItemNew = new ITEMINFO;
		ZeroMemory (pItemNew, sizeof (ITEMINFO));
		g_aItemInfo.Add (pItemNew);
		pItem = g_aItemInfo[dwCount];

	/* 更新 */
	} else {
		pItem = g_aItemInfo[pPara->dwID];
	}

	if (pItem->byType == ITEMTYPE_MSG) {
		PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
		if (pInfo) {
			SAFE_DELETE_ARRAY (pInfo->pMsgMe);
			SAFE_DELETE_ARRAY (pInfo->pMsgTo);
		}
	}
	SAFE_DELETE_ARRAY (pItem->pMsg);
	SAFE_DELETE_ARRAY (pItem->pData);
	ZeroMemory (pItem->szName, sizeof (pItem->szName));
	SboTextCopy (pItem->szName, sizeof (pItem->szName), pPara->szName);
	pItem->byType = pPara->byType;
	pItem->byData = pPara->byData;
	pItem->byFlg = pPara->byFlg;
	pItem->bySound = pPara->bySound;
	pItem->bySexDistinction = pPara->bySexDistinction;
	pItem->byEraseProbability = pPara->byEraseProbability;
	pItem->wGrpID = pPara->wGrpID;
	pItem->dwGold = pPara->dwGold;
	pItem->pData = pPara->pData;

	/* 使った時のメッセージ */
	if (pPara->pMsg) {
		int nLen = strlen ((LPSTR)pDataPtr) + 1;
		pItem->pMsg = new char[nLen];
		ZeroMemory (pItem->pMsg, nLen);
		strcat ((LPSTR)pItem->pMsg, (LPSTR)pDataPtr);
		pDataPtr += nLen;
	}

	/* 詳細データ */
	if (pItem->pData) {
		DWORD dwSize = GetItemDataSize (pItem->byType);
		pItem->pData = new BYTE[dwSize];
		ZeroMemory (pItem->pData, dwSize);
		pDataPtr = SboPackGetItemData (pItem->pData, pItem->byType, dwSize, pDataPtr, NULL);
		if (pItem->byType == ITEMTYPE_MSG) {
			PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
			if (pInfo) {
				if (pInfo->pMsgMe) {
					pInfo->pMsgMe = new char[strlen ((LPSTR)pDataPtr) + 1];
					strcpy ((LPSTR)pInfo->pMsgMe, (LPSTR)pDataPtr);
					pDataPtr += (strlen ((LPSTR)pDataPtr) + 1);
				}
				if (pInfo->pMsgTo) {
					pInfo->pMsgTo = new char[strlen ((LPSTR)pDataPtr) + 1];
					strcpy ((LPSTR)pInfo->pMsgTo, (LPSTR)pDataPtr);
					pDataPtr += (strlen ((LPSTR)pDataPtr) + 1);
				}
			}
		}
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINITEMCHG, pPara->dwID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcITEMINFO												 */
/* 内容：受信データ処理(アイテム情報)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcITEMINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	int i;

	/* 今持ってる情報をクリア */
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		if (g_aItemInfo[i]->byType == ITEMTYPE_MSG) {
			PITEMINFO_MSG pInfo = (PITEMINFO_MSG)g_aItemInfo[i]->pData;
			if (pInfo) {
				SAFE_DELETE_ARRAY (pInfo->pMsgMe);
				SAFE_DELETE_ARRAY (pInfo->pMsgTo);
			}
		}
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pData);
		SAFE_DELETE_ARRAY (g_aItemInfo[i]->pMsg);
		SAFE_DELETE (g_aItemInfo[i]);
	}

	PITEMINFO pItem = NULL;
	PBYTE pDataNew = (PBYTE)pPara + sizeof (CMDPARA_DATA3);
	for (i = 0; i < (int)pPara->dwData; i ++) {
		PITEMINFO pItemNew = new ITEMINFO;
		/* 名前は可変長で届くので構造体に戻す */
		ZeroMemory (pItemNew, sizeof (ITEMINFO));
		pDataNew = SboPackGetITEMINFO (pItemNew, pDataNew, NULL);

		/* 使った時のメッセージ */
		if (pItemNew->pMsg) {
			int nLen = strlen ((LPSTR)pDataNew) + 1;
			pItemNew->pMsg = new char[nLen];
			ZeroMemory (pItemNew->pMsg, nLen);
			strcat ((LPSTR)pItemNew->pMsg, (LPSTR)pDataNew);
			pDataNew += nLen;
		}
		/* 詳細データ */
		if (pItemNew->pData) {
			DWORD dwSize = GetItemDataSize (pItemNew->byType);
			PVOID pDataTmp = new BYTE[dwSize];
			ZeroMemory (pDataTmp, dwSize);
			pDataNew = SboPackGetItemData (pDataTmp, pItemNew->byType, dwSize, pDataNew, NULL);
			pItemNew->pData = pDataTmp;
			if (pItemNew->byType == ITEMTYPE_MSG) {
				PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItemNew->pData;
				if (pInfo) {
					if (pInfo->pMsgMe) {
						pInfo->pMsgMe = new char[strlen ((LPSTR)pDataNew) + 1];
						strcpy ((LPSTR)pInfo->pMsgMe, (LPSTR)pDataNew);
						pDataNew += (strlen ((LPSTR)pDataNew) + 1);
					}
					if (pInfo->pMsgTo) {
						pInfo->pMsgTo = new char[strlen ((LPSTR)pDataNew) + 1];
						strcpy ((LPSTR)pInfo->pMsgTo, (LPSTR)pDataNew);
						pDataNew += (strlen ((LPSTR)pDataNew) + 1);
					}
				}
			}
		}
		g_aItemInfo.Add (pItemNew);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARITEM												 */
/* 内容：受信データ処理(所持アイテム情報)									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcCHARITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARITEM pPara = (PCMDPARA_CHARITEM)pData;
	int nNo = GetCharNo (pPara->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	CopyMemory (pChar->waItem, pPara->waItem, sizeof (pChar->waItem));
	CopyMemory (pChar->waEqu, pPara->waEqu, sizeof (pChar->waEqu));
	RenewATDF (nNo);

	/* 自分の手持ちが変わったら（並べ替えなど）、開いている道具の一覧を描き直す */
	if (nNo == g_nMyCharID) {
		int i, nCount = GetItemCount (nNo);
		for (i = 0; i < g_WndInfo.GetSize (); i ++) {
			PWNDINFO pWnd = g_WndInfo[i];
			if (pWnd->nType != WNDTYPE_COMMANDITEM) {
				continue;
			}
			if (pWnd->nData1 >= nCount) {
				pWnd->nData1 = max (0, nCount - 1);
			}
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
			UpdateWindowITEMINFOList (pWnd);
		}
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHARITEM, nNo, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSVRINFO												 */
/* 内容：受信データ処理(サーバー情報)										 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void RecvDataProcSVRINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SVRINFO pPara = (PCMDPARA_SVRINFO)pData;

	if (g_hWndAdmin == NULL) {
		return;
	}
	PCMDPARA_SVRINFO pNew = new CMDPARA_SVRINFO;
	CopyMemory (pNew, pPara, sizeof (CMDPARA_SVRINFO));
	PostMessage (g_hWndAdmin, WM_ADMINSVRINFO, (WPARAM)pNew, 0);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPCHARINFO											 */
/* 内容：受信データ処理(マップ内キャラ数通知)								 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void RecvDataProcMAPCHARINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MAPCHARINFO pPara = (PCMDPARA_MAPCHARINFO)pData;

	g_dwCountPC = pPara->dwCountPC;
	g_dwCountNPC = pPara->dwCountNPC;
}

/* ========================================================================= */
/* 関数名：RecvDataProcNAMECOLOR											 */
/* 内容：受信データ処理(名前の色設定)										 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void RecvDataProcNAMECOLOR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_NAMECOLOR pPara = (PCMDPARA_NAMECOLOR)pData;
	PCHARINFO pChar;

	int nNo = GetCharNo (pPara->pID);

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	pChar->clName = pPara->clName;
	pChar->clSpeech = pPara->clSpeech;
}

/* ========================================================================= */
/* 関数名：RecvDataProcSHOP													 */
/* 内容：受信データ処理(店情報)												 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void RecvDataProcSHOP(PBYTE pData, DWORD dwDataSize)
{
	int i;
	PCMDPARA_SHOP pPara = (PCMDPARA_SHOP)pData;
	PBYTE pszMsg = pData + sizeof (CMDPARA_SHOP);

	if (g_byGameState < GAMESTATE_MOVE) {
		return;
	}

	/* 追加？ */
	if ((g_pMapInfo->byCountShop == 0) || (pPara->byID >= g_pMapInfo->byCountShop)) {
		/* 更新用のデータ作成 */
		PSHOPINFO pShopNew = new SHOPINFO;
		ZeroMemory (pShopNew, sizeof (SHOPINFO));

		pShopNew->byCountItem = pPara->byCountItem;
		CopyMemory (pShopNew->awItem, pPara->awItem, sizeof (pPara->awItem));
		pShopNew->dwItemCRC = g_crc.GetCRC ((PBYTE)pShopNew->awItem, sizeof (pShopNew->awItem));
		for (i = 0; i < MAX_SHOPMSG; i ++) {
			if (pPara->apMsg[i]) {
				pShopNew->apMsg[i] = new char[strlen ((LPSTR)pszMsg) + 1];
				ZeroMemory (pShopNew->apMsg[i], strlen ((LPSTR)pszMsg) + 1);
				strcat (pShopNew->apMsg[i], (LPSTR)pszMsg);
				pszMsg += (strlen ((LPSTR)pszMsg) + 1);
				pShopNew->dwMsgCRC[i] = g_crc.GetCRC ((PBYTE)pShopNew->apMsg[i], strlen ((LPSTR)pShopNew->apMsg[i]) + 1);
			}
		}
	
		g_pMapInfo->byCountShop ++;
		PSHOPINFO pShopInfoNew = new SHOPINFO[g_pMapInfo->byCountShop];
		ZeroMemory (pShopInfoNew, sizeof (SHOPINFO) * g_pMapInfo->byCountShop);
		CopyMemory (pShopInfoNew, g_pMapInfo->pShopInfo, sizeof (SHOPINFO) * (g_pMapInfo->byCountShop - 1));
		pShopInfoNew[g_pMapInfo->byCountShop - 1] = *pShopNew;

		if (g_pMapInfo->byCountShop == 2) {
			SAFE_DELETE (g_pMapInfo->pShopInfo);
		} else {
			SAFE_DELETE_ARRAY (g_pMapInfo->pShopInfo);
		}
		g_pMapInfo->pShopInfo = pShopInfoNew;

	/* 更新？ */
	} else {
		PSHOPINFO pShop = &g_pMapInfo->pShopInfo[pPara->byID];
		pShop->byCountItem = pPara->byCountItem;
		CopyMemory (pShop->awItem, pPara->awItem, sizeof (pPara->awItem));
		pShop->dwItemCRC = g_crc.GetCRC ((PBYTE)pShop->awItem, sizeof (pShop->awItem));
		for (int i = 0; i < MAX_SHOPMSG; i ++) {
			SAFE_DELETE_ARRAY (pShop->apMsg[i]);
			pShop->apMsg[i] = new char[strlen ((LPSTR)pszMsg) + 1];
			ZeroMemory (pShop->apMsg[i], strlen ((LPSTR)pszMsg) + 1);
			strcat (pShop->apMsg[i], (LPSTR)pszMsg);
			pszMsg += (strlen ((LPSTR)pszMsg) + 1);
			pShop->dwMsgCRC[i] = g_crc.GetCRC ((PBYTE)pShop->apMsg[i], strlen ((LPSTR)pShop->apMsg[i]) + 1);
		}
	}
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINSHOPINFO, (WPARAM)pPara->byID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcEFC													 */
/* 内容：受信データ処理(エフェクト情報更新)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void RecvDataProcEFC(BYTE *pData, DWORD dwDataSize)
{
	DWORD dwSizeTmp;
	PCMDPARA_CHGEFC pRecv = (PCMDPARA_CHGEFC)pData;
	PEFCINFO pEfcInfo, pEfcInfoTmp;

	if (g_byGameState < GAMESTATE_MOVE) {
		return;
	}

	pEfcInfoTmp	= g_EfcInfo.SetSendData ((PBYTE)(pRecv + 1), dwSizeTmp);
	pEfcInfo	= g_EfcInfo.GetPtr (pRecv->wData);
	/* 追加？ */
	if (pEfcInfo == NULL) {
		g_EfcInfo.Add (pEfcInfoTmp);

	/* 更新 */
	} else {
		g_EfcInfo.Renew (pRecv->wData, pEfcInfoTmp);
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINEFCINFO, (WPARAM)pRecv->wData, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcEFCINFO												 */
/* 内容：受信データ処理(エフェクト情報通知)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void RecvDataProcEFCINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_CHGEFC pPara = (PCMDPARA_CHGEFC)pData;

	g_EfcInfo.SetAllSendData ((PBYTE)(pPara + 1));

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINEFCINFO, (WPARAM)pPara->wData, 0);
	}
}


/* ========================================================================= */
/* 関数名：	RecvDataProcMAIN_CHAR											 */
/* 内容：	受信データ処理(キャラ系コマンド)								 */
/* 日付：	2006/07/26														 */
/* ========================================================================= */

void RecvDataProcMAIN_CHAR(PBYTE pData, DWORD dwDataSize)
{
	if (RecvDataProcTblCHAR[pData[1]]) {
		RecvDataProcTblCHAR[pData[1]](pData, dwDataSize);
	}
}


/* ========================================================================= */
/* 関数名：RecvDataProcCHARSTATUS											 */
/* 内容：受信データ処理(キャラステータス通知)								 */
/* 日付：2003/09/30															 */
/* ========================================================================= */
void RecvDataProcCHARSTATUS(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_CHARSTATUS pPara = (PCMDPARA_CHARSTATUS)pData;
	int nNo = GetCharNo (pPara->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	pChar->bySaveFlg		= pPara->bySaveFlg;
	pChar->byPW				= pPara->byPW;
	pChar->byDEX			= pPara->byDEX;
	pChar->byST				= pPara->byST;
	pChar->byIT				= pPara->byIT;
	pChar->byLK				= pPara->byLK;
	pChar->byNpcType		= pPara->byNpcType;
	pChar->byJOB			= pPara->byJOB;
	pChar->byLevel			= pPara->byLevel;
	pChar->chAttrFire		= pPara->chAttrFire;
	pChar->chAttrWater		= pPara->chAttrWater;
	pChar->chAttrWind		= pPara->chAttrWind;
	pChar->chAttrEarth		= pPara->chAttrEarth;
	pChar->chAttrLight		= pPara->chAttrLight;
	pChar->chAttrDark		= pPara->chAttrDark;
	pChar->wAT				= pPara->wAT;
	pChar->wDF				= pPara->wDF;
	pChar->wMaxHP			= pPara->wMaxHP;
	pChar->wHP				= pPara->wHP;
	pChar->wMaxMana			= pPara->wMaxMana;
	pChar->wMana			= pPara->wMana;
	pChar->wDFMagic			= pPara->wDFMagic;
	pChar->dwGold			= pPara->dwGold;
	pChar->dwExp			= pPara->dwExp;
	pChar->byDrunkPoint		= pPara->byDrunkPoint;
	pChar->byDrunkLV		= pPara->byDrunkLV;
	pChar->byDropProbability = pPara->byDropProbability;
	CopyMemory (pChar->waEqu, pPara->waEqu, sizeof (pChar->waEqu));
	CopyMemory (pChar->abyMagic, pPara->abyMagic, sizeof (pChar->abyMagic));
	RenewATDF (nNo);
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHARSTATUS, nNo, 0);
	}

	if (nNo == g_nMyCharID) {
		if (g_aCharInfo[g_nMyCharID]->wHP == 0) {
			int nNoCmd = GetWindowID (g_dwMsgIDCommand);
			if (nNoCmd >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[nNoCmd]);
				g_dwMsgIDCommand = 0;
				g_bDraw = TRUE;
			}
		}
		RenewUseMagicInfo ();
	}

	int nNoTmp = GetWindowID (g_dwMsgIDGold);
	if (nNoTmp >= 0) {
		PWNDINFO pWndTmp = (PWNDINFO)g_WndInfo[nNoTmp];
		pWndTmp->dwTimeDrawStart = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcTARGETCHAR											 */
/* 内容：受信データ処理(ターゲット要求)										 */
/* 日付：2004/11/21															 */
/* ========================================================================= */
void RecvDataProcTARGETCHAR(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	/* ターゲット指定？ */
	if (pPara->dwData != -1) {

	/* ターゲット解除 */
	} else {
		if (g_dwActiveMsgID == 0) {
			SAFE_DELETE (g_pTargetSelectInfo);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARSTATUSHP											 */
/* 内容：受信データ処理(キャラステータス(HPのみ)通知)						 */
/* 日付：2003/10/05															 */
/* ========================================================================= */
void RecvDataProcCHARSTATUSHP(BYTE *pData, DWORD dwDataSize)
{
	BYTE byCharStateBack;
	PCMDPARA_CHARSTATUSHP pPara = (PCMDPARA_CHARSTATUSHP)pData;
	int nNo = GetCharNo (pPara->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	byCharStateBack		= pChar->byCharState;
	pChar->byCharState	= pPara->byCharState;
	pChar->wHP			= pPara->wHP;
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHARSTATUS, nNo, 0);
	}

	if (nNo == g_nMyCharID) {
		if (byCharStateBack == CHARSTATE_DEAD) {
			/* 蘇生した？ */
			if (pChar->byCharState != CHARSTATE_DEAD) {
				AllEraseWindow ();
			}
		}
		if (g_aCharInfo[g_nMyCharID]->wHP == 0) {
			AllEraseWindow ();
		}
		/* 倒れた？(溜めていたターンゲージ・詠唱は取りやめる) */
		if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
			CancelTurnGage (pChar);
			if (pChar->dwTimeTurnMagicLV) {
				/* 詠唱エフェクトを消す */
				SendCmdEFECTCHG (0, TRUE);
			}
			pChar->dwTimeTurnMagicLV	= 0;
			pChar->byTurnMagicLV		= 0;
			SAFE_DELETE (g_pTargetSelectInfo);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARSTATUSGOLD										 */
/* 内容：受信データ処理(キャラステータス(所持金と経験値)通知)				 */
/* 日付：2003/11/19															 */
/* ========================================================================= */
void RecvDataProcCHARSTATUSGOLD(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA5 pPara = (PCMDPARA_DATA5)pData;
	int nNo = GetCharNo (pPara->pID);
	PCHARINFO pChar;

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	pChar->dwGold	= pPara->dwData1;
	pChar->dwExp	= pPara->dwData2;
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHARSTATUS, nNo, 0);
	}
	int nNoTmp = GetWindowID (g_dwMsgIDGold);
	if (nNoTmp >= 0) {
		PWNDINFO pWndTmp = (PWNDINFO)g_WndInfo[nNoTmp];
		pWndTmp->dwTimeDrawStart = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcTARGETCHARPC											 */
/* 内容：受信データ処理(ついていく通知)										 */
/* 日付：2003/10/07															 */
/* ========================================================================= */
void RecvDataProcTARGETCHARPC(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	int nNo = GetCharNo (pPara->pID);

	if (nNo < 0) {
		return;
	}

	if (nNo != g_nMyCharID) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[nNo];
	pChar->dwTargetTblID3 = pPara->dwData;
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDCHAREFC											 */
/* 内容：受信データ処理(文字エフェクト追加)									 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void RecvDataProcADDCHAREFC(BYTE *pData, DWORD dwDataSize)
{
	CMDPARA_ADDCHAREFC Para;
	PCMDPARA_ADDCHAREFC pPara = &Para;
	/* 文字列は可変長で届くので構造体に戻す */
	SboPackGetADDCHAREFC (&Para, pData, NULL);
	int nNo = GetCharNo (pPara->pID);

	if (nNo < 0) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[nNo];
	PCHAREFCINFO pCharEfc = NULL;
	int i;
	for (i = 0; i < MAX_COUNTCHAREFC; i ++) {
		pCharEfc = &pChar->aCharEfcInfo[i];
		if (pCharEfc->szMsg[0] == 0) {
			break;
		}
	}
	/* 空きがなかった？ */
	if (i >= MAX_COUNTCHAREFC) {
		/* 最初のを上書き */
		i = 0;
	}

	pCharEfc = &pChar->aCharEfcInfo[i];
	ZeroMemory (pCharEfc, sizeof (CHAREFCINFO));
	strcat (pCharEfc->szMsg, pPara->szMsg);
	pCharEfc->color = pPara->color;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGMAPCOLOR											 */
/* 内容：受信データ処理(マップ色変換)										 */
/* 日付：2003/10/18															 */
/* ========================================================================= */
void RecvDataProcCHGMAPCOLOR(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_CHGMAPCOLOR pPara = (PCMDPARA_CHGMAPCOLOR)pData;

	if (g_aCharInfo[g_nMyCharID]->dwMapID != pPara->dwMapID) {
		return;
	}

	g_pMapInfo->clChg = pPara->color;
	int i;

	delete g_pDibMapParts;
	g_pDibMapParts = new CImg32;
	g_pDibMapParts->CreateWithoutGdi (16 * 32, (g_wCountMapParts / 16 + 1) * 32);
	g_pDibMapParts->Clear ();

	for (i = 0; i < g_wCountMapParts; i ++) {
		PMAPPARTSINFO pParts = &g_pMapPartsInfo[i];
		int no, xx, yy;
		DWORD dwID;
		if (pParts->wBackID) {
			no = pParts->wBackID / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = pParts->wBackID % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy);
		}
		no = pParts->wGrpID / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = pParts->wGrpID % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = (dwID % g_byMapGrpCountX) * 32;
		yy = dwID / g_byMapGrpCountX * 32;
		if (pParts->byLevel) {
			g_pDibMapParts->BltAlpha ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, pParts->byLevel, TRUE);
		} else {
			g_pDibMapParts->Blt ((i % 16) * 32, i / 16 * 32, 32, 32, g_apDibMapBack[no], xx, yy, TRUE);
		}
	}
	if (g_pMapInfo->clChg) {
		g_pDibMapParts->ChgColor (0, 0, 0, 0, pPara->color);
	}

	for (i = 0; i < g_wCountMapPartsGrp; i ++) {
		g_apDibMap[i]->Blt (0, 0, 0, 0, g_apDibMapBack[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibMap[i]->ChgColor (0, 0, 0, 0, pPara->color);
		}
	}
	for (i = 0; i < g_wCountCharGrp; i ++) {
		g_apDibChar[i]->Blt (0, 0, 0, 0, g_apDibCharBack[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar[i]->ChgColor (0, 0, 0, 0, pPara->color);
		}
	}
	for (i = 0; i < g_wCountCharGrp1632; i ++) {
		g_apDibChar1632[i]->Blt (0, 0, 0, 0, g_apDibChar1632Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar1632[i]->ChgColor (0, 0, 0, 0, pPara->color);
		}
	}
	for (i = 0; i < g_wCountCharGrp3232; i ++) {
		g_apDibChar3232[i]->Blt (0, 0, 0, 0, g_apDibChar3232Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar3232[i]->ChgColor (0, 0, 0, 0, pPara->color);
		}
	}
	for (i = 0; i < g_wCountCharGrp4848; i ++) {
		g_apDibChar4848[i]->Blt (0, 0, 0, 0, g_apDibChar4848Back[i], 0, 0);
		if (g_pMapInfo->clChg) {
			g_apDibChar4848[i]->ChgColor (0, 0, 0, 0, pPara->color);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSTTMPLATE											 */
/* 内容：受信データ処理(ステータステンプレート情報)							 */
/* 日付：2003/10/25															 */
/* ========================================================================= */
void RecvDataProcSTTMPLATE(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int i;

	switch (pPara->byData) {
	case CMDSTTMPLATETYPE_ADD:	/* 追加 */
		{
			PSTATUSTEMPLATEINFO pInfo = new STATUSTEMPLATEINFO;
			ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
			SboPackGetSTATUSTEMPLATEINFO (pInfo, (PBYTE)(pPara + 1), NULL);
			g_aStTmpInfo.Add (pInfo);
		}
		break;

	case CMDSTTMPLATETYPE_CHG:	/* 変更 */
		{
			PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[pPara->dwTblID];
			SboPackGetSTATUSTEMPLATEINFO (pInfo, (PBYTE)(pPara + 1), NULL);
		}
		break;

	case CMDSTTMPLATETYPE_ALL:	/* 全て更新 */
		{
			for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
				SAFE_DELETE (g_aStTmpInfo[i]);
			}
			g_aStTmpInfo.RemoveAll ();

			PBYTE pTmp = (PBYTE)(pPara + 1);
			for (i = 0; i < (int)pPara->dwTblID; i ++) {
				PSTATUSTEMPLATEINFO pInfo = new STATUSTEMPLATEINFO;
				ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
				/* 名前は可変長で届くので構造体に戻す */
				pTmp = SboPackGetSTATUSTEMPLATEINFO (pInfo, pTmp, NULL);
				g_aStTmpInfo.Add (pInfo);
			}
		}
		break;
	}
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINSTTMPLATE, 0, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEITEMGET											 */
/* 内容：受信データ処理(預けてるアイテム一覧)								 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void RecvDataProcLEAVEITEMGET(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_LEAVEITEMGET pPara = (PCMDPARA_LEAVEITEMGET)pData;
	
	SAFE_DELETE (g_pLeaveInfo);
	g_pLeaveInfo = new LEAVEINFO;
	ZeroMemory (g_pLeaveInfo, sizeof (LEAVEINFO));
	g_pLeaveInfo->byCount = pPara->byCount;
	CopyMemory (g_pLeaveInfo->waItem, pPara->waItem, sizeof (g_pLeaveInfo->waItem));
	CopyMemory (g_pLeaveInfo->byItemCount, pPara->byItemCount, sizeof (g_pLeaveInfo->byItemCount));

	/* 作り手の画面を開いている間は、預かり所の画面ではなく材料の数え直しに使う */
	if (g_nProcMakerID >= 0) {
		g_bProcMakerLeaveOK = TRUE;
		g_bDraw = TRUE;
		return;
	}

	/* 並べ替え中に一覧が返ってきた：開いている窓をそのまま描き直す（カーソル・スクロール位置を保つ） */
	if (g_nProcShopState == LEAVESTATE_ITEMSORTSWAP) {
		int i;
		for (i = 0; i < g_WndInfo.GetSize (); i ++) {
			PWNDINFO pWndList = g_WndInfo[i];
			if (pWndList->nType != WNDTYPE_LEAVE) {
				continue;
			}
			pWndList->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
			g_bEnableMove = TRUE;
			UpdateWindowITEMINFOList (pWndList);
			return;
		}
	}

	if ((g_nProcShopState == LEAVESTATE_ITEMSELECTGET) || (g_nProcShopState == LEAVESTATE_ITEMSORTSWAP)) {

		PWNDINFO pWndTmp;
		if (g_pLeaveInfo->byCount == 0) {
			pWndTmp = MakeWindowLEAVE (4);
			g_WndInfo.Add (pWndTmp);
		} else {
			pWndTmp = MakeWindowLEAVE_ITEMGET ();
			g_WndInfo.Add (pWndTmp);
			/* 一覧の横にカーソルのアイテムの詳細を出す */
			MakeWindowITEMINFOList (pWndTmp);
		}
		g_dwActiveMsgID = pWndTmp->dwID;
		g_bEnableMove = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCOUNTSUBGAME											 */
/* 内容：受信データ処理(サブゲーム数通知)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void RecvDataProcCOUNTSUBGAME(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA2 pRecv = (PCMDPARA_DATA2)pData;

	g_nSubGameCount = pRecv->nData;

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINSUBGAMEINFO, 0, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAMEINFO											 */
/* 内容：受信データ処理(サブゲーム情報更新)									 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void RecvDataProcSUBGAMEINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;

	if (g_pSubGameInfo == NULL) {
		return;
	}

	SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);

	int nSize = GetSubGameDataSize (pRecv->byData);

	g_pSubGameInfo->byType = pRecv->byData;
	if (nSize) {
		g_pSubGameInfo->pInfo = new BYTE[nSize];
		ZeroMemory (g_pSubGameInfo->pInfo, nSize);

		switch (g_pSubGameInfo->byType) {
		case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
			{
				PSUBGAME_BOMBINFOSAVE pBombInfo = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
				PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)(pRecv + 1);
				pBombInfo->dwMapID = pBomb->dwMapID;
				pBombInfo->dwX = pBomb->dwX;
				pBombInfo->dwY = pBomb->dwY;
				pBombInfo->dwStartX = pBomb->dwStartX;
				pBombInfo->dwStartY = pBomb->dwStartY;
				pBombInfo->byX1 = pBomb->byX1;
				pBombInfo->byY1 = pBomb->byY1;
				pBombInfo->byX2 = pBomb->byX2;
				pBombInfo->byY2 = pBomb->byY2;
			}
			break;

		case SUBGAMEID_MAZE:		/* 迷路 */
			{
				PSUBGAME_MAZEINFOSAVE pMazeInfo = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
				PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)(pRecv + 1);
				pMazeInfo->dwMapID = pMaze->dwMapID;
				pMazeInfo->byX1 = pMaze->byX1;
				pMazeInfo->byY1 = pMaze->byY1;
				pMazeInfo->byX2 = pMaze->byX2;
				pMazeInfo->byY2 = pMaze->byY2;
				pMazeInfo->byCharX = pMaze->byCharX;
				pMazeInfo->byCharY = pMaze->byCharY;
				pMazeInfo->byStartX = pMaze->byStartX;
				pMazeInfo->byStartY = pMaze->byStartY;
				pMazeInfo->byGoalToX = pMaze->byGoalToX;
				pMazeInfo->byGoalToY = pMaze->byGoalToY;
				pMazeInfo->wChipIDStart = pMaze->wChipIDStart;
				pMazeInfo->wChipIDGoal = pMaze->wChipIDGoal;
				pMazeInfo->wChipIDRoad = pMaze->wChipIDRoad;
				pMazeInfo->wChipIDBlock = pMaze->wChipIDBlock;
			}		
			break;

		case SUBGAMEID_DUNGEON:		/* ダンジョン */
			{
				PSUBGAME_DUNGEONINFOSAVE pDungeonInfo = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
				PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)(pRecv + 1);
				pDungeonInfo->dwMapID = pDungeon->dwMapID;
				pDungeonInfo->byX1 = pDungeon->byX1;
				pDungeonInfo->byY1 = pDungeon->byY1;
				pDungeonInfo->byX2 = pDungeon->byX2;
				pDungeonInfo->byY2 = pDungeon->byY2;
				pDungeonInfo->byCharX = pDungeon->byCharX;
				pDungeonInfo->byCharY = pDungeon->byCharY;
				pDungeonInfo->byStartX = pDungeon->byStartX;
				pDungeonInfo->byStartY = pDungeon->byStartY;
				pDungeonInfo->byGoalX = pDungeon->byGoalX;
				pDungeonInfo->byGoalY = pDungeon->byGoalY;
				pDungeonInfo->wChipIDStart = pDungeon->wChipIDStart;
				pDungeonInfo->wChipIDGoal = pDungeon->wChipIDGoal;
				pDungeonInfo->wChipIDRoad = pDungeon->wChipIDRoad;
				pDungeonInfo->wChipIDBlock = pDungeon->wChipIDBlock;
			}		
			break;

		case SUBGAMEID_WAR:			/* 戦場ゲーム */
			{
				PSUBGAME_WARINFOSAVE pInfo = (PSUBGAME_WARINFOSAVE)g_pSubGameInfo->pInfo;
				PSUBGAME_WARINFOSAVE pInfoRecv = (PSUBGAME_WARINFOSAVE)(pRecv + 1);

				CopyMemory (pInfo, pInfoRecv, sizeof (SUBGAME_WARINFOSAVE));
			}
			break;

		case SUBGAMEID_SUGOROKU:	/* すごろく */
			{
				PSUBGAME_SUGOROKUINFOSAVE pInfo = (PSUBGAME_SUGOROKUINFOSAVE)g_pSubGameInfo->pInfo;
				PSUBGAME_SUGOROKUINFOSAVE pInfoRecv = (PSUBGAME_SUGOROKUINFOSAVE)(pRecv + 1);
				SUGOROKUEVENTINFO EventInfo;
				SUGOROKUPARTSINFO PartsInfo;
				PBYTE pTmp;
				DWORD i, dwCountTmp;

				pInfo->dwMapID = pInfoRecv->dwMapID;
				pInfo->byRule = pInfoRecv->byRule;
				pInfo->byX = pInfoRecv->byX;
				pInfo->byY = pInfoRecv->byY;
				pInfo->byMaxCount = pInfoRecv->byMaxCount;

				pTmp = (PBYTE)(pInfoRecv + 1);

				pInfo->aEventInfo.RemoveAll ();
				CopyMemory (&dwCountTmp, pTmp, sizeof (dwCountTmp));
				pTmp += sizeof (dwCountTmp);

				for (i = 0; i < dwCountTmp; i ++) {
					CopyMemory (&EventInfo, pTmp, sizeof (SUGOROKUEVENTINFO));
					pInfo->aEventInfo.Add (EventInfo);
					pTmp += sizeof (SUGOROKUEVENTINFO);
				}

				pInfo->aPartsInfo.RemoveAll ();
				CopyMemory (&dwCountTmp, pTmp, sizeof (dwCountTmp));
				pTmp += sizeof (dwCountTmp);

				for (i = 0; i < dwCountTmp; i ++) {
					CopyMemory (&PartsInfo, pTmp, sizeof (SUGOROKUPARTSINFO));
					pInfo->aPartsInfo.Add (PartsInfo);
					pTmp += sizeof (SUGOROKUPARTSINFO);
				}
			}
			break;
		}
	}
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINSUBGAMEINFO, 0, 0);
	}
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSUBGAMEINFOALL										 */
/* 内容：	受信データ処理(全サブゲーム情報更新)							 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

void RecvDataProcSUBGAMEINFOALL(BYTE *pData, DWORD dwDataSize)
{
	int i, nCount;
	BYTE byType;
	PBYTE pDataTmp;
	DWORD dwTmp;
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;
	PCInfoSubGameBase pInfo;

	/* 全てのサブゲーム情報を削除 */
	g_ProcSubGame.DeleteAll ();

	pDataTmp	= (PBYTE)(pRecv + 1);
	nCount		= pRecv->byData;

	for (i = 0; i < nCount; i ++) {
		CopyMemory (&byType, pDataTmp, sizeof (byType));
		pDataTmp += sizeof (byType);

		pInfo	= g_ProcSubGame.GetNew (byType);
		dwTmp	= pInfo->SetSendData (pDataTmp);
		pDataTmp += dwTmp;

		g_ProcSubGame.Add (pInfo);
	}
}


/* ========================================================================= */
/* 関数名：RecvDataProcSUBSTATUS											 */
/* 内容：受信データ処理(サブステータス情報)									 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void RecvDataProcSUBSTATUS(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;
	int nNo = GetCharNo (pRecv->pID);
	if (nNo < 0) {
		return;
	}
	PCHARINFO pChar = g_aCharInfo[nNo];
	CopyMemory (pChar->abySubStatus, pRecv + 1, sizeof (pChar->abySubStatus));
}

/* ========================================================================= */
/* 関数名：RecvDataProcTRADE												 */
/* 内容：受信データ処理(取り引きコマンド)									 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void RecvDataProcTRADE(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;
	int nNo = GetCharNo (pRecv->pID);
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	/* 傭兵など、番号でしか分からない相手 */
	if (nNo < 0) {
		nNo = GetCharNo2 (pRecv->dwTblID);
	}

	switch (pRecv->byData) {
	case CMDTRADE_IN:	/* 取り引き開始 */
		{
			/* 相手のキャラを知らない？ */
			if (nNo < 0) {
				/* 取り引き終了 */
				SendCmdTRADE (pRecv->dwTblID, CMDTRADE_OUT);
				break;
			}
			/* ウィンドウ表示中？ */
			if (g_dwActiveMsgID) {
				/* 取り引き終了 */
				SendCmdTRADE (pRecv->dwTblID, CMDTRADE_OUT);
				break;
			}
			/* チャット窓表示中？ */
			if (g_hWndSpeech) {
				/* 取り引き終了 */
				SendCmdTRADE (pRecv->dwTblID, CMDTRADE_OUT);
				break;
			}
			/* 取り引き中？ */
			if (g_pTradeInfo) {
				/* 取り引き終了 */
				SendCmdTRADE (pRecv->dwTblID, CMDTRADE_OUT);
				break;
			}
			/* 戦闘モード？ */
			if (pChar->byTurnLV) {
				/* 取り引き終了 */
				SendCmdTRADE (pRecv->dwTblID, CMDTRADE_OUT);
				break;
			}
			PWNDINFO pWndNew = MakeWindowTRADE (pRecv->dwTblID);
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;
			g_bDraw = TRUE;
		}
		break;

	case CMDTRADE_OUT:	/* 取り引き終了 */
		{
			g_bEnableMove = TRUE;
			if (g_pTradeInfo == NULL) {
				break;
			}

			/* 取り引きウィンドウを閉じる */
			int nNo = GetWindowID (g_dwMsgIDTRADE);
			if (nNo >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
				g_dwMsgIDCommand = 0;
				g_bDraw = TRUE;
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcTRADEINFO											 */
/* 内容：受信データ処理(取り引き情報)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void RecvDataProcTRADEINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_TRADEINFO pRecv = (PCMDPARA_TRADEINFO)pData;
	if (g_pTradeInfo == NULL) {
		return;
	}

	g_pTradeInfo->dwGold2 = pRecv->dwGold;
	g_pTradeInfo->byMode2 = pRecv->byMode;
	g_pTradeInfo->bySum2 = pRecv->bySum;
	CopyMemory (g_pTradeInfo->waItem2, pRecv->waItem, sizeof (pRecv->waItem));
	g_bDraw = TRUE;

	/* 両方決定済み？ */
	if ((g_pTradeInfo->byMode1 == 1) &&
		(g_pTradeInfo->byMode2 == 1)) {
		SendCmdTRADEPROC ();
		int nNo = GetWindowID (g_dwMsgIDTRADE);
		if (nNo >= 0) {
			/* 終了 */
			EraseWindow (g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
		}
		g_bEnableMove = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSYSMSG2												 */
/* 内容：受信データ処理(ID指定システムメッセージ)							 */
/* 日付：2003/11/24															 */
/* ========================================================================= */
void RecvDataProcSYSMSG2(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA6 pRecv = (PCMDPARA_DATA6)pData;
	char szTmp[1024];
	int nNo;
	PCHARINFO pChar, pChar2;
	PMAGICINFO pMagicInfo;

	switch (pRecv->pID) {
	case SYSMSG_ATACK1:
	case SYSMSG_ATACK2:
	case SYSMSG_BATTLEEND1:
	case SYSMSG_DEAD:
	case SYSMSG_NOPROC:
	case SYSMSG_HPMAX:
	case SYSMSG_RESURRECTION:
		nNo = GetCharNo (pRecv->dwData1);
		if (nNo < 0) {
			return;
		}
		pChar = g_aCharInfo[nNo];
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID], pChar->szName);
		break;

	case SYSMSG_DAMAGE1:
	case SYSMSG_BATTLEEND3:
	case SYSMSG_TARGET1:
	case SYSMSG_HPADD:
	case SYSMSG_GETEXP:
		nNo = GetCharNo (pRecv->dwData1);
		if (nNo < 0) {
			return;
		}
		pChar = g_aCharInfo[nNo];
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID], pChar->szName, pRecv->dwData2);
		break;

	case SYSMSG_BATTLEEND2:
		nNo = GetCharNo (pRecv->dwData1);
		if (nNo < 0) {
			return;
		}
		pChar = g_aCharInfo[nNo];
		nNo = GetCharNo (pRecv->dwData2);
		if (nNo < 0) {
			return;
		}
		pChar2 = g_aCharInfo[nNo];
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID], pChar->szName, pChar2->szName);
		break;

	case SYSMSG_GETITEM1:
	case SYSMSG_GETITEM3:
		nNo = GetCharNo (pRecv->dwData1);
		if (nNo < 0) {
			return;
		}
		pChar = g_aCharInfo[nNo];
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID], pChar->szName, g_aItemInfo[pRecv->dwData2]->szName);
		break;

	case SYSMSG_USESPECIALTY:
		nNo = GetCharNo (pRecv->dwData1);
		if (nNo < 0) {
			return;
		}
		pChar = g_aCharInfo[nNo];
		pMagicInfo = g_aMagicInfo[pRecv->dwData2];
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID], pChar->szName, pMagicInfo->szName);
		break;

	default:
		wsprintf (szTmp, aSysMsgTbl[pRecv->pID]);
		break;
	}
	AddSysMsg (szTmp, RGB (255, 255, 255));
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEGOLDGET											 */
/* 内容：受信データ処理(預けてる金額)										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void RecvDataProcLEAVEGOLDGET(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	
	SAFE_DELETE (g_pLeaveInfo);
	g_pLeaveInfo = new LEAVEINFO;
	ZeroMemory (g_pLeaveInfo, sizeof (LEAVEINFO));
	g_pLeaveInfo->dwGold = pPara->dwData;

	if (g_nProcShopState == LEAVESTATE_GOLDGET) {
		PWNDINFO pWndTmp = NULL;
		if (g_pLeaveInfo->dwGold == 0) {
			pWndTmp = MakeWindowLEAVE (6);
		} else {
			pWndTmp = MakeWindowGOLD ();
			g_WndInfo.Add (pWndTmp);
			pWndTmp = MakeWindowLEAVE_GOLD (1);
		}
		if (pWndTmp) {
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bEnableMove = TRUE;
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEINFO											 */
/* 内容：受信データ処理(預かり所情報)										 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void RecvDataProcLEAVEINFO(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_LEAVEINFO pPara = (PCMDPARA_LEAVEINFO)pData;

	SAFE_DELETE (g_pLeaveInfo);
	g_pLeaveInfo = new LEAVEINFO;
	ZeroMemory (g_pLeaveInfo, sizeof (LEAVEINFO));
	g_pLeaveInfo->byCount = pPara->byCount;
	g_pLeaveInfo->dwGold = pPara->dwGold;
	CopyMemory (g_pLeaveInfo->waItem, pPara->waItem, sizeof (g_pLeaveInfo->waItem));
	CopyMemory (g_pLeaveInfo->byItemCount, pPara->byItemCount, sizeof (g_pLeaveInfo->byItemCount));
	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINLEAVEINFO, 0, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcACCSEARCH											 */
/* 内容：受信データ処理(アカウント検索)										 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void RecvDataProcACCSEARCH(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINACCSEARCH, (WPARAM)pPara->dwTblID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINWINDOW											 */
/* 内容：受信データ処理(管理者ウィンドウ表示)								 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void RecvDataProcADMINWINDOW(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (pPara->byData == 1) {
		if (g_aCharInfo[g_nMyCharID]->byAdminLV == ADMINLV_ADMINISTRATOR) {
			MakeAdminWindow ();
			/* MFC 版の「管理者ウィンドウ」(CAdminWindow) は実装途中のため出さない。
			 * 使うのは従来の「管理者用ウィンドウ」だけ */
		}
	} else {
		SAFE_DESTROYWND (g_hWndAdmin);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcPARTY												 */
/* 内容：受信データ処理(仲間コマンド)										 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void RecvDataProcPARTY(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_PARTYCMD pPara = (PCMDPARA_PARTYCMD)pData;
	PCHARINFO pCharFrom = NULL;
	int i, nNo, nCount;

	nNo = GetCharNo2 (pPara->dwTblIDFrom);
	if (nNo >= 0) {
		pCharFrom = g_aCharInfo[nNo];
	}

	/* 各コマンド処理 */
	switch (pPara->byCmdSub) {
	case CMDPARTY_JOIN:			/* 誘う */
		{
			if (pCharFrom == NULL) {
				SendCMDPARTY (CMDPARTY_JOINNO, pPara->dwTblIDFrom, 0);
			}
			/* ウィンドウ表示中？ */
			if (g_WndInfo.GetSize ()) {
				SendCMDPARTY (CMDPARTY_JOINCANCEL, pPara->dwTblIDFrom, 0);
				break;
			}

			char szTmp[128];
			PWNDINFO pWndTmp = NULL, pWndTmp2 = NULL;
			/* 確認ウィンドウ作成 */
			pWndTmp = MakeWindowCONF (16 * 20, 16 * 10, IDCONFIRMTYPE_PARTYJOIN);
			pWndTmp->nData1 = 1;
			pWndTmp->nData2 = (int)pPara->dwTblIDFrom;
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;

			wsprintf (szTmp, "%sのパーティに参加しますか？@2", pCharFrom->szName);
			pWndTmp2 = MakeWindowMSG (szTmp, FALSE);
			pWndTmp2->pParent = pWndTmp;
			pWndTmp->pChild = pWndTmp2;
			g_WndInfo.Add (pWndTmp2);
		}
		break;

	case CMDPARTY_JOINOK:		/* 参加する */
		if (g_aPartyInfo.GetSize () == 0) {
			g_aPartyInfo.Add (g_aCharInfo[g_nMyCharID]->dwTblID);
		}
		nCount = g_aPartyInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			/* すでに参加している？ */
			if (g_aPartyInfo[i] == pPara->dwTblIDFrom) {
				break;
			}
		}
		if (i >= nCount) {
			g_aPartyInfo.Add (pPara->dwTblIDFrom);
		}
		break;

	case CMDPARTY_JOINNO:		/* 参加しない */
		break;

	case CMDPARTY_BREAKUP:		/* 解散 */
		g_aPartyInfo.RemoveAll ();
		break;

	case CMDPARTY_BREAK:		/* 抜ける */
		nCount = g_aPartyInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			/* 参加している？ */
			if (g_aPartyInfo[i] == pPara->dwTblIDFrom) {
				g_aPartyInfo.RemoveAt (i);
				if (g_aPartyInfo.GetSize () == 1) {
					g_aPartyInfo.RemoveAll ();
				}
				break;
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARSTATUSHPMANA										 */
/* 内容：受信データ処理(キャラステータス(HPとMANA)通知)						 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void RecvDataProcCHARSTATUSHPMANA(BYTE *pData, DWORD dwDataSize)
{
	PCHARINFO pChar;
	PCMDPARA_CHARSTATUSHPMANA pPara = (PCMDPARA_CHARSTATUSHPMANA)pData;
	int nNo = GetCharNo (pPara->pID);

	if (nNo < 0) {
		return;
	}

	pChar = g_aCharInfo[nNo];
	pChar->byCharState	= pPara->byCharState;
	pChar->wHP			= pPara->wHP;
	pChar->wMaxHP		= pPara->wMaxHP;
	pChar->wMana		= pPara->wMana;
	pChar->wMaxMana		= pPara->wMaxMana;

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINCHARSTATUS, nNo, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcJOBINFO												 */
/* 内容：受信データ処理(職業情報)											 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void RecvDataProcJOBINFO(BYTE *pData, DWORD dwDataSize)
{
	DWORD dwSize;
	PCMDPARA_JOBINFO pPara = (PCMDPARA_JOBINFO)pData;
	PJOBINFO pInfo, pInfoTmp;

	pInfo = g_JobInfo.GetPtr ((int)pPara->byID);
	/* 知らない情報？ */
	if (pInfo == NULL) {
		/* 情報を追加 */
		pInfo = g_JobInfo.GetNew ();
		g_JobInfo.Add (pInfo);
	}

	dwSize	 = 0;
	pInfoTmp = g_JobInfo.SetSendData ((PBYTE)(pPara + 1), dwSize);
	g_JobInfo.Copy (pInfo, pInfoTmp);
	g_JobInfo.Delete (pInfoTmp);

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINJOBINFO, (WPARAM)pPara->byID, 0);
	}
	if (g_pAdminWindow) {
		g_pAdminWindow->PostMessage (WM_ADMINJOBINFO, (WPARAM)pPara->byID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICINFO											 */
/* 内容：受信データ処理(魔法情報)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void RecvDataProcMAGICINFO(BYTE *pData, DWORD dwDataSize)
{
	CMDPARA_MAGICINFO Para;
	PCMDPARA_MAGICINFO pPara = &Para;
	PMAGICINFO pInfo;
	int nCount;

	/* 名前と説明文は可変長で届くので構造体に戻す */
	if (SboPackGetMAGICINFO (&Para, pData, NULL) == NULL) {
		return;
	}

	nCount = g_aMagicInfo.GetSize ();
	/* 知っている情報よりも大きいID？ */
	if (pPara->byID >= nCount) {
		/* 情報を追加 */
		pInfo = new MAGICINFO;
		ZeroMemory (pInfo, sizeof (MAGICINFO));
		g_aMagicInfo.Add (pInfo);

	} else {
		pInfo = g_aMagicInfo[pPara->byID];
	}

	SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pPara->szName);
	pInfo->byType = pPara->byType;
	pInfo->byTypeInfoID = pPara->byTypeInfoID;
	pInfo->byUserInfo = pPara->byUserInfo;
	pInfo->byTarget = pPara->byTarget;
	pInfo->byTargetType = pPara->byTargetType;
	pInfo->byTargetArea = pPara->byTargetArea;
	pInfo->byTargetCount = pPara->byTargetCount;
	pInfo->byTargetEfectID = pPara->byTargetEfectID;
	pInfo->byUserEfectID = pPara->byUserEfectID;
	pInfo->byTime = pPara->byTime;
	pInfo->byAttr = pPara->byAttr;
	pInfo->wData1 = pPara->wData1;
	pInfo->wData2 = pPara->wData2;
	pInfo->wData3 = pPara->wData3;
	pInfo->wData4 = pPara->wData4;
	pInfo->wMana = pPara->wMana;
	/* 説明文（空なら数値から自動の説明を出す） */
	SboTextCopy (pInfo->szExplain, sizeof (pInfo->szExplain), pPara->szExplain);

	RenewUseMagicInfo ();

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINMAGICINFO, (WPARAM)pPara->byID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICUSERINFO										 */
/* 内容：受信データ処理(魔法利用者情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void RecvDataProcMAGICUSERINFO(BYTE *pData, DWORD dwDataSize)
{
	CMDPARA_MAGICUSERINFO Para;
	PCMDPARA_MAGICUSERINFO pPara = &Para;
	PMAGICUSERINFO pInfo;
	int nCount;
	PBYTE pDataNext;

	/* 名前は可変長で届くので構造体に戻す（後ろに職業 ID の表が続く） */
	pDataNext = SboPackGetMAGICUSERINFO (&Para, pData, NULL);
	if (pDataNext == NULL) {
		return;
	}

	nCount = g_aMagicUserInfo.GetSize ();
	/* 知っている情報よりも大きいID？ */
	if (pPara->byID >= nCount) {
		/* 情報を追加 */
		pInfo = new MAGICUSERINFO;
		ZeroMemory (pInfo, sizeof (MAGICUSERINFO));

		g_aMagicUserInfo.Add (pInfo);

	} else {
		pInfo = g_aMagicUserInfo[pPara->byID];
	}

	SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pPara->szName);

	SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
	if (pPara->byCount) {
		pInfo->byCount = pPara->byCount;
		pInfo->paJobIDTbl = new BYTE[pPara->byCount];
		ZeroMemory (pInfo->paJobIDTbl, pPara->byCount);
		CopyMemory (pInfo->paJobIDTbl, pDataNext, pPara->byCount);
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINMAGICUSERINFO, (WPARAM)pPara->byID, 0);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcEQUUSERINFO											 */
/* 内容：受信データ処理(装備職業情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void RecvDataProcEQUUSERINFO(BYTE *pData, DWORD dwDataSize)
{
	CMDPARA_EQUUSERINFO Para;
	PCMDPARA_EQUUSERINFO pPara = &Para;
	PEQUUSERINFO pInfo;
	int nCount;
	PBYTE pDataNext;

	/* 名前は可変長で届くので構造体に戻す（後ろに職業 ID の表が続く） */
	pDataNext = SboPackGetEQUUSERINFO (&Para, pData, NULL);
	if (pDataNext == NULL) {
		return;
	}

	nCount = g_aEquUserInfo.GetSize ();
	/* 知っている情報よりも大きいID？ */
	if (pPara->byID >= nCount) {
		/* 情報を追加 */
		pInfo = new EQUUSERINFO;
		ZeroMemory (pInfo, sizeof (EQUUSERINFO));

		g_aEquUserInfo.Add (pInfo);

	} else {
		pInfo = g_aEquUserInfo[pPara->byID];
	}

	SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pPara->szName);

	SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
	if (pPara->byCount) {
		pInfo->byCount = pPara->byCount;
		pInfo->paJobIDTbl = new BYTE[pPara->byCount];
		ZeroMemory (pInfo->paJobIDTbl, pPara->byCount);
		CopyMemory (pInfo->paJobIDTbl, pDataNext, pPara->byCount);
	}

	if (g_hWndAdmin) {
		PostMessage (g_hWndAdmin, WM_ADMINEQUUSERINFO, (WPARAM)pPara->byID, 0);
	}
}


/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAME												 */
/* 内容：受信データ処理(サブゲーム用コマンド)								 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void RecvDataProcSUBGAME(BYTE *pData, DWORD dwDataSize)
{
	switch (pData[1]) {
	case SUBGAMEID_BOMB:	/* 爆弾ゲーム */
		if (RecvDataProcSubGameTblBomb[pData[2]]) {
			RecvDataProcSubGameTblBomb[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_MAZE:	/* 迷路 */
		if (RecvDataProcSubGameTblMaze[pData[2]]) {
			RecvDataProcSubGameTblMaze[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_WAR:		/* 戦場ゲーム */
		if (RecvDataProcSubGameTblWar[pData[2]]) {
			RecvDataProcSubGameTblWar[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_SUGOROKU:	/* すごろく */
		if (RecvDataProcSubGameTblSugoroku[pData[2]]) {
			RecvDataProcSubGameTblSugoroku[pData[2]](pData, dwDataSize);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_TIME										 */
/* 内容：受信データ処理(残り時間)											 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_TIME(BYTE *pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	PSUBGAMECLIENT_BOMBINFO pInfo;

	/* 爆弾ゲームに参加中？ */
	if (pChar->abySubStatus[0] != SUBGAMEID_BOMB) {
		return;
	}

	/* 情報が無い？ */
	if (g_pSubGameInfoPlay == NULL) {
		g_pSubGameInfoPlay = new SUBGAMEINFO;
		ZeroMemory (g_pSubGameInfoPlay, sizeof (SUBGAMEINFO));

		int nSize = sizeof (SUBGAMECLIENT_BOMBINFO);
		g_pSubGameInfoPlay->pInfo = new BYTE[nSize];
		ZeroMemory (g_pSubGameInfoPlay->pInfo, nSize);
		g_pSubGameInfoPlay->byType = SUBGAMEID_BOMB;
		pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;
		pInfo->dwTblIDGameMaster = -1;
		pInfo->byMaxBomb = 1;
		pInfo->byMaxFire = 3;

		if (GetWindowID (g_dwMsgIDSubGame) < 0) {
			g_WndInfo.Add (MakeWindowSUBGAME_BOMBINFO ());
		}

	} else {
		pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;
	}

	pInfo->byGameTimeNow = pRecv->byData;
	int i = GetWindowID (g_dwMsgIDSubGame);
	if (i >= 0) {
		g_WndInfo[i]->dwTimeDrawStart = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_INFO										 */
/* 内容：受信データ処理(設定)												 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_INFO(BYTE *pData, DWORD dwDataSize)
{
	PSUBCMDPARA_BOMBINFO pRecv = (PSUBCMDPARA_BOMBINFO)pData;
	PSUBGAMECLIENT_BOMBINFO pBomb;

	/* 情報が無い？ */
	if (g_pSubGameInfoPlay == NULL) {
		g_pSubGameInfoPlay = new SUBGAMEINFO;
		ZeroMemory (g_pSubGameInfoPlay, sizeof (SUBGAMEINFO));

		int nSize = sizeof (SUBGAMECLIENT_BOMBINFO);
		g_pSubGameInfoPlay->pInfo = new BYTE[nSize];
		ZeroMemory (g_pSubGameInfoPlay->pInfo, nSize);
		g_pSubGameInfoPlay->byType = SUBGAMEID_BOMB;
	}
	pBomb = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;
	pBomb->byGameTime = pRecv->byGameTime;
	pBomb->byMaxBomb = pRecv->byMaxBomb;
	pBomb->byMaxFire = pRecv->byMaxFire;
	pBomb->byFlgGameNow = pRecv->byFlgGameNow;
	pBomb->byFlgUseBlock = pRecv->byFlgUseBlock;
	pBomb->byFlgUseItem = pRecv->byFlgUseItem;
	pBomb->byFlgProgress = pRecv->byFlgProgress;
	pBomb->dwTblIDGameMaster = pRecv->dwTblIDGameMaster;
	/* マスターの名前（古いサーバーからは届かないので大きさを見る） */
	ZeroMemory (pBomb->szGameMasterName, sizeof (pBomb->szGameMasterName));
	if ((pRecv->dwTblIDGameMaster != (DWORD)-1) && (dwDataSize >= sizeof (SUBCMDPARA_BOMBINFO))) {
		strncpy (pBomb->szGameMasterName, pRecv->szGameMasterName, MAX_CHARNAMEBUF - 1);
	}
	int i = GetWindowID (g_dwMsgIDSubGame);
	if (i >= 0) {
		g_WndInfo[i]->dwTimeDrawStart = 0;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_STOP										 */
/* 内容：受信データ処理(試合中止)											 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_STOP(BYTE *pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	int nNo;

	nNo = GetWindowID (g_dwMsgIDSubGame);
	if (nNo >= 0) {
		EraseWindow (g_WndInfo[nNo]);
	}
	nNo = GetWindowID (g_dwMsgIDCommand);
	if (nNo >= 0) {
		EraseWindow (g_WndInfo[nNo]);
	}
	if (g_pSubGameInfoPlay) {
		SAFE_DELETE_ARRAY (g_pSubGameInfoPlay->pInfo);
	}
	SAFE_DELETE (g_pSubGameInfoPlay);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_SETBOMB									 */
/* 内容：受信データ処理(爆弾設置)											 */
/* 日付：2004/04/11															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_SETBOMB(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_BOMBSETBOMB pRecv = (PCMDPARA_BOMBSETBOMB)pData;
	PCHARINFO pCharDst, pMyChar = g_aCharInfo[g_nMyCharID];

	pCharDst = new CHARINFO;
	ZeroMemory ((PBYTE)pCharDst, sizeof (CHARINFO));
	pCharDst->dwTargetTblID3 = -1;

	pCharDst->pID = pRecv->pID;
	pCharDst->dwTblID = pRecv->dwTblID;
	pCharDst->byNpcType = pRecv->byNpcType;
	pCharDst->dwMapID = pMyChar->dwMapID;
	pCharDst->dwX = pRecv->dwX;
	pCharDst->dwY = pRecv->dwY;
	pCharDst->byGameState = GAMESTATE_NPC;
	g_aCharInfo.Add (pCharDst);

	RenewCharPosInfo ();
	if (g_pMapInfo->pdwMapChar) {
		if ((pCharDst->dwX < (DWORD)g_pMapInfo->nCX) && (pCharDst->dwY < (DWORD)g_pMapInfo->nCY)) {
			g_pMapInfo->pdwMapChar[g_pMapInfo->nCX * pCharDst->dwY + pCharDst->dwX] ++;
		}
	}
}



/* ========================================================================= */
/* 関数名：RecvDataProcSubGameWAR_CHGSTATE									 */
/* 内容：受信データ処理(戦場ゲーム：状態遷移)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void RecvDataProcSubGameWAR_CHGSTATE(BYTE *pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv;
	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;

	pRecv = (PSUBCMDPARA_DATA1)pData;
	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	pSubStatus->byState = pRecv->byData;

	switch (pSubStatus->byState) {
	case SUBGAME_WAR_STATE_INIT:			/* 初期化 */
		AllEraseWindow ();
		ZeroMemory (pSubStatus, sizeof (SUBSTATUS_WAR));
		SAFE_DELETE (g_pWarInfo);
		SAFE_DELETE (g_pWarMagicTarget);
		break;

	case SUBGAME_WAR_STATE_TURN:			/* 行動選択 */
		SAFE_DELETE (g_pWarInfo);
		SAFE_DELETE (g_pWarMagicTarget);
		g_pWarInfo = new SUBGAMECLIENT_WARINFO;
		ZeroMemory (g_pWarInfo, sizeof (SUBGAMECLIENT_WARINFO));
		g_pWarInfo->byMove = 2;
		break;

	case SUBGAME_WAR_STATE_TURNCONTINUE:	/* 行動選択続行 */
		pSubStatus->byState = SUBGAME_WAR_STATE_TURN;
		break;

	case SUBGAME_WAR_STATE_SELECTDIFENCE:	/* 防御行動選択 */
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameWAR_SELECTTURN								 */
/* 内容：受信データ処理(戦場ゲーム：順番選択)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void RecvDataProcSubGameWAR_SELECTTURN(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_WARSELECTTURN pRecv;
	int i, nCount;
	PWNDINFO pWnd;

	pRecv = (PCMDPARA_WARSELECTTURN)pData;

	SAFE_DELETE (g_pWarSelectTurn);
	g_pWarSelectTurn = new CMDPARA_WARSELECTTURN;
	ZeroMemory (g_pWarSelectTurn, sizeof (CMDPARA_WARSELECTTURN));
	/* 名前は可変長で届くので構造体に戻す */
	SboPackGetWARSELECTTURN (g_pWarSelectTurn, (PBYTE)pRecv, NULL);

	nCount = g_WndInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pWnd = (PWNDINFO)g_WndInfo[i];
		if (pWnd->nType == WNDTYPE_SUBWAR_SELECTTURN) {
			pWnd->dwTimeDrawStart = 0;
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameWAR_MAGIC_TARGET								 */
/* 内容：受信データ処理(戦場ゲーム：魔法対象者一覧)							 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void RecvDataProcSubGameWAR_MAGIC_TARGET(BYTE *pData, DWORD dwDataSize)
{
	PCMDPARA_WARSELECTMAGICTARGET pRecv;
	PWNDINFO pWnd, pWndTmp, pWndNew;
	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;

	pRecv = (PCMDPARA_WARSELECTMAGICTARGET)pData;
	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	SAFE_DELETE (g_pWarMagicTarget);
	g_pWarMagicTarget = new CMDPARA_WARSELECTMAGICTARGET;
	ZeroMemory (g_pWarMagicTarget, sizeof (CMDPARA_WARSELECTMAGICTARGET));
	/* 名前は可変長で届くので構造体に戻す */
	SboPackGetWARSELECTMAGICTARGET (g_pWarMagicTarget, (PBYTE)pRecv, NULL);

	pWnd = (PWNDINFO)g_WndInfo[GetWindowID (g_dwActiveMsgID)];
	pWndTmp = (PWNDINFO)pWnd->pChild;
	EraseWindow (pWndTmp);

	pWndNew = MakeWindowSUBWAR_TURN_MAGIC_TARGET ();
	pWndNew->pParent = pWnd;
	pWnd->pChild = pWndNew;
	g_WndInfo.Add (pWndNew);
	g_dwActiveMsgID = pWndNew->dwID;

	pSubStatus->byState = SUBGAME_WAR_STATE_TURN;
}


/* ========================================================================= */
/* 関数名：RecvDataProcSubGameSUGOROKU_CHGSTATE								 */
/* 内容：受信データ処理(すごろく：状態遷移)									 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void RecvDataProcSubGameSUGOROKU_CHGSTATE(BYTE *pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv;
	PCHARINFO pChar;
	PSUBSTATUS_SUGOROKU pSubStatus;

	pRecv = (PSUBCMDPARA_DATA1)pData;
	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	pSubStatus->byState = pRecv->byData;

	switch (pSubStatus->byState) {
	case SUBGAME_SUGOROKU_STATE_INIT:		/* 初期化 */
		ZeroMemory (pSubStatus, sizeof (SUBSTATUS_SUGOROKU));
		break;

	case SUBGAME_SUGOROKU_STATE_COMMAND:	/* 行動選択 */
		break;
	}
}


/* ========================================================================= */
/* 関数名：	RecvDataProcMAIN_CHAR_STATEINFO									 */
/* 内容：	受信データ処理(キャラ系:状態通知)								 */
/* 日付：	2006/07/26														 */
/* ========================================================================= */

void RecvDataProcMAIN_CHAR_STATEINFO(PBYTE pData, DWORD dwDataSize)
{
	CCHAR_STATEINFO Packet;

	Packet.Set (pData);
}


/* ========================================================================= */
/* 関数名：	RecvDataProcMAIN_CHAR_TAIL										 */
/* 内容：	受信データ処理(キャラ系:ついていく要求)							 */
/* 日付：	2006/08/06														 */
/* ========================================================================= */

void RecvDataProcMAIN_CHAR_TAIL(PBYTE pData, DWORD dwDataSize)
{
	char szTmp[256];
	int nNo;
	PCHARINFO pChar, pCharTarget;
	CCHAR_TAIL Packet;

	ZeroMemory (szTmp, sizeof (szTmp));

	pChar		= g_aCharInfo[g_nMyCharID];
	pCharTarget	= NULL;

	Packet.Set (pData);

	if (Packet.m_dwTailTblID != -1) {
		nNo = GetCharNo2 (Packet.m_dwTailTblID);
		if (nNo != -1) {
			pCharTarget = g_aCharInfo[nNo];
		}
	}

	switch (Packet.m_dwResult) {
	case CHAR_TAIL_BACK:		/* ○○がついてきます */
		if (pCharTarget == NULL) {
			break;
		}
		/* つなぎ直し（傭兵が列を抜けた等）の時はメッセージを出さない */
		if (pChar->dwTailBackTblID == -1) {
			wsprintf (szTmp, "%sがついてきます。", pCharTarget->szName);
		}
		pChar->dwTailBackTblID = pCharTarget->dwTblID;
		break;
	case CHAR_TAIL_ON:			/* ○○についていきます */
		if (pCharTarget == NULL) {
			break;
		}
		/* つなぎ直し（前の傭兵が列を抜けた等）の時はメッセージを出さない。
		   自分から Z でついていった時は、送る時に「とりあえずついていっている」
		   ことにして相手の番号を入れているので、同じ番号ならメッセージを出す */
		if ((pChar->dwTailTblID == -1) || (pChar->dwTailTblID == pCharTarget->dwTblID)) {
			wsprintf (szTmp, "%sについていきます。", pCharTarget->szName);
		}
		pChar->dwTailTblID = pCharTarget->dwTblID;
		break;
	case CHAR_TAIL_OFF:			/* ついていくのを解除しました */
		pChar->dwTailTblID = -1;
		wsprintf (szTmp, "ついていくのを解除しました。");
		break;
	case CHAR_TAIL_OFF2:		/* ついていくのが解除されました */
		pChar->dwTailTblID = -1;
		wsprintf (szTmp, "ついていくのが解除されました。");
		break;
	case CHAR_TAIL_NG:			/* ○○は最後尾ではありません */
		pChar->dwTailTblID = -1;
		if (pCharTarget == NULL) {
			break;
		}
		wsprintf (szTmp, "%sは最後尾ではありません。", pCharTarget->szName);
		break;
	case CHAR_TAIL_NG2:			/* ○○にはついていけません */
		pChar->dwTailTblID = -1;
		if (pCharTarget == NULL) {
			break;
		}
		wsprintf (szTmp, "%sにはついていけません。", pCharTarget->szName);
		break;
	}

	if (strlen (szTmp) > 0) {
		AddSysMsg (szTmp, RGB (255, 255, 255));
	}
}

/* Copyright(C)URARA-works 2003-2006 */
