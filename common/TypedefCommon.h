/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* 概要：		定数と構造体定義											 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* マクロ定義																 */
/* ========================================================================= */
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define SAFE_DESTROYWND(p)   { if(p) { DestroyWindow(p); (p)=NULL; } }

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define WNDTITLE			"スクラップブックオンライン "			/* ウィンドウタイトル */
#define VERTEXT				"Ver0.116(感謝祭)"						/* バージョン */
#define VERSIONVAL			44										/* バージョン番号（44: 文字列を UTF-8 にした） */
#define DLLVER				"34"									/* DLLバージョン */
#define GRPDATADLLVER		"5"										/* グラフィックデータDLLバージョン */
#define MAX_ACCNAME			14										/* 最大アカウント名文字数 */
#define TURNGAGE_LAP		20										/* ターンゲージ1周の%（1周溜まると早撃ちでき、威力は周の数×この%） */
#define TURNGAGE_LAPMAX		(100 / TURNGAGE_LAP)					/* ターンゲージが満タンになる周の数 */
#define TURNGAGE_LATETIME	300										/* 殴った直後、X でコマンドを開かない時間(ms)。ジャストに押し遅れた時のため(1周の半分まで) */
#define CHAREFC_BIG			(0x01000000)							/* 文字エフェクトの色に足すと大きい文字で出す(ジャスト・構えなど) */
#define COMEFC_TELL			4										/* 敵の攻撃の予兆に出す吹き出し(白い丸に「!」) */
#define TELL_VIEWTIME		1000									/* 予兆の「!」を出しておく時間(ms)。敵が殴るまで */
#define AUTOGAGE_LOSTTIME	500										/* 目の前から敵がいなくなってから自動ゲージを消すまでの時間(ms) */
#define TURNGAGE_JUSTTIME	300										/* ゲージ満タン後、ジャストで押せる時間(ms)。押さなければ自動で殴る */
#define GUARD_HOLDTIME		300										/* Z をこれだけ押し続けると構える(ms)。短く押すと今までどおり攻撃をやめる */
#define ENEMYTELL_TIME		1000									/* 敵が攻撃する何ms前に予兆「！」を出すか */
#define ATACKRATE_AREA		50										/* なぎ払い・貫通で、狙った相手以外に当たる時の威力(%) */
#define ATACKRATE_JUST		101										/* 攻撃要求の威力欄でジャストを表す値（威力は100%として扱う） */
#define MAX_CHARNAME		15										/* 最大キャラ名文字数（表示幅の上限は MAX_CHARNAME_WIDTH） */
#define MAX_CHARNAMEBUF		64										/* キャラ名の欄のバイト数（終端含む。UTF-8 や絵文字でも入るように広めにとる） */
#define MAX_ITEMNAME		64										/* アイテム名の欄のバイト数（終端含む） */
#define MAX_JOBNAME			64										/* 職業名の欄のバイト数（終端含む） */
#define MAX_EQUNAME			64										/* 装備情報名の欄のバイト数（終端含む） */
#define MAX_MAGICNAME		64										/* 魔法名の欄のバイト数（終端含む） */
#define MAX_MAGICEXPLAIN	512										/* 魔法説明文の欄のバイト数（終端含む） */
#define MAX_COMEFC			99										/* 最大吹き出し種類 */
#define MAX_MYITEM			10										/* 最大所持アイテム */
#define MAX_SHOPITEM		10										/* 最大売り物数 */
#define MAX_SHOPMSG			12										/* メッセージ数 */
#define MAX_LENCHAREFC		32										/* 文字エフェクトの欄のバイト数（終端含む） */
#define MAX_COUNTCHAREFC	5										/* 文字エフェクト数 */
#define MAX_LEAVEITEM		100										/* 預けられるアイテム数 */
#define MAX_LEAVEGOLD		10000000								/* 預けられるお金 */
#define MAX_SUBGAME			255										/* 最大サブゲーム数 */
#define MAX_SUBSTATUS		6										/* 保存しないサブステータス情報サイズ */
#define MAX_EQUTYPE			10										/* 装備可能アイテム数 */
#define MAX_PARTYMEMBER		6										/* 最大パーティ数 */
#define MAX_USEMAGIC		16										/* 覚えられる魔法数 */
#define MAX_NPCROUTE		16										/* NPCの巡回ルートの道しるべの最大数 */
#define MAX_JOBMAGIC		32										/* 職業別に呪文を覚えておける職業数（職業IDがこれ未満の職業が対象） */
#define MAX_SUBGAME_WARMEMBER	4									/* 戦場ゲーム最大参加人数 */

/* 入力できる長さ（表示幅。半角1文字=1、全角1文字=2）
 * 欄のバイト数は UTF-8 化（1文字が最大4バイト）に備えて広げてあるので、
 * 画面に収まる長さはこちらで決める */
#define MAX_CHARNAME_WIDTH		(MAX_CHARNAME - 1)						/* キャラ名（全角7文字） */
#define MAX_ITEMNAME_WIDTH		30										/* アイテム名 */
#define MAX_JOBNAME_WIDTH		30										/* 職業名 */
#define MAX_EQUNAME_WIDTH		30										/* 装備情報名 */
#define MAX_MAGICNAME_WIDTH		30										/* 魔法名 */
#define MAX_MAGICEXPLAIN_WIDTH	255										/* 魔法説明文 */
#define MAX_LENCHAREFC_WIDTH	14										/* 文字エフェクト */

/* 古い保存データの欄のバイト数（読み込み時の互換用） */
#define OLD_MAX_ITEMNAME		31										/* アイテム名（szName 列） */
#define OLD_MAX_LENCHAREFC		15										/* 文字エフェクト（szCharEfc 列） */

#define SENDHARTBEATTIME	10000									/* 接続チェック送信周期 */
#define DRAW_PARTS_X	7											/* マップ表示数(横) */
#define DRAW_PARTS_Y	7											/* マップ表示数(縦) */

#define MAPFLG_EVENT	(0x01)										/* マップフラグ(イベント) */
#define MAPFLG_ITEM		(0x02)										/* マップフラグ(アイテム) */

#define FLGCHARPOS_NOANIME	(0x01)									/* 移動アニメーション無し */

/* 処理中のフラグ */
#define CHARINFOFLG_MOVE		(0x01)								/* キャラフラグ：移動した */
#define CHARINFOFLG_DIRECTION	(0x02)								/* キャラフラグ：向き変更 */
#define CHARINFOFLG_SPEECH		(0x04)								/* キャラフラグ：発言した */
#define CHARINFOFLG_COMEFC		(0x08)								/* キャラフラグ：吹き出しが出る */
#define CHARINFOFLG_MOVEPARENT	(0x10)								/* キャラフラグ：前のキャラが動いた */
#define CHARINFOFLG_BTLCMD		(0x20)								/* キャラフラグ：戦闘コマンド表示中 */

/* 保存する用のフラグ */
#define CHARINFOSAVEFLG_FEMALE	(0x01)								/* キャラフラグ：性別:女 */

/* アイテム用のフラグ */
#define ITEMFLG_DROP			(0x01)								/* アイテムフラグ：捨てられない */
#define ITEMFLG_LOST			(0x02)								/* アイテムフラグ：使うとなくなる */
#define ITEMFLG_TRADE			(0x04)								/* アイテムフラグ：取り引き不可 */

/* マップ用のフラグ */
#define MAPFLG_EFECT			(0x01)								/* エフェクトを許可 */
#define MAPFLG_BATTLE			(0x02)								/* 戦闘モードを許可 */
#define MAPFLG_SUBGAME			(0x04)								/* サブゲームマップ */
#define MAPFLG_DARK				(0x08)								/* 暗闇 */
#define MAPFLG_SPEECH			(0x10)								/* マップ内放送 */
#define MAPFLG_MOVE				(0x20)								/* 移動系効果禁止 */
#define MAPFLG_MAGIC			(0x40)								/* 魔法禁止 */

/* アカウント状態 */
#define ACCSTATE_NORMAL			(0x00)								/* 通常状態 */
#define ACCSTATE_DISABLE		(0x01)								/* ログイン禁止中 */
#define ACCSTATE_ADMINON		(0x02)								/* 管理人業務中 */

/* すごろく */
#define SUBSUGOROKU_WAITTIME_JOIN	(60)							/* 待ち時間(参加受付) */
/* コマンド */
enum {
	CMD_CONNECT = 1,												/* 接続応答 */
	CMD_LOGINREQ,													/* ログイン要求 */
	CMD_LOGINRES,													/* ログイン応答 */
	CMD_CHGSTATE,													/* 状態遷移 */
	CMD_CHARINFO2,													/* キャラ情報通知2 */
	CMD_MAPINFO,													/* マップ情報通知 */
	CMD_MAPPARTSINFO,												/* マップパーツ情報通知 */
	CMD_CHGDIRECTION,												/* 向き変更要求 */
	CMD_CHARPOS,													/* キャラ座標通知 */
	CMD_CHARINFOREQ,				/* 10 */						/* キャラ情報要求 */
	CMD_SPEECH,														/* 発言通知 */
	CMD_ONLINE,														/* オンライン数通知 */
	CMD_HARTBEAT,													/* 生存確認 */
	CMD_COUNTEVENT,													/* イベント数通知 */
	CMD_EVENTPOS,													/* イベント座標通知 */
	CMD_CHECKEVENT,													/* イベントチェック要求 */
	CMD_CHGBGM,														/* BGM変更 */
	CMD_VIEWMSG,													/* メッセージ表示 */
	CMD_PLAYSOUND,													/* 効果音再生 */
	CMD_MAPMSG,						/* 20 */						/* メッセージ通知 */
	CMD_OPENDOOR,													/* 扉開け要求 */
	CMD_CHGCHARGRP,													/* 着せ替え要求 */
	CMD_CHGNAMESEX,													/* 名前と性別更新 */
	CMD_SYSMSG,														/* システムメッセージ */
	CMD_GMCALL,														/* 管理人呼び出し */
	CMD_CHARPUSH,													/* 押して要求 */
	CMD_COMEFC,														/* 吹き出し通知 */
	CMD_ERR,														/* エラー通知 */
	CMD_ADDSCORE,													/* 得点追加要求 */
	CMD_TALKRAND,					/* 30 */						/* 話してランダム要求 */
	CMD_STOPRAND,													/* 止まるかランダム切り替え */
	CMD_CEFINFO,													/* 吹き出し情報通知 */
	CMD_FADEINOUT,													/* フェードイン・アウト */
	CMD_ITEMINFO,													/* アイテム情報通知 */
	CMD_GETITEM,													/* アイテム取得 */
	CMD_CHARITEM,													/* 所持アイテム情報 */
	CMD_DROPITEM,													/* アイテム捨てる */
	CMD_USEITEM,													/* アイテム使う */
	CMD_MAPCHARINFO,												/* マップ内キャラ数通知 */
	CMD_NAMECOLOR,					/* 40 */						/* 名前の色設定 */
	CMD_SHOP,														/* 店情報 */
	CMD_EFC,														/* エフェクト情報更新 */
	CMD_EFCINFO,													/* エフェクト情報通知 */
	CMD_EFCREQ,														/* エフェクト発動 */
	CMD_TARGETCHAR,													/* ターゲット要求 */
	CMD_CHARSTATUSHP,												/* キャラステータス(HPのみ)通知 */
	CMD_CHARSTATUSGOLD,												/* キャラステータス(GOLDのみ)通知 */
	CMD_TARGETCHARPC,												/* ついていく要求 */
	CMD_ADDCHAREFC,													/* 文字エフェクト追加 */	
	CMD_CHGEQU,						/* 50 */						/* 装備変更 */
	CMD_CHGMAPCOLOR,												/* マップ色変換 */
	CMD_LEAVEITEM,													/* アイテム預ける */
	CMD_LEAVEITEMGET,												/* 預けてるアイテム取得 */
	CMD_COUNTSUBGAME,												/* サブゲーム数通知 */
	CMD_SUBSTATUS,													/* サブステータス情報 */
	CMD_SUBGAME,													/* サブゲーム用コマンド */
	CMD_TRADE,														/* 取り引きコマンド */
	CMD_TRADEINFO,													/* 取り引き情報 */
	CMD_TRADEPROC,													/* 取り引き決定 */
	CMD_SYSMSG2,					/* 60 */						/* ID指定システムメッセージ */
	CMD_LEAVEGOLD,													/* お金預ける */
	CMD_LEAVEGOLDGET,												/* 預けてるお金取得 */
	CMD_SETSTARTPOS,												/* 復活座標設定 */
	CMD_CHGPASSWORD,												/* パスワード変更 */
	CMD_CHGCHARGRPBACK,												/* キャラ画像記録 */
	CMD_MSGCMD,														/* メッセージコマンド */
	CMD_PARTY,														/* 仲間コマンド */
	CMD_CHARSTATUSHPMANA,											/* キャラステータス(HPとMANA)通知 */
	CMD_JOBCHG,														/* 職業変更 */
	CMD_USEMAGIC,					/* 70 */						/* 魔法使用 */
	CMD_TARGETCHG,													/* ターゲット変更要求 */
	CMD_SHOPBUY,													/* アイテム購入 */
	CMD_DIFENCE,													/* 防御 */
	CMD_THROWSNOW,													/* 雪を投げる */
	CMD_EFECTCHG,													/* エフェクト変更 */
	CMD_RESURRECTION,												/* 蘇生 */
	CMD_REQ_ATTACK,													/* 攻撃要求 */
	CMD_MAIN_CHAR,													/* キャラ系コマンド */
/* 以下、管理者用コマンド CMD_MAPPARTSADD より上には置かない事。判別に使ってるので。 */
	CMD_MAPPARTSADD,												/* マップパーツ追加 */
	CMD_CHARSTATUS,													/* キャラステータス通知 */
	CMD_MAPPARTS,													/* マップパーツ更新 */
	CMD_MAPPARTSRELOAD,												/* マップパーツ更新要求 */
	CMD_MAPPARTSADDANIME,											/* マップパーツアニメ追加 */
	CMD_ADDMAP,														/* マップ追加 */
	CMD_MAPMSGREQ,													/* メッセージ要求 */
	CMD_CHGMSG,														/* メッセージ更新 */
	CMD_CHARINFO,													/* キャラ情報通知 */
	CMD_CHGMAP,						/* 10 */						/* マップ書き換え */
	CMD_ADDDELMAP,													/* マップ幅増減 */
	CMD_SAVEMAP,													/* マップ保存 */
	CMD_CHGMAPINFO,													/* マップ情報更新 */
	CMD_EVENTADD,													/* イベント情報追加 */
	CMD_EVENTGET,													/* イベント情報取得 */
	CMD_CHGEVENT,													/* イベント情報更新 */
	CMD_ADDCHAR,													/* キャラ追加 */
	CMD_BANCHAR,													/* キャラ強制ログオフ */
	CMD_ADDMSG,														/* メッセージ追加 */
	CMD_MSG,						/* 20 */						/* メッセージ更新 */
	CMD_MAPMOVE,													/* マップ移動 */
	CMD_DELCHAR,													/* キャラ削除 */
	CMD_ACCADD,														/* アカウント追加 */
	CMD_GETACC,														/* アカウント名・パスワード取得 */
	CMD_CHGACC,														/* アカウント名・パスワード更新 */
	CMD_GETACCINFO,													/* アカウント情報要求 */
	CMD_CHGACCSTATE,												/* アカウント状態更新 */
	CMD_CLIVERSION,													/* クライアントバージョン通知 */
	CMD_CHGCEF,														/* 吹き出し情報更新 */
	CMD_CHGADMINON,					/* 30 */						/* 管理人ON/OFF通知 */
	CMD_ADDITEM,													/* アイテム追加 */
	CMD_CHGITEM,													/* アイテム情報更新 */
	CMD_REQCHARITEM,												/* 所持アイテム情報要求 */
	CMD_PLAYSOUNDALL,												/* 効果音放送 */
	CMD_SVRINFO,													/* サーバー情報 */
	CMD_ADDSHOP,													/* 店追加要求 */
	CMD_CHGSHOP,													/* 店情報更新 */
	CMD_ADDEFC,														/* エフェクト追加 */
	CMD_CHGEFC,														/* エフェクト更新 */
	CMD_REQCHARSTATUS,				/* 40 */						/* ステータス情報要求 */
	CMD_STTMPLATE,													/* ステータステンプレート情報 */
	CMD_SUBGAMEGET,													/* サブゲーム情報取得 */
	CMD_SUBGAMEADD,													/* サブゲーム情報追加 */
	CMD_SUBGAMEINFO,												/* サブゲーム情報通知 */
	CMD_SUBGAMEINFOALL,												/* 全サブゲーム情報通知 */
	CMD_LEAVEINFOGET,												/* 預かり所情報取得 */
	CMD_LEAVEINFO,													/* 預かり所情報 */
	CMD_ACC_SEARCH,													/* アカウント検索 */
	CMD_ADMINWINDOW,												/* 管理者ウィンドウ表示 */
	CMD_JOBINFO,													/* 職業情報 */
	CMD_JOBINFOREQ,					/* 50 */						/* 職業情報要求 */
	CMD_MAGICINFO,													/* 魔法情報 */
	CMD_MAGICINFOREQ,												/* 魔法情報要求 */
	CMD_MAGICUSERINFO,												/* 魔法利用者情報 */
	CMD_MAGICUSERINFOREQ,											/* 魔法利用者情報要求 */
	CMD_EQUUSERINFO,												/* 装備職業情報 */
	CMD_EQUUSERINFOREQ,												/* 装備職業情報要求 */
/* 以下、ブラウザ版のアカウント関係（管理者用ではないが、番号をずらさないよう最後に足す） */
	CMD_WEBACCOUNT,													/* ブラウザ版のアカウント関係 */
/* 以下、管理者用（CMD_WEBACCOUNT より後ろに足す） */
	CMD_ADMINUNDO,													/* 管理者画面の「元に戻す」 */
/* 以下、プレイヤー用（番号をずらさないよう最後に足す。管理者用の判定から外すこと） */
	CMD_REQ_GUARD,													/* 構え(防御)の開始・終了 */
	CMD_HASTE,														/* 加速の状態通知(サーバー→本人) */
	CMD_MAX
};

