/* ========================================================================= */
/* ファイル名：	LandSugoroku.h												 */
/* 概要：		周回型の土地取りすごろくのルール（サーバーが使う。通信・時計・キャラを知らない） */
/*				盤（28 マスの土地表）・お金・土地の持ち主・手番・サイコロの移動・	 */
/*				購入・通行料・周回ボーナス・脱落・勝敗だけ。					 */
/*				駒の移動・メッセージ・通信は SboSvr/src/SboSvrSubGameProcLand.cpp。	 */
/*				1 コマ＝マップ 3×3 マス。コマ番号 → マップ座標の対応もここ（LandCell*）。	 */
/*				単体テスト: tests/server/land_sugoroku_test.cpp				 */
/*				仕様・遊び方・データ投入手順は docs/land-sugoroku.md			 */
/* 日付：		2026/11/26（段階 1）										 */
/* ========================================================================= */

#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/* ========================================================================= */
/* 調整しやすい定数（遊びながら変える）										 */
/* ========================================================================= */
#define LAND_CELLS			(28)		/* 盤のコマ数（外周。1 辺 8 コマの正方形の縁） */
#define LAND_SIDE			(8)			/* 1 辺のコマ数（角を含む） */
#define LAND_CELL_TILES		(3)			/* 1 コマの 1 辺のマップのマス数（1 コマ＝3×3 マス）。盤全体は 24×24 マス */
#define LAND_MAX_PLAYERS	(4)			/* 遊べる人数の上限（段階 1 は 2 人で確かめる。ロジックは 4 人まで） */
#define LAND_START_GOLD		(1500)		/* 初めの所持金(G) */
#define LAND_LAP_BONUS		(200)		/* スタートを通る（止まる）たびの周回ボーナス(G) */
#define LAND_LAPS			(12)		/* この周回数に誰かが届くと、その周の全員の手番が終わった所で終了 */
#define LAND_TURN_SEC		(30)		/* 手番（サイコロ・購入の返事）の制限時間(秒)。過ぎると自動で進める */
#define LAND_BACK_STEPS		(3)			/* 迷いの森で戻るマス数 */
#define LAND_WALK_MS		(300)		/* 駒が地図の 1 マスを歩く間隔(ms)。クライアントの歩き（1 マス約 280ms）より少し長く、止まらず続けて歩いて見える */
#define LAND_STEP_MS		(LAND_WALK_MS * LAND_CELL_TILES)	/* 駒が 1 コマ進む（3 歩）のにかかる時間(ms) */
#define LAND_NAME_BUF		(40)		/* 土地名の欄のバイト数（UTF-8・終端含む） */
#define LAND_SHORT_BUF		(8)			/* 短い土地名の欄のバイト数（盤の上に重ねて描く。全角 2 文字・終端含む） */
#define LAND_REGIONS		(8)			/* 地方の数（土地 24 マス＝8 地方×3 町） */

/* コマの種類 */
enum {
	LANDCELL_START = 0,					/* スタート（周回ボーナス） */
	LANDCELL_FERRY,						/* 渡し舟小屋（1 回休み） */
	LANDCELL_TAVERN,					/* ギルド酒場（段階 1 は何もしない） */
	LANDCELL_FOREST,					/* 迷いの森（3 マス戻る） */
	LANDCELL_TOWN,						/* 土地（買える・通行料がかかる） */
	LANDCELL_KINDS
};

/* 手番の段階 */
enum {
	LANDPHASE_ROLL = 0,					/* サイコロ待ち（制限時間あり） */
	LANDPHASE_MOVE,						/* 駒が進んでいる（1 コマ歩き終えるたびに Step を呼ぶ） */
	LANDPHASE_BUY,						/* 空き地を買うかの返事待ち（制限時間あり） */
	LANDPHASE_OVER,						/* ゲーム終了 */
};

/* 終わった理由 */
enum {
	LANDOVER_NONE = 0,
	LANDOVER_LAST,						/* 1 人だけ残った */
	LANDOVER_LAPS,						/* 規定の周回数で終了（総資産で決める） */
	LANDOVER_EMPTY,						/* 誰もいなくなった */
};

