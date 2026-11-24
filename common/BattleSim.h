#pragma once
/* バランステスト用の戦闘ループ（サーバー・クライアント非依存、header-only）。
   一直線の戦場で、プレイヤー1人 対 敵N体（最大4体）を 50ms 刻みで進める。
   計算式は BattleCalc.h を使う。乱数は呼び出し側から関数オブジェクトで受け取る（rng() は整数）。
   仮の JS 試作（sim.js）と同じ流れ。敵の特技（攻撃魔法・回復）は入っている（敵は Mana を使わない）。
   入っていないもの: 回復道具、ジャスト、遠隔、連携、属性軽減、状態異常（毒・眠りなどの特技は「行動を使うだけ」）。 */
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
    int nDFMagic;       /* 魔法防御（標準プレイヤーは 0。魔法防御の首飾り・指輪を付けた時だけ増える） */
};

/* 敵の特技 1 枠（本番の magic 表の 効果種別・基本値・射程）。nType 0 は空き枠 */
enum { SKILL_NOOP = 99 };   /* 未対応の効果（毒・眠り・召喚など）: 枠は引かれて行動は使うが何も起きない */
struct EnemySkill {
    int nType;          /* BattleCalc::EMT_*（1 HP減少 / 2 HP増加 / 6 HP減少(攻撃力) / 10 HP減少(率)）、SKILL_NOOP、0=空き */
    int nData1, nData2; /* 基本値 wData1〜wData2（率は wData1 だけ） */
    int nTargetType;    /* 発動種別 BattleCalc::EMTT_*（0 自分 1 前方 2 周囲 3 画面内 …） */
    int nTarget;        /* 使用対象（1=NPC 側だけ。回復は 0） */
    int nArea;          /* 射程（マス） */
};

/* 敵側（全員同じ数値） */
struct Enemy {
    int nHP, nAT, nDF, nDEX;
    int nCount;         /* 体数（最大4） */
    int nDist;          /* 開始距離（マス。1 以下で目の前） */
    int bMove;          /* 1: 敵が気づいて近づく（戦闘2）/ 0: 動かず、プレイヤーが歩いて近づく（戦闘1） */
    int nSearch;        /* 気づく範囲（マス） */
    int nMoveX;         /* 気づいた後、500ms ごとに 1 マス詰める確率(%) */
    EnemySkill aSkill[BattleCalc::EM_SLOTS]; /* 特技 16 枠（全部 nType 0 なら特技なし＝くじも引かない） */
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
    int nHeal;          /* プレイヤーの回復特技の回数 */
    int nEnemyMagic;    /* 敵が特技を使った回数（全員分） */
    int nEnemyHeal;     /* うち敵の回復特技の回数 */
    int bTimeout;
};

template <class Rng>
inline int Pct(Rng &rng) { return (int)(rng () % 100); }

/* 敵 1 体（es[nSelf]）の特技のくじを引き、当たれば使う。使ったら true（行動を使った）。
   bRange: 離れた相手に撃つ時（回復はしない・届く攻撃だけ）。敵は Mana を消費しない。
   回復の「仲間」は同じ群れの生きている敵で、範囲（周囲・画面内）の中にいるとみなす。 */
template <class Foe, class Rng>
inline bool EnemyCast(const Player &P, const Enemy &E, Foe *es, int nCnt, int nSelf, bool bRange, int &nPlHP, Rng &rng, Result &r) {
    using namespace BattleCalc;
    Foe &me = es[nSelf];
    bool bAllyHurt = false;   /* 自分以外の仲間で、HPが EM_HEAL_RATE% 未満の者がいるか */
    for (int j = 0; j < nCnt; j++) {
        if (j != nSelf && es[j].nHP > 0 && NeedHealHP (es[j].nHP, E.nHP)) bAllyHurt = true;
    }
    EnemySlot slot[EM_SLOTS];
    for (int k = 0; k < EM_SLOTS; k++) {
        const EnemySkill &s = E.aSkill[k];
        slot[k].bNeedHeal = false; slot[k].bReach = false;
        if (s.nType == 0) { slot[k].nKind = EMSLOT_EMPTY; continue; }
        if (IsHealMagic (s.nType, s.nTarget)) {
            slot[k].nKind = EMSLOT_HEAL;
            slot[k].bNeedHeal = NeedHealHP (me.nHP, E.nHP) ||
                ((s.nTargetType == EMTT_CIRCLE || s.nTargetType == EMTT_SCREEN) && bAllyHurt);
        } else {
            slot[k].nKind = EMSLOT_OTHER;
            slot[k].bReach = EnemyMagicReach (s.nTargetType, s.nArea, me.nPos);
        }
    }
    int nNo = ChooseEnemyMagic (me.nHP, E.nHP, slot, bRange, rng);
    if (nNo < 0) return false;
    const EnemySkill &s = E.aSkill[nNo];
    r.nEnemyMagic++;
    if (s.nType == SKILL_NOOP) return true;
    if (slot[nNo].nKind == EMSLOT_HEAL) {
        /* 自分、周囲・画面内なら仲間にも（自分が先）。1 人ごとにくじを引く（満タンなら引かない） */
        r.nEnemyHeal++;
        me.nHP += MagicHealAmount (s.nData1, s.nData2, me.nHP, E.nHP, rng);
        if (s.nTargetType == EMTT_CIRCLE || s.nTargetType == EMTT_SCREEN) {
            for (int j = 0; j < nCnt; j++) {
                if (j != nSelf && es[j].nHP > 0) es[j].nHP += MagicHealAmount (s.nData1, s.nData2, es[j].nHP, E.nHP, rng);
            }
        }
        return true;
    }
    if (s.nTargetType == EMTT_SELF) return true;   /* 自分にHP減少は効かない */
    int d = 0;
    if (s.nType == EMT_HPDOWN) d = MagicDamageHpDown (s.nData1, s.nData2, P.nDFMagic, rng);
    else if (s.nType == EMT_HPDOWNAT) d = MagicDamageHpDownAT (s.nData1, s.nData2, E.nAT, P.nDF, rng);
    else if (s.nType == EMT_HPDOWNRATE) d = MagicDamageHpDownRate (nPlHP, s.nData1, P.nDFMagic);
    nPlHP -= d;
    return true;
}

