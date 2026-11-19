/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrDungeonDaily.cpp										 */
/* 概要：		日替わりダンジョンの版と最短記録(実装ファイル)				 */
/* 作成日：		2026/11/19													 */
/* ========================================================================= */
/*
 * 仕様は docs/dungeon-gen.md の「番外編 段階2 実装方針」。
 * 版（日付・作り直し番号・シード・テーマの写し・生成器の版・生成設定）と、版ごとの最短記録を
 * SBODATA/SboDungeonDaily.json に保存する。古い版も残す。
 *   { "format":1,
 *     "themes":[ {テーマ 1 件分の写し}, ... ],                       … 同じ内容は 1 回だけ（themeRef が指す）
 *     "dungeons":[ { "id":サブゲームの番号, "versions":[ { "no", "date", "rebuild", "seed", "genVer", "themeId",
 *                      "w", "h", "themeRef", "created", "records":[ {"names":[..], "forms":[..], "count", "ms", "at"} ] } ] } ] }
 * 最初に使う時にファイルから読む（無ければ空）。変えるたびに保存する（別名で書いてから置き換える）。
 */
#include <random>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "include.h"
#include "Glbldata.h"
#include "../../common/DungeonText.h"
/* DungeonGen のメンバー名 max が、互換層の max マクロとぶつかるので、読み込む間だけ外す */
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include "DungeonGen/DungeonGen.h"
#include "DungeonGen/MiniJson.h"
#pragma pop_macro("max")
#pragma pop_macro("min")
#include "SboSvrDungeonDaily.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
time_t g_tDungeonDailyNow = 0;										/* 0 以外ならこれを「今の時刻」にする（テスト用） */

#define DGDAILY_JST_SEC		(9 * 60 * 60)							/* 日本時間は UTC より 9 時間進んでいる */

static std::map<int, std::vector<DGDAILYVER> > s_mapVer;			/* ダンジョン（サブゲームの番号）→ 版（通し番号の順） */
static std::vector<std::string> s_aTheme;							/* テーマの写し（正規化した JSON） */
static BOOL s_bLoaded = FALSE;										/* ファイルから読んだか */

/* ========================================================================= */
/* 内部関数																	 */
/* ========================================================================= */

/* 保存先のファイル名を作る */
static void MakeFileName(char *pszOut)
{
	int i;

	GetModuleFileName (NULL, pszOut, MAX_PATH);
	for (i = (int)strlen (pszOut) - 1; i > 0; i --) {
		if (pszOut[i] == '\\') {
			pszOut[i + 1] = 0;
			break;
		}
	}
	strcat (pszOut, "SBODATA");
	CreateDirectory (pszOut, NULL);
	strcat (pszOut, "\\");
	strcat (pszOut, "SboDungeonDaily.json");
}

/* 32 ビットの混ぜ合わせ（最後に使う） */
static DWORD Mix32(DWORD h)
{
	h ^= h >> 16;
	h *= 0x85ebca6bu;
	h ^= h >> 13;
	h *= 0xc2b2ae35u;
	h ^= h >> 16;
	return h;
}

/* JSON の文字列にする（UTF-8 はそのまま） */
static void JsonStr(std::string &out, const std::string &s)
{
	char szTmp[8];
	size_t i;

	out += '"';
	for (i = 0; i < s.size (); i ++) {
		unsigned char c = (unsigned char)s[i];
		if (c == '"') {
			out += "\\\"";
		} else if (c == '\\') {
			out += "\\\\";
		} else if (c < 0x20) {
			snprintf (szTmp, sizeof (szTmp), "\\u%04x", (unsigned)c);
			out += szTmp;
		} else {
			out += (char)c;
		}
	}
	out += '"';
}

static void JsonStrArray(std::string &out, const std::vector<std::string> &a)
{
	size_t i;

	out += '[';
	for (i = 0; i < a.size (); i ++) {
		if (i) {
			out += ',';
		}
		JsonStr (out, a[i]);
	}
	out += ']';
}

