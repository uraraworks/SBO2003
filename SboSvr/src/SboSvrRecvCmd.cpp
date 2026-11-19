/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			コマンド受信処理										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "common/BowFlight.h"
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
#include "SboSvrMarket.h"
#include "SboSvrRecycle.h"
#include "SboSvrAffix.h"
#include "SboSvrQuest.h"
#include "SboSvrMapSwapEdit.h"
#include "CharProc/CharProcMERCENARY.h"
#include "SboSvrMagicProc.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>

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
extern void SboDataQuestApplyInfo	(int, const QUESTINFO *, int &, std::string &);	/* 管理者ウィンドウから依頼を書き込む(server/web_data_svr.cpp) */
extern void SboWebSendLoginNotice	(DWORD);	/* ログインした人に、覚えている再起動の予告を見せる(server/web_account_svr.cpp) */
extern void SboDataFlagNameApplyInfo	(int, const FLAGNAMEINFO *, int &, std::string &);	/* 管理者ウィンドウからフラグの名前表を書き込む(server/web_data_svr.cpp) */
extern void SboDataTalkApplyInfo		(int, const TALKINFO *, int &, std::string &);		/* 管理者ウィンドウから会話データを書き込む(server/web_data_svr.cpp) */
extern void SboDataStoryApplyInfo		(int, const STORYINFO *, int &, std::string &);		/* 管理者ウィンドウから思い出の本のシナリオを書き込む(server/web_data_svr.cpp) */
extern void SboDataFlagGroupApplyInfo	(int, const FLAGGROUPINFO *, int &, std::string &);	/* 管理者ウィンドウからフラグのグループを書き込む(server/web_data_svr.cpp) */
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
void	AddCharItemBag				(DWORD, DWORD);									/* 手持ちの空きにアイテム追加（大事な物の振り分けなし） */
BOOL	IsImportantItem				(WORD);											/* 大事な物の欄に入る品か */
int		GetImportantCount			(DWORD);										/* 大事な物の数を取得 */
BOOL	CanAddCharItem				(DWORD, DWORD);									/* その品を今受け取れるか */
int		CountImportantItem			(PCHARINFO, WORD);								/* 大事な物の欄にある、その品の数 */
BOOL	DelImportantItem			(PCHARINFO, WORD);								/* 大事な物の欄から、その品を1個取り除く */
int		MoveImportantItems			(PCHARINFO);									/* 手持ちの大事な物を大事な物の欄へ移す */
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
void	CharTargetCancelPCKeepBack	(PCHARINFO);									/* ターゲットと前の人へのついていくだけ解除 */
void	TailBattleMode				(PCHARINFO, BOOL);								/* 戦闘モードに合わせて列を離れる・戻る */
void	TailReturnForget			(DWORD);										/* 戦闘で列を離れた時に覚えた先頭を消す */
void	TailChaseProc				(PCHARINFO);									/* 列に戻る途中のプレイヤーを歩かせる */
BOOL	TailChaseIsActive			(PCHARINFO);									/* 列に戻る途中(追いかけ中)か */
void	TailMoveChain				(PCHARINFO);									/* ついてきているキャラを順に動かす */
void	FloorDamageOnStep			(PCHARINFO);									/* ダメージ床に1歩入った時の処理 */
BOOL	IsDamageFloor				(PMAPINFO, int, int);							/* ダメージ床のマスか */
BOOL	IsAvoidDamageFloor			(PCHARINFO, PMAPINFO, int, int);				/* 敵・傭兵が自分からは入らないダメージ床か */
BOOL	IsNoFloorDamageChar			(PCHARINFO);									/* ダメージ床が効かない敵か */
BOOL	CheckCharX					(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget);	/* キャラサイズに合わせた当たり判定(横) */
int		GetCharX					(PCHARINFO, DWORD, DWORD, BOOL);				/* キャラサイズに合わせた距離を求める(横) */
int		GetCharDistanceX			(PCHARINFO, PCHARINFO);							/* キャラサイズに合わせた距離を求める(横) */
DWORD	GetCharXPos					(PCHARINFO, DWORD, BYTE);						/* キャラサイズに合わせた座標を補正(横) */
BOOL	CheckCharBlock				(DWORD, DWORD, DWORD);							/* キャラのぶつかり判定 */
BOOL	CheckCharBlock				(PCHARINFO pChar, DWORD, DWORD, DWORD);			/* キャラのぶつかり判定 */
BOOL	CheckCharBlockFor			(PCHARINFO pViewer, DWORD, DWORD, DWORD);		/* 同上（pViewer の印で差し替えたパーツで判定） */
DWORD	CheckCharBlockPC			(DWORD, DWORD, DWORD, DWORD);					/* キャラのキャラとのぶつかり判定 */
DWORD	CheckCharBlockNPC			(DWORD, DWORD, DWORD, DWORD, PCHARINFO);		/* キャラのキャラとのぶつかり判定(NPC) */
DWORD	CheckCharBlockNPC2			(DWORD, DWORD, DWORD, DWORD, DWORD, PCHARINFO);	/* キャラのキャラとのぶつかり判定2(NPC) */
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
static PCHARINFO FindFrontMimic					(PCHARINFO);				/* 目の前の宝箱の敵(正体前)を探す */
static BOOL IsEventExamine						(PEVENTINFO, BYTE);			/* 調べた時に何か起きるイベントか */
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
void	RecvDataProcADMINSPACE					(PBYTE, DWORD);				/* 管理者の空間の切り替え */
void	RecvDataProcNPCROUTE					(PBYTE, DWORD);				/* NPCの巡回ルート */
void	RecvDataProcRECIPEMAKE					(PBYTE, DWORD);				/* レシピで作ってもらう */
void	RecvDataProcAPPRAISE					(PBYTE, DWORD);				/* 鑑定屋で鑑定してもらう */
void	RecvDataProcQUEST						(PBYTE, DWORD);				/* ギルドの仕事依頼（一覧・受ける・取り消す・渡す） */
void	RecvDataProcADMINQUEST					(PBYTE, DWORD);				/* 管理者ウィンドウのギルドの依頼の編集 */
void	RecvDataProcADMINRESURRECT				(PBYTE, DWORD);				/* 管理者ウィンドウからキャラを蘇生 */
void	RecvDataProcADMINVIEWALL				(PBYTE, DWORD);				/* 管理者ウィンドウの「印で隠れたNPCも表示」の切り替え */
void	RecvDataProcMAGICKEY					(PBYTE, DWORD);				/* 特技ショートカットの登録 */
void	RecvDataProcADMINFLAGNAME				(PBYTE, DWORD);				/* 管理者ウィンドウのフラグの名前表の編集 */
void	RecvDataProcADMINTALK					(PBYTE, DWORD);				/* 管理者ウィンドウの会話データの編集 */
void	RecvDataProcADMINCHARFLAG				(PBYTE, DWORD);				/* 管理者ウィンドウのキャラのフラグの取得・変更 */
void	RecvDataProcADMINSTORY					(PBYTE, DWORD);				/* 管理者ウィンドウの思い出の本の編集 */
void	RecvDataProcADMINFLAGGROUP				(PBYTE, DWORD);				/* 管理者ウィンドウのフラグのグループの編集 */
void	RecvDataProcSORTITEM					(PBYTE, DWORD);				/* 道具欄・預かり所の並べ替え */
void	RecvDataProcLEAVEITEMDISPOSE			(PBYTE, DWORD);				/* 預かり所の品を売る・捨てる */
void	RecvDataProcBATTLEMODE					(PBYTE, DWORD);				/* 戦闘モードの切り替え通知 */
void	RecvDataProcMARKETLIST					(PBYTE, DWORD);				/* 取引所の一覧 */
void	RecvDataProcMARKETPUT					(PBYTE, DWORD);				/* 取引所に出品する */
void	RecvDataProcMARKETBUY					(PBYTE, DWORD);				/* 取引所で買う */
void	RecvDataProcMARKETTAKEBACK				(PBYTE, DWORD);				/* 取引所の出品を引き取る */
void	RecvDataProcMARKETGOLDGET				(PBYTE, DWORD);				/* 取引所の売上を受け取る */
void	RecvDataProcRECYCLELIST					(PBYTE, DWORD);				/* リサイクルボックスの中身 */
void	RecvDataProcRECYCLEPUT					(PBYTE, DWORD);				/* リサイクルボックスに入れる */
void	RecvDataProcRECYCLETAKE					(PBYTE, DWORD);				/* リサイクルボックスからもらう */
void	RecvDataProcRECYCLECLOSE				(PBYTE, DWORD);				/* リサイクルボックスの窓を閉じた */
void	RecvDataProcADMINCHEST					(PBYTE, DWORD);				/* 管理者ウィンドウのキャラの開けた宝箱の記録 */
void	RecvDataProcADMINSWAP					(PBYTE, DWORD);				/* 管理者ウィンドウの「印で変わるマス・宝箱」 */
void	RecvDataProcTALKANSWER					(PBYTE, DWORD);				/* 会話データの「はい・いいえ」の答え */
/* 端末トークン（server/web_account_svr.cpp） */
BOOL	WebAccTokenToAccount					(LPCSTR, LPSTR, int);		/* 端末トークンからアカウント名を引く */
static void	SendWebOptBgm					(PCHARINFO);				/* 音楽ON/OFFのアカウント設定を送る */
static void	SendMagicKeySetting				(PCHARINFO);				/* 特技ショートカットと帯の表示設定を送る */
void	WebStatNoteLogin						(PCHARINFO);				/* ログインできた時に利用環境の統計へ記録する */
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
extern void ItemProcImportant			(DWORD, BYTE, DWORD);				/* 大事な物の欄の品を使う */
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
extern BOOL SaveMarket					(void);								/* 取引所の出品・売上保存 */
extern BOOL SaveRecycle					(void);								/* リサイクルボックスの中身・記録保存 */
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
	RecvDataProcQUEST,					/* CMD_QUEST */
	NULL,								/* CMD_QUESTSTATE(サーバー→クライアントのみ) */
	RecvDataProcSORTITEM,				/* CMD_SORTITEM */
	RecvDataProcLEAVEITEMDISPOSE,		/* CMD_LEAVEITEMDISPOSE */
	NULL,								/* CMD_CHARBUFF(サーバー→クライアントのみ) */
	RecvDataProcADMINSPACE,				/* CMD_ADMINSPACE */
	RecvDataProcADMINQUEST,				/* CMD_ADMINQUEST */
	RecvDataProcADMINRESURRECT,			/* CMD_ADMINRESURRECT */
	RecvDataProcMAGICKEY,				/* CMD_MAGICKEY */
	NULL,								/* CMD_TALKMSG(サーバー→クライアントのみ) */
	RecvDataProcADMINFLAGNAME,			/* CMD_ADMINFLAGNAME */
	RecvDataProcADMINTALK,				/* CMD_ADMINTALK */
	RecvDataProcADMINCHARFLAG,			/* CMD_ADMINCHARFLAG */
	RecvDataProcMARKETLIST,				/* CMD_MARKETLIST */
	RecvDataProcMARKETPUT,				/* CMD_MARKETPUT */
	RecvDataProcMARKETBUY,				/* CMD_MARKETBUY */
	RecvDataProcMARKETTAKEBACK,			/* CMD_MARKETTAKEBACK */
	RecvDataProcMARKETGOLDGET,			/* CMD_MARKETGOLDGET */
	RecvDataProcADMINSTORY,				/* CMD_ADMINSTORY */
	RecvDataProcTALKANSWER,				/* CMD_TALKANSWER */
	RecvDataProcADMINVIEWALL,			/* CMD_ADMINVIEWALL */
	RecvDataProcADMINFLAGGROUP,			/* CMD_ADMINFLAGGROUP */
	NULL,								/* CMD_MAPSWAP(サーバー→クライアントのみ) */
	RecvDataProcRECYCLELIST,			/* CMD_RECYCLELIST */
	RecvDataProcRECYCLEPUT,				/* CMD_RECYCLEPUT */
	RecvDataProcRECYCLETAKE,			/* CMD_RECYCLETAKE */
	RecvDataProcRECYCLECLOSE,			/* CMD_RECYCLECLOSE */
	RecvDataProcADMINCHEST,				/* CMD_ADMINCHEST */
	RecvDataProcADMINSWAP,				/* CMD_ADMINSWAP */
	NULL,								/* CMD_AFFIXINFO(サーバー→クライアントのみ) */
	NULL,								/* CMD_ITEMDERIVE(サーバー→クライアントのみ) */
	RecvDataProcAPPRAISE,				/* CMD_APPRAISE */

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

	/* 別の空間にいるキャラには届かない（管理者も、いま居る空間のキャラだけ） */
	if ((s_nCharNoRecv >= 0) && !IsSameSpace (g_aCharInfo[s_nCharNoRecv], pCharTarget)) {
		return FALSE;
	}
	/* 印で見えないNPCは、居ないのと同じ（話せない・押せない・調べられない。管理者には見える） */
	if ((s_nCharNoRecv >= 0) && !IsCharSeenBy (pCharTarget, g_aCharInfo[s_nCharNoRecv])) {
		return FALSE;
	}
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
				!ItemIDValid (pChar->waItem[pwaItem[i]])) {
			return FALSE;
		}
		/* 大事な物・依頼の品は取り引きに出せない（古いキャラで手持ちに残っていた場合の抜け道ふさぎ） */
		if (IsImportantItem (pChar->waItem[pwaItem[i]])) {
			return FALSE;
		}
		abUse[pwaItem[i]] = TRUE;
	}
	return TRUE;
}

/* 傭兵との取り引き：傭兵の側の返事を、雇い主の画面へ送る。
   傭兵は持っている道具を全部返そうとし、雇い主が「決定」して渡すものが受け取れる内容なら
   傭兵も「決定」する（雇い主の画面は両方決定で取り引きを成立させに来る） */
