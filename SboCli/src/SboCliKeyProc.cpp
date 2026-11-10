/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			キー入力処理											 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */
#include "stdafx.h"
#include "../../Common/Info/InfoJob.h"
#include "Glbldata.h"
#include "SboCliMain.h"
#include "SboCliMakeWindow.h"
#include "SboCliDrawWindow.h"
#include "SboCliSendCmd.h"
#include "SboCliRecvCmd.h"
#include "SboCliMsgLogWindow.h"
#include "SboCliDrawScreen.h"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* キー入力処理 */
void KeyProc								(void);						/* キー入力処理 */
void KeyProcMove							(BYTE, BYTE);				/* 歩き中 */
void KeyProcMoveNext						(void);						/* 歩き終わった時の続けて歩く処理 */
void KeyProcDead							(BYTE, BYTE);				/* 死亡中 */
void KeyProcLOGINMENU						(PWNDINFO, BYTE, BYTE);		/* ログインメニュー */
void KeyProcCOMMAND							(PWNDINFO, BYTE, BYTE);		/* コマンド */
void KeyProcCOMMANDETC						(PWNDINFO, BYTE, BYTE);		/* その他コマンド */
void KeyProcCOMMANDETC_CONF					(PWNDINFO, BYTE, BYTE);		/* 管理人呼び出し確認 */
void KeyProcCOMMANDITEM						(PWNDINFO, BYTE, BYTE);		/* 道具コマンド */
void KeyProcCOMMANDITEMBATTLE				(PWNDINFO, BYTE, BYTE);		/* 道具コマンド(戦闘マップ) */
void KeyProcCOMMANDITEMPROC					(PWNDINFO, BYTE, BYTE);		/* 道具処理選択 */
void KeyProcCOMMANDITEMCHAR					(PWNDINFO, BYTE, BYTE);		/* 道具使う相手選択 */
void KeyProcCOMMANDOPTION					(PWNDINFO, BYTE, BYTE);		/* オプションコマンド */
void KeyProcCOMMANDOPTIONVOLSOUND			(PWNDINFO, BYTE, BYTE);		/* 効果音音量設定 */
void KeyProcCOMMANDOPTIONVOLMUSIC			(PWNDINFO, BYTE, BYTE);		/* 曲音量設定 */
void KeyProcCOMMANDSTATUS					(PWNDINFO, BYTE, BYTE);		/* 強さコマンド */
void KeyProcMSG								(PWNDINFO, BYTE, BYTE);		/* メッセージ */
void KeyProcMALEFAMALE						(PWNDINFO, BYTE, BYTE);		/* 男女 */
void KeyProcDELETECONFIRM					(PWNDINFO, BYTE, BYTE);		/* 削除確認 */
void KeyProcNAMECOLOR						(PWNDINFO, BYTE, BYTE);		/* 名前の色設定 */
void KeyProcSHOP							(PWNDINFO, BYTE, BYTE);		/* 店 */
void KeyProcSHOPSELL						(PWNDINFO, BYTE, BYTE);		/* 店(売るだけ) */
void KeyProcSPEECHEDIT						(PWNDINFO, BYTE, BYTE);		/* 自己紹介設定 */
void KeyProcCOMMANDEQU						(PWNDINFO, BYTE, BYTE);		/* 装備 */
void KeyProcCOMMANDEQUSUB					(PWNDINFO, BYTE, BYTE);		/* 装備(種類別) */
void KeyProcLEAVE							(PWNDINFO, BYTE, BYTE);		/* 預かり所 */
void KeyProcCOMMANDTRADE					(PWNDINFO, BYTE, BYTE);		/* 取り引きコマンド */
void KeyProcTRADE							(PWNDINFO, BYTE, BYTE);		/* 取り引き */
void KeyProcTRADEITEM						(PWNDINFO, BYTE, BYTE);		/* 取り引き：アイテム選択 */
void KeyProcTRADEGOLD						(PWNDINFO, BYTE, BYTE);		/* 取り引き：金額設定 */
void KeyProcSUBGAME_BOMBSET					(PWNDINFO, BYTE, BYTE);		/* サブゲーム：爆弾ゲーム設定 */
void KeyProcCHGPASSWORD						(PWNDINFO, BYTE, BYTE);		/* パスワードの変更 */
void KeyProcCOMMANDPARTY					(PWNDINFO, BYTE, BYTE);		/* 仲間ウィンドウ */
void KeyProcCOMMANDPARTYCHAR				(PWNDINFO, BYTE, BYTE);		/* 誘う相手選択 */
void KeyProcCONF							(PWNDINFO, BYTE, BYTE);		/* 確認 */
void StartPosConfirmEnd					(PWNDINFO, BOOL);			/* 復活位置の確認の後処理 */
void StartPosFinish						(PWNDINFO, BOOL, int, int);	/* 復活位置を記録する・しないを決めてセリフの続きへ */
void KeyProcJOB								(PWNDINFO, BYTE, BYTE);		/* 冒険者登録所 */
void KeyProcCOMMANDMAGIC					(PWNDINFO, BYTE, BYTE);		/* 魔法コマンド */
void KeyProcCOMMANDMAGICBATTLE				(PWNDINFO, BYTE, BYTE);		/* 魔法コマンド(戦闘マップ) */
void KeyProcSUBWAR_SELECTTEAM				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：チーム選択 */
void KeyProcSUBWAR_WAIT						(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：他のプレイヤー待ち */
void KeyProcSUBWAR_SELECTRULE				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：ルール選択 */
void KeyProcSUBWAR_SELECTJOB				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：職業選択 */
void KeyProcSUBWAR_SELECTTURN				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：行動順序 */
void KeyProcSUBWAR_TURN						(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：行動選択 */
void KeyProcSUBWAR_TURN_MOVE				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：行動(移動) */
void KeyProcSUBWAR_TURN_ATACK				(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：行動(攻撃) */
void KeyProcSUBWAR_TURN_DIFENCE_FIGHTER		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：防御：戦士 */
void KeyProcSUBWAR_TURN_DIFENCE_MARTIAL		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：防御：武道家 */
void KeyProcSUBWAR_TURN_DIFENCE_WITCH		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：防御：魔法使い */
void KeyProcSUBWAR_TURN_DIFENCE_PRIEST		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：防御：僧侶 */
void KeyProcSUBWAR_TURN_DIFENCE_WITCH2		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：魔法防御：魔法使い */
void KeyProcSUBWAR_TURN_MAGIC_WITCH			(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：魔法：魔法使い */
void KeyProcSUBWAR_TURN_MAGIC_TARGET		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：魔法対象者選択 */
void KeyProcSUBWAR_TURN_MAGIC_PRIEST		(PWNDINFO, BYTE, BYTE);		/* 戦場ゲーム：魔法：僧侶 */
static BOOL ShotTurnGage						(PCHARINFO);				/* ゲージが溜まっていれば殴る(ジャスト・早撃ち) */
void KeyProcBATTLE_COMMAND					(PWNDINFO, BYTE, BYTE);		/* 戦闘：コマンド */
void KeyProcBATTLE_TARGET					(PWNDINFO, BYTE, BYTE);		/* 戦闘：ターゲット選択 */
void KeyProcSUBSUGOROKU_JOINMODE			(PWNDINFO, BYTE, BYTE);		/* すごろく：参加モード選択 */
void KeyProcSUBSUGOROKU_COMMAND				(PWNDINFO, BYTE, BYTE);		/* すごろく：行動選択 */
void KeyProcSUBSUGOROKU_IDLE				(PWNDINFO, BYTE, BYTE);		/* すごろく：平常状態 */
void KeyProcDEAD_COMMAND					(PWNDINFO, BYTE, BYTE);		/* 死亡中：コマンド */
void KeyProcSTATUSMAGIC					(PWNDINFO, BYTE, BYTE);		/* 強さ：覚えている特技 */
void KeyProcMAKER							(PWNDINFO, BYTE, BYTE);		/* 作り手（レシピ）：作れる物の一覧・確認 */
void KeyProcTOWNWARP						(PWNDINFO, BYTE, BYTE);		/* 町転送：行き先の選択 */

LRESULT CALLBACK AccWndProc					(HWND, UINT, WPARAM, LPARAM);	/* アカウント名入力用メッセージ処理 */
LRESULT CALLBACK PasWndProc					(HWND, UINT, WPARAM, LPARAM);	/* パスワード入力用メッセージ処理 */
LRESULT CALLBACK ConWndProc					(HWND, UINT, WPARAM, LPARAM);	/* 接続ボタン用メッセージ処理 */
LRESULT CALLBACK NameWndProc				(HWND, UINT, WPARAM, LPARAM);	/* キャラ名入力用メッセージ処理 */
LRESULT CALLBACK MakeWndProc				(HWND, UINT, WPARAM, LPARAM);	/* 作成ボタン用メッセージ処理 */
LRESULT CALLBACK SpeechWndProc				(HWND, UINT, WPARAM, LPARAM);	/* 発言入力欄用メッセージ処理 */
static BOOL IsSpeechModeEnable				(BYTE);							/* 発言の種類が今選べるか */
#define SPEECHMODE_MAX	(4)															/* 発言の種類の数(g_bySpeechMode) */
LRESULT CALLBACK SpeechEditWndProc			(HWND, UINT, WPARAM, LPARAM);	/* 自己紹介設定欄用メッセージ処理 */
LRESULT CALLBACK ChgPasswordWndProc			(HWND, UINT, WPARAM, LPARAM);	/* パスワード変更用メッセージ処理 */

void CmdProcHanasu		(PWNDINFO);											/* コマンド処理(話す) */
LPSTR MakeNpcSpeechNextExp	(LPCSTR);											/* 神父さんのセリフに次のレベルまでのお告げを足す */
BOOL CmdProcSiraberu	(PWNDINFO);											/* コマンド処理(調べる) */
int GetFrontTalkChar		(void);												/* 話しかけられる目の前のキャラを探す */
int GetSKeyTalkChar		(void);												/* Sキーで話しかける相手を探す */
BOOL IsSKeyCounterTalk	(void);												/* Sキーでカウンター越しに話す相手がいるか */
int GetFrontScoreChar	(void);												/* 目の前の得点板を探す */
BOOL CmdProcSKey		(void);												/* Sキー(調べる・話す・扉) */
void ChatMsgProc		(LPCTSTR);											/* 発言メッセージ処理 */
#ifdef __EMSCRIPTEN__
extern void SboWebSpeechSubmit(LPCSTR pszSjis, int nKeyMod);					/* 発言（ブラウザ版の分割送信） */
/* ブラウザ版：発言した時のキー（0:なし 1:Shift 2:Ctrl）。-1 ならキーの状態を見る。
   チャット欄からの発言や、分けて後から送る発言は入力した時の状態をここに入れて渡す */
int g_nWebSpeechKeyMod = -1;
#endif

extern int GetWindowID(DWORD);										/* ウィンドウ番号を取得 */


/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef void (*KEYPROC)(PWNDINFO, BYTE, BYTE);

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
BYTE g_abyKeyState[256];
BYTE m_byKeyOnX = 0,
	 m_byKeyOnZ = 0,
	 m_byKeyOnS = 0,
	 m_byKeyOnC = 0,
	 m_byKeyOnReturn = 0,
	 m_byKeyOnUp = 0,
	 m_byKeyOnDown = 0,
	 m_byKeyOnLeft = 0,
	 m_byKeyOnRight = 0,
	 m_byKeyOnTab = 0,
	 m_byKeyOn1 = 0,
	 m_byKeyOn2 = 0,
	 m_byKeyOnEsc = 0,
	 m_byKeyOnCtrl = 0;

/* 関数ジャンプテーブル */
const KEYPROC KeyProcTbl[] = {
	NULL,
	KeyProcMSG,							/* WNDTYPE_MSG							メッセージ表示 */
	NULL,								/* WNDTYPE_MSGVIEW						常に表示メッセージ表示 */
	NULL,								/* WNDTYPE_MSG3SEC						３秒メッセージ表示 */
	NULL,								/* WNDTYPE_LOGIN						ログイン情報 */
	KeyProcLOGINMENU,					/* WNDTYPE_LOGINMENU					ログインメニュー */
	NULL,								/* WNDTYPE_CONNECTLOST					接続途切れた */
	KeyProcCOMMAND,						/* WNDTYPE_COMMAND						コマンド表示 */
	KeyProcCOMMANDETC,					/* WNDTYPE_COMMANDETC					その他コマンド */
	KeyProcCOMMANDITEM,					/* WNDTYPE_COMMANDITEM					道具 */
	KeyProcCOMMANDITEMBATTLE,			/* WNDTYPE_COMMANDITEMBATTLE			道具(戦闘マップ) */
	KeyProcCOMMANDITEMPROC,				/* WNDTYPE_COMMANDITEMPROC				道具処理選択 */
	KeyProcCOMMANDITEMCHAR,				/* WNDTYPE_COMMANDITEMCHAR				道具使う相手選択 */
	KeyProcCOMMANDITEMCHAR,				/* WNDTYPE_COMMANDITEMCHARBATTLE		道具使う相手選択(戦闘マップ) */
	KeyProcCOMMANDOPTION,				/* WNDTYPE_COMMANDOPTION				オプションコマンド */
	KeyProcCOMMANDOPTIONVOLSOUND,		/* WNDTYPE_COMMANDOPTIONVOLSOUND		効果音音量設定 */
	KeyProcCOMMANDOPTIONVOLMUSIC,		/* WNDTYPE_COMMANDOPTIONVOLMUSIC		曲音量設定 */
	NULL,								/* WNDTYPE_SPEECH						発言入力欄 */
	NULL,								/* WNDTYPE_LOGINMENUCHARINFO			ログインメニューのキャラ情報 */
	NULL,								/* WNDTYPE_LOGINMENUCHARMAKE			ログインメニューのキャラ作成 */
	KeyProcMALEFAMALE,					/* WNDTYPE_MALEFAMALE					男女 */
	KeyProcDELETECONFIRM,				/* WNDTYPE_DELETECONFIRM				削除確認 */
	KeyProcNAMECOLOR,					/* WNDTYPE_NAMECOLOR					名前の色設定 */
	KeyProcSHOP,						/* WNDTYPE_SHOP							店 */
	KeyProcSHOPSELL,					/* WNDTYPE_SHOPSELL						店(売るだけ) */
	KeyProcSPEECHEDIT,					/* WNDTYPE_SPEECHEDIT					自己紹介設定 */
	KeyProcCOMMANDETC_CONF,				/* WNDTYPE_COMMANDETC_CONF				管理人呼び出し確認 */
	KeyProcCOMMANDSTATUS,				/* WNDTYPE_COMMANDSTATUS				強さコマンド */
	KeyProcCOMMANDEQU,					/* WNDTYPE_COMMANDEQU					装備 */
	NULL,								/* WNDTYPE_COMMANDEQUSUB				装備(種類別) */
	KeyProcCOMMANDEQUSUB,				/* WNDTYPE_COMMANDEQUSELECT				装備(選択) */
	KeyProcLEAVE,						/* WNDTYPE_LEAVE						預かり所 */
	KeyProcCOMMANDTRADE,				/* WNDTYPE_COMMANDTRADE					取り引きコマンド */
	KeyProcTRADE,						/* WNDTYPE_TRADE						取り引き */
	KeyProcTRADEITEM,					/* WNDTYPE_TRADEITEM					取り引き：アイテム選択 */
	KeyProcTRADEGOLD,					/* WNDTYPE_TRADEGOLD					取り引き：金額設定 */
	NULL,								/* WNDTYPE_GOLD							所持金表示 */
	NULL,								/* WNDTYPE_SUBGAME_BOMBINFO				サブゲーム：爆弾ゲーム情報 */
	KeyProcSUBGAME_BOMBSET,				/* WNDTYPE_SUBGAME_BOMBSET				サブゲーム：爆弾ゲーム設定 */
	KeyProcCHGPASSWORD,					/* WNDTYPE_CHGPASSWORD					パスワードの変更 */
	KeyProcCOMMANDPARTY,				/* WNDTYPE_COMMANDPARTY					仲間ウィンドウ */
	KeyProcCOMMANDPARTYCHAR,			/* WNDTYPE_COMMANDPARTYCHAR				誘う相手選択 */
	KeyProcCONF,						/* WNDTYPE_CONF							確認 */
	KeyProcJOB,							/* WNDTYPE_JOB							冒険者登録所 */
	KeyProcCOMMANDMAGIC,				/* WNDTYPE_COMMANDMAGIC					魔法コマンド */
	KeyProcCOMMANDMAGICBATTLE,			/* WNDTYPE_COMMANDMAGICBATTLE			魔法コマンド(戦闘マップ) */
	KeyProcSUBWAR_SELECTTEAM,			/* WNDTYPE_SUBWAR_SELECTTEAM			戦場ゲーム：チーム選択 */
	KeyProcSUBWAR_WAIT,					/* WNDTYPE_SUBWAR_WAIT					戦場ゲーム：他のプレイヤー待ち */
	KeyProcSUBWAR_SELECTRULE,			/* WNDTYPE_SUBWAR_SELECTRULE			戦場ゲーム：ルール選択 */
	KeyProcSUBWAR_SELECTJOB,			/* WNDTYPE_SUBWAR_SELECTJOB				戦場ゲーム：職業選択 */
	KeyProcSUBWAR_SELECTTURN,			/* WNDTYPE_SUBWAR_SELECTTURN			戦場ゲーム：行動順序 */
	KeyProcSUBWAR_TURN,					/* WNDTYPE_SUBWAR_TURN					戦場ゲーム：行動選択 */
	KeyProcSUBWAR_TURN_MOVE,			/* WNDTYPE_SUBWAR_TURN_MOVE				戦場ゲーム：行動(移動) */
	KeyProcSUBWAR_TURN_ATACK,			/* WNDTYPE_SUBWAR_TURN_ATACK			戦場ゲーム：行動(攻撃) */
	KeyProcSUBWAR_TURN_DIFENCE_FIGHTER,	/* WNDTYPE_SUBWAR_TURN_DIFENCE_FIGHTER	戦場ゲーム：防御：戦士 */
	KeyProcSUBWAR_TURN_DIFENCE_MARTIAL,	/* WNDTYPE_SUBWAR_TURN_DIFENCE_MARTIAL	戦場ゲーム：防御：武道家 */
	KeyProcSUBWAR_TURN_DIFENCE_WITCH,	/* WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH	戦場ゲーム：防御：魔法使い */
	KeyProcSUBWAR_TURN_DIFENCE_PRIEST,	/* WNDTYPE_SUBWAR_TURN_DIFENCE_PRIEST	戦場ゲーム：防御：僧侶 */
	KeyProcSUBWAR_TURN_DIFENCE_WITCH2,	/* WNDTYPE_SUBWAR_TURN_DIFENCE_WITCH2	戦場ゲーム：魔法防御：魔法使い */
	KeyProcSUBWAR_TURN_MAGIC_WITCH,		/* WNDTYPE_SUBWAR_TURN_MAGIC_WITCH		戦場ゲーム：魔法：魔法使い */
	KeyProcSUBWAR_TURN_MAGIC_TARGET,	/* WNDTYPE_SUBWAR_TURN_MAGIC_TARGET		戦場ゲーム：魔法対象者選択 */
	KeyProcSUBWAR_TURN_MAGIC_PRIEST,	/* WNDTYPE_SUBWAR_TURN_MAGIC_PRIEST		戦場ゲーム：魔法：僧侶 */
	NULL,								/* WNDTYPE_ITEMEQU						装備アイテム */
	NULL,								/* WNDTYPE_STATUS_ATTR					ステータス表示(属性) */
	KeyProcBATTLE_COMMAND,				/* WNDTYPE_BATTLE_COMMAND				戦闘：コマンド */
	KeyProcBATTLE_TARGET,				/* WNDTYPE_BATTLE_TARGET				戦闘：ターゲット選択 */
	KeyProcSUBSUGOROKU_JOINMODE,		/* WNDTYPE_SUBSUGOROKU_JOINMODE			すごろく：参加モード選択 */
	KeyProcSUBSUGOROKU_COMMAND,			/* WNDTYPE_SUBSUGOROKU_COMMAND			すごろく：行動選択 */
	KeyProcSUBSUGOROKU_IDLE,			/* WNDTYPE_SUBSUGOROKU_IDLE				すごろく：平常状態 */
	NULL,								/* WNDTYPE_ITEMINFO						アイテム情報 */
	KeyProcDEAD_COMMAND,				/* WNDTYPE_DEAD_COMMAND					死亡中：コマンド */
	KeyProcSTATUSMAGIC,					/* WNDTYPE_STATUSMAGIC					強さ：覚えている特技 */
	KeyProcMAKER,						/* WNDTYPE_MAKER						作り手（レシピ）：作れる物の一覧・確認 */
	KeyProcTOWNWARP,					/* WNDTYPE_TOWNWARP					町転送：行き先の選択 */
};


/* ========================================================================= */
/* 関数名：KeyProc															 */
/* 内容：キー入力処理														 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
/* ========================================================================= */
/* 関数名：	GuardStart / GuardEnd / CanGuard								 */
/* 内容：	構え(防御)：戦闘モードで Z を GUARD_HOLDTIME 押し続けると構える	 */
/*			構えている間は受けるダメージが半分・会心を受けない。ゲージは止まる	 */
/* ========================================================================= */
static DWORD m_dwTimeZDown = 0;		/* Z を押し始めた時間 */
static BOOL m_bGuardThisHold = FALSE;	/* 今の Z の押し続けで一度でも構えたか(構えたまま殴って解けた時も離した時に「攻撃をやめる」をしない) */

static BOOL CanGuard(void)
{
	PCHARINFO pChar;

	if ((g_nMyCharID < 0) || g_dwActiveMsgID || g_bSpeech || (g_bBattleMode == FALSE)) {
		return FALSE;
	}
	if ((g_pMapInfo == NULL) || ((g_pMapInfo->byFlg & MAPFLG_BATTLE) == 0)) {
		return FALSE;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || pChar->dwTimeTurnMagicLV) {
		return FALSE;
	}
	if (pChar->abySubStatus[0] != 0) {
		return FALSE;
	}
	return TRUE;
}

static void GuardStart(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	g_bGuard = TRUE;
	m_bGuardThisHold = TRUE;
	g_dwGuardGageElapse = 0;
	if (pChar->dwTimeTurnLV) {
		g_dwGuardGageElapse = timeGetTime () - pChar->dwTimeTurnLV;
	}
	SendCmdREQ_GUARD (TRUE);
	g_bDraw = TRUE;
}

void GuardEnd(void)
{
	if (!g_bGuard) {
		return;
	}
	g_bGuard = FALSE;
	SendCmdREQ_GUARD (FALSE);
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 同じキー処理の中で起きたキーの押した・離したを、起きた順に全部処理する		 */
/* 今までは最後に判定したキーのイベントだけが残るため、X を離した直後に		 */
/* Z を押すなど、2つのキーが同じキー処理に重なると X の攻撃が消えていた。	 */
/* (方向キーの押しっぱなしは今までどおり毎回1つだけ)							 */
/* ========================================================================= */
#define KEYEV_MAX	32
static BYTE m_abyKeyEvEvent[KEYEV_MAX];		/* 起きたキーイベント */
static BYTE m_abyKeyEvCode[KEYEV_MAX];		/* 起きたキー */
static int  m_nKeyEvCount = 0;				/* 起きたキーイベントの数 */

static void KeyEvPush(BYTE byEvent, BYTE byCode)
{
	if (m_nKeyEvCount < KEYEV_MAX) {
		m_abyKeyEvEvent[m_nKeyEvCount] = byEvent;
		m_abyKeyEvCode[m_nKeyEvCount] = byCode;
		m_nKeyEvCount ++;
	}
}

/* キーイベントを、開いているウィンドウか歩き中の処理へ渡す */
static void KeyProcDispatch(BYTE byEvent, BYTE byCode)
{
	if (!g_dwActiveMsgID) {
		if (g_nMyCharID >= 0) {
			if ((g_FadeInOutInfo.byFadeIn == 0) && (g_FadeInOutInfo.byFadeOut == 0)) {
				KeyProcMove (byEvent, byCode);
			}
		}

	} else {
		int nNo = GetWindowID (g_dwActiveMsgID);
		if (nNo < 0) {
			return;
		}
		PWNDINFO pWnd = (PWNDINFO)g_WndInfo[nNo];
		if (KeyProcTbl[pWnd->nType]) {
			KeyProcTbl[pWnd->nType] (pWnd, byEvent, byCode);
		}
	}
}

void KeyProc(void)
{
	BYTE byEvent = 0, byCode, byKeyOnZBack;
	int i;

	m_nKeyEvCount = 0;

	if (!g_bWindowActive) {
		/* 画面から離れたら構えもやめる(キーを離したのが分からないため) */
		GuardEnd ();
		m_dwTimeZDown = 0;
		return;
	}

	GetKeyboardState (g_abyKeyState);

	/* ↑キー押した */
	if (g_abyKeyState[VK_UP] & 0x80) {
		m_byKeyOnUp = 1;
		byEvent = KEY_DOWN;
		byCode = VK_UP;

	/* ↑キー離した */
	} else if (((g_abyKeyState[VK_UP] & 0x80) == 0) && (m_byKeyOnUp == 1)) {
		m_byKeyOnUp = 0;
		byEvent = KEY_UP;
		byCode = VK_UP;
	}

	/* ↓キー押した */
	if (g_abyKeyState[VK_DOWN] & 0x80) {
		m_byKeyOnDown = 1;
		byEvent = KEY_DOWN;
		byCode = VK_DOWN;

	/* ↓キー離した */
	} else if (((g_abyKeyState[VK_DOWN] & 0x80) == 0) && (m_byKeyOnDown == 1)) {
		m_byKeyOnDown = 0;
		byEvent = KEY_UP;
		byCode = VK_DOWN;
	}

	/* ←キー押した */
	if (g_abyKeyState[VK_LEFT] & 0x80) {
		m_byKeyOnLeft = 1;
		byEvent = KEY_DOWN;
		byCode = VK_LEFT;

	/* ←キー離した */
	} else if (((g_abyKeyState[VK_LEFT] & 0x80) == 0) && (m_byKeyOnLeft == 1)) {
		m_byKeyOnLeft = 0;
		byEvent = KEY_UP;
		byCode = VK_LEFT;
	}

	/* →キー押した */
	if (g_abyKeyState[VK_RIGHT] & 0x80) {
		m_byKeyOnRight = 1;
		byEvent = KEY_DOWN;
		byCode = VK_RIGHT;

	/* →キー離した */
	} else if (((g_abyKeyState[VK_RIGHT] & 0x80) == 0) && (m_byKeyOnRight == 1)) {
		m_byKeyOnRight = 0;
		byEvent = KEY_UP;
		byCode = VK_RIGHT;
	}

	/* Xキー押した */
	if ((g_abyKeyState['X'] & 0x80) && (m_byKeyOnX == 0)) {
		m_byKeyOnX = 1;
		byEvent = KEY_DOWN;
		byCode = 'A';
		KeyEvPush (byEvent, byCode);

	/* Xキー離した */
	} else if (((g_abyKeyState['X'] & 0x80) == 0) && (m_byKeyOnX == 1)) {
		m_byKeyOnX = 0;
		byEvent = KEY_UP;
		byCode = 'A';
		KeyEvPush (byEvent, byCode);
	}

	byKeyOnZBack = m_byKeyOnZ;
	/* Zキー押した */
	if ((g_abyKeyState['Z'] & 0x80) && (m_byKeyOnZ == 0)) {
		m_byKeyOnZ = 1;
		byEvent = KEY_DOWN;
		byCode = 'B';
		KeyEvPush (byEvent, byCode);

	/* Zキー離した */
	} else if (((g_abyKeyState['Z'] & 0x80) == 0) && (m_byKeyOnZ == 1)) {
		m_byKeyOnZ = 0;
		byEvent = KEY_UP;
		byCode = 'B';
		KeyEvPush (byEvent, byCode);
	}

	/* Sキー押した */
	if ((g_abyKeyState['S'] & 0x80) && (m_byKeyOnS == 0)) {
		m_byKeyOnS = 1;
		byEvent = KEY_DOWN;
		byCode = 'Y';
		KeyEvPush (byEvent, byCode);

	/* Sキー離した */
	} else if (((g_abyKeyState['S'] & 0x80) == 0) && (m_byKeyOnS == 1)) {
		m_byKeyOnS = 0;
		byEvent = KEY_UP;
		byCode = 'Y';
		KeyEvPush (byEvent, byCode);
	}

	/* Cキー押した */
	if ((g_abyKeyState['C'] & 0x80) && (m_byKeyOnC == 0)) {
		m_byKeyOnC = 1;
		byEvent = KEY_DOWN;
		byCode = 'C';
		KeyEvPush (byEvent, byCode);

	/* Cキー離した */
	} else if (((g_abyKeyState['C'] & 0x80) == 0) && (m_byKeyOnC == 1)) {
		m_byKeyOnC = 0;
		byEvent = KEY_UP;
		byCode = 'C';
		KeyEvPush (byEvent, byCode);
	}

	/* 1キー押した */
	if ((g_abyKeyState['1'] & 0x80) && (m_byKeyOn1 == 0)) {
		m_byKeyOn1 = 1;
		byEvent = KEY_DOWN;
		byCode = '1';
		KeyEvPush (byEvent, byCode);

	/* 1キー離した */
	} else if (((g_abyKeyState['1'] & 0x80) == 0) && (m_byKeyOn1 == 1)) {
		m_byKeyOn1 = 0;
		byEvent = KEY_UP;
		byCode = '1';
		KeyEvPush (byEvent, byCode);
	}

	/* 2キー押した */
	if ((g_abyKeyState['2'] & 0x80) && (m_byKeyOn2 == 0)) {
		m_byKeyOn2 = 1;
		byEvent = KEY_DOWN;
		byCode = '2';
		KeyEvPush (byEvent, byCode);

	/* 2キー離した */
	} else if (((g_abyKeyState['2'] & 0x80) == 0) && (m_byKeyOn2 == 1)) {
		m_byKeyOn2 = 0;
		byEvent = KEY_UP;
		byCode = '2';
		KeyEvPush (byEvent, byCode);
	}

	/* Enterキー押した */
	if ((g_abyKeyState[VK_RETURN] & 0x80) && (m_byKeyOnReturn == 0)) {
		m_byKeyOnReturn = 1;
		byEvent = KEY_DOWN;
		byCode = VK_RETURN;
		KeyEvPush (byEvent, byCode);

	/* Enterキー離した */
	} else if (((g_abyKeyState[VK_RETURN] & 0x80) == 0) && (m_byKeyOnReturn == 1)) {
		m_byKeyOnReturn = 0;
		byEvent = KEY_UP;
		byCode = VK_RETURN;
		KeyEvPush (byEvent, byCode);
	}

	/* ESCキー押した */
	if ((g_abyKeyState[VK_ESCAPE] & 0x80) && (m_byKeyOnEsc == 0)) {
		m_byKeyOnEsc = 1;
		byEvent = KEY_DOWN;
		byCode = VK_ESCAPE;
		KeyEvPush (byEvent, byCode);

	/* ESCキー離した */
	} else if (((g_abyKeyState[VK_ESCAPE] & 0x80) == 0) && (m_byKeyOnEsc == 1)) {
		m_byKeyOnEsc = 0;
		byEvent = KEY_UP;
		byCode = VK_ESCAPE;
		KeyEvPush (byEvent, byCode);
	}

	/* Tabキー押した */
	if ((g_abyKeyState[VK_TAB] & 0x80) && (m_byKeyOnTab == 0)) {
		m_byKeyOnTab = 1;
		byEvent = KEY_DOWN;
		byCode = VK_TAB;
		KeyEvPush (byEvent, byCode);

	/* Tabキー離した */
	} else if (((g_abyKeyState[VK_TAB] & 0x80) == 0) && (m_byKeyOnTab == 1)) {
		m_byKeyOnTab = 0;
		byEvent = KEY_UP;
		byCode = VK_TAB;
		KeyEvPush (byEvent, byCode);
	}

	/* Ctrlキー押した */
	if ((g_abyKeyState[VK_CONTROL] & 0x80) && (m_byKeyOnCtrl == 0)) {
		m_byKeyOnCtrl = 1;
		byEvent = KEY_DOWN;
		byCode = VK_CONTROL;
		KeyEvPush (byEvent, byCode);

	/* Ctrlキー離した */
	} else if (((g_abyKeyState[VK_CONTROL] & 0x80) == 0) && (m_byKeyOnCtrl == 1)) {
		m_byKeyOnCtrl = 0;
		byEvent = KEY_UP;
		byCode = VK_CONTROL;
		KeyEvPush (byEvent, byCode);
	}

	/* 構え：Z を押し続けたら構え、離したらやめる */
	if (m_byKeyOnZ && !byKeyOnZBack) {
		m_dwTimeZDown = timeGetTime ();
	}
	if (m_byKeyOnZ) {
		if (g_bGuard) {
			if (!CanGuard ()) {
				GuardEnd ();
			}
		} else if (m_dwTimeZDown && (timeGetTime () - m_dwTimeZDown >= GUARD_HOLDTIME) && CanGuard ()) {
			GuardStart ();
		}
	} else if (byKeyOnZBack) {
		m_dwTimeZDown = 0;
		if (g_bGuard || m_bGuardThisHold) {
			m_bGuardThisHold = FALSE;
			GuardEnd ();
			/* 構えをやめた時は、Z を離した時の「攻撃をやめる」はしない */
			if ((byEvent == KEY_UP) && (byCode == 'B')) {
				byEvent = 0;
			}
			for (i = 0; i < m_nKeyEvCount; i ++) {
				if ((m_abyKeyEvEvent[i] == KEY_UP) && (m_abyKeyEvCode[i] == 'B')) {
					m_abyKeyEvEvent[i] = 0;
				}
			}
		}
	}

	/* 方向キー以外のキーが押された・離された？(起きた順に全部処理する) */
	if (m_nKeyEvCount > 0) {
		for (i = 0; i < m_nKeyEvCount; i ++) {
			if (m_abyKeyEvEvent[i]) {
				KeyProcDispatch (m_abyKeyEvEvent[i], m_abyKeyEvCode[i]);
			}
		}
		return;
	}

	/* キーイベント無し？ */
	if (!byEvent) {
		return;
	}
	KeyProcDispatch (byEvent, byCode);
}

/* ========================================================================= */
/* 関数名：KeyProcMoveNext													 */
/* 内容：歩き終わった時の続けて歩く処理										 */
/*		 キー処理は 66ms ごとにしか回らないため、1マス歩き終わってから		 */
/*		 次の一歩が出るまで最大 66ms 待たされ、マスの区切りごとに一瞬止まって	 */
/*		 見えていた。歩き終わったその場で方向キーを見て、すぐ次の一歩を出す。	 */
/* ========================================================================= */
void KeyProcMoveNext(void)
{
	BYTE byCode = 0;

	/* KeyProc と同じ条件の時だけ処理する */
	if (!g_bWindowActive) {
		return;
	}
	if (g_FadeInOutInfo.dwLastTime != 0) {
		return;
	}
	if (g_dwActiveMsgID) {
		return;
	}
	if (g_nMyCharID < 0) {
		return;
	}
	if ((g_FadeInOutInfo.byFadeIn != 0) || (g_FadeInOutInfo.byFadeOut != 0)) {
		return;
	}

	GetKeyboardState (g_abyKeyState);

	/* 複数押されている時は KeyProc と同じく後に判定するキーを優先する */
	if (g_abyKeyState[VK_RIGHT] & 0x80) {
		byCode = VK_RIGHT;
	} else if (g_abyKeyState[VK_LEFT] & 0x80) {
		byCode = VK_LEFT;
	} else if (g_abyKeyState[VK_DOWN] & 0x80) {
		byCode = VK_DOWN;
	} else if (g_abyKeyState[VK_UP] & 0x80) {
		byCode = VK_UP;
	}
	if (byCode == 0) {
		return;
	}

	KeyProcMove (KEY_DOWN, byCode);
}

/* ========================================================================= */
/* 関数名：KeyProcMove														 */
/* 内容：キー入力処理(歩き中)												 */
/* 日付：2003/05/03															 */
/* ========================================================================= */
void KeyProcMove(BYTE byEvent, BYTE byCode)
{
	static BYTE m_byKeyOnReturn = 0;
	static DWORD m_dwTimeLastTarget = 0;
	BOOL bCommand;
	PCHARINFO pChar;

	if (g_byGameState < GAMESTATE_MOVE) {
		return;
	}

	if ((g_byGameState >= GAMESTATE_MOVE) && (!g_pMapInfo->pwMap)) {
		return;
	}

	if (g_aCharInfo.GetSize () == 0) {
		return;
	}

	if (g_nMyCharID < 0) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->byCharState == CHARSTATE_DEAD) {
		KeyProcDead (byEvent, byCode);
		return;
	}

	bCommand = TRUE;
	if ((g_aFadeInOutTbl.GetSize () != 0) ||
		(pChar->byDotMove != 0) ||
		(g_bEnableMove == FALSE) ||
		(g_dwMsgIDCommand != 0)) {
		bCommand = FALSE;
	}

	if ((byEvent == KEY_DOWN) && (byCode == VK_RETURN)) {
		if (!g_bMoving && !g_bSpeech && !m_byKeyOnReturn && !g_dwActiveMsgID) {
			PWNDINFO pWnd = MakeWindowSPEECH ();
			g_WndInfo.Add (pWnd);
			g_bSpeech = TRUE;
		}
		m_byKeyOnReturn = 1;

	/* 自分にエフェクト発動？ */
	} else if ((byEvent == KEY_DOWN) && (byCode == '1') && (g_bEnableMove)) {
		if (!g_bSpeech && !m_byKeyOnReturn && !g_dwActiveMsgID) {
			SendCmdEFCREQ (0, g_byUseEfcID);
		}

	/* 目の前のキャラにエフェクト発動？ */
	} else if ((byEvent == KEY_DOWN) && (byCode == '2') && (g_bEnableMove)) {
		if (!g_bSpeech && !m_byKeyOnReturn && !g_dwActiveMsgID) {
			SendCmdEFCREQ (1, g_byUseEfcID);
		}
	}

	if ((byEvent == KEY_UP) && (byCode == VK_RETURN)) {
		if (!g_bSpeech) {
			m_byKeyOnReturn = 0;
		}
	}

	if (g_bSpeech) {
		return;
	}

	if ((byCode == VK_UP) ||
		(byCode == VK_DOWN) ||
		(byCode == VK_LEFT) ||
		(byCode == VK_RIGHT)) {
		/* 狂酔状態？ */
		if (pChar->byDrunkLV >= CHARSTATEDRUNK_COMA2) {
			if ((rand () % 100) > 40) {
				switch (rand () % 4) {
				case 0:
					byCode = VK_DOWN;
					break;
				case 1:
					byCode = VK_UP;
					break;
				case 2:
					byCode = VK_LEFT;
					break;
				default:
					byCode = VK_RIGHT;
					break;
				}
			}

		/* 泥酔状態？ */
		} else if (pChar->byDrunkLV >= CHARSTATEDRUNK_COMA) {
			switch (byCode) {
			case VK_UP:
				byCode = VK_DOWN;
				break;
			case VK_DOWN:
				byCode = VK_UP;
				break;
			case VK_RIGHT:
				byCode = VK_LEFT;
				break;
			case VK_LEFT:
				byCode = VK_RIGHT;
				break;
			}
		}

		/* 戦場ゲームに参加中？ */
		if (pChar->abySubStatus[0] == SUBGAMEID_WAR) {
			PSUBSTATUS_WAR pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

			/* ウィンドウ表示中ではない？ */
			if (g_dwActiveMsgID == FALSE) {
				/* 移動禁止 */
				byCode = 0;
			}

		/* すごろくに参加中？ */
		} else if (pChar->abySubStatus[0] == SUBGAMEID_SUGOROKU) {
			PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

			/* 観戦者ではない？ */
			if (pSubStatus->byFlgViewer == 0) {
				/* ウィンドウ表示中ではない？ */
				if (g_dwActiveMsgID == FALSE) {
					/* 移動禁止 */
					byCode = 0;
				}
			}
		}
	}

	if ((byEvent == KEY_DOWN) && (byCode == VK_UP)) {
		if (g_aCharInfo[g_nMyCharID]->byDirection != 0) {
			SendCmdCHGDIRECTION (0);

		} else {
			SendCmdCHARPOS (0, TRUE);
		}

	} else if ((byEvent == KEY_DOWN) && (byCode == VK_DOWN)) {
		if (g_aCharInfo[g_nMyCharID]->byDirection != 1) {
			SendCmdCHGDIRECTION (1);

		} else {
			SendCmdCHARPOS (1, TRUE);
		}

	} else if ((byEvent == KEY_DOWN) && (byCode == VK_LEFT)) {
		if (g_aCharInfo[g_nMyCharID]->byDirection != 2) {
			SendCmdCHGDIRECTION (2);

		} else {
			SendCmdCHARPOS (2, TRUE);
		}

	} else if ((byEvent == KEY_DOWN) && (byCode == VK_RIGHT)) {
		if (g_aCharInfo[g_nMyCharID]->byDirection != 3) {
			SendCmdCHGDIRECTION (3);

		} else {
			SendCmdCHARPOS (3, TRUE);
		}
	}

	if ((byEvent == KEY_UP) && (byCode == 'A') && (bCommand == TRUE)) {
		PWNDINFO pWnd = NULL;
		/* 爆弾ゲームに参加中？ */
		if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
			PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
			/* 試合中ではない？ */
			if (!pSubStatus->byFlgEntry) {
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
				pWnd = MakeWindowCOMMAND ();
			/* 試合中 */
			} else {
				/* リモコン中？ */
				if (pSubStatus->byFlgRemocon) {
					SendCmdSubBOMBREMOCON ();
				} else {
					DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
				}
			}

		/* 戦場ゲームに参加中？ */
		} else if (pChar->abySubStatus[0] == SUBGAMEID_WAR) {
			PSUBSTATUS_WAR pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
			switch (pSubStatus->byState) {
			case SUBGAME_WAR_STATE_SELECTTEAM:		/* チーム選択 */
				pWnd = MakeWindowSUBWAR_SELECTTEAM ();
				break;

			case SUBGAME_WAR_STATE_WAIT:			/* 他のプレイヤー待ち */
				pWnd = MakeWindowSUBWAR_WAIT ();
				break;

			case SUBGAME_WAR_STATE_SELECTRULE:		/* ルール選択 */
				pWnd = MakeWindowSUBWAR_SELECTRULE ();
				break;

			case SUBGAME_WAR_STATE_SELECTJOB:		/* 職業選択 */
				pWnd = MakeWindowSUBWAR_SELECTJOB ();
				break;

			case SUBGAME_WAR_STATE_SELECTTURN:		/* 行動順序選択 */
				pWnd = MakeWindowSUBWAR_SELECTTURN ();
				break;

			case SUBGAME_WAR_STATE_TURN:			/* 行動選択 */
				pWnd = MakeWindowSUBWAR_TURN ();
				break;

			case SUBGAME_WAR_STATE_SELECTDIFENCE:	/* 防御行動選択 */
				switch (pSubStatus->byJob) {
				case SUBGAME_WAR_JOB_FIGHTER:		/* 戦士 */
					pWnd = MakeWindowSUBWAR_TURN_DIFENCE_FIGHTER ();
					break;
				case SUBGAME_WAR_JOB_MARTIAL:		/* 武道家 */
					pWnd = MakeWindowSUBWAR_TURN_DIFENCE_MARTIAL ();
					break;
				case SUBGAME_WAR_JOB_WITCH:			/* 魔法使い */
					pWnd = MakeWindowSUBWAR_TURN_DIFENCE_WITCH ();
					break;
				case SUBGAME_WAR_JOB_PRIEST:		/* 僧侶 */
					pWnd = MakeWindowSUBWAR_TURN_DIFENCE_PRIEST ();
					break;
				}
				break;
			}

		/* すごろくに参加中？ */
		} else if (pChar->abySubStatus[0] == SUBGAMEID_SUGOROKU) {
			PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
			switch (pSubStatus->byState) {
			case SUBGAME_SUGOROKU_STATE_JOINMODE:		/* 参加モード選択 */
				pWnd = MakeWindowSUBSUGOROKU_JOINMODE ();
				break;

			case SUBGAME_SUGOROKU_STATE_COMMAND:		/* 行動選択 */
				pWnd = MakeWindowSUBSUGOROKU_COMMAND ();
				break;

			case SUBGAME_SUGOROKU_STATE_IDLE:			/* 平常状態 */
				pWnd = MakeWindowSUBSUGOROKU_IDLE ();
				break;
			}

		} else if (pChar->dwTimeTurnMagicLV == 0) {
			/* 戦闘マップ？ */
			if (g_pMapInfo->byFlg & MAPFLG_BATTLE) {
				/* 戦闘モード？ */
				if (g_bBattleMode) {
					/* ターンゲージ溜まり中？(ジャスト・早撃ち) */
					if (ShotTurnGage (pChar)) {
					/* 殴った直後？(ジャストに押し遅れた X でコマンドを開かない。
					   1周目の後半はコマンドを開けるように、待つのは1周の半分まで) */
					} else if (pChar->dwTimeTurnLV && (timeGetTime () - g_dwTurnGageContinue <
							min ((DWORD)TURNGAGE_LATETIME, GetTurnGageTime (pChar) / TURNGAGE_LAPMAX / 2))) {
					} else {
						/* ゲージ1周目の途中なら戦闘コマンドを開く(連続攻撃中に特技・道具を選べるように) */
						pWnd = MakeWindowBATTLE_COMMAND ();
					}
				} else {
					pWnd = MakeWindowCOMMAND ();
				}
			} else {
				pWnd = MakeWindowCOMMAND ();
			}
			/* コマンドを開いた時だけ決定音(ゲージで殴った時は攻撃音とかぶるので鳴らさない) */
			if (pWnd) {
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
			}
		}

		if (pWnd) {
			g_WndInfo.Add (pWnd);
			g_dwActiveMsgID = pWnd->dwID;
		}

	/* Cキー：戦闘モード中は、殴らずに戦闘コマンドを「特技」にカーソルを合わせて開く
	   (目の前に敵がいるとゲージが溜まっていて X は攻撃になるので、特技・道具はこちらから) */
	} else if ((byEvent == KEY_UP) && (byCode == 'C') && (bCommand == TRUE)) {
		if ((pChar->abySubStatus[0] == 0) && (pChar->dwTimeTurnMagicLV == 0) &&
			(g_pMapInfo->byFlg & MAPFLG_BATTLE) && g_bBattleMode) {
			PWNDINFO pWnd = MakeWindowBATTLE_COMMAND ();
			pWnd->nData1 = 1;
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
			g_WndInfo.Add (pWnd);
			g_dwActiveMsgID = pWnd->dwID;
		}

	} else if ((byEvent == KEY_UP) && (byCode == 'Y') && (bCommand == TRUE)) {
		PWNDINFO pWnd = NULL;
		/* 爆弾ゲームに参加中？ */
		if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
			PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
			PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
			/* ゲームマスター？ */
			if (pSubStatus->byFlgGameMaster) {
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
				/* 試合中ではない？ */
				if (!pSubStatus->byFlgEntry) {
					pWnd = MakeWindowSUBGAME_BOMBSET ();
				}
			}
		} else {
			BOOL bResult;

			/* 調べる・話す・扉 */
			bResult = CmdProcSKey ();
			if (bResult) {
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
			}
		}

		if (pWnd) {
			g_WndInfo.Add (pWnd);
			g_dwActiveMsgID = pWnd->dwID;
		}

	/* ターゲット指定？ */
	} else if ((byEvent == KEY_UP) && (byCode == 'B') && (!g_dwMsgIDCommand) && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		/* 爆弾ゲームに参加中？ */
		if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
			PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
			if (pSubStatus->byFlgEntry) {
				SendCmdSubSETBOMB ();
			}

		/* 戦場ゲームに参加中？ */
		} else if (pChar->abySubStatus[0] == SUBGAMEID_WAR) {

		/* ターンゲージ溜まり中？(連続攻撃もここで止める) */
		} else if ((pChar->byTurnLV > 0) || pChar->dwTimeTurnLV) {
			/* 目の前の敵で自動で溜め直さないように覚えておく(離れるか別の敵が来るまで) */
			g_dwAutoGageSuppress = pChar->dwAtackTblID;
			CancelTurnGage (pChar);
			SAFE_DELETE (g_pTargetSelectInfo);
			SendCmdTARGETCHAR ((DWORD)-1);

		/* 詠唱中？ */
		} else if (pChar->dwTimeTurnMagicLV) {
			/* 詠唱中止 */
			pChar->dwTimeTurnMagicLV = 0;
			pChar->byTurnMagicLV = 0;
			CancelTurnGage (pChar);
			SendCmdEFECTCHG (0, TRUE);
			SAFE_DELETE (g_pTargetSelectInfo);
			AddSysMsg ("特技の実行を中断しました。", RGB (255, 255, 255));

		} else {
			DWORD dwTime = timeGetTime ();
			if (m_dwTimeLastTarget < dwTime - 2000) {
				m_dwTimeLastTarget = dwTime;
				int nScore = GetFrontScoreChar ();
				/* 目の前が得点板（雪合戦場の HP パネルなど）？ */
				if (nScore >= 0) {
					/* 昔のターゲット要求で、パネルに名前を付ける（付いていれば外す） */
					SendCmdTARGETCHAR (g_aCharInfo[nScore]->dwTblID);
				} else {
					/* ついていく要求 */
					SendCmdCHAR_TAIL (-1);
				}
				CancelTurnGage (pChar);
				SAFE_DELETE (g_pTargetSelectInfo);
			}
		}
	}

	if ((byEvent == KEY_UP) && (byCode == VK_TAB)) {
		g_byViewMode ++;
		if (g_byViewMode >= 4) {
			g_byViewMode = 0;
		}
		SaveIniOption ();
	}
	if ((byEvent == KEY_UP) && (byCode == VK_CONTROL)) {
		/* 戦闘マップ？ */
		if (g_pMapInfo->byFlg & MAPFLG_BATTLE) {
			if (g_bBattleMode) {
				g_bBattleMode = FALSE;
				AddSysMsg ("戦闘モードを解除しました。", RGB (255, 255, 255));
			} else {
				g_bBattleMode = TRUE;
				AddSysMsg ("戦闘モードに切り替えました。", RGB (255, 255, 255));
			}
			/* 傭兵は戦闘モードの時だけ自分から戦うので、サーバーへ知らせる */
			SendCmdBATTLEMODE (g_bBattleMode);
		}
	}
}


/* ========================================================================= */
/* 関数名：	KeyProcDead														 */
/* 内容：	キー入力処理(死亡中)											 */
/* 日付：	2005/10/31														 */
/* ========================================================================= */

void KeyProcDead(BYTE byEvent, BYTE byCode)
{
	static BYTE m_byKeyOnReturn = 0;
	static DWORD m_dwTimeLastTarget = 0;
	BOOL bCommand;
	PCHARINFO pChar;

	pChar = g_aCharInfo[g_nMyCharID];
	bCommand = TRUE;
	if ((g_aFadeInOutTbl.GetSize () != 0) ||
		(pChar->byDotMove != 0) ||
		(g_bEnableMove == FALSE) ||
		(g_dwMsgIDCommand != 0)) {
		bCommand = FALSE;
	}

	if ((byEvent == KEY_DOWN) && (byCode == VK_RETURN)) {
		if (!g_bMoving && !g_bSpeech && !m_byKeyOnReturn && !g_dwActiveMsgID) {
			PWNDINFO pWnd = MakeWindowSPEECH ();
			g_WndInfo.Add (pWnd);
			g_bSpeech = TRUE;
		}
		m_byKeyOnReturn = 1;
	}

	if ((byEvent == KEY_UP) && (byCode == VK_RETURN)) {
		if (!g_bSpeech) {
			m_byKeyOnReturn = 0;
		}
	}

	if (g_bSpeech) {
		return;
	}

	if ((byEvent == KEY_UP) && (byCode == 'A') && (bCommand == TRUE)) {
		PWNDINFO pWnd = NULL;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CURSOR08]);
		pWnd = MakeWindowDEAD_COMMAND ();
		g_WndInfo.Add (pWnd);
		g_dwActiveMsgID = pWnd->dwID;
	}

	if ((byEvent == KEY_UP) && (byCode == VK_TAB)) {
		g_byViewMode ++;
		if (g_byViewMode >= 4) {
			g_byViewMode = 0;
		}
		SaveIniOption ();
	}
}

