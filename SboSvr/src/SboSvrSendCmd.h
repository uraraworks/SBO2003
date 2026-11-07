/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			コマンド送信処理の外部参照用定義						 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#pragma once

class CInfoSubGameWAR;

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void SendCmdInside2				(PCHARINFO, LPBYTE, DWORD, DWORD, BYTE, BOOL bFlg = TRUE, BOOL bSend = FALSE, BYTE byPriority = 0);	/* 同画面内のプレイヤーにコマンド送信 */
extern void SendCmdSubGame				(BYTE, DWORD, LPBYTE, DWORD);					/* 指定サブゲーム参加者にコマンド送信 */
extern void CheckCharPosSend			(PCHARINFO);									/* 画面内にキャラが見えたら送信 */
extern void SendCmdInsideMap			(DWORD, LPBYTE, DWORD, BYTE);					/* 指定マップ内のプレイヤーにコマンド送信 */
extern void SendCmdCONNECT				(DWORD, BYTE);									/* コマンド送信(接続応答) */
extern void SendCmdLOGINRES				(DWORD, DWORD, BYTE);							/* コマンド送信(ログイン応答) */
extern void SendCmdCHGSTATE				(DWORD, DWORD, BYTE);							/* コマンド送信(状態遷移) */
extern void SendCmdMAPINFO				(DWORD, DWORD);									/* コマンド送信(マップ情報通知) */
extern void SendCmdMAPPARTSINFO			(DWORD);										/* コマンド送信(マップパーツ情報通知) */
extern void SendCmdCHARINFO				(DWORD, int);									/* コマンド送信(キャラ情報通知) */
extern void SendCmdCHARINFO2			(DWORD, DWORD, int);							/* コマンド送信(キャラ情報通知2) */
extern void SendCmdCHARINFO3			(int);											/* コマンド送信(キャラ情報通知) */
extern void SendCmdCHGDIRECTION			(DWORD, int);									/* コマンド送信(向き変更要求) */
extern void SendCmdCHARPOS				(DWORD, int, BYTE, BYTE);						/* コマンド送信(座標通知) */
extern void SendCmdSPEECH				(DWORD, int, BYTE);								/* コマンド送信(発言通知) */
extern void SendCmdONLINE				(DWORD);										/* コマンド送信(オンライン数通知) */
extern void SendCmdMAPPARTS				(DWORD, int);									/* コマンド送信(マップパーツ更新) */
extern void SendCmdCHGMAP				(DWORD, WORD, int, int);						/* コマンド送信(マップ書き換え) */
extern void SendCmdADDDELMAP			(DWORD, BYTE, BYTE);							/* コマンド送信(マップ幅増減) */
extern void SendCmdEVENTINFO			(DWORD, int, int);								/* コマンド送信(イベント情報更新) */
extern void SendCmdEVENTCOUNT			(DWORD, DWORD);									/* コマンド送信(イベント数通知) */
extern void SendCmdEVENTPOS				(DWORD, DWORD);									/* コマンド送信(イベント座標通知) */
extern void SendCmdHARTBEAT				(DWORD);										/* コマンド送信(生存確認応答) */
extern void SendCmdCHGBGM				(DWORD, DWORD, BYTE);							/* コマンド送信(BGM変更) */
extern void SendCmdMSG					(DWORD, DWORD);									/* コマンド送信(メッセージ更新) */
extern void SendCmdVIEWMSG				(DWORD, DWORD);									/* コマンド送信(メッセージ表示) */
extern void SendCmdPLAYSOUND			(DWORD, BYTE);									/* コマンド送信(効果音再生) */
extern void SendCmdPLAYSOUNDinside		(DWORD, BYTE);									/* 画面内のキャラにコマンド送信(効果音再生) */
extern void SendCmdMAPMSG				(DWORD, LPSTR);									/* コマンド送信(メッセージ通知) */
extern void SendCmdMAPMSGREQ			(DWORD, LPSTR);									/* コマンド送信(メッセージ通知) */
extern void SendCmdSYSMSG				(DWORD, LPCTSTR, BYTE byFlg = SPEECHCMD_ALL);	/* コマンド送信(システムメッセージ) */
extern void SendCmdSYSMSGMAP			(DWORD, LPSTR, BYTE byFlg = SPEECHCMD_ALL);		/* コマンド送信(マップ内システムメッセージ) */
extern void SendCmdEFECTCHG				(DWORD, DWORD, BYTE, BOOL bIdle = FALSE);		/* コマンド送信(エフェクト変更) */
extern void SendCmdCOMEFC				(PCHARINFO);									/* コマンド送信(吹き出し通知) */
extern void SendCmdERR					(DWORD, BYTE);									/* コマンド送信(エラー通知) */
extern void SendCmdGETACCINFO			(DWORD, DWORD);									/* コマンド送信(アカウント情報要求) */
extern void SendCmdCHGCEF				(DWORD, WORD);									/* コマンド送信(吹き出し情報更新) */
extern void SendCmdCEFINFO				(DWORD);										/* コマンド送信(吹き出し情報通知) */
extern void SendCmdFADEINOUT			(DWORD, BYTE);									/* コマンド送信(フェードイン・アウト) */
extern void SendCmdCHGITEM				(DWORD, DWORD);									/* コマンド送信(アイテム情報更新) */
extern void SendCmdITEMINFO				(DWORD);										/* コマンド送信(アイテム情報送信) */
extern void SendCmdCHARITEM				(DWORD, DWORD);									/* コマンド送信(所持アイテム情報) */
extern void SendCmdSVRINFO				(DWORD);										/* コマンド送信(サーバー情報) */
extern void SendCmdMAPCHARINFO			(DWORD);										/* コマンド送信(マップ内キャラ数通知) */
extern void SendCmdNAMECOLOR			(DWORD);										/* コマンド送信(名前の色設定) */
extern void SendCmdSHOP					(DWORD, BYTE);									/* コマンド送信(店情報) */
extern void SendCmdSHOPMap				(DWORD, BYTE);									/* コマンド送信(店情報・マップ指定) */
extern void SendCmdADMINUNDO			(DWORD, BYTE, BYTE, BYTE, WORD, DWORD, LPCSTR);	/* コマンド送信(管理者画面の「元に戻す」) */
extern void SendCmdNPCROUTE			(DWORD, BYTE, BYTE, DWORD, DWORD, LPCSTR);		/* コマンド送信(NPCの巡回ルート) */
extern void SendCmdEFC					(BYTE);											/* コマンド送信(エフェクト情報通知) */
extern void SendCmdEFCINFO				(DWORD);										/* コマンド送信(エフェクト情報通知) */
extern void SendCmdCHARSTATUS			(DWORD, DWORD);									/* コマンド送信(キャラステータス通知) */
extern void SendCmdCHARSTATUSHP			(DWORD, DWORD);									/* コマンド送信(キャラステータス(HPのみ)通知) */
extern void SendCmdCHARSTATUSHPMANA		(DWORD, DWORD);									/* コマンド送信(キャラステータス(HPとMANA)通知) */
extern void SendCmdCHARSTATUSGOLDEXP	(DWORD, DWORD);									/* コマンド送信(キャラステータス(GOLDのみ)通知) */
extern void SendCmdTARGETCHARPC			(DWORD, DWORD);									/* コマンド送信(付いていく通知) */
extern void SendCmdADDCHAREFC			(DWORD, COLORREF, LPSTR);						/* コマンド送信(文字エフェクト追加) */
extern void SendCmdCHGCOLOR				(DWORD);										/* コマンド送信(色変換) */
extern void SendCmdSTTMPLATE			(DWORD, BYTE);									/* コマンド送信(ステータステンプレート情報) */
extern void SendCmdLEAVEITEMGET			(DWORD);										/* コマンド送信(預けてるアイテム取得) */
extern void SendCmdSUBGAMECOUNT			(DWORD);										/* コマンド送信(サブゲーム数通知) */
extern void SendCmdSUBGAMEINFO			(DWORD, int);									/* コマンド送信(サブゲーム情報通知) */
extern void SendCmdSUBGAMEINFOALL		(DWORD);										/* コマンド送信(全サブゲーム情報通知) */
extern void SendCmdSUBSTATUS			(DWORD);										/* コマンド送信(サブステータス情報) */
extern void SendCmdTRADE				(DWORD, DWORD, BYTE);							/* コマンド送信(取り引きコマンド) */
extern void SendCmdSYSMSG2				(DWORD, DWORD, DWORD, DWORD);					/* コマンド送信(ID指定システムメッセージ) */
extern void SendCmdLEAVEGOLDGET			(DWORD);										/* コマンド送信(預けてる金額取得) */
extern void SendCmdLEAVEINFO			(DWORD, DWORD);									/* コマンド送信(預かり所情報) */
extern void SendCmdACCSEARCH			(DWORD, DWORD);									/* コマンド送信(アカウント検索) */
extern void SendCmdADMINWINDOW			(DWORD, BYTE);									/* コマンド送信(管理者ウィンドウ表示) */
extern void SendCmdPARTY				(DWORD, DWORD, BYTE, BYTE);						/* コマンド送信(仲間コマンド) */
extern void SendCmdJOBINFO				(DWORD, BYTE);									/* コマンド送信(職業情報) */
extern void SendCmdMAGICINFO			(DWORD, BYTE);									/* コマンド送信(魔法情報) */
extern void SendCmdMAGICUSERINFO		(DWORD, BYTE);									/* コマンド送信(魔法利用者情報) */
extern void SendCmdEQUUSERINFO			(DWORD, BYTE);									/* コマンド送信(装備職業情報) */
extern void SendCmdTARGETCHAR			(DWORD, DWORD);									/* コマンド送信(ターゲット要求) */
extern void SendCmdHASTE				(DWORD, BYTE, DWORD);							/* コマンド送信(加速の状態を本人へ) */
extern void SendCmdSubBombTime			(BYTE, PSUBGAME_BOMBINFO);						/* サブゲーム用コマンド送信(爆弾ゲーム：残り時間通知) */
extern void SendCmdSubBOMBINFO			(DWORD, BYTE, PSUBGAME_BOMBINFO);				/* サブゲーム用コマンド送信(爆弾ゲーム：設定) */
extern void SendCmdSubBOMBSTOP			(DWORD, BYTE, PSUBGAME_BOMBINFO);				/* サブゲーム用コマンド送信(爆弾ゲーム：試合中止) */
extern void SendCmdSubBOMBSETBOMB		(DWORD);										/* サブゲーム用コマンド送信(爆弾ゲーム：爆弾設置) */
extern void SendCmdSubWarCHGSTATE		(DWORD, BYTE);									/* サブゲーム用コマンド送信(戦場ゲーム：状態遷移) */
extern void SendCmdSubWarSELECTTURN		(CInfoSubGameWAR *, BYTE);						/* サブゲーム用コマンド送信(戦場ゲーム：行動順序通知) */
extern void SendCmdSubWarMAGIC_TARGET	(DWORD, PCMDPARA_WARSELECTMAGICTARGET);			/* サブゲーム用コマンド送信(戦場ゲーム：魔法対象者一覧) */
extern void SendCmdSubSugorokuCHGSTATE	(DWORD, BYTE);									/* サブゲーム用コマンド送信(すごろく：状態遷移) */
extern void SendCmdCHAR_STATEINFO		(DWORD pIDSend, DWORD dwTblID);					/* コマンド送信(キャラ系:状態通知) */

/* Copyright(C)URARA-works 2003-2005 */
