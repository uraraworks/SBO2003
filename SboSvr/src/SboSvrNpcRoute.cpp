/* ========================================================================= */
/* ファイル名：	SboSvrNpcRoute.cpp											 */
/* 内容：		NPCの巡回ルートの検査と反映									 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "CharProc/IncludeCharProc.h"
#include "SboSvrNpcRoute.h"

extern BOOL SaveCharInfo(void);												/* キャラ情報保存 */

/* ========================================================================= */
/* 関数名：NpcRouteIsTarget													 */
/* 内容：巡回の設定ができる NPC か											 */
/* ========================================================================= */
BOOL NpcRouteIsTarget(PCHARINFO pChar)
{
	if (pChar == NULL) {
		return FALSE;
	}
	return (pChar->byNpcType == NPCTYPE_NOMOVE) || (pChar->byNpcType == NPCTYPE_MOVERAND);
}

/* ========================================================================= */
/* 関数名：NpcRouteCheck													 */
/* 内容：ルートを検査する（問題があればエラー文、問題なければ NULL）		 */
/*       nWait  止まる秒数(0〜60)											 */
/*       nCount 道しるべの数（0 は巡回をやめる。1 は不可）					 */
/* ========================================================================= */
LPCSTR NpcRouteCheck(PCHARINFO pChar, int nMode, int nWait, int nCount, const int *pnX, const int *pnY)
{
	int i;
	PMAPINFO pMap;

	if (!NpcRouteIsTarget (pChar)) {
		return "巡回できるのは「移動しない」「ランダム移動」のNPCだけです";
	}
	if ((nMode < 0) || (nMode >= NPCROUTEMODE_MAX)) {
		return "回り方の値が不正です";
	}
	if ((nWait < 0) || (nWait > 60)) {
		return "止まる秒数は0〜60です";
	}
	if (pChar->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) {
		return "NPCのマップが見つかりません";
	}
	pMap = g_aMapInfo[pChar->dwMapID];

	if (nCount < 0) {
		return "道しるべの書き方が不正です";
	}
	/* 範囲を調べるのは配列の中身を読める数まで */
	for (i = 0; i < nCount && i < MAX_NPCROUTE; i ++) {
		if ((pnX[i] < 0) || (pnX[i] >= pMap->nCX) || (pnY[i] < 0) || (pnY[i] >= pMap->nCY) ||
				(pnX[i] > 255) || (pnY[i] > 255)) {
			return "道しるべがマップの外です";
		}
	}
	if (nCount == 1) {
		return "道しるべは2つ以上置いてください（巡回をやめる時は0個）";
	}
	if (nCount > MAX_NPCROUTE) {
		return "道しるべは16個までです";
	}
	return NULL;
}

/* ========================================================================= */
/* 関数名：NpcRouteApply													 */
/* 内容：ルートを反映する（動き方を作り直してキャラ情報を保存する）		 */
/* ========================================================================= */
void NpcRouteApply(PCHARINFO pChar, BYTE byMode, BYTE byWait, int nCount, const BYTE *pbyX, const BYTE *pbyY)
{
	int i;
	CCharProcBase CharProcBase;

	if (nCount > MAX_NPCROUTE) {
		nCount = MAX_NPCROUTE;
	}
	pChar->byRouteCount	= (BYTE)nCount;
	pChar->byRouteMode	= byMode;
	pChar->byRouteWait	= byWait;
	ZeroMemory (pChar->abyRouteX, sizeof (pChar->abyRouteX));
	ZeroMemory (pChar->abyRouteY, sizeof (pChar->abyRouteY));
	for (i = 0; i < nCount; i ++) {
		pChar->abyRouteX[i] = pbyX[i];
		pChar->abyRouteY[i] = pbyY[i];
	}
	/* 動き方を作り直して、すぐ反映する */
	CharProcBase.SetProc (pChar);
	SaveCharInfo ();
}

/* ========================================================================= */
/* 関数名：NpcRouteMakePara													 */
/* 内容：キャラの今のルートをコマンドにする									 */
/* ========================================================================= */
void NpcRouteMakePara(CMDPARA_NPCROUTE &Para, BYTE byCmdSub, PCHARINFO pChar)
{
	ZeroMemory (&Para, sizeof (Para));
	Para.byCmd		= CMD_NPCROUTE;
	Para.byCmdSub	= byCmdSub;
	if (pChar == NULL) {
		return;
	}
	Para.dwMapID		= pChar->dwMapID;
	Para.dwTblID		= pChar->dwTblID;
	Para.byRouteCount	= pChar->byRouteCount;
	Para.byRouteMode	= pChar->byRouteMode;
	Para.byRouteWait	= pChar->byRouteWait;
	CopyMemory (Para.abyRouteX, pChar->abyRouteX, sizeof (Para.abyRouteX));
	CopyMemory (Para.abyRouteY, pChar->abyRouteY, sizeof (Para.abyRouteY));
}
