#pragma once
/* バランステスト用の戦闘ループ（サーバー・クライアント非依存、header-only）。
   一直線の戦場で、プレイヤー（1〜4人のパーティ）対 敵N体（最大4体）を 50ms 刻みで進める。
   1人戦は OneBattle、パーティ戦は OneBattleParty（OneBattle は 1 人のパーティとして呼ぶだけで、乱数の引き方も 1 人の時は従来と同じ）。
   計算式は BattleCalc.h を使う。乱数は呼び出し側から関数オブジェクトで受け取る（rng() は整数）。
   仮の JS 試作（sim.js）と同じ流れ。敵の特技（攻撃魔法・回復）は入っている（敵は Mana を使わない）。
   入っていないもの: 回復道具、ジャスト、属性軽減、状態異常（毒・眠りなどの特技は「行動を使うだけ」）、召喚（置く魔法の複数回ヒット）。
   遠隔は任意（Player.bAtkIsAT の bit1 以上に攻撃特技の射程を入れた人だけ、射程内に敵が入ったら手前に来る前から撃つ。入れなければ従来どおり）。
   連携ボーナス（別の人が 2 秒以内に同じ敵へ当てたら 130%）はパーティ戦で入っている（RenkeiCheck と同じ）。 */
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
    int bAtkIsAT;       /* bit0=1: 攻撃力＋値−守備/2 型。bit1 以上: 攻撃特技の射程（マス。0・1 は遠隔なし。例: 射程3・通常型 = 6、AT型 = 7） */
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
    ENEMY_MAX = 4,
    PARTY_MAX = 4,          /* 味方の最大人数 */
    RENKEI_TIME = 2000,     /* 別の人が当ててからこの時間(ms)未満に当てると連携（SboSvrBattleProc.cpp の RENKEI_TIME） */
    RENKEI_RATE = 130,      /* 連携の時のダメージ(%)（RENKEI_RATE） */
    HEAL_ALLY_RATE = 40,    /* 回復特技を持つ味方は、仲間のHPがこの割合(%)以下で回復する（従来の自分用の 40% と同じ） */
    TARGET_FIRST = 0,       /* 敵の狙い方0: 並び順の先頭（前衛）。狙った相手が倒れるまで変えない */
    TARGET_LOWHP = 1        /* 敵の狙い方1: 狙い直す時に、一番HPが低い味方（現在HPの絶対値） */
};