static void TradeMercenaryReply(DWORD dwOwner, DWORD dwMerc)
{
	int i, n;
	char szMsg[256];
	BYTE bySum;
	TRADESTATE *pStateOwner, *pStateMerc;
	PCHARINFO pOwner, pMerc;
	CMDPARA_TRADEINFO SendData;

	if (!TradeGetPair (dwOwner, dwMerc, &pStateOwner, &pStateMerc)) {
		return;
	}
	pOwner	= g_aCharInfo[dwOwner];
	pMerc	= g_aCharInfo[dwMerc];

	FillMemory (pStateMerc->waItem, sizeof (pStateMerc->waItem), 0xFF);
	for (i = 0, n = 0; i < MAX_MYITEM; i ++) {
		if (pMerc->waItem[i] && ItemIDValid (pMerc->waItem[i])) {
			pStateMerc->waItem[n ++] = (WORD)i;
		}
	}
	pStateMerc->dwGold	= 0;
	pStateMerc->byMode	= 0;
	if (pStateOwner->byMode == 1) {
		if (MercenaryTradeCheck (pOwner, pMerc, pStateOwner->waItem, pStateOwner->dwGold, szMsg, sizeof (szMsg))) {
			pStateMerc->byMode = 1;
		} else if (szMsg[0]) {
			SendCmdSYSMSG (pOwner->pID, szMsg, SPEECHCMD_BATTLE1);
		}
	}

	/* 取り引きを成立させる時の確かめ用(クライアントと同じ計算) */
	bySum = (BYTE)pMerc->dwGold;
	for (i = 0; i < MAX_MYITEM; i ++) {
		bySum += (BYTE)pMerc->waItem[i];
	}

	ZeroMemory (&SendData, sizeof (SendData));
	SendData.byCmd		= CMD_TRADEINFO;
	SendData.byMode		= pStateMerc->byMode;
	SendData.dwTblID	= dwOwner;
	SendData.bySum		= bySum;
	SendData.dwGold		= 0;
	CopyMemory (SendData.waItem, pStateMerc->waItem, sizeof (SendData.waItem));
	svrSock.SendTo (pOwner->pID, (PBYTE)&SendData, sizeof (SendData));
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
/* 関数名：	RecvCmdIsTrading												 */
/* 内容：	そのキャラが取り引き中か（傭兵は取り引き中その場で待つ）		 */
/* ========================================================================= */

BOOL RecvCmdIsTrading(DWORD dwTblID)
{
	return (s_mapTrade.find (dwTblID) != s_mapTrade.end ());
}


/* ========================================================================= */
/* 関数名：	RecvCmdTradeClear												 */
/* 内容：	そのキャラの取り引きの状態を消す（傭兵が帰る時など。番号が別の	 */
/*			キャラに使い回されても、古い取り引きが成立しないように）		 */
/* ========================================================================= */

BOOL RecvCmdTradeClear(DWORD dwTblID)
{
	BOOL bRet = (s_mapTrade.find (dwTblID) != s_mapTrade.end ());
	TradeClear (dwTblID);
	return bRet;
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
	TalkPendClear ((DWORD)nNo);		/* 「はい・いいえ」の保留中の問い */
	/* 戦闘で列を離れた時の戻り先と、戦闘モードの覚えを消す。
	   プレイヤーのキャラは切断しても消えない(DeleteChar を通らない)うえ、接続番号(pID)は
	   空いた枠から使い回すので、残っていると入り直した時に「戦闘モード中」と見なされ、
	   Z の「ついていく」を黙って無視→クライアントだけ「ついていく」扱いで動けなくなっていた */
	TailReturnForget ((DWORD)nNo);
	MercenaryClearBattleMode ((DWORD)nNo);
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
	/* 敵が消える時は、そのマップの「敵を全部倒す」扉をすぐ見直す */
	BOOL bEnemy = (pChar->byNpcType == NPCTYPE_BATTLE1) || (pChar->byNpcType == NPCTYPE_BATTLE2) || (pChar->byNpcType == NPCTYPE_MIMIC);
	DWORD dwMapID = pChar->dwMapID;

	/* 戦闘で列を離れた時に覚えた先頭と、戦闘モードの覚えを消す */
	TailReturnForget (dwTblID);
	MercenaryClearBattleMode (dwTblID);
	/* 討伐報酬用に覚えていたダメージを消す */
	ClearBattleDamage (dwTblID);
	OnDecPlayer ((int)pChar->dwTblID);
	pProc = (PCCharProcBase)pChar->pProc;
	SAFE_DELETE (pProc);
	ZeroMemory (pChar, sizeof (CHARINFO));

	if (bEnemy) {
		SpaceEnemyDoorCheckMap (dwMapID);
	}
}


/* ========================================================================= */
/* 関数名：	GetCharNo														 */
/* 内容：	キャラ番号取得													 */
/* 日付：	2003/05/02														 */
/* ========================================================================= */

int GetCharNo(DWORD pID)
{
	static std::unordered_map<DWORD, int> aCharNo;
	int i, nCount;

	if (pID == 0) {
		return -1;
	}
	nCount = g_aCharInfo.GetSize ();
	/* NPCのpIDはキャラ番号＋1なので、件数以下ではPCと重複し得る。
	   件数を超えるIDだけ使う。PCはLOGINREQで同じIDが無いことを確認して
	   から割り当て、OnDecPlayerで0に戻すため、この範囲では一意になる。
	   件数が増えてNPCと重なった場合も、必ず従来の先頭からの検索に戻る。 */
	if (pID > (DWORD)nCount) {
		std::unordered_map<DWORD, int>::iterator it = aCharNo.find (pID);
		if (it != aCharNo.end ()) {
			int nNo = it->second;
			/* 配列の削除・入れ替え・再読込や接続IDの変更後も、現在の値で検証 */
			if ((nNo >= 0) && (nNo < nCount) && (g_aCharInfo[nNo]->pID == pID)) {
				return nNo;
			}
			aCharNo.erase (it);
		}
	}
	for (i = 0; i < nCount; i ++) {
		if (pID == g_aCharInfo[i]->pID) {
			if (pID > (DWORD)nCount) {
				aCharNo[pID] = i;
			}
			return i;
		}
	}

	/* 未ログインの接続は後でログインするので、未発見は記録しない */
	return -1;
}


/* ========================================================================= */
/* 関数名：	GetMapNo														 */
/* 内容：	マップ番号を取得												 */
/* 日付：	2003/05/04														 */
/* ========================================================================= */

int GetMapNo(DWORD dwID)
{
	static int nLastMapNo = -1;
	int i, nCount;
	PMAPINFO pMapInfo;

	nCount = g_aMapInfo.GetSize ();
	/* 配列変更後も使えるよう、前回の番号の範囲と現在のIDを毎回確認 */
	if ((nLastMapNo >= 0) && (nLastMapNo < nCount) && (g_aMapInfo[nLastMapNo]->dwID == dwID)) {
		return nLastMapNo;
	}
	for (i = 0; i < nCount; i ++) {
		pMapInfo = g_aMapInfo[i];
		if (pMapInfo->dwID == dwID) {
			nLastMapNo = i;
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
/* 関数名：	IsImportantItem													 */
/* 内容：	大事な物の欄に入る品か（大事な物の印がある品、依頼の品）		 */
/* ========================================================================= */

BOOL IsImportantItem(WORD wItem)
{
	if ((wItem == 0) || !ItemIDValid (wItem)) {
		return FALSE;
	}
	if (g_aItemInfo[wItem]->byFlg & ITEMFLG_IMPORTANT) {
		return TRUE;
	}
	return QuestIsQuestItem (wItem);
}


/* ========================================================================= */
/* 関数名：	GetImportantCount												 */
/* 内容：	大事な物の数を取得												 */
/* ========================================================================= */

int GetImportantCount(DWORD dwID)
{
	int i, nRet;
	PCHARINFO pChar;

	nRet  = 0;
	pChar = g_aCharInfo[dwID];
	for (i = 0; i < MAX_IMPORTANT; i ++) {
		if (pChar->waImportant[i]) {
			nRet ++;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CanAddCharItem													 */
/* 内容：	その品を今受け取れるか（大事な物は大事な物の欄、他は手持ちの空き）*/
/* ========================================================================= */

BOOL CanAddCharItem(DWORD dwCharID, DWORD dwItemID)
{
	if ((dwItemID < 0x10000) && IsImportantItem ((WORD)dwItemID)) {
		return GetImportantCount (dwCharID) < MAX_IMPORTANT;
	}
	return GetItemCount (dwCharID) < MAX_MYITEM;
}


/* ========================================================================= */
/* 関数名：	CountImportantItem												 */
/* 内容：	大事な物の欄にある、その品の数									 */
/* ========================================================================= */

int CountImportantItem(PCHARINFO pChar, WORD wItem)
{
	int i, nRet;

	nRet = 0;
	if ((pChar == NULL) || (wItem == 0)) {
		return 0;
	}
	for (i = 0; i < MAX_IMPORTANT; i ++) {
		if (pChar->waImportant[i] == wItem) {
			nRet ++;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	DelImportantItem												 */
/* 内容：	大事な物の欄から、その品を1個取り除く（後ろを前へつめる）		 */
/* ========================================================================= */

BOOL DelImportantItem(PCHARINFO pChar, WORD wItem)
{
	int i, j;

	if ((pChar == NULL) || (wItem == 0)) {
		return FALSE;
	}
	for (i = 0; i < MAX_IMPORTANT; i ++) {
		if (pChar->waImportant[i] != wItem) {
			continue;
		}
		for (j = i; j < MAX_IMPORTANT - 1; j ++) {
			pChar->waImportant[j] = pChar->waImportant[j + 1];
		}
		pChar->waImportant[MAX_IMPORTANT - 1] = 0;
		return TRUE;
	}

	return FALSE;
}


/* ========================================================================= */
/* 関数名：	MoveImportantItems												 */
/* 内容：	手持ちにある大事な物・依頼の品（装備中以外）を大事な物の欄へ移す	 */
/*			（古いキャラの持ち物の直し。欄がいっぱいなら残りは手持ちのまま）	 */
/* ========================================================================= */

int MoveImportantItems(PCHARINFO pChar)
{
	int i, j, k, nRet;
	BOOL bEqu;

	nRet = 0;
	if (pChar == NULL) {
		return 0;
	}
	for (i = 0; i < MAX_MYITEM; ) {
		WORD wItem = pChar->waItem[i];

		if ((wItem == 0) || !IsImportantItem (wItem)) {
			i ++;
			continue;
		}
		/* 装備中の物は動かさない（装備の添字がずれるため） */
		bEqu = FALSE;
		for (j = 0; j < MAX_EQUTYPE; j ++) {
			if (pChar->waEqu[j] == i) {
				bEqu = TRUE;
			}
		}
		if (bEqu) {
			i ++;
			continue;
		}
		/* 大事な物の欄に空きが無ければ、そのまま手持ちに置く */
		for (k = 0; k < MAX_IMPORTANT; k ++) {
			if (pChar->waImportant[k] == 0) {
				break;
			}
		}
		if (k >= MAX_IMPORTANT) {
			break;
		}
		pChar->waImportant[k] = wItem;
		/* 手持ちを詰める（装備の添字も一緒に直る）。詰めたので同じ位置をもう一度見る */
		DelCharItem (pChar->dwTblID, (BYTE)i);
		nRet ++;
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

	/* 存在しないアイテムを追加しようとした？（派生アイテムの番号も通す） */
	if ((dwItemID > 0xFFFF) || !ItemIDValid ((WORD)dwItemID)) {
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, (LPARAM)pChar->dwTblID);
		return;
	}

	/* 大事な物・依頼の品は手持ちではなく大事な物の欄へ（同じ物を重ねず1個1枠） */
	if (IsImportantItem ((WORD)dwItemID)) {
		for (i = 0; i < MAX_IMPORTANT; i ++) {
			if (pChar->waImportant[i]) {
				continue;
			}
			pChar->waImportant[i] = (WORD)dwItemID;
			break;
		}
		return;
	}

	AddCharItemBag (dwCharID, dwItemID);
}


/* ========================================================================= */
/* 関数名：	AddCharItemBag													 */
/* 内容：	手持ちの空きにアイテム追加（大事な物の振り分けはしない。取り引きで	 */
/*			手持ちを組み直す時用）											 */
/* ========================================================================= */

void AddCharItemBag(DWORD dwCharID, DWORD dwItemID)
{
	int i;
	PCHARINFO pChar;

	pChar = g_aCharInfo[dwCharID];

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
	/* 依頼の見えるNPCの出入りは、見えない人に知らせない */
	if (!IsQuestNpc (pChar)) {
		SendCmdMAPCHARINFO (pChar->dwTblID);
	}
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
	pChar->wShowFlag		= 0;				/* 見える・見えない条件の印（番号+1。0=無し） */
	pChar->wHideFlag		= 0;
	pChar->byViewAllNpc	= 0;
	pChar->wRecycleBox	= 0;				/* 開いているリサイクル箱（無し） */
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
	/* 依頼の見えるNPCの出入りは、見えない人に知らせない */
	if (!IsQuestNpc (pChar)) {
		SendCmdMAPCHARINFO (pChar->dwTblID);
	}
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
		if (!((pCharParent->dwMapID == pCharChild->dwMapID) && IsSameSpace (pCharParent, pCharChild) &&
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
	CharTargetCancelPCKeepBack (pChar);
	/* 後ろについてきている人も外す */
	TailOff (pChar);
}

/* ========================================================================= */
/* 関数名：	CharTargetCancelPCKeepBack										 */
/* 内容：	ターゲットと「前の人についていく」だけ解除し、					 */
/*			後ろについてきている人の列は残す								 */
/*			（移動の翼・町転送などで列ごと連れて行く時に使う）				 */
/* ========================================================================= */
void CharTargetCancelPCKeepBack(PCHARINFO pChar)
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

	/* 誰かについていっているなら、その人からは離れる */
	if (pChar->dwTailTblID != -1) {
		TailOff (pChar);
	}
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
		/* 違うマップ・違う空間？ */
		if ((pChar->dwMapID != pParent->dwMapID) || !IsSameSpace (pChar, pParent)) {
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
		pChar->dwTimeStep = timeGetTime ();

		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 1);
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		CheckCharPosSend (pChar);
		pChar->byFlg &= ~CHARINFOFLG_DIRECTION;
		pChar->byFlg &= ~CHARINFOFLG_MOVE;

		/* ついていっているプレイヤーと傭兵もダメージ床を踏む */
		FloorDamageOnStep (pChar);

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
	return CheckCharBlockFor (NULL, dwMapID, dwX, dwY);
}
/* pViewer がプレイヤーなら、その人の印で差し替えたパーツで判定する（NULL ならベースのパーツ） */
BOOL CheckCharBlockFor(PCHARINFO pViewer, DWORD dwMapID, DWORD dwX, DWORD dwY)
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

	nPartsID = MapSwapParts (pMap, pViewer, nX, nY);
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
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX, dwY);
		break;

	case CHARGRP_MAIN_3232:
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX + 1, dwY);
		break;

	case CHARGRP_MAIN_4848:
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX + 1, dwY);
		if (bRet == FALSE) {
			break;
		}
		bRet = CheckCharBlockFor (pChar, dwMapID, dwX + 2, dwY);
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckCharBlockPC													 */
/* 内容：キャラのキャラとのぶつかり判定										 */
/*       dwSpaceID と同じ空間にいるキャラとだけぶつかる						 */
/* 日付：2003/10/11															 */
/* ========================================================================= */
DWORD CheckCharBlockPC(DWORD dwMapID, DWORD dwX, DWORD dwY, DWORD dwSpaceID)
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
		/* 別の空間にいる人とはぶつからない */
		if (pCharTmp->dwSpaceID != dwSpaceID) {
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
/*       dwSpaceID と同じ空間にいるキャラとだけぶつかる						 */
/* 日付：2004/03/14															 */
/* ========================================================================= */
DWORD CheckCharBlockNPC(DWORD dwMapID, DWORD dwX, DWORD dwY, DWORD dwSpaceID, PCHARINFO pViewer)
{
	PMAPINFO pMap = g_aMapInfo[dwMapID];
	PDWORD pData = pMap->paTblNoNPC;
	for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		if (pData[i] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
		/* 依頼の見えるNPC・別の空間にいるキャラは、ぶつからない（すり抜けられる）。
		   動く本人（pViewer）に見えない条件付きNPCも、居ないのと同じでぶつからない */
		if (IsQuestNpc (pCharTmp) || (pCharTmp->dwSpaceID != dwSpaceID) || ((pViewer != NULL) && !IsCharSeenBy (pCharTmp, pViewer))) {
			continue;
		}
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
/*       dwSpaceID と同じ空間にいるキャラとだけぶつかる						 */
/* 日付：2004/08/01															 */
/* ========================================================================= */
DWORD CheckCharBlockNPC2(DWORD dwTblID, DWORD dwMapID, DWORD dwX, DWORD dwY, DWORD dwSpaceID, PCHARINFO pViewer)
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
		/* 依頼の見えるNPC・別の空間にいるキャラは、ぶつからない（すり抜けられる）。
		   動く本人（pViewer）に見えない条件付きNPCも、居ないのと同じでぶつからない */
		if (IsQuestNpc (pCharTmp) || (pCharTmp->dwSpaceID != dwSpaceID) || ((pViewer != NULL) && !IsCharSeenBy (pCharTmp, pViewer))) {
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

		if (!ItemIDValid ((WORD)nNo)) {
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
	int nAffixDFMagic;

	/* 攻撃力 */
	pChar->wAT = pChar->byPW;
	pChar->wAT += pChar->chTimerPW;

	/* 武器を装備している？ */
	if (pChar->waEqu[0] != 0xFFFF) {
		/* 強さは付与込み（未鑑定の派生アイテムは元のまま） */
		pChar->wAT += ItemValueOf (pChar->waItem[pChar->waEqu[0]]);
	}

	if (pChar->byNpcType == 0) {
		/* 魔法防御値を初期化 */
		pChar->wDFMagic = 0;
	}


	/* 守備力 */
	pChar->wDF = pChar->byST / 2;
	nAffixDFMagic = 0;

	for (int i = 0; i < MAX_EQUTYPE; i ++) {
		if (pChar->waEqu[i] != 0xFFFF) {
			WORD wEquItem = pChar->waItem[pChar->waEqu[i]];
			int nValue = ItemValueOf (wEquItem);
			pItem = g_aItemInfo[wEquItem];
			nAffixDFMagic += ItemAffixDFMagic (wEquItem);
			switch (pItem->byType) {
//			case ITEMTYPE_WEAPON:		/* 武器 */
//				break;
			case ITEMTYPE_ARMOR:		/* 鎧 */
			case ITEMTYPE_SHILD:		/* 盾 */
			case ITEMTYPE_HELMET:		/* 兜 */
			case ITEMTYPE_SEED:			/* 種 */
			case ITEMTYPE_EQU_HAND:		/* 装備(小手) */
			case ITEMTYPE_EQU_LEG:		/* 装備(靴) */
				pChar->wDF += nValue;
				break;
			case ITEMTYPE_EQU_ACC1:		/* 装備(首飾り) */
				{
					PITEMINFO_EQU_ACC1 pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += nValue;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += nValue;
						break;
					}
				}
				break;
			case ITEMTYPE_EQU_ACC2:		/* 装備(指輪1) */
				{
					PITEMINFO_EQU_ACC2 pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += nValue;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += nValue;
						break;
					}
				}
				break;
			case ITEMTYPE_EQU_ACC3:		/* 装備(指輪2) */
				{
					PITEMINFO_EQU_ACC3 pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
					switch (pInfo->byEquType) {
					case EQUTYPE_DF:
						pChar->wDF += nValue;
						break;
					case EQUTYPE_DFMAGIC:
						pChar->wDFMagic += nValue;
						break;
					}
				}
				break;
			}
		}
	}
/* 攻撃力・守備力アップ(僧侶の補助特技など) */
	if (pChar->byNpcType == NPCTYPE_INIT) {
		pChar->wAT = (WORD)min (65535, (int)pChar->wAT + GetStatusUpBonus (pChar, CHARINFOTIMEREVENT_ATUP, (int)pChar->wAT));
		pChar->wDF = (WORD)min (65535, (int)pChar->wDF + GetStatusUpBonus (pChar, CHARINFOTIMEREVENT_DFUP, (int)pChar->wDF));
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

	/* マップ移動したら、戦闘で列を離れた時の戻り先は忘れる
	   (戦闘モードの覚えは消さない。クライアントもマップ移動で戦闘モードを解除しないので合わせる) */
	TailReturnForget (pChar->dwTblID);

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
		/* マップ間移動のイベントは、ついてきているキャラを先に移動先へ移している。
		   ここでもう一度呼ぶと「すでに移動先」のフェードインが重なって届き、
		   ついていく側の画面が2回暗転するので、移動済みなら呼ばない */
		if (!((pCharTmp->dwMapID == dwMapID) &&
			  (pCharTmp->dwX == min (dwX, (DWORD)pMap->nCX - 1)) &&
			  (pCharTmp->dwY == min (dwY, (DWORD)pMap->nCY - 1)))) {
			/* 先頭と同じように暗転・BGM切り替えをしてから連れて行く
			   （移動の翼・町転送などでも列ごと移動する） */
			SendCmdCHGBGM (pCharTmp->pID, pCharTmp->dwMapID, 0);
			SendCmdFADEINOUT (pCharTmp->pID, 0);
			SubProcMapMove (pCharTmp, dwMapID, dwX, dwY);
		}
	}

	ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));

	/* マップ移動したら隠れるが解ける(この後のキャラ情報で周りに伝わる) */
	HideBreak (pChar, FALSE);

	pChar->dwLastHartBeat = timeGetTime ();
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
	/* 行き先のマップに、依頼の見えるNPCを出す・前のマップの分を片付ける */
	QuestSyncAll ();
}

/* 関数名：SubProcMoveEx													 */
/* 内容：同マップ内移動処理(向きを指定。bFadeOut=FALSEなら本人の暗転は			 */
/*		すでに始めてあるので送らない)										 */
/*		向きは CHARINFO を送る前に決める(ほかの人の画面にも合わせて出る)	 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
static void SubProcMoveEx(PCHARINFO pChar, DWORD dwX, DWORD dwY, BYTE byDirection, BOOL bFadeOut)
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

	if (bFadeOut) {
		SendCmdFADEINOUT (pChar->pID, 0);
	}
	pChar->byDirection = byDirection;
	pChar->byDirectionBack2 = pChar->byDirection;
	pChar->byDirectionBack = pChar->byDirection;
	pChar->dwX = pChar->dwXBack = dwX;
	pChar->dwY = pChar->dwYBack = dwY;
	SendCmdCHARINFO (pChar->pID, -1);
	SendCmdFADEINOUT (pChar->pID, 1);
	SendCmdCHARINFO3 (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：SubProcMove														 */
/* 内容：同マップ内移動処理													 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
void SubProcMove(PCHARINFO pChar, DWORD dwX, DWORD dwY)
{
	SubProcMoveEx (pChar, dwX, dwY, 1, TRUE);
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
	{
		/* 抜けた人と残った仲間の依頼の進み具合・見えるNPCを更新する */
		DWORD dwPartyIDOld = pChar->dwPartyID;
		pChar->dwPartyID = 0;
		QuestOnPartyLeave (pChar);
		QuestSendStateParty (dwPartyIDOld, pChar);
		QuestSyncAll ();
	}
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
	DWORD dwPartyIDOld = pChar->dwPartyID;
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
	{
		/* 解散した全員に、依頼の進み具合を送り直す（自分だけの分になる）。見えるNPCも更新する */
		std::vector<DWORD> aMember;
		for (i = 0; i < MAX_PARTYMEMBER; i ++) {
			if (pPartyInfo->adwTblID[i] == 0) {
				break;
			}
			aMember.push_back (pPartyInfo->adwTblID[i]);
		}
		ZeroMemory (pPartyInfo, sizeof (PARTYINFO));
		for (i = 0; i < (int)aMember.size (); i ++) {
			QuestOnPartyLeave (g_aCharInfo[aMember[i]]);
			QuestSendStateParty (0, g_aCharInfo[aMember[i]]);
		}
		QuestSyncAll ();
	}
	(void)dwPartyIDOld;
}


/* ========================================================================= */
/* 関数名：	IsTail	
														 */
/* 内容：	ついていけるかチェック											 */
/* 日付：	2006/08/05														 */
/* ========================================================================= */

BOOL IsTail(PCHARINFO pChar, PCHARINFO pCharTarget)
{
	BOOL bRet;
	DWORD dwX, dwY;

	bRet = FALSE;

	/* 依頼の見えるNPC・別の空間にいるキャラにはついていけない */
	if (!IsInteractable (pChar, pCharTarget)) {
		goto Exit;
	}

	/* 誰かについていっている？ */
	if (pChar->dwTailTblID != -1) {
		goto Exit;
	}

	/* 別のマップの相手（マップを移った直後で、画面にまだ残っている人など）にはついていけない */
	if (pChar->dwMapID != pCharTarget->dwMapID) {
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
	if (((byCmd >= CMD_MAPPARTSADD) && (byCmd != CMD_WEBACCOUNT) && (byCmd != CMD_REQ_GUARD) && (byCmd != CMD_RECIPEMAKE) && (byCmd != CMD_APPRAISE) && (byCmd != CMD_BATTLEMODE) && (byCmd != CMD_QUEST) && (byCmd != CMD_SORTITEM) && (byCmd != CMD_LEAVEITEMDISPOSE) && (byCmd != CMD_MAGICKEY) && (byCmd != CMD_TALKANSWER) && ((byCmd < CMD_MARKETLIST) || (byCmd > CMD_MARKETGOLDGET)) && ((byCmd < CMD_RECYCLELIST) || (byCmd > CMD_RECYCLECLOSE))) ||
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
		SendMagicKeySetting (pCharTmp);
		WebStatNoteLogin (pCharTmp);
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
	SendMagicKeySetting (pChar);
	WebStatNoteLogin (pChar);
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

	/* ログイン後の状態遷移は 1 回だけ（すでに歩ける状態なら何もしない） */
	if (g_aCharInfo[nNo]->byGameState >= GAMESTATE_MOVE) {
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

	pChar->dwLastHartBeat	= timeGetTime ();
	pChar->byGameState		= GAMESTATE_MOVE;
	pChar->byCharState		= CHARSTATE_MOVE;
	pChar->dwTimeLogin		= timeGetTime ();

	SendCmdSTTMPLATE (pChar->dwTblID, CMDSTTMPLATETYPE_ALL);
	/* マップパーツ情報を送信 */
	SendCmdMAPPARTSINFO (s_pIDRecv);
	/* 吹き出し情報を送信 */
	SendCmdCEFINFO (s_pIDRecv);
	/* エフェクト情報を送信 */
	SendCmdEFCINFO (s_pIDRecv);
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
	SendCmdITEMINFO (s_pIDRecv);
	/* 付与表と派生アイテム表を送信 */
	SendCmdAFFIXALL (s_pIDRecv);
	/* 作り手とレシピを送信 */
	SendCmdRECIPEINFOALL (s_pIDRecv);
	/* 町転送の行き先一覧を送信 */
	SendCmdTOWNWARP (pChar->dwTblID);
	/* 受けている依頼の確認と、本人・仲間の進み具合を送信 */
	/* 古いキャラの手持ちにある大事な物・依頼の品を、大事な物の欄へ移す */
	MoveImportantItems (pChar);
	QuestOnLogin (pChar);
	/* マップデータを送信 */
	SendCmdMAPINFO (s_pIDRecv, GetMapNo (pChar->dwMapID));
	/* イベント座標を送信 */
	SendCmdEVENTPOS (s_pIDRecv, GetMapNo (pChar->dwMapID));
	/* キャラデータを送信 */
	SendCmdCHARINFO (s_pIDRecv, -1);
	SendCmdCHARINFO3 (nNo);
	SendCmdCHARITEM (s_pIDRecv, pChar->dwTblID);
	SendCmdCHARSTATUS (s_pIDRecv, nNo);

	SendCmdCHGSTATE (s_pIDRecv, (DWORD)s_nCharNoRecv, GAMESTATE_MOVE);
	if (pChar->byAdminLV == ADMINLV_ADMINISTRATOR) {
		SendCmdADMINWINDOW (s_pIDRecv, 1);
		SendCmdSUBGAMEINFOALL (s_pIDRecv);
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

	SendCmdSYSMSG (s_pIDRecv, "SYSTEM:ｽｸﾗｯﾌﾟﾌﾞｯｸｵﾝﾗｲﾝの世界へようこそ♪");
	wsprintf (szBuf, "SYSTEM:現在のｵﾝﾗｲﾝ数:%ld人", g_dwOnline);
	SendCmdSYSMSG (s_pIDRecv, szBuf);
	wsprintf (szBuf, "SYSTEM:最新ｸﾗｲｱﾝﾄﾊﾞｰｼﾞｮﾝ:%s", g_szCliVer);
	SendCmdSYSMSG (s_pIDRecv, szBuf);

	if (g_dwOnline % 10 == 0) {
		wsprintf (szBuf, "SYSTEM:オンライン数が%d人になりました。", g_dwOnline);
		SendCmdSYSMSG (URARASOCK_SENDALL, szBuf);
	}
	SendMsgMostCrowdedMap (s_pIDRecv);
	/* 再起動の予告の後にログインした人にも、もうすぐ再起動することを知らせる */
	SboWebSendLoginNotice (s_pIDRecv);
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
/* 関数名：IsDamageFloor													 */
/* 内容：ダメージ床（パーツの byDamage が 0 でない）のマスか					 */
/* ========================================================================= */
BOOL IsDamageFloor(PMAPINFO pMap, int x, int y)
{
	WORD wParts;

	if ((pMap == NULL) || (x < 0) || (y < 0) || (x >= pMap->nCX) || (y >= pMap->nCY)) {
		return FALSE;
	}
	wParts = pMap->pwMap[x + y * pMap->nCX];
	if (wParts >= g_wCountMapParts) {
		return FALSE;
	}
	return (g_aMapPartsInfo[wParts].byDamage != 0);
}

/* ========================================================================= */
/* 関数名：IsNoFloorDamageChar												 */
/* 内容：ダメージ床が効かない敵か（ステータステンプレートの byNoFloorDamage） */
/*		 発生元から出た敵はテンプレートを持たないので、発生元のものを見る	 */
/* ========================================================================= */
BOOL IsNoFloorDamageChar(PCHARINFO pChar)
{
	DWORD dwStTmp;

	if (pChar->byNpcType == NPCTYPE_INIT) {
		return FALSE;
	}
	dwStTmp = pChar->dwStTmplateID;
	if ((dwStTmp == (DWORD)-1) && (pChar->dwParentTblID != (DWORD)-1) &&
		(pChar->dwParentTblID < (DWORD)g_aCharInfo.GetSize ()) && g_aCharInfo[pChar->dwParentTblID]) {
		dwStTmp = g_aCharInfo[pChar->dwParentTblID]->dwStTmplateID;
	}
	if ((dwStTmp == (DWORD)-1) || (dwStTmp >= (DWORD)g_aStTmpInfo.GetSize ())) {
		return FALSE;
	}
	return (g_aStTmpInfo[dwStTmp]->byNoFloorDamage != 0);
}

/* ========================================================================= */
/* 関数名：IsAvoidDamageFloor												 */
/* 内容：敵・傭兵が自分から歩く時に、よけるダメージ床か						 */
/*		 もうダメージ床の上にいる時は、出られなくならないように、よけない	 */
/*		 ダメージ床が効かない敵（ボスなど）は、よけずに素通りする			 */
/* ========================================================================= */
BOOL IsAvoidDamageFloor(PCHARINFO pChar, PMAPINFO pMap, int x, int y)
{
	if (IsDamageFloor (pMap, x, y) == FALSE) {
		return FALSE;
	}
	if (IsNoFloorDamageChar (pChar)) {
		return FALSE;
	}
	return (IsDamageFloor (pMap, (int)pChar->dwX, (int)pChar->dwY) == FALSE);
}

/* ========================================================================= */
/* 関数名：FloorDamageOnStep												 */
/* 内容：ダメージ床（毒沼・溶岩など）のマスに1歩入った時の処理				 */
/*		 パーツの byDamage だけHPが減る(HPは1で止まり、床では倒れない)		 */
/*		 byPoison が 0 でなければ、その秒数だけ毒になる(毒の強さは byDamage)	 */
/*		 プレイヤー・傭兵・敵(戦闘1・戦闘2)が踏む。ほかのNPCは踏まない		 */
/* ========================================================================= */
void FloorDamageOnStep(PCHARINFO pChar)
{
	int nMapNo, nDamage;
	PMAPINFO pMap;
	PMAPPARTSINFO pParts;
	WORD wParts;
	char szTmp[16];

	/* 踏むのはプレイヤー・傭兵・敵だけ（村人・押せる岩・爆弾などは踏まない） */
	switch (pChar->byNpcType) {
	case NPCTYPE_INIT:			/* プレイヤー */
	case NPCTYPE_MERCENARY:		/* 傭兵 */
	case NPCTYPE_BATTLE1:		/* 敵(戦闘1) */
	case NPCTYPE_BATTLE2:		/* 敵(戦闘2。正体を現した宝箱の敵も) */
		break;
	default:
		return;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}
	/* ダメージ床が効かない敵（ボスなど）は、HPも減らず毒にもならない */
	if (IsNoFloorDamageChar (pChar)) {
		return;
	}
	nMapNo = GetMapNo (pChar->dwMapID);
	if (nMapNo < 0) {
		return;
	}
	pMap = g_aMapInfo[nMapNo];
	if ((pChar->dwX >= (DWORD)pMap->nCX) || (pChar->dwY >= (DWORD)pMap->nCY)) {
		return;
	}
	wParts = MapSwapParts (pMap, pChar, (int)pChar->dwX, (int)pChar->dwY);
	if (wParts >= g_wCountMapParts) {
		return;
	}
	pParts = &g_aMapPartsInfo[wParts];
	if (pParts->byDamage == 0) {
		return;
	}

	nDamage = min ((int)pParts->byDamage, (int)pChar->wHP - 1);
	if (nDamage > 0) {
		pChar->wHP -= (WORD)nDamage;
		wsprintf (szTmp, "%d", nDamage);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (250, 60, 4), szTmp);
		/* HPを知らせるのはプレイヤーだけ（傭兵は傭兵の処理が右下の一覧へ送り直す） */
		if (pChar->byNpcType == NPCTYPE_INIT) {
			SendCmdCHARSTATUSHPMANA (pChar->dwTblID, pChar->dwTblID);
		}
	}
	/* 毒になる床（もう毒なら、かけ直さない。歩くたびに毒の時計が戻らないように） */
	if (pParts->byPoison && !CheckCharInfoTimer (pChar->dwTblID, CHARINFOTIMEREVENT_POISON)) {
		SetCharInfoTimerStatus (pChar, CHARINFOTIMEREVENT_POISON, pParts->byDamage, (DWORD)pParts->byPoison * 1000);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (190, 80, 255), "毒");
	}
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
		/* 列に戻る途中(サーバーが歩かせている)は、自分では動けない。今の位置を送り返す */
		if (TailChaseIsActive (pChar)) {
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			return;
		}
		/* 町転送の床に乗って発動を待っている間は、先へ歩かせない
		  （方向キーを押したままだと、歩き終わりを待つ間に床を通り過ぎてしまうので） */
		if (pChar->dwTimeReturnHold) {
			if (timeGetTime () - pChar->dwTimeReturnHold >= RETURN_HOLDTIME) {
				pChar->dwTimeReturnHold = 0;
			} else if ((pChar->dwX != (DWORD)pPara->byX) || (pChar->dwY != (DWORD)pPara->byY)) {
				SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
				return;
			}
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
		BOOL bStep = ((pChar->dwX != (DWORD)pPara->byX) || (pChar->dwY != (DWORD)pPara->byY));
		pChar->byDirection = pPara->byDirection;
		pChar->dwX = (DWORD)pPara->byX;
		pChar->dwY = (DWORD)pPara->byY;
		if (bStep) {
			/* 歩き始めた時刻（町転送の床は歩き終わるのを待ってから飛ばす） */
			pChar->dwTimeStep = timeGetTime ();
		}

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

		/* ダメージ床（毒沼・溶岩など）に入った？ */
		if (bStep) {
			FloorDamageOnStep (pChar);
		}

		/* 依頼の見えるポータルに乗った？ */
		QuestOnStep (pChar);
	}
}

/* ========================================================================= */
/* 関数名：IsCharInfoReqAllowed												 */
/* 内容：キャラ情報要求で、相手の情報を返してよいか（本人・同じ画面の周り・	 */
/*       仲間・自分の傭兵・ついていくつながり）							 */
/* ========================================================================= */
static BOOL IsCharInfoReqAllowed(PCHARINFO pChar, PCHARINFO pTarget)
{
	int i, nCount = g_aCharInfo.GetSize ();
	DWORD dwTblID;

	if (pChar == pTarget) {
		return TRUE;
	}
	/* 同じマップ・同じ空間で、画面に入りうる範囲（動き回る分のゆとりを持たせる） */
	if ((pChar->dwMapID == pTarget->dwMapID) && IsSameSpace (pChar, pTarget) &&
		(abs ((int)pChar->dwX - (int)pTarget->dwX) <= DRAW_PARTS_X + 6) &&
		(abs ((int)pChar->dwY - (int)pTarget->dwY) <= DRAW_PARTS_Y + 6)) {
		return TRUE;
	}
	/* 自分のパーティの仲間 */
	if ((pChar->dwPartyID != 0) && (pChar->dwPartyID == pTarget->dwPartyID)) {
		return TRUE;
	}
	/* 自分の傭兵 */
	if (pTarget->dwParentTblID == pChar->dwTblID) {
		return TRUE;
	}
	/* 「ついていく」でつながっている列（前へたどる・後ろへたどる） */
	dwTblID = pChar->dwTblID;
	for (i = 0; i < nCount; i ++) {
		if (dwTblID >= (DWORD)nCount) {
			break;
		}
		if (dwTblID == pTarget->dwTblID) {
			return TRUE;
		}
		dwTblID = g_aCharInfo[dwTblID]->dwTailTblID;
	}
	dwTblID = pChar->dwTblID;
	for (i = 0; i < nCount; i ++) {
		if (dwTblID >= (DWORD)nCount) {
			break;
		}
		if (dwTblID == pTarget->dwTblID) {
			return TRUE;
		}
		dwTblID = g_aCharInfo[dwTblID]->dwTailBackTblID;
	}
	return FALSE;
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

	/* 管理者以外は、関わりのあるキャラの情報だけ返す（画面に入りうる範囲・仲間・傭兵・ついていく相手） */
	if (!s_bAdminRecv && (pPara->dwTblIDReq >= 0)) {
		if (((DWORD)pPara->dwTblIDReq >= (DWORD)g_aCharInfo.GetSize ()) ||
			!IsCharInfoReqAllowed (g_aCharInfo[pPara->dwTblID], g_aCharInfo[pPara->dwTblIDReq])) {
			return;
		}
	}

	SendCmdCHARINFO2 (s_pIDRecv, pPara->dwTblID, pPara->dwTblIDReq);
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
		pPartsTmp1->byLow = pPartsTmp2->byLow;
		pPartsTmp1->byDamage = pPartsTmp2->byDamage;
		pPartsTmp1->byPoison = pPartsTmp2->byPoison;
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
	pPartsTmp->byLight = MAPPARTS_SAVE_LIGHT (pRecv);
	pPartsTmp->byLow = MAPPARTS_SAVE_LOW (pRecv);
	pPartsTmp->byDamage = pRecv->byDamage;
	pPartsTmp->byPoison = pRecv->byPoison;
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
	SaveMarket ();
	SaveRecycle ();
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
	WORD wBGMOld = pMap->wBGM;
	BOOL bChgColor = FALSE;
	if (IsValidBgmNo (pPara->wBGM)) {
		pMap->wBGM = pPara->wBGM;
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
	pMap->byLightCount = min (pPara->byLightCount, MAX_MAPLIGHT);
	CopyMemory (pMap->aLight, pPara->aLight, sizeof (pMap->aLight));
	/* パーツの灯りの上書き（範囲外のパーツの行は捨て、範囲は 0〜MAPLIGHT_RANGE_MAX に丸める） */
	ZeroMemory (pMap->aPartsLight, sizeof (pMap->aPartsLight));
	pMap->byPartsLightCount = 0;
	for (int nPL = 0; nPL < (int)pPara->byPartsLightCount && nPL < MAX_MAPPARTSLIGHT; nPL ++) {
		if (pPara->aPartsLight[nPL].wParts >= g_wCountMapParts) {
			continue;
		}
		PMAPPARTSLIGHT pPL = &pMap->aPartsLight[pMap->byPartsLightCount ++];
		pPL->wParts = pPara->aPartsLight[nPL].wParts;
		pPL->byLight = min (pPara->aPartsLight[nPL].byLight, (BYTE)MAPLIGHT_RANGE_MAX);
	}

	/* BGMが変更された？ */
	if (wBGMOld != pPara->wBGM) {
		SendCmdCHGBGM (URARASOCK_SENDALL, pPara->dwID, pPara->wBGM);
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
		/* クライアントは返事が来るまで歩けない（Z も効かない）ので、位置を送って待ちを解く */
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}
	/* 足元を調べた時(Sキー)、同じマスに依頼の見える宝箱などがあればそちらを先に */
	if ((pPara->byData == 1) && QuestExamineHere (pChar)) {
		/* クライアントは返事が来るまで歩けないので、位置を送って待ちを解く */
		SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
		return;
	}

	/* 目の前を調べた時(Sキー)、目の前に宝箱の敵(正体前)がいれば正体を現す */
	if (pPara->byData == 2) {
		PCHARINFO pMimic = FindFrontMimic (pChar);
		if (pMimic) {
			MimicReveal (pMimic, pChar);
			/* クライアントは返事が来るまで歩けないので、位置を送って待ちを解く */
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			return;
		}
	}

	/* 目の前を調べた時(Sキー)、目の前が「印で変わるマス」の宝箱で、まだ開けていなければ開ける */
	if ((pPara->byData == 2) && MapSwapExamine (pChar)) {
		return;
	}

	pMap = g_aMapInfo[nNo];
	/* イベントが1つも無いマップでも、下の「何も無い時」の返事まで進む
	   （ここで黙って戻ると、クライアントは返事待ちのまま歩けず、Z も効かなくなっていた） */
	DWORD dwX, dwY;
	for (i = 0; (pMap->pEvent != NULL) && (i < pMap->nEventCount); i ++) {
		pEvent = &pMap->pEvent[i];
		dwX = pChar->dwX;
		dwY = pChar->dwY;

		if ((pEvent->byType == EVENTTYPE_MSG) && (pPara->byData == 2)) {
			dwX += anPosAddTblX[pChar->byDirection];
			dwY += anPosAddTblY[pChar->byDirection];
		}
		/* イベント発見？ */
		if ((pEvent->dwX == dwX) && (pEvent->dwY == dwY)) {
			/* 足元を調べた時(Sキー)、足元のイベントが調べても何も起きない物なら
			   目の前の扉を開ける（扉の前のマスに乗った時のメッセージなどがあると開けられなかった） */
			if ((pPara->byData == 1) && !IsEventExamine (pEvent, pPara->byData)) {
				if (OpenDoorFront (pChar)) {
					return;
				}
			}
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
/* 関数名：FindFrontMimic													 */
/* 内容：目の前の宝箱の敵(正体前)を探す										 */
/*		 戻り値：見つけた宝箱の敵（いなければ NULL）						 */
/* 日付：2026/11/13															 */
/* ========================================================================= */
static PCHARINFO FindFrontMimic(PCHARINFO pChar)
{
	PMAPINFO pMap;
	PDWORD pData;
	DWORD dwX, dwY;
	int i, nNo;

	nNo = GetMapNo (pChar->dwMapID);
	if (nNo < 0) {
		return NULL;
	}
	pMap = g_aMapInfo[nNo];
	pData = pMap->paTblNoNPC;
	if (pData == NULL) {
		return NULL;
	}
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		PCHARINFO pCharTmp;

		if (pData[i] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[i]];
		if ((pCharTmp->byNpcType != NPCTYPE_MIMIC) || (pCharTmp->wHP == 0)) {
			continue;
		}
		/* 別の空間のキャラは調べられない */
		if (pCharTmp->dwSpaceID != pChar->dwSpaceID) {
			continue;
		}
		if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (pCharTmp->dwY == dwY)) {
			return pCharTmp;
		}
	}
	return NULL;
}

/* ========================================================================= */
/* 関数名：IsEventExamine													 */
/* 内容：調べた時(byData)に何か起きるイベントか								 */
/*		 調べて反応するのはメッセージの「調べた時」の種類だけ				 */
/*		 （それ以外は EventProc が「何も見つかりませんでした」を返す）		 */
/* ========================================================================= */
static BOOL IsEventExamine(PEVENTINFO pEvent, BYTE byData)
{
	PEVENTINFO_MSG pMsg;

	if (pEvent->byType != EVENTTYPE_MSG) {
		return FALSE;
	}
	pMsg = (PEVENTINFO_MSG)pEvent->pData;
	if (pMsg == NULL) {
		return FALSE;
	}
	return (pMsg->byType == byData);
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
	/* 追加した管理者のいる空間に出す（API から来た時は、ふつうの世界） */
	pCharTmp->dwSpaceID = (s_nCharNoRecv >= 0) ? g_aCharInfo[s_nCharNoRecv]->dwSpaceID : SPACE_NORMAL;
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
	BOOL bViewChg = FALSE;

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
		/* 見える・見えない条件の印（番号+1。0 や範囲外は「無し」）。変わったらマップの全員に見え直しを送る */
		WORD wShowNew = CharFlagWireFix (pChar->wShowFlagNo);
		WORD wHideNew = CharFlagWireFix (pChar->wHideFlagNo);
		bViewChg = (pCharDst->wShowFlag != wShowNew) || (pCharDst->wHideFlag != wHideNew);
		pCharDst->wShowFlag = wShowNew;
		pCharDst->wHideFlag = wHideNew;
	}
	pCharDst->wGrpID = pChar->wGrpID;
	pCharDst->wGrpMain = pChar->wGrpMain;
	pCharDst->wGrpSub = pChar->wGrpSub;
	pCharDst->dwMapID = pChar->dwMapID;
	/* (0,0)は禁止。 */
	if (!((pChar->dwStartX == 0) && (pChar->dwStartY == 0))) {
		BOOL bStartChg = (pCharDst->dwStartMapID != pChar->dwStartMapID);

		pCharDst->dwStartMapID = pChar->dwStartMapID;
		pCharDst->dwStartX = pChar->dwStartX;
		pCharDst->dwStartY = pChar->dwStartY;
		/* プレイヤーの復活位置の町が変わったら、町転送の行き先一覧も送り直す
		   （一覧の0番は復活位置。送り直さないと本人の画面の並びとサーバーの並びがずれて、
		     選んだ町と違う町へ飛んでしまう） */
		if (bStartChg && (pCharDst->byNpcType == NPCTYPE_INIT) && (pCharDst->byGameState != GAMESTATE_NPC)) {
			SendCmdTOWNWARP (pCharDst->dwTblID);
		}
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
	if (bViewChg) {
		CharViewRefreshNpc (pCharDst);
	}

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
/* 関数名：DoorHasKey														 */
/* 内容：鍵の扉の鍵（wKeyItem）を、大事な物の欄か手持ちに持っているか			 */
/* ========================================================================= */
static BOOL DoorHasKey(PCHARINFO pChar, PEVENTINFO_DOOR pDoor)
{
	int i;

	if ((pDoor->wKeyItem == 0) || (pDoor->wKeyItem >= (WORD)g_aItemInfo.GetSize ())) {
		return FALSE;
	}
	if (CountImportantItem (pChar, pDoor->wKeyItem) > 0) {
		return TRUE;
	}
	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == pDoor->wKeyItem) {
			return TRUE;
		}
	}
	return FALSE;
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
			PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEvent->pData;
			/* 鍵の扉（鍵の品を大事な物の欄か手持ちに持っていれば開く。鍵は減らない。品の名前は教えない） */
			if (pDoor && (pDoor->byType == EVENTDOOTTYPE_KEY)) {
				if (SpaceDoorIsOpen (pChar->dwMapID, pEvent, pDoor, pChar->dwSpaceID)) {
					return FALSE;
				}
				if (!DoorHasKey (pChar, pDoor)) {
					SendCmdMAPMSG (pChar->pID, "鍵がかかっている。@2");
					SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
					return TRUE;
				}
				if (!SpaceDoorOpenKey (pChar->dwMapID, pEvent, pDoor, pChar->dwSpaceID)) {
					return FALSE;
				}
				char szKey[128];
				wsprintf (szKey, "%sで扉を開けた。@2", g_aItemInfo[pDoor->wKeyItem]->szName);
				SendCmdMAPMSG (pChar->pID, szKey);
				SendCmdPLAYSOUND (pChar->pID, SOUND_DOOR);
				SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
				return TRUE;
			}
			/* 敵を全部倒すと開く扉（手では開けられない。閉まっていたら残りの敵の数を知らせる） */
			if (pDoor && (pDoor->byType == EVENTDOOTTYPE_ENEMY)) {
				if (SpaceDoorIsOpen (pChar->dwMapID, pEvent, pDoor, pChar->dwSpaceID)) {
					return FALSE;
				}
				char szTmp[128];
				int nEnemy = SpaceDoorEnemyCount (pChar->dwMapID, pDoor, pChar->dwSpaceID);
				if (nEnemy > 0) {
					wsprintf (szTmp, "扉はかたく閉ざされている。まだ魔物の気配がする…（残り%d体）@2", nEnemy);
				} else {
					wsprintf (szTmp, "扉はかたく閉ざされている。@2");
				}
				SendCmdMAPMSG (pChar->pID, szTmp);
				SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
				return TRUE;
			}
			/* すでに開いてる？（扉は空間ごとに開け閉めする。開けたことはその空間の人にだけ知らせる） */
			if (!SpaceDoorOpen (pChar->dwMapID, pEvent, pDoor, pChar->dwSpaceID)) {
				return FALSE;
			}
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
	/* もう名前のあるキャラに名前付きで届いたら（作成画面は名前が空の時だけ出る）、何も変えない。
	   以前はここで作成日を今日に書き換え、同名あつかいで名前まで消していたので、
	   レベルやプレイ時間はそのままで作成日だけ新しくなることがあった */
	/* （名前を空で送るのは「冒険の書をけす」。使えない名前で空にされた時は消さない） */
	if ((pChar->szName[0] != 0) && ((pPara->szName[0] != 0) || !bName)) {
		if (pChar->pID) {
			SendCmdCHARINFO (pChar->pID, -1);
		}
		return;
	}
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

	} else if (bName) {
		/* 作成日は名前が決まった時だけ（同名などで断った時は付けない） */
		SYSTEMTIME sysTime;
		GetLocalTime (&sysTime);
		pChar->wMakeYear = sysTime.wYear;
		pChar->wMakeMonth = sysTime.wMonth;
		pChar->wMakeDay = sysTime.wDay;
	}

	if (bName) {
		SendCmdCHARINFO (s_pIDRecv, pPara->dwTblID);

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
		SendCmdSYSMSG (s_pIDRecv, "SYSTEM:管理人は現在不在の為応答できません。");
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

	/* 倒れていない人は復活しない（討伐チャレンジで倒れて元に戻った後に、開いたままの窓から押された時など） */
	if ((pChar->byCharState != CHARSTATE_DEAD) && (pChar->wHP > 0)) {
		return;
	}
	/* 討伐チャレンジ中なら中断して入る前の場所へ（お金は減らさない） */
	if (QuestChallengeResurrection (pChar)) {
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
	/* ついていっているプレイヤーは、戦闘モードの間だけ列から抜ける */
	TailBattleMode (g_aCharInfo[pPara->dwTblID], pPara->byData != 0);
}


/* ========================================================================= */
/* 戦闘モードで列を離れたプレイヤーが、戻る先の先頭を覚えておく入れ物		 */
/* CHARINFO は通信・保存でそのまま使われるので、サーバーだけの表で持つ		 */
/* 先頭の pID と名前も覚え、テーブルIDの使い回しで別人に戻らないようにする	 */
/* ========================================================================= */
typedef struct _TAILRETURN {
	DWORD	dwPID;											/* 本人のpID(使い回し確認用) */
	DWORD	dwHeadTblID;									/* 先頭のテーブルID */
	DWORD	dwHeadPID;										/* 先頭のpID */
	char	szHeadName[sizeof (((PCHARINFO)0)->szName)];	/* 先頭の名前 */
	int		nOrder;											/* 列での並び順(先頭から何番目か。戻る時の順番) */
} TAILRETURN;
static std::map<DWORD, TAILRETURN> s_mapTailReturn;


/* ========================================================================= */
/* 戦闘モードを解除して列に戻る途中(前の人を歩いて追いかけている・消えている)の人 */
/* 前の人 = 並び順で一つ前に戻る人(その人がまだ追いかけ中ならその人)、いなければ	 */
/* 先頭の列の今の最後尾。前の人が列につながるまでは着いても列にはつながない	 */
/* (並び順を守る)。追いかけ中はサーバーの dwTailTblID は -1 のまま(誰かが後ろに	 */
/* つながっても引っぱられない)で、クライアントだけ CHAR_TAIL_CHASE で動けなくする */
/* ========================================================================= */
#define TAILCHASE_STEPTIME		(290)		/* 1歩の間隔(ms)。ふつうの歩き(1マス約280ms)と同じくらい */
#define TAILCHASE_FADETIME		(900)		/* 消え始めてから現れるまで(ms)。傭兵のワープと同じ */
#define TAILCHASE_LIMIT			(10000)		/* 追いかけてもこの時間たっても着かなければワープ(ms) */
#define TAILCHASE_STUCK			(3000)		/* これだけ近づけなければワープ(ms) */
#define TAILCHASE_GIVEUP		(30000)		/* 前の人を待つ分も入れて、これだけかかったら戻れなかったことにする(ms) */
#define TAILCHASE_PATHDIST		(12)		/* 道さがしで見る範囲(自分から何マスか) */

typedef struct _TAILCHASE {
	TAILRETURN	Info;							/* 戻り先(先頭・並び順) */
	DWORD		dwOrigStart;					/* 戻り始めた時間 */
	DWORD		dwStart;						/* 今の追いかけを始めた時間(となりで待つ間は進める) */
	DWORD		dwLastStep;						/* 最後に歩いた時間 */
	DWORD		dwProgress;						/* 最後に近づけた時間 */
	DWORD		dwWarpTime;						/* 消え始めた時間(0=消えていない) */
	DWORD		dwPrevTblID;					/* クライアントに知らせた追いかける相手 */
	int			nBestDist;						/* 今までで一番近づいた道のり */
} TAILCHASE;
static std::map<DWORD, TAILCHASE> s_mapTailChase;
static void TailChaseCancel(PCHARINFO pChar, LPTSTR pszMsg, BOOL bRestore);


/* ========================================================================= */
/* 関数名：	TailReturnForget												 */
/* 内容：	覚えている戻り先の先頭を消す									 */
/*			(Zで別の人についていく・解除・マップ移動・ログアウト・倒れた時)	 */
/*			この人を先頭として覚えている人も全員忘れさせ、戻れないと知らせる	 */
/*			(先頭が戦闘モードを解除できないので、覚えたままにならないように)	 */
/*			列に戻る途中(追いかけ中)の人も同じ(本人の追いかけもやめる)		 */
/* ========================================================================= */

void TailReturnForget(DWORD dwTblID)
{
	if (s_mapTailReturn.empty () && s_mapTailChase.empty ()) {
		return;
	}
	s_mapTailReturn.erase (dwTblID);
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	/* 本人の追いかけをやめる(クライアントには動けるように戻す) */
	if ((dwTblID < dwCount) && (s_mapTailChase.find (dwTblID) != s_mapTailChase.end ())) {
		TailChaseCancel (g_aCharInfo[dwTblID], NULL, FALSE);
	}

	/* この人を先頭として覚えている人を集める */
	std::vector<DWORD> adwMember;
	std::map<DWORD, TAILRETURN>::iterator it;
	for (it = s_mapTailReturn.begin (); it != s_mapTailReturn.end (); ++ it) {
		if (it->second.dwHeadTblID == dwTblID) {
			adwMember.push_back (it->first);
		}
	}
	for (size_t i = 0; i < adwMember.size (); i ++) {
		DWORD dwMember = adwMember[i];
		DWORD dwPID = s_mapTailReturn[dwMember].dwPID;
		s_mapTailReturn.erase (dwMember);
		if (dwMember >= dwCount) {
			continue;
		}
		PCHARINFO pMember = g_aCharInfo[dwMember];
		if ((pMember->pID != 0) && (pMember->pID == dwPID)) {
			SendCmdSYSMSG (pMember->pID, "列に戻れませんでした。", SPEECHCMD_BATTLE1);
		}
	}

	/* この人を先頭として戻る途中の人も、追いかけをやめる */
	std::vector<DWORD> adwChase;
	std::map<DWORD, TAILCHASE>::iterator itChase;
	for (itChase = s_mapTailChase.begin (); itChase != s_mapTailChase.end (); ++ itChase) {
		if (itChase->second.Info.dwHeadTblID == dwTblID) {
			adwChase.push_back (itChase->first);
		}
	}
	for (size_t i = 0; i < adwChase.size (); i ++) {
		if (adwChase[i] < dwCount) {
			TailChaseCancel (g_aCharInfo[adwChase[i]], "列に戻れませんでした。", TRUE);
		}
	}
}


/* ========================================================================= */
/* 関数名：	TailReturnFindEnd												 */
/* 内容：	覚えた先頭の列の最後尾を探す(戻れない時は NULL)					 */
/* ========================================================================= */

static PCHARINFO TailReturnFindEnd(PCHARINFO pChar, const TAILRETURN &Info)
{
	int i;
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PCHARINFO pHead, pCur, pNext;

	if (Info.dwHeadTblID >= dwCount) {
		return NULL;
	}
	pHead = g_aCharInfo[Info.dwHeadTblID];
	/* 先頭がまだ同じ人で、同じマップ・同じ空間にいて、倒れていない？ */
	if ((pHead == pChar) || (pHead->pID == 0) || (pHead->pID != Info.dwHeadPID) ||
		(strcmp (pHead->szName, Info.szHeadName) != 0) ||
		(pHead->dwMapID != pChar->dwMapID) || !IsInteractable (pChar, pHead) ||
		(pHead->byCharState == CHARSTATE_DEAD) || (pHead->wHP == 0)) {
		return NULL;
	}
	/* 最後尾までたどる(輪になっていても止まるように回数を区切る) */
	pCur = pHead;
	for (i = 0; i < 100; i ++) {
		if ((pCur->dwTailBackTblID == (DWORD)-1) || (pCur->dwTailBackTblID >= dwCount)) {
			return pCur;
		}
		pNext = g_aCharInfo[pCur->dwTailBackTblID];
		if ((pNext == pChar) || (pNext->dwTailTblID != pCur->dwTblID) || (pNext->dwMapID != pChar->dwMapID)) {
			return NULL;
		}
		pCur = pNext;
	}
	return NULL;
}


/* ========================================================================= */
/* 関数名：	TailChaseTarget													 */
/* 内容：	追いかける相手を決める											 */
/*			並び順で一つ前に戻る人(まだ追いかけ中)、いなければ先頭の列の最後尾	 */
/*			bPending=TRUE なら相手はまだ列につながっていない					 */
/*			先頭がいなくなっていれば FALSE									 */
/* ========================================================================= */

static BOOL TailChaseTarget(PCHARINFO pChar, const TAILRETURN &Info, PCHARINFO &pTarget, BOOL &bPending)
{
	PCHARINFO pEnd, pTmp;
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	int nBest = -1;

	pEnd = TailReturnFindEnd (pChar, Info);
	if (pEnd == NULL) {
		return FALSE;
	}
	pTarget		= pEnd;
	bPending	= FALSE;
	std::map<DWORD, TAILCHASE>::iterator it;
	for (it = s_mapTailChase.begin (); it != s_mapTailChase.end (); ++ it) {
		const TAILRETURN &InfoTmp = it->second.Info;
		if ((it->first == pChar->dwTblID) || (it->first >= dwCount) ||
			(InfoTmp.dwHeadTblID != Info.dwHeadTblID) || (InfoTmp.dwHeadPID != Info.dwHeadPID) ||
			(InfoTmp.nOrder >= Info.nOrder)) {
			continue;
		}
		pTmp = g_aCharInfo[it->first];
		if ((pTmp->pID == 0) || (pTmp->pID != InfoTmp.dwPID) ||
			(pTmp->dwMapID != pChar->dwMapID) || !IsSameSpace (pChar, pTmp)) {
			continue;
		}
		if (InfoTmp.nOrder > nBest) {
			nBest		= InfoTmp.nOrder;
			pTarget		= pTmp;
			bPending	= TRUE;
		}
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	TailChaseCanStep												 */
/* 内容：	追いかける時にそのマスへ歩いていけるか							 */
/*			地形・イベント(扉も開けない)・ダメージ床・人が立っているマスは避ける */
/* ========================================================================= */

static BOOL TailChaseCanStep(PCHARINFO pChar, PMAPINFO pMap, int x, int y)
{
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}
	if (CheckCharBlock (pChar, pChar->dwMapID, (DWORD)x, (DWORD)y) == FALSE) {
		return FALSE;
	}
	if (CheckEventBlock (pChar->dwMapID, (DWORD)x, (DWORD)y, 0xFF)) {
		return FALSE;
	}
	if (IsAvoidDamageFloor (pChar, pMap, x, y)) {
		return FALSE;
	}
	if (CheckCharBlockPC (pChar->dwMapID, (DWORD)x, (DWORD)y, pChar->dwSpaceID) != (DWORD)-1) {
		return FALSE;
	}
	if (CheckCharBlockNPC2 (pChar->dwTblID, pChar->dwMapID, (DWORD)x, (DWORD)y, pChar->dwSpaceID, pChar) != (DWORD)-1) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	TailChaseFindDir												 */
/* 内容：	相手のとなりのマスへ行く一番近い道の、最初の向き(幅優先)		 */
/*			自分のまわり TAILCHASE_PATHDIST マスの中だけ探す				 */
/*			(行けなければ -1、もうとなりなら -2。*pnDist は道のり)			 */
/*			傭兵の道さがし(CCharProcMERCENARY::PathDir)と同じ考え方で、		 */
/*			傭兵は閉まった扉を開けるが、こちらは扉を開けない				 */
/* ========================================================================= */

static int TailChaseFindDir(PCHARINFO pChar, PCHARINFO pTarget, int *pnDist)
{
	const int nR = TAILCHASE_PATHDIST;
	const int nW = nR * 2 + 1;
	static short s_anFirst[(TAILCHASE_PATHDIST * 2 + 1) * (TAILCHASE_PATHDIST * 2 + 1)];
	static short s_anDist[(TAILCHASE_PATHDIST * 2 + 1) * (TAILCHASE_PATHDIST * 2 + 1)];
	static int s_anQueue[(TAILCHASE_PATHDIST * 2 + 1) * (TAILCHASE_PATHDIST * 2 + 1)];
	int nSX, nSY, nTX, nTY, nOX, nOY, nHead, nTail, nPos, nDir, nNext, x, y, i;
	PMAPINFO pMap;

	nSX = (int)pChar->dwX;
	nSY = (int)pChar->dwY;
	nTX = (int)pTarget->dwX;
	nTY = (int)pTarget->dwY;
	if (pnDist) {
		*pnDist = 0;
	}
	if (abs (nTX - nSX) + abs (nTY - nSY) <= 1) {
		return -2;
	}
	if ((pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) || (abs (nTX - nSX) > nR) || (abs (nTY - nSY) > nR)) {
		return -1;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	nOX = nSX - nR;
	nOY = nSY - nR;
	for (i = 0; i < nW * nW; i ++) {
		s_anFirst[i] = -1;
		s_anDist[i] = 0;
	}
	nHead = nTail = 0;
	nPos = nR * nW + nR;
	s_anFirst[nPos] = 4;
	s_anQueue[nTail ++] = nPos;
	while (nHead < nTail) {
		nPos = s_anQueue[nHead ++];
		x = nOX + (nPos % nW);
		y = nOY + (nPos / nW);
		/* 相手のとなりのマスに着いた */
		if ((s_anFirst[nPos] != 4) && (abs (x - nTX) + abs (y - nTY) == 1)) {
			if (pnDist) {
				*pnDist = s_anDist[nPos];
			}
			return s_anFirst[nPos];
		}
		for (nDir = 0; nDir < 4; nDir ++) {
			int nNX = x + anPosAddTblX[nDir];
			int nNY = y + anPosAddTblY[nDir];
			if ((nNX < nOX) || (nNX >= nOX + nW) || (nNY < nOY) || (nNY >= nOY + nW)) {
				continue;
			}
			nNext = (nNY - nOY) * nW + (nNX - nOX);
			if (s_anFirst[nNext] != -1) {
				continue;
			}
			if (TailChaseCanStep (pChar, pMap, nNX, nNY) == FALSE) {
				/* 何度も調べ直さないよう、行けないマスとして覚える */
				s_anFirst[nNext] = -4;
				continue;
			}
			s_anFirst[nNext] = (s_anFirst[nPos] == 4) ? (short)nDir : s_anFirst[nPos];
			s_anDist[nNext] = (short)(s_anDist[nPos] + 1);
			s_anQueue[nTail ++] = nNext;
		}
	}
	return -1;
}


/* ========================================================================= */
/* 関数名：	TailChaseIsActive												 */
/* 内容：	列に戻る途中(追いかけ中・消えている)か							 */
/* ========================================================================= */

BOOL TailChaseIsActive(PCHARINFO pChar)
{
	if (s_mapTailChase.empty () || (pChar == NULL)) {
		return FALSE;
	}
	return (s_mapTailChase.find (pChar->dwTblID) != s_mapTailChase.end ());
}


/* ========================================================================= */
/* 関数名：	TailReturnHeadOf												 */
/* 内容：	その人が列を離れて戻る途中なら、戻り先の先頭(テーブルID)を返す	 */
/*			(離れている・追いかけ中のどちらでもない時は -1)					 */
/* ========================================================================= */

static DWORD TailReturnHeadOf(PCHARINFO pChar)
{
	std::map<DWORD, TAILRETURN>::iterator it = s_mapTailReturn.find (pChar->dwTblID);
	if ((it != s_mapTailReturn.end ()) && (it->second.dwPID == pChar->pID)) {
		return it->second.dwHeadTblID;
	}
	std::map<DWORD, TAILCHASE>::iterator itChase = s_mapTailChase.find (pChar->dwTblID);
	if ((itChase != s_mapTailChase.end ()) && (itChase->second.Info.dwPID == pChar->pID)) {
		return itChase->second.Info.dwHeadTblID;
	}
	return (DWORD)-1;
}


/* ========================================================================= */
/* 関数名：	TailIsScattered													 */
/* 内容：	その人の列が戦闘でばらけている途中か							 */
/*			(その人自身が列を離れている・追いかけ中、または同じ列の先頭を	 */
/*			戻り先に覚えている人がいる)。ばらけている間にほかの人が入ると、	 */
/*			戻る人が戻れなくなったり順番が変わったりするので入れないようにする	 */
/*			dwExceptHead: この先頭の列なら除く(元の列の人が自分で入り直す時)	 */
/* ========================================================================= */

static BOOL TailIsScattered(PCHARINFO pTarget, DWORD dwExceptHead)
{
	int i;
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PCHARINFO pHead;

	if (s_mapTailReturn.empty () && s_mapTailChase.empty ()) {
		return FALSE;
	}
	/* 相手自身が列を離れている・戻る途中 */
	if (TailReturnHeadOf (pTarget) != (DWORD)-1) {
		return TRUE;
	}
	/* 相手の列の先頭をたどる */
	pHead = pTarget;
	for (i = 0; i < 100; i ++) {
		if ((pHead->dwTailTblID == (DWORD)-1) || (pHead->dwTailTblID >= dwCount)) {
			break;
		}
		pHead = g_aCharInfo[pHead->dwTailTblID];
	}
	if (pHead->dwTblID == dwExceptHead) {
		return FALSE;
	}
	/* その先頭を戻り先に覚えている人がいる？ */
	std::map<DWORD, TAILRETURN>::iterator it;
	for (it = s_mapTailReturn.begin (); it != s_mapTailReturn.end (); ++ it) {
		if ((it->second.dwHeadTblID == pHead->dwTblID) && (it->second.dwHeadPID == pHead->pID)) {
			return TRUE;
		}
	}
	std::map<DWORD, TAILCHASE>::iterator itChase;
	for (itChase = s_mapTailChase.begin (); itChase != s_mapTailChase.end (); ++ itChase) {
		if ((itChase->second.Info.dwHeadTblID == pHead->dwTblID) && (itChase->second.Info.dwHeadPID == pHead->pID)) {
			return TRUE;
		}
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	TailChaseCancel													 */
/* 内容：	追いかけをやめる(本人のクライアントを動けるように戻す)			 */
/*			bRestore=TRUE で、消えている最中なら本人もほかの人も元に戻す		 */
/*			(マップ移動・ログアウトの時は FALSE。pszMsg があれば知らせる)	 */
/* ========================================================================= */

static void TailChaseCancel(PCHARINFO pChar, LPTSTR pszMsg, BOOL bRestore)
{
	CCHAR_TAIL Packet;
	BOOL bWarp;
	DWORD dwPID;

	std::map<DWORD, TAILCHASE>::iterator it = s_mapTailChase.find (pChar->dwTblID);
	if (it == s_mapTailChase.end ()) {
		return;
	}
	bWarp	= (it->second.dwWarpTime != 0);
	dwPID	= it->second.Info.dwPID;
	s_mapTailChase.erase (it);
	if ((pChar->pID == 0) || (pChar->pID != dwPID)) {
		return;
	}
	if (bWarp && bRestore) {
		SendCmdFADEINOUT (pChar->pID, 1);
		SendCmdCHARINFO3 (pChar->dwTblID);
	}
	Packet.Make (pChar->dwTblID, (DWORD)-1, CHAR_TAIL_CHASEEND);
	svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
	if (pszMsg) {
		SendCmdSYSMSG (pChar->pID, pszMsg, SPEECHCMD_BATTLE1);
	}
}


/* ========================================================================= */
/* 関数名：	TailChaseSendTarget												 */
/* 内容：	本人に「○○を追いかけています。」を知らせる(動けなくもする)		 */
/* ========================================================================= */

static void TailChaseSendTarget(PCHARINFO pChar, TAILCHASE &Chase, PCHARINFO pTarget)
{
	CCHAR_TAIL Packet;

	Chase.dwPrevTblID = pTarget->dwTblID;
	Packet.Make (pChar->dwTblID, pTarget->dwTblID, CHAR_TAIL_CHASE);
	svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
}


/* ========================================================================= */
/* 関数名：	TailChaseLink													 */
/* 内容：	最後尾につなぐ(TailOn と同じつなぎ方。「列に戻りました。」を出す)	 */
/*			追いかけ中の記録は消す											 */
/* ========================================================================= */

static void TailChaseLink(PCHARINFO pChar, PCHARINFO pEnd)
{
	CCHAR_TAIL Packet;

	s_mapTailChase.erase (pChar->dwTblID);
	/* 追いかけている間に後ろについた傭兵は外す（列の最後へ入り直す） */
	MercenaryTailOffBack (pChar);
	pChar->dwTailTblID		= pEnd->dwTblID;
	pEnd->dwTailBackTblID	= pChar->dwTblID;
	Packet.Make (pChar->dwTblID, pEnd->dwTblID, CHAR_TAIL_RETURN);
	svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
	if (pEnd->pID != 0) {
		Packet.Make (pEnd->dwTblID, pChar->dwTblID, CHAR_TAIL_BACK);
		svrSock.SendTo (pEnd->pID, Packet.Get (), Packet.GetSize ());
	}
}


/* ========================================================================= */
/* 関数名：	TailChaseWarpStart												 */
/* 内容：	ワープで戻る：まず本人の画面を暗くし、ほかの人の画面ではふっと消す	 */
/*			傭兵の WarpNearOwner と同じく、向き99の座標通知で消し始める。	 */
/*			TAILCHASE_FADETIME たってから TailChaseWarpFinish で現れる		 */
/*			(消えている間もサーバーの位置はそのまま。となりに空きが無ければ	 */
/*			始めずに FALSE)													 */
/* ========================================================================= */

static BOOL TailChaseWarpStart(PCHARINFO pChar, TAILCHASE &Chase, PCHARINFO pTarget)
{
	int x, y;
	BYTE byDirection;

	if (!FindPosNearChar (pChar, pTarget, x, y)) {
		return FALSE;
	}
	SendCmdFADEINOUT (pChar->pID, 0);
	byDirection = pChar->byDirection;
	pChar->byDirection = 99;
	SendCmdCHARPOS (0, pChar->dwTblID, 0, 1);
	pChar->byDirection = byDirection;
	Chase.dwWarpTime = g_dwTime ? g_dwTime : 1;
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	TailChaseWarpFinish												 */
/* 内容：	消えていた人を、前の人のとなりに出す(フェードイン)				 */
/*			向きは前の人に合わせ、向きを決めてから位置を送る				 */
/*			前の人が最後尾なら、そのまま列につなぐ。まだ追いかけ中の人なら	 */
/*			となりで待つ(その人が先にワープ中なら、その人が出るまで消えたまま) */
/* ========================================================================= */

static void TailChaseWarpFinish(PCHARINFO pChar, TAILCHASE &Chase, PCHARINFO pTarget, BOOL bPending)
{
	int x, y;

	if (bPending) {
		std::map<DWORD, TAILCHASE>::iterator it = s_mapTailChase.find (pTarget->dwTblID);
		if ((it != s_mapTailChase.end ()) && it->second.dwWarpTime) {
			return;
		}
	}
	if (!FindPosNearChar (pChar, pTarget, x, y)) {
		TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
		return;
	}
	SubProcMoveEx (pChar, (DWORD)x, (DWORD)y, (pTarget->byDirection < 4) ? pTarget->byDirection : (BYTE)1, FALSE);
	Chase.dwWarpTime = 0;
	if (!bPending) {
		TailChaseLink (pChar, pTarget);
		return;
	}
	/* 位置の通知で本人の「追いかけ中」が解けるので、知らせ直す */
	Chase.dwStart = Chase.dwProgress = g_dwTime;
	Chase.nBestDist = 0x7FFFFFFF;
	TailChaseSendTarget (pChar, Chase, pTarget);
}


/* ========================================================================= */
/* 関数名：	TailChaseProc													 */
/* 内容：	列に戻る途中のプレイヤーを歩かせる(タイマー処理から毎回呼ぶ)		 */
/*			・となりに着いたら列につなぐ(前の人がまだ追いかけ中なら待つ)		 */
/*			・歩くのは TAILCHASE_STEPTIME ごとに1マス						 */
/*			・画面の外・道が無い・時間がかかる・近づけない時はワープする		 */
/* ========================================================================= */

void TailChaseProc(PCHARINFO pChar)
{
	DWORD dwNow = g_dwTime;
	PCHARINFO pTarget = NULL;
	BOOL bPending = FALSE;
	int nDX, nDY, nDist, nDir, nPathDist;

	if (s_mapTailChase.empty ()) {
		return;
	}
	std::map<DWORD, TAILCHASE>::iterator it = s_mapTailChase.find (pChar->dwTblID);
	if (it == s_mapTailChase.end ()) {
		return;
	}
	TAILCHASE &Chase = it->second;

	/* 別の人になった・ほかの列につながった */
	if ((pChar->pID == 0) || (pChar->pID != Chase.Info.dwPID) || (pChar->dwTailTblID != (DWORD)-1)) {
		s_mapTailChase.erase (it);
		return;
	}
	/* 倒れた */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		TailChaseCancel (pChar, NULL, TRUE);
		return;
	}
	/* 先頭がいなくなった */
	if (!TailChaseTarget (pChar, Chase.Info, pTarget, bPending)) {
		TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
		return;
	}
	/* 時間がかかりすぎた */
	if ((DWORD)(dwNow - Chase.dwOrigStart) >= TAILCHASE_GIVEUP) {
		TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
		return;
	}

	/* 消えている最中。時間が来たら前の人のとなりに出る */
	if (Chase.dwWarpTime) {
		if ((DWORD)(dwNow - Chase.dwWarpTime) >= TAILCHASE_FADETIME) {
			TailChaseWarpFinish (pChar, Chase, pTarget, bPending);
		}
		return;
	}

	/* 追いかける相手が変わった */
	if (Chase.dwPrevTblID != pTarget->dwTblID) {
		TailChaseSendTarget (pChar, Chase, pTarget);
	}

	nDX		= (int)pTarget->dwX - (int)pChar->dwX;
	nDY		= (int)pTarget->dwY - (int)pChar->dwY;
	nDist	= abs (nDX) + abs (nDY);
	/* となりに着いた（同じマスに重なっている時も、そのままつなぐ。
	   重なったまま道さがしをすると行き先が無く、ワープで空いた所へ飛ばされていた） */
	if (nDist <= 1) {
		if (!bPending) {
			TailChaseLink (pChar, pTarget);
			return;
		}
		/* 前の人がつながるまで待つ(待っている間は時間を数えない) */
		Chase.dwStart = Chase.dwProgress = dwNow;
		return;
	}
	/* 画面の外(クライアントの表示範囲の外)ならワープ */
	if ((abs (nDX) > DRAW_PARTS_X) || (abs (nDY) > DRAW_PARTS_Y)) {
		if (!TailChaseWarpStart (pChar, Chase, pTarget)) {
			TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
		}
		return;
	}
	/* 前の人を待つ間は時間切れにしない。そうでなければ、時間がかかったらワープ */
	if (bPending) {
		Chase.dwStart = Chase.dwProgress = dwNow;
	} else if (((DWORD)(dwNow - Chase.dwStart) >= TAILCHASE_LIMIT) || ((DWORD)(dwNow - Chase.dwProgress) >= TAILCHASE_STUCK)) {
		if (!TailChaseWarpStart (pChar, Chase, pTarget)) {
			TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
		}
		return;
	}

	/* 歩く間隔 */
	if ((DWORD)(dwNow - Chase.dwLastStep) < TAILCHASE_STEPTIME) {
		return;
	}
	nPathDist = 0;
	nDir = TailChaseFindDir (pChar, pTarget, &nPathDist);
	if (nDir < 0) {
		/* 道が無い(前の人を待っている間は、そのまま待つ) */
		if (!bPending) {
			if (!TailChaseWarpStart (pChar, Chase, pTarget)) {
				TailChaseCancel (pChar, "列に戻れませんでした。", TRUE);
			}
		}
		return;
	}
	if (nPathDist < Chase.nBestDist) {
		Chase.nBestDist = nPathDist;
		Chase.dwProgress = dwNow;
	}

	/* 1マス歩く(TailMoveChain がついてくる人を動かす時と同じ送り方) */
	int x = (int)pChar->dwX + anPosAddTblX[nDir];
	int y = (int)pChar->dwY + anPosAddTblY[nDir];
	RenewCharBridgeLayerSvr (pChar, x, y, (BYTE)nDir);
	pChar->byDirectionBack2	= pChar->byDirectionBack;
	pChar->byDirectionBack	= pChar->byDirection;
	pChar->dwXBack			= pChar->dwX;
	pChar->dwYBack			= pChar->dwY;
	pChar->byDirection		= (BYTE)nDir;
	pChar->dwX				= (DWORD)x;
	pChar->dwY				= (DWORD)y;
	pChar->dwLastDotMove	= dwNow;
	pChar->dwTimeStep		= dwNow;
	Chase.dwLastStep		= dwNow;
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 1);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	CheckCharPosSend (pChar);
	pChar->byFlg &= ~CHARINFOFLG_DIRECTION;
	pChar->byFlg &= ~CHARINFOFLG_MOVE;
	/* 後ろに誰かがついてきていれば、その人も動かす */
	TailMoveChain (pChar);
}


/* ========================================================================= */
/* 関数名：	TailLeave														 */
/* 内容：	列から一時的に抜ける(戻り先の先頭と並び順を覚える)				 */
/*			後ろの人は前の人へ直接つなぎ直す(傭兵が抜ける時と同じ)			 */
/* ========================================================================= */

static void TailLeave(PCHARINFO pChar, PCHARINFO pHead, int nOrder)
{
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PCHARINFO pPrev, pNext;
	CCHAR_TAIL Packet;
	TAILRETURN Info;

	ZeroMemory (&Info, sizeof (Info));
	Info.dwPID			= pChar->pID;
	Info.dwHeadTblID	= pHead->dwTblID;
	Info.dwHeadPID		= pHead->pID;
	Info.nOrder			= nOrder;
	lstrcpyn (Info.szHeadName, pHead->szName, sizeof (Info.szHeadName));

	pPrev = g_aCharInfo[pChar->dwTailTblID];
	pNext = NULL;
	if ((pChar->dwTailBackTblID < dwCount) && (g_aCharInfo[pChar->dwTailBackTblID]->dwTailTblID == pChar->dwTblID)) {
		pNext = g_aCharInfo[pChar->dwTailBackTblID];
	}
	if (pPrev->dwTailBackTblID == pChar->dwTblID) {
		pPrev->dwTailBackTblID = pNext ? pNext->dwTblID : (DWORD)-1;
	}
	if (pNext) {
		pNext->dwTailTblID = pPrev->dwTblID;
	}
	pChar->dwTailTblID		= (DWORD)-1;
	pChar->dwTailBackTblID	= (DWORD)-1;
	s_mapTailReturn[pChar->dwTblID] = Info;

	/* 本人へ知らせる */
	Packet.Make (pChar->dwTblID, (DWORD)-1, CHAR_TAIL_BATTLE);
	svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
	/* 前の人の画面は、後ろをつなぎ直す(後ろがいなければ空にする。メッセージは出ない) */
	if (pPrev->pID != 0) {
		if (pNext) {
			Packet.Make (pPrev->dwTblID, pNext->dwTblID, CHAR_TAIL_BACK);
		} else {
			Packet.Make (pPrev->dwTblID, (DWORD)-1, CHAR_TAIL_BACKOFF);
		}
		svrSock.SendTo (pPrev->pID, Packet.Get (), Packet.GetSize ());
	}
	/* 後ろの人の画面は、前の人へつなぎ直す(つなぎ直しはメッセージ無し) */
	if (pNext && (pNext->pID != 0)) {
		Packet.Make (pNext->dwTblID, pPrev->dwTblID, CHAR_TAIL_ON);
		svrSock.SendTo (pNext->pID, Packet.Get (), Packet.GetSize ());
	}
}


/* ========================================================================= */
/* 関数名：	TailReturnDo													 */
/* 内容：	覚えた先頭の列に戻る											 */
/*			前の人(TailChaseTarget)のとなりにいればそのままつなぐ。			 */
/*			画面内に見えていれば、歩いて追いかける(「○○を追いかけています。」)。*/
/*			画面外・道が無い時は、フェードを挟んでとなりへワープする。		 */
/*			戻れなければ「列に戻れませんでした。」。覚えは呼び出し側で消す済み	 */
/* ========================================================================= */

static void TailReturnDo(PCHARINFO pChar, const TAILRETURN &Info)
{
	PCHARINFO pTarget = NULL;
	BOOL bPending = FALSE, bTarget = FALSE;
	TAILCHASE Chase;
	int nDX, nDY, nDist;

	/* 列を離れている間に自分の傭兵が後ろについてきていたら、先に外す
	   （残っていると「後ろに人がいる」扱いで戻れなかった。傭兵は戻った後に列の最後へ入り直す） */
	MercenaryTailOffBack (pChar);
	if ((pChar->dwTailTblID == (DWORD)-1) && (pChar->dwTailBackTblID == (DWORD)-1) &&
		(pChar->byCharState != CHARSTATE_DEAD) && (pChar->wHP != 0) && !TailChaseIsActive (pChar)) {
		bTarget = TailChaseTarget (pChar, Info, pTarget, bPending);
	}
	if (!bTarget) {
		SendCmdSYSMSG (pChar->pID, "列に戻れませんでした。", SPEECHCMD_BATTLE1);
		return;
	}

	nDX		= (int)pTarget->dwX - (int)pChar->dwX;
	nDY		= (int)pTarget->dwY - (int)pChar->dwY;
	nDist	= abs (nDX) + abs (nDY);
	/* 最後尾のとなり（か同じマス）にいれば、すぐ列につなぐ */
	if ((nDist <= 1) && !bPending) {
		TailChaseLink (pChar, pTarget);
		return;
	}

	ZeroMemory (&Chase, sizeof (Chase));
	Chase.Info			= Info;
	Chase.dwOrigStart	= Chase.dwStart = Chase.dwProgress = g_dwTime;
	Chase.dwLastStep	= g_dwTime;
	Chase.dwPrevTblID	= (DWORD)-1;
	Chase.nBestDist		= 0x7FFFFFFF;
	s_mapTailChase[pChar->dwTblID] = Chase;
	TAILCHASE &Ref = s_mapTailChase[pChar->dwTblID];

	/* 前の人のとなり（か同じマス）で、その人がつながるのを待つ */
	if (nDist <= 1) {
		TailChaseSendTarget (pChar, Ref, pTarget);
		return;
	}
	/* 画面内に見えていて、歩いて行ける道があれば追いかける */
	if ((abs (nDX) <= DRAW_PARTS_X) && (abs (nDY) <= DRAW_PARTS_Y)) {
		int nPathDist = 0;
		if (TailChaseFindDir (pChar, pTarget, &nPathDist) >= 0) {
			Ref.nBestDist = nPathDist;
			TailChaseSendTarget (pChar, Ref, pTarget);
			return;
		}
	}
	/* 画面の外・道が無い：フェードを挟んでワープ */
	if (!TailChaseWarpStart (pChar, Ref, pTarget)) {
		s_mapTailChase.erase (pChar->dwTblID);
		SendCmdSYSMSG (pChar->pID, "列に戻れませんでした。", SPEECHCMD_BATTLE1);
	}
}


/* ========================================================================= */
/* 関数名：	TailBattleMode													 */
/* 内容：	戦闘モードの切り替えに合わせて、列から一時的に抜ける・戻る		 */
/*			傭兵・NPCは対象外(傭兵は列に残り、自分で動く)					 */
/*			先頭がON=列の全員が抜ける。先頭がOFF=覚えた人が元の順で戻る		 */
/*			後ろの人がON=自分だけ抜ける。OFF=先頭が戦闘モード中なら待つ		 */
/*			(RecvDataProcBATTLEMODE で MercenarySetBattleMode の後に呼ぶ)	 */
/* ========================================================================= */

void TailBattleMode(PCHARINFO pChar, BOOL bBattle)
{
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();
	PCHARINFO pHead, pCur, pNext;
	TAILRETURN Info;
	int i;

	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT) || IsMercenary (pChar)) {
		return;
	}

	/* 列に戻る途中(追いかけ中)で戦闘モードにした：追いかけをやめて、また列から離れた状態(戻り先を覚えた状態)に戻る */
	if (bBattle && TailChaseIsActive (pChar)) {
		CCHAR_TAIL PacketChase;
		Info = s_mapTailChase[pChar->dwTblID].Info;
		TailChaseCancel (pChar, NULL, TRUE);
		s_mapTailReturn[pChar->dwTblID] = Info;
		PacketChase.Make (pChar->dwTblID, (DWORD)-1, CHAR_TAIL_BATTLE);
		svrSock.SendTo (pChar->pID, PacketChase.Get (), PacketChase.GetSize ());
		return;
	}

	/* 列の先頭(先頭自身は列には入っていない)。ONは後ろに人がいる時だけ。
	   OFFは、全員が抜けて後ろが空になっていても、覚えている人を戻す */
	if ((pChar->dwTailTblID == (DWORD)-1) && (!bBattle || (pChar->dwTailBackTblID != (DWORD)-1))) {
		if (bBattle) {
			/* 並んでいる順に、傭兵以外のプレイヤーを集めてから全員抜ける */
			std::vector<PCHARINFO> apMember;
			std::vector<int> anOrder;
			pCur = pChar;
			for (i = 1; i <= 100; i ++) {
				if (pCur->dwTailBackTblID >= dwCount) {
					break;
				}
				pNext = g_aCharInfo[pCur->dwTailBackTblID];
				if ((pNext == pChar) || (pNext->dwTailTblID != pCur->dwTblID)) {
					break;
				}
				if ((pNext->pID != 0) && (pNext->byNpcType == NPCTYPE_INIT) && !IsMercenary (pNext)) {
					apMember.push_back (pNext);
					anOrder.push_back (i);
				}
				pCur = pNext;
			}
			for (size_t j = 0; j < apMember.size (); j ++) {
				TailLeave (apMember[j], pChar, anOrder[j]);
			}
			return;
		}

		/* 戦闘モード解除：この先頭を覚えている人を、覚えた順で戻す */
		std::multimap<int, DWORD> mapOrder;
		std::map<DWORD, TAILRETURN>::iterator it;
		for (it = s_mapTailReturn.begin (); it != s_mapTailReturn.end (); ++ it) {
			if ((it->second.dwHeadTblID == pChar->dwTblID) && (it->second.dwHeadPID == pChar->pID)) {
				mapOrder.insert (std::make_pair (it->second.nOrder, it->first));
			}
		}
		std::multimap<int, DWORD>::iterator itOrder;
		for (itOrder = mapOrder.begin (); itOrder != mapOrder.end (); ++ itOrder) {
			DWORD dwMember = itOrder->second;
			it = s_mapTailReturn.find (dwMember);
			if ((it == s_mapTailReturn.end ()) || (dwMember >= dwCount)) {
				continue;
			}
			PCHARINFO pMember = g_aCharInfo[dwMember];
			Info = it->second;
			if (Info.dwPID != pMember->pID) {
				/* 別の人になっている */
				s_mapTailReturn.erase (it);
				continue;
			}
			/* 自分で戦闘モード中の人は戻さず、覚えたまま待つ */
			if (IsCharBattleMode (pMember)) {
				continue;
			}
			s_mapTailReturn.erase (it);
			TailReturnDo (pMember, Info);
		}
		/* 先頭自身も覚えがあれば、下の戻る処理へ進む */
	}

	if (bBattle) {
		if ((pChar->dwTailTblID == (DWORD)-1) || (pChar->dwTailTblID >= dwCount)) {
			return;
		}
		/* 列の先頭をたどりながら、先頭から何番目かを数える */
		int nOrder = 1;
		pHead = pChar;
		for (i = 0; i < 100; i ++) {
			if ((pHead->dwTailTblID == (DWORD)-1) || (pHead->dwTailTblID >= dwCount)) {
				break;
			}
			pHead = g_aCharInfo[pHead->dwTailTblID];
			if (pHead == pChar) {
				return;
			}
			if (pHead->dwTailTblID != (DWORD)-1) {
				nOrder ++;
			}
		}
		if (pHead == pChar) {
			return;
		}
		TailLeave (pChar, pHead, nOrder);
		return;
	}

	/* 戦闘モード解除 */
	std::map<DWORD, TAILRETURN>::iterator it = s_mapTailReturn.find (pChar->dwTblID);
	if (it == s_mapTailReturn.end ()) {
		return;
	}
	Info = it->second;
	if (Info.dwPID != pChar->pID) {
		s_mapTailReturn.erase (it);
		return;
	}
	/* 覚えた先頭が今も戦闘モード中なら、戻さずに覚えたまま待つ(先頭の解除でまとめて戻る) */
	if ((Info.dwHeadTblID < dwCount) && (Info.dwHeadTblID != pChar->dwTblID)) {
		pHead = g_aCharInfo[Info.dwHeadTblID];
		if ((pHead->pID != 0) && (pHead->pID == Info.dwHeadPID) && IsCharBattleMode (pHead)) {
			return;
		}
	}
	/* どちらにしても、覚えた先頭はここで消す */
	s_mapTailReturn.erase (it);
	TailReturnDo (pChar, Info);
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
    if (!IsRecvSizeOK(sizeof(CMDPARA_DATA4), 0)) return;
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
    if (pChar->waEqu[0] < MAX_MYITEM && ItemIDValid (pChar->waItem[pChar->waEqu[0]])) {
        PITEMINFO_WEAPON bow = BowWeapon(pChar, g_aItemInfo[pChar->waItem[pChar->waEqu[0]]]);
        if (bow) {
            if (pPara->dwData >= (DWORD)g_aCharInfo.GetSize()) return;
            pCharTarget = g_aCharInfo[pPara->dwData];
            if (!BowEnemy(pCharTarget) || !IsInteractable(pChar, pCharTarget) ||
                pCharTarget->dwMapID != pChar->dwMapID || GetMapNo(pChar->dwMapID) < 0 ||
                !(g_aMapInfo[pChar->dwMapID]->byFlg & MAPFLG_BATTLE)) return;
            BYTE rate = GetAtackRateReq(pChar, pPara->byData);
            if (!BowFacingTarget(pChar, pCharTarget) || !BowInLineRange(pChar, pCharTarget, bow->byData1) ||
                !IsBowReachChar(pChar, pCharTarget)) return;
            pChar->byAtackRate = rate;
            pChar->dwTargetTblID = pPara->dwData;
            pChar->dwLastBattle = 1;
            return;
        }
    }
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
		(pCharTarget->byNpcType == NPCTYPE_BATTLE2) ||
		(pCharTarget->byNpcType == NPCTYPE_MIMIC))) {
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
	/* 押す向きは、押した人と押されるキャラの位置関係で決める。
	   歩きながら曲がってそのまま押した時は、向きの変更がサーバーに届く前に
	   押して要求が来るので、サーバーの向きが前の向きのまま残っている。
	   (下から上へ押して壁に当てた後、横から押すと前の「上」で押して
	    壁で跳ね返り、下へ動いていた)
	   大きいキャラは左端の座標で、右へ 1～2 マス広がっている */
	{
		int nDX = (int)pCharPush->dwX - (int)pChar->dwX;
		int nDY = (int)pCharPush->dwY - (int)pChar->dwY;
		BYTE byDirection = pChar->byDirection;
		if ((nDY == 0) && (nDX != 0)) {
			byDirection = (nDX < 0) ? 2 : 3;
		} else if ((nDX <= 0) && (nDX >= -2) && (nDY != 0)) {
			byDirection = (nDY < 0) ? 0 : 1;
		}
		if (byDirection >= 4) {
			return;
		}
		/* 押した人もその向きを向かせる(画面上も押した方を向く) */
		if (pChar->byDirection != byDirection) {
			PinCharBridgeLayerSvr (pChar);
			pChar->byDirection = byDirection;
			pChar->byFlg |= CHARINFOFLG_DIRECTION;
		}
	}
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
			DWORD dwTmp = CheckCharBlockNPC (pCharPush->dwMapID, dwX, dwY, pCharPush->dwSpaceID, pCharPush);
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
	/* 何かに話しかけたら、会話データの「はい・いいえ」の保留中の問いは消える（別の NPC に話した時） */
	if (s_nCharNoRecv >= 0) {
		TalkPendClear ((DWORD)s_nCharNoRecv);
	}
	/* 別の空間にいるキャラ・印で見えないNPCとは話せない */
	if ((s_nCharNoRecv >= 0) && (!IsSameSpace (g_aCharInfo[s_nCharNoRecv], pChar) || !IsCharSeenBy (pChar, g_aCharInfo[s_nCharNoRecv]))) {
		return;
	}
	/* 宝箱の敵（正体前）は話しても何も起きない（調べると正体を現す） */
	if (pChar->byNpcType == NPCTYPE_MIMIC) {
		return;
	}
	/* 依頼の見えるNPC（台詞が「@q」。受けた人・仲間だけに見える）？ 話すとポータル・宝箱など */
	if (IsQuestNpc (pChar)) {
		if ((s_nCharNoRecv >= 0) && IsNearRecvChar (pChar, 2)) {
			QuestNpcTalk (g_aCharInfo[s_nCharNoRecv], pChar);
		}
		return;
	}
	/* 会話データのNPC（台詞が「@tNNN」）？ 話しかけた本人のフラグとレベルで行を選び、本人にだけ台詞を送る */
	if (TalkIsNpc (pChar)) {
		if ((s_nCharNoRecv >= 0) && IsNearRecvChar (pChar, 2)) {
			TalkNpcTalk (g_aCharInfo[s_nCharNoRecv], pChar);
		}
		return;
	}
	/* ギルドのNPC（台詞が「@gNN」）？ 依頼の一覧を送る */
	if (QuestIsGuildNpc (pChar)) {
		if ((s_nCharNoRecv >= 0) && IsNearRecvChar (pChar, 2)) {
			QuestGuildTalk (g_aCharInfo[s_nCharNoRecv], pChar);
		}
		return;
	}
	/* 語り部のNPC（台詞が「@r」）？ 思い出の本の一覧を送る */
	if (StoryIsNpc (pChar)) {
		if ((s_nCharNoRecv >= 0) && IsNearRecvChar (pChar, 2)) {
			StoryTalk (g_aCharInfo[s_nCharNoRecv], pChar);
		}
		return;
	}
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

	/* メッセージの「@4」でもらえるものだけ（クライアントが勝手に頼んできたものは渡さない） */
	if ((pPara->dwData1 == 0) || (pPara->dwData1 >= (DWORD)g_aItemInfo.GetSize ())) {
		return;
	}
	/* もう持てない？（大事な物は大事な物の欄の空きを見る） */
	if (!CanAddCharItem (pPara->dwTblID, pPara->dwData1)) {
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
	if ((pPara->byData >= MAX_MYITEM) || (pPara->dwData2 > 0xFFFF) || !ItemIDValid ((WORD)pPara->dwData2)) {
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
		/* 依頼の品は売れない・捨てられない（持ち物を送り直して元に戻す） */
		if (QuestIsQuestItem ((WORD)pPara->dwData2)) {
			SendCmdSYSMSG (pChar->pID, "依頼の品は、売ったり捨てたりできません。", SPEECHCMD_BATTLE1);
			SendCmdCHARITEM (pChar->pID, pPara->dwTblID);
			return;
		}

		/* 売った？（売値はクライアントの申告ではなく、クライアントと同じ式でサーバーが決める） */
		if (pPara->dwData1 > 0) {
			DWORD dwSell = max (ItemGoldOf ((WORD)pPara->dwData2) / 4, (DWORD)1);
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
	/* 管理者が足した手持ちの大事な物は、大事な物の欄へ移す（欄そのものは送られた値を使わない） */
	MoveImportantItems (pChar);
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
/* 関数名：IsSubGameItemMagicLocked											 */
/* 内容：ミニゲームに参加中で、道具・装備の持ち替え・特技を受け付けない状態か */
/*       （ショートカットの 1〜5 キー・スマホのボタンから送られても弾く。	 */
/*        戦場ゲーム・すごろくは参加中ずっと、爆弾ゲームは試合中だけ。		 */
/*        爆弾ゲームの試合前はふだんのコマンドで道具を使えるので今まで通り）	 */
/* ========================================================================= */
static BOOL IsSubGameItemMagicLocked(PCHARINFO pChar)
{
	if (pChar->byNpcType != NPCTYPE_INIT) {
		return FALSE;
	}
	switch (pChar->abySubStatus[0]) {
	case SUBGAMEID_WAR:
	case SUBGAMEID_SUGOROKU:
		return TRUE;
	case SUBGAMEID_BOMB:
		return (((PSUBSTATUS_BOMB)pChar->abySubStatus)->byFlgEntry) ? TRUE : FALSE;
	}
	return FALSE;
}

/* ========================================================================= */
void RecvDataProcUSEITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA4 pPara = (PCMDPARA_DATA4)pData;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	/* ミニゲームの最中は道具を使えない（ショートカットから送られても弾く） */
	if (IsSubGameItemMagicLocked (g_aCharInfo[pPara->dwTblID])) {
		return;
	}

	/* 大事な物の欄の品を使う（読み物だけ。欄に本当にあるかは ItemProcImportant が確かめる） */
	if (pPara->byData >= IMPORTANT_USEBASE) {
		ItemProcImportant (pPara->dwTblID, (BYTE)(pPara->byData - IMPORTANT_USEBASE), pPara->dwData);
		return;
	}

	/* 持ち物の番号・使う相手の番号が範囲内で、そこにアイテムがある？ */
	if ((pPara->byData >= MAX_MYITEM) || (pPara->dwData >= (DWORD)g_aCharInfo.GetSize ())) {
		return;
	}
	WORD wItem = g_aCharInfo[pPara->dwTblID]->waItem[pPara->byData];
	if ((wItem == 0) || !ItemIDValid (wItem)) {
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
		/* 依頼の見えるNPC・別の空間にいるキャラはターゲットにできない */
		if (!IsInteractable (pChar, pCharTmp)) {
			return;
		}

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
				(pCharTmp->byNpcType == NPCTYPE_BATTLE2) ||
				(pCharTmp->byNpcType == NPCTYPE_MIMIC)
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
	/* ミニゲームの最中は装備を持ち替えない（ショートカットから送られても弾く） */
	if (IsSubGameItemMagicLocked (pChar)) {
		return;
	}
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
		pChar->byGhost		= pInfo->byGhost;
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
/* 関数名：QuestChallengeBlock												 */
/* 内容：討伐チャレンジ中ならお知らせして TRUE（預かり所・取り引きなどを止める）	 */
/* 日付：2026/11/12															 */
/* ========================================================================= */
static BOOL QuestChallengeBlock(PCHARINFO pChar)
{
	if (!QuestInChallenge (pChar)) {
		return FALSE;
	}
	SendCmdSYSMSG (pChar->pID, "討伐チャレンジ中はできません。", SPEECHCMD_BATTLE1);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEITEM											 */
/* 内容：受信データ処理(アイテム預ける)										 */
/* 日付：2003/11/07															 */
/* ========================================================================= */
static void SendLeaveItemResult(PCHARINFO pChar, BYTE byResult);
void RecvDataProcLEAVEITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;

	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}

	pChar = g_aCharInfo[pPara->dwTblID];
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
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

	/* 依頼の品は預けられない */
	if (QuestIsQuestItem (pChar->waItem[pPara->byData])) {
		SendLeaveItemResult (pChar, LEAVEITEMRES_QUEST);
		return;
	}
	/* 預かり所に入れる（いっぱいなら断る） */
	if (!AddLeaveItem (pChar, pChar->waItem[pPara->byData])) {
		SendLeaveItemResult (pChar, LEAVEITEMRES_FULL);
		return;
	}
	DelCharItem (pPara->dwTblID, pPara->byData);
	SendCmdCHARITEM (s_pIDRecv, pPara->dwTblID);
	SendLeaveItemResult (pChar, LEAVEITEMRES_OK);
}

/* 預けた結果をクライアントに返す（クライアントは結果を待ってから預かり所の人の返事を出す） */
static void SendLeaveItemResult(PCHARINFO pChar, BYTE byResult)
{
	CMDPARA_DATA1 Para;

	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_LEAVEITEM;
	Para.byData		= byResult;
	Para.pID		= pChar->pID;
	Para.dwTblID	= pChar->dwTblID;
	svrSock.SendTo (pChar->pID, (LPBYTE)&Para, sizeof (Para), SENDTIMEOUT);
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
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
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
	/* もう持てない？（大事な物は大事な物の欄の空きを見る） */
	if (!CanAddCharItem (pChar->dwTblID, pInfo->waItem[pPara->dwData])) {
		return;
	}

	AddCharItem (pChar->dwTblID, pInfo->waItem[pPara->dwData]);
	SendCmdCHARITEM (s_pIDRecv, pPara->dwTblID);

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
	SendCmdCHARITEM (s_pIDRecv, pPara->dwTblID);
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
		if ((pCharFrom->dwMapID != pCharTo->dwMapID) || !IsSameSpace (pCharFrom, pCharTo)) {
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
		/* 受けている依頼が違う人とは仲間になれない */
		if (QuestPartyConflict (pCharFrom, pCharTo)) {
			SendCmdSYSMSG (pCharFrom->pID, QuestPartyConflictMsg (pCharFrom, pCharTo), SPEECHCMD_BATTLE1);
			SendCmdSYSMSG (pCharTo->pID, QuestPartyConflictMsg (pCharFrom, pCharTo), SPEECHCMD_BATTLE1);
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
		/* 誘われてから依頼を受けた場合もあるので、ここでも確かめる */
		if (QuestPartyConflict (pCharFrom, pCharTo)) {
			SendCmdSYSMSG (pCharFrom->pID, QuestPartyConflictMsg (pCharFrom, pCharTo), SPEECHCMD_BATTLE1);
			SendCmdSYSMSG (pCharTo->pID, QuestPartyConflictMsg (pCharFrom, pCharTo), SPEECHCMD_BATTLE1);
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
		/* 依頼の進み具合を仲間へ送り、見えるNPCを更新する */
		QuestSendStateParty (pCharFrom->dwPartyID, NULL);
		QuestSyncAll ();
		/* 攻撃力・守備力アップのアイコンを、入った人と仲間で見せ合う */
		BuffSendPartyAll (pCharFrom);
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

	case CMDPARTY_MERCOUT:	/* 雇っている傭兵と別れる */
		MercenaryDismiss (pCharFrom);
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
/* 関数名：ChangeJobMagicKey												 */
/* 内容：転職した時に特技ショートカットを職業ごとに切り替える				 */
/*       今の職業のショートカットを職業別に覚えておき、新しい職業で以前		 */
/*       使っていたショートカットがあればそれに戻す。まだ無い職業なら、		 */
/*       装備品・道具の枠はそのまま、新しい職業で使えない特技の枠だけ外す	 */
/*       （abyMagic は新しい職業のものに入れ替えてから呼ぶ）				 */
/* 日付：2026/11/17															 */
/* ========================================================================= */
void ChangeJobMagicKey(PCHARINFO pChar, BYTE byJobOld, BYTE byJobNew)
{
	int i, j;

	if ((pChar == NULL) || (byJobOld == byJobNew)) {
		return;
	}
	/* 今の職業のショートカットを覚えておく */
	if (byJobOld < MAX_JOBMAGIC) {
		CopyMemory (pChar->abyJobMagicKey[byJobOld], pChar->abyMagicKey, sizeof (pChar->abyMagicKey));
		CopyMemory (pChar->awJobKeyItem[byJobOld], pChar->awKeyItem, sizeof (pChar->awKeyItem));
		pChar->dwJobMagicKeySaved |= (1UL << byJobOld);
	}
	/* 新しい職業で以前使っていたショートカットがあれば戻す */
	if ((byJobNew < MAX_JOBMAGIC) && (pChar->dwJobMagicKeySaved & (1UL << byJobNew))) {
		CopyMemory (pChar->abyMagicKey, pChar->abyJobMagicKey[byJobNew], sizeof (pChar->abyMagicKey));
		CopyMemory (pChar->awKeyItem, pChar->awJobKeyItem[byJobNew], sizeof (pChar->awKeyItem));
	}
	/* 今覚えていない特技の枠は外す（装備品・道具の枠は、ログインの時と同じく送る時に手持ちを確かめる） */
	for (i = 0; i < MAX_MAGICKEY; i ++) {
		if (pChar->abyMagicKey[i] == 0) {
			continue;
		}
		for (j = 0; j < MAX_USEMAGIC; j ++) {
			if (pChar->abyMagic[j] == pChar->abyMagicKey[i]) {
				break;
			}
		}
		if (j >= MAX_USEMAGIC) {
			pChar->abyMagicKey[i] = 0;
		}
	}
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

		/* 特技ショートカットも職業ごとに覚えておき、新しい職業のものに切り替える */
		ChangeJobMagicKey (pChar, byJobOld, byJobNew);
		SendMagicKeySetting (pChar);
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
	/* ミニゲームの最中は特技を使えない（ショートカットから送られても弾く） */
	if (IsSubGameItemMagicLocked (pChar)) {
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
	/* もう持てない？（大事な物は大事な物の欄の空きを見る） */
	if (!CanAddCharItem (pChar->dwTblID, dwItemID)) {
		return;
	}

	pChar->dwGold -= pItem->dwGold;
	SendCmdCHARSTATUSGOLDEXP (s_pIDRecv, pPara->dwTblID);

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

/* 取り引きを断る・やめる：本人に「取り引き終了」を返して動けるようにする */
static void TradeRefuse(PCHARINFO pChar)
{
	if ((pChar == NULL) || (pChar->pID == 0)) {
		return;
	}
	SendCmdTRADE (pChar->pID, pChar->dwTblID, CMDTRADE_OUT);
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
	/* ここから先は取り引きの申し込み。申し込んだ側のクライアントは返事が来るまで動けないので、
	   断る時は必ず「取り引き終了」を返す（返さないと操作が効かないままになる） */
	/* 依頼の見えるNPC・別の空間にいるキャラとは取り引きできない */
	if (!IsInteractable (g_aCharInfo[pRecv->dwTblID], g_aCharInfo[pRecv->dwData])) {
		TradeRefuse (g_aCharInfo[pRecv->dwTblID]);
		return;
	}
	/* 討伐チャレンジ中の人とは取り引きしない */
	if (QuestChallengeBlock (g_aCharInfo[pRecv->dwTblID])) {
		TradeRefuse (g_aCharInfo[pRecv->dwTblID]);
		return;
	}
	if (QuestInChallenge (g_aCharInfo[pRecv->dwData])) {
		SendCmdSYSMSG (g_aCharInfo[pRecv->dwTblID]->pID, "相手は討伐チャレンジ中なので取り引きできません。", SPEECHCMD_BATTLE1);
		TradeRefuse (g_aCharInfo[pRecv->dwTblID]);
		return;
	}
	/* 自分の傭兵？ 道具を持たせる・返してもらう取り引き（傭兵の返事はサーバーがする） */
	if (IsMercenary (g_aCharInfo[pRecv->dwData])) {
		PCHARINFO pOwner = g_aCharInfo[pRecv->dwTblID];
		PCHARINFO pMerc = g_aCharInfo[pRecv->dwData];
		if (GetMercenaryOwner (pMerc) != pOwner) {
			SendCmdSYSMSG (pOwner->pID, "ほかの人の傭兵とは取り引きできません。", SPEECHCMD_BATTLE1);
			SendCmdTRADE (pOwner->pID, pOwner->dwTblID, CMDTRADE_OUT);
			return;
		}
		/* 仲間なので、HPが見えている範囲なら向き合っていなくてもよい */
		if ((pMerc->wHP == 0) || (pMerc->byCharState == CHARSTATE_DEAD) || (pMerc->dwMapID != pOwner->dwMapID) ||
			(abs ((int)pMerc->dwX - (int)pOwner->dwX) > MERC_TRADERANGE) ||
			(abs ((int)pMerc->dwY - (int)pOwner->dwY) > MERC_TRADERANGE)) {
			SendCmdTRADE (pOwner->pID, pOwner->dwTblID, CMDTRADE_OUT);
			return;
		}
		/* 戦っている最中は取り引きできない(戦いながら補充し放題にしない) */
		if (MercenaryIsFighting (pMerc)) {
			char szTmp[256];
			snprintf (szTmp, sizeof (szTmp), "%sは戦っている最中なので、いまは取り引きできません。", pMerc->szName);
			SendCmdSYSMSG (pOwner->pID, szTmp, SPEECHCMD_BATTLE1);
			SendCmdTRADE (pOwner->pID, pOwner->dwTblID, CMDTRADE_OUT);
			return;
		}
		TradeStart (pOwner->dwTblID, pMerc->dwTblID);
		SendCmdCHARITEM (pOwner->pID, pMerc->dwTblID);
		SendCmdTRADE (pOwner->pID, pMerc->dwTblID, CMDTRADE_IN);
		TradeMercenaryReply (pOwner->dwTblID, pMerc->dwTblID);
		return;
	}
	/* 相手は近くにいる、ログイン中の自分以外のプレイヤーだけ */
	if ((pRecv->dwData == pRecv->dwTblID) ||
			(g_aCharInfo[pRecv->dwData]->szAcc[0] == 0) ||
			(g_aCharInfo[pRecv->dwData]->pID == 0) ||
			!IsNearRecvChar (g_aCharInfo[pRecv->dwData], 20)) {
		TradeRefuse (g_aCharInfo[pRecv->dwTblID]);
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

	/* 相手が傭兵なら、傭兵の返事をサーバーが送る */
	if (IsMercenary (g_aCharInfo[pRecv->dwTblID])) {
		TradeMercenaryReply ((DWORD)s_nCharNoRecv, pRecv->dwTblID);
		return;
	}

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
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar1) || QuestChallengeBlock (pChar2)) {
		/* 取り引き画面を開いたまま待たせない */
		TradeClear (pRecv->dwTblID1);
		TradeRefuse (pChar1);
		TradeRefuse (pChar2);
		return;
	}

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
			AddCharItemBag (pChar1->dwTblID, waItemTmp1[i]);
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
			AddCharItemBag (pChar2->dwTblID, waItemTmp2[i]);
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
					wsprintf (szTmp, "[%s]は渡せません。", ItemNameOf (waItemTmp1[pRecv->waItem1[i]]));
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
					wsprintf (szTmp, "[%s]は渡せません。", ItemNameOf (waItemTmp2[pRecv->waItem2[i]]));
					SendCmdSYSMSG (pChar2->pID, szTmp, SPEECHCMD_BATTLE1);
				} else {
					AddCharItem (pChar1->dwTblID, waItemTmp2[pRecv->waItem2[i]]);
				}
			}
		}
	}

	/* 取り引き終了通知を送信 */
	SendCmdTRADE (pChar1->pID, pChar1->dwTblID, CMDTRADE_OUT);
	if (IsMercenary (pChar2)) {
		/* 傭兵には画面が無いので、持ち物を雇い主に知らせて終わり */
		SendCmdCHARITEM (pChar1->pID, pChar1->dwTblID);
		SendCmdCHARSTATUS (pChar1->pID, pChar1->dwTblID);
		if (bMax) {
			SendCmdSYSMSG (pChar1->pID, "持ち物がいっぱいで、返してもらえなかった道具があります。", SPEECHCMD_BATTLE1);
		}
		MercenaryTellItems (pChar1, pChar2);
		return;
	}
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
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
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
		SendCmdCHARSTATUSGOLDEXP (s_pIDRecv, pRecv->dwTblID);
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
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
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
		SendCmdCHARSTATUSGOLDEXP (s_pIDRecv, pRecv->dwTblID);
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
/* 関数名：IsSnowFloorServer												 */
/* 内容：その場所で雪をつかめるか（クライアントの IsSnowPickupPos と同じ判定） */
/* ========================================================================= */
static BOOL IsSnowFloorServer(DWORD dwMapID, DWORD dwX, DWORD dwY)
{
	int x, y, nCX, nCY;
	BYTE byHitType;
	PMAPINFO pMap;

	if (dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return FALSE;
	}
	pMap = g_aMapInfo[dwMapID];
	if ((pMap == NULL) || (pMap->pwMap == NULL)) {
		return FALSE;
	}
	nCX = pMap->nCX;
	nCY = pMap->nCY;
	if (((int)dwX < 0) || ((int)dwX >= nCX) || ((int)dwY < 0) || ((int)dwY >= nCY)) {
		return FALSE;
	}
	byHitType = g_aMapPartsInfo[pMap->pwMap[dwX + dwY * nCX]].byHitType;
	if (byHitType == PARTSHIT_FOOTMARK_SNOW) {
		return TRUE;
	}
	/* 上を歩ける「隠れる」マスだけ、周りを見る */
	if ((byHitType != PARTSHIT_HIDE1) && (byHitType != PARTSHIT_HIDE3) &&
		(byHitType != PARTSHIT_HIDE5) && (byHitType != PARTSHIT_HIDE7)) {
		return FALSE;
	}
	for (y = (int)dwY - 2; y <= (int)dwY + 2; y ++) {
		for (x = (int)dwX - 2; x <= (int)dwX + 2; x ++) {
			if (abs (x - (int)dwX) + abs (y - (int)dwY) > 2) {
				continue;
			}
			if ((x < 0) || (x >= nCX) || (y < 0) || (y >= nCY)) {
				continue;
			}
			if (g_aMapPartsInfo[pMap->pwMap[x + y * nCX]].byHitType == PARTSHIT_FOOTMARK_SNOW) {
				return TRUE;
			}
		}
	}
	return FALSE;
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

	/* 死亡中は投げられない */
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}
	if (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return;
	}
	/* 雪の上でだけ投げられる（足もとか、1 つ前に歩いたマスが雪なら通す） */
	if (!IsSnowFloorServer (pChar->dwMapID, pChar->dwX, pChar->dwY) &&
		!IsSnowFloorServer (pChar->dwMapID, pChar->dwXBack, pChar->dwYBack)) {
		return;
	}
	/* 前回から間隔が短すぎる投げは無視する（公式は 2 秒ごと） */
	{
		DWORD dwNow = timeGetTime ();
		static std::unordered_map<DWORD, DWORD> s_mapLastSnow;
		std::unordered_map<DWORD, DWORD>::iterator it = s_mapLastSnow.find (pChar->dwTblID);
		if ((it != s_mapLastSnow.end ()) && ((DWORD)(dwNow - it->second) < 1500)) {
			return;
		}
		s_mapLastSnow[pChar->dwTblID] = dwNow;
	}
	/* 同じ人の雪玉が同時に出ている数にも上限を設ける */
	{
		int i, nSnow = 0, nCount = g_aCharInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PCHARINFO pTmp = g_aCharInfo[i];
			if ((pTmp->byNpcType == NPCTYPE_ATACK) && (pTmp->dwParentTblID == pChar->dwTblID) &&
				(pTmp->wGrpID == 213)) {
				nSnow ++;
			}
		}
		if (nSnow >= 5) {
			return;
		}
	}

	PCHARINFO pCharNew = new CHARINFO;
	ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));

	pCharNew->byNpcType = NPCTYPE_ATACK;
	SetInitCharData (pCharNew);
	pCharNew->dwSpaceID = pChar->dwSpaceID;		/* 撃った人と同じ空間 */
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
	pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	/* 爆弾ゲームに参加していない？ */
	if (pSubStatus->byType != SUBGAMEID_BOMB) {
		return;
	}
	if (pSubStatus->byID >= g_aSubaGameInfo.GetSize ()) {
		return;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	pInfo = g_aSubaGameInfo[pSubStatus->byID];
	pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

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
	pCharTmp->dwSpaceID = pChar->dwSpaceID;		/* 置いた人と同じ空間 */
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
				/* 爆風の見える時間（800ms）をここから数える。3 秒ごとに更新していた時刻のままだと早く消えていた */
				pCharTmp->dwLastAnime = timeGetTime ();
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
		/* 爆風の見える時間（800ms）をここから数える。3 秒ごとに更新していた時刻のままだと早く消えていた */
		pCharTmp->dwLastAnime = timeGetTime ();
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
	POINT ptStart = {0, 0};
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

	/* チーム番号は 0/1 だけ（チーム情報が無い番号は受け付けない） */
	if ((pRecv->byData >= 2) || (pProcSubGameWAR->GetInfo ()->GetTeamInfo (pRecv->byData) == NULL)) {
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

	/* 実際のメンバー数の範囲内だけ受け付ける */
	if (!pProcSubGameWAR->IsTurnNoOK ((BYTE)nTmp, pRecv->byData)) {
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

	/* 向きは 0〜3 だけ */
	if (pRecv->byData >= 4) {
		goto Exit;
	}

	bCheck = TRUE;
	dwTblID = CheckCharBlockPC (pChar->dwMapID,
					pChar->dwX + anPosAddTblX[pRecv->byData],
					pChar->dwY + anPosAddTblY[pRecv->byData], pChar->dwSpaceID);
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

	/* 行動中のキャラ本人だけ、職業に合った魔法を、実在する対象にだけ使える */
	if (pInfoSubGameWAR->GetTurnTblID (pInfoSubGameWAR->m_byTurn) != pChar->dwTblID) {
		goto Exit;
	}
	if (!(((pRecv->byData1 == SUBGAME_WAR_MAGIC_FIRE) && (pSubStatus->byJob == SUBGAME_WAR_JOB_WITCH)) ||
		  (((pRecv->byData1 == SUBGAME_WAR_MAGIC_QURE) || (pRecv->byData1 == SUBGAME_WAR_MAGIC_RESUSCITATION)) &&
		   (pSubStatus->byJob == SUBGAME_WAR_JOB_PRIEST)))) {
		goto Exit;
	}
	if (!pProcSubGameWAR->IsMagicTargetOK (pRecv->byData1, pRecv->byData2)) {
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
	DWORD dwMyHead;

	pChar = pCharTarget = NULL;

	Packet.Set (pData);

	if (!IsRecvChar (Packet.m_dwTblID)) {
		goto Exit;
	}
	pChar = g_aCharInfo[Packet.m_dwTblID];
	/* 戦闘モード中のZは普通のキャンセル扱い。「ついていく」の開始・解除は通常モードの時だけ。
	   ただし黙って無視すると、送る時に「とりあえずついていっている」ことにしたクライアントが
	   動けないままになる（サーバーにだけ古い戦闘モードの覚えが残っていた時など）ので、
	   「ついていっていない」ことに戻す返事だけ送る（メッセージは出ない） */
	if (IsCharBattleMode (pChar)) {
		Packet.Make (pChar->dwTblID, (DWORD)-1, CHAR_TAIL_CHASEEND);
		svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
		goto Exit;
	}
	/* 列に戻る途中(追いかけ中)のZは、追いかけをやめて解除 */
	if (TailChaseIsActive (pChar)) {
		TailChaseCancel (pChar, NULL, TRUE);
		TailReturnForget (pChar->dwTblID);
		Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_OFF);
		svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
		goto Exit;
	}
	/* 自分が元いた列(戦闘でばらけた列)には、自分で入り直せるように覚えておく */
	dwMyHead = TailReturnHeadOf (pChar);
	/* 自分でZを押したら、戦闘で列を離れた時の戻り先は忘れる */
	TailReturnForget (pChar->dwTblID);
	/* 誰かについていっている？ */
	if (pChar->dwTailTblID != -1) {
		/* ついていくのを解除 */
		TailOff (pChar);
		goto Exit;
	}

	if (Packet.m_dwTailTblID != -1) {
		if (Packet.m_dwTailTblID >= (DWORD)g_aCharInfo.GetSize ()) {
			/* 相手がいない。クライアントは送る時に「ついていっている」ことにしているので戻す */
			Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_NG2);
			svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
			goto Exit;
		}
		pCharTarget = g_aCharInfo[Packet.m_dwTailTblID];
		bResult = IsTail (pChar, pCharTarget);
		if (bResult == FALSE) {
			Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_NG2);
			svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
			goto Exit;
		}
		/* 傭兵には直接ついていかない。傭兵の前にいる人(雇い主の列のプレイヤー)に
		   ついていき、傭兵は列の最後へ回ってもらう（傭兵は戦いで列を出入りするので、
		   傭兵の後ろにつくと列が切れたりつなぎ直しが重なったりしていた） */
		if (IsMercenary (pCharTarget)) {
			PCHARINFO pFront = pCharTarget;
			int i;
			for (i = 0; i < 100; i ++) {
				if ((pFront->dwTailTblID == (DWORD)-1) || (pFront->dwTailTblID >= (DWORD)g_aCharInfo.GetSize ())) {
					pFront = NULL;
					break;
				}
				pFront = g_aCharInfo[pFront->dwTailTblID];
				if (!IsMercenary (pFront)) {
					break;
				}
			}
			if ((pFront == NULL) || IsMercenary (pFront) || (pFront == pChar)) {
				Packet.Make (pChar->dwTblID, -1, CHAR_TAIL_NG2);
				svrSock.SendTo (pChar->pID, Packet.Get (), Packet.GetSize ());
				goto Exit;
			}
			pCharTarget = pFront;
		}
		/* 戦闘でばらけている列(と、その列を離れている人)には入れない */
		if (TailIsScattered (pCharTarget, dwMyHead)) {
			Packet.Make (pChar->dwTblID, pCharTarget->dwTblID, CHAR_TAIL_NG3);
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

	case WEBACC_OPTMAGICBAR:	/* 特技ショートカットの帯の表示設定を覚える */
		if ((s_dwRecvSize < sizeof (CMDPARA_WEBACCOUNT)) || (s_nCharNoRecv < 0)) {
			return;
		}
		if ((pPara->byData != OPTMAGICBAR_SHOW) && (pPara->byData != OPTMAGICBAR_HIDE)) {
			return;
		}
		g_aCharInfo[s_nCharNoRecv]->byOptMagicBar = pPara->byData;
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
/* 関数名：SendMagicKeySetting												 */
/* 内容：特技ショートカットの登録と帯の表示設定をログインしたクライアントへ送る */
/*       （別のアカウントで入り直した時に前の設定が残らないよう、毎回送る）	 */
/* ========================================================================= */
static void SendMagicKeySetting(PCHARINFO pChar)
{
	CMDPARA_WEBACCOUNT SendBar;
	CMDPARA_MAGICKEY SendKey;
	int i, j;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return;
	}
	/* 手持ちに無い装備品・道具の枠は外す（預けた・使い切った後に入り直した時など） */
	for (i = 0; i < MAX_MAGICKEY; i ++) {
		if (pChar->awKeyItem[i] == 0) {
			continue;
		}
		for (j = 0; j < MAX_MYITEM; j ++) {
			if (pChar->waItem[j] == pChar->awKeyItem[i]) {
				break;
			}
		}
		if (j >= MAX_MYITEM) {
			pChar->awKeyItem[i] = 0;
		}
	}
	ZeroMemory (&SendBar, sizeof (SendBar));
	SendBar.byCmd = CMD_WEBACCOUNT;
	SendBar.byCmdSub = WEBACC_OPTMAGICBAR;
	SendBar.byData = pChar->byOptMagicBar;
	svrSock.SendTo (pChar->pID, (LPBYTE)&SendBar, sizeof (SendBar), SENDTIMEOUT);

	ZeroMemory (&SendKey, sizeof (SendKey));
	SendKey.byCmd = CMD_MAGICKEY;
	SendKey.byCmdSub = MAGICKEY_ALL;
	CopyMemory (SendKey.abyMagicKey, pChar->abyMagicKey, sizeof (SendKey.abyMagicKey));
	CopyMemory (SendKey.awKeyItem, pChar->awKeyItem, sizeof (SendKey.awKeyItem));
	svrSock.SendTo (pChar->pID, (LPBYTE)&SendKey, sizeof (SendKey), SENDTIMEOUT);
}

/* ========================================================================= */
/* 関数名：RecvDataProcMAGICKEY												 */
/* 内容：受信データ処理(特技ショートカットの登録)							 */
/*       byNo 番目の枠に特技(byMagicID)か装備品・道具(wItemID)を入れる		 */
/*       （両方 0 なら外す。両方入っていたら無視）。覚えていない特技・手持ちに */
/*       無いアイテムは入れない。同じものが別の枠にあればそちらは外す		 */
/*       （重複させない）。特技と装備品・道具は同じ枠を共有する				 */
/*       保存は定期保存・終了時の保存に任せる								 */
/* ========================================================================= */
void RecvDataProcMAGICKEY(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_MAGICKEY pPara = (PCMDPARA_MAGICKEY)pData;
	PCHARINFO pChar;
	int i;

	if ((s_dwRecvSize < sizeof (CMDPARA_MAGICKEY)) || (s_nCharNoRecv < 0)) {
		return;
	}
	if ((pPara->byCmdSub != MAGICKEY_SET) || (pPara->byNo >= MAX_MAGICKEY)) {
		return;
	}
	if ((pPara->byMagicID != 0) && (pPara->wItemID != 0)) {
		return;
	}
	pChar = g_aCharInfo[s_nCharNoRecv];
	if (pPara->wItemID != 0) {
		PITEMINFO pItem;

		if (!ItemIDValid (pPara->wItemID)) {
			return;
		}
		pItem = g_aItemInfo[pPara->wItemID];
		if (pItem->byType == ITEMTYPE_NONE) {
			return;
		}
		for (i = 0; i < MAX_MYITEM; i ++) {
			if (pChar->waItem[i] == pPara->wItemID) {
				break;
			}
		}
		if (i >= MAX_MYITEM) {
			return;
		}
		for (i = 0; i < MAX_MAGICKEY; i ++) {
			if (pChar->awKeyItem[i] == pPara->wItemID) {
				pChar->awKeyItem[i] = 0;
			}
		}
		pChar->awKeyItem[pPara->byNo] = pPara->wItemID;
		pChar->abyMagicKey[pPara->byNo] = 0;
		return;
	}
	if (pPara->byMagicID != 0) {
		for (i = 0; i < MAX_USEMAGIC; i ++) {
			if (pChar->abyMagic[i] == pPara->byMagicID) {
				break;
			}
		}
		if (i >= MAX_USEMAGIC) {
			return;
		}
		for (i = 0; i < MAX_MAGICKEY; i ++) {
			if (pChar->abyMagicKey[i] == pPara->byMagicID) {
				pChar->abyMagicKey[i] = 0;
			}
		}
	}
	pChar->abyMagicKey[pPara->byNo] = pPara->byMagicID;
	pChar->awKeyItem[pPara->byNo] = 0;
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
/* 関数名：RecvDataProcADMINSPACE											 */
/* 内容：受信データ処理(管理者の空間の切り替え)								 */
/*       LISTREQ: いま在る空間の一覧と、いま居る空間を LIST で返す			 */
/*       SET    : 一覧にある空間（か 0）へ自分を移し、結果を LIST で返す		 */
/*                管理者も、移った空間では普通のキャラと同じ（見える・ぶつかる・	 */
/*                攻撃できる。ほかの人からも見える）						 */
/* ========================================================================= */
void RecvDataProcADMINSPACE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINSPACE pPara = (PCMDPARA_ADMINSPACE)pData;
	PCHARINFO pChar;
	BYTE byResult = 0;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINSPACE)) {
		return;
	}
	if (s_nCharNoRecv < 0) {
		return;
	}
	pChar = g_aCharInfo[s_nCharNoRecv];

	switch (pPara->byCmdSub) {
	case ADMINSPACE_LISTREQ:	/* 一覧の問い合わせ */
		break;

	case ADMINSPACE_SET:		/* 切り替え */
		/* 遊んでいて、倒れていない時だけ。討伐チャレンジ中の人は別の空間にいるので動かさない */
		if (!SpaceExists (pPara->dwSpaceID) || QuestInChallenge (pChar) ||
				(pChar->byGameState != GAMESTATE_MOVE) || (GetMapNo (pChar->dwMapID) < 0)) {
			break;
		}
		SpaceSet (pChar, pPara->dwSpaceID);
		byResult = 1;
		break;

	default:
		return;
	}
	SendCmdADMINSPACE (pChar->pID, byResult, pChar->dwSpaceID);
}

/* ========================================================================= */
/* 関数名：SendAdminQuestItem												 */
/* 内容：管理者ウィンドウへ依頼 1 件を送る（nNo が範囲外なら件数だけ）		 */
/* ========================================================================= */
static void SendAdminQuestItem(DWORD pID, int nNo)
{
	PCMDPARA_ADMINQUEST pPara = new CMDPARA_ADMINQUEST;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINQUEST));
	pPara->byCmd	= CMD_ADMINQUEST;
	pPara->byCmdSub	= ADMINQUEST_ITEM;
	pPara->wCount	= (WORD)g_aQuestInfo.GetSize ();
	pPara->nNo		= nNo;
	if ((nNo >= 0) && (nNo < g_aQuestInfo.GetSize ()) && g_aQuestInfo[nNo]) {
		pPara->Info = *g_aQuestInfo[nNo];
	} else {
		pPara->nNo = -1;
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINQUEST));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINQUEST											 */
/* 内容：受信データ処理(管理者ウィンドウのギルドの依頼の編集)				 */
/*       LISTREQ : 依頼を 1 件ずつ ITEM で送る（0 件なら件数 0 の ITEM だけ）	 */
/*       SET     : データAPI（SboWebDataApply）と同じ確認を通して書き込み、	 */
/*                 RESULT と、うまくいけばその依頼の ITEM を返す				 */
/* ========================================================================= */
void RecvDataProcADMINQUEST(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINQUEST pPara = (PCMDPARA_ADMINQUEST)pData;
	PCMDPARA_ADMINQUEST pRes;
	DWORD pID = s_pIDRecv;
	int i, nNoOut = -1;
	std::string strErr;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINQUEST)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINQUEST_LISTREQ:	/* 一覧の問い合わせ */
		if (g_aQuestInfo.GetSize () == 0) {
			SendAdminQuestItem (pID, -1);
			break;
		}
		for (i = 0; i < g_aQuestInfo.GetSize (); i ++) {
			SendAdminQuestItem (pID, i);
		}
		break;

	case ADMINQUEST_SET:		/* 書き込み */
		SboDataQuestApplyInfo (pPara->nNo, &pPara->Info, nNoOut, strErr);
		pRes = new CMDPARA_ADMINQUEST;
		ZeroMemory (pRes, sizeof (CMDPARA_ADMINQUEST));
		pRes->byCmd		= CMD_ADMINQUEST;
		pRes->byCmdSub	= ADMINQUEST_RESULT;
		pRes->byResult	= strErr.empty () ? 1 : 0;
		pRes->wCount	= (WORD)g_aQuestInfo.GetSize ();
		pRes->nNo		= strErr.empty () ? nNoOut : pPara->nNo;
		SboTextCopy (pRes->szMsg, sizeof (pRes->szMsg), strErr.c_str ());
		svrSock.SendTo (pID, (PBYTE)pRes, sizeof (CMDPARA_ADMINQUEST));
		SAFE_DELETE (pRes);
		if (strErr.empty ()) {
			SendAdminQuestItem (pID, nNoOut);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendAdminFlagNameItem											 */
/* 内容：管理者ウィンドウへフラグの名前表を nNo 番目から最大 1 パケットぶん送る	 */
/*       （nNo が範囲外なら件数だけ）										 */
/* ========================================================================= */
static void SendAdminFlagNameItem(DWORD pID, int nNo)
{
	PCMDPARA_ADMINFLAGNAME pPara = new CMDPARA_ADMINFLAGNAME;
	int i, nCount = g_aFlagNameInfo.GetSize ();

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINFLAGNAME));
	pPara->byCmd	= CMD_ADMINFLAGNAME;
	pPara->byCmdSub	= ADMINFLAGNAME_ITEM;
	pPara->wCount	= (WORD)nCount;
	pPara->nNo		= nNo;
	if ((nNo >= 0) && (nNo < nCount)) {
		for (i = 0; (i < MAX_ADMINFLAGNAMEBATCH) && (nNo + i < nCount); i ++) {
			if (g_aFlagNameInfo[nNo + i]) {
				pPara->aInfo[i] = *g_aFlagNameInfo[nNo + i];
			}
		}
		pPara->byCount = (BYTE)i;
	} else {
		pPara->nNo = -1;
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINFLAGNAME));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINFLAGNAME										 */
/* 内容：受信データ処理(管理者ウィンドウのフラグの名前表の編集)				 */
/*       LISTREQ : 名前表を MAX_ADMINFLAGNAMEBATCH 件ずつ ITEM で送る			 */
/*                 （0 件なら件数 0 の ITEM だけ）							 */
/*       SET     : データAPI（SboWebDataApply）と同じ確認を通して書き込み、	 */
/*                 RESULT と、うまくいけばその 1 件の ITEM を返す			 */
/* ========================================================================= */
void RecvDataProcADMINFLAGNAME(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINFLAGNAME pPara = (PCMDPARA_ADMINFLAGNAME)pData;
	PCMDPARA_ADMINFLAGNAME pRes;
	DWORD pID = s_pIDRecv;
	int i, nNoOut = -1;
	std::string strErr;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINFLAGNAME)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINFLAGNAME_LISTREQ:	/* 一覧の問い合わせ */
		if (g_aFlagNameInfo.GetSize () == 0) {
			SendAdminFlagNameItem (pID, -1);
			break;
		}
		for (i = 0; i < g_aFlagNameInfo.GetSize (); i += MAX_ADMINFLAGNAMEBATCH) {
			SendAdminFlagNameItem (pID, i);
		}
		break;

	case ADMINFLAGNAME_SET:		/* 書き込み */
		pPara->aInfo[0].szName[sizeof (pPara->aInfo[0].szName) - 1] = 0;
		SboDataFlagNameApplyInfo (pPara->nNo, &pPara->aInfo[0], nNoOut, strErr);
		pRes = new CMDPARA_ADMINFLAGNAME;
		ZeroMemory (pRes, sizeof (CMDPARA_ADMINFLAGNAME));
		pRes->byCmd		= CMD_ADMINFLAGNAME;
		pRes->byCmdSub	= ADMINFLAGNAME_RESULT;
		pRes->byResult	= strErr.empty () ? 1 : 0;
		pRes->wCount	= (WORD)g_aFlagNameInfo.GetSize ();
		pRes->nNo		= strErr.empty () ? nNoOut : pPara->nNo;
		SboTextCopy (pRes->szMsg, sizeof (pRes->szMsg), strErr.c_str ());
		svrSock.SendTo (pID, (PBYTE)pRes, sizeof (CMDPARA_ADMINFLAGNAME));
		SAFE_DELETE (pRes);
		if (strErr.empty ()) {
			SendAdminFlagNameItem (pID, nNoOut);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendAdminTalkItem												 */
/* 内容：管理者ウィンドウへ会話セット 1 件を送る（nNo が範囲外なら件数だけ）	 */
/* ========================================================================= */
static void SendAdminTalkItem(DWORD pID, int nNo)
{
	PCMDPARA_ADMINTALK pPara = new CMDPARA_ADMINTALK;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINTALK));
	pPara->byCmd	= CMD_ADMINTALK;
	pPara->byCmdSub	= ADMINTALK_ITEM;
	pPara->wCount	= (WORD)g_aTalkInfo.GetSize ();
	pPara->nNo		= nNo;
	if ((nNo >= 0) && (nNo < g_aTalkInfo.GetSize ()) && g_aTalkInfo[nNo]) {
		pPara->Info = *g_aTalkInfo[nNo];
	} else {
		pPara->nNo = -1;
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINTALK));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：SendAdminTalkUseAll												 */
/* 内容：管理者ウィンドウへ、マップの名前（MAP）と会話セットの使用先（USE）を送る */
/*       USE は、保存されている NPC（プレイヤーでも、増えた分身でもない）のうち	 */
/*       台詞が "@t番号" のもの 1 体ごとに 1 つ。書式の判定は TalkIsNpc と同じ	 */
/* ========================================================================= */
static void SendAdminTalkUseAll(DWORD pID)
{
	PCMDPARA_ADMINTALKUSE pPara = new CMDPARA_ADMINTALKUSE;
	int i;

	/* マップの名前（絞り込みの選択肢に使う） */
	for (i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		if (pMap == NULL) {
			continue;
		}
		ZeroMemory (pPara, sizeof (CMDPARA_ADMINTALKUSE));
		pPara->byCmd	= CMD_ADMINTALK;
		pPara->byCmdSub	= ADMINTALK_MAP;
		pPara->dwMapID	= pMap->dwID;
		if (pMap->pszName) {
			SboTextCopy (pPara->szName, sizeof (pPara->szName), (const char *)pMap->pszName);
		}
		svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINTALKUSE));
	}

	/* 使っている NPC */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pNpc = g_aCharInfo[i];
		if ((pNpc == NULL) || pNpc->szAcc[0] || (pNpc->dwParentTblID != (DWORD)-1) || !TalkIsNpc (pNpc)) {
			continue;
		}
		ZeroMemory (pPara, sizeof (CMDPARA_ADMINTALKUSE));
		pPara->byCmd	= CMD_ADMINTALK;
		pPara->byCmdSub	= ADMINTALK_USE;
		pPara->wTalkNo	= (WORD)TalkGetNo (pNpc);
		pPara->dwMapID	= pNpc->dwMapID;
		pPara->dwNpcID	= pNpc->dwNpcID;
		SboTextCopy (pPara->szName, sizeof (pPara->szName), pNpc->szName);
		svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINTALKUSE));
	}
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINTALK											 */
/* 内容：受信データ処理(管理者ウィンドウの会話データの編集)					 */
/*       LISTREQ : マップの名前（MAP）・使用先（USE）を送った後、				 */
/*                 会話セットを 1 件ずつ ITEM で送る（0 件なら件数 0 の ITEM だけ）*/
/*       SET     : データAPI（SboWebDataApply）と同じ確認を通して書き込み、	 */
/*                 RESULT と、うまくいけばその会話セットの ITEM を返す		 */
/* ========================================================================= */
void RecvDataProcADMINTALK(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINTALK pPara = (PCMDPARA_ADMINTALK)pData;
	PCMDPARA_ADMINTALK pRes;
	DWORD pID = s_pIDRecv;
	int i, nNoOut = -1;
	std::string strErr;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINTALK)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINTALK_LISTREQ:		/* 一覧の問い合わせ */
		SendAdminTalkUseAll (pID);
		if (g_aTalkInfo.GetSize () == 0) {
			SendAdminTalkItem (pID, -1);
			break;
		}
		for (i = 0; i < g_aTalkInfo.GetSize (); i ++) {
			SendAdminTalkItem (pID, i);
		}
		break;

	case ADMINTALK_SET:			/* 書き込み */
		SboDataTalkApplyInfo (pPara->nNo, &pPara->Info, nNoOut, strErr);
		pRes = new CMDPARA_ADMINTALK;
		ZeroMemory (pRes, sizeof (CMDPARA_ADMINTALK));
		pRes->byCmd		= CMD_ADMINTALK;
		pRes->byCmdSub	= ADMINTALK_RESULT;
		pRes->byResult	= strErr.empty () ? 1 : 0;
		pRes->wCount	= (WORD)g_aTalkInfo.GetSize ();
		pRes->nNo		= strErr.empty () ? nNoOut : pPara->nNo;
		SboTextCopy (pRes->szMsg, sizeof (pRes->szMsg), strErr.c_str ());
		svrSock.SendTo (pID, (PBYTE)pRes, sizeof (CMDPARA_ADMINTALK));
		SAFE_DELETE (pRes);
		if (strErr.empty ()) {
			SendAdminTalkItem (pID, nNoOut);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendAdminStoryItem												 */
/* 内容：管理者ウィンドウへシナリオ 1 件を送る（nNo が範囲外なら件数だけ）	 */
/* ========================================================================= */
static void SendAdminStoryItem(DWORD pID, int nNo)
{
	PCMDPARA_ADMINSTORY pPara = new CMDPARA_ADMINSTORY;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINSTORY));
	pPara->byCmd	= CMD_ADMINSTORY;
	pPara->byCmdSub	= ADMINSTORY_ITEM;
	pPara->wCount	= (WORD)g_aStoryInfo.GetSize ();
	pPara->nNo		= nNo;
	if ((nNo >= 0) && (nNo < g_aStoryInfo.GetSize ()) && g_aStoryInfo[nNo]) {
		pPara->Info = *g_aStoryInfo[nNo];
	} else {
		pPara->nNo = -1;
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINSTORY));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINSTORY											 */
/* 内容：受信データ処理(管理者ウィンドウの思い出の本の編集)					 */
/*       LISTREQ : シナリオを 1 件ずつ ITEM で送る（0 件なら件数 0 の ITEM だけ）*/
/*       SET     : データAPI（SboWebDataApply）と同じ確認を通して書き込み、	 */
/*                 RESULT と、うまくいけばそのシナリオの ITEM を返す			 */
/* ========================================================================= */
void RecvDataProcADMINSTORY(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINSTORY pPara = (PCMDPARA_ADMINSTORY)pData;
	PCMDPARA_ADMINSTORY pRes;
	DWORD pID = s_pIDRecv;
	int i, nNoOut = -1;
	std::string strErr;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINSTORY)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINSTORY_LISTREQ:	/* 一覧の問い合わせ */
		if (g_aStoryInfo.GetSize () == 0) {
			SendAdminStoryItem (pID, -1);
			break;
		}
		for (i = 0; i < g_aStoryInfo.GetSize (); i ++) {
			SendAdminStoryItem (pID, i);
		}
		break;

	case ADMINSTORY_SET:		/* 書き込み */
		SboDataStoryApplyInfo (pPara->nNo, &pPara->Info, nNoOut, strErr);
		pRes = new CMDPARA_ADMINSTORY;
		ZeroMemory (pRes, sizeof (CMDPARA_ADMINSTORY));
		pRes->byCmd		= CMD_ADMINSTORY;
		pRes->byCmdSub	= ADMINSTORY_RESULT;
		pRes->byResult	= strErr.empty () ? 1 : 0;
		pRes->wCount	= (WORD)g_aStoryInfo.GetSize ();
		pRes->nNo		= strErr.empty () ? nNoOut : pPara->nNo;
		SboTextCopy (pRes->szMsg, sizeof (pRes->szMsg), strErr.c_str ());
		svrSock.SendTo (pID, (PBYTE)pRes, sizeof (CMDPARA_ADMINSTORY));
		SAFE_DELETE (pRes);
		if (strErr.empty ()) {
			SendAdminStoryItem (pID, nNoOut);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendAdminFlagGroupItem											 */
/* 内容：管理者ウィンドウへフラグのグループ 1 件を送る（nNo が範囲外なら件数だけ）*/
/* ========================================================================= */
static void SendAdminFlagGroupItem(DWORD pID, int nNo)
{
	PCMDPARA_ADMINFLAGGROUP pPara = new CMDPARA_ADMINFLAGGROUP;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINFLAGGROUP));
	pPara->byCmd	= CMD_ADMINFLAGGROUP;
	pPara->byCmdSub	= ADMINFLAGGROUP_ITEM;
	pPara->wCount	= (WORD)g_aFlagGroupInfo.GetSize ();
	pPara->nNo		= nNo;
	if ((nNo >= 0) && (nNo < g_aFlagGroupInfo.GetSize ()) && g_aFlagGroupInfo[nNo]) {
		pPara->Info = *g_aFlagGroupInfo[nNo];
	} else {
		pPara->nNo = -1;
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINFLAGGROUP));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINFLAGGROUP										 */
/* 内容：受信データ処理(管理者ウィンドウのフラグのグループの編集)			 */
/*       LISTREQ : グループを 1 件ずつ ITEM で送る（0 件なら件数 0 の ITEM だけ）*/
/*       SET     : データAPI（SboWebDataApply）と同じ確認を通して書き込み、	 */
/*                 RESULT と、うまくいけばそのグループの ITEM を返す			 */
/* ========================================================================= */
void RecvDataProcADMINFLAGGROUP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINFLAGGROUP pPara = (PCMDPARA_ADMINFLAGGROUP)pData;
	PCMDPARA_ADMINFLAGGROUP pRes;
	DWORD pID = s_pIDRecv;
	int i, nNoOut = -1;
	std::string strErr;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINFLAGGROUP)) {
		return;
	}

	switch (pPara->byCmdSub) {
	case ADMINFLAGGROUP_LISTREQ:	/* 一覧の問い合わせ */
		if (g_aFlagGroupInfo.GetSize () == 0) {
			SendAdminFlagGroupItem (pID, -1);
			break;
		}
		for (i = 0; i < g_aFlagGroupInfo.GetSize (); i ++) {
			SendAdminFlagGroupItem (pID, i);
		}
		break;

	case ADMINFLAGGROUP_SET:		/* 書き込み */
		SboDataFlagGroupApplyInfo (pPara->nNo, &pPara->Info, nNoOut, strErr);
		pRes = new CMDPARA_ADMINFLAGGROUP;
		ZeroMemory (pRes, sizeof (CMDPARA_ADMINFLAGGROUP));
		pRes->byCmd		= CMD_ADMINFLAGGROUP;
		pRes->byCmdSub	= ADMINFLAGGROUP_RESULT;
		pRes->byResult	= strErr.empty () ? 1 : 0;
		pRes->wCount	= (WORD)g_aFlagGroupInfo.GetSize ();
		pRes->nNo		= strErr.empty () ? nNoOut : pPara->nNo;
		SboTextCopy (pRes->szMsg, sizeof (pRes->szMsg), strErr.c_str ());
		svrSock.SendTo (pID, (PBYTE)pRes, sizeof (CMDPARA_ADMINFLAGGROUP));
		SAFE_DELETE (pRes);
		if (strErr.empty ()) {
			SendAdminFlagGroupItem (pID, nNoOut);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：SendAdminCharFlagInfo											 */
/* 内容：管理者ウィンドウへキャラのフラグ（印・数値）を全部送る				 */
/* ========================================================================= */
static void SendAdminCharFlagInfo(DWORD pID, PCHARINFO pChar)
{
	PCMDPARA_ADMINCHARFLAG pPara = new CMDPARA_ADMINCHARFLAG;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINCHARFLAG));
	pPara->byCmd	= CMD_ADMINCHARFLAG;
	pPara->byCmdSub	= ADMINCHARFLAG_INFO;
	pPara->dwTblID	= pChar->dwTblID;
	CopyMemory (pPara->abyFlag, pChar->abyFlag, sizeof (pPara->abyFlag));
	CopyMemory (pPara->abyFlagNum, pChar->abyFlagNum, sizeof (pPara->abyFlagNum));
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINCHARFLAG));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：SendAdminCharFlagResult											 */
/* 内容：管理者ウィンドウへキャラのフラグの変更の結果を送る					 */
/* ========================================================================= */
static void SendAdminCharFlagResult(DWORD pID, DWORD dwTblID, BYTE byResult, LPCSTR pszMsg)
{
	PCMDPARA_ADMINCHARFLAG pPara = new CMDPARA_ADMINCHARFLAG;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINCHARFLAG));
	pPara->byCmd	= CMD_ADMINCHARFLAG;
	pPara->byCmdSub	= ADMINCHARFLAG_RESULT;
	pPara->byResult	= byResult;
	pPara->dwTblID	= dwTblID;
	SboTextCopy (pPara->szMsg, sizeof (pPara->szMsg), pszMsg);
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINCHARFLAG));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINCHARFLAG										 */
/* 内容：受信データ処理(管理者ウィンドウのキャラのフラグの取得・変更)		 */
/*       GET : 相手（プレイヤーのキャラ）の印と数値を INFO で返す			 */
/*       SET : 印を立てる・消す／数値を ○にする。結果を RESULT で、			 */
/*             うまくいけば変えた後の INFO も返す							 */
/*       相手はログアウト中のプレイヤーでもよい（次にログインした時に効く）	 */
/* ========================================================================= */
void RecvDataProcADMINCHARFLAG(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINCHARFLAG pPara = (PCMDPARA_ADMINCHARFLAG)pData;
	PCHARINFO pChar;
	DWORD pID = s_pIDRecv;

	if (s_dwRecvSize < sizeof (CMDPARA_ADMINCHARFLAG)) {
		return;
	}
	if ((pPara->byCmdSub != ADMINCHARFLAG_GET) && (pPara->byCmdSub != ADMINCHARFLAG_SET)) {
		return;
	}
	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		SendAdminCharFlagResult (pID, pPara->dwTblID, 0, "キャラが見つかりません。");
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	/* プレイヤーのキャラだけ（NPC・敵は持たない） */
	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->szAcc[0] == 0)) {
		SendAdminCharFlagResult (pID, pPara->dwTblID, 0, "プレイヤーのキャラを選んでください。");
		return;
	}

	if (pPara->byCmdSub == ADMINCHARFLAG_GET) {
		SendAdminCharFlagInfo (pID, pChar);
		return;
	}

	/* 変更 */
	if (pPara->byKind == FLAGKIND_BIT) {
		if ((pPara->wNo >= MAX_FLAGBIT) || (pPara->wValue > 1)) {
			SendAdminCharFlagResult (pID, pPara->dwTblID, 0, "印の番号は 0〜1023、値は 0（消す）か 1（立てる）で指定してください。");
			return;
		}
		FlagBitSet (pChar->abyFlag, pPara->wNo, pPara->wValue ? TRUE : FALSE);
		/* 印で見える・消えるNPCを、その人にだけ見え直す */
		CharViewRefresh (pChar);
		QuestSendStateSelf (pChar);		/* 物語の「いまの目的」も送り直す */
	} else if (pPara->byKind == FLAGKIND_NUM) {
		if ((pPara->wNo >= MAX_FLAGNUM) || (pPara->wValue > 255)) {
			SendAdminCharFlagResult (pID, pPara->dwTblID, 0, "数値の番号は 0〜127、値は 0〜255 で指定してください。");
			return;
		}
		FlagNumSet (pChar->abyFlagNum, pPara->wNo, pPara->wValue);
		QuestSendStateSelf (pChar);		/* 物語の「いまの目的」も送り直す */
	} else {
		SendAdminCharFlagResult (pID, pPara->dwTblID, 0, "種類は 0（印）か 1（数値）で指定してください。");
		return;
	}
	SendAdminCharFlagResult (pID, pPara->dwTblID, 1, "");
	SendAdminCharFlagInfo (pID, pChar);
}

