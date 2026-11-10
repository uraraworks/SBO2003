/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			ウィンドウ作成の外部参照用定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2005/10/31												 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* ウィンドウの作成 */
extern PWNDINFO MakeWindowMSGVIEW						(LPSTR);					/* 常に表示メッセージ */
extern PWNDINFO MakeWindowMSG3SEC						(LPSTR);					/* ３秒メッセージ */
extern PWNDINFO MakeWindowLOGIN							(void);						/* ログイン情報 */
extern PWNDINFO MakeWindowLOGINMENU						(void);						/* ログインメニュー */
extern PWNDINFO MakeWindowLOGINMENUCHARINFO				(void);						/* ログインメニューのキャラ情報 */
extern PWNDINFO MakeWindowLOGINMENUCHARMAKE				(void);						/* ログインメニューのキャラ作成 */
extern PWNDINFO MakeWindowCOMMAND						(void);						/* コマンド */
extern PWNDINFO MakeWindowCOMMANDETC					(void);						/* その他コマンド */
extern PWNDINFO MakeWindowCOMMANDITEM					(void);						/* 道具コマンド */
extern PWNDINFO MakeWindowCOMMANDITEMBATTLE				(void);						/* 道具コマンド(戦闘マップ) */
extern PWNDINFO MakeWindowCOMMANDITEMPROC				(void);						/* 道具処理選択 */
extern PWNDINFO MakeWindowSORTMENU						(BYTE);						/* 並べ替え：入れ替え・種類順・50音順 */
extern PWNDINFO MakeWindowCOMMANDOPTION					(void);						/* オプション */
extern PWNDINFO MakeWindowCOMMANDOPTIONVOLSOUND			(void);						/* 効果音音量設定 */
extern PWNDINFO MakeWindowCOMMANDOPTIONVOLMUSIC			(void);						/* 曲音量設定 */
extern PWNDINFO MakeWindowSPEECH						(void);						/* 発言入力ウィンドウ */
extern PWNDINFO MakeWindowCONNECTLOST					(LPSTR);					/* 接続途切れた */
extern PWNDINFO MakeWindowMSG							(LPSTR, BOOL);				/* メッセージ */
extern PWNDINFO MakeWindowMALEFAMALE					(void);						/* 男女 */
extern PWNDINFO MakeWindowDELETECONFIRM					(void);						/* 削除確認 */
extern PWNDINFO MakeWindowNAMECOLOR						(void);						/* 名前の色設定 */
extern PWNDINFO MakeWindowSHOP							(int, int);					/* 店 */
extern PWNDINFO MakeWindowSHOP_BUY						(int);						/* 店:売り物一覧 */
extern PWNDINFO MakeWindowSHOP_SELL						(void);						/* 店:売る物一覧 */
extern PWNDINFO MakeWindowSHOP_CONF						(void);						/* 店:確認 */
extern PWNDINFO MakeWindowSHOPSELL						(int);						/* 店(売るだけ) */
extern PWNDINFO MakeWindowSPEECHEDIT					(void);						/* 自己紹介設定ウィンドウ */
extern PWNDINFO MakeWindowCOMMANDETC_CONF				(void);						/* 管理人呼び出し確認ウィンドウ */
extern PWNDINFO MakeWindowCOMMANDSTATUS					(void);						/* 強さウィンドウ */
extern PWNDINFO MakeWindowCOMMANDEQU					(void);						/* 装備ウィンドウ */
extern PWNDINFO MakeWindowCOMMANDEQUSub					(void);						/* 装備ウィンドウ(値表示用) */
extern PWNDINFO MakeWindowCOMMANDEQUSelect				(BYTE byType);				/* 装備ウィンドウ(選択) */
extern PWNDINFO MakeWindowCOMMANDPARTY					(void);						/* 仲間ウィンドウ */
extern PWNDINFO MakeWindowCOMMANDPARTYCHAR				(void);						/* 誘う相手選択 */
extern PWNDINFO MakeWindowLEAVE							(BYTE);						/* 預かり所 */
extern PWNDINFO MakeWindowLEAVE_SELECT					(BYTE);						/* 預かり所：どちらか選択 */
extern PWNDINFO MakeWindowLEAVE_ITEM					(void);						/* 預かり所：アイテム選択 */
extern PWNDINFO MakeWindowLEAVE_ITEMGET					(void);						/* 預かり所：引き取るアイテム選択 */
extern PWNDINFO MakeWindowLEAVE_GOLD					(BYTE);						/* 預かり所：預ける金額設定 */
extern PWNDINFO MakeWindowCOMMANDITEMCHAR				(void);						/* 道具使う相手選択 */
extern PWNDINFO MakeWindowCOMMANDITEMCHARBATTLE			(void);						/* 道具使う相手選択(戦闘マップ) */
extern PWNDINFO MakeWindowCOMMANDTRADE					(void);						/* 取り引きコマンド */
extern int GetTradeTargets								(DWORD *padwTblID, int nMax);	/* 取り引きできる相手の一覧 */
extern PWNDINFO MakeWindowTRADE							(DWORD);					/* コマンド */
extern PWNDINFO MakeWindowTRADEITEM						(void);						/* 取り引き：アイテム選択 */
extern PWNDINFO MakeWindowTRADEGOLD						(void);						/* 取り引き：金額設定 */
extern PWNDINFO MakeWindowGOLD							(void);						/* 所持金表示 */
extern PWNDINFO MakeWindowSUBGAME_BOMBINFO				(void);						/* サブゲーム：爆弾ゲーム情報 */
extern PWNDINFO MakeWindowSUBGAME_BOMBSET				(void);						/* サブゲーム：爆弾ゲーム設定 */
extern PWNDINFO MakeWindowCHGPASSWORD					(void);						/* パスワードの変更 */
extern PWNDINFO MakeWindowCONF							(int x, int y, int nType);	/* 確認 */
extern PWNDINFO MakeWindowJOB							(BYTE);						/* 冒険者登録所 */
extern PWNDINFO MakeWindowJOB_JOBSELECT					(void);						/* 冒険者登録所：職業の選択 */
extern PWNDINFO MakeWindowCOMMANDMAGIC					(void);						/* 魔法コマンド */
extern PWNDINFO MakeWindowCOMMANDMAGICBATTLE			(void);						/* 魔法コマンド(戦闘マップ) */

