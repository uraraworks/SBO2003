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
a = recv.index('static BOOL IsBowWall(int x, int y)\n{')
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
bool bowEquipped=true;
PITEMINFO_WEAPON GetBowWeapon(PCHARINFO) { return bowEquipped ? &weapon : nullptr; }
#include "common/BowFlight.h"
bool g_bBattleMode=true,g_bGuard=false,g_bAutoGage=false,g_bReqestPos=false;
DWORD g_dwGuardGageElapse=0;
struct { bool byFadeIn=false,byFadeOut=false; } g_FadeInOutInfo;
const int MAPFLG_BATTLE=1;
const int PARTSHIT_COUNTER=1, PARTSHIT_BLOCK1=2;
const int EVENTTYPE_DOOR=1;
struct EVENTINFO_DOOR { unsigned short wGrpID=0,wBackGrpID=0; };
using PEVENTINFO_DOOR=EVENTINFO_DOOR*;
struct EVENTINFO { int byType=EVENTTYPE_DOOR; DWORD dwX=0,dwY=0; void* pData=nullptr; };
using PEVENTINFO=EVENTINFO*;
struct Map { int nEventCount=0; PEVENTINFO pEvent=nullptr; int byFlg=MAPFLG_BATTLE,nCX=16,nCY=16; unsigned short pwMap[256]={}; } map;
using PMAPINFO=Map*;
struct Part { int byHitType=0; } g_pMapPartsInfo[3];
int g_wCountMapParts=3;
bool CheckCharBlock(DWORD x,DWORD y) {
    return x<16 && y<16 && map.pwMap[x+y*16]==0;
}
Map* g_pMapInfo=&map;
int unlocked=0;
std::vector<DWORD> targetSends;
struct TARGETSELECTINFO { DWORD dwTblID=0; };
TARGETSELECTINFO* g_pTargetSelectInfo=nullptr;
#define SAFE_DELETE(p) do { delete (p); (p)=nullptr; } while(0)
void CancelTurnGage(PCHARINFO);
void PauseBowTurnGage(PCHARINFO);
void UpdateBowTurnGage(PCHARINFO);
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
void SendCmdTARGETCHAR(DWORD id) { targetSends.push_back(id); if (id==(DWORD)-1) ++unlocked; }
int GetCharNo2(DWORD id) {
    for(int i=0;i<g_aCharInfo.GetSize();++i) if(g_aCharInfo[i]->dwTblID==id) return i;
    return -1;
}