/* キャラ系コマンド */
enum {
	CMDCHAR_NONE = 0,
	CMDCHAR_STATEINFO,												/* 状態通知 */
	CMDCHAR_TAIL,													/* ついていく要求 */
	CMDCHAR_MAX
};

/* サブゲーム[爆弾ゲーム]用コマンド */
enum {
	CMDSUBBOMB_SETBOMB = 1,											/* 爆弾設置 */
	CMDSUBBOMB_TIME,												/* 残り時間通知 */
	CMDSUBBOMB_INFO,												/* 設定 */
	CMDSUBBOMB_START,												/* 試合開始 */
	CMDSUBBOMB_STOP,												/* 試合中止 */
	CNDSUBBOMB_GETITEM,												/* アイテム取得 */
	CNDSUBBOMB_REMOCON,												/* リモコン使用 */
};

/* サブゲーム[迷路]用コマンド */
enum {
	CMDSUBBOMB_MAZERENEW = 1,										/* 迷路作成 */
};

/* サブゲーム[ダンジョン]用コマンド */
enum {
	CMDSUBBOMB_DUNGEONRENEW = 1,									/* ダンジョン作成 */
};

/* サブゲーム[戦場ゲーム]用コマンド */
enum {
	CMDSUBWAR_CHGSTATE = 1,										/* 状態遷移 */
	CMDSUBWAR_SELECTTEAM,										/* チーム選択 */
	CMDSUBWAR_QUIT,												/* ゲームから抜ける */
	CMDSUBWAR_SELECTRULE,										/* ルール選択 */
	CMDSUBWAR_SELECTJOB,										/* 職業選択 */
	CMDSUBWAR_SELECTTURN,										/* 順番選択 */
	CMDSUBWAR_WAIT,												/* 待機 */
	CMDSUBWAR_ATACK,											/* 攻撃 */
	CMDSUBWAR_DIFENCE,											/* 防御 */
	CMDSUBWAR_MAGIC,											/* 魔法 */
	CMDSUBWAR_MAGIC_TARGET,										/* 対象者一覧 */
};

/* サブゲーム[すごろく]用コマンド */
enum {
	CMDSUBSUGOROKU_CHGSTATE = 1,								/* 状態遷移 */
	CMDSUBSUGOROKU_JOINMODE,									/* 参加モード */
	CMDSUBSUGOROKU_QUIT,										/* ゲームから抜ける */
	CMDSUBSUGOROKU_DICE,										/* サイコロ振る */
	CMDSUBSUGOROKU_MSG,											/* メッセージ */
};

/* 戦場ゲーム：参加人数 */
enum {
	SUBGAME_WAR_MEMBER_3 = 0,									/* ３人 */
	SUBGAME_WAR_MEMBER_4,										/* ４人 */
};

/* 戦場ゲーム：ゲーム状態 */
enum {
	SUBGAME_WAR_STATE_INIT = 0,									/* 初期状態 */
	SUBGAME_WAR_STATE_CLEAN,									/* 初期化 */
	SUBGAME_WAR_STATE_READY,									/* 準備完了 */
	SUBGAME_WAR_STATE_JOIN,										/* 参加者募集中 */
	SUBGAME_WAR_STATE_SELECTTEAM,								/* チーム選択 */
	SUBGAME_WAR_STATE_SELECTRULE,								/* ルール選択 */
	SUBGAME_WAR_STATE_SELECTJOB,								/* 職業選択 */
	SUBGAME_WAR_STATE_SELECTTURN,								/* 順番設定 */
	SUBGAME_WAR_STATE_GAMEPREPARE,								/* 試合準備 */
	SUBGAME_WAR_STATE_GAME,										/* 試合中 */
	SUBGAME_WAR_STATE_WAIT,										/* 他のプレイヤー待ち */
	SUBGAME_WAR_STATE_TURN,										/* 行動選択 */
	SUBGAME_WAR_STATE_TURNCONTINUE,								/* 行動選択続行 */
	SUBGAME_WAR_STATE_SELECTDIFENCE,							/* 防御行動選択 */
	SUBGAME_WAR_STATE_WAITMAGICINFO,							/* 魔法対象者一覧待ち */
	SUBGAME_WAR_STATE_SELECTMAGICTARGET,						/* 魔法対象者選択 */
	SUBGAME_WAR_STATE_QUIT,										/* 終了 */
};

/* 戦場ゲーム：職業 */
enum {
	SUBGAME_WAR_JOB_FIGHTER = 0,								/* 戦士 */
	SUBGAME_WAR_JOB_MARTIAL,									/* 武道家 */
	SUBGAME_WAR_JOB_WITCH,										/* 魔法使い */
	SUBGAME_WAR_JOB_PRIEST,										/* 僧侶 */
	SUBGAME_WAR_JOB_MAX
};

/* 戦場ゲーム：防御行動 */
enum {
	SUBGAME_WAR_DIFENCE_NORMAL = 0,								/* 通常防御 */
	SUBGAME_WAR_DIFENCE_AVOID,									/* 見切り */
	SUBGAME_WAR_DIFENCE_PASSIVE,								/* 受け身 */
	SUBGAME_WAR_DIFENCE_COUNTER,								/* カウンター */
	SUBGAME_WAR_DIFENCE_MAGICCOUNTER,							/* 魔法跳ね返し */
};

/* 戦場ゲーム：魔法 */
enum {
	SUBGAME_WAR_MAGIC_NONE = 0,
	SUBGAME_WAR_MAGIC_FIRE,										/* 炎 */
	SUBGAME_WAR_MAGIC_QURE,										/* 回復 */
	SUBGAME_WAR_MAGIC_RESUSCITATION,							/* 蘇生 */
};

/* すごろく：ルール */
enum {
	SUBGAME_SUGOROKU_RULE_GOAL_STOP = 0,						/* ゴール(ぴったり) */
	SUBGAME_SUGOROKU_RULE_GOAL_NONSTOP,							/* ゴール(通過) */
	SUBGAME_SUGOROKU_RULE_MAX
};

/* すごろく：イベント */
enum {
	SUBGAME_SUGOROKU_EVENT_NONE = 0,
	SUBGAME_SUGOROKU_EVENT_START,								/* スタート */
	SUBGAME_SUGOROKU_EVENT_GOAL,								/* ゴール */
	SUBGAME_SUGOROKU_EVENT_GO,									/* 進む */
	SUBGAME_SUGOROKU_EVENT_BACK,								/* 戻る */
	SUBGAME_SUGOROKU_EVENT_BACKSTART,							/* スタートに戻る */
	SUBGAME_SUGOROKU_EVENT_WAIT,								/* 休み */
	SUBGAME_SUGOROKU_EVENT_MAX
};

/* すごろく：パーツ */
enum {
	SUBGAME_SUGOROKU_PARTS_NONE = 0,							/* 何もなし */
	SUBGAME_SUGOROKU_PARTS_EVENT,								/* イベント */
	SUBGAME_SUGOROKU_PARTS_BACKSTART,							/* スタートに戻る */
	SUBGAME_SUGOROKU_PARTS_GO_1,								/* 1マス進む */
	SUBGAME_SUGOROKU_PARTS_GO_2,								/* 2マス進む */
	SUBGAME_SUGOROKU_PARTS_GO_3,								/* 3マス進む */
	SUBGAME_SUGOROKU_PARTS_GO_4,								/* 4マス進む */
	SUBGAME_SUGOROKU_PARTS_GO_5,								/* 5マス進む */
	SUBGAME_SUGOROKU_PARTS_GO_6,								/* 6マス進む */
	SUBGAME_SUGOROKU_PARTS_BACK_1,								/* 1マス戻る */
	SUBGAME_SUGOROKU_PARTS_BACK_2,								/* 2マス戻る */
	SUBGAME_SUGOROKU_PARTS_BACK_3,								/* 3マス戻る */
	SUBGAME_SUGOROKU_PARTS_BACK_4,								/* 4マス戻る */
	SUBGAME_SUGOROKU_PARTS_BACK_5,								/* 5マス戻る */
	SUBGAME_SUGOROKU_PARTS_BACK_6,								/* 6マス戻る */
	SUBGAME_SUGOROKU_PARTS_WAIT,								/* 1回休み */
	SUBGAME_SUGOROKU_PARTS_MAX
};

/* すごろく：ゲーム状態 */
enum {
	SUBGAME_SUGOROKU_STATE_INIT = 0,							/* 初期状態 */
	SUBGAME_SUGOROKU_STATE_CLEAN,								/* 初期化 */
	SUBGAME_SUGOROKU_STATE_READY,								/* 準備完了 */
	SUBGAME_SUGOROKU_STATE_JOIN,								/* 参加者募集中 */
	SUBGAME_SUGOROKU_STATE_JOINMODE,							/* 参加モード選択 */
	SUBGAME_SUGOROKU_STATE_COMMAND,								/* 行動選択 */
	SUBGAME_SUGOROKU_STATE_IDLE,								/* 平常状態 */
	SUBGAME_SUGOROKU_STATE_MOVE,								/* 移動中 */
	SUBGAME_SUGOROKU_STATE_WAIT,								/* 待ち状態 */
	SUBGAME_SUGOROKU_STATE_GOAL,								/* ゴール後の後処理状態 */
};

/* 取り引きコマンド */
enum {
	CMDTRADE_IN = 1,												/* 取り引き開始 */
	CMDTRADE_OUT,													/* 取り引き終了 */
};

/* 仲間コマンド */
enum {
	CMDPARTY_JOIN = 1,												/* 誘う */
	CMDPARTY_JOINOK,												/* 参加する */
	CMDPARTY_JOINNO,												/* 参加しない */
	CMDPARTY_BREAKUP,												/* 解散 */
	CMDPARTY_BREAK,													/* 抜ける */
	CMDPARTY_JOINCANCEL,											/* 応答拒否 */
	CMDPARTY_NOTIFY,												/* 通知 */
	CMDPARTY_GOLD,													/* 分配方法 */
};

/* お金分配方法 */
enum {
	PARTYGOLD_TARGET = 0,											/* 止めをさしたプレイヤー */
	PARTYGOLD_ALL,													/* 全員で分配 */
	PARTYGOLD_MAX,
};

/* エラーコード */
enum {
	SBOERR_NONE = 0,
	SBOERR_NAME,													/* 名前がすでに登録されている */
};

/* ステータステンプレート情報コマンド種別 */
enum {
	CMDSTTMPLATETYPE_ADD = 0,										/* 追加 */
	CMDSTTMPLATETYPE_CHG,											/* 更新 */
	CMDSTTMPLATETYPE_ALL,											/* 全て更新 */
};

/* 管理権限レベル */
enum {
	ADMINLV_USER = 0,												/* 一般ユーザー */
	ADMINLV_ADMINISTRATOR,											/* 全て許可 */
};

/* キャラ状態 */
enum {
	CHARSTATE_LOGOFF = 0,											/* ログオフ状態 */
	CHARSTATE_MOVE,													/* 歩き中 */
	CHARSTATE_DEAD,													/* 死亡中 */
};

/* 酔っ払いレベル */
enum {
	CHARSTATEDRUNK_NONE = 0,										/* 平常 */
	CHARSTATEDRUNK_DRANKARD,										/* 陶酔 */
	CHARSTATEDRUNK_COMA,											/* 泥酔 */
	CHARSTATEDRUNK_COMA2,											/* 狂酔 */
	CHARSTATEDRUNK_MAX
};

/* ゲーム状態 */
enum {
	GAMESTATE_LOGOFF = 0,										/* ログオフ状態 */
	GAMESTATE_LOGO,												/* ロゴ表示中 */
	GAMESTATE_TITLE,											/* タイトル表示中 */
	GAMESTATE_LOGIN,											/* ログイン画面 */
	GAMESTATE_CONNECTLOST,										/* 接続途切れた状態 */
	GAMESTATE_MOVE,												/* 歩き中 */
	GAMESTATE_NPC,												/* NPCとして歩き中 */
};

/* キャラグラフィック種別(メイン) */
enum {
	CHARGRP_MAIN_NORMAL = 0,									/* 標準 */
	CHARGRP_MAIN_SUBWAR,										/* サブゲーム：戦場ゲーム */
	CHARGRP_MAIN_1632,											/* 横16縦32サイズ */
	CHARGRP_MAIN_3232,											/* 横32縦32サイズ */
	CHARGRP_MAIN_4848,											/* 横48縦48サイズ */
};

/* キャラグラフィック種別(サブ：戦場ゲーム) */
enum {
	CHARGRP_SUBWAR_SUB_NORMAL = 0,								/* 標準 */
	CHARGRP_SUBWAR_SUB_DEAD,									/* やられ中 */
	CHARGRP_SUBWAR_SUB_NORMAL2,									/* 標準(暗いほう) */
	CHARGRP_SUBWAR_SUB_DEAD2,									/* やられ中(暗いほう) */
};

