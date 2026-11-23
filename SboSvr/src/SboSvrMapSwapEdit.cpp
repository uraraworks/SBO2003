/* ========================================================================= */
/* ファイル名：	SboSvrMapSwapEdit.cpp										 */
/* 内容：		マップの差し替え表の書き込みの決まり（SboSvrMapSwapEdit.h）	 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "SboSvrMapSwapEdit.h"
#include <string>
#include <vector>
#include <string.h>

/* ========================================================================= */
/* 番号（全マップ通し）														 */
/* ========================================================================= */

BOOL ChestNoUsedElsewhere(int nSkipMap, int nChestNo)
{
	for (int i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		if (i == nSkipMap) continue;
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_CHEST && pMap->aSwap[j].wChestNo == nChestNo) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

int ChestHighAll(void)
{
	int nHigh = 0;
	for (int i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		nHigh = max (nHigh, (int)pMap->wChestHigh);
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_CHEST && pMap->aSwap[j].wChestNo < MAX_CHESTNO) {
				nHigh = max (nHigh, (int)pMap->aSwap[j].wChestNo + 1);
			}
		}
	}
	return nHigh;
}

BOOL RecycleBoxNoUsedElsewhere(int nSkipMap, int nBoxNo)
{
	for (int i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		if (i == nSkipMap) continue;
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_RECYCLE && pMap->aSwap[j].wBoxNo == nBoxNo) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

int RecycleBoxHighAll(void)
{
	int nHigh = 0;
	for (int i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		nHigh = max (nHigh, (int)pMap->wBoxHigh);
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_RECYCLE && pMap->aSwap[j].wBoxNo <= MAX_RECYCLEBOX) {
				nHigh = max (nHigh, (int)pMap->aSwap[j].wBoxNo);
			}
		}
	}
	return nHigh;
}

/* ========================================================================= */
/* 確かめ用の小さな関数														 */
/* ========================================================================= */

static std::string Num(long long n)
{
	char szTmp[32];
	snprintf (szTmp, sizeof (szTmp), "%lld", n);
	return szTmp;
}

/* 範囲 */
static BOOL CheckRange(long long n, long long nMin, long long nMax, const std::string &strName, std::string &strErr)
{
	if (n < nMin || n > nMax) {
		strErr = strName + " は " + Num (nMin) + "〜" + Num (nMax) + " の範囲で指定してください";
		return FALSE;
	}
	return TRUE;
}

/* 文字列（長さ・制御文字。改行も使えない） */
static BOOL CheckText(const std::string &s, size_t nBufSize, const std::string &strName, std::string &strErr)
{
	if (s.size () + 1 > nBufSize) {
		strErr = strName + " が長すぎます（UTF-8 で " + Num ((long long)nBufSize - 1) + " バイトまで。今は " + Num ((long long)s.size ()) + " バイト）";
		return FALSE;
	}
	for (size_t i = 0; i < s.size (); i ++) {
		unsigned char c = (unsigned char)s[i];
		if (c < 0x20 || c == 0x7F) {
			strErr = strName + " に使えない文字（改行・制御文字）が入っています";
			return FALSE;
		}
	}
	return TRUE;
}

