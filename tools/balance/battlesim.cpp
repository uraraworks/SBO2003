/* バランステスト用の戦闘シミュレータ CLI（native、emscripten 不要）。
   ビルド: g++ -std=c++11 -O2 -I. tools/balance/battlesim.cpp -o /tmp/battlesim
   実行:   /tmp/battlesim [seed] [試行回数] [敵の狙い方 0|1] [連携 1|0] < scenarios.tsv
   入力形式は tools/balance/README.md を参照。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random>
#include <string>
#include <vector>
#include "common/BattleSim.h"

/* 入力の列（タブ区切り）。先頭の名前と最後の特技列以外は全部整数。
   24 列目までが必須、25 列目（敵の特技）と 26 列目（追加の味方）は省略できる（省略＝特技なし・1人戦） */
static const int COLS = 24;
static const int COLS_SKILL = 25;
static const int COLS_PARTY = 26;

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

/* 追加の味方の列: 1 人を "Lv,HP,Mana,攻撃力,守備力,素早さ,武器byMin,回復Mana,回復量,回復time,攻撃特技Mana,攻撃特技平均,攻撃特技time,AT型"（14 値）で書き、
   複数人は '|' で区切る（最大 PARTY_MAX-1 人）。1 列目のプレイヤー（列 2〜15）が先頭＝前衛。"-" や空は追加なし */
static bool ParseAllies(const std::string &s, BattleSim::Player *a, int &n) {
    if (s.empty () || s == "-") return true;
    size_t b = 0;
    for (;;) {
        size_t p = s.find ('|', b);
        std::string t = s.substr (b, p == std::string::npos ? std::string::npos : p - b);
        int v[14];
        if (n >= BattleSim::PARTY_MAX ||
            sscanf (t.c_str (), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13]) != 14) return false;
        BattleSim::Player q = {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], 0};
        a[n++] = q;
        if (p == std::string::npos) break;
        b = p + 1;
    }
    return true;
}

int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? (unsigned)strtoul (argv[1], NULL, 10) : 1;
    int nRuns = argc > 2 ? atoi (argv[2]) : 1000;
    int nRule = argc > 3 ? atoi (argv[3]) : 0;
    int bRenkei = argc > 4 ? atoi (argv[4]) : 1;
    Rng rng (seed);
    char buf[4096];
    int nLine = 0;
    printf ("名前\t勝率%%\t残HP%%\t所要秒\t回復回数\t判定\t敵特技回数\t敵回復回数\t倒れた人数\n");
    while (fgets (buf, sizeof buf, stdin)) {
        std::string line = buf;
        while (!line.empty () && (line[line.size () - 1] == '\n' || line[line.size () - 1] == '\r')) line.erase (line.size () - 1);
        nLine++;
        if (line.empty () || line[0] == '#') continue;
        std::vector<std::string> c = Split (line);
        if ((int)c.size () != COLS && (int)c.size () != COLS_SKILL && (int)c.size () != COLS_PARTY) {
            fprintf (stderr, "error: %d行目 列数 %d（%d 列、特技つきの %d 列、または味方つきの %d 列）\n", nLine, (int)c.size (), COLS, COLS_SKILL, COLS_PARTY);
            return 1;
        }
        int v[COLS];
        for (int i = 1; i < COLS; i++) v[i] = atoi (c[i].c_str ());
        BattleSim::Player P = {v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], 0};
        BattleSim::Enemy E = {v[15], v[16], v[17], v[18], v[19], v[20], v[21], v[22], v[23]};
        if ((int)c.size () >= COLS_SKILL && !ParseSkills (c[24], E.aSkill)) {
            fprintf (stderr, "error: %d行目 特技の列が読めない: %s\n", nLine, c[24].c_str ());
            return 1;
        }
        BattleSim::Player party[BattleSim::PARTY_MAX];
        int nParty = 1;
        party[0] = P;
        if ((int)c.size () == COLS_PARTY && !ParseAllies (c[25], party, nParty)) {
            fprintf (stderr, "error: %d行目 味方の列が読めない（14 値を , で、人は | で区切る。最大 %d 人）: %s\n", nLine, BattleSim::PARTY_MAX - 1, c[25].c_str ());
            return 1;
        }
        BattleSim::Summary s = BattleSim::RunParty (party, nParty, E, nRuns, rng, nRule, bRenkei);
        printf ("%s\t%.1f\t%.0f\t%.1f\t%.2f\t%s\t%.2f\t%.2f\t%.2f\n", c[0].c_str (), s.dWinRate * 100, s.dHpRate * 100, s.dSec, s.dHeal, s.pszVerdict, s.dEnemyMagic, s.dEnemyHeal, s.dDead);
    }
    return 0;
}
