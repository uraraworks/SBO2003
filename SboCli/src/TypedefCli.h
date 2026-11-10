/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 概要：			クライアント用定数と構造体定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define SCRSIZEX	(32 * (DRAW_PARTS_X * 2 + 1))				/* マップ表示サイズ(横) */
#define SCRSIZEY	(32 * (DRAW_PARTS_Y * 2 + 1))				/* マップ表示サイズ(縦) */
#define SCRMARTSSIZE	((DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y * 2 + 3))		/* 表示マップパーツ数 */
#define SYSMSG_SY	(DRAW_PARTS_Y * 2 * 32)						/* システムメッセージ表示開始位置(縦) */
#define SYSMSG_EY	(DRAW_PARTS_Y * 2 * 32 - 140)				/* システムメッセージ表示終了位置(縦) */
#define MAX_NUMMSGS		(10)
#define ITEMINFO_LINE				(16)						/* アイテム情報ウィンドウの行送り（字は一覧と同じ16ドット。スマホで縮小されても読めるように） */
#define ITEMINFO_CX					(16 * 14)					/* アイテム情報ウィンドウの幅（全角12字の名前が入る） */
#define ITEMINFO_CY					(16 * 2 + ITEMINFO_LINE * 4)	/* アイテム情報ウィンドウの高さ */
#define ITEMINFO_MAGIC_CX			(16 * 22)					/* アイテム情報ウィンドウの幅（特技の書の時） */
#define ITEMINFO_EXPLAIN_MAXLINE	(10)							/* アイテム情報ウィンドウに出す特技説明の最大行数 */
#define ITEMINFOPOS_LIST			(1)							/* アイテム情報ウィンドウ(nType2)：一覧ウィンドウの横に置く */

#define IDC_CONNECT		100										/* 接続ボタン */
#define IDC_MAKE		101										/* 作成ボタン */
#define IDC_CONNECTTOKEN	102										/* 端末トークンで接続（ブラウザ版） */
#define IDC_CONNECTNEW		103										/* 自動で作ったアカウントで接続（ブラウザ版） */

