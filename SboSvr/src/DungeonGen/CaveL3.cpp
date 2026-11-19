// L3 洞窟（SBOP2 MapGenCaveL3 の移植）
//   中央近くに 3x3 の起点を置き、4辺に 3〜4 マスのかたまりを再帰で生やす。
//   かたまりの内側は床、縁は半々で床。最大の連結成分だけ残し、床が floorMin% 以上になるまで作り直す。
#include "DungeonGen.h"
#include <algorithm>

namespace DungeonGen
{
namespace
{

enum Dir { DIR_NORTH = 0, DIR_SOUTH = 1, DIR_WEST = 2, DIR_EAST = 3 };

struct Ctx
{
	int W, H, blockMin, blockMax, cutoff;
	std::vector<int> grid;  // 0 壁、1 床
	Rng *rng;
	int &At(int x, int y) { return grid[y * W + x]; }
};

bool FillRoom(Ctx &c, int x1, int y1, int x2, int y2)
{
	if (x1 < 0 || y1 < 0 || x2 >= c.W || y2 >= c.H) return false;
	for (int y = y1 + 1; y <= y2 - 1; ++y)
		for (int x = x1 + 1; x <= x2 - 1; ++x)
			if (c.At(x, y) == 1) return false;
	for (int y = y1 + 1; y <= y2 - 1; ++y)
		for (int x = x1 + 1; x <= x2 - 1; ++x)
			c.At(x, y) = 1;
	for (int x = x1; x <= x2; ++x)
	{
		if (c.rng->Chance(50)) c.At(x, y1) = 1;
		if (c.rng->Chance(50)) c.At(x, y2) = 1;
	}
	for (int y = y1 + 1; y <= y2 - 1; ++y)
	{
		if (c.rng->Chance(50)) c.At(x1, y) = 1;
		if (c.rng->Chance(50)) c.At(x2, y) = 1;
	}
	return true;
}

void CreateBlock(Ctx &c, int px, int py, int obs)
{
	if (c.rng->Chance(c.cutoff)) return;
	int bw = c.rng->Int(c.blockMin, c.blockMax);
	int bh = c.rng->Int(c.blockMin, c.blockMax);
	int x1, y1, x2, y2;
	if (obs == DIR_NORTH)
	{
		x1 = px - c.rng->Int(0, bw - 1); x2 = x1 + bw; y2 = py - 1; y1 = y2 - bh;
	}
	else if (obs == DIR_SOUTH)
	{
		x1 = px - c.rng->Int(0, bw - 1); x2 = x1 + bw; y1 = py + 1; y2 = y1 + bh;
	}
	else if (obs == DIR_WEST)
	{
		y1 = py - c.rng->Int(0, bh - 1); y2 = y1 + bh; x2 = px - 1; x1 = x2 - bw;
	}
	else
	{
		y1 = py - c.rng->Int(0, bh - 1); y2 = y1 + bh; x1 = px + 1; x2 = x1 + bw;
	}
	if (!FillRoom(c, x1, y1, x2, y2)) return;
	int midX = (x1 + x2) / 2, midY = (y1 + y2) / 2;
	if (obs != DIR_SOUTH) CreateBlock(c, midX, y1, DIR_NORTH);
	if (obs != DIR_NORTH) CreateBlock(c, midX, y2, DIR_SOUTH);
	if (obs != DIR_EAST) CreateBlock(c, x1, midY, DIR_WEST);
	if (obs != DIR_WEST) CreateBlock(c, x2, midY, DIR_EAST);
}

// 最大の連結成分だけ残す。残した床の数を返す
int KeepLargest(Ctx &c)
{
	const int W = c.W, H = c.H;
	std::vector<int> label(W * H, 0), stack;
	int bestId = 0, bestSize = 0, nextId = 0;
	for (int s = 0; s < W * H; ++s)
	{
		if (c.grid[s] != 1 || label[s]) continue;
		int id = ++nextId, sz = 0;
		stack.clear();
		stack.push_back(s);
		label[s] = id;
		while (!stack.empty())
		{
			int idx = stack.back();
			stack.pop_back();
			++sz;
			int x = idx % W, y = idx / W;
			static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
			for (int k = 0; k < 4; ++k)
			{
				int nx = x + nb[k][0], ny = y + nb[k][1];
				if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
				int ni = ny * W + nx;
				if (c.grid[ni] == 1 && !label[ni]) { label[ni] = id; stack.push_back(ni); }
			}
		}
		if (sz > bestSize) { bestSize = sz; bestId = id; }
	}
	for (int i = 0; i < W * H; ++i)
		if (c.grid[i] == 1 && label[i] != bestId) c.grid[i] = 0;
	return bestSize;
}

void GenerateOnce(Ctx &c)
{
	std::fill(c.grid.begin(), c.grid.end(), 0);
	// 小さい地図でも収まるよう、起点の範囲を狭める
	int loX = std::min(10, (c.W - 3) / 2), hiX = std::max(loX, c.W - 14);
	int loY = std::min(10, (c.H - 3) / 2), hiY = std::max(loY, c.H - 14);
	int sx = c.rng->Int(loX, hiX), sy = c.rng->Int(loY, hiY);
	for (int y = sy; y <= sy + 2; ++y)
		for (int x = sx; x <= sx + 2; ++x)
			c.At(x, y) = 1;
	int midX = sx + 1, midY = sy + 1;
	CreateBlock(c, midX, sy, DIR_NORTH);
	CreateBlock(c, midX, sy + 2, DIR_SOUTH);
	CreateBlock(c, sx, midY, DIR_WEST);
	CreateBlock(c, sx + 2, midY, DIR_EAST);
}

// 入口の固定パターンを、cx,cy 付近を中心に置く。置けたら true。
//   パターンの範囲の洞窟を消し、パターンの縁の床（テーマの床・出入口のパーツのマス）から
//   洞窟の床へ最短で掘ってつなぎ、パターンにつながる洞窟の床だけを残す。
//   rx,ry = 置いた左上、reach = つないだ洞窟の床（掘った先）、kept = 残した洞窟の床の数
bool StampPattern(Ctx &c, const Theme &th, const TplRoom &t, int cx, int cy, int &rx, int &ry, int &reach, int &kept)
{
	const int W = c.W, H = c.H;
	rx = std::max(0, std::min(W - t.w, cx - t.w / 2));
	ry = std::max(0, std::min(H - t.h, cy - t.h / 2));
	auto inRect = [&](int x, int y) { return x >= rx && x < rx + t.w && y >= ry && y < ry + t.h; };
	for (int y = ry; y < ry + t.h; ++y)
		for (int x = rx; x < rx + t.w; ++x) c.At(x, y) = 0;
	// 縁の床
	std::vector<int> src;
	std::vector<int> all;
	for (int j = 0; j < t.h; ++j)
		for (int i = 0; i < t.w; ++i)
		{
			if (i != 0 && j != 0 && i != t.w - 1 && j != t.h - 1) continue;
			int v = t.cells[j * t.w + i], idx = (ry + j) * W + rx + i;
			all.push_back(idx);
			if (v == 0) continue;
			bool fl = v == th.door;
			for (size_t k = 0; k < th.floor.size() && !fl; ++k) fl = th.floor[k] == v;
			for (size_t k = 0; k < th.arch.size() && !fl; ++k) fl = th.arch[k] == v;
			if (fl) src.push_back(idx);
		}
	if (src.empty()) src = all;  // 床が分からなければ縁ぜんぶから
	std::vector<int> par(W * H, -2), q;
	for (size_t i = 0; i < src.size(); ++i) { par[src[i]] = -1; q.push_back(src[i]); }
	int hit = -1, from = -1;
	for (size_t qi = 0; qi < q.size() && hit < 0; ++qi)
	{
		int idx = q[qi], x = idx % W, y = idx / W;
		static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + nb[k][0], ny = y + nb[k][1];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H || inRect(nx, ny)) continue;
			int ni = ny * W + nx;
			if (par[ni] != -2) continue;
			par[ni] = idx;
			if (c.grid[ni] == 1) { hit = ni; from = idx; break; }
			q.push_back(ni);
		}
	}
	if (hit < 0) return false;
	// 掘る（hit は元から床。パターンの縁から hit の手前まで）
	for (int idx = par[hit]; idx >= 0 && !inRect(idx % W, idx / W); idx = par[idx]) c.grid[idx] = 1;
	(void)from;
	// パターンにつながる床だけを残す
	std::vector<char> seen(W * H, 0);
	std::vector<int> st(1, hit);
	seen[hit] = 1;
	kept = 0;
	while (!st.empty())
	{
		int idx = st.back(); st.pop_back();
		++kept;
		int x = idx % W, y = idx / W;
		static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + nb[k][0], ny = y + nb[k][1];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int ni = ny * W + nx;
			if (c.grid[ni] == 1 && !seen[ni]) { seen[ni] = 1; st.push_back(ni); }
		}
	}
	for (int i = 0; i < W * H; ++i) if (c.grid[i] == 1 && !seen[i]) c.grid[i] = 0;
	reach = hit;
	return true;
}

} // namespace