struct Result {
    int bWin;
    double dHpRate;     /* 終了時の残りHP/最大HP */
    int nMs;            /* 所要時間 */
    int nHeal;          /* プレイヤーの回復特技の回数 */
    int nEnemyMagic;    /* 敵が特技を使った回数（全員分） */
    int nEnemyHeal;     /* うち敵の回復特技の回数 */
    int bTimeout;
    int nDead;          /* 倒れた味方の人数（勝った戦でも数える） */
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

/* 敵が狙う味方を決める（FindTarget に近い単純な規則）。一直線の戦場では味方は全員同じ距離なので「最寄り」は並び順で決まる。
   今の狙いが生きていればそのまま（本番は dwTargetTblID を持ち続ける）。倒れていたら狙い直し:
   TARGET_FIRST=生きている先頭の人 / TARGET_LOWHP=生きている人のうち現在HPが一番低い人（同じなら先頭）。乱数は使わない */
inline int PickTarget(int nCur, const int *pHP, int nP, int nRule) {
    if (nCur >= 0 && pHP[nCur] > 0) return nCur;
    int nBest = -1;
    for (int i = 0; i < nP; i++) {
        if (pHP[i] <= 0) continue;
        if (nBest < 0 || (nRule == TARGET_LOWHP && pHP[i] < pHP[nBest])) nBest = i;
        if (nRule != TARGET_LOWHP) break;
    }
    return nBest;
}

/* 1 戦（味方 nP 人。1 ≤ nP ≤ PARTY_MAX）。
   敵: 気づく・狙うのは PickTarget、通常攻撃・特技の的はその味方。
   味方: 全員が同じ距離・同じ敵（生きている手前の敵の先頭）を狙う。各自のゲージで行動。
   回復特技を持つ人は、HP が 40% 以下の味方（自分含む）のうち一番割合が低い人を回復（Mana が足りれば。自分だけの時は従来どおり）。
   連携: 別の人が同じ敵へ当ててから RENKEI_TIME 未満で当てたら RENKEI_RATE%（bRenkei=0 で無効）。当てた時だけ記録（空振りは無し）。 */
template <class Rng>
inline Result OneBattleParty(const Player *aP, int nP, const Enemy &E, Rng &rng, int nRule = TARGET_FIRST, int bRenkei = 1) {
    struct Foe { int nHP, nPos, bAggro; long nAtkAt; /* -1: 未設定 */ long nRangeAt; int bTried; int nTgt; int nLastBy; long nLastAt; };
    Foe es[ENEMY_MAX];
    int nCnt = E.nCount < 1 ? 1 : (E.nCount > ENEMY_MAX ? ENEMY_MAX : E.nCount);
    for (int i = 0; i < nCnt; i++) {
        es[i].nHP = E.nHP; es[i].nPos = E.nDist; es[i].bAggro = 0; es[i].nAtkAt = -1; es[i].nRangeAt = -1; es[i].bTried = 0;
        es[i].nTgt = -1; es[i].nLastBy = -1; es[i].nLastAt = 0;
    }
    if (nP < 1) nP = 1;
    if (nP > PARTY_MAX) nP = PARTY_MAX;
    int aHP[PARTY_MAX], aMana[PARTY_MAX], nAlivePl = nP, nMaxSum = 0;
    long aNext[PARTY_MAX], aBusy[PARTY_MAX];
    int aGauge[PARTY_MAX];
    for (int i = 0; i < nP; i++) {
        aHP[i] = aP[i].nHP; aMana[i] = aP[i].nMana; aNext[i] = -1; aBusy[i] = 0;
        aGauge[i] = BattleCalc::GaugeTime (aP[i].nDEX);
        nMaxSum += aP[i].nHP;
    }
    bool bSkill = false;
    for (int k = 0; k < BattleCalc::EM_SLOTS; k++) if (E.aSkill[k].nType) bSkill = true;
    const int nETurn = BattleCalc::EnemyTurnTime (E.nDEX);
    Result r = {0, 0, 0, 0, 0, 0, 0, 0};
    int t = 0;

    /* 味方 i が倒れたか調べる。全員倒れたら true（負け） */
    #define BS_CHECK_DEAD(i) do { if (aHP[i] <= 0 && aHP[i] != -1000000000) { aHP[i] = -1000000000; nAlivePl--; r.nDead++; } } while (0)

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
            int nSum = 0;
            for (int i = 0; i < nP; i++) nSum += aHP[i] > 0 ? aHP[i] : 0;
            r.bWin = 1; r.dHpRate = (double)nSum / nMaxSum; r.nMs = t; return r;
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
                e.nTgt = PickTarget (e.nTgt, aHP, nP, nRule);
                if (e.nTgt >= 0 && EnemyCast (aP[e.nTgt], E, es, nCnt, i, true, aHP[e.nTgt], rng, r)) {
                    e.bTried = 0; e.nRangeAt = t + nETurn;
                    BS_CHECK_DEAD (e.nTgt);
                    if (nAlivePl <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
                }
            }
            if (e.nPos <= 1 && e.nAtkAt < 0) {
                e.nAtkAt = t + ENEMY_THINK_MS + nETurn; /* 初回は 1 間隔後 */
            } else if (e.nPos <= 1 && t >= e.nAtkAt) {
                e.nAtkAt = t + nETurn; /* 殴った時刻を記録（サーバーの dwLastBattle）。次は 1 間隔後 */
                e.bTried = 0;
                e.nTgt = PickTarget (e.nTgt, aHP, nP, nRule);
                if (e.nTgt < 0) continue;
                const int g = e.nTgt;
                /* 行動のたびに特技のくじ（BattleProc）。当たれば通常攻撃はしない */
                if (bSkill && EnemyCast (aP[g], E, es, nCnt, i, false, aHP[g], rng, r)) {
                    BS_CHECK_DEAD (g);
                    if (nAlivePl <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
                    continue;
                }
                if (Pct (rng) >= 100 - MISS_RATE) continue;
                bool bCrit = Pct (rng) >= 100 - CRIT_RATE;
                aHP[g] -= BattleCalc::Damage (E.nAT, 0, aP[g].nDF, bCrit, false, 100, 0, rng);
                BS_CHECK_DEAD (g);
                if (nAlivePl <= 0) { r.dHpRate = 0; r.nMs = t; return r; }
            }
        }
        /* 味方 */
        if (!nFrontCnt) {
            if (!E.bMove && t % PLAYER_WALK_MS == 0) {
                for (int i = 0; i < nCnt; i++) if (es[i].nHP > 0 && es[i].nPos > 1) es[i].nPos -= 1;
            }
            for (int i = 0; i < nP; i++) {
                /* 遠隔の先撃ち（任意）: 攻撃特技の射程（bAtkIsAT の bit1 以上）が 2 以上の人は、射程内に敵がいる間、
                   手前に来る前でも攻撃特技を撃つ（Mana があれば）。射程 0・1 の人は従来どおり（ゲージは敵が来てから）。連携は入れない */
                const Player &Q = aP[i];
                const int nRange = Q.bAtkIsAT >> 1;
                int nTo = -1;
                if (aHP[i] > 0 && nRange > 1 && Q.nAtkMana > 0 && aMana[i] >= Q.nAtkMana) {
                    for (int j = 0; j < nCnt; j++)
                        if (es[j].nHP > 0 && es[j].nPos <= nRange && (nTo < 0 || es[j].nPos < es[nTo].nPos)) nTo = j;
                }
                if (nTo < 0) { aNext[i] = -1; continue; }
                if (aNext[i] < 0) aNext[i] = t + aGauge[i];
                if (t >= aNext[i] && t >= aBusy[i]) {
                    aMana[i] -= Q.nAtkMana;
                    int d = Q.nAtkAvg + ((Q.bAtkIsAT & 1) ? Q.nAT - E.nDF : 0);
                    es[nTo].nHP -= d > 0 ? d : 0;
                    aBusy[i] = t + 1000 + Q.nAtkTime * 990;
                    aNext[i] = -1;
                }
            }
            continue;
        }
        for (int pi = 0; pi < nP; pi++) {
            if (aHP[pi] <= 0) continue;
            const Player &P = aP[pi];
            if (nFront[0] >= 0 && es[nFront[0]].nHP <= 0) {
                /* 同じ tick で前の味方が倒した。次の手前の敵に切り替える */
                nFrontCnt = 0;
                for (int i = 0; i < nCnt; i++) if (es[i].nHP > 0 && es[i].nPos <= 1) nFront[nFrontCnt++] = i;
                if (!nFrontCnt) break;
            }
            if (aNext[pi] < 0) aNext[pi] = t + aGauge[pi];
            if (t >= aNext[pi] && t >= aBusy[pi]) {
                Foe &tgt = es[nFront[0]];
                int nHealTo = -1;
                if (P.nHealMana > 0 && aMana[pi] >= P.nHealMana) {
                    for (int j = 0; j < nP; j++) {
                        if (aHP[j] <= 0 || aHP[j] * 100 > aP[j].nHP * HEAL_ALLY_RATE) continue;
                        if (nHealTo < 0 || (long)aHP[j] * aP[nHealTo].nHP < (long)aHP[nHealTo] * aP[j].nHP) nHealTo = j;
                    }
                }
                /* 連携の倍率: 別の人が RENKEI_TIME 未満の間に同じ敵へ当てていたら。当てた時だけ呼ぶ（記録も当てた時だけ） */
                #define BS_RENKEI(rate) do { rate = 100; \
                    if (bRenkei && tgt.nLastBy >= 0 && tgt.nLastBy != pi && t - tgt.nLastAt < RENKEI_TIME) rate = RENKEI_RATE; \
                    tgt.nLastBy = pi; tgt.nLastAt = t; } while (0)
                if (nHealTo >= 0) {
                    aMana[pi] -= P.nHealMana;
                    aHP[nHealTo] += P.nHealAvg; if (aHP[nHealTo] > aP[nHealTo].nHP) aHP[nHealTo] = aP[nHealTo].nHP;
                    r.nHeal++;
                    aBusy[pi] = t + 1000 + P.nHealTime * 990;
                } else if (P.nAtkMana > 0 && aMana[pi] >= P.nAtkMana) {
                    aMana[pi] -= P.nAtkMana;
                    int d = P.nAtkAvg + ((P.bAtkIsAT & 1) ? P.nAT - E.nDF : 0);
                    d = d > 0 ? d : 0;
                    int nRate; BS_RENKEI (nRate);
                    tgt.nHP -= d * nRate / 100;
                    aBusy[pi] = t + 1000 + P.nAtkTime * 990;
                } else if (Pct (rng) >= 100 - MISS_RATE) {
                    /* 空振り */
                } else {
                    bool bCrit = Pct (rng) >= 100 - CRIT_RATE;
                    int nDec = bCrit ? 0 : BattleCalc::WeaponMinRoll (P.nWeaponMin, rng);
                    int nRate; BS_RENKEI (nRate);
                    tgt.nHP -= BattleCalc::Damage (P.nAT, 100, E.nDF, bCrit, false, nRate, nDec, rng);
                }
                #undef BS_RENKEI
                aNext[pi] = -1;
            }
        }
    }
    #undef BS_CHECK_DEAD
    int nSum = 0;
    for (int i = 0; i < nP; i++) nSum += aHP[i] > 0 ? aHP[i] : 0;
    r.dHpRate = (double)nSum / nMaxSum; r.nMs = t; r.bTimeout = 1;
    return r;
}