/* ========================================================================= */
/* 関数名：SendAdminChestInfo												 */
/* 内容：管理者ウィンドウへキャラの開けた宝箱の記録を送る					 */
/* ========================================================================= */
static void SendAdminChestInfo(DWORD pID, PCHARINFO pChar)
{
	PCMDPARA_ADMINCHEST pPara = new CMDPARA_ADMINCHEST;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINCHEST));
	pPara->byCmd	= CMD_ADMINCHEST;
	pPara->byCmdSub	= ADMINCHEST_INFO;
	pPara->dwTblID	= pChar->dwTblID;
	CopyMemory (pPara->abyOpened, pChar->abyChestOpened, sizeof (pPara->abyOpened));
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINCHEST));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：SendAdminChestResult												 */
/* 内容：管理者ウィンドウへ宝箱の記録の変更の結果を送る						 */
/* ========================================================================= */
static void SendAdminChestResult(DWORD pID, DWORD dwTblID, BYTE byResult, LPCSTR pszMsg)
{
	PCMDPARA_ADMINCHEST pPara = new CMDPARA_ADMINCHEST;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINCHEST));
	pPara->byCmd	= CMD_ADMINCHEST;
	pPara->byCmdSub	= ADMINCHEST_RESULT;
	pPara->byResult	= byResult;
	pPara->dwTblID	= dwTblID;
	SboTextCopy (pPara->szMsg, sizeof (pPara->szMsg), pszMsg);
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINCHEST));
	SAFE_DELETE (pPara);
}

