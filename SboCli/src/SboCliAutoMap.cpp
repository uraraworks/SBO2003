/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	SboCliAutoMap.cpp											 */
/* 内容：		探索マップ（オートマップ）									 */
/* 備考：		テーマで作った気まぐれダンジョンのマップ範囲だけで使う。	 */
/*				サーバー（CMD_DUNGEONMAP）が部屋・出入口の一覧を送る。		 */
/*				記録（開けた部屋・見えたマス）はクライアントの中だけで持ち、 */
/*				サーバーへは何も送らない・保存もしない。					 */
/*				消す時: 作り直し番号が違う・ダンジョンの範囲から出た・		 */
/*				別のマップへ移った・ログイン画面へ戻った。					 */
/*				記録の作り方は SboCliAutoMapCore.h						 */
/* ========================================================================= */

#include "stdafx.h"
#include "Glbldata.h"
#include "SboCliRecvCmd.h"
#include "SboCliAutoMap.h"
#include "SboCliAutoMapCore.h"

extern BOOL CheckCharBlock (DWORD, DWORD);						/* キャラのぶつかり判定（TRUE=通れる。SboCliRecvCmd.cpp） */

static AutoMapCore::Map s_Map;									/* サーバーからの情報と記録 */
static BOOL s_bShown = FALSE;									/* 地図を出している */
static BOOL s_bInRange = FALSE;									/* 自分がダンジョンの範囲の中にいる */
static std::vector<AutoMapCore::Seg> s_aSeg;					/* 壁の線 */

/* 歩けるマスか（マップのパーツのぶつかり判定） */
struct AutoMapWalk {
	bool operator () (int x, int y) const
	{
		return CheckCharBlock ((DWORD)x, (DWORD)y) ? true : false;
	}
};

/* ========================================================================= */
/* 関数名：AutoMapReset														 */
/* 内容：全部消す															 */
/* ========================================================================= */
void AutoMapReset(void)
{
	s_Map.Clear ();
	s_aSeg.clear ();
	s_bShown = FALSE;
	s_bInRange = FALSE;
	g_bDraw = TRUE;
}

/* ========================================================================= */
/* 関数名：AutoMapOnMapInfo													 */
/* 内容：マップ情報を受け取った（別のマップなら、覚えている地図を消す）		 */
/* ========================================================================= */
void AutoMapOnMapInfo(DWORD dwMapID)
{
	if (s_Map.IsValid () && (s_Map.GetMapID () != dwMapID)) {
		AutoMapReset ();
	}
}

/* ========================================================================= */
/* 関数名：RecvDataProcDUNGEONMAP											 */
/* 内容：受信データ処理(探索マップ用の情報)									 */
/* 備考：マップ情報の直後と、ダンジョンを作り直した直後に届く。				 */
/*       作り直し番号が覚えているものと違えば、Set が記録を全部消す			 */
/* ========================================================================= */
void RecvDataProcDUNGEONMAP(PBYTE pData, DWORD dwDataSize)
{
	PCMDPARA_DUNGEONMAP pPara = (PCMDPARA_DUNGEONMAP)pData;

	if (dwDataSize < sizeof (CMDPARA_DUNGEONMAP)) {
		return;
	}
	if (!pPara->byValid) {
		AutoMapReset ();
		return;
	}
	DWORD dwRoom = pPara->wRoomCount, dwLink = pPara->wLinkCount;
	if ((DWORD)(dwRoom * sizeof (DUNGEONMAPROOM) + dwLink * sizeof (DUNGEONMAPLINK)) > dwDataSize - sizeof (CMDPARA_DUNGEONMAP)) {
		return;
	}
	std::vector<AutoMapCore::Room> aRoom (dwRoom);
	std::vector<AutoMapCore::Link> aLink (dwLink);
	PDUNGEONMAPROOM pRoom = (PDUNGEONMAPROOM)(pPara + 1);
	PDUNGEONMAPLINK pLink = (PDUNGEONMAPLINK)(pRoom + dwRoom);
	DWORD i;
	for (i = 0; i < dwRoom; i ++) {
		aRoom[i].x = pRoom[i].wX;
		aRoom[i].y = pRoom[i].wY;
		aRoom[i].w = pRoom[i].wW;
		aRoom[i].h = pRoom[i].wH;
		aRoom[i].group = pRoom[i].wGroup;
	}
	for (i = 0; i < dwLink; i ++) {
		aLink[i].a = pLink[i].wRoomA;
		aLink[i].b = pLink[i].wRoomB;
		aLink[i].x = pLink[i].wX;
		aLink[i].y = pLink[i].wY;
		aLink[i].w = pLink[i].wW;
		aLink[i].h = pLink[i].wH;
		aLink[i].type = pLink[i].byType;
	}
	s_Map.Set (pPara->dwMapID, pPara->dwBuildNo, pPara->wX1, pPara->wY1, pPara->wX2, pPara->wY2, aRoom, aLink);
	s_aSeg.clear ();
	g_bDraw = TRUE;
	/* 今いる場所の部屋・見えたマスをすぐ記録する */
	AutoMapUpdate ();
}