/* ========================================================================= */
/* 関数名：KeyProcLOGINMENU													 */
/* 内容：キー入力処理(ログインメニュー)										 */
/* 日付：2003/05/04															 */
/* ========================================================================= */
void KeyProcLOGINMENU(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'A')) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		switch (pWnd->nData1) {
		case 0:	/* 冒険をする */
			if (!pChar->szName[0]) {
				break;
			}
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
			{
				int nNo = GetWindowID (g_dwMsgIDMENU);
				EraseWindow ((PWNDINFO)g_WndInfo[nNo]);

				nNo = GetWindowID (g_dwMsgIDMENUCHARINFO);
				EraseWindow ((PWNDINFO)g_WndInfo[nNo]);

				SendCmdCHGSTATE (GAMESTATE_MOVE);
				g_dwMsgIDMENU = 0;
			}
			break;

		case 1:	/* 冒険の書をつくる */
			if (pChar->szName[0]) {
				break;
			}
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
			{
				PWNDINFO pWndNew = MakeWindowLOGINMENUCHARMAKE ();
				pWndNew->pParent = pWnd;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				SetFocus (g_hWndName);
				InvalidateRect (g_hWndMain, NULL, FALSE);
			}
			break;

		case 2:	/* 冒険の書をけす */
			if (!pChar->szName[0]) {
				break;
			}
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_CONFIRM]);
			{
				PWNDINFO pWndNew = MakeWindowDELETECONFIRM ();
				pWndNew->nData1 = 1;
				pWndNew->pParent = pWnd;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < 2)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMAND													 */
