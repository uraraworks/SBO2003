/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			グローバル変数の外部参照用定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

#pragma once

class CInfoEfc;
class CInfoJob;
class CProcSubGame;
class CAdminWindow;

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
extern CDXAudio DXAudio;											/* 音再生オブジェクト */
extern CLibMusicLoader g_LibMusic;									/* MIDI再生ライブラリ */
extern CUraraSockTCP cliSock;										/* クライアントオブジェクト */
extern CCRC g_crc;													/* CRC用クラス */
extern IDirectMusicSegment8 *aSoundTbl[SOUND_MAX];					/* 音テーブル */
extern CImg32	*g_pDibPrimary,										/* プライマリビットマップ */
				*g_pDibBack,										/* バックバッファビットマップ */
				*g_pDibSystem,										/* システムグラフィックビットマップ */
				**g_apDibChar,										/* キャラグラフィックビットマップ */
				**g_apDibCharBack,									/* キャラグラフィック保存用 */
				**g_apDibChar1632,									/* キャラグラフィック(16x32)ビットマップ */
				**g_apDibChar1632Back,								/* キャラグラフィック(16x32)保存用 */
				**g_apDibChar3232,									/* キャラグラフィック(32x32)ビットマップ */
				**g_apDibChar3232Back,								/* キャラグラフィック(32x32)保存用 */
				**g_apDibChar4848,									/* キャラグラフィック(48x48)ビットマップ */
				**g_apDibChar4848Back,								/* キャラグラフィック(48x48)保存用 */
				**g_apDibMap,										/* マップグラフィックビットマップ */
				**g_apDibMapBack,									/* マップグラフィック保存用 */
				**g_apDibEfect,										/* エフェクトグラフィックビットマップ */
				**g_apDibEfect1632,									/* エフェクトグラフィック(16x32)ビットマップ */
				**g_apDibObj3232,									/* オブジェクト(32x32)グラフィックビットマップ */
				*g_pDibMapParts,									/* マップパーツグラフィックビットマップ */
				*g_pDibDrawMap,										/* マップ描画用テンポラリビットマップ */
				*g_pDibComEfc,										/* 吹き出し用ビットマップ */
				*g_pDibComEfcWindow,								/* 吹き出しウィンドウ用ビットマップ */
				*g_pDibLogo,										/* ロゴ用ビットマップ */
				*g_pDibTitle,										/* タイトル用ビットマップ */
				*g_pDibTitleBack,									/* タイトル背景用ビットマップ */
				*g_pDibAnime,										/* アニメーション用ビットマップ */
				*g_pDibSubgBomb,									/* サブゲーム（爆弾ゲーム）用ビットマップ */
				*g_pDibWChar,										/* サブゲーム（戦場ゲーム）用ビットマップ */
				pDibDrawTmp;										/* 描画テンポラリ */

extern HMODULE g_hDll,												/* リソースDLLハンドル */
			   g_hDllGrpData;										/* グラフィックデータDLLハンドル */
