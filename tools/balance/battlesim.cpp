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

/* 入力の列（タブ区切り）。先頭の名前と最後の特技列以外は全部整数。
   24 列目までが必須、25 列目（敵の特技）は省略できる（省略＝特技なし） */
static const int COLS = 24;
static const int COLS_SKILL = 25;

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

/* 敵の特技列: 16 枠を ';' で区切る。空き枠は空文字、あれば "種別,基本値1,基本値2,発動種別,使用対象,射程"。
   全体が "-" や空なら特技なし。枠が 16 に足りなければ後ろは空き。 */
static bool ParseSkills(const std::string &s, BattleSim::EnemySkill *a) {
    if (s.empty () || s == "-") return true;
    size_t b = 0;
    for (int k = 0; k < BattleCalc::EM_SLOTS; k++) {
        size_t p = s.find (';', b);
        std::string t = s.substr (b, p == std::string::npos ? std::string::npos : p - b);
        if (!t.empty ()) {
            int v[6];
            if (sscanf (t.c_str (), "%d,%d,%d,%d,%d,%d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return false;
            BattleSim::EnemySkill e = {v[0], v[1], v[2], v[3], v[4], v[5]};
            a[k] = e;
        }
        if (p == std::string::npos) break;
        b = p + 1;
    }
    return true;
}

int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? (unsigned)strtoul (argv[1], NULL, 10) : 1;
    int nRuns = argc > 2 ? atoi (argv[2]) : 1000;
    Rng rng (seed);
    char buf[4096];
    int nLine = 0;
    printf ("名前\t勝率%%\t残HP%%\t所要秒\t回復回数\t判定\t敵特技回数\t敵回復回数\n");
    while (fgets (buf, sizeof buf, stdin)) {
        std::string line = buf;
        while (!line.empty () && (line[line.size () - 1] == '\n' || line[line.size () - 1] == '\r')) line.erase (line.size () - 1);
        nLine++;
        if (line.empty () || line[0] == '#') continue;
        std::vector<std::string> c = Split (line);
        if ((int)c.size () != COLS && (int)c.size () != COLS_SKILL) {
            fprintf (stderr, "error: %d行目 列数 %d（%d 列、または特技つきの %d 列）\n", nLine, (int)c.size (), COLS, COLS_SKILL);
            return 1;
        }
        int v[COLS];
        for (int i = 1; i < COLS; i++) v[i] = atoi (c[i].c_str ());
        BattleSim::Player P = {v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], 0};
        BattleSim::Enemy E = {v[15], v[16], v[17], v[18], v[19], v[20], v[21], v[22], v[23]};
        if ((int)c.size () == COLS_SKILL && !ParseSkills (c[24], E.aSkill)) {
            fprintf (stderr, "error: %d行目 特技の列が読めない: %s\n", nLine, c[24].c_str ());
            return 1;
        }
        BattleSim::Summary s = BattleSim::Run (P, E, nRuns, rng);
        printf ("%s\t%.1f\t%.0f\t%.1f\t%.2f\t%s\t%.2f\t%.2f\n", c[0].c_str (), s.dWinRate * 100, s.dHpRate * 100, s.dSec, s.dHeal, s.pszVerdict, s.dEnemyMagic, s.dEnemyHeal);
    }
    return 0;
}
