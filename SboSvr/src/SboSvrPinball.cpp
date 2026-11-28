/* ========================================================================= */
/* ファイル名：	SboSvrPinball.cpp											 */
/* 概要：		ピンボールの台（台NPC・席・ゲームの進行）の処理(実装ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 仕様は docs/pinball.md。物理・進行は common/Pinball.h（サーバーとクライアントで同じコード）。
 * 作りは落ち物パズル（SboSvrPuzzle.cpp）をなぞっている（台NPC・席・コイン・待機中の台・近くの人への送信）。
 * 台は、台詞が "@i<サブゲーム情報の番号>" の NPC。台の設定は、その番号のサブゲーム情報（SUBGAME_PINBALLINFO。管理者ウィンドウ・データ API）にある:
 *   コイン（アイテム番号・枚数。どちらか 0 なら無料）・球数・ボールセーブの秒数・得点（バンパー・ターゲット・全部倒したボーナス）・
 *   コンボの時間・盤の位置（dx, dy）・帯の側・座る場所（sdx, sdy）。範囲外の値は範囲に丸めて使う（PinballInfoClamp）。
 *   番号が無い・情報が無い・種別がピンボールでない時は、台ではない（何もしない）。台NPCは見えない（byGhost を CHARGHOST_NAMEONLY にして送る）。
 * 設定は、ゲームを始める時に取り込む（遊んでいる間に変えても、そのゲームは変わらない。待機中の台は変えたらすぐ盤の位置などに反映）。
 * クライアントの先読み・帯の表示・座る場所の判定に要る設定は、CMDPARA_PINBALLSTATE の末尾で一緒に送る。
 * ゲームの正はサーバー。周期処理（PinballProc）で g_dwTime の経過時間から固定ステップ（1/120 秒）を回す。
 * キーは押した時と離した時だけ届く（PINBALLKEY_*）。届いた時、まず経過時間ぶんゲームを進めてから反映する（入力の時刻が合う）。
 * 状態は PINBALLSTATE_FULL を、約 PINBALL_SEND_MS ごとと、得点・球・状態が変わった時に、盤を見ている全員
 * （座っている人・台の近くにいる人）へ送る。本人のクライアントは common/Pinball.h で先読みし、
 * 届いた状態（ステップ数 dwStep・ボール・フリッパー）から巻き直して少しずつ寄せる。観戦者は補間して描く。
 * 対戦（ペアの 2 台。落ち物パズルの対戦をなぞる）：
 *   サブゲーム情報のペア番号（byPair）が同じ・同じマップの台 2 台が 1 組。ひとりが遊んでいる所へ、ペアの台に座ろうとすると
 *   「乱入して対戦しますか？」と聞く（CMD_TALKANSWER。答えは PinballAnswer）。ペアの台に座る時（乱入するのでなければ）は
 *   「乱入されてもよいですか？」と聞く（NG なら乱入できず、その人もひとりで遊ぶ）。
 *   乱入が成り立つと両方の盤が最初に戻り、3・2・1 のカウントダウン（操作できない）のあと同時に始まる。同じ乱数の種（dwSeed）で
 *   打ち出しの乱数が両者同じになる。時間制（nVsSec 秒。既定 90）で、球は落ちても次が出る・ボールセーブは最初の球だけ。
 *   時間切れで得点が高い方の勝ち（同点は引き分け＝連勝は続き、両者とも席に残る）。途中で席を立つ・切断・離れたら残った側の勝ち。
 *   勝った側は席に残ってひとり用に戻り（次の乱入を受け付ける）、負けた側は席を立つ。連勝は台（席）ごと。
 *   両方の盤は、対戦相手と近くの人の全員に届く（相手の盤は後追い補間で描く）。
 *   ゲームの結果（得点・勝敗・連勝）は FinishResult の 1 か所に集めてある（記録＝ランキングは次の段階。そこに足す）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrPinball.h"
#include "../../common/Pinball.h"
#include "../../common/PinballInfo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <map>

#define PINBALL_NOCHAR			((DWORD)-1)
#define PINBALL_NEAR			(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define PINBALL_WATCH_MS		(250)		/* 台の近くにいる人（観戦）を見直す間隔(ms) */
#define PINBALL_SEND_MS			(100)		/* 状態を送る間隔(ms) */
#define PINBALL_KEY_BUCKET		(8)			/* キー入力に伴う即時の送信のバケツ（連打で観戦者へ送りすぎない） */
#define PINBALL_KEY_FILL_MS		(25)
#define PINBALL_OVER_SHOW_MS	(3000)		/* ゲームオーバーの字を出している時間(ms) */
#define PINBALL_ASK_TIMEOUT_MS	(30000)		/* 「乱入しますか？」の問いが有効な時間(ms) */
/* 盤面を送る距離（落ち物パズルと同じ。盤の中心から。盤は 5×7 マス） */
#define PINBALL_VIEW_MARGIN		(12)
#define PINBALL_VIEW_ENTER		(PINBALL_VIEW_MARGIN + 3)
#define PINBALL_VIEW_LEAVE		(PINBALL_VIEW_MARGIN + 6)
#define PINBALL_TILES_W			(PB_W / PBP_TILE_DOT)		/* 盤の横のマス数（切り捨て） */
#define PINBALL_TILES_H			(PB_VIEW_H / PBP_TILE_DOT)

/* 台の設定（台NPCの台詞が指すサブゲーム情報から読む。範囲に丸めた値） */
typedef struct _PINBALLPARAM {
	int		nInfo;							/* サブゲーム情報の番号（台詞の @i の後の数） */
	WORD	wCoinItem;						/* コインのアイテム番号（0=無料） */
	int		nCoinCount;						/* 必要な枚数（0=無料） */
	int		nDx;							/* 盤の左上：台のマスの左上から（マス） */
	int		nDy;
	int		nSide;							/* 帯の側（PUZZLESIDE_*） */
	int		nSdx;							/* 座る場所：台のマスから（マス） */
	int		nSdy;
	PinballConfig	Cfg;					/* ゲームの設定（球数・ボールセーブ・コンボ・得点） */
	int		nPair;							/* ペア番号（0=ペアなし。同じマップ・同じ番号の台 2 台が対戦できる） */
	int		nVsSec;							/* 対戦の時間（秒） */
} PINBALLPARAM;

/* 盤面を送ってある人 */
typedef struct _PINBALLWATCH {
	DWORD		dwChar;						/* キャラのテーブルID */
	DWORD		dwPID;						/* プレイヤーID */
	BYTE		byFlag;						/* 最後に送った待機中の案内（PINBALLIDLE_SEAT・FREE の bit） */
} PINBALLWATCH;

/* 台（座っている人がいる間と、待機中の台の近くに人がいる間・ゲームオーバーの字を出している間、表に載せる） */
typedef struct _PINBALLTABLE {
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	DWORD		dwMapID;					/* 台NPCのいるマップ */
	PINBALLPARAM Param;
	DWORD		dwChar;						/* 座っているキャラのテーブルID（いなければ PINBALL_NOCHAR） */
	DWORD		dwPID;						/* そのキャラのプレイヤーID */
	DWORD		dwAck;						/* 最後に受けたキーの連番 */
	DWORD		dwLastKey;					/* キー用のバケツを最後に計算した時刻 */
	int			nKeyTokens;
	PinballGame	Game;						/* ゲーム */
	DWORD		dwLastTick;					/* ゲームを最後に進めた時刻 */
	DWORD		dwLastSend;					/* 状態を最後に送った時刻 */
	BOOL		bCharging;					/* ↓を押している（打ち出しを溜めている） */
	DWORD		dwChargeStart;				/* 溜め始めた時刻 */
	DWORD		dwLastWatch;				/* 最後に観戦の人を見直した時刻 */
	BOOL		bIdle;						/* 待機中（席に誰もいない） */
	BOOL		bOverShow;					/* 「GAME OVER」を出している間（待機中の台） */
	DWORD		dwOverTime;					/* ゲームオーバーになった時刻 */
	DWORD		dwOverScore;				/* ゲームオーバーの得点（字と一緒に出す） */
	std::vector<PINBALLWATCH>	aWatch;		/* 盤面を送ってある人 */
	/* 対戦 */
	BOOL		bVs;						/* 対戦中（カウントダウン中を含む） */
	BOOL		bAllowVs;					/* 乱入を受け付けるか（座る前に選ぶ。FALSE ならペアの台に座った人は乱入できず、ひとりで遊ぶ） */
	DWORD		dwOppNpc;					/* 対戦相手の台NPCのテーブルID（bVs の時） */
	BOOL		bCount;						/* カウントダウン中（操作できない・ゲームを進めない） */
	DWORD		dwStartTime;				/* 対戦が始まる時刻（カウントダウンの終わり） */
	DWORD		dwEndTime;					/* 対戦が終わる時刻（時間切れ） */
	int			nCountSent;					/* 最後に知らせたカウントダウンの数 */
	int			nWinStreak;					/* この席の連勝数（勝つと +1。席を立つ・切断・負け・ひとり用の結果で 0） */
} PINBALLTABLE;