extern HINSTANCE g_hInstance;										/* インスタンスハンドル */
extern DWORD g_dwNewID,												/* 新しいID */
			 g_dwTimeDataCount,										/* 最終データ測定時間 */
			 g_dwTimeMoveDisable,									/* 移動禁止時間 */
			 g_dwTimePingLast,										/* 最終Ping送信時間 */
			 g_dwTimeLastProc,										/* 最終自キャラ処理時間 */
			 g_dwTimeLastLogo,										/* ロゴ表示時間 */
			 g_dwTimeLastMapParts,									/* 最終マップパーツ処理時間 */
			 g_dwTimeLastAnime,										/* アニメーション処理時間 */
			 g_dwTimeHartbeatReq,									/* ハートビート送信時間 */
			 g_dwTimeHartbeat,										/* ハートビート応答時間 */
			 g_dwTimeDrawTime,										/* 描画にかかった時間 */
			 g_dwTimeTextFadeInOut,									/* マップ名表示用待ち時間 */
			 g_dwTimeLastThrow,										/* 最後に投げた時間 */
			 g_dwTimeLastCursor,									/* カーソル処理時間 */
			 g_dwOnilne,											/* 参加プレイヤー数 */
			 g_dwFpsCount,											/* 1秒間のループ回数 */
			 g_dwActiveMsgID,										/* メッセージID(キー入力を行う) */
			 g_dwMsgIDLogin,										/* メッセージID(ログイン) */
			 g_dwMsgIDConnect,										/* メッセージID(接続中です) */
			 g_dwMsgIDMENU,											/* メッセージID(ログインメニュー) */
			 g_dwMsgIDMENUCHARINFO,									/* メッセージID(ログインメニューキャラ情報) */
			 g_dwMsgIDMENUCHARMAKE,									/* メッセージID(ログインメニューキャラ作成) */
			 g_dwMsgIDCommand,										/* メッセージID(コマンド) */
			 g_dwMsgIDTRADE,										/* メッセージID(取り引き) */
			 g_dwMsgIDGold,											/* メッセージID(所持金表示) */
			 g_dwMsgIDSubGame,										/* メッセージID(サブゲーム情報) */
			 g_dwSelctRura,											/* 管理人ルーラ選択用 */
			 g_dwMaxRura,											/* 管理人ルーラ記録数 */
			 g_dwCountPC,											/* マップ内PC数 */
			 g_dwCountNPC,											/* マップ内NPC数 */
			 g_dwPlayBgmNo;											/* 演奏中BGM番号 */

extern BYTE g_byGameState,											/* ゲーム状態 */
			g_byFps,												/* 描画カウンタ */
			g_byAnimeNo,											/* アニメーション番号 */
			g_byViewMode,											/* 表示モード */
			g_byComSelectBattle,									/* 戦闘用コマンド選択状態 */
			g_byComSelectMagic,										/* 魔法選択状態 */
			g_byStage,												/* 状態管理用 */
			g_byTextFadeInOut,										/* マップ名表示状態 */
			g_byMapGrpCountX,										/* マップパーツ数(横) */
			g_byMapGrpCountY,										/* マップパーツ数(縦) */
			g_byAdminAnimeNo,										/* 選択中アニメーション番号 */
			g_byPosReqNo,											/* 座標通知シーケンス番号 */
			g_byAdminCount,											/* 管理人数 */
			g_byUseEfcID,											/* 発動させるエフェクトID */
			g_byVolSound,											/* 音量（効果音） */
			g_byVolMusic,											/* 音量（曲） */
			g_bySpeechMode,											/* 発言モード */
			g_bySoundCount,											/* 鐘鳴らした数 */
			g_byCursorAnime;										/* カーソルアニメ番号 */

extern BOOL g_bWindowActive,										/* ウィンドウがアクティブ */
			g_bDraw,												/* 画面更新あり */
			g_bReqestPos,											/* 移動要求中 */
			g_bMoving,												/* 自発的に移動中 */
			g_bSpeech,												/* 発言入力中 */
			g_bAdminPos,											/* 座標を取得 */
			g_bEnableMove,											/* 歩き許可 */
			g_bBGMONOFF,											/* 音楽ON/OFF */
			g_bComEfcONOFF,											/* 吹き出し効果音ON/OFF */
			g_bChatSound,											/* 発言の音ON/OFF */
			g_bCpuMax,												/* CPUフル活動 */
		    g_bBlock,												/* 当たり判定 */
			g_bProcShopConfirm,										/* 店:確認中 */
			g_bTitleSkip,											/* ロゴ・タイトル飛ばす */
			g_bMainWindowView,										/* メインウィンドウ表示切り替え */
			g_bBalloonWindowView,									/* 吹き出しウィンドウ表示切り替え */
			g_bSpeechView,											/* 発言徐々に消えるON/OFF */
			g_bBattleMode,											/* 戦闘モード */
			g_bSnowCharge,											/* 雪をつかんでいる？ */
			g_bHartbeatCheck,										/* ハートビートチェック用 */
			g_bMapMarkView,											/* マップ編集当たり判定マーク */
			g_bWeatherDraw;											/* 天候描画判定 */