/* ========================================================================= */
/* 関数名：MapSwapBuild														 */
/* 内容：表を確かめて作る（まず番号を指定された行を確定してから、省かれた分に自動で振る）*/
/* ========================================================================= */
BOOL MapSwapBuild(int nMapNo, MAPINFO &w, const std::vector<MAPSWAPEDITROW> &aRow, LPCSTR pszPrefix, std::string &strErr)
{
	if (aRow.size () > MAX_MAPSWAP) {
		strErr = std::string (pszPrefix) + " は " + Num (MAX_MAPSWAP) + " 個までです";
		return FALSE;
	}
	ZeroMemory (w.aSwap, sizeof (w.aSwap));
	w.bySwapCount = (BYTE)aRow.size ();
	int nNext = ChestHighAll ();					/* 自動で振る宝箱の番号（消した番号は飛ばす） */
	int nHighNew = (int)w.wChestHigh;
	std::vector<char> abyUsed (MAX_CHESTNO, 0);
	int nNextBox = RecycleBoxHighAll () + 1;		/* リサイクル箱の自動で振る番号（1 から。消した番号は飛ばす） */
	int nBoxHighNew = (int)w.wBoxHigh;
	std::vector<char> abyBoxUsed (MAX_RECYCLEBOX + 1, 0);

	for (int pass = 0; pass < 2; pass ++) {
		for (size_t i = 0; i < aRow.size (); i ++) {
			const MAPSWAPEDITROW &e = aRow[i];
			std::string strP = std::string (pszPrefix) + "[" + Num ((long long)i) + "]";
			PMAPSWAP p = &w.aSwap[i];
			const DWORD dw = e.dwHas;

			if (pass == 0) {
				if (!CheckRange (e.nKind, 0, 2, strP + ".kind", strErr)) return FALSE;
				p->byKind = (BYTE)e.nKind;
				if (!CheckRange (e.nX, 0, w.nCX - 1, strP + ".x", strErr)) return FALSE;
				p->wX = (WORD)e.nX;
				if (!CheckRange (e.nY, 0, w.nCY - 1, strP + ".y", strErr)) return FALSE;
				p->wY = (WORD)e.nY;
				if (!CheckRange (e.nParts, 0, (long long)g_wCountMapParts - 1, strP + ".wParts", strErr)) return FALSE;
				p->wParts = (WORD)e.nParts;
				p->wFlag = FLAG_NOBIT;
				p->wChestNo = 0xFFFF;
				p->wNeedFlag = FLAG_NOBIT;		/* 宝箱の開ける条件: 無し */
				if (p->byKind == MAPSWAPKIND_RECYCLE) {
					p->wBoxNo = 0;		/* 0=まだ振っていない（番号は 1 から） */
				}
				if (p->byKind == MAPSWAPKIND_FLAG) {
					if (dw & (MAPSWAPEDIT_CHESTNO | MAPSWAPEDIT_BOXNO | MAPSWAPEDIT_GIVEITEM | MAPSWAPEDIT_MSG | MAPSWAPEDIT_MEMO | MAPSWAPEDIT_NEEDFLAG | MAPSWAPEDIT_LOCKMSG)) {
						strErr = strP + ": 印で変わる行（kind 0）に wChestNo・wBoxNo・wGiveItem・szMsg・szMemo・wNeedFlag・szLockMsg は使えません（宝箱は kind 1・リサイクル箱は kind 2）";
						return FALSE;
					}
					if (!(dw & MAPSWAPEDIT_FLAG)) {
						strErr = strP + " には wFlag（印の番号）が要ります";
						return FALSE;
					}
					if (!CheckRange (e.nFlag, 0, MAX_FLAGBIT - 1, strP + ".wFlag", strErr)) return FALSE;
					p->wFlag = (WORD)e.nFlag;
					if (dw & MAPSWAPEDIT_CMP) {
						if (!CheckRange (e.nCmp, 0, 1, strP + ".byCmp", strErr)) return FALSE;
						p->byCmp = (BYTE)e.nCmp;
					}
				} else if (p->byKind == MAPSWAPKIND_RECYCLE) {
					if (dw & (MAPSWAPEDIT_FLAG | MAPSWAPEDIT_CMP | MAPSWAPEDIT_CHESTNO | MAPSWAPEDIT_GIVEITEM | MAPSWAPEDIT_MSG | MAPSWAPEDIT_NEEDFLAG | MAPSWAPEDIT_LOCKMSG)) {
						strErr = strP + ": リサイクル箱の行（kind 2）に使えるのは x・y・wParts（開いた絵）・szMemo・wBoxNo だけです";
						return FALSE;
					}
					if (dw & MAPSWAPEDIT_MEMO) {
						if (!CheckText (e.strMemo, MAX_MAPSWAPMEMO, strP + ".szMemo", strErr)) return FALSE;
						CopyMemory (p->szMemo, e.strMemo.c_str (), e.strMemo.size () + 1);
					}
					if (dw & MAPSWAPEDIT_BOXNO) {
						long long n = e.nBoxNo;
						if (!CheckRange (n, 1, MAX_RECYCLEBOX, strP + ".wBoxNo", strErr)) return FALSE;
						if (abyBoxUsed[(size_t)n]) {
							strErr = strP + ".wBoxNo " + Num (n) + " は、この表の中で重なっています";
							return FALSE;
						}
						if (RecycleBoxNoUsedElsewhere (nMapNo, (int)n)) {
							strErr = strP + ".wBoxNo " + Num (n) + " は、ほかのマップのリサイクル箱がもう使っています（箱の番号は全マップ通しで重ならない）";
							return FALSE;
						}
						abyBoxUsed[(size_t)n] = 1;
						p->wBoxNo = (WORD)n;
						nBoxHighNew = max (nBoxHighNew, (int)n);
					}
				} else {
					if (dw & (MAPSWAPEDIT_FLAG | MAPSWAPEDIT_CMP | MAPSWAPEDIT_BOXNO)) {
						strErr = strP + ": 宝箱の行（kind 1）に wFlag・byCmp・wBoxNo は使えません（宝箱は印ではなく開けた記録を使います）";
						return FALSE;
					}
					if (dw & MAPSWAPEDIT_GIVEITEM) {
						long long n = e.nGiveItem;
						if (!CheckRange (n, 0, 65535, strP + ".wGiveItem", strErr)) return FALSE;
						if (n != 0 && n >= g_aItemInfo.GetSize ()) {
							strErr = strP + ".wGiveItem のアイテム " + Num (n) + " がありません（item は " + Num (g_aItemInfo.GetSize ()) + " 個）";
							return FALSE;
						}
						p->wGiveItem = (WORD)n;
					}
					if (dw & MAPSWAPEDIT_MSG) {
						if (!CheckText (e.strMsg, MAX_MAPSWAPMSG, strP + ".szMsg", strErr)) return FALSE;
						CopyMemory (p->szMsg, e.strMsg.c_str (), e.strMsg.size () + 1);
					}
					if (dw & MAPSWAPEDIT_MEMO) {
						if (!CheckText (e.strMemo, MAX_MAPSWAPMEMO, strP + ".szMemo", strErr)) return FALSE;
						CopyMemory (p->szMemo, e.strMemo.c_str (), e.strMemo.size () + 1);
					}
					if (dw & MAPSWAPEDIT_NEEDFLAG) {
						/* 印の番号 0〜1023。FLAG_NOBIT（65535）は「条件なし」（読み出した値をそのまま書き戻せるように受ける） */
						if (e.nNeedFlag != FLAG_NOBIT) {
							if (!CheckRange (e.nNeedFlag, 0, MAX_FLAGBIT - 1, strP + ".wNeedFlag", strErr)) return FALSE;
						}
						p->wNeedFlag = (WORD)e.nNeedFlag;
					}
					if (dw & MAPSWAPEDIT_LOCKMSG) {
						if (!CheckText (e.strLockMsg, MAX_MAPSWAPMSG, strP + ".szLockMsg", strErr)) return FALSE;
						CopyMemory (p->szLockMsg, e.strLockMsg.c_str (), e.strLockMsg.size () + 1);
					}
					if (dw & MAPSWAPEDIT_CHESTNO) {
						long long n = e.nChestNo;
						if (!CheckRange (n, 0, MAX_CHESTNO - 1, strP + ".wChestNo", strErr)) return FALSE;
						if (abyUsed[(size_t)n]) {
							strErr = strP + ".wChestNo " + Num (n) + " は、この表の中で重なっています";
							return FALSE;
						}
						if (ChestNoUsedElsewhere (nMapNo, (int)n)) {
							strErr = strP + ".wChestNo " + Num (n) + " は、ほかのマップの宝箱がもう使っています（宝箱の番号は全マップ通しで重ならない）";
							return FALSE;
						}
						abyUsed[(size_t)n] = 1;
						p->wChestNo = (WORD)n;
						nHighNew = max (nHighNew, (int)n + 1);
					}
				}
			} else if (p->byKind == MAPSWAPKIND_RECYCLE && p->wBoxNo == 0) {
				while (nNextBox <= MAX_RECYCLEBOX && (abyBoxUsed[nNextBox] || RecycleBoxNoUsedElsewhere (nMapNo, nNextBox))) {
					nNextBox ++;
				}
				if (nNextBox > MAX_RECYCLEBOX) {
					strErr = "リサイクル箱の番号を使い切りました（1〜" + Num (MAX_RECYCLEBOX) + "）。消した箱の番号は使い回しません";
					return FALSE;
				}
				abyBoxUsed[nNextBox] = 1;
				p->wBoxNo = (WORD)nNextBox;
				nBoxHighNew = max (nBoxHighNew, nNextBox);
				nNextBox ++;
			} else if (p->byKind == MAPSWAPKIND_CHEST && p->wChestNo == 0xFFFF) {
				while (nNext < MAX_CHESTNO && (abyUsed[nNext] || ChestNoUsedElsewhere (nMapNo, nNext))) {
					nNext ++;
				}
				if (nNext >= MAX_CHESTNO) {
					strErr = "宝箱の番号を使い切りました（0〜" + Num (MAX_CHESTNO - 1) + "）。消した宝箱の番号は使い回しません";
					return FALSE;
				}
				abyUsed[nNext] = 1;
				p->wChestNo = (WORD)nNext;
				nHighNew = max (nHighNew, nNext + 1);
				nNext ++;
			}
		}
	}
	w.wChestHigh = (WORD)nHighNew;
	w.wBoxHigh = (WORD)nBoxHighNew;
	return TRUE;
}

