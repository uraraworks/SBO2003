// L1 聖堂（通路なし。部屋の辺にそのまま次の部屋をくっつけて生やす）
//   部屋の外枠 [x,y,w,h] は壁を含む。上の壁は 2 行（天井＋側面）、下の壁は 1 行（テーマの bottomRows が 2 なら
//   天井＋側面の 2 行）、左右の壁は 1 列。
//   上下に並ぶ部屋は下の壁の行数ぶん（1 か 2 行）、左右に並ぶ部屋は 1 列を重ねる（共有した壁）。
//   共有した壁に出入口（アーチ・扉・壁ごと抜く）を開ける。
#include "DungeonGen.h"
#include <algorithm>

namespace DungeonGen
{
namespace
{

struct Ctx
{
	const Params *p;
	const Theme *th;
	Rng *rng;
	int W, H, aMin, aMax;
	int B;  // 下の壁の行数（1 か 2）
	std::vector<Room> rooms;
	std::vector<Link> links;
};

// 縦の壁に出入口を開けてよいか（100 以上・0 以下は乱数を使わない）
bool VertOk(const Ctx &c)
{
	if (c.p->vertRate >= 100) return true;
	if (c.p->vertRate <= 0) return false;
	return c.rng->Chance(c.p->vertRate);
}

inline int IX0(const Room &r) { return r.x + 1; }
inline int IX1(const Room &r) { return r.x + r.w - 2; }
inline int IY0(const Room &r) { return r.y + 2; }
inline int IY1(const Room &r) { return r.y + r.h - 1 - r.bot; }

// 2 つの外枠の重なり（幅・高さ）。重ならなければ 0
void Inter(const Room &a, const Room &b, int &iw, int &ih)
{
	iw = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
	ih = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);
	if (iw < 0) iw = 0;
	if (ih < 0) ih = 0;
}

// 置いてよいか（他の部屋と重なるのは、ちょうど下の壁の行数ぶん（1 か 2 行）か 1 列の共有壁だけ）
bool CanPlace(const Ctx &c, const Room &n)
{
	if (n.x < 0 || n.y < 0 || n.x + n.w > c.W || n.y + n.h > c.H) return false;
	for (size_t i = 0; i < c.rooms.size(); ++i)
	{
		int iw, ih;
		Inter(c.rooms[i], n, iw, ih);
		if (iw > 0 && ih > 0 && iw != 1 && ih != c.B) return false;
	}
	return true;
}

// a, b が壁を共有していて出入口を作れるなら、その共有範囲を返す
//   horiz = true: 横の壁（上下に並ぶ）。start..end は x の範囲、line は共有壁の上の行（下の部屋の上端）
//   horiz = false: 縦の壁。start..end は y の範囲、line は共有する列
bool Shared(const Ctx &c, const Room &a, const Room &b, bool &horiz, int &start, int &end, int &line)
{
	int iw, ih;
	Inter(a, b, iw, ih);
	if (iw <= 0 || ih <= 0) return false;
	int need = std::max(1, std::min(c.aMin, std::min(std::min(a.w, b.w) - 2, std::min(a.h, b.h) - 2 - c.B)));
	if (ih == c.B && iw > 1)
	{
		start = std::max(IX0(a), IX0(b));
		end = std::min(IX1(a), IX1(b));
		if (end - start + 1 < need) return false;
		horiz = true;
		line = std::max(a.y, b.y);
		return true;
	}
	if (iw == 1 && ih > 1)
	{
		start = std::max(IY0(a), IY0(b));
		end = std::min(IY1(a), IY1(b));
		if (end - start + 1 < need) return false;
		horiz = false;
		line = std::max(a.x, b.x);
		return true;
	}
	return false;
}

// 共有壁に出入口を作る
void MakeLink(Ctx &c, int ia, int ib)
{
	bool horiz;
	int s, e, line;
	if (!Shared(c, c.rooms[ia], c.rooms[ib], horiz, s, e, line)) return;
	Link l;
	l.a = ia; l.b = ib;
	int len = e - s + 1;
	int aw = 0;
	const int doorRate = (!horiz && c.p->vertDoorRate >= 0) ? c.p->vertDoorRate : c.p->doorRate;
	int aMin = c.aMin, aMax = c.aMax;
	if (!horiz)
	{
		if (c.p->vertArchMin > 0) aMin = c.p->vertArchMin;
		if (c.p->vertArchMax > 0) aMax = c.p->vertArchMax;
		aMax = std::max(aMin, aMax);
	}
	if (c.rng->Chance(c.p->openRate)) l.type = 3;
	else l.type = c.rng->Chance(doorRate) ? 2 : 1;
	// 向きと type が合う出入口パターンで、両脇まで共有壁に収まるものからくじで選ぶ
	int pos = 0;
	if (c.th)
	{
		std::vector<int> cand;
		for (size_t k = 0; k < c.th->openings.size(); ++k)
		{
			const Opening &o = c.th->openings[k];
			if (!o.Valid() || o.dir != (horiz ? 0 : 1) || o.type != l.type) continue;
			if ((horiz ? o.w : o.h) <= len) cand.push_back((int)k);
		}
		if (!cand.empty())
		{
			l.tpl = cand[c.rng->Int(0, (int)cand.size() - 1)];
			const Opening &o = c.th->openings[l.tpl];
			aw = o.hole1 - o.hole0;
			int span = horiz ? o.w : o.h;
			pos = s + c.rng->Int(0, len - span) + o.hole0;
		}
	}
	if (l.tpl < 0)
	{
		if (l.type == 3) aw = len;
		else aw = std::min(len, c.rng->Int(aMin, aMax));
		pos = s + c.rng->Int(0, len - aw);
	}
	if (horiz) { l.x = pos; l.y = line; l.w = aw; l.h = 2; }
	else { l.x = line; l.y = pos; l.w = 1; l.h = aw; }
	c.links.push_back(l);
}

bool Linked(const Ctx &c, int a, int b)
{
	for (size_t i = 0; i < c.links.size(); ++i)
		if ((c.links[i].a == a && c.links[i].b == b) || (c.links[i].a == b && c.links[i].b == a)) return true;
	return false;
}

// 親の辺に新しい部屋を 1 つ置いてみる。置けたら true
bool TryGrow(Ctx &c, int parent)
{
	const Room p = c.rooms[parent];
	Room n;
	n.id = (int)c.rooms.size(); n.kind = 0; n.tpl = -1; n.bot = c.B;
	n.w = c.rng->Int(c.p->roomMin, c.p->roomMax);
	n.h = c.rng->Int(c.p->roomMin, c.p->roomMax);
	int dir = c.rng->Int(0, 3);  // 0 下 / 1 上 / 2 右 / 3 左
	if (dir >= 2 && !VertOk(c)) dir = c.rng->Int(0, 1);  // 縦の壁が外れたら上下を選び直す
	int need = std::max(1, std::min(c.aMin, std::min(std::min(p.w, n.w) - 2, std::min(p.h, n.h) - 2 - c.B)));
	if (dir < 2)
	{
		n.y = dir == 0 ? p.y + p.h - c.B : p.y - n.h + c.B;
		n.x = c.rng->Int(p.x - n.w + 1, p.x + p.w - 1);
	}
	else
	{
		n.x = dir == 2 ? p.x + p.w - 1 : p.x - n.w + 1;
		n.y = c.rng->Int(p.y - n.h + 1, p.y + p.h - 1);
	}
	if (!CanPlace(c, n)) return false;
	// 出入口を作れるだけ重なっているか
	bool horiz;
	int s, e, line;
	c.rooms.push_back(n);
	if (!Shared(c, c.rooms[parent], c.rooms.back(), horiz, s, e, line) || e - s + 1 < need)
	{
		c.rooms.pop_back();
		return false;
	}
	MakeLink(c, parent, n.id);
	return true;
}

// 大きさ・種類が決まった部屋 n を、親の辺のどこかに置く（向きと位置を総当たり）。置けたら true
bool TryAttach(Ctx &c, int parent, Room n)
{
	const Room p = c.rooms[parent];
	n.id = (int)c.rooms.size();
	n.bot = c.B;
	const int dir0 = c.rng->Int(0, 3);
	for (int di = 0; di < 4; ++di)
	{
		int dir = (dir0 + di) % 4;  // 0 下 / 1 上 / 2 右 / 3 左
		if (dir >= 2 && !VertOk(c)) continue;
		int lo, hi;
		if (dir < 2)
		{
			n.y = dir == 0 ? p.y + p.h - c.B : p.y - n.h + c.B;
			lo = p.x - n.w + 1; hi = p.x + p.w - 1;
		}
		else
		{
			n.x = dir == 2 ? p.x + p.w - 1 : p.x - n.w + 1;
			lo = p.y - n.h + 1; hi = p.y + p.h - 1;
		}
		const int span = hi - lo + 1, off0 = c.rng->Int(0, span - 1);
		for (int k = 0; k < span; ++k)
		{
			int v = lo + (off0 + k) % span;
			if (dir < 2) n.x = v; else n.y = v;
			if (!CanPlace(c, n)) continue;
			bool horiz;
			int s, e, line;
			c.rooms.push_back(n);
			int need = std::max(1, std::min(c.aMin, std::min(std::min(p.w, n.w) - 2, std::min(p.h, n.h) - 2 - c.B)));
			if (!Shared(c, c.rooms[parent], c.rooms.back(), horiz, s, e, line) || e - s + 1 < need)
			{
				c.rooms.pop_back();
				continue;
			}
			MakeLink(c, parent, n.id);
			return true;
		}
	}
	return false;
}

// 入口・ボス用の固定パターン（kind が合い、fixed で、地図に収まるもの）の番号一覧
std::vector<int> FixedCands(const Theme *th, int kind, int W, int H, int B)
{
	std::vector<int> v;
	if (!th) return v;
	for (size_t i = 0; i < th->rooms.size(); ++i)
	{
		const TplRoom &t = th->rooms[i];
		if (t.kind != kind || !t.fixed) continue;
		if (t.w < 4 || t.h < 3 + B || t.w > W || t.h > H || (int)t.cells.size() != t.w * t.h) continue;
		v.push_back((int)i);
	}
	return v;
}

// 部屋と出入口から格子を作る
void BuildCells(const Ctx &c, std::vector<int> &cells)
{
	const int W = c.W;
	cells.assign(W * c.H, CELL_NONE);
	// 下・左右の壁
	for (size_t i = 0; i < c.rooms.size(); ++i)
	{
		const Room &r = c.rooms[i];
		for (int x = r.x; x < r.x + r.w; ++x) cells[(r.y + r.h - 1) * W + x] = CELL_WALL;
		for (int y = r.y + 2; y < r.y + r.h; ++y)
		{
			cells[y * W + r.x] = CELL_WALL;
			cells[y * W + r.x + r.w - 1] = CELL_WALL;
		}
	}
	// 上の壁（天井＋側面）
	for (size_t i = 0; i < c.rooms.size(); ++i)
	{
		const Room &r = c.rooms[i];
		for (int x = r.x; x < r.x + r.w; ++x)
		{
			cells[r.y * W + x] = CELL_TOP;
			cells[(r.y + 1) * W + x] = CELL_FACE;
			if (r.bot == 2)
			{
				// 下の壁が 2 行の時は、下の 2 行も天井＋側面（下に重なる部屋の上 2 行と同じ）
				cells[(r.y + r.h - 2) * W + x] = CELL_TOP;
				cells[(r.y + r.h - 1) * W + x] = CELL_FACE;
			}
		}
	}
	// 床
	for (size_t i = 0; i < c.rooms.size(); ++i)
	{
		const Room &r = c.rooms[i];
		for (int y = IY0(r); y <= IY1(r); ++y)
			for (int x = IX0(r); x <= IX1(r); ++x)
				cells[y * W + x] = CELL_FLOOR;
	}
	// 固定パターンの部屋（範囲ぜんぶをパターンのマスにする）
	for (size_t i = 0; i < c.rooms.size(); ++i)
	{
		const Room &r = c.rooms[i];
		if (!r.fixed) continue;
		for (int y = r.y; y < r.y + r.h; ++y)
			for (int x = r.x; x < r.x + r.w; ++x)
				cells[y * W + x] = CELL_PATTERN;
	}
	// 出入口（固定パターンの壁にも穴を開ける）
	for (size_t i = 0; i < c.links.size(); ++i)
	{
		const Link &l = c.links[i];
		for (int y = l.y; y < l.y + l.h; ++y)
			for (int x = l.x; x < l.x + l.w; ++x)
				cells[y * W + x] = l.type == 2 ? CELL_DOOR : CELL_ARCH;
	}
}

// 歩ける所がすべてつながっているか
bool AllConnected(const std::vector<int> &cells, const std::vector<Room> &rooms, int W, int H, int sx, int sy, int &walkCount)
{
	walkCount = 0;
	for (int i = 0; i < W * H; ++i) if (WalkableAt(cells, W, H, rooms, i % W, i / W)) ++walkCount;
	std::vector<char> seen(W * H, 0);
	std::vector<int> st;
	st.push_back(sy * W + sx);
	seen[sy * W + sx] = 1;
	int n = 0;
	while (!st.empty())
	{
		int idx = st.back(); st.pop_back();
		++n;
		int x = idx % W, y = idx / W;
		static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + nb[k][0], ny = y + nb[k][1];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			int ni = ny * W + nx;
			if (!seen[ni] && WalkableAt(cells, W, H, rooms, nx, ny)) { seen[ni] = 1; st.push_back(ni); }
		}
	}
	return n == walkCount;
}

} // namespace

