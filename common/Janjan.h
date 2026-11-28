/* ========================================================================= */
/* ファイル名：	Janjan.h													 */
/* 概要：		SBOじゃん（ドンジャラ風の絵合わせ）の盤面ロジック（サーバーが使う。通信・時計・描画を知らない） */
/*				牌・山・配牌・引く／捨てる・上がり判定・ロン・鳴き・流局・得点・CPU の打ち方だけ。 */
/*				単体テスト: tests/server/janjan_test.cpp					 */
/* 日付：		2026/11/26													 */
/* ========================================================================= */

#pragma once

#include <string.h>
#include <stdint.h>

/* ========================================================================= */
/* 調整しやすい定数（遊びながら変える）										 */
/* ========================================================================= */
#define JANJAN_KINDS		(16)		/* 牌の絵の種類（id 0〜15） */
#define JANJAN_COPIES		(4)			/* 同じ絵の枚数 */
#define JANJAN_GROUP_SIZE	(4)			/* 1 つの組の絵の数（id / この値 が組の番号） */
#define JANJAN_HAND			(8)			/* 配る枚数（引いた直後は 1 枚増える） */
#define JANJAN_SEATS		(4)			/* 席の数 */
#define JANJAN_TILES		(JANJAN_KINDS * JANJAN_COPIES)	/* 牌の総数（64） */
#define JANJAN_SET_SIZE		(3)			/* 1 つの組（3 枚組）の枚数 */
#define JANJAN_MELD_MAX		(2)			/* 鳴ける回数の上限（隠した牌を 1 枚は捨てられるように 2 まで） */

/* 手番の段階 */
#define JANJAN_PHASE_DRAW		(0)		/* turn の席が山から引く番（直前の捨て牌にロン・鳴きができる） */
#define JANJAN_PHASE_DISCARD	(1)		/* turn の席が 1 枚捨てる番（引いた後、または鳴いた後） */
#define JANJAN_PHASE_END		(2)		/* 局が終わった（nResult を見る） */

/* 局の結果 */
#define JANJAN_RESULT_NONE		(0)		/* 続行中 */
#define JANJAN_RESULT_WIN		(1)		/* 上がり（nWinner・nWinHow） */
#define JANJAN_RESULT_DRAW		(2)		/* 流局（山が尽きた） */

/* 上がり方 */
#define JANJAN_HOW_TSUMO		(0)		/* ツモ（自分の引いた牌） */
#define JANJAN_HOW_RON			(1)		/* ロン（他人の捨て牌） */

/* 鳴きの種類 */
#define JANJAN_MELD_SOROI		(0)		/* そろい（同じ絵 3 枚。ポン相当） */
#define JANJAN_MELD_GUMI		(1)		/* 組そろい（同じ組の違う絵 3 枚。チー相当） */

/* 捨て牌に対する申し出（強いほど優先: ロン > そろい > 組そろい） */
#define JANJAN_CLAIM_NONE		(0)
#define JANJAN_CLAIM_GUMI		(1)
#define JANJAN_CLAIM_SOROI		(2)
#define JANJAN_CLAIM_RON		(3)

#define JANJAN_WIN_POINT		(1)		/* 段階 1 の得点（上がった人に 1 点） */

/* 鳴いた組（公開された完成済みの 3 枚組） */
struct JanjanMeld {
	int nKind;						/* JANJAN_MELD_* */
	int aTile[JANJAN_SET_SIZE];		/* 牌の id（昇順） */
	int nFrom;						/* 牌をもらった席 */
};