/* ========================================================================= */
/* 関数名：AutoMapUpdate													 */
/* 内容：自分の位置・見えたマスを記録する									 */
/* 備考：暗いマップは灯りで見えたマス(g_pMapDarkTmp > 1)、					 */
/*       暗くないマップは画面に見えている範囲								 */
/* ========================================================================= */
void AutoMapUpdate(void)
{
	PCHARINFO pChar;
	int x, y, mx, my, nWidth;

	if (!s_Map.IsValid () || (g_pMapInfo == NULL) || (g_nMyCharID < 0) || (g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}
	/* 別のマップへ移った */
	if (g_pMapInfo->dwID != s_Map.GetMapID ()) {
		AutoMapReset ();
		return;
	}
	pChar = g_aCharInfo[g_nMyCharID];
	/* ダンジョンの範囲から出た（出口のワープ・死んで戻された時など）。範囲に入り直したら最初から */
	if (!s_Map.InRange ((int)pChar->dwX, (int)pChar->dwY)) {
		if (s_bInRange || s_bShown) {
			s_Map.ClearRecord ();
			s_aSeg.clear ();
			s_bShown = FALSE;
			g_bDraw = TRUE;
		}
		s_bInRange = FALSE;
		return;
	}
	s_bInRange = TRUE;

	s_Map.Visit ((int)pChar->dwX, (int)pChar->dwY);

	nWidth = DRAW_PARTS_X * 2 + 3;
	if (IsMapViewMode () || GetMapLightLevel () || IsMapDarkFlg ()) {
		/* 灯りが届いて見えたマス */
		for (y = 0; y < DRAW_PARTS_Y * 2 + 3; y ++) {
			my = (int)pChar->dwY + y - (1 + DRAW_PARTS_Y);
			for (x = 0; x < nWidth; x ++) {
				mx = (int)pChar->dwX + x - (1 + DRAW_PARTS_X);
				if (g_pMapDarkTmp[x + nWidth * y] > 1) {
					s_Map.Seen (mx, my);
				}
			}
		}
	} else {
		/* 暗くないマップは、画面に見えている範囲 */
		for (y = -DRAW_PARTS_Y; y <= DRAW_PARTS_Y; y ++) {
			for (x = -DRAW_PARTS_X; x <= DRAW_PARTS_X; x ++) {
				s_Map.Seen ((int)pChar->dwX + x, (int)pChar->dwY + y);
			}
		}
	}
	if (s_bShown && s_Map.IsDirty ()) {
		g_bDraw = TRUE;
	}
}

/* ========================================================================= */
/* 関数名：AutoMapIsAvailable												 */
/* 内容：今、探索マップが使える場所か										 */
/* ========================================================================= */
BOOL AutoMapIsAvailable(void)
{
	return (s_Map.IsValid () && s_bInRange) ? TRUE : FALSE;
}

BOOL AutoMapIsShown(void)
{
	return (AutoMapIsAvailable () && s_bShown) ? TRUE : FALSE;
}

/* ========================================================================= */
/* 関数名：AutoMapToggleKey													 */
/* 内容：Tab・M キー：使える場所なら地図を出し入れして TRUE					 */
/*       使えない場所では何もせず FALSE（Tab は今までの表示切り替えに回る）	 */
/* ========================================================================= */
BOOL AutoMapToggleKey(void)
{
	if (!AutoMapIsAvailable ()) {
		return FALSE;
	}
	s_bShown = !s_bShown;
	g_bDraw = TRUE;
	return TRUE;
}

/* 半透明で 1 ドット混ぜる（pBits は下から上の 32bit。0x00RRGGBB） */
static void BlendPixel(LPBYTE pBits, int nW, int nH, int x, int y, int r, int g, int b, int a)
{
	PDWORD pDst;
	DWORD dw;
	int nr, ng, nb;

	if ((x < 0) || (y < 0) || (x >= nW) || (y >= nH)) {
		return;
	}
	pDst = (PDWORD)(pBits + (nW * 4) * (nH - 1 - y) + x * 4);
	dw = *pDst;
	nr = (int)((dw >> 16) & 0xFF);
	ng = (int)((dw >> 8) & 0xFF);
	nb = (int)(dw & 0xFF);
	nr = (nr * (255 - a) + r * a) / 255;
	ng = (ng * (255 - a) + g * a) / 255;
	nb = (nb * (255 - a) + b * a) / 255;
	*pDst = ((DWORD)nr << 16) | ((DWORD)ng << 8) | (DWORD)nb;
}

/* 半透明の四角（画面の範囲 nL,nT〜nR,nB の外は描かない） */
static void BlendRect(LPBYTE pBits, int nW, int nH, int nL, int nT, int nR, int nB,
					  int x, int y, int cx, int cy, int r, int g, int b, int a)
{
	int i, j, x2 = min (x + cx, nR), y2 = min (y + cy, nB);

	for (j = max (y, nT); j < y2; j ++) {
		for (i = max (x, nL); i < x2; i ++) {
			BlendPixel (pBits, nW, nH, i, j, r, g, b, a);
		}
	}
}

/* ========================================================================= */
/* 関数名：AutoMapDraw														 */
/* 内容：地図を描く															 */
/* 備考：画面の真ん中に、ダンジョンの範囲が画面の約 75% に収まる倍率で、	 */
/*       半透明の線で重ねる。壁は線・アーチは緑・扉は橙・自分は赤い点		 */
/*       (nX, nY) は画面の左上のドット位置（g_pDibDrawMap の中）			 */
/* ========================================================================= */
void AutoMapDraw(int nX, int nY)
{
	LPBYTE pBits;
	PCHARINFO pChar;
	int nW, nH, i, nCX, nCY, nOX, nOY, nThick;
	int nL = nX, nT = nY, nR = nX + SCRSIZEX, nB = nY + SCRSIZEY;
	float fScale;

	if (!AutoMapIsShown () || (g_nMyCharID < 0) || (g_pDibDrawMap == NULL)) {
		return;
	}
	pBits = g_pDibDrawMap->GetBits ();
	if (pBits == NULL) {
		return;
	}
	nW = g_pDibDrawMap->Width ();
	nH = g_pDibDrawMap->Height ();
	nCX = s_Map.GetW ();
	nCY = s_Map.GetH ();
	fScale = (float)(min (SCRSIZEX, SCRSIZEY) * 3 / 4) / (float)max (nCX, nCY);
	nOX = nX + (SCRSIZEX - (int)(nCX * fScale + 0.5f)) / 2;
	nOY = nY + (SCRSIZEY - (int)(nCY * fScale + 0.5f)) / 2;
	nThick = (fScale >= 6.0f) ? 2 : 1;

	/* 背景を少し暗くして、線を見やすくする */
	BlendRect (pBits, nW, nH, nL, nT, nR, nB, nL, nT, SCRSIZEX, SCRSIZEY, 0, 0, 0, 110);

	/* 扉・差し替えで歩けるかが変わることがあるので、描くたびに作り直す（最大 128×128 マス） */
	s_Map.BuildSegments (AutoMapWalk (), s_aSeg);

	/* 出入口（アーチ・扉）。壁ごと抜いた所は広間なので印は出さない */
	for (i = 0; i < (int)s_Map.GetLinkCount (); i ++) {
		const AutoMapCore::Link &lk = s_Map.GetLink (i);
		int r, g, b;

		if (!s_Map.IsLinkShown (i) || (lk.type == 3)) {
			continue;
		}
		if (lk.type == 2) {
			r = 255; g = 160; b = 40;
		} else {
			r = 80; g = 230; b = 130;
		}
		int x1 = nOX + (int)((lk.x - s_Map.GetX1 ()) * fScale + 0.5f);
		int y1 = nOY + (int)((lk.y - s_Map.GetY1 ()) * fScale + 0.5f);
		int x2 = nOX + (int)((lk.x + lk.w - s_Map.GetX1 ()) * fScale + 0.5f);
		int y2 = nOY + (int)((lk.y + lk.h - s_Map.GetY1 ()) * fScale + 0.5f);
		BlendRect (pBits, nW, nH, nL, nT, nR, nB, x1, y1, max (x2 - x1, 2), max (y2 - y1, 2), r, g, b, 200);
	}

	/* 壁の線 */
	for (i = 0; i < (int)s_aSeg.size (); i ++) {
		const AutoMapCore::Seg &s = s_aSeg[i];
		int x1 = nOX + (int)(s.x1 * fScale + 0.5f);
		int y1 = nOY + (int)(s.y1 * fScale + 0.5f);
		int x2 = nOX + (int)(s.x2 * fScale + 0.5f);
		int y2 = nOY + (int)(s.y2 * fScale + 0.5f);
		if (s.y1 == s.y2) {
			BlendRect (pBits, nW, nH, nL, nT, nR, nB, x1, y1 - nThick / 2, max (x2 - x1, 1), nThick, 210, 225, 255, 200);
		} else {
			BlendRect (pBits, nW, nH, nL, nT, nR, nB, x1 - nThick / 2, y1, nThick, max (y2 - y1, 1), 210, 225, 255, 200);
		}
	}

	/* 自分の位置 */
	pChar = g_aCharInfo[g_nMyCharID];
	{
		int x1 = nOX + (int)(((int)pChar->dwX - s_Map.GetX1 () + 0.5f) * fScale);
		int y1 = nOY + (int)(((int)pChar->dwY - s_Map.GetY1 () + 0.5f) * fScale);
		int nR2 = max (2, (int)(fScale * 0.45f));
		BlendRect (pBits, nW, nH, nL, nT, nR, nB, x1 - nR2 - 1, y1 - nR2 - 1, nR2 * 2 + 3, nR2 * 2 + 3, 255, 255, 255, 230);
		BlendRect (pBits, nW, nH, nL, nT, nR, nB, x1 - nR2, y1 - nR2, nR2 * 2 + 1, nR2 * 2 + 1, 255, 50, 50, 255);
	}
}