/* TimeoutAction の返り値 */
enum {
	LANDTIMEOUT_NONE = 0,				/* まだ */
	LANDTIMEOUT_ROLL,					/* サイコロを自動で振る */
	LANDTIMEOUT_DECLINE,				/* 購入は「やめる」にする */
};

/* 脱落の理由 */
enum {
	LANDOUT_BANKRUPT = 0,				/* 通行料を払えない */
	LANDOUT_LEFT,						/* 切断・退出・マップ移動 */
};

/* 起きたこと（サーバーが読んでメッセージ・通信にする）。Events() に積まれ、ClearEvents で空にする */
enum {
	LANDEV_TURN = 0,					/* nPlayer の手番になった（サイコロ待ち） */
	LANDEV_SKIP,						/* nPlayer は休みで手番を飛ばされた */
	LANDEV_ROLL,						/* サイコロ。nValue=合計 nOther=(出目1*10+出目2) nCell=自動なら 1 */
	LANDEV_BONUS,						/* 周回ボーナス。nValue=金額 nOther=周回数 */
	LANDEV_ARRIVE,						/* 止まった。nCell=マス */
	LANDEV_OWN,							/* 自分の土地に止まった */
	LANDEV_ASK,							/* 空き地。買うか聞く。nCell=マス nValue=値段 */
	LANDEV_CANTBUY,						/* 空き地だがお金が足りない。nCell nValue=値段 */
	LANDEV_BOUGHT,						/* 買った。nCell nValue=値段 */
	LANDEV_DECLINE,						/* 買わなかった。nCell nOther=時間切れなら 1 */
	LANDEV_TOLL,						/* 通行料。nPlayer=払った人 nOther=受け取った人 nCell nValue=金額 */
	LANDEV_FERRY,						/* 渡し舟小屋で 1 回休みになった */
	LANDEV_TAVERN,						/* ギルド酒場（段階 1 は何も起きない） */
	LANDEV_FOREST,						/* 迷いの森。nValue=戻るマス数 nCell=戻った先のマス */
	LANDEV_OUT,							/* 脱落。nOther=LANDOUT_* nValue=銀行に戻った土地の数 */
	LANDEV_OVER,						/* 終了。nValue=LANDOVER_*。勝者は WinMask */
};

/* ========================================================================= */
/* 盤（28 マスの土地表）														 */
/* ========================================================================= */
struct LandCellDef {
	int			nKind;						/* LANDCELL_* */
	int			nRegion;					/* 地方（0〜7。土地だけ。色分け・「そろえる」用の下地） */
	int			nPrice;						/* 値段(G)（土地だけ） */
	int			nToll;						/* 通行料(G)（土地だけ） */
	char		szName[LAND_NAME_BUF];		/* 名前（UTF-8） */
	char		szShort[LAND_SHORT_BUF];	/* 盤の上に出す短い名前（全角 2 文字まで） */
};

/* コマ番号 → 盤の左上から見たコマの格子位置（横・縦。0〜LAND_SIDE-1）。
   0 が左上（スタート）、時計回りに 7=右上（渡し舟小屋）、14=右下（ギルド酒場）、21=左下（迷いの森） */
static inline void LandCellOffset(int nCell, int *pnDx, int *pnDy)
{
	int n = ((nCell % LAND_CELLS) + LAND_CELLS) % LAND_CELLS;
	int nLast = LAND_SIDE - 1;

	if (n <= nLast) {
		*pnDx = n;						/* 上の辺（左→右） */
		*pnDy = 0;
	} else if (n <= nLast * 2) {
		*pnDx = nLast;					/* 右の辺（上→下） */
		*pnDy = n - nLast;
	} else if (n <= nLast * 3) {
		*pnDx = nLast - (n - nLast * 2);	/* 下の辺（右→左） */
		*pnDy = nLast;
	} else {
		*pnDx = 0;						/* 左の辺（下→上） */
		*pnDy = nLast - (n - nLast * 3);
	}
}

/* コマ → 盤の左上から見たマップのマス位置。nSlot < 0 ならコマの中央（3×3 の真ん中。コマの代表位置）、
   0〜3 なら四隅（0=左上 1=右上 2=左下 3=右下。同じコマに複数人が止まる時の立ち位置） */
