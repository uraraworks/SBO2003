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
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH2.h"
#include "CharProcSEARCH.h"
#include "CharProcMOVERAND.h"
/* 傭兵の道さがしを中から確かめるため、protected も見えるようにして読み込む */
#define protected public
#include "CharProcMERCENARY.h"
#undef protected
#include "SboSvrSubGameProcSugoroku.h"
#include "SboSvrMagicProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrItemProc.h"
#include "common/crc.h"
#include <emscripten.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
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
static void TestMercenary(const std::vector<int> &aLogin);
static void TestTailMapChg(const std::vector<int> &aLogin);
static void TestQuest(const std::vector<int> &aLogin);
static void TestSortItem(const std::vector<int> &aLogin);
static void TestCharPush(const std::vector<int> &aLogin);
static void TestLeaveDispose(const std::vector<int> &aLogin);
static void TestChallenge(const std::vector<int> &aLogin);
static void TestStatusUp(const std::vector<int> &aLogin);
static void TestMagicWall(const std::vector<int> &aLogin);

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
	struct { BYTE byLevel; BYTE bySel; } aCase[] = { {15, 0}, {15, 1}, {15, 3}, {15, 4}, {10, 3}, {1, 1}, {5, 1} };
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
	/* M. ギルドの依頼 */
	TestQuest(aLogin);
	/* N. 道具欄・預かり所の並べ替え */
	TestSortItem(aLogin);
	/* O. 押せるNPCを押す向き */
	TestCharPush(aLogin);
	/* P. 預かり所の品を引き取らずに売る・捨てる */
	TestLeaveDispose(aLogin);
	/* Q. 討伐チャレンジ */
	TestChallenge(aLogin);
	/* R. 攻撃力・守備力アップ */
	TestStatusUp(aLogin);
	/* S. 前方・周囲の特技が壁越しに届かない */
	TestMagicWall(aLogin);
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

/* 受信したことにして処理する（送ってきた接続のキャラとして） */
static void TestRecv(PCHARINFO pChar, const void *pData, DWORD dwSize)
{
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

static void TestMercenary(const std::vector<int> &aLogin)
{
	printf("== K. Mercenary ==\n");
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
	pOther->abyQuestDone[0] = 0x06; pOther->abyQuestDone[63] = 0x80;
	strcpy(pOther->szAcc, "questtest");
	strcpy(pOther->szName, "依頼テスト");
	BOOL bSaved = SaveCharInfo();
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
	} else {
		printf("quest char reload not found\n");
	}
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
	}

	/* 片付け */
	pOther->szAcc[0] = 0;
	for (int i = 0; i < 4; i ++) {
		apPC[i]->wQuestID = 0; apPC[i]->byQuestStep = 0; apPC[i]->wQuestFlg = 0;
		ZeroMemory(apPC[i]->awQuestCount, sizeof(apPC[i]->awQuestCount));
		ZeroMemory(apPC[i]->abyQuestDone, sizeof(apPC[i]->abyQuestDone));
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
		aBack[i] = *apPC[i];
		PCHARINFO p = apPC[i];
		p->byAdminLV = ADMINLV_USER;
		p->dwPartyID = 0;
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
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
	QuestSendTalk(pA, pGate->dwTblID);
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

	/* 周期処理：まだ何も起きない */
	QuestChallengeProcNow();
	printf("challenge proc a=%d b=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB));

	/* B が倒れる：B だけ中断して元の状態・入る前の場所へ。依頼は受けたまま */
	pB->wHP = 0;
	pB->byCharState = CHARSTATE_DEAD;
	QuestChallengeProcNow();
	ChallengePrint("dead b", pB, wWeapon);
	printf("challenge dead b state=%u at_start=%d quest=%u a_in=%d\n", pB->byCharState, (int)(pB->dwX == dwBX && pB->dwY == dwBY), pB->wQuestID, (int)QuestInChallenge(pA));

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
	if (pGate) {
		QuestSendTalk(pA, pGate->dwTblID);
	}
	printf("challenge again in a=%d b=%d enemies=%d\n", (int)QuestInChallenge(pA), (int)QuestInChallenge(pB), ChallengeCountEnemy());
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

	/* 片付け */
	QuestSendParty(pA, pA, CMDPARTY_BREAKUP);
	for (int i = 0; i < 3; i ++) {
		CHARINFO *pBk = &aBack[i];
		PCHARINFO p = apPC[i];
		p->wQuestID = 0; p->byQuestStep = 0; p->wQuestFlg = 0;
		ZeroMemory(p->abyQuestDone, sizeof(p->abyQuestDone));
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
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *pszData = (argc >= 2) ? argv[1] : NULL;
	if (pszData && chdir(pszData) != 0) {
		fprintf(stderr, "chdir failed\n");
		return 1;
	}
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
