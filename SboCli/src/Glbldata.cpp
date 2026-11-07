/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：			グローバル変数定義										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

#include "stdafx.h"
#include "../Common/Info/InfoEfc.h"
#include "../Common/Info/InfoJob.h"
#include "Proc/ProcSubGame.h"

class CAdminWindow;

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
CDXAudio DXAudio;													/* 音再生オブジェクト */
CLibMusicLoader g_LibMusic;											/* MIDI再生ライブラリ */
CUraraSockTCP cliSock;												/* クライアントオブジェクト */
CCRC g_crc;															/* CRC用クラス */
IDirectMusicSegment8 *aSoundTbl[SOUND_MAX + 1];						/* 音テーブル */
CImg32	*g_pDibPrimary,												/* プライマリビットマップ */
		*g_pDibBack,												/* バックバッファビットマップ */
		*g_pDibSystem,												/* システムグラフィックビットマップ */
		**g_apDibChar,												/* キャラグラフィックビットマップ */
		**g_apDibCharBack,											/* キャラグラフィック保存用 */
		**g_apDibChar1632,											/* キャラグラフィック(16x32)ビットマップ */
		**g_apDibChar1632Back,										/* キャラグラフィック(16x32)保存用 */
		**g_apDibChar3232,											/* キャラグラフィック(32x32)ビットマップ */
		**g_apDibChar3232Back,										/* キャラグラフィック(32x32)保存用 */
		**g_apDibChar4848,											/* キャラグラフィック(48x48)ビットマップ */
		**g_apDibChar4848Back,										/* キャラグラフィック(48x48)保存用 */
		**g_apDibMap,												/* マップグラフィックビットマップ */
		**g_apDibMapBack,											/* マップグラフィック保存用 */
		**g_apDibEfect,												/* エフェクトグラフィックビットマップ */
		**g_apDibEfect1632,											/* エフェクトグラフィック(16x32)ビットマップ */
		**g_apDibObj3232,											/* オブジェクト(32x32)グラフィックビットマップ */
		*g_pDibMapParts,											/* マップパーツグラフィックビットマップ */
		*g_pDibDrawMap,												/* マップ描画用テンポラリビットマップ */
		*g_pDibComEfc,												/* 吹き出し用ビットマップ */
		*g_pDibComEfcWindow,										/* 吹き出しウィンドウ用ビットマップ */
		*g_pDibLogo,												/* ロゴ用ビットマップ */
		*g_pDibTitle,												/* タイトル用ビットマップ */
		*g_pDibTitleBack,											/* タイトル背景用ビットマップ */
		*g_pDibAnime,												/* アニメーション用ビットマップ */
		*g_pDibSubgBomb,											/* サブゲーム（爆弾ゲーム）用ビットマップ */
		*g_pDibWChar,												/* サブゲーム（戦場ゲーム）用ビットマップ */
		pDibDrawTmp;												/* 描画テンポラリ */

HMODULE g_hDll,														/* リソースDLLハンドル */
		g_hDllGrpData;												/* グラフィックデータDLLハンドル */
HINSTANCE g_hInstance;												/* インスタンスハンドル */
DWORD g_dwNewID,													/* 新しいID */
	  g_dwTimeDataCount,											/* 最終データ測定時間 */
	  g_dwTimeMoveDisable,											/* 移動禁止時間 */
	  g_dwTimePingLast,												/* 最終Ping送信時間 */
	  g_dwTimeLastProc,												/* 最終自キャラ処理時間 */
	  g_dwTimeLastLogo,												/* ロゴ表示時間 */
	  g_dwTimeLastMapParts,											/* 最終マップパーツ処理時間 */
	  g_dwTimeLastAnime,											/* アニメーション処理時間 */
	  g_dwTimeHartbeatReq,											/* ハートビート送信時間 */
	  g_dwTimeHartbeat,												/* ハートビート応答時間 */
	  g_dwTimeDrawTime,												/* 描画にかかった時間 */
	  g_dwTimeTextFadeInOut,										/* マップ名表示用待ち時間 */
	  g_dwTimeLastThrow,											/* 最後に投げた時間 */
	  g_dwTimeLastCursor,											/* カーソル処理時間 */
	  g_dwFpsCount,													/* 1秒間のループ回数 */
	  g_dwOnilne,													/* 参加プレイヤー数 */
	  g_dwActiveMsgID,												/* メッセージID(キー入力を行う) */
	  g_dwMsgIDLogin,												/* メッセージID(ログイン) */
	  g_dwMsgIDConnect,												/* メッセージID(接続中です) */
	  g_dwMsgIDMENU,												/* メッセージID(ログインメニュー) */
	  g_dwMsgIDMENUCHARINFO,										/* メッセージID(ログインメニューキャラ情報) */
	  g_dwMsgIDMENUCHARMAKE,										/* メッセージID(ログインメニューキャラ作成) */
	  g_dwMsgIDCommand,												/* メッセージID(コマンド) */
	  g_dwMsgIDTRADE,												/* メッセージID(取り引き) */
	  g_dwMsgIDGold,												/* メッセージID(所持金表示) */
	  g_dwMsgIDSubGame,												/* メッセージID(サブゲーム情報) */
	  g_dwSelctRura,												/* 管理人ルーラ選択用 */
	  g_dwMaxRura,													/* 管理人ルーラ記録数 */
	  g_dwCountPC,													/* マップ内PC数 */
	  g_dwCountNPC,													/* マップ内NPC数 */
	  g_dwPlayBgmNo;												/* 演奏中BGM番号 */

