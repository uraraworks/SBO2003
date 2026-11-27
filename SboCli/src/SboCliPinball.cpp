/* ========================================================================= */
/* ファイル名：	SboCliPinball.cpp											 */
/* 内容：		ピンボール（台に座って遊ぶ）のクライアント側					 */
/* 作成日：		2026/11/26													 */
/* ========================================================================= */
/*
 * 仕様は docs/pinball.md。物理は common/Pinball.h（サーバーと同じコード）。
 *
 * 本人の台（byMine）：先読み。common/Pinball.h の PinballGame を手元で回し、押した瞬間にフリッパーを動かす。
 *   サーバーの状態が届くたびに、その状態（ステップ数・ボール・速度・フリッパー・ターゲット）から作り直して
 *   （まだサーバーが処理していないキー＝dwAck より後のキーをかけ直す）、今の時刻まで進める。
 *   作り直しでボールの位置が飛ぶ分は、見た目のずれ（nOff）として持ち、1 ステップごとに 15/16 に縮めて少しずつ寄せる。
 *   得点・残り球・ターゲットを決めるのはサーバー（先読みの得点は使わない。穴に落ちたら先読みは止めてサーバーを待つ）。
 * ほかの人（観戦）：先読みしない。届いた位置を補間して、PINBALL_INTERP_MS だけ遅らせて描く。
 */
#include "stdafx.h"
#include "common/Pinball.h"
#include "Glbldata.h"
#include "SboCliSendCmd.h"
#include "SboCliRecvCmd.h"
#include "SboCliPinball.h"

#define PINBALL_PRED_MAX		(32)		/* 先読みで覚えておく「サーバーがまだ処理していないキー」の数 */
#define PINBALL_PRED_DROP_MS	(2000)		/* これだけ返事が来ないキーは捨てる */
#define PINBALL_INTERP_MS		(150)		/* 観戦：描画を遅らせる時間(ms)（サーバーは約 100ms ごとに送る） */
#define PINBALL_HIST_MAX		(8)
#define PINBALL_OVER_SELF_MS	(3000)		/* 本人のゲームオーバーの表示 */
#define PINBALL_OVER_LOCK_MS	(1500)		/* ゲームオーバー直後、座っているのと同じ扱いにする時間（押していたキーで歩かない） */
#define PINBALL_CAM_MS			(90)		/* 窓がボールに寄る速さ（残りの差が半分以下になるのに約この時間×0.7。小さいほど速い） */
#define PINBALL_SNAP_PX			(24)		/* 作り直しでこれ以上ずれたら、寄せずに飛ばす */

typedef struct _PBPRED {
	DWORD		dwTime;						/* 押した（離した）時刻 */
	DWORD		dwSerial;					/* サーバーへ送ったキーの連番 */
	BYTE		byKey;						/* PINBALLKEY_* */
	int			nHoldMs;					/* LAUNCH_UP の時：押していた時間 */
} PBPRED;

typedef struct _PBSAMPLE {
	DWORD		dwTime;						/* 届いた時刻 */
	int			nX, nY;
	BYTE		byPhase, byAngL, byAngR;
} PBSAMPLE;

typedef struct _PBVIEW {
	BOOL					bUse;
	CMDPARA_PINBALLSTATE	St;				/* 最後に届いた状態 */
	DWORD					dwOverTime;		/* 本人のゲームオーバーを受けた時刻（0=なし） */
	DWORD					dwFlashTime[PB_BUMPERS + PB_TARGETS];	/* 当たった印を受けた時刻 */
	/* 本人の先読み */
	PinballGame				Sim;			/* 先読みの状態 */
	BOOL					bSim;			/* Sim が使える */
	DWORD					dwSimTime;		/* Sim を最後に進めた時刻 */
	int						nOffX, nOffY;	/* 見た目のずれ（1/256 ドット。描く位置＝Sim の位置＋これ） */
	PBPRED					aPred[PINBALL_PRED_MAX];
	int						nPred;
	BOOL					bCharging;		/* ↓を押している */
	DWORD					dwChargeStart;
	int						nLatMs;			/* サーバーまでの片道の時間の見積もり（ms。返事の最短から） */
	/* 観戦の補間 */
	PBSAMPLE				aHist[PINBALL_HIST_MAX];
	int						nHist;
	/* 窓の縦位置（盤が窓より高い時。クライアントだけで決める） */
	BOOL					bCam;			/* nCamFp が使える */
	int						nCamFp;			/* 窓の上端の盤上の位置（1/256 ドット） */
	DWORD					dwCamTime;
} PBVIEW;
static PBVIEW s_aView[PINBALL_VIEW_MAX];