static inline void LandCellTile(int nCell, int nSlot, int *pnDx, int *pnDy)
{
	int gx, gy, nLast = LAND_CELL_TILES - 1;

	LandCellOffset (nCell, &gx, &gy);
	if (nSlot < 0) {
		*pnDx = gx * LAND_CELL_TILES + LAND_CELL_TILES / 2;
		*pnDy = gy * LAND_CELL_TILES + LAND_CELL_TILES / 2;
	} else {
		*pnDx = gx * LAND_CELL_TILES + ((nSlot & 1) ? nLast : 0);
		*pnDy = gy * LAND_CELL_TILES + ((nSlot & 2) ? nLast : 0);
	}
}

/* 四隅のコマ番号（スタート・渡し舟小屋・ギルド酒場・迷いの森） */
static inline int LandCornerKind(int nCell)
{
	switch (nCell) {
	case 0:						return LANDCELL_START;
	case LAND_SIDE - 1:			return LANDCELL_FERRY;
	case (LAND_SIDE - 1) * 2:	return LANDCELL_TAVERN;
	case (LAND_SIDE - 1) * 3:	return LANDCELL_FOREST;
	}
	return LANDCELL_TOWN;
}

/* 標準の土地表（町名は提案の地方表。ガルムーン 3 町・ロウェル周辺は仮名で、本番データで差し替える。
   地方ごとに値段が上がる。通行料は値段の 1/5）。盤の辺ごとに 2 地方ずつ、安い方から時計回りに並べる */
struct LandDefaultTown {
	const char *pszName;
	const char *pszShort;
	int nPrice;
};
static const LandDefaultTown g_aLandDefaultTown[LAND_REGIONS * 3] = {
	/* 1 始まりの大陸 */
	{ "ルシーの街",			"ルシ",	60 },	{ "サニアスの街",		"サニ",	60 },	{ "漁村バオ",			"バオ",	70 },
	/* 2 西フェルガナ */
	{ "港町レジェンタ",		"レジ",	90 },	{ "マイスの村",			"マイ",	100 },	{ "ブルアイの街",		"ブル",	110 },
	/* 3 山と里 */
	{ "山間の村ルノン",		"ルノ",	120 },	{ "風の棲む村ルナリア",	"ルナ",	130 },	{ "猫族の村プラムール",	"プラ",	140 },
	/* 4 北国 */
	{ "北の街サイア",		"サイ",	160 },	{ "リルシェの雪里",		"リル",	170 },	{ "ノースフレア",		"ノス",	180 },
	/* 5 ガルムーン（仮名） */
	{ "ガルムーン北町",		"ガ北",	200 },	{ "ガルムーン中町",		"ガ中",	210 },	{ "ガルムーン南町",		"ガ南",	220 },
	/* 6 水と空 */
	{ "水上都市マーメルド",	"マメ",	250 },	{ "蒼穹の都市フィレーン","フィ",	260 },	{ "自由都市ゼニー",		"ゼニ",	270 },
	/* 7 王家ゆかり */
	{ "ラスティーニ城",		"ラス",	300 },	{ "レオカーン城跡地",	"レオ",	320 },	{ "王都ウィンデル",		"ウィ",	340 },
	/* 8 ロウェル周辺（仮名） */
	{ "ロウェルの村",		"ロ村",	400 },	{ "ロウェル港",			"ロ港",	420 },	{ "ロウェル城下",		"ロ城",	450 },
};

struct LandBoard {
	LandCellDef	aCell[LAND_CELLS];

	/* 欄に収まる分だけコピー（終端つき） */
	static void LandCopy(char *pszDst, int nSize, const char *pszSrc)
	{
		strncpy (pszDst, pszSrc, nSize - 1);
		pszDst[nSize - 1] = 0;
		LandTrimUtf8 (pszDst);
	}

