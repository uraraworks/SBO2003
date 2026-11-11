// DrawMapLight の描画部分を、同じ入力・同じ背景で完全比較する。
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>

using std::min;
using std::max;
using BYTE = uint8_t;
using DWORD = uint32_t;
using PDWORD = DWORD *;
constexpr int DRAW_PARTS_X = 7, DRAW_PARTS_Y = 7;
constexpr int SCRSIZEX = 480, SCRSIZEY = 480, MAPLIGHT_MAX = 32;
constexpr int SCRMARTSSIZE = 17 * 17, MAPLIGHT_RANGE_MAX = 7;
const int anPosAddTblX[] = {0, 0, -1, 1}, anPosAddTblY[] = {-1, 1, 0, 0};

struct Scene {
	int nWidth = 544, nHeight = 544, nMapW = 17, nLight = 32;
	int anX[MAPLIGHT_MAX], anY[MAPLIGHT_MAX];
	float afR[MAPLIGHT_MAX], afPower[MAPLIGHT_MAX], afWarm[MAPLIGHT_MAX];
	std::array<BYTE, SCRMARTSSIZE> masks[MAPLIGHT_MAX];
	const BYTE *apMask[MAPLIGHT_MAX];
	BYTE abyHide[SCRMARTSSIZE];
	std::array<char, SCRMARTSSIZE> dark;
	bool bHide = false;
	struct Character { int byDirection = 0, byDotMove = 0; } character;
	std::vector<DWORD> background;
	void Old(std::vector<DWORD> &buffer) const;
	void New(std::vector<DWORD> &buffer) const;
};

// run-draw-map-light.py が旧実装と本体から抽出した新実装をここに定義する。
#include "draw-map-light-functions.inc"

static std::mt19937 randomBits(0x53424f32);
static int Pick(int limit) { return randomBits() % limit; }

static void Prepare(Scene &s, int serial)
{
	s.nWidth = serial % 8 ? 1 + Pick(544) : 544;
	s.nHeight = serial % 8 ? 1 + Pick(544) : 544;
	s.nLight = serial % 33;
	s.character = {serial % 4, serial % 33};
	s.bHide = (serial % 3 == 0);
	s.background.resize(s.nWidth * s.nHeight);
	for (auto &pixel : s.background) pixel = randomBits();
	const char darkness[] = {-1, 0, 1, 2, 100};
	for (int j = 0; j < SCRMARTSSIZE; j ++) {
		s.dark[j] = darkness[Pick(5)];
		s.abyHide[j] = Pick(2);
	}
	float fTime = (float)Pick(600000);
	for (int i = 0; i < MAPLIGHT_MAX; i ++) {
		int mx = Pick(32), my = Pick(32);
		float fFlicker = 1.0f + 0.045f * sinf(fTime * 0.0093f + (mx * 7 + my * 13)) +
						0.025f * sinf(fTime * 0.0211f + (mx * 3 + my * 5));
		s.afR[i] = (Pick(MAPLIGHT_RANGE_MAX) + 1 + 1 + 0.2f) * 32.0f * fFlicker;
		s.anX[i] = Pick(1057) - 256;
		s.anY[i] = Pick(1057) - 256;
		s.afPower[i] = Pick(256) / 100.0f;
		s.afWarm[i] = (float)Pick(2);
		for (auto &v : s.masks[i]) v = Pick(2);
		s.apMask[i] = Pick(3) ? s.masks[i].data() : nullptr;
		if (serial % 7 == 0) {
			// キャラの灯りの式・各ゆらぎ位相。最小減光量 1 が最大半径。
			fFlicker = 1.0f + 0.035f * sinf(fTime * 0.0071f) +
					0.020f * sinf(fTime * 0.0173f + 1.7f) +
					0.012f * sinf(fTime * 0.0311f + 0.4f);
			s.afR[i] = (100.0f / (1 + Pick(100)) + 0.2f) * 32.0f * fFlicker;
		}
	}
	if (serial % 11 == 0) s.afR[Pick(MAPLIGHT_MAX)] = 0.0f;
}

static int cases = 0;
static void Check(const Scene &s)
{
	auto oldBuffer = s.background, newBuffer = s.background;
	s.Old(oldBuffer);
	s.New(newBuffer);
	// 描画範囲外も含め、アルファを含む全バイトを比較する。
	if (memcmp(oldBuffer.data(), newBuffer.data(), oldBuffer.size() * sizeof(DWORD))) {
		for (size_t p = 0; p < oldBuffer.size(); p ++) {
			if (oldBuffer[p] != newBuffer[p]) {
				std::fprintf(stderr, "不一致: case=%d pixel=%zu old=%08x new=%08x\n",
					cases, p, oldBuffer[p], newBuffer[p]);
				std::exit(1);
			}
		}
	}
	cases ++;
}

