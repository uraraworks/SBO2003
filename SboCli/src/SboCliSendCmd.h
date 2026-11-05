/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			コマンド送信処理の外部参照用定義						 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* コマンド送信 */
extern void SendCmdLOGINREQ			(void);									/* ログイン要求 */
extern void SendCmdWEBACCOUNT		(BYTE, BYTE = 0);								/* ブラウザ版のアカウント関係 */
extern void SendCmdCHGSTATE			(BYTE);									/* 状態遷移 */
extern void SendCmdCHGDIRECTION		(BYTE);									/* 向き変更要求 */
extern void SendCmdCHARPOS			(BYTE, BOOL);							/* 移動要求 */
extern void SendCmdCHARINFOREQ		(DWORD);								/* キャラ情報要求 */
extern void SendCmdSPEECH			(LPSTR, BYTE);							/* 発言通知 */
extern void SendCmdHARTBEAT			(void);									/* 生存確認通知 */
extern void SendCmdMAPPARTSADD		(void);									/* マップパーツ追加 */
extern void SendCmdMAPPARTSADDANIME	(DWORD);								/* マップパーツアニメ追加 */
extern void SendCmdCHGMAP			(WORD, int, int);						/* マップ書き換え */
extern void SendCmdMAPPARTS			(int);									/* マップパーツ更新 */
extern void SendCmdRELOADPARTS		(int);									/* マップパーツ更新要求 */
extern void SendCmdADDDELMAP		(BYTE, BYTE);							/* マップ幅増減要求 */
extern void SendCmdSAVEMAP			(void);									/* マップ保存 */
extern void SendCmdEVENTADD			(void);									/* イベント情報追加 */
extern void SendCmdEVENTGET			(int, DWORD dwX = 0xFFFFFFFF, DWORD dwY = 0xFFFFFFFF);	/* イベント情報取得 */
extern void SendCmdEVENTINFO		(void);									/* イベント情報更新 */
extern void SendCmdCHGMAPINFO		(void);									/* マップ情報更新 */
extern BOOL SendCmdEVENTCHECK		(BYTE);									/* イベントチェック要求 */
extern void SendCmdADDCHAR			(PCMDPARA_ADDCHARINFO);					/* キャラ追加要求 */
extern void SendCmdBANCHAR			(DWORD);								/* キャラ強制ログオフ要求 */
extern void SendCmdCHARINFO			(PCMDPARA_CHARINFO);					/* キャラ情報 */
extern void SendCmdADDMAP			(void);									/* マップ追加 */
extern void SendCmdADDMSG			(LPSTR);								/* メッセージ追加 */
extern void SendCmdMAPMSGREQ		(DWORD);								/* メッセージ要求 */
extern void SendCmdCHGMSG			(LPSTR, DWORD);							/* メッセージ更新 */
extern void SendCmdOPENDOOR			(void);									/* 扉開け要求 */
extern void SendCmdCHGCHARGRP		(DWORD);								/* 着せ替え要求 */
extern void SendCmdCHGCHARGRPBACK	(DWORD);								/* キャラ画像記録 */
extern void SendCmdNAMESEX			(PCHARINFO);							/* 名前と性別更新 */
extern void SendCmdGMCALL			(void);									/* 管理人呼び出し */
extern void SendCmdMAPMOVE			(DWORD, DWORD, DWORD, DWORD, BYTE);		/* マップ移動 */
extern void SendCmdCHARPUSH			(DWORD);								/* 押して要求 */
extern void SendCmdCOMEFC			(BYTE);									/* 吹き出し要求 */
extern void SendCmdADDSCORE			(DWORD);								/* 得点追加要求 */
extern void SendCmdDELCHAR			(DWORD);								/* キャラ削除要求 */
extern void SendCmdACCADD			(LPSTR, LPSTR);							/* アカウント追加要求 */
extern void SendCmdGETACC			(DWORD);								/* アカウント・パスワード取得 */
extern void SendCmdCHGACC			(DWORD, LPSTR, LPSTR);					/* アカウント・パスワード更新 */
extern void SendCmdTALKRAND			(DWORD);								/* 話してランダム要求 */
extern void SendCmdSTOPRAND			(DWORD);								/* 止まるかランダム切り替え */
extern void SendCmdGETACCINFO		(DWORD);								/* アカウント情報要求 */
extern void SendCmdCHGACCSTATE		(DWORD, BYTE);							/* アカウント状態更新 */
extern void SendCmdCLIVERSION		(void);									/* クライアントバージョン通知 */
extern void SendCmdCHGCEF			(int);									/* 吹き出し情報更新 */
extern void SendCmdCHGADMINON		(void);									/* 管理人ON/OFF通知 */
extern void SendCmdADDITEM			(void);									/* アイテム追加 */
extern void SendCmdCHGITEM			(DWORD);								/* アイテム情報更新 */
extern void SendCmdGETITEM			(DWORD, DWORD);							/* アイテム取得 */
extern void SendCmdDROPITEM			(BYTE, DWORD, DWORD);					/* アイテム捨てる */
extern void SendCmdREQCHARITEM		(DWORD);								/* 所持アイテム情報要求 */
extern void SendCmdCHARITEM			(DWORD);								/* 所持アイテム情報 */
extern void SendCmdPLAYSOUNDALL		(DWORD);								/* 効果音放送 */
extern void SendCmdUSEITEM			(BYTE, DWORD);							/* アイテム使う */
extern void SendCmdSVRINFOREQ		(void);									/* サーバー情報要求 */
extern void SendCmdNAMECOLOR		(DWORD, COLORREF, COLORREF);			/* 名前の色設定 */
extern void SendCmdADDSHOP			(void);									/* 店追加要求 */
extern void SendCmdCHGSHOP			(BYTE);									/* 店情報更新 */
extern void SendCmdADDEFC			(void);									/* エフェクト追加 */
extern void SendCmdCHGEFC			(BYTE);									/* エフェクト更新 */
extern void SendCmdEFCREQ			(BYTE, BYTE);							/* エフェクト発動 */
extern void SendCmdREQCHARSTATUS	(DWORD);								/* ステータス情報要求 */
extern void SendCmdCHARSTATUS		(DWORD);								/* キャラステータス情報 */
extern void SendCmdTARGETCHAR		(int);									/* ターゲット要求 */
extern void SendCmdTARGETCHAR		(DWORD);								/* ターゲット要求 */
extern void SendCmdCHGEQU			(DWORD, BYTE);							/* 装備変更 */
extern void SendCmdSTTMPLATE		(DWORD, BYTE);							/* ステータステンプレート情報 */
extern void SendCmdLEAVEITEM		(BYTE);									/* アイテム預ける */
extern void SendCmdLEAVEITEMGET		(DWORD);								/* 預けてるアイテム取得 */
extern void SendCmdSUBGAMEINFOGET	(DWORD);								/* サブゲーム情報取得 */
extern void SendCmdSUBGAMEINFOADD	(void);									/* サブゲーム情報追加 */
extern void SendCmdSUBGAMEINFO		(int);									/* サブゲーム情報通知 */
extern void SendCmdTRADE			(DWORD, BYTE);							/* 取り引きコマンド */
extern void SendCmdTRADEINFO		(void);									/* 取り引き情報 */
extern void SendCmdTRADEPROC		(void);									/* 取り引き決定 */
extern void SendCmdLEAVEGOLD		(DWORD);								/* お金預ける */
extern void SendCmdLEAVEGOLDGET		(DWORD);								/* 預けてるお金取得 */
extern void SendCmdSETSTARTPOS		(BYTE, BYTE, BYTE);						/* 復活座標設定 */
extern void SendCmdCHGPASSWORD		(DWORD, LPSTR);							/* パスワード更新 */
extern void SendCmdLEAVEINFOGET		(DWORD);								/* 預かり所情報取得 */
extern void SendCmdLEAVEINFO		(DWORD);								/* 預かり所情報 */
extern void SendCmdACCSEARCH		(BYTE, DWORD, LPSTR);					/* アカウント検索 */
extern void SendCmdMSGCMD_0			(BYTE byCmd);							/* メッセージコマンド:パラメータ無し */
extern void SendCmdMSGCMD_1			(BYTE byCmd, int nPara);				/* メッセージコマンド:パラメータ１つ */
extern void SendCMDPARTY			(BYTE, DWORD, BYTE);					/* 仲間コマンド */
extern void SendCMDJOBINFO			(BYTE);									/* 職業情報 */
extern void SendCMDJOBINFOREQ		(BYTE);									/* 職業情報要求 */
extern void SendCMDJOBCHG			(BYTE);									/* 職業変更 */
extern void SendCMDMAGICINFO		(BYTE);									/* 魔法情報 */
extern void SendCMDMAGICINFOREQ		(BYTE);									/* 魔法情報要求 */
extern void SendCMDUSEMAGIC			(BYTE byID, DWORD dwTblID);				/* 魔法使用 */
extern void SendCMDMAGICUSERINFO	(BYTE);									/* 魔法利用者情報 */
extern void SendCMDMAGICUSERINFOREQ	(BYTE);									/* 魔法利用者情報要求 */
extern void SendCMDTARGETCHG		(DWORD);								/* ターゲット変更要求 */
extern void SendCMDEQUUSERINFO		(BYTE);									/* 装備職業情報 */
extern void SendCMDEQUUSERINFOREQ	(BYTE);									/* 装備職業情報要求 */
extern void SendCmdADMINUNDO		(BYTE, BYTE, DWORD);					/* 管理者画面の「元に戻す」 */
extern void SendCMDSHOPBUY			(DWORD, DWORD);							/* アイテム購入 */
extern void SendCMDDIFENCE			(void);									/* 防御 */
extern void SendCMDTHROWSNOW		(void);									/* 雪を投げる */
extern void SendCmdEFECTCHG			(BYTE, BOOL);							/* エフェクト変更 */
extern void SendCmdRESURRECTION		(void);									/* 蘇生 */
extern void SendCmdREQ_ATTACK		(DWORD dwTblID, BYTE byTurnLV);			/* 攻撃要求 */