BYTE g_byGameState,													/* ゲーム状態 */
	 g_byFps,														/* 描画カウンタ */
	 g_byAnimeNo,													/* アニメーション番号 */
	 g_byViewMode,													/* 表示モード */
	 g_byComSelectBattle,											/* 戦闘用コマンド選択状態 */
	 g_byComSelectMagic,											/* 魔法選択状態 */
	 g_byStage,														/* 状態管理用 */
	 g_byTextFadeInOut,												/* マップ名表示状態 */
	 g_byMapGrpCountX,												/* マップパーツ数(横) */
	 g_byMapGrpCountY,												/* マップパーツ数(縦) */
	 g_byAdminAnimeNo,												/* 選択中アニメーション番号 */
	 g_byPosReqNo,													/* 座標通知シーケンス番号 */
	 g_byAdminCount,												/* 管理人数 */
	 g_byUseEfcID,													/* 発動させるエフェクトID */
	 g_byVolSound,													/* 音量（効果音） */
	 g_byVolMusic,													/* 音量（曲） */
	 g_bySpeechMode,												/* 発言モード */
	 g_bySoundCount,												/* 鐘鳴らした数 */
	 g_byCursorAnime;												/* カーソルアニメ番号 */

BOOL g_bWindowActive,												/* ウィンドウがアクティブ */
	 g_bDraw,														/* 画面更新あり */
	 g_bReqestPos,													/* 移動要求中 */
	 g_bMoving,														/* 自発的に移動中 */
	 g_bSpeech,														/* 発言入力中 */
	 g_bAdminPos,													/* 座標を取得 */
	 g_bEnableMove,													/* 歩き許可 */
	 g_bBGMONOFF,													/* 音楽ON/OFF */
	 g_bComEfcONOFF,												/* 吹き出し効果音ON/OFF */
	 g_bChatSound,												/* 発言の音ON/OFF */
	 g_bCpuMax,														/* CPUフル活動 */
	 g_bBlock,														/* 当たり判定 */
	 g_bProcShopConfirm,											/* 店:確認中 */
	 g_bTitleSkip,													/* ロゴ・タイトル飛ばす */
	 g_bMainWindowView,												/* メインウィンドウ表示切り替え */
	 g_bBalloonWindowView,											/* 吹き出しウィンドウ表示切り替え */
	 g_bSpeechView,													/* 発言徐々に消えるON/OFF */
	 g_bBattleMode,													/* 戦闘モード */
	 g_bSnowCharge,													/* 雪をつかんでいる？ */
	 g_bHartbeatCheck,												/* ハートビートチェック用 */
	 g_bMapMarkView,												/* マップ編集当たり判定マーク */
	 g_bWeatherDraw;												/* 天候描画判定 */

WORD g_wPort,														/* 接続ポート番号 */
	 g_wCountCharGrp,												/* 使用キャラグラフィック数 */
	 g_wCountCharGrp1632,											/* 使用キャラ(16x32)グラフィック数 */
	 g_wCountCharGrp3232,											/* 使用キャラ(32x32)グラフィック数 */
	 g_wCountCharGrp4848,											/* 使用キャラ(48x48)グラフィック数 */
	 g_wCountMapPartsGrp,											/* 使用マップパーツグラフィック数 */
	 g_wCountMapParts,												/* 使用マップパーツ数 */
	 g_wCountEfcGrp,												/* 使用エフェクトグラフィック数 */
	 g_wCountEfcGrp1632,											/* 使用エフェクト(16x32)グラフィック数 */
	 g_wCountObjGrp,												/* 使用オブジェクト(32x32)グラフィック数 */
	 g_wLastProcSecond;												/* 自キャラ最終処理秒 */

char g_szAcc[MAX_ACCNAME],											/* アカウント名 */
	 g_szPas[MAX_ACCNAME],											/* パスワード */
	 g_szMacAdr[6],													/* MACアドレス */
	 *g_pMapDarkTmp,												/* 暗闇描画用テンポラリ */
	 *g_pMapDarkTmpPos;												/* 暗闇描画用テンポラリ */
int g_nMyCharID,													/* 自分のキャラ番号 */
	g_nSelectPartsID,												/* 選択中のパーツID */
	g_nSelectEventID,												/* 選択中のイベントID */
	g_nSelectSubGameID,												/* 選択中のサブゲームID */
	g_nAdminWindowType,												/* 表示中の管理者ウィンドウ種別 */
	g_nEventCount,													/* イベント登録数 */
	g_nSubGameCount,												/* サブゲーム登録数 */
	g_nProcMsg,														/* 処理ID(メッセージ) */
	g_nProcChar,													/* 処理ID(キャラ) */
	g_nProcSysMsg,													/* 処理ID(システムメッセージ) */
	g_nProcMapParts,												/* 処理ID(マップパーツ) */
	g_nProcObj,														/* 処理ID(オブジェクト) */
	g_nProcShopID,													/* 処理中の店ID */
	g_nProcShopState,												/* 処理中の店状態 */
	g_nProcShopSelect,												/* 選択した番号 */
	g_nLoginAnimeX;													/* ログイン画面アニメ表示用 */

LPSTR g_pszServer;													/* サーバーのアドレス */
DWORD g_pMyID = NULL;												/* 自分のID */

HFONT g_hFontBasic,													/* 基本文字フォント */
	  g_hFont12,													/* 12ドットフォント */
	  g_hFont14,													/* 14ドットフォント */
	  g_hFont32;													/* 32ドットフォント */

HWND g_hWndMain,													/* メインウィンドウ */
	 g_hWndAcc,														/* アカウント名入力欄 */
	 g_hWndPas,														/* パスワード入力欄 */
	 g_hWndCon,														/* 接続ボタン */
	 g_hWndName,													/* 名前入力欄 */
	 g_hWndMake,													/* 作成ボタン */
	 g_hWndSpeech,													/* 発言入力欄 */
	 g_hWndDebug,													/* デバッグ情報ウィンドウ */
	 g_hWndMsgLog,													/* メッセージログウィンドウ */
	 g_hWndBalloon,													/* 吹き出しウィンドウ */
	 g_hWndAdmin;													/* 管理者ウィンドウ */

