#pragma once
#include <math.h>
extern int anPosAddTblX[], anPosAddTblY[];

/* 周回数は1～5。 */
inline int BowLaps(int rate) {
    if (rate == ATACKRATE_JUST || rate == 0) rate = 100;
    return rate < TURNGAGE_LAP ? 1 : (rate > 100 ? TURNGAGE_LAPMAX : rate / TURNGAGE_LAP);
}
inline int BowRange(int base, int rate) { return (base * (6 + BowLaps(rate)) + 9) / 10; }
/* 1マスあたり98/86/74/62/50ms。 */
inline int BowTileTime(int rate) { return 110 - 12 * BowLaps(rate); }
inline PITEMINFO_WEAPON BowWeapon(PCHARINFO c, PITEMINFO item) {
    if (!c || c->byNpcType != NPCTYPE_INIT || !item || item->byType != ITEMTYPE_WEAPON || !item->pData) return NULL;
    PITEMINFO_WEAPON w = (PITEMINFO_WEAPON)item->pData;
    return (w->byAtackType == ATACKTYPE_ARROW || w->byAtackType == ATACKTYPE_BOW) ? w : NULL;
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