/* ========================================================================= */
/* 行の変換																	 */
/* ========================================================================= */

void MapSwapToEdit(const MAPSWAP &Sw, MAPSWAPEDITROW &Row)
{
	Row = MAPSWAPEDITROW ();
	Row.nKind	= Sw.byKind;
	Row.nX		= Sw.wX;
	Row.nY		= Sw.wY;
	Row.nParts	= Sw.wParts;
	if (Sw.byKind == MAPSWAPKIND_FLAG) {
		Row.dwHas	= MAPSWAPEDIT_FLAG | MAPSWAPEDIT_CMP;
		Row.nFlag	= Sw.wFlag;
		Row.nCmp	= Sw.byCmp;
	} else if (Sw.byKind == MAPSWAPKIND_RECYCLE) {
		Row.dwHas	= MAPSWAPEDIT_BOXNO | MAPSWAPEDIT_MEMO;
		Row.nBoxNo	= Sw.wBoxNo;
		Row.strMemo	= Sw.szMemo;
	} else {
		Row.dwHas		= MAPSWAPEDIT_CHESTNO | MAPSWAPEDIT_GIVEITEM | MAPSWAPEDIT_MSG | MAPSWAPEDIT_MEMO;
		Row.nChestNo	= Sw.wChestNo;
		Row.nGiveItem	= Sw.wGiveItem;
		Row.strMsg		= Sw.szMsg;
		Row.strMemo		= Sw.szMemo;
		Row.dwHas		|= MAPSWAPEDIT_NEEDFLAG | MAPSWAPEDIT_LOCKMSG;
		Row.nNeedFlag	= Sw.wNeedFlag;
		Row.strLockMsg	= Sw.szLockMsg;
	}
}