/* 音楽/効果音ID */
enum {
	SOUND_OK = 1,												/* 決定 */
	SOUND_BLOCK,												/* 衝突 */
	SOUND_MOVE,													/* 移動 */
	SOUND_MSG,													/* メッセージ */
	SOUND_DOWN,					/* 5 */							/* 落ちる */
	SOUND_DOOR,													/* 扉 */
	SOUND_BOX,													/* 宝箱 */
	SOUND_LVUP,													/* レベルアップ */
	SOUND_WC,													/* トイレ流す */
	SOUND_ATACK1,				/* 10 */						/* こちら攻撃 */
	SOUND_ATACK2,												/* 会心の一撃 */
	SOUND_ATACK3,												/* あちら攻撃 */
	SOUND_DAMAGE,												/* ダメージ */
	SOUND_MAGIC,												/* 呪文 */
	/* これ以上前にBGMは追加しない事 */
	SOUND_MIDI_BATTLE1,			/* 15 */
	SOUND_MIDI_BOSS1,
	SOUND_MIDI_BOSS2,
	SOUND_MIDI_CASTLE1,
	SOUND_MIDI_CASTLE2,
	SOUND_MIDI_CASTLE3,			/* 20 */
	SOUND_MIDI_CHURCH1,
	SOUND_MIDI_CHURCH2,
	SOUND_MIDI_CLIMAX,
	SOUND_MIDI_FIELD1,
	SOUND_MIDI_FIELD2,			/* 25 */
	SOUND_MIDI_FIELD3,
	SOUND_MIDI_FIELD4,
	SOUND_MIDI_FIELD5,
	SOUND_MIDI_FIELD6,
	SOUND_MIDI_LASTBATTLE,		/* 30 */
	SOUND_MIDI_MEMORY,
	SOUND_MIDI_MENU,
	SOUND_MIDI_MISSION,
	SOUND_MIDI_MOUTAIN,
	SOUND_MIDI_PINCH1,			/* 35 */
	SOUND_MIDI_PINCH2,
	SOUND_MIDI_SANCT,
	SOUND_MIDI_SHRINE,
	SOUND_MIDI_TOWER1,
	SOUND_MIDI_TOWER2,			/* 40 */
	SOUND_MIDI_TOWER3,
	SOUND_MIDI_TOWN1,
	SOUND_MIDI_TOWN1NIGHT,
	SOUND_MIDI_TOWN2,
	SOUND_MIDI_VILLAGE,			/* 45 */
	SOUND_MIDI_CLOCK,
	SOUND_MIDI_HALF,
	SOUND_MIDI_MBOX007,
	SOUND_MIDI_NEKO,
	SOUND_MIDI_OPENING,			/* 50 */
	SOUND_MIDI_ORGANEXACISE,
	SOUND_MIDI_TIRUTOKI,
	SOUND_MIDI_TOMADOI,
	SOUND_BARD,
	SOUND_CURE,					/* 55 */
	SOUND_FAIL,
	SOUND_HEE,
	SOUND_SIREN,
	SOUND_PINPON,
	SOUND_WHISTLE,				/* 60 */
	SOUND_VOICE,
	SOUND_GU,
	SOUND_CHOKI,
	SOUND_PA,
	SOUND_RURA,					/* 65 */
	SOUND_MIDI_MBOX001,
	SOUND_MIDI_CREAM,
	SOUND_MIDI_NATUNO,
	SOUND_HAKUSYU,
	SOUND_CONFIRM,				/* 70 */
	SOUND_MIDI_FOREST,
	SOUND_BOM25,
	SOUND_BOM28_A,
	SOUND_GUN08,
	SOUND_GUN16,				/* 75 */
	SOUND_BURST00,
	SOUND_CLOSE01,
	SOUND_VOICE011,
	SOUND_POWER01,
	SOUND_POWER01_B,			/* 80 */
	SOUND_PYORO33,
	SOUND_PYORO57_B,
	SOUND_POWER26,
	SOUND_POWER32,
	SOUND_GUN10,				/* 85 */
	SOUND_GORO02,
	SOUND_GORO03,
	SOUND_GUN11,
	SOUND_POWER10,
	SOUND_FREEZE02,				/* 90 */
	SOUND_FREEZE00,
	SOUND_PYORO08,
	SOUND_HIT12,
	SOUND_POWER17,
	SOUND_POWER14,				/* 91 */
	SOUND_PUU23,
	SOUND_SMOKE03,
	SOUND_FREEZE01,
	SOUND_FREEZE03,				/* 95 */
	SOUND_POWER07,
	SOUND_POWER16,
	SOUND_PUU27,
	SOUND_MECHA09,
	SOUND_MECHA09_R,			/* 100 */
	SOUND_POWER23,
	SOUND_PUU38,
	SOUND_BOM17_A,
	SOUND_DON07,
	SOUND_GUN14_C,				/* 101 */
	SOUND_BOM14,
	SOUND_GUN13_B,
	SOUND_BOM26_A,
	SOUND_BOM26_C,				/* 105 */
	SOUND_PUSH40,
	SOUND_PUU06,
	SOUND_PUU09,
	SOUND_PUU10,
	SOUND_DON00,				/* 106 */
	SOUND_DRAG02,
	SOUND_HIT_S02,
	SOUND_HIT17,
	SOUND_HIT65,				/* 110 */
	SOUND_HIT67,
	SOUND_HITS01_B,
	SOUND_POWER22,
	SOUND_POWER09,
	SOUND_POWER19,				/* 115 */
	SOUND_POWER33,
	SOUND_HIS_S06,
	SOUND_BOSU09,
	SOUND_HIT_b02_B,
	SOUND_SWING36_B,			/* 120 */
	SOUND_HIT07,
	SOUND_HIT10_B,
	SOUND_HIT25,
	SOUND_HIT26,
	SOUND_SHA00,				/* 125 */
	SOUND_SHA04,
	SOUND_FREEZE04,
	SOUND_SWING14,
	SOUND_HIT68,
	SOUND_HIT70,				/* 130 */
	SOUND_SEE,
	SOUND_MIDI_BOKYO,
	SOUND_MIDI_AIRSHIP1,
	SOUND_MIDI_MAIN2,
	SOUND_ATACKMISS,			/* 135 */
	SOUND_DAMAGE0,
	SOUND_BOMBSET,
	SOUND_BOMBBOMBER,
	SOUND_MIDI_YUKIZATO,
	SOUND_MIDI_BACATION,		/* 140 */
	SOUND_MIDI_BATTLE,
	SOUND_MIDI_DANCE,
	SOUND_MIDI_DENNEN,
	SOUND_MIDI_DESTRATION,
	SOUND_MIDI_DREAMING,
	SOUND_MIDI_KAZAN,
	SOUND_MIDI_KOUJOUTITAI,
	SOUND_MIDI_MINAMI,
	SOUND_MIDI_OYASUMI03,
	SOUND_MIDI_RYOSHU,			/* 150 */
	SOUND_MIDI_SINSIN,
	SOUND_MIDI_SNOW,
	SOUND_MIDI_YELLOW,
	SOUND_MIDI_ANSWER,
	SOUND_MIDI_NOUSONN,
	SOUND_PUSH51_C,
	SOUND_SHOOT15,
	SOUND_CURSOR08,
	SOUND_POWER21,
	SOUND_FM001,				/* 160 */
	SOUND_PI52,
	SOUND_MIDI_W0053,
	SOUND_MIDI_W0054,
	SOUND_MIDI_W0077,
	SOUND_MIDI_W0035,
	SOUND_MIDI_W0021,
	SOUND_MIDI_W0068,
	SOUND_MIDI_W0102,
	SOUND_MIDI_W0030,
	SOUND_MIDI_W0075,			/* 170 */
	SOUND_MIDI_W0016,
	SOUND_MIDI_W0040,
	SOUND_MIDI_W0069,
	SOUND_MIDI_W0071,
	SOUND_MIDI_W0014,
	SOUND_MIDI_W0113,
	SOUND_MIDI_W0081,
	SOUND_CAT,
	SOUND_MIDI_W0020,
	SOUND_MIDI_W0029,			/* 180 */
	SOUND_MIDI_W0039,
	SOUND_MIDI_W0041,
	SOUND_MIDI_W0057,
	SOUND_MIDI_W0067,
	SOUND_MIDI_W0120,
	SOUND_MIDI_W0123,
	SOUND_MIDI_W0125,
	SOUND_MIDI_W0130,
	SOUND_MIDI_W0136,
	SOUND_MIDI_WJ013,			/* 190 */
	SOUND_MIDI_WJ014,
	SOUND_MIDI_WS003,
	SOUND_MIDI_W0043,
	SOUND_CRASH18_C,
	SOUND_DON12,
	SOUND_FM003,
	SOUND_LIP01,
	SOUND_SWING32,
	SOUND_KANE,
	SOUND_MIDI_KIRAKIRA,		/* 200 */
	SOUND_MIDI_SILENT,
	SOUND_MIDI_TIRUFUSE,
	SOUND_MIDI_HAPPYBIRTHDAY,
	SOUND_MIDI_JINGLEBELL,
	SOUND_MIDI_SHOGATU,
	SOUND_MIDI_S_NIGHT_S,
	SOUND_MIDI_JANGURU,
	SOUND_LVUP2,
	SOUND_MIDI_OMOIDE,
	SOUND_MIDI_W0101,
	SOUND_MIDI_W0045,
	SOUND_MIDI_W0064,
	SOUND_MIDI_W0132,
	SOUND_VOICE012,
	SOUND_VOICE028,
	SOUND_BYORO02A,
	SOUND_BYORO06A,
	SOUND_PUQU1,				/* 220 */
	SOUND_PUQU2,
	SOUND_ONPA,
	SOUND_KIDAN,
	SOUND_KIDANHIT,
	SOUND_LASER,
	SOUND_TUTI1,
	SOUND_TUTI2,
	SOUND_TUTIHIT,
	SOUND_ONPAATACK,
	SOUND_EFFECT142,			/* 230 */
	SOUND_GUN24,
	SOUND_LVUP_SOUND,
	SOUND_SANRENGEKI,
	SOUND_SANRENZAN,
	SOUND_JUJITUKI,
	SOUND_JUJITUKI2,
	SOUND_SYOKAN,
	SOUND_DAIBAKUHATU,
	SOUND_TRAP,
	SOUND_BARA,					/* 240 */
	SOUND_TUTIDAI,
	SOUND_MIDI_WO002,
	SOUND_MIDI_WO006,
	SOUND_MIDI_W0009,
	SOUND_MIDI_W0100,
	SOUND_MAX
};

/* エフェクト種別 */
enum {
	EFECTTYPE_NONE = 0,
	EFECTTYPE_ATACK,											/* 攻撃1 */
	EFECTTYPE_ATACK2,											/* 攻撃2 */
	EFECTTYPE_MAGIC,											/* 魔法 */
	EFECTTYPE_SWORD1,											/* 剣撃1 */
	EFECTTYPE_SWORD2,											/* 剣撃2 */
	EFECTTYPE_WHIP,												/* 鞭 */
	EFECTTYPE_SPEAR,											/* 槍 */
	EFECTTYPE_STRICK,											/* 叩撃 */
	EFECTTYPE_BLOW1,											/* 打撃1 */
	EFECTTYPE_BLOW2,											/* 打撃2 */
	EFECTTYPE_TOE,												/* 爪 */
	EFECTTYPE_HOIMI,											/* ホイミ */
	EFECTTYPE_STSTUS,											/* 状態回復 */
};

/* 発言コマンド配信種別 */
enum {
	SPEECHCMD_AROUND = 0,										/* 画面内 */
	SPEECHCMD_ALL,												/* 全プレイヤー */
	SPEECHCMD_GM,												/* 管理者用内線 */
	SPEECHCMD_SPEECHEDIT,										/* 自己紹介設定 */
	SPEECHCMD_BATTLE1,											/* 戦闘関連(ログ保存しない) */
	SPEECHCMD_BATTLE2,											/* 戦闘関連(ログ保存する) */
	SPEECHCMD_MAP,												/* マップ内放送 */
	SPEECHCMD_SCREEN,											/* 画面内放送 */
	SPEECHCMD_SCREENCHAR,										/* 画面内放送(キャラの色) */
};

/* イベント種別 */
enum {
	EVENTTYPE_MOVE = 0,											/* 座標変更 */
	EVENTTYPE_MAPCHG,											/* マップ間移動 */
	EVENTTYPE_MSG,												/* メッセージ */
	EVENTTYPE_DOOR,												/* 扉 */
	EVENTTYPE_HPMANA,											/* HP/MANA回復 */
	EVENTTYPE_STATUSINIT,										/* ステータス初期化 */
	EVENTTYPE_ONCHARMAP,										/* キャラが乗っている間マップ変更 */
	EVENTTYPE_MAX,
};

/* 移動時のエフェクト種別 */
enum {
	EVENTMOVEEFECT_SILENT = 0,									/* 無音 */
	EVENTMOVEEFECT_MOVE,										/* 階段音 */
	EVENTMOVEEFECT_DOWN,										/* 落ちる音 */
	EVENTMOVEEFECT_SEE,											/* 波の音 */
	EVENTMOVEEFECT_MAX,
};

/* メッセージ発動種別 */
enum {
	EVENTMSGTYPE_MOVE = 0,										/* 自動発動 */
	EVENTMSGTYPE_FRONT,											/* 一歩前 */
	EVENTMSGTYPE_TALK,											/* 話しかける */
	EVENTMSGTYPE_MAX,
};

/* 扉種別 */
enum {
	EVENTDOOTTYPE_NONE = 0,
	EVENTDOOTTYPE_MAX,
};

/* 回復種別 */
enum {
	EVENTHPMANATYPE_HP = 0,										/* HP回復 */
	EVENTHPMANATYPE_MANA,										/* MANA回復 */
	EVENTHPMANATYPE_HPMANA,										/* HP/MANA回復 */
	EVENTHPMANATYPE_MAX,
};

/* 店メッセージ種別 */
enum {
	SHOPMSG_OPEN = 0,											/* 取り引き開始 */
	SHOPMSG_CLOSE,												/* 取り引き終了時 */
	SHOPMSG_CONTINUE,											/* 次への繋ぎ */
	SHOPMSG_BUY,												/* 買う時 */
	SHOPMSG_BUYGOLD,											/* 所持金不足 */
	SHOPMSG_BUYTAKE,											/* もう持てない時 */
	SHOPMSG_BUYCONF,											/* 買うか確認 */
	SHOPMSG_BUYCLOSE,											/* 買い終わった時 */
	SHOPMSG_SELL,												/* 売る時 */
	SHOPMSG_SELLCONF,											/* 買い取り確認 */
	SHOPMSG_SELLNO,												/* 買い取り不可 */
	SHOPMSG_SELLCLOSE,											/* 買い取り終わった時 */
};

/* 預かり所状態 */
enum {
	LEAVESTATE_OPEN = 0,										/* 取り引き開始 */
	LEAVESTATE_SELECT,											/* 選択(預ける) */
	LEAVESTATE_SELECTGET,										/* 選択(引き取る) */
	LEAVESTATE_ITEM,											/* アイテム選択 */
	LEAVESTATE_ITEMSELECTGET,									/* 引き取るアイテム選択 */
	LEAVESTATE_GOLD,											/* 預ける金額設定 */
	LEAVESTATE_GOLDGET,											/* 引き取る金額設定 */
};

/* 冒険者登録所状態 */
enum {
	JOBSTATE_OPEN = 0,											/* 選択メニュー */
	JOBSTATE_JOBSELECT,											/* 職業選択 */
};

/* マップパーツ当たり判定 */
enum {
	PARTSHIT_NONE = 0,											/* ぶつからない */
	PARTSHIT_BLOCK1,											/* ぶつかる(音付き) */
	PARTSHIT_BLOCK2,											/* ぶつかる(音無し) */
	PARTSHIT_HIDE1,												/* 隠れる */
	PARTSHIT_HIDE2,												/* 隠れる(ぶつかる) */
	PARTSHIT_COUNTER,											/* カウンター */
	PARTSHIT_FOOTMARK_SNOW,										/* 足跡(雪) */
	PARTSHIT_FOOTMARK_SAND,										/* 足跡(砂) */
	PARTSHIT_HIDE3,												/* 隠れる3 */
	PARTSHIT_HIDE4,												/* 隠れる4(ぶつかる) */
	PARTSHIT_HIDE5,												/* 隠れる5(背景ごと隠れる) */
	PARTSHIT_HIDE6,												/* 隠れる6(ぶつかる/音無し) */
	PARTSHIT_MOVEFRONT,											/* 一歩前へ進む */
	PARTSHIT_MOVEUP,											/* 一歩上へ進む */
	PARTSHIT_MOVEDOWN,											/* 一歩下へ進む */
	PARTSHIT_MOVERIGHT,											/* 一歩左へ進む */
	PARTSHIT_MOVELEFT,											/* 一歩右へ進む */
	PARTSHIT_WATERRIPPLE,										/* 水の波紋 */
	PARTSHIT_HIDE7,												/* 隠れる7(隠れる3で背景ごと隠れる) */
	PARTSHIT_MAX
};

/* NPC種別 */
enum {
	NPCTYPE_INIT = 0,											/* 未設定 */
	NPCTYPE_NOMOVE,												/* 移動しない */
	NPCTYPE_MOVERAND,											/* ランダムに移動 */
	NPCTYPE_BLOCK,												/* 押せる */
	NPCTYPE_BLOCK2,												/* 押せる(サイコロ) */
	NPCTYPE_PUT,												/* NPC発生 */
	NPCTYPE_SCORE,												/* 得点板 */
	NPCTYPE_TALKRAND,											/* 話すとランダム */
	NPCTYPE_STOPRAND,											/* 止まってるかランダムか */
	NPCTYPE_BLOCK3,												/* 押せる(サイコロ２) */
	NPCTYPE_BLOCK4,					/* 10 */					/* 押せる(サイコロ３) */
	NPCTYPE_BATTLE1,											/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
	NPCTYPE_SEARCH,												/* 周り２キャラの範囲で検索してついていく */
	NPCTYPE_BATTLE2,											/* 周り２キャラの範囲で検索してついていって攻撃 */
	NPCTYPE_NOBLOCK,											/* 移動しない(通り抜け可) */
	NPCTYPE_BOMB,												/* 爆弾（サブゲーム用） */
	NPCTYPE_USEMAGIC,											/* 重なったキャラに魔法発動 */
	NPCTYPE_ATACK,												/* 重なったキャラに攻撃 */
	NPCTYPE_BOMBBLOCK,											/* 爆弾ゲームの壁 */
	NPCTYPE_BOMBITEM,											/* 爆弾ゲームのアイテム */
	NPCTYPE_USEMAGICPARENT,										/* 魔法発動し続ける */
	NPCTYPE_CHGPUSH,											/* 押せて話すと切り替え */
	NPCTYPE_RESERVE4,
	NPCTYPE_SEARCH2,											/* 検索してついていく2 */
	NPCTYPE_MAX
};

/* NPC巡回ルートの回り方 */
enum {
	NPCROUTEMODE_BACK = 0,										/* 最後まで行ったら折り返す */
	NPCROUTEMODE_LOOP,											/* 最後まで行ったら最初に戻る */
	NPCROUTEMODE_MAX
};

/* NPC移動種別 */
enum {
	NPCMOVETYPE_NOMOVE = 0,										/* 移動しない */
	NPCMOVETYPE_RAND,											/* ランダムに移動 */
};

/* アイテム種別 */
enum {
	ITEMTYPE_NONE = 0,											/* 効果無し */
	ITEMTYPE_RURA,												/* 特定位置へ移動 */
	ITEMTYPE_HPUP,												/* HP回復 */
	ITEMTYPE_WEAPON,											/* 武器 */
	ITEMTYPE_ARMOR,												/* 鎧 */
	ITEMTYPE_SHILD,												/* 盾 */
	ITEMTYPE_HELMET,											/* 兜 */
	ITEMTYPE_SEED,												/* 種 */
	ITEMTYPE_SUBGAME,											/* サブゲーム参加 */
	ITEMTYPE_SAKE,												/* 酒 */
	ITEMTYPE_MSG,												/* メッセージ表示 */
	ITEMTYPE_GRPCHG,											/* キャラ画像変更 */
	ITEMTYPE_LIGHT,												/* 明かり */
	ITEMTYPE_HPDOWN,											/* HP減少 */
	ITEMTYPE_MAGIC,												/* 魔法習得 */
	ITEMTYPE_MANAUP,											/* Mana回復 */
	ITEMTYPE_HPMANAUP,											/* HP・Mana回復 */
	ITEMTYPE_EQU_HAND,											/* 装備(小手) */
	ITEMTYPE_EQU_LEG,											/* 装備(靴) */
	ITEMTYPE_EQU_ACC1,											/* 装備(首飾り) */
	ITEMTYPE_EQU_ACC2,											/* 装備(指輪1) */
	ITEMTYPE_EQU_ACC3,											/* 装備(指輪2) */
	ITEMTYPE_RESURRECTION,										/* 蘇生 */
	ITEMTYPE_USEMAGIC,											/* 特技発動(詳細は ITEMINFO_MAGIC と同じ形。byID=発動する特技) */
	ITEMTYPE_MAX
};

/* 装備効果種別 */
enum {
	EQUTYPE_NONE = 0,											/* 効果無し */
	EQUTYPE_DF,													/* 守備力 */
	EQUTYPE_DFMAGIC,											/* 魔法防御 */
};

/* 性別による種別 */
enum {
	SEXDISTINCTION_ALL = 0,										/* 全員 */
	SEXDISTINCTION_MALE,										/* 男性のみ */
	SEXDISTINCTION_FEMALE,										/* 女性のみ */
	SEXDISTINCTION_MAX
};

/* ステータス種別 */
enum {
	STATUSTYPE_PW = 0,											/* 力 */
	STATUSTYPE_DEX,												/* 素早さ */
	STATUSTYPE_ST,												/* 体力 */
	STATUSTYPE_IT,												/* 賢さ */
	STATUSTYPE_LK,												/* 運のよさ */
	STATUSTYPE_MAXHP,											/* 最大HP */
	STATUSTYPE_MAXMANA,											/* 最大MANA */
	STATUSTYPE_PW2,												/* 力減少 */
	STATUSTYPE_DEX2,											/* 素早さ減少 */
	STATUSTYPE_ST2,												/* 体力減少 */
	STATUSTYPE_IT2,												/* 賢さ減少 */
	STATUSTYPE_LK2,												/* 運のよさ減少 */
	STATUSTYPE_MAXHP2,											/* 最大HP減少 */
	STATUSTYPE_MAXMANA2,										/* 最大MANA減少 */
	STATUSTYPE_AT,												/* 攻撃力 */
	STATUSTYPE_DF,												/* 守備力 */
	STATUSTYPE_DFMAGIC,											/* 魔法防御 */
	STATUSTYPE_ATTR_FIRE,										/* 属性(火) */
	STATUSTYPE_ATTR_WATER,										/* 属性(水) */
	STATUSTYPE_ATTR_WIND,										/* 属性(風) */
	STATUSTYPE_ATTR_EARTH,										/* 属性(土) */
	STATUSTYPE_ATTR_LIGHT,										/* 属性(光) */
	STATUSTYPE_ATTR_DARK,										/* 属性(闇 */
};

