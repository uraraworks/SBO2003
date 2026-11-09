/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			コマンド受信処理										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "CharProc/CharProcBase.h"
#include "CharProc/CharProcPUT.h"
#include "../Common/Info/InfoJob.h"
#include "../Common/Info/SubGame/InfoSubGameWAR.h"
#include "../Common/Command/Command.h"
#include "Proc/ProcSubGame.h"
#include "Proc/SubGame/ProcSubGameWAR.h"
#include "ProcFunc/ProcFunc.h"
#include "SboSvrItemProc.h"
#include "SboSvrBattleProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrSubGameProcSugoroku.h"
#include "SboSvrAdminUndo.h"
#include "SboSvrNpcRoute.h"
#include "CharProc/CharProcMERCENARY.h"
#include "SboSvrMagicProc.h"
#include <map>
#include <vector>

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef void (*RECVDATAPROC)(PBYTE, DWORD);

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

void	MyOutputDebugString			(LPCTSTR lpszFormat, ...);						/* デバッグメッセージ出力 */
BOOL	CheckAdminBan				(DWORD);										/* 管理者権限かチェックして、違ってたらＢＡＮ */
PCHARINFO AddChar					(PCHARINFO, BOOL bCheck=TRUE);					/* キャラ追加 */
void	CharNpcIdEnsure				(PCHARINFO);									/* NPC番号がまだ無ければ振る(SboSvrSaveLoadChar.cpp) */
void	DeleteChar					(DWORD);										/* キャラ削除 */
int		GetCharNo					(DWORD);										/* キャラ番号取得 */
int		GetMapNo					(DWORD);										/* マップ番号取得 */
int		GetEventDataSize			(BYTE);											/* イベント種別毎のデータサイズを取得 */
int		GetSubGameDataSize			(BYTE);											/* サブゲーム種別毎のデータサイズを取得 */
int		GetItemDataSize				(BYTE);											/* アイテム種別毎のデータサイズを取得 */
int		GetItemCount				(DWORD);										/* アイテム数を取得 */
int		GetSubGameID				(PSUBGAMEINFO);									/* サブゲームIDを取得 */
int		SearchEvent					(DWORD, DWORD, DWORD);							/* 座標を元にイベント番号検索 */
void	AddCharItem					(DWORD, DWORD);									/* アイテム追加 */
void	DelCharItem					(DWORD, BYTE);									/* アイテム捨てる */
BOOL	AddLeaveItem				(PCHARINFO, WORD);								/* 預かり所にアイテムを1個入れる */
BOOL	CheckCharItem				(DWORD, DWORD, BYTE);							/* アイテム持ってるかチェック */
void	RenewDataPos				(DWORD, int, int);								/* 各データ位置更新 */
void	AddTblNoPC					(int);											/* PC番号テーブルに追加 */
void	DeleteTblNoPC				(int);											/* PC番号テーブルから削除 */
void	RenewTblNoChar				(void);											/* PC番号テーブルを更新 */
void	DeleteChildChar				(PCHARINFO);									/* 自分が親のキャラを削除 */
void	SetInitCharData				(PCHARINFO);									/* キャラの初期値を設定 */
void	CharTargetPC				(PCHARINFO, PCHARINFO, DWORD);					/* キャラのついてく設定 */
void	CharTargetCancelPC			(PCHARINFO);									/* キャラのついていく解除 */
void	TailMoveChain				(PCHARINFO);									/* ついてきているキャラを順に動かす */
BOOL	CheckCharX					(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget);	/* キャラサイズに合わせた当たり判定(横) */
int		GetCharX					(PCHARINFO, DWORD, DWORD, BOOL);				/* キャラサイズに合わせた距離を求める(横) */
int		GetCharDistanceX			(PCHARINFO, PCHARINFO);							/* キャラサイズに合わせた距離を求める(横) */
DWORD	GetCharXPos					(PCHARINFO, DWORD, BYTE);						/* キャラサイズに合わせた座標を補正(横) */
BOOL	CheckCharBlock				(DWORD, DWORD, DWORD);							/* キャラのぶつかり判定 */
BOOL	CheckCharBlock				(PCHARINFO pChar, DWORD, DWORD, DWORD);			/* キャラのぶつかり判定 */
DWORD	CheckCharBlockPC			(DWORD, DWORD, DWORD);							/* キャラのキャラとのぶつかり判定 */
DWORD	CheckCharBlockNPC			(DWORD, DWORD, DWORD);							/* キャラのキャラとのぶつかり判定(NPC) */
DWORD	CheckCharBlockNPC2			(DWORD, DWORD, DWORD, DWORD);					/* キャラのキャラとのぶつかり判定2(NPC) */
BOOL	CheckEventBlock				(DWORD, DWORD, DWORD, BYTE);					/* 指定座標にイベントがあるか判定 */
BOOL	CheckMagicUserInfo			(BYTE, BYTE);									/* 魔法利用者情報にあるか判定 */
BOOL	CheckEquUserInfo			(WORD, BYTE);									/* 装備職業情報にあるか判定 */
void	CheckCharEqu				(PCHARINFO);									/* 装備状況をチェックして正しくなければ解除する */
void	RenewATDF					(DWORD);										/* 攻撃力・守備力の更新 */
void	SubProcMapMove				(PCHARINFO, DWORD, DWORD, DWORD);				/* マップ移動処理 */
void	SubProcMove					(PCHARINFO, DWORD, DWORD);						/* 同マップ内移動処理 */
void	DeleteEvent					(DWORD, int);									/* イベント情報を削除 */
int		AddEvent					(DWORD);										/* イベント情報を追加 */
void	ProcBreakParty				(PCHARINFO);									/* パーティから抜ける */
void	ProcBreakUpParty			(PCHARINFO);									/* パーティを解散する */
BOOL	IsTail						(PCHARINFO pChar, PCHARINFO pCharTarget);		/* ついていけるかチェック */

/* 受信データ処理 */
void	RecvDataProc							(PBYTE, DWORD);
void	RecvDataProcLOGINREQ					(PBYTE, DWORD);				/* ログイン要求 */
void	RecvDataProcCHGSTATE					(PBYTE, DWORD);				/* 状態遷移 */
void	RecvDataProcCHGDIRECTION				(PBYTE, DWORD);				/* 向き変更要求 */
void	RecvDataProcCHARPOS						(PBYTE, DWORD);				/* 移動要求 */
void	RecvDataProcCHARINFOREQ					(PBYTE, DWORD);				/* キャラ情報要求 */
void	RecvDataProcSPEECH						(PBYTE, DWORD);				/* 発言通知 */
void	RecvDataProcHARTBEAT					(PBYTE, DWORD);				/* 生存確認通知 */
void	RecvDataProcMAPPARTSADD					(PBYTE, DWORD);				/* マップパーツ追加 */
void	RecvDataProcMAPPARTSADDANIME			(PBYTE, DWORD);				/* マップパーツアニメ追加 */
void	RecvDataProcCHGMAP						(PBYTE, DWORD);				/* マップ書き換え */
void	RecvDataProcRELOADPARTS					(PBYTE, DWORD);				/* マップパーツ更新要求 */
void	RecvDataProcMAPPARTS					(PBYTE, DWORD);				/* マップパーツ更新 */
void	RecvDataProcADDDELMAP					(PBYTE, DWORD);				/* マップ幅増減 */
void	RecvDataProcSAVEMAP						(PBYTE, DWORD);				/* マップ保存 */
void	RecvDataProcEVENTADD					(PBYTE, DWORD);				/* イベント情報追加 */
void	RecvDataProcEVENTGET					(PBYTE, DWORD);				/* イベント情報取得 */
void	RecvDataProcEVENTCHG					(PBYTE, DWORD);				/* イベント情報更新 */
void	RecvDataProcMAPINFOCHG					(PBYTE, DWORD);				/* マップ情報更新 */
void	RecvDataProcEVENTCHECK					(PBYTE, DWORD);				/* イベントチェック要求 */
void	RecvDataProcADDCHAR						(PBYTE, DWORD);				/* キャラ追加要求 */
void	RecvDataProcBANCHAR						(PBYTE, DWORD);				/* キャラ強制ログオフ要求 */
void	RecvDataProcCHARINFO					(PBYTE, DWORD);				/* キャラ情報 */
void	RecvDataProcADDMAP						(PBYTE, DWORD);				/* マップ追加 */
void	RecvDataProcADDMSG						(PBYTE, DWORD);				/* メッセージ追加 */
void	RecvDataProcMAPMSGREQ					(PBYTE, DWORD);				/* メッセージ要求 */
void	RecvDataProcCHGMSG						(PBYTE, DWORD);				/* メッセージ更新 */
void	RecvDataProcOPENDOOR					(PBYTE, DWORD);				/* 扉開け要求 */
static BOOL OpenDoorFront						(PCHARINFO);				/* 目の前の扉を開ける */
void	RecvDataProcCHGCHARGRP					(PBYTE, DWORD);				/* 着せ替え要求 */
void	RecvDataProcCHGCHARGRPBACK				(PBYTE, DWORD);				/* キャラ画像記録 */
void	RecvDataProcCHGNAMESEX					(PBYTE, DWORD);				/* 名前と性別更新 */
void	RecvDataProcGMCALL						(PBYTE, DWORD);				/* 管理人呼び出し */
void	RecvDataProcEFECTCHG					(PBYTE, DWORD);				/* エフェクト変更 */
void	RecvDataProcRESURRECTION				(PBYTE, DWORD);				/* 蘇生 */
void	RecvDataProcREQ_ATTACK					(PBYTE, DWORD);				/* 攻撃要求 */
void	RecvDataProcREQ_GUARD					(PBYTE, DWORD);				/* 構え(防御)の開始・終了 */
void	RecvDataProcMAIN_CHAR					(PBYTE, DWORD);				/* キャラ系コマンド */
void	RecvDataProcMAPMOVE						(PBYTE, DWORD);				/* マップ移動 */
void	RecvDataProcCHARPUSH					(PBYTE, DWORD);				/* キャラ押して要求 */
void	RecvDataProcCOMEFC						(PBYTE, DWORD);				/* 吹き出し要求 */
void	RecvDataProcADDSCORE					(PBYTE, DWORD);				/* 得点追加要求 */
void	RecvDataProcDELCHAR						(PBYTE, DWORD);				/* キャラ削除要求 */
void	RecvDataProcACCADD						(PBYTE, DWORD);				/* アカウント追加要求 */
void	RecvDataProcGETACC						(PBYTE, DWORD);				/* アカウント・パスワード取得 */
void	RecvDataProcCHGACC						(PBYTE, DWORD);				/* アカウント・パスワード更新 */
void	RecvDataProcTALKRAND					(PBYTE, DWORD);				/* 話してランダム要求 */
void	RecvDataProcSTOPRAND					(PBYTE, DWORD);				/* 止まるかランダム切り替え */
void	RecvDataProcGETACCINFO					(PBYTE, DWORD);				/* アカウント情報要求 */
void	RecvDataProcCHGACCSTATE					(PBYTE, DWORD);				/* アカウント状態更新 */
void	RecvDataProcCLIVERSION					(PBYTE, DWORD);				/* クライアントバージョン通知 */
void	RecvDataProcCHGCEF						(PBYTE, DWORD);				/* 吹き出し情報更新 */
void	RecvDataProcCHGADMINON					(PBYTE, DWORD);				/* 管理人ON/OFF通知 */
void	RecvDataProcADDITEM						(PBYTE, DWORD);				/* アイテム追加 */
void	RecvDataProcCHGITEM						(PBYTE, DWORD);				/* アイテム情報更新 */
void	RecvDataProcGETITEM						(PBYTE, DWORD);				/* アイテム取得 */
void	RecvDataProcDROPITEM					(PBYTE, DWORD);				/* アイテム捨てる */
void	RecvDataProcREQCHARITEM					(PBYTE, DWORD);				/* 所持アイテム情報要求 */
void	RecvDataProcCHARITEM					(PBYTE, DWORD);				/* 所持アイテム情報 */
void	RecvDataProcPLAYSOUNDALL				(PBYTE, DWORD);				/* 効果音放送 */
void	RecvDataProcUSEITEM						(PBYTE, DWORD);				/* アイテム使う */
void	RecvDataProcSVRINFOREQ					(PBYTE, DWORD);				/* サーバー情報要求 */
void	RecvDataProcNAMECOLOR					(PBYTE, DWORD);				/* 名前の色設定 */
void	RecvDataProcADDSHOP						(PBYTE, DWORD);				/* 店追加要求 */
void	RecvDataProcCHGSHOP						(PBYTE, DWORD);				/* 店情報更新 */
void	RecvDataProcADDEFC						(PBYTE, DWORD);				/* エフェクト追加 */
void	RecvDataProcCHGEFC						(PBYTE, DWORD);				/* エフェクト更新 */
void	RecvDataProcEFCREQ						(PBYTE, DWORD);				/* エフェクト発動 */
void	RecvDataProcREQCHARSTATUS				(PBYTE, DWORD);				/* ステータス情報要求 */
void	RecvDataProcCHARSTATUS					(PBYTE, DWORD);				/* キャラステータス通知 */
void	RecvDataProcTARGETCHAR					(PBYTE, DWORD);				/* ターゲット要求 */
void	RecvDataProcCHGEQU						(PBYTE, DWORD);				/* 装備変更 */
void	RecvDataProcSTTMPLATE					(PBYTE, DWORD);				/* ステータステンプレート情報 */
void	RecvDataProcLEAVEITEM					(PBYTE, DWORD);				/* アイテム預ける */
void	RecvDataProcLEAVEITEMGET				(PBYTE, DWORD);				/* 預けてるアイテム取得 */
void	RecvDataProcSUBGAMEGET					(PBYTE, DWORD);				/* サブゲーム情報取得 */
void	RecvDataProcSUBGAMEADD					(PBYTE, DWORD);				/* サブゲーム情報追加 */
void	RecvDataProcSUBGAMEINFO					(PBYTE, DWORD);				/* サブゲーム情報更新 */
void	RecvDataProcSUBGAME						(PBYTE, DWORD);				/* サブゲーム用コマンド */
void	RecvDataProcTRADE						(PBYTE, DWORD);				/* 取り引きコマンド */
void	RecvDataProcTRADEINFO					(PBYTE, DWORD);				/* 取り引き情報 */
void	RecvDataProcTRADEPROC					(PBYTE, DWORD);				/* 取り引き決定 */
void	RecvDataProcLEAVEGOLD					(PBYTE, DWORD);				/* お金預ける */
void	RecvDataProcLEAVEGOLDGET				(PBYTE, DWORD);				/* 預けてるお金取得 */
void	RecvDataProcSETSTARTPOS					(PBYTE, DWORD);				/* 復活座標設定 */
void	RecvDataProcCHGPASSWORD					(PBYTE, DWORD);				/* パスワード変更 */
void	RecvDataProcLEAVEINFOGET				(PBYTE, DWORD);				/* 預かり所情報取得 */
void	RecvDataProcLEAVEINFO					(PBYTE, DWORD);				/* 預かり所情報 */
void	RecvDataProcACCSEARCH					(PBYTE, DWORD);				/* アカウント検索 */
void	RecvDataProcMSGCMD						(PBYTE, DWORD);				/* メッセージコマンド */
void	RecvDataProcPARTY						(PBYTE, DWORD);				/* 仲間コマンド */
void	RecvDataProcJOBINFO						(PBYTE, DWORD);				/* 職業情報 */
void	RecvDataProcJOBINFOREQ					(PBYTE, DWORD);				/* 職業情報要求 */
void	RecvDataProcJOBCHG						(PBYTE, DWORD);				/* 職業変更 */
void	RecvDataProcMAGICINFO					(PBYTE, DWORD);				/* 魔法情報 */
void	RecvDataProcMAGICINFOREQ				(PBYTE, DWORD);				/* 魔法情報要求 */
void	RecvDataProcUSEMAGIC					(PBYTE, DWORD);				/* 魔法使用 */
void	RecvDataProcMAGICUSERINFO				(PBYTE, DWORD);				/* 魔法利用者情報 */
void	RecvDataProcMAGICUSERINFOREQ			(PBYTE, DWORD);				/* 魔法利用者情報要求 */
void	RecvDataProcTARGETCHG					(PBYTE, DWORD);				/* ターゲット変更要求 */
void	RecvDataProcSHOPBUY						(PBYTE, DWORD);				/* アイテム購入 */
void	RecvDataProcEQUUSERINFO					(PBYTE, DWORD);				/* 装備職業情報 */
void	RecvDataProcEQUUSERINFOREQ				(PBYTE, DWORD);				/* 装備職業情報要求 */
void	RecvDataProcWEBACCOUNT					(PBYTE, DWORD);				/* ブラウザ版のアカウント関係 */
void	RecvDataProcADMINUNDO					(PBYTE, DWORD);				/* 管理者画面の「元に戻す」 */
void	RecvDataProcNPCROUTE					(PBYTE, DWORD);				/* NPCの巡回ルート */
void	RecvDataProcRECIPEMAKE					(PBYTE, DWORD);				/* レシピで作ってもらう */
void	RecvDataProcBATTLEMODE					(PBYTE, DWORD);				/* 戦闘モードの切り替え通知 */
/* 端末トークン（server/web_account_svr.cpp） */
BOOL	WebAccTokenToAccount					(LPCSTR, LPSTR, int);		/* 端末トークンからアカウント名を引く */
static void	SendWebOptBgm					(PCHARINFO);				/* 音楽ON/OFFのアカウント設定を送る */
BOOL	WebAccIssueToken						(LPCSTR, LPSTR, int);		/* 端末トークンを発行する */
void	RecvDataProcDIFENCE						(PBYTE, DWORD);				/* 防御 */
void	RecvDataProcTHROWSNOW					(PBYTE, DWORD);				/* 雪を投げる */
/* 受信データ処理(爆弾ゲーム) */
void	RecvDataProcSubGameBOMB_SETBOMB			(PBYTE, DWORD);				/* 爆弾設置 */
void	RecvDataProcSubGameBOMB_INFO			(PBYTE, DWORD);				/* 設定 */
void	RecvDataProcSubGameBOMB_START			(PBYTE, DWORD);				/* 試合開始 */
void	RecvDataProcSubGameBOMB_STOP			(PBYTE, DWORD);				/* 試合中止 */
void	RecvDataProcSubGameBOMB_GETITEM			(PBYTE, DWORD);				/* アイテム取得 */
void	RecvDataProcSubGameBOMB_REMOCON			(PBYTE, DWORD);				/* リモコン使用 */
/* 受信データ処理(迷路) */
void	RecvDataProcSubGameMAZE_MAZERENEW		(PBYTE, DWORD);				/* 迷路作成 */
/* 受信データ処理(ダンジョン) */
void	RecvDataProcSubGameDUNGEON_DUNGEONRENEW	(PBYTE, DWORD);				/* ダンジョン作成 */
/* 受信データ処理(戦場ゲーム) */
void	RecvDataProcSubGameWAR_SELECTTEAM		(PBYTE, DWORD);				/* 戦場ゲーム：チーム選択 */
void	RecvDataProcSubGameWAR_QUIT				(PBYTE, DWORD);				/* 戦場ゲーム：ゲームから抜ける */
void	RecvDataProcSubGameWAR_SELECTRULE		(PBYTE, DWORD);				/* 戦場ゲーム：ルール選択 */
void	RecvDataProcSubGameWAR_SELECTJOB		(PBYTE, DWORD);				/* 戦場ゲーム：職業選択 */
void	RecvDataProcSubGameWAR_SELECTTURN		(PBYTE, DWORD);				/* 戦場ゲーム：順番選択 */
void	RecvDataProcSubGameWAR_WAIT				(PBYTE, DWORD);				/* 戦場ゲーム：待機 */
void	RecvDataProcSubGameWAR_ATACK			(PBYTE, DWORD);				/* 戦場ゲーム：攻撃 */
void	RecvDataProcSubGameWAR_DIFENCE			(PBYTE, DWORD);				/* 戦場ゲーム：防御行動 */
void	RecvDataProcSubGameWAR_MAGIC			(PBYTE, DWORD);				/* 戦場ゲーム：魔法 */
void	RecvDataProcSubGameWAR_MAGIC_TARGET		(PBYTE, DWORD);				/* 戦場ゲーム：魔法対象者一覧 */
/* 受信データ処理(すごろく) */
void	RecvDataProcSubGameSUGOROKU_JOINMODE	(PBYTE, DWORD);				/* すごろく：モード選択 */
void	RecvDataProcSubGameSUGOROKU_QUIT		(PBYTE, DWORD);				/* すごろく：ゲームから抜ける */
void	RecvDataProcSubGameSUGOROKU_DICE		(PBYTE, DWORD);				/* すごろく：サイコロを振る */

void	RecvDataProcMAIN_CHAR_STATEINFO			(PBYTE, DWORD);				/* キャラ系:状態通知 */
void	RecvDataProcMAIN_CHAR_TAIL				(PBYTE, DWORD);				/* キャラ系:ついていく要求 */

/* 外部参照 */
extern void OnDecPlayer					(int nNo);							/* メッセージハンドラ(WM_DECPLAYER) */
extern void EventProc					(PCHARINFO, PEVENTINFO, BYTE);		/* イベント処理 */
extern void ItemProc					(DWORD, BYTE, DWORD);				/* アイテム処理 */
extern BOOL MagicProc					(DWORD, BYTE, DWORD);				/* 魔法処理 */
extern void MakeMazeProc				(BYTE);								/* 迷路作成処理 */
extern void MakeDungeonProc				(BYTE);								/* ダンジョン作成処理 */
extern void SubGameProcBombQuit			(BYTE);								/* 爆弾ゲーム試合終了処理 */
extern void SubGameProcBombQuitMsg		(BYTE, LPCSTR);						/* 爆弾ゲーム試合終了処理(終わった理由を表示) */
extern void SubGameProcBombMakeBlock	(BYTE);								/* 爆弾ゲーム壁作成処理 */
extern void SubGameProcBombDeleteBlock	(BYTE);								/* 爆弾ゲーム壁消去処理 */
extern BOOL SaveMapPartsInfo			(void);								/* マップパーツ情報保存 */
extern BOOL SaveMapInfo					(void);								/* マップ情報保存 */
extern BOOL SaveCharInfo				(void);								/* キャラ情報保存 */
extern BOOL SaveComEfcInfo				(void);								/* 吹き出し情報保存 */
extern BOOL SaveItemInfo				(void);								/* アイテム情報保存 */
extern BOOL SaveEfcInfo					(void);								/* エフェクト情報保存 */
extern BOOL SaveStTmpInfo				(void);								/* ステータステンプレート情報保存 */
extern BOOL SaveLeaveInfo				(void);								/* 預かり所情報保存 */
extern BOOL SaveSubGameInfo				(void);								/* サブゲーム情報保存 */
extern BOOL SaveJobInfo					(void);								/* 職業情報情報保存 */
extern BOOL SaveMagicUserInfo			(void);								/* 魔法利用者情報情報保存 */
extern BOOL SaveMagicInfo				(void);								/* 魔法情報情報保存 */
extern BOOL SaveEquUserInfo				(void);								/* 装備職業情報情報保存 */
extern BOOL SaveSubGameInfo				(void);								/* サブゲーム情報保存 */
extern void WebThreadRenew				(void);								/* Web更新 */
extern void sgenrand					(unsigned long seed);				/* 乱数初期化 */
extern long genrand						();									/* 乱数取得 */


/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */

/* 関数ジャンプテーブル */
const RECVDATAPROC RecvDataProcTbl[] = {
	NULL,
	NULL,								/* CMD_CONNECT */
	RecvDataProcLOGINREQ,				/* CMD_LOGINREQ */
	NULL,								/* CMD_LOGINRES */
	RecvDataProcCHGSTATE,				/* CMD_CHGSTATE */
	NULL,								/* CMD_CHARINFO2 */
	NULL,								/* CMD_MAPINFO */
	NULL,								/* CMD_MAPPARTSINFO */
	RecvDataProcCHGDIRECTION,			/* CMD_CHGDIRECTION */
	RecvDataProcCHARPOS,				/* CMD_CHARPOS */
	RecvDataProcCHARINFOREQ,			/* CMD_CHARINFOREQ */
	RecvDataProcSPEECH,					/* CMD_SPEECH */
	NULL,								/* CMD_ONLINE */
	RecvDataProcHARTBEAT,				/* CMD_HARTBEAT */
	NULL,								/* CMD_COUNTEVENT */
	NULL,								/* CMD_EVENTPOS */
	RecvDataProcEVENTCHECK,				/* CMD_CHECKEVENT */
	NULL,								/* CMD_CHGBGM */
	NULL,								/* CMD_VIEWMSG */
	NULL,								/* CMD_PLAYSOUND */
	NULL,								/* CMD_MAPMSG */
	RecvDataProcOPENDOOR,				/* CMD_OPENDOOR */
	RecvDataProcCHGCHARGRP,				/* CMD_CHGCHARGRP */
	RecvDataProcCHGNAMESEX,				/* CMD_CHGNAMESEX */
	NULL,								/* CMD_SYSMSG */
	RecvDataProcGMCALL,					/* CMD_GMCALL */
	RecvDataProcCHARPUSH,				/* CMD_CHARPUSH */
	RecvDataProcCOMEFC,					/* CMD_COMEFC */
	NULL,								/* CMD_ERR */
	RecvDataProcADDSCORE,				/* CMD_ADDSCORE */
	RecvDataProcTALKRAND,				/* CMD_TALKRAND */
	RecvDataProcSTOPRAND,				/* CMD_STOPRAND */
	NULL,								/* CMD_CEFINFO */
	NULL,								/* CMD_FADEINOUT */
	NULL,								/* CMD_ITEMINFO */
	RecvDataProcGETITEM,				/* CMD_GETITEM */
	RecvDataProcCHARITEM,				/* CMD_CHARITEM */
	RecvDataProcDROPITEM,				/* CMD_DROPITEM */
	RecvDataProcUSEITEM,				/* CMD_USEITEM */
	NULL,								/* CMD_MAPCHARINFO */
	RecvDataProcNAMECOLOR,				/* CMD_NAMECOLOR */
	NULL,								/* CMD_SHOP */
	NULL,								/* CMD_EFC */
	NULL,								/* CMD_EFCINFO */
	RecvDataProcEFCREQ,					/* CMD_EFCREQ */
	RecvDataProcTARGETCHAR,				/* CMD_TARGETCHAR */
	NULL,								/* CMD_CHARSTATUSHP */
	NULL,								/* CMD_CHARSTATUSGOLD */
	NULL,								/* CMD_TARGETCHARPC */
	NULL,								/* CMD_ADDCHAREFC */
	RecvDataProcCHGEQU,					/* CMD_CHGEQU */
	NULL,								/* CMD_CHGMAPCOLOR */
	RecvDataProcLEAVEITEM,				/* CMD_LEAVEITEM */
	RecvDataProcLEAVEITEMGET,			/* CMD_LEAVEITEMGET */
	NULL,								/* CMD_COUNTSUBGAME */
	NULL,								/* CMD_SUBSTATUS */
	RecvDataProcSUBGAME,				/* CMD_SUBGAME */
	RecvDataProcTRADE,					/* CMD_TRADE */
	RecvDataProcTRADEINFO,				/* CMD_TRADEINFO */
	RecvDataProcTRADEPROC,				/* CMD_TRADEPROC */
	NULL,								/* CMD_SYSMSG2 */
	RecvDataProcLEAVEGOLD,				/* CMD_LEAVEGOLD */
	RecvDataProcLEAVEGOLDGET,			/* CMD_LEAVEGOLDGET */
	RecvDataProcSETSTARTPOS,			/* CMD_SETSTARTPOS */
	RecvDataProcCHGPASSWORD,			/* CMD_CHGPASSWORD */
	RecvDataProcCHGCHARGRPBACK,			/* CMD_CHGCHARGRPBACK */
	RecvDataProcMSGCMD,					/* CMD_MSGCMD */
	RecvDataProcPARTY,					/* CMD_PARTY */
	NULL,								/* CMD_CHARSTATUSHPMANA */
	RecvDataProcJOBCHG,					/* CMD_JOBCHG */
	RecvDataProcUSEMAGIC,				/* CMD_USEMAGIC */
	RecvDataProcTARGETCHG,				/* CMD_TARGETCHG */
	RecvDataProcSHOPBUY,				/* CMD_SHOPBUY */
	RecvDataProcDIFENCE,				/* CMD_DIFENCE */
	RecvDataProcTHROWSNOW,				/* CMD_THROWSNOW */
	RecvDataProcEFECTCHG,				/* CMD_EFECTCHG */
	RecvDataProcRESURRECTION,			/* CMD_RESURRECTION */
	RecvDataProcREQ_ATTACK,				/* CMD_REQ_ATTACK */
	RecvDataProcMAIN_CHAR,				/* CMD_MAIN_CHAR */
/* 以下、管理者用コマンド CMD_MAPPARTSADD より上には置かない事。判別に使ってるので。 */
	RecvDataProcMAPPARTSADD,			/* CMD_MAPPARTSADD */
	RecvDataProcCHARSTATUS,				/* CMD_CHARSTATUS */
	RecvDataProcMAPPARTS,				/* CMD_MAPPARTS */
	RecvDataProcRELOADPARTS,			/* CMD_MAPPARTSRELOAD */
	RecvDataProcMAPPARTSADDANIME,		/* CMD_MAPPARTSADDANIME */
	RecvDataProcADDMAP,					/* CMD_ADDMAP */
	RecvDataProcMAPMSGREQ,				/* CMD_MAPMSGREQ */
	RecvDataProcCHGMSG,					/* CMD_CHGMSG */
	RecvDataProcCHARINFO,				/* CMD_CHARINFO */
	RecvDataProcCHGMAP,					/* CMD_CHGMAP */
	RecvDataProcADDDELMAP,				/* CMD_ADDDELMAP */
	RecvDataProcSAVEMAP,				/* CMD_SAVEMAP */
	RecvDataProcMAPINFOCHG,				/* CMD_CHGMAPINFO */
	RecvDataProcEVENTADD,				/* CMD_EVENTADD */
	RecvDataProcEVENTGET,				/* CMD_EVENTGET */
	RecvDataProcEVENTCHG,				/* CMD_CHGEVENT */
	RecvDataProcADDCHAR,				/* CMD_ADDCHAR */
	RecvDataProcBANCHAR,				/* CMD_BANCHAR */
	RecvDataProcADDMSG,					/* CMD_ADDMSG */
	NULL,								/* CMD_MSG */
	RecvDataProcMAPMOVE,				/* CMD_MAPMOVE */
	RecvDataProcDELCHAR,				/* CMD_DELCHAR */
	RecvDataProcACCADD,					/* CMD_ACCADD */
	RecvDataProcGETACC,					/* CMD_GETACC */
	RecvDataProcCHGACC,					/* CMD_CHGACC */
	RecvDataProcGETACCINFO,				/* CMD_GETACCINFO */
	RecvDataProcCHGACCSTATE,			/* CMD_CHGACCSTATE */
	RecvDataProcCLIVERSION,				/* CMD_CLIVERSION */
	RecvDataProcCHGCEF,					/* CMD_CHGCEF */
	RecvDataProcCHGADMINON,				/* CMD_CHGADMINON */
	RecvDataProcADDITEM,				/* CMD_ADDITEM */
	RecvDataProcCHGITEM,				/* CMD_CHGITEM */
	RecvDataProcREQCHARITEM,			/* CMD_REQCHARITEM */
	RecvDataProcPLAYSOUNDALL,			/* CMD_PLAYSOUNDALL */
	RecvDataProcSVRINFOREQ,				/* CMD_SVRINFO */
	RecvDataProcADDSHOP,				/* CMD_ADDSHOP */
	RecvDataProcCHGSHOP,				/* CMD_CHGSHOP */
	RecvDataProcADDEFC,					/* CMD_ADDEFC */
	RecvDataProcCHGEFC,					/* CMD_CHGEFC */
	RecvDataProcREQCHARSTATUS,			/* CMD_REQCHARSTATUS */
	RecvDataProcSTTMPLATE,				/* CMD_STTMPLATE */
	RecvDataProcSUBGAMEGET,				/* CMD_SUBGAMEGET */
	RecvDataProcSUBGAMEADD,				/* CMD_SUBGAMEADD */
	RecvDataProcSUBGAMEINFO,			/* CMD_SUBGAMEINFO */
	NULL,								/* CMD_SUBGAMEINFOALL */
	RecvDataProcLEAVEINFOGET,			/* CMD_LEAVEINFOGET */
	RecvDataProcLEAVEINFO,				/* CMD_LEAVEINFO */
	RecvDataProcACCSEARCH,				/* CMD_ACCSEARCH */
	NULL,								/* CMD_ADMINWINDOW */
	RecvDataProcJOBINFO,				/* CMD_JOBINFO */
	RecvDataProcJOBINFOREQ,				/* CMD_JOBINFOREQ */
	RecvDataProcMAGICINFO,				/* CMD_MAGICINFO */
	RecvDataProcMAGICINFOREQ,			/* CMD_MAGICINFOREQ */
	RecvDataProcMAGICUSERINFO,			/* CMD_MAGICUSERINFO */
	RecvDataProcMAGICUSERINFOREQ,		/* CMD_MAGICUSERINFOREQ */
	RecvDataProcEQUUSERINFO,			/* CMD_EQUUSERINFO */
	RecvDataProcEQUUSERINFOREQ,			/* CMD_EQUUSERINFOREQ */
	RecvDataProcWEBACCOUNT,				/* CMD_WEBACCOUNT */
	RecvDataProcADMINUNDO,				/* CMD_ADMINUNDO */
	RecvDataProcREQ_GUARD,				/* CMD_REQ_GUARD */
	NULL,								/* CMD_HASTE(サーバー→クライアントのみ) */
	NULL,								/* CMD_MAKERINFO(サーバー→クライアントのみ) */
	NULL,								/* CMD_RECIPEINFO(サーバー→クライアントのみ) */
	RecvDataProcRECIPEMAKE,				/* CMD_RECIPEMAKE */
	RecvDataProcBATTLEMODE,				/* CMD_BATTLEMODE */
	RecvDataProcNPCROUTE,				/* CMD_NPCROUTE */
	NULL,								/* CMD_TOWNWARP(サーバー→クライアントのみ) */
	NULL
};
/* 番号とテーブルの並びがずれていないか */
static_assert (sizeof (RecvDataProcTbl) / sizeof (RecvDataProcTbl[0]) == CMD_MAX + 1, "RecvDataProcTbl");

/* 関数ジャンプテーブル(爆弾ゲーム) */
const RECVDATAPROC RecvDataProcSubGameTblBomb[] = {
	NULL,
	RecvDataProcSubGameBOMB_SETBOMB,	/* CMDSUBBOMB_SETBOMB */
	NULL,								/* CMDSUBBOMB_TIME */
	RecvDataProcSubGameBOMB_INFO,		/* CMDSUBBOMB_INFO */
	RecvDataProcSubGameBOMB_START,		/* CMDSUBBOMB_START */
	RecvDataProcSubGameBOMB_STOP,		/* CMDSUBBOMB_STOP */
	RecvDataProcSubGameBOMB_GETITEM,	/* CNDSUBBOMB_GETITEM */
	RecvDataProcSubGameBOMB_REMOCON,	/* CNDSUBBOMB_REMOCON */
	NULL
};

/* 関数ジャンプテーブル(迷路) */
const RECVDATAPROC RecvDataProcSubGameTblMaze[] = {
	NULL,
	RecvDataProcSubGameMAZE_MAZERENEW,			/* CMDSUBBOMB_MAZERENEW */
	NULL
};

/* 関数ジャンプテーブル(ダンジョン) */
const RECVDATAPROC RecvDataProcSubGameTblDungeon[] = {
	NULL,
	RecvDataProcSubGameDUNGEON_DUNGEONRENEW,	/* CMDSUBBOMB_DUNGEONRENEW */
	NULL
};

/* 関数ジャンプテーブル(戦場ゲーム) */
const RECVDATAPROC RecvDataProcSubGameTblWar[] = {
	NULL,
	NULL,										/* CMDSUBWAR_CHGSTATE */
	RecvDataProcSubGameWAR_SELECTTEAM,			/* CMDSUBWAR_SELECTTEAM */
	RecvDataProcSubGameWAR_QUIT,				/* CMDSUBWAR_QUIT */
	RecvDataProcSubGameWAR_SELECTRULE,			/* CMDSUBWAR_SELECTRULE */
	RecvDataProcSubGameWAR_SELECTJOB,			/* CMDSUBWAR_SELECTJOB */
	RecvDataProcSubGameWAR_SELECTTURN,			/* CMDSUBWAR_SELECTTURN */
	RecvDataProcSubGameWAR_WAIT,				/* CMDSUBWAR_WAIT */
	RecvDataProcSubGameWAR_ATACK,				/* CMDSUBWAR_ATACK */
	RecvDataProcSubGameWAR_DIFENCE,				/* CMDSUBWAR_DIFENCE */
	RecvDataProcSubGameWAR_MAGIC,				/* CMDSUBWAR_MAGIC */
	RecvDataProcSubGameWAR_MAGIC_TARGET,		/* CMDSUBWAR_MAGIC_TARGET */
	NULL
};

/* 関数ジャンプテーブル(すごろく) */
const RECVDATAPROC RecvDataProcSubGameTblSugoroku[] = {
	NULL,
	NULL,										/* CMDSUBSUGOROKU_CHGSTATE */
	RecvDataProcSubGameSUGOROKU_JOINMODE,		/* CMDSUBSUGOROKU_JOINMODE */
	RecvDataProcSubGameSUGOROKU_QUIT,			/* CMDSUBSUGOROKU_QUIT */
	RecvDataProcSubGameSUGOROKU_DICE,			/* CMDSUBSUGOROKU_DICE */
};

/* 関数ジャンプテーブル(キャラ系コマンド) */
const RECVDATAPROC RecvDataProcTblCHAR[] = {
	NULL,
	RecvDataProcMAIN_CHAR_STATEINFO,			/* CMDCHAR_STATEINFO */
	RecvDataProcMAIN_CHAR_TAIL,					/* CMDCHAR_TAIL */
};


/* ========================================================================= */
/* 受信データの不正チェック													 */
/*   クライアントは改造できるので、パケットに書かれた「誰の操作か」（pID・	 */
/*   dwTblID）や、アイテム・お金・座標の申告はそのまま信じない。			 */
/*   RecvDataProc が、受信した接続のキャラを下の変数に入れてから各処理を呼ぶ。	 */
/* ========================================================================= */

extern DWORD SvrTakeRecvSize (const BYTE *pData);	/* 受信データの本当の大きさ（server/CUraraSockTCP_svr.cpp） */

static DWORD	s_pIDRecv		= 0;		/* 処理中のパケットを送ってきた接続 */
static int		s_nCharNoRecv	= -1;		/* その接続のキャラ番号（未ログインなら -1） */
static BOOL		s_bAdminRecv	= FALSE;	/* 管理者からのパケット？ */
static DWORD	s_dwRecvSize	= 0;		/* 処理中のパケットの大きさ */
static const BYTE *s_pRecvData	= NULL;		/* 処理中のパケットの先頭 */
static DWORD	s_dwRecvMapNo	= (DWORD)-1;	/* 「元に戻す」で処理し直す時のマップ番号（メッセージ・店用。-1 なら送ってきたキャラのマップ） */

static void SendMsgMostCrowdedMap (DWORD pIDSend);	/* 最も人が集まっている場所を知らせる */

/* ========================================================================= */
/* 関数名：	IsRecvChar														 */
/* 内容：	操作するキャラが、送ってきた本人か（管理者なら誰でもよい）		 */
/* ========================================================================= */

static BOOL IsRecvChar(DWORD dwTblID)
{
	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return FALSE;
	}
	if (s_bAdminRecv) {
		return TRUE;
	}
	return ((s_nCharNoRecv >= 0) && (dwTblID == (DWORD)s_nCharNoRecv));
}

/* ========================================================================= */
/* 関数名：	IsNearRecvChar													 */
/* 内容：	相手が、送ってきた本人と同じマップの近く（nRange マス以内）にいるか */
/* ========================================================================= */

static BOOL IsNearRecvChar(PCHARINFO pCharTarget, int nRange)
{
	PCHARINFO pChar;

	if (s_bAdminRecv) {
		return TRUE;
	}
	if (s_nCharNoRecv < 0) {
		return FALSE;
	}
	pChar = g_aCharInfo[s_nCharNoRecv];
	if (pChar->dwMapID != pCharTarget->dwMapID) {
		return FALSE;
	}
	return ((abs ((int)pChar->dwX - (int)pCharTarget->dwX) <= nRange) &&
			(abs ((int)pChar->dwY - (int)pCharTarget->dwY) <= nRange));
}

/* ========================================================================= */
/* 関数名：	IsRecvSizeOK													 */
/* 内容：	パケットの後ろに続く可変長データが、本当に届いた範囲に収まるか	 */
/* ========================================================================= */

static BOOL IsRecvSizeOK(DWORD dwHead, DWORD dwBody)
{
	return ((dwHead <= s_dwRecvSize) && (dwBody <= s_dwRecvSize - dwHead));
}

/* ========================================================================= */
/* 関数名：	TermRecvStr														 */
/* 内容：	パケット内の固定長文字列を必ず終端させる						 */
/* ========================================================================= */

static void TermRecvStr(char *pszStr, int nSize)
{
	pszStr[nSize - 1] = 0;
}

/* ========================================================================= */
/* 関数名：	RecvEnd															 */
/* 内容：	処理中のパケットの終わり（可変長の文字列を読む時の上限）		 */
/* ========================================================================= */

static const BYTE *RecvEnd(const BYTE *pData)
{
	return pData + s_dwRecvSize;
}

/* ========================================================================= */
/* 関数名：	IsRecvStr														 */
/* 内容：	pPos からの文字列がパケットの中で終端しているか					 */
/* ========================================================================= */

static BOOL IsRecvStr(const BYTE *pPos)
{
	const BYTE *pEnd = s_pRecvData + s_dwRecvSize;

	while (pPos < pEnd) {
		if (*pPos == 0) {
			return TRUE;
		}
		pPos ++;
	}
	return FALSE;
}

/* ------------------------------------------------------------------------- */
/* メッセージの制御コード（@4 アイテム取得・@8 復活地点・@9 画像記録）		 */
/*   クライアントはメッセージを表示しながら制御コードを見つけると、サーバーに	 */
/*   アイテム取得などを頼んでくる。頼まれた内容が、サーバーが送ったメッセージ	 */
/*   か近くの NPC の台詞に本当にあったかを確かめる。						 */
/* ------------------------------------------------------------------------- */

/* 制御コード 1 つ分 */
typedef struct _MSGCODEINFO {
	char	cCode;			/* 種類（'4' '8' '9'） */
	int		anData[3];		/* 値 */
} MSGCODEINFO;

/* キャラ番号 → サーバーが送ったメッセージに入っていた制御コード（使ったら消す） */
static std::map<DWORD, std::vector<MSGCODEINFO> > s_mapMsgCode;
#define MAX_MSGCODE_KEEP	32		/* 1 キャラが覚えておく数 */

/* 数字 nLen 文字を読む（クライアントの MsgProc と同じく atoi） */
static int ReadMsgCodeNum(LPCSTR pszPos, int nLen)
{
	char szTmp[8];
	int i;

	for (i = 0; i < nLen && pszPos[i]; i ++) {
		szTmp[i] = pszPos[i];
	}
	szTmp[i] = 0;
	return atoi (szTmp);
}

/* メッセージから制御コードを取り出す */
static void ParseMsgCode(LPCSTR pszMsg, std::vector<MSGCODEINFO> &aCode)
{
	LPCSTR pszPos;
	MSGCODEINFO Info;

	if (pszMsg == NULL) {
		return;
	}
	for (pszPos = pszMsg; *pszPos; pszPos ++) {
		if ((pszPos[0] != '@') || (pszPos[1] == 0)) {
			continue;
		}
		ZeroMemory (&Info, sizeof (Info));
		Info.cCode = pszPos[1];
		switch (Info.cCode) {
		case '4':		/* アイテム取得：アイテムID 5 桁 */
		case '9':		/* 画像記録：画像ID 5 桁 */
			if (strlen (pszPos + 2) < 5) {
				continue;
			}
			Info.anData[0] = ReadMsgCodeNum (pszPos + 2, 5);
			break;
		case '8':		/* 復活地点：マップ・X・Y 各 3 桁 */
			if (strlen (pszPos + 2) < 9) {
				continue;
			}
			Info.anData[0] = ReadMsgCodeNum (pszPos + 2, 3);
			Info.anData[1] = ReadMsgCodeNum (pszPos + 5, 3);
			Info.anData[2] = ReadMsgCodeNum (pszPos + 8, 3);
			break;
		default:
			continue;
		}
		aCode.push_back (Info);
	}
}

/* ========================================================================= */
/* 関数名：	RecvCmdOnSendMsg												 */
/* 内容：	メッセージを送る時に、入っている制御コードを覚える				 */
/*			（SboSvrSendCmd.cpp の SendCmdMAPMSG から呼ぶ）					 */
/* ========================================================================= */

void RecvCmdOnSendMsg(DWORD pID, LPCSTR pszMsg)
{
	std::vector<MSGCODEINFO> aCode;
	int nNo;

	ParseMsgCode (pszMsg, aCode);
	if (aCode.empty ()) {
		return;
	}
	nNo = GetCharNo (pID);
	if (nNo < 0) {
		return;
	}
	std::vector<MSGCODEINFO> &aKeep = s_mapMsgCode[(DWORD)nNo];
	aKeep.insert (aKeep.end (), aCode.begin (), aCode.end ());
	if (aKeep.size () > MAX_MSGCODE_KEEP) {
		aKeep.erase (aKeep.begin (), aKeep.end () - MAX_MSGCODE_KEEP);
	}
}

/* ========================================================================= */
/* 関数名：	CheckMsgCode													 */
/* 内容：	頼まれた制御コードが、サーバーが送ったメッセージか				 */
/*			目の前あたりの NPC の台詞にあるか								 */
/* ========================================================================= */

static BOOL CheckMsgCode(PCHARINFO pChar, char cCode, int nData0, int nData1, int nData2)
{
	int i, nCount;
	size_t n;

	if (s_bAdminRecv) {
		return TRUE;
	}

	/* サーバーが送ったメッセージにあった？（1 回使ったら消す） */
	std::map<DWORD, std::vector<MSGCODEINFO> >::iterator it = s_mapMsgCode.find (pChar->dwTblID);
	if (it != s_mapMsgCode.end ()) {
		std::vector<MSGCODEINFO> &aKeep = it->second;
		for (n = 0; n < aKeep.size (); n ++) {
			MSGCODEINFO &Info = aKeep[n];
			if ((Info.cCode == cCode) && (Info.anData[0] == nData0) &&
					(Info.anData[1] == nData1) && (Info.anData[2] == nData2)) {
				aKeep.erase (aKeep.begin () + n);
				return TRUE;
			}
		}
	}

	/* 近くの NPC（アカウントの無いキャラ）の台詞にある？ */
	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pNpc = g_aCharInfo[i];
		if ((pNpc->szAcc[0] != 0) || (pNpc->pszNpcSpeech == NULL)) {
			continue;
		}
		if (pNpc->dwMapID != pChar->dwMapID) {
			continue;
		}
		if ((abs ((int)pNpc->dwX - (int)pChar->dwX) > 3) || (abs ((int)pNpc->dwY - (int)pChar->dwY) > 3)) {
			continue;
		}
		std::vector<MSGCODEINFO> aCode;
		ParseMsgCode (pNpc->pszNpcSpeech, aCode);
		for (n = 0; n < aCode.size (); n ++) {
			if ((aCode[n].cCode == cCode) && (aCode[n].anData[0] == nData0) &&
					(aCode[n].anData[1] == nData1) && (aCode[n].anData[2] == nData2)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* ------------------------------------------------------------------------- */
/* 取り引きの状態（以前はクライアント同士の申告だけで成立していた）			 */
/* ------------------------------------------------------------------------- */

typedef struct _TRADESTATE {
	DWORD	dwPartner;						/* 取り引き相手のキャラ番号 */
	BYTE	byMode;							/* 1 なら決定済み */
	WORD	waItem[MAX_MYITEM];				/* 出す持ち物の番号（0xFFFF は無し） */
	DWORD	dwGold;							/* 出す金額 */
} TRADESTATE;

/* キャラ番号 → 取り引きの状態 */
static std::map<DWORD, TRADESTATE> s_mapTrade;

/* 取り引きの状態を消す（相手の分も） */
static void TradeClear(DWORD dwTblID)
{
	std::map<DWORD, TRADESTATE>::iterator it = s_mapTrade.find (dwTblID);
	if (it == s_mapTrade.end ()) {
		return;
	}
	DWORD dwPartner = it->second.dwPartner;
	s_mapTrade.erase (it);
	it = s_mapTrade.find (dwPartner);
	if ((it != s_mapTrade.end ()) && (it->second.dwPartner == dwTblID)) {
		s_mapTrade.erase (it);
	}
}

/* 取り引きを始める */
static void TradeStart(DWORD dwTblID1, DWORD dwTblID2)
{
	TRADESTATE State;

	TradeClear (dwTblID1);
	TradeClear (dwTblID2);
	ZeroMemory (&State, sizeof (State));
	FillMemory (State.waItem, sizeof (State.waItem), 0xFF);
	State.dwPartner = dwTblID2;
	s_mapTrade[dwTblID1] = State;
	State.dwPartner = dwTblID1;
	s_mapTrade[dwTblID2] = State;
}

/* お互いが取り引き相手同士なら、それぞれの状態を返す */
static BOOL TradeGetPair(DWORD dwTblID1, DWORD dwTblID2, TRADESTATE **ppState1, TRADESTATE **ppState2)
{
	std::map<DWORD, TRADESTATE>::iterator it1, it2;

	it1 = s_mapTrade.find (dwTblID1);
	it2 = s_mapTrade.find (dwTblID2);
	if ((it1 == s_mapTrade.end ()) || (it2 == s_mapTrade.end ())) {
		return FALSE;
	}
	if ((it1->second.dwPartner != dwTblID2) || (it2->second.dwPartner != dwTblID1)) {
		return FALSE;
	}
	*ppState1 = &it1->second;
	*ppState2 = &it2->second;
	return TRUE;
}

/* 出す持ち物の一覧が正しいか（持ち物の番号が範囲内・空でない・重複なし） */
static BOOL TradeCheckItem(PCHARINFO pChar, const WORD *pwaItem)
{
	BOOL abUse[MAX_MYITEM];
	int i;

	ZeroMemory (abUse, sizeof (abUse));
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pwaItem[i] == 0xFFFF) {
			continue;
		}
		if (pwaItem[i] >= MAX_MYITEM) {
			return FALSE;
		}
		if (abUse[pwaItem[i]] || (pChar->waItem[pwaItem[i]] == 0) ||
				(pChar->waItem[pwaItem[i]] >= g_aItemInfo.GetSize ())) {
			return FALSE;
		}
		abUse[pwaItem[i]] = TRUE;
	}
	return TRUE;
}

/* ------------------------------------------------------------------------- */
/* 仲間の誘い（誘われていない人がパーティに入れないように覚えておく）		 */
/* ------------------------------------------------------------------------- */

/* 誘われた人のキャラ番号 → 誘った人のキャラ番号 */
static std::map<DWORD, DWORD> s_mapPartyInvite;

/* ========================================================================= */
/* 関数名：	IsRecvBombMaster												 */
/* 内容：	送ってきた人が、その爆弾ゲームのゲームマスターか				 */
/* ========================================================================= */

static BOOL IsRecvBombMaster(BYTE byID)
{
	PSUBSTATUS_BOMB pSubStatus;

	if (byID >= g_aSubaGameInfo.GetSize ()) {
		return FALSE;
	}
	if (g_aSubaGameInfo[byID]->byType != SUBGAMEID_BOMB) {
		return FALSE;
	}
	if (s_bAdminRecv) {
		return TRUE;
	}
	if (s_nCharNoRecv < 0) {
		return FALSE;
	}
	pSubStatus = (PSUBSTATUS_BOMB)g_aCharInfo[s_nCharNoRecv]->abySubStatus;
	return ((pSubStatus->byType == SUBGAMEID_BOMB) && (pSubStatus->byID == byID) && pSubStatus->byFlgGameMaster);
}

/* ========================================================================= */
/* 関数名：	RecvCmdOnDecPlayer												 */
/* 内容：	ログアウトした時に、覚えている状態を消す（SboSvr.cpp から呼ぶ）	 */
/* ========================================================================= */

void RecvCmdOnDecPlayer(int nNo)
{
	if (nNo < 0) {
		return;
	}
	TradeClear ((DWORD)nNo);
	if (nNo < g_aCharInfo.GetSize ()) {
		AdminUndoOnDisconnect (g_aCharInfo[nNo]->pID);
	}
	s_mapMsgCode.erase ((DWORD)nNo);
	s_mapPartyInvite.erase ((DWORD)nNo);
}


/* ========================================================================= */
/* 関数名：	MyOutputDebugString												 */
/* 内容：	デバッグメッセージ出力											 */
/* 日付：	2003/08/14														 */
/* ========================================================================= */

void MyOutputDebugString(LPCTSTR lpszFormat, ...)
{
	TCHAR szBuffer[1024];

	va_list argList;
	va_start (argList, lpszFormat);
	wvsprintf (szBuffer, lpszFormat, argList);
	va_end (argList);

	OutputDebugString (szBuffer);
}


/* ========================================================================= */
/* 関数名：	CheckAdmin														 */
/* 内容：	管理者権限かチェックして、違ってたらＢＡＮ						 */
/* 日付：	2003/08/14														 */
/* ========================================================================= */

BOOL CheckAdminBan(DWORD dwTblID)
{
	PCHARINFO pChar = NULL;
	BOOL bBan = TRUE, bRet = FALSE;
	if (dwTblID < (DWORD)g_aCharInfo.GetSize ()) {
		pChar = g_aCharInfo[dwTblID];
	}

	if (pChar) {
		if (pChar->byAdminLV != ADMINLV_USER) {
			bBan = FALSE;
		}
	}

	if (bBan) {
		if (pChar) {
			PostMessage (g_hWndMain, WM_DECPLAYER, (WPARAM)pChar->pID, pChar->dwTblID);
		}

	} else {
		bRet = TRUE;
	}

	return bRet;
}


/* ========================================================================= */
/* 関数名：	AddChar															 */
/* 内容：	キャラ追加														 */
/* 日付：	2003/06/14														 */
/* ========================================================================= */

PCHARINFO AddChar(PCHARINFO pChar, BOOL bCheck)
{
	int i, nMax;
	PCHARINFO pCharNew;
	PMAPINFO pMap;
	CCharProcBase CharProcBase;

	nMax = g_aCharInfo.GetSize ();
	pMap = g_aMapInfo[pChar->dwMapID];

	/* 暗闇表示？ */
	if (pMap->byFlg & MAPFLG_DARK) {
		pChar->byLightLevel		= 0;
		pChar->byLightLevelBack	= 0;
		pChar->wLightTime		= 0;
		pChar->dwTimeLight		= 0;

	} else {
		pChar->byLightLevel		= 0;
		pChar->byLightLevelBack	= 0;
		pChar->wLightTime		= 0;
		pChar->dwTimeLight		= 0;
	}

	if (bCheck) {
		for (i = 0; i < nMax; i ++) {
			pCharNew = g_aCharInfo[i];
			if ((pCharNew->dwMapID == 0) &&
				(pCharNew->dwX == 0) &&
				(pCharNew->dwY == 0) &&
				(pCharNew->szAcc[0] == 0)) {
				pChar->dwTblID = i;
				CopyMemory (pCharNew, pChar, sizeof (CHARINFO));
				CharProcBase.SetProc (pCharNew);
				AddTblNoPC (pChar->dwTblID);
				SAFE_DELETE (pChar);
				return pCharNew;
			}
		}
	}
	pChar->dwTblID = nMax;
	g_aCharInfo.Add (pChar);
	AddTblNoPC (pChar->dwTblID);

	return pChar;
}


/* ========================================================================= */
/* 関数名：	DeleteChar														 */
/* 内容：	キャラ削除														 */
/* 日付：	2006/01/13														 */
/* ========================================================================= */

void DeleteChar(DWORD dwTblID)
{
	PCHARINFO pChar;
	PCCharProcBase pProc;

	pChar = g_aCharInfo[dwTblID];

	/* 討伐報酬用に覚えていたダメージを消す */
	ClearBattleDamage (dwTblID);
	OnDecPlayer ((int)pChar->dwTblID);
	pProc = (PCCharProcBase)pChar->pProc;
	SAFE_DELETE (pProc);
	ZeroMemory (pChar, sizeof (CHARINFO));
}


/* ========================================================================= */
/* 関数名：	GetCharNo														 */
/* 内容：	キャラ番号取得													 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

int GetCharNo(DWORD pID)
{
	int i, nRetCode, nCount;

	nRetCode = -1;
	nCount = g_aCharInfo.GetSize ();

	if (pID == 0) {
		return nRetCode;
	}
	for (i = 0; i < nCount; i ++) {
		if (pID == g_aCharInfo[i]->pID) {
			nRetCode = i;
			break;
		}
	}

	return nRetCode;
}


/* ========================================================================= */
/* 関数名：	GetMapNo														 */
/* 内容：	マップ番号を取得												 */
/* 日付：	2003/05/04														 */
/* ========================================================================= */

int GetMapNo(DWORD dwID)
{
	int i, nCount;
	PMAPINFO pMapInfo;

	nCount = g_aMapInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pMapInfo = g_aMapInfo[i];
		if (pMapInfo->dwID == dwID) {
			return i;
		}
	}

	return -1;
}


/* ========================================================================= */
/* 関数名：	GetEventDataSize												 */
/* 内容：	イベント種別毎のデータサイズを取得								 */
/* 日付：	2003/06/02														 */
/* ========================================================================= */

int GetEventDataSize(BYTE byType)
{
	int nSize;

	nSize = 0;
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
/* 関数名：	GetSubGameDataSize												 */
/* 内容：	サブゲーム種別毎のデータサイズを取得							 */
/* 日付：	2003/11/08														 */
/* ========================================================================= */

int GetSubGameDataSize(BYTE byType)
{
	int nSize;

	nSize = 0;
	switch (byType) {
	case SUBGAMEID_BOMB:		nSize = sizeof (SUBGAME_BOMBINFO);		break;
	case SUBGAMEID_MAZE:		nSize = sizeof (SUBGAME_MAZEINFO);		break;
	case SUBGAMEID_DUNGEON:		nSize = sizeof (SUBGAME_DUNGEONINFO);	break;
	case SUBGAMEID_WAR:			nSize = sizeof (SUBGAME_WARINFO);		break;
	case SUBGAMEID_SUGOROKU:	nSize = sizeof (SUBGAME_SUGOROKUINFO);	break;
	}

	return nSize;
}


/* ========================================================================= */
/* 関数名：	GetItemDataSize													 */
/* 内容：	アイテム種別毎のデータサイズを取得								 */
/* 日付：	2003/09/13														 */
/* ========================================================================= */

int GetItemDataSize(BYTE byType)
{
	int nSize;

	nSize = 0;
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
	}

	return nSize;
}


/* ========================================================================= */
/* 関数名：	GetSubGameID													 */
/* 内容：	サブゲームIDを取得												 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

int GetSubGameID(PSUBGAMEINFO pInfo)
{
	int i, nCount, nRet;
	PSUBGAMEINFO pInfoTmp;

	nRet = -1;
	nCount = g_aSubaGameInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfoTmp = g_aSubaGameInfo[i];
		if (pInfo == pInfoTmp) {
			nRet = i;
			break;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	GetItemCount													 */
/* 内容：	アイテム数を取得												 */
/* 日付：	2003/09/13														 */
/* ========================================================================= */

int GetItemCount(DWORD dwID)
{
	int i, nRet;
	PCHARINFO pChar;

	nRet  = 0;
	pChar = g_aCharInfo[dwID];
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			nRet ++;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	SearchEvent														 */
/* 内容：	座標を元にイベント番号検索										 */
/* 日付：	2004/01/31														 */
/* ========================================================================= */

int SearchEvent(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	int i, nRet;
	PMAPINFO pMap;
	PEVENTINFO pEvent, pEventTmp;

	nRet	= -1;
	pMap	= g_aMapInfo[dwMapID];
	pEvent	= pMap->pEvent;

	for (i = 0; i < pMap->nEventCount; i ++) {
		pEventTmp = &pMap->pEvent[i];
		/* 座標が一致？ */
		if ((pEventTmp->dwX == dwX) && (pEventTmp->dwY == dwY)) {
			nRet = i;
			break;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	AddCharItem														 */
/* 内容：	アイテム追加													 */
/* 日付：	2003/09/13														 */
/* ========================================================================= */

void AddCharItem(DWORD dwCharID, DWORD dwItemID)
{
	int i;
	PCHARINFO pChar;

	pChar = g_aCharInfo[dwCharID];

	/* 存在しないアイテムを追加しようとした？ */
	if (dwItemID >= (DWORD)g_aItemInfo.GetSize ()) {
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, (LPARAM)pChar->dwTblID);
		return;
	}

	/* 空きスペースを探して追加 */
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i]) {
			continue;
		}
		pChar->waItem[i] = (WORD)dwItemID;
		break;
	}
}


/* ========================================================================= */
/* 関数名：	DelCharItem														 */
/* 内容：	アイテム捨てる													 */
/* 日付：	2003/09/13														 */
/* ========================================================================= */

void DelCharItem(DWORD dwCharID, BYTE byNo)
{
	int i, j;
	PCHARINFO pChar;

	pChar = g_aCharInfo[dwCharID];

	if (byNo >= MAX_MYITEM) {
		return;
	}

	/* 削除してスペースをつめる */
	for (i = byNo; i < MAX_MYITEM - 1; i ++) {
		pChar->waItem[i] = pChar->waItem[i + 1];
		for (j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] == i) {
				pChar->waEqu[j] --;
			}
		}
	}

	/* 装備品のスペースをつめる */
	for (j = 0; j < 10; j ++) {
		if (pChar->waEqu[j] == i) {
			pChar->waEqu[j] --;
		}
	}
	pChar->waItem[i] = 0;
}


/* ========================================================================= */
/* 関数名：	CheckCharItem													 */
/* 内容：	アイテム持ってるかチェック										 */
/* 日付：	2004/02/15														 */
/* ========================================================================= */

BOOL CheckCharItem(DWORD dwCharID, DWORD dwItem, BYTE byNo)
{
	BOOL bRet;
	PCHARINFO pChar;

	bRet	= FALSE;
	pChar	= g_aCharInfo[dwCharID];

	if (byNo >= MAX_MYITEM) {
		return FALSE;
	}
	if (pChar->waItem[byNo] == dwItem) {
		bRet = TRUE;
	}

	return bRet;
}


/* ========================================================================= */
/* 関数名：	RenewDataPos													 */
/* 内容：	各データ位置更新												 */
/* 日付：	2003/06/01														 */
/* ========================================================================= */

void RenewDataPos(DWORD dwID, int x, int y)
{
	int i, j, nNo;
	PCHARINFO pChar;
	PITEMINFO pItem;

	nNo = GetMapNo (dwID);
	if (nNo < 0) {
		return;
	}

	/* キャラ情報を更新 */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		pChar = g_aCharInfo[i];
		if (pChar->dwMapID != dwID) {
			continue;
		}

		/* 復活位置のマップ？ */
		if (pChar->dwStartMapID == dwID) {
			pChar->dwStartX += x;
			pChar->dwStartY += y;
		}
		pChar->dwX += x;
		pChar->dwY += y;
		pChar->byFlg |= CHARINFOFLG_MOVE;
		SendCmdCHARPOS (pChar->pID, i, 0, 0);
	}

	/* アイテム情報を更新 */
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		pItem = g_aItemInfo[i];
		switch (pItem->byType) {
		case ITEMTYPE_RURA:
			{
				PITEMINFO_RURA pInfo;

				pInfo = (PITEMINFO_RURA)pItem->pData;
				if (pInfo == NULL) {
					break;
				}

				if (pInfo->dwMapID != dwID) {
					break;
				}
				pInfo->dwX += x;
				pInfo->dwY += y;
				SendCmdCHGITEM (URARASOCK_SENDALL, i);
			}
			break;
		}
	}

	/* 開始位置を更新 */
	g_aMapInfo[dwID]->nStartX += x;
	g_aMapInfo[dwID]->nStartY += y;

	for (j = 0; j < g_aMapInfo.GetSize (); j ++) {
		BOOL bChg;
		PMAPINFO pMap;
		PEVENTINFO pEvent, pEventTmp;

		bChg	= FALSE;
		pMap	= g_aMapInfo[j];
		pEvent	= pMap->pEvent;

		if (j == nNo) {
			bChg = TRUE;
			/* イベント情報を更新 */
			for (i = 0; i < pMap->nEventCount; i ++) {
				pEventTmp = &pEvent[i];
				pEventTmp->dwX += x;
				pEventTmp->dwY += y;

				/* データあり？ */
				if (pEventTmp->pData) {
					PEVENTINFO_MOVE			pEventMove;
					PEVENTINFO_ONCHARMAP	pEventOnCharMap;

					switch (pEventTmp->byType) {
					case EVENTTYPE_MOVE:
						pEventMove = (PEVENTINFO_MOVE)pEventTmp->pData;
						pEventMove->dwX += x;
						pEventMove->dwY += y;
						break;

					case EVENTTYPE_ONCHARMAP:
						pEventOnCharMap = (PEVENTINFO_ONCHARMAP)pEventTmp->pData;
						pEventOnCharMap->dwX += x;
						pEventOnCharMap->dwY += y;
						break;
					}
				}
			}

		} else {
			/* 対象マップに移動するイベントを更新 */
			for (i = 0; i < pMap->nEventCount; i ++) {
				pEventTmp = &pEvent[i];
				/* データあり？ */
				if (pEventTmp->pData) {
					PEVENTINFO_MAPCHG pMapChg;

					switch (pEventTmp->byType) {
					case EVENTTYPE_MAPCHG:
						pMapChg = (PEVENTINFO_MAPCHG)pEventTmp->pData;
						if (pMapChg->dwMapID != dwID) {
							break;
						}

						pMapChg->dwX += x;
						pMapChg->dwY += y;
						bChg = TRUE;
						break;
					}
				}
			}
		}
		if (bChg) {
			/* 新しいイベント情報を通知 */
			SendCmdEVENTPOS (URARASOCK_SENDALL, j);
		}
	}

	/* サブゲーム情報を更新 */
	for (i = 0; i < g_aSubaGameInfo.GetSize (); i ++) {
		PSUBGAMEINFO pInfo = g_aSubaGameInfo[i];
		switch (pInfo->byType) {
		case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
			{
				PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
				if (pBomb->dwMapID != dwID) {
					break;
				}
				pBomb->byX1 += x;
				pBomb->byY1 += y;
				pBomb->byX2 += x;
				pBomb->byY2 += y;
				pBomb->dwStartX += x;
				pBomb->dwStartY += y;
				pBomb->dwX +=x;
				pBomb->dwY +=y;
			}
			break;

		case SUBGAMEID_MAZE:		/* 迷路 */
			{
				PSUBGAME_MAZEINFO pMaze = (PSUBGAME_MAZEINFO)pInfo->pInfo;
				if (pMaze->dwMapID != dwID) {
					break;
				}
				pMaze->byX1 += x;
				pMaze->byY1 += y;
				pMaze->byX2 += x;
				pMaze->byY2 += y;
			}
			break;

		case SUBGAMEID_DUNGEON:		/* ダンジョン */
			{
				PSUBGAME_DUNGEONINFO pDungeon = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
				if (pDungeon->dwMapID != dwID) {
					break;
				}
				pDungeon->byX1 += x;
				pDungeon->byY1 += y;
				pDungeon->byX2 += x;
				pDungeon->byY2 += y;
			}
			break;

		case SUBGAMEID_WAR:			/* 戦場ゲーム */
			{
				PSUBGAME_WARINFO pInfoTmp = (PSUBGAME_WARINFO)pInfo->pInfo;
				if (pInfoTmp->dwMapID != dwID) {
					break;
				}
				pInfoTmp->byX			+= x;
				pInfoTmp->byY			+= y;
				pInfoTmp->byX1			+= x;
				pInfoTmp->byY1			+= y;
				pInfoTmp->byX2			+= x;
				pInfoTmp->byY2			+= y;
				pInfoTmp->byHP1X		+= x;
				pInfoTmp->byHP2X		+= x;
				pInfoTmp->byHPY			+= y;
				pInfoTmp->byJudgeX		+= x;
				pInfoTmp->byJudgeY		+= y;
				pInfoTmp->byRes1X		+= x;
				pInfoTmp->byRes2X		+= x;
				pInfoTmp->byResY		+= y;
				pInfoTmp->byStart1X		+= x;
				pInfoTmp->byStart2X		+= x;
				pInfoTmp->byStartY		+= y;
				pInfoTmp->byCastle1X	+= x;
				pInfoTmp->byCastle2X	+= x;
				pInfoTmp->byCastleY		+= y;
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：AddTblNoPC														 */
/* 内容：PC番号テーブルに追加												 */
/* 日付：2003/09/19															 */
/* ========================================================================= */
void AddTblNoPC(int nNo)
{
	int i, nCount = 0;
	PCHARINFO pChar = g_aCharInfo[nNo];
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD *ppData = &pMap->paTblNoPC, pData, pCount = &pMap->dwCountTblNoPC;
	/* NPC？ */
	if (pChar->byGameState == GAMESTATE_NPC) {
		ppData = &pMap->paTblNoNPC;
		pCount = &pMap->dwCountTblNoNPC;

	} else {
		/* ログインメニュー画面なら追加しない */
		if (pChar->byGameState < GAMESTATE_MOVE) {
			return;
		}
	}
	pData = *ppData;

	/* 個数を求める */
	if (*ppData) {
		for (i = 0; ; i ++) {
			if (pData[i] == -1) {
				nCount = i;
				break;
			}
			/* すでにある？ */
			if (pData[i] == (DWORD)nNo) {
				return;
			}
		}
	}
	
	*pCount = nCount + 1;

	PDWORD pDataNew = new DWORD[nCount + 2];
	ZeroMemory (pDataNew, sizeof (DWORD) * (nCount + 2));
	if (pData) {
		CopyMemory (pDataNew, pData, sizeof (DWORD) * nCount);
	}
	if (nCount <= 1) {
		SAFE_DELETE (*ppData);
	} else {
		SAFE_DELETE_ARRAY (*ppData);
	}
	pDataNew[nCount] = nNo;
	pDataNew[nCount + 1] = -1;
	*ppData = pDataNew;
	SendCmdMAPCHARINFO (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：DeleteChildChar													 */
/* 内容：自分が親のキャラを削除												 */
/* 日付：2004/11/21															 */
/* ========================================================================= */
void DeleteChildChar(PCHARINFO pChar)
{
	int i, nCount = 0;
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD *ppData = &pMap->paTblNoPC, pData, pCount = &pMap->dwCountTblNoPC;

	int nNoTmp = 0;
	while (1) {
Loop:
		/* NPC？ */
		if (pChar->byGameState == GAMESTATE_NPC) {
			ppData = &pMap->paTblNoNPC;
			pCount = &pMap->dwCountTblNoNPC;
		}
		pData = *ppData;

		/* 個数を求める */
		if (*ppData) {
			for (i = 0; ; i ++) {
				if (pData[i] == -1) {
					nCount = i + 1;
					break;
				}
			}
		}

		for (i = 0; i < nCount; i ++) {
			PCHARINFO pCharTmp;

			if (pData[i] == -1) {
				i = nCount;
				break;
			}
			pCharTmp = g_aCharInfo[pData[i]];
			if (pCharTmp->dwParentTblID == pChar->dwTblID) {
				switch (pCharTmp->byNpcType) {
				case NPCTYPE_USEMAGIC:			/* 重なったキャラに魔法発動 */
				case NPCTYPE_ATACK:				/* 重なったキャラに攻撃 */
				case NPCTYPE_USEMAGICPARENT:	/* 魔法発動し続ける */
					DeleteChar (pCharTmp->dwTblID);
					goto Loop;
					break;
				}
			}
		}
		if (i >= nCount) {
			break;
		}
	}
}

/* ========================================================================= */
/* 関数名：	SetInitCharData													 */
/* 内容：	キャラの初期値を設定											 */
/* 日付：	2003/10/01														 */
/* ========================================================================= */
void SetInitCharData(PCHARINFO pChar)
{
	int i;
	CCharProcBase CharProcBase;
	PJOBINFO pJobInfo;
	PLVUPINFO pLvUpInfo;

	pChar->dwXBack			= pChar->dwX = (DWORD)g_aMapInfo[0]->nStartX;
	pChar->dwYBack			= pChar->dwY = (DWORD)g_aMapInfo[0]->nStartY;
	pChar->byPW				= 1;
	pChar->byDEX			= 1;
	pChar->byST				= 1;
	pChar->byIT				= 1;
	pChar->byLK				= 1;
	pChar->byJOB			= 0;
	pChar->byLevel			= 1;
	pChar->wMaxHP			= 20;
	pChar->wHP				= 20;
	pChar->wMaxMana			= 10;
	pChar->wMana			= 10;
	pChar->dwStartMapID		= 0;
	pChar->dwStartX			= 35;
	pChar->dwStartY			= 30;
	pChar->dwGold			= 30;
	pChar->clName			= RGB (255, 255, 255);
	pChar->clSpeech			= RGB (255, 255, 255);
	pChar->dwParentTblID	= -1;
	pChar->dwTargetTblID	= -1;
	pChar->dwTargetTblID2	= -1;
	pChar->dwTargetTblID3	= -1;
	pChar->dwTailTblID		= -1;
	pChar->dwTailBackTblID	= -1;
	pChar->dwStTmplateID	= -1;
	for (i = 0; i < 10; i ++) {
		pChar->waEqu[i] = 0xFFFF;
	}

	pJobInfo = g_JobInfo.GetPtr ((int)pChar->byJOB);
	if (pJobInfo) {
		if (pJobInfo->aLevelUpInfo.GetSize () > 0) {
			pLvUpInfo = pJobInfo->aLevelUpInfo[0];
			pChar->byPW		= pLvUpInfo->byPW;
			pChar->byDEX	= pLvUpInfo->byDEX;
			pChar->byST		= pLvUpInfo->byST;
			pChar->byIT		= pLvUpInfo->byIT;
			pChar->byLK		= pLvUpInfo->byLK;
			pChar->wMaxHP	= pLvUpInfo->wMAXHP;
			pChar->wHP		= pLvUpInfo->wMAXHP;
			pChar->wMaxMana	= pLvUpInfo->wMAXMANA;
			pChar->wMana	= pLvUpInfo->wMAXMANA;
		}
	}

	CharProcBase.SetProc (pChar);
}

/* ========================================================================= */
/* 関数名：DeleteTblNoPC													 */
/* 内容：PC番号テーブルから削除												 */
/* 日付：2003/09/19															 */
/* ========================================================================= */
void DeleteTblNoPC(int nNo)
{
	if (nNo >= g_aCharInfo.GetSize ()) {
		return;
	}

	int i, nCount = 0;
	PCHARINFO pChar = g_aCharInfo[nNo];
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD *ppData = &pMap->paTblNoPC, pData, pCount = &pMap->dwCountTblNoPC;
	/* NPC？ */
	if (pChar->byGameState == GAMESTATE_NPC) {
		ppData = &pMap->paTblNoNPC;
		pCount = &pMap->dwCountTblNoNPC;
	}
	pData = *ppData;

	/* 個数を求める */
	if (*ppData) {
		for (i = 0; ; i ++) {
			if (pData[i] == -1) {
				nCount = i + 1;
				break;
			}
		}
	}

	int nNoTmp = 0;
	for (i = 0; i < nCount; i ++) {
		if (pData[i] == -1) {
			break;
		}
		/* 見つかった？ */
		if (pData[i] == (DWORD)nNo) {
			PDWORD pDataNew = new DWORD[nCount - 1];
			ZeroMemory (pDataNew, sizeof (DWORD) * (nCount - 1));
			for (int j = 0; j < nCount; j ++) {
				if (pData[j] != (DWORD)nNo) {
					pDataNew[nNoTmp] = pData[j];
					nNoTmp ++;
				}
			}
			if (nCount <= 1) {
				SAFE_DELETE (*ppData);
			} else {
				SAFE_DELETE_ARRAY (*ppData);
			}
			*ppData = pDataNew;
			break;
		}
	}
	if (nNoTmp > 0) {
		*pCount = nNoTmp - 1;
	}
	SendCmdMAPCHARINFO (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RenewTblNoChar													 */
/* 内容：PC番号テーブルを更新												 */
/* 日付：2004/04/21															 */
/* ========================================================================= */
void RenewTblNoChar(void)
{
	int i, j, nCount, nCharCount, nMapCount;
	PCHARINFO pChar;
	PMAPINFO pMap;
	CmyArray<DWORD, DWORD>aTblNoPCTmp;
	CmyArray<DWORD, DWORD>aTblNoNPCTmp;

	nCharCount = g_aCharInfo.GetSize ();
	nMapCount = g_aMapInfo.GetSize ();

	for (i = 0; i < nMapCount; i ++) {
		pMap = g_aMapInfo[i];
		SAFE_DELETE_ARRAY (pMap->paTblNoPC);
		SAFE_DELETE_ARRAY (pMap->paTblNoNPC);

		aTblNoPCTmp.RemoveAll ();
		aTblNoNPCTmp.RemoveAll ();
		for (j = 0; j < nCharCount; j ++) {
			pChar = g_aCharInfo[j];
			if (pChar->dwMapID != (DWORD)i) {
				continue;
			}
			if (pChar->byGameState < GAMESTATE_MOVE) {
				continue;
			}
			switch (pChar->byGameState) {
			case GAMESTATE_MOVE:
				aTblNoPCTmp.Add (pChar->dwTblID);
				break;
			case GAMESTATE_NPC:
				aTblNoNPCTmp.Add (pChar->dwTblID);
				break;
			}
		}
		aTblNoPCTmp.Add (-1);
		aTblNoNPCTmp.Add (-1);

		nCount = aTblNoPCTmp.GetSize ();
		if (nCount) {
			pMap->dwCountTblNoPC = nCount - 1;
			pMap->paTblNoPC = new DWORD[nCount];
			ZeroMemory (pMap->paTblNoPC, sizeof (DWORD) * nCount);
			for (j = 0; j < nCount; j ++) {
				pMap->paTblNoPC[j] = aTblNoPCTmp[j];
			}
		}
		nCount = aTblNoNPCTmp.GetSize ();
		if (nCount) {
			pMap->dwCountTblNoNPC = nCount - 1;
			pMap->paTblNoNPC = new DWORD[nCount];
			ZeroMemory (pMap->paTblNoNPC, sizeof (DWORD) * nCount);
			for (j = 0; j < nCount; j ++) {
				pMap->paTblNoNPC[j] = aTblNoNPCTmp[j];
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：CharTargetPC														 */
/* 内容：キャラのついてく設定												 */
/* 日付：2003/10/08															 */
/* 引数：	pCharParent	ターゲットする側									 */
/*			pCharChild	ターゲットされる側									 */
/* ========================================================================= */
void CharTargetPC(PCHARINFO pCharParent, PCHARINFO pCharChild, DWORD dwData)
{
	char szTmp[128];

	if (pCharChild) {
		if (dwData != -1 && pCharChild->byNpcType) {
			return;
		}
		if (pCharChild->byCharState == CHARSTATE_DEAD) {
			return;
		}
	}

	/* 引っ張ってもらっている？ */
	if (pCharParent->dwTargetTblID3 != -1) {
		pCharChild = g_aCharInfo[pCharParent->dwTargetTblID3];
		pCharChild->dwTargetTblID2 = -1;
		pCharParent->dwTargetTblID3 = -1;
		SendCmdSYSMSG (pCharParent->pID, "ついていくのを解除しました。", SPEECHCMD_BATTLE1);
		SendCmdSYSMSG (pCharChild->pID, "ついてくるのが解除されました。", SPEECHCMD_BATTLE1);
		SendCmdTARGETCHARPC (pCharParent->pID, pCharParent->dwTblID);

	/* 先頭で引っ張っている？ */
	} else if ((dwData == -1) &&
			   (pCharParent->dwTargetTblID3 == -1) &&
			   (pCharParent->dwTargetTblID2 != -1)) {
		pCharChild = g_aCharInfo[pCharParent->dwTargetTblID2];
		pCharChild->dwTargetTblID3 = -1;
		pCharParent->dwTargetTblID2 = -1;
		SendCmdSYSMSG (pCharParent->pID, "つれていくのを解除しました。", SPEECHCMD_BATTLE1);
		SendCmdSYSMSG (pCharChild->pID, "ついていくのは解除されました。", SPEECHCMD_BATTLE1);
		SendCmdTARGETCHARPC (pCharChild->pID, pCharChild->dwTblID);

	/* 誰かについていこうとしている？ */
	} else if ((dwData != -1) &&
			   (pCharParent->dwTargetTblID3 == -1)) {
		PCHARINFO pCharTmp = pCharChild;
		/* すでにログアウトしている？ */
		if (pCharChild->pID == 0) {
			return;
		}
		/* すでに誰かを引っ張っている？ */
		if (pCharChild->dwTargetTblID2 != -1) {
			pCharTmp = g_aCharInfo[pCharChild->dwTargetTblID2];
			wsprintf (szTmp, "すでに%sを引っ張っています。", pCharTmp->szName);
			SendCmdSYSMSG (pCharParent->pID, szTmp, SPEECHCMD_BATTLE1);
			return;
		}
		if (pCharChild->dwTblID == pCharParent->dwTblID) {
			wsprintf (szTmp, "自分は引っ張れません。");
			SendCmdSYSMSG (pCharParent->pID, szTmp, SPEECHCMD_BATTLE1);
			return;
		}
		if (pCharChild->dwTargetTblID3 == pCharParent->dwTblID) {
			wsprintf (szTmp, "自分が既に引っ張っています。");
			SendCmdSYSMSG (pCharParent->pID, szTmp, SPEECHCMD_BATTLE1);
			return;
		}
		/* 親をたどるループ */
		while (1) {
			if (pCharTmp->dwTargetTblID3 == -1) {
				break;
			}
			/* 親に自分を発見？ */
			if (pCharTmp->dwTargetTblID3 == pCharParent->dwTblID) {
				wsprintf (szTmp, "自分が引っ張っています。");
				SendCmdSYSMSG (pCharParent->pID, szTmp, SPEECHCMD_BATTLE1);
				/* 無限ループ防止の為処理しない */
				return;
			}
			pCharTmp = g_aCharInfo[pCharTmp->dwTargetTblID3];
		}
		if (!((pCharParent->dwMapID == pCharChild->dwMapID) &&
			(abs (pCharParent->dwX - pCharChild->dwX) < 2) &&
			(abs (pCharParent->dwY - pCharChild->dwY) < 2))) {
			/* 離れすぎているので処理しない */
			return;
		}
		pCharChild->dwTargetTblID2 = pCharParent->dwTblID;
		pCharParent->dwTargetTblID3 = pCharChild->dwTblID;
		wsprintf (szTmp, "%sについていきます。", pCharChild->szName);
		SendCmdTARGETCHARPC (pCharParent->pID, pCharParent->dwTblID);
		SendCmdSYSMSG (pCharParent->pID, szTmp, SPEECHCMD_BATTLE1);

		/* ターゲット解除 */
		pCharParent->dwLastBattle = 0;
		pCharParent->dwTargetTblID = -1;
		SendCmdTARGETCHAR (pCharParent->dwTblID, -1);

		wsprintf (szTmp, "%sがついてきます。", pCharParent->szName);
		SendCmdSYSMSG (pCharChild->pID, szTmp, SPEECHCMD_BATTLE1);
	}
}

/* ========================================================================= */
/* 関数名：	CharTargetCancelPC												 */
/* 内容：	キャラのついていく解除											 */
/* 日付：	2003/10/08														 */
/* ========================================================================= */
void CharTargetCancelPC(PCHARINFO pChar)
{
	PCHARINFO pCharTmp;

	/* 誰かにターゲットにされている？ */
	if (pChar->dwTargetTblID2 != -1) {
		pCharTmp = g_aCharInfo[pChar->dwTargetTblID2];

		pChar->dwTargetTblID2		= -1;
		pCharTmp->dwTargetTblID3	= -1;
		SendCmdTARGETCHARPC (pCharTmp->pID, pCharTmp->dwTblID);
	}

	/* 誰かをターゲットしている？ */
	if (pChar->dwTargetTblID3 != -1) {
		pCharTmp = g_aCharInfo[pChar->dwTargetTblID3];

		pCharTmp->dwTargetTblID2	= -1;
		pChar->dwTargetTblID3		= -1;
		SendCmdTARGETCHARPC (pChar->pID, pChar->dwTblID);
	}

	TailOff (pChar);
	TailOff (pChar);
}

/* ========================================================================= */
/* 関数名：	TailMoveChain													 */
/* 内容：	ついてきているキャラを前から順に一歩ずつ動かす					 */
/*			pParent は今動いたキャラ（dwXBack/dwYBack が動く前の位置）		 */
/*			向きは「どちらへ一歩進んだか」から決める。以前は前のキャラの	 */
/*			向きの履歴を使っていて、コンベアのように向き変更の通知が無い	 */
/*			移動では角で向きがずれ、横すべりして見えていた					 */
/* ========================================================================= */
void TailMoveChain(PCHARINFO pParent)
{
	int i;
	PCHARINFO pChar;

	/* 列が輪になっていても止まるように回数を区切る */
	for (i = 0; i < 100; i ++) {
		if ((pParent->dwTailBackTblID == -1) ||
			(pParent->dwTailBackTblID >= (DWORD)g_aCharInfo.GetSize ())) {
			break;
		}
		pChar = g_aCharInfo[pParent->dwTailBackTblID];
		pChar->byFlg &= ~CHARINFOFLG_MOVEPARENT;
		if ((pChar->dwTailTblID != pParent->dwTblID) || (pChar == pParent)) {
			break;
		}
		/* 違うマップ？ */
		if (pChar->dwMapID != pParent->dwMapID) {
			CharTargetCancelPC (pChar);
			break;
		}
		/* 前のキャラが動いていない（向きを変えただけなど）、またはもうその場所にいる */
		if ((pParent->dwXBack == pParent->dwX) && (pParent->dwYBack == pParent->dwY)) {
			break;
		}
		if ((pChar->dwX == pParent->dwXBack) && (pChar->dwY == pParent->dwYBack)) {
			break;
		}

		/* 進む向き（隣のマスなら、その向き。離れていたら前の作りと同じ決め方） */
		int nAddX = (int)pParent->dwXBack - (int)pChar->dwX;
		int nAddY = (int)pParent->dwYBack - (int)pChar->dwY;
		BYTE byDirection;
		if ((nAddX == 0) && (nAddY == -1)) {
			byDirection = 0;
		} else if ((nAddX == 0) && (nAddY == 1)) {
			byDirection = 1;
		} else if ((nAddX == -1) && (nAddY == 0)) {
			byDirection = 2;
		} else if ((nAddX == 1) && (nAddY == 0)) {
			byDirection = 3;
		} else if (pParent->dwTailTblID == -1) {
			byDirection = pParent->byDirectionBack2;
		} else {
			byDirection = pParent->byDirectionBack;
		}

		pChar->byDirectionBack2 = pChar->byDirectionBack;
		pChar->byDirectionBack = pChar->byDirection;
		pChar->dwXBack = pChar->dwX;
		pChar->dwYBack = pChar->dwY;

		pChar->byDirection = byDirection;
		pChar->dwX = pParent->dwXBack;
		pChar->dwY = pParent->dwYBack;

		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 1);
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		CheckCharPosSend (pChar);
		pChar->byFlg &= ~CHARINFOFLG_DIRECTION;
		pChar->byFlg &= ~CHARINFOFLG_MOVE;

		/* 次は、このキャラについてきているキャラ */
		pParent = pChar;
	}
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
/* 関数名：GetCharX															 */
/* 内容：キャラサイズに合わせた距離を求める(横)								 */
/* 日付：2004/09/29															 */
/* ========================================================================= */
int GetCharX(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget, BOOL bRight)
{
	int nRet;

	nRet = 0;

	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		if (bRight) {
			nRet = dwXTarget - dwX;
		} else {
			nRet = dwX - dwXTarget;
		}
		break;

	case CHARGRP_MAIN_3232:
		if (bRight) {
			nRet = dwXTarget - (dwX + 1);
		} else {
			nRet = (dwX + 1) - dwXTarget;
		}
		break;

	case CHARGRP_MAIN_4848:
		if (bRight) {
			nRet = (dwX + 2) - dwXTarget;
		} else {
			nRet = dwXTarget - (dwX + 2);
		}
		break;
	}

	return nRet;
}

/* ========================================================================= */
/* 関数名：GetCharDistanceX													 */
/* 内容：キャラサイズに合わせた距離を求める(横)								 */
/* 日付：2004/09/29															 */
/* ========================================================================= */
int GetCharDistanceX(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	int nRet, nTmp, nTmpTarget;

	nTmp		= pChar->dwX;
	nTmpTarget	= pCharTarget->dwX;

	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		break;

	case CHARGRP_MAIN_3232:
		if (nTmp < nTmpTarget) {
			nTmp ++;
		}
		break;

	case CHARGRP_MAIN_4848:
		if (nTmp < nTmpTarget) {
			nTmp += 2;
			if (nTmp >= nTmpTarget) {
				nTmp = pCharTarget->dwX;
			}
		}
		break;
	}

	switch (pCharTarget->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		break;

	case CHARGRP_MAIN_3232:
		if (nTmpTarget < nTmp) {
			nTmpTarget ++;
		}
		break;

	case CHARGRP_MAIN_4848:
		if (nTmpTarget < nTmp) {
			nTmpTarget += 2;
			if (nTmpTarget >= nTmp) {
				nTmpTarget = pChar->dwX;
			}
		}
		break;
	}

	nRet = nTmpTarget - nTmp;
	return nRet;
}

/* ========================================================================= */
/* 関数名：GetCharXPos														 */
/* 内容：キャラサイズに合わせた座標を補正(横)								 */
/* 日付：2004/09/29															 */
/* ========================================================================= */
DWORD GetCharXPos(PCHARINFO pChar, DWORD dwX, BYTE byDirection)
{
	DWORD dwRet;

	dwRet = dwX;
	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		break;

	case CHARGRP_MAIN_3232:
		if (byDirection == 3) {
			dwRet ++;
		}
		break;

	case CHARGRP_MAIN_4848:
		if (byDirection == 3) {
			dwRet += 2;
		}
		break;
	}

	return dwRet;
}

/* ========================================================================= */
/* 立体交差の橋																 */
/*   地面から橋の向きに乗った人は上、横切って入った人は下を通る。			 */
/*   上か下かは橋から降りるまで覚えておく(プレイヤーは移動を受け取った時に)	 */
/* ========================================================================= */

/* マップ dwMapID の (x,y) の橋の向き(1=縦に渡る 2=横に渡る 0=橋ではない) */
static int GetSvrMapBridgeAxis(DWORD dwMapID, int x, int y)
{
	int nMapNo = GetMapNo (dwMapID);
	if (nMapNo < 0) {
		return 0;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	if ((pMap->pwMap == NULL) || (x < 0) || (y < 0) || (x >= pMap->nCX) || (y >= pMap->nCY)) {
		return 0;
	}
	WORD wParts = pMap->pwMap[x + y * pMap->nCX];
	if (wParts >= g_wCountMapParts) {
		return 0;
	}
	return GetBridgeAxis (g_aMapPartsInfo[wParts].byHitType);
}

/* 今いるマスで橋の上(1)か下(2)か(0=橋ではない)
   覚えていない時(NPC・ログイン直後)は向きで決め、pbKnown に FALSE を返す */
int GetCharBridgeLayerSvr(PCHARINFO pChar, BOOL *pbKnown)
{
	int nAxis;

	if (pbKnown) {
		*pbKnown = FALSE;
	}
	nAxis = GetSvrMapBridgeAxis (pChar->dwMapID, (int)pChar->dwX, (int)pChar->dwY);
	if (nAxis == 0) {
		return 0;
	}
	if (pChar->byBridgeLayer && (pChar->dwMapID == pChar->dwBridgeMapID) &&
		(pChar->dwX == pChar->dwBridgeX) && (pChar->dwY == pChar->dwBridgeY)) {
		if (pbKnown) {
			*pbKnown = TRUE;
		}
		return pChar->byBridgeLayer;
	}
	return GetBridgeLayerByDirection (nAxis, pChar->byDirection);
}

/* 覚えていなければ、今の向きで決めた橋の上か下かを覚える(プレイヤーが向きを変える前に) */
static void PinCharBridgeLayerSvr(PCHARINFO pChar)
{
	BOOL bKnown;
	int nLayer = GetCharBridgeLayerSvr (pChar, &bKnown);
	if ((nLayer == 0) || bKnown) {
		return;
	}
	pChar->byBridgeLayer	= (BYTE)nLayer;
	pChar->dwBridgeMapID	= pChar->dwMapID;
	pChar->dwBridgeX		= pChar->dwX;
	pChar->dwBridgeY		= pChar->dwY;
}

/* キャラが (x,y) へ動く時に、橋の上か下かを覚え直す(位置を変える前に呼ぶ) */
static void RenewCharBridgeLayerSvr(PCHARINFO pChar, int x, int y, BYTE byDirectionNew)
{
	int nAxisFrom, nAxisTo, dx, dy, nLayer;

	nAxisTo = GetSvrMapBridgeAxis (pChar->dwMapID, x, y);
	dx = x - (int)pChar->dwX;
	dy = y - (int)pChar->dwY;
	if (nAxisTo == 0) {
		nLayer = 0;
	} else if (abs (dx) + abs (dy) != 1) {
		nLayer = GetBridgeLayerByDirection (nAxisTo, byDirectionNew);
	} else {
		nAxisFrom = GetSvrMapBridgeAxis (pChar->dwMapID, (int)pChar->dwX, (int)pChar->dwY);
		nLayer = GetBridgeLayerNext (GetCharBridgeLayerSvr (pChar, NULL), nAxisFrom, nAxisTo, (dy != 0) ? 1 : 2);
	}
	pChar->byBridgeLayer	= (BYTE)nLayer;
	pChar->dwBridgeMapID	= pChar->dwMapID;
	pChar->dwBridgeX		= (DWORD)x;
	pChar->dwBridgeY		= (DWORD)y;
}

/* ログアウトする時の向きを、橋の上か下かに合わせる
   (ログインした時は向きで上か下かを決めるので、広い橋の上で横を向いていても上のまま再開できるように) */
void FitCharDirectionBridgeLayer(PCHARINFO pChar)
{
	int nAxis, nLayer;
	BOOL bKnown;

	if (pChar->byDirection >= 4) {
		return;
	}
	nAxis = GetSvrMapBridgeAxis (pChar->dwMapID, (int)pChar->dwX, (int)pChar->dwY);
	nLayer = GetCharBridgeLayerSvr (pChar, &bKnown);
	if ((nAxis == 0) || (bKnown == FALSE)) {
		return;
	}
	if (GetBridgeLayerByDirection (nAxis, pChar->byDirection) == nLayer) {
		return;
	}
	/* 上なら橋の向き、下なら橋を横切る向きにする */
	if ((nLayer == 1) == (nAxis == 1)) {
		pChar->byDirection = 1;		/* 下向き */
	} else {
		pChar->byDirection = 3;		/* 右向き */
	}
}

/* ========================================================================= */
/* 関数名：CheckCharBlock													 */
/* 内容：キャラのぶつかり判定												 */
/* 日付：2003/10/11															 */
/* ========================================================================= */
BOOL CheckCharBlock(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	BYTE byHitType;
	int nPartsID, nX, nY;
	PMAPINFO pMap;
	PMAPPARTSINFO pInfo;

	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return FALSE;
	}

	nX = (int)dwX;
	nY = (int)dwY;

	pMap = g_aMapInfo[dwMapID];
	if ((nX < 0) || (nX >= pMap->nCX) || (nY < 0) || (nY >= pMap->nCY)) {
		return FALSE;
	}

	nPartsID = pMap->pwMap[nX + nY * pMap->nCX];
	if (nPartsID >= g_wCountMapParts) {
		return FALSE;
	}

	pInfo = &g_aMapPartsInfo[nPartsID];
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
BOOL CheckCharBlock(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	BOOL bRet;

	bRet = FALSE;

	/* 橋の上から横へ落ちる・橋の下から橋の端へ出る？ */
	if ((pChar->dwMapID == dwMapID) && (pChar->byDirection < 4)) {
		int dx = (int)dwX - (int)pChar->dwX, dy = (int)dwY - (int)pChar->dwY;
		int nAxisFrom = GetSvrMapBridgeAxis (dwMapID, (int)pChar->dwX, (int)pChar->dwY);
		if ((abs (dx) + abs (dy) == 1) && nAxisFrom) {
			BOOL bKnown;
			int nMoveAxis = (dy != 0) ? 1 : 2;
			int nLayer = GetCharBridgeLayerSvr (pChar, &bKnown);
			if (bKnown) {
				if (IsBridgeMoveBlock (nLayer, nAxisFrom, GetSvrMapBridgeAxis (dwMapID, (int)dwX, (int)dwY), nMoveAxis)) {
					return FALSE;
				}
			} else if (GetDirectionAxis (pChar->byDirection) != nMoveAxis) {
				/* 上か下か覚えていない(NPC)時は、向いている方へまっすぐ抜けるだけ */
				return FALSE;
			}
		}
	}

	switch (pChar->wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
	case CHARGRP_MAIN_SUBWAR:
	case CHARGRP_MAIN_1632:
		bRet = CheckCharBlock (dwMapID, dwX, dwY);
		break;

	case CHARGRP_MAIN_3232:
		bRet = CheckCharBlock (dwMapID, dwX, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlock (dwMapID, dwX + 1, dwY);
		break;

	case CHARGRP_MAIN_4848:
		bRet = CheckCharBlock (dwMapID, dwX, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlock (dwMapID, dwX + 1, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlock (dwMapID, dwX + 2, dwY);
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckCharBlockPC													 */
/* 内容：キャラのキャラとのぶつかり判定										 */
/* 日付：2003/10/11															 */
/* ========================================================================= */
DWORD CheckCharBlockPC(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pData = pMap->paTblNoPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		if (pCharTmp->wHP == 0) {
			continue;
		}
		/* ログイン中ではない？ */
		if (pCharTmp->byGameState < GAMESTATE_MOVE) {
			continue;
		}
		/* キャラと重なる？ */
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
			return pCharTmp->dwTblID;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：CheckCharBlockNPC												 */
/* 内容：キャラのキャラとのぶつかり判定(NPC)								 */
/* 日付：2004/03/14															 */
/* ========================================================================= */
DWORD CheckCharBlockNPC(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pData = pMap->paTblNoNPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		/* キャラと重なる？ */
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
			if ((pCharTmp->byNpcType == NPCTYPE_PUT) ||
				(pCharTmp->byNpcType == NPCTYPE_USEMAGIC) ||
				(pCharTmp->byNpcType == NPCTYPE_ATACK) ||
				(pCharTmp->byNpcType == NPCTYPE_USEMAGICPARENT)
				) {
				continue;
			}
			return pCharTmp->dwTblID;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：CheckCharBlockNPC2												 */
/* 内容：キャラのキャラとのぶつかり判定(NPC)								 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
DWORD CheckCharBlockNPC2(DWORD dwTblID, DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pData = pMap->paTblNoNPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		if (dwTblID == pCharTmp->dwTblID) {
			continue;
		}
		/* キャラと重なる？ */
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
			if (pCharTmp->byNpcType == NPCTYPE_PUT) {
				continue;
			}
			return pCharTmp->dwTblID;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：CheckEventBlock													 */
/* 内容：指定座標にイベントがあるか判定										 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
BOOL CheckEventBlock(DWORD dwMapID, DWORD dwX, DWORD dwY, BYTE byMode)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PEVENTINFO pEvent = pMap->pEvent;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		pEvent = &pMap->pEvent[i];
		if ((pEvent->dwX == dwX) && (pEvent->dwY == dwY)) {
			if (byMode != 0xFF) {
				if (pEvent->byType == byMode) {
					continue;
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：CheckMagicUserInfo												 */
/* 内容：魔法利用者情報にあるか判定											 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
BOOL CheckMagicUserInfo(BYTE byNo, BYTE byJobID)
{
	BOOL bRet;
	int i, nCount;
	PMAGICUSERINFO pInfo;

	bRet = FALSE;
	if (byNo >= g_aMagicUserInfo.GetSize ()) {
		goto Exit;
	}

	pInfo = g_aMagicUserInfo[byNo];
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
/* 関数名：CheckCharEqu														 */
/* 内容：装備状況をチェックして正しくなければ解除する						 */
/* 日付：2004/07/18															 */
/* ========================================================================= */
void CheckCharEqu(PCHARINFO pChar)
{
	int i, nNo;
	PITEMINFO pItem;
	BYTE abyEquTypeTbl[] = {
		ITEMTYPE_WEAPON,
		ITEMTYPE_ARMOR,
		ITEMTYPE_SHILD,
		ITEMTYPE_HELMET,
		ITEMTYPE_EQU_HAND,
		ITEMTYPE_EQU_LEG,
		ITEMTYPE_EQU_ACC1,
		ITEMTYPE_EQU_ACC2,
		ITEMTYPE_EQU_ACC3,
		0
	};

	for (i = 0; i < MAX_EQUTYPE; i ++) {
		BOOL bCheck;

		bCheck = TRUE;

		if (pChar->waEqu[i] == 0xFFFF) {
			continue;
		}

		/* 持ち物の番号が範囲外なら、無いアイテム扱いで外す */
		nNo = g_aItemInfo.GetSize ();
		if (pChar->waEqu[i] < MAX_MYITEM) {
			nNo = pChar->waItem[pChar->waEqu[i]];
		}

		if (nNo >= g_aItemInfo.GetSize ()) {
			bCheck = FALSE;

		} else {
			WORD wNo;

			pItem = g_aItemInfo[nNo];
			/* 種別が違う？ */
			if (pItem->byType != abyEquTypeTbl[i]) {
				bCheck = FALSE;
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

			case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
				{
					PITEMINFO_EQU_HAND pInfo;

					pInfo = (PITEMINFO_EQU_HAND)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
				{
					PITEMINFO_EQU_LEG pInfo;

					pInfo = (PITEMINFO_EQU_LEG)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
				{
					PITEMINFO_EQU_ACC1 pInfo;

					pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
				{
					PITEMINFO_EQU_ACC2 pInfo;

					pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;

			case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
				{
					PITEMINFO_EQU_ACC3 pInfo;

					pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
					wNo = pInfo->wEqu;
				}
				break;
			}

			if (wNo != 0xFFFF) {
				BOOL bResult;
				bResult = CheckEquUserInfo (wNo, pChar->byJOB);
				if (bResult == FALSE) {
					bCheck = FALSE;
				}
			}
		}

		if (bCheck) {
			/* 性別による使えるかチェック */
			bCheck = CheckItemSexDistinction (pItem, pChar);
		}

		if (bCheck == FALSE) {
			pChar->waEqu[i] = 0xFFFF;
		}
	}
	RenewATDF (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RenewATDF														 */
/* 内容：攻撃力・守備力の更新												 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void RenewATDF(DWORD dwTblID)
{
	PCHARINFO pChar = g_aCharInfo[dwTblID];
	PITEMINFO pItem;
	BOOL bResult;

	/* 攻撃力 */
	pChar->wAT = pChar->byPW;
	pChar->wAT += pChar->chTimerPW;

	/* 武器を装備している？ */
	if (pChar->waEqu[0] != 0xFFFF) {
		pItem = g_aItemInfo[pChar->waItem[pChar->waEqu[0]]];
		pChar->wAT += pItem->byData;
	}

	if (pChar->byNpcType == 0) {
		/* 魔法防御値を初期化 */
		pChar->wDFMagic = 0;
	}


	/* 守備力 */
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

	/* 防御力倍？ */
	bResult = CheckCharInfoTimer (dwTblID, CHARINFOTIMEREVENT_DIFENCE);
	if (bResult) {
		pChar->wDF *= 2;
	}

	/* 酔いレベルの調整 */
	pChar->byDrunkPoint = min (pChar->byDrunkPoint, (CHARSTATEDRUNK_MAX - 1) * 10);
	pChar->byDrunkLV = pChar->byDrunkPoint / 10;
}

/* ========================================================================= */
/* 関数名：SubProcMapMove													 */
/* 内容：マップ移動処理														 */
/* 日付：2003/11/04															 */
/* ========================================================================= */
void SubProcMapMove(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	PMAPINFO pMap;
	PCHARINFO pCharTmp;

	if ((int)dwMapID >= g_aMapInfo.GetSize ()) {
		return;
	}
	/* 傭兵は別のマップへは行かない（念のため。列から外せば傭兵の処理でお別れ） */
	if (IsMercenary (pChar)) {
		MercenaryTailOff (pChar);
		return;
	}

	/* すでに移動先に立っている？ */
	if ((pChar->dwMapID == dwMapID) && (pChar->dwX == dwX) && (pChar->dwY == dwY)) {
		/* 呼び出し元がフェードアウトを送っているので、ここで戻さないと
		   マップ情報が来ないまま画面が真っ暗になる（キラメルの翼・帰還の巻物など） */
		SendCmdFADEINOUT (pChar->pID, 1);
		return;
	}

	pMap = g_aMapInfo[dwMapID];

	if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
		PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
		/* ゲームマスター？ */
		if (pSubStatus->byFlgGameMaster) {
			SubGameProcBombQuitMsg (pSubStatus->byID, "マスターが抜けたので爆弾ゲーム終了");
		}

	/* すごろくに参加中？ */
	} else if (pChar->abySubStatus[0] == SUBGAMEID_SUGOROKU) {
		PSUBGAMEINFO pInfo;
		PSUBSTATUS_SUGOROKU pSubStatus;
		PSUBGAME_SUGOROKUINFO pSugoroku;

		pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
		pInfo = g_aSubaGameInfo[pSubStatus->byID];
		pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
		SubSugoroku_RemoveAndMove (pSugoroku, pChar->dwTblID);
	}

	/* 傭兵は別のマップへは連れて行かない（列から外し、傭兵の処理でお別れ）。
	   傭兵が続けて並んでいても全部外し、つなぎ直された後ろの人を連れて行く */
	MercenaryTailOffBack (pChar);
	if (pChar->dwTailBackTblID != -1) {
		/* ついてきているキャラを移動させる */
		pCharTmp = g_aCharInfo[pChar->dwTailBackTblID];
		SubProcMapMove (pCharTmp, dwMapID, dwX, dwY);
	}

	ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));

	pChar->dwLastHartBeat = 0;
	pChar->byDirection = 99;
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 4);
	pChar->byDirection = 1;
	pChar->byDirectionBack = pChar->byDirection;
	pChar->byDirectionBack2 = pChar->byDirection;

	DeleteTblNoPC (pChar->dwTblID);
	pChar->dwX = dwX;
	pChar->dwY = dwY;
	pChar->dwX = min (pChar->dwX, (DWORD)pMap->nCX - 1);
	pChar->dwY = min (pChar->dwY, (DWORD)pMap->nCY - 1);
	pChar->dwXBack = pChar->dwX;
	pChar->dwYBack = pChar->dwY;
	pChar->dwMapID = dwMapID;
	AddTblNoPC (pChar->dwTblID);
	/* 暗闇表示？ */
	if (pMap->byFlg & MAPFLG_DARK) {
		pChar->byLightLevel = 0;
		pChar->byLightLevelBack = 0;
		pChar->wLightTime = 0;
		pChar->dwTimeLight = 0;
	} else {
		pChar->byLightLevel = 0;
		pChar->byLightLevelBack = 0;
		pChar->wLightTime = 0;
		pChar->dwTimeLight = 0;
	}

	/* マップデータを送信 */
	SendCmdMAPINFO (pChar->pID, pChar->dwMapID);
	/* イベント座標を送信 */
	SendCmdEVENTPOS (pChar->pID, pChar->dwMapID);
	/* キャラデータを送信 */
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 2);
	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：SubProcMove														 */
/* 内容：同マップ内移動処理													 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
void SubProcMove(PCHARINFO pChar, DWORD dwX, DWORD dwY)
{
	/* ターゲットを解除 */
	pChar->dwTargetTblID = -1;
	if (pChar->dwTargetTblID2 != -1) {
		PCHARINFO pCharTmp = g_aCharInfo[pChar->dwTargetTblID2];
		pCharTmp->dwTargetTblID3 = -1;
		SendCmdTARGETCHARPC (pCharTmp->pID, pCharTmp->dwTblID);
		pChar->dwTargetTblID2 = -1;
	}
	if (pChar->dwTargetTblID3 != -1) {
		PCHARINFO pCharTmp = g_aCharInfo[pChar->dwTargetTblID3];
		pCharTmp->dwTargetTblID2 = -1;
		pChar->dwTargetTblID3 = -1;
	}

	SendCmdFADEINOUT (pChar->pID, 0);
	pChar->byDirection = 1;
	pChar->byDirectionBack2 = pChar->byDirection;
	pChar->byDirectionBack = pChar->byDirection;
	pChar->dwX = pChar->dwXBack = dwX;
	pChar->dwY = pChar->dwYBack = dwY;
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdFADEINOUT (pChar->pID, 1);
	SendCmdCHARINFO3 (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：DeleteEvent														 */
/* 内容：イベント情報を削除													 */
/* 日付：2004/01/18															 */
/* ========================================================================= */
void DeleteEvent(DWORD dwMapID, int nEventID)
{
	PEVENTINFO pEventData, pEventTmp;

	DWORD dwSize = (g_aMapInfo[dwMapID]->nEventCount - 1) * sizeof (EVENTINFO);
	PBYTE pEventNew = new BYTE[dwSize];
	ZeroMemory (pEventNew, dwSize);
	pEventTmp = (PEVENTINFO)pEventNew;

	int nCount = 0;
	for (int i = 0; i < g_aMapInfo[dwMapID]->nEventCount; i ++) {
		pEventData = &g_aMapInfo[dwMapID]->pEvent[i];
		if (i == nEventID) {
			if (pEventData->pData) {
				SAFE_DELETE_ARRAY (pEventData->pData);
			}
			continue;
		}
		CopyMemory (&pEventTmp[nCount], pEventData, sizeof (EVENTINFO));
		nCount ++;
	}
	SAFE_DELETE_ARRAY (g_aMapInfo[dwMapID]->pEvent);
	g_aMapInfo[dwMapID]->pEvent = pEventTmp;
	g_aMapInfo[dwMapID]->nEventCount --;
	g_aMapInfo[dwMapID]->nProcEvent = 0;
	g_nProcMap = 0;

	/* イベント座標を通知 */
	SendCmdEVENTPOS (URARASOCK_SENDALL, dwMapID);
}

/* ========================================================================= */
/* 関数名：AddEvent															 */
/* 内容：イベント情報を追加													 */
/* 日付：2004/01/31															 */
/* ========================================================================= */
int AddEvent(DWORD dwMapID)
{
	int nRet = 0;

	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PBYTE pEventNew = new BYTE[(pMap->nEventCount + 1) * sizeof (EVENTINFO)];
	ZeroMemory (pEventNew, (pMap->nEventCount + 1) * sizeof (EVENTINFO));
	if (pMap->nEventCount > 0) {
		CopyMemory (pEventNew, pMap->pEvent, pMap->nEventCount * sizeof (EVENTINFO));
	}

	nRet = pMap->nEventCount;
	SAFE_DELETE_ARRAY (pMap->pEvent);
	pMap->pEvent = (PEVENTINFO)pEventNew;
	pMap->nEventCount ++;

	return (nRet);
}


/* ========================================================================= */
/* 関数名：	ProcBreakParty													 */
/* 内容：	パーティから抜ける												 */
/* 日付：	2006/01/04														 */
/* ========================================================================= */

void ProcBreakParty(PCHARINFO pChar)
{
	int i, j;
	char szTmp[128];
	PCHARINFO pCharTmp;
	PPARTYINFO pPartyInfo;

	if (pChar->dwPartyID == 0) {
		return;
	}

	j = 0;
	pPartyInfo = g_aPartyInfo[pChar->dwPartyID];

	wsprintf (szTmp, "%sはパーティから抜けました。", pChar->szName);
	/* 各メンバーに抜けたメッセージを通知 */
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		if (pPartyInfo->adwTblID[i] == 0) {
			break;
		}
		if (pChar->dwTblID == pPartyInfo->adwTblID[i]) {
			j = i;
			continue;
		}
		pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
		SendCmdPARTY (pCharTmp->dwTblID, pChar->dwTblID, CMDPARTY_BREAK, 0);
		SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
	}
	/* メンバーをずらす */
	for (; j < MAX_PARTYMEMBER - 1; j ++) {
		pPartyInfo->adwTblID[j] = pPartyInfo->adwTblID[j + 1];
	}

	pPartyInfo->adwTblID[j] = 0;
	pChar->dwPartyID = 0;
	SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	SendCmdPARTY (pChar->dwTblID, pChar->dwTblID, CMDPARTY_BREAKUP, 0);

	/* 一人になった？ */
	if (pPartyInfo->adwTblID[1] == 0) {
		pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[0]];
		ProcBreakUpParty (pCharTmp);
	}
}


/* ========================================================================= */
/* 関数名：	ProcBreakUpParty												 */
/* 内容：	パーティを解散する												 */
/* 日付：	2006/01/04														 */
/* ========================================================================= */

void ProcBreakUpParty(PCHARINFO pChar)
{
	int i;
	char szTmp[128];
	PCHARINFO pCharTmp;
	PPARTYINFO pPartyInfo;

	if (pChar->dwPartyID == 0) {
		return;
	}

	wsprintf (szTmp, "パーティは解散しました。");
	pPartyInfo = g_aPartyInfo[pChar->dwPartyID];
	/* 各メンバーに解散を通知 */
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		if (pPartyInfo->adwTblID[i] == 0) {
			break;
		}

		pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
		SendCmdPARTY (pCharTmp->dwTblID, pChar->dwTblID, CMDPARTY_BREAKUP, 0);
		pCharTmp->dwPartyID = 0;
		SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
	}
	ZeroMemory (pPartyInfo, sizeof (PARTYINFO));
}


/* ========================================================================= */
/* 関数名：	IsTail															 */
/* 内容：	ついていけるかチェック											 */
/* 日付：	2006/08/05														 */
/* ========================================================================= */

BOOL IsTail(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	BOOL bRet;
	DWORD dwX, dwY;

	bRet = FALSE;

	/* 誰かについていっている？ */
	if (pChar->dwTailTblID != -1) {
		goto Exit;
	}

	/* 誰かを引っ張っている？（傭兵なら、傭兵がこの人の後ろへ回るのでついていける） */
	if ((pCharTarget->dwTailBackTblID != -1) &&
		!((pCharTarget->dwTailBackTblID < (DWORD)g_aCharInfo.GetSize ()) &&
		  IsMercenary (g_aCharInfo[pCharTarget->dwTailBackTblID]))) {
		goto Exit;
	}

	switch (pCharTarget->byNpcType) {
	case NPCTYPE_INIT:			/* 未設定 */
		break;
	default:
		/* 雇い主の列に入っている傭兵にはついていける（雇い主→傭兵→自分の順に並ぶ） */
		if (IsMercenary (pCharTarget) && (pCharTarget->dwTailTblID != -1)) {
			break;
		}
		goto Exit;
	}

	switch (pCharTarget->byCharState) {
	case CHARSTATE_MOVE:		/* 歩き中 */
		break;
	default:
		goto Exit;
	}

	/* 一歩前をチェック */
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	if (!(CheckCharX (pCharTarget, pCharTarget->dwX, dwX) && (pCharTarget->dwY == dwY))) {
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：RecvDataProc														 */
/* 内容：データ受信処理														 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void RecvDataProc(PBYTE pData, DWORD dwDataSize)
{
	BYTE byCmd;
	int nNo;

	s_dwRecvSize = SvrTakeRecvSize (pData);
	s_pRecvData  = pData;
	byCmd = *pData;

	/* 知らないコマンド番号は捨てる（テーブルの外を読まないように） */
	if ((s_dwRecvSize == 0) || (byCmd >= CMD_MAX)) {
		SAFE_DELETE_ARRAY (pData);
		return;
	}

	/* 送ってきた接続のキャラ（パケットに書かれた pID・dwTblID は信じない） */
	nNo = GetCharNo (dwDataSize);
	s_pIDRecv		= dwDataSize;
	s_nCharNoRecv	= nNo;
	s_bAdminRecv	= ((nNo >= 0) && (g_aCharInfo[nNo]->byAdminLV != ADMINLV_USER));

	/* ログイン前に受け付けるのは、ログイン要求とブラウザ版のアカウント関係だけ */
	if ((nNo < 0) && (byCmd != CMD_LOGINREQ) && (byCmd != CMD_WEBACCOUNT)) {
		SAFE_DELETE_ARRAY (pData);
		return;
	}
	/* ログイン後にもう一度ログイン要求が来ても受け付けない */
	if ((nNo >= 0) && (byCmd == CMD_LOGINREQ)) {
		SAFE_DELETE_ARRAY (pData);
		return;
	}

	/* 管理者用のコマンド（管理者ウィンドウからしか送らないもの）は管理者だけ */
	if (((byCmd >= CMD_MAPPARTSADD) && (byCmd != CMD_WEBACCOUNT) && (byCmd != CMD_REQ_GUARD) && (byCmd != CMD_RECIPEMAKE) && (byCmd != CMD_BATTLEMODE)) ||
			(byCmd == CMD_CHARITEM)) {
		if (!CheckAdminBan (nNo)) {
			SAFE_DELETE_ARRAY (pData);
			return;
		}
	}
	if (RecvDataProcTbl[byCmd]) {
		/* 管理者が変えるデータは、変える前の状態を「元に戻す」用に記録する */
		BOOL bUndo = FALSE;
		if (s_bAdminRecv) {
			bUndo = AdminUndoBefore (byCmd, pData, s_dwRecvSize, s_pIDRecv,
					g_aCharInfo[nNo]->szName, AdminUndoGetScreen (s_pIDRecv));
		}
		RecvDataProcTbl[byCmd](pData, dwDataSize);
		if (bUndo) {
			AdminUndoAfter ();
		}
	}

	s_pIDRecv		= 0;
	s_nCharNoRecv	= -1;
	s_bAdminRecv	= FALSE;
	s_dwRecvSize	= 0;
	s_pRecvData		= NULL;

//char szTmp[128];
//wsprintf (szTmp, "RecvDataProc:%X\n", pData);
//OutputDebugString (szTmp);

	SAFE_DELETE_ARRAY (pData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcLOGINREQ												 */
/* 内容：受信データ処理(ログイン要求)										 */
/* 日付：2003/05/02															 */
/* ========================================================================= */
void RecvDataProcLOGINREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_LOGINREQ pPara = (PCMDPARA_LOGINREQ)pData;
	PCHARINFO pCharTmp;
	int i, nCount = g_aCharInfo.GetSize ();
	BYTE byCode = 0;
	IN_ADDR AddrTmp;

	/* プレイヤーIDはクライアントの申告ではなく、受信した接続のものを使う */
	pPara->pID = dwDataSize;

	/* この接続はもうログインしている？（1 つの接続で 2 キャラにならないように） */
	if (GetCharNo (pPara->pID) >= 0) {
		return;
	}
	TermRecvStr (pPara->szAcc, sizeof (pPara->szAcc));
	TermRecvStr (pPara->szPas, sizeof (pPara->szPas));
	/* アカウント名が空？ */
	if (pPara->szAcc[0] == 0) {
		SendCmdLOGINRES (pPara->pID, 0, 0);
		return;
	}

	for (i = 0; i < nCount; i ++) {
		pCharTmp = g_aCharInfo[i];
		/* アカウント発見？ */
		if ((strlen (pPara->szAcc) == strlen (pCharTmp->szAcc)) &&
				(!strcmp (pPara->szAcc, pCharTmp->szAcc))) {
			byCode = 1;
			LPCSTR pszPas = pCharTmp->szPas;
			/* 管理者のキャラでパスワードが空なら、INI の管理者パスワードで確かめる。
			   それも空ならログインさせない（誰でも管理者になれてしまうので） */
			if ((pCharTmp->byAdminLV != ADMINLV_USER) && (pszPas[0] == 0)) {
				pszPas = g_szAdminPas;
				if (pszPas[0] == 0) {
					break;
				}
			}
			/* パスワード一致？ */
			if ((strlen (pPara->szPas) == strlen (pszPas)) &&
					(!strcmp (pPara->szPas, pszPas))) {
				byCode = 2;
				/* ログイン中？ */
				if (pCharTmp->byGameState != GAMESTATE_LOGOFF) {
					byCode = 3;
				}
			}
			break;
		}
	}
	/* まだ見つかってない？ */
	if (!byCode) {
		/* アカウント発見？ */
		if ((strlen (pPara->szAcc) == strlen (g_szAdminAcc)) &&
				(!strcmp (pPara->szAcc, g_szAdminAcc))) {
			byCode = 1;
			/* パスワード一致？（パスワードが空の設定では管理者ログインさせない） */
			if ((g_szAdminPas[0] != 0) &&
					(strlen (pPara->szPas) == strlen (g_szAdminPas)) &&
					(!strcmp (pPara->szPas, g_szAdminPas))) {
				/* 管理者アカウントでログイン */
				byCode = 4;
			}
		}
	}

	/* 見つからなかった？ */
	if (!byCode && !g_bFreeAcount) {
		SendCmdLOGINRES (pPara->pID, 0, 0);
		MyOutputDebugString ("アカウント見つからず[%s]", pPara->szAcc);
		return;

	/* パスワード間違い？ */
	} else if (byCode == 1) {
		SendCmdLOGINRES (pPara->pID, 0, 1);
		MyOutputDebugString ("パスワード間違い[%s]", pPara->szAcc);
		return;

	/* キャラが見つかった？ */
	} else if (byCode == 2) {
		if (pCharTmp->byAccState & ACCSTATE_DISABLE) {
			SendCmdLOGINRES (pPara->pID, 0, 6);
			return;
		}
		if ((pCharTmp->byAdminLV == ADMINLV_USER) && (g_dwOnline >= MAX_CONNECTCOUNT)) {
			SendCmdLOGINRES (pPara->pID, 0, 4);
			return;
		}
		if (pCharTmp->pID) {
			SendCmdLOGINRES (pPara->pID, pCharTmp->dwTblID, 3);
			return;
		}
		pCharTmp->pID = pPara->pID;
		pCharTmp->dwLastIPAddress = svrSock.GetIPAddress (pPara->pID);
		/* 向きはログアウトした時のまま(立体交差の橋の上か下かは向きで決まる) */
		if (pCharTmp->byDirection >= 4) {
			pCharTmp->byDirection = 1;
		}
		pCharTmp->dwLoginCount ++;
		pCharTmp->dwTblID = i;
		CopyMemory (pCharTmp->szMacAdr, pPara->szMacAdr, 6);
		SendCmdCHARINFO (pPara->pID, pCharTmp->dwTblID);
		SendCmdLOGINRES (pPara->pID, pCharTmp->dwTblID, 2);
		SendWebOptBgm (pCharTmp);
		AddrTmp.S_un.S_addr = pCharTmp->dwLastIPAddress;
		g_LogLogin.Write ("ログイン\t[%s][%s][%d.%d.%d.%d][%02X-%02X-%02X-%02X-%02X-%02X]",
				pCharTmp->szName,
				pCharTmp->szAcc,
				AddrTmp.S_un.S_un_b.s_b1, AddrTmp.S_un.S_un_b.s_b2, AddrTmp.S_un.S_un_b.s_b3, AddrTmp.S_un.S_un_b.s_b4,
				(BYTE)pCharTmp->szMacAdr[0], (BYTE)pCharTmp->szMacAdr[1], (BYTE)pCharTmp->szMacAdr[2],
				(BYTE)pCharTmp->szMacAdr[3], (BYTE)pCharTmp->szMacAdr[4], (BYTE)pCharTmp->szMacAdr[5]
				);
		return;

	/* ログイン済み？ */
	} else if (byCode == 3) {
		SendCmdLOGINRES (pPara->pID, g_aCharInfo[i]->dwTblID, 3);
		MyOutputDebugString ("ログイン済み[%s]\n", pPara->szAcc);
		return;
	}

	/* 定員オーバー？ */
	if ((byCode != 4) && (g_dwOnline >= MAX_CONNECTCOUNT)) {
//		SendCmdLOGINRES (pPara->pID, 0, 4);
//		MyOutputDebugString ("定員オーバー[%s]\n", pPara->szAcc);
//		return;
	}

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pCharTmp = g_aCharInfo[i];
		BYTE abyTmp[6];
		/* すでに作成している？ */
		if (memcmp (pCharTmp->szMacAdr, pPara->szMacAdr, 6) == 0) {
			ZeroMemory (abyTmp, sizeof (abyTmp));
			/* 空(MACアドレス取得失敗)ではない？ */
			if (memcmp (pCharTmp->szMacAdr, abyTmp, 6)) {
				SendCmdLOGINRES (pPara->pID, 0, 5);
				return;
			}
		}
	}

	PCHARINFO pChar = new CHARINFO;
	ZeroMemory ((PBYTE)pChar, sizeof (CHARINFO));
	SetInitCharData (pChar);
	pChar->pID = pPara->pID;
	strcat (pChar->szAcc, pPara->szAcc);
	strcat (pChar->szPas, pPara->szPas);
	CopyMemory (pChar->szMacAdr, pPara->szMacAdr, 6);
	if (byCode == 4) {
		pChar->byAdminLV = ADMINLV_ADMINISTRATOR;
		pChar->wGrpID = 16;
	} else {
		pChar->wGrpID = 2;
	}
	pChar->wGrpIDBack = pChar->wGrpID;
	pChar->byDirection = 1;

	pChar = AddChar (pChar);
	RenewATDF (pChar->dwTblID);

	SendCmdCHARINFO (pChar->pID, pChar->dwTblID);
	SendCmdLOGINRES (pChar->pID, pChar->dwTblID, 2);
	SendWebOptBgm (pChar);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGSTATE												 */
/* 内容：受信データ処理(状態遷移)											 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void RecvDataProcCHGSTATE(PBYTE pData, DWORD dwDataSize)
{
	char szBuf[256];
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int i, nCount, nNo = GetCharNo (s_pIDRecv);
	if (nNo < 0) {
		return;
	}

	if (strlen (g_aCharInfo[nNo]->szName) == 0) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[nNo];
	MyOutputDebugString ("ログイン：[%s] pID:%ld TblID:%ld\n",
			pChar->szName, pChar->pID, pChar->dwTblID);

	BOOL bCheck = CheckCharBlock (pChar->dwMapID, pChar->dwX, pChar->dwY);
	if (pChar->wHP == 0) {
		pChar->dwGold	/= 2;
		pChar->wHP		= pChar->wMaxHP;
		pChar->wMana	= pChar->wMaxMana;
		bCheck = FALSE;
	}
	/* ぶつかる位置にいる？ */
	if (bCheck == FALSE) {
		/* イベントは配置されていない？ */
		if (CheckEventBlock (pChar->dwMapID, pChar->dwX, pChar->dwY, 0xFF) == FALSE) {
			pChar->dwMapID = pChar->dwStartMapID;
			pChar->dwX = pChar->dwStartX;
			pChar->dwY =pChar->dwStartY;
			pChar->dwXBack = pChar->dwX;
			pChar->dwYBack = pChar->dwY;
		}
	}

	/* 装備品をチェック */
	CheckCharEqu (pChar);

	/* 透明？ */
	if ((pChar->wGrpMain == CHARGRP_MAIN_NORMAL) && (pChar->wGrpID == 0)) {
		pChar->wGrpID = 2;
		pChar->wGrpIDBack = 2;
		pChar->wGrpSub = 0;
	}
	/* 管理人以外 */
	if ((pChar->byAccState & ACCSTATE_ADMINON) == 0) {
		BOOL bInit;

		bInit = FALSE;
		/* 復活マップIDが異常？ */
		if (pChar->dwStartMapID >= (DWORD)g_aMapInfo.GetSize ()) {
			bInit = TRUE;
		}
		if (bInit) {
			/* 初期化 */
			SetInitCharData (pChar);
		}
	}

	pChar->dwLastHartBeat	= 0;
	pChar->byGameState		= GAMESTATE_MOVE;
	pChar->byCharState		= CHARSTATE_MOVE;
	pChar->dwTimeLogin		= timeGetTime ();

	SendCmdSTTMPLATE (pChar->dwTblID, CMDSTTMPLATETYPE_ALL);
	/* マップパーツ情報を送信 */
	SendCmdMAPPARTSINFO (pPara->pID);
	/* 吹き出し情報を送信 */
	SendCmdCEFINFO (pPara->pID);
	/* エフェクト情報を送信 */
	SendCmdEFCINFO (pPara->pID);
	/* 職業情報を送信 */
	nCount = g_JobInfo.GetCount ();
	for (i = 0; i < nCount; i ++) {
		SendCmdJOBINFO (pChar->pID, i);
	}
	/* 装備職業情報を送信 */
	nCount = g_aEquUserInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SendCmdEQUUSERINFO (pChar->pID, i);
	}
	/* 魔法利用者情報を送信 */
	nCount = g_aMagicUserInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SendCmdMAGICUSERINFO (pChar->pID, i);
	}
	/* 魔法情報を送信 */
	nCount = g_aMagicInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		SendCmdMAGICINFO (pChar->pID, i);
	}
	/* アイテム情報を送信 */
	SendCmdITEMINFO (pPara->pID);
	/* 作り手とレシピを送信 */
	SendCmdRECIPEINFOALL (pPara->pID);
	/* 町転送の行き先一覧を送信 */
	SendCmdTOWNWARP (pChar->dwTblID);
	/* マップデータを送信 */
	SendCmdMAPINFO (pPara->pID, GetMapNo (pChar->dwMapID));
	/* イベント座標を送信 */
	SendCmdEVENTPOS (pPara->pID, GetMapNo (pChar->dwMapID));
	/* キャラデータを送信 */
	SendCmdCHARINFO (pPara->pID, -1);
	SendCmdCHARINFO3 (nNo);
	SendCmdCHARITEM (pPara->pID, pChar->dwTblID);
	SendCmdCHARSTATUS (pPara->pID, nNo);

	SendCmdCHGSTATE (pPara->pID, pPara->dwTblID, GAMESTATE_MOVE);
	if (pChar->byAdminLV == ADMINLV_ADMINISTRATOR) {
		SendCmdADMINWINDOW (pPara->pID, 1);
		SendCmdSUBGAMEINFOALL (pPara->pID);
	}

	g_dwOnline ++;
	if (pChar->byAccState & ACCSTATE_ADMINON) {
		g_byAdminCount ++;
	}

	/* 時間毎の最大人数を更新 */
	SYSTEMTIME sysTime;
	GetLocalTime (&sysTime);
	if (g_dwOnline > g_dwOnlineBack[sysTime.wHour]) {
		g_dwOnlineBack[sysTime.wHour] = g_dwOnline;
		if (g_dwOnline > g_dwOnlineBackMax[sysTime.wHour]) {
			g_dwOnlineBackMax[sysTime.wHour] = g_dwOnline;
		}
	}

	AddTblNoPC (nNo);
	SendCmdONLINE (URARASOCK_SENDALL);

	SendCmdSYSMSG (pPara->pID, "SYSTEM:ｽｸﾗｯﾌﾟﾌﾞｯｸｵﾝﾗｲﾝの世界へようこそ♪");
	wsprintf (szBuf, "SYSTEM:現在のｵﾝﾗｲﾝ数:%ld人", g_dwOnline);
	SendCmdSYSMSG (pPara->pID, szBuf);
	wsprintf (szBuf, "SYSTEM:最新ｸﾗｲｱﾝﾄﾊﾞｰｼﾞｮﾝ:%s", g_szCliVer);
	SendCmdSYSMSG (pPara->pID, szBuf);

	if (g_dwOnline % 10 == 0) {
		wsprintf (szBuf, "SYSTEM:オンライン数が%d人になりました。", g_dwOnline);
		SendCmdSYSMSG (URARASOCK_SENDALL, szBuf);
	}
	SendMsgMostCrowdedMap (pPara->pID);
}

/* ========================================================================= */
/* 関数名：SendMsgMostCrowdedMap											 */
/* 内容：最も人が集まっている場所を知らせる									 */
/*       （ログイン時と、メッセージコマンド /where・その他メニューで使う）	 */
/* ========================================================================= */
static void SendMsgMostCrowdedMap(DWORD pIDSend)
{
	char szBuf[256];
	DWORD dwMap = 0, dwCount = 0;
	int i;

	for (i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		if (pMap->dwCountTblNoPC > dwCount) {
			dwCount = pMap->dwCountTblNoPC;
			dwMap = i;
		}
	}
	if (g_aMapInfo.GetSize () <= 0) {
		return;
	}
	SendCmdSYSMSG (pIDSend, "SYSTEM:現在最も人が集まっている場所は");

	PMAPINFO pMap = g_aMapInfo[dwMap];
	if (pMap->pszName && strlen ((LPSTR)pMap->pszName)) {
		wsprintf (szBuf, "SYSTEM:[%s]のようです。", pMap->pszName);
	} else {
		wsprintf (szBuf, "SYSTEM:マップ番号[%ld]のようです。", dwMap);
	}
	SendCmdSYSMSG (pIDSend, szBuf);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGDIRECTION											 */
/* 内容：受信データ処理(向き変更要求)										 */
/* 日付：2003/05/14															 */
/* ========================================================================= */
void RecvDataProcCHGDIRECTION(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];

	/* 誰かに引っ張られてる？ */
	if (pChar->dwTargetTblID3 != -1) {
		return;
	}

	/* 向きは 0～3（座標の増分テーブルの外を読まないように） */
	if (pPara->byData >= 4) {
		return;
	}
	/* 橋のマスで向きを変える前に、今の向きで橋の上か下かを覚えておく
	   (ログイン直後はまだ覚えていないので、向きを変えると上下が入れ替わらないように) */
	PinCharBridgeLayerSvr (pChar);
	pChar->byDirection = pPara->byData;
	pChar->byFlg |= CHARINFOFLG_DIRECTION;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARPOS												 */
/* 内容：受信データ処理(移動要求)											 */
/* 日付：2003/05/17															 */
/* ========================================================================= */
void RecvDataProcCHARPOS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARPOS pPara = (PCMDPARA_CHARPOS)pData;
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];

	/* 座標を更新 */
	if (pChar->dwMapID == pPara->dwMapID) {
		/* 誰かに引っ張られてる？ */
		if (pChar->dwTargetTblID3 != -1) {
			return;
		}
		/* 向きは 0～3、座標はマップの中 */
		int nMapNo = GetMapNo (pChar->dwMapID);
		if ((pPara->byDirection >= 4) || (nMapNo < 0) ||
				(pPara->byX >= g_aMapInfo[nMapNo]->nCX) || (pPara->byY >= g_aMapInfo[nMapNo]->nCY)) {
			return;
		}
		/* 一度に動きすぎ？（改造クライアントのワープ対策。遅れて届いた分も考えて 2 マスまで）
		   動きすぎなら、サーバーの知っている位置をクライアントに送り直す */
		if ((abs ((int)pPara->byX - (int)pChar->dwX) > 2) ||
			(abs ((int)pPara->byY - (int)pChar->dwY) > 2)) {
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			return;
		}
#if 0
		/* 移動しすぎ？ */
		if ((abs ((DWORD)pPara->byX - pChar->dwX) > 1) ||
			(abs ((DWORD)pPara->byY - pChar->dwY) > 1)) {
			/* 初期位置に戻す */
			SendCmdFADEINOUT (pChar->pID, 0);
			SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);
			SubProcMapMove (pChar, pChar->dwStartMapID, pChar->dwStartX, pChar->dwStartY);

			/* 強制ログアウト */
			PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, pPara->dwTblID);
			return;
		}
#endif
		if ((pChar->dwXBack != pChar->dwX) || (pChar->dwYBack != pChar->dwY)) {
			pChar->dwLastDotMove = timeGetTime ();
		}

		pChar->byDirectionBack2 = pChar->byDirectionBack;
		pChar->byDirectionBack = pChar->byDirection;
		pChar->dwXBack = pChar->dwX;
		pChar->dwYBack = pChar->dwY;

		/* 立体交差の橋の上か下かを覚え直す */
		if ((pChar->dwX != (DWORD)pPara->byX) || (pChar->dwY != (DWORD)pPara->byY)) {
			RenewCharBridgeLayerSvr (pChar, (int)pPara->byX, (int)pPara->byY, pPara->byDirection);
		}
		pChar->byDirection = pPara->byDirection;
		pChar->dwX = (DWORD)pPara->byX;
		pChar->dwY = (DWORD)pPara->byY;

		/* 誰かを引っ張っている？
		   以前は旗を立てて次のキャラ処理で動かしていたが、移動が続けて届くと
		   ついてくる側が1マス飛ばして斜めに動き、列が崩れていたので、ここで続けて動かす */
		TailMoveChain (pChar);

		if (pChar->byFlg & CHARINFOFLG_MOVE) {
			CheckCharPosSend (pChar);
		}
		pChar->byFlg |= CHARINFOFLG_MOVE;

		if (pChar->dwLastBattle) {
			/* 移動しなおしたのでカウンタ初期化 */
			pChar->dwLastBattle = timeGetTime ();
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARINFOREQ											 */
/* 内容：受信データ処理(キャラ情報要求)										 */
/* 日付：2003/05/21															 */
/* ========================================================================= */
void RecvDataProcCHARINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARINFOREQ pPara = (PCMDPARA_CHARINFOREQ)pData;
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	SendCmdCHARINFO2 (pPara->pID, pPara->dwTblID, pPara->dwTblIDReq);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSPEECH												 */
/* 内容：受信データ処理(発言通知)											 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void RecvDataProcSPEECH(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SPEECH pPara = (PCMDPARA_SPEECH)pData;
	int nNo = pPara->dwTblID;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	PCHARINFO pChar = g_aCharInfo[nNo];

	/* 文字数の申告が、本当に届いた大きさを超えている？ */
	if (!IsRecvSizeOK (sizeof (CMDPARA_SPEECH), pPara->wSize)) {
		return;
	}

	SAFE_DELETE_ARRAY (pChar->pszSpeech);
	pChar->pszSpeech = (LPSTR)new BYTE[pPara->wSize + 1];
	ZeroMemory ((PBYTE)pChar->pszSpeech, pPara->wSize + 1);
	CopyMemory ((PBYTE)pChar->pszSpeech, (PBYTE)pData + sizeof (CMDPARA_SPEECH), pPara->wSize);
	SboTextSanitize (pChar->pszSpeech);		/* UTF-8 として正しくないバイトは「?」に */
	pChar->byFlg |= CHARINFOFLG_SPEECH;
	pChar->bySpeechFlg = pPara->byType;

	/* 発言内容の設定？ */
	if (pPara->byType == SPEECHCMD_SPEECHEDIT) {
		SAFE_DELETE_ARRAY (pChar->pszNpcSpeech);
		pChar->pszNpcSpeech = (LPSTR)new BYTE[pPara->wSize + 1];
		ZeroMemory ((PBYTE)pChar->pszNpcSpeech, pPara->wSize + 1);
		CopyMemory ((PBYTE)pChar->pszNpcSpeech, (PBYTE)pData + sizeof (CMDPARA_SPEECH), pPara->wSize);
		SboTextSanitize (pChar->pszNpcSpeech);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcHARTBEAT												 */
/* 内容：受信データ処理(生存確認通知)										 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
void RecvDataProcHARTBEAT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
#if 0
	/* 接続チェック受信間隔が早すぎる？ */
	if (pChar->dwLastHartBeat && (abs (pChar->dwLastHartBeat - timeGetTime ()) < SENDHARTBEATTIME / 2)) {
		/* 初期位置に戻す */
		SendCmdFADEINOUT (pChar->pID, 0);
		SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);
		SubProcMapMove (pChar, pChar->dwStartMapID, pChar->dwStartX, pChar->dwStartY);

		/* 強制ログアウト */
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, pPara->dwTblID);
		return;
	}
#endif
	pChar->dwLastHartBeat = timeGetTime ();
	SendCmdHARTBEAT (g_aCharInfo[pPara->dwTblID]->pID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPPARTSADD											 */
/* 内容：受信データ処理(マップパーツ追加)									 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void RecvDataProcMAPPARTSADD(PBYTE pData, DWORD dwDataSize)
{
	int i;
	PMAPPARTSINFO pParts = new MAPPARTSINFO [g_wCountMapParts + 1], pPartsTmp1, pPartsTmp2;
	ZeroMemory (pParts, sizeof (MAPPARTSINFO) * (g_wCountMapParts + 1));

	for (i = 0; i < g_wCountMapParts; i ++) {
		pPartsTmp1 = &pParts[i];
		pPartsTmp2 = &g_aMapPartsInfo[i];
		pPartsTmp1->byViewType = pPartsTmp2->byViewType;
		pPartsTmp1->byHitType = pPartsTmp2->byHitType;
		pPartsTmp1->byAnimeType = pPartsTmp2->byAnimeType;
		pPartsTmp1->byAnimeCount = pPartsTmp2->byAnimeCount;
		pPartsTmp1->byLevel = pPartsTmp2->byLevel;
		pPartsTmp1->byLight = pPartsTmp2->byLight;
		pPartsTmp1->wGrpID = pPartsTmp2->wGrpID;
		pPartsTmp1->wBackID = pPartsTmp2->wBackID;
		pPartsTmp1->pAnimeInfo = pPartsTmp2->pAnimeInfo;
	}

	ZeroMemory (&pParts[g_wCountMapParts], sizeof (MAPPARTSINFO));
	SAFE_DELETE_ARRAY (g_aMapPartsInfo);
	g_aMapPartsInfo = pParts;
	g_wCountMapParts ++;
	SendCmdMAPPARTS (URARASOCK_SENDALL, g_wCountMapParts - 1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPPARTSADDANIME										 */
/* 内容：受信データ処理(マップパーツアニメ追加)								 */
/* 日付：2003/07/03															 */
/* ========================================================================= */
void RecvDataProcMAPPARTSADDANIME(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_aMapPartsInfo[pPara->dwData];
	PANIMEINFO pAnimeTmp;

	pParts->byAnimeCount ++;
	pAnimeTmp = new ANIMEINFO[pParts->byAnimeCount];
	ZeroMemory (pAnimeTmp, sizeof (ANIMEINFO) * pParts->byAnimeCount);
	if (pParts->pAnimeInfo) {
		CopyMemory (pAnimeTmp, pParts->pAnimeInfo, sizeof (ANIMEINFO) * (pParts->byAnimeCount - 1));
		SAFE_DELETE_ARRAY (pParts->pAnimeInfo);
	}
	pParts->pAnimeInfo = pAnimeTmp;

	SendCmdMAPPARTS (URARASOCK_SENDALL, pPara->dwData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGMAP												 */
/* 内容：受信データ処理(マップ更新)											 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void RecvDataProcCHGMAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGMAP pMap = (PCMDPARA_CHGMAP)pData;
	int nNo = GetMapNo (pMap->dwID);
	if (nNo < 0) {
		return;
	}

	/* マップの外は書き換えない */
	if ((pMap->nX < 0) || (pMap->nY < 0) || (pMap->nX >= g_aMapInfo[nNo]->nCX) || (pMap->nY >= g_aMapInfo[nNo]->nCY) ||
			(g_aMapInfo[nNo]->pwMap == NULL)) {
		return;
	}
	g_aMapInfo[nNo]->pwMap[pMap->nX + pMap->nY * g_aMapInfo[nNo]->nCX] = pMap->wMap;
	SendCmdCHGMAP (pMap->dwID, pMap->wMap, pMap->nX, pMap->nY);
}

/* ========================================================================= */
/* 関数名：RecvDataProcRELOADPARTS											 */
/* 内容：受信データ処理(マップパーツ更新要求)								 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void RecvDataProcRELOADPARTS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA2 pPara = (PCMDPARA_DATA2)pData;
	int nNo = pPara->dwTblID;
	if (nNo < 0) {
		return;
	}

	SendCmdMAPPARTS (pPara->pID, pPara->nData);
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

	pPartsTmp = &g_aMapPartsInfo[pParts->wCount];
	if (pPartsTmp->pAnimeInfo) {
		SAFE_DELETE_ARRAY (pPartsTmp->pAnimeInfo);
	}

	pRecv = (PMAPPARTSINFOSAVE)(pData + sizeof (CMDPARA_MAPPARTSINFO));
	pPartsTmp->byViewType = pRecv->byViewType;
	pPartsTmp->byHitType = pRecv->byHitType;
	pPartsTmp->byAnimeType = pRecv->byAnimeType;
	pPartsTmp->byAnimeCount = pRecv->byAnimeCount;
	pPartsTmp->byLevel = pRecv->byLevel;
	pPartsTmp->byLight = pRecv->byLight;
	pPartsTmp->wGrpID = pRecv->wGrpID;
	pPartsTmp->wBackID = pRecv->wBackID;
	pPartsTmp->pAnimeInfo = pRecv->pAnimeInfo;
	pRecv ++;
	if (pPartsTmp->pAnimeInfo) {
		pPartsTmp->pAnimeInfo = new ANIMEINFO[pPartsTmp->byAnimeCount];
		CopyMemory (pPartsTmp->pAnimeInfo, pRecv, pPartsTmp->byAnimeCount * sizeof (ANIMEINFO));
	}

	SendCmdMAPPARTS (URARASOCK_SENDALL, pParts->wCount);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDDELMAP											 */
/* 内容：受信データ処理(マップ幅増減)										 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void RecvDataProcADDDELMAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADDDELMAP pMap = (PCMDPARA_ADDDELMAP)pData;
	int i, nSize, nTmp1, nTmp2, nNo = GetMapNo (pMap->dwID);
	WORD *pNewMap;
	DWORD *pdwNewMap;

	if (nNo < 0) {
		return;
	}

	switch (pMap->byType) {
	case 0:	/* 上 */
		if (g_aMapInfo[nNo]->nCY + pMap->byCount >= 256) {
			break;
		}
		nSize = g_aMapInfo[nNo]->nCX * (g_aMapInfo[nNo]->nCY + pMap->byCount);
		nTmp1 = g_aMapInfo[nNo]->nCX * pMap->byCount;
		nTmp2 = g_aMapInfo[nNo]->nCX * g_aMapInfo[nNo]->nCY;

		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		CopyMemory (pNewMap + nTmp1, g_aMapInfo[nNo]->pwMap, nTmp2 * sizeof (WORD));
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pwMap);
		g_aMapInfo[nNo]->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		CopyMemory (pdwNewMap + nTmp1, g_aMapInfo[nNo]->pdwMapChar, nTmp2 * sizeof (DWORD));
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pdwMapChar);
		g_aMapInfo[nNo]->pdwMapChar = pdwNewMap;
/*
		PBYTE pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		CopyMemory (pbyNewMap + nTmp1, g_aMapInfo[nNo]->pbyRoofLV, nTmp2);
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pbyRoofLV);
		g_aMapInfo[nNo]->pbyRoofLV = pbyNewMap;
*/
		g_aMapInfo[nNo]->nCY += pMap->byCount;
		SendCmdADDDELMAP (pMap->dwID, pMap->byType, pMap->byCount);
		RenewDataPos (pMap->dwID, 0, pMap->byCount);
		break;

	case 1:	/* 下 */
		if (g_aMapInfo[nNo]->nCY + pMap->byCount >= 256) {
			break;
		}
		nSize = g_aMapInfo[nNo]->nCX * (g_aMapInfo[nNo]->nCY + pMap->byCount);
		nTmp1 = g_aMapInfo[nNo]->nCX * g_aMapInfo[nNo]->nCY;

		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		CopyMemory (pNewMap, g_aMapInfo[nNo]->pwMap, nTmp1 * sizeof (WORD));
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pwMap);
		g_aMapInfo[nNo]->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		CopyMemory (pdwNewMap, g_aMapInfo[nNo]->pdwMapChar, nTmp1 * sizeof (DWORD));
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pdwMapChar);
		g_aMapInfo[nNo]->pdwMapChar = pdwNewMap;
/*
		PBYTE pbyNewMap = new BYTE[nSize];
		ZeroMemory (pbyNewMap, nSize);
		CopyMemory (pbyNewMap, g_aMapInfo[nNo]->pbyRoofLV, nTmp1);
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pbyRoofLV);
		g_aMapInfo[nNo]->pbyRoofLV = pbyNewMap;
*/
		g_aMapInfo[nNo]->nCY += pMap->byCount;
		SendCmdADDDELMAP (pMap->dwID, pMap->byType, pMap->byCount);
		break;

	case 2:	/* 左 */
		if (g_aMapInfo[nNo]->nCX + pMap->byCount >= 256) {
			break;
		}
		nSize = (g_aMapInfo[nNo]->nCX + pMap->byCount) * g_aMapInfo[nNo]->nCY;
		nTmp1 = g_aMapInfo[nNo]->nCX + pMap->byCount;

		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		for (i = 0; i < g_aMapInfo[nNo]->nCY; i ++) {
			CopyMemory (pNewMap + pMap->byCount + nTmp1 * i,
					g_aMapInfo[nNo]->pwMap + g_aMapInfo[nNo]->nCX * i,
					g_aMapInfo[nNo]->nCX * sizeof (WORD));
		}
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pwMap);
		g_aMapInfo[nNo]->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		for (i = 0; i < g_aMapInfo[nNo]->nCY; i ++) {
			CopyMemory (pdwNewMap + pMap->byCount + nTmp1 * i,
					g_aMapInfo[nNo]->pdwMapChar + g_aMapInfo[nNo]->nCX * i,
					g_aMapInfo[nNo]->nCX * sizeof (DWORD));
		}
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pdwMapChar);
		g_aMapInfo[nNo]->pdwMapChar = pdwNewMap;

		g_aMapInfo[nNo]->nCX += pMap->byCount;
		SendCmdADDDELMAP (pMap->dwID, pMap->byType, pMap->byCount);
		RenewDataPos (pMap->dwID, pMap->byCount, 0);
		break;

	case 3:	/* 右 */
		if (g_aMapInfo[nNo]->nCX + pMap->byCount >= 256) {
			break;
		}
		nSize = (g_aMapInfo[nNo]->nCX + pMap->byCount) * g_aMapInfo[nNo]->nCY;
		nTmp1 = g_aMapInfo[nNo]->nCX + pMap->byCount;

		pNewMap = new WORD[nSize];
		ZeroMemory (pNewMap, nSize * sizeof (WORD));
		for (i = 0; i < g_aMapInfo[nNo]->nCY; i ++) {
			CopyMemory (pNewMap + nTmp1 * i,
					g_aMapInfo[nNo]->pwMap + g_aMapInfo[nNo]->nCX * i,
					g_aMapInfo[nNo]->nCX * sizeof (WORD));
		}
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pwMap);
		g_aMapInfo[nNo]->pwMap = pNewMap;

		pdwNewMap = new DWORD[nSize];
		ZeroMemory (pdwNewMap, nSize * sizeof (DWORD));
		for (i = 0; i < g_aMapInfo[nNo]->nCY; i ++) {
			CopyMemory (pdwNewMap + nTmp1 * i,
					g_aMapInfo[nNo]->pdwMapChar + g_aMapInfo[nNo]->nCX * i,
					g_aMapInfo[nNo]->nCX * sizeof (DWORD));
		}
		SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pdwMapChar);
		g_aMapInfo[nNo]->pdwMapChar = pdwNewMap;

		g_aMapInfo[nNo]->nCX += pMap->byCount;
		SendCmdADDDELMAP (pMap->dwID, pMap->byType, pMap->byCount);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSAVEMAP												 */
/* 内容：受信データ処理(マップ保存)											 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void RecvDataProcSAVEMAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	SendCmdSYSMSG (URARASOCK_SENDALL, "SYSTEM:サーバー情報を保存します。");
	SaveMapPartsInfo ();
	SaveMapInfo ();
	SaveCharInfo ();
	SaveComEfcInfo ();
	SaveItemInfo ();
	SaveEfcInfo ();
	SaveStTmpInfo ();
	SaveLeaveInfo ();
	SaveSubGameInfo ();
	SaveJobInfo ();
	SaveMagicUserInfo ();
	SaveMagicInfo ();
	SaveEquUserInfo ();
	WebThreadRenew ();

	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];
	SendCmdSYSMSG (URARASOCK_SENDALL, "SYSTEM:サーバー情報を保存しました。");
}

/* ========================================================================= */
/* 関数名：RecvDataProcEVENTADD												 */
/* 内容：受信データ処理(イベント情報追加)									 */
/* 日付：2003/06/02															 */
/* ========================================================================= */
void RecvDataProcEVENTADD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_EVENT pEvent = (PCMDPARA_EVENT)pData;
	int nNo = GetMapNo (pEvent->dwID);
	if (nNo < 0) {
		return;
	}
	PBYTE pEventNew = new BYTE[(g_aMapInfo[nNo]->nEventCount + 1) * sizeof (EVENTINFO)];
	ZeroMemory (pEventNew, (g_aMapInfo[nNo]->nEventCount + 1) * sizeof (EVENTINFO));
	if (g_aMapInfo[nNo]->nEventCount > 0) {
		CopyMemory (pEventNew, g_aMapInfo[nNo]->pEvent, g_aMapInfo[nNo]->nEventCount * sizeof (EVENTINFO));
	}

	SAFE_DELETE_ARRAY (g_aMapInfo[nNo]->pEvent);
	g_aMapInfo[nNo]->pEvent = (PEVENTINFO)pEventNew;
	g_aMapInfo[nNo]->nEventCount ++;

	/* まず登録数を送信 */
	SendCmdEVENTCOUNT (pEvent->pID, pEvent->dwID);

	SendCmdEVENTINFO (0, nNo, g_aMapInfo[nNo]->nEventCount - 1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEVENTGET												 */
/* 内容：受信データ処理(イベント情報取得)									 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void RecvDataProcEVENTGET(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_EVENT pEvent = (PCMDPARA_EVENT)pData;
	int nNo;

	nNo = GetMapNo (pEvent->dwID);
	if (nNo < 0) {
		return;
	}

	if (pEvent->nData < 0) {
		pEvent->nData = SearchEvent (pEvent->dwID, pEvent->dwX, pEvent->dwY);
		if (pEvent->nData < 0) {
			return;
		}
	}


	/* まず登録数を送信 */
	SendCmdEVENTCOUNT (pEvent->pID, pEvent->dwID);

	if ((g_aMapInfo[nNo]->nEventCount == 0) ||
		(g_aMapInfo[nNo]->nEventCount < pEvent->nData)) {
		return;
	}
	SendCmdEVENTINFO (pEvent->pID, nNo, pEvent->nData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEVENTCHG												 */
/* 内容：受信データ処理(イベント情報更新)									 */
/* 日付：2003/06/03															 */
/* ========================================================================= */
void RecvDataProcEVENTCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGEVENT pEvent = (PCMDPARA_CHGEVENT)pData;
	PEVENTINFO pEventData;
	PMAPINFO pMap;
	int nSize, nNo = GetMapNo (pEvent->dwID);
	if (nNo < 0) {
		return;
	}
	pMap = g_aMapInfo[nNo];

	if (pEvent->dwDataID >= (DWORD)pMap->nEventCount) {
		return;
	}

	pEventData = &pMap->pEvent[pEvent->dwDataID];

	if (pEventData->pData) {
		SAFE_DELETE_ARRAY (pEventData->pData);
	}

	pEventData->dwX = pEvent->dwX;
	pEventData->dwY = pEvent->dwY;
	pEventData->byType = pEvent->byType;
	pEventData->pData = pEvent->pData;

	/* 詳細データがある？ */
	if (pEvent->pData) {
		nSize = GetEventDataSize (pEvent->byType);
		pEventData->pData = new BYTE[nSize];
		CopyMemory ((PBYTE)pEventData->pData, (PBYTE)pEvent + sizeof (CMDPARA_CHGEVENT), nSize);

	/* 詳細データが無い */
	} else {
		if ((pEventData->dwX == 0) && (pEventData->dwY == 0)) {
			DeleteEvent (nNo, pEvent->dwDataID);
			return;
		}
	}

	/* イベント座標を通知 */
	SendCmdEVENTPOS (URARASOCK_SENDALL, (DWORD)nNo);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPINFOCHG											 */
/* 内容：受信データ処理(マップ情報更新)										 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void RecvDataProcMAPINFOCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGMAPINFO pPara = (PCMDPARA_CHGMAPINFO)pData;
	int nNo = GetMapNo (pPara->dwID);
	if (nNo < 0) {
		return;
	}

	PMAPINFO pMap = g_aMapInfo[nNo];
	BYTE byData = pMap->byBGM;
	BOOL bChgColor = FALSE;
	if (pPara->byBGM < SOUND_MAX) {
		pMap->byBGM = pPara->byBGM;
	}
	if (pMap->clChg != pPara->clChg) {
		bChgColor = TRUE;
	}
	pMap->nStartX = pPara->nStartX;
	pMap->nStartY = pPara->nStartY;
	pMap->nEntranceX = pPara->nEntranceX;
	pMap->nEntranceY = pPara->nEntranceY;
	pMap->byFlg = pPara->byFlg;
	pMap->byLightLevel = pPara->byLightLevel;
	pMap->byWeather	= pPara->byWeather;
	pMap->clChg = pPara->clChg;
	pMap->byDarkAreaCount = min (pPara->byDarkAreaCount, MAX_MAPDARKAREA);
	pMap->byViewRange = min (pPara->byViewRange, (BYTE)10);
	pMap->wViewHideParts = (pPara->wViewHideParts < g_wCountMapParts) ? pPara->wViewHideParts : 0;
	CopyMemory (pMap->aDarkArea, pPara->aDarkArea, sizeof (pMap->aDarkArea));

	/* BGMが変更された？ */
	if (byData != pPara->byBGM) {
		SendCmdCHGBGM (URARASOCK_SENDALL, pPara->dwID, pPara->byBGM);
	}
	/* 色変更あり？ */
	if (bChgColor) {
		SendCmdCHGCOLOR (nNo);
	}

	SAFE_DELETE_ARRAY (pMap->pszName);
	pPara ++;
	if (strlen ((LPSTR)pPara)) {
		pMap->pszName = new char[strlen ((LPSTR)pPara) + 1];
		ZeroMemory (pMap->pszName, strlen ((LPSTR)pPara) + 1);
		strcat ((LPSTR)pMap->pszName, (LPSTR)pPara);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcEVENTCHECK											 */
/* 内容：受信データ処理(イベントチェック)									 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void RecvDataProcEVENTCHECK(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PMAPINFO pMap;
	PCHARINFO pChar;
	PEVENTINFO pEvent;
	int i, nNo;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	/* 誰かについていってる？ */
	if (pChar->dwTargetTblID3 != -1) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}

	nNo = GetMapNo (pChar->dwMapID);
	if (nNo < 0) {
		return;
	}
	pMap = g_aMapInfo[nNo];
	pEvent = pMap->pEvent;
	if (!pEvent) {
		return;
	}

	DWORD dwX, dwY;
	for (i = 0; i < pMap->nEventCount; i ++) {
		pEvent = &pMap->pEvent[i];
		dwX = pChar->dwX;
		dwY = pChar->dwY;

		if ((pEvent->byType == EVENTTYPE_MSG) && (pPara->byData == 2)) {
			dwX += anPosAddTblX[pChar->byDirection];
			dwY += anPosAddTblY[pChar->byDirection];
		}
		/* イベント発見？ */
		if ((pEvent->dwX == dwX) && (pEvent->dwY == dwY)) {
			EventProc (pChar, pEvent, pPara->byData);
			return;
		}
	}

	/* 目の前を調べた時（Sキー）は、目の前の扉なら開ける */
	if (pPara->byData == 2) {
		if (OpenDoorFront (pChar)) {
			return;
		}
	}
	/* マスに乗った時の問い合わせ(0)は、調べたわけではないのでメッセージは出さない。
	   ついていく時はサーバー側で先に次のマスへ動かしているので、扉などのマスに乗った
	   問い合わせが届く頃には足元にイベントが無く、ここに来ることがある。
	   位置を送り直してクライアントの移動待ちを解く */
	if (pPara->byData == 0) {
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}
	SendCmdMAPMSG (pChar->pID, "何も見つかりませんでした。@2");
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDCHAR												 */
/* 内容：受信データ処理(キャラ追加要求)										 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void RecvDataProcADDCHAR(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_ADDCHARINFO Para;
	PCMDPARA_ADDCHARINFO pChar = &Para;
	int nNo;

	/* 名前は可変長で届くので構造体に戻す */
	if (SboPackGetADDCHARINFO (&Para, pData, RecvEnd (pData)) == NULL) {
		return;
	}

	nNo = GetMapNo (pChar->dwMapID);
	if (nNo < 0) {
		return;
	}

	PCHARINFO pCharTmp = new CHARINFO;
	ZeroMemory ((PBYTE)pCharTmp, sizeof (CHARINFO));
	pCharTmp->byNpcType = pChar->byNpcType;
	SetInitCharData (pCharTmp);
	pCharTmp->wGrpID = pChar->wGrpID;
	pCharTmp->dwMapID = pChar->dwMapID;
	pCharTmp->dwX = pChar->dwX;
	pCharTmp->dwY = pChar->dwY;
	pCharTmp->dwXBack = pChar->dwX;
	pCharTmp->dwYBack = pChar->dwY;
	pCharTmp->dwStartMapID = pChar->dwMapID;
	pCharTmp->dwStartX = pChar->dwX;
	pCharTmp->dwStartY = pChar->dwY;
	pCharTmp->byDirection = pChar->byDirection;
	pCharTmp->byGameState = GAMESTATE_NPC;
	strcat (pCharTmp->szName, pChar->szName);

	pCharTmp = AddChar (pCharTmp);
	pCharTmp->pID = pCharTmp->dwTblID + 1;
	CharNpcIdEnsure (pCharTmp);
	RenewATDF (pCharTmp->dwTblID);
	SendCmdCHARINFO3 (pCharTmp->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcBANCHAR												 */
/* 内容：受信データ処理(キャラ強制ログオフ要求)								 */
/* 日付：2003/06/09															 */
/* ========================================================================= */
void RecvDataProcBANCHAR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARINFO												 */
/* 内容：受信データ処理(キャラ情報)											 */
/* 日付：2003/06/10															 */
/* ========================================================================= */
void RecvDataProcCHARINFO(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_CHARINFO Para;
	PCMDPARA_CHARINFO pChar = &Para;
	PCHARINFO pCharDst;
	PBYTE pDataNext;

	/* 名前は可変長で届くので構造体に戻す（後ろに NPC 発言が続く） */
	pDataNext = SboPackGetCHARINFO (&Para, pData, RecvEnd (pData));
	if (pDataNext == NULL) {
		return;
	}
	if (Para.pszNpcSpeech && !IsRecvStr (pDataNext)) {
		return;
	}

	if (pChar->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pCharDst = (PCHARINFO)g_aCharInfo[pChar->dwTblID];

	if (pCharDst->dwMapID != pChar->dwMapID) {
		/* 別マップへは移動させない */
		return;
	}

	pCharDst->byAdminLV = pChar->byAdminLV;
	pCharDst->byCharState = pChar->byCharState;
	pCharDst->byDirection = pChar->byDirection;
	pCharDst->byRoofLV = pChar->byRoofLV;
	if (pCharDst->byGameState == GAMESTATE_NPC) {
		/* 変更された？ */
		if (pCharDst->byNpcType != pChar->byNpcType) {
			CCharProcBase CharProcBase;

			SAFE_DELETE (pCharDst->pProc);

			pCharDst->byNpcType = pChar->byNpcType;
			CharProcBase.SetProc (pCharDst);
		}
		pCharDst->byNpcType = pChar->byNpcType;
		pCharDst->dwStTmplateID = pChar->dwStTmplateID;
	}
	pCharDst->wGrpID = pChar->wGrpID;
	pCharDst->wGrpMain = pChar->wGrpMain;
	pCharDst->wGrpSub = pChar->wGrpSub;
	pCharDst->dwMapID = pChar->dwMapID;
	/* (0,0)は禁止。 */
	if (!((pChar->dwStartX == 0) && (pChar->dwStartY == 0))) {
		pCharDst->dwStartMapID = pChar->dwStartMapID;
		pCharDst->dwStartX = pChar->dwStartX;
		pCharDst->dwStartY = pChar->dwStartY;
	}
	pCharDst->dwX = pChar->dwX;
	pCharDst->dwY = pChar->dwY;
	pCharDst->dwXBack = pChar->dwX;
	pCharDst->dwYBack = pChar->dwY;
	pCharDst->byAreaX = pChar->byAreaX;
	pCharDst->byAreaY = pChar->byAreaY;
	pCharDst->byPutCount = pChar->byPutCount;
	pCharDst->byMoveProbabilityX = pChar->byMoveProbabilityX;
	pCharDst->byMoveProbabilityY = pChar->byMoveProbabilityY;
	pCharDst->bySearchCx = pChar->bySearchCx;
	pCharDst->bySearchCy = pChar->bySearchCy;
	pCharDst->wTimePutInterval = pChar->wTimePutInterval;
	pCharDst->clName = pChar->clName;
	pCharDst->clSpeech = pChar->clSpeech;
	CopyMemory (pCharDst->szName, pChar->szName, sizeof (pCharDst->szName));
	if (pChar->pszNpcSpeech) {
		LPSTR pszTmp = (LPSTR)pDataNext;
		int nSize = strlen (pszTmp) + 1;

		SAFE_DELETE_ARRAY (pCharDst->pszNpcSpeech);
		pCharDst->pszNpcSpeech = new char[nSize];
		CopyMemory (pCharDst->pszNpcSpeech, pszTmp, nSize);
	}

	if (pCharDst->byNpcType) {
		pCharDst->byGameState = GAMESTATE_NPC;
	}

	SendCmdCHARINFO3 (pChar->dwTblID);
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 1);

	/* NPC発生（発生元）なら、もう出ているキャラにも名前・見た目・テンプレートを反映 */
	RenewPutChildChar (pCharDst);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDMAP												 */
/* 内容：受信データ処理(マップ追加要求)										 */
/* 日付：2003/07/06															 */
/* ========================================================================= */
void RecvDataProcADDMAP(PBYTE pData, DWORD dwDataSize)
{
	int i;
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	PMAPINFO pMap = new MAPINFO;
	ZeroMemory (pMap, sizeof (MAPINFO));
	pMap->dwID = g_dwNewMapID;
	g_dwNewMapID ++;
	pMap->nCX = 10;
	pMap->nCY = 10;
	pMap->nStartX = 0;
	pMap->nStartY = 0;
	pMap->pwMap = new WORD [pMap->nCX * pMap->nCY];
	PBYTE pMapData = (PBYTE)pMap->pwMap;
	for (i = 0; i < pMap->nCX * pMap->nCY; i ++) {
		pMap->pwMap[i] = 1;
	}
	pMap->pdwMapChar = new DWORD [pMap->nCX * pMap->nCY];
	ZeroMemory ((PBYTE )pMap->pdwMapChar, pMap->nCX * pMap->nCY * sizeof (DWORD));
	g_aMapInfo.Add (pMap);

	char szTmp[128];
	wsprintf (szTmp, "SYSTEM:マップID%dを追加しました。", pMap->dwID);
	SendCmdSYSMSG (s_pIDRecv, szTmp, SPEECHCMD_BATTLE1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDMSG												 */
/* 内容：受信データ処理(メッセージ追加)										 */
/* 日付：2003/07/15															 */
/* ========================================================================= */
void RecvDataProcADDMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	DWORD dwMapID = g_aCharInfo[pPara->dwTblID]->dwMapID;
	PMAPINFO pMap = g_aMapInfo[dwMapID];

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
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAPMSGREQ											 */
/* 内容：受信データ処理(メッセージ要求)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void RecvDataProcMAPMSGREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];

	PMAPINFO pMap = g_aMapInfo[GetMapNo (pChar->dwMapID)];
	if (pMap->nMsgCount <= (int)pPara->dwData) {
		return;
	}

	LPSTR pMsg = pMap->pMsg[pPara->dwData];
	SendCmdMAPMSGREQ (pChar->pID, pMsg);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGMSG												 */
/* 内容：受信データ処理(メッセージ更新)										 */
/* 日付：2003/07/21															 */
/* ========================================================================= */
void RecvDataProcCHGMSG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	DWORD dwMapID;

	/* 送ってきたキャラのいるマップ（「元に戻す」の時は記録したマップ） */
	if (s_dwRecvMapNo != (DWORD)-1) {
		dwMapID = s_dwRecvMapNo;
	} else {
		if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
			return;
		}
		dwMapID = g_aCharInfo[pPara->dwTblID]->dwMapID;
	}
	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	PMAPINFO pMap = g_aMapInfo[dwMapID];

	if (pPara->dwData >= (DWORD)pMap->nMsgCount) {
		return;
	}

	LPSTR pMsg, pNewMsg;
	pMsg = (LPSTR)pData + sizeof (CMDPARA_DATA3);
	pNewMsg = new char[strlen ((LPSTR)pMsg) + 1];
	ZeroMemory (pNewMsg, strlen ((LPSTR)pMsg) + 1);
	strcat ((LPSTR)pNewMsg, (LPSTR)pMsg);

	SAFE_DELETE_ARRAY (pMap->pMsg[pPara->dwData]);
	pMap->pMsg[pPara->dwData] = pNewMsg;
}

/* ========================================================================= */
/* 関数名：RecvDataProcOPENDOOR												 */
/* 内容：受信データ処理(扉開け要求)											 */
/* 日付：2003/07/23															 */
/* ========================================================================= */
void RecvDataProcOPENDOOR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (GetMapNo (pChar->dwMapID) < 0) {
		return;
	}
	if (OpenDoorFront (pChar) == FALSE) {
		SendCmdMAPMSG (pChar->pID, "そこには扉がありません。@2");
	}
}

/* ========================================================================= */
/* 関数名：OpenDoorFront													 */
/* 内容：目の前の扉を開ける													 */
/*		 戻り値：開けた時 TRUE（扉が無い・もう開いている時は何もせず FALSE）	 */
/*		 「扉」コマンドと、Sキーで目の前を調べた時の両方から使う				 */
/* ========================================================================= */
static BOOL OpenDoorFront(PCHARINFO pChar)
{
	PMAPINFO pMap;
	PEVENTINFO pEvent;
	DWORD dwX, dwY;
	int i, nNo;

	nNo = GetMapNo (pChar->dwMapID);
	if (nNo < 0) {
		return FALSE;
	}
	pMap = g_aMapInfo[nNo];
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	for (i = 0; i < pMap->nEventCount; i ++) {
		pEvent = &pMap->pEvent[i];

		/* イベント発見？ */
		if ((pEvent->dwX == dwX) && (pEvent->dwY == dwY)) {
			if (pEvent->byType != EVENTTYPE_DOOR) {
				continue;
			}
/* とりあえず鍵は無しで。 */
			PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEvent->pData;
			/* すでに開いてる？ */
			if (pDoor->dwTimeOpen) {
				return FALSE;
			}

			pDoor->dwTimeOpen = timeGetTime ();
			SendCmdCHGMAP (pChar->dwMapID, pDoor->wBackGrpID, dwX, dwY);
			SendCmdPLAYSOUND (pChar->pID, SOUND_DOOR);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGCHARGRP											 */
/* 内容：受信データ処理(着せ替え要求)										 */
/* 日付：2003/07/24															 */
/* ========================================================================= */
void RecvDataProcCHGCHARGRP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];

	/* 透明キャラに変わろうとしている？ */
	if (pPara->dwData == 0) {
		/* 何かおかしいので強制ログアウト */
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, pPara->dwTblID);
		return;
	}

	pChar->wGrpID = (WORD)pPara->dwData;
	SendCmdCHARINFO3 (pPara->dwTblID);
	SendCmdCHARINFO (pChar->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGCHARGRPBACK										 */
/* 内容：受信データ処理(キャラ画像記録)										 */
/* 日付：2004/04/03															 */
/* ========================================================================= */
void RecvDataProcCHGCHARGRPBACK(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (pPara->dwData == 0) {
		pChar->wGrpIDBack = pChar->wGrpID;
	} else {
		if (!CheckMsgCode (pChar, '9', (int)pPara->dwData, 0, 0)) {
			return;
		}
		pChar->wGrpIDBack = (WORD)pPara->dwData;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGNAMESEX											 */
/* 内容：受信データ処理(名前と性別更新)										 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
void RecvDataProcCHGNAMESEX(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_NAMESEX Para;
	PCMDPARA_NAMESEX pPara = &Para;
	PCHARINFO pChar;
	BOOL bName = TRUE;

	/* 名前は可変長で届くので構造体に戻す */
	if (SboPackGetNAMESEX (&Para, pData, RecvEnd (pData)) == NULL) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	/* 名前は表示幅 14（半角 14 文字・全角 7 文字）まで（クライアントと同じ決まり。必ず終端させる） */
	TermRecvStr (pPara->szName, sizeof (pPara->szName));
	pPara->szName[SboTextFitWidth (pPara->szName, MAX_CHARNAME_WIDTH)] = 0;
	SboTextSanitize (pPara->szName);
	/* 名前に使えない文字（絵文字など Shift_JIS に無い文字）が入っていたら、同名と同じく断る */
	if ((pPara->szName[0] != 0) && !SboTextIsNameSafe (pPara->szName)) {
		pPara->szName[0] = 0;
		bName = FALSE;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	pChar->bySaveFlg &= ~CHARINFOSAVEFLG_FEMALE;
	if (pPara->bySex) {
		pChar->bySaveFlg |= CHARINFOSAVEFLG_FEMALE;
	}

	int i;
	if (pPara->szName[0] != 0) {
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			PCHARINFO pCharCheck = g_aCharInfo[i];
			/* 同じ長さ？ */
			if (strlen (pCharCheck->szName) == strlen (pPara->szName)) {
				/* 同名？ */
				if (strcmp (pCharCheck->szName, pPara->szName) == 0) {
					pPara->szName[0] = 0;
					bName = FALSE;
					break;
				}
			}
		}
	}

	ZeroMemory (pChar->szName, sizeof (pChar->szName));
	strcat (pChar->szName, pPara->szName);

	/* キャラ削除？ */
	if (pChar->szName[0] == 0 && bName) {
		PCHARINFO pCharNew = new CHARINFO;
		ZeroMemory (pCharNew, sizeof (CHARINFO));
		SetInitCharData (pCharNew);
		RenewATDF (pCharNew->dwTblID);
		pCharNew->pID = pChar->pID;
		pCharNew->dwTblID = pChar->dwTblID;
		pCharNew->byAdminLV = pChar->byAdminLV;
		strcat (pCharNew->szAcc, pChar->szAcc);
		strcat (pCharNew->szPas, pChar->szPas);
		delete pChar;
		g_aCharInfo[pPara->dwTblID] = pCharNew;

	} else {
		SYSTEMTIME sysTime;
		GetLocalTime (&sysTime);
		pChar->wMakeYear = sysTime.wYear;
		pChar->wMakeMonth = sysTime.wMonth;
		pChar->wMakeDay = sysTime.wDay;
	}

	if (bName) {
		SendCmdCHARINFO (pPara->pID, pPara->dwTblID);

	/* 同名がすでにある？ */
	} else {
		if (pChar->pID) {
			SendCmdERR (pChar->pID, SBOERR_NAME);
			SendCmdCHARINFO (pChar->pID, -1);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcGMCALL												 */
/* 内容：受信データ処理(管理人呼び出し)										 */
/* 日付：2003/08/04															 */
/* ========================================================================= */
void RecvDataProcGMCALL(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pCharGM, pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];

	SYSTEMTIME sysTime;
	LPSTR szTmp = new char[256], szTmp2 = new char[256];
	ZeroMemory (szTmp, 256);
	ZeroMemory (szTmp2, 256);

	GetLocalTime (&sysTime);
	wsprintf (szTmp, "GMCALL:%02d/%02d %02d:%02d",
			sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute);
	wsprintf (szTmp2, "       %s(MAP:%ld X:%ld Y:%ld)",
			pChar->szName, pChar->dwMapID, pChar->dwX, pChar->dwY);

	int i, nGMCount = 0;
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		pCharGM = g_aCharInfo[i];
		if (pCharGM->byAdminLV > ADMINLV_USER) {
			if (pCharGM->byGameState >= GAMESTATE_LOGIN) {
				if (pCharGM->byAccState & ACCSTATE_ADMINON) {
					SendCmdSYSMSG (pCharGM->pID, szTmp);
					SendCmdSYSMSG (pCharGM->pID, szTmp2);
					SendCmdPLAYSOUND (pCharGM->pID, SOUND_LVUP);
					nGMCount ++;
				}
			}
		}
	}
	if (nGMCount == 0) {
		SendCmdSYSMSG (pPara->pID, "SYSTEM:管理人は現在不在の為応答できません。");
	}
	delete [] szTmp;
	delete [] szTmp2;
}

/* ========================================================================= */
/* 関数名：RecvDataProcEFECTCHG												 */
/* 内容：受信データ処理(エフェクト変更)										 */
/* 日付：2004/11/21															 */
/* ========================================================================= */
void RecvDataProcEFECTCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (pChar->pID == 0) {
		return;
	}

	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pPara->byData, (BOOL)pPara->dwData);
}


/* ========================================================================= */
/* 関数名：	RecvDataProcRESURRECTION										 */
/* 内容：	受信データ処理(蘇生)											 */
/* 日付：	2005/11/01														 */
/* ========================================================================= */

void RecvDataProcRESURRECTION(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (pChar->pID == 0) {
		return;
	}

	/* 復活位置で蘇生 */
	ResurrectionBack (pChar);
}


/* ========================================================================= */
/* 関数名：	RecvDataProcREQ_ATTACK											 */
/* 内容：	受信データ処理(攻撃要求)										 */
/* 日付：	2006/02/26														 */
/* ========================================================================= */

/* ゲージの%から威力の倍率を求める（溜まりきった周の数×20%、20～100%） */
static BYTE GetAtackRateLap(BYTE byTurnLV)
{
	int nLap = byTurnLV / TURNGAGE_LAP;

	nLap = max (nLap, 1);
	nLap = min (nLap, TURNGAGE_LAPMAX);
	return (BYTE)(nLap * TURNGAGE_LAP);
}

/* 攻撃要求の威力欄から威力の倍率を求める。ジャストは、前の攻撃要求から
   ゲージが満タンになる時間が経っている時だけ認める（改造クライアントで毎回ジャストにできないように） */
static BYTE GetAtackRateReq(PCHARINFO pChar, BYTE byData)
{
	static std::map<DWORD, DWORD> s_mapLastReq;		/* キャラごとの前の攻撃要求の時間 */
	DWORD dwNow, dwLast, dwGage;
	int nTmp;
	BYTE byRet;
	std::map<DWORD, DWORD>::iterator it;

	dwNow	= timeGetTime ();
	dwLast	= 0;
	it		= s_mapLastReq.find (pChar->dwTblID);
	if (it != s_mapLastReq.end ()) {
		dwLast = it->second;
	}
	s_mapLastReq[pChar->dwTblID] = dwNow;

	byRet = GetAtackRateLap (byData);
	if (byData != ATACKRATE_JUST) {
		return byRet;
	}
	/* ゲージが満タンになるまでの時間（クライアントと同じ計算。素早さ 0 で 3 秒～255 で 1 秒） */
	nTmp = pChar->byDEX;
	nTmp = nTmp * 100 / 255;
	nTmp = max (nTmp, 1);
	dwGage = 1000 + (2000 - ((2000 * nTmp) / 100));
	/* 加速していれば満タンまでの時間も短い */
	dwGage = dwGage * 100 / (100 + GetHasteRate (pChar));
	/* 初めての攻撃か、満タンになる時間が経っている（通信のずれは 0.25 秒まで見る） */
	if ((dwLast == 0) || (dwNow - dwLast + 250 >= dwGage)) {
		byRet = ATACKRATE_JUST;
	}
	return byRet;
}

/* ========================================================================= */
/* 関数名：	RecvDataProcBATTLEMODE											 */
/* 内容：	受信データ処理(戦闘モードの切り替え通知)						 */
/*			byData 1=戦闘モード 0=解除。傭兵が自分から戦うかに使う			 */
/* ========================================================================= */

void RecvDataProcBATTLEMODE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	MercenarySetBattleMode (g_aCharInfo[pPara->dwTblID], pPara->byData != 0);
}


/* ========================================================================= */
/* 関数名：	RecvDataProcREQ_GUARD											 */
/* 内容：	受信データ処理(構え(防御)の開始・終了)							 */
/*			byData 1=構える 0=やめる。戦闘マップで生きている時だけ構えられる	 */
/* ========================================================================= */

void RecvDataProcREQ_GUARD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}
	if (pPara->byData == 0) {
		SetGuard (pChar, FALSE);
		return;
	}
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		return;
	}
	if (GetMapNo (pChar->dwMapID) < 0) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	if ((pMap->byFlg & MAPFLG_BATTLE) == 0) {
		return;
	}
	SetGuard (pChar, TRUE);
	/* 構えたことは盾の吹き出しで知らせる */
	pChar->byComEfc = COMEFC_GUARD;
	SendCmdCOMEFC (pChar);
	pChar->byComEfc = 0;
}

void RecvDataProcREQ_ATTACK(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;
	PCHARINFO pChar, pCharTarget;
	PMAPINFO pMap, pMapTarget;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if (pChar->pID == 0) {
		return;
	}
	/* 死んでいる？(ゲージが溜まる前に倒れた後の攻撃要求は受け付けない) */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		pChar->dwLastBattle	 = 0;
		pChar->dwTargetTblID = -1;
		return;
	}
	/* 眠っている間は殴れない */
	if (IsSleep (pChar)) {
		return;
	}
	/* 殴ったら構えは解ける */
	SetGuard (pChar, FALSE);
	/* ターゲット指定無し？ */
//Todo:武器の判定
	if (pPara->dwData == -1) {
		pChar->dwLastBattle = 1;
		/* 威力の倍率（ゲージ1周＝20% 刻みで 20～100%、ジャスト。改造クライアントで 100% を超えて殴れないように） */
		pChar->byAtackRate = GetAtackRateReq (pChar, pPara->byData);
		return;
	}

	if (pPara->dwData >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	if ((GetMapNo (pChar->dwMapID) < 0) || (GetMapNo (g_aCharInfo[pPara->dwData]->dwMapID) < 0)) {
		return;
	}
	pCharTarget = g_aCharInfo[pPara->dwData];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pMapTarget	= g_aMapInfo[pCharTarget->dwMapID];

	/* 別のマップ？ */
	if (pMap != pMapTarget) {
		return;
	}
	if (!((pCharTarget->byNpcType == NPCTYPE_BATTLE1) ||
		(pCharTarget->byNpcType == NPCTYPE_BATTLE2))) {
		return;
	}
	if ((pMap->byFlg & MAPFLG_BATTLE) == 0) {
		return;
	}
	if (pCharTarget->wHP == 0) {
		return;
	}

	pChar->dwLastBattle	 = 1;
	pChar->dwTargetTblID = pPara->dwData;
	/* 威力の倍率（ゲージ1周＝20% 刻みで 20～100%、ジャスト。改造クライアントで 100% を超えて殴れないように） */
	pChar->byAtackRate = GetAtackRateReq (pChar, pPara->byData);

	/* まだ誰もターゲットしてない？ */
	if (pCharTarget->dwLastBattle == 0) {
		pCharTarget->dwLastBattle	= timeGetTime ();
		pCharTarget->dwTargetTblID	= pChar->dwTblID;
		pCharTarget->byDirection	= abyDirectionTbl[pChar->byDirection];
		pCharTarget->byFlg			|= CHARINFOFLG_DIRECTION;
	}
}


/* ========================================================================= */
/* 関数名：	RecvDataProcMAIN_CHAR											 */
/* 内容：	受信データ処理(キャラ系コマンド)								 */
/* 日付：	2006/07/26														 */
/* ========================================================================= */

void RecvDataProcMAIN_CHAR(PBYTE pData, DWORD dwDataSize)
{
	/* 関数テーブルの外を呼ばないように */
	if (pData[1] >= sizeof (RecvDataProcTblCHAR) / sizeof (RecvDataProcTblCHAR[0])) {
		return;
	}
	if (RecvDataProcTblCHAR[pData[1]]) {
		RecvDataProcTblCHAR[pData[1]](pData, dwDataSize);
	}
}


/* ========================================================================= */
/* 関数名：RecvDataProcMAPMOVE												 */
/* 内容：受信データ処理(マップ移動)											 */
/* 日付：2003/08/06															 */
/* ========================================================================= */
void RecvDataProcMAPMOVE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MAPMOVE pPara = (PCMDPARA_MAPMOVE)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (pChar->pID == 0) {
		return;
	}

	if (pPara->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}

	PMAPINFO pMap = g_aMapInfo[pPara->dwMapID];
	/* マップからはみ出している？ */
	if ((pPara->dwX >= (DWORD)pMap->nCX) || (pPara->dwY >= (DWORD)pMap->nCY)) {
		return;
	}

	SendCmdFADEINOUT (pChar->pID, 0);
	SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);
	SubProcMapMove (pChar, pPara->dwMapID, pPara->dwX, pPara->dwY);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARPUSH												 */
/* 内容：受信データ処理(キャラ押して要求)									 */
/* 日付：2003/08/11															 */
/* ========================================================================= */
void RecvDataProcCHARPUSH(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARPUSH pPara = (PCMDPARA_CHARPUSH)pData;
	PCHARINFO pChar, pCharPush;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	if (pPara->dwTblIDPush >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID],
	pCharPush = g_aCharInfo[pPara->dwTblIDPush];

	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	if (!((pCharPush->byNpcType == NPCTYPE_BLOCK) ||
		  (pCharPush->byNpcType == NPCTYPE_BLOCK2) ||
		  (pCharPush->byNpcType == NPCTYPE_BLOCK3) ||
		  (pCharPush->byNpcType == NPCTYPE_BLOCK4) ||
		  (pCharPush->byNpcType == NPCTYPE_CHGPUSH))) {
		return;
	}

	if ((pCharPush->dwX == pChar->dwX) && (pCharPush->dwY == pChar->dwY)) {
		return;
	}
	/* 離れたところのものは押せない */
	if (!IsNearRecvChar (pCharPush, 2) || (GetMapNo (pCharPush->dwMapID) < 0)) {
		return;
	}

	if (timeGetTime () < pCharPush->dwLastDotMove + 300) {
		return;
	}

	int x, y, nCount = g_aCharInfo.GetSize ();
	BOOL bUpdate = FALSE;
	BYTE byHitType;
	DWORD dwX, dwY;
	PMAPINFO pMap = g_aMapInfo[GetMapNo (pCharPush->dwMapID)];
	pCharPush->byDirection = pChar->byDirection;
	dwX = pCharPush->dwX;
	dwY = pCharPush->dwY;

	x = pCharPush->dwX + anPosAddTblX[pCharPush->byDirection];
	y = pCharPush->dwY + anPosAddTblY[pCharPush->byDirection];
	if (x >= 0 && x < pMap->nCX && y >= 0 && y < pMap->nCY) {
		byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;

		/* 障害物チェック */ 
		if ((byHitType == PARTSHIT_NONE) ||
			(byHitType == PARTSHIT_HIDE1) ||
			(byHitType == PARTSHIT_HIDE3) ||
			(byHitType == PARTSHIT_HIDE5) ||
			(byHitType == PARTSHIT_HIDE7) ||
			(byHitType == PARTSHIT_MOVEFRONT) ||
			(byHitType == PARTSHIT_MOVEUP) ||
			(byHitType == PARTSHIT_MOVEDOWN) ||
			(byHitType == PARTSHIT_MOVERIGHT) ||
			(byHitType == PARTSHIT_MOVELEFT) ||
			(byHitType == PARTSHIT_FOOTMARK_SNOW) ||
			(byHitType == PARTSHIT_FOOTMARK_SAND) ||
			(byHitType == PARTSHIT_WATERRIPPLE)) {
			dwX = x;
			dwY = y;
			bUpdate = TRUE;

		} else {
			/* 跳ね返りチェック */
			pCharPush->byDirection = abyDirectionTbl[pCharPush->byDirection];
			x = dwX + anPosAddTblX[pCharPush->byDirection];
			y = dwY + anPosAddTblY[pCharPush->byDirection];
			byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;

			if ((byHitType == PARTSHIT_NONE) ||
				(byHitType == PARTSHIT_HIDE1) ||
				(byHitType == PARTSHIT_HIDE3) ||
				(byHitType == PARTSHIT_HIDE5) ||
				(byHitType == PARTSHIT_HIDE7) ||
				(byHitType == PARTSHIT_MOVEFRONT) ||
				(byHitType == PARTSHIT_MOVEUP) ||
				(byHitType == PARTSHIT_MOVEDOWN) ||
				(byHitType == PARTSHIT_MOVERIGHT) ||
				(byHitType == PARTSHIT_MOVELEFT) ||
				(byHitType == PARTSHIT_FOOTMARK_SNOW) ||
				(byHitType == PARTSHIT_FOOTMARK_SAND) ||
				(byHitType == PARTSHIT_WATERRIPPLE)) {
				dwX = x;
				dwY = y;
				bUpdate = TRUE;
			}
		}

		if (bUpdate) {
			DWORD dwTmp = CheckCharBlockNPC (pCharPush->dwMapID, dwX, dwY);
			if (dwTmp != -1) {
				PCHARINFO pCharCheck = g_aCharInfo[dwTmp];
				if (pCharPush->dwTblID != pCharCheck->dwTblID) {
					if (pCharCheck->byNpcType != NPCTYPE_NOBLOCK) {
						/* 跳ね返りチェック */
						pCharPush->byDirection = abyDirectionTbl[pCharPush->byDirection];
						x = pCharPush->dwX + anPosAddTblX[pCharPush->byDirection];
						y = pCharPush->dwY + anPosAddTblY[pCharPush->byDirection];
						byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;

						if ((byHitType == PARTSHIT_NONE) ||
							(byHitType == PARTSHIT_HIDE1) ||
							(byHitType == PARTSHIT_HIDE3) ||
							(byHitType == PARTSHIT_HIDE5) ||
							(byHitType == PARTSHIT_HIDE7) ||
							(byHitType == PARTSHIT_MOVEFRONT) ||
							(byHitType == PARTSHIT_MOVEUP) ||
							(byHitType == PARTSHIT_MOVEDOWN) ||
							(byHitType == PARTSHIT_MOVERIGHT) ||
							(byHitType == PARTSHIT_MOVELEFT) ||
							(byHitType == PARTSHIT_FOOTMARK_SNOW) ||
							(byHitType == PARTSHIT_FOOTMARK_SAND) ||
							(byHitType == PARTSHIT_WATERRIPPLE)) {
							dwX = x;
							dwY = y;
							bUpdate = TRUE;
						}
					}
				}
			} else {
				bUpdate = TRUE;
			}
		}

		if (bUpdate) {
			if (CheckEventBlock (pCharPush->dwMapID, dwX, dwY, EVENTTYPE_ONCHARMAP)) {
				/* 跳ね返りチェック */
				pCharPush->byDirection = abyDirectionTbl[pCharPush->byDirection];
				x = pCharPush->dwX + anPosAddTblX[pCharPush->byDirection];
				y = pCharPush->dwY + anPosAddTblY[pCharPush->byDirection];
				byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;

				if ((byHitType == PARTSHIT_NONE) ||
					(byHitType == PARTSHIT_HIDE1) ||
					(byHitType == PARTSHIT_HIDE3) ||
					(byHitType == PARTSHIT_HIDE5) ||
					(byHitType == PARTSHIT_HIDE7) ||
					(byHitType == PARTSHIT_MOVEFRONT) ||
					(byHitType == PARTSHIT_MOVEUP) ||
					(byHitType == PARTSHIT_MOVEDOWN) ||
					(byHitType == PARTSHIT_MOVERIGHT) ||
					(byHitType == PARTSHIT_MOVELEFT) ||
					(byHitType == PARTSHIT_FOOTMARK_SNOW) ||
					(byHitType == PARTSHIT_FOOTMARK_SAND) ||
					(byHitType == PARTSHIT_WATERRIPPLE)) {
					dwX = x;
					dwY = y;
					bUpdate = TRUE;
				}
			}
		}

		/* 全て問題無し？ */
		if (bUpdate) {
			pCharPush->dwXBack = pCharPush->dwX;
			pCharPush->dwYBack = pCharPush->dwY;
			pCharPush->dwX = dwX;
			pCharPush->dwY = dwY;
			pCharPush->byFlg |= CHARINFOFLG_MOVE;
			pCharPush->dwLastDotMove = timeGetTime ();

			/* サイコロ？ */
			if (pCharPush->byNpcType == NPCTYPE_BLOCK2) {
				pCharPush->byEfect = (BYTE)genrand () % 6;
				SendCmdEFECTCHG (pChar->pID, pCharPush->dwTblID, pCharPush->byEfect);

			/* サイコロ２？ */
			} else if (pCharPush->byNpcType == NPCTYPE_BLOCK3) {
				pCharPush->byEfect = (BYTE)genrand () % 8;
				SendCmdEFECTCHG (pChar->pID, pCharPush->dwTblID, pCharPush->byEfect);

			/* サイコロ３？ */
			} else if (pCharPush->byNpcType == NPCTYPE_BLOCK4) {
				pCharPush->byEfect = (BYTE)genrand () % 7;
				SendCmdEFECTCHG (pChar->pID, pCharPush->dwTblID, pCharPush->byEfect);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCOMEFC												 */
/* 内容：受信データ処理(吹き出し要求)										 */
/* 日付：2003/08/12															 */
/* ========================================================================= */
void RecvDataProcCOMEFC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (!pPara->byData || (pPara->byData > MAX_COMEFC)) {
		return;
	}
	pChar->byComEfc = pPara->byData;
	pChar->byFlg |= CHARINFOFLG_COMEFC;
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDSCORE												 */
/* 内容：受信データ処理(得点追加要求)										 */
/* 日付：2003/08/22															 */
/* ========================================================================= */
void RecvDataProcADDSCORE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 話しかけた相手（目の前の NPC）だけ。種類が違うキャラや離れたキャラは変えない */
	if (((pChar->byNpcType != NPCTYPE_SCORE) && (pChar->byNpcType != NPCTYPE_CHGPUSH)) || !IsNearRecvChar (pChar, 2)) {
		return;
	}
	pChar->byEfect ++;
	switch (pChar->byNpcType) {
	case NPCTYPE_SCORE:			/* 得点板 */
		if (pChar->byEfect >= 6) {
			pChar->byEfect = 0;
		}
		break;

	case NPCTYPE_CHGPUSH:		/* 押せて話すと切り替え */
		if (pChar->byEfect >= 8) {
			pChar->byEfect = 0;
		}
		break;
	}

	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
}

/* ========================================================================= */
/* 関数名：RecvDataProcDELCHAR												 */
/* 内容：受信データ処理(キャラ削除要求)										 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void RecvDataProcDELCHAR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	DeleteChar (pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcACCADD												 */
/* 内容：受信データ処理(アカウント追加要求)									 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void RecvDataProcACCADD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ACCADD pPara = (PCMDPARA_ACCADD)pData;

	PCHARINFO pChar = new CHARINFO;
	ZeroMemory ((PBYTE)pChar, sizeof (CHARINFO));
	SetInitCharData (pChar);
	strcat (pChar->szAcc, pPara->szAcc);
	strcat (pChar->szPas, pPara->szPas);
	pChar->wGrpID = 2;
	pChar->byDirection = 1;
	AddChar (pChar);
}

/* ========================================================================= */
/* 関数名：RecvDataProcGETACC												 */
/* 内容：受信データ処理(アカウント・パスワード取得)							 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void RecvDataProcGETACC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	PCMDPARA_ACCADD pSendData;
	pSendData = new CMDPARA_ACCADD;
	ZeroMemory (pSendData, sizeof (CMDPARA_ACCADD));
	pSendData->byCmd = CMD_GETACC;
	wsprintf (pSendData->szAcc, "%s", pChar->szAcc);
	wsprintf (pSendData->szPas, "%s", pChar->szPas);
	svrSock.SendTo (pPara->pID, (PBYTE)pSendData, sizeof (CMDPARA_ACCADD), SENDTIMEOUT);
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGACC												 */
/* 内容：受信データ処理(アカウント・パスワード更新)							 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void RecvDataProcCHGACC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ACCADD pPara = (PCMDPARA_ACCADD)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	ZeroMemory ((PBYTE)pChar->szAcc, sizeof (pChar->szAcc));
	ZeroMemory ((PBYTE)pChar->szPas, sizeof (pChar->szPas));
	strcat (pChar->szAcc, pPara->szAcc);
	strcat (pChar->szPas, pPara->szPas);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTALKRAND												 */
/* 内容：受信データ処理(話してランダム要求)									 */
/* 日付：2003/08/28															 */
/* ========================================================================= */
void RecvDataProcTALKRAND(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 傭兵所のNPC（台詞が「@m」で始まる）？ 雇う・お別れする */
	if ((pChar->byNpcType != NPCTYPE_INIT) && pChar->pszNpcSpeech &&
		(pChar->pszNpcSpeech[0] == '@') && (pChar->pszNpcSpeech[1] == 'm')) {
		/* 管理者のキャラでも雇える（管理者として話しかけても、ふつうの「話す」と同じ） */
		if ((s_nCharNoRecv >= 0) && IsNearRecvChar (pChar, 2)) {
			MercenaryTalk (g_aCharInfo[s_nCharNoRecv], pChar);
		}
		return;
	}
	/* 話しかけた相手（目の前の NPC）だけ。種類が違うキャラや離れたキャラは変えない */
	if ((pChar->byNpcType != NPCTYPE_TALKRAND) || !IsNearRecvChar (pChar, 2)) {
		return;
	}
	sgenrand (timeGetTime ());
	pChar->byEfect = (BYTE)genrand () % 8;
	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSTOPRAND												 */
/* 内容：受信データ処理(止まるかランダム切り替え)							 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void RecvDataProcSTOPRAND(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 話しかけた相手（目の前の NPC）だけ。種類が違うキャラや離れたキャラは変えない */
	if ((pChar->byNpcType != NPCTYPE_STOPRAND) || !IsNearRecvChar (pChar, 2)) {
		return;
	}
	if (pChar->byEfect & 0x80) {
		pChar->byEfect &= 0x7F;

	} else {
		pChar->byEfect |= 0x80;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcGETACCINFO											 */
/* 内容：受信データ処理(アカウント情報要求)									 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void RecvDataProcGETACCINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	SendCmdGETACCINFO (pPara->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGACCSTATE											 */
/* 内容：受信データ処理(アカウント状態更新)									 */
/* 日付：2003/09/02															 */
/* ========================================================================= */
void RecvDataProcCHGACCSTATE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];
	pChar->byAccState = pPara->byData;
}

/* ========================================================================= */
/* 関数名：RecvDataProcCLIVERSION											 */
/* 内容：受信データ処理(クライアントバージョン通知)							 */
/* 日付：2003/09/05															 */
/* ========================================================================= */
void RecvDataProcCLIVERSION(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SPEECH pPara = (PCMDPARA_SPEECH)pData;

	LPSTR pszTmp = (LPSTR)new BYTE[pPara->wSize + 1];
	ZeroMemory (pszTmp, pPara->wSize + 1);
	CopyMemory (pszTmp, (PBYTE)pData + sizeof (CMDPARA_SPEECH), pPara->wSize);

	ZeroMemory (g_szCliVer, sizeof (g_szCliVer));
	wsprintf (g_szCliVer, "%s", pszTmp);
	SAFE_DELETE_ARRAY (pszTmp);
	char szBuf[128];
	ZeroMemory (szBuf, sizeof (szBuf));
	wsprintf (szBuf, "SYSTEM:ｸﾗｲｱﾝﾄﾊﾞｰｼﾞｮﾝを%sに更新しました。", g_szCliVer);
	SendCmdSYSMSG (pPara->pID, szBuf);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGCEF												 */
/* 内容：受信データ処理(吹き出し情報更新)									 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void RecvDataProcCHGCEF(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHGCEF pRecv = (PCMDPARA_CHGCEF)pData;
	PCOMEFCINFO pComEfc, pComEfcNew;

	if (pRecv->wCount >= MAX_COMEFC) {
		return;
	}
	pComEfc = &g_pComEfcInfo[pRecv->wCount];

	if (pComEfc->pAnime) {
		if (pComEfc->byAnimeCount == 1) {
			SAFE_DELETE (pComEfc->pAnime);
		} else {
			SAFE_DELETE_ARRAY (pComEfc->pAnime);
		}
	}

	pComEfcNew = (PCOMEFCINFO)(pData + sizeof (CMDPARA_CHGCEF));
	pComEfc->byAnimeCount = pComEfcNew->byAnimeCount;
	pComEfc->pAnime = pComEfcNew->pAnime;
	pComEfc->bySound = pComEfcNew->bySound;
	pComEfcNew ++;
	if (pComEfc->pAnime) {
		pComEfc->pAnime = new COMEFCANIMEINFO[pComEfc->byAnimeCount];
		CopyMemory (pComEfc->pAnime, pComEfcNew, pComEfc->byAnimeCount * sizeof (COMEFCANIMEINFO));
	}

	SendCmdCHGCEF (URARASOCK_SENDALL, pRecv->wCount);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGADMINON											 */
/* 内容：受信データ処理(管理人ON/OFF通知)									 */
/* 日付：2003/09/08															 */
/* ========================================================================= */
void RecvDataProcCHGADMINON(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	PCHARINFO pChar = g_aCharInfo[pPara->dwTblID];
	if (pChar->byAccState & ACCSTATE_ADMINON) {
		pChar->byAccState &= ~ACCSTATE_ADMINON;
		g_byAdminCount --;
		SendCmdONLINE (URARASOCK_SENDALL);
		SendCmdSYSMSG (pPara->pID, "SYSTEM:管理人業務終了を受け付けました。");

	} else {
		pChar->byAccState |= ACCSTATE_ADMINON;
		g_byAdminCount ++;
		SendCmdONLINE (URARASOCK_SENDALL);
		SendCmdSYSMSG (pPara->pID, "SYSTEM:管理人業務開始を受け付けました。");
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDITEM												 */
/* 内容：受信データ処理(アイテム追加)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcADDITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	PITEMINFO pItem = new ITEMINFO;
	ZeroMemory (pItem, sizeof (ITEMINFO));
	strcat (pItem->szName, "新規アイテム");
	g_aItemInfo.Add (pItem);

	SendCmdCHGITEM (URARASOCK_SENDALL, g_aItemInfo.GetSize () - 1);
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
	pDataPtr = SboPackGetCHGITEM (&Para, pData, RecvEnd (pData));
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
		strcat (pItemNew->szName, "新規アイテム");
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

	SendCmdCHGITEM (URARASOCK_SENDALL, pPara->dwID);
	for (int i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		RenewATDF (i);
		SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcGETITEM												 */
/* 内容：受信データ処理(アイテム取得)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcGETITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA5 pPara = (PCMDPARA_DATA5)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* もう持てない？ */
	if (GetItemCount (pPara->dwTblID) >= MAX_MYITEM) {
		return;
	}

	/* メッセージの「@4」でもらえるものだけ（クライアントが勝手に頼んできたものは渡さない） */
	if ((pPara->dwData1 == 0) || (pPara->dwData1 >= (DWORD)g_aItemInfo.GetSize ())) {
		return;
	}
	if (!CheckMsgCode (pChar, '4', (int)pPara->dwData1, 0, 0)) {
		MyOutputDebugString ("頼まれたアイテムがメッセージにありません[%s][%u]\n", pChar->szName, pPara->dwData1);
		return;
	}
	/* 代金（dwData2）はクライアントからは 0 しか送らない。以前は申告どおりに引いていたが使わない */
	AddCharItem (pPara->dwTblID, pPara->dwData1);
	SendCmdCHARITEM (pChar->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcDROPITEM												 */
/* 内容：受信データ処理(アイテム捨てる)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcDROPITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA5 pPara = (PCMDPARA_DATA5)pData;
	PCHARINFO pChar;
	PITEMINFO pItem;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	if ((pPara->byData >= MAX_MYITEM) || (pPara->dwData2 >= (DWORD)g_aItemInfo.GetSize ())) {
		return;
	}
	pItem = g_aItemInfo[pPara->dwData2];

	pChar = g_aCharInfo[pPara->dwTblID];
	if (pPara->dwData2) {
		/* 持ってないアイテム？ */
		if (CheckCharItem (pPara->dwTblID, pPara->dwData2, pPara->byData) == FALSE) {
			PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, (LPARAM)pPara->dwTblID);
#if 0		/* メッセージ出すのやめよう */
//			char szTmp[128];
//			SendCmdPLAYSOUND (URARASOCK_SENDALL, (BYTE)SOUND_PINPON);
//			wsprintf (szTmp, "SYSTEM:[%s]さんが持っていないアイテムを", pChar->szName);
//			SendCmdSYSMSG (URARASOCK_SENDALL, szTmp);
//			wsprintf (szTmp, "       手放そうとしたようです。");
//			SendCmdSYSMSG (URARASOCK_SENDALL, szTmp);
//			wsprintf (szTmp, "       バグかもしれませんが一応お知らせします。");
//			SendCmdSYSMSG (URARASOCK_SENDALL, szTmp);
#endif
			return;
		}

		/* 売った？（売値はクライアントの申告ではなく、クライアントと同じ式でサーバーが決める） */
		if (pPara->dwData1 > 0) {
			DWORD dwSell = max (pItem->dwGold / 4, (DWORD)1);
			pChar->dwGold = min (pChar->dwGold + dwSell, (DWORD)999999);
			SendCmdCHARSTATUSGOLDEXP (pChar->pID, pPara->dwTblID);
		}
	}

	DelCharItem (pPara->dwTblID, pPara->byData);
	SendCmdCHARITEM (pChar->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcREQCHARITEM											 */
/* 内容：受信データ処理(所持アイテム情報要求)								 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcREQCHARITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	SendCmdCHARITEM (pPara->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARITEM												 */
/* 内容：受信データ処理(所持アイテム情報)									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcCHARITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARITEM pPara = (PCMDPARA_CHARITEM)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	CopyMemory (pChar->waItem, pPara->waItem, sizeof (pChar->waItem));
	CopyMemory (pChar->waEqu, pPara->waEqu, sizeof (pChar->waEqu));
	SendCmdCHGITEM (pChar->pID, pChar->dwTblID);
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcPLAYSOUNDALL											 */
/* 内容：受信データ処理(効果音放送)											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void RecvDataProcPLAYSOUNDALL(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	SendCmdPLAYSOUND (URARASOCK_SENDALL, (BYTE)pPara->dwData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcUSEITEM												 */
/* 内容：受信データ処理(アイテム使う)										 */
/* 日付：2003/09/15															 */
/* ========================================================================= */
void RecvDataProcUSEITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	/* 持ち物の番号・使う相手の番号が範囲内で、そこにアイテムがある？ */
	if ((pPara->byData >= MAX_MYITEM) || (pPara->dwData >= (DWORD)g_aCharInfo.GetSize ())) {
		return;
	}
	WORD wItem = g_aCharInfo[pPara->dwTblID]->waItem[pPara->byData];
	if ((wItem == 0) || (wItem >= g_aItemInfo.GetSize ())) {
		return;
	}
	ItemProc (pPara->dwTblID, pPara->byData, pPara->dwData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSVRINFOREQ											 */
/* 内容：受信データ処理(サーバー情報要求)									 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void RecvDataProcSVRINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	SendCmdSVRINFO (pPara->pID);
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

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	pChar->clName = pPara->clName;
	pChar->clSpeech = pPara->clSpeech;
	SendCmdNAMECOLOR (pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDSHOP												 */
/* 内容：受信データ処理(店追加要求)											 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void RecvDataProcADDSHOP(PBYTE pData, DWORD dwDataSize)
{
	int i;
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];

	/* これ以上追加不可？ */
	if (pMap->byCountShop >= 100) {
		return;
	}

	/* 追加用のデータ作成 */
	PSHOPINFO pShopNew = new SHOPINFO;
	ZeroMemory (pShopNew, sizeof (SHOPINFO));

	LPSTR aMsgTbl[MAX_SHOPMSG] = {
		"＊｢いらっしゃいませ。○○へようこそ。",
		"＊｢ありがとうございました。またどうぞ。@2",
		"＊｢他に御用は？",
		"＊｢どちらをお買い求めでしょう？",
		"＊｢お金が足りないようです。@1＊｢他に御用は？",
		"＊｢もうお持ちになれないようです。@1＊｢他に御用は？",
		"＊｢%sは%dゴールドになりますがよろしいですか？",
		"＊｢ありがとうございます。@1＊｢他に御用は？",
		"＊｢どちらを買い取りましょう？",
		"＊｢%sは%dゴールドになりますがよろしいですか？",
		"＊｢そちらは買い取りできません。@1＊｢他に御用は？",
		"＊｢ありがとうございます。@1＊｢他に御用は？"
	};

	/* サンプルメッセージを登録 */
	for (i = 0; i < MAX_SHOPMSG; i ++) {
		pShopNew->apMsg[i] = new char[strlen (aMsgTbl[i]) + 1];
		ZeroMemory (pShopNew->apMsg[i], strlen (aMsgTbl[i]) + 1);
		strcat (pShopNew->apMsg[i], aMsgTbl[i]);
	}

	/* マップ情報に追加 */
	pMap->byCountShop ++;
	PSHOPINFO pShopTbl = new SHOPINFO[pMap->byCountShop];
	ZeroMemory (pShopTbl, sizeof (SHOPINFO) * pMap->byCountShop);
	CopyMemory (pShopTbl, pMap->pShopInfo, sizeof (SHOPINFO) * (pMap->byCountShop - 1));
	pShopTbl[pMap->byCountShop - 1] = *pShopNew;

	if (pMap->byCountShop == 2) {
		SAFE_DELETE (pMap->pShopInfo);
	} else {
		SAFE_DELETE_ARRAY (pMap->pShopInfo);
	}
	pMap->pShopInfo = pShopTbl;

	/* クライアントに追加分を送信 */
	SendCmdSHOP (pChar->dwTblID, pMap->byCountShop - 1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGSHOP												 */
/* 内容：受信データ処理(店情報更新)											 */
/* 日付：2003/09/22															 */
/* ========================================================================= */
void RecvDataProcCHGSHOP(PBYTE pData, DWORD dwDataSize)
{
	int i;
	PCMDPARA_SHOP pPara = (PCMDPARA_SHOP)pData;
	PBYTE pszMsg = pData + sizeof (CMDPARA_SHOP);
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSHOPINFO pShop;

	DWORD dwMapID;

	/* 送ってきたキャラのいるマップ（「元に戻す」の時は記録したマップ） */
	if (s_dwRecvMapNo != (DWORD)-1) {
		dwMapID = s_dwRecvMapNo;
	} else {
		if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
			return;
		}
		pChar = g_aCharInfo[pPara->dwTblID];
		dwMapID = pChar->dwMapID;
	}
	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	pMap = g_aMapInfo[dwMapID];
	/* 無い店は書き換えない */
	if ((pMap->pShopInfo == NULL) || (pPara->byID >= pMap->byCountShop)) {
		return;
	}
	pShop = &pMap->pShopInfo[pPara->byID];

	pShop->byCountItem = pPara->byCountItem;
	CopyMemory (pShop->awItem, pPara->awItem, sizeof (pPara->awItem));
	for (i = 0; i < MAX_SHOPMSG; i ++) {
		SAFE_DELETE_ARRAY (pShop->apMsg[i]);
		if (pPara->apMsg[i]) {
			pShop->apMsg[i] = new char[strlen ((LPSTR)pszMsg) + 1];
			ZeroMemory (pShop->apMsg[i], strlen ((LPSTR)pszMsg) + 1);
			strcat (pShop->apMsg[i], (LPSTR)pszMsg);
			pszMsg += (strlen ((LPSTR)pszMsg) + 1);
		}
	}

	/* クライアントに更新分を送信 */
	SendCmdSHOPMap (dwMapID, pPara->byID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADDEFC												 */
/* 内容：受信データ処理(エフェクト追加)										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void RecvDataProcADDEFC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	g_EfcInfo.AddNew ();

	/* クライアントに更新分を送信 */
	SendCmdEFC (g_EfcInfo.GetCount () - 1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGEFC												 */
/* 内容：受信データ処理(エフェクト情報更新)									 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void RecvDataProcCHGEFC(PBYTE pData, DWORD dwDataSize)
{
	DWORD dwSizeTmp;
	PCMDPARA_CHGEFC pRecv = (PCMDPARA_CHGEFC)pData;
	PEFCINFO pEfcInfo;

	if (pRecv->wData >= g_EfcInfo.GetCount ()) {
		return;
	}

	pEfcInfo = g_EfcInfo.SetSendData ((PBYTE)(pRecv + 1), dwSizeTmp);
	g_EfcInfo.Renew (pRecv->wData, pEfcInfo);

	SendCmdEFC ((BYTE)pRecv->wData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEFCREQ												 */
/* 内容：受信データ処理(エフェクト発動)										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void RecvDataProcEFCREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PMAPINFO pMap;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 相手は自分か、目の前あたりのキャラだけ */
	if (!IsRecvChar (pPara->dwTblID) && !IsNearRecvChar (pChar, 2)) {
		return;
	}
	if (GetMapNo (pChar->dwMapID) < 0) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	if (pPara->byData > g_EfcInfo.GetCount ()) {
		return;
	}

	/* エフェクトが許可されていない？ */
	if ((pMap->byFlg & MAPFLG_EFECT) == 0) {
		return;
	}

	/* 相手はNPC？ */
	if (pChar->byNpcType) {
		if (pChar->wHP == 0) {
			/* HPが0のNPCには発動させない */
			return;
		}
		switch (pChar->byNpcType) {
		case NPCTYPE_NOMOVE:
		case NPCTYPE_MOVERAND:
		case NPCTYPE_BLOCK:
		case NPCTYPE_BATTLE1:
		case NPCTYPE_BATTLE2:
			break;
		default:
			return;
		}
	}
	pChar->byEfect = pPara->byData;
	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
}

/* ========================================================================= */
/* 関数名：RecvDataProcREQCHARSTATUS										 */
/* 内容：受信データ処理(ステータス情報要求)									 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void RecvDataProcREQCHARSTATUS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	SendCmdCHARSTATUS (pPara->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHARSTATUS											 */
/* 内容：受信データ処理(キャラステータス通知)								 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void RecvDataProcCHARSTATUS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARSTATUS pPara = (PCMDPARA_CHARSTATUS)pData;
	PCHARINFO pChar;

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	pChar->bySaveFlg		= pPara->bySaveFlg;
	pChar->byPW				= pPara->byPW;
	pChar->byDEX			= pPara->byDEX;
	pChar->byST				= pPara->byST;
	pChar->byIT				= pPara->byIT;
	pChar->byLK				= pPara->byLK;
	if (pChar->byGameState == GAMESTATE_NPC) {
		if (pChar->byNpcType != NPCTYPE_PUT) {
			pChar->byNpcType = pPara->byNpcType;
		}
	}
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
	pChar->wDFMagic			= pPara->wDFMagic;
	pChar->wMaxHP			= pPara->wMaxHP;
	pChar->wHP				= pPara->wHP;
	pChar->wMaxMana			= pPara->wMaxMana;
	pChar->wMana			= pPara->wMana;
	pChar->dwGold			= pPara->dwGold;
	pChar->dwExp			= pPara->dwExp;
	pChar->byDrunkLV		= pPara->byDrunkLV;
	pChar->byDrunkPoint		= pPara->byDrunkPoint;
	pChar->byDropProbability = pPara->byDropProbability;
	if (pChar->byDrunkLV > CHARSTATEDRUNK_NONE) {
		pChar->dwLastSake = g_dwTime;
	} else {
		pChar->dwLastSake = 0;
		pChar->dwLastSakeHikku = 0;
	}

	RenewATDF (pChar->dwTblID);
	CopyMemory (pChar->waEqu, pPara->waEqu, sizeof (pChar->waEqu));
	CopyMemory (pChar->abyMagic, pPara->abyMagic, sizeof (pChar->abyMagic));
	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);

	/* NPC発生（発生元）？ */
	if ((pChar->byGameState == GAMESTATE_NPC) && (pChar->byNpcType == NPCTYPE_PUT)) {
		/* もう出ているキャラにも反映（以前は倒されて出直すまで古いままだった） */
		RenewPutChildChar (pChar);

		/* テンプレート指定中は、出てくるキャラはテンプレートの能力になる */
		if (s_pIDRecv && (pChar->dwStTmplateID != -1) && (pChar->dwStTmplateID < (DWORD)g_aStTmpInfo.GetSize ())) {
			char szTmp[256];
			wsprintf (szTmp, "テンプレート「%s」を使う設定なので、出てくるキャラはテンプレートの能力になります。",
				g_aStTmpInfo[pChar->dwStTmplateID]->szName);
			SendCmdSYSMSG (s_pIDRecv, szTmp, SPEECHCMD_BATTLE1);
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcTARGETCHAR											 */
/* 内容：受信データ処理(ターゲット要求)										 */
/* 日付：2003/10/02															 */
/* ========================================================================= */
void RecvDataProcTARGETCHAR(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;
	PCHARINFO pCharTmp = NULL;
	PMAPINFO pMap;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (GetMapNo (pChar->dwMapID) < 0) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];

	/* ターゲット指定？ */
	if (pPara->dwData != -1) {
		if (pPara->dwData >= (DWORD)g_aCharInfo.GetSize ()) {
			return;
		}
		pCharTmp = g_aCharInfo[pPara->dwData];

		/* 得点板属性？ */
		if (pCharTmp->byNpcType == NPCTYPE_SCORE) {
			/* 名前付きで、ほかの人のパネル？（持ち主が同じマップにいる間は外さない） */
			if (strlen (pCharTmp->szName) && (pChar->dwTargetTblID != pCharTmp->dwTblID)) {
				int i, nCount = g_aCharInfo.GetSize ();
				for (i = 0; i < nCount; i ++) {
					PCHARINFO pCharOwner = g_aCharInfo[i];
					if (pCharOwner && (pCharOwner != pChar) && (pCharOwner->byNpcType == NPCTYPE_INIT) &&
						(pCharOwner->byGameState != GAMESTATE_LOGOFF) &&
						(pCharOwner->dwMapID == pCharTmp->dwMapID) &&
						(pCharOwner->dwTargetTblID == pCharTmp->dwTblID)) {
						break;
					}
				}
				if (i < nCount) {
					SendCmdSYSMSG (pChar->pID, "ほかの人のパネルです。");
					return;
				}
				/* 持ち主がもういない名前は消して、空いたパネルとして扱う */
				ZeroMemory (pCharTmp->szName, sizeof (pCharTmp->szName));
			}
			/* 自分の名前付きパネル？（外す） */
			if (strlen (pCharTmp->szName)) {
				pCharTmp->clName = RGB (255, 255, 255);
				pCharTmp->clSpeech = RGB (255, 255, 255);
				ZeroMemory (pCharTmp->szName, sizeof (pCharTmp->szName));
				pChar->dwTargetTblID = -1;

			} else {
				/* もう同じマップの別のパネルに名前を付けている？（雪玉が当たって進むのは最後のパネルだけなので、2 つ目は付けない） */
				if ((pChar->dwTargetTblID != -1) && (pChar->dwTargetTblID < (DWORD)g_aCharInfo.GetSize ())) {
					PCHARINFO pCharScore = g_aCharInfo[pChar->dwTargetTblID];
					if (pCharScore && (pCharScore->byNpcType == NPCTYPE_SCORE) &&
						(pCharScore->dwMapID == pChar->dwMapID) && strlen (pCharScore->szName)) {
						SendCmdSYSMSG (pChar->pID, "もう名前を付けたパネルがあります。");
						return;
					}
				}
				/* 名前と色のコピー */
				/* 名前の先頭を表示幅 4（全角 2 文字）まで */
				char szTmp[MAX_CHARNAMEBUF];
				ZeroMemory (szTmp, sizeof (szTmp));
				CopyMemory (szTmp, pChar->szName, SboTextFitWidth (pChar->szName, 4));
				wsprintf (pCharTmp->szName, "[%s]", szTmp);
				pCharTmp->clName = pChar->clName;
				pCharTmp->clSpeech = pChar->clSpeech;
				/* 前にこのパネルに名前を付けていた人（ログアウト・マップ移動した人）とのつながりを切る
				   （戻ってきてから雪玉が当たっても、このパネルが進まないように） */
				{
					int i, nCount = g_aCharInfo.GetSize ();
					for (i = 0; i < nCount; i ++) {
						PCHARINFO pCharOld = g_aCharInfo[i];
						if (pCharOld && (pCharOld != pChar) && (pCharOld->dwTargetTblID == pCharTmp->dwTblID)) {
							pCharOld->dwTargetTblID = -1;
						}
					}
				}
				pChar->dwTargetTblID = pCharTmp->dwTblID;
				/* 名前を付けたら HP を満タン（HP5 の絵）に戻す */
				pCharTmp->byEfect = 0;
				SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
			}
			SendCmdCHARINFO3 (pCharTmp->dwTblID);
			return;
		}

		if (pPara->pID == 0 && pCharTmp->byNpcType == 0) {
			CharTargetPC (pChar, pCharTmp, pPara->dwData);
		}
		if (pPara->pID && pCharTmp->byNpcType && (pCharTmp->byNpcType != NPCTYPE_PUT)) {
			CharTargetPC (pChar, pCharTmp, pPara->dwData);
			if ((pMap->byFlg & MAPFLG_BATTLE) == 0) {
				return;
			}
			if (pCharTmp->wHP == 0) {
				return;
			}
			if (!((pCharTmp->byNpcType == NPCTYPE_BATTLE1) ||
				(pCharTmp->byNpcType == NPCTYPE_BATTLE2)
				)) {
				return;
			}
			if (pChar->dwTargetTblID == pPara->dwData) {
				return;
			}
			pChar->dwLastBattle	 = timeGetTime ();
			pChar->dwTargetTblID = pPara->dwData;
			/* 狙いを付けただけでは敵は起こさない（気づかれるのは攻撃が当たった時）。
			   目の前に来ただけで自動でゲージを溜めて狙うようになったので、
			   ここで起こすとノンアクティブの敵が近くを通るだけで襲ってきてしまう */
		}

	/* ターゲット解除 */
	} else {
		if (pPara->pID == 0) {
			CharTargetPC (pChar, NULL, pPara->dwData);
		}

		if ((pMap->byFlg & MAPFLG_BATTLE) == 0) {
			return;
		}

		if (pChar->dwTargetTblID != -1) {
			pChar->dwLastBattle = 0;
			pChar->dwTargetTblID = -1;
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGEQU												 */
/* 内容：受信データ処理(装備変更)											 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void RecvDataProcCHGEQU(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 装備の場所は MAX_EQUTYPE 未満、中身は持ち物の番号（0xFFFF は外す） */
	if ((pPara->byData >= MAX_EQUTYPE) ||
			((pPara->dwData != 0xFFFF) && (pPara->dwData >= MAX_MYITEM))) {
		return;
	}
	pChar->waEqu[pPara->byData] = (WORD)pPara->dwData;

	/* 装備品をチェック */
	CheckCharEqu (pChar);

	RenewATDF (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：ApplyStTmpToChars												 */
/* 内容：ステータステンプレートを使っているキャラへ反映する					 */
/*       （管理者ウィンドウと AI 用データ API（server/web_data_svr.cpp）から呼ぶ） */
/* ========================================================================= */
void ApplyStTmpToChars(DWORD dwID)
{
	if (dwID >= (DWORD)g_aStTmpInfo.GetSize ()) {
		return;
	}
	PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[dwID];
	for (int i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		/* テンプレートは指定されていない？ */
		if (pChar->dwStTmplateID == -1) {
			continue;
		}
		/* 別のテンプレートを使っている？（以前は全テンプレートのキャラを上書きしていた） */
		if (pChar->dwStTmplateID != dwID) {
			continue;
		}
		/* 発生元？（発生元そのものは変えず、出ているキャラへ反映） */
		if (pChar->byNpcType == NPCTYPE_PUT) {
			RenewPutChildChar (pChar);
			continue;
		}
		pChar->byLevel		= pInfo->byLevel;
		pChar->byPW			= pInfo->byPW;
		pChar->byDEX		= pInfo->byDEX;
		pChar->byST			= pInfo->byST;
		pChar->byIT			= pInfo->byIT;
		pChar->byLK			= pInfo->byLK;
		pChar->chAttrFire	= pInfo->chAttrFire;
		pChar->chAttrWater	= pInfo->chAttrWater;
		pChar->chAttrWind	= pInfo->chAttrWind;
		pChar->chAttrEarth	= pInfo->chAttrEarth;
		pChar->chAttrLight	= pInfo->chAttrLight;
		pChar->chAttrDark	= pInfo->chAttrDark;
		pChar->byNpcType	= pInfo->byNpcType;
		pChar->wMaxHP		= pInfo->wMaxHP;
		pChar->wHP			= pInfo->wMaxHP;
		pChar->wMaxMana		= pInfo->wMaxMana;
		pChar->wMana		= pInfo->wMaxMana;
		pChar->wDFMagic		= pInfo->wDFMagic;
		pChar->dwGold		= pInfo->dwGold;
		pChar->dwExp		= pInfo->dwExp;
		CopyMemory (pChar->waItem, pInfo->waItem, sizeof (pChar->waItem));
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSTTMPLATE											 */
/* 内容：受信データ処理(ステータステンプレート情報)							 */
/* 日付：2003/10/25															 */
/* ========================================================================= */
void RecvDataProcSTTMPLATE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	switch (pPara->byData) {
	case CMDSTTMPLATETYPE_ADD:	/* 追加 */
		{
			PSTATUSTEMPLATEINFO pInfo = new STATUSTEMPLATEINFO;
			ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
			/* 名前は可変長で届くので構造体に戻す */
			if (SboPackGetSTATUSTEMPLATEINFO (pInfo, (PBYTE)(pPara + 1), RecvEnd (pData)) == NULL) {
				delete pInfo;
				break;
			}
			g_aStTmpInfo.Add (pInfo);
			SendCmdSTTMPLATE (g_aStTmpInfo.GetSize () - 1, CMDSTTMPLATETYPE_ADD);
		}
		break;

	case CMDSTTMPLATETYPE_CHG:	/* 更新 */
		{
			STATUSTEMPLATEINFO InfoTmp;
			if (pPara->dwTblID >= (DWORD)g_aStTmpInfo.GetSize ()) {
				break;
			}
			/* 名前は可変長で届くので構造体に戻す */
			ZeroMemory (&InfoTmp, sizeof (InfoTmp));
			if (SboPackGetSTATUSTEMPLATEINFO (&InfoTmp, (PBYTE)(pPara + 1), RecvEnd (pData)) == NULL) {
				break;
			}
			PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[pPara->dwTblID];
			CopyMemory (pInfo, &InfoTmp, sizeof (STATUSTEMPLATEINFO));
			SendCmdSTTMPLATE (pPara->dwTblID, CMDSTTMPLATETYPE_CHG);

			/* 更新されたテンプレートを使用しているキャラのデータを更新 */
			ApplyStTmpToChars (pPara->dwTblID);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEITEM											 */
/* 内容：受信データ処理(アイテム預ける)										 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void RecvDataProcLEAVEITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pPara->byData >= MAX_MYITEM) || (pChar->waItem[pPara->byData] == 0)) {
		return;
	}

	int i;
	/* 装備中？ */
	for (i = 0; i < 10; i ++) {
		if (pChar->waEqu[i] == pPara->byData) {
			return;
		}
	}

	/* 預かり所に入れる（いっぱいなら何もしない） */
	if (!AddLeaveItem (pChar, pChar->waItem[pPara->byData])) {
		return;
	}
	DelCharItem (pPara->dwTblID, pPara->byData);
	SendCmdCHARITEM (pPara->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：AddLeaveItem														 */
/* 内容：預かり所にアイテムを1個入れる（口座が無ければ作る）				 */
/* 戻り値：入れられたら TRUE、預かり所がいっぱい等で入れられなければ FALSE	 */
/* 日付：2026/11/07															 */
/* ========================================================================= */
BOOL AddLeaveItem(PCHARINFO pChar, WORD wItemID)
{
	PLEAVEINFO pInfo = NULL;
	int i;

	if ((pChar == NULL) || (wItemID == 0)) {
		return FALSE;
	}
	if (g_aLeaveInfo.GetSize () == 0) {
		return FALSE;
	}
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		pInfo = g_aLeaveInfo[pChar->dwLeaveID];
	}

	/* 口座（？）が無い？ */
	if (pInfo == NULL) {
		for (i = 1; i < g_aLeaveInfo.GetSize (); i ++) {
			PLEAVEINFO pInfoTmp = g_aLeaveInfo[i];
			if (pInfoTmp->byUse == 0) {
				break;
			}
		}
		if (i < g_aLeaveInfo.GetSize ()) {
			pInfo = g_aLeaveInfo[i];
		} else {
			i = g_aLeaveInfo.GetSize ();
			pInfo = new LEAVEINFO;
			g_aLeaveInfo.Add (pInfo);
		}
		ZeroMemory (pInfo, sizeof (LEAVEINFO));
		pInfo->byUse = 1;
		pChar->dwLeaveID = i;
	}

	/* 同じアイテムがあれば重ねる */
	for (i = 0; i < pInfo->byCount; i ++) {
		if ((pInfo->waItem[i] == wItemID) && (pInfo->byItemCount[i] < 255)) {
			pInfo->byItemCount[i] ++;
			return TRUE;
		}
	}
	/* 重ならなかった：空き枠に入れる */
	if (pInfo->byCount >= MAX_LEAVEITEM) {
		return FALSE;
	}
	pInfo->waItem[pInfo->byCount] = wItemID;
	pInfo->byItemCount[pInfo->byCount] = 1;
	pInfo->byCount ++;
	return TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEITEMGET											 */
/* 内容：受信データ処理(預けてるアイテム取得)								 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
void RecvDataProcLEAVEITEMGET(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pPara = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;
	PLEAVEINFO pInfo = NULL;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	if (g_aLeaveInfo.GetSize () == 0) {
		return;
	} else if (g_aLeaveInfo.GetSize () == 1) {
	} else if (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ()) {
	} else {
		if (pChar->dwLeaveID) {
			pInfo = g_aLeaveInfo[pChar->dwLeaveID];
		}
	}

	/* 一覧取得？ */
	if (pPara->dwData == -1) {
		SendCmdLEAVEITEMGET (pPara->dwTblID);
		return;
	}

	/* 口座が無い・番号が範囲外・もう無い？ */
	if ((pInfo == NULL) || (pPara->dwData >= (DWORD)pInfo->byCount) ||
			(pPara->dwData >= MAX_LEAVEITEM) || (pInfo->byItemCount[pPara->dwData] == 0)) {
		return;
	}
	/* もう持てない？ */
	if (GetItemCount (pChar->dwTblID) >= MAX_MYITEM) {
		return;
	}

	AddCharItem (pChar->dwTblID, pInfo->waItem[pPara->dwData]);
	SendCmdCHARITEM (pPara->pID, pPara->dwTblID);

	if (pInfo->byItemCount[pPara->dwData] >= 1) {
		pInfo->byItemCount[pPara->dwData] --;
	}
	if (pInfo->byItemCount[pPara->dwData] == 0) {
		int i;
		for (i = pPara->dwData; i < MAX_LEAVEITEM - 1; i ++) {
			pInfo->waItem[i] = pInfo->waItem[i + 1];
			pInfo->byItemCount[i] = pInfo->byItemCount[i + 1];
		}
		pInfo->waItem[i] = 0;
		pInfo->byItemCount[i] = 0;
		pInfo->byCount --;
	}
	SendCmdCHARITEM (pPara->pID, pPara->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAMEGET											 */
/* 内容：受信データ処理(サブゲーム情報取得)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void RecvDataProcSUBGAMEGET(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pRecv = (PCMDPARA_DATA3)pData;

	/* まず登録数を送信 */
	SendCmdSUBGAMECOUNT (pRecv->pID);
	SendCmdSUBGAMEINFO (pRecv->pID, pRecv->dwData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAMEADD											 */
/* 内容：受信データ処理(サブゲーム情報追加)									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void RecvDataProcSUBGAMEADD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;

	PSUBGAMEINFO pInfo = new SUBGAMEINFO;
	ZeroMemory (pInfo, sizeof (SUBGAMEINFO));
	g_aSubaGameInfo.Add (pInfo);

	/* まず登録数を送信 */
	SendCmdSUBGAMECOUNT (pRecv->pID);
	SendCmdSUBGAMEINFO (pRecv->pID, g_aSubaGameInfo.GetSize () - 1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAMEINFO											 */
/* 内容：受信データ処理(サブゲーム情報更新)									 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void RecvDataProcSUBGAMEINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;
	PVOID pInfoBack;
	BYTE byTypeBack;

	if (pRecv->dwTblID >= (DWORD)g_aSubaGameInfo.GetSize ()) {
		return;
	}

	PSUBGAMEINFO pInfo = g_aSubaGameInfo[pRecv->dwTblID];
	pInfoBack = pInfo->pInfo;
	byTypeBack = pInfo->byType;
	int nSize = GetSubGameDataSize (pRecv->byData);
	if (nSize == 0) {
		ZeroMemory (pInfo, sizeof (SUBGAMEINFO));
		goto Exit;
	}
	pInfo->pInfo = new BYTE[nSize];
	ZeroMemory (pInfo->pInfo, nSize);
	pInfo->byType = pRecv->byData;
	switch (pInfo->byType) {
	case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
		{
			PSUBGAME_BOMBINFO pBombInfo = (PSUBGAME_BOMBINFO)g_aSubaGameInfo[pRecv->dwTblID]->pInfo;
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
			SubGameProcBombQuit ((BYTE)pRecv->dwTblID);
		}
		break;

	case SUBGAMEID_MAZE:		/* 迷路 */
		{
			PSUBGAME_MAZEINFO pMazeInfo = (PSUBGAME_MAZEINFO)g_aSubaGameInfo[pRecv->dwTblID]->pInfo;
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
			/* 管理者ウィンドウに無い項目（混ぜるパーツ・通路の太さ等）は前の値を引き継ぐ */
			if (pInfoBack && (byTypeBack == SUBGAMEID_MAZE)) {
				PSUBGAME_MAZEINFO pBack = (PSUBGAME_MAZEINFO)pInfoBack;
				pMazeInfo->wChipIDRoad2		= pBack->wChipIDRoad2;
				pMazeInfo->byRoad2Rate		= pBack->byRoad2Rate;
				pMazeInfo->wChipIDBlock2	= pBack->wChipIDBlock2;
				pMazeInfo->byBlock2Rate		= pBack->byBlock2Rate;
				pMazeInfo->wChipIDBlockFace	= pBack->wChipIDBlockFace;
				pMazeInfo->byPathWidth		= pBack->byPathWidth;
				pMazeInfo->wChipIDBridgeV	= pBack->wChipIDBridgeV;
				pMazeInfo->wChipIDBridgeH	= pBack->wChipIDBridgeH;
				pMazeInfo->byCrossRate		= pBack->byCrossRate;
			}
		}		
		break;

	case SUBGAMEID_DUNGEON:		/* ダンジョン */
		{
			PSUBGAME_DUNGEONINFO pDungeonInfo = (PSUBGAME_DUNGEONINFO)g_aSubaGameInfo[pRecv->dwTblID]->pInfo;
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
			PSUBGAME_WARINFO pInfo = (PSUBGAME_WARINFO)g_aSubaGameInfo[pRecv->dwTblID]->pInfo;
			PSUBGAME_WARINFOSAVE pInfoRecv = (PSUBGAME_WARINFOSAVE)(pRecv + 1);

			pInfo->dwMapID = pInfoRecv->dwMapID;
			pInfo->byMember = pInfoRecv->byMember;
			pInfo->byX = pInfoRecv->byX;
			pInfo->byY = pInfoRecv->byY;
			pInfo->byX1 = pInfoRecv->byX1;
			pInfo->byY1 = pInfoRecv->byY1;
			pInfo->byX2 = pInfoRecv->byX2;
			pInfo->byY2 = pInfoRecv->byY2;
			pInfo->byHP1X = pInfoRecv->byHP1X;
			pInfo->byHP2X = pInfoRecv->byHP2X;
			pInfo->byHPY = pInfoRecv->byHPY;
			pInfo->byJudgeX = pInfoRecv->byJudgeX;
			pInfo->byJudgeY = pInfoRecv->byJudgeY;
			pInfo->byRes1X = pInfoRecv->byRes1X;
			pInfo->byRes2X = pInfoRecv->byRes2X;
			pInfo->byResY = pInfoRecv->byResY;
			pInfo->byStart1X = pInfoRecv->byStart1X;
			pInfo->byStart2X = pInfoRecv->byStart2X;
			pInfo->byStartY = pInfoRecv->byStartY;
			pInfo->byCastle1X = pInfoRecv->byCastle1X;
			pInfo->byCastle2X = pInfoRecv->byCastle2X;
			pInfo->byCastleY = pInfoRecv->byCastleY;
		}
		break;

	case SUBGAMEID_SUGOROKU:	/* すごろく */
		{
			PSUBGAME_SUGOROKUINFO pInfo = (PSUBGAME_SUGOROKUINFO)g_aSubaGameInfo[pRecv->dwTblID]->pInfo;
			PSUBGAME_SUGOROKUINFOSAVE pInfoRecv = (PSUBGAME_SUGOROKUINFOSAVE)(pRecv + 1);
			SUGOROKUEVENTINFO EventInfo;
			SUGOROKUPARTSINFO PartsInfo;
			PBYTE pTmp;
			DWORD i, dwCountTmp;

			SubSugoroku_Quit ((PSUBGAME_SUGOROKUINFO)pInfoBack);

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
Exit:
	SAFE_DELETE_ARRAY (pInfoBack);
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEINFOGET											 */
/* 内容：受信データ処理(預かり所情報取得)									 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void RecvDataProcLEAVEINFOGET(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pRecv = (PCMDPARA_DATA1)pData;

	if (pRecv->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	SendCmdLEAVEINFO (pRecv->pID, pRecv->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEINFO											 */
/* 内容：受信データ処理(預かり所情報)										 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void RecvDataProcLEAVEINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_LEAVEINFO pRecv = (PCMDPARA_LEAVEINFO)pData;
	PCHARINFO pChar;

	if (pRecv->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	if (pChar->dwLeaveID == 0) {
		return;
	}

	PLEAVEINFO pInfo = g_aLeaveInfo[pChar->dwLeaveID];
	pInfo->dwGold = pRecv->dwGold;
	pInfo->byCount = pRecv->byCount;
	CopyMemory (pInfo->waItem, pRecv->waItem, sizeof (pInfo->waItem));
	CopyMemory (pInfo->byItemCount, pRecv->byItemCount, sizeof (pInfo->byItemCount));
}

/* ========================================================================= */
/* 関数名：RecvDataProcACCSEARCH											 */
/* 内容：受信データ処理(アカウント検索)										 */
/* 日付：2004/02/08															 */
/* ========================================================================= */
void RecvDataProcACCSEARCH(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_ACCSEARCH Para;
	PCMDPARA_ACCSEARCH pRecv = &Para;
	int i, nCount;

	/* 名前は可変長で届くので構造体に戻す */
	if (SboPackGetACCSEARCH (&Para, pData, RecvEnd (pData)) == NULL) {
		return;
	}
	if (pRecv->dwTblIDStart >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	i = (int)pRecv->dwTblIDStart;
	nCount = g_aCharInfo.GetSize ();

	for (; i < nCount; i ++) {
		BOOL bCheck;
		PCHARINFO pChar = g_aCharInfo[i];

		bCheck = FALSE;
		/* アカウント名で検索？ */
		if (pRecv->byMode == 0) {
			if (strstr (pChar->szAcc, pRecv->szName)) {
				bCheck = TRUE;
			}

		/* キャラ名で検索 */
		} else {
			if (strstr (pChar->szName, pRecv->szName)) {
				bCheck = TRUE;
			}
		}

		if (bCheck) {
			SendCmdACCSEARCH (pRecv->pID, pChar->dwTblID);
			break;
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcMSGCMD												 */
/* 内容：受信データ処理(メッセージコマンド)									 */
/* 日付：2004/05/08															 */
/* ========================================================================= */
void RecvDataProcMSGCMD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MSGCMD pPara = (PCMDPARA_MSGCMD)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* サイコロは 20 個まで（多すぎると文字列があふれる） */
	if (pPara->nData1 > 20) {
		pPara->nData1 = 20;
	}
	switch (pPara->byData) {
	case MSGCMD_DICE:			/* サイコロ */
		{
			char szTmp[256], szMsg[256];
			if (pPara->nData1 <= 1) {
				wsprintf (szMsg, "SYSTEM:%sのサイコロ！[%d]が出ました。",
						pChar->szName, genrand () % 6 + 1);
			} else {
				int i;

				wsprintf (szMsg, "SYSTEM:%sのサイコロ！[", pChar->szName);
				for (i = 0; i < pPara->nData1; i ++) {
					wsprintf (szTmp, "%d, ", genrand () % 6 + 1);
					strcat (szMsg, szTmp);
				}
				szMsg[strlen (szMsg) - 2] = 0;
				strcat (szMsg, "]が出ました。");
			}
			SendCmdSYSMSG (pChar->dwTblID, szMsg, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_KENO:			/* ケノ */
		{
			char szTmp[256];
			wsprintf (szTmp, "SYSTEM:%sのケノ！[%d]が出ました。",
					pChar->szName, genrand () % 80 + 1);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_COIN:			/* コイン */
		{
			char szTmp[256];
			wsprintf (szTmp, "SYSTEM:%sのコイン！[%s]が出ました。",
				pChar->szName, (genrand () % 2) ? "表" : "裏");
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_WDICE:			/* 戦場ゲーム */
		{
			char szTmp[256],
				 *aszTbl[] = {"３", "２", "２", "１", "１", "×", "！"};
			wsprintf (szTmp, "SYSTEM:%sの戦場！[%s]が出ました。",
				pChar->szName, aszTbl[genrand () % 7]);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_2D6:			/* サイコロ(1～6を2個) */
		{
			char szTmp[256];
			int nTmp1, nTmp2;
			nTmp1 = genrand () % 6 + 1;
			nTmp2 = genrand () % 6 + 1;

			wsprintf (szTmp, "SYSTEM:%sの2d6！(%d + %d) = %d",
					pChar->szName, nTmp1, nTmp2, nTmp1 + nTmp2);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_3D6:			/* サイコロ(1～6を3個) */
		{
			char szTmp[256];
			int nTmp1, nTmp2, nTmp3;
			nTmp1 = genrand () % 6 + 1;
			nTmp2 = genrand () % 6 + 1;
			nTmp3 = genrand () % 6 + 1;

			wsprintf (szTmp, "SYSTEM:%sの3d6！(%d + %d + %d) = %d",
					pChar->szName, nTmp1, nTmp2, nTmp3, nTmp1 + nTmp2 + nTmp3);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_1D10:			/* サイコロ(1～10) */
		{
			char szTmp[256];
			wsprintf (szTmp, "SYSTEM:%sの1d10！[%d]が出ました。",
					pChar->szName, genrand () % 10 + 1);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_1D20:			/* サイコロ(1～20) */
		{
			char szTmp[256];
			wsprintf (szTmp, "SYSTEM:%sの1d20！[%d]が出ました。",
					pChar->szName, genrand () % 20 + 1);
			SendCmdSYSMSG (pChar->dwTblID, szTmp, SPEECHCMD_SCREEN);
		}
		break;

	case MSGCMD_WHERE:			/* 最も人が集まっている場所（送ってきた本人にだけ返す） */
		SendMsgMostCrowdedMap (s_pIDRecv);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcPARTY												 */
/* 内容：受信データ処理(仲間コマンド)										 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void RecvDataProcPARTY(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_PARTYCMD pPara = (PCMDPARA_PARTYCMD)pData;
	PCHARINFO pCharFrom, pCharTo;
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PPARTYINFO pPartyInfo = NULL;
	BOOL bBreak = FALSE;
	int i, nCount;
	char szTmp[128];

	/* 送信者は常に自分自身 */
	if (!IsRecvChar (pPara->dwTblIDFrom)) {
		return;
	}
	if (pPara->dwTblIDTo >= dwCount) {
		return;
	}

	pCharFrom = g_aCharInfo[pPara->dwTblIDFrom];
	pCharTo = g_aCharInfo[pPara->dwTblIDTo];
	if (pCharTo->byGameState < GAMESTATE_MOVE) {
		return;
	}

	/* 各コマンド処理 */
	switch (pPara->byCmdSub) {
	case CMDPARTY_JOIN:			/* 誘う */
		if (pCharFrom->dwMapID != pCharTo->dwMapID) {
			wsprintf (szTmp, "%sは近くにいないので誘えません。", pCharTo->szName);
			SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
			break;
		}
		if (pCharTo->byCharState == CHARSTATE_DEAD) {
			wsprintf (szTmp, "%sは誘える状態ではありません。", pCharTo->szName);
			SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
			break;
		}
		if ((abs (pCharFrom->dwX - pCharTo->dwX) >= DRAW_PARTS_X * 2) ||
			(abs (pCharFrom->dwY - pCharTo->dwY) >= DRAW_PARTS_Y * 2)) {
			wsprintf (szTmp, "%sは近くにいないので誘えません。", pCharTo->szName);
			SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
			break;
		}

		if (pCharTo->dwPartyID) {
			wsprintf (szTmp, "%sはどこかのパーティに参加しています。", pCharTo->szName);
			SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
			break;
		}

		pPartyInfo = g_aPartyInfo[pCharFrom->dwPartyID];
		if (pPartyInfo->adwTblID[MAX_PARTYMEMBER - 1]) {
			wsprintf (szTmp, "これ以上パーティに参加できません。");
			SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
			break;
		}

		/* 誘われた側の「参加する」を照合するため誘った人を記録 */
		s_mapPartyInvite[pCharTo->dwTblID] = pCharFrom->dwTblID;
		SendCmdPARTY (pCharTo->dwTblID, pCharFrom->dwTblID, CMDPARTY_JOIN, 0);
		break;

	case CMDPARTY_JOINOK:		/* 参加する */
		{
			/* 実際に誘われた相手にだけ参加できる */
			std::map<DWORD, DWORD>::iterator it = s_mapPartyInvite.find (pCharFrom->dwTblID);
			if ((it == s_mapPartyInvite.end ()) || (it->second != pCharTo->dwTblID)) {
				break;
			}
			s_mapPartyInvite.erase (it);
		}
		if (pCharFrom->dwPartyID) {
			break;
		}
		if (pCharTo->dwPartyID == 0) {
			nCount = g_aPartyInfo.GetSize ();
			for (i = 1; i < nCount; i ++) {
				pPartyInfo = g_aPartyInfo[i];
				if (pPartyInfo->adwTblID[0] == 0) {
					break;
				}
			}
			/* 見つからなかった？ */
			if (i >= nCount) {
				pPartyInfo = new PARTYINFO;
				ZeroMemory (pPartyInfo, sizeof (PARTYINFO));
				g_aPartyInfo.Add (pPartyInfo);
			}
			pPartyInfo->adwTblID[0] = pCharTo->dwTblID;
			pCharTo->dwPartyID = i;

		} else {
			pPartyInfo = g_aPartyInfo[pCharTo->dwPartyID];
			/* 人数チェック */
			for (i = 0; i < MAX_PARTYMEMBER; i ++) {
				if (pPartyInfo->adwTblID[i] == 0) {
					break;
				}
			}
			if (i >= MAX_PARTYMEMBER) {
				wsprintf (szTmp, "パーティに参加できませんでした。");
				SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
				break;
			}
		}

		/* メンバーリストに追加 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			if (pPartyInfo->adwTblID[i] == pCharFrom->dwTblID) {
				break;
			}
			if (pPartyInfo->adwTblID[i] == 0) {
				pPartyInfo->adwTblID[i] = pCharFrom->dwTblID;
				break;
			}
		}

		pCharFrom->dwPartyID = pCharTo->dwPartyID;

		wsprintf (szTmp, "%sがパーティに参加しました。", pCharFrom->szName);
		/* 各メンバーに参加を通知 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			PCHARINFO pCharTmp;

			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}
			pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			if (pCharFrom != pCharTmp) {
				SendCmdPARTY (pCharTmp->dwTblID, pCharFrom->dwTblID, CMDPARTY_JOINOK, 0);
				SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
			}
		}
		SendCmdCHARSTATUSHPMANA (pCharFrom->dwTblID, pCharFrom->dwTblID);

		/* 現在のメンバーを通知 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			PCHARINFO pCharTmp;

			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}
			pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			if (pCharFrom != pCharTmp) {
				SendCmdPARTY (pCharFrom->dwTblID, pCharTmp->dwTblID, CMDPARTY_JOINOK, 0);
				SendCmdCHARSTATUSHPMANA (pCharFrom->dwTblID, pCharTmp->dwTblID);
			}
		}
		wsprintf (szTmp, "パーティに参加しました。");
		SendCmdSYSMSG (pCharFrom->pID, szTmp, SPEECHCMD_BATTLE1);
		break;

	case CMDPARTY_JOINNO:		/* 参加しない */
		s_mapPartyInvite.erase (pCharFrom->dwTblID);
		SendCmdPARTY (pCharTo->dwTblID, pCharFrom->dwTblID, CMDPARTY_JOINNO, 0);
		wsprintf (szTmp, "%sは参加を拒否しました。", pCharFrom->szName);
		SendCmdSYSMSG (pCharTo->pID, szTmp, SPEECHCMD_BATTLE1);
		break;

	case CMDPARTY_JOINCANCEL:	/* 応答拒否 */
		wsprintf (szTmp, "%sは今返事できないようです。", pCharFrom->szName);
		SendCmdSYSMSG (pCharTo->pID, szTmp, SPEECHCMD_BATTLE1);
		break;

	case CMDPARTY_BREAKUP:		/* 解散 */
		ProcBreakUpParty (pCharFrom);
		break;

	case CMDPARTY_BREAK:	/* 抜ける */
		ProcBreakParty (pCharFrom);
		break;

	case CMDPARTY_GOLD:		/* 分配方法 */
		if (pCharFrom->dwPartyID == 0) {
			return;
		}

		pPartyInfo = g_aPartyInfo[pCharFrom->dwPartyID];
		pPartyInfo->byGold ++;
		if (pPartyInfo->byGold >= PARTYGOLD_MAX) {
			pPartyInfo->byGold = PARTYGOLD_TARGET;
		}

		switch (pPartyInfo->byGold) {
		case PARTYGOLD_TARGET:	/* 止めをさしたプレイヤー */
			wsprintf (szTmp, "[分配方法]「倒したプレイヤー」に設定されました。");
			break;

		case PARTYGOLD_ALL:		/* 全員で分配 */
			wsprintf (szTmp, "[分配方法]「全員で分配」に設定されました。");
			break;
		}

		/* 各メンバーに通知 */
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			PCHARINFO pCharTmp;

			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}

			pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
			SendCmdSYSMSG (pCharTmp->pID, szTmp, SPEECHCMD_BATTLE1);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcJOBINFO												 */
/* 内容：受信データ処理(職業情報)											 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void RecvDataProcJOBINFO(PBYTE pData, DWORD dwDataSize)
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

	/* 全員に更新を通知 */
	SendCmdJOBINFO (URARASOCK_SENDALL, pPara->byID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcJOBINFOREQ											 */
/* 内容：受信データ処理(職業情報要求)										 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void RecvDataProcJOBINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo;

	nNo = GetCharNo (pPara->pID);
	if (nNo < 0) {
		return;
	}

	SendCmdJOBINFO (pPara->pID, pPara->byData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcJOBCHG												 */
/* 内容：受信データ処理(職業変更)											 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void RecvDataProcJOBCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	int nNo, i, j;
	BYTE byJobOld, byJobNew, byMagic;
	BOOL bRestore;

	nNo = GetCharNo (s_pIDRecv);
	if (nNo < 0) {
		return;
	}
/* Todo:ここでなれるかチェック */
	/* 少なくとも存在する職業だけ */
	if (g_JobInfo.GetPtr ((int)pPara->byData) == NULL) {
		return;
	}
	pChar = g_aCharInfo[nNo];
	byJobOld = pChar->byJOB;
	byJobNew = pPara->byData;

	if (pChar->byAdminLV != ADMINLV_USER) {
		/* 管理者は管理者ウィンドウで好きな呪文を持たせているので、転職しても消さない */
		/* （空欄があれば詰めておく） */
		j = 0;
		for (i = 0; i < MAX_USEMAGIC; i ++) {
			byMagic = pChar->abyMagic[i];
			pChar->abyMagic[i] = 0;
			if (byMagic) {
				pChar->abyMagic[j ++] = byMagic;
			}
		}
		pChar->byJOB = byJobNew;

	/* 同じ職業への登録し直しなら、覚えている呪文はそのまま */
	} else if (byJobOld != byJobNew) {
		/* 今の職業で覚えていた呪文を職業別に覚えておく（元の職業に戻した時に復活させる） */
		if (byJobOld < MAX_JOBMAGIC) {
			CopyMemory (pChar->abyJobMagic[byJobOld], pChar->abyMagic, sizeof (pChar->abyMagic));
		}
		/* 職業によって使えない呪文があるので、一旦全て初期化 */
		ZeroMemory (pChar->abyMagic, sizeof (pChar->abyMagic));

		/* 新しい職業で以前覚えていた呪文があれば復活させる */
		bRestore = FALSE;
		if (byJobNew < MAX_JOBMAGIC) {
			j = 0;
			for (i = 0; i < MAX_USEMAGIC; i ++) {
				byMagic = pChar->abyJobMagic[byJobNew][i];
				if (byMagic == 0) {
					continue;
				}
				/* 管理画面で呪文や利用者情報が変わっているかもしれないので、今使えるものだけ */
				if (byMagic >= g_aMagicInfo.GetSize ()) {
					continue;
				}
				if (CheckMagicUserInfo (g_aMagicInfo[byMagic]->byUserInfo, byJobNew) == FALSE) {
					continue;
				}
				/* 使えずに戻さなかった呪文の所が空欄にならないよう、前から詰めて戻す */
				pChar->abyMagic[j ++] = byMagic;
				bRestore = TRUE;
			}
			/* 復活させた分は今の呪文になったので、保存場所は空にしておく */
			ZeroMemory (pChar->abyJobMagic[byJobNew], sizeof (pChar->abyJobMagic[byJobNew]));
		}
		pChar->byJOB = byJobNew;
		if (bRestore) {
			SendCmdSYSMSG (pChar->pID, "以前この職業で覚えていた特技を思い出しました。", SPEECHCMD_BATTLE1);
		}
	}

	/* 装備品をチェック */
	CheckCharEqu (pChar);

	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcUSEMAGIC												 */
/* 内容：受信データ処理(魔法使用)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void RecvDataProcUSEMAGIC(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA5 pPara = (PCMDPARA_DATA5)pData;
	PCHARINFO pChar;
	DWORD dwCount;

	dwCount = g_aCharInfo.GetSize ();
	if (!IsRecvChar (pPara->dwTblID) ||
		(pPara->dwData2 >= dwCount)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 死んでいる？(詠唱中に倒れた後の特技は受け付けない) */
	if ((pChar->byNpcType == NPCTYPE_INIT) &&
		((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0))) {
		return;
	}
	if (pChar->dwTargetTblID != -1) {
		/* ターゲットは解除する */
		pChar->dwLastBattle = 0;
		pChar->dwTargetTblID = -1;
	}

	/* 町転送で選んだ行き先（byData。町転送以外は 0、選べない番号は復活位置になる） */
	pChar->byTownWarpSel = pPara->byData;
	MagicProc (pPara->dwTblID, (BYTE)pPara->dwData1, pPara->dwData2);
	pChar->byTownWarpSel = 0;
}

/* ========================================================================= */
/* 関数名：RecvDataProcTARGETCHG											 */
/* 内容：受信データ処理(ターゲット変更要求)									 */
/* 日付：2004/07/16															 */
/* ========================================================================= */
void RecvDataProcTARGETCHG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar, pCharTarget;
	int nNo;
	PCCharProcBase pProc;

	nNo = GetCharNo (s_pIDRecv);
	if (nNo < 0) {
		return;
	}

	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar		= g_aCharInfo[pPara->dwTblID];
	pCharTarget	= g_aCharInfo[nNo];

	pProc = (PCCharProcBase)pChar->pProc;
	if (pProc) {
		pProc->ChgTarget (pCharTarget->dwTblID, abyDirectionTbl[pCharTarget->byDirection]);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSHOPBUY												 */
/* 内容：受信データ処理(アイテム購入)										 */
/* 日付：2004/07/28															 */
/* ========================================================================= */
void RecvDataProcSHOPBUY(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA5 pPara = (PCMDPARA_DATA5)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSHOPINFO pShop;
	PITEMINFO pItem;
	DWORD dwItemID;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];

	if (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];

	if (pPara->dwData1 >= (DWORD)pMap->byCountShop) {
		return;
	}
	pShop = &pMap->pShopInfo[pPara->dwData1];

	if (pPara->dwData2 >= MAX_SHOPITEM) {
		return;
	}
	dwItemID = pShop->awItem[pPara->dwData2];

	if (dwItemID >= (DWORD)g_aItemInfo.GetSize ()) {
		return;
	}
	pItem = g_aItemInfo[dwItemID];

	/* お金が足りない？ */
	if (pChar->dwGold < pItem->dwGold) {
		return;
	}

	pChar->dwGold -= pItem->dwGold;
	SendCmdCHARSTATUSGOLDEXP (pPara->pID, pPara->dwTblID);

	AddCharItem (pChar->dwTblID, dwItemID);
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
}


/* ========================================================================= */
/* 関数名：RecvDataProcMAGICINFO											 */
/* 内容：受信データ処理(魔法情報)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void RecvDataProcMAGICINFO(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_MAGICINFO Para;
	PCMDPARA_MAGICINFO pPara = &Para;
	PMAGICINFO pInfo;
	int nCount;

	/* 名前と説明文は可変長で届くので構造体に戻す */
	if (SboPackGetMAGICINFO (&Para, pData, RecvEnd (pData)) == NULL) {
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

	/* 全員に更新を通知 */
	SendCmdMAGICINFO (URARASOCK_SENDALL, pPara->byID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICINFOREQ											 */
/* 内容：受信データ処理(魔法情報要求)										 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void RecvDataProcMAGICINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo;

	nNo = GetCharNo (pPara->pID);
	if (nNo < 0) {
		return;
	}

	SendCmdMAGICINFO (pPara->pID, pPara->byData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICUSERINFO										 */
/* 内容：受信データ処理(魔法利用者情報)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void RecvDataProcMAGICUSERINFO(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_MAGICUSERINFO Para;
	PCMDPARA_MAGICUSERINFO pPara = &Para;
	PMAGICUSERINFO pInfo;
	int nCount;
	PBYTE pDataNext;

	/* 名前は可変長で届くので構造体に戻す（後ろに職業 ID の表が続く） */
	pDataNext = SboPackGetMAGICUSERINFO (&Para, pData, RecvEnd (pData));
	if (pDataNext == NULL) {
		return;
	}
	if ((DWORD)(RecvEnd (pData) - pDataNext) < Para.byCount) {
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

	SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
	SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pPara->szName);

	/* 職業が 0 件の時も数を合わせる（表を消したのに数が残ると、送る時に消した表を読んでしまう） */
	pInfo->byCount = pPara->byCount;
	if (pPara->byCount) {
		pInfo->paJobIDTbl = new BYTE[pPara->byCount];
		ZeroMemory (pInfo->paJobIDTbl, pPara->byCount);
		CopyMemory (pInfo->paJobIDTbl, pDataNext, pPara->byCount);
	}

	/* 全員に更新を通知 */
	SendCmdMAGICUSERINFO (URARASOCK_SENDALL, pPara->byID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICUSERINFOREQ										 */
/* 内容：受信データ処理(魔法利用者情報要求)									 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
void RecvDataProcMAGICUSERINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo;

	nNo = GetCharNo (pPara->pID);
	if (nNo < 0) {
		return;
	}

	SendCmdMAGICUSERINFO (pPara->pID, pPara->byData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEQUUSERINFO											 */
/* 内容：受信データ処理(装備職業情報)										 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void RecvDataProcEQUUSERINFO(PBYTE pData, DWORD dwDataSize)
{
	CMDPARA_EQUUSERINFO Para;
	PCMDPARA_EQUUSERINFO pPara = &Para;
	PEQUUSERINFO pInfo;
	int nCount;
	PBYTE pDataNext;

	/* 名前は可変長で届くので構造体に戻す（後ろに職業 ID の表が続く） */
	pDataNext = SboPackGetEQUUSERINFO (&Para, pData, RecvEnd (pData));
	if (pDataNext == NULL) {
		return;
	}
	if ((DWORD)(RecvEnd (pData) - pDataNext) < Para.byCount) {
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

	SAFE_DELETE_ARRAY (pInfo->paJobIDTbl);
	SboTextCopy (pInfo->szName, sizeof (pInfo->szName), pPara->szName);

	/* 職業が 0 件の時も数を合わせる（表を消したのに数が残ると、送る時に消した表を読んでしまう） */
	pInfo->byCount = pPara->byCount;
	if (pPara->byCount) {
		pInfo->paJobIDTbl = new BYTE[pPara->byCount];
		ZeroMemory (pInfo->paJobIDTbl, pPara->byCount);
		CopyMemory (pInfo->paJobIDTbl, pDataNext, pPara->byCount);
	}

	/* 全員に更新を通知 */
	SendCmdEQUUSERINFO (URARASOCK_SENDALL, pPara->byID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcEQUUSERINFOREQ										 */
/* 内容：受信データ処理(装備職業情報要求)									 */
/* 日付：2004/07/17															 */
/* ========================================================================= */
void RecvDataProcEQUUSERINFOREQ(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	int nNo;

	nNo = GetCharNo (pPara->pID);
	if (nNo < 0) {
		return;
	}

	SendCmdEQUUSERINFO (pPara->pID, pPara->byData);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTRADE												 */
/* 内容：受信データ処理(取り引きコマンド)									 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void RecvDataProcTRADE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pRecv = (PCMDPARA_DATA3)pData;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	if (pRecv->dwData >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	if (pRecv->pID == 0) {
		std::map<DWORD, TRADESTATE>::iterator it = s_mapTrade.find (pRecv->dwTblID);
		/* 知らせる先は自分か、いま取り引きしている相手だけ */
		if ((pRecv->dwData != pRecv->dwTblID) &&
				((it == s_mapTrade.end ()) || (it->second.dwPartner != pRecv->dwData))) {
			return;
		}
		TradeClear (pRecv->dwTblID);
		/* 取り引き終了通知を送信 */
		SendCmdTRADE (g_aCharInfo[pRecv->dwData]->pID, pRecv->dwTblID, CMDTRADE_OUT);
		return;
	}
	/* 相手は近くにいる、ログイン中の自分以外のプレイヤーだけ */
	if ((pRecv->dwData == pRecv->dwTblID) ||
			(g_aCharInfo[pRecv->dwData]->szAcc[0] == 0) ||
			(g_aCharInfo[pRecv->dwData]->pID == 0) ||
			!IsNearRecvChar (g_aCharInfo[pRecv->dwData], 20)) {
		return;
	}
	if (g_aCharInfo[pRecv->dwData]->byCharState == CHARSTATE_DEAD) {
		/* 取り引き終了通知を送信 */
		SendCmdTRADE (g_aCharInfo[pRecv->dwTblID]->pID, pRecv->dwTblID, CMDTRADE_OUT);
		return;
	}

	/* サーバーでも取り引きの状態を覚える */
	TradeStart (pRecv->dwTblID, pRecv->dwData);

	/* それぞれに取り引き開始通知を送信 */
	SendCmdCHARITEM (g_aCharInfo[pRecv->dwTblID]->pID, pRecv->dwData);
	SendCmdTRADE (g_aCharInfo[pRecv->dwTblID]->pID, pRecv->dwData, CMDTRADE_IN);
	SendCmdCHARITEM (g_aCharInfo[pRecv->dwData]->pID, pRecv->dwTblID);
	SendCmdTRADE (g_aCharInfo[pRecv->dwData]->pID, pRecv->dwTblID, CMDTRADE_IN);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTRADEINFO											 */
/* 内容：受信データ処理(取り引き情報)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void RecvDataProcTRADEINFO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_TRADEINFO pRecv = (PCMDPARA_TRADEINFO)pData;
	PCMDPARA_TRADEINFO pSendData;
	TRADESTATE *pState, *pStatePartner;

	/* 送り先（dwTblID）が、送ってきた人のいまの取り引き相手のときだけ */
	if ((s_nCharNoRecv < 0) || (pRecv->dwTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return;
	}
	if (!TradeGetPair ((DWORD)s_nCharNoRecv, pRecv->dwTblID, &pState, &pStatePartner)) {
		return;
	}
	/* 出すものが変わったら、相手の「決定」は取り消す（見ていない内容で成立しないように） */
	if ((pState->dwGold != pRecv->dwGold) ||
			(memcmp (pState->waItem, pRecv->waItem, sizeof (pState->waItem)) != 0)) {
		pStatePartner->byMode = 0;
	}
	pState->byMode	= pRecv->byMode;
	pState->dwGold	= pRecv->dwGold;
	CopyMemory (pState->waItem, pRecv->waItem, sizeof (pState->waItem));

	pSendData = new CMDPARA_TRADEINFO;
	ZeroMemory (pSendData, sizeof (CMDPARA_TRADEINFO));
	pSendData->byCmd = CMD_TRADEINFO;
	pSendData->byMode = pRecv->byMode;
	pSendData->dwGold = pRecv->dwGold;
	pSendData->bySum = pRecv->bySum;
	CopyMemory (pSendData->waItem, pRecv->waItem, sizeof (pRecv->waItem));

	svrSock.SendTo (g_aCharInfo[pRecv->dwTblID]->pID, (PBYTE)pSendData, sizeof (CMDPARA_TRADEINFO));
	delete pSendData;
}

/* ========================================================================= */
/* 関数名：RecvDataProcTRADEPROC											 */
/* 内容：受信データ処理(取り引き決定)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void RecvDataProcTRADEPROC(PBYTE pData, DWORD dwDataSize)
{
	int i, j, k;
	PCMDPARA_TRADEPROC pRecv = (PCMDPARA_TRADEPROC)pData;
	PITEMINFO pItem;
	PCHARINFO pChar1, pChar2;
	WORD waItemTmp1[MAX_MYITEM],
		 waItemTmp2[MAX_MYITEM],
		 waEquTmp1[10],
		 waEquTmp2[10];
	BYTE bySum1, bySum2;
	DWORD dwGold1, dwGold2;
	BOOL bMax;
	char szTmp[256];

	bMax = FALSE;

	if (!IsRecvChar (pRecv->dwTblID1)) {
		return;
	}
	if (pRecv->dwTblID2 >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}

	pChar1 = g_aCharInfo[pRecv->dwTblID1];
	pChar2 = g_aCharInfo[pRecv->dwTblID2];

	/* サーバーが覚えている取り引きで、両方が「決定」している時だけ成立させる。
	   渡すアイテムと金額も、パケットの申告ではなく覚えている内容を使う */
	{
		TRADESTATE *pState1, *pState2;

		if (!TradeGetPair (pRecv->dwTblID1, pRecv->dwTblID2, &pState1, &pState2)) {
			return;
		}
		if ((pChar1->pID == 0) || (pChar2->pID == 0)) {
			return;
		}
		/* 決定の後で出すものが変わった（相手の決定が取り消された）なら、取り引きごとやめる。
		   クライアントは送った時点で取り引き画面を閉じるので、待たせたままにしない */
		if ((pState1->byMode != 1) || (pState2->byMode != 1)) {
			TradeClear (pRecv->dwTblID1);
			SendCmdTRADE (pChar1->pID, pChar1->dwTblID, CMDTRADE_OUT);
			SendCmdTRADE (pChar2->pID, pChar2->dwTblID, CMDTRADE_OUT);
			wsprintf (szTmp, "決定の後で内容が変わったので、取り引きを取り消しました。");
			SendCmdSYSMSG (pChar1->pID, szTmp, SPEECHCMD_BATTLE1);
			SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
			return;
		}
		/* お金が足りない・持っていないアイテム・同じアイテムを 2 回？ */
		if ((pState1->dwGold > pChar1->dwGold) || (pState2->dwGold > pChar2->dwGold) ||
				!TradeCheckItem (pChar1, pState1->waItem) || !TradeCheckItem (pChar2, pState2->waItem)) {
			TradeClear (pRecv->dwTblID1);
			SendCmdTRADE (pChar1->pID, pChar1->dwTblID, CMDTRADE_OUT);
			SendCmdTRADE (pChar2->pID, pChar2->dwTblID, CMDTRADE_OUT);
			return;
		}
		CopyMemory (pRecv->waItem1, pState1->waItem, sizeof (pRecv->waItem1));
		CopyMemory (pRecv->waItem2, pState2->waItem, sizeof (pRecv->waItem2));
		pRecv->dwGold1 = pState1->dwGold;
		pRecv->dwGold2 = pState2->dwGold;
		/* 1 回の取り引きは 1 回だけ成立させる */
		TradeClear (pRecv->dwTblID1);
	}

	/* チェックサムの算出 */
	bySum1 = (BYTE)pChar1->dwGold;
	bySum2 = (BYTE)pChar2->dwGold;
	for (i = 0; i < MAX_MYITEM; i ++) {
		bySum1 += (BYTE)pChar1->waItem[i];
		bySum2 += (BYTE)pChar2->waItem[i];
	}

	/* チェックサムの確認 */
	if ((pRecv->bySum1 != bySum1) ||
		(pRecv->bySum2 != bySum2)) {
		/* どうやら終わった取り引きらしい */
		return;
	}

	CopyMemory (waItemTmp1, pChar1->waItem, sizeof (pChar1->waItem));
	CopyMemory (waItemTmp2, pChar2->waItem, sizeof (pChar2->waItem));
	CopyMemory (waEquTmp1, pChar1->waEqu, sizeof (pChar1->waEqu));
	CopyMemory (waEquTmp2, pChar2->waEqu, sizeof (pChar2->waEqu));
	ZeroMemory (pChar1->waItem, sizeof (pChar1->waItem));
	ZeroMemory (pChar2->waItem, sizeof (pChar2->waItem));

	dwGold1 = pRecv->dwGold1;
	if (pChar2->dwGold + dwGold1 > 999999) {
		dwGold1 = 999999 - pChar2->dwGold;
		bMax = TRUE;
	}
	dwGold2 = pRecv->dwGold2;
	if (pChar1->dwGold + dwGold2 > 999999) {
		dwGold2 = 999999 - pChar1->dwGold;
		bMax = TRUE;
	}

	pChar1->dwGold -= dwGold1;
	pChar2->dwGold -= dwGold2;
	/* 一応マイナス対応 */
	if ((int)pChar1->dwGold < 0) {
		pChar1->dwGold = 0;
	}
	if ((int)pChar2->dwGold < 0) {
		pChar2->dwGold = 0;
	}

	pChar1->dwGold += dwGold2;
	pChar2->dwGold += dwGold1;

	/* 渡すアイテムを整理 */
	for (i = 0; i < MAX_MYITEM; i ++) {
		for (j = 0; j < MAX_MYITEM; j ++) {
			/* 渡すアイテム？ */
			if (i == pRecv->waItem1[j]) {
				break;
			}
		}
		if (j == MAX_MYITEM) {
			AddCharItem (pChar1->dwTblID, waItemTmp1[i]);
		} else {
			for (k = 0; k < 10; k ++) {
				if (pChar1->waEqu[k] != 0xFFFF) {
					if (waEquTmp1[k] >= i) {
						pChar1->waEqu[k] --;
					}
				}
			}
			pChar1->waEqu[9] = 0xFFFF;
		}

		for (j = 0; j < MAX_MYITEM; j ++) {
			if (i == pRecv->waItem2[j]) {
				break;
			}
		}
		if (j == MAX_MYITEM) {
			AddCharItem (pChar2->dwTblID, waItemTmp2[i]);
		} else {
			for (k = 0; k < 10; k ++) {
				if (pChar2->waEqu[k] != 0xFFFF) {
					if (waEquTmp2[k] >= i) {
						pChar2->waEqu[k] --;
					}
				}
			}
			pChar2->waEqu[9] = 0xFFFF;
		}
	}

	/* 受け取るアイテムを追加 */
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pRecv->waItem1[i] != 0xFFFF) {
			/* もう持てない？ */
			if (GetItemCount (pChar2->dwTblID) >= MAX_MYITEM) {
				AddCharItem (pChar1->dwTblID, waItemTmp1[pRecv->waItem1[i]]);
				bMax = TRUE;
			} else {
				pItem = g_aItemInfo[waItemTmp1[pRecv->waItem1[i]]];
				/* 取引不可？ */
				if (pItem->byFlg & ITEMFLG_TRADE) {
					AddCharItem (pChar1->dwTblID, waItemTmp1[pRecv->waItem1[i]]);
					bMax = TRUE;
					wsprintf (szTmp, "[%s]は渡せません。", pItem->szName);
					SendCmdSYSMSG (pChar1->pID, szTmp, SPEECHCMD_BATTLE1);
				} else {
					AddCharItem (pChar2->dwTblID, waItemTmp1[pRecv->waItem1[i]]);
				}
			}
		}
		if (pRecv->waItem2[i] != 0xFFFF) {
			/* もう持てない？ */
			if (GetItemCount (pChar1->dwTblID) >= MAX_MYITEM) {
				AddCharItem (pChar2->dwTblID, waItemTmp2[pRecv->waItem2[i]]);
				bMax = TRUE;
			} else {
				pItem = g_aItemInfo[waItemTmp2[pRecv->waItem2[i]]];
				/* 取引不可？ */
				if (pItem->byFlg & ITEMFLG_TRADE) {
					AddCharItem (pChar2->dwTblID, waItemTmp2[pRecv->waItem2[i]]);
					bMax = TRUE;
					wsprintf (szTmp, "[%s]は渡せません。", pItem->szName);
					SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
				} else {
					AddCharItem (pChar1->dwTblID, waItemTmp2[pRecv->waItem2[i]]);
				}
			}
		}
	}

	/* 取り引き終了通知を送信 */
	SendCmdTRADE (pChar1->pID, pChar1->dwTblID, CMDTRADE_OUT);
	SendCmdTRADE (pChar2->pID, pChar2->dwTblID, CMDTRADE_OUT);
	SendCmdCHARITEM (pChar1->pID, pChar1->dwTblID);
	SendCmdCHARITEM (pChar2->pID, pChar2->dwTblID);
	SendCmdCHARSTATUS (pChar1->pID, pChar1->dwTblID);
	SendCmdCHARSTATUS (pChar2->pID, pChar2->dwTblID);
	if (bMax == FALSE) {
		wsprintf (szTmp, "取り引きが成立しました。");
		SendCmdSYSMSG (pChar1->pID, szTmp, SPEECHCMD_BATTLE1);
		SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
	} else {
		wsprintf (szTmp, "取り引きは成立しましたが");
		SendCmdSYSMSG (pChar1->pID, szTmp, SPEECHCMD_BATTLE1);
		SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
		wsprintf (szTmp, "渡せなかったものがあります。ご確認ください。");
		SendCmdSYSMSG (pChar1->pID, szTmp, SPEECHCMD_BATTLE1);
		SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEGOLD											 */
/* 内容：受信データ処理(お金預ける)											 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void RecvDataProcLEAVEGOLD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pRecv = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;
	PLEAVEINFO pInfo = NULL;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	if (pRecv->dwData > pChar->dwGold) {
		return;
	}

	if (g_aLeaveInfo.GetSize () == 0) {
		return;
	} else if (g_aLeaveInfo.GetSize () == 1) {
	} else if (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ()) {
	} else {
		if (pChar->dwLeaveID) {
			pInfo = g_aLeaveInfo[pChar->dwLeaveID];
			/* もう預けられない？ */
			if (pInfo->dwGold + pRecv->dwData >= MAX_LEAVEGOLD) {
				return;
			}
		}
	}
	int i;
	/* 口座（？）が無い？ */
	if (pInfo == NULL) {
		i = -1;
		if (g_aLeaveInfo.GetSize () >= 2) {
			for (i = 1; i < g_aLeaveInfo.GetSize (); i ++) {
				PLEAVEINFO pInfoTmp = g_aLeaveInfo[i];
				if (pInfoTmp->byUse == 0) {
					break;
				}
			}
		}
		if (i >= 1 && i < g_aLeaveInfo.GetSize ()) {
			pInfo = g_aLeaveInfo[i];
		} else {
			i = g_aLeaveInfo.GetSize ();
			pInfo = new LEAVEINFO;
			g_aLeaveInfo.Add (pInfo);
		}
		ZeroMemory (pInfo, sizeof (LEAVEINFO));
		pInfo->byUse = 1;
		pChar->dwLeaveID = i;
	}

	if (pRecv->dwData <= pChar->dwGold) {
		pInfo->dwGold += pRecv->dwData;
		pChar->dwGold -= pRecv->dwData;
		SendCmdCHARSTATUSGOLDEXP (pRecv->pID, pRecv->dwTblID);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEGOLDGET											 */
/* 内容：受信データ処理(預けてるお金取得)									 */
/* 日付：2003/12/13															 */
/* ========================================================================= */
void RecvDataProcLEAVEGOLDGET(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA3 pRecv = (PCMDPARA_DATA3)pData;
	PCHARINFO pChar;
	PLEAVEINFO pInfo = NULL;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	if (g_aLeaveInfo.GetSize () == 0) {
		return;
	} else if (g_aLeaveInfo.GetSize () == 1) {
	} else if (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ()) {
	} else {
		if (pChar->dwLeaveID) {
			pInfo = g_aLeaveInfo[pChar->dwLeaveID];
		}
	}

	/* 金額取得？ */
	if (pRecv->dwData == -1) {
		SendCmdLEAVEGOLDGET (pRecv->dwTblID);
		return;
	}

	if (pInfo && (pInfo->dwGold >= pRecv->dwData)) {
		pInfo->dwGold -= pRecv->dwData;
		pChar->dwGold += pRecv->dwData;
		SendCmdCHARSTATUSGOLDEXP (pRecv->pID, pRecv->dwTblID);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSETSTARTPOS											 */
/* 内容：受信データ処理(復活座標設定)										 */
/* 日付：2004/01/23															 */
/* ========================================================================= */
void RecvDataProcSETSTARTPOS(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_CHARPOS pRecv = (PCMDPARA_CHARPOS)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];

	if (pRecv->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, (LPARAM)pChar->dwTblID);
		return;
	}

	if (!CheckMsgCode (pChar, '8', (int)pRecv->dwMapID, (int)pRecv->byX, (int)pRecv->byY)) {
		return;
	}
	/* 復活位置を記録して町転送の行き先帳にも書き、一覧を送り直す。
	   行き先がいっぱいの時は、byFlg=1 なら byDirection の町を忘れる（無ければ一番古い町） */
	TownWarpRecord (pChar, pRecv->dwMapID, (DWORD)pRecv->byX, (DWORD)pRecv->byY,
		(pRecv->byFlg == 1) ? (int)pRecv->byDirection : -1);
	SendCmdTOWNWARP (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcCHGPASSWORD											 */
/* 内容：受信データ処理(パスワード更新)										 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
void RecvDataProcCHGPASSWORD(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ACCADD pPara = (PCMDPARA_ACCADD)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	TermRecvStr (pPara->szPas, sizeof (pPara->szPas));
	if (pPara->szPas[0] == 0) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	ZeroMemory ((PBYTE)pChar->szPas, sizeof (pChar->szPas));
	strcat (pChar->szPas, pPara->szPas);

	char szTmp[128];
	wsprintf (szTmp, "SYSTEM:パスワードを「%s」に変更しました。", pPara->szPas);
	SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
}

/* ========================================================================= */
/* 関数名：RecvDataProcDIFENCE												 */
/* 内容：受信データ処理(防御)												 */
/* 日付：2004/10/17															 */
/* ========================================================================= */
void RecvDataProcDIFENCE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFOTIMEREVENT pInfoTmp;
	PCHARINFO pChar;
	BOOL bResult;
	DWORD dwTmp;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 防御力に補正がかかっているかチェック */
	bResult = CheckCharInfoTimer (pPara->dwTblID, CHARINFOTIMEREVENT_DIFENCE);
	if (bResult) {
		return;
	}

	dwTmp = (DWORD)pChar->byDEX * 100 / 255;
	dwTmp = max (dwTmp, 1);
	dwTmp = 1000 + (4000 - ((4000 * dwTmp) / 100));

	pInfoTmp = new CHARINFOTIMEREVENT;
	ZeroMemory (pInfoTmp, sizeof (CHARINFOTIMEREVENT));
	pInfoTmp->byType		= CHARINFOTIMEREVENT_DIFENCE;
	pInfoTmp->byData		= pChar->byST;
	pInfoTmp->dwTimeStart	= timeGetTime ();
	pInfoTmp->dwTimeValid	= dwTmp;

	pChar->aTimerEvent.Add (pInfoTmp);

	RenewATDF (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTHROWSNOW											 */
/* 内容：受信データ処理(雪を投げる)											 */
/* 日付：2004/10/30															 */
/* ========================================================================= */
void RecvDataProcTHROWSNOW(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];

	PCHARINFO pCharNew = new CHARINFO;
	ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));

	pCharNew->byNpcType = NPCTYPE_ATACK;
	SetInitCharData (pCharNew);
	pCharNew->byGameState = GAMESTATE_NPC;
	pCharNew->dwXBack = pCharNew->dwX = pChar->dwX;
	pCharNew->dwYBack = pCharNew->dwY = pChar->dwY;
	pCharNew->dwMapID = pCharNew->dwMapID = pChar->dwMapID;
//Todo:画像ID設定
	pCharNew->wGrpID = 213;
	pCharNew->dwTimeLogin = timeGetTime ();
	pCharNew->dwParentTblID = pChar->dwTblID;
	pCharNew->wMaxHP = 0;
	pCharNew->waEqu[0] = 0;
	pCharNew->byDirection = pChar->byDirection;
	pCharNew->byMoveProbabilityX = 100;
	pCharNew->byMoveProbabilityY = 100;
	pCharNew->wHP = 5;

	pCharNew = AddChar (pCharNew);
	pCharNew->pID = pCharNew->dwTblID + 1;
	SendCmdCHARINFO3 (pCharNew->dwTblID);
}


/* ========================================================================= */
/* 関数名：RecvDataProcSUBGAME												 */
/* 内容：受信データ処理(サブゲーム用コマンド)								 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void RecvDataProcSUBGAME(PBYTE pData, DWORD dwDataSize)
{
	/* 関数テーブルの外を呼ばないように（テーブルの要素数） */
#define SUBGAME_TBLCOUNT(tbl)	(sizeof (tbl) / sizeof (tbl[0]))
	switch (pData[1]) {
	case SUBGAMEID_BOMB:	/* 爆弾ゲーム */
		if ((pData[2] < SUBGAME_TBLCOUNT (RecvDataProcSubGameTblBomb)) && RecvDataProcSubGameTblBomb[pData[2]]) {
			RecvDataProcSubGameTblBomb[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_MAZE:	/* 迷路 */
		if ((pData[2] < SUBGAME_TBLCOUNT (RecvDataProcSubGameTblMaze)) && RecvDataProcSubGameTblMaze[pData[2]]) {
			RecvDataProcSubGameTblMaze[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_DUNGEON:	/* ダンジョン */
		if ((pData[2] < SUBGAME_TBLCOUNT (RecvDataProcSubGameTblDungeon)) && RecvDataProcSubGameTblDungeon[pData[2]]) {
			RecvDataProcSubGameTblDungeon[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_WAR:		/* 戦場ゲーム */
		if ((pData[2] < SUBGAME_TBLCOUNT (RecvDataProcSubGameTblWar)) && RecvDataProcSubGameTblWar[pData[2]]) {
			RecvDataProcSubGameTblWar[pData[2]](pData, dwDataSize);
		}
		break;

	case SUBGAMEID_SUGOROKU:	/* すごろく */
		if ((pData[2] < SUBGAME_TBLCOUNT (RecvDataProcSubGameTblSugoroku)) && RecvDataProcSubGameTblSugoroku[pData[2]]) {
			RecvDataProcSubGameTblSugoroku[pData[2]](pData, dwDataSize);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_SETBOMB									 */
/* 内容：受信データ処理(爆弾設置)											 */
/* 日付：2003/11/09															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_SETBOMB(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_BOMB pSubStatus;
	PSUBGAMEINFO pInfo;
	PSUBGAME_BOMBINFO pBomb;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
	pInfo = g_aSubaGameInfo[pSubStatus->byID];
	pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

	/* 爆弾ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_BOMB) {
		return;
	}

	/* エリア外に設置しようとしてる？ */
	if (!((pChar->dwX >= pBomb->byX1) && (pChar->dwX <= pBomb->byX2) &&
		(pChar->dwY >= pBomb->byY1) && (pChar->dwY <= pBomb->byY2))) {
		SendCmdSYSMSG (pChar->pID, "SYSTEM:爆弾ゲーム参加解除しました。", SPEECHCMD_BATTLE1);
		if (pBomb->dwTblIDGameMaster == pChar->dwTblID) {
			SubGameProcBombQuitMsg (pSubStatus->byID, "マスターが抜けたので爆弾ゲーム終了");
			return;
		}
		SendCmdSubBOMBSTOP (pChar->pID, pSubStatus->byID, pBomb);
		ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));
		SendCmdSUBSTATUS (pChar->dwTblID);
		return;
	}

	/* 試合開始前？ */
	if (!pBomb->byFlgGameNow) {
		return;
	}

	if (pSubStatus->byCount >= pSubStatus->byMaxCount) {
		/* 今はこれ以上設置できない */
		return;
	}

	BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[pChar->dwX + pChar->dwY * pMap->nCX]])->byHitType;
	if (((byHitType != PARTSHIT_NONE) &&
		 (byHitType != PARTSHIT_MOVEFRONT) &&
		 (byHitType != PARTSHIT_MOVEUP) &&
		 (byHitType != PARTSHIT_MOVEDOWN) &&
		 (byHitType != PARTSHIT_MOVELEFT) &&
		 (byHitType != PARTSHIT_MOVERIGHT) &&
		 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
		 (byHitType != PARTSHIT_FOOTMARK_SAND) &&
		 (byHitType != PARTSHIT_WATERRIPPLE))) {
		return;
	}

	PCHARINFO pCharTmp = new CHARINFO;
	ZeroMemory ((PBYTE)pCharTmp, sizeof (CHARINFO));
	pCharTmp->byNpcType = NPCTYPE_BOMB;
	SetInitCharData (pCharTmp);
	pCharTmp->dwMapID = pChar->dwMapID;
	pCharTmp->dwX = pChar->dwX;
	pCharTmp->dwY = pChar->dwY;
	pCharTmp->byDirection = pChar->byDirection;
	pCharTmp->byGameState = GAMESTATE_NPC;
	pCharTmp->dwParentTblID = pChar->dwTblID;

	PSUBSTATUS_BOMB pSubStatusTmp = (PSUBSTATUS_BOMB)pCharTmp->abySubStatus;
	pSubStatusTmp->byID = pSubStatus->byID;
	pSubStatusTmp->byType = pSubStatus->byType;

	if (pBomb->byMaxFire == 0) {
		pCharTmp->wHP = ((WORD)genrand () % 10) + 1;
	} else {
		pCharTmp->wHP = pSubStatus->byPower;
	}
	pSubStatus->byCount ++;

	pCharTmp = AddChar (pCharTmp);
	pCharTmp->pID = pCharTmp->dwTblID + 1;
	SendCmdSubBOMBSETBOMB (pCharTmp->dwTblID);
	SendCmdPLAYSOUNDinside (pCharTmp->dwTblID, SOUND_BOMBSET);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_INFO										 */
/* 内容：受信データ処理(設定)												 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_INFO(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_BOMBINFO pRecv = (PSUBCMDPARA_BOMBINFO)pData;
	if (!IsRecvBombMaster (pRecv->byID)) {
		return;
	}
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[pRecv->byID];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

	/* 試合中？ */
	if (pBomb->byFlgGameNow) {
		return;
	}

	/* 壁使用する？ */
	if ((pBomb->byFlgUseBlock == 0) && (pBomb->byFlgUseBlock != pRecv->byFlgUseBlock)) {
		SubGameProcBombMakeBlock (pRecv->byID);
	/* 壁使用やめる */
	} else if ((pBomb->byFlgUseBlock == 1) && (pBomb->byFlgUseBlock != pRecv->byFlgUseBlock)) {
		SubGameProcBombDeleteBlock (pRecv->byID);
	}

	pBomb->byGameTime = pRecv->byGameTime;
	pBomb->byMaxBomb = pRecv->byMaxBomb;
	pBomb->byMaxFire = pRecv->byMaxFire;
	pBomb->byFlgUseBlock = pRecv->byFlgUseBlock;
	pBomb->byFlgUseItem = pRecv->byFlgUseItem;
	pBomb->byFlgProgress = pRecv->byFlgProgress;
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_START									 */
/* 内容：受信データ処理(試合開始)											 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_START(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	if (!IsRecvBombMaster (pRecv->byData)) {
		return;
	}
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[pRecv->byData];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pBomb->dwMapID];
	PDWORD pIDTbl = pMap->paTblNoPC;
	PCHARINFO pChar;

	/* 試合中？ */
	if (pBomb->byFlgGameNow) {
		return;
	}

	pBomb->byFlgGameNow = 1;
	pBomb->byGameTimeNow = pBomb->byGameTime;

	/* 参加者全員に送信 */
	for (int i = 0; ; i ++) {
		if (pIDTbl[i] == -1) {
			break;
		}
		pChar = g_aCharInfo[pIDTbl[i]];
		PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
		/* 爆弾ゲーム参加中？ */
		if (pSubStatus->byType == SUBGAMEID_BOMB) {
			/* ゲームIDが一致？ */
			if (pRecv->byData == pSubStatus->byID) {
				pSubStatus->byFlgEntry = 1;
				pSubStatus->byCount = 0;
				pSubStatus->byMaxCount = 1;
				pSubStatus->byPower = 3;
				pSubStatus->byFlgBlockThrough = 0;
				pSubStatus->byFlgBombThrough = 0;
				pSubStatus->byFlgRegistFire = 0;
				pSubStatus->byFlgRemocon = 0;
				if (pBomb->byFlgUseItem == 0) {
					pSubStatus->byMaxCount = pBomb->byMaxBomb;
					pSubStatus->byPower = pBomb->byMaxFire;
				}
				SendCmdSUBSTATUS (pChar->dwTblID);
				SendCmdSubBOMBINFO (pChar->pID, pRecv->byData, pBomb);
				SendCmdSYSMSG (pChar->pID, "爆弾ゲーム試合開始！", SPEECHCMD_BATTLE1);
				SendCmdPLAYSOUND (pChar->pID, SOUND_WHISTLE);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_STOP										 */
/* 内容：受信データ処理(試合中止)											 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_STOP(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;

	if (!IsRecvBombMaster (pRecv->byData)) {
		return;
	}
	SubGameProcBombQuitMsg (pRecv->byData, "マスターが試合を中止しました");
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_GETITEM									 */
/* 内容：受信データ処理(アイテム取得)										 */
/* 日付：2003/12/29															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_GETITEM(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA2 pRecv = (PSUBCMDPARA_DATA2)pData;
	PCHARINFO pChar, pCharItem;
	PMAPINFO pMap;
	PSUBSTATUS_BOMB pSubStatus;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	/* 爆弾ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_BOMB) {
		return;
	}

	PSUBGAMEINFO pInfo = g_aSubaGameInfo[pChar->abySubStatus[1]];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

	if ((int)pRecv->dwData >= g_aCharInfo.GetSize ()) {
		return;
	}

	pCharItem = g_aCharInfo[pRecv->dwData];
	if ((pCharItem->byNpcType != NPCTYPE_BOMBITEM) ||
		(pChar->dwMapID != pCharItem->dwMapID) ||
		(pChar->dwX != pCharItem->dwX) ||
		(pChar->dwY != pCharItem->dwY)) {
		return;
	}
	switch (pCharItem->byEfect) {
	case 0:	/* 爆弾追加 */
		if (pSubStatus->byMaxCount < pBomb->byMaxBomb) {
			pSubStatus->byMaxCount ++;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 255, 213), "個数UP");
		}
		break;

	case 1:	/* 火力アップ */
		if (pSubStatus->byPower < pBomb->byMaxFire) {
			pSubStatus->byPower ++;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (255, 139, 213), "火力UP");
		}
		break;

	case 2:	/* リモコン */
		if (pSubStatus->byFlgRemocon == 0) {
			pSubStatus->byFlgRemocon = 1;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (213, 139, 255), "ﾘﾓｺﾝ");
		}
		break;

	case 3:	/* 壁すり抜け */
		if (pSubStatus->byFlgBlockThrough == 0) {
			pSubStatus->byFlgBlockThrough = 1;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (213, 139, 255), "壁すり抜け");
		}
		break;

	case 4:	/* 爆弾すり抜け */
		if (pSubStatus->byFlgBombThrough == 0) {
			pSubStatus->byFlgBombThrough = 1;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (213, 139, 255), "爆弾すり抜け");
		}
		break;

	case 5:	/* 即爆発 */
		{
			PDWORD pIDTbl = pMap->paTblNoNPC;
			for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
				if (pIDTbl[i] == -1) {
					break;
				}
				PCHARINFO pCharTmp = g_aCharInfo[pIDTbl[i]];
				if (pCharTmp->byNpcType != NPCTYPE_BOMB) {
					continue;
				}
				/* すでに爆発している？ */
				if (pCharTmp->byEfect > 0) {
					continue;
				}
				PSUBSTATUS_BOMB pSubStatusTmp = (PSUBSTATUS_BOMB)pCharTmp->abySubStatus;
				if (pSubStatusTmp->byID != pSubStatus->byID) {
					continue;
				}

				/* 爆発 */
				pCharTmp->dwLastDotMove = timeGetTime ();
				pCharTmp->byEfect ++;
				SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTmp->dwTblID);
				SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
				SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
				SendCmdPLAYSOUNDinside (pCharTmp->dwTblID, SOUND_BOMBBOMBER);
			}
		}
		break;

	case 6:	/* 初期に戻る */
		pSubStatus->byMaxCount = 1;
		pSubStatus->byPower = 3;
		pSubStatus->byFlgBlockThrough = 0;
		pSubStatus->byFlgBombThrough = 0;
		pSubStatus->byFlgRegistFire = 0;
		pSubStatus->byFlgRemocon = 0;
		SendCmdSUBSTATUS (pChar->dwTblID);
		SendCmdPLAYSOUND (pChar->pID, SOUND_FM001);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (213, 139, 255), "ﾊﾟﾜｰﾀﾞｳﾝ!");
		break;

	case 7:	/* 耐火 */
		if (pSubStatus->byFlgRegistFire == 0) {
			pSubStatus->byFlgRegistFire = 1;
			SendCmdSUBSTATUS (pChar->dwTblID);
			SendCmdPLAYSOUND (pChar->pID, SOUND_POWER21);
			SendCmdADDCHAREFC (pChar->dwTblID, RGB (213, 139, 255), "耐火");
		}
		break;
	}
	DeleteChar (pCharItem->dwTblID);
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameBOMB_REMOCON									 */
/* 内容：受信データ処理(リモコン使用)										 */
/* 日付：2003/12/29															 */
/* ========================================================================= */
void RecvDataProcSubGameBOMB_REMOCON(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_BOMB pSubStatus;
	PDWORD pIDTbl = NULL;

	if (!IsRecvChar (pRecv->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	/* 爆弾ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_BOMB) {
		return;
	}

	PSUBGAMEINFO pInfo = g_aSubaGameInfo[pSubStatus->byID];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

	pIDTbl = pMap->paTblNoNPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pIDTbl[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pIDTbl[i]];
		if (pCharTmp->byNpcType != NPCTYPE_BOMB) {
			continue;
		}
		if (pCharTmp->dwParentTblID != pChar->dwTblID) {
			continue;
		}
		/* すでに爆発している？ */
		if (pCharTmp->byEfect > 0) {
			continue;
		}
		/* 爆発 */
		pCharTmp->dwLastDotMove = timeGetTime ();
		pCharTmp->byEfect ++;
		SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTmp->dwTblID);
		SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
		SendCmdPLAYSOUNDinside (pCharTmp->dwTblID, SOUND_BOMBBOMBER);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameMAZE_MAZERENEW								 */
/* 内容：受信データ処理(迷路作成)											 */
/* 日付：2004/01/31															 */
/* ========================================================================= */
void RecvDataProcSubGameMAZE_MAZERENEW(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;

	/* 管理者ウィンドウから送るものなので管理者だけ */
	if (!s_bAdminRecv || (pRecv->byData >= g_aSubaGameInfo.GetSize ())) {
		return;
	}
	MakeMazeProc (pRecv->byData);
}


/* ========================================================================= */
/* 関数名：RecvDataProcSubGameDUNGEON_DUNGEONRENEW							 */
/* 内容：受信データ処理(ダンジョン作成)										 */
/* 日付：2004/02/26															 */
/* ========================================================================= */
void RecvDataProcSubGameDUNGEON_DUNGEONRENEW(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;

	/* 管理者ウィンドウから送るものなので管理者だけ */
	if (!s_bAdminRecv || (pRecv->byData >= g_aSubaGameInfo.GetSize ())) {
		return;
	}
	MakeDungeonProc (pRecv->byData);
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_SELECTTEAM								 */
/* 内容：	受信データ処理(戦場ゲーム：チーム選択)							 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_SELECTTEAM(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256], szTmp[256];
	int nTmp, nState;
	DWORD dwX, dwY;
	POINT ptStart;
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}

	nState = pProcSubGameWAR->GetState ();
	/* 受付状態ではない？ */
	if (!((nState == SUBGAME_WAR_STATE_READY) || (nState == SUBGAME_WAR_STATE_JOIN))) {
		wsprintf (szMsg, "すでに参加は締め切られています。@2");
		pProcSubGameWAR->RemoveAndMove (pRecv->dwTblID);
		goto Exit;
	}

	/* 参加済み？ */
	nTmp = pProcSubGameWAR->IsJoin (pRecv->dwTblID);
	if (nTmp >= 0) {
		wsprintf (szMsg, "他プレイヤーの参加をお待ちください。@2");
		goto Exit;
	}

	/* 定員オーバー？ */
	/* 1チームの定員は設定(3人/4人)に合わせる（HP 表示や待機場所もこの人数分しかない） */
	nTmp = pProcSubGameWAR->GetMemberCount (pRecv->byData);
	if (nTmp >= pProcSubGameWAR->GetMemberType ()) {
		wsprintf (szMsg, "そちらのチームは満員のようです。@2");
		goto Exit;
	}

	pProcSubGameWAR->AddMember (pRecv->byData, pRecv->dwTblID);
	wsprintf (szMsg, "参加を受け付けました。@1他プレイヤーの参加をお待ちください。@2");

	wsprintf (szTmp, "%sが", pChar->szName);
	if (pRecv->byData == 0) {
		strcat (szTmp, "赤チーム");
	} else {
		strcat (szTmp, "青チーム");
	}
	strcat (szTmp, "に参加しました。");
	pProcSubGameWAR->JudgeMsg (szTmp);

	/* 状態遷移 */
	pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	/* 開始地点へ飛ばす */
	SendCmdFADEINOUT (pChar->pID, 0);
	pChar->byDirection		= 1;
	pChar->byDirectionBack2	= pChar->byDirection;
	pChar->byDirectionBack	= pChar->byDirection;

	pProcSubGameWAR->GetStartPos (pRecv->byData, ptStart);
	CharTargetCancelPC (pChar);
	dwX				= ptStart.x;
	dwY				= ptStart.y;
	pChar->dwX		= dwX;
	pChar->dwY		= dwY;
	pChar->dwXBack	= dwX;
	pChar->dwYBack	= dwY;
	pSubStatus->byFlgTeam = pRecv->byData;
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
	SendCmdFADEINOUT (pChar->pID, 1);

	/* 最初の参加者？ */
	nTmp = pProcSubGameWAR->GetAllMemberCount ();
	if (nTmp == 1) {
		pProcSubGameWAR->ChgState (SUBGAME_WAR_STATE_JOIN);
	}

Exit:
	if (pChar && (strlen (szMsg) > 0)) {
		SendCmdMAPMSG (pChar->pID, szMsg);
	}
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_QUIT										 */
/* 内容：	受信データ処理(戦場ゲーム：ゲームから抜ける)					 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_QUIT(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	int nTmp;
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}

	/* 参加していない？ */
	nTmp = pProcSubGameWAR->IsJoin (pRecv->dwTblID);
	if (nTmp < 0) {
		goto Exit;
	}

	pProcSubGameWAR->RemoveAndMove (pRecv->dwTblID);

Exit:
	if (pChar && (strlen (szMsg) > 0)) {
		SendCmdMAPMSG (pChar->pID, szMsg);
	}
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_SELECTRULE								 */
/* 内容：	受信データ処理(戦場ゲーム：ルール選択)							 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_SELECTRULE(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}

	/* 受付状態ではない？ */
	if (pProcSubGameWAR->GetState () != SUBGAME_WAR_STATE_SELECTRULE) {
		goto Exit;
	}

	/* ルール変更する？ */
	if (pProcSubGameWAR->GetRule () != pRecv->byData) {
		pProcSubGameWAR->SetRule (pRecv->byData);
		if (pRecv->byData == 0) {
			wsprintf (szMsg, "ルールが[全滅させる][自動復活無し]に設定されました。");
		} else {
			wsprintf (szMsg, "ルールが[城へ到達][自動復活有り]に設定されました。");
		}
		pProcSubGameWAR->JudgeMsg (szMsg);
	}

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_SELECTJOB								 */
/* 内容：	受信データ処理(戦場ゲーム：職業選択)							 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_SELECTJOB(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	/* 設定範囲外？ */
	if (pRecv->byData >= SUBGAME_WAR_JOB_MAX) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}

	/* 受付状態ではない？ */
	if (pProcSubGameWAR->GetState () != SUBGAME_WAR_STATE_SELECTJOB) {
		goto Exit;
	}

	/* 職業変更する？ */
	if (pSubStatus->byJob != pRecv->byData) {
		LPCSTR apszJobNameTbl[] = {
			"戦士", "武道家", "魔法使い", "僧侶"
		};
		pSubStatus->byJob = pRecv->byData;

		wsprintf (szMsg, "%sは[%s]になりました。", pChar->szName, apszJobNameTbl[pRecv->byData]);

		pProcSubGameWAR->JudgeMsg (szMsg);
		SendCmdSUBSTATUS (pChar->dwTblID);
	}

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_SELECTTURN								 */
/* 内容：	受信データ処理(戦場ゲーム：順番選択)							 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_SELECTTURN(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	int nTmp;
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	/* 設定範囲外？ */
	if (pRecv->byData >= SUBGAME_WAR_JOB_MAX) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pProcSubGameWAR->GetState () != SUBGAME_WAR_STATE_SELECTTURN) {
		goto Exit;
	}

	/* チームがわからない？ */
	nTmp = pProcSubGameWAR->IsJoin (pChar->dwTblID);
	if (nTmp < 0) {
		goto Exit;
	}

	/* 順番変更 */
	pProcSubGameWAR->UpTurn ((BYTE)nTmp, pRecv->byData);
	SendCmdSubWarSELECTTURN (pInfoSubGameWAR, (BYTE)nTmp);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_WAIT										 */
/* 内容：	受信データ処理(戦場ゲーム：待機)								 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_WAIT(PBYTE pData, DWORD dwDataSize)
{
	int nTeam, x, y, nTmp;
	char szMsg[256];
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pSubStatus->byState != SUBGAME_WAR_STATE_TURN) {
		goto Exit;
	}

	/* 回復する？ */
	if (pRecv->byData > 0) {
		pProcSubGameWAR->HPUp (pRecv->dwTblID, pRecv->byData);
	}

	pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	/* 城へ到達ルール？ */
	if (pProcSubGameWAR->GetRule () == 1) {
		nTeam = pProcSubGameWAR->IsJoin (pChar->dwTblID);
		if (nTeam != 0) {
			pTeamInfo = pInfoSubGameWAR->GetTeamInfo (0);
		} else {
			pTeamInfo = pInfoSubGameWAR->GetTeamInfo (1);
		}
		x = pTeamInfo->ptCastle.x;
		y = pTeamInfo->ptCastle.y;

		/* 城の広さ: 4人戦は 2×2 マス、3人戦は 1 マス（以前は 3人戦で nTmp が初期化されていなかった） */
		nTmp = 0;
		if (pInfoSubGameWAR->m_byMember == SUBGAME_WAR_MEMBER_4) {
			nTmp = 1;
		}

		/* 城に到達？ */
		if ((x <= (int)pChar->dwX) && (x + nTmp >= (int)pChar->dwX)) {
			if ((y <= (int)pChar->dwY) && (y + nTmp >= (int)pChar->dwY)) {
				wsprintf (szMsg, "%sが城に到達しました！", pChar->szName);
				pProcSubGameWAR->JudgeMsg (szMsg);
				pProcSubGameWAR->JudgeMsg ("試合終了！");
				/* チーム１ */
				if (nTeam == 0) {
					strcpy (szMsg, "赤");
				} else {
					strcpy (szMsg, "青");
				}
				strcat (szMsg, "チームの勝利！");
				pProcSubGameWAR->JudgeMsg (szMsg);

				pProcSubGameWAR->Quit ();
			}
		}
	}

	pProcSubGameWAR->NextTurn ();
Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_ATACK									 */
/* 内容：	受信データ処理(戦場ゲーム：攻撃)								 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_ATACK(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	DWORD dwTblID;
	BOOL bCheck;
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar, pCharTarget;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus, pSubStatusTarget;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pSubStatus->byState != SUBGAME_WAR_STATE_TURN) {
		goto Exit;
	}

	bCheck = TRUE;
	dwTblID = CheckCharBlockPC (pChar->dwMapID,
					pChar->dwX + anPosAddTblX[pRecv->byData],
					pChar->dwY + anPosAddTblY[pRecv->byData]);
	if (dwTblID == -1) {
		bCheck = FALSE;

	} else {
		pMemberInfo = pInfoSubGameWAR->GetMemberInfo (dwTblID);
		if ((pMemberInfo == NULL) || (pMemberInfo->dwTblIDRes != -1)) {
			SendCmdSubWarCHGSTATE (pChar->pID, SUBGAME_WAR_STATE_TURNCONTINUE);
			goto Exit;
		}
		/* 同じチーム？ */
		if (pProcSubGameWAR->IsJoin (dwTblID) == pProcSubGameWAR->IsJoin (pChar->dwTblID)) {
			SendCmdSYSMSG (pChar->pID, "SYSTEM:同チームメンバーには攻撃できません。", SPEECHCMD_BATTLE1);
			SendCmdSubWarCHGSTATE (pChar->pID, SUBGAME_WAR_STATE_TURNCONTINUE);
			return;
		}
	}

	if (bCheck == FALSE) {
		SendCmdSubWarCHGSTATE (pChar->pID, SUBGAME_WAR_STATE_TURNCONTINUE);
		goto Exit;
	}

	pCharTarget = g_aCharInfo[dwTblID];
	pSubStatusTarget = (PSUBSTATUS_WAR)pCharTarget->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatusTarget->byType != SUBGAMEID_WAR) {
		SendCmdSubWarCHGSTATE (pChar->pID, SUBGAME_WAR_STATE_TURNCONTINUE);
		goto Exit;
	}

	wsprintf (szMsg, "%sの%sへの攻撃！", pChar->szName, pCharTarget->szName);
	pProcSubGameWAR->JudgeMsg (szMsg);

	wsprintf (szMsg, "%sは防御行動を選んでください。", pCharTarget->szName);
	pProcSubGameWAR->JudgeMsg (szMsg);

	pInfoSubGameWAR->m_dwWaitTblID = pCharTarget->dwTblID;
	pSubStatusTarget->byState = SUBGAME_WAR_STATE_SELECTDIFENCE;
	SendCmdSubWarCHGSTATE (pCharTarget->pID, pSubStatusTarget->byState);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_DIFENCE									 */
/* 内容：	受信データ処理(戦場ゲーム：防御行動)							 */
/* 日付：	2004/08/13														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_DIFENCE(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pSubStatus->byState != SUBGAME_WAR_STATE_SELECTDIFENCE) {
		goto Exit;
	}

	pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	/* 攻撃処理 */
	pProcSubGameWAR->Atack (pRecv->dwTblID, pRecv->byData);

	pInfoSubGameWAR->m_dwWaitTblID = -1;
	pProcSubGameWAR->NextTurn ();
Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_MAGIC									 */
/* 内容：	受信データ処理(戦場ゲーム：魔法)								 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_MAGIC(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	PSUBCMDPARA_DATA3 pRecv = (PSUBCMDPARA_DATA3)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pSubStatus->byState != SUBGAME_WAR_STATE_TURN) {
		goto Exit;
	}

	/* 一覧作成 */
	switch (pRecv->byData1) {
	case SUBGAME_WAR_MAGIC_FIRE:			/* 炎 */
		pProcSubGameWAR->UseMagicFire (pRecv->byData2);
		break;
	case SUBGAME_WAR_MAGIC_QURE:			/* 回復 */
		pProcSubGameWAR->UseMagicQure (pRecv->byData2);
		break;
	case SUBGAME_WAR_MAGIC_RESUSCITATION:	/* 蘇生 */
		pProcSubGameWAR->UseMagicResuscitation (pRecv->byData2);
		break;
	default:
		goto Exit;
	}

	pProcSubGameWAR->NextTurn ();
Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	RecvDataProcSubGameWAR_MAGIC_TARGET								 */
/* 内容：	受信データ処理(戦場ゲーム：魔法対象者一覧)						 */
/* 日付：	2004/08/15														 */
/* ========================================================================= */

void RecvDataProcSubGameWAR_MAGIC_TARGET(PBYTE pData, DWORD dwDataSize)
{
	char szMsg[256];
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_WAR pSubStatus;
	PCMDPARA_WARSELECTMAGICTARGET pTarget;
	PCProcSubGameWAR pProcSubGameWAR;
	PCInfoSubGameWAR pInfoSubGameWAR;

	pChar = NULL;
	pTarget = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar		= g_aCharInfo[pRecv->dwTblID];
	pMap		= g_aMapInfo[pChar->dwMapID];
	pSubStatus	= (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 戦場ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_WAR) {
		goto Exit;
	}

	pProcSubGameWAR = (PCProcSubGameWAR)g_ProcSubGame.Get (pSubStatus->byID);
	if (pProcSubGameWAR == NULL) {
		goto Exit;
	}
	pInfoSubGameWAR = pProcSubGameWAR->GetInfo ();

	/* 受付状態ではない？ */
	if (pSubStatus->byState != SUBGAME_WAR_STATE_TURN) {
		goto Exit;
	}

	pTarget = new CMDPARA_WARSELECTMAGICTARGET;
	ZeroMemory (pTarget, sizeof (CMDPARA_WARSELECTMAGICTARGET));

	/* 一覧作成 */
	switch (pRecv->byData) {
	case SUBGAME_WAR_MAGIC_FIRE:			/* 炎 */
		pProcSubGameWAR->MakeTargetFire (pTarget);
		break;
	case SUBGAME_WAR_MAGIC_QURE:			/* 回復 */
		pProcSubGameWAR->MakeTargetQure (pTarget);
		break;
	case SUBGAME_WAR_MAGIC_RESUSCITATION:	/* 蘇生 */
		pProcSubGameWAR->MakeTargetResuscitation (pTarget);
		break;
	default:
		goto Exit;
	}

	SendCmdSubWarMAGIC_TARGET (pChar->pID, pTarget);
Exit:
	SAFE_DELETE (pTarget);
}


/* ========================================================================= */
/* 関数名：RecvDataProcSubGameSUGOROKU_JOINMODE								 */
/* 内容：受信データ処理(すごろく：モード選択)								 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void RecvDataProcSubGameSUGOROKU_JOINMODE(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_SUGOROKU pSubStatus;
	PSUBGAMEINFO pInfo;
	PSUBGAME_SUGOROKUINFO pSugoroku;
	char szMsg[256], szTmp[256];
	int nTmp, nNo;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	/*すごろくに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_SUGOROKU) {
		goto Exit;
	}

	pInfo = g_aSubaGameInfo[pSubStatus->byID];
	pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;

	/* 参加済み？ */
	nTmp = SubSugoroku_CheckJoin (pSugoroku, pRecv->dwTblID);
	if (nTmp >= 0) {
		wsprintf (szMsg, "他プレイヤーの参加をお待ちください。@2");
		goto Exit;
	}

	if (pRecv->byData == 0) {
		/* 定員オーバー？ */
		if (pSugoroku->adwPlayerTblID.GetSize () >= pSugoroku->byMaxCount) {
			wsprintf (szMsg, "満員のようです。@2");
			/* 状態遷移 */
			pSubStatus->byState = SUBGAME_SUGOROKU_STATE_INIT;
			SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);
			goto Exit;
		}
	}

	if (pRecv->byData == 0) {
		if (pSugoroku->byState > SUBGAME_SUGOROKU_STATE_JOIN) {
			wsprintf (szMsg, "ゲーム中は参加できません。@2");
			/* 状態遷移 */
			pSubStatus->byState = SUBGAME_SUGOROKU_STATE_INIT;
			SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);
			goto Exit;
		}
		wsprintf (szMsg, "参加を受け付けました。");
		wsprintf (szTmp, "%sが参加しました。", pChar->szName);
		SubSugoroku_Msg (pSugoroku, szTmp);
	} else {
		wsprintf (szMsg, "観戦者として参加を受け付けました。");
	}
	strcat (szMsg, "@1他プレイヤーの参加をお待ちください。@2");

	nNo = SubSugoroku_AddMember (pSugoroku, pRecv->byData, pRecv->dwTblID);

	/* 状態遷移 */
	pSubStatus->byState = SUBGAME_SUGOROKU_STATE_IDLE;
	SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);

	pSubStatus->byFlgViewer = (pRecv->byData == 0) ? 0 : 1;

	if (pRecv->byData == 0) {
		CharTargetCancelPC (pChar);
		/* 開始地点へ飛ばす */
		SendCmdFADEINOUT (pChar->pID, 0);
		pChar->byDirection = pSugoroku->aPlayerInfo[nNo].byDirection;
		pChar->byDirectionBack2 = pChar->byDirection;
		pChar->byDirectionBack = pChar->byDirection;

		CharTargetCancelPC (pChar);
		pChar->dwX = pSugoroku->aPlayerInfo[nNo].dwX;
		pChar->dwY = pSugoroku->aPlayerInfo[nNo].dwY;
		pChar->dwXBack = pChar->dwX;
		pChar->dwYBack = pChar->dwY;
		SendCmdCHARINFO (pChar->pID, -1);
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
		SendCmdFADEINOUT (pChar->pID, 1);
	}
	SendCmdSUBSTATUS (pChar->dwTblID);

	/* 最初の参加者？ */
	if (pSugoroku->adwPlayerTblID.GetSize () == 1) {
		pSugoroku->byState = SUBGAME_SUGOROKU_STATE_JOIN;
		pSugoroku->dwLastTimeProc = 0;
		pSugoroku->dwFirstTimeProc = timeGetTime ();
		pSugoroku->dwNextTimeProc = pSugoroku->dwFirstTimeProc + 10 * 1000;
	}

Exit:
	if (pChar && (strlen (szMsg) > 0)) {
		SendCmdMAPMSG (pChar->pID, szMsg);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameSUGOROKU_QUIT									 */
/* 内容：受信データ処理(すごろく：ゲームから抜ける)							 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void RecvDataProcSubGameSUGOROKU_QUIT(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_SUGOROKU pSubStatus;
	PSUBGAMEINFO pInfo;
	PSUBGAME_SUGOROKUINFO pSugoroku;
	char szMsg[256];
	int nTmp;

	pChar = NULL;
	ZeroMemory (szMsg, sizeof (szMsg));
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	/* すごろくに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_SUGOROKU) {
		goto Exit;
	}

	pInfo = g_aSubaGameInfo[pSubStatus->byID];
	pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;

	/* 参加していない？ */
	nTmp = SubSugoroku_CheckJoin (pSugoroku, pRecv->dwTblID);
	if (nTmp < 0) {
		goto Exit;
	}

	SubSugoroku_RemoveAndMove (pSugoroku, pRecv->dwTblID);

Exit:
	if (pChar && (strlen (szMsg) > 0)) {
		SendCmdMAPMSG (pChar->pID, szMsg);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcSubGameSUGOROKU_DICE									 */
/* 内容：受信データ処理(すごろく：サイコロを振る)							 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void RecvDataProcSubGameSUGOROKU_DICE(PBYTE pData, DWORD dwDataSize)
{
	PSUBCMDPARA_DATA1 pRecv = (PSUBCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	PMAPINFO pMap;
	PSUBSTATUS_SUGOROKU pSubStatus;
	PSUBGAMEINFO pInfo;
	PSUBGAME_SUGOROKUINFO pSugoroku;
	char szTmp[256];
	int nTmp;

	pChar = NULL;
	if (!IsRecvChar (pRecv->dwTblID)) {
		goto Exit;
	}

	pChar = g_aCharInfo[pRecv->dwTblID];
	pMap = g_aMapInfo[pChar->dwMapID];
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	/* すごろくに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_SUGOROKU) {
		goto Exit;
	}

	pInfo = g_aSubaGameInfo[pSubStatus->byID];
	pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;

	if (pSugoroku->byState != SUBGAME_SUGOROKU_STATE_WAIT) {
		goto Exit;
	}

	/* 参加していない？ */
	nTmp = SubSugoroku_GetNo (pSugoroku, pRecv->dwTblID);
	if (nTmp < 0) {
		goto Exit;
	}

	/* 行動順以外？ */
	if (nTmp != pSugoroku->byProcNo) {
		goto Exit;
	}

	pSugoroku->byLastMove = (BYTE)(genrand () % 6 + 1);
	wsprintf (szTmp, "%sのサイコロ！[%d]が出ました。",
			pChar->szName, pSugoroku->byLastMove);
	SubSugoroku_Msg (pSugoroku, szTmp);

	/* 状態遷移 */
	pSugoroku->byState = SUBGAME_SUGOROKU_STATE_MOVE;
	pSugoroku->dwLastTimeProc = 0;

Exit:
	return;
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
/* 日付：	2006/08/02														 */
/* ========================================================================= */

void RecvDataProcMAIN_CHAR_TAIL(PBYTE pData, DWORD dwDataSize)
{
	BOOL bResult;
	PCHARINFO pChar, pCharTarget;
	CCHAR_TAIL Packet;

	pChar = pCharTarget = NULL;

	Packet.Set (pData);

	if (!IsRecvChar (Packet.m_dwTblID)) {
		goto Exit;
	}
	pChar = g_aCharInfo[Packet.m_dwTblID];
	/* 誰かについていっている？ */
	if (pChar->dwTailTblID != -1) {
		/* ついていくのを解除 */
		TailOff (pChar);
		goto Exit;
	}

	if (Packet.m_dwTailTblID != -1) {
		if (Packet.m_dwTailTblID >= (DWORD)g_aCharInfo.GetSize ()) {
			goto Exit;
		}
		pCharTarget = g_aCharInfo[Packet.m_dwTailTblID];
		bResult = IsTail (pChar, pCharTarget);
		if (bResult == FALSE) {
			Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_NG2);
			svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
			goto Exit;
		}
		/* 傭兵を連れている人についていく時は、傭兵を先に列から外す
		   （外さないと傭兵が列から浮いたまま残る。傭兵は列の最後へ入り直す） */
		MercenaryTailOffBack (pCharTarget);
		/* 傭兵の後ろにほかの人がいた（その人が先についていっている） */
		if (pCharTarget->dwTailBackTblID != -1) {
			Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_NG2);
			svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
			goto Exit;
		}
		/* ついていく */
		TailOn (pChar, pCharTarget);

	} else {
		/* ついていくのを解除 */
		TailOff (pChar);
	}
Exit:
	return;
}

/* Copyright(C)URARA-works 2003-2005 */

/* ========================================================================= */
/* 関数名：FindWebAccountChar												 */
/* 内容：アカウント名からプレイヤーキャラを探す(ブラウザ版)					 */
/* ========================================================================= */
static int FindWebAccountChar(LPCSTR pszAcc)
{
	int i, nCount = g_aCharInfo.GetSize ();

	if ((pszAcc == NULL) || (pszAcc[0] == 0)) {
		return -1;
	}
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar->szAcc[0] && (strcmp (pChar->szAcc, pszAcc) == 0)) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：RecvDataProcWEBACCOUNT											 */
/* 内容：受信データ処理(ブラウザ版のアカウント関係)							 */
/* ========================================================================= */
void RecvDataProcWEBACCOUNT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_WEBACCOUNT pPara = (PCMDPARA_WEBACCOUNT)pData;
	DWORD pID = dwDataSize;				/* 受信した接続のプレイヤーID */
	char szAcc[MAX_ACCNAME + 1];
	int i, nCount;

	pPara->szToken[MAX_WEBTOKEN] = 0;

	switch (pPara->byCmdSub) {
	case WEBACC_TOKENLOGIN:		/* 端末トークンでログイン */
		{
			CMDPARA_LOGINREQ LoginReq;

			ZeroMemory (szAcc, sizeof (szAcc));
			i = -1;
			if (WebAccTokenToAccount (pPara->szToken, szAcc, sizeof (szAcc))) {
				i = FindWebAccountChar (szAcc);
			}
			if (i < 0) {
				/* 端末トークンが無効 */
				SendCmdLOGINRES (pID, 0, 7);
				MyOutputDebugString ("端末トークンが無効\n");
				return;
			}
			/* ID とパスワードでのログインと同じ処理に任せる */
			ZeroMemory (&LoginReq, sizeof (LoginReq));
			LoginReq.byCmd = CMD_LOGINREQ;
			LoginReq.pID = pID;
			strncpy (LoginReq.szAcc, g_aCharInfo[i]->szAcc, MAX_ACCNAME - 1);
			strncpy (LoginReq.szPas, g_aCharInfo[i]->szPas, MAX_ACCNAME - 1);
			CopyMemory (LoginReq.szMacAdr, pPara->szMacAdr, 6);
			RecvDataProcLOGINREQ ((PBYTE)&LoginReq, pID);
		}
		break;

	case WEBACC_TOKENREQ:		/* 端末トークン発行 */
		{
			CMDPARA_WEBACCOUNT SendData;

			nCount = g_aCharInfo.GetSize ();
			for (i = 0; i < nCount; i ++) {
				if ((g_aCharInfo[i]->pID == pID) && g_aCharInfo[i]->szAcc[0]) {
					break;
				}
			}
			/* ログインしていない接続には出さない */
			if (i >= nCount) {
				return;
			}
			ZeroMemory (&SendData, sizeof (SendData));
			SendData.byCmd = CMD_WEBACCOUNT;
			SendData.byCmdSub = WEBACC_TOKEN;
			if (!WebAccIssueToken (g_aCharInfo[i]->szAcc, SendData.szToken, sizeof (SendData.szToken))) {
				return;
			}
			svrSock.SendTo (pID, (LPBYTE)&SendData, sizeof (SendData), SENDTIMEOUT);
		}
		break;

	case WEBACC_OPTBGM:			/* 音楽ON/OFFのアカウント設定を覚える */
		/* 古いクライアントの短いパケットや、ログイン前の接続は相手にしない */
		if ((s_dwRecvSize < sizeof (CMDPARA_WEBACCOUNT)) || (s_nCharNoRecv < 0)) {
			return;
		}
		if ((pPara->byData != OPTBGM_ON) && (pPara->byData != OPTBGM_OFF)) {
			return;
		}
		/* 保存は定期保存・終了時の保存に任せる */
		g_aCharInfo[s_nCharNoRecv]->byOptBgm = pPara->byData;
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendWebOptBgm													 */
/* 内容：音楽ON/OFFのアカウント設定をログインしたクライアントへ送る			 */
/*       まだ決めていないアカウントには送らない（端末の設定のまま）			 */
/* ========================================================================= */
static void SendWebOptBgm(PCHARINFO pChar)
{
	CMDPARA_WEBACCOUNT SendData;

	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byOptBgm == OPTBGM_NONE)) {
		return;
	}
	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd = CMD_WEBACCOUNT;
	SendData.byCmdSub = WEBACC_OPTBGM;
	SendData.byData = pChar->byOptBgm;
	svrSock.SendTo (pChar->pID, (LPBYTE)&SendData, sizeof (SendData), SENDTIMEOUT);
}


/* ========================================================================= */
/* 関数名：RecvDataProcADMINUNDO											 */
/* 内容：受信データ処理(管理者画面の「元に戻す」)							 */
/* ========================================================================= */
void RecvDataProcADMINUNDO(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINUNDO pPara = (PCMDPARA_ADMINUNDO)pData;
	DWORD pID = s_pIDRecv;
	DWORD dwStepID;
	WORD wCount = 0;
	BOOL bRet;
	std::string strMsg;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINUNDO) - sizeof (pPara->szMsg)) {
		return;
	}
	if (pPara->byScreen >= ADMINSCR_MAX) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINUNDO_SCREEN:		/* いま開いている画面 */
		AdminUndoSetScreen (pID, pPara->byScreen);
		break;

	case ADMINUNDO_INFOREQ:		/* 戻せる変更の問い合わせ */
		AdminUndoGetInfo (pPara->byScreen, dwStepID, wCount, strMsg);
		SendCmdADMINUNDO (pID, ADMINUNDO_INFO, pPara->byScreen, 0, wCount, dwStepID, strMsg.c_str ());
		break;

	case ADMINUNDO_EXEC:		/* 元に戻す */
		bRet = AdminUndoExec (pPara->byScreen, pPara->dwStepID, pID, strMsg, wCount);
		SendCmdADMINUNDO (pID, ADMINUNDO_RESULT, pPara->byScreen, bRet ? 1 : 0, wCount, 0, strMsg.c_str ());
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcNPCROUTE												 */
/* 内容：受信データ処理(NPCの巡回ルート)									 */
/*       REQ  : そのマップの巡回できるNPC全員のルートを INFO で送り、INFOEND	 */
/*       SET  : 検査して反映し、RESULT とそのNPCの INFO を返す				 */
/* ========================================================================= */
void RecvDataProcNPCROUTE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_NPCROUTE pPara = (PCMDPARA_NPCROUTE)pData;
	DWORD pID = s_pIDRecv;
	int i, anX[MAX_NPCROUTE], anY[MAX_NPCROUTE];
	LPCSTR pszError;
	PCHARINFO pChar;

	if (s_dwRecvSize < sizeof (CMDPARA_NPCROUTE)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case NPCROUTE_REQ:			/* マップの巡回できるNPCの一覧を要求 */
		if (pID == 0) {
			break;
		}
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			pChar = g_aCharInfo[i];
			if ((pChar->dwMapID != pPara->dwMapID) || !NpcRouteIsTarget (pChar)) {
				continue;
			}
			SendCmdNPCROUTE (pID, NPCROUTE_INFO, 0, pPara->dwMapID, (DWORD)i, NULL);
		}
		SendCmdNPCROUTE (pID, NPCROUTE_INFOEND, 0, pPara->dwMapID, 0, NULL);
		break;

	case NPCROUTE_SET:			/* ルートを保存 */
		if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
			SendCmdNPCROUTE (pID, NPCROUTE_RESULT, 0, pPara->dwMapID, pPara->dwTblID, "キャラの番号が範囲外です");
			break;
		}
		pChar = g_aCharInfo[pPara->dwTblID];
		if (pPara->byRouteCount > MAX_NPCROUTE) {
			SendCmdNPCROUTE (pID, NPCROUTE_RESULT, 0, pChar->dwMapID, pPara->dwTblID, "道しるべは16個までです");
			break;
		}
		for (i = 0; i < MAX_NPCROUTE; i ++) {
			anX[i] = pPara->abyRouteX[i];
			anY[i] = pPara->abyRouteY[i];
		}
		pszError = NpcRouteCheck (pChar, pPara->byRouteMode, pPara->byRouteWait, pPara->byRouteCount, anX, anY);
		if (pszError) {
			SendCmdNPCROUTE (pID, NPCROUTE_RESULT, 0, pChar->dwMapID, pPara->dwTblID, pszError);
			break;
		}
		NpcRouteApply (pChar, pPara->byRouteMode, pPara->byRouteWait, pPara->byRouteCount, pPara->abyRouteX, pPara->abyRouteY);
		SendCmdNPCROUTE (pID, NPCROUTE_RESULT, 1, pChar->dwMapID, pPara->dwTblID, NULL);
		SendCmdNPCROUTE (pID, NPCROUTE_INFO, 0, pChar->dwMapID, pPara->dwTblID, NULL);
		break;
	}
}

/* ========================================================================= */
/* 関数名：RecvCmdReplayAdmin												 */
/* 内容：記録しておいたパケットを、管理者の操作として処理し直す			 */
/*       （「元に戻す」用。pID は戻す処理を頼んだ接続。API からなら 0）		 */
/* ========================================================================= */
void RecvCmdReplayAdmin(const std::vector<BYTE> &aPacket, DWORD pID, DWORD dwMapNo)
{
	DWORD dwSize = (DWORD)aPacket.size ();

	if ((dwSize == 0) || (aPacket[0] >= CMD_MAX) || (RecvDataProcTbl[aPacket[0]] == NULL)) {
		return;
	}

	/* 処理中の受信の情報を退避（「元に戻す」の受信処理の中から呼ばれるので） */
	DWORD		pIDBack		= s_pIDRecv;
	int			nCharNoBack	= s_nCharNoRecv;
	BOOL		bAdminBack	= s_bAdminRecv;
	DWORD		dwSizeBack	= s_dwRecvSize;
	const BYTE	*pDataBack	= s_pRecvData;
	DWORD		dwMapNoBack	= s_dwRecvMapNo;

	/* 受信データと同じく、後ろに 0 埋めの余白を付ける */
	PBYTE pData = new BYTE[dwSize + 8192];
	ZeroMemory (pData, dwSize + 8192);
	CopyMemory (pData, &aPacket[0], dwSize);

	s_pIDRecv		= pID;
	s_nCharNoRecv	= pID ? GetCharNo (pID) : -1;
	s_bAdminRecv	= TRUE;
	s_dwRecvSize	= dwSize;
	s_pRecvData		= pData;
	s_dwRecvMapNo	= dwMapNo;

	RecvDataProcTbl[pData[0]](pData, pID);

	s_pIDRecv		= pIDBack;
	s_nCharNoRecv	= nCharNoBack;
	s_bAdminRecv	= bAdminBack;
	s_dwRecvSize	= dwSizeBack;
	s_pRecvData		= pDataBack;
	s_dwRecvMapNo	= dwMapNoBack;

	SAFE_DELETE_ARRAY (pData);
}

/* ========================================================================= */
/* 関数名：RecvCmdApplyAdminPacket											 */
/* 内容：管理者用コマンドを API から処理し、「元に戻す」に記録する			 */
/*       pData はクライアントが送るのと同じ形のパケット。					 */
/*       byScreen は記録する画面（ADMINSCR_NONE ならコマンドから選ぶ）		 */
/*       メッセージ・店は dwTblID のキャラのいるマップが対象になる			 */
/* ========================================================================= */
BOOL RecvCmdApplyAdminPacket(const BYTE *pData, DWORD dwSize, LPCSTR pszSource, BYTE byScreen)
{
	BYTE byCmd;
	BOOL bUndo;

	if ((pData == NULL) || (dwSize == 0)) {
		return FALSE;
	}
	byCmd = pData[0];
	/* 管理者用のコマンドだけ（ログインや「元に戻す」自体は通さない） */
	if ((byCmd >= CMD_MAX) || (RecvDataProcTbl[byCmd] == NULL) ||
			((byCmd < CMD_MAPPARTSADD) && (byCmd != CMD_CHARITEM)) ||
			(byCmd == CMD_WEBACCOUNT) || (byCmd == CMD_ADMINUNDO) || (byCmd == CMD_REQ_GUARD) || (byCmd == CMD_RECIPEMAKE) || (byCmd == CMD_BATTLEMODE)) {
		return FALSE;
	}

	std::vector<BYTE> aPacket (pData, pData + dwSize);
	bUndo = AdminUndoBefore (byCmd, &aPacket[0], dwSize, 0,
			(pszSource && pszSource[0]) ? pszSource : "API", byScreen);
	RecvCmdReplayAdmin (aPacket, 0, (DWORD)-1);
	if (bUndo) {
		AdminUndoAfter ();
	}
	return TRUE;
}

/* ========================================================================= */
/* レシピ（作り手に頼んでアイテムを作ってもらう）							 */
/* ========================================================================= */

#define RECIPE_TALKRANGE	(3)		/* 作り手のNPCがこのマス数以内にいれば頼める（カウンター越しでもよいよう余裕を持たせる） */

/* ========================================================================= */
/* 関数名：RecipeCheckMakerNear												 */
/* 内容：近くに、台詞が "@cNN"（NN=byMaker）の作り手NPCがいるか				 */
/* ========================================================================= */
static BOOL RecipeCheckMakerNear(PCHARINFO pChar, BYTE byMaker)
{
	char szCode[8];
	int i, nCount = g_aCharInfo.GetSize ();

	snprintf (szCode, sizeof (szCode), "@c%02d", (int)byMaker);
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pNpc = g_aCharInfo[i];

		if ((pNpc == NULL) || (pNpc == pChar) || (pNpc->byNpcType == NPCTYPE_INIT)) {
			continue;
		}
		if ((pNpc->dwMapID != pChar->dwMapID) || (pNpc->pszNpcSpeech == NULL)) {
			continue;
		}
		if (strncmp (pNpc->pszNpcSpeech, szCode, 4) != 0) {
			continue;
		}
		if ((abs ((int)pNpc->dwX - (int)pChar->dwX) <= RECIPE_TALKRANGE) &&
			(abs ((int)pNpc->dwY - (int)pChar->dwY) <= RECIPE_TALKRANGE)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：RecipeIsUsableItem												 */
/* 内容：持ち物の nNo 番目を材料に使えるか（装備中・捨てられない物は使えない）	 */
/* ========================================================================= */
static BOOL RecipeIsUsableItem(PCHARINFO pChar, int nNo)
{
	int i;
	WORD wItem = pChar->waItem[nNo];

	if ((wItem == 0) || (wItem >= (WORD)g_aItemInfo.GetSize ())) {
		return FALSE;
	}
	for (i = 0; i < 10; i ++) {
		if (pChar->waEqu[i] == nNo) {
			return FALSE;
		}
	}
	if (g_aItemInfo[wItem]->byFlg & ITEMFLG_DROP) {
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：RecipeMake														 */
/* 内容：材料・手数料・置き場所を確かめ、消費して、できた物を渡す				 */
/*       材料は持ち物を先に、足りない分を預かり所から使う。					 */
/*       できた物は持ち物へ、いっぱいなら預かり所へ。預かり所も満杯なら		 */
/*       何も変えずに RECIPERESULT_NOROOM を返す。							 */
/*       成功したら *pnToInv・*pnToLeave に持ち物・預かり所へ渡した個数を入れる */
/* ========================================================================= */
static BYTE RecipeMake(PCHARINFO pChar, PRECIPEINFO pRecipe, int *pnToInv, int *pnToLeave)
{
	PLEAVEINFO pLeave = NULL;
	BOOL abyInvUsed[MAX_MYITEM];
	int anLeaveTake[MAX_LEAVEITEM];
	WORD awItemBak[MAX_MYITEM], awEquBak[10];
	DWORD dwLeaveIDBak;
	LEAVEINFO LeaveBak;
	int i, k, n, nFree, nNeed, nToInv = 0, nToLeave = 0;

	*pnToInv = 0;
	*pnToLeave = 0;

	if (pChar->dwGold < pRecipe->dwGold) {
		return RECIPERESULT_LACKGOLD;
	}
	if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
		pLeave = g_aLeaveInfo[pChar->dwLeaveID];
		if (pLeave->byCount > MAX_LEAVEITEM) {
			pLeave = NULL;
		}
	}

	/* 材料が揃っているか（持ち物を先に使い、足りない分を預かり所から使う計画を立てる） */
	ZeroMemory (abyInvUsed, sizeof (abyInvUsed));
	ZeroMemory (anLeaveTake, sizeof (anLeaveTake));
	for (k = 0; k < MAX_RECIPEMAT; k ++) {
		WORD wMat = pRecipe->awMatItem[k];

		nNeed = pRecipe->abyMatCount[k];
		if ((wMat == 0) || (nNeed == 0)) {
			continue;
		}
		for (i = 0; (i < MAX_MYITEM) && (nNeed > 0); i ++) {
			if (!abyInvUsed[i] && (pChar->waItem[i] == wMat) && RecipeIsUsableItem (pChar, i)) {
				abyInvUsed[i] = TRUE;
				nNeed --;
			}
		}
		for (i = 0; pLeave && (i < pLeave->byCount) && (nNeed > 0); i ++) {
			if (pLeave->waItem[i] == wMat) {
				int nAvail = (int)pLeave->byItemCount[i] - anLeaveTake[i];
				int nTake = min (nAvail, nNeed);
				if (nTake > 0) {
					anLeaveTake[i] += nTake;
					nNeed -= nTake;
				}
			}
		}
		if (nNeed > 0) {
			return RECIPERESULT_LACKMAT;
		}
	}

	/* ここから先は途中で失敗したら元に戻せるよう、今の状態を控えておく */
	memcpy (awItemBak, pChar->waItem, sizeof (awItemBak));
	memcpy (awEquBak, pChar->waEqu, sizeof (awEquBak));
	dwLeaveIDBak = pChar->dwLeaveID;
	ZeroMemory (&LeaveBak, sizeof (LeaveBak));
	if (pLeave) {
		LeaveBak = *pLeave;
	}

	/* 材料を使う：持ち物（後ろから消して詰める） */
	for (i = MAX_MYITEM - 1; i >= 0; i --) {
		if (abyInvUsed[i]) {
			DelCharItem (pChar->dwTblID, (BYTE)i);
		}
	}
	/* 材料を使う：預かり所（0 個になった枠は詰める） */
	if (pLeave) {
		n = 0;
		for (i = 0; i < pLeave->byCount; i ++) {
			pLeave->byItemCount[i] = (BYTE)(pLeave->byItemCount[i] - anLeaveTake[i]);
			if (pLeave->byItemCount[i] == 0) {
				continue;
			}
			pLeave->waItem[n] = pLeave->waItem[i];
			pLeave->byItemCount[n] = pLeave->byItemCount[i];
			n ++;
		}
		for (i = n; i < pLeave->byCount; i ++) {
			pLeave->waItem[i] = 0;
			pLeave->byItemCount[i] = 0;
		}
		pLeave->byCount = (BYTE)n;
	}

	/* できた物を渡す：持ち物へ、いっぱいなら預かり所へ */
	nFree = MAX_MYITEM - GetItemCount (pChar->dwTblID);
	for (i = 0; i < pRecipe->byCount; i ++) {
		if (nFree > 0) {
			AddCharItem (pChar->dwTblID, pRecipe->wItem);
			nFree --;
			nToInv ++;
			continue;
		}
		if (!AddLeaveItem (pChar, pRecipe->wItem)) {
			/* 預かり所もいっぱい：何も変えなかったことにする */
			memcpy (pChar->waItem, awItemBak, sizeof (awItemBak));
			memcpy (pChar->waEqu, awEquBak, sizeof (awEquBak));
			if ((dwLeaveIDBak == 0) && (pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
				ZeroMemory (g_aLeaveInfo[pChar->dwLeaveID], sizeof (LEAVEINFO));
				pChar->dwLeaveID = 0;
			} else if (pLeave) {
				*pLeave = LeaveBak;
			}
			return RECIPERESULT_NOROOM;
		}
		nToLeave ++;
	}

	pChar->dwGold -= pRecipe->dwGold;
	*pnToInv = nToInv;
	*pnToLeave = nToLeave;
	return RECIPERESULT_OK;
}

/* ========================================================================= */
/* 関数名：RecvDataProcRECIPEMAKE											 */
/* 内容：受信データ処理(レシピで作ってもらう)								 */
/*       存在しないレシピ・停止中のレシピ・作り手の番号が違う・近くに作り手が	 */
/*       いない要求は RECIPERESULT_INVALID で断る。							 */
/* ========================================================================= */
void RecvDataProcRECIPEMAKE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_RECIPEMAKE pPara = (PCMDPARA_RECIPEMAKE)pData;
	PCHARINFO pChar;
	PRECIPEINFO pRecipe;
	BYTE byResult = RECIPERESULT_INVALID;
	int nToInv = 0, nToLeave = 0;

	if (s_dwRecvSize < sizeof (CMDPARA_RECIPEMAKE)) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}

	/* 要求が正しいか */
	if ((pPara->wRecipeID >= (WORD)g_aRecipeInfo.GetSize ()) || (pPara->byMaker >= (BYTE)g_aMakerInfo.GetSize ())) {
		goto Exit;
	}
	pRecipe = g_aRecipeInfo[pPara->wRecipeID];
	if ((pRecipe->byState != RECIPESTATE_ON) || (g_aMakerInfo[pPara->byMaker]->byState != RECIPESTATE_ON) ||
			(pRecipe->byMaker != pPara->byMaker)) {
		goto Exit;
	}
	if ((pRecipe->wItem == 0) || (pRecipe->wItem >= (WORD)g_aItemInfo.GetSize ()) || (pRecipe->byCount == 0)) {
		goto Exit;
	}
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		goto Exit;
	}
	if (!RecipeCheckMakerNear (pChar, pPara->byMaker)) {
		goto Exit;
	}

	byResult = RecipeMake (pChar, pRecipe, &nToInv, &nToLeave);
	if (byResult == RECIPERESULT_OK) {
		/* 持ち物・お金・預かり所の変化を本人へ送る */
		SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
		SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
		if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
			SendCmdLEAVEITEMGET (pChar->dwTblID);
		}
		SendCmdPLAYSOUND (pChar->pID, SOUND_PI52);
		SendCmdSYSMSG2 (pChar->pID, SYSMSG_GETITEM3, pChar->pID, pRecipe->wItem);
		if (nToLeave > 0) {
			/* 預かり所へ送った（敵のドロップと同じ設定：預かり所の使い魔が運んでくれる） */
			char szTmp[256];
			snprintf (szTmp, sizeof (szTmp), "持ちきれないので、預かり所の使い魔が%sの預かり所へ運んでいきました", pChar->szName);
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		}
	}

Exit:
	SendCmdRECIPEMAKE (pChar->pID, byResult, pPara->byMaker, pPara->wRecipeID);
}