extern WORD g_wPort,												/* 接続ポート番号 */
			g_wCountCharGrp,										/* 使用キャラグラフィック数 */
			g_wCountCharGrp1632,									/* 使用キャラ(16x32)グラフィック数 */
			g_wCountCharGrp3232,									/* 使用キャラ(32x32)グラフィック数 */
			g_wCountCharGrp4848,									/* 使用キャラ(48x48)グラフィック数 */
			g_wCountMapPartsGrp,									/* 使用マップパーツグラフィック数 */
			g_wCountMapParts,										/* 使用マップパーツ数 */
			g_wCountEfcGrp,											/* 使用エフェクトグラフィック数 */
			g_wCountEfcGrp1632,										/* 使用エフェクト(16x32)グラフィック数 */
			g_wCountObjGrp,											/* 使用オブジェクト(32x32)グラフィック数 */
			g_wLastProcSecond;										/* 自キャラ最終処理秒 */

extern char g_szWebToken[MAX_WEBTOKEN + 1];							/* 端末トークン（ブラウザ版：web/replace/SboCliWebAccount_web.cpp） */
extern char g_szAcc[MAX_ACCNAME],									/* アカウント名 */
			g_szPas[MAX_ACCNAME],									/* パスワード */
			g_szMacAdr[6],											/* MACアドレス */
			*g_pMapDarkTmp,											/* 暗闇描画用テンポラリ */
			*g_pMapDarkTmpPos;										/* 暗闇描画用テンポラリ */
extern int g_nMyCharID,												/* 自分のキャラ番号 */
		   g_nSelectPartsID,										/* 選択中のパーツID */
		   g_nSelectEventID,										/* 選択中のイベントID */
		   g_nSelectSubGameID,										/* 選択中のサブゲームID */
		   g_nAdminWindowType,										/* 表示中の管理者ウィンドウ種別 */
		   g_nEventCount,											/* イベント登録数 */
		   g_nSubGameCount,											/* サブゲーム登録数 */
		   g_nProcMsg,												/* 処理ID(メッセージ) */
		   g_nProcChar,												/* 処理ID(キャラ) */
		   g_nProcSysMsg,											/* 処理ID(システムメッセージ) */
		   g_nProcMapParts,											/* 処理ID(マップパーツ) */
		   g_nProcObj,												/* 処理ID(オブジェクト) */
		   g_nProcShopID,											/* 処理中の店ID */
		   g_nProcShopState,										/* 処理中の店状態 */
		   g_nProcShopSelect,										/* 選択した番号 */
		   g_nLoginAnimeX;											/* ログイン画面アニメ表示用 */

extern LPSTR g_pszServer;											/* サーバーのアドレス */
extern DWORD g_pMyID;												/* 自分のDPNID */

extern HFONT g_hFontBasic,											/* 基本文字フォント */
			 g_hFont12,												/* 12ドットフォント */
			 g_hFont14,												/* 14ドットフォント */
			 g_hFont32;												/* 32ドットフォント */
extern HWND g_hWndMain,												/* メインウィンドウ */
			g_hWndAcc,												/* アカウント名入力欄 */
			g_hWndPas,												/* パスワード入力欄 */
			g_hWndCon,												/* 接続ボタン */
			g_hWndName,												/* 名前入力欄 */
			g_hWndMake,												/* 作成ボタン */
			g_hWndSpeech,											/* 発言入力欄 */
			g_hWndDebug,											/* デバッグ情報ウィンドウ */
			g_hWndMsgLog,											/* メッセージログウィンドウ */
			g_hWndBalloon,											/* 吹き出しウィンドウ */
			g_hWndAdmin;											/* 管理者ウィンドウ */

extern RECT rcMain,													/* ウィンドウ位置（メインウィンドウ） */
			rcDebug,												/* ウィンドウ位置（デバッグ情報ウィンドウ） */
			rcMsgLog,												/* ウィンドウ位置（メッセージログウィンドウ） */
			rcBalloon,												/* ウィンドウ位置（吹き出しウィンドウ） */
			rcAdmin;												/* ウィンドウ位置（管理者ウィンドウ） */

extern WNDPROC g_OrgWndProcAcc,										/* アカウント名入力用メッセージ処理 */
			   g_OrgWndProcPas,										/* パスワード入力用メッセージ処理 */
			   g_OrgWndProcCon,										/* 接続ボタン用メッセージ処理 */
			   g_OrgWndProcName,									/* キャラ名入力欄用メッセージ処理 */
			   g_OrgWndProcMake,									/* 作成ボタン用メッセージ処理 */
			   g_OrgWndProcSpeech;									/* 発言入力欄用メッセージ処理 */