/* ゲームの結果（記録を足す時は FinishResult にだけ足せばよい） */
enum {
	PBRESULT_SOLO = 0,						/* ひとり用のゲームオーバー */
	PBRESULT_WIN,							/* 対戦に勝った（時間切れ・相手が席を立った・切断） */
	PBRESULT_LOSE,							/* 対戦に負けた（時間切れ・自分が席を立った・切断） */
	PBRESULT_DRAW,							/* 引き分け（同点の時間切れ） */
};
typedef struct _PINBALLRESULT {
	int			nKind;						/* PBRESULT_* */
	char		szName[32];					/* 席の人の名前 */
	DWORD		dwScore;					/* 得点 */
	DWORD		dwOppScore;					/* 相手の得点（対戦のみ） */
	int			nStreak;					/* 結果が出た後の連勝数（負け・ひとり用は 0） */
	DWORD		dwNpc;						/* 台NPCのテーブルID */
	int			nInfo;						/* 台のサブゲーム情報の番号 */
} PINBALLRESULT;

/* 「乱入しますか？」の問い（座ろうとした人ごとに 1 つ。答えは PinballAnswer。時間がたったら捨てる） */
typedef struct _PINBALLASK {
	DWORD	dwNpc;							/* 座ろうとした台NPCのテーブルID */
	DWORD	dwTime;							/* 問いを出した時刻 */
	BOOL	bCoinUsed;						/* 道具のコインから座ろうとしたか */
	BOOL	bAllowAsk;						/* TRUE=「乱入されてもよいか」の問い、FALSE=「乱入するか」の問い */
} PINBALLASK;
static std::map<DWORD, PINBALLASK> s_mapAsk;

static std::vector<PINBALLTABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 台NPC（台詞 "@i<番号>"）の設定をサブゲーム情報から読む。台でない（番号が無い・情報が無い・種別が違う）なら FALSE */
static BOOL LoadParam(PCHARINFO pNpc, PINBALLPARAM *pParam)
{
	SUBGAME_PINBALLINFO Info;
	PSUBGAMEINFO pSub;
	int nNo;

	if ((pNpc == NULL) || (pNpc->byNpcType == NPCTYPE_INIT) || (pNpc->pszNpcSpeech == NULL)) {
		return FALSE;
	}
	nNo = PinballSpeechInfoNo (pNpc->pszNpcSpeech);
	if ((nNo < 0) || (nNo >= g_aSubaGameInfo.GetSize ())) {
		return FALSE;
	}
	pSub = g_aSubaGameInfo[nNo];
	if ((pSub == NULL) || (pSub->byType != SUBGAMEID_PINBALL) || (pSub->pInfo == NULL)) {
		return FALSE;
	}
	if (pParam) {
		memcpy (&Info, pSub->pInfo, sizeof (Info));
		PinballInfoClamp (&Info);
		memset (pParam, 0, sizeof (*pParam));
		pParam->nInfo		= nNo;
		pParam->wCoinItem	= Info.wCoinItem;
		pParam->nCoinCount	= Info.byCoinCount;
		pParam->nDx			= Info.cDx;
		pParam->nDy			= Info.cDy;
		pParam->nSide		= (Info.byBandSide == 1) ? PUZZLESIDE_LEFT : PUZZLESIDE_RIGHT;
		pParam->nSdx		= Info.cSeatDx;
		pParam->nSdy		= Info.cSeatDy;
		PinballInfoToConfig (&Info, &pParam->Cfg);
		pParam->nPair		= Info.byPair;
		pParam->nVsSec		= Info.wVsSec;
	}
	return TRUE;
}

/* 設定が同じか（待機中の台の、設定の変更の検出） */
static BOOL ParamSame(const PINBALLPARAM &a, const PINBALLPARAM &b)
{
	return (a.nInfo == b.nInfo) && (a.wCoinItem == b.wCoinItem) && (a.nCoinCount == b.nCoinCount) &&
		(a.nDx == b.nDx) && (a.nDy == b.nDy) && (a.nSide == b.nSide) && (a.nSdx == b.nSdx) && (a.nSdy == b.nSdy) &&
		(a.Cfg.nBalls == b.Cfg.nBalls) && (a.Cfg.nSaveMs == b.Cfg.nSaveMs) && (a.Cfg.nComboMs == b.Cfg.nComboMs) &&
		(a.Cfg.nScoreBumper == b.Cfg.nScoreBumper) && (a.Cfg.nScoreTarget == b.Cfg.nScoreTarget) && (a.Cfg.nScoreBonus == b.Cfg.nScoreBonus) &&
		(a.nPair == b.nPair) && (a.nVsSec == b.nVsSec);
}

/* 対戦の項目を初期にする（台を作る時） */
static void VsClear(PINBALLTABLE &Table)
{
	Table.bVs = FALSE;
	Table.bAllowVs = TRUE;
	Table.dwOppNpc = PINBALL_NOCHAR;
	Table.bCount = FALSE;
	Table.dwStartTime = 0;
	Table.dwEndTime = 0;
	Table.nCountSent = 0;
	Table.nWinStreak = 0;
}

/* ゲームの結果（得点・勝敗・連勝）。ゲームが終わる時に必ずここを通る。記録（ランキング）を足す時はここに足す */
static void FinishResult(const PINBALLRESULT &R)
{
	(void)R;
}

/* 結果を作って FinishResult へ渡す */
static void ReportResult(const PINBALLTABLE &Table, PCHARINFO pChar, int nKind, DWORD dwOppScore, int nStreak)
{
	PINBALLRESULT R;

	memset (&R, 0, sizeof (R));
	R.nKind = nKind;
	if (pChar) {
		strncpy (R.szName, pChar->szName, sizeof (R.szName) - 1);
	}
	R.dwScore = Table.Game.dwScore;
	R.dwOppScore = dwOppScore;
	R.nStreak = nStreak;
	R.dwNpc = Table.dwNpc;
	R.nInfo = Table.Param.nInfo;
	FinishResult (R);
}

static PCHARINFO SeatChar(const PINBALLTABLE &Table)
{
	PCHARINFO pChar;

	if ((Table.dwChar == PINBALL_NOCHAR) || (Table.dwChar >= (DWORD)g_aCharInfo.GetSize ())) {
		return NULL;
	}
	pChar = g_aCharInfo[Table.dwChar];
	if ((pChar == NULL) || (pChar->pID != Table.dwPID)) {
		return NULL;
	}
	return pChar;
}

static int FindTable(DWORD dwNpc, DWORD dwMapID)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwNpc == dwNpc) && (s_aTable[i].dwMapID == dwMapID)) {
			return i;
		}
	}
	return -1;
}

static BOOL FindSeat(PCHARINFO pChar, int *pnTable)
{
	int i;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if ((s_aTable[i].dwChar == pChar->dwTblID) && (s_aTable[i].dwPID == pChar->pID)) {
			*pnTable = i;
			return TRUE;
		}
	}
	return FALSE;
}