RECT rcMain,														/* ウィンドウ位置（メインウィンドウ） */
	 rcDebug,														/* ウィンドウ位置（デバッグ情報ウィンドウ） */
	 rcMsgLog,														/* ウィンドウ位置（メッセージログウィンドウ） */
	 rcBalloon,														/* ウィンドウ位置（吹き出しウィンドウ） */
	 rcAdmin;														/* ウィンドウ位置（管理者ウィンドウ） */

WNDPROC g_OrgWndProcAcc,											/* アカウント名入力用メッセージ処理 */
		g_OrgWndProcPas,											/* パスワード入力用メッセージ処理 */
		g_OrgWndProcCon,											/* 接続ボタン用メッセージ処理 */
		g_OrgWndProcName,											/* キャラ名入力欄用メッセージ処理 */
		g_OrgWndProcMake,											/* 作成ボタン用メッセージ処理 */
		g_OrgWndProcSpeech;											/* 発言入力欄用メッセージ処理 */

PMAPINFO g_pMapInfo;												/* マップ情報 */
PEVENTINFO g_pEventInfo;											/* イベント情報 */
PSUBGAMEINFO g_pSubGameInfo;										/* サブゲーム情報 */
PSUBGAMEINFO g_pSubGameInfoPlay;									/* 参加中サブゲーム情報 */
PMAPPARTSINFO g_pMapPartsInfo;										/* マップパーツ情報 */
PCOMEFCINFO g_pComEfcInfo;											/* 吹き出し情報 */
CInfoEfc g_EfcInfo;													/* エフェクト情報 */
CInfoJob g_JobInfo;													/* 職業情報 */
CProcSubGame g_ProcSubGame;											/* サブゲーム処理 */
FADEINOUTINFO g_FadeInOutInfo;										/* フェードインアウト情報 */
PLEAVEINFO g_pLeaveInfo;											/* 預かり所情報 */
PTRADEINFO g_pTradeInfo;											/* 取り引き情報 */
PCMDPARA_WARSELECTTURN g_pWarSelectTurn;							/* 戦場ゲーム行動順序 */
PSUBGAMECLIENT_WARINFO g_pWarInfo;									/* 戦場ゲーム情報 */
PCMDPARA_WARSELECTMAGICTARGET g_pWarMagicTarget;					/* 戦場ゲーム魔法対象者一覧 */
PTARGETSELECTINFO g_pTargetSelectInfo;								/* ターゲット選択情報 */
CmyArray<PWNDINFO, PWNDINFO> g_WndInfo;								/* ウィンドウ情報 */
CmyArray<PCHARINFO, PCHARINFO> g_aCharInfo;							/* キャラ情報 */
CmyArray<PSYSMSGINFO, PSYSMSGINFO> g_SysMsgInfo;					/* システムメッセージ情報 */
CmyArray<PADMINRURAINFO, PADMINRURAINFO> g_AdminRuraInfo;			/* 管理人ルーラ情報 */
CmyArray<PITEMINFO, PITEMINFO> g_aItemInfo;							/* アイテム情報 */
CmyArray<DWORD, DWORD> g_aRecTbl;									/* キャラ情報要求してますテーブル */
CmyArray<PSTATUSTEMPLATEINFO, PSTATUSTEMPLATEINFO> g_aStTmpInfo;	/* ステータステンプレート情報 */
CmyArray<BYTE, BYTE> g_aFadeInOutTbl;								/* フェードイン・アウト予約テーブル */
ARRAYUSECHARINFO g_aUseCharInfo;									/* 誰に使うか一覧情報 */
CmyArray<POBJINFO, POBJINFO> g_aObjInfo;							/* オブジェクト情報 */
CmyArray<DWORD, DWORD> g_aPartyInfo;								/* 仲間情報 */
CmyArray<DWORD, DWORD> g_aCharPos[DRAW_PARTS_Y * 2 + 3];			/* キャラ座標情報 */
CmyArray<PMAGICINFO, PMAGICINFO> g_aMagicInfo;						/* 魔法情報 */
CmyArray<BYTE, BYTE> g_aMagicInfoWalk;								/* 平常状態で使える魔法 */
CmyArray<BYTE, BYTE> g_aMagicInfoBattle;							/* 戦闘状態で使える魔法 */
CmyArray<PMAGICUSERINFO, PMAGICUSERINFO> g_aMagicUserInfo;			/* 魔法利用者情報 */
CmyArray<PEQUUSERINFO, PEQUUSERINFO> g_aEquUserInfo;				/* 装備職業情報 */
CAdminWindow *g_pAdminWindow;										/* 管理者ウィンドウ */


/* 向き別縦横増減テーブル */
int anPosAddTblX[] = {0, 0, -1, 1},
	anPosAddTblY[] = {-1, 1, 0, 0};

/* 向き反転テーブル */
BYTE abyDirectionTbl[] = {1, 0, 3, 2};

/* 音量設定テーブル */
int anVolTbl[] = {-127, -15, -7, -1, 0};

/* 音量設定テーブル(MIDI) */
int anVolTblMIDI[] = {-127, -60, -40, -25, 10};

/* 文字エフェクト表示位置増減テーブル */
char achCharEfcAddTbl[] = {-4, -4, -4, -2, -2, 2, 2, 2, 2, 2, 2, 99};

