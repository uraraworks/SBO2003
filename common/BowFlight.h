#pragma once
#include <math.h>
extern int anPosAddTblX[], anPosAddTblY[];

/* プロトタイプ: 1～5周。1周目は基礎射程、以後1周につき1マス追加。 */
inline int BowLaps(int rate) {
    if (rate == ATACKRATE_JUST || rate == 0) rate = 100;
    return rate < TURNGAGE_LAP ? 1 : (rate > 100 ? TURNGAGE_LAPMAX : rate / TURNGAGE_LAP);
}
inline int BowRange(int base, int rate) { return base + BowLaps(rate) - 1; }
/* 1マス200/175/150/125/100ms。斜めでも実距離あたりの速度は同じ。 */
inline int BowTileTime(int rate) { return 225 - 25 * BowLaps(rate); }
inline PITEMINFO_WEAPON BowWeapon(PCHARINFO c, PITEMINFO item) {
    if (!c || c->byNpcType != NPCTYPE_INIT || !item || item->byType != ITEMTYPE_WEAPON || !item->pData) return NULL;
    PITEMINFO_WEAPON w = (PITEMINFO_WEAPON)item->pData;
    return w->byAtackType == ATACKTYPE_BOW ? w : NULL;
}
inline BOOL BowEnemy(PCHARINFO c) {
    return c && c->wHP && c->byCharState != CHARSTATE_DEAD &&
        (c->byNpcType == NPCTYPE_BATTLE1 || c->byNpcType == NPCTYPE_BATTLE2 || c->byNpcType == NPCTYPE_MIMIC);
}
inline int BowOffsetX(PCHARINFO c) {
    return c->dwBowDuration ? (int)c->dwX * 32 - c->nBowDrawX : anPosAddTblX[c->byDirection] * c->byDotMove;
}
inline int BowOffsetY(PCHARINFO c) {
    return c->dwBowDuration ? (int)c->dwY * 32 - c->nBowDrawY : anPosAddTblY[c->byDirection] * c->byDotMove;
}
