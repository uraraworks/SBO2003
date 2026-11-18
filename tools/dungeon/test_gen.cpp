// DungeonGen の単体テスト（ゲーム本体なしでホストの g++ だけでビルドできる）
//   使い方: tools/dungeon/test_gen.sh   （-v を付けると格子を文字で表示）
#include "SboSvr/src/DungeonGen/DungeonGen.h"
#include "SboSvr/src/DungeonGen/MiniJson.h"
#include <cstdio>
#include <cstring>

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
	Opening bad; bad.dir = 0; bad.type = 1; bad.w = 3; bad.h = 3; bad.cells.assign(9, 1); bad.hole0 = 0; bad.hole1 = 1;  // h != 2 は無効
	t.openings.push_back(bad);
	return t;
}

int main(int argc, char **argv)
{
	bool verbose = argc > 1 && !strcmp(argv[1], "-v");
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

	printf(g_fail ? "FAILED: %d\n" : "ALL OK\n", g_fail);
	return g_fail ? 1 : 0;
}
