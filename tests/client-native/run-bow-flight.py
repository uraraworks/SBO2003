#!/usr/bin/env python3
"""実装中の弓の補間・対象選択を抽出して検証する。ゲームデータや描画環境は不要。"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
main = (root / 'SboCli/src/SboCliMain.cpp').read_text()
start = main.index('    if (pChar->dwBowDuration) {', main.index('void TimerProcCharDotMove(PCHARINFO pChar)\n{'))
end = main.index('\n\t/* 自分が一歩', start)
recv = (root / 'SboCli/src/SboCliRecvCmd.cpp').read_text()
send = (root / 'SboCli/src/SboCliSendCmd.cpp').read_text()
key = (root / 'SboCli/src/SboCliKeyProc.cpp').read_text()
a = recv.index('BOOL IsBowAttackTarget(PCHARINFO c, PCHARINFO t)\n{')
b = recv.index('\nint FindNextAttackTarget', a)
code = r'''
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstring>
using std::min;
using DWORD = unsigned int;
using BYTE = unsigned char;
using BOOL = bool;
const bool TRUE = true, FALSE = false;
using std::max;
const int NPCTYPE_INIT=0, NPCTYPE_BATTLE1=1, NPCTYPE_BATTLE2=2, NPCTYPE_MIMIC=3;
const int CHARSTATE_DEAD=2, ITEMTYPE_WEAPON=3, ATACKTYPE_BOW=4, ATACKTYPE_ARROW=5, ATACKRATE_JUST=101;
const int TURNGAGE_LAP=20, TURNGAGE_LAPMAX=5;
struct CHARINFO {
    DWORD dwX=0,dwY=0,dwMapID=0,dwTimeDelete=0,dwTblID=0;
    DWORD dwAtackTblID=0,dwTimeTurnLV=0,dwTimeTurnMagicLV=0;
    BYTE byTurnLV=0;
    DWORD dwLastTurnLV=0;
    int byNpcType=0,wHP=1,byCharState=0,byDirection=0,byDotMove=0;
    DWORD dwBowStartTime=0,dwBowDuration=0,dwBowElapsed=0;
    int nBowStartX=0,nBowStartY=0,nBowEndX=0,nBowEndY=0,nBowDrawX=0,nBowDrawY=0;
};
using PCHARINFO=CHARINFO*;
struct ITEMINFO_WEAPON { int byAtackType=ATACKTYPE_ARROW,byData1=5; } weapon;
using PITEMINFO_WEAPON=ITEMINFO_WEAPON*;
struct ITEMINFO { int byType; void* pData; };
using PITEMINFO=ITEMINFO*;
int anPosAddTblX[]={0,0,-1,1},anPosAddTblY[]={-1,1,0,0};
DWORD now=0;
DWORD timeGetTime() { return now; }
bool g_bDraw=false;
struct Characters: std::vector<PCHARINFO> { int GetSize(){return size();} } g_aCharInfo;
PITEMINFO_WEAPON GetBowWeapon(PCHARINFO) { return &weapon; }
#include "common/BowFlight.h"
bool g_bBattleMode=true,g_bGuard=false,g_bAutoGage=false,g_bReqestPos=false;
DWORD g_dwGuardGageElapse=0;
struct { bool byFadeIn=false,byFadeOut=false; } g_FadeInOutInfo;
const int MAPFLG_BATTLE=1;
struct Map { int byFlg=MAPFLG_BATTLE; } map;
Map* g_pMapInfo=&map;
int unlocked=0;
using PBYTE=unsigned char*;
const int CMD_REQ_ATTACK=1;
struct CMDPARA_DATA4 { BYTE byCmd=0,byData=0; DWORD pID=0,dwTblID=0,dwData=0; };
using PCMDPARA_DATA4=CMDPARA_DATA4*;
void ZeroMemory(void* p, size_t n) { memset(p,0,n); }
int g_nMyCharID=0;
DWORD g_pMyID=9,g_dwTurnGageContinue=0,m_dwTimeZDown=0;
int sends=0;
CMDPARA_DATA4 packet;
struct Socket { void Send(PBYTE p,size_t n){assert(n==sizeof(packet));memcpy(&packet,p,n);++sends;} } cliSock;
bool just=false;
BOOL IsTurnGageJust(PCHARINFO) { return just; }
void GuardEnd() { g_bGuard=false; }
void SendCmdTARGETCHAR(DWORD id) { if (id==(DWORD)-1) ++unlocked; }
int GetCharNo2(DWORD id) {
    for(int i=0;i<g_aCharInfo.GetSize();++i) if(g_aCharInfo[i]->dwTblID==id) return i;
    return -1;
}

'''
gage_a = recv.index('static DWORD s_dwBowGageX')
gage_b = recv.index('/* ========================================================================= */', recv.index('void CancelTurnGage(PCHARINFO pChar)\n{'))
code += recv[gage_a:gage_b]
code += 'void tick(PCHARINFO pChar) {\n' + main[start:end] + '\n}\n' + recv[a:b]
def function(source, signature):
    start = source.index(signature + '\n{')
    end = source.index('\n}', start) + 2
    return source[start:end] + '\n'
code += function(recv, 'void ContinueTurnGage(PCHARINFO pChar)')
start = send.index('void SendCmdREQ_ATTACK(\n')
end = send.index('\n}', start) + 2
code += send[start:end] + '\n'
code += function(key, 'static BOOL ShotTurnGage(PCHARINFO pChar)')
code += r'''
int main() {
    for (int dx: {-137, 0, 137}) for (int dy: {-91, 0, 91}) {
        if (!dx && !dy) continue;
        CHARINFO c;
        c.dwBowDuration=1000;c.dwBowStartTime=100;
        c.nBowStartX=200;c.nBowStartY=200;
        c.nBowEndX=200+dx;c.nBowEndY=200+dy;
        for (int t=0;t<=1000;t+=10) {
            now=100+t;tick(&c);
            assert(abs(c.nBowDrawX-(200+dx*t/1000.0))<1.1);
            assert(abs(c.nBowDrawY-(200+dy*t/1000.0))<1.1);
            c.dwX=(c.nBowDrawX+16)/32;c.dwY=(c.nBowDrawY+16)/32;
            assert((int)c.dwX*32-BowOffsetX(&c)==c.nBowDrawX);
            assert((int)c.dwY*32-BowOffsetY(&c)==c.nBowDrawY);
        }
        assert(c.byDotMove==0);
    }
    CHARINFO c,near,far,tie;
    ITEMINFO item{ITEMTYPE_WEAPON,&weapon};
    assert(BowWeapon(&c,&item)); // プレイヤーの ATACKTYPE_ARROW が対象
    weapon.byAtackType=ATACKTYPE_BOW;assert(BowWeapon(&c,&item));
    weapon.byAtackType=ATACKTYPE_ARROW;
    c.dwX=5;c.dwY=5;c.byDirection=3;
    near.byNpcType=far.byNpcType=tie.byNpcType=NPCTYPE_BATTLE1;
    near.dwX=4;near.dwY=5;far.dwX=8;far.dwY=5;tie.dwX=6;tie.dwY=5;
    g_aCharInfo={};g_aCharInfo.push_back(&c);g_aCharInfo.push_back(&near);g_aCharInfo.push_back(&far);g_aCharInfo.push_back(&tie);
    near.dwTblID=1;far.dwTblID=2;tie.dwTblID=3;
    assert(FindBowAttackTarget(&c,100)==-1); // 未選択では自動ロック・開始しない
    now=100;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0);
    StartTurnGage(&c,far.dwTblID);c.byTurnLV=60;
    assert(FindBowAttackTarget(&c,20)==2); // 近い敵がいても選んだ相手だけ
    tie.dwX=10;tie.dwY=10;assert(IsBowAttackTarget(&c,&tie)); // 射程端の斜め
    tie.dwX=11;assert(!IsBowAttackTarget(&c,&tie));
    c.byDotMove=10;UpdateBowTurnGage(&c);
    assert(c.dwAtackTblID==2 && c.byTurnLV==0 && c.dwTimeTurnLV==0);
    c.byDotMove=0;now=200;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==200);
    c.byTurnLV=80;c.byDirection=0;UpdateBowTurnGage(&c);
    assert(c.dwTimeTurnLV==0 && c.byTurnLV==0 && c.dwAtackTblID==2);
    now=300;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==300);
    far.dwX=12;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0 && c.dwAtackTblID==2);
    now=400;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0);
    far.dwX=8;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==400);
    c.byTurnLV=80;StartTurnGage(&c,1);assert(c.byTurnLV==0 && c.dwAtackTblID==1);
    near.wHP=0;UpdateBowTurnGage(&c);assert(!c.dwAtackTblID && !c.dwTimeTurnLV && unlocked==1);
    StartTurnGage(&c,2);far.dwTimeDelete=1;UpdateBowTurnGage(&c);assert(!c.dwAtackTblID && unlocked==2);
    far.dwTimeDelete=0;StartTurnGage(&c,2);far.dwMapID=1;UpdateBowTurnGage(&c);assert(!c.dwAtackTblID && unlocked==3);
    far.dwMapID=0;StartTurnGage(&c,2);g_aCharInfo.pop_back();g_aCharInfo.pop_back();
    UpdateBowTurnGage(&c);assert(!c.dwAtackTblID && unlocked==4); // テーブルから消失
    g_aCharInfo.push_back(&far);StartTurnGage(&c,2);g_bBattleMode=false;
    UpdateBowTurnGage(&c);assert(!c.dwAtackTblID && unlocked==5);
    g_bBattleMode=true;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0); // 自動再ロックしない
    // 早撃ち・ジャスト・満タン相当の要求でも同じ相手へ送り、次のゲージも同じ相手。
    for (int rate: {20,100,ATACKRATE_JUST}) {
        ++now;StartTurnGage(&c,2);c.byTurnLV=(rate==20?20:100);just=(rate==ATACKRATE_JUST);
        int before=sends;assert(ShotTurnGage(&c));
        assert(sends==before+1 && packet.dwData==2 && packet.byData==rate);
        assert(c.dwAtackTblID==2 && c.byTurnLV==0 && c.dwTimeTurnLV==now);
    }
    c.byTurnLV=80;c.byDotMove=10;int before=sends;
    assert(!ShotTurnGage(&c) && sends==before && c.byTurnLV==0 && c.dwAtackTblID==2);
    c.byDotMove=0;UpdateBowTurnGage(&c);
    SendCmdREQ_ATTACK(1,100);assert(sends==before); // 別の相手に勝手に送らない
    c.byDirection=3;c.byDotMove=19;
    assert(BowOffsetX(&c)==19 && BowOffsetY(&c)==0);
    puts("bow client: 8 directions, straight interpolation, selected target, charge reset/resume, early/just/continuous shots, invalid targets, range, legacy offsets OK");
}
'''
with tempfile.TemporaryDirectory(prefix='sbo-bow-client-') as temp:
    src=Path(temp)/'bow.cpp'; src.write_text(code)
    binary=Path(temp)/'bow'
    subprocess.run(['c++','-std=c++17','-O2','-I',str(root),str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