/* 1 局の状態 */
struct JanjanGame {
	int aPile[JANJAN_TILES];		/* 山（先頭から引く） */
	int nPileTop;					/* 次に引く位置（山の残りは JANJAN_TILES - nPileTop） */
	uint32_t dwRng;					/* 乱数の状態（山と CPU のランダムで進める） */
	int aHand[JANJAN_SEATS][JANJAN_HAND + 1];	/* 隠した手牌（id の昇順） */
	int anHand[JANJAN_SEATS];
	JanjanMeld aMeld[JANJAN_SEATS][JANJAN_MELD_MAX];
	int anMeld[JANJAN_SEATS];
	int aRiver[JANJAN_SEATS][JANJAN_TILES];	/* 席ごとの捨て牌の列（川。鳴かれた牌は抜ける） */
	int anRiver[JANJAN_SEATS];
	int nTurn;						/* 手番の席 */
	int nPhase;						/* JANJAN_PHASE_* */
	int bDrew;						/* 今の手番で山から引いたか（ツモ上がりは引いた時だけ） */
	int bLastLive;					/* 直前の捨て牌がまだロン・鳴きの対象か */
	int nLastSeat;					/* 直前に捨てた席 */
	int nLastTile;					/* 直前に捨てた牌 */
	int nResult;					/* JANJAN_RESULT_* */
	int nWinner;					/* 上がった席（-1=なし） */
	int nWinHow;					/* JANJAN_HOW_* */
	int anScore[JANJAN_SEATS];		/* 得点 */
};

/* ========================================================================= */
/* 乱数（自前の線形合同法。同じ種なら同じ並び）								 */
/* ========================================================================= */
static inline int JanjanRand (JanjanGame &g, int n)
{
	g.dwRng = g.dwRng * 1664525u + 1013904223u;
	return (int)((g.dwRng >> 8) % (uint32_t)n);
}

static inline int JanjanGroupOf (int nTile)
{
	return nTile / JANJAN_GROUP_SIZE;
}

/* 山の残り枚数 */
static inline int JanjanPileLeft (const JanjanGame &g)
{
	return JANJAN_TILES - g.nPileTop;
}

/* 手牌へ昇順に足す／1 枚抜く */
static inline void JanjanHandAdd (JanjanGame &g, int nSeat, int nTile)
{
	int i = g.anHand[nSeat] ++;
	while (i > 0 && g.aHand[nSeat][i - 1] > nTile) {
		g.aHand[nSeat][i] = g.aHand[nSeat][i - 1];
		i --;
	}
	g.aHand[nSeat][i] = nTile;
}

static inline int JanjanHandCount (const JanjanGame &g, int nSeat, int nTile)
{
	int i, n = 0;
	for (i = 0; i < g.anHand[nSeat]; i ++) {
		if (g.aHand[nSeat][i] == nTile) n ++;
	}
	return n;
}

static inline bool JanjanHandRemove (JanjanGame &g, int nSeat, int nTile)
{
	int i;
	for (i = 0; i < g.anHand[nSeat]; i ++) {
		if (g.aHand[nSeat][i] == nTile) {
			for (; i + 1 < g.anHand[nSeat]; i ++) {
				g.aHand[nSeat][i] = g.aHand[nSeat][i + 1];
			}
			g.anHand[nSeat] --;
			return true;
		}
	}
	return false;
}

/* ========================================================================= */
/* 配る																		 */
/* ========================================================================= */
/* 種から山を作って配る。親は席 0。山は 64 枚を Fisher-Yates で混ぜる（同じ種なら同じ山） */
static inline void JanjanStart (JanjanGame &g, uint32_t dwSeed)
{
	int i, j, s;
	memset (&g, 0, sizeof (g));
	g.dwRng = dwSeed;
	for (i = 0; i < JANJAN_TILES; i ++) {
		g.aPile[i] = i / JANJAN_COPIES;
	}
	for (i = JANJAN_TILES - 1; i > 0; i --) {
		j = JanjanRand (g, i + 1);
		int t = g.aPile[i];
		g.aPile[i] = g.aPile[j];
		g.aPile[j] = t;
	}
	for (i = 0; i < JANJAN_HAND; i ++) {
		for (s = 0; s < JANJAN_SEATS; s ++) {
			JanjanHandAdd (g, s, g.aPile[g.nPileTop ++]);
		}
	}
	g.nTurn = 0;
	g.nPhase = JANJAN_PHASE_DRAW;
	g.nWinner = -1;
}

