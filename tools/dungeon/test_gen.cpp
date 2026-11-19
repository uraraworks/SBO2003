// DungeonGen の単体テスト（ゲーム本体なしでホストの g++ だけでビルドできる）
//   使い方: tools/dungeon/test_gen.sh   （-v を付けると格子を文字で表示）
#include "SboSvr/src/DungeonGen/DungeonGen.h"
#include "SboSvr/src/DungeonGen/MiniJson.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "theme_checks.h"

using namespace DungeonGen;

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_fail; printf("NG: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void Show(const Skeleton &sk)
{
	const char *ch = ".#=-|+Dd";  // 0..6
	for (int y = 0; y < sk.h; ++y)
	{
		for (int x = 0; x < sk.w; ++x)
		{
			char c = " .TF#ADP"[sk.cells[y * sk.w + x]];
			if (x == sk.startX && y == sk.startY) c = 'S';
			if (x == sk.goalX && y == sk.goalY) c = 'G';
			putchar(c);
		}
		putchar('\n');
	}
	(void)ch;
}

// 縦の壁の出入口か（横の出入口の x は部屋の内側、縦は部屋の左右の壁の列）
static bool IsVertLink(const Skeleton &sk, const Link &l)
{
	const Room &r = sk.rooms[l.a];
	return l.x == r.x || l.x == r.x + r.w - 1;
}

static bool Connected(const Skeleton &sk)
{
	int W = sk.w, H = sk.h, total = 0;
	for (int i = 0; i < W * H; ++i) if (WalkableAt(sk.cells, W, H, sk.rooms, i % W, i / W)) ++total;
	std::vector<char> seen(W * H, 0);
	std::vector<int> st(1, sk.startY * W + sk.startX);
	seen[st[0]] = 1;
	int n = 0;
	while (!st.empty())
	{
		int i = st.back(); st.pop_back(); ++n;
		int d[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (auto &k : d)
		{
			int x = i % W + k[0], y = i / W + k[1];
			if (x < 0 || y < 0 || x >= W || y >= H) continue;
			int j = y * W + x;
			if (!seen[j] && WalkableAt(sk.cells, W, H, sk.rooms, x, y)) { seen[j] = 1; st.push_back(j); }
		}
	}
	return n == total && seen[sk.goalY * W + sk.goalX];
}

static Theme MakeTheme(int algo)
{
	Theme t;
	t.algo = algo;
	t.floor = { 101, 102 }; t.wallTop = { 7 }; t.wallFace = { 6 }; t.wallFill = { 7 };
	t.arch = { 103 }; t.door = 104;
	for (int kind = 0; kind < 3; ++kind)
	{
		TplRoom r;
		r.name = "r"; r.kind = kind; r.w = 5; r.h = 5;
		r.cells.assign(25, 200 + kind);
		r.cutX[0] = 1; r.cutX[1] = 4; r.cutY[0] = 2; r.cutY[1] = 4;
		t.rooms.push_back(r);
	}
	Deco d1; d1.name = "pillar"; d1.w = 1; d1.h = 1; d1.cells = { 300 }; d1.on = 1; d1.rate = 5; d1.max = 6;
	Deco d2; d2.name = "torch"; d2.w = 1; d2.h = 1; d2.cells = { 824 }; d2.on = 3; d2.rate = 20; d2.max = 8;
	t.decos = { d1, d2 };
	return t;
}

// 固定パターン（入口 7x6・ボス 6x6）付きのテーマ。外周は壁（7）、中は床（101）
static Theme MakeFixedTheme(int algo)
{
	Theme t = MakeTheme(algo);
	const int dims[2][3] = { { 2, 7, 6 }, { 1, 6, 6 } };  // kind, w, h
	for (auto &d : dims)
	{
		TplRoom r;
		r.name = d[0] == 2 ? "fixedEntry" : "fixedBoss"; r.kind = d[0]; r.w = d[1]; r.h = d[2];
		r.fixed = true;
		r.cutX[0] = 1; r.cutX[1] = r.w - 1; r.cutY[0] = 2; r.cutY[1] = r.h - 1;
		for (int y = 0; y < r.h; ++y)
			for (int x = 0; x < r.w; ++x)
			{
				bool edge = x == 0 || y == 0 || x == r.w - 1 || y == r.h - 1;
				r.cells.push_back(edge ? 7 : (d[0] == 2 ? 401 : 402));
			}
		if (d[0] == 2) { r.entryX = 3; r.entryY = 3; r.cells[3 * r.w + 0] = 101; }
		t.rooms.push_back(r);
	}
	return t;
}

// 出入口パターン付きのテーマ（dir0/dir1 × アーチ/扉/壁ごと抜く）。柱・枠は 8xx、穴は 9xx
static Theme MakeOpeningTheme()
{
	Theme t = MakeTheme(1);
	struct D { int dir, type, w, h, h0, h1; };
	const D ds[] = { { 0, 1, 6, 2, 2, 4 }, { 0, 2, 5, 2, 1, 4 }, { 0, 3, 4, 2, 1, 3 }, { 0, 1, 2, 2, 0, 2 },
		{ 1, 1, 5, 1, 1, 3 }, { 1, 2, 4, 1, 0, 2 }, { 1, 3, 3, 1, 1, 2 } };
	for (auto &d : ds)
	{
		Opening o;
		o.name = "op"; o.dir = d.dir; o.type = d.type; o.w = d.dir == 0 ? d.w : 1; o.h = d.dir == 0 ? 2 : d.h;
		o.hole0 = d.h0; o.hole1 = d.h1;
		int len = d.dir == 0 ? o.w : o.h;
		for (int j = 0; j < o.h; ++j)
			for (int i = 0; i < o.w; ++i)
			{
				int k = d.dir == 0 ? i : j;
				bool hole = k >= o.hole0 && k < o.hole1;
				o.cells.push_back(hole ? 900 + d.type * 10 + j : 800 + d.type * 10 + j);
			}
		if (d.type == 2) o.cellsOpen.assign(len * (d.dir == 0 ? 2 : 1), 950);
		t.openings.push_back(o);
	}
	Opening bad; bad.dir = 0; bad.type = 1; bad.w = 3; bad.h = 3; bad.cells.assign(9, 1); bad.hole0 = 0; bad.hole1 = 5;  // 穴が長さを超える（無効）
	t.openings.push_back(bad);
	return t;
}

// 伸ばす出入口パターン（cut0/cut1・off あり）のテーマ。絵は 1000 + 行*100 + 列（テンプレート内の位置）
static Theme MakeStretchTheme()
{
	Theme t = MakeTheme(1);
	t.openings.clear();
	// dir0: 幅 7・高さ 4。off=1（行 1 が天井、行 2 が側面）。穴は列 2..4、切れ目 3..4（真ん中は列 3）
	// dir1: 幅 3・高さ 6。off=1（列 1 が共有壁）。穴は行 1..3、切れ目 2..3
	for (int dir = 0; dir < 2; ++dir)
	{
		Opening o;
		o.name = "st"; o.dir = dir; o.type = 1; o.off = 1;
		o.w = dir == 0 ? 7 : 3; o.h = dir == 0 ? 4 : 6;
		o.hole0 = dir == 0 ? 2 : 1; o.hole1 = dir == 0 ? 5 : 4;
		o.cut0 = dir == 0 ? 3 : 2; o.cut1 = dir == 0 ? 4 : 3;
		for (int j = 0; j < o.h; ++j)
			for (int i = 0; i < o.w; ++i) o.cells.push_back(1000 + j * 100 + i);
		t.openings.push_back(o);
	}
	return t;
}

int main(int argc, char **argv)
{
	bool verbose = argc > 1 && !strcmp(argv[1], "-v");
	DungeonThemeChecks themeChecks = CheckDungeonTheme(verbose);
	printf("通れるマスの直上の壁: %d マス、側面でない壁: %d マス\n", themeChecks.exposedWalls, themeChecks.nonFaceWalls);
	CHECK(themeChecks.failures == 0, "theme checks: %d failures", themeChecks.failures);
	const int seeds[] = { 1, 2, 3, 12345, 777, 20031, 99999 };
	const int sizes[][2] = { { 40, 40 }, { 64, 48 }, { 24, 24 }, { 128, 128 } };

	for (int algo = 1; algo <= 3; algo += 2)
		for (auto &sz : sizes)
			for (int seed : seeds)
			{
				Params p; p.algo = algo; p.w = sz[0]; p.h = sz[1]; p.seed = seed;
				Theme th = MakeTheme(algo);
				Skeleton a, b; std::vector<int> pa, pb;
				Err e = Generate(p, &th, a, pa);
				if (e != ERR_NONE) { CHECK(0, "algo %d %dx%d seed %d: error %d", algo, sz[0], sz[1], seed, e); continue; }
				Generate(p, &th, b, pb);
				CHECK(a.cells == b.cells && pa == pb && a.rooms.size() == b.rooms.size(), "algo %d seed %d: not reproducible", algo, seed);
				CHECK(a.seed == (uint32_t)seed, "seed echo");
				CHECK((int)a.cells.size() == a.w * a.h && (int)pa.size() == a.w * a.h, "size");
				CHECK(Connected(a), "algo %d %dx%d seed %d: floors not connected", algo, sz[0], sz[1], seed);
				CHECK(a.floorCount * 100 >= a.w * a.h * p.floorMin, "algo %d seed %d: floor %d too small", algo, seed, a.floorCount);
				CHECK(IsWalkable(a.cells[a.startY * a.w + a.startX]) && IsWalkable(a.cells[a.goalY * a.w + a.goalX]), "start/goal not floor");
				for (int i = 0; i < a.w * a.h; ++i)
					if (IsWalkable(a.cells[i])) CHECK(pa[i] != 0, "floor part empty");
				// 部屋: 範囲内・重なり無し（共有壁の 1 行 / 1 列だけ可）
				for (size_t i = 0; i < a.rooms.size(); ++i)
				{
					const Room &r = a.rooms[i];
					CHECK(r.x >= 0 && r.y >= 0 && r.x + r.w <= a.w && r.y + r.h <= a.h, "room out of range");
					for (size_t j = i + 1; j < a.rooms.size(); ++j)
					{
						const Room &s = a.rooms[j];
						int iw = std::min(r.x + r.w, s.x + s.w) - std::max(r.x, s.x);
						int ih = std::min(r.y + r.h, s.y + s.h) - std::max(r.y, s.y);
						CHECK(!(iw > 0 && ih > 0 && iw != 1 && ih != 1), "rooms %zu %zu overlap", i, j);
					}
				}
				if (algo == 1)
				{
					int boss = 0, ent = 0;
					for (auto &r : a.rooms) { if (r.kind == 1) ++boss; if (r.kind == 2) ++ent; }
					CHECK(ent == 1 && (boss == 1 || a.rooms.size() == 1), "room kinds");
					CHECK(a.links.size() + 1 >= a.rooms.size(), "links too few");
				}
				if (verbose && sz[0] == 40 && (seed == 1 || seed == 2))
				{
					printf("--- algo %d %dx%d seed %d rooms %zu links %zu floor %d tries %d\n", algo, a.w, a.h, seed, a.rooms.size(), a.links.size(), a.floorCount, a.tries);
					Show(a);
				}
			}

	// 固定パターンの入口・ボス
	for (int algo = 1; algo <= 3; algo += 2)
		for (int seed : seeds)
		{
			Params p; p.algo = algo; p.w = 48; p.h = 48; p.seed = seed;
			Theme th = MakeFixedTheme(algo);
			Skeleton a, b; std::vector<int> pa, pb;
			Err e = Generate(p, &th, a, pa);
			if (e != ERR_NONE) { CHECK(0, "fixed algo %d seed %d: error %d", algo, seed, e); continue; }
			Generate(p, &th, b, pb);
			CHECK(a.cells == b.cells && pa == pb, "fixed algo %d seed %d: not reproducible", algo, seed);
			CHECK(Connected(a), "fixed algo %d seed %d: not connected", algo, seed);
			CHECK(a.floorCount * 100 >= a.w * a.h * p.floorMin, "fixed algo %d seed %d: floor small", algo, seed);
			const Room *ent = 0, *boss = 0;
			for (auto &r : a.rooms) { if (r.kind == 2 && r.fixed) ent = &r; if (r.kind == 1 && r.fixed) boss = &r; }
			CHECK(ent && ent->w == 7 && ent->h == 6, "fixed algo %d seed %d: entry pattern missing", algo, seed);
			if (ent)
			{
				CHECK(a.startX == ent->x + 3 && a.startY == ent->y + 3, "start != entry");
				CHECK(a.cells[a.startY * a.w + a.startX] == CELL_PATTERN, "start not pattern");
				// 範囲内の絵: 穴でないマスはパターンの値
				int cnt401 = 0;
				for (int y = ent->y; y < ent->y + ent->h; ++y)
					for (int x = ent->x; x < ent->x + ent->w; ++x) if (pa[y * a.w + x] == 401) ++cnt401;
				CHECK(cnt401 >= 10 && cnt401 <= 20, "entry pattern not placed as is (%d)", cnt401);  // 穴の所だけ出入口の絵で上書き
			}
			if (algo == 1)
			{
				CHECK(boss != 0 || a.rooms.size() > 1, "fixed boss missing");
				if (boss)
				{
					int far = 0;
					for (size_t i = 0; i < a.rooms.size(); ++i) if (&a.rooms[i] == boss) far = (int)i;
					CHECK(far == (int)a.rooms.size() - 1, "boss not last");
				}
				CHECK(ent && a.rooms[0].fixed == 1 && a.rooms[0].tpl >= 0, "room0 not fixed");
			}
			else
				for (int y = ent ? ent->y : 0; ent && y < ent->y + ent->h; ++y)
					for (int x = ent->x; x < ent->x + ent->w; ++x)
						CHECK(a.cells[y * a.w + x] == CELL_PATTERN, "L3 pattern cell");
			if (verbose && seed == 1) { printf("--- fixed algo %d seed %d\n", algo, seed); Show(a); }
		}

	// 出入口パターン
	{
		int used = 0;
		for (int seed : seeds)
			for (int sz : { 40, 64 })
			{
				Params p; p.algo = 1; p.w = sz; p.h = sz; p.seed = seed; p.doorRate = 40; p.openRate = 20;
				Theme th = MakeOpeningTheme();
				Skeleton a, b; std::vector<int> pa, pb;
				Err e = Generate(p, &th, a, pa);
				if (e != ERR_NONE) { CHECK(0, "opening seed %d: error %d", seed, e); continue; }
				Generate(p, &th, b, pb);
				CHECK(a.cells == b.cells && pa == pb, "opening seed %d: not reproducible", seed);
				CHECK(Connected(a), "opening seed %d: not connected", seed);
				for (auto &l : a.links)
				{
					CHECK(l.tpl < (int)th.openings.size() - 1, "invalid opening used");
					if (l.tpl < 0) continue;
					++used;
					const Opening &o = th.openings[l.tpl];
					const Room &ra = a.rooms[l.a], &rb = a.rooms[l.b];
					CHECK(o.dir == (l.h == 2 && l.w != 1 ? 0 : (l.w == 1 ? 1 : 0)) && o.type == l.type, "opening dir/type mismatch");
					int hole = o.hole1 - o.hole0;
					int ox = o.dir == 0 ? l.x - o.hole0 : l.x, oy = o.dir == 0 ? l.y : l.y - o.hole0;
					CHECK((o.dir == 0 ? l.w : l.h) == hole, "hole width %d != %d", o.dir == 0 ? l.w : l.h, hole);
					if (o.dir == 0)
						CHECK(ox >= std::max(ra.x, rb.x) + 1 && ox + o.w <= std::min(ra.x + ra.w, rb.x + rb.w) - 1, "opening out of shared wall");
					else
						CHECK(oy >= std::max(ra.y + 2, rb.y + 2) && oy + o.h <= std::min(ra.y + ra.h, rb.y + rb.h) - 1, "opening out of shared wall (v)");
					for (int j = 0; j < o.h; ++j)
						for (int i = 0; i < o.w; ++i)
						{
							int idx = (oy + j) * a.w + ox + i;
							CHECK(pa[idx] == o.cells[j * o.w + i], "opening part mismatch %d vs %d", pa[idx], o.cells[j * o.w + i]);
							bool inHole = (o.dir == 0 ? i : j) >= o.hole0 && (o.dir == 0 ? i : j) < o.hole1;
							int c = a.cells[idx];
							CHECK(inHole ? (c == CELL_ARCH || c == CELL_DOOR) : (c == CELL_TOP || c == CELL_FACE || c == CELL_WALL || c == CELL_PATTERN), "opening cell kind");
						}
				}
			}
		CHECK(used > 20, "openings rarely used (%d)", used);
	}

	// 伸ばす出入口パターン（cut0/cut1・off）と縦の出入口の床 archV
	{
		int used = 0, wMin = 99, wMax = 0, hMin = 99, hMax = 0, overflow = 0;
		for (int seed : seeds)
			for (int sz : { 40, 64 })
			{
				Params p; p.algo = 1; p.w = sz; p.h = sz; p.seed = seed; p.doorRate = 0; p.openRate = 0; p.archMin = 2; p.archMax = 6;
				Theme th = MakeStretchTheme();
				Skeleton a, b; std::vector<int> pa, pb;
				Err e = Generate(p, &th, a, pa);
				if (e != ERR_NONE) { CHECK(0, "stretch seed %d: error %d", seed, e); continue; }
				Generate(p, &th, b, pb);
				CHECK(a.cells == b.cells && pa == pb, "stretch seed %d: not reproducible", seed);
				CHECK(Connected(a), "stretch seed %d: not connected", seed);
				for (auto &l : a.links)
				{
					if (l.tpl < 0) { CHECK(l.px == -1 && l.pw == -1, "no pattern but px set"); continue; }
					++used;
					const Opening &o = th.openings[l.tpl];
					CHECK(o.dir == l.vert, "stretch dir mismatch");
					const int len = o.Len(), holeW = o.hole1 - o.hole0;
					const int L = o.dir == 0 ? l.pw : l.ph;
					const int aw = o.dir == 0 ? l.w : l.h;
					CHECK(aw >= holeW - (o.cut1 - o.cut0) + 1 && aw <= 6 && L == len + aw - holeW, "stretch aw %d L %d", aw, L);
					if (o.dir == 0) { wMin = std::min(wMin, aw); wMax = std::max(wMax, aw); }
					else { hMin = std::min(hMin, aw); hMax = std::max(hMax, aw); }
					const Room &ra = a.rooms[l.a], &rb = a.rooms[l.b];
					if (o.dir == 0)
					{
						CHECK(l.ph == o.h && l.py == l.y - o.off && l.px + l.pw <= std::min(ra.x + ra.w, rb.x + rb.w) - 1 && l.px >= std::max(ra.x, rb.x) + 1, "stretch range (h)");
						CHECK(l.x == l.px + o.hole0 && l.h == 2, "stretch hole pos (h)");
					}
					else
					{
						CHECK(l.pw == o.w && l.px == l.x - o.off && l.py + l.ph <= std::min(ra.y + ra.h, rb.y + rb.h) - 1 && l.py >= std::max(ra.y + 2, rb.y + 2), "stretch range (v)");
						CHECK(l.y == l.py + o.hole0 && l.w == 1, "stretch hole pos (v)");
					}
					for (int j = 0; j < l.ph; ++j)
						for (int i = 0; i < l.pw; ++i)
						{
							int k = o.dir == 0 ? i : j, Lk = L;
							int tk = k < o.cut0 ? k : (k >= Lk - (len - o.cut1) ? o.cut1 + k - (Lk - (len - o.cut1)) : o.cut0 + (k - o.cut0) % (o.cut1 - o.cut0));
							int ti = o.dir == 0 ? tk : i, tj = o.dir == 0 ? j : tk;
							int idx = (l.py + j) * a.w + l.px + i;
							CHECK(pa[idx] == 1000 + tj * 100 + ti, "stretch part (%d,%d) %d vs %d", i, j, pa[idx], 1000 + tj * 100 + ti);
							// 壁の行・列の外（はみ出した行）も描かれ、骨組みは変わらない
							bool outside = o.dir == 0 ? (j < o.off || j > o.off + 1) : i != o.off;
							if (outside) { ++overflow; CHECK(a.cells[idx] != CELL_ARCH && a.cells[idx] != CELL_DOOR, "overflow cell is hole"); }
						}
				}
			}
		CHECK(used > 10, "stretch rarely used (%d)", used);
		CHECK(wMax > wMin && wMax <= 6 && wMin >= 2, "dir0 hole width does not vary (%d..%d)", wMin, wMax);
		CHECK(hMax > hMin || hMin == 99, "dir1 hole width does not vary (%d..%d)", hMin, hMax);
		CHECK(overflow > 0, "no overflow cells checked");
		// archV: 縦の出入口の床。無ければ arch と同じ
		for (int variant = 0; variant < 2; ++variant)
		{
			Params p; p.algo = 1; p.w = 64; p.h = 64; p.seed = 777; p.doorRate = 0; p.openRate = 0;
			Theme th = MakeTheme(1);
			th.arch = { 103 };
			if (variant == 0) th.archV = { 777 };
			Skeleton a; std::vector<int> pa;
			CHECK(Generate(p, &th, a, pa) == ERR_NONE, "archV generate");
			int nv = 0;
			for (auto &l : a.links)
				for (int y = l.y; y < l.y + l.h; ++y)
					for (int x = l.x; x < l.x + l.w; ++x)
					{
						int v = pa[y * a.w + x];
						CHECK(v == (l.vert && variant == 0 ? 777 : 103), "archV part %d (vert %d variant %d)", v, l.vert, variant);
						nv += l.vert;
					}
			CHECK(nv > 0, "no vertical links for archV test");
		}
	}

	// 縦の壁の出入口（vertRate / vertDoorRate）
	{
		int vertLinks = 0, vertDoors = 0;
		for (int seed : seeds)
			for (auto &sz : sizes)
			{
				// vertRate=0: 縦の壁には穴を開けず、上下だけでつながる
				Params p; p.algo = 1; p.w = sz[0]; p.h = sz[1]; p.seed = seed; p.vertRate = 0; p.loopRate = 100;
				Skeleton a; std::vector<int> pa;
				Err e = Generate(p, nullptr, a, pa);
				if (e != ERR_NONE) { CHECK(0, "vertRate0 %dx%d seed %d: error %d", sz[0], sz[1], seed, e); continue; }
				CHECK(Connected(a), "vertRate0 %dx%d seed %d: not connected", sz[0], sz[1], seed);
				for (auto &l : a.links) CHECK(!IsVertLink(a, l), "vertRate0: vertical link exists");
				for (auto &r : a.rooms)
					for (int y = r.y + 2; y <= r.y + r.h - 2; ++y)
						for (int x : { r.x, r.x + r.w - 1 })
						{
							int c = a.cells[y * a.w + x];
							CHECK(c != CELL_ARCH && c != CELL_DOOR, "vertRate0: hole in vertical wall (%d,%d)", x, y);
						}
				// vertDoorRate=100: 縦の出入口はぜんぶ扉、横は doorRate=0 でアーチだけ
				Params q; q.algo = 1; q.w = sz[0]; q.h = sz[1]; q.seed = seed; q.doorRate = 0; q.openRate = 0; q.vertDoorRate = 100; q.loopRate = 100;
				Skeleton b; std::vector<int> pb;
				e = Generate(q, nullptr, b, pb);
				if (e != ERR_NONE) { CHECK(0, "vertDoor %dx%d seed %d: error %d", sz[0], sz[1], seed, e); continue; }
				CHECK(Connected(b), "vertDoor %dx%d seed %d: not connected", sz[0], sz[1], seed);
				for (auto &l : b.links)
				{
					bool vert = IsVertLink(b, l);
					if (vert) { ++vertLinks; vertDoors += l.type == 2; }
					else CHECK(l.type == 1, "vertDoor: horizontal link type %d", l.type);
				}
			}
		CHECK(vertLinks > 20 && vertDoors == vertLinks, "vertDoorRate=100: %d/%d doors", vertDoors, vertLinks);
	}

	// 下の壁 2 行（theme.bottomRows = 2）: 上下の部屋は 2 行重なり、重なった 2 行は天井＋側面、出入口は 2 行とも抜ける
	{
		int stacked = 0, linksH = 0;
		for (int variant = 0; variant < 3; ++variant)
			for (int seed : seeds)
				for (auto &sz : sizes)
				{
					Theme th = variant == 1 ? MakeFixedTheme(1) : (variant == 2 ? MakeOpeningTheme() : MakeTheme(1));
					if (variant != 1) th.rooms.clear();  // 素の絵で壁の天井・側面を確かめる
					th.bottomRows = 2;
					Params p; p.algo = 1; p.w = sz[0]; p.h = sz[1]; p.seed = seed; p.doorRate = 40; p.openRate = 20; p.loopRate = 60;
					Skeleton a, b; std::vector<int> pa, pb;
					Err e = Generate(p, &th, a, pa);
					if (e != ERR_NONE) { CHECK(0, "bottom2 v%d %dx%d seed %d: error %d", variant, sz[0], sz[1], seed, e); continue; }
					Generate(p, &th, b, pb);
					CHECK(a.cells == b.cells && pa == pb, "bottom2 not reproducible");
					CHECK(Connected(a), "bottom2 v%d %dx%d seed %d: not connected", variant, sz[0], sz[1], seed);
					CHECK(a.floorCount * 100 >= a.w * a.h * p.floorMin, "bottom2 floor %d too few", a.floorCount);
					const int W = a.w;
					// 部屋の大きさ・重なり判定: 重なるのは 2 行（上下）か 1 列（左右）だけ
					for (size_t i = 0; i < a.rooms.size(); ++i)
					{
						const Room &r = a.rooms[i];
						CHECK(r.bot == 2 && r.h >= 5 && r.w >= 4, "bottom2 room size %dx%d bot %d", r.w, r.h, r.bot);
						CHECK(r.x >= 0 && r.y >= 0 && r.x + r.w <= a.w && r.y + r.h <= a.h, "bottom2 room out of map");
						for (size_t j = i + 1; j < a.rooms.size(); ++j)
						{
							const Room &q = a.rooms[j];
							int iw = std::min(r.x + r.w, q.x + q.w) - std::max(r.x, q.x);
							int ih = std::min(r.y + r.h, q.y + q.h) - std::max(r.y, q.y);
							if (iw > 0 && ih > 0) CHECK(iw == 1 || ih == 2, "bottom2 bad overlap %dx%d", iw, ih);
							if (iw > 1 && ih == 2)
							{
								++stacked;
								const Room &up = r.y < q.y ? r : q, &dn = r.y < q.y ? q : r;
								CHECK(up.y + up.h - 2 == dn.y, "bottom2 stacked rows not shared");
								// 共有した 2 行: 出入口以外は 天井(2) / 側面(3)（部屋の外枠の左右の角は除く）
								for (int x = std::max(r.x, q.x) + 1; x < std::min(r.x + r.w, q.x + q.w) - 1; ++x)
								{
									int c0 = a.cells[dn.y * W + x], c1 = a.cells[(dn.y + 1) * W + x];
									bool hole0 = c0 == CELL_ARCH || c0 == CELL_DOOR, hole1 = c1 == CELL_ARCH || c1 == CELL_DOOR;
									CHECK(hole0 == hole1, "bottom2 hole must cover both rows (x=%d)", x);
									if (variant == 1 || variant == 2) continue;  // 固定パターン・パターン絵は別の絵
									// 下の行の直下にも壁がある所だけ天井でつなぐ。
									int below = dn.y + 2 < a.h ? a.cells[(dn.y + 2) * W + x] : CELL_NONE;
									bool pillar = below == CELL_TOP || below == CELL_FACE || below == CELL_WALL;
									CHECK(hole0 || (c0 == CELL_TOP && c1 == (pillar ? CELL_TOP : CELL_FACE)), "bottom2 shared rows %d/%d at x=%d", c0, c1, x);
									if (!hole0) CHECK(pa[dn.y * W + x] == 7 && (pa[(dn.y + 1) * W + x] == (c1 == CELL_TOP ? 7 : 6) || pa[(dn.y + 1) * W + x] == 824), "bottom2 shared parts");
								}
							}
						}
						if (r.fixed) continue;
						// 外側に面した下の 2 行も 天井＋側面（左右の角の列は隣の部屋の壁と共有するので除く）。中は床
						for (int x = r.x + 1; x < r.x + r.w - 1; ++x)
						{
							int c0 = a.cells[(r.y + r.h - 2) * W + x], c1 = a.cells[(r.y + r.h - 1) * W + x];
							if (c0 == CELL_PATTERN || c1 == CELL_PATTERN) continue;  // 重なった固定パターン
							bool hole = c0 == CELL_ARCH || c0 == CELL_DOOR;
							int below = r.y + r.h < a.h ? a.cells[(r.y + r.h) * W + x] : CELL_NONE;
							bool pillar = th.rooms.empty() && (below == CELL_TOP || below == CELL_FACE || below == CELL_WALL);
							CHECK(hole ? (c1 == CELL_ARCH || c1 == CELL_DOOR) : (c0 == CELL_TOP && c1 == (pillar ? CELL_TOP : CELL_FACE)), "bottom2 bottom rows %d/%d", c0, c1);
						}
						for (int y = r.y + 2; y <= r.y + r.h - 3; ++y)
							for (int x = r.x + 1; x <= r.x + r.w - 2; ++x)
								CHECK(a.cells[y * W + x] == CELL_FLOOR, "bottom2 interior not floor (%d,%d)", x, y);
					}
					for (auto &l : a.links)
					{
						bool vert = IsVertLink(a, l);
						if (!vert) ++linksH;
						for (int y = l.y; y < l.y + l.h; ++y)
							for (int x = l.x; x < l.x + l.w; ++x)
							{
								int c = a.cells[y * W + x];
								CHECK(c == CELL_ARCH || c == CELL_DOOR, "bottom2 link cell %d", c);
							}
						CHECK(vert || l.h == 2, "bottom2 horizontal link h=%d", l.h);
					}
				}
		CHECK(stacked > 50 && linksH > 50, "bottom2 stacked %d links %d", stacked, linksH);
	}

	// JSON 入口
	{
		std::string out;
		Err e = RunJson("{\"params\":{\"w\":30,\"h\":30,\"seed\":42},\"theme\":{\"algo\":1,\"floor\":[1],\"wallTop\":[2],\"rooms\":[],\"decos\":[],\"x\":[true,false,null,\"\\u3042\"]}}", out);
		CHECK(e == ERR_NONE && out.find("\"ok\":true") == 1 && out.find("\"seed\":42") != std::string::npos, "RunJson");
		CHECK(RunJson("{bad", out) == ERR_JSON, "bad json");
		CHECK(RunJson("{\"params\":{\"w\":4}}", out) == ERR_PARAM, "bad param");
		CHECK(RunJson("{\"params\":{\"w\":40,\"h\":40,\"seed\":5,\"algo\":3}}", out) == ERR_NONE && out.find("\"parts\":[]") != std::string::npos, "no theme");
		CHECK(RunJson("{\"params\":{\"w\":30,\"h\":30,\"seed\":42},\"theme\":{\"algo\":1,\"floor\":[1],\"openings\":[{\"dir\":0,\"type\":1,\"w\":3,\"h\":2,\"cells\":[1,2,3,4,5,6],\"hole0\":1,\"hole1\":2}]}}", out) == ERR_NONE && out.find("\"tpl\":0") != std::string::npos, "json openings");
	}

	// 扉の開いた絵（doorOpen）: 0 なら扉イベント用の表は空、設定があれば扉のマスだけに入る
	{
		int doors = 0, withOpen = 0;
		for (int seed : seeds)
		{
			Params p; p.algo = 1; p.w = 48; p.h = 48; p.seed = seed; p.doorRate = 60;
			Theme th = MakeTheme(1);
			Skeleton a, b; std::vector<int> pa, pb;
			CHECK(Generate(p, &th, a, pa) == ERR_NONE, "doorOpen 0: error");
			for (int v : a.doorOpen) CHECK(v == 0, "doorOpen 0 but table has %d", v);
			th.doorOpen = 105;
			CHECK(Generate(p, &th, b, pb) == ERR_NONE, "doorOpen: error");
			CHECK(a.cells == b.cells && pa == pb, "doorOpen must not change the layout");
			CHECK((int)b.doorOpen.size() == 48 * 48, "doorOpen size");
			for (int i = 0; i < 48 * 48; ++i)
			{
				if (b.doorOpen[i] == 0) continue;
				++withOpen;
				CHECK(b.cells[i] == CELL_DOOR && b.doorOpen[i] == 105 && pb[i] == 104, "doorOpen cell %d: cell %d open %d part %d", i, b.cells[i], b.doorOpen[i], pb[i]);
			}
			for (int i = 0; i < 48 * 48; ++i) if (b.cells[i] == CELL_DOOR) ++doors;
		}
		CHECK(doors > 10 && withOpen > 0 && withOpen <= doors, "doorOpen doors %d withOpen %d", doors, withOpen);
	}

	// テーマファイル（{"seq":..,"themes":[..]}）から id で探す。params（w,h,seed 以外）と doorOpen・lights も読む
	{
		const char *js = "{\"seq\":3,\"themes\":[{\"id\":1,\"algo\":1,\"floor\":[1]},"
			"{\"id\":3,\"algo\":3,\"floor\":[5],\"doorOpen\":9,\"lights\":[{\"wParts\":824,\"byLight\":4},{\"wParts\":0,\"byLight\":1}],"
			"\"params\":{\"roomMin\":7,\"doorRate\":55,\"cutoff\":10}}]}";
		Theme th; Params p;
		CHECK(FindThemeInFile(js, 3, th, p), "FindThemeInFile 3");
		CHECK(th.algo == 3 && th.floor.size() == 1 && th.doorOpen == 9 && th.lights.size() == 1 && th.lights[0].first == 824 && th.lights[0].second == 4, "theme fields");
		CHECK(p.roomMin == 7 && p.doorRate == 55 && p.cutoff == 10 && p.roomMax == 12 && p.floorMin == 35, "theme params %d %d", p.roomMin, p.roomMax);
		CHECK(FindThemeInFile(js, 1, th, p) && th.doorOpen == 0 && p.roomMin == 6, "theme 1 defaults");
		CHECK(!FindThemeInFile(js, 2, th, p) && !FindThemeInFile("{bad", 1, th, p) && !FindThemeInFile("{}", 1, th, p), "FindThemeInFile misses");
	}

	// テーマの写し（日替わりダンジョンの版に残す）: FindThemeJson → ReadThemeEntry で、ファイルから読んだ時と同じテーマ・同じ形になる。
	// 同じ中身は同じ文字列（使い回しの比べ）、書き直して読み直しても変わらない
	{
		const char *js = "{\"seq\":3,\"themes\":[{\"id\":1,\"algo\":1,\"floor\":[1]},"
			"{\"id\":3,\"name\":\"\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88\\\"q\\\\\\n\",\"algo\":1,\"floor\":[1,2],\"wallTop\":[3],\"wallFace\":[4],\"wallFill\":[5],\"arch\":[6],"
			"\"door\":7,\"doorOpen\":8,\"entry\":9,\"goal\":10,\"bottomRows\":1,\"rooms\":[],\"decos\":[],\"lights\":[{\"wParts\":824,\"byLight\":4}],"
			"\"params\":{\"roomMin\":7,\"doorRate\":55,\"cutoff\":10}}]}";
		std::string copy, copy2;
		CHECK(FindThemeJson(js, 3, copy) && !copy.empty() && copy[0] == '{', "FindThemeJson 3");
		CHECK(FindThemeJson(js, 3, copy2) && copy == copy2, "copy is stable");
		CHECK(!FindThemeJson(js, 2, copy2) && copy2.empty() && !FindThemeJson("{bad", 1, copy2), "FindThemeJson misses");
		// 書き直し（読み直した JSON を書く）しても同じ文字列
		{
			JVal v; std::string again;
			CHECK(ParseJson(copy.c_str(), v), "copy parses");
			WriteJson(v, again);
			CHECK(again == copy, "write is canonical");
		}
		Theme thA, thB; Params pA, pB;
		CHECK(FindThemeInFile(js, 3, thA, pA) && ReadThemeEntry(copy.c_str(), thB, pB), "read both");
		CHECK(thB.doorOpen == 8 && thB.lights.size() == 1 && pB.roomMin == 7 && pB.doorRate == 55 && pB.cutoff == 10 && pB.roomMax == pA.roomMax, "copy fields");
		CHECK(!ReadThemeEntry("[1]", thB, pB) && !ReadThemeEntry("{bad", thB, pB) && !ReadThemeEntry(0, thB, pB), "ReadThemeEntry misses");
		for (int seed : seeds)
		{
			Params a = pA, b = pB; a.w = b.w = 40; a.h = b.h = 40; a.seed = b.seed = (uint32_t)seed;
			Skeleton sa, sb; std::vector<int> qa, qb;
			CHECK(Generate(a, &thA, sa, qa) == ERR_NONE && Generate(b, &thB, sb, qb) == ERR_NONE, "generate from copy");
			CHECK(sa.cells == sb.cells && qa == qb && sa.doorOpen == sb.doorOpen && sa.rooms.size() == sb.rooms.size() && sa.links.size() == sb.links.size(), "copy gives the same layout (seed %d)", seed);
		}
		CHECK(GEN_VERSION >= 1, "GEN_VERSION");
	}

	printf(g_fail ? "FAILED: %d\n" : "ALL OK\n", g_fail);
	return g_fail ? 1 : 0;
}