/* 効果音名テーブル */
LPSTR aszSoundName[] = {"音無し", "決定", "ぶつかり音", "移動", "メッセージ",
				    "落ちる", "扉", "宝箱", "レベルアップ", "トイレ流す",
				    "こちら攻撃", "会心の一撃", "あちら攻撃", "ダメージ",
				    "呪文", "ちょっとしたBGM", "復活", "失敗", "へぇ",
				    "サイレン", "ピンポンパンポン", "ホイッスル", "歓声",
				    "ぐー", "ちょき", "ぱー", "ルーラ", "拍手", "確認",
					"BOM25", "BOM28_A", "GUN08", "GUN16",
					"BURST00", "CLOSE01", "VOICE011", "POWER01",
					"POWER01_B", "PYORO33", "PYORO57_B", "POWER26",
					"POWER32", "GUN10", "GORO02", "GORO03",
					"GUN11", "POWER10", "FREEZE02", "FREEZE00",
					"PYORO08", "HIT12", "POWER17", "POWER14",
					"PUU23", "SMOKE03", "FREEZE01", "FREEZE03",
					"POWER07", "POWER16", "PUU27", "MECHA09",
					"MECHA09_R", "POWER23", "PUU38", "BOM17_A",
					"DON07", "GUN14_C", "BOM14", "GUN13_B",
					"BOM26_A", "BOM26_C", "PUSH40", "PUU06",
					"PUU09", "PUU10", "DON00", "DRAG02",
					"HIT_S02", "HIT17", "HIT65", "HIT67",
					"HITS01_B", "POWER22", "POWER09", "POWER19",
					"POWER33", "HIS_S06", "BOSU09", "HIT_b02_B",
					"SWING36_B", "HIT07", "HIT10_B", "HIT25",
					"HIT26", "SHA00", "SHA04", "FREEZE04",
					"SWING14", "HIT68", "HIT70", "波の音",
					"攻撃ミス", "ダメージ0", "爆弾設置", "爆弾爆発",
					"キャンセル・戻る", "ターゲット指定", "コマンド呼び出し",
					"爆弾ｹﾞｰﾑ用ﾊﾟﾜｰｱｯﾌﾟ", "爆弾ｹﾞｰﾑ用ﾊﾟﾜｰﾀﾞｳﾝ", "アイテムドロップ",
					"猫の鳴き声", "CRASH18_C", "DON12", "FM003", "LIP01", "SWING32",
					"鐘", "レベルアップ2", "VOICE012", "VOICE028", "BYORO02a", "BYORO06a",
					"プクゥのような", "プクゥのようなもの", "音波", "気弾", "気弾ヒット",
					"聖なる光のレーザー", "土属性（弱", "土属性（大", "土属性ヒット", "音波攻撃",
					"$142", "gun24", "lvup_sound", "三連撃", "三連斬", "十字突き", "十字突き２",
					"召喚", "大爆発", "罠設置", "薔薇の舞", "土大",
					"発言", "z"};

