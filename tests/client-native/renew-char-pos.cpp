// 本体の配列・構造体・更新処理を使い、作り直し1回の時間を測る。
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = uint32_t;
using COLORREF = uint32_t;
using BOOL = int;
using LPSTR = char *;
using PBYTE = BYTE *;
using PVOID = void *;
using PDWORD = DWORD *;
using LPBYTE = BYTE *;
using LPCSTR = const char *;
constexpr int MAX_PATH = 260;
inline int abs(DWORD x) { return std::abs(static_cast<int32_t>(x)); }
constexpr BOOL TRUE = 1, FALSE = 0;
#define ZeroMemory(p, n) memset(p, 0, n)
#define CopyMemory(p, q, n) memcpy(p, q, n)
#define MoveMemory(p, q, n) memmove(p, q, n)
#include "common/myArray.h"
#include "common/TypedefCommon.h"
#include "common/SboPacket.h"
#include "common/crc.h"
#include "renew-packet.inc"
// 本体と同じ引数の型が異なる min/max (Windows マクロ相当)。
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
constexpr int SCRMARTSSIZE = (DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y * 2 + 3);
CmyArray<PCHARINFO, PCHARINFO> g_aCharInfo;
CmyArray<DWORD, DWORD> g_aCharPos[DRAW_PARTS_Y * 2 + 3];
MAPINFO mapInfo{};
PMAPINFO g_pMapInfo = &mapInfo;
MAPPARTSINFO parts[3]{};
PMAPPARTSINFO g_pMapPartsInfo = parts;
WORD g_wCountMapParts = 3;
int g_nMyCharID = 0;
char dark[SCRMARTSSIZE]{}, darkPos[SCRMARTSSIZE]{};
char *g_pMapDarkTmp = dark, *g_pMapDarkTmpPos = darkPos;
#include "renew-functions.inc"
constexpr int SCRSIZEX = 480, SCRSIZEY = 480;
int anPosAddTblX[] = {0, 0, -1, 1}, anPosAddTblY[] = {-1, 1, 0, 0};
#include "common/BowFlight.h"
static DWORD testTime = 0;
static DWORD timeGetTime() { return testTime; }
struct Dib {
	std::vector<DWORD> pixels = std::vector<DWORD>(544 * 544);
	BYTE *GetBits() { return reinterpret_cast<BYTE *>(pixels.data()); }
	int Width() { return 544; }
	int Height() { return 544; }
} dib;
Dib *g_pDibDrawMap = &dib;
#include "renew-draw.inc"

// 通信・UI の外部作用は記録する。受信・削除・橋判定は本体の処理を使用する。
static bool useOld;
static std::vector<DWORD> sent;
CmyArray<DWORD, DWORD> g_aRecTbl;
CmyArray<POBJINFO, POBJINFO> g_aObjInfo;
DWORD g_pMyID = 1;
BYTE g_byGameState = GAMESTATE_MOVE;
BOOL g_bDraw, g_bEnableMove, g_bReqestPos;
SUBGAMECLIENT_WARINFO warInfo{};
PSUBGAMECLIENT_WARINFO g_pWarInfo = &warInfo;
int *g_pTargetSelectInfo = nullptr;
CCRC g_crc;
static int GetCharID(DWORD id) { return GetCharNo(id); }
static void SendCmdCHARINFOREQ(DWORD id) { sent.push_back(id); }
static void SendCmdEFECTCHG(BYTE a, BOOL b) { sent.push_back(a + b); }
static void AddSysMsg(LPCSTR, COLORREF c) { sent.push_back(c); }
#define RGB(r, g, b) ((COLORREF)((r) | ((g) << 8) | ((b) << 16)))

#include "renew-trade.inc"
static TRADEINFO tradeInfo{};
PTRADEINFO g_pTradeInfo = nullptr;
int g_nProcChar;
static void SendCmdTRADE(DWORD id, BYTE command) { sent.push_back(id); sent.push_back(command); }
#include "renew-delete.inc"
#include "renew-receivers.inc"
#undef min
#undef max