static PCHARINFO WatchChar(const PINBALLWATCH &w)
{
	PCHARINFO pChar;

	if (w.dwChar >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pChar = g_aCharInfo[w.dwChar];
	if ((pChar == NULL) || (pChar->pID != w.dwPID) || (pChar->byGameState < GAMESTATE_MOVE)) {
		return NULL;
	}
	return pChar;
}

static PCHARINFO NpcOf(const PINBALLTABLE &Table)
{
	return (Table.dwNpc < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[Table.dwNpc] : NULL;
}

/* カウントダウンの残り（3・2・1。始まる時刻を過ぎたら 0） */
static int CountRemain(const PINBALLTABLE &Table)
{
	int nMs = (int)(Table.dwStartTime - g_dwTime);

	if (nMs <= 0) {
		return 0;
	}
	return (nMs + 999) / 1000;
}

/* 台の状態のパケットを作る。byKind: FULL=状態 OVER=終わり（本人向け） NONE=消す。byMine は送る相手ごとに書き換える */
static void BuildState(PINBALLTABLE &Table, BYTE byKind, std::vector<BYTE> &aBuf, BOOL bFlash)
{
	const PinballGame &g = Table.Game;
	CMDPARA_PINBALLSTATE Head;
	PCHARINFO pNpc = NpcOf (Table);
	BOOL bIdle = Table.bIdle && (byKind != PINBALLSTATE_OVER);
	int nX, nY;

	ZeroMemory (&Head, sizeof (Head));
	Head.byCmd		= CMD_PINBALLSTATE;
	Head.byKind		= byKind;
	Head.bySide		= (BYTE)Table.Param.nSide;
	Head.byIdle		= (BYTE)(bIdle ? PINBALLIDLE_ON : 0);
	Head.byOver		= (BYTE)((bIdle && Table.bOverShow) ? 1 : 0);
	Head.byPhase	= (BYTE)g.nPhase;
	Head.byBalls	= (BYTE)g.nBallsLeft;
	Head.byTargets	= (BYTE)g.nTargetMask;
	Head.wFlash	= (WORD)(bFlash ? g.nFlash : 0);
	Head.byCombo	= (BYTE)((g.nCombo > 255) ? 255 : g.nCombo);
	Head.byAngL		= (BYTE)g.nAngL;
	Head.byAngR		= (BYTE)g.nAngR;
	Head.byHeld		= (BYTE)((g.bHoldL ? 1 : 0) | (g.bHoldR ? 2 : 0) | (Table.bCharging ? 4 : 0));
	Head.byCharge	= (BYTE)(Table.bCharging ? PinballGame::ChargePercent ((int)(g_dwTime - Table.dwChargeStart)) : 0);
	Head.bySave		= (BYTE)((g.nSaveSteps + PB_STEP_HZ - 1) / PB_STEP_HZ);
	Head.nBallX		= g.nX;
	Head.nBallY		= g.nY;
	Head.sVx		= (short)g.nVx;
	Head.sVy		= (short)g.nVy;
	Head.dwScore	= g.dwScore;
	Head.dwNpc		= Table.dwNpc;
	Head.dwStep		= g.dwStep;
	Head.dwAck		= Table.dwAck;
	/* 台の設定（クライアントの先読み・帯・座る場所の判定用。待機中も同じ） */
	Head.byBallsMax		= (BYTE)Table.Param.Cfg.nBalls;
	Head.bySaveSecMax	= (BYTE)(Table.Param.Cfg.nSaveMs / 1000);
	Head.cSeatDx		= (signed char)Table.Param.nSdx;
	Head.cSeatDy		= (signed char)Table.Param.nSdy;
	Head.wComboMs		= (WORD)Table.Param.Cfg.nComboMs;
	Head.wScoreBumper	= (WORD)Table.Param.Cfg.nScoreBumper;
	Head.wScoreTarget	= (WORD)Table.Param.Cfg.nScoreTarget;
	Head.wScoreBonus	= (WORD)Table.Param.Cfg.nScoreBonus;
	/* 対戦（待機中・終わりの本人向けは対戦なし。連勝は席の人の分） */
	if (!bIdle) {
		Head.byStreak	= (BYTE)((Table.nWinStreak > 255) ? 255 : Table.nWinStreak);
		Head.byBallNo	= (BYTE)((g.nBallNo > 255) ? 255 : g.nBallNo);
		Head.byLaunchNo	= (BYTE)(g.nLaunchNo & 0xFF);
		Head.dwSeed		= g.nSeed;
		if (Table.bVs) {
			int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);
			int nMs = Table.bCount ? (int)(Table.dwEndTime - Table.dwStartTime) : PinballVsRemainMs (g_dwTime, Table.dwEndTime);

			Head.byVs = (BYTE)(Table.bCount ? 2 : 1);
			Head.byCount = (BYTE)(Table.bCount ? CountRemain (Table) : 0);
			Head.wVsRemain = (WORD)(((nMs + 99) / 100 > 65535) ? 65535 : (nMs + 99) / 100);
			Head.dwOppScore = (nOpp >= 0) ? s_aTable[nOpp].Game.dwScore : 0;
		}
	}
	if (bIdle) {
		/* 待機中は盤の枠だけ（ボール・帯なし）。ゲームオーバーの字の間だけ最後の得点を持つ */
		Head.byPhase = PB_PHASE_OVER;
		Head.byBalls = 0;
		Head.byTargets = 0;
		Head.wFlash = 0;
		Head.byCombo = 0;
		Head.byHeld = 0;
		Head.byCharge = 0;
		Head.bySave = 0;
		Head.dwScore = Table.bOverShow ? Table.dwOverScore : 0;
	}
	if (pNpc) {
		PinballFieldPos ((int)pNpc->dwX, (int)pNpc->dwY, Table.Param.nDx, Table.Param.nDy, &nX, &nY);
		Head.sFieldX = (short)nX;
		Head.sFieldY = (short)nY;
	}
	aBuf.resize (sizeof (Head));
	memcpy (&aBuf[0], &Head, sizeof (Head));
}

/* 待機中の台の、その人だけへの案内（座る場所に立っている→PINBALLIDLE_SEAT。必要な枚数が 0 なら FREE も） */
static BYTE SeatFlag(const PINBALLTABLE &Table, PCHARINFO pChar)
{
	PCHARINFO pNpc = NpcOf (Table);

	if (!Table.bIdle || (pNpc == NULL) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) ||
		(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0) ||
		PuzzleIsSeated (pChar) || PinballIsSeated (pChar) || MazeIsSeated (pChar) || JanIsSeated (pChar) ||
		(pChar->dwMapID != Table.dwMapID) || !IsSameSpace (pNpc, pChar) ||
		((int)pChar->dwX != (int)pNpc->dwX + Table.Param.nSdx) || ((int)pChar->dwY != (int)pNpc->dwY + Table.Param.nSdy)) {
		return 0;
	}
	return (BYTE)(PINBALLIDLE_SEAT | (((Table.Param.wCoinItem == 0) || (Table.Param.nCoinCount <= 0)) ? PINBALLIDLE_FREE : 0));
}

static BYTE SendStateTo(const PINBALLTABLE &Table, PCHARINFO pChar, std::vector<BYTE> &aBuf)
{
	PCMDPARA_PINBALLSTATE pHead = (PCMDPARA_PINBALLSTATE)&aBuf[0];
	BYTE byFlag = 0;

	pHead->byMine = (SeatChar (Table) == pChar) ? 1 : 0;
	if ((pHead->byKind == PINBALLSTATE_FULL) && (pHead->byIdle & PINBALLIDLE_ON)) {
		byFlag = SeatFlag (Table, pChar);
	}
	pHead->byIdle = (BYTE)((pHead->byIdle & PINBALLIDLE_ON) | byFlag);
	SendCmdPINBALLSTATE (pChar->pID, &aBuf[0], (DWORD)aBuf.size ());
	return byFlag;
}

/* 台の状態を、盤面を送ってある全員に送る。pSkip には送らない。当たった印（nFlash）は送ったら 0 に戻す */
static void SendTable(int nTable, BYTE byKind, PCHARINFO pSkip = NULL)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	std::vector<BYTE> aBuf;
	int i;

	BuildState (Table, byKind, aBuf, TRUE);
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar = WatchChar (Table.aWatch[i]);
		if ((pChar == NULL) || (pChar == pSkip)) {
			continue;
		}
		Table.aWatch[i].byFlag = SendStateTo (Table, pChar, aBuf);
	}
	Table.Game.nFlash = 0;
	Table.dwLastSend = g_dwTime;
}