/* 台NPCの印（dwNpc）で探す。bAlloc なら無ければ作る */
static PBVIEW *ViewFind(DWORD dwNpc, BOOL bAlloc)
{
	int i, nFree = -1;

	for (i = 0; i < PINBALL_VIEW_MAX; i ++) {
		if (s_aView[i].bUse && (s_aView[i].St.dwNpc == dwNpc)) {
			return &s_aView[i];
		}
		if (!s_aView[i].bUse && (nFree < 0)) {
			nFree = i;
		}
	}
	if (!bAlloc) {
		return NULL;
	}
	if (nFree < 0) {
		nFree = 0;
	}
	ZeroMemory (&s_aView[nFree], sizeof (PBVIEW));
	s_aView[nFree].bUse = TRUE;
	s_aView[nFree].nLatMs = 40;
	return &s_aView[nFree];
}

/* 先読みできる台か（座っている本人の、遊んでいる台） */
static BOOL Predictable(const PBVIEW *pView)
{
	return pView->bUse && pView->St.byMine && !pView->St.byIdle && (pView->dwOverTime == 0) &&
		(pView->St.byPhase != PB_PHASE_OVER);
}

/* サーバーの状態から先読み用のゲームを作る */
static void SimFromState(PinballGame *pG, const CMDPARA_PINBALLSTATE &St)
{
	pG->Start ();
	pG->nPhase = St.byPhase;
	pG->nX = St.nBallX;
	pG->nY = St.nBallY;
	pG->nVx = St.sVx;
	pG->nVy = St.sVy;
	pG->nAngL = St.byAngL;
	pG->nAngR = St.byAngR;
	pG->bHoldL = (St.byHeld & 1) ? 1 : 0;
	pG->bHoldR = (St.byHeld & 2) ? 1 : 0;
	pG->nTargetMask = St.byTargets;
	pG->dwScore = St.dwScore;
	pG->nBallsLeft = St.byBalls;
	pG->dwStep = St.dwStep;
	pG->bPredict = 1;		/* 穴に落ちたら球を減らさず、サーバーの返事を待つ */
}

/* キーを先読みのゲームへかける */
static void SimApply(PinballGame *pG, const PBPRED &P)
{
	switch (P.byKey) {
	case PINBALLKEY_LEFT_DOWN:		pG->SetFlipper (0, 1);	break;
	case PINBALLKEY_LEFT_UP:		pG->SetFlipper (0, 0);	break;
	case PINBALLKEY_RIGHT_DOWN:		pG->SetFlipper (1, 1);	break;
	case PINBALLKEY_RIGHT_UP:		pG->SetFlipper (1, 0);	break;
	case PINBALLKEY_LAUNCH_UP:		pG->Launch (P.nHoldMs);	break;
	}
}

/* 先読みのゲームを今の時刻まで進める。進んだステップぶん、見た目のずれを縮める */
static void SimAdvance(PBVIEW *pView, DWORD dwNow)
{
	DWORD dwDt = (DWORD)(dwNow - pView->dwSimTime);
	unsigned int dwStepBefore = pView->Sim.dwStep;
	unsigned int n;

	pView->dwSimTime = dwNow;
	if (dwDt == 0) {
		return;
	}
	if (dwDt > 250) {
		dwDt = 250;
	}
	pView->Sim.Update (dwDt);
	for (n = dwStepBefore; n < pView->Sim.dwStep; n ++) {
		pView->nOffX = pView->nOffX * 15 / 16;
		pView->nOffY = pView->nOffY * 15 / 16;
	}
}