/* BGMリソースIDテーブル */
DWORD adwBgmIDTbl[] = {
	0,
	IDR_WAVE_OK,		IDR_WAVE_BLOCK,		IDR_WAVE_MOVE,		IDR_WAVE_MSG,
	IDR_WAVE_DOWN,		IDR_WAVE_DOOR,		IDR_WAVE_BOX,		IDR_WAVE_LVUP,
	IDR_WAVE_WC,		IDR_WAVE_ATACK1,	IDR_WAVE_ATACK2,	IDR_WAVE_ATACK3,
	IDR_WAVE_DAMAGE,	IDR_WAVE_MAGIC,		IDR_MIDI_BATTLE1,	IDR_MIDI_BOSS1,
	IDR_MIDI_BOSS2,		IDR_MIDI_CASTLE1,	IDR_MIDI_CASTLE2,	IDR_MIDI_CASTLE3,
	IDR_MIDI_CHURCH1,	IDR_MIDI_CHURCH2,	IDR_MIDI_CLIMAX,	IDR_MIDI_FIELD1,
	IDR_MIDI_FIELD2,	IDR_MIDI_FIELD3,	IDR_MIDI_FIELD4,	IDR_MIDI_FIELD5,
	IDR_MIDI_FIELD6,	IDR_MIDI_LASTBATTLE,IDR_MIDI_MEMORY,	IDR_MIDI_MENU,
	IDR_MIDI_MISSION,	IDR_MIDI_MOUNTAIN,	IDR_MIDI_PINCH1,	IDR_MIDI_PINCH2,
	IDR_MIDI_SANCT,		IDR_MIDI_SHRINE,	IDR_MIDI_TOWER1,	IDR_MIDI_TOWER2,
	IDR_MIDI_TOWER3,	IDR_MIDI_TOWN1,		IDR_MIDI_TOWN1NIGHT,IDR_MIDI_TOWN2,
	IDR_MIDI_VILLAGE,	IDR_MIDI_CLOCK,		IDR_MIDI_HALF,		IDR_MIDI_MBOX007,
	IDR_MIDI_NEKO,		IDR_MIDI_OPENING,	IDR_MIDI_ORGANEXACISE,	IDR_MIDI_TIRUTOKI,
	IDR_MIDI_TOMADOI,	IDR_WAVE_BARD,		IDR_WAVE_CURE,		IDR_WAVE_FAIL,
	IDR_WAVE_HEE,		IDR_WAVE_SIREN,		IDR_WAVE_PINPON,	IDR_WAVE_WHISTLE,
	IDR_WAVE_VOICE,		IDR_WAVE_GU,		IDR_WAVE_CHOKI,		IDR_WAVE_PA,
	IDR_WAVE_RURA,		IDR_MIDI_MBOX001,	IDR_MIDI_CREAM,		IDR_MIDI_NATUNO,
	IDR_WAVE_HAKUSYU,	IDR_WAVE_CONFIRM,	IDR_MIDI_FOREST,	IDR_WAVE_BOM25,
	IDR_WAVE_BOM28_A,	IDR_WAVE_GUN08,		IDR_WAVE_GUN16,		IDR_WAVE_BURST00,
	IDR_WAVE_CLOSE01,	IDR_WAVE_VOICE011,	IDR_WAVE_POWER01,	IDR_WAVE_POWER01_B,
	IDR_WAVE_PYORO33,	IDR_WAVE_PYORO57_B,	IDR_WAVE_POWER26,	IDR_WAVE_POWER32,
	IDR_WAVE_GUN10,		IDR_WAVE_GORO02,	IDR_WAVE_GORO03,	IDR_WAVE_GUN11,
	IDR_WAVE_POWER10,	IDR_WAVE_FREEZE02,	IDR_WAVE_FREEZE00,	IDR_WAVE_PYORO08,
	IDR_WAVE_HIT12,		IDR_WAVE_POWER17,	IDR_WAVE_POWER14,	IDR_WAVE_PUU23,
	IDR_WAVE_SMOKE03,	IDR_WAVE_FREEZE01,	IDR_WAVE_FREEZE03,	IDR_WAVE_POWER07,
	IDR_WAVE_POWER16,	IDR_WAVE_PUU27,		IDR_WAVE_MECHA09,	IDR_WAVE_MECHA09_R,
	IDR_WAVE_POWER23,	IDR_WAVE_PUU38,		IDR_WAVE_BOM17_A,	IDR_WAVE_DON07,
	IDR_WAVE_GUN14_C,	IDR_WAVE_BOM14,		IDR_WAVE_GUN13_B,	IDR_WAVE_BOM26_A,
	IDR_WAVE_BOM26_C,	IDR_WAVE_PUSH40,	IDR_WAVE_PUU06,		IDR_WAVE_PUU09,
	IDR_WAVE_PUU10,		IDR_WAVE_DON00,		IDR_WAVE_DRAG02,	IDR_WAVE_HIT_S02,
	IDR_WAVE_HIT17,		IDR_WAVE_HIT65,		IDR_WAVE_HIT67,		IDR_WAVE_HIT_S01_B,
	IDR_WAVE_POWER22,	IDR_WAVE_POWER09,	IDR_WAVE_POWER19,	IDR_WAVE_POWER33,
	IDR_WAVE_HIT_S06,	IDR_WAVE_BOS09,		IDR_WAVE_HIT_P02_B,	IDR_WAVE_SWING36_B,
	IDR_WAVE_HIT07,		IDR_WAVE_HIT10_B,	IDR_WAVE_HIT25,		IDR_WAVE_HIT26,
	IDR_WAVE_SHA00,		IDR_WAVE_SHA04,		IDR_WAVE_FREEZE04,	IDR_WAVE_SWING14,
	IDR_WAVE_HIT68,		IDR_WAVE_HIT70,		IDR_WAVE_SEE,		IDR_MIDI_BOKYO,
	IDR_MIDI_AIRSHIP1,	IDR_MIDI_MAIN2,		IDR_WAVE_ATACKMISS,	IDR_WAVE_DAMAGE0,
	IDR_WAVE_BOMBSET,	IDR_WAVE_BOMBBOMBER,IDR_MIDI_YUKIZATO,	IDR_MIDI_BACATION,
	IDR_MIDI_BATTLE,	IDR_MIDI_DANCE,		IDR_MIDI_DENNEN,	IDR_MIDI_DESTRATION,
	IDR_MIDI_DREAMING,	IDR_MIDI_KAZAN,		IDR_MIDI_KOUJOUTITAI,IDR_MIDI_MINAMI,
	IDR_MIDI_OYASUMI03,	IDR_MIDI_RYOSHU,	IDR_MIDI_SINSIN,	IDR_MIDI_SNOW,
	IDR_MIDI_YELLOW,	IDR_MIDI_ANSWER,	IDR_MIDI_NOUSONN,	IDR_WAVE_PUSH51_C,
	IDR_WAVE_SHOOT15,	IDR_WAVE_CURSOR08,	IDR_WAVE_POWER21,	IDR_WAVE_FM001,
	IDR_WAVE_PI52,		IDR_MIDI_W0053,		IDR_MIDI_W0054,		IDR_MIDI_W0077,
	IDR_MIDI_W0035,		IDR_MIDI_W0021,		IDR_MIDI_W0068,		IDR_MIDI_W0102,
	IDR_MIDI_W0030,		IDR_MIDI_W0075,		IDR_MIDI_W0016,		IDR_MIDI_W0040,
	IDR_MIDI_W0069,		IDR_MIDI_W0071,		IDR_MIDI_W0014,		IDR_MIDI_W0113,
	IDR_MIDI_W0081,		IDR_WAVE_CAT,		IDR_MIDI_W0020,		IDR_MIDI_W0029,
	IDR_MIDI_W0039,		IDR_MIDI_W0041,		IDR_MIDI_W0057,		IDR_MIDI_W0067,
	IDR_MIDI_W0120,		IDR_MIDI_W0123,		IDR_MIDI_W0125,		IDR_MIDI_W0130,
	IDR_MIDI_W0136,		IDR_MIDI_WJ013,		IDR_MIDI_WJ014,		IDR_MIDI_WS003,
	IDR_MIDI_W0043,		IDR_WAVE_CRASH18_C,	IDR_WAVE_DON12,		IDR_WAVE_FM003,
	IDR_WAVE_LIP01,		IDR_WAVE_SWING32,	IDR_WAVE_KANE,		IDR_MIDI_KIRAKIRA,
	IDR_MIDI_SILENT,	IDR_MIDI_TIRUFUSE,	IDR_MIDI_HAPPYBIRTHDAY,
	IDR_MIDI_MIDIJINGLEBELL, IDR_MIDI_SHOGATU,	IDR_MIDI_S_NIGHT_S,
	IDR_MIDI_JANGURU,	IDR_WAVE_LVUP,		
	IDR_MIDI_OMOIDE,	IDR_MIDI_W0101,		IDR_MIDI_W0045,		IDR_MIDI_W0064,
	IDR_MIDI_W0132,		IDR_WAVE_VOICE012,	IDR_WAVE_VOICE028,	IDR_WAVE_BYORO02A,
	IDR_WAVE_BYORO06A,	IDR_WAVE_PUQU1,		IDR_WAVE_PUQU2,		IDR_WAVE_ONPA,
	IDR_WAVE_KIDAN,		IDR_WAVE_KIDANHIT,	IDR_WAVE_LASER,		IDR_WAVE_TUTI1,
	IDR_WAVE_TUTI2,		IDR_WAVE_TUTIHIT,	IDR_WAVE_ONPAATACK,	IDR_WAVE_EFFECT142,
	IDR_WAVE_GUN24,		IDR_WAVE_LVUP_SOUND,IDR_WAVE_SANRENGEKI,IDR_WAVE_SANRENZAN,
	IDR_WAVE_JUJITUKI,	IDR_WAVE_JUJITUKI2,	IDR_WAVE_SYOKAN,	IDR_WAVE_DAIBAKUHATU,
	IDR_WAVE_TRAP,		IDR_WAVE_BARA,		IDR_WAVE_TUTIDAI,	IDR_MIDI_WO002,
	IDR_MIDI_WO006,		IDR_MIDI_W0009,		IDR_MIDI_W0100,		IDR_WAVE_CHAT,
};