/* 全マップの宝箱の行を宝箱番号の順に集める（(宝箱番号, (マップ番号, 行の番号))。消した宝箱は無いので入らない） */
static void AdminChestCollect(std::vector<std::pair<int, std::pair<int, int> > > &aRow)
{
	for (int i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];

		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if ((pMap->aSwap[j].byKind == MAPSWAPKIND_CHEST) && (pMap->aSwap[j].wChestNo < MAX_CHESTNO)) {
				aRow.push_back (std::make_pair ((int)pMap->aSwap[j].wChestNo, std::make_pair (i, j)));
			}
		}
	}
	std::sort (aRow.begin (), aRow.end ());
}

/* ========================================================================= */
/* 関数名：SendAdminChestRows												 */
/* 内容：管理者ウィンドウへ全宝箱の一覧（メモ付き）を ADMINCHEST_ROWMAX 件ずつ送る	 */
/*       0 件でも wTotal=0 の 1 回は送る（クライアントが「無い」と分かるように）	 */
/* ========================================================================= */
static void SendAdminChestRows(DWORD pID)
{
	std::vector<std::pair<int, std::pair<int, int> > > aRow;
	int nTotal, nStart = 0;

	AdminChestCollect (aRow);
	nTotal = (int)aRow.size ();
	do {
		PCMDPARA_ADMINCHEST pPara = new CMDPARA_ADMINCHEST;
		int nCnt = min (ADMINCHEST_ROWMAX, nTotal - nStart);

		ZeroMemory (pPara, sizeof (CMDPARA_ADMINCHEST));
		pPara->byCmd	= CMD_ADMINCHEST;
		pPara->byCmdSub	= ADMINCHEST_ROWS;
		pPara->wTotal	= (WORD)nTotal;
		pPara->wStart	= (WORD)nStart;
		pPara->byRows	= (BYTE)nCnt;
		for (int k = 0; k < nCnt; k ++) {
			PMAPINFO pMap = g_aMapInfo[aRow[nStart + k].second.first];
			const MAPSWAP *pSw = &pMap->aSwap[aRow[nStart + k].second.second];
			PADMINCHESTROW pOut = &pPara->aRow[k];

			pOut->wChestNo	= pSw->wChestNo;
			pOut->wMapNo	= (WORD)aRow[nStart + k].second.first;
			pOut->wX		= pSw->wX;
			pOut->wY		= pSw->wY;
			pOut->wGiveItem	= pSw->wGiveItem;
			SboTextCopy (pOut->szMap, sizeof (pOut->szMap), pMap->pszName ? pMap->pszName : "");
			if ((pSw->wGiveItem != 0) && (pSw->wGiveItem < g_aItemInfo.GetSize ())) {
				SboTextCopy (pOut->szItem, sizeof (pOut->szItem), g_aItemInfo[pSw->wGiveItem]->szName);
			}
			SboTextCopy (pOut->szMemo, sizeof (pOut->szMemo), pSw->szMemo);
		}
		svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINCHEST));
		SAFE_DELETE (pPara);
		nStart += nCnt;
	} while (nStart < nTotal);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINCHEST											 */