static std::array<DWORD, 64 * 64> occupancy{};
static std::array<WORD, 64 * 64> tiles{};
static volatile int checksum;
static void Prepare(int count, int level, bool walls, int lights)
{
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		SAFE_DELETE_ARRAY(g_aCharInfo[i]->pszNpcSpeech);
		delete g_aCharInfo[i];
	}
	g_aCharInfo.RemoveAll ();
	for (int i = 0; i < g_aObjInfo.GetSize(); i ++) delete g_aObjInfo[i];
	g_aObjInfo.RemoveAll();
	g_aRecTbl.RemoveAll();
	sent.clear();
	ResetMapViewSeen();
	memset(dark, 0, sizeof(dark));
	memset(darkPos, 0, sizeof(darkPos));
	memset(s_abyHideShow, 0, sizeof(s_abyHideShow));
	s_nLightMaskCount = 0;
	s_bCharPosInfoDirty = FALSE;
	mapInfo = {};
	parts[0] = parts[1] = parts[2] = {};
	occupancy.fill(0);
	mapInfo.pdwMapChar = occupancy.data();
	g_nMyCharID = 0;
	g_bDraw = g_bEnableMove = FALSE;
	g_bReqestPos = TRUE;
	mapInfo.nCX = mapInfo.nCY = 64;
	mapInfo.pwMap = tiles.data();
	mapInfo.byLightLevel = level;
	mapInfo.byFlg = walls ? MAPFLG_DARK : 0;
	mapInfo.byLightCount = lights;
	parts[1].byHitType = PARTSHIT_BLOCK1;
	for (int i = 0; i < 64 * 64; i ++) tiles[i] = walls && i % 19 == 0 ? 1 : 0;
	for (int i = 0; i < count; i ++) {
		PCHARINFO c = new CHARINFO;
		ZeroMemory(static_cast<void *>(c), sizeof(*c));
		c->pID = i + 1;
		c->dwTblID = i + 1;
		c->dwMapID = 1;
		c->dwX = 32 + (i % 15) - 7;
		c->dwY = 32 + (i / 15) - 6;
		c->byLightLevel = i % 10 == 0 ? 5 : 0;
		g_aCharInfo.Add (c);
	}
	if (count) g_aCharInfo[0]->dwX = g_aCharInfo[0]->dwY = 32;
	for (int i = 0; i < count; i ++) occupancy[g_aCharInfo[i]->dwX + 64 * g_aCharInfo[i]->dwY] ++;
	for (int i = 0; i < lights; i ++) {
		mapInfo.aLight[i].wX = 26 + i % 4 * 4;
		mapInfo.aLight[i].wY = 26 + i / 4 * 4;
		mapInfo.aLight[i].byRange = MAPLIGHT_RANGE_MAX;
	}
}
static void Benchmark(int count, int level, bool walls, int lights)
{
	Prepare(count, level, walls, lights);
	constexpr int repeats = 200, rounds = 5;
	std::array<double, rounds> times;
	for (int i = 0; i < 20; i ++) RenewCharPosInfo ();
	for (auto &time : times) {
		auto begin = std::chrono::steady_clock::now();
		for (int i = 0; i < repeats; i ++) {
			RenewCharPosInfo ();
			checksum = dark[SCRMARTSSIZE / 2] + g_aCharPos[8].GetSize ();
		}
		time = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() / repeats;
	}
	std::sort(times.begin(), times.end());
	std::printf("%dキャラ level=%d 壁=%d マップ光源=%d: 更新1回 %.6f ms\n", count, level, walls, lights, times[rounds / 2]);
}
using Buffer = std::vector<BYTE>;
static void Append(Buffer &out, const void *p, size_t size)
{
	const BYTE *bytes = static_cast<const BYTE *>(p);
	out.insert(out.end(), bytes, bytes + size);
}
template<class T> static void Append(Buffer &out, const T &v) { Append(out, &v, sizeof(v)); }
static Buffer ReceiveState()
{
	Buffer out;
	Append(out, g_nMyCharID);
	Append(out, g_bDraw);
	Append(out, g_bReqestPos);
	Append(out, g_bEnableMove);
	Append(out, warInfo);
	Append(out, g_nProcChar);
	Append(out, g_aRecTbl.GetSize());
	for (int i = 0; i < g_aRecTbl.GetSize(); i ++) Append(out, g_aRecTbl[i]);
	Append(out, occupancy);
	Append(out, g_aCharInfo.GetSize());
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		const auto *c = g_aCharInfo[i];
		// アドレスだけが違う所有文字列を正規化し、キャラ情報の全バイトを比較する。
		std::array<BYTE, sizeof(CHARINFO)> bytes;
		memcpy(bytes.data(), static_cast<const void *>(c), bytes.size());
		for (const auto *field : {&c->pszSpeech, &c->pszNpcSpeech}) {
			size_t offset = reinterpret_cast<const BYTE *>(field) - reinterpret_cast<const BYTE *>(c);
			memset(bytes.data() + offset, 0, sizeof(*field));
		}
		Append(out, bytes);
		if (c->pszNpcSpeech) Append(out, c->pszNpcSpeech, strlen(c->pszNpcSpeech) + 1);
	}
	Append(out, g_aObjInfo.GetSize());
	for (int i = 0; i < g_aObjInfo.GetSize(); i ++) Append(out, *g_aObjInfo[i]);
	for (DWORD v : sent) Append(out, v);
	return out;
}
static Buffer DrawState()
{
	Buffer out;
	for (auto &row : g_aCharPos) {
		Append(out, row.GetSize());
		for (int i = 0; i < row.GetSize(); i ++) Append(out, row[i]);
	}
	Append(out, dark);
	Append(out, s_abyHideShow);
	Append(out, s_nLightMaskCount);
	for (int i = 0; i < s_nLightMaskCount; i ++) {
		const auto &mask = s_aLightMask[i];
		DWORD id = mask.pKey ? static_cast<const CHARINFO *>(mask.pKey)->pID : 0;
		Append(out, id);
		Append(out, mask.nX);
		Append(out, mask.nY);
		Append(out, mask.abyMask);
	}
	Append(out, s_dwViewSeenMapID);
	Append(out, s_nViewSeenCX);
	Append(out, s_nViewSeenCY);
	if (s_pbyViewSeen) Append(out, s_pbyViewSeen, s_nViewSeenCX * s_nViewSeenCY);
	return out;
}