	/* 標準の土地表にする */
	void SetDefault()
	{
		int i, nTown = 0;

		memset (aCell, 0, sizeof (aCell));
		for (i = 0; i < LAND_CELLS; i ++) {
			LandCellDef &C = aCell[i];
			C.nKind = LandCornerKind (i);
			switch (C.nKind) {
			case LANDCELL_START:	strcpy (C.szName, "始まりの町");	strcpy (C.szShort, "出発");	break;
			case LANDCELL_FERRY:	strcpy (C.szName, "渡し舟小屋");	strcpy (C.szShort, "舟");	break;
			case LANDCELL_TAVERN:	strcpy (C.szName, "ギルド酒場");	strcpy (C.szShort, "酒場");	break;
			case LANDCELL_FOREST:	strcpy (C.szName, "迷いの森");		strcpy (C.szShort, "森");	break;
			default:
				C.nRegion = nTown / 3;
				C.nPrice = g_aLandDefaultTown[nTown].nPrice;
				C.nToll = C.nPrice / 5;
				LandCopy (C.szName, LAND_NAME_BUF, g_aLandDefaultTown[nTown].pszName);
				LandCopy (C.szShort, LAND_SHORT_BUF, g_aLandDefaultTown[nTown].pszShort);
				nTown ++;
				break;
			}
		}
	}

	/* 表が使えるか（四隅は決まった種類・土地は値段と通行料が正）。使えなければ理由を pszWhy に入れて false */
	bool Validate(std::string *pstrWhy = NULL) const
	{
		int i;
		char szTmp[96];

		for (i = 0; i < LAND_CELLS; i ++) {
			const LandCellDef &C = aCell[i];
			int nWant = LandCornerKind (i);

			if (C.nKind != nWant) {
				sprintf (szTmp, "マス %d の種類が違います（四隅以外は土地）", i);
				if (pstrWhy) *pstrWhy = szTmp;
				return false;
			}
			if ((nWant == LANDCELL_TOWN) && ((C.nPrice <= 0) || (C.nToll <= 0) || (C.nPrice > 60000) || (C.nToll > 60000))) {
				sprintf (szTmp, "マス %d の値段か通行料が 1〜60000 ではありません", i);
				if (pstrWhy) *pstrWhy = szTmp;
				return false;
			}
			if (C.szName[0] == 0) {
				sprintf (szTmp, "マス %d の名前がありません", i);
				if (pstrWhy) *pstrWhy = szTmp;
				return false;
			}
		}
		return true;
	}

	/* タブ区切りの表から読む。1 行 1 マス：マス番号 \t 種類 \t 地方 \t 名前 \t 短い名前 \t 値段 \t 通行料。
	   種類は start / ferry / tavern / forest / town。# で始まる行と空行は読み飛ばす。
	   ここに無いマスは標準のまま。形が違う行があれば false（読めたぶんは入る） */
	bool LoadTsv(const char *pszText, std::string *pstrWhy = NULL)
	{
		const char *p = pszText;
		int nLine = 0;
		bool bOk = true;

		while (*p) {
			std::string strLine;
			std::vector<std::string> aCol;
			size_t pos = 0, tab;
			int nCell, nKind, nRegion, nPrice, nToll;
			char szTmp[96];

			while (*p && (*p != '\n')) {
				if (*p != '\r') {
					strLine += *p;
				}
				p ++;
			}
			if (*p == '\n') {
				p ++;
			}
			nLine ++;
			if (strLine.empty () || (strLine[0] == '#')) {
				continue;
			}
			while ((tab = strLine.find ('\t', pos)) != std::string::npos) {
				aCol.push_back (strLine.substr (pos, tab - pos));
				pos = tab + 1;
			}
			aCol.push_back (strLine.substr (pos));
			if (aCol.size () < 7) {
				sprintf (szTmp, "%d 行目：列が足りません（7 列）", nLine);
				if (pstrWhy) *pstrWhy = szTmp;
				bOk = false;
				continue;
			}
			nCell = atoi (aCol[0].c_str ());
			if ((nCell < 0) || (nCell >= LAND_CELLS) || ((aCol[0][0] < '0') || (aCol[0][0] > '9'))) {
				sprintf (szTmp, "%d 行目：マス番号は 0〜%d", nLine, LAND_CELLS - 1);
				if (pstrWhy) *pstrWhy = szTmp;
				bOk = false;
				continue;
			}
			if (aCol[1] == "start")			nKind = LANDCELL_START;
			else if (aCol[1] == "ferry")	nKind = LANDCELL_FERRY;
			else if (aCol[1] == "tavern")	nKind = LANDCELL_TAVERN;
			else if (aCol[1] == "forest")	nKind = LANDCELL_FOREST;
			else if (aCol[1] == "town")		nKind = LANDCELL_TOWN;
			else {
				sprintf (szTmp, "%d 行目：種類は start/ferry/tavern/forest/town", nLine);
				if (pstrWhy) *pstrWhy = szTmp;
				bOk = false;
				continue;
			}
			nRegion = atoi (aCol[2].c_str ());
			nPrice = atoi (aCol[5].c_str ());
			nToll = atoi (aCol[6].c_str ());
			LandCellDef &C = aCell[nCell];
			C.nKind = nKind;
			C.nRegion = (nRegion < 0) ? 0 : ((nRegion >= LAND_REGIONS) ? LAND_REGIONS - 1 : nRegion);
			C.nPrice = nPrice;
			C.nToll = nToll;
			strncpy (C.szName, aCol[3].c_str (), LAND_NAME_BUF - 1);
			C.szName[LAND_NAME_BUF - 1] = 0;
			strncpy (C.szShort, aCol[4].c_str (), LAND_SHORT_BUF - 1);
			C.szShort[LAND_SHORT_BUF - 1] = 0;
			LandTrimUtf8 (C.szName);
			LandTrimUtf8 (C.szShort);
		}
		return bOk;
	}

