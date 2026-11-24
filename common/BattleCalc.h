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

} // namespace BattleCalc
