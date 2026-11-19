#include "MiniJson.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace DungeonGen
{
namespace
{

struct Reader
{
	const char *p;
	bool bad;
	Reader(const char *s) : p(s), bad(false) {}

	void Ws() { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++; }

	static void AppendUtf8(std::string &s, unsigned cp)
	{
		if (cp < 0x80) s += (char)cp;
		else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
		else if (cp < 0x10000) { s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
		else { s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 0x3F)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
	}

	bool Hex4(unsigned &v)
	{
		v = 0;
		for (int i = 0; i < 4; i++)
		{
			char c = *p++;
			v <<= 4;
			if (c >= '0' && c <= '9') v |= c - '0';
			else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
			else return false;
		}
		return true;
	}

	bool Str(std::string &s)
	{
		if (*p != '"') return false;
		p++;
		while (*p && *p != '"')
		{
			if (*p == '\\')
			{
				p++;
				switch (*p)
				{
				case '"': s += '"'; p++; break;
				case '\\': s += '\\'; p++; break;
				case '/': s += '/'; p++; break;
				case 'b': s += '\b'; p++; break;
				case 'f': s += '\f'; p++; break;
				case 'n': s += '\n'; p++; break;
				case 'r': s += '\r'; p++; break;
				case 't': s += '\t'; p++; break;
				case 'u':
				{
					p++;
					unsigned v;
					if (!Hex4(v)) return false;
					if (v >= 0xD800 && v < 0xDC00 && p[0] == '\\' && p[1] == 'u')
					{
						p += 2;
						unsigned lo;
						if (!Hex4(lo)) return false;
						v = 0x10000 + ((v - 0xD800) << 10) + (lo - 0xDC00);
					}
					AppendUtf8(s, v);
					break;
				}
				default: return false;
				}
			}
			else s += *p++;
		}
		if (*p != '"') return false;
		p++;
		return true;
	}

	bool Val(JVal &v, int depth)
	{
		if (depth > 64) return false;
		Ws();
		char c = *p;
		if (c == '{')
		{
			p++;
			v.t = JVal::OBJ;
			Ws();
			if (*p == '}') { p++; return true; }
			for (;;)
			{
				Ws();
				std::string key;
				if (!Str(key)) return false;
				Ws();
				if (*p != ':') return false;
				p++;
				JVal child;
				if (!Val(child, depth + 1)) return false;
				v.o.push_back(std::make_pair(key, child));
				Ws();
				if (*p == ',') { p++; continue; }
				if (*p == '}') { p++; return true; }
				return false;
			}
		}
		if (c == '[')
		{
			p++;
			v.t = JVal::ARR;
			Ws();
			if (*p == ']') { p++; return true; }
			for (;;)
			{
				JVal child;
				if (!Val(child, depth + 1)) return false;
				v.a.push_back(child);
				Ws();
				if (*p == ',') { p++; continue; }
				if (*p == ']') { p++; return true; }
				return false;
			}
		}
		if (c == '"') { v.t = JVal::STR; return Str(v.s); }
		if (!strncmp(p, "true", 4)) { p += 4; v.t = JVal::BOOL_; v.b = true; return true; }
		if (!strncmp(p, "false", 5)) { p += 5; v.t = JVal::BOOL_; v.b = false; return true; }
		if (!strncmp(p, "null", 4)) { p += 4; v.t = JVal::NUL; return true; }
		if (c == '-' || (c >= '0' && c <= '9'))
		{
			char *end = 0;
			v.n = strtod(p, &end);
			if (end == p) return false;
			p = end;
			v.t = JVal::NUM;
			return true;
		}
		return false;
	}
};

} // namespace

bool ParseJson(const char *psz, JVal &out)
{
	if (!psz) return false;
	Reader r(psz);
	JVal v;
	if (!r.Val(v, 0)) return false;
	r.Ws();
	if (*r.p) return false;
	out = v;
	return true;
}

namespace
{

void WriteStr(const std::string &s, std::string &out)
{
	out += '"';
	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned char c = (unsigned char)s[i];
		switch (c)
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (c < 0x20)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
				out += buf;
			}
			else out += (char)c;  // 0x80 以上（UTF-8）はそのまま
		}
	}
	out += '"';
}

} // namespace

void WriteJson(const JVal &v, std::string &out)
{
	switch (v.t)
	{
	case JVal::NUL: out += "null"; break;
	case JVal::BOOL_: out += v.b ? "true" : "false"; break;
	case JVal::NUM:
	{
		char buf[40];
		if (v.n == (double)(long long)v.n && v.n > -1e15 && v.n < 1e15) snprintf(buf, sizeof(buf), "%lld", (long long)v.n);
		else snprintf(buf, sizeof(buf), "%.17g", v.n);
		out += buf;
		break;
	}
	case JVal::STR: WriteStr(v.s, out); break;
	case JVal::ARR:
		out += '[';
		for (size_t i = 0; i < v.a.size(); ++i)
		{
			if (i) out += ',';
			WriteJson(v.a[i], out);
		}
		out += ']';
		break;
	case JVal::OBJ:
		out += '{';
		for (size_t i = 0; i < v.o.size(); ++i)
		{
			if (i) out += ',';
			WriteStr(v.o[i].first, out);
			out += ':';
			WriteJson(v.o[i].second, out);
		}
		out += '}';
		break;
	}
}

} // namespace DungeonGen
