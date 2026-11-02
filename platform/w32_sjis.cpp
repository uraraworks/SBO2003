/* ========================================================================= */
/* ファイル名：	w32_sjis.cpp												 */
/* 概要：		Win32 互換レイヤ：文字コード（CP932 ⇔ UTF-8）				 */
/* ========================================================================= */
/*
 * ゲーム内部の文字列は Windows 版と同じく SJIS(CP932) のまま扱う。
 * ブラウザに渡す時（HTML の入力欄、メッセージボックス、ログ）だけ UTF-8 に変換する。
 */
#include "w32_internal.h"

#include <map>

#include "sjis_table.inc"

/* リードバイトを表の行番号にする（リードバイトでなければ -1） */
static int SjisLeadIndex(BYTE by)
{
	if (by >= 0x81 && by <= 0x9F) {
		return by - 0x81;
	}
	if (by >= 0xE0 && by <= 0xFC) {
		return by - 0xE0 + 31;
	}
	return -1;
}

BOOL IsDBCSLeadByte(BYTE byChar)
{
	return (SjisLeadIndex(byChar) >= 0) ? TRUE : FALSE;
}

LPSTR CharNext(LPCSTR psz)
{
	if (psz == NULL || *psz == 0) {
		return (LPSTR)psz;
	}
	if (IsDBCSLeadByte((BYTE)psz[0]) && psz[1]) {
		return (LPSTR)(psz + 2);
	}
	return (LPSTR)(psz + 1);
}

LPSTR CharPrev(LPCSTR pszStart, LPCSTR psz)
{
	if (psz <= pszStart) {
		return (LPSTR)pszStart;
	}
	/* 先頭から数えて直前の文字の位置を探す */
	LPCSTR p = pszStart;
	LPCSTR pPrev = pszStart;
	while (p < psz && *p) {
		pPrev = p;
		p = CharNext(p);
	}
	return (LPSTR)pPrev;
}

void W32SjisToUcs(const char *pszSjis, int nLen, std::vector<unsigned short> &aOut)
{
	aOut.clear();
	if (pszSjis == NULL) {
		return;
	}
	if (nLen < 0) {
		nLen = (int)strlen(pszSjis);
	}
	const BYTE *p = (const BYTE *)pszSjis;
	for (int i = 0; i < nLen; ) {
		BYTE by = p[i];
		int nLead = SjisLeadIndex(by);
		if (nLead >= 0 && i + 1 < nLen && p[i + 1] >= 0x40 && p[i + 1] <= 0xFC) {
			unsigned short wc = s_awSjisToUcs[nLead][p[i + 1] - 0x40];
			aOut.push_back(wc ? wc : 0x30FB);	/* 未定義は「・」 */
			i += 2;
			continue;
		}
		if (by >= 0xA1 && by <= 0xDF) {
			aOut.push_back((unsigned short)(0xFF61 + (by - 0xA1)));	/* 半角カナ */
		} else if (by < 0x80) {
			aOut.push_back(by);
		} else {
			aOut.push_back('?');
		}
		i ++;
	}
}

static void AppendUtf8(std::string &str, unsigned int wc)
{
	if (wc < 0x80) {
		str += (char)wc;
	} else if (wc < 0x800) {
		str += (char)(0xC0 | (wc >> 6));
		str += (char)(0x80 | (wc & 0x3F));
	} else {
		str += (char)(0xE0 | (wc >> 12));
		str += (char)(0x80 | ((wc >> 6) & 0x3F));
		str += (char)(0x80 | (wc & 0x3F));
	}
}

std::string W32SjisToUtf8Str(const char *pszSjis, int nLen)
{
	std::vector<unsigned short> aUcs;
	W32SjisToUcs(pszSjis, nLen, aUcs);
	std::string str;
	str.reserve(aUcs.size() * 3);
	for (size_t i = 0; i < aUcs.size(); i ++) {
		AppendUtf8(str, aUcs[i]);
	}
	return str;
}