static BOOL WatchEligible(const PINBALLPARAM &Param, DWORD dwMapID, PCHARINFO pNpc, PCHARINFO pChar, int nMargin)
{
	int nFx, nFy, nCx, nCy;

	if ((pChar->byNpcType != NPCTYPE_INIT) || (pChar->byGameState < GAMESTATE_MOVE) || (pChar->dwMapID != dwMapID) ||
		!IsSameSpace (pNpc, pChar) || !IsCharSeenBy (pNpc, pChar)) {
		return FALSE;
	}
	/* 座る場所のマスに立っている人は、盤から遠くても届ける（案内を出すため） */
	if (((int)pChar->dwX == (int)pNpc->dwX + Param.nSdx) && ((int)pChar->dwY == (int)pNpc->dwY + Param.nSdy)) {
		return TRUE;
	}
	PinballFieldPos ((int)pNpc->dwX, (int)pNpc->dwY, Param.nDx, Param.nDy, &nFx, &nFy);
	nCx = (nFx + PB_W / 2) / PBP_TILE_DOT;
	nCy = (nFy + PB_VIEW_H / 2) / PBP_TILE_DOT;
	return (abs ((int)pChar->dwX - nCx) <= nMargin + PINBALL_TILES_W / 2) &&
		(abs ((int)pChar->dwY - nCy) <= nMargin + PINBALL_TILES_H / 2);
}

static int FindWatch(const std::vector<PINBALLWATCH> &aWatch, const PINBALLWATCH &w)
{
	int i;

	for (i = 0; i < (int)aWatch.size (); i ++) {
		if ((aWatch[i].dwChar == w.dwChar) && (aWatch[i].dwPID == w.dwPID)) {
			return i;
		}
	}
	return -1;
}

/* 盤面が見える人を見直す。新しく見える人には状態を 1 回送り、見えなくなった人には NONE を送って消してもらう */
static void WatchRefresh(int nTable)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	PCHARINFO pNpc = NpcOf (Table);
	std::vector<PINBALLWATCH> aNew;
	std::vector<BYTE> aFull, aNone;
	PCHARINFO pSeat, pOpp = NULL;
	PMAPINFO pMap;
	int i;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	/* 座っている人と、対戦相手の席の人は距離に関係なく入れる（相手の盤も見えるように） */
	pSeat = SeatChar (Table);
	if (Table.bVs) {
		int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);
		if (nOpp >= 0) {
			pOpp = SeatChar (s_aTable[nOpp]);
		}
	}
	if (pSeat) {
		PINBALLWATCH w;
		w.dwChar = pSeat->dwTblID;
		w.dwPID = pSeat->pID;
		w.byFlag = 0;
		aNew.push_back (w);
	}
	if (pOpp && (pOpp != pSeat)) {
		PINBALLWATCH w;
		w.dwChar = pOpp->dwTblID;
		w.dwPID = pOpp->pID;
		w.byFlag = 0;
		aNew.push_back (w);
	}
	pMap = (Table.dwMapID < (DWORD)g_aMapInfo.GetSize ()) ? g_aMapInfo[Table.dwMapID] : NULL;
	if (pMap && pMap->paTblNoPC) {
		for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
			PCHARINFO pChar;
			PINBALLWATCH w;

			if (pMap->paTblNoPC[i] == (DWORD)-1) {
				break;
			}
			pChar = g_aCharInfo[pMap->paTblNoPC[i]];
			if ((pChar == pSeat) || (pChar == pOpp)) {
				continue;
			}
			w.dwChar = pChar->dwTblID;
			w.dwPID = pChar->pID;
			w.byFlag = 0;
			/* すでに送ってある人は遠い距離（LEAVE）まで、新しい人は近い距離（ENTER）から（境目で送る・消すを繰り返さない） */
			if (!WatchEligible (Table.Param, Table.dwMapID, pNpc, pChar, (FindWatch (Table.aWatch, w) >= 0) ? PINBALL_VIEW_LEAVE : PINBALL_VIEW_ENTER)) {
				continue;
			}
			aNew.push_back (w);
		}
	}
	/* 見えなくなった人へ NONE */
	for (i = 0; i < (int)Table.aWatch.size (); i ++) {
		PCHARINFO pChar;
		if (FindWatch (aNew, Table.aWatch[i]) >= 0) {
			continue;
		}
		pChar = WatchChar (Table.aWatch[i]);
		if (pChar) {
			if (aNone.empty ()) {
				BuildState (Table, PINBALLSTATE_NONE, aNone, FALSE);
			}
			SendStateTo (Table, pChar, aNone);
		}
	}
	/* 新しく見える人へ状態。すでに見ている人は、座る場所に立った・離れた（待機中の案内が変わった）時だけ送り直す */
	for (i = 0; i < (int)aNew.size (); i ++) {
		PCHARINFO pChar;
		int nOld = FindWatch (Table.aWatch, aNew[i]);

		pChar = WatchChar (aNew[i]);
		if (nOld >= 0) {
			aNew[i].byFlag = Table.aWatch[nOld].byFlag;
		}
		if (pChar == NULL) {
			continue;
		}
		if ((nOld >= 0) && (!Table.bIdle || (SeatFlag (Table, pChar) == aNew[i].byFlag))) {
			continue;
		}
		if (aFull.empty ()) {
			BuildState (Table, PINBALLSTATE_FULL, aFull, FALSE);
		}
		aNew[i].byFlag = SendStateTo (Table, pChar, aFull);
	}
	Table.aWatch = aNew;
}

/* 席が空になった台を待機中にする（表からは消さない。近くの人への盤は待機状態で送り続ける）。bOver なら「GAME OVER」の字を出す */
static void ToIdle(int nTable, BOOL bOver)
{
	PINBALLTABLE &Table = s_aTable[nTable];

	Table.dwOverScore = Table.Game.dwScore;
	Table.dwChar = PINBALL_NOCHAR;
	Table.dwPID = 0;
	Table.bIdle = TRUE;
	Table.bOverShow = bOver;
	Table.dwOverTime = g_dwTime;
	Table.bCharging = FALSE;
	VsClear (Table);		/* 席を立つ・負け・ひとり用の結果で連勝は 0 に戻る */
	Table.Game.Start (&Table.Param.Cfg);
}

/* 対戦を終える（席に残る側。ひとり用に戻って続ける。次の乱入を受け付ける）。ゲームは最初からやり直す */
static void EndVs(int nTable, PCHARINFO pSkip)
{
	PINBALLTABLE &Table = s_aTable[nTable];

	Table.bVs = FALSE;
	Table.bAllowVs = TRUE;
	Table.dwOppNpc = PINBALL_NOCHAR;
	Table.bCount = FALSE;
	Table.nCountSent = 0;
	Table.bCharging = FALSE;
	Table.Game.Start (&Table.Param.Cfg);
	Table.dwLastTick = g_dwTime;
	SendTable (nTable, PINBALLSTATE_FULL, pSkip);
}

/* 対戦の勝ち・引き分けの知らせ（連勝は 2 以上で付く）。席の人（取れる人）だけに送る */
static void TellVsResult(PCHARINFO pWin, int nStreak, PCHARINFO pTell1, PCHARINFO pTell2)
{
	char szMsg[128];

	if (pWin == NULL) {
		strcpy (szMsg, "引き分け！");
	} else if (nStreak >= PB_STREAK_MIN) {
		sprintf (szMsg, "%sの勝ち！ %sの%d連勝！", pWin->szName, pWin->szName, nStreak);
	} else {
		sprintf (szMsg, "%sの勝ち！", pWin->szName);
	}
	if (pTell1) {
		SendCmdSYSMSG (pTell1->pID, szMsg);
	}
	if (pTell2 && (pTell2 != pTell1)) {
		SendCmdSYSMSG (pTell2->pID, szMsg);
	}
}

/* 対戦の決着（nA・nB は対戦中の 2 台の表の添字。nJudge は A から見た PB_VS_*）。勝った側（と引き分けの両者）は席に残って
   ひとり用に戻る。負けた側の席は、呼んだ側が立たせる（時間切れ→Leave。席を立つ・切断→その人の Leave の続き）。
   bNotifyA・bNotifyB: その席の人へ知らせを送るか（切断した人へは送らない） */