/* BGM番号をリソース番号に変換するテーブル */
DWORD adwBgmNoTbl[] = {
	0,
	SOUND_MIDI_BATTLE1,		SOUND_MIDI_BOSS1,		SOUND_MIDI_BOSS2,
	SOUND_MIDI_CASTLE1,		SOUND_MIDI_CASTLE2,		SOUND_MIDI_CASTLE3,
	SOUND_MIDI_CHURCH1,		SOUND_MIDI_CHURCH2,		SOUND_MIDI_CLIMAX,
	SOUND_MIDI_FIELD1,		SOUND_MIDI_FIELD2,		SOUND_MIDI_FIELD3,
	SOUND_MIDI_FIELD4,		SOUND_MIDI_FIELD5,		SOUND_MIDI_FIELD6,
	SOUND_MIDI_LASTBATTLE,	SOUND_MIDI_MEMORY,		SOUND_MIDI_MENU,
	SOUND_MIDI_MISSION,		SOUND_MIDI_MOUTAIN,		SOUND_MIDI_PINCH1,
	SOUND_MIDI_PINCH2,		SOUND_MIDI_SANCT,		SOUND_MIDI_SHRINE,
	SOUND_MIDI_TOWER1,		SOUND_MIDI_TOWER2,		SOUND_MIDI_TOWER3,
	SOUND_MIDI_TOWN1,		SOUND_MIDI_TOWN1NIGHT,	SOUND_MIDI_TOWN2,
	SOUND_MIDI_VILLAGE,		SOUND_MIDI_CLOCK,		SOUND_MIDI_HALF,
	SOUND_MIDI_MBOX007,		SOUND_MIDI_NEKO,		SOUND_MIDI_OPENING,
	SOUND_MIDI_ORGANEXACISE,SOUND_MIDI_TIRUTOKI,	SOUND_MIDI_TOMADOI,
	SOUND_MIDI_MBOX001,		SOUND_MIDI_CREAM,		SOUND_MIDI_NATUNO,
	SOUND_MIDI_FOREST,		SOUND_MIDI_BOKYO,		SOUND_MIDI_AIRSHIP1,
	SOUND_MIDI_MAIN2,		SOUND_MIDI_YUKIZATO,	SOUND_MIDI_BACATION,
	SOUND_MIDI_BATTLE,		SOUND_MIDI_DANCE,		SOUND_MIDI_DENNEN,
	SOUND_MIDI_DESTRATION,	SOUND_MIDI_DREAMING,	SOUND_MIDI_KAZAN,
	SOUND_MIDI_KOUJOUTITAI,	SOUND_MIDI_MINAMI,		SOUND_MIDI_OYASUMI03,
	SOUND_MIDI_RYOSHU,		SOUND_MIDI_SINSIN,		SOUND_MIDI_SNOW,
	SOUND_MIDI_YELLOW,		SOUND_MIDI_ANSWER,		SOUND_MIDI_NOUSONN,
	SOUND_MIDI_W0053,		SOUND_MIDI_W0054,		SOUND_MIDI_W0077,
	SOUND_MIDI_W0035,		SOUND_MIDI_W0021,		SOUND_MIDI_W0068,
	SOUND_MIDI_W0102,		SOUND_MIDI_W0030,		SOUND_MIDI_W0075,
	SOUND_MIDI_W0016,		SOUND_MIDI_W0040,		SOUND_MIDI_W0069,
	SOUND_MIDI_W0071,		SOUND_MIDI_W0014,		SOUND_MIDI_W0113,
	SOUND_MIDI_W0081,		SOUND_MIDI_W0020,		SOUND_MIDI_W0029,
	SOUND_MIDI_W0039,		SOUND_MIDI_W0041,		SOUND_MIDI_W0057,
	SOUND_MIDI_W0067,		SOUND_MIDI_W0120,		SOUND_MIDI_W0123,
	SOUND_MIDI_W0125,		SOUND_MIDI_W0130,		SOUND_MIDI_W0136,
	SOUND_MIDI_WJ013,		SOUND_MIDI_WJ014,		SOUND_MIDI_WS003,
	SOUND_MIDI_W0043,		SOUND_MIDI_KIRAKIRA,	SOUND_MIDI_SILENT,
	SOUND_MIDI_TIRUFUSE,	SOUND_MIDI_HAPPYBIRTHDAY,
	SOUND_MIDI_JINGLEBELL,	SOUND_MIDI_SHOGATU,		SOUND_MIDI_S_NIGHT_S,
	SOUND_MIDI_JANGURU,		
	SOUND_MIDI_OMOIDE,		SOUND_MIDI_W0101,		SOUND_MIDI_W0045,
	SOUND_MIDI_W0064,		SOUND_MIDI_W0132,		SOUND_MIDI_WO002,
	SOUND_MIDI_WO006,		SOUND_MIDI_W0009,		SOUND_MIDI_W0100,
	0xFFFFFFFF
};

