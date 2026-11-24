#pragma once
/* 戦闘計算の純粋な部分（副作用なし）。乱数は呼び出し側から関数オブジェクトで受け取る。
   乱数を引く順番・回数は SboSvrBattleProc.cpp の旧実装と同一（golden に影響するので変えない）。 */
namespace BattleCalc {

/* 武器の減少値くじ。byMin が 0 なら乱数を引かず 0。引く時は 1 回だけ。 */
template <class Rng>
inline int WeaponMinRoll(int byMin, Rng &rng) {
    int nTmp = 0;
    if (byMin) {
        nTmp = ((rng () % byMin) + 1);
    }
    return nTmp;
}

/* ダメージ量（HP へ反映する前の値）。
   nWeaponDec は WeaponMinRoll の結果（会心なら 0 を渡す＝使われない）。
   乱数は「±補正の向き」「±補正の幅」「おまけの1ダメージ（条件を満たす時だけ）」の順に引く。 */
template <class Rng>
inline int Damage(int nAT, int byAtackRate, int nDF, bool bSPAtack, bool bGuard, int nRate, int nWeaponDec, Rng &rng) {
    int nTmp;

    /* PCの行動？ */
    if (byAtackRate > 0) {
        /* ジャスト(101)も威力は 100% */
        nAT = nAT * (byAtackRate < 100 ? byAtackRate : 100) / 100;
    }

    if (bSPAtack == false) {
        /* 減少幅を補正する */
        nAT -= nWeaponDec;
    }

    /* 割り算用に補正 */
    nAT = (nAT > 2) ? nAT : 2;

    /* 会心の一撃 */
    if (bSPAtack) {
        nAT += (nAT / 2);
    }

    /* 直接攻撃のダメージ＝{ 攻撃力 - ( 守備力/2 ) }　+-16％ */
    nTmp = nAT - (nDF / 2);

    /* ＋－の補正 */
    if (rng () % 2) {
        nTmp += (nTmp * ((rng () % 16) + 1) / 100);
    } else {
        nTmp -= (nTmp * ((rng () % 16) + 1) / 100);
    }
    nTmp = (nTmp > 0) ? nTmp : 0;
    /* 構えている相手には半分 */
    if (bGuard) {
        nTmp /= 2;
    }
    /* 連携ボーナス */
    nTmp = nTmp * nRate / 100;

    /* ノーダメージ？(おまけの1ダメージはゲージ満タンの攻撃だけ。早撃ちの連打で削れないように) */
    if ((nTmp == 0) && ((byAtackRate == 0) || (byAtackRate >= 100))) {
        if ((rng () % 100) > 66) {
            nTmp = 1;
        }
    }
    return nTmp;
}

/* 素早さから 0〜100 の段へ（最低 1）。ゲージ時間・敵の行動間隔で共通 */
inline int DexStep(int nDEX) {
    int nTmp = nDEX * 100 / 255;
    return (nTmp > 1) ? nTmp : 1;
}

/* プレイヤーのターンゲージが満タンになるまでの時間(ms)。素早さ 0 で 3 秒〜255 で 1 秒。
   nHasteRate は加速率(%)。0 なら加速なし。SboCli の GetTurnGageTime と同じ式。 */
inline int GaugeTime(int nDEX, int nHasteRate = 0) {
    int nTmp = DexStep(nDEX);
    nTmp = 1000 + (2000 - ((2000 * nTmp) / 100));
    if (nHasteRate) {
        nTmp = nTmp * 100 / (100 + nHasteRate);
    }
    return nTmp;
}

/* 敵の行動の間隔(ms)。素早さ 0 で 5 秒〜255 で 1 秒。加速で短くなる。
   SboSvrBattleProc.cpp の GetEnemyTurnTime と同じ式。 */
inline int EnemyTurnTime(int nDEX, int nHasteRate = 0) {
    int nTmp = DexStep(nDEX);
    nTmp = 1000 + (4000 - ((4000 * nTmp) / 100));
    nTmp = nTmp * 100 / (100 + nHasteRate);
    return (nTmp > 1) ? nTmp : 1;
}

/* ===================== 敵の特技（SboSvrBattleProc.cpp の ChooseEnemyMagic など） =====================
   サーバー側はまだこの関数に置き換えていない（今回はバランスシミュレータ用）。式と乱数の順番は旧実装のまま。
   型名・定数名はサーバーの #define / enum（MAX_USEMAGIC, ENEMYHEAL_*, MAGICTYPE_*）とぶつからないよう別名にしている。 */
enum {
    EM_SLOTS = 16,          /* 特技の枠数（MAX_USEMAGIC） */
    EM_HEAL_RATE = 70,      /* 敵はHPがこの割合(%)以上なら回復の特技を使わない（ENEMYHEAL_RATE） */
    EM_HEAL_PINCH = 30,     /* HPがこの割合(%)以下なら回復を優先する（ENEMYHEAL_PINCH） */
    EM_HEAL_PINCHRATE = 50, /* HPが少ない時に回復を優先する確率(%)（ENEMYHEAL_PINCHRATE） */
    /* 魔法効果種別（TypedefCommon.h の MAGICTYPE_*） */
    EMT_HPDOWN = 1, EMT_HPUP = 2, EMT_HPDOWNAT = 6, EMT_HPDOWNRATE = 10,
    /* 魔法発動種別（MAGICTARGETTYPE_*）と使用対象（MAGICTARGET_NPC） */
    EMTT_SELF = 0, EMTT_FRONT = 1, EMTT_CIRCLE = 2, EMTT_SCREEN = 3, EMTT_FRONTSIDE = 4,
    EMTARGET_NPC = 1,
    EM_SCREEN_X = 7         /* 画面内の特技が届く横幅（DRAW_PARTS_X） */
};

/* 味方(自分側)のHPを増やす特技か（IsHealMagic） */
inline bool IsHealMagic(int nType, int nTarget) {
    return (nType == EMT_HPUP) && (nTarget != EMTARGET_NPC);
}

/* 枠 1 つの状態。ChooseEnemyMagic に渡す（サーバーの g_aMagicInfo を引く部分を呼び出し側に任せる） */
enum { EMSLOT_EMPTY = 0, EMSLOT_HEAL = 1, EMSLOT_OTHER = 2 };
struct EnemySlot {
    int nKind;          /* EMSLOT_*。空き枠・存在しない特技は EMPTY */
    bool bNeedHeal;     /* HEAL の時: IsNeedHeal の結果（自分か届く範囲の仲間のHPが EM_HEAL_RATE% 未満か） */
    bool bReach;        /* OTHER の時: CanReachMagic の結果（bRange の時だけ見る） */
};

/* 自分のHPだけで見た「回復する意味があるか」（IsNeedHeal の最初の条件） */
inline bool NeedHealHP(int nHP, int nMaxHP) {
    return (nMaxHP == 0) || (nHP * 100 < nMaxHP * EM_HEAL_RATE);
}

/* 敵が使う特技の枠を選ぶ（使わずに普通に殴るなら -1）。ChooseEnemyMagic と同じ乱数の引き方:
   ① HP が EM_HEAL_PINCH% 以下の時だけ 1 回引いて EM_HEAL_PINCHRATE% を通ったら、回復の枠を集めて
      あれば 1 回引いて選ぶ（回復枠が 1 つも無ければ引かずに②へ） ② 16 枠のくじを 1 回 */
template <class Rng>
inline int ChooseEnemyMagic(int nHP, int nMaxHP, const EnemySlot *pSlot, bool bRange, Rng &rng) {
    int i, nNo, nCount, anHeal[EM_SLOTS];

    if ((nMaxHP > 0) && (nHP * 100 <= nMaxHP * EM_HEAL_PINCH) &&
        ((int)(rng () % 100) < EM_HEAL_PINCHRATE)) {
        nCount = 0;
        for (i = 0; i < EM_SLOTS; i++) {
            if (pSlot[i].nKind == EMSLOT_HEAL) {
                anHeal[nCount++] = i;
            }
        }
        if (nCount > 0) {
            return anHeal[rng () % nCount];
        }
    }

    nNo = (int)(rng () % EM_SLOTS);
    if (pSlot[nNo].nKind == EMSLOT_EMPTY) {
        return -1;
    }
    if (pSlot[nNo].nKind == EMSLOT_HEAL) {
        /* 離れた所からは回復しない */
        if (bRange || (pSlot[nNo].bNeedHeal == false)) {
            return -1;
        }
        return nNo;
    }
    if (bRange && (pSlot[nNo].bReach == false)) {
        return -1;
    }
    return nNo;
}

/* 一直線の戦場で、nDist マス離れた相手に届く攻撃の特技か（CanReachMagic の壁判定を除いた形）。
   前方=1〜範囲、周囲=範囲内、画面内=EM_SCREEN_X 以内、前方横一列などは届かない */
inline bool EnemyMagicReach(int nTargetType, int nArea, int nDist) {
    switch (nTargetType) {
    case EMTT_FRONT:    return (nDist >= 1) && (nDist <= nArea);
    case EMTT_CIRCLE:   return nDist <= nArea;
    case EMTT_SCREEN:   return nDist <= EM_SCREEN_X;
    }
    return false;
}

/* HP減少（魔法）: 基本値 wData1〜wData2 のくじ 1 回 − 魔法防御/2。属性・連携は入れていない。乱数は必ず 1 回 */
template <class Rng>
inline int MagicDamageHpDown(int nData1, int nData2, int nDFMagic, Rng &rng) {
    int nTmp = nData2 - nData1 + 1;
    if (nTmp < 0) nTmp = -nTmp;
    nTmp = (nTmp > 1) ? nTmp : 1;
    int nTmp2 = nData1 + (int)(rng () % nTmp);
    nTmp2 -= (nDFMagic / 2);
    return (nTmp2 > 0) ? nTmp2 : 0;
}

/* HP減少（攻撃力）: 攻撃力＋基本値のくじ 1 回 − 守備/2。乱数は必ず 1 回 */
template <class Rng>
inline int MagicDamageHpDownAT(int nData1, int nData2, int nAT, int nDF, Rng &rng) {
    int nTmp = nData2 - nData1 + 1;
    if (nTmp < 0) nTmp = -nTmp;
    nTmp = (nTmp > 1) ? nTmp : 1;
    int nTmp2 = nData1 + (int)(rng () % nTmp);
    nTmp2 += nAT;
    nTmp2 -= (nDF / 2);
    return (nTmp2 > 0) ? nTmp2 : 0;
}

/* HP減少（率）: 相手の今のHPの wData1% （最低 1）− 魔法防御/2。乱数は引かない */
inline int MagicDamageHpDownRate(int nTargetHP, int nData1, int nDFMagic) {
    int nTmp = (nTargetHP * 10 * nData1 / 1000);
    int nTmp2 = (nTmp > 1) ? nTmp : 1;
    nTmp2 -= (nDFMagic / 2);
    return (nTmp2 > 0) ? nTmp2 : 0;
}

/* HP増加: 満タンなら 0（乱数は引かない）。そうでなければ wData1〜wData2 のくじ 1 回（(WORD) にしてから割る）で、
   最大HPを超える分は切る。戻り値は増える量 */
template <class Rng>
inline int MagicHealAmount(int nData1, int nData2, int nHP, int nMaxHP, Rng &rng) {
    if (nHP == nMaxHP) {
        return 0;
    }
    int nTmp = nData2 - nData1 + 1;
    if (nTmp < 0) nTmp = -nTmp;
    nTmp = (nTmp > 1) ? nTmp : 1;
    int nAdd = nData1 + (int)((unsigned short)rng () % nTmp);
    if (nAdd + nHP > nMaxHP) {
        nAdd = nMaxHP - nHP;
    }
    return nAdd;
}

} // namespace BattleCalc