Err GenerateCathedralL1(const Params &p0, Rng &rng, Skeleton &sk, const Theme *pTheme)
{
	Params p = p0;
	if (p.w < 8 || p.w > 128 || p.h < 8 || p.h > 128) return ERR_PARAM;
	const int B = (pTheme && pTheme->bottomRows == 2) ? 2 : 1;  // 下の壁の行数
	p.roomMin = std::max(3 + B, p.roomMin);
	p.roomMax = std::min(std::max(p.roomMin, p.roomMax), std::min(p.w, p.h));
	if (p.roomMin > p.roomMax) return ERR_PARAM;
	p.archMin = std::max(1, p.archMin);
	p.archMax = std::max(p.archMin, p.archMax);
	p.floorMin = std::max(0, std::min(100, p.floorMin));
	p.doorRate = std::max(0, std::min(100, p.doorRate));
	p.openRate = std::max(0, std::min(100, p.openRate));
	p.loopRate = std::max(0, std::min(100, p.loopRate));
	p.vertRate = std::max(0, std::min(100, p.vertRate));
	p.vertDoorRate = std::max(-1, std::min(100, p.vertDoorRate));
	p.vertArchMin = std::max(0, p.vertArchMin);
	p.vertArchMax = std::max(0, p.vertArchMax);

	const int W = p.w, H = p.h;
	const int maxTries = 300;
	const std::vector<int> entryCands = FixedCands(pTheme, 2, W, H, B);
	const std::vector<int> bossCands = FixedCands(pTheme, 1, W, H, B);
	for (int tries = 1; tries <= maxTries; ++tries)
	{
		Ctx c;
		c.p = &p; c.th = pTheme; c.rng = &rng; c.W = W; c.H = H; c.aMin = p.archMin; c.aMax = p.archMax; c.B = B;

		// 最初の部屋は中央付近
		Room r0;
		r0.id = 0; r0.kind = 2; r0.tpl = -1; r0.bot = B;
		r0.w = rng.Int(p.roomMin, p.roomMax);
		r0.h = rng.Int(p.roomMin, p.roomMax);
		if (!entryCands.empty())
		{
			// 入口の固定パターン: パターンの大きさちょうどで作る
			r0.tpl = entryCands[rng.Int(0, (int)entryCands.size() - 1)];
			r0.fixed = 1;
			r0.w = pTheme->rooms[r0.tpl].w;
			r0.h = pTheme->rooms[r0.tpl].h;
		}
		r0.x = (W - r0.w) / 2 + rng.Int(-2, 2);
		r0.y = (H - r0.h) / 2 + rng.Int(-2, 2);
		r0.x = std::max(0, std::min(W - r0.w, r0.x));
		r0.y = std::max(0, std::min(H - r0.h, r0.y));
		c.rooms.push_back(r0);

		// 生やす（床が目標に届くか、置けなくなるまで）
		const int target = std::min(100, p.floorMin + rng.Int(0, 10));
		long area = (long)(r0.w - 2) * (r0.h - 2 - B);
		int fails = 0;
		while (area * 100 < (long)W * H * target && fails < 300)
		{
			int parent = rng.Int(0, (int)c.rooms.size() - 1);
			if (TryGrow(c, parent))
			{
				fails = 0;
				const Room &n = c.rooms.back();
				area += (long)(n.w - 2) * (n.h - 2 - B);
			}
			else ++fails;
		}

		// 親以外の隣の部屋ともつなぐ
		for (size_t i = 0; i < c.rooms.size(); ++i)
			for (size_t j = i + 1; j < c.rooms.size(); ++j)
			{
				bool horiz;
				int s, e, line;
				if (Linked(c, (int)i, (int)j)) continue;
				if (!Shared(c, c.rooms[i], c.rooms[j], horiz, s, e, line)) continue;
				if (rng.Chance(p.loopRate) && (horiz || VertOk(c))) MakeLink(c, (int)i, (int)j);
			}

		// 入口 = 最初の部屋、ボス部屋 = つながりで一番遠い部屋
		int boss = 0;
		for (int pass = 0; pass < 2; ++pass)
		{
			const int n = (int)c.rooms.size();
			std::vector<int> dist(n, -1), q;
			q.push_back(0); dist[0] = 0;
			boss = 0;
			for (size_t qi = 0; qi < q.size(); ++qi)
			{
				int a = q[qi];
				if (dist[a] > dist[boss]) boss = a;
				for (size_t k = 0; k < c.links.size(); ++k)
				{
					int b = -1;
					if (c.links[k].a == a) b = c.links[k].b;
					else if (c.links[k].b == a) b = c.links[k].a;
					if (b >= 0 && dist[b] < 0) { dist[b] = dist[a] + 1; q.push_back(b); }
				}
			}
			if (pass == 0 && !bossCands.empty())
			{
				// ボスの固定パターンを、入口から一番遠い部屋に隣接させて生やしてみる。置けなければ今までどおり
				Room bn;
				bn.kind = 1; bn.fixed = 1; bn.bot = B;
				bn.tpl = bossCands[rng.Int(0, (int)bossCands.size() - 1)];
				bn.w = pTheme->rooms[bn.tpl].w;
				bn.h = pTheme->rooms[bn.tpl].h;
				if (TryAttach(c, boss, bn)) continue;  // 置けたので、遠さを測り直す（新しい部屋が一番遠くなる）
			}
			break;
		}
		if (boss != 0) c.rooms[boss].kind = 1;

		std::vector<int> cells;
		BuildCells(c, cells);
		const Room &e = c.rooms[0], &g = c.rooms[boss];
		int sx = e.x + e.w / 2, sy = e.y + 2 + (e.h - 2 - B) / 2;
		if (e.fixed)
		{
			const TplRoom &t = pTheme->rooms[e.tpl];
			if (t.entryX >= 0 && t.entryX < t.w && t.entryY >= 0 && t.entryY < t.h) { sx = e.x + t.entryX; sy = e.y + t.entryY; }
		}
		int gx = g.x + g.w / 2, gy = g.y + 2 + (g.h - 2 - B) / 2;
		int walk = 0;
		if (!AllConnected(cells, c.rooms, W, H, sx, sy, walk)) continue;
		if ((long)walk * 100 < (long)W * H * p.floorMin) continue;

		sk.w = W; sk.h = H; sk.algo = 1; sk.tries = tries;
		sk.cells.swap(cells);
		sk.rooms = c.rooms;
		sk.links = c.links;
		sk.startX = sx; sk.startY = sy; sk.goalX = gx; sk.goalY = gy;
		sk.floorCount = walk;
		return ERR_NONE;
	}
	return ERR_RETRY;
}

} // namespace DungeonGen