/* サブゲーム種別 */
enum {
	SUBGAMEID_NONE = 0,
	SUBGAMEID_BOMB,												/* 爆弾ゲーム */
	SUBGAMEID_MAZE,												/* 迷路 */
	SUBGAMEID_DUNGEON,											/* ダンジョン */
	SUBGAMEID_WAR,												/* 戦場ゲーム */
	SUBGAMEID_SUGOROKU,											/* すごろく */
	SUBGAMEID_MAX
};

/* オブジェクト種別 */
enum {
	OBJTYPE_NONE = 0,
	OBJTYPE_FOOTMARK_SNOW,										/* 足跡(雪) */
	OBJTYPE_FOOTMARK_SAND,										/* 足跡(砂) */
	OBJTYPE_WATERRIPPLE,										/* 水の波紋 */
	OBJTYPE_MAX
};

/* メッセージコマンド */
enum {
	MSGCMD_NONE = 0,
	MSGCMD_DICE,												/* サイコロ(1～6) */
	MSGCMD_KENO,												/* ケノ(1～80) */
	MSGCMD_COIN,												/* コイン(裏表) */
	MSGCMD_WDICE,												/* 戦場ゲーム */
	MSGCMD_2D6,													/* サイコロ(1～6を2個) */
	MSGCMD_3D6,													/* サイコロ(1～6を3個) */
	MSGCMD_1D10,												/* サイコロ(1～10) */
	MSGCMD_1D20,												/* サイコロ(1～20) */
	MSGCMD_CLOCK,												/* 現在時刻 */
	MSGCMD_MAX
};

/* システムメッセージ */
enum {
	SYSMSG_ATACK1 = 1,											/* "%sの攻撃" */
	SYSMSG_ATACK2,												/* "%sのクリティカルヒット" */
	SYSMSG_DAMAGE1,												/* "%sに%dのダメージ" */
	SYSMSG_MISS,												/* "ミス" */
	SYSMSG_NODAMAGE,											/* "ダメージを与えられませんでした" */
	SYSMSG_BATTLEEND1,											/* "%sを倒しました" */
	SYSMSG_BATTLEEND2,											/* "%sは%sに倒されました" */
	SYSMSG_GETEXP,												/* "%sは%uポイントの経験値を得ました" */
	SYSMSG_BATTLEEND3,											/* "%sは%ldゴールド手に入れました" */
	SYSMSG_DEAD,												/* "%sは死んでしまいました" */
	SYSMSG_TARGET1,												/* "ターゲットを%sに設定" */
	SYSMSG_TARGET2,												/* "ターゲットを解除" */
	SYSMSG_GETITEM1,											/* "%sは%sを持っていました" */
	SYSMSG_GETITEM2,											/* "しかし持てないのであきらめました" */
	SYSMSG_GETITEM3,											/* "%sは%sを手に入れました" */
	SYSMSG_NOPROC,												/* "%sには効果がありません" */
	SYSMSG_HPMAX,												/* "%sのHPは完全に回復しました" */
	SYSMSG_HPADD,												/* "%sのHPが%dポイント回復しました" */
	SYSMSG_NOPROC2,												/* "ここで使う必要はありません" */
	SYSMSG_NOUSE,												/* "ここでは使えません" */
	SYSMSG_USESPECIALTY,										/* "%sは%sを使用しました" */
	SYSMSG_RESURRECTION,										/* "%sは生き返りました" */
};

/* 魔法効果種別 */
enum {
	MAGICTYPE_NONE = 0,											/* 効果無し */
	MAGICTYPE_HPDOWN,											/* HP減少 */
	MAGICTYPE_HPUP,												/* HP増加 */
	MAGICTYPE_LIGHT,											/* 明かり */
	MAGICTYPE_PUT,												/* 魔法発生 */
	MAGICTYPE_RETURN,											/* 帰還 */
	MAGICTYPE_HPDOWNAT,											/* HP減少(攻撃力) */
	MAGICTYPE_PUTSELF,											/* 魔法発生(自分) */
	MAGICTYPE_RESURRECTION,										/* 蘇生 */
	MAGICTYPE_HPDOWNHPCOST,										/* HP減少(HP消費) */
	MAGICTYPE_HPDOWNRATE,										/* HP減少(率) */
	MAGICTYPE_HASTE,											/* 加速(付加データ1:速くなる率(%) 2:秒数) */
	MAGICTYPE_POISON,											/* 毒(付加データ1:2秒ごとのダメージ 2:秒数 3:成功率(%、0なら100)) */
	MAGICTYPE_SLEEP,											/* 眠り(付加データ1:秒数 2:成功率(%、0なら100)) */
};

/* 魔法利用情報 */
enum {
	MAGICUSETYPE_WALK = 0,										/* 平常状態 */
	MAGICUSETYPE_BATTLE,										/* 戦闘状態 */
	MAGICUSETYPE_WALKBATTLE,									/* 平常状態と戦闘状態 */
};

/* 魔法使用対象 */
enum {
	MAGICTARGET_PC = 0,											/* PC */
	MAGICTARGET_NPC,											/* NPC */
	MAGICTARGET_ALL,											/* 両方 */
};

/* 魔法発動人数 */
enum {
	MAGICTARGETCOUNT_ONE = 0,									/* 一人 */
	MAGICTARGETCOUNT_ALL,										/* 全員 */
};

/* 魔法発動種別 */
enum {
	MAGICTARGETTYPE_SELF = 0,									/* 自分 */
	MAGICTARGETTYPE_FRONT,										/* 前方 */
	MAGICTARGETTYPE_CIRCLE,										/* 周囲 */
	MAGICTARGETTYPE_SCREEN,										/* 画面内 */
	MAGICTARGETTYPE_FRONTSIDE,									/* 前方横一列 */
	MAGICTARGETTYPE_SELECT,										/* 選択 */
	MAGICTARGETTYPE_SELECT2,									/* 選択(自分以外) */
};

/* 属性 */
enum {
	ATTRTYPE_NONE = 0,											/* 無し */
	ATTRTYPE_FIRE,												/* 火 */
	ATTRTYPE_WATER,												/* 水 */
	ATTRTYPE_WIND,												/* 風 */
	ATTRTYPE_EARTH,												/* 土 */
	ATTRTYPE_LIGHT,												/* 光 */
	ATTRTYPE_DARK,												/* 闇 */
	ATTRTYPE_MAX
};

/* 攻撃種別 */
enum {
	ATACKTYPE_NORMAL = 0,										/* 直接攻撃 */
	ATACKTYPE_ARROW,											/* 矢 */
	ATACKTYPE_SWEEP,											/* なぎ払い(目の前と、自分の左右のマスにも当たる。斧など) */
	ATACKTYPE_PIERCE,											/* 貫通(目の前と、その1マス先にも当たる。槍など) */
	ATACKTYPE_MAX
};

/* キャラ情報タイマーイベント */
enum {
	CHARINFOTIMEREVENT_NONE = 0,
	CHARINFOTIMEREVENT_PW,										/* 力 */
	CHARINFOTIMEREVENT_ST,										/* 体力 */
	CHARINFOTIMEREVENT_DEX,										/* 素早さ */
	CHARINFOTIMEREVENT_HPUPDOWN,								/* HP増減 */
	CHARINFOTIMEREVENT_DIFENCE,									/* 防御力倍 */
	CHARINFOTIMEREVENT_HASTE,									/* 加速(byData=速くなる率%) */
	CHARINFOTIMEREVENT_POISON,									/* 毒(byData=2秒ごとのダメージ) */
	CHARINFOTIMEREVENT_SLEEP,									/* 眠り(dwTimeLast=眠った時のHP。減ったら起きる) */
};


/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* キャラ情報タイマーイベント */
typedef struct _CHARINFOTIMEREVENT {
	BYTE		byType;											/* 種別 */
	BYTE		byData;											/* 使用する値 */
	BYTE		byEfect;										/* 発動時のエフェクト番号 */
	DWORD		dwTimeLast;										/* 最終更新時間 */
	DWORD		dwTimeInterval;									/* 更新間隔 */
	DWORD		dwTimeStart;									/* 開始時間 */
	DWORD		dwTimeValid;									/* 有効時間 */
} CHARINFOTIMEREVENT, *PCHARINFOTIMEREVENT;

