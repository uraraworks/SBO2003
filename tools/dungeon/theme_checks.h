// 飾りの数え方と、素の絵のつなぎ目をゲーム本体・単体テストの両方で確かめる。
#pragma once
#include "../../SboSvr/src/DungeonGen/DungeonGen.h"
#include <algorithm>
#include <cstdio>

struct DungeonThemeChecks
{
	int failures = 0, global = 0, perRoom = 0, walls = 0, faces = 0, verticalDoors = 0, exposedWalls = 0, nonFaceWalls = 0;
};

inline DungeonThemeChecks CheckDungeonTheme(bool maps = false)
{
	using namespace DungeonGen;
	DungeonThemeChecks out;
	// JSON の省略時の既定値と、テーマの写しに per が残ることも確かめる。
	for (int room : {0, 1})
		for (int per : {-1, 0, 1})
		{
			std::string js = "{\"id\":1,\"decos\":[{\"room\":" + std::to_string(room) +
				(per < 0 ? "" : ",\"per\":" + std::to_string(per)) + "}]}";
			Theme th; Params p;
			int expected = per < 0 ? room : per;
			if (!ReadThemeEntry(js.c_str(), th, p) || th.decos[0].per != expected) ++out.failures;
			std::string copy;
			if (!FindThemeJson(("{\"themes\":[" + js + "]}").c_str(), 1, copy) ||
				!ReadThemeEntry(copy.c_str(), th, p) || th.decos[0].per != expected) ++out.failures;
		}
	for (int bottom : {1, 2})
		for (int seed : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 12345, 777})
		{
			Theme th;
			th.floor = {643}; th.wallTop = {645}; th.wallFace = {633}; th.wallFill = {646};
			th.arch = {643}; th.door = 38; th.doorOpen = 643; th.entryPart = 644; th.goalPart = 178;
			th.bottomRows = bottom;
			Params p; p.seed = seed; p.vertDoorRate = 100;
			Skeleton sk; std::vector<int> parts;
			if (Generate(p, &th, sk, parts) != ERR_NONE) { ++out.failures; continue; }
			// 壁を重ねず辺だけが接すると、同じ境界に二枚の外壁が並んでしまう。
			for (size_t i = 0; i < sk.rooms.size(); ++i)
				for (size_t j = i + 1; j < sk.rooms.size(); ++j)
				{
					const Room &a = sk.rooms[i], &b = sk.rooms[j];
					bool xOverlap = a.x < b.x + b.w && b.x < a.x + a.w;
					bool yOverlap = a.y < b.y + b.h && b.y < a.y + a.h;
					if ((yOverlap && (a.x + a.w == b.x || b.x + b.w == a.x)) ||
						(xOverlap && (a.y + a.h == b.y || b.y + b.h == a.y))) ++out.failures;
				}
			std::vector<char> sides(p.w * p.h, 0), bottoms(p.w * p.h, 0);
			for (const Room &r : sk.rooms)
			{
				if (r.bot == 2)
					for (int x = r.x; x < r.x + r.w; ++x) bottoms[(r.y + r.h - 1) * p.w + x] = 1;
				for (int y = r.y; y < r.y + r.h; ++y)
					for (int x : {r.x, r.x + r.w - 1}) sides[y * p.w + x] = 1;
			}
			for (int i = 0; i < p.w * p.h; ++i)
			{
				int c = sk.cells[i];
				int below = i + p.w < p.w * p.h ? sk.cells[i + p.w] : CELL_NONE;
				bool belowWall = below == CELL_TOP || below == CELL_FACE || below == CELL_WALL;
				bool wall = c == CELL_TOP || c == CELL_FACE || c == CELL_WALL;
				bool exposed = bottom == 2 && wall && IsWalkable(below);
				bool bottomFace = (bottoms[i] && !belowWall) || exposed;
				if (exposed)
				{
					++out.exposedWalls;
					if (c != CELL_FACE || parts[i] != 633) { ++out.nonFaceWalls; ++out.failures; }
					if (i >= p.w && (sk.cells[i - p.w] == CELL_TOP || sk.cells[i - p.w] == CELL_FACE || sk.cells[i - p.w] == CELL_WALL) &&
						(sk.cells[i - p.w] != CELL_TOP || parts[i - p.w] != 645)) ++out.failures;
				}
				// 外周も共有部分も、下壁の最下段は角まで側面。直下に壁が続く時だけ天井。
				if (bottoms[i] && (c == CELL_TOP || c == CELL_FACE || c == CELL_WALL) &&
					(parts[i] != (bottomFace ? 633 : 645) || c != (bottomFace ? CELL_FACE : CELL_TOP))) ++out.failures;
				if (sides[i] && (c == CELL_TOP || c == CELL_FACE || c == CELL_WALL))
				{
					++out.walls;
					if (parts[i] != (bottomFace ? 633 : 645)) ++out.failures;
				}
				if (parts[i] == 633)
				{
					++out.faces;
					// 2行では通れるマスの直上も側面。それ以外は従来の共有壁の規則。
					int cover = 0;
					for (const Room &r : sk.rooms)
						if (i % p.w >= r.x && i % p.w < r.x + r.w && i / p.w >= r.y && i / p.w < r.y + r.h) ++cover;
					if (i < p.w || parts[i - p.w] != 645 || (sides[i] && !bottomFace) ||
						(!bottomFace && cover >= 2 && below != CELL_FLOOR)) ++out.failures;
				}
			}
			for (const Link &l : sk.links)
				if (l.vert && l.type == 2)
				{
					++out.verticalDoors;
					for (int y : {l.y - 1, l.y + l.h})
						// 2行の時は扉の直上も側面。それ以外は従来の柱の規則で確かめる。
						if (y >= 0 && y < p.h && !bottoms[y * p.w + l.x] &&
							!(bottom == 2 && y + 1 < p.h && IsWalkable(sk.cells[(y + 1) * p.w + l.x])) && parts[y * p.w + l.x] == 633) ++out.failures;
				}
			if (maps && seed <= 3)
			{
				printf("素の絵 bottomRows=%d seed=%d（T=天井 F=側面 #=その他 .=床 D=扉）\n", bottom, seed);
				for (int y = 0; y < p.h; ++y)
				{
					for (int x = 0; x < p.w; ++x)
					{
						int v = parts[y * p.w + x];
						putchar(v == 645 ? 'T' : v == 633 ? 'F' : v == 646 ? '#' : v == 38 ? 'D' : v == 644 ? 'S' : v == 178 ? 'G' : v == 643 ? '.' : ' ');
					}
					putchar('\n');
				}
			}
			// 同じ骨組みで全体/部屋ごとと、部屋指定の有無を比較する。
			for (int room : {0, 1})
			{
				Theme decoTheme = th;
				if (room)
				{
					TplRoom t; t.w = t.h = 5; t.cells.assign(25, 643);
					t.cutX[0] = 1; t.cutX[1] = 4; t.cutY[0] = 2; t.cutY[1] = 4;
					decoTheme.rooms.push_back(t);
				}
				std::vector<int> counts;
				std::vector<std::vector<int>> pictures;
				for (int per : {0, 1, -1})
				{
					Deco d; d.cells = {999}; d.rate = 100; d.max = 2; d.room = room; d.per = per;
					decoTheme.decos = {d};
					Skeleton a; std::vector<int> pa;
					if (Generate(p, &decoTheme, a, pa) != ERR_NONE) { ++out.failures; continue; }
					int count = (int)std::count(pa.begin(), pa.end(), 999);
					counts.push_back(count);
					pictures.push_back(pa);
					bool perRoom = per < 0 ? room > 0 : per == 1;
					if (!perRoom && count != 2) ++out.failures;
					if (perRoom)
					{
						if (count <= 2 || count > 2 * (int)a.rooms.size()) ++out.failures;
						for (const Room &r : a.rooms)
						{
							int n = 0;
							for (int y = r.y; y < r.y + r.h; ++y)
								for (int x = r.x; x < r.x + r.w; ++x) if (pa[y * p.w + x] == 999) ++n;
							if (n > 2) ++out.failures;
						}
					}
					if (!room && seed == 1 && bottom == 1 && per >= 0)
						(per ? out.perRoom : out.global) = count;
				}
				if (counts.size() != 3 || pictures[2] != pictures[room]) ++out.failures;
			}
		}
	if (!out.walls || !out.faces || !out.verticalDoors) ++out.failures;
	return out;
}
