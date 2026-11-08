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
 */
#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcSEARCH2.h"
#include "SboSvrSubGameProcSugoroku.h"
#include "common/crc.h"
#include <emscripten.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>

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
			CTestSearch2 Proc;
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
extern void TownWarpRecord(PCHARINFO pChar);

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
	pPC->byTownWarpCount = 0;
	/* 教会で記録する順：始まりの町→港町→マイス→ルシー→港町（上に戻る）→サニアス→バオ（一番古いのが押し出される） */
	static const struct { DWORD dwMap; BYTE x, y; } aRec[] = {
		{0, 26, 50}, {14, 9, 12}, {35, 13, 30}, {39, 51, 40}, {14, 9, 12}, {43, 32, 50}, {51, 35, 6},
	};
	for (size_t i = 0; i < sizeof(aRec) / sizeof(aRec[0]); i ++) {
		pPC->dwStartMapID = aRec[i].dwMap;
		pPC->dwStartX = aRec[i].x;
		pPC->dwStartY = aRec[i].y;
		TownWarpRecord(pPC);
		char sz[32];
		sprintf(sz, "rec%d", (int)i);
		PrintTownWarp(sz, pPC);
	}
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
	TestSearch2();

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
			printf("charinfo %s ret=%d tbl=%d same=%d login=%u play=%u make=%u area=%u put=%u interval=%u move=%u search=%u drop=%u sttmp=%d\n",
				aCase[c].pszLabel, bRet, (int)(Para.dwTblID == aCase[c].pChar->dwTblID), (int)(strcmp(Para.szName, aCase[c].pChar->szName) == 0),
				Para.dwLoginCount, Para.dwPlayTime, Para.wMakeYear, Para.byAreaX, Para.byPutCount, Para.wTimePutInterval,
				Para.byMoveProbabilityX, Para.bySearchCx, Para.byDropProbability, (int)Para.dwStTmplateID);
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
			printf("charinfo3 %s ret=%d area=%u put=%u interval=%u move=%u search=%u drop=%u\n",
				a ? "admin" : "user", bRet, Para.byAreaX, Para.byPutCount, Para.wTimePutInterval,
				Para.byMoveProbabilityX, Para.bySearchCx, Para.byDropProbability);
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
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	const char *pszData = (argc >= 2) ? argv[1] : NULL;
	if (pszData && chdir(pszData) != 0) {
		fprintf(stderr, "chdir failed\n");
		return 1;
	}
	TestSend();
	TestMyArray();
	TestSugorokuViewer();
	TestWorld();
	printf("== done ==\n");
	fflush(stdout);
	_exit(0);
}