/* 内容：受信データ処理(管理者ウィンドウのキャラの開けた宝箱の記録)			 */
/*       管理者だけ（管理者用のコマンドとして受信の入口で断る。ここでも確かめる）*/
/*       GET   : 全宝箱の一覧を ROWS で、キャラの記録を INFO で返す			 */
/*       SET   : 宝箱 1 つを開けた・開けていないにする						 */
/*       CLEAR : 全部開けていないに戻す										 */
/*       変えたら、そのキャラがログイン中なら差し替えを送り直し、宝箱の見た目をすぐ変える。*/
/*       保存は、ふつうのキャラの保存に乗る。相手はログアウト中でもよい		 */
/* ========================================================================= */
void RecvDataProcADMINCHEST(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINCHEST pPara = (PCMDPARA_ADMINCHEST)pData;
	PCHARINFO pChar;
	DWORD pID = s_pIDRecv;

	if (!s_bAdminRecv || (s_dwRecvSize < sizeof (CMDPARA_ADMINCHEST))) {
		return;
	}
	if ((pPara->byCmdSub != ADMINCHEST_GET) && (pPara->byCmdSub != ADMINCHEST_SET) && (pPara->byCmdSub != ADMINCHEST_CLEAR)) {
		return;
	}
	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		SendAdminChestResult (pID, pPara->dwTblID, 0, "キャラが見つかりません。");
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	/* プレイヤーのキャラだけ（NPC・敵は持たない） */
	if ((pChar == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->szAcc[0] == 0)) {
		SendAdminChestResult (pID, pPara->dwTblID, 0, "プレイヤーのキャラを選んでください。");
		return;
	}

	if (pPara->byCmdSub == ADMINCHEST_GET) {
		SendAdminChestRows (pID);
		SendAdminChestInfo (pID, pChar);
		return;
	}

	if (pPara->byCmdSub == ADMINCHEST_SET) {
		std::vector<std::pair<int, std::pair<int, int> > > aRow;
		BOOL bFound = FALSE;

		if ((pPara->wNo >= MAX_CHESTNO) || (pPara->wValue > 1)) {
			SendAdminChestResult (pID, pPara->dwTblID, 0, "宝箱の番号は 0〜2047、値は 0（開けていない）か 1（開けた）で指定してください。");
			return;
		}
		AdminChestCollect (aRow);
		for (size_t i = 0; i < aRow.size (); i ++) {
			if (aRow[i].first == (int)pPara->wNo) {
				bFound = TRUE;
				break;
			}
		}
		if (!bFound) {
			SendAdminChestResult (pID, pPara->dwTblID, 0, "その番号の宝箱はありません（消した宝箱です）。");
			return;
		}
		ChestOpenedSet (pChar, pPara->wNo, pPara->wValue ? TRUE : FALSE);
	} else {
		ZeroMemory (pChar->abyChestOpened, sizeof (pChar->abyChestOpened));
	}
	/* ログイン中なら、差し替え（宝箱の見た目）をその人にだけ送り直す */
	CharViewRefresh (pChar);
	SendAdminChestResult (pID, pPara->dwTblID, 1, "");
	SendAdminChestInfo (pID, pChar);
}