/* 1 戦 */
template <class Rng>
inline Result OneBattle(const Player &P, const Enemy &E, Rng &rng) {
    struct Foe { int nHP, nPos, bAggro; long nAtkAt; /* -1: 未設定 */ long nRangeAt; int bTried; };
    Foe es[ENEMY_MAX];
    int nCnt = E.nCount < 1 ? 1 : (E.nCount > ENEMY_MAX ? ENEMY_MAX : E.nCount);
    for (int i = 0; i < nCnt; i++) {
        es[i].nHP = E.nHP; es[i].nPos = E.nDist; es[i].bAggro = 0; es[i].nAtkAt = -1; es[i].nRangeAt = -1; es[i].bTried = 0;
    }
    int nPlHP = P.nHP, nMana = P.nMana;
    bool bSkill = false;
    for (int k = 0; k < BattleCalc::EM_SLOTS; k++) if (E.aSkill[k].nType) bSkill = true;
    long nNext = -1, nBusy = 0;
    const int nETurn = BattleCalc::EnemyTurnTime (E.nDEX);
    const int nGauge = BattleCalc::GaugeTime (P.nDEX);
    Result r = {0, 0, 0, 0, 0, 0, 0};
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
                if (!e.bAggro && e.nPos <= E.nSearch && Pct (rng) >= MISS_NOTICE) {
                    e.bAggro = 1;
                    e.nRangeAt = t + nETurn; /* 狙いが決まって 1 間隔後に、離れた所からのくじを引く */
                }
                if (e.bAggro && Pct (rng) < E.nMoveX) e.nPos -= 1;
            }
            /* 離れた所から特技（BattleProcRangeMagic）。くじは行動の記録（dwLastBattle）1 つにつき 1 回だけ。
               外れたら、殴る・特技を使うまで引き直さない。動かない敵（戦闘1）は殴られるまで狙いが無いので使わない */
            if (bSkill && e.nPos > 1 && e.bAggro && e.nRangeAt >= 0 && t >= e.nRangeAt && !e.bTried) {
                e.bTried = 1;
                if (EnemyCast (P, E, es, nCnt, i, true, nPlHP, rng, r)) {
                    e.bTried = 0; e.nRangeAt = t + nETurn;
                    if (nPlHP <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
                }
            }
            if (e.nPos <= 1 && e.nAtkAt < 0) {
                e.nAtkAt = t + ENEMY_THINK_MS + nETurn; /* 初回は 1 間隔後 */
            } else if (e.nPos <= 1 && t >= e.nAtkAt) {
                e.nAtkAt = t + nETurn; /* 殴った時刻を記録（サーバーの dwLastBattle）。次は 1 間隔後 */
                e.bTried = 0;
                /* 行動のたびに特技のくじ（BattleProc）。当たれば通常攻撃はしない */
                if (bSkill && EnemyCast (P, E, es, nCnt, i, false, nPlHP, rng, r)) {
                    if (nPlHP <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
                    continue;
                }
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
    double dWinRate, dHpRate, dSec, dHeal, dEnemyMagic, dEnemyHeal;
    const char *pszVerdict;
};

inline const char *Verdict(double w, double h) {
    return (w >= .95 && h >= .5) ? "余裕" : w >= .8 ? "互角" : w >= .4 ? "きつい" : "無理";
}

template <class Rng>
inline Summary Run(const Player &P, const Enemy &E, int nRuns, Rng &rng) {
    int nWin = 0; double dHp = 0, dMs = 0, dHeal = 0, dEM = 0, dEH = 0;
    for (int i = 0; i < nRuns; i++) {
        Result b = OneBattle (P, E, rng);
        if (b.bWin) { nWin++; dHp += b.dHpRate; dMs += b.nMs; }
        dHeal += b.nHeal; dEM += b.nEnemyMagic; dEH += b.nEnemyHeal;
    }
    Summary s;
    s.dWinRate = nRuns ? (double)nWin / nRuns : 0;
    s.dHpRate = nWin ? dHp / nWin : 0;
    s.dSec = nWin ? dMs / nWin / 1000.0 : 0;
    s.dHeal = nRuns ? dHeal / nRuns : 0;
    s.dEnemyMagic = nRuns ? dEM / nRuns : 0;
    s.dEnemyHeal = nRuns ? dEH / nRuns : 0;
    s.pszVerdict = Verdict (s.dWinRate, s.dHpRate);
    return s;
}

} // namespace BattleSim