extern PMAPINFO g_pMapInfo;												/* マップ情報 */
extern PEVENTINFO g_pEventInfo;											/* イベント情報 */
extern PSUBGAMEINFO g_pSubGameInfo;										/* サブゲーム情報 */
extern PSUBGAMEINFO g_pSubGameInfoPlay;									/* 参加中サブゲーム情報 */
extern PMAPPARTSINFO g_pMapPartsInfo;									/* マップパーツ情報 */
extern PCOMEFCINFO g_pComEfcInfo;										/* 吹き出し情報 */
extern CInfoEfc g_EfcInfo;												/* エフェクト情報 */
extern CInfoJob g_JobInfo;												/* 職業情報 */
extern CProcSubGame g_ProcSubGame;										/* サブゲーム処理 */
extern FADEINOUTINFO g_FadeInOutInfo;									/* フェードインアウト情報 */
extern PLEAVEINFO g_pLeaveInfo;											/* 預かり所情報 */
extern MAKERINFO g_aMakerInfo[MAX_MAKER];								/* 作り手情報（番号がそのまま添字。状態が RECIPESTATE_ON のものが有効） */
extern RECIPEINFO g_aRecipeInfo[MAX_RECIPE];							/* レシピ情報（番号がそのまま添字） */
extern int g_nMakerCount;												/* 作り手の総数（空き枠を含む） */
extern int g_nRecipeCount;												/* レシピの総数（空き枠を含む） */
extern CMDPARA_TOWNWARP g_TownWarpInfo;								/* 町転送の行き先一覧（サーバーから届いたもの。byCount=0 なら無し） */
extern BYTE g_byTownWarpSel;										/* 町転送で選んだ行き先の番号（0=復活位置。特技を使う時に送る） */
extern int g_nProcMakerID;												/* 開いている作り手の番号（開いていなければ -1） */
extern BOOL g_bProcMakerLeaveOK;										/* 預かり所の中身（g_pLeaveInfo）を作り手の画面を開いてから受け取ったか */
extern BOOL g_bProcMakerWait;											/* 作成要求を出して、サーバーの返事待ちか */
extern DWORD g_dwProcMakerWaitTime;										/* 返事待ちを始めた時間 */
extern PTRADEINFO g_pTradeInfo;											/* 取り引き情報 */
extern PCMDPARA_WARSELECTTURN g_pWarSelectTurn;							/* 戦場ゲーム行動順序 */
extern PSUBGAMECLIENT_WARINFO g_pWarInfo;								/* 戦場ゲーム情報 */
extern PCMDPARA_WARSELECTMAGICTARGET g_pWarMagicTarget;					/* 戦場ゲーム魔法対象者一覧 */
extern PTARGETSELECTINFO g_pTargetSelectInfo;							/* ターゲット選択情報 */
extern CmyArray<PWNDINFO, PWNDINFO> g_WndInfo;							/* ウィンドウ情報 */
extern CmyArray<PCHARINFO, PCHARINFO> g_aCharInfo;						/* キャラ情報 */
extern CmyArray<PSYSMSGINFO, PSYSMSGINFO> g_SysMsgInfo;					/* システムメッセージ情報 */
extern CmyArray<PADMINRURAINFO, PADMINRURAINFO> g_AdminRuraInfo;		/* 管理人ルーラ情報 */
extern CmyArray<PITEMINFO, PITEMINFO> g_aItemInfo;						/* アイテム情報 */
extern CmyArray<DWORD, DWORD> g_aRecTbl;								/* キャラ情報要求してますテーブル */
extern CmyArray<PSTATUSTEMPLATEINFO, PSTATUSTEMPLATEINFO> g_aStTmpInfo;	/* ステータステンプレート情報 */
extern CmyArray<BYTE, BYTE> g_aFadeInOutTbl;							/* フェードイン・アウト予約テーブル */
extern ARRAYUSECHARINFO g_aUseCharInfo;									/* 誰に使うか一覧情報 */
extern CmyArray<POBJINFO, POBJINFO> g_aObjInfo;							/* オブジェクト情報 */
extern CmyArray<DWORD, DWORD> g_aPartyInfo;								/* 仲間情報 */
extern CmyArray<DWORD, DWORD> g_aCharPos[DRAW_PARTS_Y * 2 + 3];			/* キャラ座標情報 */
extern CmyArray<PMAGICINFO, PMAGICINFO> g_aMagicInfo;					/* 魔法情報 */
extern CmyArray<BYTE, BYTE> g_aMagicInfoWalk;							/* 平常状態で使える魔法 */
extern CmyArray<BYTE, BYTE> g_aMagicInfoBattle;							/* 戦闘状態で使える魔法 */
extern CmyArray<PMAGICUSERINFO, PMAGICUSERINFO> g_aMagicUserInfo;		/* 魔法利用者情報 */
extern CmyArray<PEQUUSERINFO, PEQUUSERINFO> g_aEquUserInfo;				/* 装備職業情報 */
extern CAdminWindow *g_pAdminWindow;									/* 管理者ウィンドウ */