/* 状態を受け取った本人の台：サーバーの状態から作り直して、まだ処理されていないキーをかけ直し、今の時刻まで進める */
static void Rebase(PBVIEW *pView, DWORD dwNow)
{
	PinballGame T;
	int i, nKeep = 0, nOldX = 0, nOldY = 0;
	BOOL bOldPlay = FALSE;
	DWORD dwVt;

	/* サーバーが処理したキー（dwAck まで）を捨てる。そのうち一番新しいキーを送ってからの時間で、片道の時間を見積もる */
	for (i = 0; i < pView->nPred; i ++) {
		if (pView->aPred[i].dwSerial <= pView->St.dwAck) {
			int nSample = (int)(dwNow - pView->aPred[i].dwTime) / 2;
			if (nSample < 0) nSample = 0;
			if (nSample > 250) nSample = 250;
			/* 返事は最大 0.1 秒の送信間隔ぶん遅れて届くので、最短を取る（ゆっくり増やして外れ値を戻す） */
			if (nSample < pView->nLatMs) pView->nLatMs = nSample; else pView->nLatMs += 1;
		} else if ((int)(dwNow - pView->aPred[i].dwTime) < PINBALL_PRED_DROP_MS) {
			pView->aPred[nKeep ++] = pView->aPred[i];
		}
	}
	pView->nPred = nKeep;

	if (pView->bSim && (pView->Sim.nPhase == PB_PHASE_PLAY)) {
		SimAdvance (pView, dwNow);
		nOldX = pView->Sim.nX + pView->nOffX;
		nOldY = pView->Sim.nY + pView->nOffY;
		bOldPlay = TRUE;
	}
	SimFromState (&T, pView->St);
	/* 状態は片道ぶん古い。その時刻から今まで、未処理のキーを時刻どおりにかけながら進める */
	dwVt = dwNow - (DWORD)pView->nLatMs;
	for (i = 0; i < pView->nPred; i ++) {
		const PBPRED &P = pView->aPred[i];
		if ((int)(P.dwTime - dwVt) > 0) {
			T.Update (P.dwTime - dwVt);
			dwVt = P.dwTime;
		}
		SimApply (&T, P);
	}
	if ((int)(dwNow - dwVt) > 0) {
		T.Update (dwNow - dwVt);
	}
	/* 打ち出しを溜めている最中にサーバーが「打ち出せない」状態ならやめる */
	if ((T.nPhase != PB_PHASE_READY) && pView->bCharging && (pView->St.byHeld & 4) == 0 && (pView->nPred == 0)) {
		pView->bCharging = FALSE;
	}
	/* ボールの位置の飛びは、見た目のずれとして持ち、少しずつ縮める */
	pView->nOffX = pView->nOffY = 0;
	if (bOldPlay && (T.nPhase == PB_PHASE_PLAY)) {
		int dx = nOldX - T.nX, dy = nOldY - T.nY;
		if ((abs (dx) < PINBALL_SNAP_PX * PB_FP) && (abs (dy) < PINBALL_SNAP_PX * PB_FP)) {
			pView->nOffX = dx;
			pView->nOffY = dy;
		}
	}
	pView->Sim = T;
	pView->bSim = TRUE;
	pView->dwSimTime = dwNow;
}