/* ========================================================================= */
/* 上がり判定																 */
/* ========================================================================= */
/* 3 枚が 3 枚組か（そろい=同じ絵 3 枚 / 組そろい=同じ組の違う絵 3 枚） */
static inline bool JanjanIsSet (int a, int b, int c)
{
	if (a == b && b == c) return true;
	if (a == b || b == c || a == c) return false;
	return JanjanGroupOf (a) == JanjanGroupOf (b) && JanjanGroupOf (b) == JanjanGroupOf (c);
}

/* 枚数表 cnt の牌を全部 3 枚組に分けられるか（一番小さい絵を含む組を順に試す） */
static inline bool JanjanSplit (int *cnt)
{
	int i, j, k;
	for (i = 0; i < JANJAN_KINDS && cnt[i] == 0; i ++) {}
	if (i >= JANJAN_KINDS) return true;
	if (cnt[i] >= 3) {
		cnt[i] -= 3;
		bool b = JanjanSplit (cnt);
		cnt[i] += 3;
		if (b) return true;
	}
	int nEnd = (JanjanGroupOf (i) + 1) * JANJAN_GROUP_SIZE;
	if (nEnd > JANJAN_KINDS) nEnd = JANJAN_KINDS;
	for (j = i + 1; j < nEnd; j ++) {
		if (cnt[j] == 0) continue;
		for (k = j + 1; k < nEnd; k ++) {
			if (cnt[k] == 0) continue;
			cnt[i] --; cnt[j] --; cnt[k] --;
			bool b = JanjanSplit (cnt);
			cnt[i] ++; cnt[j] ++; cnt[k] ++;
			if (b) return true;
		}
	}
	return false;
}

/* 隠した牌 n 枚（3 の倍数）が全部 3 枚組に分けられるか。鳴いた組は数えない（呼び出し側が枚数を減らして渡す） */
static inline bool JanjanIsWinHand (const int *pTile, int n)
{
	int cnt[JANJAN_KINDS], i;
	if (n < 0 || n % JANJAN_SET_SIZE != 0) return false;
	memset (cnt, 0, sizeof (cnt));
	for (i = 0; i < n; i ++) {
		cnt[pTile[i]] ++;
	}
	return JanjanSplit (cnt);
}

/* 席の手牌に nExtra（-1 なら足さない）を足した形が上がりか。鳴いた組は完成済みなので、隠した牌だけ見ればよい */
static inline bool JanjanIsWin (const JanjanGame &g, int nSeat, int nExtra)
{
	int a[JANJAN_HAND + 2], n = g.anHand[nSeat];
	memcpy (a, g.aHand[nSeat], sizeof (int) * n);
	if (nExtra >= 0) a[n ++] = nExtra;
	return JanjanIsWinHand (a, n);
}

/* ========================================================================= */
/* 得点（役は後で足す。今は上がった人に 1 点）								 */
/* ========================================================================= */
static inline int JanjanScore (const JanjanGame &g, int nSeat, int nHow)
{
	(void)g; (void)nSeat; (void)nHow;
	return JANJAN_WIN_POINT;
}

static inline void JanjanFinishWin (JanjanGame &g, int nSeat, int nHow)
{
	g.nResult = JANJAN_RESULT_WIN;
	g.nPhase = JANJAN_PHASE_END;
	g.nWinner = nSeat;
	g.nWinHow = nHow;
	g.bLastLive = 0;
	g.anScore[nSeat] += JanjanScore (g, nSeat, nHow);
}

/* ========================================================================= */
/* 引く・ツモ・捨てる														 */
/* ========================================================================= */
/* 手番の席が山から 1 枚引く。直前の捨て牌への権利はここで消える。山が空なら false */
static inline bool JanjanDraw (JanjanGame &g, int nSeat)
{
	if (g.nPhase != JANJAN_PHASE_DRAW || nSeat != g.nTurn || JanjanPileLeft (g) <= 0) return false;
	JanjanHandAdd (g, nSeat, g.aPile[g.nPileTop ++]);
	g.nPhase = JANJAN_PHASE_DISCARD;
	g.bDrew = 1;
	g.bLastLive = 0;
	return true;
}

