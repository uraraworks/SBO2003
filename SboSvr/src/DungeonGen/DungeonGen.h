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
	CELL_DOOR = 6    // 扉
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
	Params() : algo(1), w(40), h(40), seed(0), roomMin(6), roomMax(12), floorMin(35),
		archMin(2), archMax(4), doorRate(20), openRate(8), loopRate(30), cutoff(25) {}
};

struct Room
{
	int id, x, y, w, h;
	int kind;  // 0 ふつう / 1 ボス部屋 / 2 入口の部屋
	int tpl;   // 使った 9分割の部屋の番号（-1 = 素の絵）
};

struct Link
{
	int a, b, x, y, w, h;
	int type;  // 1 アーチ / 2 扉 / 3 壁ごと抜いた
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
	TplRoom() : kind(0), w(0), h(0) { cutX[0] = cutX[1] = cutY[0] = cutY[1] = 0; }
};

struct Deco
{
	std::string name;
	int w, h;
	std::vector<int> cells;
	int on, rate, max;
	Deco() : w(1), h(1), on(1), rate(10), max(0) {}
};

struct Theme
{
	int algo;
	std::vector<int> floor, wallTop, wallFace, wallFill, arch;
	int voidPart, door;
	std::vector<TplRoom> rooms;
	std::vector<Deco> decos;
	Theme() : algo(0), voidPart(0), door(0) {}
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
Err GenerateCathedralL1(const Params &p, Rng &rng, Skeleton &sk);
Err GenerateCaveL3(const Params &p, Rng &rng, Skeleton &sk);

// 絵を当てる。parts は w*h 個
void ApplyTheme(const Theme &th, Skeleton &sk, Rng &rng, std::vector<int> &parts);

// 設定を確かめて骨組みを作る（seed が 0 ならランダムに決め、sk.seed に入れる）。
// pTheme があれば絵も当てる（parts が空でなくなる）
Err Generate(Params p, const Theme *pTheme, Skeleton &sk, std::vector<int> &parts);

// 入力 JSON {"params":{...},"theme":{...}} から結果の JSON を作る。失敗時は out は空で Err を返す
Err RunJson(const char *pszJson, std::string &out);

} // namespace DungeonGen
