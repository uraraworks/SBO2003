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
a = recv.index('int FindBowAttackTarget(PCHARINFO c, BYTE rate)\n{')
b = recv.index('\nint FindNextAttackTarget', a)
code = r'''
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstdio>
using std::min;
using DWORD = unsigned int;
using BYTE = unsigned char;
using BOOL = bool;
const bool TRUE = true;
const int NPCTYPE_INIT=0, NPCTYPE_BATTLE1=1, NPCTYPE_BATTLE2=2, NPCTYPE_MIMIC=3;
const int CHARSTATE_DEAD=2, ITEMTYPE_WEAPON=3, ATACKTYPE_BOW=4, ATACKRATE_JUST=101;
const int TURNGAGE_LAP=20, TURNGAGE_LAPMAX=5;
struct CHARINFO {
    DWORD dwX=0,dwY=0,dwMapID=0,dwTimeDelete=0;
    int byNpcType=0,wHP=1,byCharState=0,byDirection=0,byDotMove=0;
    DWORD dwBowStartTime=0,dwBowDuration=0,dwBowElapsed=0;
    int nBowStartX=0,nBowStartY=0,nBowEndX=0,nBowEndY=0,nBowDrawX=0,nBowDrawY=0;
};
using PCHARINFO=CHARINFO*;
struct ITEMINFO_WEAPON { int byAtackType=ATACKTYPE_BOW,byData1=5; } weapon;
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
'''
code += 'void tick(PCHARINFO pChar) {\n' + main[start:end] + '\n}\n' + recv[a:b]
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
    c.dwX=5;c.dwY=5;c.byDirection=3;
    near.byNpcType=far.byNpcType=tie.byNpcType=NPCTYPE_BATTLE1;
    near.dwX=4;near.dwY=5;far.dwX=8;far.dwY=5;tie.dwX=6;tie.dwY=5;
    g_aCharInfo={};g_aCharInfo.push_back(&c);g_aCharInfo.push_back(&near);g_aCharInfo.push_back(&far);g_aCharInfo.push_back(&tie);
    assert(FindBowAttackTarget(&c,100)==3); // 同距離では向きの角度を優先
    tie.wHP=0;assert(FindBowAttackTarget(&c,100)==1); // 最寄りは向きより優先
    near.dwTimeDelete=1;assert(FindBowAttackTarget(&c,100)==2);
    far.dwMapID=1;assert(FindBowAttackTarget(&c,100)==-1);
    far.dwMapID=0;far.dwX=12;assert(FindBowAttackTarget(&c,20)==-1);
    assert(FindBowAttackTarget(&c,100)==2);
    c.byDirection=3;c.byDotMove=19;
    assert(BowOffsetX(&c)==19 && BowOffsetY(&c)==0);
    puts("bow client: 8 directions, straight interpolation, target priority, invalid targets, range, legacy offsets OK");
}
'''
with tempfile.TemporaryDirectory(prefix='sbo-bow-client-') as temp:
    src=Path(temp)/'bow.cpp'; src.write_text(code)
    binary=Path(temp)/'bow'
    subprocess.run(['c++','-std=c++17','-O2','-I',str(root),str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