/* 座標情報 */
typedef struct _POSINFO {
	DWORD		dwMapID;											/* マップID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
} POSINFO, *PPOSINFO;

/* サブゲーム情報 */
typedef struct _SUBGAMEINFO {
	BYTE		byType;												/* 種別 */
	PVOID		pInfo;												/* 詳細情報 */
} SUBGAMEINFO, *PSUBGAMEINFO;

/* サブゲーム：ダンジョン情報 */
typedef struct _SUBGAME_DUNGEONINFO {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byCharX;											/* 管理人キャラ座標(X) */
	BYTE		byCharY;											/* 管理人キャラ座標(Y) */
	BYTE		byStartX;											/* スタート地点(X) */
	BYTE		byStartY;											/* スタート地点(Y) */
	BYTE		byGoalX;											/* ゴール地点(移動元X) */
	BYTE		byGoalY;											/* ゴール地点(移動元Y) */
	BYTE		byTmpX;												/* 作業中(X) */
	BYTE		byTmpY;												/* 作業中(Y) */
	WORD		wChipIDStart;										/* 開始地点へのマップチップID */
	WORD		wChipIDGoal;										/* ゴール地点のマップチップID */
	WORD		wChipIDRoad;										/* 道のマップチップID */
	WORD		wChipIDBlock;										/* 壁のマップチップID */
	BYTE		*pMapTmp;											/* 作成するマップ */
	DWORD		dwTblID;											/* 管理人キャラのテーブルID */
	DWORD		dwCountTmp;											/* 参加人数カウント用 */
	DWORD		dwTimeCheck;										/* 最後に人数チェックした時間 */
	DWORD		dwTblIDCheck;										/* 最後に人数チェックしたテーブルID */
} SUBGAME_DUNGEONINFO, *PSUBGAME_DUNGEONINFO;

/* サブゲーム：ダンジョン情報(保存データ) */
typedef struct _SUBGAME_DUNGEONINFOSAVE {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byCharX;											/* 管理人キャラ座標(X) */
	BYTE		byCharY;											/* 管理人キャラ座標(Y) */
	BYTE		byStartX;											/* スタート地点(X) */
	BYTE		byStartY;											/* スタート地点(Y) */
	BYTE		byGoalX;											/* ゴール地点(移動元X) */
	BYTE		byGoalY;											/* ゴール地点(移動元Y) */
	WORD		wChipIDStart;										/* 開始地点へのマップチップID */
	WORD		wChipIDGoal;										/* ゴール地点のマップチップID */
	WORD		wChipIDRoad;										/* 道のマップチップID */
	WORD		wChipIDBlock;										/* 壁のマップチップID */
} SUBGAME_DUNGEONINFOSAVE, *PSUBGAME_DUNGEONINFOSAVE;

/* サブゲーム：迷路情報 */
typedef struct _SUBGAME_MAZEINFO {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byCharX;											/* 管理人キャラ座標(X) */
	BYTE		byCharY;											/* 管理人キャラ座標(Y) */
	BYTE		byStartX;											/* スタート地点(X) */
	BYTE		byStartY;											/* スタート地点(Y) */
	BYTE		byGoalFromX;										/* ゴール地点(移動元X) */
	BYTE		byGoalFromY;										/* ゴール地点(移動元Y) */
	BYTE		byGoalToX;											/* ゴール地点(移動先X) */
	BYTE		byGoalToY;											/* ゴール地点(移動先Y) */
	BYTE		byTmpX;												/* 作業中(X) */
	BYTE		byTmpY;												/* 作業中(Y) */
	WORD		wChipIDStart;										/* 開始地点へのマップチップID */
	WORD		wChipIDGoal;										/* ゴール地点のマップチップID */
	WORD		wChipIDRoad;										/* 道のマップチップID */
	WORD		wChipIDBlock;										/* 壁のマップチップID */
	BYTE		*pMapTmp;											/* 作成するマップ */
	DWORD		dwTblID;											/* 管理人キャラのテーブルID */
	DWORD		dwCountTmp;											/* 参加人数カウント用 */
	DWORD		dwTimeCheck;										/* 最後に人数チェックした時間 */
	DWORD		dwTblIDCheck;										/* 最後に人数チェックしたテーブルID */
} SUBGAME_MAZEINFO, *PSUBGAME_MAZEINFO;

/* サブゲーム：迷路情報(保存データ) */
typedef struct _SUBGAME_MAZEINFOSAVE {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byCharX;											/* 管理人キャラ座標(X) */
	BYTE		byCharY;											/* 管理人キャラ座標(Y) */
	BYTE		byStartX;											/* スタート地点(X) */
	BYTE		byStartY;											/* スタート地点(Y) */
	BYTE		byGoalFromX;										/* ゴール地点(移動元X) */
	BYTE		byGoalFromY;										/* ゴール地点(移動元Y) */
	BYTE		byGoalToX;											/* ゴール地点(移動先X) */
	BYTE		byGoalToY;											/* ゴール地点(移動先Y) */
	WORD		wChipIDStart;										/* 開始地点へのマップチップID */
	WORD		wChipIDGoal;										/* ゴール地点のマップチップID */
	WORD		wChipIDRoad;										/* 道のマップチップID */
	WORD		wChipIDBlock;										/* 壁のマップチップID */
} SUBGAME_MAZEINFOSAVE, *PSUBGAME_MAZEINFOSAVE;

/* サブゲーム：爆弾ゲーム情報 */
typedef struct _SUBGAME_BOMBINFO {
	DWORD		dwMapID;											/* マップID */
	DWORD		dwX;												/* 参加受付X座標 */
	DWORD		dwY;												/* 参加受付Y座標 */
	DWORD		dwStartX;											/* 参加開始X座標 */
	DWORD		dwStartY;											/* 参加開始Y座標 */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byGameTime;											/* 制限時間 */
	BYTE		byGameTimeNow;										/* 残り時間 */
	BYTE		byMaxBomb;											/* 最大設置数 */
	BYTE		byMaxFire;											/* 最大火力 */
	DWORD		dwLastTime;											/* 前回の時間更新 */
	DWORD		dwTblIDGameMaster;									/* ゲームマスターのテーブルID */
	BYTE		byFlgGameNow	: 1;								/* 試合中？ */
	BYTE		byFlgUseBlock	: 1;								/* 壊れる壁使う／使わない */
	BYTE		byFlgUseItem	: 1;								/* アイテム使う／使わない */
	BYTE		byFlgProgress	: 1;								/* 途中参加あり？ */
	BYTE		byReserve		: 4;								/* 予約 */
} SUBGAME_BOMBINFO, *PSUBGAME_BOMBINFO;

/* サブゲーム：爆弾ゲーム情報(保存データ) */
typedef struct _SUBGAME_BOMBINFOSAVE {
	DWORD		dwMapID;											/* マップID */
	DWORD		dwX;												/* 参加受付X座標 */
	DWORD		dwY;												/* 参加受付Y座標 */
	DWORD		dwStartX;											/* 参加開始X座標 */
	DWORD		dwStartY;											/* 参加開始Y座標 */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
} SUBGAME_BOMBINFOSAVE, *PSUBGAME_BOMBINFOSAVE;

/* サブゲーム(クライアント側)：爆弾ゲーム情報 */
typedef struct _SUBGAMECLIENT_BOMBINFO {
	BYTE		byGameTimeNow;										/* 残り時間 */
	BYTE		byGameTime;											/* 制限時間 */
	BYTE		byMaxBomb;											/* 最大設置数 */
	BYTE		byMaxFire;											/* 最大火力 */
	BYTE		byFlgGameNow	: 1;								/* 試合中？ */
	BYTE		byFlgUseBlock	: 1;								/* 壊れる壁使う／使わない */
	BYTE		byFlgUseItem	: 1;								/* アイテム使う／使わない */
	BYTE		byFlgProgress	: 1;								/* 途中参加あり？ */
	BYTE		byReserve		: 4;								/* 予約 */
	DWORD		dwTblIDGameMaster;									/* ゲームマスターのテーブルID */
} SUBGAMECLIENT_BOMBINFO, *PSUBGAMECLIENT_BOMBINFO;

/* サブステータス情報(爆弾ゲーム) */
typedef struct _SUBSTATUS_BOMB {
	BYTE		byType;												/* サブゲーム種別 */
	BYTE		byID;												/* サブゲームID */
	BYTE		byPower;											/* 威力 */
	BYTE		byMaxCount;											/* 置ける数 */
	BYTE		byCount;											/* 置いた数 */
	BYTE		byFlgGameMaster		: 1;							/* ゲームマスター？ */
	BYTE		byFlgEntry			: 1;							/* 参加中 */
	BYTE		byFlgRemocon		: 1;							/* リモコン */
	BYTE		byFlgBlockThrough	: 1;							/* 壁すり抜け */
	BYTE		byFlgBombThrough	: 1;							/* 爆弾すり抜け */
	BYTE		byFlgRegistFire		: 1;							/* 無敵 */
	BYTE		byFlgReserve		: 2;							/* 予約 */
} SUBSTATUS_BOMB, *PSUBSTATUS_BOMB;

/* サブゲーム：戦場ゲーム情報 */
typedef struct _SUBGAME_WARINFO {
	DWORD		dwMapID;											/* マップID */
	BYTE		byMember;											/* １チームの参加人数 */
	BYTE		byX;												/* 参加受付X座標 */
	BYTE		byY;												/* 参加受付Y座標 */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byHP1X;												/* チーム１のHP(X) */
	BYTE		byHP2X;												/* チーム２のHP(X) */
	BYTE		byHPY;												/* チームのHP(Y) */
	BYTE		byJudgeX;											/* 審判キャラX座標 */
	BYTE		byJudgeY;											/* 審判キャラY座標 */
	BYTE		byRes1X;											/* チーム１の蘇生カウンタX座標 */
	BYTE		byRes2X;											/* チーム２の蘇生カウンタX座標 */
	BYTE		byResY;												/* チームの蘇生カウンタY座標 */
	BYTE		byStart1X;											/* チーム１の開始X座標 */
	BYTE		byStart2X;											/* チーム２の開始X座標 */
	BYTE		byStartY;											/* チームの開始Y座標 */
	BYTE		byCastle1X;											/* チーム１の城X座標 */
	BYTE		byCastle2X;											/* チーム２の城X座標 */
	BYTE		byCastleY;											/* チームの城Y座標 */

	BYTE		byRule;												/* ルール */
	BYTE		byState;											/* ゲーム状態 */
	BYTE		byAllMember;										/* 現在の参加人数 */
	BYTE		byTurn;												/* 行動しているチーム */
	BYTE		byTurn1;											/* チーム１の行動キャラ */
	BYTE		byTurn2;											/* チーム２の行動キャラ */
	BYTE		abyJob1[MAX_SUBGAME_WARMEMBER];						/* チーム１の職業 */
	BYTE		abyJob2[MAX_SUBGAME_WARMEMBER];						/* チーム２の職業 */
	DWORD		dwWaitTblID;										/* 応答待ちキャラのテーブルID */
	DWORD		dwFirstTimeProc;									/* 最初に処理した時間 */
	DWORD		dwLastTimeProc;										/* 最後に処理した時間 */
	DWORD		dwNextTimeProc;										/* 次の処理待ち時間 */
	DWORD		dwTblIDJudge;										/* 審判キャラのテーブルID */
	DWORD		adwTblID1[MAX_SUBGAME_WARMEMBER];					/* チーム１のテーブルID */
	DWORD		adwTblID2[MAX_SUBGAME_WARMEMBER];					/* チーム２のテーブルID */
	DWORD		adwTblIDHP1[MAX_SUBGAME_WARMEMBER];					/* チーム１のHPのテーブルID */
	DWORD		adwTblIDHP2[MAX_SUBGAME_WARMEMBER];					/* チーム２のHPのテーブルID */
	DWORD		adwTblIDRes1[MAX_SUBGAME_WARMEMBER];				/* チーム１の蘇生カウンタのテーブルID */
	DWORD		adwTblIDRes2[MAX_SUBGAME_WARMEMBER];				/* チーム２の蘇生カウンタのテーブルID */
	DWORD		adwTblIDTargetRes1[MAX_SUBGAME_WARMEMBER];			/* チーム１のやられ中キャラのテーブルID */
	DWORD		adwTblIDTargetRes2[MAX_SUBGAME_WARMEMBER];			/* チーム２のやられ中キャラのテーブルID */
} SUBGAME_WARINFO, *PSUBGAME_WARINFO;

/* サブゲーム：戦場ゲーム情報(保存データ) */
typedef struct _SUBGAME_WARINFOSAVE {
	DWORD		dwMapID;											/* マップID */
	BYTE		byMember;											/* １チームの参加人数 */
	BYTE		byX;												/* 参加受付X座標 */
	BYTE		byY;												/* 参加受付Y座標 */
	BYTE		byX1;												/* エリア情報（開始X） */
	BYTE		byY1;												/* エリア情報（開始Y） */
	BYTE		byX2;												/* エリア情報（終了X） */
	BYTE		byY2;												/* エリア情報（終了Y） */
	BYTE		byHP1X;												/* チーム１のHP(X) */
	BYTE		byHP2X;												/* チーム２のHP(X) */
	BYTE		byHPY;												/* チームのHP(Y) */
	BYTE		byJudgeX;											/* 審判キャラX座標 */
	BYTE		byJudgeY;											/* 審判キャラY座標 */
	BYTE		byRes1X;											/* チーム１の蘇生カウンタX座標 */
	BYTE		byRes2X;											/* チーム２の蘇生カウンタX座標 */
	BYTE		byResY;												/* チームの蘇生カウンタY座標 */
	BYTE		byStart1X;											/* チーム１の開始X座標 */
	BYTE		byStart2X;											/* チーム２の開始X座標 */
	BYTE		byStartY;											/* チームの開始Y座標 */
	BYTE		byCastle1X;											/* チーム１の城X座標 */
	BYTE		byCastle2X;											/* チーム２の城X座標 */
	BYTE		byCastleY;											/* チームの城Y座標 */
} SUBGAME_WARINFOSAVE, *PSUBGAME_WARINFOSAVE;

/* サブステータス情報(戦場ゲーム) */
typedef struct _SUBSTATUS_WAR {
	BYTE		byType;												/* サブゲーム種別 */
	BYTE		byID;												/* サブゲームID */
	BYTE		byJob;												/* 職業 */
	BYTE		byState;											/* 状態 */
	BYTE		byReserve;
	BYTE		byFlgTeam			: 2;							/* チーム */
	BYTE		byFlgReserve		: 6;							/* 予約 */
} SUBSTATUS_WAR, *PSUBSTATUS_WAR;

/* サブゲーム(クライアント側)：戦場ゲーム情報 */
typedef struct _SUBGAMECLIENT_WARINFO {
	BYTE		byMove;												/* 移動残り歩数 */
} SUBGAMECLIENT_WARINFO, *PSUBGAMECLIENT_WARINFO;

/* サブゲーム：すごろく情報(イベント情報) */
typedef struct _SUGOROKUEVENTINFO {
	BYTE		byX;												/* X座標 */
	BYTE		byY;												/* Y座標 */
	BYTE		byEventType;										/* イベント種別 */
	BYTE		byPara1;											/* パラメータ */
} SUGOROKUEVENTINFO, *PSUGOROKUEVENTINFO;

/* サブゲーム：すごろく情報(パーツ情報) */
typedef struct _SUGOROKUPARTSINFO {
	BYTE		byType;												/* 種別 */
	WORD		wPartsID;											/* パーツID */
} SUGOROKUPARTSINFO, *PSUGOROKUPARTSINFO;

/* サブゲーム：すごろく情報(プレイヤー情報) */
typedef struct _SUGOROKUPLAYERINFO {
	BYTE		byDirection;										/* 向き */
	BYTE		byBack;												/* 逆走？ */
	BYTE		byNoCheck;											/* イベントチェック不要？ */
	BYTE		byWait;												/* 休み回数 */
	BYTE		byGoal;												/* ゴールした？ */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	DWORD		dwTblID;											/* テーブルID */
} SUGOROKUPLAYERINFO, *PSUGOROKUPLAYERINFO;

/* サブゲーム：すごろく情報 */
typedef struct _SUBGAME_SUGOROKUINFO {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX;												/* 参加受付X座標 */
	BYTE		byY;												/* 参加受付Y座標 */
	BYTE		byRule;												/* ルール */
	BYTE		byMaxCount;											/* 最大参加人数 */
	BYTE		byState;											/* 状態 */
	BYTE		byProcNo;											/* 処理中のキャラ番号 */
	BYTE		byLastMove;											/* 残り歩数 */
	DWORD		dwStartX;											/* スタート地点(X) */
	DWORD		dwStartY;											/* スタート地点(Y) */
	DWORD		dwGoalX;											/* ゴール地点(X) */
	DWORD		dwGoalY;											/* ゴール地点(Y) */
	DWORD		dwLastProc;											/* 最後に処理した時間 */
	DWORD		dwFirstTimeProc;									/* 最初に処理した時間 */
	DWORD		dwLastTimeProc;										/* 最後に処理した時間 */
	DWORD		dwNextTimeProc;										/* 次の処理待ち時間 */
	CmyArray<DWORD, DWORD>	adwPlayerTblID;							/* 参加者テーブルID */
	CmyArray<DWORD, DWORD>	adwViewerTblID;							/* 見学者テーブルID */
	CmyArray<SUGOROKUEVENTINFO, SUGOROKUEVENTINFO>	aEventInfo;		/* イベント情報 */
	CmyArray<SUGOROKUPARTSINFO, SUGOROKUPARTSINFO>	aPartsInfo;		/* パーツ情報 */
	CmyArray<SUGOROKUPLAYERINFO, SUGOROKUPLAYERINFO>	aPlayerInfo;	/* プレイヤー情報 */
} SUBGAME_SUGOROKUINFO, *PSUBGAME_SUGOROKUINFO;

/* サブゲーム：すごろく情報(保存データ) */
typedef struct _SUBGAME_SUGOROKUINFOSAVE {
	DWORD		dwMapID;											/* マップID */
	BYTE		byX;												/* 参加受付X座標 */
	BYTE		byY;												/* 参加受付Y座標 */
	BYTE		byRule;												/* ルール */
	BYTE		byMaxCount;											/* 最大参加人数 */
	CmyArray<SUGOROKUEVENTINFO, SUGOROKUEVENTINFO>	aEventInfo;		/* イベント情報 */
	CmyArray<SUGOROKUPARTSINFO, SUGOROKUPARTSINFO>	aPartsInfo;		/* パーツ情報 */
} SUBGAME_SUGOROKUINFOSAVE, *PSUBGAME_SUGOROKUINFOSAVE;

/* サブステータス情報(すごろく) */
typedef struct _SUBSTATUS_SUGOROKU {
	BYTE		byType;												/* サブゲーム種別 */
	BYTE		byID;												/* サブゲームID */
	BYTE		byState;											/* 状態 */
	BYTE		byReserve1;
	BYTE		byReserve2;
	BYTE		byFlgViewer			: 1;							/* 観客として参加 */
	BYTE		byFlgReserve		: 7;							/* 予約 */
} SUBSTATUS_SUGOROKU, *PSUBSTATUS_SUGOROKU;

/* 座標用構造体 */
typedef struct _XYINFO {
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
} XYINFO, *PXYINFO;

/* 吹き出しアニメーション情報 */
typedef struct _COMEFCANIMEINFO {
	BYTE		byWait;												/* 待ち時間(×１０ミリ秒) */
	BYTE		byGrpID;											/* グラフィックID */
} COMEFCANIMEINFO, *PCOMEFCANIMEINFO;

/* 吹き出し情報 */
typedef struct _COMEFCINFO {
	BYTE				byAnimeCount;								/* アニメーションコマ数 */
	BYTE				bySound;									/* 使った時に鳴らす音 */
	PCOMEFCANIMEINFO	pAnime;										/* アニメーション情報 */
} COMEFCINFO, *PCOMEFCINFO;

/* エフェクトアニメーション情報 */
typedef struct _EFCANIMEINFOOLD {
	BYTE		byWait;												/* 待ち時間(×１０ミリ秒) */
	WORD		wGrpID;												/* グラフィックID */
} EFCANIMEINFOOLD, *PEFCANIMEINFOOLD;

/* エフェクト情報 */
typedef struct _EFCINFOOLD {
	BYTE				byAnimeCount;								/* アニメーションコマ数 */
	BYTE				byLevel;									/* 透明度 */
	BYTE				bySound;									/* 効果音ID */
	PEFCANIMEINFOOLD	pAnime;										/* アニメーション情報 */
} EFCINFOOLD, *PEFCINFOOLD;

/* アニメーション情報 */
typedef struct _ANIMEINFO {
	BYTE		byWait;												/* 待ち時間(×１０ミリ秒) */
	BYTE		byLevel;											/* 透明度 */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wBackID;											/* 重ね合わせ時の背景ID */
} ANIMEINFO, *PANIMEINFO;

/* 文字エフェクト情報 */
typedef struct _CHAREFCINFO {
	COLORREF	color;												/* 表示色 */
	BYTE		byStage;											/* 処理番号 */
	char		chY;												/* 表示Y座標 */
	DWORD		dwLastProc;											/* 最後に処理した時間 */
	char		szMsg[MAX_LENCHAREFC];								/* 表示文字列 */
} CHAREFCINFO, *PCHAREFCINFO;

/* マップパーツ情報 */
typedef struct _MAPPARTSINFO {
	BYTE		byViewType;											/* 表示種別 */
	BYTE		byHitType;											/* 当たり判定種別 */
	BYTE		byAnimeType;										/* アニメーション種別 */
	BYTE		byAnimeNo;											/* アニメーションコマ番号 */
	BYTE		byAnimeCount;										/* アニメーションコマ数 */
	BYTE		byLevel;											/* 透明度 */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wBackID;											/* 重ね合わせ時の背景ID */
	DWORD		dwLastAnime;										/* 最後にアニメーションした時間 */
	ANIMEINFO	*pAnimeInfo;										/* アニメーション情報 */
} MAPPARTSINFO, *PMAPPARTSINFO;

/* マップパーツ情報保存用 */
typedef struct _MAPPARTSINFOSAVE {
	BYTE		byViewType;											/* 表示種別 */
	BYTE		byHitType;											/* 当たり判定種別 */
	BYTE		byAnimeType;										/* アニメーション種別 */
	BYTE		byAnimeCount;										/* アニメーションコマ数 */
	BYTE		byLevel;											/* 透明度 */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wBackID;											/* 重ね合わせ時の背景ID */
	ANIMEINFO	*pAnimeInfo;										/* アニメーション情報 */
} MAPPARTSINFOSAVE, *PMAPPARTSINFOSAVE;

/* イベント情報 */
typedef struct _EVENTINFO {
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	BYTE		byType;												/* 種別 */
	PVOID		pData;												/* データ */
} EVENTINFO, *PEVENTINFO;

/* アイテム配置情報 */
typedef struct _ITEMPOSINFO {
	DWORD		dwMapID;											/* マップID */
	DWORD		dwID;												/* アイテムID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
} ITEMPOSINFO, *PITEMPOSINFO;

/* アイテム情報 */
typedef struct _ITEMINFO {
	char		szName[MAX_ITEMNAME];								/* アイテム名 */
	BYTE		byType;												/* 種別 */
	BYTE		byData;												/* 付加データ */
	BYTE		byFlg;												/* フラグ */
	BYTE		bySound;											/* 使った時に鳴らす音 */
	BYTE		bySexDistinction;									/* 性別による種別 */
	BYTE		byEraseProbability;									/* 無くなる確率 */
	WORD		wGrpID;												/* グラフィックID */
	DWORD		dwGold;												/* 値段 */
	PVOID		pData;												/* 詳細情報 */
	LPSTR		pMsg;												/* 使った時のメッセージ */
} ITEMINFO, *PITEMINFO;

/* アイテム情報(指定座標へ移動) */
typedef struct _ITEMINFO_RURA {
	DWORD		dwEnableMapID;										/* 使用限定するマップID(先頭ビットはフラグ) */
	DWORD		dwMapID;											/* 移動先マップID */
	DWORD		dwX;												/* 移動先X座標 */
	DWORD		dwY;												/* 移動先Y座標 */
} ITEMINFO_RURA, *PITEMINFO_RURA;

/* アイテム情報(HP回復) */
typedef struct _ITEMINFO_HPUP {
	BYTE		byRate;												/* 確立で回復？ */
	WORD		wMin;												/* 最低回復値 */
	WORD		wMax;												/* 最大回復値 */
} ITEMINFO_HPUP, *PITEMINFO_HPUP;

/* アイテム情報(武器) */
typedef struct _ITEMINFO_WEAPON {
	BYTE		byEfcID;											/* 攻撃時に発動するエフェクト番号 */
	BYTE		byMin;												/* 攻撃力減少幅 */
	BYTE		byAtackType;										/* 攻撃種別 */
	BYTE		byData1;											/* 付加データ1 */
	BYTE		byData2;											/* 付加データ2 */
	BYTE		byData3;											/* 付加データ3 */
	BYTE		byData4;											/* 付加データ4 */
	BYTE		byData5;											/* 付加データ5 */
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_WEAPON, *PITEMINFO_WEAPON;

/* アイテム情報(鎧) */
typedef struct _ITEMINFO_ARMOR {
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_ARMOR, *PITEMINFO_ARMOR;

/* アイテム情報(盾) */
typedef struct _ITEMINFO_SHILD {
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_SHILD, *PITEMINFO_SHILD;

/* アイテム情報(兜) */
typedef struct _ITEMINFO_HELMET {
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_HELMET, *PITEMINFO_HELMET;

/* アイテム情報(種) */
typedef struct _ITEMINFO_SEED {
	BYTE		byType;												/* 種類 */
	WORD		wMax;												/* 上限値 */
} ITEMINFO_SEED, *PITEMINFO_SEED;

/* アイテム情報(サブゲーム参加) */
typedef struct _ITEMINFO_SUBGAME {
	BYTE		byType;												/* 種類 */
} ITEMINFO_SUBGAME, *PITEMINFO_SUBGAME;

/* アイテム情報(酒) */
typedef struct _ITEMINFO_SAKE {
	char		chPoint;											/* 度数 */
} ITEMINFO_SAKE, *PITEMINFO_SAKE;

/* アイテム情報(装備(小手)) */
typedef struct _ITEMINFO_EQU_HAND {
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_EQU_HAND, *PITEMINFO_EQU_HAND;

/* アイテム情報(装備(靴)) */
typedef struct _ITEMINFO_EQU_LEG {
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_EQU_LEG, *PITEMINFO_EQU_LEG;

/* アイテム情報(装備(首飾り)) */
typedef struct _ITEMINFO_EQU_ACC1 {
	BYTE		byEquType;											/* 装備効果 */
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_EQU_ACC1, *PITEMINFO_EQU_ACC1;

/* アイテム情報(装備(指輪1)) */
typedef struct _ITEMINFO_EQU_ACC2 {
	BYTE		byEquType;											/* 装備効果 */
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_EQU_ACC2, *PITEMINFO_EQU_ACC2;

/* アイテム情報(装備(指輪2)) */
typedef struct _ITEMINFO_EQU_ACC3 {
	BYTE		byEquType;											/* 装備効果 */
	WORD		wEqu;												/* 装備できるキャラ情報 */
} ITEMINFO_EQU_ACC3, *PITEMINFO_EQU_ACC3;

/* アイテム情報(メッセージ表示) */
typedef struct _ITEMINFO_MSG {
	BYTE		byType;												/* 表示種別 */
	BYTE		byEfectID;											/* エフェクト番号 */
	char		szCharEfc[MAX_LENCHAREFC];							/* 文字エフェクト */
	LPSTR		pMsgTo;												/* 他のキャラに使った時のメッセージ */
	LPSTR		pMsgMe;												/* 他のキャラに使われた時のメッセージ */
} ITEMINFO_MSG, *PITEMINFO_MSG;

/* アイテム情報(キャラ画像変更) */
typedef struct _ITEMINFO_GRPCHG {
	WORD		wGrpID;												/* 変更するグラフィックID（０は記録した画像） */
} ITEMINFO_GRPCHG, *PITEMINFO_GRPCHG;

/* アイテム情報(明かり) */
typedef struct _ITEMINFO_LIGHT {
	BYTE		byLevel;											/* 明るさレベル */
	WORD		wTime;												/* 維持時間 */
} ITEMINFO_LIGHT, *PITEMINFO_LIGHT;

/* アイテム情報(HP減少) */
typedef struct _ITEMINFO_HPDOWN {
	BYTE		byEfectID;											/* エフェクト番号 */
	BYTE		byRate;												/* 確立で減少？ */
	WORD		wMin;												/* 最低減少値 */
	WORD		wMax;												/* 最大減少値 */
} ITEMINFO_HPDOWN, *PITEMINFO_HPDOWN;

/* アイテム情報(魔法習得) */
typedef struct _ITEMINFO_MAGIC {
	BYTE		byID;												/* 習得する魔法 */
} ITEMINFO_MAGIC, *PITEMINFO_MAGIC;

/* アイテム情報(Mana回復) */
typedef struct _ITEMINFO_MANAUP {
	BYTE		byRate;												/* 確立で減少？ */
	WORD		wMin;												/* 最低回復値 */
	WORD		wMax;												/* 最大回復値 */
} ITEMINFO_MANAUP, *PITEMINFO_MANAUP;

/* アイテム情報(HP・Mana回復) */
typedef struct _ITEMINFO_HPMANAUP {
	BYTE		byRateHP;											/* 確立で減少？(HP) */
	BYTE		byRateMana;											/* 確立で減少？(Mana) */
	WORD		wMinHP;												/* 最低回復値(HP) */
	WORD		wMaxHP;												/* 最大回復値(HP) */
	WORD		wMinMana;											/* 最低回復値(Mana) */
	WORD		wMaxMana;											/* 最大回復値(Mana) */
} ITEMINFO_HPMANAUP, *PITEMINFO_HPMANAUP;

/* アイテム情報(蘇生) */
typedef struct _ITEMINFO_RESURRECTION {
	BYTE		byRate;												/* 回復率 */
} ITEMINFO_RESURRECTION, *PITEMINFO_RESURRECTION;

/* 装備情報 */
typedef struct _EQUUSERINFO {
	char		szName[MAX_EQUNAME];								/* 名前 */
	BYTE		byCount;											/* 保存している職業数 */
	PBYTE		paJobIDTbl;											/* 職業一覧 */
} EQUUSERINFO, *PEQUUSERINFO;

/* 魔法利用者種別情報 */
typedef struct _MAGICUSERINFO {
	char		szName[MAX_EQUNAME];								/* 名前 */
	BYTE		byCount;											/* 保存している職業数 */
	PBYTE		paJobIDTbl;											/* 職業一覧 */
} MAGICUSERINFO, *PMAGICUSERINFO;

/* 魔法情報 */
typedef struct _MAGICINFO {
	char		szName[MAX_MAGICNAME];								/* 名前 */
	BYTE		byType;												/* 効果種別 */
	BYTE		byTypeInfoID;										/* 種別情報ID */
	BYTE		byUserInfo;											/* 利用情報 */
	BYTE		byTarget;											/* 使用対象 */
	BYTE		byTargetType;										/* 発動種別 */
	BYTE		byTargetArea;										/* 発動範囲 */
	BYTE		byTargetCount;										/* 発動人数 */
	BYTE		byTargetEfectID;									/* エフェクト番号（相手） */
	BYTE		byUserEfectID;										/* エフェクト番号（利用者） */
	BYTE		byTime;												/* 詠唱時間(1で1秒10で10秒) */
	BYTE		byAttr;												/* 属性 */
	WORD		wData1;												/* 付加データ */
	WORD		wData2;												/* 付加データ */
	WORD		wData3;												/* 付加データ */
	WORD		wData4;												/* 付加データ */
	WORD		wMana;												/* 消費Mana */
	char		szExplain[MAX_MAGICEXPLAIN];						/* 説明文（空なら数値から自動で作る） */
} MAGICINFO, *PMAGICINFO;

/* 店情報 */
typedef struct _SHOPINFO {
	BYTE		byCountItem;										/* 売り物数 */
	WORD		awItem[MAX_SHOPITEM];								/* 売り物 */
	LPSTR		apMsg[MAX_SHOPMSG];									/* メッセージへのポインタテーブル */
	DWORD		dwItemCRC;											/* 売り物用チェックサム */
	DWORD		dwMsgCRC[MAX_SHOPMSG];								/* メッセージ用チェックサム */
} SHOPINFO, *PSHOPINFO;

/* オブジェクト情報 */
typedef struct _OBJINFO {
	BYTE		byTypeMain;											/* 種別（メイン） */
	BYTE		byTypeSub;											/* 種別（サブ） */
	BYTE		byX;												/* X座標 */
	BYTE		byY;												/* Y座標 */
	WORD		wGrpID;												/* グラフィックID */
	BYTE		byHitType;											/* 当たり判定種別 */
	BYTE		byAnimeType;										/* アニメーション種別 */
	BYTE		byAnimeNo;											/* アニメーションコマ番号 */
	BYTE		byAnimeCount;										/* アニメーションコマ数 */
	BYTE		byLevel;											/* 透明度 */
	BYTE		byData[8];											/* 汎用データ */
	DWORD		dwLastAnime;										/* 最後にアニメーションした時間 */
	ANIMEINFO	*pAnimeInfo;										/* アニメーション情報 */
} OBJINFO, *POBJINFO;

/* マップ情報 */
typedef struct _MAPINFO {
	DWORD			dwID;											/* マップID */
	int				nCX;											/* 横サイズ */
	int				nCY;											/* 縦サイズ */
	int				nStartX;										/* 初期位置(X) */
	int				nStartY;										/* 初期位置(Y) */
	int				nEntranceX;										/* 入り口(X) */
	int				nEntranceY;										/* 入り口(Y) */
	BYTE			byBGM;											/* 音楽ID */
	BYTE			byCountShop;									/* 店の数 */
	BYTE			byFlg;											/* 各フラグ */
	BYTE			byLightLevel;									/* 明るさレベル */
	BYTE			byWeather;										/* 天候 */
	char			*pszName;										/* マップ名 */
	BYTE			*pbyRoofLV;										/* 屋根レベル */
	BYTE			*pbyFlg;										/* 各フラグ情報 */
	WORD			*pwMap;											/* マップパーツ */
	DWORD			*pdwMapChar;									/* キャラの配置状況 */
	int				nEventCount;									/* 登録イベント数 */
	int				nItemCount;										/* 登録アイテム数 */
	int				nMsgCount;										/* 登録メッセージ数 */
	int				nProcEvent;										/* イベント処理ID */
	PEVENTINFO		pEvent;											/* イベント情報 */
	PITEMPOSINFO	pItem;											/* アイテム配置情報 */
	LPSTR			*pMsg;											/* メッセージ情報 */
	DWORD			*paTblNoPC;										/* PC番号テーブル */
	DWORD			*paTblNoNPC;									/* NPC番号テーブル */
	DWORD			dwCountTblNoPC;									/* PC番号テーブル数 */
	DWORD			dwCountTblNoNPC;								/* NPC番号テーブル数 */
	PSHOPINFO		pShopInfo;										/* 店情報 */
	COLORREF		clChg;											/* 色変換 */
} MAPINFO, *PMAPINFO;

/* マップ情報保存用 */
typedef struct _MAPINFOSAVE {
	DWORD			dwID;											/* マップID */
	int				nCX;											/* 横サイズ */
	int				nCY;											/* 縦サイズ */
	int				nStartX;										/* 初期位置(X) */
	int				nStartY;										/* 初期位置(Y) */
	BYTE			byBGM;											/* 音楽ID */
	BYTE			byCountShop;									/* 店の数 */
	BYTE			byFlg;											/* 各フラグ */
	char			*pszName;										/* マップ名 */
	BYTE			*pbyRoofLV;										/* 屋根レベル */
	WORD			*pwMap;											/* マップパーツ */
	int				nEventCount;									/* 登録イベント数 */
	int				nMsgCount;										/* 登録メッセージ数 */
	COLORREF		clChg;											/* 色変換 */
} MAPINFOSAVE, *PMAPINFOSAVE;

/* キャラ情報 */
typedef struct _CHARINFO {
	DWORD		pID;												/* プレイヤーID */
	BYTE		byAdminLV;											/* 管理権限レベル */
	BYTE		byGameState;										/* ゲーム状態 */
	BYTE		byCharState;										/* キャラ状態 */
	BYTE		byAccState;											/* アカウント状態 */
	BYTE		byDirection;										/* 向き */
	BYTE		byDirectionBack;									/* 向き */
	BYTE		byDirectionBack2;									/* 向き */
	BYTE		byRoofLV;											/* 屋根レベル */
	BYTE		byEfect;											/* エフェクト状態 */
	BYTE		byEfectIdle;										/* 常に表示するエフェクト */
	BYTE		byEfcAnime;											/* エフェクトアニメ番号 */
	BYTE		byDotMove;											/* ドット移動量 */
	BYTE		byAnime;											/* アニメーション番号 */
	BYTE		byNpcType;											/* NPC移動種別 */
	BYTE		byFlg;												/* いろいろフラグ */
	BYTE		bySpeechFlg;										/* 発言種別 */
	BYTE		byFadeLevel;										/* 明度 */
	BYTE		byComEfc;											/* 吹き出し種別 */
	BYTE		byComEfcAnime;										/* 吹き出しアニメ番号 */
	BYTE		byAreaX;											/* 移動範囲（横） */
	BYTE		byAreaY;											/* 移動範囲（縦） */
	BYTE		byPutCount;											/* 最大NPC発生数 */
	BYTE		byPutCountNow;										/* 現在のNPC発生数 */
	BYTE		byCharEfcProc;										/* 処理中の文字エフェクト番号 */
	BYTE		byDrunkPoint;										/* 酔いポイント */
	BYTE		byDrunkLV;											/* 酔いレベル */
	BYTE		byDropProbability;									/* ドロップ率 */
	BYTE		byTurnLV;											/* ターンゲージ(TURNGAGE_LAP ごとに1周) */
	BYTE		byTurnMagicLV;										/* 魔法ターンゲージ */
	BYTE		byMoveProbabilityX;									/* 移動確率X(NPC) */
	BYTE		byMoveProbabilityY;									/* 移動確率Y(NPC) */
	BYTE		bySearchCx;											/* 検索範囲X */
	BYTE		bySearchCy;											/* 検索範囲Y */
	BYTE		byLightLevel;										/* 明るさレベル */
	BYTE		byLightLevelBack;									/* 明るさレベル（最初） */
	BYTE		byAtackRate;										/* 攻撃力の割合 */

	BYTE		abySubStatus[MAX_SUBSTATUS];						/* 保存しないサブステータス */
	BYTE		bySaveFlg;											/* 保存用フラグ */
	BYTE		byPW;												/* 力 */
	BYTE		byDEX;												/* 素早さ */
	BYTE		byST;												/* 体力 */
	BYTE		byIT;												/* 賢さ */
	BYTE		byLK;												/* 運のよさ */
	BYTE		byJOB;												/* 職業 */
	BYTE		byLevel;											/* レベル */
	BYTE		abyMagic[MAX_USEMAGIC];								/* 呪文 */
	BYTE		abyJobMagic[MAX_JOBMAGIC][MAX_USEMAGIC];			/* 転職前の職業で覚えていた呪文（職業ID別） */
	BYTE		byOptBgm;											/* 音楽ON/OFFのアカウント設定(OPTBGM_*) */
	BYTE		byRouteCount;										/* 巡回ルートの道しるべ数(2以上で巡回する) */
	BYTE		byRouteMode;										/* 巡回ルートの回り方(NPCROUTEMODE_*) */
	BYTE		byRouteWait;										/* 巡回ルートの各道しるべで止まる秒数 */
	BYTE		abyRouteX[MAX_NPCROUTE];							/* 巡回ルートの道しるべ(横) */
	BYTE		abyRouteY[MAX_NPCROUTE];							/* 巡回ルートの道しるべ(縦) */
	char		chAttrFire;											/* 属性(火) */
	char		chAttrWater;										/* 属性(水) */
	char		chAttrWind;											/* 属性(風) */
	char		chAttrEarth;										/* 属性(土) */
	char		chAttrLight;										/* 属性(光) */
	char		chAttrDark;											/* 属性(闇) */
	char		chTimerPW;											/* タイマー補正(力) */
	char		chTimerST;											/* タイマー補正(体力) */
	char		chTimerDEX;											/* タイマー補正(素早さ) */
	WORD		wAT;												/* 攻撃力 */
	WORD		wDF;												/* 守備力 */
	WORD		wDFMagic;											/* 魔法防御 */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wHP;												/* HP */
	WORD		wMaxMana;											/* 最大Mana */
	WORD		wMana;												/* Mana */
	WORD		waEqu[MAX_EQUTYPE];									/* 装備品 */

	WORD		wSearchTmp;											/* キャラ検索用テンポラリ */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wGrpIDBack;											/* グラフィックID記録用 */
	WORD		wGrpMain;											/* グラフィック種別(メイン) */
	WORD		wGrpSub;											/* グラフィック種別(サブ) */
	WORD		wGrpTmpID;											/* 一時的グラフィックID */
	WORD		wGrpTmpMain;										/* 一時的グラフィック種別(メイン) */
	WORD		wGrpTmpSub;											/* 一時的グラフィック種別(サブ) */
	WORD		wMakeYear;											/* キャラ作成年 */
	WORD		wMakeMonth;											/* キャラ作成月 */
	WORD		wMakeDay;											/* キャラ作成日 */
	WORD		wTimePutInterval;									/* 発生周期 */
	WORD		wLightTime;											/* 明かり維持時間 */
	WORD		waItem[MAX_MYITEM];									/* 所持アイテム */
	DWORD		dwTargetTblID;										/* ターゲット先のテーブルID */
	DWORD		dwTargetTblID2;										/* ターゲット先のテーブルID */
	DWORD		dwTargetTblID3;										/* ターゲット先のテーブルID */
	DWORD		dwTailTblID;										/* ついていっているキャラのテーブルID */
	DWORD		dwTailBackTblID;									/* ついてきているキャラのテーブルID */
	DWORD		dwAtackTblID;										/* 攻撃するキャラのテーブルID */
	DWORD		dwGold;												/* 所持金 */
	DWORD		dwLeaveID;											/* 預かり所ID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwParentTblID;										/* 親のテーブルID */
	DWORD		dwStTmplateID;										/* ステータステンプレートID */
	DWORD		dwPartyID;											/* 仲間情報ID */
	DWORD		dwMapID;											/* マップID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	DWORD		dwXBack;											/* X座標 */
	DWORD		dwYBack;											/* Y座標 */
	DWORD		dwStartMapID;										/* 開始マップID */
	DWORD		dwStartX;											/* 開始X座標 */
	DWORD		dwStartY;											/* 開始Y座標 */
	DWORD		dwLastDotMove;										/* 最後に移動した時間 */
	DWORD		dwLastAnime;										/* 最後にアニメーションした時間 */
	DWORD		dwLastHartBeat;										/* 最後に生存確認を受けた時間 */
	DWORD		dwLastSpeech;										/* 最後に発言した時間 */
	DWORD		dwLastPut;											/* 最後に発生させた時間 */
	DWORD		dwLastBattle;										/* 最後に戦闘関連で行動した時間 */
	DWORD		dwLastSake;											/* 最後に酒を飲んだ時間 */
	DWORD		dwLastSakeHikku;									/* 最後の「ﾋｯｸ」な時間 */
	DWORD		dwLastTurnLV;										/* 最後にターンゲージを動かした時間 */
	DWORD		dwLastTurnMagicLV;									/* 最後に魔法ターンゲージを動かした時間 */
	DWORD		dwTimeDelete;										/* 削除開始時間 */
	DWORD		dwTimeAdd;											/* 表示開始時間 */
	DWORD		dwTimeEfc;											/* エフェクト開始時間 */
	DWORD		dwTimeEfcAnime;										/* 最後にエフェクトアニメした時間 */
	DWORD		dwTimeComEfc;										/* 吹き出し表示開始時間 */
	DWORD		dwTimeComEfcAnime;									/* 最後に吹き出しアニメした時間 */
	DWORD		dwTimeTurnLV;										/* ターンゲージ開始時間 */
	DWORD		dwTimeTurnMagicLV;									/* 魔法ターンゲージ開始時間 */
	DWORD		dwTimeLight;										/* 明かりつけた時間 */
	DWORD		dwLoginCount;										/* ログイン数 */
	DWORD		dwTimeLogin;										/* ログイン時刻 */
	DWORD		dwPlayTime;											/* 総プレイ時間 */
	DWORD		dwLastIPAddress;									/* 最終IPアドレス */
	DWORD		dwSpeechCRC;										/* 発言内容のチェックサム */
	DWORD		dwExp;												/* 経験値 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	char		szPas[MAX_ACCNAME];									/* パスワード */
	char		szMacAdr[6];										/* MACアドレス */
	LPSTR		pszSpeech;											/* 発言 */
	LPSTR		pszNpcSpeech;										/* NPC発言 */
	COLORREF	clName;												/* 名前の色 */
	COLORREF	clSpeech;											/* 発言の色 */
	CHAREFCINFO	aCharEfcInfo[MAX_COUNTCHAREFC];						/* 文字エフェクト */
	CmyArray<PCHARINFOTIMEREVENT, PCHARINFOTIMEREVENT> aTimerEvent;	/* タイマーイベント */
	PVOID		pProc;												/* 処理 */
} CHARINFO, *PCHARINFO;

/* キャラ情報通知 */
typedef struct _CMDPARA_CHARINFO {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byAdminLV;											/* 管理権限レベル */
	BYTE		byCharState;										/* キャラ状態 */
	BYTE		byAccState;											/* アカウント状態 */
	BYTE		byDirection;										/* 向き */
	BYTE		byRoofLV;											/* 屋根レベル */
	BYTE		byEfect;											/* エフェクト状態 */
	BYTE		byEfectIdle;										/* 常に表示するエフェクト */
	BYTE		byNpcType;											/* NPC移動種別 */
	BYTE		byAreaX;											/* 移動範囲（横） */
	BYTE		byAreaY;											/* 移動範囲（縦） */
	BYTE		byPutCount;											/* 最大NPC発生数 */
	BYTE		byDropProbability;									/* ドロップ率 */
	BYTE		byMoveProbabilityX;									/* 移動確率X(NPC) */
	BYTE		byMoveProbabilityY;									/* 移動確率Y(NPC) */
	BYTE		bySearchCx;											/* 検索範囲X */
	BYTE		bySearchCy;											/* 検索範囲Y */
	BYTE		byLightLevel;										/* 明るさレベル */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wGrpMain;											/* グラフィック種別(メイン) */
	WORD		wGrpSub;											/* グラフィック種別(サブ) */
	WORD		wGrpTmpID;											/* 一時的グラフィックID */
	WORD		wGrpTmpMain;										/* 一時的グラフィック種別(メイン) */
	WORD		wGrpTmpSub;											/* 一時的グラフィック種別(サブ) */
	WORD		wTimePutInterval;									/* 発生周期 */
	WORD		wMakeYear;											/* キャラ作成年 */
	WORD		wMakeMonth;											/* キャラ作成月 */
	WORD		wMakeDay;											/* キャラ作成日 */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wHP;												/* HP */
	WORD		wMaxMana;											/* 最大Mana */
	WORD		wMana;												/* Mana */
	DWORD		dwMapID;											/* マップID */
	DWORD		dwStartMapID;										/* 開始マップID */
	DWORD		dwStartX;											/* 開始X座標 */
	DWORD		dwStartY;											/* 開始Y座標 */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	DWORD		dwLoginCount;										/* ログイン数 */
	DWORD		dwPlayTime;											/* 総プレイ時間 */
	DWORD		dwStTmplateID;										/* ステータステンプレートID */
	DWORD		dwLastIPAddress;									/* 最終IPアドレス */
	DWORD		dwTailTblID;										/* ついていっているキャラのテーブルID */
	DWORD		dwTailBackTblID;									/* ついてきているキャラのテーブルID */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	char		szPas[MAX_ACCNAME];									/* パスワード */
	char		szMacAdr[6];										/* MACアドレス */
	LPSTR		pszNpcSpeech;										/* NPC発言 */
	COLORREF	clName;												/* 名前の色 */
	COLORREF	clSpeech;											/* 発言の色 */
} CMDPARA_CHARINFO, *PCMDPARA_CHARINFO;

/* キャラ情報通知2 */
typedef struct _CMDPARA_CHARINFO2 {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwMapID;											/* マップID */
	BYTE		byAdminLV;											/* 管理権限レベル */
	BYTE		byCharState;										/* キャラ状態 */
	BYTE		byDirection;										/* 向き */
	BYTE		byRoofLV;											/* 屋根レベル */
	BYTE		byEfect;											/* エフェクト状態 */
	BYTE		byEfectIdle;										/* 常に表示するエフェクト */
	BYTE		byNpcType;											/* NPC移動種別 */
	BYTE		byLightLevel;										/* 明るさレベル */
	WORD		wGrpID;												/* グラフィックID */
	WORD		wGrpMain;											/* グラフィック種別(メイン) */
	WORD		wGrpSub;											/* グラフィック種別(サブ) */
	WORD		wGrpTmpID;											/* 一時的グラフィックID */
	WORD		wGrpTmpMain;										/* 一時的グラフィック種別(メイン) */
	WORD		wGrpTmpSub;											/* 一時的グラフィック種別(サブ) */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wHP;												/* HP */
	WORD		wMaxMana;											/* 最大Mana */
	WORD		wMana;												/* Mana */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	LPSTR		pszNpcSpeech;										/* NPC発言 */
	COLORREF	clName;												/* 名前の色 */
	COLORREF	clSpeech;											/* 発言の色 */
} CMDPARA_CHARINFO2, *PCMDPARA_CHARINFO2;

/* 預かり所情報 */
typedef struct _LEAVEINFO {
	BYTE		byUse;												/* １なら使用中 */
	BYTE		byCount;											/* アイテム数 */
	BYTE		byItemCount[MAX_LEAVEITEM];							/* アイテム個数 */
	WORD		waItem[MAX_LEAVEITEM];								/* アイテム */
	DWORD		dwGold;												/* お金 */
} LEAVEINFO, *PLEAVEINFO;

typedef struct _LEAVEINFO2 {
	BYTE		byUse;												/* １なら使用中 */
	BYTE		byCount;											/* アイテム数 */
	WORD		waItem[MAX_LEAVEITEM];								/* アイテム */
	DWORD		dwGold;												/* お金 */
} LEAVEINFO2, *PLEAVEINFO2;

/* 預けてるアイテム取得 */
typedef struct _CMDPARA_LEAVEITEMGET {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCount;											/* アイテム数 */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byItemCount[MAX_LEAVEITEM];							/* アイテム個数 */
	WORD		waItem[MAX_LEAVEITEM];								/* アイテム */
} CMDPARA_LEAVEITEMGET, *PCMDPARA_LEAVEITEMGET;

/* 預かり所情報通知 */
typedef struct _CMDPARA_LEAVEINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCount;											/* アイテム数 */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byItemCount[MAX_LEAVEITEM];							/* アイテム個数 */
	WORD		waItem[MAX_LEAVEITEM];								/* アイテム */
	DWORD		dwGold;												/* お金 */
} CMDPARA_LEAVEINFO, *PCMDPARA_LEAVEINFO;

/* ステータステンプレート情報 */
typedef struct _STATUSTEMPLATEINFO {
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	BYTE		byLevel;											/* レベル */
	BYTE		byPW;												/* 力 */
	BYTE		byDEX;												/* 素早さ */
	BYTE		byST;												/* 体力 */
	BYTE		byIT;												/* 賢さ */
	BYTE		byLK;												/* 運のよさ */
	BYTE		byNpcType;											/* 職業 */
	BYTE		byDropProbability;									/* ドロップ率 */
	BYTE		byMoveProbabilityX;									/* 移動確率X */
	BYTE		byMoveProbabilityY;									/* 移動確率Y */
	BYTE		bySearchCx;											/* 検索範囲X */
	BYTE		bySearchCy;											/* 検索範囲Y */
	BYTE		abyMagic[MAX_USEMAGIC];								/* 呪文 */
	char		chAttrFire;											/* 属性(火) */
	char		chAttrWater;										/* 属性(水) */
	char		chAttrWind;											/* 属性(風) */
	char		chAttrEarth;										/* 属性(土) */
	char		chAttrLight;										/* 属性(光) */
	char		chAttrDark;											/* 属性(闇) */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wMaxMana;											/* 最大Mana */
	WORD		wDFMagic;											/* 魔法防御 */
	WORD		waItem[MAX_MYITEM];									/* 所持アイテム */
	DWORD		dwGold;												/* 所持金 */
	DWORD		dwExp;												/* 経験値 */
	BYTE		byStatusResist;										/* 毒・眠りの効きにくさ(%、100で効かない) */
} STATUSTEMPLATEINFO, *PSTATUSTEMPLATEINFO;

/* キャラ追加通知 */
typedef struct _CMDPARA_ADDCHARINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byDirection;										/* 向き */
	BYTE		byNpcType;											/* NPC移動種別 */
	WORD		wGrpID;												/* グラフィックID */
	DWORD		dwMapID;											/* マップID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
	LPSTR		pszNpcSpeech;										/* NPC発言 */
} CMDPARA_ADDCHARINFO, *PCMDPARA_ADDCHARINFO;

/* イベントデータ(移動) */
typedef struct _EVENTINFO_MOVE {
	DWORD		dwX;												/* 移動先X座標 */
	DWORD		dwY;												/* 移動先Y座標 */
	BYTE		byEfect;											/* エフェクト種類 */
} EVENTINFO_MOVE, *PEVENTINFO_MOVE;

/* イベントデータ(マップ間移動) */
typedef struct _EVENTINFO_MAPCHG {
	DWORD		dwX;												/* 移動先X座標 */
	DWORD		dwY;												/* 移動先Y座標 */
	DWORD		dwMapID;											/* マップID */
	BYTE		byEfect;											/* エフェクト種類 */
} EVENTINFO_MAPCHG, *PEVENTINFO_MAPCHG;

/* イベントデータ(メッセージ) */
typedef struct _EVENTINFO_MSG {
	DWORD		dwNo;												/* メッセージID */
	BYTE		byType;												/* 0:自動 1:調べる 2:話すと同じ */
	WORD		wGrpID;												/* 書き換えるグラフィックID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
} EVENTINFO_MSG, *PEVENTINFO_MSG;

/* イベントデータ(扉) */
typedef struct _EVENTINFO_DOOR {
	DWORD		dwTimeOpen;											/* 開けた時間 */
	WORD		wGrpID;												/* 扉のグラフィックID */
	WORD		wBackGrpID;											/* 背景のグラフィックID */
	BYTE		byTimeClose;										/* 閉まるまでの時間(100ms) */
	BYTE		byType;												/* 種別 */
} EVENTINFO_DOOR, *PEVENTINFO_DOOR;

/* イベントデータ(HP/Mana回復) */
typedef struct _EVENTINFO_HPMANA {
	BYTE		byType;												/* 種別 */
} EVENTINFO_HPMANA, *PEVENTINFO_HPMANA;

/* イベントデータ(キャラが乗っている間マップ変更) */
typedef struct _EVENTINFO_ONCHARMAP {
	WORD		wGrpIDOn;											/* グラフィックID（キャラが乗っている） */
	WORD		wGrpIDOff;											/* グラフィックID（キャラが乗っていない） */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	DWORD		dwCheckTime;										/* 最後にチェックした時間 */
} EVENTINFO_ONCHARMAP, *PEVENTINFO_ONCHARMAP;

/* 仲間情報 */
typedef struct _PARTYINFO {
	BYTE		byGold;												/* お金分配方法 */
	DWORD		adwTblID[MAX_PARTYMEMBER];							/* パーティメンバー */
} PARTYINFO, *PPARTYINFO;

/* 汎用データ１ */
typedef struct _CMDPARA_DATA1 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byData;												/* データ */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
} CMDPARA_DATA1, *PCMDPARA_DATA1;

/* 汎用データ２ */
typedef struct _CMDPARA_DATA2 {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	int			nData;												/* データ */
} CMDPARA_DATA2, *PCMDPARA_DATA2;

/* 汎用データ３ */
typedef struct _CMDPARA_DATA3 {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwData;												/* データ */
} CMDPARA_DATA3, *PCMDPARA_DATA3;

/* 汎用データ４ */
typedef struct _CMDPARA_DATA4 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byData;												/* データ */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwData;												/* データ */
} CMDPARA_DATA4, *PCMDPARA_DATA4;

/* 汎用データ５ */
typedef struct _CMDPARA_DATA5 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byData;												/* データ */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwData1;											/* データ１ */
	DWORD		dwData2;											/* データ２ */
} CMDPARA_DATA5, *PCMDPARA_DATA5;

/* 汎用データ６ */
typedef struct _CMDPARA_DATA6 {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwData1;											/* データ１ */
	DWORD		dwData2;											/* データ２ */
} CMDPARA_DATA6, *PCMDPARA_DATA6;

/* ログイン要求 */
typedef struct _CMDPARA_LOGINREQ {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	char		szPas[MAX_ACCNAME];									/* パスワード */
	char		szMacAdr[6];										/* MACアドレス */
} CMDPARA_LOGINREQ, *PCMDPARA_LOGINREQ;

/* ブラウザ版のアカウント関係（CMD_WEBACCOUNT の byCmdSub） */
enum {
	WEBACC_TOKENLOGIN = 0,										/* 端末トークンでログイン要求（クライアント→サーバー） */
	WEBACC_TOKENREQ,											/* 端末トークン発行要求（クライアント→サーバー） */
	WEBACC_TOKEN,												/* 端末トークン通知（サーバー→クライアント） */
	WEBACC_OPTBGM,												/* 音楽ON/OFFのアカウント設定（両方向。byData に OPTBGM_*） */
};

/* 音楽ON/OFFのアカウント設定（CHARINFO の byOptBgm） */
enum {
	OPTBGM_NONE = 0,											/* まだ決めていない（端末の設定に従う） */
	OPTBGM_ON,													/* 音楽を鳴らす */
	OPTBGM_OFF,													/* 音楽を止める */
};

#define MAX_WEBTOKEN		64										/* 端末トークンの文字数（16進） */

/* ブラウザ版のアカウント関係 */
typedef struct _CMDPARA_WEBACCOUNT {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCmdSub;											/* サブコマンド(WEBACC_*) */
	DWORD		pID;												/* プレイヤーID */
	char		szToken[MAX_WEBTOKEN + 1];							/* 端末トークン */
	char		szMacAdr[6];										/* MACアドレス */
	BYTE		byData;												/* サブコマンドごとのデータ（WEBACC_OPTBGM の OPTBGM_*） */
} CMDPARA_WEBACCOUNT, *PCMDPARA_WEBACCOUNT;

/* 管理者画面の「元に戻す」（CMD_ADMINUNDO の byCmdSub） */
enum {
	ADMINUNDO_SCREEN = 0,										/* いま開いている画面の通知（クライアント→サーバー） */
	ADMINUNDO_INFOREQ,											/* 戻せる変更の問い合わせ（クライアント→サーバー） */
	ADMINUNDO_INFO,												/* 戻せる変更の通知（サーバー→クライアント） */
	ADMINUNDO_EXEC,												/* 元に戻す（クライアント→サーバー） */
	ADMINUNDO_RESULT,											/* 元に戻した結果（サーバー→クライアント） */
};

/* 「元に戻す」の単位になる管理者画面（ADMINSCR_*）。変更はその時に開いていた画面で記録する */
enum {
	ADMINSCR_NONE = 0,											/* 画面なし・対応していない画面 */
	ADMINSCR_MAPPARTS,											/* パーツ（マップの塗り替え） */
	ADMINSCR_MAPINFO,											/* マップ情報 */
	ADMINSCR_EVENT,												/* イベント情報 */
	ADMINSCR_MSG,												/* メッセージ */
	ADMINSCR_SHOP,												/* 店情報 */
	ADMINSCR_CHARSTATUS,										/* ステータス情報 */
	ADMINSCR_CHARITEM,											/* 所持アイテム情報 */
	ADMINSCR_CHARMAGIC,											/* 所持魔法情報 */
	ADMINSCR_STTMP,												/* ステータステンプレート情報 */
	ADMINSCR_ITEM,												/* アイテム一覧 */
	ADMINSCR_CEF,												/* ふきだし */
	ADMINSCR_EFC,												/* 効果いろいろ */
	ADMINSCR_JOBINFO,											/* 職業情報（レベルアップ情報も） */
	ADMINSCR_MAGICINFO,											/* 魔法情報 */
	ADMINSCR_MAGICUSERINFO,										/* 魔法利用者情報 */
	ADMINSCR_EQUUSERINFO,										/* 装備職業情報 */
	ADMINSCR_MAX
};

#define MAX_ADMINUNDOMSG	400										/* 「元に戻す」の案内文の大きさ（UTF-8） */

#pragma pack(push, 1)
/* 管理者画面の「元に戻す」 */
typedef struct _CMDPARA_ADMINUNDO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCmdSub;											/* サブコマンド(ADMINUNDO_*) */
	BYTE		byScreen;											/* 画面(ADMINSCR_*) */
	BYTE		byResult;											/* 結果（RESULT: 1=戻した 0=戻せなかった） */
	WORD		wCount;												/* 戻せる回数（INFO・RESULT） */
	DWORD		dwStepID;											/* 戻す変更の番号（INFO で知らせ、EXEC で指定） */
	char		szMsg[MAX_ADMINUNDOMSG];							/* 案内文（サーバー→クライアント） */
} CMDPARA_ADMINUNDO, *PCMDPARA_ADMINUNDO;
#pragma pack(pop)

/* マップ情報通知 */
typedef struct _CMDPARA_MAPINFO {
	BYTE		byCmd;												/* コマンドID */
	MAPINFO		MapInfo;											/* マップ情報 */
} CMDPARA_MAPINFO, *PCMDPARA_MAPINFO;

/* マップパーツ情報通知 */
typedef struct _CMDPARA_MAPPARTSINFO {
	BYTE		byCmd;												/* コマンドID */
	WORD		wCount;												/* データ数 */
} CMDPARA_MAPPARTSINFO, *PCMDPARA_MAPPARTSINFO;

#pragma pack(push, 1)
/* キャラ座標通知 */
typedef struct _CMDPARA_CHARPOS {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwMapID;											/* マップID */
	BYTE		byX;												/* X座標 */
	BYTE		byY;												/* Y座標 */
	BYTE		byDirection;										/* 向き */
	BYTE		byFlg;												/* フラグ */
} CMDPARA_CHARPOS, *PCMDPARA_CHARPOS;
#pragma pack(pop)

/* キャラ情報要求 */
typedef struct _CMDPARA_CHARINFOREQ {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwTblIDReq;											/* 要求するプレイヤーID */
} CMDPARA_CHARINFOREQ, *PCMDPARA_CHARINFOREQ;

/* 発言通知 */
typedef struct _CMDPARA_SPEECH {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	WORD		wSize;												/* 文字数 */
	BYTE		byType;												/* 配信種別 */
	COLORREF	color;												/* 色 */
} CMDPARA_SPEECH, *PCMDPARA_SPEECH;

/* オンライン数通知 */
typedef struct _CMDPARA_ONLINE {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byAdmin;											/* 管理人数 */
	DWORD		dwData;												/* オンライン数 */
} CMDPARA_ONLINE, *PCMDPARA_ONLINE;

/* マップ書き換え */
typedef struct _CMDPARA_CHGMAP {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwID;												/* マップID */
	WORD		wMap;												/* 書き換えデータ */
	int			nX;													/* X座標 */
	int			nY;													/* Y座標 */
} CMDPARA_CHGMAP, *PCMDPARA_CHGMAP;

/* マップ幅増減 */
typedef struct _CMDPARA_ADDDELMAP {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwID;												/* マップID */
	BYTE		byType;												/* 0～3:追加 4～7:削除 */
	BYTE		byCount;											/* 幅 */
} CMDPARA_ADDDELMAP, *PCMDPARA_ADDDELMAP;

/* イベント情報追加要求 */
typedef struct _CMDPARA_EVENT {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	int			nData;												/* データ */
	DWORD		dwID;												/* マップID */
	DWORD		dwX;												/* 指定座標(X) */
	DWORD		dwY;												/* 指定座標(Y) */
} CMDPARA_EVENT, *PCMDPARA_EVENT;

/* イベント情報更新 */
typedef struct _CMDPARA_CHGEVENT {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwID;												/* マップID */
	DWORD		dwDataID;											/* イベントID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
	BYTE		byType;												/* 種別 */
	PVOID		pData;												/* データ */
} CMDPARA_CHGEVENT, *PCMDPARA_CHGEVENT;

/* マップ情報更新 */
typedef struct _CMDPARA_CHGMAPINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byBGM;												/* 音楽ID */
	BYTE		byFlg;												/* 各フラグ */
	BYTE		byLightLevel;										/* 明るさレベル */
	BYTE		byWeather;											/* 天候 */
	DWORD		dwID;												/* マップID */
	int			nStartX;											/* 初期位置(X) */
	int			nStartY;											/* 初期位置(Y) */
	int			nEntranceX;											/* 入り口(X) */
	int			nEntranceY;											/* 入り口(Y) */
	COLORREF	clChg;												/* 色変換 */
} CMDPARA_CHGMAPINFO, *PCMDPARA_CHGMAPINFO;

/* イベント座標通知 */
typedef struct _CMDPARA_EVENTPOS {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwCount;											/* データ数 */
} CMDPARA_EVENTPOS, *PCMDPARA_EVENTPOS;

/* 名前と性別更新 */
typedef struct _CMDPARA_NAMESEX {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		bySex;												/* 性別 */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
} CMDPARA_NAMESEX, *PCMDPARA_NAMESEX;

/* マップ移動 */
typedef struct _CMDPARA_MAPMOVE {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwX;												/* 移動先X座標 */
	DWORD		dwY;												/* 移動先Y座標 */
	DWORD		dwMapID;											/* マップID */
	BYTE		byEfect;											/* エフェクト種類 */
} CMDPARA_MAPMOVE, *PCMDPARA_MAPMOVE;

/* キャラ押して要求 */
typedef struct _CMDPARA_CHARPUSH {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwTblIDPush;										/* 押すキャラのテーブルID */
} CMDPARA_CHARPUSH, *PCMDPARA_CHARPUSH;

/* アカウント追加 */
typedef struct _CMDPARA_ACCADD {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwTblID;											/* テーブルID */
	char		szAcc[MAX_ACCNAME];									/* アカウント名 */
	char		szPas[MAX_ACCNAME];									/* パスワード */
} CMDPARA_ACCADD, *PCMDPARA_ACCADD;

/* エフェクト情報通知 */
typedef struct _CMDPARA_CHGEFC {
	BYTE		byCmd;												/* コマンドID */
	WORD		wData;												/* データ */
//以下エフェクト情報
} CMDPARA_CHGEFC, *PCMDPARA_CHGEFC;

/* 吹き出し情報通知 */
typedef struct _CMDPARA_CHGCEF {
	BYTE		byCmd;												/* コマンドID */
	WORD		wCount;												/* データ数 */
} CMDPARA_CHGCEF, *PCMDPARA_CHGCEF;

/* アイテム情報更新 */
typedef struct _CMDPARA_CHGITEM {
	BYTE		byCmd;												/* コマンドID */
	char		szName[MAX_ITEMNAME];								/* アイテム名 */
	BYTE		byType;												/* 種別 */
	BYTE		byData;												/* 付加データ */
	BYTE		byFlg;												/* フラグ */
	BYTE		bySound;											/* 使った時に鳴らす音 */
	BYTE		bySexDistinction;									/* 性別による種別 */
	BYTE		byEraseProbability;									/* 無くなる確率 */
	WORD		wGrpID;												/* グラフィックID */
	DWORD		dwGold;												/* 値段 */
	DWORD		dwID;												/* アイテムID */
	PVOID		pData;												/* 詳細情報 */
	LPSTR		pMsg;												/* 使った時のメッセージ */
} CMDPARA_CHGITEM, *PCMDPARA_CHGITEM;

/* 所持アイテム情報 */
typedef struct _CMDPARA_CHARITEM {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	WORD		waItem[MAX_MYITEM];									/* 所持アイテム */
	WORD		waEqu[MAX_EQUTYPE];									/* 装備品 */
} CMDPARA_CHARITEM, *PCMDPARA_CHARITEM;

/* サーバー情報 */
typedef struct _CMDPARA_SVRINFO {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwTime;												/* 稼動時間 */
	DWORD		dwMapCount;											/* 処理マップ数 */
	DWORD		dwCharCount;										/* 処理キャラ数 */
} CMDPARA_SVRINFO, *PCMDPARA_SVRINFO;

/* マップ内キャラ数通知 */
typedef struct _CMDPARA_MAPCHARINFO {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwCountPC;											/* PC数 */
	DWORD		dwCountNPC;											/* NPC数 */
} CMDPARA_MAPCHARINFO, *PCMDPARA_MAPCHARINFO;

/* 色設定 */
typedef struct _CMDPARA_NAMECOLOR {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	COLORREF	clName;												/* 名前の色 */
	COLORREF	clSpeech;											/* 発言の色 */
} CMDPARA_NAMECOLOR, *PCMDPARA_NAMECOLOR;

/* 店情報 */
typedef struct _CMDPARA_SHOP {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byID;												/* 店ID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byCountItem;										/* 売り物数 */
	WORD		awItem[MAX_SHOPITEM];								/* 売り物 */
	LPSTR		apMsg[MAX_SHOPMSG];									/* メッセージへのポインタテーブル */
} CMDPARA_SHOP, *PCMDPARA_SHOP;

/* ステータス通知 */
typedef struct _CMDPARA_CHARSTATUS {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		bySaveFlg;											/* 保存用フラグ */
	BYTE		byPW;												/* 力 */
	BYTE		byDEX;												/* 素早さ */
	BYTE		byST;												/* 体力 */
	BYTE		byIT;												/* 賢さ */
	BYTE		byLK;												/* 運のよさ */
	BYTE		byNpcType;											/* NPC移動種別 */
	BYTE		byJOB;												/* 職業 */
	BYTE		byLevel;											/* レベル */
	BYTE		byDrunkPoint;										/* 酔いポイント */
	BYTE		byDrunkLV;											/* 酔いレベル */
	BYTE		byDropProbability;									/* ドロップ率 */
	BYTE		abyMagic[MAX_USEMAGIC];								/* 呪文 */
	char		chAttrFire;											/* 属性(火) */
	char		chAttrWater;										/* 属性(水) */
	char		chAttrWind;											/* 属性(風) */
	char		chAttrEarth;										/* 属性(土) */
	char		chAttrLight;										/* 属性(光) */
	char		chAttrDark;											/* 属性(闇) */
	WORD		wAT;												/* 攻撃力 */
	WORD		wDF;												/* 守備力 */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wHP;												/* HP */
	WORD		wMaxMana;											/* 最大Mana */
	WORD		wMana;												/* Mana */
	WORD		wDFMagic;											/* 魔法防御 */
	WORD		waEqu[MAX_EQUTYPE];									/* 装備品 */
	DWORD		dwGold;												/* 所持金 */
	DWORD		dwExp;												/* 経験値 */
} CMDPARA_CHARSTATUS, *PCMDPARA_CHARSTATUS;

/* ステータス通知(HPのみ) */
typedef struct _CMDPARA_CHARSTATUSHP {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCharState;										/* キャラ状態 */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	WORD		wHP;												/* HP */
} CMDPARA_CHARSTATUSHP, *PCMDPARA_CHARSTATUSHP;

/* ステータス通知(HPとMANA) */
typedef struct _CMDPARA_CHARSTATUSHPMANA {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCharState;										/* キャラ状態 */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	WORD		wHP;												/* HP */
	WORD		wMaxHP;												/* 最大HP */
	WORD		wMana;												/* Mana */
	WORD		wMaxMana;											/* 最大Mana */
} CMDPARA_CHARSTATUSHPMANA, *PCMDPARA_CHARSTATUSHPMANA;

/* 文字エフェクト追加 */
typedef struct _CMDPARA_ADDCHAREFC {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	COLORREF	color;												/* 表示色 */
	char		szMsg[MAX_LENCHAREFC];								/* 表示文字列 */
} CMDPARA_ADDCHAREFC, *PCMDPARA_ADDCHAREFC;

/* マップ色変換 */
typedef struct _CMDPARA_CHGMAPCOLOR {
	BYTE		byCmd;												/* コマンドID */
	DWORD		dwMapID;											/* マップID */
	COLORREF	color;												/* 変換色 */
} CMDPARA_CHGMAPCOLOR, *PCMDPARA_CHGMAPCOLOR;

/* 取り引き情報 */
typedef struct _CMDPARA_TRADEINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byMode;												/* 状態 */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		bySum;												/* チェックサム */
	WORD		waItem[MAX_MYITEM];									/* アイテム */
	DWORD		dwGold;												/* 金額 */
} CMDPARA_TRADEINFO, *PCMDPARA_TRADEINFO;

/* 取り引き決定 */
typedef struct _CMDPARA_TRADEPROC {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySum1;												/* チェックサム1 */
	BYTE		bySum2;												/* チェックサム2 */
	DWORD		dwTblID1;											/* テーブルID1 */
	DWORD		dwTblID2;											/* テーブルID2 */
	WORD		waItem1[MAX_MYITEM];								/* アイテム1 */
	WORD		waItem2[MAX_MYITEM];								/* アイテム2 */
	DWORD		dwGold1;											/* 金額1 */
	DWORD		dwGold2;											/* 金額2 */
} CMDPARA_TRADEPROC, *PCMDPARA_TRADEPROC;

/* アカウント検索 */
typedef struct _CMDPARA_ACCSEARCH {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byMode;												/* 検索モード */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblIDStart;										/* 検索開始テーブルID */
	char		szName[MAX_CHARNAMEBUF];							/* キャラ名 */
} CMDPARA_ACCSEARCH, *PCMDPARA_ACCSEARCH;

/* サーバー情報アップロード情報 */
typedef struct _SERVERINFOUPLOADINFO {
	char		szServerName[MAX_PATH];								/* サーバー名 */
	char		szAccount[32];										/* アカウント名 */
	char		szPassword[32];										/* パスワード */
	char		szDirectory[MAX_PATH];								/* アップ先ディレクトリ */
} SERVERINFOUPLOADINFO, *PSERVERINFOUPLOADINFO;

/* メッセージコマンド */
typedef struct _CMDPARA_MSGCMD {
	BYTE		byCmd;												/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byData;												/* データ */
	int			nData1;												/* パラメータ */
} CMDPARA_MSGCMD, *PCMDPARA_MSGCMD;

/* 仲間コマンド */
typedef struct _CMDPARA_PARTYCMD {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byCmdSub;											/* 仲間コマンドID */
	BYTE		byData;												/* データ */
	DWORD		dwTblIDFrom;										/* コマンド送信元 */
	DWORD		dwTblIDTo;											/* コマンド送信先 */
} CMDPARA_PARTYCMD, *PCMDPARA_PARTYCMD;

/* 職業情報通知 */
typedef struct _CMDPARA_JOBINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byID;												/* 職業ID */
/* 以下 CInfoJob::GetSendData の形（名前は可変長・レベルアップ情報） */
} CMDPARA_JOBINFO, *PCMDPARA_JOBINFO;

/* 魔法情報通知 */
typedef struct _CMDPARA_MAGICINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byID;												/* 職業ID */
	char		szName[MAX_MAGICNAME];								/* 名前 */
	BYTE		byType;												/* 効果種別 */
	BYTE		byTypeInfoID;										/* 種別情報ID */
	BYTE		byUserInfo;											/* 利用者種別情報 */
	BYTE		byTarget;											/* 使用対象 */
	BYTE		byTargetType;										/* 発動種別 */
	BYTE		byTargetArea;										/* 発動範囲 */
	BYTE		byTargetCount;										/* 発動人数 */
	BYTE		byTargetEfectID;									/* エフェクト番号（相手） */
	BYTE		byUserEfectID;										/* エフェクト番号（利用者） */
	BYTE		byTime;												/* 詠唱時間(1で1秒10で10秒) */
	BYTE		byAttr;												/* 属性 */
	WORD		wData1;												/* 付加データ */
	WORD		wData2;												/* 付加データ */
	WORD		wData3;												/* 付加データ */
	WORD		wData4;												/* 付加データ */
	WORD		wMana;												/* 消費Mana */
	char		szExplain[MAX_MAGICEXPLAIN];						/* 説明文（古い版との互換のため最後に置く） */
} CMDPARA_MAGICINFO, *PCMDPARA_MAGICINFO;

/* 魔法利用者情報通知 */
typedef struct _CMDPARA_MAGICUSERINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byID;												/* 利用者情報ID */
	BYTE		byCount;											/* 職業数 */
	char		szName[MAX_EQUNAME];								/* 名前 */
} CMDPARA_MAGICUSERINFO, *PCMDPARA_MAGICUSERINFO;

