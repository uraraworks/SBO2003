#pragma once
/* バランステスト用の戦闘ループ（サーバー・クライアント非依存、header-only）。
   一直線の戦場で、プレイヤー1人 対 敵N体（最大4体）を 50ms 刻みで進める。
   計算式は BattleCalc.h を使う。乱数は呼び出し側から関数オブジェクトで受け取る（rng() は整数）。
   仮の JS 試作（sim.js）と同じ流れ。入っていないもの: 敵の特技・回復、回復道具、ジャスト、遠隔、連携、状態異常。 */
#include <string>
#include <vector>
#include "BattleCalc.h"

namespace BattleSim {

/* プレイヤー側（標準プレイヤーの数値を呼び出し側が詰める） */
struct Player {
    int nLv;
    int nHP, nMana;
    int nAT;            /* 力など＋武器の攻撃力 */
    int nDF;            /* Damage に渡す守備力（Damage 内で 1/2 される） */
    int nDEX;
    int nWeaponMin;     /* 武器の減少値 byMin（0 なら減らない） */
    /* 回復特技（nMana が 0 なら無し） */
    int nHealMana, nHealAvg, nHealTime;
    /* 攻撃特技（nMana が 0 なら無し） */
    int nAtkMana, nAtkAvg, nAtkTime;
    int bAtkIsAT;       /* 1: 攻撃力＋値−守備/2 型 */
};

/* 敵側（全員同じ数値） */
struct Enemy {
    int nHP, nAT, nDF, nDEX;
    int nCount;         /* 体数（最大4） */
    int nDist;          /* 開始距離（マス。1 以下で目の前） */
    int bMove;          /* 1: 敵が気づいて近づく（戦闘2）/ 0: 動かず、プレイヤーが歩いて近づく（戦闘1） */
    int nSearch;        /* 気づく範囲（マス） */
    int nMoveX;         /* 気づいた後、500ms ごとに 1 マス詰める確率(%) */
};

/* 定数（SboSvr/src/CharProc/CharProcSEARCH.cpp など） */
enum {
    TICK_MS = 50,
    MAX_MS = 300000,
    ENEMY_THINK_MS = 500,   /* 敵が探す・歩く間隔 */
    PLAYER_WALK_MS = 400,   /* プレイヤーが 1 マス歩く間隔 */
    MISS_NOTICE = 30,       /* 気づく見逃し(%) */
    MISS_RATE = 5,          /* 通常攻撃の空振り(%) */
    CRIT_RATE = 2,          /* 会心(%) */
    ENEMY_MAX = 4
};

struct Result {
    int bWin;
    double dHpRate;     /* 終了時の残りHP/最大HP */
    int nMs;            /* 所要時間 */
    int nHeal;          /* 回復特技の回数 */
    int bTimeout;
};

template <class Rng>
inline int Pct(Rng &rng) { return (int)(rng () % 100); }

/* 1 戦 */
template <class Rng>
inline Result OneBattle(const Player &P, const Enemy &E, Rng &rng) {
    struct Foe { int nHP, nPos, bAggro; long nAtkAt; /* -1: 未設定 */ };
    Foe es[ENEMY_MAX];
    int nCnt = E.nCount < 1 ? 1 : (E.nCount > ENEMY_MAX ? ENEMY_MAX : E.nCount);
    for (int i = 0; i < nCnt; i++) {
        es[i].nHP = E.nHP; es[i].nPos = E.nDist; es[i].bAggro = 0; es[i].nAtkAt = -1;
    }
    int nPlHP = P.nHP, nMana = P.nMana;
    long nNext = -1, nBusy = 0;
    const int nETurn = BattleCalc::EnemyTurnTime (E.nDEX);
    const int nGauge = BattleCalc::GaugeTime (P.nDEX);
    Result r = {0, 0, 0, 0, 0};
    int t = 0;

    while (t < MAX_MS) {
        t += TICK_MS;
        int nFront[ENEMY_MAX], nFrontCnt = 0, nAlive = 0;
        for (int i = 0; i < nCnt; i++) {
            if (es[i].nHP > 0) {
                nAlive++;
                if (es[i].nPos <= 1) nFront[nFrontCnt++] = i;
            }
        }
        if (!nAlive) {
            r.bWin = 1; r.dHpRate = (double)nPlHP / P.nHP; r.nMs = t; return r;
        }
        /* 敵の動き */
        for (int i = 0; i < nCnt; i++) {
            Foe &e = es[i];
            if (e.nHP <= 0) continue;
            if (e.nPos > 1 && t % ENEMY_THINK_MS == 0 && E.bMove) {
                if (!e.bAggro && e.nPos <= E.nSearch && Pct (rng) >= MISS_NOTICE) e.bAggro = 1;
                if (e.bAggro && Pct (rng) < E.nMoveX) e.nPos -= 1;
            }
            if (e.nPos <= 1 && e.nAtkAt < 0) {
                e.nAtkAt = t + ENEMY_THINK_MS + nETurn; /* 初回は 1 間隔後 */
            } else if (e.nPos <= 1 && t >= e.nAtkAt) {
                e.nAtkAt = t;
                if (Pct (rng) >= 100 - MISS_RATE) continue;
                bool bCrit = Pct (rng) >= 100 - CRIT_RATE;
                nPlHP -= BattleCalc::Damage (E.nAT, 0, P.nDF, bCrit, false, 100, 0, rng);
                if (nPlHP <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
            }
        }
        /* プレイヤー */
        if (!nFrontCnt) {
            if (!E.bMove && t % PLAYER_WALK_MS == 0) {
                for (int i = 0; i < nCnt; i++) if (es[i].nHP > 0 && es[i].nPos > 1) es[i].nPos -= 1;
            }
            nNext = -1;
            continue;
        }
        if (nNext < 0) nNext = t + nGauge;
        if (t >= nNext && t >= nBusy) {
            Foe &tgt = es[nFront[0]];
            if (nPlHP * 10 <= P.nHP * 4 && P.nHealMana > 0 && nMana >= P.nHealMana) {
                nMana -= P.nHealMana;
                nPlHP += P.nHealAvg; if (nPlHP > P.nHP) nPlHP = P.nHP;
                r.nHeal++;
                nBusy = t + 1000 + P.nHealTime * 990;
            } else if (P.nAtkMana > 0 && nMana >= P.nAtkMana) {
                nMana -= P.nAtkMana;
                int d = P.nAtkAvg + (P.bAtkIsAT ? P.nAT - E.nDF : 0);
                tgt.nHP -= d > 0 ? d : 0;
                nBusy = t + 1000 + P.nAtkTime * 990;
            } else if (Pct (rng) >= 100 - MISS_RATE) {
                /* 空振り */
            } else {
                bool bCrit = Pct (rng) >= 100 - CRIT_RATE;
                int nDec = bCrit ? 0 : BattleCalc::WeaponMinRoll (P.nWeaponMin, rng);
                tgt.nHP -= BattleCalc::Damage (P.nAT, 100, E.nDF, bCrit, false, 100, nDec, rng);
            }
            nNext = -1;
        }
    }
    r.dHpRate = (double)nPlHP / P.nHP; r.nMs = t; r.bTimeout = 1;
    return r;
}

/* 集計 */
struct Summary {
    double dWinRate, dHpRate, dSec, dHeal;
    const char *pszVerdict;
};

inline const char *Verdict(double w, double h) {
    return (w >= .95 && h >= .5) ? "余裕" : w >= .8 ? "互角" : w >= .4 ? "きつい" : "無理";
}

template <class Rng>
inline Summary Run(const Player &P, const Enemy &E, int nRuns, Rng &rng) {
    int nWin = 0; double dHp = 0, dMs = 0, dHeal = 0;
    for (int i = 0; i < nRuns; i++) {
        Result b = OneBattle (P, E, rng);
        if (b.bWin) { nWin++; dHp += b.dHpRate; dMs += b.nMs; }
        dHeal += b.nHeal;
    }
    Summary s;
    s.dWinRate = nRuns ? (double)nWin / nRuns : 0;
    s.dHpRate = nWin ? dHp / nWin : 0;
    s.dSec = nWin ? dMs / nWin / 1000.0 : 0;
    s.dHeal = nRuns ? dHeal / nRuns : 0;
    s.pszVerdict = Verdict (s.dWinRate, s.dHpRate);
    return s;
}

} // namespace BattleSim