/* 町転送 */
extern BOOL IsTownWarpMagic		(BYTE byID);						/* 町転送の特技か判定 */
extern int GetTownWarpSelectCount	(void);							/* 町転送で選べる行き先の数 */
extern void GetTownWarpLabel	(int nNo, LPSTR pszDst, int nSize);	/* 行き先の表示名を取得 */
extern LPCSTR GetTownWarpTitle	(int nMode);						/* 町転送の一覧ウィンドウの見出し */
extern BOOL TownWarpNeedForget	(DWORD dwMapID);					/* 教会で記録する時に忘れる町を選ぶ必要があるか */

/* 向き別縦横増減テーブル */
extern  int anPosAddTblX[],
			anPosAddTblY[];
/* 向き反転テーブル */
extern BYTE abyDirectionTbl[];
/* 音量設定テーブル */
extern int anVolTbl[];
/* 音量設定テーブル(MIDI) */
extern int anVolTblMIDI[];
extern char achCharEfcAddTbl[];
/* 効果音名テーブル */
extern LPSTR aszSoundName[];
/* BGMリソースIDテーブル */
extern DWORD adwBgmIDTbl[];
/* BGM番号をリソース番号に変換するテーブル */
extern DWORD adwBgmNoTbl[];
/* 効果音をリソース番号に変換するテーブル */
extern DWORD adwSoundNoTbl[];
/* アイテム種別名テーブル */
extern const char *aItemeTypeNameTbl[];
/* 装備効果テーブル */
extern const char *aEquTypeTbl[];
/* 性別種別 */
extern const char *aSexDistinctionTbl[];
/* 装備名テーブル */
extern const char *aEquNameTbl[];
/* 酔っ払いレベルテーブル */
extern const char *aDrunkTypeTbl[];
/* NPC移動種別名テーブル */
extern const char *aNpcMoveTypeTbl[];
/* 攻撃種別名テーブル */
extern const char *aAtackTypeTbl[];
/* サブゲーム名テーブル */
extern const char *aSubGameNameTbl[];
/* 魔法効果種別 */
extern LPCTSTR g_aStrType[];
/* 魔法利用情報 */
extern LPCTSTR g_aStrTypeInfo[];
/* 魔法使用対象 */
extern LPCTSTR g_aStrTarget[];
/* 魔法発動対象 */
extern LPCTSTR g_aStrTargetCount[];
/* 魔法発動種別 */
extern LPCTSTR g_aStrTargeType[];
/* 属性種別 */
extern LPCTSTR g_aStrAttrType[];
/* 名前の色テーブル */
extern COLORREF aColorTbl[];
/* システムメッセージテーブル */
extern const char *aSysMsgTbl[];
/* メッセージコマンドテーブル */
extern const char *aMsgCmdTbl[];
/* すごろくルール */
extern const char *aSugorokuRuleTbl[];
/* すごろくパーツ */
extern const char *aSugorokuPartsTbl[];
/* すごろくイベント */
extern const char *aSugorokuEventTbl[];

/* Copyright(C)URARA-works 2003-2005 */