/* 内容：キー入力処理(コマンド)												 */
/* 日付：2003/05/05															 */
/* ========================================================================= */
void KeyProcCOMMAND(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (g_dwMsgIDCommand);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		switch (pWnd->nData1) {
		/* 話す */
		case 0:
			CmdProcHanasu (pWnd);
			break;

		/* 魔法 */
		case 1:
			{
				PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

				/* 詠唱中？ */
				if (pChar->dwTimeTurnMagicLV) {
					break;
				}
				if (g_aMagicInfoWalk.GetSize () == 0) {
					PWNDINFO pWndTmp;
					pWndTmp = MakeWindowMSG ("現在使える特技はありません。@2", FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					break;
				}

				PWNDINFO pWndNew = MakeWindowCOMMANDMAGIC ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 強さ */
		case 2:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDSTATUS ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 道具 */
		case 3:
			{
				if (GetItemCount (g_nMyCharID) == 0) {
					PWNDINFO pWndTmp;
					pWndTmp = MakeWindowMSG ("何も持っていません。@2", FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					break;
				}

				PWNDINFO pWndNew = MakeWindowCOMMANDITEM ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);

				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;

				/* 一覧の横にカーソルのアイテムの詳細を出す */
				/* （以前は職業の「アイテム情報を見られる」フラグがある時だけだった） */
				MakeWindowITEMINFOList (pWndNew);
			}
			break;

		/* 装備 */
		case 4:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDEQU ();
				PWNDINFO pWndSub = MakeWindowCOMMANDEQUSub ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				pWndNew->pChild = pWndSub;
				pWndSub->pParent = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_WndInfo.Add (pWndSub);
//				g_dwActiveMsgID = ((PWNDINFO)(pWndNew->pChild))->dwID;
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 調べる */
		case 5:
			CmdProcSiraberu (pWnd);
			break;

		/* 仲間 */
		case 6:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDPARTY ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 取り引き */
		case 7:
			{
				/* 目の前の人か、HPが見えている範囲の仲間(傭兵も) */
				DWORD adwTblID[10];
				int nCount = GetTradeTargets (adwTblID, 10);
				if (nCount == 0) {
					PWNDINFO pWndTmp;
					pWndTmp = MakeWindowMSG ("取り引きする相手がいません。@2", FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;
					break;
				}
				PWNDINFO pWndNew = MakeWindowCOMMANDTRADE ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* 扉 */
		case 8:
			{
				g_bEnableMove = FALSE;
				SendCmdOPENDOOR ();

				int nNo = GetWindowID (g_dwMsgIDCommand);
				if (nNo >= 0) {
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
					g_dwMsgIDCommand = 0;
					g_bDraw = TRUE;
				}
			}
			break;

		/* その他 */
		case 9:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDETC ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 - 1 > 0)) {
			pWnd->nData1 -= 2;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 + 1 < 9)) {
			pWnd->nData1 += 2;
			bMove = TRUE;

		} else if ((byCode == VK_LEFT) && (pWnd->nData1 % 2)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_RIGHT) && (pWnd->nData1 % 2 == 0)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDETC												 */
/* 内容：キー入力処理(その他コマンド)										 */
/* 日付：2003/08/03															 */
/* ========================================================================= */
void KeyProcCOMMANDETC(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		switch (pWnd->nData1) {
		/* ログ保存 */	
		case 0:
			{
				SaveMsgLog ();
				PWNDINFO pWndTmp;
				pWndTmp = MakeWindowMSG ("メッセージログを保存しました。@2", FALSE);
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 自己紹介メッセージ確認 */
		case 1:
			{
				PWNDINFO pWndTmp;
				if ((g_aCharInfo[g_nMyCharID]->pszNpcSpeech) && (strlen (g_aCharInfo[g_nMyCharID]->pszNpcSpeech))) {
					pWndTmp = MakeWindowMSG (g_aCharInfo[g_nMyCharID]->pszNpcSpeech, TRUE);
				} else {
					pWndTmp = MakeWindowMSG ("自己紹介メッセージは登録されていません。@2", FALSE);
				}
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 自己紹介メッセージ設定 */
		case 2:
			{
				PWNDINFO pWndNew = MakeWindowSPEECHEDIT ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* 名前の色設定 */
		case 3:
			{
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
				pWnd->byAnime = 1;
				pWnd->dwTimeDrawStart = 0;
				g_bDraw = TRUE;

				PWNDINFO pWndNew = MakeWindowNAMECOLOR ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* パスワードの変更 */
		case 4:
			{
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
				pWnd->byAnime = 1;
				pWnd->dwTimeDrawStart = 0;
				g_bDraw = TRUE;

				PWNDINFO pWndNew = MakeWindowCHGPASSWORD ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* 管理人呼び出し */
		case 5:
			{
				PWNDINFO pWndTmp, pWndTmp2;
				pWndTmp = MakeWindowCOMMANDETC_CONF ();
				g_WndInfo.Add (pWndTmp);
				pWndTmp->pParent = pWnd;
				pWnd->pChild = pWndTmp;

				pWndTmp2 = MakeWindowMSG ("管理人呼び出しをしますか？@2", TRUE);
				pWndTmp2->pParent = pWndTmp;
				pWndTmp->pChild = pWndTmp2;
				g_WndInfo.Add (pWndTmp2);

				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 人の多い場所（チャットの /where と同じ。返事はメッセージログに出る） */
		case 6:
			SendCmdMSGCMD_0 (MSGCMD_WHERE);
			g_bDraw = TRUE;
			break;

		/* オプション */
		case 7:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDOPTION ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDETC_CONF											 */
/* 内容：キー入力処理(管理人呼び出し確認)									 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
void KeyProcCOMMANDETC_CONF(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		if (pWnd->nData1 == 0) {
			EraseWindow ((PWNDINFO)pWnd->pChild);

			PWNDINFO pWndTmp;
			pWndTmp = MakeWindowMSG ("管理人呼び出しをしました。@1しばらくその場でお待ちください。@0"
					"多忙の為すぐには来られない場合がありま　す。@1ご了承くださいませ。@2", FALSE);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			SendCmdGMCALL ();
			g_bDraw = TRUE;

		} else {
			EraseWindow (pWnd);
			g_bDraw = TRUE;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDITEM												 */
/* 内容：キー入力処理(道具コマンド)											 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
void KeyProcCOMMANDITEM(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->byAnime = 1;
		pWnd->dwTimeDrawStart = 0;
		g_bDraw = TRUE;

		PWNDINFO pWndNew = MakeWindowCOMMANDITEMPROC ();
		pWndNew->pParent = pWnd;
		pWnd->pChild = pWndNew;
		g_WndInfo.Add (pWndNew);
		g_dwActiveMsgID = pWndNew->dwID;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;

			/* アイテムの詳細を出し直す */
			UpdateWindowITEMINFOList (pWnd);
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDITEMBATTLE											 */
/* 内容：キー入力処理(道具コマンド(戦闘マップ))								 */
/* 日付：2005/02/01															 */
/* ========================================================================= */
void KeyProcCOMMANDITEMBATTLE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->byAnime = 1;
		pWnd->dwTimeDrawStart = 0;
		g_bDraw = TRUE;

		int i;
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
			  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
		for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
			PCHARINFO pCharTmp = g_aCharInfo[i];
			/* 目の前にいる？ */
			if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (dwY == pCharTmp->dwY)) {
				if ((pCharTmp->byNpcType == 0) ||
					(pCharTmp->byNpcType == NPCTYPE_BATTLE1) ||
					(pCharTmp->byNpcType == NPCTYPE_BATTLE2) ||
					(pCharTmp->byNpcType == NPCTYPE_MERCENARY)
					) {
					if (pCharTmp != pChar) {
						break;
					}
				}
			}
		}
		if (i < g_aCharInfo.GetSize ()) {
			PWNDINFO pWndNew = MakeWindowCOMMANDITEMCHARBATTLE ();
			pWndNew->pParent = pWnd;
			pWnd->pChild = pWndNew;
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;

		} else {
			PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
			BYTE byNo = pWnd->nData1;
			PITEMINFO pItem = g_aItemInfo[pChar->waItem[byNo]];

			SendCmdUSEITEM (byNo, pChar->dwTblID);
			/* コマンドウィンドウを閉じる */
			int nNo = GetWindowID (g_dwMsgIDCommand);
			if (nNo >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
				g_dwMsgIDCommand = 0;
				g_bDraw = TRUE;
			}
			CancelTurnGage (pChar);
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDITEMPROC											 */
/* 内容：キー入力処理(道具処理選択)											 */
/* 日付：2003/08/26															 */
/* ========================================================================= */
void KeyProcCOMMANDITEMPROC(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;

		PWNDINFO pWndTmp = NULL, pWndTmp2 = NULL;
		switch (pWnd->nData1) {
		/* 使う */
		case 0:
			{
				int i;
				PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
				DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
					  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
				for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
					PCHARINFO pCharTmp = g_aCharInfo[i];
					/* 目の前にいる？ */
					if (CheckCharX (pCharTmp, pCharTmp->dwX, dwX) && (dwY == pCharTmp->dwY)) {
						if ((pCharTmp->byNpcType == 0) ||
							(pCharTmp->byNpcType == NPCTYPE_BATTLE1) ||
							(pCharTmp->byNpcType == NPCTYPE_BATTLE2) ||
							(pCharTmp->byNpcType == NPCTYPE_MERCENARY)
							) {
							if (pCharTmp != pChar) {
								break;
							}
						}
					}
				}
				if (i < g_aCharInfo.GetSize ()) {
					PWNDINFO pWndNew = MakeWindowCOMMANDITEMCHAR ();
					pWndNew->pParent = pWnd;
					pWnd->pChild = pWndNew;
					g_WndInfo.Add (pWndNew);
					g_dwActiveMsgID = pWndNew->dwID;
					break;
				}

				PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
				BYTE byNo = pWndParent->nData1;
				PITEMINFO pItem = g_aItemInfo[pChar->waItem[byNo]];

				SendCmdUSEITEM (byNo, pChar->dwTblID);
				/* コマンドウィンドウを閉じる */
				int nNo = GetWindowID (g_dwMsgIDCommand);
				if (nNo >= 0) {
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
					g_dwMsgIDCommand = 0;
					g_bDraw = TRUE;
				}
			}
			break;

		/* 捨てる */
		case 1:
			{
				PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
				PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
				BYTE byNo = pWndParent->nData1;
				PITEMINFO pItem = g_aItemInfo[pChar->waItem[byNo]];
				char szTmp[128];
				ZeroMemory (szTmp, sizeof (szTmp));

				int i;
				for (i = 0; i < 10; i ++) {
					if (pChar->waEqu[i] == byNo) {
						wsprintf (szTmp, "装備しているので捨てられません。@2");
						pWndTmp = MakeWindowMSG (szTmp, FALSE);
						break;
					}
				}
				if (i < 10) {
					break;
				}
				/* 捨てられる判定 */
				if (pItem->byFlg & ITEMFLG_DROP) {
					wsprintf (szTmp, "%sを捨てるなんてとんでもない！@2", pItem->szName);
					pWndTmp = MakeWindowMSG (szTmp, FALSE);

				} else {
					/* 確認ウィンドウ作成 */
					pWndTmp = MakeWindowCONF (16 * 13, 16 * 10, IDCONFIRMTYPE_DROP);
					pWndTmp->byData1 = byNo;
					pWndTmp->nData1 = 1;
					pWndTmp->pParent = pWnd;
					pWnd->pChild = pWndTmp;
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;

					wsprintf (szTmp, "%sを捨てますか？@2", pItem->szName);
					pWndTmp2 = MakeWindowMSG (szTmp, FALSE);
					pWndTmp2->pParent = pWndTmp;
					pWndTmp->pChild = pWndTmp2;
					g_WndInfo.Add (pWndTmp2);
					pWndTmp = NULL;
				}
			}
			break;

		default:
			pWndTmp = MakeWindowMSG ("未実装。@2", FALSE);
			break;
		}

		if (pWndTmp) {
			g_bDraw = TRUE;
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDITEMCHAR											 */
/* 内容：キー入力処理(道具使う相手選択)										 */
/* 日付：2003/11/11															 */
/* ========================================================================= */
void KeyProcCOMMANDITEMCHAR(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;

		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		BYTE byNo = ((PWNDINFO)pWndParent->pParent)->nData1;
		if (pWndParent->nType == WNDTYPE_COMMANDITEMBATTLE) {
			byNo = (BYTE)pWndParent->nData1;
		}
		PITEMINFO pItem = g_aItemInfo[pChar->waItem[byNo]];

		SendCmdUSEITEM (byNo, g_aUseCharInfo[pWnd->nData1]->dwTblID);
		CancelTurnGage (pChar);

		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDOPTION												 */
/* 内容：キー入力処理(オプションコマンド)									 */
/* 日付：2003/09/07															 */
/* ========================================================================= */
void KeyProcCOMMANDOPTION(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		switch (pWnd->nData1) {
		/* 発言徐々に消えるON/OFF */
		case 0:
			{
				PWNDINFO pWndTmp;
				if (g_bSpeechView) {
					g_bSpeechView = FALSE;
					pWndTmp = MakeWindowMSG ("発言は１０秒間消えません。@2", FALSE);
				} else {
					g_bSpeechView = TRUE;
					pWndTmp = MakeWindowMSG ("発言は５秒後から徐々に消えます。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* 音楽ON/OFF */
		case 1:
			{
				PWNDINFO pWndTmp;
				if (g_bBGMONOFF) {
					g_bBGMONOFF = FALSE;
					StopBGM ();
					pWndTmp = MakeWindowMSG ("音楽停止しました。@2", FALSE);
				} else {
					g_bBGMONOFF = TRUE;
					PlayBGM (0);
					pWndTmp = MakeWindowMSG ("音楽再生再開しました。@2", FALSE);
				}
				/* アカウントごとに覚えてもらう（次にこのアカウントで入った時もこの設定） */
				SendCmdWEBACCOUNT (WEBACC_OPTBGM, g_bBGMONOFF ? OPTBGM_ON : OPTBGM_OFF);
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* 吹き出し効果音ON/OFF */
		case 2:
			{
				PWNDINFO pWndTmp;
				if (g_bComEfcONOFF) {
					g_bComEfcONOFF = FALSE;
					pWndTmp = MakeWindowMSG ("吹き出し効果音をOFFにしました。@2", FALSE);
				} else {
					g_bComEfcONOFF = TRUE;
					pWndTmp = MakeWindowMSG ("吹き出し効果音をONにしました。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* 吹き出しウィンドウON/OFF */
		case 3:
			if (g_bBalloonWindowView) {
				g_bBalloonWindowView = FALSE;
				ShowWindow (g_hWndBalloon, SW_HIDE);
			} else {
				g_bBalloonWindowView = TRUE;
				ShowWindow (g_hWndBalloon, SW_SHOWNOACTIVATE);
			}
			break;

		/* CPUフル活用 */
		case 4:
			{
				PWNDINFO pWndTmp;
				if (g_bCpuMax) {
					g_bCpuMax = FALSE;
					pWndTmp = MakeWindowMSG ("CPUのんびりモードに切り替えました。@2", FALSE);
				} else {
					g_bCpuMax = TRUE;
					pWndTmp = MakeWindowMSG ("CPUフル活動モードに切り替えました。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* 音量(曲) */
		case 5:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDOPTIONVOLMUSIC ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* 音量(効果音) */
		case 6:
			{
				PWNDINFO pWndNew = MakeWindowCOMMANDOPTIONVOLSOUND ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		/* 天候の描画 */
		case 7:
			{
				PWNDINFO pWndTmp;
				if (g_bWeatherDraw) {
					g_bWeatherDraw = FALSE;
					pWndTmp = MakeWindowMSG ("天候の描画をやめます。@2", FALSE);
				} else {
					g_bWeatherDraw = TRUE;
					pWndTmp = MakeWindowMSG ("天候を描画します。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* タイトル画面のスキップ */
		case 8:
			{
				PWNDINFO pWndTmp;
				if (g_bTitleSkip) {
					g_bTitleSkip = FALSE;
					pWndTmp = MakeWindowMSG ("タイトル画面を表示します。@2", FALSE);
				} else {
					g_bTitleSkip = TRUE;
					pWndTmp = MakeWindowMSG ("ログイン画面から表示します。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;

		/* 発言の音ON/OFF（SBOP2 のように、誰かが発言したら音を鳴らす） */
		case 9:
			{
				PWNDINFO pWndTmp;
				if (g_bChatSound) {
					g_bChatSound = FALSE;
					pWndTmp = MakeWindowMSG ("発言の音をOFFにしました。@2", FALSE);
				} else {
					g_bChatSound = TRUE;
					pWndTmp = MakeWindowMSG ("発言の音をONにしました。@2", FALSE);
				}
				g_bDraw = TRUE;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
			}
			break;
		}
		/* 変えた設定をその場で保存する（ブラウザ版は終了時の保存が来ないため） */
		SaveIniOption ();

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDOPTIONVOLSOUND										 */
/* 内容：キー入力処理(効果音音量設定)										 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
void KeyProcCOMMANDOPTIONVOLSOUND(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		g_byVolSound = (BYTE)pWnd->nData1;
		DXAudio.SetVolSecoundary ((long)anVolTbl[g_byVolSound]);
		SaveIniOption ();
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if (byCode == VK_UP) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_DOWN) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;

		} else if ((byCode == VK_LEFT) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_RIGHT) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDOPTIONVOLMUSIC										 */
/* 内容：キー入力処理(曲音量設定)											 */
/* 日付：2003/11/30															 */
/* ========================================================================= */
void KeyProcCOMMANDOPTIONVOLMUSIC(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		g_byVolMusic = (BYTE)pWnd->nData1;
		g_LibMusic.SetRelativeVolume (anVolTblMIDI[g_byVolMusic]);
		SaveIniOption ();

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if (byCode == VK_UP) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_DOWN) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;

		} else if ((byCode == VK_LEFT) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_RIGHT) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDSTATUS												 */
/* 内容：キー入力処理(強さコマンド)											 */
/* 日付：2003/09/28															 */
/* ========================================================================= */
void KeyProcCOMMANDSTATUS(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		/* ステータスの次は、覚えている特技の一覧を出す */
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;

		EraseWindow (pWnd);
		if (pWndParent) {
			PWNDINFO pWndNew = MakeWindowSTATUSMAGIC ();
			pWndNew->pParent = pWndParent;
			pWndParent->pChild = pWndNew;
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;
		}
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：	KeyProcMAKER													 */
/* 内容：	キー入力処理(作り手（レシピ）：作れる物の一覧・確認)			 */
/*			一覧：上下で選び、決定で確認へ、取消で閉じる					 */
/*			確認：はい→作成要求を出す、いいえ・取消→一覧へ戻る				 */
/* ========================================================================= */
void KeyProcMAKER(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	PWNDINFO pWndList = NULL;
	int i, nNo;

	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}
	if ((g_nProcMakerID < 0) || (g_nProcMakerID >= MAX_MAKER)) {
		/* 作り手がいない（画面の外から閉じられた） */
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		return;
	}

	/* 一覧のウィンドウ */
	for (i = 0; i < g_WndInfo.GetSize (); i ++) {
		if ((g_WndInfo[i]->nType == WNDTYPE_MAKER) && (g_WndInfo[i]->nType2 == MAKERWND_LIST)) {
			pWndList = g_WndInfo[i];
			break;
		}
	}
	if (pWndList == NULL) {
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		return;
	}

	/* 作成要求の返事待ち（一定時間たったらあきらめる） */
	if (g_bProcMakerWait) {
		if (timeGetTime () > g_dwProcMakerWaitTime + MAKER_WAITMAX) {
			g_bProcMakerWait = FALSE;
			MakerSetMsg (pWndList, "うまく伝わらなかったようです。もう一度お願いします。@2");
			g_dwActiveMsgID = pWndList->dwID;
			g_bDraw = TRUE;
		}
		return;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		if (pWnd->nType2 == MAKERWND_CONF) {
			/* 確認をやめて一覧へ戻る */
			EraseWindow (pWnd);
			MakerSetTalkMsg (pWndList);
			g_dwActiveMsgID = pWndList->dwID;
			pWndList->dwTimeDrawStart = 0;
		} else {
			/* 閉じる（台詞と横の詳細も一緒に消える） */
			EraseWindow (pWnd);
		}
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		if (pWnd->nType2 == MAKERWND_LIST) {
			int nRecipe = MakerGetCursorRecipe (pWnd);
			if (nRecipe < 0) {
				return;
			}
			PRECIPEINFO pRecipe = &g_aRecipeInfo[nRecipe];
			PITEMINFO pItem = g_aItemInfo[pRecipe->wItem];
			char szTmp[256];

			/* 足りない時は、確認せずに断る。持ち物だけで足りず預かり所の中身がまだ届いていない時は、
			   届くまで確認を出さない（通信が遅い時に決定キーを連打すると、材料が無いのに確認が出ていた） */
			if (!MakerCanMake (nRecipe)) {
				if (!g_bProcMakerLeaveOK && (g_aCharInfo[g_nMyCharID]->dwGold >= pRecipe->dwGold)) {
					MakerSetMsg (pWnd, "預かり所を確かめています。@1少々お待ちください。@2");
				} else if (g_aCharInfo[g_nMyCharID]->dwGold < pRecipe->dwGold) {
					MakerSetMsg (pWnd, "手数料が足りないようです。@2");
				} else if (g_aMakerInfo[g_nProcMakerID].szMsgLack[0]) {
					MakerSetMsg (pWnd, g_aMakerInfo[g_nProcMakerID].szMsgLack);
				} else {
					MakerSetMsg (pWnd, "材料が足りないようです。@2");
				}
				g_dwActiveMsgID = pWnd->dwID;
				g_bDraw = TRUE;
				return;
			}

			if (pRecipe->byCount > 1) {
				wsprintf (szTmp, "%sを%d個、作ってもらいますか？@1手数料は%dGです。@2", pItem->szName, (int)pRecipe->byCount, (int)pRecipe->dwGold);
			} else {
				wsprintf (szTmp, "%sを作ってもらいますか？@1手数料は%dGです。@2", pItem->szName, (int)pRecipe->dwGold);
			}
			MakerSetMsg (pWnd, szTmp);

			PWNDINFO pWndConf = MakeWindowMAKER_CONF (nRecipe);
			g_WndInfo.Add (pWndConf);
			g_dwActiveMsgID = pWndConf->dwID;

		} else {
			/* 確認：はい／いいえ */
			int nRecipe = (int)pWnd->dwData;
			BOOL bYes = (pWnd->nData1 == 0);

			EraseWindow (pWnd);
			g_dwActiveMsgID = pWndList->dwID;
			if (bYes && (nRecipe >= 0) && (nRecipe < MAX_RECIPE) && (g_aRecipeInfo[nRecipe].byState == RECIPESTATE_ON)) {
				SendCmdRECIPEMAKE ((BYTE)g_nProcMakerID, (WORD)nRecipe);
				g_bProcMakerWait = TRUE;
				g_dwProcMakerWaitTime = timeGetTime ();
				MakerSetMsg (pWndList, "少々お待ちください。@2");
			} else {
				MakerSetTalkMsg (pWndList);
			}
			pWndList->dwTimeDrawStart = 0;
		}
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if (pWnd->nType2 == MAKERWND_CONF) {
			if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
				pWnd->nData1 --;
				bMove = TRUE;
			} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
				pWnd->nData1 ++;
				bMove = TRUE;
			}

		} else {
			int anList[MAX_RECIPE];
			int nCount = MakerGetRecipeList (g_nProcMakerID, anList, MAX_RECIPE);
			int nIndex = (int)pWnd->byData1 + pWnd->nData1;
			int nIndexNew = nIndex;

			if ((byCode == VK_UP) && (nIndex > 0)) {
				nIndexNew = nIndex - 1;
			} else if ((byCode == VK_DOWN) && (nIndex < nCount - 1)) {
				nIndexNew = nIndex + 1;
			} else if (byCode == VK_LEFT) {
				nIndexNew = 0;
			} else if (byCode == VK_RIGHT) {
				nIndexNew = max (nCount - 1, 0);
			}
			if (nIndexNew != nIndex) {
				MakerScrollFit (pWnd, nIndexNew, nCount);
				UpdateWindowITEMINFOList (pWnd);
				bMove = TRUE;
			}
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：	KeyProcSTATUSMAGIC												 */
/* 内容：	キー入力処理(強さ：覚えている特技)								 */
/* 			上下で特技を選ぶと下の欄に説明が出る。見るだけで使いはしない	 */
/* 日付：	2026/11/07														 */
/* ========================================================================= */
void KeyProcSTATUSMAGIC(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		/* コマンドメニューに戻る */
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		/* 全部閉じる（「強さ」で決定を押した時の元の動きと同じ） */
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if (pWnd->nData2 <= 1) {
			/* 動かす先が無い */
		} else if (byCode == VK_UP) {
			pWnd->nData1 = (pWnd->nData1 > 0) ? pWnd->nData1 - 1 : pWnd->nData2 - 1;
			bMove = TRUE;

		} else if (byCode == VK_DOWN) {
			pWnd->nData1 = (pWnd->nData1 < pWnd->nData2 - 1) ? pWnd->nData1 + 1 : 0;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;		/* 説明欄も描き直す */
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcMSG														 */
/* 内容：キー入力処理(メッセージ)											 */
/* 日付：2003/06/11															 */
/* ========================================================================= */
void KeyProcMSG(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	if (pWnd->nData2 == 0) {
		return;
	}
	/* 復活位置の「はい・いいえ」を選ぶまでは進めない */
	if (pWnd->nData2 == MSGSTATE_STARTPOSCONF) {
		return;
	}

	if ((byEvent == KEY_UP) && ((byCode == 'A') || (byCode == 'Y'))) {
		if (pWnd->nData2 == 1) {
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		}
		pWnd->nData2 = 0;
		pWnd->dwTimeDrawStart = 0;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		if (pWnd->nData2 == 1) {
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		}
//		if (pWnd->nData2 == 2) {
			pWnd->nData2 = 0;
			pWnd->dwTimeDrawStart = 0;
//		}
	}
}

/* ========================================================================= */
/* 関数名：KeyProcMALEFAMALE												 */
/* 内容：キー入力処理(男女)													 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
void KeyProcMALEFAMALE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'A')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

		switch (pWnd->nData1) {
		case 0:
			SendCmdCHGCHARGRP (2);
			pChar->bySaveFlg &= ~CHARINFOSAVEFLG_FEMALE;
			break;

		case 1:
			SendCmdCHGCHARGRP (3);
			pChar->bySaveFlg |= CHARINFOSAVEFLG_FEMALE;
			break;
		}
		int nNo = GetWindowID (g_dwActiveMsgID);
		g_dwActiveMsgID = GetWindowID (((PWNDINFO)(g_WndInfo[nNo]->pParent))->dwID);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);

		nNo = GetWindowID (g_dwActiveMsgID);
		g_dwActiveMsgID = GetWindowID (((PWNDINFO)(g_WndInfo[nNo]->pParent))->dwID);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);

		nNo = GetWindowID (g_dwMsgIDMENUCHARINFO);
		g_WndInfo[nNo]->dwTimeDrawStart = 0;

		SendCmdNAMESEX (pChar);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (!pWnd->nData1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcDELETECONFIRM												 */
/* 内容：キー入力処理(削除確認)												 */
/* 日付：2004/01/16															 */
/* ========================================================================= */
void KeyProcDELETECONFIRM(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

		switch (pWnd->nData1) {
		case 0:	/* はい */
			{
				ZeroMemory (pChar->szName, sizeof (pChar->szName));
				int nNo = GetWindowID (g_dwMsgIDMENUCHARINFO);
				g_WndInfo[nNo]->dwTimeDrawStart = 0;
				SendCmdNAMESEX (g_aCharInfo[g_nMyCharID]);
			}
			break;
		}
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (!pWnd->nData1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcNAMECOLOR													 */
/* 内容：キー入力処理(名前の色設定)											 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void KeyProcNAMECOLOR(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		SendCmdNAMECOLOR (g_aCharInfo[g_nMyCharID]->dwTblID, aColorTbl[pWnd->nData1], aColorTbl[pWnd->nData1]);
		int nNo = GetWindowID (g_dwMsgIDCommand);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSHOP														 */
/* 内容：キー入力処理(店)													 */
/* 日付：2003/09/21															 */
/* ========================================================================= */
void KeyProcSHOP(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		switch (g_nProcShopState) {
		case SHOPMSG_OPEN:
			{
				int nNo = GetWindowID (g_dwMsgIDGold);
				if (nNo >= 0) {
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
				}
				EraseWindow (pWnd);
				g_nProcShopID = -1;

				pWndTmp = MakeWindowMSG (pShop->apMsg[SHOPMSG_CLOSE], TRUE);
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;
			}
			break;

		case SHOPMSG_BUY:
		case SHOPMSG_SELL:
			EraseWindow (pWnd);
			g_bProcShopConfirm = FALSE;

			pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_CONTINUE);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bDraw = TRUE;
			break;
		}

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		int nTmp = 0;
		switch (g_nProcShopState) {
		case SHOPMSG_OPEN:
			switch (pWnd->nData1) {
			case 0:		/* 買いにきた */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_BUY);
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;

				if (pShop->awItem[0] > 0) {
					PWNDINFO pWndItemInfo = MakeWindowITEMINFO (16 * 19, 16 * (pShop->byCountItem + 5));
					pWndTmp->pWndTmp = pWndItemInfo;
					pWndItemInfo->nData1 = pShop->awItem[0];
					g_WndInfo.Add (pWndItemInfo);
				}
				break;

			case 1:		/* 売りにきた */
				EraseWindow (pWnd);
				/* 何も持ってない？ */
				if (GetItemCount (g_nMyCharID) == 0) {
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_CONTINUE);

				} else {
					pWndTmp = MakeWindowSHOP_SELL ();
					pWndNew = pWndTmp;
				}
				g_WndInfo.Add (pWndTmp);
				if (pWndNew) {
					/* 売る物の一覧の横にカーソルのアイテムの詳細を出す */
					MakeWindowITEMINFOList (pWndNew);
				}
				g_dwActiveMsgID = pWndTmp->dwID;
				g_bDraw = TRUE;
				break;

			case 2:		/* やめる */
				{
					int nNo = GetWindowID (g_dwMsgIDGold);
					if (nNo >= 0) {
						EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
					}
					EraseWindow (pWnd);
					g_nProcShopID = -1;

					pWndTmp = MakeWindowMSG (pShop->apMsg[SHOPMSG_CLOSE], TRUE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;
				}
				break;
			}
			break;

		/* 買う */
		case SHOPMSG_BUY:
			nTmp = pWnd->nData1;
			EraseWindow (pWnd);

			/* 初回表示？ */
			if (g_bProcShopConfirm == FALSE) {
				pWndTmp = MakeWindowSHOP_CONF ();
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_nProcShopSelect = nTmp;

				char szMsgTmp[2048];
				PITEMINFO pItem = g_aItemInfo[pShop->awItem[nTmp]];
				wsprintf (szMsgTmp, pShop->apMsg[SHOPMSG_BUYCONF], pItem->szName, pItem->dwGold);
				pWndNew = MakeWindowMSG (szMsgTmp, TRUE);
				pWndNew->pParent = pWndTmp;
				pWndTmp->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);

			/* 買う確認中 */
			} else {
				/* はい */
				if (nTmp == 0) {
					PITEMINFO pItem = g_aItemInfo[pShop->awItem[g_nProcShopSelect]];
					/* お金が足りない？ */
					if (pChar->dwGold < pItem->dwGold) {
						pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_BUYGOLD);

					/* もう持てない？ */
					} else if (GetItemCount (g_nMyCharID) >= MAX_MYITEM) {
						pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_BUYTAKE);

					} else {
						SendCMDSHOPBUY ((DWORD)g_nProcShopID, (DWORD)g_nProcShopSelect);
						pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_BUYCLOSE);
					}
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;

				} else {
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_CONTINUE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;
				}
			}

			g_bDraw = TRUE;
			break;

		/* 売る */
		case SHOPMSG_SELL:
			nTmp = pWnd->nData1;
			EraseWindow (pWnd);
			char szMsgTmp[256];

			/* 初回表示？ */
			if (g_bProcShopConfirm == FALSE) {
				PITEMINFO pItem = g_aItemInfo[pChar->waItem[nTmp]];
				DWORD dwTmp = pItem->dwGold / 4;
				int i;

				/* 装備しているかチェック */
				for (i = 0; i < 10; i ++) {
					if (pChar->waEqu[i] == nTmp) {
						break;
					}
				}
				/* 価値が無い？ */
				if ((pItem->dwGold == 0) ||
				/* 捨てられない */
					(pItem->byFlg & ITEMFLG_DROP)) {
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_SELLNO);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;

				/* 装備してない */
				} else if (i == 10) {
					pWndTmp = MakeWindowSHOP_CONF ();
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_nProcShopSelect = nTmp;

					dwTmp = max (dwTmp, 1);
					wsprintf (szMsgTmp, pShop->apMsg[SHOPMSG_SELLCONF], pItem->szName, dwTmp);
					pWndNew = MakeWindowMSG (szMsgTmp, TRUE);
					pWndNew->pParent = pWndTmp;
					pWndTmp->pChild = pWndNew;
					g_WndInfo.Add (pWndNew);

				} else {
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_SELLNO);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
				}

			/* 売る確認中 */
			} else {
				PITEMINFO pItem = g_aItemInfo[pChar->waItem[g_nProcShopSelect]];
				/* はい */
				if (nTmp == 0) {
					DWORD dwTmp = pItem->dwGold / 4;
					dwTmp = max (dwTmp, 1);
					SendCmdDROPITEM (g_nProcShopSelect, dwTmp, pChar->waItem[g_nProcShopSelect]);
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_SELLCLOSE);

					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;

				} else {
					pWndTmp = MakeWindowSHOP (g_nProcShopID, SHOPMSG_CONTINUE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
				}
			}
			g_bDraw = TRUE;
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;

			if (g_nProcShopState == SHOPMSG_SELL) {
				/* 売る物の一覧：自分の持ち物の詳細 */
				UpdateWindowITEMINFOList (pWnd);

			} else if (pWnd->pWndTmp) {
				PWNDINFO pWndTmp;

				pWndTmp = (PWNDINFO)pWnd->pWndTmp;
				pWndTmp->nData1 = pShop->awItem[pWnd->nData1];
				pWndTmp->dwTimeDrawStart = 0;
			}
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSHOPSELL													 */
/* 内容：キー入力処理(店(売るだけ))											 */
/* 日付：2003/12/06															 */
/* ========================================================================= */
void KeyProcSHOPSELL(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[g_nProcShopID];
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		EraseWindow (pWnd);
		if (g_bProcShopConfirm) {
			pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_CONTINUE);
			pWndTmp->nType = WNDTYPE_SHOPSELL;
			g_bProcShopConfirm = FALSE;
			g_WndInfo.Add (pWndTmp);

			if (pShop->awItem[0] > 0) {
				PWNDINFO pWndItemInfo = MakeWindowITEMINFO (16 * 19, 16 * (pShop->byCountItem + 5));
				pWndTmp->pWndTmp = pWndItemInfo;
				pWndItemInfo->nData1 = pShop->awItem[0];
				g_WndInfo.Add (pWndItemInfo);
			}
		} else {
			int nNo = GetWindowID (g_dwMsgIDGold);
			if (nNo >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			}
			g_nProcShopID = -1;
			pWndTmp = MakeWindowMSG (pShop->apMsg[SHOPMSG_CLOSE], TRUE);
			g_WndInfo.Add (pWndTmp);
		}
		g_dwActiveMsgID = pWndTmp->dwID;
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		int nTmp = 0;
		switch (g_nProcShopState) {
		case SHOPMSG_BUY:
			nTmp = pWnd->nData1;
			EraseWindow (pWnd);

			/* 初回表示？ */
			if (g_bProcShopConfirm == FALSE) {
				pWndTmp = MakeWindowSHOP_CONF ();
				pWndTmp->nType = WNDTYPE_SHOPSELL;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;
				g_nProcShopSelect = nTmp;

				char szMsgTmp[2048];
				PITEMINFO pItem = g_aItemInfo[pShop->awItem[nTmp]];
				wsprintf (szMsgTmp, pShop->apMsg[SHOPMSG_BUYCONF], pItem->szName, pItem->dwGold);
				pWndNew = MakeWindowMSG (szMsgTmp, TRUE);
				pWndNew->pParent = pWndTmp;
				pWndTmp->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);

			/* 買う確認中 */
			} else {
				g_bProcShopConfirm = FALSE;
				/* はい */
				if (nTmp == 0) {
					PITEMINFO pItem = g_aItemInfo[pShop->awItem[g_nProcShopSelect]];
					/* お金が足りない？ */
					if (pChar->dwGold < pItem->dwGold) {
						pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_BUYGOLD);

					/* もう持てない？ */
					} else if (GetItemCount (g_nMyCharID) >= MAX_MYITEM) {
						pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_BUYTAKE);

					} else {
						SendCMDSHOPBUY ((DWORD)g_nProcShopID, (DWORD)g_nProcShopSelect);
						pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_BUYCLOSE);
					}
					pWndTmp->nType = WNDTYPE_SHOPSELL;
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;

					if (pShop->awItem[0] > 0) {
						PWNDINFO pWndItemInfo = MakeWindowITEMINFO (16 * 19, 16 * (pShop->byCountItem + 5));
						pWndTmp->pWndTmp = pWndItemInfo;
						pWndItemInfo->nData1 = pShop->awItem[0];
						g_WndInfo.Add (pWndItemInfo);
					}

				} else {
					pWndTmp = MakeWindowSHOP_BUY (SHOPMSG_CONTINUE);
					pWndTmp->nType = WNDTYPE_SHOPSELL;
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					g_bDraw = TRUE;

					if (pShop->awItem[0] > 0) {
						PWNDINFO pWndItemInfo = MakeWindowITEMINFO (16 * 19, 16 * (pShop->byCountItem + 5));
						pWndTmp->pWndTmp = pWndItemInfo;
						pWndItemInfo->nData1 = pShop->awItem[0];
						g_WndInfo.Add (pWndItemInfo);
					}
				}
			}

			g_bDraw = TRUE;
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;

			if (pWnd->pWndTmp) {
				PWNDINFO pWndTmp;

				pWndTmp = (PWNDINFO)pWnd->pWndTmp;
				pWndTmp->nData1 = pShop->awItem[pWnd->nData1];
				pWndTmp->dwTimeDrawStart = 0;
			}
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDEQU												 */
/* 内容：キー入力処理(装備)													 */
/* 日付：2005/04/11															 */
/* ========================================================================= */
void KeyProcCOMMANDEQU(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		BYTE byType;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;

		switch (pWnd->nData1) {
		case 0:	byType = ITEMTYPE_WEAPON;	break;
		case 1:	byType = ITEMTYPE_ARMOR;	break;
		case 2:	byType = ITEMTYPE_SHILD;	break;
		case 3:	byType = ITEMTYPE_HELMET;	break;
		case 4:	byType = ITEMTYPE_EQU_HAND;	break;
		case 5:	byType = ITEMTYPE_EQU_LEG;	break;
		case 6:	byType = ITEMTYPE_EQU_ACC1;	break;
		case 7:	byType = ITEMTYPE_EQU_ACC2;	break;
		case 8:	byType = ITEMTYPE_EQU_ACC3;	break;
		}

		PWNDINFO pWndNew = MakeWindowCOMMANDEQUSelect (byType);
		PWNDINFO pWndParent;
		int nTmp;
		pWndNew->pParent = pWnd->pChild;
		pWndParent = (PWNDINFO)pWnd->pChild;
		((PWNDINFO)pWnd->pChild)->pChild = pWndNew;
		g_WndInfo.Add (pWndNew);
		g_dwActiveMsgID = pWndNew->dwID;

		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		nTmp = pChar->waEqu[pWnd->nData1];
		pChar->waEqu[pWnd->nData1] = GetItemCountTypeNoEqu (g_nMyCharID, byType, 0);

		/* 攻撃力・守備力の計算 */
		RenewATDF (g_nMyCharID);
		pWndParent->dwTimeDrawStart = 0;
		pWndParent->nData1 = (int)pChar->wAT;
		pWndParent->nData2 = (int)pChar->wDF;

		pChar->waEqu[pWnd->nData1] = nTmp;
		RenewATDF (g_nMyCharID);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDEQUSUB												 */
/* 内容：キー入力処理(装備(種類別))											 */
/* 日付：2003/10/13															 */
/* ========================================================================= */
void KeyProcCOMMANDEQUSUB(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	PWNDINFO pWndParent;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		pWndParent = (PWNDINFO)pWnd->pParent;
		pWndParent->dwTimeDrawStart = 0;
		pWndParent->nData1 = (int)pChar->wAT;
		pWndParent->nData2 = (int)pChar->wDF;

		EraseWindow (pWnd);
		g_dwActiveMsgID = ((PWNDINFO)pWndParent->pParent)->dwID;
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		int nTmp;
		/* 装備しない？ */
		if (pWnd->nData1 == pWnd->nData2 - 1) {
			nTmp = 0xFFFF;
		} else {
			nTmp = GetItemCountTypeNoEqu (g_nMyCharID, pWnd->byData1, pWnd->nData1);
		}
		BYTE byType;
		switch (pWnd->byData1) {
		case ITEMTYPE_WEAPON:	byType = 0;	break;
		case ITEMTYPE_ARMOR:	byType = 1;	break;
		case ITEMTYPE_SHILD:	byType = 2;	break;
		case ITEMTYPE_HELMET:	byType = 3;	break;
		case ITEMTYPE_EQU_HAND:	byType = 4;	break;
		case ITEMTYPE_EQU_LEG:	byType = 5;	break;
		case ITEMTYPE_EQU_ACC1:	byType = 6;	break;
		case ITEMTYPE_EQU_ACC2:	byType = 7;	break;
		case ITEMTYPE_EQU_ACC3:	byType = 8;	break;
		}
		pChar->waEqu[byType] = nTmp;

		/* 攻撃力・守備力の計算 */
		RenewATDF (g_nMyCharID);
		/* サーバーに通知 */
		SendCmdCHGEQU (g_nMyCharID, byType);

		pWndParent = (PWNDINFO)pWnd->pParent;
		pWndParent->dwTimeDrawStart = 0;
		EraseWindow (pWnd);
		g_dwActiveMsgID = ((PWNDINFO)pWndParent->pParent)->dwID;
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;

			PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
			int nTmp, nTmp2;
			/* 装備しない？ */
			if (pWnd->nData1 == pWnd->nData2 - 1) {
				nTmp = 0xFFFF;
			} else {
				nTmp = GetItemCountTypeNoEqu (g_nMyCharID, pWnd->byData1, pWnd->nData1);
			}
			BYTE byType;
			switch (pWnd->byData1) {
			case ITEMTYPE_WEAPON:	byType = 0;	break;
			case ITEMTYPE_ARMOR:	byType = 1;	break;
			case ITEMTYPE_SHILD:	byType = 2;	break;
			case ITEMTYPE_HELMET:	byType = 3;	break;
			case ITEMTYPE_EQU_HAND:	byType = 4;	break;
			case ITEMTYPE_EQU_LEG:	byType = 5;	break;
			case ITEMTYPE_EQU_ACC1:	byType = 6;	break;
			case ITEMTYPE_EQU_ACC2:	byType = 7;	break;
			case ITEMTYPE_EQU_ACC3:	byType = 8;	break;
			}
			nTmp2 = pChar->waEqu[byType];
			pChar->waEqu[byType] = nTmp;

			/* 攻撃力・守備力の計算 */
			RenewATDF (g_nMyCharID);
			pWndParent = (PWNDINFO)pWnd->pParent;
			pWndParent->dwTimeDrawStart = 0;
			pWndParent->nData1 = (int)pChar->wAT;
			pWndParent->nData2 = (int)pChar->wDF;

			pChar->waEqu[byType] = nTmp2;
			RenewATDF (g_nMyCharID);
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcLEAVE														 */
/* 内容：キー入力処理(預かり所)												 */
/* 日付：2003/11/06															 */
/* ========================================================================= */
void KeyProcLEAVE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		switch (g_nProcShopState) {
		case LEAVESTATE_OPEN:
			EraseWindow (pWnd);

			pWndTmp = MakeWindowMSG ("＊｢ありがとうございました。またどうぞ。@2", TRUE);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bDraw = TRUE;
			break;

		case LEAVESTATE_GOLD:
		case LEAVESTATE_GOLDGET:
			{
				int nNo = GetWindowID (g_dwMsgIDGold);
				if (nNo >= 0) {
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
				}
			}
		case LEAVESTATE_SELECT:
		case LEAVESTATE_SELECTGET:
		case LEAVESTATE_ITEM:
		case LEAVESTATE_ITEMSELECTGET:
			EraseWindow (pWnd);

			pWndTmp = MakeWindowLEAVE (1);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bDraw = TRUE;
			break;
		}

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		PWNDINFO pWndTmp = NULL;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		int i;
		BOOL bItemInfo = FALSE;

		switch (g_nProcShopState) {
		/* 最初 */
		case LEAVESTATE_OPEN:
			switch (pWnd->nData1) {
			case 0:		/* 預ける */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowLEAVE_SELECT (0);
				break;

			case 1:		/* 引き取る */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowLEAVE_SELECT (1);
				g_nProcShopState = LEAVESTATE_SELECTGET;
				break;

			default:	/* やめる */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowMSG ("＊｢ありがとうございました。またどうぞ。@2", TRUE);
				break;
			}
			break;

		/* どちらを預けるか選択 */
		case LEAVESTATE_SELECT:
			switch (pWnd->nData1) {
			case 0:		/* もちもの */
				if (GetItemCount (g_nMyCharID) == 0) {
					pWndTmp = MakeWindowLEAVE (7);
					break;
				}
				pWndTmp = MakeWindowLEAVE_ITEM ();
				/* 一覧の横にカーソルのアイテムの詳細を出す（一覧を登録した後で付ける） */
				bItemInfo = TRUE;
				break;

			case 1:		/* お金 */
				if (pChar->dwGold == 0) {
					pWndTmp = MakeWindowLEAVE (8);
					break;
				}
				pWndTmp = MakeWindowGOLD ();
				g_WndInfo.Add (pWndTmp);
				pWndTmp = MakeWindowLEAVE_GOLD (0);
				break;

			default:	/* やめる */
				pWndTmp = MakeWindowLEAVE (1);
				break;
			}
			EraseWindow (pWnd);
			break;

		/* どちらを引き取るか選択 */
		case LEAVESTATE_SELECTGET:
			switch (pWnd->nData1) {
			case 0:		/* もちもの */
				if (GetItemCount (g_nMyCharID) == MAX_MYITEM) {
					pWndTmp = MakeWindowLEAVE (9);
					break;
				}
				g_nProcShopState = LEAVESTATE_ITEMSELECTGET;
				SAFE_DELETE (g_pLeaveInfo);
				SendCmdLEAVEITEMGET (-1);
				g_bEnableMove = FALSE;
				break;

			case 1:		/* お金 */
				/* これ以上持てない？ */
				if (pChar->dwGold > 999999) {
					pWndTmp = MakeWindowLEAVE (9);
					break;
				}
				g_nProcShopState = LEAVESTATE_GOLDGET;
				SendCmdLEAVEGOLDGET (-1);
				g_bEnableMove = FALSE;
				break;

			default:	/* やめる */
				pWndTmp = MakeWindowLEAVE (1);
				break;
			}
			EraseWindow (pWnd);
			break;

		case LEAVESTATE_ITEM:
			/* 装備中？ */
			for (i = 0; i < 10; i ++) {
				if (pChar->waEqu[i] == pWnd->nData1) {
					break;
				}
			}
			if (i < 10) {
				pWndTmp = MakeWindowLEAVE (2);

			} else {
				SendCmdLEAVEITEM (pWnd->nData1);
				pWndTmp = MakeWindowLEAVE (1);
			}
			EraseWindow (pWnd);
			break;

		case LEAVESTATE_ITEMSELECTGET:
			if (g_pLeaveInfo == NULL) {
				break;
			}
			if (GetItemCount (g_nMyCharID) == MAX_MYITEM) {
				EraseWindow (pWnd);
				pWndTmp = MakeWindowLEAVE (5);

			} else {
				SendCmdLEAVEITEMGET (pWnd->nData1 + pWnd->byData1);
				EraseWindow (pWnd);
				pWndTmp = MakeWindowLEAVE (1);
			}
			break;

		/* 預ける金額設定 */
		case LEAVESTATE_GOLD:
			i = GetWindowID (g_dwMsgIDGold);
			if (i >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[i]);
			}
			SendCmdLEAVEGOLD ((DWORD)pWnd->nData1);
			EraseWindow (pWnd);
			pWndTmp = MakeWindowLEAVE (1);
			break;

		/* 引き取る金額設定 */
		case LEAVESTATE_GOLDGET:
			i = GetWindowID (g_dwMsgIDGold);
			if (i >= 0) {
				EraseWindow ((PWNDINFO)g_WndInfo[i]);
			}
			SendCmdLEAVEGOLDGET ((DWORD)pWnd->nData1);
			EraseWindow (pWnd);
			pWndTmp = MakeWindowLEAVE (1);
			break;
		}

		if (pWndTmp) {
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			if (bItemInfo) {
				MakeWindowITEMINFOList (pWndTmp);
			}
		}

	} else if ((byEvent == KEY_DOWN) && (g_bEnableMove) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		/* 預ける金額設定？ */
		if ((g_nProcShopState == LEAVESTATE_GOLD) ||
			(g_nProcShopState == LEAVESTATE_GOLDGET)) {
			if (byCode == VK_UP) {
				int nTmp = 1, nTmpGold;
				char szTmp[7], szTmp2[2];
				nTmpGold = pWnd->nData1;
				if (nTmpGold >= 1000000) {
					nTmpGold -= 1000000;
				}
				wsprintf (szTmp, "%06d", nTmpGold);
				szTmp2[0] = szTmp[pWnd->byData1];
				szTmp2[1] = 0;
				for (int i = 0; i < 5 - pWnd->byData1; i ++) {
					nTmp *= 10;
				}
				if (atoi (szTmp2) + 1 >= 10) {
					nTmp -= ((atoi (szTmp2) + 1) * nTmp);
				}
				pWnd->nData1 += nTmp;
				if (g_nProcShopState == LEAVESTATE_GOLD) {
					pWnd->nData1 = min (pWnd->nData1, (int)pChar->dwGold);
				} else {
					pWnd->nData1 = min (pWnd->nData1, (int)g_pLeaveInfo->dwGold);
					if (pWnd->nData1 + (int)pChar->dwGold > 999999) {
						pWnd->nData1 = 999999 - pChar->dwGold;
					}
				}
				bMove = TRUE;
				pWnd->byAnime = 0;

			} else if (byCode == VK_DOWN) {
				int nTmp = 1, nTmpGold;
				char szTmp[7], szTmp2[2];
				nTmpGold = pWnd->nData1;
				if (nTmpGold >= 1000000) {
					nTmpGold -= 1000000;
				}
				wsprintf (szTmp, "%06d", nTmpGold);
				szTmp2[0] = szTmp[pWnd->byData1];
				szTmp2[1] = 0;
				for (int i = 0; i < 5 - pWnd->byData1; i ++) {
					nTmp *= 10;
				}
				if (atoi (szTmp2) - 1 < 0) {
					nTmp += ((atoi (szTmp2) - 1) * nTmp * 10);
				}
				pWnd->nData1 -= nTmp;
				if (g_nProcShopState == LEAVESTATE_GOLD) {
					pWnd->nData1 = min (pWnd->nData1, (int)pChar->dwGold);
				} else {
					pWnd->nData1 = min (pWnd->nData1, (int)g_pLeaveInfo->dwGold);
					if (pWnd->nData1 + (int)pChar->dwGold >= 999999) {
						pWnd->nData1 = 999999 - pChar->dwGold;
					}
				}
				bMove = TRUE;
				pWnd->byAnime = 0;

			} else if ((byCode == VK_LEFT) && (pWnd->byData1 > 0)) {
				pWnd->byData1 --;
				bMove = TRUE;
				pWnd->byAnime = 1;

			} else if ((byCode == VK_RIGHT) && (pWnd->byData1 < 5)) {
				pWnd->byData1 ++;
				bMove = TRUE;
				pWnd->byAnime = 1;
			}

		} else {
			if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
				pWnd->nData1 --;
				bMove = TRUE;
				pWnd->byAnime = 1;
				if (pWnd->nData1 == 0) {
					if (g_pLeaveInfo && (g_pLeaveInfo->byCount > 10)) {
						if (pWnd->nData1 + pWnd->byData1 > 0) {
							pWnd->nData1 ++;
							pWnd->byData1 --;
						}
					}
				}

			} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
				pWnd->nData1 ++;
				bMove = TRUE;
				pWnd->byAnime = 1;
				if (g_nProcShopState == LEAVESTATE_ITEMSELECTGET) {
					if (pWnd->nData1 == pWnd->nData2 - 1) {
						if (g_pLeaveInfo && (g_pLeaveInfo->byCount > 10)) {
							if (pWnd->nData1 + pWnd->byData1 < g_pLeaveInfo->byCount - 1) {
								pWnd->nData1 --;
								pWnd->byData1 ++;
							}
						}
					}
				}

			} else if (byCode == VK_LEFT) {
				pWnd->nData1 = 0;
				pWnd->byData1 = 0;
				bMove = TRUE;
				pWnd->byAnime = 1;

			} else if ((byCode == VK_RIGHT) && (pWnd->nData1 < pWnd->nData2 - 1)) {
				pWnd->nData1 = pWnd->nData2 - 1;
				if (g_nProcShopState == LEAVESTATE_ITEMSELECTGET) {
					if (g_pLeaveInfo && (g_pLeaveInfo->byCount > 10)) {
						pWnd->byData1 = g_pLeaveInfo->byCount - 10;
					}
				}
				bMove = TRUE;
				pWnd->byAnime = 1;
			}
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;

			/* アイテムの詳細を出し直す（アイテム一覧の時だけ付いている） */
			UpdateWindowITEMINFOList (pWnd);
		}

	} else if ((byEvent == KEY_UP) && (g_bEnableMove)) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDTRADE												 */
/* 内容：キー入力処理(取り引きコマンド)										 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void KeyProcCOMMANDTRADE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;

		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		SendCmdTRADE (g_aUseCharInfo[pWnd->nData1]->dwTblID, CMDTRADE_IN);

		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}
		g_bEnableMove = FALSE;

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcTRADE														 */
/* 内容：キー入力処理(取り引き)												 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void KeyProcTRADE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	/* 取り引き終了 */
	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		/* 取り引き終了 */
		SendCmdTRADE (g_pTradeInfo->dwTblID, CMDTRADE_OUT);
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;

		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		/* 決定 */
		if (pWnd->nData1 == 0) {
			if (g_pTradeInfo->byMode1 == 0) {
				g_pTradeInfo->byMode1 = 1;
			} else {
				g_pTradeInfo->byMode1 = 0;
			}
			SendCmdTRADEINFO ();

		/* 金額設定 */
		} else if (pWnd->nData1 == 1) {
			PWNDINFO pWndNew = MakeWindowTRADEGOLD ();
			pWndNew->pParent = pWnd;
			pWnd->pChild = pWndNew;
			pWndNew->nData1 = g_pTradeInfo->dwGold1;
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;

		/* アイテム追加 */
		} else if (pWnd->nData1 >= 2) {
			if (GetItemCount (g_nMyCharID) >= 1) {
				PWNDINFO pWndNew = MakeWindowTRADEITEM ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcTRADEITEM													 */
/* 内容：キー入力処理(取り引き：アイテム選択)								 */
/* 日付：2003/11/16															 */
/* ========================================================================= */
void KeyProcTRADEITEM(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		EraseWindow (pWnd);
		g_bDraw = TRUE;

		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
		if ((pWndParent->nData1 < pWndParent->nData2 - 1) ||
			(((pWndParent->nData1 == pWndParent->nData2 - 1) && (pWndParent->nData2 == MAX_MYITEM + 2)))) {
			int i;
			for (i = pWndParent->nData1 - 2; i < MAX_MYITEM - 1; i ++) {
				g_pTradeInfo->waItem1[i] = g_pTradeInfo->waItem1[i + 1];
			}
			g_pTradeInfo->waItem1[i] = 0xFFFF;
			if (pWndParent->nData2 < MAX_MYITEM + 2) {
				pWndParent->nData2 --;
			}
			SendCmdTRADEINFO ();
		}

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

		int i, j;
		for (i = 0; i < MAX_MYITEM; i ++) {
			if (g_pTradeInfo->waItem1[i] == pWnd->nData1) {
				break;
			}
		}
		if (i == MAX_MYITEM) {
			/* 装備しているかチェック */
			for (j = 0; j < 10; j ++) {
				if (pChar->waEqu[j] == pWnd->nData1) {
					break;
				}
			}
			if (j == 10) {
				g_pTradeInfo->waItem1[pWndParent->nData1 - 2] = pWnd->nData1;
				if (pWndParent->nData1 == pWndParent->nData2 - 1) {
					if (pWndParent->nData2 < MAX_MYITEM + 2) {
						pWndParent->nData2 ++;
					}
				}
			}
		}

		SendCmdTRADEINFO ();
		EraseWindow (pWnd);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcTRADEGOLD													 */
/* 内容：キー入力処理(取り引き：金額設定)									 */
/* 日付：2004/01/03															 */
/* ========================================================================= */
void KeyProcTRADEGOLD(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		EraseWindow (pWnd);
		g_bDraw = TRUE;

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		g_pTradeInfo->dwGold1 = (DWORD)pWnd->nData1;
		g_pTradeInfo->dwGold1 = min (g_pTradeInfo->dwGold1, pChar->dwGold);

		SendCmdTRADEINFO ();
		EraseWindow (pWnd);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if (byCode == VK_UP) {
			int nTmp = 1, nTmpGold;
			char szTmp[7], szTmp2[2];
			nTmpGold = pWnd->nData1;
			if (nTmpGold >= 1000000) {
				nTmpGold -= 1000000;
			}
			wsprintf (szTmp, "%06d", nTmpGold);
			szTmp2[0] = szTmp[pWnd->byData1];
			szTmp2[1] = 0;
			for (int i = 0; i < 5 - pWnd->byData1; i ++) {
				nTmp *= 10;
			}
			if (atoi (szTmp2) + 1 >= 10) {
				nTmp -= ((atoi (szTmp2) + 1) * nTmp);
			}
			pWnd->nData1 += nTmp;
			bMove = TRUE;
			pWnd->byAnime = 0;

		} else if (byCode == VK_DOWN) {
			int nTmp = 1, nTmpGold;
			char szTmp[7], szTmp2[2];
			nTmpGold = pWnd->nData1;
			if (nTmpGold >= 1000000) {
				nTmpGold -= 1000000;
			}
			wsprintf (szTmp, "%06d", nTmpGold);
			szTmp2[0] = szTmp[pWnd->byData1];
			szTmp2[1] = 0;
			for (int i = 0; i < 5 - pWnd->byData1; i ++) {
				nTmp *= 10;
			}
			if (atoi (szTmp2) - 1 < 0) {
				nTmp += ((atoi (szTmp2) - 1) * nTmp * 10);
			}
			pWnd->nData1 -= nTmp;
			bMove = TRUE;
			pWnd->byAnime = 0;

		} else if ((byCode == VK_LEFT) && (pWnd->byData1 > 0)) {
			pWnd->byData1 --;
			bMove = TRUE;
			pWnd->byAnime = 1;

		} else if ((byCode == VK_RIGHT) && (pWnd->byData1 < 5)) {
			pWnd->byData1 ++;
			bMove = TRUE;
			pWnd->byAnime = 1;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBGAME_BOMBSET											 */
/* 内容：キー入力処理(サブゲーム：爆弾ゲーム設定)							 */
/* 日付：2003/12/20															 */
/* ========================================================================= */
void KeyProcSUBGAME_BOMBSET(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		if (g_pSubGameInfoPlay == NULL) {
			return;
		}
		PSUBGAMECLIENT_BOMBINFO pInfo = (PSUBGAMECLIENT_BOMBINFO)g_pSubGameInfoPlay->pInfo;

		switch (pWnd->nData1) {
		case 0:		/* 試合開始 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			SendCmdSubBOMBINFO ();
			SendCmdSubBOMBSTART ();
			EraseWindow (pWnd);
			break;

		case 1:		/* 制限時間 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			if (pInfo->byGameTime == 240) {
				pInfo->byGameTime = 0;
				break;
			}
			pInfo->byGameTime += 30;
			break;

		case 2:		/* 設置数 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			pInfo->byMaxBomb ++;
			if (pInfo->byMaxBomb > 5) {
				pInfo->byMaxBomb = 1;
			}
			break;

		case 3:		/* 火力 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			pInfo->byMaxFire ++;
			if (pInfo->byMaxFire == 1) {
				pInfo->byMaxFire = 3;
			} else if (pInfo->byMaxFire > 10) {
				pInfo->byMaxFire = 0;
			}
			break;

		case 4:		/* 壊れる壁 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			pInfo->byFlgUseBlock = 1 - pInfo->byFlgUseBlock;
			SendCmdSubBOMBINFO ();
			break;

		case 5:		/* アイテム */
			if (pInfo->byFlgGameNow) {
				break;
			}
			pInfo->byFlgUseItem = 1 - pInfo->byFlgUseItem;
			break;

		case 6:		/* 途中参加 */
			if (pInfo->byFlgGameNow) {
				break;
			}
			pInfo->byFlgProgress = 1 - pInfo->byFlgProgress;
			break;

		case 7:		/* 試合中止 */
			if (!pInfo->byFlgGameNow) {
				break;
			}
			SendCmdSubBOMBSTOP ();
			EraseWindow (pWnd);
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSPEECHEDIT												 */
/* 内容：キー入力処理(自己紹介設定)											 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
void KeyProcSPEECHEDIT(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == VK_ESCAPE)) {
		EraseWindow (pWnd);
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCHGPASSWORD												 */
/* 内容：キー入力処理(パスワードの変更)										 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
void KeyProcCHGPASSWORD(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == VK_ESCAPE)) {
		EraseWindow (pWnd);
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDPARTY												 */
/* 内容：キー入力処理(仲間ウィンドウ)										 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void KeyProcCOMMANDPARTY(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;

		switch (pWnd->nData1) {
		/* 仲間に誘う */
		case 0:
			{
				int i;
				DWORD dwX = pChar->dwX + anPosAddTblX[pChar->byDirection],
					  dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
				for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
					PCHARINFO pCharTmp = g_aCharInfo[i];
					/* 目の前にいる？ */
					if ((dwX == pCharTmp->dwX) && (dwY == pCharTmp->dwY)) {
						if (pCharTmp->byNpcType == 0) {
							if (pCharTmp != pChar) {
								break;
							}
						}
					}
				}
				if (i < g_aCharInfo.GetSize ()) {
					PWNDINFO pWndNew = MakeWindowCOMMANDPARTYCHAR ();
					pWndNew->pParent = pWnd;
					pWnd->pChild = pWndNew;
					g_WndInfo.Add (pWndNew);
					g_dwActiveMsgID = pWndNew->dwID;
					break;
				}
			}
			break;

		/* 抜ける */
		case 1:
			SendCMDPARTY (CMDPARTY_BREAK, pChar->dwTblID, 0);
			EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
			break;

		/* 分配方法 */
		case 2:
			SendCMDPARTY (CMDPARTY_GOLD, pChar->dwTblID, 0);
			EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
			break;

		/* 解散 */
		case 3:
			{
				char szTmp[128];
				PWNDINFO pWndTmp = NULL, pWndTmp2 = NULL;
				/* 確認ウィンドウ作成 */
				pWndTmp = MakeWindowCONF (16 * 20, 16 * 10, IDCONFIRMTYPE_BREAKUP);
				pWndTmp->nData1 = 1;
				pWndTmp->pParent = pWnd;
				pWnd->pChild = pWndTmp;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;

				wsprintf (szTmp, "パーティを解散しますか？@2");
				pWndTmp2 = MakeWindowMSG (szTmp, FALSE);
				pWndTmp2->pParent = pWndTmp;
				pWndTmp->pChild = pWndTmp2;
				g_WndInfo.Add (pWndTmp2);
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDPARTYCHAR											 */
/* 内容：キー入力処理(誘う相手選択)											 */
/* 日付：2004/05/30															 */
/* ========================================================================= */
void KeyProcCOMMANDPARTYCHAR(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;
		PWNDINFO pWndParent = (PWNDINFO)pWnd->pParent;

		SendCMDPARTY (CMDPARTY_JOIN, g_aUseCharInfo[pWnd->nData1]->dwTblID, 0);

		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：StartPosConfirmEnd												 */
/* 内容：復活位置の「はい・いいえ」を選んだ後の処理							 */
/*		 はい：記録してセリフの続き（「復活位置が記録されました。」など）へ	 */
/*		 いいえ：記録せず、続きを「記録しませんでした」に差し替える			 */
/* ========================================================================= */
void StartPosConfirmEnd(PWNDINFO pWnd, BOOL bYes)
{
	PWNDINFO pWndMsg = (PWNDINFO)pWnd->pParent;
	int nPos = pWnd->nData2;

	/* 町転送の行き先がいっぱいで、新しい町なら、忘れる町を選んでもらう */
	if (bYes && pWndMsg && TownWarpNeedForget ((DWORD)((nPos >> 16) & 0xFF))) {
		PWNDINFO pWndForget;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);
		pWndForget = MakeWindowTOWNWARP (TOWNWARPWND_FORGET);
		pWndForget->dwData = (DWORD)nPos;
		/* メッセージ欄の左上に出す */
		pWndForget->nX = pWndMsg->nX + 16;
		pWndForget->nY = max (0, pWndMsg->nY - pWndForget->nCy - 8);
		pWndForget->pParent = pWndMsg;
		pWndMsg->pChild = pWndForget;
		g_WndInfo.Add (pWndForget);
		g_dwActiveMsgID = pWndForget->dwID;
		g_bDraw = TRUE;
		return;
	}

	EraseWindow (pWnd);
	StartPosFinish (pWndMsg, bYes, nPos, -1);
}

/* ========================================================================= */
/* 関数名：StartPosFinish													 */
/* 内容：復活位置を記録する（bYes）・しないを決めて、セリフの続きを流す		 */
/*		 nForgetMapID：町転送の行き先から忘れる町（-1 なら指定なし）		 */
/* ========================================================================= */
void StartPosFinish(PWNDINFO pWndMsg, BOOL bYes, int nPos, int nForgetMapID)
{
	if (bYes) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		SendCmdSETSTARTPOS ((BYTE)((nPos >> 16) & 0xFF), (BYTE)((nPos >> 8) & 0xFF), (BYTE)(nPos & 0xFF), nForgetMapID);

	} else {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		if (pWndMsg && pWndMsg->pszMsg) {
			/* 問いかけ（「@k」まで）は残して、続きだけ差し替える */
			static const char *pszNo = "@1@1復活位置はそのままにしました。@2";
			int nLenHead = pWndMsg->nData1 + 1;
			LPSTR pszNew = new char[nLenHead + strlen (pszNo) + 1];
			CopyMemory (pszNew, pWndMsg->pszMsg, nLenHead);
			strcpy (&pszNew[nLenHead], pszNo);
			SAFE_DELETE_ARRAY (pWndMsg->pszMsg);
			pWndMsg->pszMsg = pszNew;
		}
	}

	/* メッセージの続きを流す */
	if (pWndMsg) {
		pWndMsg->nData2 = 0;
		pWndMsg->dwTime = 0;		/* 1 文字ずつ流し直す */
		pWndMsg->dwTimeDrawStart = 0;
	}
	g_bDraw = TRUE;
}


/* ========================================================================= */
/* 関数名：KeyProcCONF														 */
/* 内容：キー入力処理(確認)													 */
/* 日付：2004/06/20															 */
/* ========================================================================= */
void KeyProcCONF(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	/* 復活位置の確認は専用の処理 */
	if (pWnd->nType2 == IDCONFIRMTYPE_STARTPOS) {
		if ((byEvent == KEY_UP) && ((byCode == 'A') || (byCode == 'B'))) {
			/* セリフを読み進める決定ボタンの連打で選ばれないよう、出た直後は受け付けない */
			PWNDINFO pWndMsg = (PWNDINFO)pWnd->pParent;
			if (pWndMsg && (timeGetTime () < pWndMsg->dwTime + 400)) {
				return;
			}
			StartPosConfirmEnd (pWnd, (byCode == 'A') && (pWnd->nData1 == 0));
			return;
		}
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		switch (pWnd->nType2) {
		case IDCONFIRMTYPE_PARTYJOIN:	/* 仲間に入る確認 */
			SendCMDPARTY (CMDPARTY_JOINNO, (DWORD)pWnd->nData2, 0);
			EraseWindow (pWnd);
			break;

		case IDCONFIRMTYPE_GRPCHANGE:	/* 着せ替え確認 */
		case IDCONFIRMTYPE_DROP:		/* 捨てる確認 */
			EraseWindow (pWnd);
			break;

		default:
			EraseWindow (pWnd);
			break;
		}
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A')) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

		switch (pWnd->nData1) {
		case 0:	/* はい */
			{
				char szTmp[128];
				PWNDINFO pWndTmp = NULL;
				PITEMINFO pItem = g_aItemInfo[pChar->waItem[pWnd->byData1]];

				ZeroMemory (szTmp, sizeof (szTmp));
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

				switch (pWnd->nType2) {
				case IDCONFIRMTYPE_DROP:		/* 捨てる確認 */
					EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);

					wsprintf (szTmp, "%sは%sを捨てました。@2", pChar->szName, pItem->szName);
					SendCmdDROPITEM (pWnd->byData1, 0, pChar->waItem[pWnd->byData1]);

					pWndTmp = MakeWindowMSG (szTmp, FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					break;

				case IDCONFIRMTYPE_GRPCHANGE:	/* 着せ替え確認 */
					SendCmdCHGCHARGRP ((DWORD)pWnd->nData2);
					{
						/* Sキーで話した時はコマンドウィンドウが無いので確認ウィンドウだけ閉じる */
						int nNoCommand = GetWindowID (g_dwMsgIDCommand);
						if (nNoCommand >= 0) {
							EraseWindow ((PWNDINFO)g_WndInfo[nNoCommand]);
						} else {
							EraseWindow (pWnd);
						}
					}
					break;

				case IDCONFIRMTYPE_PARTYJOIN:	/* 仲間に入る確認 */
					SendCMDPARTY (CMDPARTY_JOINOK, (DWORD)pWnd->nData2, 0);
					EraseWindow (pWnd);
					break;

				case IDCONFIRMTYPE_BREAKUP:		/* パーティ解散 */
					SendCMDPARTY (CMDPARTY_BREAKUP, pChar->dwTblID, 0);
					EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
					break;

				case IDCONFIRMTYPE_BREAKSUGOROKU:	/* すごろくを抜ける */
					{
						PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

						SendCmdSubSUGOROKUQUIT ();
						pSubStatus->byState = SUBGAME_SUGOROKU_STATE_IDLE;
						EraseWindow ((PWNDINFO)pWnd->pParent);
					}
					break;

				default:
					EraseWindow (pWnd);
					break;
				}
			}
			break;

		case 1:	/* いいえ */
			{
				DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

				switch (pWnd->nType2) {
				case IDCONFIRMTYPE_PARTYJOIN:	/* 仲間に入る確認 */
					SendCMDPARTY (CMDPARTY_JOINNO, (DWORD)pWnd->nData2, 0);
					EraseWindow (pWnd);
					break;

				case IDCONFIRMTYPE_GRPCHANGE:	/* 着せ替え確認 */
				case IDCONFIRMTYPE_DROP:		/* 捨てる確認 */
				default:
					EraseWindow (pWnd);
					g_bDraw = TRUE;
					break;
				}
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (!pWnd->nData1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcJOB														 */
/* 内容：キー入力処理(冒険者登録所)											 */
/* 日付：2004/07/11															 */
/* ========================================================================= */
void KeyProcJOB(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		PWNDINFO pWndTmp = NULL, pWndNew = NULL;
		switch (g_nProcShopState) {
		case JOBSTATE_OPEN:			/* 選択メニュー */
			EraseWindow (pWnd);

			pWndTmp = MakeWindowMSG ("＊｢ご苦労様でした。よい旅を。@2", TRUE);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bDraw = TRUE;
			break;

		case JOBSTATE_JOBSELECT:	/* 職業選択 */
			EraseWindow (pWnd);

			pWndTmp = MakeWindowJOB (1);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
			g_bDraw = TRUE;
			break;
		}

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		PWNDINFO pWndTmp = NULL;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;
		g_bDraw = TRUE;

		switch (g_nProcShopState) {
		case JOBSTATE_OPEN:			/* 選択メニュー */
			switch (pWnd->nData1) {
			case 0:		/* 職業の変更 */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowJOB_JOBSELECT ();
				break;

			default:	/* やめる */
				EraseWindow (pWnd);
				pWndTmp = MakeWindowMSG ("＊｢ご苦労様でした。お気をつけて。@2", TRUE);
				break;
			}
			break;

		case JOBSTATE_JOBSELECT:	/* 職業選択 */
			SendCMDJOBCHG ((BYTE)(pWnd->nData1));
			EraseWindow (pWnd);
			pWndTmp = MakeWindowJOB (2);
			break;
		}

		if (pWndTmp) {
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
		}

	} else if ((byEvent == KEY_DOWN) && (g_bEnableMove) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;
			pWnd->byAnime = 1;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;
			pWnd->byAnime = 1;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			pWnd->byData1 = 0;
			bMove = TRUE;
			pWnd->byAnime = 1;

		} else if ((byCode == VK_RIGHT) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
			pWnd->byAnime = 1;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if ((byEvent == KEY_UP) && (g_bEnableMove)) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDMAGIC												 */
/* 内容：キー入力処理(魔法コマンド)											 */
/* 日付：2004/07/12															 */
/* ========================================================================= */
void KeyProcCOMMANDMAGIC(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		int			nData1;
		char		szTmp[128];
		PWNDINFO	pWndTmp;
		PCHARINFO	pChar;
		PMAGICINFO	pInfo;
		BOOL		bClose;

		bClose = TRUE;
		g_bDraw = TRUE;
		nData1 = pWnd->nData1;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

		pWndTmp = NULL;
		pChar = g_aCharInfo[g_nMyCharID];
		pInfo = g_aMagicInfo[g_aMagicInfoWalk[nData1]];

		switch (pInfo->byTypeInfoID) {
		case MAGICUSETYPE_WALK:			/* 平常状態 */
		case MAGICUSETYPE_WALKBATTLE:	/* 平常状態と戦闘状態 */
			/* Manaチェック */
			if (pInfo->byType == MAGICTYPE_HPDOWNHPCOST) {
				if (pChar->wHP <= pInfo->wMana) {
					wsprintf (szTmp, "HPが足りません。@2");
					pWndTmp = MakeWindowMSG (szTmp, FALSE);
					SAFE_DELETE (g_pTargetSelectInfo);
				}
			} else if (pChar->wMana < pInfo->wMana) {
				wsprintf (szTmp, "Manaが足りません。@2");
				pWndTmp = MakeWindowMSG (szTmp, FALSE);
				SAFE_DELETE (g_pTargetSelectInfo);

			} else {
				/* 発動種別が「選択」？ */
				if ((pInfo->byTargetType == MAGICTARGETTYPE_SELECT) ||
					(pInfo->byTargetType == MAGICTARGETTYPE_SELECT2)) {
					BOOL bTmp;

					bTmp = TRUE;
					if (pInfo->byTargetType == MAGICTARGETTYPE_SELECT2) {
						bTmp = FALSE;
					}

					/* キャラ選択 */
					pWndTmp = MakeWindowBATTLE_TARGET (pInfo->byTargetArea, bTmp, pInfo->byTarget);
					if (pWndTmp == NULL) {
						pWndTmp = MakeWindowMSG ("対象者がいません。@2", FALSE);
						break;
					}
					pWndTmp->pParent = pWnd;
					pWnd->pChild = pWndTmp;
					bClose = FALSE;
				/* 町転送で、選べる行き先が2つ以上 */
				} else if (IsTownWarpMagic ((BYTE)g_aMagicInfoWalk[nData1]) && (GetTownWarpSelectCount () >= 2)) {
					/* 行き先の選択（決まったら子ウィンドウ側で詠唱開始） */
					pWndTmp = MakeWindowTOWNWARP (TOWNWARPWND_USE);
					pWndTmp->pParent = pWnd;
					pWnd->pChild = pWndTmp;
					bClose = FALSE;
				} else {
					/* 詠唱開始 */
					g_byTownWarpSel = 0;
					g_byComSelectMagic = (BYTE)g_aMagicInfoWalk[nData1];
					pChar->dwTimeTurnMagicLV = timeGetTime ();
					pChar->byTurnMagicLV = 1;
					SendCmdEFECTCHG (pInfo->byUserEfectID, TRUE);
				}
			}
			break;
		}

		if (bClose) {
			EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
		}

		if (pWndTmp) {
			g_bDraw = TRUE;
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcCOMMANDMAGICBATTLE										 */
/* 内容：キー入力処理(魔法コマンド(戦闘マップ))								 */
/* 日付：2004/09/25															 */
/* ========================================================================= */
void KeyProcCOMMANDMAGICBATTLE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		int			nData1;
		char		szTmp[128];
		PWNDINFO	pWndTmp;
		PCHARINFO	pChar;
		PMAGICINFO	pInfo;
		BOOL		bClose;

		bClose = TRUE;
		g_bDraw = TRUE;
		nData1 = pWnd->nData1;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWndTmp = NULL;
		pChar = g_aCharInfo[g_nMyCharID];
		pInfo = g_aMagicInfo[g_aMagicInfoBattle[nData1]];

		switch (pInfo->byTypeInfoID) {
		case MAGICUSETYPE_BATTLE:		/* 戦闘状態 */
		case MAGICUSETYPE_WALKBATTLE:	/* 平常状態と戦闘状態 */
			/* Manaチェック */
			if (pInfo->byType == MAGICTYPE_HPDOWNHPCOST) {
				if (pChar->wHP <= pInfo->wMana) {
					wsprintf (szTmp, "HPが足りません。@2");
					pWndTmp = MakeWindowMSG (szTmp, FALSE);
					SAFE_DELETE (g_pTargetSelectInfo);
				}
			} else if (pChar->wMana < pInfo->wMana) {
				wsprintf (szTmp, "Manaが足りません。@2");
				pWndTmp = MakeWindowMSG (szTmp, FALSE);
				SAFE_DELETE (g_pTargetSelectInfo);
				CancelTurnGage (pChar);

			} else {
				/* 発動種別が「選択」？ */
				if ((pInfo->byTargetType == MAGICTARGETTYPE_SELECT) ||
					(pInfo->byTargetType == MAGICTARGETTYPE_SELECT2)) {
					PWNDINFO pWndNew;
					BOOL bTmp;

					bTmp = TRUE;
					if (pInfo->byTargetType == MAGICTARGETTYPE_SELECT2) {
						bTmp = FALSE;
					}

					if (pWnd->pChild) {
						/* メッセージウィンドウを削除 */
						EraseWindow ((PWNDINFO)pWnd->pChild);
					}

					/* キャラ選択 */
					pWndNew = MakeWindowBATTLE_TARGET (pInfo->byTargetArea, bTmp, pInfo->byTarget);
					if (pWndNew == NULL) {
						pWndTmp = MakeWindowMSG ("対象者がいません。@2", FALSE);
						break;
					}
					pWndNew->pParent = pWnd;
					pWnd->pChild = pWndNew;
					g_WndInfo.Add (pWndNew);
					g_dwActiveMsgID = pWndNew->dwID;
					bClose = FALSE;
					CancelTurnGage (pChar);
				/* 町転送で、選べる行き先が2つ以上 */
				} else if (IsTownWarpMagic ((BYTE)g_aMagicInfoBattle[nData1]) && (GetTownWarpSelectCount () >= 2)) {
					if (pWnd->pChild) {
						/* メッセージウィンドウを削除 */
						EraseWindow ((PWNDINFO)pWnd->pChild);
					}
					CancelTurnGage (pChar);
					/* 行き先の選択（決まったら子ウィンドウ側で詠唱開始） */
					pWndTmp = MakeWindowTOWNWARP (TOWNWARPWND_USE);
					pWndTmp->pParent = pWnd;
					pWnd->pChild = pWndTmp;
					bClose = FALSE;
				/* その他 */
				} else {
					/* 詠唱開始 */
					g_byTownWarpSel = 0;
					g_byComSelectMagic = (BYTE)g_aMagicInfoBattle[nData1];
					pChar->dwTimeTurnMagicLV = timeGetTime ();
					pChar->byTurnMagicLV = 1;
					SendCmdEFECTCHG (pInfo->byUserEfectID, TRUE);
				}
			}
			break;
		}

		if (bClose && g_dwMsgIDCommand) {
			EraseWindow ((PWNDINFO)g_WndInfo[GetWindowID (g_dwMsgIDCommand)]);
			g_dwMsgIDCommand = 0;
		}

		if (pWndTmp) {
			g_bDraw = TRUE;
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_SELECTTEAM											 */
/* 内容：キー入力処理(戦場ゲーム：チーム選択)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void KeyProcSUBWAR_SELECTTEAM(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO	pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		g_bDraw = TRUE;

		SendCmdSubWARSELECTTEAM ((BYTE)pWnd->nData1);

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_WAIT												 */
/* 内容：キー入力処理(戦場ゲーム：他のプレイヤー待ち)						 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void KeyProcSUBWAR_WAIT(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO	pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		g_bDraw = TRUE;

		switch (pWnd->nData1) {
		case 0:
			SendCmdSubWARQUIT ();
			break;
		}

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_SELECTRULE											 */
/* 内容：キー入力処理(戦場ゲーム：ルール選択)								 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void KeyProcSUBWAR_SELECTRULE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO	pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		g_bDraw = TRUE;

		SendCmdSubWARSELECTRULE ((BYTE)pWnd->nData1);

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_SELECTJOB											 */
/* 内容：キー入力処理(戦場ゲーム：職業選択)									 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
void KeyProcSUBWAR_SELECTJOB(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO	pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		g_bDraw = TRUE;

		SendCmdSubWARSELECTJOB ((BYTE)pWnd->nData1);

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_SELECTTURN											 */
/* 内容：キー入力処理(戦場ゲーム：行動順序)									 */
/* 日付：2004/08/11															 */
/* ========================================================================= */
void KeyProcSUBWAR_SELECTTURN(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO	pChar;

		pChar = g_aCharInfo[g_nMyCharID];
		g_bDraw = TRUE;

		SendCmdSubWARSELECTTURN ((BYTE)pWnd->nData1);

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < g_pWarSelectTurn->byCount - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = g_pWarSelectTurn->byCount - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN												 */
/* 内容：キー入力処理(戦場ゲーム：行動選択)									 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		PWNDINFO pWndNew;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		pWndNew = NULL;
		g_bDraw = TRUE;

		switch (pWnd->nData1) {
		case 0:			/* 移動 */
			/* もう歩けない？ */
			if (g_pWarInfo->byMove == 0) {
				break;
			}
			pWndNew = MakeWindowSUBWAR_TURN_MOVE ();
			pWndNew->pParent = pWnd;
			pWnd->pChild = pWndNew;
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;
			break;

		case 1:			/* 攻撃 */
			/* もう歩けない？ */
			if (g_pWarInfo->byMove == 0) {
				break;
			}
			pWndNew = MakeWindowSUBWAR_TURN_ATACK ();
			pWndNew->pParent = pWnd;
			pWnd->pChild = pWndNew;
			g_WndInfo.Add (pWndNew);
			g_dwActiveMsgID = pWndNew->dwID;
			break;

		case 2:			/* 魔法 */
			if (g_pWarInfo->byMove == 0) {
				break;
			}
			/* 魔法使い？ */
			if (pSubStatus->byJob == SUBGAME_WAR_JOB_WITCH) {
				pWndNew = MakeWindowSUBWAR_TURN_MAGIC_WITCH ();

			/* 僧侶？ */
			} else if (pSubStatus->byJob == SUBGAME_WAR_JOB_PRIEST) {
				pWndNew = MakeWindowSUBWAR_TURN_MAGIC_PRIEST ();
			}
			if (pWndNew) {
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
			}
			break;

		case 3:			/* 待機 */
			byData = 0;
			/* 歩いていない？ */
			if (g_pWarInfo->byMove == 2) {
				byData = 1;
			}
			SendCmdSubWARCOMMAND (CMDSUBWAR_WAIT, byData);
			EraseWindow (pWnd);
			break;
		}

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_MOVE											 */
/* 内容：キー入力処理(戦場ゲーム：行動(移動))								 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_MOVE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		BOOL bResult;
		PCHARINFO	pChar;
		PSUBSTATUS_WAR pSubStatus;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;

		/* 移動先にイベントがあるかチェック */
		bResult = CheckBlockEventFront (pChar, pChar->dwX, pChar->dwY, pWnd->nData1);
		/* 橋の上から横へ落ちる・橋の下から橋の端へ出る？ */
		if (IsBridgeMoveBlock (pChar, (BYTE)pWnd->nData1)) {
			bResult = TRUE;
		}
		if (bResult == FALSE) {
			/* イベントが無い時だけ移動させる */
			pChar->byDirection = (BYTE)pWnd->nData1;
			SendCmdCHARPOS (pChar->byDirection, FALSE);
		}

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_ATACK											 */
/* 内容：キー入力処理(戦場ゲーム：行動(攻撃))								 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_ATACK(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		int nTmp;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;

		nTmp = CheckCharBlockPC (
						pChar->dwX + anPosAddTblX[pWnd->nData1],
						pChar->dwY + anPosAddTblY[pWnd->nData1]);

		if (nTmp >= 0) {
			SendCmdSubWARCOMMAND (CMDSUBWAR_ATACK, (BYTE)pWnd->nData1);
			pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
			DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
			EraseWindow ((PWNDINFO)pWnd->pParent);
		}


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_DIFENCE_FIGHTER								 */
/* 内容：キー入力処理(戦場ゲーム：行動(防御：戦士))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_DIFENCE_FIGHTER(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 通常防御 */
			byData = SUBGAME_WAR_DIFENCE_NORMAL;
			break;
		case 1:			/* 見切り */
			byData = SUBGAME_WAR_DIFENCE_AVOID;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_DIFENCE, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_DIFENCE_MARTIAL								 */
/* 内容：キー入力処理(戦場ゲーム：行動(防御：武道家))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_DIFENCE_MARTIAL(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 通常防御 */
			byData = SUBGAME_WAR_DIFENCE_NORMAL;
			break;
		case 1:			/* 受け身 */
			byData = SUBGAME_WAR_DIFENCE_PASSIVE;
			break;
		case 2:			/* カウンター */
			byData = SUBGAME_WAR_DIFENCE_COUNTER;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_DIFENCE, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_DIFENCE_WITCH									 */
/* 内容：キー入力処理(戦場ゲーム：行動(防御：魔法使い))						 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_DIFENCE_WITCH(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 通常防御 */
			byData = SUBGAME_WAR_DIFENCE_NORMAL;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_DIFENCE, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_DIFENCE_PRIEST								 */
/* 内容：キー入力処理(戦場ゲーム：行動(防御：僧侶))							 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_DIFENCE_PRIEST(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 通常防御 */
			byData = SUBGAME_WAR_DIFENCE_NORMAL;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_DIFENCE, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_DIFENCE_WITCH2								 */
/* 内容：キー入力処理(戦場ゲーム：行動(魔法防御：魔法使い))					 */
/* 日付：2004/08/13															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_DIFENCE_WITCH2(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;
		BYTE byData;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 魔法跳ね返し */
			byData = SUBGAME_WAR_DIFENCE_MAGICCOUNTER;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_DIFENCE, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_MAGIC_WITCH									 */
/* 内容：キー入力処理(戦場ゲーム：行動(魔法：魔法使い))						 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_MAGIC_WITCH(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;
	DWORD dwTime = 250;

	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 一覧待ち？ */
	if (pSubStatus->byState == SUBGAME_WAR_STATE_WAITMAGICINFO) {
		return;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		BYTE byData;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 炎 */
			byData = SUBGAME_WAR_MAGIC_FIRE;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_MAGIC_TARGET, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAITMAGICINFO;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_MAGIC_TARGET									 */
/* 内容：キー入力処理(戦場ゲーム：行動(魔法対象者選択))						 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_MAGIC_TARGET(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PSUBSTATUS_WAR pSubStatus;

		pChar = g_aCharInfo[g_nMyCharID];
		pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

		g_bDraw = TRUE;

		SendCmdSubWARUSEMAGIC (g_pWarMagicTarget->byMagic, g_pWarMagicTarget->abyNo[pWnd->nData1]);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAIT;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow ((PWNDINFO)((PWNDINFO)pWnd->pParent)->pParent);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBWAR_TURN_MAGIC_PRIEST									 */
/* 内容：キー入力処理(戦場ゲーム：行動(魔法：僧侶))							 */
/* 日付：2004/08/15															 */
/* ========================================================================= */
void KeyProcSUBWAR_TURN_MAGIC_PRIEST(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	PCHARINFO pChar;
	PSUBSTATUS_WAR pSubStatus;
	DWORD dwTime = 250;

	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 一覧待ち？ */
	if (pSubStatus->byState == SUBGAME_WAR_STATE_WAITMAGICINFO) {
		return;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		BYTE byData;

		g_bDraw = TRUE;
		byData = SUBGAME_WAR_DIFENCE_NORMAL;

		switch (pWnd->nData1) {
		case 0:			/* 回復 */
			byData = SUBGAME_WAR_MAGIC_QURE;
			break;
		case 1:			/* 蘇生 */
			byData = SUBGAME_WAR_MAGIC_RESUSCITATION;
			break;
		}
		SendCmdSubWARCOMMAND (CMDSUBWAR_MAGIC_TARGET, byData);
		pSubStatus->byState = SUBGAME_WAR_STATE_WAITMAGICINFO;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);


	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：ShotTurnGage													 */
/* 内容：ゲージが1周以上溜まっていれば殴る(満タン直後ならジャスト、		 */
/*		 それより前なら早撃ち)。殴ったら TRUE								 */
/* ========================================================================= */
static BOOL ShotTurnGage(PCHARINFO pChar)
{
	if ((pChar->dwTimeTurnLV == 0) || (pChar->byTurnLV < TURNGAGE_LAP)) {
		return FALSE;
	}
	/* 構えたまま殴った：殴ると構えは解ける(サーバーも解く)ので、こちらも解く。
	   解かないと次のゲージが止まったままになり、Z を離した瞬間に満タンになっていた。
	   Z を押したままなら、また GUARD_HOLDTIME たってから構え直す */
	if (g_bGuard) {
		GuardEnd ();
		m_dwTimeZDown = timeGetTime ();
	}
	if (IsTurnGageJust (pChar)) {
		/* ジャスト：満タン直後に押した(会心が出やすく、外れない) */
		SendCmdREQ_ATTACK (pChar->dwAtackTblID, ATACKRATE_JUST);
	} else {
		/* 早撃ち：攻撃要求(威力は溜まりきった周の数×20%) */
		SendCmdREQ_ATTACK (pChar->dwAtackTblID, (BYTE)(pChar->byTurnLV / TURNGAGE_LAP * TURNGAGE_LAP));
	}
	/* 連続攻撃：次のゲージを溜め始める */
	ContinueTurnGage (pChar);
	return TRUE;
}


/* ========================================================================= */
/* 関数名：KeyProcBATTLE_COMMAND											 */
/* 内容：キー入力処理(戦闘：コマンド)										 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
void KeyProcBATTLE_COMMAND(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	int nTmp;
	PCHARINFO pChar;
	DWORD dwTime = 250;

	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}
	pChar = g_aCharInfo[g_nMyCharID];

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (g_dwMsgIDCommand);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;
		g_dwMsgIDCommand = 0;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		/* 連続攻撃中なら、コマンドを閉じてもゲージはそのまま続ける */

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove) &&
				(pWnd->nData1 == 0) && ShotTurnGage (pChar)) {
		/* 「攻撃」：連続攻撃中でゲージが1周以上溜まっていれば、相手を選び直さずにそのまま殴る
		   (攻撃音とかぶるので決定音は鳴らさない) */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;
		g_dwMsgIDCommand = 0;

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		switch (pWnd->nData1) {
		/* 攻撃 */
		case 0:
			{
				int nResult, nLen;
				ARRAYUSECHARINFO aUseCharInfoTmp;
//				PUSECHARINFO pUseCharInfo;

				nTmp = GetAttackType (pChar);
				if (nTmp == ATACKTYPE_ARROW) {
					StartTurnGage (pChar, -1);

					int nNo = GetWindowID (g_dwMsgIDCommand);
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
					g_bDraw = TRUE;
					g_dwMsgIDCommand = 0;
					break;
				}

				nResult = GetUseCharInfo (3, pChar, FALSE, MAGICTARGET_NPC, aUseCharInfoTmp, nLen);
				if (nResult == 0) {
					DeleteUseCharInfo (&aUseCharInfoTmp);
					break;
				}
#if 0
				if (nResult == 1) {
					pUseCharInfo = aUseCharInfoTmp[0];
					StartTurnGage (pChar, pUseCharInfo->dwTblID);
					DeleteUseCharInfo (&aUseCharInfoTmp);

					int nNo = GetWindowID (g_dwMsgIDCommand);
					EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
					g_bDraw = TRUE;
					g_dwMsgIDCommand = 0;
					break;
				}
#endif
				DeleteUseCharInfo (&aUseCharInfoTmp);
				PWNDINFO pWndNew = MakeWindowBATTLE_TARGET (3, FALSE, MAGICTARGET_NPC);
				if (pWndNew == NULL) {
					break;
				}
				pWndNew->pParent	= pWnd;
				pWnd->pChild		= pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 特技 */
		case 1:
			{
				if (pChar->dwTimeTurnMagicLV) {
					break;
				}
				if (g_aMagicInfoBattle.GetSize () == 0) {
					PWNDINFO pWndTmp;
					pWndTmp = MakeWindowMSG ("現在使える特技はありません。@2", FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					CancelTurnGage (pChar);
					break;
				}

				PWNDINFO pWndNew = MakeWindowCOMMANDMAGICBATTLE ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;

		/* 道具 */
		case 2:
			{
				if (GetItemCount (g_nMyCharID) == 0) {
					PWNDINFO pWndTmp;
					pWndTmp = MakeWindowMSG ("何も持っていません。@2", FALSE);
					g_WndInfo.Add (pWndTmp);
					g_dwActiveMsgID = pWndTmp->dwID;
					CancelTurnGage (pChar);
					break;
				}

				PWNDINFO pWndNew = MakeWindowCOMMANDITEMBATTLE ();
				pWndNew->pParent = pWnd;
				pWnd->pChild = pWndNew;
				g_WndInfo.Add (pWndNew);
				g_dwActiveMsgID = pWndNew->dwID;
				g_bDraw = TRUE;
			}
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 + 1 < 3)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = 2;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcBATTLE_TARGET												 */
/* 内容：キー入力処理(戦闘：ターゲット選択)									 */
/* 日付：2004/10/31															 */
/* ========================================================================= */
void KeyProcBATTLE_TARGET(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	PUSECHARINFO pInfo;
	PWNDINFO pWndTmp;
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);
		SAFE_DELETE (g_pTargetSelectInfo);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		int nNo = GetWindowID (g_dwMsgIDCommand);

		pWndTmp = (PWNDINFO)pWnd->pParent;
		pChar = g_aCharInfo[g_nMyCharID];

		/* 親ウィンドウは戦闘コマンド？ */
		if (pWndTmp->nType == WNDTYPE_BATTLE_COMMAND) {
			/* 攻撃 */
			if (pWndTmp->nData1 == 0) {
				pInfo = g_aUseCharInfo[pWnd->nData1];
				StartTurnGage (pChar, pInfo->dwTblID);
				SendCmdTARGETCHAR (pInfo->dwTblID);
			}
		/* 親ウィンドウは魔法選択？ */
		} else if ((pWndTmp->nType == WNDTYPE_COMMANDMAGICBATTLE) ||
					(pWndTmp->nType == WNDTYPE_COMMANDMAGIC)) {
			PMAGICINFO pMagicInfo;

			/* 詠唱開始 */
			g_byTownWarpSel = 0;
			switch (pWndTmp->nType) {
			case WNDTYPE_COMMANDMAGICBATTLE:
				g_byComSelectMagic = (BYTE)g_aMagicInfoBattle[pWndTmp->nData1];
				break;
			case WNDTYPE_COMMANDMAGIC:
				g_byComSelectMagic = (BYTE)g_aMagicInfoWalk[pWndTmp->nData1];
				break;
			}
			pChar->dwTimeTurnMagicLV = timeGetTime ();
			pChar->byTurnMagicLV = 1;
			pMagicInfo = g_aMagicInfo[g_byComSelectMagic];
			SendCmdEFECTCHG (pMagicInfo->byUserEfectID, TRUE);
		}

		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;
		g_dwMsgIDCommand = 0;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 + 1 < pWnd->nData2)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pInfo = g_aUseCharInfo[pWnd->nData1];
			g_pTargetSelectInfo->dwTblID = pInfo->dwTblID;

			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcTOWNWARP													 */
/* 内容：キー入力処理(町転送：行き先の選択)									 */
/* ========================================================================= */
void KeyProcTOWNWARP(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	/* 教会で記録する時の「どの町を忘れますか？」：決定で記録、キャンセルで記録しない */
	if ((pWnd->nType2 == TOWNWARPWND_FORGET) && (byEvent == KEY_UP) && ((byCode == 'A') || (byCode == 'B'))) {
		PWNDINFO pWndMsg = (PWNDINFO)pWnd->pParent;
		int nPos = (int)pWnd->dwData, nForget = -1;

		if ((byCode == 'A') && (pWnd->nData1 >= 0) && (pWnd->nData1 <= MAX_TOWNWARP)) {
			nForget = (int)g_TownWarpInfo.adwMapID[pWnd->nData1];
		}
		EraseWindow (pWnd);
		StartPosFinish (pWndMsg, (byCode == 'A'), nPos, nForget);
		return;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		/* この窓だけ閉じて、特技の一覧に戻る */
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar;
		PWNDINFO pWndTmp;
		PMAGICINFO pMagicInfo;
		int nNo = GetWindowID (g_dwMsgIDCommand);
		BOOL bStart = FALSE;

		pWndTmp = (PWNDINFO)pWnd->pParent;
		pChar = g_aCharInfo[g_nMyCharID];

		/* 親ウィンドウは魔法選択？ */
		if (pWndTmp) {
			switch (pWndTmp->nType) {
			case WNDTYPE_COMMANDMAGICBATTLE:
				if ((pWndTmp->nData1 >= 0) && (pWndTmp->nData1 < g_aMagicInfoBattle.GetSize ())) {
					g_byComSelectMagic = (BYTE)g_aMagicInfoBattle[pWndTmp->nData1];
					bStart = TRUE;
				}
				break;
			case WNDTYPE_COMMANDMAGIC:
				if ((pWndTmp->nData1 >= 0) && (pWndTmp->nData1 < g_aMagicInfoWalk.GetSize ())) {
					g_byComSelectMagic = (BYTE)g_aMagicInfoWalk[pWndTmp->nData1];
					bStart = TRUE;
				}
				break;
			}
		}
		if (bStart) {
			/* 選んだ行き先で詠唱開始 */
			g_byTownWarpSel = (BYTE)pWnd->nData1;
			pChar->dwTimeTurnMagicLV = timeGetTime ();
			pChar->byTurnMagicLV = 1;
			pMagicInfo = g_aMagicInfo[g_byComSelectMagic];
			SendCmdEFECTCHG (pMagicInfo->byUserEfectID, TRUE);
		}

		/* コマンドごと閉じる（この窓も親の窓も一緒に消える） */
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
		} else {
			EraseWindow (pWnd);
		}
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 + 1 < pWnd->nData2)) {
			pWnd->nData1 ++;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBSUGOROKU_JOINMODE										 */
/* 内容：キー入力処理(すごろく：参加モード選択)								 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
void KeyProcSUBSUGOROKU_JOINMODE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
		g_bDraw = TRUE;

		SendCmdSubSUGOROKUJOINMODE ((BYTE)pWnd->nData1);
		pSubStatus->byState = SUBGAME_SUGOROKU_STATE_IDLE;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBSUGOROKU_COMMAND										 */
/* 内容：キー入力処理(すごろく：行動選択)									 */
/* 日付：2004/12/28															 */
/* ========================================================================= */
void KeyProcSUBSUGOROKU_COMMAND(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
		g_bDraw = TRUE;

		switch (pWnd->nData1) {
		case 0:		/* サイコロを振る */
			SendCmdSubSUGOROKUDICE ();
			pSubStatus->byState = SUBGAME_SUGOROKU_STATE_IDLE;
			break;

		case 1:		/* すごろくから抜ける */
			{
				char szTmp[128];
				PWNDINFO pWndTmp = NULL, pWndTmp2 = NULL;
				/* 確認ウィンドウ作成 */
				pWndTmp = MakeWindowCONF (16 * 20, 16 * 10, IDCONFIRMTYPE_BREAKSUGOROKU);
				pWndTmp->nData1 = 1;
				pWndTmp->pParent = pWnd;
				pWnd->pChild = pWndTmp;
				g_WndInfo.Add (pWndTmp);
				g_dwActiveMsgID = pWndTmp->dwID;

				wsprintf (szTmp, "すごろくから抜けますか？@2");
				pWndTmp2 = MakeWindowMSG (szTmp, FALSE);
				pWndTmp2->pParent = pWndTmp;
				pWndTmp->pChild = pWndTmp2;
				g_WndInfo.Add (pWndTmp2);
			}
			break;
		}

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		EraseWindow (pWnd);

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：KeyProcSUBSUGOROKU_IDLE											 */
/* 内容：キー入力処理(すごろく：平常状態)									 */
/* 日付：2004/12/25															 */
/* ========================================================================= */
void KeyProcSUBSUGOROKU_IDLE(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	DWORD dwTime = 250;
	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		int nNo = GetWindowID (pWnd->dwID);
		EraseWindow (pWnd);
		g_bDraw = TRUE;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
		PSUBSTATUS_SUGOROKU pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;
		g_bDraw = TRUE;

		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);

		{
			char szTmp[128];
			PWNDINFO pWndTmp = NULL, pWndTmp2 = NULL;
			/* 確認ウィンドウ作成 */
			pWndTmp = MakeWindowCONF (16 * 20, 16 * 10, IDCONFIRMTYPE_BREAKSUGOROKU);
			pWndTmp->nData1 = 1;
			pWndTmp->pParent = pWnd;
			pWnd->pChild = pWndTmp;
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;

			wsprintf (szTmp, "すごろくから抜けますか？@2");
			pWndTmp2 = MakeWindowMSG (szTmp, FALSE);
			pWndTmp2->pParent = pWndTmp;
			pWndTmp->pChild = pWndTmp2;
			g_WndInfo.Add (pWndTmp2);
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 < pWnd->nData2 - 1)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = pWnd->nData2 - 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}


/* ========================================================================= */
/* 関数名：	KeyProcDEAD_COMMAND												 */
/* 内容：	キー入力処理(死亡中：コマンド)									 */
/* 日付：	2005/11/01														 */
/* ========================================================================= */

void KeyProcDEAD_COMMAND(PWNDINFO pWnd, BYTE byEvent, BYTE byCode)
{
	int nNo;
	DWORD dwTime = 250;

	if (pWnd->byRepeate > 1) {
		dwTime = 125;
	}

	if ((byEvent == KEY_UP) && (byCode == 'B')) {
		nNo = GetWindowID (g_dwMsgIDCommand);
		EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
		g_bDraw = TRUE;
		g_dwMsgIDCommand = 0;
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_PUSH51_C]);

	} else if ((byEvent == KEY_UP) && (byCode == 'A') && (g_bEnableMove)) {
		DXAudio.PlaySecoundary (&aSoundTbl[SOUND_OK]);
		pWnd->dwTimeDrawStart = 0;
		pWnd->byAnime = 1;

		switch (pWnd->nData1) {
		/* この場で助けを待つ */
		case 0:
			nNo = GetWindowID (g_dwMsgIDCommand);
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
			g_dwMsgIDCommand = 0;
			break;

		/* 記録した場所で復活する */
		case 1:
			SendCmdRESURRECTION ();
			nNo = GetWindowID (g_dwMsgIDCommand);
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
			g_dwMsgIDCommand = 0;
			break;
		}

	} else if ((byEvent == KEY_DOWN) && (timeGetTime () > pWnd->dwTimeKey + dwTime)) {
		BOOL bMove = FALSE;

		if ((byCode == VK_UP) && (pWnd->nData1 > 0)) {
			pWnd->nData1 --;
			bMove = TRUE;

		} else if ((byCode == VK_DOWN) && (pWnd->nData1 + 1 < 2)) {
			pWnd->nData1 ++;
			bMove = TRUE;

		} else if (byCode == VK_LEFT) {
			pWnd->nData1 = 0;
			bMove = TRUE;

		} else if (byCode == VK_RIGHT) {
			pWnd->nData1 = 1;
			bMove = TRUE;
		}

		if (bMove) {
			pWnd->dwTime = timeGetTime ();
			pWnd->byAnime = 1;
			pWnd->byRepeate ++;
			pWnd->dwTimeKey = timeGetTime ();
			pWnd->dwTimeDrawStart = 0;
			g_bDraw = TRUE;
		}

	} else if (byEvent == KEY_UP) {
		pWnd->dwTimeKey = timeGetTime () - 250;
		pWnd->byRepeate = 0;
		g_bDraw = TRUE;
	}
}


/* ========================================================================= */
/* 関数名：AccWndProc														 */
/* 内容：アカウント名入力用メッセージ処理									 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
LRESULT CALLBACK AccWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_TAB) {
			if (GetKeyState (VK_SHIFT) & 0x8000) {
				SetFocus (g_hWndCon);
			} else {
				SetFocus (g_hWndPas);
			}
			return 0;

		} else if (wp == VK_RETURN) {
			PostMessage (g_hWndMain, WM_COMMAND, IDC_CONNECT, 0);
			return 0;
		}
	}
	return (CallWindowProc (g_OrgWndProcAcc, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：PasWndProc														 */
/* 内容：パスワード入力用メッセージ処理										 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
LRESULT CALLBACK PasWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_TAB) {
			if (GetKeyState (VK_SHIFT) & 0x8000) {
				SetFocus (g_hWndAcc);
			} else {
				SetFocus (g_hWndCon);
			}
			return 0;

		} else if (wp == VK_RETURN) {
			PostMessage (g_hWndMain, WM_COMMAND, IDC_CONNECT, 0);
			return 0;
		}
	}
	return (CallWindowProc (g_OrgWndProcPas, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：ConWndProc														 */
/* 内容：接続ボタン用メッセージ処理											 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
LRESULT CALLBACK ConWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_TAB) {
			if (GetKeyState (VK_SHIFT) & 0x8000) {
				SetFocus (g_hWndPas);
			} else {
				SetFocus (g_hWndAcc);
			}
		}
	}
	return (CallWindowProc (g_OrgWndProcCon, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：NameWndProc														 */
/* 内容：キャラ名入力用メッセージ処理										 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
LRESULT CALLBACK NameWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_TAB) {
			SetFocus (g_hWndMake);
			return 0;

		} else if (wp == VK_RETURN) {
			PostMessage (g_hWndMain, WM_COMMAND, IDC_MAKE, 0);
			return 0;
		}
	}
	return (CallWindowProc (g_OrgWndProcAcc, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：MakeWndProc														 */
/* 内容：作成ボタン用メッセージ処理											 */
/* 日付：2003/07/29															 */
/* ========================================================================= */
LRESULT CALLBACK MakeWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_TAB) {
			SetFocus (g_hWndName);
		}
	}
	return (CallWindowProc (g_OrgWndProcCon, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：	IsSpeechModeEnable												 */
/* 内容：	発言の種類(g_bySpeechMode)が今選べるか							 */
/*			0:普通 1:画面内放送 2:マップ内放送 3:パーティ					 */
/* 日付：	2026/11/09														 */
/* ========================================================================= */
static BOOL IsSpeechModeEnable(BYTE byMode)
{
	switch (byMode) {
	case 1:
	case 2:
		/* 放送できるマップだけ */
		return ((g_pMapInfo != NULL) && (g_pMapInfo->byFlg & MAPFLG_SPEECH)) ? TRUE : FALSE;
	case 3:
		/* パーティに入っている時だけ */
		return (g_aPartyInfo.GetSize () > 0) ? TRUE : FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：SpeechWndProc													 */
/* 内容：発言入力欄用メッセージ処理											 */
/* 日付：2003/05/24															 */
/* ========================================================================= */
LRESULT CALLBACK SpeechWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_RETURN) {
			return 0;
		}
		break;

	case WM_KEYDOWN:
		if (wp == VK_RETURN) {
			PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
			int i;
			for (i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* 発言ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_SPEECH) {
#ifdef __EMSCRIPTEN__
					/* ブラウザ版：長い発言はチャット欄と同じように分けて順に送る */
					{
						int nLen = GetWindowTextLength (hWnd);
						LPSTR pszBuf = new char[nLen + 2];
						ZeroMemory (pszBuf, nLen + 2);
						GetWindowText (hWnd, pszBuf, nLen + 1);
						/* 全体発言（Shift）・管理者用内線（Ctrl）は、分けた発言すべてに効かせる */
						int nKeyMod = 0;
						if (GetKeyState (VK_SHIFT) & 0x8000) {
							nKeyMod = 1;
						} else if (GetKeyState (VK_CONTROL) & 0x8000) {
							nKeyMod = 2;
						}
						SboWebSpeechSubmit (pszBuf, nKeyMod);
						SAFE_DELETE_ARRAY (pszBuf);
					}
#else
					char szBuf[42];
					ZeroMemory (szBuf, sizeof (szBuf));
					GetWindowText (hWnd, szBuf, 41);
					ChatMsgProc (szBuf);
#endif

					EraseWindow (g_WndInfo[i]);
					g_bSpeech = FALSE;
					KeyProc ();
					break;
				}
			}

		} else if (wp == VK_ESCAPE) {
			for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* 発言ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_SPEECH) {
					EraseWindow (g_WndInfo[i]);
					g_bSpeech = FALSE;
					KeyProcMove (KEY_UP, VK_RETURN);
					break;
				}
			}
			break;

		} else if ((wp == VK_UP) || (wp == VK_DOWN)) {
			/* 発言の種類を切り替える。0:普通 1:画面内放送 2:マップ内放送 3:パーティ
			   放送は放送できるマップだけ、パーティはパーティに入っている時だけ選べる */
			{
				int nMode = g_bySpeechMode, nTry;
				for (nTry = 0; nTry < SPEECHMODE_MAX; nTry ++) {
					if (wp == VK_UP) {
						nMode = (nMode + SPEECHMODE_MAX - 1) % SPEECHMODE_MAX;
					} else {
						nMode = (nMode + 1) % SPEECHMODE_MAX;
					}
					if (IsSpeechModeEnable ((BYTE)nMode)) {
						break;
					}
				}
				if (nMode != g_bySpeechMode) {
					g_bySpeechMode = (BYTE)nMode;
					int i;
					for (i = 0; i < g_WndInfo.GetSize (); i ++) {
						/* 発言ウィンドウ検索 */
						if (g_WndInfo[i]->nType == WNDTYPE_SPEECH) {
							g_WndInfo[i]->dwTimeDrawStart = 0;
						}
					}
				}
			}
		}
	}
	return (CallWindowProc (g_OrgWndProcSpeech, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：SpeechEditWndProc												 */
/* 内容：自己紹介設定欄用メッセージ処理										 */
/* 日付：2003/09/26															 */
/* ========================================================================= */
LRESULT CALLBACK SpeechEditWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_RETURN) {
			return 0;
		}
		break;

	case WM_KEYDOWN:
		if (wp == VK_RETURN) {
			for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* 発言ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_SPEECHEDIT) {
					int k, j, nSize = 601;
					LPSTR pszTmp = new char[nSize], pszTmp2 = new char[nSize];
					ZeroMemory (pszTmp, nSize);
					ZeroMemory (pszTmp2, nSize);

					GetWindowText (g_hWndSpeech, pszTmp, 601);
					pszTmp[600] = 0;

					for (k = 0, j = 0; k < 600; k ++, j ++) {
						pszTmp2[j] = pszTmp[k];
						/* 使ってはいけない@チェック */
						if (pszTmp[k] == '@') {
							switch (pszTmp[k + 1]) {
							case '0':
							case '1':
							case '2':
								break;
							default:
								{
									/* @を空白に置き換え */
									pszTmp2[j] = ' ';
								}
								break;
							}
						}
						/* 2 バイト目以降は文字の一部なのでそのまま写す */
						for (int n = SboTextCharLen (&pszTmp[k]); n > 1; n --) {
							j ++;
							k ++;
							pszTmp2[j] = pszTmp[k];
						}
					}

					pszTmp[600] = 0;
					PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
					SAFE_DELETE_ARRAY (pChar->pszNpcSpeech);
					pChar->pszNpcSpeech = pszTmp2;
					SendCmdSPEECH (pszTmp2, SPEECHCMD_SPEECHEDIT);

					SAFE_DELETE_ARRAY (pszTmp);
					EraseWindow (g_WndInfo[i]);
					g_bDraw = TRUE;
					break;
				}
			}

		} else if (wp == VK_ESCAPE) {
			for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* 発言ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_SPEECHEDIT) {
					EraseWindow (g_WndInfo[i]);
					break;
				}
			}
			break;
		}
	}
	return (CallWindowProc (g_OrgWndProcSpeech, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：ChgPasswordWndProc												 */
/* 内容：パスワード変更用メッセージ処理										 */
/* 日付：2004/02/01															 */
/* ========================================================================= */
LRESULT CALLBACK ChgPasswordWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CHAR:
		if (wp == VK_RETURN) {
			return 0;
		}
		break;

	case WM_KEYDOWN:
		if (wp == VK_RETURN) {
			for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_CHGPASSWORD) {
					char szTmp[MAX_ACCNAME];
					ZeroMemory (szTmp, MAX_ACCNAME);

					GetWindowText (g_hWndSpeech, szTmp, MAX_ACCNAME);
					PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
					SendCmdCHGPASSWORD (pChar->dwTblID, szTmp);

					EraseWindow ((PWNDINFO)g_WndInfo[i]);
					g_bDraw = TRUE;
				}
			}
			break;

		} else if (wp == VK_ESCAPE) {
			for (int i = 0; i < g_WndInfo.GetSize (); i ++) {
				/* 発言ウィンドウ検索 */
				if (g_WndInfo[i]->nType == WNDTYPE_CHGPASSWORD) {
					EraseWindow (g_WndInfo[i]);
					break;
				}
			}
			break;
		}
	}
	return (CallWindowProc (g_OrgWndProcSpeech, hWnd, msg, wp, lp));
}

/* ========================================================================= */
/* 関数名：MakeNpcSpeechNextExp												 */
/* 内容：復活地点を記録する神父さん（セリフに「@8」がある人）のセリフの		 */
/*		 「@8」の直前に、次のレベルまでに必要な経験値のお告げを足す			 */
/*		 「はい・いいえ」で記録を選ぶ場合も、聞く前に言うのでどちらでも聞ける */
/*		 戻り値：新しいセリフ（呼んだ側で delete[]）。足さない時は NULL		 */
/* ========================================================================= */
LPSTR MakeNpcSpeechNextExp(LPCSTR pszSpeech)
{
	char szMsg[256];
	int i, nPos, nLen;
	PCHARINFO pChar;
	PJOBINFO pJobInfo;

	if ((pszSpeech == NULL) || (g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return NULL;
	}

	/* 「@8」＋数字 9 桁（マップ・X・Y）を探す */
	nLen = (int)strlen (pszSpeech);
	nPos = -1;
	for (i = 0; i + 11 <= nLen; i ++) {
		if ((pszSpeech[i] == '@') && (pszSpeech[i + 1] == '8')) {
			int j;
			for (j = 0; j < 9; j ++) {
				if ((pszSpeech[i + 2 + j] < '0') || (pszSpeech[i + 2 + j] > '9')) {
					break;
				}
			}
			if (j == 9) {
				nPos = i;
				break;
			}
		}
	}
	if (nPos < 0) {
		return NULL;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->byJOB >= g_JobInfo.GetCount ()) {
		return NULL;
	}
	pJobInfo = g_JobInfo.GetPtr (pChar->byJOB);
	if (pJobInfo == NULL) {
		return NULL;
	}

	if (pChar->byLevel >= pJobInfo->aLevelUpInfo.GetSize ()) {
		/* 最高レベル（これ以上は経験値も増えない） */
		_snprintf (szMsg, sizeof (szMsg), "@0%sさんは、もうこれ以上ないほど@1修行を積まれましたね。", pChar->szName);
	} else {
		/* aLevelUpInfo[今のレベル] が次のレベルに必要な累計経験値（サーバーの AddExp と同じ） */
		PLVUPINFO pLvUpInfo = pJobInfo->aLevelUpInfo[pChar->byLevel];
		DWORD dwRest = 0;
		if (pLvUpInfo && (pLvUpInfo->dwExp > pChar->dwExp)) {
			dwRest = pLvUpInfo->dwExp - pChar->dwExp;
		}
		_snprintf (szMsg, sizeof (szMsg), "@0%sさんが次のレベルになるには、@1あと %u の経験値が必要です。", pChar->szName, (unsigned)dwRest);
	}
	szMsg[sizeof (szMsg) - 1] = 0;

	/* 「(ここまでのセリフ)(お告げ)@8...(続き)」に組み替える */
	int nLenMsg = (int)strlen (szMsg);
	LPSTR pszNew = new char[nLen + nLenMsg + 1];
	CopyMemory (pszNew, pszSpeech, nPos);
	CopyMemory (&pszNew[nPos], szMsg, nLenMsg);
	CopyMemory (&pszNew[nPos + nLenMsg], &pszSpeech[nPos], nLen - nPos);
	pszNew[nLen + nLenMsg] = 0;
	return pszNew;
}

/* ========================================================================= */
/* 関数名：CmdProcHanasu													 */
/* 内容：コマンド処理(話す)													 */
/* 日付：2003/07/12															 */
/* ========================================================================= */
void CmdProcHanasu(PWNDINFO pWnd)
{
	PWNDINFO pWndTmp = NULL;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID], pChar2;
	DWORD dwX, dwY, dwCRC;
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	/* Sキーから呼ばれた時はコマンドウィンドウが無いので pWnd は NULL */
	if (pWnd) {
		pWnd->byAnime = 1;
		pWnd->dwTimeDrawStart = 0;
	}

	int i, nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar2 = (PCHARINFO)g_aCharInfo[i];
		if ((dwX == pChar2->dwX) && (dwY == pChar2->dwY)) {
			break;
		}
	}

	PMAPPARTSINFO pParts = &g_pMapPartsInfo[g_pMapInfo->pwMap[dwX + dwY * g_pMapInfo->nCX]];
	/* １歩前にキャラが見つからなかった？ */
	if (i == nCount) {
		if (pParts->byHitType == PARTSHIT_COUNTER) {
			dwX += anPosAddTblX[pChar->byDirection];
			dwY += anPosAddTblY[pChar->byDirection];
		}
		for (i = 0; i < nCount; i ++) {
			pChar2 = (PCHARINFO)g_aCharInfo[i];
			if ((dwX == pChar2->dwX) && (dwY == pChar2->dwY)) {
				break;
			}
		}
	}

	if ((i == nCount) || (pChar2->byNpcType == NPCTYPE_PUT)) {
		pWndTmp = MakeWindowMSG ("その方向には誰もいません。@2", FALSE);
		goto Exit;
	}

	if (pChar2->pszNpcSpeech) {
		dwCRC = g_crc.GetCRC ((PBYTE)pChar2->pszNpcSpeech, strlen (pChar2->pszNpcSpeech) + 1);
		if (pChar2->dwSpeechCRC != dwCRC) {
			/* あやしいのでそっと終了。 */
			PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
			goto Exit;
		}
	}

	/* 得点板？ */
	if ((pChar2->byNpcType == NPCTYPE_SCORE) ||
				(pChar2->byNpcType == NPCTYPE_CHGPUSH)) {
		SendCmdADDSCORE (pChar2->dwTblID);
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}
		return;

	/* 話すとランダム？ */
	} else if (pChar2->byNpcType == NPCTYPE_TALKRAND) {
		SendCmdTALKRAND (pChar2->dwTblID);
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}
		return;

	/* 止まるかランダム？ */
	} else if (pChar2->byNpcType == NPCTYPE_STOPRAND) {
		SendCmdSTOPRAND (pChar2->dwTblID);
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}
		return;

	} else if (!pChar2->pszNpcSpeech || (pChar2->pszNpcSpeech && !strlen (pChar2->pszNpcSpeech))) {
		pWndTmp = MakeWindowMSG ("返事がないようです。@2", FALSE);

	/* 変身？ */
	} else if ((strlen (pChar2->pszNpcSpeech) == 1) && (pChar2->pszNpcSpeech[0] == '@') &&
			(pChar2->byNpcType >= NPCTYPE_NOMOVE)) {
		PWNDINFO pWndParent = NULL, pWndTmp = NULL, pWndTmp2 = NULL;

		/* 透明キャラに変わろうとしている？ */
		if (pChar2->wGrpID == 0) {
			/* あやしいのでそっと終了。 */
			PostMessage (g_hWndMain, WM_CLOSE, 0, 0);
			return;
		}
		/* Sキーから呼ばれた時はコマンドウィンドウが無い */
		int nNoCommand = GetWindowID (g_dwMsgIDCommand);
		if (nNoCommand >= 0) {
			pWndParent = (PWNDINFO)g_WndInfo[nNoCommand];
		}

		/* 確認ウィンドウ作成 */
		pWndTmp = MakeWindowCONF (16 * 20, 16 * 10, IDCONFIRMTYPE_GRPCHANGE);
		pWndTmp->nData1 = 1;
		pWndTmp->nData2 = (int)pChar2->wGrpID;
		pWndTmp->pParent = pWndParent;
		if (pWndParent) {
			pWndParent->pChild = pWndTmp;
		}
		g_WndInfo.Add (pWndTmp);
		g_dwActiveMsgID = pWndTmp->dwID;

		pWndTmp2 = MakeWindowMSG ("着せ替えますか？@2", FALSE);
		pWndTmp2->pParent = pWndTmp;
		pWndTmp->pChild = pWndTmp2;
		g_WndInfo.Add (pWndTmp2);
		return;

	/* 店？ */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == '5') {
		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
		}

		char szTmp[3];
		szTmp[0] = pChar2->pszNpcSpeech[2];
		szTmp[1] = pChar2->pszNpcSpeech[3];
		szTmp[2] = 0;
		pWndTmp = MakeWindowGOLD ();
		g_WndInfo.Add (pWndTmp);
		pWndTmp = MakeWindowSHOP (atoi (szTmp), SHOPMSG_OPEN);
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

	/* 預かり所？ */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == '6') {
		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
		}
		pWndTmp = MakeWindowLEAVE (0);
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

	/* 店(売るだけ)？ */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == '7') {
		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
		}

		char szTmp[3];
		szTmp[0] = pChar2->pszNpcSpeech[2];
		szTmp[1] = pChar2->pszNpcSpeech[3];
		szTmp[2] = 0;
		pWndTmp = MakeWindowGOLD ();
		g_WndInfo.Add (pWndTmp);
		pWndTmp = MakeWindowSHOPSELL (atoi (szTmp));
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

		g_WndInfo.Add (pWndTmp);
		g_dwActiveMsgID = pWndTmp->dwID;

		PSHOPINFO pShop = &g_pMapInfo->pShopInfo[atoi (szTmp)];
		if (pShop->awItem[0] > 0) {
			PWNDINFO pWndItemInfo = MakeWindowITEMINFO (16 * 19, 16 * (pShop->byCountItem + 5));
			pWndTmp->pWndTmp = pWndItemInfo;
			pWndItemInfo->nData1 = pShop->awItem[0];
			g_WndInfo.Add (pWndItemInfo);
		}
		pWndTmp = NULL;

	/* 作り手(レシピ)？ @cNN の NN が作り手の番号 */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == 'c') {
		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
		}
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

		int nMaker = -1;
		LPCSTR pszSp = pChar2->pszNpcSpeech;
		if ((strlen (pszSp) >= 4) && (pszSp[2] >= '0') && (pszSp[2] <= '9') && (pszSp[3] >= '0') && (pszSp[3] <= '9')) {
			nMaker = (pszSp[2] - '0') * 10 + (pszSp[3] - '0');
		}
		int anList[MAX_RECIPE];
		if ((nMaker < 0) || (nMaker >= MAX_MAKER) || (nMaker >= g_nMakerCount) ||
			(g_aMakerInfo[nMaker].byState != RECIPESTATE_ON)) {
			/* 作り手が登録されていない */
			pWndTmp = MakeWindowMSG ("返事がないようです。@2", FALSE);

		} else if (MakerGetRecipeList (nMaker, anList, MAX_RECIPE) == 0) {
			/* 作れる物が無い */
			pWndTmp = MakeWindowMSG ("今は作れる物がないようです。@2", TRUE);

		} else {
			/* 一覧・台詞・横の詳細を出し、預かり所の中身を取りに行く */
			/* 所持金はお店と同じく右上の別の窓に出す（一覧を閉じた時に一緒に消す） */
			g_WndInfo.Add (MakeWindowGOLD ());
			PWNDINFO pWndList = MakeWindowMAKER (nMaker);
			g_WndInfo.Add (pWndList);
			MakerSetTalkMsg (pWndList);
			MakeWindowITEMINFOList (pWndList);
			g_dwActiveMsgID = pWndList->dwID;
			g_bDraw = TRUE;
			SendCmdLEAVEITEMGET ((DWORD)-1);
			pWndTmp = NULL;
		}

	/* 傭兵所？ 雇う・お別れするのはサーバーが決めて、お知らせが来る */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == 'm') {
		SendCmdTALKRAND (pChar2->dwTblID);
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_dwMsgIDCommand = 0;
			g_bDraw = TRUE;
		}
		return;

	/* 冒険者登録所？ */
	} else if (pChar2->pszNpcSpeech[0] == '@' && pChar2->pszNpcSpeech[1] == 'a') {
		/* コマンドウィンドウを閉じる */
		int nNo = GetWindowID (g_dwMsgIDCommand);
		if (nNo >= 0) {
			EraseWindow ((PWNDINFO)g_WndInfo[nNo]);
			g_bDraw = TRUE;
		}
		pWndTmp = MakeWindowJOB (0);
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

	} else {
		/* 復活地点を記録する神父さんなら、次のレベルまでのお告げを足す */
		LPSTR pszNextExp = MakeNpcSpeechNextExp (pChar2->pszNpcSpeech);
		if (pszNextExp) {
			pWndTmp = MakeWindowMSG (pszNextExp, TRUE);
			SAFE_DELETE_ARRAY (pszNextExp);
		} else {
			pWndTmp = MakeWindowMSG (pChar2->pszNpcSpeech, TRUE);
		}
		pChar2->byDirection = abyDirectionTbl[pChar->byDirection];

		if ((pChar2->byNpcType == NPCTYPE_SEARCH) ||
			(pChar2->byNpcType == NPCTYPE_SEARCH2)) {
			SendCMDTARGETCHG (pChar2->dwTblID);
		}
	}

Exit:
	if (pWndTmp) {
		g_WndInfo.Add (pWndTmp);
		g_dwActiveMsgID = pWndTmp->dwID;
	}
}

/* ========================================================================= */
/* 関数名：GetFrontTalkChar													 */
/* 内容：話しかけられる目の前のキャラを探す									 */
/*		 CmdProcHanasu と同じく、目の前に誰もいなくてカウンターなら			 */
/*		 その向こうも見る。置物(NPCTYPE_PUT)は話しかけられないので除く		 */
/*		 戻り値：キャラ番号（見つからない時は -1）							 */
/* ========================================================================= */
int GetFrontTalkChar(void)
{
	PCHARINFO pChar, pChar2;
	DWORD dwX, dwY;
	int i, nCount, nPass;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return -1;
	}
	if ((g_pMapInfo == NULL) || (g_pMapInfo->pwMap == NULL)) {
		return -1;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	nCount = g_aCharInfo.GetSize ();

	for (nPass = 0; nPass < 2; nPass ++) {
		/* マップの外は見ない */
		if ((dwX >= (DWORD)g_pMapInfo->nCX) || (dwY >= (DWORD)g_pMapInfo->nCY)) {
			return -1;
		}
		for (i = 0; i < nCount; i ++) {
			pChar2 = (PCHARINFO)g_aCharInfo[i];
			if (i == g_nMyCharID) {
				continue;
			}
			if ((dwX == pChar2->dwX) && (dwY == pChar2->dwY)) {
				break;
			}
		}
		if (i < nCount) {
			if (pChar2->byNpcType == NPCTYPE_PUT) {
				return -1;
			}
			return i;
		}
		/* 目の前がカウンターならその向こうを見る */
		if (nPass == 0) {
			PMAPPARTSINFO pParts = &g_pMapPartsInfo[g_pMapInfo->pwMap[dwX + dwY * g_pMapInfo->nCX]];
			if (pParts->byHitType != PARTSHIT_COUNTER) {
				return -1;
			}
			dwX += anPosAddTblX[pChar->byDirection];
			dwY += anPosAddTblY[pChar->byDirection];
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetFrontScoreChar												 */
/* 内容：目の前のマスにいる得点板（雪合戦場の HP パネルなど）を探す			 */
/*		 Z キーで名前を付ける（2006 年に Z が「ついていく」になってから	 */
/*		 付けられなくなっていたのを戻す）									 */
/*		 戻り値：キャラ番号（いない時は -1）								 */
/* ========================================================================= */
int GetFrontScoreChar(void)
{
	PCHARINFO pChar, pChar2;
	DWORD dwX, dwY;
	int i, nCount;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return -1;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar2 = g_aCharInfo[i];
		if ((pChar2->byNpcType == NPCTYPE_SCORE) &&
			CheckCharX (pChar2, pChar2->dwX, dwX) && (pChar2->dwY == dwY)) {
			return i;
		}
	}
	return -1;
}

/* ========================================================================= */
/* 関数名：GetSKeyTalkChar													 */
/* 内容：Sキーで話しかける相手を探す										 */
/*		 掲示板（お知らせ掲示板・立て札）は動かない NPC の足元に「調べる」	 */
/*		 メッセージが置いてあり、話すと「立て札は話せない」などになる。		 */
/*		 目の前のマスにイベント（扉・掲示板など）がある時は、誰が立っていても	 */
/*		 話さずに調べる（掲示板が読める・扉の所に人がいても扉を開けられる）		 */
/*		 投げた雪玉など、話す相手ではない NPC も除く							 */
/*		 戻り値：キャラ番号（話さない時は -1）								 */
/* ========================================================================= */
int GetSKeyTalkChar(void)
{
	PCHARINFO pChar, pChar2;
	DWORD dwX, dwY;
	int nChar;

	nChar = GetFrontTalkChar ();
	if (nChar < 0) {
		return -1;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	pChar2 = g_aCharInfo[nChar];
	/* 投げた雪玉・魔法の発動役・爆弾ゲームの爆弾などは話し相手にしない */
	switch (pChar2->byNpcType) {
	case NPCTYPE_ATACK:				/* 重なったキャラに攻撃（投げた雪玉など） */
	case NPCTYPE_USEMAGIC:			/* 重なったキャラに魔法発動 */
	case NPCTYPE_USEMAGICPARENT:	/* 魔法発動し続ける */
	case NPCTYPE_BOMB:				/* 爆弾 */
	case NPCTYPE_BOMBBLOCK:			/* 爆弾ゲームの壁 */
	case NPCTYPE_BOMBITEM:			/* 爆弾ゲームのアイテム */
		return -1;
	}
	if (g_pMapInfo->pbyFlg == NULL) {
		return nChar;
	}
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	/* 相手が目の前のマス（カウンター越しではない）にいる？ */
	if ((pChar2->dwX != dwX) || (pChar2->dwY != dwY)) {
		return nChar;
	}
	/* そのマスにイベント（扉・掲示板など）があれば、誰が立っていても調べるを優先する。
	   扉の所に他のプレイヤーが立っていると、扉を開けたいのに話しかけてしまっていた */
	if (g_pMapInfo->pbyFlg[dwX + g_pMapInfo->nCX * dwY] & MAPFLG_EVENT) {
		return -1;
	}
	return nChar;
}

/* ========================================================================= */
/* 関数名：IsSKeyCounterTalk												 */
/* 内容：Sキーでカウンター越しに話しかける相手がいるか						 */
/*		 目の前がカウンターで、その向こうに話せる相手がいる時 TRUE			 */
/*		 足元にイベントがあっても、この時は話すを優先する					 */
/*		 （戦場ゲーム・すごろくの間は話さない）								 */
/* ========================================================================= */
BOOL IsSKeyCounterTalk(void)
{
	PCHARINFO pChar, pChar2;
	DWORD dwX, dwY;
	int nChar;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return FALSE;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	if ((pChar->abySubStatus[0] == SUBGAMEID_WAR) ||
		(pChar->abySubStatus[0] == SUBGAMEID_SUGOROKU)) {
		return FALSE;
	}
	nChar = GetSKeyTalkChar ();
	if (nChar < 0) {
		return FALSE;
	}
	pChar2 = g_aCharInfo[nChar];
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	/* 相手が目の前のマスにいる時はカウンター越しではない */
	if ((pChar2->dwX == dwX) && (pChar2->dwY == dwY)) {
		return FALSE;
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：CmdProcSKey														 */
/* 内容：Sキーの処理（調べる・話す・扉）									 */
/*		 足元にイベントがある時は今まで通り調べる。							 */
/*		 目の前に話しかけられるキャラがいれば「話す」。						 */
/*		 それ以外は「調べる」（目の前の扉はサーバーが開けてくれる）			 */
/*		 戻り値：決定音を鳴らす時 TRUE										 */
/* ========================================================================= */
BOOL CmdProcSKey(void)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	/* 足元にイベントがある時は調べる。
	   ただしカウンター越しに話せる相手がいる時は話す（爆弾亭のカウンター前の椅子のように、
	   乗っている間だけ机にグラスが出るイベントがあると、話しかけられなかった） */
	if (g_pMapInfo->pbyFlg[pChar->dwX + g_pMapInfo->nCX * pChar->dwY] & MAPFLG_EVENT) {
		if (IsSKeyCounterTalk ()) {
			CmdProcHanasu (NULL);
			return TRUE;
		}
		return CmdProcSiraberu (NULL);
	}

	/* 戦場ゲーム・すごろくの間は「話す」コマンドが無いので今まで通り調べるだけ */
	if ((pChar->abySubStatus[0] != SUBGAMEID_WAR) &&
		(pChar->abySubStatus[0] != SUBGAMEID_SUGOROKU)) {
		/* 目の前に誰かいれば話す（掲示板などは調べる） */
		if (GetSKeyTalkChar () >= 0) {
			CmdProcHanasu (NULL);
			return TRUE;
		}
	}

	/* 調べる（目の前の扉もここで開く） */
	return CmdProcSiraberu (NULL);
}

/* ========================================================================= */
/* 関数名：CmdProcSiraberu													 */
/* 内容：コマンド処理(調べる)												 */
/* 日付：2003/07/12															 */
/* ========================================================================= */
BOOL CmdProcSiraberu(PWNDINFO pWnd)
{
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];
	DWORD dwX, dwY;
	BYTE byFlg = 0;
	BOOL bResult;

	bResult = TRUE;
	if (g_aCharInfo[g_nMyCharID]->dwTargetTblID3 != -1) {
		PWNDINFO pWndTmp;
		pWndTmp = MakeWindowMSG ("今は調べることができません。@2", FALSE);
		g_WndInfo.Add (pWndTmp);
		g_dwActiveMsgID = pWndTmp->dwID;
		goto Exit;
	}

	if (pWnd) {
		pWnd->byAnime = 1;
		pWnd->dwTimeDrawStart = 0;
	}

	dwX = pChar->dwX;
	dwY = pChar->dwY;

	/* 地面に何かある？ */
	if (g_pMapInfo->pbyFlg[pChar->dwX + g_pMapInfo->nCX * pChar->dwY] & MAPFLG_ITEM) {

	}

	dwX = pChar->dwX;
	dwY = pChar->dwY;
	if (g_pMapInfo->pbyFlg[dwX + g_pMapInfo->nCX * dwY] & MAPFLG_EVENT) {
		byFlg = 1;
		/* サーバーにイベント問い合わせ */
		if (SendCmdEVENTCHECK (1) == FALSE) {
			byFlg = 2;
		}

	} else {
		dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
		dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
		if (pChar->dwX >= 0 && pChar->dwX < (DWORD)g_pMapInfo->nCX &&
			pChar->dwY >= 0 && pChar->dwY < (DWORD)g_pMapInfo->nCY) {
			if (g_pMapInfo->pbyFlg[dwX + g_pMapInfo->nCX * dwY] & MAPFLG_EVENT) {
				byFlg = 1;
				/* サーバーにイベント問い合わせ */
				if (SendCmdEVENTCHECK (2) == FALSE) {
					byFlg = 2;
				}
			}
		}
	}

	if (byFlg == 0) {
		PMAPPARTSINFO pInfo;
		int nPartsID;

		nPartsID = g_pMapInfo->pwMap[pChar->dwX + pChar->dwY * g_pMapInfo->nCX];
		pInfo = &g_pMapPartsInfo[nPartsID];
		/* 雪の上？ */
		if (pInfo->byHitType == PARTSHIT_FOOTMARK_SNOW) {
			DWORD dwTime = timeGetTime ();
			if (abs (g_dwTimeLastThrow - dwTime) > 2000) {
				g_dwTimeLastThrow = dwTime;
				if (g_bSnowCharge) {
					SendCMDTHROWSNOW ();
					g_bSnowCharge = FALSE;
				} else {
					g_bSnowCharge = TRUE;
					g_dwTimeLastThrow = 0;
				}
			}
			bResult = FALSE;

		} else {
			PWNDINFO pWndTmp;

			pWndTmp = MakeWindowMSG ("何も見つかりませんでした。@2", FALSE);
			g_WndInfo.Add (pWndTmp);
			g_dwActiveMsgID = pWndTmp->dwID;
		}
	}

Exit:
	return bResult;
}

/* ========================================================================= */
/* 関数名：ChatMsgProc														 */
/* 内容：発言メッセージ処理													 */
/* 日付：2004/05/08															 */
/* ========================================================================= */
void ChatMsgProc(LPCTSTR pszMsg)
{
	LPSTR pszMsgTmp = NULL;
	BOOL bProc = FALSE;
	BYTE byType = SPEECHCMD_AROUND;
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	if (strlen (pszMsg) == 0) {
		goto ChatMsgProc_END;
	}

	/* 管理人権限？ */
	if (pChar->byAdminLV == ADMINLV_ADMINISTRATOR) {
		BOOL bShift = (GetKeyState (VK_SHIFT) & 0x8000) ? TRUE : FALSE;
		BOOL bCtrl = (GetKeyState (VK_CONTROL) & 0x8000) ? TRUE : FALSE;
#ifdef __EMSCRIPTEN__
		/* ブラウザ版：チャット欄などから渡された Shift/Ctrl を使う */
		if (g_nWebSpeechKeyMod >= 0) {
			bShift = (g_nWebSpeechKeyMod == 1) ? TRUE : FALSE;
			bCtrl = (g_nWebSpeechKeyMod == 2) ? TRUE : FALSE;
		}
#endif
		/* 一斉放送？ */
		if (bShift) {
			byType = SPEECHCMD_ALL;

		/* 管理者用内線？ */
		} else if (bCtrl) {
			byType = SPEECHCMD_GM;
		}
	}

	/* 1 文字目が 1 バイト文字？ */
	if (SboTextCharLen (pszMsg) <= 1) {
		/* 一文字目のコマンドチェック */
		switch ((BYTE)pszMsg[0]) {
		case '#':		/* 吹き出しコマンド */
			{
				char szBuf3[5];
				BYTE byID = 0;

				if (strlen (pszMsg) > 3) {
					break;
				}

				wsprintf (szBuf3, "%s", &pszMsg[1]);
				byID = atoi (szBuf3);
				/* ちゃんとした値になってる？ */
				if (byID && (byID <= MAX_COMEFC)) {
					/* 吹き出しコマンド送信 */
					SendCmdCOMEFC (byID);
					bProc = TRUE;
					break;
				}
			}
			break;

		case '$':		/* エフェクト指定コマンド */
			{
				BYTE byID = 0;
				char szBuf[6], chType = 0;
				int i, nLen = strlen (pszMsg);
				if (nLen > 5) {
					break;
				}

				strcpy (szBuf, pszMsg);
				for (i = 1; i < nLen; i ++) {
					if (szBuf[i] == '-') {
						szBuf[i] = 0;
						/* 自分に使う？ */
						if (szBuf[i + 1] == '1') {
							chType = 1;
						/* 前のキャラに使う？ */
						} else if (szBuf[i + 1] == '2') {
							chType = 2;
						}
						break;
					}
				}
				byID = (BYTE)atoi (&szBuf[1]);
				/* ちゃんとした値になってる？ */
				if ((byID == 0) || (byID > g_EfcInfo.GetCount ())) {
					break;
				}

				/* エフェクトコマンド送信 */
				g_byUseEfcID = byID;
				if (chType) {
					SendCmdEFCREQ (chType - 1, g_byUseEfcID);
				}
				bProc = TRUE;
			}
			break;

		case '!':		/* マップ内放送 */
			/* マップ内放送が許可されてる？ */
			if (g_pMapInfo->byFlg & MAPFLG_SPEECH) {
				byType = SPEECHCMD_MAP;
				pszMsg ++;
			}
			break;

		case '%':		/* パーティ発言（画面内の人＋画面外の仲間に届く） */
			if (g_aPartyInfo.GetSize () > 0) {
				byType = SPEECHCMD_PARTY;
				pszMsg ++;
			} else {
				AddSysMsg ("パーティに入っていません。", RGB (255, 255, 255));
				bProc = TRUE;
			}
			break;

		case '<':		/* 画面内放送 */
			/* マップ内放送が許可されてる？ */
			if (g_pMapInfo->byFlg & MAPFLG_SPEECH) {
				byType = SPEECHCMD_SCREENCHAR;
				pszMsg ++;
			}
			break;

		case '/':		/* メッセージコマンド */
			{
				int i,
					nPara = 0,
					nLen = strlen (pszMsg);

				/* パラメータの位置 */
				for (i = 0; i < nLen; i ++) {
					if ((BYTE)pszMsg[i] == ' ') {
						nPara = i + 1;
						if (nPara >= nLen) {
							nPara = 0;
						}
						break;
					}
				}

				int nTmp = nLen;
				if (nPara) {
					nTmp = nPara - 2;
				}

				/* コマンド名の検索 */
				BYTE byCmdID = 0;
				LPSTR pszTmp = new char[nTmp + 1];
				ZeroMemory (pszTmp, nTmp + 1);
				strncpy (pszTmp, &pszMsg[1], nTmp);
				/* サイコロ */
				if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_DICE]) == 0) {
					byCmdID = MSGCMD_DICE;
					int nParaTmp = 1;

					if (nPara) {
						nParaTmp = atoi (&pszMsg[nPara]);
						nParaTmp = min (6, nParaTmp);
						nParaTmp = max (1, nParaTmp);
					}

					if (nParaTmp == 1) {
						SendCmdMSGCMD_0 (byCmdID);
					} else {
						SendCmdMSGCMD_1 (byCmdID, nParaTmp);
					}
					bProc = TRUE;
				/* ケノ */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_KENO]) == 0) {
					byCmdID = MSGCMD_KENO;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* コイン */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_COIN]) == 0) {
					byCmdID = MSGCMD_COIN;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* 戦場ゲーム */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_WDICE]) == 0) {
					byCmdID = MSGCMD_WDICE;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* サイコロ(1～6を2個) */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_2D6]) == 0) {
					byCmdID = MSGCMD_2D6;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* サイコロ(1～6を3個) */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_3D6]) == 0) {
					byCmdID = MSGCMD_3D6;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* サイコロ(1～10) */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_1D10]) == 0) {
					byCmdID = MSGCMD_1D10;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* サイコロ(1～20) */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_1D20]) == 0) {
					byCmdID = MSGCMD_1D20;
					SendCmdMSGCMD_0 (byCmdID);
					bProc = TRUE;
				/* 最も人が集まっている場所（返事はサーバーから自分にだけ届く） */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_WHERE]) == 0) {
					SendCmdMSGCMD_0 (MSGCMD_WHERE);
					bProc = TRUE;
				/* 現在時刻 */
				} else if (strcmp (pszTmp, aMsgCmdTbl[MSGCMD_CLOCK]) == 0) {
					SYSTEMTIME sysTime;

					GetLocalTime (&sysTime);
					pszMsgTmp = new char[40];
					wsprintf (pszMsgTmp, "%02d:%02d:%02d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
				}

				SAFE_DELETE_ARRAY (pszTmp);
			}
			break;
		}
	}

	switch (g_bySpeechMode) {
	case 1:
		/* マップ内放送が許可されてる？ */
		if (g_pMapInfo->byFlg & MAPFLG_SPEECH) {
			byType = SPEECHCMD_SCREENCHAR;
		}
		break;
	case 2:
		/* マップ内放送が許可されてる？ */
		if (g_pMapInfo->byFlg & MAPFLG_SPEECH) {
			byType = SPEECHCMD_MAP;
		}
		break;
	case 3:
		/* パーティ発言（パーティに入っている時だけ。ほかの記号で種類を選んだ時はそちら） */
		if ((byType == SPEECHCMD_AROUND) && (g_aPartyInfo.GetSize () > 0)) {
			byType = SPEECHCMD_PARTY;
		}
		break;
	}

	/* 記号だけで中身が無い発言は送らない */
	if (pszMsg[0] == 0) {
		bProc = TRUE;
	}

	/* 処理された？ */
	if (bProc) {
		goto ChatMsgProc_END;
	}

	/*************************************
		ここからは通常メッセージの処理
	*************************************/

	if (pszMsgTmp == NULL) {
		pszMsgTmp = new char[strlen (pszMsg) + 1];
		strcpy (pszMsgTmp, pszMsg);
	}

	/* 酔っ払い変換をする？ */
	if (pChar->byDrunkLV > CHARSTATEDRUNK_NONE) {
		char szHenkan1[] = {"たちつてとだぢづでどxさしすせそざじずぜぞ"};
		char szHenkan2[] = {"らりるれろらりるれろxはひふへほはひふへほ"};
		int i, nChar, nChar1, nChar2;
		LPCSTR psz1, psz2;

		for (i = 0; pszMsgTmp[i]; i += nChar) {
			nChar = SboTextCharLen (&pszMsgTmp[i]);
			/* 1 バイト文字は変換しない */
			if (nChar <= 1) {
				continue;
			}

			/* 変換用テーブルを 1 文字ずつ見る（2 つの表は同じ並び） */
			for (psz1 = szHenkan1, psz2 = szHenkan2; *psz1 && *psz2; psz1 += nChar1, psz2 += nChar2) {
				nChar1 = SboTextCharLen (psz1);
				nChar2 = SboTextCharLen (psz2);
				if (*psz1 == 'x') {
					/* 陶酔？ */
					if (pChar->byDrunkLV == CHARSTATEDRUNK_DRANKARD) {
						/* これ以上は変換しない */
						break;
					}
					continue;
				}
				/* 変換文字？（置き換えで長さが変わらない時だけ） */
				if ((nChar1 == nChar) && (nChar2 == nChar) &&
					(memcmp (&pszMsgTmp[i], psz1, nChar) == 0)) {
					CopyMemory (&pszMsgTmp[i], psz2, nChar);
					break;
				}
			}
		}
	}

	/* 使用文字チェック */
	{
		int i, nChar;

		for (i = 0; pszMsgTmp[i]; i += nChar) {
			nChar = SboTextCharLen (&pszMsgTmp[i]);
			/* 制御文字と、UTF-8 として正しくないバイトは「?」にする */
			if ((nChar == 1) && (((BYTE)pszMsgTmp[i] < ' ') || ((BYTE)pszMsgTmp[i] >= 0x80))) {
				pszMsgTmp[i] = '?';
			}
		}
	}

	/* 流れるメッセージ？（パーティ発言は普通の発言として吹き出しにも出すので名前を付けない） */
	if ((byType != SPEECHCMD_AROUND) && (byType != SPEECHCMD_PARTY)) {
		LPSTR pszTmp;
		pszTmp = new char[strlen (pChar->szName) + strlen (pszMsgTmp) + 1 + 1];
		wsprintf (pszTmp, "%s:%s", pChar->szName, pszMsgTmp);

		/* 名前を追加した文字列に置き換える */
		SAFE_DELETE_ARRAY (pszMsgTmp);
		pszMsgTmp = pszTmp;
	}

	SendCmdSPEECH (pszMsgTmp, byType);

ChatMsgProc_END:
	SAFE_DELETE_ARRAY (pszMsgTmp);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* ========================================================================= */
/* 関数名：SboWebGetPadState												 */
/* 内容：スマホの仮想パッドで今押して効くボタンを返す（ブラウザ版）			 */
/*		 シェル HTML が効かないボタンを隠すのに使う。KeyProc・KeyProcMove・	 */
/*		 KeyProcDead・KeyProcMSG が実際にキーを使う条件に合わせている		 */
/*		 戻り値：1=調べる(S) 2=表示(Tab) 4=戦闘(Ctrl) 8=Sが話すになる 16=戦闘モード中 32=Zで得点板に名前 の組み合わせ */
/* ========================================================================= */
#define PADSTATE_SIRABERU	(0x01)
#define PADSTATE_VIEW		(0x02)
#define PADSTATE_BATTLE		(0x04)
#define PADSTATE_HANASU		(0x08)	/* 目の前に話せる相手がいる（「調べる」ボタンを「話す」と表示） */
#define PADSTATE_BATTLEMODE	(0x10)	/* 戦闘モード中（「決定」を「攻撃」、「キャンセル」を「構え」と表示） */
#define PADSTATE_SCORE		(0x20)	/* 目の前に得点板がある（「キャンセル」を「名前」と表示。Z で名前を付ける） */

extern "C" EMSCRIPTEN_KEEPALIVE int SboWebGetPadState(void)
{
	PCHARINFO pChar;
	int nRet, nBattle;

	/* マップにいない（ログイン・キャラ選択など）時はどれも効かない */
	if ((g_byGameState < GAMESTATE_MOVE) || (g_byGameState == GAMESTATE_CONNECTLOST)) {
		return 0;
	}
	if ((g_pMapInfo == NULL) || (g_pMapInfo->pwMap == NULL)) {
		return 0;
	}
	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return 0;
	}
	/* 発言の入力欄を出している間はどれも効かない */
	if (g_bSpeech) {
		return 0;
	}

	/* ウィンドウ（コマンド・道具・お店など）を開いている間は、そのウィンドウにキーが渡る。
	   S はメッセージ（NPC のセリフなど）を読み進める時だけ使える */
	pChar = g_aCharInfo[g_nMyCharID];
	/* 戦闘モード中（戦闘コマンドを開いている間も、ボタンの名前と隠すボタンは変えない） */
	nBattle = 0;
	if ((g_pMapInfo->byFlg & MAPFLG_BATTLE) && g_bBattleMode && (pChar->abySubStatus[0] == 0) &&
		(pChar->byCharState != CHARSTATE_DEAD)) {
		nBattle = PADSTATE_BATTLEMODE;
	}
	if (g_dwActiveMsgID) {
		int nID = GetWindowID (g_dwActiveMsgID);
		if ((nID >= 0) && (nID < g_WndInfo.GetSize ()) && (g_WndInfo[nID]->nType == WNDTYPE_MSG)) {
			return PADSTATE_SIRABERU | nBattle;
		}
		return nBattle;
	}

	/* 死んでいる間は表示の切り替えだけ */
	if (pChar->byCharState == CHARSTATE_DEAD) {
		return PADSTATE_VIEW;
	}

	nRet = PADSTATE_VIEW;
	/* 爆弾ゲームでは、試合をしていないゲームマスターの設定にだけ使う */
	if (pChar->abySubStatus[0] == SUBGAMEID_BOMB) {
		PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
		if (pSubStatus->byFlgGameMaster && !pSubStatus->byFlgEntry) {
			nRet |= PADSTATE_SIRABERU;
		}
	} else {
		nRet |= PADSTATE_SIRABERU;
		/* Sキーで話す相手がいるか（CmdProcSKey と同じ条件） */
		if (((g_pMapInfo->pbyFlg == NULL) ||
			((g_pMapInfo->pbyFlg[pChar->dwX + g_pMapInfo->nCX * pChar->dwY] & MAPFLG_EVENT) == 0)) &&
			(pChar->abySubStatus[0] != SUBGAMEID_WAR) &&
			(pChar->abySubStatus[0] != SUBGAMEID_SUGOROKU) &&
			(GetSKeyTalkChar () >= 0)) {
			nRet |= PADSTATE_HANASU;
		/* 足元にイベントがあっても、カウンター越しに話せる相手がいれば話す */
		} else if (IsSKeyCounterTalk ()) {
			nRet |= PADSTATE_HANASU;
		}
		/* Z キーで得点板（雪合戦場の HP パネル）に名前を付けられるか（KeyProcMove と同じ条件） */
		if ((pChar->abySubStatus[0] == 0) && (pChar->byTurnLV == 0) && (pChar->dwTimeTurnLV == 0) &&
			(pChar->dwTimeTurnMagicLV == 0) && (GetFrontScoreChar () >= 0)) {
			nRet |= PADSTATE_SCORE;
		}
	}
	/* 戦闘モードは戦闘できるマップでだけ切り替えられる */
	if (g_pMapInfo->byFlg & MAPFLG_BATTLE) {
		nRet |= PADSTATE_BATTLE;
	}
	return nRet | nBattle;
}

/* ========================================================================= */
/* 関数名：	SboWebChatKinds													 */
/* 内容：	今選べる発言の種類（ブラウザ版）								 */
/*			シェル HTML がチャット欄の送り先切り替え（周り/PT/マップ）に使う	 */
/*			戻り値：1=パーティに入っている 2=マップ内放送ができるマップ	 */
/*			の組み合わせ（マップにいない時は 0）							 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebChatKinds(void)
{
	int nRet = 0;

	if ((g_byGameState < GAMESTATE_MOVE) || (g_byGameState == GAMESTATE_CONNECTLOST)) {
		return 0;
	}
	if (IsSpeechModeEnable (3)) {
		nRet |= 1;
	}
	if (IsSpeechModeEnable (2)) {
		nRet |= 2;
	}
	return nRet;
}
#endif

/* Copyright(C)URARA-works 2003-2005 */