static void VsResolve(int nA, int nB, int nJudge, BOOL bNotifyA, BOOL bNotifyB)
{
	PINBALLTABLE &A = s_aTable[nA];
	PINBALLTABLE &B = s_aTable[nB];
	PCHARINFO pA = SeatChar (A), pB = SeatChar (B);
	DWORD dwSa = A.Game.dwScore, dwSb = B.Game.dwScore;

	A.bVs = B.bVs = FALSE;		/* 離席の処理で、もう一度「残った側の勝ち」にならないように */
	if (nJudge == PB_VS_DRAW) {
		/* 引き分け：連勝は途切れない。両者とも席に残る */
		ReportResult (A, pA, PBRESULT_DRAW, dwSb, A.nWinStreak);
		ReportResult (B, pB, PBRESULT_DRAW, dwSa, B.nWinStreak);
		TellVsResult (NULL, 0, bNotifyA ? pA : NULL, bNotifyB ? pB : NULL);
		EndVs (nA, bNotifyA ? NULL : pA);
		EndVs (nB, bNotifyB ? NULL : pB);
	} else {
		BOOL bAWin = (nJudge == PB_VS_WIN_A);
		PINBALLTABLE &W = bAWin ? A : B;
		PINBALLTABLE &L = bAWin ? B : A;
		PCHARINFO pW = bAWin ? pA : pB, pL = bAWin ? pB : pA;
		BOOL bNotifyW = bAWin ? bNotifyA : bNotifyB, bNotifyL = bAWin ? bNotifyB : bNotifyA;

		W.nWinStreak = PinballStreakNext (W.nWinStreak, 1);
		ReportResult (W, pW, PBRESULT_WIN, bAWin ? dwSb : dwSa, W.nWinStreak);
		ReportResult (L, pL, PBRESULT_LOSE, bAWin ? dwSa : dwSb, 0);
		TellVsResult (pW, W.nWinStreak, bNotifyW ? pW : NULL, bNotifyL ? pL : NULL);
		EndVs (bAWin ? nA : nB, bNotifyW ? NULL : pW);
		/* 負けた側は、対戦ではなくなったので連勝を 0 にする（席を立つのは呼んだ側） */
		L.nWinStreak = 0;
	}
}

/* 席を立たせる。bOver=ゲームオーバー（得点を知らせ、本人に OVER を送る）。bNotifySelf でなければ本人へは何も送らない（切断）。
   対戦中に席を立つ・切断・離れた時は、残った側の勝ち（連勝 +1。残った側は席に残る）。
   bSolo=FALSE は、対戦の負け（時間切れ）で立たせる時（ひとり用のゲームオーバーの知らせ・結果を出さない。結果は VsResolve が済ませてある）。
   結果（得点・勝敗・連勝）は FinishResult（ReportResult）へ集まる */
static void Leave(int nTable, BOOL bOver, BOOL bNotifySelf, BOOL bSolo = TRUE)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	PCHARINFO pSelf = SeatChar (Table);
	char szMsg[128];

	if (Table.bVs) {
		int nOpp = FindTable (Table.dwOppNpc, Table.dwMapID);

		if ((nOpp >= 0) && (SeatChar (s_aTable[nOpp]) != NULL)) {
			VsResolve (nOpp, nTable, PB_VS_WIN_A, TRUE, bNotifySelf);
		} else {
			Table.bVs = FALSE;
		}
		bSolo = FALSE;
	}
	if (bOver && pSelf && bSolo) {
		sprintf (szMsg, "ゲームオーバー！ 得点 %u", Table.Game.dwScore);
		SendCmdSYSMSG (pSelf->pID, szMsg);
	}
	if (bOver && bSolo) {
		ReportResult (Table, pSelf, PBRESULT_SOLO, 0, 0);
	}
	/* 本人へ：ゲームオーバーは OVER（字を 3 秒ほど見せる）。ほかの人（と、席を立っただけの本人）へは待機中の台の状態 */
	if (bOver && pSelf && bNotifySelf) {
		std::vector<BYTE> aOver;
		BuildState (Table, PINBALLSTATE_OVER, aOver, FALSE);
		SendStateTo (Table, pSelf, aOver);
	}
	ToIdle (nTable, bOver);
	SendTable (nTable, PINBALLSTATE_FULL, (bOver || !bNotifySelf) ? pSelf : NULL);
	/* 本人へ待機中の盤を送らなかった時は、送ってある案内の値を今の値にそろえておく（見直しで OVER の表示を消さない） */
	if (pSelf && (bOver || !bNotifySelf)) {
		PINBALLWATCH w;
		int nW;

		w.dwChar = pSelf->dwTblID;
		w.dwPID = pSelf->pID;
		w.byFlag = 0;
		nW = FindWatch (Table.aWatch, w);
		if (nW >= 0) {
			Table.aWatch[nW].byFlag = SeatFlag (Table, pSelf);
		}
	}
}

/* ゲームを進めた結果の出来事を処理する。台が片付いたら（ゲームオーバー）FALSE */
static BOOL HandleEvents(int nTable, int nEv)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	PCHARINFO pChar = SeatChar (Table);
	char szMsg[96];

	if (pChar == NULL) {
		return TRUE;
	}
	if (nEv & PB_EV_SAVE) {
		SendCmdSYSMSG (pChar->pID, "ボールセーブ！ もう一度打ち出せます。");
	} else if ((nEv & PB_EV_DRAIN) && !(nEv & PB_EV_OVER) && !Table.bVs) {
		sprintf (szMsg, "球が落ちました。残り%d球", Table.Game.nBallsLeft);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}
	if (nEv & PB_EV_BONUS) {
		sprintf (szMsg, "ターゲットを全部倒した！ ボーナス %d", Table.Game.Cfg.nScoreBonus);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}
	if (nEv & PB_EV_OVER) {
		Leave (nTable, TRUE, TRUE);
		return FALSE;
	}
	/* 得点・球・状態が変わった時はすぐ送る */
	if (nEv & (PB_EV_BUMPER | PB_EV_TARGET | PB_EV_BONUS | PB_EV_DRAIN | PB_EV_LAUNCH | PB_EV_NEXT | PB_EV_SAVE | PB_EV_STALL)) {
		SendTable (nTable, PINBALLSTATE_FULL);
	}
	return TRUE;
}

/* ゲームを今の時刻まで進める。台が片付いたら FALSE */
static BOOL Advance(int nTable)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	DWORD dwDt = (DWORD)(g_dwTime - Table.dwLastTick);
	int nEv;

	Table.dwLastTick = g_dwTime;
	if (dwDt == 0) {
		return TRUE;
	}
	nEv = Table.Game.Update (dwDt);
	return nEv ? HandleEvents (nTable, nEv) : TRUE;
}

static int CountCoin(PCHARINFO pChar, WORD wItem)
{
	int i, nRet = 0;

	for (i = 0; i < MAX_MYITEM; i ++) {
		if (pChar->waItem[i] == wItem) {
			nRet ++;
		}
	}
	return nRet;
}

/* ========================================================================= */
/* 外から呼ぶ関数															 */
/* ========================================================================= */

BOOL PinballIsTableNpc(PCHARINFO pNpc)
{
	return LoadParam (pNpc, NULL);
}

BOOL PinballIsMarkedNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'i');
}

static BOOL OnSeatCell(PCHARINFO pChar, PCHARINFO pNpc)
{
	PINBALLPARAM Param;

	if (!LoadParam (pNpc, &Param) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return FALSE;
	}
	return ((int)pChar->dwX == (int)pNpc->dwX + Param.nSdx) && ((int)pChar->dwY == (int)pNpc->dwY + Param.nSdy);
}

BOOL PinballIsSeated(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable);
}

PinballGame *PinballGetGame(PCHARINFO pChar)
{
	int nTable;

	return FindSeat (pChar, &nTable) ? &s_aTable[nTable].Game : NULL;
}

int PinballSeatCount(void)
{
	int i, nRet = 0;

	for (i = 0; i < (int)s_aTable.size (); i ++) {
		if (!s_aTable[i].bIdle) {
			nRet ++;
		}
	}
	return nRet;
}

