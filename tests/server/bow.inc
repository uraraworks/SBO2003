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
    weapon->byAtackType = ATACKTYPE_ARROW; weapon->byData1 = 3;
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
    BowCheck(pc->dwLastBattle == 0, "diagonal request rejected");
    pc->dwTargetTblID = enemy->dwTblID;
    BattleProcWeapon(pc);
    bool diagonalArrow = false;
    for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
        if (g_aCharInfo[i]->dwBowDuration && g_aCharInfo[i]->pID) diagonalArrow = true;
    BowCheck(!diagonalArrow, "diagonal launch rejected");
    enemy->dwY = 2;
    TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 1 && enemy->dwLastBattle == 0, "request does not alert enemy");
    BattleProcCheck(pc);
    PCHARINFO arrow = NULL;
    for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
        if (g_aCharInfo[i]->dwBowDuration) arrow = g_aCharInfo[i];
    BowCheck(arrow && arrow->nBowEndX == 128 && arrow->nBowEndY == 64 && arrow->byDirection == 3 && pc->byDirection == 3, "horizontal fixed endpoint and shooter direction");
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
    enemy->dwX = 5; enemy->dwY = 2;
    BowCheck(arrow->nBowEndX == 128, "moving target does not change trajectory");
    enemy->dwX = 4; enemy->dwY = 2;
    s_dwFakeNow += 300;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle != 0 && enemy->dwTargetTblID == pc->dwTblID, "contact alerts enemy to shooter");
    BowCheck(g_aCharInfo[arrowID]->pID == 0, "arrow disappears on first contact");
    BowCheck(BowRange(5,20) == 4 && BowRange(5,100) == 6 &&
        BowTileTime(20) == 98 && BowTileTime(100) == 50, "lap range and speed");
    BowCheck(BowWeapon(pc,item) != NULL, "player arrow weapons use lock-on");
    pc->dwTargetTblID = (DWORD)-1;
    enemy->dwLastBattle = 0; enemy->dwTargetTblID = (DWORD)-1;
    enemy->dwSpaceID = 9;
    pc->dwLastBattle = 0; TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0, "different space rejected");
    enemy->dwSpaceID = 0; enemy->dwX = 9; enemy->dwY = 2;
    TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0, "out of range request rejected");
    enemy->dwX = 4; enemy->dwY = 2;
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
    enemy->dwX = 6; enemy->dwY = 2;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle == 0 && !g_aCharInfo[arrowID]->pID, "target can dodge fixed trajectory");
    enemy->dwX = 4; enemy->dwY = 2;
    arrow = fire(); BowCheck(arrow && arrow->nBowEndX == 128 && arrow->nBowEndY == 64, "current position aimed without prediction");
    DeleteChar(arrow->dwTblID);
    int cell = 3 + 2 * map->nCX;
    WORD oldPart = map->pwMap[cell];
    // 合成パーツを一つだけ壁にして通過マスの判定を確認する。
    PMAPPARTSINFO oldParts = g_aMapPartsInfo;
    g_aMapPartsInfo = new MAPPARTSINFO[g_wCountMapParts+1];
    CopyMemory(g_aMapPartsInfo, oldParts, sizeof(MAPPARTSINFO)*g_wCountMapParts);
    WORD wall = g_wCountMapParts++;
    g_aMapPartsInfo[wall] = g_aMapPartsInfo[oldPart];
    g_aMapPartsInfo[wall].byHitType = PARTSHIT_BLOCK1;
    arrow = fire(); arrowID = arrow->dwTblID;
    map->pwMap[cell] = wall;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(!g_aCharInfo[arrowID]->pID && enemy->dwLastBattle == 0, "intermediate wall stops arrow before enemy");
    pc->dwLastBattle = 0; TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0, "wall blocks targeting");
    map->pwMap[cell] = oldPart;
    weapon->byData1 = 5;
    enemy->dwX = 5; enemy->dwY = 2; enemy->dwLastBattle = 0;
    arrow = fire(); arrowID = arrow->dwTblID;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle != 0 && !g_aCharInfo[arrowID]->pID, "horizontal three tiles hits");
    g_aMapPartsInfo[wall].byHitType = PARTSHIT_COUNTER;
    map->pwMap[cell] = wall;
    enemy->dwLastBattle = 0;
    arrow = fire(); arrowID = arrow->dwTblID;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle != 0 && !g_aCharInfo[arrowID]->pID, "counter passes arrow");
    map->pwMap[cell] = oldPart;
    // 扉の画像が歩ける・カウンターであっても、閉じたイベント扉で遮る。
    int oldEventCount = map->nEventCount;
    PEVENTINFO oldEvents = map->pEvent;
    EVENTINFO doorEvent = {};
    EVENTINFO_DOOR door = {};
    doorEvent.byType = EVENTTYPE_DOOR; doorEvent.dwX = 3; doorEvent.dwY = 2;
    doorEvent.pData = &door;
    door.wGrpID = wall; door.wBackGrpID = oldPart;
    enemy->dwLastBattle = 0;
    arrow = fire(); arrowID = arrow->dwTblID;
    map->nEventCount = 1; map->pEvent = &doorEvent;
    map->pwMap[cell] = wall;
    pc->dwLastBattle = 0; TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle == 0 && IsBowWall(pc, 3, 2), "closed door rejects request even with counter hit type");
    int liveArrows = 0;
    for (int i=0;i<g_aCharInfo.GetSize();++i)
        if (g_aCharInfo[i]->dwBowDuration && g_aCharInfo[i]->pID) ++liveArrows;
    BattleProcWeapon(pc);
    int afterArrows = 0;
    for (int i=0;i<g_aCharInfo.GetSize();++i)
        if (g_aCharInfo[i]->dwBowDuration && g_aCharInfo[i]->pID) ++afterArrows;
    BowCheck(liveArrows == afterArrows, "closed door rejects launch revalidation");
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(!g_aCharInfo[arrowID]->pID && enemy->dwLastBattle == 0, "door closed during flight stops arrow");
    map->pwMap[cell] = oldPart;
    BowCheck(IsBowWall(pc, 3, 2), "closed door blocks even with walkable image");
    door.dwTimeOpen = s_dwFakeNow;
    pc->dwLastBattle = 0; TestRecv(pc, &req, sizeof(req));
    BowCheck(pc->dwLastBattle != 0 && !IsBowWall(pc, 3, 2), "open normal-space door accepts request");
    arrow = fire(); arrowID = arrow->dwTblID;
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(!g_aCharInfo[arrowID]->pID && enemy->dwLastBattle != 0, "open door passes flight");
    door.dwTimeOpen = 0;
    pc->dwSpaceID = enemy->dwSpaceID = 7;
    BowCheck(IsBowWall(pc, 3, 2), "private-space door initially closed");
    BowCheck(SpaceDoorOpen(pc->dwMapID, &doorEvent, &door, 7), "private-space door opens");
    g_aMapPartsInfo[wall].byHitType = PARTSHIT_BLOCK1;
    map->pwMap[cell] = wall;
    BowCheck(!IsBowWall(pc, 3, 2) && IsBowReachChar(pc, enemy), "open private door ignores shared closed image");
    pc->dwSpaceID = enemy->dwSpaceID = 8;
    BowCheck(IsBowWall(pc, 3, 2) && !IsBowReachChar(pc, enemy), "other private-space door remains closed");
    pc->dwSpaceID = enemy->dwSpaceID = 0;
    map->nEventCount = oldEventCount; map->pEvent = oldEvents;
    map->pwMap[cell] = oldPart;
    enemy->dwX = 7; enemy->dwY = 2; enemy->dwLastBattle = 0;
    pc->dwTargetTblID = enemy->dwTblID; pc->byAtackRate = 20;
    BowCheck(BattleProcWeapon(pc), "lap one launches");
    arrow = NULL;
    for (int i = 0; i < g_aCharInfo.GetSize(); ++i)
        if (g_aCharInfo[i]->dwBowDuration && g_aCharInfo[i]->pID) arrow = g_aCharInfo[i];
    BowCheck(arrow != NULL, "lap one arrow created");
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle == 0, "lap one does not reach five tiles");
    arrow = fire();
    s_dwFakeNow += 1000;
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(enemy->dwLastBattle != 0, "lap five reaches five tiles");
    for (int direction = 0; direction < 4; ++direction) {
        pc->dwX = pc->dwY = 5;
        pc->byDirection = (direction + 1) % 4;
        enemy->dwX = 5 + anPosAddTblX[direction] * 2;
        enemy->dwY = 5 + anPosAddTblY[direction] * 2;
        enemy->dwLastBattle = 0;
        arrow = fire(); BowCheck(arrow && arrow->byDirection == direction && pc->byDirection == direction,
            "cardinal shooter and arrow direction");
        arrowID = arrow->dwTblID;
        s_dwFakeNow += 1000;
        ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
        BowCheck(enemy->dwLastBattle != 0 && !g_aCharInfo[arrowID]->pID, "cardinal direction hits");
    }
    pc->dwX = pc->dwY = 2;
    enemy->dwX = 5; enemy->dwY = 2; enemy->dwLastBattle = 0;
    PCHARINFO between = BowTestChar(NPCTYPE_BATTLE1, 3, 2);
    arrow = fire(); arrowID = arrow->dwTblID;
    s_dwFakeNow += 1000;
    SvrSendCaptureBegin();
    ((CCharProcATACK*)arrow->pProc)->Proc(s_dwFakeNow);
    BowCheck(SvrSendCaptureEnd(packet) && packet.size() == sizeof(CMDPARA_CHARPOS),
        "contact notification preserves CHARPOS header size");
    CopyMemory(&pos, packet.data(), sizeof(pos));
    BowCheck(pos.byCmd == CMD_CHARPOS && pos.byFlg == FLGCHARPOS_BOWHIT &&
        pos.dwTblID == between->dwTblID && pos.pID == between->pID,
        "contact notification identifies intermediate enemy before damage");
    BowCheck(between->dwLastBattle != 0 && enemy->dwLastBattle == 0 && !g_aCharInfo[arrowID]->pID,
        "intermediate enemy takes first hit");
    --g_wCountMapParts;
    delete[] g_aMapPartsInfo; g_aMapPartsInfo = oldParts;
    printf("bow tests passed\n");
    return 0;
}