/* 引いた牌で上がれるか／上がる */
static inline bool JanjanCanTsumo (const JanjanGame &g, int nSeat)
{
	return g.nPhase == JANJAN_PHASE_DISCARD && nSeat == g.nTurn && g.bDrew && JanjanIsWin (g, nSeat, -1);
}

static inline bool JanjanTsumo (JanjanGame &g, int nSeat)
{
	if (!JanjanCanTsumo (g, nSeat)) return false;
	JanjanFinishWin (g, nSeat, JANJAN_HOW_TSUMO);
	return true;
}

/* 手牌から 1 枚捨てて川に置く。手番は次の席へ移り、その席が引く番になる */
static inline bool JanjanDiscard (JanjanGame &g, int nSeat, int nTile)
{
	if (g.nPhase != JANJAN_PHASE_DISCARD || nSeat != g.nTurn) return false;
	if (!JanjanHandRemove (g, nSeat, nTile)) return false;
	g.aRiver[nSeat][g.anRiver[nSeat] ++] = nTile;
	g.nLastSeat = nSeat;
	g.nLastTile = nTile;
	g.bLastLive = 1;
	g.bDrew = 0;
	g.nTurn = (nSeat + 1) % JANJAN_SEATS;
	g.nPhase = JANJAN_PHASE_DRAW;
	return true;
}

/* ========================================================================= */
/* ロン																		 */
/* ========================================================================= */
static inline bool JanjanCanRon (const JanjanGame &g, int nSeat)
{
	if (g.nPhase != JANJAN_PHASE_DRAW || !g.bLastLive) return false;
	if (nSeat < 0 || nSeat >= JANJAN_SEATS || nSeat == g.nLastSeat) return false;
	return JanjanIsWin (g, nSeat, g.nLastTile);
}

/* ロンできる席のうち、捨てた人の次の席から順で最初の席（頭ハネ）。いなければ -1 */
static inline int JanjanRonSeat (const JanjanGame &g)
{
	int i;
	for (i = 1; i < JANJAN_SEATS; i ++) {
		int s = (g.nLastSeat + i) % JANJAN_SEATS;
		if (JanjanCanRon (g, s)) return s;
	}
	return -1;
}

/* ロンする。頭ハネで負ける席（先にロンできる席がいる）は false */
static inline bool JanjanRon (JanjanGame &g, int nSeat)
{
	if (nSeat < 0 || JanjanRonSeat (g) != nSeat) return false;
	JanjanFinishWin (g, nSeat, JANJAN_HOW_RON);
	return true;
}

/* ========================================================================= */
/* 鳴き																		 */
/* ========================================================================= */
/* 鳴いたあとに 1 枚捨てられるよう、隠した牌が 3 枚以上あることと、鳴き回数の上限も見る */
static inline bool JanjanCallBase (const JanjanGame &g, int nSeat)
{
	if (g.nPhase != JANJAN_PHASE_DRAW || !g.bLastLive) return false;
	if (nSeat < 0 || nSeat >= JANJAN_SEATS || nSeat == g.nLastSeat) return false;
	return g.anMeld[nSeat] < JANJAN_MELD_MAX && g.anHand[nSeat] >= JANJAN_SET_SIZE;
}

/* そろい（ポン相当）: 同じ絵を 2 枚持っていれば誰の捨て牌でももらえる */
static inline bool JanjanCanSoroi (const JanjanGame &g, int nSeat)
{
	return JanjanCallBase (g, nSeat) && JanjanHandCount (g, nSeat, g.nLastTile) >= 2;
}

/* 組そろい（チー相当）の作り方: 捨てた人の次の席だけ。同じ組の別の絵 2 枚（a<b）の組を aOut に入れて数を返す */
static inline int JanjanGumiOptions (const JanjanGame &g, int nSeat, int aOut[][2], int nMax)
{
	int n = 0, a, b, t = g.nLastTile;
	if (!JanjanCallBase (g, nSeat) || nSeat != (g.nLastSeat + 1) % JANJAN_SEATS) return 0;
	int nBeg = JanjanGroupOf (t) * JANJAN_GROUP_SIZE;
	int nEnd = nBeg + JANJAN_GROUP_SIZE;
	if (nEnd > JANJAN_KINDS) nEnd = JANJAN_KINDS;
	for (a = nBeg; a < nEnd; a ++) {
		if (a == t || JanjanHandCount (g, nSeat, a) == 0) continue;
		for (b = a + 1; b < nEnd; b ++) {
			if (b == t || JanjanHandCount (g, nSeat, b) == 0) continue;
			if (n < nMax) {
				aOut[n][0] = a;
				aOut[n][1] = b;
			}
			n ++;
		}
	}
	return n < nMax ? n : nMax;
}