int PinballIdleCount(void)
{
	return (int)s_aTable.size () - PinballSeatCount ();
}

/* 「乱入しますか？」「乱入されてもよいですか？」への答え（CMD_TALKANSWER から。問いを出していなければ FALSE） */
BOOL PinballAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes)
{
	std::map<DWORD, PINBALLASK>::iterator it;
	PINBALLASK Ask;
	PCHARINFO pNpc;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return FALSE;
	}
	it = s_mapAsk.find (pChar->dwTblID);
	if ((it == s_mapAsk.end ()) || (it->second.dwNpc != dwNpcTblID)) {
		return FALSE;
	}
	Ask = it->second;
	s_mapAsk.erase (it);
	if (((DWORD)(g_dwTime - Ask.dwTime) > (DWORD)PINBALL_ASK_TIMEOUT_MS) || (dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return TRUE;
	}
	pNpc = g_aCharInfo[dwNpcTblID];
	/* 座る条件は PinballSit が改めて確かめる（その間に台が使われた・立つ場所を離れた時は座らない）。
	   乱入の問い：はい=乱入、いいえ=ひとりで。乱入されてもよいかの問い：はい=乱入OK、いいえ=乱入NG */
	PinballSit (pChar, pNpc, Ask.bCoinUsed, Ask.bAllowAsk ? (bYes ? 3 : 4) : (bYes ? 1 : 2));
	return TRUE;
}

/* ペアの台（同じマップ・同じ空間で、ペア番号が同じ別の台NPC）のテーブルIDを探す（無ければ PINBALL_NOCHAR） */
static DWORD FindPartnerNpc(PCHARINFO pNpc, const PINBALLPARAM &Param)
{
	PMAPINFO pMap;
	int i;

	if ((Param.nPair <= 0) || (pNpc->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return PINBALL_NOCHAR;
	}
	pMap = g_aMapInfo[pNpc->dwMapID];
	if (pMap == NULL || pMap->paTblNoNPC == NULL) {
		return PINBALL_NOCHAR;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		PCHARINFO pTmp;
		PINBALLPARAM Tmp;

		if (pMap->paTblNoNPC[i] == (DWORD)-1) {
			break;
		}
		pTmp = g_aCharInfo[pMap->paTblNoNPC[i]];
		if ((pTmp == pNpc) || !IsSameSpace (pTmp, pNpc) || !LoadParam (pTmp, &Tmp)) {
			continue;
		}
		if (Tmp.nPair == Param.nPair) {
			return pTmp->dwTblID;
		}
	}
	return PINBALL_NOCHAR;
}

BOOL PinballUseItem(PCHARINFO pChar, WORD wItem)
{
	PMAPINFO pMap;
	int i;

	if ((wItem == 0) || (pChar->byNpcType != NPCTYPE_INIT) || (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ())) {
		return FALSE;
	}
	pMap = g_aMapInfo[pChar->dwMapID];
	if (pMap == NULL || pMap->paTblNoNPC == NULL) {
		return FALSE;
	}
	for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
		PCHARINFO pNpc;
		PINBALLPARAM Param;

		if (pMap->paTblNoNPC[i] == (DWORD)-1) {
			break;
		}
		pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
		if (!OnSeatCell (pChar, pNpc) || !LoadParam (pNpc, &Param)) {
			continue;
		}
		if ((Param.wCoinItem != wItem) || (Param.nCoinCount <= 0)) {
			continue;
		}
		PinballSit (pChar, pNpc, TRUE);
		return TRUE;
	}
	return FALSE;
}

void PinballSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bCoinUsed, int nChoice)
{
	PINBALLPARAM Param;
	int i, nTable, nPartner = -1;
	DWORD dwPartnerNpc;
	char szMsg[128];

	if (!LoadParam (pNpc, &Param) || (pChar->byNpcType != NPCTYPE_INIT) || PinballIsSeated (pChar) || PuzzleIsSeated (pChar) || MazeIsSeated (pChar) || JanIsSeated (pChar)) {
		return;
	}
	/* 座れるのは、台の座る場所のマスの上だけ */
	if (!OnSeatCell (pChar, pNpc)) {
		return;
	}
	/* 倒れている・ほかのミニゲーム中は座れない */
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0) || (pChar->abySubStatus[0] != 0)) {
		return;
	}

	/* この台は席が 1 つ。使われていれば座れない（待機中の台は座れる） */
	nTable = FindTable (pNpc->dwTblID, pNpc->dwMapID);
	if ((nTable >= 0) && !s_aTable[nTable].bIdle) {
		SendCmdSYSMSG (pChar->pID, "この台は今、使われています。");
		return;
	}
	/* 有料の台（必要枚数 1 以上）は S キーでは始まらない。道具のコインを使って始める */
	if (!bCoinUsed && (Param.wCoinItem != 0) && (Param.nCoinCount > 0)) {
		SendCmdSYSMSG (pChar->pID, "コインを使って始めます。コマンドの道具からコインを使ってください。");
		return;
	}
	/* ペアの台でひとりが遊んでいれば、対戦になる */
	dwPartnerNpc = FindPartnerNpc (pNpc, Param);
	if (dwPartnerNpc != PINBALL_NOCHAR) {
		nPartner = FindTable (dwPartnerNpc, pNpc->dwMapID);
		if ((nPartner >= 0) && (s_aTable[nPartner].bVs || s_aTable[nPartner].bIdle || (SeatChar (s_aTable[nPartner]) == NULL))) {
			nPartner = -1;
		}
	}
	/* ペアの台があるのに乱入できない（ひとりで遊んでいる人が乱入NGを選んでいる）時は、対戦にならない */
	if ((nPartner >= 0) && !s_aTable[nPartner].bAllowVs) {
		nPartner = -1;
		if (nChoice == 0) {
			SendCmdSYSMSG (pChar->pID, "相手は乱入を受け付けていません。");
		}
	}
	/* ひとりが遊んでいる台のペアへ座る時は、乱入するか、ひとりで遊ぶかを聞く（コインは答えを受けてから取り上げる） */
	if ((nPartner >= 0) && (nChoice == 0)) {
		PINBALLASK Ask;
		char szAsk[160];
		PCHARINFO pOther = SeatChar (s_aTable[nPartner]);

		Ask.dwNpc = pNpc->dwTblID;
		Ask.dwTime = g_dwTime;
		Ask.bCoinUsed = bCoinUsed;
		Ask.bAllowAsk = FALSE;
		s_mapAsk[pChar->dwTblID] = Ask;
		sprintf (szAsk, "%sさんが遊んでいます。乱入して対戦しますか？（「いいえ」でひとりで遊びます）@y", pOther ? pOther->szName : "だれか");
		SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szAsk);
		return;
	}
	/* ペアの台（相手と組める台）には、始める前に、乱入されてもよいかを聞く（対戦に乱入するのでなければ） */
	if ((nPartner < 0) && (nChoice == 0) && (Param.nPair > 0) && (FindPartnerNpc (pNpc, Param) != PINBALL_NOCHAR)) {
		PINBALLASK Ask;

		Ask.dwNpc = pNpc->dwTblID;
		Ask.dwTime = g_dwTime;
		Ask.bCoinUsed = bCoinUsed;
		Ask.bAllowAsk = TRUE;
		s_mapAsk[pChar->dwTblID] = Ask;
		SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, "遊んでいる途中で乱入されてもよいですか？（「いいえ」だと乱入されずにひとりで遊べます）@y");
		return;
	}
	s_mapAsk.erase (pChar->dwTblID);
	if (nChoice == 2) {
		nPartner = -1;		/* ひとりで遊ぶ */
	}

	/* コイン（無料の台は何も使わない）。足りなければ座らせない */
	if ((Param.wCoinItem != 0) && (Param.nCoinCount > 0)) {
		if (CountCoin (pChar, Param.wCoinItem) < Param.nCoinCount) {
			SendCmdSYSMSG (pChar->pID, "コインがありません。");
			return;
		}
		for (i = 0; i < Param.nCoinCount; i ++) {
			QuestTakeItemOne (pChar, Param.wCoinItem);
		}
		/* 取り上げた分を本人の画面の持ち物に反映する */
		SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
		sprintf (szMsg, "コインを%d枚使いました。", Param.nCoinCount);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}

	{
		PINBALLTABLE Table;
		Table.dwNpc = pNpc->dwTblID;
		Table.dwMapID = pNpc->dwMapID;
		Table.Param = Param;
		Table.dwChar = pChar->dwTblID;
		Table.dwPID = pChar->pID;
		Table.dwAck = 0;
		Table.dwLastKey = 0;
		Table.nKeyTokens = 0;
		Table.Game.Start (&Param.Cfg);
		Table.dwLastTick = g_dwTime;
		Table.dwLastSend = g_dwTime;
		Table.bCharging = FALSE;
		Table.dwChargeStart = 0;
		Table.dwLastWatch = 0;
		Table.bIdle = FALSE;
		Table.bOverShow = FALSE;
		Table.dwOverTime = 0;
		Table.dwOverScore = 0;
		VsClear (Table);
		Table.bAllowVs = (nChoice != 2) && (nChoice != 4);	/* 乱入NG・乱入を断ってひとりで遊ぶ人は乱入を受けない */
		if (nTable >= 0) {
			/* 待機中の台に座る：今見ている人の表を引き継ぐ */
			Table.aWatch = s_aTable[nTable].aWatch;
			s_aTable[nTable] = Table;
		} else {
			s_aTable.push_back (Table);
			nTable = (int)s_aTable.size () - 1;
		}
	}
	if (nPartner < 0) {
		sprintf (szMsg, "ピンボール開始！ %d球。↓を押して離すと打ち出し、Xで左・→で右のフリッパー。", Param.Cfg.nBalls);
		SendCmdSYSMSG (pChar->pID, szMsg);
	} else {
		/* 対戦開始：両方の盤を最初に戻し、同じ乱数の種でカウントダウンのあと同時にスタート（球は落ちても次が出る時間制） */
		PINBALLTABLE &Table = s_aTable[nTable];
		PINBALLTABLE &Partner = s_aTable[nPartner];
		unsigned int dwSeed = (unsigned int)(g_dwTime * 2654435761u) ^ (pChar->dwTblID * 40503u + 1);
		DWORD dwStart = g_dwTime + PB_VS_COUNT_SEC * 1000;
		DWORD dwEnd = dwStart + (DWORD)Partner.Param.nVsSec * 1000;		/* 時間は、先に遊んでいた側の台の設定 */
		PCHARINFO pOther = SeatChar (Partner);

		Table.Game.StartVs (&Table.Param.Cfg, dwSeed);
		Partner.Game.StartVs (&Partner.Param.Cfg, dwSeed);
		Table.bVs = Partner.bVs = TRUE;
		Table.dwOppNpc = Partner.dwNpc;
		Partner.dwOppNpc = Table.dwNpc;
		Table.bAllowVs = Partner.bAllowVs = FALSE;		/* 対戦中は乱入を受けない（終わると勝った側は TRUE に戻る） */
		Table.bCount = Partner.bCount = TRUE;
		Table.dwStartTime = Partner.dwStartTime = dwStart;
		Table.dwEndTime = Partner.dwEndTime = dwEnd;
		Table.nCountSent = Partner.nCountSent = PB_VS_COUNT_SEC;
		Table.dwLastTick = Partner.dwLastTick = dwStart;
		Partner.bCharging = FALSE;
		Partner.dwLastKey = 0;
		Partner.nKeyTokens = 0;
		if (pOther) {
			sprintf (szMsg, "対戦開始！ 相手は%sです。%d秒間、球は何個でも出ます。", pOther->szName, Partner.Param.nVsSec);
			SendCmdSYSMSG (pChar->pID, szMsg);
			sprintf (szMsg, "対戦開始！ 相手は%sです。%d秒間、球は何個でも出ます。", pChar->szName, Partner.Param.nVsSec);
			SendCmdSYSMSG (pOther->pID, szMsg);
		}
		/* 相手の盤（最初に戻った）を今の見ている人へ。新しく見える人（今座った人）へは見直しで届く */
		SendTable (nPartner, PINBALLSTATE_FULL);
		WatchRefresh (nPartner);
	}
	/* 見ている人全員へ始まった状態。新しく見える人へは見直しで届く */
	SendTable (nTable, PINBALLSTATE_FULL);
	WatchRefresh (nTable);
}