#ifndef ListView_SetCheckState
	#define ListView_SetCheckState(hwndLV, i, fCheck) \
		ListView_SetItemState(hwndLV, i, \
		INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

/* ローカルメッセージ */
enum {
	WM_INITEND = WM_APP,										/* 初期化完了 */
	WM_MSG3SEC,													/* ３秒メッセージ表示終了 */
	WM_RCVDATA,													/* データ受信 */
	WM_CONNECTLOST,												/* サーバーと途切れた */
	WM_ADMINPOSLEFT,											/* 座標を取得 */
	WM_ADMINPOSRIGHT,											/* 座標を取得 */
	WM_ADMINCHGEVENT,											/* イベント情報更新 */
	WM_MAPPARTSCOUNT,											/* マップパーツ数更新 */
	WM_ADMINMSGREQ,												/* マップメッセージ受信 */
	WM_ADMINGETACC,												/* アカウント・パスワード取得 */
	WM_ADMINGETACCINFO,											/* アカウント情報要求 */
	WM_ADMINITEMCHG,											/* アイテム情報更新 */
	WM_ADMINCHARITEM,											/* 所持アイテム情報取得 */
	WM_ADMINSVRINFO,											/* サーバー情報取得 */
	WM_ADMINRBUTTONDBLCLK,										/* 右ダブルクリック */
	WM_ADMINSHOPINFO,											/* 店情報更新 */
	WM_ADMINEFCINFO,											/* エフェクト情報更新 */
	WM_ADMINCHARSTATUS,											/* ステータス情報更新 */
	WM_ADMINSTTMPLATE,											/* ステータステンプレート情報更新 */
	WM_ADMINSUBGAMEINFO,										/* サブゲーム情報更新 */
	WM_ADMINLEAVEINFO,											/* 預かり所情報更新 */
	WM_ADMINACCSEARCH,											/* アカウント検索 */
	WM_ADMINJOBINFO,											/* 職業情報更新 */
	WM_ADMINMAGICINFO,											/* 魔法情報更新 */
	WM_ADMINMAGICUSERINFO,										/* 魔法利用者情報更新 */
	WM_ADMINEQUUSERINFO,										/* 職業装備情報 */
	WM_ADMINSELECTMAPPARTS,										/* マップパーツが選択された */
	WM_ADMINSELECTCHARGRP,										/* キャラ画像が選択された */
	WM_ADMINCLOSEDIALOG,										/* ダイアログが閉じた */
	WM_ADMIN_OK,												/* OKボタンが押された */
	WM_ADMIN_CANCEL,											/* キャンセルボタンが押された */
};

/* ウィンドウ種別 */
enum {
	WNDTYPE_MSG = 1,											/* メッセージ表示 */
	WNDTYPE_MSGVIEW,											/* 常に表示メッセージ表示 */
	WNDTYPE_MSG3SEC,											/* ３秒メッセージ表示 */
	WNDTYPE_LOGIN,												/* ログイン情報 */
	WNDTYPE_LOGINMENU,											/* ログインメニュー */
	WNDTYPE_CONNECTLOST,										/* 接続途切れた */
	WNDTYPE_COMMAND,											/* コマンド表示 */
	WNDTYPE_COMMANDETC,											/* その他コマンド */
	WNDTYPE_COMMANDITEM,										/* 道具 */
	WNDTYPE_COMMANDITEMBATTLE,									/* 道具(戦闘マップ) */
	WNDTYPE_COMMANDITEMPROC,									/* 道具処理選択 */
	WNDTYPE_COMMANDITEMCHAR,									/* 道具使う相手選択 */
	WNDTYPE_COMMANDITEMCHARBATTLE,								/* 道具使う相手選択(戦闘マップ) */
	WNDTYPE_COMMANDOPTION,										/* オプション */
	WNDTYPE_COMMANDOPTIONVOLSOUND,								/* 効果音音量設定 */
	WNDTYPE_COMMANDOPTIONVOLMUSIC,								/* 曲音量設定 */
	WNDTYPE_SPEECH,												/* 発言入力欄 */
	WNDTYPE_LOGINMENUCHARINFO,									/* ログインメニューのキャラ情報 */
	WNDTYPE_LOGINMENUCHARMAKE,									/* ログインメニューのキャラ作成 */
	WNDTYPE_MALEFAMALE,											/* 男女 */
	WNDTYPE_DELETECONFIRM,										/* 削除確認 */
	WNDTYPE_NAMECOLOR,											/* 名前の色設定 */
	WNDTYPE_SHOP,												/* 店 */
	WNDTYPE_SHOPSELL,											/* 店(売るだけ) */
	WNDTYPE_SPEECHEDIT,											/* 自己紹介入力欄 */
	WNDTYPE_COMMANDETC_CONF,									/* 管理人呼び出し確認 */
	WNDTYPE_COMMANDSTATUS,										/* 強さ */
	WNDTYPE_COMMANDEQU,											/* 装備 */
	WNDTYPE_COMMANDEQUSUB,										/* 装備(種類別) */
	WNDTYPE_COMMANDEQUSELECT,									/* 装備(選択) */
	WNDTYPE_LEAVE,												/* 預かり所 */
	WNDTYPE_COMMANDTRADE,										/* 取り引きコマンド */
	WNDTYPE_TRADE,												/* 取り引き */
	WNDTYPE_TRADEITEM,											/* 取り引き(アイテム選択) */
	WNDTYPE_TRADEGOLD,											/* 取り引き(金額設定) */
	WNDTYPE_GOLD,												/* 所持金表示 */
	WNDTYPE_SUBGAME_BOMBINFO,									/* サブゲーム：爆弾ゲーム情報 */
	WNDTYPE_SUBGAME_BOMBSET,									/* サブゲーム：爆弾ゲーム設定 */
	WNDTYPE_CHGPASSWORD,										/* パスワードの変更 */
	WNDTYPE_COMMANDPARTY,										/* 仲間ウィンドウ */
	WNDTYPE_COMMANDPARTYCHAR,									/* 誘う相手選択 */
	WNDTYPE_CONF,												/* 確認 */
	WNDTYPE_JOB,												/* 冒険者登録所 */
	WNDTYPE_COMMANDMAGIC,										/* 魔法 */
	WNDTYPE_COMMANDMAGICBATTLE,									/* 魔法(戦闘マップ) */
	WNDTYPE_SUBWAR_SELECTTEAM,									/* 戦場ゲーム：チーム選択 */
	WNDTYPE_SUBWAR_WAIT,										/* 戦場ゲーム：他のプレイヤー待ち */
	WNDTYPE_SUBWAR_SELECTRULE,									/* 戦場ゲーム：ルール選択 */
	WNDTYPE_SUBWAR_SELECTJOB,									/* 戦場ゲーム：職業選択 */
	WNDTYPE_SUBWAR_SELECTTURN,									/* 戦場ゲーム：行動順序 */
	WNDTYPE_SUBWAR_TURN,										/* 戦場ゲーム：行動選択 */
	WNDTYPE_SUBWAR_TURN_MOVE,									/* 戦場ゲーム：行動(移動) */
	WNDTYPE_SUBWAR_TURN_ATACK,									/* 戦場ゲーム：行動(攻撃) */
	WNDTYPE_SUBWAR_TURN_DIFENCE_FIGHTER,						/* 戦場ゲーム：行動(防御：戦士) */
	WNDTYPE_SUBWAR_TURN_DIFENCE_MARTIAL,						/* 戦場ゲーム：行動(防御：武道家) */
	WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH,							/* 戦場ゲーム：行動(防御：魔法使い) */
	WNDTYPE_SUBWAR_TURN_DIFENCE_PRIEST,							/* 戦場ゲーム：行動(防御：僧侶) */
	WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH2,							/* 戦場ゲーム：行動(魔法防御：魔法使い) */
	WNDTYPE_SUBWAR_TURN_MAGIC_WITCH,							/* 戦場ゲーム：行動(魔法：魔法使い) */
	WNDTYPE_SUBWAR_TURN_MAGIC_TARGET,							/* 戦場ゲーム：行動(魔法対象者選択) */
	WNDTYPE_SUBWAR_TURN_MAGIC_PRIEST,							/* 戦場ゲーム：行動(魔法：僧侶) */
	WNDTYPE_ITEMEQU,											/* 装備アイテム */
	WNDTYPE_STATUS_ATTR,										/* ステータス表示(属性) */
	WNDTYPE_BATTLE_COMMAND,										/* 戦闘：コマンド */
	WNDTYPE_BATTLE_TARGET,										/* 戦闘：ターゲット選択 */
	WNDTYPE_SUBSUGOROKU_JOINMODE,								/* すごろく：参加モード選択 */
	WNDTYPE_SUBSUGOROKU_COMMAND,								/* すごろく：行動選択 */
	WNDTYPE_SUBSUGOROKU_IDLE,									/* すごろく：平常状態 */
	WNDTYPE_ITEMINFO,											/* アイテム情報 */
	WNDTYPE_DEAD_COMMAND,										/* 死亡中：コマンド */
	WNDTYPE_STATUSMAGIC,										/* 強さ：覚えている特技 */
	WNDTYPE_MAKER,												/* 作り手（レシピ）：作れる物の一覧・確認 */
	WNDTYPE_TOWNWARP,											/* 町転送：行き先の選択 */
};

/* 作り手ウィンドウ(WNDTYPE_MAKER)の nType2 */
enum {
	MAKERWND_LIST = 0,											/* 作れる物の一覧 */
	MAKERWND_CONF,												/* 「作ってもらいますか？」のはい/いいえ */
};
#define MAKER_LISTCX				(16 * 12)									/* 作れる物の一覧の幅 */
#define MAKER_LISTROWS				(10)										/* 一覧に一度に出す行数 */
#define MAKERINFO_CX				(16 * 18)									/* 作り手の画面のアイテム情報ウィンドウの最小の幅（材料の行が入る広さ） */
#define MAKER_WAITMAX				(5000)										/* 作成要求の返事を待つ最長時間(ms) */

/* 強さ：覚えている特技ウィンドウ(WNDTYPE_STATUSMAGIC)の大きさ */
#define STATUSMAGIC_EXPLAIN_LINE	(4)											/* 説明欄の行数（16ドットの字） */
#define STATUSMAGIC_EXPLAIN_CY		(16 * 2 + 16 * STATUSMAGIC_EXPLAIN_LINE)	/* 説明欄の高さ */
#define STATUSMAGIC_LIST_CY(n)		(16 * (2 + max (1, (n))))					/* 一覧の高さ（特技が無くても1行分） */

/* 町転送の一覧ウィンドウ(WNDTYPE_TOWNWARP)の nType2 */
enum {
	TOWNWARPWND_USE = 0,										/* 町転送を使う時の行き先選び */
	TOWNWARPWND_FORGET,											/* 教会で記録する時、いっぱいなので忘れる町を選ぶ（dwData に記録する位置） */
};

/* はい/いいえ種別 */
enum {
	IDCONFIRMTYPE_DROP = 1,										/* 捨てる */
	IDCONFIRMTYPE_GRPCHANGE,									/* 着せ替え確認 */
	IDCONFIRMTYPE_PARTYJOIN,									/* 仲間に入る確認 */
	IDCONFIRMTYPE_BREAKUP,										/* パーティ解散 */
	IDCONFIRMTYPE_BREAKSUGOROKU,								/* すごろくを抜ける */
	IDCONFIRMTYPE_STARTPOS,										/* 復活位置を記録する確認 */
	IDCONFIRMTYPE_MERCOUT,										/* 傭兵と別れる */
};

/* メッセージウィンドウ(WNDTYPE_MSG)の nData2 の状態 */
#define MSGSTATE_STARTPOSCONF	(3)								/* 復活位置の「はい・いいえ」待ち */

/* ３秒メッセージ終了後イベントID */
enum {
	MSG3SEC_ACC = 1,											/* アカウント名未入力 */
};

/* キーイベント */
enum {
	KEY_DOWN = 1,												/* キー押下 */
	KEY_UP,														/* キー押下解除 */
};

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ウィンドウ情報 */
typedef struct _WNDINFO {
	DWORD			dwID;										/* ウィンドウID */
	int				nX;											/* 表示座標(横) */
	int				nY;											/* 表示座標(縦) */
	int				nCx;										/* 横幅 */
	int				nCy;										/* 縦幅 */
	int				nType;										/* 種別 */
	int				nType2;										/* 種別2 */
	int				nData1;										/* データ1 */
	int				nData2;										/* データ2 */
	BYTE			byAnime;									/* アニメーション番号 */
	BYTE			byRepeate;									/* キーリピート判定 */
	BYTE			byData1;									/* データ１ */
	char			*pszMsg;									/* 表示文字列 */
	void			*pParent;									/* 親ウィンドウ */
	void			*pChild;									/* 子ウィンドウ */
	void			*pWndTmp;									/* 汎用ウィンドウ用 */
	CImg32			*pDib;										/* ビットマップ */
	DWORD			dwTime;										/* 最後に処理した時間 */
	DWORD			dwTimeDrawStart;							/* 最初に描画した時間 */
	DWORD			dwTimeKey;									/* 最後にキー処理した時間 */
	DWORD			dwData;										/* 汎用データ（作り手の画面の再描画判定に使う） */
	int				nMsgDrawLen;								/* メッセージ：描いてある所まで（pszMsg のバイト数、0 なら描き直し） */
	int				nMsgDrawScroll;								/* メッセージ：描いた時にずれていた行数 */
	DWORD			dwMsgDrawHash;								/* メッセージ：描いてある所までの中身の目印 */
} WNDINFO, *PWNDINFO;

/* システムメッセージ情報 */
typedef struct _SYSMSGINFO {
	int				nY;											/* 表示座標(縦) */
	COLORREF		cl;											/* 表示色 */
	LPSTR			pszMsg;										/* 表示文字列 */
	DWORD			dwTime;										/* 最後に処理した時間 */
} SYSMSGINFO, *PSYSMSGINFO;

/* 管理人ルーラ */
typedef struct _ADMINRURAINFO {
	DWORD			dwMapID;									/* マップID */
	DWORD			dwX;										/* X座標 */
	DWORD			dwY;										/* Y座標 */
	char			szComment[64];								/* コメント（UTF-8・全角20字ほど入る） */
} ADMINRURAINFO, *PADMINRURAINFO;

/* 管理人ルーラ（コメント欄が 21 バイトだった頃の保存形式。SboCliPosInfo.dat の読み込み用） */
typedef struct _ADMINRURAINFO_OLD {
	DWORD			dwMapID;									/* マップID */
	DWORD			dwX;										/* X座標 */
	DWORD			dwY;										/* Y座標 */
	char			szComment[21];								/* コメント */
} ADMINRURAINFO_OLD, *PADMINRURAINFO_OLD;

/* フェードイン・フェードアウト情報 */
typedef struct _FADEINOUTINFO {
	BYTE			byFadeIn;									/* フェードインレベル */
	BYTE			byFadeOut;									/* フェードアウトレベル */
	DWORD			dwLastTime;									/* 最後に処理した時間 */
} FADEINOUTINFO, *PFADEINOUTINFO;

/* 誰に使うか一覧情報 */
typedef struct _USECHARINFO {
	char		szName[MAX_CHARNAMEBUF];						/* キャラ名 */
	DWORD		dwTblID;										/* テーブルID */
} USECHARINFO, *PUSECHARINFO;
typedef CmyArray<PUSECHARINFO, PUSECHARINFO>	  ARRAYUSECHARINFO;
typedef CmyArray<PUSECHARINFO, PUSECHARINFO>	*PARRAYUSECHARINFO;

/* 取り引き情報 */
typedef struct _TRADEINFO {
	BYTE		byMode1;										/* 決定状態(自分) */
	BYTE		byMode2;										/* 決定状態(相手) */
	BYTE		bySum1;											/* チェックサム1 */
	BYTE		bySum2;											/* チェックサム2 */
	WORD		waItem1[MAX_MYITEM];							/* アイテム(自分) */
	WORD		waItem2[MAX_MYITEM];							/* アイテム(相手) */
	DWORD		dwGold1;										/* 金額(自分) */
	DWORD		dwGold2;										/* 金額(相手) */
	DWORD		dwTblID;										/* テーブルID(相手) */
	DWORD		pID;											/* キャラID(相手) */
} TRADEINFO, *PTRADEINFO;

/* ターゲット選択情報 */
typedef struct _TARGETSELECTINFO {
	DWORD		dwTblID;										/* キャラID */
	DWORD		dwLastTime;										/* 最後に処理した時間 */
	BYTE		byAnime;										/* アニメーション番号 */
} TARGETSELECTINFO, *PTARGETSELECTINFO;

/* Copyright(C)URARA-works 2003-2005 */
