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
    bool nonneg;    /* true: 0 以上だけ返す（枠の添字に使うくじは、サーバーの genrand が 0 以上である前提） */
    Src(unsigned long long seed, bool nn = false) : s(seed), nonneg(nn) {}
    long operator()() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        long v = (long)(int)(s >> 32);
        if (nonneg) v &= 0x7fffffffL;
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

/* ---- 敵の特技（SboSvrBattleProc.cpp の ChooseEnemyMagic、SboSvrMagicProc.cpp の HPDOWN / HPDOWNAT / HPDOWNRATE / HPUP の式を写したもの） ---- */
struct OldSlot { int kind; bool needHeal, reach; };   /* kind 0=空き 1=回復 2=その他（g_aMagicInfo を引く所は呼び出し側の結果） */
static int oldChooseEnemyMagic(int wHP, int wMaxHP, const OldSlot *sl, bool bRange, Src &genrand) {
    int i, nNo, nCount, anHeal[16];
    if ((wMaxHP > 0) && ((int)wHP * 100 <= (int)wMaxHP * 30) && ((int)(genrand() % 100) < 50)) {
        nCount = 0;
        for (i = 0; i < 16; i++) {
            if (sl[i].kind == 1) anHeal[nCount++] = i;
        }
        if (nCount > 0) {
            return anHeal[genrand() % nCount];
        }
    }
    nNo = (int)(genrand() % 16);
    if (sl[nNo].kind == 0) return -1;
    if (sl[nNo].kind == 1) {
        if (bRange || (sl[nNo].needHeal == false)) return -1;
        return nNo;
    }
    if (bRange && (sl[nNo].reach == false)) return -1;
    return nNo;
}
static int oldHpDown(int d1, int d2, int dfm, Src &genrand) {
    int nTmp, nTmp2, nTmp3;
    nTmp = abs(d2 - d1 + 1);
    nTmp = omax(1, nTmp);
    nTmp2 = (d1 + (genrand() % nTmp));
    nTmp3 = dfm;
    nTmp2 -= (nTmp3 / 2);
    nTmp2 = omax(0, nTmp2);
    return nTmp2;
}
static int oldHpDownAT(int d1, int d2, int at, int df, Src &genrand) {
    int nTmp, nTmp2, nTmp3;
    nTmp = abs(d2 - d1 + 1);
    nTmp = omax(1, nTmp);
    nTmp2 = (d1 + (genrand() % nTmp));
    nTmp2 += at;
    nTmp3 = df;
    nTmp2 -= (nTmp3 / 2);
    nTmp2 = omax(0, nTmp2);
    return nTmp2;
}
static int oldHpDownRate(int hp, int d1, int dfm) {
    int nTmp, nTmp2, nTmp3;
    nTmp = (hp * 10 * d1 / 1000);
    nTmp2 = omax(1, nTmp);
    nTmp3 = dfm;
    nTmp2 -= (nTmp3 / 2);
    nTmp2 = omax(0, nTmp2);
    return nTmp2;
}
static int oldHpUp(int d1, int d2, int hp, int maxhp, Src &genrand) {
    unsigned short wTmp;
    int nTmp;
    if (hp == maxhp) return 0;
    nTmp = abs(d2 - d1 + 1);
    nTmp = omax(1, nTmp);
    wTmp = (unsigned short)(d1 + ((unsigned short)genrand() % nTmp));
    if (wTmp + hp > maxhp) {
        wTmp = (unsigned short)(maxhp - hp);
    }
    return wTmp;
}

/* 敵の特技の式・くじが旧実装と同じ結果・同じ乱数の消費順序か。戻り値は不一致の数 */
static long testEnemyMagic() {
    long bad = 0, cases = 0;
    unsigned long long seed = 7;
    /* ChooseEnemyMagic: 枠の並び・HP・bRange をいろいろ変えて比べる（HP は 30% 前後・70% 前後を多めに） */
    for (int pat = 0; pat < 400; pat++) {
        OldSlot os[16]; BattleCalc::EnemySlot ns[16];
        for (int i = 0; i < 16; i++) {
            int k = (int)((pat * 7 + i * 13 + (pat >> 3) * i) % 3);
            if (pat % 5 == 0) k = (i < 4) ? 1 : 0;       /* 回復だけ */
            if (pat % 11 == 0) k = 2;                     /* 攻撃だけ */
            os[i].kind = k; os[i].needHeal = ((pat + i) % 2) == 0; os[i].reach = ((pat + i) % 3) != 0;
            ns[i].nKind = k; ns[i].bNeedHeal = os[i].needHeal; ns[i].bReach = os[i].reach;
        }
        const int maxhps[] = {0, 1, 10, 100, 1000};
        for (size_t m = 0; m < sizeof(maxhps) / sizeof(*maxhps); m++)
            for (int hp = 0; hp <= maxhps[m] + 1; hp += (maxhps[m] > 20 ? maxhps[m] / 20 : 1))
                for (int rg = 0; rg < 2; rg++)
                    for (int rep = 0; rep < 8; rep++) {
                        seed++;
                        Src a(seed * 2654435761ULL, true), b(seed * 2654435761ULL, true);
                        int x = oldChooseEnemyMagic(hp, maxhps[m], os, rg != 0, a);
                        int y = BattleCalc::ChooseEnemyMagic(hp, maxhps[m], ns, rg != 0, b);
                        cases++;
                        if (x != y || a.log != b.log) {
                            if (bad++ < 10) printf("不一致(くじ) pat=%d hp=%d/%d range=%d old=%d new=%d calls %zu/%zu\n", pat, hp, maxhps[m], rg, x, y, a.log.size(), b.log.size());
                        }
                    }
    }
    /* ダメージ・回復の式 */
    const int d1s[] = {0, 1, 4, 10, 50, 100}, spans[] = {-3, 0, 1, 5, 40, 100};
    for (size_t i = 0; i < sizeof(d1s) / sizeof(*d1s); i++)
        for (size_t j = 0; j < sizeof(spans) / sizeof(*spans); j++) {
            int d1 = d1s[i], d2 = d1 + spans[j];
            if (d2 < 0) continue;
            for (int df = 0; df <= 300; df += 17)
                for (int at = 0; at <= 100; at += 25)
                    for (int hp = 0; hp <= 300; hp += 37)
                        for (int mx = hp; mx <= hp + 300; mx += 150) {
                            seed++;
                            Src a(seed * 2654435761ULL), b(seed * 2654435761ULL);
                            int bd = 0;
                            bd |= oldHpDown(d1, d2, df, a) != BattleCalc::MagicDamageHpDown(d1, d2, df, b);
                            bd |= oldHpDownAT(d1, d2, at, df, a) != BattleCalc::MagicDamageHpDownAT(d1, d2, at, df, b);
                            bd |= oldHpDownRate(hp, d1, df) != BattleCalc::MagicDamageHpDownRate(hp, d1, df);
                            bd |= oldHpUp(d1, d2, hp, mx, a) != BattleCalc::MagicHealAmount(d1, d2, hp, mx, b);
                            cases += 4;
                            if (bd || a.log != b.log) {
                                if (bad++ < 10) printf("不一致(式) d=%d-%d df=%d at=%d hp=%d/%d\n", d1, d2, df, at, hp, mx);
                            }
                        }
        }
    printf("敵の特技 %ld 通り, 不一致 %ld\n", cases, bad);
    return bad;
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
    bad += testEnemyMagic();
    printf(bad ? "NG\n" : "一致\n");
    return bad ? 1 : 0;
}