void PinballKey(PCHARINFO pChar, BYTE byKey, DWORD dwSerial)
{
	int nTable, nEv = 0;
	BOOL bSend = FALSE;
	DWORD dwAdd;

	if (!FindSeat (pChar, &nTable)) {
		return;
	}
	if (dwSerial != 0) {
		s_aTable[nTable].dwAck = dwSerial;
	}
	if (byKey == PINBALLKEY_LEAVE) {
		Leave (nTable, FALSE, TRUE);
		return;
	}
	/* カウントダウン中は操作できない（席を立つ以外は受けない） */
	if (s_aTable[nTable].bCount) {
		return;
	}
	/* 入力の時刻に合わせて、まず今までの経過を進める（ゲームオーバーで片付いたら終わり） */
	if (!Advance (nTable)) {
		return;
	}
	PINBALLTABLE &Table = s_aTable[nTable];
	PinballGame &g = Table.Game;
	switch (byKey) {
	case PINBALLKEY_LEFT_DOWN:	g.SetFlipper (0, 1);	bSend = TRUE;	break;
	case PINBALLKEY_LEFT_UP:	g.SetFlipper (0, 0);	bSend = TRUE;	break;
	case PINBALLKEY_RIGHT_DOWN:	g.SetFlipper (1, 1);	bSend = TRUE;	break;
	case PINBALLKEY_RIGHT_UP:	g.SetFlipper (1, 0);	bSend = TRUE;	break;
	case PINBALLKEY_LAUNCH_DOWN:
		if ((g.nPhase == PB_PHASE_READY) && !Table.bCharging) {
			Table.bCharging = TRUE;
			Table.dwChargeStart = g_dwTime;
			bSend = TRUE;
		}
		break;
	case PINBALLKEY_LAUNCH_UP:
		if (Table.bCharging) {
			Table.bCharging = FALSE;
			if (g.Launch ((int)(g_dwTime - Table.dwChargeStart))) {
				nEv |= PB_EV_LAUNCH;
			}
			bSend = TRUE;
		}
		break;
	default:
		return;
	}
	/* 観戦者へ送りすぎないよう、入力に伴う即時の送信はバケツ方式で制限する（状態は周期処理でも送る） */
	if (Table.dwLastKey == 0) {
		Table.nKeyTokens = PINBALL_KEY_BUCKET;
		Table.dwLastKey = g_dwTime;
	}
	dwAdd = (DWORD)(g_dwTime - Table.dwLastKey) / PINBALL_KEY_FILL_MS;
	if ((int)dwAdd >= PINBALL_KEY_BUCKET - Table.nKeyTokens) {
		Table.nKeyTokens = PINBALL_KEY_BUCKET;
		Table.dwLastKey = g_dwTime;
	} else {
		Table.nKeyTokens += (int)dwAdd;
		Table.dwLastKey += dwAdd * PINBALL_KEY_FILL_MS;
	}
	if (bSend && (Table.nKeyTokens > 0)) {
		Table.nKeyTokens --;
		SendTable (nTable, PINBALLSTATE_FULL);
	}
	(void)nEv;
}