	/* バッファの長さで切れた UTF-8 の最後の欠けた文字を落とす */
	static void LandTrimUtf8(char *psz)
	{
		int n = (int)strlen (psz), i = n - 1, nLead = 0;

		while ((i >= 0) && (((unsigned char)psz[i] & 0xC0) == 0x80)) {
			i --;
			nLead ++;
		}
		if (i >= 0) {
			unsigned char c = (unsigned char)psz[i];
			int nNeed = (c >= 0xF0) ? 3 : ((c >= 0xE0) ? 2 : ((c >= 0xC0) ? 1 : 0));
			if (nNeed > nLead) {
				psz[i] = 0;
			}
		}
	}
};

/* ========================================================================= */
/* 遊んでいる状態															 */
/* ========================================================================= */
struct LandPlayer {
	int		nPos;				/* いるマス（0〜27） */
	int		nGold;				/* 所持金(G) */
	int		nLaps;				/* 回った周回数（スタートを通る・止まるたびに +1） */
	bool	bAlive;				/* 遊んでいるか（脱落したら false） */
	int		nSkip;				/* 残りの休み回数（次の手番を飛ばす数） */
};

struct LandEvent {
	int		nType;				/* LANDEV_* */
	int		nPlayer;			/* 主な人 */
	int		nCell;
	int		nValue;
	int		nOther;
};

class LandGame {
public:
	LandBoard				Board;
	LandPlayer				aPlayer[LAND_MAX_PLAYERS];
	int						nPlayers;				/* 参加者数（脱落者も数える。番号は最後まで変わらない） */
	signed char				anOwner[LAND_CELLS];	/* 土地の持ち主（-1=銀行・空き地） */
	int						nTurn;					/* 手番の人 */
	int						nPhase;					/* LANDPHASE_* */
	int						nStepsLeft;				/* 残りの歩数 */
	int						nDice1, nDice2;			/* 最後の出目 */
	int						nAskCell;				/* 購入の返事待ちのマス */
	unsigned int			dwDeadline;				/* 今の返事待ちの制限時刻(ms)。ROLL・BUY の間だけ意味がある */
	int						nOverReason;			/* LANDOVER_* */
	int						nWinMask;				/* 勝者（ビット = 参加者番号）。同点は複数 */
	int						nMaxLaps;				/* 終了する周回数 */
	int						nRound;					/* 何周目の手番か（nTurn が先頭に戻るたびに +1） */
	std::vector<LandEvent>	aEvent;

	LandGame() { Start (0, 0); }

