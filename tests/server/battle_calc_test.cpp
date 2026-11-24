/* common/BattleCalc.h が旧実装（SboSvrBattleProc.cpp の DamageProc / GetWeponMin）と
   同じ結果・同じ乱数の消費順序になることを確かめる native テスト。
   実行: g++ -std=c++11 -O1 -I. tests/server/battle_calc_test.cpp -o /tmp/battle_calc_test && /tmp/battle_calc_test */
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "common/BattleCalc.h"

static int omin(int a, int b) { return a < b ? a : b; }
static int omax(int a, int b) { return a > b ? a : b; }

/* 乱数の記録つき供給元。負の値も含める（サーバーの genrand は long を返す） */
struct Src {
    unsigned long long s;
    std::vector<long> log;
    Src(unsigned long long seed) : s(seed) {}
    long operator()() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        long v = (long)(int)(s >> 32);
        log.push_back(v);
        return v;
    }
};

/* 旧実装（git show HEAD:SboSvr/src/SboSvrBattleProc.cpp から式を写したもの） */
static int oldWeaponMin(int byMin, Src &g) {
    int nTmp = 0;
    if (byMin) {
        nTmp = ((g() % byMin) + 1);
    }
    return nTmp;
}
static int oldDamage(int wAT, int byAtackRate, int wDF, bool bSPAtack, bool bGuard, int nRate, int byMin, Src &g) {
    int nTmp, nAT, nDF;
    if (byAtackRate > 0) {
        nAT = wAT * omin(byAtackRate, 100) / 100;
    } else {
        nAT = wAT;
    }
    nDF = wDF;
    if (bSPAtack == false) {
        nTmp = oldWeaponMin(byMin, g);
        nAT -= nTmp;
    }
    nAT = omax(nAT, 2);
    if (bSPAtack) {
        nAT += (nAT / 2);
    }
    nTmp = nAT - (nDF / 2);
    if (g() % 2) {
        nTmp += (nTmp * ((g() % 16) + 1) / 100);
    } else {
        nTmp -= (nTmp * ((g() % 16) + 1) / 100);
    }
    nTmp = omax(nTmp, 0);
    if (bGuard) {
        nTmp /= 2;
    }
    nTmp = nTmp * nRate / 100;
    if ((nTmp == 0) && ((byAtackRate == 0) || (byAtackRate >= 100))) {
        if ((g() % 100) > 66) {
            nTmp = 1;
        }
    }
    return nTmp;
}

/* サーバー側の呼び方（GetWeponMin → BattleCalc::Damage） */
static int newDamage(int wAT, int byAtackRate, int wDF, bool bSPAtack, bool bGuard, int nRate, int byMin, Src &g) {
    int dec = bSPAtack ? 0 : BattleCalc::WeaponMinRoll(byMin, g);
    return BattleCalc::Damage(wAT, byAtackRate, wDF, bSPAtack, bGuard, nRate, dec, g);
}

int main() {
    const int rates[] = {0, 1, 5, 20, 50, 99, 100, 101};
    const int nrates[] = {100, 120, 150, 200, 50};
    const int mins[] = {0, 1, 2, 5, 10, 50, 255};
    long cases = 0, bad = 0;
    unsigned long long seed = 1;
    for (int at = 0; at <= 400; at += 7)
        for (int df = 0; df <= 400; df += 13)
            for (size_t r = 0; r < sizeof(rates) / sizeof(*rates); r++)
                for (int sp = 0; sp < 2; sp++)
                    for (int gd = 0; gd < 2; gd++)
                        for (size_t n = 0; n < sizeof(nrates) / sizeof(*nrates); n++)
                            for (size_t m = 0; m < sizeof(mins) / sizeof(*mins); m++) {
                                seed++;
                                Src a(seed * 2654435761ULL), b(seed * 2654435761ULL);
                                int x = oldDamage(at, rates[r], df, sp, gd, nrates[n], mins[m], a);
                                int y = newDamage(at, rates[r], df, sp, gd, nrates[n], mins[m], b);
                                cases++;
                                if (x != y || a.log != b.log) {
                                    if (bad++ < 10) printf("不一致 at=%d df=%d rate=%d sp=%d gd=%d nr=%d min=%d old=%d new=%d calls %zu/%zu\n",
                                        at, df, rates[r], sp, gd, nrates[n], mins[m], x, y, a.log.size(), b.log.size());
                                }
                            }
    printf("%ld 通り, 不一致 %ld\n", cases, bad);
    printf(bad ? "NG\n" : "一致\n");
    return bad ? 1 : 0;
}