'''
gage_a = recv.index('static DWORD s_dwBowGageX')
gage_b = recv.index('/* ========================================================================= */', recv.index('void CancelTurnGage(PCHARINFO pChar)\n{'))
code += recv[gage_a:gage_b]
sight_a = recv.index('BOOL IsSightWall(int x, int y)\n{')
sight_b = recv.index('\n}', sight_a) + 2
code += recv[sight_a:sight_b]
code += 'void tick(PCHARINFO pChar) {\n' + main[start:end] + '\n}\n' + recv[a:b]
def function(source, signature):
    start = source.index(signature + '\n{')
    end = source.index('\n}', start) + 2
    return source[start:end] + '\n'
# CHARPOS の実際のヘッダー検証・命中フラグ分岐を通す。
common = (root / 'common/TypedefCommon.h').read_text()
flag = next(line for line in common.splitlines() if line.startswith('#define FLGCHARPOS_BOWHIT '))
code += flag + '\n'
code += 'struct CMDPARA_CHARPOS { DWORD dwTblID=0; BYTE byFlg=0; }; using PCMDPARA_CHARPOS=CMDPARA_CHARPOS*;\n'
pos_a = recv.index('    if (dwDataSize < sizeof(CMDPARA_CHARPOS)) return;', recv.index('void RecvDataProcCHARPOS('))
pos_b = recv.index('\tint x =', pos_a)
code += 'void hitPacket(PBYTE pData, DWORD dwDataSize) {\n' + recv[pos_a:pos_b] + '}\n'
code += function(recv, 'void ContinueTurnGage(PCHARINFO pChar)')
start = send.index('void SendCmdREQ_ATTACK(\n')
end = send.index('\n}', start) + 2
code += send[start:end] + '\n'
code += function(key, 'static BOOL ShotTurnGage(PCHARINFO pChar)')
# 実際のタイマーと、Ctrl・コマンド・相手選択のキャンセル部分を抽出する。
code += r'''
DWORD GetTurnGageTime(PCHARINFO) { return 1000; }
void TimerProcAutoTurnGage(PCHARINFO,DWORD) { assert(false); }
int FindFrontAttackTarget(PCHARINFO) { return -1; }
int FindNextAttackTarget(PCHARINFO) { return -1; }
int GetCharDistance(PCHARINFO,DWORD,DWORD,DWORD,int) { return 0; }
int min(BYTE a,int b) { return std::min((int)a,b); }
'''
code += function(main, 'void TimerProcCharTurnLV(PCHARINFO pChar)')
ctrl = key.index('g_bBattleMode = FALSE;', key.index('byCode == VK_CONTROL'))
code += 'void modeOff(PCHARINFO pChar) {\n' + key[ctrl:key.index('AddSysMsg',ctrl)] + '}\n'
for name, sig in [('commandCancel','void KeyProcBATTLE_COMMAND('), ('targetCancel','void KeyProcBATTLE_TARGET(')]:
    pos = key.index('PCHARINFO bowChar =', key.index(sig))
    code += 'void ' + name + '() {\n' + key[pos:key.index('int nNo =',pos)] + '}\n'
# コマンド内の X でも、溜まった弓を通常の ShotTurnGage に渡す。
command = key[key.index('void KeyProcBATTLE_COMMAND('):key.index('void KeyProcBATTLE_TARGET(')]
condition = command[command.index('(pWnd->nData1 == 0)'):].split(') {',1)[0]
code += 'struct Window { int nData1=0; };\nBOOL commandX(PCHARINFO pChar) { Window window; auto pWnd=&window; return ' + condition + '; }\n'

code += r'''
int main() {
    for (int dx: {-137, 0, 137}) for (int dy: {-91, 0, 91}) {
        if ((!dx) == (!dy)) continue;
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
    tie.dwX=10;tie.dwY=10;assert(!IsBowAttackTarget(&c,&tie)); // 斜めは射程内でも対象外
    for (int direction=0;direction<4;++direction) {
        tie.dwX=5+anPosAddTblX[direction]*5;tie.dwY=5+anPosAddTblY[direction]*5;
        assert(IsBowAttackTarget(&c,&tie)); // 縦横の射程端
        tie.dwX=5+anPosAddTblX[direction]*6;tie.dwY=5+anPosAddTblY[direction]*6;
        assert(!IsBowAttackTarget(&c,&tie));
    }
    tie.dwX=5;tie.dwY=5;assert(!IsBowAttackTarget(&c,&tie));
    g_pMapPartsInfo[1].byHitType=PARTSHIT_COUNTER;
    g_pMapPartsInfo[2].byHitType=PARTSHIT_BLOCK1;
    map.pwMap[6+5*16]=2;assert(!IsBowAttackTarget(&c,&far));
    map.pwMap[6+5*16]=1;assert(IsBowAttackTarget(&c,&far));
    map.pwMap[6+5*16]=0;
    c.byDotMove=10;UpdateBowTurnGage(&c);
    assert(c.dwAtackTblID==2 && c.byTurnLV==0 && c.dwTimeTurnLV==0);
    c.byDotMove=0;now=200;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==200);
    c.byTurnLV=80;c.byDirection=0;UpdateBowTurnGage(&c);
    assert(c.dwTimeTurnLV==200 && c.byTurnLV==80 && c.dwAtackTblID==2);
    PauseBowTurnGage(&c);assert(c.dwTimeTurnLV==0 && c.byTurnLV==0);
    now=300;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==300);
    far.dwX=12;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0 && c.dwAtackTblID==2);
    now=400;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0);
    far.dwX=8;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==400);
    c.byTurnLV=80;far.dwY=6;UpdateBowTurnGage(&c);
    assert(c.byTurnLV==0 && c.dwTimeTurnLV==0 && c.dwAtackTblID==2);
    now=450;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==0);
    far.dwY=5;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==450);
    c.byTurnLV=80;map.pwMap[6+5*16]=2;UpdateBowTurnGage(&c);
    assert(c.byTurnLV==0 && c.dwTimeTurnLV==0 && c.dwAtackTblID==2);
    map.pwMap[6+5*16]=0;now=500;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==500);
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
    // 移動して元の位置へ戻り、タイマーが実際に1周を溜めた状態でX。
    just=false;g_dwTurnGageContinue=now+100000;
    PauseBowTurnGage(&c);c.dwX=6;c.byDotMove=10;TimerProcCharTurnLV(&c);
    assert(c.byTurnLV==0 && !c.dwTimeTurnLV);
    c.dwX=5;c.byDotMove=0;TimerProcCharTurnLV(&c);
    now+=10;TimerProcCharTurnLV(&c);now+=200;TimerProcCharTurnLV(&c);
    assert(c.byTurnLV>=TURNGAGE_LAP && c.dwTimeTurnLV);
    SAFE_DELETE(g_pTargetSelectInfo);g_pTargetSelectInfo=new TARGETSELECTINFO{2};before=sends;
    assert(commandX(&c) && sends==before+1 && packet.dwData==2);
    assert(g_pTargetSelectInfo && c.dwAtackTblID==2); // 連続発射は枠を保持
    // 奥をロックして発射後、実際の接触先の通知で枠とロックだけを移す（縦横4方向）。
    auto hit = [&](DWORD id, BYTE flag=FLGCHARPOS_BOWHIT) {
        CMDPARA_CHARPOS pos;pos.dwTblID=id;pos.byFlg=flag;
        hitPacket((PBYTE)&pos,sizeof(pos));
    };
    g_aCharInfo.push_back(&near);near.wHP=1;
    for (int direction=0;direction<4;++direction) {
        far.dwX=5+anPosAddTblX[direction]*3;far.dwY=5+anPosAddTblY[direction]*3;
        near.dwX=5+anPosAddTblX[direction];near.dwY=5+anPosAddTblY[direction];
        StartTurnGage(&c,2);c.byTurnLV=80;c.dwLastTurnLV=now;
        DWORD started=c.dwTimeTurnLV,last=c.dwLastTurnLV;
        int count=targetSends.size();
        hit(1,0);assert(c.dwAtackTblID==2 && targetSends.size()==count);
        CMDPARA_CHARPOS shortPos;shortPos.dwTblID=1;shortPos.byFlg=FLGCHARPOS_BOWHIT;
        hitPacket((PBYTE)&shortPos,sizeof(shortPos)-1);assert(c.dwAtackTblID==2);
        hit(1);
        assert(c.dwAtackTblID==1 && g_pTargetSelectInfo && g_pTargetSelectInfo->dwTblID==1);
        assert(targetSends.size()==count+1 && targetSends.back()==1);
        assert(c.byTurnLV==80 && c.dwTimeTurnLV==started && c.dwLastTurnLV==last);
        ++now;TimerProcCharTurnLV(&c);assert(c.byTurnLV>=80);
        just=false;assert(ShotTurnGage(&c) && packet.dwData==1); // 次の発射先も接触先。
    }
    far.dwX=8;far.dwY=5;
    StartTurnGage(&c,2);c.byTurnLV=80;near.dwX=6;near.dwY=6;hit(1);
    assert(c.dwAtackTblID==1 && !c.dwTimeTurnLV && !c.byTurnLV);
    near.dwY=5;++now;UpdateBowTurnGage(&c);assert(c.dwTimeTurnLV==now);
    // 接触通知の後に死亡通知が届く順序でも、次のタイマーで解除する。
    StartTurnGage(&c,2);hit(1);near.wHP=0;TimerProcCharTurnLV(&c);
    assert(!c.dwAtackTblID && !g_pTargetSelectInfo && !c.dwTimeTurnLV);
    // 既に死亡を受信済みの場合も解除する。
    StartTurnGage(&c,2);hit(1);
    assert(!c.dwAtackTblID && !g_pTargetSelectInfo && !c.dwTimeTurnLV);
    int count=targetSends.size();hit(2);assert(!c.dwAtackTblID && targetSends.size()==count);
    bowEquipped=false;StartTurnGage(&c,2);c.byTurnLV=80;hit(1);
    assert(c.dwAtackTblID==2 && c.byTurnLV==80 && targetSends.size()==count);
    bowEquipped=true;g_aCharInfo.pop_back();
    StartTurnGage(&c,2);
    // 閉扉は歩ける画像でも遮蔽し、開扉は通す。途中と相手のマスの両方。
    EVENTINFO_DOOR door;door.wGrpID=0;door.wBackGrpID=1;
    EVENTINFO event;event.dwX=6;event.dwY=5;event.pData=&door;
    map.nEventCount=1;map.pEvent=&event;
    c.byTurnLV=80;TimerProcCharTurnLV(&c);
    assert(!IsBowAttackTarget(&c,&far) && !c.dwTimeTurnLV && c.byTurnLV==0);
    now+=1000;TimerProcCharTurnLV(&c);before=sends;
    assert(!c.dwTimeTurnLV && !c.byTurnLV && !ShotTurnGage(&c) && sends==before);
    door.wGrpID=2;TimerProcCharTurnLV(&c);now+=200;TimerProcCharTurnLV(&c);
    assert(c.byTurnLV>=TURNGAGE_LAP && IsBowAttackTarget(&c,&far));
    event.dwX=far.dwX;door.wGrpID=0;
    assert(!IsBowAttackTarget(&c,&far));
    map.nEventCount=0;
    // CtrlとZ/ESC（コマンド・相手選択を含む）は枠をローカルですぐ消す。
    for (int action=0;action<4;++action) {
        g_bBattleMode=true;StartTurnGage(&c,2);
        SAFE_DELETE(g_pTargetSelectInfo);g_pTargetSelectInfo=new TARGETSELECTINFO{2};
        int prev=unlocked;
        if(action==0) modeOff(&c);
        if(action==1) CancelTurnGage(&c);
        if(action==2) commandCancel();
        if(action==3) targetCancel();
        assert(!c.dwAtackTblID && !c.byTurnLV && !c.dwTimeTurnLV);
        assert(!g_pTargetSelectInfo && unlocked==prev+1);
    }
    // ロックが無い弓に表示用の古い値だけ残っても、タイマーで消す。
    c.byTurnLV=80;c.dwTimeTurnLV=now;TimerProcCharTurnLV(&c);
    assert(!c.byTurnLV && !c.dwTimeTurnLV);
    // 近接のキャンセルは従来どおりターゲット枠や送信には触れない。
    bowEquipped=false;StartTurnGage(&c,2);SAFE_DELETE(g_pTargetSelectInfo);g_pTargetSelectInfo=new TARGETSELECTINFO{2};
    int prev=unlocked;CancelTurnGage(&c);
    assert(!c.dwAtackTblID && !c.byTurnLV && !c.dwTimeTurnLV);
    assert(g_pTargetSelectInfo && unlocked==prev);SAFE_DELETE(g_pTargetSelectInfo);
    bowEquipped=true;
    c.byDirection=3;c.byDotMove=19;
    assert(BowOffsetX(&c)==19 && BowOffsetY(&c)==0);
    puts("bow client: 4 directions, straight interpolation, selected target, hit retarget/frame/charge preservation, charge reset/resume, early/just/continuous shots, Ctrl/cancel frame cleanup, closed/open doors, stale gauge cleanup, melee cancellation, invalid targets, range, legacy offsets OK");
}
'''
with tempfile.TemporaryDirectory(prefix='sbo-bow-client-') as temp:
    src=Path(temp)/'bow.cpp'; src.write_text(code)
    binary=Path(temp)/'bow'
    subprocess.run(['c++','-std=c++17','-O2','-I',str(root),str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
