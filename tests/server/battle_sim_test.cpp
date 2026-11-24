/* バランスシミュレータ（common/BattleSim.h）の敵の特技・回復が正しく働くかの native テスト。
   実行: g++ -std=c++11 -O1 -I. tests/server/battle_sim_test.cpp -o /tmp/battle_sim_test && /tmp/battle_sim_test */
#include <stdio.h>
#include <random>
#include "common/BattleSim.h"

struct Rng {
    std::mt19937 mt;
    Rng(unsigned seed) : mt(seed) {}
    long operator()() { return (long)(mt() >> 1); }
};

static int g_bad = 0;
#define CHECK(c, ...) do { if (!(c)) { g_bad++; printf ("NG: " __VA_ARGS__); printf ("\n"); } } while (0)

/* 16 枠すべてに同じ特技を入れる */
static void Fill(BattleSim::Enemy &E, int nType, int d1, int d2, int tt, int tg, int area) {
    BattleSim::EnemySkill s = {nType, d1, d2, tt, tg, area};
    for (int i = 0; i < BattleCalc::EM_SLOTS; i++) E.aSkill[i] = s;
}

/* 標準的な近接プレイヤー（特技なし） */
static BattleSim::Player Player() {
    BattleSim::Player P = {10, 200, 0, 30, 10, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    return P;
}

int main() {
    /* 1. 回復持ちの敵は長引く: 同じ敵（HP200・弱い攻撃）に 16 枠のうち 8 枠に回復(50〜60)を持たせると、
          倒すまでの時間が増え、回復の回数が 0 より多い。持たせなければ回復は 0 回 */
    {
        BattleSim::Player P = Player();
        BattleSim::Enemy A = {200, 5, 5, 100, 1, 1, 1, 3, 20};     /* 特技なし */
        BattleSim::Enemy B = A;
        Fill (B, BattleCalc::EMT_HPUP, 50, 60, BattleCalc::EMTT_SELF, 0, 0);
        for (int i = 8; i < BattleCalc::EM_SLOTS; i++) { BattleSim::EnemySkill none = {0, 0, 0, 0, 0, 0}; B.aSkill[i] = none; }
        Rng r1 (1), r2 (1);
        BattleSim::Summary a = BattleSim::Run (P, A, 400, r1), b = BattleSim::Run (P, B, 400, r2);
        printf ("特技なし 勝率%.2f %.1f秒 / 回復持ち 勝率%.2f %.1f秒 回復%.2f回\n", a.dWinRate, a.dSec, b.dWinRate, b.dSec, b.dEnemyHeal);
        CHECK (a.dEnemyHeal == 0 && a.dEnemyMagic == 0, "特技なしの敵が特技を使った");
        CHECK (b.dEnemyHeal > 0.5, "回復持ちが回復していない (%.2f)", b.dEnemyHeal);
        CHECK (b.dSec > a.dSec * 1.3, "回復持ちが長引いていない (%.1f 秒 vs %.1f 秒)", b.dSec, a.dSec);
    }
    /* 2. HP が 70% 以上のうちは回復しない: 殴っても 1 ダメージ（おまけ）しか入らない相手に 16 枠すべて回復を持たせ、
          5 分戦っても回復 0 回（くじは毎回引くが IsNeedHeal が通らない）。敵の攻撃は 0 */
    {
        BattleSim::Player P = Player();
        P.nHP = 1000000; P.nDF = 1000;
        BattleSim::Enemy E = {1000, 0, 1000, 100, 1, 1, 1, 3, 20};
        Fill (E, BattleCalc::EMT_HPUP, 50, 60, BattleCalc::EMTT_SELF, 0, 0);
        Rng r (3);
        int nHeal = 0, nMagic = 0;
        for (int i = 0; i < 20; i++) {
            BattleSim::Result x = BattleSim::OneBattle (P, E, r);
            nHeal += x.nEnemyHeal; nMagic += x.nEnemyMagic;
        }
        printf ("満タン付近 回復%d回 特技%d回\n", nHeal, nMagic);
        CHECK (nHeal == 0 && nMagic == 0, "HP が多いのに回復した (%d回)", nHeal);
    }
    /* 3. 離れている時は回復しない・攻撃魔法は届く時だけ撃つ: 遠くから近づく敵（開始距離 6）に回復だけを持たせ、
          プレイヤーが倒せるまで一度も回復しない（近づく間は離れているので、HP満タンでなくても使わない扱い）。
          前方 3 マスの攻撃魔法なら、近づいて 3 マス以内になってから撃つので、撃たれた分だけ HP が減る */
    {
        BattleSim::Player P = Player();
        BattleSim::Enemy E = {1, 0, 0, 0, 1, 6, 1, 8, 100};   /* 素早さ0=5秒に1回の行動 */
        Fill (E, BattleCalc::EMT_HPUP, 50, 60, BattleCalc::EMTT_SELF, 0, 0);
        Rng r (4);
        BattleSim::Summary s = BattleSim::Run (P, E, 50, r);
        CHECK (s.dEnemyHeal == 0, "離れている敵が回復した (%.2f)", s.dEnemyHeal);
        BattleSim::Enemy M = {100000, 0, 0, 255, 1, 6, 1, 8, 100};
        Fill (M, BattleCalc::EMT_HPDOWN, 5, 8, BattleCalc::EMTT_FRONT, 1, 3);
        P.nHP = 100000;
        BattleSim::Result x = BattleSim::OneBattle (P, M, r);
        CHECK (x.nEnemyMagic > 0, "前方魔法を撃たない");
    }
    printf (g_bad ? "NG\n" : "一致\n");
    return g_bad ? 1 : 0;
}