static volatile DWORD checksum;
static void Benchmark(Scene &s, const char *name)
{
	constexpr int repeats = 60, rounds = 5;
	std::vector<DWORD> buffer = s.background;
	auto measure = [&](bool newer) {
		auto begin = std::chrono::steady_clock::now();
		for (int i = 0; i < repeats; i ++) {
			// 背景復元も両方に同じ条件で含める。
			buffer = s.background;
			if (newer) s.New(buffer); else s.Old(buffer);
			checksum = buffer[buffer.size() / 2];
		}
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() / repeats;
	};
	measure(false);
	measure(true);
	std::array<double, rounds> oldTimes, newTimes;
	for (int r = 0; r < rounds; r ++) {
		// 測定順を交互にして、各 5 回の中央値を採る。
		if (r % 2) { newTimes[r] = measure(true); oldTimes[r] = measure(false); }
		else { oldTimes[r] = measure(false); newTimes[r] = measure(true); }
	}
	std::sort(oldTimes.begin(), oldTimes.end());
	std::sort(newTimes.begin(), newTimes.end());
	std::printf("%s: 旧 %.3f ms → 新 %.3f ms (%.2f 倍)\n", name,
		oldTimes[rounds / 2], newTimes[rounds / 2], oldTimes[rounds / 2] / newTimes[rounds / 2]);
}

int main()
{
	Scene s;
	for (int serial = 0; serial < 2048; serial ++) {
		Prepare(s, serial);
		Check(s);
	}
	// 画面端・半径境界の直前/一致/直後、光源0/1/32、重なり、無限灯りの順序。
	Prepare(s, 0);
	s.bHide = false;
	s.dark.fill(100);
	const int positions[] = {-256, -1, 0, 16, 32, 271, 511, 543, 544, 800};
	const float radii[] = {0.0f, 1.0f, 16.0f, 32.0f, 38.4f,
		(MAPLIGHT_RANGE_MAX + 1 + 0.2f) * 32.0f * 1.07f,
		(100.0f + 0.2f) * 32.0f * 1.067f};
	for (int count : {0, 1, 32}) {
		s.nLight = count;
		for (int pos : positions) for (float radius : radii) for (int edge = -1; edge <= 1; edge ++) {
			float r = edge < 0 ? std::nextafter(radius, 0.0f) :
				(edge > 0 ? std::nextafter(radius, INFINITY) : radius);
			for (int i = 0; i < MAPLIGHT_MAX; i ++) {
				s.anX[i] = pos;
				s.anY[i] = positions[(i + count) % 10];
				s.afR[i] = r;
				s.afPower[i] = i % 3 ? 0.6f : 1.0f;
				s.apMask[i] = i % 2 ? s.masks[i].data() : nullptr;
			}
			Check(s);
		}
	}
	for (int zero = 0; zero < MAPLIGHT_MAX; zero ++) {
		s.nLight = MAPLIGHT_MAX;
		for (int i = 0; i < MAPLIGHT_MAX; i ++) {
			s.anX[i] = s.anY[i] = 256;
			s.afR[i] = i == zero ? 0.0f : 96.0f;
		}
		Check(s);
	}
	// 切り詰め・補間の境界を通る全方向、全歩行オフセット。
	for (int direction = 0; direction < 4; direction ++) for (int dot = 0; dot <= 32; dot ++) {
		s.character = {direction, dot};
		s.bHide = dot % 2;
		Check(s);
	}
	std::printf("完全一致: %d ケース、不一致 0\n", cases);
	// 32 光源が画面内外に分散する、全480×480画素が見える画面。
	Prepare(s, 0);
	s.nLight = 32;
	s.bHide = false;
	s.dark.fill(100);
	for (int i = 0; i < 32; i ++) {
		s.anX[i] = (i % 8) * 96 - 64;
		s.anY[i] = (i / 8) * 192 - 64;
		s.afR[i] = 96.0f;
	}
	Check(s);
	Benchmark(s, "分散32光源・半径96");
	for (int i = 0; i < 32; i ++) s.afR[i] = radii[5];
	Check(s);
	Benchmark(s, "分散32光源・マップ最大半径");
	s.nLight = 0;
	Check(s);
	Benchmark(s, "光源0");
	std::printf("計測入力を含む合計: %d ケース、不一致 0\n", cases);
}