/* 装備職業情報通知 */
typedef struct _CMDPARA_EQUUSERINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		byID;												/* 利用者情報ID */
	BYTE		byCount;											/* 職業数 */
	char		szName[MAX_EQUNAME];								/* 名前 */
} CMDPARA_EQUUSERINFO, *PCMDPARA_EQUUSERINFO;

/* パケット基底構造体 */
typedef struct _PACKETBASE {
	BYTE		byCmdMain;											/* コマンドID(メイン) */
	BYTE		byCmdSub;											/* コマンドID(サブ) */
} PACKETBASE, *PPACKETBASE;

/* キャラ系:状態通知 */
typedef struct _CMDPARA_CHAR_STATEINFO {
	BYTE		byCmdMain;											/* コマンドID(メイン) */
	BYTE		byCmdSub;											/* コマンドID(サブ) */
	DWORD		dwTblID;											/* 対象キャラのテーブルID */
	DWORD		dwTailTblID;										/* ついていっているキャラのテーブルID */
} CMDPARA_CHAR_STATEINFO, *PCMDPARA_CHAR_STATEINFO;

/* 以下サブゲーム用 */
/* 汎用データ１ */
typedef struct _SUBCMDPARA_DATA1 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byData;												/* データ */
} SUBCMDPARA_DATA1, *PSUBCMDPARA_DATA1;