/* ========================================================================= */
/* 関数名：SendAdminSwapInfo												 */
/* 内容：管理者ウィンドウへマップの差し替え表を送る							 */
/* ========================================================================= */
static void SendAdminSwapInfo(DWORD pID, PMAPINFO pMap)
{
	PCMDPARA_ADMINSWAP pPara = new CMDPARA_ADMINSWAP;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINSWAP));
	pPara->byCmd		= CMD_ADMINSWAP;
	pPara->byCmdSub		= ADMINSWAP_INFO;
	pPara->dwMapID		= pMap->dwID;
	pPara->byCount		= pMap->bySwapCount;
	pPara->wNextChest	= (WORD)ChestHighAll ();
	pPara->wNextBox		= (WORD)(RecycleBoxHighAll () + 1);
	for (int i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
		MapSwapWireFromSwap (pMap->aSwap[i], pPara->aRow[i]);
	}
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINSWAP));
	SAFE_DELETE (pPara);
}

/* 管理者ウィンドウへ差し替え表の変更の結果を送る */
static void SendAdminSwapResult(DWORD pID, DWORD dwMapID, BYTE byResult, LPCSTR pszMsg)
{
	PCMDPARA_ADMINSWAP pPara = new CMDPARA_ADMINSWAP;

	ZeroMemory (pPara, sizeof (CMDPARA_ADMINSWAP));
	pPara->byCmd	= CMD_ADMINSWAP;
	pPara->byCmdSub	= ADMINSWAP_RESULT;
	pPara->byResult	= byResult;
	pPara->dwMapID	= dwMapID;
	SboTextCopy (pPara->szMsg, sizeof (pPara->szMsg), pszMsg);
	svrSock.SendTo (pID, (PBYTE)pPara, sizeof (CMDPARA_ADMINSWAP));
	SAFE_DELETE (pPara);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINSWAP											 */
/* 内容：受信データ処理(管理者ウィンドウの「印で変わるマス・宝箱」)			 */
/*       管理者だけ（受信の入口で断る。ここでも確かめる）					 */
/*       GET     : 送ってきた人のいるマップの差し替え表を INFO で返す		 */
/*       ADD/SET/DEL : 1 行足す・直す・消す。表全体を MapSwapBuild（データ API の	 */
/*                 swaps と同じ確かめ・番号の自動振り）に通してから反映する	 */
/*       RESTORE : 表を丸ごと置き換える（「元に戻す」用。同じ確かめを通す）	 */
/*       変えたら、そのマップの全員へ差し替えを送り直し、マップを保存する		 */
/*       （データ API と同じ。ほかのマップの編集のように定期保存を待たない。	 */
/*        番号の覚え wChestHigh・wBoxHigh を失わないため）					 */
/* ========================================================================= */
void RecvDataProcADMINSWAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_ADMINSWAP pPara = (PCMDPARA_ADMINSWAP)pData;
	DWORD pID = s_pIDRecv;
	PCHARINFO pChar = (s_nCharNoRecv >= 0) ? g_aCharInfo[s_nCharNoRecv] : NULL;
	BYTE bySub;
	int nMapNo;
	PMAPINFO pMap;
	MAPINFO w;
	std::vector<MAPSWAPEDITROW> aRow;
	std::string strErr;

	if (!s_bAdminRecv || (s_dwRecvSize < sizeof (CMDPARA_ADMINSWAP))) {
		return;
	}
	bySub = pPara->byCmdSub;
	if ((bySub != ADMINSWAP_GET) && (bySub != ADMINSWAP_ADD) && (bySub != ADMINSWAP_SET) &&
		(bySub != ADMINSWAP_DEL) && (bySub != ADMINSWAP_RESTORE)) {
		return;
	}
	/* 取得は送ってきた人のいるマップ。変更は指定のマップ（人がいる時は、今いるマップと同じでなければ断る） */
	if ((bySub == ADMINSWAP_GET) && pChar) {
		nMapNo = GetMapNo (pChar->dwMapID);
	} else {
		nMapNo = GetMapNo (pPara->dwMapID);
	}
	if (nMapNo < 0) {
		if (pID) SendAdminSwapResult (pID, pPara->dwMapID, 0, "マップが見つかりません。");
		return;
	}
	pMap = g_aMapInfo[nMapNo];
	if (bySub == ADMINSWAP_GET) {
		if (pID) SendAdminSwapInfo (pID, pMap);
		return;
	}
	if (pChar && (bySub != ADMINSWAP_RESTORE) && (pChar->dwMapID != pMap->dwID)) {
		SendAdminSwapResult (pID, pPara->dwMapID, 0, "マップが変わりました。取り直してから操作してください。");
		SendAdminSwapInfo (pID, g_aMapInfo[GetMapNo (pChar->dwMapID) >= 0 ? GetMapNo (pChar->dwMapID) : nMapNo]);
		return;
	}

	/* 変更後の表を入力の形で作る */
	if (bySub == ADMINSWAP_RESTORE) {
		if (pPara->byCount > MAX_MAPSWAP) {
			if (pID) SendAdminSwapResult (pID, pMap->dwID, 0, "行が多すぎます。");
			return;
		}
		aRow.resize (pPara->byCount);
		for (int i = 0; i < pPara->byCount; i ++) {
			MapSwapEditFromWire (pPara->aRow[i], aRow[i]);
		}
	} else {
		for (int i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
			MAPSWAPEDITROW Row;
			MapSwapToEdit (pMap->aSwap[i], Row);
			aRow.push_back (Row);
		}
		if (bySub == ADMINSWAP_ADD) {
			MAPSWAPEDITROW Row;
			MapSwapEditFromWire (pPara->Row, Row);
			aRow.push_back (Row);
		} else {
			if (pPara->wIndex >= aRow.size ()) {
				if (pID) SendAdminSwapResult (pID, pMap->dwID, 0, "その行はありません。取り直してから操作してください。");
				return;
			}
			if (bySub == ADMINSWAP_DEL) {
				aRow.erase (aRow.begin () + pPara->wIndex);
			} else {
				MapSwapEditFromWire (pPara->Row, aRow[pPara->wIndex]);
			}
		}
	}

	w = *pMap;
	if (!MapSwapBuild (nMapNo, w, aRow, "行", strErr)) {
		if (pID) SendAdminSwapResult (pID, pMap->dwID, 0, strErr.c_str ());
		return;
	}
	if (MapSwapChanged (*pMap, w)) {
		MapSwapSet (pMap, w);
		MapSwapSendMap (pMap);
		SaveMapInfo ();
	}
	if (pID) {
		SendAdminSwapResult (pID, pMap->dwID, 1, "");
		SendAdminSwapInfo (pID, pMap);
	}
}