/* 版 1 件の JSON */
static void VerJson(std::string &out, int nDungeon, const DGDAILYVER &v, BOOL bTheme)
{
	char szTmp[192];
	size_t i;

	snprintf (szTmp, sizeof (szTmp), "{\"no\":%u,\"date\":", (unsigned)v.dwNo);
	out += szTmp;
	JsonStr (out, v.strDate);
	snprintf (szTmp, sizeof (szTmp), ",\"rebuild\":%d,\"seed\":%u,\"genVer\":%d,\"themeId\":%u,\"w\":%d,\"h\":%d,\"themeRef\":%d,\"created\":",
			v.nRebuild, (unsigned)v.dwSeed, v.nGenVer, (unsigned)v.dwThemeID, v.nW, v.nH, v.nThemeRef);
	out += szTmp;
	JsonStr (out, v.strCreated);
	if (bTheme) {
		std::string strTheme = DungeonDailyThemeText (v.nThemeRef);
		out += ",\"theme\":";
		out += strTheme.empty () ? std::string ("null") : strTheme;
	}
	out += ",\"records\":[";
	for (i = 0; i < v.aRec.size (); i ++) {
		const DGDAILYREC &r = v.aRec[i];
		if (i) {
			out += ',';
		}
		out += "{\"names\":";
		JsonStrArray (out, r.aName);
		out += ",\"forms\":";
		JsonStrArray (out, r.aForm);
		out += ",\"tbl\":[";
		for (size_t t = 0; t < r.aTbl.size (); t ++) {
			snprintf (szTmp, sizeof (szTmp), t ? ",%u" : "%u", (unsigned)r.aTbl[t]);
			out += szTmp;
		}
		out += "]";
		snprintf (szTmp, sizeof (szTmp), ",\"count\":%d,\"ms\":%u,\"at\":", r.nCount, (unsigned)r.dwMs);
		out += szTmp;
		JsonStr (out, r.strAt);
		out += '}';
	}
	out += "],\"tried\":[";
	for (i = 0; i < v.aTried.size (); i ++) {
		snprintf (szTmp, sizeof (szTmp), i ? ",%u" : "%u", (unsigned)v.aTried[i]);
		out += szTmp;
	}
	out += "]}";
	(void)nDungeon;
}

