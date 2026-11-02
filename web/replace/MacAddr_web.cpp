/* ========================================================================= */
/* MacAddr_web.cpp															 */
/* MAC アドレス取得クラス（Web版）											 */
/* ========================================================================= */
/*
 * ブラウザからは MAC アドレスを取れないので、最初に乱数で作った 6 バイトを
 * 保存データ（/sbo/macaddr.bin）に残し、以降はそれを返す。
 * 先頭バイトは「ローカル管理」のアドレスであることを示す値にしておく。
 */
#include "StdAfx.h"
#include "Lib/MacAddr.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MACADDR_FILE	"/sbo/macaddr.bin"

static BOOL LoadOrMake(BYTE *pbyAddr)
{
	FILE *fp = fopen(MACADDR_FILE, "rb");
	if (fp) {
		size_t n = fread(pbyAddr, 1, 6, fp);
		fclose(fp);
		if (n == 6) {
			return TRUE;
		}
	}
	srand((unsigned)time(NULL) ^ (unsigned)GetTickCount());
	for (int i = 0; i < 6; i ++) {
		pbyAddr[i] = (BYTE)(rand() & 0xFF);
	}
	pbyAddr[0] = (BYTE)((pbyAddr[0] & 0xFC) | 0x02);
	fp = fopen(MACADDR_FILE, "wb");
	if (fp) {
		fwrite(pbyAddr, 1, 6, fp);
		fclose(fp);
		W32SyncFiles();
	}
	return TRUE;
}

CMacAddr::CMacAddr()
{
	m_nCount = 1;
}

CMacAddr::~CMacAddr()
{
}

BOOL CMacAddr::Get(PBYTE pDst, int nNo)
{
	if (pDst == NULL || nNo != 0) {
		return FALSE;
	}
	return LoadOrMake(pDst);
}

BOOL CMacAddr::GetStr(LPSTR pszDst, int nNo)
{
	BYTE abyAddr[6];
	if (pszDst == NULL || !Get(abyAddr, nNo)) {
		return FALSE;
	}
	sprintf(pszDst, "%02X-%02X-%02X-%02X-%02X-%02X",
			abyAddr[0], abyAddr[1], abyAddr[2], abyAddr[3], abyAddr[4], abyAddr[5]);
	return TRUE;
}

int CMacAddr::GetCount(void)
{
	return 1;
}

BOOL CMacAddr::GetDeviceName(LPSTR pszDst, int nNo)
{
	if (pszDst == NULL || nNo != 0) {
		return FALSE;
	}
	strcpy(pszDst, "Browser");
	return TRUE;
}

int CMacAddr::GetNICCount(void)
{
	return 1;
}