	/* 始める。n 人（1〜4）。時刻 dwNow(ms)。土地表は Board（先に SetDefault か LoadTsv で用意しておく） */
	void Start(int n, unsigned int dwNow)
	{
		int i;

		nPlayers = (n < 0) ? 0 : ((n > LAND_MAX_PLAYERS) ? LAND_MAX_PLAYERS : n);
		memset (aPlayer, 0, sizeof (aPlayer));
		for (i = 0; i < nPlayers; i ++) {
			aPlayer[i].nGold = LAND_START_GOLD;
			aPlayer[i].bAlive = true;
		}
		memset (anOwner, 0xFF, sizeof (anOwner));
		nTurn = 0;
		nPhase = LANDPHASE_ROLL;
		nStepsLeft = 0;
		nDice1 = nDice2 = 0;
		nAskCell = -1;
		dwDeadline = 0;
		nOverReason = LANDOVER_NONE;
		nWinMask = 0;
		nMaxLaps = LAND_LAPS;
		nRound = 0;
		aEvent.clear ();
		if (nPlayers == 0) {
			nPhase = LANDPHASE_OVER;
			nOverReason = LANDOVER_EMPTY;
			return;
		}
		if (nPlayers == 1) {
			Finish (LANDOVER_LAST);
			return;
		}
		BeginTurn (dwNow);
	}

	void ClearEvents() { aEvent.clear (); }
	bool IsOver() const { return nPhase == LANDPHASE_OVER; }

	int AliveCount() const
	{
		int i, n = 0;
		for (i = 0; i < nPlayers; i ++) {
			if (aPlayer[i].bAlive) n ++;
		}
		return n;
	}

	/* 総資産 = 現金 + 持っている土地の値段（脱落者は 0） */
	int Assets(int p) const
	{
		int i, n;

		if ((p < 0) || (p >= nPlayers) || !aPlayer[p].bAlive) {
			return 0;
		}
		n = aPlayer[p].nGold;
		for (i = 0; i < LAND_CELLS; i ++) {
			if (anOwner[i] == p) {
				n += Board.aCell[i].nPrice;
			}
		}
		return n;
	}

	int OwnedCount(int p) const
	{
		int i, n = 0;
		for (i = 0; i < LAND_CELLS; i ++) {
			if (anOwner[i] == p) n ++;
		}
		return n;
	}

	/* 制限時間が来ていたら、機械が代わりに何をするか（LANDTIMEOUT_*）。呼ぶだけでは状態は変わらない */
	int TimeoutAction(unsigned int dwNow) const
	{
		if (((nPhase != LANDPHASE_ROLL) && (nPhase != LANDPHASE_BUY)) || ((int)(dwNow - dwDeadline) < 0)) {
			return LANDTIMEOUT_NONE;
		}
		return (nPhase == LANDPHASE_ROLL) ? LANDTIMEOUT_ROLL : LANDTIMEOUT_DECLINE;
	}

	/* 制限時間までの残り秒（切り上げ。待ちでない時は 0） */
	int SecondsLeft(unsigned int dwNow) const
	{
		int nMs;

		if ((nPhase != LANDPHASE_ROLL) && (nPhase != LANDPHASE_BUY)) {
			return 0;
		}
		nMs = (int)(dwDeadline - dwNow);
		return (nMs <= 0) ? 0 : (int)((nMs + 999) / 1000);
	}

	/* サイコロ（出目 d1・d2 は 1〜6）。手番の人 p がサイコロ待ちの時だけ。bAuto は時間切れの自動。進める歩数は d1+d2 */
	bool Roll(int p, int d1, int d2, bool bAuto = false)
	{
		if ((nPhase != LANDPHASE_ROLL) || (p != nTurn) || (d1 < 1) || (d1 > 6) || (d2 < 1) || (d2 > 6)) {
			return false;
		}
		nDice1 = d1;
		nDice2 = d2;
		nStepsLeft = d1 + d2;
		nPhase = LANDPHASE_MOVE;
		Push (LANDEV_ROLL, p, bAuto ? 1 : 0, d1 + d2, d1 * 10 + d2);
		return true;
	}