/* ========================================================================= */
/* 関数名：GetResurrectionEfectID											 */
/* 内容：蘇生のエフェクト番号を特技のデータから探す							 */
/*       効果種別が「蘇生」で相手側のエフェクトがある最初の特技のもの。	 */
/*       無ければ蘇生アイテムと同じ 14 番									 */
/* ========================================================================= */
static BYTE GetResurrectionEfectID(void)
{
	int i, nCount;
	PMAGICINFO pInfo;

	nCount = g_aMagicInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = g_aMagicInfo[i];
		if ((pInfo == NULL) || (pInfo->byType != MAGICTYPE_RESURRECTION)) {
			continue;
		}
		if (pInfo->byTargetEfectID) {
			return pInfo->byTargetEfectID;
		}
	}
	return 14;
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINVIEWALL											 */
/* 内容：受信データ処理(管理者ウィンドウの「印で隠れたNPCも表示」の切り替え) */
/*       送った管理者本人のメモリ上のフラグだけを変える（保存しない）。		 */
/*       変えたら、いるマップの条件付きNPCを出し直す						 */
/* ========================================================================= */
void RecvDataProcADMINVIEWALL(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	BYTE byNew;

	if (s_dwRecvSize < sizeof (CMDPARA_DATA1)) {
		return;
	}
	if (s_nCharNoRecv < 0) {
		return;
	}
	pChar = g_aCharInfo[s_nCharNoRecv];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byAdminLV == ADMINLV_USER)) {
		return;
	}
	byNew = (pPara->byData != 0) ? 1 : 0;
	if (pChar->byViewAllNpc == byNew) {
		return;
	}
	pChar->byViewAllNpc = byNew;
	CharViewRefresh (pChar);
}