/* 汎用データ２ */
typedef struct _SUBCMDPARA_DATA2 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwData;												/* データ */
} SUBCMDPARA_DATA2, *PSUBCMDPARA_DATA2;

/* 汎用データ３ */
typedef struct _SUBCMDPARA_DATA3 {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	BYTE		byData1;											/* データ1 */
	BYTE		byData2;											/* データ2 */
} SUBCMDPARA_DATA3, *PSUBCMDPARA_DATA3;

/* サブゲーム(クライアント側)：爆弾ゲーム情報 */
typedef struct _SUBCMDPARA_BOMBINFO {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	BYTE		byID;												/* サブゲームID */
	BYTE		byGameTime;											/* 制限時間 */
	BYTE		byMaxBomb;											/* 最大設置数 */
	BYTE		byMaxFire;											/* 最大火力 */
	BYTE		byFlgGameNow	: 1;								/* 試合中？ */
	BYTE		byFlgUseBlock	: 1;								/* 壊れる壁使う／使わない */
	BYTE		byFlgUseItem	: 1;								/* アイテム使う／使わない */
	BYTE		byFlgProgress	: 1;								/* 途中参加あり？ */
	BYTE		byReserve		: 4;								/* 予約 */
	DWORD		dwTblIDGameMaster;									/* ゲームマスターのテーブルID */
} SUBCMDPARA_BOMBINFO, *PSUBCMDPARA_BOMBINFO;