/* 効果音をリソース番号に変換するテーブル */
DWORD adwSoundNoTbl[] = {
	0,
	SOUND_OK,		SOUND_BLOCK,		SOUND_MOVE,			SOUND_MSG,
	SOUND_DOWN,		SOUND_DOOR,			SOUND_BOX,			SOUND_LVUP,
	SOUND_WC,		SOUND_ATACK1,		SOUND_ATACK2,		SOUND_ATACK3,
	SOUND_DAMAGE,	SOUND_MAGIC,		SOUND_BARD,			SOUND_CURE,
	SOUND_FAIL,		SOUND_HEE,			SOUND_SIREN,		SOUND_PINPON,
	SOUND_WHISTLE,	SOUND_VOICE,		SOUND_GU,			SOUND_CHOKI,
	SOUND_PA,		SOUND_RURA,			SOUND_HAKUSYU,		SOUND_CONFIRM,
	SOUND_BOM25,	SOUND_BOM28_A,		SOUND_GUN08,		SOUND_GUN16,
	SOUND_BURST00,	SOUND_CLOSE01,		SOUND_VOICE011,		SOUND_POWER01,
	SOUND_POWER01_B,	SOUND_PYORO33,	SOUND_PYORO57_B,	SOUND_POWER26,
	SOUND_POWER32,	SOUND_GUN10,	SOUND_GORO02,	SOUND_GORO03,
	SOUND_GUN11,	SOUND_POWER10,	SOUND_FREEZE02,	SOUND_FREEZE00,
	SOUND_PYORO08,	SOUND_HIT12,	SOUND_POWER17,	SOUND_POWER14,
	SOUND_PUU23,	SOUND_SMOKE03,	SOUND_FREEZE01,	SOUND_FREEZE03,
	SOUND_POWER07,	SOUND_POWER16,	SOUND_PUU27,	SOUND_MECHA09,
	SOUND_MECHA09_R,	SOUND_POWER23,	SOUND_PUU38,	SOUND_BOM17_A,
	SOUND_DON07,	SOUND_GUN14_C,	SOUND_BOM14,	SOUND_GUN13_B,
	SOUND_BOM26_A,	SOUND_BOM26_C,	SOUND_PUSH40,	SOUND_PUU06,
	SOUND_PUU09,	SOUND_PUU10,	SOUND_DON00,	SOUND_DRAG02,
	SOUND_HIT_S02,	SOUND_HIT17,	SOUND_HIT65,	SOUND_HIT67,
	SOUND_HITS01_B,	SOUND_POWER22,	SOUND_POWER09,	SOUND_POWER19,
	SOUND_POWER33,	SOUND_HIS_S06,	SOUND_BOSU09,	SOUND_HIT_b02_B,
	SOUND_SWING36_B,	SOUND_HIT07,	SOUND_HIT10_B,	SOUND_HIT25,
	SOUND_HIT26,	SOUND_SHA00,	SOUND_SHA04,	SOUND_FREEZE04,
	SOUND_SWING14,	SOUND_HIT68,	SOUND_HIT70,	SOUND_SEE,
	SOUND_ATACKMISS,SOUND_DAMAGE0,	SOUND_BOMBSET,	SOUND_BOMBBOMBER,
	SOUND_PUSH51_C,	SOUND_SHOOT15,	SOUND_CURSOR08,	SOUND_POWER21,
	SOUND_FM001,	SOUND_PI52,		SOUND_CAT,		SOUND_CRASH18_C,
	SOUND_DON12,	SOUND_FM003,	SOUND_LIP01,	SOUND_SWING32,
	SOUND_KANE,		SOUND_LVUP2,	SOUND_VOICE012,	SOUND_VOICE028,
	SOUND_BYORO02A,	SOUND_BYORO06A,	SOUND_PUQU1,	SOUND_PUQU2,
	SOUND_ONPA,		SOUND_KIDAN,	SOUND_KIDANHIT,	SOUND_LASER,
	SOUND_TUTI1,	SOUND_TUTI2,	SOUND_TUTIHIT,	SOUND_ONPAATACK,
	SOUND_EFFECT142,SOUND_GUN24,	SOUND_LVUP_SOUND,SOUND_SANRENGEKI,
	SOUND_SANRENZAN,SOUND_JUJITUKI,	SOUND_JUJITUKI2,SOUND_SYOKAN,
	SOUND_DAIBAKUHATU,SOUND_TRAP,	SOUND_BARA,		SOUND_TUTIDAI,
	SOUND_CHAT,
	0xFFFFFFFF
};

/* アイテム種別名テーブル */
const char *aItemeTypeNameTbl[] = {
	"効果無し",
	"特定位置へ移動",
	"HP回復",
	"武器",
	"鎧",
	"盾",
	"兜",
	"種",
	"サブゲーム参加",
	"酒",
	"メッセージ表示",
	"ｷｬﾗ画像変更",
	"明かり",
	"HP減少",
	"魔法習得",
	"Mana回復",
	"HP・Mana回復",
	"装備(小手)",
	"装備(靴)",
	"装備(首飾り)",
	"装備(指輪1)",
	"装備(指輪2)",
	"蘇生",
	"特技発動",
	"z"
};

/* 装備効果テーブル */
const char *aEquTypeTbl[] = {
	"効果無し",
	"守備力",
	"魔法防御",
	"z"
};

/* 性別種別 */
const char *aSexDistinctionTbl[] = {
	"全員",
	"男性のみ",
	"女性のみ"
};

/* 装備名テーブル */
const char *aEquNameTbl[] = {
	"武器",
	"鎧",
	"盾",
	"兜",
	"小手",
	"靴",
	"首飾り",
	"指輪1",
	"指輪2",
	"未定6",
};

/* 酔っ払いレベルテーブル */
const char *aDrunkTypeTbl[] = {
	"平常",
	"陶酔",
	"泥酔",
	"狂酔",
	"z"
};

/* NPC移動種別名テーブル */
const char *aNpcMoveTypeTbl[] = {
	"未設定", "動かない", "ランダム移動", "押せる", "サイコロ", "NPC発生", "得点板",
	"話すとランダム", "止まるかランダム", "サイコロ(8面)", "サイコロ(7面)", "戦闘１",
	"検索して追尾", "戦闘２", "通り抜け可", "爆弾", "重なったキャラに魔法発動",
	"重なったキャラに攻撃", "壁(爆弾G)", "アイテム(爆弾G)", "魔法発動し続ける", "押せて話すと切り替え", "reserve",
	"検索して追尾2", "z"
};