/* サブゲーム用コマンド送信 */
extern void SendCmdSubSETBOMB		(void);									/* 爆弾設置 */
extern void SendCmdSubBOMBINFO		(void);									/* 爆弾ゲーム設定 */
extern void SendCmdSubBOMBSTART		(void);									/* 試合開始 */
extern void SendCmdSubBOMBSTOP		(void);									/* 試合中止 */
extern void SendCmdSubBOMBREMOCON	(void);									/* リモコン使用 */
extern void SendCmdSubMAZERENEW		(BYTE);									/* 迷路作成 */
extern void SendCmdSubDUNGEONRENEW	(BYTE);									/* ダンジョン作成 */
extern void SendCmdSubWARSELECTTEAM	(BYTE);									/* 戦場ゲーム：チーム選択 */
extern void SendCmdSubWARQUIT		(void);									/* 戦場ゲーム：ゲームから抜ける */
extern void SendCmdSubWARSELECTRULE	(BYTE);									/* 戦場ゲーム：ルール選択 */
extern void SendCmdSubWARSELECTJOB	(BYTE);									/* 戦場ゲーム：職業選択 */
extern void SendCmdSubWARSELECTTURN	(BYTE);									/* 戦場ゲーム：行動順序 */
extern void SendCmdSubWARCOMMAND	(BYTE, BYTE);							/* 戦場ゲーム：コマンド通知 */
extern void SendCmdSubWARUSEMAGIC	(BYTE, BYTE);							/* 戦場ゲーム：魔法 */
extern void SendCmdSubSUGOROKUJOINMODE	(BYTE);								/* すごろく：参加モード選択 */
extern void SendCmdSubSUGOROKUQUIT		(void);								/* すごろく：ゲームから抜ける */
extern void SendCmdSubSUGOROKUDICE		(void);								/* すごろく：サイコロを振る */

extern void SendCmdCHAR_TAIL			(DWORD dwTblIDTarget);				/* キャラ系:ついていく要求 */
/* Copyright(C)URARA-works 2003-2005 */
