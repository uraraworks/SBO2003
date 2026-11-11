/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboCliDrawScreen.cpp										 */
/* 内容：		画面表示													 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#include "stdafx.h"
#include "Glbldata.h"
#include "SboCliDrawWindow.h"
#include "SboCliRecvCmd.h"
#include "AdminWindowOld/AdminDlgNpcRoute.h"
#include "AdminWindowOld/AdminDlgMapLight.h"
#include <math.h>

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
void DrawETC			(void);										/* ゲージ等の描画 */
static void DrawFrameRect	(int x, int y, int cx, int cy, COLORREF cl);	/* 1ドットの枠を描く */
static void DrawGuardMark	(int x, int y);								/* 構えの盾マーク */
void DrawObj			(void);										/* オブジェクトの描画 */
void DrawEvent			(void);										/* イベント情報の描画 */
void DrawMapEdit		(void);										/* マップ編集情報の描画 */
void DrawNpcRoute		(void);										/* NPCの巡回情報の描画 */
void DrawMapLightEdit	(void);										/* 明るさの範囲・光源の編集中の印 */
static void FillRectClip	(int x, int y, int cx, int cy, COLORREF cl);	/* 画像の外にはみ出さない矩形塗りつぶし */
static void DrawLineClip	(int x1, int y1, int x2, int y2, int nWidth, COLORREF cl);	/* 線を描く */
void DrawWeather		(void);										/* 天候の描画 */
static void DrawMapLight	(const char *pMapDark);						/* 丸くゆらめく灯りの描画 */
static void DrawMapViewFog	(void);										/* 見える範囲の外に白い霧をかける */

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
		x = (int)pChar->dwX - (int)pMyChar->dwX + (DRAW_PARTS_X + 1);
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
				x = anPosAddTblX[pChar->byDirection] * pChar->byDotMove;
				y = anPosAddTblY[pChar->byDirection] * pChar->byDotMove;

				/* ウィンドウを表示 */
				DrawWindow (g_pDibDrawMap, 32 - x, 32 - y);

				hDCPrimary = g_pDibDrawMap->Lock ();
				HFONT hFontOld = (HFONT)SelectObject (hDCPrimary, g_hFont14);
				SetBkMode (hDCPrimary, TRANSPARENT);
				for (i = 0; i < nCount; i ++) {
					PSYSMSGINFO pSysMsgInfo = g_SysMsgInfo[i];
					if (pSysMsgInfo->nY >= SYSMSG_EY) {
						TextOut2 (hDCPrimary, 32 - x, 32 - y + pSysMsgInfo->nY, pSysMsgInfo->pszMsg, pSysMsgInfo->cl);
					}
				}
				SelectObject (hDCPrimary, hFontOld);
				g_pDibDrawMap->Unlock ();
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
				if (g_pMapInfo->pszName) {
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

					pDibDrawTmp.FillRect (0, 0, (SboTextWidth ((LPSTR)g_pMapInfo->pszName) + 1) * 16, 66, RGB (0, 0, 0));

					HDC hDC = pDibDrawTmp.Lock ();
					HFONT hFontOld = (HFONT)SelectObject (hDC, g_hFont32);
					SetBkMode (hDC, TRANSPARENT);

					TextOut2 (hDC, 0, 0, (LPSTR)g_pMapInfo->pszName, RGB (255, 255, 255));

					SelectObject (hDC, hFontOld);
					pDibDrawTmp.Unlock ();
					int xTmp = (SCRSIZEX / 2) - (SboTextWidth ((LPSTR)g_pMapInfo->pszName) * 8);
					xTmp = max (xTmp, 0);
					g_pDibDrawMap->BltAlpha (32 - x + xTmp,
								32 - y + 5 * 32, (SboTextWidth ((LPSTR)g_pMapInfo->pszName) + 1) * 16, 64, &pDibDrawTmp, 0, 0, byLv, TRUE);
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
	DrawChar2 ();			/* エフェクトを描画 */
	DrawEvent ();			/* イベント情報を描画 */
	DrawMapEdit ();			/* マップ編集情報を描画 */
	DrawNpcRoute ();		/* NPCの巡回情報を描画 */
	DrawMapLightEdit ();	/* 明るさの範囲・光源の編集中の印を描画 */
	DrawChar3 ();			/* 文字を描画 */
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

static void DrawMapLight(const char *pMapDark)
{
	int i, x, y, nCount, nLight, nWidth, nHeight, nCX, nCY, nSX, nSY, nMapW;
	int anX[MAPLIGHT_MAX], anY[MAPLIGHT_MAX];
	float afR[MAPLIGHT_MAX], afPower[MAPLIGHT_MAX], afWarm[MAPLIGHT_MAX], fFlicker, fTime;
	float afVisX[DRAW_PARTS_X * 2 + 3];
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
		int nLevel, xTmp, yTmp;

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
		xTmp = (int)pMyChar->dwX - (int)pChar->dwX;
		yTmp = (int)pMyChar->dwY - (int)pChar->dwY;
		if ((abs (xTmp) > DRAW_PARTS_X + 1) || (abs (yTmp) > DRAW_PARTS_Y + 1)) {
			continue;
		}
		/* 1マス進むごとに暗くなる量(RenewMapDarkWork と同じ) */
		nLevel = GetMapLightLevel ();
		if (pChar->byLightLevel) {
			nLevel = GetMapLightLevel () - pChar->byLightLevel;
		}
		/* 灯りの中心(歩き中のずれも入れて、なめらかについてくるように) */
		anX[nLight] = (DRAW_PARTS_X + 1) * 32 + 16 - xTmp * 32 - anPosAddTblX[pChar->byDirection] * pChar->byDotMove;
		anY[nLight] = (DRAW_PARTS_Y + 1) * 32 + 16 - yTmp * 32 - anPosAddTblY[pChar->byDirection] * pChar->byDotMove;
		/* 明るさが 0 になる距離(ドット)。灯りが強くて暗くならない時は 0 */
		afR[nLight] = 0.0f;
		afPower[nLight] = 1.0f;
		afWarm[nLight] = 0.0f;
		if (nLevel > 0) {
			afR[nLight] = (100.0f / nLevel + 0.2f) * 32.0f * fFlicker;
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
			my = (int)pMyChar->dwY + y;
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
				mx = (int)pMyChar->dwX + x;
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

	/* 画面に見えている範囲だけ処理する */
	nSX = 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nSY = 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;
	nCX = SCRSIZEX;
	nCY = SCRSIZEY;
	if (nSX < 0) { nCX += nSX; nSX = 0; }
	if (nSY < 0) { nCY += nSY; nSY = 0; }
	nCX = min (nCX, nWidth - nSX);
	nCY = min (nCY, nHeight - nSY);

	for (y = nSY; y < nSY + nCY; y ++) {
		PDWORD pDst;
		float fTy, fVisY0, fVisY1;
		int nTy;

		/* 見えるマスかどうかをマスの中心どうしでなめらかにつなぐ(縦) */
		fTy = (y - 16) / 32.0f;
		nTy = (int)floorf (fTy);
		fTy -= nTy;
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
			int nTx, x0, x1, nLevel, nWarm;

			/* 見えるマスかどうか(横) */
			fTx = (x - 16) / 32.0f;
			nTx = (int)floorf (fTx);
			fTx -= nTx;
			x0 = max (0, min (nTx, nMapW - 1));
			x1 = max (0, min (nTx + 1, nMapW - 1));
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
				fLight = 1.0f;
				i = nLight;
			} else {
				i = 0;
			}
			for (; i < nLight; i ++) {
				float fDX, fDY, fD;
				if (afR[i] <= 0.0f) {
					fLight = 1.0f;
					break;
				}
				fDX = (float)(x - anX[i]);
				fDY = (float)(y - anY[i]);
				fD = fDX * fDX + fDY * fDY;
				if (fD >= afR[i] * afR[i]) {
					continue;		/* 届かない灯りは平方根を取らない */
				}
				fD = sqrtf (fD);
				if (fD < afR[i]) {
					float fAdd = (1.0f - fD / afR[i]) * afPower[i];
					fLight += fAdd;
					fWarm += fAdd * afWarm[i];
				}
			}
			if (fLight > 1.0f) {
				fLight = 1.0f;
			}
			if (fVis < 1.0f) {
				fLight *= fVis;
			}

			nLevel = (int)(fLight * 256.0f);
			/* 暖色の光源の光が届いている所は橙色に寄せる */
			nWarm = (int)(min (fWarm, 1.0f) * 256.0f);
			if (fVis < 1.0f) {
				nWarm = (int)(nWarm * fVis);
			}
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

	mx = pMyChar->dwX;
	my = pMyChar->dwY;
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
	dwMyX		= pCharMyChar->dwX;
	dwMyY		= pCharMyChar->dwY;
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

		if (pCharMyChar->byAdminLV == ADMINLV_USER) {
			if (pChar->byNpcType == NPCTYPE_PUT) {
				continue;
			}
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
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);

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
					g_pDibDrawMap->BltAlpha (x, y, nGrpSizeX, nGrpSizeY, pDib, xx, yy, pChar->byFadeLevel, TRUE);

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

	DWORD dwMyX = g_aCharInfo[g_nMyCharID]->dwX,
		  dwMyY = g_aCharInfo[g_nMyCharID]->dwY;

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

		if (g_aCharInfo[g_nMyCharID]->byAdminLV == ADMINLV_USER) {
			if (pChar->byNpcType == NPCTYPE_PUT) {
				continue;
			}
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
		if (i != g_nMyCharID) {
			x -= (xTmp * 32);
			y -= (yTmp * 32);
		}
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);

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

		if (g_aCharInfo[g_nMyCharID]->byAdminLV == ADMINLV_USER) {
			if (pChar->byNpcType == NPCTYPE_PUT) {
				continue;
			}
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
		if (i != g_nMyCharID) {
			x -= (xTmp * 32);
			y -= (yTmp * 32);
		}
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);
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

	DWORD dwMyX = g_aCharInfo[g_nMyCharID]->dwX,
		  dwMyY = g_aCharInfo[g_nMyCharID]->dwY;

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

		if (g_aCharInfo[g_nMyCharID]->byAdminLV == ADMINLV_USER) {
			if (pChar->byNpcType == NPCTYPE_PUT) {
				continue;
			}
		}

		pDibDrawTmp.FillRect (0, 0, 8 * 20, 14 * 2, RGB (0, 0, 0));

		x = 256;
		y = 256;
		if (i != g_nMyCharID) {
			x -= (xTmp * 32);
			y -= (yTmp * 32);
		}
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);

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

		if ((g_byViewMode == 1) || (g_byViewMode == 3)) {
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
	x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
	y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);

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
			y -= 48;
		}
	}

	pChar = g_aCharInfo[g_nMyCharID];
	/* 魔法ターンゲージ表示 */
	if (pChar->dwTimeTurnMagicLV) {
		x = 256;
		y = 256;
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);
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
		x = 256;
		y = 256;
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);
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
		x = 256;
		y = 256;
		x -= (anPosAddTblX[pChar->byDirection] * pChar->byDotMove);
		y -= (anPosAddTblY[pChar->byDirection] * pChar->byDotMove);
		x -= 3;
		y -= 16;
		if (pChar->dwTimeTurnMagicLV || pChar->dwTimeTurnLV) {
			DrawFrameRect (x - 1, y - 1, 40, 14, RGB (90, 180, 255));
		}
		DrawGuardMark (x + 40, y - 2);
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
/* 内容：構えの盾マーク(幅9×高さ12。白縁の青い盾に白い線)				 */
/* ========================================================================= */
static void DrawGuardMark(int x, int y)
{
	/* 1行ごとの左右の幅(下に行くほど細くなる盾の形) */
	static const int anInset[12] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4};
	int i, nL, nW;

	for (i = 0; i < 12; i ++) {
		nL = anInset[i];
		nW = 9 - nL * 2;
		/* 白い縁 */
		g_pDibDrawMap->FillRect (x + nL, y + i, nW, 1, RGB (255, 255, 255));
		/* 中の青 */
		if ((i > 0) && (i < 11) && (nW > 2)) {
			g_pDibDrawMap->FillRect (x + nL + 1, y + i, nW - 2, 1, RGB (60, 140, 255));
		}
	}
	/* 真ん中の白い線 */
	g_pDibDrawMap->FillRect (x + 4, y + 2, 1, 7, RGB (255, 255, 255));
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
		nMyX = pChar->dwX, nMyY = pChar->dwY;
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
	xTmp = (int)pChar->dwX;
	yTmp = (int)pChar->dwY;

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
	xTmp = (int)pChar->dwX;
	yTmp = (int)pChar->dwY;

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
	xTmp = (int)pChar->dwX;
	yTmp = (int)pChar->dwY;
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
	xTmp = ((int)pChar->dwX - DRAW_PARTS_X - 1) * 32;		/* マップのドット座標から画像の座標へ */
	yTmp = ((int)pChar->dwY - DRAW_PARTS_Y - 1) * 32;

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
	nSX = 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nSY = 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;
	/* 見ている場所(ドット)。歩くと雪が後ろへ流れる */
	nCamX = (int)pMyChar->dwX * 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nCamY = (int)pMyChar->dwY * 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;

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
	nSX = 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nSY = 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;
	nCamX = (int)pMyChar->dwX * 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nCamY = (int)pMyChar->dwY * 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;

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

	mx = pMyChar->dwX;
	my = pMyChar->dwY;
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

	if (nCount > 0) {
		y = g_SysMsgInfo[nCount - 1]->nY;
	}

	if (y >= SYSMSG_SY - 15) {
		tmp = y - (SYSMSG_SY - 15);
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
	pSysMsgInfo->nY = SYSMSG_SY;
	pSysMsgInfo->cl = cl;
	pSysMsgInfo->pszMsg = pszTmp;
	pSysMsgInfo->dwTime = timeGetTime ();
	g_SysMsgInfo.Add (pSysMsgInfo);
}

/* Copyright(C)URARA-works 2003-2006 */