extern PWNDINFO MakeWindowSUBWAR_SELECTTEAM				(void);						/* 戦場ゲーム：チーム選択 */
extern PWNDINFO MakeWindowSUBWAR_WAIT					(void);						/* 戦場ゲーム：他のプレイヤー待ち */
extern PWNDINFO MakeWindowSUBWAR_SELECTRULE				(void);						/* 戦場ゲーム：ルール選択 */
extern PWNDINFO MakeWindowSUBWAR_SELECTJOB				(void);						/* 戦場ゲーム：職業選択 */
extern PWNDINFO MakeWindowSUBWAR_SELECTTURN				(void);						/* 戦場ゲーム：行動順序 */
extern PWNDINFO MakeWindowSUBWAR_TURN					(void);						/* 戦場ゲーム：行動選択 */
extern PWNDINFO MakeWindowSUBWAR_TURN_MOVE				(void);						/* 戦場ゲーム：行動(移動) */
extern PWNDINFO MakeWindowSUBWAR_TURN_ATACK				(void);						/* 戦場ゲーム：行動(攻撃) */
extern PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_FIGHTER	(void);						/* 戦場ゲーム：行動(防御：戦士) */
extern PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_MARTIAL	(void);						/* 戦場ゲーム：行動(防御：武道家) */
extern PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH		(void);						/* 戦場ゲーム：行動(防御：魔法使い) */
extern PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_PRIEST	(void);						/* 戦場ゲーム：行動(防御：僧侶) */
extern PWNDINFO MakeWindowSUBWAR_TURN_DIFENCE_WITCH2	(void);						/* 戦場ゲーム：行動(魔法防御：魔法使い) */
extern PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_WITCH		(void);						/* 戦場ゲーム：行動(魔法：魔法使い) */
extern PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_TARGET		(void);						/* 戦場ゲーム：行動(魔法対象者選択) */
extern PWNDINFO MakeWindowSUBWAR_TURN_MAGIC_PRIEST		(void);						/* 戦場ゲーム：行動(魔法：僧侶) */

extern PWNDINFO MakeWindowBATTLE_COMMAND				(void);									/* 戦闘：コマンド */
extern PWNDINFO MakeWindowBATTLE_TARGET					(int nArea, BOOL bMyChar, int nType);	/* 戦闘：ターゲット選択 */

extern PWNDINFO MakeWindowSUBSUGOROKU_JOINMODE			(void);						/* すごろく：参加モード選択 */
extern PWNDINFO MakeWindowSUBSUGOROKU_COMMAND			(void);						/* すごろく：行動選択 */
extern PWNDINFO MakeWindowSUBSUGOROKU_IDLE				(void);						/* すごろく：平常状態 */

extern PWNDINFO MakeWindowITEMINFO						(int x, int y);				/* アイテム情報 */
extern PWNDINFO MakeWindowITEMINFOList					(PWNDINFO pWndList);		/* アイテム情報(一覧の横) */
extern void UpdateWindowITEMINFOList						(PWNDINFO pWndList);		/* アイテム情報(一覧の横)の更新 */
extern PWNDINFO MakeWindowDEAD_COMMAND					(void);						/* 死亡中：コマンド */
extern PWNDINFO MakeWindowSTATUSMAGIC					(void);						/* 強さ：覚えている特技 */
extern PWNDINFO MakeWindowMAKER							(int nMaker);				/* 作り手：作れる物の一覧 */
extern PWNDINFO MakeWindowMAKER_CONF					(int nRecipe);				/* 作り手：確認 */
extern PWNDINFO MakeWindowTOWNWARP					(int nMode);						/* 町転送：行き先の選択 */
extern PWNDINFO MakeWindowQUEST						(int nMode);						/* ギルドの依頼：一覧・進み具合 */

extern int MakerGetRecipeList							(int nMaker, int *pnList, int nMax);	/* 作り手の一覧に出すレシピの番号を並び順で取得 */
extern int MakerGetCursorRecipe							(PWNDINFO pWndList);		/* 一覧のカーソル位置のレシピの番号 */
extern int MakerCountHave								(WORD wItem);				/* 材料として使える数（持ち物+預かり所） */
extern BOOL MakerCanMake								(int nRecipe);				/* 材料と手数料が足りているか */
extern DWORD MakerStateHash								(int nMaker);				/* 画面に出ている内容が変わったか調べる値 */
extern void MakerSetMsg									(PWNDINFO pWndList, LPCSTR pszMsg);	/* 作り手の画面のメッセージを差し替える */
extern void MakerSetTalkMsg								(PWNDINFO pWndList);		/* 作り手の話しかけた時の台詞を出す */
extern void MakerScrollFit								(PWNDINFO pWndList, int nIndex, int nCount);	/* 一覧のカーソルとスクロールを整える */

/* Copyright(C)URARA-works 2003-2005 */
