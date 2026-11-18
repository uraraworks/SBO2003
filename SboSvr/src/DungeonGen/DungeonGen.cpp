// 入口: 設定を確かめて骨組みを作り、テーマがあれば絵を当てる。JSON の読み書きもここ。
#include "DungeonGen.h"
#include "MiniJson.h"
#include <algorithm>
#include <ctime>
#include <random>

namespace DungeonGen
{

Err Generate(Params p, const Theme *pTheme, Skeleton &sk, std::vector<int> &parts)
{
	parts.clear();
	if (p.w < 8 || p.w > 128 || p.h < 8 || p.h > 128) return ERR_PARAM;
	int algo = (pTheme && pTheme->algo) ? pTheme->algo : p.algo;
	if (algo != 1 && algo != 3) return ERR_ALGO;
	p.algo = algo;

	uint32_t seed = p.seed;
	if (seed == 0)
	{
		std::random_device rd;
		seed = (uint32_t)rd() ^ (uint32_t)std::time(0);
		if (seed == 0) seed = 1;
	}
	Rng rng;
	rng.Seed(seed);
	sk = Skeleton();
	Err e = algo == 1 ? GenerateCathedralL1(p, rng, sk, pTheme) : GenerateCaveL3(p, rng, sk, pTheme);
	if (e != ERR_NONE) return e;
	sk.seed = seed;
	if (pTheme) ApplyTheme(*pTheme, sk, rng, parts);
	return ERR_NONE;
}

namespace
{

void ReadInts(const JVal *v, std::vector<int> &out)
{
	out.clear();
	if (!v || v->t != JVal::ARR) return;
	for (size_t i = 0; i < v->a.size(); ++i)
		if (v->a[i].t == JVal::NUM) out.push_back((int)v->a[i].n);
}

int ReadPair(const JVal *v, int idx, int def)
{
	if (v && v->t == JVal::ARR && (int)v->a.size() > idx && v->a[idx].t == JVal::NUM) return (int)v->a[idx].n;
	return def;
}

void ReadParams(const JVal *j, Params &p)
{
	if (!j || j->t != JVal::OBJ) return;
	p.w = j->Int("w", p.w);
	p.h = j->Int("h", p.h);
	const JVal *s = j->Get("seed");
	if (s && s->t == JVal::NUM) p.seed = (uint32_t)(long long)s->n;
	p.algo = j->Int("algo", p.algo);
	p.roomMin = j->Int("roomMin", p.roomMin);
	p.roomMax = j->Int("roomMax", p.roomMax);
	p.floorMin = j->Int("floorMin", p.floorMin);
	p.archMin = j->Int("archMin", p.archMin);
	p.archMax = j->Int("archMax", p.archMax);
	p.doorRate = j->Int("doorRate", p.doorRate);
	p.openRate = j->Int("openRate", p.openRate);
	p.loopRate = j->Int("loopRate", p.loopRate);
	p.cutoff = j->Int("cutoff", p.cutoff);
	p.vertRate = j->Int("vertRate", p.vertRate);
	p.vertDoorRate = j->Int("vertDoorRate", p.vertDoorRate);
	p.vertArchMin = j->Int("vertArchMin", p.vertArchMin);
	p.vertArchMax = j->Int("vertArchMax", p.vertArchMax);
}

void ReadTheme(const JVal &j, Theme &t)
{
	t.algo = j.Int("algo", 0);
	ReadInts(j.Get("floor"), t.floor);
	ReadInts(j.Get("wallTop"), t.wallTop);
	ReadInts(j.Get("wallFace"), t.wallFace);
	ReadInts(j.Get("wallFill"), t.wallFill);
	ReadInts(j.Get("arch"), t.arch);
	t.voidPart = j.Int("void", 0);
	t.door = j.Int("door", 0);
	t.bottomRows = j.Int("bottomRows", 1) == 2 ? 2 : 1;
	const JVal *rooms = j.Get("rooms");
	if (rooms && rooms->t == JVal::ARR)
		for (size_t i = 0; i < rooms->a.size(); ++i)
		{
			const JVal &r = rooms->a[i];
			TplRoom tr;
			const JVal *nm = r.Get("name");
			if (nm && nm->t == JVal::STR) tr.name = nm->s;
			tr.kind = r.Int("kind", 0);
			tr.w = r.Int("w", 0);
			tr.h = r.Int("h", 0);
			ReadInts(r.Get("cells"), tr.cells);
			tr.cutX[0] = ReadPair(r.Get("cutX"), 0, 0);
			tr.cutX[1] = ReadPair(r.Get("cutX"), 1, tr.w);
			tr.cutY[0] = ReadPair(r.Get("cutY"), 0, 0);
			tr.cutY[1] = ReadPair(r.Get("cutY"), 1, tr.h);
			tr.fixed = r.Int("fixed", 0) != 0;
			tr.entryX = r.Int("entryX", -1);
			tr.entryY = r.Int("entryY", -1);
			t.rooms.push_back(tr);
			// 9分割の部屋の下の切れ目から下が 2 行以上なら、下の壁は 2 行（天井＋側面）として作る。
			// 1 行のままだと上下の部屋の重なりが 1 行になり、つなぎ目の壁が途切れるため。
			if (!tr.fixed && tr.h - tr.cutY[1] >= 2) t.bottomRows = 2;
		}
	const JVal *ops = j.Get("openings");
	if (ops && ops->t == JVal::ARR)
		for (size_t i = 0; i < ops->a.size() && (int)t.openings.size() < MAX_OPENINGS; ++i)
		{
			const JVal &o = ops->a[i];
			Opening op;
			const JVal *nm = o.Get("name");
			if (nm && nm->t == JVal::STR) op.name = nm->s;
			op.dir = o.Int("dir", 0);
			op.type = o.Int("type", 1);
			op.w = o.Int("w", 0);
			op.h = o.Int("h", 0);
			ReadInts(o.Get("cells"), op.cells);
			ReadInts(o.Get("cellsOpen"), op.cellsOpen);
			op.hole0 = o.Int("hole0", 0);
			op.hole1 = o.Int("hole1", 0);
			t.openings.push_back(op);
		}
	const JVal *decos = j.Get("decos");
	if (decos && decos->t == JVal::ARR)
		for (size_t i = 0; i < decos->a.size(); ++i)
		{
			const JVal &d = decos->a[i];
			Deco dc;
			const JVal *nm = d.Get("name");
			if (nm && nm->t == JVal::STR) dc.name = nm->s;
			dc.w = d.Int("w", 1);
			dc.h = d.Int("h", 1);
			ReadInts(d.Get("cells"), dc.cells);
			dc.on = d.Int("on", 1);
			dc.rate = d.Int("rate", 10);
			dc.max = d.Int("max", 0);
			t.decos.push_back(dc);
		}
}

void Num(std::string &s, long long v)
{
	s += std::to_string(v);
}

void IntArray(std::string &s, const std::vector<int> &v)
{
	s += '[';
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i) s += ',';
		s += std::to_string(v[i]);
	}
	s += ']';
}

} // namespace

