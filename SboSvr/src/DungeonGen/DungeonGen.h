#pragma once
// ---------------------------------------------------------------------------
// DungeonGen: ディアブロ1式のランダムダンジョン生成（ゲーム本体に依存しない純粋な C++）
//   1. 骨組み（Generate*）: 床・壁・出入口の意味だけの格子と、部屋・つながりの一覧
//   2. 絵を当てる（ApplyTheme）: テーマで格子をマップパーツ番号に置き換える
// 乱数は std::mt19937。シードが同じなら（同じビルドで）同じ結果になる。
// 仕様: docs/dungeon-gen.md
// ---------------------------------------------------------------------------
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace DungeonGen
{

// セルの値
enum Cell
{
	CELL_NONE = 0,   // 何もない
	CELL_FLOOR = 1,  // 床
	CELL_TOP = 2,    // 壁の天井
	CELL_FACE = 3,   // 壁の側面
	CELL_WALL = 4,   // 壁（その他）
	CELL_ARCH = 5,   // 出入口の床
	CELL_DOOR = 6,   // 扉
	CELL_PATTERN = 7 // 固定パターンの部屋の中（床か壁かは分からない。theme がある時だけ）
};

inline bool IsWalkable(int c) { return c == CELL_FLOOR || c == CELL_ARCH || c == CELL_DOOR; }

// 乱数（実装によらず同じ結果になるよう、分布クラスは使わない）
struct Rng
{
	std::mt19937 eng;
	void Seed(uint32_t s) { eng.seed(s); }
	int Int(int lo, int hi) { return hi <= lo ? lo : lo + (int)(eng() % (uint32_t)(hi - lo + 1)); }
	bool Chance(int percent) { return Int(0, 99) < percent; }
};

struct Params
{
	int algo;       // 1 = L1 聖堂、3 = L3 洞窟（テーマがあればテーマの algo が優先）
	int w, h;
	uint32_t seed;  // 0 = ランダム
	int roomMin, roomMax, floorMin, archMin, archMax, doorRate, openRate, loopRate, cutoff;
	int vertRate, vertDoorRate, vertArchMin, vertArchMax;  // L1 縦の壁（左右に並ぶ部屋の共有壁）の出入口。vertDoorRate -1・vertArchMin/Max 0 = 横と同じ
	Params() : algo(1), w(40), h(40), seed(0), roomMin(6), roomMax(12), floorMin(35),
		archMin(2), archMax(4), doorRate(20), openRate(8), loopRate(30), cutoff(25),
		vertRate(100), vertDoorRate(-1), vertArchMin(0), vertArchMax(0) {}
};

struct Room
{
	int id, x, y, w, h;
	int kind;  // 0 ふつう / 1 ボス部屋 / 2 入口の部屋
	int tpl;   // 使った 9分割の部屋の番号（-1 = 素の絵）。fixed の時は置いた固定パターンの番号
	int bot;   // 下の壁の行数（1 か 2。L1 のテーマの bottomRows）。L3 は 1
	int fixed; // 0 = ふつう / 1 = 固定パターン（L1。外周は通れない扱い） / 2 = 固定パターン（L3。外周は隣が床なら出入口）
	Room() : id(0), x(0), y(0), w(0), h(0), kind(0), tpl(-1), bot(1), fixed(0) {}
};



// 固定パターン（rooms[].fixed）の範囲のセルが歩けるか。
//   パターンの中（外周を除く）は 1 つの床のかたまりとみなして歩ける。
//   外周は、fixed == 2（L3）の時だけ、外側の隣が歩ける床なら出入口として歩ける。
inline bool WalkableAt(const std::vector<int> &cells, int W, int H, const std::vector<Room> &rooms, int x, int y)
{
	int c = cells[y * W + x];
	if (IsWalkable(c)) return true;
	if (c != CELL_PATTERN) return false;
	for (size_t i = 0; i < rooms.size(); ++i)
	{
		const Room &r = rooms[i];
		if (!r.fixed || x < r.x || x >= r.x + r.w || y < r.y || y >= r.y + r.h) continue;
		bool ring = x == r.x || x == r.x + r.w - 1 || y == r.y || y >= r.y + r.h - r.bot;
		if (!ring) return true;
		if (r.fixed != 2) return false;
		static const int nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + nb[k][0], ny = y + nb[k][1];
			if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
			if (nx >= r.x && nx < r.x + r.w && ny >= r.y && ny < r.y + r.h) continue;
			if (IsWalkable(cells[ny * W + nx])) return true;
		}
		return false;
	}
	return false;
}