/* 攻撃種別名テーブル */
const char *aAtackTypeTbl[] = {
	"直接攻撃", "飛び道具", "なぎ払い", "貫通", "z"
};

/* サブゲーム名テーブル */
const char *aSubGameNameTbl[] = {
	"未設定",
	"爆弾ゲーム",
	"迷路",
	"ダンジョン",
	"戦場ゲーム",
	"すごろく",
	"z"
};

/* 魔法効果種別 */
LPCTSTR g_aStrType[] = {
	"効果無し",
	"HP減少",
	"HP増加",
	"明かり",
	"魔法発生",
	"帰還",
	"HP減少(攻撃力)",
	"魔法発生(自分)",
	"蘇生",
	"HP減少(HP消費)",
	"HP減少(率)",
	"加速",
	"毒",
	"眠り",
	"z"};

/* 魔法利用情報 */
LPCTSTR g_aStrTypeInfo[] = {"平常状態", "戦闘状態", "平常・戦闘状態", "z"};
/* 魔法使用対象 */
LPCTSTR g_aStrTarget[] = {"PC", "NPC", "両方", "z"};
/* 魔法発動対象 */
LPCTSTR g_aStrTargetCount[] = {"一人", "全員", "z"};
/* 魔法発動種別 */
LPCTSTR g_aStrTargeType[] = {"自分", "前方", "周囲", "画面内", "前方横一列", "選択", "選択(自分以外)", "z"};
/* 属性種別 */
LPCTSTR g_aStrAttrType[] = {"無し", "火", "水", "風", "土", "光", "闇", "z"};

/* 名前の色テーブル */
COLORREF aColorTbl[] = {
	RGB (255, 200, 200),	/* 赤 */
	RGB (255, 255, 200),	/* 黄 */
	RGB (200, 255, 200),	/* 緑 */
	RGB (200, 255, 255),	/* 青緑 */
	RGB (200, 200, 255),	/* 青 */
	RGB (255, 200, 255),	/* 紫 */
	RGB (255, 255, 255),	/* 白 */
/* 管理人、NPC用の濃い目の色 */
	RGB (255, 150, 150),	/* 赤（ピンク） */
	RGB (255, 200, 100),	/* 橙 */
	RGB (150, 150, 255),	/* 青 */
	RGB (200, 100, 255),	/* 紫 */
	RGB (200, 255, 150),	/* 黄緑 */
	RGB (150, 255, 150),	/* 緑 */
	RGB (100, 200, 255),	/* 水 */
	0
};

/* システムメッセージテーブル */
const char *aSysMsgTbl[] = {
	"",
	"%sの攻撃",							/* SYSMSG_ATACK1 */
	"%sのクリティカルヒット",			/* SYSMSG_ATACK2 */
	"%sに%dのダメージ",					/* SYSMSG_DAMAGE1 */
	"ミス",								/* SYSMSG_MISS */
	"ダメージを与えられませんでした",	/* SYSMSG_NODAMAGE */
	"%sを倒しました",					/* SYSMSG_BATTLEEND1 */
	"%sは%sに倒されました",				/* SYSMSG_BATTLEEND2 */
	"%sは%uポイントの経験値を得ました",	/* SYSMSG_GETEXP */
	"%sは%uゴールド手に入れました",		/* SYSMSG_BATTLEEND3 */
	"%sは死んでしまいました",			/* SYSMSG_DEAD */
	"ターゲットを%sに設定",				/* SYSMSG_TARGET1 */
	"ターゲットを解除",					/* SYSMSG_TARGET2 */
	"%sは%sを持っていました",			/* SYSMSG_GETITEM1 */
	"しかし持てないのであきらめました",	/* SYSMSG_GETITEM2 */
	"%sは%sを手に入れました",			/* SYSMSG_GETITEM3 */
	"%sには効果がありません",			/* SYSMSG_NOPROC */
	"%sのHPは完全に回復しました",		/* SYSMSG_HPMAX */
	"%sのHPが%dポイント回復しました",	/* SYSMSG_HPADD */
	"ここで使う必要はありません",		/* SYSMSG_NOPROC2 */
	"ここでは使えません",				/* SYSMSG_NOUSE */
	"%sは%sを使用しました",				/* SYSMSG_USESPECIALTY */
	"%sは生き返りました"				/* SYSMSG_RESURRECTION */
};

/* メッセージコマンドテーブル */
const char *aMsgCmdTbl[] = {
	"",
	"dice",								/* サイコロ(1～6) */
	"keno",								/* ケノ(1～80) */
	"coin",								/* コイン(裏表) */
	"wdice",							/* 戦場ゲームコマンド */
	"2d6",								/* サイコロ(1～6を2個) */
	"3d6",								/* サイコロ(1～6を3個) */
	"1d10",								/* サイコロ(1～10) */
	"1d20",								/* サイコロ(1～20) */
	"now",								/* 現在時刻 */
	"where",							/* 最も人が集まっている場所 */
};

/* すごろくルール */
const char *aSugorokuRuleTbl[] = {
	"ゴール(ぴったり)",
	"ゴール(通過)"
};

/* すごろくパーツ */
const char *aSugorokuPartsTbl[] = {
	"何もしない",
	"イベント",
	"スタートに戻る",
	"1マス進む",
	"2マス進む",
	"3マス進む",
	"4マス進む",
	"5マス進む",
	"6マス進む",
	"1マス戻る",
	"2マス戻る",
	"3マス戻る",
	"4マス戻る",
	"5マス戻る",
	"6マス戻る",
	"1回休み"
};

/* すごろくイベント */
const char *aSugorokuEventTbl[] = {
	"何もしない",
	"スタート",
	"ゴール",
	"進む",
	"戻る",
	"スタートに戻る",
	"休み"
};

/* Copyright(C)URARA-works 2003-2005 */