/* ========================================================================= */
/* 関数名：RecvDataProcADMINRESURRECT										 */
/* 内容：受信データ処理(管理者ウィンドウからキャラを蘇生)					 */
/*       倒れていればHP満タンで生き返らせ、倒れていなければHPを満タンにする */
/* ========================================================================= */
void RecvDataProcADMINRESURRECT(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;
	PCHARINFO pChar;
	BOOL bDead;
	long lTmp;
	char szTmp[128];

	if (s_dwRecvSize < sizeof (CMDPARA_DATA1)) {
		return;
	}
	if (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0)) {
		return;
	}
	/* ログインしていない・マップにいないキャラ（保存されているだけ）は相手にしない */
	if ((pChar->byGameState != GAMESTATE_NPC) && (pChar->byGameState != GAMESTATE_MOVE)) {
		return;
	}
	if (GetMapNo (pChar->dwMapID) < 0) {
		return;
	}

	bDead = ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0));
	if (!bDead && (pChar->wHP >= pChar->wMaxHP)) {
		if (s_pIDRecv) {
			wsprintf (szTmp, "%sは倒れていません（HPも満タンです）。", pChar->szName);
			SendCmdSYSMSG (s_pIDRecv, szTmp, SPEECHCMD_BATTLE1);
		}
		return;
	}

	lTmp = (long)pChar->wMaxHP - (long)pChar->wHP;
	pChar->wHP = pChar->wMaxHP;
	if (pChar->wHP == 0) {
		pChar->wHP = 1;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		pChar->byCharState = CHARSTATE_MOVE;
	}

	/* エフェクト発動（蘇生の特技と同じもの） */
	pChar->byEfect = GetResurrectionEfectID ();
	SendCmdEFECTCHG (pChar->pID, pChar->dwTblID, pChar->byEfect);
	pChar->byEfect = 0;

	/* 文字エフェクト発動（回復した量） */
	if (lTmp > 0) {
		wsprintf (szTmp, "%d", lTmp);
		SendCmdADDCHAREFC (pChar->dwTblID, RGB (139, 213, 255), szTmp);
	}

	/* HPを通知（本人は倒れた窓が閉じる） */
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pChar->dwTblID);

	if (bDead) {
		/* 本人と管理者に「生き返りました」 */
		if (pChar->byGameState == GAMESTATE_MOVE) {
			SendCmdSYSMSG2 (pChar->pID, SYSMSG_RESURRECTION, pChar->pID, 0);
		}
		if (s_pIDRecv && (s_pIDRecv != pChar->pID)) {
			SendCmdSYSMSG2 (s_pIDRecv, SYSMSG_RESURRECTION, pChar->pID, 0);
		}
	} else if (s_pIDRecv) {
		wsprintf (szTmp, "%sのHPを満タンにしました。", pChar->szName);
		SendCmdSYSMSG (s_pIDRecv, szTmp, SPEECHCMD_BATTLE1);
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
			(byCmd == CMD_WEBACCOUNT) || (byCmd == CMD_ADMINUNDO) || (byCmd == CMD_ADMINSPACE) || (byCmd == CMD_ADMINVIEWALL) || (byCmd == CMD_REQ_GUARD) || (byCmd == CMD_RECIPEMAKE) || (byCmd == CMD_APPRAISE) || (byCmd == CMD_BATTLEMODE) || (byCmd == CMD_QUEST) || (byCmd == CMD_SORTITEM) || (byCmd == CMD_LEAVEITEMDISPOSE) || (byCmd == CMD_MAGICKEY) || (byCmd == CMD_TALKANSWER) || ((byCmd >= CMD_MARKETLIST) && (byCmd <= CMD_MARKETGOLDGET)) || ((byCmd >= CMD_RECYCLELIST) && (byCmd <= CMD_RECYCLECLOSE))) {
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

	if ((wItem == 0) || !ItemIDValid (wItem)) {
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
/*       pnTarget が NULL でない時（鍛える・付け替える）は、手持ちの *pnTarget 番目が */
/*       対象の装備なので材料に数えず、できた物は渡さない。材料を消して持ち物が詰まった */
/*       後の対象の位置を *pnTarget に入れて返す（装備中の添字も DelCharItem が直す）	 */
/* ========================================================================= */
static BYTE RecipeMake(PCHARINFO pChar, PRECIPEINFO pRecipe, int *pnToInv, int *pnToLeave, int *pnTarget = NULL)
{
	PLEAVEINFO pLeave = NULL;
	BOOL abyInvUsed[MAX_MYITEM], abyImpUsed[MAX_IMPORTANT];
	int anLeaveTake[MAX_LEAVEITEM];
	WORD awItemBak[MAX_MYITEM], awEquBak[10], awImpBak[MAX_IMPORTANT];
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
	ZeroMemory (abyImpUsed, sizeof (abyImpUsed));
	ZeroMemory (anLeaveTake, sizeof (anLeaveTake));
	for (k = 0; k < MAX_RECIPEMAT; k ++) {
		WORD wMat = pRecipe->awMatItem[k];

		nNeed = pRecipe->abyMatCount[k];
		if ((wMat == 0) || (nNeed == 0)) {
			continue;
		}
		for (i = 0; (i < MAX_MYITEM) && (nNeed > 0); i ++) {
			if (!abyInvUsed[i] && (pChar->waItem[i] == wMat) && RecipeIsUsableItem (pChar, i) && (!pnTarget || (i != *pnTarget))) {
				abyInvUsed[i] = TRUE;
				nNeed --;
			}
		}
		/* 大事な物の欄にある分（捨てられない印の品は使えない） */
		for (i = 0; (i < MAX_IMPORTANT) && (nNeed > 0); i ++) {
			if (!abyImpUsed[i] && (pChar->waImportant[i] == wMat) && !(g_aItemInfo[wMat]->byFlg & ITEMFLG_DROP)) {
				abyImpUsed[i] = TRUE;
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
	memcpy (awImpBak, pChar->waImportant, sizeof (awImpBak));
	dwLeaveIDBak = pChar->dwLeaveID;
	ZeroMemory (&LeaveBak, sizeof (LeaveBak));
	if (pLeave) {
		LeaveBak = *pLeave;
	}

	/* 材料を使う：大事な物の欄（後ろから消して詰める） */
	for (i = MAX_IMPORTANT - 1; i >= 0; i --) {
		if (abyImpUsed[i]) {
			DelImportantItem (pChar, pChar->waImportant[i]);
		}
	}

	/* 材料を使う：持ち物（後ろから消して詰める） */
	for (i = MAX_MYITEM - 1; i >= 0; i --) {
		if (abyInvUsed[i]) {
			DelCharItem (pChar->dwTblID, (BYTE)i);
			if (pnTarget && (i < *pnTarget)) {
				(*pnTarget) --;
			}
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

	/* できた物を渡す：持ち物へ、いっぱいなら預かり所へ（鍛える・付け替えるは対象の装備を置き換えるので渡さない） */
	for (i = 0; !pnTarget && (i < pRecipe->byCount); i ++) {
		/* 大事な物の欄に入る品は、欄に空きがなければ作れない（預かり所へは送らない） */
		if (CanAddCharItem (pChar->dwTblID, pRecipe->wItem)) {
			AddCharItem (pChar->dwTblID, pRecipe->wItem);
			nToInv ++;
			continue;
		}
		if (IsImportantItem (pRecipe->wItem) || !AddLeaveItem (pChar, pRecipe->wItem)) {
			/* 預かり所もいっぱい：何も変えなかったことにする */
			memcpy (pChar->waItem, awItemBak, sizeof (awItemBak));
			memcpy (pChar->waEqu, awEquBak, sizeof (awEquBak));
			memcpy (pChar->waImportant, awImpBak, sizeof (awImpBak));
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
/* 関数名：RecvDataProcQUEST												 */
/* 内容：受信データ処理(ギルドの仕事依頼：一覧・受ける・取り消す・渡す)		 */
/* ========================================================================= */
void RecvDataProcQUEST(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_QUEST pPara = (PCMDPARA_QUEST)pData;
	PCHARINFO pChar;

	if (s_dwRecvSize < sizeof (CMDPARA_QUEST)) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}
	QuestRecv (pChar, pPara);
}

/* ========================================================================= */
/* 関数名：ItemSortKanjiKana												 */
/* 内容：漢字1文字の読みの頭のかな（清音のひらがな）を返す					 */
/*       JIS 第一水準漢字は読み（音読み）の順に並んでいるので、Shift_JIS の	 */
/*       番号から行の頭を引く。第二水準など分からない時は 0					 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
static unsigned ItemSortKanjiKana(unsigned c, unsigned *pnSjis)
{
	/* 第一水準の、読みの頭のかなが変わる所（表は pykakasi の読みから作った） */
	static const struct { unsigned short wSjis; unsigned short wKana; } s_aTbl[] = {
		{ 0x889f, 0x3042 },		/* あ 亜 */
		{ 0x88c8, 0x3044 },		/* い 以 */
		{ 0x8946, 0x3046 },		/* う 宇 */
		{ 0x8960, 0x3048 },		/* え 荏 */
		{ 0x8997, 0x304A },		/* お 於 */
		{ 0x89bb, 0x304B },		/* か 化 */
		{ 0x8ae9, 0x304D },		/* き 企 */
		{ 0x8be4, 0x304F },		/* く 倶 */
		{ 0x8c54, 0x3051 },		/* け 卦 */
		{ 0x8cc1, 0x3053 },		/* こ 乎 */
		{ 0x8db1, 0x3055 },		/* さ 些 */
		{ 0x8e64, 0x3057 },		/* し 仕 */
		{ 0x9079, 0x3059 },		/* す 笥 */
		{ 0x90a3, 0x305B },		/* せ 瀬 */
		{ 0x9158, 0x305D },		/* そ 噌 */
		{ 0x91bd, 0x305F },		/* た 多 */
		{ 0x926d, 0x3061 },		/* ち 知 */
		{ 0x92c3, 0x3064 },		/* つ 津 */
		{ 0x92e0, 0x3066 },		/* て 亭 */
		{ 0x9366, 0x3068 },		/* と 吐 */
		{ 0x93de, 0x306A },		/* な 奈 */
		{ 0x93f1, 0x306B },		/* に 二 */
		{ 0x9448, 0x306D },		/* ね 禰 */
		{ 0x9454, 0x306E },		/* の 乃 */
		{ 0x9463, 0x306F },		/* は 把 */
		{ 0x94d9, 0x3072 },		/* ひ 匪 */
		{ 0x9573, 0x3075 },		/* ふ 不 */
		{ 0x95b9, 0x3078 },		/* へ 併 */
		{ 0x95db, 0x307B },		/* ほ 保 */
		{ 0x9680, 0x307E },		/* ま 摩 */
		{ 0x96a3, 0x307F },		/* み 魅 */
		{ 0x96b3, 0x3080 },		/* む 無 */
		{ 0x96bb, 0x3081 },		/* め 冥 */
		{ 0x96cc, 0x3082 },		/* も 摸 */
		{ 0x96e7, 0x3084 },		/* や 也 */
		{ 0x96f9, 0x3086 },		/* ゆ 愉 */
		{ 0x975d, 0x3088 },		/* よ 余 */
		{ 0x9785, 0x3089 },		/* ら 羅 */
		{ 0x9798, 0x308A },		/* り 利 */
		{ 0x97da, 0x308B },		/* る 瑠 */
		{ 0x97df, 0x308C },		/* れ 令 */
		{ 0x9843, 0x308D },		/* ろ 呂 */
		{ 0x9860, 0x308F },		/* わ 倭 */
	};
	char szUtf8[8], szSjis[8];
	unsigned nSjis, wKana;
	int i;

	*pnSjis = 0;
	if ((c < 0x80) || (c > 0xFFFF)) {
		return 0;
	}
	/* 1文字だけ UTF-8 にして Shift_JIS へ */
	if (c < 0x800) {
		szUtf8[0] = (char)(0xC0 | (c >> 6));
		szUtf8[1] = (char)(0x80 | (c & 0x3F));
		szUtf8[2] = 0;
	} else {
		szUtf8[0] = (char)(0xE0 | (c >> 12));
		szUtf8[1] = (char)(0x80 | ((c >> 6) & 0x3F));
		szUtf8[2] = (char)(0x80 | (c & 0x3F));
		szUtf8[3] = 0;
	}
	ZeroMemory (szSjis, sizeof (szSjis));
	W32Utf8ToSjis (szUtf8, szSjis, sizeof (szSjis));
	if ((szSjis[0] == 0) || (szSjis[1] == 0) || (szSjis[2] != 0)) {
		return 0;
	}
	nSjis = ((unsigned)(BYTE)szSjis[0] << 8) | (BYTE)szSjis[1];
	*pnSjis = nSjis;
	if ((nSjis < s_aTbl[0].wSjis) || (nSjis > 0x9872)) {
		return 0;
	}
	wKana = s_aTbl[0].wKana;
	for (i = 0; i < (int)(sizeof (s_aTbl) / sizeof (s_aTbl[0])); i ++) {
		if (nSjis < s_aTbl[i].wSjis) {
			break;
		}
		wKana = s_aTbl[i].wKana;
	}
	return wKana;
}

/* ========================================================================= */
/* 関数名：ItemSortPlainKana												 */
/* 内容：ひらがなの濁点・半濁点・小さい字を外した字を返す（50音の1段目の比較用） */
/* 日付：2026/11/11															 */
/* ========================================================================= */
static unsigned ItemSortPlainKana(unsigned c)
{
	/* ぁ(3041)〜ゖ(3096) の清音・大きい字 */
	static const unsigned short s_aPlain[] = {
		0x3042, 0x3042, 0x3044, 0x3044, 0x3046, 0x3046, 0x3048, 0x3048, 0x304A, 0x304A,	/* ぁあぃいぅうぇえぉお */
		0x304B, 0x304B, 0x304D, 0x304D, 0x304F, 0x304F, 0x3051, 0x3051, 0x3053, 0x3053,	/* かがきぎくぐけげこご */
		0x3055, 0x3055, 0x3057, 0x3057, 0x3059, 0x3059, 0x305B, 0x305B, 0x305D, 0x305D,	/* さざしじすずせぜそぞ */
		0x305F, 0x305F, 0x3061, 0x3061, 0x3064, 0x3064, 0x3064, 0x3066, 0x3066, 0x3068, 0x3068,	/* ただちぢっつづてでとど */
		0x306A, 0x306B, 0x306C, 0x306D, 0x306E,											/* なにぬねの */
		0x306F, 0x306F, 0x306F, 0x3072, 0x3072, 0x3072, 0x3075, 0x3075, 0x3075,			/* はばぱひびぴふぶぷ */
		0x3078, 0x3078, 0x3078, 0x307B, 0x307B, 0x307B,									/* へべぺほぼぽ */
		0x307E, 0x307F, 0x3080, 0x3081, 0x3082,											/* まみむめも */
		0x3084, 0x3084, 0x3086, 0x3086, 0x3088, 0x3088,									/* ゃやゅゆょよ */
		0x3089, 0x308A, 0x308B, 0x308C, 0x308D,											/* らりるれろ */
		0x308F, 0x308F, 0x3090, 0x3091, 0x3092, 0x3093, 0x3046, 0x304B, 0x3051,			/* ゎわゐゑをんゔゕゖ */
	};
	if ((c >= 0x3041) && (c <= 0x3096)) {
		return s_aPlain[c - 0x3041];
	}
	return c;
}

/* ========================================================================= */
/* 関数名：ItemNameSortKey													 */
/* 内容：アイテム名から50音順の並べ替えキーを作る							 */
/*       UTF-8 を符号点の列にして、カタカナ・半角カナはひらがなへ、		 */
/*       全角英数記号は半角へ、英字は小文字へそろえる						 */
/*       キーは「1段目（清音のかな。漢字は読みの頭のかな）」0「2段目（濁点	 */
/*       などを含む字。漢字は Shift_JIS の番号＝読みの順）」の並び。		 */
/*       1段目で比べて同じなら2段目で比べることになる						 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
void ItemNameSortKey(LPCSTR pszName, std::vector<unsigned> &aKey)
{
	/* 半角カナ(U+FF66〜U+FF9D)→ひらがな */
	static const unsigned s_awHalfKana[0x9D - 0x66 + 1] = {
		0x3092,														/* ヲ */
		0x3041, 0x3043, 0x3045, 0x3047, 0x3049,						/* ｧｨｩｪｫ */
		0x3083, 0x3085, 0x3087, 0x3063,								/* ｬｭｮｯ */
		0x30FC,														/* ｰ */
		0x3042, 0x3044, 0x3046, 0x3048, 0x304A,						/* ｱｲｳｴｵ */
		0x304B, 0x304D, 0x304F, 0x3051, 0x3053,						/* ｶｷｸｹｺ */
		0x3055, 0x3057, 0x3059, 0x305B, 0x305D,						/* ｻｼｽｾｿ */
		0x305F, 0x3061, 0x3064, 0x3066, 0x3068,						/* ﾀﾁﾂﾃﾄ */
		0x306A, 0x306B, 0x306C, 0x306D, 0x306E,						/* ﾅﾆﾇﾈﾉ */
		0x306F, 0x3072, 0x3075, 0x3078, 0x307B,						/* ﾊﾋﾌﾍﾎ */
		0x307E, 0x307F, 0x3080, 0x3081, 0x3082,						/* ﾏﾐﾑﾒﾓ */
		0x3084, 0x3086, 0x3088,										/* ﾔﾕﾖ */
		0x3089, 0x308A, 0x308B, 0x308C, 0x308D,						/* ﾗﾘﾙﾚﾛ */
		0x308F,														/* ﾜ */
		0x3093,														/* ﾝ */
	};
	const unsigned char *p = (const unsigned char *)pszName;
	std::vector<unsigned> aCode, aNorm;
	size_t i;

	aKey.clear ();
	if (pszName == NULL) {
		return;
	}

	/* UTF-8 を符号点に直す（壊れた並びは1バイトずつそのまま） */
	while (*p) {
		unsigned c = *p;
		int nLen = 1;
		if ((c >= 0xC2) && (c <= 0xDF)) {
			nLen = 2;
			c &= 0x1F;
		} else if ((c >= 0xE0) && (c <= 0xEF)) {
			nLen = 3;
			c &= 0x0F;
		} else if ((c >= 0xF0) && (c <= 0xF4)) {
			nLen = 4;
			c &= 0x07;
		}
		for (int k = 1; k < nLen; k ++) {
			if ((p[k] & 0xC0) != 0x80) {
				nLen = 1;
				c = *p;
				break;
			}
			c = (c << 6) | (p[k] & 0x3F);
		}
		aCode.push_back (c);
		p += nLen;
	}

	for (i = 0; i < aCode.size (); i ++) {
		unsigned c = aCode[i];
		if ((c >= 0x30A1) && (c <= 0x30F6)) {
			/* カタカナ→ひらがな */
			c -= 0x60;
		} else if ((c >= 0xFF66) && (c <= 0xFF9D)) {
			/* 半角カナ→ひらがな（後ろの濁点・半濁点もまとめる） */
			unsigned cNext = (i + 1 < aCode.size ()) ? aCode[i + 1] : 0;
			c = s_awHalfKana[c - 0xFF66];
			if (cNext == 0xFF9E) {
				if (c == 0x3046) {
					c = 0x3094;
					i ++;
				} else if (((c >= 0x304B) && (c <= 0x3053)) || ((c >= 0x3055) && (c <= 0x305D)) ||
						(c == 0x305F) || (c == 0x3061) || (c == 0x3064) || (c == 0x3066) || (c == 0x3068) ||
						(c == 0x306F) || (c == 0x3072) || (c == 0x3075) || (c == 0x3078) || (c == 0x307B)) {
					c ++;
					i ++;
				}
			} else if (cNext == 0xFF9F) {
				if ((c == 0x306F) || (c == 0x3072) || (c == 0x3075) || (c == 0x3078) || (c == 0x307B)) {
					c += 2;
					i ++;
				}
			}
		} else if ((c >= 0xFF01) && (c <= 0xFF5E)) {
			/* 全角英数記号→ASCII */
			c -= 0xFEE0;
		}
		if ((c >= 'A') && (c <= 'Z')) {
			c += 'a' - 'A';
		}
		aNorm.push_back (c);
	}

	/* 1段目：清音のかな（漢字は読みの頭のかな、分からない漢字はかなの後ろ） */
	for (i = 0; i < aNorm.size (); i ++) {
		unsigned c = aNorm[i], nSjis;
		if ((c >= 0x3041) && (c <= 0x3096)) {
			aKey.push_back (ItemSortPlainKana (c));
		} else if ((c >= 0x3400) && (c <= 0x9FFF)) {
			unsigned wKana = ItemSortKanjiKana (c, &nSjis);
			aKey.push_back (wKana ? wKana : (0x10000 + c));
		} else {
			aKey.push_back (c);
		}
	}
	/* 区切り（どの字より小さい） */
	aKey.push_back (0);
	/* 2段目：濁点なども含めた字（漢字は Shift_JIS の番号で読みの順に） */
	for (i = 0; i < aNorm.size (); i ++) {
		unsigned c = aNorm[i], nSjis;
		if ((c >= 0x3400) && (c <= 0x9FFF)) {
			ItemSortKanjiKana (c, &nSjis);
			aKey.push_back (nSjis ? (0x20000 + nSjis) : (0x30000 + c));
		} else {
			aKey.push_back (c);
		}
	}
}

/* ========================================================================= */
/* 関数名：ItemSortGroup													 */
/* 内容：種類順の並べ替えで使う、種類のグループ番号							 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
static int ItemSortGroup(PITEMINFO pItem)
{
	switch (pItem->byType) {
	case ITEMTYPE_HPUP:
	case ITEMTYPE_MANAUP:
	case ITEMTYPE_HPMANAUP:
	case ITEMTYPE_RESURRECTION:	return 0;		/* 回復 */
	case ITEMTYPE_RURA:			return 1;		/* 移動 */
	case ITEMTYPE_USEMAGIC:		return 2;		/* 特技発動 */
	case ITEMTYPE_SEED:			return 3;		/* 種 */
	case ITEMTYPE_SAKE:
	case ITEMTYPE_LIGHT:		return 4;		/* 酒・明かり */
	case ITEMTYPE_MAGIC:		return 5;		/* 特技の書 */
	case ITEMTYPE_WEAPON:		return 6;		/* 武器 */
	case ITEMTYPE_SHILD:		return 7;		/* 盾 */
	case ITEMTYPE_ARMOR:		return 8;		/* 鎧 */
	case ITEMTYPE_HELMET:		return 9;		/* 兜 */
	case ITEMTYPE_EQU_HAND:		return 10;		/* 小手 */
	case ITEMTYPE_EQU_LEG:		return 11;		/* 靴 */
	case ITEMTYPE_EQU_ACC1:		return 12;		/* 首飾り */
	case ITEMTYPE_EQU_ACC2:
	case ITEMTYPE_EQU_ACC3:		return 13;		/* 指輪 */
	}
	return 14;								/* その他 */
}

/* 並べ替えの1件分 */
typedef struct _SORTITEMENTRY {
	int					nOld;										/* 並べ替える前の番号 */
	WORD				wItemID;									/* アイテムID */
	int					nGroup;										/* 種類のグループ番号 */
	std::vector<unsigned>	aKey;									/* 50音キー */
} SORTITEMENTRY;

/* 種類順の比較（グループ→50音キー→アイテムID） */
static bool SortItemLessType(const SORTITEMENTRY &a, const SORTITEMENTRY &b)
{
	if (a.nGroup != b.nGroup) {
		return a.nGroup < b.nGroup;
	}
	if (a.aKey != b.aKey) {
		return a.aKey < b.aKey;
	}
	return a.wItemID < b.wItemID;
}

/* 50音順の比較（50音キー→アイテムID） */
static bool SortItemLessName(const SORTITEMENTRY &a, const SORTITEMENTRY &b)
{
	if (a.aKey != b.aKey) {
		return a.aKey < b.aKey;
	}
	return a.wItemID < b.wItemID;
}

/* ========================================================================= */
/* 関数名：SortItemList														 */
/* 内容：アイテムの並びを種類順・50音順に並べ替える（安定ソート）			 */
/*       pwaItem の先頭 nCount 個を並べ替え、pbyCount（NULL可。預かり所の	 */
/*       個数）も同じように動かす。pnNewToOld（NULL可）には、新しい番号→	 */
/*       古い番号の対応を nCount 個返す										 */
/*       もうその順に並んでいたら逆の順（降順）にする（続けて選ぶと切り替わる） */
/* 日付：2026/11/11															 */
/* ========================================================================= */
void SortItemList(WORD *pwaItem, BYTE *pbyCount, int nCount, BYTE byMode, int *pnNewToOld)
{
	std::vector<SORTITEMENTRY> aEntry;
	std::vector<WORD> aItem;
	std::vector<BYTE> aCount;
	int i;

	if ((pwaItem == NULL) || (nCount <= 0)) {
		return;
	}
	if ((byMode != SORTITEMMODE_TYPE) && (byMode != SORTITEMMODE_NAME)) {
		return;
	}

	aEntry.resize (nCount);
	for (i = 0; i < nCount; i ++) {
		PITEMINFO pItem = NULL;
		aEntry[i].nOld		= i;
		aEntry[i].wItemID	= pwaItem[i];
		aEntry[i].nGroup	= 14;
		if (ItemIDValid (pwaItem[i])) {
			pItem = g_aItemInfo[pwaItem[i]];
		}
		if (pItem) {
			aEntry[i].nGroup = ItemSortGroup (pItem);
			ItemNameSortKey (pItem->szName, aEntry[i].aKey);
		}
	}
	bool (*pfnLess)(const SORTITEMENTRY &, const SORTITEMENTRY &) =
			(byMode == SORTITEMMODE_TYPE) ? SortItemLessType : SortItemLessName;
	std::stable_sort (aEntry.begin (), aEntry.end (), pfnLess);

	/* もうこの順に並んでいた（続けて同じ並べ替えを選んだ）なら、逆の順にする */
	for (i = 0; i < nCount; i ++) {
		if (aEntry[i].nOld != i) {
			break;
		}
	}
	if (i == nCount) {
		std::stable_sort (aEntry.begin (), aEntry.end (),
				[pfnLess](const SORTITEMENTRY &a, const SORTITEMENTRY &b) { return pfnLess (b, a); });
	}

	aItem.assign (pwaItem, pwaItem + nCount);
	if (pbyCount) {
		aCount.assign (pbyCount, pbyCount + nCount);
	}
	for (i = 0; i < nCount; i ++) {
		pwaItem[i] = aItem[aEntry[i].nOld];
		if (pbyCount) {
			pbyCount[i] = aCount[aEntry[i].nOld];
		}
		if (pnNewToOld) {
			pnNewToOld[i] = aEntry[i].nOld;
		}
	}
}

/* 特技の並べ替えの1件分 */
typedef struct _SORTMAGICENTRY {
	int					nOld;										/* 並べ替える前の番号 */
	BYTE				byMagicID;									/* 特技ID */
	int					nScene;										/* 使える場面の順（ふだん→いつでも→戦闘中） */
	int					nType;										/* 効果種別 */
	int					nMana;										/* 消費Mana */
	std::vector<unsigned>	aKey;									/* 50音キー */
} SORTMAGICENTRY;

/* 特技の種類順の比較（使える場面→効果種別→消費Mana→50音キー→特技ID） */
static bool SortMagicLessType(const SORTMAGICENTRY &a, const SORTMAGICENTRY &b)
{
	if (a.nScene != b.nScene) {
		return a.nScene < b.nScene;
	}
	if (a.nType != b.nType) {
		return a.nType < b.nType;
	}
	if (a.nMana != b.nMana) {
		return a.nMana < b.nMana;
	}
	if (a.aKey != b.aKey) {
		return a.aKey < b.aKey;
	}
	return a.byMagicID < b.byMagicID;
}

/* 特技の50音順の比較（50音キー→特技ID） */
static bool SortMagicLessName(const SORTMAGICENTRY &a, const SORTMAGICENTRY &b)
{
	if (a.aKey != b.aKey) {
		return a.aKey < b.aKey;
	}
	return a.byMagicID < b.byMagicID;
}

/* ========================================================================= */
/* 関数名：SortMagicList													 */
/* 内容：覚えている特技の並びを入れ替え・種類順・50音順に並べ替える			 */
/*       空き(0)は後ろに詰める。入れ替えは詰めた後の番号(byNo1,byNo2)で		 */
/*       もうその順に並んでいたら逆の順（降順）にする（道具の並べ替えと同じ） */
/*       並べ替えたら TRUE														 */
/* 日付：2026/11/17															 */
/* ========================================================================= */
BOOL SortMagicList(BYTE *pabyMagic, BYTE byMode, BYTE byNo1, BYTE byNo2)
{
	std::vector<SORTMAGICENTRY> aEntry;
	int i, nCount;

	if (pabyMagic == NULL) {
		return FALSE;
	}
	/* 空きを後ろに詰める */
	nCount = 0;
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		BYTE byMagic = pabyMagic[i];
		pabyMagic[i] = 0;
		if (byMagic) {
			pabyMagic[nCount ++] = byMagic;
		}
	}

	if (byMode == SORTITEMMODE_SWAP) {
		if ((byNo1 >= nCount) || (byNo2 >= nCount) || (byNo1 == byNo2)) {
			return FALSE;
		}
		BYTE byTmp = pabyMagic[byNo1];
		pabyMagic[byNo1] = pabyMagic[byNo2];
		pabyMagic[byNo2] = byTmp;
		return TRUE;
	}
	if ((byMode != SORTITEMMODE_TYPE) && (byMode != SORTITEMMODE_NAME)) {
		return FALSE;
	}
	if (nCount <= 1) {
		return FALSE;
	}

	aEntry.resize (nCount);
	for (i = 0; i < nCount; i ++) {
		PMAGICINFO pInfo = NULL;
		aEntry[i].nOld		= i;
		aEntry[i].byMagicID	= pabyMagic[i];
		aEntry[i].nScene	= 3;
		aEntry[i].nType		= 255;
		aEntry[i].nMana		= 65535;
		if ((int)pabyMagic[i] < (int)g_aMagicInfo.GetSize ()) {
			pInfo = g_aMagicInfo[pabyMagic[i]];
		}
		if (pInfo) {
			switch (pInfo->byTypeInfoID) {
			case MAGICUSETYPE_WALK:			aEntry[i].nScene = 0;	break;	/* ふだん */
			case MAGICUSETYPE_WALKBATTLE:	aEntry[i].nScene = 1;	break;	/* いつでも */
			case MAGICUSETYPE_BATTLE:		aEntry[i].nScene = 2;	break;	/* 戦闘中 */
			}
			aEntry[i].nType	= pInfo->byType;
			aEntry[i].nMana	= pInfo->wMana;
			ItemNameSortKey (pInfo->szName, aEntry[i].aKey);
		}
	}
	bool (*pfnLess)(const SORTMAGICENTRY &, const SORTMAGICENTRY &) =
			(byMode == SORTITEMMODE_TYPE) ? SortMagicLessType : SortMagicLessName;
	std::stable_sort (aEntry.begin (), aEntry.end (), pfnLess);

	/* もうこの順に並んでいた（続けて同じ並べ替えを選んだ）なら、逆の順にする */
	for (i = 0; i < nCount; i ++) {
		if (aEntry[i].nOld != i) {
			break;
		}
	}
	if (i == nCount) {
		std::stable_sort (aEntry.begin (), aEntry.end (),
				[pfnLess](const SORTMAGICENTRY &a, const SORTMAGICENTRY &b) { return pfnLess (b, a); });
	}
	for (i = 0; i < nCount; i ++) {
		pabyMagic[i] = aEntry[i].byMagicID;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：RecvDataProcSORTITEM												 */
/* 内容：受信データ処理(道具欄・預かり所の並べ替え)							 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
void RecvDataProcSORTITEM(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_SORTITEM pPara = (PCMDPARA_SORTITEM)pData;
	PCHARINFO pChar;
	int i, j, nCount, anNewToOld[MAX_LEAVEITEM];

	if (s_dwRecvSize < sizeof (CMDPARA_SORTITEM)) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}
	if ((pPara->byMode != SORTITEMMODE_SWAP) && (pPara->byMode != SORTITEMMODE_TYPE) &&
			(pPara->byMode != SORTITEMMODE_NAME)) {
		return;
	}

	if (pPara->byTarget == SORTITEMTARGET_MYITEM) {
		/* 取り引き中は取り引きの欄が手持ちの番号で動くので、並べ替えない */
		if (s_mapTrade.find (pChar->dwTblID) != s_mapTrade.end ()) {
			return;
		}
		nCount = GetItemCount (pChar->dwTblID);
		for (i = 0; i < nCount; i ++) {
			anNewToOld[i] = i;
		}
		if (pPara->byMode == SORTITEMMODE_SWAP) {
			if ((pPara->byNo1 >= nCount) || (pPara->byNo2 >= nCount) || (pPara->byNo1 == pPara->byNo2)) {
				return;
			}
			WORD wTmp = pChar->waItem[pPara->byNo1];
			pChar->waItem[pPara->byNo1] = pChar->waItem[pPara->byNo2];
			pChar->waItem[pPara->byNo2] = wTmp;
			anNewToOld[pPara->byNo1] = pPara->byNo2;
			anNewToOld[pPara->byNo2] = pPara->byNo1;
		} else {
			SortItemList (pChar->waItem, NULL, nCount, pPara->byMode, anNewToOld);
		}
		/* 装備は「何番目か」で覚えているので、旧番号→新番号で付け替える */
		for (j = 0; j < 10; j ++) {
			if (pChar->waEqu[j] >= nCount) {
				continue;
			}
			for (i = 0; i < nCount; i ++) {
				if (anNewToOld[i] == pChar->waEqu[j]) {
					pChar->waEqu[j] = (WORD)i;
					break;
				}
			}
		}
		SendCmdCHARITEM (pChar->pID, pChar->dwTblID);

	} else if (pPara->byTarget == SORTITEMTARGET_LEAVE) {
		PLEAVEINFO pInfo = NULL;

		if (g_aLeaveInfo.GetSize () == 0) {
		} else if (g_aLeaveInfo.GetSize () == 1) {
		} else if (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ()) {
		} else if (pChar->dwLeaveID) {
			pInfo = g_aLeaveInfo[pChar->dwLeaveID];
		}
		if (pInfo == NULL) {
			return;
		}
		nCount = pInfo->byCount;
		if (nCount > MAX_LEAVEITEM) {
			nCount = MAX_LEAVEITEM;
		}
		if (pPara->byMode == SORTITEMMODE_SWAP) {
			if ((pPara->byNo1 >= nCount) || (pPara->byNo2 >= nCount) || (pPara->byNo1 == pPara->byNo2)) {
				return;
			}
			WORD wTmp = pInfo->waItem[pPara->byNo1];
			BYTE byTmp = pInfo->byItemCount[pPara->byNo1];
			pInfo->waItem[pPara->byNo1] = pInfo->waItem[pPara->byNo2];
			pInfo->byItemCount[pPara->byNo1] = pInfo->byItemCount[pPara->byNo2];
			pInfo->waItem[pPara->byNo2] = wTmp;
			pInfo->byItemCount[pPara->byNo2] = byTmp;
		} else {
			SortItemList (pInfo->waItem, pInfo->byItemCount, nCount, pPara->byMode, NULL);
		}
		SendCmdLEAVEITEMGET (pChar->dwTblID);

	} else if (pPara->byTarget == SORTITEMTARGET_MAGIC) {
		/* 覚えている特技（並び順は今の職業の分。転職すると職業別の保存にそのまま入る） */
		SortMagicList (pChar->abyMagic, pPara->byMode, pPara->byNo1, pPara->byNo2);
		SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcLEAVEITEMDISPOSE										 */
/* 内容：受信データ処理(預かり所の品を引き取らずに売る・捨てる)				 */
/*       売値は店の買い取りと同じ（価値の1/4、最低1）。捨てられない品・		 */
/*       価値の無い品は売れない。所持金は 999999 までなので、超える分は売らない。 */
/*       結果は預かり所の一覧（CMD_LEAVEITEMGET）で返す（断った時も返す）	 */
/* 日付：2026/11/11															 */
/* ========================================================================= */
void RecvDataProcLEAVEITEMDISPOSE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_LEAVEITEMDISPOSE pPara = (PCMDPARA_LEAVEITEMDISPOSE)pData;
	PCHARINFO pChar;
	PLEAVEINFO pInfo = NULL;
	PITEMINFO pItem;
	int i, nCount;

	if (s_dwRecvSize < sizeof (CMDPARA_LEAVEITEMDISPOSE)) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}

	if (g_aLeaveInfo.GetSize () == 0) {
	} else if (g_aLeaveInfo.GetSize () == 1) {
	} else if (pChar->dwLeaveID >= (DWORD)g_aLeaveInfo.GetSize ()) {
	} else if (pChar->dwLeaveID) {
		pInfo = g_aLeaveInfo[pChar->dwLeaveID];
	}
	if (pInfo == NULL) {
		SendCmdLEAVEITEMGET (pChar->dwTblID);
		return;
	}

	/* 番号・品・個数が預かり所の中身と合っているか */
	if ((pPara->byNo >= pInfo->byCount) || (pPara->byNo >= MAX_LEAVEITEM) ||
			(pInfo->waItem[pPara->byNo] != pPara->wItemID) ||
			(pPara->wItemID == 0) || !ItemIDValid (pPara->wItemID) ||
			(pPara->byCount == 0) || (pPara->byCount > pInfo->byItemCount[pPara->byNo])) {
		SendCmdLEAVEITEMGET (pChar->dwTblID);
		return;
	}
	pItem = g_aItemInfo[pPara->wItemID];
	/* 捨てられない品・依頼の品は売るのも捨てるのもだめ */
	if ((pItem == NULL) || (pItem->byFlg & ITEMFLG_DROP) || QuestIsQuestItem (pPara->wItemID)) {
		SendCmdLEAVEITEMGET (pChar->dwTblID);
		return;
	}

	nCount = pPara->byCount;
	if (pPara->byMode == LEAVEDISPOSE_SELL) {
		/* 価値の無い品は買い取らない */
		if (ItemGoldOf (pPara->wItemID) == 0) {
			SendCmdLEAVEITEMGET (pChar->dwTblID);
			return;
		}
		DWORD dwSell = max (ItemGoldOf (pPara->wItemID) / 4, (DWORD)1);
		/* 所持金が 999999 を超える分は売らない */
		DWORD dwRoom = (pChar->dwGold >= 999999) ? 0 : (999999 - pChar->dwGold);
		if ((DWORD)nCount > dwRoom / dwSell) {
			nCount = (int)(dwRoom / dwSell);
		}
		if (nCount <= 0) {
			SendCmdLEAVEITEMGET (pChar->dwTblID);
			return;
		}
		pChar->dwGold += dwSell * nCount;
		SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);

	} else if (pPara->byMode != LEAVEDISPOSE_DROP) {
		SendCmdLEAVEITEMGET (pChar->dwTblID);
		return;
	}

	/* 預かり所から減らす（0個になったら詰める） */
	pInfo->byItemCount[pPara->byNo] -= (BYTE)nCount;
	if (pInfo->byItemCount[pPara->byNo] == 0) {
		for (i = pPara->byNo; i < MAX_LEAVEITEM - 1; i ++) {
			pInfo->waItem[i] = pInfo->waItem[i + 1];
			pInfo->byItemCount[i] = pInfo->byItemCount[i + 1];
		}
		pInfo->waItem[i] = 0;
		pInfo->byItemCount[i] = 0;
		pInfo->byCount --;
	}
	SendCmdLEAVEITEMGET (pChar->dwTblID);
}

/* ========================================================================= */
/* 関数名：MarketRecvChar													 */
/* 内容：取引所のコマンドの送り主を確かめる（正しければキャラを返す）		 */
/*       サイズ不足・別人・キャラがいない・倒れている・討伐チャレンジ中は NULL */
/* ========================================================================= */
static PCHARINFO MarketRecvChar(PBYTE pData)
{
	PCMDPARA_MARKET pPara = (PCMDPARA_MARKET)pData;
	PCHARINFO pChar;

	if (s_dwRecvSize < sizeof (CMDPARA_MARKET)) {
		return NULL;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return NULL;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return NULL;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return NULL;
	}
	/* 討伐チャレンジ中は預かり所・取り引きと同じく使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return NULL;
	}
	return pChar;
}

/* 取引所の受信処理（中身は SboSvrMarket.cpp） */
void RecvDataProcMARKETLIST(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = MarketRecvChar (pData);
	if (pChar) {
		MarketList (pChar, ((PCMDPARA_MARKET)pData)->byType);
	}
}

void RecvDataProcMARKETPUT(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = MarketRecvChar (pData);
	if (pChar) {
		MarketPut (pChar, ((PCMDPARA_MARKET)pData)->byNo, ((PCMDPARA_MARKET)pData)->dwPrice);
	}
}

void RecvDataProcMARKETBUY(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = MarketRecvChar (pData);
	if (pChar) {
		PCMDPARA_MARKET pPara = (PCMDPARA_MARKET)pData;
		MarketBuy (pChar, pPara->dwID, pPara->wItemID, pPara->dwPrice);
	}
}

void RecvDataProcMARKETTAKEBACK(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = MarketRecvChar (pData);
	if (pChar) {
		MarketTakeBack (pChar, ((PCMDPARA_MARKET)pData)->dwID);
	}
}

void RecvDataProcMARKETGOLDGET(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = MarketRecvChar (pData);
	if (pChar) {
		MarketGoldGet (pChar);
	}
}

/* ========================================================================= */
/* 関数名：RecycleRecvChar													 */
/* 内容：リサイクルボックスのコマンドの送り主を確かめる（正しければキャラを返す）*/
/*       取引所（MarketRecvChar）と同じ決まり。サイズ不足・別人・倒れている・ */
/*       討伐チャレンジ中は NULL											 */
/* ========================================================================= */
static PCHARINFO RecycleRecvChar(PBYTE pData)
{
	PCMDPARA_RECYCLE pPara = (PCMDPARA_RECYCLE)pData;
	PCHARINFO pChar;

	if (s_dwRecvSize < sizeof (CMDPARA_RECYCLE)) {
		return NULL;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return NULL;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return NULL;
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return NULL;
	}
	/* 討伐チャレンジ中は預かり所・取引所と同じく使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return NULL;
	}
	return pChar;
}

/* リサイクルボックスの受信処理（中身は SboSvrRecycle.cpp） */
void RecvDataProcRECYCLELIST(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = RecycleRecvChar (pData);
	if (pChar) {
		RecycleList (pChar, FALSE);
	}
}

void RecvDataProcRECYCLEPUT(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = RecycleRecvChar (pData);
	if (pChar) {
		PCMDPARA_RECYCLE pPara = (PCMDPARA_RECYCLE)pData;
		RecyclePut (pChar, pPara->wItemID, pPara->byCount);
	}
}

void RecvDataProcRECYCLETAKE(PBYTE pData, DWORD dwDataSize)
{
	PCHARINFO pChar = RecycleRecvChar (pData);
	if (pChar) {
		RecycleTake (pChar, ((PCMDPARA_RECYCLE)pData)->wItemID);
	}
}

/* 窓を閉じた：箱を閉じた絵に戻す（倒れていても戻す。持ち物は動かないので RecycleRecvChar の確かめは要らない） */
void RecvDataProcRECYCLECLOSE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_RECYCLE pPara = (PCMDPARA_RECYCLE)pData;

	if ((s_dwRecvSize < sizeof (CMDPARA_RECYCLE)) || !IsRecvChar (pPara->dwTblID)) {
		return;
	}
	RecycleClose (g_aCharInfo[pPara->dwTblID]);
}

/* ========================================================================= */
/* 関数名：RenameItemKeys													 */
/* 内容：ショートカットに登録していた wOld を、手持ちに同じ番号が残らなければ wNew へ付け替える */
/*       （鑑定・鍛える・付け替えるで番号が変わった時）						 */
/* ========================================================================= */
static void RenameItemKeys(PCHARINFO pChar, WORD wOld, WORD wNew)
{
	int k;
	BOOL bRemain = FALSE, bKey = FALSE;

	for (k = 0; k < MAX_MYITEM; k ++) {
		if (pChar->waItem[k] == wOld) {
			bRemain = TRUE;
		}
	}
	for (k = 0; (k < MAX_MAGICKEY) && !bRemain; k ++) {
		if (pChar->awKeyItem[k] == wOld) {
			pChar->awKeyItem[k] = wNew;
			bKey = TRUE;
		}
	}
	if (bKey) {
		SendMagicKeySetting (pChar);
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcRECIPEMAKE											 */

/* 内容：受信データ処理(レシピで作ってもらう)								 */
/*       存在しないレシピ・停止中のレシピ・作り手の番号が違う・近くに作り手が	 */
/*       いない要求は RECIPERESULT_INVALID で断る。							 */
/*       鍛える・付け替えるレシピは、手持ちの byNo の装備が対象（装備中でもよい）。*/
/*       条件（鑑定済み・種別・これ以上鍛えられない・同じ付与）はここで全部確かめ、 */
/*       材料と手数料を消して、同じ枠を鑑定済みの派生番号に置き換える（失敗なし）。 */
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
	/* 討伐チャレンジ中は預かり所・取り引き・作ってもらうを使わない（貸し出しの品が出入りしないように） */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
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
	if (pRecipe->byKind == RECIPEKIND_NORMAL) {
		if ((pRecipe->wItem == 0) || (pRecipe->wItem >= (WORD)g_aItemInfo.GetSize ()) || (pRecipe->byCount == 0)) {
			goto Exit;
		}
	} else if ((pRecipe->byKind != RECIPEKIND_FORGE) && (pRecipe->byKind != RECIPEKIND_SWAP)) {
		goto Exit;
	}
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		goto Exit;
	}
	if (!RecipeCheckMakerNear (pChar, pPara->byMaker)) {
		goto Exit;
	}

	if (pRecipe->byKind != RECIPEKIND_NORMAL) {
		/* 鍛える・付け替える */
		WORD wOld, wBase, wPre, wSuf, wNew;
		int nTarget = pPara->byNo;

		if (nTarget >= MAX_MYITEM) {
			goto Exit;
		}
		wOld = pChar->waItem[nTarget];
		switch (ItemForgeCalc (pRecipe, wOld, &wBase, &wPre, &wSuf)) {
		case FORGE_OK:
			break;
		case FORGE_UNKNOWN:
			byResult = RECIPERESULT_UNKNOWN;
			goto Exit;
		case FORGE_MAXED:
			byResult = RECIPERESULT_MAXED;
			goto Exit;
		case FORGE_SAME:
			byResult = RECIPERESULT_SAME;
			goto Exit;
		default:
			goto Exit;
		}
		wNew = AffixDeriveFindOrCreate (wBase, wPre, wSuf, 0);
		if ((wNew == wOld) || !ItemIsAppraised (wNew)) {
			/* 派生表がいっぱいで作れない */
			goto Exit;
		}
		byResult = RecipeMake (pChar, pRecipe, &nToInv, &nToLeave, &nTarget);
		if (byResult == RECIPERESULT_OK) {
			/* 材料を消して持ち物が詰まった後の位置 nTarget の枠を置き換える。装備中なら waEqu は枠の添字なのでそのまま装備したまま、強さを更新する */
			pChar->waItem[nTarget] = wNew;
			RenameItemKeys (pChar, wOld, wNew);
			RenewATDF (pChar->dwTblID);
			SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
			SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
			SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
			if ((pChar->dwLeaveID != 0) && (pChar->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize ())) {
				SendCmdLEAVEITEMGET (pChar->dwTblID);
			}
			SendCmdPLAYSOUND (pChar->pID, SOUND_PI52);
		}
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

/* ========================================================================= */
/* 関数名：RecvDataProcAPPRAISE												 */
/* 内容：受信データ処理(鑑定屋で鑑定してもらう)								 */
/*       近くに種類が鑑定の作り手NPCがいて、手持ちの byNo が未鑑定の派生アイテム	 */
/*       で、代金（AffixAppraiseCost）が払える時だけ、代金を引いて鑑定済みの		 */
/*       番号に置き換える。装備中なら装備したまま強さを更新する。				 */
/*       断る時は何も変えず、結果だけ返す。									 */
/* ========================================================================= */
void RecvDataProcAPPRAISE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_APPRAISE pPara = (PCMDPARA_APPRAISE)pData;
	PCHARINFO pChar;
	BYTE byResult = APPRAISERESULT_INVALID;
	DWORD dwCost = 0;
	WORD wOld, wNew;

	if (s_dwRecvSize < sizeof (CMDPARA_APPRAISE)) {
		return;
	}
	if (!IsRecvChar (pPara->dwTblID)) {
		return;
	}
	pChar = g_aCharInfo[pPara->dwTblID];
	/* 討伐チャレンジ中は貸し出しの品が入れ替わらないよう使わせない */
	if (QuestChallengeBlock (pChar)) {
		return;
	}
	if ((pChar == NULL) || (pChar->pID == 0) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}

	if ((pPara->byMaker >= (BYTE)g_aMakerInfo.GetSize ()) || (pPara->byNo >= MAX_MYITEM)) {
		goto Exit;
	}
	if ((g_aMakerInfo[pPara->byMaker]->byState != RECIPESTATE_ON) || (g_aMakerInfo[pPara->byMaker]->byKind != MAKERKIND_APPRAISE)) {
		goto Exit;
	}
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		goto Exit;
	}
	if (!RecipeCheckMakerNear (pChar, pPara->byMaker)) {
		goto Exit;
	}
	wOld = pChar->waItem[pPara->byNo];
	if (!ItemIsUnknown (wOld) || !ItemIDValid (wOld)) {
		goto Exit;
	}
	dwCost = AffixAppraiseCost (wOld);
	if (pChar->dwGold < dwCost) {
		byResult = APPRAISERESULT_LACKGOLD;
		goto Exit;
	}
	wNew = AffixAppraise (wOld);
	if (wNew == wOld) {
		goto Exit;
	}

	pChar->dwGold -= dwCost;
	pChar->waItem[pPara->byNo] = wNew;
	/* ショートカットに登録していた未鑑定品は、手持ちに同じ番号が残らなければ鑑定済みの番号へ付け替える */
	RenameItemKeys (pChar, wOld, wNew);
	/* 装備中なら waEqu は手持ちの添字なので、そのまま装備したまま。強さを更新する */
	RenewATDF (pChar->dwTblID);
	SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
	SendCmdCHARSTATUS (pChar->pID, pChar->dwTblID);
	SendCmdCHARSTATUSGOLDEXP (pChar->pID, pChar->dwTblID);
	SendCmdPLAYSOUND (pChar->pID, SOUND_PI52);
	byResult = APPRAISERESULT_OK;

Exit:
	SendCmdAPPRAISE (pChar->pID, byResult, pPara->byMaker, pPara->byNo, dwCost);
}

/* ========================================================================= */
/* 関数名：RecvDataProcTALKANSWER											 */
/* 内容：受信データ処理(会話データの「はい・いいえ」の答え)					 */
/*       byData=1 がはい・0 がいいえ、dwTblID が話しかけた NPC。				 */
/*       有効かどうかの確認と反映は TalkAnswer がする（無効なら黙って捨てる）	 */
/* ========================================================================= */
void RecvDataProcTALKANSWER(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)pData;

	if ((dwDataSize < sizeof (CMDPARA_DATA1)) || (s_nCharNoRecv < 0) || (s_nCharNoRecv >= g_aCharInfo.GetSize ())) {
		return;
	}
	TalkAnswer (g_aCharInfo[s_nCharNoRecv], pPara->dwTblID, pPara->byData ? TRUE : FALSE);
}