struct Message {
	BYTE command;
	Buffer data;
};
static int Clamp(int x) { return std::max(0, std::min(63, x)); }
static std::vector<Message> Messages(int serial, int x, int y)
{
	std::mt19937 rng(0x53424f33 + serial);
	std::vector<Message> messages;
	for (int i = 0; i < 24; i ++) {
		BYTE command = i % 3 == 0 ? CMD_CHARINFO : (i % 3 == 1 ? CMD_CHARINFO2 : CMD_CHARPOS);
		int id = i % 7 == 0 ? 1 : 2 + rng() % 220;
		int mx = Clamp(x + int(rng() % 19) - 9), my = Clamp(y + int(rng() % 19) - 9);
		if (id == 1) { mx = Clamp(x + i % 3 - 1); my = Clamp(y + i % 2); }
		BYTE direction = rng() % 4;
		BYTE light = serial % 4 == 0 ? 0 : BYTE(rng() % 22);
		BYTE npc = rng() % 5 == 0 ? NPCTYPE_QUESTNPC : 0;
		Buffer packet(2048, 0);
		BYTE *end;
		if (command == CMD_CHARINFO) {
			CMDPARA_CHARINFO c{};
			c.byCmd = command; c.pID = c.dwTblID = id; c.dwMapID = 1;
			c.dwX = mx; c.dwY = my; c.byDirection = direction;
			c.byLightLevel = light; c.byNpcType = npc;
			strcpy(c.szName, "test");
			c.pszNpcSpeech = serial % 9 == 0 ? reinterpret_cast<LPSTR>(1) : nullptr;
			end = SboPackPutCHARINFO(packet.data(), &c);
			if (c.pszNpcSpeech) { memcpy(end, "speech", 7); end += 7; }
		} else if (command == CMD_CHARINFO2) {
			CMDPARA_CHARINFO2 c{};
			c.byCmd = command; c.pID = c.dwTblID = id; c.dwMapID = 1;
			c.dwX = mx; c.dwY = my; c.byDirection = direction;
			c.byLightLevel = light; c.byNpcType = npc;
			strcpy(c.szName, "test");
			end = SboPackPutCHARINFO2(packet.data(), &c);
		} else {
			CMDPARA_CHARPOS c{};
			c.byCmd = command; c.pID = c.dwTblID = id; c.dwMapID = 1;
			c.byX = mx; c.byY = my; c.byDirection = direction; c.byFlg = serial % 2;
			// 不明キャラ・別マップ・範囲外・消滅通知も通す。
			if (serial % 11 == 0 && i == 5) c.byDirection = 99;
			if (serial % 13 == 0 && i == 8) c.dwMapID = 2;
			if (serial % 17 == 0 && i == 11) c.byX = 64;
			memcpy(packet.data(), &c, sizeof(c));
			end = packet.data() + sizeof(c);
		}
		packet.resize(end - packet.data());
		messages.push_back({command, std::move(packet)});
		// 受信途中の参照・描画、向き変更、扉・マップ属性変更を混ぜる。
		if (i == 7 || i == 17) messages.push_back({CMD_CHGDIRECTION, {BYTE(rng() % 4)}});
		if (i == 12 && serial % 2 == 0) messages.push_back({CMD_MAPINFO, {}});
	}
	return messages;
}
static void PrepareCase(int serial, int count, int x, int y)
{
	Prepare(count, serial % 6 == 0 || serial % 6 == 3 ? 0 : 20, serial % 6 >= 2, serial % 3 == 0 ? 0 : MAX_MAPLIGHT);
	mapInfo.dwID = 1;
	if (serial % 6 == 3) mapInfo.byViewRange = 3 + serial % 5;
	if (serial % 6 >= 4) {
		mapInfo.byDarkAreaCount = 2;
		mapInfo.aDarkArea[0] = {0, 0, 31, 63, BYTE(serial % 6 == 4 ? MAPDARK_HIDE : MAPDARK_BRIGHT)};
		mapInfo.aDarkArea[1] = {30, 1, 60, 62, MAPDARK_HIDE};
	}
	mapInfo.byFlg |= serial % 5 == 0 ? MAPFLG_NOCHARLIGHT : 0;
	parts[2].byHitType = serial % 2 ? PARTSHIT_FOOTMARK_SNOW : PARTSHIT_COUNTER;
	parts[2].byLight = serial % 4 == 0 ? MAPLIGHT_RANGE_MAX : 0;
	for (int i = 0; i < 4096; i ++) if (i % 113 == 0) tiles[i] = 2;
	occupancy.fill(0);
	for (int i = 0; i < count; i ++) {
		auto *c = g_aCharInfo[i];
		c->dwX = Clamp(x + int(c->dwX) - 32);
		c->dwY = Clamp(y + int(c->dwY) - 32);
		c->byNpcType = i % 4 == 0 ? NPCTYPE_QUESTNPC : 0;
		c->byDirection = i % 4;
		c->byDotMove = (serial + i) % 33;
		if (serial % 10 == 0) c->byLightLevel = 0;
		if (serial % 10 == 1) c->byLightLevel = 19;
		occupancy[c->dwX + 64 * c->dwY] ++;
	}
	g_aCharInfo[0]->byTurnMagicLV = serial % 3 == 0 ? 1 : 0;
	for (int i = 0; i < mapInfo.byLightCount; i ++) {
		auto &l = mapInfo.aLight[i];
		l.wX = Clamp(x + (serial % 8 == 0 ? 0 : (i % 4 - 2) * 4));
		l.wY = Clamp(y + (serial % 8 == 0 ? 0 : (i / 4 - 2) * 4));
		l.byRange = serial % 3 == 1 ? 0 : MAPLIGHT_RANGE_MAX;
		l.byPower = (i * 17) % 101;
		l.byFlg = i % 4;
	}
	for (int i = 0; i < 10; i ++) g_aRecTbl.Add(i + 1);
	warInfo.byMove = 100;
	g_nProcChar = 0;
	tradeInfo.dwTblID = 2;
	g_pTradeInfo = serial % 3 == 0 ? &tradeInfo : nullptr;
}
struct Result {
	std::vector<Buffer> receive, draw;
	std::vector<DWORD> pixels;
};
static Result Replay(int serial, int count, int x, int y, const std::vector<Message> &messages, bool old)
{
	useOld = old;
	PrepareCase(serial, count, x, y);
	if (old) RenewCharPosInfoOld(); else RenewCharPosInfo();
	Result result;
	for (size_t i = 0; i < messages.size(); i ++) {
		testTime = (serial * 631 + i * 17) % 600000;
		const auto &message = messages[i];
		auto data = message.data;
		if (!old) PrepareRecvCharPosInfo(message.command);
		if (message.command == CMD_CHARINFO) {
			if (old) RecvDataProcCHARINFOOld(data.data(), data.size()); else RecvDataProcCHARINFO(data.data(), data.size());
		} else if (message.command == CMD_CHARINFO2) {
			if (old) RecvDataProcCHARINFO2Old(data.data(), data.size()); else RecvDataProcCHARINFO2(data.data(), data.size());
		} else if (message.command == CMD_CHARPOS) {
			if (old) RecvDataProcCHARPOSOld(data.data(), data.size()); else RecvDataProcCHARPOS(data.data(), data.size());
		} else {
			result.draw.push_back(DrawState());
			if (message.command == CMD_CHGDIRECTION && g_nMyCharID >= 0)
				g_aCharInfo[g_nMyCharID]->byDirection = data[0];
			else {
				// 更新を伴わない入力変更でも、直前の保留を先に確定しておく。
				mapInfo.byLightLevel = serial % 2 ? 20 : 0;
				mapInfo.byViewRange = 4;
				tiles[32 * 64 + 32] ^= 1;
			}
		}
		result.receive.push_back(ReceiveState());
	}
	if (!old) FlushCharPosInfo();
	result.draw.push_back(DrawState());
	for (size_t i = 0; i < dib.pixels.size(); i ++) dib.pixels[i] = 0xa3000000u + DWORD(i * 7919 + serial);
	if (g_nMyCharID >= 0 && GetMapLightLevel()) DrawMapLight(dark);
	result.pixels = dib.pixels;
	return result;
}
static void Compare(int serial, int count, int x, int y)
{
	if (std::getenv("SBO_TRACE")) std::fprintf(stderr, "case %d (%d,%d)\n", serial, x, y);
	auto messages = Messages(serial, x, y);
	auto old = Replay(serial, count, x, y, messages, true);
	auto newer = Replay(serial, count, x, y, messages, false);
	if (old.receive != newer.receive || old.draw != newer.draw || old.pixels != newer.pixels) {
		std::fprintf(stderr, "不一致 serial=%d count=%d pos=(%d,%d) 受信=%d 描画入力=%d 画素=%d\n", serial, count, x, y,
			old.receive == newer.receive, old.draw == newer.draw, old.pixels == newer.pixels);
		std::exit(1);
	}
}
static void BenchmarkBatch(int count, int level, bool walls, int lights)
{
	constexpr int repeats = 30, rounds = 5, notifications = 20;
	std::array<double, rounds> oldTimes, newTimes;
	auto measure = [&](bool old) {
		Prepare(count, level, walls, lights);
		RenewCharPosInfo();
		auto begin = std::chrono::steady_clock::now();
		for (int frame = 0; frame < repeats; frame ++) {
			for (int i = 0; i < notifications; i ++) {
				// 受信で座標や灯りが変わった後の、更新にかかる時間のみを測る。
				g_aCharInfo[1]->byLightLevel = (i + frame) % 10;
				if (old) RenewCharPosInfoOld(); else RequestCharPosInfo();
			}
			if (!old) FlushCharPosInfo();
			checksum = dark[144];
		}
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() / repeats;
	};
	measure(true); measure(false);
	for (int round = 0; round < rounds; round ++) {
		if (round % 2) { newTimes[round] = measure(false); oldTimes[round] = measure(true); }
		else { oldTimes[round] = measure(true); newTimes[round] = measure(false); }
	}
	std::sort(oldTimes.begin(), oldTimes.end()); std::sort(newTimes.begin(), newTimes.end());
	std::printf("20通知/フレーム %dキャラ level=%d 壁=%d 光源=%d: 旧 %.6f → 新 %.6f ms\n",
		count, level, walls, lights, oldTimes[2], newTimes[2]);
}