Err GenerateCaveL3(const Params &p, Rng &rng, Skeleton &sk, const Theme *pTheme)
{
	if (p.w < 10 || p.h < 10) return ERR_PARAM;
	Ctx c;
	c.W = p.w; c.H = p.h; c.blockMin = 3; c.blockMax = 4;
	c.cutoff = std::max(0, std::min(100, p.cutoff));
	c.grid.assign(p.w * p.h, 0);
	c.rng = &rng;
	const int need = std::max(1, p.w * p.h * p.floorMin / 100);
	const int maxRetries = 1000;

	// 入口の固定パターン
	int tpl = -1;
	if (pTheme)
	{
		std::vector<int> cand;
		for (size_t i = 0; i < pTheme->rooms.size(); ++i)
		{
			const TplRoom &t = pTheme->rooms[i];
			if (t.kind == 2 && t.fixed && t.w >= 4 && t.h >= 4 && t.w <= p.w && t.h <= p.h && (int)t.cells.size() == t.w * t.h) cand.push_back((int)i);
		}
		if (!cand.empty()) tpl = cand[rng.Int(0, (int)cand.size() - 1)];
	}

	int kept = 0, tries = 0, patX = 0, patY = 0, reach = -1, patInner = 0;
	for (int attempt = 0; attempt < maxRetries; ++attempt)
	{
		++tries;
		GenerateOnce(c);
		kept = KeepLargest(c);
		if (kept < need) { kept = 0; continue; }
		if (tpl >= 0)
		{
			// start に近い床（地図の中心に近い床）を中心にパターンを置く
			int cx = p.w / 2, cy = p.h / 2, best = -1, bd = 1 << 30;
			for (int i = 0; i < p.w * p.h; ++i)
				if (c.grid[i] == 1)
				{
					int d = (i % p.w - cx) * (i % p.w - cx) + (i / p.w - cy) * (i / p.w - cy);
					if (d < bd) { bd = d; best = i; }
				}
			const TplRoom &t = pTheme->rooms[tpl];
			int k2 = 0;
			if (!StampPattern(c, *pTheme, t, best % p.w, best / p.w, patX, patY, reach, k2)) { kept = 0; continue; }
			patInner = (t.w - 2) * (t.h - 2);
			kept = k2;
			if (kept + patInner < need) { kept = 0; continue; }
		}
		break;
	}
	if (!kept) return ERR_RETRY;

	const int W = p.w, H = p.h;
	sk.w = W; sk.h = H; sk.algo = 3; sk.tries = tries;
	sk.cells.assign(W * H, CELL_NONE);
	sk.rooms.clear(); sk.links.clear();
	const std::vector<int> &g = c.grid;
	// 床
	for (int i = 0; i < W * H; ++i) if (g[i] == 1) sk.cells[i] = CELL_FLOOR;
	// 床の1つ上の壁 = 側面、そのさらに上の壁 = 天井
	for (int y = 1; y < H; ++y)
		for (int x = 0; x < W; ++x)
			if (g[y * W + x] == 1 && g[(y - 1) * W + x] != 1) sk.cells[(y - 1) * W + x] = CELL_FACE;
	for (int y = 2; y < H; ++y)
		for (int x = 0; x < W; ++x)
			if (sk.cells[y * W + x] == CELL_FACE && g[(y - 1) * W + x] != 1 && sk.cells[(y - 1) * W + x] == CELL_NONE)
				sk.cells[(y - 1) * W + x] = CELL_TOP;
	// 残りの壁で床（8方向）に隣接するもの
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
		{
			if (sk.cells[y * W + x] != CELL_NONE) continue;
			bool near = false;
			for (int dy = -1; dy <= 1 && !near; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
				{
					int nx = x + dx, ny = y + dy;
					if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
					if (g[ny * W + nx] == 1) { near = true; break; }
				}
			if (near) sk.cells[y * W + x] = CELL_WALL;
		}
	sk.floorCount = kept;
	if (tpl >= 0)
	{
		// 固定パターンの範囲をパターンのマスにする。数え方: 中（外周を除く）を床のかたまり 1 つとして数える
		const TplRoom &t = pTheme->rooms[tpl];
		for (int y = patY; y < patY + t.h; ++y)
			for (int x = patX; x < patX + t.w; ++x) sk.cells[y * W + x] = CELL_PATTERN;
		Room r;
		r.id = 0; r.x = patX; r.y = patY; r.w = t.w; r.h = t.h; r.kind = 2; r.tpl = tpl; r.fixed = 2;
		sk.rooms.push_back(r);
		sk.floorCount = kept + patInner;
	}

	// 入口 = 起点ブロックに一番近い床（地図の中心に近い床）、出口 = そこから一番遠い床（幅優先）
	int cx = W / 2, cy = H / 2, best = -1, bd = 1 << 30;
	for (int i = 0; i < W * H; ++i)
		if (g[i] == 1)
		{
			int d = (i % W - cx) * (i % W - cx) + (i / W - cy) * (i / W - cy);
			if (d < bd) { bd = d; best = i; }
		}
	if (tpl >= 0) best = reach;  // 固定パターンの時は、パターンにつないだ床から測る
	std::vector<int> dist(W * H, -1), q;
	q.push_back(best); dist[best] = 0;
	int far = best;
	for (size_t qi = 0; qi < q.size(); ++qi)
	{
		int idx = q[qi], x = idx % W, y = idx / W;
		if (dist[idx] > dist[far]) far = idx;
		static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + nb[k][0], ny = y + nb[k][1];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int ni = ny * W + nx;
			if (g[ni] == 1 && dist[ni] < 0) { dist[ni] = dist[idx] + 1; q.push_back(ni); }
		}
	}
	sk.startX = best % W; sk.startY = best / W;
	if (tpl >= 0)
	{
		// start = パターンの entry の位置（無ければパターンの中央）
		const TplRoom &t = pTheme->rooms[tpl];
		bool ok = t.entryX >= 0 && t.entryX < t.w && t.entryY >= 0 && t.entryY < t.h;
		sk.startX = patX + (ok ? t.entryX : t.w / 2);
		sk.startY = patY + (ok ? t.entryY : t.h / 2);
	}
	sk.goalX = far % W; sk.goalY = far / W;
	if (tpl >= 0)
	{
		// 全床連結の再確認（パターンの中は 1 つのかたまり、外周は隣が床なら出入口）
		std::vector<char> seen(W * H, 0);
		std::vector<int> st(1, sk.startY * W + sk.startX);
		seen[st[0]] = 1;
		int total = 0, n = 0;
		for (int i = 0; i < W * H; ++i) if (WalkableAt(sk.cells, W, H, sk.rooms, i % W, i / W)) ++total;
		while (!st.empty())
		{
			int idx = st.back(); st.pop_back(); ++n;
			static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
			for (int k = 0; k < 4; ++k)
			{
				int nx = idx % W + nb[k][0], ny = idx / W + nb[k][1];
				if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
				int ni = ny * W + nx;
				if (!seen[ni] && WalkableAt(sk.cells, W, H, sk.rooms, nx, ny)) { seen[ni] = 1; st.push_back(ni); }
			}
		}
		if (n != total) return ERR_RETRY;
	}
	return ERR_NONE;
}

} // namespace DungeonGen