static inline bool JanjanCanGumi (const JanjanGame &g, int nSeat)
{
	int aOut[1][2];
	return JanjanGumiOptions (g, nSeat, aOut, 1) > 0;
}

/* 捨て牌を川から取って組を公開し、その席が捨てる番になる（引かない） */
static inline void JanjanMeldDone (JanjanGame &g, int nSeat, int nKind, int nA, int nB)
{
	JanjanMeld &m = g.aMeld[nSeat][g.anMeld[nSeat] ++];
	int t = g.nLastTile, tmp, i, j;
	m.nKind = nKind;
	m.nFrom = g.nLastSeat;
	m.aTile[0] = t; m.aTile[1] = nA; m.aTile[2] = nB;
	for (i = 0; i < 3; i ++) {
		for (j = i + 1; j < 3; j ++) {
			if (m.aTile[j] < m.aTile[i]) { tmp = m.aTile[i]; m.aTile[i] = m.aTile[j]; m.aTile[j] = tmp; }
		}
	}
	JanjanHandRemove (g, nSeat, nA);
	JanjanHandRemove (g, nSeat, nB);
	g.anRiver[g.nLastSeat] --;
	g.bLastLive = 0;
	g.bDrew = 0;
	g.nTurn = nSeat;
	g.nPhase = JANJAN_PHASE_DISCARD;
}

static inline bool JanjanSoroi (JanjanGame &g, int nSeat)
{
	if (!JanjanCanSoroi (g, nSeat)) return false;
	JanjanMeldDone (g, nSeat, JANJAN_MELD_SOROI, g.nLastTile, g.nLastTile);
	return true;
}

/* 手牌の a, b と捨て牌で組そろいを作る。作れない選び方は false */
static inline bool JanjanGumi (JanjanGame &g, int nSeat, int nA, int nB)
{
	int aOut[JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE][2], n, i;
	n = JanjanGumiOptions (g, nSeat, aOut, JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE);
	if (nA > nB) { int t = nA; nA = nB; nB = t; }
	for (i = 0; i < n; i ++) {
		if (aOut[i][0] == nA && aOut[i][1] == nB) {
			JanjanMeldDone (g, nSeat, JANJAN_MELD_GUMI, nA, nB);
			return true;
		}
	}
	return false;
}

/* 申し出の優先: ロン > そろい > 組そろい。同じ種類なら捨てた人の次の席から順で最初。
   aClaim[席] は各席の申し出（JANJAN_CLAIM_*。できない申し出は無視）。勝った席を *pSeat に入れて種類を返す（なければ NONE） */
static inline int JanjanResolveClaim (const JanjanGame &g, const int *aClaim, int *pSeat)
{
	int nType, i;
	for (nType = JANJAN_CLAIM_RON; nType > JANJAN_CLAIM_NONE; nType --) {
		for (i = 1; i < JANJAN_SEATS; i ++) {
			int s = (g.nLastSeat + i) % JANJAN_SEATS;
			if (aClaim[s] != nType) continue;
			bool bOk = (nType == JANJAN_CLAIM_RON) ? JanjanCanRon (g, s)
				: (nType == JANJAN_CLAIM_SOROI) ? JanjanCanSoroi (g, s) : JanjanCanGumi (g, s);
			if (bOk) {
				*pSeat = s;
				return nType;
			}
		}
	}
	return JANJAN_CLAIM_NONE;
}

