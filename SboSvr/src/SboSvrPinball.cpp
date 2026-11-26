/* ========================================================================= */
/* ファイル名：	SboSvrPinball.cpp											 */
/* 概要：		ピンボールの台（台NPC・席・ゲームの進行）の処理(実装ファイル)	 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 仕様は docs/pinball.md。物理・進行は common/Pinball.h（サーバーとクライアントで同じコード）。
 * 作りは落ち物パズル（SboSvrPuzzle.cpp）をなぞっている（台NPC・席・コイン・待機中の台・近くの人への送信）。
 * 台は、台詞が "@i" で始まる NPC。台詞のパラメータ（コンマ区切り。後ろは省略でき、空欄も既定になる）:
 *     @i<コインのアイテム番号>,<必要な枚数>,<dx>,<dy>,<帯の側 0=右 1=左>,<sdx>,<sdy>
 *   dx, dy … 盤の左上が、台のマスの左上から何マスか（負は左・上）。省略は盤の真下に席（160×224 なら -2,-6）
 *   帯の側 … 得点・残り球の帯を盤のどちら側に出すか（省略は右）
 *   sdx, sdy … 座る場所：台NPCのマスからの相対位置（省略は 0,+1）
 *   例  @i20,1  … アイテム 20 を 1 枚使って遊ぶ。  @i … 無料
 *   枚数 0 かアイテム番号 0 なら無料。台NPCは見えない（byGhost を CHARGHOST_NAMEONLY にして送る）。
 * ゲームの正はサーバー。周期処理（PinballProc）で g_dwTime の経過時間から固定ステップ（1/120 秒）を回す。
 * キーは押した時と離した時だけ届く（PINBALLKEY_*）。届いた時、まず経過時間ぶんゲームを進めてから反映する（入力の時刻が合う）。
 * 状態は PINBALLSTATE_FULL を、約 PINBALL_SEND_MS ごとと、得点・球・状態が変わった時に、盤を見ている全員
 * （座っている人・台の近くにいる人）へ送る。本人のクライアントは common/Pinball.h で先読みし、
 * 届いた状態（ステップ数 dwStep・ボール・フリッパー）から巻き直して少しずつ寄せる。観戦者は補間して描く。
 * 記録（ランキング）・対戦・台のデータ化は今回やらない（後で足せるよう、終了は Leave の 1 か所に集めてある）。
 */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrPinball.h"
#include "../../common/Pinball.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#define PINBALL_NOCHAR			((DWORD)-1)
#define PINBALL_NEAR			(3)			/* 座る場所のマスからこれ以上離れたら席を立たせる（マス） */
#define PINBALL_WATCH_MS		(250)		/* 台の近くにいる人（観戦）を見直す間隔(ms) */
#define PINBALL_SEND_MS			(100)		/* 状態を送る間隔(ms) */
#define PINBALL_KEY_BUCKET		(8)			/* キー入力に伴う即時の送信のバケツ（連打で観戦者へ送りすぎない） */
#define PINBALL_KEY_FILL_MS		(25)
#define PINBALL_OVER_SHOW_MS	(3000)		/* ゲームオーバーの字を出している時間(ms) */
/* 盤面を送る距離（落ち物パズルと同じ。盤の中心から。盤は 5×7 マス） */
#define PINBALL_VIEW_MARGIN		(12)
#define PINBALL_VIEW_ENTER		(PINBALL_VIEW_MARGIN + 3)
#define PINBALL_VIEW_LEAVE		(PINBALL_VIEW_MARGIN + 6)
#define PINBALL_TILES_W			(PB_W / PBP_TILE_DOT)		/* 盤の横のマス数（切り捨て） */
#define PINBALL_TILES_H			(PB_H / PBP_TILE_DOT)

/* 台の設定（台NPCの台詞から読む） */
typedef struct _PINBALLPARAM {
	WORD	wCoinItem;						/* コインのアイテム番号（0=無料） */
	int		nCoinCount;						/* 必要な枚数（0=無料） */
	int		nDx;							/* 盤の左上：台のマスの左上から（マス） */
	int		nDy;
	int		nSide;							/* 帯の側（PUZZLESIDE_*） */
	int		nSdx;							/* 座る場所：台のマスから（マス。既定 0,+1） */
	int		nSdy;
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
} PINBALLTABLE;

static std::vector<PINBALLTABLE> s_aTable;

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

static int PbClamp(int n, int nMin, int nMax) { return (n < nMin) ? nMin : (n > nMax) ? nMax : n; }