/* 1 人戦（従来の入口）。味方 1 人のパーティ */
template <class Rng>
inline Result OneBattle(const Player &P, const Enemy &E, Rng &rng) {
    return OneBattleParty (&P, 1, E, rng);
}

/* 集計 */
struct Summary {
    double dWinRate, dHpRate, dSec, dHeal, dEnemyMagic, dEnemyHeal;
    double dDead;       /* 1 戦あたりの倒れた味方の人数 */
    const char *pszVerdict;
};

inline const char *Verdict(double w, double h) {
    return (w >= .95 && h >= .5) ? "余裕" : w >= .8 ? "互角" : w >= .4 ? "きつい" : "無理";
}

template <class Rng>
inline Summary RunParty(const Player *aP, int nP, const Enemy &E, int nRuns, Rng &rng, int nRule = TARGET_FIRST, int bRenkei = 1) {
    int nWin = 0; double dHp = 0, dMs = 0, dHeal = 0, dEM = 0, dEH = 0, dDead = 0;
    for (int i = 0; i < nRuns; i++) {
        Result b = OneBattleParty (aP, nP, E, rng, nRule, bRenkei);
        if (b.bWin) { nWin++; dHp += b.dHpRate; dMs += b.nMs; }
        dHeal += b.nHeal; dEM += b.nEnemyMagic; dEH += b.nEnemyHeal; dDead += b.nDead;
    }
    Summary s;
    s.dWinRate = nRuns ? (double)nWin / nRuns : 0;
    s.dHpRate = nWin ? dHp / nWin : 0;
    s.dSec = nWin ? dMs / nWin / 1000.0 : 0;
    s.dHeal = nRuns ? dHeal / nRuns : 0;
    s.dEnemyMagic = nRuns ? dEM / nRuns : 0;
    s.dEnemyHeal = nRuns ? dEH / nRuns : 0;
    s.dDead = nRuns ? dDead / nRuns : 0;
    s.pszVerdict = Verdict (s.dWinRate, s.dHpRate);
    return s;
}

template <class Rng>
inline Summary Run(const Player &P, const Enemy &E, int nRuns, Rng &rng) {
    return RunParty (&P, 1, E, nRuns, rng);
}

} // namespace BattleSim