/* ========================================================================= */
/* 流局																		 */
/* ========================================================================= */
/* 山が尽きて、次の人が引けない状態か */
static inline bool JanjanIsExhausted (const JanjanGame &g)
{
	return g.nPhase == JANJAN_PHASE_DRAW && JanjanPileLeft (g) <= 0;
}

/* 流局にする（山が尽きていなければ false。最後の捨て牌へのロン・鳴きの確認を済ませてから呼ぶ） */
static inline bool JanjanFinishDraw (JanjanGame &g)
{
	if (!JanjanIsExhausted (g)) return false;
	g.nResult = JANJAN_RESULT_DRAW;
	g.nPhase = JANJAN_PHASE_END;
	g.bLastLive = 0;
	return true;
}

/* ========================================================================= */
/* CPU																		 */
/* ========================================================================= */
/* 席から見えていない牌の残り枚数（自分の手牌・全員の鳴き・全員の川を引く） */
static inline int JanjanUnseen (const JanjanGame &g, int nSeat, int nTile)
{
	int n = JANJAN_COPIES - JanjanHandCount (g, nSeat, nTile), s, i, j;
	for (s = 0; s < JANJAN_SEATS; s ++) {
		for (i = 0; i < g.anRiver[s]; i ++) {
			if (g.aRiver[s][i] == nTile) n --;
		}
		for (i = 0; i < g.anMeld[s]; i ++) {
			for (j = 0; j < JANJAN_SET_SIZE; j ++) {
				if (g.aMeld[s][i].aTile[j] == nTile) n --;
			}
		}
	}
	return n > 0 ? n : 0;
}

/* 牌 nTile を残す価値（同じ絵の仲間が多いほど、同じ組の別の絵があるほど高い）。孤立牌は 0 */
static inline int JanjanTileValue (const JanjanGame &g, int nSeat, int nTile)
{
	int v = 0;
	v += (JanjanHandCount (g, nSeat, nTile) - 1) * 4;		/* 同じ絵 */
	{
		int nBeg = JanjanGroupOf (nTile) * JANJAN_GROUP_SIZE, a;
		for (a = nBeg; a < nBeg + JANJAN_GROUP_SIZE && a < JANJAN_KINDS; a ++) {
			if (a != nTile && JanjanHandCount (g, nSeat, a) > 0) v += 2;	/* 同じ組の別の絵 */
		}
	}
	return v;
}

/* nSeat の手牌から nDiscard を 1 枚捨てた形の待ち（あと 1 枚で上がれる牌）の、見えていない残り枚数の合計。待ちがなければ 0 */
static inline int JanjanWaitCount (const JanjanGame &g, int nSeat, int nDiscard)
{
	int a[JANJAN_HAND + 2], n = 0, i, t, nSum = 0, bSkip = 1;
	for (i = 0; i < g.anHand[nSeat]; i ++) {
		if (bSkip && g.aHand[nSeat][i] == nDiscard) { bSkip = 0; continue; }
		a[n ++] = g.aHand[nSeat][i];
	}
	for (t = 0; t < JANJAN_KINDS; t ++) {
		a[n] = t;
		if (JanjanIsWinHand (a, n + 1)) nSum += JanjanUnseen (g, nSeat, t);
	}
	return nSum;
}

/* CPU が捨てる牌（手牌にある id）。強さ 1=ランダム 2=孤立牌から捨てる 3=2 に加えて待ちを保つ捨て方を優先。
   手牌が空なら -1。乱数は g.dwRng を進める */
static inline int JanjanCpuDiscard (JanjanGame &g, int nSeat, int nStrength)
{
	int n = g.anHand[nSeat], i, nBest = -1, nBestScore = -1;
	if (n <= 0) return -1;
	if (nStrength <= 1) return g.aHand[nSeat][JanjanRand (g, n)];
	for (i = 0; i < n; i ++) {
		int t = g.aHand[nSeat][i];
		if (i > 0 && g.aHand[nSeat][i - 1] == t) continue;
		/* 価値が低いほど捨てたい。同点はランダム（下位ビット）で散らす */
		int nScore = 1000 - JanjanTileValue (g, nSeat, t) * 10;
		if (nStrength >= 3) {
			nScore += JanjanWaitCount (g, nSeat, t) * 1000;	/* 待ちが残る捨て方を最優先 */
		}
		nScore = nScore * 4 + JanjanRand (g, 4);
		if (nScore > nBestScore) {
			nBestScore = nScore;
			nBest = t;
		}
	}
	return nBest;
}