/* 文字列の欄は、終端が無くても読み過ぎない */
static std::string WireText(const char *psz, size_t nSize)
{
	size_t n = 0;
	while (n < nSize && psz[n]) n ++;
	return std::string (psz, n);
}

void MapSwapEditFromWire(const ADMINSWAPROW &Wire, MAPSWAPEDITROW &Row)
{
	const BYTE by = Wire.byHas;

	Row = MAPSWAPEDITROW ();
	Row.nKind	= Wire.byKind;
	Row.nX		= Wire.wX;
	Row.nY		= Wire.wY;
	Row.nParts	= Wire.wParts;
	if (by & ADMINSWAPHAS_FLAG) {
		Row.dwHas |= MAPSWAPEDIT_FLAG;
		Row.nFlag = Wire.wFlag;
	}
	if (by & ADMINSWAPHAS_CMP) {
		Row.dwHas |= MAPSWAPEDIT_CMP;
		Row.nCmp = Wire.byCmp;
	}
	if (by & ADMINSWAPHAS_NO) {
		/* 番号は種類で宝箱かリサイクル箱かが決まる（印の行に付けたら宝箱の番号として断られる） */
		if (Wire.byKind == MAPSWAPKIND_RECYCLE) {
			Row.dwHas |= MAPSWAPEDIT_BOXNO;
			Row.nBoxNo = Wire.wNo;
		} else {
			Row.dwHas |= MAPSWAPEDIT_CHESTNO;
			Row.nChestNo = Wire.wNo;
		}
	}
	if (by & ADMINSWAPHAS_GIVEITEM) {
		Row.dwHas |= MAPSWAPEDIT_GIVEITEM;
		Row.nGiveItem = Wire.wGiveItem;
	}
	if (by & ADMINSWAPHAS_MSG) {
		Row.dwHas |= MAPSWAPEDIT_MSG;
		Row.strMsg = WireText (Wire.szMsg, sizeof (Wire.szMsg));
	}
	if (by & ADMINSWAPHAS_MEMO) {
		Row.dwHas |= MAPSWAPEDIT_MEMO;
		Row.strMemo = WireText (Wire.szMemo, sizeof (Wire.szMemo));
	}
	if (by & ADMINSWAPHAS_NEED) {
		Row.dwHas |= MAPSWAPEDIT_NEEDFLAG;
		Row.nNeedFlag = Wire.wNeedFlag;
	}
	if (by & ADMINSWAPHAS_LOCKMSG) {
		Row.dwHas |= MAPSWAPEDIT_LOCKMSG;
		Row.strLockMsg = WireText (Wire.szLockMsg, sizeof (Wire.szLockMsg));
	}
}

