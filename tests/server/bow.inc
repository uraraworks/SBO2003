/* 公開データだけで実行できる弓の通信・接触テスト。 --bow を指定する。 */
#include "common/BowFlight.h"
#include "SboSvrBattleProc.h"
#include "CharProcATACK.h"
#include <assert.h>

static PCHARINFO BowTestChar(BYTE type, int x, int y)
{
    PCHARINFO c = new CHARINFO;
    ZeroMemory(c, sizeof(*c));
    c->byNpcType = type;
    SetInitCharData(c);
    c->dwX = c->dwXBack = x; c->dwY = c->dwYBack = y;
    c->byGameState = type ? GAMESTATE_NPC : GAMESTATE_MOVE;
    c->byCharState = CHARSTATE_MOVE;
    c->wHP = c->wMaxHP = 30000;
    c = AddChar(c, FALSE);
    c->pID = c->dwTblID + 1;
    return c;
}
static void BowCheck(bool ok, const char *label)
{
    if (!ok) { fprintf(stderr, "bow FAILED: %s\n", label); exit(1); }
    printf("bow OK: %s\n", label);
}
static int TestBowOnly()
{
    WNDCLASS wc = {};
    wc.lpszClassName = "BOWTEST"; wc.lpfnWndProc = (WNDPROC)WndProc;
    RegisterClass(&wc);
    CreateWindow("BOWTEST", "t", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    BowCheck(g_aMapInfo.GetSize() > 0, "public map loaded");
    PMAPINFO map = g_aMapInfo[0]; map->byFlg |= MAPFLG_BATTLE;
    PITEMINFO item = new ITEMINFO; ZeroMemory(item, sizeof(*item));
    PITEMINFO_WEAPON weapon = new ITEMINFO_WEAPON; ZeroMemory(weapon, sizeof(*weapon));
    item->byType = ITEMTYPE_WEAPON; item->pData = weapon;
    weapon->byAtackType = ATACKTYPE_BOW; weapon->byData1 = 3;
    weapon->byData2 = weapon->byData3 = weapon->byData4 = weapon->byData5 = 1;
    int itemID = g_aItemInfo.GetSize(); g_aItemInfo.Add(item);
    PCHARINFO pc = BowTestChar(NPCTYPE_INIT, 2, 2);
    int conn = Connect(1000, TRUE);
    pc->pID = URARASOCK_IDBASE + conn;
    pc->waEqu[0] = 0; pc->waItem[0] = itemID; pc->wAT = 10;
    PCHARINFO enemy = BowTestChar(NPCTYPE_BATTLE1, 4, 4);
    CMDPARA_DATA4 req = {};
    req.byCmd = CMD_REQ_ATTACK; req.pID = pc->pID; req.dwTblID = pc->dwTblID;
    req.dwData = enemy->dwTblID; req.byData = 100;
    TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 1 && enemy->dwLastBattle == 0, "request does not alert enemy");
    BattleProcCheck(pc);
    PCHARINFO arrow = NULL;
    for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
        if (g_aCharInfo[i]->dwBowDuration) arrow = g_aCharInfo[i];
    BowCheck(arrow && arrow->nBowEndX == 128 && arrow->nBowEndY == 128, "diagonal fixed endpoint");
    std::vector<BYTE> packet;
    SvrSendCaptureBegin();
    SendCmdCHARPOS(arrow->pID, arrow->dwTblID, FLGCHARPOS_BOWLAUNCH, 1);
    BowCheck(SvrSendCaptureEnd(packet) && packet.size() == sizeof(CMDPARA_CHARPOS) + sizeof(BOWFLIGHTINFO), "launch packet keeps header and appends flight data");
    CMDPARA_CHARPOS pos; CopyMemory(&pos, packet.data(), sizeof(pos));
    BowCheck(pos.byDirection < 4 && (pos.byFlg & FLGCHARPOS_BOWLAUNCH) && (pos.byFlg & FLGCHARPOS_BOW), "launch flag and legacy direction are compatible");
    SvrSendCaptureBegin();
    SendCmdCHARPOS(arrow->pID, arrow->dwTblID, 0, 1);
    BowCheck(SvrSendCaptureEnd(packet), "movement packet sent separately");
    CopyMemory(&pos, packet.data(), sizeof(pos));
    BowCheck(!(pos.byFlg & FLGCHARPOS_BOWLAUNCH) && (pos.byFlg & FLGCHARPOS_BOW), "movement is distinct from launch");
    BowCheck(enemy->dwLastBattle == 0, "launch does not alert enemy");
    DWORD arrowID = arrow->dwTblID;
    s_dwFakeNow += 60;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle == 0, "flight does not alert enemy before contact");
    enemy->dwX = 5; enemy->dwY = 4;
    BowCheck(arrow->nBowEndX == 128, "moving target does not change trajectory");
    enemy->dwX = 4; enemy->dwY = 4;
    s_dwFakeNow += 300;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle != 0 && enemy->dwTargetTblID == pc->dwTblID, "contact alerts enemy to shooter");
    BowCheck(g_aCharInfo[arrowID]->pID == 0, "arrow disappears on first contact");
    BowCheck(BowRange(3,20) == 3 && BowRange(3,100) == 7 && BowRange(3,ATACKRATE_JUST) == 7 &&
        BowTileTime(20) == 200 && BowTileTime(100) == 100, "lap prototype values");
    weapon->byAtackType = ATACKTYPE_ARROW;
    BowCheck(!BowWeapon(pc,item), "throwing weapons are not bows");
    pc->dwTargetTblID = (DWORD)-1;
    BowCheck(BattleProcWeapon(pc), "legacy projectile still launches without target");
    arrow = NULL;
    for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
        if (g_aCharInfo[i]->byNpcType == NPCTYPE_ATACK && g_aCharInfo[i]->pID) arrow = g_aCharInfo[i];
    BowCheck(arrow && !arrow->dwBowDuration && arrow->wHP == 3, "legacy projectile keeps original range and movement");
    SvrSendCaptureBegin();
    SendCmdCHARPOS(arrow->pID, arrow->dwTblID, 0, 1);
    BowCheck(SvrSendCaptureEnd(packet) && packet.size() == sizeof(CMDPARA_CHARPOS), "legacy projectile packet size is unchanged");

    DeleteChar(arrow->dwTblID);
    weapon->byAtackType = ATACKTYPE_BOW;
    enemy->dwLastBattle = 0; enemy->dwTargetTblID = (DWORD)-1;
    enemy->dwSpaceID = 9;
    pc->dwLastBattle = 0; TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0, "different space rejected");
    enemy->dwSpaceID = 0; enemy->dwX = 9; enemy->dwY = 9;
    TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0, "out of range request rejected");
    enemy->dwX = 4; enemy->dwY = 4;
    TestRecv(pc, &req, sizeof(req)-1);
    BowCheck(pc->dwLastBattle == 0, "short attack packet rejected");
    auto fire = [&]() -> PCHARINFO {
        pc->dwTargetTblID = enemy->dwTblID; pc->byAtackRate = 100;
        BowCheck(BattleProcWeapon(pc), "bow launches");
        for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
            if (g_aCharInfo[i]->dwBowDuration && g_aCharInfo[i]->pID) return g_aCharInfo[i];
        return NULL;
    };
    arrow = fire(); BowCheck(arrow != NULL, "miss test arrow created");
    arrowID = arrow->dwTblID;
    enemy->dwX = 6; enemy->dwY = 4;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle == 0 && !g_aCharInfo[arrowID]->pID, "target can dodge fixed trajectory");
    enemy->dwX = 4; enemy->dwY = 4;
    enemy->dwBowMoveInterval = 320; enemy->dwBowMoveTime = s_dwFakeNow;
    enemy->nBowVelocityX = 1; enemy->nBowVelocityY = 0;
    arrow = fire(); BowCheck(arrow && arrow->nBowEndX > 128, "moving enemy is led using observed speed");
    DeleteChar(arrow->dwTblID); enemy->dwBowMoveInterval = 0;
    int cell = 3 + 3 * map->nCX;
    WORD oldPart = map->pwMap[cell];
    // 合成パーツを一つだけ壁にして通過マスの判定を確認する。
    PMAPPARTSINFO oldParts = g_aMapPartsInfo;
    g_aMapPartsInfo = new MAPPARTSINFO[g_wCountMapParts+1];
    CopyMemory(g_aMapPartsInfo, oldParts, sizeof(MAPPARTSINFO)*g_wCountMapParts);
    WORD wall = g_wCountMapParts++;
    g_aMapPartsInfo[wall] = g_aMapPartsInfo[oldPart];
    g_aMapPartsInfo[wall].byHitType = PARTSHIT_BLOCK1;
    map->pwMap[cell] = wall;
    arrow = fire(); arrowID = arrow->dwTblID;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(!g_aCharInfo[arrowID]->pID && enemy->dwLastBattle == 0, "intermediate wall stops arrow before enemy");
    map->pwMap[cell] = oldPart; --g_wCountMapParts;
    delete[] g_aMapPartsInfo; g_aMapPartsInfo = oldParts;
    printf("bow tests passed\n");
    return 0;
}