/* 保存する */
static BOOL SaveAll(void)
{
	char szFile[MAX_PATH + 16], szTmp[MAX_PATH + 16];
	std::string strOut;
	std::map<int, std::vector<DGDAILYVER> >::iterator it;
	size_t i, j;
	HANDLE hFile;
	DWORD dwBytes;
	BOOL bFirst;

	strOut = "{\"format\":1,\"themes\":[";
	for (i = 0; i < s_aTheme.size (); i ++) {
		if (i) {
			strOut += ',';
		}
		strOut += s_aTheme[i];
	}
	strOut += "],\"dungeons\":[";
	bFirst = TRUE;
	for (it = s_mapVer.begin (); it != s_mapVer.end (); it ++) {
		char szTmp2[32];
		if (it->second.empty ()) {
			continue;
		}
		if (!bFirst) {
			strOut += ',';
		}
		bFirst = FALSE;
		snprintf (szTmp2, sizeof (szTmp2), "{\"id\":%d,\"versions\":[", it->first);
		strOut += szTmp2;
		for (j = 0; j < it->second.size (); j ++) {
			if (j) {
				strOut += ',';
			}
			VerJson (strOut, it->first, it->second[j], FALSE);
		}
		strOut += "]}";
	}
	strOut += "]}\n";

	MakeFileName (szFile);
	/* 途中で止まっても元のファイルが壊れないよう、別名で書いてから置き換える */
	strcpy (szTmp, szFile);
	strcat (szTmp, ".tmp");
	hFile = CreateFile (szTmp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	WriteFile (hFile, strOut.c_str (), (DWORD)strOut.size (), &dwBytes, NULL);
	CloseHandle (hFile);
	DeleteFile (szFile);
	return MoveFile (szTmp, szFile);
}

/* JSON の数（0〜4294967295）を DWORD で読む（無い・数でなければ 0） */
static DWORD GetU32(const DungeonGen::JVal &j, const char *pszKey)
{
	const DungeonGen::JVal *v = j.Get (pszKey);
	if ((v == NULL) || (v->t != DungeonGen::JVal::NUM) || (v->n < 0) || (v->n > 4294967295.0)) {
		return 0;
	}
	return (DWORD)(unsigned long long)v->n;
}

static std::string GetStr(const DungeonGen::JVal &j, const char *pszKey)
{
	const DungeonGen::JVal *v = j.Get (pszKey);
	return (v && (v->t == DungeonGen::JVal::STR)) ? v->s : std::string ();
}

static void GetStrArray(const DungeonGen::JVal &j, const char *pszKey, std::vector<std::string> &a)
{
	const DungeonGen::JVal *v = j.Get (pszKey);
	size_t i;

	a.clear ();
	if (v && (v->t == DungeonGen::JVal::ARR)) {
		for (i = 0; i < v->a.size (); i ++) {
			if (v->a[i].t == DungeonGen::JVal::STR) {
				a.push_back (v->a[i].s);
			}
		}
	}
}

/* ファイルから読む（無い・壊れている時は空のまま） */
static void LoadAll(void)
{
	char szFile[MAX_PATH + 16];
	HANDLE hFile;
	DWORD dwSize, dwBytes = 0;
	std::string strJson;
	DungeonGen::JVal root;
	const DungeonGen::JVal *pThemes, *pDungeons;
	size_t i, j, k;

	s_bLoaded = TRUE;
	s_mapVer.clear ();
	s_aTheme.clear ();

	MakeFileName (szFile);
	hFile = CreateFile (szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}
	dwSize = GetFileSize (hFile, NULL);
	if ((dwSize == 0) || (dwSize > 256 * 1024 * 1024)) {
		CloseHandle (hFile);
		return;
	}
	strJson.resize (dwSize);
	ReadFile (hFile, &strJson[0], dwSize, &dwBytes, NULL);
	CloseHandle (hFile);
	strJson.resize (dwBytes);
	if (!DungeonGen::ParseJson (strJson.c_str (), root) || (root.t != DungeonGen::JVal::OBJ)) {
		return;
	}
	pThemes = root.Get ("themes");
	if (pThemes && (pThemes->t == DungeonGen::JVal::ARR)) {
		for (i = 0; i < pThemes->a.size (); i ++) {
			std::string strText;
			DungeonGen::WriteJson (pThemes->a[i], strText);
			s_aTheme.push_back (strText);
		}
	}
	pDungeons = root.Get ("dungeons");
	if ((pDungeons == NULL) || (pDungeons->t != DungeonGen::JVal::ARR)) {
		return;
	}
	for (i = 0; i < pDungeons->a.size (); i ++) {
		const DungeonGen::JVal &jd = pDungeons->a[i];
		const DungeonGen::JVal *pVers = jd.Get ("versions");
		int nID = jd.Int ("id", -1);

		if ((nID < 0) || (pVers == NULL) || (pVers->t != DungeonGen::JVal::ARR)) {
			continue;
		}
		std::vector<DGDAILYVER> &aVer = s_mapVer[nID];
		for (j = 0; j < pVers->a.size (); j ++) {
			const DungeonGen::JVal &jv = pVers->a[j];
			const DungeonGen::JVal *pRecs = jv.Get ("records");
			DGDAILYVER v;

			v.dwNo = GetU32 (jv, "no");
			v.strDate = GetStr (jv, "date");
			v.nRebuild = jv.Int ("rebuild", 0);
			v.dwSeed = GetU32 (jv, "seed");
			v.nGenVer = jv.Int ("genVer", 0);
			v.dwThemeID = GetU32 (jv, "themeId");
			v.nW = jv.Int ("w", 0);
			v.nH = jv.Int ("h", 0);
			v.nThemeRef = jv.Int ("themeRef", -1);
			v.strCreated = GetStr (jv, "created");
			if ((v.dwNo == 0) || v.strDate.empty ()) {
				continue;
			}
			if (pRecs && (pRecs->t == DungeonGen::JVal::ARR)) {
				for (k = 0; (k < pRecs->a.size ()) && ((int)v.aRec.size () < DGDAILY_RECMAX); k ++) {
					const DungeonGen::JVal &jr = pRecs->a[k];
					DGDAILYREC r;

					GetStrArray (jr, "names", r.aName);
					GetStrArray (jr, "forms", r.aForm);
					{
						const DungeonGen::JVal *pTbl = jr.Get ("tbl");
						if (pTbl && (pTbl->t == DungeonGen::JVal::ARR)) {
							for (size_t m = 0; m < pTbl->a.size (); m ++) {
								if ((pTbl->a[m].t == DungeonGen::JVal::NUM) && (pTbl->a[m].n >= 0)) {
									r.aTbl.push_back ((DWORD)(unsigned long long)pTbl->a[m].n);
								}
							}
						}
					}
					r.nCount = jr.Int ("count", (int)r.aName.size ());
					r.dwMs = GetU32 (jr, "ms");
					r.strAt = GetStr (jr, "at");
					v.aRec.push_back (r);
				}
			}
			{
				const DungeonGen::JVal *pTried = jv.Get ("tried");
				if (pTried && (pTried->t == DungeonGen::JVal::ARR)) {
					for (size_t m = 0; m < pTried->a.size (); m ++) {
						if ((pTried->a[m].t == DungeonGen::JVal::NUM) && (pTried->a[m].n >= 0)) {
							v.aTried.push_back ((DWORD)(unsigned long long)pTried->a[m].n);
						}
					}
				}
			}
			aVer.push_back (v);
		}
	}
}

static void Ensure(void)
{
	if (!s_bLoaded) {
		LoadAll ();
	}
}

/* テーマの写しの番号（同じ内容があればそれ、無ければ足す） */
static int ThemeRefOf(const std::string &strText)
{
	size_t i;

	for (i = 0; i < s_aTheme.size (); i ++) {
		if (s_aTheme[i] == strText) {
			return (int)i;
		}
	}
	s_aTheme.push_back (strText);
	return (int)s_aTheme.size () - 1;
}

/* ランダムなシード（0 以外。avoid と違うもの） */
static DWORD RandomSeed(DWORD dwAvoid)
{
	std::random_device rd;
	DWORD dwSeed = 0;
	int i;

	for (i = 0; i < 16; i ++) {
		dwSeed = (DWORD)rd () ^ (DWORD)time (NULL) ^ Mix32 (timeGetTime () + (DWORD)i);
		if ((dwSeed != 0) && (dwSeed != dwAvoid)) {
			return dwSeed;
		}
	}
	return dwAvoid + 1 ? dwAvoid + 1 : 1;
}

/* ========================================================================= */
/* 公開関数																	 */
/* ========================================================================= */

time_t DungeonDailyNow(void)
{
	return g_tDungeonDailyNow ? g_tDungeonDailyNow : time (NULL);
}

static void JstTm(time_t t, struct tm *pTm)
{
	time_t tJst = t + DGDAILY_JST_SEC;
	struct tm *p = gmtime (&tJst);

	if (p) {
		*pTm = *p;
	} else {
		memset (pTm, 0, sizeof (*pTm));
		pTm->tm_year = 70;
		pTm->tm_mday = 1;
	}
}

std::string DungeonDailyDate(time_t t)
{
	struct tm tmJst;
	char szTmp[32];

	JstTm (t, &tmJst);
	snprintf (szTmp, sizeof (szTmp), "%04d-%02d-%02d", tmJst.tm_year + 1900, tmJst.tm_mon + 1, tmJst.tm_mday);
	return szTmp;
}

std::string DungeonDailyDateTime(time_t t)
{
	struct tm tmJst;
	char szTmp[48];

	JstTm (t, &tmJst);
	snprintf (szTmp, sizeof (szTmp), "%04d-%02d-%02d %02d:%02d:%02d", tmJst.tm_year + 1900, tmJst.tm_mon + 1, tmJst.tm_mday,
			tmJst.tm_hour, tmJst.tm_min, tmJst.tm_sec);
	return szTmp;
}

/* その日のシード：日付の文字（FNV-1a）とダンジョン番号を混ぜる。どの環境でも同じ値になる */
DWORD DungeonDailySeed(const std::string &strDate, int nDungeon)
{
	DWORD h = 2166136261u;
	size_t i;

	for (i = 0; i < strDate.size (); i ++) {
		h = (h ^ (DWORD)(unsigned char)strDate[i]) * 16777619u;
	}
	h = Mix32 (h ^ ((DWORD)nDungeon * 0x9e3779b1u + 0x7f4a7c15u));
	return h ? h : 1;
}

/* 敵の出る位置の乱数のシード：版のシード・発生元の番号・何回目の試しかを混ぜる */
DWORD DungeonDailyPutSeed(DWORD dwVerSeed, DWORD dwParentTblID, DWORD dwTry)
{
	DWORD h = Mix32 (dwVerSeed ^ 0x51ed270bu);

	h = Mix32 (h + dwParentTblID * 0x9e3779b1u + 0x165667b1u);
	h = Mix32 (h + dwTry * 0x85ebca6bu + 0x27d4eb2fu);
	return h ? h : 1;
}

BOOL DungeonDailyAcquire(int nDungeon, DWORD dwThemeID, int nW, int nH, int nHow, const char *pszFileTheme, DGDAILYVER &out)
{
	std::string strToday = DungeonDailyDate (DungeonDailyNow ());
	BOOL bToday;
	DGDAILYVER v;

	Ensure ();
	std::vector<DGDAILYVER> &aVer = s_mapVer[nDungeon];
	bToday = !aVer.empty () && (aVer.back ().strDate == strToday);
	/* 今日の版があって、生成器の版・テーマ番号・大きさが今と同じなら、そのまま使う（再起動しても同じ形） */
	if ((nHow == DGDAILY_NORMAL) && bToday) {
		const DGDAILYVER &c = aVer.back ();
		if ((c.nGenVer == DungeonGen::GEN_VERSION) && (c.dwThemeID == dwThemeID) && (c.nW == nW) && (c.nH == nH) && (c.nThemeRef >= 0) &&
			(c.nThemeRef < (int)s_aTheme.size ())) {
			out = c;
			return TRUE;
		}
	}
	/* 新しい版を足す（テーマの写しは今のテーマファイルから） */
	if ((pszFileTheme == NULL) || (pszFileTheme[0] == 0)) {
		return FALSE;
	}
	v.dwNo = aVer.empty () ? 1 : (aVer.back ().dwNo + 1);
	v.strDate = strToday;
	if (nHow == DGDAILY_ADMIN) {
		v.nRebuild = bToday ? (aVer.back ().nRebuild + 1) : 1;
		v.dwSeed = RandomSeed (bToday ? aVer.back ().dwSeed : DungeonDailySeed (strToday, nDungeon));
	} else if (bToday) {
		/* その日のうちに設定（生成器の版・テーマ番号・大きさ）が変わった：同じシードで、新しい版にする */
		v.nRebuild = aVer.back ().nRebuild;
		v.dwSeed = aVer.back ().dwSeed;
	} else {
		v.nRebuild = 0;
		v.dwSeed = DungeonDailySeed (strToday, nDungeon);
	}
	v.nGenVer = DungeonGen::GEN_VERSION;
	v.dwThemeID = dwThemeID;
	v.nW = nW;
	v.nH = nH;
	v.nThemeRef = ThemeRefOf (pszFileTheme);
	v.strCreated = DungeonDailyDateTime (DungeonDailyNow ());
	aVer.push_back (v);
	SaveAll ();
	out = v;
	return TRUE;
}

BOOL DungeonDailyFind(int nDungeon, DWORD dwNo, DGDAILYVER &out)
{
	std::map<int, std::vector<DGDAILYVER> >::iterator it;
	size_t i;

	Ensure ();
	it = s_mapVer.find (nDungeon);
	if (it == s_mapVer.end ()) {
		return FALSE;
	}
	for (i = 0; i < it->second.size (); i ++) {
		if (it->second[i].dwNo == dwNo) {
			out = it->second[i];
			return TRUE;
		}
	}
	return FALSE;
}

std::string DungeonDailyThemeText(int nRef)
{
	Ensure ();
	if ((nRef < 0) || (nRef >= (int)s_aTheme.size ())) {
		return std::string ();
	}
	return s_aTheme[nRef];
}

BOOL DungeonDailyStale(int nDungeon, DWORD dwVerNo, DWORD dwThemeID, int nW, int nH)
{
	DGDAILYVER v;

	if (!DungeonDailyFind (nDungeon, dwVerNo, v)) {
		return TRUE;
	}
	return (v.strDate != DungeonDailyDate (DungeonDailyNow ())) || (v.nGenVer != DungeonGen::GEN_VERSION) || (v.dwThemeID != dwThemeID) ||
			(v.nW != nW) || (v.nH != nH);
}

/* 記録を足す。時間の短い順（同じ時間は先に出した組が上）。入らなければ 0 */
int DungeonDailyAddRecord(int nDungeon, DWORD dwVerNo, const DGDAILYREC &Rec, BOOL *pbBetter)
{
	std::map<int, std::vector<DGDAILYVER> >::iterator it;
	size_t i, nPos;

	Ensure ();
	it = s_mapVer.find (nDungeon);
	if (it == s_mapVer.end ()) {
		return 0;
	}
	for (i = 0; i < it->second.size (); i ++) {
		if (it->second[i].dwNo != dwVerNo) {
			continue;
		}
		std::vector<DGDAILYREC> &aRec = it->second[i].aRec;
		if (pbBetter) {
			*pbBetter = TRUE;
		}
		/* 同じ顔ぶれの組：速い記録だけ残す */
		if (!Rec.aTbl.empty ()) {
			for (size_t m = 0; m < aRec.size (); m ++) {
				if (aRec[m].aTbl != Rec.aTbl) {
					continue;
				}
				if (aRec[m].dwMs <= Rec.dwMs) {
					if (pbBetter) {
						*pbBetter = FALSE;
					}
					return (int)m + 1;
				}
				aRec.erase (aRec.begin () + m);
				break;
			}
		}
		nPos = 0;
		while ((nPos < aRec.size ()) && (aRec[nPos].dwMs <= Rec.dwMs)) {
			nPos ++;
		}
		if (nPos >= (size_t)DGDAILY_RECMAX) {
			return 0;
		}
		aRec.insert (aRec.begin () + nPos, Rec);
		if ((int)aRec.size () > DGDAILY_RECMAX) {
			aRec.resize (DGDAILY_RECMAX);
		}
		SaveAll ();
		return (int)nPos + 1;
	}
	return 0;
}

BOOL DungeonDailyTried(int nDungeon, DWORD dwVerNo, DWORD dwTblID)
{
	DGDAILYVER v;

	if (!DungeonDailyFind (nDungeon, dwVerNo, v)) {
		return FALSE;
	}
	return std::find (v.aTried.begin (), v.aTried.end (), dwTblID) != v.aTried.end ();
}

void DungeonDailyMarkTried(int nDungeon, DWORD dwVerNo, const std::vector<DWORD> &aTbl)
{
	std::map<int, std::vector<DGDAILYVER> >::iterator it;
	size_t i, m;
	BOOL bChanged = FALSE;

	Ensure ();
	it = s_mapVer.find (nDungeon);
	if (it == s_mapVer.end ()) {
		return;
	}
	for (i = 0; i < it->second.size (); i ++) {
		if (it->second[i].dwNo != dwVerNo) {
			continue;
		}
		std::vector<DWORD> &aTried = it->second[i].aTried;
		for (m = 0; m < aTbl.size (); m ++) {
			if (std::find (aTried.begin (), aTried.end (), aTbl[m]) == aTried.end ()) {
				aTried.push_back (aTbl[m]);
				bChanged = TRUE;
			}
		}
		if (bChanged) {
			SaveAll ();
		}
		return;
	}
}

std::string DungeonDailyTimeText(DWORD dwMs)
{
	char szTmp[48];
	DWORD dwSec = dwMs / 1000;

	snprintf (szTmp, sizeof (szTmp), "%u分%u秒", (unsigned)(dwSec / 60), (unsigned)(dwSec % 60));
	return szTmp;
}

/* 入口で知らせる「今日の 1〜3 位」 */
std::string DungeonDailyTopText(int nDungeon, DWORD dwVerNo)
{
	DGDAILYVER v;
	std::string strOut;
	size_t i;

	if (!DungeonDailyFind (nDungeon, dwVerNo, v)) {
		return std::string ();
	}
    const SUBGAME_DUNGEONINFO *p = NULL;
    if (nDungeon >= 0 && nDungeon < g_aSubaGameInfo.GetSize () && g_aSubaGameInfo[nDungeon]->byType == SUBGAMEID_DUNGEON)
        p = (PSUBGAME_DUNGEONINFO)g_aSubaGameInfo[nDungeon]->pInfo;
    if (v.aRec.empty ()) return DungeonText (p, "records_empty");
    strOut = DungeonText (p, "records_title");
    for (i = 0; i < v.aRec.size () && i < 3; i ++) {
        const DGDAILYREC &r = v.aRec[i];
        if (i) strOut += DungeonText (p, "record_separator");
        std::string names;
        if (!r.aName.empty ()) {
            std::string others = r.nCount > 1 ? DungeonText (p, "record_others", "count", std::to_string (r.nCount - 1)) : "";
            names = DungeonText (p, "record_names", "name", r.aName[0], "others", others);
        }
        strOut += DungeonText (p, "record_entry", "rank", std::to_string (i + 1),
            "time", DungeonDailyTimeText (r.dwMs), "names", names);
    }
	return strOut;
}

std::string DungeonDailyVersionsJson(int nDungeon, DWORD dwVerNo, BOOL bTheme)
{
	std::map<int, std::vector<DGDAILYVER> >::iterator it;
	std::string strOut = "[";
	size_t i;
	BOOL bFirst = TRUE;

	Ensure ();
	it = s_mapVer.find (nDungeon);
	if (it != s_mapVer.end ()) {
		for (i = 0; i < it->second.size (); i ++) {
			if (dwVerNo && (it->second[i].dwNo != dwVerNo)) {
				continue;
			}
			if (!bFirst) {
				strOut += ',';
			}
			bFirst = FALSE;
			VerJson (strOut, nDungeon, it->second[i], bTheme);
		}
	}
	strOut += "]";
	return strOut;
}

int DungeonDailyGenVersion(void)
{
	return DungeonGen::GEN_VERSION;
}

int DungeonDailyVersionCount(int nDungeon)
{
	std::map<int, std::vector<DGDAILYVER> >::iterator it;

	Ensure ();
	it = s_mapVer.find (nDungeon);
	return (it == s_mapVer.end ()) ? 0 : (int)it->second.size ();
}

void DungeonDailyReset(BOOL bDeleteFile)
{
	s_mapVer.clear ();
	s_aTheme.clear ();
	s_bLoaded = FALSE;
	if (bDeleteFile) {
		char szFile[MAX_PATH + 16];
		MakeFileName (szFile);
		DeleteFile (szFile);
	}
}

void DungeonDailyReload(void)
{
	DungeonDailyReset (FALSE);
}