/* 待機中の台を新しく表に載せる：人のいるマップの台NPC（席が空）のうち、近くに人がいるものだけ */
static void IdleScan(void)
{
	int nMap, i, j;

	for (nMap = 0; nMap < (int)g_aMapInfo.GetSize (); nMap ++) {
		PMAPINFO pMap = g_aMapInfo[nMap];

		if ((pMap == NULL) || (pMap->paTblNoPC == NULL) || (pMap->paTblNoNPC == NULL) || (pMap->dwCountTblNoPC == 0) ||
			(pMap->paTblNoPC[0] == (DWORD)-1)) {
			continue;
		}
		for (i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
			PCHARINFO pNpc;
			PINBALLPARAM Param;
			BOOL bNear = FALSE;

			if (pMap->paTblNoNPC[i] == (DWORD)-1) {
				break;
			}
			if (pMap->paTblNoNPC[i] >= (DWORD)g_aCharInfo.GetSize ()) {
				continue;
			}
			pNpc = g_aCharInfo[pMap->paTblNoNPC[i]];
			if ((FindTable (pNpc->dwTblID, (DWORD)nMap) >= 0) || !LoadParam (pNpc, &Param)) {
				continue;
			}
			for (j = 0; j < (int)pMap->dwCountTblNoPC; j ++) {
				PCHARINFO pChar;

				if (pMap->paTblNoPC[j] == (DWORD)-1) {
					break;
				}
				if (pMap->paTblNoPC[j] >= (DWORD)g_aCharInfo.GetSize ()) {
					continue;
				}
				pChar = g_aCharInfo[pMap->paTblNoPC[j]];
				if (WatchEligible (Param, (DWORD)nMap, pNpc, pChar, PINBALL_VIEW_ENTER)) {
					bNear = TRUE;
					break;
				}
			}
			if (bNear) {
				PINBALLTABLE Table;

				Table.dwNpc = pNpc->dwTblID;
				Table.dwMapID = (DWORD)nMap;
				Table.Param = Param;
				Table.dwChar = PINBALL_NOCHAR;
				Table.dwPID = 0;
				Table.dwAck = 0;
				Table.dwLastKey = 0;
				Table.nKeyTokens = 0;
				Table.Game.Start (&Param.Cfg);
				Table.dwLastTick = g_dwTime;
				Table.dwLastSend = g_dwTime;
				Table.bCharging = FALSE;
				Table.dwChargeStart = 0;
				Table.dwLastWatch = 0;
				Table.bIdle = TRUE;
				Table.bOverShow = FALSE;
				Table.dwOverTime = 0;
				Table.dwOverScore = 0;
				VsClear (Table);
				s_aTable.push_back (Table);
				WatchRefresh ((int)s_aTable.size () - 1);
			}
		}
	}
}

/* 対戦の時間切れ。両方のゲームを今の時刻まで進めてから、得点で勝敗を決める。勝った側は席に残り、負けた側は席を立つ */
static void VsTimeUp(int nTable)
{
	int nOpp = FindTable (s_aTable[nTable].dwOppNpc, s_aTable[nTable].dwMapID);
	int nJudge;

	if ((nOpp < 0) || (SeatChar (s_aTable[nOpp]) == NULL) || !s_aTable[nOpp].bVs) {
		EndVs (nTable, NULL);		/* 相手がいない（ありえない）：ひとり用に戻す */
		return;
	}
	Advance (nTable);
	Advance (nOpp);
	nJudge = PinballVsJudge (s_aTable[nTable].Game.dwScore, s_aTable[nOpp].Game.dwScore);
	VsResolve (nTable, nOpp, nJudge, TRUE, TRUE);
	if (nJudge == PB_VS_WIN_A) {
		Leave (nOpp, TRUE, TRUE, FALSE);
	} else if (nJudge == PB_VS_WIN_B) {
		Leave (nTable, TRUE, TRUE, FALSE);
	}
}

void PinballProc(void)
{
	static DWORD s_dwLastScan = 0;
	int i;

	/* 台が消えると添字がずれるので、後ろから 1 つずつ */
	for (i = (int)s_aTable.size () - 1; i >= 0; i --) {
		if (i >= (int)s_aTable.size ()) {
			continue;
		}
		PINBALLTABLE &Table = s_aTable[i];
		PCHARINFO pNpc = NpcOf (Table);
		BOOL bNpcOK = PinballIsTableNpc (pNpc) && (pNpc->dwMapID == Table.dwMapID);
		PCHARINFO pChar = SeatChar (Table);

		/* 待機中の台：台が無くなれば見ている人へ NONE を送って表から消す。近くの人の見直し。誰もいなくなれば表から消す */
		if (Table.bIdle) {
			if (!bNpcOK) {
				std::vector<BYTE> aNone;
				int k;

				BuildState (Table, PINBALLSTATE_NONE, aNone, FALSE);
				for (k = 0; k < (int)Table.aWatch.size (); k ++) {
					PCHARINFO pW = WatchChar (Table.aWatch[k]);
					if (pW) {
						SendStateTo (Table, pW, aNone);
					}
				}
				s_aTable.erase (s_aTable.begin () + i);
				continue;
			}
			/* 台の設定が変わっていたら取り込んで、見ている人へ送り直す（盤の位置・座る場所・帯の側など） */
			{
				PINBALLPARAM Np;

				if (LoadParam (pNpc, &Np) && !ParamSame (Np, Table.Param)) {
					Table.Param = Np;
					Table.Game.Start (&Np.Cfg);
					SendTable (i, PINBALLSTATE_FULL);
				}
			}
			/* GAME OVER の字を消す（見ている全員へ byOver=0 の盤を送る） */
			if (Table.bOverShow && ((DWORD)(g_dwTime - Table.dwOverTime) >= (DWORD)PINBALL_OVER_SHOW_MS)) {
				Table.bOverShow = FALSE;
				SendTable (i, PINBALLSTATE_FULL);
			}
			if ((DWORD)(g_dwTime - Table.dwLastWatch) >= PINBALL_WATCH_MS) {
				WatchRefresh (i);
			}
			if (Table.aWatch.empty ()) {
				s_aTable.erase (s_aTable.begin () + i);
			}
			continue;
		}

		/* 切断・マップ移動・席から離れた・倒れた・台が無くなった時は片付ける */
		if ((pChar == NULL) || !bNpcOK || (pChar->byGameState < GAMESTATE_MOVE) ||
			(pChar->dwMapID != Table.dwMapID) ||
			(abs ((int)pChar->dwX - ((int)pNpc->dwX + Table.Param.nSdx)) > PINBALL_NEAR) ||
			(abs ((int)pChar->dwY - ((int)pNpc->dwY + Table.Param.nSdy)) > PINBALL_NEAR) ||
			(pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
			Leave (i, FALSE, TRUE);
			continue;
		}
		if ((DWORD)(g_dwTime - Table.dwLastWatch) >= PINBALL_WATCH_MS) {
			WatchRefresh (i);
		}
		/* 対戦開始前のカウントダウン（数が変わった時と、始まる時に知らせる。それまでゲームを進めない） */
		if (Table.bCount) {
			if (PinballVsTimeUp (g_dwTime, Table.dwStartTime)) {
				Table.bCount = FALSE;
				Table.nCountSent = 0;
				Table.dwLastTick = g_dwTime;
				SendTable (i, PINBALLSTATE_FULL);
			} else {
				int nRemain = CountRemain (Table);

				if ((nRemain != Table.nCountSent) || ((DWORD)(g_dwTime - Table.dwLastSend) >= PINBALL_SEND_MS)) {
					Table.nCountSent = nRemain;
					SendTable (i, PINBALLSTATE_FULL);
				}
				continue;
			}
		}
		/* ゲームを進める。片付いたらこの台は終わり */
		{
			int nSize = (int)s_aTable.size ();
			if (!Advance (i) || (nSize != (int)s_aTable.size ())) {
				continue;
			}
		}
		/* 対戦の時間切れ：得点が高い方の勝ち（同点は引き分け） */
		if (s_aTable[i].bVs && PinballVsTimeUp (g_dwTime, s_aTable[i].dwEndTime)) {
			VsTimeUp (i);
			continue;
		}
		/* 約 0.1 秒ごとに状態を送る */
		if ((DWORD)(g_dwTime - s_aTable[i].dwLastSend) >= PINBALL_SEND_MS) {
			SendTable (i, PINBALLSTATE_FULL);
		}
	}
	/* 待機中の台を新しく見つける（PINBALL_WATCH_MS ごと） */
	if ((DWORD)(g_dwTime - s_dwLastScan) >= PINBALL_WATCH_MS) {
		s_dwLastScan = g_dwTime;
		IdleScan ();
	}
}

void PinballOnLeave(PCHARINFO pChar)
{
	int nTable;

	if (FindSeat (pChar, &nTable)) {
		Leave (nTable, FALSE, FALSE);
	}
}