/* 爆弾設置 */
typedef struct _CMDPARA_BOMBSETBOMB {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	BYTE		byNpcType;											/* NPC移動種別 */
	DWORD		pID;												/* プレイヤーID */
	DWORD		dwTblID;											/* テーブルID */
	DWORD		dwX;												/* X座標 */
	DWORD		dwY;												/* Y座標 */
} CMDPARA_BOMBSETBOMB, *PCMDPARA_BOMBSETBOMB;

/* 行動順序通知 */
typedef struct _CMDPARA_WARSELECTTURN {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	BYTE		byCount;											/* メンバー数 */
	char		aszName[MAX_SUBGAME_WARMEMBER][MAX_CHARNAMEBUF];	/* メンバー名 */
} CMDPARA_WARSELECTTURN, *PCMDPARA_WARSELECTTURN;

/* 魔法対象者一覧通知 */
typedef struct _CMDPARA_WARSELECTMAGICTARGET {
	BYTE		byCmd;												/* コマンドID */
	BYTE		bySubGameID;										/* サブゲームID */
	BYTE		byCmdSub;											/* コマンドID */
	BYTE		byCount;											/* メンバー数 */
	BYTE		byMagic;											/* 魔法 */
	char		aszName[MAX_SUBGAME_WARMEMBER][MAX_CHARNAMEBUF];	/* メンバー名 */
	BYTE		abyNo[MAX_SUBGAME_WARMEMBER];						/* 配列番号 */
} CMDPARA_WARSELECTMAGICTARGET, *PCMDPARA_WARSELECTMAGICTARGET;

/* Copyright(C)URARA-works 2003-2006 */