void RecvDataProcPINBALLSTATE(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_PINBALLSTATE pPara = (PCMDPARA_PINBALLSTATE)pData;
	PBVIEW *pView;
	DWORD dwNow = timeGetTime ();
	int i;

	if (dwDataSize < sizeof (CMDPARA_PINBALLSTATE)) {
		return;
	}
	switch (pPara->byKind) {
	case PINBALLSTATE_NONE:
		pView = ViewFind (pPara->dwNpc, FALSE);
		if (pView) {
			pView->bUse = FALSE;
		}
		break;
	case PINBALLSTATE_FULL:
	case PINBALLSTATE_OVER:
		pView = ViewFind (pPara->dwNpc, TRUE);
		memcpy (&pView->St, pData, sizeof (CMDPARA_PINBALLSTATE));
		/* 当たった印（光らせる） */
		for (i = 0; i < PB_BUMPERS + PB_TARGETS; i ++) {
			if (pPara->wFlash & (1 << i)) {
				pView->dwFlashTime[i] = dwNow ? dwNow : 1;
			}
		}
		if (pPara->byKind == PINBALLSTATE_OVER) {
			pView->dwOverTime = dwNow ? dwNow : 1;
			pView->bSim = FALSE;
			pView->nPred = 0;
			pView->bCharging = FALSE;
		} else {
			pView->dwOverTime = 0;
			if (Predictable (pView)) {
				Rebase (pView, dwNow);
			} else {
				pView->bSim = FALSE;
				pView->nPred = 0;
				pView->bCharging = FALSE;
			}
			/* 観戦用の履歴 */
			if (pView->nHist >= PINBALL_HIST_MAX) {
				memmove (&pView->aHist[0], &pView->aHist[1], sizeof (PBSAMPLE) * (PINBALL_HIST_MAX - 1));
				pView->nHist = PINBALL_HIST_MAX - 1;
			}
			pView->aHist[pView->nHist].dwTime = dwNow;
			pView->aHist[pView->nHist].nX = pPara->nBallX;
			pView->aHist[pView->nHist].nY = pPara->nBallY;
			pView->aHist[pView->nHist].byPhase = pPara->byPhase;
			pView->aHist[pView->nHist].byAngL = pPara->byAngL;
			pView->aHist[pView->nHist].byAngR = pPara->byAngR;
			pView->nHist ++;
		}
		break;
	default:
		return;
	}
	g_bDraw = TRUE;
}