/* 台詞 "@i..." のパラメータを読む（読み方は common/Pinball.h の PinballParseSpeech） */
static void ParseParam(LPCSTR pszSpeech, PINBALLPARAM *pParam)
{
	int anVal[PBP_COUNT];

	PinballParseSpeech (pszSpeech, anVal);
	pParam->wCoinItem	= ((anVal[0] < 0) || (anVal[0] > 65535)) ? 0 : (WORD)anVal[0];
	pParam->nCoinCount	= (anVal[1] < 0) ? 0 : (anVal[1] > 99) ? 99 : anVal[1];
	pParam->nDx = (anVal[2] == PBP_NONE) ? PinballDefaultDx () : PbClamp (anVal[2], -60, 60);
	pParam->nDy = (anVal[3] == PBP_NONE) ? PinballDefaultDy () : PbClamp (anVal[3], -60, 60);
	pParam->nSide = (anVal[4] == 1) ? PUZZLESIDE_LEFT : PUZZLESIDE_RIGHT;
	PinballSeatOffset (pszSpeech, &pParam->nSdx, &pParam->nSdy);
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
	Head.byFlash	= (BYTE)(bFlash ? g.nFlash : 0);
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
	if (bIdle) {
		/* 待機中は盤の枠だけ（ボール・帯なし）。ゲームオーバーの字の間だけ最後の得点を持つ */
		Head.byPhase = PB_PHASE_OVER;
		Head.byBalls = 0;
		Head.byTargets = 0;
		Head.byFlash = 0;
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
		PuzzleIsSeated (pChar) || PinballIsSeated (pChar) ||
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
	nCy = (nFy + PB_H / 2) / PBP_TILE_DOT;
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
	PCHARINFO pSeat;
	PMAPINFO pMap;
	int i;

	Table.dwLastWatch = g_dwTime;
	if (pNpc == NULL) {
		return;
	}
	pSeat = SeatChar (Table);
	if (pSeat) {
		PINBALLWATCH w;
		w.dwChar = pSeat->dwTblID;
		w.dwPID = pSeat->pID;
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
			if (pChar == pSeat) {
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
	Table.Game.Start ();
}

/* 席を立たせる。bOver=ゲームオーバー（得点を知らせ、本人に OVER を送る）。bNotifySelf でなければ本人へは何も送らない（切断）。
   記録（ランキング）を足す時はここ（得点が決まるのはここだけ） */
static void Leave(int nTable, BOOL bOver, BOOL bNotifySelf)
{
	PINBALLTABLE &Table = s_aTable[nTable];
	PCHARINFO pSelf = SeatChar (Table);
	char szMsg[128];

	if (bOver && pSelf) {
		sprintf (szMsg, "ゲームオーバー！ 得点 %u", Table.Game.dwScore);
		SendCmdSYSMSG (pSelf->pID, szMsg);
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
	} else if ((nEv & PB_EV_DRAIN) && !(nEv & PB_EV_OVER)) {
		sprintf (szMsg, "球が落ちました。残り%d球", Table.Game.nBallsLeft);
		SendCmdSYSMSG (pChar->pID, szMsg);
	}
	if (nEv & PB_EV_BONUS) {
		sprintf (szMsg, "ターゲットを全部倒した！ ボーナス %d", PB_SCORE_BONUS);
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
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'i');
}

static BOOL OnSeatCell(PCHARINFO pChar, PCHARINFO pNpc)
{
	PINBALLPARAM Param;

	if (!PinballIsTableNpc (pNpc) || (pChar->dwMapID != pNpc->dwMapID) || !IsSameSpace (pNpc, pChar)) {
		return FALSE;
	}
	ParseParam (pNpc->pszNpcSpeech, &Param);
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
		if (!PinballIsTableNpc (pNpc) || !OnSeatCell (pChar, pNpc)) {
			continue;
		}
		ParseParam (pNpc->pszNpcSpeech, &Param);
		if ((Param.wCoinItem != wItem) || (Param.nCoinCount <= 0)) {
			continue;
		}
		PinballSit (pChar, pNpc, TRUE);
		return TRUE;
	}
	return FALSE;
}

void PinballSit(PCHARINFO pChar, PCHARINFO pNpc, BOOL bCoinUsed)
{
	PINBALLPARAM Param;
	int i, nTable;
	char szMsg[96];

	if (!PinballIsTableNpc (pNpc) || (pChar->byNpcType != NPCTYPE_INIT) || PinballIsSeated (pChar) || PuzzleIsSeated (pChar)) {
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
	ParseParam (pNpc->pszNpcSpeech, &Param);

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
		Table.Game.Start ();
		Table.dwLastTick = g_dwTime;
		Table.dwLastSend = g_dwTime;
		Table.bCharging = FALSE;
		Table.dwChargeStart = 0;
		Table.dwLastWatch = 0;
		Table.bIdle = FALSE;
		Table.bOverShow = FALSE;
		Table.dwOverTime = 0;
		Table.dwOverScore = 0;
		if (nTable >= 0) {
			/* 待機中の台に座る：今見ている人の表を引き継ぐ */
			Table.aWatch = s_aTable[nTable].aWatch;
			s_aTable[nTable] = Table;
		} else {
			s_aTable.push_back (Table);
			nTable = (int)s_aTable.size () - 1;
		}
	}
	sprintf (szMsg, "ピンボール開始！ %d球。↓を押して離すと打ち出し、←→でフリッパー。", PB_BALLS);
	SendCmdSYSMSG (pChar->pID, szMsg);
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
			if (!PinballIsTableNpc (pNpc) || (FindTable (pNpc->dwTblID, (DWORD)nMap) >= 0)) {
				continue;
			}
			ParseParam (pNpc->pszNpcSpeech, &Param);
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
				Table.Game.Start ();
				Table.dwLastTick = g_dwTime;
				Table.dwLastSend = g_dwTime;
				Table.bCharging = FALSE;
				Table.dwChargeStart = 0;
				Table.dwLastWatch = 0;
				Table.bIdle = TRUE;
				Table.bOverShow = FALSE;
				Table.dwOverTime = 0;
				Table.dwOverScore = 0;
				s_aTable.push_back (Table);
				WatchRefresh ((int)s_aTable.size () - 1);
			}
		}
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
		/* ゲームを進める。片付いたらこの台は終わり */
		{
			int nSize = (int)s_aTable.size ();
			if (!Advance (i) || (nSize != (int)s_aTable.size ())) {
				continue;
			}
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
