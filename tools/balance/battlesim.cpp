/* バランステスト用の戦闘シミュレータ CLI（native、emscripten 不要）。
   ビルド: g++ -std=c++11 -O2 -I. tools/balance/battlesim.cpp -o /tmp/battlesim
   実行:   /tmp/battlesim [seed] [試行回数] < scenarios.tsv
   入力形式は tools/balance/README.md を参照。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random>
#include <string>
#include <vector>
#include "common/BattleSim.h"

/* 入力の列（タブ区切り、数値は整数）。先頭の名前以外は全部整数 */
static const int COLS = 24;

/* 乱数。BattleCalc は「サーバーの genrand は符号付き long」を前提に int と混ぜて計算するので、
   unsigned のまま返すとマイナスのダメージ計算が符号なしに化ける。0 以上の long で返す。 */
struct Rng {
    std::mt19937 mt;
    Rng(unsigned seed) : mt(seed) {}
    long operator()() { return (long)(mt() >> 1); }
};

static std::vector<std::string> Split(const std::string &s) {
    std::vector<std::string> v;
    size_t b = 0;
    for (;;) {
        size_t p = s.find ('\t', b);
        if (p == std::string::npos) { v.push_back (s.substr (b)); break; }
        v.push_back (s.substr (b, p - b));
        b = p + 1;
    }
    return v;
}

int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? (unsigned)strtoul (argv[1], NULL, 10) : 1;
    int nRuns = argc > 2 ? atoi (argv[2]) : 1000;
    Rng rng (seed);
    char buf[4096];
    int nLine = 0;
    printf ("名前\t勝率%%\t残HP%%\t所要秒\t回復回数\t判定\n");
    while (fgets (buf, sizeof buf, stdin)) {
        std::string line = buf;
        while (!line.empty () && (line[line.size () - 1] == '\n' || line[line.size () - 1] == '\r')) line.erase (line.size () - 1);
        nLine++;
        if (line.empty () || line[0] == '#') continue;
        std::vector<std::string> c = Split (line);
        if ((int)c.size () != COLS) {
            fprintf (stderr, "error: %d行目 列数 %d（%d 列必要）\n", nLine, (int)c.size (), COLS);
            return 1;
        }
        int v[COLS];
        for (int i = 1; i < COLS; i++) v[i] = atoi (c[i].c_str ());
        BattleSim::Player P = {v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14]};
        BattleSim::Enemy E = {v[15], v[16], v[17], v[18], v[19], v[20], v[21], v[22], v[23]};
        BattleSim::Summary s = BattleSim::Run (P, E, nRuns, rng);
        printf ("%s\t%.1f\t%.0f\t%.1f\t%.2f\t%s\n", c[0].c_str (), s.dWinRate * 100, s.dHpRate * 100, s.dSec, s.dHeal, s.pszVerdict);
    }
    return 0;
}