/* CPU の申し出。できるなら上がる／そろいは鳴く／組そろいは強さ 2 以上で鳴く。
   組そろいの時は使う 2 枚を *pA, *pB に入れる（残す価値の低い組み合わせを選ぶ） */
static inline int JanjanCpuClaim (JanjanGame &g, int nSeat, int nStrength, int *pA, int *pB)
{
	if (JanjanCanRon (g, nSeat)) return JANJAN_CLAIM_RON;
	if (JanjanCanSoroi (g, nSeat)) return JANJAN_CLAIM_SOROI;
	if (nStrength >= 2) {
		int aOut[JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE][2], n, i, nBest = 0, nBestV = 1 << 30;
		n = JanjanGumiOptions (g, nSeat, aOut, JANJAN_GROUP_SIZE * JANJAN_GROUP_SIZE);
		if (n > 0) {
			for (i = 0; i < n; i ++) {
				int v = JanjanTileValue (g, nSeat, aOut[i][0]) + JanjanTileValue (g, nSeat, aOut[i][1]);
				if (v < nBestV) { nBestV = v; nBest = i; }
			}
			*pA = aOut[nBest][0];
			*pB = aOut[nBest][1];
			return JANJAN_CLAIM_GUMI;
		}
	}
	return JANJAN_CLAIM_NONE;
}

/* ========================================================================= */
/* 台の台詞・席の位置（サーバーとクライアントで同じ読み方をする）
 *   台詞 "@j<サブゲーム情報の番号>"（数字 1 つだけ。設定は SUBGAME_JANJANINFO。番号の指す情報が SUBGAMEID_JANJAN でなければ設定は既定=無料・強さ 2）
 *   旧形式 "@j<コイン番号>,<枚数>,<CPUの強さ>"（後ろは省略可。枚数 0=無料。強さの省略は 2）も、移行のため当面は読む。
 *   数字だけ（カンマなし）なら番号、カンマがあれば旧形式。旧形式で数字 1 つだけの台詞は枚数 0（＝無料）で意味が無かったので、番号として読んで困らない
 *   台NPCのマスが卓の中心 (cx,cy)。盤は 7×7 マス（cx-3〜cx+3, cy-3〜cy+3）。
 *   座る場所のマスは 席0=(cx,cy+4) 席1=(cx+4,cy) 席2=(cx,cy-4) 席3=(cx-4,cy)
 */
/* ========================================================================= */
#define JANJAN_PARAM_NONE		(-99999)	/* 省略・空欄 */
#define JANJAN_PARAM_COUNT		(3)			/* 台詞の数値の数 */
#define JANJAN_STRENGTH_DEFAULT	(2)			/* CPU の強さの既定 */
#define JANJAN_TABLE_HALF		(3)			/* 盤の半分の大きさ（マス。7×7） */
#define JANJAN_SEAT_DIST		(4)			/* 台NPCから座る場所までの距離（マス） */

/* 台詞（"@j" で始まる）を読み、anVal[JANJAN_PARAM_COUNT] に入れる（空欄・足りない欄は JANJAN_PARAM_NONE） */
static inline void JanjanParseSpeech (const char *pszSpeech, int anVal[JANJAN_PARAM_COUNT])
{
	const char *p = pszSpeech + 2;
	int i;

	for (i = 0; i < JANJAN_PARAM_COUNT; i ++) {
		anVal[i] = JANJAN_PARAM_NONE;
	}
	for (i = 0; i < JANJAN_PARAM_COUNT; i ++) {
		bool bMinus = false, bNum = false;
		int n = 0;

		if (*p == '-') {
			bMinus = true;
			p ++;
		}
		while ((*p >= '0') && (*p <= '9')) {
			if (n < 100000) {
				n = n * 10 + (*p - '0');
			}
			bNum = true;
			p ++;
		}
		if (bNum) {
			anVal[i] = bMinus ? -n : n;
		}
		if (*p != ',') {
			break;
		}
		p ++;
	}
}