	/* 駒を 1 コマ進める（LANDPHASE_MOVE の間、駒が 1 コマ（マップ 3 歩）歩き終えるたびに）。スタートを通る（止まる）と周回ボーナス。
	   最後の 1 マスなら、そのマスの処理（購入の問い合わせ・通行料・四隅の効果）まで済ませる */
	bool Step(unsigned int dwNow)
	{
		LandPlayer &P = aPlayer[nTurn];

		if (nPhase != LANDPHASE_MOVE) {
			return false;
		}
		P.nPos = (P.nPos + 1) % LAND_CELLS;
		if (P.nPos == 0) {
			P.nLaps ++;
			P.nGold += LAND_LAP_BONUS;
			Push (LANDEV_BONUS, nTurn, 0, LAND_LAP_BONUS, P.nLaps);
		}
		nStepsLeft --;
		if (nStepsLeft <= 0) {
			Arrive (dwNow);
		}
		return true;
	}

	/* 購入の返事（bYes=買う）。購入待ちの手番の人 p だけ。お金が足りなければ買えず「やめる」扱い */
	bool Buy(int p, bool bYes, unsigned int dwNow, bool bTimeout = false)
	{
		const LandCellDef *pC;

		if ((nPhase != LANDPHASE_BUY) || (p != nTurn) || (nAskCell < 0)) {
			return false;
		}
		pC = &Board.aCell[nAskCell];
		if (bYes && (aPlayer[p].nGold >= pC->nPrice)) {
			aPlayer[p].nGold -= pC->nPrice;
			anOwner[nAskCell] = (signed char)p;
			Push (LANDEV_BOUGHT, p, nAskCell, pC->nPrice, 0);
		} else {
			Push (LANDEV_DECLINE, p, nAskCell, pC->nPrice, bTimeout ? 1 : 0);
		}
		nAskCell = -1;
		EndTurn (dwNow);
		return true;
	}

	/* 脱落させる（通行料を払えない・切断・退出・マップ移動）。土地は銀行へ。
	   手番の人なら手番を終え、残りが 1 人ならその人の勝ち */
	void Eliminate(int p, int nReason, unsigned int dwNow)
	{
		int i, nLost = 0;
		bool bTurn;

		if ((p < 0) || (p >= nPlayers) || !aPlayer[p].bAlive || (nPhase == LANDPHASE_OVER)) {
			return;
		}
		for (i = 0; i < LAND_CELLS; i ++) {
			if (anOwner[i] == p) {
				anOwner[i] = -1;
				nLost ++;
			}
		}
		aPlayer[p].bAlive = false;
		aPlayer[p].nGold = 0;
		aPlayer[p].nSkip = 0;
		Push (LANDEV_OUT, p, 0, nLost, nReason);
		bTurn = (p == nTurn);
		if (AliveCount () <= 1) {
			Finish ((AliveCount () == 1) ? LANDOVER_LAST : LANDOVER_EMPTY);
			return;
		}
		if (bTurn) {
			nStepsLeft = 0;
			nAskCell = -1;
			EndTurn (dwNow);
		}
	}

private:
	void Push(int nType, int nPlayer, int nCell, int nValue, int nOther)
	{
		LandEvent E;
		E.nType = nType;
		E.nPlayer = nPlayer;
		E.nCell = nCell;
		E.nValue = nValue;
		E.nOther = nOther;
		aEvent.push_back (E);
	}

	/* 手番の人の番を始める（休みなら飛ばして次の人へ）。サイコロ待ちにして制限時間を決める */
	void BeginTurn(unsigned int dwNow)
	{
		int nGuard = 0;

		while (nPhase != LANDPHASE_OVER) {
			if (aPlayer[nTurn].bAlive && (aPlayer[nTurn].nSkip <= 0)) {
				break;
			}
			if (aPlayer[nTurn].bAlive) {
				aPlayer[nTurn].nSkip --;
				Push (LANDEV_SKIP, nTurn, 0, 0, 0);
			}
			if (!Advance ()) {
				return;
			}
			/* 全員が休みでも止まらないよう、念のため */
			if (++ nGuard > LAND_MAX_PLAYERS * 4) {
				break;
			}
		}
		nPhase = LANDPHASE_ROLL;
		dwDeadline = dwNow + LAND_TURN_SEC * 1000U;
		Push (LANDEV_TURN, nTurn, 0, 0, 0);
	}

