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
			char c = " .TF#AD"[sk.cells[y * sk.w + x]];
			if (x == sk.startX && y == sk.startY) c = 'S';
			if (x == sk.goalX && y == sk.goalY) c = 'G';
			putchar(c);
		}
		putchar('\n');
	}
	(void)ch;
}

static bool Connected(const Skeleton &sk)
{
	int W = sk.w, H = sk.h, total = 0;
	for (int c : sk.cells) if (IsWalkable(c)) ++total;
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
			if (!seen[j] && IsWalkable(sk.cells[j])) { seen[j] = 1; st.push_back(j); }
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

	// JSON 入口
	{
		std::string out;
		Err e = RunJson("{\"params\":{\"w\":30,\"h\":30,\"seed\":42},\"theme\":{\"algo\":1,\"floor\":[1],\"wallTop\":[2],\"rooms\":[],\"decos\":[],\"x\":[true,false,null,\"\\u3042\"]}}", out);
		CHECK(e == ERR_NONE && out.find("\"ok\":true") == 1 && out.find("\"seed\":42") != std::string::npos, "RunJson");
		CHECK(RunJson("{bad", out) == ERR_JSON, "bad json");
		CHECK(RunJson("{\"params\":{\"w\":4}}", out) == ERR_PARAM, "bad param");
		CHECK(RunJson("{\"params\":{\"w\":40,\"h\":40,\"seed\":5,\"algo\":3}}", out) == ERR_NONE && out.find("\"parts\":[]") != std::string::npos, "no theme");
	}

	printf(g_fail ? "FAILED: %d\n" : "ALL OK\n", g_fail);
	return g_fail ? 1 : 0;
}
