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
void DrawObj			(void);										/* オブジェクトの描画 */
void DrawEvent			(void);										/* イベント情報の描画 */
void DrawMapEdit		(void);										/* マップ編集情報の描画 */
void DrawWeather		(void);										/* 天候の描画 */
void AddSysMsg			(LPSTR, COLORREF);							/* システムメッセージの追加 */

/* ========================================================================= */
/* 関数名：TextOut2															 */
/* 内容：黒縁取りで文字描画													 */
/* 日付：2002/07/15															 */
/* ========================================================================= */
void TextOut2(HDC hdc, int iX, int iY, char *pStr, COLORREF Color)
{
	int nLen = strlen (pStr);
	SetTextColor (hdc, RGB(10, 10, 10));
	TextOut (hdc, iX - 1, iY, pStr, nLen);
	TextOut (hdc, iX + 1, iY, pStr, nLen);
	TextOut (hdc, iX, iY - 1, pStr, nLen);
	TextOut (hdc, iX, iY + 1, pStr, nLen);
	SetTextColor (hdc, Color);
	TextOut (hdc, iX, iY, pStr, nLen);
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
	wsprintf (szBuf, "Presented By URARA-works 2003-2007");
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
	wsprintf (szBuf, "Presented By URARA-works 2003-2007");
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
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawMapLine (i, -1);
	}

	DrawObj ();				/* オブジェクトを描画 */
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		DrawCharLine (i);
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
	DrawWeather ();
	DrawChar2 ();			/* エフェクトを描画 */
	DrawEvent ();			/* イベント情報を描画 */
	DrawMapEdit ();			/* マップ編集情報を描画 */
	DrawChar3 ();			/* 文字を描画 */
	DrawETC ();				/* ゲージ等の描画 */

	if (g_hWndDebug) {
		InvalidateRect (g_hWndDebug, NULL, TRUE);
	}
	return TRUE;
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
	nTmp = g_pMapInfo->byLightLevel - byLight;
	nTmp = max (nTmp, 0);
	nTmp = min (nTmp, 100);
	byLight = (BYTE)nTmp;

	mx = pMyChar->dwX;
	my = pMyChar->dwY;
	j = my - (DRAW_PARTS_Y + 1) + y;
	xMax = DRAW_PARTS_X * 2 + 3;
	for (x = 0; x < xMax; x ++) {
		WORD wMap;
		i = mx - (DRAW_PARTS_X + 1) + x;

		byLevel = 100;
		if (g_pMapInfo->byLightLevel) {
			byLevel = g_pMapDarkTmp[x + (DRAW_PARTS_X * 2 + 3) * nLine];
		}

		/* 範囲外？ */
		if ((i < 0) || (i >= g_pMapInfo->nCX) || (j < 0) || (j >= g_pMapInfo->nCY)) {
			if (nType < 0) {
				g_pDibDrawMap->BltLevel (x * 32, y * 32, 32, 32, g_pDibMapParts, 0, 0, byLevel, FALSE);
			}
			continue;
		}

		wMap = g_pMapInfo->pwMap[i + j * g_pMapInfo->nCX];
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

		if (pCharMyChar->byAdminLV == ADMINLV_USER) {
			if (pChar->byNpcType == NPCTYPE_PUT) {
				continue;
			}
		}

		nMapDark = 0;
		xTmp = dwMyX - pChar->dwX;
		yTmp = dwMyY - pChar->dwY;

		/* 暗闇表示？ */
		if (g_pMapInfo->byLightLevel) {
			nMapDark = g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) +
						(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp)];
			if (nMapDark <= 1) {
				continue;
			}
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
			if (g_pMapInfo->byLightLevel) {
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
		if (g_pMapInfo->byLightLevel) {
			if (g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) +
						(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp)] <= 1) {
				continue;
			}
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

	/* エフェクト表示 */
	for (i = 0; i < nCount; i ++) {
		BYTE byEfect;
		pChar = g_aCharInfo[i];
		if (pChar->dwTimeEfc == 0) {
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
		if (g_pMapInfo->byLightLevel) {
			if (g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) +
						(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp)] <= 1) {
				continue;
			}
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
		if (g_pMapInfo->byLightLevel) {
			if (g_pMapDarkTmp[(DRAW_PARTS_X + 1 - xTmp) +
						(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y + 1 - yTmp)] <= 1) {
				continue;
			}
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

				SboTextCopy (szTmp, sizeof (szTmp), &pChar->pszSpeech[j]);
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
		for (int j = 0; j < MAX_COUNTCHAREFC; j ++) {
			PCHAREFCINFO pCharEfc = &pChar->aCharEfcInfo[j];
			int nLen = SboTextWidth (pCharEfc->szMsg);
			if (nLen) {
				TextOut2 (hDC, nameX - nLen * 6 / 2 + GetCharCenterX (pChar),
						nameY - 8 + pCharEfc->chY,
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
		/* 現在値表示 */
		int nTmp = pChar->byTurnLV * 32 / 100;
		nTmp = max (nTmp, 1);
		g_pDibDrawMap->Blt (x + 3, y + 3, nTmp, 3, g_pDibSystem, 99, 142, TRUE);
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

	PMAPPARTSINFO pParts;
	PCHARINFO pMyChar = g_aCharInfo[g_nMyCharID];
	BYTE byLevel, byLight = pMyChar->byLightLevel;
	int i, j, x, y, mx, my, xMax;

	int nTmp = g_pMapInfo->byLightLevel - byLight;
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
			if (g_pMapInfo->byLightLevel) {
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
void AddSysMsg(LPSTR pszStr, COLORREF cl)
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

	int nSize = strlen (pszStr) + 1;
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