struct Link
{
	int a, b, x, y, w, h;
	int type;  // 1 アーチ / 2 扉 / 3 壁ごと抜いた
	int tpl;   // 使った出入口パターンの番号（-1 = 無し）。x,y,w,h は穴の範囲
	Link() : a(0), b(0), x(0), y(0), w(0), h(0), type(1), tpl(-1) {}
};

struct Skeleton
{
	int w, h, algo;
	uint32_t seed;
	int tries;
	std::vector<int> cells;
	std::vector<Room> rooms;
	std::vector<Link> links;
	int startX, startY, goalX, goalY;
	int floorCount;
	Skeleton() : w(0), h(0), algo(0), seed(0), tries(0), startX(0), startY(0), goalX(0), goalY(0), floorCount(0) {}
};

// テーマ
struct TplRoom
{
	std::string name;
	int kind, w, h;
	std::vector<int> cells;
	int cutX[2], cutY[2];
	bool fixed;           // true なら 9分割で伸ばさず cells をそのまま置く
	int entryX, entryY;   // パターン内の出てくる位置（-1 = 無し）
	TplRoom() : kind(0), w(0), h(0), fixed(false), entryX(-1), entryY(-1) { cutX[0] = cutX[1] = cutY[0] = cutY[1] = 0; }
};

struct Deco
{
	std::string name;
	int w, h;
	std::vector<int> cells;
	int on, rate, max;
	Deco() : w(1), h(1), on(1), rate(10), max(0) {}
};

// 出入口パターン（theme.openings。最大 50 件）
const int MAX_OPENINGS = 50;
struct Opening
{
	std::string name;
	int dir;    // 0 = 横の壁（天井＋側面の 2 行） / 1 = 縦の壁（1 列）
	int type;   // 1 アーチ / 2 扉 / 3 壁ごと抜く
	int w, h;
	std::vector<int> cells, cellsOpen;  // cellsOpen は扉だけ（無ければ空）
	int hole0, hole1;  // 通れる範囲。dir0 なら列 hole0..hole1-1、dir1 なら行 hole0..hole1-1
	Opening() : dir(0), type(1), w(0), h(0), hole0(0), hole1(0) {}
	bool Valid() const
	{
		if (dir != 0 && dir != 1) return false;
		if (type < 1 || type > 3) return false;
		if (dir == 0 ? h != 2 : w != 1) return false;
		if (w <= 0 || h <= 0 || (int)cells.size() != w * h) return false;
		if (!cellsOpen.empty() && (type != 2 || (int)cellsOpen.size() != w * h)) return false;
		int len = dir == 0 ? w : h;
		return hole0 >= 0 && hole0 < hole1 && hole1 <= len;
	}
};

struct Theme
{
	int algo;
	std::vector<int> floor, wallTop, wallFace, wallFill, arch;
	int voidPart, door;
	int entryPart, goalPart;  // start・goal のマスに置く絵（0 = 置かない）
	int bottomRows;  // L1: 部屋の外枠の下の壁の行数（1 = 天井だけ / 2 = 天井＋側面）
	std::vector<TplRoom> rooms;
	std::vector<Deco> decos;
	std::vector<Opening> openings;
	Theme() : algo(0), voidPart(0), door(0), entryPart(0), goalPart(0), bottomRows(1) {}
};

// エラーコード（0 = 成功）
enum Err
{
	ERR_NONE = 0,
	ERR_PARAM = 1,    // 設定が範囲外
	ERR_RETRY = 2,    // 作り直しの上限を超えた
	ERR_JSON = 3,     // 入力の JSON が読めない
	ERR_ALGO = 4      // 未対応の作り方
};

// 骨組み。rng は呼び出し側で seed 済みのもの
// pTheme があれば、kind が 2（入口）・1（ボス）で fixed の部屋パターンを置く
Err GenerateCathedralL1(const Params &p, Rng &rng, Skeleton &sk, const Theme *pTheme = 0);
Err GenerateCaveL3(const Params &p, Rng &rng, Skeleton &sk, const Theme *pTheme = 0);

// 絵を当てる。parts は w*h 個
void ApplyTheme(const Theme &th, Skeleton &sk, Rng &rng, std::vector<int> &parts);

// 設定を確かめて骨組みを作る（seed が 0 ならランダムに決め、sk.seed に入れる）。
// pTheme があれば絵も当てる（parts が空でなくなる）
Err Generate(Params p, const Theme *pTheme, Skeleton &sk, std::vector<int> &parts);

// 入力 JSON {"params":{...},"theme":{...}} から結果の JSON を作る。失敗時は out は空で Err を返す
Err RunJson(const char *pszJson, std::string &out);

} // namespace DungeonGen
