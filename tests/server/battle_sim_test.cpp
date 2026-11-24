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
    /* 4. 味方が複数: 1 人のパーティは OneBattle と同じ乱数・同じ結果（既存の 1 人戦を変えていない） */
    {
        BattleSim::Player P = Player();
        BattleSim::Enemy E = {300, 20, 10, 100, 2, 3, 1, 5, 40};
        Fill (E, BattleCalc::EMT_HPDOWN, 3, 6, BattleCalc::EMTT_FRONT, 1, 2);
        for (int i = 4; i < BattleCalc::EM_SLOTS; i++) { BattleSim::EnemySkill none = {0, 0, 0, 0, 0, 0}; E.aSkill[i] = none; }
        Rng r1 (9), r2 (9);
        int nSame = 0;
        for (int i = 0; i < 100; i++) {
            BattleSim::Result a = BattleSim::OneBattle (P, E, r1), b = BattleSim::OneBattleParty (&P, 1, E, r2);
            if (a.bWin == b.bWin && a.nMs == b.nMs && a.nHeal == b.nHeal && a.nEnemyMagic == b.nEnemyMagic && a.dHpRate == b.dHpRate) nSame++;
        }
        CHECK (nSame == 100, "1人のパーティが OneBattle と違う (%d/100 一致)", nSame);
    }
    /* 5. 2 人なら 1 人より勝てる・連携で速く倒せる: HP が多くて硬い敵（1 人では削り切られる）に、同じ戦士 1 人と 2 人で挑む。
          連携あり（2 秒以内に別の人が当てる）の方が、連携なしより所要時間が短い */
    {
        BattleSim::Player P = Player();
        BattleSim::Enemy E = {800, 25, 10, 120, 1, 1, 1, 3, 20};
        BattleSim::Player two[2] = {P, P};
        Rng r1 (11), r2 (11), r3 (11);
        BattleSim::Summary one = BattleSim::Run (P, E, 300, r1);
        BattleSim::Summary duo = BattleSim::RunParty (two, 2, E, 300, r2);
        BattleSim::Summary noRenkei = BattleSim::RunParty (two, 2, E, 300, r3, BattleSim::TARGET_FIRST, 0);
        printf ("1人 勝率%.2f %.1f秒 / 2人 勝率%.2f %.1f秒 倒れ%.2f人 / 連携なし 勝率%.2f %.1f秒\n", one.dWinRate, one.dSec, duo.dWinRate, duo.dSec, duo.dDead, noRenkei.dWinRate, noRenkei.dSec);
        CHECK (duo.dWinRate > one.dWinRate + 0.2, "2人が1人より勝てていない (%.2f vs %.2f)", duo.dWinRate, one.dWinRate);
        CHECK (duo.dSec < noRenkei.dSec * 0.97, "連携で速く倒せていない (%.1f秒 vs %.1f秒)", duo.dSec, noRenkei.dSec);
        CHECK (duo.dSec < one.dSec * 0.6 || one.dWinRate < 0.3, "2人が速く倒せていない (%.1f秒 vs %.1f秒)", duo.dSec, one.dSec);
    }
    /* 6. 回復役は仲間を回復する・敵の狙い方: 殴り役 + 回復役（自分は攻撃しない・回復だけ）。敵は先頭の殴り役を狙い続けるので、
          回復役が仲間のHPが 40% 以下で回復し、回復の回数が 0 より多い。回復役がいない時より勝率が高く、
          狙い方 TARGET_FIRST では先頭が倒れるまで後ろの回復役は殴られない（倒れた人数が回復役ぶん増えない） */
    {
        BattleSim::Player F = Player();
        F.nHP = 150; F.nAT = 40;
        BattleSim::Player H = Player();
        H.nHP = 60; H.nAT = 2; H.nMana = 200; H.nHealMana = 3; H.nHealAvg = 40; H.nHealTime = 0;
        BattleSim::Enemy E = {900, 30, 10, 150, 1, 1, 1, 3, 20};
        BattleSim::Player duo[2] = {F, H};
        BattleSim::Player solo[1] = {F};
        Rng r1 (13), r2 (13), r3 (13);
        BattleSim::Summary a = BattleSim::RunParty (solo, 1, E, 300, r1);
        BattleSim::Summary b = BattleSim::RunParty (duo, 2, E, 300, r2);
        printf ("前衛だけ 勝率%.2f / 前衛+回復役 勝率%.2f 回復%.1f回 倒れ%.2f人\n", a.dWinRate, b.dWinRate, b.dHeal, b.dDead);
        CHECK (b.dHeal > 1.0, "回復役が仲間を回復していない (%.2f回)", b.dHeal);
        CHECK (b.dWinRate > a.dWinRate + 0.2, "回復役がいても勝率が上がらない (%.2f vs %.2f)", b.dWinRate, a.dWinRate);
        /* 狙い方 TARGET_LOWHP: 一番HPが低い（回復役）が最初に狙われるので、先頭狙いより回復役が倒れやすい */
        BattleSim::Summary c = BattleSim::RunParty (duo, 2, E, 300, r3, BattleSim::TARGET_LOWHP);
        CHECK (c.dDead >= b.dDead, "HPが低い人を狙う方が倒れる人数が少ない (%.2f vs %.2f)", c.dDead, b.dDead);
    }
    /* 7. 遠隔の先撃ち（bAtkIsAT の bit1 以上＝攻撃特技の射程）: 動かない敵（開始距離4）に射程3の攻撃特技（通常型）を持つ人は、
          歩いて近づく間にも撃つので、射程なし（同じ特技）より早く倒せる。射程 0・1 の人は従来どおり（同じ乱数で同じ結果） */
    {
        BattleSim::Player M = Player();
        M.nHP = 300; M.nMana = 100; M.nAT = 5; M.nAtkMana = 3; M.nAtkAvg = 40; M.nAtkTime = 0; M.nDEX = 255;
        BattleSim::Player Mr = M; Mr.bAtkIsAT = 6;      /* 射程3・通常型 */
        BattleSim::Player Mn = M; Mn.bAtkIsAT = 0;
        BattleSim::Player M1 = M; M1.bAtkIsAT = 2;      /* 射程1 = 遠隔なし */
        BattleSim::Enemy E = {120, 1, 5, 0, 1, 4, 0, 3, 20};
        Rng r1 (21), r2 (21), r3 (21);
        BattleSim::Summary a = BattleSim::Run (Mn, E, 200, r1);
        BattleSim::Summary b = BattleSim::Run (Mr, E, 200, r2);
        BattleSim::Summary c = BattleSim::Run (M1, E, 200, r3);
        printf ("遠隔なし %.2f秒 / 射程3 %.2f秒 / 射程1 %.2f秒\n", a.dSec, b.dSec, c.dSec);
        CHECK (b.dSec < a.dSec - 0.5, "遠隔の先撃ちで速く倒せていない (%.2f秒 vs %.2f秒)", b.dSec, a.dSec);
        CHECK (a.dSec == c.dSec, "射程1 が射程なしと違う (%.2f秒 vs %.2f秒)", c.dSec, a.dSec);
    }
    printf (g_bad ? "NG\n" : "一致\n");
    return g_bad ? 1 : 0;
}