#define JANJAN_SPEECH_NO_MAX	(100000)	/* 台詞の番号の上限（これを超えたら番号ではない） */
#define JANJAN_TICKET_MAX		(99)		/* チケット必要枚数の上限 */
#define JANJAN_WAIT_DEFAULT		(60)		/* 参加募集の待ち時間（秒）の既定（0 や欄なしはこの値） */
#define JANJAN_WAIT_MIN			(5)			/* 待ち時間の下限（秒） */
#define JANJAN_WAIT_MAX			(600)		/* 待ち時間の上限（秒） */
#define JANJAN_CPU_LEVEL_MAX	(3)			/* CPU強さの最大（1=よわい・2=ふつう・3=つよい） */

/* 台詞 "@j<番号>" の番号（サブゲーム情報の番号）。"@j" の後ろが数字だけの時の番号・そうでなければ（空・旧形式のカンマ区切り・数字以外）-1 */
static inline int JanjanSpeechInfoNo (const char *pszSpeech)
{
	const char *p;
	int nNo = 0;

	if (!pszSpeech || (pszSpeech[0] != '@') || (pszSpeech[1] != 'j')) {
		return -1;
	}
	p = pszSpeech + 2;
	if ((*p < '0') || (*p > '9')) {
		return -1;
	}
	while ((*p >= '0') && (*p <= '9')) {
		nNo = nNo * 10 + (*p - '0');
		if (nNo > JANJAN_SPEECH_NO_MAX) {
			return -1;
		}
		p ++;
	}
	return (*p == '\0') ? nNo : -1;
}

/* 台の設定（SUBGAME_JANJANINFO / INFOSAVE。同じ並び）を既定値にする（新しい台・保存に無い項目。無料・強さ 2） */
template<class T> static inline void JanjanInfoSetDefault (T *p)
{
	p->byCpuLevel		= (unsigned char)JANJAN_STRENGTH_DEFAULT;
	p->wTicketItemID	= 0;
	p->byTicketNum		= 0;
	p->wWaitSec			= (unsigned short)JANJAN_WAIT_DEFAULT;
}

/* 範囲に丸める（保存データ・管理者の入力・データ API が範囲外でも遊べる値にする。CPU強さ 0 は既定の 2） */
template<class T> static inline void JanjanInfoClamp (T *p)
{
	if (p->byCpuLevel == 0) {
		p->byCpuLevel = (unsigned char)JANJAN_STRENGTH_DEFAULT;
	} else if (p->byCpuLevel > JANJAN_CPU_LEVEL_MAX) {
		p->byCpuLevel = (unsigned char)JANJAN_CPU_LEVEL_MAX;
	}
	if (p->byTicketNum > JANJAN_TICKET_MAX) {
		p->byTicketNum = (unsigned char)JANJAN_TICKET_MAX;
	}
	if (p->wWaitSec == 0) {
		p->wWaitSec = (unsigned short)JANJAN_WAIT_DEFAULT;
	} else if (p->wWaitSec < JANJAN_WAIT_MIN) {
		p->wWaitSec = (unsigned short)JANJAN_WAIT_MIN;
	} else if (p->wWaitSec > JANJAN_WAIT_MAX) {
		p->wWaitSec = (unsigned short)JANJAN_WAIT_MAX;
	}
}

/* 席 nSeat の座る場所：台NPCのマスからの相対位置（マス） */
static inline void JanjanSeatOffset (int nSeat, int *pnDx, int *pnDy)
{
	static const int aDx[JANJAN_SEATS] = { 0, JANJAN_SEAT_DIST, 0, -JANJAN_SEAT_DIST };
	static const int aDy[JANJAN_SEATS] = { JANJAN_SEAT_DIST, 0, -JANJAN_SEAT_DIST, 0 };
	*pnDx = aDx[nSeat & 3];
	*pnDy = aDy[nSeat & 3];
}
