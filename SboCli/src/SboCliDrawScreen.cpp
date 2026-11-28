/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboCliDrawScreen.cpp										 */
/* 内容：		画面表示													 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#include "stdafx.h"
#include "common/BowFlight.h"
#include "common/FallingPuzzle.h"
#include "common/Pinball.h"
#include "SboCliPinball.h"
#include "common/Janjan.h"
#include "common/MazeAttack.h"
#include "common/MazeTable.h"
#include "Glbldata.h"
#include "SboCliDrawWindow.h"
#include "SboCliRecvCmd.h"
#include "SboCliKeyProc.h"
#include "SboCliAutoMap.h"
#include "AdminWindowOld/AdminDlgNpcRoute.h"
#include "AdminWindowOld/AdminDlgMapLight.h"
#include "AdminWindowOld/AdminDlgMapSwap.h"
#include <math.h>

/* ========================================================================= */
/* 描画の基準点（カメラ）。通常は自分のキャラのマス＋歩きずれ。迷路対戦の席に座っている間は盤の中央・ずれ 0	 */
/* 実体は SboCliRecvCmd.cpp の GetCameraBase。地図・キャラ・NPC・盤・天気の描画は全部これを基準にする		 */
/* ========================================================================= */
static int CamTileX(void) { int x, y, ox, oy; GetCameraBase (&x, &y, &ox, &oy); return x; }
static int CamTileY(void) { int x, y, ox, oy; GetCameraBase (&x, &y, &ox, &oy); return y; }
static int CamOfsX(void)  { int x, y, ox, oy; GetCameraBase (&x, &y, &ox, &oy); return ox; }
static int CamOfsY(void)  { int x, y, ox, oy; GetCameraBase (&x, &y, &ox, &oy); return oy; }

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */

void TextOut2			(HDC, int, int, char *, COLORREF);			/* 黒縁取りで文字描画 */
void DrawScreen			(HWND);										/* 画面の描画 */
void DrawScreenLOGO		(void);										/* ロゴ画面の描画 */
void DrawScreenTITLE	(void);										/* タイトル画面の描画 */
void DrawScreenLOGIN	(void);										/* ログイン画面の描画 */
BOOL DrawScreenMOVE		(void);										/* 歩き中画面の描画 */
void DrawMapLine		(int, int);									/* 指定ラインの背景描画 */
void DrawCharLine		(int);										/* 指定ラインのキャラ描画 */
void DrawChar2			(void);										/* キャラのエフェクトの描画 */
void DrawChar3			(void);										/* キャラの名前等の描画 */
static void DrawDropEffects	(void);									/* アイテムドロップ演出の描画 */
void DrawETC			(void);										/* ゲージ等の描画 */
static BOOL IsHideNpcPut	(PCHARINFO pChar);						/* NPC発生を描かない？ */
static void DrawFrameRect	(int x, int y, int cx, int cy, COLORREF cl);	/* 1ドットの枠を描く */
static void DrawGuardMark	(int x, int y);								/* 構えの盾マーク(system.png) */
static void DrawBuffIcons	(int x, int y, DWORD dwTblID);				/* 攻撃力・守備力アップのアイコン(system.png) */
void DrawObj			(void);										/* オブジェクトの描画 */
void DrawEvent			(void);										/* イベント情報の描画 */
void DrawMapEdit		(void);										/* マップ編集情報の描画 */
void DrawNpcRoute		(void);										/* NPCの巡回情報の描画 */
void DrawMapSwapEdit	(void);										/* 印で変わるマス・宝箱の編集中の枠 */
void DrawMapLightEdit	(void);										/* 明るさの範囲・光源の編集中の印 */
static void FillRectClip	(int x, int y, int cx, int cy, COLORREF cl);	/* 画像の外にはみ出さない矩形塗りつぶし */
static void DrawLineClip	(int x1, int y1, int x2, int y2, int nWidth, COLORREF cl);	/* 線を描く */
void DrawWeather		(void);										/* 天候の描画 */
static void DrawMapLight	(const char *pMapDark);						/* 丸くゆらめく灯りの描画 */
static void DrawMapViewFog	(void);										/* 見える範囲の外に白い霧をかける */
static void DrawCharLightGlow	(void);									/* 明かりを使っている人の体をぼんやり光らせる */

static BOOL s_bDrawCharBridgeUpper = FALSE;							/* TRUE なら橋の上を渡っている人だけ描く */
static BYTE s_abyBridgeCol[DRAW_PARTS_X * 2 + 3];						/* 橋を重ねる列(NULL でない時だけ見る) */
static BYTE *s_pbyBridgeCol = NULL;										/* 橋を重ねる列を絞る時は s_abyBridgeCol */

/* 行 nLine で橋の下をくぐっている人の頭が上の行にはみ出すので、
   その人の列(歩いている途中は左右の列も)だけ、上の行の橋を重ねる */
static void DrawBridgeOverHead(int nLine)
{
	int i, x, nCount, nXMax;
	BOOL bAny = FALSE;
	PCHARINFO pChar, pMyChar;

	if ((nLine <= 0) || (g_nMyCharID < 0)) {
		return;
	}
	pMyChar = g_aCharInfo[g_nMyCharID];
	nXMax = DRAW_PARTS_X * 2 + 3;
	ZeroMemory (s_abyBridgeCol, sizeof (s_abyBridgeCol));
	nCount = g_aCharPos[nLine].GetSize ();
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[g_aCharPos[nLine][i]];
		if (GetCharBridgeLayer (pChar) != 2) {
			continue;
		}
		x = (int)pChar->dwX - CamTileX () + (DRAW_PARTS_X + 1);
		for (int k = x - 1; k <= x + 1; k ++) {
			if ((k >= 0) && (k < nXMax)) {
				s_abyBridgeCol[k] = 1;
				bAny = TRUE;
			}
		}
	}
	if (bAny == FALSE) {
		return;
	}
	s_pbyBridgeCol = s_abyBridgeCol;
	DrawMapLine (nLine - 1, PARTSHIT_BRIDGEV);
	s_pbyBridgeCol = NULL;
}
void AddSysMsg			(LPSTR, COLORREF);							/* システムメッセージの追加 */

/* ========================================================================= */
/* 関数名：TextOut2															 */
/* 内容：黒縁取りで文字描画													 */
/* 日付：2002/07/15															 */
/* ========================================================================= */
void TextOut2(HDC hdc, int iX, int iY, char *pStr, COLORREF Color)
{
	/* 縁取りの 4 回と本体の 1 回を、文字の形を 1 回だけ調べてまとめて描く */
	/* ウィンドウの中ならHPの減り具合で白い文字の色を変える */
	Color = HpWarnTextColor (Color);
	TextOutOutline (hdc, iX, iY, pStr, strlen (pStr), RGB(10, 10, 10), Color);
}

/* ========================================================================= */
/* 関数名：GetSysMsgBottomOfs												 */
/* 内容：戦闘ログの流れ始める位置と消える位置を上へ持ち上げる量				 */
/*		 画面の下に特技ショートカットの帯が出ている間は、ログが帯に重ならない	 */
/*		 ように、帯のぶん（帯の高さ21＋文字の高さ16）だけ上にずらす			 */
/* ========================================================================= */
int GetSysMsgBottomOfs(void)
{
	PCHARINFO pChar;

	if (g_nMyCharID < 0 || g_nMyCharID >= g_aCharInfo.GetSize ()) {
		return 0;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	if (pChar == NULL || !IsMagicBandShown (pChar)) {
		return 0;
	}
	return 37;
}

/* ========================================================================= */
/* 関数名：DrawSysMsgBand													 */
/* 内容：戦闘ログ（システムメッセージ）の後ろに半透明の暗い帯を敷く			 */
/*		 草原など明るい背景でも細い文字が読めるように。右端は 1 文字ぶんぼかす */
/*		 行が重なっても濃くならないよう、1 ドット行ごとに 1 回だけ塗る		 */
/* ========================================================================= */
static void DrawSysMsgBand(HDC hDC, int nX, int nY)
{
	const int nFade = 14;		/* 右端をぼかす幅（1 文字ぶん） */
	const int nAlpha = 120;		/* 帯の濃さ（0〜255） */
	int i, yy, nCount = g_SysMsgInfo.GetSize ();
	int nOfs = GetSysMsgBottomOfs ();
	int anRight[SCRSIZEY + 2];
	TEXTMETRIC tm;
	SIZE size;

	if (nCount <= 0) {
		return;
	}
	GetTextMetrics (hDC, &tm);
	for (yy = 0; yy < SCRSIZEY + 2; yy ++) {
		anRight[yy] = 0;
	}
	for (i = 0; i < nCount; i ++) {
		PSYSMSGINFO pSysMsgInfo = g_SysMsgInfo[i];
		if (pSysMsgInfo->nY < SYSMSG_EY - nOfs) {
			continue;
		}
		GetTextExtentPoint32 (hDC, pSysMsgInfo->pszMsg, strlen (pSysMsgInfo->pszMsg), &size);
		/* 縁取りの 1 ドットと上下 1 ドットの余白も含めて帯にする（行の間にすき間が出ないように） */
		for (yy = pSysMsgInfo->nY - 2; yy <= pSysMsgInfo->nY + tm.tmHeight + 1; yy ++) {
			if (yy < 0 || yy >= SCRSIZEY + 2) {
				continue;
			}
			anRight[yy] = max (anRight[yy], size.cx + 2);
		}
	}
	for (yy = 0; yy < SCRSIZEY + 2; yy ++) {
		if (anRight[yy] > 0) {
			W32ShadeSpan (hDC, nX, nY + yy, anRight[yy], nFade, RGB (0, 0, 0), nAlpha);
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawMagicKeyBand													 */
/* 内容：戦闘モード中（装備品・道具の枠があれば戦闘外でも）、画面の下に		 */
/*		 ショートカットの帯を出す											 */
/*		 「1 炎の 2 回復 …」（番号と特技名の頭2文字）。空きは「-」			 */
/*		 色は攻撃(赤系)・回復(緑系)・それ以外の補助(青系)。Mana不足・詠唱中・	 */
/*		 覚えていない特技は暗くする。後ろに薄い暗い帯を敷き、マップを隠しすぎない */
/*		 ウィンドウより下になるよう、ウィンドウを描く前に呼ぶ				 */
/* ========================================================================= */
static void DrawMagicKeyBand(PCHARINFO pChar, int nX, int nY)
{
	const int nFade = 14;		/* 右端をぼかす幅 */
	const int nAlpha = 120;		/* 帯の濃さ（0〜255） */
	const int nGap = 10;		/* 枠と枠の間 */
	const int nLeft = 4;
	int i, yy, x, nWidth = 0;
	int anWidth[MAX_MAGICKEY];
	char aszText[MAX_MAGICKEY][32];
	COLORREF aClr[MAX_MAGICKEY];
	HDC hDC;
	HFONT hFontOld;
	SIZE size;

	/* 出す条件はスマホのボタン（SboWebGetMagicKeyState）と同じ関数で決める */
	if (!IsMagicBandShown (pChar)) {
		return;
	}

	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
	SetBkMode (hDC, TRANSPARENT);

	for (i = 0; i < MAX_MAGICKEY; i ++) {
		int nR = 150, nG = 150, nB = 150;
		char szName[16];
		int nKind;
		BOOL bDark;

		if (!GetMagicKeySlotLabel (pChar, i, szName, sizeof (szName), &nKind, &bDark)) {
			wsprintf (aszText[i], "%d -", i + 1);
		} else {
			wsprintf (aszText[i], "%d %s", i + 1, szName);
			switch (nKind) {
			case 0:		nR = 255;	nG = 120;	nB = 110;	break;		/* 攻撃 */
			case 1:		nR = 120;	nG = 230;	nB = 130;	break;		/* 回復 */
			case 3:		nR = 205;	nG = 150;	nB = 255;	break;		/* 装備品 */
			case 4:		nR = 240;	nG = 220;	nB = 100;	break;		/* 道具 */
			default:	nR = 130;	nG = 180;	nB = 255;	break;		/* 補助 */
			}
		}
		if (bDark) {
			nR = nR * 45 / 100;
			nG = nG * 45 / 100;
			nB = nB * 45 / 100;
		}
		aClr[i] = RGB (nR, nG, nB);
		GetTextExtentPoint32 (hDC, aszText[i], strlen (aszText[i]), &size);
		anWidth[i] = size.cx;
		nWidth += size.cx + ((i > 0) ? nGap : 0);
	}

	/* 後ろの帯（上下 1 ドットの余白つき。右端はぼかす） */
	for (yy = SCRSIZEY - 21; yy < SCRSIZEY; yy ++) {
		W32ShadeSpan (hDC, nX, nY + yy, nLeft + nWidth + 4, nFade, RGB (0, 0, 0), nAlpha);
	}
	x = nLeft;
	for (i = 0; i < MAX_MAGICKEY; i ++) {
		TextOut2 (hDC, nX + x, nY + SCRSIZEY - 19, aszText[i], aClr[i]);
		x += anWidth[i] + nGap;
	}

	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

/* ========================================================================= */
/* 拠点防衛の状況表示（上の中央のパネル・中央の大見出し・拠点が危ない時の赤い縁）	 */
/*		 サーバーから届く状態（CMD_DGDEFSTATUS）の変わり目から、クライアントだけで演出を出す */
/*		 H キーで全部まとめて隠す・出す（Tab は表示切り替えで使っているので使えない）	 */
/* ========================================================================= */
#define DEFHUD_BANNER_WAVE		(2500)		/* 「WAVE n/N」の大見出しを出す時間(ms) */
#define DEFHUD_BANNER_RESULT	(4000)		/* 勝ち・負けの大見出しを出す時間(ms) */
#define DEFHUD_BANNER_OPEN		(220)		/* 大見出しがスッと開く時間(ms) */
#define DEFHUD_BANNER_CLOSE		(300)		/* 大見出しがスッと閉じる時間(ms) */

enum {
	DEFHUD_BANNER_NONE = 0,
	DEFHUD_BANNER_KIND_WAVE,
	DEFHUD_BANNER_KIND_WIN,
	DEFHUD_BANNER_KIND_LOSE,
};

static BOOL		s_bDefHudShow = TRUE;			/* 状況表示を出すか（Tab の表示切替が「なし」の間だけ隠す。毎フレーム決める） */
static BYTE		s_byDefPrevState = DGDEFSTATUS_NONE;	/* 前に見た状態 */
static BYTE		s_byDefPrevWave = 0;			/* 前に見た波の番号 */
static BYTE		s_byDefBanner = DEFHUD_BANNER_NONE;	/* 出している大見出しの種類 */
static BYTE		s_byDefBannerWave = 0;			/* 大見出しの波の番号 */
static BYTE		s_byDefBannerWaveMax = 0;		/* 大見出しの波の数 */
static DWORD	s_dwDefBannerStart = 0;			/* 大見出しを出し始めた時刻 */

/* 拠点のHPの割合（0〜100。残っていれば最低 1） */
static int DefenseHpPercent(const CMDPARA_DGDEFSTATUS *pSt)
{
	int nMax = max ((int)pSt->wBaseMaxHP, 1);
	int nHP = min ((int)pSt->wBaseHP, nMax);
	int nPct = nHP * 100 / nMax;

	if (nHP > 0 && nPct == 0) {
		nPct = 1;
	}
	return nPct;
}

/* 拠点が危ない（HPが25%以下で、まだ続いている）か */
static BOOL IsDefenseDanger(const CMDPARA_DGDEFSTATUS *pSt)
{
	return ((pSt->byState == DGDEFSTATUS_PREP) || (pSt->byState == DGDEFSTATUS_WAVE)) &&
		(pSt->wBaseHP > 0) && (DefenseHpPercent (pSt) <= 25);
}

/* 状態の変わり目を見て、大見出しを出し始める（毎フレーム 1 回。隠している間も見ておく） */
static void DefenseHudWatch(BOOL bActive, const CMDPARA_DGDEFSTATUS *pSt)
{
	DWORD dwNow = timeGetTime ();

	if (!bActive) {
		/* 拠点防衛の外：全部元に戻す */
		s_byDefPrevState = DGDEFSTATUS_NONE;
		s_byDefPrevWave = 0;
		s_byDefBanner = DEFHUD_BANNER_NONE;
		s_bDefHudShow = TRUE;
		return;
	}
	s_bDefHudShow = (g_byViewMode != 0);
	/* 入った直後（前の状態が無い）は、途中から見た時に古い演出を出さないよう見るだけ */
	if (s_byDefPrevState != DGDEFSTATUS_NONE) {
		if ((pSt->byState == DGDEFSTATUS_WAVE) && ((s_byDefPrevState != DGDEFSTATUS_WAVE) || (pSt->byWave > s_byDefPrevWave))) {
			s_byDefBanner = DEFHUD_BANNER_KIND_WAVE;
			s_byDefBannerWave = pSt->byWave;
			s_byDefBannerWaveMax = pSt->byWaveMax;
			s_dwDefBannerStart = dwNow;
		} else if ((pSt->byState == DGDEFSTATUS_WIN) && (s_byDefPrevState != DGDEFSTATUS_WIN)) {
			s_byDefBanner = DEFHUD_BANNER_KIND_WIN;
			s_dwDefBannerStart = dwNow;
		} else if ((pSt->byState == DGDEFSTATUS_LOSE) && (s_byDefPrevState != DGDEFSTATUS_LOSE)) {
			s_byDefBanner = DEFHUD_BANNER_KIND_LOSE;
			s_dwDefBannerStart = dwNow;
		}
	}
	s_byDefPrevState = pSt->byState;
	s_byDefPrevWave = pSt->byWave;
}

/* ========================================================================= */
/* 関数名：DrawDefenseDanger													 */
/* 内容：拠点のHPが25%以下の間、画面の縁をゆっくり赤く点滅させる（強すぎない程度）	 */
/*		 ウィンドウの下になるよう、ウィンドウを描く前に呼ぶ					 */
/* ========================================================================= */
static void DrawDefenseDanger(int nX, int nY)
{
	const int nWidth = 26;		/* 縁の幅 */
	CMDPARA_DGDEFSTATUS St;
	HDC hDC;
	DWORD dwNow = timeGetTime ();
	int yy, d, nAlpha;
	double dRate;

	if (!s_bDefHudShow || !GetDefenseStatus (&St) || !IsDefenseDanger (&St)) {
		return;
	}
	/* 2.4 秒で 1 回ゆっくり明るくなって戻る */
	dRate = 0.5 + 0.5 * sin ((double)(dwNow % 2400) * 6.2831853 / 2400.0);
	nAlpha = 35 + (int)(75.0 * dRate);

	hDC = g_pDibDrawMap->Lock ();
	for (yy = 0; yy < SCRSIZEY; yy ++) {
		/* 上下の縁（行ごとに全幅を薄く） */
		if (yy < nWidth) {
			W32ShadeSpan (hDC, nX, nY + yy, SCRSIZEX, 0, RGB (220, 20, 20), nAlpha * (nWidth - yy) / nWidth);
		} else if (yy >= SCRSIZEY - nWidth) {
			W32ShadeSpan (hDC, nX, nY + yy, SCRSIZEX, 0, RGB (220, 20, 20), nAlpha * (yy - (SCRSIZEY - nWidth) + 1) / nWidth);
		}
		/* 左の縁（右へ向かって薄くなる） */
		W32ShadeSpan (hDC, nX, nY + yy, 0, nWidth, RGB (220, 20, 20), nAlpha);
		/* 右の縁（2 ドットずつ） */
		for (d = 0; d < nWidth; d += 2) {
			W32ShadeSpan (hDC, nX + SCRSIZEX - d - 2, nY + yy, 2, 0, RGB (220, 20, 20), nAlpha * (nWidth - d) / nWidth);
		}
	}
	g_pDibDrawMap->Unlock ();
	g_bDraw = TRUE;
}

/* 枠のある帯を描く（半透明の暗い帯＋1 ドットの枠） */
static void DrawDefenseFrame(HDC hDC, int x, int y, int cx, int cy, COLORREF clFrame, int nAlpha)
{
	int yy;

	for (yy = 0; yy < cy; yy ++) {
		if ((yy == 0) || (yy == cy - 1)) {
			W32ShadeSpan (hDC, x + 1, y + yy, cx - 2, 0, clFrame, 255);
		} else {
			W32ShadeSpan (hDC, x, y + yy, cx, 0, RGB (0, 0, 0), nAlpha);
			W32ShadeSpan (hDC, x, y + yy, 1, 0, clFrame, 255);
			W32ShadeSpan (hDC, x + cx - 1, y + yy, 1, 0, clFrame, 255);
		}
	}
}

/* 真ん中揃えで文字を描く */
static void DrawDefenseTextCenter(HDC hDC, int xCenter, int y, LPSTR pszText, COLORREF cl)
{
	SIZE size;

	GetTextExtentPoint32 (hDC, pszText, strlen (pszText), &size);
	TextOut2 (hDC, xCenter - size.cx / 2, y, pszText, cl);
}

/* 塗りつぶしの丸 */
static void DrawDefenseDot(HDC hDC, int cx, int cy, int r, COLORREF cl)
{
	int yy;

	for (yy = -r; yy <= r; yy ++) {
		int w = (int)(sqrt ((double)(r * r - yy * yy) + 0.5));
		W32ShadeSpan (hDC, cx - w, cy + yy, w * 2 + 1, 0, cl, 255);
	}
}

/* ========================================================================= */
/* 関数名：DrawDungeonPanel													 */
/* 内容：番外編ダンジョン（拠点防衛以外）の挑戦中だけ、画面の左上に小さなパネルを出す	 */
/*		 1行目 題（テーマ名）／2行目 残り敵 n/N（敵を数えるモードだけ）／3行目 経過 m:ss／4行目 目標	 */
/*		 拠点防衛の帯と同じ作り（半透明の暗い帯・枠・影つきの文字）。Tab の表示切替が「なし」の間は隠す	 */
/* ========================================================================= */
static int s_nDgPanelSec = -1;		/* 最後に描いた経過秒 */
static void DrawDungeonPanel(int nX, int nY)
{
	const int nWidth = 190;		/* パネルの幅 */
	const int nLine = 18;		/* 1 行の高さ */
	CMDPARA_DGSTATUS St;
	char szTmp[96];
	HDC hDC;
	HFONT hFontOld;
	BOOL bEnemy;
	int nRows, nHeight, x0, y0, yy, nSec;

	if ((g_byViewMode == 0) || !GetDungeonStatus (&St)) {
		return;
	}
	bEnemy = (St.byGoal == DGSTATUS_GOAL_KILLALL) && (St.wEnemyMax > 0);
	nRows = bEnemy ? 4 : 3;
	nHeight = nRows * nLine + 8;
	x0 = nX + 4;
	y0 = nY + 4;
	nSec = (int)min (St.dwElapsedSec, (DWORD)(99 * 3600 + 59 * 60 + 59));

	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
	SetBkMode (hDC, TRANSPARENT);
	DrawDefenseFrame (hDC, x0, y0, nWidth, nHeight, RGB (200, 200, 200), 150);
	yy = y0 + 5;
	TextOut2 (hDC, x0 + 8, yy, St.szTitle, RGB (255, 235, 120));
	yy += nLine;
	if (bEnemy) {
		wsprintf (szTmp, "残り敵 %d/%d", (int)St.wEnemy, (int)St.wEnemyMax);
		TextOut2 (hDC, x0 + 8, yy, szTmp, RGB (255, 255, 255));
		yy += nLine;
	}
	if (nSec >= 3600) {
		wsprintf (szTmp, "経過 %d:%02d:%02d", nSec / 3600, (nSec / 60) % 60, nSec % 60);
	} else {
		wsprintf (szTmp, "経過 %d:%02d", nSec / 60, nSec % 60);
	}
	TextOut2 (hDC, x0 + 8, yy, szTmp, RGB (255, 255, 255));
	yy += nLine;
	if (St.byState == DGSTATUS_CLEAR) {
		TextOut2 (hDC, x0 + 8, yy, (LPSTR)"目標: クリア！", RGB (140, 255, 150));
	} else if (St.byGoal == DGSTATUS_GOAL_KILLALL) {
		TextOut2 (hDC, x0 + 8, yy, (LPSTR)"目標: 全部倒せ", RGB (255, 255, 255));
	} else {
		TextOut2 (hDC, x0 + 8, yy, (LPSTR)"目標: 奥の扉", RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
	/* 経過秒が進むので、秒が変わったら次も描き直す（毎フレームは描かない） */
	if ((St.byState == DGSTATUS_RUN) && (nSec != s_nDgPanelSec)) {
		g_bDraw = TRUE;
	}
	s_nDgPanelSec = nSec;
}

/* ========================================================================= */
/* 関数名：DrawPuzzle														 */
/* 内容：落ち物パズル（見えている台すべて＝自分の台・対戦相手の台・近くの台。				 */
/*		 ゲームオーバー後の少しの間も）の重ね描き								 */
/*		 枠・盤面・操作中の落ち物・次の落ち物・得点を、地図の上に描く。			 */
/*		 フィールドの位置はサーバーが台NPCの座標から決めて（マップのドット座標）	 */
/*		 送ってくる。地図のパーツ・キャラと同じ描画用の画面（歩きでずれる画面の外側まで含む）の	 */
/*		 座標（自分のマスが (DRAW_PARTS_X+1)*32）に合わせるので、歩いている間も地図と一緒に	 */
/*		 ドット単位で動く（画面に見える範囲の切り取りだけ、画面の左上 nX,nY を使う）。	 */
/*		 2 層に分けて描く：下の層（bTop=FALSE。キャラより前＝地図のパーツの後）は井戸の背景・	 */
/*		 枠・ブロック・落ち物。上の層（bTop=TRUE。ウィンドウの下）は帯・待ちお邪魔のバー・	 */
/*		 カウントダウン・GAME OVER。盤面の下に立つキャラの頭が隠れないようにする			 */
/*		 ブロックの絵は新しい画像を使わず、単色の塗り分け（立体の縁つき）		 */
/* ========================================================================= */
static const COLORREF s_aclPuzzle[PUZZLE_KINDS + 1] = {
	RGB (70, 210, 235),		/* I 水色 */
	RGB (240, 215, 60),		/* O 黄 */
	RGB (180, 90, 220),		/* T 紫 */
	RGB (90, 210, 90),		/* S 緑 */
	RGB (235, 80, 70),		/* Z 赤 */
	RGB (80, 120, 235),		/* J 青 */
	RGB (240, 150, 50),		/* L 橙 */
	RGB (130, 130, 140),	/* お邪魔ブロック 灰 */
};

static COLORREF PuzzleShade(COLORREF cl, int nPct)
{
	int r = GetRValue (cl) * nPct / 100, g = GetGValue (cl) * nPct / 100, b = GetBValue (cl) * nPct / 100;
	return RGB (min (r, 255), min (g, 255), min (b, 255));
}

/* 重ね描きを画面の中だけに切り取る範囲（DrawPuzzle が画面の左上・右下に合わせて決める）。
   盤面が画面の端にかかる時、画面に入っている部分だけを描く */
static int s_nPzL, s_nPzT, s_nPzR, s_nPzB;

/* 横 1 行を、切り取り範囲の中だけ半透明で塗る */
static void PuzSpan(HDC hDC, int x, int y, int cx, COLORREF cl, int nAlpha)
{
	int x1 = x + cx;

	if ((y < s_nPzT) || (y >= s_nPzB)) {
		return;
	}
	if (x < s_nPzL) {
		x = s_nPzL;
	}
	if (x1 > s_nPzR) {
		x1 = s_nPzR;
	}
	if (x1 > x) {
		W32ShadeSpan (hDC, x, y, x1 - x, 0, cl, nAlpha);
	}
}

/* 枠のある帯（半透明の暗い帯＋1 ドットの枠）を、切り取り範囲の中だけ描く */
static void PuzFrame(HDC hDC, int x, int y, int cx, int cy, COLORREF clFrame, int nAlpha)
{
	int yy;

	if ((y >= s_nPzB) || (y + cy <= s_nPzT) || (x >= s_nPzR) || (x + cx <= s_nPzL)) {
		return;
	}
	for (yy = max (0, s_nPzT - y); yy < cy; yy ++) {
		if (y + yy >= s_nPzB) {
			break;
		}
		if ((yy == 0) || (yy == cy - 1)) {
			PuzSpan (hDC, x + 1, y + yy, cx - 2, clFrame, 255);
		} else {
			PuzSpan (hDC, x, y + yy, cx, RGB (0, 0, 0), nAlpha);
			PuzSpan (hDC, x, y + yy, 1, clFrame, 255);
			PuzSpan (hDC, x + cx - 1, y + yy, 1, clFrame, 255);
		}
	}
}

/* 枠線なしで背景の暗い色だけを敷く（盤面の背景用。盤面のブロックと枠線が重ならないようにする） */
static void PuzFill(HDC hDC, int x, int y, int cx, int cy, int nAlpha)
{
	int yy;

	if ((y >= s_nPzB) || (y + cy <= s_nPzT) || (x >= s_nPzR) || (x + cx <= s_nPzL)) {
		return;
	}
	for (yy = max (0, s_nPzT - y); yy < cy; yy ++) {
		if (y + yy >= s_nPzB) {
			break;
		}
		PuzSpan (hDC, x, y + yy, cx, RGB (0, 0, 0), nAlpha);
	}
}

/* 文字は、全体が切り取り範囲に入る時だけ描く（半分切れた文字は出さない）。nHeight は文字の高さ */
static void PuzText(HDC hDC, int x, int y, LPSTR pszText, COLORREF cl, int nHeight, BOOL bCenter)
{
	SIZE size;

	GetTextExtentPoint32 (hDC, pszText, strlen (pszText), &size);
	if (bCenter) {
		x -= size.cx / 2;
	}
	if ((x < s_nPzL) || (x + size.cx > s_nPzR) || (y < s_nPzT) || (y + nHeight > s_nPzB)) {
		return;
	}
	TextOut2 (hDC, x, y, pszText, cl);
}

/* 文字を 1 文字ずつ、切り取り範囲に全部入る文字だけ描く（半角英数字だけの短い文字列用）。
   帯が画面の左右に見切れる時、枠が見えているのに字が全部消えることがないよう、見えている分の字は出す */
static void PuzTextClip(HDC hDC, int x, int y, LPSTR pszText, COLORREF cl, int nHeight)
{
	char szOne[2];
	SIZE size;

	szOne[1] = 0;
	for (; *pszText; pszText ++) {
		szOne[0] = *pszText;
		GetTextExtentPoint32 (hDC, szOne, 1, &size);
		PuzText (hDC, x, y, szOne, cl, nHeight, FALSE);
		x += size.cx;
	}
}

/* ブロック 1 つ（PUZZLE_BLOCK_DOT 四方）。nKind は 0〜6、お邪魔は 7 */
static void PuzzleDrawBlock(HDC hDC, int x, int y, int nKind)
{
	const int n = PUZZLE_BLOCK_DOT;
	COLORREF cl = s_aclPuzzle[nKind];
	COLORREF clLight = PuzzleShade (cl, 135), clDark = PuzzleShade (cl, 60);
	int yy;

	if ((x >= s_nPzR) || (x + n <= s_nPzL) || (y >= s_nPzB) || (y + n <= s_nPzT)) {
		return;
	}
	for (yy = 0; yy < n; yy ++) {
		if (yy == 0) {
			PuzSpan (hDC, x, y + yy, n, clLight, 255);
		} else if (yy == n - 1) {
			PuzSpan (hDC, x, y + yy, n, clDark, 255);
		} else {
			PuzSpan (hDC, x, y + yy, n, cl, 255);
			PuzSpan (hDC, x, y + yy, 1, clLight, 255);
			PuzSpan (hDC, x + n - 1, y + yy, 1, clDark, 255);
		}
	}
}

/* 盤面 1 つ（自分の台・対戦相手の台・近くで遊んでいる台）を描く。sx, sy は画面上のフィールドの左上 */
static void DrawPuzzleOne(HDC hDC, const CMDPARA_PUZZLESTATE &St, const BYTE *pBoard, BOOL bOver, int sx, int sy, BOOL bTop)
{
	char szTmp[64];
	int x, y, i, nCols = St.byCols, nRows = St.byRows, nW, nH, px, py, anC[4][2];
	const int nPanelW = 96;

	nW = nCols * PUZZLE_BLOCK_DOT;
	nH = nRows * PUZZLE_BLOCK_DOT;

	if (!bTop) {
		/* 下の層：枠と盤面の背景 */
		PuzFill (hDC, sx, sy, nW, nH, 190);

		/* 積んだブロック（お邪魔は灰色） */
		for (y = 0; y < nRows; y ++) {
			for (x = 0; x < nCols; x ++) {
				int nCell = pBoard[x + y * nCols];
				if ((nCell >= 1) && (nCell <= PUZZLE_CELL_GARBAGE)) {
					PuzzleDrawBlock (hDC, sx + x * PUZZLE_BLOCK_DOT, sy + y * PUZZLE_BLOCK_DOT, nCell - 1);
				}
			}
		}
		/* 操作中の落ち物 */
		if ((St.byPiece < PUZZLE_KINDS) && !bOver && !St.byIdle) {
			PuzzleCells (St.byPiece, St.byRot, anC);
			for (i = 0; i < 4; i ++) {
				x = (int)St.cX + anC[i][0];
				y = (int)St.cY + anC[i][1];
				if ((x >= 0) && (x < nCols) && (y >= 0) && (y < nRows)) {
					PuzzleDrawBlock (hDC, sx + x * PUZZLE_BLOCK_DOT, sy + y * PUZZLE_BLOCK_DOT, St.byPiece);
				}
			}
		}
		return;
	}

	/* ここから上の層（帯・バー・文字） */

	/* 待ちお邪魔の段数のバー（帯と反対側の、フィールドの横。下から積み上がる分だけ赤く伸びる） */
	/* 待ちが 0 の時は何も描かない（対戦中ずっと、盤面の横に黒っぽい縦線が出て見えないように） */
	if (!St.byIdle && (St.byGarbage > 0)) {
		const int nBarW = 6;
		int bx = (St.bySide == PUZZLESIDE_LEFT) ? (sx + nW + 6) : (sx - 6 - nBarW);
		int nBarH = (int)St.byGarbage * PUZZLE_BLOCK_DOT;
		if (nBarH > nH) {
			nBarH = nH;
		}
		for (y = 0; y < nH; y ++) {
			PuzSpan (hDC, bx, sy + y, nBarW, RGB (0, 0, 0), 150);
		}
		for (y = nH - nBarH; y < nH; y ++) {
			PuzSpan (hDC, bx + 1, sy + y, nBarW - 2, RGB (235, 70, 60), 255);
		}
	}

	/* 横の帯：次の落ち物・得点・ライン・レベル（座っている本人の台にだけ。待機中・ほかの人の台には出さない） */
	if (St.byMine && !St.byIdle) {
		px = (St.bySide == PUZZLESIDE_LEFT) ? (sx - 6 - nPanelW) : (sx + nW + 6);
		py = sy;
		PuzFrame (hDC, px, py, nPanelW, 150, RGB (200, 200, 200), 150);
		PuzText (hDC, px + 8, py + 4, (LPSTR)"NEXT", RGB (255, 235, 120), 16, FALSE);
		if (St.byNext < PUZZLE_KINDS) {
			PuzzleCells (St.byNext, 0, anC);
			for (i = 0; i < 4; i ++) {
				PuzzleDrawBlock (hDC, px + 16 + anC[i][0] * PUZZLE_BLOCK_DOT, py + 24 + anC[i][1] * PUZZLE_BLOCK_DOT, St.byNext);
			}
		}
		PuzText (hDC, px + 8, py + 70, (LPSTR)"SCORE", RGB (255, 235, 120), 16, FALSE);
		wsprintf (szTmp, "%u", (unsigned int)St.dwScore);
		PuzText (hDC, px + 8, py + 88, szTmp, RGB (255, 255, 255), 16, FALSE);
		wsprintf (szTmp, "LINES %d", (int)St.wLines);
		PuzText (hDC, px + 8, py + 108, szTmp, RGB (255, 255, 255), 16, FALSE);
		wsprintf (szTmp, "LEVEL %d", (int)St.byLevel);
		PuzText (hDC, px + 8, py + 126, szTmp, RGB (255, 255, 255), 16, FALSE);
	}

	/* 遊んでいる台を見ている人（座っている本人以外）には、得点・ライン・レベルを盤面の上端に小さく出す。
	   盤面の幅の中に収め（隣の盤面と重ならない）、盤面が画面の上へ出ていく間は画面の上端に張り付いて、
	   盤面と一緒に見えなくなる（盤面から離れて浮かない）。画面の外・盤面の外にかかる分は描かない（PuzText の切り取り） */
	if (!St.byMine && !St.byIdle) {
		SIZE sz1, sz2;
		int tw, tx, ty;
		char szLine1[48], szLine2[48];

		wsprintf (szLine1, "SCORE %u", (unsigned int)St.dwScore);
		wsprintf (szLine2, "LINES %d LV %d", (int)St.wLines, (int)St.byLevel);
		GetTextExtentPoint32 (hDC, szLine1, strlen (szLine1), &sz1);
		GetTextExtentPoint32 (hDC, szLine2, strlen (szLine2), &sz2);
		tw = min (max (sz1.cx, sz2.cx) + 8, nW);
		tx = sx + (nW - tw) / 2;
		ty = max (sy + 2, s_nPzT + 2);
		ty = min (ty, sy + nH - 40);		/* 盤面の下端より下へは出ない */
		PuzFrame (hDC, tx, ty, tw, 38, RGB (200, 200, 200), 150);
		PuzTextClip (hDC, tx + 4, ty + 3, szLine1, RGB (255, 235, 120), 16);
		PuzTextClip (hDC, tx + 4, ty + 20, szLine2, RGB (255, 255, 255), 16);
	}

	/* 勝ち残りの連勝数（2 連勝以上。待ちお邪魔のバーの外側、帯と反対側の上の方に小さく。観覧者にも見える） */
	if (!St.byIdle && (St.byStreak >= 2)) {
		SIZE size;
		int tx, ty = sy + 2;
		wsprintf (szTmp, "%d連勝", (int)St.byStreak);
		GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
		tx = (St.bySide == PUZZLESIDE_LEFT) ? (sx + nW + 6 + 6 + 4) : (sx - 6 - 6 - 4 - size.cx);
		PuzFrame (hDC, tx - 3, ty - 2, size.cx + 6, size.cy + 4, RGB (240, 200, 60), 150);
		PuzText (hDC, tx, ty, szTmp, RGB (255, 235, 120), size.cy, FALSE);
	}

	/* 座る場所に立っている時の案内（待機中の台の、本人にだけ。サーバーが PUZZLEIDLE_SEAT を付ける）。
	   盤面の中央に「INSERT COIN」「道具→コイン」（コマンドの道具からコインを使う）を約 1 秒周期で点滅させる
	   （必要な枚数が 0 の台は「PUSH START」「（Sキー）」）。GAME OVER の字が出ている間は出さない */
	if ((St.byIdle & PUZZLEIDLE_SEAT) && !bOver) {
		g_bDraw = TRUE;		/* 点滅のため、出ている間は描き続ける */
		if (((timeGetTime () / 500) & 1) == 0) {
			BOOL bFree = (St.byIdle & PUZZLEIDLE_FREE) ? TRUE : FALSE;
			PuzText (hDC, sx + nW / 2, sy + nH / 2 - 18, (LPSTR)(bFree ? "PUSH START" : "INSERT COIN"), RGB (255, 235, 120), 16, TRUE);
			/* 有料の台は「道具→コイン」を下に小さく。無料の台は今まで通り「（Sキー）」 */
			if (bFree) {
				PuzText (hDC, sx + nW / 2, sy + nH / 2 + 2, (LPSTR)"（Sキー）", RGB (255, 255, 255), 16, TRUE);
			} else {
				PuzText (hDC, sx + nW / 2, sy + nH / 2 + 4, (LPSTR)"道具→コイン", RGB (255, 255, 255), 14, TRUE);
			}
		}
	}

	/* 対戦開始前のカウントダウン（盤面の真ん中に大きく） */
	if ((St.byCount > 0) && !St.byIdle) {
		wsprintf (szTmp, "%d", (int)St.byCount);
		SelectObject (hDC, g_hFont32);
		PuzText (hDC, sx + nW / 2, sy + nH / 2 - 16, szTmp, RGB (255, 255, 255), 32, TRUE);
		SelectObject (hDC, g_hFont14);
	}
	if (bOver) {
		PuzText (hDC, sx + nW / 2, sy + nH / 2 - 8, (LPSTR)"GAME OVER", RGB (255, 120, 110), 16, TRUE);
	}
}

static void DrawPuzzle(int nX, int nY, BOOL bTop)
{
	CMDPARA_PUZZLESTATE St;
	const BYTE *pBoard;
	PCHARINFO pChar;
	HDC hDC = NULL;
	HFONT hFontOld = NULL;
	BOOL bOver;
	int i, sx, sy;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	for (i = 0; i < PUZZLE_VIEW_MAX; i ++) {
		if (!GetPuzzleState (i, &St, &pBoard, &bOver)) {
			continue;
		}
		/* マップのドット座標 → 描画用の画面（地図のパーツ・キャラと同じ座標。自分のマスが (DRAW_PARTS_X+1)*32）。
		   歩きのずれ（nX,nY = 32 - BowOffset）は画面の切り取り範囲だけに効かせ、盤面の位置には足さない
		   （以前は盤面の位置に nX を足していたため歩きのずれが地図と合わず、1 マスごとにカクカクした） */
		sx = (DRAW_PARTS_X + 1) * 32 + (int)St.sFieldX - CamTileX () * 32;
		sy = (DRAW_PARTS_Y + 1) * 32 + (int)St.sFieldY - CamTileY () * 32;
		/* 画面に少しもかからない台は描かない（帯・バーが出る台はそのぶん広く見る。画面にかかる分だけ切り取って描く） */
		{
			int nExtra = (St.byIdle ? 0 : 110) + 6;
			if ((sx + St.byCols * PUZZLE_BLOCK_DOT + nExtra <= nX) || (sx - nExtra >= nX + SCRSIZEX) ||
				(sy + St.byRows * PUZZLE_BLOCK_DOT + 6 <= nY) || (sy - 6 >= nY + SCRSIZEY)) {
				continue;
			}
		}
		s_nPzL = nX;
		s_nPzT = nY;
		s_nPzR = nX + SCRSIZEX;
		s_nPzB = nY + SCRSIZEY;
		if (hDC == NULL) {
			hDC = g_pDibDrawMap->Lock ();
			hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
			SetBkMode (hDC, TRANSPARENT);
		}
		DrawPuzzleOne (hDC, St, pBoard, bOver, sx, sy, bTop);
	}
	if (hDC) {
		SelectObject (hDC, hFontOld);
		g_pDibDrawMap->Unlock ();
	}
}

/* ========================================================================= */
/* 関数名：DrawPinball														 */
/* 内容：ピンボール（見えている台すべて＝自分の台・近くの台）の重ね描き				 */
/*		 落ち物パズルと同じ 2 層：bTop=FALSE（盤本体：壁・アーチ・バンパー・ターゲット・	 */
/*		 フリッパー・ボール。キャラの前）、bTop=TRUE（帯・文字。ウィンドウの下）。		 */
/*		 盤の位置は台NPCの座標からサーバーが決めて送る（sFieldX/Y）。					 */
/*		 絵は図形（円・線・矩形）だけ。台の形は common/Pinball.h の PinballTable			 */
/* ========================================================================= */
static void PbFillCircle(HDC hDC, int cx, int cy, int r, COLORREF cl)
{
	int dy;

	for (dy = -r; dy <= r; dy ++) {
		int nHalf = (int)PbIsqrt ((unsigned long long)(r * r - dy * dy));
		PuzSpan (hDC, cx - nHalf, cy + dy, nHalf * 2 + 1, cl, 255);
	}
}

/* 太さ nThick の線（四角い筆で、1 ドットずつ） */
static void PbLine(HDC hDC, int x1, int y1, int x2, int y2, int nThick, COLORREF cl)
{
	int dx = abs (x2 - x1), dy = abs (y2 - y1);
	int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
	int err = dx - dy, ty;

	for (;;) {
		for (ty = 0; ty < nThick; ty ++) {
			PuzSpan (hDC, x1 - nThick / 2, y1 - nThick / 2 + ty, nThick, cl, 255);
		}
		if ((x1 == x2) && (y1 == y2)) {
			break;
		}
		{
			int e2 = err * 2;
			if (e2 > -dy) { err -= dy; x1 += sx; }
			if (e2 < dx) { err += dx; y1 += sy; }
		}
	}
}

static int PbPx(int nFp) { return (nFp + PB_FP / 2) / PB_FP; }

/* ---- 差し替え絵（docs/pinball-art.md）。黒(0,0,0)は透明。描く範囲は切り取り範囲（s_nPz*）の中だけ ---- */
/* 絵の一部を (dx,dy) へ写す（切り取り範囲に合わせて削る） */
static void PbArtBlt(CImg32 *pSrc, int sx, int sy, int cx, int cy, int dx, int dy)
{
	if (dx < s_nPzL) { sx += s_nPzL - dx; cx -= s_nPzL - dx; dx = s_nPzL; }
	if (dy < s_nPzT) { sy += s_nPzT - dy; cy -= s_nPzT - dy; dy = s_nPzT; }
	if (dx + cx > s_nPzR) { cx = s_nPzR - dx; }
	if (dy + cy > s_nPzB) { cy = s_nPzB - dy; }
	if ((cx <= 0) || (cy <= 0)) {
		return;
	}
	g_pDibDrawMap->Blt (dx, dy, cx, cy, pSrc, sx, sy, TRUE);
}

/* 絵のドット (x,y) の色（上下逆の DIB を読む。範囲外・透明は FALSE） */
static BOOL PbArtPixel(CImg32 *pSrc, int x, int y, DWORD *pdwCl)
{
	DWORD dw;

	if ((x < 0) || (y < 0) || (x >= pSrc->Width ()) || (y >= pSrc->Height ())) {
		return FALSE;
	}
	dw = ((PDWORD)(pSrc->GetBits () + (size_t)(pSrc->Height () - 1 - y) * pSrc->Width () * 4))[x];
	if ((dw & 0x00FFFFFF) == 0) {
		return FALSE;
	}
	*pdwCl = dw;
	return TRUE;
}

/* 軸 (px,py)（画面）を中心に、絵（セルの左上 sx,sy・軸の位置 pivx,pivy）を nDeg 度回して描く。bMirror で左右反転（右のフリッパー）。
   回転は逆向きに写して（最近傍）取る。角度は PbFlipDeg と同じ向き（水平から下向きが正） */
static void PbArtBltRot(CImg32 *pSrc, int sx, int sy, int nPivX, int nPivY, int px, int py, int nDeg, BOOL bMirror, int nRadius)
{
	int dxx, dyy, nCos = PbCos (nDeg), nSin = PbSin (nDeg);
	DWORD dwCl;
	PDWORD pDst;

	for (dyy = -nRadius; dyy <= nRadius; dyy ++) {
		int y = py + dyy;
		if ((y < s_nPzT) || (y >= s_nPzB)) {
			continue;
		}
		for (dxx = -nRadius; dxx <= nRadius; dxx ++) {
			int x = px + dxx, rx = bMirror ? -dxx : dxx, ux, uy, ix, iy;
			if ((x < s_nPzL) || (x >= s_nPzR)) {
				continue;
			}
			/* 画面の (rx,dyy) を -nDeg 回して、絵の軸からの位置 (ux,uy) にする */
			ux = (rx * nCos + dyy * nSin) / PB_TRIG;
			uy = (-rx * nSin + dyy * nCos) / PB_TRIG;
			ix = sx + nPivX + ux;
			iy = sy + nPivY + uy;
			if ((ix < sx) || (iy < sy) || (ix >= sx + PBART_FLIP_S) || (iy >= sy + PBART_FLIP_S)) {
				continue;
			}
			if (PbArtPixel (pSrc, ix, iy, &dwCl)) {
				pDst = (PDWORD)(g_pDibDrawMap->GetBits () + (size_t)(g_pDibDrawMap->Height () - 1 - y) * g_pDibDrawMap->Width () * 4);
				pDst[x] = dwCl;
			}
		}
	}
}

/* 絵（w×h）を縦だけ dh 行に伸び縮みして (dx,dy) へ（最近傍。切り取り範囲の中だけ） */
static void PbArtBltStretchV(CImg32 *pSrc, int sx, int sy, int w, int h, int dx, int dy, int dh)
{
	int r, i;
	DWORD dwCl;

	for (r = 0; r < dh; r ++) {
		int y = dy + r, ys = sy + r * h / dh;
		PDWORD pDst;
		if ((y < s_nPzT) || (y >= s_nPzB)) {
			continue;
		}
		pDst = (PDWORD)(g_pDibDrawMap->GetBits () + (size_t)(g_pDibDrawMap->Height () - 1 - y) * g_pDibDrawMap->Width () * 4);
		for (i = 0; i < w; i ++) {
			int x = dx + i;
			if ((x < s_nPzL) || (x >= s_nPzR)) {
				continue;
			}
			if (PbArtPixel (pSrc, sx + i, ys, &dwCl)) {
				pDst[x] = dwCl;
			}
		}
	}
}

/* 打ち出しのバネ（見た目だけ）：レーンの床の下に穴（暗い箱）を重ね、頭（受け皿）を nSpring ドット沈めて、コイルを縮める。
   休み位置の球は頭の上に乗って一緒に沈む。絵（部品シートのバネ）が無ければ、頭は横棒・コイルは線のジグザグ。
   穴は盤の下端より下へはみ出すので、切り取り範囲の下端を穴の深さまで広げて描く */
static void PbDrawSpring(HDC hDC, const PINBALLDRAW &D, int sx, int sy, int ty0, CImg32 *pParts, const int nScrL, const int nScrT, const int nScrR, const int nScrB)
{
	const PinballTable &T = PinballGetTable ();
	const int nSaveL = s_nPzL, nSaveT = s_nPzT, nSaveR = s_nPzR, nSaveB = s_nPzB;		/* 窓に切り取った範囲（終わりに戻す） */
	const int nFloor = ty0 + PB_LANE_FLOOR, nBottom = nFloor + PBART_SPRING_WELL_H;
	const int cx = sx + PbPx (T.nStartX), nHeadY = nFloor + D.nSpring, nCoilTop = nHeadY + PBART_SPRING_HEAD_H;
	const int nWellL = sx + PB_LANE_L, nWellR = sx + PB_WALL_R + 2;
	int yy, i;

	/* 床が窓の外（窓が上へ動いている間）は描かない。窓の下端より下へは穴の深さまで */
	const BOOL bVis = ((nFloor >= s_nPzT) && (nFloor < sy + PB_VIEW_H)) ? TRUE : FALSE;

	if (bVis) {
	s_nPzL = max (nScrL, nWellL);			/* 画面の範囲（窓ではなく）に、穴の範囲を重ねる */
	s_nPzR = min (nScrR, nWellR);
	s_nPzT = max (nScrT, nFloor);
	s_nPzB = min (nScrB, nBottom);
	/* 穴（床の絵を隠す暗い箱。左右と底に細い縁） */
	for (yy = nFloor; yy < nBottom; yy ++) {
		PuzSpan (hDC, nWellL, yy, nWellR - nWellL, RGB (20, 20, 38), 255);
		if (yy >= nBottom - 2) {
			PuzSpan (hDC, nWellL, yy, nWellR - nWellL, RGB (84, 70, 76), 255);
		} else {
			PuzSpan (hDC, nWellL, yy, 2, RGB (84, 70, 76), 255);
			PuzSpan (hDC, nWellR - 2, yy, 2, RGB (84, 70, 76), 255);
		}
	}
	{
		const int nCoilBottom = nBottom - 2;
		const int nCoilH = nCoilBottom - nCoilTop;
		if (pParts) {
			if (nCoilH > 0) {
				PbArtBltStretchV (pParts, PBART_SPRING_X, PBART_SPRING_COIL_Y, PBART_SPRING_COIL_W, PBART_SPRING_COIL_H,
					cx - PBART_SPRING_COIL_W / 2, nCoilTop, nCoilH);
			}
			PbArtBlt (pParts, PBART_SPRING_X, PBART_SPRING_HEAD_Y, PBART_SPRING_HEAD_W, PBART_SPRING_HEAD_H, cx - PBART_SPRING_HEAD_W / 2, nHeadY);
		} else {
			/* コイル：線のジグザグ（縮むと細かくなる） */
			if (nCoilH > 0) {
				int nZig = 6, nPrevX = cx, nPrevY = nCoilTop;
				for (i = 1; i <= nZig; i ++) {
					int zx = cx + ((i & 1) ? 5 : -5), zy = nCoilTop + nCoilH * i / nZig;
					if (i == nZig) { zx = cx; }
					PbLine (hDC, nPrevX, nPrevY, zx, zy, 1, RGB (200, 200, 215));
					nPrevX = zx; nPrevY = zy;
				}
			}
			/* 頭：横棒 */
			for (yy = 0; yy < PBART_SPRING_HEAD_H / 2; yy ++) {
				PuzSpan (hDC, cx - 6, nHeadY + yy, 12, RGB (235, 235, 245), 255);
			}
		}
	}
	/* 球は床の上から穴の中まで（下端だけ穴の深さまで広げる） */
	s_nPzL = nSaveL; s_nPzR = nSaveR; s_nPzT = nSaveT; s_nPzB = min (nScrB, nBottom);
	}
	/* 休み位置の球（頭の上に乗って沈む） */
	if (D.bBallShow && D.bBallReady) {
		int bx = sx + PbPx (D.nBallX), by = ty0 + PbPx (D.nBallY) + D.nSpring;
		if (pParts) {
			PbArtBlt (pParts, PBART_BALL_X, PBART_BALL_Y, PBART_BALL_S, PBART_BALL_S, bx - PBART_BALL_S / 2, by - PBART_BALL_S / 2);
		} else {
			PbFillCircle (hDC, bx, by, PbPx (PB_BALL_R), RGB (235, 235, 235));
			PbFillCircle (hDC, bx - 1, by - 1, 1, RGB (255, 255, 255));
		}
	}
	s_nPzL = nSaveL; s_nPzT = nSaveT; s_nPzR = nSaveR; s_nPzB = nSaveB;
}

static void DrawPinballOne(HDC hDC, const PINBALLDRAW &D, int sx, int sy, BOOL bTop)
{
	const PinballTable &T = PinballGetTable ();
	const CMDPARA_PINBALLSTATE &St = D.St;
	const int nW = PB_W, nH = PB_VIEW_H;		/* 見える窓の大きさ（盤全体は PB_H。窓が D.nCamY の高さに映す） */
	const int ty0 = sy - D.nCamY;			/* 盤の上端の画面上の位置（盤の座標 y は ty0 + y に描く） */
	char szTmp[64];
	int i;

	if (!bTop) {
		const COLORREF clWall = RGB (150, 190, 255);
		const int nClipL = s_nPzL, nClipT = s_nPzT, nClipR = s_nPzR, nClipB = s_nPzB;
		CImg32 *pArtBg = NULL, *pArtParts = NULL;
		const BOOL bArt = GetPinballArt (&pArtBg, &pArtParts);		/* 差し替え絵があれば、図形の代わりにそれで描く */
		/* 背景と枠（窓の大きさ。待機中は盤の下端を映す。差し替え絵の時は絵に任せる） */
		if (!bArt) {
			PuzFrame (hDC, sx - 1, sy - 1, nW + 2, nH + 2, RGB (200, 200, 200), 170);
		}
		/* 盤の絵は窓の中だけに切り取る（盤が窓より高い時、窓の外にはみ出さない） */
		s_nPzL = max (s_nPzL, sx);
		s_nPzR = min (s_nPzR, sx + nW);
		s_nPzT = max (s_nPzT, sy);
		s_nPzB = min (s_nPzB, sy + nH);
		if (bArt) {
			/* 背景（盤全体 160×PB_H の、窓が映している範囲） */
			PbArtBlt (pArtBg, 0, D.nCamY, nW, nH, sx, sy);
			/* ターゲット（セルの中心＝線分の中心） */
			for (i = 0; i < T.nSeg; i ++) {
				const PBSEG &S = T.aSeg[i];
				int nCellX, cx, cy;
				if (S.nTarget < 0) {
					continue;
				}
				if (D.dwFlashAge[PB_BUMPERS + S.nTarget] < PINBALL_FLASH_MS) {
					nCellX = PBART_TARGET_HIT_X;
				} else if (D.nTargetMask & (1u << S.nTarget)) {
					nCellX = PBART_TARGET_DOWN_X;
				} else {
					nCellX = PBART_TARGET_UP_X;
				}
				cx = sx + PbPx ((S.x1 + S.x2) / 2);
				cy = ty0 + PbPx ((S.y1 + S.y2) / 2);
				PbArtBlt (pArtParts, nCellX, PBART_TARGET_Y, PBART_TARGET_W, PBART_TARGET_H, cx - PBART_TARGET_W / 2, cy - PBART_TARGET_H / 2);
			}
			/* バンパー */
			for (i = 0; i < PB_BUMPERS; i ++) {
				int cx = sx + PbPx (T.aBumper[i].x), cy = ty0 + PbPx (T.aBumper[i].y);
				BOOL bFlash = D.dwFlashAge[i] < PINBALL_FLASH_MS;
				PbArtBlt (pArtParts, bFlash ? PBART_BUMPER_HIT_X : PBART_BUMPER_X, PBART_BUMPER_Y, PBART_BUMPER_S, PBART_BUMPER_S,
					cx - PBART_BUMPER_S / 2, cy - PBART_BUMPER_S / 2);
			}
			/* 打ち出しレーンの溜めは、バネの沈み込み（PbDrawSpring）で見せる。強さの棒は描かない */
			/* フリッパー：近い方の姿（下がり・上がり）の絵を、今の角度との差だけ回して描く。右は左右反転 */
			for (i = 0; i < 2; i ++) {
				int nAng = i ? D.nAngR : D.nAngL, bUp = (nAng * 2 >= PB_FLIP_STEPS) ? 1 : 0;
				int nPoseDeg = PbFlipDeg (bUp ? PB_FLIP_STEPS : 0);
				PbArtBltRot (pArtParts, bUp ? PBART_FLIP_UP_X : PBART_FLIP_REST_X, PBART_FLIP_Y, PBART_FLIP_PX, PBART_FLIP_PY,
					sx + PbPx (i ? T.nPivotRx : T.nPivotLx), ty0 + PbPx (T.nPivotY), PbFlipDeg (nAng) - nPoseDeg, i ? TRUE : FALSE, 46);
			}
			/* 打ち出しのバネ（絵にバネが無い古い部品シートなら図形のバネ）と、休み位置の球 */
			if (!St.byIdle) {
				PbDrawSpring (hDC, D, sx, sy, ty0, PinballArtHasSpring () ? pArtParts : NULL, nClipL, nClipT, nClipR, nClipB);
			}
			/* ボール */
			if (D.bBallShow && !St.byIdle && !D.bBallReady) {
				PbArtBlt (pArtParts, PBART_BALL_X, PBART_BALL_Y, PBART_BALL_S, PBART_BALL_S,
					sx + PbPx (D.nBallX) - PBART_BALL_S / 2, ty0 + PbPx (D.nBallY) - PBART_BALL_S / 2);
			}
		}
		if (!bArt) {		/* 図形で描く（差し替え絵が無い時） */
		/* 壁・斜め壁・仕切り・床（ターゲット以外の線分） */
		for (i = 0; i < T.nSeg; i ++) {
			const PBSEG &S = T.aSeg[i];
			if (S.nTarget >= 0) {
				continue;
			}
			PbLine (hDC, sx + PbPx (S.x1), ty0 + PbPx (S.y1), sx + PbPx (S.x2), ty0 + PbPx (S.y2), 2, clWall);
		}
		/* 上部のアーチ（半円。5 度刻みの点をつなぐ） */
		{
			int nDeg, nPrevX = 0, nPrevY = 0;
			int cx = sx + PbPx (T.nArchCx), cy = ty0 + PbPx (T.nArchCy), r = PbPx (T.nArchR);
			for (nDeg = 180; nDeg <= 360; nDeg += 5) {
				int ax = cx + r * PbCos (nDeg) / PB_TRIG, ay = cy + r * PbSin (nDeg) / PB_TRIG;
				if (nDeg > 180) {
					PbLine (hDC, nPrevX, nPrevY, ax, ay, 2, clWall);
				}
				nPrevX = ax;
				nPrevY = ay;
			}
		}
		/* ターゲット（立っている間は黄色、倒れたら暗い） */
		for (i = 0; i < T.nSeg; i ++) {
			const PBSEG &S = T.aSeg[i];
			BOOL bDown, bFlash;
			if (S.nTarget < 0) {
				continue;
			}
			bDown = (D.nTargetMask & (1u << S.nTarget)) ? TRUE : FALSE;
			bFlash = D.dwFlashAge[PB_BUMPERS + S.nTarget] < PINBALL_FLASH_MS;
			PbLine (hDC, sx + PbPx (S.x1), ty0 + PbPx (S.y1), sx + PbPx (S.x2), ty0 + PbPx (S.y2), 4,
				bFlash ? RGB (255, 255, 255) : (bDown ? RGB (90, 80, 40) : RGB (255, 215, 60)));
		}
		/* バンパー（丸い反発。当たると光る） */
		for (i = 0; i < PB_BUMPERS; i ++) {
			int bx = sx + PbPx (T.aBumper[i].x), by = ty0 + PbPx (T.aBumper[i].y), r = PbPx (PB_BUMPER_R);
			BOOL bFlash = D.dwFlashAge[i] < PINBALL_FLASH_MS;
			PbFillCircle (hDC, bx, by, r, bFlash ? RGB (255, 255, 255) : RGB (240, 110, 70));
			PbFillCircle (hDC, bx, by, r - 3, bFlash ? RGB (255, 235, 150) : RGB (150, 40, 40));
			PbFillCircle (hDC, bx, by, 2, RGB (255, 220, 160));
		}
		/* 打ち出しレーンの溜めは、バネの沈み込み（PbDrawSpring）で見せる。強さの棒は描かない */
		/* フリッパー（回る線分。太く） */
		for (i = 0; i < 2; i ++) {
			int nAng = i ? D.nAngR : D.nAngL, tx, ty, px = PbPx (i ? T.nPivotRx : T.nPivotLx), py = PbPx (T.nPivotY);
			PinballFlipperTip (T, i, nAng, &tx, &ty);
			PbLine (hDC, sx + px, ty0 + py, sx + PbPx (tx), ty0 + PbPx (ty), 5, RGB (235, 235, 245));
			PbFillCircle (hDC, sx + px, ty0 + py, 3, RGB (235, 235, 245));
			PbFillCircle (hDC, sx + PbPx (tx), ty0 + PbPx (ty), 3, RGB (235, 235, 245));
			PbFillCircle (hDC, sx + px, ty0 + py, 1, RGB (90, 90, 120));
		}
		/* 打ち出しのバネ（図形）と、休み位置の球 */
		if (!bArt && !St.byIdle) {
			PbDrawSpring (hDC, D, sx, sy, ty0, NULL, nClipL, nClipT, nClipR, nClipB);
		}
		/* ボール */
		if (D.bBallShow && !St.byIdle && !D.bBallReady) {
			int bx = sx + PbPx (D.nBallX), by = ty0 + PbPx (D.nBallY);
			PbFillCircle (hDC, bx, by, PbPx (PB_BALL_R), RGB (235, 235, 235));
			PbFillCircle (hDC, bx - 1, by - 1, 1, RGB (255, 255, 255));
		}
		}
		/* 窓の位置の目印（盤が窓より高い時。左端の細い縦線で、盤全体のどこを映しているか） */
		if ((PB_YOFS > 0) && !St.byIdle) {
			int nThumbH = max (8, nH * nH / PB_H), nThumbY = (int)((long long)(nH - nThumbH) * D.nCamY / PB_YOFS), yy;
			for (yy = 0; yy < nH; yy ++) {
				PuzSpan (hDC, sx + 1, sy + yy, 2, ((yy >= nThumbY) && (yy < nThumbY + nThumbH)) ? RGB (230, 230, 120) : RGB (90, 90, 90), 160);
			}
		}
		s_nPzL = nClipL; s_nPzT = nClipT; s_nPzR = nClipR; s_nPzB = nClipB;
		return;
	}

	/* 上の層：横の帯（落ち物の帯と同じ作り：盤の横・盤と同じ高さの上端・半透明の枠・黄色の見出しと白い値）。
	   座っている本人の台にだけ。待機中・ほかの人の台には出さない。帯の側は台の設定（bySide）。
	   ペアの台は、隣の盤と重ならないよう、左の台を左(1)・右の台を右(0)にして帯が外側へ出るようにする */
	if (!St.byIdle && St.byMine) {
		const int nPanelW = 96, nPanelH = 156;
		int px = (St.bySide == PUZZLESIDE_LEFT) ? (sx - 6 - nPanelW) : (sx + nW + 6), py = sy, ty = py + 4;
		int nBallsMax = ((int)St.byBallsMax >= 1) ? min ((int)St.byBallsMax, PB_BALLS_MAX) : PB_BALLS;
		int nBallStep = (nBallsMax <= 6) ? 14 : max (6, (nPanelW - 24) / (nBallsMax - 1));
		PuzFrame (hDC, px, py, nPanelW, nPanelH, RGB (200, 200, 200), 150);
		PuzText (hDC, px + 8, ty, (LPSTR)"SCORE", RGB (255, 235, 120), 16, FALSE);
		wsprintf (szTmp, "%u", (unsigned int)St.dwScore);
		PuzText (hDC, px + 8, ty + 18, szTmp, RGB (255, 255, 255), 16, FALSE);
		if (St.byVs) {
			/* 対戦：球は減らない。残り時間と相手の得点を出す */
			PuzText (hDC, px + 8, ty + 40, (LPSTR)"TIME", RGB (255, 235, 120), 16, FALSE);
			wsprintf (szTmp, "%d", (int)((St.wVsRemain + 9) / 10));
			PuzText (hDC, px + 8, ty + 58, szTmp, RGB (255, 255, 255), 16, FALSE);
			PuzText (hDC, px + 8, ty + 80, (LPSTR)"VS", RGB (255, 235, 120), 16, FALSE);
			wsprintf (szTmp, "%u", (unsigned int)St.dwOppScore);
			PuzText (hDC, px + 8, ty + 98, szTmp, RGB (255, 255, 255), 16, FALSE);
		} else {
			PuzText (hDC, px + 8, ty + 40, (LPSTR)"BALLS", RGB (255, 235, 120), 16, FALSE);
			/* 球の数は台の設定（サーバーが送ってくる。多い台は間隔をつめて帯に収める） */
			for (i = 0; i < nBallsMax; i ++) {
				int bx = px + 14 + i * nBallStep, by = ty + 66;
				PbFillCircle (hDC, bx, by, 4, (i < (int)St.byBalls) ? RGB (235, 235, 235) : RGB (70, 70, 80));
			}
		}
		if (St.byCombo >= 2) {
			wsprintf (szTmp, "COMBO %d", (int)St.byCombo);
			PuzText (hDC, px + 8, ty + (St.byVs ? 116 : 80), szTmp, RGB (255, 150, 90), 16, FALSE);
		}
		if ((St.bySave > 0) && (St.byPhase == PB_PHASE_PLAY)) {
			wsprintf (szTmp, "SAVE %d", (int)St.bySave);
			PuzText (hDC, px + 8, ty + (St.byVs ? 134 : 98), szTmp, RGB (120, 230, 255), 16, FALSE);		/* コンボの下 */
		}
	}

	/* 遊んでいる台を見ている人（座っている本人以外）には、得点・残り球（対戦中は相手の得点・残り時間）を盤の上端に小さく出す
	   （落ち物の観戦者向けの表示と同じ）。盤の幅の中に収め（隣の盤と重ならない）、盤が画面の上へ出ていく間は画面の上端に張り付いて、
	   盤と一緒に見えなくなる（盤から離れて浮かない）。画面の外・盤の外にかかる分は描かない（PuzText の切り取り） */
	if (!St.byMine && !St.byIdle) {
		SIZE sz1, sz2;
		int tw, tx, ty;
		char szLine1[48], szLine2[48];

		wsprintf (szLine1, "SCORE %u", (unsigned int)St.dwScore);
		if (St.byVs) {
			wsprintf (szLine2, "VS %u  TIME %d", (unsigned int)St.dwOppScore, (int)((St.wVsRemain + 9) / 10));
		} else {
			wsprintf (szLine2, "BALL %d", (int)St.byBalls);
		}
		GetTextExtentPoint32 (hDC, szLine1, strlen (szLine1), &sz1);
		GetTextExtentPoint32 (hDC, szLine2, strlen (szLine2), &sz2);
		tw = min (max (sz1.cx, sz2.cx) + 8, nW);
		tx = sx + (nW - tw) / 2;
		ty = max (sy + 2, s_nPzT + 2);
		ty = min (ty, sy + nH - 40);		/* 盤の下端より下へは出ない */
		PuzFrame (hDC, tx, ty, tw, 38, RGB (200, 200, 200), 150);
		PuzTextClip (hDC, tx + 4, ty + 3, szLine1, RGB (255, 235, 120), 16);
		PuzTextClip (hDC, tx + 4, ty + 20, szLine2, RGB (255, 255, 255), 16);
	}

	/* 勝ち残りの連勝数（2 連勝以上。帯と反対側の上の方に小さく。観戦者にも見える。落ち物と同じ） */
	if (!St.byIdle && (St.byStreak >= 2)) {
		SIZE size;
		int tx, ty = sy + 2;
		wsprintf (szTmp, "%d連勝", (int)St.byStreak);
		GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
		tx = (St.bySide == PUZZLESIDE_LEFT) ? (sx + nW + 6 + 4) : (sx - 6 - 4 - size.cx);
		PuzFrame (hDC, tx - 3, ty - 2, size.cx + 6, size.cy + 4, RGB (240, 200, 60), 150);
		PuzText (hDC, tx, ty, szTmp, RGB (255, 235, 120), size.cy, FALSE);
	}
	/* 対戦開始前のカウントダウン（盤の真ん中に大きく。3・2・1） */
	if ((St.byCount > 0) && !St.byIdle) {
		wsprintf (szTmp, "%d", (int)St.byCount);
		SelectObject (hDC, g_hFont32);
		PuzText (hDC, sx + nW / 2, sy + nH / 2 - 16, szTmp, RGB (255, 255, 255), 32, TRUE);
		SelectObject (hDC, g_hFont14);
		g_bDraw = TRUE;
	}
	/* 座る場所に立っている時の案内（待機中の台の、本人にだけ） */
	if ((St.byIdle & PINBALLIDLE_SEAT) && !D.bOver) {
		g_bDraw = TRUE;
		if (((timeGetTime () / 500) & 1) == 0) {
			BOOL bFree = (St.byIdle & PINBALLIDLE_FREE) ? TRUE : FALSE;
			PuzText (hDC, sx + nW / 2, sy + nH / 2 - 18, (LPSTR)(bFree ? "PUSH START" : "INSERT COIN"), RGB (255, 235, 120), 16, TRUE);
			if (bFree) {
				PuzText (hDC, sx + nW / 2, sy + nH / 2 + 2, (LPSTR)"（Sキー）", RGB (255, 255, 255), 16, TRUE);
			} else {
				PuzText (hDC, sx + nW / 2, sy + nH / 2 + 4, (LPSTR)"道具→コイン", RGB (255, 255, 255), 14, TRUE);
			}
		}
	}
	/* 打ち出し待ちの案内（本人の台：レーンで球が待っている間） */
	if (!St.byIdle && St.byMine && (St.byPhase == PB_PHASE_READY) && !D.bOver) {
		PuzText (hDC, sx + nW / 2, sy + nH - 56, (LPSTR)"↓を押して離すと発射", RGB (255, 255, 255), 14, TRUE);
	}
	if (D.bOver) {
		PuzText (hDC, sx + nW / 2, sy + nH / 2 - 8, (LPSTR)"GAME OVER", RGB (255, 120, 110), 16, TRUE);		/* 落ち物と同じ位置 */
		wsprintf (szTmp, "SCORE %u", (unsigned int)St.dwScore);
		PuzText (hDC, sx + nW / 2, sy + nH / 2 + 10, szTmp, RGB (255, 255, 255), 16, TRUE);
	}
}

static void DrawPinball(int nX, int nY, BOOL bTop)
{
	PINBALLDRAW D;
	HDC hDC = NULL;
	HFONT hFontOld = NULL;
	int i, sx, sy;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}
	for (i = 0; i < PINBALL_VIEW_MAX; i ++) {
		if (!GetPinballView (i, &D)) {
			continue;
		}
		/* マップのドット座標 → 描画用の画面（落ち物パズルの DrawPuzzle と同じ） */
		sx = (DRAW_PARTS_X + 1) * 32 + (int)D.St.sFieldX - CamTileX () * 32;
		sy = (DRAW_PARTS_Y + 1) * 32 + (int)D.St.sFieldY - CamTileY () * 32;
		{
			int nExtra = (D.St.byIdle ? 0 : 110) + 6;
			if ((sx + PB_W + nExtra <= nX) || (sx - nExtra >= nX + SCRSIZEX) ||
				(sy + PB_VIEW_H + 6 <= nY) || (sy - 24 >= nY + SCRSIZEY)) {
				continue;
			}
		}
		s_nPzL = nX;
		s_nPzT = nY;
		s_nPzR = nX + SCRSIZEX;
		s_nPzB = nY + SCRSIZEY;
		if (hDC == NULL) {
			hDC = g_pDibDrawMap->Lock ();
			hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
			SetBkMode (hDC, TRANSPARENT);
		}
		DrawPinballOne (hDC, D, sx, sy, bTop);
	}
	if (hDC) {
		SelectObject (hDC, hFontOld);
		g_pDibDrawMap->Unlock ();
	}
}

/* ========================================================================= */
/* 関数名：DrawJan															 */
/* 内容：SBOじゃんの卓（見えている卓すべて。待機中の空卓・観戦も）の重ね描き				 */
/*		 卓の左上（nFieldX,nFieldY。マップのドット座標）・描く座標・画面外の切り取り・重なり順は	 */
/*		 落ち物パズル（DrawPuzzle）と同じ。2 層に分けて描く：							 */
/*		 下の層（bTop=FALSE。キャラより前＝地図のパーツの後）は卓の面・枠・座る場所の目印・	 */
/*		 手牌・鳴いた組・川（捨て牌）。上の層（bTop=TRUE。ウィンドウの下）は名前札・得点・	 */
/*		 山の残り・「上がり！」「流局」・座る場所の案内・座っている本人の手牌の別枠（画面下に固定）	 */
/*		 卓は 7×7 マス = 224 ドット四方。牌は 16 ドット（回転しない）。4 席とも同じ向きの格子で	 */
/*		 川を並べ、持ち主は枠の色と名前札の色で見分ける。							 */
/*		 配置（卓の左上が原点。JAN_BAND=20）：									 */
/*		   各辺に手牌の帯（席0=下 席1=右 席2=上 席3=左。牌の列は辺の中央そろえ）、			 */
/*		   内側 184 四方を 4 つの箱（席0=右下 席1=右上 席2=左上 席3=左下。92 四方）に分け、	 */
/*		   箱の上に名前札（高さ 14）、その下に川（5 列×4 行）。上の 2 箱は上詰め・下の 2 箱は下詰めにして、	 */
/*		   真ん中に帯（高さ 28）を空ける（山の残り・手番・案内・結果の文字）。				 */
/*		 牌の絵は 16 個の絵（s_aJanTileGrp。組＝id/4）。卓・牌の台・裏面・枠・ボタン・名前札などは	 */
/*		 SboGrpData/grp/janjan/jan_*.png の素材を透明度つきで重ねる（読めない物は単色＋縁の描画にもどる）	 */
/* ========================================================================= */
#define JAN_TBL			(224)		/* 卓の大きさ（7×7 マス） */
#define JAN_T			(16)		/* 卓の上の牌の大きさ */
#define JAN_BAND		(20)		/* 辺の手牌の帯の厚み（牌 16＋手前の線） */
#define JAN_BOX			(92)		/* 川の箱の大きさ（内側 184 四方の 4 分の 1） */
#define JAN_HEAD		(14)		/* 名前札の高さ */
#define JAN_RCOL		(5)			/* 川の列数 */
#define JAN_RROW		(4)			/* 川の行数 */
#define JAN_BIG			(32)		/* 自分の手牌の牌の大きさ */
#define JAN_PIC			(28)		/* 32 の台に重ねる牌の絵の大きさ */
#define JAN_BIG_STEP	(34)		/* 自分の手牌の牌の間隔 */
#define JAN_PANEL_W		(448)		/* 自分の手牌の別枠の幅（画面 480 の縦持ちでも収まる） */
#define JAN_PANEL_H		(124)		/* 別枠の高さ（素材は 96。上下の枠・文字・バー・操作・牌の間を 6 ドット以上あけるため縦に伸ばす） */
#define JAN_OP_H		(24)		/* 操作の行のボタンの高さ */
#define JAN_VIEW_DRAW_MAX	(16)	/* GetJanState の添字の数（SboCliRecvCmd.cpp の JAN_VIEW_MAX と同じ） */

/* 牌の絵の割り当て（牌id 0〜15 → SboGrpData/grp/char<ファイル番号>.png の<行>行目の「正面・立ち止まり」のコマ）。
   仮の割り当て。後で差し替える時はこの表だけを変える（組＝id/4。同じ組の 4 つは仲間の絵にする）。
   選んだ理由と一覧は docs/janjan.md の末尾。 */
typedef struct _JANTILEGRP {
	BYTE		byFile;				/* char<NN>.png の NN（1〜16） */
	BYTE		byRow;				/* その絵の行（0〜15。キャラの絵 ID の 16 で割った余り） */
} JANTILEGRP;
static const JANTILEGRP s_aJanTileGrp[JANJAN_KINDS] = {
	/* 組0：町の子どもたち */
	{ 1,  2}, { 1,  3}, { 1,  5}, { 1,  7},
	/* 組1：町のおとなたち */
	{ 1,  4}, { 1,  6}, { 1,  8}, { 1,  9},
	/* 組2：町の女の子・エルフ */
	{ 1, 10}, { 1, 11}, { 1, 13}, { 1, 14},
	/* 組3：プヨンなどのスライム・小さな魔物 */
	{ 5,  6}, { 5,  7}, { 5, 11}, { 5, 12},
};

/* 席の色（席0=赤 席1=青 席2=緑 席3=黄） */
static const COLORREF s_aclJanSeat[JANJAN_SEATS] = {
	RGB (235, 95, 85), RGB (95, 150, 245), RGB (100, 205, 115), RGB (240, 205, 75),
};

static CImg32 *s_pJanSheetR[4];		/* 16 ドット版を回した物（添字は回転 JAN_ROT_*。0 は使わない） */
static CImg32 *s_pJanSheet16;		/* 牌の絵 16 ドット版（横に 16 個。キャラの絵を 1/2 にしたもの） */
static CImg32 *s_pJanSheet32;		/* 牌の絵 32 ドット版（横に 16 個） */
static CImg32 *s_pJanSheet28;		/* 牌の絵 28 ドット版（32 の台の中央に重ねる。32 版を縮めたもの） */

/* ---- 見た目の素材（SboGrpData/grp/janjan/jan_*.png。透明度つき。読めない時は単色の描画にもどる） ---- */
BOOL ReadGrpDataRgba(LPSTR pszName, int *pnWidth, int *pnHeight, BYTE **ppRgba);		/* SboCliMain.cpp */

enum {
	JIMG_TABLE, JIMG_FRONT, JIMG_FRONT16, JIMG_BACK, JIMG_BACK16, JIMG_SEL, JIMG_PANEL,
	JIMG_BTN, JIMG_NAMEPLATE, JIMG_RIVERBOX, JIMG_SEATMARK, JIMG_BANNER, JIMG_TURN, JIMG_MAX
};
typedef struct _JANIMG {
	const char	*pszRes;			/* リソース名（SboGrpData.rc） */
	int			nW, nH;				/* 決まった大きさ（違う画像は使わない） */
	BYTE		*pRgba;				/* 読み込んだ絵（上の行から R,G,B,A）。NULL なら使えない */
} JANIMG;
static JANIMG s_aJanImg[JIMG_MAX] = {
	{"IDP_JAN_TABLE", 224, 224, NULL}, {"IDP_JAN_TILE_FRONT", 32, 32, NULL}, {"IDP_JAN_TILE_FRONT16", 16, 16, NULL},
	{"IDP_JAN_TILE_BACK", 32, 32, NULL}, {"IDP_JAN_TILE_BACK16", 16, 16, NULL}, {"IDP_JAN_TILE_SEL32", 36, 36, NULL},
	{"IDP_JAN_PANEL", 448, 96, NULL}, {"IDP_JAN_BTN", 192, 24, NULL}, {"IDP_JAN_NAMEPLATE", 92, 14, NULL},
	{"IDP_JAN_RIVERBOX", 92, 78, NULL}, {"IDP_JAN_SEATMARK", 26, 26, NULL}, {"IDP_JAN_BANNER", 200, 48, NULL},
	{"IDP_JAN_TURN", 16, 16, NULL},
};
#define JAN_NOTINT		((COLORREF)0xFFFFFFFF)		/* 色を掛けない */
/* 他席の牌の回転（牌の絵の上が卓の中央を向くように。時計回りに 90 度ずつ） */
#define JAN_ROT_0		(0)
#define JAN_ROT_CW		(1)		/* 右に 90 度（席3＝左の辺：絵の上が右＝中央を向く） */
#define JAN_ROT_180		(2)		/* 席2＝上の辺 */
#define JAN_ROT_CCW		(3)		/* 左に 90 度（席1＝右の辺：絵の上が左＝中央を向く） */
static const int s_anJanSeatRot[4] = { JAN_ROT_0, JAN_ROT_CCW, JAN_ROT_180, JAN_ROT_CW };	/* 席 0〜3 の牌の向き */

/* 素材を読む（最初に描く時に 1 回。1 枚ずつ、読めた物だけ使う） */
static void JanArtInit(void)
{
	static BOOL s_bTried = FALSE;
	int i, nW, nH;
	BYTE *pRgba;

	if (s_bTried) {
		return;
	}
	s_bTried = TRUE;
	for (i = 0; i < JIMG_MAX; i ++) {
		pRgba = NULL;
		if (!ReadGrpDataRgba ((LPSTR)s_aJanImg[i].pszRes, &nW, &nH, &pRgba)) {
			continue;
		}
		if ((nW != s_aJanImg[i].nW) || (nH != s_aJanImg[i].nH)) {
			delete [] pRgba;		/* 大きさが違う素材は使わない（単色の描画にもどる） */
			continue;
		}
		s_aJanImg[i].pRgba = pRgba;
	}
}

/* 素材 nImg の (sx,sy,sw,sh) を、画面 (dx,dy) へ幅 dw に伸ばして透明度つきで重ねる（切り取り範囲の中だけ）。
   nCap > 0 なら左右 nCap ドットはそのまま、真ん中だけ伸ばす（3 分割）。
   clTint を指定すると nTint(0〜255) の強さで色を掛ける（乗算。白〜灰色の絵に色をつける）。nOpa は全体の不透明度 */
static void JanArtEx(int nImg, int dx, int dy, int sx, int sy, int sw, int sh, int dw, int nCap, COLORREF clTint, int nTint, int nOpa)
{
	const JANIMG &Im = s_aJanImg[nImg];
	CImg32 *pDst = g_pDibDrawMap;
	int xx, yy, x0, x1, y0, y1, nDW, nDH, nSrcX, nSrcY, a, tr, tg, tb;
	BYTE *pBits, *pS, *pD;

	if ((Im.pRgba == NULL) || (pDst == NULL) || (dw <= 0)) {
		return;
	}
	nDW = pDst->Width ();
	nDH = pDst->Height ();
	pBits = pDst->GetBits ();
	x0 = max (max (dx, s_nPzL), 0);
	x1 = min (min (dx + dw, s_nPzR), nDW);
	y0 = max (max (dy, s_nPzT), 0);
	y1 = min (min (dy + sh, s_nPzB), nDH);
	if (clTint != JAN_NOTINT) {
		/* 乗算の強さ nTint：255 で完全に掛ける、0 で掛けない */
		tr = 255 - nTint + GetRValue (clTint) * nTint / 255;
		tg = 255 - nTint + GetGValue (clTint) * nTint / 255;
		tb = 255 - nTint + GetBValue (clTint) * nTint / 255;
	} else {
		tr = tg = tb = 255;
	}
	for (yy = y0; yy < y1; yy ++) {
		nSrcY = sy + (yy - dy);
		for (xx = x0; xx < x1; xx ++) {
			if ((nCap > 0) && (dw != sw)) {
				if (xx - dx < nCap) {
					nSrcX = sx + (xx - dx);
				} else if (xx - dx >= dw - nCap) {
					nSrcX = sx + sw - (dx + dw - xx);
				} else {
					nSrcX = sx + nCap + (xx - dx - nCap) * (sw - nCap * 2) / max (dw - nCap * 2, 1);
				}
			} else {
				nSrcX = sx + (xx - dx);
			}
			pS = Im.pRgba + (nSrcY * Im.nW + nSrcX) * 4;
			a = pS[3] * nOpa / 255;
			if (a <= 0) {
				continue;
			}
			pD = pBits + ((nDH - 1 - yy) * nDW + xx) * 4;		/* 下の行から並ぶ B,G,R,X */
			pD[0] = (BYTE)((pD[0] * (255 - a) + (pS[2] * tb / 255) * a) / 255);
			pD[1] = (BYTE)((pD[1] * (255 - a) + (pS[1] * tg / 255) * a) / 255);
			pD[2] = (BYTE)((pD[2] * (255 - a) + (pS[0] * tr / 255) * a) / 255);
		}
	}
}

/* 素材をそのまま重ねる */
static void JanArt(int nImg, int dx, int dy, COLORREF clTint, int nTint, int nOpa)
{
	JanArtEx (nImg, dx, dy, 0, 0, s_aJanImg[nImg].nW, s_aJanImg[nImg].nH, s_aJanImg[nImg].nW, 0, clTint, nTint, nOpa);
}

/* 回転 nRot（JAN_ROT_*）した 16×16 の絵の (x,y) に入る元の絵の位置 */
static void JanRotSrc(int nRot, int x, int y, int *pnSx, int *pnSy)
{
	switch (nRot) {
	case JAN_ROT_CW:	*pnSx = y;				*pnSy = JAN_T - 1 - x;	break;		/* 右に 90 度 */
	case JAN_ROT_180:	*pnSx = JAN_T - 1 - x;	*pnSy = JAN_T - 1 - y;	break;
	case JAN_ROT_CCW:	*pnSx = JAN_T - 1 - y;	*pnSy = x;				break;		/* 左に 90 度 */
	default:			*pnSx = x;				*pnSy = y;				break;
	}
}

/* 16×16 の素材を nRot 回して重ねる（回転なしは JanArt と同じ。色は掛けない） */
static void JanArtRot16(int nImg, int dx, int dy, int nRot)
{
	const JANIMG &Im = s_aJanImg[nImg];
	CImg32 *pDst = g_pDibDrawMap;
	int xx, yy, nSx, nSy, a, nDW, nDH;
	BYTE *pBits, *pS, *pD;

	if (nRot == JAN_ROT_0) {
		JanArt (nImg, dx, dy, JAN_NOTINT, 0, 255);
		return;
	}
	if ((Im.pRgba == NULL) || (pDst == NULL)) {
		return;
	}
	nDW = pDst->Width ();
	nDH = pDst->Height ();
	pBits = pDst->GetBits ();
	for (yy = 0; yy < JAN_T; yy ++) {
		for (xx = 0; xx < JAN_T; xx ++) {
			if ((dx + xx < max (s_nPzL, 0)) || (dx + xx >= min (s_nPzR, nDW)) || (dy + yy < max (s_nPzT, 0)) || (dy + yy >= min (s_nPzB, nDH))) {
				continue;
			}
			JanRotSrc (nRot, xx, yy, &nSx, &nSy);
			pS = Im.pRgba + (nSy * Im.nW + nSx) * 4;
			a = pS[3];
			if (a <= 0) {
				continue;
			}
			pD = pBits + ((nDH - 1 - (dy + yy)) * nDW + dx + xx) * 4;
			pD[0] = (BYTE)((pD[0] * (255 - a) + pS[2] * a) / 255);
			pD[1] = (BYTE)((pD[1] * (255 - a) + pS[1] * a) / 255);
			pD[2] = (BYTE)((pD[2] * (255 - a) + pS[0] * a) / 255);
		}
	}
}

/* 正方形の素材を nRot 回して、色（clTint）を掛けて重ねる（座る場所の目印の矢印用。席の方向＝卓の中央を向く） */
static void JanArtRotTint(int nImg, int dx, int dy, int nRot, COLORREF clTint, int nOpa)
{
	const JANIMG &Im = s_aJanImg[nImg];
	CImg32 *pDst = g_pDibDrawMap;
	int xx, yy, nSx, nSy, a, nDW, nDH, nN = Im.nW, tr, tg, tb;
	BYTE *pBits, *pS, *pD;

	if ((Im.pRgba == NULL) || (pDst == NULL) || (Im.nW != Im.nH)) {
		return;
	}
	tr = GetRValue (clTint);
	tg = GetGValue (clTint);
	tb = GetBValue (clTint);
	nDW = pDst->Width ();
	nDH = pDst->Height ();
	pBits = pDst->GetBits ();
	for (yy = 0; yy < nN; yy ++) {
		for (xx = 0; xx < nN; xx ++) {
			if ((dx + xx < max (s_nPzL, 0)) || (dx + xx >= min (s_nPzR, nDW)) || (dy + yy < max (s_nPzT, 0)) || (dy + yy >= min (s_nPzB, nDH))) {
				continue;
			}
			switch (nRot) {
			case JAN_ROT_CW:	nSx = yy;			nSy = nN - 1 - xx;	break;
			case JAN_ROT_180:	nSx = nN - 1 - xx;	nSy = nN - 1 - yy;	break;
			case JAN_ROT_CCW:	nSx = nN - 1 - yy;	nSy = xx;			break;
			default:			nSx = xx;			nSy = yy;			break;
			}
			pS = Im.pRgba + (nSy * Im.nW + nSx) * 4;
			a = pS[3] * nOpa / 255;
			if (a <= 0) {
				continue;
			}
			pD = pBits + ((nDH - 1 - (dy + yy)) * nDW + dx + xx) * 4;
			pD[0] = (BYTE)((pD[0] * (255 - a) + (pS[2] * tb / 255) * a) / 255);
			pD[1] = (BYTE)((pD[1] * (255 - a) + (pS[1] * tg / 255) * a) / 255);
			pD[2] = (BYTE)((pD[2] * (255 - a) + (pS[0] * tr / 255) * a) / 255);
		}
	}
}

/* 牌の絵の一覧を作る（最初に描く時に 1 回）。キャラの絵は 2 倍に拡大して持っているので、
   16 ドット版は 1 ドットおきに取り出す。取り出せなければ FALSE（その間は牌の絵なしで描く） */
static BOOL JanSheetInit(void)
{
	int i, xx, yy, no, sx, sy;
	CImg32 *p16, *p32, *p28;

	if (s_pJanSheet16 && s_pJanSheet32 && s_pJanSheet28) {
		return TRUE;
	}
	if ((g_apDibCharBack == NULL) || (g_wCountCharGrp < 16)) {
		return FALSE;
	}
	p16 = new CImg32;
	p32 = new CImg32;
	p28 = new CImg32;
	if (!p16->Create (JANJAN_KINDS * JAN_T, JAN_T) || !p32->Create (JANJAN_KINDS * JAN_BIG, JAN_BIG)
			|| !p28->Create (JANJAN_KINDS * JAN_PIC, JAN_PIC)) {
		delete p16;
		delete p32;
		delete p28;
		return FALSE;
	}
	p16->Clear ();
	p32->Clear ();
	p28->Clear ();
	for (i = 0; i < JANJAN_KINDS; i ++) {
		no = (int)s_aJanTileGrp[i].byFile - 1;
		if ((no < 0) || (no >= g_wCountCharGrp) || (g_apDibCharBack[no] == NULL)) {
			continue;
		}
		sx = 1 * 64;							/* 向き「下」の 1 コマ目（立ち止まり） */
		sy = 32 * (int)s_aJanTileGrp[i].byRow;
		p32->Blt (i * JAN_BIG, 0, JAN_BIG, JAN_BIG, g_apDibCharBack[no], sx, sy, TRUE);
		for (yy = 0; yy < JAN_T; yy ++) {
			for (xx = 0; xx < JAN_T; xx ++) {
				p16->Blt (i * JAN_T + xx, yy, 1, 1, g_apDibCharBack[no], sx + xx * 2, sy + yy * 2, TRUE);
			}
		}
	}
	/* 28 ドット版：32 版を 1 ドットずつ取り出して縮める（台の中央に収める） */
	for (i = 0; i < JANJAN_KINDS; i ++) {
		for (yy = 0; yy < JAN_PIC; yy ++) {
			for (xx = 0; xx < JAN_PIC; xx ++) {
				p28->Blt (i * JAN_PIC + xx, yy, 1, 1, p32, i * JAN_BIG + xx * JAN_BIG / JAN_PIC, yy * JAN_BIG / JAN_PIC, TRUE);
			}
		}
	}
	/* 他席の牌の向き用に、16 ドット版を 90 度単位で回した物を作る（最近傍の回転なのでドットは崩れない） */
	for (i = 1; i < 4; i ++) {
		CImg32 *pR = new CImg32;
		int nSrcX, nSrcY, t;

		if (!pR->Create (JANJAN_KINDS * JAN_T, JAN_T)) {
			delete pR;
			continue;
		}
		pR->Clear ();
		for (t = 0; t < JANJAN_KINDS; t ++) {
			for (yy = 0; yy < JAN_T; yy ++) {
				for (xx = 0; xx < JAN_T; xx ++) {
					JanRotSrc (i, xx, yy, &nSrcX, &nSrcY);
					pR->Blt (t * JAN_T + xx, yy, 1, 1, p16, t * JAN_T + nSrcX, nSrcY, TRUE);
				}
			}
		}
		s_pJanSheetR[i] = pR;
	}
	s_pJanSheet16 = p16;
	s_pJanSheet32 = p32;
	s_pJanSheet28 = p28;
	return TRUE;
}

/* 切り取り範囲（s_nPz*）の中だけ、牌の絵を貼る */
static void JanBlt(int x, int y, int w, int h, CImg32 *pSrc, int sx, int sy)
{
	int x1 = x + w, y1 = y + h;

	if (pSrc == NULL) {
		return;
	}
	if (x < s_nPzL) {
		sx += s_nPzL - x;
		x = s_nPzL;
	}
	if (y < s_nPzT) {
		sy += s_nPzT - y;
		y = s_nPzT;
	}
	x1 = min (x1, s_nPzR);
	y1 = min (y1, s_nPzB);
	if ((x1 <= x) || (y1 <= y)) {
		return;
	}
	g_pDibDrawMap->Blt (x, y, x1 - x, y1 - y, pSrc, sx, sy, TRUE);
}

/* 四角を塗る */
static void JanRect(HDC hDC, int x, int y, int w, int h, COLORREF cl, int nAlpha)
{
	int yy;

	if ((y >= s_nPzB) || (y + h <= s_nPzT) || (x >= s_nPzR) || (x + w <= s_nPzL)) {
		return;
	}
	for (yy = max (0, s_nPzT - y); yy < h; yy ++) {
		if (y + yy >= s_nPzB) {
			break;
		}
		PuzSpan (hDC, x, y + yy, w, cl, nAlpha);
	}
}

/* 1 ドットの縁 */
static void JanEdge(HDC hDC, int x, int y, int w, int h, COLORREF cl)
{
	int yy;

	if ((y >= s_nPzB) || (y + h <= s_nPzT) || (x >= s_nPzR) || (x + w <= s_nPzL)) {
		return;
	}
	PuzSpan (hDC, x, y, w, cl, 255);
	PuzSpan (hDC, x, y + h - 1, w, cl, 255);
	for (yy = max (1, s_nPzT - y); yy < h - 1; yy ++) {
		if (y + yy >= s_nPzB) {
			break;
		}
		PuzSpan (hDC, x, y + yy, 1, cl, 255);
		PuzSpan (hDC, x + w - 1, y + yy, 1, cl, 255);
	}
}

/* 右向きの小さな三角（手番の目印） */
static void JanTriRight(HDC hDC, int x, int y, int h, COLORREF cl)
{
	int yy, w;

	for (yy = 0; yy < h; yy ++) {
		w = (yy <= h / 2) ? (yy + 1) : (h - yy);
		JanRect (hDC, x, y + yy, w, 1, cl, 255);
	}
}

/* 下向きの小さな三角（手牌のカーソルの目印） */
static void JanTriDown(HDC hDC, int x, int y, int w, COLORREF cl)
{
	int xx, h;

	for (xx = 0; xx < w; xx ++) {
		h = (xx <= w / 2) ? (xx + 1) : (w - xx);
		JanRect (hDC, x + xx, y, 1, h, cl, 255);
	}
}

/* 卓の上の牌（16 ドット）。nTile が範囲外なら絵なし。clEdge は縁の色 */
static void JanTileSmall(HDC hDC, int x, int y, int nTile, COLORREF clEdge, BOOL bMark = FALSE, int nRot = JAN_ROT_0)
{
	BOOL bArt = (s_aJanImg[JIMG_FRONT16].pRgba != NULL) ? TRUE : FALSE;

	if (bArt) {
		JanArtRot16 (JIMG_FRONT16, x, y, nRot);
	} else {
		JanRect (hDC, x, y, JAN_T, JAN_T, RGB (245, 240, 220), 255);
	}
	if ((nTile >= 0) && (nTile < JANJAN_KINDS)) {
		JanBlt (x, y, JAN_T, JAN_T, ((nRot != JAN_ROT_0) && s_pJanSheetR[nRot]) ? s_pJanSheetR[nRot] : s_pJanSheet16, nTile * JAN_T, 0);
	}
	/* 素材の台には縁があるので、色の縁は鳴いた組の目印（bMark）の時だけ重ねる */
	if (!bArt || bMark) {
		JanEdge (hDC, x, y, JAN_T, JAN_T, clEdge);
	}
}

/* 卓の上の牌の裏面（16 ドット） */
static void JanTileBackSmall(HDC hDC, int x, int y, int nRot = JAN_ROT_0)
{
	if (s_aJanImg[JIMG_BACK16].pRgba != NULL) {
		JanArtRot16 (JIMG_BACK16, x, y, nRot);
		return;
	}
	JanRect (hDC, x, y, JAN_T, JAN_T, RGB (55, 70, 160), 255);
	JanEdge (hDC, x + 3, y + 3, JAN_T - 6, JAN_T - 6, RGB (120, 135, 215));
	JanEdge (hDC, x, y, JAN_T, JAN_T, RGB (200, 205, 235));
}

/* 自分の手牌の牌（32 ドット） */
static void JanTileBig(HDC hDC, int x, int y, int nTile)
{
	if (s_aJanImg[JIMG_FRONT].pRgba != NULL) {
		/* 素材の台（32）の中央に、牌の絵（28）を重ねる */
		JanArt (JIMG_FRONT, x, y, JAN_NOTINT, 0, 255);
		if ((nTile >= 0) && (nTile < JANJAN_KINDS)) {
			JanBlt (x + (JAN_BIG - JAN_PIC) / 2, y + (JAN_BIG - JAN_PIC) / 2, JAN_PIC, JAN_PIC, s_pJanSheet28, nTile * JAN_PIC, 0);
		}
		return;
	}
	JanRect (hDC, x, y, JAN_BIG, JAN_BIG, RGB (245, 240, 220), 255);
	if ((nTile >= 0) && (nTile < JANJAN_KINDS)) {
		JanBlt (x, y, JAN_BIG, JAN_BIG, s_pJanSheet32, nTile * JAN_BIG, 0);
	}
	JanEdge (hDC, x, y, JAN_BIG, JAN_BIG, RGB (90, 70, 40));
}

/* 席の手牌の行（列）の並び：手牌（引いた牌は少し離す）→ 鳴いた組（公開）。
   aHandOff / aMeldOff は帯の先頭からの距離（ドット）。戻り値は全体の長さ */
static int JanStripLayout(const JANSEATWIRE &Seat, BOOL bGap, int nStep, int nGap, int nMeldGap, int *aHandOff, int *aMeldOff)
{
	int i, m, k, nAlong = 0, nHand = min ((int)Seat.byHandNum, JAN_HAND_WIRE), nMeld = min ((int)Seat.byMeldNum, JAN_MELD_WIRE);

	for (i = 0; i < nHand; i ++) {
		if (bGap && (i == nHand - 1) && (nHand > 1)) {
			nAlong += nGap;
		}
		aHandOff[i] = nAlong;
		nAlong += nStep;
	}
	if (nMeld > 0) {
		nAlong += nMeldGap;
	}
	for (m = 0; m < nMeld; m ++) {
		for (k = 0; k < JANJAN_SET_SIZE; k ++) {
			aMeldOff[m * JANJAN_SET_SIZE + k] = nAlong;
			nAlong += nStep;
		}
		if (m + 1 < nMeld) {
			nAlong += 3;
		}
	}
	return nAlong;
}

/* 席の手牌の帯の先頭の位置（卓の左上からの距離）。席0=下 席1=右 席2=上 席3=左。牌の列は辺の中央そろえ */
static void JanStripOrigin(int nSeat, int nLen, int *pnX, int *pnY, BOOL *pbHoriz)
{
	switch (nSeat) {
	case 0:		*pnX = (JAN_TBL - nLen) / 2;	*pnY = JAN_TBL - JAN_BAND + 2;	*pbHoriz = TRUE;	break;
	case 1:		*pnX = JAN_TBL - JAN_BAND + 2;	*pnY = (JAN_TBL - nLen) / 2;	*pbHoriz = FALSE;	break;
	case 2:		*pnX = (JAN_TBL - nLen) / 2;	*pnY = 2;						*pbHoriz = TRUE;	break;
	default:	*pnX = 2;						*pnY = (JAN_TBL - nLen) / 2;	*pbHoriz = FALSE;	break;
	}
}

/* 川の箱の左上（卓の左上からの距離）。席0=右下 席1=右上 席2=左上 席3=左下。上の箱は上詰め・下の箱は下詰め */
static void JanBoxOrigin(int nSeat, int *pnX, int *pnY)
{
	const int nInner = JAN_TBL - JAN_BAND * 2;				/* 184 */
	const int nH = JAN_HEAD + JAN_RROW * JAN_T;				/* 箱の中身の高さ 78 */

	*pnX = ((nSeat == 0) || (nSeat == 1)) ? (JAN_BAND + nInner / 2) : JAN_BAND;
	*pnY = ((nSeat == 1) || (nSeat == 2)) ? JAN_BAND : (JAN_TBL - JAN_BAND - nH);
}

/* 席名を幅に収まるように縮める（全角の途中で切らない） */
static void JanFitText(HDC hDC, const char *pszSrc, int nMaxW, char *pszOut, int nOutSize)
{
	int nLen = (int)strlen (pszSrc);
	SIZE size;

	if (nLen > nOutSize - 1) {
		nLen = nOutSize - 1;
	}
	for (;;) {
		memcpy (pszOut, pszSrc, nLen);
		pszOut[nLen] = '\0';
		GetTextExtentPoint32 (hDC, pszOut, nLen, &size);
		if ((size.cx <= nMaxW) || (nLen <= 0)) {
			break;
		}
		/* 末尾を 1 文字（全角なら 2 バイト）落とす */
		{
			int i = 0, nLast = 0;
			while (i < nLen) {
				nLast = i;
				i += (((BYTE)pszOut[i] >= 0x81 && (BYTE)pszOut[i] <= 0x9F) || ((BYTE)pszOut[i] >= 0xE0 && (BYTE)pszOut[i] <= 0xFC)) ? 2 : 1;
			}
			nLen = nLast;
		}
	}
}

/* 卓 1 つ。sx, sy は画面上の卓の左上。nMyX, nMyY は自分のマス（座る場所の案内用） */
static void DrawJanOne(HDC hDC, const JANDRAW &D, int sx, int sy, int nMyX, int nMyY, BOOL bTop)
{
	const CMDPARA_JANSTATE &St = D.St;
	BOOL bIdle = (St.byPhase == JANPHASE_IDLE) ? TRUE : FALSE;
	BOOL bOver = (St.byPhase == JANPHASE_OVER) ? TRUE : FALSE;
	BOOL bRecruit = (St.byPhase == JANPHASE_RECRUIT) ? TRUE : FALSE;		/* 参加募集中（残りはサーバーの dwLimitMs をクライアントが減らす） */
	DWORD dwNow = timeGetTime ();
	BOOL bBlink = (((dwNow / 400) & 1) == 0) ? TRUE : FALSE;
	int s, i, nX, nY, nLen, nDx, nDy, aHandOff[JAN_HAND_WIRE + 1], aMeldOff[JAN_MELD_WIRE * JANJAN_SET_SIZE + 1];
	BOOL bHoriz;

	if (!bTop) {
		/* ---- 下の層：卓の面・枠 ---- */
		if (s_aJanImg[JIMG_TABLE].pRgba != NULL) {
			/* 卓の素材（面は明るいので、真ん中の帯だけ暗く敷いて白い文字を読めるようにする） */
			JanArt (JIMG_TABLE, sx, sy, JAN_NOTINT, 0, 255);
			JanRect (hDC, sx + JAN_BAND, sy + JAN_BAND + JAN_HEAD + JAN_RROW * JAN_T + 1, JAN_TBL - JAN_BAND * 2,
				JAN_TBL - (JAN_BAND * 2) - (JAN_HEAD + JAN_RROW * JAN_T) * 2 - 2, RGB (20, 45, 32), 190);
		} else {
			JanRect (hDC, sx, sy, JAN_TBL, JAN_TBL, RGB (26, 88, 56), 242);
			JanRect (hDC, sx + JAN_BAND, sy + JAN_BAND + JAN_HEAD + JAN_RROW * JAN_T + 1, JAN_TBL - JAN_BAND * 2,
				JAN_TBL - (JAN_BAND * 2) - (JAN_HEAD + JAN_RROW * JAN_T) * 2 - 2, RGB (0, 0, 0), 55);		/* 真ん中の帯 */
			JanEdge (hDC, sx, sy, JAN_TBL, JAN_TBL, RGB (80, 50, 25));
			JanEdge (hDC, sx + 1, sy + 1, JAN_TBL - 2, JAN_TBL - 2, RGB (170, 125, 60));
		}

		/* 座る場所の目印（卓の外の 4 マス。誰かが立っていればその下に敷かれる） */
		for (s = 0; s < JANJAN_SEATS; s ++) {
			JanjanSeatOffset (s, &nDx, &nDy);
			nX = sx + (JANJAN_TABLE_HALF + nDx) * 32;
			nY = sy + (JANJAN_TABLE_HALF + nDy) * 32;
			if (s_aJanImg[JIMG_SEATMARK].pRgba != NULL) {
				JanArtRotTint (JIMG_SEATMARK, nX + 3, nY + 3, s_anJanSeatRot[s & 3], s_aclJanSeat[s], (bIdle || bRecruit) ? 255 : 190);		/* 白〜灰色の絵に席の色を掛ける */
			} else {
				JanRect (hDC, nX + 3, nY + 3, 26, 26, s_aclJanSeat[s], (bIdle || bRecruit) ? 90 : 60);
				JanEdge (hDC, nX + 3, nY + 3, 26, 26, s_aclJanSeat[s]);
			}
		}

		/* 各席の川・手牌・鳴いた組 */
		for (s = 0; s < JANJAN_SEATS; s ++) {
			const JANSEATWIRE &Seat = St.aSeat[s];
			COLORREF clSeat = s_aclJanSeat[s];
			int nHand = min ((int)Seat.byHandNum, JAN_HAND_WIRE), nMeld = min ((int)Seat.byMeldNum, JAN_MELD_WIRE);
			int nRiver = min ((int)Seat.byRiverNum, JAN_RIVER_WIRE), nFrom, bx, by;
			BOOL bGap = (!bIdle && !bRecruit && (St.byLogicPhase == JANJAN_PHASE_DISCARD) && (St.byTurn == s) && (nHand % 3 == 0)) ? TRUE : FALSE;
			BOOL bTurn = (!bIdle && !bRecruit && !bOver && (St.byTurn == s) && (St.byLogicPhase != JANJAN_PHASE_END)) ? TRUE : FALSE;

			/* 川の箱の縁（手番の席は黄色で強調） */
			JanBoxOrigin (s, &bx, &by);
			if (s_aJanImg[JIMG_RIVERBOX].pRgba != NULL) {
				/* 箱の素材（92×78）に席の色を薄く掛ける。手番の席は黄色の線を内側に足す */
				JanArt (JIMG_RIVERBOX, sx + bx, sy + by, clSeat, bTurn ? 90 : 150, 255);
				if (bTurn) {
					JanEdge (hDC, sx + bx + 1, sy + by + 1, JAN_BOX - 2, JAN_HEAD + JAN_RROW * JAN_T - 2, RGB (255, 240, 120));
				}
			} else {
				JanEdge (hDC, sx + bx, sy + by, JAN_BOX, JAN_HEAD + JAN_RROW * JAN_T, bTurn ? RGB (255, 240, 120) : clSeat);
			}

			if (Seat.byState == JANSEAT_EMPTY) {
				/* 空席：手牌の置き場所だけうすく見せる */
				nLen = JANJAN_HAND * JAN_T;
				JanStripOrigin (s, nLen, &nX, &nY, &bHoriz);
				for (i = 0; i < 8; i ++) {
					JanEdge (hDC, sx + nX + (bHoriz ? i * JAN_T : 0), sy + nY + (bHoriz ? 0 : i * JAN_T), JAN_T, JAN_T, RGB (60, 120, 80));
				}
				continue;
			}

			/* 手牌の帯（内側に席の色の線。手番の席は太く黄色） */
			nLen = JanStripLayout (Seat, bGap, JAN_T, 4, 6, aHandOff, aMeldOff);
			JanStripOrigin (s, nLen, &nX, &nY, &bHoriz);
			{
				COLORREF clBar = bTurn ? RGB (255, 240, 120) : clSeat;
				int nBarW = bTurn ? 3 : 2;
				if (bHoriz) {
					int by2 = (s == 0) ? (sy + nY - nBarW) : (sy + nY + JAN_T);
					JanRect (hDC, sx + nX - 2, by2, nLen + 4, nBarW, clBar, 255);
				} else {
					int bx2 = (s == 3) ? (sx + nX + JAN_T) : (sx + nX - nBarW);
					JanRect (hDC, bx2, sy + nY - 2, nBarW, nLen + 4, clBar, 255);
				}
			}
			for (i = 0; i < nHand; i ++) {
				int tx = sx + nX + (bHoriz ? aHandOff[i] : 0), ty = sy + nY + (bHoriz ? 0 : aHandOff[i]);
				if (Seat.abyHand[i] < JANJAN_KINDS) {
					JanTileSmall (hDC, tx, ty, Seat.abyHand[i], RGB (90, 70, 40), FALSE, s_anJanSeatRot[s & 3]);
				} else {
					JanTileBackSmall (hDC, tx, ty, s_anJanSeatRot[s & 3]);
				}
			}
			for (i = 0; i < nMeld * JANJAN_SET_SIZE; i ++) {
				int tx = sx + nX + (bHoriz ? aMeldOff[i] : 0), ty = sy + nY + (bHoriz ? 0 : aMeldOff[i]);
				JanTileSmall (hDC, tx, ty, Seat.aMeld[i / JANJAN_SET_SIZE].abyTile[i % JANJAN_SET_SIZE],
					(Seat.aMeld[i / JANJAN_SET_SIZE].byKind == JANJAN_MELD_SOROI) ? RGB (60, 160, 70) : RGB (70, 110, 220), TRUE, s_anJanSeatRot[s & 3]);
			}

			/* 川（捨て牌。上向きのまま、4 席とも左から右・上から下の同じ向き。多い時は新しい方を残す） */
			nFrom = max (0, nRiver - JAN_RCOL * JAN_RROW);
			for (i = nFrom; i < nRiver; i ++) {
				int nIdx = i - nFrom;
				int tx = sx + bx + (JAN_BOX - JAN_RCOL * JAN_T) / 2 + (nIdx % JAN_RCOL) * JAN_T;
				int ty = sy + by + JAN_HEAD + (nIdx / JAN_RCOL) * JAN_T;
				JanTileSmall (hDC, tx, ty, Seat.abyRiver[i], clSeat);		/* 素材の台の時、席の色は箱の色で見せる */
				/* 最後の捨て牌（ロン・鳴きの対象。ロンで終わった時は上がり牌）を目立たせる */
				if ((i == nRiver - 1) && (St.byLastTile < JANJAN_KINDS) && (St.byLastSeat == s) && (Seat.abyRiver[i] == St.byLastTile)) {
					COLORREF clHi = bBlink ? RGB (255, 245, 90) : RGB (255, 255, 255);
					JanEdge (hDC, tx, ty, JAN_T, JAN_T, clHi);
					JanEdge (hDC, tx + 1, ty + 1, JAN_T - 2, JAN_T - 2, clHi);
					g_bDraw = TRUE;
				}
			}
		}
		return;
	}

	/* ---- 上の層：名前札・得点・真ん中の帯・結果・案内・自分の手牌の別枠 ---- */
	{
		int nBandY = sy + JAN_BAND + JAN_HEAD + JAN_RROW * JAN_T;		/* 真ん中の帯の上端 */
		int nBandH = JAN_TBL - JAN_BAND * 2 - (JAN_HEAD + JAN_RROW * JAN_T) * 2;
		int nBandCX = sx + JAN_TBL / 2;
		char szTmp[64], szFit[JAN_NAME_LEN + 4];

		/* 名前札と得点 */
		for (s = 0; s < JANJAN_SEATS; s++) {
			const JANSEATWIRE &Seat = St.aSeat[s];
			BOOL bTurn = (!bIdle && !bRecruit && !bOver && (St.byTurn == s) && (St.byLogicPhase != JANJAN_PHASE_END)) ? TRUE : FALSE;
			BOOL bWin = (bOver && (St.byResult == JANJAN_RESULT_WIN) && (St.byWinner == s)) ? TRUE : FALSE;
			int bx, by, nNameW, nScoreW = 0;
			SIZE size;

			JanBoxOrigin (s, &bx, &by);
			bx += sx;
			by += sy;
			BOOL bPlate = (s_aJanImg[JIMG_NAMEPLATE].pRgba != NULL) ? TRUE : FALSE;
			if (Seat.byState == JANSEAT_EMPTY) {
				if (bPlate) {
					JanArt (JIMG_NAMEPLATE, bx, by, RGB (110, 110, 110), 255, 255);
				} else {
					JanRect (hDC, bx, by, JAN_BOX, JAN_HEAD, RGB (90, 90, 90), 110);
				}
				PuzText (hDC, bx + (bPlate ? 11 : 4), by, (LPSTR)"空席", RGB (225, 225, 225), JAN_HEAD, FALSE);
				continue;
			}
			if (bPlate) {
				/* 名前札の素材に席の色を掛ける。字は明るい色＋黒縁（席の色は札の下地で見せる）。手番の席は左端に目印を出す */
				JanArt (JIMG_NAMEPLATE, bx, by, s_aclJanSeat[s], 215, 255);
				if (bTurn && (s_aJanImg[JIMG_TURN].pRgba != NULL)) {
					JanArt (JIMG_TURN, bx - 1, by - 1, JAN_NOTINT, 0, 255);
				}
			} else {
				JanRect (hDC, bx, by, JAN_BOX, JAN_HEAD, s_aclJanSeat[s], bTurn ? 245 : 140);
				if (bTurn) {
					JanTriRight (hDC, bx + 2, by + 3, 8, RGB (40, 30, 0));
				}
			}
			/* 得点（右）と名前（左。得点の分を空けて縮める） */
			wsprintf (szTmp, "%d", (int)Seat.sScore);
			GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
			nScoreW = size.cx;
			if (bPlate) {
				/* 札の両端の飾りを避ける（手番の時は左の目印の分も空ける） */
				int nL = bTurn ? 16 : 9;
				nNameW = JAN_BOX - nL - 9 - nScoreW - 3;
				JanFitText (hDC, D.aszName[s], nNameW, szFit, sizeof (szFit));
				PuzText (hDC, bx + nL, by, szFit, RGB (255, 250, 225), JAN_HEAD, FALSE);
				PuzText (hDC, bx + JAN_BOX - 9 - nScoreW, by, szTmp, RGB (255, 240, 170), JAN_HEAD, FALSE);
			} else {
				nNameW = JAN_BOX - (bTurn ? 12 : 5) - nScoreW - 6;
				JanFitText (hDC, D.aszName[s], nNameW, szFit, sizeof (szFit));
				PuzText (hDC, bx + (bTurn ? 11 : 4), by, szFit, bTurn ? RGB (20, 20, 20) : RGB (255, 255, 255), JAN_HEAD, FALSE);
				PuzText (hDC, bx + JAN_BOX - 3 - nScoreW, by, szTmp, bTurn ? RGB (20, 20, 20) : RGB (255, 245, 170), JAN_HEAD, FALSE);
			}
			if (bWin) {
				/* 上がった席は金色の枠を点滅 */
				COLORREF clWin = bBlink ? RGB (255, 225, 60) : RGB (255, 255, 255);
				JanEdge (hDC, bx - 1, by - 1, JAN_BOX + 2, JAN_HEAD + JAN_RROW * JAN_T + 2, clWin);
				JanEdge (hDC, bx, by, JAN_BOX, JAN_HEAD + JAN_RROW * JAN_T, clWin);
				g_bDraw = TRUE;
			}
		}

		/* 真ん中の帯：山の残りと手番（待機中は案内） */
		if (bIdle) {
			PuzText (hDC, nBandCX, nBandY, (LPSTR)"SBOじゃん", RGB (255, 240, 150), 14, TRUE);
			if ((St.byIdle & JANIDLE_SEAT) == 0) {
				PuzText (hDC, nBandCX, nBandY + 14, (LPSTR)"空き卓", RGB (210, 230, 210), 14, TRUE);
			}
		} else if (bRecruit) {
			/* 参加募集中：残り時間（座れる席の座る場所に立っている人には、下で案内を点滅させる） */
			if (!((St.byIdle & JANIDLE_SEAT) && bBlink)) {
				PuzText (hDC, nBandCX, nBandY, (LPSTR)"参加者を募集中", RGB (255, 240, 150), 14, TRUE);
				if (D.nLeftMs > 0) {
					wsprintf (szTmp, "残り %d 秒", (D.nLeftMs + 999) / 1000);
				} else {
					strcpy (szTmp, "まもなく開始");
				}
				PuzText (hDC, nBandCX, nBandY + 14, szTmp, RGB (255, 255, 255), 14, TRUE);
			}
		} else if (!bOver) {
			wsprintf (szTmp, "山 %d", (int)St.byPileLeft);
			PuzText (hDC, nBandCX, nBandY, szTmp, RGB (255, 255, 255), 14, TRUE);
			if (St.byTurn < JANJAN_SEATS) {
				JanFitText (hDC, D.aszName[St.byTurn], JAN_TBL - JAN_BAND * 2 - 40, szFit, sizeof (szFit));
				wsprintf (szTmp, "%sの番", szFit);
				PuzText (hDC, nBandCX, nBandY + 14, szTmp, s_aclJanSeat[St.byTurn], 14, TRUE);
			}
		}

		/* 座る場所に立っている時の案内（待機中の卓の、本人にだけ。サーバーが JANIDLE_SEAT を付ける）。
		   帯に「INSERT COIN」「道具→コイン」（無料の台は「PUSH START」「（Sキー）」）を点滅させ、立っている席の目印も点滅させる */
		if ((bIdle || bRecruit) && (St.byIdle & JANIDLE_SEAT)) {
			int nCellX = St.nFieldX / 32 + JANJAN_TABLE_HALF, nCellY = St.nFieldY / 32 + JANJAN_TABLE_HALF;
			g_bDraw = TRUE;		/* 点滅のため、出ている間は描き続ける */
			for (s = 0; s < JANJAN_SEATS; s ++) {
				JanjanSeatOffset (s, &nDx, &nDy);
				if ((nCellX + nDx == nMyX) && (nCellY + nDy == nMyY) && bBlink) {
					int nX2 = sx + (JANJAN_TABLE_HALF + nDx) * 32, nY2 = sy + (JANJAN_TABLE_HALF + nDy) * 32;
					JanEdge (hDC, nX2 + 1, nY2 + 1, 30, 30, RGB (255, 245, 90));
					JanEdge (hDC, nX2 + 2, nY2 + 2, 28, 28, RGB (255, 245, 90));
				}
			}
			if (bBlink) {
				BOOL bFree = (St.byIdle & JANIDLE_FREE) ? TRUE : FALSE;
				PuzText (hDC, nBandCX, nBandY, (LPSTR)(bFree ? "PUSH START" : "INSERT COIN"), RGB (255, 235, 120), 14, TRUE);
				PuzText (hDC, nBandCX, nBandY + 14, (LPSTR)(bFree ? "（Sキー）" : "道具→コイン"), RGB (255, 255, 255), 14, TRUE);
			}
		}

		/* 終局：「上がり！」「流局」を卓の上（真ん中）に出す */
		if (bOver && ((St.byResult == JANJAN_RESULT_WIN) || (St.byResult == JANJAN_RESULT_DRAW))) {
			BOOL bWin = (St.byResult == JANJAN_RESULT_WIN) ? TRUE : FALSE;
			BOOL bBan = (s_aJanImg[JIMG_BANNER].pRgba != NULL) ? TRUE : FALSE;
			int bw = 160, bh = bWin ? 54 : 40, bxx = nBandCX - bw / 2, byy = nBandY + nBandH / 2 - bh / 2;

			if (bBan) {
				/* 帯の素材（200×48。中の明るい面は 28 の高さ）の上に大きな字。名前の行は帯の下に暗く敷いて出す */
				bw = 200;
				bh = bWin ? 48 + 16 : 48;
				byy = nBandY + nBandH / 2 - bh / 2;
				bxx = nBandCX - bw / 2;
				JanArt (JIMG_BANNER, bxx, byy, JAN_NOTINT, 0, 255);
				SelectObject (hDC, g_hFont32);
				PuzText (hDC, nBandCX, byy + 3, (LPSTR)(bWin ? "上がり！" : "流局"), bWin ? RGB (185, 50, 30) : RGB (50, 70, 130), 32, TRUE);
				SelectObject (hDC, g_hFont14);
				if (bWin && (St.byWinner < JANJAN_SEATS)) {
					JanRect (hDC, nBandCX - 80, byy + 46, 160, 17, RGB (30, 22, 20), 215);
					JanEdge (hDC, nBandCX - 80, byy + 46, 160, 17, s_aclJanSeat[St.byWinner]);
					JanFitText (hDC, D.aszName[St.byWinner], 160 - 50, szFit, sizeof (szFit));
					wsprintf (szTmp, "%s（%s）", szFit, (St.byWinHow == JANJAN_HOW_RON) ? "ロン" : "ツモ");
					PuzText (hDC, nBandCX, byy + 47, szTmp, RGB (255, 255, 255), 14, TRUE);
				}
			} else {
				PuzFrame (hDC, bxx, byy, bw, bh, bWin ? RGB (255, 225, 60) : RGB (170, 190, 230), 215);
				SelectObject (hDC, g_hFont32);
				PuzText (hDC, nBandCX, byy + 4, (LPSTR)(bWin ? "上がり！" : "流局"), bWin ? RGB (255, 235, 90) : RGB (190, 210, 255), 32, TRUE);
				SelectObject (hDC, g_hFont14);
				if (bWin && (St.byWinner < JANJAN_SEATS)) {
					JanFitText (hDC, D.aszName[St.byWinner], bw - 50, szFit, sizeof (szFit));
					wsprintf (szTmp, "%s（%s）", szFit, (St.byWinHow == JANJAN_HOW_RON) ? "ロン" : "ツモ");
					PuzText (hDC, nBandCX, byy + 37, szTmp, s_aclJanSeat[St.byWinner], 14, TRUE);
				}
			}
		}
	}
}

/* 座っている本人の手牌の別枠（画面の下に固定。落ち物の帯と同じ最前面）。
   手牌（牌 32 ドット×最大 9 枚）・鳴いた組・カーソル・操作の行（ロン／そろい／組そろい／ツモ上がり／見送り）・残り時間 */
static void DrawJanPanel(HDC hDC, const JANDRAW &D, int nX, int nY)
{
	const CMDPARA_JANSTATE &St = D.St;
	const JANSEATWIRE &Seat = St.aSeat[St.byMySeat];
	int px = nX + (SCRSIZEX - JAN_PANEL_W) / 2, py = nY + SCRSIZEY - JAN_PANEL_H - 6;
	int nHand = min ((int)Seat.byHandNum, JAN_HAND_WIRE), nMeld = min ((int)Seat.byMeldNum, JAN_MELD_WIRE);
	int i, nLen, nStartX, nTileY, aHandOff[JAN_HAND_WIRE + 1], aMeldOff[JAN_MELD_WIRE * JANJAN_SET_SIZE + 1];
	BOOL bGap = (!(St.byPhase == JANPHASE_IDLE) && (St.byLogicPhase == JANJAN_PHASE_DISCARD) && (St.byTurn == St.byMySeat) && (nHand % 3 == 0)) ? TRUE : FALSE;
	BOOL bBlink = (((timeGetTime () / 400) & 1) == 0) ? TRUE : FALSE;
	char szTmp[96], szFit[JAN_NAME_LEN + 4];
	const char *pszMsg;
	SIZE size;

	/* 別枠の素材（448×96。枠の厚みは 7）。素材がある時は、中身を枠の内側に収まるよう少しずらす */
	BOOL bPnl = (s_aJanImg[JIMG_PANEL].pRgba != NULL) ? TRUE : FALSE;
	int nPadX = bPnl ? 4 : 0;
	/* 縦の並び（枠の内側は py+8 から py+H-8。各段のあいだは 6 ドット以上あける）：
	   案内文 py+14〜28／残り時間バー py+34〜38／操作行 py+44〜68／牌 py+76〜108（カーソルは上下 2 はみ出す） */
	const int nYMsg = 14, nYBar = 34, nYOp = 44, nYTile = 76;

	if (bPnl) {
		/* 席の色を乗算で薄く掛け、内側に席の色の細い線も足す（4 席を見分けるため） */
		/* 素材は高さ 96。上半分と下半分を上下に置き、あいだは真ん中の 1 行を繰り返して伸ばす */
		JanArtEx (JIMG_PANEL, px, py, 0, 0, 448, 48, 448, 0, s_aclJanSeat[St.byMySeat], 120, 255);
		JanArtEx (JIMG_PANEL, px, py + JAN_PANEL_H - 48, 0, 48, 448, 48, 448, 0, s_aclJanSeat[St.byMySeat], 120, 255);
		for (i = 48; i < JAN_PANEL_H - 48; i ++) {
			JanArtEx (JIMG_PANEL, px, py + i, 0, 47, 448, 1, 448, 0, s_aclJanSeat[St.byMySeat], 120, 255);
		}
		JanEdge (hDC, px + 7, py + 7, JAN_PANEL_W - 14, JAN_PANEL_H - 14, s_aclJanSeat[St.byMySeat]);
		nPadX += 3;
	} else {
		PuzFrame (hDC, px, py, JAN_PANEL_W, JAN_PANEL_H, s_aclJanSeat[St.byMySeat], 200);
	}

	/* 1 行目：状況の案内（左）と待ち時間の残り（右） */
	if (St.byPhase == JANPHASE_OVER) {
		if (St.byResult == JANJAN_RESULT_WIN) {
			JanFitText (hDC, D.aszName[St.byWinner < JANJAN_SEATS ? St.byWinner : 0], 150, szFit, sizeof (szFit));
			wsprintf (szTmp, "%sの上がり！", szFit);
			pszMsg = szTmp;
		} else {
			pszMsg = "流局";
		}
	} else if (D.bCanDiscard) {
		pszMsg = (St.byCanAct & JAN_CAN_TSUMO) ? "ツモ上がりできます" : "捨てる牌を選んで 決定";
	} else if (D.nOpNum > 0) {
		pszMsg = "どうしますか？";
	} else if (St.byPhase == JANPHASE_RECRUIT) {
		pszMsg = "参加者を募集中（ESCで席を立つ）";
	} else if (nHand <= 0) {
		pszMsg = "配牌をまっています";
	} else {
		pszMsg = "ほかの人の番です";
	}
	PuzText (hDC, px + 10 + nPadX, py + nYMsg, (LPSTR)pszMsg, RGB (255, 235, 120), 14, FALSE);
	if (D.nLeftMs > 0) {
		int nSec = (D.nLeftMs + 999) / 1000;
		int nBarW = JAN_PANEL_W - 20 - nPadX * 2, nFill = (D.nLimitMax > 0) ? (nBarW * min (D.nLeftMs, D.nLimitMax) / D.nLimitMax) : nBarW;
		COLORREF clBar = (D.nLimitMax > 0 && D.nLeftMs * 4 < D.nLimitMax) ? RGB (235, 80, 70) : RGB (240, 215, 60);

		wsprintf (szTmp, "残り %d 秒", nSec);
		GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
		PuzText (hDC, px + JAN_PANEL_W - 10 - nPadX - size.cx, py + nYMsg, szTmp, RGB (255, 255, 255), 14, FALSE);
		JanRect (hDC, px + 10 + nPadX, py + nYBar, nBarW, 4, RGB (70, 70, 70), 255);
		JanRect (hDC, px + 10 + nPadX, py + nYBar, nFill, 4, clBar, 255);
	}

	/* 操作の行（ある時だけ。手牌の上） */
	if (D.nOpNum > 0) {
		int bx = px + 10 + nPadX, by = py + nYOp, bw, nTextW;
		static const char *s_apszOp[] = { "離席", "捨てる", "ツモ上がり", "ロン", "そろい", "組", "見送り" };	/* JAN_ACT_* の順 */

		for (i = 0; i < D.nOpNum; i ++) {
			const JANOP &Op = D.aOp[i];
			BOOL bSel = ((D.nFocus == 1) && (D.nOpSel == i)) ? TRUE : FALSE;
			const char *pszText = s_apszOp[Op.byAct <= JAN_ACT_PASS ? Op.byAct : 0];

			GetTextExtentPoint32 (hDC, pszText, strlen (pszText), &size);
			nTextW = size.cx;
			bw = nTextW + 16;
			if (Op.byAct == JAN_ACT_GUMI) {
				bw += JAN_T * 2 + 6;
			}
			if (s_aJanImg[JIMG_BTN].pRgba != NULL) {
				/* ボタンの素材（左=ふつう 中=選択中 右=選べない。各 64×24）を、左右 8 ドットを残して横に伸ばす */
				JanArtEx (JIMG_BTN, bx, by, bSel ? 64 : 0, 0, 64, JAN_OP_H, bw, 8, JAN_NOTINT, 0, 255);
			} else {
				PuzFrame (hDC, bx, by, bw, JAN_OP_H, bSel ? RGB (255, 240, 120) : RGB (150, 150, 150), bSel ? 235 : 170);
				if (bSel) {
					JanRect (hDC, bx + 1, by + 1, bw - 2, JAN_OP_H - 2, RGB (120, 100, 20), 200);
					JanEdge (hDC, bx + 1, by + 1, bw - 2, JAN_OP_H - 2, RGB (255, 240, 120));
				}
			}
			PuzText (hDC, bx + 8, by + 5, (LPSTR)pszText, (s_aJanImg[JIMG_BTN].pRgba != NULL) ? (bSel ? RGB (255, 255, 255) : RGB (60, 40, 25)) : (bSel ? RGB (255, 255, 255) : RGB (210, 210, 210)), 14, FALSE);
			if ((Op.byAct == JAN_ACT_GUMI) && (Op.byParam < JAN_GUMI_WIRE)) {
				/* 組そろいは、手牌から出す 2 枚を並べて見せる */
				int tx = bx + 8 + nTextW + 4;
				JanTileSmall (hDC, tx, by + 4, St.abyGumi[Op.byParam][0], RGB (70, 110, 220), TRUE);
				JanTileSmall (hDC, tx + JAN_T + 2, by + 4, St.abyGumi[Op.byParam][1], RGB (70, 110, 220), TRUE);
			}
			bx += bw + 6;
		}
	}

	/* 手牌の行（牌 32 ドット。引いた牌は少し離す。鳴いた組は右に公開） */
	nLen = JanStripLayout (Seat, bGap, JAN_BIG_STEP, 8, 14, aHandOff, aMeldOff);
	nStartX = px + (JAN_PANEL_W - nLen) / 2;
	nTileY = py + nYTile;
	for (i = 0; i < nHand; i ++) {
		JanTileBig (hDC, nStartX + aHandOff[i], nTileY, Seat.abyHand[i]);
	}
	for (i = 0; i < nMeld * JANJAN_SET_SIZE; i ++) {
		JanTileBig (hDC, nStartX + aMeldOff[i], nTileY, Seat.aMeld[i / JANJAN_SET_SIZE].abyTile[i % JANJAN_SET_SIZE]);
		JanEdge (hDC, nStartX + aMeldOff[i] - 1, nTileY - 1, JAN_BIG + 2, JAN_BIG + 2,
			(Seat.aMeld[i / JANJAN_SET_SIZE].byKind == JANJAN_MELD_SOROI) ? RGB (60, 160, 70) : RGB (70, 110, 220));
	}
	/* 手牌のカーソル（手牌の行にいる時は黄色の太枠と▼。操作の行にいる時は細い灰色の枠） */
	if ((D.nCursor >= 0) && (D.nCursor < nHand)) {
		int cx = nStartX + aHandOff[D.nCursor];
		BOOL bSelArt = (s_aJanImg[JIMG_SEL].pRgba != NULL) ? TRUE : FALSE;
		if (D.nFocus == 0) {
			COLORREF clCur = D.bCanDiscard ? (bBlink ? RGB (255, 245, 90) : RGB (255, 200, 40)) : RGB (215, 215, 215);
			if (bSelArt) {
				/* 選択カーソルの素材（36×36）。捨てられる時は点滅、捨てられない時は薄く */
				JanArt (JIMG_SEL, cx - 2, nTileY - 2, JAN_NOTINT, 0, D.bCanDiscard ? (bBlink ? 255 : 150) : 120);
			} else {
				JanEdge (hDC, cx - 1, nTileY - 1, JAN_BIG + 2, JAN_BIG + 2, clCur);
				JanEdge (hDC, cx - 2, nTileY - 2, JAN_BIG + 4, JAN_BIG + 4, clCur);
				JanTriDown (hDC, cx + JAN_BIG / 2 - 5, nTileY - 8, 11, clCur);
			}
			if (D.bCanDiscard) {
				g_bDraw = TRUE;
			}
		} else if (bSelArt) {
			JanArt (JIMG_SEL, cx - 2, nTileY - 2, JAN_NOTINT, 0, 100);
		} else {
			JanEdge (hDC, cx - 1, nTileY - 1, JAN_BIG + 2, JAN_BIG + 2, RGB (130, 130, 130));
		}
	}
}

/* 見えている卓すべて。bTop=FALSE は卓本体（地図のパーツ・NPC の後でキャラの前）、bTop=TRUE は文字と本人の別枠（ウィンドウの下） */
static void DrawJan(int nX, int nY, BOOL bTop)
{
	JANDRAW D;
	PCHARINFO pChar;
	HDC hDC = NULL;
	HFONT hFontOld = NULL;
	BOOL bOff;
	int i, sx, sy;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	for (i = 0; i < JAN_VIEW_DRAW_MAX; i ++) {
		if (!GetJanState (i, &D)) {
			continue;
		}
		/* マップのドット座標 → 描画用の画面（DrawPuzzle と同じ。自分のマスが (DRAW_PARTS_X+1)*32） */
		sx = (DRAW_PARTS_X + 1) * 32 + D.St.nFieldX - CamTileX () * 32;
		sy = (DRAW_PARTS_Y + 1) * 32 + D.St.nFieldY - CamTileY () * 32;
		/* 画面に少しもかからない卓は描かない（座る場所の目印が卓の外 1 マスに出るぶん広く見る） */
		bOff = ((sx + JAN_TBL + 32 <= nX) || (sx - 32 >= nX + SCRSIZEX) || (sy + JAN_TBL + 32 <= nY) || (sy - 32 >= nY + SCRSIZEY)) ? TRUE : FALSE;
		/* 座っている本人の別枠は、卓が画面の外でも画面の下に出す */
		if (bOff && !(bTop && (D.St.byMySeat < JANJAN_SEATS))) {
			continue;
		}
		s_nPzL = nX;
		s_nPzT = nY;
		s_nPzR = nX + SCRSIZEX;
		s_nPzB = nY + SCRSIZEY;
		if (hDC == NULL) {
			JanSheetInit ();
			JanArtInit ();
			hDC = g_pDibDrawMap->Lock ();
			hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
			SetBkMode (hDC, TRANSPARENT);
		}
		if (!bOff) {
			DrawJanOne (hDC, D, sx, sy, (int)pChar->dwX, (int)pChar->dwY, bTop);
		}
		if (bTop && (D.St.byMySeat < JANJAN_SEATS)) {
			DrawJanPanel (hDC, D, nX, nY);
		}
	}
	if (hDC) {
		SelectObject (hDC, hFontOld);
		g_pDibDrawMap->Unlock ();
	}
}

/* ========================================================================= */
/* 関数名：DrawMaze															 */
/* 内容：迷路対戦の盤（320×320 ドット）の重ね描き								 */
/*		 マス 32 ドット（市松の明るい床。左上は盤ドット (48x, 48y)）、マスの間の通り道 16 ドット＝暗い溝、	 */
/*		 置かれた壁＝レンガ色（太さ 16・長さ 80。交差点の 16×16 も壁の一部）。					 */
/*		 bTop=FALSE：盤の床・壁・カーソル（地面の上・キャラの下）					 */
/*		 bTop=TRUE ：コマ（台NPCに隠れないよう前に）・手番・壁の残り・勝敗・操作ヒント（最前面）	 */
/*		 盤の左上はサーバーが台の座標から決めた（マップのドット座標）				 */
/* ========================================================================= */
static int s_nMazeL, s_nMazeT, s_nMazeR, s_nMazeB;		/* 迷路対戦の重ね描きを切り取る範囲（DrawMaze が決める） */
#define MAZE_BOARD_DOT		(MAZE_BOARD_DOTS)		/* 盤の一辺（ドット。320） */

/* 塗りつぶし（切り取り範囲の中だけ） */
static void MazeRect(HDC hDC, int x, int y, int cx, int cy, COLORREF cl, int nAlpha)
{
	int yy;

	if ((y >= s_nPzB) || (y + cy <= s_nPzT) || (x >= s_nPzR) || (x + cx <= s_nPzL)) {
		return;
	}
	for (yy = max (0, s_nPzT - y); yy < cy; yy ++) {
		if (y + yy >= s_nPzB) {
			break;
		}
		PuzSpan (hDC, x, y + yy, cx, cl, nAlpha);
	}
}

/* レンガ 1 枚分の長方形（段の高さ 8 ドット。段ごとに継ぎ目を半分ずらす） */
static void MazeBrick(HDC hDC, int x, int y, int cx, int cy)
{
	const COLORREF clBase = RGB (178, 86, 58), clMortar = RGB (104, 52, 40), clLight = RGB (214, 128, 92);
	int r, xs, nRow = 0, nH;

	MazeRect (hDC, x, y, cx, cy, clBase, 255);
	for (r = 0; r < cy; r += 8, nRow ++) {
		nH = min (8, cy - r);
		MazeRect (hDC, x, y + r, cx, 1, clLight, 255);							/* 段の上の明るい線 */
		MazeRect (hDC, x, y + r + nH - 1, cx, 1, clMortar, 255);					/* 段の下の目地 */
		for (xs = (nRow & 1) ? 16 : 0; xs < cx; xs += 32) {
			if (xs > 0) {
				MazeRect (hDC, x + xs, y + r, 1, nH, clMortar, 255);				/* 縦の継ぎ目 */
			}
		}
	}
	/* 外枠（暗く縁取る） */
	MazeRect (hDC, x, y, cx, 1, clMortar, 255);
	MazeRect (hDC, x, y + cy - 1, cx, 1, clMortar, 255);
	MazeRect (hDC, x, y, 1, cy, clMortar, 255);
	MazeRect (hDC, x + cx - 1, y, 1, cy, clMortar, 255);
}

/* 壁 1 枚（交点 (wx, wy)・向き）を盤の左上 (bx, by) を基準に描く。マスの間の通り道（16 ドット）にぴったり置く。
   横壁：縦 48y+32〜+48・横 48x〜48x+80（2 マス＋隙間 1 つ分）／縦壁はその縦横を入れ替えた形 */
static void MazeWallRect(int wx, int wy, int nOrient, int bx, int by, int *px, int *py, int *pcx, int *pcy)
{
	const int P = MAZE_PITCH_DOT;

	if (nOrient == MAZE_H) {
		*px = bx + wx * P;
		*py = by + wy * P + MAZE_CELL_DOT;
		*pcx = MAZE_CELL_DOT * 2 + MAZE_GAP_DOT;
		*pcy = MAZE_GAP_DOT;
	} else {
		*px = bx + wx * P + MAZE_CELL_DOT;
		*py = by + wy * P;
		*pcx = MAZE_GAP_DOT;
		*pcy = MAZE_CELL_DOT * 2 + MAZE_GAP_DOT;
	}
}

/* ---- 迷路対戦の見た目の素材 ----------------------------------------------------------------
   壁の絵：system.png の右端に 16×16 を 14 枚描き足してある（プレイヤー 0＝茶のレンガ・プレイヤー 1＝灰の石レンガ）。
	 x=224 がプレイヤー 0、x=240 がプレイヤー 1。y は上から
	   128 横壁の左端 / 144 横壁の中 / 160 横壁の右端 / 176 縦壁の上端 / 208 縦壁の中 / 224 縦壁の下端 / 240 交差点（壁の真ん中の石）
	 （y=192〜207 は元からある絵があるので飛ばしてある。ほかの使用済みの場所は docs/maze-attack.md）
	 横壁（80×16）は 左端・中・交差点・中・右端 の 5 枚、縦壁（16×80）は 上端・中・交差点・中・下端 の 5 枚で組む。
	 黒 (0,0,0) は透過色なので、描き足した絵に黒は使っていない。
   床・隙間：台詞のパーツID（マップパーツ。(wMap%16*32, wMap/16*32) の 32×32）から描く。隙間は中央の 16×16 をタイル状に敷く */
#define MAZEART_X0			224		/* system.png の壁の絵の左（プレイヤー 0）。プレイヤー 1 は +16 */
#define MAZEART_HL			0		/* 絵の番号：横壁の左端 */
#define MAZEART_HM			1		/* 横壁の中 */
#define MAZEART_HR			2		/* 横壁の右端 */
#define MAZEART_VT			3		/* 縦壁の上端 */
#define MAZEART_VM			4		/* 縦壁の中 */
#define MAZEART_VB			5		/* 縦壁の下端 */
#define MAZEART_X			6		/* 交差点（壁の真ん中） */
static const int s_anMazeArtY[7] = { 128, 144, 160, 176, 208, 224, 240 };	/* 絵の番号ごとの system.png の y */

/* 壁の絵が使えるか（system.png を読めていて、描き足した場所が透明でない） */
static BOOL MazeWallArtOk(void)
{
	int p, k;

	if ((g_pDibSystem == NULL) || (g_pDibSystem->Width () < 256) || (g_pDibSystem->Height () < 256)) {
		return FALSE;
	}
	for (p = 0; p < 2; p ++) {
		for (k = 0; k < 7; k ++) {
			int x = MAZEART_X0 + p * 16 + 8, y = s_anMazeArtY[k] + 8;
			DWORD dwPix = *(PDWORD)(g_pDibSystem->GetBits () + (size_t)(g_pDibSystem->Width () * 4) * (g_pDibSystem->Height () - 1 - y) + x * 4);
			if ((dwPix & 0x00FFFFFF) == 0) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/* マップパーツIDが使えるか（0 は「指定なし」、パーツ数以上は範囲外。どちらもコード描画に戻す） */
static BOOL MazePartOk(WORD wMap)
{
	return (g_pDibMapParts != NULL) && (wMap != 0) && (wMap < g_wCountMapParts);
}

/* 切り取り範囲（s_nPz*）の中だけ矩形転送する。nAlpha が 100 未満なら半透明 */
static void MazeBlt(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BOOL bColorKey, int nAlpha)
{
	int l = max (dx, s_nPzL), t = max (dy, s_nPzT), r = min (dx + cx, s_nPzR), b = min (dy + cy, s_nPzB);

	if ((r <= l) || (b <= t)) {
		return;
	}
	sx += l - dx;
	sy += t - dy;
	if (nAlpha >= 100) {
		g_pDibDrawMap->Blt (l, t, r - l, b - t, pSrc, sx, sy, bColorKey);
	} else {
		g_pDibDrawMap->BltAlpha (l, t, r - l, b - t, pSrc, sx, sy, (BYTE)nAlpha, bColorKey);
	}
}

/* 壁 1 枚を system.png の絵で描く（Lock していない状態で呼ぶ。nOwner=0/1 で色違い。nAlpha<100 で半透明） */
static void MazeWallArt(int wx, int wy, int nOrient, int nOwner, int nAlpha)
{
	static const int anH[5] = { MAZEART_HL, MAZEART_HM, MAZEART_X, MAZEART_HM, MAZEART_HR };
	static const int anV[5] = { MAZEART_VT, MAZEART_VM, MAZEART_X, MAZEART_VM, MAZEART_VB };
	const int *pn = (nOrient == MAZE_H) ? anH : anV;
	int k, sx = MAZEART_X0 + ((nOwner == 1) ? 16 : 0);

	for (k = 0; k < 5; k ++) {
		int x = (nOrient == MAZE_H) ? wx + k * 16 : wx;
		int y = (nOrient == MAZE_H) ? wy : wy + k * 16;
		MazeBlt (x, y, 16, 16, g_pDibSystem, sx, s_anMazeArtY[pn[k]], TRUE, nAlpha);
	}
}

/* 色付きの丸（半径 nR。中心 cx, cy） */
static void MazeCircle(HDC hDC, int cx, int cy, int nR, COLORREF cl, COLORREF clEdge, int nAlpha)
{
	int dy, nHalf;

	for (dy = -nR; dy <= nR; dy ++) {
		nHalf = (int)(sqrtf ((float)(nR * nR - dy * dy)) + 0.5f);
		if ((cy + dy < s_nPzT) || (cy + dy >= s_nPzB)) {
			continue;
		}
		PuzSpan (hDC, cx - nHalf, cy + dy, nHalf * 2 + 1, clEdge, nAlpha);
		if (nHalf > 2) {
			PuzSpan (hDC, cx - nHalf + 2, cy + dy, nHalf * 2 - 3, cl, nAlpha);
		}
	}
}

/* コマの絵（転送元の画像と範囲） */
typedef struct _MAZEPIECE {
	CImg32		*pDib;											/* 転送元の画像 */
	int			xx, yy;											/* 転送元の左上 */
	int			cx, cy;											/* 転送する大きさ（32×32 のキャラ以外は大きい） */
} MAZEPIECE;

/* キャラの絵（通常・横16縦32・戦場ゲーム・横32縦32・横48縦48）から、向き nDir（0=上・1=下）・アニメ 0 コマ目の転送元を求める。
   DrawCharLine がキャラを描く時と同じ絵の探し方。描けない種類・範囲外なら FALSE */
static BOOL MazeGrpSource(WORD wGrpID, WORD wGrpMain, WORD wGrpSub, int nDir, MAZEPIECE *pOut)
{
	int no;

	switch (wGrpMain) {
	case CHARGRP_MAIN_NORMAL:
		no = wGrpID / 16;
		if (no >= g_wCountCharGrp) {
			return FALSE;
		}
		pOut->pDib = g_apDibChar[no];
		pOut->xx = nDir * 64;
		pOut->yy = 32 * (wGrpID % 16);
		pOut->cx = 32;
		pOut->cy = 32;
		return TRUE;
	case CHARGRP_MAIN_1632:
		no = wGrpID / 8;
		if (no >= g_wCountCharGrp1632) {
			return FALSE;
		}
		pOut->pDib = g_apDibChar1632[no];
		pOut->xx = nDir * 64;
		pOut->yy = 64 * (wGrpID % 16);
		pOut->cx = 32;
		pOut->cy = 64;
		return TRUE;
	case CHARGRP_MAIN_SUBWAR:
		if (g_pDibWChar == NULL) {
			return FALSE;
		}
		pOut->pDib = g_pDibWChar;
		pOut->xx = wGrpSub * 64;
		pOut->yy = wGrpID * 32;
		pOut->cx = 32;
		pOut->cy = 32;
		return TRUE;
	case CHARGRP_MAIN_3232:
		no = wGrpID / 8;
		if (no >= g_wCountCharGrp3232) {
			return FALSE;
		}
		pOut->pDib = g_apDibChar3232[no];
		pOut->xx = nDir * 128;
		pOut->yy = 64 * (wGrpID % 16);
		pOut->cx = 64;
		pOut->cy = 64;
		return TRUE;
	case CHARGRP_MAIN_4848:
		no = wGrpID / 8;
		if (no >= g_wCountCharGrp4848) {
			return FALSE;
		}
		pOut->pDib = g_apDibChar4848[no];
		pOut->xx = nDir * 192;
		pOut->yy = 96 * (wGrpID % 16);
		pOut->cx = 96;
		pOut->cy = 96;
		return TRUE;
	}
	return FALSE;
}

/* コマの絵：座っているキャラの、その人が普段描かれる絵（変身中は変身後の絵。特殊なグラフィックも含む）。向きは、下側スタートのプレイヤー 0 が上向き・上側スタートのプレイヤー 1 が下向き。描けなければ FALSE */
static BOOL MazePieceImage(PCHARINFO pChar, int nPlayer, MAZEPIECE *pOut)
{
	WORD wGrpID = pChar->wGrpID, wGrpMain = pChar->wGrpMain, wGrpSub = pChar->wGrpSub;

	if (!((pChar->wGrpTmpID == 0) && (pChar->wGrpTmpMain == 0) && (pChar->wGrpTmpSub == 0))) {
		wGrpID = pChar->wGrpTmpID;
		wGrpMain = pChar->wGrpTmpMain;
		wGrpSub = pChar->wGrpTmpSub;
	}
	return MazeGrpSource (wGrpID, wGrpMain, wGrpSub, (nPlayer == 0) ? 0 : 1, pOut);
}

/* CPUのコマの絵：台の設定のキャラ画像（wCpuGrpID。NPC と同じ通常グラフィック）。向きは盤の進行方向（人と同じ）。0 や範囲外なら FALSE（色付きの丸） */
static BOOL MazeCpuImage(const CMDPARA_MAZESTATE &St, int nPlayer, MAZEPIECE *pOut)
{
	if (St.wCpuGrpID == 0) {
		return FALSE;
	}
	return MazeGrpSource (St.wCpuGrpID, CHARGRP_MAIN_NORMAL, 0, (nPlayer == 0) ? 0 : 1, pOut);
}

/* コマの絵を、マス (x, y)（左上）を基準に描く。32×32 より大きい絵は、マスの中央・足元をマスの下端に合わせる。切り取り範囲（s_nPz*）にかかる所は切る */
static void MazeBltPiece(int x, int y, const MAZEPIECE &Pc)
{
	int dx = x + (MAZE_CELL_DOT - Pc.cx) / 2, dy = y + MAZE_CELL_DOT - Pc.cy;
	int sx = Pc.xx, sy = Pc.yy, cx = Pc.cx, cy = Pc.cy;

	if (dx < s_nPzL) { sx += s_nPzL - dx; cx -= s_nPzL - dx; dx = s_nPzL; }
	if (dy < s_nPzT) { sy += s_nPzT - dy; cy -= s_nPzT - dy; dy = s_nPzT; }
	if (dx + cx > s_nPzR) { cx = s_nPzR - dx; }
	if (dy + cy > s_nPzB) { cy = s_nPzB - dy; }
	if ((cx <= 0) || (cy <= 0)) {
		return;
	}
	g_pDibDrawMap->Blt (dx, dy, cx, cy, Pc.pDib, sx, sy, TRUE);
}

/* CPUの名前表示（空なら「CPU」。強さの数だけ ★ を添える） */
static void MazeCpuName(const CMDPARA_MAZESTATE &St, char *pszOut, int nSize)
{
	int k, nLen;

	lstrcpyn (pszOut, St.szCpuName, nSize);
	if (pszOut[0] == '\0') {
		lstrcpyn (pszOut, "CPU", nSize);
	}
	for (k = 0; (k < (int)St.byCpuLevel) && (k < 3); k ++) {
		nLen = lstrlen (pszOut);
		if (nLen + 3 >= nSize) {
			break;
		}
		lstrcat (pszOut, "★");
	}
}

/* コマ（台NPCのマスに盤の下の行が重なるので、キャラより前に描く） */
static void DrawMazePieces(const CMDPARA_MAZESTATE &St, int sx, int sy)
{
	const COLORREF clPiece[2] = { RGB (255, 210, 70), RGB (110, 190, 255) };
	int x, y, p;
	HDC hDC;
	HFONT hFontOld;
	MAZEPIECE aPiece[2];
	BOOL abPiece[2] = { FALSE, FALSE };		/* 絵で描くコマか */

	s_nPzL = s_nMazeL;
	s_nPzT = s_nMazeT;
	s_nPzR = s_nMazeR;
	s_nPzB = s_nMazeB;
	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
	SetBkMode (hDC, TRANSPARENT);
	/* コマ（絵が使えない時と不在の側は色付きの丸） */
	for (p = 0; p < 2; p ++) {
		int cx = sx + (int)St.byPosX[p] * MAZE_PITCH_DOT + MAZE_CELL_DOT / 2;
		int cy = sy + (int)St.byPosY[p] * MAZE_PITCH_DOT + MAZE_CELL_DOT / 2;
		BOOL bHave = (St.dwChar[p] != MAZE_NOCHAR);
		BOOL bCpu = (!bHave && (St.byState & MAZESTATE_CPU)) ? TRUE : FALSE;		/* CPU対戦中の、人のいない側＝CPU */
		int nNo = bHave ? GetCharNo2 (St.dwChar[p]) : -1;
		if (St.byPosX[p] >= MAZE_SIZE || St.byPosY[p] >= MAZE_SIZE) {
			continue;
		}
		if (bCpu) {
			/* CPU：キャラ画像の指定があればその絵（名前は絵の後に描く）。無ければ色付きの丸と名前 */
			char szCpu[64];
			if (MazeCpuImage (St, p, &aPiece[p])) {
				abPiece[p] = TRUE;
				continue;
			}
			MazeCpuName (St, szCpu, sizeof (szCpu));
			MazeCircle (hDC, cx, cy, 11, clPiece[p], RGB (40, 30, 20), 255);
			PuzText (hDC, cx, cy - 6, (LPSTR)"C", RGB (40, 30, 20), 14, TRUE);
			PuzText (hDC, cx, cy + 12, szCpu, RGB (255, 255, 255), 14, TRUE);
			continue;
		}
		if ((nNo >= 0) && MazePieceImage (g_aCharInfo[nNo], p, &aPiece[p])) {
			abPiece[p] = TRUE;
			continue;		/* 絵は下の層を閉じてから描く */
		}
		if (bHave && (nNo >= 0)) {
			PCHARINFO pChar = g_aCharInfo[nNo];
			MazeCircle (hDC, cx, cy, 11, clPiece[p], RGB (40, 30, 20), 255);
			PuzText (hDC, cx, cy + 12, pChar->szName, RGB (255, 255, 255), 14, TRUE);
		} else if (bHave) {
			/* 座っている人が画面の外にいて（キャラ情報が届いていない）絵も名前も出せない時は、色付きの丸に「相手」 */
			MazeCircle (hDC, cx, cy, 11, clPiece[p], RGB (40, 30, 20), 255);
			PuzText (hDC, cx, cy + 12, (LPSTR)"相手", RGB (255, 255, 255), 14, TRUE);
		} else {
			MazeCircle (hDC, cx, cy, 11, clPiece[p], RGB (40, 30, 20), 110);		/* 不在：薄い丸 */
		}
	}
	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();

	/* キャラの絵のコマ（参加者本人の絵・CPUの絵） */
	for (p = 0; p < 2; p ++) {
		if (!abPiece[p] || (St.byPosX[p] >= MAZE_SIZE) || (St.byPosY[p] >= MAZE_SIZE)) {
			continue;
		}
		x = sx + (int)St.byPosX[p] * MAZE_PITCH_DOT;		/* マスの左上から（32 ドット四方のキャラはずらさない） */
		y = sy + (int)St.byPosY[p] * MAZE_PITCH_DOT;
		MazeBltPiece (x, y, aPiece[p]);
	}

	/* 絵で出したCPUの名前（絵に隠れないよう、絵のあとで描く） */
	if (St.byState & MAZESTATE_CPU) {
		BOOL bAny = FALSE;
		for (p = 0; p < 2; p ++) {
			if (abPiece[p] && (St.dwChar[p] == MAZE_NOCHAR) && (St.byPosX[p] < MAZE_SIZE) && (St.byPosY[p] < MAZE_SIZE)) {
				bAny = TRUE;
			}
		}
		if (bAny) {
			hDC = g_pDibDrawMap->Lock ();
			hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
			SetBkMode (hDC, TRANSPARENT);
			for (p = 0; p < 2; p ++) {
				char szCpu[64];
				if (!abPiece[p] || (St.dwChar[p] != MAZE_NOCHAR) || (St.byPosX[p] >= MAZE_SIZE) || (St.byPosY[p] >= MAZE_SIZE)) {
					continue;
				}
				MazeCpuName (St, szCpu, sizeof (szCpu));
				PuzText (hDC, sx + (int)St.byPosX[p] * MAZE_PITCH_DOT + MAZE_CELL_DOT / 2,
					sy + (int)St.byPosY[p] * MAZE_PITCH_DOT + MAZE_CELL_DOT + 1, szCpu, RGB (255, 255, 255), 14, TRUE);
			}
			SelectObject (hDC, hFontOld);
			g_pDibDrawMap->Unlock ();
		}
	}

	/* 手番の側のコマにだけ、頭の上で点滅する小さな三角（2 人対戦中。カウントダウン・勝ち表示中は出さない） */
	if ((St.byState & MAZESTATE_VS) && !(St.byState & (MAZESTATE_COUNT | MAZESTATE_WIN)) && (St.byTurn < 2) &&
		(St.byPosX[St.byTurn] < MAZE_SIZE) && (St.byPosY[St.byTurn] < MAZE_SIZE) && ((timeGetTime () / 350) % 2)) {
		int cx = sx + (int)St.byPosX[St.byTurn] * MAZE_PITCH_DOT + MAZE_CELL_DOT / 2;
		int cy = sy + (int)St.byPosY[St.byTurn] * MAZE_PITCH_DOT - 7;		/* マスの上の縁の少し上（盤の隙間にかかる） */
		hDC = g_pDibDrawMap->Lock ();
		for (y = 0; y < 7; y ++) {
			PuzSpan (hDC, cx - 6 + y, cy + y - 6, 13 - y * 2, RGB (255, 235, 60), 255);
		}
		g_pDibDrawMap->Unlock ();
	}
	if (St.byState & MAZESTATE_VS) {
		g_bDraw = TRUE;		/* 点滅・残り秒のために描き続ける */
	}
}

/* 盤（床・隙間・壁・カーソル）。GDI の描画（Lock 中）と絵の転送（Lock していない時）を分けて順に描く */
static void DrawMazeBoard(const CMDPARA_MAZESTATE &St, int sx, int sy)
{
	BOOL bSeat = (St.byState & MAZESTATE_SEAT) ? TRUE : FALSE;
	BOOL bWin = (St.byState & MAZESTATE_WIN) ? TRUE : FALSE;
	BOOL bGapPart = MazePartOk (St.wGap);
	BOOL bWallArt = MazeWallArtOk ();
	BOOL bNeedCode = FALSE;
	BOOL bVs = (St.byState & MAZESTATE_VS) ? TRUE : FALSE;
	int nSeat = bSeat ? max (0, GetMazeMySeat (&St)) : 0;
	BOOL bMyTurn = (!bVs || ((int)St.byTurn == nSeat)) && !(St.byState & MAZESTATE_COUNT);
	int nMode, nCurX, nCurY, nCurO;
	int i, x, y;
	HDC hDC;

	GetMazeCursor (&nMode, &nCurX, &nCurY, &nCurO);
	s_nPzL = s_nMazeL;
	s_nPzT = s_nMazeT;
	s_nPzR = s_nMazeR;
	s_nPzB = s_nMazeB;

	/* 盤の縁と、隙間の地（パーツ指定が無ければ暗い単色） */
	hDC = g_pDibDrawMap->Lock ();
	MazeRect (hDC, sx - 4, sy - 4, MAZE_BOARD_DOT + 8, MAZE_BOARD_DOT + 8, RGB (36, 28, 24), 255);
	if (!bGapPart) {
		MazeRect (hDC, sx, sy, MAZE_BOARD_DOT, MAZE_BOARD_DOT, RGB (58, 48, 44), 255);
	}
	g_pDibDrawMap->Unlock ();
	if (bGapPart) {
		/* 隙間：パーツの中央 16×16 を、通り道（16 ドット幅の溝と交差点）にだけ敷く（盤を 16 ドットで割った格子のうち、マスにかからない所） */
		int nSx = (St.wGap % 16) * 32 + 8, nSy = (St.wGap / 16) * 32 + 8;
		for (y = 0; y < MAZE_BOARD_DOT / MAZE_GAP_DOT; y ++) {
			for (x = 0; x < MAZE_BOARD_DOT / MAZE_GAP_DOT; x ++) {
				if ((x % 3 != 2) && (y % 3 != 2)) {
					continue;		/* マスの上（床が覆う） */
				}
				MazeBlt (sx + x * MAZE_GAP_DOT, sy + y * MAZE_GAP_DOT, MAZE_GAP_DOT, MAZE_GAP_DOT, g_pDibMapParts, nSx, nSy, FALSE, 100);
			}
		}
	}

	/* 床：マスごとに、市松で床A・床B のパーツ（B が無ければ A だけ。どちらも使えないマスはコード描画） */
	for (y = 0; y < MAZE_BOARD_CELLS; y ++) {
		for (x = 0; x < MAZE_BOARD_CELLS; x ++) {
			WORD wA = St.wFloor[0], wB = St.wFloor[1];
			WORD wMap = ((x + y) & 1) ? wB : wA, wOther = ((x + y) & 1) ? wA : wB;
			if (!MazePartOk (wMap)) {
				wMap = wOther;
			}
			if (MazePartOk (wMap)) {
				MazeBlt (sx + x * MAZE_PITCH_DOT, sy + y * MAZE_PITCH_DOT, MAZE_CELL_DOT, MAZE_CELL_DOT, g_pDibMapParts,
					(wMap % 16) * 32, (wMap / 16) * 32, FALSE, 100);
			} else {
				bNeedCode = TRUE;
			}
		}
	}
	hDC = g_pDibDrawMap->Lock ();
	if (bNeedCode) {
		for (y = 0; y < MAZE_BOARD_CELLS; y ++) {
			for (x = 0; x < MAZE_BOARD_CELLS; x ++) {
				int cx0 = sx + x * MAZE_PITCH_DOT, cy0 = sy + y * MAZE_PITCH_DOT;
				WORD wMap = ((x + y) & 1) ? St.wFloor[1] : St.wFloor[0], wOther = ((x + y) & 1) ? St.wFloor[0] : St.wFloor[1];
				COLORREF cl = ((x + y) & 1) ? RGB (200, 188, 158) : RGB (226, 216, 186);
				if (MazePartOk (wMap) || MazePartOk (wOther)) {
					continue;
				}
				MazeRect (hDC, cx0, cy0, MAZE_CELL_DOT, MAZE_CELL_DOT, cl, 255);
				MazeRect (hDC, cx0, cy0, MAZE_CELL_DOT, 1, RGB (244, 238, 214), 255);							/* マスの上の縁を明るく */
				MazeRect (hDC, cx0, cy0 + MAZE_CELL_DOT - 1, MAZE_CELL_DOT, 1, RGB (150, 138, 110), 255);		/* 下の縁を暗く */
			}
		}
	}
	/* ゴール行（プレイヤー 0 は盤の上端、プレイヤー 1 は下端のマスの行）をうっすら色づける。
	   座っている本人は自分のゴールだけ、2 人対戦を見ている人は両方（コマと同じ色）。勝ちを見せている間は勝者のゴールを光らせる */
	if (bSeat) {
		MazeRect (hDC, sx, sy + ((nSeat == 0) ? 0 : (MAZE_BOARD_DOT - MAZE_CELL_DOT)), MAZE_BOARD_DOT, MAZE_CELL_DOT, RGB (255, 200, 40), 90);
	} else if (bVs) {
		MazeRect (hDC, sx, sy, MAZE_BOARD_DOT, MAZE_CELL_DOT, RGB (255, 210, 70), 70);
		MazeRect (hDC, sx, sy + MAZE_BOARD_DOT - MAZE_CELL_DOT, MAZE_BOARD_DOT, MAZE_CELL_DOT, RGB (110, 190, 255), 70);
	}
	if (bWin && (St.cWinner >= 0) && ((timeGetTime () / 250) % 2)) {
		MazeRect (hDC, sx, sy + ((St.cWinner == 0) ? 0 : (MAZE_BOARD_DOT - MAZE_CELL_DOT)), MAZE_BOARD_DOT, MAZE_CELL_DOT, RGB (255, 255, 200), 150);
	}
	g_pDibDrawMap->Unlock ();

	/* 置かれた壁（絵が使えなければコード描画のレンガ） */
	if (bWallArt) {
		for (i = 0; i < St.byWallCount && i < MAZEPARA_WALL_MAX; i ++) {
			int wx, wy, wcx, wcy;
			MazeWallRect (St.aWall[i].byX, St.aWall[i].byY, St.aWall[i].byOrient, sx, sy, &wx, &wy, &wcx, &wcy);
			MazeWallArt (wx, wy, St.aWall[i].byOrient, St.aWall[i].byOwner, 100);
		}
	} else if (St.byWallCount > 0) {
		hDC = g_pDibDrawMap->Lock ();
		for (i = 0; i < St.byWallCount && i < MAZEPARA_WALL_MAX; i ++) {
			int wx, wy, wcx, wcy;
			MazeWallRect (St.aWall[i].byX, St.aWall[i].byY, St.aWall[i].byOrient, sx, sy, &wx, &wy, &wcx, &wcy);
			MazeBrick (hDC, wx, wy, wcx, wcy);
		}
		g_pDibDrawMap->Unlock ();
	}

	/* 壁を置くカーソル（半透明の壁。置けない位置は赤い壁と ✕）。自分の番の時だけ */
	if (bSeat && (nMode == MAZEMODE_WALL) && !bWin && bMyTurn) {
		int wx, wy, wcx, wcy;
		BOOL bOk = (St.byWallLeft[nSeat] > 0) && MazeCanPlaceWall (&St, nCurX, nCurY, nCurO);
		MazeWallRect (nCurX, nCurY, nCurO, sx, sy, &wx, &wy, &wcx, &wcy);
		if (bWallArt) {
			MazeWallArt (wx, wy, nCurO, nSeat, 100);		/* 壁の絵に明るい色を重ねて、置いた壁と見分ける（半透明だと隙間の土に紛れる） */
		}
		hDC = g_pDibDrawMap->Lock ();
		if (bOk) {
			MazeRect (hDC, wx, wy, wcx, wcy, RGB (255, 235, 150), bWallArt ? 120 : 150);
		} else {
			MazeRect (hDC, wx, wy, wcx, wcy, RGB (255, 70, 60), 120);
			int mx = wx + wcx / 2, my = wy + wcy / 2, k;
			for (k = -6; k <= 6; k ++) {
				MazeRect (hDC, mx + k - 1, my + k - 1, 3, 3, RGB (255, 40, 40), 255);
				MazeRect (hDC, mx + k - 1, my - k - 1, 3, 3, RGB (255, 40, 40), 255);
			}
		}
		g_pDibDrawMap->Unlock ();
	}
}

static void DrawMazeOne(const CMDPARA_MAZESTATE &St, int sx, int sy, BOOL bTop)
{
	BOOL bSeat = (St.byState & MAZESTATE_SEAT) ? TRUE : FALSE;
	BOOL bWin = (St.byState & MAZESTATE_WIN) ? TRUE : FALSE;
	BOOL bVs = (St.byState & MAZESTATE_VS) ? TRUE : FALSE;
	BOOL bCpuMatch = (St.byState & MAZESTATE_CPU) ? TRUE : FALSE;
	int nMode, nCurX, nCurY, nCurO;
	HDC hDC;
	HFONT hFontOld;
	char szTmp[96];

	if (!bTop) {
		DrawMazeBoard (St, sx, sy);
		return;
	}
	GetMazeCursor (&nMode, &nCurX, &nCurY, &nCurO);
	s_nPzL = s_nMazeL;
	s_nPzT = s_nMazeT;
	s_nPzR = s_nMazeR;
	s_nPzB = s_nMazeB;

	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
	SetBkMode (hDC, TRANSPARENT);

	/* ここから最前面の文字。2 人対戦中は、座っている人にも見ている人にも手番・残り秒・壁の残りを出す。
	   ひとり練習は座っている本人だけ（待機中や、片方だけ座っている間の見ている人には従来どおり盤だけ） */
	if (bSeat || bVs) {
		SIZE size;
		int nSeat = bSeat ? max (0, GetMazeMySeat (&St)) : 0;
		BOOL bMyTurn = (!bVs || ((int)St.byTurn == nSeat)) && !(St.byState & MAZESTATE_COUNT);
		char szWho[64];
		const char *pszOpp = bCpuMatch ? "CPU" : "相手";
		PCHARINFO pTurn = NULL;
		int nNo;

		/* 手番の人の名前（dwTurnChar のキャラ。いなければ「相手」。CPU対戦中のCPUの手番は「CPU」） */
		lstrcpy (szWho, bCpuMatch ? "CPU" : "相手");
		nNo = (St.dwTurnChar != MAZE_NOCHAR) ? GetCharNo2 (St.dwTurnChar) : -1;
		if (nNo >= 0) {
			pTurn = g_aCharInfo[nNo];
			wsprintf (szWho, "%.20s", pTurn->szName);
		}
		/* 手番と残り秒（盤の上）。壁の残りは自分側・相手側の順（見ている人はプレイヤー 0・1 の順） */
		if (bVs && (St.byState & MAZESTATE_COUNT)) {
			wsprintf (szTmp, "まもなく対戦開始");
		} else if (bVs && (St.byState & MAZESTATE_WIN)) {
			wsprintf (szTmp, "勝負あり");
		} else {
			char szTime[24] = "";
			if (bVs && (St.byTimeLeft > 0)) {
				wsprintf (szTime, "  残り %d 秒", (int)St.byTimeLeft);
			}
			if (bSeat) {
				wsprintf (szTmp, "%s%s   壁 あなた %d / %s %d", bMyTurn ? "あなたの番" : "", szTime, (int)St.byWallLeft[nSeat], pszOpp, (int)St.byWallLeft[1 - nSeat]);
				if (!bMyTurn) {
					wsprintf (szTmp, "%sの番%s   壁 あなた %d / %s %d", szWho, szTime, (int)St.byWallLeft[nSeat], pszOpp, (int)St.byWallLeft[1 - nSeat]);
				}
			} else {
				wsprintf (szTmp, "%sの番%s   壁 %d / %d", szWho, szTime, (int)St.byWallLeft[0], (int)St.byWallLeft[1]);
			}
		}
		GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
		PuzFrame (hDC, sx + (MAZE_BOARD_DOT - size.cx) / 2 - 6, sy - 38, size.cx + 12, size.cy + 4, RGB (200, 200, 200), 170);
		PuzText (hDC, sx + MAZE_BOARD_DOT / 2, sy - 36, szTmp, (bSeat && bMyTurn && bVs) ? RGB (150, 255, 160) : RGB (255, 235, 120), size.cy, TRUE);

		/* 操作ヒント（盤の下。座っている人だけ） */
		if (bSeat) {
			const char *pszHint = (bVs && !bMyTurn) ? (bCpuMatch ? "CPUの番です  ESC：やめる" : "相手の番です  ESC：やめる") :
				(nMode == MAZEMODE_WALL) ? "矢印：ばしょ  Z：むき  X：おく  ESC：もどる" : "矢印：うごく  X：かべ  ESC：やめる";
			if (bVs && (St.byState & MAZESTATE_COUNT)) {
				pszHint = "じゅんびしてね  ESC：やめる";
			}
			GetTextExtentPoint32 (hDC, pszHint, strlen (pszHint), &size);
			PuzFrame (hDC, sx + (MAZE_BOARD_DOT - size.cx) / 2 - 6, sy + MAZE_BOARD_DOT + 6, size.cx + 12, size.cy + 4, RGB (200, 200, 200), 170);
			PuzText (hDC, sx + MAZE_BOARD_DOT / 2, sy + MAZE_BOARD_DOT + 8, (LPSTR)pszHint, RGB (255, 255, 255), size.cy, TRUE);
		}
	}
	/* CPU対戦の台の案内（盤の外の上に小さく。待機中と、CPU対戦中を見ている人に。座っている人には手番表示があるので出さない） */
	if ((St.byCpuLevel >= 1) && (St.byCpuLevel <= 3) && ((St.byState & MAZESTATE_IDLE) || (bCpuMatch && !bSeat))) {
		static const char *s_apszLv[4] = { "", "よわい", "ふつう", "つよい" };
		SIZE size;
		int yy = bCpuMatch ? (sy + MAZE_BOARD_DOT + 6) : (sy - 24);		/* 対戦中は手番表示が上にあるので下に */
		wsprintf (szTmp, "CPU対戦（%s）", s_apszLv[St.byCpuLevel]);
		GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
		PuzFrame (hDC, sx + (MAZE_BOARD_DOT - size.cx) / 2 - 6, yy, size.cx + 12, size.cy + 4, RGB (200, 200, 200), 150);
		PuzText (hDC, sx + MAZE_BOARD_DOT / 2, yy + 2, szTmp, RGB (190, 230, 255), size.cy, TRUE);
	}
	/* 開始前のカウントダウン（盤の真ん中に大きく 3・2・1） */
	if ((St.byState & MAZESTATE_COUNT) && (St.byCount > 0)) {
		wsprintf (szTmp, "%d", (int)St.byCount);
		SelectObject (hDC, g_hFont32);
		PuzFrame (hDC, sx + MAZE_BOARD_DOT / 2 - 40, sy + MAZE_BOARD_DOT / 2 - 36, 80, 72, RGB (255, 235, 120), 190);
		PuzText (hDC, sx + MAZE_BOARD_DOT / 2, sy + MAZE_BOARD_DOT / 2 - 16, szTmp, RGB (255, 235, 120), 32, TRUE);
		SelectObject (hDC, g_hFont14);
	}
	/* 勝敗（盤の真ん中に大きく。座っている人には勝ち・負け、見ている人には勝者の名前） */
	if (bWin && (St.cWinner >= 0)) {
		char szWin[64];
		BOOL bWon;
		int nSeat = bSeat ? max (0, GetMazeMySeat (&St)) : 0;
		if (bSeat && bVs) {
			bWon = ((int)St.cWinner == nSeat);
			lstrcpy (szWin, bWon ? "WIN!" : "LOSE...");
		} else if (bSeat) {
			bWon = ((int)St.cWinner == nSeat);
			lstrcpy (szWin, bWon ? "勝ち！" : "負け…");
		} else {
			int nNo = (St.dwChar[St.cWinner] != MAZE_NOCHAR) ? GetCharNo2 (St.dwChar[St.cWinner]) : -1;
			bWon = TRUE;
			if (bCpuMatch && (St.dwChar[St.cWinner] == MAZE_NOCHAR)) {
				lstrcpy (szWin, "CPU WIN!");
			} else if (nNo >= 0) {
				wsprintf (szWin, "%.16s WIN!", g_aCharInfo[nNo]->szName);
			} else {
				lstrcpy (szWin, "WIN!");
			}
		}
		SelectObject (hDC, g_hFont32);
		PuzFrame (hDC, sx + MAZE_BOARD_DOT / 2 - 100, sy + MAZE_BOARD_DOT / 2 - 28, 200, 56, RGB (255, 235, 120), 190);
		PuzText (hDC, sx + MAZE_BOARD_DOT / 2, sy + MAZE_BOARD_DOT / 2 - 16, szWin, bWon ? RGB (255, 235, 120) : RGB (255, 120, 110), 32, TRUE);
		SelectObject (hDC, g_hFont14);
	}
	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

static void DrawMaze(int nX, int nY, BOOL bTop)
{
	CMDPARA_MAZESTATE St;
	int i, sx, sy;

	if ((g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}
	for (i = 0; i < MAZE_VIEW_MAX; i ++) {
		if (!GetMazeState (i, &St)) {
			continue;
		}
		/* マップのドット座標 → 描画用の画面（地図のパーツ・キャラと同じ座標。基準のマスが (DRAW_PARTS_X+1)*32） */
		sx = (DRAW_PARTS_X + 1) * 32 + (int)St.sBoardX - CamTileX () * 32;
		sy = (DRAW_PARTS_Y + 1) * 32 + (int)St.sBoardY - CamTileY () * 32;
		/* 画面に少しもかからない台は描かない */
		if ((sx + MAZE_BOARD_DOT + 8 <= nX) || (sx - 8 >= nX + SCRSIZEX) ||
			(sy + MAZE_BOARD_DOT + 40 <= nY) || (sy - 42 >= nY + SCRSIZEY)) {
			continue;
		}
		s_nMazeL = nX;
		s_nMazeT = nY;
		s_nMazeR = nX + SCRSIZEX;
		s_nMazeB = nY + SCRSIZEY;
		if (bTop) {
			DrawMazePieces (St, sx, sy);
		}
		DrawMazeOne (St, sx, sy, bTop);
	}
}

/* ========================================================================= */
/* 関数名：DrawDefenseBand													 */
/* 内容：番外編の拠点防衛の挑戦中だけ、画面の上の中央にパネルを出す			 */
/*		 1行目「拠点HP [バー] 720/1000」（残り割合で緑→黄(50%以下)→赤(25%以下)）	 */
/*		 2行目 ウェーブの丸（済み=緑・今=橙で番号・未=暗）と、残り敵数を大きく	 */
/*		 準備中は「次のウェーブまで N 秒」。勝ち・負けは文言を入れ替える		 */
/*		 状態の変わり目で、中央の大見出し（WAVE n/N・勝ち・負け）を出す		 */
/*		 後ろに半透明の暗い帯を敷く。スマホで縮小されても読めるよう大きめに	 */
/* ========================================================================= */
static void DrawDefenseBand(int nX, int nY)
{
	const int nWidth = 316;		/* パネルの幅 */
	const int nHeight = 76;		/* パネルの高さ */
	const int nBarX = 62;		/* HPバーの左（パネルの左から） */
	const int nBarW = 150;		/* HPバーの幅 */
	CMDPARA_DGDEFSTATUS St;
	char szTmp[96];
	HDC hDC;
	HFONT hFontOld;
	COLORREF clBar;
	BOOL bActive;
	DWORD dwNow = timeGetTime ();
	int i, yy, x0, nPct, nFill, nHP, nMax, nY0;

	bActive = GetDefenseStatus (&St);
	DefenseHudWatch (bActive, &St);
	if (!bActive || !s_bDefHudShow) {
		return;
	}
	nMax = max ((int)St.wBaseMaxHP, 1);
	nHP = min ((int)St.wBaseHP, nMax);
	nPct = DefenseHpPercent (&St);
	if (nPct <= 25) {
		clBar = RGB (230, 60, 50);		/* 赤 */
	} else if (nPct <= 50) {
		clBar = RGB (240, 200, 50);		/* 黄 */
	} else {
		clBar = RGB (70, 200, 80);		/* 緑 */
	}
	nFill = nBarW * nHP / nMax;
	if (nHP > 0) {
		nFill = max (nFill, 1);
	}
	x0 = nX + (SCRSIZEX - nWidth) / 2;
	nY0 = nY + 4;

	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont14);
	SetBkMode (hDC, TRANSPARENT);

	/* 後ろの帯 */
	DrawDefenseFrame (hDC, x0, nY0, nWidth, nHeight, RGB (200, 200, 200), 150);

	/* 1行目：拠点HP */
	TextOut2 (hDC, x0 + 8, nY0 + 6, (LPSTR)"拠点HP", RGB (255, 255, 255));
	for (yy = 0; yy < 14; yy ++) {
		W32ShadeSpan (hDC, x0 + nBarX - 1, nY0 + 6 + yy, nBarW + 2, 0, RGB (230, 230, 230), 255);
		W32ShadeSpan (hDC, x0 + nBarX, nY0 + 7 + yy, nBarW, 0, RGB (50, 20, 20), 255);
	}
	for (yy = 0; yy < 12; yy ++) {
		if (nFill > 0) {
			W32ShadeSpan (hDC, x0 + nBarX, nY0 + 7 + yy, nFill, 0, clBar, 255);
		}
	}
	wsprintf (szTmp, "%d/%d", nHP, nMax);
	TextOut2 (hDC, x0 + nBarX + nBarW + 8, nY0 + 6, szTmp, RGB (255, 255, 255));

	/* 2行目：ウェーブの丸 */
	{
		int nWaves = max ((int)St.byWaveMax, 1);
		int nStep = min (22, 150 / nWaves);
		int nR = (nStep >= 20) ? 8 : max (nStep / 2 - 1, 3);
		int nDotX = x0 + 8 + nR;
		int nDotY = nY0 + 50;
		int nNow = (St.byState == DGDEFSTATUS_PREP || St.byState == DGDEFSTATUS_WAVE) ? (int)St.byWave : 0;

		for (i = 1; i <= nWaves; i ++) {
			COLORREF cl;
			BOOL bDone;

			if (St.byState == DGDEFSTATUS_WIN) {
				bDone = TRUE;
			} else if (St.byState == DGDEFSTATUS_LOSE) {
				bDone = (i < (int)St.byWave);
			} else if (St.byState == DGDEFSTATUS_PREP) {
				bDone = (i < nNow);		/* 準備中は次に来る波が「今」 */
			} else {
				bDone = (i < nNow);
			}
			DrawDefenseDot (hDC, nDotX, nDotY, nR + 1, RGB (230, 230, 230));
			if (bDone) {
				cl = RGB (70, 200, 80);
			} else if ((i == nNow) && (St.byState == DGDEFSTATUS_WAVE)) {
				cl = RGB (245, 150, 40);
			} else if ((i == nNow) && (St.byState == DGDEFSTATUS_PREP) && ((dwNow / 500) % 2)) {
				cl = RGB (120, 80, 40);
			} else {
				cl = RGB (45, 45, 55);
			}
			DrawDefenseDot (hDC, nDotX, nDotY, nR - 1, cl);
			if ((i == nNow) && (St.byState == DGDEFSTATUS_WAVE) && (nR >= 7)) {
				wsprintf (szTmp, "%d", i);
				DrawDefenseTextCenter (hDC, nDotX, nDotY - 7, szTmp, RGB (255, 255, 255));
			}
			nDotX += nStep;
		}
	}
	/* 区切り */
	for (yy = 0; yy < 44; yy ++) {
		W32ShadeSpan (hDC, x0 + 8 + 160, nY0 + 28 + yy, 1, 0, RGB (200, 200, 200), 140);
	}

	/* 2行目の右：残り敵数（大きく）・準備中の秒・結果 */
	switch (St.byState) {
	case DGDEFSTATUS_PREP:
		TextOut2 (hDC, x0 + 176, nY0 + 28, (LPSTR)"次のウェーブまで", RGB (255, 255, 255));
		SelectObject (hDC, g_hFont32);
		wsprintf (szTmp, "%d", (int)St.wRestSec);
		TextOut2 (hDC, x0 + 176, nY0 + 40, szTmp, RGB (255, 235, 120));
		{
			SIZE size;
			GetTextExtentPoint32 (hDC, szTmp, strlen (szTmp), &size);
			SelectObject (hDC, g_hFont14);
			TextOut2 (hDC, x0 + 176 + size.cx + 4, nY0 + 56, (LPSTR)"秒", RGB (255, 235, 120));
		}
		SelectObject (hDC, g_hFont14);
		break;
	case DGDEFSTATUS_WAVE:
		TextOut2 (hDC, x0 + 176, nY0 + 50, (LPSTR)"残り", RGB (255, 255, 255));
		SelectObject (hDC, g_hFont32);
		wsprintf (szTmp, "%d", (int)St.wEnemy);
		TextOut2 (hDC, x0 + 176 + 36, nY0 + 38, szTmp, RGB (255, 255, 255));
		SelectObject (hDC, g_hFont14);
		break;
	case DGDEFSTATUS_WIN:
		TextOut2 (hDC, x0 + 176, nY0 + 48, (LPSTR)"全部しのいだ！", RGB (140, 255, 150));
		break;
	default:
		TextOut2 (hDC, x0 + 176, nY0 + 48, (LPSTR)"拠点が落ちた…", RGB (255, 110, 100));
		break;
	}

	/* 拠点が危ない：パネルの下に知らせ（赤い縁と同じ周期でゆっくり点滅） */
	if (IsDefenseDanger (&St)) {
		const int nWarnW = 188;
		int nWx = x0 + (nWidth - nWarnW) / 2;
		COLORREF clWarn = ((dwNow / 600) % 2) ? RGB (255, 120, 100) : RGB (255, 235, 200);

		DrawDefenseFrame (hDC, nWx, nY0 + nHeight + 3, nWarnW, 20, RGB (200, 60, 50), 150);
		DrawDefenseTextCenter (hDC, nWx + nWarnW / 2, nY0 + nHeight + 6, (LPSTR)"拠点が攻撃されています！", clWarn);
		g_bDraw = TRUE;
	}

	/* 中央の大見出し（WAVE n/N・勝ち・負け）。スッと開いて、スッと閉じる */
	if (s_byDefBanner != DEFHUD_BANNER_NONE) {
		DWORD dwLife = (s_byDefBanner == DEFHUD_BANNER_KIND_WAVE) ? DEFHUD_BANNER_WAVE : DEFHUD_BANNER_RESULT;
		DWORD dwAge = dwNow - s_dwDefBannerStart;

		if (dwAge >= dwLife) {
			s_byDefBanner = DEFHUD_BANNER_NONE;
		} else {
			const int nBannerW = 400;
			const int nBannerH = 74;
			int nOpen = 100;
			int nH, bx, by, nMid;
			COLORREF clFrame, clText;

			if (dwAge < DEFHUD_BANNER_OPEN) {
				nOpen = (int)(dwAge * 100 / DEFHUD_BANNER_OPEN);
			} else if (dwAge > dwLife - DEFHUD_BANNER_CLOSE) {
				nOpen = (int)((dwLife - dwAge) * 100 / DEFHUD_BANNER_CLOSE);
			}
			nH = max (nBannerH * nOpen / 100, 2);
			nMid = nY + SCRSIZEY / 2 - 80;
			bx = nX + (SCRSIZEX - nBannerW) / 2;
			by = nMid - nH / 2;
			switch (s_byDefBanner) {
			case DEFHUD_BANNER_KIND_WIN:	clFrame = RGB (120, 230, 130);	clText = RGB (170, 255, 170);	break;
			case DEFHUD_BANNER_KIND_LOSE:	clFrame = RGB (230, 80, 70);	clText = RGB (255, 130, 120);	break;
			default:						clFrame = RGB (240, 200, 40);	clText = RGB (255, 255, 255);	break;
			}
			DrawDefenseFrame (hDC, bx, by, nBannerW, nH, clFrame, 170);
			if (nOpen >= 70) {
				SelectObject (hDC, g_hFont32);
				if (s_byDefBanner == DEFHUD_BANNER_KIND_WAVE) {
					wsprintf (szTmp, "WAVE %d/%d", (int)s_byDefBannerWave, (int)s_byDefBannerWaveMax);
					DrawDefenseTextCenter (hDC, nX + SCRSIZEX / 2, nMid - 30, szTmp, clText);
					SelectObject (hDC, g_hFont14);
					DrawDefenseTextCenter (hDC, nX + SCRSIZEX / 2, nMid + 12, (LPSTR)"敵が来るぞ！", RGB (255, 255, 255));
				} else {
					DrawDefenseTextCenter (hDC, nX + SCRSIZEX / 2, nMid - 16,
						(LPSTR)((s_byDefBanner == DEFHUD_BANNER_KIND_WIN) ? "全部しのいだ！" : "拠点が落ちた…"), clText);
				}
				SelectObject (hDC, g_hFont14);
			}
			g_bDraw = TRUE;
		}
	}

	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

/* ========================================================================= */
/* 関数名：DrawScreen														 */
/* 内容：画面の描画															 */
/* 日付：2003/04/29															 */
/* ========================================================================= */
void DrawScreen(HWND hWnd, HDC hDC)
{
	if (!g_pDibPrimary) {
		return;
	}

	BOOL bRet = TRUE;
	switch (g_byGameState) {
	case GAMESTATE_LOGO:		DrawScreenLOGO ();			break;
	case GAMESTATE_TITLE:		DrawScreenTITLE ();			break;
	case GAMESTATE_LOGIN:		DrawScreenLOGIN ();			break;
	case GAMESTATE_CONNECTLOST:	DrawScreenLOGIN ();			break;
	case GAMESTATE_MOVE:
		{
			if (g_bMainWindowView == FALSE) {
				return;
			}

			bRet = FALSE;
			if (g_FadeInOutInfo.byFadeOut == 0) {
				bRet = DrawScreenMOVE ();
			}
		}
		break;
	default:	return;
	}

	if (g_byGameState == GAMESTATE_MOVE) {
		if (bRet || (!bRet && g_FadeInOutInfo.byFadeOut)) {
			PCHARINFO pChar;
			int i, nCount, x = 0, y = 0;
			HDC hDCPrimary;

			if (bRet) {
				pChar = g_aCharInfo[g_nMyCharID];
				nCount = g_SysMsgInfo.GetSize ();
				x = CamOfsX ();
				y = CamOfsY ();

				/* 拠点が危ない時の赤い縁（ウィンドウの下に敷く） */
				DrawDefenseDanger (32 - x, 32 - y);

				/* 特技ショートカットの帯（ウィンドウの下に敷く） */
				DrawMagicKeyBand (pChar, 32 - x, 32 - y);

				/* 探索マップ（ウィンドウの下に重ねる） */
				AutoMapDraw (32 - x, 32 - y);

				/* 拠点防衛の帯（上の中央。挑戦中だけ。コマンドなどのウィンドウが隠れないよう、ウィンドウの下に敷く） */
				DrawDefenseBand (32 - x, 32 - y);

				/* 拠点防衛以外の番外編ダンジョンの状況パネル（左上。挑戦中だけ） */
				DrawDungeonPanel (32 - x, 32 - y);

				/* 落ち物パズルの帯・待ちお邪魔のバー・文字（盤面本体は DrawScreenMOVE でキャラの前に描いてある。ウィンドウの下に敷く） */
				DrawPuzzle (32 - x, 32 - y, TRUE);
				DrawPinball (32 - x, 32 - y, TRUE);

				/* SBOじゃんの名前札・結果・本人の手牌の別枠（卓本体は DrawScreenMOVE でキャラの前に描いてある。ウィンドウの下に敷く） */
				DrawJan (32 - x, 32 - y, TRUE);
				/* 迷路対戦の手番・壁の残り・勝敗の文字（盤本体は DrawScreenMOVE で描いてある） */
				DrawMaze (32 - x, 32 - y, TRUE);

				hDCPrimary = g_pDibDrawMap->Lock ();
				HFONT hFontOld = (HFONT)SelectObject (hDCPrimary, g_hFont14);
				SetBkMode (hDCPrimary, TRANSPARENT);
				DrawSysMsgBand (hDCPrimary, 32 - x, 32 - y);
				int nOfs = GetSysMsgBottomOfs ();
				for (i = 0; i < nCount; i ++) {
					PSYSMSGINFO pSysMsgInfo = g_SysMsgInfo[i];
					if (pSysMsgInfo->nY >= SYSMSG_EY - nOfs) {
						TextOut2 (hDCPrimary, 32 - x, 32 - y + pSysMsgInfo->nY, pSysMsgInfo->pszMsg, pSysMsgInfo->cl);
					}
				}
				SelectObject (hDCPrimary, hFontOld);
				g_pDibDrawMap->Unlock ();

				/* ウィンドウを表示（帯・戦闘ログより上に重ねる） */
				DrawWindow (g_pDibDrawMap, 32 - x, 32 - y);
			}

			BYTE byLV = 0;
			/* フェードインかフェードアウト中？ */
			if (g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut) {
				if (g_FadeInOutInfo.byFadeIn && g_FadeInOutInfo.byFadeIn != 255) {
					byLV = g_FadeInOutInfo.byFadeIn;

				} else {
					byLV = g_FadeInOutInfo.byFadeOut;
				}
				g_pDibDrawMap->ChgLevel (32 - x, 32 - y, SCRSIZEX, SCRSIZEY, byLV);
			}

			/* マップ名表示？ */
			if (g_byTextFadeInOut && byLV == 0 && g_pMapInfo) {
				/* 気まぐれダンジョンの中ではテーマの名前を出す */
				LPCSTR pszMapName = AutoMapGetName ();
				if (pszMapName == NULL) {
					pszMapName = (LPCSTR)g_pMapInfo->pszName;
				}
				if (pszMapName) {
					pChar = g_aCharInfo[g_nMyCharID];
					BYTE byLv = 0;
					if (g_byTextFadeInOut < 10) {
						byLv = 100 - (g_byTextFadeInOut * 10);
					} else if (g_byTextFadeInOut < 30) {
						byLv = 0;
					} else if (g_byTextFadeInOut < 41) {
						byLv = (g_byTextFadeInOut - 30) * 10;
					} else {
						byLv = 100;
					}

					pDibDrawTmp.FillRect (0, 0, (SboTextWidth ((LPSTR)pszMapName) + 1) * 16, 66, RGB (0, 0, 0));

					HDC hDC = pDibDrawTmp.Lock ();
					HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFont32);
					SetBkMode (hDC, TRANSPARENT);

					TextOut2 (hDC, 0, 0, (LPSTR)pszMapName, RGB (255, 255, 255));

					SelectObject (hDC, hFontOld);
					pDibDrawTmp.Unlock ();
					int xTmp = (SCRSIZEX / 2) - (SboTextWidth ((LPSTR)pszMapName) * 8);
					xTmp = max (xTmp, 0);
					g_pDibDrawMap->BltAlpha (32 - x + xTmp,
								32 - y + 5 * 32, (SboTextWidth ((LPSTR)pszMapName) + 1) * 16, 64, &pDibDrawTmp, 0, 0, byLv, TRUE);
				}
			}

			hDCPrimary = g_pDibDrawMap->Lock ();
			BitBlt (hDC, 0, 0, SCRSIZEX, SCRSIZEY, hDCPrimary, 32 - x, 32 - y, SRCCOPY);
			g_pDibDrawMap->Unlock ();
		}

	} else {
		/* ウィンドウを表示 */
		DrawWindow ();

		/* フェードインかフェードアウト中？ */
		if (g_FadeInOutInfo.byFadeIn || g_FadeInOutInfo.byFadeOut) {
			BYTE byLV;
			if (g_FadeInOutInfo.byFadeIn && g_FadeInOutInfo.byFadeIn != 255) {
				byLV = g_FadeInOutInfo.byFadeIn;

			} else {
				byLV = g_FadeInOutInfo.byFadeOut;
			}
			g_pDibPrimary->ChgLevel (0, 0, SCRSIZEX, SCRSIZEY, byLV);
		}

		HDC hDCPrimary = g_pDibPrimary->Lock ();

		BitBlt (hDC, 0, 0, SCRSIZEX, SCRSIZEY, hDCPrimary, 0, 0, SRCCOPY);
		g_pDibPrimary->Unlock ();
	}

	/* 次に変更されるまでは描画しない */
	g_bDraw = FALSE;
}

/* ========================================================================= */
/* 関数名：DrawScreenLOGO													 */
/* 内容：ロゴ画面の描画														 */
/* 日付：2003/10/14															 */
/* ========================================================================= */
void DrawScreenLOGO(void)
{
	g_pDibPrimary->Blt (SCRSIZEX / 2 - 183, SCRSIZEY / 2 - 67, 366, 134, g_pDibLogo, 0, 0);
}

/* ========================================================================= */
/* 関数名：DrawScreenTITLE													 */
/* 内容：タイトル画面の描画													 */
/* 日付：2003/10/14															 */
/* ========================================================================= */
void DrawScreenTITLE(void)
{
	HDC hDC;
	HFONT hFontOld;
	char szBuf[256];

	g_pDibPrimary->Clear ();
	g_pDibPrimary->Blt (SCRSIZEX / 2 - 192, SCRSIZEY / 2 - 132 - 60, 384, 275, g_pDibTitle, 0, 0, TRUE);

	hDC = g_pDibPrimary->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFontBasic);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "スクラップブックオンライン");
	TextOut2 (hDC, SCRSIZEX / 2 - (SboTextWidth (szBuf) * 8) / 2 - 8, SCRSIZEY / 2 + 52, szBuf, RGB (255, 255, 255));
	wsprintf (szBuf, "Presented By URARA-works 2003-2007,2026-");
	TextOut2 (hDC, SCRSIZEX / 2 - (SboTextWidth (szBuf) * 8) / 2 - 8, SCRSIZEY / 2 - 85 + 82 + 218, szBuf, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	g_pDibPrimary->Unlock ();
}

/* ========================================================================= */
/* 関数名：DrawScreenLOGIN													 */
/* 内容：ログイン画面の描画													 */
/* 日付：2003/05/01															 */
/* ========================================================================= */
void DrawScreenLOGIN(void)
{
	HDC hDC;
	HFONT hFontOld;
	char szBuf[256];

	g_pDibPrimary->Clear ();

	/* プヨンを表示 */
	int x, x2 = 0, cx = 32;
	int nX1, nX2;

	x = g_nLoginAnimeX;
	if (x < 0) {
		x2 -= g_nLoginAnimeX;
		cx += g_nLoginAnimeX;
		cx = max (cx, 1);
	}
	x = max (x, 0);
	g_pDibPrimary->Blt (x, SCRSIZEY - 33 - 8, cx, 32, g_pDibAnime, (g_byAnimeNo % 8) * 32 + x2, 0, TRUE);
	nX1 = x - SboTextWidth ("スクラップブックオンライン") * 6;
	nX2 = nX1 - 48;

	hDC = g_pDibPrimary->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "スクラップブックオンライン");
	TextOut2 (hDC, nX1, SCRSIZEY - 24 - 8, szBuf, RGB (255, 255, 255));
	wsprintf (szBuf, "Presented By URARA-works 2003-2007,2026-");
	TextOut2 (hDC, nX2, SCRSIZEY - 12 - 8, szBuf, RGB (255, 255, 255));

	SelectObject (hDC, hFontOld);
	g_pDibPrimary->Unlock ();
}

/* ========================================================================= */
/* 関数名：DrawScreenMOVE													 */
/* 内容：歩き中画面の描画													 */
/* 日付：2003/05/04															 */
/* ========================================================================= */
BOOL DrawScreenMOVE(void)
{
	if (g_nMyCharID < 0) {
		return FALSE;
	}

	if (g_aCharInfo.GetSize () == 0) {
		return FALSE;
	}

	int i;
	BOOL bLight;
	char aMapDark[SCRMARTSSIZE];

	/* 暗いマップは、ひとまず見えるマスを明るいまま描いておき、
	   最後に DrawMapLight で丸い灯りの形に暗くする */
	bLight = (GetMapLightLevel () != 0);
	if (bLight) {
		CopyMemory (aMapDark, g_pMapDarkTmp, SCRMARTSSIZE);
		for (i = 0; i < SCRMARTSSIZE; i ++) {
			g_pMapDarkTmp[i] = (aMapDark[i] > 1) ? 100 : 0;
		}
	}

	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawMapLine (i, -1);
	}

	DrawObj ();				/* オブジェクトを描画 */
	/* 落ち物パズルの盤面本体（井戸・枠・ブロック）は、地面のパーツの上・キャラの下に描く。
	   台の下に立つキャラの頭が盤面に隠れないように（帯・文字は最前面で DrawScreen が描く） */
	{
		DrawPuzzle (32 - CamOfsX (), 32 - CamOfsY (), FALSE);
		DrawPinball (32 - CamOfsX (), 32 - CamOfsY (), FALSE);
		DrawMaze (32 - CamOfsX (), 32 - CamOfsY (), FALSE);
		/* SBOじゃんの卓本体（面・手牌・川）も同じ重なり順 */
		DrawJan (32 - CamOfsX (), 32 - CamOfsY (), FALSE);
	}
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawCharLine (i);
		DrawMapLine (i, PARTSHIT_BRIDGEV);	/* 橋の下をくぐる人に橋を重ねる */
		DrawBridgeOverHead (i);				/* 上の行にはみ出した頭にも */
		DrawMapLine (i, PARTSHIT_HIDE3);
		if (i > 0) {
			DrawMapLine (i - 1, PARTSHIT_HIDE1);
			if (i > 1) {
				if (i == DRAW_PARTS_Y * 2 + 3 - 1) {
					DrawMapLine (i, PARTSHIT_HIDE1);
				}
			}
		}
	}
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawMapLine (i, PARTSHIT_HIDE5);
	}
	/* 橋の上を渡っている人は橋より手前に描き直す */
	s_bDrawCharBridgeUpper = TRUE;
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawCharLine (i);
	}
	s_bDrawCharBridgeUpper = FALSE;
	DrawWeather ();
	DrawMapViewFog ();		/* 見えない所のパーツを使うマップは、見えない所を白くかすませる */
	if (bLight) {
		DrawMapLight (aMapDark);	/* 丸くゆらめく灯りで暗くする */
		CopyMemory (g_pMapDarkTmp, aMapDark, SCRMARTSSIZE);
	}
	DrawCharLightGlow ();	/* 明かりを使っている人の体をぼんやり光らせる(暗幕の後に描く) */
	DrawChar2 ();			/* エフェクトを描画 */
	DrawEvent ();			/* イベント情報を描画 */
	DrawMapEdit ();			/* マップ編集情報を描画 */
	DrawNpcRoute ();		/* NPCの巡回情報を描画 */
	DrawMapLightEdit ();	/* 明るさの範囲・光源の編集中の印を描画 */
	DrawMapSwapEdit ();		/* 印で変わるマス・宝箱の編集中の枠を描画 */
	DrawChar3 ();			/* 文字を描画 */
	DrawDropEffects ();		/* アイテムドロップ演出を描画 */
	DrawETC ();				/* ゲージ等の描画 */

	if (g_hWndDebug) {
		InvalidateRect (g_hWndDebug, NULL, TRUE);
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	DrawMapViewFog													 */
/* 内容：	見える範囲の外に白い霧をかける									 */
/* 			見えない所のパーツ(草原など)が見えている地面と同じでも、		 */
/* 			どこまで見えているか分かるようにする							 */
/* 日付：	2026/11/09														 */
/* ========================================================================= */
#define VIEW_FOG_HIDE		55		/* まだ見ていない所の白さ(%) */
#define VIEW_FOG_SEEN		35		/* 一度見た所の白さ(%) */
static void DrawMapViewFog(void)
{
	int x, y, nCX, nCY;
	char cLevel;

	if (!IsMapViewMode () || (g_pMapInfo == NULL) || (g_pMapInfo->wViewHideParts == 0)) {
		return;
	}
	nCX = DRAW_PARTS_X * 2 + 3;
	nCY = DRAW_PARTS_Y * 2 + 3;
	for (y = 0; y < nCY; y ++) {
		for (x = 0; x < nCX; x ++) {
			cLevel = g_pMapDarkTmp[x + nCX * y];
			if (cLevel >= 100) {
				continue;
			}
			g_pDibDrawMap->ChgWhite (x * 32, y * 32, 32, 32, (cLevel <= 0) ? VIEW_FOG_HIDE : VIEW_FOG_SEEN);
		}
	}
}


/* ========================================================================= */
/* 関数名：	DrawMapLight													 */
/* 内容：	丸くゆらめく灯りの描画											 */
/*			昔はマスごとに段階で暗くしていたので、灯りが四角くカクカクして	 */
/*			いた。描き終わった画面を1ドットずつ、灯りからの距離でなめらかに	 */
/*			暗くする。どのマスが見えるか(扉や壁の向こうは見えない)は今まで	 */
/*			どおり pMapDark(RenewMapDark で作ったもの)で決める				 */
/* ========================================================================= */
#define MAPLIGHT_MAX	32									/* 灯りの最大数 */
#define LIGHTITEM_POWER	1.6f								/* 明かりの道具の強さ(1 より大きいと真ん中が全部明るい。1.6 なら半径の 4 割まで) */
#define LIGHTITEM_WARM	0.15f								/* 明かりの道具の炎の色の強さ(光源の暖色は 1。強いと水・洞窟の青が濁るので弱め) */

static void DrawMapLight(const char *pMapDark)
{
	int i, x, y, nCount, nLight, nWidth, nHeight, nCX, nCY, nSX, nSY, nMapW;
	int anX[MAPLIGHT_MAX], anY[MAPLIGHT_MAX];
	float afR[MAPLIGHT_MAX], afPower[MAPLIGHT_MAX], afWarm[MAPLIGHT_MAX], fFlicker, fTime;
	float afVisX[DRAW_PARTS_X * 2 + 3];
	const BYTE *apMask[MAPLIGHT_MAX];		/* 灯りごとに照らせたマス(無ければ見えるマス全体を使う) */
	BYTE abyHide[SCRMARTSSIZE];
	BOOL bHide;
	PCHARINFO pMyChar, pChar;
	LPBYTE pBits;
	DWORD dwTime;

	if (g_nMyCharID < 0) {
		return;
	}
	pBits = g_pDibDrawMap->GetBits ();
	if (pBits == NULL) {
		return;
	}
	nWidth	= g_pDibDrawMap->Width ();
	nHeight	= g_pDibDrawMap->Height ();
	nMapW	= DRAW_PARTS_X * 2 + 3;
	pMyChar	= g_aCharInfo[g_nMyCharID];

	/* たいまつのようなゆらめき(灯りの半径を少しだけ伸び縮みさせる) */
	dwTime = timeGetTime ();
	fTime = (float)(dwTime % 600000);
	fFlicker = 1.0f + 0.035f * sinf (fTime * 0.0071f) +
			   0.020f * sinf (fTime * 0.0173f + 1.7f) +
			   0.012f * sinf (fTime * 0.0311f + 0.4f);

	/* 灯りを集める(自分と、灯りを持っているキャラ) */
	nLight = 0;
	nCount = g_aCharInfo.GetSize ();
	for (i = -1; i < nCount; i ++) {
		int xTmp, yTmp;

		if (nLight >= MAPLIGHT_MAX) {
			break;
		}
		if (i < 0) {
			pChar = pMyChar;
		} else {
			pChar = g_aCharInfo[i];
			if ((pChar == pMyChar) || (pChar->byLightLevel == 0)) {
				continue;
			}
			/* 自分と違う部屋(隠す範囲)にいる人の灯りは見えない */
			if (IsLightOtherRoom ((int)pChar->dwX, (int)pChar->dwY)) {
				continue;
			}
		}
		xTmp = CamTileX () - (int)pChar->dwX;
		yTmp = CamTileY () - (int)pChar->dwY;
		if ((abs (xTmp) > DRAW_PARTS_X + 1) || (abs (yTmp) > DRAW_PARTS_Y + 1)) {
			continue;
		}
		/* 灯りの中心(歩き中のずれも入れて、なめらかについてくるように) */
		anX[nLight] = (DRAW_PARTS_X + 1) * 32 + 16 - xTmp * 32 - BowOffsetX(pChar);
		anY[nLight] = (DRAW_PARTS_Y + 1) * 32 + 16 - yTmp * 32 - BowOffsetY(pChar);
		/* 明るさが 0 になる距離(ドット)。灯りが強くて暗くならない時は 0 */
		afR[nLight] = 0.0f;
		afPower[nLight] = 1.0f;
		afWarm[nLight] = 0.0f;
		apMask[nLight] = GetLightMask (pChar, 0, 0);
		/* 明かりの道具を持っていれば広くなる(GetCharLightTiles。0 は全部見える) */
		afR[nLight] = GetCharLightTiles (pChar) * 32.0f * fFlicker;
		/* 明かりの道具・特技の灯りは、真ん中あたりを昼のように明るくして、炎の色を少し付ける。
		   広がりだけ増やしても、端がうっすら伸びるだけで明るくなった気がしなかった */
		if (pChar->byLightLevel) {
			afPower[nLight] = LIGHTITEM_POWER;
			afWarm[nLight] = LIGHTITEM_WARM;
		}
		/* キャラの灯り無しのマップでは、明かりの道具を持っていない自分は足元がうっすら見えるだけ */
		if ((pChar == pMyChar) && (g_pMapInfo->byFlg & MAPFLG_NOCHARLIGHT) && (pChar->byLightLevel == 0)) {
			afR[nLight] = 1.2f * 32.0f;
			afPower[nLight] = 0.6f;
		}
		nLight ++;
	}

	/* 光るマップパーツ(たいまつ・かがり火など)も灯りにする */
	{
		int mx, my;
		/* 画面の外にある灯りでも、照らす範囲が画面に入っていれば使う */
		for (y = -(DRAW_PARTS_Y + 1 + MAPLIGHT_RANGE_MAX); y <= DRAW_PARTS_Y + 1 + MAPLIGHT_RANGE_MAX; y ++) {
			my = CamTileY () + y;
			if ((my < 0) || (my >= g_pMapInfo->nCY)) {
				continue;
			}
			for (x = -(DRAW_PARTS_X + 1 + MAPLIGHT_RANGE_MAX); x <= DRAW_PARTS_X + 1 + MAPLIGHT_RANGE_MAX; x ++) {
				float fFlickerParts;
				int nRange;
				BYTE byPower, byFlg;

				if (nLight >= MAPLIGHT_MAX) {
					break;
				}
				mx = CamTileX () + x;
				if ((mx < 0) || (mx >= g_pMapInfo->nCX)) {
					continue;
				}
				/* 光るパーツか、マップの光源 */
				nRange = GetMapPosLight (mx, my, &byPower, &byFlg);
				if (nRange == 0) {
					continue;
				}
				if (IsLightOtherRoom (mx, my)) {
					continue;
				}
				/* 灯りが画面に届かない？ */
				if ((abs (x) > DRAW_PARTS_X + 1 + nRange) || (abs (y) > DRAW_PARTS_Y + 1 + nRange)) {
					continue;
				}
				/* マップのマスは描画用画像の中で動かない(歩き中は見る場所の方がずれる) */
				anX[nLight] = (DRAW_PARTS_X + 1 + x) * 32 + 16;
				anY[nLight] = (DRAW_PARTS_Y + 1 + y) * 32 + 16;
				/* 炎ごとに少しずつ違うゆらめき */
				fFlickerParts = 1.0f + 0.045f * sinf (fTime * 0.0093f + (mx * 7 + my * 13)) +
								0.025f * sinf (fTime * 0.0211f + (mx * 3 + my * 5));
				if (byFlg & MAPLIGHTFLG_NOFLICKER) {
					fFlickerParts = 1.0f;		/* ゆらめかない光源 */
				}
				afPower[nLight] = byPower / 100.0f;
				apMask[nLight] = GetLightMask (NULL, mx, my);
				afWarm[nLight] = (byFlg & MAPLIGHTFLG_WARM) ? 1.0f : 0.0f;
				/* 範囲のマスまで見えて、その少し先で真っ暗になる(RenewMapDarkParts と同じ) */
				afR[nLight] = (nRange + 1 + 0.2f) * 32.0f * fFlickerParts;
				nLight ++;
			}
		}
	}

	/* 「明るく壁で隠す」範囲にいる時は、範囲の中のマスに印を付ける */
	bHide = IsMapHideArea ();
	if (bHide) {
		for (i = 0; i < SCRMARTSSIZE; i ++) {
			abyHide[i] = IsMapHideShowTile (i) ? 1 : 0;
		}
	}

	float afR2[MAPLIGHT_MAX], afDY2[MAPLIGHT_MAX], afTx[SCRSIZEX];
	int anRowLight[MAPLIGHT_MAX], anX0[SCRSIZEX], anX1[SCRSIZEX];

	/* ゆらぎを反映済みの半径を、そのまま二乗して使う */
	for (i = 0; i < nLight; i ++) {
		afR2[i] = afR[i] * afR[i];
	}

	/* 画面に見えている範囲だけ処理する */
	nSX = 32 - CamOfsX ();
	nSY = 32 - CamOfsY ();
	nCX = SCRSIZEX;
	nCY = SCRSIZEY;
	if (nSX < 0) { nCX += nSX; nSX = 0; }
	if (nSY < 0) { nCY += nSY; nSY = 0; }
	nCX = min (nCX, nWidth - nSX);
	nCY = min (nCY, nHeight - nSY);

	/* 横の補間係数は、どの行でも同じ */
	for (x = nSX; x < nSX + nCX; x ++) {
		float fTx = (x - 16) / 32.0f;
		int nTx = (int)floorf (fTx);
		fTx -= nTx;
		afTx[x - nSX] = fTx;
		anX0[x - nSX] = max (0, min (nTx, nMapW - 1));
		anX1[x - nSX] = max (0, min (nTx + 1, nMapW - 1));
	}

	for (y = nSY; y < nSY + nCY; y ++) {
		PDWORD pDst;
		float fTy, fVisY0, fVisY1;
		int nTy, nMY0, nMY1, nRowLight, j;

		/* この行に届く灯りだけを元の順番で並べる。半径 0 は全体を照らす */
		nRowLight = 0;
		for (i = 0; i < nLight; i ++) {
			float fDY = (float)(y - anY[i]);
			afDY2[i] = fDY * fDY;
			if ((afR[i] <= 0.0f) || (afDY2[i] < afR2[i])) {
				anRowLight[nRowLight ++] = i;
			}
		}

		/* 見えるマスかどうかをマスの中心どうしでなめらかにつなぐ(縦) */
		fTy = (y - 16) / 32.0f;
		nTy = (int)floorf (fTy);
		fTy -= nTy;
		nMY0 = nMapW * max (0, min (nTy, DRAW_PARTS_Y * 2 + 2));
		nMY1 = nMapW * max (0, min (nTy + 1, DRAW_PARTS_Y * 2 + 2));
		for (x = 0; x < nMapW; x ++) {
			int y0 = max (0, min (nTy, DRAW_PARTS_Y * 2 + 2));
			int y1 = max (0, min (nTy + 1, DRAW_PARTS_Y * 2 + 2));
			fVisY0 = (pMapDark[x + nMapW * y0] > 1) ? 1.0f : 0.0f;
			fVisY1 = (pMapDark[x + nMapW * y1] > 1) ? 1.0f : 0.0f;
			afVisX[x] = fVisY0 + (fVisY1 - fVisY0) * fTy;
		}

		pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + nSX;
		for (x = nSX; x < nSX + nCX; x ++, pDst ++) {
			float fTx, fVis, fLight, fWarm;
			int x0, x1, nLevel, nWarm;

			/* 見えるマスかどうか(横) */
			fTx = afTx[x - nSX];
			x0 = anX0[x - nSX];
			x1 = anX1[x - nSX];
			fVis = afVisX[x0] + (afVisX[x1] - afVisX[x0]) * fTx;
			/* 見えるマスの縁だけ少しぼかす(真ん中は暗くしない) */
			fVis *= 1.6f;
			if (fVis <= 0.0f) {
				*pDst &= 0xFF000000;
				continue;
			}

			/* 灯りからの距離でなめらかに暗くする(灯りが重なると明るくなる) */
			fLight = 0.0f;
			fWarm = 0.0f;
			/* 「明るく壁で隠す」範囲の中のマスは全部明るい */
			if (bHide && abyHide[(x / 32) + nMapW * (y / 32)]) {
				fLight = min (fVis, 1.0f);
				j = nRowLight;
			} else {
				j = 0;
			}
			for (; j < nRowLight; j ++) {
				float fDX, fD, fVisLight;
				i = anRowLight[j];
				if (afR[i] <= 0.0f) {
					fLight = min (fVis, 1.0f);
					fWarm = 0.0f;
					break;
				}
				fDX = (float)(x - anX[i]);
				fD = fDX * fDX + afDY2[i];
				if (fD >= afR2[i]) {
					continue;		/* 届かない灯りは平方根を取らない */
				}
				/* この灯りが照らせたマスだけ明るくする(マスの境目はなめらかに) */
				if (apMask[i]) {
					const BYTE *pMask = apMask[i];
					float fA = pMask[x0 + nMY0] + (pMask[x1 + nMY0] - pMask[x0 + nMY0]) * fTx;
					float fB = pMask[x0 + nMY1] + (pMask[x1 + nMY1] - pMask[x0 + nMY1]) * fTx;
					fVisLight = (fA + (fB - fA) * fTy) * 1.6f;
					if (fVisLight <= 0.0f) {
						continue;
					}
					fVisLight = min (fVisLight, 1.0f);
				} else {
					fVisLight = min (fVis, 1.0f);
				}
				fD = sqrtf (fD);
				if (fD < afR[i]) {
					float fAdd = (1.0f - fD / afR[i]) * afPower[i] * fVisLight;
					fLight += fAdd;
					fWarm += fAdd * afWarm[i];
				}
			}
			if (fLight > 1.0f) {
				fLight = 1.0f;
			}

			nLevel = (int)(fLight * 256.0f);
			/* 暖色の光源の光が届いている所は橙色に寄せる */
			nWarm = (int)(min (fWarm, 1.0f) * 256.0f);
			if ((nLevel >= 256) && (nWarm <= 0)) {
				continue;
			}
			nLevel = min (nLevel, 256);
			{
				DWORD dw = *pDst;
				DWORD r = ((dw >> 16) & 0xFF) * nLevel >> 8;
				DWORD g = ((dw >> 8) & 0xFF) * nLevel >> 8;
				DWORD b = (dw & 0xFF) * nLevel >> 8;
				if (nWarm > 0) {
					r = min ((DWORD)255, r * (256 + nWarm * 12 / 100) >> 8);
					g = g * (256 - nWarm * 14 / 100) >> 8;
					b = b * (256 - nWarm * 42 / 100) >> 8;
				}
				*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	DrawCharLightGlow												 */
/* 内容：	明かり(特技・ランタン・たいまつ等)を使っている人の体を			 */
/*			柔らかい暖色でぼんやり光らせる									 */
/*			明かりでマップ全体が明るくなるマップでも、誰が使っているか		 */
/*			分かるようにする。DrawMapLight の暗幕の後に、体のまわりの		 */
/*			小さな円だけスクリーン合成(明るい所でも暗い所でも見える)する	 */
/*			ゆっくり呼吸のように明滅する									 */
/* ========================================================================= */
#define CHARGLOW_RADIUS		22			/* 光の半径(ドット) */
#define CHARGLOW_BASE		0.34f		/* 光の強さの中心(0〜1) */
#define CHARGLOW_SWING		0.10f		/* 明滅の振れ幅 */

static void DrawCharLightGlow(void)
{
	int i, x, y, nCount, nWidth, nHeight, nMapW;
	float fTime, fBreath;
	PCHARINFO pMyChar, pChar;
	LPBYTE pBits;

	if ((g_nMyCharID < 0) || (g_pDibDrawMap == NULL)) {
		return;
	}
	pBits = g_pDibDrawMap->GetBits ();
	if (pBits == NULL) {
		return;
	}
	nWidth	= g_pDibDrawMap->Width ();
	nHeight	= g_pDibDrawMap->Height ();
	nMapW	= DRAW_PARTS_X * 2 + 3;
	pMyChar	= g_aCharInfo[g_nMyCharID];

	/* 呼吸のようなゆっくりした明滅(周期 約 2.6 秒) */
	fTime = (float)(timeGetTime () % 600000);
	fBreath = CHARGLOW_BASE + CHARGLOW_SWING * sinf (fTime * 0.0024f);

	nCount = g_aCharInfo.GetSize ();
	for (i = 0; i < nCount; i ++) {
		int xTmp, yTmp, nCX, nCY, nX0, nX1, nY0, nY1;

		pChar = g_aCharInfo[i];
		if (pChar->byLightLevel == 0) {
			continue;
		}
		xTmp = CamTileX () - (int)pChar->dwX;
		yTmp = CamTileY () - (int)pChar->dwY;
		if ((abs (xTmp) > DRAW_PARTS_X + 1) || (abs (yTmp) > DRAW_PARTS_Y + 1)) {
			continue;
		}
		/* キャラ描画(DrawCharLine)と同じ見えない条件 */
		if (pChar != pMyChar) {
			if (GetMapLightLevel ()) {
				if (g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) + nMapW * (DRAW_PARTS_Y + 1 - yTmp)] <= 1) {
					continue;
				}
			}
			if (IsMapViewMode () && IsMapDarkHidePos (xTmp, yTmp)) {
				continue;
			}
			if (IsLightOtherRoom ((int)pChar->dwX, (int)pChar->dwY)) {
				continue;
			}
		}
		/* 体の中心(キャラ描画と同じ座標。絵は 32 ドット角で 8 ドット上にずらして描く) */
		nCX = 256 + 16 - xTmp * 32 - BowOffsetX (pChar);
		nCY = 256 - 8 + 16 - yTmp * 32 - BowOffsetY (pChar);

		nX0 = max (0, nCX - CHARGLOW_RADIUS);
		nX1 = min (nWidth - 1, nCX + CHARGLOW_RADIUS);
		nY0 = max (0, nCY - CHARGLOW_RADIUS);
		nY1 = min (nHeight - 1, nCY + CHARGLOW_RADIUS);
		for (y = nY0; y <= nY1; y ++) {
			PDWORD pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + nX0;
			int nDY = y - nCY;
			for (x = nX0; x <= nX1; x ++, pDst ++) {
				int nDX = x - nCX;
				int nD2 = nDX * nDX + nDY * nDY;
				float fRate;
				int nA;
				DWORD dw, r, g, b;

				if (nD2 >= CHARGLOW_RADIUS * CHARGLOW_RADIUS) {
					continue;
				}
				/* 真ん中が一番強く、端へなめらかに消える */
				fRate = 1.0f - sqrtf ((float)nD2) / CHARGLOW_RADIUS;
				fRate *= fRate;
				nA = (int)(fRate * fBreath * 256.0f);
				if (nA <= 0) {
					continue;
				}
				/* スクリーン合成: 暖色(255,205,120)を c + (255-c) * 色 * 強さ で重ねる */
				dw = *pDst;
				r = (dw >> 16) & 0xFF;
				g = (dw >> 8) & 0xFF;
				b = dw & 0xFF;
				r += ((255 - r) * 255 >> 8) * nA >> 8;
				g += ((255 - g) * 205 >> 8) * nA >> 8;
				b += ((255 - b) * 120 >> 8) * nA >> 8;
				*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：	DrawMapLine														 */
/* 内容：	指定ラインの背景描画											 */
/* 日付：	2004/07/03														 */
/* ========================================================================= */

void DrawMapLine(int nLine, int nType)
{
	PMAPPARTSINFO pParts;
	PCHARINFO pMyChar;
	BYTE byLevel, byLight;
	int i, j, x, y, mx, my, xMax, nTmp;

	if (!g_pMapInfo->pwMap || !g_pMapPartsInfo) {
		return;
	}
	if (g_nMyCharID < 0) {
		return;
	}

	y = nLine;
	pMyChar = g_aCharInfo[g_nMyCharID];
	byLight = pMyChar->byLightLevel;
	nTmp = GetMapLightLevel () - byLight;
	nTmp = max (nTmp, 0);
	nTmp = min (nTmp, 100);
	byLight = (BYTE)nTmp;

	/* 見える範囲(視界)の外に描くパーツ(0 なら黒) */
	WORD wViewHide = 0;
	if (IsMapViewMode () && (g_pMapInfo->wViewHideParts < g_wCountMapParts)) {
		wViewHide = g_pMapInfo->wViewHideParts;
	}

	mx = CamTileX ();
	my = CamTileY ();
	j = my - (DRAW_PARTS_Y + 1) + y;
	xMax = DRAW_PARTS_X * 2 + 3;
	for (x = 0; x < xMax; x ++) {
		WORD wMap;
		BOOL bViewHide;
		i = mx - (DRAW_PARTS_X + 1) + x;

		byLevel = 100;
		if (GetMapLightLevel () || IsMapViewMode ()) {
			byLevel = g_pMapDarkTmp[x + (DRAW_PARTS_X * 2 + 3) * nLine];
		}
		/* 見えない所はそのパーツを明るいまま描く(本当の地形は描かない)。
		   暗くはせず、あとで DrawMapViewFog が白い霧をかける */
		bViewHide = (wViewHide && (byLevel == 0));
		if (wViewHide) {
			byLevel = 100;
		}

		/* 範囲外？ */
		if (!bViewHide && ((i < 0) || (i >= g_pMapInfo->nCX) || (j < 0) || (j >= g_pMapInfo->nCY))) {
			if (nType < 0) {
				g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_pDibMapParts, 0, 0, byLevel, FALSE);
			}
			continue;
		}

		if (bViewHide) {
			wMap = wViewHide;
		} else {
			wMap = g_pMapInfo->pwMap[i + j * g_pMapInfo->nCX];
		}
		if (wMap >= g_wCountMapParts) {
			wMap = 0;
		}

		PANIMEINFO pAnime = NULL;
		DWORD dwNo, dwBackNo;
		BOOL bDrawBack = TRUE, bDrawMain = TRUE;
		pParts = &g_pMapPartsInfo[wMap];

		if (nType >= 0) {
			switch (nType) {
			case PARTSHIT_HIDE1:
				if (!((pParts->byHitType == PARTSHIT_HIDE1) ||
					  (pParts->byHitType == PARTSHIT_HIDE2))) {
					continue;
				}
				bDrawBack = FALSE;
				break;

			case PARTSHIT_HIDE3:
				if (!((pParts->byHitType == PARTSHIT_HIDE3) ||
					  (pParts->byHitType == PARTSHIT_HIDE4) ||
					  (pParts->byHitType == PARTSHIT_HIDE7))) {
					continue;
				}
				if (pParts->byHitType != PARTSHIT_HIDE7) {
					bDrawBack = FALSE;
				}
				break;

			case PARTSHIT_HIDE5:
				if (!((pParts->byHitType == PARTSHIT_HIDE5) ||
					  (pParts->byHitType == PARTSHIT_HIDE6))) {
					continue;
				}
				break;

			case PARTSHIT_BRIDGEV:		/* 橋(縦・横とも)を重ねる */
				if (GetBridgeAxis (pParts->byHitType) == 0) {
					continue;
				}
				if (s_pbyBridgeCol && (s_pbyBridgeCol[x] == 0)) {
					continue;
				}
				break;
			}

		} else {
			switch (pParts->byHitType) {
			case PARTSHIT_HIDE1:
			case PARTSHIT_HIDE2:
			case PARTSHIT_HIDE3:
			case PARTSHIT_HIDE4:
			case PARTSHIT_HIDE7:
				bDrawMain = FALSE;
				break;
			}
		}

		/* アニメーションするパーツ？ */
		if (pParts->byAnimeCount) {
			pAnime = &pParts->pAnimeInfo[pParts->byAnimeNo];
			dwNo = pAnime->wGrpID;
			dwBackNo = pAnime->wBackID;

		} else {
			if (nType < 0 && bDrawMain) {
				g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_pDibMapParts, (wMap % 16) * 32, wMap / 16 * 32, byLevel, FALSE);
				continue;
			}

			dwNo = pParts->wGrpID;
			dwBackNo = pParts->wBackID;
		}

		int no, xx, yy;
		DWORD dwID;
		BYTE byLevelParts = 0;

		/* 重ね合わせが必要？ */
		if (bDrawBack) {
			if (dwBackNo == 0) {
				dwBackNo = dwNo;
			}
			no = dwBackNo / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = dwBackNo % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_apDibMap[no], xx, yy, byLevel, FALSE);
		}
		if (bDrawMain == FALSE) {
			continue;
		}

		no = dwNo / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = dwNo % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = (dwID % g_byMapGrpCountX) * 32;
		yy = dwID / g_byMapGrpCountX * 32;
		byLevelParts = 0;

		if (pAnime) {
			if (pAnime->byLevel) {
				byLevelParts = pAnime->byLevel;
			}
		} else {
			if (pParts->byLevel) {
				byLevelParts = pParts->byLevel;
			}
		}
		if (byLevelParts) {
			g_pDibDrawMap->BltAlpha (x * 32, y * 32, 32, 32, g_apDibMap[no], xx, yy, byLevelParts, TRUE);
			if (byLevel < 100) {
				g_pDibDrawMap->ChgLevel (x * 32, y * 32, 32, 32, byLevel);
			}

		} else {
			g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_apDibMap[no], xx, yy, byLevel, TRUE);
		}
	}
}


/* ========================================================================= */
/* 関数名：	IsHideNpcPut												 */
/* 内容：	NPC発生（管理者にだけ見える目印）を描かないか				 */
/* 			一般プレイヤーには見せない。管理者もスクショ用に				 */
/* 			「その他→各種通知」の「NPC発生を表示」を外すと隠れる			 */
/* ========================================================================= */

static BOOL IsHideNpcPut(PCHARINFO pChar)
{
	if (pChar->byNpcType != NPCTYPE_PUT) {
		return FALSE;
	}
	if (g_aCharInfo[g_nMyCharID]->byAdminLV == ADMINLV_USER) {
		return TRUE;
	}
	return (g_bViewNpcPut == FALSE);
}


/* ========================================================================= */
/* 関数名：	DrawCharLine													 */
/* 内容：	キャラの描画													 */
/* 日付：	2003/05/11														 */
/* ========================================================================= */

void DrawCharLine(int nLine)
{
	RECT rc;
	BYTE byMapGrpCountX, byMapGrpCountY;
	DWORD dwMapID, dwMyX, dwMyY, wGrpID, wGrpMain, wGrpSub;
	PCHARINFO pChar, pCharMyChar;
	PMAPINFO pMapInfo;
	int i, no, x, y, xx, yy, xTmp, yTmp, nCountLine, nCount, nGrpSizeX, nGrpSizeY;

	if (g_nMyCharID < 0) {
		return;
	}

	pCharMyChar	= g_aCharInfo[g_nMyCharID];
	dwMapID		= pCharMyChar->dwMapID;
	dwMyX		= CamTileX ();
	dwMyY		= CamTileY ();
	pMapInfo	= g_pMapInfo;
	byMapGrpCountX = g_byMapGrpCountX;
	byMapGrpCountY = g_byMapGrpCountY;
	nCount = g_aCharInfo.GetSize ();

	nCountLine = g_aCharPos[nLine].GetSize ();
	if (nCountLine <= 0) {
		return;
	}

	/* 絵を表示 */
	for (i = 0; i < nCountLine; i ++) {
		char nMapDark;
		pChar = g_aCharInfo[g_aCharPos[nLine][i]];

		/* 橋の上を渡っている人だけ描き直す時？ */
		if (s_bDrawCharBridgeUpper && (GetCharBridgeLayer (pChar) != 1)) {
			continue;
		}

		/* NPC発生は管理者だけ見える（管理者も「各種通知」で隠せる） */
		if (IsHideNpcPut (pChar)) {
			continue;
		}

		nMapDark = 0;
		xTmp = dwMyX - pChar->dwX;
		yTmp = dwMyY - pChar->dwY;

		/* 暗闇表示？ */
		if (GetMapLightLevel ()) {
			nMapDark = g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) +
						(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp)];
			if (nMapDark <= 1) {
				continue;
			}
		}
		/* 見える範囲(視界)の外？ */
		if (IsMapViewMode () && IsMapDarkHidePos (xTmp, yTmp)) {
			continue;
		}
		
		xx = pChar->byDirection * 64;
		yy = 32 * (pChar->wGrpID % 16);
		xx += (pChar->byAnime * 32);
		SetRect (&rc, xx, yy, xx + 32, yy + 32);

		x = 256;
		y = 256;
		x -= (xTmp * 32);
		y -= (yTmp * 32);
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));

		y -= 8;
		nGrpSizeX = 32;
		nGrpSizeY = 32;
		if (y < 0) {
			if (y <= -32) {
				continue;
			}
			nGrpSizeY += y;
			y = 0;
			yy += 8;
		}
		CImg32 *pDib;

		wGrpID		= pChar->wGrpID;
		wGrpMain	= pChar->wGrpMain;
		wGrpSub		= pChar->wGrpSub;
		if (!((pChar->wGrpTmpID == 0) && (pChar->wGrpTmpMain == 0) && (pChar->wGrpTmpSub == 0))) {
			wGrpID		= pChar->wGrpTmpID;
			wGrpMain	= pChar->wGrpTmpMain;
			wGrpSub		= pChar->wGrpTmpSub;
		}

		/* 通常グラフィック？ */
		if (wGrpMain == CHARGRP_MAIN_NORMAL) {
			no = wGrpID / 16;
			/* 正体を現した宝箱の敵？ 上向きの絵はただの宝箱なので、上を向いた時は下向きの絵で描く。
			   絵ID 194〜196（char13.png の3〜5行目）で、種別が戦闘１・戦闘２の時だけ。
			   正体前（宝箱の敵）や、依頼の宝箱などの同じ絵のNPCは、上向きの宝箱のまま描く */
			if ((pChar->byDirection == 0) &&
				((pChar->byNpcType == NPCTYPE_BATTLE1) || (pChar->byNpcType == NPCTYPE_BATTLE2)) &&
				(wGrpID >= MIMIC_GRPID_FIRST) && (wGrpID <= MIMIC_GRPID_LAST)) {
				xx = 1 * 64 + (pChar->byAnime * 32);
			}
			if (pChar->byCharState == CHARSTATE_DEAD) {
				xx = pChar->byDirection * 64;
				yy = 32 * 1;
				no = 0;
			}
			if (no >= g_wCountCharGrp) {
				continue;
			}
			pDib = g_apDibChar[no];
			/* サイコロ？ */
			if ((pChar->byNpcType == NPCTYPE_BLOCK2) ||
				(pChar->byNpcType == NPCTYPE_BLOCK3) ||
				(pChar->byNpcType == NPCTYPE_BLOCK4)) {
				xx = pChar->byEfect * 32;

			/* 得点板か話すとランダムか止まるかランダムか押せて話すと切り替え？ */
			} else if ((pChar->byNpcType == NPCTYPE_SCORE) ||
					   (pChar->byNpcType == NPCTYPE_TALKRAND) ||
					   (pChar->byNpcType == NPCTYPE_CHGPUSH)) {
				xx = pChar->byEfect * 32;

			/* 止まるかランダム？ */
			} else if (pChar->byNpcType == NPCTYPE_STOPRAND) {
				xx = (pChar->byEfect & 0x7F) * 32;

			/* 爆弾？ */
			} else if (pChar->byNpcType == NPCTYPE_BOMB) {
				pDib = g_pDibSubgBomb;
				switch (pChar->byEfect) {
				case 0:	/* 爆発前 */
					xx = pChar->byAnime * 32;
					yy = 32;
					break;
				default:	/* 爆発中 */
					xx = 0;
					if (pChar->byEfect > 4) {
						yy = 64 + (6 - (pChar->byEfect - 1)) * 32;
					} else {
						yy = 64 + (pChar->byEfect - 1) * 32;
					}
					BOOL bUp = TRUE, bDown = TRUE, bLeft = TRUE, bRight = TRUE;
					/* 火力分ループ */
					for (int j = 1; j < pChar->wHP; j ++) {
						int xTmp2, yTmp2;

						if (j != pChar->wHP - 1) {
							xTmp = pChar->dwX;
							yTmp = pChar->dwY;
							xTmp2 = xTmp;
							yTmp2 = yTmp;

							if (bUp) {
								yTmp2 = yTmp - j;
								yTmp2 = max (0, yTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bUp = FALSE;
										}
									}
								}

								if (bUp) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y - (j * 32) >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x, y - (j * 32), 32, yTmp, pDib, xx + 192, yy, TRUE);
									} else {
										bUp = FALSE;
									}
								}
							}
							if (bDown) {
								yTmp2 = yTmp + j;
								yTmp2 = min (g_pMapInfo->nCY - 1, yTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bDown = FALSE;
										}
									}
								}

								if (bDown) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y + (j * 32) >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x, y + (j * 32), 32, yTmp, pDib, xx + 192, yy, TRUE);
									} else {
										bDown = FALSE;
									}
								}
							}
							yTmp2 = yTmp;
							if (bLeft) {
								xTmp2 = xTmp - j;
								xTmp2 = max (0, xTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bLeft = FALSE;
										}
									}
								}

								if (bLeft) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x - (j * 32), y, 32, yTmp, pDib, xx + 160, yy, TRUE);
									} else {
										bLeft = FALSE;
									}
								}
							}
							if (bRight) {
								xTmp2 = xTmp + j;
								xTmp2 = min (g_pMapInfo->nCX - 1, xTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bRight = FALSE;
										}
									}
								}

								if (bRight) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x + (j * 32), y, 32, yTmp, pDib, xx + 160, yy, TRUE);
									} else {
										bRight = FALSE;
									}
								}
							}

						/* 先端？ */
						} else if (j == pChar->wHP - 1) {
							xTmp = pChar->dwX;
							yTmp = pChar->dwY;
							xTmp2 = xTmp;
							yTmp2 = yTmp;

							if (bUp) {
								yTmp2 = yTmp - j;
								yTmp2 = max (0, yTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bUp = FALSE;
										}
									}
								}

								if (bUp) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y - (j * 32) >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x, y - (j * 32), 32, yTmp, pDib, xx + 32, yy, TRUE);
									} else {
										bUp = FALSE;
									}
								}
							}
							if (bDown) {
								yTmp2 = yTmp + j;
								yTmp2 = min (g_pMapInfo->nCY - 1, yTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bDown = FALSE;
										}
									}
								}

								if (bDown) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y + (j * 32) >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x, y + (j * 32), 32, yTmp, pDib, xx + 64, yy, TRUE);
									} else {
										bDown = FALSE;
									}
								}
							}
							yTmp2 = yTmp;
							if (bLeft) {
								xTmp2 = xTmp - j;
								xTmp2 = max (0, xTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bLeft = FALSE;
										}
									}
								}

								if (bLeft) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x - (j * 32), y, 32, yTmp, pDib, xx + 96, yy, TRUE);
									} else {
										bLeft = FALSE;
									}
								}
							}
							if (bRight) {
								xTmp2 = xTmp + j;
								xTmp2 = min (g_pMapInfo->nCX - 1, xTmp2);

								for (int k = 0; k < nCount; k ++) {
									PCHARINFO pCharTmp = g_aCharInfo[k];
									if (pCharTmp->byNpcType == NPCTYPE_BOMBBLOCK) {
										if ((pCharTmp->dwX == (DWORD)xTmp2) && (pCharTmp->dwY == (DWORD)yTmp2)) {
											bRight = FALSE;
										}
									}
								}

								if (bRight) {
									if (IsBlock (xTmp2, yTmp2, TRUE) == FALSE) {
										int yTmp = nGrpSizeY;
										if (y >= 0) {
											yTmp = 32;
										}
										g_pDibDrawMap->Blt (x + (j * 32), y, 32, yTmp, pDib, xx + 128, yy, TRUE);
									} else {
										bRight = FALSE;
									}
								}
							}
						}
					}
					break;
				}

			/* 爆弾ゲームの壁？ */
			} else if (pChar->byNpcType == NPCTYPE_BOMBBLOCK) {
				pDib = g_pDibSubgBomb;
				xx = (pChar->byEfect) * 32;
				yy = 0;
				y += 8;

			/* 爆弾ゲームのアイテム？ */
			} else if (pChar->byNpcType == NPCTYPE_BOMBITEM) {
				pDib = g_pDibSubgBomb;
				xx = 128 + (pChar->byEfect) * 32;
				yy = 32;
				y += 8;
			}

		/* 横16縦32サイズ？ */
		} else if (wGrpMain == CHARGRP_MAIN_1632) {
			no = wGrpID / 8;
			if (no >= g_wCountCharGrp1632) {
				continue;
			}
			pDib = g_apDibChar1632[no];
			yy = 64 * (wGrpID % 16);
			y -= 32;
			nGrpSizeY = 64;
			/* 押せて話すと切り替え？ */
			if (pChar->byNpcType == NPCTYPE_CHGPUSH) {
				xx = pChar->byEfect * 32;
			}

		/* 戦場ゲームグラフィック？ */
		} else if (wGrpMain == CHARGRP_MAIN_SUBWAR) {
			PSUBSTATUS_WAR pSubStatus;

			pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;
			pDib = g_pDibWChar;
			xx = wGrpSub * 64;
			xx += (pChar->byAnime * 32);
			yy = wGrpID * 32;

		/* 横32縦32サイズ？ */
		} else if (wGrpMain == CHARGRP_MAIN_3232) {
			no = wGrpID / 8;
			if (no >= g_wCountCharGrp3232) {
				continue;
			}
			pDib = g_apDibChar3232[no];
			xx = pChar->byDirection * 128;
			xx += (pChar->byAnime * 64);
			yy = 64 * (wGrpID % 16);
			y -= 32;
			nGrpSizeX = 64;
			nGrpSizeY = 64;

		/* 横48縦48サイズ？ */
		} else if (wGrpMain == CHARGRP_MAIN_4848) {
			no = wGrpID / 8;
			if (no >= g_wCountCharGrp4848) {
				continue;
			}
			pDib = g_apDibChar4848[no];
			xx = pChar->byDirection * 192;
			xx += (pChar->byAnime * 96);
			yy = 96 * (wGrpID % 16);
			y -= 64;
			nGrpSizeX = 96;
			nGrpSizeY = 96;
		}

		/* 体を描かない（名前だけ見える目印。拠点防衛の「回復の泉」など） */
		if (pChar->byGhost == CHARGHOST_NAMEONLY) {
			continue;
		}

		/* NPC発生？ */
		if (pChar->byNpcType == NPCTYPE_PUT) {
			g_pDibDrawMap->BltAlpha (x, y, nGrpSizeX, nGrpSizeY, pDib, xx, yy, 50, TRUE);

		} else {
			int nTmp = 0;
			/* 暗闇表示？ */
			if (GetMapLightLevel ()) {
				nTmp = 100 - (nMapDark + pChar->byLightLevel);
				nTmp = max (nTmp, 0);
				nTmp = min (nTmp, 100);
			}
			if (nTmp < 100) {
				BYTE byLevel = 100 - nTmp;

				if (pChar->byFadeLevel) {
					/* フェード中でも、霊・隠れている間は半透明より濃くならない（出てきた直後に一瞬ふつうに見えない） */
					BYTE byAlpha = pChar->byFadeLevel;
					if (pChar->byHide || pChar->byGhost) {
						byAlpha = max (byAlpha, (BYTE)(100 - byLevel / 2));
					}
					g_pDibDrawMap->BltAlpha (x, y, nGrpSizeX, nGrpSizeY, pDib, xx, yy, byAlpha, TRUE);

				} else if (pChar->byHide || pChar->byGhost) {
					/* 隠れている間・霊(byGhost)は半透明(BltAlpha は大きいほど透明。明るい所は半分、暗い所ほど薄く) */
					g_pDibDrawMap->BltAlpha (x, y, nGrpSizeX, nGrpSizeY, pDib, xx, yy, (BYTE)(100 - byLevel / 2), TRUE);

				} else {
					g_pDibDrawMap->BltLevel (x, y, nGrpSizeX, nGrpSizeY, pDib, xx, yy, byLevel, TRUE);
				}
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawChar2														 */
/* 内容：キャラのエフェクトの描画											 */
/* 日付：2003/05/11															 */
/* ========================================================================= */
void DrawChar2(void)
{
	RECT rc, rcMyChar;
	PCHARINFO pChar;
	int i, x, y, xx, yy, xTmp, yTmp,
		nGrpSizeY, xBack, yBack, nCount = g_aCharInfo.GetSize ();

	if (g_nMyCharID < 0) {
		return;
	}

	DWORD dwMyX = CamTileX (),
		  dwMyY = CamTileY ();

	xBack = yBack = -1;
	/* 吹き出しエフェクト表示 */
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[i];
		if (pChar->byComEfc == 0) {
			continue;
		}

		PCOMEFCINFO pComEfc = &g_pComEfcInfo[pChar->byComEfc - 1];
		PCOMEFCANIMEINFO pAnime = &pComEfc->pAnime[pChar->byComEfcAnime];

		if (pAnime == NULL) {
			continue;
		}

		/* NPC発生は管理者だけ見える（管理者も「各種通知」で隠せる） */
		if (IsHideNpcPut (pChar)) {
			continue;
		}
		xTmp = dwMyX - pChar->dwX;
		yTmp = dwMyY - pChar->dwY;

		/* 暗闇表示？ */
		if (IsMapDarkHidePos (xTmp, yTmp)) {
			continue;
		}

		xx = 32 * (pAnime->byGrpID % 8);
		yy = 32 * (pAnime->byGrpID / 8);
		SetRect (&rc, xx, yy, xx + 32, yy + 32);

		x = 256;
		y = 256 - 32;
		x -= (xTmp * 32);		/* 自分も視点（盤の中央に固定中は自分が中央とは限らない）からの位置で描く */
		y -= (yTmp * 32);
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));

		y -= 8;
		/* 自分のすぐ下の敵の予兆の「!」は、自分に重ならないように敵の体の上に出す */
		if ((pChar->byComEfc == COMEFC_TELL) && (xTmp == 0) && (yTmp == -1) &&
			((pChar->byNpcType == NPCTYPE_BATTLE1) || (pChar->byNpcType == NPCTYPE_BATTLE2))) {
			y += 32;
		}
		nGrpSizeY = 32;
		if (y < 0) {
			nGrpSizeY += y;
			y = 0;
			yy += 8;
		}
		if (i == g_nMyCharID) {
			rcMyChar = rc;
			xBack = x;
			yBack = y;
		} else {
			g_pDibDrawMap->Blt (x, y, 32, nGrpSizeY, g_pDibComEfc, xx, yy, TRUE);
		}
	}
	if (g_aCharInfo[g_nMyCharID]->byComEfc) {
		if (xBack >= 0) {
			g_pDibDrawMap->Blt (xBack, yBack, 32, 32, g_pDibComEfc, rcMyChar.left, rcMyChar.top, TRUE);
		}
	}

	/* エフェクト表示
	   (置いた魔法(爆弾など)のエフェクトを先に描き、当たった相手の爆発などをその上に描く。
	    同じマスに置くと、後から出た魔法の絵で相手のエフェクトが隠れて見えなかった) */
	for (int nPass = 0; nPass < 2; nPass ++) {
	for (i = 0; i < nCount; i ++) {
		BYTE byEfect;
		BOOL bPutMagic;
		pChar = g_aCharInfo[i];
		if (pChar->dwTimeEfc == 0) {
			continue;
		}
		bPutMagic = ((pChar->byNpcType == NPCTYPE_USEMAGIC) ||
					 (pChar->byNpcType == NPCTYPE_USEMAGICPARENT) ||
					 (pChar->byNpcType == NPCTYPE_ATACK));
		if (bPutMagic != (nPass == 0)) {
			continue;
		}

		byEfect = pChar->byEfect;
		if (byEfect == 0) {
			byEfect = pChar->byEfectIdle;
		}
		if (byEfect == 0) {
			continue;
		}

		PEFCINFO pEfcInfo = g_EfcInfo.GetPtr (byEfect - 1);
		PEFCANIMEINFO pAnime = g_EfcInfo.GetPtrAnime (byEfect - 1, pChar->byEfcAnime);
		if (pAnime == NULL) {
			continue;
		}

		/* NPC発生は管理者だけ見える（管理者も「各種通知」で隠せる） */
		if (IsHideNpcPut (pChar)) {
			continue;
		}
		xTmp = dwMyX - pChar->dwX;
		yTmp = dwMyY - pChar->dwY;

		/* 暗闇表示？ */
		if (IsMapDarkHidePos (xTmp, yTmp)) {
			continue;
		}

		xx = (pAnime->wGrpID % 8) * 32;
		if (pEfcInfo->byType == 1) {
			yy = (pAnime->wGrpID / 8) % 8 * 64;
		} else {
			yy = (pAnime->wGrpID / 8) % 16 * 32;
		}
		SetRect (&rc, xx, yy, xx + 32, yy + 32);

		x = 256;
		y = 256;
		x -= (xTmp * 32);		/* 自分も視点（盤の中央に固定中は自分が中央とは限らない）からの位置で描く */
		y -= (yTmp * 32);
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));
		x += (GetCharCenterX (pChar) - 16);

		y -= 8;
		if (pEfcInfo->byType == 1) {
			nGrpSizeY = 64;
		} else {
			nGrpSizeY = 32;
		}
		if (y < 0) {
			nGrpSizeY += y;
			y = 0;
			yy += 8;
		}
		if (i == g_nMyCharID) {
			rcMyChar = rc;
			xBack = x;
			yBack = y;
		} else {
			if (pEfcInfo->byType == 1) {
				if (pAnime->byLevel) {
					g_pDibDrawMap->BltAlpha (x, y - 32, 32, nGrpSizeY, g_apDibEfect1632[pAnime->wGrpID / 128], xx, yy, pAnime->byLevel, TRUE);
				} else {
					g_pDibDrawMap->Blt (x, y - 32, 32, nGrpSizeY, g_apDibEfect1632[pAnime->wGrpID / 128], xx, yy, TRUE);
				}
			} else {
				if (pAnime->byLevel) {
					g_pDibDrawMap->BltAlpha (x, y, 32, nGrpSizeY, g_apDibEfect[pAnime->wGrpID / 128], xx, yy, pAnime->byLevel, TRUE);
				} else {
					g_pDibDrawMap->Blt (x, y, 32, nGrpSizeY, g_apDibEfect[pAnime->wGrpID / 128], xx, yy, TRUE);
				}
			}
		}
	}
	}
	pChar = g_aCharInfo[g_nMyCharID];
	if (pChar->dwTimeEfc) {
		BYTE byEfect;
		byEfect = pChar->byEfect;
		if (byEfect == 0) {
			byEfect = pChar->byEfectIdle;
		}
		if (byEfect > 0) {
			PEFCINFO pEfcInfo = g_EfcInfo.GetPtr (byEfect - 1);
			PEFCANIMEINFO pAnime = g_EfcInfo.GetPtrAnime (byEfect - 1, pChar->byEfcAnime);
			if (pEfcInfo->byType == 1) {
				if (pAnime->byLevel) {
					g_pDibDrawMap->BltAlpha (xBack, yBack - 32, 32, 64, g_apDibEfect1632[pAnime->wGrpID / 64], rcMyChar.left, rcMyChar.top, pAnime->byLevel, TRUE);
				} else {
					g_pDibDrawMap->Blt (xBack, yBack - 32, 32, 64, g_apDibEfect1632[pAnime->wGrpID / 64], rcMyChar.left, rcMyChar.top, TRUE);
				}
			} else {
				if (pAnime->byLevel) {
					g_pDibDrawMap->BltAlpha (xBack, yBack, 32, 32, g_apDibEfect[pAnime->wGrpID / 128], rcMyChar.left, rcMyChar.top, pAnime->byLevel, TRUE);
				} else {
					g_pDibDrawMap->Blt (xBack, yBack, 32, 32, g_apDibEfect[pAnime->wGrpID / 128], rcMyChar.left, rcMyChar.top, TRUE);
				}
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawChar3														 */
/* 内容：キャラの名前等の描画												 */
/* 日付：2003/05/11															 */
/* ========================================================================= */
void DrawChar3(void)
{
	HDC hDC, hdcTmp;
	PCHARINFO pChar;
	int i, x, y, xTmp, yTmp, nameX, nameY,
		nCount = g_aCharInfo.GetSize ();
	HFONT hFontOld, hFontOld2;

	if (g_nMyCharID < 0) {
		return;
	}

	DWORD dwMyX = CamTileX (),
		  dwMyY = CamTileY ();

	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	hdcTmp = pDibDrawTmp.Lock ();
	hFontOld2 = (HFONT)SelectObject (hdcTmp, g_hFont12);
	SetBkMode (hdcTmp, TRANSPARENT);

	/* チャットや名前を表示 */
	for (i = 0; i < nCount; i ++) {
		pChar = g_aCharInfo[i];
		xTmp = dwMyX - pChar->dwX;
		yTmp = dwMyY - pChar->dwY;

		/* 暗闇表示？ */
		if (IsMapDarkHidePos (xTmp, yTmp)) {
			continue;
		}

		/* NPC発生は管理者だけ見える（管理者も「各種通知」で隠せる） */
		if (IsHideNpcPut (pChar)) {
			continue;
		}

		pDibDrawTmp.FillRect (0, 0, 8 * 20, 14 * 2, RGB (0, 0, 0));

		x = 256;
		y = 256;
		x -= (xTmp * 32);		/* 自分も視点（盤の中央に固定中は自分が中央とは限らない）からの位置で描く */
		y -= (yTmp * 32);
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));

		/* ターゲット選択情報 */
		if (g_pTargetSelectInfo) {
			/* 選択しているキャラ？ */
			if (g_pTargetSelectInfo->dwTblID == pChar->dwTblID) {
				g_pDibDrawMap->Blt (x, y, 32, 32, g_pDibSystem, g_pTargetSelectInfo->byAnime * 32, 4 * 16, TRUE);
			}
		}

		COLORREF cl = RGB(255, 255, 255);
		nameX = x;
		nameY = y;

		if (pChar->pszSpeech) {
			if (pChar->clSpeech) {
				cl = pChar->clSpeech;
			}

			/* nLen は表示幅（半角 1 文字 = 1） */
			int xx, yy, nLen = SboTextWidth (pChar->pszSpeech);
			char szTmp[128];
			if (nLen > 20) {
				int j, nWidth, nLenTmp;
				/* 表示幅 20 に届くまでを 1 行目にする（最後の全角ははみ出してよい） */
				for (j = 0, nWidth = 0; pChar->pszSpeech[j] && (nWidth < 20); ) {
					nWidth += SboTextCharWidth (&pChar->pszSpeech[j]);
					j += SboTextCharLen (&pChar->pszSpeech[j]);
				}
				ZeroMemory (szTmp, sizeof (szTmp));
				CopyMemory (szTmp, pChar->pszSpeech, min (j, (int)sizeof (szTmp) - 1));
				xx = x - nWidth * 6 / 2 + 16;
				TextOut2 (hdcTmp, 1, 1, szTmp, cl);
				nLenTmp = SboTextWidth (szTmp);

				/* 2 行目も表示幅 20 まで。URL を含む長い発言（ブラウザ版のチャット欄で
				   1 つのリンクにするため分けずに送る）は、入らない分を「..」で省く */
				{
					LPCSTR pszLine2 = &pChar->pszSpeech[j];
					int nCopy = SboTextFitWidth (pszLine2, 20);
					BOOL bCut = (pszLine2[nCopy] != 0) ? TRUE : FALSE;
					if (bCut) {
						nCopy = SboTextFitWidth (pszLine2, 18);
					}
					nCopy = min (nCopy, (int)sizeof (szTmp) - 3);
					ZeroMemory (szTmp, sizeof (szTmp));
					CopyMemory (szTmp, pszLine2, nCopy);
					if (bCut) {
						strcat (szTmp, "..");
					}
				}
				nLenTmp = max (nLenTmp, SboTextWidth (szTmp));
				TextOut2 (hdcTmp, 1, 12, szTmp, cl);

				x = xx;
				y = y - 12 - 8 - 12;
				xx = max (x, 0);
				yy = max (y, 0);
				if (x < 0) {
					x *= x;
				} else {
					x = 0;
				}
				if (y < 0) {
					y *= y;
				} else {
					y = 0;
				}
				BYTE byLevel = 0;
				if (g_bSpeechView) {
					DWORD dwTmp = (timeGetTime () - pChar->dwLastSpeech);
					if (dwTmp >= 5000) {
						byLevel = (BYTE)((dwTmp - 5000) / 50);
					}
				} 
				g_pDibDrawMap->BltAlpha (xx - 2, yy,
						nLenTmp * 6 - x + 1, 28 - y, &pDibDrawTmp, x, y,
						byLevel, TRUE);
				
			} else {
				/* 発言内容を表示 */
				TextOut2 (hdcTmp, 1, 1, pChar->pszSpeech, cl);
				x = x - nLen * 6 / 2 + 16;
				y = y - 12 - 8;
				xx = max (x, 0);
				yy = max (y, 0);
				if (x < 0) {
					x *= x;
				} else {
					x = 0;
				}
				if (y < 0) {
					y *= y;
				} else {
					y = 0;
				}
				BYTE byLevel = 0;
				if (g_bSpeechView) {
					DWORD dwTmp = (timeGetTime () - pChar->dwLastSpeech);
					if (dwTmp >= 5000) {
						byLevel = (BYTE)((dwTmp - 5000) / 50);
					}
				} 
				g_pDibDrawMap->BltAlpha (xx - 2, yy,
						nLen * 6 - x + 1, 14 - y, &pDibDrawTmp, x, y,
						byLevel, TRUE);
			}
		}

		/* 文字エフェクトを表示 */
		/* 自分の頭の上にゲージが出ている間は、ゲージに重ならないようにその上から出す */
		int nEfcOfsY = 0;
		if ((i == g_nMyCharID) && (pChar->dwTimeTurnLV || pChar->dwTimeTurnMagicLV)) {
			nEfcOfsY = -14;
		}
		for (int j = 0; j < MAX_COUNTCHAREFC; j ++) {
			PCHAREFCINFO pCharEfc = &pChar->aCharEfcInfo[j];
			int nLen = SboTextWidth (pCharEfc->szMsg);
			if (nLen == 0) {
				continue;
			}
			if (pCharEfc->color & CHAREFC_BIG) {
				/* 大きい文字(ジャスト・構えなど、見落とさないように) */
				HFONT hFontEfc = (HFONT)SelectObject (hDC, g_hFontBasic);
				TextOut2 (hDC, nameX - nLen * 8 / 2 + GetCharCenterX (pChar),
						nameY - 12 + nEfcOfsY + pCharEfc->chY,
						pCharEfc->szMsg, pCharEfc->color & 0x00FFFFFF);
				SelectObject (hDC, hFontEfc);
			} else {
				TextOut2 (hDC, nameX - nLen * 6 / 2 + GetCharCenterX (pChar),
						nameY - 8 + nEfcOfsY + pCharEfc->chY,
						pCharEfc->szMsg, pCharEfc->color);
			}
		}

		/* 落ち物パズルの台NPC（台詞が @p。サーバーが byGhost=CHARGHOST_NAMEONLY で送ってくる）は体も名前も描かない（見えない台） */
		BOOL bPuzzleTable = (pChar->byGhost == CHARGHOST_NAMEONLY) && (pChar->byNpcType != NPCTYPE_INIT) && pChar->pszNpcSpeech &&
				(pChar->pszNpcSpeech[0] == '@') && ((pChar->pszNpcSpeech[1] == 'p') || (pChar->pszNpcSpeech[1] == 'i') || (pChar->pszNpcSpeech[1] == 'j'));
		if (!bPuzzleTable && ((g_byViewMode == 1) || (g_byViewMode == 3))) {
			cl = RGB(255, 255, 255);
			if (pChar->clName) {
				cl = pChar->clName;
			}

			/* キャラ名を表示 */
			TextOut2 (hDC, nameX - SboTextWidth (pChar->szName) * 6 / 2 + GetCharCenterX (pChar), nameY + 32 - 8,
					g_aCharInfo[i]->szName, cl);
		}
	}
	SelectObject (hdcTmp, hFontOld2);
	pDibDrawTmp.Unlock ();

	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

/* ========================================================================= */
/* アイテムドロップ演出														 */
/* 敵がアイテムを落とした時、敵の位置からアイテムの絵が跳ね上がり、キラッと	 */
/* 光り、もらった人の頭上で浮かんでから消える。ブラウザ版は毎フレーム描画	 */
/* しているので、時刻(timeGetTime)だけで位置を決める						 */
/* ========================================================================= */
#define DROPFX_MAX		8		/* 同時に出せる数 */
#define DROPFX_HOP		350		/* 跳ね上がる時間(ms) */
#define DROPFX_MOVE		250		/* もらった人の頭上へ移る時間(ms) */
#define DROPFX_FLOAT	1000	/* 浮かんでいる時間(ms) */
#define DROPFX_FADE		400		/* 消えるまでの時間(ms。浮かんでいる時間の終わりの部分) */
#define DROPFX_SPARK	300		/* 光る時間(ms。跳ね終わりから) */

typedef struct _DROPFXINFO {
	BOOL	bUse;				/* 使用中 */
	DWORD	dwStart;			/* 開始時間 */
	DWORD	dwMapID;			/* 敵のいたマップ */
	int		nWorldX, nWorldY;	/* 敵のいた位置(ドット。歩き中のずれも含む) */
	DWORD	dwItemNo;			/* アイテム番号 */
	DWORD	dwOwnerID;			/* もらった人のキャラID(0は不明) */
	DWORD	dwMoveStart;		/* 頭上へ移り始める時間(0は移らない) */
} DROPFXINFO;

static DROPFXINFO g_aDropFx[DROPFX_MAX];

/* 追加(いっぱいなら一番古いものを使う) */
void AddDropEffect(DWORD dwMapID, int nWorldX, int nWorldY, DWORD dwItemNo)
{
	int i, nUse = 0;

	for (i = 0; i < DROPFX_MAX; i ++) {
		if (!g_aDropFx[i].bUse) {
			nUse = i;
			break;
		}
		if ((int)(g_aDropFx[nUse].dwStart - g_aDropFx[i].dwStart) > 0) {
			nUse = i;
		}
	}
	DROPFXINFO *p = &g_aDropFx[nUse];
	ZeroMemory (p, sizeof (DROPFXINFO));
	p->bUse = TRUE;
	p->dwStart = timeGetTime ();
	p->dwMapID = dwMapID;
	p->nWorldX = nWorldX;
	p->nWorldY = nWorldY;
	p->dwItemNo = dwItemNo;
	g_bDraw = TRUE;
}

/* もらった人を設定(同じアイテムでまだ持ち主のいない一番新しい演出に付ける) */
void SetDropEffectOwner(DWORD dwItemNo, DWORD dwCharID)
{
	int i, nFind = -1;
	DWORD dwNow = timeGetTime ();

	for (i = 0; i < DROPFX_MAX; i ++) {
		DROPFXINFO *p = &g_aDropFx[i];
		if (!p->bUse || p->dwOwnerID || (p->dwItemNo != dwItemNo)) {
			continue;
		}
		if ((nFind < 0) || ((int)(p->dwStart - g_aDropFx[nFind].dwStart) > 0)) {
			nFind = i;
		}
	}
	if (nFind < 0) {
		return;
	}
	g_aDropFx[nFind].dwOwnerID = dwCharID;
	/* 跳ね終わる前なら跳ね終わりから、過ぎていればすぐ移る */
	g_aDropFx[nFind].dwMoveStart = max (dwNow, g_aDropFx[nFind].dwStart + DROPFX_HOP);
}

/* アイテムの種別に合わせた system.png の 16x16 の絵の位置 */
static void GetDropIconPos(PITEMINFO pItem, int *pnX, int *pnY)
{
	int x = 128, y = 0;	/* 袋 */

	switch (pItem->byType) {
	case ITEMTYPE_WEAPON:		x = 0;		y = 96;	break;
	case ITEMTYPE_SEED:			x = 16;		y = 96;	break;
	case ITEMTYPE_ARMOR:		x = 32;		y = 96;	break;
	case ITEMTYPE_HELMET:		x = 48;		y = 96;	break;
	case ITEMTYPE_EQU_HAND:		x = 64;		y = 96;	break;
	case ITEMTYPE_EQU_LEG:		x = 80;		y = 96;	break;
	case ITEMTYPE_EQU_ACC1:		x = 96;		y = 96;	break;
	case ITEMTYPE_EQU_ACC2:
	case ITEMTYPE_EQU_ACC3:		x = 112;	y = 96;	break;
	case ITEMTYPE_SHILD:		x = 144;	y = 64;	break;
	case ITEMTYPE_MAGIC:
	case ITEMTYPE_MSG:			x = 160;	y = 64;	break;
	case ITEMTYPE_HPUP:
	case ITEMTYPE_MANAUP:
	case ITEMTYPE_HPMANAUP:
	case ITEMTYPE_SAKE:			x = 208;	y = 64;	break;
	}
	*pnX = x;
	*pnY = y;
}

static void DrawDropEffects(void)
{
	int i;
	DWORD dwNow = timeGetTime ();

	if (g_nMyCharID < 0) {
		return;
	}
	PCHARINFO pMy = g_aCharInfo[g_nMyCharID];

	for (i = 0; i < DROPFX_MAX; i ++) {
		DROPFXINFO *p = &g_aDropFx[i];
		if (!p->bUse) {
			continue;
		}
		DWORD dwTime = dwNow - p->dwStart;
		/* 終わる時間(頭上へ移る場合は移り終わってから1秒) */
		DWORD dwEnd = DROPFX_HOP + DROPFX_FLOAT;
		if (p->dwMoveStart) {
			dwEnd = (p->dwMoveStart - p->dwStart) + DROPFX_MOVE + DROPFX_FLOAT;
		}
		if ((dwTime >= dwEnd) || (dwTime > 10000)) {
			p->bUse = FALSE;
			continue;
		}
		if ((p->dwMapID != pMy->dwMapID) || (p->dwItemNo > 0xFFFF) || !ItemIDValid ((WORD)p->dwItemNo)) {
			continue;
		}
		g_bDraw = TRUE;

		/* 敵がいた場所の画面位置(絵の真ん中と足元の高さ) */
		int x = 256 + p->nWorldX - CamTileX () * 32 + 16;
		int y = 256 + p->nWorldY - CamTileY () * 32 - 8;
		float fHopY = 0.0f;
		int nSparkX, nSparkY;
		BYTE byLevel = 0;

		if (dwTime < DROPFX_HOP) {
			/* 放物線で跳ね上がる(横へも少しずつずれる) */
			float u = (float)dwTime / DROPFX_HOP;
			fHopY = 4.0f * u * (1.0f - u) * 28.0f;
			x += (int)(u * 10.0f);
			y -= 8;
		} else {
			x += 10;
			y -= 8;
		}
		y -= (int)fHopY;

		/* もらった人が分かる時は、その人の頭上へ移る */
		if (p->dwOwnerID) {
			int nNo = GetCharNo (p->dwOwnerID);
			if (nNo >= 0) {
				PCHARINFO pOwner = g_aCharInfo[nNo];
				int tx = 256 + ((int)pOwner->dwX - CamTileX ()) * 32 - BowOffsetX (pOwner) + GetCharCenterX (pOwner);
				int ty = 256 + ((int)pOwner->dwY - CamTileY ()) * 32 - BowOffsetY (pOwner) - 8 - 20;
				DWORD dwMoveTime = dwNow - p->dwMoveStart;
				if ((int)dwMoveTime < 0) {
					dwMoveTime = 0;
				}
				float r = (dwMoveTime >= DROPFX_MOVE) ? 1.0f : ((float)dwMoveTime / DROPFX_MOVE);
				r = r * (2.0f - r);	/* 最初が速く、最後がゆっくり */
				int nFromX = 256 + p->nWorldX - CamTileX () * 32 + 26;
				int nFromY = 256 + p->nWorldY - CamTileY () * 32 - 16;
				x = nFromX + (int)((tx - nFromX) * r);
				y = nFromY + (int)((ty - nFromY) * r);
			}
		}
		nSparkX = x;
		nSparkY = y;

		/* 浮かんでいる間は、ゆっくり上へ動く */
		DWORD dwFloatStart = DROPFX_HOP;
		if (p->dwMoveStart) {
			dwFloatStart = (p->dwMoveStart - p->dwStart) + DROPFX_MOVE;
		}
		if (dwTime >= dwFloatStart) {
			DWORD dwF = dwTime - dwFloatStart;
			y -= (int)(dwF / 100);
			nSparkY = y;
			/* 終わりのほうで薄くなって消える(BltAlpha は大きいほど透明) */
			if (dwF + DROPFX_FADE > DROPFX_FLOAT) {
				byLevel = (BYTE)(min (100, (int)((dwF + DROPFX_FADE - DROPFX_FLOAT) * 100 / DROPFX_FADE)));
			}
		}

		/* アイテムの絵 */
		int nGx, nGy;
		GetDropIconPos (g_aItemInfo[p->dwItemNo], &nGx, &nGy);
		if (byLevel) {
			g_pDibDrawMap->BltAlpha (x - 8, y - 8, 16, 16, g_pDibSystem, nGx, nGy, byLevel, TRUE);
		} else {
			g_pDibDrawMap->Blt (x - 8, y - 8, 16, 16, g_pDibSystem, nGx, nGy, TRUE);
		}

		/* 跳ね終わりにキラッと光る(白い十字が大きくなって小さくなる) */
		if ((dwTime >= DROPFX_HOP - 60) && (dwTime < DROPFX_HOP - 60 + DROPFX_SPARK)) {
			float u = (float)(dwTime - (DROPFX_HOP - 60)) / DROPFX_SPARK;
			int nLen = (int)(10.0f * (1.0f - fabsf (2.0f * u - 1.0f))) + 1;
			FillRectClip (nSparkX - nLen, nSparkY - 1, nLen * 2 + 1, 2, RGB (255, 255, 255));
			FillRectClip (nSparkX - 1, nSparkY - nLen, 2, nLen * 2 + 1, RGB (255, 255, 255));
			FillRectClip (nSparkX - 4, nSparkY - 4, 8, 8, RGB (255, 255, 200));
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawETC															 */
/* 内容：ゲージ等の描画														 */
/* 日付：2004/01/02															 */
/* ========================================================================= */
void DrawETC(void)
{
	HDC			hDC;
	HFONT		hFontOld;
	COLORREF	cl;
	PCHARINFO	pChar;
	int			x = SCRSIZEX - 96 + 30,
				y = SCRSIZEY - 18,
				nNameX,
				i, j;

	if (g_nMyCharID < 0) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	x -= CamOfsX ();
	y -= CamOfsY ();

	/* 表示していい？ */
	if (g_byViewMode >= 2) {
		int nMax;
		nMax = g_aPartyInfo.GetSize ();
		if (nMax <= 0) {
			nMax = 1;
		}
		for (i = 0; i < nMax; i ++) {
			if (nMax > 1) {
				int nNo = GetCharNo2 (g_aPartyInfo[i]);
				if (nNo < 0) {
					continue;
				}
				pChar = g_aCharInfo[nNo];
				if (pChar->wMaxHP == 0) {
					continue;
				}
			}

			nNameX = x + 3;
			if (SboTextWidth (pChar->szName) >= 14) {
				nNameX -= ((SboTextWidth (pChar->szName) - 14) * 6);
			}

			/* フレーム表示 */
			g_pDibSystem->SetColorKey (RGB (40, 40, 40));
			g_pDibDrawMap->BltAlpha2 (x, y, 16, 48, g_pDibSystem, 0, 128, 25, TRUE);
			for (j = 1; j <= 4; j ++) {
				g_pDibDrawMap->BltAlpha2 (x + j * 16, y, 16, 48, g_pDibSystem, 16, 128, 25, TRUE);
			}
			g_pDibDrawMap->BltAlpha2 (x + j * 16, y, 16, 48, g_pDibSystem, 32, 128, 25, TRUE);
			g_pDibSystem->SetColorKey (RGB (0, 0, 0));

			/* HP表示 */
			g_pDibDrawMap->Blt (x + 8, y + 13, 80, 16, g_pDibSystem, 0, 208, TRUE);
			if (pChar->wHP) {
				char chLV = (char)(pChar->wHP * 10000 / pChar->wMaxHP / 100);
				chLV = max (chLV, 1);
				int nTmp = chLV * 80 / 100;
				nTmp = max (nTmp, 1);
				g_pDibDrawMap->Blt (x + 8, y + 13, nTmp, 16, g_pDibSystem, 0, 176, TRUE);
			}
			/* Mana表示 */
			g_pDibDrawMap->Blt (x + 8, y + 29, 80, 16, g_pDibSystem, 0, 208, TRUE);
			if (pChar->wMana) {
				char chLV = (char)(pChar->wMana * 10000 / pChar->wMaxMana / 100);
				chLV = max (chLV, 1);
				int nTmp = chLV * 80 / 100;
				nTmp = max (nTmp, 1);
				g_pDibDrawMap->Blt (x + 8, y + 29, nTmp, 16, g_pDibSystem, 0, 192, TRUE);
			}

			cl = RGB(255, 255, 255);
			char szTmp[10];
			if (pChar->clName) {
				cl = pChar->clName;
			}
			hDC = g_pDibDrawMap->Lock ();
			hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
			SetBkMode (hDC, TRANSPARENT);
			/* 名前表示 */
			TextOut2 (hDC, nNameX, y + 1, pChar->szName, cl);

			/* HP表示 */
			wsprintf (szTmp, "%d", pChar->wHP);
			TextOut2 (hDC, x + 80 - (strlen (szTmp) * 6), y + 15, szTmp, RGB (255, 255, 255));

			/* Mana表示 */
			wsprintf (szTmp, "%5d", pChar->wMana);
			TextOut2 (hDC, x + 80 - (strlen (szTmp) * 6), y + 31, szTmp, RGB (255, 255, 255));

			SelectObject (hDC, hFontOld);
			g_pDibDrawMap->Unlock ();

			/* 酔い状態アイコン表示 */
			if (pChar->byDrunkLV > CHARSTATEDRUNK_NONE) {
				g_pDibDrawMap->Blt (x + 3, y + 29, 16, 16, g_pDibSystem, pChar->byDrunkLV * 16, 112, TRUE);
			}
			/* 攻撃力・守備力アップのアイコン(HP 欄の左。仲間の分も) */
			DrawBuffIcons (x + 3, y + 13, pChar->dwTblID);
			y -= 48;
		}
	}

	pChar = g_aCharInfo[g_nMyCharID];
	/* 魔法ターンゲージ表示 */
	if (pChar->dwTimeTurnMagicLV) {
		x = 256 - (CamTileX () - (int)pChar->dwX) * 32;
		y = 256 - (CamTileY () - (int)pChar->dwY) * 32;
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));
		x -= 3;
		y -= 16;

		/* フレーム表示 */
		g_pDibDrawMap->Blt (x, y, 38, 9, g_pDibSystem, 96, 128, TRUE);
		/* 現在値表示 */
		int nTmp = pChar->byTurnMagicLV * 32 / 100;
		nTmp = max (nTmp, 1);
		g_pDibDrawMap->Blt (x + 3, y + 3, 32 - nTmp, 3, g_pDibSystem, 99, 146, TRUE);

	/* ターンゲージ表示 */
	} else if (pChar->dwTimeTurnLV) {
		x = 256 - (CamTileX () - (int)pChar->dwX) * 32;
		y = 256 - (CamTileY () - (int)pChar->dwY) * 32;
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));
		x -= 3;
		y -= 16;

		/* フレーム表示 */
		g_pDibDrawMap->Blt (x, y, 38, 9, g_pDibSystem, 96, 128, TRUE);

		/* 現在値表示：ゲージは5周で満タン。1周(20%)ごとに色が変わり、
		   1周目が溜まると早撃ちできる。威力は溜まった周の数(1周=20%)で決まる */
		static const COLORREF aclLap[5] = {
			RGB (64, 160, 255),		/* 1周目：青 */
			RGB (64, 224, 64),		/* 2周目：緑 */
			RGB (255, 230, 0),		/* 3周目：黄 */
			RGB (255, 140, 0),		/* 4周目：橙 */
			RGB (255, 40, 40)		/* 5周目：赤 */
		};
		int nLap = pChar->byTurnLV / TURNGAGE_LAP;			/* 溜まりきった周の数(0～5) */
		if (nLap >= TURNGAGE_LAPMAX) {
			if (IsTurnGageJust (pChar)) {
				/* ジャストで押せる時間：白く光らせる */
				g_pDibDrawMap->FillRect (x + 3, y + 3, 32, 3, RGB (255, 255, 255));
			} else {
				/* 満タン：白と赤で点滅 */
				g_pDibDrawMap->FillRect (x + 3, y + 3, 32, 3, ((timeGetTime () / 120) % 2) ? RGB (255, 255, 255) : aclLap[4]);
			}
		} else {
			int nTmp = (pChar->byTurnLV % TURNGAGE_LAP) * 32 / TURNGAGE_LAP;
			nTmp = max (nTmp, 1);
			/* 下地に溜まりきった周の色 */
			if (nLap > 0) {
				g_pDibDrawMap->FillRect (x + 3, y + 3, 32, 3, aclLap[nLap - 1]);
			}
			/* 溜まり中の周 */
			g_pDibDrawMap->FillRect (x + 3, y + 3, nTmp, 3, aclLap[nLap]);
		}
		/* ジャストで押せる間は、ゲージの枠を黄色く光らせる */
		if (IsTurnGageJust (pChar)) {
			DrawFrameRect (x - 1, y - 1, 40, 14, RGB (255, 255, 64));
		}
		/* 5周目に入ったら、ゲージの右端にジャストの目印(ゴルフのインパクトのような小さな印) */
		if (nLap >= TURNGAGE_LAPMAX - 1) {
			g_pDibDrawMap->FillRect (x + 35, y + 1, 2, 7, IsTurnGageJust (pChar) ? RGB (255, 255, 64) : RGB (255, 255, 255));
		}
		/* ゲージの下に、全部で何周あって何周溜まったかを5つの目盛りで出す */
		g_pDibDrawMap->FillRect (x + 2, y + 8, 34, 4, RGB (0, 0, 0));
		for (int i = 0; i < TURNGAGE_LAPMAX; i ++) {
			g_pDibDrawMap->FillRect (x + 3 + i * 7, y + 9, 5, 2, (i < nLap) ? aclLap[i] : RGB (80, 80, 80));
		}
		g_bDraw = TRUE;
	}

	/* 構えている間：ゲージの横に盾マーク(吹き出しが消えた後も構えているのが分かるように)。
	   ゲージが出ている時は、止まっているのが分かるようにゲージを青い枠で囲む */
	if (g_bGuard) {
		x = 256 - (CamTileX () - (int)pChar->dwX) * 32;
		y = 256 - (CamTileY () - (int)pChar->dwY) * 32;
		x -= (BowOffsetX(pChar));
		y -= (BowOffsetY(pChar));
		x -= 3;
		y -= 16;
		if (pChar->dwTimeTurnMagicLV || pChar->dwTimeTurnLV) {
			DrawFrameRect (x - 1, y - 1, 40, 14, RGB (90, 180, 255));
		}
		DrawGuardMark (x + 38, y - 4);
		g_bDraw = TRUE;
	}
}


/* ========================================================================= */
/* 関数名：DrawFrameRect													 */
/* 内容：1ドットの枠を描く													 */
/* ========================================================================= */
static void DrawFrameRect(int x, int y, int cx, int cy, COLORREF cl)
{
	g_pDibDrawMap->FillRect (x, y, cx, 1, cl);
	g_pDibDrawMap->FillRect (x, y + cy - 1, cx, 1, cl);
	g_pDibDrawMap->FillRect (x, y, 1, cy, cl);
	g_pDibDrawMap->FillRect (x + cx - 1, y, 1, cy, cl);
}


/* ========================================================================= */
/* 関数名：DrawGuardMark													 */
/* 内容：構えの盾マーク(絵は system.png の x=144,y=96 の 16×16)			 */
/*		 守りアップと同じ絵。盾は 16×16 の左上(2,1)から幅12×高さ14			 */
/* ========================================================================= */
#define GUARDMARK_GRPX	144
#define GUARDMARK_GRPY	96
static void DrawGuardMark(int x, int y)
{
	g_pDibDrawMap->Blt (x, y, 16, 16, g_pDibSystem, GUARDMARK_GRPX, GUARDMARK_GRPY, TRUE);
}


/* ========================================================================= */
/* 関数名：DrawBuffIcons													 */
/* 内容：攻撃力アップ(剣)・守備力アップ(盾)のアイコン(16×16 を横に並べる)	 */
/*		 絵は system.png の y=96 の行。x=160 攻撃アップ、x=144 守りアップ(構えと同じ盾) */
/*		 切れる15秒前から点滅する											 */
/* ========================================================================= */
#define BUFFICON_GRPX_AT	160		/* system.png の攻撃アップの絵の左 */
#define BUFFICON_GRPX_DF	144		/* 守りアップの絵の左(構えの盾と同じ) */
#define BUFFICON_GRPY	96
static void DrawBuffIcons(int x, int y, DWORD dwTblID)
{
	int i;
	DWORD dwRest;

	for (i = 0; i < 2; i ++) {
		dwRest = GetCharBuffRest (dwTblID, i);
		if (dwRest == 0) {
			continue;
		}
		/* 切れそうな時は点滅(点滅の間は描き直し続ける) */
		if (dwRest < 15000) {
			g_bDraw = TRUE;
			if ((timeGetTime () / 250) % 2) {
				x += 16;
				continue;
			}
		}
		g_pDibDrawMap->Blt (x, y, 16, 16, g_pDibSystem, (i == 0) ? BUFFICON_GRPX_AT : BUFFICON_GRPX_DF, BUFFICON_GRPY, TRUE);
		x += 16;
	}
}


/* ========================================================================= */
/* 関数名：DrawObj															 */
/* 内容：オブジェクトの描画													 */
/* 日付：2004/01/16															 */
/* ========================================================================= */
void DrawObj(void)
{
	int nCount = g_aObjInfo.GetSize ();
	if (nCount == 0) {
		return;
	}
	PCHARINFO pChar = g_aCharInfo[g_nMyCharID];

	int i, x, y, xx, yy, xTmp, yTmp, sizeX, sizeY,
		nMyX = CamTileX (), nMyY = CamTileY ();
	for (i = 0; i < nCount; i ++) {
		POBJINFO pObj = g_aObjInfo[i];

		/* 表示座標を求める */
		xTmp = nMyX - pObj->byX;
		yTmp = nMyY - pObj->byY;
		x = 256 - (xTmp * 32);
		y = 256 - (yTmp * 32);

		CImg32 *pDib;
		xx = yy = 0;
		sizeX = sizeY = 32;

		/* 種別に応じて表示 */
		switch (pObj->byTypeMain) {
		case OBJTYPE_FOOTMARK_SNOW:		/* 足跡(雪) */
			pDib = g_apDibObj3232[0];
			xx = pObj->wGrpID * 32;
			break;

		case OBJTYPE_FOOTMARK_SAND:		/* 足跡(砂) */
			pDib = g_apDibObj3232[0];
			xx = pObj->wGrpID * 32;
			break;

		case OBJTYPE_WATERRIPPLE:		/* 水の波紋 */
			pDib = g_apDibObj3232[0];
			xx = pObj->byAnimeNo * 32;
			yy = 64;
			break;
		}
		if (pObj->byLevel) {
			g_pDibDrawMap->BltAlpha (x, y, sizeX, sizeY, pDib, xx, yy, pObj->byLevel, TRUE);
		} else {
			g_pDibDrawMap->Blt (x, y, sizeX, sizeY, pDib, xx, yy, TRUE);
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawEvent														 */
/* 内容：イベント情報の描画													 */
/* 日付：2004/07/19															 */
/* ========================================================================= */
void DrawEvent(void)
{
	int x, y, xTmp, yTmp;
	PCHARINFO pChar;

	if (g_nMyCharID < 0) {
		return;
	}

	if (g_hWndAdmin == NULL) {
		return;
	}

	if (g_nAdminWindowType != IDM_MAP_EVENT) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	xTmp = CamTileX ();
	yTmp = CamTileY ();

	for (y = yTmp - DRAW_PARTS_Y; y <= yTmp + DRAW_PARTS_Y; y ++) {
		if (y < 0 || y >= g_pMapInfo->nCY) {
			continue;
		}
		for (x = xTmp - DRAW_PARTS_X; x <= xTmp + DRAW_PARTS_X; x ++) {
			if (x < 0 || x >= g_pMapInfo->nCX) {
				continue;
			}
			if (g_pMapInfo->pbyFlg[x + g_pMapInfo->nCX * y] & MAPFLG_EVENT) {
				g_pDibDrawMap->Blt (((x - xTmp) + DRAW_PARTS_X + 1) * 32,
						((y - yTmp) + DRAW_PARTS_Y + 1) * 32, 32, 32, g_pDibSystem, 0, 224, TRUE);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawMapEdit														 */
/* 内容：マップ編集情報の描画												 */
/* 日付：2005/01/12															 */
/* ========================================================================= */
void DrawMapEdit(void)
{
	int x, y, xx, yy, xTmp, yTmp;
	PCHARINFO pChar;
	PMAPPARTSINFO pParts;
	WORD wMap;

	if (g_nMyCharID < 0) {
		return;
	}

	if (g_hWndAdmin == NULL) {
		return;
	}

	if (g_nAdminWindowType != IDM_MAP_PARTS) {
		return;
	}

	if (g_bMapMarkView == FALSE) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	xTmp = CamTileX ();
	yTmp = CamTileY ();

	for (y = yTmp - DRAW_PARTS_Y; y <= yTmp + DRAW_PARTS_Y; y ++) {
		if (y < 0 || y >= g_pMapInfo->nCY) {
			continue;
		}
		for (x = xTmp - DRAW_PARTS_X; x <= xTmp + DRAW_PARTS_X; x ++) {
			if (x < 0 || x >= g_pMapInfo->nCX) {
				continue;
			}
			wMap = g_pMapInfo->pwMap[x + y * g_pMapInfo->nCX];
			if (wMap >= g_wCountMapParts) {
				wMap = 0;
			}
			xx = yy = 0;
			pParts = &g_pMapPartsInfo[wMap];
			switch (pParts->byHitType) {
			case PARTSHIT_BLOCK1:			/* ぶつかる(音付き) */
			case PARTSHIT_COUNTER:			/* カウンター */
				xx = 32;
				yy = 224;
				break;
			case PARTSHIT_BLOCK2:			/* ぶつかる(音無し) */
				xx = 64;
				yy = 224;
				break;
			case PARTSHIT_HIDE1:			/* 隠れる */
				xx = 32 * 3;
				yy = 224;
				break;
			case PARTSHIT_HIDE2:			/* 隠れる(ぶつかる) */
				xx = 32 * 4;
				yy = 224;
				break;
			case PARTSHIT_HIDE3:			/* 隠れる3 */
				xx = 32 * 5;
				yy = 224;
				break;
			case PARTSHIT_HIDE4:			/* 隠れる4(ぶつかる) */
				xx = 32 * 6;
				yy = 224;
				break;
			case PARTSHIT_HIDE5:			/* 隠れる5(背景ごと隠れる) */
				xx = 32 * 4;
				yy = 192;
				break;
			case PARTSHIT_HIDE6:			/* 隠れる6(ぶつかる/音無し) */
				xx = 32 * 5;
				yy = 192;
				break;
			case PARTSHIT_HIDE7:			/* 隠れる7(隠れる3で背景ごと隠れる) */
				xx = 32 * 6;
				yy = 192;
				break;
			}

			if (xx || yy) {
				g_pDibDrawMap->Blt (((x - xTmp) + DRAW_PARTS_X + 1) * 32,
						((y - yTmp) + DRAW_PARTS_Y + 1) * 32, 32, 32, g_pDibSystem, xx, yy, TRUE);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：FillRectClip														 */
/* 内容：画像の外にはみ出さない矩形塗りつぶし								 */
/*       （FillRect は左上が負の時に位置がずれるので、先に切り取る）		 */
/* ========================================================================= */
static void FillRectClip(int x, int y, int cx, int cy, COLORREF cl)
{
	int x2 = x + cx, y2 = y + cy;

	x = max (x, 0);
	y = max (y, 0);
	x2 = min (x2, g_pDibDrawMap->Width ());
	y2 = min (y2, g_pDibDrawMap->Height ());
	if (x2 <= x || y2 <= y) {
		return;
	}
	g_pDibDrawMap->FillRect (x, y, x2 - x, y2 - y, cl);
}

/* ========================================================================= */
/* 関数名：DrawLineClip														 */
/* 内容：線を描く（nWidth ドットの正方形を並べる。画像の外は描かない）		 */
/* ========================================================================= */
static void DrawLineClip(int x1, int y1, int x2, int y2, int nWidth, COLORREF cl)
{
	int dx, dy, sx, sy, nErr, nErr2;

	dx = abs (x2 - x1);
	dy = abs (y2 - y1);
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	nErr = dx - dy;
	for (;;) {
		FillRectClip (x1 - nWidth / 2, y1 - nWidth / 2, nWidth, nWidth, cl);
		if (x1 == x2 && y1 == y2) {
			break;
		}
		nErr2 = nErr * 2;
		if (nErr2 > -dy) {
			nErr -= dy;
			x1 += sx;
		}
		if (nErr2 < dx) {
			nErr += dx;
			y1 += sy;
		}
	}
}

/* ========================================================================= */
/* 関数名：DrawNpcRoute														 */
/* 内容：NPCの巡回情報の描画												 */
/*       巡回できるNPCに枠、選んでいるNPCは目立つ色の枠、					 */
/*       編集中の道しるべに番号（1は色を変える）と点どうしをつなぐ線		 */
/*       （一周なら最後→最初も）											 */
/* ========================================================================= */
void DrawNpcRoute(void)
{
	int i, x, y, xTmp, yTmp, nCount, nW, nH, anX[MAX_NPCROUTE], anY[MAX_NPCROUTE];
	DWORD dwSel;
	HDC hDC;
	HFONT hFontOld;
	PCHARINFO pChar;
	char szTmp[16];
	COLORREF clLine = RGB (240, 184, 74);

	if (g_nMyCharID < 0) {
		return;
	}

	if (g_hWndAdmin == NULL) {
		return;
	}

	if (g_nAdminWindowType != IDM_CHAR_NPCROUTE) {
		return;
	}

	/* 自分のマップが変わっていないか確かめる */
	NpcRouteTick ();

	pChar = g_aCharInfo[g_nMyCharID];
	xTmp = CamTileX ();
	yTmp = CamTileY ();
	dwSel = NpcRouteGetSelTblID ();
	nW = (DRAW_PARTS_X * 2 + 1) * 32;
	nH = (DRAW_PARTS_Y * 2 + 1) * 32;

	/* 巡回できるNPCの枠（選んでいるのは目立つ色で太く） */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		pChar = g_aCharInfo[i];
		if (!NpcRouteIsListed (pChar->dwTblID)) {
			continue;
		}
		x = (((int)pChar->dwX - xTmp) + DRAW_PARTS_X + 1) * 32;
		y = (((int)pChar->dwY - yTmp) + DRAW_PARTS_Y + 1) * 32;
		if (x < 32 || y < 32 || x >= 32 + nW || y >= 32 + nH) {
			continue;
		}
		if (pChar->dwTblID == dwSel) {
			DrawFrameRect (x, y, 32, 32, RGB (255, 200, 0));
			DrawFrameRect (x + 1, y + 1, 30, 30, RGB (255, 200, 0));
			DrawFrameRect (x + 2, y + 2, 28, 28, RGB (255, 120, 0));
		} else {
			DrawFrameRect (x, y, 32, 32, RGB (90, 150, 255));
			DrawFrameRect (x + 1, y + 1, 30, 30, RGB (90, 150, 255));
		}
	}

	/* 編集中の道しるべ */
	nCount = NpcRouteGetEditCount ();
	if (nCount <= 0) {
		return;
	}
	for (i = 0; i < nCount && i < MAX_NPCROUTE; i ++) {
		NpcRouteGetEditPoint (i, x, y);
		anX[i] = ((x - xTmp) + DRAW_PARTS_X + 1) * 32 + 16;
		anY[i] = ((y - yTmp) + DRAW_PARTS_Y + 1) * 32 + 16;
	}
	/* 線（先に描いて、番号を上に重ねる） */
	for (i = 1; i < nCount; i ++) {
		DrawLineClip (anX[i - 1], anY[i - 1], anX[i], anY[i], 3, clLine);
	}
	if (NpcRouteGetEditMode () == NPCROUTEMODE_LOOP && nCount >= 3) {
		DrawLineClip (anX[nCount - 1], anY[nCount - 1], anX[0], anY[0], 1, clLine);
	}
	/* 番号の札 */
	for (i = 0; i < nCount; i ++) {
		FillRectClip (anX[i] - 9, anY[i] - 8, 18, 16, RGB (255, 255, 255));
		FillRectClip (anX[i] - 8, anY[i] - 7, 16, 14, (i == 0) ? RGB (46, 158, 91) : RGB (194, 122, 0));
	}
	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);
	for (i = 0; i < nCount; i ++) {
		snprintf (szTmp, sizeof (szTmp), "%d", i + 1);
		TextOut2 (hDC, anX[i] - (int)strlen (szTmp) * 4, anY[i] - 7, szTmp, RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

/* ========================================================================= */
/* 関数名：DrawMapLightEdit													 */
/* 内容：明るさ・灯り・見え方の設定を開いている間、範囲の枠と光源の印を描く	 */
/*       範囲の枠の色は 暗くする=青 暗くしない=黄 明るく壁で隠す=緑。		 */
/*       選んでいる範囲は太く、選んでいる光源は照らす範囲も枠で出す			 */
/* ========================================================================= */
void DrawMapLightEdit(void)
{
	static const COLORREF aclArea[] = {RGB (255, 220, 60), RGB (80, 140, 255), RGB (70, 220, 120)};	/* MAPDARK_* の順 */
	int i, nSel, nW, x1, y1, x2, y2, xTmp, yTmp, anTagX[MAX_MAPDARKAREA + MAX_MAPLIGHT], anTagY[MAX_MAPDARKAREA + MAX_MAPLIGHT];
	char szTmp[16];
	HDC hDC;
	HFONT hFontOld;
	PCHARINFO pChar;
	COLORREF cl;

	if ((g_nMyCharID < 0) || (g_hWndAdmin == NULL) || (g_nAdminWindowType != IDM_MAP_INFO)) {
		return;
	}
	/* 自分のマップが変わったら閉じる */
	MapLightDlgTick ();
	if (!IsDlgMapLightOpen () || (g_pMapInfo == NULL)) {
		return;
	}

	pChar = g_aCharInfo[g_nMyCharID];
	xTmp = (CamTileX () - DRAW_PARTS_X - 1) * 32;		/* マップのドット座標から画像の座標へ */
	yTmp = (CamTileY () - DRAW_PARTS_Y - 1) * 32;

	/* 範囲の枠（選んでいるのは最後に太く） */
	nSel = MapLightDlgGetSelArea ();
	for (i = 0; i < g_pMapInfo->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
		PMAPDARKAREA pArea = &g_pMapInfo->aDarkArea[i];
		cl = aclArea[(pArea->byDark <= MAPDARK_HIDE) ? pArea->byDark : MAPDARK_DARK];
		nW = (i == nSel) ? 4 : 2;
		x1 = pArea->wX1 * 32 - xTmp;
		y1 = pArea->wY1 * 32 - yTmp;
		x2 = (pArea->wX2 + 1) * 32 - xTmp;
		y2 = (pArea->wY2 + 1) * 32 - yTmp;
		FillRectClip (x1, y1, x2 - x1, nW, cl);
		FillRectClip (x1, y2 - nW, x2 - x1, nW, cl);
		FillRectClip (x1, y1, nW, y2 - y1, cl);
		FillRectClip (x2 - nW, y1, nW, y2 - y1, cl);
		/* 番号の札は左上（画面の外なら画面の端に寄せる） */
		anTagX[i] = max (x1 + nW, 32 + 2);
		anTagY[i] = max (y1 + nW, 32 + 2);
		if ((x2 <= 32) || (y2 <= 32)) {
			anTagX[i] = anTagY[i] = -1000;		/* 範囲が画面の左・上の外 */
			continue;
		}
		FillRectClip (anTagX[i], anTagY[i], 16, 14, cl);
	}

	/* 右下のクリック待ちなら、左上のマスに印 */
	if (MapLightDlgGetCorner (x1, y1)) {
		x1 = x1 * 32 - xTmp;
		y1 = y1 * 32 - yTmp;
		FillRectClip (x1, y1, 32, 3, RGB (255, 255, 255));
		FillRectClip (x1, y1, 3, 32, RGB (255, 255, 255));
	}

	/* 光源（選んでいるのは照らす範囲も枠で出す） */
	nSel = MapLightDlgGetSelLight ();
	for (i = 0; i < g_pMapInfo->byLightCount && i < MAX_MAPLIGHT; i ++) {
		PMAPLIGHT pLight = &g_pMapInfo->aLight[i];
		int nNo = MAX_MAPDARKAREA + i;
		cl = (pLight->byFlg & MAPLIGHTFLG_WARM) ? RGB (255, 150, 40) : RGB (255, 245, 170);
		x1 = pLight->wX * 32 - xTmp;
		y1 = pLight->wY * 32 - yTmp;
		if (i == nSel) {
			int r = pLight->byRange;
			x2 = (pLight->wX - r) * 32 - xTmp;
			y2 = (pLight->wY - r) * 32 - yTmp;
			nW = (r * 2 + 1) * 32;
			FillRectClip (x2, y2, nW, 1, cl);
			FillRectClip (x2, y2 + nW - 1, nW, 1, cl);
			FillRectClip (x2, y2, 1, nW, cl);
			FillRectClip (x2 + nW - 1, y2, 1, nW, cl);
			FillRectClip (x1 + 3, y1 + 3, 26, 26, RGB (255, 255, 255));
		}
		FillRectClip (x1 + 7, y1 + 8, 18, 16, RGB (40, 40, 40));
		FillRectClip (x1 + 8, y1 + 9, 16, 14, cl);
		anTagX[nNo] = x1 + 8;
		anTagY[nNo] = y1 + 9;
	}

	/* 番号 */
	hDC = g_pDibDrawMap->Lock ();
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);
	for (i = 0; i < g_pMapInfo->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
		if (anTagX[i] < 0) {
			continue;
		}
		snprintf (szTmp, sizeof (szTmp), "%d", i + 1);
		TextOut2 (hDC, anTagX[i] + 8 - (int)strlen (szTmp) * 3, anTagY[i] + 1, szTmp, RGB (255, 255, 255));
	}
	for (i = 0; i < g_pMapInfo->byLightCount && i < MAX_MAPLIGHT; i ++) {
		int nNo = MAX_MAPDARKAREA + i;
		if (anTagX[nNo] < 0 || anTagY[nNo] < 0) {
			continue;
		}
		snprintf (szTmp, sizeof (szTmp), "%d", i + 1);
		TextOut2 (hDC, anTagX[nNo] + 8 - (int)strlen (szTmp) * 3, anTagY[nNo] + 1, szTmp, RGB (255, 255, 255));
	}
	SelectObject (hDC, hFontOld);
	g_pDibDrawMap->Unlock ();
}

/* ========================================================================= */
/* 関数名：DrawMapSwapEdit													 */
/* 内容：印で変わるマス・宝箱の画面を開いている間、表の各行のマスに枠を描く	 */
/*       枠の色は 印=水色 宝箱=黄 リサイクル箱=緑。選んでいる行は太く白で重ねる	 */
/*       自分のマップが変わったら表を取り直す								 */
/* ========================================================================= */
void DrawMapSwapEdit(void)
{
	static const COLORREF aclKind[] = {RGB (80, 220, 255), RGB (255, 220, 60), RGB (70, 220, 120)};	/* MAPSWAPKIND_* の順 */
	int i, nSel, xTmp, yTmp, x1, y1, nW;
	PCHARINFO pChar;

	if ((g_nMyCharID < 0) || (g_hWndAdmin == NULL) || (g_nAdminWindowType != IDM_MAP_SWAP)) {
		return;
	}
	MapSwapDlgTick ();
	if (!IsDlgMapSwapOpen () || (g_pMapInfo == NULL)) {
		return;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	xTmp = (CamTileX () - DRAW_PARTS_X - 1) * 32;		/* マップのドット座標から画像の座標へ */
	yTmp = (CamTileY () - DRAW_PARTS_Y - 1) * 32;
	nSel = MapSwapDlgGetSel ();
	for (i = 0; i < MapSwapDlgGetCount (); i ++) {
		const ADMINSWAPROW *pRow = MapSwapDlgGetRow (i);
		COLORREF cl = aclKind[(pRow->byKind <= MAPSWAPKIND_RECYCLE) ? pRow->byKind : 0];

		x1 = pRow->wX * 32 - xTmp;
		y1 = pRow->wY * 32 - yTmp;
		nW = (i == nSel) ? 4 : 2;
		if (i == nSel) {
			FillRectClip (x1 - 2, y1 - 2, 36, 2, RGB (255, 255, 255));
			FillRectClip (x1 - 2, y1 + 32, 36, 2, RGB (255, 255, 255));
			FillRectClip (x1 - 2, y1, 2, 32, RGB (255, 255, 255));
			FillRectClip (x1 + 32, y1, 2, 32, RGB (255, 255, 255));
		}
		FillRectClip (x1, y1, 32, nW, cl);
		FillRectClip (x1, y1 + 32 - nW, 32, nW, cl);
		FillRectClip (x1, y1, nW, 32, cl);
		FillRectClip (x1 + 32 - nW, y1, nW, 32, cl);
	}
}

/* ========================================================================= */
/* 関数名：	DrawSnowFall													 */
/* 内容：	しとしと降る雪の描画											 */
/*			昔は32ドットの雪パーツを全部のマスに敷いていたので、同じ並びの	 */
/*			点が格子になって見えていた。雪粒を1つずつ、奥・中・手前の3段で	 */
/*			大きさと速さを変えて、ゆらゆら揺れながら落ちるように描く。		 */
/*			雪粒の場所は時刻だけから決める(毎フレームの更新は要らない)。	 */
/*			歩くと手前の雪ほど大きく流れて、奥行きが出る					 */
/* 日付：	2026/11/10														 */
/* ========================================================================= */
#define SNOW_COUNT_DOT		70		/* いちばん奥の雪粒(1ドット)の数 */
#define SNOW_COUNT_FAR		45		/* 奥の雪粒(3ドット)の数 */
#define SNOW_COUNT_MID		22		/* 中の雪粒(5ドット)の数 */
#define SNOW_COUNT_NEAR		7		/* 手前の雪粒(7ドット)の数 */
#define SNOW_COUNT			(SNOW_COUNT_DOT + SNOW_COUNT_FAR + SNOW_COUNT_MID + SNOW_COUNT_NEAR)

typedef struct _SNOWFLAKE {
	float	fX, fY;			/* 時刻0の場所 */
	float	fSpeed;			/* 落ちる速さ(ドット/秒) */
	float	fSwayAmp;		/* 横に揺れる幅(ドット) */
	float	fSwayFreq;		/* 揺れの速さ */
	float	fPhase;			/* 揺れのずれ */
	float	fParallax;		/* 歩いた時の流れ方(手前ほど大きい) */
	BYTE	byLayer;		/* 0=いちばん奥 1=奥 2=中 3=手前 */
} SNOWFLAKE;

static SNOWFLAKE s_aSnow[SNOW_COUNT];
static BOOL s_bSnowInit = FALSE;

/* 決まった並びの乱数(起動ごとに同じ降り方) */
static float SnowRand(DWORD &dwSeed)
{
	dwSeed = dwSeed * 1103515245 + 12345;
	return (float)((dwSeed >> 8) & 0xFFFF) / 65535.0f;
}

static void InitSnowFall(void)
{
	int i;
	DWORD dwSeed = 20261110;
	float fW = (float)(SCRSIZEX + 32), fH = (float)(SCRSIZEY + 32);

	for (i = 0; i < SNOW_COUNT; i ++) {
		SNOWFLAKE *p = &s_aSnow[i];
		p->byLayer = (i < SNOW_COUNT_DOT) ? 0 : (i < SNOW_COUNT_DOT + SNOW_COUNT_FAR) ? 1 :
					  (i < SNOW_COUNT_DOT + SNOW_COUNT_FAR + SNOW_COUNT_MID) ? 2 : 3;
		p->fX = SnowRand (dwSeed) * fW;
		p->fY = SnowRand (dwSeed) * fH;
		p->fPhase = SnowRand (dwSeed) * 6.2832f;
		p->fSwayFreq = 0.5f + SnowRand (dwSeed) * 0.9f;
		switch (p->byLayer) {
		case 0:
			p->fSpeed	= 12.0f + SnowRand (dwSeed) * 6.0f;
			p->fSwayAmp	= 1.5f + SnowRand (dwSeed) * 2.0f;
			p->fParallax = 0.7f;
			break;
		case 1:
			p->fSpeed	= 17.0f + SnowRand (dwSeed) * 8.0f;
			p->fSwayAmp	= 2.0f + SnowRand (dwSeed) * 3.0f;
			p->fParallax = 0.8f;
			break;
		case 2:
			p->fSpeed	= 26.0f + SnowRand (dwSeed) * 10.0f;
			p->fSwayAmp	= 4.0f + SnowRand (dwSeed) * 5.0f;
			p->fParallax = 0.95f;
			break;
		default:
			p->fSpeed	= 38.0f + SnowRand (dwSeed) * 14.0f;
			p->fSwayAmp	= 7.0f + SnowRand (dwSeed) * 7.0f;
			p->fParallax = 1.15f;
			break;
		}
	}
	s_bSnowInit = TRUE;
}

/* 1ドットを白へ寄せる(nAlpha は 0〜256) */
static inline void SnowPlot(LPBYTE pBits, int nWidth, int nHeight, int x, int y, int nAlpha)
{
	if ((x < 0) || (y < 0) || (x >= nWidth) || (y >= nHeight) || (nAlpha <= 0)) {
		return;
	}
	PDWORD pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + x;
	DWORD dw = *pDst;
	int r = (dw >> 16) & 0xFF, g = (dw >> 8) & 0xFF, b = dw & 0xFF;
	/* ほんの少し青みのある白 */
	r += ((246 - r) * nAlpha) >> 8;
	g += ((250 - g) * nAlpha) >> 8;
	b += ((255 - b) * nAlpha) >> 8;
	*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
}

/* 1ドットを少し暗くする(雪の地面の上でも手前の雪粒が見えるように) */
static inline void SnowShade(LPBYTE pBits, int nWidth, int nHeight, int x, int y, int nAlpha)
{
	if ((x < 0) || (y < 0) || (x >= nWidth) || (y >= nHeight) || (nAlpha <= 0)) {
		return;
	}
	PDWORD pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + x;
	DWORD dw = *pDst;
	int r = (dw >> 16) & 0xFF, g = (dw >> 8) & 0xFF, b = dw & 0xFF;
	r -= ((r - 110) * nAlpha) >> 8;
	g -= ((g - 125) * nAlpha) >> 8;
	b -= ((b - 160) * nAlpha) >> 8;
	*pDst = (dw & 0xFF000000) | (max (r, 0) << 16) | (max (g, 0) << 8) | max (b, 0);
}

/* 画面の見えている所を、天気の空の色へ少しだけ寄せる(nAlpha は 0〜256) */
static void WeatherTint(LPBYTE pBits, int nWidth, int nHeight, int nSX, int nSY, int nR, int nG, int nB, int nAlpha)
{
	int x, y;

	for (y = max (nSY, 0); y < min (nSY + SCRSIZEY, nHeight); y ++) {
		PDWORD pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + max (nSX, 0);
		for (x = max (nSX, 0); x < min (nSX + SCRSIZEX, nWidth); x ++, pDst ++) {
			DWORD dw = *pDst;
			int r = (dw >> 16) & 0xFF, g = (dw >> 8) & 0xFF, b = dw & 0xFF;
			r += ((nR - r) * nAlpha) >> 8;
			g += ((nG - g) * nAlpha) >> 8;
			b += ((nB - b) * nAlpha) >> 8;
			*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
		}
	}
}

/* 雪粒の形(中心ほど白い丸) */
static const BYTE s_abySnowShape3[3][3] = {
	{  0, 200,   0},
	{200, 255, 200},
	{  0, 200,   0},
};
static const BYTE s_abySnowShape5[5][5] = {
	{  0, 150, 200, 150,   0},
	{150, 255, 255, 255, 150},
	{200, 255, 255, 255, 200},
	{150, 255, 255, 255, 150},
	{  0, 150, 200, 150,   0},
};
static const BYTE s_abySnowShape7[7][7] = {
	{  0,   0, 120, 170, 120,   0,   0},
	{  0, 170, 255, 255, 255, 170,   0},
	{120, 255, 255, 255, 255, 255, 120},
	{170, 255, 255, 255, 255, 255, 170},
	{120, 255, 255, 255, 255, 255, 120},
	{  0, 170, 255, 255, 255, 170,   0},
	{  0,   0, 120, 170, 120,   0,   0},
};

/* 雪粒を1つ描く。右下へ1ドットずらした淡い影を先に描いて、白い地面でも見えるように */
static void SnowStamp(LPBYTE pBits, int nWidth, int nHeight, int x, int y, const BYTE *pShape, int nSize, int nShade)
{
	int xx, yy, nHalf = nSize / 2;

	for (yy = 0; yy < nSize; yy ++) {
		for (xx = 0; xx < nSize; xx ++) {
			SnowShade (pBits, nWidth, nHeight, x - nHalf + xx + 1, y - nHalf + yy + 1, pShape[yy * nSize + xx] * nShade / 255);
		}
	}
	for (yy = 0; yy < nSize; yy ++) {
		for (xx = 0; xx < nSize; xx ++) {
			SnowPlot (pBits, nWidth, nHeight, x - nHalf + xx, y - nHalf + yy, pShape[yy * nSize + xx]);
		}
	}
}

static void DrawSnowFall(void)
{
	int i, nWidth, nHeight, nSX, nSY, nCamX, nCamY;
	float fTime, fW, fH;
	LPBYTE pBits;
	PCHARINFO pMyChar;

	if (g_nMyCharID < 0) {
		return;
	}
	pBits = g_pDibDrawMap->GetBits ();
	if (pBits == NULL) {
		return;
	}
	if (s_bSnowInit == FALSE) {
		InitSnowFall ();
	}
	pMyChar	= g_aCharInfo[g_nMyCharID];
	nWidth	= g_pDibDrawMap->Width ();
	nHeight	= g_pDibDrawMap->Height ();
	/* 画面に見えている所の左上(歩き中はずれる) */
	nSX = 32 - CamOfsX ();
	nSY = 32 - CamOfsY ();
	/* 見ている場所(ドット)。歩くと雪が後ろへ流れる */
	nCamX = CamTileX () * 32 - CamOfsX ();
	nCamY = CamTileY () * 32 - CamOfsY ();

	fTime = (float)(timeGetTime () % 3600000) / 1000.0f;
	fW = (float)(SCRSIZEX + 32);
	fH = (float)(SCRSIZEY + 32);

	/* 雪の日の曇り空: 画面を少しだけ青灰色に沈めて、白い雪粒が浮くように */
	WeatherTint (pBits, nWidth, nHeight, nSX, nSY, 150, 165, 195, 42);

	for (i = 0; i < SNOW_COUNT; i ++) {
		SNOWFLAKE *p = &s_aSnow[i];
		float fX, fY;
		int x, y;

		/* ゆるい風で少しだけ右へ流れる */
		fX = p->fX + fTime * 3.0f * p->fParallax + p->fSwayAmp * sinf (fTime * p->fSwayFreq + p->fPhase) - nCamX * p->fParallax;
		fY = p->fY + fTime * p->fSpeed - nCamY * p->fParallax;
		fX = fmodf (fX, fW);
		if (fX < 0.0f) {
			fX += fW;
		}
		fY = fmodf (fY, fH);
		if (fY < 0.0f) {
			fY += fH;
		}
		x = nSX + (int)fX - 16;
		y = nSY + (int)fY - 16;

		switch (p->byLayer) {
		case 0:		/* いちばん奥: 1ドット、うっすら */
			SnowPlot (pBits, nWidth, nHeight, x, y, 160);
			break;
		case 1:
			SnowStamp (pBits, nWidth, nHeight, x, y, &s_abySnowShape3[0][0], 3, 60);
			break;
		case 2:
			SnowStamp (pBits, nWidth, nHeight, x, y, &s_abySnowShape5[0][0], 5, 80);
			break;
		default:
			SnowStamp (pBits, nWidth, nHeight, x, y, &s_abySnowShape7[0][0], 7, 90);
			break;
		}
	}
}

/* ========================================================================= */
/* 関数名：	DrawRainFall													 */
/* 内容：	雨の描画														 */
/*			雪と同じく、昔の雨パーツを敷き詰める代わりに雨粒を1本ずつ		 */
/*			斜めの線で降らせる。奥の雨ほど細く短く遅い。地面には時々		 */
/*			小さな波紋(はね)を出す。画面は少し暗い灰色に沈める				 */
/* 日付：	2026/11/10														 */
/* ========================================================================= */
#define RAIN_COUNT_FAR		90		/* 奥の雨粒の数 */
#define RAIN_COUNT_MID		60		/* 中の雨粒の数 */
#define RAIN_COUNT_NEAR		24		/* 手前の雨粒の数 */
#define RAIN_COUNT			(RAIN_COUNT_FAR + RAIN_COUNT_MID + RAIN_COUNT_NEAR)
#define RAIN_SPLASH_COUNT	14		/* 同時に出る波紋の数 */
#define RAIN_SPLASH_TIME	0.35f	/* 波紋1つが広がって消えるまで(秒) */
#define RAIN_WIND			0.22f	/* 雨の傾き(下へ1ドットで右へ何ドット) */

typedef struct _RAINDROP {
	float	fX, fY;			/* 時刻0の場所 */
	float	fSpeed;			/* 落ちる速さ(ドット/秒) */
	float	fParallax;		/* 歩いた時の流れ方(手前ほど大きい) */
	BYTE	byLen;			/* 線の長さ(ドット) */
	BYTE	byAlpha;		/* 濃さ(0〜256) */
} RAINDROP;

static RAINDROP s_aRain[RAIN_COUNT];
static BOOL s_bRainInit = FALSE;

static void InitRainFall(void)
{
	int i;
	DWORD dwSeed = 20261111;
	float fW = (float)(SCRSIZEX + 64), fH = (float)(SCRSIZEY + 64);

	for (i = 0; i < RAIN_COUNT; i ++) {
		RAINDROP *p = &s_aRain[i];
		p->fX = SnowRand (dwSeed) * fW;
		p->fY = SnowRand (dwSeed) * fH;
		if (i < RAIN_COUNT_FAR) {
			p->fSpeed	= 300.0f + SnowRand (dwSeed) * 60.0f;
			p->byLen	= 6 + (BYTE)(SnowRand (dwSeed) * 3);
			p->byAlpha	= 110;
			p->fParallax = 0.8f;
		} else if (i < RAIN_COUNT_FAR + RAIN_COUNT_MID) {
			p->fSpeed	= 390.0f + SnowRand (dwSeed) * 70.0f;
			p->byLen	= 10 + (BYTE)(SnowRand (dwSeed) * 4);
			p->byAlpha	= 170;
			p->fParallax = 0.95f;
		} else {
			p->fSpeed	= 500.0f + SnowRand (dwSeed) * 90.0f;
			p->byLen	= 15 + (BYTE)(SnowRand (dwSeed) * 5);
			p->byAlpha	= 220;
			p->fParallax = 1.15f;
		}
	}
	s_bRainInit = TRUE;
}

/* 1ドットを雨の色へ寄せる(nAlpha は 0〜256) */
static inline void RainPlot(LPBYTE pBits, int nWidth, int nHeight, int x, int y, int nAlpha)
{
	if ((x < 0) || (y < 0) || (x >= nWidth) || (y >= nHeight) || (nAlpha <= 0)) {
		return;
	}
	PDWORD pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + x;
	DWORD dw = *pDst;
	int r = (dw >> 16) & 0xFF, g = (dw >> 8) & 0xFF, b = dw & 0xFF;
	r += ((225 - r) * nAlpha) >> 8;
	g += ((235 - g) * nAlpha) >> 8;
	b += ((250 - b) * nAlpha) >> 8;
	*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
}

static void DrawRainFall(void)
{
	int i, j, nWidth, nHeight, nSX, nSY, nCamX, nCamY;
	float fTime, fW, fH;
	LPBYTE pBits;
	PCHARINFO pMyChar;

	if (g_nMyCharID < 0) {
		return;
	}
	pBits = g_pDibDrawMap->GetBits ();
	if (pBits == NULL) {
		return;
	}
	if (s_bRainInit == FALSE) {
		InitRainFall ();
	}
	pMyChar	= g_aCharInfo[g_nMyCharID];
	nWidth	= g_pDibDrawMap->Width ();
	nHeight	= g_pDibDrawMap->Height ();
	nSX = 32 - CamOfsX ();
	nSY = 32 - CamOfsY ();
	nCamX = CamTileX () * 32 - CamOfsX ();
	nCamY = CamTileY () * 32 - CamOfsY ();

	fTime = (float)(timeGetTime () % 3600000) / 1000.0f;
	fW = (float)(SCRSIZEX + 64);
	fH = (float)(SCRSIZEY + 64);

	/* 雨の日の暗い空: 画面を少し灰色に沈める */
	WeatherTint (pBits, nWidth, nHeight, nSX, nSY, 60, 70, 90, 70);

	/* 地面のはね(小さな波紋)。時刻を区切って、区切りごとに場所を決め直す */
	for (i = 0; i < RAIN_SPLASH_COUNT; i ++) {
		float fSlot, fAge;
		DWORD dwSeed;
		int x, y, nR, nAlpha;

		fSlot = fTime / RAIN_SPLASH_TIME + (float)i / RAIN_SPLASH_COUNT;
		fAge = fSlot - floorf (fSlot);
		dwSeed = (DWORD)floorf (fSlot) * 7919 + i * 104729;
		/* 描画用画像の中ではマップのマスは動かないので、画像の中の場所で決めると
		   歩いても波紋が地面と一緒に動く */
		x = (int)(SnowRand (dwSeed) * nWidth);
		y = (int)(SnowRand (dwSeed) * nHeight);
		nR = 1 + (int)(fAge * 4.0f);
		nAlpha = (int)((1.0f - fAge) * 150.0f);
		/* 横長の楕円(上から斜めに見た水たまりの輪) */
		for (j = -nR * 2; j <= nR * 2; j ++) {
			int dy = (int)(sqrtf ((float)max (0, nR * nR * 4 - j * j)) / 4.0f + 0.5f);
			RainPlot (pBits, nWidth, nHeight, x + j, y - dy, nAlpha);
			if (dy) {
				RainPlot (pBits, nWidth, nHeight, x + j, y + dy, nAlpha);
			}
		}
	}

	/* 雨粒(斜めの線) */
	for (i = 0; i < RAIN_COUNT; i ++) {
		RAINDROP *p = &s_aRain[i];
		float fX, fY, fFall;
		int x, y;

		fFall = fTime * p->fSpeed;
		fX = p->fX + fFall * RAIN_WIND - nCamX * p->fParallax;
		fY = p->fY + fFall - nCamY * p->fParallax;
		fX = fmodf (fX, fW);
		if (fX < 0.0f) {
			fX += fW;
		}
		fY = fmodf (fY, fH);
		if (fY < 0.0f) {
			fY += fH;
		}
		x = nSX + (int)fX - 32;
		y = nSY + (int)fY - 32;
		/* 先(下)ほど濃く、尾は薄く */
		for (j = 0; j < p->byLen; j ++) {
			int nAlpha = p->byAlpha * (j + 1) / p->byLen;
			RainPlot (pBits, nWidth, nHeight, x - (int)((p->byLen - j) * RAIN_WIND), y - (p->byLen - j), nAlpha);
			/* 手前の雨は少し太く */
			if (p->fParallax > 1.0f) {
				RainPlot (pBits, nWidth, nHeight, x + 1 - (int)((p->byLen - j) * RAIN_WIND), y - (p->byLen - j), nAlpha / 2);
			}
		}
	}
}


/* ========================================================================= */
/* 関数名：DrawWeather														 */
/* 内容：天候の描画															 */
/* 日付：2004/07/19															 */
/* ========================================================================= */
void DrawWeather(void)
{
	if (g_nMyCharID < 0) {
		return;
	}
	if (g_pMapInfo->byWeather == 0) {
		return;
	}
	if (g_bWeatherDraw == FALSE) {
		return;
	}
	/* 雪と雨は、パーツを敷き詰めずに粒を降らせる */
	switch (g_pMapInfo->byWeather) {
	case 1:
		DrawSnowFall ();
		return;
	case 2:
		DrawRainFall ();
		return;
	}

	PMAPPARTSINFO pParts;
	PCHARINFO pMyChar = g_aCharInfo[g_nMyCharID];
	BYTE byLevel, byLight = pMyChar->byLightLevel;
	int i, j, x, y, mx, my, xMax;

	int nTmp = GetMapLightLevel () - byLight;
	nTmp = max (nTmp, 0);
	nTmp = min (nTmp, 100);
	byLight = (BYTE)nTmp;

	mx = CamTileX ();
	my = CamTileY ();
	xMax = DRAW_PARTS_X * 2 + 3;
	for (y = 0; y < DRAW_PARTS_Y * 2 + 3; y ++) {
		j = my - (DRAW_PARTS_Y + 1) + y;
		for (x = 0; x < xMax; x ++) {
			WORD wMap;
			i = mx - (DRAW_PARTS_X + 1) + x;

			byLevel = 100;
			if (GetMapLightLevel () || IsMapViewMode ()) {
				byLevel = g_pMapDarkTmp[x + (DRAW_PARTS_X * 2 + 3) * y];
			}

			/* 範囲外？ */
			if ((i < 0) || (i >= g_pMapInfo->nCX) || (j < 0) || (j >= g_pMapInfo->nCY)) {
				continue;
			}

			switch (g_pMapInfo->byWeather) {
			case 1:
				wMap = 1527;
				break;
			case 2:
				wMap = 1528;
				break;
			case 3:
				wMap = 1529;
				break;
			case 4:
				wMap = 1530;
				break;
			}
			if (wMap >= g_wCountMapParts) {
				continue;
			}

			PANIMEINFO pAnime = NULL;
			DWORD dwNo, dwBackNo;
			BOOL bDrawBack = TRUE, bDrawMain = TRUE;
			pParts = &g_pMapPartsInfo[wMap];
			int no, xx, yy;
			DWORD dwID;
			BYTE byLevelParts = 0;

			/* アニメーションするパーツ？ */
			if (pParts->byAnimeCount) {
				pAnime = &pParts->pAnimeInfo[pParts->byAnimeNo];
				dwNo = pAnime->wGrpID;
				dwBackNo = pAnime->wBackID;

			} else {
				dwNo = pParts->wGrpID;
				dwBackNo = pParts->wBackID;
			}

			no = dwNo / (g_byMapGrpCountX * g_byMapGrpCountY);
			dwID = dwNo % (g_byMapGrpCountX * g_byMapGrpCountY);
			xx = (dwID % g_byMapGrpCountX) * 32;
			yy = dwID / g_byMapGrpCountX * 32;
			byLevelParts = 0;

			if (pAnime) {
				if (pAnime->byLevel) {
					byLevelParts = pAnime->byLevel;
				}
			} else {
				if (pParts->byLevel) {
					byLevelParts = pParts->byLevel;
				}
			}

			if (byLevelParts) {
				g_pDibDrawMap->BltAlpha (x * 32, y * 32, 32, 32, g_apDibMap[no], xx, yy, byLevelParts, TRUE);
				if (byLevel < 100) {
					g_pDibDrawMap->ChgLevel (x * 32, y * 32, 32, 32, byLevel);
				}

			} else {
				g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_apDibMap[no], xx, yy, byLevel, TRUE);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：AddSysMsg														 */
/* 内容：システムメッセージの追加											 */
/* 日付：2003/05/25															 */
/* ========================================================================= */
static void AddSysMsgLine(LPCSTR pszStr, int nLen, COLORREF cl);

void AddSysMsg(LPSTR pszStr, COLORREF cl)
{
	/* 画面の幅に収まらない長い文は、何行かに分けて流す（はみ出した分が見切れていた） */
	const int nMaxWidth = SCRSIZEX / 8 - 2;
	LPCSTR pszSrc = pszStr;
	int nRest = strlen (pszStr);

	do {
		int nLen = nRest;
		if (SboTextWidth ((LPSTR)pszSrc) > nMaxWidth) {
			nLen = SboTextFitWidth (pszSrc, nMaxWidth);
			if (nLen <= 0) {
				nLen = max (SboTextCharLen (pszSrc), 1);
			}
			nLen = min (nLen, nRest);
		}
		AddSysMsgLine (pszSrc, nLen, cl);
		pszSrc += nLen;
		nRest -= nLen;
	} while (nRest > 0);
}

/* 1 行ぶんを流す */
static void AddSysMsgLine(LPCSTR pszStr, int nLen, COLORREF cl)
{
	int i, y = 0, tmp, nCount = g_SysMsgInfo.GetSize ();
	int nStart = SYSMSG_SY - GetSysMsgBottomOfs ();

	if (nCount > 0) {
		y = g_SysMsgInfo[nCount - 1]->nY;
	}

	if (y >= nStart - 15) {
		tmp = y - (nStart - 15);
		for (i = 0; i < nCount; i ++) {
			g_SysMsgInfo[i]->nY -= tmp;
		}
	}

	int nSize = nLen + 1;
	LPSTR pszTmp = new char[nSize];
	ZeroMemory (pszTmp, nSize);
	CopyMemory (pszTmp, pszStr, nSize - 1);

	PSYSMSGINFO pSysMsgInfo = new SYSMSGINFO;
	ZeroMemory (pSysMsgInfo, sizeof (SYSMSGINFO));
	pSysMsgInfo->nY = nStart;
	pSysMsgInfo->cl = cl;
	pSysMsgInfo->pszMsg = pszTmp;
	pSysMsgInfo->dwTime = timeGetTime ();
	g_SysMsgInfo.Add (pSysMsgInfo);
}

/* Copyright(C)URARA-works 2003-2006 */