	/* 次の生きている人へ。先頭に戻る時に周回の終了を調べる。終了したら false */
	bool Advance()
	{
		int i, nNext = nTurn;

		for (i = 0; i < nPlayers; i ++) {
			nNext = (nNext + 1) % nPlayers;
			if (nNext <= nTurn) {
				nRound ++;
				if (LapsReached ()) {
					Finish (LANDOVER_LAPS);
					return false;
				}
			}
			nTurn = nNext;
			if (aPlayer[nTurn].bAlive) {
				return true;
			}
		}
		return true;
	}

	bool LapsReached() const
	{
		int i;
		for (i = 0; i < nPlayers; i ++) {
			if (aPlayer[i].bAlive && (aPlayer[i].nLaps >= nMaxLaps)) {
				return true;
			}
		}
		return false;
	}

	void EndTurn(unsigned int dwNow)
	{
		if (nPhase == LANDPHASE_OVER) {
			return;
		}
		nPhase = LANDPHASE_ROLL;
		if (Advance ()) {
			BeginTurn (dwNow);
		}
	}

	/* 止まったマスの処理 */
	void Arrive(unsigned int dwNow)
	{
		LandPlayer &P = aPlayer[nTurn];
		const LandCellDef &C = Board.aCell[P.nPos];
		int nOwner;

		Push (LANDEV_ARRIVE, nTurn, P.nPos, 0, 0);
		switch (C.nKind) {
		case LANDCELL_FERRY:
			P.nSkip ++;
			Push (LANDEV_FERRY, nTurn, P.nPos, 0, 0);
			break;
		case LANDCELL_TAVERN:
			Push (LANDEV_TAVERN, nTurn, P.nPos, 0, 0);
			break;
		case LANDCELL_FOREST:
			/* 後ろへ戻る。戻る途中でスタートは通らない（ボーナスなし）。戻った先の効果は起きない */
			P.nPos = ((P.nPos - LAND_BACK_STEPS) % LAND_CELLS + LAND_CELLS) % LAND_CELLS;
			Push (LANDEV_FOREST, nTurn, P.nPos, LAND_BACK_STEPS, 0);
			break;
		case LANDCELL_TOWN:
			nOwner = anOwner[P.nPos];
			if (nOwner < 0) {
				if (P.nGold >= C.nPrice) {
					/* 空き地：買うか聞く（制限時間つき） */
					nAskCell = P.nPos;
					nPhase = LANDPHASE_BUY;
					dwDeadline = dwNow + LAND_TURN_SEC * 1000U;
					Push (LANDEV_ASK, nTurn, P.nPos, C.nPrice, 0);
					return;
				}
				Push (LANDEV_CANTBUY, nTurn, P.nPos, C.nPrice, 0);
			} else if (nOwner == nTurn) {
				Push (LANDEV_OWN, nTurn, P.nPos, 0, 0);
			} else if (P.nGold >= C.nToll) {
				P.nGold -= C.nToll;
				aPlayer[nOwner].nGold += C.nToll;
				Push (LANDEV_TOLL, nTurn, P.nPos, C.nToll, nOwner);
			} else {
				/* 払えない：脱落（土地は銀行へ）。残りの人数で勝敗まで進める */
				Push (LANDEV_TOLL, nTurn, P.nPos, 0, nOwner);
				Eliminate (nTurn, LANDOUT_BANKRUPT, dwNow);
				return;
			}
			break;
		default:
			break;
		}
		EndTurn (dwNow);
	}

	/* 終了：勝者を決める（1 人残りならその人。周回終了なら総資産が最大の人。同点は全員） */
	void Finish(int nReason)
	{
		int i, nBest = -1;

		nPhase = LANDPHASE_OVER;
		nOverReason = nReason;
		nWinMask = 0;
		if (nReason == LANDOVER_LAST) {
			for (i = 0; i < nPlayers; i ++) {
				if (aPlayer[i].bAlive) nWinMask |= (1 << i);
			}
		} else if (nReason == LANDOVER_LAPS) {
			for (i = 0; i < nPlayers; i ++) {
				if (aPlayer[i].bAlive && (Assets (i) > nBest)) nBest = Assets (i);
			}
			for (i = 0; i < nPlayers; i ++) {
				if (aPlayer[i].bAlive && (Assets (i) == nBest)) nWinMask |= (1 << i);
			}
		}
		Push (LANDEV_OVER, -1, 0, nReason, nWinMask);
	}
};