/* Unicode → CP932 の逆引き表（初回に作る） */
static std::map<unsigned short, unsigned short> &GetUcsToSjis(void)
{
	static std::map<unsigned short, unsigned short> s_map;
	if (s_map.empty()) {
		for (int nLead = 0; nLead < 60; nLead ++) {
			int byLead = (nLead < 31) ? (0x81 + nLead) : (0xE0 + nLead - 31);
			for (int nTrail = 0; nTrail < 189; nTrail ++) {
				unsigned short wc = s_awSjisToUcs[nLead][nTrail];
				/* 同じ文字が複数ある時(NEC選定IBM拡張など)は先に出た方を使う */
				if (wc && s_map.find(wc) == s_map.end()) {
					s_map[wc] = (unsigned short)((byLead << 8) | (nTrail + 0x40));
				}
			}
		}
	}
	return s_map;
}

std::string W32Utf8ToSjisStr(const char *pszUtf8)
{
	std::string str;
	if (pszUtf8 == NULL) {
		return str;
	}
	std::map<unsigned short, unsigned short> &mapUcs = GetUcsToSjis();
	const BYTE *p = (const BYTE *)pszUtf8;
	while (*p) {
		unsigned int wc;
		int nBytes;
		if (p[0] < 0x80) {
			wc = p[0];
			nBytes = 1;
		} else if ((p[0] & 0xE0) == 0xC0 && p[1]) {
			wc = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
			nBytes = 2;
		} else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
			wc = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
			nBytes = 3;
		} else if ((p[0] & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
			wc = 0x10000;		/* BMP 外は SJIS に無い */
			nBytes = 4;
		} else {
			wc = '?';
			nBytes = 1;
		}
		p += nBytes;

		if (wc < 0x80) {
			str += (char)wc;
		} else if (wc >= 0xFF61 && wc <= 0xFF9F) {
			str += (char)(0xA1 + (wc - 0xFF61));
		} else {
			std::map<unsigned short, unsigned short>::iterator it = (wc < 0x10000) ? mapUcs.find((unsigned short)wc) : mapUcs.end();
			if (it == mapUcs.end()) {
				/* 波ダッシュなど、ブラウザと CP932 で文字が違うものを寄せる */
				unsigned int wcAlt = 0;
				switch (wc) {
				case 0x301C: wcAlt = 0xFF5E; break;		/* 〜 */
				case 0x2016: wcAlt = 0x2225; break;		/* ‖ */
				case 0x2212: wcAlt = 0xFF0D; break;		/* − */
				case 0x00A2: wcAlt = 0xFFE0; break;		/* ¢ */
				case 0x00A3: wcAlt = 0xFFE1; break;		/* £ */
				case 0x00AC: wcAlt = 0xFFE2; break;		/* ¬ */
				}
				if (wcAlt) {
					it = mapUcs.find((unsigned short)wcAlt);
				}
			}
			if (it != mapUcs.end()) {
				str += (char)(it->second >> 8);
				str += (char)(it->second & 0xFF);
			} else {
				str += '?';
			}
		}
	}
	return str;
}

int W32SjisToUtf8(const char *pszSjis, int nLen, char *pszOut, int nOutSize)
{
	std::string str = W32SjisToUtf8Str(pszSjis, nLen);
	if (pszOut == NULL || nOutSize <= 0) {
		return (int)str.size();
	}
	int nCopy = (int)str.size();
	if (nCopy > nOutSize - 1) {
		nCopy = nOutSize - 1;
	}
	memcpy(pszOut, str.data(), nCopy);
	pszOut[nCopy] = 0;
	return nCopy;
}

int W32Utf8ToSjis(const char *pszUtf8, char *pszOut, int nOutSize)
{
	std::string str = W32Utf8ToSjisStr(pszUtf8);
	if (pszOut == NULL || nOutSize <= 0) {
		return (int)str.size();
	}
	int nCopy = (int)str.size();
	if (nCopy > nOutSize - 1) {
		nCopy = nOutSize - 1;
	}
	/* 2 バイト文字の途中で切らない */
	if (nCopy > 0 && nCopy < (int)str.size()) {
		int i = 0;
		while (i < nCopy) {
			int n = IsDBCSLeadByte((BYTE)str[i]) ? 2 : 1;
			if (i + n > nCopy) {
				break;
			}
			i += n;
		}
		nCopy = i;
	}
	memcpy(pszOut, str.data(), nCopy);
	pszOut[nCopy] = 0;
	return nCopy;
}