void MapSwapWireFromSwap(const MAPSWAP &Sw, ADMINSWAPROW &Wire)
{
	ZeroMemory (&Wire, sizeof (Wire));
	Wire.wNeedFlag	= FLAG_NOBIT;
	Wire.byKind	= Sw.byKind;
	Wire.wX		= Sw.wX;
	Wire.wY		= Sw.wY;
	Wire.wParts	= Sw.wParts;
	if (Sw.byKind == MAPSWAPKIND_FLAG) {
		Wire.byHas	= ADMINSWAPHAS_FLAG | ADMINSWAPHAS_CMP;
		Wire.wFlag	= Sw.wFlag;
		Wire.byCmp	= Sw.byCmp;
	} else if (Sw.byKind == MAPSWAPKIND_RECYCLE) {
		Wire.byHas	= ADMINSWAPHAS_NO | ADMINSWAPHAS_MEMO;
		Wire.wNo	= Sw.wBoxNo;
		CopyMemory (Wire.szMemo, Sw.szMemo, sizeof (Wire.szMemo));
	} else {
		Wire.byHas		= ADMINSWAPHAS_NO | ADMINSWAPHAS_GIVEITEM | ADMINSWAPHAS_MSG | ADMINSWAPHAS_MEMO;
		Wire.wNo		= Sw.wChestNo;
		Wire.wGiveItem	= Sw.wGiveItem;
		CopyMemory (Wire.szMsg, Sw.szMsg, sizeof (Wire.szMsg));
		CopyMemory (Wire.szMemo, Sw.szMemo, sizeof (Wire.szMemo));
		Wire.byHas		|= ADMINSWAPHAS_NEED | ADMINSWAPHAS_LOCKMSG;
		Wire.wNeedFlag	= Sw.wNeedFlag;
		CopyMemory (Wire.szLockMsg, Sw.szLockMsg, sizeof (Wire.szLockMsg));
	}
	Wire.szLockMsg[sizeof (Wire.szLockMsg) - 1] = 0;
	Wire.szMsg[sizeof (Wire.szMsg) - 1] = 0;
	Wire.szMemo[sizeof (Wire.szMemo) - 1] = 0;
}

/* ========================================================================= */
/* 反映																		 */
/* ========================================================================= */

BOOL MapSwapChanged(const MAPINFO &Cur, const MAPINFO &w)
{
	return (Cur.bySwapCount != w.bySwapCount) || (Cur.wChestHigh != w.wChestHigh) || (Cur.wBoxHigh != w.wBoxHigh) ||
		(memcmp (Cur.aSwap, w.aSwap, sizeof (Cur.aSwap)) != 0);
}

void MapSwapSet(PMAPINFO pMap, const MAPINFO &w)
{
	pMap->bySwapCount	= w.bySwapCount;
	CopyMemory (pMap->aSwap, w.aSwap, sizeof (pMap->aSwap));
	pMap->wChestHigh	= w.wChestHigh;
	pMap->wBoxHigh		= w.wBoxHigh;
}