Err RunJson(const char *pszJson, std::string &out)
{
	out.clear();
	JVal root;
	if (!ParseJson(pszJson, root) || root.t != JVal::OBJ) return ERR_JSON;
	Params p;
	ReadParams(root.Get("params"), p);
	Theme th;
	const JVal *jt = root.Get("theme");
	bool hasTheme = jt && jt->t == JVal::OBJ;
	if (hasTheme) ReadTheme(*jt, th);

	Skeleton sk;
	std::vector<int> parts;
	Err e = Generate(p, hasTheme ? &th : 0, sk, parts);
	if (e != ERR_NONE) return e;

	out.reserve(sk.w * sk.h * 8 + 1024);
	out += "{\"ok\":true,\"w\":"; Num(out, sk.w);
	out += ",\"h\":"; Num(out, sk.h);
	out += ",\"seed\":"; Num(out, sk.seed);
	out += ",\"algo\":"; Num(out, sk.algo);
	out += ",\"tries\":"; Num(out, sk.tries);
	out += ",\"cells\":"; IntArray(out, sk.cells);
	out += ",\"parts\":"; IntArray(out, parts);
	out += ",\"rooms\":[";
	for (size_t i = 0; i < sk.rooms.size(); ++i)
	{
		const Room &r = sk.rooms[i];
		if (i) out += ',';
		out += "{\"id\":"; Num(out, r.id);
		out += ",\"x\":"; Num(out, r.x);
		out += ",\"y\":"; Num(out, r.y);
		out += ",\"w\":"; Num(out, r.w);
		out += ",\"h\":"; Num(out, r.h);
		out += ",\"kind\":"; Num(out, r.kind);
		out += ",\"tpl\":"; Num(out, r.tpl);
		out += ",\"fixed\":"; Num(out, r.fixed ? 1 : 0);
		out += '}';
	}
	out += "],\"links\":[";
	for (size_t i = 0; i < sk.links.size(); ++i)
	{
		const Link &l = sk.links[i];
		if (i) out += ',';
		out += "{\"a\":"; Num(out, l.a);
		out += ",\"b\":"; Num(out, l.b);
		out += ",\"x\":"; Num(out, l.x);
		out += ",\"y\":"; Num(out, l.y);
		out += ",\"w\":"; Num(out, l.w);
		out += ",\"h\":"; Num(out, l.h);
		out += ",\"type\":"; Num(out, l.type);
		out += ",\"tpl\":"; Num(out, l.tpl);
		out += '}';
	}
	out += "],\"start\":{\"x\":"; Num(out, sk.startX);
	out += ",\"y\":"; Num(out, sk.startY);
	out += "},\"goal\":{\"x\":"; Num(out, sk.goalX);
	out += ",\"y\":"; Num(out, sk.goalY);
	out += "},\"floorCount\":"; Num(out, sk.floorCount);
	out += '}';
	return ERR_NONE;
}

} // namespace DungeonGen