int main()
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	if (!std::getenv("SBO_CHECK_ONLY")) for (int count : {50, 100, 200}) {
		Benchmark(count, 0, false, 0);
		Benchmark(count, 20, false, 0);
		Benchmark(count, 20, true, 0);
		Benchmark(count, 20, true, MAX_MAPLIGHT);
	}
	int cases = 0;
	for (int serial = 0; serial < 768; serial ++) {
		Compare(serial, 50 + (serial % 4) * 50, 32, 32);
		cases ++;
	}
	for (int x : {0, 1, 7, 8, 31, 32, 55, 56, 62, 63})
		for (int y : {0, 1, 31, 32, 62, 63}) for (int mode = 0; mode < 6; mode ++) {
			Compare(768 + mode + x * 6 + y * 60, mode % 2 ? 200 : 50, x, y);
			cases ++;
		}
	std::printf("完全一致: %d 受信列 (各24通知)、受信結果・行別一覧・灯り・視界記録・544×544 RGBA、不一致0\n", cases);
	BenchmarkBatch(50, 0, false, 0);
	BenchmarkBatch(50, 20, true, 0);
	BenchmarkBatch(200, 20, true, MAX_MAPLIGHT);
	Prepare(0, 0, false, 0);
	ResetMapViewSeen ();
}
