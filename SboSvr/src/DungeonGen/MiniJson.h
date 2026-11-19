#pragma once
// 小さな JSON 読み取り（オブジェクト・配列・数値・文字列・true/false/null）
#include <string>
#include <utility>
#include <vector>

namespace DungeonGen
{

struct JVal
{
	enum Type { NUL, BOOL_, NUM, STR, ARR, OBJ };
	Type t;
	double n;
	bool b;
	std::string s;
	std::vector<JVal> a;
	std::vector<std::pair<std::string, JVal> > o;

	JVal() : t(NUL), n(0), b(false) {}
	const JVal *Get(const char *key) const
	{
		if (t != OBJ) return 0;
		for (size_t i = 0; i < o.size(); i++)
			if (o[i].first == key) return &o[i].second;
		return 0;
	}
	// 数値（無い・数値でない時は def）
	int Int(const char *key, int def) const
	{
		const JVal *v = Get(key);
		if (!v) return def;
		if (v->t == NUM) return (int)v->n;
		if (v->t == BOOL_) return v->b ? 1 : 0;
		return def;
	}
};

// 読めたら true。深さ 64 まで
bool ParseJson(const char *psz, JVal &out);

// JSON の文字列にして out の後ろへ足す（空白なし・キーの順はそのまま。数は整数なら整数、そうでなければ %.17g）。
// ParseJson で読み直すと同じ中身になる。同じ中身なら同じ文字列になるので、写しの比べ・使い回しに使える
void WriteJson(const JVal &v, std::string &out);

} // namespace DungeonGen