/* 席に座って遊んでいるか。ゲームオーバーの直後も PINBALL_OVER_LOCK_MS の間は座っているのと同じ扱い */
BOOL IsPinballSeated(void)
{
	int i;

	if (g_byGameState != GAMESTATE_MOVE) {
		return FALSE;
	}
	for (i = 0; i < PINBALL_VIEW_MAX; i ++) {
		if (s_aView[i].bUse && s_aView[i].St.byMine &&
			((s_aView[i].dwOverTime == 0) || ((int)(timeGetTime () - s_aView[i].dwOverTime) < PINBALL_OVER_LOCK_MS))) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 本人の台 */
static PBVIEW *MyView(void)
{
	int i;

	for (i = 0; i < PINBALL_VIEW_MAX; i ++) {
		if (s_aView[i].bUse && s_aView[i].St.byMine) {
			return &s_aView[i];
		}
	}
	return NULL;
}

/* キーを送って、先読みにもかける */
static void SendKey(PBVIEW *pView, BYTE byKey)
{
	PBPRED P;
	DWORD dwNow = timeGetTime ();

	ZeroMemory (&P, sizeof (P));
	P.dwTime = dwNow;
	P.byKey = byKey;
	if ((byKey == PINBALLKEY_LAUNCH_UP) && pView->bCharging) {
		P.nHoldMs = (int)(dwNow - pView->dwChargeStart);
	}
	P.dwSerial = SendCmdPINBALLKEY (byKey);
	if (byKey == PINBALLKEY_LEAVE) {
		return;
	}
	if (byKey == PINBALLKEY_LAUNCH_DOWN) {
		/* 溜め始め（打ち出しレーンで待っている時だけ。サーバーへは常に送る） */
		if (pView->bSim && (pView->Sim.nPhase == PB_PHASE_READY)) {
			pView->bCharging = TRUE;
			pView->dwChargeStart = dwNow;
		}
	}
	if (pView->bSim && Predictable (pView)) {
		SimAdvance (pView, dwNow);
		if (byKey == PINBALLKEY_LAUNCH_UP) {
			if (!pView->bCharging || (pView->Sim.nPhase != PB_PHASE_READY)) {
				pView->bCharging = FALSE;
				return;		/* 打ち出さない（溜めていない・レーンに球がない）。サーバーも同じ判断 */
			}
			pView->bCharging = FALSE;
		}
		SimApply (&pView->Sim, P);
		if (pView->nPred >= PINBALL_PRED_MAX) {
			memmove (&pView->aPred[0], &pView->aPred[1], sizeof (PBPRED) * (PINBALL_PRED_MAX - 1));
			pView->nPred = PINBALL_PRED_MAX - 1;
		}
		pView->aPred[pView->nPred ++] = P;
	}
	g_bDraw = TRUE;
}

/* 座っている間のキー処理。左右（と X・Z）＝フリッパー、下＝打ち出し、ESC＝席を立つ。押した時・離した時だけ送る */
void PinballKeyPoll(const BYTE *abyKeyState)
{
	static BOOL s_bL, s_bR, s_bD, s_bEsc;
	PBVIEW *pView = MyView ();
	BOOL bL, bR, bD, bEsc;

	/* 座っていなければ、押している状態を忘れる（次に座った時に押しっぱなしでも押した扱いにする） */
	if (pView == NULL) {
		s_bL = s_bR = s_bD = s_bEsc = FALSE;
		return;
	}
	bL = ((abyKeyState[VK_LEFT] & 0x80) || (abyKeyState['Z'] & 0x80)) ? TRUE : FALSE;
	bR = ((abyKeyState[VK_RIGHT] & 0x80) || (abyKeyState['X'] & 0x80)) ? TRUE : FALSE;
	bD = (abyKeyState[VK_DOWN] & 0x80) ? TRUE : FALSE;
	bEsc = (abyKeyState[VK_ESCAPE] & 0x80) ? TRUE : FALSE;
	/* ゲームオーバー後の少しの間（座っている扱いのまま）は何も送らない */
	if (pView->dwOverTime != 0) {
		s_bL = bL; s_bR = bR; s_bD = bD; s_bEsc = bEsc;
		return;
	}
	if (bEsc && !s_bEsc) {
		SendKey (pView, PINBALLKEY_LEAVE);
	}
	if (bL != s_bL) {
		SendKey (pView, bL ? PINBALLKEY_LEFT_DOWN : PINBALLKEY_LEFT_UP);
	}
	if (bR != s_bR) {
		SendKey (pView, bR ? PINBALLKEY_RIGHT_DOWN : PINBALLKEY_RIGHT_UP);
	}
	if (bD != s_bD) {
		SendKey (pView, bD ? PINBALLKEY_LAUNCH_DOWN : PINBALLKEY_LAUNCH_UP);
	}
	s_bL = bL; s_bR = bR; s_bD = bD; s_bEsc = bEsc;
}

/* 観戦：描画を遅らせた時刻のボール位置を補間する */
static void Interpolate(PBVIEW *pView, DWORD dwNow, PPINBALLDRAW pOut)
{
	DWORD dwRt = dwNow - PINBALL_INTERP_MS;
	int i, nOld = -1;

	pOut->bBallShow = FALSE;
	pOut->nAngL = pView->St.byAngL;
	pOut->nAngR = pView->St.byAngR;
	if (pView->nHist == 0) {
		return;
	}
	/* dwRt 以前で一番新しい標本 */
	for (i = 0; i < pView->nHist; i ++) {
		if ((int)(pView->aHist[i].dwTime - dwRt) <= 0) {
			nOld = i;
		}
	}
	if (nOld < 0) {
		nOld = 0;		/* まだ遅らせた時刻に届いていない：一番古い標本のまま */
	}
	pOut->nAngL = pView->aHist[nOld].byAngL;
	pOut->nAngR = pView->aHist[nOld].byAngR;
	pOut->nBallX = pView->aHist[nOld].nX;
	pOut->nBallY = pView->aHist[nOld].nY;
	pOut->bBallShow = (pView->aHist[nOld].byPhase == PB_PHASE_PLAY) || (pView->aHist[nOld].byPhase == PB_PHASE_READY);
	if ((nOld + 1 < pView->nHist) && (pView->aHist[nOld].byPhase == PB_PHASE_PLAY) && (pView->aHist[nOld + 1].byPhase == PB_PHASE_PLAY)) {
		const PBSAMPLE &A = pView->aHist[nOld], &B = pView->aHist[nOld + 1];
		int nSpan = (int)(B.dwTime - A.dwTime), nT = (int)(dwRt - A.dwTime);
		int dx = B.nX - A.nX, dy = B.nY - A.nY;
		/* 近い時だけ補間（壁で跳ねた・飛んだ時は新しい方へ） */
		if ((nSpan > 0) && (nT > 0) && (abs (dx) < 40 * PB_FP) && (abs (dy) < 40 * PB_FP)) {
			if (nT > nSpan) nT = nSpan;
			pOut->nBallX = A.nX + (int)((long long)dx * nT / nSpan);
			pOut->nBallY = A.nY + (int)((long long)dy * nT / nSpan);
		}
	}
	g_bDraw = TRUE;	/* 補間のため、見えている間は描き続ける */
}

/* 窓の縦位置：ボールを窓の中央に置く位置へ、滑らかに寄る。bFollow が FALSE（待機中・打ち出し前・穴に落ちて返事待ち）なら下端
   ボールの位置は、本人は先読み・観戦者は補間した位置（pOut->nBallY）なので、同じ式で決まる */
static void CamUpdate(PBVIEW *pView, PPINBALLDRAW pOut, DWORD dwNow, BOOL bFollow)
{
	const int nBottom = PB_YOFS * PB_FP;
	int nTarget = nBottom;

	if (bFollow) {
		nTarget = pOut->nBallY - PB_VIEW_H * PB_FP / 2;
		if (nTarget < 0) nTarget = 0;
		if (nTarget > nBottom) nTarget = nBottom;
	}
	if (!pView->bCam) {
		pView->nCamFp = nTarget;
		pView->bCam = TRUE;
	} else {
		int nDt = (int)(dwNow - pView->dwCamTime);
		if (nDt < 0) nDt = 0;
		if (nDt > 200) nDt = 200;
		pView->nCamFp += (int)((long long)(nTarget - pView->nCamFp) * nDt / (nDt + PINBALL_CAM_MS));
	}
	pView->dwCamTime = dwNow;
	if (abs (pView->nCamFp - nTarget) > PB_FP / 2) {
		g_bDraw = TRUE;		/* 寄っている間は描き続ける */
	}
	pOut->nCamY = (pView->nCamFp + PB_FP / 2) / PB_FP;
	if (pOut->nCamY < 0) pOut->nCamY = 0;
	if (pOut->nCamY > PB_YOFS) pOut->nCamY = PB_YOFS;
}

/* 差し替え絵を読む（初めて描く時に 1 回だけ。リソースが無い・形式違い・大きさ違いなら図形で描く） */
extern void ReadGrpData2(LPSTR pszName, CImg32 **pDib, int nSize);	/* SboCliMain.cpp */

BOOL GetPinballArt(CImg32 **ppBg, CImg32 **ppParts)
{
	static int s_nState = 0;				/* 0=未読み込み・1=使える・2=使わない */
	static CImg32 *s_pBg = NULL, *s_pParts = NULL;

	if (s_nState == 0) {
		s_nState = 2;
		ReadGrpData2 ("IDP_PINBALL_BG", &s_pBg, 1);
		ReadGrpData2 ("IDP_PINBALL_PARTS", &s_pParts, 1);
		if (s_pBg && s_pParts && (s_pBg->Width () == PB_W) && (s_pBg->Height () == PB_H) &&
			(s_pParts->Width () >= PBART_PARTS_W) && (s_pParts->Height () >= PBART_PARTS_H)) {
			s_nState = 1;
		}
	}
	if (s_nState != 1) {
		return FALSE;
	}
	*ppBg = s_pBg;
	*ppParts = s_pParts;
	return TRUE;
}

BOOL GetPinballView(int nIdx, PPINBALLDRAW pOut)
{
	PBVIEW *pView;
	DWORD dwNow = timeGetTime ();
	int i;
	BOOL bPlay = FALSE;

	if ((nIdx < 0) || (nIdx >= PINBALL_VIEW_MAX) || (g_byGameState != GAMESTATE_MOVE)) {
		return FALSE;
	}
	pView = &s_aView[nIdx];
	if (!pView->bUse) {
		return FALSE;
	}
	/* 返事の来ない先読みのキーは捨てる（サーバーが取り合わなかったキー） */
	if (pView->nPred > 0) {
		while ((pView->nPred > 0) && ((int)(dwNow - pView->aPred[0].dwTime) >= PINBALL_PRED_DROP_MS)) {
			memmove (&pView->aPred[0], &pView->aPred[1], sizeof (PBPRED) * (pView->nPred - 1));
			pView->nPred --;
		}
	}
	/* 本人のゲームオーバーの表示を終える（ほかの人と同じ待機中の台にする） */
	if (pView->dwOverTime != 0) {
		if ((int)(dwNow - pView->dwOverTime) >= PINBALL_OVER_SELF_MS) {
			pView->dwOverTime = 0;
			pView->St.byMine = 0;
			pView->St.byIdle = PINBALLIDLE_ON;
			pView->St.byOver = 0;
		} else {
			g_bDraw = TRUE;
		}
	}
	ZeroMemory (pOut, sizeof (PINBALLDRAW));
	pOut->St = pView->St;
	pOut->nTargetMask = pView->St.byTargets;
	pOut->bOver = (pView->dwOverTime != 0) || (pView->St.byOver != 0);
	for (i = 0; i < PB_BUMPERS + PB_TARGETS; i ++) {
		pOut->dwFlashAge[i] = pView->dwFlashTime[i] ? (DWORD)(dwNow - pView->dwFlashTime[i]) : 0xFFFFFFFF;
		if (pOut->dwFlashAge[i] < PINBALL_FLASH_MS) {
			g_bDraw = TRUE;
		}
	}
	if (pView->St.byIdle || (pView->dwOverTime != 0)) {
		pOut->bBallShow = FALSE;
		if (pView->St.byIdle) {
			pView->bCam = FALSE;		/* 待機中の台は下端を映す（次に遊ぶ時も下端から） */
		}
		CamUpdate (pView, pOut, dwNow, FALSE);
		return TRUE;
	}
	if (Predictable (pView) && pView->bSim) {
		SimAdvance (pView, dwNow);
		pOut->nBallX = pView->Sim.nX + pView->nOffX;
		pOut->nBallY = pView->Sim.nY + pView->nOffY;
		pOut->bBallShow = (pView->Sim.nPhase == PB_PHASE_PLAY) || (pView->Sim.nPhase == PB_PHASE_READY);
		bPlay = (pView->Sim.nPhase == PB_PHASE_PLAY);
		pOut->nAngL = pView->Sim.nAngL;
		pOut->nAngR = pView->Sim.nAngR;
		pOut->nTargetMask = pView->Sim.nTargetMask;
		pOut->nCharge = pView->bCharging ? PinballGame::ChargePercent ((int)(dwNow - pView->dwChargeStart)) : 0;
		g_bDraw = TRUE;		/* 先読みの動きを描き続ける */
	} else {
		Interpolate (pView, dwNow, pOut);
		pOut->nCharge = (pView->St.byHeld & 4) ? pView->St.byCharge : 0;
		bPlay = (pView->St.byPhase == PB_PHASE_PLAY);
	}
	CamUpdate (pView, pOut, dwNow, pOut->bBallShow && bPlay);
	return TRUE;
}
