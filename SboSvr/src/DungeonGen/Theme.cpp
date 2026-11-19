// 絵を当てる処理（テーマ）
//   1. 素の絵: セルの値 → パーツ番号（床・壁はくじで混ぜる）
//   2. 9分割の部屋（L1 のみ）: 部屋ごとにくじで選び、外枠いっぱいに当てる
//   3. 出入口・扉を上から置く
//   4. 飾り（decos）
// くじはすべて渡された乱数（骨組みと同じシード）から引く。
#include "DungeonGen.h"
#include <algorithm>

namespace DungeonGen
{
namespace
{

bool inPartList(const std::vector<int> &v, int n)
{
	for (size_t k = 0; k < v.size(); ++k) if (v[k] == n) return true;
	return false;
}

int Pick(const std::vector<int> &v, Rng &rng)
{
	if (v.empty()) return 0;
	return v[rng.Int(0, (int)v.size() - 1)];
}

// 9分割の部屋として使えるか
bool TplValid(const TplRoom &t)
{
	if (t.w <= 0 || t.h <= 0 || (int)t.cells.size() != t.w * t.h) return false;
	if (t.cutX[0] < 0 || t.cutX[0] > t.cutX[1] || t.cutX[1] > t.w) return false;
	if (t.cutY[0] < 0 || t.cutY[0] > t.cutY[1] || t.cutY[1] > t.h) return false;
	return true;
}

// 部屋の大きさ rw x rh に当てられるか（四隅が収まり、繰り返す真ん中があること）
bool TplFits(const TplRoom &t, int rw, int rh)
{
	int left = t.cutX[0], right = t.w - t.cutX[1], top = t.cutY[0], bottom = t.h - t.cutY[1];
	if (rw < left + right || rh < top + bottom) return false;
	if (rw > left + right && t.cutX[1] - t.cutX[0] <= 0) return false;
	if (rh > top + bottom && t.cutY[1] - t.cutY[0] <= 0) return false;
	return true;
}

// 部屋の中の位置 i（0..rw-1）を、テンプレートの列に対応づける
int MapAxis(int i, int rw, int tw, int cut0, int cut1)
{
	int left = cut0, right = tw - cut1;
	if (i < left) return i;
	if (i >= rw - right) return cut1 + (i - (rw - right));
	return cut0 + (i - left) % (cut1 - cut0);
}

} // namespace

void ApplyTheme(const Theme &th, Skeleton &sk, Rng &rng, std::vector<int> &parts)
{
	const int W = sk.w, H = sk.h;
	parts.assign(W * H, 0);

	// 1. 素の絵
	for (int i = 0; i < W * H; ++i)
	{
		switch (sk.cells[i])
		{
		case CELL_FLOOR: parts[i] = Pick(th.floor, rng); break;
		case CELL_TOP: parts[i] = Pick(th.wallTop, rng); break;
		case CELL_FACE: parts[i] = Pick(th.wallFace, rng); break;
		case CELL_WALL: parts[i] = Pick(th.wallFill, rng); break;
		case CELL_NONE: parts[i] = th.voidPart; break;
		case CELL_PATTERN: parts[i] = th.voidPart; break;  // 固定パターンで上書きする
		default: break;  // 出入口・扉はあとで
		}
	}

	// 2. 9分割の部屋
	if (sk.algo == 1)
	{
		for (size_t ri = 0; ri < sk.rooms.size(); ++ri)
		{
			Room &r = sk.rooms[ri];
			if (r.fixed) continue;  // 固定パターンは下で置く
			std::vector<int> cand;
			for (size_t k = 0; k < th.rooms.size(); ++k)
				if (th.rooms[k].kind == r.kind && !th.rooms[k].fixed && TplValid(th.rooms[k])) cand.push_back((int)k);
			if (cand.empty() && r.kind != 0)  // 入口・ボス用の 9分割が無ければ、ふつうの部屋の絵を使う
				for (size_t k = 0; k < th.rooms.size(); ++k)
					if (th.rooms[k].kind == 0 && !th.rooms[k].fixed && TplValid(th.rooms[k])) cand.push_back((int)k);
			if (cand.empty()) continue;
			int ti = cand[rng.Int(0, (int)cand.size() - 1)];
			const TplRoom &t = th.rooms[ti];
			if (!TplFits(t, r.w, r.h)) continue;  // 小さい部屋は素の絵のまま
			r.tpl = ti;
			// 入口の部屋の 9分割に入口の印があれば、伸ばした部屋の同じ所を start にする（床の時だけ）
			if (r.kind == 2 && t.entryX >= 0 && t.entryY >= 0 && t.entryX < t.w && t.entryY < t.h)
			{
				auto place = [](int e, int rw, int tw, int c0, int c1) {
					if (e < c0) return e;
					if (e >= c1) return rw - (tw - e);
					int mid = rw - c0 - (tw - c1);  // 伸ばした真ん中の長さ
					return c0 + (int)((long)(e - c0) * mid / (c1 - c0)) + (mid > (c1 - c0) ? (mid - (c1 - c0)) / 2 : 0);
				};
				int ex = r.x + place(t.entryX, r.w, t.w, t.cutX[0], t.cutX[1]);
				int ey = r.y + place(t.entryY, r.h, t.h, t.cutY[0], t.cutY[1]);
				if (ex >= 0 && ey >= 0 && ex < W && ey < H && sk.cells[ey * W + ex] == CELL_FLOOR) { sk.startX = ex; sk.startY = ey; }
			}
			for (int j = 0; j < r.h; ++j)
			{
				int tj = MapAxis(j, r.h, t.h, t.cutY[0], t.cutY[1]);
				for (int i = 0; i < r.w; ++i)
				{
					int ti2 = MapAxis(i, r.w, t.w, t.cutX[0], t.cutX[1]);
					int v = t.cells[tj * t.w + ti2];
					if (v != 0) parts[(r.y + j) * W + r.x + i] = v;
				}
			}
		}
	}

	// 2b. 固定パターン（L1・L3 とも。伸ばさず、そのまま置く）
	for (size_t ri = 0; ri < sk.rooms.size(); ++ri)
	{
		const Room &r = sk.rooms[ri];
		if (!r.fixed || r.tpl < 0 || r.tpl >= (int)th.rooms.size()) continue;
		const TplRoom &t = th.rooms[r.tpl];
		if (t.w != r.w || t.h != r.h || (int)t.cells.size() != t.w * t.h) continue;
		for (int j = 0; j < r.h; ++j)
			for (int i = 0; i < r.w; ++i)
			{
				int v = t.cells[j * t.w + i];
				if (v != 0) parts[(r.y + j) * W + r.x + i] = v;
			}
	}

	// 2c. 部屋どうしが重なった壁（つなぎ目）の絵をそろえる（L1 のみ）
	//   部屋は順に上書きで描くので、重なった所には後の部屋の縁が残り、絵がちぐはぐになる。
	//   2 部屋以上が重なった壁のマスは、どの部屋の床に面しているか（役目）で持ち主を決め、
	//   その部屋の 9分割の絵の同じ位置を描き直す。役目は強い順に:
	//     1. 側面   : 下が床        → 下の床の部屋の上から 2 行目（側面の行）
	//     1a. 下が出入口 → 側面（wallFace。縦の壁をくり抜いた出入口の上）
	//     1b. 縦の壁 : どれかの部屋の左右の列 → その部屋の列（角・柱が横の壁で途切れないように）
	//     2. 上の縁 : 2 つ下が床    → 2 つ下の床の部屋の 1 行目（天井の縁）
	//     3. 横の壁 : 左か右が床    → その床の部屋の左右の列
	//     5. 下の縁 : 上か 2 つ上が床 → その床の部屋の下の壁の行
	//   どれでもない（壁の角・壁どうしが接する所）は天井（wallTop）。
	if (sk.algo == 1 && !th.wallTop.empty())
	{
		std::vector<unsigned char> cover(W * H, 0);
		std::vector<int> owner(W * H, -1);  // 床のマス → その床を中に持つ部屋
		for (size_t ri = 0; ri < sk.rooms.size(); ++ri)
		{
			const Room &r = sk.rooms[ri];
			for (int j = 0; j < r.h; ++j)
				for (int i = 0; i < r.w; ++i)
				{
					int x = r.x + i, y = r.y + j;
					if (x < 0 || y < 0 || x >= W || y >= H) continue;
					if (cover[y * W + x] < 255) ++cover[y * W + x];
					if (i >= 1 && i <= r.w - 2 && j >= 2 && j <= r.h - 1 - r.bot && sk.cells[y * W + x] == CELL_FLOOR)
						owner[y * W + x] = (int)ri;
				}
		}
		auto own = [&](int x, int y) { return (x < 0 || y < 0 || x >= W || y >= H) ? -1 : owner[y * W + x]; };
		// 部屋 ri の 9分割の絵で、マップの (x,y) に当たる絵（範囲外・素の絵の部屋は -1）
		auto tplAt = [&](int ri, int x, int y) {
			const Room &r = sk.rooms[ri];
			if (r.fixed || r.tpl < 0 || r.tpl >= (int)th.rooms.size()) return -1;
			const TplRoom &t = th.rooms[r.tpl];
			int i = x - r.x, j = y - r.y;
			if (i < 0 || j < 0 || i >= r.w || j >= r.h) return -1;
			int v = t.cells[MapAxis(j, r.h, t.h, t.cutY[0], t.cutY[1]) * t.w + MapAxis(i, r.w, t.w, t.cutX[0], t.cutX[1])];
			return v != 0 ? v : -1;
		};
		// (x,y) が左右の列（縦の壁）に当たる部屋。9分割の絵がある部屋を優先
		auto sideRoom = [&](int x, int y) {
			int found = -1;
			for (size_t k = 0; k < sk.rooms.size(); ++k)
			{
				const Room &r = sk.rooms[k];
				if (y < r.y || y >= r.y + r.h - r.bot || (x != r.x && x != r.x + r.w - 1)) continue;  // 下の壁の行は横の壁なので除く
				if (tplAt((int)k, x, y) > 0) return (int)k;
				if (found < 0) found = (int)k;
			}
			return found;
		};
		for (int y = 0; y < H; ++y)
			for (int x = 0; x < W; ++x)
			{
				int idx = y * W + x, c = sk.cells[idx];
				if (cover[idx] < 2 || (c != CELL_TOP && c != CELL_FACE && c != CELL_WALL)) continue;
				int ri = -1, v = -1;
				bool face = false;
				if ((ri = own(x, y + 1)) >= 0) face = true;
				else if (y + 1 < H && IsWalkable(sk.cells[(y + 1) * W + x])) face = true;  // 下が出入口（縦の壁をくり抜いた上は側面）
				else if ((ri = sideRoom(x, y)) >= 0) {}
				else if ((ri = own(x, y + 2)) >= 0 || (ri = own(x - 1, y)) >= 0 || (ri = own(x + 1, y)) >= 0) {}
				else if ((ri = own(x, y - 1)) >= 0 || (ri = own(x, y - 2)) >= 0) {}
				if (ri >= 0) v = tplAt(ri, x, y);
				if (v <= 0) v = (face && !th.wallFace.empty()) ? Pick(th.wallFace, rng) : Pick(th.wallTop, rng);  // 素の絵の部屋
				if (v > 0) parts[idx] = v;
			}
	}

	// 3. 出入口・扉
	const std::vector<int> &archVParts = th.archV.empty() ? th.arch : th.archV;  // 縦の出入口の床
	for (size_t k = 0; k < sk.links.size(); ++k)
	{
		const Link &l = sk.links[k];
		const std::vector<int> &archP = l.vert ? archVParts : th.arch;
		for (int y = l.y; y < l.y + l.h; ++y)
			for (int x = l.x; x < l.x + l.w; ++x)
			{
				int idx = y * W + x;
				if (sk.cells[idx] == CELL_DOOR)
				{
					// 横の壁の扉は 2 行（天井＋側面）を抜くが、扉の絵は下の側面の行だけ。上の天井の行は出入口の床
					if (th.door && !l.vert && l.h == 2 && y == l.y && l.tpl < 0) parts[idx] = archP.empty() ? Pick(th.floor, rng) : Pick(archP, rng);
					else if (th.door) parts[idx] = th.door;
					else parts[idx] = archP.empty() ? Pick(th.floor, rng) : Pick(archP, rng);
				}
				else parts[idx] = archP.empty() ? Pick(th.floor, rng) : Pick(archP, rng);
			}
		// 出入口パターン: 両脇の柱・枠も含めて cells で上書き（扉の開いた絵 cellsOpen は使わない）
		//   伸ばすパターン（cut0/cut1）は、置いた範囲 px,py,pw,ph の向きに沿った軸を部屋の 9分割と同じ対応づけで写す。
		//   壁の行・列の外にはみ出した部分も絵だけ描く（骨組みは変えない）
		if (l.tpl >= 0 && l.tpl < (int)th.openings.size() && th.openings[l.tpl].Valid())
		{
			const Opening &o = th.openings[l.tpl];
			if (l.px < 0 || l.py < 0 || l.px + l.pw > W || l.py + l.ph > H) continue;
			for (int j = 0; j < l.ph; ++j)
				for (int i = 0; i < l.pw; ++i)
				{
					int ti = i, tj = j;
					if (o.Stretch())
					{
						if (o.dir == 0) ti = MapAxis(i, l.pw, o.w, o.cut0, o.cut1);
						else tj = MapAxis(j, l.ph, o.h, o.cut0, o.cut1);
					}
					int v = o.cells[tj * o.w + ti];
					if (v != 0) parts[(l.py + j) * W + l.px + i] = v;
				}
		}
	}

	// 3b. 入口・ゴールの絵（start・goal のマスに 1 枚だけ）
	const bool hasS = sk.startX >= 0 && sk.startY >= 0 && sk.startX < W && sk.startY < H;
	const bool hasG = sk.goalX >= 0 && sk.goalY >= 0 && sk.goalX < W && sk.goalY < H;
	if (th.entryPart > 0 && hasS) parts[sk.startY * W + sk.startX] = th.entryPart;
	if (th.goalPart > 0 && hasG) parts[sk.goalY * W + sk.goalX] = th.goalPart;

	// 4. 飾り（L3 洞窟は素の絵だけ）
	if (sk.algo != 1) return;
	std::vector<char> used(W * H, 0);
	if (hasS) used[sk.startY * W + sk.startX] = 1;  // 入口・ゴールには飾りを置かない
	if (hasG) used[sk.goalY * W + sk.goalX] = 1;
	// 出入口パターンの範囲（柱・枠）には飾りを置かない
	for (size_t k = 0; k < sk.links.size(); ++k)
	{
		const Link &l = sk.links[k];
		if (l.tpl < 0 || l.tpl >= (int)th.openings.size() || !th.openings[l.tpl].Valid()) continue;
		for (int j = 0; j < l.ph; ++j)
			for (int i = 0; i < l.pw; ++i)
				if (l.px + i >= 0 && l.px + i < W && l.py + j >= 0 && l.py + j < H) used[(l.py + j) * W + l.px + i] = 1;
	}
	for (size_t di = 0; di < th.decos.size(); ++di)
	{
		const Deco &d = th.decos[di];
		if (d.w <= 0 || d.h <= 0 || (int)d.cells.size() != d.w * d.h) continue;
		if (d.on != 1 && d.on != 3) continue;
		std::vector<int> cand;  // 左上の位置
		if (d.on == 1)
		{
			// 部屋の床。まわり 1 マスも床（壁から 1 マス離す）。L1 だけでなく L3 でも床のセルを見る
			for (int y = 1; y + d.h < H; ++y)
				for (int x = 1; x + d.w < W; ++x)
				{
					bool ok = true;
					for (int yy = y - 1; yy <= y + d.h && ok; ++yy)
						for (int xx = x - 1; xx <= x + d.w; ++xx)
						{
							int c = sk.cells[yy * W + xx];
							if (c != CELL_FLOOR || used[yy * W + xx]) { ok = false; break; }
						}
					if (ok) cand.push_back(y * W + x);
				}
		}
		else
		{
			// 壁の側面。いちばん下の行が側面で、その下が床。上の行は壁
			for (int y = d.h - 1; y + 1 < H; ++y)
				for (int x = 0; x + d.w <= W; ++x)
				{
					bool ok = true;
					for (int xx = x; xx < x + d.w && ok; ++xx)
						if (sk.cells[y * W + xx] != CELL_FACE || sk.cells[(y + 1) * W + xx] != CELL_FLOOR) ok = false;
					for (int yy = y - d.h + 1; yy < y && ok; ++yy)
						for (int xx = x; xx < x + d.w; ++xx)
						{
							int c = sk.cells[yy * W + xx];
							if (c != CELL_TOP && c != CELL_FACE) { ok = false; break; }
						}
					// 9分割の部屋の絵で床・出入口の床に描かれたマス（壁が 1 行の部屋の 2 行目など）には置かない
					for (int yy = y - d.h + 1; yy <= y && ok; ++yy)
						for (int xx = x; xx < x + d.w; ++xx)
						{
							int v = parts[yy * W + xx];
							if (inPartList(th.floor, v) || inPartList(th.arch, v) || inPartList(th.archV, v)) { ok = false; break; }
						}
					for (int yy = y - d.h + 1; yy <= y && ok; ++yy)
						for (int xx = x; xx < x + d.w; ++xx)
							if (used[yy * W + xx]) { ok = false; break; }
					if (ok) cand.push_back((y - d.h + 1) * W + x);
				}
		}
		for (int i = (int)cand.size() - 1; i > 0; --i)
			std::swap(cand[i], cand[rng.Int(0, i)]);
		int placed = 0;
		for (size_t k = 0; k < cand.size(); ++k)
		{
			if (d.max > 0 && placed >= d.max) break;
			int x = cand[k] % W, y = cand[k] / W;
			// 先に置いた飾りと重ならないか（候補を作った後に置かれたもの）
			bool ok = true;
			for (int yy = y; yy < y + d.h && ok; ++yy)
				for (int xx = x; xx < x + d.w; ++xx)
					if (used[yy * W + xx]) { ok = false; break; }
			if (!ok || !rng.Chance(d.rate)) continue;
			for (int yy = 0; yy < d.h; ++yy)
				for (int xx = 0; xx < d.w; ++xx)
				{
					used[(y + yy) * W + x + xx] = 1;
					int v = d.cells[yy * d.w + xx];
					if (v != 0) parts[(y + yy) * W + x + xx] = v;
				}
			++placed;
		}
	}
}

} // namespace DungeonGen
