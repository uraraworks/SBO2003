/* ========================================================================= */
/* test_main.cpp														     */
/* サーバーの性能改善用の回帰テスト（Node.js で動かす）						 */
/* ========================================================================= */
/*
 * 修正の前後で出力が完全に一致することを確かめるためのもの。
 *   - 時刻は固定（timeGetTime / GetTickCount を差し替え）、乱数は固定シード
 *   - 通信は mock_net.js が受け、各接続に送られたバイト列の長さ・回数・SHA-256 を出す
 * 出力のうち "BENCH" で始まる行は計測値なので、比較には使わない。
 *
 *   A. 送信（SendTo）            … 宛先・大きさ・優先度ごとに積まれるバイト列
 *   C. myArray                   … 乱数の操作列を std::vector と突き合わせる
 *   D. 経路探索（SEARCH2）       … 実データのマップで、探索の結果が変わらないこと
 *   B. 同画面内への配信          … SendCmdInside2 / SendCmdInsideMap
 *   E. キャラの巡回（TimerProc） … 実データ＋ログイン中のPCで、状態と送信が変わらないこと
 *   H. キャラ情報の項目消し     … 他人・NPC の情報から、画面に要らない項目が消えること（本人・管理者は全部）
 *   K. 傭兵                      … 道さがしの表が前の方法と同じ結果か・敵探しの速さ・道具を持たせる取り引き
 *   M. ギルドの依頼              … 受ける・見えるNPCが持ち主と仲間にだけ届く・宝箱・ポータル・渡して報酬・保存して読み直し
 *   N. 並べ替え                  … 道具欄・預かり所の入れ替え・種類順・50音順（装備の付け替え・取り引き中は動かない・50音キー）
 *   O. 接続ID検索                … 配列・ID変更後も先頭の一致を返すことと、多数キャラでの速さ
 */
#include "stdafx.h"
#include "../../common/DungeonReception.h"
#include "../../common/FallingPuzzle.h"
#include "../../common/MazeAttack.h"
#include "../../common/MazeTable.h"
#include "../../common/DungeonText.h"
#include <algorithm>
#pragma push_macro("max")
#undef max
#include "../../tools/dungeon/theme_checks.h"
#pragma pop_macro("max")
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH2.h"
#include "CharProcSEARCH.h"
#include "CharProcMOVERAND.h"
/* 傭兵の道さがしを中から確かめるため、protected も見えるようにして読み込む */
#define protected public
#include "CharProcMERCENARY.h"
#include "CharProcPUT.h"
#undef protected
#include "SboSvrSubGameProcSugoroku.h"
#include "SboSvrMagicProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrItemProc.h"
#include "SboSvrTalk.h"
#include "SboSvrMarket.h"
#include "SboSvrRecycle.h"
#include "SboSvrAffix.h"
#include "SboSvrDungeonDaily.h"
#include "common/crc.h"
#include "common/Info/InfoJob.h"
#include "common/Command/Command.h"
#include <emscripten.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <string>
#include <functional>
#include <set>
#include <vector>
#include <algorithm>

/* ------------------------------------------------------------------------- */
/* 固定の時刻																 */
/* ------------------------------------------------------------------------- */
static DWORD s_dwFakeNow = 1000000;
extern "C" DWORD __wrap__Z11timeGetTimev(void)		{ return s_dwFakeNow; }
extern "C" DWORD __wrap__Z12GetTickCountv(void)		{ return s_dwFakeNow; }

/* ------------------------------------------------------------------------- */
/* mock_net.js との連絡														 */
/* ------------------------------------------------------------------------- */
EM_JS(char *, MockReport, (int nConn), {
	return stringToNewUTF8(Module['sboNet'].report(nConn));
});
EM_JS(char *, MockReportAll, (void), {
	return stringToNewUTF8(Module['sboNet'].reportAll());
});
EM_JS(int, MockFirstWord, (int nConn), {
	var b = Module['sboNet'].firstPacket(nConn);
	return b ? b.readUInt32LE(8) : 0;
});
EM_JS(void, MockReset, (void), {
	Module['sboNet'].reset();
});
/* 送ったパケットを覚えるか（L で使う）。覚えた中から指定コマンドの byData を並べて返す */
EM_JS(void, MockTrace, (int bOn), {
	Module['sboNet'].trace(!!bOn);
});
EM_JS(char *, MockCmdData, (int nConn, int nCmd), {
	return stringToNewUTF8(Module['sboNet'].cmdData(nConn, nCmd));
});
EM_JS(double, NowMs, (void), {
	return performance.now();
});

extern "C" int SboSockOnAccept(int nConn, DWORD dwAddr);
extern "C" void SboSockOnData(int nNo, const BYTE *pData, int nSize);
extern "C" void SboSockOnClose(int nNo);

extern LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
extern void TimerProc(void);
extern BOOL SaveMapInfo(void);
extern void sgenrand(unsigned long);
extern void SendCmdInsideMap(DWORD, LPBYTE, DWORD);
extern void SvrSendCaptureBegin(void);
extern BOOL SvrSendCaptureEnd(std::vector<BYTE> &);

void SvrLogSettingWarning(int) {}

static void PrintAll(const char *pszLabel)
{
	char *p = MockReportAll();
	printf("%s %s\n", pszLabel, p);
	free(p);
}

static void PrintConn(const char *pszLabel, int nConn)
{
	char *p = MockReport(nConn);
	printf("%s conn=%d %s\n", pszLabel, nConn, p);
	free(p);
}

/* 乱数（テスト用。ゲーム側の genrand とは別） */
static DWORD s_dwSeed = 12345;
static DWORD Rnd(void)
{
	s_dwSeed = s_dwSeed * 1664525u + 1013904223u;
	return s_dwSeed >> 8;
}

static void MakePayload(std::vector<BYTE> &a, DWORD dwSize, int nKind)
{
	a.resize(dwSize ? dwSize : 1);
	for (DWORD i = 0; i < dwSize; i ++) {
		a[i] = (nKind == 0) ? (BYTE)((i / 5) & 0xFF) : (BYTE)(Rnd() & 0xFF);
	}
	if (dwSize) {
		a[0] = 0x31;		/* 先頭が 0xFF だと圧縮済みと紛らわしいので避ける */
	}
}

/* 接続して接続前チェックを済ませる。スロット番号を返す */
static int Connect(int nConn, BOOL bPreCheck)
{
	int nNo = SboSockOnAccept(nConn, 0x0100007F + nConn);
	if (nNo < 0 || !bPreCheck) {
		return nNo;
	}
	DWORD dwKey = (DWORD)MockFirstWord(nConn);
	DWORD dwReply = (dwKey & URARASOCK_PRECHECK) * URARASOCK_PRECHECK;
	CCRC Crc;
	URARASOCK_PACKETINFO Info;
	Info.dwSize = sizeof(DWORD);
	Info.dwCRC = Crc.GetCRC((PBYTE)&dwReply, sizeof(DWORD));
	BYTE abyBuf[sizeof(Info) + sizeof(DWORD)];
	memcpy(abyBuf, &Info, sizeof(Info));
	memcpy(abyBuf + sizeof(Info), &dwReply, sizeof(DWORD));
	SboSockOnData(nNo, abyBuf, sizeof(abyBuf));
	return nNo;
}

/* ========================================================================= */
/* 関数名：	TestZlib													 */
/* 内容：	パケットごとの初期化と再利用のバイト列・展開結果・時間を比較	 */
/* ========================================================================= */
static void TestZlib(void)
{
	const DWORD aSize[] = {256, 4096, 32768};
	const int NRUN = 1000;
	for (int k = 0; k < 3; k ++) {
		DWORD dwSize = aSize[k];
		std::vector<BYTE> aSrc(dwSize), aOld(dwSize), aNew(dwSize), aOut(dwSize);
		/* ゲーム側・既存テストの乱数を消費しない */
		for (DWORD i = 0; i < dwSize; i ++) aSrc[i] = (BYTE)(i / 7 + i / 251);
		CmyZlib Fresh, Reuse, Inflate;
		DWORD dwOld = 0, dwNew = 0;
		double d0 = NowMs();
		for (int i = 0; i < NRUN; i ++) {
			if (!Fresh.DeflateInit()) abort();
			dwOld = Fresh.Deflate(&aSrc[0], dwSize, &aOld[0], dwSize);
			Fresh.DeflateEnd();
		}
		double d1 = NowMs();
		if (!Reuse.DeflateInit() || !Inflate.InflateInit()) abort();
		for (int i = 0; i < NRUN; i ++) {
			dwNew = Reuse.Deflate(&aSrc[0], dwSize, &aNew[0], dwSize);
		}
		double d2 = NowMs();
		if (!dwNew || dwNew != dwOld || memcmp(&aOld[0], &aNew[0], dwOld)) {
			fprintf(stderr, "zlib 圧縮結果が不一致\n"); abort();
		}
		/* 連続展開・破損データ・出力不足の後も独立したパケットとして扱う */
		for (int i = 0; i < 6; i ++) {
			if (Inflate.Inflate(&aNew[0], dwNew, &aOut[0], dwSize) != dwSize || memcmp(&aSrc[0], &aOut[0], dwSize)) abort();
			if (Inflate.Inflate(&aNew[0], dwNew, &aOut[0], 1) != 0) abort();
			aNew[0] ^= 0xFF;
			if (Inflate.Inflate(&aNew[0], dwNew, &aOut[0], dwSize) != 0) abort();
			aNew[0] ^= 0xFF;
		}
		Reuse.DeflateEnd();
		Inflate.InflateEnd();
		printf("BENCH zlib bytes=%u runs=%d old_ms=%.1f reuse_ms=%.1f same=1\n", dwSize, NRUN, d1 - d0, d2 - d1);
	}
}

/* ========================================================================= */
/* 関数名：	TestZlibPackets												 */
/* 内容：	送受信の交互実行・展開失敗後の復旧・Destroy後の再開を確認	 */
/* ========================================================================= */
static void TestZlibPackets(void)
{
	extern DWORD SvrTakeRecvSize(const BYTE *pData);
	WNDCLASS wc = {};
	wc.lpszClassName = "SBOZLIBTEST";
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	RegisterClass(&wc);
	HWND hWnd = CreateWindow(wc.lpszClassName, "t", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
	if (!hWnd) abort();
	CUraraSockTCP Sock;
	CCRC Crc;
	for (int round = 0; round < 2; round ++) {
		if (!Sock.Host(hWnd, 2004, 2)) abort();
		MockReset();
		int anSlot[] = {Connect(1, TRUE), Connect(2, TRUE)};
		for (int k = 0; k < 8; k ++) {
			DWORD dwSize = k % 2 ? 32768 : 256;
			std::vector<BYTE> aSrc(dwSize), aBody(dwSize + 5);
			for (DWORD i = 0; i < dwSize; i ++) aSrc[i] = (BYTE)(i / 5 + k);
			CmyZlib Zlib;
			if (!Zlib.DeflateInit()) abort();
			DWORD dwZipped = Zlib.Deflate(&aSrc[0], dwSize, &aBody[5], dwSize);
			Zlib.DeflateEnd();
			if (!dwZipped) abort();
			aBody.resize(dwZipped + 5);
			aBody[0] = 0xFF;
			DWORD dwOut = k == 4 ? 1 : dwSize;
			memcpy(&aBody[1], &dwOut, 4);
			if (k == 2) aBody[5] ^= 0xFF;
			URARASOCK_PACKETINFO Info;
			Info.dwSize = (DWORD)aBody.size();
			Info.dwCRC = Crc.GetCRC(&aBody[0], Info.dwSize);
			std::vector<BYTE> aPacket(sizeof(Info) + Info.dwSize);
			memcpy(&aPacket[0], &Info, sizeof(Info));
			memcpy(&aPacket[sizeof(Info)], &aBody[0], Info.dwSize);
			Sock.SendTo(URARASOCK_IDBASE + anSlot[k % 2], &aSrc[0], dwSize);
			SboSockOnData(anSlot[k % 2], &aPacket[0], (int)aPacket.size());
			MSG Msg;
			int nRecv = 0;
			while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
				if (Msg.message != WM_URARASOCK_RECV) continue;
				PBYTE pData = (PBYTE)Msg.wParam;
				DWORD dwRecv = SvrTakeRecvSize(pData);
				if (dwRecv != dwSize || memcmp(pData, &aSrc[0], dwSize)) abort();
				delete [] pData;
				nRecv ++;
			}
			if (nRecv != ((k == 2 || k == 4) ? 0 : 1)) {
				fprintf(stderr, "zlib パケットの展開結果が不一致 round=%d k=%d\n", round, k); abort();
			}
		}
		Sock.Destroy();
	}
	DestroyWindow(hWnd);
	printf("BENCH zlib packets rounds=2 checks=16 same=1\n");
}

/* 変更前の CanStep を残して、返り値を照合する（狙いを変えないことも確かめる） */
class CTestSearch : public CCharProcSEARCH
{
public:
	using CCharProcSEARCH::CanStep;
	BOOL OldCanStep(PCHARINFO pCharTmp, int x, int y)
	{
		PMAPINFO pMap = g_aMapInfo[m_pChar->dwMapID];
		if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) return FALSE;
		if (!NpcProcSEARCH_Check(m_pChar, pCharTmp, x, y)) return FALSE;
		if (CheckCharBlockPC(m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) return FALSE;
		if (CheckCharBlockNPC2(m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) return FALSE;
		return TRUE;
	}
};

/* ========================================================================= */
/* 関数名：	TestSearchStep												 */
/* 内容：	独立したマップで旧判定と比較（壁・イベント・PC・NPC・範囲外） */
/* ========================================================================= */
static void TestSearchStep(void)
{
	const int NPC = 128, NRUN = 100000;
	MAPINFO Map = {};
	WORD awMap[16 * 16];
	DWORD adwPC[NPC], adwNPC[2];
	CHARINFO *aChar = new CHARINFO[NPC + 2]();
	EVENTINFO Event = {};
	int nMap = g_aMapInfo.GetSize(), nChar = g_aCharInfo.GetSize();
	WORD wFloor = 0, wWall = 0;
	for (int i = 0; i < g_wCountMapParts; i ++) {
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) wFloor = (WORD)i;
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) wWall = (WORD)i;
	}
	std::fill(awMap, awMap + 16 * 16, wFloor);
	awMap[4 + 5 * 16] = wWall;
	Map.dwID = nMap;
	Map.nCX = Map.nCY = 16;
	Map.pwMap = awMap;
	Map.paTblNoPC = adwPC;
	Map.dwCountTblNoPC = NPC;
	Map.paTblNoNPC = adwNPC;
	Map.dwCountTblNoNPC = 2;
	Event.dwX = 6; Event.dwY = 4;
	Map.pEvent = &Event; Map.nEventCount = 1;
	g_aMapInfo.Add(&Map);
	for (int i = 0; i < NPC + 2; i ++) {
		PCHARINFO p = &aChar[i];
		p->dwTblID = nChar + i; p->dwMapID = nMap;
		p->dwX = 10 + i % 6; p->dwY = 10 + i / 6 % 6;
		p->wHP = 10; p->byGameState = GAMESTATE_MOVE;
		p->wGrpMain = CHARGRP_MAIN_NORMAL;
		g_aCharInfo.Add(p);
		if (i < NPC) adwPC[i] = p->dwTblID;
		else adwNPC[i - NPC] = p->dwTblID;
	}
	PCHARINFO pNpc = &aChar[NPC];
	pNpc->dwX = 4; pNpc->dwY = 4;
	aChar[NPC + 1].dwX = 7; aChar[NPC + 1].dwY = 4;
	aChar[NPC + 1].byNpcType = NPCTYPE_SEARCH;
	CTestSearch Proc;
	Proc.Create(pNpc);
	int nCases = 0;
	for (int mode = 0; mode < 5; mode ++) {
		aChar[0].dwX = 5; aChar[0].dwY = 4;
		aChar[0].wHP = mode == 1 ? 0 : 10;
		aChar[0].byGameState = mode == 2 ? GAMESTATE_LOGOFF : GAMESTATE_MOVE;
		aChar[0].wGrpMain = mode == 3 ? CHARGRP_MAIN_4848 : CHARGRP_MAIN_NORMAL;
		/* PCとイベントが重なる場合も、ターゲット変更を先に行う */
		if (mode == 4) aChar[0].dwX = 6;
		for (int type = 0; type < 2; type ++) {
			pNpc->byNpcType = type ? NPCTYPE_BATTLE2 : NPCTYPE_SEARCH;
			for (int y = -1; y <= 16; y ++) for (int x = -1; x <= 16; x ++) {
				pNpc->dwTargetTblID = (DWORD)-1;
				BOOL bOld = Proc.OldCanStep(&aChar[0], x, y);
				CHARINFO Old = *pNpc;
				/* 2026/11/12 から CanStep は調べるだけで狙いを変えない（変えるのは追いかけている時の StepToward） */
				Old.dwTargetTblID = (DWORD)-1;
				pNpc->dwTargetTblID = (DWORD)-1;
				BOOL bNew = Proc.CanStep(&aChar[0], x, y);
				if (bOld != bNew || memcmp(&Old, pNpc, sizeof(Old))) {
					fprintf(stderr, "SEARCH 判定が不一致 mode=%d type=%d (%d,%d)\n", mode, type, x, y); abort();
				}
				nCases ++;
			}
		}
	}
	/* PCに当たらないマスでは全PCを二度走査していた */
	aChar[0].dwX = 10; aChar[0].dwY = 10;
	int nOld = 0, nNew = 0;
	double d0 = NowMs();
	for (int i = 0; i < NRUN; i ++) nOld += Proc.OldCanStep(&aChar[0], 5, 4);
	double d1 = NowMs();
	for (int i = 0; i < NRUN; i ++) nNew += Proc.CanStep(&aChar[0], 5, 4);
	double d2 = NowMs();
	if (nOld != NRUN || nNew != nOld) abort();
	printf("BENCH search step pcs=%d runs=%d old_ms=%.1f new_ms=%.1f cases=%d same=1\n", NPC, NRUN, d1 - d0, d2 - d1, nCases);
	g_aCharInfo.RemoveAt(nChar, NPC + 2);
	g_aMapInfo.RemoveAt(nMap);
	delete [] aChar;
}

/* 敵の動き（2026/11/12 改善）の中身を外から呼べるようにする */
class CTestSearchAI : public CCharProcSEARCH
{
public:
	using CCharProcSEARCH::FindPathStep;
	using CCharProcSEARCH::IsInSight;
	using CCharProcSEARCH::IsOverLeash;
};

/* ========================================================================= */
/* 関数名：	TestEnemyAI													 */
/* 内容：	Q. 敵の動き：壁越しに見えない・回り込む・離れすぎ・戦闘１が戦い続ける */
/* ========================================================================= */
static void TestEnemyAI(void)
{
	printf("== Q. EnemyAI ==\n");
	MAPINFO Map = {};
	WORD awMap[16 * 16];
	DWORD adwPC[1], adwNPC[2];
	CHARINFO *aChar = new CHARINFO[3]();
	int nMap = g_aMapInfo.GetSize(), nChar = g_aCharInfo.GetSize();
	WORD wFloor = 0, wWall = 0;
	for (int i = 0; i < g_wCountMapParts; i ++) {
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) wFloor = (WORD)i;
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) wWall = (WORD)i;
	}
	std::fill(awMap, awMap + 16 * 16, wFloor);
	/* x=6 の y=3〜9 が壁 */
	for (int y = 3; y <= 9; y ++) awMap[6 + y * 16] = wWall;
	Map.dwID = nMap;
	Map.nCX = Map.nCY = 16;
	Map.pwMap = awMap;
	Map.paTblNoPC = adwPC;
	Map.dwCountTblNoPC = 1;
	Map.paTblNoNPC = adwNPC;
	Map.dwCountTblNoNPC = 2;
	g_aMapInfo.Add(&Map);
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = &aChar[i];
		p->dwTblID = nChar + i; p->dwMapID = nMap;
		p->wHP = p->wMaxHP = 10; p->byGameState = GAMESTATE_MOVE;
		p->wGrpMain = CHARGRP_MAIN_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		g_aCharInfo.Add(p);
	}
	adwPC[0] = aChar[0].dwTblID;
	adwNPC[0] = aChar[1].dwTblID;
	adwNPC[1] = aChar[2].dwTblID;
	PCHARINFO pPC = &aChar[0], pEnemy = &aChar[1], pOther = &aChar[2];
	pOther->dwX = 15; pOther->dwY = 15;

	pEnemy->byNpcType = NPCTYPE_BATTLE2;
	pEnemy->dwX = pEnemy->dwStartX = 4; pEnemy->dwY = pEnemy->dwStartY = 6;
	pEnemy->bySearchCx = pEnemy->bySearchCy = 4;
	CTestSearchAI Proc;
	Proc.Create(pEnemy);

	/* 壁の向こうは見えない・壁が無ければ見える */
	pPC->dwX = 8; pPC->dwY = 6;
	BOOL bWall = Proc.IsInSight(pPC);
	pPC->dwX = 4; pPC->dwY = 1;
	BOOL bOpen = Proc.IsInSight(pPC);
	printf("sight wall=%d open=%d\n", bWall, bOpen);
	if (bWall || !bOpen) abort();

	/* 壁の向こうの相手へは、壁の端（上）へ回り込む */
	pPC->dwX = 8; pPC->dwY = 6;
	int nDir = Proc.FindPathStep(pPC, 8, 6);
	/* 回り込む道を最後までたどれるか（1歩ずつ進めて相手のとなりに着く） */
	int nSteps = 0;
	while (nDir >= 0 && nSteps < 40) {
		pEnemy->dwX += anPosAddTblX[nDir];
		pEnemy->dwY += anPosAddTblY[nDir];
		nSteps ++;
		if (abs((int)pEnemy->dwX - 8) + abs((int)pEnemy->dwY - 6) == 1) break;
		nDir = Proc.FindPathStep(pPC, 8, 6);
	}
	printf("path steps=%d at=%u,%u\n", nSteps, pEnemy->dwX, pEnemy->dwY);
	if (abs((int)pEnemy->dwX - 8) + abs((int)pEnemy->dwY - 6) != 1) abort();

	/* 開始位置から 探索範囲×2＋4 を超えると離れすぎ */
	pEnemy->dwX = 4 + 12; pEnemy->dwY = 6;
	BOOL bNear = Proc.IsOverLeash();
	pEnemy->dwX = 4 + 13; pEnemy->dwY = 6;
	BOOL bFar = Proc.IsOverLeash();
	printf("leash near=%d far=%d\n", bNear, bFar);
	if (bNear || !bFar) abort();

	/* 戦闘１：戦っている相手がとなりにいる間は、20秒たっても戦いをやめず、相手の方を向く */
	CCharProcMOVERAND Proc1;
	pEnemy->byNpcType = NPCTYPE_BATTLE1;
	pEnemy->dwX = 10; pEnemy->dwY = pEnemy->dwStartY = 12;
	pEnemy->dwStartX = 2;	/* 開始位置から離れた所で戦っている */
	pEnemy->bySearchCx = pEnemy->bySearchCy = 3;
	pEnemy->byDirection = 0;
	pEnemy->byAreaX = pEnemy->byAreaY = 0;
	pPC->dwX = 11; pPC->dwY = 12;
	pEnemy->dwTargetTblID = pPC->dwTblID;
	pEnemy->dwLastBattle = 5;
	pEnemy->dwLastDotMove = s_dwFakeNow - 60000;
	Proc1.Create(pEnemy);
	DWORD dwStart = s_dwFakeNow;
	for (int t = 0; t <= 20000; t += 500) {
		Proc1.Proc(dwStart + t);
	}
	printf("battle1 keep=%d dir=%d pos=%u,%u\n", pEnemy->dwLastBattle != 0, pEnemy->byDirection, pEnemy->dwX, pEnemy->dwY);
	if (pEnemy->dwLastBattle == 0 || pEnemy->byDirection != 3 || pEnemy->dwX != 10) abort();
	/* 相手が遠くへ行って15秒たったらやめる */
	pPC->dwX = 0; pPC->dwY = 0;
	DWORD dwFar = dwStart + 20500;
	for (int t = 0; t <= 16000; t += 500) {
		Proc1.Proc(dwFar + t);
	}
	printf("battle1 release=%d target=%d\n", pEnemy->dwLastBattle == 0, pEnemy->dwTargetTblID == (DWORD)-1);
	if (pEnemy->dwLastBattle != 0 || pEnemy->dwTargetTblID != (DWORD)-1) abort();

	g_aCharInfo.RemoveAt(nChar, 3);
	g_aMapInfo.RemoveAt(nMap);
	delete [] aChar;
}

/* 戦闘２の回り込み・狙いを変える（2026/11/13）の中身を外から呼べるようにする */
class CTestSearchAI2 : public CCharProcSEARCH
{
public:
	using CCharProcSEARCH::ProcTarget;
};

/* ========================================================================= */
/* 関数名：	TestEnemyAI2													 */
/* 内容：	V. 戦闘２：L字の壁を回り込む・となりの人へ狙いを変える・囲まれたら相手を変える */
/* ========================================================================= */
static void TestEnemyAI2(void)
{
	printf("== V. EnemyAI2 ==\n");
	MAPINFO Map = {};
	WORD awMap[16 * 16];
	DWORD adwPC[4], adwNPC[1];
	CHARINFO *aChar = new CHARINFO[5]();
	int nMap = g_aMapInfo.GetSize(), nChar = g_aCharInfo.GetSize();
	WORD wFloor = 0, wWall = 0;
	for (int i = 0; i < g_wCountMapParts; i ++) {
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) wFloor = (WORD)i;
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) wWall = (WORD)i;
	}
	std::fill(awMap, awMap + 16 * 16, wFloor);
	/* L字の壁：x=6 の y=3〜9 と、y=9 の x=2〜6 */
	for (int y = 3; y <= 9; y ++) awMap[6 + y * 16] = wWall;
	for (int x = 2; x <= 6; x ++) awMap[x + 9 * 16] = wWall;
	Map.dwID = nMap;
	Map.nCX = Map.nCY = 16;
	Map.pwMap = awMap;
	Map.paTblNoPC = adwPC;
	Map.dwCountTblNoPC = 1;
	Map.paTblNoNPC = adwNPC;
	Map.dwCountTblNoNPC = 1;
	g_aMapInfo.Add(&Map);
	for (int i = 0; i < 5; i ++) {
		PCHARINFO p = &aChar[i];
		p->dwTblID = nChar + i; p->dwMapID = nMap;
		p->wHP = p->wMaxHP = 10; p->byGameState = GAMESTATE_MOVE;
		p->wGrpMain = CHARGRP_MAIN_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->dwX = p->dwY = 15;
		g_aCharInfo.Add(p);
	}
	PCHARINFO pEnemy = &aChar[0];
	PCHARINFO apPC[4] = { &aChar[1], &aChar[2], &aChar[3], &aChar[4] };
	for (int i = 0; i < 4; i ++) adwPC[i] = apPC[i]->dwTblID;
	adwNPC[0] = pEnemy->dwTblID;

	pEnemy->byNpcType = NPCTYPE_BATTLE2;
	pEnemy->dwX = pEnemy->dwStartX = 4; pEnemy->dwY = pEnemy->dwStartY = 6;
	pEnemy->bySearchCx = pEnemy->bySearchCy = 5;
	pEnemy->byMoveProbabilityX = pEnemy->byMoveProbabilityY = 100;
	CTestSearchAI2 Proc;
	Proc.Create(pEnemy);

	/* L字の壁の内側の角から、壁の向こうの相手へ回り込んでとなりに着く（上下に行ったり来たりしない） */
	apPC[0]->dwX = 8; apPC[0]->dwY = 7;
	pEnemy->dwTargetTblID = apPC[0]->dwTblID;
	pEnemy->dwLastBattle = 1;
	DWORD dwTime = s_dwFakeNow;
	int nSteps = 0, nBack = 0;
	BYTE byLastDir = 0xFF;
	for (int i = 0; i < 40; i ++) {
		dwTime += 500;
		g_dwTime = dwTime;
		DWORD dwX = pEnemy->dwX, dwY = pEnemy->dwY;
		Proc.ProcTarget(dwTime);
		if ((dwX != pEnemy->dwX) || (dwY != pEnemy->dwY)) {
			/* 前の1歩と逆向きに戻った回数 */
			if ((byLastDir != 0xFF) && (pEnemy->byDirection == abyDirectionTbl[byLastDir])) nBack ++;
			byLastDir = pEnemy->byDirection;
			nSteps ++;
		}
		if (abs((int)pEnemy->dwX - 8) + abs((int)pEnemy->dwY - 7) == 1) break;
	}
	printf("lwall steps=%d back=%d at=%u,%u\n", nSteps, nBack, pEnemy->dwX, pEnemy->dwY);
	if ((abs((int)pEnemy->dwX - 8) + abs((int)pEnemy->dwY - 7) != 1) || nBack) abort();

	/* 狙いの相手は壁の向こう、別の人がとなりにいる → となりの人へ狙いを変えて向く */
	Map.dwCountTblNoPC = 2;
	pEnemy->dwX = 4; pEnemy->dwY = 6; pEnemy->byDirection = 0;
	apPC[0]->dwX = 8; apPC[0]->dwY = 7;
	apPC[1]->dwX = 3; apPC[1]->dwY = 6;
	pEnemy->dwTargetTblID = apPC[0]->dwTblID;
	dwTime += 500;
	Proc.ProcTarget(dwTime);
	printf("nextto target=%d dir=%d\n", pEnemy->dwTargetTblID == apPC[1]->dwTblID, pEnemy->byDirection);
	if ((pEnemy->dwTargetTblID != apPC[1]->dwTblID) || (pEnemy->byDirection != 2)) abort();
	/* 倒れている人には変えない */
	apPC[1]->wHP = 0;
	pEnemy->dwTargetTblID = apPC[0]->dwTblID;
	dwTime += 500;
	Proc.ProcTarget(dwTime);
	printf("nextto dead keep=%d\n", pEnemy->dwTargetTblID == apPC[0]->dwTblID);
	if (pEnemy->dwTargetTblID != apPC[0]->dwTblID) abort();
	apPC[1]->wHP = 10;

	/* 一人だけがとなりなら、行動しても相手は変えない */
	Map.dwCountTblNoPC = 1;
	pEnemy->dwX = 10; pEnemy->dwY = 12;
	apPC[0]->dwX = 10; apPC[0]->dwY = 11;
	pEnemy->dwTargetTblID = apPC[0]->dwTblID;
	pEnemy->dwLastBattle = 100;
	int nChange = 0;
	for (int i = 0; i < 20; i ++) {
		dwTime += 500;
		pEnemy->dwLastBattle += 1000;	/* 毎回行動した */
		Proc.ProcTarget(dwTime);
		if (pEnemy->dwTargetTblID != apPC[0]->dwTblID) nChange ++;
	}
	printf("alone change=%d\n", nChange);
	if (nChange) abort();

	/* 4方向から囲まれたら、行動するたびにときどきとなりの別の人へ変える */
	Map.dwCountTblNoPC = 4;
	apPC[1]->dwX = 10; apPC[1]->dwY = 13;
	apPC[2]->dwX = 9; apPC[2]->dwY = 12;
	apPC[3]->dwX = 11; apPC[3]->dwY = 12;
	int anHit[4] = {};
	nChange = 0;
	for (int i = 0; i < 40; i ++) {
		DWORD dwOld = pEnemy->dwTargetTblID;
		dwTime += 500;
		pEnemy->dwLastBattle += 1000;
		Proc.ProcTarget(dwTime);
		if (pEnemy->dwTargetTblID != dwOld) nChange ++;
		for (int k = 0; k < 4; k ++) {
			if (pEnemy->dwTargetTblID == apPC[k]->dwTblID) {
				anHit[k] ++;
				/* 狙いの相手の方を向いている */
				int nDir = -1;
				if (apPC[k]->dwY < pEnemy->dwY) nDir = 0; else if (apPC[k]->dwY > pEnemy->dwY) nDir = 1;
				else if (apPC[k]->dwX < pEnemy->dwX) nDir = 2; else nDir = 3;
				if (pEnemy->byDirection != nDir) abort();
			}
		}
	}
	printf("surround change=%d hit=%d,%d,%d,%d\n", nChange, anHit[0], anHit[1], anHit[2], anHit[3]);
	if ((nChange < 5) || !anHit[0] || !anHit[1] || !anHit[2] || !anHit[3]) abort();
	/* 行動していない間（ゲージが溜まるまで）は変えない */
	DWORD dwKeep = pEnemy->dwTargetTblID;
	for (int i = 0; i < 10; i ++) {
		dwTime += 500;
		Proc.ProcTarget(dwTime);
		if (pEnemy->dwTargetTblID != dwKeep) abort();
	}

	g_aCharInfo.RemoveAt(nChar, 5);
	g_aMapInfo.RemoveAt(nMap);
	delete [] aChar;
}

/* ------------------------------------------------------------------------- */
/* A. 送信																	 */
/* ------------------------------------------------------------------------- */
static void TestSend(void)
{
	printf("== A. SendTo ==\n");
	CUraraSockTCP Sock;
	const int NSLOT = 12;
	Sock.Host((HWND)1, 2004, NSLOT);
	for (int i = 0; i < NSLOT; i ++) {
		Connect(i + 1, i < 10);			/* 10, 11 番は接続前チェック前 */
	}
	SboSockOnClose(3);					/* 3 番は切断済み */
	MockReset();

	static const DWORD aSize[] = {0, 1, 7, 127, 128, 129, 300, 1000, 1000, 5000, 70000};
	static const int aKind[]   = {1, 1, 1, 1,   1,   1,   0,   0,    1,    1,    0};
	DWORD adwID[] = {
		URARASOCK_IDBASE + 0, URARASOCK_IDBASE + 3, URARASOCK_IDBASE + 10, URARASOCK_IDBASE + 11,
		URARASOCK_IDBASE + 12, URARASOCK_IDBASE + 999, 5, 0, URARASOCK_IDBASE - 1,
		URARASOCK_IDBASE + 7, URARASOCK_SENDALL
	};
	int nCall = 0;
	for (size_t s = 0; s < sizeof(aSize) / sizeof(aSize[0]); s ++) {
		std::vector<BYTE> aData;
		MakePayload(aData, aSize[s], aKind[s]);
		for (size_t t = 0; t < sizeof(adwID) / sizeof(adwID[0]); t ++) {
			for (int nPri = 0; nPri < 2; nPri ++) {
				Sock.SendTo(adwID[t], &aData[0], aSize[s], (DWORD)0,
					nPri ? (BYTE)URARASOCK_SENDPRIORITY_HIGH : (BYTE)URARASOCK_SENDPRIORITY_MIDDLE);
				char szLabel[96];
				sprintf(szLabel, "send#%d size=%u id=%u pri=%d", nCall ++, (unsigned)aSize[s], (unsigned)adwID[t], nPri);
				PrintAll(szLabel);
			}
		}
	}
	/* 引数違いの SendTo と NULL */
	{
		std::vector<BYTE> aData;
		MakePayload(aData, 200, 0);
		Sock.SendTo(URARASOCK_IDBASE + 1, &aData[0], 200);
		Sock.SendTo(URARASOCK_IDBASE + 1, &aData[0], 200, (BYTE)URARASOCK_SENDPRIORITY_HIGH);
		Sock.SendTo(URARASOCK_IDBASE + 1, &aData[0], 200, (DWORD)5);
		Sock.SendTo(URARASOCK_SENDALL, NULL, 200);
		PrintAll("send-overloads");
	}
	/* 横取り */
	{
		std::vector<BYTE> aData;
		MakePayload(aData, 500, 1);
		SvrSendCaptureBegin();
		Sock.SendTo(URARASOCK_SENDALL, &aData[0], 500);
		Sock.SendTo(URARASOCK_SENDALL, &aData[0], 300);
		std::vector<BYTE> aCap;
		BOOL bRet = SvrSendCaptureEnd(aCap);
		printf("capture ret=%d size=%u same=%d\n", bRet, (unsigned)aCap.size(),
			(int)(aCap.size() == 500 && memcmp(&aCap[0], &aData[0], 500) == 0));
		PrintAll("after-capture");
	}
	for (int i = 1; i <= NSLOT; i ++) {
		PrintConn("final", i);
	}
	Sock.Destroy();
}

/* ------------------------------------------------------------------------- */
/* C. myArray																 */
/* ------------------------------------------------------------------------- */
struct ARRELEM {
	BYTE a;
	BYTE b;
	WORD c;
	DWORD d;
};

static void TestMyArray(void)
{
	printf("== C. myArray ==\n");
	s_dwSeed = 777;
	double dStart = NowMs();
	for (int nRound = 0; nRound < 20; nRound ++) {
		CmyArray<ARRELEM, ARRELEM> a;
		CmyArray<DWORD, DWORD> aD;
		std::vector<ARRELEM> ref;
		std::vector<DWORD> refD;
		int nOps = 50 + nRound * 40;
		DWORD dwHash = 2166136261u;
		for (int k = 0; k < nOps; k ++) {
			DWORD r = Rnd() % 100;
			ARRELEM e;
			e.a = (BYTE)Rnd(); e.b = (BYTE)Rnd(); e.c = (WORD)Rnd(); e.d = Rnd();
			int nSize = (int)ref.size();
			if (r < 45) {
				int n = a.Add(e);
				ref.push_back(e);
				aD.Add(e.d);
				refD.push_back(e.d);
				dwHash = (dwHash ^ (DWORD)n) * 16777619u;
			} else if (r < 60) {
				int n = nSize ? (int)(Rnd() % (nSize + 1)) : 0;
				a.InsertAt(n, e);
				ref.insert(ref.begin() + n, e);
				aD.InsertAt(n, e.d);
				refD.insert(refD.begin() + n, e.d);
			} else if (r < 75) {
				if (nSize) {
					int n = (int)(Rnd() % nSize);
					int c = 1 + (int)(Rnd() % 3);
					a.RemoveAt(n, c);
					aD.RemoveAt(n, c);
					for (int j = 0; j < c && n < (int)ref.size(); j ++) {
						ref.erase(ref.begin() + n);
						refD.erase(refD.begin() + n);
					}
				}
			} else if (r < 85) {
				if (nSize) {
					int n = (int)(Rnd() % nSize);
					a.SetAt(n, e);
					ref[n] = e;
					a[n].a ^= 0x55;
					ref[n].a ^= 0x55;
				}
				/* 範囲外の SetAt / GetAt */
				a.SetAt(nSize + 3, e);
				ARRELEM g = a.GetAt(nSize + 2);
				dwHash = (dwHash ^ g.d ^ g.a) * 16777619u;
			} else if (r < 90) {
				CmyArray<ARRELEM, ARRELEM> b;
				b.Copy(&a);
				a.RemoveAll();
				a.Copy(&b);
			} else if (r < 92) {
				a.RemoveAll();
				aD.RemoveAll();
				ref.clear();
				refD.clear();
			} else {
				if (nSize) {
					int n = (int)(Rnd() % nSize);
					ARRELEM g = a.GetAt(n);
					dwHash = (dwHash ^ g.d ^ g.c) * 16777619u;
				}
			}
			if (a.GetSize() != (int)ref.size() || aD.GetSize() != (int)refD.size()) {
				printf("MISMATCH size round=%d op=%d\n", nRound, k);
				return;
			}
		}
		int nBad = 0;
		for (int i = 0; i < (int)ref.size(); i ++) {
			ARRELEM g = a[i];
			if (memcmp(&g, &ref[i], sizeof(ARRELEM)) != 0 || aD[i] != refD[i]) {
				nBad ++;
			}
			dwHash = (dwHash ^ g.d) * 16777619u;
		}
		printf("array round=%d size=%d bad=%d hash=%08x\n", nRound, a.GetSize(), nBad, dwHash);
	}
	/* 追加だけを大量に */
	{
		CmyArray<ARRELEM, ARRELEM> a;
		ARRELEM e = {1, 2, 3, 4};
		DWORD dwSum = 0;
		for (int i = 0; i < 20000; i ++) {
			e.d = i;
			a.Add(e);
		}
		for (int i = 0; i < 20000; i ++) {
			dwSum += a[i].d;
		}
		printf("array bulk size=%d sum=%u\n", a.GetSize(), dwSum);
	}
	printf("BENCH myarray ms=%.1f\n", NowMs() - dStart);
}

/* ------------------------------------------------------------------------- */
/* D. 経路探索（SEARCH2）													 */
/* ------------------------------------------------------------------------- */
class CTestSearch2 : public CCharProcSEARCH2
{
public:
	/* 実際の処理（ProcSEARCH / ProcSEARCHMOVE）と同じ手順で探して経路を取り出す */
	void Run(PCHARINFO pChar, int nEndX, int nEndY, std::string &strOut)
	{
		Create(pChar);
		Init();
		m_ptEnd.x = nEndX;
		m_ptEnd.y = nEndY;
		m_ptNow = m_ptStart;
		int nTurn = 0;
		DWORD dwHash = 2166136261u;
		for (;;) {
			Search(FALSE);
			int nRet = SearchMove();
			nTurn ++;
			if (nRet == -1) {
				InfoCleanup(m_ptEnd.x, m_ptEnd.y);
				break;
			}
			if (nRet == 0 || nTurn > 100000) {
				break;
			}
		}
		int nCount = m_aSearchInfo.GetSize();
		for (int i = 0; i < nCount; i ++) {
			SEARCH2INFO &I = m_aSearchInfo[i];
			dwHash = (dwHash ^ I.x) * 16777619u;
			dwHash = (dwHash ^ I.y) * 16777619u;
			dwHash = (dwHash ^ I.byDirection) * 16777619u;
			dwHash = (dwHash ^ I.byState) * 16777619u;
			dwHash = (dwHash ^ I.wScore) * 16777619u;
			dwHash = (dwHash ^ I.wTurn) * 16777619u;
		}
		DWORD dwSize = (m_sizeArea.cx * 2 + 1) * (m_sizeArea.cy * 2 + 1);
		for (DWORD i = 0; i < dwSize; i ++) {
			dwHash = (dwHash ^ m_pwSearchMap[i]) * 16777619u;
		}
		char sz[128];
		sprintf(sz, "turns=%d infos=%d dir=%d hash=%08x", nTurn, nCount, m_nSearchDirection, dwHash);
		strOut = sz;
	}
};

static void TestSearch2(void)
{
	printf("== D. SEARCH2 ==\n");
	s_dwSeed = 4242;
	int nMaps = g_aMapInfo.GetSize();
	double dStart = NowMs();
	int nRuns = 0;
	/* 同じ作業領域で探索を繰り返し、マップ変更時の初期化も確認 */
	CTestSearch2 Proc;
	for (int m = 0; m < nMaps; m ++) {
		PMAPINFO pMap = g_aMapInfo[m];
		if (pMap->nCX < 4 || pMap->nCY < 4) {
			continue;
		}
		for (int k = 0; k < 6; k ++) {
			CHARINFO Char;
			ZeroMemory(&Char, sizeof(Char));
			Char.dwMapID = m;
			int nSx = 1 + (int)(Rnd() % (pMap->nCX - 2));
			int nSy = 1 + (int)(Rnd() % (pMap->nCY - 2));
			int nEx = 1 + (int)(Rnd() % (pMap->nCX - 2));
			int nEy = 1 + (int)(Rnd() % (pMap->nCY - 2));
			Char.dwX = nSx;
			Char.dwY = nSy;
			Char.byDirection = (BYTE)(Rnd() % 4);
			Char.bySearchCx = (BYTE)(10 + Rnd() % 30);
			Char.bySearchCy = (BYTE)(10 + Rnd() % 30);
			std::string str;
			Proc.Run(&Char, nEx, nEy, str);
			nRuns ++;
			printf("search map=%d k=%d (%d,%d)->(%d,%d) %s\n", m, k, nSx, nSy, nEx, nEy, str.c_str());
		}
	}
	printf("BENCH search2 runs=%d ms=%.1f\n", nRuns, NowMs() - dStart);
}

/* ------------------------------------------------------------------------- */
/* B / E. ゲームを読み込んで動かす											 */
/* ------------------------------------------------------------------------- */
static DWORD HashChars(void)
{
	DWORD dwHash = 2166136261u;
	int nCount = g_aCharInfo.GetSize();
	for (int i = 0; i < nCount; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		DWORD a[] = {p->dwTblID, p->dwMapID, p->dwX, p->dwY, p->byDirection, p->byGameState, p->byFlg,
			p->wHP, p->dwTargetTblID, p->pID, p->byCharState, p->dwLastDotMove};
		for (size_t j = 0; j < sizeof(a) / sizeof(a[0]); j ++) {
			dwHash = (dwHash ^ a[j]) * 16777619u;
		}
	}
	return dwHash;
}

/* ------------------------------------------------------------------------- */
/* F. 飛ぶ特技・魔法が近い敵にも当たるか									 */
/*    目の前の敵のマスに出た弾が、当たり判定の前に1マス進んですり抜けていた	 */
/* ------------------------------------------------------------------------- */
extern BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);
extern BOOL CheckCharBlock(DWORD dwMapID, DWORD dwX, DWORD dwY);
extern void AddTblNoPC(int nNo);
extern void DeleteTblNoPC(int nNo);

static void TestShotMagic(const std::vector<int> &aLogin)
{
	printf("== F. ShotMagic ==\n");
	if (aLogin.empty()) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	/* 右向きに撃つので、左に3マス空いている戦闘用の敵を探す */
	PCHARINFO pEnemy = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pEnemy; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->wGrpMain != CHARGRP_MAIN_NORMAL || (g_aMapInfo[p->dwMapID]->byFlg & MAPFLG_MAGIC)) {
			continue;
		}
		if (p->byNpcType != NPCTYPE_BATTLE1 && p->byNpcType != NPCTYPE_BATTLE2) {
			continue;
		}
		if (p->dwX < 4) {
			continue;
		}
		BOOL bOK = TRUE;
		for (int d = 1; d <= 3; d ++) {
			if (!CheckCharBlock(p->dwMapID, p->dwX - d, p->dwY)) {
				bOK = FALSE;
			}
		}
		if (bOK) {
			pEnemy = p;
		}
	}
	if (!pEnemy) {
		printf("no enemy\n");
		return;
	}
	/* 敵のいるマップへ移す */
	DWORD dwPCMap = pPC->dwMapID;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = pEnemy->dwMapID;
	AddTblNoPC((int)pPC->dwTblID);
	printf("enemy map=%u x=%u y=%u type=%d\n", pEnemy->dwMapID, pEnemy->dwX, pEnemy->dwY, pEnemy->byNpcType);
	DWORD dwEnemyX = pEnemy->dwX, dwEnemyY = pEnemy->dwY;
	WORD wHPBack = pEnemy->wHP, wMaxHPBack = pEnemy->wMaxHP;
	DWORD dwPCX = pPC->dwX, dwPCY = pPC->dwY;
	BYTE byDirBack = pPC->byDirection, abyMagicBack[MAX_USEMAGIC];
	memcpy(abyMagicBack, pPC->abyMagic, sizeof(abyMagicBack));

	static const BYTE abyMagic[] = {39, 74};		/* 風塵剣（戦士）・ヘタな歌（詩人） */
	for (size_t m = 0; m < sizeof(abyMagic) / sizeof(abyMagic[0]); m ++) {
		BYTE byID = abyMagic[m];
		if (byID >= g_aMagicInfo.GetSize()) {
			continue;
		}
		for (int nDist = 1; nDist <= 3; nDist ++) {
			int nHit = 0, nTry = 100;
			for (int t = 0; t < nTry; t ++) {
				pPC->dwX = dwEnemyX - nDist;
				pPC->dwY = dwEnemyY;
				pPC->byDirection = 3;
				pPC->abyMagic[0] = byID;
				pPC->wMana = 9999;
				pEnemy->dwX = dwEnemyX;
				pEnemy->dwY = dwEnemyY;
				pEnemy->wMaxHP = pEnemy->wHP = 30000;
				MagicProc(pPC->dwTblID, byID, (DWORD)-1);
				/* 出た弾を動かし切る */
				for (int k = 0; k < 60; k ++) {
					s_dwFakeNow += 260;
					g_dwTime = s_dwFakeNow;
					BOOL bAny = FALSE;
					for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
						PCHARINFO p = g_aCharInfo[i];
						if ((p->byNpcType != NPCTYPE_USEMAGIC && p->byNpcType != NPCTYPE_USEMAGICPARENT) ||
							p->dwParentTblID != pPC->dwTblID || p->pProc == NULL) {
							continue;
						}
						bAny = TRUE;
						((PCCharProcBase)p->pProc)->Proc(s_dwFakeNow);
					}
					if (!bAny) {
						break;
					}
				}
				if (pEnemy->wHP < 30000) {
					nHit ++;
				}
			}
			printf("shot magic=%d dist=%d hit=%d/%d\n", byID, nDist, nHit, nTry);
		}
	}
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = dwPCMap;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->dwX = dwPCX;
	pPC->dwY = dwPCY;
	pPC->byDirection = byDirBack;
	memcpy(pPC->abyMagic, abyMagicBack, sizeof(abyMagicBack));
	pEnemy->dwX = dwEnemyX;
	pEnemy->dwY = dwEnemyY;
	pEnemy->wHP = wHPBack;
	pEnemy->wMaxHP = wMaxHPBack;
}

/* ------------------------------------------------------------------------- */
/* SM. 召喚の特技（獣召喚 79→80→81 など）を 1 回使って、動かない敵に何回当たるか	 */
/*    環境変数 SUMMON_HITS=1 の時だけ流す（golden には出ない。最後に流す）。		 */
/*    本物の TimerProc を 1 周ずつ回し、時刻は 1 回呼ぶごとに dCharUs マイクロ秒、	 */
/*    1 周ごとに nLapMs ミリ秒（本体の「ちょっと休憩」10ms）進める。				 */
/*    hole = 親より前に空き番号（消えたキャラの跡）が常に H 個ある状態			 */
/*    （子は空いている一番若い番号に入るので、親より若い番号の子はその周に処理されない） */
/* ------------------------------------------------------------------------- */
extern void DeleteChar(DWORD dwTblID);
extern void SetInitCharData(PCHARINFO);
extern PCHARINFO AddChar(PCHARINFO, BOOL bCheck);
extern int g_nProcChar;

static BOOL SmIsHole(PCHARINFO p)
{
	return p->dwMapID == 0 && p->dwX == 0 && p->dwY == 0 && p->szAcc[0] == 0;
}

static int SmAddDummy(void)
{
	PCHARINFO p = new CHARINFO;
	ZeroMemory(p, sizeof(CHARINFO));
	p->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(p);
	p->byGameState = GAMESTATE_NPC;
	p->dwMapID = 0;
	p->dwX = 1;
	p->dwY = 1;
	p = AddChar(p, TRUE);
	return (int)p->dwTblID;
}

struct SMRES { int nHit, nDmg, nTick, nMoved; };

static SMRES SmRun(PCHARINFO pPC, PCHARINFO pEnemy, BYTE byMagic, int dx, int dy, int nHole, double dCharUs, int nLapMs, int nSeed)
{
	SMRES r = {0, 0, 0, 0};
	DWORD dwEnemyX = pEnemy->dwX, dwEnemyY = pEnemy->dwY;
	sgenrand((unsigned long)(1000 + nSeed * 7919));
	/* 空き番号を全部ふさぎ、親は一番後ろに入るようにする。ふさいだ物は穴を作る材料にする */
	std::vector<int> aDummy;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (SmIsHole(g_aCharInfo[i])) {
			aDummy.push_back(SmAddDummy());
			i = -1;
		}
	}
	/* 後ろの方の材料（親の番号より前）を消す順に使う */
	std::sort(aDummy.begin(), aDummy.end());

	pPC->dwMapID = pEnemy->dwMapID;
	pPC->dwX = dwEnemyX + dx;
	pPC->dwY = dwEnemyY + dy;
	pPC->byDirection = 3;
	pPC->wMana = 9999;
	pPC->wHP = pPC->wMaxHP = 60000;
	memset(pPC->abyMagic, 0, sizeof(pPC->abyMagic));
	pPC->abyMagic[0] = byMagic;
	pEnemy->wMaxHP = pEnemy->wHP = 30000;
	pEnemy->wDF = 0;
	pEnemy->chAttrFire = pEnemy->chAttrWater = pEnemy->chAttrWind = pEnemy->chAttrEarth = pEnemy->chAttrLight = pEnemy->chAttrDark = 0;
	pEnemy->byMoveProbabilityX = pEnemy->byMoveProbabilityY = 0;

	double dUs = (double)((nSeed * 397) % 1000);					/* ミリ秒の中のどこで始まるか */
	DWORD dwBase = 3000000 + (DWORD)(nSeed * 100003 % 5000) * 1000;
	int nPre = (nSeed * 131) % 700;									/* 周の途中のどこで使うか */
	size_t nUsedDummy = 0;
	BOOL bCast = FALSE;
	int nCastCalls = 0;
	std::set<DWORD> aTick;
	WORD wBefore = pEnemy->wHP;
	int nLap = 0;
	/* 周の頭（g_nProcChar==0）まで進めてから、nPre 回呼んで使う */
	for (int guard = 0; guard < 100000000 && nLap < 400; guard ++) {
		s_dwFakeNow = dwBase + (DWORD)((dUs) / 1000.0);
		g_dwTime = s_dwFakeNow;
		if (!bCast && g_nProcChar == 0 && nCastCalls == 0) {
			nCastCalls = 1;
		}
		if (!bCast && nCastCalls > 0 && nCastCalls > nPre) {
			MagicProc(pPC->dwTblID, byMagic, pPC->dwTblID);
			bCast = TRUE;
			nLap = 0;
			wBefore = pEnemy->wHP;
		}
		if (nCastCalls > 0 && !bCast) {
			nCastCalls ++;
		}
		pPC->dwLastHartBeat = s_dwFakeNow;
		pPC->wHP = pPC->wMaxHP = 60000;
		TimerProc();
		dUs += dCharUs;
		if (pEnemy->wHP < wBefore) {
			r.nHit ++;
			r.nDmg += wBefore - pEnemy->wHP;
		}
		wBefore = pEnemy->wHP;
		if (g_nProcChar == 0) {
			dUs += nLapMs * 1000.0;
			if (bCast) {
				nLap ++;
				/* 親より前の穴を H 個に保つ */
				if (nHole > 0) {
					int nNow = 0;
					for (size_t k = 0; k < aDummy.size(); k ++) {
						if (SmIsHole(g_aCharInfo[aDummy[k]])) nNow ++;
					}
					while (nNow < nHole && nUsedDummy < aDummy.size()) {
						int n = aDummy[nUsedDummy ++];
						if (!SmIsHole(g_aCharInfo[n])) {
							DeleteChar((DWORD)n);
							nNow ++;
						}
					}
				}
				for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
					PCHARINFO p = g_aCharInfo[i];
					if ((p->byNpcType == NPCTYPE_USEMAGIC || p->byNpcType == NPCTYPE_USEMAGICPARENT) && p->dwParentTblID == pPC->dwTblID) {
						aTick.insert(p->dwTimeLogin);
					}
				}
				/* 親も子も居なくなったら終わり */
				BOOL bAny = FALSE;
				for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
					PCHARINFO p = g_aCharInfo[i];
					if ((p->byNpcType == NPCTYPE_USEMAGIC || p->byNpcType == NPCTYPE_USEMAGICPARENT) && p->dwParentTblID == pPC->dwTblID) {
						bAny = TRUE;
					}
				}
				if (!bAny && nLap > 3) {
					break;
				}
			}
		}
	}
	r.nTick = (int)aTick.size();		/* 最初の親（使った時）の分を含む。子を出した回数は −1 */
	r.nMoved = (pEnemy->dwX != dwEnemyX || pEnemy->dwY != dwEnemyY);
	pEnemy->dwX = dwEnemyX;
	pEnemy->dwY = dwEnemyY;
	/* 残りを片付ける */
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if ((p->byNpcType == NPCTYPE_USEMAGIC || p->byNpcType == NPCTYPE_USEMAGICPARENT) && p->dwParentTblID == pPC->dwTblID) {
			DeleteChar((DWORD)i);
		}
	}
	return r;
}

static void TestSummonHits(const std::vector<int> &aLogin)
{
	if (!getenv("SUMMON_HITS") || aLogin.empty()) {
		return;
	}
	printf("== SM. Summon hits ==\n");
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	/* 周り 8 マスが全部通れる、動かない敵にできる戦闘用の敵（ぬめぬめ優先） */
	PCHARINFO pEnemy = NULL;
	for (int pass = 0; pass < 2 && !pEnemy; pass ++) {
		for (int i = 0; i < g_aCharInfo.GetSize() && !pEnemy; i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if ((p->byNpcType != NPCTYPE_BATTLE1 && p->byNpcType != NPCTYPE_BATTLE2) || p->wGrpMain != CHARGRP_MAIN_NORMAL ||
				p->dwX < 3 || p->dwY < 3 || (g_aMapInfo[p->dwMapID]->byFlg & MAPFLG_MAGIC) || p->dwSpaceID != 0) {
				continue;
			}
			if (pass == 0 && strstr(p->szName, "ぬめぬめ") == NULL) {
				continue;
			}
			BOOL bOK = TRUE;
			for (int dy = -1; dy <= 1; dy ++) {
				for (int dx = -1; dx <= 1; dx ++) {
					if ((dx || dy) && !CheckCharBlock(p->dwMapID, p->dwX + dx, p->dwY + dy)) bOK = FALSE;
				}
			}
			if (bOK) pEnemy = p;
		}
	}
	if (!pEnemy) {
		printf("SM no enemy\n");
		return;
	}
	printf("SM enemy=%s map=%u x=%u y=%u type=%d chars=%d\n", pEnemy->szName, pEnemy->dwMapID, pEnemy->dwX, pEnemy->dwY, pEnemy->byNpcType, g_aCharInfo.GetSize());
	static const BYTE abyMagic[] = {79, 83, 86};
	for (size_t m = 0; m < sizeof(abyMagic); m ++) {
		for (int k = 0; k < 3; k ++) {
			int id = abyMagic[m] + k;
			if (id >= g_aMagicInfo.GetSize()) continue;
			PMAGICINFO pi = g_aMagicInfo[id];
			printf("SM magic %d name=%s type=%d tgt=%d area=%d cnt=%d d1=%d d2=%d d3=%d d4=%d mana=%d\n", id, pi->szName, pi->byType, pi->byTargetType,
				pi->byTargetArea, pi->byTargetCount, pi->wData1, pi->wData2, pi->wData3, pi->wData4, pi->wMana);
		}
	}
	const int NSEED = getenv("SUMMON_SEEDS") ? atoi(getenv("SUMMON_SEEDS")) : 12;
	static const int aPos[][2] = {{1, 0}, {0, 1}, {-1, -1}, {2, 0}, {0, 0}};
	static const int aHole[] = {0, 9};
	for (size_t m = 0; m < sizeof(abyMagic); m ++) {
		for (size_t q = 0; q < sizeof(aPos) / sizeof(aPos[0]); q ++) {
			for (size_t h = 0; h < sizeof(aHole) / sizeof(aHole[0]); h ++) {
				int aHist[40] = {0};
				int nSum = 0, nTick = 0, nMoved = 0, nDmg = 0;
				for (int s = 0; s < NSEED; s ++) {
					SMRES r = SmRun(pPC, pEnemy, abyMagic[m], aPos[q][0], aPos[q][1], aHole[h], 0.05, 10, s);
					aHist[r.nHit < 39 ? r.nHit : 39] ++;
					nSum += r.nHit; nTick += r.nTick; nMoved += r.nMoved; nDmg += r.nDmg;
				}
				printf("SM magic=%d pos=%d,%d hole=%d cost=0.05us lap=10ms avg_hit=%.2f avg_tick=%.2f avg_dmg=%.1f moved=%d hist:", abyMagic[m], aPos[q][0], aPos[q][1],
					aHole[h], (double)nSum / NSEED, (double)nTick / NSEED, (double)nDmg / NSEED, nMoved);
				for (int i = 0; i < 40; i ++) if (aHist[i]) printf(" %d:%d", i, aHist[i]);
				printf("\n");
			}
		}
	}
	/* 時刻の違い（1 周の長さ・1 キャラあたりの処理時間）。獣召喚・隣の敵・穴なし */
	static const double aCost[] = {0.05, 1.0, 5.0, 20.0};
	static const int aLap[] = {10, 16, 30};
	for (size_t c = 0; c < sizeof(aCost) / sizeof(aCost[0]); c ++) {
		for (size_t l = 0; l < sizeof(aLap) / sizeof(aLap[0]); l ++) {
			int aHist[40] = {0};
			int nSum = 0;
			for (int s = 0; s < NSEED; s ++) {
				SMRES r = SmRun(pPC, pEnemy, 79, 1, 0, 0, aCost[c], aLap[l], s);
				aHist[r.nHit < 39 ? r.nHit : 39] ++;
				nSum += r.nHit;
			}
			printf("SM time magic=79 pos=1,0 hole=0 cost=%.2fus lap=%dms avg_hit=%.2f hist:", aCost[c], aLap[l], (double)nSum / NSEED);
			for (int i = 0; i < 40; i ++) if (aHist[i]) printf(" %d:%d", i, aHist[i]);
			printf("\n");
		}
	}
	/* 後始末：動かした位置を戻す必要はない（最後に流すので） */
}

/* ------------------------------------------------------------------------- */
/* G. すごろくの観戦参加													 */
/* ------------------------------------------------------------------------- */
/* 観戦で参加すると、以前は最適化ビルドで unreachable になりサーバーが止まった */
static void TestSugorokuViewer(void)
{
	printf("== G. Sugoroku viewer ==\n");
	PSUBGAME_SUGOROKUINFO pSugoroku = new SUBGAME_SUGOROKUINFO;
	int nNo1 = SubSugoroku_AddMember(pSugoroku, 1, 12);
	int nNo2 = SubSugoroku_AddMember(pSugoroku, 1, 34);
	printf("viewer no=%d,%d join=%d,%d,%d players=%d viewers=%d\n",
		nNo1, nNo2,
		SubSugoroku_CheckJoin(pSugoroku, 12), SubSugoroku_CheckJoin(pSugoroku, 34), SubSugoroku_CheckJoin(pSugoroku, 56),
		pSugoroku->adwPlayerTblID.GetSize(), pSugoroku->adwViewerTblID.GetSize());
	SubSugoroku_Remove(pSugoroku, 12);
	printf("after remove join=%d viewers=%d\n", SubSugoroku_CheckJoin(pSugoroku, 12), pSugoroku->adwViewerTblID.GetSize());
	delete pSugoroku;
}

/* ------------------------------------------------------------------------- */
/* I. 町転送の行き先帳														 */
/* ------------------------------------------------------------------------- */
extern BOOL IsTownWarpMagic(BYTE byID);
extern int  TownWarpMakeList(PCHARINFO pChar, PDWORD padwMapID, PDWORD padwX, PDWORD padwY);
extern void TownWarpRecord(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY, int nForgetMapID);

static void PrintTownWarp(const char *pszLabel, PCHARINFO pChar)
{
	DWORD adwMapID[MAX_TOWNWARP + 1], adwX[MAX_TOWNWARP + 1], adwY[MAX_TOWNWARP + 1];
	int nCount = TownWarpMakeList(pChar, adwMapID, adwX, adwY);
	printf("townwarp %s book=%d list=", pszLabel, pChar->byTownWarpCount);
	for (int i = 0; i < nCount; i ++) {
		printf("%s%u(%u,%u)", i ? "," : "", adwMapID[i], adwX[i], adwY[i]);
	}
	printf("\n");
}

static void TestShipBed(const std::vector<int> &aLogin);
static void TestMercenary(const std::vector<int> &aLogin, BOOL bMagic = FALSE);
static void TestTailMapChg(const std::vector<int> &aLogin);
static void TestTailBattle(const std::vector<int> &aLogin);
extern "C" char *MockLastBody(int nConn, int nCmd);
static void TestQuest(const std::vector<int> &aLogin);
static void TestStory(const std::vector<int> &aLogin);
static void TestSortItem(const std::vector<int> &aLogin);
static void TestCharPush(const std::vector<int> &aLogin);
static void TestSpamLimit(const std::vector<int> &aLogin);
static void TestLeaveDispose(const std::vector<int> &aLogin);
static void TestNpcNear(const std::vector<int> &aLogin);
static void TestChallenge(const std::vector<int> &aLogin);
static void TestStatusUp(const std::vector<int> &aLogin);
static void TestMagicWall(const std::vector<int> &aLogin);
static void TestSpace(const std::vector<int> &aLogin);
static void TestAdminQuest(const std::vector<int> &aLogin);
static void TestFloorDamage(const std::vector<int> &aLogin);
static void TestMoveGuard(const std::vector<int> &aLogin);
static void TestLowParts(const std::vector<int> &aLogin);
static void TestMimic(const std::vector<int> &aLogin);
static void TestEnemyDoor(const std::vector<int> &aLogin);
static void TestDoorExtra(const std::vector<int> &aLogin);
static void TestDoorLockMsg(const std::vector<int> &aLogin);
static void TestImportant(const std::vector<int> &aLogin);
static void TestAdminResurrect(const std::vector<int> &aLogin);
static void TestFlagTalk(const std::vector<int> &aLogin);
static void TestFlagGroup(const std::vector<int> &aLogin);
static void TestStoryGoal(const std::vector<int> &aLogin);
static void TestMarket(const std::vector<int> &aLogin);
static void TestRecycle(const std::vector<int> &aLogin);
static void TestTailItemMove(const std::vector<int> &aLogin);
static void TestViewFlag(const std::vector<int> &aLogin);
static void TestViewFlagTalk(const std::vector<int> &aLogin);
static void TestMapSwap(const std::vector<int> &aLogin);
static void TestAdminChest(const std::vector<int> &aLogin);
static void TestAdminSwap(const std::vector<int> &aLogin);
static void TestMapSwapSave(const std::vector<int> &aLogin);
static void TestMapPartsLight(void);
static void TestJobMagicKey(const std::vector<int> &aLogin);
static void TestAffix(const std::vector<int> &aLogin);
static void TestAffixForge(const std::vector<int> &aLogin);
extern BOOL SaveAffixInfo(void);
extern BOOL LoadAffixInfo(void);
extern BOOL SaveItemDerive(void);
extern BOOL SaveRecipeInfo(void);
extern BOOL LoadRecipeInfo(void);
extern BOOL LoadItemDerive(void);
static void TestTailRelogin(const std::vector<int> &aLogin);
static void TestAttackRate(const std::vector<int> &aLogin);
static void TestTailOverlap(const std::vector<int> &aLogin);
static void TestTailBattleMapMove(const std::vector<int> &aLogin);
static void TestTailSpaceMove(const std::vector<int> &aLogin);
static void TestTailMidLeave(const std::vector<int> &aLogin);
static void TestTailArrive(const std::vector<int> &aLogin);
static void TestPuzzle(const std::vector<int> &aLogin);
static void TestPuzzleVs(const std::vector<int> &aLogin);
static void TestPuzzleIdle(const std::vector<int> &aLogin);
static void TestPuzzleSeat(const std::vector<int> &aLogin);
static void TestPuzzleRecord(const std::vector<int> &aLogin);
static void TestMaze(const std::vector<int> &aLogin);
static void TestEmptyMapCheck(const std::vector<int> &aLogin);
static void TestPutFill(void);
static void TestDungeonTheme(void);
static void TestDungeonBonus(const std::vector<int> &aLogin);
static void TestDungeonDaily(const std::vector<int> &aLogin);
static void TestDungeonDefense(const std::vector<int> &aLogin);
static void TestDungeonDefenseMarch(const std::vector<int> &aLogin);
static void TestDungeonDefenseInside(const std::vector<int> &aLogin);
extern void SubGameProc(DWORD);
extern BOOL DungeonDailyRebuildNow(int, std::string &);
extern "C" const char *SboWebDataDungeonRecords(int nID, int nVerNo, int bTheme);
extern "C" const char *SboWebDataDungeonRebuild(int nID);
extern void MakeDungeonProc(BYTE);
extern BOOL SaveSubGameInfo(void);
extern BOOL LoadSubGameInfo(void);
extern BOOL g_bPutFillOnStart;
extern BOOL g_bLoginLimit;
extern void RecvCmdOnDecPlayer(int);
extern void TailRelinkLeave(PCHARINFO);
extern void BattleProcCheck(PCHARINFO);
extern void BattleProcCheckPC(PCHARINFO);

static void TestTownWarp(const std::vector<int> &aLogin)
{
	printf("== I. Town warp ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	printf("townwarp magic24=%d magic25=%d magic5=%d slot lv1=%d lv4=%d lv5=%d lv10=%d lv15=%d lv99=%d\n",
		IsTownWarpMagic(24), IsTownWarpMagic(25), IsTownWarpMagic(5),
		TOWNWARP_SLOT(1), TOWNWARP_SLOT(4), TOWNWARP_SLOT(5), TOWNWARP_SLOT(10), TOWNWARP_SLOT(15), TOWNWARP_SLOT(99));

	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	PCHARINFO pFriend = g_aCharInfo[aLogin[1]];
	BYTE byLevelRec = pPC->byLevel;
	/* 教会で記録する順（Lv15＝4つまで）：始まりの町→港町→マイス→ルシー→港町（上に戻る）→
	   サニアス（忘れる町の指定なし＝一番古い町）→バオ（マイスを忘れる）→ゼニー（一覧に無い町を指定＝一番古い町）
	   続けて Lv5（2つまで）でブルアイ、Lv1（1つだけ）でリルシェ */
	static const struct { BYTE byLevel; DWORD dwMap; BYTE x, y; int nForget; } aRec[] = {
		{15, 0, 26, 50, -1}, {15, 14, 9, 12, -1}, {15, 35, 13, 30, -1}, {15, 39, 51, 40, -1}, {15, 14, 9, 12, -1},
		{15, 43, 32, 50, -1}, {15, 51, 35, 6, 35}, {15, 55, 60, 100, 99}, {5, 52, 29, 50, 14}, {1, 63, 10, 20, -1},
	};
	pPC->byTownWarpCount = 0;
	pPC->dwStartMapID = 0;
	pPC->dwStartX = 26;
	pPC->dwStartY = 50;
	for (size_t i = 0; i < sizeof(aRec) / sizeof(aRec[0]); i ++) {
		pPC->byLevel = aRec[i].byLevel;
		TownWarpRecord(pPC, aRec[i].dwMap, aRec[i].x, aRec[i].y, aRec[i].nForget);
		char sz[32];
		sprintf(sz, "rec%d", (int)i);
		PrintTownWarp(sz, pPC);
	}
	/* キャスト用に Lv15 の行き先帳を作り直す（始まりの町→港町→マイス→ルシー→バオ→始まりの町） */
	pPC->byLevel = 15;
	pPC->byTownWarpCount = 0;
	TownWarpRecord(pPC, 14, 9, 12, -1);
	TownWarpRecord(pPC, 39, 51, 40, -1);
	TownWarpRecord(pPC, 43, 32, 50, -1);
	TownWarpRecord(pPC, 51, 35, 6, -1);
	PrintTownWarp("cast", pPC);
	pPC->byLevel = byLevelRec;
	/* 復活位置が行き先帳に無い町（管理者が動かした等）でも、0 番目は復活位置 */
	pPC->dwStartMapID = 0;
	pPC->dwStartX = 26;
	pPC->dwStartY = 50;
	PrintTownWarp("start0", pPC);

	/* 町転送を使って床を出し、仲間を乗せる。選んだ番号・レベルごとの行き先 */
	DWORD dwMapBack = pPC->dwMapID, dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byLevelBack = pPC->byLevel, byDirBack = pPC->byDirection, abyMagicBack[MAX_USEMAGIC];
	memcpy(abyMagicBack, pPC->abyMagic, sizeof(abyMagicBack));
	/* byOwner: 0=置いた人はその場 1=置いた人が先に別のマップへ行く 2=置いた人がログアウトする */
	struct { BYTE byLevel; BYTE bySel; BYTE byOwner; } aCase[] = { {15, 0, 0}, {15, 1, 0}, {15, 3, 0}, {15, 4, 0}, {10, 3, 0}, {1, 1, 0}, {5, 1, 0}, {15, 1, 1}, {15, 1, 2} };
	/* 始まりの町で、右に2マス・下に1マス空いている所を探す */
	DWORD dwCastX = 0, dwCastY = 0;
	for (DWORD y = 1; y + 2 < (DWORD)g_aMapInfo[0]->nCY && !dwCastX; y ++) {
		for (DWORD x = 1; x + 3 < (DWORD)g_aMapInfo[0]->nCX && !dwCastX; x ++) {
			if (CheckCharBlock(0, x, y) && CheckCharBlock(0, x + 1, y) && CheckCharBlock(0, x + 2, y) && CheckCharBlock(0, x, y + 1)) {
				dwCastX = x;
				dwCastY = y;
			}
		}
	}
	printf("townwarp map0 flg=%02x cast=(%u,%u)\n", g_aMapInfo[0]->byFlg, dwCastX, dwCastY);
	for (size_t c = 0; c < sizeof(aCase) / sizeof(aCase[0]); c ++) {
		/* 始まりの町 (26,53) 付近で、右向きに使う */
		DeleteTblNoPC((int)pPC->dwTblID);
		DeleteTblNoPC((int)pFriend->dwTblID);
		pPC->dwMapID = pFriend->dwMapID = 0;
		pPC->dwX = dwCastX;
		pPC->dwY = dwCastY;
		pPC->byDirection = 3;
		AddTblNoPC((int)pPC->dwTblID);
		pFriend->dwX = dwCastX;
		pFriend->dwY = dwCastY + 1;
		AddTblNoPC((int)pFriend->dwTblID);
		pPC->byLevel = aCase[c].byLevel;
		pPC->abyMagic[0] = 24;
		pPC->wMana = 9999;
		pPC->byCharState = CHARSTATE_MOVE;
		pFriend->byCharState = CHARSTATE_MOVE;
		pPC->byTownWarpSel = aCase[c].bySel;
		s_dwFakeNow += 5000;
		g_dwTime = s_dwFakeNow;
		MagicProc(pPC->dwTblID, 24, (DWORD)-1);
		pPC->byTownWarpSel = 0;
		/* 出た床を探す */
		PCHARINFO pFloor = NULL;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p->byNpcType == NPCTYPE_USEMAGIC && p->dwParentTblID == pPC->dwTblID && p->pProc && p->dwTimeDelete) {
				pFloor = p;
			}
		}
		if (pFloor == NULL) {
			printf("townwarp cast lv=%d sel=%d no floor map=%u\n", aCase[c].byLevel, aCase[c].bySel, pPC->dwMapID);
			continue;
		}
		/* 置いた人が先に床に入って別のマップへ行った・ログアウトした */
		DWORD pIDBack = pPC->pID;
		if (aCase[c].byOwner == 1) {
			DeleteTblNoPC((int)pPC->dwTblID);
			pPC->dwMapID = 14;
			AddTblNoPC((int)pPC->dwTblID);
		} else if (aCase[c].byOwner == 2) {
			pPC->pID = 0;
		}
		if (aCase[c].byOwner) {
			/* 消える時は 1 秒後なので、少し長めに回す */
			for (int k = 0; k < 6 && pFloor->pProc; k ++) {
				s_dwFakeNow += 300;
				g_dwTime = s_dwFakeNow;
				((PCCharProcBase)pFloor->pProc)->Proc(s_dwFakeNow);
			}
			BOOL bGone = (pFloor->pProc == NULL);
			pPC->pID = pIDBack;
			printf("townwarp owner=%d lv=%d sel=%d floor gone=%d\n", aCase[c].byOwner, aCase[c].byLevel, aCase[c].bySel, bGone);
			if (bGone) {
				continue;
			}
		}
		/* 仲間が床へ歩いて乗る */
		DeleteTblNoPC((int)pFriend->dwTblID);
		pFriend->dwX = pFloor->dwX;
		pFriend->dwY = pFloor->dwY;
		AddTblNoPC((int)pFriend->dwTblID);
		pFriend->dwLastDotMove = s_dwFakeNow;
		for (int k = 0; k < 8 && pFriend->dwMapID == 0; k ++) {
			s_dwFakeNow += 300;
			g_dwTime = s_dwFakeNow;
			pFriend->dwLastDotMove = s_dwFakeNow;
			((PCCharProcBase)pFloor->pProc)->Proc(s_dwFakeNow);
		}
		printf("townwarp cast lv=%d sel=%d floor dest=%d:%u -> friend map=%u x=%u y=%u\n",
			aCase[c].byLevel, aCase[c].bySel, pFloor->byTownWarpDest, pFloor->dwStartMapID,
			pFriend->dwMapID, pFriend->dwX, pFriend->dwY);
		/* 床を片付ける */
		pFloor->dwTimeDelete = 0;
		pFloor->dwTimeLogin = 0;
		pFloor->dwLastDotMove = 0;
		s_dwFakeNow += 300;
		((PCCharProcBase)pFloor->pProc)->Proc(s_dwFakeNow);
	}
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = dwMapBack;
	pPC->dwX = dwXBack;
	pPC->dwY = dwYBack;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->byLevel = byLevelBack;
	pPC->byDirection = byDirBack;
	memcpy(pPC->abyMagic, abyMagicBack, sizeof(abyMagicBack));
}

#define NLOGIN	14			/* ログイン中にするPCの数 */

static void TestWorld(void)
{
	/* ゲーム本体と同じ手順で読み込む（ウィンドウを作ると WM_CREATE で OnCreate が走る） */
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = "SBOSVRTEST";
	wc.lpfnWndProc = (WNDPROC)WndProc;
	RegisterClass(&wc);
	s_dwFakeNow = 1000000;
	/* 起動時の NPC発生の出し切りは、前からの出力と比べるため止めておき、最後（AM）で確かめる */
	g_bPutFillOnStart = FALSE;
	/* 同じ接続元から何度もログインするので、ログイン回数の制限は止める */
	g_bLoginLimit = FALSE;
	HWND hWnd = CreateWindow("SBOSVRTEST", "t", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
	if (!hWnd) {
		printf("CreateWindow failed\n");
		return;
	}
	sgenrand(4357);

	int nChars = g_aCharInfo.GetSize();
	int nPC = 0, nNPC = 0;
	int anType[256];
	ZeroMemory(anType, sizeof(anType));
	for (int i = 0; i < nChars; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType == 0) {
			nPC ++;
		} else {
			nNPC ++;
			anType[p->byNpcType] ++;
		}
	}
	printf("== world chars=%d pc=%d npc=%d maps=%d ==\n", nChars, nPC, nNPC, g_aMapInfo.GetSize());
	for (int i = 0; i < 256; i ++) {
		if (anType[i]) {
			printf("npctype %d count %d\n", i, anType[i]);
		}
	}
	TestSearchStep();
	TestSearch2();
	TestEnemyAI();
	TestEnemyAI2();

	/* データには PC がいないので、NPC を複製して PC を作る（ログアウト中 3000 人、うち NLOGIN 人をログイン中にする） */
	PCHARINFO pTemplate = NULL;
	for (int i = 0; i < nChars && !pTemplate; i ++) {
		if (g_aCharInfo[i]->byNpcType == NPCTYPE_MOVERAND) {
			pTemplate = g_aCharInfo[i];
		}
	}
	for (int i = 0; i < 3000; i ++) {
		PCHARINFO p = new CHARINFO;
		memcpy(p, pTemplate, sizeof(CHARINFO));
		p->byNpcType = 0;
		p->pProc = NULL;
		p->pID = 0;
		p->byGameState = GAMESTATE_LOGOFF;
		p->byCharState = CHARSTATE_LOGOFF;
		p->wHP = 10;
		p->dwTblID = g_aCharInfo.GetSize();
		p->dwMapID = i % g_aMapInfo.GetSize();
		g_aCharInfo.Add(p);
	}
	nChars = g_aCharInfo.GetSize();

	std::vector<int> aNpcOnMap(g_aMapInfo.GetSize(), 0);
	for (int i = 0; i < nChars; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType != 0 && p->dwMapID < aNpcOnMap.size()) {
			aNpcOnMap[p->dwMapID] ++;
		}
	}
	int nMap = 0;
	for (size_t i = 1; i < aNpcOnMap.size(); i ++) {
		if (aNpcOnMap[i] > aNpcOnMap[nMap]) {
			nMap = (int)i;
		}
	}
	PMAPINFO pMap = g_aMapInfo[nMap];
	std::vector<int> aNpcPos;
	for (int i = 0; i < nChars; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType != 0 && p->dwMapID == (DWORD)nMap) {
			aNpcPos.push_back(i);
		}
	}
	printf("login map=%d npcs=%d size=%dx%d\n", nMap, (int)aNpcPos.size(), pMap->nCX, pMap->nCY);

	s_dwSeed = 99;
	std::vector<int> aLogin;
	for (int i = 0; i < nChars && (int)aLogin.size() < NLOGIN; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType != 0 || p->byGameState != GAMESTATE_LOGOFF || (i % 200) != 7) {
			continue;
		}
		int nSlot = Connect(1000 + (int)aLogin.size(), TRUE);
		if (nSlot < 0) {
			break;
		}
		PCHARINFO pBase = g_aCharInfo[aNpcPos[Rnd() % aNpcPos.size()]];
		int x = (int)pBase->dwX + (int)(Rnd() % 9) - 4;
		int y = (int)pBase->dwY + (int)(Rnd() % 9) - 4;
		x = x < 1 ? 1 : (x >= pMap->nCX - 1 ? pMap->nCX - 2 : x);
		y = y < 1 ? 1 : (y >= pMap->nCY - 1 ? pMap->nCY - 2 : y);
		p->dwMapID = nMap;
		p->dwX = x;
		p->dwY = y;
		p->pID = URARASOCK_IDBASE + nSlot;
		p->byGameState = GAMESTATE_MOVE;
		p->byCharState = CHARSTATE_MOVE;
		if (p->wHP == 0) {
			p->wHP = 10;
		}
		p->dwLastHartBeat = 0;
		AddTblNoPC(p->dwTblID);
		aLogin.push_back(i);
	}
	printf("logged in %d\n", (int)aLogin.size());
	PrintAll("login-sent");

	/* B. 同画面内への配信 */
	{
		printf("== B. Inside ==\n");
		MockReset();
		static const DWORD aSize[] = {20, 127, 128, 300, 3000};
		for (size_t s = 0; s < sizeof(aSize) / sizeof(aSize[0]); s ++) {
			for (size_t j = 0; j < aLogin.size(); j ++) {
				PCHARINFO p = g_aCharInfo[aLogin[j]];
				std::vector<BYTE> a;
				MakePayload(a, aSize[s], (s & 1) ? 1 : 0);
				SendCmdInside2(p, &a[0], aSize[s], 0, (j & 1) ? SENDINSIDE_INSIDENOME : 0, TRUE, FALSE,
					(j % 3 == 0) ? (BYTE)URARASOCK_SENDPRIORITY_HIGH : (BYTE)URARASOCK_SENDPRIORITY_MIDDLE);
			}
			std::vector<BYTE> a;
			MakePayload(a, aSize[s], 0);
			SendCmdInsideMap(nMap, &a[0], aSize[s]);
			char sz[64];
			sprintf(sz, "inside size=%u", (unsigned)aSize[s]);
			PrintAll(sz);
		}
		/* 横取り */
		std::vector<BYTE> a;
		MakePayload(a, 400, 1);
		SvrSendCaptureBegin();
		SendCmdInside2(g_aCharInfo[aLogin[0]], &a[0], 400, 0, 0);
		std::vector<BYTE> aCap;
		BOOL bRet = SvrSendCaptureEnd(aCap);
		printf("inside capture ret=%d size=%u\n", bRet, (unsigned)aCap.size());
	}

	/* E. 巡回。1 周ごとに時刻を進める（周回数は g_nProcChar が 0 に戻るのを見る） */
	{
		printf("== E. TimerProc ==\n");
		MockReset();
		s_dwSeed = 31337;
		int nLaps = 400;
		double dStart = NowMs();
		int nCalls = 0;
		for (int lap = 0; lap < nLaps; lap ++) {
			/* PC を動かす・発言させる */
			for (size_t j = 0; j < aLogin.size(); j ++) {
				PCHARINFO p = g_aCharInfo[aLogin[j]];
				if (Rnd() % 6 == 0) {
					int x = (int)p->dwX + (int)(Rnd() % 3) - 1;
					int y = (int)p->dwY + (int)(Rnd() % 3) - 1;
					if (x >= 1 && y >= 1 && x < pMap->nCX - 1 && y < pMap->nCY - 1) {
						p->dwX = x;
						p->dwY = y;
						p->byDirection = (BYTE)(Rnd() % 4);
						p->byFlg |= CHARINFOFLG_MOVE | CHARINFOFLG_DIRECTION;
					}
				}
				p->dwLastHartBeat = s_dwFakeNow;
			}
			do {
				TimerProc();
				nCalls ++;
			} while (g_nProcChar != 0 && nCalls < 100000000);
			s_dwFakeNow += 20 + (lap % 7);
			if (lap % 40 == 39) {
				char sz[64];
				sprintf(sz, "lap=%d chars=%08x", lap, HashChars());
				PrintAll(sz);
				if (getenv("DUMPLAP") && lap == atoi(getenv("DUMPLAP"))) {
					for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
						PCHARINFO p = g_aCharInfo[i];
						printf("DUMP %d %u %u %u %u %u %u %u %u\n", i, p->dwMapID, p->dwX, p->dwY, p->byDirection, p->byFlg, p->wHP, p->dwTargetTblID, p->dwLastDotMove);
					}
				}
			}
		}
		printf("BENCH timerproc laps=%d calls=%d ms=%.1f\n", nLaps, nCalls, NowMs() - dStart);
		PrintAll("lap-final");
		printf("final chars=%08x\n", HashChars());
	}

	/* F. 飛ぶ特技・魔法（移動確率つきの魔法発生）が、近い敵にも当たること */
	TestShotMagic(aLogin);

	/* H. 他人に送るキャラ情報は、画面に要らない項目（ログイン回数・NPC の発生範囲など）を消す */
	{
		printf("== H. CHARINFO mask ==\n");
		PCHARINFO pMe = g_aCharInfo[aLogin[0]];
		PCHARINFO pOther = g_aCharInfo[aLogin[1]];
		/* 同じマスに集めて、画面内の判定を確実に通す */
		pOther->dwX = pMe->dwX;
		pOther->dwY = pMe->dwY;
		int nNpc = -1;
		for (int i = 0; i < nChars; i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p->byNpcType != 0 && p->dwMapID == (DWORD)nMap && p->byGameState >= GAMESTATE_MOVE) {
				nNpc = i;
				break;
			}
		}
		PCHARINFO pNpc = g_aCharInfo[nNpc];
		DWORD dwNpcX = pNpc->dwX, dwNpcY = pNpc->dwY;
		pNpc->dwX = pMe->dwX;
		pNpc->dwY = pMe->dwY;
		/* 確かめやすい値を入れておく */
		pOther->dwLoginCount = 123;
		pOther->dwPlayTime = 456;
		pOther->wMakeYear = 2003;
		pNpc->byAreaX = 7;
		pNpc->byPutCount = 5;
		pNpc->wTimePutInterval = 30;
		pNpc->byMoveProbabilityX = 40;
		pNpc->bySearchCx = 3;
		pNpc->byDropProbability = 9;
		pMe->dwLoginCount = 77;
		BYTE byAdminBack = pMe->byAdminLV;

		struct { const char *pszLabel; PCHARINFO pChar; int nNo; BYTE byAdmin; } aCase[] = {
			{"self",        pMe,    aLogin[0], ADMINLV_USER},
			{"other",       pOther, aLogin[1], ADMINLV_USER},
			{"npc",         pNpc,   nNpc,      ADMINLV_USER},
			{"other-admin", pOther, aLogin[1], ADMINLV_ADMINISTRATOR},
			{"npc-admin",   pNpc,   nNpc,      ADMINLV_ADMINISTRATOR},
		};
		for (size_t c = 0; c < sizeof(aCase) / sizeof(aCase[0]); c ++) {
			pMe->byAdminLV = aCase[c].byAdmin;
			SvrSendCaptureBegin();
			SendCmdCHARINFO(pMe->pID, aCase[c].nNo);
			std::vector<BYTE> aCap;
			BOOL bRet = SvrSendCaptureEnd(aCap);
			CMDPARA_CHARINFO Para;
			ZeroMemory(&Para, sizeof(Para));
			if (bRet) {
				SboPackGetCHARINFO(&Para, &aCap[0], &aCap[0] + aCap.size());
			}
			printf("charinfo %s ret=%d tbl=%d same=%d login=%u play=%u make=%u area=%u put=%u interval=%u move=%u search=%u drop=%u sttmp=%d npcid=%d\n",
				aCase[c].pszLabel, bRet, (int)(Para.dwTblID == aCase[c].pChar->dwTblID), (int)(strcmp(Para.szName, aCase[c].pChar->szName) == 0),
				Para.dwLoginCount, Para.dwPlayTime, Para.wMakeYear, Para.byAreaX, Para.byPutCount, Para.wTimePutInterval,
				Para.byMoveProbabilityX, Para.bySearchCx, Para.byDropProbability, (int)Para.dwStTmplateID,
				/* NPC番号: 0=送っていない 1=正しい値 -1=違う値 */
				Para.dwNpcID ? ((Para.dwNpcID == aCase[c].pChar->dwNpcID) ? 1 : -1) : 0);
		}
		/* 同画面内へ配る版（SendCmdCHARINFO3）。NPC の情報を、自分だけが見ている状態で送る */
		std::vector<BYTE> aState;
		for (size_t j = 1; j < aLogin.size(); j ++) {
			aState.push_back(g_aCharInfo[aLogin[j]]->byGameState);
			g_aCharInfo[aLogin[j]]->byGameState = GAMESTATE_LOGOFF;
		}
		for (int a = 0; a < 2; a ++) {
			pMe->byAdminLV = a ? ADMINLV_ADMINISTRATOR : ADMINLV_USER;
			SvrSendCaptureBegin();
			SendCmdCHARINFO3(nNpc);
			std::vector<BYTE> aCap;
			BOOL bRet = SvrSendCaptureEnd(aCap);
			CMDPARA_CHARINFO Para;
			ZeroMemory(&Para, sizeof(Para));
			if (bRet) {
				SboPackGetCHARINFO(&Para, &aCap[0], &aCap[0] + aCap.size());
			}
			printf("charinfo3 %s ret=%d area=%u put=%u interval=%u move=%u search=%u drop=%u npcid=%d\n",
				a ? "admin" : "user", bRet, Para.byAreaX, Para.byPutCount, Para.wTimePutInterval,
				Para.byMoveProbabilityX, Para.bySearchCx, Para.byDropProbability,
				Para.dwNpcID ? ((Para.dwNpcID == pNpc->dwNpcID) ? 1 : -1) : 0);
		}
		for (size_t j = 1; j < aLogin.size(); j ++) {
			g_aCharInfo[aLogin[j]]->byGameState = aState[j - 1];
		}
		pMe->byAdminLV = byAdminBack;
		pNpc->dwX = dwNpcX;
		pNpc->dwY = dwNpcY;
	}

	/* I. 町転送の行き先帳（教会で記録した町を覚える・選んだ町へ飛ぶ） */
	TestTownWarp(aLogin);
	/* J. 船で目覚めるベッドがばらける */
	TestShipBed(aLogin);
	/* K. 傭兵 */
	TestMercenary(aLogin);
	/* L. ついていく列のマップ間移動で暗転が1回だけ */
	TestTailMapChg(aLogin);
	/* L2. 戦闘モードの間だけ、ついていく列から抜ける */
	TestTailBattle(aLogin);
	/* M. ギルドの依頼 */
	TestQuest(aLogin);
	/* AG2. 印で変わるマス・宝箱の保存と読み直し（キャラ・マップの保存を読み直すので、ギルドの依頼の読み直しの直後に流す） */
	TestMapSwapSave(aLogin);
	TestMapPartsLight();
	/* N. 道具欄・預かり所の並べ替え */
	TestSortItem(aLogin);
	/* O. 押せるNPCを押す向き */
	TestCharPush(aLogin);
	/* AN. 発言の長さ・連投、コマンドの連打、取り引きの割り込み */
	TestSpamLimit(aLogin);
	/* P. 預かり所の品を引き取らずに売る・捨てる */
	TestLeaveDispose(aLogin);
	/* AN. 店・売る・預かり所・転職・着せ替え・名前の色が、公式クライアントと同じ場面だけ通る */
	TestNpcNear(aLogin);
	/* Q. 討伐チャレンジ */
	TestChallenge(aLogin);
	/* R. 攻撃力・守備力アップ */
	TestStatusUp(aLogin);
	/* S. 前方・周囲の特技が壁越しに届かない */
	TestMagicWall(aLogin);
	/* T. 空間（同じ場所を組ごとに分ける） */
	TestSpace(aLogin);
	/* U. 管理者ウィンドウからギルドの依頼を直す */
	TestAdminQuest(aLogin);
	/* V. ダメージ床（毒沼・溶岩など） */
	TestFloorDamage(aLogin);
	/* 移動の見張り（壁・速さ・倒れている間） */
	TestMoveGuard(aLogin);
	/* 低いパーツ（灯り・矢・特技が越える） */
	TestLowParts(aLogin);
	/* W. 宝箱の敵（調べる・叩くと正体を現す） */
	TestMimic(aLogin);
	/* X. 敵を全部倒すと開く扉 */
	TestEnemyDoor(aLogin);
	TestDoorExtra(aLogin);
	TestDoorLockMsg(aLogin);
	/* Y. 管理者ウィンドウの蘇生ボタン */
	TestAdminResurrect(aLogin);
	TestFlagTalk(aLogin);
	/* AA. 取引所（委託販売） */
	TestMarket(aLogin);
	/* AH. リサイクルボックス（おすそわけの共有の箱） */
	TestRecycle(aLogin);
	/* AB. 大事な物の欄と鍵の扉（保存の読み直しでキャラが増えるので最後に流す） */
	TestImportant(aLogin);
	/* AD. 印で見える・消えるNPC（乱数を使わない部分。保存の読み直しでキャラの欄が書き換わるので、AB の後・乱数を使う K2 の前に流す） */
	TestViewFlag(aLogin);
	/* AG. 印で変わるマス・宝箱（差し替えの送信・通れる判定・調べて品と記録・持ち物いっぱい・API の書き込み。乱数を使わない） */
	TestMapSwap(aLogin);
	/* AG3. 管理者ウィンドウのキャラの開けた宝箱の記録（全宝箱の一覧・立てる／消す・全部戻す・差し替えの送り直し・管理者以外は断る） */
	TestAdminChest(aLogin);
	/* AI. 管理者ウィンドウの「印で変わるマス・宝箱」（CMD_ADMINSWAP。足す・直す・消す・番号の自動振り・重なりと種類違いの拒否・送り直し・元に戻す） */
	TestAdminSwap(aLogin);
	/* AD2. 転職で特技ショートカットを職業ごとに切り替え・特技の並べ替え・列の保存（保存の読み直しがあるので、K2 の前に流す） */
	TestJobMagicKey(aLogin);
	/* K2. 傭兵への回復・蘇生の特技（ゲームの乱数を使うので最後） */
	TestMercenary(aLogin, TRUE);
	/* AC. 移動アイテム・帰還の床で、ついていく列ごと移動する（時刻と乱数を使うので最後） */
	TestTailItemMove(aLogin);
	/* AE. 印で見える・消えるNPC：話して印が立つ時の見え直し（会話で乱数を使うので、いちばん最後） */
	TestViewFlagTalk(aLogin);
	/* M2. 思い出の本（保存の読み直しでキャラの欄が書き換わるので、いちばん最後） */
	TestStory(aLogin);
	/* AF. フラグのグループ（メイン-サブ。印の保存は触らないが、名前表・グループの保存を読み直すので最後） */
	TestFlagGroup(aLogin);
	TestStoryGoal(aLogin);
	/* AJ. 戦闘モードの覚えが残ったまま入り直しても「ついていく」が届く */
	TestTailRelogin(aLogin);
	/* 攻撃の威力は、前の攻撃からの経過時間で切り詰める */
	TestAttackRate(aLogin);
	/* AK. 先頭と同じマスに重なって戦闘モードを解除しても、ワープせず列につながる */
	TestTailOverlap(aLogin);
	/* AL. イベントの無いマップでマスに乗った問い合わせ(CHECKEVENT 0)にも返事が来る */
	TestEmptyMapCheck(aLogin);
	/* AM. 起動時に NPC発生を発生数まで出す（乱数を使うので最後） */
	TestPutFill();
	/* AH. 付与（接頭辞・接尾辞）：派生アイテム・強さと売値・未鑑定・鑑定屋・保存・ドロップ（乱数を使うので、いちばん最後） */
	TestAffix(aLogin);
	/* AH2. 付与の段階2：鍛冶屋で鍛える・付け替える（TestAffix の後ろ。乱数は使わない） */
	TestAffixForge(aLogin);
	/* AN. ダンジョンのテーマ生成（サブゲーム「ダンジョン」の作り直し。マップの中身を一時的に差し替えて、終わったら戻す） */
	TestDungeonTheme();
	/* AO. 番外編ダンジョン（段階1。ダンジョンのテーマ生成の後ろ。乱数を使うので最後のほう） */
	TestDungeonBonus(aLogin);
	/* AP. 番外編ダンジョン（段階2。日替わり・最短記録・クリア条件。AO の後ろ） */
	TestDungeonDaily(aLogin);
	/* AQ. 番外編ダンジョン（拠点防衛モード。作り直さない固定の形・波・拠点・復活・勝ち負け。AP の後ろ） */
	TestDungeonDefense(aLogin);
	/* AR. 番外編ダンジョン（拠点防衛モードの敵の行進。AQ の後ろ） */
	TestDungeonDefenseMarch(aLogin);
	/* AS. 番外編ダンジョン（固定の形の中の開始位置。AR の後ろ） */
	TestDungeonDefenseInside(aLogin);
	/* L3. 戦闘でばらけた列は、マップ移動後も戻り先を覚えている */
	TestTailBattleMapMove(aLogin);
	/* L4. 戦闘で列を離れて戻る途中に空間が変わっても、追いかけが残らない */
	TestTailSpaceMove(aLogin);
	/* AT. 列の途中の人が抜けても、後ろの人は前の人へつなぎ直る */
	TestTailMidLeave(aLogin);
	/* AU. ついていく人が、着いたマスの別のマップ移動に反応しない */
	TestTailArrive(aLogin);
	/* AV〜AV5 のコイン（アイテム 1）は、本番のコイン（番号 314）と同じ「効果なし・使っても減らない」にしておく
	   （座る場所でコインを使うと台が取り上げ、使えなかった時は減らないことを確かめるため。終わったら元に戻す） */
	BYTE byCoinTypeBack = g_aItemInfo[1]->byType, byCoinFlgBack = g_aItemInfo[1]->byFlg, byCoinEraseBack = g_aItemInfo[1]->byEraseProbability;
	g_aItemInfo[1]->byType = 0;
	g_aItemInfo[1]->byFlg &= (BYTE)~ITEMFLG_LOST;
	g_aItemInfo[1]->byEraseProbability = 0;
	/* AV. 落ち物パズルの台（コイン・席・キー入力・落下・ゲームオーバー・片付け） */
	TestPuzzle(aLogin);
	/* AV2. 落ち物パズルの対戦（ペアの台・カウントダウン・お邪魔・勝敗・観戦） */
	TestPuzzleVs(aLogin);
	TestPuzzleIdle(aLogin);
	/* AV4. 落ち物パズルの座る場所（台詞の 9・10 番目）と、見えない台NPC */
	TestPuzzleSeat(aLogin);
	/* AV5. 落ち物パズルの INSERT COIN の案内（座る場所に立つと本人にだけ）・連勝・ハイスコアと連勝記録（保存・掲示板 @h・API） */
	TestPuzzleRecord(aLogin);
	/* AW. 迷路対戦の台（台詞 @k・座る・進む・壁・不正な操作・勝ち・やり直し・席を立つ・観戦・片付け） */
	TestMaze(aLogin);
	g_aItemInfo[1]->byType = byCoinTypeBack;
	g_aItemInfo[1]->byFlg = byCoinFlgBack;
	g_aItemInfo[1]->byEraseProbability = byCoinEraseBack;
	/* SM. 召喚特技のヒット数の実測（SUMMON_HITS=1 の時だけ。最後に流す） */
	TestSummonHits(aLogin);
}

/* ------------------------------------------------------------------------- */
/* AC. ついていく列の移動アイテム・帰還の床：列ごと連れて行く				 */
/*    (a) 先頭が移動アイテム (b) 先頭が帰還の床（町転送の床）で移動			 */
/*        → 後ろの人も同じ場所へ移り、列はそのまま・フェードアウト(0)が1回	 */
/*    (c) 列の途中の人が使うと、前の人からは外れ、後ろの人を連れて行く		 */
/* ------------------------------------------------------------------------- */
static int TailIdx(PCHARINFO *apChar, DWORD dwTblID)
{
	for (int i = 0; i < 3; i ++) {
		if (apChar[i]->dwTblID == dwTblID) return i;
	}
	return -1;		/* 列にいない（-1 の時も含む） */
}

static void TailItemPrint(const char *pszLabel, PCHARINFO *apChar, int nTrace)
{
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		char *pszFade = MockCmdData(1000 + i, CMD_FADEINOUT);		/* aLogin[i] は接続 1000+i */
		char *pszBgm = MockCmdData(1000 + i, CMD_CHGBGM);
		printf("tailitem %s %c map=%u pos=(%u,%u) front=%d back=%d fade=[%s] chgbgm=%d\n",
			pszLabel, 'A' + i, p->dwMapID, p->dwX, p->dwY,
			TailIdx(apChar, p->dwTailTblID), TailIdx(apChar, p->dwTailBackTblID),
			pszFade, (int)(pszBgm[0] ? 1 + std::count(pszBgm, pszBgm + strlen(pszBgm), ',') : 0));
		free(pszFade);
		free(pszBgm);
	}
	(void)nTrace;
}

static void TestTailItemMove(const std::vector<int> &aLogin)
{
	printf("== AC. Tail item move ==\n");
	if (aLogin.size() < 4) {
		return;
	}
	DWORD dwNowBack = s_dwFakeNow, dwTimeBack = g_dwTime;
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	/* 始まりの町で、右に2マス・下に1マス空いている所を探す（移動先・床を置く所） */
	DWORD dwCastX = 0, dwCastY = 0;
	for (DWORD y = 1; y + 3 < (DWORD)g_aMapInfo[0]->nCY && !dwCastX; y ++) {
		for (DWORD x = 1; x + 3 < (DWORD)g_aMapInfo[0]->nCX && !dwCastX; x ++) {
			if (CheckCharBlock(0, x, y) && CheckCharBlock(0, x + 1, y) && CheckCharBlock(0, x + 2, y) && CheckCharBlock(0, x, y + 1)) {
				dwCastX = x;
				dwCastY = y;
			}
		}
	}
	printf("tailitem dest=(%u,%u)\n", dwCastX, dwCastY);

	PCHARINFO apChar[3];
	DWORD adwBack[3][5];
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i] = g_aCharInfo[aLogin[i]];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
	}
	PCHARINFO pCaster = g_aCharInfo[aLogin[3]];

	/* 列（A←B←C）を港町の同じ場所に縦に並べる。移動の後で呼び直す */
	auto setLine = [&](DWORD dwMapID, DWORD dwX, DWORD dwY) {
		for (int i = 0; i < 3; i ++) {
			PCHARINFO p = apChar[i];
			DeleteTblNoPC((int)p->dwTblID);
			p->dwMapID = dwMapID;
			p->dwX = p->dwXBack = dwX;
			p->dwY = p->dwYBack = dwY + i;
			p->byCharState = CHARSTATE_MOVE;
			if (p->wHP == 0) p->wHP = 10;
			p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
			AddTblNoPC((int)p->dwTblID);
		}
		for (int i = 0; i < 3; i ++) {
			apChar[i]->dwTailTblID = (i > 0) ? apChar[i - 1]->dwTblID : (DWORD)-1;
			apChar[i]->dwTailBackTblID = (i < 2) ? apChar[i + 1]->dwTblID : (DWORD)-1;
		}
	};

	/* 移動アイテム：テスト用に実データの道具を1つ移動アイテムへ書き換えて使う（終わったら戻す） */
	int nItem = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && !nItem; i ++) {
		if (g_aItemInfo[i] && g_aItemInfo[i]->szName[0]) nItem = i;
	}
	ITEMINFO ItemBack = *g_aItemInfo[nItem];
	ITEMINFO_RURA Rura;
	Rura.dwEnableMapID = 0;
	Rura.dwMapID = 0;
	Rura.dwX = dwCastX;
	Rura.dwY = dwCastY;
	PITEMINFO pItem = g_aItemInfo[nItem];
	pItem->byType = ITEMTYPE_RURA;
	pItem->pData = &Rura;
	pItem->pMsg = NULL;
	pItem->byFlg &= ~ITEMFLG_LOST;
	pItem->byEraseProbability = 0;
	pItem->bySexDistinction = SEXDISTINCTION_ALL;
	pItem->bySound = 0;
	WORD wItemBack[3] = { apChar[0]->waItem[0], apChar[1]->waItem[0], apChar[2]->waItem[0] };
	for (int i = 0; i < 3; i ++) apChar[i]->waItem[0] = (WORD)nItem;
	BYTE byMapFlgBack = pPort->byFlg;
	pPort->byFlg &= ~MAPFLG_MOVE;

	/* (a) 先頭が移動アイテムを使う */
	setLine(pPort->dwID, 40, 10);
	MockTrace(1);
	ItemProc(apChar[0]->dwTblID, 0, apChar[0]->dwTblID);
	MockTrace(0);
	TailItemPrint("a-head", apChar, 0);

	/* (c) 列の途中の人が使う（A はその場・B と C が一緒に移る） */
	setLine(pPort->dwID, 40, 10);
	MockTrace(1);
	ItemProc(apChar[1]->dwTblID, 0, apChar[1]->dwTblID);
	MockTrace(0);
	TailItemPrint("c-middle", apChar, 0);

	/* 最後尾が使う（前の人から外れて一人で移る） */
	setLine(pPort->dwID, 40, 10);
	MockTrace(1);
	ItemProc(apChar[2]->dwTblID, 0, apChar[2]->dwTblID);
	MockTrace(0);
	TailItemPrint("c-tail", apChar, 0);

	pPort->byFlg = byMapFlgBack;
	for (int i = 0; i < 3; i ++) apChar[i]->waItem[0] = wItemBack[i];
	*g_aItemInfo[nItem] = ItemBack;

	/* (b) 先頭が町転送の床（帰還）に乗る。D が始まりの町で床を出し、A が歩いて乗る */
	DWORD dwMapBack = pCaster->dwMapID, dwXBack = pCaster->dwX, dwYBack = pCaster->dwY;
	BYTE byLevelBack = pCaster->byLevel, byDirBack = pCaster->byDirection, abyMagicBack[MAX_USEMAGIC];
	WORD wManaBack = pCaster->wMana;
	DWORD dwStartMapBack = pCaster->dwStartMapID, dwStartXBack = pCaster->dwStartX, dwStartYBack = pCaster->dwStartY;
	memcpy(abyMagicBack, pCaster->abyMagic, sizeof(abyMagicBack));
	/* 行き先は D の復活位置（港町） */
	pCaster->dwStartMapID = pPort->dwID;
	pCaster->dwStartX = 40;
	pCaster->dwStartY = 54;
	DeleteTblNoPC((int)pCaster->dwTblID);
	pCaster->dwMapID = 0;
	pCaster->dwX = dwCastX;
	pCaster->dwY = dwCastY;
	pCaster->byDirection = 3;
	AddTblNoPC((int)pCaster->dwTblID);
	pCaster->byLevel = 15;
	pCaster->abyMagic[0] = 24;
	pCaster->wMana = 9999;
	pCaster->byCharState = CHARSTATE_MOVE;
	pCaster->byTownWarpSel = 0;
	s_dwFakeNow += 5000;
	g_dwTime = s_dwFakeNow;
	MagicProc(pCaster->dwTblID, 24, (DWORD)-1);
	PCHARINFO pFloor = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType == NPCTYPE_USEMAGIC && p->dwParentTblID == pCaster->dwTblID && p->pProc && p->dwTimeDelete) {
			pFloor = p;
		}
	}
	if (pFloor == NULL) {
		printf("tailitem b no floor\n");
	} else {
		/* A が床の上・B と C はその後ろ（床の外）に並ぶ */
		setLine(0, pFloor->dwX, pFloor->dwY);
		DeleteTblNoPC((int)apChar[0]->dwTblID);
		apChar[0]->dwX = apChar[0]->dwXBack = pFloor->dwX;
		apChar[0]->dwY = apChar[0]->dwYBack = pFloor->dwY;
		AddTblNoPC((int)apChar[0]->dwTblID);
		for (int i = 1; i < 3; i ++) {
			DeleteTblNoPC((int)apChar[i]->dwTblID);
			apChar[i]->dwX = apChar[i]->dwXBack = dwCastX;
			apChar[i]->dwY = apChar[i]->dwYBack = dwCastY + i;
			AddTblNoPC((int)apChar[i]->dwTblID);
		}
		apChar[0]->dwLastDotMove = s_dwFakeNow;
		MockTrace(1);
		for (int k = 0; k < 8 && apChar[0]->dwMapID == 0; k ++) {
			s_dwFakeNow += 300;
			g_dwTime = s_dwFakeNow;
			apChar[0]->dwLastDotMove = s_dwFakeNow;
			((PCCharProcBase)pFloor->pProc)->Proc(s_dwFakeNow);
		}
		MockTrace(0);
		TailItemPrint("b-return", apChar, 0);
		/* 床を片付ける */
		pFloor->dwTimeDelete = 0;
		pFloor->dwTimeLogin = 0;
		pFloor->dwLastDotMove = 0;
		s_dwFakeNow += 300;
		((PCCharProcBase)pFloor->pProc)->Proc(s_dwFakeNow);
	}

	/* 元に戻す */
	DeleteTblNoPC((int)pCaster->dwTblID);
	pCaster->dwMapID = dwMapBack;
	pCaster->dwX = dwXBack;
	pCaster->dwY = dwYBack;
	AddTblNoPC((int)pCaster->dwTblID);
	pCaster->byLevel = byLevelBack;
	pCaster->byDirection = byDirBack;
	pCaster->wMana = wManaBack;
	pCaster->dwStartMapID = dwStartMapBack;
	pCaster->dwStartX = dwStartXBack;
	pCaster->dwStartY = dwStartYBack;
	memcpy(pCaster->abyMagic, abyMagicBack, sizeof(abyMagicBack));
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		AddTblNoPC((int)p->dwTblID);
	}
	s_dwFakeNow = dwNowBack;
	g_dwTime = dwTimeBack;
}

/* ------------------------------------------------------------------------- */
/* U. 管理者ウィンドウからギルドの依頼を直す（CMD_ADMINQUEST）				 */
/*    一覧が 1 件ずつ届くか・直す／足すがデータAPIと同じ確認を通るか		 */
/*    （マップの外・番号の範囲外は断って中身を変えない、足すと番号が振られる） */
/* ------------------------------------------------------------------------- */
static void TestRecv(PCHARINFO pChar, const void *pData, DWORD dwSize);
static BOOL AdminQuestSend(PCHARINFO p, BYTE bySub, int nNo, const QUESTINFO *pInfo, CMDPARA_ADMINQUEST &Res, DWORD dwSize = sizeof(CMDPARA_ADMINQUEST))
{
	PCMDPARA_ADMINQUEST pPara = new CMDPARA_ADMINQUEST;
	std::vector<BYTE> aCap;
	BOOL bRet;

	ZeroMemory(pPara, sizeof(*pPara));
	pPara->byCmd = CMD_ADMINQUEST;
	pPara->byCmdSub = bySub;
	pPara->nNo = nNo;
	if (pInfo) {
		pPara->Info = *pInfo;
	}
	SvrSendCaptureBegin();
	TestRecv(p, pPara, dwSize);
	bRet = SvrSendCaptureEnd(aCap);
	delete pPara;
	ZeroMemory(&Res, sizeof(Res));
	if (!bRet || aCap.size() != sizeof(CMDPARA_ADMINQUEST)) {
		return FALSE;
	}
	memcpy(&Res, &aCap[0], sizeof(Res));
	return TRUE;
}

static void TestAdminQuest(const std::vector<int> &aLogin)
{
	printf("== U. AdminQuest ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pAdmin = g_aCharInfo[aLogin[0]];
	BYTE byAdminBack = pAdmin->byAdminLV;
	PMAPINFO pMap = g_aMapInfo[pAdmin->dwMapID];
	CMDPARA_ADMINQUEST Res;
	BOOL bGot;

	pAdmin->byAdminLV = ADMINLV_ADMINISTRATOR;

	/* 依頼 1 件（見えるNPC 1 つ） */
	g_aQuestInfo.RemoveAll();
	{
		PQUESTINFO p = new QUESTINFO;
		ZeroMemory(p, sizeof(*p));
		p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
		p->wID = 1; p->byState = QUESTSTATE_ON; p->byStepCount = 2;
		strcpy(p->szName, "AdminQuest1");
		strcpy(p->aszGoal[0], "goal0");
		strcpy(p->aszGoal[1], "goal1");
		for (int i = 0; i < MAX_QUESTNPC; i ++) {
			p->aNpc[i].byStepMax = QUESTNPC_NOSTEP;
			p->aNpc[i].byNextStep = QUESTNPC_NOSTEP;
		}
		p->aNpc[0].byKind = QUESTNPCKIND_PERSON;
		p->aNpc[0].dwMapID = pMap->dwID;
		p->aNpc[0].byX = 2; p->aNpc[0].byY = 3;
		strcpy(p->aNpc[0].szName, "npc");
		g_aQuestInfo.Add(p);
	}

	/* 一覧（最初の 1 件） */
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_LISTREQ, 0, NULL, Res);
	printf("adminquest list got=%d sub=%d count=%d no=%d id=%d name=%s npc0=%d,%d\n", bGot, Res.byCmdSub, Res.wCount, Res.nNo,
			Res.Info.wID, Res.Info.szName, Res.Info.aNpc[0].byX, Res.Info.aNpc[0].byY);

	/* 直す（名前・お金・NPC の場所。番号は送ってきた値ではなく元のまま） */
	QUESTINFO Info = *g_aQuestInfo[0];
	strcpy(Info.szName, "AdminQuest1b");
	Info.dwGold = 123;
	Info.aNpc[0].byX = 4;
	Info.wID = 77;
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, 0, &Info, Res);
	printf("adminquest edit got=%d sub=%d result=%d no=%d msg=%d | id=%d name=%s gold=%lu npc0x=%d goals=%s,%s count=%d\n", bGot, Res.byCmdSub,
			Res.byResult, Res.nNo, (int)strlen(Res.szMsg), g_aQuestInfo[0]->wID, g_aQuestInfo[0]->szName, (unsigned long)g_aQuestInfo[0]->dwGold,
			g_aQuestInfo[0]->aNpc[0].byX, g_aQuestInfo[0]->aszGoal[0], g_aQuestInfo[0]->aszGoal[1], g_aQuestInfo.GetSize());

	/* マップの外（断って中身を変えない） */
	Info = *g_aQuestInfo[0];
	Info.aNpc[0].byX = (BYTE)min(255, pMap->nCX);
	Info.dwGold = 999;
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, 0, &Info, Res);
	printf("adminquest outside got=%d result=%d msg_byX=%d | gold=%lu npc0x=%d\n", bGot, Res.byResult,
			(strstr(Res.szMsg, "byX") != NULL) ? 1 : 0, (unsigned long)g_aQuestInfo[0]->dwGold, g_aQuestInfo[0]->aNpc[0].byX);

	/* 討伐チャレンジでない依頼に入口は置けない */
	Info = *g_aQuestInfo[0];
	Info.aNpc[0].byKind = QUESTNPCKIND_GATE;
	Info.aNpc[0].dwDestMapID = pMap->dwID;
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, 0, &Info, Res);
	printf("adminquest gate-in-normal got=%d result=%d | kind=%d\n", bGot, Res.byResult, g_aQuestInfo[0]->aNpc[0].byKind);

	/* 足す（番号は空いている一番小さい 2 が振られる） */
	Info = *g_aQuestInfo[0];
	strcpy(Info.szName, "AdminQuest2");
	Info.wID = 0;
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, -1, &Info, Res);
	printf("adminquest add got=%d result=%d no=%d count=%d | id=%d name=%s\n", bGot, Res.byResult, Res.nNo, Res.wCount,
			(g_aQuestInfo.GetSize() > 1) ? g_aQuestInfo[1]->wID : -1, (g_aQuestInfo.GetSize() > 1) ? g_aQuestInfo[1]->szName : "-");

	/* 範囲外の番号・名前なし・短いパケット */
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, 5, &Info, Res);
	printf("adminquest badno got=%d result=%d count=%d\n", bGot, Res.byResult, g_aQuestInfo.GetSize());
	Info.szName[0] = 0;
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_SET, 1, &Info, Res);
	printf("adminquest noname got=%d result=%d name=%s\n", bGot, Res.byResult, g_aQuestInfo[1]->szName);
	bGot = AdminQuestSend(pAdmin, ADMINQUEST_LISTREQ, 0, NULL, Res, 16);
	printf("adminquest short got=%d\n", bGot);

	pAdmin->byAdminLV = byAdminBack;
}

/* ------------------------------------------------------------------------- */
/* Y. 管理者ウィンドウの蘇生ボタン（CMD_ADMINRESURRECT）					 */
/*    倒れた人がHP満タンで起き上がるか・蘇生の特技と同じエフェクトが出るか	 */
/*    管理者でない人・短いパケットは弾くか・倒れていない人はHPだけ満タン	 */
/* ------------------------------------------------------------------------- */
/* Z. キャラのフラグと会話データ											 */
/*    印・数値の読み書き（範囲の外・255 で止まる）・会話の行の選び方		 */
/*    （順位・必要な印・いけない印・数値の条件・レベル・同順位のくじ）		 */
/*    話した後の印・数値の動き												 */
/* ------------------------------------------------------------------------- */
static void TalkTestLineInit(TALKLINEINFO *pLine, int nRank)
{
	int i;

	ZeroMemory(pLine, sizeof(*pLine));
	pLine->byRank = (BYTE)nRank;
	for (i = 0; i < MAX_TALKNEED; i ++) {
		pLine->awNeed[i] = FLAG_NOBIT;
		pLine->awNot[i] = FLAG_NOBIT;
	}
	for (i = 0; i < MAX_TALKCOND; i ++) {
		pLine->abyCondNo[i] = FLAG_NONUM;
	}
	for (i = 0; i < MAX_TALKSET; i ++) {
		pLine->awSetOn[i] = FLAG_NOBIT;
		pLine->awSetOff[i] = FLAG_NOBIT;
	}
	for (i = 0; i < MAX_TALKACT; i ++) {
		pLine->abyActNo[i] = FLAG_NONUM;
	}
	pLine->byQuestStepMax = QUESTNPC_NOSTEP;
}

static void TestFlagTalk(const std::vector<int> &aLogin)
{
	printf("== Z. FlagTalk ==\n");
	(void)aLogin;
	static TALKINFO Talk;
	BYTE abyFlag[MAX_FLAGBITBYTE], abyNum[MAX_FLAGNUM];

	ZeroMemory(abyFlag, sizeof(abyFlag));
	ZeroMemory(abyNum, sizeof(abyNum));

	/* 印 */
	FlagBitSet(abyFlag, 5, TRUE);
	FlagBitSet(abyFlag, 1023, TRUE);
	FlagBitSet(abyFlag, 1024, TRUE);
	printf("flagtalk bit 5=%d 1023=%d 1024=%d 6=%d\n", FlagBitGet(abyFlag, 5) ? 1 : 0, FlagBitGet(abyFlag, 1023) ? 1 : 0,
			FlagBitGet(abyFlag, 1024) ? 1 : 0, FlagBitGet(abyFlag, 6) ? 1 : 0);
	FlagBitSet(abyFlag, 5, FALSE);
	printf("flagtalk bit off 5=%d 1023=%d\n", FlagBitGet(abyFlag, 5) ? 1 : 0, FlagBitGet(abyFlag, 1023) ? 1 : 0);

	/* 数値 */
	FlagNumSet(abyNum, 3, 300);
	printf("flagtalk num clamp=%d", FlagNumGet(abyNum, 3));
	FlagNumAdd(abyNum, 3, 10);
	printf(" add=%d", FlagNumGet(abyNum, 3));
	FlagNumSet(abyNum, 4, 7);
	FlagNumAdd(abyNum, 4, 1);
	printf(" four=%d", FlagNumGet(abyNum, 4));
	FlagNumSet(abyNum, MAX_FLAGNUM, 9);
	printf(" out=%d\n", FlagNumGet(abyNum, MAX_FLAGNUM));

	/* 行の選び方 */
	ZeroMemory(&Talk, sizeof(Talk));
	Talk.byState = TALKSTATE_ON;
	Talk.wID = 1;
	Talk.byLineCount = 7;
	TalkTestLineInit(&Talk.aLine[0], 1);
	Talk.aLine[0].awNeed[0] = 0;							/* 印 0 が必要（0 番も条件に使える） */
	TalkTestLineInit(&Talk.aLine[1], 2);
	Talk.aLine[1].awNeed[0] = 10;
	Talk.aLine[1].awNot[0] = 11;
	TalkTestLineInit(&Talk.aLine[2], 3);
	Talk.aLine[2].abyCondNo[0] = 2;
	Talk.aLine[2].abyCondCmp[0] = TALKCMP_GE;
	Talk.aLine[2].abyCondVal[0] = 5;
	TalkTestLineInit(&Talk.aLine[3], 3);
	Talk.aLine[3].abyCondNo[0] = 2;
	Talk.aLine[3].abyCondCmp[0] = TALKCMP_EQ;
	Talk.aLine[3].abyCondVal[0] = 2;
	TalkTestLineInit(&Talk.aLine[4], 4);
	Talk.aLine[4].byLvMin = 10;
	Talk.aLine[4].byLvMax = 20;
	TalkTestLineInit(&Talk.aLine[5], 9);
	TalkTestLineInit(&Talk.aLine[6], 9);

	ZeroMemory(abyFlag, sizeof(abyFlag));
	ZeroMemory(abyNum, sizeof(abyNum));
	printf("flagtalk pick none=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagBitSet(abyFlag, 0, TRUE);
	printf(" flag0=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagBitSet(abyFlag, 0, FALSE);
	FlagBitSet(abyFlag, 10, TRUE);
	printf(" need=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagBitSet(abyFlag, 11, TRUE);
	printf(" not=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagBitSet(abyFlag, 10, FALSE);
	FlagBitSet(abyFlag, 11, FALSE);
	FlagNumSet(abyNum, 2, 5);
	printf(" ge=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagNumSet(abyNum, 2, 2);
	printf(" eq=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	FlagNumSet(abyNum, 2, 3);
	printf(" nomatch=%d\n", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	printf("flagtalk level 9=%d 10=%d 20=%d 21=%d\n", TalkPickLine(&Talk, abyFlag, abyNum, 9, 0, 0, NULL, NULL, 0), TalkPickLine(&Talk, abyFlag, abyNum, 10, 0, 0, NULL, NULL, 0),
			TalkPickLine(&Talk, abyFlag, abyNum, 20, 0, 0, NULL, NULL, 0), TalkPickLine(&Talk, abyFlag, abyNum, 21, 0, 0, NULL, NULL, 0));
	printf("flagtalk tie rand0=%d rand1=%d rand3=%d\n", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0), TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 1),
			TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 3));
	/* 合う行が無い・知らない比べ方は合わない */
	Talk.byLineCount = 1;
	TalkTestLineInit(&Talk.aLine[0], 1);
	Talk.aLine[0].abyCondNo[0] = 2;
	Talk.aLine[0].abyCondCmp[0] = 99;
	printf("flagtalk badcmp=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));
	Talk.byLineCount = 0;
	printf(" empty=%d\n", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, NULL, NULL, 0));

	/* 持っている物の条件（手持ち・大事な物の欄のどちらでも合う。0 は条件なし） */
	{
		WORD awItem[MAX_MYITEM], awImp[MAX_IMPORTANT];
		ZeroMemory(awItem, sizeof(awItem));
		ZeroMemory(awImp, sizeof(awImp));
		Talk.byLineCount = 2;
		TalkTestLineInit(&Talk.aLine[0], 1);
		Talk.aLine[0].wNeedItem = 7;
		TalkTestLineInit(&Talk.aLine[1], 2);
		printf("flagtalk needitem none=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, awItem, awImp, 0));
		awItem[MAX_MYITEM - 1] = 7;
		printf(" item=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, awItem, awImp, 0));
		awItem[MAX_MYITEM - 1] = 0;
		awImp[MAX_IMPORTANT - 1] = 7;
		printf(" imp=%d", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, awItem, awImp, 0));
		awImp[MAX_IMPORTANT - 1] = 8;
		printf(" other=%d\n", TalkPickLine(&Talk, abyFlag, abyNum, 1, 0, 0, awItem, awImp, 0));
	}

	/* 話した後の動き（立てる→消す→数値。数値は 255 で止まる） */
	TALKLINEINFO Line;
	TalkTestLineInit(&Line, 1);
	Line.awSetOn[0] = 20;
	Line.awSetOff[0] = 21;
	Line.abyActNo[0] = 5;
	Line.abyActOp[0] = TALKNUMOP_SET;
	Line.abyActVal[0] = 200;
	Line.abyActNo[1] = 5;
	Line.abyActOp[1] = TALKNUMOP_ADD;
	Line.abyActVal[1] = 100;
	ZeroMemory(abyFlag, sizeof(abyFlag));
	ZeroMemory(abyNum, sizeof(abyNum));
	FlagBitSet(abyFlag, 21, TRUE);
	TalkApplyLine(&Line, abyFlag, abyNum);
	printf("flagtalk apply on=%d off=%d num=%d other=%d\n", FlagBitGet(abyFlag, 20) ? 1 : 0, FlagBitGet(abyFlag, 21) ? 1 : 0,
			FlagNumGet(abyNum, 5), FlagNumGet(abyNum, 6));
}

/* ------------------------------------------------------------------------- */
static void TestAdminResurrect(const std::vector<int> &aLogin)
{
	printf("== Y. AdminResurrect ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PCHARINFO pAdmin = g_aCharInfo[aLogin[0]];
	PCHARINFO pTarget = g_aCharInfo[aLogin[1]];
	BYTE byAdminBack = pAdmin->byAdminLV;
	BYTE byTargetAdminBack = pTarget->byAdminLV;
	WORD wHPBack = pTarget->wHP;
	BYTE byStateBack = pTarget->byCharState;
	CMDPARA_DATA1 Para;
	std::vector<BYTE> aCap;
	BYTE byEfc = 14;
	int i, nEfc;

	/* データの蘇生の特技のエフェクト番号（サーバーと同じ探し方） */
	for (i = 0; i < g_aMagicInfo.GetSize(); i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[i];
		if (pInfo && (pInfo->byType == MAGICTYPE_RESURRECTION) && pInfo->byTargetEfectID) {
			byEfc = pInfo->byTargetEfectID;
			break;
		}
	}

	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_ADMINRESURRECT;
	Para.dwTblID = pTarget->dwTblID;

	/* 管理者でない人からは効かない */
	pAdmin->byAdminLV = ADMINLV_USER;
	pTarget->byAdminLV = ADMINLV_USER;
	pTarget->wHP = 0;
	pTarget->byCharState = CHARSTATE_DEAD;
	TestRecv(pAdmin, &Para, sizeof(Para));
	printf("adminres user hp=%d dead=%d\n", pTarget->wHP, (pTarget->byCharState == CHARSTATE_DEAD) ? 1 : 0);

	pAdmin->byAdminLV = ADMINLV_ADMINISTRATOR;
	/* 短いパケットは無視 */
	TestRecv(pAdmin, &Para, 8);
	printf("adminres short hp=%d dead=%d\n", pTarget->wHP, (pTarget->byCharState == CHARSTATE_DEAD) ? 1 : 0);

	/* 倒れた人を蘇生（HP満タン・エフェクトは蘇生の特技のもの。最初に送るのがエフェクト） */
	SvrSendCaptureBegin();
	TestRecv(pAdmin, &Para, sizeof(Para));
	SvrSendCaptureEnd(aCap);
	nEfc = -1;
	if ((aCap.size() == sizeof(CMDPARA_DATA4)) && (aCap[0] == CMD_EFECTCHG)) {
		nEfc = ((PCMDPARA_DATA4)&aCap[0])->byData;
	}
	printf("adminres dead full=%d dead=%d efc_ok=%d efc=%d\n", (pTarget->wHP == pTarget->wMaxHP) ? 1 : 0,
			(pTarget->byCharState == CHARSTATE_DEAD) ? 1 : 0, (nEfc == byEfc) ? 1 : 0, nEfc);

	/* 倒れていない人はHPだけ満タン */
	pTarget->wHP = 1;
	TestRecv(pAdmin, &Para, sizeof(Para));
	printf("adminres alive full=%d dead=%d\n", (pTarget->wHP == pTarget->wMaxHP) ? 1 : 0, (pTarget->byCharState == CHARSTATE_DEAD) ? 1 : 0);

	/* 範囲外の番号は無視 */
	Para.dwTblID = (DWORD)g_aCharInfo.GetSize();
	TestRecv(pAdmin, &Para, sizeof(Para));
	printf("adminres badno ok\n");

	pTarget->wHP = wHPBack;
	pTarget->byCharState = byStateBack;
	pTarget->byAdminLV = byTargetAdminBack;
	pAdmin->byAdminLV = byAdminBack;
}

/* ------------------------------------------------------------------------- */
/* S. 前方・周囲の特技が壁越しに届かない									 */
/*    敵の1マス手前を壁にして、2マス先から前方・周囲の特技を使う。			 */
/*    壁が無ければ当たり、壁があれば当たらないこと。置く魔法も壁の向こうに出ないこと */
/*    回復は壁越しでも効くこと											 */
/* ------------------------------------------------------------------------- */
extern void DeleteChar(DWORD dwTblID);
static void TestMagicWall(const std::vector<int> &aLogin)
{
	printf("== S. MagicWall ==\n");
	if (aLogin.empty()) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	/* 壁にするパーツ */
	int nWall = -1;
	for (int i = 0; i < g_wCountMapParts; i ++) {
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) {
			nWall = i;
			break;
		}
	}
	/* 左に2マス空いている戦闘用の敵を探す */
	PCHARINFO pEnemy = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pEnemy; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->wGrpMain != CHARGRP_MAIN_NORMAL || (g_aMapInfo[p->dwMapID]->byFlg & MAPFLG_MAGIC)) {
			continue;
		}
		if (p->byNpcType != NPCTYPE_BATTLE1 && p->byNpcType != NPCTYPE_BATTLE2) {
			continue;
		}
		if (p->dwX < 3 || p->dwY < 1) {
			continue;
		}
		if (CheckCharBlock(p->dwMapID, p->dwX - 1, p->dwY) && CheckCharBlock(p->dwMapID, p->dwX - 2, p->dwY)) {
			pEnemy = p;
		}
	}
	if (!pEnemy || nWall < 0) {
		printf("no enemy\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[pEnemy->dwMapID];
	DWORD dwPCMap = pPC->dwMapID;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = pEnemy->dwMapID;
	AddTblNoPC((int)pPC->dwTblID);
	DWORD dwEnemyX = pEnemy->dwX, dwEnemyY = pEnemy->dwY;
	WORD wHPBack = pEnemy->wHP, wMaxHPBack = pEnemy->wMaxHP;
	DWORD dwPCX = pPC->dwX, dwPCY = pPC->dwY;
	BYTE byDirBack = pPC->byDirection, abyMagicBack[MAX_USEMAGIC];
	memcpy(abyMagicBack, pPC->abyMagic, sizeof(abyMagicBack));
	int nWallPos = (int)(dwEnemyX - 1) + (int)dwEnemyY * pMap->nCX;
	WORD wPartsBack = pMap->pwMap[nWallPos];

	/* 十字突（前方2マス）・ブレイズフィールド（周囲2マス）・広がる火炎（周囲3マスに置く） */
	static const BYTE abyMagic[] = {55, 50, 35};
	for (size_t m = 0; m < sizeof(abyMagic) / sizeof(abyMagic[0]); m ++) {
		BYTE byID = abyMagic[m];
		if (byID >= g_aMagicInfo.GetSize()) {
			continue;
		}
		for (int nWallOn = 0; nWallOn < 2; nWallOn ++) {
			pMap->pwMap[nWallPos] = nWallOn ? (WORD)nWall : wPartsBack;
			int nHit = 0, nTry = 20, nBeyond = 0;
			for (int t = 0; t < nTry; t ++) {
				pPC->dwX = dwEnemyX - 2;
				pPC->dwY = dwEnemyY;
				pPC->byDirection = 3;
				pPC->abyMagic[0] = byID;
				pPC->wMana = 9999;
				pEnemy->dwX = dwEnemyX;
				pEnemy->dwY = dwEnemyY;
				pEnemy->wMaxHP = pEnemy->wHP = 30000;
				MagicProc(pPC->dwTblID, byID, (DWORD)-1);
				if (pEnemy->wHP < 30000) {
					nHit ++;
				}
				/* 置いた魔法が敵のマスにあるか数えてから消す */
				for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
					PCHARINFO p = g_aCharInfo[i];
					if ((p->byNpcType != NPCTYPE_USEMAGIC && p->byNpcType != NPCTYPE_USEMAGICPARENT) ||
						p->dwParentTblID != pPC->dwTblID || p->pProc == NULL) {
						continue;
					}
					if (p->dwX == dwEnemyX && p->dwY == dwEnemyY) {
						nBeyond ++;
					}
					DeleteChar(p->dwTblID);
				}
			}
			printf("magic=%d wall=%d hit=%d/%d put-on-enemy=%d\n", byID, nWallOn, nHit, nTry, nBeyond);
		}
	}
	/* 回復（周囲2マス）は壁越しでも仲間に効く */
	if (aLogin.size() > 1 && 12 < g_aMagicInfo.GetSize()) {
		PCHARINFO pPC2 = g_aCharInfo[aLogin[1]];
		DWORD dwPC2Map = pPC2->dwMapID, dwPC2X = pPC2->dwX, dwPC2Y = pPC2->dwY;
		WORD wPC2HP = pPC2->wHP, wPC2MaxHP = pPC2->wMaxHP;
		DeleteTblNoPC((int)pPC2->dwTblID);
		pPC2->dwMapID = pEnemy->dwMapID;
		AddTblNoPC((int)pPC2->dwTblID);
		for (int nWallOn = 0; nWallOn < 2; nWallOn ++) {
			pMap->pwMap[nWallPos] = nWallOn ? (WORD)nWall : wPartsBack;
			pPC->dwX = dwEnemyX - 2;
			pPC->dwY = dwEnemyY;
			pPC->byDirection = 3;
			pPC->abyMagic[0] = 12;
			pPC->wMana = 9999;
			pPC2->dwX = dwEnemyX;
			pPC2->dwY = dwEnemyY;
			pPC2->wMaxHP = 30000;
			pPC2->wHP = 1;
			MagicProc(pPC->dwTblID, 12, (DWORD)-1);
			printf("heal wall=%d healed=%d\n", nWallOn, pPC2->wHP > 1 ? 1 : 0);
		}
		DeleteTblNoPC((int)pPC2->dwTblID);
		pPC2->dwMapID = dwPC2Map;
		AddTblNoPC((int)pPC2->dwTblID);
		pPC2->dwX = dwPC2X;
		pPC2->dwY = dwPC2Y;
		pPC2->wHP = wPC2HP;
		pPC2->wMaxHP = wPC2MaxHP;
	}
	pMap->pwMap[nWallPos] = wPartsBack;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = dwPCMap;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->dwX = dwPCX;
	pPC->dwY = dwPCY;
	pPC->byDirection = byDirBack;
	memcpy(pPC->abyMagic, abyMagicBack, sizeof(abyMagicBack));
	pEnemy->dwX = dwEnemyX;
	pEnemy->dwY = dwEnemyY;
	pEnemy->wHP = wHPBack;
	pEnemy->wMaxHP = wMaxHPBack;
}

/* ------------------------------------------------------------------------- */
/* J. 船で目覚めるベッドがばらける（マップ間移動のばらける範囲）			 */
/* ------------------------------------------------------------------------- */
extern void EventProc(PCHARINFO pChar, PEVENTINFO pEvent, BYTE byData);
extern int  GetMapNo(DWORD dwID);

static void TestShipBed(const std::vector<int> &aLogin)
{
	printf("== J. Ship bed ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	/* 港町レジェンタ(14)の乗り場 (40,54) → ルシー行きの船(40) */
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	PEVENTINFO pEvent = NULL;
	for (int i = 0; i < pPort->nEventCount; i ++) {
		PEVENTINFO p = &pPort->pEvent[i];
		if (p->byType == EVENTTYPE_MAPCHG && p->pData && p->dwX == 40 && p->dwY == 54) {
			pEvent = p;
		}
	}
	if (!pEvent) {
		printf("no event\n");
		return;
	}
	PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
	PMAPINFO pShip = g_aMapInfo[GetMapNo(pInfo->dwMapID)];
	WORD wBed = pShip->pwMap[pInfo->dwX + pInfo->dwY * pShip->nCX];
	printf("ship map=%u dest=(%u,%u) parts=%u\n", pInfo->dwMapID, pInfo->dwX, pInfo->dwY, wBed);

	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	PCHARINFO pFriend = g_aCharInfo[aLogin[1]];
	DWORD dwMapBack = pPC->dwMapID, dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	DWORD dwFMapBack = pFriend->dwMapID, dwFXBack = pFriend->dwX, dwFYBack = pFriend->dwY;
	BYTE byRangeBack = pInfo->byRandomRange;

	/* 乗り場へ戻す */
	struct Local {
		static void Put(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY) {
			DeleteTblNoPC((int)pChar->dwTblID);
			pChar->dwMapID = dwMapID;
			pChar->dwX = dwX;
			pChar->dwY = dwY;
			AddTblNoPC((int)pChar->dwTblID);
		}
	};
	/* 0=いつも同じ、16=客室のベッド全部、3=移動先の近くだけ、16+仲間が寝ているベッドは避ける */
	struct { BYTE byRange; BOOL bFriendOnBed; } aCase[] = { {0, FALSE}, {16, FALSE}, {3, FALSE}, {16, TRUE} };
	for (size_t c = 0; c < sizeof(aCase) / sizeof(aCase[0]); c ++) {
		pInfo->byRandomRange = aCase[c].byRange;
		DWORD dwFX = pInfo->dwX, dwFY = pInfo->dwY;
		if (aCase[c].bFriendOnBed) {
			Local::Put(pFriend, pShip->dwID, dwFX, dwFY);
		} else {
			Local::Put(pFriend, dwFMapBack, dwFXBack, dwFYBack);
		}
		std::set<std::pair<DWORD, DWORD> > aSeen;
		int nBad = 0, nOnFriend = 0, nTry = 200;
		for (int t = 0; t < nTry; t ++) {
			Local::Put(pPC, pPort->dwID, pEvent->dwX, pEvent->dwY);
			EventProc(pPC, pEvent, 0);
			DWORD x = pPC->dwX, y = pPC->dwY;
			int dx = (int)x - (int)pInfo->dwX, dy = (int)y - (int)pInfo->dwY;
			int nRange = aCase[c].byRange;
			if (pPC->dwMapID != pShip->dwID || pShip->pwMap[x + y * pShip->nCX] != wBed ||
				dx < -nRange || dx > nRange || dy < -nRange || dy > nRange) {
				nBad ++;
			}
			if (aCase[c].bFriendOnBed && x == dwFX && y == dwFY) {
				nOnFriend ++;
			}
			aSeen.insert(std::make_pair(x, y));
		}
		printf("shipbed range=%u friend=%d beds=%d bad=%d onfriend=%d list=", aCase[c].byRange, aCase[c].bFriendOnBed,
			(int)aSeen.size(), nBad, nOnFriend);
		int n = 0;
		for (std::set<std::pair<DWORD, DWORD> >::iterator it = aSeen.begin(); it != aSeen.end(); ++ it) {
			printf("%s(%u,%u)", n ++ ? "," : "", it->first, it->second);
		}
		printf("\n");
	}
	pInfo->byRandomRange = byRangeBack;
	Local::Put(pPC, dwMapBack, dwXBack, dwYBack);
	Local::Put(pFriend, dwFMapBack, dwFXBack, dwFYBack);
}

/* ------------------------------------------------------------------------- */
/* K. 傭兵：道さがしの表（前の1回ずつの道さがしと同じ結果か・速さ）と、	 */
/*    道具を持たせる取り引き・自分で回復・帰る時に返す						 */
/* ------------------------------------------------------------------------- */
extern void RecvDataProc(PBYTE pData, DWORD dwDataSize);

/* 前の道さがし（目標のとなりに着くまで毎回幅優先）。比べるための写し */
static int RefPathDir(CCharProcMERCENARY *pProc, PCHARINFO pChar, int nX, int nY, int nMaxDist, int *pnDist)
{
	int nSX = (int)pChar->dwX, nSY = (int)pChar->dwY;
	if (abs(nX - nSX) + abs(nY - nSY) <= 1) {
		if (pnDist) *pnDist = 0;
		return -2;
	}
	if (abs(nX - nSX) + abs(nY - nSY) > nMaxDist + 1) {
		return -1;
	}
	int nW = nMaxDist * 2 + 1, nOX = nSX - nMaxDist, nOY = nSY - nMaxDist;
	std::vector<short> anFirst(nW * nW, -1), anDist(nW * nW, 0);
	std::vector<int> anQueue(nW * nW);
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];
	PDWORD apTbl[2] = {pMap->paTblNoPC, pMap->paTblNoNPC};
	DWORD adwCount[2] = {pMap->dwCountTblNoPC, pMap->dwCountTblNoNPC};
	for (int t = 0; t < 2; t ++) {
		if (apTbl[t] == NULL) continue;
		for (DWORD j = 0; j < adwCount[t]; j ++) {
			if (apTbl[t][j] == (DWORD)-1) break;
			PCHARINFO p = g_aCharInfo[apTbl[t][j]];
			if (p == pChar || p->byNpcType == NPCTYPE_NOBLOCK) continue;
			int x = (int)p->dwX - nOX, y = (int)p->dwY - nOY;
			if (x >= 0 && x < nW && y >= 0 && y < nW) anFirst[y * nW + x] = -3;
		}
	}
	int nHead = 0, nTail = 0, nPos = (nSY - nOY) * nW + (nSX - nOX);
	anFirst[nPos] = 4;
	anQueue[nTail ++] = nPos;
	while (nHead < nTail) {
		nPos = anQueue[nHead ++];
		if (anDist[nPos] >= nMaxDist) continue;
		for (int nDir = 0; nDir < 4; nDir ++) {
			int x = nOX + (nPos % nW) + anPosAddTblX[nDir];
			int y = nOY + (nPos / nW) + anPosAddTblY[nDir];
			if (x < nOX || x >= nOX + nW || y < nOY || y >= nOY + nW) continue;
			int nNext = (y - nOY) * nW + (x - nOX);
			if (anFirst[nNext] != -1) continue;
			if (pProc->CanWalk(x, y, TRUE) == FALSE) continue;
			anFirst[nNext] = (anFirst[nPos] == 4) ? nDir : anFirst[nPos];
			anDist[nNext] = anDist[nPos] + 1;
			if (abs(nX - x) + abs(nY - y) == 1) {
				if (pnDist) *pnDist = anDist[nNext];
				return anFirst[nNext];
			}
			anQueue[nTail ++] = nNext;
		}
	}
	return -1;
}

/* 連打の間隔・発言の数えを、受信のたびに空にするか（既存のテストは時刻が固定で、続けて送るため。連打のテストだけ FALSE にする） */
static BOOL s_bTestRateReset = TRUE;

/* 受信したことにして処理する（送ってきた接続のキャラとして） */
/* 状態の控えは数値欄だけを使う。タイマー配列の所有権を共有して二重解放しない */
static void TestCharBackup(PCHARINFO pBack, PCHARINFO pChar)
{
	*pBack = *pChar;
	CmyArray<PCHARINFOTIMEREVENT, PCHARINFOTIMEREVENT> Empty;
	pBack->aTimerEvent = Empty;
}

static void TestRecv(PCHARINFO pChar, const void *pData, DWORD dwSize)
{
	if (s_bTestRateReset) {
		ZeroMemory(pChar->adwRateLast, sizeof(pChar->adwRateLast));
		pChar->bySpeechWinCount = 0;
	}
	CCRC Crc;
	URARASOCK_PACKETINFO Info;
	Info.dwSize = dwSize;
	Info.dwCRC = Crc.GetCRC((PBYTE)pData, dwSize);
	std::vector<BYTE> a(sizeof(Info) + dwSize);
	memcpy(&a[0], &Info, sizeof(Info));
	memcpy(&a[sizeof(Info)], pData, dwSize);
	SboSockOnData((int)(pChar->pID - URARASOCK_IDBASE), &a[0], (int)a.size());
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		DispatchMessage(&msg);
	}
}

/* 店・預かり所・転職所・着せ替えの人（台詞の印 "@6" "@7" "@5" "@a" "@"）を、p と同じ場所に置く／片付ける。
   サーバーは、これらの人が近くにいる時だけ買う・売る・預ける・転職・着せ替えを受ける */
static std::vector<PCHARINFO> s_aNearNpc;
static void NearNpcOpen(PCHARINFO p, const char *pszSpeech, WORD wGrp = 0)
{
	PCHARINFO pNpc = new CHARINFO;
	ZeroMemory((PBYTE)pNpc, sizeof(CHARINFO));
	pNpc->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pNpc);
	pNpc->byGameState = GAMESTATE_NPC;
	pNpc->byCharState = CHARSTATE_MOVE;
	pNpc->dwMapID = p->dwMapID;
	pNpc->dwX = pNpc->dwXBack = pNpc->dwStartX = p->dwX;
	pNpc->dwY = pNpc->dwYBack = pNpc->dwStartY = p->dwY;
	pNpc->dwSpaceID = p->dwSpaceID;
	pNpc->wGrpID = wGrp;
	strcpy(pNpc->szName, "窓口");
	pNpc->pszNpcSpeech = new char[strlen(pszSpeech) + 1];
	strcpy(pNpc->pszNpcSpeech, pszSpeech);
	pNpc = AddChar(pNpc);
	s_aNearNpc.push_back(pNpc);
}
static void NearNpcOpen(PCHARINFO p)
{
	NearNpcOpen(p, "@6");
	NearNpcOpen(p, "@5");
}
static void NearNpcClose(void)
{
	/* 後から作った順に消す（番号がずれないように最後の要素だけ） */
	while (!s_aNearNpc.empty()) {
		PCHARINFO pNpc = s_aNearNpc.back();
		if (g_aCharInfo.GetSize() == 0 || g_aCharInfo[g_aCharInfo.GetSize() - 1] != pNpc) {
			break;
		}
		g_aCharInfo.RemoveAt(g_aCharInfo.GetSize() - 1);
		delete [] pNpc->pszNpcSpeech;
		delete pNpc;
		s_aNearNpc.pop_back();
	}
}

static BYTE ItemSum(PCHARINFO p)
{
	BYTE by = (BYTE)p->dwGold;
	for (int i = 0; i < MAX_MYITEM; i ++) by += (BYTE)p->waItem[i];
	return by;
}

/* 取り引きを最後まで進める（傭兵に waGive を渡す・お金 dwGold） */
static void TestTradeWithMerc(PCHARINFO pOwner, PCHARINFO pMerc, const std::vector<WORD> &aGive, DWORD dwGold)
{
	CMDPARA_DATA3 Trade;
	ZeroMemory(&Trade, sizeof(Trade));
	Trade.byCmd = CMD_TRADE;
	Trade.pID = pOwner->pID;
	Trade.dwTblID = pOwner->dwTblID;
	Trade.dwData = pMerc->dwTblID;
	TestRecv(pOwner, &Trade, sizeof(Trade));

	CMDPARA_TRADEINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.byCmd = CMD_TRADEINFO;
	Info.byMode = 1;
	Info.dwTblID = pMerc->dwTblID;
	Info.dwGold = dwGold;
	for (int i = 0; i < MAX_MYITEM; i ++) Info.waItem[i] = 0xFFFF;
	for (size_t i = 0; i < aGive.size(); i ++) Info.waItem[i] = aGive[i];
	Info.bySum = ItemSum(pOwner);
	TestRecv(pOwner, &Info, sizeof(Info));

	/* 傭兵は持ち物を全部返そうとする */
	CMDPARA_TRADEPROC Proc;
	ZeroMemory(&Proc, sizeof(Proc));
	Proc.byCmd = CMD_TRADEPROC;
	Proc.bySum1 = ItemSum(pOwner);
	Proc.bySum2 = ItemSum(pMerc);
	Proc.dwTblID1 = pOwner->dwTblID;
	Proc.dwTblID2 = pMerc->dwTblID;
	Proc.dwGold1 = dwGold;
	for (int i = 0; i < MAX_MYITEM; i ++) {
		Proc.waItem1[i] = Info.waItem[i];
		Proc.waItem2[i] = 0xFFFF;
	}
	for (int i = 0, n = 0; i < MAX_MYITEM; i ++) {
		if (pMerc->waItem[i]) Proc.waItem2[n ++] = (WORD)i;
	}
	TestRecv(pOwner, &Proc, sizeof(Proc));
}

static void PrintItems(const char *pszLabel, PCHARINFO p)
{
	printf("%s", pszLabel);
	for (int i = 0; i < MAX_MYITEM; i ++) printf(" %u", p->waItem[i]);
	printf("\n");
}

/* ------------------------------------------------------------------------- */
/* K2. 傭兵への特技：プレイヤーの回復・蘇生の特技が傭兵にも効く			 */
/*     （攻撃の特技は当たらない）。ゲームの乱数を使うので最後に流す		 */
/* ------------------------------------------------------------------------- */
static void TestMercMagicBody(PCHARINFO pOwner, PCHARINFO pMerc, CCharProcMERCENARY *pProc, PMAPINFO pMap)
{
	int nHealSel = -1, nHealCircle = -1, nResSel = -1, nResScreen = -1, nAtkCircle = -1;
	for (int i = 1; i < g_aMagicInfo.GetSize(); i ++) {
		PMAGICINFO pM = g_aMagicInfo[i];
		if (pM == NULL) continue;
		if (pM->byType == MAGICTYPE_HPUP && pM->byTarget != MAGICTARGET_NPC) {
			if (nHealSel < 0 && (pM->byTargetType == MAGICTARGETTYPE_SELECT || pM->byTargetType == MAGICTARGETTYPE_SELECT2)) nHealSel = i;
			if (nHealCircle < 0 && pM->byTargetType == MAGICTARGETTYPE_CIRCLE && pM->byTargetArea >= 1) nHealCircle = i;
		}
		if (pM->byType == MAGICTYPE_RESURRECTION && pM->byTarget != MAGICTARGET_NPC) {
			if (nResSel < 0 && (pM->byTargetType == MAGICTARGETTYPE_SELECT || pM->byTargetType == MAGICTARGETTYPE_SELECT2)) nResSel = i;
			if (nResScreen < 0 && pM->byTargetType == MAGICTARGETTYPE_SCREEN) nResScreen = i;
		}
		if (nAtkCircle < 0 && pM->byType == MAGICTYPE_HPDOWN && pM->byTarget == MAGICTARGET_NPC &&
			pM->byTargetType == MAGICTARGETTYPE_CIRCLE && pM->byTargetArea >= 1) nAtkCircle = i;
	}
	printf("merc magic found healsel=%d healcircle=%d ressel=%d resscreen=%d atkcircle=%d\n",
		(int)(nHealSel > 0), (int)(nHealCircle > 0), (int)(nResSel > 0), (int)(nResScreen > 0), (int)(nAtkCircle > 0));
	BYTE byMapFlgBack = pMap->byFlg;
	pMap->byFlg &= ~MAPFLG_MAGIC;
	/* 雇い主のとなりに置く */
	pMerc->dwX = pOwner->dwX + 1;
	pMerc->dwY = pOwner->dwY;
	pOwner->wHP = pOwner->wMaxHP;
	pOwner->byDirection = 3;

	/* 選んで回復 */
	pMerc->wHP = 1;
	if (nHealSel > 0) MagicProcItem(pOwner->dwTblID, (BYTE)nHealSel, pMerc->dwTblID);
	printf("merc magic healsel up=%d\n", (int)(pMerc->wHP > 1));
	/* 周りを回復 */
	pMerc->wHP = 1;
	if (nHealCircle > 0) MagicProcItem(pOwner->dwTblID, (BYTE)nHealCircle, (DWORD)-1);
	printf("merc magic healcircle up=%d\n", (int)(pMerc->wHP > 1));
	/* 周りへの攻撃は傭兵に当たらない */
	WORD wHPAtk = pMerc->wHP = pMerc->wMaxHP;
	if (nAtkCircle > 0) MagicProcItem(pOwner->dwTblID, (BYTE)nAtkCircle, (DWORD)-1);
	printf("merc magic atkcircle same=%d\n", (int)(pMerc->wHP == wHPAtk));

	/* 倒れた傭兵を選んで蘇生 → 傭兵の処理で起き上がる */
	pMerc->wHP = 0;
	MercenaryDamaged(NULL, pMerc);
	s_dwFakeNow += 100;
	g_dwTime = s_dwFakeNow;
	pProc->Proc(g_dwTime);
	if (nResSel > 0) MagicProcItem(pOwner->dwTblID, (BYTE)nResSel, pMerc->dwTblID);
	s_dwFakeNow += 100;
	g_dwTime = s_dwFakeNow;
	pProc->Proc(g_dwTime);
	printf("merc magic ressel alive=%d state=%d owner=%d\n", (int)(pMerc->wHP > 0),
		(int)(pMerc->byCharState != CHARSTATE_DEAD), (int)(GetMercenaryOwner(pMerc) == pOwner));
	/* 画面内の蘇生 */
	pMerc->wHP = 0;
	MercenaryDamaged(NULL, pMerc);
	s_dwFakeNow += 100;
	g_dwTime = s_dwFakeNow;
	pProc->Proc(g_dwTime);
	if (nResScreen > 0) MagicProcItem(pOwner->dwTblID, (BYTE)nResScreen, (DWORD)-1);
	s_dwFakeNow += 100;
	g_dwTime = s_dwFakeNow;
	pProc->Proc(g_dwTime);
	printf("merc magic resscreen alive=%d state=%d\n", (int)(pMerc->wHP > 0), (int)(pMerc->byCharState != CHARSTATE_DEAD));
	pMerc->wHP = pMerc->wMaxHP;
	pMap->byFlg = byMapFlgBack;
}

static void TestMercenary(const std::vector<int> &aLogin, BOOL bMagic)
{
	printf(bMagic ? "== K2. Mercenary magic ==\n" : "== K. Mercenary ==\n");
	if (aLogin.empty()) {
		return;
	}
	/* 敵がいちばん多い戦闘マップ */
	std::vector<int> aEnemy(g_aMapInfo.GetSize(), 0);
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if ((p->byNpcType == NPCTYPE_BATTLE1 || p->byNpcType == NPCTYPE_BATTLE2) && p->dwMapID < aEnemy.size() &&
			(g_aMapInfo[p->dwMapID]->byFlg & MAPFLG_BATTLE)) {
			aEnemy[p->dwMapID] ++;
		}
	}
	int nMap = 0;
	for (size_t i = 0; i < aEnemy.size(); i ++) {
		if (aEnemy[i] > aEnemy[nMap]) nMap = (int)i;
	}
	PMAPINFO pMap = g_aMapInfo[nMap];
	std::vector<int> aEnemyNo;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if ((p->byNpcType == NPCTYPE_BATTLE1 || p->byNpcType == NPCTYPE_BATTLE2) && p->dwMapID == (DWORD)nMap) {
			aEnemyNo.push_back(i);
		}
	}
	printf("merc map=%d enemies=%d size=%dx%d\n", nMap, (int)aEnemyNo.size(), pMap->nCX, pMap->nCY);

	PCHARINFO pOwner = g_aCharInfo[aLogin[0]];
	DWORD dwMapBack = pOwner->dwMapID, dwXBack = pOwner->dwX, dwYBack = pOwner->dwY;
	WORD waItemBack[MAX_MYITEM];
	memcpy(waItemBack, pOwner->waItem, sizeof(waItemBack));
	WORD waEquBack[10];
	memcpy(waEquBack, pOwner->waEqu, sizeof(waEquBack));
	WORD wHPBack = pOwner->wHP, wMaxHPBack = pOwner->wMaxHP;
	DWORD dwGoldBack = pOwner->dwGold;

	/* 敵のそばの歩けるマスに雇い主を置く */
	PCHARINFO pBase = g_aCharInfo[aEnemyNo[0]];
	DeleteTblNoPC((int)pOwner->dwTblID);
	pOwner->dwMapID = nMap;
	pOwner->dwX = pBase->dwX;
	pOwner->dwY = pBase->dwY + 1;
	for (int r = 1; r < 6; r ++) {
		BOOL bFound = FALSE;
		for (int dy = -r; dy <= r && !bFound; dy ++) {
			for (int dx = -r; dx <= r && !bFound; dx ++) {
				int x = (int)pBase->dwX + dx, y = (int)pBase->dwY + dy;
				if (x < 2 || y < 2 || x >= pMap->nCX - 2 || y >= pMap->nCY - 2) continue;
				if (CheckCharBlock((DWORD)nMap, x, y) && CheckCharBlock((DWORD)nMap, x + 1, y) &&
					CheckCharBlockNPC((DWORD)nMap, x, y) == (DWORD)-1 && CheckCharBlockNPC((DWORD)nMap, x + 1, y) == (DWORD)-1) {
					pOwner->dwX = x;
					pOwner->dwY = y;
					bFound = TRUE;
				}
			}
		}
		if (bFound) break;
	}
	pOwner->byDirection = 3;
	pOwner->wHP = pOwner->wMaxHP = 200;
	pOwner->byCharState = CHARSTATE_MOVE;
	AddTblNoPC((int)pOwner->dwTblID);
	MercenarySetBattleMode(pOwner, TRUE);

	/* 傭兵所の人に話しかけて雇う */
	CHARINFO Npc;
	memcpy(&Npc, pBase, sizeof(Npc));
	strcpy(Npc.szName, "ハーン");
	static char szSpeech[] = "@m";
	Npc.pszNpcSpeech = szSpeech;
	MercenaryTalk(pOwner, &Npc);
	PCHARINFO pMerc = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (GetMercenaryOwner(g_aCharInfo[i]) == pOwner) pMerc = g_aCharInfo[i];
	}
	if (!pMerc) {
		printf("merc hire failed\n");
		return;
	}
	CCharProcMERCENARY *pProc = (CCharProcMERCENARY *)pMerc->pProc;
	printf("merc hired dist=%d\n", abs((int)pMerc->dwX - (int)pOwner->dwX) + abs((int)pMerc->dwY - (int)pOwner->dwY));

	/* 特技のテストだけ（最後に流す） */
	if (bMagic) {
		MercenarySetBattleMode(pOwner, FALSE);	/* 戦わせない */
		TestMercMagicBody(pOwner, pMerc, pProc, pMap);
		MercenaryOwnerLogout(pOwner);
		DeleteTblNoPC((int)pOwner->dwTblID);
		pOwner->dwMapID = dwMapBack;
		pOwner->dwX = dwXBack;
		pOwner->dwY = dwYBack;
		memcpy(pOwner->waItem, waItemBack, sizeof(waItemBack));
		memcpy(pOwner->waEqu, waEquBack, sizeof(waEquBack));
		pOwner->wHP = wHPBack;
		pOwner->wMaxHP = wMaxHPBack;
		pOwner->dwGold = dwGoldBack;
		AddTblNoPC((int)pOwner->dwTblID);
		return;
	}

	/* 1) 道さがし：傭兵をいろいろなマスに置き、マップの敵へ行く道を前の方法と比べる */
	{
		DWORD dwMX = pMerc->dwX, dwMY = pMerc->dwY;
		int nCase = 0, nReach = 0, nDiffReach = 0, nDiffDist = 0, nBadDir = 0;
		s_dwSeed = 777;
		for (int t = 0; t < 300; t ++) {
			PCHARINFO pE = g_aCharInfo[aEnemyNo[Rnd() % aEnemyNo.size()]];
			int x = (int)pE->dwX + (int)(Rnd() % 17) - 8;
			int y = (int)pE->dwY + (int)(Rnd() % 17) - 8;
			if (x < 0 || y < 0 || x >= pMap->nCX || y >= pMap->nCY || !pProc->CanWalk(x, y)) continue;
			pMerc->dwX = x;
			pMerc->dwY = y;
			for (size_t e = 0; e < aEnemyNo.size(); e ++) {
				PCHARINFO pT = g_aCharInfo[aEnemyNo[e]];
				int nD1 = -9, nD2 = -9;
				int nR1 = RefPathDir(pProc, pMerc, (int)pT->dwX, (int)pT->dwY, 12, &nD1);
				/* 表は傭兵の位置が変われば作り直すので、ここでは消さない（同じ位置なら使い回し） */
				int nR2 = pProc->PathDir((int)pT->dwX, (int)pT->dwY, 12, &nD2, TRUE);
				nCase ++;
				if (nR1 >= 0) nReach ++;
				if ((nR1 >= 0) != (nR2 >= 0) || (nR1 == -2) != (nR2 == -2)) {
					nDiffReach ++;
				} else if (nR1 >= 0 && nD1 != nD2) {
					nDiffDist ++;
				} else if (nR2 >= 0) {
					/* 返した向きに1歩進んだマスから、残りの歩数で着けること */
					pMerc->dwX = x + anPosAddTblX[nR2];
					pMerc->dwY = y + anPosAddTblY[nR2];
					int nD3 = -9;
					int nR3 = RefPathDir(pProc, pMerc, (int)pT->dwX, (int)pT->dwY, 12, &nD3);
					if (!(nR3 == -2 ? nD2 == 1 : (nR3 >= 0 && nD3 == nD2 - 1))) {
						nBadDir ++;
					}
					pMerc->dwX = x;
					pMerc->dwY = y;
				}
			}
		}
		printf("merc path cases=%d reach=%d diffreach=%d diffdist=%d baddir=%d\n", nCase, nReach, nDiffReach, nDiffDist, nBadDir);
		pMerc->dwX = dwMX;
		pMerc->dwY = dwMY;
	}

	/* 2) 敵探し：雇い主を狙っている敵をまわりに集めて、前の方法（候補ごとに道さがし）と比べる */
	{
		std::vector<DWORD> aBack;
		int nNear = 0;
		for (size_t e = 0; e < aEnemyNo.size() && nNear < 12; e ++) {
			PCHARINFO pE = g_aCharInfo[aEnemyNo[e]];
			aBack.push_back(pE->dwX);
			aBack.push_back(pE->dwY);
			aBack.push_back(pE->dwTargetTblID);
			for (int k = 0; k < 40; k ++) {
				int x = (int)pOwner->dwX + (int)(Rnd() % 11) - 5;
				int y = (int)pOwner->dwY + (int)(Rnd() % 11) - 5;
				if (abs(x - (int)pOwner->dwX) + abs(y - (int)pOwner->dwY) > 6) continue;
				if (x < 0 || y < 0 || x >= pMap->nCX || y >= pMap->nCY || !pProc->CanWalk(x, y)) continue;
				if (CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 || CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				pE->dwX = x;
				pE->dwY = y;
				break;
			}
			pE->dwTargetTblID = pOwner->dwTblID;
			nNear ++;
		}
		/* 前の方法で選ぶ敵 */
		DWORD dwRef = (DWORD)-1;
		int nRefMin = 0;
		for (size_t e = 0; e < aEnemyNo.size(); e ++) {
			PCHARINFO pE = g_aCharInfo[aEnemyNo[e]];
			if (pE->wHP == 0 || pE->byCharState == CHARSTATE_DEAD) continue;
			int nDistOwner = abs((int)pE->dwX - (int)pOwner->dwX) + abs((int)pE->dwY - (int)pOwner->dwY);
			if (nDistOwner > 6) continue;
			int nDist = abs((int)pE->dwX - (int)pMerc->dwX) + abs((int)pE->dwY - (int)pMerc->dwY);
			if (!(pE->dwTargetTblID == pOwner->dwTblID || nDistOwner <= 2 || nDist <= 1)) continue;
			if (nDist > 1) {
				if (RefPathDir(pProc, pMerc, (int)pE->dwX, (int)pE->dwY, 12, &nDist) == -1) continue;
				nDist ++;
			}
			if (dwRef == (DWORD)-1 || nDist < nRefMin) {
				dwRef = pE->dwTblID;
				nRefMin = nDist;
			}
		}
		pProc->m_aPathField[0].bValid = pProc->m_aPathField[1].bValid = FALSE;
		DWORD dwNew = pProc->FindEnemy(pOwner);
		printf("merc findenemy near=%d found=%d same=%d\n", nNear, (int)(dwNew != (DWORD)-1), (int)(dwNew == dwRef));

		/* 速さ：0.5秒ごとの敵探しを 2000 回ぶん */
		const int NRUN = 2000;
		double d0 = NowMs();
		for (int r = 0; r < NRUN; r ++) {
			for (size_t e = 0; e < aEnemyNo.size(); e ++) {
				PCHARINFO pE = g_aCharInfo[aEnemyNo[e]];
				int nDistOwner = abs((int)pE->dwX - (int)pOwner->dwX) + abs((int)pE->dwY - (int)pOwner->dwY);
				if (nDistOwner > 6 || pE->dwTargetTblID != pOwner->dwTblID) continue;
				int nDist;
				RefPathDir(pProc, pMerc, (int)pE->dwX, (int)pE->dwY, 12, &nDist);
			}
		}
		double d1 = NowMs();
		for (int r = 0; r < NRUN; r ++) {
			pProc->m_aPathField[0].bValid = pProc->m_aPathField[1].bValid = FALSE;
			pProc->FindEnemy(pOwner);
		}
		double d2 = NowMs();
		printf("BENCH merc findenemy runs=%d near=%d old_ms=%.1f new_ms=%.1f\n", NRUN, nNear, d1 - d0, d2 - d1);

		for (size_t e = 0, k = 0; e < aEnemyNo.size() && k < aBack.size(); e ++, k += 3) {
			PCHARINFO pE = g_aCharInfo[aEnemyNo[e]];
			pE->dwX = aBack[k];
			pE->dwY = aBack[k + 1];
			pE->dwTargetTblID = aBack[k + 2];
		}
	}

	/* 3) 道具を持たせる取り引き */
	WORD wHeal = 0, wOther = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem == NULL) continue;
		if (!wHeal && pItem->byType == ITEMTYPE_HPUP && pItem->pData && (pItem->byFlg & ITEMFLG_LOST) &&
			pItem->byEraseProbability == 0 && !(pItem->byFlg & ITEMFLG_TRADE)) {
			wHeal = (WORD)i;
		}
		if (!wOther && pItem->byType == ITEMTYPE_WEAPON && !(pItem->byFlg & ITEMFLG_TRADE)) {
			wOther = (WORD)i;
		}
	}
	printf("merc items heal=%d other=%d\n", (int)(wHeal != 0), (int)(wOther != 0));
	ZeroMemory(pOwner->waItem, sizeof(pOwner->waItem));
	for (int i = 0; i < 10; i ++) pOwner->waEqu[i] = 0xFFFF;
	pOwner->dwGold = 100;
	pOwner->waItem[0] = wHeal;
	pOwner->waItem[1] = wHeal;
	pOwner->waItem[2] = wOther;
	pOwner->waItem[3] = wHeal;

	{
		std::vector<WORD> a;
		a.push_back(2);			/* 武器は持てない */
		TestTradeWithMerc(pOwner, pMerc, a, 0);
		PrintItems("merc trade weapon owner", pOwner);
		PrintItems("merc trade weapon merc ", pMerc);
		a.clear();
		a.push_back(0);
		TestTradeWithMerc(pOwner, pMerc, a, 50);	/* お金は渡せない */
		PrintItems("merc trade gold owner", pOwner);
		printf("merc trade gold owner_gold=%u merc_gold=%u\n", pOwner->dwGold, pMerc->dwGold);
		a.clear();
		a.push_back(0);
		a.push_back(1);
		TestTradeWithMerc(pOwner, pMerc, a, 0);		/* 回復の道具2個を持たせる */
		PrintItems("merc trade heal owner", pOwner);
		PrintItems("merc trade heal merc ", pMerc);
		a.clear();
		TestTradeWithMerc(pOwner, pMerc, a, 0);		/* 何も渡さない＝全部返してもらう */
		PrintItems("merc trade back owner", pOwner);
		PrintItems("merc trade back merc ", pMerc);
		/* 向き合っていなくても、仲間のHPが見える範囲(10マス)なら渡せる。それより遠いと渡せない */
		{
			DWORD dwMX = pMerc->dwX, dwMY = pMerc->dwY;
			pMerc->dwX = pOwner->dwX + 10;
			pMerc->dwY = pOwner->dwY;
			a.clear();
			a.push_back(1);		/* 回復の道具 */
			TestTradeWithMerc(pOwner, pMerc, a, 0);
			PrintItems("merc trade far10 merc ", pMerc);
			a.clear();
			TestTradeWithMerc(pOwner, pMerc, a, 0);		/* 返してもらう */
			pMerc->dwX = pOwner->dwX + 11;
			a.clear();
			a.push_back(1);
			TestTradeWithMerc(pOwner, pMerc, a, 0);
			PrintItems("merc trade far11 merc ", pMerc);
			pMerc->dwX = dwMX;
			pMerc->dwY = dwMY;
		}
		/* もう一度2個持たせる（持ち物は 回復×3・武器 → 回復×1・武器） */
		a.clear();
		for (int i = 0, n = 0; i < MAX_MYITEM && n < 2; i ++) {
			if (pOwner->waItem[i] == wHeal) { a.push_back((WORD)i); n ++; }
		}
		TestTradeWithMerc(pOwner, pMerc, a, 0);
		PrintItems("merc trade again merc ", pMerc);
	}

	/* 4) HPが減ったら自分に使う・となりの雇い主のHPが減ったら雇い主に使う */
	{
		MercenarySetBattleMode(pOwner, FALSE);	/* 戦わせない */
		s_dwFakeNow += 10000;
		g_dwTime = s_dwFakeNow;
		pMerc->wHP = pMerc->wMaxHP / 4;
		if (pMerc->wHP == 0) pMerc->wHP = 1;
		WORD wBefore = pMerc->wHP;
		pProc->Proc(g_dwTime);
		printf("merc selfheal up=%d items=%d\n", (int)(pMerc->wHP > wBefore), GetItemCount(pMerc->dwTblID));
		/* すぐには続けて使わない */
		pMerc->wHP = 1;
		s_dwFakeNow += 600;
		g_dwTime = s_dwFakeNow;
		pProc->Proc(g_dwTime);
		printf("merc selfheal wait items=%d\n", GetItemCount(pMerc->dwTblID));
		pMerc->wHP = pMerc->wMaxHP;
		/* 雇い主はとなりにいる？ いなければとなりへ */
		pMerc->dwX = pOwner->dwX + 1;
		pMerc->dwY = pOwner->dwY;
		pOwner->wHP = pOwner->wMaxHP / 5;
		wBefore = pOwner->wHP;
		s_dwFakeNow += 3000;
		g_dwTime = s_dwFakeNow;
		pProc->Proc(g_dwTime);
		printf("merc ownerheal up=%d items=%d\n", (int)(pOwner->wHP > wBefore), GetItemCount(pMerc->dwTblID));
	}

	/* 5) 帰る時は残りを返す（持ち物がいっぱいなら預かり所へ） */
	{
		pMerc->waItem[0] = wHeal;
		pMerc->waItem[1] = wHeal;
		pMerc->waItem[2] = 0;
		for (int i = 0; i < MAX_MYITEM; i ++) {
			if (pOwner->waItem[i] == 0) pOwner->waItem[i] = wOther;
		}
		pOwner->waItem[MAX_MYITEM - 1] = 0;		/* 空きは1つだけ */
		DWORD dwLeaveBack = pOwner->dwLeaveID;
		int nLeaveBefore = 0;
		if (pOwner->dwLeaveID && pOwner->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			PLEAVEINFO pInfo = g_aLeaveInfo[pOwner->dwLeaveID];
			for (int i = 0; i < pInfo->byCount; i ++) if (pInfo->waItem[i] == wHeal) nLeaveBefore += pInfo->byItemCount[i];
		}
		DWORD dwMercTbl = pMerc->dwTblID;
		MercenaryOwnerLogout(pOwner);
		int nLeaveAfter = 0;
		if (pOwner->dwLeaveID && pOwner->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			PLEAVEINFO pInfo = g_aLeaveInfo[pOwner->dwLeaveID];
			for (int i = 0; i < pInfo->byCount; i ++) if (pInfo->waItem[i] == wHeal) nLeaveAfter += pInfo->byItemCount[i];
		}
		int nHealBag = 0;
		for (int i = 0; i < MAX_MYITEM; i ++) if (pOwner->waItem[i] == wHeal) nHealBag ++;
		printf("merc return gone=%d bag_heal=%d leave_added=%d\n",
			(int)(GetMercenaryOwner(g_aCharInfo[dwMercTbl]) == NULL), nHealBag, nLeaveAfter - nLeaveBefore);
		(void)dwLeaveBack;
	}

	/* 元に戻す */
	DeleteTblNoPC((int)pOwner->dwTblID);
	pOwner->dwMapID = dwMapBack;
	pOwner->dwX = dwXBack;
	pOwner->dwY = dwYBack;
	memcpy(pOwner->waItem, waItemBack, sizeof(waItemBack));
	memcpy(pOwner->waEqu, waEquBack, sizeof(waEquBack));
	pOwner->wHP = wHPBack;
	pOwner->wMaxHP = wMaxHPBack;
	pOwner->dwGold = dwGoldBack;
	AddTblNoPC((int)pOwner->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* L. ついていく列のマップ間移動：ついていく側にもフェードの指示は			 */
/*    アウト1回だけ（フェードインはクライアントがマップ情報で始める）		 */
/* ------------------------------------------------------------------------- */
static void TestTailMapChg(const std::vector<int> &aLogin)
{
	printf("== L. Tail mapchg ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	/* 港町レジェンタ(14)の乗り場 (40,54) → 船(40) */
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	PEVENTINFO pEvent = NULL;
	for (int i = 0; i < pPort->nEventCount; i ++) {
		PEVENTINFO p = &pPort->pEvent[i];
		if (p->byType == EVENTTYPE_MAPCHG && p->pData && p->dwX == 40 && p->dwY == 54) {
			pEvent = p;
		}
	}
	if (!pEvent) {
		printf("no event\n");
		return;
	}
	PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
	BYTE byRangeBack = pInfo->byRandomRange;
	pInfo->byRandomRange = 0;

	/* 先頭・2人目・3人目を縦に並べて列にする */
	PCHARINFO apChar[3];
	DWORD adwBack[3][5];
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i] = g_aCharInfo[aLogin[i]];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = pPort->dwID;
		p->dwX = p->dwXBack = pEvent->dwX;
		p->dwY = p->dwYBack = pEvent->dwY + i;
		AddTblNoPC((int)p->dwTblID);
	}
	for (int i = 0; i < 3; i ++) {
		apChar[i]->dwTailTblID = (i > 0) ? apChar[i - 1]->dwTblID : (DWORD)-1;
		apChar[i]->dwTailBackTblID = (i < 2) ? apChar[i + 1]->dwTblID : (DWORD)-1;
	}

	MockTrace(1);
	EventProc(apChar[0], pEvent, 0);
	MockTrace(0);
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		char *psz = MockCmdData(1000 + i, CMD_FADEINOUT);		/* aLogin[i] は接続 1000+i */
		printf("tailmapchg %d map=%u pos=(%u,%u) fade=%s\n", i, p->dwMapID, p->dwX, p->dwY, psz);
		free(psz);
	}

	pInfo->byRandomRange = byRangeBack;
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		AddTblNoPC((int)p->dwTblID);
	}
}

/* ------------------------------------------------------------------------- */
/* L2. 戦闘モードの間だけ、ついていく列から抜ける							 */
/*    ON=列から抜けて後ろは前の人へつなぎ直し・OFF=最後尾へ戻る				 */
/*    離れていれば最後尾のとなりへワープ・先頭がいなければ戻れない			 */
/*    傭兵・列に入っていない人・Zで解除した人は対象外						 */
/* ------------------------------------------------------------------------- */
static void TailBattleSend(PCHARINFO p, BYTE byOn)
{
	CMDPARA_DATA1 Para;
	/* 戦闘モードの切り替えは 0.2 秒未満の連打を無視するので、時刻を少し進めてから送る */
	s_dwFakeNow += 250;
	g_dwTime = s_dwFakeNow;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_BATTLEMODE;
	Para.pID = p->pID;
	Para.dwTblID = p->dwTblID;
	Para.byData = byOn;
	TestRecv(p, &Para, sizeof(Para));
}

/* 列に戻る途中のプレイヤーを、時間を進めながら歩かせる(100ms刻み) */
static void TailChaseAdvance(PCHARINFO *apChar, int nCount, int nMs)
{
	for (int t = 0; t < nMs; t += 100) {
		s_dwFakeNow += 100;
		g_dwTime = s_dwFakeNow;
		for (int i = 0; i < nCount; i ++) {
			TailChaseProc(apChar[i]);
		}
	}
}

static void TestTailBattle(const std::vector<int> &aLogin)
{
	printf("== L2. Tail battle ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	/* 港町レジェンタ(14)の乗り場の近くに先頭・2人目・3人目を縦に並べて列にする */
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	/* 列の周り(横1マス・縦は先頭の前から6マス先まで)がぜんぶ通れる場所を探す */
	int nX0 = -1, nY0 = -1;
	for (int y = 1; y + 7 < pPort->nCY && nX0 < 0; y ++) {
		for (int x = 1; x + 1 < pPort->nCX && nX0 < 0; x ++) {
			bool bOpen = true;
			for (int dy = -1; dy <= 6 && bOpen; dy ++) {
				for (int dx = -1; dx <= 1 && bOpen; dx ++) {
					if (!CheckCharBlock(pPort->dwID, x + dx, y + dy) || CheckEventBlock(pPort->dwID, x + dx, y + dy)) bOpen = false;
				}
			}
			if (bOpen) { nX0 = x; nY0 = y; }
		}
	}
	printf("tailbattle area found=%d\n", nX0 >= 0 ? 1 : 0);
	if (nX0 < 0) {
		return;
	}
	PCHARINFO apChar[3];
	DWORD adwBack[3][5];
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i] = g_aCharInfo[aLogin[i]];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = pPort->dwID;
		p->dwX = p->dwXBack = nX0;
		p->dwY = p->dwYBack = nY0 + i;
		p->byCharState = CHARSTATE_MOVE;
		if (p->wHP == 0) p->wHP = 10;
		AddTblNoPC((int)p->dwTblID);
	}
	for (int i = 0; i < 3; i ++) {
		apChar[i]->dwTailTblID = (i > 0) ? apChar[i - 1]->dwTblID : (DWORD)-1;
		apChar[i]->dwTailBackTblID = (i < 2) ? apChar[i + 1]->dwTblID : (DWORD)-1;
	}
	PCHARINFO pA = apChar[0], pB = apChar[1], pC = apChar[2];
	#define TAILSTATE(label) printf("tailbattle %s A(back=%d) B(front=%d back=%d) C(front=%d)\n", label, \
		pA->dwTailBackTblID == pB->dwTblID ? 1 : (pA->dwTailBackTblID == pC->dwTblID ? 2 : (int)pA->dwTailBackTblID), \
		pB->dwTailTblID == pA->dwTblID ? 1 : (int)pB->dwTailTblID, \
		pB->dwTailBackTblID == pC->dwTblID ? 1 : (int)pB->dwTailBackTblID, \
		pC->dwTailTblID == pA->dwTblID ? 1 : (pC->dwTailTblID == pB->dwTblID ? 2 : (int)pC->dwTailTblID))

	#define TAILPLACE(p_, x_, y_) do { DeleteTblNoPC((int)(p_)->dwTblID); \
		(p_)->dwX = (p_)->dwXBack = (x_); (p_)->dwY = (p_)->dwYBack = (y_); AddTblNoPC((int)(p_)->dwTblID); } while (0)

	/* 真ん中の人が戦闘モードON：列から抜けて、後ろの人は先頭へつなぎ直し */
	MockTrace(1);
	TailBattleSend(pB, 1);
	MockTrace(0);
	TAILSTATE("on");
	/* 戦闘モードOFF：となりにいれば、ワープせずに最後尾につく */
	TailBattleSend(pB, 0);
	TAILSTATE("off");
	printf("tailbattle off end=%d\n", pC->dwTailBackTblID == pB->dwTblID ? 1 : 0);

	/* 画面内の離れた所でON→OFF：追いかけ始める(まだ列にはつながらず、歩いて着いてからつながる) */
	TailBattleSend(pB, 1);
	TAILPLACE(pB, nX0 + 1, nY0 + 5);
	TailBattleSend(pB, 0);
	printf("tailchase near start active=%d linked=%d moved=%d\n", TailChaseIsActive(pB) ? 1 : 0,
		pB->dwTailTblID == pC->dwTblID ? 1 : 0, pB->dwY != (DWORD)(nY0 + 5) ? 1 : 0);
	{
		int nMs = 0, nSteps = 0;
		PCHARINFO apRun[1] = { pB };
		for (; nMs < 6000 && pB->dwTailTblID != pC->dwTblID; nMs += 100) {
			DWORD dwX = pB->dwX, dwY = pB->dwY;
			TailChaseAdvance(apRun, 1, 100);
			if (dwX != pB->dwX || dwY != pB->dwY) nSteps ++;
		}
		int nDist = abs((int)pB->dwX - (int)pC->dwX) + abs((int)pB->dwY - (int)pC->dwY);
		printf("tailchase near arrived front=%d active=%d dist=%d steps=%d ms=%d\n", pB->dwTailTblID == pC->dwTblID ? 1 : 0,
			TailChaseIsActive(pB) ? 1 : 0, nDist, nSteps, nMs);
	}

	/* 画面の外でON→OFF：フェードを挟んで最後尾のとなりへワープして戻る */
	TailBattleSend(pB, 1);
	int nFarX = -1, nFarY = -1;
	for (int y = 0; y < pPort->nCY && nFarX < 0; y ++) {
		for (int x = 0; x < pPort->nCX && nFarX < 0; x ++) {
			if (abs(x - nX0) > DRAW_PARTS_X + 1 && CheckCharBlock(pPort->dwID, x, y) && !CheckEventBlock(pPort->dwID, x, y)) {
				nFarX = x; nFarY = y;
			}
		}
	}
	printf("tailchase far found=%d\n", nFarX >= 0 ? 1 : 0);
	if (nFarX >= 0) {
		TAILPLACE(pB, nFarX, nFarY);
		TailBattleSend(pB, 0);
		PCHARINFO apRun[1] = { pB };
		printf("tailchase far start active=%d stay=%d linked=%d\n", TailChaseIsActive(pB) ? 1 : 0,
			(pB->dwX == (DWORD)nFarX && pB->dwY == (DWORD)nFarY) ? 1 : 0, pB->dwTailTblID == pC->dwTblID ? 1 : 0);
		TailChaseAdvance(apRun, 1, 500);
		printf("tailchase far fading active=%d stay=%d linked=%d\n", TailChaseIsActive(pB) ? 1 : 0,
			(pB->dwX == (DWORD)nFarX && pB->dwY == (DWORD)nFarY) ? 1 : 0, pB->dwTailTblID == pC->dwTblID ? 1 : 0);
		TailChaseAdvance(apRun, 1, 600);
		int nDist = abs((int)pB->dwX - (int)pC->dwX) + abs((int)pB->dwY - (int)pC->dwY);
		printf("tailchase far done front=%d active=%d dist=%d samemap=%d dir=%d\n", pB->dwTailTblID == pC->dwTblID ? 1 : 0,
			TailChaseIsActive(pB) ? 1 : 0, nDist, pB->dwMapID == pPort->dwID ? 1 : 0, pB->byDirection == pC->byDirection ? 1 : 0);
	}

	/* 先頭がいなくなったら戻れない・覚えは消える */
	TailBattleSend(pB, 1);
	DWORD dwPIDBack = pA->pID;
	pA->pID = 0;
	TailBattleSend(pB, 0);
	pA->pID = dwPIDBack;
	printf("tailbattle nohead front=%d\n", (int)pB->dwTailTblID);
	/* 覚えが消えているので、もう一度OFFしても何も起きない */
	TailBattleSend(pB, 0);
	printf("tailbattle again front=%d\n", (int)pB->dwTailTblID);

	/* Zで別の人についたら覚えは消える（ONのまま列に戻ったりしない） */
	pB->dwTailTblID = pB->dwTailBackTblID = (DWORD)-1;
	pC->dwTailTblID = pA->dwTblID; pA->dwTailBackTblID = pC->dwTblID;
	pB->dwTailTblID = pC->dwTblID; pC->dwTailBackTblID = pB->dwTblID;
	TailBattleSend(pB, 1);
	/* 戦闘モード中のZは無視されるので、サーバーの覚えだけ通常モードに戻してからZを押す */
	MercenarySetBattleMode(pB, FALSE);
	{
		CCHAR_TAIL Z;
		Z.Make(pB->dwTblID, pA->dwTblID, CHAR_TAIL_REQ);
		/* Z は ついていっていない時にだけ新しく始まる */
		TestRecv(pB, Z.Get(), Z.GetSize());
	}
	TailBattleSend(pB, 0);
	printf("tailbattle zforget front=%d\n", pB->dwTailTblID == pA->dwTblID ? 1 : 0);

	/* 並びを A-B-C に戻す(位置も縦に並べる) */
	#define TAILRESET() do { \
		PCHARINFO ap_[3] = { pA, pB, pC }; \
		for (int k_ = 0; k_ < 3; k_ ++) { \
			DeleteTblNoPC((int)ap_[k_]->dwTblID); \
			ap_[k_]->dwX = ap_[k_]->dwXBack = nX0; ap_[k_]->dwY = ap_[k_]->dwYBack = nY0 + k_; \
			AddTblNoPC((int)ap_[k_]->dwTblID); \
			ap_[k_]->dwTailTblID = (k_ > 0) ? ap_[k_ - 1]->dwTblID : (DWORD)-1; \
			ap_[k_]->dwTailBackTblID = (k_ < 2) ? ap_[k_ + 1]->dwTblID : (DWORD)-1; \
		} } while (0)
	TAILRESET();
	/* 先頭のONで後ろの2人が抜ける → 先頭のOFFで同じ順(B,C)に戻る */
	TailBattleSend(pA, 1);
	printf("tailbattle head on A(back=%d) B(front=%d) C(front=%d)\n", (int)pA->dwTailBackTblID,
		(int)pB->dwTailTblID, (int)pC->dwTailTblID);
	TailBattleSend(pA, 0);
	printf("tailbattle head off A(back=B:%d) B(front=A:%d back=C:%d) C(front=B:%d)\n",
		pA->dwTailBackTblID == pB->dwTblID ? 1 : 0, pB->dwTailTblID == pA->dwTblID ? 1 : 0,
		pB->dwTailBackTblID == pC->dwTblID ? 1 : 0, pC->dwTailTblID == pB->dwTblID ? 1 : 0);

	/* 後ろの人が自分でON中は、先頭のOFFでは戻らない。その人がOFFすると戻る */
	TAILRESET();
	TailBattleSend(pA, 1);
	TailBattleSend(pB, 1);
	TailBattleSend(pA, 0);
	{
		/* Cは離れているので、歩いて追いついてから列につながる */
		PCHARINFO apRun[1] = { pC };
		TailChaseAdvance(apRun, 1, 3000);
	}
	printf("tailbattle self wait B(front=%d) C(front=A:%d)\n", (int)pB->dwTailTblID, pC->dwTailTblID == pA->dwTblID ? 1 : 0);
	TailBattleSend(pB, 0);
	{
		PCHARINFO apRun[1] = { pB };
		TailChaseAdvance(apRun, 1, 3000);
	}
	printf("tailbattle self off B(front=C:%d)\n", pB->dwTailTblID == pC->dwTblID ? 1 : 0);

	/* 戦闘モード中のZは何もしない(覚えも列も変わらない) */
	TAILRESET();
	TailBattleSend(pB, 1);
	{
		CCHAR_TAIL Z;
		Z.Make(pB->dwTblID, pA->dwTblID, CHAR_TAIL_REQ);
		TestRecv(pB, Z.Get(), Z.GetSize());
	}
	TailBattleSend(pB, 0);
	printf("tailbattle z in battle B(front=C:%d)\n", pB->dwTailTblID == pC->dwTblID ? 1 : 0);
	TAILRESET();

	/* 3人ばらけて先頭がOFF：Bが先頭Aを、CがBを追い、B→Cの順でつながる(Cは先に着いてもBがつながるまで待つ) */
	TailBattleSend(pA, 1);
	TAILPLACE(pB, nX0 + 1, nY0 + 4);
	TAILPLACE(pC, nX0 - 1, nY0 + 6);
	TailBattleSend(pA, 0);
	printf("tailchase scatter start B(active=%d) C(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0, TailChaseIsActive(pC) ? 1 : 0);
	{
		PCHARINFO apRun[2] = { pB, pC };
		int nMsB = -1, nMsC = -1;
		for (int nMs = 0; nMs < 10000 && (nMsB < 0 || nMsC < 0); nMs += 100) {
			TailChaseAdvance(apRun, 2, 100);
			if (nMsB < 0 && pB->dwTailTblID == pA->dwTblID) nMsB = nMs;
			if (nMsC < 0 && pC->dwTailTblID == pB->dwTblID) nMsC = nMs;
		}
		printf("tailchase scatter done A(back=B:%d) B(front=A:%d back=C:%d) C(front=B:%d) Bfirst=%d\n",
			pA->dwTailBackTblID == pB->dwTblID ? 1 : 0, pB->dwTailTblID == pA->dwTblID ? 1 : 0,
			pB->dwTailBackTblID == pC->dwTblID ? 1 : 0, pC->dwTailTblID == pB->dwTblID ? 1 : 0,
			(nMsB >= 0 && nMsC >= nMsB) ? 1 : 0);
	}

	/* 追いかけ中に自分で戦闘モードにする：追いかけをやめて戻り先を覚えた状態に。OFFでまた追いかける */
	TAILRESET();
	TailBattleSend(pA, 1);
	TAILPLACE(pB, nX0 + 1, nY0 + 4);
	TAILPLACE(pC, nX0 - 1, nY0 + 6);
	TailBattleSend(pA, 0);
	TailBattleSend(pB, 1);
	printf("tailchase battle on B(active=%d front=%d) C(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0, (int)pB->dwTailTblID, TailChaseIsActive(pC) ? 1 : 0);
	TailBattleSend(pB, 0);
	printf("tailchase battle off B(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0);

	/* 追いかけ中に通常モードでZ：追いかけをやめて戻り先も忘れる。ほかの人(C)はそのまま先頭を追う */
	{
		CCHAR_TAIL Z;
		MercenarySetBattleMode(pB, FALSE);
		Z.Make(pB->dwTblID, pA->dwTblID, CHAR_TAIL_REQ);
		TestRecv(pB, Z.Get(), Z.GetSize());
		printf("tailchase z B(active=%d front=%d) C(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0, (int)pB->dwTailTblID, TailChaseIsActive(pC) ? 1 : 0);
		TailBattleSend(pB, 0);
		printf("tailchase z again B(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0);
		PCHARINFO apRun[1] = { pC };
		TailChaseAdvance(apRun, 1, 6000);
		printf("tailchase z C(front=A:%d active=%d)\n", pC->dwTailTblID == pA->dwTblID ? 1 : 0, TailChaseIsActive(pC) ? 1 : 0);
	}

	/* 追いかけ中に本人から動く要求が来ても動けない(今の位置を送り返す) */
	TAILRESET();
	TailBattleSend(pA, 1);
	TAILPLACE(pB, nX0 + 1, nY0 + 4);
	TailBattleSend(pA, 0);
	{
		CMDPARA_CHARPOS Pos;
		ZeroMemory(&Pos, sizeof(Pos));
		Pos.byCmd = CMD_CHARPOS;
		Pos.pID = pB->pID;
		Pos.dwTblID = pB->dwTblID;
		Pos.dwMapID = pB->dwMapID;
		Pos.byX = (BYTE)(pB->dwX - 1);
		Pos.byY = (BYTE)pB->dwY;
		Pos.byDirection = 2;
		DWORD dwXBack = pB->dwX;
		TestRecv(pB, &Pos, sizeof(Pos));
		printf("tailchase keymove blocked=%d active=%d\n", pB->dwX == dwXBack ? 1 : 0, TailChaseIsActive(pB) ? 1 : 0);
	}
	/* 先頭がいなくなったら追いかけをやめる */
	{
		DWORD dwPIDBack = pA->pID;
		pA->pID = 0;
		TailReturnForget(pA->dwTblID);
		pA->pID = dwPIDBack;
		printf("tailchase headgone B(active=%d) C(active=%d)\n", TailChaseIsActive(pB) ? 1 : 0, TailChaseIsActive(pC) ? 1 : 0);
	}
	TAILRESET();
	for (int k = 0; k < 3; k ++) TailReturnForget(apChar[k]->dwTblID);

	/* ---- ばらけている間は、外の人がついていけない（CHAR_TAIL_NG3）。傭兵まわりの列の入れ替え ---- */
	if (aLogin.size() >= 4) {
		PCHARINFO pD = g_aCharInfo[aLogin[3]];
		DWORD dwDBack[5] = { pD->dwMapID, pD->dwX, pD->dwY, pD->dwTailTblID, pD->dwTailBackTblID };
		BYTE byDirBack = pD->byDirection;
		int nUp = 0;
		for (int d = 0; d < 4; d ++) if (anPosAddTblX[d] == 0 && anPosAddTblY[d] == -1) nUp = d;
		DeleteTblNoPC((int)pD->dwTblID);
		pD->dwMapID = pPort->dwID;
		pD->byCharState = CHARSTATE_MOVE;
		if (pD->wHP == 0) pD->wHP = 10;
		pD->dwTailTblID = pD->dwTailBackTblID = (DWORD)-1;
		AddTblNoPC((int)pD->dwTblID);
		for (int k = 0; k < 4; k ++) MercenarySetBattleMode(g_aCharInfo[aLogin[k]], FALSE);

		/* 外の人 D が、列の最後尾の1歩後ろから pT に Z を押す。結果の種類(届いた CHAR_TAIL の最後の1つ)を返す */
		#define TAILZ(pT_, x_, y_, res_) do { \
			TAILPLACE(pD, (x_), (y_)); pD->byDirection = (BYTE)nUp; \
			MockTrace(1); \
			CCHAR_TAIL Zd_; Zd_.Make(pD->dwTblID, (pT_)->dwTblID, CHAR_TAIL_REQ); \
			TestRecv(pD, Zd_.Get(), Zd_.GetSize()); \
			MockTrace(0); \
			char *psz_ = MockLastBody(1000 + 3, CMD_MAIN_CHAR); \
			unsigned int u_ = 0xFFFFFFFF; \
			unsigned int uSub_ = 0; if (strlen(psz_) >= 28) { sscanf(psz_ + 2, "%2x", &uSub_); if (uSub_ == CMDCHAR_TAIL) sscanf(psz_ + 20, "%2x", &u_); } \
			free(psz_); \
			(res_) = (int)u_; } while (0)
		int nRes;

		/* B が戦闘モードで離れている間(A→C の列)：外の D が最後尾 C にZ → NG3 で列は変わらない */
		TAILRESET();
		TailBattleSend(pB, 1);
		TAILPLACE(pB, nX0 + 1, nY0 + 5);
		TAILZ(pC, nX0, nY0 + 3, nRes);
		printf("tailscatter D->last C ng3=%d D(front=%d back=%d) C(back=%d)\n", nRes == CHAR_TAIL_NG3 ? 1 : 0,
			(int)pD->dwTailTblID, (int)pD->dwTailBackTblID, (int)pC->dwTailBackTblID);
		/* 先頭 A にZ(A の後ろは C がつながっているので元々ついていけないが、列は変わらない) */
		TAILZ(pA, nX0, nY0 + 1, nRes);
		printf("tailscatter D->head A ng3=%d D(front=%d) A(back=C:%d)\n", nRes == CHAR_TAIL_NG3 ? 1 : 0,
			(int)pD->dwTailTblID, pA->dwTailBackTblID == pC->dwTblID ? 1 : 0);
		/* 離れている B にZ → NG3 */
		TAILZ(pB, nX0 + 1, nY0 + 6, nRes);
		printf("tailscatter D->away B ng3=%d D(front=%d) B(back=%d)\n", nRes == CHAR_TAIL_NG3 ? 1 : 0,
			(int)pD->dwTailTblID, (int)pB->dwTailBackTblID);

		/* B が通常モードに戻って待っている状態：自分でZを押して C の後ろに入るのは断られない */
		TAILPLACE(pB, nX0, nY0 + 3);
		pB->byDirection = (BYTE)nUp;
		MercenarySetBattleMode(pB, FALSE);		/* サーバーの覚えだけ通常モードに戻す(戻り先は覚えたまま待っている) */
		{
			CCHAR_TAIL Zb;
			Zb.Make(pB->dwTblID, pC->dwTblID, CHAR_TAIL_REQ);
			TestRecv(pB, Zb.Get(), Zb.GetSize());
		}
		printf("tailscatter B self Z front=C:%d C(back=B:%d)\n", pB->dwTailTblID == pC->dwTblID ? 1 : 0,
			pC->dwTailBackTblID == pB->dwTblID ? 1 : 0);
		TAILRESET();
		for (int k = 0; k < 3; k ++) TailReturnForget(apChar[k]->dwTblID);
		for (int k = 0; k < 4; k ++) MercenarySetBattleMode(g_aCharInfo[aLogin[k]], FALSE);

		/* B が傭兵を連れている */
		TAILPLACE(pB, nX0, nY0 + 1);
		pB->byDirection = 3;
		CHARINFO Npc;
		ZeroMemory(&Npc, sizeof(Npc));
		strcpy(Npc.szName, "ハーン");
		static char szSpeechT[] = "@m";
		Npc.pszNpcSpeech = szSpeechT;
		PCHARINFO pM = NULL;
		MercenaryTalk(pB, &Npc);
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (GetMercenaryOwner(g_aCharInfo[i]) == pB) pM = g_aCharInfo[i];
		}
		printf("tailmerc hired=%d\n", pM ? 1 : 0);
		if (pM) {
			/* B が戦闘モードで離れ、傭兵が B の後ろについた後、戦闘モードを解除すると B が列に戻れる */
			pC->dwTailTblID = (DWORD)-1;	/* A→B の2人の列にする */
			pC->dwTailBackTblID = (DWORD)-1;
			pB->dwTailBackTblID = (DWORD)-1;
			TailBattleSend(pB, 1);
			pB->dwTailBackTblID = pM->dwTblID;
			pM->dwTailTblID = pB->dwTblID;
			pM->dwTailBackTblID = (DWORD)-1;
			TailBattleSend(pB, 0);
			printf("tailmerc return B(front=A:%d) M(front=%d) A(back=B:%d)\n", pB->dwTailTblID == pA->dwTblID ? 1 : 0,
				(int)pM->dwTailTblID, pA->dwTailBackTblID == pB->dwTblID ? 1 : 0);
			TailReturnForget(pB->dwTblID);

			/* A→B→傭兵(Bの) の列で、D が傭兵の後ろからZ → D は B の後ろにつながり、傭兵は列から外れる */
			pB->dwTailBackTblID = pM->dwTblID;
			pM->dwTailTblID = pB->dwTblID;
			pM->dwTailBackTblID = (DWORD)-1;
			pM->dwX = pM->dwXBack = nX0;
			pM->dwY = pM->dwYBack = nY0 + 2;
			pM->byCharState = CHARSTATE_MOVE;
			TAILZ(pM, nX0, nY0 + 3, nRes);
			printf("tailmerc D->merc D(front=B:%d) B(back=D:%d) M(front=%d back=%d) res=%d\n", pD->dwTailTblID == pB->dwTblID ? 1 : 0,
				pB->dwTailBackTblID == pD->dwTblID ? 1 : 0, (int)pM->dwTailTblID, (int)pM->dwTailBackTblID, nRes);
			/* 傭兵の前にプレイヤーがいなければ断る(NG2) */
			pD->dwTailTblID = pD->dwTailBackTblID = (DWORD)-1;
			pB->dwTailBackTblID = (DWORD)-1;
			pM->dwTailTblID = pM->dwTailBackTblID = (DWORD)-1;
			TAILZ(pM, nX0, nY0 + 3, nRes);
			printf("tailmerc D->lone merc ng=%d D(front=%d)\n", nRes, (int)pD->dwTailTblID);
			MercenaryTalk(pB, &Npc);		/* もう一度話しかけてお別れ */
		}
		TAILRESET();
		#undef TAILZ

		DeleteTblNoPC((int)pD->dwTblID);
		pD->dwMapID = dwDBack[0];
		pD->dwX = pD->dwXBack = dwDBack[1];
		pD->dwY = pD->dwYBack = dwDBack[2];
		pD->dwTailTblID = dwDBack[3];
		pD->dwTailBackTblID = dwDBack[4];
		pD->byDirection = byDirBack;
		AddTblNoPC((int)pD->dwTblID);
	}
	#undef TAILRESET

	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		AddTblNoPC((int)p->dwTblID);
	}
	#undef TAILPLACE
	#undef TAILSTATE
}

/* ------------------------------------------------------------------------- */
/* M. ギルドの依頼															 */
/*    受ける → 見えるNPCが持ち主と仲間にだけ届く・ぶつからない・宝箱・ポータル・	 */
/*    渡して報酬・別の依頼の人は仲間になれない・取り消し・保存して読み直し	 */
/* ------------------------------------------------------------------------- */
EM_JS(int, MockCalls, (int nConn), {
	return Module['sboNet'].calls(nConn);
});
EM_JS(int, MockCmdCount, (int nConn, int nCmd), {
	return Module['sboNet'].cmdCount(nConn, nCmd);
});
EM_JS(int, MockCmdLen, (int nConn), {
	return Module['sboNet'].cmdLen(nConn);
});
EM_JS(int, MockHasText, (int nConn, int nCmd, const char *psz), {
	return Module['sboNet'].hasText(nConn, nCmd, UTF8ToString(psz));
});
/* nFrom 番目以降で、コマンド nA が全部 nB より前なら 1、逆なら 0、どちらかが無ければ -1 */
EM_JS(int, MockCmdBefore, (int nConn, int nFrom, int nA, int nB), {
	return Module['sboNet'].cmdBefore(nConn, nFrom, nA, nB);
});
extern BOOL SaveCharInfo(void);
extern BOOL LoadCharInfo(void);
extern BOOL SaveQuestInfo(void);
extern BOOL LoadQuestInfo(void);

static void QuestMovePC(PCHARINFO p, int nMap, int x, int y)
{
	DeleteTblNoPC((int)p->dwTblID);
	p->dwMapID = (DWORD)nMap;
	p->dwX = p->dwXBack = (DWORD)x;
	p->dwY = p->dwYBack = (DWORD)y;
	p->byCharState = CHARSTATE_MOVE;
	if (p->wHP == 0) p->wHP = 10;
	AddTblNoPC((int)p->dwTblID);
}

static void QuestSendCmd(PCHARINFO p, BYTE byType, WORD wID, DWORD dwNpc)
{
	CMDPARA_QUEST Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_QUEST;
	Para.byType = byType;
	Para.wQuestID = wID;
	Para.dwTblID = p->dwTblID;
	Para.dwNpcTblID = dwNpc;
	TestRecv(p, &Para, sizeof(Para));
}

static void QuestSendTalk(PCHARINFO p, DWORD dwNpcTbl)
{
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_TALKRAND;
	Para.pID = p->pID;
	Para.dwTblID = dwNpcTbl;
	TestRecv(p, &Para, sizeof(Para));
}

static void QuestSendParty(PCHARINFO pFrom, PCHARINFO pTo, BYTE bySub)
{
	CMDPARA_PARTYCMD Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_PARTY;
	Para.byCmdSub = bySub;
	Para.dwTblIDFrom = pFrom->dwTblID;
	Para.dwTblIDTo = pTo->dwTblID;
	TestRecv(pFrom, &Para, sizeof(Para));
}

static int QuestCountItem(PCHARINFO p, WORD wItem)
{
	int n = 0;
	for (int i = 0; i < MAX_MYITEM; i ++) if (p->waItem[i] == wItem) n ++;
	/* 依頼の品は大事な物の欄に入るので、そちらも数える */
	for (int i = 0; i < MAX_IMPORTANT; i ++) if (p->waImportant[i] == wItem) n ++;
	return n;
}

/* 持ち主が見える見えるNPC（種類 byKind。持ち主に見える分だけ）を探す */
static PCHARINFO QuestFindCopy(PCHARINFO pViewer, BYTE byKindWanted)
{
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType == NPCTYPE_QUESTNPC && p->wGrpID == (WORD)byKindWanted && IsCharVisibleTo(p, pViewer)) {
			return p;
		}
	}
	return NULL;
}

static int QuestCountCopies(void)
{
	int n = 0;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (g_aCharInfo[i]->byNpcType == NPCTYPE_QUESTNPC) n ++;
	}
	return n;
}

/* ------------------------------------------------------------------------- */
/* M2. 思い出の本（語り部 @r・クリア済みのお話をやり直す）					 */
/*    クリア→やり直し→依頼をまた受けられる／印が消える／後のお話も戻る／	 */
/*    前のお話は残る／2回目は報酬なし・印は立つ／依頼中・遠くは断る／		 */
/*    古い保存データのもらい済みの補完・お話の保存と読み込み				 */
/* ------------------------------------------------------------------------- */
extern BOOL SaveStoryInfo(void);
extern BOOL LoadStoryInfo(void);
extern "C" const char *SboWebDataGet(const char *, int);
static std::string FlagGroupApi(const char *pszKind, int nID, const char *pszJson);	/* 後ろで定義 */

static PSTORYINFO StoryTestNew(WORD wID, LPCSTR pszName, WORD wClear, WORD wPrev, WORD wFrom, WORD wTo, WORD wQuest, BYTE byCountNum)
{
	PSTORYINFO p = new STORYINFO;
	ZeroMemory(p, sizeof(*p));
	p->wID = wID; p->byState = STORYSTATE_ON; p->wSort = wID;
	strcpy(p->szName, pszName);
	strcpy(p->szExplain, "説明");
	p->wClearFlag = wClear; p->wPrevStory = wPrev;
	for (int i = 0; i < MAX_STORYFLAGRANGE; i ++) p->awFlagFrom[i] = p->awFlagTo[i] = FLAG_NOBIT;
	for (int i = 0; i < MAX_STORYNUM; i ++) p->abyNum[i] = FLAG_NONUM;
	for (int i = 0; i < MAX_STORYCHEST; i ++) p->awTakeChest[i] = FLAG_NOBIT;
	p->awFlagFrom[0] = wFrom; p->awFlagTo[0] = wTo;
	p->awQuest[0] = wQuest;
	p->byCountNum = byCountNum;
	return p;
}

static void StoryReset(PCHARINFO p, WORD wStory, DWORD dwNpc)
{
	QuestSendCmd(p, QUESTCMD_STORYRESET, wStory, dwNpc);
}

static BOOL AdminStorySend(PCHARINFO p, BYTE bySub, int nNo, const STORYINFO *pInfo, CMDPARA_ADMINSTORY &Res)
{
	PCMDPARA_ADMINSTORY pPara = new CMDPARA_ADMINSTORY;
	std::vector<BYTE> aCap;
	BOOL bRet;

	ZeroMemory(pPara, sizeof(*pPara));
	pPara->byCmd = CMD_ADMINSTORY;
	pPara->byCmdSub = bySub;
	pPara->nNo = nNo;
	if (pInfo) pPara->Info = *pInfo;
	SvrSendCaptureBegin();
	TestRecv(p, pPara, sizeof(CMDPARA_ADMINSTORY));
	bRet = SvrSendCaptureEnd(aCap);
	delete pPara;
	ZeroMemory(&Res, sizeof(Res));
	if (!bRet || aCap.size() < sizeof(CMDPARA_ADMINSTORY)) return FALSE;
	memcpy(&Res, &aCap[0], sizeof(Res));
	return TRUE;
}

static void TestStory(const std::vector<int> &aLogin)
{
	printf("== M2. Story ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pOwner = g_aCharInfo[aLogin[0]], pFar = g_aCharInfo[aLogin[1]], pAdmin = g_aCharInfo[aLogin[2]];
	PCHARINFO apPC[3] = { pOwner, pFar, pAdmin };
	int nMap = (int)pOwner->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	DWORD dwMapBack[3], dwXBack[3], dwYBack[3], dwGoldBack[3];
	BYTE byAdminBack = pAdmin->byAdminLV;
	for (int i = 0; i < 3; i ++) {
		dwMapBack[i] = apPC[i]->dwMapID; dwXBack[i] = apPC[i]->dwX; dwYBack[i] = apPC[i]->dwY;
		dwGoldBack[i] = apPC[i]->dwGold;
		apPC[i]->wQuestID = 0; apPC[i]->byQuestStep = 0; apPC[i]->wQuestFlg = 0;
		apPC[i]->dwPartyID = 0;
		apPC[i]->dwSpaceID = SPACE_NORMAL;
		apPC[i]->dwTargetTblID = apPC[i]->dwTargetTblID2 = apPC[i]->dwTargetTblID3 = (DWORD)-1;
		ZeroMemory(apPC[i]->abyQuestDone, sizeof(apPC[i]->abyQuestDone));
		ZeroMemory(apPC[i]->abyQuestRewarded, sizeof(apPC[i]->abyQuestRewarded));
		ZeroMemory(apPC[i]->abyFlag, sizeof(apPC[i]->abyFlag));
		ZeroMemory(apPC[i]->abyFlagNum, sizeof(apPC[i]->abyFlagNum));
	}
	pAdmin->byAdminLV = ADMINLV_USER;

	/* 空いているマスを 4 つ */
	std::vector<std::pair<int, int> > aCell;
	for (int r = 1; r < 12 && aCell.size() < 4; r ++) {
		for (int dy = -r; dy <= r && aCell.size() < 4; dy ++) {
			for (int dx = -r; dx <= r && aCell.size() < 4; dx ++) {
				if (abs(dx) != r && abs(dy) != r) continue;
				int x = (int)pOwner->dwX + dx, y = (int)pOwner->dwY + dy;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
	}
	printf("story cells=%d\n", (int)aCell.size());
	if (aCell.size() < 4) {
		return;
	}

	/* 依頼を差し替える（あとで戻す） */
	std::vector<PQUESTINFO> aQuestBack;
	for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) aQuestBack.push_back(g_aQuestInfo[i]);
	g_aQuestInfo.RemoveAll();
	/* 依頼 1・2：一度きり・お金だけの報酬・終えると印 100・110 が立つ */
	for (int q = 1; q <= 2; q ++) {
		PQUESTINFO p = new QUESTINFO;
		ZeroMemory(p, sizeof(*p));
		p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
		p->wID = (WORD)q; p->byState = QUESTSTATE_ON; p->byGuild = 0; p->wSort = (WORD)q; p->byOnce = 1; p->byStepCount = 1;
		sprintf(p->szName, "思い出依頼%d", q);
		p->dwGold = (q == 1) ? 100 : 50;
		for (int k = 0; k < MAX_QUESTDONESET; k ++) p->awDoneSetFlag[k] = FLAG_NOBIT;
		p->awDoneSetFlag[0] = (WORD)(q == 1 ? 100 : 110);
		p->byDoneNumNo = FLAG_NONUM;
		for (int k = 0; k < MAX_QUESTNPC; k ++) { p->aNpc[k].byStepMax = QUESTNPC_NOSTEP; p->aNpc[k].byNextStep = QUESTNPC_NOSTEP; }
		g_aQuestInfo.Add(p);
	}
	/* お話を差し替える：1=L1（印 100・依頼 1・回数は数値 5）、2=L2（前は 1・印 110・依頼 2）、3=止めているお話 */
	std::vector<PSTORYINFO> aStoryBack;
	for (int i = 0; i < g_aStoryInfo.GetSize(); i ++) aStoryBack.push_back(g_aStoryInfo[i]);
	g_aStoryInfo.RemoveAll();
	g_aStoryInfo.Add(StoryTestNew(1, "L1", 100, 0, 100, 100, 1, 5));
	g_aStoryInfo.Add(StoryTestNew(2, "L2", 110, 1, 110, 110, 2, FLAG_NONUM));
	g_aStoryInfo.Add(StoryTestNew(3, "止めた話", 120, 0, 120, 120, 0, FLAG_NONUM));
	g_aStoryInfo[2]->byState = STORYSTATE_OFF;
	g_aStoryInfo[0]->abyNum[0] = 6;						/* 数値 6 も 0 に戻す */

	/* ギルドの人（@g0）と語り部（@r） */
	PCHARINFO apNpc[2];
	LPCSTR apszSpeech[2] = { "@g0", "@r" };
	for (int n = 0; n < 2; n ++) {
		PCHARINFO p = new CHARINFO;
		ZeroMemory((PBYTE)p, sizeof(CHARINFO));
		p->byNpcType = NPCTYPE_NOMOVE;
		SetInitCharData(p);
		p->byGameState = GAMESTATE_NPC;
		p->byCharState = CHARSTATE_MOVE;
		p->dwMapID = nMap;
		p->dwX = p->dwXBack = p->dwStartX = (DWORD)aCell[n].first;
		p->dwY = p->dwYBack = p->dwStartY = (DWORD)aCell[n].second;
		strcpy(p->szName, n == 0 ? "ギルド受付" : "語り部");
		p->pszNpcSpeech = new char[8];
		strcpy(p->pszNpcSpeech, apszSpeech[n]);
		p = AddChar(p);
		p->pID = p->dwTblID + 1;
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(p);
		apNpc[n] = p;
	}
	DWORD dwGuild = apNpc[0]->dwTblID, dwTeller = apNpc[1]->dwTblID;

	/* 遠くの人と近くの人 */
	QuestMovePC(pOwner, nMap, aCell[0].first, aCell[0].second + 1);
	QuestMovePC(pFar, nMap, aCell[2].first, aCell[2].second);
	QuestMovePC(pAdmin, nMap, aCell[3].first, aCell[3].second);
	pFar->dwX = pFar->dwXBack = (DWORD)(pMap->nCX - 3);
	pFar->dwY = pFar->dwYBack = (DWORD)(pMap->nCY - 3);

	/* まだ何もクリアしていない：本は空（0 件の 1 パケットだけ届く） */
	int nList0 = MockCmdCount(1000, CMD_QUEST);
	QuestSendTalk(pOwner, dwTeller);
	printf("story empty list packets=%d\n", MockCmdCount(1000, CMD_QUEST) - nList0);

	/* 依頼 1 を受けて終える（報酬 100・印 100・もらい済み） */
	DWORD dwGold0 = pOwner->dwGold;
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 1, dwGuild);
	QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
	printf("story q1 clear gold+=%d flag100=%d done=%d rewarded=%d quest=%u\n", (int)(pOwner->dwGold - dwGold0),
		FlagBitGet(pOwner->abyFlag, 100), (int)QuestIsDone(pOwner, 1), (int)QuestIsRewarded(pOwner, 1), pOwner->wQuestID);
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 1, dwGuild);
	printf("story q1 once-blocked quest=%u\n", pOwner->wQuestID);

	/* 依頼 2 も終える。さらに止めているお話の印 120 と、数値 5・6 に値を入れておく */
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 2, dwGuild);
	QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
	FlagBitSet(pOwner->abyFlag, 120, TRUE);
	FlagBitSet(pOwner->abyFlag, 99, TRUE);				/* 範囲の外の印（もらった印）は残る */
	FlagBitSet(pOwner->abyFlag, 111, TRUE);
	FlagNumSet(pOwner->abyFlagNum, 5, 3);
	FlagNumSet(pOwner->abyFlagNum, 6, 9);
	printf("story q2 clear flag110=%d done=%d\n", FlagBitGet(pOwner->abyFlag, 110), (int)QuestIsDone(pOwner, 2));

	/* 本：止めているお話は出ない（2 件）。遠くの人は話しても届かない */
	nList0 = MockCmdCount(1000, CMD_QUEST);
	QuestSendTalk(pOwner, dwTeller);
	printf("story list packets=%d\n", MockCmdCount(1000, CMD_QUEST) - nList0);

	/* 遠くの人（印は立てておく）はやり直せない */
	FlagBitSet(pFar->abyFlag, 100, TRUE);
	StoryReset(pFar, 1, dwTeller);
	printf("story far refused flag100=%d\n", FlagBitGet(pFar->abyFlag, 100));

	/* 後のお話 L2 だけやり直す：L1 の印・依頼はそのまま */
	StoryReset(pOwner, 2, dwTeller);
	printf("story reset L2 flag100=%d flag110=%d flag111=%d done1=%d done2=%d count5=%u\n", FlagBitGet(pOwner->abyFlag, 100),
		FlagBitGet(pOwner->abyFlag, 110), FlagBitGet(pOwner->abyFlag, 111), (int)QuestIsDone(pOwner, 1), (int)QuestIsDone(pOwner, 2),
		FlagNumGet(pOwner->abyFlagNum, 5));

	/* L2 の依頼 2 をまた受けられて、2 回目は報酬なし・印は立つ */
	dwGold0 = pOwner->dwGold;
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 2, dwGuild);
	printf("story q2 reaccept quest=%u\n", pOwner->wQuestID);
	/* 依頼中は、その依頼を含むお話をやり直せない */
	FlagBitSet(pOwner->abyFlag, 110, TRUE);
	StoryReset(pOwner, 2, dwTeller);
	StoryReset(pOwner, 1, dwTeller);
	printf("story busy refused flag100=%d flag110=%d quest=%u count5=%u\n", FlagBitGet(pOwner->abyFlag, 100), FlagBitGet(pOwner->abyFlag, 110),
		pOwner->wQuestID, FlagNumGet(pOwner->abyFlagNum, 5));
	FlagBitSet(pOwner->abyFlag, 110, FALSE);
	QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
	printf("story q2 second gold+=%d flag110=%d done=%d rewarded=%d\n", (int)(pOwner->dwGold - dwGold0), FlagBitGet(pOwner->abyFlag, 110),
		(int)QuestIsDone(pOwner, 2), (int)QuestIsRewarded(pOwner, 2));

	/* 前のお話 L1 をやり直すと、後の L2 もいっしょに戻る。数値 6 は 0、数値 5（回数）は +1 */
	StoryReset(pOwner, 1, dwTeller);
	printf("story reset L1 flag99=%d flag100=%d flag110=%d flag111=%d flag120=%d done1=%d done2=%d count5=%u num6=%u rewarded1=%d rewarded2=%d\n",
		FlagBitGet(pOwner->abyFlag, 99), FlagBitGet(pOwner->abyFlag, 100), FlagBitGet(pOwner->abyFlag, 110), FlagBitGet(pOwner->abyFlag, 111),
		FlagBitGet(pOwner->abyFlag, 120), (int)QuestIsDone(pOwner, 1), (int)QuestIsDone(pOwner, 2), FlagNumGet(pOwner->abyFlagNum, 5),
		FlagNumGet(pOwner->abyFlagNum, 6), (int)QuestIsRewarded(pOwner, 1), (int)QuestIsRewarded(pOwner, 2));

	/* やり直した依頼 1 をまた受けて終える：報酬なし・印は立つ */
	dwGold0 = pOwner->dwGold;
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 1, dwGuild);
	printf("story q1 reaccept quest=%u\n", pOwner->wQuestID);
	QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
	printf("story q1 second gold+=%d flag100=%d done=%d\n", (int)(pOwner->dwGold - dwGold0), FlagBitGet(pOwner->abyFlag, 100), (int)QuestIsDone(pOwner, 1));

	/* 取り上げる持ち物：大事な物の欄と手持ち（装備中を除く）から消える・預かり所と他の物は残る・0 のお話は何も消えない */
	if (g_aItemInfo.GetSize() > 6) {
		WORD awImpBak[MAX_IMPORTANT], awItemBak[MAX_MYITEM], awEquBak[MAX_EQUTYPE];
		memcpy(awImpBak, pOwner->waImportant, sizeof(awImpBak));
		memcpy(awItemBak, pOwner->waItem, sizeof(awItemBak));
		memcpy(awEquBak, pOwner->waEqu, sizeof(awEquBak));
		auto TakeSetup = [&]() {
			ZeroMemory(pOwner->waImportant, sizeof(pOwner->waImportant));
			ZeroMemory(pOwner->waItem, sizeof(pOwner->waItem));
			for (int e = 0; e < MAX_EQUTYPE; e ++) pOwner->waEqu[e] = 0xFFFF;
			pOwner->waImportant[0] = 1; pOwner->waImportant[1] = 5; pOwner->waImportant[2] = 2; pOwner->waImportant[3] = 1;
			pOwner->waItem[0] = 4; pOwner->waItem[1] = 3; pOwner->waItem[2] = 1; pOwner->waItem[3] = 3;
			pOwner->waEqu[0] = 3;													/* 手持ち 3 番目（品 3）を装備中 */
		};
		auto TakeShow = [&](LPCSTR pszTitle) {
			printf("story take %s imp=", pszTitle);
			for (int n = 0; n < MAX_IMPORTANT && n < 6; n ++) printf("%u,", pOwner->waImportant[n]);
			printf(" item=");
			for (int n = 0; n < 5; n ++) printf("%u,", pOwner->waItem[n]);
			printf(" equ0=%u equitem=%u\n", pOwner->waEqu[0], (pOwner->waEqu[0] < MAX_MYITEM) ? pOwner->waItem[pOwner->waEqu[0]] : 0);
		};
		/* 取り上げ指定が 0 のお話：何も消えない */
		TakeSetup();
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		StoryReset(pOwner, 1, dwTeller);
		TakeShow("none");
		/* 後のお話 L2 だけ：品 3 だけ消える（装備中の 3 は残り、装備も付いたまま）。品 1・2 は残る */
		g_aStoryInfo[0]->awTakeItem[0] = 1; g_aStoryInfo[0]->awTakeItem[1] = 2;
		g_aStoryInfo[1]->awTakeItem[0] = 3;
		TakeSetup();
		FlagBitSet(pOwner->abyFlag, 110, TRUE);
		StoryReset(pOwner, 2, dwTeller);
		TakeShow("L2");
		/* 前のお話 L1：L1 と L2 の分が全部消える */
		TakeSetup();
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		StoryReset(pOwner, 1, dwTeller);
		TakeShow("L1");
		/* 持っていなければ何も起きない（もう一度やり直しても同じ） */
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		StoryReset(pOwner, 1, dwTeller);
		TakeShow("again");
		for (int n = 0; n < MAX_STORYTAKEITEM; n ++) g_aStoryInfo[0]->awTakeItem[n] = g_aStoryInfo[1]->awTakeItem[n] = 0;
		memcpy(pOwner->waImportant, awImpBak, sizeof(awImpBak));
		memcpy(pOwner->waItem, awItemBak, sizeof(awItemBak));
		memcpy(pOwner->waEqu, awEquBak, sizeof(awEquBak));
	}

	/* 戻す宝箱：指定した宝箱の開けた記録だけ消える（番号 0 も）・指定外は残る・無し(65535)は何も消えない・後のお話だけならその分だけ */
	{
		BYTE abyChestBak[MAX_CHESTBYTE];
		memcpy(abyChestBak, pOwner->abyChestOpened, sizeof(abyChestBak));
		/* 宝箱 5 の「印で変わるマス」を 1 行足す（本人の見た目が閉じた絵に戻って送り直されるかを見る。終わったら戻す） */
		MAPSWAP aSwapBak[MAX_MAPSWAP];
		BYTE bySwapCountBak = pMap->bySwapCount;
		memcpy(aSwapBak, pMap->aSwap, sizeof(aSwapBak));
		ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
		pMap->bySwapCount = 1;
		pMap->aSwap[0].wX = (WORD)aCell[0].first; pMap->aSwap[0].wY = (WORD)aCell[0].second;
		pMap->aSwap[0].byKind = MAPSWAPKIND_CHEST; pMap->aSwap[0].wChestNo = 5; pMap->aSwap[0].wNeedFlag = FLAG_NOBIT;
		pMap->aSwap[0].wParts = 1;
		ZeroMemory(pOwner->abyChestOpened, sizeof(pOwner->abyChestOpened));
		WORD wBaseParts = MapSwapParts(pMap, pOwner, aCell[0].first, aCell[0].second);
		WORD wOpenParts = (wBaseParts == 1) ? 2 : 1;
		pMap->aSwap[0].wParts = wOpenParts;
		auto ChestSetup = [&]() {
			ZeroMemory(pOwner->abyChestOpened, sizeof(pOwner->abyChestOpened));
			ChestOpenedSet(pOwner, 0, TRUE); ChestOpenedSet(pOwner, 5, TRUE); ChestOpenedSet(pOwner, 6, TRUE);
			ChestOpenedSet(pOwner, 7, TRUE); ChestOpenedSet(pOwner, 2047, TRUE);
		};
		auto ChestShow = [&](LPCSTR pszTitle, int nSwap0) {
			printf("story chest %s bits0=%d,5=%d,6=%d,7=%d,2047=%d,8=%d swap=%d open_view=%d\n", pszTitle, (int)ChestOpenedGet(pOwner, 0), (int)ChestOpenedGet(pOwner, 5),
				(int)ChestOpenedGet(pOwner, 6), (int)ChestOpenedGet(pOwner, 7), (int)ChestOpenedGet(pOwner, 2047), (int)ChestOpenedGet(pOwner, 8),
				MockCmdCount(1000, CMD_MAPSWAP) - nSwap0, (int)(MapSwapParts(pMap, pOwner, aCell[0].first, aCell[0].second) == wOpenParts));
		};
		int nSwap0;
		/* 指定が全部無しのお話：何も戻らない */
		ChestSetup();
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		nSwap0 = MockCmdCount(1000, CMD_MAPSWAP);
		StoryReset(pOwner, 1, dwTeller);
		ChestShow("none", nSwap0);
		/* 後のお話 L2 だけ指定（5・2047）：L2 だけやり直すとその分だけ。0・6・7 は残る */
		g_aStoryInfo[1]->awTakeChest[0] = 5; g_aStoryInfo[1]->awTakeChest[1] = 2047;
		g_aStoryInfo[0]->awTakeChest[0] = 0; g_aStoryInfo[0]->awTakeChest[3] = 6;
		ChestSetup();
		FlagBitSet(pOwner->abyFlag, 110, TRUE);
		nSwap0 = MockCmdCount(1000, CMD_MAPSWAP);
		StoryReset(pOwner, 2, dwTeller);
		ChestShow("L2", nSwap0);
		/* 前のお話 L1：L1（0・6）と L2（5・2047）の分が全部戻る。7 と 8 は残る */
		ChestSetup();
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		nSwap0 = MockCmdCount(1000, CMD_MAPSWAP);
		StoryReset(pOwner, 1, dwTeller);
		ChestShow("L1", nSwap0);
		/* 開けていなければ何も起きない（もう一度やり直しても同じ） */
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		nSwap0 = MockCmdCount(1000, CMD_MAPSWAP);
		StoryReset(pOwner, 1, dwTeller);
		ChestShow("again", nSwap0);
		/* 同じ宝箱を 2 つのお話で指定しても、戻す数は 1 個 */
		g_aStoryInfo[1]->awTakeChest[2] = 0;
		ChestSetup();
		FlagBitSet(pOwner->abyFlag, 100, TRUE);
		nSwap0 = MockCmdCount(1000, CMD_MAPSWAP);
		StoryReset(pOwner, 1, dwTeller);
		ChestShow("dup", nSwap0);
		for (int n = 0; n < MAX_STORYCHEST; n ++) g_aStoryInfo[0]->awTakeChest[n] = g_aStoryInfo[1]->awTakeChest[n] = FLAG_NOBIT;
		memcpy(pOwner->abyChestOpened, abyChestBak, sizeof(abyChestBak));
		memcpy(pMap->aSwap, aSwapBak, sizeof(aSwapBak));
		pMap->bySwapCount = bySwapCountBak;
	}

	/* 依頼を取り消した時に戻すお話（wCancelStory）：L1（と後の L2）の印・持ち物・宝箱が最初に戻る。
	   0 は何もしない・完了（渡す）では戻さない・受けていない時の取り消しは何も起きない・パーティ離脱など他の経路では戻さない */
	{
		BYTE abyChestBak[MAX_CHESTBYTE];
		WORD awImpBak[MAX_IMPORTANT];
		memcpy(abyChestBak, pOwner->abyChestOpened, sizeof(abyChestBak));
		memcpy(awImpBak, pOwner->waImportant, sizeof(awImpBak));
		PQUESTINFO pQC = new QUESTINFO;
		ZeroMemory(pQC, sizeof(*pQC));
		pQC->wNeedFlag = pQC->wNotFlag = FLAG_NOBIT;
		pQC->wID = 3; pQC->byState = QUESTSTATE_ON; pQC->byGuild = 0; pQC->wSort = 3; pQC->byOnce = 0; pQC->byStepCount = 1;
		strcpy(pQC->szName, "取り消し依頼");
		for (int k = 0; k < MAX_QUESTDONESET; k ++) pQC->awDoneSetFlag[k] = FLAG_NOBIT;
		pQC->byDoneNumNo = FLAG_NONUM;
		for (int k = 0; k < MAX_QUESTNPC; k ++) { pQC->aNpc[k].byStepMax = QUESTNPC_NOSTEP; pQC->aNpc[k].byNextStep = QUESTNPC_NOSTEP; }
		g_aQuestInfo.Add(pQC);
		g_aStoryInfo[0]->awTakeItem[0] = 1; g_aStoryInfo[0]->awTakeChest[0] = 0;
		g_aStoryInfo[1]->awTakeItem[0] = 3; g_aStoryInfo[1]->awTakeChest[0] = 5;
		/* 進めた状態を作る（印 100・110・111・99、数値 5=3 6=9、品 1・3、宝箱 0・5・7） */
		auto CancelSetup = [&]() {
			ZeroMemory(pOwner->waImportant, sizeof(pOwner->waImportant));
			pOwner->waImportant[0] = 1; pOwner->waImportant[1] = 3; pOwner->waImportant[2] = 2;
			FlagBitSet(pOwner->abyFlag, 99, TRUE); FlagBitSet(pOwner->abyFlag, 100, TRUE);
			FlagBitSet(pOwner->abyFlag, 110, TRUE); FlagBitSet(pOwner->abyFlag, 111, TRUE);
			FlagNumSet(pOwner->abyFlagNum, 5, 3); FlagNumSet(pOwner->abyFlagNum, 6, 9);
			ZeroMemory(pOwner->abyChestOpened, sizeof(pOwner->abyChestOpened));
			ChestOpenedSet(pOwner, 0, TRUE); ChestOpenedSet(pOwner, 5, TRUE); ChestOpenedSet(pOwner, 7, TRUE);
			pOwner->abyQuestDone[0] |= (BYTE)((1 << 1) | (1 << 2));
		};
		/* 取り消した時に届いた CMD_QUEST の中に、追記の一文があったか（nNote に入れる） */
		int nNote = 0;
		auto CancelShow = [&](LPCSTR pszTitle, const std::string &) {
			printf("story cancel %s quest=%u flag99=%d,100=%d,110=%d,111=%d num5=%u,6=%u imp=%u,%u,%u chest0=%d,5=%d,7=%d done1=%d,2=%d note=%d\n", pszTitle, pOwner->wQuestID,
				FlagBitGet(pOwner->abyFlag, 99), FlagBitGet(pOwner->abyFlag, 100), FlagBitGet(pOwner->abyFlag, 110), FlagBitGet(pOwner->abyFlag, 111),
				FlagNumGet(pOwner->abyFlagNum, 5), FlagNumGet(pOwner->abyFlagNum, 6),
				pOwner->waImportant[0], pOwner->waImportant[1], pOwner->waImportant[2],
				(int)ChestOpenedGet(pOwner, 0), (int)ChestOpenedGet(pOwner, 5), (int)ChestOpenedGet(pOwner, 7),
				(int)QuestIsDone(pOwner, 1), (int)QuestIsDone(pOwner, 2),
				nNote);
		};
		auto DoCancelCap = [&](std::string &) {
			MockTrace(1);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			nNote = MockHasText(1000, CMD_QUEST, "（進めていた分は最初に戻りました）");
			MockTrace(0);
		};
		std::string strCap;
		/* L1 を指定：L1 と後の L2 の印・持ち物・宝箱が戻る（範囲外の印 99・指定外の宝箱 7・品 2 は残る。数値 5 は回数なので +1 しない） */
		pQC->wCancelStory = 1;
		CancelSetup();
		QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
		DoCancelCap(strCap);
		CancelShow("L1", strCap);
		/* L2 だけ指定：L2 の分だけ戻る */
		pQC->wCancelStory = 2;
		CancelSetup();
		QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
		DoCancelCap(strCap);
		CancelShow("L2", strCap);
		/* 0：何も戻らない（普通の取り消し文のまま） */
		pQC->wCancelStory = 0;
		CancelSetup();
		QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
		DoCancelCap(strCap);
		CancelShow("zero", strCap);
		/* 無いお話の番号：何も戻らない */
		pQC->wCancelStory = 200;
		CancelSetup();
		QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
		DoCancelCap(strCap);
		CancelShow("nostory", strCap);
		/* 依頼を受けていない時の取り消し：何も起きない */
		pQC->wCancelStory = 1;
		CancelSetup();
		DoCancelCap(strCap);
		CancelShow("noquest", strCap);
		/* 完了（渡す）では戻さない。そのあと取り消し操作をしても受けていないので何も起きない */
		CancelSetup();
		QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
		QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
		CancelShow("handin", "");
		DoCancelCap(strCap);
		CancelShow("handin+cancel", strCap);
		/* 他の取り消し経路（パーティ離脱で取り消される討伐チャレンジの依頼）では戻さない */
		pQC->byChallenge = QUESTCHALLENGE_ARENA;
		CancelSetup();
		pOwner->wQuestID = 3; pOwner->byQuestStep = 0; pOwner->wQuestFlg = 0;
		QuestOnPartyLeave(pOwner);
		CancelShow("partyleave", "");
		pQC->byChallenge = QUESTCHALLENGE_NONE;
		/* 保存して読み直しても残る・データAPI（0 は何もしない／存在するお話だけ／足す時に省くと 0／取得に出る） */
		pQC->wCancelStory = 2;
		SaveQuestInfo();
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
		g_aQuestInfo.RemoveAll();
		LoadQuestInfo();
		pQC = NULL;
		int nIdxC = 0;
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) { pQC = g_aQuestInfo[i]; nIdxC = i; }
		printf("story cancel reload=%u\n", pQC ? pQC->wCancelStory : 9999);
		if (pQC) {
			printf("story cancel api set %s", FlagGroupApi("quest", nIdxC, "{\"wCancelStory\":1}").c_str());
			printf(" now=%u get=%d\n", pQC->wCancelStory, SboWebDataGet("quest", nIdxC) && strstr(SboWebDataGet("quest", nIdxC), "\"wCancelStory\":1") ? 1 : 0);
			printf("story cancel api nostory %s", FlagGroupApi("quest", nIdxC, "{\"wCancelStory\":9}").c_str());
			printf(" now=%u\n", pQC->wCancelStory);
			printf("story cancel api over %s", FlagGroupApi("quest", nIdxC, "{\"wCancelStory\":256}").c_str());
			printf(" now=%u\n", pQC->wCancelStory);
			printf("story cancel api zero %s", FlagGroupApi("quest", nIdxC, "{\"wCancelStory\":0}").c_str());
			printf(" now=%u\n", pQC->wCancelStory);
		}
		for (int n = 0; n < MAX_STORYTAKEITEM; n ++) g_aStoryInfo[0]->awTakeItem[n] = g_aStoryInfo[1]->awTakeItem[n] = 0;
		for (int n = 0; n < MAX_STORYCHEST; n ++) g_aStoryInfo[0]->awTakeChest[n] = g_aStoryInfo[1]->awTakeChest[n] = FLAG_NOBIT;
		memcpy(pOwner->abyChestOpened, abyChestBak, sizeof(abyChestBak));
		memcpy(pOwner->waImportant, awImpBak, sizeof(awImpBak));
		pOwner->wQuestID = 0; pOwner->byQuestStep = 0; pOwner->wQuestFlg = 0;
	}

	/* 本が空に戻ったら（印を全部消す）また空 */
	ZeroMemory(pOwner->abyFlag, sizeof(pOwner->abyFlag));
	nList0 = MockCmdCount(1000, CMD_QUEST);
	QuestSendTalk(pOwner, dwTeller);
	printf("story empty again packets=%d\n", MockCmdCount(1000, CMD_QUEST) - nList0);

	/* 管理者ウィンドウ：足す・直す・断る（前のお話が無い／自分自身／輪になる） */
	{
		CMDPARA_ADMINSTORY Res;
		STORYINFO Info = *g_aStoryInfo[0];
		pAdmin->byAdminLV = ADMINLV_ADMINISTRATOR;
		BOOL bGot = AdminStorySend(pAdmin, ADMINSTORY_LISTREQ, 0, NULL, Res);
		printf("adminstory list got=%d count=%d id=%d name=%s\n", bGot, Res.wCount, Res.Info.wID, Res.Info.szName);
		Info.wID = 0; strcpy(Info.szName, "L3"); Info.wPrevStory = 2; Info.wClearFlag = 130;
		Info.awFlagFrom[0] = 130; Info.awFlagTo[0] = 131;
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, -1, &Info, Res);
		printf("adminstory add got=%d result=%d count=%d id=%d prev=%d\n", bGot, Res.byResult, g_aStoryInfo.GetSize(),
			(g_aStoryInfo.GetSize() > 3) ? g_aStoryInfo[3]->wID : -1, (g_aStoryInfo.GetSize() > 3) ? g_aStoryInfo[3]->wPrevStory : -1);
		Info = *g_aStoryInfo[0];
		Info.wPrevStory = 2;
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, 0, &Info, Res);
		printf("adminstory loop got=%d result=%d prev=%d\n", bGot, Res.byResult, g_aStoryInfo[0]->wPrevStory);
		Info = *g_aStoryInfo[0];
		Info.wPrevStory = 1;
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, 0, &Info, Res);
		printf("adminstory self got=%d result=%d prev=%d\n", bGot, Res.byResult, g_aStoryInfo[0]->wPrevStory);
		Info = *g_aStoryInfo[0];
		Info.awFlagFrom[0] = 50; Info.awFlagTo[0] = 40;
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, 0, &Info, Res);
		printf("adminstory range got=%d result=%d from=%u\n", bGot, Res.byResult, g_aStoryInfo[0]->awFlagFrom[0]);
		Info = *g_aStoryInfo[0];
		strcpy(Info.szName, "L1改"); Info.byState = STORYSTATE_OFF;
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, 0, &Info, Res);
		printf("adminstory edit got=%d result=%d name=%s state=%d id=%u\n", bGot, Res.byResult, g_aStoryInfo[0]->szName,
			g_aStoryInfo[0]->byState, g_aStoryInfo[0]->wID);
		pAdmin->byAdminLV = ADMINLV_USER;
	}

	/* お話の保存と読み込み：印 0 の範囲・数値 0・「無し」が崩れない */
	{
		g_aStoryInfo.RemoveAll();
		g_aStoryInfo.Add(StoryTestNew(7, "保存", 0, 0, 0, 0, 0, FLAG_NONUM));
		g_aStoryInfo[0]->abyNum[0] = 0;
		g_aStoryInfo[0]->awQuest[1] = 2;
		g_aStoryInfo[0]->byCountNum = 0;
		g_aStoryInfo[0]->awTakeItem[0] = 2; g_aStoryInfo[0]->awTakeItem[7] = 5;
		g_aStoryInfo[0]->awTakeChest[0] = 0; g_aStoryInfo[0]->awTakeChest[7] = 2047;		/* 宝箱 0 と最後の 2047 */
		BOOL bSaved = SaveStoryInfo();
		g_aStoryInfo.RemoveAll();
		BOOL bLoaded = LoadStoryInfo();
		PSTORYINFO p = (g_aStoryInfo.GetSize() > 0) ? g_aStoryInfo[0] : NULL;
		if (p) {
			printf("story save=%d load=%d count=%d id=%u name=%s clear=%u prev=%u from=%u,%u to=%u,%u num=%u,%u quest=%u,%u count=%u\n", (int)bSaved, (int)bLoaded,
				g_aStoryInfo.GetSize(), p->wID, p->szName, p->wClearFlag, p->wPrevStory, p->awFlagFrom[0], p->awFlagFrom[1], p->awFlagTo[0],
				p->awFlagTo[1], p->abyNum[0], p->abyNum[1], p->awQuest[0], p->awQuest[1], p->byCountNum);
			printf("story save take=%u,%u,%u,%u,%u,%u,%u,%u\n", p->awTakeItem[0], p->awTakeItem[1], p->awTakeItem[2], p->awTakeItem[3],
				p->awTakeItem[4], p->awTakeItem[5], p->awTakeItem[6], p->awTakeItem[7]);
			printf("story save chest=%u,%u,%u,%u,%u,%u,%u,%u\n", p->awTakeChest[0], p->awTakeChest[1], p->awTakeChest[2], p->awTakeChest[3],
				p->awTakeChest[4], p->awTakeChest[5], p->awTakeChest[6], p->awTakeChest[7]);
		} else {
			printf("story save=%d load=%d not found\n", (int)bSaved, (int)bLoaded);
		}
	}

	/* 片付け */
	g_aStoryInfo.RemoveAll();
	for (size_t i = 0; i < aStoryBack.size(); i ++) g_aStoryInfo.Add(aStoryBack[i]);
	g_aQuestInfo.RemoveAll();
	for (size_t i = 0; i < aQuestBack.size(); i ++) g_aQuestInfo.Add(aQuestBack[i]);
	pAdmin->byAdminLV = byAdminBack;
	for (int i = 0; i < 3; i ++) {
		apPC[i]->wQuestID = 0; apPC[i]->byQuestStep = 0; apPC[i]->wQuestFlg = 0;
		ZeroMemory(apPC[i]->abyQuestDone, sizeof(apPC[i]->abyQuestDone));
		ZeroMemory(apPC[i]->abyQuestRewarded, sizeof(apPC[i]->abyQuestRewarded));
		ZeroMemory(apPC[i]->abyFlag, sizeof(apPC[i]->abyFlag));
		ZeroMemory(apPC[i]->abyFlagNum, sizeof(apPC[i]->abyFlagNum));
		apPC[i]->dwGold = dwGoldBack[i];
		QuestMovePC(apPC[i], (int)dwMapBack[i], (int)dwXBack[i], (int)dwYBack[i]);
	}
	for (int n = 0; n < 2; n ++) {
		DeleteChar(apNpc[n]->dwTblID);
	}
}

/* ------------------------------------------------------------------------- */
/* AF. フラグのグループ（メイン-サブ）：足す・重なりの拒否・保存と読み直し・メイン-サブの計算・データAPI・管理者ウィンドウ */
/* ------------------------------------------------------------------------- */
extern "C" const char *SboWebDataApply(const char *, int, const char *, int, const char *);
extern "C" const char *SboWebDataGet(const char *, int);
extern BOOL SaveFlagGroupInfo(void);
extern BOOL LoadFlagGroupInfo(void);
extern BOOL SaveFlagNameInfo(void);

static BOOL AdminFlagGroupSend(PCHARINFO p, BYTE bySub, int nNo, const FLAGGROUPINFO *pInfo, CMDPARA_ADMINFLAGGROUP &Res)
{
	PCMDPARA_ADMINFLAGGROUP pPara = new CMDPARA_ADMINFLAGGROUP;
	std::vector<BYTE> aCap;
	BOOL bRet;

	ZeroMemory(pPara, sizeof(*pPara));
	pPara->byCmd = CMD_ADMINFLAGGROUP;
	pPara->byCmdSub = bySub;
	pPara->nNo = nNo;
	if (pInfo) pPara->Info = *pInfo;
	SvrSendCaptureBegin();
	TestRecv(p, pPara, sizeof(CMDPARA_ADMINFLAGGROUP));
	bRet = SvrSendCaptureEnd(aCap);
	delete pPara;
	ZeroMemory(&Res, sizeof(Res));
	if (!bRet || aCap.size() < sizeof(CMDPARA_ADMINFLAGGROUP)) return FALSE;
	memcpy(&Res, &aCap[0], sizeof(Res));
	return TRUE;
}

/* データAPIで書き込み、返事の ok と（だめなら）error を 1 行にする */
static std::string FlagGroupApi(const char *pszKind, int nID, const char *pszJson)
{
	const char *psz = SboWebDataApply(pszKind, nID, pszJson, 1, "テスト");
	std::string str = psz ? psz : "(null)";
	/* record の中身は長いので、ok と error だけ見せる */
	size_t nPos = str.find("\"record\"");
	if (nPos != std::string::npos) str = str.substr(0, nPos) + "...}";
	return str;
}

/* ------------------------------------------------------------------------- */
/* AG. 物語の「いまの目的」：行の選び方・コマンド窓への送信・データAPIの読み書き・保存と読み直し */
/* ------------------------------------------------------------------------- */
static void StoryGoalRow(STORYGOAL *g, LPCSTR pszGoal)
{
	memset(g, 0, sizeof(*g));
	for (int i = 0; i < MAX_TALKNEED; i ++) g->awNeed[i] = g->awNot[i] = FLAG_NOBIT;
	for (int i = 0; i < MAX_TALKCOND; i ++) g->abyCondNo[i] = FLAG_NONUM;
	strcpy(g->szGoal, pszGoal);
}

static void StoryGoalClear(STORYINFO *p)
{
	for (int i = 0; i < MAX_STORYGOAL; i ++) StoryGoalRow(&p->aGoal[i], "");
}

/* そのキャラへ届く CMD_QUESTSTATE の物語の目的を 1 行にする */
static std::string StoryGoalSent(PCHARINFO p)
{
	std::vector<BYTE> aCap;
	std::string str;

	SvrSendCaptureBegin();
	QuestSendStateSelf(p);
	if (!SvrSendCaptureEnd(aCap) || aCap.size() < offsetof(CMDPARA_QUESTSTATE, aStory)) return "(none)";
	CMDPARA_QUESTSTATE Para;
	memset(&Para, 0, sizeof(Para));
	memcpy(&Para, &aCap[0], (aCap.size() < sizeof(Para)) ? aCap.size() : sizeof(Para));
	char sz[64];
	snprintf(sz, sizeof(sz), "size=%u count=%u [", (unsigned)aCap.size(), (unsigned)Para.byStoryCount);
	str = sz;
	for (int i = 0; i < Para.byStoryCount && i < MAX_STORYGOAL; i ++) {
		str += std::string(i ? "," : "") + Para.aStory[i].szName + ":" + Para.aStory[i].szGoal;
	}
	return str + "]";
}

static void TestStoryGoal(const std::vector<int> &aLogin)
{
	printf("== AG. StoryGoal ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	std::vector<PSTORYINFO> aStoryBack;
	BYTE abyFlagBack[sizeof(pPC->abyFlag)], abyNumBack[sizeof(pPC->abyFlagNum)];
	memcpy(abyFlagBack, pPC->abyFlag, sizeof(abyFlagBack));
	memcpy(abyNumBack, pPC->abyFlagNum, sizeof(abyNumBack));
	for (int i = 0; i < g_aStoryInfo.GetSize(); i ++) aStoryBack.push_back(g_aStoryInfo[i]);
	g_aStoryInfo.RemoveAll();
	ZeroMemory(pPC->abyFlag, sizeof(pPC->abyFlag));
	ZeroMemory(pPC->abyFlagNum, sizeof(pPC->abyFlagNum));

	/* 行の選び方：飛ばす行（文が空）・印と数値の条件・上から最初に合う行 */
	PSTORYINFO pA = StoryTestNew(11, "港の話", 200, 0, 200, 200, 0, FLAG_NONUM);
	StoryGoalClear(pA);
	StoryGoalRow(&pA->aGoal[0], "");									/* 文が空の行は条件に合っても飛ばす */
	pA->aGoal[0].awNeed[0] = 1;
	StoryGoalRow(&pA->aGoal[1], "ねらい1");							/* 印 1 が立っていて、数値 3 がちょうど 2 */
	pA->aGoal[1].awNeed[0] = 1; pA->aGoal[1].abyCondNo[0] = 3; pA->aGoal[1].abyCondCmp[0] = TALKCMP_EQ; pA->aGoal[1].abyCondVal[0] = 2;
	StoryGoalRow(&pA->aGoal[2], "ねらい2");							/* 印 1 が立っていて、印 9 が立っていない */
	pA->aGoal[2].awNeed[0] = 1; pA->aGoal[2].awNot[0] = 9;
	StoryGoalRow(&pA->aGoal[3], "ねらい3");							/* 数値 4 が 2 以下（比べ方 LE）。印 0 を使う行 */
	pA->aGoal[3].abyCondNo[0] = 4; pA->aGoal[3].abyCondCmp[0] = TALKCMP_LE; pA->aGoal[3].abyCondVal[0] = 2; pA->aGoal[3].awNot[0] = 0;
	g_aStoryInfo.Add(pA);
	BYTE *pF = pPC->abyFlag, *pN = pPC->abyFlagNum;
	printf("storygoal pick none=%d", StoryGoalPick(pA, pF, pN));			/* 数値 4=0 なので LE に合う → 3 */
	FlagBitSet(pF, 0, TRUE);
	printf(" flag0=%d", StoryGoalPick(pA, pF, pN));						/* 印 0 が立つと行 3 は外れる → -1 */
	FlagBitSet(pF, 1, TRUE);
	printf(" flag1=%d", StoryGoalPick(pA, pF, pN));						/* → 2 */
	FlagNumSet(pN, 3, 2);
	printf(" num3eq2=%d", StoryGoalPick(pA, pF, pN));						/* → 1 */
	FlagNumSet(pN, 3, 3);
	FlagBitSet(pF, 9, TRUE);
	printf(" flag9=%d", StoryGoalPick(pA, pF, pN));						/* 行 2 は外れ、行 3 も印 0 で外れる → -1 */
	FlagBitSet(pF, 0, FALSE);
	FlagNumSet(pN, 4, 3);
	printf(" num4=3 => %d", StoryGoalPick(pA, pF, pN));
	FlagNumSet(pN, 4, 2);
	printf(" num4=2 => %d\n", StoryGoalPick(pA, pF, pN));
	StoryGoalClear(pA);
	printf("storygoal empty=%d\n", StoryGoalPick(pA, pF, pN));

	/* 送信：wSort 順（クリアの印は見ない。消すのは行の条件で）・止めたシナリオと合う行が無いシナリオは出ない */
	ZeroMemory(pPC->abyFlag, sizeof(pPC->abyFlag));
	ZeroMemory(pPC->abyFlagNum, sizeof(pPC->abyFlagNum));
	StoryGoalClear(pA);
	StoryGoalRow(&pA->aGoal[0], "ルシーに話す");
	pA->aGoal[0].awNot[0] = 100;
	StoryGoalRow(&pA->aGoal[1], "灯台へ行く");
	pA->aGoal[1].awNeed[0] = 100;
	pA->wSort = 20;
	PSTORYINFO pB = StoryTestNew(12, "森の話", 201, 0, 201, 201, 0, FLAG_NONUM);
	StoryGoalClear(pB);
	StoryGoalRow(&pB->aGoal[0], "木を切る");
	pB->aGoal[0].awNot[0] = 201;										/* クリアで消したい時はクリアの印を awNot に */
	pB->wSort = 10;
	PSTORYINFO pC = StoryTestNew(13, "止めた話", 202, 0, 202, 202, 0, FLAG_NONUM);
	StoryGoalClear(pC);
	StoryGoalRow(&pC->aGoal[0], "出ない");
	pC->byState = STORYSTATE_OFF;
	PSTORYINFO pD = StoryTestNew(14, "目的なし", 203, 0, 203, 203, 0, FLAG_NONUM);
	StoryGoalClear(pD);
	PSTORYINFO pE = StoryTestNew(15, "合わない話", 204, 0, 204, 204, 0, FLAG_NONUM);
	StoryGoalClear(pE);
	StoryGoalRow(&pE->aGoal[0], "合わない");
	pE->aGoal[0].awNeed[0] = 500;
	g_aStoryInfo.RemoveAll();											/* 最初の確認で足した pA を外して、並びを作り直す */
	g_aStoryInfo.Add(pA); g_aStoryInfo.Add(pB); g_aStoryInfo.Add(pC); g_aStoryInfo.Add(pD); g_aStoryInfo.Add(pE);
	printf("storygoal sent start %s\n", StoryGoalSent(pPC).c_str());
	FlagBitSet(pPC->abyFlag, 100, TRUE);
	printf("storygoal sent flag100 %s\n", StoryGoalSent(pPC).c_str());
	FlagBitSet(pPC->abyFlag, 201, TRUE);									/* 森の話をクリア → awNot で出ない */
	printf("storygoal sent clear-B %s\n", StoryGoalSent(pPC).c_str());
	FlagBitSet(pPC->abyFlag, 200, TRUE);									/* 港の話をクリア → 条件が合うので出たまま */
	printf("storygoal sent clear-A %s\n", StoryGoalSent(pPC).c_str());

	/* 印・数値が動いた時の送り直し：会話データで話した後（動いた時だけ。同じ話をもう一度しても動かなければ送らない） */
	{
		ZeroMemory(pPC->abyFlag, sizeof(pPC->abyFlag));
		ZeroMemory(pPC->abyFlagNum, sizeof(pPC->abyFlagNum));
		g_aStoryInfo.RemoveAll();
		PSTORYINFO pS = StoryTestNew(21, "再送", 230, 0, 230, 230, 0, FLAG_NONUM);
		StoryGoalClear(pS);
		StoryGoalRow(&pS->aGoal[0], "次へ");
		pS->aGoal[0].awNeed[0] = 231;
		g_aStoryInfo.Add(pS);
		PTALKINFO pTalk = new TALKINFO;
		ZeroMemory(pTalk, sizeof(*pTalk));
		pTalk->wID = 997; pTalk->byState = TALKSTATE_ON; pTalk->byLineCount = 1;
		TALKLINEINFO *pLine = &pTalk->aLine[0];
		for (int i = 0; i < MAX_TALKNEED; i ++) pLine->awNeed[i] = pLine->awNot[i] = FLAG_NOBIT;
		for (int i = 0; i < MAX_TALKCOND; i ++) pLine->abyCondNo[i] = FLAG_NONUM;
		for (int i = 0; i < MAX_TALKSET; i ++) pLine->awSetOn[i] = pLine->awSetOff[i] = FLAG_NOBIT;
		for (int i = 0; i < MAX_TALKACT; i ++) pLine->abyActNo[i] = FLAG_NONUM;
		pLine->byQuestStepMax = QUESTNPC_NOSTEP;
		pLine->awSetOn[0] = 231;
		strcpy(pLine->szSpeech, "進んだ");
		g_aTalkInfo.Add(pTalk);
		CHARINFO Npc;
		char szSpeech[8];
		ZeroMemory((PBYTE)&Npc, sizeof(CHARINFO));
		strcpy(szSpeech, "@t997");
		Npc.pszNpcSpeech = szSpeech;
		Npc.dwTblID = 1;
		int nState0 = MockCmdCount(1000, CMD_QUESTSTATE);
		TalkNpcTalk(pPC, &Npc);
		int nState1 = MockCmdCount(1000, CMD_QUESTSTATE);
		TalkNpcTalk(pPC, &Npc);
		int nState2 = MockCmdCount(1000, CMD_QUESTSTATE);
		printf("storygoal resend talk=%d again=%d flag231=%d %s\n", nState1 - nState0, nState2 - nState1, FlagBitGet(pPC->abyFlag, 231) ? 1 : 0, StoryGoalSent(pPC).c_str());
		g_aTalkInfo.RemoveAt(g_aTalkInfo.GetSize() - 1);
		delete pTalk;
	}

	/* データAPI：goals の読み書き（丸ごと置き換え・省くと今のまま・短い配列は残りが無し・おかしい値は断る） */
	g_aStoryInfo.RemoveAll();
	printf("storygoal api add %s\n", FlagGroupApi("story", 0, "{\"szName\":\"API\",\"wClearFlag\":300,\"goals\":[{\"awNeed\":[5],\"abyCondNo\":[7],\"abyCondCmp\":[2],\"abyCondVal\":[3],\"szGoal\":\"一つめ\"},{\"szGoal\":\"二つめ\"}]}").c_str());
	printf("storygoal api get %s\n", SboWebDataGet("story", 0));
	printf("storygoal api internal need=%u,%u not=%u no=%u,%u cmp=%u,%u val=%u row1need=%u row1no=%u row2=[%s] pick=%d\n", g_aStoryInfo[0]->aGoal[0].awNeed[0], g_aStoryInfo[0]->aGoal[0].awNeed[1],
		g_aStoryInfo[0]->aGoal[0].awNot[0], g_aStoryInfo[0]->aGoal[0].abyCondNo[0], g_aStoryInfo[0]->aGoal[0].abyCondNo[1], g_aStoryInfo[0]->aGoal[0].abyCondCmp[0],
		g_aStoryInfo[0]->aGoal[0].abyCondCmp[1], g_aStoryInfo[0]->aGoal[0].abyCondVal[0], g_aStoryInfo[0]->aGoal[1].awNeed[0], g_aStoryInfo[0]->aGoal[1].abyCondNo[0],
		g_aStoryInfo[0]->aGoal[2].szGoal, StoryGoalPick(g_aStoryInfo[0], pPC->abyFlag, pPC->abyFlagNum));
	printf("storygoal api keep %s goals=[%s]\n", FlagGroupApi("story", 0, "{\"szName\":\"API改\"}").c_str(), g_aStoryInfo[0]->aGoal[0].szGoal);
	printf("storygoal api replace %s row0=[%s] row1=[%s]\n", FlagGroupApi("story", 0, "{\"goals\":[{\"szGoal\":\"置換\"}]}").c_str(), g_aStoryInfo[0]->aGoal[0].szGoal, g_aStoryInfo[0]->aGoal[1].szGoal);
	printf("storygoal api get2 %s\n", SboWebDataGet("story", 0));
	printf("storygoal api toomany %s\n", FlagGroupApi("story", 0, "{\"goals\":[{},{},{},{},{},{},{},{},{}]}").c_str());
	printf("storygoal api badflag %s\n", FlagGroupApi("story", 0, "{\"goals\":[{\"awNeed\":[2000],\"szGoal\":\"x\"}]}").c_str());
	printf("storygoal api badnum %s\n", FlagGroupApi("story", 0, "{\"goals\":[{\"abyCondNo\":[200],\"szGoal\":\"x\"}]}").c_str());
	printf("storygoal api badcmp %s\n", FlagGroupApi("story", 0, "{\"goals\":[{\"abyCondNo\":[1],\"abyCondCmp\":[3],\"szGoal\":\"x\"}]}").c_str());
	printf("storygoal api badkey %s\n", FlagGroupApi("story", 0, "{\"goals\":[{\"zzz\":1}]}").c_str());
	printf("storygoal api kept after-error row0=[%s]\n", g_aStoryInfo[0]->aGoal[0].szGoal);
	printf("storygoal api clear %s get=%s\n", FlagGroupApi("story", 0, "{\"goals\":[]}").c_str(), SboWebDataGet("story", 0));

	/* 取り上げる持ち物（データAPI）：入れる・省いたら今のまま・存在しない item 番号は断る */
	{
		std::string strOk = FlagGroupApi("story", 0, "{\"awTakeItem\":[1,2]}");
		printf("storytake api set ok=%d take=%u,%u,%u\n", strOk.find("\"ok\":true") != std::string::npos, g_aStoryInfo[0]->awTakeItem[0], g_aStoryInfo[0]->awTakeItem[1], g_aStoryInfo[0]->awTakeItem[2]);
		FlagGroupApi("story", 0, "{\"szName\":\"API改2\"}");
		printf("storytake api keep take=%u,%u\n", g_aStoryInfo[0]->awTakeItem[0], g_aStoryInfo[0]->awTakeItem[1]);
		std::string strBad = FlagGroupApi("story", 0, "{\"awTakeItem\":[65000]}");
		printf("storytake api bad ng=%d take=%u,%u\n", strBad.find("\"ok\":false") != std::string::npos, g_aStoryInfo[0]->awTakeItem[0], g_aStoryInfo[0]->awTakeItem[1]);
		strBad = FlagGroupApi("story", 0, "{\"awTakeItem\":[1,2,3,4,5,6,7,8,9]}");
		printf("storytake api toomany ng=%d\n", strBad.find("\"ok\":false") != std::string::npos);
		printf("storytake api get %s\n", strstr(SboWebDataGet("story", 0), "\"awTakeItem\":[1,2,0,0,0,0,0,0]") ? "has" : "none");
	}

	/* 戻す宝箱（データAPI）：入れる（0 も）・短い配列の残りは無し(65535)・省いたら今のまま・範囲外・多すぎは断る */
	{
		std::string strOk = FlagGroupApi("story", 0, "{\"awTakeChest\":[0,2047,5]}");
		printf("storychest api set ok=%d chest=%u,%u,%u,%u\n", strOk.find("\"ok\":true") != std::string::npos, g_aStoryInfo[0]->awTakeChest[0],
			g_aStoryInfo[0]->awTakeChest[1], g_aStoryInfo[0]->awTakeChest[2], g_aStoryInfo[0]->awTakeChest[3]);
		FlagGroupApi("story", 0, "{\"szName\":\"API改3\"}");
		printf("storychest api keep chest=%u,%u,%u\n", g_aStoryInfo[0]->awTakeChest[0], g_aStoryInfo[0]->awTakeChest[1], g_aStoryInfo[0]->awTakeChest[2]);
		std::string strBad = FlagGroupApi("story", 0, "{\"awTakeChest\":[2048]}");
		printf("storychest api bad ng=%d chest=%u,%u\n", strBad.find("\"ok\":false") != std::string::npos, g_aStoryInfo[0]->awTakeChest[0], g_aStoryInfo[0]->awTakeChest[1]);
		strBad = FlagGroupApi("story", 0, "{\"awTakeChest\":[1,2,3,4,5,6,7,8,9]}");
		printf("storychest api toomany ng=%d\n", strBad.find("\"ok\":false") != std::string::npos);
		printf("storychest api get %s\n", strstr(SboWebDataGet("story", 0), "\"awTakeChest\":[0,2047,5,65535,65535,65535,65535,65535]") ? "has" : "none");
		/* 足す時に省くと全部無し（0 にならない） */
		std::string strAdd = FlagGroupApi("story", g_aStoryInfo.GetSize(), "{\"szName\":\"宝箱なし\",\"wClearFlag\":321}");
		int nLast = g_aStoryInfo.GetSize() - 1;
		printf("storychest api add ok=%d chest=%u,%u,%u\n", strAdd.find("\"ok\":true") != std::string::npos, g_aStoryInfo[nLast]->awTakeChest[0],
			g_aStoryInfo[nLast]->awTakeChest[1], g_aStoryInfo[nLast]->awTakeChest[7]);
		if (nLast > 0) { PSTORYINFO pDel = g_aStoryInfo[nLast]; g_aStoryInfo.RemoveAt(nLast); delete pDel; }
		FlagGroupApi("story", 0, "{\"awTakeChest\":[]}");
		printf("storychest api clear chest=%u\n", g_aStoryInfo[0]->awTakeChest[0]);
	}

	/* 管理者ウィンドウの書き込み（目的の欄は無い）：保存しても目的が消えない・足す時は目的なし */
	{
		PCHARINFO pAdmin = pPC;
		BYTE byAdminBack = pAdmin->byAdminLV;
		CMDPARA_ADMINSTORY Res;
		FlagGroupApi("story", 0, "{\"goals\":[{\"awNeed\":[5],\"szGoal\":\"残る目的\"}]}");
		pAdmin->byAdminLV = ADMINLV_ADMINISTRATOR;
		STORYINFO Info = *g_aStoryInfo[0];
		memset(Info.aGoal, 0, sizeof(Info.aGoal));						/* 目的の欄を知らない（0 で埋まった）通信 */
		strcpy(Info.szName, "管理者改");
		BOOL bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, 0, &Info, Res);
		printf("storygoal admin edit got=%d result=%d name=%s goal=[%s] need=%u\n", bGot, Res.byResult, g_aStoryInfo[0]->szName,
			g_aStoryInfo[0]->aGoal[0].szGoal, g_aStoryInfo[0]->aGoal[0].awNeed[0]);
		printf("storytake admin keep take=%u,%u\n", g_aStoryInfo[0]->awTakeItem[0], g_aStoryInfo[0]->awTakeItem[1]);
		Info.wID = 0; strcpy(Info.szName, "管理者新");
		bGot = AdminStorySend(pAdmin, ADMINSTORY_SET, -1, &Info, Res);
		printf("storygoal admin add got=%d result=%d count=%d goal=[%s]\n", bGot, Res.byResult, g_aStoryInfo.GetSize(),
			(g_aStoryInfo.GetSize() > 1) ? g_aStoryInfo[1]->aGoal[0].szGoal : "(none)");
		pAdmin->byAdminLV = byAdminBack;
	}

	/* 保存と読み直し：目的が残る・印 0・数値 0 の条件が崩れない */
	{
		g_aStoryInfo.RemoveAll();
		PSTORYINFO pS = StoryTestNew(31, "保存", 0, 0, 0, 0, 0, FLAG_NONUM);
		StoryGoalClear(pS);
		StoryGoalRow(&pS->aGoal[0], "ゼロの条件");
		pS->aGoal[0].awNeed[0] = 0; pS->aGoal[0].awNot[1] = 1023; pS->aGoal[0].abyCondNo[0] = 0; pS->aGoal[0].abyCondCmp[0] = TALKCMP_LE; pS->aGoal[0].abyCondVal[0] = 9;
		StoryGoalRow(&pS->aGoal[7], "最後の行");
		g_aStoryInfo.Add(pS);
		BOOL bSaved = SaveStoryInfo();
		g_aStoryInfo.RemoveAll();
		BOOL bLoaded = LoadStoryInfo();
		PSTORYINFO p = (g_aStoryInfo.GetSize() > 0) ? g_aStoryInfo[0] : NULL;
		if (p) {
			printf("storygoal save=%d load=%d row0=[%s] need=%u,%u not=%u,%u no=%u,%u cmp=%u val=%u row1=[%s] row7=[%s] row7need=%u row7no=%u\n", (int)bSaved, (int)bLoaded,
				p->aGoal[0].szGoal, p->aGoal[0].awNeed[0], p->aGoal[0].awNeed[1], p->aGoal[0].awNot[0], p->aGoal[0].awNot[1], p->aGoal[0].abyCondNo[0],
				p->aGoal[0].abyCondNo[1], p->aGoal[0].abyCondCmp[0], p->aGoal[0].abyCondVal[0], p->aGoal[1].szGoal, p->aGoal[7].szGoal, p->aGoal[7].awNeed[0], p->aGoal[7].abyCondNo[0]);
		} else {
			printf("storygoal save=%d load=%d not found\n", (int)bSaved, (int)bLoaded);
		}
	}

	/* 片付け */
	g_aStoryInfo.RemoveAll();
	for (size_t i = 0; i < aStoryBack.size(); i ++) g_aStoryInfo.Add(aStoryBack[i]);
	SaveStoryInfo();
	memcpy(pPC->abyFlag, abyFlagBack, sizeof(abyFlagBack));
	memcpy(pPC->abyFlagNum, abyNumBack, sizeof(abyNumBack));
}

static void TestFlagGroup(const std::vector<int> &aLogin)
{
	printf("== AF. FlagGroup ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pAdmin = g_aCharInfo[aLogin[0]];
	BYTE byAdminBack = pAdmin->byAdminLV;
	std::vector<PFLAGGROUPINFO> aGroupBack;
	std::vector<PFLAGNAMEINFO> aNameBack;
	for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) aGroupBack.push_back(g_aFlagGroupInfo[i]);
	for (int i = 0; i < g_aFlagNameInfo.GetSize(); i ++) aNameBack.push_back(g_aFlagNameInfo[i]);
	g_aFlagGroupInfo.RemoveAll();
	g_aFlagNameInfo.RemoveAll();

	/* 足す：番号は 1 から自動。ルシー＝印 100〜139 */
	printf("flaggroup add1 %s count=%d\n", FlagGroupApi("flagGroup", 0, "{\"byKind\":0,\"szName\":\"ルシー\",\"wFrom\":100,\"wTo\":139,\"wSort\":2}").c_str(), g_aFlagGroupInfo.GetSize());
	printf("flaggroup add2 %s count=%d\n", FlagGroupApi("flagGroup", 1, "{\"szName\":\"リノ\",\"wFrom\":140,\"wTo\":149}").c_str(), g_aFlagGroupInfo.GetSize());
	/* 同じ種類で帯が重なる・端が 1 つだけ重なる・全部を含む・中に入る */
	printf("flaggroup overlap-mid %s count=%d\n", FlagGroupApi("flagGroup", 2, "{\"szName\":\"重\",\"wFrom\":130,\"wTo\":145}").c_str(), g_aFlagGroupInfo.GetSize());
	printf("flaggroup overlap-edge %s count=%d\n", FlagGroupApi("flagGroup", 2, "{\"szName\":\"重\",\"wFrom\":149,\"wTo\":160}").c_str(), g_aFlagGroupInfo.GetSize());
	printf("flaggroup overlap-in %s count=%d\n", FlagGroupApi("flagGroup", 2, "{\"szName\":\"重\",\"wFrom\":110,\"wTo\":110}").c_str(), g_aFlagGroupInfo.GetSize());
	/* 種類が違えば同じ番号の帯でもよい（数値 100 以上は無いので 10〜20）。範囲外・逆さ・名前なし・帯なしは断る */
	printf("flaggroup num %s count=%d\n", FlagGroupApi("flagGroup", 2, "{\"byKind\":1,\"szName\":\"数\",\"wFrom\":10,\"wTo\":20}").c_str(), g_aFlagGroupInfo.GetSize());
	printf("flaggroup num-range %s\n", FlagGroupApi("flagGroup", 3, "{\"byKind\":1,\"szName\":\"数2\",\"wFrom\":120,\"wTo\":130}").c_str());
	printf("flaggroup bit-range %s\n", FlagGroupApi("flagGroup", 3, "{\"szName\":\"印2\",\"wFrom\":1000,\"wTo\":1024}").c_str());
	printf("flaggroup reverse %s\n", FlagGroupApi("flagGroup", 3, "{\"szName\":\"印2\",\"wFrom\":50,\"wTo\":40}").c_str());
	printf("flaggroup noname %s\n", FlagGroupApi("flagGroup", 3, "{\"szName\":\"\",\"wFrom\":50,\"wTo\":60}").c_str());
	printf("flaggroup noband %s\n", FlagGroupApi("flagGroup", 3, "{\"szName\":\"印2\"}").c_str());
	printf("flaggroup badwid %s\n", FlagGroupApi("flagGroup", 0, "{\"wID\":9}").c_str());
	printf("flaggroup badkey %s\n", FlagGroupApi("flagGroup", 0, "{\"zzz\":1}").c_str());
	/* 直す：自分自身の帯は重なりに数えない（縮めてまた戻せる）。隣へはみ出すのは断る */
	printf("flaggroup shrink %s from=%u to=%u\n", FlagGroupApi("flagGroup", 0, "{\"wTo\":138}").c_str(), g_aFlagGroupInfo[0]->awFrom[0], g_aFlagGroupInfo[0]->awTo[0]);
	printf("flaggroup grow-over %s to=%u\n", FlagGroupApi("flagGroup", 0, "{\"wTo\":140}").c_str(), g_aFlagGroupInfo[0]->awTo[0]);
	printf("flaggroup grow-back %s to=%u\n", FlagGroupApi("flagGroup", 0, "{\"wTo\":139}").c_str(), g_aFlagGroupInfo[0]->awTo[0]);

	/* メイン-サブの計算（kind "flag" の記録に group・sub が付く。名前表の中の番号は変わらない） */
	printf("flaggroup name103 %s\n", FlagGroupApi("flag", 0, "{\"byKind\":0,\"wNo\":103,\"szName\":\"港町の事件\"}").c_str());
	printf("flaggroup name100 %s\n", FlagGroupApi("flag", 1, "{\"byKind\":0,\"wNo\":100,\"szName\":\"先頭\"}").c_str());
	printf("flaggroup name139 %s\n", FlagGroupApi("flag", 2, "{\"byKind\":0,\"wNo\":139,\"szName\":\"最後\"}").c_str());
	printf("flaggroup name140 %s\n", FlagGroupApi("flag", 3, "{\"byKind\":0,\"wNo\":140,\"szName\":\"次の先頭\"}").c_str());
	printf("flaggroup name50 %s\n", FlagGroupApi("flag", 4, "{\"byKind\":0,\"wNo\":50,\"szName\":\"どこにも入らない\"}").c_str());
	printf("flaggroup name-num15 %s\n", FlagGroupApi("flag", 5, "{\"byKind\":1,\"wNo\":15,\"szName\":\"数値15\"}").c_str());
	printf("flaggroup name-num100 %s\n", FlagGroupApi("flag", 6, "{\"byKind\":1,\"wNo\":100,\"szName\":\"範囲外\"}").c_str());
	for (int i = 0; i < 6; i ++) {
		printf("flaggroup get flag %d %s\n", i, SboWebDataGet("flag", i));
	}
	/* group・sub は読むだけ（同じ値を書き戻すのはよい。違う値・追加は断る） */
	printf("flaggroup ro-same %s\n", FlagGroupApi("flag", 0, "{\"group\":\"ルシー\",\"sub\":4,\"szName\":\"港町の事件2\"}").c_str());
	printf("flaggroup ro-diff %s\n", FlagGroupApi("flag", 0, "{\"sub\":9}").c_str());
	printf("flaggroup ro-add %s\n", FlagGroupApi("flag", 7, "{\"byKind\":0,\"wNo\":60,\"szName\":\"x\",\"group\":\"ルシー\"}").c_str());

	/* 削除（byState=0）：グループから外れ、帯はほかのグループが使える（枠は残る） */
	printf("flaggroup del %s count=%d state=%d\n", FlagGroupApi("flagGroup", 1, "{\"byState\":0}").c_str(), g_aFlagGroupInfo.GetSize(), g_aFlagGroupInfo[1]->byState);
	printf("flaggroup get flag 3 after del %s\n", SboWebDataGet("flag", 3));
	printf("flaggroup reuse %s count=%d id=%u\n", FlagGroupApi("flagGroup", g_aFlagGroupInfo.GetSize(), "{\"szName\":\"リノ2\",\"wFrom\":140,\"wTo\":150}").c_str(),
			g_aFlagGroupInfo.GetSize(), g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1]->wID);
	/* 削除済みの枠を戻すと、重なっていれば断る */
	printf("flaggroup revive %s state=%d\n", FlagGroupApi("flagGroup", 1, "{\"byState\":1}").c_str(), g_aFlagGroupInfo[1]->byState);
	printf("flaggroup get 0 %s\n", SboWebDataGet("flagGroup", 0));
	printf("flaggroup get 9 %s\n", SboWebDataGet("flagGroup", 9));


	/* 帯を 2 本以上持つグループ：サブは帯の順に続けて数える（印 400〜439=サブ 1〜40・500〜519=サブ 41〜60） */
	printf("flaggroup multi-add %s count=%d\n", FlagGroupApi("flagGroup", g_aFlagGroupInfo.GetSize(), "{\"szName\":\"多帯\",\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":500,\"wTo\":519}]}").c_str(), g_aFlagGroupInfo.GetSize());
	{
		const FLAGGROUPINFO *pM = g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1];
		WORD wNo = 0;
		printf("flaggroup multi bands=%d subcount=%d sub400=%d sub439=%d sub500=%d sub519=%d sub440=%d sub499=%d has500=%d has520=%d\n", FlagGroupBandCount(pM), FlagGroupSubCount(pM),
				FlagGroupSub(pM, 400), FlagGroupSub(pM, 439), FlagGroupSub(pM, 500), FlagGroupSub(pM, 519), FlagGroupSub(pM, 440), FlagGroupSub(pM, 499),
				(int)FlagGroupHas(pM, FLAGKIND_BIT, 500), (int)FlagGroupHas(pM, FLAGKIND_BIT, 520));
		printf("flaggroup multi nofsub1=%d/%u nofsub40=%d/%u nofsub41=%d/%u nofsub60=%d/%u nofsub61=%d nofsub0=%d\n",
				(int)FlagGroupNoOfSub(pM, 1, wNo), wNo, (int)FlagGroupNoOfSub(pM, 40, wNo), wNo, (int)FlagGroupNoOfSub(pM, 41, wNo), wNo, (int)FlagGroupNoOfSub(pM, 60, wNo), wNo,
				(int)FlagGroupNoOfSub(pM, 61, wNo), (int)FlagGroupNoOfSub(pM, 0, wNo));
	}
	printf("flaggroup multi get %s\n", SboWebDataGet("flagGroup", g_aFlagGroupInfo.GetSize() - 1));
	/* flag の記録の group・sub も新しい数え方で */
	printf("flaggroup multi flag439 %s\n", FlagGroupApi("flag", 0, "{\"byKind\":0,\"wNo\":439,\"szName\":\"帯1の最後\"}").c_str());
	printf("flaggroup multi flag500 %s\n", FlagGroupApi("flag", 1, "{\"byKind\":0,\"wNo\":500,\"szName\":\"帯2の先頭\"}").c_str());
	printf("flaggroup multi flag519 %s\n", FlagGroupApi("flag", 2, "{\"byKind\":0,\"wNo\":519,\"szName\":\"帯2の最後\"}").c_str());
	printf("flaggroup multi flag440 %s\n", FlagGroupApi("flag", 3, "{\"byKind\":0,\"wNo\":440,\"szName\":\"帯の間\"}").c_str());
	for (int i = 0; i < 4; i ++) {
		printf("flaggroup multi get flag %d %s\n", i, SboWebDataGet("flag", i));
	}
	/* 重なり：ほかのグループの帯 2 と自分の帯 1・自分の帯同士・同じ種類のほかの全帯。種類が違えば重ならない */
	{
		int nNew = g_aFlagGroupInfo.GetSize();
		int nMulti = nNew - 1;
		printf("flaggroup multi overlap-other-band2 %s count=%d\n", FlagGroupApi("flagGroup", nNew, "{\"szName\":\"重\",\"bands\":[{\"wFrom\":700,\"wTo\":710},{\"wFrom\":519,\"wTo\":530}]}").c_str(), g_aFlagGroupInfo.GetSize());
		printf("flaggroup multi overlap-own-bands %s count=%d\n", FlagGroupApi("flagGroup", nNew, "{\"szName\":\"重\",\"bands\":[{\"wFrom\":600,\"wTo\":610},{\"wFrom\":605,\"wTo\":620}]}").c_str(), g_aFlagGroupInfo.GetSize());
		printf("flaggroup multi overlap-band1-vs-band1 %s count=%d\n", FlagGroupApi("flagGroup", nNew, "{\"szName\":\"重\",\"bands\":[{\"wFrom\":600,\"wTo\":610},{\"wFrom\":139,\"wTo\":139}]}").c_str(), g_aFlagGroupInfo.GetSize());
		printf("flaggroup multi kind-differs %s count=%d\n", FlagGroupApi("flagGroup", nNew, "{\"byKind\":1,\"szName\":\"数多\",\"bands\":[{\"wFrom\":30,\"wTo\":35},{\"wFrom\":40,\"wTo\":45}]}").c_str(), g_aFlagGroupInfo.GetSize());
		nNew = g_aFlagGroupInfo.GetSize();		/* 種類違いのグループが増えたので数え直す */
		/* 今あるグループの続きの帯を足す（自分の帯は重なりに数えない） */
		printf("flaggroup multi grow-band %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":500,\"wTo\":519},{\"wFrom\":800,\"wTo\":809}]}").c_str());
		printf("flaggroup multi grown bands=%d subcount=%d sub800=%d\n", FlagGroupBandCount(g_aFlagGroupInfo[nMulti]), FlagGroupSubCount(g_aFlagGroupInfo[nMulti]), FlagGroupSub(g_aFlagGroupInfo[nMulti], 800));
		/* 帯の数・形・範囲・逆さ */
		/* 帯の本数：16 本までは通り（保存・読み直しも）、17 本は断る。元の 3 本へ戻す */
		{
			std::string strMax = "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":500,\"wTo\":519},{\"wFrom\":800,\"wTo\":809}", strOver;
			for (int k = 0; k < 13; k ++) strMax += ",{\"wFrom\":" + std::to_string(900 + k * 2) + ",\"wTo\":" + std::to_string(900 + k * 2) + "}";
			strOver = strMax + ",{\"wFrom\":990,\"wTo\":990}]}";
			printf("flaggroup multi bands17 %s bands=%d\n", FlagGroupApi("flagGroup", nMulti, strOver.c_str()).c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]));
			printf("flaggroup multi bands16 %s bands=%d subcount=%d sub924=%d\n", FlagGroupApi("flagGroup", nMulti, (strMax + "]}").c_str()).c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]),
					FlagGroupSubCount(g_aFlagGroupInfo[nMulti]), FlagGroupSub(g_aFlagGroupInfo[nMulti], 924));
			BOOL bSaved16 = SaveFlagGroupInfo();
			for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) delete g_aFlagGroupInfo[i];
			g_aFlagGroupInfo.RemoveAll();
			BOOL bLoaded16 = LoadFlagGroupInfo();
			printf("flaggroup multi bands16 reload save=%d load=%d bands=%d subcount=%d band16=%u-%u\n", (int)bSaved16, (int)bLoaded16, FlagGroupBandCount(g_aFlagGroupInfo[nMulti]),
					FlagGroupSubCount(g_aFlagGroupInfo[nMulti]), g_aFlagGroupInfo[nMulti]->awFrom[15], g_aFlagGroupInfo[nMulti]->awTo[15]);
			printf("flaggroup multi bands16 back %s bands=%d\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":500,\"wTo\":519},{\"wFrom\":800,\"wTo\":809}]}").c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]));
		}
		/* 管理者ウィンドウの帯の入力文字（カンマ区切り）の読み取り */
		{
			static const struct { const char *pszText; int nMax; } aParse[] = {
				{ "100-139, 300-319, 500-509", 1024 }, { "120", 1024 }, { "100〜139、300～319,500~509", 1024 }, { "　１００−１３９ 、 ３００〜３１９　", 1024 },
				{ "5-5,0-0", 128 }, { "0-1023", 1024 }, { "0-127", 128 }, { "0-128", 128 }, { "1024", 1024 }, { "999999", 1024 },
				{ "", 1024 }, { "   ", 1024 }, { "100-", 1024 }, { "-100", 1024 }, { "1-2-3", 1024 }, { "abc", 1024 }, { "100,", 1024 }, { ",100", 1024 }, { "1,,2", 1024 },
				{ "139-100", 1024 }, { "100-139, 139-150", 1024 }, { "100-139, 110", 1024 },
				{ "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31", 1024 }, { "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33", 1024 },
			};
			for (size_t t = 0; t < sizeof(aParse) / sizeof(aParse[0]); t ++) {
				WORD awF[MAX_FLAGGROUPBAND], awT[MAX_FLAGGROUPBAND];
				int nCnt = -1;
				char szErr[300];
				BOOL bOk = FlagGroupParseBands(aParse[t].pszText, aParse[t].nMax, awF, awT, nCnt, szErr, sizeof(szErr));
				printf("flaggroup parse %d [%s] max=%d ok=%d count=%d", (int)t, aParse[t].pszText, aParse[t].nMax, (int)bOk, nCnt);
				for (int k = 0; k < nCnt; k ++) printf(" %u-%u", awF[k], awT[k]);
				printf(" err=%s\n", bOk ? "" : szErr);
			}
		}
		printf("flaggroup multi bands0 %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[]}").c_str());
		printf("flaggroup multi bands-notarray %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":5}").c_str());
		printf("flaggroup multi bands-badelem %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":1}]}").c_str());
		printf("flaggroup multi bands-range %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":1000,\"wTo\":1024}]}").c_str());
		printf("flaggroup multi bands-reverse %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":520,\"wTo\":510}]}").c_str());
		/* 互換の wFrom・wTo：帯 1 だけが変わり、帯 2 以降は残る。bands と一緒なら帯 1 と同じ値だけ許す */
		printf("flaggroup multi legacy-wto %s bands=%d band1=%u-%u band2=%u-%u\n", FlagGroupApi("flagGroup", nMulti, "{\"wTo\":429}").c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]),
				g_aFlagGroupInfo[nMulti]->awFrom[0], g_aFlagGroupInfo[nMulti]->awTo[0], g_aFlagGroupInfo[nMulti]->awFrom[1], g_aFlagGroupInfo[nMulti]->awTo[1]);
		printf("flaggroup multi both-mismatch %s\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439}],\"wFrom\":401}").c_str());
		printf("flaggroup multi both-same %s bands=%d\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439},{\"wFrom\":500,\"wTo\":519}],\"wFrom\":400,\"wTo\":439}").c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]));
		/* 取得した記録をそのまま書き戻せる（履歴・元に戻す・しおり戻しはこの形で戻す） */
		{
			std::string strRec = SboWebDataGet("flagGroup", nMulti);
			size_t nB = strRec.find("\"bands\"");
			printf("flaggroup multi restore-ok %s\n", FlagGroupApi("flagGroup", nMulti, ("{\"szName\":\"多帯\",\"byState\":1," + strRec.substr(nB, strRec.size() - 1 - nB) + "}").c_str()).c_str());
			/* 1 本に戻して、また帯 2 つへ */
			printf("flaggroup multi one-band %s bands=%d\n", FlagGroupApi("flagGroup", nMulti, "{\"bands\":[{\"wFrom\":400,\"wTo\":439}]}").c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]));
			printf("flaggroup multi restore %s bands=%d subcount=%d\n", FlagGroupApi("flagGroup", nMulti, ("{" + strRec.substr(nB)).c_str()).c_str(), FlagGroupBandCount(g_aFlagGroupInfo[nMulti]), FlagGroupSubCount(g_aFlagGroupInfo[nMulti]));
		}
		/* 削除すると帯 2 本ぶんがほかのグループに使える。戻す時は重なれば断る */
		printf("flaggroup multi del %s\n", FlagGroupApi("flagGroup", nMulti, "{\"byState\":0}").c_str());
		printf("flaggroup multi reuse-band2 %s count=%d\n", FlagGroupApi("flagGroup", nNew, "{\"szName\":\"再利用\",\"wFrom\":510,\"wTo\":515}").c_str(), g_aFlagGroupInfo.GetSize());
		printf("flaggroup multi revive-conflict %s state=%d\n", FlagGroupApi("flagGroup", nMulti, "{\"byState\":1}").c_str(), g_aFlagGroupInfo[nMulti]->byState);
		printf("flaggroup multi get flag 2 after del %s\n", SboWebDataGet("flag", 2));
		/* 再利用を片付けて、多帯を戻す（保存・読み直しにも帯 2 本のグループを通す） */
		printf("flaggroup multi reuse-del %s\n", FlagGroupApi("flagGroup", nNew, "{\"byState\":0}").c_str());
		printf("flaggroup multi revive %s state=%d\n", FlagGroupApi("flagGroup", nMulti, "{\"byState\":1}").c_str(), g_aFlagGroupInfo[nMulti]->byState);
	}

	/* 保存と読み直し */
	{
		BOOL bSaved = SaveFlagGroupInfo();
		int nBefore = g_aFlagGroupInfo.GetSize();
		for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) delete g_aFlagGroupInfo[i];
		g_aFlagGroupInfo.RemoveAll();
		BOOL bLoaded = LoadFlagGroupInfo();
		printf("flaggroup save=%d load=%d before=%d after=%d\n", (int)bSaved, (int)bLoaded, nBefore, g_aFlagGroupInfo.GetSize());
		for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) {
			const FLAGGROUPINFO *p = g_aFlagGroupInfo[i];
			printf("flaggroup loaded %d id=%u state=%d kind=%d name=%s from=%u to=%u sort=%u\n", i, p->wID, p->byState, p->byKind, p->szName, p->awFrom[0], p->awTo[0], p->wSort);
			for (int b = 1; b < MAX_FLAGGROUPBAND; b ++) if (FlagGroupBandUsed(p, b)) printf("flaggroup loaded %d band%d=%u-%u\n", i, b + 1, p->awFrom[b], p->awTo[b]);
		}
		printf("flaggroup get flag 0 after load %s\n", SboWebDataGet("flag", 0));
	}

	/* 古い形のファイル（帯 1 本だけの列方式。wFrom・wTo はあるが、帯の本数の列は無い）が読める */
	{
		static const struct { WORD wID; BYTE byState; BYTE byKind; const char *pszName; WORD wFrom; WORD wTo; WORD wSort; } aOld[2] = {
			{ 1, 1, 0, "ルシー", 100, 139, 1 }, { 2, 1, 1, "数値グループ", 10, 19, 2 },
		};
		static const char *apszCol[7] = { "wID", "byState", "byKind", "szName", "wFrom", "wTo", "wSort" };
		std::vector<BYTE> aBody, aHead;
		std::vector<DWORD> aOff;
		DWORD dwTmp, dwHead = 8;
		for (int c = 0; c < 7; c ++) dwHead += (DWORD)strlen(apszCol[c]) + 1 + 4;
		for (int c = 0; c < 7; c ++) {
			aOff.push_back((DWORD)aBody.size());
			for (int r = 0; r < 2; r ++) {
				const BYTE *pB = NULL; size_t nB = 0;
				WORD w; BYTE by;
				switch (c) {
				case 0: w = aOld[r].wID; pB = (const BYTE *)&w; nB = 2; break;
				case 1: by = aOld[r].byState; pB = &by; nB = 1; break;
				case 2: by = aOld[r].byKind; pB = &by; nB = 1; break;
				case 3: pB = (const BYTE *)aOld[r].pszName; nB = strlen(aOld[r].pszName) + 1; break;
				case 4: w = aOld[r].wFrom; pB = (const BYTE *)&w; nB = 2; break;
				case 5: w = aOld[r].wTo; pB = (const BYTE *)&w; nB = 2; break;
				default: w = aOld[r].wSort; pB = (const BYTE *)&w; nB = 2; break;
				}
				aBody.insert(aBody.end(), pB, pB + nB);
			}
		}
		dwTmp = 7; aHead.insert(aHead.end(), (BYTE *)&dwTmp, (BYTE *)&dwTmp + 4);
		dwTmp = 2; aHead.insert(aHead.end(), (BYTE *)&dwTmp, (BYTE *)&dwTmp + 4);
		for (int c = 0; c < 7; c ++) {
			aHead.insert(aHead.end(), (const BYTE *)apszCol[c], (const BYTE *)apszCol[c] + strlen(apszCol[c]) + 1);
			dwTmp = dwHead + aOff[c]; aHead.insert(aHead.end(), (BYTE *)&dwTmp, (BYTE *)&dwTmp + 4);
		}
		FILE *fp = fopen("SBODATA/SboDataFlagGroupInfo.dat", "wb");
		if (fp) {
			fwrite(&aHead[0], 1, aHead.size(), fp);
			fwrite(&aBody[0], 1, aBody.size(), fp);
			fclose(fp);
		}
		for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) delete g_aFlagGroupInfo[i];
		g_aFlagGroupInfo.RemoveAll();
		BOOL bLoaded = LoadFlagGroupInfo();
		printf("flaggroup legacy-file wrote=%d load=%d count=%d\n", (int)(fp != NULL), (int)bLoaded, g_aFlagGroupInfo.GetSize());
		for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) {
			const FLAGGROUPINFO *p = g_aFlagGroupInfo[i];
			printf("flaggroup legacy-file %d id=%u kind=%d name=%s bands=%d band1=%u-%u subcount=%d sort=%u\n", i, p->wID, p->byKind, p->szName, FlagGroupBandCount(p), p->awFrom[0], p->awTo[0], FlagGroupSubCount(p), p->wSort);
		}
		/* 古いファイルから読んだグループに続きの帯を足して保存し、読み直しても残る */
		printf("flaggroup legacy-file add-band %s\n", FlagGroupApi("flagGroup", 0, "{\"bands\":[{\"wFrom\":100,\"wTo\":139},{\"wFrom\":300,\"wTo\":319}]}").c_str());
		for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) delete g_aFlagGroupInfo[i];
		g_aFlagGroupInfo.RemoveAll();
		bLoaded = LoadFlagGroupInfo();
		printf("flaggroup legacy-file reload load=%d count=%d bands=%d sub300=%d sub319=%d\n", (int)bLoaded, g_aFlagGroupInfo.GetSize(), FlagGroupBandCount(g_aFlagGroupInfo[0]),
				FlagGroupSub(g_aFlagGroupInfo[0], 300), FlagGroupSub(g_aFlagGroupInfo[0], 319));
	}

	/* 管理者ウィンドウ：一覧・足す・直す・重なりを断る・管理者でない人は弾く */
	{
		CMDPARA_ADMINFLAGGROUP Res;
		FLAGGROUPINFO Info;
		ZeroMemory(&Info, sizeof(Info));
		FlagGroupClearBands(&Info);
		pAdmin->byAdminLV = ADMINLV_ADMINISTRATOR;
		BOOL bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_LISTREQ, 0, NULL, Res);
		printf("adminflaggroup list got=%d sub=%d count=%d id=%u name=%s\n", bGot, Res.byCmdSub, Res.wCount, Res.Info.wID, Res.Info.szName);
		Info.byState = FLAGGROUPSTATE_ON; Info.byKind = FLAGKIND_BIT; strcpy(Info.szName, "管理者"); Info.awFrom[0] = 200; Info.awTo[0] = 209;
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, -1, &Info, Res);
		printf("adminflaggroup add got=%d result=%d count=%d id=%u\n", bGot, Res.byResult, g_aFlagGroupInfo.GetSize(), g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1]->wID);
		Info.awFrom[0] = 205; Info.awTo[0] = 215; strcpy(Info.szName, "重なる");
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, -1, &Info, Res);
		printf("adminflaggroup overlap got=%d result=%d count=%d msg=%s\n", bGot, Res.byResult, g_aFlagGroupInfo.GetSize(), Res.szMsg);
		/* 帯 2 本（クライアントの構造体の帯 2 を使う）で足す。帯 2 が重なれば断る */
		FlagGroupClearBands(&Info);
		Info.byState = FLAGGROUPSTATE_ON; Info.byKind = FLAGKIND_BIT; strcpy(Info.szName, "二帯"); Info.awFrom[0] = 250; Info.awTo[0] = 259; Info.awFrom[1] = 350; Info.awTo[1] = 359;
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, -1, &Info, Res);
		printf("adminflaggroup multi got=%d result=%d count=%d bands=%d sub350=%d\n", bGot, Res.byResult, g_aFlagGroupInfo.GetSize(), FlagGroupBandCount(g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1]), FlagGroupSub(g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1], 350));
		Info.awFrom[0] = 600; Info.awTo[0] = 609; Info.awFrom[1] = 205; Info.awTo[1] = 206; strcpy(Info.szName, "二帯重");
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, -1, &Info, Res);
		printf("adminflaggroup multi-overlap got=%d result=%d count=%d msg=%s\n", bGot, Res.byResult, g_aFlagGroupInfo.GetSize(), Res.szMsg);
		Info = *g_aFlagGroupInfo[g_aFlagGroupInfo.GetSize() - 1];
		Info.wID = 99; strcpy(Info.szName, "管理者改");
		int nLast = g_aFlagGroupInfo.GetSize() - 1;
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, nLast, &Info, Res);
		printf("adminflaggroup edit got=%d result=%d name=%s id=%u\n", bGot, Res.byResult, g_aFlagGroupInfo[nLast]->szName, g_aFlagGroupInfo[nLast]->wID);
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, 99, &Info, Res);
		printf("adminflaggroup range got=%d result=%d\n", bGot, Res.byResult);
		pAdmin->byAdminLV = ADMINLV_USER;
		int nCount = g_aFlagGroupInfo.GetSize();
		bGot = AdminFlagGroupSend(pAdmin, ADMINFLAGGROUP_SET, -1, &Info, Res);
		printf("adminflaggroup user got=%d count=%d same=%d\n", bGot, g_aFlagGroupInfo.GetSize(), (int)(nCount == g_aFlagGroupInfo.GetSize()));
	}

	/* 片付け */
	for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) delete g_aFlagGroupInfo[i];
	g_aFlagGroupInfo.RemoveAll();
	for (size_t i = 0; i < aGroupBack.size(); i ++) g_aFlagGroupInfo.Add(aGroupBack[i]);
	for (int i = 0; i < g_aFlagNameInfo.GetSize(); i ++) delete g_aFlagNameInfo[i];
	g_aFlagNameInfo.RemoveAll();
	for (size_t i = 0; i < aNameBack.size(); i ++) g_aFlagNameInfo.Add(aNameBack[i]);
	SaveFlagGroupInfo();
	SaveFlagNameInfo();
	pAdmin->byAdminLV = byAdminBack;
}

static void TestQuest(const std::vector<int> &aLogin)
{
	printf("== M. Quest ==\n");
	if (aLogin.size() < 4) {
		return;
	}
	PCHARINFO pOwner = g_aCharInfo[aLogin[0]], pMember = g_aCharInfo[aLogin[1]];
	PCHARINFO pOther = g_aCharInfo[aLogin[2]], pP4 = g_aCharInfo[aLogin[3]];
	PCHARINFO apPC[4] = { pOwner, pMember, pOther, pP4 };
	int nMap = (int)pOwner->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	DWORD dwMapBack[4], dwXBack[4], dwYBack[4];
	WORD awItemBack[4][MAX_MYITEM];
	DWORD dwGoldBack[4];
	for (int i = 0; i < 4; i ++) {
		dwMapBack[i] = apPC[i]->dwMapID; dwXBack[i] = apPC[i]->dwX; dwYBack[i] = apPC[i]->dwY;
		memcpy(awItemBack[i], apPC[i]->waItem, sizeof(awItemBack[i]));
		dwGoldBack[i] = apPC[i]->dwGold;
		ZeroMemory(apPC[i]->waItem, sizeof(apPC[i]->waItem));
		apPC[i]->byAdminLV = ADMINLV_USER;
		apPC[i]->dwPartyID = 0;
		apPC[i]->wQuestID = 0;
		apPC[i]->byQuestStep = 0;
		apPC[i]->wQuestFlg = 0;
	}

	/* 空いているマスを 8 つ（持ち主の近く） */
	std::vector<std::pair<int, int> > aCell;
	for (int r = 1; r < 12 && aCell.size() < 8; r ++) {
		for (int dy = -r; dy <= r && aCell.size() < 8; dy ++) {
			for (int dx = -r; dx <= r && aCell.size() < 8; dx ++) {
				if (abs(dx) != r && abs(dy) != r) continue;
				int x = (int)pOwner->dwX + dx, y = (int)pOwner->dwY + dy;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				if (dx == 0 && dy == 0) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
	}
	printf("quest map=%d cells=%d\n", nMap, (int)aCell.size());
	if (aCell.size() < 8) {
		return;
	}

	/* 使うアイテム（0 以外で名前があるもの 2 つ） */
	WORD wItemQ = 0, wItemR = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		if (g_aItemInfo[i]->szName[0] == 0) continue;
		if (!wItemQ) wItemQ = (WORD)i; else if (!wItemR) { wItemR = (WORD)i; break; }
	}

	/* 依頼 1（一度きり・2 段・見えるNPC 3 つ）・依頼 2・3（見えるNPC なし） */
	g_aQuestInfo.RemoveAll();
	{
		PQUESTINFO p = new QUESTINFO;
		ZeroMemory(p, sizeof(*p));
		p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
		p->wID = 1; p->byState = QUESTSTATE_ON; p->byGuild = 0; p->wSort = 1; p->byOnce = 1;
		strcpy(p->szName, "テスト依頼1");
		strcpy(p->szExplain, "宝箱の品を持ち帰る");
		strcpy(p->szMsgAccept, "お願いします");
		strcpy(p->szMsgDone, "ありがとう");
		p->dwGold = 100; p->dwExp = 10; p->wRewardItem = wItemR; p->wQuestItem = wItemQ; p->byStepCount = 2;
		strcpy(p->aszGoal[0], "宝箱を探す");
		strcpy(p->aszGoal[1], "ギルドへ持ち帰る");
		for (int k = 0; k < MAX_QUESTNPC; k ++) { p->aNpc[k].byStepMax = QUESTNPC_NOSTEP; p->aNpc[k].byNextStep = QUESTNPC_NOSTEP; }
		/* 0: ポータル（どの段でも） grp=1 */
		p->aNpc[0].dwMapID = pMap->dwID; p->aNpc[0].byX = (BYTE)aCell[0].first; p->aNpc[0].byY = (BYTE)aCell[0].second;
		p->aNpc[0].byKind = QUESTNPCKIND_PORTAL; p->aNpc[0].wGrpID = 1; strcpy(p->aNpc[0].szName, "ポータル");
		p->aNpc[0].dwDestMapID = pMap->dwID; p->aNpc[0].byDestX = (BYTE)aCell[3].first; p->aNpc[0].byDestY = (BYTE)aCell[3].second;
		/* 1: 宝箱（0 段目だけ。話すと旗1・次の段へ） grp=2 */
		p->aNpc[1].dwMapID = pMap->dwID; p->aNpc[1].byX = (BYTE)aCell[1].first; p->aNpc[1].byY = (BYTE)aCell[1].second;
		p->aNpc[1].byKind = QUESTNPCKIND_CHEST; p->aNpc[1].wGrpID = 2; strcpy(p->aNpc[1].szName, "宝箱");
		strcpy(p->aNpc[1].szSpeech, "箱をあけた");
		p->aNpc[1].byStepMin = 0; p->aNpc[1].byStepMax = 0; p->aNpc[1].wItem = wItemQ; p->aNpc[1].bySetFlg = 1; p->aNpc[1].byNextStep = 1;
		/* 2: 人（旗1が立ってから） grp=3 */
		p->aNpc[2].dwMapID = pMap->dwID; p->aNpc[2].byX = (BYTE)aCell[2].first; p->aNpc[2].byY = (BYTE)aCell[2].second;
		p->aNpc[2].byKind = QUESTNPCKIND_PERSON; p->aNpc[2].wGrpID = 3; strcpy(p->aNpc[2].szName, "旅人");
		strcpy(p->aNpc[2].szSpeech, "よくやった"); p->aNpc[2].byStepMin = 1; p->aNpc[2].byFlgNeed = 1;
		g_aQuestInfo.Add(p);
		for (int q = 2; q <= 3; q ++) {
			p = new QUESTINFO;
			ZeroMemory(p, sizeof(*p));
			p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
			p->wID = (WORD)q; p->byState = QUESTSTATE_ON; p->byGuild = 0; p->wSort = (WORD)q; p->byStepCount = 1;
			sprintf(p->szName, "テスト依頼%d", q);
			g_aQuestInfo.Add(p);
		}
	}

	/* ギルドの人を持ち主の近くに置く */
	PCHARINFO pGuild = new CHARINFO;
	ZeroMemory((PBYTE)pGuild, sizeof(CHARINFO));
	pGuild->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pGuild);
	pGuild->byGameState = GAMESTATE_NPC;
	pGuild->byCharState = CHARSTATE_MOVE;
	pGuild->dwMapID = nMap;
	pGuild->dwX = pGuild->dwXBack = pGuild->dwStartX = (DWORD)aCell[4].first;
	pGuild->dwY = pGuild->dwYBack = pGuild->dwStartY = (DWORD)aCell[4].second;
	strcpy(pGuild->szName, "ギルド受付");
	pGuild->pszNpcSpeech = new char[8];
	strcpy(pGuild->pszNpcSpeech, "@g0");
	pGuild = AddChar(pGuild);
	pGuild->pID = pGuild->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pGuild);
	}
	DWORD dwGuild = pGuild->dwTblID;

	/* みんなを画面内に集める */
	QuestMovePC(pOwner, nMap, aCell[5].first, aCell[5].second);
	QuestMovePC(pMember, nMap, aCell[6].first, aCell[6].second);
	QuestMovePC(pOther, nMap, aCell[7].first, aCell[7].second);
	QuestMovePC(pP4, nMap, aCell[7].first, aCell[7].second);
	int anConn[4];
	for (int i = 0; i < 4; i ++) anConn[i] = 1000 + i;		/* ログインさせた順に 1000 番から（TestWorld） */

	/* 持ち主と仲間がパーティを組む */
	QuestSendParty(pOwner, pMember, CMDPARTY_JOIN);
	QuestSendParty(pMember, pOwner, CMDPARTY_JOINOK);
	printf("quest party same=%d nonzero=%d\n", (int)(pOwner->dwPartyID == pMember->dwPartyID), (int)(pOwner->dwPartyID != 0));

	/* ギルドの人に話すと一覧が届く（依頼 1・2・3） */
	QuestMovePC(pOwner, nMap, aCell[4].first, aCell[4].second + 1);
	int nList0 = MockCmdCount(anConn[0], CMD_QUEST);
	QuestSendTalk(pOwner, dwGuild);
	printf("quest list packets=%d\n", MockCmdCount(anConn[0], CMD_QUEST) - nList0);

	/* 受ける：見えるNPCは持ち主と仲間にだけ届く（関係ない人には 1 つも届かない） */
	int nInfo0[4], nCalls0[4];
	for (int i = 0; i < 4; i ++) { nInfo0[i] = MockCmdCount(anConn[i], CMD_CHARINFO); nCalls0[i] = MockCalls(anConn[i]); }
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 1, dwGuild);
	printf("quest accept id=%u step=%u copies=%d\n", pOwner->wQuestID, pOwner->byQuestStep, QuestCountCopies());
	printf("quest sent charinfo owner=%d member=%d other=%d p4=%d other_calls=%d queststate owner>0=%d member>0=%d\n",
		MockCmdCount(anConn[0], CMD_CHARINFO) - nInfo0[0], MockCmdCount(anConn[1], CMD_CHARINFO) - nInfo0[1],
		MockCmdCount(anConn[2], CMD_CHARINFO) - nInfo0[2], MockCmdCount(anConn[3], CMD_CHARINFO) - nInfo0[3],
		MockCalls(anConn[2]) - nCalls0[2],
		(int)(MockCmdCount(anConn[0], CMD_QUESTSTATE) > 0), (int)(MockCmdCount(anConn[1], CMD_QUESTSTATE) > 0));

	/* 仲間も同じ依頼を受ける：自分の分だけ見える。管理者には全部見える */
	QuestMovePC(pMember, nMap, aCell[4].first + 1, aCell[4].second);
	QuestSendCmd(pMember, QUESTCMD_ACCEPT, 1, dwGuild);
	QuestMovePC(pMember, nMap, aCell[6].first, aCell[6].second);
	printf("quest member accept id=%u copies=%d\n", pMember->wQuestID, QuestCountCopies());
	{
		CHARINFO Admin = *pOther;
		Admin.byAdminLV = ADMINLV_ADMINISTRATOR;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p->byNpcType != NPCTYPE_QUESTNPC) continue;
			printf("quest copy grp=%u vis owner=%d member=%d other=%d admin=%d block=%d\n", p->wGrpID,
				IsCharVisibleTo(p, pOwner), IsCharVisibleTo(p, pMember), IsCharVisibleTo(p, pOther), IsCharVisibleTo(p, &Admin),
				(int)CheckCharBlockNPC((DWORD)nMap, p->dwX, p->dwY));
		}
	}

	/* 管理者は見えるだけ：光に話しても移らない */
	{
		PCHARINFO pPortalA = QuestFindCopy(pOwner, 1);
		if (pPortalA) {
			pOther->byAdminLV = ADMINLV_ADMINISTRATOR;
			QuestMovePC(pOther, nMap, (int)pPortalA->dwX, (int)pPortalA->dwY + 1);
			DWORD dwX0 = pOther->dwX, dwY0 = pOther->dwY;
			QuestSendTalk(pOther, pPortalA->dwTblID);
			printf("quest admin portal moved=%d\n", (int)(pOther->dwX != dwX0 || pOther->dwY != dwY0));
			pOther->byAdminLV = ADMINLV_USER;
			QuestMovePC(pOther, nMap, aCell[7].first, aCell[7].second);

			/* 依頼を受けた管理者：同じマスに重なったほかの人の光に話しても、自分の分の光で移る */
			pMember->byAdminLV = ADMINLV_ADMINISTRATOR;
			QuestMovePC(pMember, nMap, (int)pPortalA->dwX, (int)pPortalA->dwY + 1);
			QuestSendTalk(pMember, pPortalA->dwTblID);
			printf("quest admin own portal moved=%d\n", (int)(pMember->dwX == (DWORD)aCell[3].first && pMember->dwY == (DWORD)aCell[3].second));
			pMember->byAdminLV = ADMINLV_USER;
			QuestMovePC(pMember, nMap, aCell[6].first, aCell[6].second);
		}
	}

	/* 宝箱：持ち物がいっぱいなら受け取れない（預かり所にも入らず、旗も立たない） */
	PCHARINFO pChest = QuestFindCopy(pOwner, 2);
	if (pChest) {
		QuestMovePC(pOwner, nMap, (int)pChest->dwX, (int)pChest->dwY + 1);
		for (int i = 0; i < MAX_MYITEM; i ++) pOwner->waItem[i] = wItemR;
		QuestSendTalk(pOwner, pChest->dwTblID);
		int nLeaveQ = 0;
		if (pOwner->dwLeaveID && pOwner->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			PLEAVEINFO pL = g_aLeaveInfo[pOwner->dwLeaveID];
			for (int i = 0; i < pL->byCount; i ++) if (pL->waItem[i] == wItemQ) nLeaveQ += pL->byItemCount[i];
		}
		printf("quest chest full item=%d leave=%d flag1=%d step=%u\n", QuestCountItem(pOwner, wItemQ), nLeaveQ, QuestFlagCheck(pOwner, 1), pOwner->byQuestStep);
		ZeroMemory(pOwner->waItem, sizeof(pOwner->waItem));
	}

	/* 宝箱：持ち主が話す → 品物・旗1・次の段。仲間（別の人の宝箱）は見えないので何も起きない */
	if (pChest) {
		QuestSendTalk(pOwner, pChest->dwTblID);
		printf("quest chest item=%d flag1=%d step=%u person=%d chest=%d\n", QuestCountItem(pOwner, wItemQ), QuestFlagCheck(pOwner, 1),
			pOwner->byQuestStep, (QuestFindCopy(pOwner, 3) != NULL), (QuestFindCopy(pOwner, 2) != NULL));
	} else {
		printf("quest chest not found\n");
	}
	printf("quest member item=%d step=%u\n", QuestCountItem(pMember, wItemQ), pMember->byQuestStep);

	/* ポータル：話すと行き先へ移る */
	PCHARINFO pPortal = QuestFindCopy(pOwner, 1);
	if (pPortal) {
		QuestMovePC(pOwner, nMap, (int)pPortal->dwX, (int)pPortal->dwY + 1);
		QuestSendTalk(pOwner, pPortal->dwTblID);
		printf("quest portal to dest=%d\n", (int)(pOwner->dwX == (DWORD)aCell[3].first && pOwner->dwY == (DWORD)aCell[3].second));

		/* 歩いてポータルに乗っても移る（話さなくてよい） */
		pPortal = QuestFindCopy(pOwner, 1);
		if (pPortal) {
			QuestMovePC(pOwner, nMap, (int)pPortal->dwX, (int)pPortal->dwY + 1);
			CMDPARA_CHARPOS PosPara;
			ZeroMemory(&PosPara, sizeof(PosPara));
			PosPara.byCmd = CMD_CHARPOS;
			PosPara.dwTblID = pOwner->dwTblID;
			PosPara.dwMapID = pOwner->dwMapID;
			PosPara.byDirection = 1;
			PosPara.byX = (BYTE)pPortal->dwX;
			PosPara.byY = (BYTE)pPortal->dwY;
			TestRecv(pOwner, &PosPara, sizeof(PosPara));
			/* 乗った瞬間には移らない（歩き終わるまで少し待つ） */
			printf("quest portal step wait moved=%d\n", (int)(pOwner->dwX == (DWORD)aCell[3].first && pOwner->dwY == (DWORD)aCell[3].second));
			s_dwFakeNow += 1000;
			QuestSyncProc();
			printf("quest portal step to dest=%d\n", (int)(pOwner->dwX == (DWORD)aCell[3].first && pOwner->dwY == (DWORD)aCell[3].second));
		}
	}

	/* ギルドで渡す：品物を回収して報酬（お金・経験値・アイテム）。一度きりなので済みの印が付く */
	QuestMovePC(pOwner, nMap, aCell[4].first, aCell[4].second + 1);
	DWORD dwGold0 = pOwner->dwGold, dwExp0 = pOwner->dwExp;
	QuestSendCmd(pOwner, QUESTCMD_HANDIN, 0, dwGuild);
	printf("quest handin quest=%u gold+=%d exp>0=%d item_q=%d item_r=%d done=%d copies=%d\n", pOwner->wQuestID, (int)(pOwner->dwGold - dwGold0),
		(int)(pOwner->dwExp != dwExp0), QuestCountItem(pOwner, wItemQ), QuestCountItem(pOwner, wItemR), (int)QuestIsDone(pOwner, 1), QuestCountCopies());
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 1, dwGuild);
	printf("quest reaccept id=%u\n", pOwner->wQuestID);

	/* 仲間が別の依頼を受けているなら、同じ依頼しか受けられない */
	QuestMovePC(pMember, nMap, aCell[4].first + 1, aCell[4].second);
	QuestSendCmd(pMember, QUESTCMD_CANCEL, 0, dwGuild);
	printf("quest cancel member q=%u item=%d flag1=%d copies=%d\n", pMember->wQuestID, QuestCountItem(pMember, wItemQ), QuestFlagCheck(pMember, 1), QuestCountCopies());
	QuestSendCmd(pMember, QUESTCMD_ACCEPT, 2, dwGuild);
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
	printf("quest party-rule member=%u owner_q3=%u", pMember->wQuestID, pOwner->wQuestID);
	QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 2, dwGuild);
	printf(" owner_q2=%u\n", pOwner->wQuestID);
	QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
	QuestSendCmd(pMember, QUESTCMD_CANCEL, 0, dwGuild);

	/* 別の依頼を受けている人同士は仲間になれない */
	QuestSendParty(pOwner, pOwner, CMDPARTY_BREAKUP);
	printf("quest break owner_party=%u member_party=%u\n", pOwner->dwPartyID, pMember->dwPartyID);
	QuestMovePC(pOther, nMap, aCell[4].first + 1, aCell[4].second);
	QuestMovePC(pP4, nMap, aCell[4].first - 1, aCell[4].second);
	QuestSendCmd(pOther, QUESTCMD_ACCEPT, 2, dwGuild);
	QuestSendCmd(pP4, QUESTCMD_ACCEPT, 3, dwGuild);
	printf("quest other=%u p4=%u conflict=%d\n", pOther->wQuestID, pP4->wQuestID, (int)QuestPartyConflict(pOther, pP4));
	QuestSendParty(pOther, pP4, CMDPARTY_JOIN);
	QuestSendParty(pP4, pOther, CMDPARTY_JOINOK);
	printf("quest join refused other_party=%u p4_party=%u\n", pOther->dwPartyID, pP4->dwPartyID);

	/* 取り消し：依頼の品・旗・数が消え、罰は無い（依頼 1 は依頼の品がある） */
	QuestSendCmd(pOther, QUESTCMD_CANCEL, 0, dwGuild);
	QuestSendCmd(pP4, QUESTCMD_CANCEL, 0, dwGuild);
	QuestSendCmd(pOther, QUESTCMD_ACCEPT, 1, dwGuild);
	AddCharItem(pOther->dwTblID, wItemQ);
	AddLeaveItem(pOther, wItemQ);
	pOther->wQuestFlg = 5;
	pOther->awQuestCount[1] = 3;
	QuestSendCmd(pOther, QUESTCMD_CANCEL, 0, dwGuild);
	{
		int nLeave = 0;
		if (pOther->dwLeaveID && pOther->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			PLEAVEINFO pL = g_aLeaveInfo[pOther->dwLeaveID];
			for (int i = 0; i < pL->byCount; i ++) if (pL->waItem[i] == wItemQ) nLeave += pL->byItemCount[i];
		}
		printf("quest cancel other q=%u flg=%u count1=%u item=%d leave=%d done=%d\n", pOther->wQuestID, pOther->wQuestFlg, pOther->awQuestCount[1],
			QuestCountItem(pOther, wItemQ), nLeave, (int)QuestIsDone(pOther, 1));
	}
	/* ギルドから離れていたら取り消せない */
	QuestSendCmd(pOther, QUESTCMD_ACCEPT, 2, dwGuild);
	QuestMovePC(pOther, nMap, aCell[7].first, aCell[7].second);
	QuestSendCmd(pOther, QUESTCMD_CANCEL, 0, dwGuild);
	printf("quest far cancel q=%u\n", pOther->wQuestID);

	/* 受ける条件の印：合わない依頼は一覧に出ず、受けられない（依頼 3 に条件を付けて確かめる） */
	{
		PQUESTINFO pQ3 = NULL;
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) pQ3 = g_aQuestInfo[i];
		QuestMovePC(pOwner, nMap, aCell[4].first, aCell[4].second + 1);
		pOwner->wQuestID = 0;
		FlagBitSet(pOwner->abyFlag, 500, FALSE);
		FlagBitSet(pOwner->abyFlag, 501, FALSE);
		int nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
		QuestSendTalk(pOwner, dwGuild);
		int nBase = MockCmdCount(anConn[0], CMD_QUEST) - nBase0;
		if (pQ3) {
			/* 立っている人だけ（印が無いと一覧に出ず、受けられない。立てると出て、受けられる） */
			pQ3->wNeedFlag = 500;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf("quest cond need off list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u", pOwner->wQuestID);
			FlagBitSet(pOwner->abyFlag, 500, TRUE);
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf(" | on list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u\n", pOwner->wQuestID);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			/* 立っている人は受けられない（立てると出なくなり、消すと戻る） */
			pQ3->wNeedFlag = FLAG_NOBIT;
			pQ3->wNotFlag = 501;
			FlagBitSet(pOwner->abyFlag, 501, TRUE);
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf("quest cond not on list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u", pOwner->wQuestID);
			FlagBitSet(pOwner->abyFlag, 501, FALSE);
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf(" | off list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u\n", pOwner->wQuestID);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			/* 保存して読み直しても同じ（古いファイルで新しい項目が無い時は「無し」になる） */
			pQ3->wNeedFlag = 0;
			pQ3->wNotFlag = 1023;
			SaveQuestInfo();
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
			g_aQuestInfo.RemoveAll();
			LoadQuestInfo();
			pQ3 = NULL;
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) pQ3 = g_aQuestInfo[i];
			printf("quest cond reload need=%u not=%u", pQ3 ? pQ3->wNeedFlag : 9999, pQ3 ? pQ3->wNotFlag : 9999);
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 2) printf(" q2=%u,%u", g_aQuestInfo[i]->wNeedFlag, g_aQuestInfo[i]->wNotFlag);
			printf("\n");
			if (pQ3) {
				pQ3->wNeedFlag = FLAG_NOBIT;
				pQ3->wNotFlag = FLAG_NOBIT;
			}
		}
		FlagBitSet(pOwner->abyFlag, 500, FALSE);
		FlagBitSet(pOwner->abyFlag, 501, FALSE);
	}

	/* 導入のお話：一覧には「ある」目印だけ。決定を押した時の要求（INTROREQ）で本文が 1 通だけ返る。
	   読まずに受けてもよい。保存して読み直しても残る（古いファイルは空） */
	{
		PQUESTINFO pQ3 = NULL;
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) pQ3 = g_aQuestInfo[i];
		if (pQ3) {
			QuestMovePC(pOwner, nMap, aCell[4].first, aCell[4].second + 1);
			pOwner->wQuestID = 0;
			int n0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendCmd(pOwner, QUESTCMD_INTROREQ, 3, dwGuild);
			printf("quest intro none packets=%d", MockCmdCount(anConn[0], CMD_QUEST) - n0);
			strcpy(pQ3->szIntro, "むかしむかし。@0つづきです。@1おわり。@2");
			SaveQuestInfo();
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
			g_aQuestInfo.RemoveAll();
			LoadQuestInfo();
			pQ3 = NULL;
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) pQ3 = g_aQuestInfo[i];
			printf(" reload=%d", (pQ3 && strcmp(pQ3->szIntro, "むかしむかし。@0つづきです。@1おわり。@2") == 0) ? 1 : 0);
			if (pQ3) {
				n0 = MockCmdCount(anConn[0], CMD_QUEST);
				QuestSendCmd(pOwner, QUESTCMD_INTROREQ, 3, dwGuild);
				printf(" req packets=%d", MockCmdCount(anConn[0], CMD_QUEST) - n0);
				n0 = MockCmdCount(anConn[0], CMD_QUEST);
				QuestSendCmd(pOwner, QUESTCMD_INTROREQ, 3, pOwner->dwTblID);		/* ギルドの人ではない */
				QuestSendCmd(pOwner, QUESTCMD_INTROREQ, 9999, dwGuild);	/* 無い依頼 */
				printf(" bad packets=%d", MockCmdCount(anConn[0], CMD_QUEST) - n0);
				QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);			/* 読まずに受けてもよい */
				printf(" accept=%u\n", pOwner->wQuestID);
				QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
				memset(pQ3->szIntro, 0, sizeof(pQ3->szIntro));
			} else {
				printf("\n");
			}
		}
	}

	/* 最低レベル（byLevelMin）：足りない人には一覧に出ず、受けられない。0 は制限なし・管理者は常に見える・受けている人は続けられる
	   窓口の一言：レベルで隠した依頼がある人にだけ、NPC に自前の台詞（@g の後ろの文）が無い時に出る */
	{
		PQUESTINFO pQ3 = NULL;
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) pQ3 = g_aQuestInfo[i];
		if (pQ3) {
			BYTE byLvBack = pOwner->byLevel, byAdBack = pOwner->byAdminLV;
			char *pszSpeechBack = pGuild->pszNpcSpeech;
			char szOwnSpeech[64];
			std::vector<BYTE> aCap;
			int nList;
			QuestMovePC(pOwner, nMap, aCell[4].first, aCell[4].second + 1);
			pOwner->wQuestID = 0;
			pOwner->byAdminLV = ADMINLV_USER;
			pOwner->byLevel = 1;
			int nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			int nBase = MockCmdCount(anConn[0], CMD_QUEST) - nBase0;
			/* 足りない人：出ない・受けられない（理由にレベルが出る）。一言が最初の 1 件に添えられる */
			pQ3->byLevelMin = 30;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			nList = MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase;
			SvrSendCaptureBegin();
			QuestSendTalk(pOwner, dwGuild);
			SvrSendCaptureEnd(aCap);
			std::string strCap((const char *)(aCap.empty() ? (const BYTE *)"" : &aCap[0]), aCap.size());
			printf("quest lvmin low list=%d note=%d", nList,
				(int)(strCap.find("今のレベルに合った依頼は、こんな感じです。") != std::string::npos));
			SvrSendCaptureBegin();
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			SvrSendCaptureEnd(aCap);
			strCap.assign((const char *)(aCap.empty() ? (const BYTE *)"" : &aCap[0]), aCap.size());
			printf(" accept=%u msg=%d", pOwner->wQuestID, (int)(strCap.find("この依頼を受けるには、レベル30が必要です。") != std::string::npos));
			/* NPC に自前の台詞（@g の後ろの文）があれば、そちらを優先して一言は添えない */
			strcpy(szOwnSpeech, "@g0【受付】@1いらっしゃい@0");
			pGuild->pszNpcSpeech = szOwnSpeech;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			nList = MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase;
			SvrSendCaptureBegin();
			QuestSendTalk(pOwner, dwGuild);
			SvrSendCaptureEnd(aCap);
			strCap.assign((const char *)(aCap.empty() ? (const BYTE *)"" : &aCap[0]), aCap.size());
			printf(" own_speech list=%d note=%d", nList,
				(int)(strCap.find("今のレベルに合った依頼は、こんな感じです。") != std::string::npos));
			pGuild->pszNpcSpeech = pszSpeechBack;
			/* 1 件も出なくなった時は「合う依頼はありません」 */
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 2) g_aQuestInfo[i]->byLevelMin = 30;
			SvrSendCaptureBegin();
			QuestSendTalk(pOwner, dwGuild);
			SvrSendCaptureEnd(aCap);
			strCap.assign((const char *)(aCap.empty() ? (const BYTE *)"" : &aCap[0]), aCap.size());
			printf(" empty=%d", (int)(strCap.find("いまのレベルに合う依頼はありません。") != std::string::npos));
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 2) g_aQuestInfo[i]->byLevelMin = 0;
			/* 足りると出る・受けられる */
			pOwner->byLevel = 30;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			nList = MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase;
			SvrSendCaptureBegin();
			QuestSendTalk(pOwner, dwGuild);
			SvrSendCaptureEnd(aCap);
			strCap.assign((const char *)(aCap.empty() ? (const BYTE *)"" : &aCap[0]), aCap.size());
			printf(" | enough list=%d note=%d", nList,
				(int)(strCap.find("今のレベルに合った依頼は、こんな感じです。") != std::string::npos));
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u", pOwner->wQuestID);
			/* 受けている人は、レベルが足りなくなっても続けられる・一覧に出る */
			pOwner->byLevel = 1;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf(" mine list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			/* 0 は制限なし */
			pQ3->byLevelMin = 0;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf(" | zero list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u", pOwner->wQuestID);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			/* 管理者は常に見える・受けられる（レベル 1 でも） */
			pQ3->byLevelMin = 99;
			pOwner->byAdminLV = ADMINLV_ADMINISTRATOR;
			nBase0 = MockCmdCount(anConn[0], CMD_QUEST);
			QuestSendTalk(pOwner, dwGuild);
			printf(" | admin list=%d", MockCmdCount(anConn[0], CMD_QUEST) - nBase0 - nBase);
			QuestSendCmd(pOwner, QUESTCMD_ACCEPT, 3, dwGuild);
			printf(" accept=%u\n", pOwner->wQuestID);
			QuestSendCmd(pOwner, QUESTCMD_CANCEL, 0, dwGuild);
			pOwner->byAdminLV = ADMINLV_USER;
			/* 保存して読み直しても残る（古いファイルは 0） */
			pQ3->byLevelMin = 42;
			SaveQuestInfo();
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
			g_aQuestInfo.RemoveAll();
			LoadQuestInfo();
			pQ3 = NULL;
			int nIdx3 = 0;
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 3) { pQ3 = g_aQuestInfo[i]; nIdx3 = i; }
			printf("quest lvmin reload=%u", pQ3 ? pQ3->byLevelMin : 9999);
			for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 2) printf(" q2=%u", g_aQuestInfo[i]->byLevelMin);
			printf("\n");
			/* データAPI：設定・範囲外の拒否・足す時に省くと 0・取得に出る */
			printf("quest lvmin api set %s", FlagGroupApi("quest", nIdx3, "{\"byLevelMin\":50}").c_str());
			printf(" now=%u get=%d\n", pQ3 ? pQ3->byLevelMin : 9999, SboWebDataGet("quest", nIdx3) && strstr(SboWebDataGet("quest", nIdx3), "\"byLevelMin\":50") ? 1 : 0);
			printf("quest lvmin api max %s", FlagGroupApi("quest", nIdx3, "{\"byLevelMin\":99}").c_str());
			printf(" now=%u\n", pQ3 ? pQ3->byLevelMin : 9999);
			printf("quest lvmin api over %s", FlagGroupApi("quest", nIdx3, "{\"byLevelMin\":100}").c_str());
			printf(" now=%u\n", pQ3 ? pQ3->byLevelMin : 9999);
			printf("quest lvmin api zero %s", FlagGroupApi("quest", nIdx3, "{\"byLevelMin\":0}").c_str());
			printf(" now=%u\n", pQ3 ? pQ3->byLevelMin : 9999);
			pOwner->byLevel = byLvBack;
			pOwner->byAdminLV = byAdBack;
			if (pQ3) pQ3->byLevelMin = 0;
		}
	}

	/* 会話の行：受けている依頼の条件・アイテムを渡す（一度きりは「立てる印」と「いけない印」で組む） */
	{
		PTALKINFO pTalk = new TALKINFO;
		CHARINFO Npc;
		char szSpeech[8];
		ZeroMemory(pTalk, sizeof(*pTalk));
		pTalk->wID = 998; pTalk->byState = TALKSTATE_ON; pTalk->byLineCount = 3;
		TalkTestLineInit(&pTalk->aLine[0], 1);
		pTalk->aLine[0].wQuestID = 2; pTalk->aLine[0].byQuestStepMin = 1; pTalk->aLine[0].byQuestStepMax = 2;
		TalkTestLineInit(&pTalk->aLine[1], 2);
		pTalk->aLine[1].wGiveItem = wItemQ; pTalk->aLine[1].awSetOn[0] = 502; pTalk->aLine[1].awNot[0] = 502;
		TalkTestLineInit(&pTalk->aLine[2], 3);
		BYTE abyF[MAX_FLAGBITBYTE], abyN[MAX_FLAGNUM];
		ZeroMemory(abyF, sizeof(abyF));
		ZeroMemory(abyN, sizeof(abyN));
		printf("flagtalk quest cond none=%d q2s0=%d q2s1=%d q2s2=%d q2s3=%d q3s1=%d", TalkPickLine(pTalk, abyF, abyN, 1, 0, 0, NULL, NULL, 0),
			TalkPickLine(pTalk, abyF, abyN, 1, 2, 0, NULL, NULL, 0), TalkPickLine(pTalk, abyF, abyN, 1, 2, 1, NULL, NULL, 0), TalkPickLine(pTalk, abyF, abyN, 1, 2, 2, NULL, NULL, 0),
			TalkPickLine(pTalk, abyF, abyN, 1, 2, 3, NULL, NULL, 0), TalkPickLine(pTalk, abyF, abyN, 1, 3, 1, NULL, NULL, 0));
		pTalk->aLine[0].byQuestStepMax = QUESTNPC_NOSTEP;
		printf(" any s200=%d\n", TalkPickLine(pTalk, abyF, abyN, 1, 2, 200, NULL, NULL, 0));
		pTalk->aLine[0].byQuestStepMax = 2;
		g_aTalkInfo.Add(pTalk);

		ZeroMemory((PBYTE)&Npc, sizeof(CHARINFO));
		strcpy(szSpeech, "@t998");
		Npc.pszNpcSpeech = szSpeech;
		Npc.dwTblID = dwGuild;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		pP4->wQuestID = 0;
		FlagBitSet(pP4->abyFlag, 502, FALSE);
		int nItem0 = QuestCountItem(pP4, wItemQ);
		int nSys0 = MockCmdCount(anConn[3], CMD_SYSMSG), nTalk0 = MockCmdCount(anConn[3], CMD_TALKMSG), nLen0 = MockCmdLen(anConn[3]);
		TalkNpcTalk(pP4, &Npc);
		printf("talk give first item+=%d flag=%d talkmsg=%d sysmsg=%d", QuestCountItem(pP4, wItemQ) - nItem0, FlagBitGet(pP4->abyFlag, 502) ? 1 : 0,
			MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk0, MockCmdCount(anConn[3], CMD_SYSMSG) - nSys0);
		/* 送る順番：セリフの窓（TALKMSG）が先、「手に入れました」の白文字（SYSMSG）は窓の後（1=窓が先） */
		printf(" order_window_first=%d", MockCmdBefore(anConn[3], nLen0, CMD_TALKMSG, CMD_SYSMSG));
		nItem0 = QuestCountItem(pP4, wItemQ);
		nSys0 = MockCmdCount(anConn[3], CMD_SYSMSG);
		TalkNpcTalk(pP4, &Npc);
		printf(" second item+=%d sysmsg=%d", QuestCountItem(pP4, wItemQ) - nItem0, MockCmdCount(anConn[3], CMD_SYSMSG) - nSys0);
		/* 依頼を受けている間は依頼の行が選ばれ、アイテムは渡さない */
		FlagBitSet(pP4->abyFlag, 502, FALSE);
		pP4->wQuestID = 2; pP4->byQuestStep = 1;
		nItem0 = QuestCountItem(pP4, wItemQ);
		TalkNpcTalk(pP4, &Npc);
		printf(" quest item+=%d flag=%d", QuestCountItem(pP4, wItemQ) - nItem0, FlagBitGet(pP4->abyFlag, 502) ? 1 : 0);
		pP4->wQuestID = 0; pP4->byQuestStep = 0;
		/* 持ち物がいっぱいなら預かり所へ */
		for (int i = 0; i < MAX_MYITEM; i ++) pP4->waItem[i] = wItemR;
		nItem0 = QuestCountItem(pP4, wItemQ);
		TalkNpcTalk(pP4, &Npc);
		int nLeaveQ = 0;
		if (pP4->dwLeaveID && pP4->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			PLEAVEINFO pL = g_aLeaveInfo[pP4->dwLeaveID];
			for (int i = 0; i < pL->byCount; i ++) if (pL->waItem[i] == wItemQ) nLeaveQ += pL->byItemCount[i];
		}
		printf(" full item+=%d leave=%d\n", QuestCountItem(pP4, wItemQ) - nItem0, nLeaveQ);
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		if (pP4->dwLeaveID && pP4->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			ZeroMemory(g_aLeaveInfo[pP4->dwLeaveID], sizeof(LEAVEINFO));
			pP4->dwLeaveID = 0;
		}
		FlagBitSet(pP4->abyFlag, 502, FALSE);
		delete g_aTalkInfo[g_aTalkInfo.GetSize() - 1];
		g_aTalkInfo.RemoveAt(g_aTalkInfo.GetSize() - 1);
	}

	/* 会話の行：取り上げる物（引換券 wItemR → 景品 wItemQ）。取り上げる→渡すが通る／持っていない／受け取れない */
	{
		PTALKINFO pTalk = new TALKINFO;
		CHARINFO Npc;
		char szSpeech[8];
		WORD wFill = 0;
		for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
			if (g_aItemInfo[i]->szName[0] && i != wItemQ && i != wItemR) { wFill = (WORD)i; break; }
		}
		ZeroMemory(pTalk, sizeof(*pTalk));
		pTalk->wID = 997; pTalk->byState = TALKSTATE_ON; pTalk->byLineCount = 1;
		TalkTestLineInit(&pTalk->aLine[0], 1);
		pTalk->aLine[0].wTakeItem = wItemR; pTalk->aLine[0].wGiveItem = wItemQ;
		pTalk->aLine[0].awSetOn[0] = 503; pTalk->aLine[0].abyActNo[0] = 9; pTalk->aLine[0].abyActOp[0] = TALKNUMOP_ADD; pTalk->aLine[0].abyActVal[0] = 1;
		g_aTalkInfo.Add(pTalk);
		ZeroMemory((PBYTE)&Npc, sizeof(CHARINFO));
		strcpy(szSpeech, "@t997");
		Npc.pszNpcSpeech = szSpeech;
		Npc.dwTblID = dwGuild;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		for (int i = 0; i < MAX_EQUTYPE; i ++) pP4->waEqu[i] = 0xFFFF;
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		/* 券を持っている：券 -1・景品 +1・印と数値が動く・「渡しました」の知らせ（渡す知らせと合わせて 2 通） */
		pP4->waItem[0] = wItemR;
		int nTk = QuestCountItem(pP4, wItemR), nPz = QuestCountItem(pP4, wItemQ);
		int nSys = MockCmdCount(anConn[3], CMD_SYSMSG), nTalk = MockCmdCount(anConn[3], CMD_TALKMSG);
		TalkNpcTalk(pP4, &Npc);
		printf("talk take ok ticket%+d prize%+d flag=%d num=%d sysmsg=%d talkmsg=%d", QuestCountItem(pP4, wItemR) - nTk, QuestCountItem(pP4, wItemQ) - nPz,
			FlagBitGet(pP4->abyFlag, 503) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), MockCmdCount(anConn[3], CMD_SYSMSG) - nSys, MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk);
		/* 券を持っていない：何も動かない（台詞は出る） */
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		nPz = QuestCountItem(pP4, wItemQ);
		nSys = MockCmdCount(anConn[3], CMD_SYSMSG); nTalk = MockCmdCount(anConn[3], CMD_TALKMSG);
		TalkNpcTalk(pP4, &Npc);
		printf(" none prize%+d flag=%d num=%d sysmsg=%d talkmsg=%d", QuestCountItem(pP4, wItemQ) - nPz, FlagBitGet(pP4->abyFlag, 503) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9),
			MockCmdCount(anConn[3], CMD_SYSMSG) - nSys, MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk);
		/* 装備中の券は取らない（装備していない同じ券があればそれを取る） */
		pP4->waItem[0] = wItemR; pP4->waEqu[1] = 0;
		nPz = QuestCountItem(pP4, wItemQ);
		TalkNpcTalk(pP4, &Npc);
		printf(" equ prize%+d ticket=%d flag=%d", QuestCountItem(pP4, wItemQ) - nPz, QuestCountItem(pP4, wItemR), FlagBitGet(pP4->abyFlag, 503) ? 1 : 0);
		pP4->waItem[1] = wItemR;
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		TalkNpcTalk(pP4, &Npc);
		printf(" equ2 slot0=%d slot1=%d equ=%u", pP4->waItem[0] == wItemR ? 1 : 0, pP4->waItem[1] == wItemR ? 1 : 0, (unsigned)pP4->waEqu[1]);
		pP4->waEqu[1] = 0xFFFF;
		/* 手持ちを先に見る（手持ちと大事な物の欄の両方にあれば、手持ちの 1 つを取る） */
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		pP4->waItem[0] = wItemR; pP4->waImportant[0] = wItemR;
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		TalkNpcTalk(pP4, &Npc);
		int nBag = 0;
		for (int i = 0; i < MAX_MYITEM; i ++) if (pP4->waItem[i] == wItemR) nBag ++;
		printf(" order bag=%d imp=%d", nBag, CountImportantItem(pP4, wItemR));
		/* 渡す品を受け取れない（大事な物の欄がいっぱい）：券も取られず、印も数値も動かない。今までと同じ知らせ */
		BYTE byFlgBak = g_aItemInfo[wItemQ]->byFlg;
		g_aItemInfo[wItemQ]->byFlg |= ITEMFLG_IMPORTANT;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		for (int i = 0; i < MAX_IMPORTANT; i ++) pP4->waImportant[i] = wFill;
		pP4->waItem[0] = wItemR;
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		nSys = MockCmdCount(anConn[3], CMD_SYSMSG); nTalk = MockCmdCount(anConn[3], CMD_TALKMSG);
		TalkNpcTalk(pP4, &Npc);
		printf(" full ticket=%d prize=%d flag=%d num=%d sysmsg=%d talkmsg=%d\n", QuestCountItem(pP4, wItemR), QuestCountItem(pP4, wItemQ), FlagBitGet(pP4->abyFlag, 503) ? 1 : 0,
			FlagNumGet(pP4->abyFlagNum, 9), MockCmdCount(anConn[3], CMD_SYSMSG) - nSys, MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk);
		g_aItemInfo[wItemQ]->byFlg = byFlgBak;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		FlagBitSet(pP4->abyFlag, 503, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		delete g_aTalkInfo[g_aTalkInfo.GetSize() - 1];
		g_aTalkInfo.RemoveAt(g_aTalkInfo.GetSize() - 1);
	}

	/* 会話の行：「はい・いいえ」で聞く（byAsk）。聞く行では印・数値・渡す物が動かない→はいで動く→いいえは動かず返事→
	   遠い・時間切れ・保留なし・ほかの NPC への答えは無視→古い会話データを読むと byAsk=0 */
	{
		PTALKINFO pTalk = new TALKINFO;
		PCHARINFO pNpcReal = g_aCharInfo[dwGuild];
		char szSpeechBak[8];
		DWORD dwMapB = pP4->dwMapID, dwXB = pP4->dwX, dwYB = pP4->dwY;
		std::vector<BYTE> aCap;
		WORD wFill = 0;
		for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
			if (g_aItemInfo[i]->szName[0] && i != wItemQ && i != wItemR) { wFill = (WORD)i; break; }
		}
		ZeroMemory(pTalk, sizeof(*pTalk));
		pTalk->wID = 996; pTalk->byState = TALKSTATE_ON; pTalk->byLineCount = 1;
		TalkTestLineInit(&pTalk->aLine[0], 1);
		pTalk->aLine[0].byAsk = 1;
		strcpy(pTalk->aLine[0].szSpeech, "ほしいですか？");
		strcpy(pTalk->aLine[0].szNoMsg, "また今度ね。");
		pTalk->aLine[0].wGiveItem = wItemQ;
		pTalk->aLine[0].awSetOn[0] = 504; pTalk->aLine[0].abyActNo[0] = 9; pTalk->aLine[0].abyActOp[0] = TALKNUMOP_ADD; pTalk->aLine[0].abyActVal[0] = 1;
		g_aTalkInfo.Add(pTalk);
		/* 本物の NPC（ギルド受付）の台詞を一時的に @t996 にして、そのそばに置く */
		strcpy(szSpeechBak, pNpcReal->pszNpcSpeech);
		strcpy(pNpcReal->pszNpcSpeech, "@t996");
		pP4->dwMapID = pNpcReal->dwMapID; pP4->dwX = pNpcReal->dwX + 1; pP4->dwY = pNpcReal->dwY;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		TalkPendClear(pP4->dwTblID);

		/* 送った台詞の中身を調べる（横取りの間は誰にも送られないので、数えるのは別に 1 回ずつ流す） */
		struct Has {
			static bool In(const std::vector<BYTE> &a, const char *psz) {
				size_t n = strlen(psz);
				for (size_t i = 0; i + n <= a.size(); i ++) if (memcmp(&a[i], psz, n) == 0) return true;
				return false;
			}
		};
		/* 話す：台詞（末尾に @y）は届くが、印・数値・渡す物は動かない。保留ができる */
		SvrSendCaptureBegin();
		TalkNpcTalk(pP4, pNpcReal);
		SvrSendCaptureEnd(aCap);
		bool bTail = Has::In(aCap, "ほしいですか？@y") && !Has::In(aCap, "@2");
		int nTalk = MockCmdCount(anConn[3], CMD_TALKMSG), nPz = QuestCountItem(pP4, wItemQ);
		TalkNpcTalk(pP4, pNpcReal);
		printf("talk ask first talkmsg=%d tail=%d flag=%d num=%d item%+d pend=%d", MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk, bTail ? 1 : 0,
			FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), QuestCountItem(pP4, wItemQ) - nPz, TalkPendGet(pP4->dwTblID) ? 1 : 0);
		/* はい（本物のパケット経由。一般のプレイヤーが送れる）：印・数値・渡す物が動いて、保留が消える */
		CMDPARA_DATA1 Ans;
		ZeroMemory(&Ans, sizeof(Ans));
		Ans.byCmd = CMD_TALKANSWER; Ans.byData = 1; Ans.dwTblID = dwGuild;
		nPz = QuestCountItem(pP4, wItemQ);
		TestRecv(pP4, &Ans, sizeof(Ans));
		printf(" | yes flag=%d num=%d item%+d pend=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), QuestCountItem(pP4, wItemQ) - nPz, TalkPendGet(pP4->dwTblID) ? 1 : 0);
		/* 保留が無い時の答えは無視（二重に送っても 1 回しか動かない） */
		nPz = QuestCountItem(pP4, wItemQ);
		TestRecv(pP4, &Ans, sizeof(Ans));
		printf(" again flag=%d num=%d item%+d\n", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), QuestCountItem(pP4, wItemQ) - nPz);

		/* いいえ：何も動かず、返事（szNoMsg）が届く。保留は消える */
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		Ans.byData = 0;
		TalkNpcTalk(pP4, pNpcReal);
		SvrSendCaptureBegin();
		TestRecv(pP4, &Ans, sizeof(Ans));
		SvrSendCaptureEnd(aCap);
		bool bNo1 = Has::In(aCap, "また今度ね。@2");
		TalkNpcTalk(pP4, pNpcReal);
		nTalk = MockCmdCount(anConn[3], CMD_TALKMSG); nPz = QuestCountItem(pP4, wItemQ);
		TestRecv(pP4, &Ans, sizeof(Ans));
		printf("talk ask no talkmsg=%d msg=%d flag=%d num=%d item%+d pend=%d", MockCmdCount(anConn[3], CMD_TALKMSG) - nTalk, bNo1 ? 1 : 0,
			FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), QuestCountItem(pP4, wItemQ) - nPz, TalkPendGet(pP4->dwTblID) ? 1 : 0);
		/* いいえの返事が空なら既定の一言 */
		pTalk->aLine[0].szNoMsg[0] = 0;
		TalkNpcTalk(pP4, pNpcReal);
		SvrSendCaptureBegin();
		TalkAnswer(pP4, dwGuild, FALSE);
		SvrSendCaptureEnd(aCap);
		bool bNoDef = Has::In(aCap, TALK_NOMSG_DEFAULT "@2");
		printf(" default msg=%d\n", bNoDef ? 1 : 0);

		/* 遠い：答えは無視（保留は消える） */
		TalkNpcTalk(pP4, pNpcReal);
		pP4->dwX = pNpcReal->dwX + 5;
		TalkAnswer(pP4, dwGuild, TRUE);
		printf("talk ask far flag=%d num=%d pend=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), TalkPendGet(pP4->dwTblID) ? 1 : 0);
		/* 2 マスまでは受ける */
		pP4->dwX = pNpcReal->dwX + 2;
		TalkNpcTalk(pP4, pNpcReal);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" near2 flag=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		pP4->dwX = pNpcReal->dwX + 1;
		/* 別のマップに移った */
		TalkNpcTalk(pP4, pNpcReal);
		pP4->dwMapID = pNpcReal->dwMapID + 1;
		TalkAnswer(pP4, dwGuild, TRUE);
		pP4->dwMapID = pNpcReal->dwMapID;
		printf(" othermap flag=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		/* 別の空間に移った */
		TalkNpcTalk(pP4, pNpcReal);
		pP4->dwSpaceID = 5;
		TalkAnswer(pP4, dwGuild, TRUE);
		pP4->dwSpaceID = 0;
		printf(" otherspace flag=%d\n", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);

		/* 時間切れ（60 秒）：59 秒なら受け、61 秒は捨てる */
		TalkNpcTalk(pP4, pNpcReal);
		TalkPendGet(pP4->dwTblID)->dwTime = timeGetTime() - (TALK_ASK_TIMEOUT - 1000);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf("talk ask time 59s flag=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		TalkNpcTalk(pP4, pNpcReal);
		TalkPendGet(pP4->dwTblID)->dwTime = timeGetTime() - (TALK_ASK_TIMEOUT + 1000);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" 61s flag=%d pend=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, TalkPendGet(pP4->dwTblID) ? 1 : 0);
		/* ほかの NPC への答えは無視（保留はそのまま残り、本物の NPC への答えは通る） */
		TalkNpcTalk(pP4, pNpcReal);
		TalkAnswer(pP4, pOwner->dwTblID, TRUE);
		printf(" otherNpc flag=%d pend=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, TalkPendGet(pP4->dwTblID) ? 1 : 0);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" then flag=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		/* ほかの NPC に話しかけると保留は消える（話しかけの通信 CMD_TALKRAND 経由） */
		TalkNpcTalk(pP4, pNpcReal);
		CMDPARA_DATA1 Rand;
		ZeroMemory(&Rand, sizeof(Rand));
		Rand.byCmd = CMD_TALKRAND; Rand.dwTblID = pOwner->dwTblID;
		TestRecv(pP4, &Rand, sizeof(Rand));
		printf(" talkother pend=%d", TalkPendGet(pP4->dwTblID) ? 1 : 0);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" flag=%d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		/* ログアウトで消える */
		TalkNpcTalk(pP4, pNpcReal);
		TalkPendClear(pP4->dwTblID);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" cleared flag=%d\n", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);

		/* はいの時にその行がもう条件に合わない（印が変わった）なら何も動かさない。渡す物が受け取れなくても印は動かさない */
		pTalk->aLine[0].awNeed[0] = 505;
		FlagBitSet(pP4->abyFlag, 505, TRUE);
		TalkNpcTalk(pP4, pNpcReal);
		FlagBitSet(pP4->abyFlag, 505, FALSE);
		nPz = QuestCountItem(pP4, wItemQ);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf("talk ask cond gone flag=%d item%+d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, QuestCountItem(pP4, wItemQ) - nPz);
		pTalk->aLine[0].awNeed[0] = FLAG_NOBIT;
		BYTE byFlgBak = g_aItemInfo[wItemQ]->byFlg;
		g_aItemInfo[wItemQ]->byFlg |= ITEMFLG_IMPORTANT;
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		for (int i = 0; i < MAX_IMPORTANT; i ++) pP4->waImportant[i] = wFill;
		nPz = QuestCountItem(pP4, wItemQ);
		TalkNpcTalk(pP4, pNpcReal);
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" full flag=%d num=%d item%+d", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0, FlagNumGet(pP4->abyFlagNum, 9), QuestCountItem(pP4, wItemQ) - nPz);
		g_aItemInfo[wItemQ]->byFlg = byFlgBak;
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		/* 聞く行をやめた後の答えは無視 */
		TalkNpcTalk(pP4, pNpcReal);
		pTalk->aLine[0].byAsk = 0;
		TalkAnswer(pP4, dwGuild, TRUE);
		printf(" askoff flag=%d\n", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		pTalk->aLine[0].byAsk = 1;

		/* 管理者が送った時も、遠ければ無視（管理者でも近くで答える） */
		TalkNpcTalk(pP4, pNpcReal);
		pP4->byAdminLV = ADMINLV_ADMINISTRATOR;
		pP4->dwX = pNpcReal->dwX + 9;
		TestRecv(pP4, &Ans, sizeof(Ans));
		pP4->byAdminLV = ADMINLV_USER;
		pP4->dwX = pNpcReal->dwX + 1;
		printf("talk ask admin far flag=%d\n", FlagBitGet(pP4->abyFlag, 504) ? 1 : 0);
		Ans.byData = 1;
		TalkPendClear(pP4->dwTblID);

		/* 保存して読み直す：聞く行・返事はそのまま。古い会話データ（新しい列が無い）では byAsk=0・返事は空 */
		{
			extern BOOL SaveTalkInfo(void);
			extern BOOL LoadTalkInfo(void);
			std::vector<PTALKINFO> aBak;
			char szFile[MAX_PATH + 1];
			strcpy(pTalk->aLine[0].szNoMsg, "また今度ね。");
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) aBak.push_back(g_aTalkInfo[i]);
			SaveTalkInfo();
			g_aTalkInfo.RemoveAll();
			LoadTalkInfo();
			PTALKINFO pLoad = NULL;
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) if (g_aTalkInfo[i]->wID == 996) pLoad = g_aTalkInfo[i];
			printf("talk ask reload ask=%d nomsg=%s speech=%s", pLoad ? pLoad->aLine[0].byAsk : 9, pLoad ? pLoad->aLine[0].szNoMsg : "-", pLoad ? pLoad->aLine[0].szSpeech : "-");
			/* 保存したファイルから新しい列（byAsk・返事）の見出しだけ外して、古い形にする */
			GetModuleFileName(NULL, szFile, MAX_PATH);
			for (int i = (int)strlen(szFile) - 1; i > 0; i --) if (szFile[i] == '\\') { szFile[i + 1] = 0; break; }
			strcat(szFile, "SBODATA\\SboDataTalkInfo.dat");
			{
				std::vector<BYTE> aFile;
				HANDLE hFile = CreateFile(szFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile != INVALID_HANDLE_VALUE) {
					DWORD dwSize = GetFileSize(hFile, NULL), dwBytes = 0;
					aFile.resize(dwSize);
					if (dwSize) ReadFile(hFile, &aFile[0], dwSize, &dwBytes, NULL);
					CloseHandle(hFile);
				}
				DWORD dwCols = 0, dwData = 0;
				if (aFile.size() > 8) { memcpy(&dwCols, &aFile[0], 4); memcpy(&dwData, &aFile[4], 4); }
				std::vector<std::pair<std::string, DWORD> > aCol;
				size_t nPos = 8;
				for (DWORD i = 0; i < dwCols && nPos < aFile.size(); i ++) {
					std::string strName((const char *)&aFile[nPos]);
					nPos += strName.size() + 1;
					DWORD dwOff;
					memcpy(&dwOff, &aFile[nPos], 4);
					nPos += 4;
					aCol.push_back(std::make_pair(strName, dwOff));
				}
				DWORD dwOldHead = (DWORD)nPos;
				std::vector<std::pair<std::string, DWORD> > aKeep;
				for (size_t i = 0; i < aCol.size(); i ++) {
					if (aCol[i].first != "abyAsk" && aCol[i].first.compare(0, 7, "szNoMsg") != 0) aKeep.push_back(aCol[i]);
				}
				DWORD dwNewHead = 8;
				for (size_t i = 0; i < aKeep.size(); i ++) dwNewHead += (DWORD)aKeep[i].first.size() + 1 + 4;
				std::vector<BYTE> aOut;
				DWORD dwTmp = (DWORD)aKeep.size();
				aOut.insert(aOut.end(), (BYTE *)&dwTmp, (BYTE *)&dwTmp + 4);
				aOut.insert(aOut.end(), (BYTE *)&dwData, (BYTE *)&dwData + 4);
				for (size_t i = 0; i < aKeep.size(); i ++) {
					aOut.insert(aOut.end(), aKeep[i].first.c_str(), aKeep[i].first.c_str() + aKeep[i].first.size() + 1);
					dwTmp = aKeep[i].second - dwOldHead + dwNewHead;
					aOut.insert(aOut.end(), (BYTE *)&dwTmp, (BYTE *)&dwTmp + 4);
				}
				aOut.insert(aOut.end(), aFile.begin() + dwOldHead, aFile.end());
				hFile = CreateFile(szFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile != INVALID_HANDLE_VALUE) {
					DWORD dwBytes = 0;
					WriteFile(hFile, &aOut[0], (DWORD)aOut.size(), &dwBytes, NULL);
					CloseHandle(hFile);
				}
			}
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) delete g_aTalkInfo[i];
			g_aTalkInfo.RemoveAll();
			LoadTalkInfo();
			pLoad = NULL;
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) if (g_aTalkInfo[i]->wID == 996) pLoad = g_aTalkInfo[i];
			printf(" | old ask=%d nomsg=%s speech=%s\n", pLoad ? pLoad->aLine[0].byAsk : 9, (pLoad && pLoad->aLine[0].szNoMsg[0]) ? pLoad->aLine[0].szNoMsg : "(empty)", pLoad ? pLoad->aLine[0].szSpeech : "-");
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) delete g_aTalkInfo[i];
			g_aTalkInfo.RemoveAll();
			for (size_t i = 0; i < aBak.size(); i ++) g_aTalkInfo.Add(aBak[i]);
			pTalk = NULL;
			for (int i = 0; i < g_aTalkInfo.GetSize(); i ++) if (g_aTalkInfo[i]->wID == 996) pTalk = g_aTalkInfo[i];
		}

		/* 元に戻す */
		strcpy(pNpcReal->pszNpcSpeech, szSpeechBak);
		pP4->dwMapID = dwMapB; pP4->dwX = dwXB; pP4->dwY = dwYB;
		TalkPendClear(pP4->dwTblID);
		ZeroMemory(pP4->waItem, sizeof(pP4->waItem));
		ZeroMemory(pP4->waImportant, sizeof(pP4->waImportant));
		FlagBitSet(pP4->abyFlag, 504, FALSE);
		FlagNumSet(pP4->abyFlagNum, 9, 0);
		if (pTalk) {
			for (int i = g_aTalkInfo.GetSize() - 1; i >= 0; i --) {
				if (g_aTalkInfo[i] == pTalk) { delete pTalk; g_aTalkInfo.RemoveAt(i); break; }
			}
		}
	}

	/* 保存して読み直す：依頼情報（見えるNPC もそのまま） */
	{
		std::vector<QUESTINFO> aBak;
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) aBak.push_back(*g_aQuestInfo[i]);
		SaveQuestInfo();
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
		g_aQuestInfo.RemoveAll();
		LoadQuestInfo();
		int nSame = (g_aQuestInfo.GetSize() == (int)aBak.size());
		for (int i = 0; nSame && i < g_aQuestInfo.GetSize(); i ++) {
			if (memcmp(g_aQuestInfo[i], &aBak[i], sizeof(QUESTINFO)) != 0) nSame = 0;
		}
		printf("quest info reload count=%d same=%d\n", g_aQuestInfo.GetSize(), nSame);
	}

	/* 保存して読み直す：キャラの進み具合（段・旗・数・済みの印） */
	pOther->wQuestID = 2; pOther->byQuestStep = 1; pOther->wQuestFlg = 0x1234;
	pOther->awQuestCount[0] = 1; pOther->awQuestCount[1] = 22; pOther->awQuestCount[2] = 333; pOther->awQuestCount[3] = 4444;
	pOther->dwQuestAcceptTime = 1700000000;
	ZeroMemory(pOther->abyQuestDone, sizeof(pOther->abyQuestDone));
	ZeroMemory(pOther->abyQuestRewarded, sizeof(pOther->abyQuestRewarded));
	pOther->abyQuestDone[0] = 0x06; pOther->abyQuestDone[63] = 0x80;
	pOther->abyQuestRewarded[0] = 0x02; pOther->abyQuestRewarded[63] = 0x80;
	strcpy(pOther->szAcc, "questtest");
	strcpy(pOther->szName, "依頼テスト");
	BOOL bSaved = SaveCharInfo();
	/* 見える・見えない条件の印を、全員 1（=印 0 の条件）にしてから読み直す：保存では「無し」が 0 で入っているので、
	   読み込みで必ず「無し」（0）に戻る */
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) g_aCharInfo[i]->wShowFlag = g_aCharInfo[i]->wHideFlag = 1;
	BOOL bLoaded = LoadCharInfo();
	printf("quest char save=%d load=%d\n", (int)bSaved, (int)bLoaded);
	PCHARINFO pLoaded = NULL;
	/* 読み込んだキャラは空き枠に入ることもあるので、元の持ち主以外で探す */
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (g_aCharInfo[i] != pOther && strcmp(g_aCharInfo[i]->szAcc, "questtest") == 0) pLoaded = g_aCharInfo[i];
	}
	if (pLoaded) {
		printf("quest char reload id=%u step=%u flg=%u counts=%u,%u,%u,%u time=%u done=%02x,%02x\n", pLoaded->wQuestID, pLoaded->byQuestStep,
			pLoaded->wQuestFlg, pLoaded->awQuestCount[0], pLoaded->awQuestCount[1], pLoaded->awQuestCount[2], pLoaded->awQuestCount[3],
			pLoaded->dwQuestAcceptTime, pLoaded->abyQuestDone[0], pLoaded->abyQuestDone[63]);
		printf("quest char reload rewarded=%02x,%02x\n", pLoaded->abyQuestRewarded[0], pLoaded->abyQuestRewarded[63]);
	} else {
		printf("quest char reload not found\n");
	}
	/* 読み込んだキャラの条件は「無し」。ほかのキャラは元（無し）に戻す */
	if (pLoaded) {
		printf("quest char reload viewcond show=%d hide=%d\n", (int)pLoaded->wShowFlag, (int)pLoaded->wHideFlag);
	}
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) g_aCharInfo[i]->wShowFlag = g_aCharInfo[i]->wHideFlag = 0;
	/* 見えるNPC（保存しない）は保存データに入らない */
	{
		int nQuestNpcLoaded = 0;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			/* ここまでに作ったもの（持ち主が見える分）を除き、読み込みで増えた見えるNPC が無いこと */
			if (p->byNpcType == NPCTYPE_QUESTNPC && p->pProc == NULL) nQuestNpcLoaded ++;
		}
		printf("quest npc loaded=%d\n", nQuestNpcLoaded);
	}

	/* 依頼の品は預けられない・売れない。預かり所が100種類なら断られる（どちらも結果が返る） */
	{
		NearNpcOpen(pOther);	/* 預かり所・店の人が近くにいる時だけ使える */
		ZeroMemory(pOther->waItem, sizeof(pOther->waItem));
		pOther->waItem[0] = wItemQ;
		pOther->waItem[1] = wItemR;
		CMDPARA_DATA1 Leave;
		ZeroMemory(&Leave, sizeof(Leave));
		Leave.byCmd = CMD_LEAVEITEM;
		Leave.pID = pOther->pID;
		Leave.dwTblID = pOther->dwTblID;
		MockTrace(1);
		Leave.byData = 0;
		TestRecv(pOther, &Leave, sizeof(Leave));
		int nQ = QuestCountItem(pOther, wItemQ);
		CMDPARA_DATA5 Sell;
		ZeroMemory(&Sell, sizeof(Sell));
		Sell.byCmd = CMD_DROPITEM;
		Sell.byData = 0;
		Sell.pID = pOther->pID;
		Sell.dwTblID = pOther->dwTblID;
		Sell.dwData1 = 1;
		Sell.dwData2 = wItemQ;
		DWORD dwGold0 = pOther->dwGold;
		TestRecv(pOther, &Sell, sizeof(Sell));
		int nQSell = QuestCountItem(pOther, wItemQ);
		/* 預かり所を100種類で埋める */
		AddLeaveItem(pOther, wItemR);
		PLEAVEINFO pL = g_aLeaveInfo[pOther->dwLeaveID];
		LEAVEINFO LeaveBack = *pL;
		for (int i = 0; i < MAX_LEAVEITEM; i ++) { pL->waItem[i] = (WORD)(60000 + i); pL->byItemCount[i] = 1; }
		pL->byCount = MAX_LEAVEITEM;
		Leave.byData = 1;
		TestRecv(pOther, &Leave, sizeof(Leave));
		int nR = QuestCountItem(pOther, wItemR);
		*pL = LeaveBack;
		Leave.byData = 1;
		TestRecv(pOther, &Leave, sizeof(Leave));
		MockTrace(0);
		char *psz = MockCmdData(anConn[2], CMD_LEAVEITEM);
		printf("quest leave questitem=%d sell=%d gold+=%d full=%d res=%s after=%d\n", nQ, nQSell, (int)(pOther->dwGold - dwGold0), nR, psz, QuestCountItem(pOther, wItemR));
		free(psz);
		NearNpcClose();
	}

	/* 古い形式（持ち物 10 個）のキャラを読むと、11〜20 枠は空のままで、後ろの列もずれない。
	   いま保存したファイルを「持ち物の列を 10 個分に縮めた形」に書き換えて読み直す */
	{
		WORD awItemBak[MAX_MYITEM], awEquBak[MAX_EQUTYPE];
		char szAccBak[sizeof(pOther->szAcc)], szNameBak[sizeof(pOther->szName)];
		memcpy(awItemBak, pOther->waItem, sizeof(awItemBak));
		memcpy(awEquBak, pOther->waEqu, sizeof(awEquBak));
		memcpy(szAccBak, pOther->szAcc, sizeof(szAccBak));
		memcpy(szNameBak, pOther->szName, sizeof(szNameBak));
		for (int i = 0; i < MAX_MYITEM; i ++) pOther->waItem[i] = (WORD)(i < 10 ? i + 1 : 777);
		for (int i = 0; i < MAX_EQUTYPE; i ++) pOther->waEqu[i] = 0xFFFF;
		pOther->waEqu[0] = 3;
		strcpy(pOther->szAcc, "legacyitem");
		strcpy(pOther->szName, "旧持ち物");
		SaveCharInfo();

		const char *pszFile = "SBODATA/SboDataCharInfo.dat";
		std::vector<BYTE> aFile;
		FILE *fp = fopen(pszFile, "rb");
		if (fp) {
			BYTE abyBuf[4096];
			size_t n;
			while ((n = fread(abyBuf, 1, sizeof(abyBuf), fp)) > 0) aFile.insert(aFile.end(), abyBuf, abyBuf + n);
			fclose(fp);
		}
		/* ヘッダ：列数・データ数、続いて「列名 + 開始位置」 */
		DWORD dwColumn = 0, dwData = 0;
		size_t nHead = 8, nCountPos = 0, nItemPos = 0;
		DWORD dwCountOfs = 0, dwItemOfs = 0;
		if (aFile.size() > 8) {
			memcpy(&dwColumn, &aFile[0], 4);
			memcpy(&dwData, &aFile[4], 4);
			for (DWORD c = 0; c < dwColumn; c ++) {
				const char *pszName = (const char *)&aFile[nHead];
				size_t nPos = nHead + strlen(pszName) + 1;
				DWORD dwOfs;
				memcpy(&dwOfs, &aFile[nPos], 4);
				if (strcmp(pszName, "waItemCount") == 0) { nCountPos = nPos; dwCountOfs = dwOfs; }
				if (strcmp(pszName, "waItem") == 0) { nItemPos = nPos; dwItemOfs = dwOfs; }
				nHead = nPos + 4;
			}
		}
		int nPatched = 0;
		if (nCountPos && nItemPos && dwData > 0) {
			const DWORD dwNew = 10 * sizeof(WORD), dwOld = MAX_MYITEM * sizeof(WORD);
			std::vector<BYTE> aNew(aFile.begin(), aFile.begin() + dwItemOfs);
			for (DWORD r = 0; r < dwData; r ++) aFile[dwCountOfs + r] = 10;
			memcpy(&aNew[dwCountOfs], &aFile[dwCountOfs], dwData);
			for (DWORD r = 0; r < dwData; r ++) aNew.insert(aNew.end(), aFile.begin() + dwItemOfs + r * dwOld, aFile.begin() + dwItemOfs + r * dwOld + dwNew);
			aNew.insert(aNew.end(), aFile.begin() + dwItemOfs + dwData * dwOld, aFile.end());
			/* 持ち物の列より後ろの列は、縮めた分だけ開始位置を前へ */
			size_t nPos = 8;
			for (DWORD c = 0; c < dwColumn; c ++) {
				size_t nPosOfs = nPos + strlen((const char *)&aNew[nPos]) + 1;
				DWORD dwOfs;
				memcpy(&dwOfs, &aNew[nPosOfs], 4);
				if (dwOfs > dwItemOfs) {
					dwOfs -= dwData * (dwOld - dwNew);
					memcpy(&aNew[nPosOfs], &dwOfs, 4);
				}
				nPos = nPosOfs + 4;
			}
			fp = fopen(pszFile, "wb");
			if (fp) {
				fwrite(&aNew[0], 1, aNew.size(), fp);
				fclose(fp);
				nPatched = 1;
			}
		}
		LoadCharInfo();
		PCHARINFO pLegacy = NULL;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (g_aCharInfo[i] != pOther && strcmp(g_aCharInfo[i]->szAcc, "legacyitem") == 0) pLegacy = g_aCharInfo[i];
		}
		if (pLegacy) {
			int nHead10 = 0, nTail0 = 0;
			for (int i = 0; i < 10; i ++) if (pLegacy->waItem[i] == i + 1) nHead10 ++;
			for (int i = 10; i < MAX_MYITEM; i ++) if (pLegacy->waItem[i] == 0) nTail0 ++;
			printf("legacy item load patched=%d first10=%d rest0=%d equ=%u,%u name=%s\n", nPatched, nHead10, nTail0,
				pLegacy->waEqu[0], pLegacy->waEqu[1], pLegacy->szName);
			pLegacy->szAcc[0] = 0;
		} else {
			printf("legacy item load patched=%d not found\n", nPatched);
		}
		memcpy(pOther->waItem, awItemBak, sizeof(awItemBak));
		memcpy(pOther->waEqu, awEquBak, sizeof(awEquBak));
		memcpy(pOther->szAcc, szAccBak, sizeof(szAccBak));
		memcpy(pOther->szName, szNameBak, sizeof(szNameBak));
	}

	/* 思い出の本：古い保存データ（もらい済みの列が無い）を読むと、クリア済みの依頼はもらい済みになる */
	{
		BYTE abyDoneBak[MAX_QUESTDONEBYTE];
		char szAccBak[sizeof(pOther->szAcc)], szNameBak[sizeof(pOther->szName)];
		memcpy(abyDoneBak, pOther->abyQuestDone, sizeof(abyDoneBak));
		memcpy(szAccBak, pOther->szAcc, sizeof(szAccBak));
		memcpy(szNameBak, pOther->szName, sizeof(szNameBak));
		strcpy(pOther->szAcc, "storybf");
		strcpy(pOther->szName, "旧もらい済み");
		ZeroMemory(pOther->abyQuestDone, sizeof(pOther->abyQuestDone));
		ZeroMemory(pOther->abyQuestRewarded, sizeof(pOther->abyQuestRewarded));
		pOther->abyQuestDone[0] = 0x06;
		SaveCharInfo();
		/* 保存した列名を「別の名前」に書き換えて、古いデータ（列が無い）にする */
		const char *pszFile = "SBODATA/SboDataCharInfo.dat";
		std::vector<BYTE> aFile;
		FILE *fp = fopen(pszFile, "rb");
		if (fp) {
			BYTE abyBuf[4096];
			size_t n;
			while ((n = fread(abyBuf, 1, sizeof(abyBuf), fp)) > 0) aFile.insert(aFile.end(), abyBuf, abyBuf + n);
			fclose(fp);
		}
		int nPatched = 0;
		if (aFile.size() > 8) {
			DWORD dwColumn = 0;
			size_t nHead = 8;
			memcpy(&dwColumn, &aFile[0], 4);
			for (DWORD c = 0; c < dwColumn; c ++) {
				char *pszName = (char *)&aFile[nHead];
				if (strcmp(pszName, "abyQuestRewarded") == 0) { pszName[0] = 'X'; nPatched ++; }
				nHead += strlen(pszName) + 1 + 4;
			}
			fp = fopen(pszFile, "wb");
			if (fp) { fwrite(&aFile[0], 1, aFile.size(), fp); fclose(fp); }
		}
		LoadCharInfo();
		PCHARINFO pLoaded = NULL;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (g_aCharInfo[i] != pOther && strcmp(g_aCharInfo[i]->szAcc, "storybf") == 0) pLoaded = g_aCharInfo[i];
		}
		if (pLoaded) {
			printf("quest old save patched=%d done=%02x rewarded=%02x\n", nPatched, pLoaded->abyQuestDone[0], pLoaded->abyQuestRewarded[0]);
			pLoaded->szAcc[0] = 0;
		} else {
			printf("quest old save patched=%d not found\n", nPatched);
		}
		memcpy(pOther->abyQuestDone, abyDoneBak, sizeof(abyDoneBak));
		memcpy(pOther->szAcc, szAccBak, sizeof(szAccBak));
		memcpy(pOther->szName, szNameBak, sizeof(szNameBak));
	}

	/* 片付け */
	pOther->szAcc[0] = 0;
	for (int i = 0; i < 4; i ++) {
		apPC[i]->wQuestID = 0; apPC[i]->byQuestStep = 0; apPC[i]->wQuestFlg = 0;
		ZeroMemory(apPC[i]->awQuestCount, sizeof(apPC[i]->awQuestCount));
		ZeroMemory(apPC[i]->abyQuestDone, sizeof(apPC[i]->abyQuestDone));
		ZeroMemory(apPC[i]->abyQuestRewarded, sizeof(apPC[i]->abyQuestRewarded));
		apPC[i]->dwPartyID = 0;
		memcpy(apPC[i]->waItem, awItemBack[i], sizeof(awItemBack[i]));
		apPC[i]->dwGold = dwGoldBack[i];
		QuestMovePC(apPC[i], (int)dwMapBack[i], (int)dwXBack[i], (int)dwYBack[i]);
	}
	QuestSyncAll();
	printf("quest end copies=%d\n", QuestCountCopies());
}

/* ------------------------------------------------------------------------- */
/* P. 討伐チャレンジ（貸し出しの状態で戦い、どう終わっても元に戻る）		 */
/* ------------------------------------------------------------------------- */
static int ChallengeCountEnemy(void)
{
	int n = 0;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (QuestIsChallengeEnemy(g_aCharInfo[i])) n ++;
	}
	return n;
}

static PCHARINFO ChallengeFindEnemy(void)
{
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (QuestIsChallengeEnemy(g_aCharInfo[i])) return g_aCharInfo[i];
	}
	return NULL;
}

static void ChallengePrint(const char *pszLabel, PCHARINFO p, WORD wWeapon)
{
	printf("challenge %s in=%d lv=%u pw=%u hp=%u/%u mana=%u gold=%u item0=%d weapon_equ=%d\n", pszLabel, (int)QuestInChallenge(p),
		p->byLevel, p->byPW, p->wHP, p->wMaxHP, p->wMaxMana, p->dwGold, (int)(p->waItem[0] == wWeapon),
		(int)(p->waEqu[0] < MAX_MYITEM && p->waItem[p->waEqu[0]] == wWeapon));
}

static void TestChallenge(const std::vector<int> &aLogin)
{
	printf("== Q. Challenge ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO apPC[3] = { pA, pB, pC };
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];

	/* 本物の状態（テスト用に決める。最後に戻す） */
	CHARINFO aBack[3];
	for (int i = 0; i < 3; i ++) {
		TestCharBackup(&aBack[i], apPC[i]);
		PCHARINFO p = apPC[i];
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
		ZeroMemory(p->abyQuestRewarded, sizeof(p->abyQuestRewarded));
		p->byLevel = 30; p->byPW = 99; p->wMaxHP = 500; p->wHP = 480; p->wMaxMana = 90; p->wMana = 80; p->dwGold = 5000;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int k = 0; k < MAX_EQUTYPE; k ++) p->waEqu[k] = 0xFFFF;
	}

	/* 空いているマス */
	std::vector<std::pair<int, int> > aCell;
	for (int r = 1; r < 12 && aCell.size() < 8; r ++) {
		for (int dy = -r; dy <= r && aCell.size() < 8; dy ++) {
			for (int dx = -r; dx <= r && aCell.size() < 8; dx ++) {
				if (abs(dx) != r && abs(dy) != r) continue;
				int x = (int)pA->dwX + dx, y = (int)pA->dwY + dy;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
	}
	printf("challenge cells=%d\n", (int)aCell.size());
	if (aCell.size() < 8) {
		return;
	}

	/* 貸し出しに使うアイテム（武器・鎧・HP回復を 1 つずつ） */
	WORD wWeapon = 0, wArmor = 0, wPotion = 0, wMine = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pI = g_aItemInfo[i];
		if (pI->szName[0] == 0) continue;
		if (!wWeapon && pI->byType == ITEMTYPE_WEAPON) wWeapon = (WORD)i;
		else if (!wArmor && pI->byType == ITEMTYPE_ARMOR) wArmor = (WORD)i;
		else if (!wPotion && pI->byType == ITEMTYPE_HPUP) wPotion = (WORD)i;
		else if (!wMine) wMine = (WORD)i;
	}

	/* 貸し出しセットと敵のステータステンプレート */
	int nStLoan = g_aStTmpInfo.GetSize(), nStEnemy = nStLoan + 1;
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "貸し出し");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 20; t->dwGold = 3;
		t->waItem[0] = wPotion; t->waItem[1] = wWeapon; t->waItem[2] = wArmor; t->waItem[3] = wPotion;
		t->abyMagic[0] = 1;
		g_aStTmpInfo.Add(t);
		t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "試しの魔物");
		t->byLevel = 3; t->byPW = 5; t->byNpcType = NPCTYPE_MOVERAND; t->wMaxHP = 30; t->dwExp = 999; t->dwGold = 999; t->byDropProbability = 100;
		t->waItem[0] = wMine;
		g_aStTmpInfo.Add(t);
	}

	/* 依頼 20（2 人以上・入口 1・敵 1）。ギルドの人 */
	g_aQuestInfo.RemoveAll();
	{
		PQUESTINFO p = new QUESTINFO;
		ZeroMemory(p, sizeof(*p));
		p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
		p->wID = 20; p->byState = QUESTSTATE_ON; p->byGuild = 0; p->byStepCount = 2;
		strcpy(p->szName, "試しの討伐");
		p->dwGold = 100; p->dwExp = 10;
		p->byChallenge = QUESTCHALLENGE_OTHERWORLD; p->wLoanStTmp = (WORD)nStLoan; p->byMemberMin = 2;
		for (int k = 0; k < MAX_QUESTNPC; k ++) { p->aNpc[k].byStepMax = QUESTNPC_NOSTEP; p->aNpc[k].byNextStep = QUESTNPC_NOSTEP; }
		p->aNpc[0].dwMapID = pMap->dwID; p->aNpc[0].byX = (BYTE)aCell[0].first; p->aNpc[0].byY = (BYTE)aCell[0].second;
		p->aNpc[0].byKind = QUESTNPCKIND_GATE; p->aNpc[0].wGrpID = 4; strcpy(p->aNpc[0].szName, "入口");
		p->aNpc[0].dwDestMapID = pMap->dwID; p->aNpc[0].byDestX = (BYTE)aCell[3].first; p->aNpc[0].byDestY = (BYTE)aCell[3].second;
		p->aNpc[1].dwMapID = pMap->dwID; p->aNpc[1].byX = (BYTE)aCell[2].first; p->aNpc[1].byY = (BYTE)aCell[2].second;
		p->aNpc[1].byKind = QUESTNPCKIND_ENEMY; p->aNpc[1].wGrpID = 5; p->aNpc[1].wStTmp = (WORD)nStEnemy;
		g_aQuestInfo.Add(p);
	}
	PCHARINFO pGuild = new CHARINFO;
	ZeroMemory((PBYTE)pGuild, sizeof(CHARINFO));
	pGuild->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pGuild);
	pGuild->byGameState = GAMESTATE_NPC;
	pGuild->byCharState = CHARSTATE_MOVE;
	pGuild->dwMapID = nMap;
	pGuild->dwX = pGuild->dwXBack = pGuild->dwStartX = (DWORD)aCell[4].first;
	pGuild->dwY = pGuild->dwYBack = pGuild->dwStartY = (DWORD)aCell[4].second;
	strcpy(pGuild->szName, "ギルド受付");
	pGuild->pszNpcSpeech = new char[8];
	strcpy(pGuild->pszNpcSpeech, "@g0");
	pGuild = AddChar(pGuild);
	pGuild->pID = pGuild->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pGuild);
	}
	DWORD dwGuild = pGuild->dwTblID;

	QuestMovePC(pA, nMap, aCell[4].first, aCell[4].second + 1);
	QuestMovePC(pB, nMap, aCell[4].first + 1, aCell[4].second + 1);
	QuestMovePC(pC, nMap, aCell[7].first, aCell[7].second);

	/* 一人では受けられない（2 人以上） */
	QuestSendCmd(pA, QUESTCMD_ACCEPT, 20, dwGuild);
	printf("challenge solo accept=%u\n", pA->wQuestID);

	/* パーティで受けると近くの仲間も受ける（パーティでない人は受けない） */
	QuestSendParty(pA, pB, CMDPARTY_JOIN);
	QuestSendParty(pB, pA, CMDPARTY_JOINOK);
	QuestSendCmd(pA, QUESTCMD_ACCEPT, 20, dwGuild);
	printf("challenge party accept a=%u b=%u c=%u\n", pA->wQuestID, pB->wQuestID, pC->wQuestID);
	/* 後から仲間が一人で受けることはできない */
	QuestSendCmd(pC, QUESTCMD_ACCEPT, 20, dwGuild);
	printf("challenge other accept c=%u\n", pC->wQuestID);

	/* 入口に乗る：二人とも貸し出しの状態で行き先へ。敵が 1 体出る（経験値・お金・落とす物は無し） */
	DWORD dwAX = pA->dwX, dwAY = pA->dwY, dwBX = pB->dwX, dwBY = pB->dwY;
	QuestSyncAll();
	PCHARINFO pGate = QuestFindCopy(pA, 4);
	printf("challenge gate found=%d\n", (int)(pGate != NULL));
	if (pGate == NULL) {
		return;
	}
	/* 入る前にかけた攻撃アップは持ち込まない（効果も HP 枠のアイコンも消える） */
	SetCharInfoTimerStatus(pA, CHARINFOTIMEREVENT_ATUP, 50, 60000);
	RenewATDF(pA->dwTblID);
	int nBuff0 = MockCmdCount(1000, CMD_CHARBUFF);
	/* 入る前に出した「魔法を出し続けるキャラ」も消える */
	DWORD dwChildTbl;
	{
		PCHARINFO pChild = new CHARINFO;
		ZeroMemory((PBYTE)pChild, sizeof(CHARINFO));
		pChild->byNpcType = NPCTYPE_USEMAGICPARENT;
		SetInitCharData(pChild);
		pChild->byGameState = GAMESTATE_NPC;
		pChild->dwMapID = pA->dwMapID;
		pChild->dwXBack = pChild->dwX = pA->dwX;
		pChild->dwYBack = pChild->dwY = pA->dwY;
		pChild->dwParentTblID = pA->dwTblID;
		pChild = AddChar(pChild);
		pChild->pID = pChild->dwTblID + 1;
		dwChildTbl = pChild->dwTblID;
	}
	int nMapInfoA0 = MockCmdCount(1000, CMD_MAPINFO), nMapInfoB0 = MockCmdCount(1001, CMD_MAPINFO);
	QuestSendTalk(pA, pGate->dwTblID);
	/* 同じマップの中で移る時も、マップ情報を送り直して見え方を作り直す（ふつうの世界のキャラが残らないように） */
	printf("challenge start mapinfo a=%d b=%d\n", MockCmdCount(1000, CMD_MAPINFO) - nMapInfoA0, MockCmdCount(1001, CMD_MAPINFO) - nMapInfoB0);
	printf("challenge buff timers=%d charbuff_sent=%d\n", pA->aTimerEvent.GetSize(), (int)(MockCmdCount(1000, CMD_CHARBUFF) > nBuff0));
	printf("challenge child removed=%d\n", (int)(g_aCharInfo[dwChildTbl]->byNpcType != NPCTYPE_USEMAGICPARENT));
	ChallengePrint("start a", pA, wWeapon);
	ChallengePrint("start b", pB, wWeapon);
	printf("challenge start a_dest=%d b_dest=%d armor_equ=%d enemies=%d\n",
		(int)(pA->dwX == (DWORD)aCell[3].first && pA->dwY == (DWORD)aCell[3].second),
		(int)(pB->dwX == (DWORD)aCell[3].first && pB->dwY == (DWORD)aCell[3].second),
		(int)(pA->waEqu[1] < MAX_MYITEM && pA->waItem[pA->waEqu[1]] == wArmor), ChallengeCountEnemy());
	PCHARINFO pEnemy = ChallengeFindEnemy();
	if (pEnemy) {
		printf("challenge enemy type=%u hp=%u exp=%u gold=%u drop=%u\n", pEnemy->byNpcType, pEnemy->wMaxHP, pEnemy->dwExp, pEnemy->dwGold, pEnemy->byDropProbability);
	}

	/* 保存の間だけ本物の状態と入る前の場所になる */
	QuestChallengeSaveSwap(TRUE);
	printf("challenge saving lv=%u gold=%u at_start=%d\n", pA->byLevel, pA->dwGold, (int)(pA->dwX == dwAX && pA->dwY == dwAY));
	QuestChallengeSaveSwap(FALSE);
	printf("challenge saved lv=%u gold=%u at_dest=%d\n", pA->byLevel, pA->dwGold, (int)(pA->dwX == (DWORD)aCell[3].first));

	/* 預かり所には入れられない */
	{
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_LEAVEITEM;
		Para.pID = pA->pID;
		Para.dwTblID = pA->dwTblID;
		Para.byData = 0;
		TestRecv(pA, &Para, sizeof(Para));
		printf("challenge leave blocked item0=%d\n", (int)(pA->waItem[0] == wPotion));
	}

	/* 取り引きの申し込みは断られ、申し込んだ側に「取り引き終了」が返る（返らないと操作が効かないまま） */
	{
		CMDPARA_DATA3 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_TRADE;
		Para.pID = pA->pID;
		Para.dwTblID = pA->dwTblID;
		Para.dwData = pB->dwTblID;
		int nTrade0 = MockCmdCount(1000, CMD_TRADE), nTradeB0 = MockCmdCount(1001, CMD_TRADE);
		TestRecv(pA, &Para, sizeof(Para));
		printf("challenge trade refused a_out=%d b=%d\n", MockCmdCount(1000, CMD_TRADE) - nTrade0, MockCmdCount(1001, CMD_TRADE) - nTradeB0);
	}

	/* 周期処理：まだ何も起きない */
	QuestChallengeProcNow();
	printf("challenge proc a=%d b=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB));

	/* B が倒れる：B だけ中断して元の状態・光に入った A の入る前の場所へ（仲間は全員同じ場所へ戻る）。依頼は受けたまま */
	pB->wHP = 0;
	pB->byCharState = CHARSTATE_DEAD;
	QuestChallengeProcNow();
	ChallengePrint("dead b", pB, wWeapon);
	printf("challenge dead b state=%u at_start=%d quest=%u a_in=%d\n", pB->byCharState, (int)(pB->dwX == dwAX && pB->dwY == dwAY && dwBX != dwAX), pB->wQuestID, (int)QuestInChallenge(pA));

	/* 倒れて出た B は、A がまだ中にいる間は入り直せない（一人で別の組を始めない） */
	{
		QuestSyncAll();
		PCHARINFO pGateB = QuestFindCopy(pB, 4);
		if (pGateB) {
			QuestSendTalk(pB, pGateB->dwTblID);
		}
		printf("challenge reenter gate=%d b_in=%d challenges_enemy=%d\n", (int)(pGateB != NULL), (int)QuestInChallenge(pB), ChallengeCountEnemy());
	}
	/* 討伐チャレンジを別々に受けた人同士は、同じ依頼でも仲間になれない */
	{
		WORD wQB = pB->wQuestID, wQC = pC->wQuestID;
		DWORD dwPB = pB->dwPartyID;
		pB->dwPartyID = 0;
		pB->wQuestID = 20; pC->wQuestID = 20;
		printf("challenge separate conflict=%d\n", (int)QuestPartyConflict(pB, pC));
		pC->wQuestID = 0;
		printf("challenge helper conflict=%d\n", (int)QuestPartyConflict(pB, pC));
		pB->wQuestID = wQB; pC->wQuestID = wQC; pB->dwPartyID = dwPB;
	}

	/* 敵を倒す：A が元に戻り、最後の段・成功の印・入る前の場所へ */
	if (pEnemy) {
		DWORD dwExp0 = aBack[0].dwExp;		/* 本物の経験値（敵は経験値を出さないので変わらない） */
		DeleteChar(pEnemy->dwTblID);
		QuestChallengeProcNow();
		/* すぐには戻さない（知らせてから 3 秒後） */
		printf("challenge clear wait in=%d\n", (int)QuestInChallenge(pA));
		s_dwFakeNow += 2000;
		QuestChallengeProcNow();
		printf("challenge clear wait2 in=%d\n", (int)QuestInChallenge(pA));
		s_dwFakeNow += 1000;
		QuestChallengeProcNow();
		ChallengePrint("clear a", pA, wWeapon);
		printf("challenge clear step=%u flag16=%d at_start=%d exp_same=%d enemies=%d\n", pA->byQuestStep, QuestFlagCheck(pA, 16),
			(int)(pA->dwX == dwAX && pA->dwY == dwAY), (int)(pA->dwExp == dwExp0), ChallengeCountEnemy());
	}

	/* ギルドで渡す：初回は報酬あり */
	QuestMovePC(pA, nMap, aCell[4].first, aCell[4].second + 1);
	{
		DWORD dwGold0 = pA->dwGold;
		QuestSendCmd(pA, QUESTCMD_HANDIN, 0, dwGuild);
		printf("challenge handin1 quest=%u gold+=%d done=%d\n", pA->wQuestID, (int)(pA->dwGold - dwGold0), (int)QuestIsDone(pA, 20));
	}
	/* B はまだ終わっていないので渡せない */
	{
		QuestSendCmd(pB, QUESTCMD_HANDIN, 0, dwGuild);
		printf("challenge handin b quest=%u\n", pB->wQuestID);
	}

	/* もう一度：B を取り消して二人で受け直し、入ってから A が切断 */
	QuestSendCmd(pB, QUESTCMD_CANCEL, 0, dwGuild);
	QuestSendCmd(pA, QUESTCMD_ACCEPT, 20, dwGuild);
	printf("challenge again a=%u b=%u\n", pA->wQuestID, pB->wQuestID);
	QuestSyncAll();
	pGate = QuestFindCopy(pA, 4);
	/* 2 回目は闘技場にする（ふつうの世界の C から見えるが、手は出せない） */
	PQUESTINFO pQ20 = NULL;
	for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) if (g_aQuestInfo[i]->wID == 20) pQ20 = g_aQuestInfo[i];
	BYTE byChallengeBak = pQ20 ? pQ20->byChallenge : 0;
	if (pQ20) pQ20->byChallenge = QUESTCHALLENGE_ARENA;
	int nInfoC0 = MockCmdCount(1002, CMD_CHARINFO) + MockCmdCount(1002, CMD_CHARINFO2), nPosC0 = MockCmdCount(1002, CMD_CHARPOS);
	if (pGate) {
		QuestSendTalk(pA, pGate->dwTblID);
	}
	printf("challenge arena c_got info=%d pos=%d\n", MockCmdCount(1002, CMD_CHARINFO) + MockCmdCount(1002, CMD_CHARINFO2) - nInfoC0, MockCmdCount(1002, CMD_CHARPOS) - nPosC0);
	printf("challenge again in a=%d b=%d enemies=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), ChallengeCountEnemy());
	{
		PCHARINFO pEn = ChallengeFindEnemy();
		printf("challenge arena space_a=%d enemy_vis_c=%d a_vis_c=%d c_vis_a=%d c_hit_enemy=%d a_hit_c=%d\n",
			(int)(pA->dwSpaceID != 0), pEn ? IsCharVisibleTo(pEn, pC) : -1, IsCharVisibleTo(pA, pC), IsCharVisibleTo(pC, pA),
			pEn ? IsInteractable(pC, pEn) : -1, IsInteractable(pA, pC));
	}
	if (pQ20) pQ20->byChallenge = byChallengeBak;
	/* 二組目は入れない（同じ依頼は一度に 1 組） */
	pC->wQuestID = 20;
	QuestMovePC(pC, nMap, aCell[0].first, aCell[0].second + 1);
	{
		/* C 自身の入口（見えるNPC は持ち主ごと） */
		QuestSyncAll();
		PCHARINFO pGateC = QuestFindCopy(pC, 4);
		if (pGateC) QuestSendTalk(pC, pGateC->dwTblID);
		printf("challenge busy c_in=%d gate=%d\n", (int)QuestInChallenge(pC), (int)(pGateC != NULL));
	}
	pC->wQuestID = 0;
	QuestSyncAll();

	/* A が切断：その場で元の状態。場所はマップの表から外れた後（ログイン時）に入る前の場所へ */
	QuestOnDecPlayer(pA);
	printf("challenge logout a in=%d lv=%u\n", (int)QuestInChallenge(pA), pA->byLevel);
	DeleteTblNoPC((int)pA->dwTblID);
	QuestOnLogin(pA);
	AddTblNoPC((int)pA->dwTblID);
	printf("challenge relogin a at_start=%d\n", (int)(pA->dwX == (DWORD)aCell[4].first && pA->dwY == (DWORD)aCell[4].second + 1));

	/* B が部屋の出口（同じマップの中のワープ）を踏む：B も元に戻ってその場に残り、誰もいなくなったので敵も消える */
	{
		EVENTINFO Event;
		EVENTINFO_MOVE Move;
		ZeroMemory(&Event, sizeof(Event));
		ZeroMemory(&Move, sizeof(Move));
		Move.dwX = (DWORD)aCell[5].first;
		Move.dwY = (DWORD)aCell[5].second;
		Event.byType = EVENTTYPE_MOVE;
		Event.pData = &Move;
		EventProc(pB, &Event, 0);
		printf("challenge warp b still_in=%d\n", (int)QuestInChallenge(pB));
		QuestChallengeProcNow();
		ChallengePrint("warp b", pB, wWeapon);
		printf("challenge warp b in=%d at_exit=%d quest=%u enemies=%d\n", (int)QuestInChallenge(pB),
			(int)(pB->dwX == (DWORD)aCell[5].first && pB->dwY == (DWORD)aCell[5].second), pB->wQuestID, ChallengeCountEnemy());
	}

	/* 2 回目のクリア：報酬は無く、依頼は終わる */
	pA->wQuestID = 20; pA->byQuestStep = 1; pA->wQuestFlg = (WORD)(1 << 15);
	QuestMovePC(pA, nMap, aCell[4].first, aCell[4].second + 1);
	{
		DWORD dwGold0 = pA->dwGold, dwExp0 = pA->dwExp;
		QuestSendCmd(pA, QUESTCMD_HANDIN, 0, dwGuild);
		printf("challenge handin2 quest=%u gold+=%d exp_same=%d\n", pA->wQuestID, (int)(pA->dwGold - dwGold0), (int)(pA->dwExp == dwExp0));
	}

	/* 依頼データの新しい欄も保存して読み直せる */
	{
		QUESTINFO Bak = *g_aQuestInfo[0];
		SaveQuestInfo();
		for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
		g_aQuestInfo.RemoveAll();
		LoadQuestInfo();
		printf("challenge info reload same=%d\n", (int)(g_aQuestInfo.GetSize() == 1 && memcmp(g_aQuestInfo[0], &Bak, sizeof(Bak)) == 0));
	}

	/* パーティを解散すると、まだ倒していない討伐チャレンジの依頼は取り消し（光も見えなくなる） */
	printf("challenge before breakup b_quest=%u party=%d\n", pB->wQuestID, (int)(pB->dwPartyID != 0));
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	QuestSyncAll();
	printf("challenge after breakup b_quest=%u b_gate=%d\n", pB->wQuestID, (int)(QuestFindCopy(pB, 4) != NULL));

	/* 片付け */
	for (int i = 0; i < 3; i ++) {
		CHARINFO *pBk = &aBack[i];
		PCHARINFO p = apPC[i];
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
		ZeroMemory(p->abyQuestRewarded, sizeof(p->abyQuestRewarded));
		p->dwPartyID = 0;
		p->byLevel = pBk->byLevel; p->byPW = pBk->byPW; p->wMaxHP = pBk->wMaxHP; p->wHP = pBk->wHP;
		p->wMaxMana = pBk->wMaxMana; p->wMana = pBk->wMana; p->dwGold = pBk->dwGold; p->dwExp = pBk->dwExp;
		p->byCharState = pBk->byCharState; p->byAdminLV = pBk->byAdminLV;
		memcpy(p->waItem, pBk->waItem, sizeof(p->waItem));
		memcpy(p->waEqu, pBk->waEqu, sizeof(p->waEqu));
		QuestMovePC(p, (int)pBk->dwMapID, (int)pBk->dwX, (int)pBk->dwY);
	}
	QuestSyncAll();
	DeleteChar(dwGuild);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nStLoan; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
	g_aQuestInfo.RemoveAll();
	printf("challenge end enemies=%d copies=%d\n", ChallengeCountEnemy(), QuestCountCopies());
}

/* ------------------------------------------------------------------------- */
/* N. 道具欄・預かり所の並べ替え											 */
/* ------------------------------------------------------------------------- */
extern void ItemNameSortKey(LPCSTR pszName, std::vector<unsigned> &aKey);

static void SortSend(PCHARINFO p, BYTE byTarget, BYTE byMode, BYTE byNo1, BYTE byNo2)
{
	CMDPARA_SORTITEM Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_SORTITEM;
	Para.byTarget = byTarget;
	Para.byMode = byMode;
	Para.byNo1 = byNo1;
	Para.byNo2 = byNo2;
	Para.dwTblID = p->dwTblID;
	TestRecv(p, &Para, sizeof(Para));
}

static void SortPrintMy(const char *pszLabel, PCHARINFO p)
{
	printf("%s items:", pszLabel);
	for (int i = 0; i < MAX_MYITEM; i ++) printf(" %u", p->waItem[i]);
	printf(" equ:");
	for (int i = 0; i < 10; i ++) printf(" %u", p->waEqu[i]);
	/* 装備しているアイテムのID（付け替えが正しければ並べ替えても変わらない） */
	printf(" equid:");
	for (int i = 0; i < 10; i ++) {
		if (p->waEqu[i] < MAX_MYITEM) printf(" %u", p->waItem[p->waEqu[i]]); else printf(" -");
	}
	printf("\n");
}

static void SortPrintLeave(const char *pszLabel, PCHARINFO p)
{
	printf("%s leave:", pszLabel);
	if (p->dwLeaveID && p->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
		PLEAVEINFO pL = g_aLeaveInfo[p->dwLeaveID];
		printf(" n=%u", pL->byCount);
		for (int i = 0; i < pL->byCount; i ++) printf(" %u*%u", pL->waItem[i], pL->byItemCount[i]);
	} else {
		printf(" none");
	}
	printf("\n");
}

static void TestSortItem(const std::vector<int> &aLogin)
{
	printf("== N. SortItem ==\n");

	/* 50音キー：全角カナ・ひらがな・半角カナが同じキーになる等 */
	{
		static const char *s_apszA[][2] = {
			{ "アイテム", "あいてむ" }, { "アイテム", "ｱｲﾃﾑ" }, { "ガード", "が\xe3\x83\xbc\xe3\x81\xa9" },
			{ "ｶﾞ", "が" }, { "ｶﾞ", "ガ" }, { "ﾊﾟ", "ぱ" }, { "ﾊﾞ", "ば" }, { "ｳﾞ", "ゔ" }, { "ｳﾞ", "ヴ" },
			{ "ＡＢＣ１", "abc1" }, { "Potion", "potion" }, { "ｱ", "あ" }, { "ｶ", "か" }, { "ｶﾞ", "か" },
			{ "ﾂ", "つ" }, { "ｯ", "っ" }, { "ﾁﾞ", "ぢ" },
		};
		for (size_t i = 0; i < sizeof(s_apszA) / sizeof(s_apszA[0]); i ++) {
			std::vector<unsigned> a, b;
			ItemNameSortKey(s_apszA[i][0], a);
			ItemNameSortKey(s_apszA[i][1], b);
			printf("sortkey [%s] [%s] same=%d len=%d,%d\n", s_apszA[i][0], s_apszA[i][1], (int)(a == b), (int)a.size(), (int)b.size());
		}
		std::vector<unsigned> a;
		ItemNameSortKey("あいう", a);
		printf("sortkey codes:");
		for (size_t i = 0; i < a.size(); i ++) printf(" %X", a[i]);
		printf("\n");
		/* 漢字は読みの頭のかなで並ぶ（JIS 第一水準の順）。濁点は清音の後ろ */
		{
			static const char *s_apszN[] = { "力の種", "布の服", "明かりの旅術書", "樫の剣", "皮の盾", "パン", "ばくだん", "はしご", "かがみ", "ガラス", "薬草", "ひのきの棒" };
			std::vector<std::pair<std::vector<unsigned>, const char *> > aN;
			for (size_t i = 0; i < sizeof(s_apszN) / sizeof(s_apszN[0]); i ++) {
				std::vector<unsigned> k;
				ItemNameSortKey(s_apszN[i], k);
				aN.push_back(std::make_pair(k, s_apszN[i]));
			}
			std::sort(aN.begin(), aN.end());
			printf("sortkey order:");
			for (size_t i = 0; i < aN.size(); i ++) printf(" %s", aN[i].second);
			printf("\n");
		}
		ItemNameSortKey(NULL, a);
		printf("sortkey null len=%d\n", (int)a.size());
		/* メッセージに埋め込む名前の半角 @ は全角にする・新しい名前に半角 @ は使えない */
		printf("msgsafe [%s] namesafe at=%d plain=%d\n", SboTextMsgSafe("ab@400100c@8").c_str(), (int)SboTextIsNameSafe("ab@1"), (int)SboTextIsNameSafe("abc"));
	}
	if (aLogin.size() < 2) {
		return;
	}

	/* 種類の違うアイテムを集める（名前のあるものだけ。ID の小さい順に、種類ごとに 1 つ） */
	static const BYTE s_abyType[] = { ITEMTYPE_WEAPON, ITEMTYPE_HPUP, ITEMTYPE_ARMOR, ITEMTYPE_SEED, ITEMTYPE_EQU_ACC2, ITEMTYPE_SHILD, ITEMTYPE_MAGIC };
	std::vector<WORD> aPick;
	for (size_t t = 0; t < sizeof(s_abyType); t ++) {
		for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
			if (g_aItemInfo[i]->szName[0] == 0 || g_aItemInfo[i]->byType != s_abyType[t]) continue;
			aPick.push_back((WORD)i);
			break;
		}
	}
	printf("sort picked:");
	for (size_t i = 0; i < aPick.size(); i ++) printf(" %u(type%u)", aPick[i], g_aItemInfo[aPick[i]]->byType);
	printf("\n");
	if (aPick.size() < 5) {
		return;
	}

	PCHARINFO p = g_aCharInfo[aLogin[0]];
	PCHARINFO pL = g_aCharInfo[aLogin[1]];
	BYTE byAdminBack = p->byAdminLV, byAdminBackL = pL->byAdminLV;
	WORD awItemBack[MAX_MYITEM], awEquBack[10];
	memcpy(awItemBack, p->waItem, sizeof(awItemBack));
	memcpy(awEquBack, p->waEqu, sizeof(awEquBack));
	p->byAdminLV = ADMINLV_USER;
	pL->byAdminLV = ADMINLV_USER;
	NearNpcOpen(p);		/* 預かり所・店の人が近くにいる時だけ使える */
	NearNpcOpen(pL, "@6");

	/* 手持ち：6 つ（同じものが 2 つ）。1 番目と 4 番目を装備 */
	ZeroMemory(p->waItem, sizeof(p->waItem));
	for (int i = 0; i < 10; i ++) p->waEqu[i] = 0xFFFF;
	p->waItem[0] = aPick[0];
	p->waItem[1] = aPick[1];
	p->waItem[2] = aPick[2];
	p->waItem[3] = aPick[3];
	p->waItem[4] = aPick[4];
	p->waItem[5] = aPick[1];
	p->waEqu[0] = 0;
	p->waEqu[3] = 4;
	SortPrintMy("sort my init", p);

	/* 入れ替え：装備も一緒に動く */
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 0, 4);
	SortPrintMy("sort my swap 0,4", p);
	/* 入れ替え：同じ番号・範囲外（アイテム数 6 以上）・別の対象は動かない */
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 2, 2);
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 1, 6);
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 9, 0);
	SortSend(p, 9, SORTITEMMODE_SWAP, 0, 1);
	SortSend(p, SORTITEMTARGET_MYITEM, 0, 0, 1);
	SortPrintMy("sort my swap invalid", p);
	/* 種類順 */
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_TYPE, 0, 0);
	SortPrintMy("sort my type", p);
	/* 50音順 */
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_NAME, 0, 0);
	SortPrintMy("sort my name", p);
	/* 続けて同じ並べ替え：逆の順（降順）→もう一度で元の順（昇順） */
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_NAME, 0, 0);
	SortPrintMy("sort my name again(desc)", p);
	SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_NAME, 0, 0);
	SortPrintMy("sort my name again(asc)", p);
	/* 他人のキャラは操作できない */
	{
		CMDPARA_SORTITEM Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_SORTITEM;
		Para.byTarget = SORTITEMTARGET_MYITEM;
		Para.byMode = SORTITEMMODE_SWAP;
		Para.byNo1 = 0;
		Para.byNo2 = 1;
		Para.dwTblID = p->dwTblID;
		TestRecv(pL, &Para, sizeof(Para));
		SortPrintMy("sort my other-char", p);
		/* 短いパケットは弾く */
		TestRecv(p, &Para, 3);
		SortPrintMy("sort my short", p);
	}

	/* 取り引き中は変わらない */
	{
		CMDPARA_DATA3 Trade;
		ZeroMemory(&Trade, sizeof(Trade));
		Trade.byCmd = CMD_TRADE;
		Trade.pID = p->pID;
		Trade.dwTblID = p->dwTblID;
		Trade.dwData = pL->dwTblID;
		DWORD dwMapBack = pL->dwMapID, dwXBack = pL->dwX, dwYBack = pL->dwY;
		char szAccBack[sizeof(pL->szAcc)];
		memcpy(szAccBack, pL->szAcc, sizeof(szAccBack));
		pL->dwMapID = p->dwMapID; pL->dwX = p->dwX; pL->dwY = p->dwY;
		if (pL->szAcc[0] == 0) strcpy(pL->szAcc, "sorttest");
		TestRecv(p, &Trade, sizeof(Trade));
		printf("sort trading=%d\n", (int)RecvCmdIsTrading(p->dwTblID));
		SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 2, 3);
		SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_TYPE, 0, 0);
		SortPrintMy("sort my trading", p);
		RecvCmdTradeClear(p->dwTblID);
		RecvCmdTradeClear(pL->dwTblID);
		pL->dwMapID = dwMapBack; pL->dwX = dwXBack; pL->dwY = dwYBack;
		memcpy(pL->szAcc, szAccBack, sizeof(szAccBack));
		printf("sort trading cleared=%d\n", (int)RecvCmdIsTrading(p->dwTblID));
		SortSend(p, SORTITEMTARGET_MYITEM, SORTITEMMODE_SWAP, 2, 3);
		SortPrintMy("sort my after trade", p);
	}

	/* 取り引きのすり替え防止・取り引き中の持ち物コマンド停止・装備中の品を売らない・装備欄のずれ */
	{
		DWORD dwMapBack = pL->dwMapID, dwXBack = pL->dwX, dwYBack = pL->dwY;
		char szAccBack[sizeof(pL->szAcc)];
		memcpy(szAccBack, pL->szAcc, sizeof(szAccBack));
		pL->dwMapID = p->dwMapID; pL->dwX = p->dwX; pL->dwY = p->dwY;
		if (pL->szAcc[0] == 0) strcpy(pL->szAcc, "sorttest");
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int i = 0; i < 10; i ++) p->waEqu[i] = 0xFFFF;
		p->waItem[0] = aPick[0];
		p->waItem[1] = aPick[1];
		p->waItem[2] = aPick[2];
		p->waItem[3] = aPick[3];
		p->waEqu[0] = 2;
		p->waEqu[9] = 3;

		/* 装備中の品は売れない。装備していない品を売ると、後ろの装備の欄だけ 1 つ前へ */
		CMDPARA_DATA5 Drop;
		ZeroMemory(&Drop, sizeof(Drop));
		Drop.byCmd = CMD_DROPITEM;
		Drop.pID = p->pID;
		Drop.dwTblID = p->dwTblID;
		Drop.byData = 2;
		Drop.dwData1 = 1;
		Drop.dwData2 = aPick[2];
		TestRecv(p, &Drop, sizeof(Drop));
		SortPrintMy("drop equipped", p);
		Drop.byData = 0;
		Drop.dwData2 = aPick[0];
		TestRecv(p, &Drop, sizeof(Drop));
		SortPrintMy("drop before equipped", p);

		/* 取り引き：出す位置の品が入れ替わったら成立しない */
		CMDPARA_DATA3 Trade;
		ZeroMemory(&Trade, sizeof(Trade));
		Trade.byCmd = CMD_TRADE;
		Trade.pID = p->pID;
		Trade.dwTblID = p->dwTblID;
		Trade.dwData = pL->dwTblID;
		TestRecv(p, &Trade, sizeof(Trade));
		CMDPARA_TRADEINFO Info;
		ZeroMemory(&Info, sizeof(Info));
		Info.byCmd = CMD_TRADEINFO;
		Info.byMode = 1;
		Info.dwTblID = pL->dwTblID;
		for (int i = 0; i < MAX_MYITEM; i ++) Info.waItem[i] = 0xFFFF;
		Info.waItem[0] = 0;
		Info.bySum = ItemSum(p);
		TestRecv(p, &Info, sizeof(Info));
		Info.dwTblID = p->dwTblID;
		Info.waItem[0] = 0xFFFF;
		TestRecv(pL, &Info, sizeof(Info));
		/* 取り引き中は持ち物を動かすコマンドを受け付けない */
		Drop.byData = 0;
		Drop.dwData2 = p->waItem[0];
		TestRecv(p, &Drop, sizeof(Drop));
		SortPrintMy("drop while trading", p);
		/* すり替え（改造クライアントが別の経路で持ち物を動かしたことにする） */
		WORD wSwap = p->waItem[0];
		p->waItem[0] = aPick[4];
		CMDPARA_TRADEPROC Proc;
		ZeroMemory(&Proc, sizeof(Proc));
		Proc.byCmd = CMD_TRADEPROC;
		Proc.bySum1 = ItemSum(p);
		Proc.bySum2 = ItemSum(pL);
		Proc.dwTblID1 = p->dwTblID;
		Proc.dwTblID2 = pL->dwTblID;
		for (int i = 0; i < MAX_MYITEM; i ++) { Proc.waItem1[i] = 0xFFFF; Proc.waItem2[i] = 0xFFFF; }
		Proc.waItem1[0] = 0;
		TestRecv(p, &Proc, sizeof(Proc));
		printf("trade swap trading=%d first=%u (was %u)\n", (int)RecvCmdIsTrading(p->dwTblID), p->waItem[0], wSwap);
		RecvCmdTradeClear(p->dwTblID);
		RecvCmdTradeClear(pL->dwTblID);
		pL->dwMapID = dwMapBack; pL->dwX = dwXBack; pL->dwY = dwYBack;
		memcpy(pL->szAcc, szAccBack, sizeof(szAccBack));
	}

	/* 預かり所：口座が無い人は何も起きない・あれば 個数ごと動く */
	{
		DWORD dwLeaveBack = pL->dwLeaveID;
		LEAVEINFO LeaveBack;
		ZeroMemory(&LeaveBack, sizeof(LeaveBack));
		if (dwLeaveBack && dwLeaveBack < (DWORD)g_aLeaveInfo.GetSize()) LeaveBack = *g_aLeaveInfo[dwLeaveBack];
		if (dwLeaveBack) {
			ZeroMemory(g_aLeaveInfo[dwLeaveBack], sizeof(LEAVEINFO));
			g_aLeaveInfo[dwLeaveBack]->byUse = 1;
		}
		/* 口座がまだ無い時（dwLeaveID==0）の振る舞いを先に見る */
		if (dwLeaveBack == 0) {
			SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_NAME, 0, 0);
			SortPrintLeave("sort leave noaccount", pL);
		}
		AddLeaveItem(pL, aPick[4]);
		AddLeaveItem(pL, aPick[1]);
		AddLeaveItem(pL, aPick[0]);
		AddLeaveItem(pL, aPick[1]);
		AddLeaveItem(pL, aPick[2]);
		SortPrintLeave("sort leave init", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_SWAP, 0, 3);
		SortPrintLeave("sort leave swap 0,3", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_SWAP, 1, 1);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_SWAP, 0, 5);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_SWAP, 4, 99);
		SortPrintLeave("sort leave swap invalid", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_NAME, 0, 0);
		SortPrintLeave("sort leave name", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_TYPE, 0, 0);
		SortPrintLeave("sort leave type", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_TYPE, 0, 0);
		SortPrintLeave("sort leave type again(desc)", pL);
		SortSend(pL, SORTITEMTARGET_LEAVE, SORTITEMMODE_TYPE, 0, 0);
		SortPrintLeave("sort leave type again(asc)", pL);

		/* 片付け */
		if (dwLeaveBack) {
			*g_aLeaveInfo[dwLeaveBack] = LeaveBack;
		} else if (pL->dwLeaveID && pL->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
			ZeroMemory(g_aLeaveInfo[pL->dwLeaveID], sizeof(LEAVEINFO));
			pL->dwLeaveID = 0;
		}
	}

	/* 片付け */
	memcpy(p->waItem, awItemBack, sizeof(awItemBack));
	memcpy(p->waEqu, awEquBack, sizeof(awEquBack));
	p->byAdminLV = byAdminBack;
	pL->byAdminLV = byAdminBackL;
	NearNpcClose();
}

/* ------------------------------------------------------------------------- */
/* P. 預かり所の品を引き取らずに売る・捨てる（CMD_LEAVEITEMDISPOSE）			 */
/*    売値は店と同じ 価値/4（最低1）。値打ちの無い品は売れない・捨てられない	 */
/*    品はどちらもだめ・所持金 999999 を超える分は売らない・番号や品の違い、	 */
/*    個数の多すぎ・他人・短いパケットは弾く									 */
/* ------------------------------------------------------------------------- */
static void DisposeSend(PCHARINFO pFrom, PCHARINFO p, BYTE byMode, BYTE byNo, BYTE byCount, WORD wItem, DWORD dwSize = sizeof(CMDPARA_LEAVEITEMDISPOSE))
{
	CMDPARA_LEAVEITEMDISPOSE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_LEAVEITEMDISPOSE;
	Para.byMode = byMode;
	Para.byNo = byNo;
	Para.byCount = byCount;
	Para.wItemID = wItem;
	Para.dwTblID = p->dwTblID;
	TestRecv(pFrom, &Para, dwSize);
}

static void TestLeaveDispose(const std::vector<int> &aLogin)
{
	printf("== P. LeaveDispose ==\n");
	if (aLogin.empty()) {
		return;
	}
	/* 値打ちのある品 2 つ・値打ちの無い品・捨てられない品を探す */
	WORD wSell1 = 0, wSell2 = 0, wNoGold = 0, wNoDrop = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem->szName[0] == 0) continue;
		if (pItem->byFlg & ITEMFLG_DROP) {
			if (wNoDrop == 0) wNoDrop = (WORD)i;
			continue;
		}
		if (pItem->dwGold == 0) {
			if (wNoGold == 0) wNoGold = (WORD)i;
		} else if (wSell1 == 0) {
			wSell1 = (WORD)i;
		} else if ((wSell2 == 0) && (pItem->dwGold >= 8)) {
			wSell2 = (WORD)i;
		}
	}
	printf("dispose picked sell1=%u(%u) sell2=%u(%u) nogold=%u nodrop=%u\n", wSell1, wSell1 ? g_aItemInfo[wSell1]->dwGold : 0,
			wSell2, wSell2 ? g_aItemInfo[wSell2]->dwGold : 0, wNoGold, wNoDrop);
	if ((wSell1 == 0) || (wSell2 == 0)) {
		return;
	}

	PCHARINFO p = g_aCharInfo[aLogin[0]];
	PCHARINFO pOther = (aLogin.size() >= 2) ? g_aCharInfo[aLogin[1]] : NULL;
	BYTE byAdminBack = p->byAdminLV;
	DWORD dwGoldBack = p->dwGold;
	DWORD dwLeaveBack = p->dwLeaveID;
	NearNpcOpen(p);		/* 預かり所の人が近くにいる時だけ使える */
	LEAVEINFO LeaveBack;
	ZeroMemory(&LeaveBack, sizeof(LeaveBack));
	if (dwLeaveBack && dwLeaveBack < (DWORD)g_aLeaveInfo.GetSize()) LeaveBack = *g_aLeaveInfo[dwLeaveBack];
	if (dwLeaveBack) {
		ZeroMemory(g_aLeaveInfo[dwLeaveBack], sizeof(LEAVEINFO));
		g_aLeaveInfo[dwLeaveBack]->byUse = 1;
	}
	p->byAdminLV = ADMINLV_USER;
	p->dwGold = 100;

	/* 口座が無い時は何も起きない */
	if (dwLeaveBack == 0) {
		DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 1, wSell1);
		SortPrintLeave("dispose noaccount", p);
	}
	for (int i = 0; i < 5; i ++) AddLeaveItem(p, wSell2);
	AddLeaveItem(p, wSell1);
	if (wNoGold) AddLeaveItem(p, wNoGold);
	if (wNoDrop) AddLeaveItem(p, wNoDrop);
	SortPrintLeave("dispose init", p);
	printf("dispose init gold=%u\n", p->dwGold);

	/* 2 個売る：価値/4 × 2 */
	DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 2, wSell2);
	SortPrintLeave("dispose sell 2", p);
	printf("dispose sell 2 gold=%u\n", p->dwGold);
	/* 弾くもの：品が違う・個数が多すぎ・0個・番号が範囲外・方法が違う・他人・短いパケット */
	DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 1, wSell1);
	DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 4, wSell2);
	DisposeSend(p, p, LEAVEDISPOSE_DROP, 0, 0, wSell2);
	DisposeSend(p, p, LEAVEDISPOSE_DROP, 50, 1, wSell2);
	DisposeSend(p, p, 7, 0, 1, wSell2);
	if (pOther) {
		BYTE byAdminBackO = pOther->byAdminLV;
		pOther->byAdminLV = ADMINLV_USER;
		DisposeSend(pOther, p, LEAVEDISPOSE_DROP, 0, 1, wSell2);
		pOther->byAdminLV = byAdminBackO;
	}
	DisposeSend(p, p, LEAVEDISPOSE_DROP, 0, 1, wSell2, 5);
	SortPrintLeave("dispose invalid", p);
	printf("dispose invalid gold=%u\n", p->dwGold);

	/* 値打ちの無い品は売れないが捨てられる。捨てられない品はどちらもだめ */
	if (wNoGold) {
		DisposeSend(p, p, LEAVEDISPOSE_SELL, 2, 1, wNoGold);
		SortPrintLeave("dispose nogold sell", p);
		DisposeSend(p, p, LEAVEDISPOSE_DROP, 2, 1, wNoGold);
		SortPrintLeave("dispose nogold drop", p);
	}
	if (wNoDrop) {
		int nNo = p->dwLeaveID ? g_aLeaveInfo[p->dwLeaveID]->byCount - 1 : 0;
		DisposeSend(p, p, LEAVEDISPOSE_SELL, (BYTE)nNo, 1, wNoDrop);
		DisposeSend(p, p, LEAVEDISPOSE_DROP, (BYTE)nNo, 1, wNoDrop);
		SortPrintLeave("dispose nodrop", p);
	}

	/* 所持金の上限：1 個分しか入らない時は 1 個だけ売る */
	DWORD dwSell2 = g_aItemInfo[wSell2]->dwGold / 4;
	if (dwSell2 == 0) dwSell2 = 1;
	p->dwGold = 999999 - dwSell2 - 1;
	DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 3, wSell2);
	SortPrintLeave("dispose sell cap", p);
	printf("dispose sell cap gold=%u\n", p->dwGold);
	/* もう 1 個も入らない時は売らない */
	DisposeSend(p, p, LEAVEDISPOSE_SELL, 0, 1, wSell2);
	SortPrintLeave("dispose sell full", p);
	printf("dispose sell full gold=%u\n", p->dwGold);

	/* 残りを全部捨てる：枠が詰まる */
	DisposeSend(p, p, LEAVEDISPOSE_DROP, 0, 2, wSell2);
	SortPrintLeave("dispose drop all", p);
	printf("dispose drop all gold=%u\n", p->dwGold);

	/* 片付け */
	if (dwLeaveBack) {
		*g_aLeaveInfo[dwLeaveBack] = LeaveBack;
	} else if (p->dwLeaveID && p->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
		ZeroMemory(g_aLeaveInfo[p->dwLeaveID], sizeof(LEAVEINFO));
		p->dwLeaveID = 0;
	}
	p->dwGold = dwGoldBack;
	p->byAdminLV = byAdminBack;
	NearNpcClose();
}

/* ------------------------------------------------------------------------- */
/* AN. 店・売る・預かり所・転職・着せ替え・名前の色が、公式クライアントと同じ場面だけ通る	 */
/*    店の人（"@5NN"）・預かり所の人（"@6"）・転職所の人（"@a"）が近くにいる時だけ	 */
/*    着せ替えは男女(2・3)か、着せ替えの人（"@"）の画像だけ。名前の色は 7 色だけ	 */
/*    価値の無い品・捨てられない品は売れない・dwData2 が 0 の捨て方は受けない		 */
/* ------------------------------------------------------------------------- */
static void NpcNearSendDrop(PCHARINFO p, BYTE byNo, DWORD dwSell, DWORD dwItem)
{
	CMDPARA_DATA5 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_DROPITEM;
	Para.byData = byNo;
	Para.pID = p->pID;
	Para.dwTblID = p->dwTblID;
	Para.dwData1 = dwSell;
	Para.dwData2 = dwItem;
	TestRecv(p, &Para, sizeof(Para));
}

static void TestNpcNear(const std::vector<int> &aLogin)
{
	printf("== AN. NpcNear ==\n");
	if (aLogin.empty()) {
		return;
	}
	/* 値打ちのある品・値打ちの無い品・捨てられない品・買う品（安い）を探す */
	WORD wSell = 0, wNoGold = 0, wNoDrop = 0, wBuy = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem->szName[0] == 0 || IsImportantItem((WORD)i)) continue;
		if (pItem->byFlg & ITEMFLG_DROP) {
			if (wNoDrop == 0) wNoDrop = (WORD)i;
			continue;
		}
		if (pItem->dwGold == 0) {
			if (wNoGold == 0) wNoGold = (WORD)i;
		} else if (wSell == 0 && pItem->dwGold >= 8) {
			wSell = (WORD)i;
		} else if (wBuy == 0 && pItem->dwGold < 1000) {
			wBuy = (WORD)i;
		}
	}
	if (wSell == 0 || wNoGold == 0 || wNoDrop == 0 || wBuy == 0) {
		printf("npcnear skipped\n");
		return;
	}

	PCHARINFO p = g_aCharInfo[aLogin[0]];
	WORD awItemBack[MAX_MYITEM], awEquBack[10];
	memcpy(awItemBack, p->waItem, sizeof(awItemBack));
	memcpy(awEquBack, p->waEqu, sizeof(awEquBack));
	BYTE byAdminBack = p->byAdminLV, byJobBack = p->byJOB;
	DWORD dwGoldBack = p->dwGold, dwLeaveBack = p->dwLeaveID;
	WORD wGrpBack = p->wGrpID;
	COLORREF clNameBack = p->clName, clSpeechBack = p->clSpeech;
	LEAVEINFO LeaveBack;
	ZeroMemory(&LeaveBack, sizeof(LeaveBack));
	if (dwLeaveBack && dwLeaveBack < (DWORD)g_aLeaveInfo.GetSize()) LeaveBack = *g_aLeaveInfo[dwLeaveBack];
	p->byAdminLV = ADMINLV_USER;
	ZeroMemory(p->waItem, sizeof(p->waItem));
	for (int i = 0; i < 10; i ++) p->waEqu[i] = 0xFFFF;
	/* 実データの店・預かり所などの人が近くにいると結果が変わるので、この間だけ台詞を外しておく */
	std::vector<std::pair<PCHARINFO, char *> > aSpeechBack;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO pNpc = g_aCharInfo[i];
		if (pNpc != p && pNpc->szAcc[0] == 0 && pNpc->pszNpcSpeech != NULL && pNpc->dwMapID == p->dwMapID) {
			aSpeechBack.push_back(std::make_pair(pNpc, pNpc->pszNpcSpeech));
			pNpc->pszNpcSpeech = NULL;
		}
	}

	/* 店：店の人（"@500"）が近くにいる時だけ買える。違う番号の店・遠い人ではだめ */
	PMAPINFO pMap = g_aMapInfo[p->dwMapID];
	PSHOPINFO pShopBack = pMap->pShopInfo;
	BYTE byCountShopBack = pMap->byCountShop;
	pMap->pShopInfo = new SHOPINFO[1];
	ZeroMemory(pMap->pShopInfo, sizeof(SHOPINFO));
	pMap->byCountShop = 1;
	pMap->pShopInfo[0].byCountItem = 1;
	pMap->pShopInfo[0].awItem[0] = wBuy;
	CMDPARA_DATA5 Buy;
	ZeroMemory(&Buy, sizeof(Buy));
	Buy.byCmd = CMD_SHOPBUY;
	Buy.pID = p->pID;
	Buy.dwTblID = p->dwTblID;
	Buy.dwData1 = 0;
	Buy.dwData2 = 0;
	p->dwGold = 5000;
	TestRecv(p, &Buy, sizeof(Buy));
	printf("npcnear buy none item=%u gold=%u\n", p->waItem[0], p->dwGold);
	NearNpcOpen(p, "@501");
	TestRecv(p, &Buy, sizeof(Buy));
	printf("npcnear buy othershop item=%u gold=%u\n", p->waItem[0], p->dwGold);
	NearNpcClose();
	NearNpcOpen(p, "@500");
	TestRecv(p, &Buy, sizeof(Buy));
	printf("npcnear buy ok item=%u gold=%u (price %u)\n", p->waItem[0], p->dwGold, g_aItemInfo[wBuy]->dwGold);
	NearNpcClose();
	{
		/* 4 マス離れた人は遠い・3 マスなら近い */
		ZeroMemory(p->waItem, sizeof(p->waItem));
		p->dwGold = 5000;
		DWORD dwXBack = p->dwX;
		p->dwX += 4;
		NearNpcOpen(p, "@500");
		p->dwX = dwXBack;
		TestRecv(p, &Buy, sizeof(Buy));
		printf("npcnear buy far item=%u\n", p->waItem[0]);
		NearNpcClose();
		p->dwX += 3;
		NearNpcOpen(p, "@500");
		p->dwX = dwXBack;
		TestRecv(p, &Buy, sizeof(Buy));
		printf("npcnear buy 3cells item=%u\n", p->waItem[0]);
		NearNpcClose();
	}
	delete [] pMap->pShopInfo;
	pMap->pShopInfo = pShopBack;
	pMap->byCountShop = byCountShopBack;

	/* 売る：店の人が近くにいる時だけ。価値の無い品・捨てられない品・dwData2 が 0 は受けない */
	ZeroMemory(p->waItem, sizeof(p->waItem));
	p->waItem[0] = wSell;
	p->waItem[1] = wNoGold;
	p->waItem[2] = wNoDrop;
	p->dwGold = 0;
	NpcNearSendDrop(p, 0, 1, wSell);
	printf("npcnear sell none item0=%u gold=%u\n", p->waItem[0], p->dwGold);
	NearNpcOpen(p, "@700");
	NpcNearSendDrop(p, 1, 1, wNoGold);
	printf("npcnear sell nogold item1=%u gold=%u\n", p->waItem[1], p->dwGold);
	NpcNearSendDrop(p, 2, 1, wNoDrop);
	printf("npcnear sell nodrop item2=%u gold=%u\n", p->waItem[2], p->dwGold);
	NpcNearSendDrop(p, 0, 0, 0);
	printf("npcnear drop zero-id item0=%u\n", p->waItem[0]);
	NpcNearSendDrop(p, 2, 0, wNoDrop);
	printf("npcnear drop nodrop item2=%u\n", p->waItem[2]);
	NpcNearSendDrop(p, 1, 0, wNoGold);
	printf("npcnear drop nogold item1=%u\n", p->waItem[1]);
	NpcNearSendDrop(p, 0, 1, wSell);
	printf("npcnear sell ok item0=%u gold=%u (expect %u)\n", p->waItem[0], p->dwGold, g_aItemInfo[wSell]->dwGold / 4);
	NearNpcClose();
	/* 捨てるのは店の人がいなくてもよい */
	p->waItem[0] = wSell;
	NpcNearSendDrop(p, 0, 0, wSell);
	printf("npcnear drop free item0=%u\n", p->waItem[0]);

	/* 預かり所：預かり所の人（"@6"）が近くにいる時だけ */
	ZeroMemory(p->waItem, sizeof(p->waItem));
	p->waItem[0] = wSell;
	CMDPARA_DATA1 Leave;
	ZeroMemory(&Leave, sizeof(Leave));
	Leave.byCmd = CMD_LEAVEITEM;
	Leave.pID = p->pID;
	Leave.dwTblID = p->dwTblID;
	Leave.byData = 0;
	TestRecv(p, &Leave, sizeof(Leave));
	printf("npcnear leave none item0=%u\n", p->waItem[0]);
	NearNpcOpen(p, "@500");
	TestRecv(p, &Leave, sizeof(Leave));
	printf("npcnear leave shopman item0=%u\n", p->waItem[0]);
	NearNpcClose();
	NearNpcOpen(p, "@6");
	TestRecv(p, &Leave, sizeof(Leave));
	printf("npcnear leave ok item0=%u\n", p->waItem[0]);
	NearNpcClose();
	/* 引き出し・お金の預け引き出しも同じ */
	CMDPARA_DATA3 Get;
	ZeroMemory(&Get, sizeof(Get));
	Get.byCmd = CMD_LEAVEITEMGET;
	Get.pID = p->pID;
	Get.dwTblID = p->dwTblID;
	Get.dwData = 0;
	TestRecv(p, &Get, sizeof(Get));
	printf("npcnear get none item0=%u\n", p->waItem[0]);
	CMDPARA_DATA3 Gold;
	ZeroMemory(&Gold, sizeof(Gold));
	Gold.byCmd = CMD_LEAVEGOLD;
	Gold.pID = p->pID;
	Gold.dwTblID = p->dwTblID;
	Gold.dwData = 10;
	p->dwGold = 100;
	TestRecv(p, &Gold, sizeof(Gold));
	printf("npcnear goldleave none gold=%u\n", p->dwGold);
	NearNpcOpen(p, "@6");
	TestRecv(p, &Gold, sizeof(Gold));
	printf("npcnear goldleave ok gold=%u\n", p->dwGold);
	NearNpcClose();
	TestRecv(p, &Get, sizeof(Get));
	printf("npcnear get none2 item0=%u\n", p->waItem[0]);
	NearNpcOpen(p, "@6");
	TestRecv(p, &Get, sizeof(Get));
	printf("npcnear get ok item0=%u\n", p->waItem[0]);
	NearNpcClose();

	/* 転職：転職所の人（"@a"）が近くにいる時だけ */
	{
		int nJobNew = -1;
		for (int i = 0; i < 16; i ++) {
			if (i != p->byJOB && g_JobInfo.GetPtr(i) != NULL) { nJobNew = i; break; }
		}
		if (nJobNew >= 0) {
			CMDPARA_DATA1 Job;
			ZeroMemory(&Job, sizeof(Job));
			Job.byCmd = CMD_JOBCHG;
			Job.pID = p->pID;
			Job.dwTblID = p->dwTblID;
			Job.byData = (BYTE)nJobNew;
			BYTE byJobOld = p->byJOB;
			TestRecv(p, &Job, sizeof(Job));
			printf("npcnear job none changed=%d\n", (int)(p->byJOB != byJobOld));
			NearNpcOpen(p, "@6");
			TestRecv(p, &Job, sizeof(Job));
			printf("npcnear job wrongnpc changed=%d\n", (int)(p->byJOB != byJobOld));
			NearNpcClose();
			NearNpcOpen(p, "@a");
			TestRecv(p, &Job, sizeof(Job));
			printf("npcnear job ok changed=%d\n", (int)(p->byJOB == nJobNew));
			NearNpcClose();
		}
	}

	/* 着せ替え：男女(2・3)か、着せ替えの人（台詞が "@"）の画像だけ */
	{
		CMDPARA_DATA3 Grp;
		ZeroMemory(&Grp, sizeof(Grp));
		Grp.byCmd = CMD_CHGCHARGRP;
		Grp.pID = p->pID;
		Grp.dwTblID = p->dwTblID;
		p->wGrpID = 5;
		Grp.dwData = 16;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp admin16 grp=%u\n", p->wGrpID);
		Grp.dwData = 70000;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp huge grp=%u\n", p->wGrpID);
		Grp.dwData = 3;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp female grp=%u\n", p->wGrpID);
		NearNpcOpen(p, "@", 40);
		Grp.dwData = 40;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp dressnpc grp=%u\n", p->wGrpID);
		Grp.dwData = 41;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp otherimage grp=%u\n", p->wGrpID);
		NearNpcClose();
		Grp.dwData = 40;
		p->wGrpID = 2;
		TestRecv(p, &Grp, sizeof(Grp));
		printf("npcnear grp nonpc grp=%u\n", p->wGrpID);
	}

	/* 名前の色：色の表の 7 色（名前と発言が同じ）だけ */
	{
		CMDPARA_NAMECOLOR Col;
		ZeroMemory(&Col, sizeof(Col));
		Col.byCmd = CMD_NAMECOLOR;
		Col.pID = p->pID;
		Col.dwTblID = p->dwTblID;
		p->clName = p->clSpeech = RGB(255, 255, 255);
		Col.clName = Col.clSpeech = RGB(1, 2, 3);
		TestRecv(p, &Col, sizeof(Col));
		printf("npcnear color random=%u\n", (unsigned)p->clName);
		Col.clName = Col.clSpeech = RGB(255, 150, 150);
		TestRecv(p, &Col, sizeof(Col));
		printf("npcnear color admin=%u\n", (unsigned)p->clName);
		Col.clName = RGB(255, 200, 200);
		Col.clSpeech = RGB(255, 255, 200);
		TestRecv(p, &Col, sizeof(Col));
		printf("npcnear color mismatch=%u\n", (unsigned)p->clName);
		Col.clName = Col.clSpeech = RGB(200, 255, 255);
		TestRecv(p, &Col, sizeof(Col));
		printf("npcnear color ok=%u\n", (unsigned)p->clName);
	}

	/* 片付け */
	for (size_t i = 0; i < aSpeechBack.size(); i ++) aSpeechBack[i].first->pszNpcSpeech = aSpeechBack[i].second;
	if (dwLeaveBack) {
		*g_aLeaveInfo[dwLeaveBack] = LeaveBack;
	} else if (p->dwLeaveID && p->dwLeaveID < (DWORD)g_aLeaveInfo.GetSize()) {
		ZeroMemory(g_aLeaveInfo[p->dwLeaveID], sizeof(LEAVEINFO));
		p->dwLeaveID = 0;
	}
	memcpy(p->waItem, awItemBack, sizeof(awItemBack));
	memcpy(p->waEqu, awEquBack, sizeof(awEquBack));
	p->byAdminLV = byAdminBack;
	p->byJOB = byJobBack;
	p->dwGold = dwGoldBack;
	p->wGrpID = wGrpBack;
	p->clName = clNameBack;
	p->clSpeech = clSpeechBack;
}

/* ------------------------------------------------------------------------- */
/* AA. 取引所（委託販売。CMD_MARKET*）										 */
/*    受付の近くでだけ使える・出品（装備中・取引不可・依頼の品・値段の範囲・5 件）*/
/*    買う（自分の品・値段違い・所持金・手持ちいっぱい）・手数料 5%（切り捨て）・	 */
/*    売上の受け取り（上限）・期限切れ・引き取り・保存して読み直し			 */
/* ------------------------------------------------------------------------- */
extern BOOL SaveMarket(void);
extern BOOL LoadMarket(void);
EM_JS(char *, MockLastBody, (int nConn, int nCmd), {
	return stringToNewUTF8(Module["sboNet"].lastBody(nConn, nCmd));
});

/* 最後に届いた取引所のパケットを取り出す（届いていなければ FALSE。pOut は大きいほうの構造体の大きさ） */
static BOOL MarketLast(int nConn, BYTE byCmd, std::vector<BYTE> &aOut)
{
	char *psz = MockLastBody(nConn, byCmd);
	size_t n = strlen(psz) / 2;
	aOut.assign(n, 0);
	for (size_t i = 0; i < n; i ++) {
		unsigned int u = 0;
		sscanf(psz + i * 2, "%2x", &u);
		aOut[i] = (BYTE)u;
	}
	free(psz);
	return n > 0;
}

/* 取引所へ 1 つ頼んで結果（MARKETRES_*。届かなければ -1）を返す。届いたパケットは Last に入る */
static CMDPARA_MARKET s_MarketLast;
static int MarketAsk(PCHARINFO p, int nConn, BYTE byCmd, BYTE byType, BYTE byNo, WORD wItem, DWORD dwID, DWORD dwPrice, DWORD dwSize = sizeof(CMDPARA_MARKET))
{
	CMDPARA_MARKET Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = byCmd;
	Para.byType = byType;
	Para.byNo = byNo;
	Para.wItemID = wItem;
	Para.dwTblID = p->dwTblID;
	Para.dwID = dwID;
	Para.dwPrice = dwPrice;
	/* 前の結果と見分けるため、送る前に届いた数を覚える */
	int nBefore = MockCmdCount(nConn, byCmd);
	TestRecv(p, &Para, dwSize);
	if (MockCmdCount(nConn, byCmd) == nBefore) {
		return -1;
	}
	std::vector<BYTE> a;
	if (!MarketLast(nConn, byCmd, a) || a.size() < sizeof(CMDPARA_MARKET)) {
		return -1;
	}
	memcpy(&s_MarketLast, &a[0], sizeof(s_MarketLast));
	return s_MarketLast.byResult;
}

/* 一覧を頼む。届いた件数（届かなければ -1）。aEntry に中身。pdwSale に売上。pnSize に届いた大きさ */
static int MarketAskList(PCHARINFO p, int nConn, BYTE byType, std::vector<MARKETENTRY> &aEntry, DWORD *pdwSale, size_t *pnSize)
{
	CMDPARA_MARKET Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_MARKETLIST;
	Para.byType = byType;
	Para.dwTblID = p->dwTblID;
	int nBefore = MockCmdCount(nConn, CMD_MARKETLIST);
	TestRecv(p, &Para, sizeof(Para));
	aEntry.clear();
	if (MockCmdCount(nConn, CMD_MARKETLIST) == nBefore) {
		return -1;
	}
	std::vector<BYTE> a;
	if (!MarketLast(nConn, CMD_MARKETLIST, a) || a.size() < offsetof(CMDPARA_MARKETLIST, aEntry)) {
		return -1;
	}
	std::vector<BYTE> aFull(sizeof(CMDPARA_MARKETLIST), 0);
	memcpy(&aFull[0], &a[0], a.size() < aFull.size() ? a.size() : aFull.size());
	PCMDPARA_MARKETLIST pList = (PCMDPARA_MARKETLIST)&aFull[0];
	for (int i = 0; i < pList->byCount; i ++) {
		aEntry.push_back(pList->aEntry[i]);
	}
	if (pdwSale) *pdwSale = pList->dwSale;
	if (pnSize) *pnSize = a.size();
	return pList->byCount;
}

static void TestMarket(const std::vector<int> &aLogin)
{
	printf("== AA. Market ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]];
	int nConnA = 1000, nConnB = 1001;
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];

	/* 受付にする NPC（今ある NPC の台詞を一時的に "@b" にする。キャラは増やさない）と、その近くの空きマス 2 つ */
	PCHARINFO pNpc = NULL;
	std::vector<std::pair<int, int> > aCell;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pNpc; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType == 0 || p->szAcc[0] != 0 || p->dwMapID != (DWORD)nMap || IsQuestNpc(p) || p->dwSpaceID != 0) continue;
		aCell.clear();
		for (int dy = -2; dy <= 2 && aCell.size() < 2; dy ++) {
			for (int dx = -2; dx <= 2 && aCell.size() < 2; dx ++) {
				int x = (int)p->dwX + dx, y = (int)p->dwY + dy;
				if (dx == 0 && dy == 0) continue;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
		if (aCell.size() == 2) pNpc = p;
	}
	printf("market npc=%d\n", pNpc ? 1 : 0);
	if (!pNpc) {
		return;
	}

	/* 品：値打ちのある品を 8 個（取引できる・依頼の品でない）。取引不可の品・依頼の品・値打ちの無い品も探す */
	std::vector<WORD> aItem;
	WORD wTrade = 0, wQuestItem = 0, wNoGold = 0;
	for (int i = 3; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem->szName[0] == 0) continue;
		if (pItem->byFlg & ITEMFLG_TRADE) {
			if (!wTrade) wTrade = (WORD)i;
		} else if (QuestIsQuestItem((WORD)i)) {
			if (!wQuestItem) wQuestItem = (WORD)i;
		} else if (pItem->dwGold == 0) {
			if (!wNoGold) wNoGold = (WORD)i;
		} else if (pItem->dwGold >= 40 && aItem.size() < 8) {
			aItem.push_back((WORD)i);
		}
	}
	printf("market picked items=%d trade=%d quest=%d nogold=%d\n", (int)aItem.size(), wTrade ? 1 : 0, wQuestItem ? 1 : 0, wNoGold ? 1 : 0);
	if (aItem.size() < 8) {
		return;
	}

	/* 状態を控える */
	PCHARINFO apPC[2] = { pA, pB };
	DWORD dwMapBack[2], dwXBack[2], dwYBack[2], dwGoldBack[2];
	WORD awItemBack[2][MAX_MYITEM], awEquBack[2][MAX_EQUTYPE];
	BYTE byAdminBack[2];
	char szAccBack[2][sizeof(pA->szAcc)], szNameBack[2][sizeof(pA->szName)];
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apPC[i];
		dwMapBack[i] = p->dwMapID; dwXBack[i] = p->dwX; dwYBack[i] = p->dwY; dwGoldBack[i] = p->dwGold;
		memcpy(awItemBack[i], p->waItem, sizeof(awItemBack[i]));
		memcpy(awEquBack[i], p->waEqu, sizeof(awEquBack[i]));
		byAdminBack[i] = p->byAdminLV;
		memcpy(szAccBack[i], p->szAcc, sizeof(szAccBack[i]));
		memcpy(szNameBack[i], p->szName, sizeof(szNameBack[i]));
		p->byAdminLV = ADMINLV_USER;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int j = 0; j < MAX_EQUTYPE; j ++) p->waEqu[j] = 0xFFFF;
	}
	strcpy(pA->szAcc, "mktsell"); strcpy(pA->szName, "売り手");
	strcpy(pB->szAcc, "mktbuy");  strcpy(pB->szName, "買い手");
	char *pszSpeechBack = pNpc->pszNpcSpeech;
	pNpc->pszNpcSpeech = new char[8];
	strcpy(pNpc->pszNpcSpeech, "@b");
	QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
	QuestMovePC(pB, nMap, aCell[1].first, aCell[1].second);
	MarketClear();
	MockTrace(1);

	/* 売り手の手持ち：品 8 個のうち先頭から 7 個 */
	for (int i = 0; i < 7; i ++) pA->waItem[i] = aItem[i];
	pB->dwGold = 5000;
	#define MKTCOUNT(p) GetItemCount((p)->dwTblID)
	DWORD dwMin0 = g_aItemInfo[aItem[0]]->dwGold / 4;
	if (dwMin0 == 0) dwMin0 = 1;
	printf("market init a_items=%d min0_ok=%d\n", MKTCOUNT(pA), (int)(dwMin0 >= 10));

	/* 受付が近くにいない時は何もできない（台詞が違う・遠い） */
	pNpc->pszNpcSpeech[1] = 0x78;
	std::vector<MARKETENTRY> aE;
	DWORD dwSale = 99;
	size_t nSize = 0;
	int nRes, nR1, nR2, nR3;
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
	nR2 = MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, NULL, NULL);
	printf("market nospeech put=%d list=%d items=%d\n", nR1, nR2, MKTCOUNT(pA));
	pNpc->pszNpcSpeech[1] = 0x62;
	DWORD dwXKeep = pA->dwX;
	pA->dwX = pNpc->dwX + MARKET_RANGE + 1;
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
	printf("market far put=%d items=%d\n", nR1, MKTCOUNT(pA));
	pA->dwX = pNpc->dwX + MARKET_RANGE;
	nR1 = MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, NULL, NULL);
	printf("market edge ok list=%d\n", nR1);
	pA->dwX = dwXKeep;

	/* 出せない品：位置が範囲外・空き枠・装備中・取引不可・依頼の品・値打ちの無い品は下限 1 */
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, MAX_MYITEM, 0, 0, 1000);
	nR2 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 10, 0, 0, 1000);
	printf("market ng range=%d empty=%d\n", nR1, nR2);
	pA->waEqu[0] = 1;
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 1, 0, 0, 1000);
	printf("market ng equip=%d items=%d\n", nR1, MKTCOUNT(pA));
	pA->waEqu[0] = 0xFFFF;
	if (wTrade) {
		pA->waItem[7] = wTrade;
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 7, 0, 0, 1000);
		printf("market ng trade=%d\n", nR1);
		pA->waItem[7] = 0;
	}
	if (wQuestItem) {
		pA->waItem[7] = wQuestItem;
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 7, 0, 0, 1000);
		printf("market ng quest=%d\n", nR1);
		pA->waItem[7] = 0;
	}
	if (wNoGold) {
		pA->waItem[7] = wNoGold;
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 7, 0, 0, 0);
		nR2 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 7, 0, 0, 1);
		printf("market nogold-item price0=%d price1=%d\n", nR1, nR2);
		printf("market nogold-item listed=%d items=%d\n", (int)g_aMarketItem.size(), MKTCOUNT(pA));
		/* 戻す（出品できてしまったので片付ける） */
		if (!g_aMarketItem.empty()) {
			MarketClear();
		}
		pA->waItem[7] = 0;
		for (int i = 0; i < 7; i ++) pA->waItem[i] = aItem[i];
	}
	/* 値段：下限未満・上限超え・ちょうど下限・ちょうど上限 */
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, dwMin0 - 1);
	nR2 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, MARKET_PRICEMAX + 1);
	nR3 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000, 5);
	printf("market price low=%d high=%d size_short=%d\n", nR1, nR2, nR3);
	printf("market price none listed=%d items=%d\n", (int)g_aMarketItem.size(), MKTCOUNT(pA));

	/* 出品：手数料 5%（切り捨て）。手取りは返事の dwGold */
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
	printf("market put1 res=%d id=%u item_ok=%d price=%u net=%u items=%d listed=%d\n", nRes, s_MarketLast.dwID,
		(int)(s_MarketLast.wItemID == aItem[0]), s_MarketLast.dwPrice, s_MarketLast.dwGold, MKTCOUNT(pA), (int)g_aMarketItem.size());
	/* 手持ちは詰まる：先頭が元の 2 番目の品になる */
	printf("market put1 shift first_ok=%d\n", (int)(pA->waItem[0] == aItem[1]));
	DWORD dwId1 = s_MarketLast.dwID;
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 999);			/* 手数料 49・手取り 950 */
	printf("market put2 res=%d net=%u\n", nRes, s_MarketLast.dwGold);
	DWORD dwId2 = s_MarketLast.dwID;
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 19);			/* 手数料 0（切り捨て）。下限を超えていれば */
	printf("market put3 res=%d net=%u\n", nRes, s_MarketLast.dwGold);
	if (nRes != MARKETRES_OK) {
		nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, MARKET_PRICEMAX);
		printf("market put3b res=%d net=%u\n", nRes, s_MarketLast.dwGold);
	}
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 20000);
	printf("market put4 res=%d\n", nRes);
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
	printf("market put5 res=%d\n", nRes);
	/* 6 件目は断る（期限切れも数える） */
	nRes = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
	printf("market put6 res=%d listed=%d items=%d\n", nRes, (int)g_aMarketItem.size(), MKTCOUNT(pA));
	/* 1 件目の次の ID は増えていく */
	printf("market ids ascending=%d\n", (int)(dwId2 > dwId1 && g_dwMarketNextID > dwId2));

	/* 一覧：自分のアカウントの品は「買う」に出ない・買い手には全部出る・新しい順 */
	dwSale = 99;
	int nA = MarketAskList(pA, nConnA, MARKETLIST_BUY, aE, &dwSale, &nSize);
	printf("market list a buy=%d size_ok=%d sale=%u\n", nA, (int)(nSize == offsetof(CMDPARA_MARKETLIST, aEntry)), dwSale);
	int nB = MarketAskList(pB, nConnB, MARKETLIST_BUY, aE, &dwSale, &nSize);
	printf("market list b buy=%d size_ok=%d sale=%u\n", nB, (int)(nSize == offsetof(CMDPARA_MARKETLIST, aEntry) + sizeof(MARKETENTRY) * nB), dwSale);
	BOOL bDesc = TRUE, bSeller = TRUE, bState = TRUE, bRemain = TRUE;
	for (size_t i = 0; i < aE.size(); i ++) {
		if (i && aE[i - 1].dwID <= aE[i].dwID) bDesc = FALSE;
		if (strcmp(aE[i].szSeller, "売り手") != 0) bSeller = FALSE;
		if (aE[i].byState != MARKETSTATE_ON) bState = FALSE;
		if (aE[i].dwRemain < MARKET_LIMITSEC - 5 || aE[i].dwRemain > MARKET_LIMITSEC) bRemain = FALSE;
	}
	printf("market list b entries desc=%d seller=%d on=%d remain=%d first_price=%u last_price=%u\n", bDesc, bSeller, bState, bRemain,
		aE.empty() ? 0 : aE[0].dwPrice, aE.empty() ? 0 : aE.back().dwPrice);
	int nMineA = MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, &dwSale, NULL);
	int nMineB = MarketAskList(pB, nConnB, MARKETLIST_MINE, aE, &dwSale, NULL);
	printf("market list mine a=%d b=%d\n", nMineA, nMineB);
	/* 一覧の種類が違う・短いパケットは届かない */
	{
		CMDPARA_MARKET Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_MARKETLIST; Para.byType = 9; Para.dwTblID = pA->dwTblID;
		int n0 = MockCmdCount(nConnA, CMD_MARKETLIST);
		TestRecv(pA, &Para, sizeof(Para));
		Para.byType = MARKETLIST_BUY;
		TestRecv(pA, &Para, 4);
		printf("market list invalid replies=%d\n", MockCmdCount(nConnA, CMD_MARKETLIST) - n0);
	}

	/* 買う：自分の品・もう無い・値段違い・品違い・所持金不足・手持ちいっぱい */
	printf("market buy own=%d\n", MarketAsk(pA, nConnA, CMD_MARKETBUY, 0, 0, aItem[1], dwId1, 1000));
	nR1 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], 9999, 1000);
	nR2 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwId1, 1001);
	nR3 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[3], dwId1, 1000);
	printf("market buy gone=%d wrongprice=%d wrongitem=%d\n", nR1, nR2, nR3);
	pB->dwGold = 999;
	nR1 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwId1, 1000);
	printf("market buy nogold=%d gold=%u\n", nR1, pB->dwGold);
	pB->dwGold = 5000;
	for (int i = 0; i < MAX_MYITEM; i ++) pB->waItem[i] = aItem[7];
	nR1 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwId1, 1000);
	printf("market buy full=%d gold=%u items=%d listed=%d\n", nR1, pB->dwGold, MKTCOUNT(pB), (int)g_aMarketItem.size());
	pB->waItem[MAX_MYITEM - 1] = 0;
	/* 買えた：払う・品が手持ちへ・売上は手取り（1000-50）・出品は消える */
	nRes = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwId1, 1000);
	printf("market buy ok=%d gold=%u items=%d last_ok=%d listed=%d sale_a=%u sale_b=%u\n", nRes, pB->dwGold, MKTCOUNT(pB),
		(int)(pB->waItem[MAX_MYITEM - 1] == aItem[0]), (int)g_aMarketItem.size(), MarketGetSale("mktsell"), MarketGetSale("mktbuy"));
	nR1 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwId1, 1000);
	printf("market buy again=%d\n", nR1);
	pB->waItem[MAX_MYITEM - 2] = 0;
	nRes = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[1], dwId2, 999);
	printf("market buy2=%d gold=%u sale_a=%u (999-49)\n", nRes, pB->dwGold, MarketGetSale("mktsell"));

	/* 売上は一覧の見出しにも付く（手取りの合計） */
	nR1 = MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, &dwSale, NULL);
	printf("market list mine after sale=%d sale=%u\n", nR1, dwSale);

	/* 売上の受け取り：買い手には無い・売り手は所持金の上限まで・上限なら受け取れない */
	nR1 = MarketAsk(pB, nConnB, CMD_MARKETGOLDGET, 0, 0, 0, 0, 0);
	printf("market goldget nosale=%d\n", nR1);
	DWORD dwSaleBefore = MarketGetSale("mktsell");
	pA->dwGold = 999999 - 100;
	nRes = MarketAsk(pA, nConnA, CMD_MARKETGOLDGET, 0, 0, 0, 0, 0);
	printf("market goldget cap res=%d got=%u left=%u gold=%u sale=%u\n", nRes, s_MarketLast.dwGold, s_MarketLast.dwPrice, pA->dwGold, MarketGetSale("mktsell"));
	nRes = MarketAsk(pA, nConnA, CMD_MARKETGOLDGET, 0, 0, 0, 0, 0);
	printf("market goldget full res=%d gold=%u sale=%u\n", nRes, pA->dwGold, MarketGetSale("mktsell"));
	pA->dwGold = 0;
	nRes = MarketAsk(pA, nConnA, CMD_MARKETGOLDGET, 0, 0, 0, 0, 0);
	printf("market goldget all res=%d got=%u left=%u gold=%u sale=%u rows=%d same_total=%d\n", nRes, s_MarketLast.dwGold, s_MarketLast.dwPrice, pA->dwGold,
		MarketGetSale("mktsell"), (int)g_aMarketSale.size(), (int)(dwSaleBefore == pA->dwGold + 100));
	nR1 = MarketAsk(pA, nConnA, CMD_MARKETGOLDGET, 0, 0, 0, 0, 0);
	printf("market goldget again=%d\n", nR1);

	/* 期限切れ：一覧に出ない・買えない・出品者は「期限切れ」で見える・引き取れる */
	{
		MarketClear();
		for (int i = 0; i < MAX_MYITEM; i ++) pB->waItem[i] = 0;
		pA->waItem[0] = aItem[0]; pA->waItem[1] = aItem[1]; pA->waItem[2] = aItem[2];
		MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 3000);
		DWORD dwIdOld = s_MarketLast.dwID;
		MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 4000);
		DWORD dwIdNew = s_MarketLast.dwID;
		MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 5000);
		DWORD dwIdMid = s_MarketLast.dwID;
		/* 古いほうを 8 日前、真ん中を 6 日と 23 時間前（まだ有効）にする */
		DWORD dwNow = (DWORD)time(NULL);
		for (size_t i = 0; i < g_aMarketItem.size(); i ++) {
			if (g_aMarketItem[i].dwID == dwIdOld) g_aMarketItem[i].dwTime = dwNow - 8 * 24 * 3600;
			if (g_aMarketItem[i].dwID == dwIdMid) g_aMarketItem[i].dwTime = dwNow - (MARKET_LIMITSEC - 3600);
		}
		int n = MarketAskList(pB, nConnB, MARKETLIST_BUY, aE, &dwSale, NULL);
		printf("market expire buylist=%d ids_ok=%d remain_mid=%d\n", n, (int)(n == 2 && aE[0].dwID == dwIdMid && aE[1].dwID == dwIdNew),
			(int)(n == 2 && aE[0].dwRemain > 3500 && aE[0].dwRemain <= 3600));
		nR1 = MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], dwIdOld, 3000);
		printf("market expire buy=%d\n", nR1);
		n = MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, &dwSale, NULL);
		printf("market expire mine=%d state=%d,%d,%d remain_old=%u\n", n, n == 3 ? aE[0].byState : -1, n == 3 ? aE[1].byState : -1, n == 3 ? aE[2].byState : -1,
			n == 3 ? aE[2].dwRemain : 1);
		/* 引き取り：他人の品・無い品は断る。手持ちがいっぱいなら断って残す。期限切れも出品中も戻せる */
		nR1 = MarketAsk(pB, nConnB, CMD_MARKETTAKEBACK, 0, 0, 0, dwIdOld, 0);
		nR2 = MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, 12345, 0);
		printf("market takeback other=%d none=%d\n", nR1, nR2);
		for (int i = 0; i < MAX_MYITEM; i ++) pA->waItem[i] = aItem[7];
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, dwIdOld, 0);
		printf("market takeback full=%d listed=%d\n", nR1, (int)g_aMarketItem.size());
		for (int i = 0; i < MAX_MYITEM; i ++) pA->waItem[i] = 0;
		nRes = MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, dwIdOld, 0);
		printf("market takeback expired=%d item_ok=%d listed=%d first_ok=%d\n", nRes, (int)(s_MarketLast.wItemID == aItem[0]), (int)g_aMarketItem.size(), (int)(pA->waItem[0] == aItem[0]));
		nRes = MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, dwIdNew, 0);
		printf("market takeback active=%d listed=%d items=%d\n", nRes, (int)g_aMarketItem.size(), MKTCOUNT(pA));
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, dwIdNew, 0);
		printf("market takeback again=%d\n", nR1);
		/* 出品数は引き取ると空く（期限切れも数えていたので、5 件いっぱいから 1 件戻して出せる） */
		MarketClear();
		for (int i = 0; i < 6; i ++) pA->waItem[i] = aItem[i];
		int nOk = 0;
		for (int i = 0; i < 6; i ++) if (MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000) == MARKETRES_OK) nOk ++;
		g_aMarketItem[0].dwTime = dwNow - 9 * 24 * 3600;
		MarketAskList(pA, nConnA, MARKETLIST_MINE, aE, &dwSale, NULL);		/* 期限切れに変わる */
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
		printf("market limit ok=%d listed=%d put_with_expired=%d\n", nOk, (int)g_aMarketItem.size(), nR1);
		MarketAsk(pA, nConnA, CMD_MARKETTAKEBACK, 0, 0, 0, g_aMarketItem[0].dwID, 0);
		nR1 = MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
		printf("market limit after takeback listed=%d put=%d\n", (int)g_aMarketItem.size(), nR1);
	}

	/* 保存して読み直す：出品・売上・状態・次の出品 ID */
	{
		MarketClear();
		for (int i = 0; i < 3; i ++) pA->waItem[i] = aItem[i];
		for (int i = 3; i < MAX_MYITEM; i ++) pA->waItem[i] = 0;
		MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 1000);
		MarketAsk(pA, nConnA, CMD_MARKETPUT, 0, 0, 0, 0, 2000);
		MarketAsk(pB, nConnB, CMD_MARKETBUY, 0, 0, aItem[0], g_aMarketItem[0].dwID, 1000);
		pB->waItem[MAX_MYITEM - 1] = 0;
		g_aMarketItem[0].byState = MARKETSTATE_EXPIRED;
		MARKETSALE Sale;
		ZeroMemory(&Sale, sizeof(Sale));
		strcpy(Sale.szAcc, "mktother"); Sale.dwGold = 4242;
		g_aMarketSale.push_back(Sale);
		std::vector<MARKETITEM> aItemBefore = g_aMarketItem;
		std::vector<MARKETSALE> aSaleBefore = g_aMarketSale;
		DWORD dwNextBefore = g_dwMarketNextID;
		BOOL bSaved = SaveMarket();
		MarketClear();
		BOOL bLoaded = LoadMarket();
		BOOL bSame = (aItemBefore.size() == g_aMarketItem.size()) && (aSaleBefore.size() == g_aMarketSale.size());
		for (size_t i = 0; bSame && i < aItemBefore.size(); i ++) {
			bSame = (memcmp(&aItemBefore[i], &g_aMarketItem[i], sizeof(MARKETITEM)) == 0);
		}
		for (size_t i = 0; bSame && i < aSaleBefore.size(); i ++) {
			bSame = (memcmp(&aSaleBefore[i], &g_aMarketSale[i], sizeof(MARKETSALE)) == 0);
		}
		printf("market save=%d load=%d same=%d items=%d sales=%d next_same=%d sale_a=%u other=%u\n", (int)bSaved, (int)bLoaded, (int)bSame,
			(int)g_aMarketItem.size(), (int)g_aMarketSale.size(), (int)(g_dwMarketNextID == dwNextBefore), MarketGetSale("mktsell"), MarketGetSale("mktother"));
		/* 空にして保存すると、次に読むと 0 件 */
		MarketClear();
		bSaved = SaveMarket();
		bLoaded = LoadMarket();
		printf("market save empty=%d load=%d items=%d sales=%d next=%u\n", (int)bSaved, (int)bLoaded, (int)g_aMarketItem.size(), (int)g_aMarketSale.size(), g_dwMarketNextID);
	}

	/* 片付け */
	MockTrace(0);
	MarketClear();
	delete[] pNpc->pszNpcSpeech;
	pNpc->pszNpcSpeech = pszSpeechBack;
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apPC[i];
		memcpy(p->waItem, awItemBack[i], sizeof(awItemBack[i]));
		memcpy(p->waEqu, awEquBack[i], sizeof(awEquBack[i]));
		p->dwGold = dwGoldBack[i];
		p->byAdminLV = byAdminBack[i];
		memcpy(p->szAcc, szAccBack[i], sizeof(szAccBack[i]));
		memcpy(p->szName, szNameBack[i], sizeof(szNameBack[i]));
		QuestMovePC(p, (int)dwMapBack[i], (int)dwXBack[i], (int)dwYBack[i]);
	}
	#undef MKTCOUNT
	printf("market end\n");
}

/* ------------------------------------------------------------------------- */
/* AH. リサイクルボックス（おすそわけの共有の箱。CMD_RECYCLE*）				 */
/*    マップに置いた箱（印で変わるマスの kind 2）を正面で調べると、その人の画面だけ */
/*    開いた絵になって窓が開く・閉じると戻る・別の人には開いて見えない			 */
/*    箱の近くでだけ使える・入れる（まとめる・20種類・255個・装備中・依頼の品・	 */
/*    取引不可を断る）・もらう（1個ずつ・0個で消える・1日3個・日本時間の0時で戻る・ */
/*    手持ちいっぱい）・箱番号ごとに別の中身・保存して読み直し・記録・データAPI	 */
/*    時刻は g_dwRecycleNowOverride で決め打ちにする（日付の変わり目を試す）	 */
/* ------------------------------------------------------------------------- */
extern BOOL SaveRecycle(void);
extern BOOL SavePuzzleRecord(void);
extern BOOL LoadPuzzleRecord(void);
extern BOOL LoadRecycle(void);
extern "C" const char *SboWebDataRecycle();
extern "C" const char *SboWebDataPuzzleRecords();
static std::string SwapApi(int nMap, const char *pszJson);
static void SwapRestore(PMAPINFO p, const MAPINFO &Bak);
static void MimicExamine(PCHARINFO pPC);

/* 箱へ頼んで結果（RECYCLERES_*。届かなければ -1）を返す。届いたパケットは s_RecycleLast に入る */
static CMDPARA_RECYCLE s_RecycleLast;
static int RecycleAsk(PCHARINFO p, int nConn, BYTE byCmd, WORD wItem, BYTE byCount, DWORD dwSize = sizeof(CMDPARA_RECYCLE))
{
	CMDPARA_RECYCLE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = byCmd;
	Para.wItemID = wItem;
	Para.byCount = byCount;
	Para.dwTblID = p->dwTblID;
	int nBefore = MockCmdCount(nConn, byCmd);
	TestRecv(p, &Para, dwSize);
	if (MockCmdCount(nConn, byCmd) == nBefore) {
		return -1;
	}
	std::vector<BYTE> a;
	if (!MarketLast(nConn, byCmd, a) || a.size() < sizeof(CMDPARA_RECYCLE)) {
		return -1;
	}
	memcpy(&s_RecycleLast, &a[0], sizeof(s_RecycleLast));
	return s_RecycleLast.byResult;
}

/* 中身を頼む。届いた種類の数（届かなければ -1）。aEntry に中身、pnLeft に今日あと何個、pnSize に届いた大きさ */
static int RecycleAskList(PCHARINFO p, int nConn, std::vector<RECYCLEENTRY> &aEntry, int *pnLeft, size_t *pnSize)
{
	CMDPARA_RECYCLE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_RECYCLELIST;
	Para.dwTblID = p->dwTblID;
	int nBefore = MockCmdCount(nConn, CMD_RECYCLELIST);
	TestRecv(p, &Para, sizeof(Para));
	aEntry.clear();
	if (MockCmdCount(nConn, CMD_RECYCLELIST) == nBefore) {
		return -1;
	}
	std::vector<BYTE> a;
	if (!MarketLast(nConn, CMD_RECYCLELIST, a) || a.size() < offsetof(CMDPARA_RECYCLELIST, aEntry)) {
		return -1;
	}
	std::vector<BYTE> aFull(sizeof(CMDPARA_RECYCLELIST), 0);
	memcpy(&aFull[0], &a[0], a.size() < aFull.size() ? a.size() : aFull.size());
	PCMDPARA_RECYCLELIST pList = (PCMDPARA_RECYCLELIST)&aFull[0];
	for (int i = 0; i < pList->byKind; i ++) {
		aEntry.push_back(pList->aEntry[i]);
	}
	if (pnLeft) *pnLeft = pList->byLeft;
	if (pnSize) *pnSize = a.size();
	return pList->byKind;
}

/* swaps を API で書いて、通ったか */
static BOOL SwapOk(int nMap, const char *pszJson)
{
	return SwapApi(nMap, pszJson).find("\"ok\":true") != std::string::npos;
}

/* 全マップを通して使ったリサイクル箱の番号の最大（自動で振られる次の番号は、これ+1） */
static int RecycleBoxHighAllForTest(void)
{
	int nHigh = 0;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		PMAPINFO p = g_aMapInfo[i];
		nHigh = max(nHigh, (int)p->wBoxHigh);
		for (int j = 0; j < p->bySwapCount; j ++) {
			if (p->aSwap[j].byKind == MAPSWAPKIND_RECYCLE) nHigh = max(nHigh, (int)p->aSwap[j].wBoxNo);
		}
	}
	return nHigh;
}

/* 窓を閉じたことを知らせる（CMD_RECYCLECLOSE。返事は無い） */
static void RecycleAskClose(PCHARINFO p)
{
	CMDPARA_RECYCLE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_RECYCLECLOSE;
	Para.dwTblID = p->dwTblID;
	TestRecv(p, &Para, sizeof(Para));
}

/* p に送る差し替え一覧の、差し替え中（APPLY）と調べられる（EXAMINE）の数 */
static void RecycleSwapFlags(PCHARINFO p, PMAPINFO pMap, int *pnApply, int *pnExam)
{
	std::vector<BYTE> aCap;
	*pnApply = *pnExam = -1;
	SvrSendCaptureBegin();
	MapSwapSend(p, pMap, TRUE);
	if (SvrSendCaptureEnd(aCap) && aCap.size() >= sizeof(CMDPARA_MAPSWAP) && aCap[0] == CMD_MAPSWAP) {
		PCMDPARA_MAPSWAP pP = (PCMDPARA_MAPSWAP)&aCap[0];
		PMAPSWAPSEND pS = (PMAPSWAPSEND)(&aCap[0] + sizeof(CMDPARA_MAPSWAP));
		*pnApply = *pnExam = 0;
		for (int i = 0; i < pP->wCount && (size_t)((BYTE *)&pS[i + 1] - &aCap[0]) <= aCap.size(); i ++) {
			if (pS[i].byFlg & MAPSWAPSEND_APPLY) (*pnApply) ++;
			if (pS[i].byFlg & MAPSWAPSEND_EXAMINE) (*pnExam) ++;
		}
	}
}

static void TestRecycle(const std::vector<int> &aLogin)
{
	printf("== AH. Recycle ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]];
	int nConnA = 1000, nConnB = 1001;
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];

	/* 箱を置くマスと、プレイヤー 2 人が立つ空きマス。A は (x0,y0) で上向きなら箱 1 が正面、B は (x0+1,y0) で上向きなら箱 2 が正面。
	   箱はマップのパーツのまま（歩けるかは問わない）。2 つの箱はどちらも、2 人の 3 マス以内 */
	int x0 = -1, y0 = -1;
	for (int y = 4; y < pMap->nCY - 3 && x0 < 0; y ++) {
		for (int x = 3; x < pMap->nCX - 4 && x0 < 0; x ++) {
			BOOL bFree = TRUE;
			for (int k = 0; k < 2; k ++) {
				if (!CheckCharBlock((DWORD)nMap, x + k, y) || CheckCharBlockNPC((DWORD)nMap, x + k, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x + k, y) != (DWORD)-1) bFree = FALSE;
			}
			if (bFree) { x0 = x; y0 = y; }
		}
	}
	printf("recycle cells=%d\n", x0 >= 0 ? 1 : 0);
	if (x0 < 0) {
		return;
	}

	/* 品：取り引きできる・大事な物でも依頼の品でもない品を 24 種類。取引不可・依頼の品・大事な物も探す */
	std::vector<WORD> aItem;
	WORD wTrade = 0;
	for (int i = 3; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem->szName[0] == 0) continue;
		if (pItem->byFlg & ITEMFLG_TRADE) {
			if (!wTrade && !IsImportantItem((WORD)i)) wTrade = (WORD)i;
		} else if (QuestIsQuestItem((WORD)i) || (pItem->byFlg & ITEMFLG_IMPORTANT)) {
			continue;
		} else if (aItem.size() < 24) {
			aItem.push_back((WORD)i);
		}
	}
	printf("recycle picked items=%d trade=%d\n", (int)aItem.size(), wTrade ? 1 : 0);
	if (aItem.size() < 24) {
		return;
	}
	/* 依頼の品・大事な物は、手元のデータに無くても試せるよう、一時的に作る（終わりに戻す） */
	WORD wQuestItem = aItem[22], wImportant = aItem[23];
	std::vector<PQUESTINFO> aQuestBack;
	for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) aQuestBack.push_back(g_aQuestInfo[i]);
	g_aQuestInfo.RemoveAll();
	PQUESTINFO pQuestTmp = new QUESTINFO;
	ZeroMemory(pQuestTmp, sizeof(*pQuestTmp));
	pQuestTmp->wNeedFlag = pQuestTmp->wNotFlag = FLAG_NOBIT;
	pQuestTmp->wID = 1; pQuestTmp->byState = QUESTSTATE_ON; pQuestTmp->wQuestItem = wQuestItem;
	g_aQuestInfo.Add(pQuestTmp);
	BYTE byImportantFlgBack = g_aItemInfo[wImportant]->byFlg;
	g_aItemInfo[wImportant]->byFlg |= ITEMFLG_IMPORTANT;
	printf("recycle temp quest=%d important=%d\n", (int)QuestIsQuestItem(wQuestItem), (int)IsImportantItem(wImportant));

	/* 状態を控える */
	PCHARINFO apPC[2] = { pA, pB };
	DWORD dwMapBack[2], dwXBack[2], dwYBack[2];
	WORD awItemBack[2][MAX_MYITEM], awEquBack[2][MAX_EQUTYPE];
	BYTE byAdminBack[2];
	char szAccBack[2][sizeof(pA->szAcc)], szNameBack[2][sizeof(pA->szName)];
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apPC[i];
		dwMapBack[i] = p->dwMapID; dwXBack[i] = p->dwX; dwYBack[i] = p->dwY;
		memcpy(awItemBack[i], p->waItem, sizeof(awItemBack[i]));
		memcpy(awEquBack[i], p->waEqu, sizeof(awEquBack[i]));
		byAdminBack[i] = p->byAdminLV;
		memcpy(szAccBack[i], p->szAcc, sizeof(szAccBack[i]));
		memcpy(szNameBack[i], p->szName, sizeof(szNameBack[i]));
		p->byAdminLV = ADMINLV_USER;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int j = 0; j < MAX_EQUTYPE; j ++) p->waEqu[j] = 0xFFFF;
	}
	strcpy(pA->szAcc, "rcyA"); strcpy(pA->szName, "入れ手");
	strcpy(pB->szAcc, "rcyB"); strcpy(pB->szName, "もらい手");
	/* 箱の行を 2 つ置く（終わりに元へ戻す）。閉じた絵はマップのパーツのまま、開いた絵は別のパーツ */
	MAPINFO MapBak = *pMap;
	const WORD dwBox = 901, dwBox2 = 902;
	const int xBox1 = x0, yBox = y0 - 1, xBox2 = x0 + 1;
	WORD wBase1 = pMap->pwMap[xBox1 + yBox * pMap->nCX], wBase2 = pMap->pwMap[xBox2 + yBox * pMap->nCX];
	const WORD wOpen1 = (WORD)((wBase1 == 1) ? 2 : 1), wOpen2 = (WORD)((wBase2 == 3) ? 4 : 3);
	pMap->bySwapCount = 2;
	ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
	pMap->aSwap[0].wX = (WORD)xBox1; pMap->aSwap[0].wY = (WORD)yBox; pMap->aSwap[0].byKind = MAPSWAPKIND_RECYCLE; pMap->aSwap[0].wParts = wOpen1; pMap->aSwap[0].wBoxNo = dwBox;
	pMap->aSwap[1].wX = (WORD)xBox2; pMap->aSwap[1].wY = (WORD)yBox; pMap->aSwap[1].byKind = MAPSWAPKIND_RECYCLE; pMap->aSwap[1].wParts = wOpen2; pMap->aSwap[1].wBoxNo = dwBox2;
	WORD wRecycleBoxBack[2] = { pA->wRecycleBox, pB->wRecycleBox };
	pA->wRecycleBox = pB->wRecycleBox = 0;
	QuestMovePC(pA, nMap, x0, y0);
	QuestMovePC(pB, nMap, x0 + 1, y0);
	pA->byDirection = pB->byDirection = 0;
	RecycleClear();
	MockTrace(1);

	/* 時刻を決め打ちにする：日本時間 2026-11-17 の 23:59:00（この日の最後の 1 分） */
	const DWORD dwDay0 = 20774;										/* 2026-11-17（日本時間で 1970/1/1 から数えた日数） */
	const DWORD dwJstEnd = (dwDay0 + 1) * 86400 - 9 * 3600;		/* 日本時間 2026-11-18 の 0 時（UTC の秒） */
	g_dwRecycleNowOverride = dwJstEnd - 60;
	printf("recycle day today=%u next=%u left=%d\n", RecycleDay(RecycleNowSec()) - dwDay0, RecycleDay(dwJstEnd) - dwDay0, RecycleLeft("rcyA"));

	#define RCYCOUNT(p) GetItemCount((p)->dwTblID)
	#define RCYHAND(p, ...) do { int a_[] = { __VA_ARGS__ }; ZeroMemory((p)->waItem, sizeof((p)->waItem)); for (size_t k_ = 0; k_ < sizeof(a_) / sizeof(a_[0]); k_ ++) (p)->waItem[k_] = (WORD)a_[k_]; } while (0)
	std::vector<RECYCLEENTRY> aE;
	int nLeft = -1;
	size_t nSize = 0;
	int nRes, nR1, nR2, nR3;

	/* 調べる前の見え方：2 つの箱は閉じた絵（マップのパーツのまま）で、どちらも調べられるマスとして届く */
	int nApply = -1, nExam = -1;
	RecycleSwapFlags(pA, pMap, &nApply, &nExam);
	printf("recycle closed a apply=%d examine=%d parts_closed=%d,%d box=%d\n", nApply, nExam,
		(int)(MapSwapParts(pMap, pA, xBox1, yBox) == wBase1), (int)(MapSwapParts(pMap, pA, xBox2, yBox) == wBase2), (int)pA->wRecycleBox);

	/* 調べる前（窓を開いていない）は何もできない：入れる・もらうは断られ、中身は届かない */
	RCYHAND(pA, aItem[0], aItem[0]);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	nR2 = RecycleAskList(pA, nConnA, aE, NULL, NULL);
	nR3 = RecycleAsk(pA, nConnA, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle notopened put=%d list=%d take=%d items=%d\n", nR1, nR2, nR3, RCYCOUNT(pA));
	/* 箱の番号が置いてある箱と合わない（箱を消した）時も同じ */
	pA->wRecycleBox = 555;
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	printf("recycle nobox put=%d items=%d state_cleared=%d\n", nR1, RCYCOUNT(pA), (int)(pA->wRecycleBox == 0));

	/* 正面を調べる（Sキー）：その人の画面だけ開いた絵になり、窓を開く知らせ（中身つき）が届く。位置も届いて待ちを解く */
	{
		int nSw0 = MockCmdCount(nConnA, CMD_MAPSWAP), nSwB0 = MockCmdCount(nConnB, CMD_MAPSWAP), nList0 = MockCmdCount(nConnA, CMD_RECYCLELIST);
		int nPos0 = MockCmdCount(nConnA, CMD_CHARPOS), nListB0 = MockCmdCount(nConnB, CMD_RECYCLELIST);
		MimicExamine(pA);
		std::vector<BYTE> aL;
		int nOpenFlag = -1, nKinds = -1;
		if (MarketLast(nConnA, CMD_RECYCLELIST, aL) && aL.size() >= offsetof(CMDPARA_RECYCLELIST, aEntry)) {
			nOpenFlag = ((PCMDPARA_RECYCLELIST)&aL[0])->byOpen;
			nKinds = ((PCMDPARA_RECYCLELIST)&aL[0])->byKind;
		}
		printf("recycle examine box=%d list_push=%d open_flag=%d kinds=%d charpos=%d swap=%d parts_a_open=%d other_box_closed=%d\n", (int)pA->wRecycleBox,
			MockCmdCount(nConnA, CMD_RECYCLELIST) - nList0, nOpenFlag, nKinds, MockCmdCount(nConnA, CMD_CHARPOS) - nPos0,
			MockCmdCount(nConnA, CMD_MAPSWAP) - nSw0, (int)(MapSwapParts(pMap, pA, xBox1, yBox) == wOpen1), (int)(MapSwapParts(pMap, pA, xBox2, yBox) == wBase2));
		/* 別の人（B）には開いて見えない・何も届かない */
		printf("recycle examine b_view closed=%d b_box=%d b_swap=%d b_list=%d\n", (int)(MapSwapParts(pMap, pB, xBox1, yBox) == wBase1), (int)pB->wRecycleBox,
			MockCmdCount(nConnB, CMD_MAPSWAP) - nSwB0, MockCmdCount(nConnB, CMD_RECYCLELIST) - nListB0);
		RecycleSwapFlags(pA, pMap, &nApply, &nExam);
		printf("recycle examine a flags apply=%d examine=%d\n", nApply, nExam);
		RecycleSwapFlags(pB, pMap, &nApply, &nExam);
		printf("recycle examine b flags apply=%d examine=%d\n", nApply, nExam);
	}
	/* 窓の中の頼みごとが通る（開いた直後に中身を頼む）。箱の遠近は箱のマスから測る */
	nR1 = RecycleAskList(pA, nConnA, aE, &nLeft, &nSize);
	printf("recycle opened list=%d left=%d size_ok=%d\n", nR1, nLeft, (int)(nSize == offsetof(CMDPARA_RECYCLELIST, aEntry)));
	/* 窓を閉じる：閉じた絵に戻り、差し替え一覧が本人にだけ届く。閉じた後は頼めない */
	{
		int nSw0 = MockCmdCount(nConnA, CMD_MAPSWAP), nSwB0 = MockCmdCount(nConnB, CMD_MAPSWAP);
		RecycleAskClose(pA);
		printf("recycle close box=%d parts_closed=%d swap=%d b_swap=%d\n", (int)pA->wRecycleBox, (int)(MapSwapParts(pMap, pA, xBox1, yBox) == wBase1),
			MockCmdCount(nConnA, CMD_MAPSWAP) - nSw0, MockCmdCount(nConnB, CMD_MAPSWAP) - nSwB0);
		RecycleSwapFlags(pA, pMap, &nApply, &nExam);
		printf("recycle close flags apply=%d examine=%d\n", nApply, nExam);
		nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
		printf("recycle close then put=%d items=%d\n", nR1, RCYCOUNT(pA));
		/* 開いていない時に閉じても何も起きない */
		nSw0 = MockCmdCount(nConnA, CMD_MAPSWAP);
		RecycleAskClose(pA);
		printf("recycle close again swap=%d\n", MockCmdCount(nConnA, CMD_MAPSWAP) - nSw0);
	}
	/* 向きが違うと開かない（正面の箱だけ）。B は自分の正面の箱 2 を開く */
	pA->byDirection = 1;
	MimicExamine(pA);
	printf("recycle examine wrong dir box=%d\n", (int)pA->wRecycleBox);
	pA->byDirection = 0;
	MimicExamine(pB);
	printf("recycle examine b box=%d parts_b_open=%d a_closed=%d\n", (int)pB->wRecycleBox, (int)(MapSwapParts(pMap, pB, xBox2, yBox) == wOpen2),
		(int)(MapSwapParts(pMap, pA, xBox2, yBox) == wBase2));
	RecycleAskClose(pB);
	/* マップを移った・箱を消した時：開いた記憶は捨てる（別のマップへの差し替え一覧を作ると消える） */
	pA->wRecycleBox = dwBox;
	if (g_aMapInfo.GetSize() >= 2) {
		PMAPINFO pOtherMap = g_aMapInfo[(nMap + 1) % g_aMapInfo.GetSize()];
		MapSwapSend(pA, pOtherMap, TRUE);
		printf("recycle other map cleared=%d\n", (int)(pA->wRecycleBox == 0));
	}
	/* 窓を開いたまま遠ざかった：箱のマスから 3 マスまでは使える（端）。4 マスで断られ、箱は閉じる */
	MimicExamine(pA);
	DWORD dwXKeep = pA->dwX;
	pA->dwX = (DWORD)(xBox1 + RECYCLE_RANGE + 1);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	printf("recycle far put=%d items=%d box=%d parts_closed=%d\n", nR1, RCYCOUNT(pA), (int)pA->wRecycleBox, (int)(MapSwapParts(pMap, pA, xBox1, yBox) == wBase1));
	pA->dwX = dwXKeep;
	MimicExamine(pA);
	pA->dwX = (DWORD)(xBox1 + RECYCLE_RANGE);
	pA->dwY = (DWORD)(yBox + RECYCLE_RANGE);
	nR1 = RecycleAskList(pA, nConnA, aE, &nLeft, &nSize);
	printf("recycle edge ok list=%d left=%d size_ok=%d\n", nR1, nLeft, (int)(nSize == offsetof(CMDPARA_RECYCLELIST, aEntry)));
	pA->dwY = (DWORD)(yBox + RECYCLE_RANGE + 1);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	printf("recycle far y put=%d box=%d\n", nR1, (int)pA->wRecycleBox);
	pA->dwX = dwXKeep;
	pA->dwY = (DWORD)y0;
	/* ここから先は、A も B も自分の正面の箱を調べて開いた状態で試す（A=箱 1、B=箱 1 は下で切り替える） */
	MimicExamine(pA);
	pB->wRecycleBox = dwBox;
	/* 短いパケットは届かない */
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1, 4);
	printf("recycle short put=%d items=%d\n", nR1, RCYCOUNT(pA));

	/* 入れる：同じ品 2 個のうち 2 個。手持ちは詰まる。個数が 0・多すぎ・品 0・持っていない品は断る */
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 0);
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 3);
	nR3 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, 0, 1);
	printf("recycle put count0=%d toomany=%d item0=%d items=%d\n", nR1, nR2, nR3, RCYCOUNT(pA));
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[5], 1);
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, (WORD)g_aItemInfo.GetSize(), 1);
	printf("recycle put nothave=%d badid=%d stock=%d\n", nR1, nR2, (int)g_aRecycleStock.size());
	RCYHAND(pA, aItem[1], aItem[0], aItem[2], aItem[0], aItem[0]);
	nRes = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 2);
	printf("recycle put1 res=%d item_ok=%d count=%d left=%d items=%d first_ok=%d second_ok=%d stock0=%d kinds=%d log=%d\n", nRes,
		(int)(s_RecycleLast.wItemID == aItem[0]), s_RecycleLast.byCount, s_RecycleLast.byLeft, RCYCOUNT(pA),
		(int)(pA->waItem[0] == aItem[1]), (int)(pA->waItem[1] == aItem[0] && pA->waItem[2] == aItem[2]),
		RecycleStockCount(dwBox, aItem[0]), RecycleKindCount(dwBox), (int)g_aRecycleLog.size());
	/* まとめる：同じ品は 1 行のまま個数が増える */
	nRes = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	printf("recycle put2 merge res=%d stock0=%d kinds=%d rows=%d items=%d log=%d\n", nRes, RecycleStockCount(dwBox, aItem[0]), RecycleKindCount(dwBox),
		(int)g_aRecycleStock.size(), RCYCOUNT(pA), (int)g_aRecycleLog.size());

	/* 装備中は入れられない。同じ品を 2 つ持っていて片方が装備中なら、装備していない分だけ */
	RCYHAND(pA, aItem[1], aItem[2], aItem[2]);
	pA->waEqu[0] = 1;
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[2], 2);
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[2], 1);
	printf("recycle equip two_of_two=%d one=%d items=%d equ_kept=%d stock2=%d\n", nR1, nR2, RCYCOUNT(pA), (int)(pA->waEqu[0] < MAX_MYITEM && pA->waItem[pA->waEqu[0]] == aItem[2]), RecycleStockCount(dwBox, aItem[2]));
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[2], 1);
	printf("recycle equip only_equipped=%d items=%d\n", nR1, RCYCOUNT(pA));
	pA->waEqu[0] = 0xFFFF;
	/* 取引不可・依頼の品・大事な物は入れられない */
	if (wTrade) {
		RCYHAND(pA, wTrade);
		printf("recycle ng trade=%d items=%d\n", RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, wTrade, 1), RCYCOUNT(pA));
	}
	RCYHAND(pA, wQuestItem);
	printf("recycle ng quest=%d items=%d\n", RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, wQuestItem, 1), RCYCOUNT(pA));
	RCYHAND(pA, wImportant);
	printf("recycle ng important=%d items=%d\n", RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, wImportant, 1), RCYCOUNT(pA));
	printf("recycle ng stock_rows=%d kinds=%d\n", (int)g_aRecycleStock.size(), RecycleKindCount(dwBox));

	/* 中身の一覧：入れた順・個数・今日あと何個・届く大きさ */
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, &nSize);
	printf("recycle list b kinds=%d left=%d size_ok=%d order_ok=%d counts=%d,%d\n", nR1, nLeft,
		(int)(nSize == offsetof(CMDPARA_RECYCLELIST, aEntry) + sizeof(RECYCLEENTRY) * (nR1 > 0 ? nR1 : 0)),
		(int)(aE.size() == 2 && aE[0].wItemID == aItem[0] && aE[1].wItemID == aItem[2]), aE.size() > 0 ? aE[0].byCount : -1, aE.size() > 1 ? aE[1].byCount : -1);

	/* 別の箱（箱番号が違う）は中身が別々 */
	pB->wRecycleBox = dwBox2;
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, NULL);
	RCYHAND(pA, aItem[3]);
	pA->wRecycleBox = dwBox2;
	nRes = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[3], 1);
	pA->wRecycleBox = dwBox;
	nR2 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle other box empty=%d put=%d take_other_box_item=%d kinds2=%d kinds1=%d\n", nR1, nRes, nR2, RecycleKindCount(dwBox2), RecycleKindCount(dwBox));
	pB->wRecycleBox = dwBox;
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, NULL);
	printf("recycle back to box1 kinds=%d\n", nR1);
	/* 別の箱の品を片付ける */
	for (size_t i = g_aRecycleStock.size(); i > 0; i --) {
		if (g_aRecycleStock[i - 1].dwBoxNo == dwBox2) g_aRecycleStock.erase(g_aRecycleStock.begin() + (i - 1));
	}

	/* もらう：1 回 1 個・箱の中身が 1 つ減る・手持ちに入る・無い品は断る */
	ZeroMemory(pB->waItem, sizeof(pB->waItem));
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[9], 0);
	printf("recycle take none=%d left=%d\n", nR1, RecycleLeft("rcyB"));
	nRes = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle take1 res=%d count=%d left=%d item_ok=%d items=%d first_ok=%d stock0=%d daily_left=%d\n", nRes, s_RecycleLast.byCount, s_RecycleLast.byLeft,
		(int)(s_RecycleLast.wItemID == aItem[0]), RCYCOUNT(pB), (int)(pB->waItem[0] == aItem[0]), RecycleStockCount(dwBox, aItem[0]), RecycleLeft("rcyB"));
	/* 手持ちがいっぱいなら断る（今日の分は減らない） */
	for (int i = 0; i < MAX_MYITEM; i ++) pB->waItem[i] = aItem[20];
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle take full=%d items=%d stock0=%d left=%d\n", nR1, RCYCOUNT(pB), RecycleStockCount(dwBox, aItem[0]), RecycleLeft("rcyB"));
	ZeroMemory(pB->waItem, sizeof(pB->waItem));
	/* 0 個になると種類ごと消える */
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle take2 res=%d stock0=%d left=%d\n", nR1, RecycleStockCount(dwBox, aItem[0]), s_RecycleLast.byLeft);
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle take3 res=%d stock0=%d kinds=%d rows=%d left=%d items=%d\n", nR1, RecycleStockCount(dwBox, aItem[0]), RecycleKindCount(dwBox),
		(int)g_aRecycleStock.size(), s_RecycleLast.byLeft, RCYCOUNT(pB));
	/* 1 日 3 個まで（もらうたびに 3 個目で使い切る）。4 個目は断る。箱の中身は減らない */
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
	printf("recycle take empty-item=%d\n", nR1);
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[2], 0);
	printf("recycle take4 res=%d stock2=%d left=%d items=%d (daily limit)\n", nR1, RecycleStockCount(dwBox, aItem[2]), s_RecycleLast.byLeft, RCYCOUNT(pB));
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, NULL);
	printf("recycle list b after limit kinds=%d left=%d\n", nR1, nLeft);
	/* アカウントごと：別のアカウントはまだもらえる。自分が入れた品ももらえる（数に数える） */
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLETAKE, aItem[2], 0);
	printf("recycle take own res=%d left_a=%d left_b=%d\n", nR2, RecycleLeft("rcyA"), RecycleLeft("rcyB"));
	RCYHAND(pA, aItem[4], aItem[4]);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[4], 2);
	printf("recycle put for next day res=%d stock4=%d\n", nR1, RecycleStockCount(dwBox, aItem[4]));
	/* 日本時間の 0 時になると戻る（23:59 → 0:00 ちょうど）。別の日の分は残らない */
	g_dwRecycleNowOverride = dwJstEnd - 1;
	printf("recycle before midnight left_b=%d\n", RecycleLeft("rcyB"));
	g_dwRecycleNowOverride = dwJstEnd;
	printf("recycle after midnight left_b=%d left_a=%d\n", RecycleLeft("rcyB"), RecycleLeft("rcyA"));
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, NULL);
	nRes = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[4], 0);
	printf("recycle next day list_left=%d take=%d left=%d items=%d stock4=%d\n", nLeft, nRes, s_RecycleLast.byLeft, RCYCOUNT(pB), RecycleStockCount(dwBox, aItem[4]));

	/* 20 種類まで・同じ品は 255 個まで（箱を空にして、時刻を戻して試す） */
	g_dwRecycleNowOverride = dwJstEnd - 60;
	RecycleClear();
	for (int i = 0; i < 20; i ++) pA->waItem[i] = aItem[i];
	int nOk = 0;
	for (int i = 0; i < 20; i ++) {
		/* 先頭の品を 1 個ずつ入れる（入れるたびに手持ちが詰まる） */
		if (RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, pA->waItem[0], 1) == RECYCLERES_OK) nOk ++;
	}
	printf("recycle kinds filled ok=%d kinds=%d items=%d\n", nOk, RecycleKindCount(dwBox), RCYCOUNT(pA));
	RCYHAND(pA, aItem[20], aItem[0], aItem[0]);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[20], 1);
	printf("recycle kinds 21st res=%d kinds=%d items=%d\n", nR1, RecycleKindCount(dwBox), RCYCOUNT(pA));
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 2);
	printf("recycle kinds merge-when-full res=%d stock0=%d kinds=%d\n", nR2, RecycleStockCount(dwBox, aItem[0]), RecycleKindCount(dwBox));
	nR1 = RecycleAskList(pB, nConnB, aE, &nLeft, &nSize);
	printf("recycle list full kinds=%d size_ok=%d\n", nR1, (int)(nSize == offsetof(CMDPARA_RECYCLELIST, aEntry) + sizeof(RECYCLEENTRY) * 20));
	/* 20 種類の 1 つがなくなれば、新しい種類が入る */
	nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[19], 0);
	RCYHAND(pA, aItem[20]);
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[20], 1);
	printf("recycle kinds after take take=%d put21=%d kinds=%d\n", nR1, nR2, RecycleKindCount(dwBox));
	/* 255 個まで */
	for (size_t i = 0; i < g_aRecycleStock.size(); i ++) {
		if (g_aRecycleStock[i].wItemID == aItem[0]) g_aRecycleStock[i].byCount = 250;
	}
	RCYHAND(pA, aItem[0], aItem[0], aItem[0], aItem[0], aItem[0], aItem[0], aItem[0]);
	nR1 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 6);
	printf("recycle count 250+6=%d stock0=%d items=%d\n", nR1, RecycleStockCount(dwBox, aItem[0]), RCYCOUNT(pA));
	nR2 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 5);
	nR3 = RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
	printf("recycle count 250+5=%d stock0=%d then+1=%d items=%d\n", nR2, RecycleStockCount(dwBox, aItem[0]), nR3, RCYCOUNT(pA));
	nRes = RecycleAskList(pB, nConnB, aE, &nLeft, NULL);
	printf("recycle list 255 first_count=%d\n", aE.empty() ? -1 : (int)aE[0].byCount);

	/* 保存して読み直す：中身・今日もらった数・記録 */
	{
		RecycleClear();
		RCYHAND(pA, aItem[0], aItem[0], aItem[1], aItem[2]);
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 2);
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[1], 1);
		pA->wRecycleBox = dwBox2;
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[2], 1);
		pA->wRecycleBox = dwBox;
		ZeroMemory(pB->waItem, sizeof(pB->waItem));
		RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
		/* 古い日付の「もらった数」は保存しない（0 個と同じ） */
		RECYCLEDAILY Old;
		ZeroMemory(&Old, sizeof(Old));
		strcpy(Old.szAcc, "rcyold"); Old.dwDay = dwDay0 - 3; Old.byCount = 3;
		g_aRecycleDaily.push_back(Old);
		std::vector<RECYCLESTOCK> aStockBefore = g_aRecycleStock;
		std::vector<RECYCLELOG> aLogBefore = g_aRecycleLog;
		BOOL bSaved = SaveRecycle();
		RecycleClear();
		BOOL bLoaded = LoadRecycle();
		BOOL bSame = (aStockBefore.size() == g_aRecycleStock.size()) && (aLogBefore.size() == g_aRecycleLog.size());
		for (size_t i = 0; bSame && i < aStockBefore.size(); i ++) bSame = (memcmp(&aStockBefore[i], &g_aRecycleStock[i], sizeof(RECYCLESTOCK)) == 0);
		for (size_t i = 0; bSame && i < aLogBefore.size(); i ++) bSame = (memcmp(&aLogBefore[i], &g_aRecycleLog[i], sizeof(RECYCLELOG)) == 0);
		printf("recycle save=%d load=%d same=%d stock=%d log=%d daily_rows=%d left_b=%d left_a=%d left_old=%d\n", (int)bSaved, (int)bLoaded, (int)bSame,
			(int)g_aRecycleStock.size(), (int)g_aRecycleLog.size(), (int)g_aRecycleDaily.size(), RecycleLeft("rcyB"), RecycleLeft("rcyA"), RecycleLeft("rcyold"));
		/* 読み直した後も続けて使える（もらうと減る・次の日に戻る） */
		ZeroMemory(pB->waItem, sizeof(pB->waItem));
		nR1 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
		nR2 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[1], 0);
		nR3 = RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[1], 0);
		printf("recycle after load take1=%d take2=%d take3=%d stock0=%d stock1=%d left_b=%d\n", nR1, nR2, nR3,
			RecycleStockCount(dwBox, aItem[0]), RecycleStockCount(dwBox, aItem[1]), RecycleLeft("rcyB"));

		/* 記録：誰が・いつ・どの箱で・何を何個（時刻・アカウント・キャラ・箱・操作・品・個数） */
		BOOL bLog = (aLogBefore.size() == 4);
		for (size_t i = 0; i < aLogBefore.size(); i ++) {
			const RECYCLELOG &L = aLogBefore[i];
			printf("recycle log %d t=%d acc=%s name=%s box=%u kind=%d item_ok=%d count=%d\n", (int)i, (int)((int)L.dwTime - (int)(dwJstEnd - 60)), L.szAcc, L.szName, L.dwBoxNo,
				(int)L.byKind, (int)(L.wItemID == aItem[0] || L.wItemID == aItem[1] || L.wItemID == aItem[2]), (int)L.byCount);
		}
		printf("recycle log count_ok=%d\n", (int)bLog);

		/* 記録は新しい 1000 件まで（古いほうから捨てる）。保存して読み直しても 1000 件 */
		RecycleClear();
		for (int i = 0; i < RECYCLE_LOGMAX + 5; i ++) {
			RECYCLELOG L;
			ZeroMemory(&L, sizeof(L));
			L.dwTime = dwJstEnd - 60 + (DWORD)i;
			strcpy(L.szAcc, "rcylog"); strcpy(L.szName, "記録");
			L.dwBoxNo = dwBox; L.byKind = (BYTE)(i % 2); L.wItemID = aItem[0]; L.byCount = (BYTE)(1 + i % 3);
			g_aRecycleLog.push_back(L);
		}
		RecycleTrimLog();
		printf("recycle log trim size=%d first_t=%d last_t=%d\n", (int)g_aRecycleLog.size(), (int)(g_aRecycleLog.front().dwTime - (dwJstEnd - 60)), (int)(g_aRecycleLog.back().dwTime - (dwJstEnd - 60)));
		RCYHAND(pA, aItem[0]);
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 1);
		printf("recycle log after put size=%d first_t=%d last_acc=%s\n", (int)g_aRecycleLog.size(), (int)(g_aRecycleLog.front().dwTime - (dwJstEnd - 60)), g_aRecycleLog.back().szAcc);
		aLogBefore = g_aRecycleLog;
		bSaved = SaveRecycle();
		RecycleClear();
		bLoaded = LoadRecycle();
		bSame = (aLogBefore.size() == g_aRecycleLog.size());
		for (size_t i = 0; bSame && i < aLogBefore.size(); i ++) bSame = (memcmp(&aLogBefore[i], &g_aRecycleLog[i], sizeof(RECYCLELOG)) == 0);
		printf("recycle log save=%d load=%d same=%d size=%d\n", (int)bSaved, (int)bLoaded, (int)bSame, (int)g_aRecycleLog.size());
	}

	/* データ API（管理者が読むだけの一覧）：箱ごとの中身・記録（新しい順）・今日もらった数・箱の NPC がいない中身 */
	{
		RecycleClear();
		RCYHAND(pA, aItem[0], aItem[0], aItem[1]);
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[0], 2);
		RecycleAsk(pA, nConnA, CMD_RECYCLEPUT, aItem[1], 1);
		ZeroMemory(pB->waItem, sizeof(pB->waItem));
		g_dwRecycleNowOverride = dwJstEnd - 30;
		RecycleAsk(pB, nConnB, CMD_RECYCLETAKE, aItem[0], 0);
		RECYCLESTOCK Lost;
		ZeroMemory(&Lost, sizeof(Lost));
		Lost.dwBoxNo = 999; Lost.wItemID = aItem[3]; Lost.byCount = 4;
		g_aRecycleStock.push_back(Lost);
		std::string s = SboWebDataRecycle();
		#define RCYHAS(sz) (int)(s.find(sz) != std::string::npos)
		char szTmp[256];
		snprintf(szTmp, sizeof(szTmp), "{\"boxNo\":%u,\"mapNo\":%d,", (unsigned)dwBox, nMap);
		int nBox = RCYHAS(szTmp);
		snprintf(szTmp, sizeof(szTmp), "\"log\":[{\"time\":%u,", dwJstEnd - 30);
		int nStockEnd = RCYHAS(szTmp);
		printf("recycle api box=%d stock_then_log_newest_first=%d take=%d put=%d\n", nBox, nStockEnd, RCYHAS("\"action\":\"take\""), RCYHAS("\"action\":\"put\""));
		printf("recycle api date=%d acc=%d name=%d missing=%d daily=%d limits=%d\n", RCYHAS("\"date\":\"2026-11-17 23:59\""), RCYHAS("\"acc\":\"rcyA\""), RCYHAS("\"name\":\"入れ手\""),
			RCYHAS("{\"boxNo\":999,\"mapNo\":-1,"), RCYHAS("\"daily\":[{\"acc\":\"rcyB\",\"count\":1,\"left\":2}]"),
			RCYHAS("\"limits\":{\"maxKind\":20,\"maxCount\":255,\"dayLimit\":3,\"logMax\":1000,\"range\":3,\"maxBox\":1024}"));
		printf("recycle api kinds=%d missing_true=%d box_missing_false=%d\n", RCYHAS("\"kinds\":2,\"total\":2"), RCYHAS("\"missing\":true,\"stock\":[{\"itemId\":"), RCYHAS("\"missing\":false"));
		/* 箱の一覧は箱番号の順・マップ・座標・メモつき（メモは行に書いた分） */
		strcpy(pMap->aSwap[1].szMemo, "テスト箱ふたつめ");
		s = SboWebDataRecycle();
		snprintf(szTmp, sizeof(szTmp), "{\"boxNo\":%u,\"mapNo\":%d,\"mapName\":", (unsigned)dwBox, nMap);
		size_t nPos1 = s.find(szTmp);
		snprintf(szTmp, sizeof(szTmp), "{\"boxNo\":%u,\"mapNo\":%d,\"mapName\":", (unsigned)dwBox2, nMap);
		size_t nPos2 = s.find(szTmp);
		snprintf(szTmp, sizeof(szTmp), "\"x\":%d,\"y\":%d,\"wParts\":%d,\"memo\":\"テスト箱ふたつめ\"", xBox2, yBox, (int)wOpen2);
		printf("recycle api boxes order=%d (box1 before box2) both=%d xy_memo=%d\n", (int)(nPos1 != std::string::npos && nPos2 != std::string::npos && nPos1 < nPos2),
			(int)(nPos1 != std::string::npos && nPos2 != std::string::npos), RCYHAS(szTmp));
		pMap->aSwap[1].szMemo[0] = 0;
		#undef RCYHAS
	}

	/* データ API（map の swaps の kind 2）：番号は自動で振る（全マップ通し・消した番号は使い回さない）・範囲と型の確かめ・書いて読んで同じ・調べると開く */
	{
		pMap->bySwapCount = MapBak.bySwapCount;
		memcpy(pMap->aSwap, MapBak.aSwap, sizeof(pMap->aSwap));
		pMap->wBoxHigh = MapBak.wBoxHigh;
		pA->wRecycleBox = pB->wRecycleBox = 0;
		RecycleClear();
		int nHigh = RecycleBoxHighAllForTest();
		char szJ[1024], szBad[1024];
		snprintf(szJ, sizeof(szJ),
			"{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"szMemo\":\"港の広場\"},{\"kind\":\"recycle\",\"x\":%d,\"y\":%d,\"wParts\":%d}]}",
			xBox1, yBox, (int)wOpen1, xBox2, yBox, (int)wOpen2);
		BOOL bOk = SwapOk(nMap, szJ);
		WORD wNo1 = pMap->aSwap[0].wBoxNo, wNo2 = pMap->aSwap[1].wBoxNo;
		printf("recycle swaps api ok=%d count=%d kinds=%d,%d no_auto=%d,%d high=%d memo=%s memo2_empty=%d\n", (int)bOk, (int)pMap->bySwapCount,
			(int)pMap->aSwap[0].byKind, (int)pMap->aSwap[1].byKind, (int)(wNo1 == nHigh + 1), (int)(wNo2 == nHigh + 2),
			(int)(pMap->wBoxHigh == nHigh + 2), pMap->aSwap[0].szMemo, (int)(pMap->aSwap[1].szMemo[0] == 0));
		/* 取り出すと同じ形で出る・読んで書いて同じになる */
		std::string strGet = SboWebDataGet("map", nMap);
		char szTmp[256];
		snprintf(szTmp, sizeof(szTmp), "{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wBoxNo\":%d,\"szMemo\":\"港の広場\"}", xBox1, yBox, (int)wOpen1, (int)wNo1);
		printf("recycle swaps api get has=%d\n", (int)(strGet.find(szTmp) != std::string::npos));
		{
			size_t nS = strGet.find("\"swaps\":"), nE = strGet.find(",\"events\":");
			std::string strBody = "{" + ((nS != std::string::npos && nE != std::string::npos) ? strGet.substr(nS, nE - nS) : std::string()) + "}";
			MAPINFO Before = *pMap;
			BOOL bRound = SwapOk(nMap, strBody.c_str());
			printf("recycle swaps api roundtrip ok=%d same=%d\n", (int)bRound,
				(int)(memcmp(Before.aSwap, pMap->aSwap, sizeof(Before.aSwap)) == 0 && Before.bySwapCount == pMap->bySwapCount && Before.wBoxHigh == pMap->wBoxHigh));
		}
		/* 調べると、書いた番号の箱が開く */
		MimicExamine(pA);
		printf("recycle swaps examine box_is_no1=%d\n", (int)(pA->wRecycleBox == wNo1));
		RecycleAskClose(pA);
		/* 拒否（どれも書き込まれず、行は変わらない） */
		MAPINFO Keep = *pMap;
		int nBad = 0;
		const char *apszBad[] = {
			"\"kind\":2,\"wFlag\":3",											/* 印は使わない */
			"\"kind\":2,\"szMsg\":\"あ\"",										/* 調べた時の文は無い */
			"\"kind\":2,\"wGiveItem\":1",										/* 渡す品は無い */
			"\"kind\":2,\"wBoxNo\":0",											/* 番号は 1 から */
			"\"kind\":2,\"wBoxNo\":1025",										/* 番号は 1024 まで */
			"\"kind\":3",														/* 知らない種類 */
			"\"kind\":1,\"wBoxNo\":5",											/* 宝箱の行に箱番号 */
			"\"kind\":0,\"wFlag\":1,\"wBoxNo\":5",								/* 印の行に箱番号 */
			"\"kind\":2,\"bogus\":1",											/* 知らない項目 */
			"\"kind\":2,\"szMemo\":\"01234567890123456789012345678901234567890123456789012345678901234567890123456789\"",	/* メモが長すぎる */
		};
		for (size_t k = 0; k < sizeof(apszBad) / sizeof(apszBad[0]); k ++) {
			snprintf(szBad, sizeof(szBad), "{\"swaps\":[{%s,\"x\":%d,\"y\":%d,\"wParts\":%d}]}", apszBad[k], xBox1, yBox, (int)wOpen1);
			if (SwapOk(nMap, szBad)) nBad |= (1 << k);
		}
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d}]}", xBox1, yBox, (int)g_wCountMapParts);
		if (SwapOk(nMap, szBad)) nBad |= (1 << 20);
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d}]}", pMap->nCX, yBox, (int)wOpen1);
		if (SwapOk(nMap, szBad)) nBad |= (1 << 21);
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wBoxNo\":7},{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wBoxNo\":7}]}",
			xBox1, yBox, (int)wOpen1, xBox2, yBox, (int)wOpen2);
		if (SwapOk(nMap, szBad)) nBad |= (1 << 22);
		printf("recycle swaps api rejected_mask=%d unchanged=%d\n", nBad, (int)(memcmp(Keep.aSwap, pMap->aSwap, sizeof(Keep.aSwap)) == 0 && Keep.bySwapCount == pMap->bySwapCount));
		/* ほかのマップが使っている番号は指定できない。自分の番号の指定し直しは良い。消した番号は自動では使い回さない */
		if (g_aMapInfo.GetSize() >= 2) {
			int nOther = (nMap == 0) ? 1 : 0;
			PMAPINFO pOtherMap = g_aMapInfo[nOther];
			MAPINFO OtherBak = *pOtherMap;
			snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":0,\"y\":0,\"wParts\":%d,\"wBoxNo\":%d}]}", (int)wOpen1, (int)wNo1);
			BOOL bDup = SwapOk(nOther, szBad);
			snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wBoxNo\":%d}]}", xBox1, yBox, (int)wOpen1, (int)wNo1);
			BOOL bSelf = SwapOk(nMap, szBad);
			printf("recycle swaps api dup other map ok=%d rewrite own no ok=%d\n", (int)bDup, (int)bSelf);
			/* 消した番号（箱 2 の番号）は使い回さない。新しい箱は消した番号の次の番号 */
			snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wBoxNo\":%d},{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d}]}",
				xBox1, yBox, (int)wOpen1, (int)wNo1, xBox2, yBox, (int)wOpen2);
			BOOL bNew = SwapOk(nMap, szBad);
			printf("recycle swaps api no reuse ok=%d new_no=%d (not %d) high=%d\n", (int)bNew, (int)pMap->aSwap[1].wBoxNo - nHigh, (int)wNo2 - nHigh, (int)pMap->wBoxHigh - nHigh);
			std::string sR = SboWebDataRecycle();
			snprintf(szTmp, sizeof(szTmp), "\"retired\":[%d],\"nextNo\":%d,\"max\":1024", (int)wNo2, (int)pMap->wBoxHigh + 1);
			printf("recycle api retired=%d\n", (int)(sR.find(szTmp) != std::string::npos));
			/* ほかのマップに足すと、全マップ通しで次の番号 */
			snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":2,\"x\":0,\"y\":0,\"wParts\":%d}]}", (int)wOpen1);
			BOOL bAuto = SwapOk(nOther, szBad);
			printf("recycle swaps api other map auto ok=%d no_is_next=%d\n", (int)bAuto, (int)(pOtherMap->aSwap[0].wBoxNo == pMap->wBoxHigh + 1));
			SwapRestore(pOtherMap, OtherBak);
		}
		/* 箱を消すと、開いていた人の窓の絵は戻る（開いた記憶も捨てる） */
		pA->wRecycleBox = (WORD)pMap->aSwap[0].wBoxNo;
		SwapOk(nMap, "{\"swaps\":[]}");
		MapSwapSendMap(pMap);
		printf("recycle swaps api delete clears box=%d count=%d\n", (int)pA->wRecycleBox, (int)pMap->bySwapCount);
	}

	/* 片付け */
	g_dwRecycleNowOverride = 0;
	MockTrace(0);
	RecycleClear();
	SaveRecycle();
	/* マップの行を元に戻す（API の書き込みでマップ名などのポインタは作り直されるので、構造体ごとは戻さない） */
	pMap->bySwapCount = MapBak.bySwapCount;
	memcpy(pMap->aSwap, MapBak.aSwap, sizeof(pMap->aSwap));
	pMap->wChestHigh = MapBak.wChestHigh;
	pMap->wBoxHigh = MapBak.wBoxHigh;
	pA->wRecycleBox = wRecycleBoxBack[0];
	pB->wRecycleBox = wRecycleBoxBack[1];
	g_aItemInfo[wImportant]->byFlg = byImportantFlgBack;
	g_aQuestInfo.RemoveAll();
	delete pQuestTmp;
	for (size_t i = 0; i < aQuestBack.size(); i ++) g_aQuestInfo.Add(aQuestBack[i]);
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apPC[i];
		memcpy(p->waItem, awItemBack[i], sizeof(awItemBack[i]));
		memcpy(p->waEqu, awEquBack[i], sizeof(awEquBack[i]));
		p->byAdminLV = byAdminBack[i];
		memcpy(p->szAcc, szAccBack[i], sizeof(szAccBack[i]));
		memcpy(p->szName, szNameBack[i], sizeof(szNameBack[i]));
		QuestMovePC(p, (int)dwMapBack[i], (int)dwXBack[i], (int)dwYBack[i]);
	}
	#undef RCYCOUNT
	#undef RCYHAND
	printf("recycle end\n");
}

/* ------------------------------------------------------------------------- */
/* O. 押せるNPCを押す向き													 */
/*    サーバーに前の向きが残っていても、押した人のいる側から押した方へ動く	 */
/*    (歩きながら曲がってそのまま押すと、向きの通知より先に押して要求が届く)	 */
/* ------------------------------------------------------------------------- */
static BOOL PushIsFree(PMAPINFO pMap, int x, int y)
{
	if (x < 0 || y < 0 || x >= pMap->nCX || y >= pMap->nCY) {
		return FALSE;
	}
	if ((&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType != PARTSHIT_NONE) {
		return FALSE;
	}
	return (CheckCharBlockNPC(pMap->dwID, x, y) == (DWORD)-1);
}

/* ------------------------------------------------------------------------- */
/* V. ダメージ床（パーツの byDamage・byPoison）								 */
/*    1歩入るとHPが減るか・向きを変えただけでは減らないか・HPは1で止まるか	 */
/*    毒になる床で毒になるか・もう毒ならかけ直さないか						 */
/* ------------------------------------------------------------------------- */
static void FloorStep(PCHARINFO pPC, int x, int y, BYTE byDir)
{
	CMDPARA_CHARPOS Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_CHARPOS;
	Para.pID = pPC->pID;
	Para.dwTblID = pPC->dwTblID;
	Para.dwMapID = pPC->dwMapID;
	Para.byX = (BYTE)x;
	Para.byY = (BYTE)y;
	Para.byDirection = byDir;
	TestRecv(pPC, &Para, sizeof(Para));
}

static void TestFloorDamage(const std::vector<int> &aLogin)
{
	printf("== V. FloorDamage ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	/* プレイヤーのいるマップで、横に2マス続けて歩ける所を探す */
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	int nX = -1, nY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 1 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	if (nX < 0) {
		printf("no floor\n");
		return;
	}
	PMAPPARTSINFO pParts = &g_aMapPartsInfo[pMap->pwMap[(nX + 1) + nY * pMap->nCX]];
	BYTE byDamageBack = pParts->byDamage, byPoisonBack = pParts->byPoison;
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	WORD wHPBack = pPC->wHP;
	/* 左のマスと同じパーツなら、左のマスもダメージ床になるので、立つ所は直接置く */
	pParts->byDamage = 5;
	pParts->byPoison = 0;

	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)nX;
	pPC->dwY = pPC->dwYBack = (DWORD)nY;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->wHP = 20;
	FloorStep(pPC, nX + 1, nY, 3);
	printf("floor step hp=%d\n", pPC->wHP);
	/* その場で向きを変えただけ（同じマスの CHARPOS）では減らない */
	FloorStep(pPC, nX + 1, nY, 0);
	printf("floor turn hp=%d\n", pPC->wHP);
	/* HPは1で止まる */
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)nX;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->wHP = 3;
	FloorStep(pPC, nX + 1, nY, 3);
	printf("floor low hp=%d\n", pPC->wHP);
	/* 毒になる床 */
	pParts->byPoison = 10;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)nX;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->wHP = 50;
	FloorStep(pPC, nX + 1, nY, 3);
	int nPoison = CheckCharInfoTimer(pPC->dwTblID, CHARINFOTIMEREVENT_POISON);
	DWORD dwStart = 0;
	for (int i = 0; i < pPC->aTimerEvent.GetSize(); i ++) {
		if (pPC->aTimerEvent[i]->byType == CHARINFOTIMEREVENT_POISON) {
			dwStart = pPC->aTimerEvent[i]->dwTimeStart;
		}
	}
	printf("floor poison hp=%d poison=%d\n", pPC->wHP, nPoison);
	/* もう毒ならかけ直さない（毒の始まりの時刻が変わらない） */
	s_dwFakeNow += 500;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)nX;
	AddTblNoPC((int)pPC->dwTblID);
	FloorStep(pPC, nX + 1, nY, 3);
	DWORD dwStart2 = 0;
	for (int i = 0; i < pPC->aTimerEvent.GetSize(); i ++) {
		if (pPC->aTimerEvent[i]->byType == CHARINFOTIMEREVENT_POISON) {
			dwStart2 = pPC->aTimerEvent[i]->dwTimeStart;
		}
	}
	printf("floor poison again hp=%d same_start=%d\n", pPC->wHP, (int)(dwStart == dwStart2));

	/* 敵（戦闘1・戦闘2）と傭兵も踏む（HPは1で止まる）。村人など、ほかのNPCは踏まない
	   （NPCは歩いた時に TimerProcChar から呼ばれる。ここでは種類だけ変えて直接呼ぶ） */
	{
		static const BYTE abyType[] = {NPCTYPE_BATTLE1, NPCTYPE_BATTLE2, NPCTYPE_MERCENARY, NPCTYPE_MOVERAND};
		BYTE byNpcTypeBack = pPC->byNpcType;
		pParts->byPoison = 0;
		DeleteTblNoPC((int)pPC->dwTblID);
		pPC->dwX = pPC->dwXBack = (DWORD)(nX + 1);
		AddTblNoPC((int)pPC->dwTblID);
		for (size_t i = 0; i < sizeof(abyType) / sizeof(abyType[0]); i ++) {
			pPC->byNpcType = abyType[i];
			pPC->wHP = 20;
			FloorDamageOnStep(pPC);
			int nHP1 = pPC->wHP;
			pPC->wHP = 3;
			FloorDamageOnStep(pPC);
			printf("floor npc type=%d hp=%d low=%d\n", (int)abyType[i], nHP1, (int)pPC->wHP);
		}
		pPC->byNpcType = byNpcTypeBack;
	}

	/* 敵・傭兵は自分からはダメージ床に入らない（もう上にいる時は出られるように、よけない）。
	   戦闘２の1歩の判定でも、ダメージ床をなくせば入れる */
	{
		WORD wDmg = pMap->pwMap[(nX + 1) + nY * pMap->nCX];
		BYTE byNpcTypeBack = pPC->byNpcType;
		printf("floor avoid on=%d", IsAvoidDamageFloor(pPC, pMap, nX + 1, nY));
		if (pMap->pwMap[nX + nY * pMap->nCX] != wDmg) {
			DeleteTblNoPC((int)pPC->dwTblID);
			pPC->dwX = pPC->dwXBack = (DWORD)nX;
			AddTblNoPC((int)pPC->dwTblID);
			printf(" off=%d", IsAvoidDamageFloor(pPC, pMap, nX + 1, nY));
			CTestSearch Proc;
			Proc.Create(pPC);
			pPC->byNpcType = NPCTYPE_BATTLE2;
			int nStep1 = Proc.CanStep(NULL, nX + 1, nY);
			pParts->byDamage = 0;
			int nStep2 = Proc.CanStep(NULL, nX + 1, nY);
			pParts->byDamage = 5;
			pPC->byNpcType = byNpcTypeBack;
			printf(" search_step=%d nodamage_step=%d", nStep1, nStep2);
		}
		printf("\n");
	}

	/* ダメージ床が効かない敵（ステータステンプレートの byNoFloorDamage）は、
	   よけずに素通りし、HPも減らない。発生元から出た敵（テンプレートは親が持つ）も同じ */
	{
		BYTE byNpcTypeBack = pPC->byNpcType;
		DWORD dwStTmpBack = pPC->dwStTmplateID;
		PSTATUSTEMPLATEINFO pTmp = new STATUSTEMPLATEINFO;
		ZeroMemory(pTmp, sizeof(STATUSTEMPLATEINFO));
		pTmp->byNoFloorDamage = 1;
		int nTmp = g_aStTmpInfo.GetSize();
		g_aStTmpInfo.Add(pTmp);
		DeleteTblNoPC((int)pPC->dwTblID);
		pPC->dwX = pPC->dwXBack = (DWORD)nX;
		AddTblNoPC((int)pPC->dwTblID);
		pPC->byNpcType = NPCTYPE_BATTLE2;
		pPC->dwStTmplateID = (DWORD)nTmp;
		int nAvoid = IsAvoidDamageFloor(pPC, pMap, nX + 1, nY);
		DeleteTblNoPC((int)pPC->dwTblID);
		pPC->dwX = pPC->dwXBack = (DWORD)(nX + 1);
		AddTblNoPC((int)pPC->dwTblID);
		pPC->wHP = 20;
		FloorDamageOnStep(pPC);
		int nHP1 = pPC->wHP;
		pTmp->byNoFloorDamage = 0;
		FloorDamageOnStep(pPC);
		printf("floor immune avoid=%d hp=%d normal_hp=%d\n", nAvoid, nHP1, (int)pPC->wHP);
		pPC->dwStTmplateID = dwStTmpBack;
		pPC->byNpcType = byNpcTypeBack;
		g_aStTmpInfo.RemoveAt(nTmp);
		delete pTmp;
	}

	/* 片付け */
	for (int i = pPC->aTimerEvent.GetSize() - 1; i >= 0; i --) {
		if (pPC->aTimerEvent[i]->byType == CHARINFOTIMEREVENT_POISON) {
			delete pPC->aTimerEvent[i];
			pPC->aTimerEvent.RemoveAt(i);
		}
	}
	pParts->byDamage = byDamageBack;
	pParts->byPoison = byPoisonBack;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->byDirection = byDirBack;
	pPC->wHP = wHPBack;
}

/* ------------------------------------------------------------------------- */
/* 移動の見張り：壁へは入れない・速すぎる歩きは戻される・ふつうの速さは通る	 */
/*    倒れている間と取り引き中は歩けない・サーバーが動かした後は戻されない	 */
/* ------------------------------------------------------------------------- */
static void MoveGuardPlace(PCHARINFO pPC, int x, int y)
{
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)x;
	pPC->dwY = pPC->dwYBack = (DWORD)y;
	AddTblNoPC((int)pPC->dwTblID);
}

static void TestMoveGuard(const std::vector<int> &aLogin)
{
	printf("== MoveGuard ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection, byAdminBack = pPC->byAdminLV;
	WORD wHPBack = pPC->wHP;
	pPC->byAdminLV = ADMINLV_USER;

	/* 横に 8 マス続けて歩ける所（歩ける: ぶつからない・NPC がいない）と、その隣のぶつかるマス */
	int nX = -1, nY = -1, nWallX = -1, nWallY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 8 < pMap->nCX; x ++) {
			int i;
			for (i = 0; i < 8; i ++) {
				if (!PushIsFree(pMap, x + i, y)) {
					break;
				}
			}
			if (i == 8) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	for (int y = 0; y < pMap->nCY && nWallX < 0; y ++) {
		for (int x = 1; x < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x - 1, y) && !CheckCharBlock(pMap->dwID, x, y) && (CheckCharBlockNPC(pMap->dwID, x, y) == (DWORD)-1)) {
				BOOL bDoor = FALSE;
				for (int e = 0; e < pMap->nEventCount; e ++) {
					if ((pMap->pEvent[e].dwX == (DWORD)x) && (pMap->pEvent[e].dwY == (DWORD)y)) {
						bDoor = TRUE;
					}
				}
				if (!bDoor) {
					nWallX = x;
					nWallY = y;
					break;
				}
			}
		}
	}
	if (nX < 0) {
		printf("no floor\n");
		pPC->byAdminLV = byAdminBack;
		return;
	}

	/* 壁へは入れない（位置が変わらない） */
	if (nWallX >= 0) {
		MoveGuardPlace(pPC, nWallX - 1, nWallY);
		s_dwFakeNow += 10000;
		FloorStep(pPC, nWallX, nWallY, 3);
		printf("moveguard wall stay=%d\n", (int)(pPC->dwX == (DWORD)(nWallX - 1)));
	} else {
		printf("moveguard wall none\n");
	}

	/* ふつうの速さ（280ms ごと）で 7 歩歩ける */
	MoveGuardPlace(pPC, nX, nY);
	int nWalked = 0;
	for (int i = 1; i <= 7; i ++) {
		s_dwFakeNow += 280;
		FloorStep(pPC, nX + i, nY, 3);
		if (pPC->dwX == (DWORD)(nX + i)) {
			nWalked ++;
		}
	}
	printf("moveguard normal walked=%d\n", nWalked);

	/* 間隔なしで続けて送る：ラグ分の数歩は通るが、すぐ戻される */
	MoveGuardPlace(pPC, nX, nY);
	s_dwFakeNow += 10000;
	nWalked = 0;
	for (int i = 1; i <= 8; i ++) {
		FloorStep(pPC, nX + i, nY, 3);
		if (pPC->dwX == (DWORD)(nX + i)) {
			nWalked ++;
		} else {
			break;
		}
	}
	printf("moveguard burst walked=%d x=%d\n", nWalked, (int)(pPC->dwX - (DWORD)nX));
	/* 戻された後、少し待てばまた歩ける */
	s_dwFakeNow += 300;
	FloorStep(pPC, pPC->dwX + 1, nY, 3);
	printf("moveguard after wait x=%d\n", (int)(pPC->dwX - (DWORD)nX));

	/* 管理者は見ない */
	MoveGuardPlace(pPC, nX, nY);
	pPC->byAdminLV = ADMINLV_USER + 1;
	for (int i = 1; i <= 8; i ++) {
		FloorStep(pPC, nX + (i % 2), nY, 3);
	}
	FloorStep(pPC, nX + 1, nY, 3);
	FloorStep(pPC, nX + 2, nY, 3);
	FloorStep(pPC, nX + 3, nY, 3);
	FloorStep(pPC, nX + 4, nY, 3);
	FloorStep(pPC, nX + 5, nY, 3);
	printf("moveguard admin x=%d\n", (int)(pPC->dwX - (DWORD)nX));
	pPC->byAdminLV = ADMINLV_USER;

	/* 一度に 3 マスは今までどおり戻される */
	MoveGuardPlace(pPC, nX, nY);
	s_dwFakeNow += 10000;
	FloorStep(pPC, nX + 3, nY, 3);
	printf("moveguard far x=%d\n", (int)(pPC->dwX - (DWORD)nX));

	/* 倒れている間は歩けない */
	MoveGuardPlace(pPC, nX, nY);
	s_dwFakeNow += 10000;
	pPC->wHP = 0;
	FloorStep(pPC, nX + 1, nY, 3);
	printf("moveguard dead x=%d\n", (int)(pPC->dwX - (DWORD)nX));
	pPC->wHP = 20;
	FloorStep(pPC, nX + 1, nY, 3);
	printf("moveguard alive x=%d\n", (int)(pPC->dwX - (DWORD)nX));

	/* サーバーが動かした後（マップ移動・押されるなど）は、続けて歩いても戻されない */
	MoveGuardPlace(pPC, nX, nY);
	s_dwFakeNow += 10000;
	for (int i = 1; i <= 4; i ++) {
		FloorStep(pPC, nX + i, nY, 3);
	}
	MoveGuardPlace(pPC, nX + 2, nY);
	nWalked = 0;
	for (int i = 1; i <= 3; i ++) {
		FloorStep(pPC, nX + 2 + i, nY, 3);
		if (pPC->dwX == (DWORD)(nX + 2 + i)) {
			nWalked ++;
		}
	}
	printf("moveguard servermove walked=%d\n", nWalked);

	/* 扉：閉まっている間は入れず、開けると入れる（扉の絵がぶつかる絵の時。扉のあるマップへ移して試す） */
	{
		DWORD dwMapBack = pPC->dwMapID;
		int nDoorX = -1, nDoorY = -1, nFromX = -1;
		PEVENTINFO_DOOR pDoor = NULL;
		PMAPINFO pMapDoor = NULL;
		for (int m = 0; m < g_aMapInfo.GetSize() && nDoorX < 0; m ++) {
			PMAPINFO pM = g_aMapInfo[m];
			for (int e = 0; e < pM->nEventCount && nDoorX < 0; e ++) {
				PEVENTINFO pEv = &pM->pEvent[e];
				if ((pEv->byType != EVENTTYPE_DOOR) || (pEv->pData == NULL) || (pEv->dwX == 0) || CheckCharBlock(pM->dwID, pEv->dwX, pEv->dwY)) {
					continue;
				}
				if (PushIsFree(pM, (int)pEv->dwX - 1, (int)pEv->dwY)) {
					pMapDoor = pM;
					nDoorX = (int)pEv->dwX;
					nDoorY = (int)pEv->dwY;
					nFromX = nDoorX - 1;
					pDoor = (PEVENTINFO_DOOR)pEv->pData;
				}
			}
		}
		if (nDoorX < 0) {
			printf("moveguard door none\n");
		} else {
			DWORD dwTimeOpenBack = pDoor->dwTimeOpen;
			DeleteTblNoPC((int)pPC->dwTblID);
			pPC->dwMapID = pMapDoor->dwID;
			AddTblNoPC((int)pPC->dwTblID);
			pDoor->dwTimeOpen = 0;
			MoveGuardPlace(pPC, nFromX, nDoorY);
			s_dwFakeNow += 10000;
			FloorStep(pPC, nDoorX, nDoorY, 3);
			printf("moveguard door closed stay=%d\n", (int)(pPC->dwX == (DWORD)nFromX));
			pDoor->dwTimeOpen = s_dwFakeNow;
			FloorStep(pPC, nDoorX, nDoorY, 3);
			printf("moveguard door open in=%d\n", (int)(pPC->dwX == (DWORD)nDoorX));
			pDoor->dwTimeOpen = dwTimeOpenBack;
			DeleteTblNoPC((int)pPC->dwTblID);
			pPC->dwMapID = dwMapBack;
			AddTblNoPC((int)pPC->dwTblID);
		}
	}

	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	AddTblNoPC((int)pPC->dwTblID);
	pPC->byDirection = byDirBack;
	pPC->byAdminLV = byAdminBack;
	pPC->wHP = wHPBack;
}

/* ------------------------------------------------------------------------- */
/* 低いパーツ（byLow）：ぶつかるが、特技・矢の見通しは越える。保存の形も確かめる	 */
/* ------------------------------------------------------------------------- */
static void TestLowParts(const std::vector<int> &aLogin)
{
	printf("== V2. LowParts ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	int nX = pMap->nCX / 2, nY = pMap->nCY / 2;
	PMAPPARTSINFO pParts = &g_aMapPartsInfo[pMap->pwMap[nX + nY * pMap->nCX]];
	BYTE byHitBack = pParts->byHitType, byLowBack = pParts->byLow;
	pParts->byHitType = PARTSHIT_BLOCK1;
	pParts->byLow = 0;
	BOOL bWall0 = IsMagicWall(pMap->dwID, nX, nY);
	pParts->byLow = 1;
	BOOL bWall1 = IsMagicWall(pMap->dwID, nX, nY);
	/* ぶつかり判定は変わらない */
	BOOL bBlock1 = !pMap->pwMap ? FALSE : (pParts->byHitType == PARTSHIT_BLOCK1);
	printf("low wall normal=%d low=%d hit=%d\n", bWall0 ? 1 : 0, bWall1 ? 1 : 0, bBlock1 ? 1 : 0);
	/* 保存用の byLight への入れ方（灯りと重ならない・今のデータはそのまま） */
	MAPPARTSINFOSAVE Save;
	ZeroMemory(&Save, sizeof(Save));
	pParts->byLight = 5;
	Save.byLight = MAPPARTS_MAKE_SAVELIGHT(pParts);
	printf("low save light=%d low=%d\n", MAPPARTS_SAVE_LIGHT(&Save), MAPPARTS_SAVE_LOW(&Save));
	Save.byLight = 3;
	printf("low old data light=%d low=%d\n", MAPPARTS_SAVE_LIGHT(&Save), MAPPARTS_SAVE_LOW(&Save));
	pParts->byLight = 0;
	pParts->byHitType = byHitBack;
	pParts->byLow = byLowBack;
}

/* ------------------------------------------------------------------------- */
/* W. 宝箱の敵（NPCTYPE_MIMIC）												 */
/*    ふだんはぶつかる・話しても変わらない・向きが違う所から調べても出ない。	 */
/*    目の前を調べる(EVENTCHECK byData=2)か叩くと戦闘２になり、調べた人の方を	 */
/*    向いてその人を狙う。目の前にいない所からは調べられない					 */
/* ------------------------------------------------------------------------- */
extern BOOL BattleProcAtack(PCHARINFO, PCHARINFO);
static PCHARINFO MimicMake(PCHARINFO pPC, int nX, int nY)
{
	PCHARINFO pM = new CHARINFO;
	ZeroMemory((PBYTE)pM, sizeof(CHARINFO));
	pM->byNpcType = NPCTYPE_MIMIC;
	SetInitCharData(pM);
	pM->byGameState = GAMESTATE_NPC;
	pM->dwMapID = pPC->dwMapID;
	pM->dwXBack = pM->dwX = (DWORD)nX;
	pM->dwYBack = pM->dwY = (DWORD)nY;
	pM->byDirection = 0;
	pM->wGrpID = MIMIC_GRPID_FIRST + 1;
	pM->wGrpMain = CHARGRP_MAIN_NORMAL;
	pM->wMaxHP = pM->wHP = 30;
	strcpy(pM->szName, "ミミック");
	pM = AddChar(pM);
	pM->pID = pM->dwTblID + 1;
	return pM;
}

static void MimicPC(PCHARINFO pPC, int nX, int nY, BYTE byDir)
{
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = (DWORD)nX;
	pPC->dwY = pPC->dwYBack = (DWORD)nY;
	pPC->byDirection = byDir;
	pPC->dwTargetTblID = (DWORD)-1;
	pPC->dwLastBattle = 0;
	AddTblNoPC((int)pPC->dwTblID);
}

static void MimicExamine(PCHARINFO pPC)
{
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_CHECKEVENT;
	Para.pID = pPC->pID;
	Para.dwTblID = pPC->dwTblID;
	Para.byData = 2;
	TestRecv(pPC, &Para, sizeof(Para));
}

static void TestMimic(const std::vector<int> &aLogin)
{
	printf("== W. Mimic ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	/* 横に3マス続けて空いている所を探す（左から プレイヤー・宝箱の敵・空き） */
	int nX = -1, nY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 2 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y) && PushIsFree(pMap, x + 2, y)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	if (nX < 0) {
		printf("no floor\n");
		return;
	}
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	DWORD dwTargetBack = pPC->dwTargetTblID, dwLastBattleBack = pPC->dwLastBattle;
	const int nConn = 1000;		/* aLogin[0] は 1000 番 */

	PCHARINFO pM = MimicMake(pPC, nX + 1, nY);
	DWORD dwM = pM->dwTblID;
	MimicPC(pPC, nX, nY, 3);

	/* ふだん：ぶつかる・上向き・動かない */
	printf("mimic idle type=%u dir=%d block=%d proc=%d\n", pM->byNpcType, pM->byDirection,
		(int)(CheckCharBlockNPC(pPC->dwMapID, (DWORD)(nX + 1), (DWORD)nY, 0) == dwM), (int)(pM->pProc != NULL));

	/* 話しても何も起きない */
	{
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_TALKRAND;
		Para.pID = pPC->pID;
		Para.dwTblID = pM->dwTblID;
		int nMsg0 = MockCmdCount(nConn, CMD_SYSMSG), nInfo0 = MockCmdCount(nConn, CMD_CHARINFO);
		TestRecv(pPC, &Para, sizeof(Para));
		printf("mimic talk type=%u dir=%d target=%d sysmsg=%d charinfo=%d\n", pM->byNpcType, pM->byDirection,
			(int)(pM->dwTargetTblID != (DWORD)-1), MockCmdCount(nConn, CMD_SYSMSG) - nMsg0, MockCmdCount(nConn, CMD_CHARINFO) - nInfo0);
	}

	/* 目の前にいない向きで調べても出ない */
	MimicPC(pPC, nX, nY, 0);
	MimicExamine(pPC);
	printf("mimic examine away type=%u\n", pM->byNpcType);

	/* 目の前を調べると正体を現す：戦闘２になり、調べた人の方（左）を向いてその人を狙う */
	MimicPC(pPC, nX, nY, 3);
	int nMsg0 = MockCmdCount(nConn, CMD_SYSMSG), nInfo0 = MockCmdCount(nConn, CMD_CHARINFO) + MockCmdCount(nConn, CMD_CHARINFO2);
	MimicExamine(pPC);
	printf("mimic examine type=%u dir=%d target_pc=%d proc=%d sysmsg=%d charinfo=%d hp=%u\n", pM->byNpcType, pM->byDirection,
		(int)(pM->dwTargetTblID == pPC->dwTblID), (int)(pM->pProc != NULL), MockCmdCount(nConn, CMD_SYSMSG) - nMsg0,
		MockCmdCount(nConn, CMD_CHARINFO) + MockCmdCount(nConn, CMD_CHARINFO2) - nInfo0, pM->wHP);
	/* 正体を現した後は敵（戦闘２）。もう一度調べても何も起きない */
	MimicExamine(pPC);
	printf("mimic examine again type=%u\n", pM->byNpcType);

	/* 叩かれても正体を現す（ダメージは普通に入る）。右にいる別の向きから叩く */
	DeleteChar(dwM);
	pM = MimicMake(pPC, nX + 1, nY);
	dwM = pM->dwTblID;
	pM->wMaxHP = pM->wHP = 30000;	/* 一撃では倒れない（会心の一撃でも。乱数の並びで結果が変わらないように） */
	MimicPC(pPC, nX + 2, nY, 2);
	pPC->dwTargetTblID = pM->dwTblID;
	pPC->dwLastBattle = 1;
	pPC->byAtackRate = ATACKRATE_JUST;
	BattleProcAtack(pPC, pM);
	printf("mimic hit type=%u dir=%d target_pc=%d damaged=%d\n", pM->byNpcType, pM->byDirection,
		(int)(pM->dwTargetTblID == pPC->dwTblID), (int)(pM->wHP < 30000));

	/* 一撃で倒れたら正体を現さずに普通に倒れる */
	DeleteChar(dwM);
	pM = MimicMake(pPC, nX + 1, nY);
	dwM = pM->dwTblID;
	pM->wHP = 1;
	MimicPC(pPC, nX + 2, nY, 2);
	pPC->dwTargetTblID = pM->dwTblID;
	pPC->dwLastBattle = 1;
	BattleProcAtack(pPC, pM);
	printf("mimic kill hp=%u removed=%d\n", pM->wHP, (int)(pM->byNpcType == NPCTYPE_INIT));

	/* 片付け */
	if (g_aCharInfo[dwM]->byNpcType != NPCTYPE_INIT) {
		DeleteChar(dwM);
	}
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	pPC->byDirection = byDirBack;
	pPC->dwTargetTblID = dwTargetBack;
	pPC->dwLastBattle = dwLastBattleBack;
	AddTblNoPC((int)pPC->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* X. 敵を全部倒すと開く扉（扉イベントの種別 EVENTDOOTTYPE_ENEMY）			 */
/*    敵がいる間は扉コマンドで開かず残りの数を知らせるか・傭兵も開けられないか */
/*    最後の1体が倒れたらすぐ開いて知らせるか・範囲の外や別の空間の敵は		 */
/*    数えないか・敵がまた出たら閉まるか（扉の上に人がいる間は待つか）		 */
/* ------------------------------------------------------------------------- */
static PCHARINFO EnemyDoorMake(PCHARINFO pPC, int nX, int nY, DWORD dwSpaceID)
{
	PCHARINFO pE = new CHARINFO;
	ZeroMemory((PBYTE)pE, sizeof(CHARINFO));
	pE->byNpcType = NPCTYPE_BATTLE1;
	SetInitCharData(pE);
	pE->byGameState = GAMESTATE_NPC;
	pE->dwMapID = pPC->dwMapID;
	pE->dwXBack = pE->dwX = (DWORD)nX;
	pE->dwYBack = pE->dwY = (DWORD)nY;
	pE->dwSpaceID = dwSpaceID;
	pE->wGrpMain = CHARGRP_MAIN_NORMAL;
	pE->wMaxHP = pE->wHP = 30;
	strcpy(pE->szName, "とびらの番人");
	pE = AddChar(pE);
	pE->pID = pE->dwTblID + 1;
	return pE;
}

static int EnemyDoorOpen(PCHARINFO pPC, PEVENTINFO pEvent, PEVENTINFO_DOOR pDoor)
{
	return (int)SpaceDoorIsOpen(pPC->dwMapID, pEvent, pDoor, SPACE_NORMAL);
}

static void TestEnemyDoor(const std::vector<int> &aLogin)
{
	printf("== X. EnemyDoor ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	/* 横に4マス続けて空いている所（左から プレイヤー・扉・敵・範囲の外の敵） */
	int nX = -1, nY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 3 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y) && PushIsFree(pMap, x + 2, y) && PushIsFree(pMap, x + 3, y)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	if (nX < 0) {
		printf("no floor\n");
		return;
	}
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	DWORD dwSpaceBack = pPC->dwSpaceID;
	DWORD dwNowBack = s_dwFakeNow;
	const int nConn = 1000;		/* aLogin[0] は 1000 番 */

	/* 扉イベントを一時的に足す（範囲は プレイヤー〜敵 の3マス） */
	int nEventBack = pMap->nEventCount;
	PEVENTINFO pEventBack = pMap->pEvent;
	pMap->pEvent = new EVENTINFO[nEventBack + 1];
	ZeroMemory(pMap->pEvent, sizeof(EVENTINFO) * (nEventBack + 1));
	if (nEventBack) {
		memcpy(pMap->pEvent, pEventBack, sizeof(EVENTINFO) * nEventBack);
	}
	PEVENTINFO pEvent = &pMap->pEvent[nEventBack];
	EVENTINFO_DOOR Door;
	ZeroMemory(&Door, sizeof(Door));
	Door.wGrpID = 11;
	Door.wBackGrpID = 22;
	Door.byTimeClose = 30;
	Door.byType = EVENTDOOTTYPE_ENEMY;
	Door.wEnemyStTmp = 65535;			/* 敵の種類の指定なし（全部数える） */
	Door.wAreaX1 = (WORD)(nX + 2);		/* 右下→左上の順に入れても同じ四角 */
	Door.wAreaY1 = (WORD)nY;
	Door.wAreaX2 = (WORD)nX;
	Door.wAreaY2 = (WORD)nY;
	pEvent->dwX = (DWORD)(nX + 1);
	pEvent->dwY = (DWORD)nY;
	pEvent->byType = EVENTTYPE_DOOR;
	pEvent->pData = (PBYTE)&Door;
	pMap->nEventCount = nEventBack + 1;

	MimicPC(pPC, nX, nY, 3);
	pPC->dwSpaceID = SPACE_NORMAL;

	/* 範囲の外（右端）と別の空間の敵だけ：数えないので開く */
	PCHARINFO pOut = EnemyDoorMake(pPC, nX + 3, nY, SPACE_NORMAL);
	PCHARINFO pOther = EnemyDoorMake(pPC, nX + 2, nY, 77);
	SpaceEnemyDoorCheckMap(pPC->dwMapID);
	printf("enemydoor outside count=%d other=%d open=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL),
		SpaceDoorEnemyCount(pPC->dwMapID, &Door, 77), EnemyDoorOpen(pPC, pEvent, &Door));
	DeleteChar(pOther->dwTblID);

	/* 範囲の中に敵が出る：閉まる */
	PCHARINFO pE = EnemyDoorMake(pPC, nX + 2, nY, SPACE_NORMAL);
	int nMap0 = MockCmdCount(nConn, CMD_CHGMAP);
	SpaceEnemyDoorCheckMap(pPC->dwMapID);
	printf("enemydoor spawn count=%d open=%d chgmap=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL),
		EnemyDoorOpen(pPC, pEvent, &Door), MockCmdCount(nConn, CMD_CHGMAP) - nMap0);

	/* 扉コマンド：開かず、残りの数を知らせる */
	{
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_OPENDOOR;
		Para.pID = pPC->pID;
		Para.dwTblID = pPC->dwTblID;
		int nMsg0 = MockCmdCount(nConn, CMD_MAPMSG), nSnd0 = MockCmdCount(nConn, CMD_PLAYSOUND);
		TestRecv(pPC, &Para, sizeof(Para));
		printf("enemydoor command open=%d mapmsg=%d sound=%d\n", EnemyDoorOpen(pPC, pEvent, &Door),
			MockCmdCount(nConn, CMD_MAPMSG) - nMsg0, MockCmdCount(nConn, CMD_PLAYSOUND) - nSnd0);
	}
	/* Sキーで目の前を調べても同じ */
	{
		int nMsg0 = MockCmdCount(nConn, CMD_MAPMSG);
		MimicExamine(pPC);
		printf("enemydoor examine open=%d mapmsg=%d\n", EnemyDoorOpen(pPC, pEvent, &Door), MockCmdCount(nConn, CMD_MAPMSG) - nMsg0);
	}
	/* 傭兵などが開けようとしても開かない */
	printf("enemydoor force=%d\n", (int)SpaceDoorOpen(pPC->dwMapID, pEvent, &Door, SPACE_NORMAL));

	/* 最後の1体が倒れる：すぐ開いて、近くの人に音とメッセージ */
	{
		int nMap1 = MockCmdCount(nConn, CMD_CHGMAP), nSnd0 = MockCmdCount(nConn, CMD_PLAYSOUND), nSys0 = MockCmdCount(nConn, CMD_SYSMSG);
		DeleteChar(pE->dwTblID);
		printf("enemydoor clear open=%d chgmap=%d sound=%d sysmsg=%d\n", EnemyDoorOpen(pPC, pEvent, &Door),
			MockCmdCount(nConn, CMD_CHGMAP) - nMap1, MockCmdCount(nConn, CMD_PLAYSOUND) - nSnd0, MockCmdCount(nConn, CMD_SYSMSG) - nSys0);
	}
	/* 開いている間は時間がたっても閉まらない */
	s_dwFakeNow += 60000;
	g_dwTime = s_dwFakeNow;
	SpaceDoorProc(pPC->dwMapID, pEvent, &Door);
	printf("enemydoor later open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));

	/* 敵がまた出る：扉の上に人がいる間は閉めない、降りたら閉まる */
	pE = EnemyDoorMake(pPC, nX + 2, nY, SPACE_NORMAL);
	MimicPC(pPC, nX + 1, nY, 3);
	SpaceEnemyDoorCheckMap(pPC->dwMapID);
	printf("enemydoor respawn onDoor open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));
	MimicPC(pPC, nX, nY, 3);
	SpaceEnemyDoorCheckMap(pPC->dwMapID);
	printf("enemydoor respawn next open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));	/* 扉のすぐ隣にいる間は閉めない */
	MimicPC(pPC, nX - 1, nY, 3);
	{
		int nSys0 = MockCmdCount(nConn, CMD_SYSMSG);
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor respawn open=%d sysmsg=%d\n", EnemyDoorOpen(pPC, pEvent, &Door), MockCmdCount(nConn, CMD_SYSMSG) - nSys0);
	}

	/* 数える敵の種類を指定：指定外の敵が残っていても開き、指定の敵が残れば閉まる。指定なしは全部数える */
	{
		const WORD wTarget = 5;
		Door.wEnemyStTmp = wTarget;
		pE->dwStTmplateID = 7;						/* 指定外の敵だけが範囲にいる */
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor sttmp other count=%d open=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL), EnemyDoorOpen(pPC, pEvent, &Door));
		PCHARINFO pT = EnemyDoorMake(pPC, nX + 2, nY, SPACE_NORMAL);
		pT->dwStTmplateID = wTarget;				/* 指定の敵が出る */
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor sttmp target count=%d open=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL), EnemyDoorOpen(pPC, pEvent, &Door));
		DeleteChar(pT->dwTblID);					/* 指定の敵が倒れる（指定外は残っている） */
		printf("enemydoor sttmp clear count=%d open=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL), EnemyDoorOpen(pPC, pEvent, &Door));
		/* NPC発生から出た敵は自分では番号を持たず（-1）、発生元(dwParentTblID)の番号で数える */
		{
			PCHARINFO pC = EnemyDoorMake(pPC, nX + 2, nY, SPACE_NORMAL);
			DWORD dwOutBack = pOut->dwStTmplateID;
			pC->dwStTmplateID = (DWORD)-1;
			pC->dwParentTblID = pOut->dwTblID;
			pOut->dwStTmplateID = wTarget;			/* 発生元が指定の番号 */
			printf("enemydoor sttmp child target count=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL));
			pOut->dwStTmplateID = 7;				/* 発生元が指定外 */
			printf("enemydoor sttmp child other count=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL));
			pOut->dwStTmplateID = dwOutBack;
			DeleteChar(pC->dwTblID);
		}
		Door.wEnemyStTmp = 65535;					/* 指定なしなら指定外の敵も数える */
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor sttmp none count=%d open=%d\n", SpaceDoorEnemyCount(pPC->dwMapID, &Door, SPACE_NORMAL), EnemyDoorOpen(pPC, pEvent, &Door));
	}

	/* 閉じ込められた時：範囲の外（奥）の人は中から開けられる。範囲の中（広間側）からは開けられない */
	{
		Door.wAreaX1 = (WORD)(nX + 2);
		Door.wAreaX2 = (WORD)(nX + 4);
		int nMsg0 = MockCmdCount(nConn, CMD_MAPMSG);
		printf("enemydoor inside before open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));
		MimicPC(pPC, nX + 2, nY, 2);				/* 広間側（範囲の中）から。扉は西の隣 */
		MimicExamine(pPC);
		printf("enemydoor inside hall open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));
		MimicPC(pPC, nX, nY, 3);					/* 奥側（範囲の外）から */
		MimicExamine(pPC);
		printf("enemydoor inside back open=%d mapmsg=%d\n", EnemyDoorOpen(pPC, pEvent, &Door), MockCmdCount(nConn, CMD_MAPMSG) - nMsg0);
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor inside stay open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));	/* 開けた直後、隣にいる間は閉まらない */
		MimicPC(pPC, nX - 2, nY, 3);
		SpaceEnemyDoorCheckMap(pPC->dwMapID);
		printf("enemydoor inside leave open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));	/* 離れたら閉まる */
	}

	/* 片付け */
	DeleteChar(pE->dwTblID);
	DeleteChar(pOut->dwTblID);
	pMap->nEventCount = nEventBack;
	delete [] pMap->pEvent;
	pMap->pEvent = pEventBack;
	s_dwFakeNow = dwNowBack;
	g_dwTime = s_dwFakeNow;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	pPC->byDirection = byDirBack;
	pPC->dwSpaceID = dwSpaceBack;
	AddTblNoPC((int)pPC->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* X2. 扉の追加マス（EVENTINFO_DOOR の byExtraCount・aExtra）				 */
/*    扉の開閉に合わせて、扉の位置からずらしたマスのパーツも切り替わるか		 */
/*    （開けた空間の人にだけ・閉めた時に戻る・通れる判定も付いてくる）。		 */
/*    0 件なら今までどおり。データAPIの範囲外の入力は断る・省くと今のまま。	 */
/*    管理者ウィンドウ（追加マスを知らない短い扉）で直しても追加マスは消えない。	 */
/*    保存して読み直しても残る												 */
/* ------------------------------------------------------------------------- */
typedef struct _DOOREXEV {
	DWORD				dwX, dwY;
	BYTE				byType;
	std::vector<BYTE>	aData;
} DOOREXEV;

static void DoorExtraSaveEvents(PMAPINFO pMap, std::vector<DOOREXEV> &aOut)
{
	aOut.clear();
	for (int i = 0; i < pMap->nEventCount; i ++) {
		DOOREXEV e;
		e.dwX = pMap->pEvent[i].dwX;
		e.dwY = pMap->pEvent[i].dwY;
		e.byType = pMap->pEvent[i].byType;
		int nSize = GetEventDataSize(e.byType);
		if (pMap->pEvent[i].pData && nSize > 0) {
			e.aData.assign((const BYTE *)pMap->pEvent[i].pData, (const BYTE *)pMap->pEvent[i].pData + nSize);
		}
		aOut.push_back(e);
	}
}

static void DoorExtraRestoreEvents(PMAPINFO pMap, const std::vector<DOOREXEV> &aSrc)
{
	for (int i = 0; i < pMap->nEventCount && pMap->pEvent; i ++) {
		if (pMap->pEvent[i].pData) {
			delete [] (PBYTE)pMap->pEvent[i].pData;
		}
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->nEventCount = (int)aSrc.size();
	if (!aSrc.empty()) {
		pMap->pEvent = new EVENTINFO[aSrc.size()];
		ZeroMemory(pMap->pEvent, sizeof(EVENTINFO) * aSrc.size());
		for (size_t i = 0; i < aSrc.size(); i ++) {
			pMap->pEvent[i].dwX = aSrc[i].dwX;
			pMap->pEvent[i].dwY = aSrc[i].dwY;
			pMap->pEvent[i].byType = aSrc[i].byType;
			if (!aSrc[i].aData.empty()) {
				PBYTE p = new BYTE[aSrc[i].aData.size()];
				memcpy(p, &aSrc[i].aData[0], aSrc[i].aData.size());
				pMap->pEvent[i].pData = p;
			}
		}
	}
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
}

/* マップの JSON から events の配列（[...]）を取り出す */
static std::string DoorExtraEventsJson(int nMapNo)
{
	std::string str = SboWebDataGet("map", nMapNo);
	size_t nPos = str.find("\"events\":[");
	if (nPos == std::string::npos) {
		return "[]";
	}
	nPos += 9;
	int nDepth = 0;
	for (size_t i = nPos; i < str.size(); i ++) {
		if (str[i] == '[') nDepth ++;
		if (str[i] == ']') {
			nDepth --;
			if (nDepth == 0) {
				return str.substr(nPos, i - nPos + 1);
			}
		}
	}
	return "[]";
}

static PEVENTINFO DoorExtraFind(PMAPINFO pMap, int x, int y)
{
	for (int i = 0; i < pMap->nEventCount; i ++) {
		if (pMap->pEvent[i].byType == EVENTTYPE_DOOR && pMap->pEvent[i].pData && (int)pMap->pEvent[i].dwX == x && (int)pMap->pEvent[i].dwY == y) {
			return &pMap->pEvent[i];
		}
	}
	return NULL;
}

static void TestDoorExtra(const std::vector<int> &aLogin)
{
	printf("== X2. DoorExtra ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	/* 横に3マス空いていて、その上（y-1）の左端も空いている所（左から プレイヤーが立つ上のマス・扉の下のマス…） */
	int nX = -1, nY = -1;
	for (int y = 1; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 2 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y) && PushIsFree(pMap, x + 2, y) && PushIsFree(pMap, x, y - 1)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	WORD wBlock = 0xFFFF, wFree = 0xFFFF;
	for (int i = 0; i < (int)g_wCountMapParts; i ++) {
		if (wBlock == 0xFFFF && g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) wBlock = (WORD)i;
		if (wFree == 0xFFFF && g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) wFree = (WORD)i;
	}
	if (nX < 0 || wBlock == 0xFFFF || wFree == 0xFFFF) {
		printf("no floor\n");
		return;
	}
	const int nConn = 1000;		/* aLogin[0] は 1000 番 */
	const DWORD dwMapID = pMap->dwID;
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	DWORD dwSpaceBack = pPC->dwSpaceID;
	DWORD dwNowBack = s_dwFakeNow;
	BYTE byAdminBack = pPC->byAdminLV;
	std::vector<DOOREXEV> aBackup;
	DoorExtraSaveEvents(pMap, aBackup);
	const std::string strOrig = DoorExtraEventsJson(nMapNo);
	const int nEvOrig = pMap->nEventCount;
	const int xD = nX + 1, yD = nY;			/* 扉 */
	const int xA = nX + 1, yA = nY - 1;		/* 追加マス A（扉の上。dx=0 dy=-1） */
	const int xB = nX + 2, yB = nY;			/* 追加マス B（扉の右。dx=1 dy=0） */
	/* 追加マスを付ける前の通れる判定（0 件なら変わらないことの確認用） */
	int nBaseA = (int)CheckCharBlock(dwMapID, xA, yA), nBaseB = (int)CheckCharBlock(dwMapID, xB, yB);

	char sz[1024];
	/* events の全部（元のイベント＋試す扉）を作る。pszExtra は扉の detail に足す文字列（"" か ,"extraCells":[...]） */
	auto Events = [&](const std::string &strExtra1, const std::string &strDoor2Extra) {
		std::string strHead = strOrig.substr(0, strOrig.size() - 1);
		std::string s = "{\"events\":" + strHead + (strOrig.size() > 2 ? "," : "");
		snprintf(sz, sizeof(sz), "{\"x\":%d,\"y\":%d,\"type\":3,\"detail\":{\"wGrpID\":%d,\"wBackGrpID\":%d,\"byTimeClose\":30,\"byType\":0%s}}", xD, yD, (int)wBlock, (int)wFree, strExtra1.c_str());
		s += sz;
		snprintf(sz, sizeof(sz), ",{\"x\":%d,\"y\":%d,\"type\":3,\"detail\":{\"wGrpID\":%d,\"wBackGrpID\":%d,\"byTimeClose\":30,\"byType\":0%s}}]}", nX, nY, (int)wBlock, (int)wFree, strDoor2Extra.c_str());
		s += sz;
		return s;
	};
	char szCells[512];
	snprintf(szCells, sizeof(szCells), ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":%d,\"open\":%d},{\"dx\":1,\"dy\":0,\"closed\":%d,\"open\":%d}]", (int)wBlock, (int)wFree, (int)wBlock, (int)wFree);
	const std::string strCells = szCells;
	auto ok = [](const std::string &s) { return (int)(s.find("\"ok\":true") != std::string::npos); };

	/* 範囲外・不正な入力は断る（イベントは増えず、追加マスも付かない） */
	{
		std::vector<std::pair<std::string, std::string> > aBad;
		snprintf(sz, sizeof(sz), "%d", (int)g_wCountMapParts);
		const std::string strCnt = sz;
		aBad.push_back(std::make_pair("dx over", ",\"extraCells\":[{\"dx\":9,\"dy\":0,\"closed\":1,\"open\":1}]"));
		aBad.push_back(std::make_pair("dx under", ",\"extraCells\":[{\"dx\":-9,\"dy\":0,\"closed\":1,\"open\":1}]"));
		aBad.push_back(std::make_pair("dy over", ",\"extraCells\":[{\"dx\":0,\"dy\":9,\"closed\":1,\"open\":1}]"));
		aBad.push_back(std::make_pair("dy under", ",\"extraCells\":[{\"dx\":0,\"dy\":-9,\"closed\":1,\"open\":1}]"));
		aBad.push_back(std::make_pair("both zero", ",\"extraCells\":[{\"dx\":0,\"dy\":0,\"closed\":1,\"open\":1}]"));
		aBad.push_back(std::make_pair("closed range", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":" + strCnt + ",\"open\":1}]"));
		aBad.push_back(std::make_pair("open range", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":1,\"open\":" + strCnt + "}]"));
		aBad.push_back(std::make_pair("open negative", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":1,\"open\":-1}]"));
		aBad.push_back(std::make_pair("dup", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":1,\"open\":1},{\"dx\":0,\"dy\":-1,\"closed\":2,\"open\":2}]"));
		aBad.push_back(std::make_pair("missing key", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":1}]"));
		aBad.push_back(std::make_pair("unknown key", ",\"extraCells\":[{\"dx\":0,\"dy\":-1,\"closed\":1,\"open\":1,\"zz\":1}]"));
		aBad.push_back(std::make_pair("not array", ",\"extraCells\":{\"dx\":0}"));
		aBad.push_back(std::make_pair("not int", ",\"extraCells\":[{\"dx\":0.5,\"dy\":-1,\"closed\":1,\"open\":1}]"));
		std::string strNine = ",\"extraCells\":[";
		for (int i = 0; i < 9; i ++) {
			snprintf(sz, sizeof(sz), "%s{\"dx\":%d,\"dy\":-1,\"closed\":1,\"open\":1}", i ? "," : "", i - 4);
			strNine += sz;
		}
		aBad.push_back(std::make_pair("nine", strNine + "]"));
		/* マップの外のマス（扉の位置から左・右・上・下へずらして外に出る組） */
		if (xD <= DOOREXTRA_OFFMAX) {
			snprintf(sz, sizeof(sz), ",\"extraCells\":[{\"dx\":%d,\"dy\":0,\"closed\":1,\"open\":1}]", -(xD + 1));
			aBad.push_back(std::make_pair("outside left", sz));
		}
		if (pMap->nCX - xD <= DOOREXTRA_OFFMAX) {
			snprintf(sz, sizeof(sz), ",\"extraCells\":[{\"dx\":%d,\"dy\":0,\"closed\":1,\"open\":1}]", pMap->nCX - xD);
			aBad.push_back(std::make_pair("outside right", sz));
		}
		if (yD + 1 <= DOOREXTRA_OFFMAX) {
			snprintf(sz, sizeof(sz), ",\"extraCells\":[{\"dx\":0,\"dy\":%d,\"closed\":1,\"open\":1}]", -(yD + 1));
			aBad.push_back(std::make_pair("outside top", sz));
		}
		if (pMap->nCY - yD <= DOOREXTRA_OFFMAX) {
			snprintf(sz, sizeof(sz), ",\"extraCells\":[{\"dx\":0,\"dy\":%d,\"closed\":1,\"open\":1}]", pMap->nCY - yD);
			aBad.push_back(std::make_pair("outside bottom", sz));
		}
		for (size_t i = 0; i < aBad.size(); i ++) {
			std::string strRes = SwapApi(nMapNo, Events(aBad[i].second, "").c_str());
			printf("doorextra api bad %s ok=%d events=%d\n", aBad[i].first.c_str(), ok(strRes), (int)(pMap->nEventCount == nEvOrig));
		}
		/* 扉ではないイベントには付けられない */
		snprintf(sz, sizeof(sz), "{\"events\":[{\"x\":%d,\"y\":%d,\"type\":0,\"detail\":{\"extraCells\":[]}}]}", xD, yD);
		printf("doorextra api bad nondoor ok=%d events=%d\n", ok(SwapApi(nMapNo, sz)), (int)(pMap->nEventCount == nEvOrig));
	}

	/* 追加マスを 2 件つけて保存する（扉の下見だけでは何も変わらない） */
	{
		const char *pszDry = SboWebDataApply("map", nMapNo, Events(strCells, "").c_str(), 0, "テスト");
		printf("doorextra api dry has=%d events=%d\n", (int)(pszDry && strstr(pszDry, "\"extraCells\":[{\"dx\":0,\"dy\":-1,") != NULL), (int)(pMap->nEventCount == nEvOrig));
	}
	std::string strRes = SwapApi(nMapNo, Events(strCells, "").c_str());
	PEVENTINFO pEvent = DoorExtraFind(pMap, xD, yD);
	PEVENTINFO_DOOR pDoor = pEvent ? (PEVENTINFO_DOOR)pEvent->pData : NULL;
	if (!pDoor) {
		printf("doorextra api set ok=%d door=0\n", ok(strRes));
		DoorExtraRestoreEvents(pMap, aBackup);
		return;
	}
	printf("doorextra api set ok=%d events=%d count=%d a=%d,%d,%d,%d b=%d,%d,%d,%d\n", ok(strRes), pMap->nEventCount - nEvOrig, (int)pDoor->byExtraCount,
		(int)pDoor->aExtra[0].sbyOffX, (int)pDoor->aExtra[0].sbyOffY, (int)(pDoor->aExtra[0].wClosedParts == wBlock), (int)(pDoor->aExtra[0].wOpenParts == wFree),
		(int)pDoor->aExtra[1].sbyOffX, (int)pDoor->aExtra[1].sbyOffY, (int)(pDoor->aExtra[1].wClosedParts == wBlock), (int)(pDoor->aExtra[1].wOpenParts == wFree));
	{
		std::string strGet = SboWebDataGet("map", nMapNo);
		size_t nFirst = strGet.find("\"extraCells\"");
		printf("doorextra api get has=%d once=%d\n", (int)(strGet.find(strCells.substr(1)) != std::string::npos),
			(int)(nFirst != std::string::npos && strGet.find("\"extraCells\"", nFirst + 1) == std::string::npos));
	}

	/* ふつうの世界：閉まっている間は追加マスが通れず、開けると 3 マス（扉＋追加2）切り替わって通れる */
	MimicPC(pPC, nX, nY - 1, 3);
	pPC->dwSpaceID = SPACE_NORMAL;
	printf("doorextra closed block normal a=%d b=%d viewer a=%d b=%d\n", (int)CheckCharBlock(dwMapID, xA, yA), (int)CheckCharBlock(dwMapID, xB, yB),
		(int)CheckCharBlock(pPC, dwMapID, xA, yA), (int)CheckCharBlock(pPC, dwMapID, xB, yB));
	s_dwFakeNow += 10000;
	FloorStep(pPC, xA, yA, 3);
	printf("doorextra closed walk stay=%d\n", (int)(pPC->dwX == (DWORD)nX && pPC->dwY == (DWORD)(nY - 1)));
	{
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		MockTrace(1);
		int nOpen = (int)SpaceDoorOpen(dwMapID, pEvent, pDoor, SPACE_NORMAL);
		std::vector<BYTE> aBody;
		MarketLast(nConn, CMD_CHGMAP, aBody);
		MockTrace(0);
		PCMDPARA_CHGMAP pLast = (aBody.size() >= sizeof(CMDPARA_CHGMAP)) ? (PCMDPARA_CHGMAP)&aBody[0] : NULL;
		printf("doorextra open ret=%d chgmap=%d last=%d,%d,%d\n", nOpen, MockCmdCount(nConn, CMD_CHGMAP) - nChg0,
			(int)(pLast && pLast->wMap == wFree), pLast ? pLast->nX : -1, pLast ? pLast->nY : -1);
	}
	printf("doorextra open block normal a=%d b=%d viewer a=%d b=%d\n", (int)CheckCharBlock(dwMapID, xA, yA), (int)CheckCharBlock(dwMapID, xB, yB),
		(int)CheckCharBlock(pPC, dwMapID, xA, yA), (int)CheckCharBlock(pPC, dwMapID, xB, yB));
	s_dwFakeNow += 10000;
	FloorStep(pPC, xA, yA, 3);
	printf("doorextra open walk in=%d\n", (int)(pPC->dwX == (DWORD)xA && pPC->dwY == (DWORD)yA));
	MimicPC(pPC, nX, nY - 1, 3);
	/* 閉まる時間が過ぎると 3 マスとも戻る */
	{
		s_dwFakeNow += 60000;
		g_dwTime = s_dwFakeNow;
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		MockTrace(1);
		SpaceDoorProc(dwMapID, pEvent, pDoor);
		std::vector<BYTE> aBody;
		MarketLast(nConn, CMD_CHGMAP, aBody);
		MockTrace(0);
		PCMDPARA_CHGMAP pLast = (aBody.size() >= sizeof(CMDPARA_CHGMAP)) ? (PCMDPARA_CHGMAP)&aBody[0] : NULL;
		printf("doorextra close open=%d chgmap=%d last=%d,%d,%d block a=%d b=%d\n", (int)SpaceDoorIsOpen(dwMapID, pEvent, pDoor, SPACE_NORMAL), MockCmdCount(nConn, CMD_CHGMAP) - nChg0,
			(int)(pLast && pLast->wMap == wBlock), pLast ? pLast->nX : -1, pLast ? pLast->nY : -1,
			(int)CheckCharBlock(dwMapID, xA, yA), (int)CheckCharBlock(dwMapID, xB, yB));
	}

	/* 別の空間：開けた空間の人だけ切り替わり、ふつうの世界は閉じたまま。空間を移った人には今の状態を送る */
	pPC->dwSpaceID = 77;
	{
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		int nOpen = (int)SpaceDoorOpen(dwMapID, pEvent, pDoor, 77);
		printf("doorextra space open ret=%d chgmap=%d viewer a=%d b=%d normal a=%d b=%d\n", nOpen, MockCmdCount(nConn, CMD_CHGMAP) - nChg0,
			(int)CheckCharBlock(pPC, dwMapID, xA, yA), (int)CheckCharBlock(pPC, dwMapID, xB, yB),
			(int)CheckCharBlock(dwMapID, xA, yA), (int)CheckCharBlock(dwMapID, xB, yB));
	}
	{
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		SpaceSendMapState(pPC);
		int nWith = MockCmdCount(nConn, CMD_CHGMAP) - nChg0;
		BYTE byCnt = pDoor->byExtraCount;
		pDoor->byExtraCount = 0;
		nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		SpaceSendMapState(pPC);
		int nWithout = MockCmdCount(nConn, CMD_CHGMAP) - nChg0;
		pDoor->byExtraCount = byCnt;
		printf("doorextra space state extra=%d\n", nWith - nWithout);
	}
	pPC->dwSpaceID = SPACE_NORMAL;
	printf("doorextra space other normal open=%d a=%d b=%d\n", (int)SpaceDoorIsOpen(dwMapID, pEvent, pDoor, SPACE_NORMAL),
		(int)CheckCharBlock(pPC, dwMapID, xA, yA), (int)CheckCharBlock(pPC, dwMapID, xB, yB));
	pPC->dwSpaceID = 77;
	{
		s_dwFakeNow += 60000;
		g_dwTime = s_dwFakeNow;
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		SpaceDoorProc(dwMapID, pEvent, pDoor);
		printf("doorextra space close open=%d chgmap=%d viewer a=%d b=%d\n", (int)SpaceDoorIsOpen(dwMapID, pEvent, pDoor, 77), MockCmdCount(nConn, CMD_CHGMAP) - nChg0,
			(int)CheckCharBlock(pPC, dwMapID, xA, yA), (int)CheckCharBlock(pPC, dwMapID, xB, yB));
	}
	pPC->dwSpaceID = SPACE_NORMAL;

	/* データAPIで extraCells を省くと今のまま（残る）・[] で全部消える */
	SwapApi(nMapNo, Events("", "").c_str());
	pEvent = DoorExtraFind(pMap, xD, yD);
	pDoor = pEvent ? (PEVENTINFO_DOOR)pEvent->pData : NULL;
	printf("doorextra api omit count=%d\n", pDoor ? (int)pDoor->byExtraCount : -1);

	/* 管理者ウィンドウの扉の更新（追加マスを知らない短い扉・追加マスが 0 件の新しい扉）でも追加マスは消えない */
	{
		pPC->byAdminLV = ADMINLV_ADMINISTRATOR;
		int nNo = (int)(pEvent - pMap->pEvent);
		EVENTINFO_DOOR New;
		ZeroMemory(&New, sizeof(New));
		New.wGrpID = wFree;
		New.wBackGrpID = wBlock;
		New.byTimeClose = 50;
		New.wEnemyStTmp = 65535;
		const size_t nShort = offsetof(EVENTINFO_DOOR, byExtraCount);
		/* 新しい大きさで送る時は、クライアントが送った追加マスが採用される。まず今の追加マスをそのまま送って引き継ぎを確かめる */
		New.byExtraCount = pDoor->byExtraCount;
		CopyMemory(New.aExtra, pDoor->aExtra, sizeof(New.aExtra));
		for (int k = 0; k < 2; k ++) {
			const WORD wExpect = New.wGrpID;
			size_t nBody = (k == 0) ? sizeof(New) : nShort;
			std::vector<BYTE> aPacket(sizeof(CMDPARA_CHGEVENT) + nBody, 0xCC);
			PCMDPARA_CHGEVENT pPara = (PCMDPARA_CHGEVENT)&aPacket[0];
			ZeroMemory(pPara, sizeof(CMDPARA_CHGEVENT));
			pPara->byCmd = CMD_CHGEVENT;
			pPara->dwID = dwMapID;
			pPara->dwDataID = (DWORD)nNo;
			pPara->dwX = (DWORD)xD;
			pPara->dwY = (DWORD)yD;
			pPara->byType = EVENTTYPE_DOOR;
			pPara->pData = (PVOID)1;
			memcpy(&aPacket[sizeof(CMDPARA_CHGEVENT)], &New, nBody);
			TestRecv(pPC, &aPacket[0], aPacket.size());
			pEvent = DoorExtraFind(pMap, xD, yD);
			pDoor = pEvent ? (PEVENTINFO_DOOR)pEvent->pData : NULL;
			printf("doorextra admin %s count=%d a=%d,%d b=%d,%d changed=%d\n", (k == 0) ? "full" : "short", pDoor ? (int)pDoor->byExtraCount : -1,
				pDoor ? (int)pDoor->aExtra[0].sbyOffX : 0, pDoor ? (int)pDoor->aExtra[0].sbyOffY : 0, pDoor ? (int)pDoor->aExtra[1].sbyOffX : 0, pDoor ? (int)pDoor->aExtra[1].sbyOffY : 0,
				pDoor ? (int)(pDoor->wGrpID == wExpect && pDoor->byTimeClose == 50) : -1);
			New.wGrpID = wBlock;
		}
		/* 新しい大きさで送った追加マスは、引き継ぎで上書きされずに反映される（件数・値の変更、0 件で全部消す） */
		{
			DOOREXTRA aKeep[MAX_DOOREXTRA];
			BYTE byKeep = pDoor->byExtraCount;
			CopyMemory(aKeep, pDoor->aExtra, sizeof(aKeep));
			for (int k = 0; k < 3; k ++) {
				EVENTINFO_DOOR Send = New;
				if (k == 0) {			/* 1 件に減らして値も変える */
					ZeroMemory(Send.aExtra, sizeof(Send.aExtra));
					Send.byExtraCount = 1;
					Send.aExtra[0].sbyOffX = 2;
					Send.aExtra[0].sbyOffY = -3;
					Send.aExtra[0].wClosedParts = wFree;
					Send.aExtra[0].wOpenParts = wBlock;
				} else if (k == 1) {	/* 0 件で全部消す */
					ZeroMemory(Send.aExtra, sizeof(Send.aExtra));
					Send.byExtraCount = 0;
				} else {				/* 元に戻す */
					Send.byExtraCount = byKeep;
					CopyMemory(Send.aExtra, aKeep, sizeof(Send.aExtra));
				}
				std::vector<BYTE> aPacket(sizeof(CMDPARA_CHGEVENT) + sizeof(Send), 0xCC);
				PCMDPARA_CHGEVENT pPara = (PCMDPARA_CHGEVENT)&aPacket[0];
				ZeroMemory(pPara, sizeof(CMDPARA_CHGEVENT));
				pPara->byCmd = CMD_CHGEVENT;
				pPara->dwID = dwMapID;
				pPara->dwDataID = (DWORD)nNo;
				pPara->dwX = (DWORD)xD;
				pPara->dwY = (DWORD)yD;
				pPara->byType = EVENTTYPE_DOOR;
				pPara->pData = (PVOID)1;
				memcpy(&aPacket[sizeof(CMDPARA_CHGEVENT)], &Send, sizeof(Send));
				TestRecv(pPC, &aPacket[0], aPacket.size());
				pEvent = DoorExtraFind(pMap, xD, yD);
				pDoor = pEvent ? (PEVENTINFO_DOOR)pEvent->pData : NULL;
				if (k < 2) {
					printf("doorextra admin new %s count=%d a=%d,%d,%d,%d\n", (k == 0) ? "edit" : "clear", pDoor ? (int)pDoor->byExtraCount : -1,
						pDoor ? (int)pDoor->aExtra[0].sbyOffX : 0, pDoor ? (int)pDoor->aExtra[0].sbyOffY : 0,
						pDoor ? (int)(pDoor->aExtra[0].wClosedParts == wFree) : -1, pDoor ? (int)(pDoor->aExtra[0].wOpenParts == wBlock) : -1);
				} else {
					printf("doorextra admin new restore count=%d a=%d,%d b=%d,%d\n", pDoor ? (int)pDoor->byExtraCount : -1,
						pDoor ? (int)pDoor->aExtra[0].sbyOffX : 0, pDoor ? (int)pDoor->aExtra[0].sbyOffY : 0, pDoor ? (int)pDoor->aExtra[1].sbyOffX : 0, pDoor ? (int)pDoor->aExtra[1].sbyOffY : 0);
				}
			}
		}
		pPC->byAdminLV = byAdminBack;
	}

	/* 保存して読み直す（読み込みは末尾に足される）。追加マスのある扉と無い扉 */
	{
		extern BOOL SaveMapInfo(void);
		extern BOOL LoadMapInfo(void);
		int nMaps = g_aMapInfo.GetSize();
		DWORD dwNewMapIDBak = g_dwNewMapID;
		BOOL bSaved = SaveMapInfo();
		BOOL bLoaded = LoadMapInfo();
		int nAdded = g_aMapInfo.GetSize() - nMaps;
		if (nAdded >= 1) {
			PMAPINFO pL = g_aMapInfo[nMaps + nMapNo];
			PEVENTINFO pE1 = DoorExtraFind(pL, xD, yD), pE2 = DoorExtraFind(pL, nX, nY);
			PEVENTINFO_DOOR pD1 = pE1 ? (PEVENTINFO_DOOR)pE1->pData : NULL, pD2 = pE2 ? (PEVENTINFO_DOOR)pE2->pData : NULL;
			printf("doorextra save=%d load=%d added=%d events=%d door1=%d,%d,%d,%d,%d,%d,%d door2=%d\n", (int)bSaved, (int)bLoaded, nAdded, (int)(pL->nEventCount == pMap->nEventCount),
				pD1 ? (int)pD1->byExtraCount : -1, pD1 ? (int)pD1->aExtra[0].sbyOffX : 0, pD1 ? (int)pD1->aExtra[0].sbyOffY : 0,
				pD1 ? (int)(pD1->aExtra[0].wClosedParts == wBlock && pD1->aExtra[0].wOpenParts == wFree) : -1,
				pD1 ? (int)pD1->aExtra[1].sbyOffX : 0, pD1 ? (int)pD1->aExtra[1].sbyOffY : 0, pD1 ? (int)(pD1->aExtra[1].wClosedParts == wBlock && pD1->aExtra[1].wOpenParts == wFree) : -1,
				pD2 ? (int)pD2->byExtraCount : -1);
		} else {
			printf("doorextra save=%d load=%d added=%d\n", (int)bSaved, (int)bLoaded, nAdded);
		}
		for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
			g_aMapInfo.RemoveAt(i);
		}
		g_dwNewMapID = dwNewMapIDBak;
	}

	/* [] で全部消す：0 件なら今までどおり（開けても扉の 1 マスだけ送り、追加マスだった所は元の通れる判定に戻る） */
	SwapApi(nMapNo, Events(",\"extraCells\":[]", "").c_str());
	pEvent = DoorExtraFind(pMap, xD, yD);
	pDoor = pEvent ? (PEVENTINFO_DOOR)pEvent->pData : NULL;
	if (pDoor) {
		std::string strGet = SboWebDataGet("map", nMapNo);
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		SpaceDoorOpen(dwMapID, pEvent, pDoor, SPACE_NORMAL);
		printf("doorextra none count=%d get_has=%d open chgmap=%d block a=%d b=%d same=%d\n", (int)pDoor->byExtraCount, (int)(strGet.find("\"extraCells\"") != std::string::npos),
			MockCmdCount(nConn, CMD_CHGMAP) - nChg0, (int)CheckCharBlock(dwMapID, xA, yA), (int)CheckCharBlock(dwMapID, xB, yB),
			(int)(CheckCharBlock(dwMapID, xA, yA) == nBaseA && CheckCharBlock(dwMapID, xB, yB) == nBaseB));
	} else {
		printf("doorextra none door=0\n");
	}

	/* 片付け */
	DoorExtraRestoreEvents(pMap, aBackup);
	s_dwFakeNow = dwNowBack;
	g_dwTime = s_dwFakeNow;
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	pPC->byDirection = byDirBack;
	pPC->dwSpaceID = dwSpaceBack;
	pPC->byAdminLV = byAdminBack;
	AddTblNoPC((int)pPC->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* X3. 鍵の扉の「鍵がない時の文」（EVENTINFO_DOOR の szLockMsg）				 */
/*    鍵を持たない人が開けようとすると szLockMsg（末尾に @2 が無ければ足す）が	 */
/*    その人に届く・空なら既定文。データAPIの範囲外・鍵以外の扉への指定は断る。	 */
/*    保存して読み直しても残り、管理者ウィンドウの短い（文を知らない）扉で		 */
/*    直しても消えない（新しい大きさで送れば反映される）					 */
/* ------------------------------------------------------------------------- */
static void TestDoorLockMsg(const std::vector<int> &aLogin)
{
	printf("== X3. DoorLockMsg ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	int nX = -1, nY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 2 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y) && PushIsFree(pMap, x + 2, y)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	WORD wKey = 0;
	for (int i = 1; i < g_aItemInfo.GetSize(); i ++) {
		if (g_aItemInfo[i]->szName[0]) {
			wKey = (WORD)i;
			break;
		}
	}
	if (nX < 0 || wKey == 0) {
		printf("no floor\n");
		return;
	}
	const int nConn = 1000;
	const DWORD dwMapID = pMap->dwID;
	DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	DWORD dwSpaceBack = pPC->dwSpaceID;
	BYTE byAdminBack = pPC->byAdminLV;
	WORD awImpBack[MAX_IMPORTANT], awItemBack[MAX_MYITEM];
	memcpy(awImpBack, pPC->waImportant, sizeof(awImpBack));
	memcpy(awItemBack, pPC->waItem, sizeof(awItemBack));
	std::vector<DOOREXEV> aBackup;
	DoorExtraSaveEvents(pMap, aBackup);
	const std::string strOrig = DoorExtraEventsJson(nMapNo);
	const int nEvOrig = pMap->nEventCount;
	const int xD = nX + 1, yD = nY;
	const char szMsg[] = "カルバの話を聞いてからだ。";
	const char szDefault[] = "鍵がかかっている。@2";

	char sz[1024];
	/* events の全部（元のイベント＋試す扉）。strDetail は扉の detail に足す文字列（byType 以降） */
	auto Events = [&](const std::string &strDetail) {
		std::string strHead = strOrig.substr(0, strOrig.size() - 1);
		std::string s = "{\"events\":" + strHead + (strOrig.size() > 2 ? "," : "");
		snprintf(sz, sizeof(sz), "{\"x\":%d,\"y\":%d,\"type\":3,\"detail\":{\"wGrpID\":11,\"wBackGrpID\":22,\"byTimeClose\":30%s}}]}", xD, yD, strDetail.c_str());
		return s + sz;
	};
	auto ok = [](const std::string &s) { return (int)(s.find("\"ok\":true") != std::string::npos); };
	auto Door = [&]() {
		PEVENTINFO pE = DoorExtraFind(pMap, xD, yD);
		return pE ? (PEVENTINFO_DOOR)pE->pData : (PEVENTINFO_DOOR)NULL;
	};
	char szKeyType[64];
	snprintf(szKeyType, sizeof(szKeyType), ",\"byType\":2,\"wKeyItem\":%d", (int)wKey);
	const std::string strKey = szKeyType;

	/* 範囲外・鍵以外の扉への指定は断る（イベントは増えない） */
	{
		std::string str95(95, 'a'), str96(96, 'a');
		std::vector<std::pair<std::string, std::string> > aBad;
		aBad.push_back(std::make_pair("nonkey type0", ",\"byType\":0,\"szLockMsg\":\"x\""));
		aBad.push_back(std::make_pair("nonkey type1", ",\"byType\":1,\"szLockMsg\":\"x\""));
		aBad.push_back(std::make_pair("too long", strKey + ",\"szLockMsg\":\"" + str96 + "\""));
		aBad.push_back(std::make_pair("not string", strKey + ",\"szLockMsg\":5"));
		for (size_t i = 0; i < aBad.size(); i ++) {
			std::string strRes = SwapApi(nMapNo, Events(aBad[i].second).c_str());
			printf("doorlock api bad %s ok=%d events=%d\n", aBad[i].first.c_str(), ok(strRes), (int)(pMap->nEventCount == nEvOrig));
		}
		/* 空文字は鍵以外の扉でも書ける（既定文のまま） */
		std::string strEmpty = SwapApi(nMapNo, Events(",\"byType\":0,\"szLockMsg\":\"\"").c_str());
		printf("doorlock api nonkey empty ok=%d\n", ok(strEmpty));
		/* ちょうど 95 バイトは通る */
		std::string strMax = SwapApi(nMapNo, Events(strKey + ",\"szLockMsg\":\"" + str95 + "\"").c_str());
		printf("doorlock api max95 ok=%d len=%d\n", ok(strMax), Door() ? (int)strlen(Door()->szLockMsg) : -1);
	}

	/* 設定する */
	std::string strRes = SwapApi(nMapNo, Events(strKey + ",\"szLockMsg\":\"" + szMsg + "\"").c_str());
	PEVENTINFO_DOOR pDoor = Door();
	if (!pDoor) {
		printf("doorlock api set ok=%d door=0\n", ok(strRes));
		DoorExtraRestoreEvents(pMap, aBackup);
		return;
	}
	printf("doorlock api set ok=%d type=%d msg=%d\n", ok(strRes), (int)pDoor->byType, (int)(strcmp(pDoor->szLockMsg, szMsg) == 0));
	{
		std::string strGet = SboWebDataGet("map", nMapNo);
		printf("doorlock api get has=%d\n", (int)(strGet.find(std::string("\"szLockMsg\":\"") + szMsg + "\"") != std::string::npos));
		/* get した中身をそのまま書き戻しても変わらない */
		size_t nS = strGet.find("\"events\":"), nE = strGet.find(",\"shops\":");
		std::string strBody = "{" + ((nS != std::string::npos && nE != std::string::npos) ? strGet.substr(nS, nE - nS) : std::string()) + "}";
		std::string strRound = SwapApi(nMapNo, strBody.c_str());
		pDoor = Door();
		printf("doorlock api roundtrip ok=%d msg=%d\n", ok(strRound), pDoor ? (int)(strcmp(pDoor->szLockMsg, szMsg) == 0) : -1);
	}
	/* detail に szLockMsg を書かない時は今のまま（残る） */
	SwapApi(nMapNo, Events(strKey).c_str());
	pDoor = Door();
	printf("doorlock api omit msg=%d\n", pDoor ? (int)(strcmp(pDoor->szLockMsg, szMsg) == 0) : -1);

	/* 鍵なしの人が開けようとする */
	MimicPC(pPC, nX, nY, 3);
	pPC->dwSpaceID = SPACE_NORMAL;
	ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
	ZeroMemory(pPC->waItem, sizeof(pPC->waItem));
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_OPENDOOR;
	Para.pID = pPC->pID;
	Para.dwTblID = pPC->dwTblID;
	auto Try = [&](const char *pszFind1, const char *pszFind2, const char *pszLabel) {
		int nMsg0 = MockCmdCount(nConn, CMD_MAPMSG);
		MockTrace(1);
		TestRecv(pPC, &Para, sizeof(Para));
		int nHit1 = MockHasText(nConn, CMD_MAPMSG, pszFind1), nHit2 = MockHasText(nConn, CMD_MAPMSG, pszFind2);
		MockTrace(0);
		PEVENTINFO_DOOR pD = Door();
		printf("doorlock %s mapmsg=%d text=%d other=%d open=%d\n", pszLabel, MockCmdCount(nConn, CMD_MAPMSG) - nMsg0, nHit1, nHit2,
			(int)(pD && SpaceDoorIsOpen(dwMapID, DoorExtraFind(pMap, xD, yD), pD, SPACE_NORMAL)));
	};
	{
		std::string strWithAt = std::string(szMsg) + "@2";
		Try(strWithAt.c_str(), szDefault, "nokey custom");
	}
	/* 文の末尾に @2 が最初からあれば二重に付けない */
	SwapApi(nMapNo, Events(strKey + ",\"szLockMsg\":\"カルバの話を聞いてからだ。@2\"").c_str());
	Try("だ。@2", "@2@2", "nokey with@2");
	/* 空なら既定文 */
	SwapApi(nMapNo, Events(strKey + ",\"szLockMsg\":\"\"").c_str());
	pDoor = Door();
	printf("doorlock api empty msg_empty=%d\n", pDoor ? (int)(pDoor->szLockMsg[0] == 0) : -1);
	Try(szDefault, szMsg, "nokey default");

	/* 鍵があれば今までどおり開く（鍵なしの文は出ない） */
	SwapApi(nMapNo, Events(strKey + ",\"szLockMsg\":\"カルバの話を聞いてからだ。\"").c_str());
	AddCharItem(pPC->dwTblID, wKey);
	Try("で扉を開けた。@2", szMsg, "withkey");
	ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
	ZeroMemory(pPC->waItem, sizeof(pPC->waItem));

	/* 管理者ウィンドウからの CMD_CHGEVENT：文を知らない短い扉は文が残る・新しい大きさなら反映される */
	{
		pPC->byAdminLV = ADMINLV_ADMINISTRATOR;
		PEVENTINFO pEvent = DoorExtraFind(pMap, xD, yD);
		int nNo = (int)(pEvent - pMap->pEvent);
		EVENTINFO_DOOR New;
		ZeroMemory(&New, sizeof(New));
		New.wGrpID = 33;
		New.wBackGrpID = 44;
		New.byTimeClose = 50;
		New.byType = EVENTDOOTTYPE_KEY;
		New.wKeyItem = wKey;
		New.wEnemyStTmp = 65535;
		const size_t nShortNoExtra = offsetof(EVENTINFO_DOOR, byExtraCount);
		const size_t nShortNoLock = offsetof(EVENTINFO_DOOR, szLockMsg);
		auto Send = [&](const EVENTINFO_DOOR &Src, size_t nBody) {
			std::vector<BYTE> aPacket(sizeof(CMDPARA_CHGEVENT) + nBody, 0xCC);
			PCMDPARA_CHGEVENT pPara = (PCMDPARA_CHGEVENT)&aPacket[0];
			ZeroMemory(pPara, sizeof(CMDPARA_CHGEVENT));
			pPara->byCmd = CMD_CHGEVENT;
			pPara->dwID = dwMapID;
			pPara->dwDataID = (DWORD)nNo;
			pPara->dwX = (DWORD)xD;
			pPara->dwY = (DWORD)yD;
			pPara->byType = EVENTTYPE_DOOR;
			pPara->pData = (PVOID)1;
			memcpy(&aPacket[sizeof(CMDPARA_CHGEVENT)], &Src, nBody);
			TestRecv(pPC, &aPacket[0], aPacket.size());
			PEVENTINFO_DOOR pD = Door();
			return pD;
		};
		/* 追加マスも文も知らない短い扉 */
		pDoor = Send(New, nShortNoExtra);
		printf("doorlock admin short noextra msg=%d changed=%d\n", pDoor ? (int)(strcmp(pDoor->szLockMsg, szMsg) == 0) : -1, pDoor ? (int)(pDoor->wGrpID == 33 && pDoor->byTimeClose == 50) : -1);
		/* 追加マスまで知っていて文を知らない扉 */
		New.wGrpID = 35;
		pDoor = Send(New, nShortNoLock);
		printf("doorlock admin short noroom msg=%d changed=%d\n", pDoor ? (int)(strcmp(pDoor->szLockMsg, szMsg) == 0) : -1, pDoor ? (int)(pDoor->wGrpID == 35) : -1);
		/* 新しい大きさで、文が空 → 消える */
		New.wGrpID = 36;
		pDoor = Send(New, sizeof(New));
		printf("doorlock admin full empty msg_empty=%d changed=%d\n", pDoor ? (int)(pDoor->szLockMsg[0] == 0) : -1, pDoor ? (int)(pDoor->wGrpID == 36) : -1);
		/* 新しい大きさで、文あり → 反映（終端が無い長い文は 95 バイトで切る） */
		memset(New.szLockMsg, 'b', sizeof(New.szLockMsg));
		pDoor = Send(New, sizeof(New));
		printf("doorlock admin full msg len=%d\n", pDoor ? (int)strlen(pDoor->szLockMsg) : -1);
		ZeroMemory(New.szLockMsg, sizeof(New.szLockMsg));
		strcpy(New.szLockMsg, szMsg);
		pDoor = Send(New, sizeof(New));
		printf("doorlock admin full msg set=%d\n", pDoor ? (int)(strcmp(pDoor->szLockMsg, szMsg) == 0) : -1);
		pPC->byAdminLV = byAdminBack;
	}

	/* 保存して読み直す（読み込みは末尾に足される）。文のある扉と、文が空の扉 */
	{
		extern BOOL SaveMapInfo(void);
		extern BOOL LoadMapInfo(void);
		int nMaps = g_aMapInfo.GetSize();
		DWORD dwNewMapIDBak = g_dwNewMapID;
		BOOL bSaved = SaveMapInfo();
		BOOL bLoaded = LoadMapInfo();
		int nAdded = g_aMapInfo.GetSize() - nMaps;
		if (nAdded >= 1) {
			PMAPINFO pL = g_aMapInfo[nMaps + nMapNo];
			PEVENTINFO pE = DoorExtraFind(pL, xD, yD);
			PEVENTINFO_DOOR pD = pE ? (PEVENTINFO_DOOR)pE->pData : NULL;
			printf("doorlock save=%d load=%d added=%d msg=%d type=%d key=%d\n", (int)bSaved, (int)bLoaded, nAdded,
				pD ? (int)(strcmp(pD->szLockMsg, szMsg) == 0) : -1, pD ? (int)pD->byType : -1, pD ? (int)(pD->wKeyItem == wKey) : -1);
			/* 文を空にして保存し直すと、読み直した扉も空（古いデータと同じ・既定文） */
			ZeroMemory(((PEVENTINFO_DOOR)DoorExtraFind(pMap, xD, yD)->pData)->szLockMsg, MAX_MAPSWAPMSG);
		} else {
			printf("doorlock save=%d load=%d added=%d\n", (int)bSaved, (int)bLoaded, nAdded);
		}
		for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
			g_aMapInfo.RemoveAt(i);
		}
		g_dwNewMapID = dwNewMapIDBak;
		nMaps = g_aMapInfo.GetSize();
		bSaved = SaveMapInfo();
		bLoaded = LoadMapInfo();
		nAdded = g_aMapInfo.GetSize() - nMaps;
		if (nAdded >= 1) {
			PEVENTINFO pE = DoorExtraFind(g_aMapInfo[nMaps + nMapNo], xD, yD);
			PEVENTINFO_DOOR pD = pE ? (PEVENTINFO_DOOR)pE->pData : NULL;
			printf("doorlock save empty=%d type=%d\n", pD ? (int)(pD->szLockMsg[0] == 0) : -1, pD ? (int)pD->byType : -1);
		}
		for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
			g_aMapInfo.RemoveAt(i);
		}
		g_dwNewMapID = dwNewMapIDBak;
	}

	/* 片付け */
	DoorExtraRestoreEvents(pMap, aBackup);
	memcpy(pPC->waImportant, awImpBack, sizeof(awImpBack));
	memcpy(pPC->waItem, awItemBack, sizeof(awItemBack));
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	pPC->byDirection = byDirBack;
	pPC->dwSpaceID = dwSpaceBack;
	pPC->byAdminLV = byAdminBack;
	AddTblNoPC((int)pPC->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* AB. 大事な物の欄と鍵の扉												 */
/*    印の品・依頼の品が手持ちではなく大事な物の欄に入るか（同じ物を重ねない・	 */
/*    いっぱいなら入らない）・保存して読み直しても残るか・列が無い古いデータは空か */
/*    CountItem/TakeItem が欄も見るか・手持ちの大事な物を移し替えるか（装備は動かさず */
/*    添字がずれないか）・鍵の扉が鍵なしで開かず、鍵ありで開いて鍵は減らないか	 */
/* ------------------------------------------------------------------------- */
static int ImportantCountOf(PCHARINFO p, WORD w)
{
	int n = 0;
	for (int i = 0; i < MAX_IMPORTANT; i ++) if (p->waImportant[i] == w) n ++;
	return n;
}

static void TestImportant(const std::vector<int> &aLogin)
{
	printf("== AB. Important ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	int nMapNo = GetMapNo(pPC->dwMapID);
	if (nMapNo < 0) {
		printf("no map\n");
		return;
	}
	/* 使う品（名前があり、依頼の品でも印付きでもない物を 4 つ） */
	WORD awItem[4] = {0, 0, 0, 0};
	int nFound = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && nFound < 4; i ++) {
		if (g_aItemInfo[i]->szName[0] == 0 || IsImportantItem((WORD)i)) continue;
		awItem[nFound ++] = (WORD)i;
	}
	if (nFound < 4) {
		printf("no items\n");
		return;
	}
	WORD wKey = awItem[0], wNorm = awItem[1], wQuest = awItem[2], wEqu = awItem[3];
	BYTE byFlgBack = g_aItemInfo[wKey]->byFlg;
	WORD awItemBack[MAX_MYITEM], awEquBack[MAX_EQUTYPE], awImpBack[MAX_IMPORTANT];
	memcpy(awItemBack, pPC->waItem, sizeof(awItemBack));
	memcpy(awEquBack, pPC->waEqu, sizeof(awEquBack));
	memcpy(awImpBack, pPC->waImportant, sizeof(awImpBack));
	ZeroMemory(pPC->waItem, sizeof(pPC->waItem));
	ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
	FillMemory(pPC->waEqu, sizeof(pPC->waEqu), 0xFF);

	/* 印の品・依頼の品・ふつうの品の振り分け（同じ物は重ねず 1 個 1 枠） */
	g_aItemInfo[wKey]->byFlg |= ITEMFLG_IMPORTANT;
	PQUESTINFO pQ = new QUESTINFO;
	ZeroMemory(pQ, sizeof(*pQ));
	pQ->wNeedFlag = pQ->wNotFlag = FLAG_NOBIT;
	pQ->wID = 9999; pQ->byState = QUESTSTATE_ON; pQ->wQuestItem = wQuest;
	g_aQuestInfo.Add(pQ);
	AddCharItem(pPC->dwTblID, wKey);
	AddCharItem(pPC->dwTblID, wKey);
	AddCharItem(pPC->dwTblID, wQuest);
	AddCharItem(pPC->dwTblID, wNorm);
	printf("important add bag=%d imp=%d key=%d quest=%d norm=%d\n", GetItemCount(pPC->dwTblID), GetImportantCount(pPC->dwTblID),
		ImportantCountOf(pPC, wKey), ImportantCountOf(pPC, wQuest), ImportantCountOf(pPC, wNorm));

	/* 数える・取り上げる（大事な物の欄も見る。手持ちの品も今までどおり） */
	printf("important count key=%d quest=%d norm=%d\n", QuestCountItemAll(pPC, wKey), QuestCountItemAll(pPC, wQuest), QuestCountItemAll(pPC, wNorm));
	BOOL bTake1 = QuestTakeItemOne(pPC, wKey), bTake2 = QuestTakeItemOne(pPC, wQuest), bTake3 = QuestTakeItemOne(pPC, wNorm), bTake4 = QuestTakeItemOne(pPC, wQuest);
	printf("important take %d%d%d%d bag=%d imp=%d key=%d\n", (int)bTake1, (int)bTake2, (int)bTake3, (int)bTake4,
		GetItemCount(pPC->dwTblID), GetImportantCount(pPC->dwTblID), QuestCountItemAll(pPC, wKey));

	/* 大事な物の欄がいっぱいなら受け取れない（手持ちは別） */
	for (int i = 0; i < MAX_IMPORTANT; i ++) pPC->waImportant[i] = wKey;
	printf("important full canKey=%d canNorm=%d\n", (int)CanAddCharItem(pPC->dwTblID, wKey), (int)CanAddCharItem(pPC->dwTblID, wNorm));
	AddCharItem(pPC->dwTblID, wKey);
	printf("important full add imp=%d bag=%d\n", GetImportantCount(pPC->dwTblID), GetItemCount(pPC->dwTblID));

	/* 保存して読み直す（読み込みは保存済みの個数分だけ読む。列が無い古いデータは ZeroMemory のまま空） */
	ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
	pPC->waImportant[0] = wKey; pPC->waImportant[1] = wQuest; pPC->waImportant[2] = wKey;
	char szAccBack[MAX_ACCNAME], szNameBack[MAX_CHARNAMEBUF];
	memcpy(szAccBack, pPC->szAcc, sizeof(szAccBack));
	memcpy(szNameBack, pPC->szName, sizeof(szNameBack));
	strcpy(pPC->szAcc, "imptest");
	strcpy(pPC->szName, "大事な物テスト");
	BOOL bSaved = SaveCharInfo();
	BOOL bLoaded = LoadCharInfo();
	PCHARINFO pLoaded = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (g_aCharInfo[i] != pPC && strcmp(g_aCharInfo[i]->szAcc, "imptest") == 0) pLoaded = g_aCharInfo[i];
	}
	printf("important save=%d load=%d found=%d\n", (int)bSaved, (int)bLoaded, pLoaded ? 1 : 0);
	if (pLoaded) {
		printf("important reload imp=%d key=%d quest=%d first=%d,%d,%d\n", GetImportantCount(pLoaded->dwTblID), ImportantCountOf(pLoaded, wKey),
			ImportantCountOf(pLoaded, wQuest), pLoaded->waImportant[0] == wKey, pLoaded->waImportant[1] == wQuest, pLoaded->waImportant[2] == wKey);
		strcpy(pLoaded->szAcc, "imptestX");		/* 読み込みで増えた分は、次の探し物の邪魔にならないよう名前を変える */
	}
	memcpy(pPC->szAcc, szAccBack, sizeof(szAccBack));
	memcpy(pPC->szName, szNameBack, sizeof(szNameBack));

	/* 古いキャラ：手持ちにある印の品・依頼の品を欄へ移す（装備中は動かさず、添字がずれない） */
	ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
	ZeroMemory(pPC->waItem, sizeof(pPC->waItem));
	FillMemory(pPC->waEqu, sizeof(pPC->waEqu), 0xFF);
	pPC->waItem[0] = wNorm; pPC->waItem[1] = wKey; pPC->waItem[2] = wEqu; pPC->waItem[3] = wKey; pPC->waItem[4] = wQuest;
	g_aItemInfo[wKey]->byFlg |= ITEMFLG_IMPORTANT;
	pPC->waEqu[0] = 3;		/* 2 つ目の鍵を装備中 */
	pPC->waEqu[1] = 2;
	int nMoved = MoveImportantItems(pPC);
	printf("important move moved=%d bag=%d,%d,%d,%d,%d imp=%d key=%d quest=%d equ=%d,%d\n", nMoved,
		pPC->waItem[0] == wNorm, pPC->waItem[1] == wEqu, pPC->waItem[2] == wKey, pPC->waItem[3] == 0, pPC->waItem[4] == 0,
		GetImportantCount(pPC->dwTblID), ImportantCountOf(pPC, wKey), ImportantCountOf(pPC, wQuest), pPC->waEqu[0], pPC->waEqu[1]);
	printf("important move again=%d\n", MoveImportantItems(pPC));

	/* 鍵の扉 */
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	int nX = -1, nY = -1;
	for (int y = 0; y < pMap->nCY && nX < 0; y ++) {
		for (int x = 0; x + 2 < pMap->nCX; x ++) {
			if (PushIsFree(pMap, x, y) && PushIsFree(pMap, x + 1, y) && PushIsFree(pMap, x + 2, y)) {
				nX = x;
				nY = y;
				break;
			}
		}
	}
	if (nX >= 0) {
		DWORD dwXBack = pPC->dwX, dwYBack = pPC->dwY;
		BYTE byDirBack = pPC->byDirection;
		DWORD dwSpaceBack = pPC->dwSpaceID;
		DWORD dwNowBack = s_dwFakeNow;
		const int nConn = 1000;
		int nEventBack = pMap->nEventCount;
		PEVENTINFO pEventBack = pMap->pEvent;
		pMap->pEvent = new EVENTINFO[nEventBack + 1];
		ZeroMemory(pMap->pEvent, sizeof(EVENTINFO) * (nEventBack + 1));
		if (nEventBack) {
			memcpy(pMap->pEvent, pEventBack, sizeof(EVENTINFO) * nEventBack);
		}
		PEVENTINFO pEvent = &pMap->pEvent[nEventBack];
		EVENTINFO_DOOR Door;
		ZeroMemory(&Door, sizeof(Door));
		Door.wGrpID = 11;
		Door.wBackGrpID = 22;
		Door.byTimeClose = 30;
		Door.byType = EVENTDOOTTYPE_KEY;
		Door.wKeyItem = wKey;
		pEvent->dwX = (DWORD)(nX + 1);
		pEvent->dwY = (DWORD)nY;
		pEvent->byType = EVENTTYPE_DOOR;
		pEvent->pData = (PBYTE)&Door;
		pMap->nEventCount = nEventBack + 1;
		MimicPC(pPC, nX, nY, 3);
		pPC->dwSpaceID = SPACE_NORMAL;

		ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
		ZeroMemory(pPC->waItem, sizeof(pPC->waItem));
		FillMemory(pPC->waEqu, sizeof(pPC->waEqu), 0xFF);
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_OPENDOOR;
		Para.pID = pPC->pID;
		Para.dwTblID = pPC->dwTblID;
		/* 鍵なし：開かず「鍵がかかっている」 */
		int nMsg0 = MockCmdCount(nConn, CMD_MAPMSG), nSnd0 = MockCmdCount(nConn, CMD_PLAYSOUND);
		TestRecv(pPC, &Para, sizeof(Para));
		printf("keydoor nokey open=%d mapmsg=%d sound=%d\n", EnemyDoorOpen(pPC, pEvent, &Door),
			MockCmdCount(nConn, CMD_MAPMSG) - nMsg0, MockCmdCount(nConn, CMD_PLAYSOUND) - nSnd0);
		MimicExamine(pPC);
		printf("keydoor nokey examine open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));
		/* 傭兵などは鍵を持たないので開けられない */
		printf("keydoor force=%d\n", (int)SpaceDoorOpen(pPC->dwMapID, pEvent, &Door, SPACE_NORMAL));
		/* 鍵あり（大事な物の欄）：開く。鍵は減らない */
		AddCharItem(pPC->dwTblID, wKey);
		nMsg0 = MockCmdCount(nConn, CMD_MAPMSG); nSnd0 = MockCmdCount(nConn, CMD_PLAYSOUND);
		int nChg0 = MockCmdCount(nConn, CMD_CHGMAP);
		TestRecv(pPC, &Para, sizeof(Para));
		printf("keydoor key open=%d mapmsg=%d sound=%d chgmap=%d keys=%d\n", EnemyDoorOpen(pPC, pEvent, &Door),
			MockCmdCount(nConn, CMD_MAPMSG) - nMsg0, MockCmdCount(nConn, CMD_PLAYSOUND) - nSnd0,
			MockCmdCount(nConn, CMD_CHGMAP) - nChg0, ImportantCountOf(pPC, wKey));
		/* 開いた後に閉まる動きは普通の扉と同じ。手持ちに鍵があっても開く */
		s_dwFakeNow += 60000;
		g_dwTime = s_dwFakeNow;
		SpaceDoorProc(pPC->dwMapID, pEvent, &Door);
		printf("keydoor closed=%d\n", 1 - EnemyDoorOpen(pPC, pEvent, &Door));
		ZeroMemory(pPC->waImportant, sizeof(pPC->waImportant));
		AddCharItemBag(pPC->dwTblID, wKey);
		MimicExamine(pPC);
		printf("keydoor bagkey open=%d\n", EnemyDoorOpen(pPC, pEvent, &Door));

		/* 鍵の品の番号がマップの保存データに書かれる（サーバーを再起動しても鍵の扉のまま） */
		{
			int nMapNo = -1;
			for (int m = 0; m < g_aMapInfo.GetSize(); m ++) if (g_aMapInfo[m] == pMap) nMapNo = m;
			SaveMapInfo();
			char szMapFile[64];
			sprintf(szMapFile, "SBODATA/SboDataMapInfo%03d.dat", nMapNo);
			std::vector<BYTE> aMapFile;
			FILE *fpMap = fopen(szMapFile, "rb");
			if (fpMap) {
				BYTE abyBuf[4096];
				size_t n;
				while ((n = fread(abyBuf, 1, sizeof(abyBuf), fpMap)) > 0) aMapFile.insert(aMapFile.end(), abyBuf, abyBuf + n);
				fclose(fpMap);
			}
			int nSaved = 0;
			const char szLabel[] = "wKeyItem";
			for (size_t k = 0; k + sizeof(szLabel) + 4 + 2 <= aMapFile.size(); k ++) {
				if (memcmp(&aMapFile[k], szLabel, sizeof(szLabel)) == 0) {
					WORD wSaved = 0;
					memcpy(&wSaved, &aMapFile[k + sizeof(szLabel) + 4], 2);
					if (wSaved == wKey) nSaved = 1;
				}
			}
			printf("keydoor saved=%d\n", nSaved);
		}

		pMap->nEventCount = nEventBack;
		delete [] pMap->pEvent;
		pMap->pEvent = pEventBack;
		s_dwFakeNow = dwNowBack;
		g_dwTime = s_dwFakeNow;
		DeleteTblNoPC((int)pPC->dwTblID);
		pPC->dwX = pPC->dwXBack = dwXBack;
		pPC->dwY = pPC->dwYBack = dwYBack;
		pPC->byDirection = byDirBack;
		pPC->dwSpaceID = dwSpaceBack;
		AddTblNoPC((int)pPC->dwTblID);
	}

	/* 片付け */
	g_aQuestInfo.RemoveAt(g_aQuestInfo.GetSize() - 1);
	delete pQ;
	g_aItemInfo[wKey]->byFlg = byFlgBack;
	memcpy(pPC->waItem, awItemBack, sizeof(awItemBack));
	memcpy(pPC->waEqu, awEquBack, sizeof(awEquBack));
	memcpy(pPC->waImportant, awImpBack, sizeof(awImpBack));
}

static void TestCharPush(const std::vector<int> &aLogin)
{
	printf("== O. CharPush ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	/* 上下左右 2 マスずつ空いている、普通の大きさの押せるNPCを探す */
	PCHARINFO pBlock = NULL;
	PMAPINFO pMap = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pBlock; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p->byNpcType != NPCTYPE_BLOCK || p->wGrpMain != CHARGRP_MAIN_NORMAL || GetMapNo(p->dwMapID) < 0) {
			continue;
		}
		PMAPINFO pM = g_aMapInfo[GetMapNo(p->dwMapID)];
		BOOL bOK = TRUE;
		for (int d = 0; d < 4 && bOK; d ++) {
			for (int n = 1; n <= 2 && bOK; n ++) {
				bOK = PushIsFree(pM, (int)p->dwX + anPosAddTblX[d] * n, (int)p->dwY + anPosAddTblY[d] * n);
			}
		}
		if (bOK) {
			pBlock = p;
			pMap = pM;
		}
	}
	if (!pBlock) {
		printf("no block\n");
		return;
	}
	PCHARINFO pPC = g_aCharInfo[aLogin[0]];
	DWORD dwMapBack = pPC->dwMapID, dwXBack = pPC->dwX, dwYBack = pPC->dwY;
	BYTE byDirBack = pPC->byDirection;
	DWORD dwBX = pBlock->dwX, dwBY = pBlock->dwY;
	BYTE byBDirBack = pBlock->byDirection;
	BYTE byFlgBack = pPC->byFlg, byBFlgBack = pBlock->byFlg;
	DWORD dwBDotBack = pBlock->dwLastDotMove;

	/* 押す人の立つ側(押す向きの反対側)と、サーバーに残っている前の向き */
	static const BYTE s_abyCase[][2] = {
		{ 3, 0 },	/* 左から右へ押す・前の向きは上 */
		{ 2, 0 },	/* 右から左へ押す・前の向きは上 */
		{ 0, 2 },	/* 下から上へ押す・前の向きは左 */
		{ 1, 3 },	/* 上から下へ押す・前の向きは右 */
		{ 3, 3 },	/* 向きが合っている時は今までどおり */
	};
	for (size_t c = 0; c < sizeof(s_abyCase) / sizeof(s_abyCase[0]); c ++) {
		BYTE byPush = s_abyCase[c][0];
		DeleteTblNoPC((int)pBlock->dwTblID);
		pBlock->dwX = pBlock->dwXBack = dwBX;
		pBlock->dwY = pBlock->dwYBack = dwBY;
		pBlock->dwLastDotMove = 0;
		AddTblNoPC((int)pBlock->dwTblID);
		DeleteTblNoPC((int)pPC->dwTblID);
		pPC->dwMapID = pBlock->dwMapID;
		pPC->dwX = pPC->dwXBack = dwBX - anPosAddTblX[byPush];
		pPC->dwY = pPC->dwYBack = dwBY - anPosAddTblY[byPush];
		pPC->byDirection = s_abyCase[c][1];
		AddTblNoPC((int)pPC->dwTblID);

		CMDPARA_CHARPUSH Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_CHARPUSH;
		Para.pID = pPC->pID;
		Para.dwTblID = pPC->dwTblID;
		Para.dwTblIDPush = pBlock->dwTblID;
		TestRecv(pPC, &Para, sizeof(Para));
		printf("charpush want=%d old=%d -> move=(%d,%d) pcdir=%d blockdir=%d\n",
			byPush, s_abyCase[c][1],
			(int)pBlock->dwX - (int)dwBX, (int)pBlock->dwY - (int)dwBY,
			pPC->byDirection, pBlock->byDirection);
	}

	/* 片付け */
	DeleteTblNoPC((int)pBlock->dwTblID);
	pBlock->dwX = pBlock->dwXBack = dwBX;
	pBlock->dwY = pBlock->dwYBack = dwBY;
	pBlock->byDirection = byBDirBack;
	pBlock->dwLastDotMove = dwBDotBack;
	pBlock->byFlg = byBFlgBack;
	AddTblNoPC((int)pBlock->dwTblID);
	DeleteTblNoPC((int)pPC->dwTblID);
	pPC->dwMapID = dwMapBack;
	pPC->dwX = pPC->dwXBack = dwXBack;
	pPC->dwY = pPC->dwYBack = dwYBack;
	pPC->byDirection = byDirBack;
	pPC->byFlg = byFlgBack;
	AddTblNoPC((int)pPC->dwTblID);
}

/* ========================================================================= */
/* 関数名：	TestStatusUp												 */
/* 内容：	攻撃力・守備力アップ(僧侶の補助特技)							 */
/*			最低でも上がる量・率・かけ直し・時間切れで戻る・敵には効かない	 */
/* ========================================================================= */
static void TestStatusUp(const std::vector<int> &aLogin)
{
	printf("== R. StatusUp ==\n");
	if (aLogin.size() < 2 || g_aMagicInfo.GetSize() <= 8) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	PCHARINFO pB = g_aCharInfo[aLogin[1]];
	if (g_aMapInfo[GetMapNo(pA->dwMapID)]->byFlg & MAPFLG_MAGIC) {
		printf("magic map\n");
		return;
	}
	/* 他人回復(8)の枠を借りて効果を差し替える */
	PMAGICINFO pMagic = g_aMagicInfo[8];
	MAGICINFO MagicBack = *pMagic;
	pMagic->byTarget		= MAGICTARGET_PC;
	pMagic->byTargetType	= MAGICTARGETTYPE_SELECT;
	pMagic->byTargetEfectID	= 0;
	pMagic->byUserEfectID	= 0;
	BYTE byPWBack = pB->byPW, bySTBack = pB->byST;
	DeleteAllTimerEvent(pB);
	RenewATDF(pB->dwTblID);
	int nAT0 = pB->wAT, nDF0 = pB->wDF;
	int nBuff0 = MockCmdCount(1001, CMD_CHARBUFF);		/* B はログインした順で 1001 番 */

	/* 攻撃 25%・最低+3、守り 50%・最低+4、3分 */
	pMagic->byType = MAGICTYPE_ATUP;
	pMagic->wData1 = 25; pMagic->wData2 = 180; pMagic->wData3 = 3;
	BOOL bAT = MagicProcItem(pA->dwTblID, 8, pB->dwTblID);
	pMagic->byType = MAGICTYPE_DFUP;
	pMagic->wData1 = 50; pMagic->wData2 = 180; pMagic->wData3 = 4;
	BOOL bDF = MagicProcItem(pA->dwTblID, 8, pB->dwTblID);
	printf("statusup use=%d,%d at+%d df+%d events=%d buffpkt=%d\n", bAT, bDF,
		(int)pB->wAT - nAT0, (int)pB->wDF - nDF0, pB->aTimerEvent.GetSize(), MockCmdCount(1001, CMD_CHARBUFF) - nBuff0);

	/* 強い人は率で上がる(力を上げて計り直す) */
	pB->byPW = 200;
	pB->byST = 200;
	RenewATDF(pB->dwTblID);
	printf("statusup strong at=%d df=%d\n", pB->wAT, pB->wDF);

	/* かけ直しても重ならない(時間が延びるだけ) */
	s_dwFakeNow += 100000;
	g_dwTime = s_dwFakeNow;
	pMagic->byType = MAGICTYPE_ATUP;
	pMagic->wData1 = 25; pMagic->wData2 = 180; pMagic->wData3 = 3;
	MagicProcItem(pA->dwTblID, 8, pB->dwTblID);
	printf("statusup recast at=%d events=%d\n", pB->wAT, pB->aTimerEvent.GetSize());

	/* 守りは最初から3分で切れ、攻撃はかけ直しから3分まで残る */
	s_dwFakeNow += 81000;
	g_dwTime = s_dwFakeNow;
	CharInfoTimerProc(pB->dwTblID);
	CharInfoTimerProc(pB->dwTblID);
	printf("statusup 181s at=%d df=%d events=%d\n", pB->wAT, pB->wDF, pB->aTimerEvent.GetSize());
	s_dwFakeNow += 100000;
	g_dwTime = s_dwFakeNow;
	CharInfoTimerProc(pB->dwTblID);
	printf("statusup 281s at=%d df=%d events=%d buffpkt=%d\n", pB->wAT, pB->wDF, pB->aTimerEvent.GetSize(), MockCmdCount(1001, CMD_CHARBUFF) - nBuff0);

	/* 敵にはかからない */
	PCHARINFO pEnemy = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pEnemy; i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if ((p->byNpcType == NPCTYPE_BATTLE1 || p->byNpcType == NPCTYPE_BATTLE2) && p->aTimerEvent.GetSize() == 0) {
			pEnemy = p;
		}
	}
	if (pEnemy) {
		WORD wATBack = pEnemy->wAT;
		BOOL bEnemy = MagicProcItem(pA->dwTblID, 8, pEnemy->dwTblID);
		printf("statusup enemy use=%d at_same=%d events=%d\n", bEnemy, pEnemy->wAT == wATBack, pEnemy->aTimerEvent.GetSize());
	}

	/* 片付け */
	*pMagic = MagicBack;
	DeleteAllTimerEvent(pB);
	pB->byPW = byPWBack;
	pB->byST = bySTBack;
	RenewATDF(pB->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* S. 空間番号（同じ場所でも、空間が違えば見えない・ぶつからない・当たらない）*/
/* ------------------------------------------------------------------------- */
static const std::vector<int> *s_pSpLogin;

/* ログインした順に 1000 番から（TestRecv の接続番号） */
static int SpCnt(PCHARINFO p, int nCmd)
{
	for (size_t i = 0; i < s_pSpLogin->size(); i ++) {
		if (g_aCharInfo[(*s_pSpLogin)[i]] == p) return MockCmdCount(1000 + (int)i, nCmd);
	}
	return -1;
}

static int SpEnemyCount(DWORD dwSpace)
{
	int n = 0;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (QuestIsChallengeEnemy(g_aCharInfo[i]) && g_aCharInfo[i]->dwSpaceID == dwSpace) n ++;
	}
	return n;
}

static PCHARINFO SpEnemyFind(DWORD dwSpace)
{
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (QuestIsChallengeEnemy(g_aCharInfo[i]) && g_aCharInfo[i]->dwSpaceID == dwSpace) return g_aCharInfo[i];
	}
	return NULL;
}

static void SpAdminSend(PCHARINFO p, BYTE bySub, DWORD dwSpace)
{
	CMDPARA_ADMINSPACE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_ADMINSPACE;
	Para.byCmdSub = bySub;
	Para.dwSpaceID = dwSpace;
	TestRecv(p, &Para, sizeof(Para));
}

static void SpTarget(PCHARINFO p, DWORD dwTarget, DWORD dwPID)
{
	CMDPARA_DATA3 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_TARGETCHAR;
	Para.pID = dwPID;
	Para.dwTblID = p->dwTblID;
	Para.dwData = dwTarget;
	TestRecv(p, &Para, sizeof(Para));
}

static void TestSpace(const std::vector<int> &aLogin)
{
	printf("== T. Space ==\n");
	if (aLogin.size() < 5) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO pD = g_aCharInfo[aLogin[3]], pE = g_aCharInfo[aLogin[4]];
	PCHARINFO apPC[5] = { pA, pB, pC, pD, pE };
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	s_pSpLogin = &aLogin;

	CHARINFO aBack[5];
	for (int i = 0; i < 5; i ++) {
		TestCharBackup(&aBack[i], apPC[i]);
		PCHARINFO p = apPC[i];
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
		ZeroMemory(p->abyQuestRewarded, sizeof(p->abyQuestRewarded));
		p->byLevel = 30; p->byPW = 99; p->wMaxHP = 500; p->wHP = 480; p->wMaxMana = 90; p->wMana = 80; p->dwGold = 5000;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int k = 0; k < MAX_EQUTYPE; k ++) p->waEqu[k] = 0xFFFF;
	}

	std::vector<std::pair<int, int> > aCell;
	for (int r = 1; r < 12 && aCell.size() < 8; r ++) {
		for (int dy = -r; dy <= r && aCell.size() < 8; dy ++) {
			for (int dx = -r; dx <= r && aCell.size() < 8; dx ++) {
				if (abs(dx) != r && abs(dy) != r) continue;
				int x = (int)pA->dwX + dx, y = (int)pA->dwY + dy;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
	}
	printf("space cells=%d\n", (int)aCell.size());
	if (aCell.size() < 8) {
		return;
	}

	/* --- 同じマスに別の空間の二人：見えない・ぶつからない・ターゲットできない --- */
	QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
	QuestMovePC(pB, nMap, aCell[0].first, aCell[0].second);
	SpaceSet(pB, 1);
	printf("space set b=%u a=%u\n", pB->dwSpaceID, pA->dwSpaceID);
	{
		int a0 = SpCnt(pA, CMD_CHARINFO), b0 = SpCnt(pB, CMD_CHARINFO);
		SendCmdCHARINFO3((int)pB->dwTblID);
		int a1 = SpCnt(pA, CMD_CHARINFO);
		SendCmdCHARINFO3((int)pA->dwTblID);
		printf("space charinfo diff a_got_b=%d b_got_a=%d\n", a1 - a0, SpCnt(pB, CMD_CHARINFO) - b0 - 1);
		printf("space visible a_sees_b=%d b_sees_a=%d same=%d null=%d\n", (int)IsCharVisibleTo(pB, pA), (int)IsCharVisibleTo(pA, pB),
			(int)IsSameSpace(pA, pB), (int)IsSameSpace(pA, NULL));
		/* 同じ空間なら見える */
		SpaceSet(pB, SPACE_NORMAL);
		a0 = SpCnt(pA, CMD_CHARINFO);
		SendCmdCHARINFO3((int)pB->dwTblID);
		printf("space charinfo same a_got_b=%d visible=%d\n", (int)(SpCnt(pA, CMD_CHARINFO) - a0 > 0), (int)IsCharVisibleTo(pB, pA));
		SpaceSet(pB, 1);
	}
	printf("space block s0_is_a=%d s1_is_b=%d s2_none=%d\n",
		(int)(CheckCharBlockPC((DWORD)nMap, aCell[0].first, aCell[0].second, 0) == pA->dwTblID),
		(int)(CheckCharBlockPC((DWORD)nMap, aCell[0].first, aCell[0].second, 1) == pB->dwTblID),
		(int)(CheckCharBlockPC((DWORD)nMap, aCell[0].first, aCell[0].second, 2) == (DWORD)-1));
	pA->dwTargetTblID = (DWORD)-1;
	SpTarget(pA, pB->dwTblID, 0);
	printf("space follow other=%d\n", (int)(pA->dwTargetTblID3 == pB->dwTblID));
	SpaceSet(pB, SPACE_NORMAL);
	SpTarget(pA, pB->dwTblID, 0);
	printf("space follow same=%d\n", (int)(pA->dwTargetTblID3 == pB->dwTblID));
	pA->dwTargetTblID = pA->dwTargetTblID2 = pA->dwTargetTblID3 = (DWORD)-1;
	pB->dwTargetTblID = pB->dwTargetTblID2 = pB->dwTargetTblID3 = (DWORD)-1;
	SpaceSet(pB, 1);

	/* --- 扉：その空間の人にだけ知らせ、閉まるのも空間ごと --- */
	{
		PEVENTINFO pEvBack = pMap->pEvent;
		int nEvBack = pMap->nEventCount, nProcBack = pMap->nProcEvent;
		EVENTINFO aEv[2];
		EVENTINFO_DOOR Door;
		EVENTINFO_ONCHARMAP OnChar;
		ZeroMemory(aEv, sizeof(aEv));
		ZeroMemory(&Door, sizeof(Door));
		ZeroMemory(&OnChar, sizeof(OnChar));
		Door.wGrpID = 100; Door.wBackGrpID = 101; Door.byTimeClose = 10;
		aEv[0].byType = EVENTTYPE_DOOR; aEv[0].pData = &Door;
		aEv[0].dwX = (DWORD)aCell[1].first; aEv[0].dwY = (DWORD)aCell[1].second;
		pMap->pEvent = aEv; pMap->nEventCount = 1; pMap->nProcEvent = 0;

		int a0 = SpCnt(pA, CMD_CHGMAP), b0 = SpCnt(pB, CMD_CHGMAP);
		s_dwFakeNow += 10000; g_dwTime = s_dwFakeNow;
		BOOL bOpen1 = SpaceDoorOpen((DWORD)nMap, &aEv[0], &Door, 1);
		BOOL bOpen1b = SpaceDoorOpen((DWORD)nMap, &aEv[0], &Door, 1);
		printf("space door open1=%d again=%d a_chgmap=%d b_chgmap=%d is_open s0=%d s1=%d\n", (int)bOpen1, (int)bOpen1b,
			SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0,
			(int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 0), (int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 1));
		s_dwFakeNow += 600; g_dwTime = s_dwFakeNow;
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		BOOL bOpen0 = SpaceDoorOpen((DWORD)nMap, &aEv[0], &Door, 0);
		printf("space door open0=%d a_chgmap=%d b_chgmap=%d dwTimeOpen_set=%d\n", (int)bOpen0, SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0, (int)(Door.dwTimeOpen != 0));
		s_dwFakeNow += 600; g_dwTime = s_dwFakeNow;
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		SpaceDoorProc((DWORD)nMap, &aEv[0], &Door);
		printf("space door close1 a_chgmap=%d b_chgmap=%d is_open s0=%d s1=%d\n", SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0,
			(int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 0), (int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 1));
		s_dwFakeNow += 600; g_dwTime = s_dwFakeNow;
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		SpaceDoorProc((DWORD)nMap, &aEv[0], &Door);
		printf("space door close0 a_chgmap=%d b_chgmap=%d is_open s0=%d s1=%d\n", SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0,
			(int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 0), (int)SpaceDoorIsOpen((DWORD)nMap, &aEv[0], &Door, 1));

		/* --- 乗っている間変わるマス：乗った空間にだけ知らせる --- */
		OnChar.wGrpIDOn = 200; OnChar.wGrpIDOff = 201;
		OnChar.dwX = (DWORD)aCell[3].first; OnChar.dwY = (DWORD)aCell[3].second;
		aEv[0].byType = EVENTTYPE_ONCHARMAP; aEv[0].pData = &OnChar;
		aEv[0].dwX = (DWORD)aCell[2].first; aEv[0].dwY = (DWORD)aCell[2].second;
		size_t nTarget = OnChar.dwX + OnChar.dwY * pMap->nCX;
		WORD wMapBack = pMap->pwMap[nTarget];
		pMap->pwMap[nTarget] = OnChar.wGrpIDOff;
		QuestMovePC(pA, nMap, aCell[5].first, aCell[5].second);
		QuestMovePC(pB, nMap, aCell[2].first, aCell[2].second);
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		SpaceOnCharMapProc(pMap, &aEv[0], &OnChar);
		printf("space onchar b_on a_chgmap=%d b_chgmap=%d map_on=%d\n", SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0, (int)(pMap->pwMap[nTarget] == OnChar.wGrpIDOn));
		QuestMovePC(pB, nMap, aCell[0].first, aCell[0].second);
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		SpaceOnCharMapProc(pMap, &aEv[0], &OnChar);
		printf("space onchar b_off a_chgmap=%d b_chgmap=%d\n", SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0);
		QuestMovePC(pA, nMap, aCell[2].first, aCell[2].second);
		a0 = SpCnt(pA, CMD_CHGMAP); b0 = SpCnt(pB, CMD_CHGMAP);
		SpaceOnCharMapProc(pMap, &aEv[0], &OnChar);
		printf("space onchar a_on a_chgmap=%d b_chgmap=%d map_on=%d\n", SpCnt(pA, CMD_CHGMAP) - a0, SpCnt(pB, CMD_CHGMAP) - b0, (int)(pMap->pwMap[nTarget] == OnChar.wGrpIDOn));
		QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
		SpaceOnCharMapProc(pMap, &aEv[0], &OnChar);
		pMap->pwMap[nTarget] = wMapBack;
		pMap->pEvent = pEvBack; pMap->nEventCount = nEvBack; pMap->nProcEvent = nProcBack;
	}
	SpaceSet(pB, SPACE_NORMAL);
	printf("space reset b=%u\n", pB->dwSpaceID);

	/* --- 討伐チャレンジを二組が同時に：それぞれの空間・それぞれの敵 --- */
	g_aQuestInfo.RemoveAll();
	int nStLoan = g_aStTmpInfo.GetSize(), nStEnemy = nStLoan + 1;
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "貸し出し");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 20;
		g_aStTmpInfo.Add(t);
		t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "試しの魔物");
		t->byLevel = 3; t->byPW = 5; t->byNpcType = NPCTYPE_MOVERAND; t->wMaxHP = 30;
		g_aStTmpInfo.Add(t);
		PQUESTINFO p = new QUESTINFO;
		ZeroMemory(p, sizeof(*p));
		p->wNeedFlag = p->wNotFlag = FLAG_NOBIT;
		p->wID = 20; p->byState = QUESTSTATE_ON; p->byGuild = 0; p->byStepCount = 2;
		strcpy(p->szName, "試しの討伐");
		p->dwGold = 100; p->dwExp = 10;
		p->byChallenge = QUESTCHALLENGE_OTHERWORLD; p->wLoanStTmp = (WORD)nStLoan; p->byMemberMin = 2;
		for (int k = 0; k < MAX_QUESTNPC; k ++) { p->aNpc[k].byStepMax = QUESTNPC_NOSTEP; p->aNpc[k].byNextStep = QUESTNPC_NOSTEP; }
		p->aNpc[0].dwMapID = pMap->dwID; p->aNpc[0].byX = (BYTE)aCell[0].first; p->aNpc[0].byY = (BYTE)aCell[0].second;
		p->aNpc[0].byKind = QUESTNPCKIND_GATE; p->aNpc[0].wGrpID = 4; strcpy(p->aNpc[0].szName, "入口");
		p->aNpc[0].dwDestMapID = pMap->dwID; p->aNpc[0].byDestX = (BYTE)aCell[3].first; p->aNpc[0].byDestY = (BYTE)aCell[3].second;
		p->aNpc[1].dwMapID = pMap->dwID; p->aNpc[1].byX = (BYTE)aCell[2].first; p->aNpc[1].byY = (BYTE)aCell[2].second;
		p->aNpc[1].byKind = QUESTNPCKIND_ENEMY; p->aNpc[1].wGrpID = 5; p->aNpc[1].wStTmp = (WORD)nStEnemy;
		g_aQuestInfo.Add(p);
	}
	PCHARINFO pGuild = new CHARINFO;
	ZeroMemory((PBYTE)pGuild, sizeof(CHARINFO));
	pGuild->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pGuild);
	pGuild->byGameState = GAMESTATE_NPC;
	pGuild->byCharState = CHARSTATE_MOVE;
	pGuild->dwMapID = nMap;
	pGuild->dwX = pGuild->dwXBack = pGuild->dwStartX = (DWORD)aCell[4].first;
	pGuild->dwY = pGuild->dwYBack = pGuild->dwStartY = (DWORD)aCell[4].second;
	strcpy(pGuild->szName, "ギルド受付");
	pGuild->pszNpcSpeech = new char[8];
	strcpy(pGuild->pszNpcSpeech, "@g0");
	pGuild = AddChar(pGuild);
	pGuild->pID = pGuild->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pGuild);
	}
	DWORD dwGuild = pGuild->dwTblID;
	int nGX = aCell[4].first, nGY = aCell[4].second;

	QuestMovePC(pA, nMap, nGX, nGY + 1);
	QuestMovePC(pB, nMap, nGX + 1, nGY + 1);
	QuestMovePC(pC, nMap, nGX - 1, nGY + 1);
	QuestMovePC(pD, nMap, nGX + 2, nGY + 1);
	QuestMovePC(pE, nMap, aCell[6].first, aCell[6].second);
	QuestSendParty(pA, pB, CMDPARTY_JOIN);
	QuestSendParty(pB, pA, CMDPARTY_JOINOK);
	QuestSendParty(pC, pD, CMDPARTY_JOIN);
	QuestSendParty(pD, pC, CMDPARTY_JOINOK);
	QuestSendCmd(pA, QUESTCMD_ACCEPT, 20, dwGuild);
	QuestSendCmd(pC, QUESTCMD_ACCEPT, 20, dwGuild);
	printf("space accept a=%u b=%u c=%u d=%u\n", pA->wQuestID, pB->wQuestID, pC->wQuestID, pD->wQuestID);
	QuestSyncAll();
	PCHARINFO pGateA = QuestFindCopy(pA, 4), pGateC = QuestFindCopy(pC, 4);
	printf("space gate a=%d c=%d\n", (int)(pGateA != NULL), (int)(pGateC != NULL));
	if (pGateA && pGateC) {
		QuestSendTalk(pA, pGateA->dwTblID);
		printf("space group1 in a=%d b=%d enemies=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), ChallengeCountEnemy());
		/* 一組目が遊んでいても二組目は入れる（以前は「別の組が挑戦しています」で断っていた） */
		QuestSendTalk(pC, pGateC->dwTblID);
		printf("space group2 in c=%d d=%d enemies=%d\n", (int)QuestInChallenge(pC), (int)QuestInChallenge(pD), ChallengeCountEnemy());
		DWORD dwS1 = pA->dwSpaceID, dwS2 = pC->dwSpaceID;
		printf("space ids a_nonzero=%d b_same_a=%d d_same_c=%d groups_differ=%d\n", (int)(dwS1 != 0), (int)(pB->dwSpaceID == dwS1), (int)(pD->dwSpaceID == dwS2), (int)(dwS1 != dwS2));
		printf("space enemies g1=%d g2=%d\n", SpEnemyCount(dwS1), SpEnemyCount(dwS2));
		PCHARINFO pEn1 = SpEnemyFind(dwS1), pEn2 = SpEnemyFind(dwS2);
		if (pEn1 && pEn2) {
			printf("space enemy visible a_own=%d a_other=%d c_own=%d c_other=%d\n", (int)IsCharVisibleTo(pEn1, pA), (int)IsCharVisibleTo(pEn2, pA),
				(int)IsCharVisibleTo(pEn2, pC), (int)IsCharVisibleTo(pEn1, pC));
			/* 目の前に他の組の敵だけ・自分の組の敵だけがいる時に殴る：他の組の敵には当たらない */
			BYTE byFlgBack = pMap->byFlg;
			pMap->byFlg |= MAPFLG_BATTLE;
			DWORD dwEx = pA->dwX + anPosAddTblX[1], dwEy = pA->dwY + anPosAddTblY[1];
			WORD wHP1 = pEn1->wHP, wHP2 = pEn2->wHP;
			DWORD dwX1 = pEn1->dwX, dwY1 = pEn1->dwY, dwX2 = pEn2->dwX, dwY2 = pEn2->dwY;
			pEn2->dwX = pEn2->dwXBack = dwEx; pEn2->dwY = pEn2->dwYBack = dwEy;
			for (int k = 0; k < 8; k ++) {
				s_dwFakeNow += 3000; g_dwTime = s_dwFakeNow;
				pA->byDirection = 1; pA->dwLastBattle = 1; pA->dwTargetTblID = pEn2->dwTblID;
				BattleProcCheck(pA);
			}
			printf("space attack other_enemy hp_same=%d\n", (int)(pEn2->wHP == wHP2));
			pEn2->dwX = pEn2->dwXBack = dwX2; pEn2->dwY = pEn2->dwYBack = dwY2;
			pEn1->dwX = pEn1->dwXBack = dwEx; pEn1->dwY = pEn1->dwYBack = dwEy;
			for (int k = 0; k < 8; k ++) {
				s_dwFakeNow += 3000; g_dwTime = s_dwFakeNow;
				pA->byDirection = 1; pA->dwLastBattle = 1; pA->dwTargetTblID = pEn1->dwTblID;
				BattleProcCheck(pA);
				if (pEn1->wHP < wHP1) break;
			}
			printf("space attack own_enemy hp_down=%d\n", (int)(pEn1->wHP < wHP1));
			pEn1->dwX = pEn1->dwXBack = dwX1; pEn1->dwY = pEn1->dwYBack = dwY1;
			pEn1->wHP = wHP1;
			pMap->byFlg = byFlgBack;
			pA->dwTargetTblID = (DWORD)-1;
		}
		/* 同じ場所（入口の行き先）でもぶつからない */
		printf("space dest same_tile=%d\n", (int)(pA->dwX == pC->dwX && pA->dwY == pC->dwY));

		/* 管理者の切り替え：一覧・切り替え・戻す */
		pE->byAdminLV = ADMINLV_ADMINISTRATOR;
		{
			int e0 = SpCnt(pE, CMD_ADMINSPACE);
			SpAdminSend(pE, ADMINSPACE_LISTREQ, 0);
			printf("space admin list reply=%d space=%u\n", SpCnt(pE, CMD_ADMINSPACE) - e0, pE->dwSpaceID);
			PCHARINFO pObs = pE;
			(void)pObs;
			ADMINSPACEENTRY aEntry[MAX_ADMINSPACE];
			int nList = SpaceGetList(aEntry, MAX_ADMINSPACE);
			printf("space list count=%d first_member=%u quest=%u\n", nList, nList > 0 ? aEntry[0].wMember : 0, nList > 0 ? aEntry[0].wQuestID : 0);
			int nMapI = SpCnt(pE, CMD_MAPINFO);
			SpAdminSend(pE, ADMINSPACE_SET, 99999);
			printf("space admin set invalid space=%u\n", pE->dwSpaceID);
			int a0 = SpCnt(pA, CMD_CHARINFO), e1 = SpCnt(pE, CMD_CHARINFO);
			SpAdminSend(pE, ADMINSPACE_SET, dwS1);
			printf("space admin set g1 ok=%d mapinfo_resent=%d a_saw_e=%d e_sees_a=%d e_sees_c=%d e_sees_enemy1=%d e_sees_enemy2=%d\n", (int)(pE->dwSpaceID == dwS1),
				(int)(SpCnt(pE, CMD_MAPINFO) > nMapI), (int)(SpCnt(pA, CMD_CHARINFO) > a0), (int)IsCharVisibleTo(pA, pE), (int)IsCharVisibleTo(pC, pE),
				(int)(pEn1 && IsCharVisibleTo(pEn1, pE)), (int)(pEn2 && IsCharVisibleTo(pEn2, pE)));
			(void)e1;
			int c0 = SpCnt(pC, CMD_CHARINFO);
			SpAdminSend(pE, ADMINSPACE_SET, dwS2);
			printf("space admin set g2 ok=%d c_saw_e=%d e_sees_a=%d e_sees_c=%d\n", (int)(pE->dwSpaceID == dwS2), (int)(SpCnt(pC, CMD_CHARINFO) > c0),
				(int)IsCharVisibleTo(pA, pE), (int)IsCharVisibleTo(pC, pE));
			SpAdminSend(pE, ADMINSPACE_SET, SPACE_NORMAL);
			printf("space admin back space=%u sees_a=%d\n", pE->dwSpaceID, (int)IsCharVisibleTo(pA, pE));
			/* 討伐チャレンジ中の人は切り替えられない（管理者でも） */
			pE->byAdminLV = ADMINLV_USER;
		}

		/* 一組目だけ敵を倒す：二組目はそのまま */
		if (pEn1) {
			DeleteChar(pEn1->dwTblID);
			QuestChallengeProcNow();
			s_dwFakeNow += 2000; QuestChallengeProcNow();
			s_dwFakeNow += 1000; QuestChallengeProcNow();
			printf("space clear g1 in a=%d b=%d c=%d d=%d a_space=%u enemies g2=%d total=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB),
				(int)QuestInChallenge(pC), (int)QuestInChallenge(pD), pA->dwSpaceID, SpEnemyCount(dwS2), ChallengeCountEnemy());
			printf("space clear g1 step=%u flag16=%d c_step=%u\n", pA->byQuestStep, QuestFlagCheck(pA, 16), pC->byQuestStep);
		}
		if (pEn2) {
			DeleteChar(pEn2->dwTblID);
			QuestChallengeProcNow();
			s_dwFakeNow += 2000; QuestChallengeProcNow();
			s_dwFakeNow += 1000; QuestChallengeProcNow();
			printf("space clear g2 in c=%d d=%d c_space=%u enemies=%d\n", (int)QuestInChallenge(pC), (int)QuestInChallenge(pD), pC->dwSpaceID, ChallengeCountEnemy());
			printf("space clear g2 step=%u flag16=%d\n", pC->byQuestStep, QuestFlagCheck(pC, 16));
		}
	}

	/* 片付け */
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	QuestSendParty(pC, pC, CMDPARTY_BREAKUP);
	for (int i = 0; i < 5; i ++) {
		CHARINFO *pBk = &aBack[i];
		PCHARINFO p = apPC[i];
		SpaceSet(p, SPACE_NORMAL);
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
		ZeroMemory(p->abyQuestRewarded, sizeof(p->abyQuestRewarded));
		p->dwPartyID = 0;
		p->byLevel = pBk->byLevel; p->byPW = pBk->byPW; p->wMaxHP = pBk->wMaxHP; p->wHP = pBk->wHP;
		p->wMaxMana = pBk->wMaxMana; p->wMana = pBk->wMana; p->dwGold = pBk->dwGold; p->dwExp = pBk->dwExp;
		p->byCharState = pBk->byCharState; p->byAdminLV = pBk->byAdminLV;
		memcpy(p->waItem, pBk->waItem, sizeof(p->waItem));
		memcpy(p->waEqu, pBk->waEqu, sizeof(p->waEqu));
		QuestMovePC(p, (int)pBk->dwMapID, (int)pBk->dwX, (int)pBk->dwY);
	}
	QuestSyncAll();
	DeleteChar(dwGuild);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nStLoan; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	for (int i = 0; i < g_aQuestInfo.GetSize(); i ++) delete g_aQuestInfo[i];
	g_aQuestInfo.RemoveAll();
	printf("space end enemies=%d copies=%d\n", ChallengeCountEnemy(), QuestCountCopies());
}

/* ========================================================================= */
/* 関数名：	CheckMapNo													 */
/* 内容：	マップ番号の検索結果を確認									 */
/* ========================================================================= */
static void CheckMapNo(DWORD dwID, int nExpected)
{
	int nNo = GetMapNo(dwID);
	if (nNo != nExpected) {
		fprintf(stderr, "GetMapNo(%u): %d != %d\n", dwID, nNo, nExpected);
		abort();
	}
}

/* ========================================================================= */
/* 関数名：	TestMapNo													 */
/* 内容：	マップ配列変更後の検索と、同じIDの繰り返し検索を確認		 */
/* ========================================================================= */
static void TestMapNo(void)
{
	/* ゲームデータの読込前に、独立した配列で確かめる（乱数は使わない） */
	MAPINFO *aMap = new MAPINFO[256];
	ZeroMemory(aMap, sizeof(MAPINFO) * 256);
	CheckMapNo(0, -1);
	for (int i = 0; i < 256; i ++) {
		aMap[i].dwID = 1000 + i;
		g_aMapInfo.Add(&aMap[i]);
	}
	CheckMapNo(1255, 255);
	CheckMapNo(1255, 255);
	CheckMapNo(999, -1);
	CheckMapNo(1255, 255);

	const int NRUN = 1000000;
	int nSum = 0;
	double dStart = NowMs();
	for (int i = 0; i < NRUN; i ++) nSum += GetMapNo(1255);
	printf("BENCH getmapno maps=256 runs=%d sum=%d ms=%.1f\n", NRUN, nSum, NowMs() - dStart);

	/* 同じ要素数での並べ替えとID変更 */
	g_aMapInfo[0] = &aMap[255];
	g_aMapInfo[255] = &aMap[0];
	CheckMapNo(1255, 0);
	aMap[255].dwID = 2000;
	CheckMapNo(1255, -1);
	CheckMapNo(2000, 0);
	/* 先頭への挿入・削除で番号がずれる */
	g_aMapInfo.RemoveAt(0);
	CheckMapNo(1000, 254);
	g_aMapInfo.InsertAt(0, &aMap[255]);
	CheckMapNo(1000, 255);
	g_aMapInfo.RemoveAt(255);
	CheckMapNo(1000, -1);
	/* 同じ番号を別のマップが使う場合と、全削除・再読込 */
	CheckMapNo(1001, 1);
	g_aMapInfo[1] = &aMap[0];
	CheckMapNo(1001, -1);
	CheckMapNo(1000, 1);
	g_aMapInfo.RemoveAll();
	CheckMapNo(1000, -1);
	g_aMapInfo.Add(&aMap[0]);
	CheckMapNo(1000, 0);
	g_aMapInfo.RemoveAll();
	delete [] aMap;
}

/* ========================================================================= */
/* 関数名：	CheckCharNo													 */
/* 内容：	従来の先頭からの検索と結果を比較							 */
/* ========================================================================= */
static void CheckCharNo(DWORD pID)
{
	int nExpected = -1;
	if (pID) {
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (g_aCharInfo[i]->pID == pID) {
				nExpected = i;
				break;
			}
		}
	}
	int nNo = GetCharNo(pID);
	if (nNo != nExpected) {
		fprintf(stderr, "GetCharNo(%u): %d != %d\n", pID, nNo, nExpected);
		abort();
	}
}

/* ========================================================================= */
/* 関数名：	TestCharNo													 */
/* 内容：	キャラ配列・接続ID変更後の検索と、多数キャラでの計測		 */
/* ========================================================================= */
static void TestCharNo(void)
{
	/* 実データ読込前に確認する。ゲームの乱数・通信は使わない */
	const int NCHAR = 4096, NRUN = 200000;
	CHARINFO *aChar = new CHARINFO[NCHAR];
	ZeroMemory(aChar, sizeof(CHARINFO) * NCHAR);
	CheckCharNo(0);
	CheckCharNo(URARASOCK_IDBASE);
	for (int i = 0; i < NCHAR; i ++) {
		/* NPC・オフラインと、末尾のログイン中PC */
		aChar[i].pID = (i % 2) ? i + 1 : 0;
		g_aCharInfo.Add(&aChar[i]);
	}
	for (int i = NCHAR - 32; i < NCHAR; i ++) {
		aChar[i].pID = URARASOCK_IDBASE + i;
		CheckCharNo(aChar[i].pID);
		CheckCharNo(aChar[i].pID);
	}
	/* 複数接続を交互に調べる。合計も固定出力にして呼び出しを確認 */
	int nSum = 0;
	double dStart = NowMs();
	for (int i = 0; i < NRUN; i ++) nSum += GetCharNo(URARASOCK_IDBASE + NCHAR - 32 + i % 32);
	printf("BENCH getcharno chars=%d runs=%d sum=%d ms=%.1f\n", NCHAR, NRUN, nSum, NowMs() - dStart);

	DWORD pID = aChar[NCHAR - 1].pID;
	/* キャッシュ先が配列の範囲外になってから、同じIDを追加し直す */
	g_aCharInfo.RemoveAt(NCHAR - 1);
	CheckCharNo(pID);
	g_aCharInfo.Add(&aChar[NCHAR - 1]);
	CheckCharNo(pID);
	/* 同じ件数での入れ替え・接続ID変更・切断後のID再利用 */
	std::swap(g_aCharInfo[0], g_aCharInfo[NCHAR - 1]);
	CheckCharNo(pID);
	aChar[NCHAR - 1].pID = 0;
	CheckCharNo(pID);
	aChar[100].pID = pID;
	CheckCharNo(pID);
	g_aCharInfo.RemoveAt(0);
	CheckCharNo(pID);
	g_aCharInfo.InsertAt(0, &aChar[NCHAR - 1]);
	CheckCharNo(pID);
	g_aCharInfo.SetAt(100, &aChar[101]);
	CheckCharNo(pID);
	g_aCharInfo.SetAt(100, &aChar[100]);
	CheckCharNo(pID);
	/* 未ログインの検索後に同じ接続がログインしても見つかる */
	CheckCharNo(pID + 1);
	aChar[100].pID = pID + 1;
	CheckCharNo(pID + 1);
	CheckCharNo(pID);
	/* NPCと重複し得るIDは常に先頭を返す（前方に後から重複を作る） */
	aChar[100].pID = 42;
	CheckCharNo(42);
	aChar[10].pID = 42;
	CheckCharNo(42);
	std::swap(g_aCharInfo[10], g_aCharInfo[100]);
	CheckCharNo(42);
	g_aCharInfo.RemoveAt(10);
	CheckCharNo(42);
	g_aCharInfo.RemoveAll();
	CheckCharNo(pID + 1);
	/* 件数増加でキャッシュ済みIDがNPCの範囲に入っても先頭を返す */
	aChar[0].pID = 2;
	g_aCharInfo.Add(&aChar[0]);
	CheckCharNo(2);
	aChar[1].pID = 2;
	g_aCharInfo.InsertAt(0, &aChar[1]);
	CheckCharNo(2);
	g_aCharInfo.RemoveAll();
	CheckCharNo(2);
	g_aCharInfo.Add(&aChar[1]);
	CheckCharNo(2);
	g_aCharInfo.RemoveAll();
	delete [] aChar;
	printf("getcharno mutations=ok\n");
}

/* S: アイテムを使った時のメッセージに入力待ち（@2）を足すか（無いと出てすぐ消える） */
/* ------------------------------------------------------------------------- */
/* AD. 印で見える・消えるNPC												 */
/*    (a) 見え方：印のある人・ない人・管理者・受け取る人が分からない送信		 */
/*    (b) 印が動いた時の見え直し（会話・管理者の印の切り替え・NPCの条件変更）	 */
/*    (c) 見えない人はぶつからず、話せず、ついていけない					 */
/*    (d) 保存して読み直しても条件が残り、古いデータは「無し」になる			 */
/* ------------------------------------------------------------------------- */
static PCHARINFO ViewNpcMake(int nMap, int nX, int nY, const char *pszName, const char *pszSpeech, WORD wShow, WORD wHide)
{
	PCHARINFO pNpc = new CHARINFO;
	ZeroMemory((PBYTE)pNpc, sizeof(CHARINFO));
	pNpc->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pNpc);
	pNpc->byGameState = GAMESTATE_NPC;
	pNpc->byCharState = CHARSTATE_MOVE;
	pNpc->dwMapID = nMap;
	pNpc->dwX = pNpc->dwXBack = pNpc->dwStartX = (DWORD)nX;
	pNpc->dwY = pNpc->dwYBack = pNpc->dwStartY = (DWORD)nY;
	pNpc->wShowFlag = CharFlagToWire(wShow);	/* 中は「番号+1（0=無し）」 */
	pNpc->wHideFlag = CharFlagToWire(wHide);
	strcpy(pNpc->szName, pszName);
	pNpc->pszNpcSpeech = new char[strlen(pszSpeech) + 1];
	strcpy(pNpc->pszNpcSpeech, pszSpeech);
	pNpc = AddChar(pNpc);
	pNpc->pID = pNpc->dwTblID + 1;
	CCharProcBase CharProcBase;
	CharProcBase.SetProc(pNpc);
	return pNpc;
}

static void ViewAdminFlag(PCHARINFO pAdmin, PCHARINFO pTarget, WORD wNo, WORD wValue)
{
	CMDPARA_ADMINCHARFLAG Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_ADMINCHARFLAG;
	Para.byCmdSub = ADMINCHARFLAG_SET;
	Para.byKind = FLAGKIND_BIT;
	Para.dwTblID = pTarget->dwTblID;
	Para.wNo = wNo;
	Para.wValue = wValue;
	TestRecv(pAdmin, &Para, sizeof(Para));
}

/* 共通の準備：A（印なし）・B・C（管理者）を画面内に集め、使える空きマスを 8 つ探す。足りなければ FALSE */
static BOOL ViewSetup(const std::vector<int> &aLogin, std::vector<std::pair<int, int> > &aCell)
{
	if (aLogin.size() < 3) {
		return FALSE;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO apPC[3] = { pA, pB, pC };
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	s_pSpLogin = &aLogin;

	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apPC[i];
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyFlag, sizeof(p->abyFlag));
	}
	pC->byAdminLV = ADMINLV_ADMINISTRATOR;

	aCell.clear();
	for (int r = 1; r < 12 && aCell.size() < 8; r ++) {
		for (int dy = -r; dy <= r && aCell.size() < 8; dy ++) {
			for (int dx = -r; dx <= r && aCell.size() < 8; dx ++) {
				if (abs(dx) != r && abs(dy) != r) continue;
				int x = (int)pA->dwX + dx, y = (int)pA->dwY + dy;
				if (x < 3 || y < 3 || x >= pMap->nCX - 3 || y >= pMap->nCY - 3) continue;
				if (!CheckCharBlock((DWORD)nMap, x, y) || CheckCharBlockNPC((DWORD)nMap, x, y) != (DWORD)-1 ||
					CheckCharBlockPC((DWORD)nMap, x, y) != (DWORD)-1) continue;
				aCell.push_back(std::make_pair(x, y));
			}
		}
	}
	printf("viewflag cells=%d\n", (int)aCell.size());
	if (aCell.size() < 8) {
		return FALSE;
	}
	QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
	QuestMovePC(pB, nMap, aCell[1].first, aCell[1].second);
	QuestMovePC(pC, nMap, aCell[2].first, aCell[2].second);
	return TRUE;
}

/* 話しかけ用の会話データ：901＝話すと印 5 が立つ（お礼）、902＝何も動かさない */
static void ViewTalkAdd(void)
{
	for (int t = 901; t <= 902; t ++) {
		PTALKINFO pTalk = new TALKINFO;
		ZeroMemory(pTalk, sizeof(TALKINFO));
		pTalk->byState = TALKSTATE_ON;
		pTalk->wID = (WORD)t;
		pTalk->byLineCount = 1;
		TalkTestLineInit(&pTalk->aLine[0], 1);
		strcpy(pTalk->aLine[0].szSpeech, (t == 901) ? "助けてくれてありがとう" : "こんにちは");
		if (t == 901) pTalk->aLine[0].awSetOn[0] = 5;
		g_aTalkInfo.Add(pTalk);
	}
}

static void ViewTalkRemove(void)
{
	for (int i = g_aTalkInfo.GetSize() - 1; i >= 0; i --) {
		if (g_aTalkInfo[i]->wID == 901 || g_aTalkInfo[i]->wID == 902) {
			delete g_aTalkInfo[i];
			g_aTalkInfo.RemoveAt(i);
		}
	}
}

/* (a)(b)(c)(d)：乱数を使わない部分。保存の読み直しがあるので、乱数を使う最後の部分より前に流す */
static void TestViewFlag(const std::vector<int> &aLogin)
{
	printf("== AD. ViewFlag ==\n");
	std::vector<std::pair<int, int> > aCell;
	if (!ViewSetup(aLogin, aCell)) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	BYTE byAdminBack = pC->byAdminLV;
	char szAccBack[MAX_ACCNAME];
	int nMap = (int)pA->dwMapID;

	PCHARINFO pHelper = ViewNpcMake(nMap, aCell[3].first, aCell[3].second, "助けを呼ぶ人", "@t901", FLAG_NOBIT, FLAG_NOBIT);
	PCHARINFO pPipi   = ViewNpcMake(nMap, aCell[4].first, aCell[4].second, "ピピ", "@t902", 5, FLAG_NOBIT);
	PCHARINFO pGuard  = ViewNpcMake(nMap, aCell[5].first, aCell[5].second, "見張りの兵", "@t902", FLAG_NOBIT, 5);
	PCHARINFO pBoth   = ViewNpcMake(nMap, aCell[6].first, aCell[6].second, "両方", "@t902", 5, 6);
	ViewTalkAdd();
	printf("viewflag has cond helper=%d pipi=%d guard=%d both=%d player=%d\n", (int)CharHasViewCond(pHelper), (int)CharHasViewCond(pPipi),
		(int)CharHasViewCond(pGuard), (int)CharHasViewCond(pBoth), (int)CharHasViewCond(pA));

	/* (a) 見え方：A は印なし、B は印 5 あり、C は管理者 */
	FlagBitSet(pB->abyFlag, 5, TRUE);
	printf("viewflag visible pipi a=%d b=%d admin=%d null=%d guard a=%d b=%d admin=%d null=%d\n",
		(int)IsCharVisibleTo(pPipi, pA), (int)IsCharVisibleTo(pPipi, pB), (int)IsCharVisibleTo(pPipi, pC), (int)IsCharVisibleTo(pPipi, NULL),
		(int)IsCharVisibleTo(pGuard, pA), (int)IsCharVisibleTo(pGuard, pB), (int)IsCharVisibleTo(pGuard, pC), (int)IsCharVisibleTo(pGuard, NULL));
	FlagBitSet(pB->abyFlag, 6, TRUE);
	printf("viewflag visible both(5 and not 6) b56=%d", (int)IsCharVisibleTo(pBoth, pB));
	FlagBitSet(pB->abyFlag, 6, FALSE);
	printf(" b5=%d a=%d admin=%d helper a=%d null=%d\n", (int)IsCharVisibleTo(pBoth, pB), (int)IsCharVisibleTo(pBoth, pA), (int)IsCharVisibleTo(pBoth, pC),
		(int)IsCharVisibleTo(pHelper, pA), (int)IsCharVisibleTo(pHelper, NULL));
	printf("viewflag visiblepid pipi a=%d b=%d admin=%d all=%d guard a=%d b=%d helper all=%d\n",
		(int)IsCharVisibleToPID(pPipi, pA->pID), (int)IsCharVisibleToPID(pPipi, pB->pID), (int)IsCharVisibleToPID(pPipi, pC->pID),
		(int)IsCharVisibleToPID(pPipi, URARASOCK_SENDALL), (int)IsCharVisibleToPID(pGuard, pA->pID), (int)IsCharVisibleToPID(pGuard, pB->pID),
		(int)IsCharVisibleToPID(pHelper, URARASOCK_SENDALL));

	/* 送られ方：CHARINFO（1体）・CHARINFO3（まわりの人へ）・座標 */
	{
		int a0 = SpCnt(pA, CMD_CHARINFO), b0 = SpCnt(pB, CMD_CHARINFO), c0 = SpCnt(pC, CMD_CHARINFO);
		SendCmdCHARINFO(pA->pID, (int)pPipi->dwTblID);
		SendCmdCHARINFO(pB->pID, (int)pPipi->dwTblID);
		SendCmdCHARINFO(pC->pID, (int)pPipi->dwTblID);
		printf("viewflag send charinfo pipi a=%d b=%d admin=%d", SpCnt(pA, CMD_CHARINFO) - a0, SpCnt(pB, CMD_CHARINFO) - b0, SpCnt(pC, CMD_CHARINFO) - c0);
		a0 = SpCnt(pA, CMD_CHARINFO); b0 = SpCnt(pB, CMD_CHARINFO); c0 = SpCnt(pC, CMD_CHARINFO);
		SendCmdCHARINFO3((int)pPipi->dwTblID);
		printf(" charinfo3 a=%d b=%d admin=%d", SpCnt(pA, CMD_CHARINFO) - a0, SpCnt(pB, CMD_CHARINFO) - b0, SpCnt(pC, CMD_CHARINFO) - c0);
		a0 = SpCnt(pA, CMD_CHARINFO); b0 = SpCnt(pB, CMD_CHARINFO); c0 = SpCnt(pC, CMD_CHARINFO);
		SendCmdCHARINFO3((int)pGuard->dwTblID);
		printf(" guard a=%d b=%d admin=%d", SpCnt(pA, CMD_CHARINFO) - a0, SpCnt(pB, CMD_CHARINFO) - b0, SpCnt(pC, CMD_CHARINFO) - c0);
		a0 = SpCnt(pA, CMD_CHARPOS); b0 = SpCnt(pB, CMD_CHARPOS); c0 = SpCnt(pC, CMD_CHARPOS);
		SendCmdCHARPOS(0, (int)pPipi->dwTblID, 0, 1);
		SendCmdCHARPOS(URARASOCK_SENDALL, (int)pPipi->dwTblID, 0, 0);
		printf(" charpos a=%d b=%d admin=%d\n", SpCnt(pA, CMD_CHARPOS) - a0, SpCnt(pB, CMD_CHARPOS) - b0, SpCnt(pC, CMD_CHARPOS) - c0);
	}
	/* マップに入った時・ログインした時と同じ全員分の送信も、同じ判定を通る（ピピ・両方 は印のある B だけ。見張りの兵 は印のない A だけ） */
	{
		int a0 = MockCalls(1000), b0 = MockCalls(1001);
		SendCmdCHARINFO(pA->pID, -1);
		SendCmdCHARINFO(pB->pID, -1);
		int nA = MockCalls(1000) - a0, nB = MockCalls(1001) - b0;
		printf("viewflag send all a_minus_b=%d\n", nA - nB);
	}

	/* (b) 印が動いた時の見え直し。管理者ウィンドウから A に印 5 を立てる：ピピ・両方が出て、見張りの兵が消える（A だけに届く）。
	   管理者の印の切り替えはプレイヤーのキャラにだけできる */
	strcpy(szAccBack, pA->szAcc);
	strcpy(pA->szAcc, "viewtest");
	{
		int ai = SpCnt(pA, CMD_CHARINFO), ap = SpCnt(pA, CMD_CHARPOS), bi = SpCnt(pB, CMD_CHARINFO), bp = SpCnt(pB, CMD_CHARPOS);
		int ci = SpCnt(pC, CMD_CHARINFO), cp = SpCnt(pC, CMD_CHARPOS);
		ViewAdminFlag(pC, pA, 5, 1);
		printf("viewflag admin set a_flag=%d a_charinfo=%d a_hide=%d b_charinfo=%d b_hide=%d admin_charinfo=%d admin_hide=%d\n",
			(int)FlagBitGet(pA->abyFlag, 5), SpCnt(pA, CMD_CHARINFO) - ai, SpCnt(pA, CMD_CHARPOS) - ap, SpCnt(pB, CMD_CHARINFO) - bi,
			SpCnt(pB, CMD_CHARPOS) - bp, SpCnt(pC, CMD_CHARINFO) - ci, SpCnt(pC, CMD_CHARPOS) - cp);
		/* 印を消すと元に戻る：ピピ・両方が消え、見張りの兵が出る */
		ai = SpCnt(pA, CMD_CHARINFO); ap = SpCnt(pA, CMD_CHARPOS);
		ViewAdminFlag(pC, pA, 5, 0);
		printf("viewflag admin clear a_flag=%d a_charinfo=%d a_hide=%d\n", (int)FlagBitGet(pA->abyFlag, 5), SpCnt(pA, CMD_CHARINFO) - ai, SpCnt(pA, CMD_CHARPOS) - ap);
	}
	strcpy(pA->szAcc, szAccBack);
	/* 条件を変えた時は、そのマップの全員に見え直し：ピピの条件を外すと、見える人も見えない人も CHARINFO が届く。
	   条件を戻すと、見えない人（A）にだけ消す通知が届く */
	{
		pPipi->wShowFlag = 0;
		int ai = SpCnt(pA, CMD_CHARINFO), bi = SpCnt(pB, CMD_CHARINFO), ci = SpCnt(pC, CMD_CHARINFO);
		CharViewRefreshNpc(pPipi);
		printf("viewflag npc cond off a=%d b=%d admin=%d", SpCnt(pA, CMD_CHARINFO) - ai, SpCnt(pB, CMD_CHARINFO) - bi, SpCnt(pC, CMD_CHARINFO) - ci);
		pPipi->wShowFlag = CharFlagToWire(5);
		int ap = SpCnt(pA, CMD_CHARPOS), bp = SpCnt(pB, CMD_CHARPOS), cp = SpCnt(pC, CMD_CHARPOS);
		CharViewRefreshNpc(pPipi);
		printf(" on hide a=%d b=%d admin=%d\n", SpCnt(pA, CMD_CHARPOS) - ap, SpCnt(pB, CMD_CHARPOS) - bp, SpCnt(pC, CMD_CHARPOS) - cp);
	}

	/* (c) 見えない人は、ぶつからず・話せず・相手にできない（A は印なし、B は印 5 あり、C は管理者） */
	{
		DWORD dwX = pPipi->dwX, dwY = pPipi->dwY;
		printf("viewflag block pipi a=%d b=%d admin=%d none=%d guard a=%d b=%d\n",
			(int)(CheckCharBlockNPC((DWORD)nMap, dwX, dwY, 0, pA) == pPipi->dwTblID), (int)(CheckCharBlockNPC((DWORD)nMap, dwX, dwY, 0, pB) == pPipi->dwTblID),
			(int)(CheckCharBlockNPC((DWORD)nMap, dwX, dwY, 0, pC) == pPipi->dwTblID), (int)(CheckCharBlockNPC((DWORD)nMap, dwX, dwY, 0) == pPipi->dwTblID),
			(int)(CheckCharBlockNPC((DWORD)nMap, pGuard->dwX, pGuard->dwY, 0, pA) == pGuard->dwTblID),
			(int)(CheckCharBlockNPC((DWORD)nMap, pGuard->dwX, pGuard->dwY, 0, pB) == pGuard->dwTblID));
		printf("viewflag block2 pipi a=%d b=%d interact a=%d b=%d admin=%d guard a=%d b=%d admin=%d\n",
			(int)(CheckCharBlockNPC2((DWORD)-1, (DWORD)nMap, dwX, dwY, 0, pA) == pPipi->dwTblID), (int)(CheckCharBlockNPC2((DWORD)-1, (DWORD)nMap, dwX, dwY, 0, pB) == pPipi->dwTblID),
			(int)IsInteractable(pA, pPipi), (int)IsInteractable(pB, pPipi), (int)IsInteractable(pC, pPipi),
			(int)IsInteractable(pA, pGuard), (int)IsInteractable(pB, pGuard), (int)IsInteractable(pC, pGuard));
		/* 見えない人が話しかけても、何も返らない（見える人の話は最後の部分で確かめる。話すと乱数を使うため） */
		QuestMovePC(pA, nMap, pPipi->dwX, pPipi->dwY + 1);
		int at = SpCnt(pA, CMD_TALKMSG);
		QuestSendTalk(pA, pPipi->dwTblID);
		printf("viewflag talk pipi a=%d", SpCnt(pA, CMD_TALKMSG) - at);
		QuestSendTalk(pA, pBoth->dwTblID);
		printf(" both a=%d", SpCnt(pA, CMD_TALKMSG) - at);
		QuestMovePC(pB, nMap, pGuard->dwX + 1, pGuard->dwY);
		int bt = SpCnt(pB, CMD_TALKMSG);
		QuestSendTalk(pB, pGuard->dwTblID);
		printf(" guard b=%d\n", SpCnt(pB, CMD_TALKMSG) - bt);
	}

	/* (c2) 管理者も、ふだんは印どおり（C は印 5 なし：ピピは見えず・ぶつからず・話せない。見張りの兵は見える）。
	   「印で隠れたNPCも表示」（CMD_ADMINVIEWALL）をオンにすると全部見える。切り替えた時は、その管理者にだけ出し直しが届く。
	   管理者でない人が送っても変わらない */
	{
		BYTE byAdminKeep = pC->byAdminLV;
		char szAccKeep[MAX_ACCNAME];
		strcpy(szAccKeep, pC->szAcc);
		strcpy(pC->szAcc, "viewtest");
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_ADMINVIEWALL;
		Para.byData = 1;
		printf("viewflag viewall default flag=%d pipi=%d guard=%d both=%d\n", (int)pC->byViewAllNpc, (int)IsCharVisibleTo(pPipi, pC),
			(int)IsCharVisibleTo(pGuard, pC), (int)IsCharVisibleTo(pBoth, pC));
		int ai = SpCnt(pA, CMD_CHARINFO), ap = SpCnt(pA, CMD_CHARPOS), bi = SpCnt(pB, CMD_CHARINFO), bp = SpCnt(pB, CMD_CHARPOS);
		int ci = SpCnt(pC, CMD_CHARINFO), cp = SpCnt(pC, CMD_CHARPOS);
		TestRecv(pC, &Para, sizeof(Para));
		printf("viewflag viewall on flag=%d pipi=%d guard=%d both=%d c_charinfo=%d c_hide=%d a=%d,%d b=%d,%d\n", (int)pC->byViewAllNpc,
			(int)IsCharVisibleTo(pPipi, pC), (int)IsCharVisibleTo(pGuard, pC), (int)IsCharVisibleTo(pBoth, pC), SpCnt(pC, CMD_CHARINFO) - ci,
			SpCnt(pC, CMD_CHARPOS) - cp, SpCnt(pA, CMD_CHARINFO) - ai + SpCnt(pA, CMD_CHARPOS) - ap, 0, SpCnt(pB, CMD_CHARINFO) - bi + SpCnt(pB, CMD_CHARPOS) - bp, 0);
		printf("viewflag viewall on block pipi=%d interact=%d talk_ok=%d\n",
			(int)(CheckCharBlockNPC((DWORD)nMap, pPipi->dwX, pPipi->dwY, 0, pC) == pPipi->dwTblID), (int)IsInteractable(pC, pPipi), (int)IsCharSeenBy(pPipi, pC));
		/* 同じ値をもう一度送っても、出し直しは起きない */
		ci = SpCnt(pC, CMD_CHARINFO); cp = SpCnt(pC, CMD_CHARPOS);
		TestRecv(pC, &Para, sizeof(Para));
		printf("viewflag viewall again c=%d", SpCnt(pC, CMD_CHARINFO) - ci + SpCnt(pC, CMD_CHARPOS) - cp);
		/* オフに戻すと、元の印どおり：ピピは消え（隠す通知が届く）、見張りの兵はそのまま */
		Para.byData = 0;
		ci = SpCnt(pC, CMD_CHARINFO); cp = SpCnt(pC, CMD_CHARPOS);
		TestRecv(pC, &Para, sizeof(Para));
		printf(" off flag=%d pipi=%d guard=%d c_charinfo=%d c_hide=%d\n", (int)pC->byViewAllNpc, (int)IsCharVisibleTo(pPipi, pC),
			(int)IsCharVisibleTo(pGuard, pC), SpCnt(pC, CMD_CHARINFO) - ci, SpCnt(pC, CMD_CHARPOS) - cp);
		/* 管理者でない人が送っても変わらない */
		pC->byAdminLV = ADMINLV_USER;
		Para.byData = 1;
		TestRecv(pC, &Para, sizeof(Para));
		printf("viewflag viewall user flag=%d\n", (int)pC->byViewAllNpc);
		pC->byAdminLV = byAdminKeep;
		strcpy(pC->szAcc, szAccKeep);
	}

	/* (d) 保存：条件は「番号+1・0=無し」で書く（古い読み込みや 0 のデータが「印 0」にならないように）。
	   保存したファイルを開いて、NPC番号で探した行の値を見る。読み込みは別の所（M. Quest）で確かめる */
	{
		BOOL bSaved = SaveCharInfo();
		PCHARINFO apNpc[4] = { pHelper, pPipi, pGuard, pBoth };
		const char *apszName[4] = { "helper", "pipi", "guard", "both" };
		printf("viewflag save=%d", (int)bSaved);
		FILE *fp = fopen("SBODATA/SboDataCharInfo.dat", "rb");
		std::vector<BYTE> aFile;
		if (fp) {
			BYTE abyBuf[4096];
			size_t n;
			while ((n = fread(abyBuf, 1, sizeof(abyBuf), fp)) > 0) aFile.insert(aFile.end(), abyBuf, abyBuf + n);
			fclose(fp);
		}
		DWORD dwColumn = 0, dwCount = 0, dwOffNpcID = 0, dwOffShow = 0, dwOffHide = 0;
		if (aFile.size() > 8) {
			memcpy(&dwColumn, &aFile[0], 4);
			memcpy(&dwCount, &aFile[4], 4);
			size_t nPos = 8;
			for (DWORD c = 0; c < dwColumn && nPos < aFile.size(); c ++) {
				const char *pszName = (const char *)&aFile[nPos];
				nPos += strlen(pszName) + 1;
				DWORD dwOff;
				memcpy(&dwOff, &aFile[nPos], 4);
				nPos += 4;
				if (strcmp(pszName, "dwNpcID") == 0) dwOffNpcID = dwOff;
				if (strcmp(pszName, "wShowFlag") == 0) dwOffShow = dwOff;
				if (strcmp(pszName, "wHideFlag") == 0) dwOffHide = dwOff;
			}
		}
		printf(" columns=%d", (int)(dwOffNpcID && dwOffShow && dwOffHide));
		for (int k = 0; k < 4 && dwOffNpcID && dwOffShow && dwOffHide; k ++) {
			int nShow = -1, nHide = -1;
			for (DWORD r = 0; r < dwCount; r ++) {
				DWORD dwID;
				memcpy(&dwID, &aFile[dwOffNpcID + r * 4], 4);
				if (dwID == apNpc[k]->dwNpcID) {
					WORD w1, w2;
					memcpy(&w1, &aFile[dwOffShow + r * 2], 2);
					memcpy(&w2, &aFile[dwOffHide + r * 2], 2);
					nShow = w1; nHide = w2;
					break;
				}
			}
			printf(" %s=%d,%d", apszName[k], nShow, nHide);
		}
		printf("\n");
		/* 書く・読む：0=無し、番号は +1。範囲の外や 0（古いデータ）は必ず「無し」 */
		printf("viewflag wire to=%d,%d,%d,%d from=%d,%d,%d,%d,%d\n", CharFlagToWire(FLAG_NOBIT), CharFlagToWire(0), CharFlagToWire(5), CharFlagToWire(1023),
			CharFlagFromWire(0), CharFlagFromWire(1), CharFlagFromWire(6), CharFlagFromWire(1024), CharFlagFromWire(1025));
		/* 作ったばかりのキャラ（読み込みで足されるキャラ・古いデータで列が無いキャラ）は「無し」から始まる */
		CHARINFO Fresh;
		ZeroMemory(&Fresh, sizeof(Fresh));
		Fresh.byNpcType = NPCTYPE_NOMOVE;
		SetInitCharData(&Fresh);
		printf("viewflag fresh show=%d hide=%d cond=%d\n", (int)Fresh.wShowFlag, (int)Fresh.wHideFlag, (int)CharHasViewCond(&Fresh));
		/* ZeroMemory しただけのNPC（SetInitCharData を通らない作り方）も、条件なしで誰にでも見える */
		CHARINFO ZeroNpc, ZeroPc;
		ZeroMemory(&ZeroNpc, sizeof(ZeroNpc));
		ZeroMemory(&ZeroPc, sizeof(ZeroPc));
		ZeroNpc.byNpcType = NPCTYPE_NOMOVE;
		printf("viewflag zeroed npc cond=%d seen=%d\n", (int)CharHasViewCond(&ZeroNpc), (int)IsCharSeenBy(&ZeroNpc, &ZeroPc));
		/* 後片付け：作った NPC を消す */
		for (int k = 0; k < 4; k ++) {
			SAFE_DELETE_ARRAY(apNpc[k]->pszNpcSpeech);
			DeleteChar(apNpc[k]->dwTblID);
		}
	}
	ViewTalkRemove();
	for (int i = 0; i < 3; i ++) ZeroMemory(g_aCharInfo[aLogin[i]]->abyFlag, sizeof(g_aCharInfo[aLogin[i]]->abyFlag));
	pC->byAdminLV = byAdminBack;
}

/* (e) 話して印が立つ時の見え直し・見える人の会話（会話で乱数を使うので、いちばん最後に流す） */
static void TestViewFlagTalk(const std::vector<int> &aLogin)
{
	printf("== AE. ViewFlagTalk ==\n");
	std::vector<std::pair<int, int> > aCell;
	if (!ViewSetup(aLogin, aCell)) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	int nMap = (int)pA->dwMapID;

	PCHARINFO pHelper = ViewNpcMake(nMap, aCell[3].first, aCell[3].second, "助けを呼ぶ人2", "@t901", FLAG_NOBIT, FLAG_NOBIT);
	PCHARINFO pPipi   = ViewNpcMake(nMap, aCell[4].first, aCell[4].second, "ピピ2", "@t902", 5, FLAG_NOBIT);
	PCHARINFO pGuard  = ViewNpcMake(nMap, aCell[5].first, aCell[5].second, "見張りの兵2", "@t902", FLAG_NOBIT, 5);
	ViewTalkAdd();
	FlagBitSet(pB->abyFlag, 5, TRUE);

	/* A が助けを呼ぶ人に話す：印が立ち、ピピが出て、見張りの兵が消える（A にだけ届く）。2 回目は印が変わらないので送らない */
	QuestMovePC(pA, nMap, pHelper->dwX, pHelper->dwY + 1);
	{
		int ai = SpCnt(pA, CMD_CHARINFO), ap = SpCnt(pA, CMD_CHARPOS), at = SpCnt(pA, CMD_TALKMSG);
		int bi = SpCnt(pB, CMD_CHARINFO), bp = SpCnt(pB, CMD_CHARPOS), ci = SpCnt(pC, CMD_CHARINFO), cp = SpCnt(pC, CMD_CHARPOS);
		QuestSendTalk(pA, pHelper->dwTblID);
		printf("viewflag talk first flag=%d talkmsg=%d a_charinfo=%d a_hide=%d b_charinfo=%d b_hide=%d admin_charinfo=%d admin_hide=%d",
			(int)FlagBitGet(pA->abyFlag, 5), SpCnt(pA, CMD_TALKMSG) - at, SpCnt(pA, CMD_CHARINFO) - ai, SpCnt(pA, CMD_CHARPOS) - ap,
			SpCnt(pB, CMD_CHARINFO) - bi, SpCnt(pB, CMD_CHARPOS) - bp, SpCnt(pC, CMD_CHARINFO) - ci, SpCnt(pC, CMD_CHARPOS) - cp);
		ai = SpCnt(pA, CMD_CHARINFO); ap = SpCnt(pA, CMD_CHARPOS);
		QuestSendTalk(pA, pHelper->dwTblID);
		printf(" second a_charinfo=%d a_hide=%d\n", SpCnt(pA, CMD_CHARINFO) - ai, SpCnt(pA, CMD_CHARPOS) - ap);
		printf("viewflag after talk pipi=%d guard=%d\n", (int)IsCharVisibleTo(pPipi, pA), (int)IsCharVisibleTo(pGuard, pA));
	}
	/* 見える人は話せる（B は印 5 あり。ピピと話せて、見張りの兵は居ないのと同じ） */
	{
		QuestMovePC(pB, nMap, pPipi->dwX + 1, pPipi->dwY);
		int bt = SpCnt(pB, CMD_TALKMSG);
		QuestSendTalk(pB, pPipi->dwTblID);
		printf("viewflag talk visible pipi b=%d", SpCnt(pB, CMD_TALKMSG) - bt);
		QuestMovePC(pB, nMap, pGuard->dwX + 1, pGuard->dwY);
		bt = SpCnt(pB, CMD_TALKMSG);
		QuestSendTalk(pB, pGuard->dwTblID);
		printf(" guard b=%d", SpCnt(pB, CMD_TALKMSG) - bt);
		QuestMovePC(pC, nMap, pGuard->dwX + 1, pGuard->dwY + 1);
		int ct = SpCnt(pC, CMD_TALKMSG);
		QuestSendTalk(pC, pGuard->dwTblID);
		printf(" guard admin=%d\n", SpCnt(pC, CMD_TALKMSG) - ct);
	}
	ViewTalkRemove();
	for (int i = 0; i < 3; i ++) ZeroMemory(g_aCharInfo[aLogin[i]]->abyFlag, sizeof(g_aCharInfo[aLogin[i]]->abyFlag));
	pC->byAdminLV = ADMINLV_USER;
}

static void TestItemMsgWait(void)
{
	static const char *aMsg[] = {
		"ギルドで探していた指輪。",		/* 無い → 足す */
		"何も起こりませんでした。@2",	/* ある → そのまま */
		"一行目@0二行目",				/* 待ちの後に文字 → 足す */
		"読んだ。@0",					/* 点滅の待ちで終わる → そのまま */
		"音@2@310",						/* 待ちの後は効果音だけ → そのまま */
		"改行だけ@1",					/* 改行は待ちではない → 足す */
		"@412345",						/* アイテム取得の印だけ → 足す */
	};
	for (size_t n = 0; n < sizeof(aMsg) / sizeof(aMsg[0]); n ++) {
		char szMsg[64];
		strcpy(szMsg, aMsg[n]);
		ItemMsgAddWait(szMsg, sizeof(szMsg));
		printf("S itemmsg %d %s\n", (int)n, (strcmp(szMsg, aMsg[n]) == 0) ? "same" : szMsg + strlen(aMsg[n]));
	}
	/* 入りきらない時は足さない */
	char szFull[4] = "abc";
	ItemMsgAddWait(szFull, sizeof(szFull));
	printf("S itemmsg full %s\n", szFull);

	/* {f番号}・{v番号} を印・数値で置き換える */
	BYTE abyF[MAX_FLAGBITBYTE], abyN[MAX_FLAGNUM];
	ZeroMemory(abyF, sizeof(abyF));
	ZeroMemory(abyN, sizeof(abyN));
	FlagBitSet(abyF, 3, TRUE);
	FlagBitSet(abyF, 1023, TRUE);
	abyN[2] = 45;
	abyN[127] = 255;
	static const char *aExp[] = {
		"印{f3}{f4}{f1023} 数{v2}/{v127}/{v0}",	/* 通常 */
		"{f1024}{v128}{f}{f3{v2}{x3}{f99999}",	/* 範囲外・崩れ → そのまま（{v2} は置き換わる） */
		"あ{f3}い",								/* 日本語の間 */
	};
	for (size_t n = 0; n < sizeof(aExp) / sizeof(aExp[0]); n ++) {
		char szOut[128];
		ItemMsgExpand(szOut, sizeof(szOut), aExp[n], abyF, abyN);
		printf("S itemmsg expand %d %s\n", (int)n, szOut);
	}
	/* 入りきらない時は文字の途中で切らない */
	char szSmall[8];
	ItemMsgExpand(szSmall, sizeof(szSmall), "あい{f3}う", abyF, abyN);
	printf("S itemmsg expand small %s len=%d\n", szSmall, (int)strlen(szSmall));
}


/* ------------------------------------------------------------------------- */
/* AG. 印で変わるマス（MAPSWAP）・宝箱											 */
/*    (a) データAPIの書き込み（宝箱番号の自動・重なりの拒否・種類違いの項目の拒否）	 */
/*    (b) 見た目と通れるか：その人の印・開けた記録だけ変わる（NPC・管理者の全表示はベース）*/
/*    (c) 差し替え一覧の送信（本人にだけ・マップ情報には入れない）				 */
/*    (d) 調べると宝箱が開く：品・記録・文・もう一度は何も起きない・持ち物いっぱいは記録を立てない */
/* ------------------------------------------------------------------------- */
static std::string SwapApi(int nMap, const char *pszJson)
{
	const char *psz = SboWebDataApply("map", nMap, pszJson, 1, "テスト");
	std::string str = psz ? psz : "(null)";
	size_t nPos = str.find("\"record\"");
	if (nPos != std::string::npos) str = str.substr(0, nPos) + "...}";
	return str;
}

/* 全マップを通して使った宝箱の番号の最大+1（自動で振られる次の番号の見込み） */
static int ChestHighAllForTest(void)
{
	int nHigh = 0;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		PMAPINFO p = g_aMapInfo[i];
		nHigh = max(nHigh, (int)p->wChestHigh);
		for (int j = 0; j < p->bySwapCount; j ++) {
			if (p->aSwap[j].byKind == MAPSWAPKIND_CHEST) nHigh = max(nHigh, (int)p->aSwap[j].wChestNo + 1);
		}
	}
	return nHigh;
}

/* マップの印で変わるマスだけ元に戻す（API の書き込みでマップ名などのポインタは作り直されるので、構造体ごとは戻さない） */
static void SwapRestore(PMAPINFO p, const MAPINFO &Bak)
{
	p->bySwapCount = Bak.bySwapCount;
	memcpy(p->aSwap, Bak.aSwap, sizeof(p->aSwap));
	p->wChestHigh = Bak.wChestHigh;
	p->wBoxHigh = Bak.wBoxHigh;
}

static void TestMapSwap(const std::vector<int> &aLogin)
{
	printf("== AG. MapSwap ==\n");
	std::vector<std::pair<int, int> > aCell;
	if (!ViewSetup(aLogin, aCell)) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	WORD wWalk = 0xFFFF, wBlock = 0xFFFF;
	for (int i = 0; i < (int)g_wCountMapParts; i ++) {
		if ((wWalk == 0xFFFF) && (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) && (g_aMapPartsInfo[i].byDamage == 0)) wWalk = (WORD)i;
		if ((wBlock == 0xFFFF) && (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1)) wBlock = (WORD)i;
	}
	if ((wWalk == 0xFFFF) || (wBlock == 0xFFFF) || (g_aItemInfo.GetSize() < 3)) {
		printf("mapswap no parts/items\n");
		return;
	}
	WORD wItem = 1, wFill = 2;
	/* 後で戻す */
	MAPINFO Bak = *pMap;
	int ax = aCell[3].first, ay = aCell[3].second;		/* 宝箱 */
	int fx = aCell[4].first, fy = aCell[4].second;		/* 印 7 が立つと壁になる */
	int gx = aCell[5].first, gy = aCell[5].second;		/* 印 8 が立っていない人には壁 */
	WORD wBaseA = pMap->pwMap[ax + ay * pMap->nCX];
	WORD wBaseF = pMap->pwMap[fx + fy * pMap->nCX];

	/* (a) データAPI */
	char szJson[2048];
	snprintf(szJson, sizeof(szJson),
		"{\"swaps\":["
		"{\"kind\":1,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wGiveItem\":%d,\"szMsg\":\"宝箱を開けた！\",\"szMemo\":\"テスト倉庫の奥\"},"
		"{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":7,\"byCmp\":0,\"wParts\":%d},"
		"{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":8,\"byCmp\":1,\"wParts\":%d}]}",
		ax, ay, (int)wWalk, (int)wItem, fx, fy, (int)wBlock, gx, gy, (int)wBlock);
	/* 宝箱の番号は省くと空き番号が振られる（番号は実データの使用状況で決まるので、ここでは重なりの有無だけ見る） */
	std::string strRes = SwapApi(nMap, szJson);
	printf("mapswap api ok=%d count=%d kinds=%d,%d,%d\n", (int)(strRes.find("\"ok\":true") != std::string::npos), (int)pMap->bySwapCount,
		(int)pMap->aSwap[0].byKind, (int)pMap->aSwap[1].byKind, (int)pMap->aSwap[2].byKind);
	WORD wChestNo = pMap->aSwap[0].wChestNo;
	printf("mapswap api chestno valid=%d high_ok=%d memo=%s msg=%s\n", (int)(wChestNo < MAX_CHESTNO), (int)(pMap->wChestHigh == wChestNo + 1),
		pMap->aSwap[0].szMemo, pMap->aSwap[0].szMsg);
	/* 取り出すと同じ形で出る（履歴・元に戻す・しおりは、この JSON を丸ごと書き戻すので、読んで書いて同じになる） */
	std::string strGet = SboWebDataGet("map", nMap);
	printf("mapswap api get has=%d\n", (int)(strGet.find("\"swaps\":[{\"kind\":1,\"x\":") != std::string::npos));
	{
		size_t nS = strGet.find("\"swaps\":"), nE = strGet.find(",\"events\":");
		std::string strSw = (nS != std::string::npos && nE != std::string::npos) ? strGet.substr(nS, nE - nS) : "";
		std::string strBody = "{" + strSw + "}";
		MAPINFO Before = *pMap;
		std::string strRound = SwapApi(nMap, strBody.c_str());
		printf("mapswap api roundtrip ok=%d same=%d\n", (int)(strRound.find("\"ok\":true") != std::string::npos),
			(int)(memcmp(Before.aSwap, pMap->aSwap, sizeof(Before.aSwap)) == 0 && Before.bySwapCount == pMap->bySwapCount));
	}
	/* 拒否：宝箱番号の重なり（この中）・ほかのマップの番号・印の行に宝箱の項目・宝箱の行に印・数の上限・パーツ範囲・知らない項目 */
	char szBad[1024];
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":1,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wChestNo\":5},{\"kind\":1,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wChestNo\":5}]}",
		ax, ay, (int)wWalk, fx, fy, (int)wWalk);
	strRes = SwapApi(nMap, szBad);
	printf("mapswap api dup inside ok=%d\n", (int)(strRes.find("\"ok\":true") != std::string::npos));
	if (g_aMapInfo.GetSize() >= 2) {
		int nOther = (nMap == 0) ? 1 : 0;
		PMAPINFO pOther = g_aMapInfo[nOther];
		MAPINFO OtherBak = *pOther;
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":1,\"x\":0,\"y\":0,\"wParts\":%d,\"wChestNo\":%d}]}", (int)wWalk, (int)wChestNo);
		strRes = SwapApi(nOther, szBad);
		printf("mapswap api dup other map ok=%d\n", (int)(strRes.find("\"ok\":true") != std::string::npos));
		/* 同じマップ内の自分の番号を指定し直すのは良い（書き直し） */
		/* ほかのマップが空いている番号を指定できる／消した番号は自動では使い回さない */
		int nFree = (int)ChestHighAllForTest();
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":1,\"x\":0,\"y\":0,\"wParts\":%d}]}", (int)wWalk);
		strRes = SwapApi(nOther, szBad);
		printf("mapswap api auto other ok=%d no_is_next=%d\n", (int)(strRes.find("\"ok\":true") != std::string::npos), (int)(pOther->aSwap[0].wChestNo == nFree));
		WORD wOtherNo = pOther->aSwap[0].wChestNo;
		/* 消す（0 件にする）→ 足す：消した番号は使い回さない */
		SwapApi(nOther, "{\"swaps\":[]}");
		strRes = SwapApi(nOther, szBad);
		printf("mapswap api no reuse after delete=%d high=%d\n", (int)(pOther->aSwap[0].wChestNo != wOtherNo), (int)(pOther->wChestHigh > wOtherNo));
		SwapRestore(pOther, OtherBak);
	}
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":3,\"wParts\":%d,\"wChestNo\":9}]}", ax, ay, (int)wWalk);
	printf("mapswap api flag+chestkey ok=%d\n", (int)(SwapApi(nMap, szBad).find("\"ok\":true") != std::string::npos));
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":1,\"x\":%d,\"y\":%d,\"wFlag\":3,\"wParts\":%d}]}", ax, ay, (int)wWalk);
	printf("mapswap api chest+flagkey ok=%d\n", (int)(SwapApi(nMap, szBad).find("\"ok\":true") != std::string::npos));
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":1024,\"wParts\":%d}]}", ax, ay, (int)wWalk);
	printf("mapswap api flag range ok=%d\n", (int)(SwapApi(nMap, szBad).find("\"ok\":true") != std::string::npos));
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":1,\"wParts\":%d}]}", ax, ay, (int)g_wCountMapParts);
	printf("mapswap api parts range ok=%d\n", (int)(SwapApi(nMap, szBad).find("\"ok\":true") != std::string::npos));
	snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":1,\"wParts\":%d,\"bogus\":1}]}", ax, ay, (int)wWalk);
	printf("mapswap api unknown key ok=%d\n", (int)(SwapApi(nMap, szBad).find("\"ok\":true") != std::string::npos));
	{
		std::string strMany = "{\"swaps\":[";
		for (int i = 0; i <= MAX_MAPSWAP; i ++) {
			char sz[128];
			snprintf(sz, sizeof(sz), "%s{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":1,\"wParts\":%d}", i ? "," : "", ax, ay, (int)wWalk);
			strMany += sz;
		}
		strMany += "]}";
		printf("mapswap api over limit ok=%d\n", (int)(SwapApi(nMap, strMany.c_str()).find("\"ok\":true") != std::string::npos));
	}
	printf("mapswap api after bad count=%d\n", (int)pMap->bySwapCount);

	/* (b) 見た目と通れるか。A は印なし・記録なし、B も同じ、C は管理者 */
	FlagBitSet(pA->abyFlag, 7, FALSE); FlagBitSet(pA->abyFlag, 8, FALSE);
	FlagBitSet(pB->abyFlag, 7, FALSE); FlagBitSet(pB->abyFlag, 8, FALSE);
	ZeroMemory(pA->abyChestOpened, sizeof(pA->abyChestOpened));
	ZeroMemory(pB->abyChestOpened, sizeof(pB->abyChestOpened));
	pC->byViewAllNpc = 0;
	printf("mapswap parts none: chest a=%d f a=%d g a=%d(wall)\n", (int)(MapSwapParts(pMap, pA, ax, ay) == wBaseA), (int)(MapSwapParts(pMap, pA, fx, fy) == wBaseF),
		(int)(MapSwapParts(pMap, pA, gx, gy) == wBlock));
	FlagBitSet(pA->abyFlag, 7, TRUE);
	FlagBitSet(pA->abyFlag, 8, TRUE);
	printf("mapswap parts a flags: f a=%d b=%d null=%d g a_base=%d b=%d\n", (int)(MapSwapParts(pMap, pA, fx, fy) == wBlock),
		(int)(MapSwapParts(pMap, pB, fx, fy) == wBaseF), (int)(MapSwapParts(pMap, NULL, fx, fy) == wBaseF),
		(int)(MapSwapParts(pMap, pA, gx, gy) != wBlock), (int)(MapSwapParts(pMap, pB, gx, gy) == wBlock));
	printf("mapswap block f a=%d b=%d npc=%d (1=passable)\n", (int)CheckCharBlockFor(pA, (DWORD)nMap, (DWORD)fx, (DWORD)fy),
		(int)CheckCharBlockFor(pB, (DWORD)nMap, (DWORD)fx, (DWORD)fy), (int)CheckCharBlock((DWORD)nMap, (DWORD)fx, (DWORD)fy));
	/* 管理者の印どおり。「印で隠れたNPCも表示」オンのときは、ベースのまま */
	FlagBitSet(pC->abyFlag, 7, TRUE);
	int nAdmin = (MapSwapParts(pMap, pC, fx, fy) == wBlock);
	pC->byViewAllNpc = 1;
	int nAdminAll = (MapSwapParts(pMap, pC, fx, fy) == wBaseF);
	pC->byViewAllNpc = 0;
	FlagBitSet(pC->abyFlag, 7, FALSE);
	printf("mapswap parts admin follows=%d viewall_base=%d\n", nAdmin, nAdminAll);
	/* 宝箱：開けた記録の人にだけ開いた絵（印は使わない） */
	ChestOpenedSet(pB, wChestNo, TRUE);
	printf("mapswap parts chest b_open=%d a_closed=%d\n", (int)(MapSwapParts(pMap, pB, ax, ay) == wWalk), (int)(MapSwapParts(pMap, pA, ax, ay) == wBaseA));
	ChestOpenedSet(pB, wChestNo, FALSE);
	FlagBitSet(pA->abyFlag, 7, FALSE); FlagBitSet(pA->abyFlag, 8, FALSE);

	/* (c) 送信：本人にだけ。MAPINFO には入れない */
	{
		FlagBitSet(pA->abyFlag, 7, TRUE);
		std::vector<BYTE> aCap;
		SvrSendCaptureBegin();
		MapSwapSend(pA, pMap, TRUE);
		BOOL bCap = SvrSendCaptureEnd(aCap);
		int nCount = -1, nApply = 0, nExam = 0;
		if (bCap && aCap.size() >= sizeof(CMDPARA_MAPSWAP) && aCap[0] == CMD_MAPSWAP) {
			PCMDPARA_MAPSWAP pP = (PCMDPARA_MAPSWAP)&aCap[0];
			nCount = pP->wCount;
			PMAPSWAPSEND pS = (PMAPSWAPSEND)(&aCap[0] + sizeof(CMDPARA_MAPSWAP));
			for (int i = 0; i < nCount && (size_t)((BYTE *)&pS[i + 1] - &aCap[0]) <= aCap.size(); i ++) {
				if (pS[i].byFlg & MAPSWAPSEND_APPLY) nApply ++;
				if (pS[i].byFlg & MAPSWAPSEND_EXAMINE) nExam ++;
			}
			printf("mapswap send payload map=%d count=%d apply=%d examine=%d size_ok=%d\n", (int)(pP->dwMapID == pMap->dwID), nCount, nApply, nExam,
				(int)(aCap.size() == sizeof(CMDPARA_MAPSWAP) + nCount * sizeof(MAPSWAPSEND)));
		} else {
			printf("mapswap send payload none\n");
		}
		/* 印 8 が立っている A は g の壁が無くなる（印 7 の f は壁、宝箱は調べられる）。数: f=apply, chest=examine, g(印 8 あり)=無し → apply 1・examine 1 */
		int a0 = SpCnt(pA, CMD_MAPSWAP), b0 = SpCnt(pB, CMD_MAPSWAP), c0 = SpCnt(pC, CMD_MAPSWAP);
		CharViewRefresh(pA);
		printf("mapswap send refresh a=%d b=%d admin=%d\n", SpCnt(pA, CMD_MAPSWAP) - a0, SpCnt(pB, CMD_MAPSWAP) - b0, SpCnt(pC, CMD_MAPSWAP) - c0);
		/* マップに入った時（マップ情報）：本人にだけ MAPSWAP も届き、MAPINFO の中に差し替えは入らない */
		a0 = SpCnt(pA, CMD_MAPSWAP); b0 = SpCnt(pB, CMD_MAPSWAP);
		aCap.clear();
		SvrSendCaptureBegin();
		SendCmdMAPINFO(pA->pID, pMap->dwID);
		bCap = SvrSendCaptureEnd(aCap);
		int nInfoSwap = -1;
		if (bCap && aCap.size() >= sizeof(CMDPARA_MAPINFO) && aCap[0] == CMD_MAPINFO) {
			nInfoSwap = ((PCMDPARA_MAPINFO)&aCap[0])->MapInfo.bySwapCount;
		}
		SendCmdMAPINFO(pA->pID, pMap->dwID);
		printf("mapswap send mapinfo swapcount_in_info=%d a=%d b=%d\n", nInfoSwap, SpCnt(pA, CMD_MAPSWAP) - a0, SpCnt(pB, CMD_MAPSWAP) - b0);
		FlagBitSet(pA->abyFlag, 7, FALSE);
	}
	/* 印が動く所：管理者ウィンドウの印の変更で本人にだけ送り直される */
	{
		char szAccBack[MAX_ACCNAME];
		strcpy(szAccBack, pA->szAcc);
		strcpy(pA->szAcc, "swaptest");
		int a0 = SpCnt(pA, CMD_MAPSWAP), b0 = SpCnt(pB, CMD_MAPSWAP);
		ViewAdminFlag(pC, pA, 7, 1);
		printf("mapswap send adminflag a=%d b=%d flag=%d\n", SpCnt(pA, CMD_MAPSWAP) - a0, SpCnt(pB, CMD_MAPSWAP) - b0, (int)FlagBitGet(pA->abyFlag, 7));
		ViewAdminFlag(pC, pA, 7, 0);
		strcpy(pA->szAcc, szAccBack);
		/* 「印で隠れたNPCも表示」の切り替えでも送り直す（オンなら差し替え 0 件が届く） */
		int c0 = SpCnt(pC, CMD_MAPSWAP);
		pC->byViewAllNpc = 1;
		CharViewRefresh(pC);
		pC->byViewAllNpc = 0;
		printf("mapswap send viewall admin=%d\n", SpCnt(pC, CMD_MAPSWAP) - c0);
	}
	/* マップの表を書き換えた時（API）：そのマップのプレイヤー全員に */
	{
		int a0 = SpCnt(pA, CMD_MAPSWAP), b0 = SpCnt(pB, CMD_MAPSWAP);
		SwapApi(nMap, szJson);	/* 番号を省いて書き直すと宝箱の番号が新しく振られるので、差し替え一覧が変わって届く */
		int a1 = SpCnt(pA, CMD_MAPSWAP) - a0;
		snprintf(szBad, sizeof(szBad), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":7,\"byCmp\":0,\"wParts\":%d}]}", fx, fy, (int)wBlock);
		SwapApi(nMap, szBad);
		printf("mapswap send api rewrite(ids reassigned)=%d changed a=%d b=%d\n", a1, SpCnt(pA, CMD_MAPSWAP) - a0, SpCnt(pB, CMD_MAPSWAP) - b0);
		SwapApi(nMap, szJson);	/* 宝箱の行に戻す */
		wChestNo = pMap->aSwap[0].wChestNo;
	}

	/* (d) 調べると宝箱が開く（A は宝箱の下に立って上向き。Sキー=CMD_CHECKEVENT の byData 2） */
	{
		ZeroMemory(pA->abyChestOpened, sizeof(pA->abyChestOpened));
		ZeroMemory(pA->waItem, sizeof(pA->waItem));
		ZeroMemory(pA->waImportant, sizeof(pA->waImportant));
		FlagBitSet(pA->abyFlag, 7, FALSE); FlagBitSet(pA->abyFlag, 8, FALSE);
		QuestMovePC(pA, nMap, ax, ay + 1);
		pA->byDirection = 0;
		int nItem0 = QuestCountItem(pA, wItem);
		int nSwap0 = SpCnt(pA, CMD_MAPSWAP), nMsg0 = SpCnt(pA, CMD_MAPMSG), nSys0 = SpCnt(pA, CMD_SYSMSG), nB0 = SpCnt(pB, CMD_MAPSWAP);
		int nLenA0 = MockCmdLen(1000);
		/* 条件(1)：まだ開けていない A は、調べられるマスとして届く */
		MimicExamine(pA);
		/* 送る順番：文の窓（MAPMSG）が先、「手に入れました」の白文字（SYSMSG）は窓の後（1=窓が先） */
		printf("mapswap open order_window_first=%d\n", MockCmdBefore(1000, nLenA0, CMD_MAPMSG, CMD_SYSMSG));
		printf("mapswap open item+=%d bit=%d flag7=%d swap=%d msg=%d sys=%d b_swap=%d parts_a_open=%d b_closed=%d\n", QuestCountItem(pA, wItem) - nItem0,
			(int)ChestOpenedGet(pA, wChestNo), (int)FlagBitGet(pA->abyFlag, 7), SpCnt(pA, CMD_MAPSWAP) - nSwap0, SpCnt(pA, CMD_MAPMSG) - nMsg0,
			SpCnt(pA, CMD_SYSMSG) - nSys0, SpCnt(pB, CMD_MAPSWAP) - nB0, (int)(MapSwapParts(pMap, pA, ax, ay) == wWalk), (int)(MapSwapParts(pMap, pB, ax, ay) == wBaseA));
		/* もう一度：何も起きない（品も増えず、文も出ない。ほかにイベントが無ければふつうの「何も見つかりませんでした」） */
		nItem0 = QuestCountItem(pA, wItem); nMsg0 = SpCnt(pA, CMD_MAPMSG); nSwap0 = SpCnt(pA, CMD_MAPSWAP);
		MimicExamine(pA);
		printf("mapswap open again item+=%d swap=%d bit=%d\n", QuestCountItem(pA, wItem) - nItem0, SpCnt(pA, CMD_MAPSWAP) - nSwap0, (int)ChestOpenedGet(pA, wChestNo));
		/* 持ち物がいっぱい（大事な物の欄がいっぱいで、預かり所にも送れない品）：記録を立てず、品も増えない。知らせは会話データと同じ */
		BYTE byFlgBak = g_aItemInfo[wItem]->byFlg;
		g_aItemInfo[wItem]->byFlg |= ITEMFLG_IMPORTANT;
		ChestOpenedSet(pA, wChestNo, FALSE);
		for (int i = 0; i < MAX_IMPORTANT; i ++) pA->waImportant[i] = wFill;
		nItem0 = QuestCountItem(pA, wItem); nSys0 = SpCnt(pA, CMD_SYSMSG); nMsg0 = SpCnt(pA, CMD_MAPMSG); nSwap0 = SpCnt(pA, CMD_MAPSWAP);
		MimicExamine(pA);
		printf("mapswap open full item+=%d bit=%d sys=%d msg=%d swap=%d parts_closed=%d\n", QuestCountItem(pA, wItem) - nItem0, (int)ChestOpenedGet(pA, wChestNo),
			SpCnt(pA, CMD_SYSMSG) - nSys0, SpCnt(pA, CMD_MAPMSG) - nMsg0, SpCnt(pA, CMD_SYSMSG) >= 0 ? SpCnt(pA, CMD_MAPSWAP) - nSwap0 : -1,
			(int)(MapSwapParts(pMap, pA, ax, ay) == wBaseA));
		g_aItemInfo[wItem]->byFlg = byFlgBak;
		ZeroMemory(pA->waImportant, sizeof(pA->waImportant));
		/* 別の向き・別のマスを調べても開かない（目の前だけ） */
		pA->byDirection = 1;
		nItem0 = QuestCountItem(pA, wItem);
		MimicExamine(pA);
		printf("mapswap open wrongdir item+=%d bit=%d\n", QuestCountItem(pA, wItem) - nItem0, (int)ChestOpenedGet(pA, wChestNo));
		pA->byDirection = 0;
		/* 印は使わない：印を全部立てても宝箱は開かず、記録だけが効く */
		memset(pA->abyFlag, 0xFF, sizeof(pA->abyFlag));
		printf("mapswap open flags_dont_open=%d", (int)(MapSwapParts(pMap, pA, ax, ay) == wBaseA));
		ZeroMemory(pA->abyFlag, sizeof(pA->abyFlag));
		/* 傭兵・NPC は調べない（プレイヤー以外は FALSE） */
		printf(" npc_examine=%d\n", (int)MapSwapExamine(NULL));
		ZeroMemory(pA->abyChestOpened, sizeof(pA->abyChestOpened));
		ZeroMemory(pA->waItem, sizeof(pA->waItem));
	}

	/* (e) 開ける条件（wNeedFlag・szLockMsg）：この印が立っている人だけ開けられる。ミニシナリオの鍵の宝箱用 */
	{
		auto ok = [](const std::string &s) { return (int)(s.find("\"ok\":true") != std::string::npos); };
		const char *pszDefault = "いまは開けられないようだ。", *pszCustom = "カルバの話を聞いてからだ。";
		auto Json = [&](const char *pszNeed, const char *pszLock) {
			char sz[1024];
			snprintf(sz, sizeof(sz), "{\"swaps\":[{\"kind\":1,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wGiveItem\":%d,\"szMsg\":\"鍵を手に入れた！\",\"szMemo\":\"鍵の宝箱\"%s%s}]}",
				ax, ay, (int)wWalk, (int)wItem, pszNeed, pszLock);
			return std::string(sz);
		};
		/* 調べて、最初に届いたパケット（開けられない時は MAPMSG だけ）に文が入っているか */
		auto Examine = [&](PCHARINFO p, const char *pszFind, int &nHit) {
			std::vector<BYTE> aCap;
			SvrSendCaptureBegin();
			MimicExamine(p);
			BOOL bCap = SvrSendCaptureEnd(aCap);
			nHit = 0;
			size_t nLen = strlen(pszFind);
			if (bCap && aCap.size() >= nLen) {
				for (size_t i = 0; i + nLen <= aCap.size(); i ++) {
					if (memcmp(&aCap[i], pszFind, nLen) == 0) { nHit = 1; break; }
				}
			}
			return bCap ? (int)aCap[0] : -1;
		};
		auto Clear = [&](PCHARINFO p) {
			ZeroMemory(p->abyChestOpened, sizeof(p->abyChestOpened));
			ZeroMemory(p->waItem, sizeof(p->waItem));
			ZeroMemory(p->waImportant, sizeof(p->waImportant));
			FlagBitSet(p->abyFlag, 241, FALSE); FlagBitSet(p->abyFlag, 0, FALSE);
		};
		Clear(pA); Clear(pB); Clear(pC);
		QuestMovePC(pA, nMap, ax, ay + 1);
		pA->byDirection = 0;

		/* API：設定・読み出し・書き戻し */
		std::string strSet = SwapApi(nMap, Json(",\"wNeedFlag\":241", "").c_str());
		printf("mapswap lock api ok=%d need=%d lockmsg_empty=%d kind=%d\n", ok(strSet), (int)pMap->aSwap[0].wNeedFlag, (int)(pMap->aSwap[0].szLockMsg[0] == 0), (int)pMap->aSwap[0].byKind);
		std::string strG = SboWebDataGet("map", nMap);
		printf("mapswap lock api get need=%d lockmsg=%d\n", (int)(strG.find("\"wNeedFlag\":241,\"szLockMsg\":\"\"}") != std::string::npos),
			(int)(strG.find("\"szLockMsg\"") != std::string::npos));
		{
			size_t nS = strG.find("\"swaps\":"), nE = strG.find(",\"events\":");
			std::string strBody = "{" + ((nS != std::string::npos && nE != std::string::npos) ? strG.substr(nS, nE - nS) : std::string()) + "}";
			MAPINFO Before = *pMap;
			std::string strRound = SwapApi(nMap, strBody.c_str());
			printf("mapswap lock api roundtrip ok=%d same=%d\n", ok(strRound), (int)(memcmp(Before.aSwap, pMap->aSwap, sizeof(Before.aSwap)) == 0));
		}
		/* 印が無い人（既定の文）：開かず、品も記録も無く、開いた絵にもならず、本人にだけ文が届く */
		int nItem0 = QuestCountItem(pA, wItem);
		int nSwap0 = SpCnt(pA, CMD_MAPSWAP), nMsg0 = SpCnt(pA, CMD_MAPMSG), nSys0 = SpCnt(pA, CMD_SYSMSG), nB0 = SpCnt(pB, CMD_MAPMSG);
		int nHitDef = 0;
		MimicExamine(pA);			/* 実際に送らせて数える */
		int nFirst = Examine(pA, pszDefault, nHitDef);		/* もう一度（開けないので同じ）。最初に届くパケットの中身を見る */
		printf("mapswap lock none default first=%d msg=%d item+=%d bit=%d swap=%d mapmsg=%d sys=%d b_msg=%d parts_closed=%d\n", nFirst == CMD_MAPMSG, nHitDef,
			QuestCountItem(pA, wItem) - nItem0, (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo), SpCnt(pA, CMD_MAPSWAP) - nSwap0,
			SpCnt(pA, CMD_MAPMSG) - nMsg0, SpCnt(pA, CMD_SYSMSG) - nSys0, SpCnt(pB, CMD_MAPMSG) - nB0,
			(int)(MapSwapParts(pMap, pA, ax, ay) == pMap->pwMap[ax + ay * pMap->nCX]));
		/* 別の印（241 以外）が立っていても開かない */
		FlagBitSet(pA->abyFlag, 240, TRUE); FlagBitSet(pA->abyFlag, 242, TRUE);
		Examine(pA, pszDefault, nHitDef);
		printf("mapswap lock other_flags msg=%d bit=%d item+=%d\n", nHitDef, (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo), QuestCountItem(pA, wItem) - nItem0);
		FlagBitSet(pA->abyFlag, 240, FALSE); FlagBitSet(pA->abyFlag, 242, FALSE);
		/* szLockMsg を決めると、その文が出る */
		SwapApi(nMap, Json(",\"wNeedFlag\":241", ",\"szLockMsg\":\"カルバの話を聞いてからだ。\"").c_str());
		int nHitCustom = 0;
		Examine(pA, pszCustom, nHitCustom);
		nMsg0 = SpCnt(pA, CMD_MAPMSG);
		Examine(pA, pszDefault, nHitDef);
		printf("mapswap lock none custom msg=%d default_not=%d bit=%d item+=%d lockmsg=%s\n", nHitCustom, 1 - nHitDef,
			(int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo), QuestCountItem(pA, wItem) - nItem0, pMap->aSwap[0].szLockMsg);
		/* 管理者も同じ（印を見る。特別扱いしない） */
		QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
		QuestMovePC(pC, nMap, ax, ay + 1);
		pC->byDirection = 0;
		int nItemC0 = QuestCountItem(pC, wItem);
		int nHitC = 0;
		Examine(pC, pszCustom, nHitC);
		printf("mapswap lock admin none msg=%d bit=%d item+=%d\n", nHitC, (int)ChestOpenedGet(pC, pMap->aSwap[0].wChestNo), QuestCountItem(pC, wItem) - nItemC0);
		FlagBitSet(pC->abyFlag, 241, TRUE);
		int nMsgC0 = SpCnt(pC, CMD_MAPMSG);
		MimicExamine(pC);
		printf("mapswap lock admin flag opens mapmsg=%d bit=%d item+=%d\n", SpCnt(pC, CMD_MAPMSG) - nMsgC0, (int)ChestOpenedGet(pC, pMap->aSwap[0].wChestNo), QuestCountItem(pC, wItem) - nItemC0);
		Clear(pC);
		QuestMovePC(pC, nMap, aCell[1].first, aCell[1].second);
		QuestMovePC(pA, nMap, ax, ay + 1);
		pA->byDirection = 0;
		/* 印が立つと、いつもの宝箱として開く（品・記録・文・開いた絵） */
		FlagBitSet(pA->abyFlag, 241, TRUE);
		nSwap0 = SpCnt(pA, CMD_MAPSWAP); nMsg0 = SpCnt(pA, CMD_MAPMSG);
		MimicExamine(pA);
		printf("mapswap lock flag opens item+=%d bit=%d swap=%d mapmsg=%d parts_open=%d b_closed=%d\n",
			QuestCountItem(pA, wItem) - nItem0, (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo), SpCnt(pA, CMD_MAPSWAP) - nSwap0, SpCnt(pA, CMD_MAPMSG) - nMsg0,
			(int)(MapSwapParts(pMap, pA, ax, ay) == wWalk), (int)(MapSwapParts(pMap, pB, ax, ay) == pMap->pwMap[ax + ay * pMap->nCX]));
		/* 開けた後は、印が消えても何も起きない（開けた記録が先） */
		FlagBitSet(pA->abyFlag, 241, FALSE);
		nMsg0 = SpCnt(pA, CMD_MAPMSG);
		MimicExamine(pA);
		printf("mapswap lock opened again item+=%d mapmsg=%d bit=%d\n", QuestCountItem(pA, wItem) - nItem0, SpCnt(pA, CMD_MAPMSG) - nMsg0, (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo));
		/* 持ち物がいっぱいで渡せなければ、印があっても記録を立てない（今までどおり） */
		Clear(pA);
		FlagBitSet(pA->abyFlag, 241, TRUE);
		BYTE byFlgBak = g_aItemInfo[wItem]->byFlg;
		g_aItemInfo[wItem]->byFlg |= ITEMFLG_IMPORTANT;
		for (int i = 0; i < MAX_IMPORTANT; i ++) pA->waImportant[i] = wFill;
		MimicExamine(pA);
		printf("mapswap lock flag full bit=%d item+=%d\n", (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo), QuestCountItem(pA, wItem));
		g_aItemInfo[wItem]->byFlg = byFlgBak;
		Clear(pA);

		/* 条件なし（65535 を書く・省く）は今までどおり、印が無くても開く */
		for (int nCase = 0; nCase < 2; nCase ++) {
			SwapApi(nMap, Json(nCase == 0 ? ",\"wNeedFlag\":65535" : "", "").c_str());
			nItem0 = QuestCountItem(pA, wItem);
			nMsg0 = SpCnt(pA, CMD_MAPMSG);
			MimicExamine(pA);
			printf("mapswap lock nocond case=%d need=%d mapmsg=%d item+=%d bit=%d\n", nCase, (int)pMap->aSwap[0].wNeedFlag, SpCnt(pA, CMD_MAPMSG) - nMsg0, QuestCountItem(pA, wItem) - nItem0,
				(int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo));
			Clear(pA);
		}
		/* 印 0 を条件にできる（65535 だけが条件なし）。範囲 1023 まで */
		SwapApi(nMap, Json(",\"wNeedFlag\":0", "").c_str());
		int nHit0 = 0;
		Examine(pA, pszDefault, nHit0);
		int nBit0 = (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo);
		FlagBitSet(pA->abyFlag, 0, TRUE);
		MimicExamine(pA);
		printf("mapswap lock flag0 need=%d locked=%d bit_before=%d opens=%d\n", (int)pMap->aSwap[0].wNeedFlag, nHit0, nBit0, (int)ChestOpenedGet(pA, pMap->aSwap[0].wChestNo));
		Clear(pA);
		printf("mapswap lock api 1023 ok=%d need=%d\n", ok(SwapApi(nMap, Json(",\"wNeedFlag\":1023", "").c_str())), (int)pMap->aSwap[0].wNeedFlag);

		/* 断る：範囲の外・文字でない・長すぎる文・kind 0/2 に付ける。表は変わらない */
		SwapApi(nMap, Json(",\"wNeedFlag\":241", ",\"szLockMsg\":\"カルバの話を聞いてからだ。\"").c_str());
		MAPINFO Keep = *pMap;
		std::string strLong(MAX_MAPSWAPMSG, 'a');
		char szOne[512];
		struct { const char *pszName; std::string strJson; } aBad[8];
		int nBad = 0;
		aBad[nBad ++] = { "need1024", Json(",\"wNeedFlag\":1024", "") };
		aBad[nBad ++] = { "need65534", Json(",\"wNeedFlag\":65534", "") };
		aBad[nBad ++] = { "need-1", Json(",\"wNeedFlag\":-1", "") };
		aBad[nBad ++] = { "need_str", Json(",\"wNeedFlag\":\"241\"", "") };
		aBad[nBad ++] = { "lock_long", Json(",\"wNeedFlag\":241", (",\"szLockMsg\":\"" + strLong + "\"").c_str()) };
		snprintf(szOne, sizeof(szOne), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":3,\"wParts\":%d,\"wNeedFlag\":241}]}", ax, ay, (int)wWalk);
		aBad[nBad ++] = { "kind0_need", szOne };
		snprintf(szOne, sizeof(szOne), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"szLockMsg\":\"x\"}]}", ax, ay, (int)wWalk);
		aBad[nBad ++] = { "kind2_lockmsg", szOne };
		snprintf(szOne, sizeof(szOne), "{\"swaps\":[{\"kind\":0,\"x\":%d,\"y\":%d,\"wFlag\":3,\"wParts\":%d,\"szLockMsg\":\"x\"}]}", ax, ay, (int)wWalk);
		aBad[nBad ++] = { "kind0_lockmsg", szOne };
		for (int i = 0; i < nBad; i ++) {
			printf("mapswap lock api reject %s ok=%d same=%d\n", aBad[i].pszName, ok(SwapApi(nMap, aBad[i].strJson.c_str())),
				(int)(memcmp(Keep.aSwap, pMap->aSwap, sizeof(Keep.aSwap)) == 0 && Keep.bySwapCount == pMap->bySwapCount));
		}
		snprintf(szOne, sizeof(szOne), "{\"swaps\":[{\"kind\":2,\"x\":%d,\"y\":%d,\"wParts\":%d,\"wNeedFlag\":241}]}", ax, ay, (int)wWalk);
		printf("mapswap lock api reject kind2_need ok=%d\n", ok(SwapApi(nMap, szOne)));
		/* 長さちょうど（95 バイト）は通る */
		std::string strMax(MAX_MAPSWAPMSG - 1, 'b');
		printf("mapswap lock api max len ok=%d len=%d\n", ok(SwapApi(nMap, Json(",\"wNeedFlag\":241", (",\"szLockMsg\":\"" + strMax + "\"").c_str()).c_str())), (int)strlen(pMap->aSwap[0].szLockMsg));
		Clear(pA); Clear(pB); Clear(pC);
		QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
	}

	/* 片付け：マップを元に戻す */
	QuestMovePC(pA, nMap, aCell[0].first, aCell[0].second);
	SwapRestore(pMap, Bak);
}

/* ------------------------------------------------------------------------- */
/* AG3. 管理者ウィンドウのキャラの開けた宝箱の記録（CMD_ADMINCHEST）				 */
/*   (a) 管理者以外は断る（一覧も変更も、記録は動かず返事も無い）				 */
/*   (b) 一覧：全宝箱が宝箱番号の順に、番号・マップ・座標・品名・メモ付きで届く	 */
/*       （8 件ずつ。続けてキャラの記録 INFO）。消した宝箱（今はどのマップにも無い番号）は出ない */
/*   (c) 立てる・消すと記録が変わり、ログイン中なら差し替えが送り直されて見た目が変わる */
/*   (d) 無い番号・範囲外の値は断る。全部戻すと全部 0 になり、差し替えも送り直す	 */
/* ------------------------------------------------------------------------- */
static BOOL AdminChestSend(PCHARINFO p, BYTE bySub, DWORD dwTblID, WORD wNo, WORD wValue, BOOL bCapture, CMDPARA_ADMINCHEST &Res)
{
	PCMDPARA_ADMINCHEST pPara = new CMDPARA_ADMINCHEST;
	std::vector<BYTE> aCap;
	BOOL bRet = TRUE;

	ZeroMemory(pPara, sizeof(*pPara));
	pPara->byCmd = CMD_ADMINCHEST;
	pPara->byCmdSub = bySub;
	pPara->dwTblID = dwTblID;
	pPara->wNo = wNo;
	pPara->wValue = wValue;
	if (bCapture) SvrSendCaptureBegin();
	TestRecv(p, pPara, sizeof(CMDPARA_ADMINCHEST));
	if (bCapture) bRet = SvrSendCaptureEnd(aCap);
	delete pPara;
	ZeroMemory(&Res, sizeof(Res));
	if (!bCapture) return TRUE;		/* 実際に送らせた時は、届いた数を SpCnt で見る */
	if (!bRet || aCap.size() < sizeof(CMDPARA_ADMINCHEST)) return FALSE;
	memcpy(&Res, &aCap[0], sizeof(Res));
	return TRUE;
}

static void TestAdminChest(const std::vector<int> &aLogin)
{
	printf("== AG3. AdminChest ==\n");
	std::vector<std::pair<int, int> > aCell;
	if (!ViewSetup(aLogin, aCell)) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	WORD wWalk = 0xFFFF;
	for (int i = 0; i < (int)g_wCountMapParts; i ++) {
		if ((wWalk == 0xFFFF) && (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) && (g_aMapPartsInfo[i].byDamage == 0)) wWalk = (WORD)i;
	}
	if ((wWalk == 0xFFFF) || (g_aItemInfo.GetSize() < 2)) {
		printf("adminchest no parts/items\n");
		return;
	}
	/* 後で戻す。ほかのマップの宝箱は一覧に混ざらないよう、いったん無いことにする */
	MAPINFO Bak = *pMap;
	std::vector<BYTE> aSwapCountBak(g_aMapInfo.GetSize());
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		aSwapCountBak[i] = g_aMapInfo[i]->bySwapCount;
		if (i != nMap) g_aMapInfo[i]->bySwapCount = 0;
	}
	char szAccBack[sizeof(pA->szAcc)];
	strcpy(szAccBack, pA->szAcc);
	strcpy(pA->szAcc, "chestadmintest");
	BYTE abyBak[MAX_CHESTBYTE];
	memcpy(abyBak, pA->abyChestOpened, sizeof(abyBak));
	ZeroMemory(pA->abyChestOpened, sizeof(pA->abyChestOpened));

	/* 宝箱 10 件（番号 100〜109。8 件ずつなので 2 回に分かれる）。最初の 1 件だけ品とメモを付け、位置は画面内の空きマス */
	ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
	pMap->bySwapCount = 10;
	for (int k = 0; k < 10; k ++) {
		MAPSWAP *pSw = &pMap->aSwap[k];
		pSw->wX = (WORD)aCell[3 + (k % 5)].first;
		pSw->wY = (WORD)aCell[3 + (k % 5)].second;
		pSw->byKind = MAPSWAPKIND_CHEST;
		pSw->wNeedFlag = FLAG_NOBIT;
		pSw->wParts = wWalk;
		pSw->wChestNo = (WORD)(109 - k);		/* 並びは逆順で持たせ、宝箱番号の順に直って届くか見る */
	}
	pMap->aSwap[9].wGiveItem = 1;				/* 番号 100 */
	strcpy(pMap->aSwap[9].szMemo, "テスト倉庫の奥");
	strcpy(pMap->aSwap[9].szMsg, "宝箱を開けた！");
	pMap->aSwap[8].wChestNo = 101;
	pMap->aSwap[8].wGiveItem = 0;
	WORD wBaseParts = pMap->pwMap[pMap->aSwap[9].wX + pMap->aSwap[9].wY * pMap->nCX];
	int nX0 = pMap->aSwap[9].wX, nY0 = pMap->aSwap[9].wY;
	CMDPARA_ADMINCHEST Res;
	DWORD dwTbl = pA->dwTblID;

	/* (a) 管理者でない B（と A 自身）は、一覧も変更も断られる：記録は動かず、返事も届かない */
	{
		int b0 = SpCnt(pB, CMD_ADMINCHEST), a0 = SpCnt(pA, CMD_ADMINCHEST), sw0 = SpCnt(pA, CMD_MAPSWAP);
		AdminChestSend(pB, ADMINCHEST_SET, dwTbl, 100, 1, FALSE, Res);
		AdminChestSend(pA, ADMINCHEST_SET, dwTbl, 100, 1, FALSE, Res);
		AdminChestSend(pB, ADMINCHEST_GET, dwTbl, 0, 0, FALSE, Res);
		AdminChestSend(pA, ADMINCHEST_CLEAR, dwTbl, 0, 0, FALSE, Res);
		printf("adminchest user bit=%d reply_b=%d reply_a=%d swap=%d\n", (int)ChestOpenedGet(pA, 100), SpCnt(pB, CMD_ADMINCHEST) - b0,
			SpCnt(pA, CMD_ADMINCHEST) - a0, SpCnt(pA, CMD_MAPSWAP) - sw0);
		/* 管理者の一覧の問い合わせは通る（下の (b)）。同じ B が管理者になれば通ることも見る */
		pB->byAdminLV = ADMINLV_ADMINISTRATOR;
		b0 = SpCnt(pB, CMD_ADMINCHEST);
		AdminChestSend(pB, ADMINCHEST_GET, dwTbl, 0, 0, FALSE, Res);
		printf("adminchest admin-b reply=%d\n", SpCnt(pB, CMD_ADMINCHEST) - b0);
		pB->byAdminLV = ADMINLV_USER;
	}

	/* (b) 一覧：10 件が 8 件＋2 件に分かれて届き、続けて記録が届く。内容は宝箱番号の順 */
	{
		int c0 = SpCnt(pC, CMD_ADMINCHEST);
		AdminChestSend(pC, ADMINCHEST_GET, dwTbl, 0, 0, FALSE, Res);
		printf("adminchest list packets=%d\n", SpCnt(pC, CMD_ADMINCHEST) - c0);
		BOOL bGot = AdminChestSend(pC, ADMINCHEST_GET, dwTbl, 0, 0, TRUE, Res);
		const ADMINCHESTROW *pR = &Res.aRow[0];
		printf("adminchest list first got=%d sub=%d total=%d start=%d rows=%d no=%d map_ok=%d x=%d y=%d item=%d name_ok=%d memo=%s\n", (int)bGot, Res.byCmdSub, Res.wTotal, Res.wStart,
			Res.byRows, pR->wChestNo, (int)(pR->wMapNo == nMap), pR->wX == nX0, pR->wY == nY0, pR->wGiveItem, (int)(strcmp(pR->szItem, g_aItemInfo[1]->szName) == 0), pR->szMemo);
		printf("adminchest list order=%d,%d,%d,%d map_name_ok=%d\n", Res.aRow[0].wChestNo, Res.aRow[1].wChestNo, Res.aRow[2].wChestNo, Res.aRow[7].wChestNo,
			(int)(pMap->pszName == NULL || strncmp(pR->szMap, pMap->pszName, sizeof(pR->szMap) - 1) == 0));
		/* 渡す品が無い行の品名は空 */
		printf("adminchest list noitem name_empty=%d\n", (int)(Res.aRow[1].szItem[0] == 0 && Res.aRow[1].szMemo[0] == 0));
	}

	/* (c) 立てる・消す：記録が変わり、差し替えが A に送り直されて、宝箱の見た目が変わる（B は変わらない） */
	{
		int c0 = SpCnt(pC, CMD_ADMINCHEST), sw0 = SpCnt(pA, CMD_MAPSWAP), swB0 = SpCnt(pB, CMD_MAPSWAP);
		AdminChestSend(pC, ADMINCHEST_SET, dwTbl, 100, 1, FALSE, Res);
		printf("adminchest set on bit100=%d bit101=%d bit_b=%d reply=%d swap=%d swap_b=%d parts_a_open=%d parts_b_closed=%d\n", (int)ChestOpenedGet(pA, 100), (int)ChestOpenedGet(pA, 101),
			(int)ChestOpenedGet(pB, 100), SpCnt(pC, CMD_ADMINCHEST) - c0, SpCnt(pA, CMD_MAPSWAP) - sw0, SpCnt(pB, CMD_MAPSWAP) - swB0,
			(int)(MapSwapParts(pMap, pA, nX0, nY0) == wWalk), (int)(MapSwapParts(pMap, pB, nX0, nY0) == wBaseParts));
		sw0 = SpCnt(pA, CMD_MAPSWAP); c0 = SpCnt(pC, CMD_ADMINCHEST);
		AdminChestSend(pC, ADMINCHEST_SET, dwTbl, 100, 0, FALSE, Res);
		printf("adminchest set off bit100=%d swap=%d parts_a_closed=%d reply=%d\n", (int)ChestOpenedGet(pA, 100), SpCnt(pA, CMD_MAPSWAP) - sw0,
			(int)(MapSwapParts(pMap, pA, nX0, nY0) == wBaseParts), SpCnt(pC, CMD_ADMINCHEST) - c0);
	}

	/* (d) 無い番号（どのマップにも無い＝消した宝箱）・範囲外の値・範囲外のキャラは断る。記録も差し替えも動かない */
	{
		int sw0 = SpCnt(pA, CMD_MAPSWAP);
		BOOL bGot1 = AdminChestSend(pC, ADMINCHEST_SET, dwTbl, 2000, 1, TRUE, Res);
		int nR1 = Res.byResult;
		BOOL bGot2 = AdminChestSend(pC, ADMINCHEST_SET, dwTbl, 100, 2, TRUE, Res);
		int nR2 = Res.byResult;
		BOOL bGot3 = AdminChestSend(pC, ADMINCHEST_SET, dwTbl, MAX_CHESTNO, 1, TRUE, Res);
		int nR3 = Res.byResult;
		BOOL bGot4 = AdminChestSend(pC, ADMINCHEST_SET, (DWORD)g_aCharInfo.GetSize() + 5, 100, 1, TRUE, Res);
		int nR4 = Res.byResult;
		BOOL bNo = TRUE;
		for (int i = 0; i < MAX_CHESTBYTE; i ++) if (pA->abyChestOpened[i]) bNo = FALSE;
		printf("adminchest bad got=%d,%d,%d,%d result=%d,%d,%d,%d unchanged=%d swap=%d\n", (int)bGot1, (int)bGot2, (int)bGot3, (int)bGot4, nR1, nR2, nR3, nR4, (int)bNo,
			SpCnt(pA, CMD_MAPSWAP) - sw0);
	}

	/* (e) 全部戻す：全部の記録が 0 になり、差し替えが送り直される */
	{
		ChestOpenedSet(pA, 100, TRUE); ChestOpenedSet(pA, 105, TRUE); ChestOpenedSet(pA, 109, TRUE); ChestOpenedSet(pA, 2000, TRUE);
		int sw0 = SpCnt(pA, CMD_MAPSWAP), c0 = SpCnt(pC, CMD_ADMINCHEST);
		AdminChestSend(pC, ADMINCHEST_CLEAR, dwTbl, 0, 0, FALSE, Res);
		BOOL bZero = TRUE;
		for (int i = 0; i < MAX_CHESTBYTE; i ++) if (pA->abyChestOpened[i]) bZero = FALSE;
		printf("adminchest clear zero=%d swap=%d reply=%d parts_a_closed=%d\n", (int)bZero, SpCnt(pA, CMD_MAPSWAP) - sw0, SpCnt(pC, CMD_ADMINCHEST) - c0,
			(int)(MapSwapParts(pMap, pA, nX0, nY0) == wBaseParts));
	}

	/* 片付け */
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) g_aMapInfo[i]->bySwapCount = aSwapCountBak[i];
	SwapRestore(pMap, Bak);
	memcpy(pA->abyChestOpened, abyBak, sizeof(abyBak));
	strcpy(pA->szAcc, szAccBack);
}

/* ------------------------------------------------------------------------- */
/* AI. 管理者ウィンドウの「印で変わるマス・宝箱」（CMD_ADMINSWAP）				 */
/*   (a) 管理者以外は断る（表は動かず、返事も差し替えの送り直しも無い）			 */
/*   (b) 取得・足す（印・宝箱・リサイクル箱）：表に入り、番号は自動で振られ、	 */
/*       そのマップの人（B も）へ差し替えが送り直される						 */
/*   (c) 直す・消す：番号は変わらず、消した番号は使い回さない				 */
/*   (d) 重なり（表の中・ほかのマップ）・種類に合わない欄・範囲外・古い画面は断り、表も送り直しも動かない */
/*   (e) 元に戻す（画面ごとのアンドゥ）で足す前の表に戻る					 */
/* ------------------------------------------------------------------------- */
extern BOOL AdminUndoExec(BYTE byScreen, DWORD dwStepID, DWORD pID, std::string &strMsg, WORD &wCount);

static void AdminSwapRow(ADMINSWAPROW &R, BYTE byKind, int x, int y, WORD wParts, BYTE byHas)
{
	ZeroMemory(&R, sizeof(R));
	R.byKind = byKind; R.wX = (WORD)x; R.wY = (WORD)y; R.wParts = wParts; R.byHas = byHas;
}

/* 1 つ送る。bCapture なら、返事（最初の 1 つ）を Res に入れる */
static BOOL AdminSwapSend(PCHARINFO p, BYTE bySub, DWORD dwMapID, WORD wIndex, const ADMINSWAPROW *pRow, BOOL bCapture, CMDPARA_ADMINSWAP &Res)
{
	PCMDPARA_ADMINSWAP pPara = new CMDPARA_ADMINSWAP;
	std::vector<BYTE> aCap;
	BOOL bRet = TRUE;

	ZeroMemory(pPara, sizeof(*pPara));
	pPara->byCmd = CMD_ADMINSWAP;
	pPara->byCmdSub = bySub;
	pPara->dwMapID = dwMapID;
	pPara->wIndex = wIndex;
	if (pRow) pPara->Row = *pRow;
	if (bCapture) SvrSendCaptureBegin();
	TestRecv(p, pPara, sizeof(CMDPARA_ADMINSWAP));
	if (bCapture) bRet = SvrSendCaptureEnd(aCap);
	delete pPara;
	ZeroMemory(&Res, sizeof(Res));
	if (!bCapture) return TRUE;
	if (!bRet || aCap.size() < sizeof(CMDPARA_ADMINSWAP)) return FALSE;
	memcpy(&Res, &aCap[0], sizeof(Res));
	return TRUE;
}

static void TestAdminSwap(const std::vector<int> &aLogin)
{
	printf("== AI. AdminSwap ==\n");
	std::vector<std::pair<int, int> > aCell;
	if (!ViewSetup(aLogin, aCell)) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	int nMap = (int)pA->dwMapID;
	PMAPINFO pMap = g_aMapInfo[nMap];
	WORD wWalk = 0xFFFF;
	for (int i = 0; i < (int)g_wCountMapParts; i ++) {
		if ((wWalk == 0xFFFF) && (g_aMapPartsInfo[i].byHitType == PARTSHIT_NONE) && (g_aMapPartsInfo[i].byDamage == 0)) wWalk = (WORD)i;
	}
	int nOther = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) if (i != nMap) { nOther = i; break; }
	if ((wWalk == 0xFFFF) || (g_aItemInfo.GetSize() < 2) || (nOther < 0)) {
		printf("adminswap no parts/items/map\n");
		return;
	}
	MAPINFO Bak = *pMap, BakOther = *g_aMapInfo[nOther];
	DWORD dwMapID = pMap->dwID;
	ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
	pMap->bySwapCount = 0;
	ZeroMemory(g_aMapInfo[nOther]->aSwap, sizeof(g_aMapInfo[nOther]->aSwap));
	g_aMapInfo[nOther]->bySwapCount = 0;
	int nHigh0 = ChestHighAllForTest();
	int nBoxHigh0 = 0;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) nBoxHigh0 = max(nBoxHigh0, (int)g_aMapInfo[i]->wBoxHigh);
	CMDPARA_ADMINSWAP Res;
	ADMINSWAPROW R;
	const WORD wBad = (WORD)g_wCountMapParts;

	/* (a) 管理者でない A・B は断られる */
	{
		AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[3].first, aCell[3].second, wWalk, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_CMP);
		R.wFlag = 7;
		int a0 = SpCnt(pA, CMD_ADMINSWAP), b0 = SpCnt(pB, CMD_ADMINSWAP), sw0 = SpCnt(pA, CMD_MAPSWAP);
		AdminSwapSend(pA, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		AdminSwapSend(pB, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		AdminSwapSend(pB, ADMINSWAP_GET, dwMapID, 0, NULL, FALSE, Res);
		AdminSwapSend(pA, ADMINSWAP_RESTORE, dwMapID, 0, NULL, FALSE, Res);
		printf("adminswap user count=%d reply_a=%d reply_b=%d swap=%d\n", (int)pMap->bySwapCount, SpCnt(pA, CMD_ADMINSWAP) - a0,
			SpCnt(pB, CMD_ADMINSWAP) - b0, SpCnt(pA, CMD_MAPSWAP) - sw0);
	}

	/* (b) 取得と足す */
	{
		int c0 = SpCnt(pC, CMD_ADMINSWAP);
		AdminSwapSend(pC, ADMINSWAP_GET, dwMapID, 0, NULL, FALSE, Res);
		BOOL bGot = AdminSwapSend(pC, ADMINSWAP_GET, dwMapID, 0, NULL, TRUE, Res);
		printf("adminswap get reply=%d got=%d sub=%d map_ok=%d count=%d next_chest_ok=%d next_box_ok=%d\n", SpCnt(pC, CMD_ADMINSWAP) - c0 - 1, (int)bGot, Res.byCmdSub,
			(int)(Res.dwMapID == dwMapID), Res.byCount, (int)(Res.wNextChest == nHigh0), (int)(Res.wNextBox == nBoxHigh0 + 1));

		int sw0 = SpCnt(pA, CMD_MAPSWAP), swB0 = SpCnt(pB, CMD_MAPSWAP), swC0 = SpCnt(pC, CMD_MAPSWAP);
		AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[3].first, aCell[3].second, wWalk, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_CMP);
		R.wFlag = 7; R.byCmp = 1;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap add flag count=%d kind=%d flag=%d cmp=%d parts_ok=%d swap_a=%d swap_b=%d swap_c=%d\n", (int)pMap->bySwapCount, (int)pMap->aSwap[0].byKind,
			(int)pMap->aSwap[0].wFlag, (int)pMap->aSwap[0].byCmp, (int)(pMap->aSwap[0].wParts == wWalk), SpCnt(pA, CMD_MAPSWAP) - sw0,
			SpCnt(pB, CMD_MAPSWAP) - swB0, SpCnt(pC, CMD_MAPSWAP) - swC0);

		/* 宝箱 2 つ（番号は省く）とリサイクル箱 1 つ */
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[4].first, aCell[4].second, wWalk, ADMINSWAPHAS_GIVEITEM | ADMINSWAPHAS_MSG | ADMINSWAPHAS_MEMO);
		R.wGiveItem = 1; strcpy(R.szMsg, "宝箱を開けた"); strcpy(R.szMemo, "テスト 箱1");
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[5].first, aCell[5].second, wWalk, 0);
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		AdminSwapRow(R, MAPSWAPKIND_RECYCLE, aCell[6].first, aCell[6].second, wWalk, ADMINSWAPHAS_MEMO);
		strcpy(R.szMemo, "テスト 回収箱");
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap add auto count=%d chest_no=%d,%d (high0+%d,+%d) item=%d msg=%s memo=%s box_no_ok=%d high=%d boxhigh_ok=%d\n", (int)pMap->bySwapCount,
			(int)pMap->aSwap[1].wChestNo - nHigh0, (int)pMap->aSwap[2].wChestNo - nHigh0, 0, 1, (int)pMap->aSwap[1].wGiveItem, pMap->aSwap[1].szMsg, pMap->aSwap[1].szMemo,
			(int)(pMap->aSwap[3].wBoxNo == nBoxHigh0 + 1), (int)pMap->wChestHigh - nHigh0, (int)(pMap->wBoxHigh == nBoxHigh0 + 1));
	}

	/* (c) 直す・消す */
	{
		int sw0 = SpCnt(pA, CMD_MAPSWAP);
		WORD wNo1 = pMap->aSwap[1].wChestNo;
		/* 行 1 の宝箱を、番号そのままで位置とメモを変える */
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[7].first, aCell[7].second, wWalk, ADMINSWAPHAS_NO | ADMINSWAPHAS_GIVEITEM | ADMINSWAPHAS_MEMO);
		R.wNo = wNo1; R.wGiveItem = 0; strcpy(R.szMemo, "直した");
		AdminSwapSend(pC, ADMINSWAP_SET, dwMapID, 1, &R, FALSE, Res);
		printf("adminswap set no_same=%d x_ok=%d item=%d memo=%s msg_empty=%d swap=%d count=%d\n", (int)(pMap->aSwap[1].wChestNo == wNo1), (int)(pMap->aSwap[1].wX == aCell[7].first),
			(int)pMap->aSwap[1].wGiveItem, pMap->aSwap[1].szMemo, (int)(pMap->aSwap[1].szMsg[0] == 0), SpCnt(pA, CMD_MAPSWAP) - sw0, (int)pMap->bySwapCount);
		/* 番号を省いて種類を印に変える */
		AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[7].first, aCell[7].second, wWalk, ADMINSWAPHAS_FLAG);
		R.wFlag = 9;
		AdminSwapSend(pC, ADMINSWAP_SET, dwMapID, 1, &R, FALSE, Res);
		printf("adminswap set kind=%d flag=%d cmp=%d\n", (int)pMap->aSwap[1].byKind, (int)pMap->aSwap[1].wFlag, (int)pMap->aSwap[1].byCmp);
		/* 消す：行が詰まり、消した宝箱の番号は使い回さない */
		WORD wNo2 = pMap->aSwap[2].wChestNo;
		sw0 = SpCnt(pA, CMD_MAPSWAP);
		AdminSwapSend(pC, ADMINSWAP_DEL, dwMapID, 2, NULL, FALSE, Res);
		printf("adminswap del count=%d row2_kind=%d swap=%d\n", (int)pMap->bySwapCount, (int)pMap->aSwap[2].byKind, SpCnt(pA, CMD_MAPSWAP) - sw0);
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[5].first, aCell[5].second, wWalk, 0);
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap no reuse new=%d (not %d) high=%d\n", (int)pMap->aSwap[3].wChestNo - nHigh0, (int)wNo2 - nHigh0, (int)pMap->wChestHigh - nHigh0);
		/* 消した番号を明示すれば通る */
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[5].first, aCell[5].second, wWalk, ADMINSWAPHAS_NO);
		R.wNo = wNo2;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap explicit retired count=%d no=%d\n", (int)pMap->bySwapCount, (int)pMap->aSwap[4].wChestNo - nHigh0);
	}

	/* (d) 断る：表も送り直しも動かない */
	{
		MAPINFO Keep = *pMap;
		g_aMapInfo[nOther]->bySwapCount = 1;
		g_aMapInfo[nOther]->aSwap[0].byKind = MAPSWAPKIND_CHEST;
		g_aMapInfo[nOther]->aSwap[0].wChestNo = 1500;
		g_aMapInfo[nOther]->aSwap[0].wNeedFlag = FLAG_NOBIT;
		g_aMapInfo[nOther]->aSwap[0].wParts = wWalk;
		int sw0 = SpCnt(pA, CMD_MAPSWAP), nMask = 0, nMsg = 0, nTry = 0;
		struct { BYTE kind; BYTE has; } aT[] = {
			{ MAPSWAPKIND_CHEST, ADMINSWAPHAS_FLAG },							/* 宝箱に印 */
			{ MAPSWAPKIND_CHEST, ADMINSWAPHAS_CMP },							/* 宝箱に比べ方 */
			{ MAPSWAPKIND_FLAG, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_GIVEITEM },	/* 印に品 */
			{ MAPSWAPKIND_FLAG, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_MSG },			/* 印に文 */
			{ MAPSWAPKIND_FLAG, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_NO },			/* 印に番号 */
			{ MAPSWAPKIND_FLAG, 0 },											/* 印の番号なし */
			{ MAPSWAPKIND_RECYCLE, ADMINSWAPHAS_MSG },							/* リサイクル箱に文 */
			{ MAPSWAPKIND_RECYCLE, ADMINSWAPHAS_GIVEITEM },						/* リサイクル箱に品 */
			{ MAPSWAPKIND_RECYCLE, ADMINSWAPHAS_FLAG },							/* リサイクル箱に印 */
			{ 3, 0 },															/* 知らない種類 */
		};
		for (size_t k = 0; k < sizeof(aT) / sizeof(aT[0]); k ++) {
			AdminSwapRow(R, aT[k].kind, aCell[0].first, aCell[0].second, wWalk, aT[k].has);
			R.wFlag = 1; R.wGiveItem = 1; strcpy(R.szMsg, "x");
			BOOL bGot = AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, TRUE, Res);
			nTry ++;
			if (!(Keep.bySwapCount == pMap->bySwapCount && memcmp(Keep.aSwap, pMap->aSwap, sizeof(Keep.aSwap)) == 0)) nMask |= 1 << k;
			if (bGot && Res.byCmdSub == ADMINSWAP_RESULT && Res.byResult == 0 && Res.szMsg[0]) nMsg ++;
		}
		printf("adminswap kind mismatch tried=%d changed_mask=%d result_msgs=%d\n", nTry, nMask, nMsg);

		/* 範囲外・重なり・古い画面・行の外 */
		int nBad = 0, nMsg2 = 0, nTry2 = 0;
		for (int k = 0; k < 8; k ++) {
			BYTE byKind = MAPSWAPKIND_CHEST; BYTE byHas = 0; DWORD dwID = dwMapID; BYTE bySub = ADMINSWAP_ADD; WORD wIdx = 0;
			AdminSwapRow(R, byKind, aCell[0].first, aCell[0].second, wWalk, byHas);
			switch (k) {
			case 0: R.wX = (WORD)pMap->nCX; break;											/* X が外 */
			case 1: R.wY = (WORD)pMap->nCY; break;											/* Y が外 */
			case 2: R.wParts = wBad; break;													/* パーツが外 */
			case 3: R.byHas = ADMINSWAPHAS_NO; R.wNo = pMap->aSwap[1].wChestNo ? pMap->aSwap[3].wChestNo : 0; break;	/* 表の中で重なる宝箱の番号 */
			case 4: R.byHas = ADMINSWAPHAS_NO; R.wNo = 1500; break;							/* ほかのマップが使っている宝箱の番号 */
			case 5: R.byHas = ADMINSWAPHAS_GIVEITEM; R.wGiveItem = (WORD)g_aItemInfo.GetSize(); break;	/* 無い品 */
			case 6: dwID = g_aMapInfo[nOther]->dwID; break;									/* 古い画面（今いるマップと違う） */
			case 7: bySub = ADMINSWAP_DEL; wIdx = 50; break;								/* 行の外 */
			}
			BOOL bGot = AdminSwapSend(pC, bySub, dwID, wIdx, &R, TRUE, Res);
			nTry2 ++;
			if (!(Keep.bySwapCount == pMap->bySwapCount && memcmp(Keep.aSwap, pMap->aSwap, sizeof(Keep.aSwap)) == 0)) nBad |= 1 << k;
			if (bGot && Res.byCmdSub == ADMINSWAP_RESULT && Res.byResult == 0 && Res.szMsg[0]) nMsg2 ++;
		}
		/* リサイクル箱の番号の重なり（表の中・ほかのマップ） */
		g_aMapInfo[nOther]->aSwap[0].byKind = MAPSWAPKIND_RECYCLE;
		g_aMapInfo[nOther]->aSwap[0].wBoxNo = 900;
		AdminSwapRow(R, MAPSWAPKIND_RECYCLE, aCell[0].first, aCell[0].second, wWalk, ADMINSWAPHAS_NO);
		R.wNo = 900;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		if (pMap->bySwapCount != Keep.bySwapCount) nBad |= 1 << 8;
		R.wNo = pMap->aSwap[3].byKind == MAPSWAPKIND_RECYCLE ? pMap->aSwap[3].wBoxNo : 1;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		if (pMap->bySwapCount != Keep.bySwapCount) nBad |= 1 << 9;
		/* 64 個を超える */
		{
			MAPINFO Full = *pMap;
			pMap->bySwapCount = MAX_MAPSWAP;
			for (int i = Keep.bySwapCount; i < MAX_MAPSWAP; i ++) {
				pMap->aSwap[i] = Keep.aSwap[0];
			}
			AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[0].first, aCell[0].second, wWalk, ADMINSWAPHAS_FLAG);
			AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
			if (pMap->bySwapCount != MAX_MAPSWAP) nBad |= 1 << 10;
			pMap->bySwapCount = Full.bySwapCount;
			memcpy(pMap->aSwap, Full.aSwap, sizeof(pMap->aSwap));
		}
		printf("adminswap rejected tried=%d changed_mask=%d result_msgs=%d swap_resent=%d unchanged=%d\n", nTry2, nBad, nMsg2, SpCnt(pA, CMD_MAPSWAP) - sw0,
			(int)(Keep.bySwapCount == pMap->bySwapCount && memcmp(Keep.aSwap, pMap->aSwap, sizeof(Keep.aSwap)) == 0));
		g_aMapInfo[nOther]->bySwapCount = 0;
	}

	/* (e) 元に戻す：画面「印で変わるマス・宝箱」の記録で、足す前（空）の表に戻る */
	{
		std::string strMsg;
		WORD wCount = 0;
		int sw0 = SpCnt(pA, CMD_MAPSWAP);
		BOOL bOk = AdminUndoExec(ADMINSCR_SWAP, 0, 0, strMsg, wCount);
		printf("adminswap undo ok=%d count=%d swap=%d high_kept=%d\n", (int)bOk, (int)pMap->bySwapCount, SpCnt(pA, CMD_MAPSWAP) - sw0, (int)pMap->wChestHigh - nHigh0);
	}

	/* (f) 開ける条件（wNeedFlag・szLockMsg）：管理者ウィンドウの行の受け渡しで消えない・壊れない */
	{
		int sw0 = SpCnt(pA, CMD_MAPSWAP);
		/* 足す（条件を指定した行。一覧にも入って届く） */
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[4].first, aCell[4].second, wWalk, ADMINSWAPHAS_MEMO | ADMINSWAPHAS_NEED | ADMINSWAPHAS_LOCKMSG);
		R.wNeedFlag = 241; strcpy(R.szLockMsg, "錠が固い。"); strcpy(R.szMemo, "鍵");
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap lock add count=%d need=%d lockmsg=%s memo=%s swap=%d\n", (int)pMap->bySwapCount, (int)pMap->aSwap[0].wNeedFlag, pMap->aSwap[0].szLockMsg, pMap->aSwap[0].szMemo,
			SpCnt(pA, CMD_MAPSWAP) - sw0);
		/* ウィンドウから直す（入力欄が無いので条件の指定が無い行）：条件はそのまま残る */
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[4].first, aCell[4].second, wWalk, ADMINSWAPHAS_NO | ADMINSWAPHAS_MEMO);
		R.wNo = pMap->aSwap[0].wChestNo; strcpy(R.szMemo, "鍵・直した");
		AdminSwapSend(pC, ADMINSWAP_SET, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap lock set keeps need=%d lockmsg=%s memo=%s\n", (int)pMap->aSwap[0].wNeedFlag, pMap->aSwap[0].szLockMsg, pMap->aSwap[0].szMemo);
		/* ほかの行を足して消しても残る */
		AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[5].first, aCell[5].second, wWalk, ADMINSWAPHAS_FLAG);
		R.wFlag = 9;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		AdminSwapSend(pC, ADMINSWAP_DEL, dwMapID, 1, NULL, FALSE, Res);
		printf("adminswap lock other rows need=%d lockmsg=%s count=%d\n", (int)pMap->aSwap[0].wNeedFlag, pMap->aSwap[0].szLockMsg, (int)pMap->bySwapCount);
		/* 取得（INFO）の行に条件が入り、そのまま RESTORE で戻せる */
		BOOL bGet = AdminSwapSend(pC, ADMINSWAP_GET, dwMapID, 0, NULL, TRUE, Res);
		printf("adminswap lock info get=%d has=%d need=%d lockmsg=%s\n", (int)bGet, (int)((Res.aRow[0].byHas & (ADMINSWAPHAS_NEED | ADMINSWAPHAS_LOCKMSG)) == (ADMINSWAPHAS_NEED | ADMINSWAPHAS_LOCKMSG)),
			(int)Res.aRow[0].wNeedFlag, Res.aRow[0].szLockMsg);
		CMDPARA_ADMINSWAP Info = Res;
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[4].first, aCell[4].second, wWalk, ADMINSWAPHAS_NEED);
		R.wNeedFlag = 300;											/* ウィンドウ以外（API 相当）で条件を直せる */
		AdminSwapSend(pC, ADMINSWAP_SET, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap lock set need=%d lockmsg=%s\n", (int)pMap->aSwap[0].wNeedFlag, pMap->aSwap[0].szLockMsg);
		{
			PCMDPARA_ADMINSWAP pRes = new CMDPARA_ADMINSWAP(Info);
			pRes->byCmd = CMD_ADMINSWAP; pRes->byCmdSub = ADMINSWAP_RESTORE;
			TestRecv(pC, pRes, sizeof(CMDPARA_ADMINSWAP));
			delete pRes;
		}
		printf("adminswap lock restore need=%d lockmsg=%s count=%d\n", (int)pMap->aSwap[0].wNeedFlag, pMap->aSwap[0].szLockMsg, (int)pMap->bySwapCount);
		/* 範囲の外・種類違い（印の行に条件）は断る */
		MAPINFO Keep2 = *pMap;
		AdminSwapRow(R, MAPSWAPKIND_CHEST, aCell[4].first, aCell[4].second, wWalk, ADMINSWAPHAS_NEED);
		R.wNeedFlag = 1024;
		AdminSwapSend(pC, ADMINSWAP_SET, dwMapID, 0, &R, FALSE, Res);
		int nR1 = (memcmp(Keep2.aSwap, pMap->aSwap, sizeof(Keep2.aSwap)) == 0);
		AdminSwapRow(R, MAPSWAPKIND_FLAG, aCell[5].first, aCell[5].second, wWalk, ADMINSWAPHAS_FLAG | ADMINSWAPHAS_NEED);
		R.wFlag = 9; R.wNeedFlag = 241;
		AdminSwapSend(pC, ADMINSWAP_ADD, dwMapID, 0, &R, FALSE, Res);
		printf("adminswap lock rejected range_same=%d kind0_same=%d\n", nR1, (int)(memcmp(Keep2.aSwap, pMap->aSwap, sizeof(Keep2.aSwap)) == 0 && Keep2.bySwapCount == pMap->bySwapCount));
	}

	/* 片付け */
	{
		MAPINFO *pOt = g_aMapInfo[nOther];
		pOt->bySwapCount = BakOther.bySwapCount;
		memcpy(pOt->aSwap, BakOther.aSwap, sizeof(pOt->aSwap));
		pOt->wChestHigh = BakOther.wChestHigh;
		pOt->wBoxHigh = BakOther.wBoxHigh;
	}
	SwapRestore(pMap, Bak);
}

/* ------------------------------------------------------------------------- */
/* AG2. 保存・読み直し：宝箱の記録（キャラの列）・印で変わるマス（マップの項目）・古いデータ	 */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* AD2. 転職の特技ショートカット・特技の並べ替え・abyJobMagicKey の保存		 */
/* ------------------------------------------------------------------------- */
static void JobKeyPrint(const char *pszLabel, PCHARINFO p)
{
	printf("jobkey %s key=%d,%d,%d,%d,%d item=%d,%d,%d,%d,%d saved=%08x\n", pszLabel,
		p->abyMagicKey[0], p->abyMagicKey[1], p->abyMagicKey[2], p->abyMagicKey[3], p->abyMagicKey[4],
		p->awKeyItem[0], p->awKeyItem[1], p->awKeyItem[2], p->awKeyItem[3], p->awKeyItem[4], (unsigned)p->dwJobMagicKeySaved);
}

static void MagicListPrint(const char *pszLabel, int nRet, const BYTE *pabyMagic)
{
	printf("magicsort %s ret=%d list=", pszLabel, nRet);
	for (int i = 0; i < 8; i ++) printf("%s%d", i ? "," : "", pabyMagic[i]);
	printf("\n");
}

static void TestJobMagicKey(const std::vector<int> &aLogin)
{
	printf("== AD2. JobMagicKey ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[2]];
	BYTE byJobBak = pA->byJOB;
	BYTE abyMagicBak[sizeof(pA->abyMagic)], abyKeyBak[sizeof(pA->abyMagicKey)], abyJobKeyBak0[sizeof(pA->abyJobMagicKey)];
	WORD awKeyItemBak[MAX_MAGICKEY], awJobKeyItemBak[sizeof(pA->awJobKeyItem) / sizeof(WORD)];
	DWORD dwSavedBak0 = pA->dwJobMagicKeySaved;
	memcpy(abyMagicBak, pA->abyMagic, sizeof(abyMagicBak)); memcpy(abyKeyBak, pA->abyMagicKey, sizeof(abyKeyBak));
	memcpy(abyJobKeyBak0, pA->abyJobMagicKey, sizeof(abyJobKeyBak0)); memcpy(awKeyItemBak, pA->awKeyItem, sizeof(awKeyItemBak));
	memcpy(awJobKeyItemBak, pA->awJobKeyItem, sizeof(awJobKeyItemBak));
	/* 特技IDは名前のあるもの先頭8つ */
	BYTE abyID[8];
	int nID = 0;
	for (int i = 1; i < g_aMagicInfo.GetSize() && i < 256 && nID < 8; i ++) {
		if (g_aMagicInfo[i] && g_aMagicInfo[i]->szName[0]) abyID[nID ++] = (BYTE)i;
	}
	printf("jobkey ids=%d\n", nID);
	if (nID < 8) {
		return;
	}
	const BYTE byJobA = 1, byJobB = 2;

	/* (1) ChangeJobMagicKey */
	ZeroMemory(pA->abyMagic, sizeof(pA->abyMagic));
	ZeroMemory(pA->abyMagicKey, sizeof(pA->abyMagicKey));
	ZeroMemory(pA->awKeyItem, sizeof(pA->awKeyItem));
	ZeroMemory(pA->abyJobMagicKey, sizeof(pA->abyJobMagicKey));
	ZeroMemory(pA->awJobKeyItem, sizeof(pA->awJobKeyItem));
	pA->dwJobMagicKeySaved = 0;
	pA->byJOB = byJobA;
	pA->abyMagic[0] = abyID[0]; pA->abyMagic[1] = abyID[1]; pA->abyMagic[2] = abyID[2];
	pA->abyMagicKey[0] = abyID[0]; pA->abyMagicKey[1] = abyID[1];		/* 特技の枠 */
	pA->awKeyItem[2] = 77;												/* 道具の枠 */
	JobKeyPrint("A set", pA);
	/* B へ（初めて）：B の abyMagic は abyID[1] だけ → abyID[0] の枠は外れ、abyID[1]・道具の枠は残る */
	ZeroMemory(pA->abyMagic, sizeof(pA->abyMagic));
	pA->abyMagic[0] = abyID[1]; pA->abyMagic[1] = abyID[3];
	pA->byJOB = byJobB;
	ChangeJobMagicKey(pA, byJobA, byJobB);
	JobKeyPrint("to B first", pA);
	/* B で別の枠 */
	pA->abyMagicKey[3] = abyID[3];
	JobKeyPrint("B set", pA);
	/* A へ戻す：A の枠が戻る（abyMagic は A の呪文に戻した状態） */
	ZeroMemory(pA->abyMagic, sizeof(pA->abyMagic));
	pA->abyMagic[0] = abyID[0]; pA->abyMagic[1] = abyID[1]; pA->abyMagic[2] = abyID[2];
	pA->byJOB = byJobA;
	ChangeJobMagicKey(pA, byJobB, byJobA);
	JobKeyPrint("back A", pA);
	/* B へ戻す：B の枠が戻る */
	ZeroMemory(pA->abyMagic, sizeof(pA->abyMagic));
	pA->abyMagic[0] = abyID[1]; pA->abyMagic[1] = abyID[3];
	pA->byJOB = byJobB;
	ChangeJobMagicKey(pA, byJobA, byJobB);
	JobKeyPrint("back B", pA);
	/* 同じ職業・範囲外の職業は何もしない／覚えない */
	ChangeJobMagicKey(pA, byJobB, byJobB);
	JobKeyPrint("same", pA);

	/* (2) SortMagicList */
	{
		BYTE abyM[MAX_USEMAGIC];
		for (int mode = 0; mode < 3; mode ++) {
			const char *apszName[] = { "swap", "type", "name", "name again" };
			ZeroMemory(abyM, sizeof(abyM));
			if (mode == 0) {
				/* 空きを含む並び：詰めてから入れ替え（詰めた後の番号） */
				abyM[1] = abyID[0]; abyM[3] = abyID[1]; abyM[4] = abyID[2]; abyM[7] = abyID[3];
				int r = SortMagicList(abyM, SORTITEMMODE_SWAP, 0, 2);
				MagicListPrint("swap 0,2 (gaps packed)", r, abyM);
				r = SortMagicList(abyM, SORTITEMMODE_SWAP, 1, 9);
				MagicListPrint("swap out of range", r, abyM);
				r = SortMagicList(abyM, SORTITEMMODE_SWAP, 1, 1);
				MagicListPrint("swap same", r, abyM);
			} else {
				for (int i = 0; i < 8; i ++) abyM[i] = abyID[7 - i];
				abyM[9] = abyID[0];											/* 空きの後ろにあるもの */
				BYTE byMode = (mode == 1) ? SORTITEMMODE_TYPE : SORTITEMMODE_NAME;
				int r = SortMagicList(abyM, byMode, 0, 0);
				MagicListPrint(apszName[mode], r, abyM);
				BYTE abyCopy[MAX_USEMAGIC];
				memcpy(abyCopy, abyM, sizeof(abyCopy));
				r = SortMagicList(abyM, byMode, 0, 0);
				MagicListPrint((mode == 1) ? "type again(desc)" : "name again(desc)", r, abyM);
				int nZero = 0, nTail = 1;
				for (int i = 0; i < MAX_USEMAGIC; i ++) { if (abyM[i] == 0) nZero ++; else if (nZero) nTail = 0; }
				printf("magicsort %s packed=%d zeros=%d\n", apszName[mode], nTail, nZero);
			}
		}
		/* 1つ以下は並べ替えない・不正な方法 */
		ZeroMemory(abyM, sizeof(abyM)); abyM[0] = abyID[0];
		printf("magicsort single ret=%d bad_mode ret=%d null ret=%d\n", (int)SortMagicList(abyM, SORTITEMMODE_NAME, 0, 0), (int)SortMagicList(abyM, 99, 0, 0), (int)SortMagicList(NULL, SORTITEMMODE_NAME, 0, 0));
	}

	/* (3) 保存→読み込み。列が無い古いデータは全部空 */
	char szAccBak[sizeof(pA->szAcc)], szNameBak[sizeof(pA->szName)];
	memcpy(szAccBak, pA->szAcc, sizeof(szAccBak));
	memcpy(szNameBak, pA->szName, sizeof(szNameBak));
	ZeroMemory(pA->abyJobMagicKey, sizeof(pA->abyJobMagicKey));
	ZeroMemory(pA->awJobKeyItem, sizeof(pA->awJobKeyItem));
	pA->abyJobMagicKey[1][0] = 11; pA->abyJobMagicKey[1][4] = 12;
	pA->abyJobMagicKey[31][2] = 13;
	pA->dwJobMagicKeySaved = (1UL << 1) | (1UL << 31);
	DWORD dwSavedBak = pA->dwJobMagicKeySaved;
	BYTE abyJobKeyBak[sizeof(pA->abyJobMagicKey)];
	memcpy(abyJobKeyBak, pA->abyJobMagicKey, sizeof(abyJobKeyBak));
	strcpy(pA->szAcc, "jobkeytest");
	strcpy(pA->szName, "転職キーテスト");
	BOOL bSaved = SaveCharInfo();
	BOOL bLoaded = LoadCharInfo();
	PCHARINFO pLoaded = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (g_aCharInfo[i] != pA && strcmp(g_aCharInfo[i]->szAcc, "jobkeytest") == 0) pLoaded = g_aCharInfo[i];
	}
	printf("jobkey save=%d load=%d found=%d\n", (int)bSaved, (int)bLoaded, (int)(pLoaded != NULL));
	if (pLoaded) {
		printf("jobkey reload key=%d,%d,%d saved_same=%d key_same=%d\n", pLoaded->abyJobMagicKey[1][0], pLoaded->abyJobMagicKey[1][4], pLoaded->abyJobMagicKey[31][2],
			(int)(pLoaded->dwJobMagicKeySaved == dwSavedBak), (int)(memcmp(pLoaded->abyJobMagicKey, abyJobKeyBak, sizeof(abyJobKeyBak)) == 0));
		pLoaded->szAcc[0] = 0;
	}
	{
		const char *pszFile = "SBODATA/SboDataCharInfo.dat";
		strcpy(pA->szAcc, "jobkeyold");
		SaveCharInfo();
		std::vector<BYTE> aFile;
		FILE *fp = fopen(pszFile, "rb");
		if (fp) {
			BYTE abyBuf[4096];
			size_t n;
			while ((n = fread(abyBuf, 1, sizeof(abyBuf), fp)) > 0) aFile.insert(aFile.end(), abyBuf, abyBuf + n);
			fclose(fp);
		}
		int nPatched = 0;
		if (aFile.size() > 8) {
			DWORD dwColumn = 0;
			size_t nHead = 8;
			memcpy(&dwColumn, &aFile[0], 4);
			for (DWORD c = 0; c < dwColumn; c ++) {
				char *pszName = (char *)&aFile[nHead];
				if (strcmp(pszName, "abyJobMagicKey") == 0) { pszName[0] = 'X'; nPatched ++; }
				nHead += strlen(pszName) + 1 + 4;
			}
			fp = fopen(pszFile, "wb");
			if (fp) { fwrite(&aFile[0], 1, aFile.size(), fp); fclose(fp); }
		}
		LoadCharInfo();
		PCHARINFO pOld = NULL;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (g_aCharInfo[i] != pA && g_aCharInfo[i] != pLoaded && strcmp(g_aCharInfo[i]->szAcc, "jobkeyold") == 0) pOld = g_aCharInfo[i];
		}
		if (pOld) {
			int nAny = 0;
			for (size_t i = 0; i < sizeof(pOld->abyJobMagicKey); i ++) nAny |= ((BYTE *)pOld->abyJobMagicKey)[i];
			printf("jobkey old save patched=%d found=1 all_zero=%d saved=%u name_ok=%d\n", nPatched, (int)(nAny == 0), (unsigned)pOld->dwJobMagicKeySaved, (int)(strcmp(pOld->szName, "転職キーテスト") == 0));
			pOld->szAcc[0] = 0;
		} else {
			printf("jobkey old save patched=%d not found\n", nPatched);
		}
	}
	memcpy(pA->szAcc, szAccBak, sizeof(szAccBak));
	memcpy(pA->szName, szNameBak, sizeof(szNameBak));
	pA->byJOB = byJobBak; pA->dwJobMagicKeySaved = dwSavedBak0;
	memcpy(pA->abyMagic, abyMagicBak, sizeof(abyMagicBak)); memcpy(pA->abyMagicKey, abyKeyBak, sizeof(abyKeyBak));
	memcpy(pA->abyJobMagicKey, abyJobKeyBak0, sizeof(abyJobKeyBak0)); memcpy(pA->awKeyItem, awKeyItemBak, sizeof(awKeyItemBak));
	memcpy(pA->awJobKeyItem, awJobKeyItemBak, sizeof(awJobKeyItemBak));
}

/* ------------------------------------------------------------------------- */
/* AG3. マップごとのパーツの灯りの上書き（partsLights）						 */
/*    データAPI の入出力・拒否、保存して読み直す（無いマップは書かない）		 */
/* ------------------------------------------------------------------------- */
static void TestMapPartsLight(void)
{
	printf("== AG3. MapPartsLight ==\n");
	if (g_aMapInfo.GetSize() < 1 || g_wCountMapParts < 2) {
		printf("partslight no map/parts\n");
		return;
	}
	int nMaps = g_aMapInfo.GetSize();
	PMAPINFO pMap = g_aMapInfo[0];
	BYTE byCountBak = pMap->byPartsLightCount;
	MAPPARTSLIGHT aBak[MAX_MAPPARTSLIGHT];
	memcpy(aBak, pMap->aPartsLight, sizeof(aBak));
	auto ok = [](const std::string &s) { return (int)(s.find("\"ok\":true") != std::string::npos); };
	char sz[512];

	/* 書く→取り出すと同じ形で出る */
	snprintf(sz, sizeof(sz), "{\"partsLights\":[{\"wParts\":%d,\"byLight\":2},{\"wParts\":1,\"byLight\":0}]}", (int)g_wCountMapParts - 1);
	std::string strRes = SwapApi(0, sz);
	printf("partslight api ok=%d count=%d row0=%d,%d row1=%d,%d\n", ok(strRes), (int)pMap->byPartsLightCount,
		(int)pMap->aPartsLight[0].wParts, (int)pMap->aPartsLight[0].byLight, (int)pMap->aPartsLight[1].wParts, (int)pMap->aPartsLight[1].byLight);
	std::string strGet = SboWebDataGet("map", 0);
	snprintf(sz, sizeof(sz), "\"partsLights\":[{\"wParts\":%d,\"byLight\":2},{\"wParts\":1,\"byLight\":0}]", (int)g_wCountMapParts - 1);
	printf("partslight api get has=%d\n", (int)(strGet.find(sz) != std::string::npos));

	/* 拒否：重複・範囲・上限・知らない項目・足りない項目 */
	MAPPARTSLIGHT aSaved[MAX_MAPPARTSLIGHT];
	memcpy(aSaved, pMap->aPartsLight, sizeof(aSaved));
	printf("partslight api dup ok=%d\n", ok(SwapApi(0, "{\"partsLights\":[{\"wParts\":1,\"byLight\":1},{\"wParts\":1,\"byLight\":2}]}")));
	printf("partslight api light range ok=%d\n", ok(SwapApi(0, "{\"partsLights\":[{\"wParts\":1,\"byLight\":8}]}")));
	snprintf(sz, sizeof(sz), "{\"partsLights\":[{\"wParts\":%d,\"byLight\":1}]}", (int)g_wCountMapParts);
	printf("partslight api parts range ok=%d\n", ok(SwapApi(0, sz)));
	printf("partslight api unknown key ok=%d\n", ok(SwapApi(0, "{\"partsLights\":[{\"wParts\":1,\"byLight\":1,\"x\":1}]}")));
	printf("partslight api missing key ok=%d\n", ok(SwapApi(0, "{\"partsLights\":[{\"wParts\":1}]}")));
	{
		std::string strMany = "{\"partsLights\":[";
		for (int i = 0; i <= MAX_MAPPARTSLIGHT; i ++) {
			char szOne[64];
			snprintf(szOne, sizeof(szOne), "%s{\"wParts\":%d,\"byLight\":1}", i ? "," : "", i % (int)g_wCountMapParts);
			strMany += szOne;
		}
		strMany += "]}";
		printf("partslight api over limit ok=%d\n", ok(SwapApi(0, strMany.c_str())));
	}
	printf("partslight api after bad same=%d\n", (int)(pMap->byPartsLightCount == 2 && memcmp(aSaved, pMap->aPartsLight, sizeof(aSaved)) == 0));

	/* 保存して読み直す（足された分のマップで確かめる） */
	extern BOOL SaveMapInfo(void);
	extern BOOL LoadMapInfo(void);
	DWORD dwNewMapIDBak = g_dwNewMapID;
	BOOL bSaved = SaveMapInfo();
	BOOL bLoaded = LoadMapInfo();
	int nAdded = g_aMapInfo.GetSize() - nMaps;
	if (nAdded >= 1) {
		PMAPINFO pL = g_aMapInfo[nMaps];
		printf("partslight map save=%d load=%d count=%d row0=%d,%d row1=%d,%d\n", (int)bSaved, (int)bLoaded, (int)pL->byPartsLightCount,
			(int)pL->aPartsLight[0].wParts, (int)pL->aPartsLight[0].byLight, (int)pL->aPartsLight[1].wParts, (int)pL->aPartsLight[1].byLight);
	} else {
		printf("partslight map save=%d load=%d added=%d\n", (int)bSaved, (int)bLoaded, nAdded);
	}
	/* 0 件なら書かない（古いデータと同じ形で、読み直しても 0 件） */
	SwapApi(0, "{\"partsLights\":[]}");
	for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
		g_aMapInfo.RemoveAt(i);
	}
	int nMaps2 = g_aMapInfo.GetSize();
	SaveMapInfo();
	LoadMapInfo();
	printf("partslight map empty count=%d\n", (nMaps2 < g_aMapInfo.GetSize()) ? (int)g_aMapInfo[nMaps2]->byPartsLightCount : -1);
	for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps2; i --) {
		g_aMapInfo.RemoveAt(i);
	}
	g_dwNewMapID = dwNewMapIDBak;

	pMap->byPartsLightCount = byCountBak;
	memcpy(pMap->aPartsLight, aBak, sizeof(aBak));
}

static void TestMapSwapSave(const std::vector<int> &aLogin)
{
	printf("== AG2. MapSwapSave ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[2]];
	char szAccBak[sizeof(pA->szAcc)], szNameBak[sizeof(pA->szName)];
	memcpy(szAccBak, pA->szAcc, sizeof(szAccBak));
	memcpy(szNameBak, pA->szName, sizeof(szNameBak));
	BYTE abyBak[MAX_CHESTBYTE];
	memcpy(abyBak, pA->abyChestOpened, sizeof(abyBak));

	/* キャラの列：開けた宝箱の記録（先頭・途中・最後の番号）を保存して読み直す */
	ZeroMemory(pA->abyChestOpened, sizeof(pA->abyChestOpened));
	ChestOpenedSet(pA, 0, TRUE); ChestOpenedSet(pA, 100, TRUE); ChestOpenedSet(pA, MAX_CHESTNO - 1, TRUE);
	printf("chest bit range get=%d,%d,%d,%d set_out=", (int)ChestOpenedGet(pA, 0), (int)ChestOpenedGet(pA, 1), (int)ChestOpenedGet(pA, MAX_CHESTNO - 1), (int)ChestOpenedGet(pA, MAX_CHESTNO));
	{
		BYTE abyCopy[MAX_CHESTBYTE];
		memcpy(abyCopy, pA->abyChestOpened, sizeof(abyCopy));
		ChestOpenedSet(pA, MAX_CHESTNO, TRUE); ChestOpenedSet(pA, -1, TRUE);
		printf("%d\n", (int)(memcmp(abyCopy, pA->abyChestOpened, sizeof(abyCopy)) == 0));
	}
	strcpy(pA->szAcc, "chesttest");
	strcpy(pA->szName, "宝箱テスト");
	BOOL bSaved = SaveCharInfo();
	BOOL bLoaded = LoadCharInfo();
	PCHARINFO pLoaded = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		if (g_aCharInfo[i] != pA && strcmp(g_aCharInfo[i]->szAcc, "chesttest") == 0) pLoaded = g_aCharInfo[i];
	}
	printf("chest char save=%d load=%d found=%d\n", (int)bSaved, (int)bLoaded, (int)(pLoaded != NULL));
	if (pLoaded) {
		printf("chest char reload bits=%d,%d,%d,%d,%d same=%d\n", (int)ChestOpenedGet(pLoaded, 0), (int)ChestOpenedGet(pLoaded, 100), (int)ChestOpenedGet(pLoaded, MAX_CHESTNO - 1),
			(int)ChestOpenedGet(pLoaded, 1), (int)ChestOpenedGet(pLoaded, 101), (int)(memcmp(pLoaded->abyChestOpened, pA->abyChestOpened, MAX_CHESTBYTE) == 0));
		pLoaded->szAcc[0] = 0;
	}
	/* 古い保存データ（abyChestOpened の列が無い）：全部 0（まだ開けていない）で読める */
	{
		const char *pszFile = "SBODATA/SboDataCharInfo.dat";
		strcpy(pA->szAcc, "chestold");
		SaveCharInfo();
		std::vector<BYTE> aFile;
		FILE *fp = fopen(pszFile, "rb");
		if (fp) {
			BYTE abyBuf[4096];
			size_t n;
			while ((n = fread(abyBuf, 1, sizeof(abyBuf), fp)) > 0) aFile.insert(aFile.end(), abyBuf, abyBuf + n);
			fclose(fp);
		}
		int nPatched = 0;
		if (aFile.size() > 8) {
			DWORD dwColumn = 0;
			size_t nHead = 8;
			memcpy(&dwColumn, &aFile[0], 4);
			for (DWORD c = 0; c < dwColumn; c ++) {
				char *pszName = (char *)&aFile[nHead];
				if (strcmp(pszName, "abyChestOpened") == 0) { pszName[0] = 'X'; nPatched ++; }
				nHead += strlen(pszName) + 1 + 4;
			}
			fp = fopen(pszFile, "wb");
			if (fp) { fwrite(&aFile[0], 1, aFile.size(), fp); fclose(fp); }
		}
		LoadCharInfo();
		PCHARINFO pOld = NULL;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			if (g_aCharInfo[i] != pA && g_aCharInfo[i] != pLoaded && strcmp(g_aCharInfo[i]->szAcc, "chestold") == 0) pOld = g_aCharInfo[i];
		}
		if (pOld) {
			int nAny = 0;
			for (int i = 0; i < MAX_CHESTBYTE; i ++) nAny |= pOld->abyChestOpened[i];
			printf("chest old save patched=%d found=1 all_zero=%d name_ok=%d\n", nPatched, (int)(nAny == 0), (int)(strcmp(pOld->szName, "宝箱テスト") == 0));
			pOld->szAcc[0] = 0;
		} else {
			printf("chest old save patched=%d not found\n", nPatched);
		}
	}
	memcpy(pA->szAcc, szAccBak, sizeof(szAccBak));
	memcpy(pA->szName, szNameBak, sizeof(szNameBak));
	memcpy(pA->abyChestOpened, abyBak, sizeof(abyBak));

	/* マップの項目：保存して読み直す（読み込みは末尾に足されるので、足された分と元のマップを比べる）。
	   印で変わるマスの無いマップ（＝古いデータと同じ形）はそのまま読める */
	int nMaps = g_aMapInfo.GetSize();
	int nMap = 0;
	PMAPINFO pMap = g_aMapInfo[nMap];
	MAPINFO Bak = *pMap;
	ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
	pMap->bySwapCount = 4;
	pMap->aSwap[0].wX = 1; pMap->aSwap[0].wY = 2; pMap->aSwap[0].byKind = MAPSWAPKIND_CHEST; pMap->aSwap[0].wParts = 1; pMap->aSwap[0].wGiveItem = 1; pMap->aSwap[0].wChestNo = 77;
	strcpy(pMap->aSwap[0].szMsg, "宝箱を開けた"); strcpy(pMap->aSwap[0].szMemo, "ルシー 港の倉庫の奥");
	for (int i = 0; i < 4; i ++) pMap->aSwap[i].wNeedFlag = FLAG_NOBIT;
	pMap->aSwap[0].wNeedFlag = 241; strcpy(pMap->aSwap[0].szLockMsg, "カルバの話を聞いてからだ。");		/* 開ける条件（宝箱だけ） */
	pMap->aSwap[2].wNeedFlag = 0;				/* 印 0 も条件にできる（65535 だけが条件なし） */
	pMap->aSwap[1].wX = 3; pMap->aSwap[1].wY = 4; pMap->aSwap[1].byKind = MAPSWAPKIND_FLAG; pMap->aSwap[1].wFlag = 1023; pMap->aSwap[1].byCmp = MAPSWAPCMP_OFF; pMap->aSwap[1].wParts = 2;
	pMap->aSwap[2].wX = 5; pMap->aSwap[2].wY = 6; pMap->aSwap[2].byKind = MAPSWAPKIND_CHEST; pMap->aSwap[2].wParts = 3; pMap->aSwap[2].wChestNo = 5;
	pMap->aSwap[3].wX = 7; pMap->aSwap[3].wY = 8; pMap->aSwap[3].byKind = MAPSWAPKIND_RECYCLE; pMap->aSwap[3].wParts = 4; pMap->aSwap[3].wBoxNo = 6;
	strcpy(pMap->aSwap[3].szMemo, "ルシー 広場の箱");
	pMap->wChestHigh = 90;						/* 消した番号 89 までを使ったことの覚え */
	pMap->wBoxHigh = 12;						/* 消した箱番号 11 までを使ったことの覚え */
	extern BOOL SaveMapInfo(void);
	extern BOOL LoadMapInfo(void);
	BOOL bMapSaved = SaveMapInfo();
	SwapRestore(pMap, Bak);						/* 元の（差し替え無しの）マップは古いデータと同じ形 */
	DWORD dwNewMapIDBak = g_dwNewMapID;
	BOOL bMapLoaded = LoadMapInfo();
	int nAdded = g_aMapInfo.GetSize() - nMaps;
	if (nAdded >= 1) {
		PMAPINFO pL = g_aMapInfo[nMaps + nMap];
		printf("mapswap map save=%d load=%d added=%d count=%d high=%d row0=%d,%d,%d,%d,%d,%d,%s,%s row1=%d,%d,%d,%d,%d,%d row2=%d,%d,%d\n", (int)bMapSaved, (int)bMapLoaded, nAdded,
			(int)pL->bySwapCount, (int)pL->wChestHigh,
			(int)pL->aSwap[0].wX, (int)pL->aSwap[0].wY, (int)pL->aSwap[0].byKind, (int)pL->aSwap[0].wParts, (int)pL->aSwap[0].wGiveItem, (int)pL->aSwap[0].wChestNo,
			pL->aSwap[0].szMsg, pL->aSwap[0].szMemo,
			(int)pL->aSwap[1].wX, (int)pL->aSwap[1].wY, (int)pL->aSwap[1].byKind, (int)pL->aSwap[1].wFlag, (int)pL->aSwap[1].byCmp, (int)pL->aSwap[1].wParts,
			(int)pL->aSwap[2].wX, (int)pL->aSwap[2].wChestNo, (int)pL->aSwap[2].wParts);
		printf("mapswap map recycle row3=%d,%d,%d,%d,%d,%s boxhigh=%d chesthigh=%d\n", (int)pL->aSwap[3].wX, (int)pL->aSwap[3].wY, (int)pL->aSwap[3].byKind,
			(int)pL->aSwap[3].wParts, (int)pL->aSwap[3].wBoxNo, pL->aSwap[3].szMemo, (int)pL->wBoxHigh, (int)pL->wChestHigh);
		printf("mapswap map lock row0=%d,%s row1=%d,%d row2=%d,%d row3=%d,%d\n", (int)pL->aSwap[0].wNeedFlag, pL->aSwap[0].szLockMsg,
			(int)pL->aSwap[1].wNeedFlag, (int)pL->aSwap[1].szLockMsg[0], (int)pL->aSwap[2].wNeedFlag, (int)pL->aSwap[2].szLockMsg[0],
			(int)pL->aSwap[3].wNeedFlag, (int)pL->aSwap[3].szLockMsg[0]);
		/* 差し替えの無い別のマップ（古いデータと同じ）：読み直しても 0 件・マス目は同じ */
		if (nMaps >= 2) {
			PMAPINFO pOrig = g_aMapInfo[1], pOld = g_aMapInfo[nMaps + 1];
			printf("mapswap map old-format count=%d high=%d tiles_same=%d\n", (int)pOld->bySwapCount, (int)pOld->wChestHigh,
				(int)(pOrig->nCX == pOld->nCX && pOrig->nCY == pOld->nCY && memcmp(pOrig->pwMap, pOld->pwMap, pOrig->nCX * pOrig->nCY * sizeof(WORD)) == 0));
		}
	} else {
		printf("mapswap map save=%d load=%d added=%d\n", (int)bMapSaved, (int)bMapLoaded, nAdded);
	}
	/* 読み込みは末尾に足されるので、足された分は外して元に戻す（あとのテストに響かないように） */
	for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
		g_aMapInfo.RemoveAt(i);
	}
	/* 開ける条件を決めた宝箱が 1 つも無いマップ（古いデータと同じ形。項目 aSwapLock を書かない）：条件なし・文は空で読める。
	   先頭の宝箱の文だけ（印は条件なしのまま）でも書いて読める */
	for (int nRound = 0; nRound < 2; nRound ++) {
		ZeroMemory(pMap->aSwap, sizeof(pMap->aSwap));
		pMap->bySwapCount = 2;
		pMap->aSwap[0].wX = 1; pMap->aSwap[0].wY = 2; pMap->aSwap[0].byKind = MAPSWAPKIND_CHEST; pMap->aSwap[0].wParts = 1; pMap->aSwap[0].wChestNo = 77;
		pMap->aSwap[1].wX = 3; pMap->aSwap[1].wY = 4; pMap->aSwap[1].byKind = MAPSWAPKIND_FLAG; pMap->aSwap[1].wFlag = 5; pMap->aSwap[1].wParts = 2;
		for (int i = 0; i < 2; i ++) pMap->aSwap[i].wNeedFlag = FLAG_NOBIT;
		if (nRound == 1) strcpy(pMap->aSwap[0].szLockMsg, "文だけ");
		SaveMapInfo();
		SwapRestore(pMap, Bak);
		LoadMapInfo();
		int nAdd2 = g_aMapInfo.GetSize() - nMaps;
		if (nAdd2 >= 1) {
			PMAPINFO pL = g_aMapInfo[nMaps + nMap];
			printf("mapswap map nolock round=%d count=%d row0=%d,%d,%s row1=%d,%d,%d\n", nRound, (int)pL->bySwapCount, (int)pL->aSwap[0].wChestNo, (int)pL->aSwap[0].wNeedFlag,
				pL->aSwap[0].szLockMsg, (int)pL->aSwap[1].wFlag, (int)pL->aSwap[1].wNeedFlag, (int)pL->aSwap[1].szLockMsg[0]);
		} else {
			printf("mapswap map nolock round=%d added=%d\n", nRound, nAdd2);
		}
		for (int i = g_aMapInfo.GetSize() - 1; i >= nMaps; i --) {
			g_aMapInfo.RemoveAt(i);
		}
	}
	g_dwNewMapID = dwNewMapIDBak;
}

#include "bow.inc"

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *pszData = (argc >= 2) ? argv[1] : NULL;
	if (pszData && chdir(pszData) != 0) {
		fprintf(stderr, "chdir failed\n");
		return 1;
	}
	if (argc >= 3 && strcmp(argv[2], "--bow") == 0) return TestBowOnly();
	TestMapNo();
	TestCharNo();
	TestItemMsgWait();
	TestZlib();
	TestZlibPackets();
	TestSend();
	TestMyArray();
	TestSugorokuViewer();
	TestWorld();
	printf("== done ==\n");
	fflush(stdout);
	_exit(0);
}


/* ------------------------------------------------------------------------- */
/* AK. 攻撃要求の威力欄は、前の攻撃からの経過時間から出せる上限で切り詰める	 */
/*     (最初と十分待った時は満額・早撃ちは周ごとに 20%・ジャストは満タン後だけ) */
/* ------------------------------------------------------------------------- */
static BYTE TestAttackSend(PCHARINFO p, BYTE byData, DWORD dwAdvance)
{
	CMDPARA_DATA4 Para;
	s_dwFakeNow += dwAdvance;
	g_dwTime = s_dwFakeNow;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_REQ_ATTACK;
	Para.pID = p->pID;
	Para.dwTblID = p->dwTblID;
	Para.byData = byData;
	Para.dwData = (DWORD)-1;
	p->byAtackRate = 0;
	TestRecv(p, &Para, sizeof(Para));
	return p->byAtackRate;
}

static void TestAttackRate(const std::vector<int> &aLogin)
{
	printf("== AK. Attack rate ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	PCHARINFO p = g_aCharInfo[aLogin[0]];
	BYTE byDEX = p->byDEX, byState = p->byCharState;
	WORD wHP = p->wHP, wEqu = p->waEqu[0];
	p->byDEX = 0;			/* ゲージは約 3 秒で満タン */
	p->wHP = max(p->wHP, (WORD)10);
	p->byCharState = CHARSTATE_MOVE;
	p->waEqu[0] = 0xFFFF;
	printf("attackrate first(255)=%d\n", (int)TestAttackSend(p, 255, 20000));
	printf("attackrate fast(255)=%d\n", (int)TestAttackSend(p, 255, 100));
	printf("attackrate mid(100)=%d\n", (int)TestAttackSend(p, 100, 1500));
	printf("attackrate full(255)=%d\n", (int)TestAttackSend(p, 255, 3000));
	printf("attackrate just=%d\n", (int)TestAttackSend(p, ATACKRATE_JUST, 3000));
	printf("attackrate just early=%d\n", (int)TestAttackSend(p, ATACKRATE_JUST, 100));
	printf("attackrate normal(40) waited=%d\n", (int)TestAttackSend(p, 40, 3000));
	p->byDEX = byDEX; p->wHP = wHP; p->byCharState = byState; p->waEqu[0] = wEqu;
}

/* ------------------------------------------------------------------------- */
/* AJ. 戦闘モードの覚え(s_mapBattleMode)が切断で消え、残っていても			 */
/*     「ついていく」に返事が来る・範囲外の相手番号は NG2					 */
/* ------------------------------------------------------------------------- */
static void TestTailRelogin(const std::vector<int> &aLogin)
{
	printf("== AJ. Tail relogin ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	int nX0 = -1, nY0 = -1;
	for (int y = 2; y + 2 < pPort->nCY && nX0 < 0; y ++) {
		for (int x = 1; x + 1 < pPort->nCX && nX0 < 0; x ++) {
			bool bOpen = true;
			for (int dy = -1; dy <= 2 && bOpen; dy ++) {
				for (int dx = -1; dx <= 1 && bOpen; dx ++) {
					if (!CheckCharBlock(pPort->dwID, x + dx, y + dy) || CheckEventBlock(pPort->dwID, x + dx, y + dy)) bOpen = false;
				}
			}
			if (bOpen) { nX0 = x; nY0 = y; }
		}
	}
	printf("tailrelogin area found=%d\n", nX0 >= 0 ? 1 : 0);
	if (nX0 < 0) {
		return;
	}
	int nUp = 0;
	for (int d = 0; d < 4; d ++) if (anPosAddTblX[d] == 0 && anPosAddTblY[d] == -1) nUp = d;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]];
	DWORD adwBack[2][6];
	PCHARINFO apChar[2] = { pA, pB };
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID; adwBack[i][5] = p->byDirection;
	}
	/* B が奥、A がその下(B の方を向く)。どちらも列に入っていない通常の状態 */
	auto place = [&]() {
		for (int i = 0; i < 2; i ++) {
			PCHARINFO p = apChar[i];
			DeleteTblNoPC((int)p->dwTblID);
			p->dwMapID = pPort->dwID;
			p->dwX = p->dwXBack = nX0;
			p->dwY = p->dwYBack = nY0 + (i == 0 ? 1 : 0);
			p->byCharState = CHARSTATE_MOVE;
			if (p->wHP == 0) p->wHP = 10;
			p->dwTailTblID = p->dwTailBackTblID = (DWORD)-1;
			p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
			AddTblNoPC((int)p->dwTblID);
		}
		pA->byDirection = (BYTE)nUp;
	};
	/* 最後に届いた CHAR_TAIL の種類（届かなければ -1） */
	auto sendZ = [&](DWORD dwTarget) -> int {
		MockTrace(1);
		CCHAR_TAIL Z;
		Z.Make(pA->dwTblID, dwTarget, CHAR_TAIL_REQ);
		TestRecv(pA, Z.Get(), Z.GetSize());
		MockTrace(0);
		char *psz = MockLastBody(1000, CMD_MAIN_CHAR);
		unsigned int u = 0xFFFFFFFF, uSub = 0;
		if (strlen(psz) >= 28) { sscanf(psz + 2, "%2x", &uSub); if (uSub == CMDCHAR_TAIL) sscanf(psz + 20, "%2x", &u); }
		free(psz);
		return (int)u;
	};

	/* A: 戦闘モードにして切断(OnDecPlayer 相当)→同じ pID で入り直し→ B を向いて Z */
	place();
	TailBattleSend(pA, 1);
	printf("tailrelogin battle before=%d\n", IsCharBattleMode(pA) ? 1 : 0);
	RecvCmdOnDecPlayer((int)pA->dwTblID);
	printf("tailrelogin battle after logout=%d\n", IsCharBattleMode(pA) ? 1 : 0);
	int nRes = sendZ(pB->dwTblID);
	printf("tailrelogin relogin on=%d front=%d\n", nRes == CHAR_TAIL_ON ? 1 : 0, pA->dwTailTblID == pB->dwTblID ? 1 : 0);

	/* サーバーが戦闘モードと思っている時の Z は何もしないが、クライアントを動けるように戻す返事(CHASEEND)は来る */
	place();
	TailBattleSend(pA, 1);
	nRes = sendZ(pB->dwTblID);
	printf("tailrelogin stale chaseend=%d front=%d battle=%d\n", nRes == CHAR_TAIL_CHASEEND ? 1 : 0, (int)pA->dwTailTblID, IsCharBattleMode(pA) ? 1 : 0);

	/* 範囲外の相手番号 → NG2（通常モードに戻してから） */
	TailBattleSend(pA, 0);
	MercenarySetBattleMode(pA, FALSE);
	place();
	nRes = sendZ((DWORD)g_aCharInfo.GetSize() + 5);
	printf("tailrelogin range ng2=%d front=%d\n", nRes == CHAR_TAIL_NG2 ? 1 : 0, (int)pA->dwTailTblID);

	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		p->byDirection = (BYTE)adwBack[i][5];
		AddTblNoPC((int)p->dwTblID);
	}
	MercenarySetBattleMode(pA, FALSE);
}


/* ------------------------------------------------------------------------- */
/* AK. 先頭(P)が戦闘モードON→列を離れた2人目(Q)と同じマスに重なって解除		 */
/*     → Q はワープせずその場で列につながる(RETURN が届き FADEINOUT は無い)	 */
/* ------------------------------------------------------------------------- */
static void TestTailOverlap(const std::vector<int> &aLogin)
{
	printf("== AK. Tail overlap ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	int nX0 = -1, nY0 = -1;
	for (int y = 2; y + 2 < pPort->nCY && nX0 < 0; y ++) {
		for (int x = 1; x + 1 < pPort->nCX && nX0 < 0; x ++) {
			bool bOpen = true;
			for (int dy = -1; dy <= 2 && bOpen; dy ++) {
				for (int dx = -1; dx <= 1 && bOpen; dx ++) {
					if (!CheckCharBlock(pPort->dwID, x + dx, y + dy) || CheckEventBlock(pPort->dwID, x + dx, y + dy)) bOpen = false;
				}
			}
			if (bOpen) { nX0 = x; nY0 = y; }
		}
	}
	printf("tailoverlap area found=%d\n", nX0 >= 0 ? 1 : 0);
	if (nX0 < 0) {
		return;
	}
	PCHARINFO pP = g_aCharInfo[aLogin[0]], pQ = g_aCharInfo[aLogin[1]];
	PCHARINFO apChar[2] = { pP, pQ };
	DWORD adwBack[2][5];
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = pPort->dwID;
		p->dwX = p->dwXBack = nX0;
		p->dwY = p->dwYBack = nY0 + i;
		p->byCharState = CHARSTATE_MOVE;
		if (p->wHP == 0) p->wHP = 10;
		p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
		AddTblNoPC((int)p->dwTblID);
	}
	pP->dwTailTblID = (DWORD)-1;
	pP->dwTailBackTblID = pQ->dwTblID;
	pQ->dwTailTblID = pP->dwTblID;
	pQ->dwTailBackTblID = (DWORD)-1;
	MercenarySetBattleMode(pP, FALSE);
	MercenarySetBattleMode(pQ, FALSE);

	/* P がON：Q も列を離れる。P が Q と同じマスへ重なる */
	TailBattleSend(pP, 1);
	printf("tailoverlap on Q(front=%d)\n", (int)pQ->dwTailTblID);
	DeleteTblNoPC((int)pP->dwTblID);
	pP->dwX = pP->dwXBack = pQ->dwX;
	pP->dwY = pP->dwYBack = pQ->dwY;
	AddTblNoPC((int)pP->dwTblID);
	DWORD dwQX = pQ->dwX, dwQY = pQ->dwY;

	MockTrace(1);
	TailBattleSend(pP, 0);
	MockTrace(0);
	char *pszFade = MockCmdData(1001, CMD_FADEINOUT);
	char *psz = MockLastBody(1001, CMD_MAIN_CHAR);
	unsigned int u = 0xFFFFFFFF, uSub = 0;
	if (strlen(psz) >= 28) { sscanf(psz + 2, "%2x", &uSub); if (uSub == CMDCHAR_TAIL) sscanf(psz + 20, "%2x", &u); }
	printf("tailoverlap off Q(front=P:%d) P(back=Q:%d) Q(stay=%d) return=%d fade=[%s]\n",
		pQ->dwTailTblID == pP->dwTblID ? 1 : 0, pP->dwTailBackTblID == pQ->dwTblID ? 1 : 0,
		(pQ->dwX == dwQX && pQ->dwY == dwQY) ? 1 : 0, u == CHAR_TAIL_RETURN ? 1 : 0, pszFade);
	free(pszFade);
	free(psz);

	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		AddTblNoPC((int)p->dwTblID);
	}
	MercenarySetBattleMode(pP, FALSE);
	MercenarySetBattleMode(pQ, FALSE);
}

/* ------------------------------------------------------------------------- */
/* AL. イベントが1つも無いマップで CMD_CHECKEVENT(byData=0) → CHARPOS が返る	 */
/*     (黙って戻ると、クライアントは返事待ちのまま歩けなくなる)				 */
/* ------------------------------------------------------------------------- */
static void TestEmptyMapCheck(const std::vector<int> &aLogin)
{
	printf("== AL. Empty map check ==\n");
	if (aLogin.empty()) {
		return;
	}
	PMAPINFO pMap = g_aMapInfo[GetMapNo(14)];
	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	DWORD adwBack[3] = { pA->dwMapID, pA->dwX, pA->dwY };
	PEVENTINFO pEvBack = pMap->pEvent;
	int nCountBack = pMap->nEventCount;
	DeleteTblNoPC((int)pA->dwTblID);
	pA->dwMapID = pMap->dwID;
	pA->dwX = pA->dwXBack = 1;
	pA->dwY = pA->dwYBack = 1;
	pA->dwTargetTblID3 = (DWORD)-1;
	AddTblNoPC((int)pA->dwTblID);
	/* イベントを一時的に 0 件にする（NULL と 0 件の両方） */
	for (int k = 0; k < 2; k ++) {
		pMap->pEvent = (k == 0) ? NULL : pEvBack;
		pMap->nEventCount = 0;
		MockTrace(1);
		CMDPARA_DATA1 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_CHECKEVENT;
		Para.pID = pA->pID;
		Para.dwTblID = pA->dwTblID;
		Para.byData = 0;
		TestRecv(pA, &Para, sizeof(Para));
		MockTrace(0);
		char *psz = MockCmdData(1000, CMD_CHARPOS);
		char *pszMsg = MockCmdData(1000, CMD_MAPMSG);
		printf("emptymap %s charpos=%d mapmsg=%d\n", k == 0 ? "null" : "zero", psz[0] ? 1 : 0, pszMsg[0] ? 1 : 0);
		free(psz);
		free(pszMsg);
	}
	pMap->pEvent = pEvBack;
	pMap->nEventCount = nCountBack;
	DeleteTblNoPC((int)pA->dwTblID);
	pA->dwMapID = adwBack[0];
	pA->dwX = pA->dwXBack = adwBack[1];
	pA->dwY = pA->dwYBack = adwBack[2];
	AddTblNoPC((int)pA->dwTblID);
}

/* ------------------------------------------------------------------------- */
/* AM. 起動時の NPC発生：発生数まですぐ出し、二度呼んでも増えない			 */
/*     発生数より多くならないか・数えた子の数と byPutCountNow が合うか		 */
/* ------------------------------------------------------------------------- */
static void PutFillCount(int *pnParent, int *pnFull, int *pnOver, int *pnMismatch, int *pnChild, int *pnMimic)
{
	int nCount = g_aCharInfo.GetSize();
	*pnParent = *pnFull = *pnOver = *pnMismatch = *pnChild = *pnMimic = 0;
	for (int i = 0; i < nCount; i ++) {
		PCHARINFO pParent = g_aCharInfo[i];
		if ((pParent == NULL) || (pParent->byNpcType != NPCTYPE_PUT) || (pParent->pProc == NULL)) {
			continue;
		}
		int nChild = 0, nMimic = 0;
		for (int j = 0; j < nCount; j ++) {
			PCHARINFO p = g_aCharInfo[j];
			if ((p != NULL) && (p != pParent) && (p->dwParentTblID == pParent->dwTblID) && p->byNpcType) {
				nChild ++;
				if (p->byNpcType == NPCTYPE_MIMIC) {
					nMimic ++;
				}
			}
		}
		(*pnParent) ++;
		*pnChild += nChild;
		*pnMimic += nMimic;
		if (nChild > pParent->byPutCount) {
			(*pnOver) ++;
		}
		if (nChild != pParent->byPutCountNow) {
			(*pnMismatch) ++;
		}
		if (pParent->byPutCountNow >= pParent->byPutCount) {
			(*pnFull) ++;
		}
	}
}

static void TestPutFill(void)
{
	printf("== AM. Put fill ==\n");
	int nParent, nFull, nOver, nMismatch, nChild, nMimic;
	PutFillCount(&nParent, &nFull, &nOver, &nMismatch, &nChild, &nMimic);
	printf("putfill before parents=%d over=%d mismatch=%d\n", nParent, nOver, nMismatch);
	PutCharFillAll();
	PutFillCount(&nParent, &nFull, &nOver, &nMismatch, &nChild, &nMimic);
	int nChild1 = nChild;
	printf("putfill after parents=%d full=%d over=%d mismatch=%d child>0=%d mimic>0=%d\n",
		nParent, nFull, nOver, nMismatch, (int)(nChild > 0), (int)(nMimic > 0));
	/* もう一度呼んでも増えない */
	PutCharFillAll();
	PutFillCount(&nParent, &nFull, &nOver, &nMismatch, &nChild, &nMimic);
	printf("putfill again added=%d over=%d mismatch=%d\n", nChild - nChild1, nOver, nMismatch);
}


/* ------------------------------------------------------------------------- */
/* AH. 付与（接頭辞・接尾辞）												 */
/*    (a) 派生アイテムの探す・作る（同じ組は同じ番号・未鑑定は別の番号）		 */
/*    (b) 強さ・売値・名前（鑑定済みだけ付与が効く。未鑑定は元のまま）		 */
/*    (c) 装備したまま鑑定して攻撃力が変わる・断る場合（お金・未鑑定でない・近くにいない） */
/*    (d) 付与表・派生表の保存と読み直し										 */
/*    (e) ドロップのくじ引き（率0・ランクの上限・前後が付く）				 */
/* ------------------------------------------------------------------------- */
static void TestAffix(const std::vector<int> &aLogin)
{
	printf("== AH. Affix ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[2]];

	/* 今ある表は退避して、空から始める */
	std::vector<PITEMAFFIXINFO> aAffixBak;
	std::vector<PITEMDERIVEINFO> aDeriveBak;
	for (int i = 0; i < g_aAffixInfo.GetSize(); i ++) aAffixBak.push_back(g_aAffixInfo[i]);
	for (int i = 0; i < g_aItemDerive.GetSize(); i ++) aDeriveBak.push_back(g_aItemDerive[i]);
	g_aAffixInfo.RemoveAll();
	g_aItemDerive.RemoveAll();

	/* 武器（強さと値段がある物） */
	WORD wWeapon = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && i < ITEMID_DERIVE_BASE; i ++) {
		if (g_aItemInfo[i]->szName[0] && (g_aItemInfo[i]->byType == ITEMTYPE_WEAPON) &&
			(g_aItemInfo[i]->byData >= 10) && (g_aItemInfo[i]->dwGold >= 200) && !IsImportantItem((WORD)i)) {
			wWeapon = (WORD)i;
			break;
		}
	}
	printf("affix weapon found=%d\n", wWeapon ? 1 : 0);
	if (wWeapon == 0) {
		return;
	}
	int nBase = g_aItemInfo[wWeapon]->byData;
	DWORD dwGold = g_aItemInfo[wWeapon]->dwGold;

	/* 付与表：前 1(鋭い +20% +2 値段150%)・後ろ 2(＋1 +1 値段120%)・前 3(強い ランク2 +50%) */
	struct { BYTE byPos; const char *pszName; WORD wMask; char chRate; char chAdd; BYTE byRank; WORD wGoldRate; } aDef[] = {
		{ AFFIXPOS_FRONT, "鋭い", AFFIXEQU_ALL, 20, 2, 1, 150 },
		{ AFFIXPOS_BACK, "＋1", AFFIXEQU_ALL, 0, 1, 1, 120 },
		{ AFFIXPOS_FRONT, "強い", AFFIXEQU_WEAPON, 50, 0, 2, 200 },
	};
	for (int i = 0; i < 3; i ++) {
		PITEMAFFIXINFO p = new ITEMAFFIXINFO;
		ZeroMemory(p, sizeof(*p));
		p->wID = (WORD)(i + 1);
		p->byPos = aDef[i].byPos;
		strcpy(p->szName, aDef[i].pszName);
		p->wEquMask = aDef[i].wMask;
		p->chRate = aDef[i].chRate;
		p->chAdd = aDef[i].chAdd;
		p->byRank = aDef[i].byRank;
		p->wWeight = 1;
		p->wGoldRate = aDef[i].wGoldRate;
		g_aAffixInfo.Add(p);
	}

	/* (a) 派生アイテム */
	WORD wU = AffixDeriveFindOrCreate(wWeapon, 1, 2, 1);
	WORD wU2 = AffixDeriveFindOrCreate(wWeapon, 1, 2, 1);
	WORD wK = AffixDeriveFindOrCreate(wWeapon, 1, 2, 0);
	WORD wNone = AffixDeriveFindOrCreate(wWeapon, 0, 0, 0);
	printf("affix derive u=%u same=%d known=%u none=%d rows=%d base=%d valid=%d\n", wU, wU == wU2, wK, wNone == wWeapon,
		g_aItemDerive.GetSize(), ItemBaseID(wU) == wWeapon, ItemIDValid(wU));
	printf("affix id invalid=%d out=%d\n", ItemIDValid(ITEMID_DERIVE_BASE + 99), ItemIDValid(wWeapon));

	/* (b) 強さ・売値・名前 */
	int nExpect = nBase + nBase * 20 / 100 + 3;
	if (nExpect > 255) nExpect = 255;
	printf("affix value base=%d unknown=%d known=%d expect=%d\n", nBase, ItemValueOf(wU), ItemValueOf(wK), nExpect);
	printf("affix gold base=%u unknown=%u known=%u expect=%u\n", dwGold, ItemGoldOf(wU), ItemGoldOf(wK), (unsigned)(dwGold * 150 / 100 * 120 / 100));
	printf("affix name unknown=%s known=%s plain=%s\n", ItemNameOf(wU), ItemNameOf(wK), ItemNameOf(wWeapon));
	/* ランク2の「強い」は武器だけ・防具には効かない */
	WORD wArmor = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && i < ITEMID_DERIVE_BASE; i ++) {
		if (g_aItemInfo[i]->szName[0] && (g_aItemInfo[i]->byType == ITEMTYPE_ARMOR) && (g_aItemInfo[i]->byData >= 10)) { wArmor = (WORD)i; break; }
	}
	if (wArmor) {
		WORD wAr = AffixDeriveFindOrCreate(wArmor, 3, 0, 0);
		printf("affix mask armor=%d same=%d\n", ItemValueOf(wAr), ItemValueOf(wAr) == (int)g_aItemInfo[wArmor]->byData);
	}

	/* (c) 鑑定屋：作り手とNPCを仮に用意し、持ち物を差し替える */
	WORD waItemBak[MAX_MYITEM], waEquBak[MAX_EQUTYPE];
	DWORD dwGoldBak = pA->dwGold, dwMapBak = pA->dwMapID, dwXBak = pA->dwX, dwYBak = pA->dwY;
	WORD wATBak = pA->wAT;
	memcpy(waItemBak, pA->waItem, sizeof(waItemBak));
	memcpy(waEquBak, pA->waEqu, sizeof(waEquBak));
	PMAKERINFO pMaker = new MAKERINFO;
	ZeroMemory(pMaker, sizeof(*pMaker));
	pMaker->byState = RECIPESTATE_ON;
	pMaker->byKind = MAKERKIND_APPRAISE;
	int nMaker = g_aMakerInfo.GetSize();
	g_aMakerInfo.Add(pMaker);
	PCHARINFO pNpc = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pNpc; i ++) {
		if (g_aCharInfo[i]->byNpcType == NPCTYPE_MOVERAND) pNpc = g_aCharInfo[i];
	}
	char *pszSpeechBak = pNpc->pszNpcSpeech;
	DWORD dwNpcMapBak = pNpc->dwMapID, dwNpcXBak = pNpc->dwX, dwNpcYBak = pNpc->dwY;
	char szSpeech[16];
	snprintf(szSpeech, sizeof(szSpeech), "@c%02d", nMaker);
	pNpc->pszNpcSpeech = szSpeech;
	pNpc->dwMapID = pA->dwMapID; pNpc->dwX = pA->dwX; pNpc->dwY = pA->dwY;

	ZeroMemory(pA->waItem, sizeof(pA->waItem));
	for (int i = 0; i < MAX_EQUTYPE; i ++) pA->waEqu[i] = 0xFFFF;
	pA->waItem[0] = wU;
	pA->waItem[1] = wWeapon;
	pA->waEqu[0] = 0;
	pA->byCharState = pA->byCharState;
	RenewATDF(pA->dwTblID);
	WORD wATUnknown = pA->wAT;
	pA->dwGold = 5;
	CMDPARA_APPRAISE Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_APPRAISE; Para.byMaker = (BYTE)nMaker; Para.byNo = 0; Para.dwTblID = pA->dwTblID;
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise lack gold item=%u gold=%u\n", pA->waItem[0], pA->dwGold);
	pA->dwGold = 1000;
	TestRecv(pA, &Para, sizeof(Para));
	DWORD dwCost = AffixAppraiseCost(wWeapon);
	printf("appraise ok item=%u(known=%d) paid=%u equ=%u at=%u->%u\n", pA->waItem[0], pA->waItem[0] == wK, 1000 - pA->dwGold, pA->waEqu[0], wATUnknown, pA->wAT);
	printf("appraise cost=%u\n", dwCost);
	pA->dwGold = 1000;
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise again item=%u gold=%u\n", pA->waItem[0], pA->dwGold);
	Para.byNo = 1;
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise plain item=%u gold=%u\n", pA->waItem[1], pA->dwGold);
	pA->waItem[2] = wU;
	Para.byNo = 2; Para.byMaker = (BYTE)(nMaker + 1);
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise no maker item=%u\n", pA->waItem[2]);
	Para.byMaker = (BYTE)nMaker;
	pNpc->dwX = pA->dwX + 100;
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise far item=%u\n", pA->waItem[2]);
	pNpc->dwX = pA->dwX;
	pMaker->byKind = 0;
	TestRecv(pA, &Para, sizeof(Para));
	printf("appraise wrong kind item=%u\n", pA->waItem[2]);
	pMaker->byKind = MAKERKIND_APPRAISE;

	/* 後始末（作り手・NPC・持ち物） */
	g_aMakerInfo.RemoveAt(nMaker);
	delete pMaker;
	pNpc->pszNpcSpeech = pszSpeechBak;
	pNpc->dwMapID = dwNpcMapBak; pNpc->dwX = dwNpcXBak; pNpc->dwY = dwNpcYBak;
	memcpy(pA->waItem, waItemBak, sizeof(waItemBak));
	memcpy(pA->waEqu, waEquBak, sizeof(waEquBak));
	pA->dwGold = dwGoldBak; pA->dwMapID = dwMapBak; pA->dwX = dwXBak; pA->dwY = dwYBak;
	RenewATDF(pA->dwTblID);
	pA->wAT = wATBak;

	/* (d) 保存と読み直し */
	int nAffixN = g_aAffixInfo.GetSize(), nDeriveN = g_aItemDerive.GetSize();
	SaveAffixInfo();
	SaveItemDerive();
	for (int i = 0; i < g_aAffixInfo.GetSize(); i ++) delete g_aAffixInfo[i];
	for (int i = 0; i < g_aItemDerive.GetSize(); i ++) delete g_aItemDerive[i];
	g_aAffixInfo.RemoveAll();
	g_aItemDerive.RemoveAll();
	LoadAffixInfo();
	LoadItemDerive();
	printf("affix load affix=%d/%d derive=%d/%d\n", g_aAffixInfo.GetSize(), nAffixN, g_aItemDerive.GetSize(), nDeriveN);
	if (g_aAffixInfo.GetSize() == nAffixN && nAffixN >= 3) {
		PITEMAFFIXINFO p = g_aAffixInfo[0];
		printf("affix load row0 id=%u pos=%u name=%s mask=%u rate=%d add=%d rank=%u gold=%u\n", p->wID, p->byPos, p->szName, p->wEquMask, p->chRate, p->chAdd, p->byRank, p->wGoldRate);
	}
	if (g_aItemDerive.GetSize() >= 2) {
		printf("affix load derive0 base=%d pre=%u suf=%u unknown=%u / derive1 unknown=%u\n", g_aItemDerive[0]->wBase == wWeapon, g_aItemDerive[0]->wPre, g_aItemDerive[0]->wSuf, g_aItemDerive[0]->byUnknown, g_aItemDerive[1]->byUnknown);
	}
	printf("affix load value=%d\n", ItemValueOf(wK));

	/* (e) ドロップのくじ引き */
	sgenrand(777);
	int nPlain0 = 0, nPre = 0, nSuf = 0, nBoth = 0, nHigh = 0, nUnk = 0;
	for (int i = 0; i < 300; i ++) {
		WORD wRes = AffixRollDrop(wWeapon, 100, 1);
		PITEMDERIVEINFO pD = ItemDeriveGet(wRes);
		if (pD == NULL) { nPlain0 ++; continue; }
		nUnk += pD->byUnknown ? 1 : 0;
		if (pD->wPre && pD->wSuf) nBoth ++; else if (pD->wPre) nPre ++; else nSuf ++;
		if (pD->wPre == 3) nHigh ++;
	}
	printf("affix roll rank1 plain=%d pre=%d suf=%d both=%d rank2=%d unknown=%d\n", nPlain0, nPre, nSuf, nBoth, nHigh, nUnk);
	nHigh = 0;
	for (int i = 0; i < 300; i ++) {
		PITEMDERIVEINFO pD = ItemDeriveGet(AffixRollDrop(wWeapon, 100, 2));
		if (pD && pD->wPre == 3) nHigh ++;
	}
	printf("affix roll rank2 has=%d\n", nHigh > 0);
	printf("affix roll rate0=%d armorOnly=%d\n", AffixRollDrop(wWeapon, 0, 3) == wWeapon, 1);
	int nHit = 0;
	for (int i = 0; i < 1000; i ++) {
		if (AffixRollDrop(wWeapon, 10, 1) != wWeapon) nHit ++;
	}
	printf("affix roll rate10 hit=%d/1000\n", nHit);
	/* 付与表が空なら何も起きない */
	for (int i = 0; i < g_aAffixInfo.GetSize(); i ++) delete g_aAffixInfo[i];
	g_aAffixInfo.RemoveAll();
	printf("affix roll empty=%d\n", AffixRollDrop(wWeapon, 100, 3) == wWeapon);

	/* 元の表へ戻す */
	for (int i = 0; i < g_aItemDerive.GetSize(); i ++) delete g_aItemDerive[i];
	g_aItemDerive.RemoveAll();
	for (size_t i = 0; i < aAffixBak.size(); i ++) g_aAffixInfo.Add(aAffixBak[i]);
	for (size_t i = 0; i < aDeriveBak.size(); i ++) g_aItemDerive.Add(aDeriveBak[i]);
	SaveAffixInfo();
	SaveItemDerive();
}

/* ------------------------------------------------------------------------- */
/* AH2. 付与の段階2：鍛冶屋で鍛える・付け替える								 */
/*    (a) 鍛える＋1→＋2→＋3・これ以上は鍛えられない（wNext 0）				 */
/*    (b) 普通の装備は byAllowPlain=0 なら断る・1 なら＋1					 */
/*    (c) 付け替える（前の付与を置き換え・同じ付与は断る）					 */
/*    (d) 未鑑定・種別違いは断る												 */
/*    (e) 装備中のまま強さが変わる・材料と手数料の消費・対象は材料に数えない	 */
/*    (f) 材料不足・手数料不足は何も減らない									 */
/*    (g) レシピ・付与表（wNext）の保存と読み直し								 */
/* ------------------------------------------------------------------------- */
static void TestAffixForge(const std::vector<int> &aLogin)
{
	printf("== AH2. Forge ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[2]];

	std::vector<PITEMAFFIXINFO> aAffixBak;
	std::vector<PITEMDERIVEINFO> aDeriveBak;
	for (int i = 0; i < g_aAffixInfo.GetSize(); i ++) aAffixBak.push_back(g_aAffixInfo[i]);
	for (int i = 0; i < g_aItemDerive.GetSize(); i ++) aDeriveBak.push_back(g_aItemDerive[i]);
	g_aAffixInfo.RemoveAll();
	g_aItemDerive.RemoveAll();

	WORD wWeapon = 0, wArmor = 0, wMat1 = 0, wMat2 = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && i < ITEMID_DERIVE_BASE; i ++) {
		PITEMINFO p = g_aItemInfo[i];
		if (!p->szName[0] || IsImportantItem((WORD)i) || (p->byFlg & ITEMFLG_DROP)) continue;
		if (!wWeapon && (p->byType == ITEMTYPE_WEAPON) && (p->byData >= 10) && (p->dwGold >= 200)) wWeapon = (WORD)i;
		else if (!wArmor && (p->byType == ITEMTYPE_ARMOR) && (p->byData >= 10)) wArmor = (WORD)i;
		else if (ItemAffixEquBit(p->byType) == 0) {
			if (!wMat1) wMat1 = (WORD)i; else if (!wMat2 && (WORD)i != wMat1) wMat2 = (WORD)i;
		}
	}
	printf("forge found weapon=%d armor=%d mat=%d\n", wWeapon ? 1 : 0, wArmor ? 1 : 0, (wMat1 && wMat2) ? 1 : 0);
	if (!wWeapon || !wMat1 || !wMat2) {
		return;
	}

	/* 付与表：1 鋭い(前) 2 ＋1(後ろ→3) 3 ＋2(後ろ→4) 4 ＋3(後ろ。次なし) 5 堅い(前・鎧だけ) */
	struct { BYTE byPos; const char *pszName; WORD wMask; char chAdd; WORD wNext; } aDef[] = {
		{ AFFIXPOS_FRONT, "鋭い", AFFIXEQU_ALL, 2, 0 },
		{ AFFIXPOS_BACK, "＋1", AFFIXEQU_ALL, 1, 3 },
		{ AFFIXPOS_BACK, "＋2", AFFIXEQU_ALL, 2, 4 },
		{ AFFIXPOS_BACK, "＋3", AFFIXEQU_ALL, 3, 0 },
		{ AFFIXPOS_FRONT, "堅い", AFFIXEQU_ARMOR, 0, 0 },
	};
	for (int i = 0; i < 5; i ++) {
		PITEMAFFIXINFO p = new ITEMAFFIXINFO;
		ZeroMemory(p, sizeof(*p));
		p->wID = (WORD)(i + 1);
		p->byPos = aDef[i].byPos;
		strcpy(p->szName, aDef[i].pszName);
		p->wEquMask = aDef[i].wMask;
		p->chAdd = aDef[i].chAdd;
		p->byRank = 1;
		p->wWeight = 1;
		p->wGoldRate = 100;
		p->wNext = aDef[i].wNext;
		g_aAffixInfo.Add(p);
	}

	/* 作り手・NPC・レシピを仮に用意する */
	WORD waItemBak[MAX_MYITEM], waEquBak[MAX_EQUTYPE];
	DWORD dwGoldBak = pA->dwGold, dwMapBak = pA->dwMapID, dwXBak = pA->dwX, dwYBak = pA->dwY;
	WORD wATBak = pA->wAT;
	memcpy(waItemBak, pA->waItem, sizeof(waItemBak));
	memcpy(waEquBak, pA->waEqu, sizeof(waEquBak));
	PMAKERINFO pMaker = new MAKERINFO;
	ZeroMemory(pMaker, sizeof(*pMaker));
	pMaker->byState = RECIPESTATE_ON;
	int nMaker = g_aMakerInfo.GetSize();
	g_aMakerInfo.Add(pMaker);
	PCHARINFO pNpc = NULL;
	for (int i = 0; i < g_aCharInfo.GetSize() && !pNpc; i ++) {
		if (g_aCharInfo[i]->byNpcType == NPCTYPE_MOVERAND) pNpc = g_aCharInfo[i];
	}
	char *pszSpeechBak = pNpc->pszNpcSpeech;
	DWORD dwNpcMapBak = pNpc->dwMapID, dwNpcXBak = pNpc->dwX, dwNpcYBak = pNpc->dwY;
	char szSpeech[16];
	snprintf(szSpeech, sizeof(szSpeech), "@c%02d", nMaker);
	pNpc->pszNpcSpeech = szSpeech;
	pNpc->dwMapID = pA->dwMapID; pNpc->dwX = pA->dwX; pNpc->dwY = pA->dwY;

	std::vector<PRECIPEINFO> aRecipe;
	int nRecipeBase = g_aRecipeInfo.GetSize();
	struct { BYTE byKind; WORD wAffix; BYTE byAllowPlain; WORD wEquMask; WORD wMat; BYTE byMatCount; DWORD dwGold; } aRDef[] = {
		{ RECIPEKIND_FORGE, 2, 1, 0, wMat1, 1, 50 },			/* 0: 鍛える（＋1）。普通の装備も可 */
		{ RECIPEKIND_FORGE, 2, 0, 0, wMat1, 1, 0 },			/* 1: 鍛える（＋1）。派生装備だけ */
		{ RECIPEKIND_SWAP, 1, 0, 0, wMat2, 1, 0 },				/* 2: 付け替える：鋭い */
		{ RECIPEKIND_SWAP, 5, 1, 0, wMat2, 1, 0 },				/* 3: 付け替える：堅い（鎧だけ） */
		{ RECIPEKIND_FORGE, 2, 1, 0, wWeapon, 1, 0 },			/* 4: 鍛える。材料が対象と同じ品 */
		{ RECIPEKIND_FORGE, 2, 1, AFFIXEQU_ARMOR, wMat1, 1, 0 },/* 5: 鍛える。レシピ側で鎧だけ */
	};
	for (int i = 0; i < 6; i ++) {
		PRECIPEINFO p = new RECIPEINFO;
		ZeroMemory(p, sizeof(*p));
		p->byState = RECIPESTATE_ON;
		p->byMaker = (BYTE)nMaker;
		p->byCount = 1;
		p->byKind = aRDef[i].byKind;
		p->wAffix = aRDef[i].wAffix;
		p->byAllowPlain = aRDef[i].byAllowPlain;
		p->wEquMask = aRDef[i].wEquMask;
		p->awMatItem[0] = aRDef[i].wMat;
		p->abyMatCount[0] = aRDef[i].byMatCount;
		p->dwGold = aRDef[i].dwGold;
		g_aRecipeInfo.Add(p);
		aRecipe.push_back(p);
	}

	auto Reset = [&]() {
		ZeroMemory(pA->waItem, sizeof(pA->waItem));
		for (int i = 0; i < MAX_EQUTYPE; i ++) pA->waEqu[i] = 0xFFFF;
		pA->dwGold = 1000;
	};
	auto Forge = [&](int nRecipe, int nNo) {
		CMDPARA_RECIPEMAKE Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_RECIPEMAKE; Para.byMaker = (BYTE)nMaker; Para.wRecipeID = (WORD)(nRecipeBase + nRecipe);
		Para.byNo = (BYTE)nNo; Para.dwTblID = pA->dwTblID;
		TestRecv(pA, &Para, sizeof(Para));
	};
	auto Show = [&](const char *pszTag) {
		printf("forge %s items=", pszTag);
		for (int i = 0; i < 4; i ++) printf("%s%s", i ? "," : "", pA->waItem[i] ? ItemNameOf(pA->waItem[i]) : "-");
		printf(" gold=%u equ=%u\n", pA->dwGold, (unsigned)pA->waEqu[0]);
	};
	auto Code = [&](int nRecipe, WORD wItem) {
		WORD wB, wP, wS;
		return ItemForgeCalc(aRecipe[nRecipe], wItem, &wB, &wP, &wS);
	};

	/* (b) 普通の装備：byAllowPlain=0 のレシピは断る（一覧にも出ない）・1 なら＋1 */
	Reset();
	pA->waItem[0] = wWeapon; pA->waItem[1] = wMat1;
	printf("forge plain calc allow0=%d allow1=%d\n", Code(1, wWeapon), Code(0, wWeapon));
	Forge(1, 0);
	Show("plain allow0");
	Forge(0, 0);
	Show("plain allow1");

	/* (a) ＋1→＋2→＋3→これ以上は鍛えられない。材料は手前の枠・対象は装備中で、材料が消えて詰まっても装備したまま強さが変わる */
	Reset();
	pA->waItem[0] = wMat1; pA->waItem[1] = wWeapon; pA->waEqu[0] = 1;
	RenewATDF(pA->dwTblID);
	WORD wAT0 = pA->wAT;
	Forge(0, 1);
	Show("equipped +1");
	WORD wAT1 = pA->wAT;
	pA->waItem[1] = wMat1;
	Forge(0, 0);
	Show("+2");
	pA->waItem[1] = wMat1;
	Forge(0, 0);
	Show("+3");
	printf("forge at %u->%u->%u expect=%d\n", wAT0, wAT1, pA->wAT, (int)wAT0 + 3);
	pA->waItem[1] = wMat1;
	printf("forge maxed calc=%d\n", Code(0, pA->waItem[0]));
	Forge(0, 0);
	Show("maxed");

	/* (c) 付け替える：後ろの付与を残したまま前を置き換え・同じ付与は断る */
	Reset();
	pA->waItem[0] = AffixDeriveFindOrCreate(wWeapon, 0, 2, 0);
	pA->waItem[1] = wMat2; pA->waItem[2] = wMat2;
	Forge(2, 0);
	Show("swap front");
	printf("forge swap same calc=%d\n", Code(2, pA->waItem[0]));
	Forge(2, 0);
	Show("swap same");

	/* (d) 未鑑定・種別違い・近くに作り手がいない */
	Reset();
	WORD wUnk = AffixDeriveFindOrCreate(wWeapon, 0, 2, 1);
	pA->waItem[0] = wUnk; pA->waItem[1] = wMat2;
	printf("forge unknown calc=%d\n", Code(2, wUnk));
	Forge(2, 0);
	printf("forge unknown item=%u(unknown=%d) mat=%u gold=%u\n", pA->waItem[0], pA->waItem[0] == wUnk, pA->waItem[1], pA->dwGold);
	pA->waItem[0] = AffixDeriveFindOrCreate(wWeapon, 0, 2, 0);
	printf("forge armor-only calc affix=%d recipeMask=%d\n", Code(3, pA->waItem[0]), Code(5, pA->waItem[0]));
	Forge(3, 0);
	Show("armor-only");
	pA->waItem[2] = wMat2;
	Forge(2, 5);
	printf("forge empty slot item=%u\n", pA->waItem[5]);
	pNpc->dwX = pA->dwX + 100;
	pA->waItem[3] = wMat2;
	Forge(2, 0);
	Show("far");
	pNpc->dwX = pA->dwX;
	if (wArmor) {
		pA->waItem[0] = wArmor;
		printf("forge armor calc forge=%d swapPlainOK=%d recipeMask=%d\n", Code(0, wArmor), Code(3, wArmor), Code(5, wArmor));
	}

	/* (f) 材料不足・手数料不足は何も減らない */
	Reset();
	pA->waItem[0] = wWeapon;
	Forge(0, 0);
	Show("lack mat");
	pA->waItem[1] = wMat1;
	pA->dwGold = 49;
	Forge(0, 0);
	Show("lack gold");
	/* (e) 材料が対象と同じ品でも、対象そのものは材料に数えない */
	Reset();
	pA->waItem[0] = wWeapon;
	Forge(4, 0);
	Show("self material");
	pA->waItem[1] = wWeapon;
	Forge(4, 0);
	Show("self material 2");
	/* 手数料と材料の消費 */
	Reset();
	pA->waItem[0] = wWeapon; pA->waItem[1] = wMat1; pA->waItem[2] = wMat1;
	Forge(0, 0);
	Show("consume");

	/* (g) レシピ・付与表の保存と読み直し */
	pA->wAT = wATBak;
	g_aMakerInfo.RemoveAt(nMaker);
	delete pMaker;
	pNpc->pszNpcSpeech = pszSpeechBak;
	pNpc->dwMapID = dwNpcMapBak; pNpc->dwX = dwNpcXBak; pNpc->dwY = dwNpcYBak;
	memcpy(pA->waItem, waItemBak, sizeof(waItemBak));
	memcpy(pA->waEqu, waEquBak, sizeof(waEquBak));
	pA->dwGold = dwGoldBak; pA->dwMapID = dwMapBak; pA->dwX = dwXBak; pA->dwY = dwYBak;
	RenewATDF(pA->dwTblID);
	pA->wAT = wATBak;

	std::vector<PRECIPEINFO> aRecipeBak;
	for (int i = 0; i < nRecipeBase; i ++) aRecipeBak.push_back(g_aRecipeInfo[i]);
	SaveAffixInfo();
	SaveRecipeInfo();
	g_aAffixInfo.RemoveAll();
	g_aRecipeInfo.RemoveAll();
	LoadAffixInfo();
	LoadRecipeInfo();
	printf("forge load affix=%d recipes=%d\n", g_aAffixInfo.GetSize(), g_aRecipeInfo.GetSize());
	if (g_aAffixInfo.GetSize() == 5) {
		printf("forge load next=%u,%u,%u,%u\n", g_aAffixInfo[1]->wNext, g_aAffixInfo[2]->wNext, g_aAffixInfo[3]->wNext, g_aAffixInfo[0]->wNext);
	}
	if (g_aRecipeInfo.GetSize() == nRecipeBase + 6) {
		for (int i = 0; i < 6; i ++) {
			PRECIPEINFO p = g_aRecipeInfo[nRecipeBase + i];
			printf("forge load recipe%d kind=%u affix=%u plain=%u mask=%u mat=%d gold=%u\n", i, p->byKind, p->wAffix, p->byAllowPlain, p->wEquMask, p->awMatItem[0] == aRDef[i].wMat, p->dwGold);
		}
	}

	/* 元の表へ戻す */
	for (int i = nRecipeBase; i < g_aRecipeInfo.GetSize(); i ++) delete g_aRecipeInfo[i];
	g_aRecipeInfo.RemoveAll();
	for (size_t i = 0; i < aRecipeBak.size(); i ++) g_aRecipeInfo.Add(aRecipeBak[i]);
	SaveRecipeInfo();
	for (int i = 0; i < g_aAffixInfo.GetSize(); i ++) delete g_aAffixInfo[i];
	g_aAffixInfo.RemoveAll();
	for (int i = 0; i < g_aItemDerive.GetSize(); i ++) delete g_aItemDerive[i];
	g_aItemDerive.RemoveAll();
	for (size_t i = 0; i < aAffixBak.size(); i ++) g_aAffixInfo.Add(aAffixBak[i]);
	for (size_t i = 0; i < aDeriveBak.size(); i ++) g_aItemDerive.Add(aDeriveBak[i]);
	SaveAffixInfo();
	SaveItemDerive();
}

/* ------------------------------------------------------------------------- */
/* AN. 発言の長さ・連投、コマンドの連打、取り引きの割り込み					 */
/*     公式のクライアントでは起きない送り方だけを止め、ふつうの発言は通すか	 */
/* ------------------------------------------------------------------------- */
static void SpamSpeech(PCHARINFO p, const char *psz, BYTE byType)
{
	std::vector<BYTE> a(sizeof(CMDPARA_SPEECH) + strlen(psz) + 1);
	PCMDPARA_SPEECH pPara = (PCMDPARA_SPEECH)&a[0];
	ZeroMemory(pPara, sizeof(CMDPARA_SPEECH));
	pPara->byCmd = CMD_SPEECH;
	pPara->pID = p->pID;
	pPara->dwTblID = p->dwTblID;
	pPara->wSize = (WORD)(strlen(psz) + 1);
	pPara->byType = byType;
	memcpy(&a[sizeof(CMDPARA_SPEECH)], psz, strlen(psz) + 1);
	TestRecv(p, &a[0], (DWORD)a.size());
}

static void TestSpamLimit(const std::vector<int> &aLogin)
{
	printf("== AN. SpamLimit ==\n");
	if (aLogin.size() < 3) {
		return;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	BYTE byAdminBack = pA->byAdminLV;
	DWORD dwNowBack = s_dwFakeNow;
	pA->byAdminLV = ADMINLV_USER;

	/* 長さ：ふつうの発言は 256 バイトまで・URL を含む発言は長いまま・自己紹介は 600 バイトまで */
	{
		std::string sLong(1000, 'a'), sUrl = "https://example.com/" + std::string(1000, 'b'), sEdit(2000, 'c');
		SpamSpeech(pA, "hello", SPEECHCMD_AROUND);
		printf("speech normal len=%d\n", (int)strlen(pA->pszSpeech));
		SpamSpeech(pA, sLong.c_str(), SPEECHCMD_AROUND);
		printf("speech long len=%d\n", (int)strlen(pA->pszSpeech));
		SpamSpeech(pA, sUrl.c_str(), SPEECHCMD_AROUND);
		printf("speech url len=%d\n", (int)strlen(pA->pszSpeech));
		SpamSpeech(pA, sEdit.c_str(), SPEECHCMD_SPEECHEDIT);
		printf("speech edit len=%d\n", (int)strlen(pA->pszNpcSpeech));
	}

	/* 連投：5 秒に 8 回まで。1.2 秒おきの自動分割は通る */
	s_bTestRateReset = FALSE;
	{
		int nOK = 0;
		ZeroMemory(pA->adwRateLast, sizeof(pA->adwRateLast));
		pA->bySpeechWinCount = 0;
		for (int i = 0; i < 12; i ++) {
			pA->byFlg &= ~CHARINFOFLG_SPEECH;
			SpamSpeech(pA, "x", SPEECHCMD_AROUND);
			if (pA->byFlg & CHARINFOFLG_SPEECH) nOK ++;
		}
		printf("speech burst accepted=%d of 12\n", nOK);
		s_dwFakeNow += 5001;
		pA->byFlg &= ~CHARINFOFLG_SPEECH;
		SpamSpeech(pA, "x", SPEECHCMD_AROUND);
		printf("speech after window accepted=%d\n", (pA->byFlg & CHARINFOFLG_SPEECH) ? 1 : 0);
		nOK = 0;
		for (int i = 0; i < 20; i ++) {
			s_dwFakeNow += 1200;
			pA->byFlg &= ~CHARINFOFLG_SPEECH;
			SpamSpeech(pA, "x", SPEECHCMD_AROUND);
			if (pA->byFlg & CHARINFOFLG_SPEECH) nOK ++;
		}
		printf("speech every 1.2s accepted=%d of 20\n", nOK);
	}

	/* 最小間隔：吹き出し 0.5 秒・管理人呼び出し 30 秒 */
	{
		int nOK = 0;
		ZeroMemory(pA->adwRateLast, sizeof(pA->adwRateLast));
		for (int i = 0; i < 4; i ++) {
			CMDPARA_DATA1 Para;
			ZeroMemory(&Para, sizeof(Para));
			Para.byCmd = CMD_COMEFC;
			Para.pID = pA->pID;
			Para.dwTblID = pA->dwTblID;
			Para.byData = 1;
			pA->byFlg &= ~CHARINFOFLG_COMEFC;
			TestRecv(pA, &Para, sizeof(Para));
			if (pA->byFlg & CHARINFOFLG_COMEFC) nOK ++;
			s_dwFakeNow += (i == 1) ? 600 : 100;
		}
		printf("comefc accepted=%d of 4\n", nOK);

		DWORD adwLast[3];
		for (int i = 0; i < 3; i ++) {
			CMDPARA_DATA1 Para;
			ZeroMemory(&Para, sizeof(Para));
			Para.byCmd = CMD_GMCALL;
			Para.pID = pA->pID;
			Para.dwTblID = pA->dwTblID;
			TestRecv(pA, &Para, sizeof(Para));
			adwLast[i] = pA->adwRateLast[RATE_GMCALL];
			s_dwFakeNow += (i == 0) ? 1000 : 30000;
		}
		printf("gmcall first_kept=%d second_renewed=%d\n", adwLast[1] == adwLast[0], adwLast[2] != adwLast[1]);
	}
	s_bTestRateReset = TRUE;

	/* 取り引き：相手が別の人と取り引き中なら割り込めない。自分が別の人と取り引き中なら元の相手に終了を知らせる */
	{
		DWORD adwBack[3][3];
		PCHARINFO apTmp[3] = { pA, pB, pC };
		for (int i = 0; i < 3; i ++) {
			adwBack[i][0] = apTmp[i]->dwMapID; adwBack[i][1] = apTmp[i]->dwX; adwBack[i][2] = apTmp[i]->dwY;
		}
		for (int i = 1; i < 3; i ++) {
			DeleteTblNoPC((int)apTmp[i]->dwTblID);
			apTmp[i]->dwMapID = pA->dwMapID;
			apTmp[i]->dwX = apTmp[i]->dwXBack = pA->dwX;
			apTmp[i]->dwY = apTmp[i]->dwYBack = pA->dwY;
			AddTblNoPC((int)apTmp[i]->dwTblID);
		}
		RecvCmdTradeClear(pA->dwTblID);
		RecvCmdTradeClear(pB->dwTblID);
		RecvCmdTradeClear(pC->dwTblID);
		/* 申し込める相手はログイン中のプレイヤーだけ（このテストのキャラは名前だけ空なので仮に付ける） */
		char cAccBack = pB->szAcc[0];
		pB->szAcc[0] = 'b';

		CMDPARA_DATA3 Para;
		ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_TRADE;
		Para.pID = pA->pID;
		Para.dwTblID = pA->dwTblID;
		Para.dwData = pB->dwTblID;
		TestRecv(pA, &Para, sizeof(Para));
		printf("trade a->b trading a=%d b=%d\n", RecvCmdIsTrading(pA->dwTblID) ? 1 : 0, RecvCmdIsTrading(pB->dwTblID) ? 1 : 0);

		Para.pID = pC->pID;
		Para.dwTblID = pC->dwTblID;
		Para.dwData = pB->dwTblID;
		int nOut0 = MockCmdCount(1000 + 2, CMD_TRADE);
		TestRecv(pC, &Para, sizeof(Para));
		printf("trade c->b (b busy) c_trading=%d a_still=%d c_got_out=%d\n", RecvCmdIsTrading(pC->dwTblID) ? 1 : 0,
				RecvCmdIsTrading(pA->dwTblID) ? 1 : 0, MockCmdCount(1000 + 2, CMD_TRADE) - nOut0);

		pB->szAcc[0] = cAccBack;
		for (int i = 0; i < 3; i ++) {
			DeleteTblNoPC((int)apTmp[i]->dwTblID);
			apTmp[i]->dwMapID = adwBack[i][0];
			apTmp[i]->dwX = apTmp[i]->dwXBack = adwBack[i][1];
			apTmp[i]->dwY = apTmp[i]->dwYBack = adwBack[i][2];
			AddTblNoPC((int)apTmp[i]->dwTblID);
		}
		RecvCmdTradeClear(pA->dwTblID);
		RecvCmdTradeClear(pB->dwTblID);
		RecvCmdTradeClear(pC->dwTblID);
	}

	pA->byAdminLV = byAdminBack;
	s_dwFakeNow = dwNowBack;
}

/* ------------------------------------------------------------------------- */
/* AN. ダンジョンのテーマ生成												 */
/*     テーマ番号 0・見つからない番号は今までの作り方（作成中の設計図が付く）	 */
/*     テーマがあれば、範囲に絵が入り・入口と出口のワープ・扉イベントが置かれ・	 */
/*     範囲内の古いイベントは消え・作り直し番号が進み・保存の列が増える		 */
/* ------------------------------------------------------------------------- */
static void TestDungeonTheme(void)
{
	printf("== AN. Dungeon theme ==\n");
	DungeonThemeChecks checks = CheckDungeonTheme();
	printf("dungeontheme deco global=%d perRoom=%d limits=%d\n", checks.global, checks.perRoom, checks.failures == 0);
	printf("dungeontheme plain bottom=1,2 walls=%d faces=%d verticalDoors=%d exposed=%d non-face=%d errors=%d\n", checks.walls, checks.faces, checks.verticalDoors, checks.exposedWalls, checks.nonFaceWalls, checks.failures);
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	if (nMapNo < 0) {
		printf("dungeontheme no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	/* マップの中身を控える（終わったら戻す）。イベントは空にして始める */
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	BYTE byPLCBack = pMap->byPartsLightCount;
	MAPPARTSLIGHT aPLBack[MAX_MAPPARTSLIGHT];
	memcpy(aPLBack, pMap->aPartsLight, sizeof(aPLBack));

	/* テーマ（床 1・2、天井 3、側面 4、その他の壁 5、出入口 6、扉 7→開いた 8、入口の絵 9、ゴールの絵 10。扉を多めに） */
	{
		FILE *fp = fopen("SBODATA/SboWebDungeonThemes.json", "wb");
		if (fp) {
			const char *psz = "{\"seq\":5,\"themes\":[{\"id\":5,\"name\":\"test\",\"algo\":1,\"floor\":[1,2],\"wallTop\":[3],\"wallFace\":[4],\"wallFill\":[5],"
				"\"void\":0,\"arch\":[6],\"door\":7,\"doorOpen\":8,\"entry\":9,\"goal\":10,\"bottomRows\":1,\"rooms\":[],\"decos\":[],\"lights\":[{\"wParts\":3,\"byLight\":4},{\"wParts\":1,\"byLight\":7},{\"wParts\":2,\"byLight\":9}],"
				"\"params\":{\"doorRate\":100,\"openRate\":0,\"floorMin\":30}},"
				"{\"id\":6,\"name\":\"nodoor\",\"algo\":1,\"floor\":[1],\"wallTop\":[3],\"wallFace\":[4],\"wallFill\":[5],\"arch\":[6],\"door\":7,\"goal\":10,\"rooms\":[],\"decos\":[]}]}";
			fwrite(psz, 1, strlen(psz), fp);
			fclose(fp);
		}
	}

	SUBGAME_DUNGEONINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 12; Info.byY1 = 10; Info.byX2 = 51; Info.byY2 = 49;		/* 40×40 */
	Info.byStartX = 3; Info.byStartY = 4;
	Info.wChipIDStart = 21; Info.wChipIDGoal = 22; Info.wChipIDRoad = 23; Info.wChipIDBlock = 24;
	Info.dwTblID = (DWORD)-1;
	SUBGAMEINFO Sub;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);
	/* 古いイベント: 範囲の中（消える）・外（残る） */
	PMAPINFO pm = pMap;
	for (int k = 0; k < 2; k ++) {
		int nNo = AddEvent(pm->dwID);
		pm->pEvent[nNo].dwX = (DWORD)(k == 0 ? 20 : 60);
		pm->pEvent[nNo].dwY = (DWORD)(k == 0 ? 20 : 55);
		pm->pEvent[nNo].byType = EVENTTYPE_STATUSINIT;
		pm->pEvent[nNo].pData = NULL;
	}

	/* 1. 見つからない番号 → 今までの作り方（設計図が付き、イベントは触らない） */
	Info.dwThemeID = 99;
	MakeDungeonProc(byID);
	printf("dungeontheme missing old=%d buildNo=%lu events=%d\n", (int)(Info.pMapTmp != NULL), (unsigned long)Info.dwBuildNo, pMap->nEventCount);
	SAFE_DELETE_ARRAY(Info.pMapTmp);

	/* 2. テーマ 5 */
	Info.dwThemeID = 5;
	MakeDungeonProc(byID);
	int nTop = 0, nMapChg = 0, nDoor = 0, nDoorOk = 0, nOld = 0, nKeep = 0, nOutside = 0, nBadParts = 0;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pE = &pMap->pEvent[i];
		bool bIn = pE->dwX >= 12 && pE->dwX <= 51 && pE->dwY >= 10 && pE->dwY <= 49;
		if (pE->byType == EVENTTYPE_STATUSINIT) {
			if (pE->dwX == 20) nOld ++;
			if (pE->dwX == 60) nKeep ++;
		} else if (pE->byType == EVENTTYPE_MAPCHG) {
			nMapChg ++;
			PEVENTINFO_MAPCHG pC = (PEVENTINFO_MAPCHG)pE->pData;
			if (pE->dwX == 3 && pE->dwY == 4) {
				/* 外の入口 → 中の入口（生成の start = byGoalX/Y）。エリアの中 */
				if (pC->dwX == Info.byGoalX && pC->dwY == Info.byGoalY && pC->dwX >= 12 && pC->dwX <= 51 && pC->dwY >= 10 && pC->dwY <= 49) nTop ++;
			} else if (bIn) {
				/* 出口（生成の goal）→ 外の入口 */
				if (pC->dwX == 3 && pC->dwY == 4 && pMap->pwMap[pE->dwX + pE->dwY * pMap->nCX] == 10) nTop += 10;
			}
		} else if (pE->byType == EVENTTYPE_DOOR) {
			nDoor ++;
			PEVENTINFO_DOOR pD = (PEVENTINFO_DOOR)pE->pData;
			if (bIn && pD->wGrpID == 7 && pD->wBackGrpID == 8 && pMap->pwMap[pE->dwX + pE->dwY * pMap->nCX] == 7) nDoorOk ++;
		}
		if (!bIn && pE->byType != EVENTTYPE_STATUSINIT && !(pE->dwX == 3 && pE->dwY == 4)) nOutside ++;
	}
	/* 範囲のマスは、テーマの絵（1〜10）だけ。外は触らない */
	for (int y = 10; y <= 49; y ++) {
		for (int x = 12; x <= 51; x ++) {
			WORD w = pMap->pwMap[x + y * pMap->nCX];
			if (w < 1 || w > 10) nBadParts ++;
		}
	}
	int nOutChg = 0;
	for (int y = 0; y < pMap->nCY; y ++) {
		for (int x = 0; x < pMap->nCX; x ++) {
			bool bIn = x >= 12 && x <= 51 && y >= 10 && y <= 49;
			if (!bIn && pMap->pwMap[x + y * pMap->nCX] != aMapBack[x + y * pMap->nCX]) nOutChg ++;
		}
	}
	printf("dungeontheme theme5 old=%d buildNo=%lu entry=%d mapchg=%d doors>0=%d doorsOk=%d oldEvent=%d keepEvent=%d eventOutside=%d badParts=%d outChanged=%d(entrance=%d) goal=%d genSize=%dx%d\n",
		(int)(Info.pMapTmp != NULL), (unsigned long)Info.dwBuildNo, (int)(nTop % 10 == 1), nMapChg, (int)(nDoor > 0), (int)(nDoor == nDoorOk), nOld, nKeep, nOutside, nBadParts,
		nOutChg, (int)(pMap->pwMap[3 + 4 * pMap->nCX] == 21), (int)(nTop >= 10), (int)Info.wGenCX, (int)Info.wGenCY);

	/* 灯りの上書き表: テーマ 5 の lights（パーツ 3→4、1→7。byLight 9 の行は読み込みで捨てる）が入る */
	printf("dungeontheme lights count=%d p0=%d/%d p1=%d/%d\n", (int)pMap->byPartsLightCount, (int)pMap->aPartsLight[0].wParts, (int)pMap->aPartsLight[0].byLight,
		(int)pMap->aPartsLight[1].wParts, (int)pMap->aPartsLight[1].byLight);

	/* 3. 作り直すと番号が進み、古い扉は消えて置き直し（扉の数は前後で 0 でない・events が積み上がらない） */
	int nEvents1 = pMap->nEventCount;
	MakeDungeonProc(byID);
	int nDoor2 = 0;
	for (int i = 0; i < pMap->nEventCount; i ++) if (pMap->pEvent[i].byType == EVENTTYPE_DOOR) nDoor2 ++;
	printf("dungeontheme rebuild buildNo=%lu doors>0=%d eventsBounded=%d keepEvent=%d\n", (unsigned long)Info.dwBuildNo, (int)(nDoor2 > 0),
		(int)(pMap->nEventCount < nEvents1 + 400 && pMap->nEventCount > 2), nKeep);

	/* 3b. 探索マップ用の情報（CMD_DUNGEONMAP）：作り直した直後の内容が、送るままの形で持たれ、
	   部屋・出入口・グループが範囲の中に収まり、type 3 でつながった部屋は同じグループ番号になる */
	{
		int nValid = 0, nBuildOk = 0, nRooms = 0, nLinks = 0, nRoomIn = 0, nLinkIn = 0, nGroupOk = 0, nSizeOk = 0, nType3Same = 0, nType3 = 0;
		if (Info.pbyAutoMap && Info.dwAutoMapSize >= sizeof(CMDPARA_DUNGEONMAP)) {
			PCMDPARA_DUNGEONMAP pDM = (PCMDPARA_DUNGEONMAP)Info.pbyAutoMap;
			nValid = (pDM->byCmd == CMD_DUNGEONMAP && pDM->byValid == 1 && pDM->dwMapID == pMap->dwID);
			nBuildOk = (pDM->dwBuildNo == Info.dwBuildNo && pDM->wX1 == 12 && pDM->wY1 == 10 && pDM->wX2 == 51 && pDM->wY2 == 49);
			nRooms = pDM->wRoomCount;
			nLinks = pDM->wLinkCount;
			printf("dungeontheme automap name=%s\n", pDM->szName);
			nSizeOk = (Info.dwAutoMapSize == sizeof(CMDPARA_DUNGEONMAP) + nRooms * sizeof(DUNGEONMAPROOM) + nLinks * sizeof(DUNGEONMAPLINK));
			PDUNGEONMAPROOM pR = (PDUNGEONMAPROOM)(pDM + 1);
			PDUNGEONMAPLINK pL = (PDUNGEONMAPLINK)(pR + nRooms);
			nRoomIn = nLinkIn = nGroupOk = 1;
			for (int i = 0; i < nRooms; i ++) {
				if (pR[i].wX < 12 || pR[i].wY < 10 || pR[i].wX + pR[i].wW - 1 > 51 || pR[i].wY + pR[i].wH - 1 > 49) nRoomIn = 0;
				if (pR[i].wGroup > i) nGroupOk = 0;
			}
			for (int i = 0; i < nLinks; i ++) {
				if (pL[i].wX < 12 || pL[i].wY < 10 || pL[i].wX + pL[i].wW - 1 > 51 || pL[i].wY + pL[i].wH - 1 > 49 || pL[i].wRoomA >= nRooms || pL[i].wRoomB >= nRooms) nLinkIn = 0;
				if (pL[i].byType == DUNGEONMAPLINK_OPEN) {
					nType3 ++;
					if (pR[pL[i].wRoomA].wGroup == pR[pL[i].wRoomB].wGroup) nType3Same ++;
				}
			}
		}
		printf("dungeontheme automap valid=%d build=%d rooms>1=%d links>0=%d roomsIn=%d linksIn=%d group=%d size=%d type3same=%d\n",
			nValid, nBuildOk, (int)(nRooms > 1), (int)(nLinks > 0), nRoomIn, nLinkIn, nGroupOk, nSizeOk, (int)(nType3Same == nType3));
		/* マップ情報を送る時（SendCmdMAPINFO）に、直後に同じ内容が届く。ダンジョンの無いマップには何も送らない */
		std::vector<BYTE> aCap;
		SvrSendCaptureBegin();
		SendCmdDUNGEONMAP(7777, pMap->dwID);
		BOOL bSent = SvrSendCaptureEnd(aCap);
		printf("dungeontheme automap send=%d same=%d\n", (int)bSent, (int)(bSent && aCap.size() == Info.dwAutoMapSize && memcmp(&aCap[0], Info.pbyAutoMap, aCap.size()) == 0));
		int nMax = 0;
		for (int i = 0; i < g_aMapInfo.GetSize(); i ++) if (g_aMapInfo[i] && (int)g_aMapInfo[i]->dwID > nMax) nMax = (int)g_aMapInfo[i]->dwID;
		int nCnt0 = MockCmdCount(7778, CMD_DUNGEONMAP);
		SendCmdDUNGEONMAP(7778, (DWORD)(nMax + 1));
		printf("dungeontheme automap other map sent=%d\n", MockCmdCount(7778, CMD_DUNGEONMAP) - nCnt0);
		/* テーマ番号を外すと「使わない」を送る */
		DWORD dwThemeBack = Info.dwThemeID;
		Info.dwThemeID = 0;
		SvrSendCaptureBegin();
		SendCmdDUNGEONMAP(7777, pMap->dwID);
		bSent = SvrSendCaptureEnd(aCap);
		printf("dungeontheme automap off send=%d valid=%d\n", (int)bSent, (int)(bSent && aCap.size() == sizeof(CMDPARA_DUNGEONMAP) && ((PCMDPARA_DUNGEONMAP)&aCap[0])->byValid == 0 && aCap[0] == CMD_DUNGEONMAP));
		Info.dwThemeID = dwThemeBack;
	}

	/* 4. doorOpen の無いテーマ 6 は扉イベントを置かない（lights が空なので灯りの表は触らない） */
	Info.dwThemeID = 6;
	MakeDungeonProc(byID);
	printf("dungeontheme theme6 lights untouched=%d\n", (int)(pMap->byPartsLightCount == 2));
	int nDoor3 = 0;
	for (int i = 0; i < pMap->nEventCount; i ++) if (pMap->pEvent[i].byType == EVENTTYPE_DOOR) nDoor3 ++;
	printf("dungeontheme theme6 doors=%d buildNo=%lu goalPart=%d\n", nDoor3, (unsigned long)Info.dwBuildNo,
		(int)(pMap->pwMap[(int)Info.wGenGoalX + 12 + ((int)Info.wGenGoalY + 10) * pMap->nCX] == 10));

	/* 5. 範囲が狭い（8 未満）なら今までの作り方 */
	Info.dwThemeID = 5;
	Info.byX2 = Info.byX1 + 5;
	DWORD dwBuild = Info.dwBuildNo;
	MakeDungeonProc(byID);
	printf("dungeontheme narrow buildNo+=%lu mapTmp=%d\n", (unsigned long)(Info.dwBuildNo - dwBuild), (int)(Info.pMapTmp != NULL));
	SAFE_DELETE_ARRAY(Info.pMapTmp);
	printf("dungeontheme narrow automap dropped=%d\n", (int)(Info.pbyAutoMap == NULL && Info.dwAutoMapSize == 0));

	/* 6. 保存の列（テーマ番号 0 の時は列を増やさない）→ 読み直しで戻る */
	Info.byX2 = 51;
	{
		struct stat st;
		long nSize[2] = { 0, 0 };
		DWORD dwTheme[2] = { 0, 0 };
		int nOk[2] = { 0, 0 };
		const DWORD adwSave[2] = { 0, 7 };
		for (int k = 0; k < 2; k ++) {
			Info.dwThemeID = adwSave[k];
			SaveSubGameInfo();
			if (stat("SBODATA/SboDataSubGameInfo.dat", &st) == 0) nSize[k] = (long)st.st_size;
			/* 読み込みは空の表に入れる作りなので、今の表を退避して空にしてから読む（最後が今のダンジョン） */
			std::vector<PSUBGAMEINFO> aOrig;
			for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) aOrig.push_back(g_aSubaGameInfo[i]);
			int nBase = (int)aOrig.size();
			g_aSubaGameInfo.RemoveAll();
			LoadSubGameInfo();
			PSUBGAMEINFO pLoad = g_aSubaGameInfo.GetSize() ? g_aSubaGameInfo[g_aSubaGameInfo.GetSize() - 1] : NULL;
			if (g_aSubaGameInfo.GetSize() == nBase && pLoad && pLoad->byType == SUBGAMEID_DUNGEON && pLoad->pInfo) {
				PSUBGAME_DUNGEONINFO pL = (PSUBGAME_DUNGEONINFO)pLoad->pInfo;
				dwTheme[k] = pL->dwThemeID;
				nOk[k] = (int)(pL->byX1 == 12 && pL->byY2 == 49 && pL->wChipIDBlock == 24 && pL->byStartY == 4);
			}
			/* 読んだ写しを片づけて、元の表に戻す */
			for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) {
				PSUBGAMEINFO pDel = g_aSubaGameInfo[i];
				SAFE_DELETE_ARRAY(pDel->pInfo);
				delete pDel;
			}
			g_aSubaGameInfo.RemoveAll();
			for (int i = 0; i < nBase; i ++) g_aSubaGameInfo.Add(aOrig[i]);
		}
		printf("dungeontheme save theme0=%lu theme7=%lu fields=%d%d extraBytes=%ld\n", (unsigned long)dwTheme[0], (unsigned long)dwTheme[1], nOk[0], nOk[1], nSize[1] - nSize[0]);
	}

	/* 後始末 */
	SAFE_DELETE_ARRAY(Info.pwGenParts);
	SAFE_DELETE_ARRAY(Info.pbyAutoMap);
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
	pMap->byPartsLightCount = byPLCBack;
	memcpy(pMap->aPartsLight, aPLBack, sizeof(aPLBack));
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID);
	unlink("SBODATA/SboWebDungeonThemes.json");
}

/* ------------------------------------------------------------------------- */
/* AO. 番外編ダンジョン（段階1）											 */
/*     ソロもパーティも選択後に受付済みになり、入口でそろって入る・締め切りで	 */
/*     選ばなかった人は置いていく・組ごとに別の空間と敵・補充しない印・		 */
/*     中で拾った物は持ち帰らない（ゴールでも）・作り直しで全員出る・保存	 */
/* ------------------------------------------------------------------------- */
static int DgTestCountEnemy(DWORD dwParent, DWORD dwSpace)
{
	int n = 0;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->dwParentTblID == dwParent && p->dwSpaceID == dwSpace && p->byNpcType != NPCTYPE_PUT && p->byNpcType != NPCTYPE_INIT) n ++;
	}
	return n;
}

static void DgTestSelPick(PCHARINFO p, BYTE byIndex)
{
	CMDPARA_DUNGEONSEL Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_DUNGEONSEL;
	Para.byType = DUNGEONSEL_PICK;
	Para.byIndex = byIndex;
	Para.dwTblID = p->dwTblID;
	TestRecv(p, &Para, DungeonSelectionCommandSize());
}

/* 開始マスを踏んだ時の「抜けますか？」への返事（bYes=はい・いいえ） */
static void DgTestLeaveAnswer(PCHARINFO p, BOOL bYes)
{
	CMDPARA_DUNGEONSEL Para;
	DungeonSelectionCommand(Para, p->dwTblID, bYes ? DUNGEONSEL_LEAVEYES : DUNGEONSEL_LEAVENO, 0);
	TestRecv(p, &Para, DungeonSelectionCommandSize());
}

#include "client-automap-test.inc"
EM_JS(char *, MockMapBody, (int conn, int cmd, int valid), {
    return stringToNewUTF8(Module['sboNet'].lastBody(conn, cmd, valid));
});
EM_JS(char *, MockMapCells, (int conn, int cmd, int offset), {
    return stringToNewUTF8(Module['sboNet'].lastBody(conn, cmd, 2, offset));
});

/* 通信の検査先と、旧クライアントの続行通知。通常の受付では BEGIN を送らない。 */
static int DgTestConn(PCHARINFO p)
{
    for (size_t i = 0; i < s_pSpLogin->size(); i ++)
        if (g_aCharInfo[(*s_pSpLogin)[i]] == p) return 1000 + (int)i;
    return -1;
}

static void DgTestLegacyBegin(PCHARINFO p)
{
    CMDPARA_DUNGEONSEL para;
    DungeonSelectionCommand(para, p->dwTblID, DUNGEONSEL_BEGIN, 0);
    TestRecv(p, &para, DungeonSelectionCommandSize());
}
static void DgTestTalk(PCHARINFO p, DWORD dwNpc)
{
    MockTrace(1);
    QuestSendTalk(p, dwNpc);
    MockTrace(0);
}

/* 実際の移動と同じく外の入口へ移ってからイベントを踏む。作り直し後も検索し直す。 */
static void DgTestEnter(PCHARINFO p)
{
    int mapNo = GetMapNo(p->dwMapID);
    PMAPINFO map = g_aMapInfo[mapNo];
    for (int i = 0; i < map->nEventCount; i ++) {
        PEVENTINFO e = &map->pEvent[i];
        if (e->byType == EVENTTYPE_MAPCHG && e->dwX == 3 && e->dwY == 4) {
            QuestMovePC(p, mapNo, 3, 4);
            DungeonEventStep(p, e);
            return;
        }
    }
}

/* 届いた文の本文を調べる。 */
static BOOL DgTestText(PCHARINFO p, BYTE cmd, const char *text)
{
    std::vector<BYTE> body;
    if (!MarketLast(DgTestConn(p), cmd, body)) return FALSE;
    size_t offset = cmd == CMD_TALKMSG ? sizeof(CMDPARA_TALKMSG) : sizeof(CMDPARA_SPEECH);
    if (body.size() <= offset) return FALSE;
    std::string msg((const char *)&body[offset], body.size() - offset);
    return msg.find(text) != std::string::npos;
}

/* 受付の会話は実際の受信経路（距離・空間・可視性の確認込み）で試す */
static DWORD DgTestReception(DWORD dwMapID, int nID)
{
	PCHARINFO pNpc = new CHARINFO;
	ZeroMemory(pNpc, sizeof(*pNpc));
	pNpc->byNpcType = NPCTYPE_NOMOVE;
	SetInitCharData(pNpc);
	pNpc->byGameState = GAMESTATE_NPC;
	pNpc->byCharState = CHARSTATE_MOVE;
	pNpc->dwMapID = dwMapID;
	pNpc->dwX = pNpc->dwXBack = pNpc->dwStartX = 4;
	pNpc->dwY = pNpc->dwYBack = pNpc->dwStartY = 4;
	strcpy(pNpc->szName, "ダンジョン受付");
	pNpc->pszNpcSpeech = new char[16];
	snprintf(pNpc->pszNpcSpeech, 16, "@d%d", nID);
	pNpc = AddChar(pNpc);
	pNpc->pID = pNpc->dwTblID + 1;
	CCharProcBase Proc;
	Proc.SetProc(pNpc);
	return pNpc->dwTblID;
}

static void TestDungeonBonus(const std::vector<int> &aLogin)
{
	printf("== AO. Dungeon bonus-game ==\n");
	if (aLogin.size() < 5) {
		return;
	}
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	if (nMapNo < 0) {
		printf("dungeonbonus no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	BYTE byPLCBack = pMap->byPartsLightCount;
	MAPPARTSLIGHT aPLBack[MAX_MAPPARTSLIGHT];
	memcpy(aPLBack, pMap->aPartsLight, sizeof(aPLBack));
	DWORD dwNowBack = s_dwFakeNow;

	{
		FILE *fp = fopen("SBODATA/SboWebDungeonThemes.json", "wb");
		if (fp) {
			const char *psz = "{\"seq\":5,\"themes\":[{\"id\":5,\"name\":\"test\",\"algo\":1,\"floor\":[1,2],\"wallTop\":[3],\"wallFace\":[4],\"wallFill\":[5],"
				"\"void\":0,\"arch\":[6],\"door\":7,\"doorOpen\":8,\"entry\":9,\"goal\":10,\"bottomRows\":1,\"rooms\":[],\"decos\":[],\"lights\":[],"
				"\"params\":{\"doorRate\":0,\"openRate\":0,\"floorMin\":30}},{\"id\":6,\"algo\":1,\"floor\":[1,2],\"wallTop\":[3],\"wallFace\":[4],\"goal\":0,\"params\":{\"floorMin\":30}}]}";
			fwrite(psz, 1, strlen(psz), fp);
			fclose(fp);
		}
	}

	/* 遊ぶ人（TestSpace と同じように、持ち物と状態を決めて始める） */
	PCHARINFO apPC[5];
	CHARINFO aBack[5];
	for (int i = 0; i < 5; i ++) {
		apPC[i] = g_aCharInfo[aLogin[i]];
		PCHARINFO p = apPC[i];
		TestCharBackup(&aBack[i], p);
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		p->byLevel = 30; p->byPW = 99; p->wMaxHP = 500; p->wHP = 480; p->wMaxMana = 90; p->wMana = 80; p->dwGold = 5000;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int k = 0; k < MAX_EQUTYPE; k ++) p->waEqu[k] = 0xFFFF;
	}
	PCHARINFO pA = apPC[0], pB = apPC[1], pC = apPC[2], pD = apPC[3], pE = apPC[4];

	/* 使うアイテム（大事な物でない 5 種） */
	WORD awItem[5];
	int nItem = 0;
	for (int i = 1; i < g_aItemInfo.GetSize() && i < ITEMID_DERIVE_BASE && nItem < 5; i ++) {
		if (g_aItemInfo[i]->szName[0] && !IsImportantItem((WORD)i)) awItem[nItem ++] = (WORD)i;
	}
	if (nItem < 5) {
		printf("dungeonbonus no items\n");
		return;
	}

	/* テンプレート 2 つ（1 つ目は道具 2 つ持ち、2 つ目は手ぶら） */
	int nSt1 = g_aStTmpInfo.GetSize(), nSt2 = nSt1 + 1;
	for (int k = 0; k < 2; k ++) {
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, k == 0 ? "戦士の姿" : "魔法使いの姿");
		t->byLevel = (BYTE)(7 + k); t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 20;
		if (k == 0) { t->waItem[0] = awItem[0]; t->waItem[1] = awItem[1]; }
		g_aStTmpInfo.Add(t);
	}

	SUBGAME_DUNGEONINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 12; Info.byY1 = 10; Info.byX2 = 51; Info.byY2 = 49;
	Info.byStartX = 3; Info.byStartY = 4;
	Info.wChipIDStart = 21; Info.wChipIDGoal = 22; Info.wChipIDRoad = 23; Info.wChipIDBlock = 24;
	Info.dwTblID = (DWORD)-1;
	Info.dwThemeID = 5;
	Info.byTmplCount = 2;
	Info.wTmplID[0] = (WORD)nSt1; strcpy(Info.szTmplName[0], "戦士");
	Info.wTmplID[1] = (WORD)nSt2;
	SUBGAMEINFO Sub;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);
	MakeDungeonProc(byID);

	/* 外の入口・中のゴール（外の入口へ戻るワープ） */
	PEVENTINFO pEntry = NULL, pGoal = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pE = &pMap->pEvent[i];
		if (pE->byType != EVENTTYPE_MAPCHG) continue;
		if (pE->dwX == 3 && pE->dwY == 4) pEntry = pE;
		else if (pE->dwX >= 12 && pE->dwX <= 51 && pE->dwY >= 10 && pE->dwY <= 49 && !(pE->dwX == (DWORD)Info.byGoalX && pE->dwY == (DWORD)Info.byGoalY)) pGoal = pE;
	}
	printf("dungeonbonus build ready=%d entry=%d goal=%d\n", (int)(Info.pwGenParts != NULL && Info.pMapTmp == NULL), (int)(pEntry != NULL), (int)(pGoal != NULL));
	if (!pEntry || !pGoal) {
		return;
	}

	/* 敵の発生元（範囲の中。補充しない印は後で付ける）：3 体 */
	PCHARINFO pPut = new CHARINFO;
	ZeroMemory((PBYTE)pPut, sizeof(CHARINFO));
	pPut->byNpcType = NPCTYPE_PUT;
	SetInitCharData(pPut);
	pPut->byGameState = GAMESTATE_NPC;
	pPut->byCharState = CHARSTATE_MOVE;
	pPut->dwMapID = pMap->dwID;
	pPut->dwX = pPut->dwXBack = pPut->dwStartX = (DWORD)Info.byGoalX + 1;
	pPut->dwY = pPut->dwYBack = pPut->dwStartY = (DWORD)Info.byGoalY + 1;
	pPut->byAreaX = 6; pPut->byAreaY = 6;
	pPut->byPutCount = 3;
	pPut->wTimePutInterval = 0;
	pPut->wGrpID = 5;
	pPut->wMaxHP = pPut->wHP = 30;
	pPut->dwStTmplateID = (DWORD)-1;
	strcpy(pPut->szName, "試しの魔物");
	pPut = AddChar(pPut);
	pPut->pID = pPut->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pPut);
	}
	DWORD dwPut = pPut->dwTblID;
	DWORD dwReception = DgTestReception(pMap->dwID, byID);
	/* 起動時の一斉発生はふつうの世界へは出さない（空間ごとに出す） */
	PutCharFillAll();
	printf("dungeonbonus put normal=%d spaced=%d\n", DgTestCountEnemy(dwPut, SPACE_NORMAL), (int)DungeonPutIsSpaced(pPut));

	/* 位置 */
	QuestMovePC(pA, nMapNo, 3, 5);
	QuestMovePC(pB, nMapNo, 4, 5);
	QuestMovePC(pC, nMapNo, 2, 5);
	QuestMovePC(pD, nMapNo, 3, 6);
	QuestMovePC(pE, nMapNo, 4, 6);
	for (int i = 0; i < 5; i ++) apPC[i]->waItem[10] = awItem[4];		/* 本物の持ち物（外で持っている物） */
	QuestSendParty(pA, pB, CMDPARTY_JOIN);
	QuestSendParty(pB, pA, CMDPARTY_JOINOK);
	QuestSendParty(pD, pE, CMDPARTY_JOIN);
	QuestSendParty(pE, pD, CMDPARTY_JOINOK);

	/* 外の入口に乗るだけでは姿選びもワープも始まらない */
	int nSel0 = SpCnt(pC, CMD_DUNGEONSEL), nMsg0 = SpCnt(pC, CMD_TALKMSG);
	DungeonEventStep(pC, pEntry);
	printf("dungeonbonus entrance-only sel=%d open=%d notice=%d in=%d\n", DungeonSelCount(), SpCnt(pC, CMD_DUNGEONSEL) - nSel0,
		SpCnt(pC, CMD_TALKMSG) - nMsg0, (int)QuestInChallenge(pC));
	/* 受付後のマップ離脱：移動の共通処理で即時取消し、短時間で戻っても復活しない。 */
	{
		DWORD otherMap = (DWORD)(nMapNo == 0 ? 1 : 0);
		strcpy(Info.szMessages, "ready_cancelled=受付取消の通知");
		for (int mode = 0; mode < 5; mode ++) {
			DgTestTalk(pA, dwReception);
			if (mode > 0) DgTestSelPick(pA, 0);
			if (mode == 2 || mode == 4) DgTestSelPick(pB, 0);
			MockTrace(1);
			int noticeA = SpCnt(pA, CMD_SYSMSG), noticeB = SpCnt(pB, CMD_SYSMSG);
			int closeB = SpCnt(pB, CMD_DUNGEONSEL);
			if (mode < 3) SubProcMapMove(pB, otherMap, 1, 1);
			else {
				/* 登録処理を通らない変更でも、周期処理で選択確定前に取り消す。 */
				DeleteTblNoPC((int)pB->dwTblID);
				pB->dwMapID = otherMap;
				DungeonProcNow(s_dwFakeNow);
				AddTblNoPC((int)pB->dwTblID);
			}
			BOOL cancelled = DungeonSelMemberCount(pA) == 0 && DungeonSelMemberCount(pB) == 0;
			BOOL told = SpCnt(pA, CMD_SYSMSG) == noticeA + 1 && SpCnt(pB, CMD_SYSMSG) == noticeB + 1 &&
				DgTestText(pA, CMD_SYSMSG, "受付取消の通知") && DgTestText(pB, CMD_SYSMSG, "受付取消の通知");
			BOOL closed = mode == 2 || mode == 4 || SpCnt(pB, CMD_DUNGEONSEL) == closeB + 1;
			SubProcMapMove(pB, pMap->dwID, 4, 5);
			DgTestEnter(pB);
			BOOL blocked = !QuestInChallenge(pA) && !QuestInChallenge(pB);
			if (!cancelled || !told || !closed || !blocked) abort();
			printf("dungeonbonus map-leave mode=%d cancelled=%d notice=%d closed=%d blocked=%d\n", mode, cancelled, told, closed, blocked);
			QuestMovePC(pB, nMapNo, 4, 5);
			MockTrace(0);
		}
		/* 選択中のログアウトも、残る仲間へ同じ取消通知と閉じる指示を出す。 */
		DgTestTalk(pA, dwReception);
		MockTrace(1);
		QuestOnDecPlayer(pB);
		BOOL logout = DungeonSelMemberCount(pA) == 0 && DgTestText(pA, CMD_SYSMSG, "受付取消の通知");
		if (!logout) abort();
		printf("dungeonbonus selecting logout cancelled-notice=%d\n", logout);
		MockTrace(0);
		Info.szMessages[0] = 0;
	}
	int nLen0 = MockCmdLen(1002);
	/* 1. ソロ（C）：受付に話すと一覧が開き、選ぶと受付済みになり入口で入る */
	MockTrace(1);
	int nTalk0 = SpCnt(pC, CMD_TALKMSG);
	QuestSendTalk(pC, dwReception);
	printf("dungeonbonus reception explain-before-open=%d\n", MockCmdBefore(1002, nLen0, CMD_TALKMSG, CMD_DUNGEONSEL));
	printf("dungeonbonus reception simultaneous talk=%d sel=%d\n", SpCnt(pC, CMD_TALKMSG) - nTalk0, DungeonSelCount());
	int nOpen0 = SpCnt(pC, CMD_DUNGEONSEL);
	s_dwFakeNow += 29000;
	DgTestLegacyBegin(pC);
	printf("dungeonbonus legacy-begin active sel=%d resend=%d\n", DungeonSelCount(), SpCnt(pC, CMD_DUNGEONSEL) - nOpen0);
	s_dwFakeNow += 999; DungeonProcNow(s_dwFakeNow);
	printf("dungeonbonus reception after29999ms sel=%d\n", DungeonSelCount());
	s_dwFakeNow += 1; DungeonProcNow(s_dwFakeNow);
	DgTestLegacyBegin(pC);
	printf("dungeonbonus reception after30s sel=%d in=%d\n", DungeonSelCount(), (int)QuestInChallenge(pC));
	QuestSendTalk(pC, dwReception);
	std::vector<BYTE> aSelect; MarketLast(1002, CMD_DUNGEONSEL, aSelect);
	printf("dungeonbonus solo open sel=%d in=%d wait30=%d\n", DungeonSelCount(), (int)QuestInChallenge(pC),
		(int)(aSelect.size() >= sizeof(CMDPARA_DUNGEONSEL) && ((PCMDPARA_DUNGEONSEL)&aSelect[0])->dwWait == 30000));
	MockTrace(0);
    strcpy(Info.szMessages, "ready=");
    MockTrace(1);
	DgTestSelPick(pC, 1);
    printf("dungeonbonus ready solo outside=%d talk=%d\n", (int)!QuestInChallenge(pC),
        (int)DgTestText(pC, CMD_TALKMSG, "準備ができました。そこの入口に入ると開始です。いってらっしゃいませ。@2"));
    Info.szMessages[0] = 0;
    MockTrace(0);
    DgTestEnter(pC);
	DWORD dwS1 = pC->dwSpaceID;
	printf("dungeonbonus solo picked sel=%d in=%d indungeon=%d space>0=%d lv=%d runs=%d\n", DungeonSelCount(), (int)QuestInChallenge(pC),
		(int)QuestInDungeon(pC), (int)(dwS1 != SPACE_NORMAL), (int)pC->byLevel, DungeonRunCount());

	/* 2. パーティ（A がリーダー）：リーダー以外が話しても始まらない。リーダーが話すと全員に一覧。全員が選んだ後に入口でそろって入る */
	DgTestTalk(pB, dwReception);
	printf("dungeonbonus party member-talk sel=%d\n", DungeonSelCount());
	/* 2b. 遅れて来た仲間：B が遠くにいる間に A が話すと A だけ。選んでいる間に B が同じ受付に話すと組に加わる */
	QuestMovePC(pB, nMapNo, 30, 30);
	DgTestTalk(pA, dwReception);
	printf("dungeonbonus late leader-only sel=%d members=%d b=%d\n", DungeonSelCount(), DungeonSelMemberCount(pA), DungeonSelMemberCount(pB));
	QuestMovePC(pB, nMapNo, 4, 5);
	int nBOpen0 = SpCnt(pB, CMD_DUNGEONSEL), nBTalk0 = SpCnt(pB, CMD_TALKMSG);
	DgTestTalk(pB, dwReception);
	printf("dungeonbonus late explanation=%d\n", SpCnt(pB, CMD_TALKMSG) - nBTalk0);
	printf("dungeonbonus late b-joined sel=%d members=%d b=%d open=%d\n", DungeonSelCount(), DungeonSelMemberCount(pA), DungeonSelMemberCount(pB), SpCnt(pB, CMD_DUNGEONSEL) - nBOpen0);
	DgTestTalk(pA, dwReception);
	printf("dungeonbonus party leader-talk sel=%d\n", DungeonSelCount());
	DgTestSelPick(pA, 0);
	printf("dungeonbonus party a-picked in a=%d b=%d sel=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), DungeonSelCount());
    strcpy(Info.szMessages, "ready=全員の準備ができました。");
    MockTrace(1);
	DgTestSelPick(pB, 0);
    printf("dungeonbonus ready party outside=%d%d talk=%d%d\n", (int)!QuestInChallenge(pA), (int)!QuestInChallenge(pB),
        (int)DgTestText(pA, CMD_TALKMSG, "全員の準備ができました。@2"), (int)DgTestText(pB, CMD_TALKMSG, "全員の準備ができました。@2"));
    Info.szMessages[0] = 0;
    QuestMovePC(pA, nMapNo, 3, 13);
    DgTestEnter(pB);
    printf("dungeonbonus ready gather waiting=%d%d notice=%d\n", (int)!QuestInChallenge(pA), (int)!QuestInChallenge(pB),
        (int)DgTestText(pB, CMD_SYSMSG, "全員そろってから入口に入ってください。"));
    QuestMovePC(pA, nMapNo, 3, 5);
    int cancelBeforeA = SpCnt(pA, CMD_SYSMSG), cancelBeforeB = SpCnt(pB, CMD_SYSMSG);
    DgTestEnter(pB);
    DungeonProcNow(s_dwFakeNow);
    BOOL regularEntry = QuestInChallenge(pA) && QuestInChallenge(pB) &&
        SpCnt(pA, CMD_SYSMSG) == cancelBeforeA + 1 && SpCnt(pB, CMD_SYSMSG) == cancelBeforeB + 1 &&
        !DgTestText(pA, CMD_SYSMSG, "組の受付を取り消しました") && !DgTestText(pB, CMD_SYSMSG, "組の受付を取り消しました");
    if (!regularEntry) abort();
    printf("dungeonbonus regular-entry no-cancel=%d\n", regularEntry);
    MockTrace(0);
	DWORD dwS2 = pA->dwSpaceID;
	printf("dungeonbonus party both-in a=%d b=%d same=%d differs=%d lv=%d sel=%d runs=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB),
		(int)(pB->dwSpaceID == dwS2), (int)(dwS2 != dwS1 && dwS2 != SPACE_NORMAL), (int)pA->byLevel, DungeonSelCount(), DungeonRunCount());

	/* 共有地図：実際の受信経路で見たマスが仲間へ届き、同じ報告は再送しない。 */
	{
        std::vector<BYTE> terrain;
        char *hex = MockMapBody(DgTestConn(pA), CMD_DUNGEONMAP, 3);
        for (size_t i = 0; i < strlen(hex); i += 2) {
            unsigned v; sscanf(hex + i, "%2x", &v); terrain.push_back((BYTE)v);
        }
        free(hex);
        if (terrain.empty()) abort();
        MockTrace(1);
        // 地形受信→現在地と可視範囲の記録→200ms後の送信を本体で実行する。
        DWORD clientTime = s_dwFakeNow;
        ClientAutoMapTest::Begin(pA, pMap);
        ClientAutoMapTest::g_nMyCharID = -1;
        ClientAutoMapTest::Receive(terrain, TRUE);
        if (!ClientAutoMapTest::outgoing.empty()) abort();
        ClientAutoMapTest::g_nMyCharID = 0;
        s_dwFakeNow += 200;
        ClientAutoMapTest::AutoMapUpdate();
        BOOL clientSent = ClientAutoMapTest::deliveredSize == terrain.size() &&
            ClientAutoMapTest::s_dwSpace == dwS2 && !ClientAutoMapTest::outgoing.empty();
        printf("dungeonbonus map client terrain=%d sent=%d\n", (int)(ClientAutoMapTest::s_dwSpace == dwS2), (int)clientSent);
        fflush(stdout);
        if (!clientSent) abort();
        size_t initialCount = 0;
        for (size_t i = 0; i < ClientAutoMapTest::outgoing.size(); i ++)
            initialCount += ((PCMDPARA_DUNGEONMAPDIFF)&ClientAutoMapTest::outgoing[i][0])->wCount;
        int clientBefore = SpCnt(pB, CMD_DUNGEONMAP);
        for (size_t i = 0; i < ClientAutoMapTest::outgoing.size(); i ++) {
            std::vector<BYTE> &packet = ClientAutoMapTest::outgoing[i];
            TestRecv(pA, &packet[0], packet.size());
        }
        std::vector<BYTE> diff;
        hex = MockMapCells(DgTestConn(pB), CMD_DUNGEONMAP, offsetof(CMDPARA_DUNGEONMAPDIFF, wCount));
        for (size_t i = 0; i < strlen(hex); i += 2) {
            unsigned v; sscanf(hex + i, "%2x", &v); diff.push_back((BYTE)v);
        }
        free(hex);
        if (diff.empty() || SpCnt(pB, CMD_DUNGEONMAP) == clientBefore) abort();
        ClientAutoMapTest::Begin(pB, pMap);
        // 入場直後は地形→累積差分→自分のキャラ情報の順に届く。
        ClientAutoMapTest::g_nMyCharID = -1;
        ClientAutoMapTest::Receive(terrain, FALSE);
        // 圧縮あり・なしの差分をどちらも検査する。
        for (int zipped = 0; zipped < 2; zipped ++) {
            ClientAutoMapTest::s_Map.ClearRecord();
            ClientAutoMapTest::s_members.clear();
            ClientAutoMapTest::Receive(diff, zipped);
            BOOL reflected = ClientAutoMapTest::deliveredSize == diff.size() &&
                ClientAutoMapTest::s_members.size() == 2 && ClientAutoMapTest::s_Map.IsRecorded(pA->dwX, pA->dwY);
            printf("dungeonbonus map client diff compressed=%d reflected=%d\n", zipped, (int)reflected);
            if (!reflected) abort();
        }
        s_dwFakeNow = clientTime;
        // 続く不正報告・再送の検査には未報告のマスを使う。
		CMDPARA_DUNGEONMAPDIFF h;
		ZeroMemory(&h, sizeof(h));
		h.byCmd = CMD_DUNGEONMAP; h.byValid = 2;
		h.dwTblID = pA->dwTblID; h.dwSpaceID = dwS2; h.dwMapID = pA->dwMapID; h.dwBuildNo = Info.dwBuildNo;
		h.wCount = 2;
		WORD cells[2] = { 7, (WORD)(0x8000 | 8) };
		std::vector<BYTE> packet(sizeof(h) + sizeof(cells));
		memcpy(&packet[0], &h, sizeof(h)); memcpy(&packet[sizeof(h)], cells, sizeof(cells));
		int before = SpCnt(pB, CMD_DUNGEONMAP), other = SpCnt(pC, CMD_DUNGEONMAP);
		TestRecv(pA, &packet[0], packet.size());
		std::vector<BYTE> body;
		BOOL got = MarketLast(DgTestConn(pB), CMD_DUNGEONMAP, body);
		BOOL same = got && body.size() == sizeof(h) + sizeof(cells) + 2 * sizeof(DUNGEONMAPPOS);
		BOOL positions = FALSE;
		if (same) {
			PCMDPARA_DUNGEONMAPDIFF r = (PCMDPARA_DUNGEONMAPDIFF)&body[0];
			same = r->byValid == 2 && r->wCount == 2 && r->dwSpaceID == dwS2 && !memcmp(r + 1, cells, sizeof(cells));
			DUNGEONMAPPOS *pos = (DUNGEONMAPPOS *)(&body[sizeof(h) + sizeof(cells)]);
			positions = r->byMember == 2 && pos[0].dwTblID == pA->dwTblID && pos[1].dwTblID == pB->dwTblID &&
				pos[0].wX == pA->dwX && pos[0].wY == pA->dwY && pos[1].wX == pB->dwX && pos[1].wY == pB->dwY;
		}
		if (!same || !positions || SpCnt(pB, CMD_DUNGEONMAP) - before != 1 || SpCnt(pC, CMD_DUNGEONMAP) != other || DungeonMapSpace(pC)) abort();
		printf("dungeonbonus map shared=%d cells=%d positions=%d isolated=%d solo=%d\n", SpCnt(pB, CMD_DUNGEONMAP) - before,
			(int)same, (int)positions, (int)(SpCnt(pC, CMD_DUNGEONMAP) == other), (int)(DungeonMapSpace(pC) == 0));
		before = SpCnt(pB, CMD_DUNGEONMAP);
		TestRecv(pA, &packet[0], packet.size());
		printf("dungeonbonus map duplicate=%d\n", SpCnt(pB, CMD_DUNGEONMAP) - before);
		/* 本人以外・別空間・古い生成・短いパケット・範囲外は記録も配信もしない。 */
		PCMDPARA_DUNGEONMAPDIFF bad = (PCMDPARA_DUNGEONMAPDIFF)&packet[0];
		bad->dwTblID = pB->dwTblID; TestRecv(pA, &packet[0], packet.size()); bad->dwTblID = pA->dwTblID;
		bad->dwSpaceID = dwS1; TestRecv(pA, &packet[0], packet.size()); bad->dwSpaceID = dwS2;
		bad->dwBuildNo ++; TestRecv(pA, &packet[0], packet.size()); bad->dwBuildNo --;
		TestRecv(pA, &packet[0], sizeof(h) - 1);
		((WORD *)(bad + 1))[0] = 0x7fff; TestRecv(pA, &packet[0], packet.size());
		printf("dungeonbonus map invalid=%d\n", SpCnt(pB, CMD_DUNGEONMAP) - before);
		/* 地形再送時は組の累積記録、位置のみの更新はマスを付けずに届く。 */
		DungeonMapSend(pB);
		got = MarketLast(DgTestConn(pB), CMD_DUNGEONMAP, body);
		printf("dungeonbonus map snapshot=%d\n", (int)(got && ((PCMDPARA_DUNGEONMAPDIFF)&body[0])->wCount == initialCount + 2));
		DungeonProcNow(s_dwFakeNow);
		pA->dwX ++;
		before = SpCnt(pB, CMD_DUNGEONMAP);
		DungeonProcNow(s_dwFakeNow);
		got = MarketLast(DgTestConn(pB), CMD_DUNGEONMAP, body);
		BOOL moved = got && ((PCMDPARA_DUNGEONMAPDIFF)&body[0])->wCount == 0 &&
			((DUNGEONMAPPOS *)(&body[sizeof(h)]))[0].wX == pA->dwX;
		if (!moved || SpCnt(pB, CMD_DUNGEONMAP) - before != 1) abort();
		printf("dungeonbonus map moved=%d position-only=%d\n", SpCnt(pB, CMD_DUNGEONMAP) - before, (int)moved);
		before = SpCnt(pB, CMD_DUNGEONMAP);
		DungeonProcNow(s_dwFakeNow);
		printf("dungeonbonus map stationary=%d\n", SpCnt(pB, CMD_DUNGEONMAP) - before);
		pA->dwX --;
		MockTrace(0);
	}

	/* 3. 締め切り：D が選び、E は選ばない → 30 秒後に D だけ受付済みになり入口で入り、E は置いていかれる */
	int nDOpen0 = SpCnt(pD, CMD_DUNGEONSEL), nEOpen0 = SpCnt(pE, CMD_DUNGEONSEL);
	int nDTalk0 = SpCnt(pD, CMD_TALKMSG), nETalk0 = SpCnt(pE, CMD_TALKMSG);
	DgTestTalk(pD, dwReception);
	printf("dungeonbonus party explanation leader=%d member=%d\n", SpCnt(pD, CMD_TALKMSG) - nDTalk0, SpCnt(pE, CMD_TALKMSG) - nETalk0);
	printf("dungeonbonus deadline open sel=%d members=%d d-open=%d e-open=%d\n", DungeonSelCount(), DungeonSelMemberCount(pD),
		SpCnt(pD, CMD_DUNGEONSEL) - nDOpen0, SpCnt(pE, CMD_DUNGEONSEL) - nEOpen0);
	DgTestSelPick(pD, 0);
	printf("dungeonbonus deadline d-picked in d=%d sel=%d\n", (int)QuestInChallenge(pD), DungeonSelCount());
	s_dwFakeNow += 10000;
	DungeonProcNow(s_dwFakeNow);
	printf("dungeonbonus deadline 10s in d=%d sel=%d\n", (int)QuestInChallenge(pD), DungeonSelCount());
	strcpy(Info.szMessages, "selection_timeout=締め切りになりました。");
	MockTrace(1);
	s_dwFakeNow += 21000;
	DungeonProcNow(s_dwFakeNow);
	printf("dungeonbonus messages timeout=%d\n", (int)DgTestText(pE, CMD_SYSMSG, "締め切りになりました。"));
	Info.szMessages[0] = 0;
	MockTrace(0);
    printf("dungeonbonus ready deadline outside=%d%d\n", (int)!QuestInChallenge(pD), (int)!QuestInChallenge(pE));
    DgTestEnter(pE);
    printf("dungeonbonus ready unselected blocked=%d\n", (int)!QuestInChallenge(pE));
    DgTestEnter(pD);
	DWORD dwS3 = pD->dwSpaceID;
	printf("dungeonbonus deadline 31s in d=%d e=%d sel=%d runs=%d\n", (int)QuestInChallenge(pD), (int)QuestInChallenge(pE), DungeonSelCount(), DungeonRunCount());

	/* 4. 組ごとの敵（別々の空間に、決めた数まで） */
	printf("dungeonbonus enemies s1=%d s2=%d s3=%d normal=%d\n", DgTestCountEnemy(dwPut, dwS1), DgTestCountEnemy(dwPut, dwS2), DgTestCountEnemy(dwPut, dwS3),
		DgTestCountEnemy(dwPut, SPACE_NORMAL));
	int nFull = DgTestCountEnemy(dwPut, dwS3);
	/* 倒すと、補充する（補充しない印なし） */
	auto KillOne = [&](DWORD dwSpace) {
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p && p->byGameState == GAMESTATE_NPC && p->dwParentTblID == dwPut && p->dwSpaceID == dwSpace && p->byNpcType != NPCTYPE_PUT && p->byNpcType != NPCTYPE_INIT) {
				DeleteChar(p->dwTblID);
				return;
			}
		}
	};
	auto ProcMany = [&]() { for (int k = 0; k < 40; k ++) { s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow); } };
	KillOne(dwS3);
	int nKilled = DgTestCountEnemy(dwPut, dwS3);
	ProcMany();
	printf("dungeonbonus refill killed=%d after=%d full=%d\n", nKilled, DgTestCountEnemy(dwPut, dwS3), (int)(DgTestCountEnemy(dwPut, dwS3) == nFull));
	/* 補充しない印：出し切ったあとは倒しても出ない。ほかの組の数には響かない。あとから入る組は最初から出る */
	pPut->byPutOnce = 1;
	KillOne(dwS3);
	ProcMany();
	printf("dungeonbonus once killed after=%d (full=%d) s1=%d s2=%d\n", DgTestCountEnemy(dwPut, dwS3), nFull, DgTestCountEnemy(dwPut, dwS1), DgTestCountEnemy(dwPut, dwS2));
	DgTestTalk(pE, dwReception);
	DgTestSelPick(pE, 1);
    DgTestEnter(pE);
	DWORD dwS4 = pE->dwSpaceID;
	printf("dungeonbonus once new-group in e=%d new space=%d enemies=%d full=%d runs=%d\n", (int)QuestInChallenge(pE), (int)(dwS4 != SPACE_NORMAL && dwS4 != dwS3 && dwS4 != dwS1 && dwS4 != dwS2),
		DgTestCountEnemy(dwPut, dwS4), (int)(DgTestCountEnemy(dwPut, dwS4) == nFull), DungeonRunCount());

	/* 5. 中で増えた物：入った時のテンプレートの持ち物は数えない。ゴールで出ても持ち帰らない（got=0） */
	int nBag0 = DungeonBagCount(pC);
	pC->waItem[5] = awItem[2];
	pC->waItem[6] = awItem[2];
	printf("dungeonbonus bag in c bag0=%d bag=%d real-item-visible=%d\n", nBag0, DungeonBagCount(pC), (int)(QuestCountItem(pC, awItem[4]) == 0));
	DungeonEventStep(pC, pGoal);
	printf("dungeonbonus goal c in=%d space=%u got=%d kept=%d lv=%d\n", (int)QuestInChallenge(pC), pC->dwSpaceID, QuestCountItem(pC, awItem[2]), QuestCountItem(pC, awItem[4]), (int)pC->byLevel);
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	printf("dungeonbonus goal freed runs=%d enemies-s1=%d\n", DungeonRunCount(), DgTestCountEnemy(dwPut, dwS1));

	/* 6. 倒れる：袋は消える（本物には入らない）。仲間は残る */
	pA->waItem[7] = awItem[3];
	printf("dungeonbonus dead a bag=%d\n", DungeonBagCount(pA));
	pA->wHP = 0;
	pA->byCharState = CHARSTATE_DEAD;
	s_dwFakeNow += 1000;
	DungeonProcNow(s_dwFakeNow);
	/* 仲間（B）が中で元気なら、倒れても中に残る（蘇生を待てる）。「帰る」（蘇生の受信）で外へ */
	printf("dungeonbonus dead a waits in=%d b-still=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB));
	QuestChallengeResurrection(pA);
	printf("dungeonbonus dead a in=%d got=%d kept=%d b-still=%d\n", (int)QuestInChallenge(pA), QuestCountItem(pA, awItem[3]), QuestCountItem(pA, awItem[4]), (int)QuestInChallenge(pB));
	pA->wHP = 100;
	pA->byCharState = CHARSTATE_MOVE;

	/* 8. 中の入口（入った時に立つ所）に乗ると途中で帰る（記録なし・元の姿） */
	{
		PEVENTINFO pRetire = NULL;
		for (int i = 0; i < pMap->nEventCount; i ++) {
			PEVENTINFO pE = &pMap->pEvent[i];
			if (pE->byType == EVENTTYPE_MAPCHG && pE->dwX == (DWORD)Info.byGoalX && pE->dwY == (DWORD)Info.byGoalY) pRetire = pE;
		}
		for (int i = 0; i < pMap->nEventCount; i ++) {
			PEVENTINFO pE = &pMap->pEvent[i];
			if (pE->byType == EVENTTYPE_MAPCHG && pE->dwX == 3 && pE->dwY == 4) pEntry = pE;
		}
		QuestMovePC(pC, nMapNo, 3, 5);
		DgTestTalk(pC, dwReception);
		DgTestSelPick(pC, 0);
    DgTestEnter(pC);
		int nIn = (int)QuestInChallenge(pC);
		/* 開始マスを踏むと、すぐには抜けず「抜けますか？」を聞く。いいえなら残り、踏み直すとまた聞く。はいで抜ける */
		int nAsk0 = SpCnt(pC, CMD_DUNGEONSEL);
		if (pRetire) DungeonEventStep(pC, pRetire);
		int nAsked = SpCnt(pC, CMD_DUNGEONSEL) - nAsk0;
		int nStay0 = (int)QuestInChallenge(pC);
		DgTestLeaveAnswer(pC, FALSE);
		int nStay1 = (int)QuestInChallenge(pC);
		if (pRetire) DungeonEventStep(pC, pRetire);
		int nAsked2 = SpCnt(pC, CMD_DUNGEONSEL) - nAsk0 - nAsked;
		DgTestLeaveAnswer(pC, TRUE);
		printf("dungeonbonus leave-ask asked=%d stay-before=%d stay-no=%d asked-again=%d\n", nAsked, nStay0, nStay1, nAsked2);
		s_dwFakeNow += 1000;
		DungeonProcNow(s_dwFakeNow);
		printf("dungeonbonus retire event=%d in-before=%d in=%d space=%u lv=%d runs=%d\n", (int)(pRetire != NULL), nIn, (int)QuestInChallenge(pC),
			pC->dwSpaceID, (int)pC->byLevel, DungeonRunCount());
		/* 帰還後は実際のイベント実行経路でも、入口に乗るだけでは入れない */
		QuestMovePC(pC, nMapNo, Info.byStartX, Info.byStartY);
		int nOpen = SpCnt(pC, CMD_DUNGEONSEL);
		EventProc(pC, pEntry, 0);
		printf("dungeonbonus return-entrance outside=%d normal=%d in=%d open=%d\n",
			(int)(pC->dwX == Info.byStartX && pC->dwY == Info.byStartY), (int)(pC->dwSpaceID == SPACE_NORMAL),
			(int)QuestInChallenge(pC), SpCnt(pC, CMD_DUNGEONSEL) - nOpen);

	}
	/* 7. 作り直し：中にいる人は全員出て（袋はなくなる）、組の空間は空きになる */
	pB->waItem[8] = awItem[3];
	MakeDungeonProc(byID);
	s_dwFakeNow += 1000;
	DungeonProcNow(s_dwFakeNow);
	printf("dungeonbonus rebuild b in=%d got=%d kept=%d d in=%d e in=%d\n", (int)QuestInChallenge(pB), QuestCountItem(pB, awItem[3]), QuestCountItem(pB, awItem[4]),
		(int)QuestInChallenge(pD), (int)QuestInChallenge(pE));
	s_dwFakeNow += 1000;
	DungeonProcNow(s_dwFakeNow);
	printf("dungeonbonus rebuild freed runs=%d enemies=%d/%d/%d/%d\n", DungeonRunCount(), DgTestCountEnemy(dwPut, dwS1), DgTestCountEnemy(dwPut, dwS2),
		DgTestCountEnemy(dwPut, dwS3), DgTestCountEnemy(dwPut, dwS4));

	/* 全敵撃破ではテーマの絵・サブゲームの絵・旧方式ともゴールを置かない。
	   発生元が無ければゴールに戻る。入口と、テーマ方式の途中帰還は残る。 */
	for (int themeID : {5, 6, 0}) {
		for (int once : {1, 0}) {
			Info.dwThemeID = themeID;
			Info.byClearMode = DGCLEAR_KILLALL;
			pPut->byPutOnce = (BYTE)once;
			MakeDungeonProc(byID);
			for (int n = 0; Info.pMapTmp && n < 10000; n ++) SubGameProc(byID);
			int gx = themeID ? Info.byX1 + Info.wGenGoalX : Info.byGoalX;
			int gy = themeID ? Info.byY1 + Info.wGenGoalY : Info.byGoalY;
			int chip = pMap->pwMap[gx + gy * pMap->nCX];
			int goalChip = themeID == 5 ? 10 : Info.wChipIDGoal;
			int picture = chip == goalChip;
			int event = SearchEvent(Info.dwMapID, gx, gy) >= 0;
			int savedPicture = themeID ? Info.pwGenParts[Info.wGenGoalX + Info.wGenGoalY * Info.wGenCX] == 10 : 0;
			printf("dungeonbonus killall theme=%d once=%d goal-event=%d picture=%d saved-picture=%d entrance=%d ready=%d\n",
				themeID, once, event, picture, savedPicture,
				(int)(SearchEvent(Info.dwMapID, Info.byStartX, Info.byStartY) >= 0), (int)(Info.pMapTmp == NULL));
		}
	}
	Info.dwThemeID = 5;
	Info.byClearMode = DGCLEAR_GOAL;
	pPut->byPutOnce = 1;
	MakeDungeonProc(byID);

	/* 左上を変えた直後は古い結果に入れず、作り直し後は新しい範囲に揃う */
	Info.byX1 = 3; Info.byY1 = 12; Info.byX2 = 42; Info.byY2 = 51;
	QuestMovePC(pC, nMapNo, 3, 5);
	DgTestTalk(pC, dwReception);
	printf("dungeonbonus moved-before-build sel=%d in=%d\n", DungeonSelCount(), (int)QuestInChallenge(pC));
	MakeDungeonProc(byID);
	int nInX = Info.byX1 + Info.wGenStartX, nInY = Info.byY1 + Info.wGenStartY;
	int nOutX = Info.byX1 + Info.wGenGoalX, nOutY = Info.byY1 + Info.wGenGoalY;
	int nInEvent = SearchEvent(Info.dwMapID, nInX, nInY), nOutEvent = SearchEvent(Info.dwMapID, nOutX, nOutY);
	DgTestTalk(pC, dwReception); DgTestSelPick(pC, 0);
    DgTestEnter(pC);
	printf("dungeonbonus moved-build entry=%d goal=%d in-area=%d floor=%d warp=%d automap=%d\n",
		(int)(nInEvent >= 0 && Info.byGoalX == nInX && Info.byGoalY == nInY), (int)(nOutEvent >= 0),
		(int)(nInX >= 3 && nInX <= 42 && nInY >= 12 && nInY <= 51 && nOutX >= 3 && nOutX <= 42 && nOutY >= 12 && nOutY <= 51),
		(int)(Info.pwGenParts[Info.wGenStartX + Info.wGenStartY * Info.wGenCX] == 9 && Info.pwGenParts[Info.wGenGoalX + Info.wGenGoalY * Info.wGenCX] == 10),
		(int)(QuestInChallenge(pC) && pC->dwX == (DWORD)nInX && pC->dwY == (DWORD)nInY),
		(int)(Info.pbyAutoMap && ((PCMDPARA_DUNGEONMAP)Info.pbyAutoMap)->wX1 == 3 && ((PCMDPARA_DUNGEONMAP)Info.pbyAutoMap)->wY1 == 12));
	int nEnemies = 0, nOutside = 0;
	for (int k = 0; k < g_aCharInfo.GetSize(); k ++) {
		PCHARINFO p = g_aCharInfo[k];
		if (p && p->byNpcType != NPCTYPE_PUT && p->dwParentTblID == dwPut && p->dwSpaceID == pC->dwSpaceID) {
			nEnemies ++;
			if (p->dwX < 3 || p->dwX > 42 || p->dwY < 12 || p->dwY > 51) nOutside ++;
		}
	}
	printf("dungeonbonus moved-enemies source=%d spawned=%d outside=%d\n",
		(int)(pPut->dwX >= 3 && pPut->dwX <= 42 && pPut->dwY >= 12 && pPut->dwY <= 51), (int)(nEnemies > 0), nOutside);
	DungeonRunLeaveAll(byID, "テスト終了");
	s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow);

    /* 受付済みは2分で失効。移動・離脱・切断でも取り消される。 */
    QuestMovePC(pC, nMapNo, 3, 5);
    DgTestTalk(pC, dwReception); DgTestSelPick(pC, 0);
    MockTrace(1);
    s_dwFakeNow += 119000; DungeonProcNow(s_dwFakeNow);
    int alive = !DgTestText(pC, CMD_SYSMSG, "準備完了から2分が過ぎたので") && !QuestInChallenge(pC);
    s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow);
    int expired = DgTestText(pC, CMD_SYSMSG, "準備完了から2分が過ぎたので");
    DgTestEnter(pC);
    printf("dungeonbonus ready expiry waiting=%d notice=%d blocked=%d\n", alive, expired, (int)!QuestInChallenge(pC));
    DgTestTalk(pC, dwReception); DgTestSelPick(pC, 0);
    MockTrace(1);
    QuestMovePC(pC, nMapNo, 30, 30);
    s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow);
    int moved = DgTestText(pC, CMD_SYSMSG, "組の受付を取り消しました。");
    QuestMovePC(pC, nMapNo, 3, 5); DgTestEnter(pC);
    printf("dungeonbonus ready moved notice=%d blocked=%d\n", moved, (int)!QuestInChallenge(pC));
    DgTestTalk(pC, dwReception); DgTestSelPick(pC, 0);
    MockTrace(1);
    QuestOnPartyLeave(pC);
    DgTestEnter(pC);
    printf("dungeonbonus ready party-leave notice=%d blocked=%d\n",
        (int)DgTestText(pC, CMD_SYSMSG, "組の受付を取り消しました。"), (int)!QuestInChallenge(pC));
    DgTestTalk(pC, dwReception); DgTestSelPick(pC, 0);
    MockTrace(1);
    QuestOnDecPlayer(pC);
    DgTestEnter(pC);
    printf("dungeonbonus ready disconnect notice=%d blocked=%d\n",
        (int)DgTestText(pC, CMD_SYSMSG, "組の受付を取り消しました。"), (int)!QuestInChallenge(pC));
    MockTrace(0);

	/* 独自説明の末尾は、新クライアントでは入力待ち無しになる。 */
	{
		PCHARINFO pNpc = g_aCharInfo[dwReception];
		LPSTR pszBack = pNpc->pszNpcSpeech;
		char szSpeech[128]; snprintf(szSpeech, sizeof(szSpeech), "@d%d 受付の独自説明です。@2", byID);
		pNpc->pszNpcSpeech = szSpeech;
		MockTrace(1);
		QuestMovePC(pC, nMapNo, 3, 5); QuestSendTalk(pC, dwReception);
		std::vector<BYTE> aBody; MarketLast(1002, CMD_TALKMSG, aBody);
		const char *psz = aBody.size() > sizeof(CMDPARA_TALKMSG) ? (const char *)&aBody[sizeof(CMDPARA_TALKMSG)] : "";
		std::vector<char> msg(psz, psz + strlen(psz) + 1);
		BOOL reception = DungeonReceptionMessage(&msg[0]);
		printf("dungeonbonus custom-talk text=%d no-wait=%d sel=%d\n", (int)(strstr(psz, "受付の独自説明です。") != NULL),
			(int)(reception && strcmp(&msg[0], "受付の独自説明です。") == 0), DungeonSelCount());
		CMDPARA_DUNGEONSEL para;
		DungeonSelectionCommand(para, pC->dwTblID, DUNGEONSEL_CANCEL, 0);
		TestRecv(pC, &para, DungeonSelectionCommandSize());
		pNpc->pszNpcSpeech = pszBack;
		MockTrace(0);
	}

    /* 設定した文と空欄の既定文を、実際に届く会話・選択パケットで確かめる。 */
    {
        strcpy(Info.szMessages, "reception=設定した説明です。\nselect_title=姿を選択してください\nentrance=窓口へどうぞ。\ncancel=今回はやめました。\nentered={name}で出発！\nall_dead=全員倒れました。");
        MockTrace(1);
        QuestMovePC(pC, nMapNo, 3, 5);
        QuestSendTalk(pC, dwReception);
        int custom = DgTestText(pC, CMD_TALKMSG, "設定した説明です。");
        int before = DungeonSelCount();
        int nOpen = SpCnt(pC, CMD_DUNGEONSEL);
        DgTestLegacyBegin(pC);
        int unchanged = SpCnt(pC, CMD_DUNGEONSEL) == nOpen && DungeonSelCount() == before;
        std::vector<BYTE> body;
        MarketLast(DgTestConn(pC), CMD_DUNGEONSEL, body);
        int title = body.size() >= sizeof(CMDPARA_DUNGEONSEL) &&
            strcmp(((PCMDPARA_DUNGEONSEL)&body[0])->szTitle, "姿を選択してください") == 0;
        CMDPARA_DUNGEONSEL para;
        DungeonSelectionCommand(para, pC->dwTblID, DUNGEONSEL_CANCEL, 0);
        TestRecv(pC, &para, DungeonSelectionCommandSize());
        printf("dungeonbonus messages custom=%d immediate=%d legacy-ignored=%d title=%d cancel=%d\n", custom, before, unchanged, title,
            (int)DgTestText(pC, CMD_SYSMSG, "今回はやめました。"));
        DgTestTalk(pC, dwReception);
        MockTrace(1); DgTestSelPick(pC, 0);
    DgTestEnter(pC);
        printf("dungeonbonus messages entered=%d\n", (int)DgTestText(pC, CMD_SYSMSG, "で出発！"));
        pC->wHP = 0; pC->byCharState = CHARSTATE_DEAD;
        s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow);
        printf("dungeonbonus messages all-dead=%d out=%d\n", (int)DgTestText(pC, CMD_SYSMSG, "全員倒れました。"), (int)!QuestInChallenge(pC));
        QuestMovePC(pC, nMapNo, 3, 5);
        strcpy(Info.szMessages, "reception=\nselect_title=");
        MockTrace(1); QuestSendTalk(pC, dwReception);
        int fallback = DgTestText(pC, CMD_TALKMSG, "経験値・お金は入りません。");
        MarketLast(DgTestConn(pC), CMD_DUNGEONSEL, body);
        title = body.size() >= sizeof(CMDPARA_DUNGEONSEL) &&
            strcmp(((PCMDPARA_DUNGEONSEL)&body[0])->szTitle, "どの姿で入る？") == 0;
        TestRecv(pC, &para, DungeonSelectionCommandSize());
        printf("dungeonbonus messages empty-default=%d title-default=%d packet-small=%d\n", fallback, title,
            (int)(sizeof(CMDPARA_DATA1) + sizeof(SUBGAME_DUNGEONINFO) + 8 < 65536));
        strcpy(Info.szMessages, "reception=設定した説明です。");
        PCHARINFO npc = g_aCharInfo[dwReception];
        LPSTR back = npc->pszNpcSpeech;
        char speech[128]; snprintf(speech, sizeof(speech), "@d%d NPCを優先します。", byID);
        npc->pszNpcSpeech = speech;
        MockTrace(1); QuestSendTalk(pC, dwReception);
        printf("dungeonbonus messages npc-priority=%d\n", (int)DgTestText(pC, CMD_TALKMSG, "NPCを優先します。"));
        TestRecv(pC, &para, DungeonSelectionCommandSize());
        npc->pszNpcSpeech = back;
        QuestMovePC(pC, nMapNo, 30, 30); DgTestLegacyBegin(pC);
        printf("dungeonbonus messages begin-moved sel=%d\n", DungeonSelCount());
        QuestMovePC(pC, nMapNo, 3, 5);
        DgTestLegacyBegin(pC);
        printf("dungeonbonus messages begin-replay sel=%d\n", DungeonSelCount());
        Info.szMessages[0] = 0;
        MockTrace(0);
    }

	/* 座標検索は同じマスのイベントを順に選び、番号指定も従来どおり */
	{
		int nFirst = SearchEvent(Info.dwMapID, nInX, nInY);
		int nNext = AddEvent(Info.dwMapID);
		pMap->pEvent[nNext].dwX = nInX; pMap->pEvent[nNext].dwY = nInY;
		CMDPARA_EVENT Para; ZeroMemory(&Para, sizeof(Para));
		Para.byCmd = CMD_EVENTGET; Para.pID = pC->pID; Para.dwID = Info.dwMapID; Para.dwX = nInX; Para.dwY = nInY;
		MockTrace(1);
		pC->byAdminLV = ADMINLV_ADMINISTRATOR;
		int aRequest[] = {-1, -2 - nFirst, -2 - nNext, nFirst};
		int aExpected[] = {nFirst, nNext, nFirst, nFirst};
		int nOK = 0;
		for (int k = 0; k < 4; k ++) {
			Para.nData = aRequest[k]; TestRecv(pC, &Para, sizeof(Para));
			std::vector<BYTE> aBody; MarketLast(1002, CMD_CHGEVENT, aBody);
			if (aBody.size() >= sizeof(CMDPARA_CHGEVENT) && ((PCMDPARA_CHGEVENT)&aBody[0])->dwDataID == (DWORD)aExpected[k]) nOK ++;
		}
		Para.nData = -1; Para.dwX = 0; Para.dwY = 0;
		int nBefore = SpCnt(pC, CMD_CHGEVENT); TestRecv(pC, &Para, sizeof(Para));
		printf("dungeonbonus event-select cycle=%d empty-kept=%d\n", (int)(nOK == 4), (int)(SpCnt(pC, CMD_CHGEVENT) == nBefore));
		pC->byAdminLV = ADMINLV_USER;
		DeleteEvent(Info.dwMapID, nNext);
		MockTrace(0);
	}

	/* 8. 保存：入る姿の列は、使う時だけ増える。読み直しで戻る */
	{
		struct stat st;
		long nSize[2] = { 0, 0 };
		int nOk = 0, nOld = 0, nMessages = 0;
		for (int k = 0; k < 2; k ++) {
			BYTE byBack = Info.byTmplCount;
			if (k == 0) Info.byTmplCount = 0;
			if (k == 1) strcpy(Info.szMessages, "reception=保存した説明\nselect_title=保存した題名");
			SaveSubGameInfo();
			if (stat("SBODATA/SboDataSubGameInfo.dat", &st) == 0) nSize[k] = (long)st.st_size;
			{
				std::vector<PSUBGAMEINFO> aOrig;
				for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) aOrig.push_back(g_aSubaGameInfo[i]);
				int nBase = (int)aOrig.size();
				g_aSubaGameInfo.RemoveAll();
				LoadSubGameInfo();
				PSUBGAMEINFO pLoad = g_aSubaGameInfo.GetSize() ? g_aSubaGameInfo[g_aSubaGameInfo.GetSize() - 1] : NULL;
				if (g_aSubaGameInfo.GetSize() == nBase && pLoad && pLoad->byType == SUBGAMEID_DUNGEON && pLoad->pInfo) {
					PSUBGAME_DUNGEONINFO pL = (PSUBGAME_DUNGEONINFO)pLoad->pInfo;
					if (k == 0) nOld = pL->szMessages[0] == 0;
					if (k == 1) nMessages = strcmp(pL->szMessages, Info.szMessages) == 0;
					if (k == 1) nOk = (int)(pL->byTmplCount == 2 && pL->wTmplID[0] == (WORD)nSt1 && pL->wTmplID[1] == (WORD)nSt2 && strcmp(pL->szTmplName[0], "戦士") == 0 && pL->szTmplName[1][0] == 0);
				}
				for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) {
					PSUBGAMEINFO pDel = g_aSubaGameInfo[i];
					SAFE_DELETE_ARRAY(pDel->pInfo);
					delete pDel;
				}
				g_aSubaGameInfo.RemoveAll();
				for (int i = 0; i < nBase; i ++) g_aSubaGameInfo.Add(aOrig[i]);
			}
			Info.byTmplCount = byBack;
			Info.szMessages[0] = 0;
		}
		printf("dungeonbonus save roundtrip=%d extra>0=%d messages=%d old-empty=%d\n", nOk, (int)(nSize[1] > nSize[0]), nMessages, nOld);
	}

	/* 後始末 */
	for (int i = 0; i < 5; i ++) {
		CHARINFO *pBk = &aBack[i];
		PCHARINFO p = apPC[i];
		QuestSendParty(p, p, CMDPARTY_BREAKUP);
		SpaceSet(p, SPACE_NORMAL);
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->dwPartyID = 0;
		p->byLevel = pBk->byLevel; p->byPW = pBk->byPW; p->wMaxHP = pBk->wMaxHP; p->wHP = pBk->wHP;
		p->wMaxMana = pBk->wMaxMana; p->wMana = pBk->wMana; p->dwGold = pBk->dwGold; p->dwExp = pBk->dwExp;
		p->byCharState = pBk->byCharState; p->byAdminLV = pBk->byAdminLV;
		memcpy(p->waItem, pBk->waItem, sizeof(p->waItem));
		memcpy(p->waEqu, pBk->waEqu, sizeof(p->waEqu));
		QuestMovePC(p, (int)pBk->dwMapID, (int)pBk->dwX, (int)pBk->dwY);
	}
	QuestSyncAll();
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->dwParentTblID == dwPut && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT) DeleteChar(p->dwTblID);
	}
	DeleteChar(dwReception);
	DeleteChar(dwPut);
	/* エリア変更は古い探索マップを捨てる（下見の間は保持する） */
	{
		PBYTE pAutoBack = Info.pbyAutoMap;
		DWORD dwSizeBack = Info.dwAutoMapSize;
		std::string strPreview = SboWebDataApply("subGame", byID, "{\"detail\":{\"byY1\":11}}", 0, "test");
		BOOL bPreview = pAutoBack && dwSizeBack && Info.pbyAutoMap == pAutoBack && Info.dwAutoMapSize == dwSizeBack;
		std::string strR = FlagGroupApi("subGame", byID, "{\"detail\":{\"byY1\":11}}");
		std::vector<BYTE> aCap;
		SvrSendCaptureBegin();
		SendCmdDUNGEONMAP(7777, pMap->dwID);
		BOOL bSent = SvrSendCaptureEnd(aCap);
		printf("dungeonbonus area automap dropped=%d off=%d preview-kept=%d\n",
			(int)(strR.find("\"ok\":true") != std::string::npos && Info.byY1 == 11 && Info.pbyAutoMap == NULL && Info.dwAutoMapSize == 0),
			(int)(bSent && aCap.size() == sizeof(CMDPARA_DUNGEONMAP) && ((PCMDPARA_DUNGEONMAP)&aCap[0])->byValid == 0), (int)bPreview);
		Info.byY1 = 10;
	}
	/* データAPIで配列・文字の項目を書き換えると全部が書き戻されるか（以前は先頭 1 バイトだけだった） */
	{
		char szJson[256];
		snprintf(szJson, sizeof(szJson), "{\"detail\":{\"wTmplID\":[%d,%d],\"szTmplName2\":\"魔法使い\",\"szMessages\":\"reception=APIの説明\\nselect_title=APIの題名\"}}", nSt2, nSt1);
		std::string strR = FlagGroupApi("subGame", byID, szJson);
		if (strR.find("\"ok\":true") == std::string::npos) printf("dungeonbonus api err %s\n", strR.c_str());
		printf("dungeonbonus api ok=%d tmpl=%d,%d name2=%s name1=%s\n", (int)(strR.find("\"ok\":true") != std::string::npos),
			(int)(Info.wTmplID[0] == (WORD)nSt2), (int)(Info.wTmplID[1] == (WORD)nSt1), Info.szTmplName[1], Info.szTmplName[0]);
        std::vector<BYTE> captured;
        SvrSendCaptureBegin();
        SendCmdSUBGAMEINFO(7777, byID);
        BOOL sent = SvrSendCaptureEnd(captured);
        int adminText = sent && captured.size() >= sizeof(CMDPARA_DATA1) + sizeof(SUBGAME_DUNGEONINFOSAVE) &&
            strcmp(((PSUBGAME_DUNGEONINFOSAVE)(&captured[0] + sizeof(CMDPARA_DATA1)))->szMessages, Info.szMessages) == 0;
        printf("dungeonbonus messages api=%d admin-send=%d\n", (int)(DungeonText(&Info, "reception") == "APIの説明"), adminText);
	}
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nSt1; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	SAFE_DELETE_ARRAY(Info.pwGenParts);
	SAFE_DELETE_ARRAY(Info.pbyAutoMap);
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
	pMap->byPartsLightCount = byPLCBack;
	memcpy(pMap->aPartsLight, aPLBack, sizeof(aPLBack));
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID);
	unlink("SBODATA/SboWebDungeonThemes.json");
	s_dwFakeNow = dwNowBack + 200000;
	printf("dungeonbonus end runs=%d sel=%d\n", DungeonRunCount(), DungeonSelCount());
}

/* ------------------------------------------------------------------------- */
/* AP. 番外編ダンジョン（段階2）											 */
/*     日替わり（同じ日は同じ形・再起動しても同じ・日付が変わったら無人の時だけ	 */
/*     作り直す）・同じ敵の位置・管理者の作り直し・版の保存と読み直し・		 */
/*     最短記録（順位・上位20件・同じ顔ぶれは速い方だけ）・クリア条件			 */
/*     「敵を全部倒す」（残り・クリアの瞬間・10秒後に全員戻る・ゴールは途中帰り）	 */
/* ------------------------------------------------------------------------- */
static unsigned DpAreaHash(PMAPINFO pMap, int x1, int y1, int x2, int y2)
{
	unsigned h = 2166136261u;
	for (int y = y1; y <= y2; y ++) for (int x = x1; x <= x2; x ++) h = (h ^ pMap->pwMap[x + y * pMap->nCX]) * 16777619u;
	return h;
}

static std::vector<std::pair<DWORD, DWORD> > DpEnemyPos(DWORD dwParent, DWORD dwSpace)
{
	std::vector<std::pair<DWORD, DWORD> > a;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->dwParentTblID == dwParent && p->dwSpaceID == dwSpace && p->byNpcType != NPCTYPE_PUT && p->byNpcType != NPCTYPE_INIT) a.push_back(std::make_pair(p->dwX, p->dwY));
	}
	std::sort(a.begin(), a.end());
	return a;
}

static void DpKillEnemy(DWORD dwParent, DWORD dwSpace)
{
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->dwParentTblID == dwParent && p->dwSpaceID == dwSpace && p->byNpcType != NPCTYPE_PUT && p->byNpcType != NPCTYPE_INIT) {
			DeleteChar(p->dwTblID);
			return;
		}
	}
}

static void TestDungeonDaily(const std::vector<int> &aLogin)
{
	printf("== AP. Dungeon daily ==\n");
	if (aLogin.size() < 5) {
		return;
	}
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	if (nMapNo < 0) {
		printf("dungeondaily no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	BYTE byPLCBack = pMap->byPartsLightCount;
	MAPPARTSLIGHT aPLBack[MAX_MAPPARTSLIGHT];
	memcpy(aPLBack, pMap->aPartsLight, sizeof(aPLBack));
	DWORD dwNowBack = s_dwFakeNow;
	const time_t T0 = 1795111200;			/* 2026-11-20 03:00 日本時間 */
	g_tDungeonDailyNow = T0;
	DungeonDailyReset(TRUE);

	{
		FILE *fp = fopen("SBODATA/SboWebDungeonThemes.json", "wb");
		if (fp) {
			const char *psz = "{\"seq\":5,\"themes\":[{\"id\":5,\"name\":\"test\",\"algo\":1,\"floor\":[1,2],\"wallTop\":[3],\"wallFace\":[4],\"wallFill\":[5],\"void\":0,\"arch\":[6],\"door\":7,"
				"\"doorOpen\":8,\"entry\":9,\"goal\":10,\"bottomRows\":1,\"rooms\":[],\"decos\":[],\"lights\":[],\"params\":{\"doorRate\":0,\"openRate\":0,\"floorMin\":30}}]}";
			fwrite(psz, 1, strlen(psz), fp);
			fclose(fp);
		}
	}

	PCHARINFO apPC[5];
	CHARINFO aBack[5];
	for (int i = 0; i < 5; i ++) {
		apPC[i] = g_aCharInfo[aLogin[i]];
		PCHARINFO p = apPC[i];
		TestCharBackup(&aBack[i], p);
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		p->byLevel = 30; p->byPW = 99; p->wMaxHP = 500; p->wHP = 480; p->wMaxMana = 90; p->wMana = 80; p->dwGold = 5000;
		ZeroMemory(p->waItem, sizeof(p->waItem));
		for (int k = 0; k < MAX_EQUTYPE; k ++) p->waEqu[k] = 0xFFFF;
	}
	PCHARINFO pA = apPC[0], pB = apPC[1], pC = apPC[2], pD = apPC[3], pE = apPC[4];

	int nSt1 = g_aStTmpInfo.GetSize();
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "戦士の姿");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 20;
		g_aStTmpInfo.Add(t);
	}

	/* 日替わり（方式 1）のダンジョンと、気まぐれ（方式 0）のダンジョン（別の場所・別のサブゲーム） */
	SUBGAME_DUNGEONINFO Info, Info0;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 12; Info.byY1 = 10; Info.byX2 = 51; Info.byY2 = 49;
	Info.byStartX = 3; Info.byStartY = 4;
	Info.wChipIDStart = 21; Info.wChipIDGoal = 22; Info.wChipIDRoad = 23; Info.wChipIDBlock = 24;
	Info.dwTblID = (DWORD)-1;
	Info.dwThemeID = 5;
	Info.byTmplCount = 1;
	Info.wTmplID[0] = (WORD)nSt1; strcpy(Info.szTmplName[0], "戦士");
	Info.byRebuildMode = DGREBUILD_DAILY;
	Info0 = Info;
	Info0.byRebuildMode = DGREBUILD_RANDOM;
	SUBGAMEINFO Sub, Sub0;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	Sub0 = Sub;
	Sub0.pInfo = &Info0;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);
	g_aSubaGameInfo.Add(&Sub0);
	BYTE byID0 = (BYTE)(g_aSubaGameInfo.GetSize() - 1);

	/* 1. 起動後の最初：日替わりは自動で作る。同じ日なら何度作っても同じ形。再起動（読み直し）しても同じ */
	g_nProcSubGame = byID;
	SubGameProc(byID);
	unsigned h1 = DpAreaHash(pMap, 12, 10, 51, 49);
	DGDAILYVER v1;
	BOOL bV1 = DungeonDailyFind(byID, Info.dwDailyVer, v1);
	printf("dungeondaily first auto ready=%d build=%u ver=%u found=%d date=%s rebuild=%d seed=%s genVer=%d themeId=%u size=%dx%d\n", (int)(Info.pwGenParts != NULL && Info.pMapTmp == NULL),
		(unsigned)Info.dwBuildNo, (unsigned)Info.dwDailyVer, (int)bV1, v1.strDate.c_str(), v1.nRebuild, (v1.dwSeed == DungeonDailySeed("2026-11-20", byID)) ? "date" : "other",
		v1.nGenVer, (unsigned)v1.dwThemeID, v1.nW, v1.nH);
	MakeDungeonProc(byID);
	printf("dungeondaily same-day rebuild same=%d build=%u versions=%d\n", (int)(DpAreaHash(pMap, 12, 10, 51, 49) == h1), (unsigned)Info.dwBuildNo, DungeonDailyVersionCount(byID));
	DungeonDailyReload();
	MakeDungeonProc(byID);
	printf("dungeondaily after-reload same=%d versions=%d themeSaved=%d\n", (int)(DpAreaHash(pMap, 12, 10, 51, 49) == h1), DungeonDailyVersionCount(byID), (int)(!DungeonDailyThemeText(v1.nThemeRef).empty()));
	/* 別のダンジョン番号は別のシード */
	printf("dungeondaily seed differs-by-dungeon=%d differs-by-date=%d\n", (int)(DungeonDailySeed("2026-11-20", byID) != DungeonDailySeed("2026-11-20", byID0)),
		(int)(DungeonDailySeed("2026-11-20", byID) != DungeonDailySeed("2026-11-21", byID)));
	/* 気まぐれ（方式 0）は日替わりの版を作らない */
	MakeDungeonProc(byID0);
	printf("dungeondaily random-mode versions=%d daily-ver=%u\n", DungeonDailyVersionCount(byID0), (unsigned)Info0.dwDailyVer);
	/* 作ったばかりの形はもう気まぐれのほうの場所に上書きされているので、日替わりをもう一度作る */
	MakeDungeonProc(byID);
	h1 = DpAreaHash(pMap, 12, 10, 51, 49);

	/* 外の入口・ゴール */
	PEVENTINFO pEntry = NULL, pGoal = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pEv = &pMap->pEvent[i];
		if (pEv->byType != EVENTTYPE_MAPCHG) continue;
		if (pEv->dwX == 3 && pEv->dwY == 4) pEntry = pEv;
		else if (pEv->dwX >= 12 && pEv->dwX <= 51 && pEv->dwY >= 10 && pEv->dwY <= 49 && !(pEv->dwX == (DWORD)Info.byGoalX && pEv->dwY == (DWORD)Info.byGoalY)) pGoal = pEv;
	}
	if (!pEntry || !pGoal) {
		printf("dungeondaily no entry/goal\n");
		return;
	}

	/* 敵の発生元（補充しない印つき 3 体） */
	PCHARINFO pPut = new CHARINFO;
	ZeroMemory((PBYTE)pPut, sizeof(CHARINFO));
	pPut->byNpcType = NPCTYPE_PUT;
	SetInitCharData(pPut);
	pPut->byGameState = GAMESTATE_NPC;
	pPut->byCharState = CHARSTATE_MOVE;
	pPut->dwMapID = pMap->dwID;
	pPut->dwX = pPut->dwXBack = pPut->dwStartX = (DWORD)Info.byGoalX + 1;
	pPut->dwY = pPut->dwYBack = pPut->dwStartY = (DWORD)Info.byGoalY + 1;
	pPut->byAreaX = 6; pPut->byAreaY = 6;
	pPut->byPutCount = 3;
	pPut->byPutOnce = 1;
	pPut->wTimePutInterval = 0;
	pPut->wGrpID = 5;
	pPut->wMaxHP = pPut->wHP = 30;
	pPut->dwStTmplateID = (DWORD)-1;
	strcpy(pPut->szName, "試しの魔物");
	pPut = AddChar(pPut);
	pPut->pID = pPut->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pPut);
	}
	DWORD dwPut = pPut->dwTblID;
	DWORD dwReception = DgTestReception(pMap->dwID, byID);
	for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
	QuestSendParty(pA, pB, CMDPARTY_JOIN);
	QuestSendParty(pB, pA, CMDPARTY_JOINOK);

	int nSel0 = SpCnt(pC, CMD_DUNGEONSEL), nMsg0 = SpCnt(pC, CMD_TALKMSG);
	DungeonEventStep(pC, pEntry);
	printf("dungeondaily entrance-only sel=%d open=%d notice=%d in=%d\n", DungeonSelCount(), SpCnt(pC, CMD_DUNGEONSEL) - nSel0,
		SpCnt(pC, CMD_TALKMSG) - nMsg0, (int)QuestInChallenge(pC));
	/* 2. 敵の出る位置：どの組も同じ（日替わり）。気まぐれは今までどおり乱数 */
	DgTestTalk(pC, dwReception);
	printf("dungeondaily reception explanation=%d\n", (int)DgTestText(pC, CMD_TALKMSG, "@2@D"));
	printf("dungeondaily reception open sel=%d members=%d in=%d\n", DungeonSelCount(), DungeonSelMemberCount(pC), (int)QuestInChallenge(pC));
	DgTestSelPick(pC, 0);
    DgTestEnter(pC);
	DWORD dwS1 = pC->dwSpaceID;
	DgTestTalk(pD, dwReception);
	DgTestSelPick(pD, 0);
    DgTestEnter(pD);
	DWORD dwS2 = pD->dwSpaceID;
	std::vector<std::pair<DWORD, DWORD> > aPos1 = DpEnemyPos(dwPut, dwS1), aPos2 = DpEnemyPos(dwPut, dwS2);
	printf("dungeondaily enemies s1=%d s2=%d same-positions=%d\n", (int)aPos1.size(), (int)aPos2.size(), (int)(aPos1 == aPos2 && !aPos1.empty()));

    {
        strcpy(Info.szMessages, "records_empty=まだ記録なし\nrecords_title=本日の順位 \nrecord_entry={rank}番 {time}{names}\nrecord_names=[{name}{others}]\nrecord_others=と{count}名\nrecord_separator= / ");
        std::string top = DungeonDailyTopText(byID, Info.dwDailyVer);
        printf("dungeondaily messages empty=%d format=%d literal-name=%d\n", (int)(top == "まだ記録なし"),
            (int)(DungeonText(&Info, "record_entry", "rank", "2", "time", "1分", "names", "[仲間]") == "2番 1分[仲間]"),
            (int)(DungeonText(&Info, "record_names", "name", "{others}", "others", "と1名") == "[{others}と1名]"));
        Info.szMessages[0] = 0;
        printf("dungeondaily messages default=%d\n", (int)(DungeonDailyTopText(byID, Info.dwDailyVer) == "今日の記録はまだありません。"));
    }

	/* 3. 日付が変わっても、中に人がいる間は作り直さない（入口で知らせる。新しく入る組は前の日の形） */
	g_tDungeonDailyNow = T0 + 24 * 3600;
	s_dwFakeNow += 2000; g_dwTime = s_dwFakeNow;
	DWORD dwBuild1 = Info.dwBuildNo;
	SubGameProc(byID);
	printf("dungeondaily date-change occupied rebuilt=%d stale=%d\n", (int)(Info.dwBuildNo != dwBuild1), (int)DungeonDailyStale(byID, Info.dwDailyVer, 5, 40, 40));
	DgTestTalk(pE, dwReception);
	DgTestSelPick(pE, 0);
    DgTestEnter(pE);
	DWORD dwS3 = pE->dwSpaceID;
	printf("dungeondaily date-change new group in=%d same-version=%d positions-same=%d\n", (int)QuestInChallenge(pE), (int)(Info.dwBuildNo == dwBuild1), (int)(DpEnemyPos(dwPut, dwS3) == aPos1));

	/* 4. ゴールの記録：入った時刻からゴールまで。日替わりは版ごとの記録に残る（前の日の版へ）。順位 */
	strcpy(Info.szMessages, "clear_time=所要時間 {time}\nrank_best=（本日{rank}番）");
	MockTrace(1);
	s_dwFakeNow += 83000;
	DungeonEventStep(pC, pGoal);
	printf("dungeondaily messages clear-notice=%d rank-notice=%d\n", (int)DgTestText(pC, CMD_SYSMSG, "所要時間 "), (int)DgTestText(pC, CMD_SYSMSG, "（本日1番）"));
	Info.szMessages[0] = 0;
	MockTrace(0);
	DGDAILYVER vOld;
	DungeonDailyFind(byID, v1.dwNo, vOld);
	printf("dungeondaily goal c in=%d records=%d ms-ok=%d count=%d form=%s\n", (int)QuestInChallenge(pC), (int)vOld.aRec.size(),
		(int)(!vOld.aRec.empty() && vOld.aRec[0].dwMs >= 83000 && vOld.aRec[0].dwMs < 100000), vOld.aRec.empty() ? 0 : vOld.aRec[0].nCount,
		vOld.aRec.empty() || vOld.aRec[0].aForm.empty() ? "-" : vOld.aRec[0].aForm[0].c_str());
	s_dwFakeNow += 20000;
	DungeonEventStep(pD, pGoal);
	DungeonDailyFind(byID, v1.dwNo, vOld);
	printf("dungeondaily goal d records=%d ordered=%d\n", (int)vOld.aRec.size(), (int)(vOld.aRec.size() == 2 && vOld.aRec[0].dwMs <= vOld.aRec[1].dwMs));
	/* 2 人目が同じ組の中で先にゴールしても、記録は 1 回だけ（C と D は別の組なので 2 件。同じ組の 2 人目は増えない）*/
	/* 5. 全員出たら（E も出す）日付が変わっていて無人 → 作り直す（新しい版・新しい日のシード） */
	s_dwFakeNow += 1000;
	DungeonEventStep(pE, pGoal);
	DungeonProcNow(s_dwFakeNow);
	s_dwFakeNow += 2000; g_dwTime = s_dwFakeNow;
	SubGameProc(byID);
	DGDAILYVER vNew;
	DungeonDailyFind(byID, Info.dwDailyVer, vNew);
	printf("dungeondaily date-change empty rebuilt=%d versions=%d new-date=%s new-seed=%s shape-changed=%d old-records-kept=%d\n", (int)(Info.dwBuildNo != dwBuild1 + 0 && Info.dwDailyVer != v1.dwNo),
		DungeonDailyVersionCount(byID), vNew.strDate.c_str(), (vNew.dwSeed == DungeonDailySeed("2026-11-21", byID)) ? "date" : "other",
		(int)(DpAreaHash(pMap, 12, 10, 51, 49) != h1), (int)(DungeonDailyFind(byID, v1.dwNo, vOld) && vOld.aRec.size() == 3));
	DWORD dwBuild2 = Info.dwBuildNo;
	s_dwFakeNow += 2000; g_dwTime = s_dwFakeNow;
	SubGameProc(byID);
	printf("dungeondaily no-more-rebuild=%d\n", (int)(Info.dwBuildNo == dwBuild2));

	/* 6. 管理者の作り直し：中の人は出て、シードが変わって保存される。その後は再起動してもそのシード */
	unsigned h2 = DpAreaHash(pMap, 12, 10, 51, 49);
	DWORD dwSeedBefore = vNew.dwSeed;
	DgTestTalk(pC, dwReception);
	DgTestSelPick(pC, 0);
    DgTestEnter(pC);
	printf("dungeondaily admin before in=%d\n", (int)QuestInChallenge(pC));
	std::string strErr;
	BOOL bOk = DungeonDailyRebuildNow(byID, strErr);
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	DGDAILYVER vAdm;
	DungeonDailyFind(byID, Info.dwDailyVer, vAdm);
	unsigned h3 = DpAreaHash(pMap, 12, 10, 51, 49);
	printf("dungeondaily admin rebuild ok=%d c-out=%d runs=%d seed-changed=%d rebuild-no=%d versions=%d shape-changed=%d\n", (int)bOk, (int)!QuestInChallenge(pC), DungeonRunCount(),
		(int)(vAdm.dwSeed != dwSeedBefore), vAdm.nRebuild, DungeonDailyVersionCount(byID), (int)(h3 != h2));
	DungeonDailyReload();
	DWORD dwBuild3 = Info.dwBuildNo;
	s_dwFakeNow += 2000; g_dwTime = s_dwFakeNow;
	SubGameProc(byID);
	DGDAILYVER vAdm2;
	BOOL bF = DungeonDailyFind(byID, Info.dwDailyVer, vAdm2);
	BOOL bNoAuto = (Info.dwBuildNo == dwBuild3);
	MakeDungeonProc(byID);
	printf("dungeondaily admin persisted found=%d seed-kept=%d no-auto-rebuild=%d same-shape-after-restart=%d versions=%d\n", (int)bF, (int)(vAdm2.dwSeed == vAdm.dwSeed), (int)bNoAuto,
		(int)(DpAreaHash(pMap, 12, 10, 51, 49) == h3), DungeonDailyVersionCount(byID));
	/* 気まぐれのダンジョンは今すぐ作り直せない。データ API */
	{
		std::string e2;
		BOOL b0 = DungeonDailyRebuildNow(byID0, e2);
		std::string sR = SboWebDataDungeonRecords(byID, 0, 1);
		std::string sB = SboWebDataDungeonRebuild(byID);
		std::string sBad = SboWebDataDungeonRebuild(byID0);
		printf("dungeondaily api random-rebuild=%d records-ok=%d has-theme=%d has-records=%d rebuild-ok=%d bad-error=%d versions-after=%d\n", (int)b0, (int)(sR.find("\"ok\":true") != std::string::npos),
			(int)(sR.find("\"theme\":{") != std::string::npos), (int)(sR.find("\"records\":[{") != std::string::npos), (int)(sB.find("\"ok\":true") != std::string::npos),
			(int)(sBad.find("\"ok\":false") != std::string::npos), DungeonDailyVersionCount(byID));
	}

	/* 7. 記録の並び・上位 20 件・同じ顔ぶれは速い方だけ（版 1 に直接足す） */
	{
		DWORD dwV = v1.dwNo;
		DGDAILYREC R;
		R.aName.push_back("x"); R.aForm.push_back("f"); R.nCount = 1;
		/* 版 1 は C・D の 2 件。さらに 22 件（顔ぶれは別々、時間は 200 秒から 1 秒ずつ短く） */
		int nLast = 0;
		for (int k = 0; k < 22; k ++) {
			R.aTbl.clear(); R.aTbl.push_back(1000 + k);
			R.dwMs = 200000 - k * 1000;
			nLast = DungeonDailyAddRecord(byID, dwV, R);
		}
		DGDAILYVER v;
		DungeonDailyFind(byID, dwV, v);
		printf("dungeondaily records count=%d last-rank=%d sorted=%d\n", (int)v.aRec.size(), nLast, (int)std::is_sorted(v.aRec.begin(), v.aRec.end(), [](const DGDAILYREC &a, const DGDAILYREC &b) { return a.dwMs < b.dwMs; }));
		R.aTbl.clear(); R.aTbl.push_back(1021);			/* 一番速い顔ぶれ（179 秒）より遅い・同じ顔ぶれ */
		R.dwMs = 300000;
		BOOL bBetter = TRUE;
		int nR1 = DungeonDailyAddRecord(byID, dwV, R, &bBetter);
		BOOL bB1 = bBetter;
		R.dwMs = 100;
		int nR2 = DungeonDailyAddRecord(byID, dwV, R, &bBetter);
		DungeonDailyFind(byID, dwV, v);
		int nSame = 0;
		for (size_t k = 0; k < v.aRec.size(); k ++) if (v.aRec[k].aTbl.size() == 1 && v.aRec[k].aTbl[0] == 1021) nSame ++;
		printf("dungeondaily records same-party slower rank=%d better=%d faster rank=%d better=%d same-party-count=%d count=%d\n", nR1, (int)bB1, nR2, (int)bBetter, nSame, (int)v.aRec.size());
		std::string strTop = DungeonDailyTopText(byID, dwV);
		printf("dungeondaily top-text first=%d time=%s\n", (int)(strTop.find("1位 0分0秒") != std::string::npos), DungeonDailyTimeText(125000).c_str());
    strcpy(Info.szMessages, "records_title=順位表 \nrecord_entry={rank}番:{time}{names}\nrecord_names=[{name}{others}]\nrecord_others=と{count}人");
    std::string customTop = DungeonDailyTopText(byID, dwV);
    printf("dungeondaily messages actual-rank=%d default-rank=%d\n", (int)(customTop.find("順位表 1番:") == 0),
        (int)(DungeonText(&Info, "rank_best", "rank", "3") == "（今日の3位）"));
    Info.szMessages[0] = 0;

		/* 保存と読み直し */
		std::string sBefore = DungeonDailyVersionsJson(byID, 0, FALSE);
		DungeonDailyReload();
		std::string sAfter = DungeonDailyVersionsJson(byID, 0, FALSE);
		printf("dungeondaily reload same-json=%d len>0=%d\n", (int)(sBefore == sAfter), (int)(sAfter.size() > 100));
	}

	/* 8. クリア条件「敵を全部倒す」：残り・クリアの瞬間・中の入口で途中帰り（記録なし）・10 秒後に全員（倒れた人も）戻る */
	Info.byClearMode = DGCLEAR_KILLALL;
	MakeDungeonProc(byID);
	DGDAILYVER vK;
	DungeonDailyFind(byID, Info.dwDailyVer, vK);
	DWORD dwKVer = vK.dwNo;
	pMap = g_aMapInfo[nMapNo];
	/* ゴール・入口が作り直しで新しくなっているので探し直す */
	pEntry = pGoal = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pEv = &pMap->pEvent[i];
		if (pEv->byType != EVENTTYPE_MAPCHG) continue;
		if (pEv->dwX == 3 && pEv->dwY == 4) pEntry = pEv;
		else if (pEv->dwX >= 12 && pEv->dwX <= 51 && pEv->dwY >= 10 && pEv->dwY <= 49 && !(pEv->dwX == (DWORD)Info.byGoalX && pEv->dwY == (DWORD)Info.byGoalY)) pGoal = pEv;
	}
	int gx = Info.byX1 + Info.wGenGoalX, gy = Info.byY1 + Info.wGenGoalY;
	printf("dungeondaily killall once goal-event=%d picture=%d saved-picture=%d\n", (int)(pGoal != NULL),
		(int)(pMap->pwMap[gx + gy * pMap->nCX] == 10 || pMap->pwMap[gx + gy * pMap->nCX] == Info.wChipIDGoal),
		(int)(Info.pwGenParts[Info.wGenGoalX + Info.wGenGoalY * Info.wGenCX] == 10));
	PEVENTINFO pRetire = NULL;
	int nRetire = SearchEvent(Info.dwMapID, Info.byGoalX, Info.byGoalY);
	if (nRetire >= 0) pRetire = &pMap->pEvent[nRetire];
	pPut->byPutDone = 0;
	for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
	/* 中の入口は途中で帰る出口（記録なし） */
	DgTestTalk(pC, dwReception);
	DgTestSelPick(pC, 0);
    DgTestEnter(pC);
	DWORD dwK1 = pC->dwSpaceID;
	s_dwFakeNow += 5000;
	DungeonEventStep(pC, pRetire);
	DgTestLeaveAnswer(pC, TRUE);
	DungeonDailyFind(byID, dwKVer, vK);
	printf("dungeondaily killall entrance-retire out=%d records=%d\n", (int)!QuestInChallenge(pC), (int)vK.aRec.size());
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	/* パーティ（A・B）で入る。敵は 3 体 */
	DgTestTalk(pA, dwReception);
	DgTestSelPick(pA, 0);
	DgTestSelPick(pB, 0);
    DgTestEnter(pB);
	DWORD dwK2 = pA->dwSpaceID;
	int nEn = DgTestCountEnemy(dwPut, dwK2);
	DWORD dwStartMs = s_dwFakeNow;
	s_dwFakeNow += 30000;
	DpKillEnemy(dwPut, dwK2);
	DpKillEnemy(dwPut, dwK2);
	printf("dungeondaily killall start in=%d%d enemies=%d after2=%d cleared-yet=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), nEn, DgTestCountEnemy(dwPut, dwK2),
		(int)(DungeonDailyFind(byID, dwKVer, vK) && vK.aRec.size() > 0));
	/* 状況パネル（CMD_DGSTATUS）：A にも B にも届き、残り敵数・総数・経過秒・目標が入っている */
	auto DgSt = [&](const char *pszTag) {
		CMDPARA_DGSTATUS St;
		std::vector<BYTE> aSt;
		MockTrace(1);
		DungeonProcNow(s_dwFakeNow);
		if (MarketLast(DgTestConn(pA), CMD_DGSTATUS, aSt) && aSt.size() >= sizeof(St)) {
			memcpy(&St, &aSt[0], sizeof(St));
			printf("dungeonstatus %s state=%d goal=%d enemy=%d/%d elapsed=%u title-ok=%d a=%d b-got=%d defstatus=%d\n", pszTag, (int)St.byState, (int)St.byGoal, (int)St.wEnemy, (int)St.wEnemyMax,
				(unsigned)St.dwElapsedSec, (int)(St.szTitle[0] != 0), (int)(MockCmdCount(DgTestConn(pA), CMD_DGSTATUS) > 0), (int)(MockCmdCount(DgTestConn(pB), CMD_DGSTATUS) > 0), MockCmdCount(DgTestConn(pA), CMD_DGDEFSTATUS));
		} else {
			printf("dungeonstatus %s none\n", pszTag);
		}
	};
	DgSt("killall-after2");
	s_dwFakeNow += 12345;
	DpKillEnemy(dwPut, dwK2);
	DgSt("killall-cleared");
	MockTrace(0);
	DungeonDailyFind(byID, dwKVer, vK);
	printf("dungeondaily killall cleared records=%d ms=%u count=%d in=%d%d\n", (int)vK.aRec.size(), vK.aRec.empty() ? 0u : (unsigned)vK.aRec[0].dwMs, vK.aRec.empty() ? 0 : vK.aRec[0].nCount,
		(int)QuestInChallenge(pA), (int)QuestInChallenge(pB));
	(void)dwStartMs;
	/* B が倒れても、10 秒たつまで残り、いっしょに戻る */
	pB->wHP = 0; pB->byCharState = CHARSTATE_DEAD;
	s_dwFakeNow += 3000; DungeonProcNow(s_dwFakeNow);
	printf("dungeondaily killall countdown 3s in=%d%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB));
	s_dwFakeNow += 8000; DungeonProcNow(s_dwFakeNow);
	printf("dungeondaily killall after10s in=%d%d b-hp>0=%d runs=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), (int)(pB->wHP > 0), DungeonRunCount());
	s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow);
	/* 補充しない印の敵が 1 体もいない設定なら、ゴールと同じ動き（ゴールで記録される） */
	pPut->byPutOnce = 0;
	MakeDungeonProc(byID);
	gx = Info.byX1 + Info.wGenGoalX; gy = Info.byY1 + Info.wGenGoalY;
	int nGoal = SearchEvent(Info.dwMapID, gx, gy);
	pGoal = nGoal >= 0 ? &pMap->pEvent[nGoal] : NULL;
	printf("dungeondaily killall no-once goal-event=%d picture=%d saved-picture=%d\n", (int)(pGoal != NULL),
		(int)(pMap->pwMap[gx + gy * pMap->nCX] == 10),
		(int)(Info.pwGenParts[Info.wGenGoalX + Info.wGenGoalY * Info.wGenCX] == 10));
	for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
	pB->wHP = 300; pB->byCharState = CHARSTATE_MOVE;
	DgTestTalk(pD, dwReception);
	DgTestSelPick(pD, 0);
    DgTestEnter(pD);
	s_dwFakeNow += 7000;
	DungeonEventStep(pD, pGoal);
	DungeonDailyFind(byID, dwKVer, vK);
	printf("dungeondaily killall no-once-enemy acts-as-goal out=%d records=%d\n", (int)!QuestInChallenge(pD), (int)vK.aRec.size());
	(void)dwK1;

	/* 9. 気まぐれ（方式 0）のダンジョンでゴールしても、記録は保存しない */
	{
		int nBefore = DungeonDailyVersionCount(byID0);
		Info0.byClearMode = DGCLEAR_GOAL;
		MakeDungeonProc(byID0);
		printf("dungeondaily random-mode records-versions=%d->%d\n", nBefore, DungeonDailyVersionCount(byID0));
	}

	/* 10. 一発勝負：順位に入るのは、その版で初めて挑戦した人だけの組。2 回目の人が混ざると入らない。作り直しでまた 1 回目 */
	{
		Info.byClearMode = DGCLEAR_GOAL;
		pPut->byPutOnce = 0;
		std::string strE1;
		DungeonDailyRebuildNow(byID, strE1);
		DungeonProcNow(s_dwFakeNow + 1000);
		s_dwFakeNow += 1000;
		DWORD dwOV = Info.dwDailyVer;
		pMap = g_aMapInfo[nMapNo];
		pEntry = pGoal = NULL;
		for (int i = 0; i < pMap->nEventCount; i ++) {
			PEVENTINFO pEv = &pMap->pEvent[i];
			if (pEv->byType != EVENTTYPE_MAPCHG) continue;
			if (pEv->dwX == 3 && pEv->dwY == 4) pEntry = pEv;
			else if (pEv->dwX >= 12 && pEv->dwX <= 51 && pEv->dwY >= 10 && pEv->dwY <= 49 && !(pEv->dwX == (DWORD)Info.byGoalX && pEv->dwY == (DWORD)Info.byGoalY)) pGoal = pEv;
		}
		QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
		for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
		DGDAILYVER vO;
		/* D が 1 人で初めて挑戦してクリア → 順位に入る */
		DgTestTalk(pD, dwReception);
		DgTestSelPick(pD, 0);
    DgTestEnter(pD);
		s_dwFakeNow += 5000;
		DungeonEventStep(pD, pGoal);
		DungeonDailyFind(byID, dwOV, vO);
		int nR1 = (int)vO.aRec.size();
		printf("dungeondaily oneshot first ranked records=%d tried-d=%d tried-c=%d\n", nR1, (int)DungeonDailyTried(byID, dwOV, pD->dwTblID), (int)DungeonDailyTried(byID, dwOV, pC->dwTblID));
		/* D がもう 1 回 → タイムだけ（記録は増えない） */
		QuestMovePC(pD, nMapNo, 3, 6);
		DgTestTalk(pD, dwReception);
		DgTestSelPick(pD, 0);
    DgTestEnter(pD);
		s_dwFakeNow += 5000;
		DungeonEventStep(pD, pGoal);
		DungeonDailyFind(byID, dwOV, vO);
		printf("dungeondaily oneshot second not-ranked out=%d records=%d\n", (int)!QuestInChallenge(pD), (int)vO.aRec.size());
		/* 初めての C と 2 回目の D の組 → 順位に入らない */
		QuestMovePC(pD, nMapNo, 3, 6);
		QuestSendParty(pC, pD, CMDPARTY_JOIN);
		QuestSendParty(pD, pC, CMDPARTY_JOINOK);
		DgTestTalk(pC, dwReception);
		DgTestSelPick(pC, 0);
		DgTestSelPick(pD, 0);
    DgTestEnter(pD);
		int nIn = (int)QuestInChallenge(pC) * 10 + (int)QuestInChallenge(pD);
		s_dwFakeNow += 5000;
		DungeonEventStep(pC, pGoal);
		DungeonEventStep(pD, pGoal);
		DungeonDailyFind(byID, dwOV, vO);
		printf("dungeondaily oneshot mixed party in=%02d out=%d%d records=%d tried-c=%d\n", nIn, (int)!QuestInChallenge(pC), (int)!QuestInChallenge(pD), (int)vO.aRec.size(),
			(int)DungeonDailyTried(byID, dwOV, pC->dwTblID));
		DungeonProcNow(s_dwFakeNow + 1000);
		s_dwFakeNow += 1000;
		QuestSendParty(pC, pC, CMDPARTY_BREAKUP);
		/* 初めての E が 1 人 → 入る */
		for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
		DgTestTalk(pE, dwReception);
		DgTestSelPick(pE, 0);
    DgTestEnter(pE);
		s_dwFakeNow += 5000;
		DungeonEventStep(pE, pGoal);
		DungeonDailyFind(byID, dwOV, vO);
		printf("dungeondaily oneshot another first ranked records=%d\n", (int)vO.aRec.size());
		/* 保存・読み直しでも「挑戦済み」が残る */
		DungeonDailyReload();
		DungeonDailyFind(byID, dwOV, vO);
		printf("dungeondaily oneshot reload tried=%d d=%d c=%d e=%d a=%d\n", (int)vO.aTried.size(), (int)DungeonDailyTried(byID, dwOV, pD->dwTblID), (int)DungeonDailyTried(byID, dwOV, pC->dwTblID),
			(int)DungeonDailyTried(byID, dwOV, pE->dwTblID), (int)DungeonDailyTried(byID, dwOV, pA->dwTblID));
		/* 管理者の作り直しで版が変わると、また 1 回目 */
		DungeonProcNow(s_dwFakeNow + 1000);
		s_dwFakeNow += 1000;
		DungeonDailyRebuildNow(byID, strE1);
		DungeonProcNow(s_dwFakeNow + 1000);
		s_dwFakeNow += 1000;
		DWORD dwNV = Info.dwDailyVer;
		pMap = g_aMapInfo[nMapNo];
		pEntry = pGoal = NULL;
		for (int i = 0; i < pMap->nEventCount; i ++) {
			PEVENTINFO pEv = &pMap->pEvent[i];
			if (pEv->byType != EVENTTYPE_MAPCHG) continue;
			if (pEv->dwX == 3 && pEv->dwY == 4) pEntry = pEv;
			else if (pEv->dwX >= 12 && pEv->dwX <= 51 && pEv->dwY >= 10 && pEv->dwY <= 49 && !(pEv->dwX == (DWORD)Info.byGoalX && pEv->dwY == (DWORD)Info.byGoalY)) pGoal = pEv;
		}
		printf("dungeondaily oneshot rebuilt new-version=%d tried-d=%d\n", (int)(dwNV != dwOV), (int)DungeonDailyTried(byID, dwNV, pD->dwTblID));
		for (int i = 0; i < 5; i ++) QuestMovePC(apPC[i], nMapNo, 3 + (i % 3), 5 + (i / 3));
		DgTestTalk(pD, dwReception);
		DgTestSelPick(pD, 0);
    DgTestEnter(pD);
		s_dwFakeNow += 5000;
		DungeonEventStep(pD, pGoal);
		DungeonDailyFind(byID, dwNV, vO);
		DGDAILYVER vOld2;
		DungeonDailyFind(byID, dwOV, vOld2);
		printf("dungeondaily oneshot new-version first again ranked records=%d old-version-records=%d\n", (int)vO.aRec.size(), (int)vOld2.aRec.size());
	}

	/* 後始末 */
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	for (int i = 0; i < 5; i ++) {
		CHARINFO *pBk = &aBack[i];
		PCHARINFO p = apPC[i];
		QuestSendParty(p, p, CMDPARTY_BREAKUP);
		SpaceSet(p, SPACE_NORMAL);
		p->dwSpaceID = SPACE_NORMAL;
		p->dwTargetTblID = (DWORD)-1;
		p->dwPartyID = 0;
		p->byLevel = pBk->byLevel; p->byPW = pBk->byPW; p->wMaxHP = pBk->wMaxHP; p->wHP = pBk->wHP;
		p->wMaxMana = pBk->wMaxMana; p->wMana = pBk->wMana; p->dwGold = pBk->dwGold; p->dwExp = pBk->dwExp;
		p->byCharState = pBk->byCharState; p->byAdminLV = pBk->byAdminLV;
		memcpy(p->waItem, pBk->waItem, sizeof(p->waItem));
		memcpy(p->waEqu, pBk->waEqu, sizeof(p->waEqu));
		QuestMovePC(p, (int)pBk->dwMapID, (int)pBk->dwX, (int)pBk->dwY);
	}
	QuestSyncAll();
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->dwParentTblID == dwPut && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT) DeleteChar(p->dwTblID);
	}
	DeleteChar(dwReception);
	DeleteChar(dwPut);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nSt1; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	SAFE_DELETE_ARRAY(Info.pwGenParts);
	SAFE_DELETE_ARRAY(Info.pbyAutoMap);
	SAFE_DELETE_ARRAY(Info0.pwGenParts);
	SAFE_DELETE_ARRAY(Info0.pbyAutoMap);
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
	pMap->byPartsLightCount = byPLCBack;
	memcpy(pMap->aPartsLight, aPLBack, sizeof(aPLBack));
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID0);
	g_aSubaGameInfo.RemoveAt(byID);
	unlink("SBODATA/SboWebDungeonThemes.json");
	DungeonDailyReset(TRUE);
	g_tDungeonDailyNow = 0;
	g_nProcSubGame = 0;
	s_dwFakeNow = dwNowBack + 400000;
	printf("dungeondaily end runs=%d sel=%d\n", DungeonRunCount(), DungeonSelCount());
}

/* ------------------------------------------------------------------------- */
/* AQ. 番外編ダンジョン（拠点防衛モード・段階1）								 */
/*     作り直さない固定の形・拠点ができる・波ごとに敵が出る（波番号の違う発生元は */
/*     出ない）・波の終わりで準備時間と全回復・最後の波で勝ち・拠点HP 0 で負け・	 */
/*     倒れた人は拠点の横で待って（回数×5秒）起き上がる・全員倒れても終わらない・ */
/*     泉でだけ Mana が戻る・どちらの結果も10秒後に全員帰還・保存と読み直し		 */
/*     （敵の行進・拠点を殴る動きは別の担当。ここでは拠点HPを直接減らして確かめる）*/
/* ------------------------------------------------------------------------- */
static void TestDungeonDefense(const std::vector<int> &aLogin)
{
	printf("== AQ. Dungeon defense ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	if (nMapNo < 0) {
		printf("dungeondefense no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	DWORD dwNowBack = s_dwFakeNow;
	MockTrace(1);

	/* 手で作った形：範囲（9,3)-(22,12) と、その周りを床で埋める（作り直さないので、地形はそのまま使われる） */
	WORD wFloor = 1;
	for (int y = 2; y <= 13; y ++) {
		for (int x = 2; x <= 23; x ++) pMap->pwMap[x + y * pMap->nCX] = wFloor;
	}
	unsigned hMapBefore = 0;
	for (int y = 3; y <= 12; y ++) for (int x = 9; x <= 22; x ++) hMapBefore = hMapBefore * 31 + pMap->pwMap[x + y * pMap->nCX];

	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	CHARINFO aBack;
	TestCharBackup(&aBack, pA);
	pA->byAdminLV = ADMINLV_USER;
	pA->dwPartyID = 0;
	pA->dwSpaceID = SPACE_NORMAL;
	pA->dwTargetTblID = (DWORD)-1;
	pA->wQuestID = 0; pA->byQuestStep = 0; pA->wQuestFlg = 0;
	pA->byLevel = 30; pA->byPW = 99; pA->wMaxHP = 500; pA->wHP = 480; pA->wMaxMana = 90; pA->wMana = 80; pA->dwGold = 5000;
	ZeroMemory(pA->waItem, sizeof(pA->waItem));
	for (int k = 0; k < MAX_EQUTYPE; k ++) pA->waEqu[k] = 0xFFFF;

	int nSt1 = g_aStTmpInfo.GetSize();
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "守り手の姿");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 50;
		g_aStTmpInfo.Add(t);
	}

	SUBGAME_DUNGEONINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 9; Info.byY1 = 3; Info.byX2 = 22; Info.byY2 = 12;
	Info.byStartX = 10; Info.byStartY = 6;					/* 入口＝入った人が立つマス */
	Info.byGoalX = 20; Info.byGoalY = 8;					/* 拠点 */
	Info.dwTblID = (DWORD)-1;
	Info.dwThemeID = 0;										/* テーマなし */
	Info.byTmplCount = 1;
	Info.wTmplID[0] = (WORD)nSt1; strcpy(Info.szTmplName[0], "守り手");
	Info.byRebuildMode = DGREBUILD_FIXED;
	Info.byClearMode = DGCLEAR_DEFENSE;
	Info.wDefBaseHP = 100;
	Info.byDefWaveCount = 2;
	Info.byDefPrepSec = 3;
	Info.wDefWaveMaxSec = 300;								/* 最初の run は長め（時間切れは run 2 で試す） */
	Info.byDefSpringRange = 2;
	SUBGAMEINFO Sub;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);

	/* 1. 固定の形は作らない（作成ボタンを押しても地形・生成情報は何も変わらない）。入口のワープだけ用意する */
	MakeDungeonProc(byID);
	g_nProcSubGame = byID;
	SubGameProc(byID);
	unsigned hMapAfter = 0;
	for (int y = 3; y <= 12; y ++) for (int x = 9; x <= 22; x ++) hMapAfter = hMapAfter * 31 + pMap->pwMap[x + y * pMap->nCX];
	PEVENTINFO pEntry = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pE = &pMap->pEvent[i];
		if (pE->byType == EVENTTYPE_MAPCHG && pE->dwX == 10 && pE->dwY == 6) pEntry = pE;
	}
	printf("dungeondefense fixed map-same=%d gen=%d buildNo=%lu automap=%d entry=%d events=%d\n", (int)(hMapBefore == hMapAfter),
		(int)(Info.pwGenParts != NULL), (unsigned long)Info.dwBuildNo, (int)(Info.pbyAutoMap != NULL), (int)(pEntry != NULL), pMap->nEventCount);
	SubGameProc(byID);
	printf("dungeondefense fixed events-again=%d\n", pMap->nEventCount);
	if (!pEntry) {
		return;
	}

	/* 敵の発生元：波 1（2 体）・波 2（1 体）・ふつう（0。防衛の空間では出ない）。
	   最大発生数は 4 人で遊ぶ時の数で、1 人だと 1/4（切り上げ・最低 1）になるので、波 1 は 8 で 2 体、波 2 は 1 のまま 1 体 */
	auto MakePut = [&](BYTE byWave, BYTE byCount, int x, int y, WORD wInterval) -> DWORD {
		PCHARINFO pPut = new CHARINFO;
		ZeroMemory((PBYTE)pPut, sizeof(CHARINFO));
		pPut->byNpcType = NPCTYPE_PUT;
		SetInitCharData(pPut);
		pPut->byGameState = GAMESTATE_NPC;
		pPut->byCharState = CHARSTATE_MOVE;
		pPut->dwMapID = pMap->dwID;
		pPut->dwX = pPut->dwXBack = pPut->dwStartX = (DWORD)x;
		pPut->dwY = pPut->dwYBack = pPut->dwStartY = (DWORD)y;
		pPut->byAreaX = 2; pPut->byAreaY = 2;
		pPut->byPutCount = byCount;
		pPut->byPutWave = byWave;
		pPut->wTimePutInterval = wInterval;
		pPut->wGrpID = 5;
		pPut->wMaxHP = pPut->wHP = 30;
		pPut->dwStTmplateID = (DWORD)-1;
		strcpy(pPut->szName, "試しの魔物");
		pPut = AddChar(pPut);
		pPut->pID = pPut->dwTblID + 1;
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pPut);
		return pPut->dwTblID;
	};
	DWORD dwPut1 = MakePut(1, 8, 17, 6, 0);
	DWORD dwPut2 = MakePut(2, 1, 17, 10, 0);
	DWORD dwPut0 = MakePut(0, 3, 15, 8, 0);
	DWORD dwReception = DgTestReception(pMap->dwID, byID);
	PutCharFillAll();
	printf("dungeondefense put normal=%d,%d,%d spaced=%d\n", DgTestCountEnemy(dwPut1, SPACE_NORMAL), DgTestCountEnemy(dwPut2, SPACE_NORMAL),
		DgTestCountEnemy(dwPut0, SPACE_NORMAL), (int)DungeonPutIsSpaced(g_aCharInfo[dwPut1]));

	auto Tick = [&](int n) { for (int k = 0; k < n; k ++) { s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow); } };
	auto Say = [&](const char *psz) -> int { return MockHasText(DgTestConn(pA), CMD_SYSMSG, psz); };
	auto Mark = [&]() { MockTrace(1); };
	/* 画面の帯の状態（CMD_DGDEFSTATUS）：最後に届いた内容と、届いた数 */
	auto Status = [&](const char *pszTag) {
		CMDPARA_DGDEFSTATUS St;
		std::vector<BYTE> aSt;
		if (MarketLast(DgTestConn(pA), CMD_DGDEFSTATUS, aSt) && aSt.size() >= sizeof(St)) {
			memcpy(&St, &aSt[0], sizeof(St));
			printf("dungeondefense status %s sent=%d state=%d wave=%d/%d hp=%d/%d rest=%d enemy=%d\n", pszTag, SpCnt(pA, CMD_DGDEFSTATUS), (int)St.byState, (int)St.byWave, (int)St.byWaveMax,
				(int)St.wBaseHP, (int)St.wBaseMaxHP, (int)St.wRestSec, (int)St.wEnemy);
		} else {
			printf("dungeondefense status %s none sent=%d\n", pszTag, SpCnt(pA, CMD_DGDEFSTATUS));
		}
	};
	auto Enemies = [&](DWORD dwSpace) -> int { return DgTestCountEnemy(dwPut1, dwSpace) + DgTestCountEnemy(dwPut2, dwSpace) + DgTestCountEnemy(dwPut0, dwSpace); };
	auto Enter = [&]() {
		QuestMovePC(pA, nMapNo, 4, 5);
		DgTestTalk(pA, dwReception);
		DgTestSelPick(pA, 0);
		QuestMovePC(pA, nMapNo, 10, 6);
		DungeonEventStep(pA, pEntry);
	};

	/* 2. 入る：組の空間・拠点ができる・敵はまだ出ない */
	Enter();
	DWORD dwSpace = pA->dwSpaceID;
	PCHARINFO pBase = DgDefenseBaseOf(dwSpace);
	printf("dungeondefense enter in=%d run=%d space=%d base=%d\n", (int)QuestInDungeon(pA), DungeonRunCount(), (int)(dwSpace != SPACE_NORMAL), (int)(pBase != NULL));
	if (!pBase) {
		return;
	}
	printf("dungeondefense base name=%s type=%d pos=%lu,%lu hp=%d/%d space=%d isbase=%d enemy=%d player-enemy=%d normal-base=%d\n", pBase->szName, (int)pBase->byNpcType,
		(unsigned long)pBase->dwX, (unsigned long)pBase->dwY, (int)pBase->wHP, (int)pBase->wMaxHP, (int)(pBase->dwSpaceID == dwSpace),
		(int)DgIsDefenseBase(pBase), (int)DgIsDefenseEnemy(pBase), (int)DgIsDefenseEnemy(pA), (int)(DgDefenseBaseOf(SPACE_NORMAL) != NULL));
	printf("dungeondefense player pos=%lu,%lu hp=%d/%d mana=%d/%d enemies=%d\n", (unsigned long)pA->dwX, (unsigned long)pA->dwY, (int)pA->wHP, (int)pA->wMaxHP,
		(int)pA->wMana, (int)pA->wMaxMana, Enemies(dwSpace));

	/* プレイヤーから拠点は攻撃できない */
	{
		pA->dwTargetTblID = pBase->dwTblID;
		pA->dwLastBattle = 1;
		BOOL bAtk = BattleProcAtack(pA, pBase);
		printf("dungeondefense player-attack-base ret=%d hp=%d\n", (int)bAtk, (int)pBase->wHP);
		pA->dwTargetTblID = (DWORD)-1;
		pA->dwLastBattle = 0;
	}

	/* 3. 最初の波の前に準備時間（3 秒）。波 1 が始まると、波 1 の発生元だけが 1 体ずつ出す */
	Mark();
	Tick(1);
	printf("dungeondefense prep coming=%d enemies=%d\n", Say("ウェーブ 1/2 が来ます（3秒後）"), Enemies(dwSpace));
	Status("prep");
	Tick(2);
	printf("dungeondefense prep-2s enemies=%d start=%d\n", Enemies(dwSpace), Say("ウェーブ 1 開始"));
	Status("prep-2s");
	Tick(1);
	printf("dungeondefense wave1 start=%d put1=%d put2=%d put0=%d\n", Say("ウェーブ 1 開始"), DgTestCountEnemy(dwPut1, dwSpace), DgTestCountEnemy(dwPut2, dwSpace), DgTestCountEnemy(dwPut0, dwSpace));
	Status("wave1");
	Tick(1);
	printf("dungeondefense wave1 put1=%d put2=%d put0=%d normal=%d\n", DgTestCountEnemy(dwPut1, dwSpace), DgTestCountEnemy(dwPut2, dwSpace), DgTestCountEnemy(dwPut0, dwSpace),
		Enemies(SPACE_NORMAL));
	{
		int nIsEnemy = 0, nIsOther = 0;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (!p || p->dwSpaceID != dwSpace || p->byGameState != GAMESTATE_NPC) continue;
			if (DgIsDefenseEnemy(p)) nIsEnemy ++; else nIsOther ++;
		}
		printf("dungeondefense enemy-judge enemy=%d other=%d\n", nIsEnemy, nIsOther);
	}
	/* 全体地図（CMD_DGDEFMAP）：約 1 秒ごとに組へ届き、範囲・拠点・泉・仲間と生きている敵の位置（詰めた形）が入っている */
	{
		std::vector<BYTE> aMap;
		CMDPARA_DGDEFMAP Mp;
		int nAlive = 0, nMatch = 0, nSizeOk = 0;
		if (MarketLast(DgTestConn(pA), CMD_DGDEFMAP, aMap) && aMap.size() >= sizeof(Mp)) {
			memcpy(&Mp, &aMap[0], sizeof(Mp));
			nSizeOk = (aMap.size() == sizeof(Mp) + Mp.byMember * sizeof(DUNGEONMAPPOS) + Mp.wEnemy * 2);
			for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
				PCHARINFO p = g_aCharInfo[i];
				if (!p || p->dwSpaceID != dwSpace || !DgIsDefenseEnemy(p) || p->wHP == 0) continue;
				nAlive ++;
				for (int k = 0; nSizeOk && k < (int)Mp.wEnemy; k ++) {
					const BYTE *pe = &aMap[sizeof(Mp) + Mp.byMember * sizeof(DUNGEONMAPPOS) + k * 2];
					if (pe[0] == (BYTE)p->dwX && pe[1] == (BYTE)p->dwY) { nMatch ++; break; }
				}
			}
			DUNGEONMAPPOS Me;
			memset(&Me, 0, sizeof(Me));
			if (Mp.byMember >= 1) memcpy(&Me, &aMap[sizeof(Mp)], sizeof(Me));
			printf("dungeondefense map sent=%d size-ok=%d members=%d me=%d,%d(%d) enemies=%d alive=%d match=%d base=%d,%d(%d) spring=%d,%d r=%d range=%d,%d-%d,%d\n", SpCnt(pA, CMD_DGDEFMAP), nSizeOk, (int)Mp.byMember,
				(int)Me.wX, (int)Me.wY, (int)(Me.dwTblID == pA->dwTblID && Me.wX == pA->dwX && Me.wY == pA->dwY), (int)Mp.wEnemy, nAlive, nMatch,
				(int)Mp.wBaseX, (int)Mp.wBaseY, (int)(pBase && Mp.wBaseX == pBase->dwX && Mp.wBaseY == pBase->dwY),
				(int)Mp.wSpringX, (int)Mp.wSpringY, (int)Mp.bySpringRange, (int)Mp.wX1, (int)Mp.wY1, (int)Mp.wX2, (int)Mp.wY2);
		} else {
			printf("dungeondefense map none sent=%d\n", SpCnt(pA, CMD_DGDEFMAP));
		}
	}
	Tick(3);
	printf("dungeondefense wave1 hold put1=%d wave-end=%d\n", DgTestCountEnemy(dwPut1, dwSpace), Say("ウェーブ 1 をしのいだ"));

	/* 4. 泉：拠点から遠いと Mana は戻らない（自然回復なし）・範囲内にいる間だけ最大Manaの 10% ずつ */
	pA->wMana = 10;
	Tick(2);
	printf("dungeondefense spring far mana=%d\n", (int)pA->wMana);
	QuestMovePC(pA, nMapNo, 19, 8);
	Tick(2);
	printf("dungeondefense spring near mana=%d\n", (int)pA->wMana);
	QuestMovePC(pA, nMapNo, 17, 8);
	Tick(2);
	printf("dungeondefense spring out mana=%d\n", (int)pA->wMana);
	/* 4b. 泉を拠点から離れた位置（12,6）に置く：拠点の横では戻らず、泉の位置で最大Manaの「泉の回復(%/秒)」ずつ戻る（ここは byDefSpringPct=2 を明示して、設定が効くことと従来の 2% の値を確かめる。既定の 10% は上の 4 で確認）（数字は毎回・キラッとは最初だけ。時間を進めすぎると後ろの状態帯の送信数がずれるので短く） */
	{
		WORD wManaBk = pA->wMana, wMaxBk = pA->wMaxMana;
		Info.byDefSpringX = 12; Info.byDefSpringY = 6;
		Info.byDefSpringPct = 2;
		pA->wMaxMana = 500; pA->wMana = 10;
		QuestMovePC(pA, nMapNo, 19, 8);
		Tick(1);
		printf("dungeondefense spring moved base-side mana=%d\n", (int)pA->wMana);
		QuestMovePC(pA, nMapNo, 12, 6);
		int nEfc0 = SpCnt(pA, CMD_ADDCHAREFC), nChg0 = SpCnt(pA, CMD_EFECTCHG);
		Tick(1);
		printf("dungeondefense spring moved at-spring mana=%d efc=%d fx=%d\n", (int)pA->wMana, SpCnt(pA, CMD_ADDCHAREFC) - nEfc0, SpCnt(pA, CMD_EFECTCHG) - nChg0);
		Tick(1);
		printf("dungeondefense spring moved at-spring +1s mana=%d efc=%d fx=%d\n", (int)pA->wMana, SpCnt(pA, CMD_ADDCHAREFC) - nEfc0, SpCnt(pA, CMD_EFECTCHG) - nChg0);
		/* 泉の回復を 5% にすると、最大Mana 500 の 5% = 25 ずつ戻る */
		Info.byDefSpringPct = 5;
		pA->wMana = 10;
		Tick(1);
		printf("dungeondefense spring pct5 mana=%d\n", (int)pA->wMana);
		Info.byDefSpringX = 0; Info.byDefSpringY = 0; Info.byDefSpringPct = 0;
		pA->wMaxMana = wMaxBk; pA->wMana = wManaBk;
		QuestMovePC(pA, nMapNo, 17, 8);
	}

	/* 5. 拠点の HP：25% 刻みで下がった時だけ知らせる */
	{
		int n0 = SpCnt(pA, CMD_SYSMSG);
		pBase->wHP = 90;
		Tick(1);
		int n1 = SpCnt(pA, CMD_SYSMSG);
		pBase->wHP = 70;
		Mark();
		Tick(1);
		int n2 = SpCnt(pA, CMD_SYSMSG);
		BOOL b70 = Say("拠点が攻撃されています（HP 70%）");
		pBase->wHP = 60;
		Tick(1);
		int n3 = SpCnt(pA, CMD_SYSMSG);
		pBase->wHP = 40;
		Mark();
		Tick(1);
		BOOL b40 = Say("拠点が攻撃されています（HP 40%）");
		printf("dungeondefense base-hp notices 90%%=%d 70%%=%d(%d) 60%%=%d 40%%=%d\n", n1 - n0, n2 - n1, (int)b70, n3 - n2, (int)b40);
		pBase->wHP = 100;
	}

	/* 6. 倒れる：その場に体を残さず入ってすぐの位置（ここでは入口 10,6）へ移る（2026-11-21 拠点の横から変更）。回数×5秒で HP/Mana 満タンで起き上がる。お金は減らない。全員倒れても終わらない */
	{
		DWORD dwGold = pA->dwGold;
		QuestMovePC(pA, nMapNo, 15, 6);
		pA->wHP = 0; pA->byCharState = CHARSTATE_DEAD; pA->wMana = 3;
		Mark();
		Tick(1);
		int dx = (int)pA->dwX - (int)pBase->dwX, dy = (int)pA->dwY - (int)pBase->dwY;
		int dist = max(abs(dx), abs(dy));
		printf("dungeondefense down1 dead=%d at-start=%d (%d,%d dist-base=%d) down=%d inrun=%d\n", (int)(pA->byCharState == CHARSTATE_DEAD), (int)(pA->dwX == 10 && pA->dwY == 6), (int)pA->dwX, (int)pA->dwY, dist,
			Say("倒れた…。5秒後に復活します。"), (int)QuestInDungeon(pA));
		printf("dungeondefense down1 resurrect-button=%d inrun=%d\n", (int)QuestChallengeResurrection(pA), (int)QuestInDungeon(pA));
		Tick(4);
		printf("dungeondefense down1 +4s dead=%d runs=%d\n", (int)(pA->byCharState == CHARSTATE_DEAD), DungeonRunCount());
		int nSt0 = SpCnt(pA, CMD_DGDEFSTATUS);
		Mark();
		Tick(1);
		printf("dungeondefense down1 +5s dead=%d hp=%d/%d mana=%d/%d gold-kept=%d revived-msg=%d\n", (int)(pA->byCharState == CHARSTATE_DEAD), (int)pA->wHP, (int)pA->wMaxHP,
			(int)pA->wMana, (int)pA->wMaxMana, (int)(pA->dwGold == dwGold), Say("復活しました"));
		printf("dungeondefense status revive resent=%d\n", (int)(SpCnt(pA, CMD_DGDEFSTATUS) > nSt0));
		pA->wHP = 0; pA->byCharState = CHARSTATE_DEAD;
		Mark();
		Tick(1);
		printf("dungeondefense down2 down=%d\n", Say("倒れた…。10秒後に復活します。"));
		Tick(9);
		printf("dungeondefense down2 +9s dead=%d\n", (int)(pA->byCharState == CHARSTATE_DEAD));
		Tick(1);
		printf("dungeondefense down2 +10s dead=%d hp=%d\n", (int)(pA->byCharState == CHARSTATE_DEAD), (int)pA->wHP);
	}

	/* 6b. 敵が倒されるたびに「残り敵 N」（残り＝まだ出ていない数＋今いる敵）。倒れても減らない数（最後の波以外の時間切れ等）は出さない。1 秒に何体倒れても最後の数を 1 回だけ（この形は敵が 2 体なので 1 体だけ確かめる） */
	{
		auto LeftMsg = [&]() -> int { for (int n = 0; n <= 40; n ++) { char sz[64]; sprintf(sz, "残り敵 %d", n); if (Say(sz)) return n; } return -1; };
		/* 本当に消すと後ろの既存の確認（倒す数）が変わるので、HP を 0 にして「倒された」状態にし、確かめたら元に戻す */
		std::vector<std::pair<int, WORD> > aKilled;
		auto KillSome = [&](int nKill) -> int {
			int nDone = 0;
			for (int i = 0; i < g_aCharInfo.GetSize() && nDone < nKill; i ++) {
				PCHARINFO p = g_aCharInfo[i];
				if (p && p->dwSpaceID == dwSpace && DgIsDefenseEnemy(p) && p->wHP > 0) { aKilled.push_back(std::make_pair(i, p->wHP)); p->wHP = 0; nDone ++; }
			}
			return nDone;
		};
		Mark();
		Tick(1);
		printf("dungeondefense enemy-left idle msg=%d enemies=%d\n", LeftMsg(), Enemies(dwSpace));
		int nK1 = KillSome(1);
		Mark();
		Tick(1);
		printf("dungeondefense enemy-left kill1 killed=%d msg=%d enemies=%d\n", nK1, LeftMsg(), Enemies(dwSpace));
		Status("enemy-left");
		for (size_t i = 0; i < aKilled.size(); i ++) { PCHARINFO p = g_aCharInfo[aKilled[i].first]; if (p) p->wHP = aKilled[i].second; }
	}

	/* 7. 敵を倒し切ると波が終わる：準備時間に入って全員 HP/Mana 全回復・次の波の知らせ → 波 2 の敵が出る */
	{
		std::vector<DWORD> aDel;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p && DgIsDefenseEnemy(p)) aDel.push_back((DWORD)i);
		}
		pA->wHP = 20; pA->wMana = 1;
		for (size_t i = 0; i < aDel.size(); i ++) DeleteChar(aDel[i]);
		Mark();
		Tick(1);
		printf("dungeondefense wave1 clear killed=%d enemies=%d cleared=%d coming2=%d hp=%d/%d mana=%d/%d\n", (int)aDel.size(), Enemies(dwSpace), Say("ウェーブ 1 をしのいだ"),
			Say("ウェーブ 2/2 が来ます（3秒後）"), (int)pA->wHP, (int)pA->wMaxHP, (int)pA->wMana, (int)pA->wMaxMana);
		Status("wave2-prep");
		Tick(2);
		printf("dungeondefense wave2 before enemies=%d\n", Enemies(dwSpace));
		Tick(1);
		printf("dungeondefense wave2 start=%d put1=%d put2=%d put0=%d\n", Say("ウェーブ 2 開始"), DgTestCountEnemy(dwPut1, dwSpace), DgTestCountEnemy(dwPut2, dwSpace), DgTestCountEnemy(dwPut0, dwSpace));
		/* 最後の波を倒すと勝ち：結果を知らせ、10 秒後に全員帰還（敵と拠点は消える） */
		aDel.clear();
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p && DgIsDefenseEnemy(p)) aDel.push_back((DWORD)i);
		}
		for (size_t i = 0; i < aDel.size(); i ++) DeleteChar(aDel[i]);
		Mark();
		Tick(1);
		printf("dungeondefense win killed=%d cleared=%d win=%d inrun=%d enemies=%d base=%d\n", (int)aDel.size(), Say("ウェーブ 2 をしのいだ"), Say("全部のウェーブをしのいだ！"),
			(int)QuestInDungeon(pA), Enemies(dwSpace), (int)(DgDefenseBaseOf(dwSpace) != NULL));
		Status("win");
		Tick(9);
		printf("dungeondefense win +9s inrun=%d runs=%d\n", (int)QuestInDungeon(pA), DungeonRunCount());
		Tick(1);
		printf("dungeondefense win +10s inrun=%d runs=%d space=%d pos=%lu,%lu base=%d\n", (int)QuestInDungeon(pA), DungeonRunCount(), (int)pA->dwSpaceID,
			(unsigned long)pA->dwX, (unsigned long)pA->dwY, (int)(DgDefenseBaseOf(dwSpace) != NULL));
		int nLeft = 0;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p && p->dwSpaceID == dwSpace && p->byGameState == GAMESTATE_NPC) nLeft ++;
		}
		Status("left");
		printf("dungeondefense win left-npcs=%d put1=%d,%d\n", nLeft, DgTestCountEnemy(dwPut1, dwSpace), DgTestCountEnemy(dwPut2, dwSpace));
	}

	/* 8. 負け：波が長引くと（最後の波以外は）敵が残っていても次の波へ。拠点 HP 0 で負け・敵は消える・10 秒後に帰還 */
	{
		pA->wHP = pA->wMaxHP = 480; pA->wMana = pA->wMaxMana = 90;
		Info.wDefWaveMaxSec = 30;
		/* 泉の位置を決めて入ると、泉の位置に名前だけの透明な目印（通り抜けられる NPC）が立つ（入った後は位置を戻す） */
		Info.byDefSpringX = 12; Info.byDefSpringY = 6;
		Enter();
		Info.byDefSpringX = 0; Info.byDefSpringY = 0;
		dwSpace = pA->dwSpaceID;
		pBase = DgDefenseBaseOf(dwSpace);
		printf("dungeondefense run2 in=%d base=%d hp=%d\n", (int)QuestInDungeon(pA), (int)(pBase != NULL), pBase ? (int)pBase->wHP : -1);
		{
			int nMark = 0, nOk = 0;
			for (int k = 0; k < g_aCharInfo.GetSize(); k ++) {
				PCHARINFO p = g_aCharInfo[k];
				if (p && (p->dwSpaceID == dwSpace) && (p->byGameState == GAMESTATE_NPC) && (strcmp(p->szName, "回復の泉") == 0)) {
					nMark ++;
					nOk = (p->byNpcType == NPCTYPE_NOBLOCK) && (p->byGhost == CHARGHOST_NAMEONLY) && (p->dwX == 12) && (p->dwY == 6) && (p->dwParentTblID == p->dwTblID) && !DgIsDefenseEnemy(p);
				}
			}
			printf("dungeondefense run2 spring-mark count=%d ok=%d\n", nMark, nOk);
		}
		if (pBase) {
			Tick(1 + 3 + 1);
			int nWave1 = Enemies(dwSpace);
			Mark();
			Tick(28);
			printf("dungeondefense run2 wave1 enemies=%d +28s enemies=%d cleared=%d coming2=%d\n", nWave1, Enemies(dwSpace), Say("ウェーブ 1 をしのいだ"), Say("ウェーブ 2/2 が来ます"));
			Tick(1);
			printf("dungeondefense run2 timeout enemies=%d coming2=%d\n", Enemies(dwSpace), Say("ウェーブ 2/2 が来ます"));
			Tick(4);
			printf("dungeondefense run2 wave2 put1=%d put2=%d\n", DgTestCountEnemy(dwPut1, dwSpace), DgTestCountEnemy(dwPut2, dwSpace));
			pBase->wHP = 0;
			Mark();
			Tick(1);
			printf("dungeondefense lose fallen=%d enemies=%d inrun=%d\n", Say("拠点が落ちた…"), Enemies(dwSpace), (int)QuestInDungeon(pA));
			Status("lose");
			Tick(9);
			printf("dungeondefense lose +9s inrun=%d\n", (int)QuestInDungeon(pA));
			Tick(1);
			printf("dungeondefense lose +10s inrun=%d runs=%d space=%d pos=%lu,%lu\n", (int)QuestInDungeon(pA), DungeonRunCount(), (int)pA->dwSpaceID, (unsigned long)pA->dwX, (unsigned long)pA->dwY);
		}
	}

	/* 9. 保存：拠点防衛の列は使う時だけ増える。読み直しで戻る（前の版の保存データ＝列なしは 0 のまま） */
	{
		struct stat st;
		long nSize[2] = { 0, 0 };
		int nOk[2] = { 0, 0 };
		for (int k = 0; k < 2; k ++) {
			SUBGAME_DUNGEONINFO Back = Info;
			if (k == 0) {
				Info.wDefBaseHP = 0; Info.wDefBaseStTmp = 0; Info.byDefWaveCount = 0; Info.byDefPrepSec = 0; Info.wDefWaveMaxSec = 0; Info.byDefSpringRange = 0; Info.byDefSpringPct = 0;
				Info.byClearMode = DGCLEAR_GOAL; Info.byRebuildMode = DGREBUILD_RANDOM;
			} else {
				Info.wDefBaseStTmp = 3;
				Info.byDefSpringPct = 7;						/* 泉の回復も保存・読み込みできる */
			}
			SaveSubGameInfo();
			if (stat("SBODATA/SboDataSubGameInfo.dat", &st) == 0) nSize[k] = (long)st.st_size;
			std::vector<PSUBGAMEINFO> aOrig;
			for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) aOrig.push_back(g_aSubaGameInfo[i]);
			int nBase = (int)aOrig.size();
			g_aSubaGameInfo.RemoveAll();
			LoadSubGameInfo();
			PSUBGAMEINFO pLoad = g_aSubaGameInfo.GetSize() ? g_aSubaGameInfo[g_aSubaGameInfo.GetSize() - 1] : NULL;
			if (g_aSubaGameInfo.GetSize() == nBase && pLoad && pLoad->byType == SUBGAMEID_DUNGEON && pLoad->pInfo) {
				PSUBGAME_DUNGEONINFO pL = (PSUBGAME_DUNGEONINFO)pLoad->pInfo;
				if (k == 0) {
					nOk[k] = (int)(pL->byRebuildMode == DGREBUILD_RANDOM && pL->byClearMode == DGCLEAR_GOAL && pL->wDefBaseHP == 0 && pL->byDefWaveCount == 0 && pL->byStartX == 10);
				} else {
					nOk[k] = (int)(pL->byRebuildMode == DGREBUILD_FIXED && pL->byClearMode == DGCLEAR_DEFENSE && pL->wDefBaseHP == 100 && pL->wDefBaseStTmp == 3 &&
						pL->byDefWaveCount == 2 && pL->byDefPrepSec == 3 && pL->wDefWaveMaxSec == 30 && pL->byDefSpringRange == 2 && pL->byDefSpringPct == 7 && pL->byGoalX == 20 && pL->byGoalY == 8);
				}
			}
			for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) {
				PSUBGAMEINFO pDel = g_aSubaGameInfo[i];
				SAFE_DELETE_ARRAY(pDel->pInfo);
				delete pDel;
			}
			g_aSubaGameInfo.RemoveAll();
			for (int i = 0; i < nBase; i ++) g_aSubaGameInfo.Add(aOrig[i]);
			Info = Back;
		}
		printf("dungeondefense save plain=%d defense=%d extra>0=%d\n", nOk[0], nOk[1], (int)(nSize[1] > nSize[0]));
	}

	/* 10. データAPI：新しい項目を書ける・範囲外の選択肢は断る */
	{
		std::string strOk = FlagGroupApi("subGame", byID, "{\"detail\":{\"byDefWaveCount\":5,\"wDefBaseHP\":777,\"byClearMode\":2,\"byRebuildMode\":2}}");
		BOOL bSet = (Info.byDefWaveCount == 5 && Info.wDefBaseHP == 777 && Info.byClearMode == DGCLEAR_DEFENSE && Info.byRebuildMode == DGREBUILD_FIXED);
		std::string strNg = FlagGroupApi("subGame", byID, "{\"detail\":{\"byClearMode\":3}}");
		std::string strNg2 = FlagGroupApi("subGame", byID, "{\"detail\":{\"byRebuildMode\":3}}");
		printf("dungeondefense api ok=%d set=%d clear3-rejected=%d rebuild3-rejected=%d clear-kept=%d\n", (int)(strOk.find("\"ok\":true") != std::string::npos), (int)bSet,
			(int)(strNg.find("\"ok\":true") == std::string::npos), (int)(strNg2.find("\"ok\":true") == std::string::npos), (int)(Info.byClearMode == DGCLEAR_DEFENSE));
	}

	/* 11. 観戦あり（byDefArena=1）：組は闘技場の空間・2 組目は受付で断られる・観客席（ふつうの世界の範囲）の人に帯と残り敵が届き、外の人には届かない・
	       範囲を出る／組が終わると消す帯が届く */
	if (aLogin.size() >= 4) {
		PCHARINFO pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]], pD = g_aCharInfo[aLogin[3]];
		PCHARINFO apW[3] = { pB, pC, pD };
		CHARINFO aWBack[3];
		for (int k = 0; k < 3; k ++) {
			TestCharBackup(&aWBack[k], apW[k]);
			apW[k]->byAdminLV = ADMINLV_USER;
			apW[k]->dwPartyID = 0;
			apW[k]->dwSpaceID = SPACE_NORMAL;
			apW[k]->dwTargetTblID = (DWORD)-1;
			apW[k]->wQuestID = 0; apW[k]->byQuestStep = 0; apW[k]->wQuestFlg = 0;
			apW[k]->byLevel = 30; apW[k]->byPW = 99; apW[k]->wMaxHP = 500; apW[k]->wHP = 480; apW[k]->wMaxMana = 90; apW[k]->wMana = 80;
			ZeroMemory(apW[k]->waItem, sizeof(apW[k]->waItem));
			for (int m = 0; m < MAX_EQUTYPE; m ++) apW[k]->waEqu[m] = 0xFFFF;
		}
		Info.byDefWaveCount = 2; Info.wDefBaseHP = 100; Info.byDefPrepSec = 3; Info.wDefWaveMaxSec = 30;
		Info.byDefSpringX = 0; Info.byDefSpringY = 0;
		Info.byDefArena = 1;
		Info.byViewX1 = 1; Info.byViewY1 = 1; Info.byViewX2 = 6; Info.byViewY2 = 3;			/* 観客席：(1,1)-(6,3) */
		QuestMovePC(pB, nMapNo, 3, 2);									/* 観客席の中 */
		QuestMovePC(pC, nMapNo, 7, 8);									/* 観客席の外 */
		QuestMovePC(pD, nMapNo, 4, 5);									/* 受付の前（観客席の外）。2 組目の申し込み */
		/* 帯の最後の状態（届いていなければ -1） */
		auto LastState = [&](PCHARINFO p) -> int {
			CMDPARA_DGDEFSTATUS St;
			std::vector<BYTE> aSt;
			if (MarketLast(DgTestConn(p), CMD_DGDEFSTATUS, aSt) && aSt.size() >= sizeof(St)) {
				memcpy(&St, &aSt[0], sizeof(St));
				return (int)St.byState;
			}
			return -1;
		};
		auto SayTo = [&](PCHARINFO p, const char *psz) -> int { return MockHasText(DgTestConn(p), CMD_SYSMSG, psz); };
		auto LeftMsgOf = [&](PCHARINFO p) -> int { for (int n = 0; n <= 40; n ++) { char sz[64]; sprintf(sz, "残り敵 %d", n); if (SayTo(p, sz)) return n; } return -1; };

		pA->wHP = pA->wMaxHP = 480; pA->wMana = pA->wMaxMana = 90;
		Enter();
		dwSpace = pA->dwSpaceID;
		printf("dungeondefense arena enter in=%d arena=%d runs=%d\n", (int)QuestInDungeon(pA), (int)SpaceIsArena(dwSpace), DungeonRunCount());
		/* 観客席の外の人が受付に申し込んでも（2 組目）断られる */
		DgTestTalk(pD, dwReception);
		printf("dungeondefense arena second busy=%d sel=%d\n", (int)DgTestText(pD, CMD_TALKMSG, "いま別の組が拠点を守っています。終わるまで観客席で観戦して待ちましょう。"), DungeonSelCount());
		/* 周期処理で、観客席の人に帯が届く（外の人には届かない） */
		Mark();
		Tick(1);
		printf("dungeondefense arena view in sent=%d state=%d out sent=%d\n", SpCnt(pB, CMD_DGDEFSTATUS), LastState(pB), SpCnt(pC, CMD_DGDEFSTATUS));
		Tick(1 + 3);
		printf("dungeondefense arena view wave-start in=%d out=%d state=%d\n", SayTo(pB, "ウェーブ 1 開始"), SayTo(pC, "ウェーブ 1 開始"), LastState(pB));
		/* 敵が倒されると「残り敵」も観客に届く */
		{
			std::vector<std::pair<int, WORD> > aKilled;
			for (int k = 0; k < g_aCharInfo.GetSize() && aKilled.empty(); k ++) {
				PCHARINFO p = g_aCharInfo[k];
				if (p && p->dwSpaceID == dwSpace && DgIsDefenseEnemy(p) && p->wHP > 0) { aKilled.push_back(std::make_pair(k, p->wHP)); p->wHP = 0; }
			}
			Mark();
			Tick(1);
			printf("dungeondefense arena enemy-left killed=%d member=%d in=%d out=%d\n", (int)aKilled.size(), LeftMsgOf(pA), LeftMsgOf(pB), LeftMsgOf(pC));
			for (size_t k = 0; k < aKilled.size(); k ++) { PCHARINFO p = g_aCharInfo[aKilled[k].first]; if (p) p->wHP = aKilled[k].second; }
		}
		/* 観客席を出ると、消す帯が届く。戻ると今の帯が届く */
		Mark();
		QuestMovePC(pB, nMapNo, 3, 8);
		Tick(1);
		printf("dungeondefense arena view leave state=%d sent=%d\n", LastState(pB), SpCnt(pB, CMD_DGDEFSTATUS));
		QuestMovePC(pB, nMapNo, 3, 2);
		Mark();
		Tick(1);
		printf("dungeondefense arena view back state=%d sent=%d\n", LastState(pB), SpCnt(pB, CMD_DGDEFSTATUS));
		/* 拠点が落ちる：負けの帯とお知らせが観客にも届く。10 秒後に組が終わると、消す帯が届く */
		{
			PCHARINFO pBase2 = DgDefenseBaseOf(dwSpace);
			if (pBase2) pBase2->wHP = 0;
			Mark();
			Tick(1);
			printf("dungeondefense arena lose in-state=%d fallen=%d out=%d\n", LastState(pB), SayTo(pB, "拠点が落ちた…"), SayTo(pC, "拠点が落ちた…"));
			Tick(10);
			printf("dungeondefense arena end in-state=%d runs=%d out-sent=%d\n", LastState(pB), DungeonRunCount(), SpCnt(pC, CMD_DGDEFSTATUS));
		}
		Info.byDefArena = 0; Info.byViewX1 = 0; Info.byViewY1 = 0; Info.byViewX2 = 0; Info.byViewY2 = 0;
		for (int k = 0; k < 3; k ++) {
			PCHARINFO p = apW[k];
			p->byLevel = aWBack[k].byLevel; p->byPW = aWBack[k].byPW; p->wMaxHP = aWBack[k].wMaxHP; p->wHP = aWBack[k].wHP;
			p->wMaxMana = aWBack[k].wMaxMana; p->wMana = aWBack[k].wMana;
			p->byCharState = aWBack[k].byCharState; p->byAdminLV = aWBack[k].byAdminLV;
			memcpy(p->waItem, aWBack[k].waItem, sizeof(p->waItem));
			memcpy(p->waEqu, aWBack[k].waEqu, sizeof(p->waEqu));
			QuestMovePC(p, (int)aWBack[k].dwMapID, (int)aWBack[k].dwX, (int)aWBack[k].dwY);
		}
		/* 観戦なしに戻せば、今まで通り（その場で始まる）ことは上の AQ で確かめている */
		sgenrand(4357);												/* この確認で使った乱数の分、後ろのテスト（敵の殴りの当たり外れなど）の出目が動かないよう、決まった種に戻す */
	}

	/* 後始末 */
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	SpaceSet(pA, SPACE_NORMAL);
	pA->dwSpaceID = SPACE_NORMAL;
	pA->dwTargetTblID = (DWORD)-1;
	pA->dwPartyID = 0;
	pA->byLevel = aBack.byLevel; pA->byPW = aBack.byPW; pA->wMaxHP = aBack.wMaxHP; pA->wHP = aBack.wHP;
	pA->wMaxMana = aBack.wMaxMana; pA->wMana = aBack.wMana; pA->dwGold = aBack.dwGold; pA->dwExp = aBack.dwExp;
	pA->byCharState = aBack.byCharState; pA->byAdminLV = aBack.byAdminLV;
	memcpy(pA->waItem, aBack.waItem, sizeof(pA->waItem));
	memcpy(pA->waEqu, aBack.waEqu, sizeof(pA->waEqu));
	QuestMovePC(pA, (int)aBack.dwMapID, (int)aBack.dwX, (int)aBack.dwY);
	QuestSyncAll();
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT && p->dwSpaceID != SPACE_NORMAL) DeleteChar(p->dwTblID);
	}
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->dwParentTblID == dwPut1 && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT) DeleteChar(p->dwTblID);
	}
	DeleteChar(dwReception);
	DeleteChar(dwPut2);
	DeleteChar(dwPut1);
	DeleteChar(dwPut0);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nSt1; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID);
	g_nProcSubGame = 0;
	MockTrace(0);
	s_dwFakeNow = dwNowBack + 400000;
	printf("dungeondefense end runs=%d sel=%d\n", DungeonRunCount(), DungeonSelCount());
}

/* ------------------------------------------------------------------------- */
/* AS. 番外編ダンジョン（固定の形の「中の開始位置」）							 */
/*     中の開始位置（byFixStartX・byFixStartY）を設定した固定の形：外の入口に乗る */
/*     と組の空間の中の開始位置に立つ（マップ・キャラの情報が届く）・中の開始位置 */
/*     のマスに戻ると「抜けますか？」・組の外の人が踏んでも何も起きない			 */
/*     （設定しない時は AQ で確かめている。入口と同じマスのまま）				 */
/* ------------------------------------------------------------------------- */
static void TestDungeonDefenseInside(const std::vector<int> &aLogin)
{
	printf("== AS. Dungeon defense inside start ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	if (nMapNo < 0) {
		printf("dungeoninside no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	DWORD dwNowBack = s_dwFakeNow;
	MockTrace(1);

	/* 外の入口（10,6）は範囲（9,3)-(22,12）の中だが、中の開始位置（18,9）は別のマス */
	for (int y = 2; y <= 13; y ++) {
		for (int x = 2; x <= 23; x ++) pMap->pwMap[x + y * pMap->nCX] = 1;
	}
	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	PCHARINFO pB = g_aCharInfo[aLogin[1]];
	CHARINFO aBack, aBackB;
	TestCharBackup(&aBack, pA);
	TestCharBackup(&aBackB, pB);
	pA->byAdminLV = ADMINLV_USER;
	pA->dwPartyID = 0;
	pA->dwSpaceID = SPACE_NORMAL;
	pA->dwTargetTblID = (DWORD)-1;
	pA->wQuestID = 0; pA->byQuestStep = 0; pA->wQuestFlg = 0;
	pA->byLevel = 30; pA->byPW = 99; pA->wMaxHP = 500; pA->wHP = 480; pA->wMaxMana = 90; pA->wMana = 80;
	ZeroMemory(pA->waItem, sizeof(pA->waItem));
	for (int k = 0; k < MAX_EQUTYPE; k ++) pA->waEqu[k] = 0xFFFF;
	pB->byAdminLV = ADMINLV_USER;
	pB->dwPartyID = 0;
	pB->dwSpaceID = SPACE_NORMAL;
	pB->dwTargetTblID = (DWORD)-1;

	int nSt1 = g_aStTmpInfo.GetSize();
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "守り手の姿");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 50;
		g_aStTmpInfo.Add(t);
	}
	SUBGAME_DUNGEONINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 9; Info.byY1 = 3; Info.byX2 = 22; Info.byY2 = 12;
	Info.byStartX = 10; Info.byStartY = 6;					/* 外の入口 */
	Info.byFixStartX = 18; Info.byFixStartY = 9;			/* 中の開始位置 */
	Info.byGoalX = 20; Info.byGoalY = 8;					/* 拠点 */
	Info.dwTblID = (DWORD)-1;
	Info.byTmplCount = 1;
	Info.wTmplID[0] = (WORD)nSt1; strcpy(Info.szTmplName[0], "守り手");
	Info.byRebuildMode = DGREBUILD_FIXED;
	Info.byClearMode = DGCLEAR_DEFENSE;
	Info.wDefBaseHP = 100;
	Info.byDefWaveCount = 1;
	Info.byDefPrepSec = 30;
	Info.wDefWaveMaxSec = 300;
	SUBGAMEINFO Sub;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);

	/* 1. 外の入口と中の開始位置の両方にワープが置かれる。中の開始位置のワープは外の入口へ戻る */
	g_nProcSubGame = byID;
	SubGameProc(byID);
	PEVENTINFO pEntry = NULL, pInside = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		PEVENTINFO pE = &pMap->pEvent[i];
		if (pE->byType != EVENTTYPE_MAPCHG) continue;
		if (pE->dwX == 10 && pE->dwY == 6) pEntry = pE;
		if (pE->dwX == 18 && pE->dwY == 9) pInside = pE;
	}
	{
		PEVENTINFO_MAPCHG pBack = pInside ? (PEVENTINFO_MAPCHG)pInside->pData : NULL;
		DWORD dwX = 0, dwY = 0;
		printf("dungeoninside events entry=%d inside=%d inside-back=%d,%d use=%d count=%d\n", (int)(pEntry != NULL), (int)(pInside != NULL),
			pBack ? (int)pBack->dwX : -1, pBack ? (int)pBack->dwY : -1, (int)DungeonFixedInside(&Info, &dwX, &dwY), pMap->nEventCount);
	}
	SubGameProc(byID);
	printf("dungeoninside events-again=%d\n", pMap->nEventCount);
	if (!pEntry || !pInside) {
		return;
	}
	DWORD dwReception = DgTestReception(pMap->dwID, byID);

	/* 2. 組の外の人（受付済みでない）が中の開始位置のマスを踏んでも何も起きない（外の入口へ飛ばされない） */
	QuestMovePC(pB, nMapNo, 18, 9);
	BOOL bStep = DungeonEventStep(pB, pInside);
	printf("dungeoninside outsider step handled=%d pos=%lu,%lu space=%d\n", (int)bStep, (unsigned long)pB->dwX, (unsigned long)pB->dwY, (int)(pB->dwSpaceID != SPACE_NORMAL));
	QuestMovePC(pB, (int)aBackB.dwMapID, (int)aBackB.dwX, (int)aBackB.dwY);

	/* A と B は同じパーティ。B は受付から離れた所にいて、A だけが先に入る */
	QuestSendParty(pA, pB, CMDPARTY_JOIN);
	QuestSendParty(pB, pA, CMDPARTY_JOINOK);
	QuestMovePC(pB, nMapNo, 20, 13);
	printf("dungeoninside party same=%d nonzero=%d\n", (int)(pA->dwPartyID == pB->dwPartyID), (int)(pA->dwPartyID != 0));
	/* 3. 受付→外の入口に乗ると、組の空間の中の開始位置へ移る。マップ・キャラの情報が届く */
	QuestMovePC(pA, nMapNo, 4, 5);
	DgTestTalk(pA, dwReception);
	DgTestSelPick(pA, 0);
	QuestMovePC(pA, nMapNo, 10, 6);
	int nMapInfo0 = SpCnt(pA, CMD_MAPINFO), nCharInfo0 = SpCnt(pA, CMD_CHARINFO);
	DungeonEventStep(pA, pEntry);
	DWORD dwSpace = pA->dwSpaceID;
	PCHARINFO pBase = DgDefenseBaseOf(dwSpace);
	printf("dungeoninside enter in=%d run=%d space=%d pos=%lu,%lu map-same=%d base=%d\n", (int)QuestInDungeon(pA), DungeonRunCount(), (int)(dwSpace != SPACE_NORMAL),
		(unsigned long)pA->dwX, (unsigned long)pA->dwY, (int)(pA->dwMapID == pMap->dwID), (int)(pBase != NULL));
	printf("dungeoninside enter sent mapinfo=%d charinfo=%d\n", (int)(SpCnt(pA, CMD_MAPINFO) > nMapInfo0), (int)(SpCnt(pA, CMD_CHARINFO) > nCharInfo0));

	/* 3b. 同じパーティの B が遅れて受付に話しても、同じパーティ番号の組を 2 つ目として作らない
	   （作ると先の組が終わった時に B まで巻き込まれて操作できなくなる）。組は 1 つのまま・B は外のまま・選択も始まらない */
	{
		int nRun0 = DungeonRunCount();
		QuestMovePC(pB, nMapNo, 4, 5);
		DgTestTalk(pB, dwReception);
		DgTestSelPick(pB, 0);
		DungeonProcNow(s_dwFakeNow + 1000);
		s_dwFakeNow += 1000;
		printf("dungeoninside late-member runs=%d->%d sel=%d b-space=%d b-in=%d\n", nRun0, DungeonRunCount(), DungeonSelCount(),
			(int)(pB->dwSpaceID != SPACE_NORMAL), (int)QuestInDungeon(pB));
		QuestMovePC(pB, nMapNo, 20, 13);
	}

	/* 4. 中の開始位置のマスを（離れて戻って）踏むと「抜けますか？」。いいえなら残り、はいで受付の場所へ戻る */
	int nAsk0 = SpCnt(pA, CMD_DUNGEONSEL);
	DungeonEventStep(pA, pInside);
	int nAsked = SpCnt(pA, CMD_DUNGEONSEL) - nAsk0;
	DgTestLeaveAnswer(pA, FALSE);
	int nStay = (int)QuestInDungeon(pA);
	DungeonEventStep(pA, pInside);
	int nAsked2 = SpCnt(pA, CMD_DUNGEONSEL) - nAsk0 - nAsked;
	DgTestLeaveAnswer(pA, TRUE);
	s_dwFakeNow += 1000;
	DungeonProcNow(s_dwFakeNow);
	printf("dungeoninside leave-ask asked=%d stay-no=%d asked-again=%d out=%d space=%d pos=%lu,%lu runs=%d\n", nAsked, nStay, nAsked2, (int)!QuestInDungeon(pA),
		(int)(pA->dwSpaceID != SPACE_NORMAL), (unsigned long)pA->dwX, (unsigned long)pA->dwY, DungeonRunCount());

	/* 後始末 */
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT && p->dwSpaceID != SPACE_NORMAL) DeleteChar(p->dwTblID);
	}
	SpaceSet(pA, SPACE_NORMAL);
	pA->dwSpaceID = SPACE_NORMAL;
	pA->byLevel = aBack.byLevel; pA->byPW = aBack.byPW; pA->wMaxHP = aBack.wMaxHP; pA->wHP = aBack.wHP;
	pA->wMaxMana = aBack.wMaxMana; pA->wMana = aBack.wMana; pA->dwGold = aBack.dwGold; pA->dwExp = aBack.dwExp;
	pA->byCharState = aBack.byCharState; pA->byAdminLV = aBack.byAdminLV;
	memcpy(pA->waItem, aBack.waItem, sizeof(pA->waItem));
	memcpy(pA->waEqu, aBack.waEqu, sizeof(pA->waEqu));
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	pA->dwPartyID = 0; pB->dwPartyID = 0;
	QuestMovePC(pB, (int)aBackB.dwMapID, (int)aBackB.dwX, (int)aBackB.dwY);
	pB->byAdminLV = aBackB.byAdminLV;
	QuestMovePC(pA, (int)aBack.dwMapID, (int)aBack.dwX, (int)aBack.dwY);
	QuestSyncAll();
	DeleteChar(dwReception);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nSt1; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	g_nProcMap = 0;
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID);
	g_nProcSubGame = 0;
	MockTrace(0);
	s_dwFakeNow = dwNowBack + 400000;
	printf("dungeoninside end runs=%d sel=%d\n", DungeonRunCount(), DungeonSelCount());
}

/* ------------------------------------------------------------------------- */
/* AR. 番外編ダンジョン（拠点防衛モード・敵の行進）							 */
/*     壁と通り道のある小さな形で、敵が拠点へ進んで拠点の HP を減らす・遠くの	 */
/*     プレイヤーは追わない・前を敵がふさげば待って（回り込める時は別経路へ）・ */
/*     前をプレイヤーがふさげば戦う（離れたら行進に戻る）・拠点とプレイヤーが	 */
/*     となりなら拠点が先・閉じた扉は通れず組の空間で開けば通る・プレイヤーの	 */
/*     攻撃（ふつうの攻撃・範囲の特技・なぎ払い）は拠点に当たらない			 */
/* ------------------------------------------------------------------------- */
static void TestDungeonDefenseMarch(const std::vector<int> &aLogin)
{
	printf("== AR. Dungeon defense march ==\n");
	if (aLogin.size() < 1) {
		return;
	}
	int nMapNo = -1;
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		if (g_aMapInfo[i] && g_aMapInfo[i]->nCX >= 60 && g_aMapInfo[i]->nCY >= 60 && g_aMapInfo[i]->pwMap) {
			nMapNo = i;
			break;
		}
	}
	WORD wWall = 0;
	for (int i = 1; i < g_wCountMapParts; i ++) {
		if (g_aMapPartsInfo[i].byHitType == PARTSHIT_BLOCK1) {
			wWall = (WORD)i;
			break;
		}
	}
	if (nMapNo < 0 || wWall == 0) {
		printf("dungeonmarch no map\n");
		return;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	std::vector<WORD> aMapBack(pMap->pwMap, pMap->pwMap + pMap->nCX * pMap->nCY);
	PEVENTINFO pEventBack = pMap->pEvent;
	int nEventBack = pMap->nEventCount;
	BYTE byFlgBack = pMap->byFlg;
	pMap->pEvent = NULL;
	pMap->nEventCount = 0;
	pMap->byFlg |= MAPFLG_BATTLE;
	pMap->byFlg &= ~MAPFLG_MAGIC;
	DWORD dwNowBack = s_dwFakeNow, dwTimeBack = g_dwTime;
	MockTrace(1);

	/* 形：範囲（9,3)-(22,12）を床にし、x=15 に壁（y=3〜11）。y=12 が通り道（あとで扉にする）。拠点は（20,8） */
	for (int y = 2; y <= 13; y ++) {
		for (int x = 2; x <= 23; x ++) pMap->pwMap[x + y * pMap->nCX] = 1;
	}
	for (int y = 3; y <= 11; y ++) pMap->pwMap[15 + y * pMap->nCX] = wWall;

	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	CHARINFO aBack;
	TestCharBackup(&aBack, pA);
	pA->byAdminLV = ADMINLV_USER;
	pA->dwPartyID = 0;
	pA->dwSpaceID = SPACE_NORMAL;
	pA->dwTargetTblID = (DWORD)-1;
	pA->wQuestID = 0; pA->byQuestStep = 0; pA->wQuestFlg = 0;
	pA->byLevel = 30; pA->byPW = 99; pA->wMaxHP = 500; pA->wHP = 480; pA->wMaxMana = 90; pA->wMana = 80; pA->dwGold = 5000;
	ZeroMemory(pA->waItem, sizeof(pA->waItem));
	for (int k = 0; k < MAX_EQUTYPE; k ++) pA->waEqu[k] = 0xFFFF;

	int nSt1 = g_aStTmpInfo.GetSize();
	{
		PSTATUSTEMPLATEINFO t = new STATUSTEMPLATEINFO;
		ZeroMemory(t, sizeof(*t));
		strcpy(t->szName, "守り手の姿");
		t->byLevel = 7; t->byPW = 11; t->byDEX = 12; t->byST = 13; t->byIT = 14; t->byLK = 15;
		t->wMaxHP = 60; t->wMaxMana = 50;
		g_aStTmpInfo.Add(t);
	}
	SUBGAME_DUNGEONINFO Info;
	ZeroMemory(&Info, sizeof(Info));
	Info.dwMapID = pMap->dwID;
	Info.byX1 = 9; Info.byY1 = 3; Info.byX2 = 22; Info.byY2 = 12;
	Info.byStartX = 10; Info.byStartY = 6;
	Info.byGoalX = 20; Info.byGoalY = 8;
	Info.dwTblID = (DWORD)-1;
	Info.byTmplCount = 1;
	Info.wTmplID[0] = (WORD)nSt1; strcpy(Info.szTmplName[0], "守り手");
	Info.byRebuildMode = DGREBUILD_FIXED;
	Info.byClearMode = DGCLEAR_DEFENSE;
	Info.wDefBaseHP = 1000;
	Info.byDefWaveCount = 9;								/* 敵が一時的に 0 になっても、勝ちにならない */
	Info.byDefPrepSec = 3;
	Info.wDefWaveMaxSec = 3000;
	Info.byDefSpringRange = 2;
	SUBGAMEINFO Sub;
	ZeroMemory(&Sub, sizeof(Sub));
	Sub.byType = SUBGAMEID_DUNGEON;
	Sub.pInfo = &Info;
	g_aSubaGameInfo.Add(&Sub);
	BYTE byID = (BYTE)(g_aSubaGameInfo.GetSize() - 1);
	MakeDungeonProc(byID);
	g_nProcSubGame = byID;
	SubGameProc(byID);

	/* 発生元：波 1 に 3 体 */
	PCHARINFO pPut = new CHARINFO;
	ZeroMemory((PBYTE)pPut, sizeof(CHARINFO));
	pPut->byNpcType = NPCTYPE_PUT;
	SetInitCharData(pPut);
	pPut->byGameState = GAMESTATE_NPC;
	pPut->byCharState = CHARSTATE_MOVE;
	pPut->dwMapID = pMap->dwID;
	pPut->dwX = pPut->dwXBack = pPut->dwStartX = 11;
	pPut->dwY = pPut->dwYBack = pPut->dwStartY = 8;
	pPut->byAreaX = 1; pPut->byAreaY = 1;
	pPut->byPutCount = 12;		/* 4 人で遊ぶ時の数。1 人なので 1/4 の 3 体 */
	pPut->byPutWave = 1;
	pPut->wGrpID = 5;
	pPut->wMaxHP = pPut->wHP = 30;
	pPut->dwStTmplateID = (DWORD)-1;
	strcpy(pPut->szName, "試しの魔物");
	pPut = AddChar(pPut);
	pPut->pID = pPut->dwTblID + 1;
	{
		CCharProcBase CharProcBase;
		CharProcBase.SetProc(pPut);
	}
	DWORD dwPut = pPut->dwTblID;
	DWORD dwReception = DgTestReception(pMap->dwID, byID);
	PEVENTINFO pEntry = NULL;
	for (int i = 0; i < pMap->nEventCount; i ++) {
		if (pMap->pEvent[i].byType == EVENTTYPE_MAPCHG && pMap->pEvent[i].dwX == 10 && pMap->pEvent[i].dwY == 6) pEntry = &pMap->pEvent[i];
	}
	if (!pEntry) {
		printf("dungeonmarch no entry\n");
		return;
	}

	auto Tick = [&](int n) { for (int k = 0; k < n; k ++) { s_dwFakeNow += 1000; DungeonProcNow(s_dwFakeNow); } };
	auto Enemies = [&]() -> std::vector<PCHARINFO> {
		std::vector<PCHARINFO> v;
		for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
			PCHARINFO p = g_aCharInfo[i];
			if (p && p->byNpcType != NPCTYPE_INIT && DgIsDefenseEnemy(p) && p->wHP > 0) v.push_back(p);
		}
		return v;
	};
	/* 0.5 秒ずつ進める（敵の処理→戦闘処理。組の周期処理は 1 秒ごと） */
	int nStepNo = 0;
	auto Step = [&](int n, const std::function<void()> &fn) {
		for (int k = 0; k < n; k ++) {
			s_dwFakeNow += 500;
			g_dwTime = s_dwFakeNow;
			std::vector<PCHARINFO> v = Enemies();
			for (size_t i = 0; i < v.size(); i ++) {
				if (v[i]->pProc) ((PCCharProcBase)v[i]->pProc)->Proc(s_dwFakeNow);
				BattleProcCheck(v[i]);
			}
			if ((++ nStepNo) % 2 == 0) DungeonProcNow(s_dwFakeNow);
			if (fn) fn();
		}
	};
	auto Arm = [&](PCHARINFO p) { p->wAT = 10; p->byDEX = 255; p->wMaxHP = p->wHP = 5000; };
	auto Place = [&](PCHARINFO p, int x, int y) {
		p->dwX = (DWORD)x; p->dwY = (DWORD)y; p->dwLastDotMove = 0; p->dwTargetTblID = (DWORD)-1; p->dwLastBattle = 0;
	};
	auto Overlap = [&]() -> int {
		std::vector<PCHARINFO> v = Enemies();
		int n = 0;
		for (size_t i = 0; i < v.size(); i ++) {
			for (size_t j = i + 1; j < v.size(); j ++) if (v[i]->dwX == v[j]->dwX && v[i]->dwY == v[j]->dwY) n ++;
			if (v[i]->dwX == pA->dwX && v[i]->dwY == pA->dwY) n ++;
		}
		return n;
	};

	/* 入る → 準備時間 → 波 1 の敵が 3 体出る */
	QuestMovePC(pA, nMapNo, 4, 5);
	DgTestTalk(pA, dwReception);
	DgTestSelPick(pA, 0);
	QuestMovePC(pA, nMapNo, 10, 6);
	DungeonEventStep(pA, pEntry);
	DWORD dwSpace = pA->dwSpaceID;
	PCHARINFO pBase = DgDefenseBaseOf(dwSpace);
	if (!pBase) {
		printf("dungeonmarch no base\n");
		return;
	}
	QuestMovePC(pA, nMapNo, 10, 3);
	for (int k = 0; k < 12 && (int)Enemies().size() < 3; k ++) Tick(1);
	std::vector<PCHARINFO> aE = Enemies();
	{
		int nProc = 0, nType1 = 0;
		for (size_t i = 0; i < aE.size(); i ++) {
			if (!aE[i]->pProc) {
				CCharProcBase CharProcBase;
				CharProcBase.SetProc(aE[i]);
			}
			nProc += (aE[i]->pProc != NULL);
			nType1 += (aE[i]->byNpcType == NPCTYPE_BATTLE1);
			Arm(aE[i]);
		}
		printf("dungeonmarch spawn enemies=%d proc=%d battle1=%d base-hp=%d/%d\n", (int)aE.size(), nProc, nType1, (int)pBase->wHP, (int)pBase->wMaxHP);
	}
	if (aE.size() < 3) {
		return;
	}

	/* 1. 壁を回って（通り道 y=12）拠点へ進み、拠点を殴って HP を減らす。重ならない・遠くのプレイヤーは追わない */
	{
		int nSteps = 0, nOverlap = 0, nGap = 0, nWallCross = 0;
		WORD wHPA = pA->wHP;
		auto Watch = [&]() {
			nOverlap += Overlap();
			std::vector<PCHARINFO> v = Enemies();
			for (size_t i = 0; i < v.size(); i ++) {
				if (v[i]->dwX == 15 && v[i]->dwY == 12) nGap ++;
				if (v[i]->dwX >= 16) nWallCross = 1;
			}
		};
		while (nSteps < 160 && pBase->wHP == pBase->wMaxHP) {
			Step(1, Watch);
			nSteps ++;
		}
		printf("dungeonmarch march base-hp-lower=%d (within 80s=%d) wall-crossed=%d via-gap=%d overlap=%d player-hp-kept=%d\n", (int)(pBase->wHP < pBase->wMaxHP),
			(int)(nSteps < 160), nWallCross, (int)(nGap > 0), nOverlap, (int)(pA->wHP == wHPA));
		Step(12, Watch);
		int nAdj = 0, nTarget = 0;
		std::vector<PCHARINFO> v = Enemies();
		for (size_t i = 0; i < v.size(); i ++) {
			if (abs((int)v[i]->dwX - (int)pBase->dwX) + abs((int)v[i]->dwY - (int)pBase->dwY) == 1) nAdj ++;
			nTarget += (v[i]->dwTargetTblID == pBase->dwTblID);
		}
		printf("dungeonmarch march +6s adjacent=%d target-base=%d overlap=%d base-hp-lower=%d player-hp-kept=%d\n", nAdj, nTarget, nOverlap, (int)(pBase->wHP < pBase->wMaxHP), (int)(pA->wHP == wHPA));
		pBase->wHP = pBase->wMaxHP;
	}

	/* 2. 列：拠点のとなりの敵の後ろの敵は待つ。2 秒たっても進めず、横に空きがあれば別経路で回り込んで拠点のとなりへ */
	{
		std::vector<PCHARINFO> v = Enemies();
		for (size_t i = 2; i < v.size(); i ++) DeleteChar(v[i]->dwTblID);
		v = Enemies();
		PCHARINFO pE1 = v[0], pE2 = v[1];
		Place(pE1, 19, 8);
		Place(pE2, 18, 8);
		Step(1, nullptr);
		int nOk1 = (pE1->dwTargetTblID == pBase->dwTblID) && (pE1->dwX == 19) && (pE1->dwY == 8);
		Step(2, nullptr);
		printf("dungeonmarch queue e1-hits-base=%d e2-waits=%d (%lu,%lu) overlap=%d\n", nOk1, (int)(pE2->dwX == 18 && pE2->dwY == 8), (unsigned long)pE2->dwX, (unsigned long)pE2->dwY, Overlap());
		Step(14, nullptr);
		int nAdj = abs((int)pE2->dwX - (int)pBase->dwX) + abs((int)pE2->dwY - (int)pBase->dwY) == 1;
		printf("dungeonmarch queue e2-detour adjacent=%d target-base=%d overlap=%d base-hp-lower=%d\n", nAdj, (int)(pE2->dwTargetTblID == pBase->dwTblID), Overlap(),
			(int)(pBase->wHP < pBase->wMaxHP));
		pBase->wHP = pBase->wMaxHP;
	}

	/* 3. 前をプレイヤーがふさぐ：そのプレイヤーを狙って戦う。離れたら（2 マス以上）行進に戻る */
	{
		std::vector<PCHARINFO> v = Enemies();
		for (size_t i = 1; i < v.size(); i ++) DeleteChar(v[i]->dwTblID);
		PCHARINFO pE = Enemies()[0];
		Place(pE, 16, 8);
		QuestMovePC(pA, nMapNo, 17, 8);
		pA->wHP = pA->wMaxHP = 500;
		Step(1, nullptr);
		int nTarget = (pE->dwTargetTblID == pA->dwTblID), nFace = (pE->byDirection == 3), nStay = (pE->dwX == 16);
		Step(6, nullptr);
		printf("dungeonmarch block target-player=%d facing=%d stayed=%d player-hp-lower=%d base-hp-kept=%d\n", nTarget, nFace, nStay, (int)(pA->wHP < pA->wMaxHP),
			(int)(pBase->wHP == pBase->wMaxHP));
		QuestMovePC(pA, nMapNo, 17, 5);
		WORD wHPA = pA->wHP;
		Step(2, nullptr);
		printf("dungeonmarch block released target-cleared=%d moved=%d pos=(%lu,%lu)\n", (int)(pE->dwTargetTblID == (DWORD)-1), (int)(pE->dwX > 16), (unsigned long)pE->dwX, (unsigned long)pE->dwY);
		Step(4, nullptr);
		printf("dungeonmarch block marching pos=(%lu,%lu) far-player-kept=%d\n", (unsigned long)pE->dwX, (unsigned long)pE->dwY, (int)(pA->wHP == wHPA));
	}

	/* 4. となりにプレイヤーと拠点がいる：拠点が先。殴られても（狙いをプレイヤーにされても）拠点を殴る */
	{
		PCHARINFO pE = Enemies()[0];
		Place(pE, 19, 8);
		QuestMovePC(pA, nMapNo, 19, 7);
		pA->wHP = pA->wMaxHP = 500;
		pE->dwTargetTblID = pA->dwTblID;
		pE->dwLastBattle = 1;
		Step(1, nullptr);
		int nTarget = (pE->dwTargetTblID == pBase->dwTblID);
		Step(8, nullptr);
		printf("dungeonmarch priority target-base=%d base-hp-lower=%d player-hp-kept=%d\n", nTarget, (int)(pBase->wHP < pBase->wMaxHP), (int)(pA->wHP == pA->wMaxHP));
		pBase->wHP = pBase->wMaxHP;
	}

	/* 5. プレイヤーは拠点を殴れない：ふつうの攻撃・範囲の特技（周囲・前方・前方横一列・画面内）・なぎ払い。
	      比べる相手に、行進しない敵（拠点防衛の敵ではない普通の敵）を拠点のとなりに置く（特技で倒れて消えても行進の敵は減らさない） */
	{
		auto MakeCtl = [&](int x, int y) -> DWORD {
			PCHARINFO p = new CHARINFO;
			ZeroMemory((PBYTE)p, sizeof(CHARINFO));
			p->byNpcType = NPCTYPE_BATTLE1;
			SetInitCharData(p);
			p->dwSpaceID = dwSpace;
			p->byGameState = GAMESTATE_NPC;
			p->byCharState = CHARSTATE_MOVE;
			p->dwMapID = pMap->dwID;
			p->dwX = p->dwXBack = p->dwStartX = (DWORD)x;
			p->dwY = p->dwYBack = p->dwStartY = (DWORD)y;
			p->dwParentTblID = (DWORD)-1;
			p->wGrpID = 5;
			p->wMaxHP = p->wHP = 60000;
			strcpy(p->szName, "比べる魔物");
			p = AddChar(p);
			p->pID = p->dwTblID + 1;
			return p->dwTblID;
		};
		/* 比べる敵に特技などが当たったか（倒れて消えた・HP が減った） */
		auto CtlHit = [&](DWORD dwTbl) -> int {
			PCHARINFO q = (dwTbl < (DWORD)g_aCharInfo.GetSize()) ? g_aCharInfo[dwTbl] : NULL;
			return (q == NULL || q->dwTblID != dwTbl || q->wHP < 60000) ? 1 : 0;
		};
		auto CtlDel = [&](DWORD dwTbl) {
			PCHARINFO q = (dwTbl < (DWORD)g_aCharInfo.GetSize()) ? g_aCharInfo[dwTbl] : NULL;
			if (q && q->dwTblID == dwTbl) DeleteChar(dwTbl);
		};
		PCHARINFO pE = Enemies()[0];
		DWORD dwCtl = MakeCtl(19, 9);
		Place(pE, 21, 8);
		QuestMovePC(pA, nMapNo, 19, 8);
		pA->byDirection = 3;
		pA->wMana = pA->wMaxMana = 9999;
		pA->dwTargetTblID = pBase->dwTblID;
		pA->dwLastBattle = 1;
		BOOL bAtk = BattleProcAtack(pA, pBase);
		pA->dwTargetTblID = (DWORD)-1;
		pA->dwLastBattle = 0;
		int nPlain = (!bAtk && pBase->wHP == pBase->wMaxHP);
		/* 拠点防衛の敵なら殴れる */
		pE->dwTargetTblID = pBase->dwTblID;
		BOOL bEnemyAtk = BattleProcAtack(pE, pBase);
		int nEnemyHit = (bEnemyAtk && pBase->wHP < pBase->wMaxHP);
		pBase->wHP = pBase->wMaxHP;
		Place(pE, 12, 3);
		printf("dungeonmarch player-attack plain-rejected=%d enemy-attack-works=%d\n", nPlain, nEnemyHit);

		static const BYTE abyType[] = { MAGICTARGETTYPE_CIRCLE, MAGICTARGETTYPE_FRONT, MAGICTARGETTYPE_FRONTSIDE, MAGICTARGETTYPE_SCREEN };
		static const char *apszType[] = { "circle", "front", "frontside", "screen" };
		for (int t = 0; t < 4; t ++) {
			int nFound = -1;
			for (int i = 1; i < g_aMagicInfo.GetSize() && nFound < 0; i ++) {
				PMAGICINFO pM = g_aMagicInfo[i];
				if (pM == NULL || pM->byType != MAGICTYPE_HPDOWN || pM->byTarget != MAGICTARGET_NPC || pM->byTargetType != abyType[t]) continue;
				if (abyType[t] == MAGICTARGETTYPE_CIRCLE && pM->byTargetArea < 1) continue;
				nFound = i;
			}
			/* 使う人が拠点の左（拠点は前・横）か、その下（前に比べる敵）から右向きに使う。拠点に当たらず、比べる敵には当たる */
			static const int aSetup[2][4] = { { 19, 8, 19, 9 }, { 19, 9, 20, 9 } };
			int anBase[2] = { 0, 0 }, anCtl[2] = { 0, 0 };
			for (int s = 0; s < 2 && nFound > 0; s ++) {
				CtlDel(dwCtl);
				dwCtl = MakeCtl(aSetup[s][2], aSetup[s][3]);
				QuestMovePC(pA, nMapNo, aSetup[s][0], aSetup[s][1]);
				pA->byDirection = 3;
				pBase->wHP = pBase->wMaxHP;
				pA->wMana = pA->wMaxMana;
				MagicProcItem(pA->dwTblID, (BYTE)nFound, (DWORD)-1);
				anBase[s] = (pBase->wHP == pBase->wMaxHP);
				anCtl[s] = CtlHit(dwCtl);
			}
			printf("dungeonmarch magic %s found=%d base-hp-kept=%d,%d control-enemy-hit=%d,%d\n", apszType[t], (int)(nFound > 0), anBase[0], anBase[1], anCtl[0], anCtl[1]);
		}

		/* なぎ払い：自分（20,7）の左右（20,8）（20,6）のうち、(20,8) が拠点。前（21,7）の敵に当たり、拠点には当たらない */
		int nSweep = -1;
		BYTE byTypeBack = 0;
		for (int i = 1; i < g_aItemInfo.GetSize() && nSweep < 0; i ++) {
			PITEMINFO pI = g_aItemInfo[i];
			if (pI && pI->byType == ITEMTYPE_WEAPON && pI->pData) nSweep = i;
		}
		if (nSweep > 0) {
			/* 手元にある武器の 1 つを、一時的になぎ払いにする */
			PITEMINFO_WEAPON pW = (PITEMINFO_WEAPON)g_aItemInfo[nSweep]->pData;
			byTypeBack = pW->byAtackType;
			pW->byAtackType = ATACKTYPE_SWEEP;
			pA->waItem[0] = (WORD)nSweep;
			pA->waEqu[0] = 0;
		}
		CtlDel(dwCtl);
		DWORD dwMain = MakeCtl(21, 7);
		DWORD dwSide = MakeCtl(20, 6);
		QuestMovePC(pA, nMapNo, 20, 7);
		pA->byDirection = 3;
		pBase->wHP = pBase->wMaxHP;
		pA->dwTargetTblID = dwMain;
		pA->dwLastBattle = 1;
		BattleProcCheckPC(pA);
		printf("dungeonmarch sweep found=%d main-hit=%d side-enemy-hit=%d base-hp-kept=%d\n", (int)(nSweep > 0), CtlHit(dwMain), CtlHit(dwSide), (int)(pBase->wHP == pBase->wMaxHP));
		if (nSweep > 0) ((PITEMINFO_WEAPON)g_aItemInfo[nSweep]->pData)->byAtackType = byTypeBack;
		pA->waItem[0] = 0;
		pA->waEqu[0] = 0xFFFF;
		CtlDel(dwMain);
		CtlDel(dwSide);
	}

	/* 6. 扉：通り道 (15,12) を扉にする。閉じている間は拠点へ行けず（壁の前で待つ）、組の空間で開くと通って拠点へ着く */
	{
		PCHARINFO pE = Enemies()[0];
		pMap->pwMap[15 + 12 * pMap->nCX] = wWall;
		int nOld = pMap->nEventCount;
		PEVENTINFO pNew = new EVENTINFO[nOld + 1];
		ZeroMemory(pNew, sizeof(EVENTINFO) * (nOld + 1));
		memcpy(pNew, pMap->pEvent, sizeof(EVENTINFO) * nOld);
		SAFE_DELETE_ARRAY(pMap->pEvent);
		pMap->pEvent = pNew;
		PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)new BYTE[sizeof(EVENTINFO_DOOR)];
		ZeroMemory(pDoor, sizeof(EVENTINFO_DOOR));
		pDoor->wGrpID = 11;
		pDoor->wBackGrpID = 22;
		pDoor->byType = EVENTDOOTTYPE_NONE;
		PEVENTINFO pDoorEv = &pMap->pEvent[nOld];
		pDoorEv->dwX = 15;
		pDoorEv->dwY = 12;
		pDoorEv->byType = EVENTTYPE_DOOR;
		pDoorEv->pData = (PBYTE)pDoor;
		pMap->nEventCount = nOld + 1;
		QuestMovePC(pA, nMapNo, 10, 3);
		Place(pE, 12, 8);
		pBase->wHP = pBase->wMaxHP;
		int nCross = 0;
		auto Watch = [&]() { if (pE->dwX >= 16) nCross = 1; };
		Step(60, Watch);
		printf("dungeonmarch door closed crossed=%d base-hp-kept=%d\n", nCross, (int)(pBase->wHP == pBase->wMaxHP));
		BOOL bOpen = SpaceDoorOpen(pMap->dwID, pDoorEv, pDoor, dwSpace);
		int nOpenOther = SpaceDoorIsOpen(pMap->dwID, pDoorEv, pDoor, SPACE_NORMAL);
		int nSteps = 0;
		while (nSteps < 160 && pBase->wHP == pBase->wMaxHP) {
			Step(1, Watch);
			nSteps ++;
		}
		printf("dungeonmarch door opened=%d normal-space-closed=%d crossed=%d arrived=%d\n", (int)bOpen, (int)(nOpenOther == 0), nCross, (int)(pBase->wHP < pBase->wMaxHP));
	}

	/* 後始末 */
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	SpaceSet(pA, SPACE_NORMAL);
	pA->dwSpaceID = SPACE_NORMAL;
	pA->dwTargetTblID = (DWORD)-1;
	pA->dwLastBattle = 0;
	pA->dwPartyID = 0;
	pA->byLevel = aBack.byLevel; pA->byPW = aBack.byPW; pA->wMaxHP = aBack.wMaxHP; pA->wHP = aBack.wHP;
	pA->wMaxMana = aBack.wMaxMana; pA->wMana = aBack.wMana; pA->dwGold = aBack.dwGold; pA->dwExp = aBack.dwExp;
	pA->byCharState = aBack.byCharState; pA->byAdminLV = aBack.byAdminLV;
	memcpy(pA->waItem, aBack.waItem, sizeof(pA->waItem));
	memcpy(pA->waEqu, aBack.waEqu, sizeof(pA->waEqu));
	QuestMovePC(pA, (int)aBack.dwMapID, (int)aBack.dwX, (int)aBack.dwY);
	QuestSyncAll();
	/* 全員が出たので、次の周期処理で組が終わる */
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	DungeonProcNow(s_dwFakeNow + 1000);
	s_dwFakeNow += 1000;
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT && p->dwSpaceID != SPACE_NORMAL) DeleteChar(p->dwTblID);
	}
	for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO p = g_aCharInfo[i];
		if (p && p->dwParentTblID == dwPut && p->byGameState == GAMESTATE_NPC && p->byNpcType != NPCTYPE_INIT) DeleteChar(p->dwTblID);
	}
	DeleteChar(dwReception);
	DeleteChar(dwPut);
	for (int i = g_aStTmpInfo.GetSize() - 1; i >= nSt1; i --) {
		delete g_aStTmpInfo[i];
		g_aStTmpInfo.RemoveAt(i);
	}
	for (int i = 0; i < pMap->nEventCount; i ++) {
		SAFE_DELETE_ARRAY(pMap->pEvent[i].pData);
	}
	SAFE_DELETE_ARRAY(pMap->pEvent);
	pMap->pEvent = pEventBack;
	pMap->nEventCount = nEventBack;
	pMap->nProcEvent = 0;
	pMap->byFlg = byFlgBack;
	g_nProcMap = 0;
	memcpy(pMap->pwMap, &aMapBack[0], sizeof(WORD) * aMapBack.size());
	g_aSubaGameInfo.RemoveAt(byID);
	g_nProcSubGame = 0;
	MockTrace(0);
	g_dwTime = dwTimeBack;
	s_dwFakeNow = dwNowBack + 400000;
	printf("dungeonmarch end runs=%d sel=%d\n", DungeonRunCount(), DungeonSelCount());
}

/* ------------------------------------------------------------------------- */
/* L3. 戦闘中・復帰中のマップ移動：別マップでは待ち、そろったら列へ戻る */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* AT. ついていく列の途中の人がログアウトなどで抜ける時のつなぎ直し			 */
/*    A←B←C←D で B が抜けると A←C←D。先頭・最後尾が抜けた時は従来どおり		 */
/* ------------------------------------------------------------------------- */
static void TestTailMidLeave(const std::vector<int> &aLogin)
{
	printf("== AT. Tail mid leave ==\n");
	if (aLogin.size() < 4) {
		return;
	}
	PCHARINFO apChar[4];
	DWORD adwBack[4][5];
	for (int i = 0; i < 4; i ++) {
		PCHARINFO p = apChar[i] = g_aCharInfo[aLogin[i]];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
	}
	auto setLine = [&]() {
		for (int i = 0; i < 4; i ++) {
			PCHARINFO p = apChar[i];
			p->dwMapID = apChar[0]->dwMapID;
			p->dwTailTblID = (i > 0) ? apChar[i - 1]->dwTblID : (DWORD)-1;
			p->dwTailBackTblID = (i < 3) ? apChar[i + 1]->dwTblID : (DWORD)-1;
		}
	};
	auto idx = [&](DWORD dwTblID) -> int {
		for (int i = 0; i < 4; i ++) if (apChar[i]->dwTblID == dwTblID) return i;
		return -1;
	};
	auto show = [&](const char *pszName) {
		printf("%s:", pszName);
		for (int i = 0; i < 4; i ++) {
			printf(" front[%d]=%d back[%d]=%d", i, idx(apChar[i]->dwTailTblID), i, idx(apChar[i]->dwTailBackTblID));
		}
		printf("\n");
	};
	/* 抜ける時の処理（OnDecPlayer と同じ順） */
	auto leave = [&](int n) {
		MockTrace(1);
		TailRelinkLeave(apChar[n]);
		CharTargetCancelPC(apChar[n]);
		MockTrace(0);
	};

	/* (a) 2番目が抜ける：A←C←D */
	setLine();
	leave(1);
	show("mid-leave 2nd");
	/* (b) 3番目が抜ける：A←B←D */
	setLine();
	leave(2);
	show("mid-leave 3rd");
	/* (c) 先頭が抜ける：B←C←D の先頭は B になる（つなぎ直しなし） */
	setLine();
	leave(0);
	show("mid-leave head");
	/* (d) 最後尾が抜ける：A←B←C */
	setLine();
	leave(3);
	show("mid-leave tail");
	/* (e) 前後が食い違っていたら触らない */
	setLine();
	apChar[2]->dwTailTblID = apChar[0]->dwTblID;
	TailRelinkLeave(apChar[1]);
	show("mid-leave mismatch");
	/* (f) 続けて2人抜ける：A←D */
	setLine();
	leave(1);
	leave(2);
	show("mid-leave twice");

	for (int i = 0; i < 4; i ++) {
		PCHARINFO p = apChar[i];
		p->dwMapID = adwBack[i][0]; p->dwX = adwBack[i][1]; p->dwY = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3]; p->dwTailBackTblID = adwBack[i][4];
	}
}

/* ------------------------------------------------------------------------- */
/* AU. マップ間移動で着いた直後、着いたマスにあるマップ移動に反応しない		 */
/*    管理人室の出口(始まりの町 (48,74))→マップ3 (10,9) は、着いたマスが		 */
/*    別のマップ移動(→始まりの町)の上。ついていく人の古い問い合わせで			 */
/*    すぐ飛ばされていた。着いてしばらくは捨て、動いた後・時間が過ぎた後は動く	 */
/* ------------------------------------------------------------------------- */
static void TailArriveCheck(PCHARINFO p, BYTE byData)
{
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_CHECKEVENT;
	Para.pID = p->pID;
	Para.dwTblID = p->dwTblID;
	Para.byData = byData;
	TestRecv(p, &Para, sizeof(Para));
}

static void TestTailArrive(const std::vector<int> &aLogin)
{
	printf("== AU. Tail arrive ==\n");
	if (aLogin.size() < 2) {
		return;
	}
	PMAPINFO pTown = g_aMapInfo[GetMapNo(0)];
	PEVENTINFO pEvent = NULL;
	for (int i = 0; i < pTown->nEventCount; i ++) {
		PEVENTINFO p = &pTown->pEvent[i];
		if (p->byType == EVENTTYPE_MAPCHG && p->pData && p->dwX == 48 && p->dwY == 74) {
			pEvent = p;
		}
	}
	printf("tailarrive setup=%d\n", pEvent ? 1 : 0);
	if (!pEvent) {
		return;
	}
	PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
	BYTE byRangeBack = pInfo->byRandomRange;
	pInfo->byRandomRange = 0;
	printf("tailarrive dest map=%u pos=(%u,%u)\n", pInfo->dwMapID, pInfo->dwX, pInfo->dwY);

	PCHARINFO apChar[2] = { g_aCharInfo[aLogin[0]], g_aCharInfo[aLogin[1]] };
	DWORD adwBack[2][5];
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		adwBack[i][0] = p->dwMapID; adwBack[i][1] = p->dwX; adwBack[i][2] = p->dwY;
		adwBack[i][3] = p->dwTailTblID; adwBack[i][4] = p->dwTailBackTblID;
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = pTown->dwID;
		p->dwX = p->dwXBack = pEvent->dwX;
		p->dwY = p->dwYBack = pEvent->dwY + i;
		AddTblNoPC((int)p->dwTblID);
	}
	apChar[0]->dwTailTblID = (DWORD)-1;
	apChar[0]->dwTailBackTblID = apChar[1]->dwTblID;
	apChar[1]->dwTailTblID = apChar[0]->dwTblID;
	apChar[1]->dwTailBackTblID = (DWORD)-1;

	/* 先頭が出口に乗る → 後ろの人も着いたマスへ */
	EventProc(apChar[0], pEvent, 0);
	for (int i = 0; i < 2; i ++) {
		printf("tailarrive moved %d map=%u pos=(%u,%u)\n", i, apChar[i]->dwMapID, apChar[i]->dwX, apChar[i]->dwY);
	}
	/* 着いた直後の古い問い合わせ（歩き終わり）は捨てる */
	s_dwFakeNow += 200;
	g_dwTime = s_dwFakeNow;
	TailArriveCheck(apChar[1], 0);
	printf("tailarrive stale follower map=%u pos=(%u,%u)\n", apChar[1]->dwMapID, apChar[1]->dwX, apChar[1]->dwY);
	TailArriveCheck(apChar[0], 0);
	printf("tailarrive stale leader map=%u pos=(%u,%u)\n", apChar[0]->dwMapID, apChar[0]->dwX, apChar[0]->dwY);
	/* 時間が過ぎれば、乗っているマス（まだ動いていない）のマップ移動は動く */
	s_dwFakeNow += 2000;
	g_dwTime = s_dwFakeNow;
	TailArriveCheck(apChar[1], 0);
	printf("tailarrive later follower map=%u pos=(%u,%u)\n", apChar[1]->dwMapID, apChar[1]->dwX, apChar[1]->dwY);

	pInfo->byRandomRange = byRangeBack;
	for (int i = 0; i < 2; i ++) {
		PCHARINFO p = apChar[i];
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = adwBack[i][0];
		p->dwX = p->dwXBack = adwBack[i][1];
		p->dwY = p->dwYBack = adwBack[i][2];
		p->dwTailTblID = adwBack[i][3];
		p->dwTailBackTblID = adwBack[i][4];
		p->dwTimeMapMove = 0;
		AddTblNoPC((int)p->dwTblID);
	}
}

static void TestTailBattleMapMove(const std::vector<int> &aLogin)
{
	printf("== L3. Tail battle mapmove ==\n");
	if (aLogin.size() < 3) return;
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	PEVENTINFO pEvent = NULL;
	for (int i = 0; i < pPort->nEventCount; i ++) {
		PEVENTINFO p = &pPort->pEvent[i];
		if (p->byType == EVENTTYPE_MAPCHG && p->pData && p->dwX == 40 && p->dwY == 54) pEvent = p;
	}
	int nX = -1, nY = -1;
	for (int y = 1; y + 7 < pPort->nCY && nX < 0; y ++) {
		for (int x = 1; x + 1 < pPort->nCX && nX < 0; x ++) {
			bool bOpen = true;
			for (int dy = -1; dy <= 6 && bOpen; dy ++) {
				for (int dx = -1; dx <= 1 && bOpen; dx ++) {
					if (!CheckCharBlock(pPort->dwID, x + dx, y + dy) || CheckEventBlock(pPort->dwID, x + dx, y + dy)) bOpen = false;
				}
			}
			if (bOpen) { nX = x; nY = y; }
		}
	}
	printf("tailbattlemap setup=%d\n", pEvent && nX >= 0 ? 1 : 0);
	if (!pEvent || nX < 0) return;
	PEVENTINFO_MAPCHG pInfo = (PEVENTINFO_MAPCHG)pEvent->pData;
	BYTE byRangeBack = pInfo->byRandomRange;
	pInfo->byRandomRange = 0;
	PCHARINFO apChar[3] = { g_aCharInfo[aLogin[0]], g_aCharInfo[aLogin[1]], g_aCharInfo[aLogin[2]] };
	PCHARINFO pA = apChar[0], pB = apChar[1], pC = apChar[2];
	/* 配列などを所有する CHARINFO 全体は複製せず、変更する欄だけを保存する */
	struct {
		DWORD dwMapID, dwX, dwY, dwTailTblID, dwTailBackTblID, dwSpaceID;
		DWORD dwTargetTblID, dwTargetTblID2, dwTargetTblID3;
		BYTE byCharState, byDirection;
		WORD wHP;
	} aBack[3];
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		aBack[i] = { p->dwMapID, p->dwX, p->dwY, p->dwTailTblID, p->dwTailBackTblID, p->dwSpaceID,
			p->dwTargetTblID, p->dwTargetTblID2, p->dwTargetTblID3, p->byCharState, p->byDirection, p->wHP };
	}
	auto place = [&](PCHARINFO p, DWORD dwMap, DWORD x, DWORD y) {
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = dwMap;
		p->dwX = p->dwXBack = x;
		p->dwY = p->dwYBack = y;
		AddTblNoPC((int)p->dwTblID);
	};
	auto reset = [&]() {
		for (int i = 0; i < 3; i ++) TailReturnForget(apChar[i]->dwTblID);
		for (int i = 0; i < 3; i ++) {
			PCHARINFO p = apChar[i];
			MercenarySetBattleMode(p, FALSE);
			p->byCharState = CHARSTATE_MOVE;
			p->wHP = max(p->wHP, (WORD)10);
			p->dwSpaceID = pA->dwSpaceID;
			p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
			p->dwTailTblID = i ? apChar[i - 1]->dwTblID : (DWORD)-1;
			p->dwTailBackTblID = i < 2 ? apChar[i + 1]->dwTblID : (DWORD)-1;
			place(p, pPort->dwID, nX, nY + i);
		}
	};
	auto move = [&](PCHARINFO p) { EventProc(p, pEvent, 0); };
	auto linked = [&]() {
		return pA->dwTailBackTblID == pB->dwTblID && pB->dwTailTblID == pA->dwTblID &&
			pB->dwTailBackTblID == pC->dwTblID && pC->dwTailTblID == pB->dwTblID;
	};

	/* 全員が戦闘中のまま出口を通り、MAPINFO 後の同じモードの再通知も受ける */
	reset();
	TailBattleSend(pA, 1);
	TailBattleSend(pB, 1);
	TailBattleSend(pC, 1);
	move(pA); move(pB); move(pC);
	for (int i = 0; i < 3; i ++) TailBattleSend(apChar[i], 1);
	TailBattleSend(pB, 0); TailBattleSend(pC, 0);
	TailChaseAdvance(apChar, 3, 1000);
	printf("tailbattlemap battle preserved=%d wait=%d\n", IsCharBattleMode(pA) ? 1 : 0,
		pB->dwTailTblID == (DWORD)-1 && pC->dwTailTblID == (DWORD)-1);
	TailBattleSend(pA, 0);
	printf("tailbattlemap all arrived ordered=%d\n", linked() ? 1 : 0);

	/* 先頭が先に解除。別マップの人をワープさせず、30秒を超えても到着を待つ */
	reset();
	TailBattleSend(pA, 1);
	move(pA);
	TailBattleSend(pA, 0);
	TailChaseAdvance(apChar, 3, 31000);
	printf("tailbattlemap late waiting=%d active=%d\n", pB->dwMapID == pPort->dwID && pC->dwMapID == pPort->dwID,
		TailChaseIsActive(pB) || TailChaseIsActive(pC));
	move(pB); move(pC);
	/* タイマーが後ろの人から巡回しても、元の順番で復帰を始める */
	PCHARINFO apReverse[3] = { pC, pB, pA };
	TailChaseAdvance(apReverse, 3, 1000);
	printf("tailbattlemap late arrived ordered=%d\n", linked() ? 1 : 0);

	/* 真ん中の人だけ戦闘中。通常の列 A→C は出口で外れず、B は後から最後尾へ戻る */
	reset();
	TailBattleSend(pB, 1);
	move(pA);
	printf("tailbattlemap normal chain=%d same=%d\n", pC->dwTailTblID == pA->dwTblID && pA->dwTailBackTblID == pC->dwTblID,
		pA->dwMapID == pC->dwMapID);
	TailBattleSend(pB, 0);
	move(pB);
	TailChaseAdvance(apChar, 3, 1000);
	printf("tailbattlemap member arrived end=%d\n", pB->dwTailTblID == pC->dwTblID && pC->dwTailBackTblID == pB->dwTblID);

	/* 後ろの人が先に別マップで解除しても、先頭が来るまで自由に動ける */
	reset();
	TailBattleSend(pA, 1);
	TailBattleSend(pB, 1);
	move(pB);
	TailBattleSend(pB, 0);
	TailBattleSend(pA, 0);
	TailChaseAdvance(apChar, 3, 3000);
	printf("tailbattlemap member first waiting=%d\n", pB->dwTailTblID == (DWORD)-1 && !TailChaseIsActive(pB));
	move(pA);
	TailChaseAdvance(apChar, 3, 2000);
	printf("tailbattlemap member first returned=%d\n", pB->dwTailTblID == pC->dwTblID);

	/* B が歩いて復帰中に、先頭 A と列につながっている C が出口を通る */
	reset();
	TailBattleSend(pB, 1);
	place(pB, pPort->dwID, nX, nY + 6);
	TailBattleSend(pB, 0);
	TailChaseAdvance(apChar, 3, 300);
	printf("tailbattlemap chase head start=%d walked=%d\n", TailChaseIsActive(pB),
		pB->dwX != (DWORD)nX || pB->dwY != (DWORD)(nY + 6));
	MockTrace(1);
	move(pA);
	MockTrace(0);
	char *pszFade = MockCmdData(1001, CMD_FADEINOUT);
	printf("tailbattlemap chase head arrived=%d fade=[%s]\n", pB->dwMapID != pPort->dwID &&
		pB->dwMapID == pA->dwMapID && pB->dwX == pA->dwX && pB->dwY == pA->dwY, pszFade);
	free(pszFade);
	TailChaseAdvance(apChar, 3, 1000);
	printf("tailbattlemap chase head returned=%d\n", pB->dwTailTblID == pC->dwTblID &&
		pC->dwTailBackTblID == pB->dwTblID && !TailChaseIsActive(pB));

	/* B が先に出口を通った場合は、A が着くまで追跡を保留する */
	reset();
	TailBattleSend(pB, 1);
	place(pB, pPort->dwID, nX, nY + 6);
	TailBattleSend(pB, 0);
	printf("tailbattlemap chase member start=%d\n", TailChaseIsActive(pB));
	move(pB);
	TailChaseAdvance(apChar, 3, 31000);
	printf("tailbattlemap chase member paused=%d separate=%d\n", !TailChaseIsActive(pB), pB->dwMapID != pA->dwMapID);
	move(pA);
	TailChaseAdvance(apChar, 3, 2000);
	printf("tailbattlemap chase member returned=%d\n", pB->dwTailTblID == pC->dwTblID);

	/* ワープのために暗転中の B も同行し、まだ戦闘中の C は元のマップに残る */
	reset();
	TailBattleSend(pA, 1);
	TailBattleSend(pC, 1);
	place(pB, pPort->dwID, pPort->nCX - 1, pPort->nCY - 1);
	MockTrace(1);
	TailBattleSend(pA, 0);
	move(pA);
	MockTrace(0);
	pszFade = MockCmdData(1001, CMD_FADEINOUT);
	printf("tailbattlemap fading arrived=%d battle stayed=%d fade=[%s]\n", pB->dwMapID == pA->dwMapID,
		pC->dwMapID == pPort->dwID && IsCharBattleMode(pC), pszFade);
	free(pszFade);
	TailChaseAdvance(apChar, 3, 2000);
	printf("tailbattlemap fading returned=%d\n", pB->dwTailTblID == pA->dwTblID && !TailChaseIsActive(pB));

	/* 先頭が切断された場合は覚えを消し、後から同じマップへ来ても復帰しない */
	reset();
	TailBattleSend(pA, 1);
	move(pA);
	TailBattleSend(pA, 0);
	TailReturnForget(pA->dwTblID);
	move(pB); move(pC);
	TailChaseAdvance(apChar, 3, 2000);
	printf("tailbattlemap forgotten detached=%d\n", pB->dwTailTblID == (DWORD)-1 && pC->dwTailTblID == (DWORD)-1);

	pInfo->byRandomRange = byRangeBack;
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		TailReturnForget(p->dwTblID);
		MercenarySetBattleMode(p, FALSE);
		p->dwTailTblID = aBack[i].dwTailTblID;
		p->dwTailBackTblID = aBack[i].dwTailBackTblID;
		p->dwSpaceID = aBack[i].dwSpaceID;
		p->byCharState = aBack[i].byCharState;
		p->wHP = aBack[i].wHP;
		p->byDirection = aBack[i].byDirection;
		p->dwTargetTblID = aBack[i].dwTargetTblID;
		p->dwTargetTblID2 = aBack[i].dwTargetTblID2;
		p->dwTargetTblID3 = aBack[i].dwTargetTblID3;
		place(p, aBack[i].dwMapID, aBack[i].dwX, aBack[i].dwY);
	}
}

/* ------------------------------------------------------------------------- */
/* L4. 戦闘で列を離れて戻る途中（追いかけ中）に、先頭か本人が空間を移る			 */
/*    （番外編ダンジョンの受付→入口など。SpaceSet）							 */
/*    追いかけが残ると、本人が動けない(位置を送り返される)まま戻れなくなる		 */
/* ------------------------------------------------------------------------- */
static void TestTailSpaceMove(const std::vector<int> &aLogin)
{
	printf("== L4. Tail battle space move ==\n");
	if (aLogin.size() < 3) return;
	PMAPINFO pPort = g_aMapInfo[GetMapNo(14)];
	int nX = -1, nY = -1;
	for (int y = 1; y + 7 < pPort->nCY && nX < 0; y ++) {
		for (int x = 1; x + 1 < pPort->nCX && nX < 0; x ++) {
			bool bOpen = true;
			for (int dy = -1; dy <= 6 && bOpen; dy ++) {
				for (int dx = -1; dx <= 1 && bOpen; dx ++) {
					if (!CheckCharBlock(pPort->dwID, x + dx, y + dy) || CheckEventBlock(pPort->dwID, x + dx, y + dy)) bOpen = false;
				}
			}
			if (bOpen) { nX = x; nY = y; }
		}
	}
	printf("tailspace setup=%d\n", nX >= 0 ? 1 : 0);
	if (nX < 0) return;
	PCHARINFO apChar[3] = { g_aCharInfo[aLogin[0]], g_aCharInfo[aLogin[1]], g_aCharInfo[aLogin[2]] };
	PCHARINFO pA = apChar[0], pB = apChar[1], pC = apChar[2];
	/* 変更する欄だけを保存する */
	struct {
		DWORD dwMapID, dwX, dwY, dwTailTblID, dwTailBackTblID, dwSpaceID;
		DWORD dwTargetTblID, dwTargetTblID2, dwTargetTblID3;
		BYTE byCharState, byDirection;
		WORD wHP;
	} aBack[3];
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		aBack[i] = { p->dwMapID, p->dwX, p->dwY, p->dwTailTblID, p->dwTailBackTblID, p->dwSpaceID,
			p->dwTargetTblID, p->dwTargetTblID2, p->dwTargetTblID3, p->byCharState, p->byDirection, p->wHP };
	}
	auto place = [&](PCHARINFO p, DWORD dwMap, DWORD x, DWORD y) {
		DeleteTblNoPC((int)p->dwTblID);
		p->dwMapID = dwMap;
		p->dwX = p->dwXBack = x;
		p->dwY = p->dwYBack = y;
		AddTblNoPC((int)p->dwTblID);
	};
	/* A-B-C の列（縦に並ぶ）に戻す。B を戦闘モードにして列から離し、離れた所で解除＝追いかけ中にする */
	auto reset = [&]() {
		for (int i = 0; i < 3; i ++) TailReturnForget(apChar[i]->dwTblID);
		for (int i = 0; i < 3; i ++) {
			PCHARINFO p = apChar[i];
			MercenarySetBattleMode(p, FALSE);
			p->byCharState = CHARSTATE_MOVE;
			p->wHP = max(p->wHP, (WORD)10);
			p->dwSpaceID = SPACE_NORMAL;
			p->dwTargetTblID = p->dwTargetTblID2 = p->dwTargetTblID3 = (DWORD)-1;
			p->dwTailTblID = i ? apChar[i - 1]->dwTblID : (DWORD)-1;
			p->dwTailBackTblID = i < 2 ? apChar[i + 1]->dwTblID : (DWORD)-1;
			place(p, pPort->dwID, nX, nY + i);
		}
	};
	auto startChase = [&]() {
		TailBattleSend(pB, 1);
		place(pB, pPort->dwID, nX + 1, nY + 5);
		TailBattleSend(pB, 0);
	};
	/* B から動く要求（1マス横）が受け付けられたか */
	auto keyMove = [&]() {
		CMDPARA_CHARPOS Pos;
		ZeroMemory(&Pos, sizeof(Pos));
		Pos.byCmd = CMD_CHARPOS;
		Pos.pID = pB->pID;
		Pos.dwTblID = pB->dwTblID;
		Pos.dwMapID = pB->dwMapID;
		Pos.byX = (BYTE)(pB->dwX - 1);
		Pos.byY = (BYTE)pB->dwY;
		Pos.byDirection = 2;
		DWORD dwXBack = pB->dwX;
		TestRecv(pB, &Pos, sizeof(Pos));
		return pB->dwX != dwXBack;
	};

	/* 1. 先頭が空間を移る：B の追いかけが残らず、B は動ける */
	reset();
	startChase();
	printf("tailspace head chase start=%d\n", TailChaseIsActive(pB) ? 1 : 0);
	SpaceSet(pA, 1, FALSE);
	printf("tailspace head moved active=%d\n", TailChaseIsActive(pB) ? 1 : 0);
	TailChaseAdvance(apChar, 3, 3000);
	printf("tailspace head after active=%d free=%d\n", TailChaseIsActive(pB) ? 1 : 0, keyMove() ? 1 : 0);
	/* 先頭が戻ってきて B が戦闘モードを解除しても、古い覚えで追いかけ直さない */
	SpaceSet(pA, SPACE_NORMAL, FALSE);
	TailBattleSend(pB, 1);
	TailBattleSend(pB, 0);
	printf("tailspace head back active=%d\n", TailChaseIsActive(pB) ? 1 : 0);

	/* 2. 追いかけ中の本人が空間を移る：追いかけが残らず、先頭が戻るのを待たずに動ける */
	reset();
	startChase();
	SpaceSet(pB, 1, FALSE);
	printf("tailspace self moved active=%d\n", TailChaseIsActive(pB) ? 1 : 0);
	SpaceSet(pB, SPACE_NORMAL, FALSE);
	TailChaseAdvance(apChar, 3, 3000);
	printf("tailspace self back active=%d free=%d\n", TailChaseIsActive(pB) ? 1 : 0, keyMove() ? 1 : 0);

	/* 3. 先頭が空間を移ってから、先の空間の終了で戻っても、後ろの人は動ける（戻る途中だった人の残り） */
	reset();
	startChase();
	SpaceSet(pA, 1, FALSE);
	SpaceSet(pA, SPACE_NORMAL, FALSE);
	TailChaseAdvance(apChar, 3, 31000);
	printf("tailspace head round trip active=%d free=%d\n", TailChaseIsActive(pB) ? 1 : 0, keyMove() ? 1 : 0);

	/* 4. 先頭が別マップの空間へ移ると、戦闘モードで列を離れた B は戻り先を覚えたまま待つ。
	      その間に B が別の空間へ移って遊び、先頭が元のマップへ戻ると、B が同じ空間に戻った時に
	      古い覚えで列を追いかけ始めて動けなくなる（空間を移る時に覚えを消す） */
	{
		PMAPINFO pMap2 = NULL;
		int nX2 = -1, nY2 = -1;
		for (int m = 0; m < g_aMapInfo.GetSize() && !pMap2; m ++) {
			PMAPINFO p2 = g_aMapInfo[m];
			if (!p2 || p2->dwID == pPort->dwID || !p2->pwMap) continue;
			for (int y = 2; y + 2 < p2->nCY && !pMap2; y ++) {
				for (int x = 2; x + 2 < p2->nCX && !pMap2; x ++) {
					bool bOpen = true;
					for (int dy = -1; dy <= 1 && bOpen; dy ++) {
						for (int dx = -1; dx <= 1 && bOpen; dx ++) {
							if (!CheckCharBlock(p2->dwID, x + dx, y + dy) || CheckEventBlock(p2->dwID, x + dx, y + dy)) bOpen = false;
						}
					}
					if (bOpen) { pMap2 = p2; nX2 = x; nY2 = y; }
				}
			}
		}
		printf("tailspace map2 found=%d\n", pMap2 ? 1 : 0);
		if (pMap2) {
			reset();
			TailBattleSend(pB, 1);
			place(pA, pMap2->dwID, nX2, nY2);
			SpaceSet(pA, 1, FALSE);
			TailBattleSend(pB, 0);
			PCHARINFO apRun[3] = { pA, pB, pC };
			TailChaseAdvance(apRun, 3, 1000);
			/* B は別の組の空間へ移る */
			place(pB, pMap2->dwID, nX2 + 1, nY2);
			SpaceSet(pB, 2, FALSE);
			/* 先の組が終わって先頭が戻り、B も同じ空間に戻る */
			place(pA, pPort->dwID, nX, nY);
			SpaceSet(pA, SPACE_NORMAL, FALSE);
			place(pB, pPort->dwID, nX + 1, nY + 5);
			SpaceSet(pB, SPACE_NORMAL, FALSE);
			TailChaseAdvance(apRun, 3, 1000);
			printf("tailspace stale return active=%d free=%d\n", TailChaseIsActive(pB) ? 1 : 0, keyMove() ? 1 : 0);
		}
	}

	reset();
	for (int i = 0; i < 3; i ++) {
		PCHARINFO p = apChar[i];
		TailReturnForget(p->dwTblID);
		MercenarySetBattleMode(p, FALSE);
		p->dwTailTblID = aBack[i].dwTailTblID;
		p->dwTailBackTblID = aBack[i].dwTailBackTblID;
		p->dwSpaceID = aBack[i].dwSpaceID;
		p->byCharState = aBack[i].byCharState;
		p->wHP = aBack[i].wHP;
		p->byDirection = aBack[i].byDirection;
		p->dwTargetTblID = aBack[i].dwTargetTblID;
		p->dwTargetTblID2 = aBack[i].dwTargetTblID2;
		p->dwTargetTblID3 = aBack[i].dwTargetTblID3;
		place(p, aBack[i].dwMapID, aBack[i].dwX, aBack[i].dwY);
	}
}


/* ------------------------------------------------------------------------- */
/* AV. 落ち物パズルの台（台詞 "@p<コイン>,<枚数>,<側>,<距離>" の NPC）			 */
/*     コインが無ければ座れない・あれば枚数ぶん減って座る・座っている間は歩けない・	 */
/*     キー入力で位置だけ／盤面ごと送る・落下は経過時間で進む・ゲームオーバーで得点を出して立つ・	 */
/*     席を立つ／離れる／切断で片付く・使用中の台には座れない					 */
/* ------------------------------------------------------------------------- */
/* 台 NPC を、台でなくす（台詞を空にする）。NearNpcClose は途中までしか消せない事があり、台 NPC が残ると
   待機中の台として次のテストの人に見えてしまうため、片付けの前に呼ぶ */
static void PuzzleRetireNpcs(void)
{
	for (size_t i = 0; i < s_aNearNpc.size(); i ++) {
		PCHARINFO pNpc = s_aNearNpc[i];
		if (PuzzleIsTableNpc(pNpc)) {
			delete [] pNpc->pszNpcSpeech;
			pNpc->pszNpcSpeech = new char[1];
			pNpc->pszNpcSpeech[0] = '\0';
		}
	}
}

static DWORD s_dwPuzzleSerial = 0;		/* 最後に送ったキーの連番（クライアントと同じく 1 から） */
static void PuzzleKeySend(PCHARINFO p, BYTE byKey)
{
	CMDPARA_PUZZLEKEY Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_PUZZLEKEY;
	Para.byData = byKey;
	Para.dwTblID = p->dwTblID;
	Para.dwSerial = ++ s_dwPuzzleSerial;
	TestRecv(p, &Para, sizeof(Para));
}

/* その人が最後に受けた、台 dwNpc の CMD_PUZZLESTATE の byOver（パケットが無ければ -1）。*pnIdle=byIdle・*pnCells=盤面のブロック数・*pdwAck=dwAck */
static void PuzzlePackets(int nConn, std::vector<std::vector<BYTE> > &aOut);
static int PuzzleOverFlag(int nConn, DWORD dwNpc, int *pnIdle = NULL, int *pnCells = NULL, DWORD *pdwAck = NULL)
{
	std::vector<std::vector<BYTE> > aP;
	PuzzlePackets(nConn, aP);
	for (int i = (int)aP.size() - 1; i >= 0; i --) {
		if (aP[i].size() < sizeof(CMDPARA_PUZZLESTATE)) continue;
		const CMDPARA_PUZZLESTATE *pSt = (const CMDPARA_PUZZLESTATE *)&aP[i][0];
		if (pSt->dwNpc != dwNpc) continue;
		if (pnIdle) *pnIdle = pSt->byIdle;
		if (pnCells) {
			*pnCells = 0;
			for (size_t j = sizeof(CMDPARA_PUZZLESTATE); j < aP[i].size(); j ++) if (aP[i][j]) (*pnCells) ++;
		}
		if (pdwAck) *pdwAck = pSt->dwAck;
		return pSt->byKind == PUZZLESTATE_NONE ? -2 : pSt->byOver;
	}
	return -1;
}

static int PuzzleCoinCount(PCHARINFO p, WORD wItem)
{
	int n = 0;
	for (int i = 0; i < MAX_MYITEM; i ++) if (p->waItem[i] == wItem) n ++;
	return n;
}

/* 持ち物の byNo 番目を自分に使う（CMD_USEITEM。クライアントの道具メニューの「使う」と同じ通信） */
static void PuzzleUseItemSend(PCHARINFO p, BYTE byNo)
{
	CMDPARA_DATA4 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_USEITEM;
	Para.dwTblID = p->dwTblID;
	Para.byData = byNo;
	Para.dwData = p->dwTblID;
	TestRecv(p, &Para, sizeof(Para));
}

/* 台のコインを持っていれば、その 1 枚目を使う（使えたら TRUE。持っていなければ何もせず FALSE） */
static BOOL PuzzleUseCoin(PCHARINFO p, WORD wCoin)
{
	for (int i = 0; i < MAX_MYITEM; i ++) {
		if (p->waItem[i] == wCoin) {
			PuzzleUseItemSend(p, (BYTE)i);
			return TRUE;
		}
	}
	return FALSE;
}

/* S キーで台に話しかけた（今いる場所のまま送る） */
static void PuzzleKeyS(PCHARINFO p, PCHARINFO pNpc)
{
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_TALKRAND;
	Para.dwTblID = pNpc->dwTblID;
	TestRecv(p, &Para, sizeof(Para));
}

/* 台に座ろうとする（今いる場所のまま。座れるのは台の座る場所のマスの上だけ）。
   有料の台はコマンドの道具からコインを使う（持っていなければ何も使わず S キー＝案内だけ）。無料の台と台でない NPC は S キー */
static void PuzzleTalkRaw(PCHARINFO p, PCHARINFO pNpc)
{
	int anVal[PUZZLE_PARAM_COUNT];
	if (PuzzleIsTableNpc(pNpc)) {
		PuzzleParseSpeech(pNpc->pszNpcSpeech, anVal);
		if ((anVal[0] > 0) && (anVal[1] > 0) && (PuzzleCoinCount(p, (WORD)anVal[0]) > 0)) {
			/* 立っているマスが、この台だけの座る場所なら、本当に道具のコインを使う（本番の通り）。
			   テストでは台を同じマスに何台も置くので、ほかの台と座る場所が重なる時（または別の台の座る場所の時）は、
			   使う台を決めて PuzzleSit にコインを使った扱いで渡す（本番では座る場所は重ならない） */
			int nSame = 0, nSelf = 0;
			for (size_t i = 0; i < s_aNearNpc.size(); i ++) {
				if (PuzzleIsTableNpc(s_aNearNpc[i]) && PuzzleOnSeatCell(p, s_aNearNpc[i])) {
					int anTmp[PUZZLE_PARAM_COUNT];
					PuzzleParseSpeech(s_aNearNpc[i]->pszNpcSpeech, anTmp);
					if (anTmp[0] == anVal[0]) {
						nSame ++;
						if (s_aNearNpc[i] == pNpc) nSelf ++;
					}
				}
			}
			if ((nSame == 1) && (nSelf == 1)) {
				PuzzleUseCoin(p, (WORD)anVal[0]);
			} else {
				PuzzleSit(p, pNpc, TRUE);
			}
			return;
		}
	}
	PuzzleKeyS(p, pNpc);
}

/* 人を決めたマスへ置く（同じマップ・同じ空間。QuestMovePC と違い席は片付けない） */
static void PuzzlePlace(PCHARINFO p, PCHARINFO pNpc, DWORD dwX, DWORD dwY)
{
	if ((p->dwMapID == pNpc->dwMapID) && (p->dwX == dwX) && (p->dwY == dwY)) {
		return;
	}
	DeleteTblNoPC((int)p->dwTblID);
	p->dwMapID = pNpc->dwMapID;
	p->dwX = dwX;
	p->dwY = dwY;
	AddTblNoPC((int)p->dwTblID);
}

/* 台の座る場所のマス（台詞の 9・10 番目。省略は台NPCの 1 つ下）に立ってから S キーで話しかける */
/* 「乱入しますか？」への答え（CMD_TALKANSWER。問いが無ければ黙って捨てられる） */
static void PuzzleAnswerSend(PCHARINFO p, PCHARINFO pNpc, BOOL bYes)
{
	CMDPARA_DATA1 Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_TALKANSWER;
	Para.byData = bYes ? 1 : 0;
	Para.dwTblID = pNpc->dwTblID;
	TestRecv(p, &Para, sizeof(Para));
}

/* ペアの台でひとりが遊んでいる所へ座ると、乱入するかを聞かれる。bYes=乱入（対戦）、FALSE=ひとりで遊ぶ。
   聞かれたら「puzzle ask」を出し、答えを送る（聞かれなければ何もしない） */
static void PuzzleTalk(PCHARINFO p, PCHARINFO pNpc, BOOL bYes = TRUE)
{
	int nSdx, nSdy;
	int nConn = DgTestConn(p);
	PuzzleSeatOffset(pNpc->pszNpcSpeech, &nSdx, &nSdy);
	PuzzlePlace(p, pNpc, (DWORD)((int)pNpc->dwX + nSdx), (DWORD)((int)pNpc->dwY + nSdy));
	int nBefore = MockCmdCount(nConn, CMD_TALKMSG);
	PuzzleTalkRaw(p, pNpc);
	if (MockCmdCount(nConn, CMD_TALKMSG) > nBefore && MockHasText(nConn, CMD_TALKMSG, "乱入")) {
		printf("puzzle ask shown seated=%d answer=%d\n", PuzzleIsSeated(p), bYes ? 1 : 0);
		PuzzleAnswerSend(p, pNpc, bYes);
	}
}

static void TestPuzzle(const std::vector<int> &aLogin)
{
	printf("== AV. Falling puzzle table ==\n");
	if (aLogin.size() < 2) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]];
	int nConnA = DgTestConn(pA), nConnB = DgTestConn(pB);
	WORD wCoin = 1;
	char szSpeech[32];
	DWORD dwXBack = pA->dwX, dwYBack = pA->dwY, dwMapBack = pA->dwMapID;
	BYTE byStateBack = pA->byCharState;
	WORD wHPBack = pA->wHP;
	DWORD dwTimeBack = g_dwTime;

	/* 座れる状態にする（A は台のすぐ隣、B は同じ場所） */
	pA->byCharState = CHARSTATE_MOVE;
	if (pA->wHP == 0) pA->wHP = 10;
	sprintf(szSpeech, "@p%d,2", (int)wCoin);
	NearNpcOpen(pA, szSpeech);
	PCHARINFO pNpc = s_aNearNpc.back();
	for (int i = 0; i < MAX_MYITEM; i ++) if (pA->waItem[i] == wCoin) DelCharItem(pA->dwTblID, (BYTE)i);
	printf("puzzle npc=%d isTable=%d free=%d\n", pNpc ? 1 : 0, PuzzleIsTableNpc(pNpc), PuzzleSeatCount());

	/* コインが無い（使えるものが無いので S キー）→ 座れず、コインを使う案内だけ（文字の確認は、送ったバイト列を覚えさせている間だけ） */
	MockTrace(1);
	PuzzleTalk(pA, pNpc);
	printf("puzzle nocoin seated=%d msg=%d state=%d\n", PuzzleIsSeated(pA), MockHasText(nConnA, CMD_SYSMSG, "コマンドの道具からコインを使ってください"),
		MockCmdCount(nConnA, CMD_PUZZLESTATE));

	/* コイン 1 枚（2 枚必要）を座る場所で使う → まだ座れず「コインがありません」、減りもしない */
	AddCharItem(pA->dwTblID, wCoin);
	PuzzleTalk(pA, pNpc);
	printf("puzzle short seated=%d coin=%d msg=%d\n", PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin),
		MockHasText(nConnA, CMD_SYSMSG, "コインがありません"));
	/* 有料の台で S キー → コインを持っていても使わず、座れず、減らない（案内のメッセージが 1 つ出る） */
	{
		int nMsg = MockCmdCount(nConnA, CMD_SYSMSG);
		PuzzleKeyS(pA, pNpc);
		printf("puzzle skey paid seated=%d coin=%d msg+%d\n", PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin),
			MockCmdCount(nConnA, CMD_SYSMSG) - nMsg);
	}
	/* 座る場所以外（座る場所の 1 つ隣）でコインを使う → 座れず、減らず、案内も出ない（今まで通り効果なしの説明）。
	   「何も起こりませんでした」の MAPMSG は出る（SYSMSG は増えない） */
	{
		int nSdx, nSdy, nMsg = MockCmdCount(nConnA, CMD_SYSMSG);
		PuzzleSeatOffset(pNpc->pszNpcSpeech, &nSdx, &nSdy);
		PuzzlePlace(pA, pNpc, (DWORD)((int)pNpc->dwX + nSdx + 1), (DWORD)((int)pNpc->dwY + nSdy));
		PuzzleUseCoin(pA, wCoin);
		printf("puzzle use elsewhere seated=%d coin=%d state=%d sysmsg+%d\n", PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin),
			PuzzleSeatCount(), MockCmdCount(nConnA, CMD_SYSMSG) - nMsg);
	}
	/* 2 枚 → 座る。2 枚減り、盤面全体(FULL)が届く */
	AddCharItem(pA->dwTblID, wCoin);
	AddCharItem(pA->dwTblID, wCoin);
	int nState0 = MockCmdCount(nConnA, CMD_PUZZLESTATE), nItem0 = MockCmdCount(nConnA, CMD_CHARITEM);
	PuzzleTalk(pA, pNpc);
	printf("puzzle sit seated=%d coin=%d state+%d used=%d\n", PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin),
		MockCmdCount(nConnA, CMD_PUZZLESTATE) - nState0, MockHasText(nConnA, CMD_SYSMSG, "コインを2枚使いました"));
	/* コインを取り上げたら持ち物を本人に送り直す（送らないと画面にコインが残り、捨てると切断になる） */
	printf("puzzle coin charitem +%d\n", MockCmdCount(nConnA, CMD_CHARITEM) - nItem0);

	/* 同じ台に 2 人目（段階1は席 1 つ）は座れず、コインも減らない */
	AddCharItem(pB->dwTblID, wCoin);
	AddCharItem(pB->dwTblID, wCoin);
	DWORD dwBX = pB->dwX, dwBY = pB->dwY, dwBMap = pB->dwMapID;
	DeleteTblNoPC((int)pB->dwTblID);
	pB->dwMapID = pA->dwMapID; pB->dwX = pA->dwX; pB->dwY = pA->dwY;
	AddTblNoPC((int)pB->dwTblID);
	BYTE byBState = pB->byCharState; WORD wBHP = pB->wHP;
	pB->byCharState = CHARSTATE_MOVE; if (pB->wHP == 0) pB->wHP = 10;
	PuzzleTalk(pB, pNpc);
	printf("puzzle busy seatedB=%d coinB=%d msg=%d\n", PuzzleIsSeated(pB), PuzzleCoinCount(pB, wCoin),
		MockHasText(nConnB, CMD_SYSMSG, "この台は今、使われています"));

	/* 座っている間は歩けない（歩いても位置は動かない） */
	{
		CMDPARA_CHARPOS Pos;
		ZeroMemory(&Pos, sizeof(Pos));
		Pos.byCmd = CMD_CHARPOS;
		Pos.dwTblID = pA->dwTblID;
		Pos.dwMapID = pA->dwMapID;
		Pos.byX = (BYTE)(pA->dwX + 1);
		Pos.byY = (BYTE)pA->dwY;
		Pos.byDirection = 3;
		DWORD dwX0 = pA->dwX;
		TestRecv(pA, &Pos, sizeof(Pos));
		printf("puzzle nowalk moved=%d\n", pA->dwX != dwX0);
	}

	/* キー入力：回す・左右は位置だけ(MOVE)。速すぎる入力は捨てる。下は落とす */
	g_dwTime = s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 100;
	int nBeforeKey = MockCmdCount(nConnA, CMD_PUZZLESTATE);
	PuzzleKeySend(pA, PUZZLEKEY_LEFT);
	g_dwTime = (s_dwFakeNow += 5);
	PuzzleKeySend(pA, PUZZLEKEY_LEFT);							/* 5ms 後（バケツに余裕があるので受ける） */
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_ROTATE);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_RIGHT);
	printf("puzzle keys packets=%d seated=%d\n", MockCmdCount(nConnA, CMD_PUZZLESTATE) - nBeforeKey, PuzzleIsSeated(pA));
	{
		/* 先読みの差し引き用：受けたキーの連番を、変わった時の通知に付けて返す（捨てたキーも連番は進む） */
		DWORD dwAck = 0;
		PuzzleOverFlag(nConnA, pNpc->dwTblID, NULL, NULL, &dwAck);
		printf("puzzle ack last=%d\n", dwAck == s_dwPuzzleSerial);
	}
	{
		/* 回線が遅れてまとまって届いたキー：バケツ（8 個）の分だけ受け、残りは捨てるが連番（dwAck）は MOVE で返す（先読みしたクライアントがすぐ直せる） */
		g_dwTime = s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000;
		int nBeforeBurst = MockCmdCount(nConnA, CMD_PUZZLESTATE);
		for (int i = 0; i < 12; i ++) {
			PuzzleKeySend(pA, (i % 2) ? PUZZLEKEY_RIGHT : PUZZLEKEY_LEFT);
		}
		DWORD dwAckBurst = 0;
		PuzzleOverFlag(nConnA, pNpc->dwTblID, NULL, NULL, &dwAckBurst);
		printf("puzzle burst packets=%d ack last=%d\n", MockCmdCount(nConnA, CMD_PUZZLESTATE) - nBeforeBurst, dwAckBurst == s_dwPuzzleSerial);
		/* 左回しも受ける */
		g_dwTime = s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 100;
		int nSeA0 = MockCmdCount(nConnA, CMD_PUZZLESE), nSeB0 = MockCmdCount(nConnB, CMD_PUZZLESE);
		PuzzleKeySend(pA, PUZZLEKEY_ROTATE_L);
		printf("puzzle rotate left ok=%d\n", PuzzleIsSeated(pA));
		/* 向きを変えた効果音：見ている人(B)には届き、座っている本人(A)には送らない（本人は押した時にクライアントで鳴らす） */
		printf("puzzle se rotate self=%d viewer=%d\n", MockCmdCount(nConnA, CMD_PUZZLESE) - nSeA0, MockCmdCount(nConnB, CMD_PUZZLESE) - nSeB0);
	}

	/* 落下は経過時間で進む（PuzzleProc を呼ぶ間隔に関係なく 1 秒に 1 段）。動いた時は位置だけ(MOVE) */
	int nBeforeDrop = MockCmdCount(nConnA, CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 400);
	PuzzleProc();
	int nNoDrop = MockCmdCount(nConnA, CMD_PUZZLESTATE) - nBeforeDrop;
	g_dwTime = (s_dwFakeNow += 700);							/* 前のキー入力から 1 秒以上 */
	PuzzleProc();
	int nDrop = MockCmdCount(nConnA, CMD_PUZZLESTATE) - nBeforeDrop;
	printf("puzzle gravity early=%d later=%d\n", nNoDrop, nDrop > 0 ? 1 : 0);

	/* 他人の席には触れない：B（座っていない）のキー入力は無視 */
	int nBefore = MockCmdCount(nConnB, CMD_PUZZLESTATE);
	PuzzleKeySend(pB, PUZZLEKEY_DOWN);
	printf("puzzle otherkey packets=%d\n", MockCmdCount(nConnB, CMD_PUZZLESTATE) - nBefore);

	/* 下キーで落とし続けると、置いて積み上がり、置けなくなってゲームオーバー（得点を出して席を立つ） */
	int nLoops = 0;
	int nLockA0 = MockCmdCount(nConnA, CMD_PUZZLESE), nLockB0 = MockCmdCount(nConnB, CMD_PUZZLESE);
	while (PuzzleIsSeated(pA) && nLoops < 5000) {
		g_dwTime = (s_dwFakeNow += 30);
		PuzzleKeySend(pA, PUZZLEKEY_DOWN);
		nLoops ++;
	}
	/* 定着の効果音は本人にも見ている人にも届く（同じ回数） */
	printf("puzzle se lock self>0=%d same=%d\n", MockCmdCount(nConnA, CMD_PUZZLESE) - nLockA0 > 0,
		MockCmdCount(nConnA, CMD_PUZZLESE) - nLockA0 == MockCmdCount(nConnB, CMD_PUZZLESE) - nLockB0);
	printf("puzzle over seated=%d score=%d state=%d\n", PuzzleIsSeated(pA), MockHasText(nConnA, CMD_SYSMSG, "ゲームオーバー！ 得点"),
		PuzzleSeatCount());

	/* 席を立つ（ESC）→ 片付く。立った後のキーは無視 */
	PuzzleTalk(pB, pNpc);
	printf("puzzle sitB seated=%d coinB=%d\n", PuzzleIsSeated(pB), PuzzleCoinCount(pB, wCoin));
	int nNone0 = MockCmdCount(nConnB, CMD_PUZZLESTATE);
	PuzzleKeySend(pB, PUZZLEKEY_LEAVE);
	printf("puzzle leave seated=%d count=%d none+%d\n", PuzzleIsSeated(pB), PuzzleSeatCount(), MockCmdCount(nConnB, CMD_PUZZLESTATE) - nNone0);

	/* 席から離れる（マップ移動など）と周期処理で片付く */
	AddCharItem(pB->dwTblID, wCoin);
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pNpc);
	int nSeatedAway = PuzzleIsSeated(pB);
	DeleteTblNoPC((int)pB->dwTblID);
	pB->dwX = pA->dwX + 10;
	AddTblNoPC((int)pB->dwTblID);
	g_dwTime = (s_dwFakeNow += 10);
	PuzzleProc();
	printf("puzzle away before=%d after=%d count=%d\n", nSeatedAway, PuzzleIsSeated(pB), PuzzleSeatCount());

	/* 切断（OnLeave）で片付く */
	DeleteTblNoPC((int)pB->dwTblID);
	pB->dwX = pA->dwX;
	AddTblNoPC((int)pB->dwTblID);
	AddCharItem(pB->dwTblID, wCoin);
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pNpc);
	int nSeatedBefore = PuzzleIsSeated(pB);
	PuzzleOnLeave(pB);
	printf("puzzle logout before=%d after=%d count=%d\n", nSeatedBefore, PuzzleIsSeated(pB), PuzzleSeatCount());

	/* 無料の台（枚数 0）はコイン無しで座れる */
	{
		PCHARINFO pFree;
		NearNpcOpen(pB, "@p");
		pFree = s_aNearNpc.back();
		for (int i = 0; i < MAX_MYITEM; i ++) if (pB->waItem[i] == wCoin) DelCharItem(pB->dwTblID, (BYTE)i);
		PuzzleTalk(pB, pFree);
		printf("puzzle free seated=%d\n", PuzzleIsSeated(pB));
		PuzzleOnLeave(pB);
	}

	/* 片付け */
	MockTrace(0);
	PuzzleOnLeave(pA);
	PuzzleOnLeave(pB);
	PuzzleRetireNpcs();
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();									/* 台 NPC が無くなった待機中の台を表から消す */
	for (int i = 0; i < MAX_MYITEM; i ++) if (pA->waItem[i] == wCoin) DelCharItem(pA->dwTblID, (BYTE)i);
	for (int i = 0; i < MAX_MYITEM; i ++) if (pB->waItem[i] == wCoin) DelCharItem(pB->dwTblID, (BYTE)i);
	DeleteTblNoPC((int)pB->dwTblID);
	pB->dwMapID = dwBMap; pB->dwX = dwBX; pB->dwY = dwBY;
	pB->byCharState = byBState; pB->wHP = wBHP;
	AddTblNoPC((int)pB->dwTblID);
	pA->dwMapID = dwMapBack; pA->dwX = dwXBack; pA->dwY = dwYBack;
	pA->byCharState = byStateBack; pA->wHP = wHPBack;
	g_dwTime = dwTimeBack;
	printf("puzzle done seats=%d\n", PuzzleSeatCount());
}

/* ------------------------------------------------------------------------- */
/* AV2. 落ち物パズルの対戦（台詞 "@p<コイン>,<枚数>,,,,,,<ペア番号>" の 2 台）	 */
/*     ひとりが遊んでいる所へ、ペアのもう 1 台に座ると対戦（盤面を空に戻す・同じ落ち物の順・カウントダウン）・	 */
/*     お邪魔の送信・相殺・固定で積まれる・置けなくなった側が負け（勝者は続ける）・	 */
/*     ESC・切断・離れるで残った側の勝ち・別のペアやペアなしでは対戦にならない・近くの人（観戦）にも届く	 */
/* ------------------------------------------------------------------------- */
static void PuzzleVsSit(PCHARINFO p, WORD wCoin, PCHARINFO pNpc)
{
	AddCharItem(p->dwTblID, wCoin);
	PuzzleTalk(p, pNpc);
}

static void TestPuzzleVs(const std::vector<int> &aLogin)
{
	printf("== AV2. Falling puzzle versus ==\n");
	if (aLogin.size() < 3) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO apP[3] = { pA, pB, pC };
	int anConn[3] = { DgTestConn(pA), DgTestConn(pB), DgTestConn(pC) };
	struct { DWORD dwMap, dwX, dwY; BYTE byState; WORD wHP; } aBack[3];
	WORD wCoin = 1;
	DWORD dwTimeBack = g_dwTime;
	int i;

	for (i = 0; i < 3; i ++) {
		aBack[i].dwMap = apP[i]->dwMapID; aBack[i].dwX = apP[i]->dwX; aBack[i].dwY = apP[i]->dwY;
		aBack[i].byState = apP[i]->byCharState; aBack[i].wHP = apP[i]->wHP;
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
	}
	/* 3 人とも pA のいる所（台のすぐそば）。C は観戦 */
	DWORD dwMap = pA->dwMapID, dwX = pA->dwX, dwY = pA->dwY;
	for (i = 1; i < 3; i ++) QuestMovePC(apP[i], (int)dwMap, (int)dwX, (int)dwY);
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
	/* ペア 5 の 2 台・ペア 6 の台・ペアなしの台（コイン 1 枚） */
	NearNpcOpen(pA, "@p1,1,,,,,,5");
	PCHARINFO pT1 = s_aNearNpc.back();
	NearNpcOpen(pA, "@p1,1,,,,,,5");
	PCHARINFO pT2 = s_aNearNpc.back();
	NearNpcOpen(pA, "@p1,1,,,,,,6");
	PCHARINFO pT3 = s_aNearNpc.back();
	NearNpcOpen(pA, "@p1,1");
	PCHARINFO pT4 = s_aNearNpc.back();
	MockTrace(1);
	g_dwTime = (s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000);

	/* 1. A がペア 5 の台 1 にひとりで座る（ひとり用）。ペアの相手がいないので対戦にならない */
	int nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	PuzzleVsSit(pA, wCoin, pT1);
	printf("vs solo seatedA=%d vsA=%d coinA=%d\n", PuzzleIsSeated(pA), PuzzleIsVersus(pA), PuzzleCoinCount(pA, wCoin));

	/* 2. 観戦：近くにいる C へは、座った時に盤面全体が 1 回届き、見直しでは重ねて送らない
	   （席が空の待機中の台 3 つ＝台 2・3・4 の待機状態は、見直しで 1 回ずつ届く）。
	   遠くへ離れたら NONE が届き（座っている台 1 つと待機中の台 3 つで 4 回。遠い間はキーの更新も届かない）、戻るとまた盤面全体が届く */
	printf("vs spectate near packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("vs spectate nochange packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);
	QuestMovePC(pC, (int)dwMap, (int)dwX + 40, (int)dwY);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("vs spectate far packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleKeySend(pA, PUZZLEKEY_LEFT);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_RIGHT);
	printf("vs spectate far key packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);
	QuestMovePC(pC, (int)dwMap, (int)dwX, (int)dwY);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("vs spectate back packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0 >= 1);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_LEFT);
	printf("vs spectate key packets=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);
	/* （以降、台が増減するとゲームの置き場所が動くので、使う直前に取り直す） */

	/* 3. A が少し遊んだ盤面で、B が台 2 に座る → 対戦開始：両方の盤面が空に戻り、同じ落ち物の順番・カウントダウン */
	PuzzleGame *gA = PuzzleGetGame(pA);
	gA->Board.Set(0, 13, 3);
	gA->dwScore = 999;
	int nA0 = MockCmdCount(anConn[0], CMD_PUZZLESTATE), nB0 = MockCmdCount(anConn[1], CMD_PUZZLESTATE);
	nC0 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 100);
	DWORD dwSit = g_dwTime;
	PuzzleVsSit(pB, wCoin, pT2);
	gA = PuzzleGetGame(pA);
	PuzzleGame *gB = PuzzleGetGame(pB);
	printf("vs start seated=%d%d vs=%d%d coinB=%d msg=%d%d\n", PuzzleIsSeated(pA), PuzzleIsSeated(pB), PuzzleIsVersus(pA), PuzzleIsVersus(pB),
		PuzzleCoinCount(pB, wCoin), MockHasText(anConn[0], CMD_SYSMSG, "対戦開始"), MockHasText(anConn[1], CMD_SYSMSG, "対戦開始"));
	printf("vs start reset board=%d score=%d sameOrder=%d\n", gA->Board.Get(0, 13), (int)gA->dwScore,
		gA->nKind == gB->nKind && gA->nNext == gB->nNext);
	printf("vs start packets A=%d B=%d C=%d\n", MockCmdCount(anConn[0], CMD_PUZZLESTATE) - nA0, MockCmdCount(anConn[1], CMD_PUZZLESTATE) - nB0,
		MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC0);

	/* 4. カウントダウン中は落ちず、キーも効かない。秒が変わるたびに状態が届き、終わると動く */
	int nY0 = gA->nY;
	g_dwTime = (s_dwFakeNow = dwSit + 1500);
	PuzzleKeySend(pA, PUZZLEKEY_DOWN);
	nA0 = MockCmdCount(anConn[0], CMD_PUZZLESTATE);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow = dwSit + 2500);
	PuzzleProc();
	printf("vs countdown y=%d packets=%d\n", gA->nY - nY0, MockCmdCount(anConn[0], CMD_PUZZLESTATE) - nA0 > 0);
	g_dwTime = (s_dwFakeNow = dwSit + 3100);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_DOWN);
	printf("vs go y=%d\n", gA->nY - nY0);

	/* 5. お邪魔：A が 4 列消す → 相手 B の待ちに 4 段。B の盤面にはまだ積まれない */
	gA->Board.Init(10, 14);
	for (int r = 0; r < 4; r ++) for (int c = 0; c < 10; c ++) if (c != 4) gA->Board.Set(c, 10 + r, 1);
	gA->nKind = 0; gA->nRot = 1; gA->nX = 2; gA->nY = 10;
	g_dwTime = (s_dwFakeNow += 100);
	int nB1 = MockCmdCount(anConn[1], CMD_PUZZLESTATE);
	PuzzleKeySend(pA, PUZZLEKEY_DOWN);
	printf("vs attack4 pendingB=%d pendingA=%d linesA=%d packetsB=%d\n", gB->nPending, gA->nPending, gA->nLines,
		MockCmdCount(anConn[1], CMD_PUZZLESTATE) - nB1 > 0);
	/* B が 2 列消す（攻撃 1）→ 自分宛て 4 から 1 を相殺して 3 が残り（A へは送らない）、残りの 3 段はこの固定で B の盤面の下から積まれる */
	gB->Board.Init(10, 14);
	for (int r = 0; r < 2; r ++) for (int c = 0; c < 10; c ++) if (c != 4) gB->Board.Set(c, 12 + r, 1);
	gB->nKind = 0; gB->nRot = 1; gB->nX = 2; gB->nY = 10;
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pB, PUZZLEKEY_DOWN);
	int nGarbRows = 0, nHoleOK = 0;
	for (int r = 0; r < 14; r ++) {
		int nG = 0, nHole = 0;
		for (int c = 0; c < 10; c ++) { if (gB->Board.Get(c, r) == PUZZLE_CELL_GARBAGE) nG ++; if (gB->Board.Get(c, r) == 0) nHole ++; }
		if (nG == 9 && nHole == 1) { nGarbRows ++; if (r >= 11) nHoleOK ++; }
	}
	printf("vs offset pendingB=%d pendingA=%d linesB=%d garbageRows=%d bottom3=%d\n", gB->nPending, gA->nPending, gB->nLines, nGarbRows, nHoleOK);
	/* B が消さずに固定する → 待ちが下から積まれる（穴は 1 段 1 か所） */
	gB->Board.Init(10, 14);
	gB->AddPending(3);
	gB->nKind = 1; gB->nRot = 0; gB->nX = 0; gB->nY = 12;
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pB, PUZZLEKEY_DOWN);
	nGarbRows = 0;
	for (int r = 0; r < 14; r ++) {
		int nG = 0;
		for (int c = 0; c < 10; c ++) if (gB->Board.Get(c, r) == PUZZLE_CELL_GARBAGE) nG ++;
		if (nG >= 8) nGarbRows ++;
	}
	printf("vs garbage pendingB=%d rows=%d\n", gB->nPending, nGarbRows);

	/* 6. B が置けなくなって負け → 両者に「○○の勝ち！」・B は席を立つ・A は続ける（得点そのまま・待ちなし） */
	DWORD dwScoreA = gA->dwScore;
	gB->Board.Init(10, 14);
	gB->Board.Set(4, 0, 1); gB->Board.Set(5, 0, 1);
	gB->nKind = 1; gB->nRot = 0; gB->nX = 0; gB->nY = 12;
	gB->nPending = 0;
	gA->AddPending(2);
	int nA2 = MockCmdCount(anConn[0], CMD_PUZZLESTATE), nC2 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pB, PUZZLEKEY_DOWN);
	printf("vs lose seatedB=%d seatedA=%d vsA=%d msgA=%d msgB=%d scoreB=%d\n", PuzzleIsSeated(pB), PuzzleIsSeated(pA), PuzzleIsVersus(pA),
		MockHasText(anConn[0], CMD_SYSMSG, "の勝ち！"), MockHasText(anConn[1], CMD_SYSMSG, "の勝ち！"),
		MockHasText(anConn[1], CMD_SYSMSG, "ゲームオーバー！ 得点"));
	gA = PuzzleGetGame(pA);
	printf("vs lose keep score=%d pending=%d packetsA=%d packetsC=%d\n", gA && gA->dwScore == dwScoreA, gA ? gA->nPending : -1,
		MockCmdCount(anConn[0], CMD_PUZZLESTATE) - nA2 > 0, MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC2 > 0);
	/* 負けた B の盤面の GAME OVER は、観覧者 C と勝った A にも見える（最後の盤面＋byOver=1・待機中）。本人 B へは OVER（k3） */
	{
		int nIdC = 0, nCellsC = 0, nIdA = 0;
		int nOvC = PuzzleOverFlag(anConn[2], pT2->dwTblID, &nIdC, &nCellsC), nOvA = PuzzleOverFlag(anConn[0], pT2->dwTblID, &nIdA);
		printf("vs lose gameover spectator=%d idle=%d board=%d winner=%d\n", nOvC, nIdC, nCellsC > 0, nOvA);
		/* 消える時間（PUZZLE_OVER_SHOW_MS）の手前ではまだ出ている。過ぎると byOver=0 の盤面が届き、最後の盤面だけ残る */
		g_dwTime = (s_dwFakeNow += PUZZLE_OVER_SHOW_MS - 200);
		PuzzleProc();
		nOvC = PuzzleOverFlag(anConn[2], pT2->dwTblID);
		g_dwTime = (s_dwFakeNow += 300);
		PuzzleProc();
		int nOvC2 = PuzzleOverFlag(anConn[2], pT2->dwTblID, &nIdC, &nCellsC), nOvA2 = PuzzleOverFlag(anConn[0], pT2->dwTblID);
		printf("vs lose gameover before=%d after=%d idle=%d board=%d winner=%d\n", nOvC, nOvC2, nIdC, nCellsC > 0, nOvA2);
	}
	/* 勝者はそのまま続けられる：落下が進む・消しても相手へは送られない */
	g_dwTime = (s_dwFakeNow += 1500);
	nA2 = MockCmdCount(anConn[0], CMD_PUZZLESTATE);
	PuzzleProc();
	printf("vs winner continues seated=%d packets=%d\n", PuzzleIsSeated(pA), MockCmdCount(anConn[0], CMD_PUZZLESTATE) - nA2 > 0);

	/* 7. もう一度対戦 → B が ESC で席を立つ → A の勝ち */
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleVsSit(pB, wCoin, pT2);
	int nVsAgain = PuzzleIsVersus(pA) && PuzzleIsVersus(pB);
	int nMsgA = MockCmdCount(anConn[0], CMD_SYSMSG);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pB, PUZZLEKEY_LEAVE);
	printf("vs esc again=%d seatedB=%d seatedA=%d vsA=%d msgA+%d\n", nVsAgain, PuzzleIsSeated(pB), PuzzleIsSeated(pA), PuzzleIsVersus(pA),
		MockCmdCount(anConn[0], CMD_SYSMSG) - nMsgA);

	/* 8. 対戦中に B が切断（OnLeave）→ A の勝ち（B には何も送らない）。A は続ける */
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleVsSit(pB, wCoin, pT2);
	nVsAgain = PuzzleIsVersus(pA) && PuzzleIsVersus(pB);
	int nMsgA3 = MockCmdCount(anConn[0], CMD_SYSMSG), nStB = MockCmdCount(anConn[1], CMD_PUZZLESTATE);
	PuzzleOnLeave(pB);
	printf("vs logout again=%d seatedB=%d seatedA=%d vsA=%d msgA+%d packetsB=%d\n", nVsAgain, PuzzleIsSeated(pB), PuzzleIsSeated(pA),
		PuzzleIsVersus(pA), MockCmdCount(anConn[0], CMD_SYSMSG) - nMsgA3, MockCmdCount(anConn[1], CMD_PUZZLESTATE) - nStB);

	/* 9. 対戦中に B が席から離れる → A の勝ち */
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleVsSit(pB, wCoin, pT2);
	nVsAgain = PuzzleIsVersus(pA) && PuzzleIsVersus(pB);
	QuestMovePC(pB, (int)dwMap, (int)dwX + 10, (int)dwY);
	g_dwTime = (s_dwFakeNow += 10);
	PuzzleProc();
	printf("vs away again=%d seatedB=%d seatedA=%d vsA=%d\n", nVsAgain, PuzzleIsSeated(pB), PuzzleIsSeated(pA), PuzzleIsVersus(pA));
	QuestMovePC(pB, (int)dwMap, (int)dwX, (int)dwY);

	/* 10. 別のペア・ペアなしの台に座っても対戦にならない（A はペア 5 の台 1 でひとり） */
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleVsSit(pB, wCoin, pT3);
	printf("vs otherpair seated=%d%d vs=%d%d\n", PuzzleIsSeated(pA), PuzzleIsSeated(pB), PuzzleIsVersus(pA), PuzzleIsVersus(pB));
	PuzzleOnLeave(pB);
	PuzzleVsSit(pB, wCoin, pT4);
	printf("vs nopair seated=%d%d vs=%d%d\n", PuzzleIsSeated(pA), PuzzleIsSeated(pB), PuzzleIsVersus(pA), PuzzleIsVersus(pB));
	PuzzleOnLeave(pB);

	/* 11. コインが無ければ対戦にもならない（A は台 1 で遊んでいる所へ、コイン無しの B が台 2） */
	for (int j = 0; j < MAX_MYITEM; j ++) if (pB->waItem[j] == wCoin) DelCharItem(pB->dwTblID, (BYTE)j);
	PuzzleTalk(pB, pT2);
	printf("vs nocoin seatedB=%d vsA=%d\n", PuzzleIsSeated(pB), PuzzleIsVersus(pA));

	/* 11b. 乱入を聞かれて「いいえ」と答えると、座るがひとりで遊ぶ（対戦にならない）。コインは答えのあとに取り上げる */
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2, FALSE);
	printf("vs decline seatedB=%d vs=%d%d coinB=%d\n", PuzzleIsSeated(pB), PuzzleIsVersus(pA), PuzzleIsVersus(pB), PuzzleCoinCount(pB, wCoin));
	PuzzleOnLeave(pB);

	/* 12. 台が空になった時、観戦の C に NONE（OVER）が届く */
	nC2 = MockCmdCount(anConn[2], CMD_PUZZLESTATE);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	PuzzleKeySend(pA, PUZZLEKEY_LEAVE);
	printf("vs spectate leave packets=%d seats=%d\n", MockCmdCount(anConn[2], CMD_PUZZLESTATE) - nC2 > 0, PuzzleSeatCount());

	/* 片付け */
	MockTrace(0);
	PuzzleOnLeave(pA); PuzzleOnLeave(pB); PuzzleOnLeave(pC);
	PuzzleRetireNpcs();
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();									/* 台 NPC が無くなった待機中の台を表から消す */
	for (i = 0; i < 3; i ++) {
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
		QuestMovePC(apP[i], (int)aBack[i].dwMap, (int)aBack[i].dwX, (int)aBack[i].dwY);
		apP[i]->byCharState = aBack[i].byState;
		apP[i]->wHP = aBack[i].wHP;
	}
	g_dwTime = dwTimeBack;
	printf("vs done seats=%d\n", PuzzleSeatCount());
}

/* ------------------------------------------------------------------------- */
/* AV3. 落ち物パズルの待機中の台（席に誰もいない台）が、近づいた人に見える					 */
/*     台の近くに来た人へ待機状態（byIdle=1・空の盤面・byMine=0）が届く・画面に入り始めるより手前から送り、	 */
/*     離れる時は送り始める距離より外まで離れてから NONE（境目で繰り返さない）・	 */
/*     同じマップの遠い人には送らない・座るとゲーム中の状態（byMine=1・byIdle=0）に切り替わる・	 */
/*     ゲームオーバーの最後の盤面は次に誰かが座るまで残る（戻ってきた人にも見える）・	 */
/*     席を立つと空の井戸に戻る・誰もいなくなった待機中の台は表から消える			 */
/* ------------------------------------------------------------------------- */
EM_JS(char *, MockCmdAll, (int nConn, int nCmd), {
	return stringToNewUTF8(Module['sboNet'].cmdAll(nConn, nCmd));
});

/* 覚えたパケットのうち CMD_PUZZLESTATE を順番に取り出す */
static void PuzzlePackets(int nConn, std::vector<std::vector<BYTE> > &aOut)
{
	char *psz = MockCmdAll(nConn, CMD_PUZZLESTATE);
	size_t nLen = strlen(psz), nPos = 0;
	aOut.clear();
	while (nPos < nLen) {
		size_t nEnd = nPos;
		while (nEnd < nLen && psz[nEnd] != ',') nEnd ++;
		std::vector<BYTE> a((nEnd - nPos) / 2, 0);
		for (size_t i = 0; i < a.size(); i ++) {
			unsigned int u = 0;
			sscanf(psz + nPos + i * 2, "%2x", &u);
			a[i] = (BYTE)u;
		}
		aOut.push_back(a);
		nPos = nEnd + 1;
	}
	free(psz);
}

/* 盤面のセルのうち 0 でないものの数（盤面が付いていなければ -1） */
static int PuzzlePktCells(const std::vector<BYTE> &a)
{
	int n = 0;
	if (a.size() <= sizeof(CMDPARA_PUZZLESTATE)) return -1;
	for (size_t i = sizeof(CMDPARA_PUZZLESTATE); i < a.size(); i ++) if (a[i] != 0) n ++;
	return n;
}

/* 起点（nFrom 個目）より後ろのパケットの数と、最後のパケットの要約を作る。
   要約: kind/mine/idle/盤面のブロック数（パケットが無ければ "-"） */
static std::string PuzzleSummary(int nConn, int nFrom, int *pnCount)
{
	std::vector<std::vector<BYTE> > aP;
	char sz[64];
	PuzzlePackets(nConn, aP);
	*pnCount = (int)aP.size() - nFrom;
	if (*pnCount <= 0) return "-";
	const std::vector<BYTE> &a = aP.back();
	const CMDPARA_PUZZLESTATE *pSt = (const CMDPARA_PUZZLESTATE *)&a[0];
	sprintf(sz, "k%d/m%d/i%d/c%d", (int)pSt->byKind, (int)pSt->byMine, (int)pSt->byIdle, PuzzlePktCells(a));
	return sz;
}

static int PuzzleCount(int nConn)
{
	std::vector<std::vector<BYTE> > aP;
	PuzzlePackets(nConn, aP);
	return (int)aP.size();
}

static void TestPuzzleIdle(const std::vector<int> &aLogin)
{
	printf("== AV3. Falling puzzle idle table ==\n");
	if (aLogin.size() < 3) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO apP[3] = { pA, pB, pC };
	int anConn[3] = { DgTestConn(pA), DgTestConn(pB), DgTestConn(pC) };
	struct { DWORD dwMap, dwX, dwY; BYTE byState; WORD wHP; } aBack[3];
	WORD wCoin = 1;
	DWORD dwTimeBack = g_dwTime;
	int i, n, anN[3];

	for (i = 0; i < 3; i ++) {
		aBack[i].dwMap = apP[i]->dwMapID; aBack[i].dwX = apP[i]->dwX; aBack[i].dwY = apP[i]->dwY;
		aBack[i].byState = apP[i]->byCharState; aBack[i].wHP = apP[i]->wHP;
		apP[i]->byCharState = CHARSTATE_MOVE;
		if (apP[i]->wHP == 0) apP[i]->wHP = 10;
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
	}
	/* A・B は台のすぐそば、C は同じマップの遠く（台 NPC は A と同じマス。10×14 の既定では盤面の中心が台と同じ列） */
	DWORD dwMap = pA->dwMapID, dwX = pA->dwX, dwY = pA->dwY;
	QuestMovePC(pB, (int)dwMap, (int)dwX, (int)dwY);
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
	QuestMovePC(pC, (int)dwMap, (int)dwX + 40, (int)dwY);
	NearNpcOpen(pA, "@p1,1");
	PCHARINFO pT = s_aNearNpc.back();
	MockTrace(1);
	int an0[3] = { 0, 0, 0 };
	g_dwTime = (s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000);

	/* 1. 誰も座っていない台でも、近くにいる A・B に待機状態（k1=FULL・m0=本人でない・i1=待機・c0=ブロックなし）が 1 回届く。遠い C には何も届かない */
	PuzzleProc();
	std::string sA = PuzzleSummary(anConn[0], 0, &anN[0]), sB = PuzzleSummary(anConn[1], 0, &anN[1]), sC = PuzzleSummary(anConn[2], 0, &anN[2]);
	printf("idle near A=%s x%d B=%s x%d far C=%s x%d idleTables=%d seats=%d\n", sA.c_str(), anN[0], sB.c_str(), anN[1], sC.c_str(), anN[2],
		PuzzleIdleCount(), PuzzleSeatCount());
	/* 変わらなければ送り直さない（時間が過ぎても） */
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("idle nochange A=%d B=%d C=%d\n", PuzzleCount(anConn[0]) - 1, PuzzleCount(anConn[1]) - 1, PuzzleCount(anConn[2]));

	/* 2. C が歩いて近づく。送り始める距離は横 17 マス（画面に入り始めるより手前）。18 マスではまだ届かない */
	QuestMovePC(pC, (int)dwMap, (int)dwX + 18, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	n = PuzzleCount(anConn[2]);
	QuestMovePC(pC, (int)dwMap, (int)dwX + 17, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	sC = PuzzleSummary(anConn[2], n, &anN[2]);
	printf("idle approach 18=%d 17=%s x%d\n", n, sC.c_str(), anN[2]);

	/* 3. 離れる：送り始めた距離より外（横 20 マスまで）は消さない（19・20 では何も届かず）、21 マスで NONE（k0）。
	   境目（19〜20）をうろうろしても送ったり消したりしない */
	n = PuzzleCount(anConn[2]);
	for (i = 19; i <= 20; i ++) {
		QuestMovePC(pC, (int)dwMap, (int)dwX + i, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		PuzzleProc();
		QuestMovePC(pC, (int)dwMap, (int)dwX + 18, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		PuzzleProc();
	}
	printf("idle hysteresis packets=%d\n", PuzzleCount(anConn[2]) - n);
	QuestMovePC(pC, (int)dwMap, (int)dwX + 21, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	sC = PuzzleSummary(anConn[2], n, &anN[2]);
	{
		/* NONE は盤面が付かないので c は -1 */
		printf("idle leave 21=%s x%d\n", sC.c_str(), anN[2]);
	}
	/* 遠くなると、離れた C には台のキー操作も届かない（NONE のあとは何も来ない）。もう一度 17 マスまで来ると待機状態が届く */
	n = PuzzleCount(anConn[2]);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	QuestMovePC(pC, (int)dwMap, (int)dwX + 17, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	sC = PuzzleSummary(anConn[2], n, &anN[2]);
	printf("idle re-approach=%s x%d\n", sC.c_str(), anN[2]);

	/* 4. A が座る → 見ていた全員へゲーム中の状態（A は m1・B と C は m0。i0）に切り替わる（それぞれ 1 回） */
	AddCharItem(pA->dwTblID, wCoin);
	for (i = 0; i < 3; i ++) an0[i] = PuzzleCount(anConn[i]);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleTalk(pA, pT);
	sA = PuzzleSummary(anConn[0], an0[0], &anN[0]);
	sB = PuzzleSummary(anConn[1], an0[1], &anN[1]);
	sC = PuzzleSummary(anConn[2], an0[2], &anN[2]);
	printf("idle sit seated=%d A=%s x%d B=%s x%d C=%s x%d seats=%d\n", PuzzleIsSeated(pA), sA.c_str(), anN[0], sB.c_str(), anN[1], sC.c_str(), anN[2],
		PuzzleSeatCount());

	/* 5. ゲームオーバー：A には OVER（k3・m1）、見ていた B・C には待機状態で最後の盤面（k1・m0・i1・積んだブロックあり）。席は空になる */
	for (i = 0; i < 3; i ++) an0[i] = PuzzleCount(anConn[i]);
	n = 0;
	while (PuzzleIsSeated(pA) && n < 5000) {
		g_dwTime = (s_dwFakeNow += 30);
		PuzzleKeySend(pA, PUZZLEKEY_DOWN);
		n ++;
	}
	std::vector<std::vector<BYTE> > aP;
	PuzzlePackets(anConn[1], aP);
	std::vector<BYTE> aLastB = aP.back();		/* B が最後に見た盤面（残るはずの盤面） */
	/* 落とし続けた途中の更新も含まれるので、起点は使わず最後の 1 つだけ見る */
	int nOverA = 0;
	{
		std::vector<std::vector<BYTE> > aPA;
		PuzzlePackets(anConn[0], aPA);
		const CMDPARA_PUZZLESTATE *pSt = (const CMDPARA_PUZZLESTATE *)&aPA.back()[0];
		nOverA = (pSt->byKind == PUZZLESTATE_OVER) && pSt->byMine && !pSt->byIdle;
	}
	sB = PuzzleSummary(anConn[1], 0, &anN[1]);
	sC = PuzzleSummary(anConn[2], 0, &anN[2]);
	printf("idle over seated=%d A_over=%d B=%s C=%s kept=%d idleTables=%d seats=%d\n", PuzzleIsSeated(pA), nOverA,
		sB.substr(0, sB.find("/c")).c_str(), sC.substr(0, sC.find("/c")).c_str(), PuzzlePktCells(aLastB) > 0, PuzzleIdleCount(), PuzzleSeatCount());
	/* 5b. 観覧者 B・C の最後の盤面には GAME OVER（byOver=1）が付く。PUZZLE_OVER_SHOW_MS の手前ではまだ出ていて、過ぎると byOver=0 の盤面が
	   届いて消える（最後の盤面は残る）。字が出ている間に近づいた人（C を一度遠ざけてから戻す）にも byOver=1 で届く */
	{
		int nOvB = PuzzleOverFlag(anConn[1], pT->dwTblID), nOvC = PuzzleOverFlag(anConn[2], pT->dwTblID);
		int nOvA = PuzzleOverFlag(anConn[0], pT->dwTblID);		/* 本人には OVER（byOver は付けず k3 で決まる） */
		printf("idle over flag B=%d C=%d selfA=%d\n", nOvB, nOvC, nOvA);
		g_dwTime = (s_dwFakeNow += PUZZLE_OVER_SHOW_MS - 2000);
		PuzzleProc();
		int nB0 = PuzzleCount(anConn[1]);
		QuestMovePC(pC, (int)dwMap, (int)dwX + 25, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		PuzzleProc();
		QuestMovePC(pC, (int)dwMap, (int)dwX + 10, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		PuzzleProc();
		int nOvC2 = PuzzleOverFlag(anConn[2], pT->dwTblID);
		g_dwTime = (s_dwFakeNow += 1500);
		PuzzleProc();
		int nIdB = 0, nCellsB = 0, nIdC = 0;
		int nOvB3 = PuzzleOverFlag(anConn[1], pT->dwTblID, &nIdB, &nCellsB), nOvC3 = PuzzleOverFlag(anConn[2], pT->dwTblID, &nIdC);
		printf("idle over timeline before=%d late-comer=%d after B=%d C=%d idle=%d/%d board=%d packetsB=%d\n", nOvB, nOvC2, nOvB3, nOvC3, nIdB, nIdC,
			nCellsB > 0, PuzzleCount(anConn[1]) - nB0);
		PuzzlePackets(anConn[1], aP);
		aLastB = aP.back();		/* 字が消えたあとに残る盤面（次の確認で戻ってきた人にも同じものが届く） */
	}

	/* 6. 最後の盤面は、見ていた人が離れても・誰もいなくなっても残り、戻ってきた人に同じ盤面が待機状態で届く */
	QuestMovePC(pB, (int)dwMap, (int)dwX + 30, (int)dwY);
	QuestMovePC(pC, (int)dwMap, (int)dwX + 30, (int)dwY);
	QuestMovePC(pA, (int)dwMap, (int)dwX + 30, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	int nIdleAway = PuzzleIdleCount();
	n = PuzzleCount(anConn[1]);
	QuestMovePC(pB, (int)dwMap, (int)dwX + 5, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	PuzzlePackets(anConn[1], aP);
	bool bSame = ((int)aP.size() == n + 1) && (aP.back() == aLastB);
	printf("idle last board stays away_idleTables=%d returned_same=%d\n", nIdleAway, bSame);

	/* 7. 次に誰かが座ると、最後の盤面は消えて新しいゲーム（B は m1・盤面は空）。見ている人（A を呼び戻す）には m0・i0・空 */
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
	QuestMovePC(pB, (int)dwMap, (int)dwX, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	AddCharItem(pB->dwTblID, wCoin);
	for (i = 0; i < 2; i ++) an0[i] = PuzzleCount(anConn[i]);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleTalk(pB, pT);
	sA = PuzzleSummary(anConn[0], an0[0], &anN[0]);
	sB = PuzzleSummary(anConn[1], an0[1], &anN[1]);
	printf("idle sit again seatedB=%d A=%s x%d B=%s x%d\n", PuzzleIsSeated(pB), sA.c_str(), anN[0], sB.c_str(), anN[1]);

	/* 8. 席を立つ（ESC）と空の井戸に戻る（A にも、立った B にも待機状態。k1・m0・i1・c0） */
	for (i = 0; i < 2; i ++) an0[i] = PuzzleCount(anConn[i]);
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pB, PUZZLEKEY_LEAVE);
	sA = PuzzleSummary(anConn[0], an0[0], &anN[0]);
	sB = PuzzleSummary(anConn[1], an0[1], &anN[1]);
	printf("idle esc seatedB=%d A=%s x%d B=%s x%d seats=%d\n", PuzzleIsSeated(pB), sA.c_str(), anN[0], sB.c_str(), anN[1], PuzzleSeatCount());

	/* 9. 全員が遠くへ行くと、最後の盤面のない待機中の台は表から消える（何も送らず、覚えない） */
	for (i = 0; i < 3; i ++) QuestMovePC(apP[i], (int)dwMap, (int)dwX + 40, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("idle empty cleaned idleTables=%d\n", PuzzleIdleCount());

	/* 片付け */
	MockTrace(0);
	PuzzleOnLeave(pA); PuzzleOnLeave(pB); PuzzleOnLeave(pC);
	PuzzleRetireNpcs();
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	for (i = 0; i < 3; i ++) {
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
		QuestMovePC(apP[i], (int)aBack[i].dwMap, (int)aBack[i].dwX, (int)aBack[i].dwY);
		apP[i]->byCharState = aBack[i].byState;
		apP[i]->wHP = aBack[i].wHP;
	}
	g_dwTime = dwTimeBack;
	printf("idle done seats=%d idleTables=%d\n", PuzzleSeatCount(), PuzzleIdleCount());
}

/* ------------------------------------------------------------------------- */
/* AV4. 落ち物パズルの座る場所と、見えない台NPC								 */
/*     台詞 "@p<コイン>,<枚数>,<dx>,<dy>,<横>,<縦>,<帯>,<ペア>,<sdx>,<sdy>" の 9・10 番目が座る場所（台NPCのマスから）。	 */
/*     座る場所のマスで S → 座れる（向きは問わない・台NPCが目の前でなくてもよい）・台NPCのマスや隣のマスでは座れない・	 */
/*     sdx,sdy を指定した台では指定の場所に座れる（負の値も）・席を立たせる距離は座る場所のマスが基準・	 */
/*     台NPCの CMD_CHARINFO は体を描かない byGhost=CHARGHOST_NAMEONLY で送る（データは書き換えない・台でないNPCはそのまま）	 */
/* ------------------------------------------------------------------------- */
static int PuzzleGhostSent(PCHARINFO pViewer, PCHARINFO pNpc)
{
	std::vector<BYTE> aCap;
	CMDPARA_CHARINFO Para;

	SvrSendCaptureBegin();
	SendCmdCHARINFO(pViewer->pID, (int)pNpc->dwTblID);
	BOOL bRet = SvrSendCaptureEnd(aCap);
	ZeroMemory(&Para, sizeof(Para));
	if (!bRet) {
		return -1;
	}
	SboPackGetCHARINFO(&Para, &aCap[0], &aCap[0] + aCap.size());
	return (Para.dwTblID == pNpc->dwTblID) ? (int)Para.byGhost : -2;
}

static void TestPuzzleSeat(const std::vector<int> &aLogin)
{
	printf("== AV4. Falling puzzle seat cell ==\n");
	if (aLogin.size() < 2) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]];
	WORD wCoin = 1;
	DWORD dwXBack = pA->dwX, dwYBack = pA->dwY, dwMapBack = pA->dwMapID;
	BYTE byStateBack = pA->byCharState, byDirBack = pA->byDirection;
	WORD wHPBack = pA->wHP;
	DWORD dwTimeBack = g_dwTime;

	pA->byCharState = CHARSTATE_MOVE;
	if (pA->wHP == 0) pA->wHP = 10;
	for (int i = 0; i < MAX_MYITEM; i ++) if (pA->waItem[i] == wCoin) DelCharItem(pA->dwTblID, (BYTE)i);
	DWORD dwX = pA->dwX, dwY = pA->dwY;
	/* 台NPC は A のマスに置く。既定の座る場所は 1 つ下 */
	NearNpcOpen(pA, "@p1,1");
	PCHARINFO pT = s_aNearNpc.back();
	g_dwTime = (s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000);
	AddCharItem(pA->dwTblID, wCoin);

	/* 1. 台NPCのマス・隣のマス・その上下では座れず、コインも減らない。座る場所（NPC の 1 つ下）では向きを問わず座れる */
	{
		int anCell[5][2] = { {0, 0}, {1, 1}, {-1, 1}, {0, 2}, {0, -1} };
		int nSeated = 0;
		for (int i = 0; i < 5; i ++) {
			PuzzlePlace(pA, pT, (DWORD)((int)dwX + anCell[i][0]), (DWORD)((int)dwY + anCell[i][1]));
			pA->byDirection = 0;
			PuzzleTalkRaw(pA, pT);
			nSeated += PuzzleIsSeated(pA);
		}
		printf("seat other cells seated=%d coin=%d\n", nSeated, PuzzleCoinCount(pA, wCoin));
		PuzzlePlace(pA, pT, dwX, dwY + 1);
		pA->byDirection = 3;											/* 台NPCと反対側・横向きでもよい */
		PuzzleTalkRaw(pA, pT);
		printf("seat default cell seated=%d coin=%d x=%d y=%d\n", PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin),
			(int)(pA->dwX == dwX), (int)(pA->dwY == dwY + 1));
		PuzzleKeySend(pA, PUZZLEKEY_LEAVE);
	}

	/* 2. sdx,sdy を指定した台（右 5・上 2）。既定の場所では座れず、指定の場所では座れる。
	   座る場所から台NPCは 5 マス離れているが、席を立たせる距離は座る場所が基準なので座ったままでいられる。
	   座る場所から 4 マス離れると立たされる */
	PuzzlePlace(pA, pT, dwX, dwY);							/* 台NPC は人のいるマスに置かれる */
	NearNpcOpen(pA, "@p1,1,,,,,,,5,-2");
	PCHARINFO pT2 = s_aNearNpc.back();
	AddCharItem(pA->dwTblID, wCoin);
	PuzzlePlace(pA, pT2, dwX, dwY + 1);
	PuzzleTalkRaw(pA, pT2);
	int nDefault = PuzzleIsSeated(pA);
	PuzzlePlace(pA, pT2, dwX + 5, dwY - 2);
	PuzzleTalkRaw(pA, pT2);
	int nCustom = PuzzleIsSeated(pA);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	int nStay = PuzzleIsSeated(pA);
	PuzzlePlace(pA, pT2, dwX + 5 + 4, dwY - 2);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("seat custom default=%d custom=%d stay=%d away=%d coin=%d\n", nDefault, nCustom, nStay, PuzzleIsSeated(pA), PuzzleCoinCount(pA, wCoin));
	PuzzleOnLeave(pA);

	/* 3. 負の値（左 2・下 0）。台の近く 3 マス以内の制限は座る場所が基準 */
	PuzzlePlace(pA, pT, dwX, dwY);							/* 台NPC は人のいるマスに置かれる */
	NearNpcOpen(pA, "@p1,1,,,,,,,-2,0");
	PCHARINFO pT3 = s_aNearNpc.back();
	AddCharItem(pA->dwTblID, wCoin);
	PuzzlePlace(pA, pT3, dwX, dwY + 1);
	PuzzleTalkRaw(pA, pT3);
	int nOld = PuzzleIsSeated(pA);
	PuzzlePlace(pA, pT3, dwX - 2, dwY);
	PuzzleTalkRaw(pA, pT3);
	printf("seat negative default=%d left2=%d\n", nOld, PuzzleIsSeated(pA));
	PuzzleOnLeave(pA);

	/* 4. 別のマップ・別の空間からは座れない（同じ座標でも） */
	{
		DWORD dwSpaceBack = pA->dwSpaceID;
		AddCharItem(pA->dwTblID, wCoin);
		PuzzlePlace(pA, pT, dwX, dwY + 1);
		pA->dwSpaceID = dwSpaceBack + 1;
		PuzzleTalkRaw(pA, pT);
		int nSpace = PuzzleIsSeated(pA);
		pA->dwSpaceID = dwSpaceBack;
		printf("seat other space seated=%d\n", nSpace);
	}

	/* 5. 台NPCは体を描かない（CMD_CHARINFO の byGhost が CHARGHOST_NAMEONLY。名前は送ったまま）。
	   台NPCのデータは書き換えない・台でないNPC（byGhost=1 の霊も）はそのまま */
	PuzzlePlace(pA, pT, dwX, dwY + 1);
	NearNpcOpen(pA, "@6");
	PCHARINFO pShop = s_aNearNpc.back();
	NearNpcOpen(pA, "@6");
	PCHARINFO pGhost = s_aNearNpc.back();
	pGhost->byGhost = 1;
	pT2->byGhost = 1;										/* 台NPCに半透明の設定が入っていても、体を描かない値で送る */
	int nTable = PuzzleGhostSent(pA, pT), nShop = PuzzleGhostSent(pA, pShop), nGhost = PuzzleGhostSent(pA, pGhost), nPreset = PuzzleGhostSent(pA, pT2);
	printf("seat ghost table=%d shop=%d ghost=%d preset=%d data=%d%d func=%d%d\n", nTable, nShop, nGhost, nPreset, (int)pT->byGhost, (int)pT2->byGhost,
		(int)PuzzleGhostForSend(pT), (int)PuzzleGhostForSend(pShop));

	/* 片付け */
	PuzzleOnLeave(pA);
	PuzzleRetireNpcs();
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	for (int i = 0; i < MAX_MYITEM; i ++) if (pA->waItem[i] == wCoin) DelCharItem(pA->dwTblID, (BYTE)i);
	DeleteTblNoPC((int)pA->dwTblID);
	pA->dwMapID = dwMapBack; pA->dwX = dwXBack; pA->dwY = dwYBack;
	AddTblNoPC((int)pA->dwTblID);
	pA->byCharState = byStateBack; pA->wHP = wHPBack; pA->byDirection = byDirBack;
	g_dwTime = dwTimeBack;
	printf("seat done seats=%d idleTables=%d\n", PuzzleSeatCount(), PuzzleIdleCount());
}

/* ------------------------------------------------------------------------- */
/* AV5. 落ち物パズルの INSERT COIN の案内・連勝・ハイスコアと連勝記録					 */
/*     待機中の台の座る場所のマスに立つと、本人にだけ byIdle の PUZZLEIDLE_SEAT（必要な枚数が 0 なら FREE も）が届く・	 */
/*     離れると消える・変化が無ければ送り直さない・使われている台には付かない・座る場所の隣では座れない・	 */
/*     連勝数が勝ちで増え（byStreak・「N連勝！」）、負け・席を立つ・切断・ゲームオーバーで 0 に戻る（2 以上は連勝記録へ）・	 */
/*     ハイスコアは 1 キャラ 1 件で上位 10 件・同点は先着・保存して読み直せる・掲示板 @h の文・API の JSON			 */
/* ------------------------------------------------------------------------- */
/* その人が最後に受けた、台 dwNpc の CMD_PUZZLESTATE の byStreak（パケットが無ければ -1） */
static int PuzzleLastStreak(int nConn, DWORD dwNpc)
{
	std::vector<std::vector<BYTE> > aP;
	PuzzlePackets(nConn, aP);
	for (int i = (int)aP.size() - 1; i >= 0; i --) {
		if (aP[i].size() < sizeof(CMDPARA_PUZZLESTATE)) continue;
		const CMDPARA_PUZZLESTATE *pSt = (const CMDPARA_PUZZLESTATE *)&aP[i][0];
		if (pSt->dwNpc == dwNpc) return pSt->byKind == PUZZLESTATE_NONE ? -2 : pSt->byStreak;
	}
	return -1;
}

/* その人が最後に受けた、台 dwNpc の byIdle（パケットが無ければ -1） */
static int PuzzleLastIdle(int nConn, DWORD dwNpc)
{
	int nIdle = -1;
	PuzzleOverFlag(nConn, dwNpc, &nIdle);
	return nIdle;
}

/* 対戦中の p が置けなくなって負ける（盤面を詰めて、下キーで固定させる）。得点を決めておく */
static void PuzzleForceLose(PCHARINFO p, DWORD dwScore)
{
	PuzzleGame *g = PuzzleGetGame(p);
	if (g == NULL) return;
	g->dwScore = dwScore;
	g->Board.Init(10, 14);
	for (int c = 3; c <= 6; c ++) { g->Board.Set(c, 0, 1); g->Board.Set(c, 1, 1); }		/* 次の落ち物は必ず出せず、ゲームオーバー */
	g->nKind = 1; g->nRot = 0; g->nX = 0; g->nY = 12;
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(p, PUZZLEKEY_DOWN);
}

static void TestPuzzleRecord(const std::vector<int> &aLogin)
{
	printf("== AV5. Falling puzzle insert coin, win streak, records ==\n");
	if (aLogin.size() < 3) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]], pC = g_aCharInfo[aLogin[2]];
	PCHARINFO apP[3] = { pA, pB, pC };
	int anConn[3] = { DgTestConn(pA), DgTestConn(pB), DgTestConn(pC) };
	struct { DWORD dwMap, dwX, dwY; BYTE byState; WORD wHP; } aBack[3];
	WORD wCoin = 1;
	DWORD dwTimeBack = g_dwTime;
	int i;

	for (i = 0; i < 3; i ++) {
		aBack[i].dwMap = apP[i]->dwMapID; aBack[i].dwX = apP[i]->dwX; aBack[i].dwY = apP[i]->dwY;
		aBack[i].byState = apP[i]->byCharState; aBack[i].wHP = apP[i]->wHP;
		apP[i]->byCharState = CHARSTATE_MOVE;
		if (apP[i]->wHP == 0) apP[i]->wHP = 10;
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
	}
	DWORD dwMap = pA->dwMapID, dwX = pA->dwX, dwY = pA->dwY;
	for (i = 1; i < 3; i ++) QuestMovePC(apP[i], (int)dwMap, (int)dwX, (int)dwY);
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
	/* キャラ名は記録の名前になる（空の名前は載らない）ので、決めておく */
	char aszNameBack[3][MAX_CHARNAMEBUF];
	for (i = 0; i < 3; i ++) {
		strcpy(aszNameBack[i], apP[i]->szName);
	}
	strcpy(pA->szName, "甲"); strcpy(pB->szName, "乙"); strcpy(pC->szName, "丙");
	/* 記録は空から始める。日付は 2026-11-25（日本時間）に決め打ち */
	PuzzleRecordClear();
	g_dwPuzzleNowOverride = 1795575600u;
	printf("record date=%u empty=%d%d\n", PuzzleDateJst(), g_PuzzleScoreRec.nCount, g_PuzzleStreakRec.nCount);
	/* 台 1・2 はペア 5（座る場所は台NPCの下・右 4 の下）・台 3 は無料（座る場所は左 4 の下） */
	NearNpcOpen(pA, "@p1,1,,,,,,5,0,1");
	PCHARINFO pT1 = s_aNearNpc.back();
	NearNpcOpen(pA, "@p1,1,,,,,,5,4,1");
	PCHARINFO pT2 = s_aNearNpc.back();
	NearNpcOpen(pA, "@p0,0,,,,,,,-4,1");
	PCHARINFO pT3 = s_aNearNpc.back();
	MockTrace(1);
	g_dwTime = (s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000);

	/* 1. A は台 1 の座る場所・B は座る場所でないマス・C は無料の台 3 の座る場所に立つ。
	   A には台 1 の待機中の盤面に SEAT（byIdle=3）、C には台 3 に SEAT+FREE（7）が届く。B と、それぞれほかの台・ほかの人の座る場所には付かない（1） */
	PuzzlePlace(pA, pT1, dwX, dwY + 1);
	PuzzlePlace(pB, pT1, dwX + 1, dwY + 3);
	PuzzlePlace(pC, pT3, dwX - 4, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("insert flags A:t1=%d t2=%d t3=%d B:t1=%d t2=%d t3=%d C:t1=%d t2=%d t3=%d\n",
		PuzzleLastIdle(anConn[0], pT1->dwTblID), PuzzleLastIdle(anConn[0], pT2->dwTblID), PuzzleLastIdle(anConn[0], pT3->dwTblID),
		PuzzleLastIdle(anConn[1], pT1->dwTblID), PuzzleLastIdle(anConn[1], pT2->dwTblID), PuzzleLastIdle(anConn[1], pT3->dwTblID),
		PuzzleLastIdle(anConn[2], pT1->dwTblID), PuzzleLastIdle(anConn[2], pT2->dwTblID), PuzzleLastIdle(anConn[2], pT3->dwTblID));

	/* 2. 変化が無ければ送り直さない（動かずに見直しを重ねてもパケットは増えない） */
	int nA0 = PuzzleCount(anConn[0]), nC0 = PuzzleCount(anConn[2]);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("insert nochange packets A=%d C=%d\n", PuzzleCount(anConn[0]) - nA0, PuzzleCount(anConn[2]) - nC0);

	/* 3. 座る場所から 1 歩離れると消える（A に 1 回だけ届く）。戻るとまた出る */
	nA0 = PuzzleCount(anConn[0]);
	PuzzlePlace(pA, pT1, dwX + 1, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	int nOff = PuzzleLastIdle(anConn[0], pT1->dwTblID), nOffPk = PuzzleCount(anConn[0]) - nA0;
	PuzzlePlace(pA, pT1, dwX, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("insert leave off=%d packets=%d back=%d\n", nOff, nOffPk, PuzzleLastIdle(anConn[0], pT1->dwTblID));

	/* 4. 座る場所の隣（B のいるマス）で S を押しても座れない（台NPCのマスでも）。コインは減らない */
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalkRaw(pB, pT1);
	PuzzlePlace(pB, pT1, dwX, dwY);
	PuzzleTalkRaw(pB, pT1);
	printf("insert notcell seatedB=%d coinB=%d\n", PuzzleIsSeated(pB), PuzzleCoinCount(pB, wCoin));
	/* 無料の台 3 は、座る場所の隣では座れず、座る場所では座れる（コイン無し）。座ると案内は消える */
	PuzzlePlace(pC, pT3, dwX - 3, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	int nNear = PuzzleLastIdle(anConn[2], pT3->dwTblID);
	PuzzleTalkRaw(pC, pT3);
	int nSeatedNear = PuzzleIsSeated(pC);
	PuzzlePlace(pC, pT3, dwX - 4, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	int nBack = PuzzleLastIdle(anConn[2], pT3->dwTblID);
	PuzzleTalkRaw(pC, pT3);
	printf("insert free near=%d seatedNear=%d back=%d seated=%d idleAfter=%d\n", nNear, nSeatedNear, nBack, PuzzleIsSeated(pC),
		PuzzleLastIdle(anConn[2], pT3->dwTblID));
	/* C が ESC で席を立つと、座る場所に立ったままなので案内がすぐ戻る */
	PuzzleKeySend(pC, PUZZLEKEY_LEAVE);
	printf("insert free esc seated=%d idle=%d\n", PuzzleIsSeated(pC), PuzzleLastIdle(anConn[2], pT3->dwTblID));

	/* 5. コインが無ければ S を押しても座れず案内は残る。コインを持って S → 座る（案内は消え、本人の盤面になる）。
	   A が座っている台 1 の座る場所に立つ B には、使われている台なので案内は付かない */
	PuzzlePlace(pB, pT1, dwX + 1, dwY + 3);
	for (int j = 0; j < MAX_MYITEM; j ++) if (pA->waItem[j] == wCoin) DelCharItem(pA->dwTblID, (BYTE)j);
	PuzzleTalkRaw(pA, pT1);
	int nNoCoin = PuzzleIsSeated(pA), nStill = PuzzleLastIdle(anConn[0], pT1->dwTblID);
	AddCharItem(pA->dwTblID, wCoin);
	PuzzleTalkRaw(pA, pT1);
	PuzzlePlace(pB, pT1, dwX, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	printf("insert sit nocoin=%d still=%d seated=%d A:idle=%d B:idle=%d\n", nNoCoin, nStill, PuzzleIsSeated(pA),
		PuzzleLastIdle(anConn[0], pT1->dwTblID), PuzzleLastIdle(anConn[1], pT1->dwTblID));
	PuzzlePlace(pB, pT1, dwX + 1, dwY + 3);

	/* 6. 連勝：B が台 2（座る場所は右 4 の下）に座ると対戦。B が負けると A の連勝が 1 になり（「N連勝！」は 2 から）、盤面の通知に載る */
	g_dwTime = (s_dwFakeNow += 100);
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2);
	DWORD dwSit = g_dwTime;
	g_dwTime = (s_dwFakeNow = dwSit + 3100);
	PuzzleProc();
	printf("streak vs=%d%d start=%d\n", PuzzleIsVersus(pA), PuzzleIsVersus(pB), PuzzleWinStreak(pA));
	PuzzleForceLose(pB, 1234);
	printf("streak win1 A=%d B=%d seatedB=%d msgStreak=%d notice=%d pkt=%d\n", PuzzleWinStreak(pA), PuzzleWinStreak(pB), PuzzleIsSeated(pB),
		MockHasText(anConn[0], CMD_SYSMSG, "連勝！"), MockHasText(anConn[0], CMD_SYSMSG, "の勝ち！"), PuzzleLastStreak(anConn[0], pT1->dwTblID));
	printf("streak rec scores=%d streaks=%d\n", g_PuzzleScoreRec.nCount, g_PuzzleStreakRec.nCount);
	/* 2 連勝：もう一度 B が乱入して負ける → 「○○の 2 連勝！」が両者に出て、観覧者 C にも byStreak=2 が届く */
	PuzzlePlace(pC, pT3, dwX - 4, dwY + 1);
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2);
	g_dwTime = (s_dwFakeNow += 3100);
	PuzzleProc();
	int nMsgA = MockCmdCount(anConn[0], CMD_SYSMSG);
	PuzzleForceLose(pB, 1500);
	char szWin[96];
	sprintf(szWin, "%sの勝ち！ %sの2連勝！", pA->szName, pA->szName);
	printf("streak win2 A=%d msgA=%d msgB=%d msgNew=%d pktC=%d pktA=%d\n", PuzzleWinStreak(pA), MockHasText(anConn[0], CMD_SYSMSG, szWin),
		MockHasText(anConn[1], CMD_SYSMSG, szWin), MockCmdCount(anConn[0], CMD_SYSMSG) - nMsgA, PuzzleLastStreak(anConn[2], pT1->dwTblID),
		PuzzleLastStreak(anConn[0], pT1->dwTblID));
	/* B の得点は 1 キャラ 1 件で、高い方（1500）に更新される。連勝記録はまだ空（B は連勝していない・A は続行中） */
	printf("streak rec scoreB=%u n=%d streaks=%d\n", g_PuzzleScoreRec.nCount ? g_PuzzleScoreRec.aRec[0].dwValue : 0, g_PuzzleScoreRec.nCount, g_PuzzleStreakRec.nCount);

	/* 7. 連勝中の A が B に負ける（B が台 2 で乱入し、A が置けなくなる）→ A の 2 連勝が記録され、A は 0 に戻り、B は 1 連勝。
	   A の得点もハイスコアに載る（5000 で 1 位・B が 2 位） */
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2);
	g_dwTime = (s_dwFakeNow += 3100);
	PuzzleProc();
	PuzzleForceLose(pA, 5000);
	printf("streak lose A seated=%d streakA=%d B=%d recStreak=%d(%s:%u %u) recScore=%s:%u,%s:%u\n", PuzzleIsSeated(pA), PuzzleWinStreak(pA), PuzzleWinStreak(pB),
		g_PuzzleStreakRec.nCount, g_PuzzleStreakRec.nCount ? (strcmp(g_PuzzleStreakRec.aRec[0].szName, pA->szName) == 0 ? "A" : "?") : "-",
		g_PuzzleStreakRec.nCount ? g_PuzzleStreakRec.aRec[0].dwValue : 0, g_PuzzleStreakRec.nCount ? g_PuzzleStreakRec.aRec[0].dwDate : 0,
		g_PuzzleScoreRec.nCount > 0 ? (strcmp(g_PuzzleScoreRec.aRec[0].szName, pA->szName) == 0 ? "A" : "?") : "-", g_PuzzleScoreRec.nCount > 0 ? g_PuzzleScoreRec.aRec[0].dwValue : 0,
		g_PuzzleScoreRec.nCount > 1 ? (strcmp(g_PuzzleScoreRec.aRec[1].szName, pB->szName) == 0 ? "B" : "?") : "-", g_PuzzleScoreRec.nCount > 1 ? g_PuzzleScoreRec.aRec[1].dwValue : 0);

	/* 8. B（1 連勝）が席を立つと 0 に戻る（連勝記録には 2 未満なので載らない）。座り直しは 0 から */
	PuzzleKeySend(pB, PUZZLEKEY_LEAVE);
	int nRecAfterEsc = g_PuzzleStreakRec.nCount;
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2);
	printf("streak esc recStreak=%d sitAgain=%d\n", nRecAfterEsc, PuzzleWinStreak(pB));
	PuzzleKeySend(pB, PUZZLEKEY_LEAVE);

	/* 9. 2 連勝した B が切断すると記録される（A の 2 連勝と同点なので、先に達成した A が上）。
	   B が台 2 で座っている所へ A が台 1 に座って乱入し、A が負けるのを 2 回 */
	AddCharItem(pB->dwTblID, wCoin);
	PuzzleTalk(pB, pT2);
	for (i = 0; i < 2; i ++) {
		AddCharItem(pA->dwTblID, wCoin);
		PuzzleTalk(pA, pT1);
		g_dwTime = (s_dwFakeNow += 3100);
		PuzzleProc();
		PuzzleForceLose(pA, 10);
	}
	int nStreakB = PuzzleWinStreak(pB);
	PuzzleOnLeave(pB);
	printf("streak logout B=%d recStreak=%d first=%s second=%s values=%u,%u seatedB=%d\n", nStreakB, g_PuzzleStreakRec.nCount,
		g_PuzzleStreakRec.nCount > 0 && strcmp(g_PuzzleStreakRec.aRec[0].szName, pA->szName) == 0 ? "A" : "?",
		g_PuzzleStreakRec.nCount > 1 && strcmp(g_PuzzleStreakRec.aRec[1].szName, pB->szName) == 0 ? "B" : "?",
		g_PuzzleStreakRec.nCount > 0 ? g_PuzzleStreakRec.aRec[0].dwValue : 0, g_PuzzleStreakRec.nCount > 1 ? g_PuzzleStreakRec.aRec[1].dwValue : 0, PuzzleIsSeated(pB));

	/* 10. ひとり用のゲームオーバーでも得点が載る（1 キャラ 1 件：A の低い得点では更新しない）。ゲームオーバーで連勝は 0 */
	AddCharItem(pA->dwTblID, wCoin);
	PuzzleTalk(pA, pT1);
	PuzzleGame *gA = PuzzleGetGame(pA);
	gA->Board.Init(10, 14);
	for (int c = 3; c <= 6; c ++) { gA->Board.Set(c, 0, 1); gA->Board.Set(c, 1, 1); }
	gA->nKind = 1; gA->nRot = 0; gA->nX = 0; gA->nY = 12;
	gA->dwScore = 3000;
	g_dwTime = (s_dwFakeNow += 100);
	PuzzleKeySend(pA, PUZZLEKEY_DOWN);
	printf("solo over seatedA=%d scores=%d top=%u\n", PuzzleIsSeated(pA), g_PuzzleScoreRec.nCount, g_PuzzleScoreRec.aRec[0].dwValue);

	/* 11. 掲示板 @h：話しかけた本人にだけ、ハイスコアと連勝記録が会話の窓（CMD_TALKMSG）で届く。名前が長い時は切り詰める。 */
	g_PuzzleScoreRec.Add("あいうえおかきくけこさしすせそたちつてと", 7777, 20261125);
	g_PuzzleStreakRec.Add("ア@0イ", 9, 20261125);
	PuzzlePlace(pA, pT1, dwX + 6, dwY);
	NearNpcOpen(pA, "@h");
	PCHARINFO pH = s_aNearNpc.back();
	int nTalk0 = MockCmdCount(anConn[0], CMD_TALKMSG), nTalkB0 = MockCmdCount(anConn[1], CMD_TALKMSG);
	PuzzleTalkRaw(pA, pH);
	printf("board talk A=%d B=%d head=%d streakHead=%d name=%d cut=%d noat0=%d\n", MockCmdCount(anConn[0], CMD_TALKMSG) - nTalk0,
		MockCmdCount(anConn[1], CMD_TALKMSG) - nTalkB0, MockHasText(anConn[0], CMD_TALKMSG, "【ハイスコア】"), MockHasText(anConn[0], CMD_TALKMSG, "【連勝記録】"),
		MockHasText(anConn[0], CMD_TALKMSG, "7777点"), MockHasText(anConn[0], CMD_TALKMSG, "あいうえお…"), !MockHasText(anConn[0], CMD_TALKMSG, "ア@0イ"));
	{
		/* マップのメッセージ（調べるマス）が「@h」で始まる掲示板パーツ：調べた本人にだけ、ランキングが CMD_MAPMSG で届く。普通のメッセージはそのまま */
		PMAPINFO pMapB = g_aMapInfo[GetMapNo(pA->dwMapID)];
		LPSTR *ppOld = pMapB->pMsg;
		int nOldCount = pMapB->nMsgCount;
		LPSTR apBoardMsg[2] = { (LPSTR)"@h", (LPSTR)"ふつうの看板です。@2" };
		EVENTINFO_MSG EvMsg = { 0, 1, 0, 0, 0 };
		EVENTINFO Ev = { 0, 0, EVENTTYPE_MSG, &EvMsg };
		pMapB->pMsg = apBoardMsg;
		pMapB->nMsgCount = 2;
		int nMsgA0 = MockCmdCount(anConn[0], CMD_MAPMSG), nMsgB0 = MockCmdCount(anConn[1], CMD_MAPMSG);
		EventProc(pA, &Ev, 1);
		int nBoardA = MockCmdCount(anConn[0], CMD_MAPMSG) - nMsgA0, nBoardB = MockCmdCount(anConn[1], CMD_MAPMSG) - nMsgB0;
		BOOL bHead = MockHasText(anConn[0], CMD_MAPMSG, "【ハイスコア】"), bScore = MockHasText(anConn[0], CMD_MAPMSG, "7777点");
		EvMsg.dwNo = 1;
		EventProc(pA, &Ev, 1);
		printf("board parts A=%d B=%d head=%d score=%d plain=%d\n", nBoardA, nBoardB, bHead, bScore, MockHasText(anConn[0], CMD_MAPMSG, "ふつうの看板です。"));
		pMapB->pMsg = ppOld;
		pMapB->nMsgCount = nOldCount;
	}
	{
		char szMsg[MAX_TALKMSG];
		PuzzleRecordMessage(szMsg, sizeof(szMsg));
		int nLen = (int)strlen(szMsg), nWait = 0, nLine = 0;
		for (int k = 0; k + 1 < nLen; k ++) if (szMsg[k] == '@') { if (szMsg[k + 1] == '0') nWait ++; if (szMsg[k + 1] == '1') nLine ++; }
		printf("board msg len<max=%d end=%d waits=%d lines=%d\n", nLen < MAX_TALKMSG, nLen >= 2 && strcmp(szMsg + nLen - 2, "@2") == 0, nWait, nLine);
	}

	/* 12. 保存して読み直せる（名前・値・日付・並びが同じ）。ファイルが無い時（読み直し前に消した後）は空で始まる */
	{
		PuzzleRecordList sc = g_PuzzleScoreRec, st = g_PuzzleStreakRec;
		BOOL bSaved = SavePuzzleRecord();
		PuzzleRecordClear();
		int nCleared = g_PuzzleScoreRec.nCount + g_PuzzleStreakRec.nCount;
		BOOL bLoaded = LoadPuzzleRecord();
		int nSame = (g_PuzzleScoreRec.nCount == sc.nCount) && (g_PuzzleStreakRec.nCount == st.nCount);
		for (int k = 0; nSame && k < sc.nCount; k ++) nSame = strcmp(sc.aRec[k].szName, g_PuzzleScoreRec.aRec[k].szName) == 0 && sc.aRec[k].dwValue == g_PuzzleScoreRec.aRec[k].dwValue && sc.aRec[k].dwDate == g_PuzzleScoreRec.aRec[k].dwDate;
		for (int k = 0; nSame && k < st.nCount; k ++) nSame = strcmp(st.aRec[k].szName, g_PuzzleStreakRec.aRec[k].szName) == 0 && st.aRec[k].dwValue == g_PuzzleStreakRec.aRec[k].dwValue && st.aRec[k].dwDate == g_PuzzleStreakRec.aRec[k].dwDate;
		printf("record save=%d cleared=%d load=%d same=%d counts=%d,%d\n", bSaved, nCleared, bLoaded, nSame, g_PuzzleScoreRec.nCount, g_PuzzleStreakRec.nCount);
		/* 読み込んだ記録は、そのまま次の記録を足せる（1 キャラ 1 件・同点は先着が保たれる） */
		printf("record after load addLow=%d addHigh=%d\n", g_PuzzleScoreRec.Add("あいうえおかきくけこさしすせそたちつてと", 100, 1), g_PuzzleScoreRec.Add("あいうえおかきくけこさしすせそたちつてと", 8000, 20261126));
	}

	/* 13. API の JSON（読むだけ）。順位順・1 キャラ 1 件 */
	{
		std::string sJson = SboWebDataPuzzleRecords();
		printf("record api scores=%d streaks=%d max=%d top=%d date=%d\n", sJson.find("\"scores\":[{\"rank\":1,") != std::string::npos,
			sJson.find("\"streaks\":[{\"rank\":1,") != std::string::npos, sJson.find("\"max\":10") != std::string::npos,
			sJson.find("\"score\":8000") != std::string::npos, sJson.find("\"date\":\"2026-11-25\"") != std::string::npos);
	}

	/* 記録が空の時の掲示板（まだ記録がありません。が 2 回） */
	PuzzleRecordClear();
	{
		char szMsg[MAX_TALKMSG];
		int nEmpty = 0;
		PuzzleRecordMessage(szMsg, sizeof(szMsg));
		for (const char *p = szMsg; (p = strstr(p, "まだ記録がありません。")) != NULL; p ++) nEmpty ++;
		printf("board empty text=%d\n", nEmpty);
	}

	/* 10 件ずつ・長い名前・大きい値でも、1 つのメッセージ（MAX_TALKMSG バイト）に収まり、10 位まで出る */
	{
		char szMsg[MAX_TALKMSG], szName[PUZZLE_RECORD_NAMEBUF];
		for (int k = 1; k <= 12; k ++) {
			sprintf(szName, "ながいなまえのひとのなまえ%02d", k);
			g_PuzzleScoreRec.Add(szName, 4000000000u - k, 20261125);
			g_PuzzleStreakRec.Add(szName, 200 - k, 20261125);
		}
		PuzzleRecordMessage(szMsg, sizeof(szMsg));
		int nLen = (int)strlen(szMsg), nTen = 0;
		for (const char *p = szMsg; (p = strstr(p, "10位")) != NULL; p ++) nTen ++;
		printf("board full counts=%d,%d len<max=%d tenth=%d end=%d\n", g_PuzzleScoreRec.nCount, g_PuzzleStreakRec.nCount, nLen < MAX_TALKMSG, nTen, strcmp(szMsg + nLen - 2, "@2") == 0);
	}

	/* 片付け */
	MockTrace(0);
	PuzzleOnLeave(pA); PuzzleOnLeave(pB); PuzzleOnLeave(pC);
	PuzzleRetireNpcs();
	for (size_t k = 0; k < s_aNearNpc.size(); k ++) {
		if (PuzzleIsRecordNpc(s_aNearNpc[k])) {
			delete [] s_aNearNpc[k]->pszNpcSpeech;
			s_aNearNpc[k]->pszNpcSpeech = new char[1];
			s_aNearNpc[k]->pszNpcSpeech[0] = '\0';
		}
	}
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	PuzzleProc();
	for (i = 0; i < 3; i ++) {
		for (int j = 0; j < MAX_MYITEM; j ++) if (apP[i]->waItem[j] == wCoin) DelCharItem(apP[i]->dwTblID, (BYTE)j);
		QuestMovePC(apP[i], (int)aBack[i].dwMap, (int)aBack[i].dwX, (int)aBack[i].dwY);
		apP[i]->byCharState = aBack[i].byState;
		apP[i]->wHP = aBack[i].wHP;
	}
	g_dwPuzzleNowOverride = 0;
	for (i = 0; i < 3; i ++) {
		strcpy(apP[i]->szName, aszNameBack[i]);
	}
	PuzzleRecordClear();
	SavePuzzleRecord();
	g_dwTime = dwTimeBack;
	printf("record done seats=%d\n", PuzzleSeatCount());
}

/* ------------------------------------------------------------------------- */
/* AW. 迷路対戦の台（台詞 "@k<dx>,<dy>,<sdx>,<sdy>" の NPC。段階 1＝ひとり練習）				 */
/*     台詞の解析（既定・省略・負の値・依頼の見えるNPCは台でない）・座る場所で S → 座る・		 */
/*     隣では座れない・使用中は座れない・座っている間は歩けない・進む・壁を置く・				 */
/*     不正な操作（壁の重なり・十字・範囲外・進めない向き・知らない操作）は無視して状態を送り直す・	 */
/*     勝つと 3 秒盤を見せて（その間は操作を受けない）座ったまま戻る・やり直し・席を立つ・		 */
/*     近くの人（観戦）への盤の送り始め・消す距離・離れる／切断で片付く						 */
/* ------------------------------------------------------------------------- */
static void MazePackets(int nConn, std::vector<std::vector<BYTE> > &aOut)
{
	char *psz = MockCmdAll(nConn, CMD_MAZESTATE);
	size_t nLen = strlen(psz), nPos = 0;
	aOut.clear();
	while (nPos < nLen) {
		size_t nEnd = nPos;
		while (nEnd < nLen && psz[nEnd] != ',') nEnd ++;
		std::vector<BYTE> a((nEnd - nPos) / 2, 0);
		for (size_t i = 0; i < a.size(); i ++) {
			unsigned int u = 0;
			sscanf(psz + nPos + i * 2, "%2x", &u);
			a[i] = (BYTE)u;
		}
		aOut.push_back(a);
		nPos = nEnd + 1;
	}
	free(psz);
}

/* 覚えた CMD_MAZESTATE の数 */
static int MazeCount(int nConn)
{
	std::vector<std::vector<BYTE> > aP;
	MazePackets(nConn, aP);
	return (int)aP.size();
}

/* 最後に受けた CMD_MAZESTATE（無ければ FALSE。台 dwNpc のものだけ見る） */
static BOOL MazeLast(int nConn, DWORD dwNpc, CMDPARA_MAZESTATE *pOut)
{
	std::vector<std::vector<BYTE> > aP;
	MazePackets(nConn, aP);
	for (int i = (int)aP.size() - 1; i >= 0; i --) {
		if (aP[i].size() != sizeof(CMDPARA_MAZESTATE)) continue;
		memcpy(pOut, &aP[i][0], sizeof(CMDPARA_MAZESTATE));
		if (pOut->dwNpc == dwNpc) return TRUE;
	}
	return FALSE;
}

static void MazeKeySend(PCHARINFO p, BYTE byAct, BYTE byDir = 0, BYTE byX = 0, BYTE byY = 0, BYTE byOrient = 0)
{
	CMDPARA_MAZEKEY Para;
	ZeroMemory(&Para, sizeof(Para));
	Para.byCmd = CMD_MAZEKEY;
	Para.byAct = byAct;
	Para.byDir = byDir;
	Para.byX = byX;
	Para.byY = byY;
	Para.byOrient = byOrient;
	TestRecv(p, &Para, sizeof(Para));
}

/* 台の座る場所のマスに立って S キーで話しかける */
static void MazeTalk(PCHARINFO p, PCHARINFO pNpc)
{
	int nDx, nDy, nSdx, nSdy;
	MazeParseTable(pNpc->pszNpcSpeech, &nDx, &nDy, &nSdx, &nSdy);
	PuzzlePlace(p, pNpc, (DWORD)((int)pNpc->dwX + nSdx), (DWORD)((int)pNpc->dwY + nSdy));
	PuzzleKeyS(p, pNpc);
}

/* 駒の位置・残りの壁・手番・勝者・状態ビットを 1 つの文字列にして見やすくする */
static std::string MazeBrief(int nConn, DWORD dwNpc)
{
	CMDPARA_MAZESTATE St;
	char sz[160];
	if (!MazeLast(nConn, dwNpc, &St)) return "-";
	if (St.byKind == MAZEKIND_NONE) return "none";
	sprintf(sz, "p0=%d,%d p1=%d,%d left=%d/%d walls=%d turn=%d win=%d st=%d", St.byPosX[0], St.byPosY[0], St.byPosX[1], St.byPosY[1],
		St.byWallLeft[0], St.byWallLeft[1], St.byWallCount, St.byTurn, St.cWinner, St.byState);
	return sz;
}

static void TestMaze(const std::vector<int> &aLogin)
{
	printf("== AW. Maze attack table ==\n");
	if (aLogin.size() < 2) return;
	s_pSpLogin = &aLogin;
	PCHARINFO pA = g_aCharInfo[aLogin[0]], pB = g_aCharInfo[aLogin[1]];
	int nConnA = DgTestConn(pA), nConnB = DgTestConn(pB);
	DWORD dwMapA = pA->dwMapID, dwXA = pA->dwX, dwYA = pA->dwY, dwMapB = pB->dwMapID, dwXB = pB->dwX, dwYB = pB->dwY;
	BYTE byStateA = pA->byCharState, byStateB = pB->byCharState;
	WORD wHPA = pA->wHP, wHPB = pB->wHP;
	DWORD dwTimeBack = g_dwTime;

	pA->byCharState = CHARSTATE_MOVE;
	pB->byCharState = CHARSTATE_MOVE;
	if (pA->wHP == 0) pA->wHP = 10;
	if (pB->wHP == 0) pB->wHP = 10;
	QuestMovePC(pB, (int)pA->dwMapID, (int)pA->dwX + 40, (int)pA->dwY);			/* B は同じマップの遠く */
	DWORD dwMap = pA->dwMapID, dwX = pA->dwX, dwY = pA->dwY;

	/* 1. 台詞の解析：既定・全部指定・空欄・上限・後ろの省略。台でない NPC（@p・依頼の見えるNPC）は台として扱わない */
	{
		int dx, dy, sdx, sdy;
		MazeParseTable("@k", &dx, &dy, &sdx, &sdy);
		printf("maze parse default %d,%d,%d,%d\n", dx, dy, sdx, sdy);
		MazeParseTable("@k3,-4,5,-2", &dx, &dy, &sdx, &sdy);
		printf("maze parse all %d,%d,%d,%d\n", dx, dy, sdx, sdy);
		MazeParseTable("@k,,3", &dx, &dy, &sdx, &sdy);
		printf("maze parse blank %d,%d,%d,%d\n", dx, dy, sdx, sdy);
		MazeParseTable("@k999,-999,x,2", &dx, &dy, &sdx, &sdy);
		printf("maze parse clamp %d,%d,%d,%d\n", dx, dy, sdx, sdy);
		NearNpcOpen(pA, "@k");
		PCHARINFO pPlain = s_aNearNpc.back();
		NearNpcOpen(pA, "@p");
		PCHARINFO pPuzzleNpc = s_aNearNpc.back();
		NearNpcOpen(pA, "@q");
		PCHARINFO pQuest = s_aNearNpc.back();
		pQuest->byNpcType = NPCTYPE_QUESTNPC;
		printf("maze isTable q=%d puzzle=%d questnpc=%d\n", MazeIsTableNpc(pPlain), MazeIsTableNpc(pPuzzleNpc), MazeIsTableNpc(pQuest));
		pQuest->byNpcType = NPCTYPE_NOMOVE;
		for (int i = 0; i < 3; i ++) {
			PCHARINFO pNpc = s_aNearNpc[s_aNearNpc.size() - 1 - i];
			delete [] pNpc->pszNpcSpeech;
			pNpc->pszNpcSpeech = new char[1];
			pNpc->pszNpcSpeech[0] = '\0';
		}
	}

	/* 台（既定：盤の左上 -5,-9・座る場所は台NPCの 1 つ下）。A のマスに置く */
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
	NearNpcOpen(pA, "@k");
	PCHARINFO pNpc = s_aNearNpc.back();
	DWORD dwNpc = pNpc->dwTblID;
	MockTrace(1);
	g_dwTime = (s_dwFakeNow = max(s_dwFakeNow, g_dwTime) + 1000);

	/* 2. 座る場所以外（台NPCのマス・横の隣・2 つ下）では座れない。座る場所では向きを問わず座れる */
	{
		int anCell[3][2] = { {0, 0}, {1, 1}, {0, 2} };
		int nSeated = 0;
		for (int i = 0; i < 3; i ++) {
			PuzzlePlace(pA, pNpc, (DWORD)((int)dwX + anCell[i][0]), (DWORD)((int)dwY + anCell[i][1]));
			PuzzleKeyS(pA, pNpc);
			nSeated += MazeIsSeated(pA);
		}
		printf("maze other cells seated=%d state=%d\n", nSeated, MazeCount(nConnA));
	}

	/* 3. 座る：盤全体 FULL が届く（本人=SEAT ビット・待機でない・A がプレイヤー 0・相手は不在・盤の左上は台から決まる） */
	pA->byDirection = 3;
	MazeTalk(pA, pNpc);
	{
		CMDPARA_MAZESTATE St;
		BOOL bOK = MazeLast(nConnA, dwNpc, &St);
		printf("maze sit seated=%d seats=%d got=%d kind=%d seat=%d idle=%d c0=%d c1=%d\n", MazeIsSeated(pA), MazeSeatCount(), bOK,
			bOK ? St.byKind : -1, bOK ? (St.byState & MAZESTATE_SEAT) != 0 : -1, bOK ? (St.byState & MAZESTATE_IDLE) != 0 : -1,
			bOK ? St.dwChar[0] == pA->dwTblID : -1, bOK ? St.dwChar[1] == MAZE_NOCHAR : -1);
		printf("maze sit board x=%d y=%d size=%d\n", bOK ? St.sBoardX == (short)(((int)dwX - 5) * 32) : -1,
			bOK ? St.sBoardY == (short)(((int)dwY - 9) * 32) : -1, (int)sizeof(CMDPARA_MAZESTATE));
		printf("maze sit %s\n", MazeBrief(nConnA, dwNpc).c_str());
	}

	/* 4. 使用中の台には座れない（B を座る場所へ呼ぶ）・座っている間は歩けない */
	QuestMovePC(pB, (int)dwMap, (int)dwX, (int)dwY + 1);
	PuzzleKeyS(pB, pNpc);
	printf("maze busy seatedB=%d msg=%d\n", MazeIsSeated(pB), MockHasText(nConnB, CMD_SYSMSG, "この台は今、使われています"));
	QuestMovePC(pB, (int)dwMap, (int)dwX + 40, (int)dwY);
	{
		CMDPARA_CHARPOS Pos;
		ZeroMemory(&Pos, sizeof(Pos));
		Pos.byCmd = CMD_CHARPOS;
		Pos.dwTblID = pA->dwTblID;
		Pos.dwMapID = pA->dwMapID;
		Pos.byX = (BYTE)(pA->dwX + 1);
		Pos.byY = (BYTE)pA->dwY;
		Pos.byDirection = 3;
		DWORD dwX0 = pA->dwX;
		TestRecv(pA, &Pos, sizeof(Pos));
		printf("maze nowalk moved=%d\n", pA->dwX != dwX0);
	}

	/* 5. 進む：上へ 1 マス。手番は自動で戻る（turn=0）。進めない向き（盤の外）は無視して状態を送り直す */
	int n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze move up %s +%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_RIGHT);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_RIGHT);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_RIGHT);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_RIGHT);									/* 3 回目で右端 (6,5)、4 回目は盤の外 */
	printf("maze move right %s +%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);
	for (int i = 0; i < 3; i ++) MazeKeySend(pA, MAZEACT_MOVE, MAZE_LEFT);			/* 真ん中の列へ戻る (3,5) */
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, 9);											/* 知らない向き */
	MazeKeySend(pA, 77);														/* 知らない操作 */
	printf("maze move invalid %s +%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);

	/* 6. 壁を置く：残りが 1 枚減り、手番は戻る。置いた壁の向こうへは進めない */
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_WALL, 0, 3, 4, MAZE_H);								/* (3,5) の真上の境目（x=3〜4 列） */
	{
		CMDPARA_MAZESTATE St;
		BOOL bOK = MazeLast(nConnA, dwNpc, &St);
		printf("maze wall %s +%d wall0=%d,%d,%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0,
			bOK ? St.aWall[0].byX : -1, bOK ? St.aWall[0].byY : -1, bOK ? St.aWall[0].byOrient : -1);
	}
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze blocked %s +%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);

	/* 7. 不正な壁（どれも無視して状態を送り直す。壁の数・残りは変わらない）：同じ向きで重なる／十字／盤の外／知らない向き */
	{
		int anWall[6][3] = { {2, 4, MAZE_H}, {4, 4, MAZE_H}, {3, 4, MAZE_V}, {6, 0, MAZE_H}, {0, 6, MAZE_V}, {1, 1, 2} };
		const char *apName[6] = { "overlapL", "overlapR", "cross", "outX", "outY", "orient" };
		for (int i = 0; i < 6; i ++) {
			n0 = MazeCount(nConnA);
			MazeKeySend(pA, MAZEACT_WALL, 0, (BYTE)anWall[i][0], (BYTE)anWall[i][1], (BYTE)anWall[i][2]);
			printf("maze badwall %s %s +%d\n", apName[i], MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);
		}
	}

	/* 8. 勝つ：(3,5) から左へ 1・上へ 5 で最上段へ着く。勝ちを見せている間（WIN ビット）は操作を受けない */
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_LEFT);
	for (int i = 0; i < 5; i ++) MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze win %s\n", MazeBrief(nConnA, dwNpc).c_str());
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_DOWN);
	MazeKeySend(pA, MAZEACT_RESTART);
	MazeKeySend(pA, MAZEACT_WALL, 0, 0, 3, MAZE_V);
	printf("maze win locked %s +%d seated=%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0, MazeIsSeated(pA));
	/* 3 秒たつ前は戻らず、過ぎると座ったまま盤が初期状態に戻る（WIN ビットが消え、勝者は未決着）。S キーは要らない */
	g_dwTime = (s_dwFakeNow += 2000);
	MazeProc();
	std::string strBefore = MazeBrief(nConnA, dwNpc);
	g_dwTime = (s_dwFakeNow += 1100);
	MazeProc();
	printf("maze win reset early=[%s] later=[%s] seated=%d\n", strBefore.c_str(), MazeBrief(nConnA, dwNpc).c_str(), MazeIsSeated(pA));

	/* 9. やり直し（途中で盤を初期状態へ）。戻した後も続けて指せる */
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	MazeKeySend(pA, MAZEACT_WALL, 0, 0, 3, MAZE_V);
	MazeKeySend(pA, MAZEACT_RESTART);
	printf("maze restart %s\n", MazeBrief(nConnA, dwNpc).c_str());
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze after restart %s\n", MazeBrief(nConnA, dwNpc).c_str());

	/* 9b. 自分のコマの行き場をなくす壁は置けない：左下の隅 (0,6) へ行って上に横壁 (0,5)・右に縦壁 (1,5) を置くと、
	   (0,6)(1,6) の 2 マスに閉じ込められる（最後の 1 枚は経路が無くなるので無視され、状態が送り直される） */
	MazeKeySend(pA, MAZEACT_RESTART);
	for (int i = 0; i < 3; i ++) MazeKeySend(pA, MAZEACT_MOVE, MAZE_LEFT);
	MazeKeySend(pA, MAZEACT_WALL, 0, 0, 5, MAZE_H);
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_WALL, 0, 1, 5, MAZE_V);
	printf("maze nopath %s +%d\n", MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);
	MazeKeySend(pA, MAZEACT_RESTART);

	/* 10. 座っていない人（B）の操作は無視（何も送られない・盤も変わらない） */
	n0 = MazeCount(nConnB);
	MazeKeySend(pB, MAZEACT_MOVE, MAZE_UP);
	MazeKeySend(pB, MAZEACT_RESTART);
	printf("maze otherkey packets=%d %s\n", MazeCount(nConnB) - n0, MazeBrief(nConnA, dwNpc).c_str());

	/* 11. 席を立つ（ESC）→ 片付く（待機中の盤が本人に届く）。立った後の操作は無視 */
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_LEAVE);
	printf("maze leave seated=%d seats=%d %s +%d\n", MazeIsSeated(pA), MazeSeatCount(), MazeBrief(nConnA, dwNpc).c_str(), MazeCount(nConnA) - n0);
	n0 = MazeCount(nConnA);
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze leave then key packets=%d\n", MazeCount(nConnA) - n0);

	/* 12. 待機中の台は近くの人に初期配置の盤（IDLE ビット）を送る。送り始めは 17 マス（盤の中心から。盤の半分 5＋余白 9＋3）・消すのは 20 マスの外 */
	{
		int nB0 = MazeCount(nConnB);
		QuestMovePC(pB, (int)dwMap, (int)dwX + 18, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		MazeProc();
		int nFar = MazeCount(nConnB) - nB0;
		QuestMovePC(pB, (int)dwMap, (int)dwX + 17, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		MazeProc();
		printf("maze watch approach 18=%d 17=[%s] +%d idleTables=%d\n", nFar, MazeBrief(nConnB, dwNpc).c_str(), MazeCount(nConnB) - nB0 - nFar, MazeIdleCount());
		/* 境目（18〜20）をうろうろしても送ったり消したりしない・21 マスで NONE */
		nB0 = MazeCount(nConnB);
		for (int i = 18; i <= 20; i ++) {
			QuestMovePC(pB, (int)dwMap, (int)dwX + i, (int)dwY);
			g_dwTime = (s_dwFakeNow += 300);
			MazeProc();
		}
		printf("maze watch hysteresis packets=%d\n", MazeCount(nConnB) - nB0);
		QuestMovePC(pB, (int)dwMap, (int)dwX + 21, (int)dwY);
		g_dwTime = (s_dwFakeNow += 300);
		MazeProc();
		printf("maze watch leave %s +%d\n", MazeBrief(nConnB, dwNpc).c_str(), MazeCount(nConnB) - nB0);
	}

	/* 13. 観戦：B が近くにいる間に A が座ると、B に始まった盤（SEAT でない・A のキャラ）が届き、A の手が B にも届く */
	QuestMovePC(pB, (int)dwMap, (int)dwX + 5, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	MazeProc();
	MazeTalk(pA, pNpc);
	{
		CMDPARA_MAZESTATE St;
		BOOL bOK = MazeLast(nConnB, dwNpc, &St);
		printf("maze watch sit %s seat=%d idle=%d chA=%d\n", MazeBrief(nConnB, dwNpc).c_str(), bOK ? (St.byState & MAZESTATE_SEAT) != 0 : -1,
			bOK ? (St.byState & MAZESTATE_IDLE) != 0 : -1, bOK ? St.dwChar[0] == pA->dwTblID : -1);
	}
	MazeKeySend(pA, MAZEACT_MOVE, MAZE_UP);
	printf("maze watch move B=%s\n", MazeBrief(nConnB, dwNpc).c_str());
	/* 観戦中の B の操作は受けない */
	MazeKeySend(pB, MAZEACT_MOVE, MAZE_UP);
	printf("maze watch Bkey A=%s\n", MazeBrief(nConnA, dwNpc).c_str());

	/* 14. 座る場所から離れる（マップ移動など）と周期処理で片付く（A への NONE は無く待機の盤）。B は待機の盤を受ける */
	QuestMovePC(pA, (int)dwMap, (int)dwX + 8, (int)dwY);
	g_dwTime = (s_dwFakeNow += 300);
	MazeProc();
	printf("maze away seated=%d seats=%d B=%s\n", MazeIsSeated(pA), MazeSeatCount(), MazeBrief(nConnB, dwNpc).c_str());

	/* 15. 倒れたら片付く */
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY + 1);
	MazeTalk(pA, pNpc);
	int nSeatedBeforeDead = MazeIsSeated(pA);
	pA->byCharState = CHARSTATE_DEAD;
	g_dwTime = (s_dwFakeNow += 300);
	MazeProc();
	printf("maze dead before=%d after=%d\n", nSeatedBeforeDead, MazeIsSeated(pA));
	pA->byCharState = CHARSTATE_MOVE;

	/* 16. 切断（OnLeave）で片付く。本人には何も送らない。見ている B には待機の盤が届く */
	QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY + 1);
	MazeTalk(pA, pNpc);
	{
		int nSeatedBefore = MazeIsSeated(pA);
		int nA1 = MazeCount(nConnA), nB1 = MazeCount(nConnB);
		MazeOnLeave(pA);
		printf("maze logout before=%d after=%d seats=%d A+%d B+%d B=%s\n", nSeatedBefore, MazeIsSeated(pA), MazeSeatCount(),
			MazeCount(nConnA) - nA1, MazeCount(nConnB) - nB1, MazeBrief(nConnB, dwNpc).c_str());
	}

	/* 17. 落ち物パズルの席に座っている人は迷路に座れない（ふたつの席に同時に座らない） */
	{
		QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY);
		NearNpcOpen(pA, "@p");
		PCHARINFO pPuz = s_aNearNpc.back();
		QuestMovePC(pA, (int)dwMap, (int)dwX, (int)dwY + 1);
		PuzzleKeyS(pA, pPuz);
		int nPuz = PuzzleIsSeated(pA);
		PuzzleKeyS(pA, pNpc);
		printf("maze exclusive puzzle=%d maze=%d\n", nPuz, MazeIsSeated(pA));
		PuzzleOnLeave(pA);
	}

	/* 片付け */
	MockTrace(0);
	MazeOnLeave(pA);
	MazeOnLeave(pB);
	PuzzleRetireNpcs();
	for (size_t i = 0; i < s_aNearNpc.size(); i ++) {
		if (MazeIsTableNpc(s_aNearNpc[i])) {
			delete [] s_aNearNpc[i]->pszNpcSpeech;
			s_aNearNpc[i]->pszNpcSpeech = new char[1];
			s_aNearNpc[i]->pszNpcSpeech[0] = '\0';
		}
	}
	NearNpcClose();
	g_dwTime = (s_dwFakeNow += 300);
	MazeProc();									/* 台 NPC が無くなった待機中の台を表から消す */
	PuzzleProc();
	QuestMovePC(pB, (int)dwMapB, (int)dwXB, (int)dwYB);
	QuestMovePC(pA, (int)dwMapA, (int)dwXA, (int)dwYA);
	pA->byCharState = byStateA; pA->wHP = wHPA;
	pB->byCharState = byStateB; pB->wHP = wHPB;
	g_dwTime = dwTimeBack;
	printf("maze done seats=%d idleTables=%d\n", MazeSeatCount(), MazeIdleCount());
}
