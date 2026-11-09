/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrSubGameProc.cpp										 */
/* 概要：		サブゲーム処理												 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/08/09													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSubGameProcSugoroku.h"
#include <vector>
#include <algorithm>

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void SubGameProc(DWORD dwID);											/* サブゲーム処理 */
static void SubGameProc_BOMB(PSUBGAMEINFO pInfo);						/* サブゲーム処理(爆弾ゲーム) */
static void SubGameProc_MAZE(PSUBGAMEINFO pInfo);						/* サブゲーム処理(迷路) */
static void SubGameProc_DUNGEON(PSUBGAMEINFO pInfo);					/* サブゲーム処理(ダンジョン) */
static void SubGameProc_WAR(PSUBGAMEINFO pInfo);						/* サブゲーム処理(戦場ゲーム) */
static void SubGameProc_SUGOROKU(PSUBGAMEINFO pInfo);					/* サブゲーム処理(すごろく) */

void SubGameProcBombQuit(BYTE);											/* 爆弾ゲーム試合終了処理 */
void SubGameProcBombQuitMsg(BYTE, LPCSTR);								/* 爆弾ゲーム試合終了処理(終わった理由を表示) */
void SubGameProcBombMakeBlock(BYTE);									/* 爆弾ゲーム壁作成処理 */
void SubGameProcBombDeleteBlock(BYTE);									/* 爆弾ゲーム壁消去処理 */

void MakeMazeProc(BYTE);												/* 迷路作成処理 */
static int GetMazePathWidth(PSUBGAME_MAZEINFO);							/* 迷路の通路の太さ */
static WORD GetMazeTileParts(int, BOOL, WORD, WORD, BYTE, WORD, WORD, BYTE, WORD);	/* 迷路のマスのパーツ */
static void MakeMazeWeave(PBYTE, int, int, PSUBGAME_MAZEINFO);			/* 立体交差のある迷路の設計図 */
void MakeDungeonProc(BYTE);												/* ダンジョン作成処理 */
static BOOL MakeDungeonProcTmp(PBYTE, int, int, int, int, int);			/* ダンジョン作成処理(メイン) */
static BOOL CheckMakeDungeon(PBYTE, int, int, int, int, int, int);		/* 進行方向のチェック */

extern void sgenrand(unsigned long);									/* 乱数初期化 */
extern long genrand();													/* 乱数取得 */

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef void (*SUBGAMEPROC)(PSUBGAMEINFO);

/* 関数ジャンプテーブル */
static const SUBGAMEPROC SubGameProcTbl[] = {
	NULL,
	SubGameProc_BOMB,				/* 爆弾ゲーム */
	SubGameProc_MAZE,				/* 迷路 */
	SubGameProc_DUNGEON,			/* ダンジョン */
	SubGameProc_WAR,				/* 戦場ゲーム */
	SubGameProc_SUGOROKU			/* すごろく */
};

/* ========================================================================= */
/* 関数名：EventProc														 */
/* 内容：サブゲーム処理														 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
void SubGameProc(DWORD dwID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[dwID];

	/* 種類別に処理 */
	if (SubGameProcTbl[pInfo->byType]) {
		SubGameProcTbl[pInfo->byType] (pInfo);
	}
}

/* ========================================================================= */
/* 関数名：SubGameProc_BOMB													 */
/* 内容：サブゲーム処理(爆弾ゲーム)											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void SubGameProc_BOMB(PSUBGAMEINFO pInfo)
{
	if (g_aCharInfo.GetSize () == 0) {
		return;
	}
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;

	/* 試合中？ */
	if (pBomb->byFlgGameNow) {
		/* 制限時間付き？ */
		if (pBomb->byGameTime) {
			if (g_dwTime > pBomb->dwLastTime + 1000) {
				pBomb->dwLastTime = g_dwTime;
				if (pBomb->byGameTimeNow > 0) {
					pBomb->byGameTimeNow --;
					SendCmdSubBombTime ((BYTE)g_nProcSubGame, pBomb);
				}
				/* タイムオーバー？ */
				if (pBomb->byGameTimeNow == 0) {
					/* 試合終了処理 */
					SubGameProcBombQuitMsg ((BYTE)g_nProcSubGame, "時間切れ！爆弾ゲーム試合終了");
				}
			}
		}
	}

	/* 参加者以外のキャラがエリア内にいるかチェック */
	PCHARINFO pChar = g_aCharInfo[g_nProcChar];

	/* ログインしてない？ */
	if (!((pChar->byGameState != GAMESTATE_NPC) &&
			(pChar->byGameState >= GAMESTATE_MOVE))) {
		return;
	}
	if (pBomb->dwMapID != pChar->dwMapID) {
		return;
	}

	/* エリア内にいる？ */
	if ((pChar->dwX >= pBomb->byX1) && (pChar->dwX <= pBomb->byX2) &&
		(pChar->dwY >= pBomb->byY1) && (pChar->dwY <= pBomb->byY2)) {
		/* 参加していない？ */
		if (pChar->abySubStatus[0] != SUBGAMEID_BOMB) {
			/* 管理者以外？ */
			if (pChar->byAdminLV == 0) {
				/* 追い出す */
				SendCmdFADEINOUT (pChar->pID, 0);
				pChar->dwX = pBomb->dwX;
				pChar->dwY = pBomb->dwY;
				CharTargetCancelPC (pChar);
				SendCmdCHARINFO (pChar->pID, -1);
				SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
				SendCmdFADEINOUT (pChar->pID, 1);
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：SubGameProc_MAZE													 */
/* 内容：サブゲーム処理(迷路)												 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void SubGameProc_MAZE(PSUBGAMEINFO pInfo)
{
	PSUBGAME_MAZEINFO pMaze = (PSUBGAME_MAZEINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pMaze->dwMapID];

	/* マップ作成中？ */
	if (pMaze->pMapTmp) {
		/* 作り始めた時の大きさで描く（途中で設定が変わってもはみ出さない） */
		int cx, cy, nWidth;
		nWidth = (int)pMaze->byWorkWidth;
		cx = (int)pMaze->wWorkCX;
		cy = (int)pMaze->wWorkCY;

		/* 0=道 1=壁 → 1=道 2=壁 にそろえる(2=縦に渡る橋 3=横に渡る橋はそのパーツ) */
		BYTE byTmpCell = pMaze->pMapTmp[cx * pMaze->byTmpY + pMaze->byTmpX];
		int nCell = (byTmpCell == 0) ? 1 : 2;
		/* 下のマスが道(橋)なら、壁の下の段は手前の面にする */
		BOOL bBelowRoad = FALSE;
		if (pMaze->byTmpY + 1 < cy) {
			bBelowRoad = (pMaze->pMapTmp[cx * (pMaze->byTmpY + 1) + pMaze->byTmpX] != 1);
		}
		int x, y, xx, yy;
		x = (int)(pMaze->byTmpX * nWidth) + (int)pMaze->byX1;
		y = (int)(pMaze->byTmpY * nWidth) + (int)pMaze->byY1;

		for (yy = 0; yy < nWidth; yy ++) {
			for (xx = 0; xx < nWidth; xx ++) {
				if ((x + xx >= pMap->nCX) || (y + yy >= pMap->nCY)) {
					continue;
				}
				WORD wParts;
				if (byTmpCell == 2) {
					wParts = pMaze->wChipIDBridgeV;
				} else if (byTmpCell == 3) {
					wParts = pMaze->wChipIDBridgeH;
				} else {
					wParts = GetMazeTileParts (nCell, (bBelowRoad && (yy == nWidth - 1)),
						pMaze->wChipIDRoad, pMaze->wChipIDRoad2, pMaze->byRoad2Rate,
						pMaze->wChipIDBlock, pMaze->wChipIDBlock2, pMaze->byBlock2Rate, pMaze->wChipIDBlockFace);
				}
				pMap->pwMap[pMap->nCX * (y + yy) + (x + xx)] = wParts;
			}
		}
		pMaze->byTmpX ++;
		if (pMaze->byTmpX >= cx) {
			pMaze->byTmpX = 0;
			pMaze->byTmpY ++;
			/* 完了？ */
			if (pMaze->byTmpY >= cy) {
				/* スタートとゴールを設定 */
				int nStartX, nStartY, nGoalX, nGoalY;
				nStartX = nStartY = nGoalX = nGoalY = -1;

				while (1) {
					if (nStartX < 0) {
						nStartX = (int)((unsigned long)genrand () % (unsigned long)cx);
						nStartY = (int)((unsigned long)genrand () % (unsigned long)cy);
						if (pMaze->pMapTmp[nStartY * cx + nStartX] != 0) {
							nStartX = nStartY = -1;

						} else {
							nStartX *= nWidth;
							nStartY *= nWidth;
							nStartX += (int)pMaze->byX1;
							nStartY += (int)pMaze->byY1;
						}
					}
					if (nGoalX < 0) {
						nGoalX = (int)((unsigned long)genrand () % (unsigned long)cx);
						nGoalY = (int)((unsigned long)genrand () % (unsigned long)cy);
						if (pMaze->pMapTmp[nGoalY * cx + nGoalX] != 0) {
							nGoalX = nGoalY = -1;

						} else {
							nGoalX *= nWidth;
							nGoalY *= nWidth;
							nGoalX += (int)pMaze->byX1;
							nGoalY += (int)pMaze->byY1;
						}
					}
					if (nStartX >= 0 && nStartY >= 0 && nGoalX >= 0 && nGoalY >= 0) {
						break;
					}
				}

				SAFE_DELETE_ARRAY (pMaze->pMapTmp);
				pMaze->byTmpX = pMaze->byTmpY = 0;

				/* スタート地点とゴール地点のイベント・マップを設置 */
				pMap->pwMap[pMaze->byStartX + pMaze->byStartY * pMap->nCX] = pMaze->wChipIDStart;
				SendCmdCHGMAP (pMap->dwID, pMaze->wChipIDStart, pMaze->byStartX, pMaze->byStartY);
				pMap->pwMap[nGoalX + nGoalY * pMap->nCX] = pMaze->wChipIDGoal;

				/* スタート地点へ */
				int nNo = AddEvent (pMap->dwID);
				PEVENTINFO pEvent = &pMap->pEvent[nNo];
				pEvent->dwX = (DWORD)pMaze->byStartX;
				pEvent->dwY = (DWORD)pMaze->byStartY;
				pEvent->byType = EVENTTYPE_MAPCHG;
				pEvent->pData = NULL;

				int nSize = GetEventDataSize (EVENTTYPE_MAPCHG);
				pEvent->pData = new BYTE[nSize];
				ZeroMemory (pEvent->pData, nSize);
				PEVENTINFO_MAPCHG pMapChg = (PEVENTINFO_MAPCHG)pEvent->pData;
				pMapChg->dwMapID = pMap->dwID;
				pMapChg->dwX = (DWORD)nStartX;
				pMapChg->dwY = (DWORD)nStartY;
				pMapChg->byEfect = EVENTMOVEEFECT_SILENT;


				/* ゴールから脱出 */
				nNo = AddEvent (pMap->dwID);
				pEvent = &pMap->pEvent[nNo];
				pEvent->dwX = (DWORD)nGoalX;
				pEvent->dwY = (DWORD)nGoalY;
				pEvent->byType = EVENTTYPE_MOVE;
				pEvent->pData = NULL;

				nSize = GetEventDataSize (EVENTTYPE_MOVE);
				pEvent->pData = new BYTE[nSize];
				ZeroMemory (pEvent->pData, nSize);
				PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)pEvent->pData;
				pMove->dwX = (DWORD)pMaze->byGoalToX;
				pMove->dwY = (DWORD)pMaze->byGoalToY;
				pMove->byEfect = EVENTMOVEEFECT_SILENT;


				/* ゴール地点へ */
				nNo = AddEvent (pMap->dwID);
				pEvent = &pMap->pEvent[nNo];
				pEvent->dwX = (DWORD)pMaze->byGoalToX;
				pEvent->dwY = (DWORD)pMaze->byGoalToY;
				pEvent->byType = EVENTTYPE_MAPCHG;
				pEvent->pData = NULL;

				nSize = GetEventDataSize (EVENTTYPE_MAPCHG);
				pEvent->pData = new BYTE[nSize];
				ZeroMemory (pEvent->pData, nSize);
				pMapChg = (PEVENTINFO_MAPCHG)pEvent->pData;
				pMapChg->dwMapID = pMap->dwID;
				pMapChg->dwX = (DWORD)nGoalX;
				pMapChg->dwY = (DWORD)nGoalY;
				pMapChg->byEfect = EVENTMOVEEFECT_SILENT;

				pMaze->byGoalFromX = (BYTE)nGoalX;
				pMaze->byGoalFromY = (BYTE)nGoalY;

				SendCmdEVENTPOS (URARASOCK_SENDALL, pMap->dwID);
				SendCmdSYSMSGMAP (pMap->dwID, "SYSTEM:迷路作成完了しました。");
			}
		}

	/* 作成中ではない */
	} else {
		if (g_aCharInfo.GetSize () == 0) {
			return;
		}
		PCHARINFO pChar = g_aCharInfo[pMaze->dwTblIDCheck];
		if (g_nProcChar == 0) {
			/* 誰もいないかもしれない？ */
			if (pMaze->dwCountTmp == 0) {
				if (pMaze->dwTimeCheck) {
					/* 5秒以上経過？ */
					if (g_dwTime > pMaze->dwTimeCheck + 5000) {
						/* 人数カウント情報をクリア */
						pMaze->dwCountTmp = 0;
						pMaze->dwTimeCheck = timeGetTime ();
						/* 迷路リニューアル */
						MakeMazeProc (g_nProcSubGame);
					}
				}

			} else {
				pMaze->dwTimeCheck = timeGetTime ();
				pMaze->dwCountTmp = 0;
			}
		}
		/* エリア内にいる？ */
		if ((pChar->dwMapID == pMaze->dwMapID) &&
			(pChar->dwX >= (DWORD)pMaze->byX1) && (pChar->dwX <= (DWORD)pMaze->byX2) &&
			(pChar->dwY >= (DWORD)pMaze->byY1) && (pChar->dwY <= (DWORD)pMaze->byY2)) {
			/* オフライン中？ */
			if (pChar->byGameState < GAMESTATE_MOVE) {
				/* 追い出す */
				pChar->dwMapID = pMaze->dwMapID;
				pChar->dwX = (DWORD)pMaze->byStartX;
				pChar->dwY = (DWORD)pMaze->byStartY;

			} else {
				/* カウンタを増やす */
				if (pMaze->dwCountTmp < -1) {
					pMaze->dwCountTmp ++;
				}
				pMaze->dwTimeCheck = g_dwTime;
			}
		}
		pMaze->dwTblIDCheck ++;
		pMaze->dwTblIDCheck = (pMaze->dwTblIDCheck >= (DWORD)g_aCharInfo.GetSize ()) ? 0 : pMaze->dwTblIDCheck;
	}
}

/* ========================================================================= */
/* 関数名：SubGameProc_DUNGEON												 */
/* 内容：サブゲーム処理(ダンジョン)											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void SubGameProc_DUNGEON(PSUBGAMEINFO pInfo)
{
	PSUBGAME_DUNGEONINFO pDungeon = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
	if (pDungeon == NULL) {
		return;
	}
	if (g_aCharInfo.GetSize () <= 0) {
		return;
	}
	PMAPINFO pMap = g_aMapInfo[pDungeon->dwMapID];

	/* マップ作成中？ */
	if (pDungeon->pMapTmp) {
		int cx, cy;
		cx = (pDungeon->byX2 - pDungeon->byX1) / 2 + 1;
		cy = (pDungeon->byY2 - pDungeon->byY1) / 2 + 1;

		WORD wParts = 0;
		switch (pDungeon->pMapTmp[cx * pDungeon->byTmpY + pDungeon->byTmpX]) {
		case 0:
			wParts = 0;
			break;
		case 1:	/* 道 */
		case 3:	/* 扉 */
			wParts = pDungeon->wChipIDRoad;
			break;

		case 2:	/* 壁 */
			wParts = pDungeon->wChipIDBlock;
			break;
		}
		int x, y;
		x = (int)(pDungeon->byTmpX * 2);
		y = (int)(pDungeon->byTmpY * 2);
		x += (int)pDungeon->byX1;
		y += (int)pDungeon->byY1;

		if ((x + 1 < pMap->nCX) && (y + 1 < pMap->nCY)) {
			pMap->pwMap[pMap->nCX * y + x] = wParts;
			pMap->pwMap[pMap->nCX * y + (x + 1)] = wParts;
			pMap->pwMap[(pMap->nCX * (y + 1)) + x] = wParts;
			pMap->pwMap[(pMap->nCX * (y + 1)) + x + 1] = wParts;
		}
		pDungeon->byTmpX ++;
		if (pDungeon->byTmpX >= cx) {
			pDungeon->byTmpX = 0;
			pDungeon->byTmpY ++;
			/* 完了？ */
			if (pDungeon->byTmpY >= cy) {
				SAFE_DELETE_ARRAY (pDungeon->pMapTmp);
				pDungeon->byTmpX = pDungeon->byTmpY = 0;

				/* スタート地点とゴール地点のイベント・マップを設置 */
				pMap->pwMap[pDungeon->byStartX + pDungeon->byStartY * pMap->nCX] = pDungeon->wChipIDStart;
				SendCmdCHGMAP (pMap->dwID, pDungeon->wChipIDStart, pDungeon->byStartX, pDungeon->byStartY);
				pMap->pwMap[pDungeon->byGoalX + (pDungeon->byGoalY * pMap->nCX)] = pDungeon->wChipIDGoal;

				/* スタート地点へ */
				int nNo = AddEvent (pMap->dwID);
				PEVENTINFO pEvent = &pMap->pEvent[nNo];
				pEvent->dwX = (DWORD)pDungeon->byStartX;
				pEvent->dwY = (DWORD)pDungeon->byStartY;
				pEvent->byType = EVENTTYPE_MAPCHG;
				pEvent->pData = NULL;

				int nSize = GetEventDataSize (EVENTTYPE_MAPCHG);
				pEvent->pData = new BYTE[nSize];
				ZeroMemory (pEvent->pData, nSize);
				PEVENTINFO_MAPCHG pMapChg = (PEVENTINFO_MAPCHG)pEvent->pData;
				pMapChg->dwMapID = pMap->dwID;
				pMapChg->dwX = (DWORD)pDungeon->byGoalX;
				pMapChg->dwY = (DWORD)pDungeon->byGoalY;
				pMapChg->byEfect = EVENTMOVEEFECT_SILENT;


				/* ゴールから脱出 */
				nNo = AddEvent (pMap->dwID);
				pEvent = &pMap->pEvent[nNo];
				pEvent->dwX = (DWORD)pDungeon->byGoalX;
				pEvent->dwY = (DWORD)pDungeon->byGoalY;
				pEvent->byType = EVENTTYPE_MAPCHG;
				pEvent->pData = NULL;

				nSize = GetEventDataSize (EVENTTYPE_MAPCHG);
				pEvent->pData = new BYTE[nSize];
				ZeroMemory (pEvent->pData, nSize);
				pMapChg = (PEVENTINFO_MAPCHG)pEvent->pData;
				pMapChg->dwMapID = pDungeon->dwMapID;
				pMapChg->dwX = (DWORD)pDungeon->byStartX;
				pMapChg->dwY = (DWORD)pDungeon->byStartY;
				pMapChg->byEfect = EVENTMOVEEFECT_SILENT;

				SendCmdEVENTPOS (URARASOCK_SENDALL, pMap->dwID);
				SendCmdSYSMSGMAP (pMap->dwID, "SYSTEM:ダンジョン作成完了しました。");
			}
		}

	/* 作成中ではない */
	} else {
		PCHARINFO pChar = g_aCharInfo[pDungeon->dwTblIDCheck];
		if (pDungeon->dwTblIDCheck == 0) {
			/* 誰もいないかもしれない？ */
			if (pDungeon->dwCountTmp == 0) {
				if (pDungeon->dwTimeCheck) {
					/* 5秒以上経過？ */
					if (g_dwTime > pDungeon->dwTimeCheck + 5000) {
						/* 人数カウント情報をクリア */
						pDungeon->dwCountTmp = 0;
						pDungeon->dwTimeCheck = timeGetTime ();
						/* ダンジョンリニューアル */
						MakeDungeonProc (g_nProcSubGame);
					}
				}

			} else {
				pDungeon->dwTimeCheck = timeGetTime ();
				pDungeon->dwCountTmp = 0;
			}
		}
		/* エリア内にいる？ */
		if ((pChar->dwMapID == pDungeon->dwMapID) &&
			(pChar->dwX >= (DWORD)pDungeon->byX1) && (pChar->dwX <= (DWORD)pDungeon->byX2) &&
			(pChar->dwY >= (DWORD)pDungeon->byY1) && (pChar->dwY <= (DWORD)pDungeon->byY2)) {
			/* オフライン中？ */
			if (pChar->byGameState < GAMESTATE_MOVE) {
				/* 追い出す */
				pChar->dwMapID = pDungeon->dwMapID;
				pChar->dwX = (DWORD)pDungeon->byStartX;
				pChar->dwY = (DWORD)pDungeon->byStartY;

			} else {
				/* カウンタを増やす */
				if (pDungeon->dwCountTmp < -1) {
					pDungeon->dwCountTmp ++;
				}
				pDungeon->dwTimeCheck = g_dwTime;
			}
		}
		pDungeon->dwTblIDCheck ++;
		pDungeon->dwTblIDCheck = (pDungeon->dwTblIDCheck >= (DWORD)g_aCharInfo.GetSize ()) ? 0 : pDungeon->dwTblIDCheck;
	}
}

/* ========================================================================= */
/* 関数名：SubGameProc_WAR													 */
/* 内容：サブゲーム処理(戦場ゲーム)											 */
/* 日付：2004/08/09															 */
/* ========================================================================= */
static void SubGameProc_WAR(PSUBGAMEINFO pInfo)
{
	return;

#if 0
//	int i, nTmp;
//	DWORD dwTime;
//
//	if (g_aCharInfo.GetSize () == 0) {
//		return;
//	}
//	PSUBGAME_WARINFO pWar = (PSUBGAME_WARINFO)pInfo->pInfo;
//	dwTime = timeGetTime ();
//
//	switch (pWar->byState) {
//	case SUBGAME_WAR_STATE_INIT:		/* 初期状態 */
//		pWar->dwTblIDJudge = -1;
//		for (i = 0; i < MAX_SUBGAME_WARMEMBER; i ++) {
//			pWar->adwTblID1[i] = -1;
//			pWar->adwTblID2[i] = -1;
//			pWar->adwTblIDHP1[i] = -1;
//			pWar->adwTblIDHP2[i] = -1;
//			pWar->adwTblIDRes1[i] = -1;
//			pWar->adwTblIDRes2[i] = -1;
//			pWar->adwTblIDTargetRes1[i] = -1;
//			pWar->adwTblIDTargetRes2[i] = -1;
//		}
//		pWar->byState = SUBGAME_WAR_STATE_CLEAN;
//		break;
//
//	case SUBGAME_WAR_STATE_CLEAN:		/* 初期化 */
//		pWar->byRule = 0;
//		pWar->byTurn = 0;
//		pWar->byTurn1 = 0;
//		pWar->byTurn2 = 0;
//		pWar->dwFirstTimeProc = 0;
//		pWar->dwLastTimeProc = 0;
//		pWar->dwNextTimeProc = 0;
//		pWar->dwWaitTblID = -1;
//		ZeroMemory (pWar->abyJob1, sizeof (pWar->abyJob1));
//		ZeroMemory (pWar->abyJob2, sizeof (pWar->abyJob2));
//
//		if (pWar->dwTblIDJudge == -1) {
//			pWar->dwTblIDJudge = CheckCharBlockNPC (pWar->dwMapID, pWar->byJudgeX, pWar->byJudgeY);
//		}
//
//		for (i = 0; i < MAX_SUBGAME_WARMEMBER; i ++) {
//			if (pWar->adwTblIDHP1[i] == -1) {
//				pWar->adwTblIDHP1[i] = CheckCharBlockNPC (pWar->dwMapID, pWar->byHP1X + i, pWar->byHPY);
//			}
//			if (pWar->adwTblIDHP2[i] == -1) {
//				pWar->adwTblIDHP2[i] = CheckCharBlockNPC (pWar->dwMapID, pWar->byHP2X + i, pWar->byHPY);
//			}
//			if (pWar->adwTblIDRes1[i] == -1) {
//				pWar->adwTblIDRes1[i] = CheckCharBlockNPC (pWar->dwMapID, pWar->byRes1X + i, pWar->byResY);
//			}
//			if (pWar->adwTblIDRes2[i] == -1) {
//				pWar->adwTblIDRes2[i] = CheckCharBlockNPC (pWar->dwMapID, pWar->byRes2X + i, pWar->byResY);
//			}
//		}
//		pWar->byState = SUBGAME_WAR_STATE_READY;
//		break;
//
//	case SUBGAME_WAR_STATE_READY:		/* 準備完了 */
//		break;
//
//	case SUBGAME_WAR_STATE_JOIN:		/* 参加者募集中 */
//		if (dwTime < pWar->dwNextTimeProc) {
//			break;
//		}
//
//		pWar->dwLastTimeProc = dwTime;
//		pWar->dwNextTimeProc = dwTime + 10 * 1000;
//		nTmp = abs (pWar->dwFirstTimeProc - dwTime) / 1000;
//		/* 規定時間経過した？ */
//		if (nTmp >= SUBWAR_WAITTIME_JOIN) {
//			/* どちらかのチームが不参加？ */
//			if ((pWar->adwTblID1[0] == -1) ||
//				(pWar->adwTblID2[0] == -1)) {
//				SubWar_JudgeMsg (pWar, "相手チームがいません。無効試合となりました。");
//				SubWar_Quit (pWar);
//				break;
//			}
//
//			SubWar_JudgeMsg (pWar, "参加者募集終了しました。");
//			SubWar_JudgeMsg (pWar, "参加者の方はルールを選択してください。");
//
//			pWar->dwLastTimeProc = 0;
//			pWar->dwFirstTimeProc = timeGetTime ();
//			pWar->dwNextTimeProc = pWar->dwFirstTimeProc + 10 * 1000;
//
//			/* ルール選択に状態遷移 */
//			pWar->byState = SUBGAME_WAR_STATE_SELECTRULE;
//			SubWar_AllChgStateAndQuit (pWar, SUBGAME_WAR_STATE_SELECTRULE, SUBGAME_WAR_STATE_WAIT);
//
//		} else {
//			char szMsg[256];
//
//			/* 残り時間表示 */
//			wsprintf (szMsg, "参加者募集中。残り %d 秒", SUBWAR_WAITTIME_JOIN - nTmp);
//			SubWar_JudgeMsg (pWar, szMsg);
//		}
//		break;
//
//	case SUBGAME_WAR_STATE_SELECTRULE:	/* ルール選択 */
//		if (dwTime < pWar->dwNextTimeProc) {
//			break;
//		}
//
//		pWar->dwLastTimeProc = dwTime;
//		pWar->dwNextTimeProc = dwTime + 10 * 1000;
//		nTmp = abs (pWar->dwFirstTimeProc - dwTime) / 1000;
//		/* 規定時間経過した？ */
//		if (nTmp >= SUBWAR_WAITTIME) {
//			char szMsg[256];
//
//			SubWar_JudgeMsg (pWar, "ルール選択終了しました。");
//			if (pWar->byRule == 0) {
//				strcpy (szMsg, "[全滅させる]で試合を行います。");
//			} else {
//				strcpy (szMsg, "[城へ到達]で試合を行います。");
//			}
//			SubWar_JudgeMsg (pWar, szMsg);
//			SubWar_JudgeMsg (pWar, "参加者の方は職業を選択してください。");
//
//			pWar->dwLastTimeProc = 0;
//			pWar->dwFirstTimeProc = timeGetTime ();
//			pWar->dwNextTimeProc = pWar->dwFirstTimeProc + 10 * 1000;
//
//			/* 職業選択に状態遷移 */
//			pWar->byState = SUBGAME_WAR_STATE_SELECTJOB;
//			SubWar_AllChgState (pWar, SUBGAME_WAR_STATE_SELECTJOB);
//
//		} else {
//			char szMsg[256];
//
//			/* 残り時間表示 */
//			wsprintf (szMsg, "ルール選択中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
//			SubWar_JudgeMsg (pWar, szMsg);
//		}
//		break;
//
//	case SUBGAME_WAR_STATE_SELECTJOB:	/* 職業選択 */
//		if (dwTime < pWar->dwNextTimeProc) {
//			break;
//		}
//
//		pWar->dwLastTimeProc = dwTime;
//		pWar->dwNextTimeProc = dwTime + 10 * 1000;
//		nTmp = abs (pWar->dwFirstTimeProc - dwTime) / 1000;
//		/* 規定時間経過した？ */
//		if (nTmp >= SUBWAR_WAITTIME) {
//			SubWar_JudgeMsg (pWar, "職業選択終了しました。");
//			SubWar_JudgeMsg (pWar, "参加者の方は行動順を設定してください。");
//
//			pWar->dwLastTimeProc = 0;
//			pWar->dwFirstTimeProc = timeGetTime ();
//			pWar->dwNextTimeProc = pWar->dwFirstTimeProc + 10 * 1000;
//
//			nTmp = GetSubGameID (pInfo);
//			SendCmdSubWarSELECTTURN ((BYTE)nTmp, pWar, 0);
//			SendCmdSubWarSELECTTURN ((BYTE)nTmp, pWar, 1);
//
//			/* 順番設定に状態遷移 */
//			pWar->byState = SUBGAME_WAR_STATE_SELECTTURN;
//			SubWar_AllChgState (pWar, SUBGAME_WAR_STATE_SELECTTURN);
//
//		} else {
//			char szMsg[256];
//
//			/* 残り時間表示 */
//			wsprintf (szMsg, "職業選択中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
//			SubWar_JudgeMsg (pWar, szMsg);
//		}
//		break;
//
//	case SUBGAME_WAR_STATE_SELECTTURN:	/* 順番設定 */
//		if (dwTime < pWar->dwNextTimeProc) {
//			break;
//		}
//
//		pWar->dwLastTimeProc = dwTime;
//		pWar->dwNextTimeProc = dwTime + 10 * 1000;
//		nTmp = abs (pWar->dwFirstTimeProc - dwTime) / 1000;
//		/* 規定時間経過した？ */
//		if (nTmp >= SUBWAR_WAITTIME) {
//			SubWar_JudgeMsg (pWar, "順番設定終了しました。");
//
//			pWar->dwLastTimeProc = 0;
//			pWar->dwFirstTimeProc = 0;
//			pWar->dwNextTimeProc = 0;
//
//			/* 試合準備に状態遷移 */
//			pWar->byState = SUBGAME_WAR_STATE_GAMEPREPARE;
//
//		} else {
//			char szMsg[256];
//
//			/* 残り時間表示 */
//			wsprintf (szMsg, "行動順設定中。残り %d 秒", SUBWAR_WAITTIME - nTmp);
//			SubWar_JudgeMsg (pWar, szMsg);
//		}
//		break;
//
//	case SUBGAME_WAR_STATE_GAMEPREPARE:	/* 試合準備 */
//		{
//			SubWar_JudgeMsg (pWar, "戦場ゲーム戦闘開始！");
//			SendCmdPLAYSOUNDinside (pWar->dwTblIDJudge, SOUND_WHISTLE);
//
//			pWar->byTurn = (BYTE)(genrand () % 2);
//
//			if (pWar->byTurn == 0) {
//				SubWar_JudgeMsg (pWar, "先攻は赤チームです！");
//			} else {
//				SubWar_JudgeMsg (pWar, "先攻は青チームです！");
//			}
//
//			/* 準備処理 */
//			SubWar_PrePare (pWar);
//
//			/* 試合中に状態遷移 */
//			pWar->byState = SUBGAME_WAR_STATE_GAME;
//
//			SubWar_ChgTurn (pWar);
//		}
//		break;
//
//	case SUBGAME_WAR_STATE_GAME:		/* 試合中 */
//		break;
//	}
#endif
}

/* ========================================================================= */
/* 関数名：SubGameProc_SUGOROKU												 */
/* 内容：サブゲーム処理(すごろく)											 */
/* 日付：2004/12/21															 */
/* ========================================================================= */
static void SubGameProc_SUGOROKU(PSUBGAMEINFO pInfo)
{
	int nTmp;
	DWORD dwTime;
	PSUBGAME_SUGOROKUINFO pSugoroku;

	if (g_aCharInfo.GetSize () == 0) {
		return;
	}
	pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
	if (pSugoroku == NULL) {
		return;
	}

	dwTime = timeGetTime ();

	switch (pSugoroku->byState) {
	case SUBGAME_SUGOROKU_STATE_INIT:		/* 初期状態 */
		pSugoroku->byState = SUBGAME_SUGOROKU_STATE_CLEAN;
		break;

	case SUBGAME_SUGOROKU_STATE_CLEAN:		/* 初期化 */
		pSugoroku->byState = SUBGAME_SUGOROKU_STATE_READY;
		break;

	case SUBGAME_SUGOROKU_STATE_READY:		/* 準備完了 */
		break;

	case SUBGAME_SUGOROKU_STATE_JOIN:		/* 参加者募集中 */
		if (dwTime < pSugoroku->dwNextTimeProc) {
			break;
		}

		pSugoroku->dwLastTimeProc = dwTime;
		pSugoroku->dwNextTimeProc = dwTime + 10 * 1000;
		nTmp = abs (pSugoroku->dwFirstTimeProc - dwTime) / 1000;
#if 0	//デバッグ
		if (nTmp >= 10) {
#else
		/* 規定時間経過した？ */
		if (nTmp >= SUBSUGOROKU_WAITTIME_JOIN) {
			/* 参加者が二人未満？ */
			if (pSugoroku->adwPlayerTblID.GetSize () < 2) {
				SubSugoroku_Msg (pSugoroku, "参加者が足りない為ゲーム開始できません。");
				SubSugoroku_Quit (pSugoroku);
				break;
			}
#endif
			SubSugoroku_Msg (pSugoroku, "参加者募集終了しました。");
			SubSugoroku_Msg (pSugoroku, "ゲーム開始します。");

			pSugoroku->dwLastTimeProc = 0;
			pSugoroku->dwFirstTimeProc = timeGetTime ();
			pSugoroku->dwNextTimeProc = pSugoroku->dwFirstTimeProc + 10 * 1000;
			SubSugoroku_GetStartPos (pSugoroku, &pSugoroku->dwStartX, &pSugoroku->dwStartY);
			SubSugoroku_GetGoalPos (pSugoroku, &pSugoroku->dwGoalX, &pSugoroku->dwGoalY);

			/* 状態遷移 */
			pSugoroku->byState = SUBGAME_SUGOROKU_STATE_COMMAND;

		} else {
			char szMsg[256];

			/* 残り時間表示 */
			wsprintf (szMsg, "参加者募集中。残り %d 秒", SUBSUGOROKU_WAITTIME_JOIN - nTmp);
			SubSugoroku_Msg (pSugoroku, szMsg);
		}
		break;

	case SUBGAME_SUGOROKU_STATE_COMMAND:	/* 行動選択 */
		{
			char szTmp[256];
			BYTE byNo;
			PCHARINFO pChar;
			PSUGOROKUPLAYERINFO pPlayerInfo;

			byNo = pSugoroku->byProcNo;
			pPlayerInfo = &pSugoroku->aPlayerInfo[byNo];
			pChar = g_aCharInfo[pPlayerInfo->dwTblID];

			/* ゴール済み？ */
			if (pPlayerInfo->byGoal) {
				SubSugoroku_NextPlayer (pSugoroku);
				break;
			}

			wsprintf (szTmp, "%sの行動です。", pChar->szName);
			SubSugoroku_Msg (pSugoroku, szTmp);

			if (pPlayerInfo->byWait > 0) {
				wsprintf (szTmp, "%sは休み中です。", pChar->szName);
				SubSugoroku_Msg (pSugoroku, szTmp);
				SubSugoroku_NextPlayer (pSugoroku);

				pPlayerInfo->byWait --;
				break;
			}

			SendCmdSubSugorokuCHGSTATE (pChar->pID, SUBGAME_SUGOROKU_STATE_COMMAND);

			/* 状態遷移 */
			pSugoroku->byState = SUBGAME_SUGOROKU_STATE_WAIT;
		}
		break;

	case SUBGAME_SUGOROKU_STATE_MOVE:		/* 移動中 */
		{
			BOOL bDirection, bResult;
			BYTE byNo;
			PSUGOROKUPLAYERINFO pPlayerInfo;

			if (abs (dwTime - pSugoroku->dwLastTimeProc) < 1000) {
				break;
			}

			byNo = pSugoroku->byProcNo;
			pPlayerInfo = &pSugoroku->aPlayerInfo[byNo];

			SubSugoroku_Move (pSugoroku, byNo);
			pSugoroku->dwLastTimeProc = dwTime;
			pSugoroku->byLastMove --;
			bDirection = FALSE;

			if (pSugoroku->byLastMove == 0) {
				bResult = TRUE;
				/* 逆走中？ */
				if (pPlayerInfo->byBack) {
					pPlayerInfo->byBack = 0;
					pPlayerInfo->byDirection = abyDirectionTbl[pPlayerInfo->byDirection];
					bDirection = TRUE;

				} else {
					bResult = SubSugoroku_CheckParts (pSugoroku, byNo);
					/* 全員ゴールした？ */
					if (pSugoroku->byState == SUBGAME_SUGOROKU_STATE_INIT) {
						break;
					}
				}

				if (bResult) {
					pPlayerInfo->byNoCheck = 0;
					SubSugoroku_NextPlayer (pSugoroku);
				}
			}

			if (bDirection == FALSE) {
				int nTmp;

				/* 次の向きを設定しておく */
				nTmp = SubSugoroku_GetNextDirection (pSugoroku, byNo);
				pPlayerInfo->byDirection = (BYTE)nTmp;
			}

			if (pSugoroku->byLastMove > 0) {
				if (pSugoroku->byRule == SUBGAME_SUGOROKU_RULE_GOAL_NONSTOP) {
					/* ゴールの上？ */
					if ((pPlayerInfo->dwX == pSugoroku->dwGoalX) && (pPlayerInfo->dwY == pSugoroku->dwGoalY)) {
						pPlayerInfo->byNoCheck = 0;
						SubSugoroku_NextPlayer (pSugoroku);
						pSugoroku->byLastMove = 0;
						SubSugoroku_GoalProc (pSugoroku, byNo);
					}
				}
			}
		}
		break;

	case SUBGAME_SUGOROKU_STATE_WAIT:		/* 待ち状態 */
		break;
	}
}

/* ========================================================================= */
/* 関数名：SubGameProcBombQuit												 */
/* 内容：爆弾ゲーム試合終了処理												 */
/* 日付：2003/12/21															 */
/* ========================================================================= */
void SubGameProcBombQuit(BYTE byID)
{
	SubGameProcBombQuitMsg (byID, NULL);
}

/* ========================================================================= */
/* 関数名：SubGameProcBombQuitMsg											 */
/* 内容：爆弾ゲーム試合終了処理(参加者に終わった理由を表示)					 */
/*       pszMsg が NULL なら何も表示しない									 */
/* 日付：2026/11/04															 */
/* ========================================================================= */
void SubGameProcBombQuitMsg(BYTE byID, LPCSTR pszMsg)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pBomb->dwMapID];
	PDWORD pIDTbl = pMap->paTblNoPC;
	PCHARINFO pChar;

	/* 誰もいない？ */
	if (pIDTbl == NULL) {
		return;
	}

	/* 試合終了通知を送信 */
	SendCmdSubBOMBSTOP (URARASOCK_SENDALL, byID, pBomb);
	if (pIDTbl) {
		for (int i = 0; ; i ++) {
			if (pIDTbl[i] == -1) {
				break;
			}
			pChar = g_aCharInfo[pIDTbl[i]];
			PSUBSTATUS_BOMB pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;
			/* 爆弾ゲーム参加中？ */
			if (pSubStatus->byType == SUBGAMEID_BOMB) {
				/* ゲームIDが一致？ */
				if (byID == pSubStatus->byID) {
					/* 終わった理由を知らせる（何も出ないと時間切れなどに気づけないため） */
					if (pszMsg) {
						SendCmdSYSMSG (pChar->pID, pszMsg, SPEECHCMD_BATTLE1);
						SendCmdPLAYSOUND (pChar->pID, SOUND_WHISTLE);
					}
					/* 参加中？ */
					if ((pSubStatus->byFlgEntry) ||
						/* ゲームマスター以外で待機中？ */
						((!pSubStatus->byFlgEntry) && (!pSubStatus->byFlgGameMaster))) {
						/* 受け付け地点へ飛ばす */
						SendCmdFADEINOUT (pChar->pID, 0);
						pChar->byDirection = 1;
						pChar->byDirectionBack2 = pChar->byDirection;
						pChar->byDirectionBack = pChar->byDirection;
						pChar->dwX = pBomb->dwX;
						pChar->dwY = pBomb->dwY;
						pChar->dwXBack = pChar->dwX;
						pChar->dwYBack = pChar->dwY;
						SendCmdCHARINFO (pChar->pID, -1);
						SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
						SendCmdFADEINOUT (pChar->pID, 1);
					}
					/* サブゲーム情報をクリア */
					ZeroMemory (pSubStatus, sizeof (SUBSTATUS_BOMB));
					SendCmdSUBSTATUS (pChar->dwTblID);
				}
			}
		}
	}
	/* ゲーム情報を初期化 */
	SubGameProcBombDeleteBlock (byID);
	pBomb->byFlgGameNow = 0;
	pBomb->byFlgProgress = 0;
	pBomb->byFlgUseBlock = 0;
	pBomb->byFlgUseItem = 0;
	pBomb->byGameTime = 60;
	pBomb->byGameTimeNow = 0;
	pBomb->byMaxBomb = 1;
	pBomb->byMaxFire = 3;
	pBomb->dwLastTime = 0;
	pBomb->dwTblIDGameMaster = -1;
}

/* ========================================================================= */
/* 関数名：SubGameProcBombMakeBlock											 */
/* 内容：爆弾ゲーム壁作成処理												 */
/* 日付：2003/12/28															 */
/* ========================================================================= */
void SubGameProcBombMakeBlock(BYTE byID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pBomb->dwMapID];

	/* 試合中？ */
	if (pBomb->byFlgGameNow) {
		return;
	}

	int x, y, nCount = 0, nCountMax = pBomb->byX2 / 2;
	for (y = pBomb->byY1; y <= pBomb->byY2; y ++) {
		nCount = 0;
		for (x = pBomb->byX1; x <= pBomb->byX2; x ++) {
			/* 壁作成？ */
			if ((nCount < nCountMax) && (genrand () % 100 >= 70)) {
				BYTE byHitType = (&g_aMapPartsInfo[pMap->pwMap[x + y * pMap->nCX]])->byHitType;
				/* 通れない所？ */
				if (((byHitType != PARTSHIT_NONE) &&
					 (byHitType != PARTSHIT_MOVEFRONT) &&
					 (byHitType != PARTSHIT_MOVEUP) &&
					 (byHitType != PARTSHIT_MOVEDOWN) &&
					 (byHitType != PARTSHIT_MOVELEFT) &&
					 (byHitType != PARTSHIT_MOVERIGHT) &&
					 (byHitType != PARTSHIT_FOOTMARK_SNOW) &&
					 (byHitType != PARTSHIT_FOOTMARK_SAND))) {
					continue;
				}
				nCount ++;

				/* 壁の追加 */
				PCHARINFO pCharTmp = new CHARINFO;
				ZeroMemory ((PBYTE)pCharTmp, sizeof (CHARINFO));
				pCharTmp->byNpcType = NPCTYPE_BOMBBLOCK;
				SetInitCharData (pCharTmp);
				pCharTmp->wGrpID = 0;
				pCharTmp->dwMapID = pMap->dwID;
				pCharTmp->dwX = (DWORD)x;
				pCharTmp->dwY = (DWORD)y;
				pCharTmp->byDirection = 1;
				pCharTmp->byGameState = GAMESTATE_NPC;
				pCharTmp->byFlg |= CHARINFOFLG_MOVE;

				PSUBSTATUS_BOMB pSubStatusTmp = (PSUBSTATUS_BOMB)pCharTmp->abySubStatus;
				pSubStatusTmp->byID = byID;
				pSubStatusTmp->byType = pInfo->byType;
				pCharTmp = AddChar (pCharTmp);
				pCharTmp->pID = pCharTmp->dwTblID + 1;
			}
		}
	}
}

/* ========================================================================= */
/* 関数名：SubGameProcBombDeleteBlock										 */
/* 内容：爆弾ゲーム壁消去処理												 */
/* 日付：2003/12/28															 */
/* ========================================================================= */
void SubGameProcBombDeleteBlock(BYTE byID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PSUBGAME_BOMBINFO pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pBomb->dwMapID];

	PDWORD pData;
	BOOL bLoop;
	while (1) {
		bLoop = FALSE;
		pData = pMap->paTblNoNPC;
		for (int i = 0; i < (int)pMap->dwCountTblNoNPC; i ++) {
			if (pData[i] == -1) {
				break;
			}
			PCHARINFO pCharTmp = g_aCharInfo[pData[i]];
			/* 壁かアイテム以外？ */
			if ((pCharTmp->byNpcType != NPCTYPE_BOMBBLOCK) &&
				(pCharTmp->byNpcType != NPCTYPE_BOMBITEM)) {
				continue;
			}
			/* エリア内にある？ */
			if ((pCharTmp->dwX >= (DWORD)pBomb->byX1) && (pCharTmp->dwX <= (DWORD)pBomb->byX2) &&
				(pCharTmp->dwY >= (DWORD)pBomb->byY1) && (pCharTmp->dwY <= (DWORD)pBomb->byY2)) {
				/* 消去 */
				DeleteChar (pCharTmp->dwTblID);
				bLoop = TRUE;
				break;
			}
		}
		if (bLoop == FALSE) {
			break;
		}
	}
}

/* ========================================================================= */
/* 関数名：StopMaze															 */
/* 内容：迷路を止める（入口・ゴールのイベントを外し、作成中ならやめる）		 */
/*       データAPIで種類を「無し」にする時に使う							 */
/* ========================================================================= */
void StopMaze(BYTE byID)
{
	if (byID >= g_aSubaGameInfo.GetSize ()) {
		return;
	}
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	if (pInfo == NULL || pInfo->pInfo == NULL || pInfo->byType != SUBGAMEID_MAZE) {
		return;
	}
	PSUBGAME_MAZEINFO pMaze = (PSUBGAME_MAZEINFO)pInfo->pInfo;
	SAFE_DELETE_ARRAY (pMaze->pMapTmp);
	if (pMaze->dwMapID >= (DWORD)g_aMapInfo.GetSize () || g_aMapInfo[pMaze->dwMapID] == NULL) {
		return;
	}
	int aX[3] = {pMaze->byStartX, pMaze->byGoalFromX, pMaze->byGoalToX};
	int aY[3] = {pMaze->byStartY, pMaze->byGoalFromY, pMaze->byGoalToY};
	for (int i = 0; i < 3; i ++) {
		int nNo = SearchEvent (pMaze->dwMapID, (DWORD)aX[i], (DWORD)aY[i]);
		if (nNo >= 0) {
			DeleteEvent (pMaze->dwMapID, nNo);
		}
	}
	SendCmdEVENTPOS (URARASOCK_SENDALL, pMaze->dwMapID);
}

/* ========================================================================= */
/* 関数名：GetMazePathWidth													 */
/* 内容：迷路の通路の太さ（1～3マス。0や範囲外は昔からの2マス）				 */
/* ========================================================================= */
static int GetMazePathWidth(PSUBGAME_MAZEINFO pMaze)
{
	int nWidth = (int)pMaze->byPathWidth;
	if ((nWidth < 1) || (nWidth > 3)) {
		nWidth = 2;
	}
	return nWidth;
}

/* ========================================================================= */
/* 関数名：GetMazeTileParts													 */
/* 内容：迷路の1マスに置くパーツを決める									 */
/*       nCell 0=何も無し 1=道 2=壁、bFace=壁の手前の面にする段か			 */
/*       混ぜるパーツが0なら混ぜない、手前の面のパーツが0なら使わない		 */
/* ========================================================================= */
static WORD GetMazeTileParts(int nCell, BOOL bFace,
		WORD wRoad, WORD wRoad2, BYTE byRoad2Rate,
		WORD wBlock, WORD wBlock2, BYTE byBlock2Rate, WORD wBlockFace)
{
	switch (nCell) {
	case 1:	/* 道 */
		if (wRoad2 && byRoad2Rate && ((int)((unsigned long)genrand () % 100) < (int)byRoad2Rate)) {
			return wRoad2;
		}
		return wRoad;

	case 2:	/* 壁 */
		if (bFace && wBlockFace) {
			return wBlockFace;
		}
		if (wBlock2 && byBlock2Rate && ((int)((unsigned long)genrand () % 100) < (int)byBlock2Rate)) {
			return wBlock2;
		}
		return wBlock;
	}
	return 0;
}

/* ========================================================================= */
/* 関数名：MakeMazeWeave													 */
/* 内容：立体交差のある迷路の設計図(0=道 1=壁 2=縦に渡る橋 3=横に渡る橋)	 */
/*       奇数の座標が部屋、その間が通路。先に橋の交差を決めて、橋の上の	 */
/*       道(上下か左右)と下の道をそれぞれつなぎ、残りはクラスカル法で		 */
/*       全部の部屋が1本道でつながるように壁を壊す(行き止まりも残る)		 */
/* ========================================================================= */
static int MazeFindRoot(std::vector<int> &aParent, int n)
{
	while (aParent[n] != n) {
		aParent[n] = aParent[aParent[n]];
		n = aParent[n];
	}
	return n;
}

static void MakeMazeWeave(PBYTE pMap, int cx, int cy, PSUBGAME_MAZEINFO pMaze)
{
	int i, j, nRX, nRY, nRoom;

	/* 部屋の数(部屋 i,j はマス 2i+1,2j+1。右と下の端は壁) */
	nRX = (cx - 1) / 2;
	nRY = (cy - 1) / 2;
	nRoom = nRX * nRY;

	FillMemory (pMap, cx * cy, 1);
	for (j = 0; j < nRY; j ++) {
		for (i = 0; i < nRX; i ++) {
			pMap[(j * 2 + 1) * cx + (i * 2 + 1)] = 0;
		}
	}

	std::vector<int> aParent (nRoom);
	std::vector<BYTE> abyCross (nRoom, 0);
	for (i = 0; i < nRoom; i ++) {
		aParent[i] = i;
	}

	/* 交差する部屋を決める(端の部屋と、交差の隣は使わない) */
	std::vector<int> aOrder;
	for (j = 1; j < nRY - 1; j ++) {
		for (i = 1; i < nRX - 1; i ++) {
			aOrder.push_back (j * nRX + i);
		}
	}
	for (i = (int)aOrder.size () - 1; i > 0; i --) {
		int k = (int)((unsigned long)genrand () % (unsigned long)(i + 1));
		std::swap (aOrder[i], aOrder[k]);
	}
	for (size_t n = 0; n < aOrder.size (); n ++) {
		int r = aOrder[n];
		int ri = r % nRX, rj = r / nRX;
		int rN = r - nRX, rS = r + nRX, rW = r - 1, rE = r + 1;

		if ((int)((unsigned long)genrand () % 100) >= (int)pMaze->byCrossRate) {
			continue;
		}
		if (abyCross[r] || abyCross[rN] || abyCross[rS] || abyCross[rW] || abyCross[rE]) {
			continue;
		}
		if (MazeFindRoot (aParent, rN) == MazeFindRoot (aParent, rS)) {
			continue;
		}
		if (MazeFindRoot (aParent, rW) == MazeFindRoot (aParent, rE)) {
			continue;
		}
		/* 上下と左右をつなぐ(輪ができないよう、つなぐ前に別々か確かめる) */
		{
			int a = MazeFindRoot (aParent, rN), b = MazeFindRoot (aParent, rS);
			int c = MazeFindRoot (aParent, rW), d = MazeFindRoot (aParent, rE);
			/* 上下と左右が同じ組み合わせになると輪ができる */
			if (((a == c) && (b == d)) || ((a == d) && (b == c))) {
				continue;
			}
			aParent[a] = b;
			c = MazeFindRoot (aParent, rW);
			d = MazeFindRoot (aParent, rE);
			if (c == d) {
				/* 左右がつながってしまった(上下をつないだ時に一緒になった)。元に戻す */
				aParent[a] = a;
				continue;
			}
			aParent[c] = d;
		}
		/* どちらを橋(上)にするか。橋のパーツがある向きだけ */
		BYTE byBridge;
		if (pMaze->wChipIDBridgeV && pMaze->wChipIDBridgeH) {
			byBridge = (genrand () % 2) ? 2 : 3;
		} else {
			byBridge = pMaze->wChipIDBridgeV ? 2 : 3;
		}
		abyCross[r] = byBridge;
		int x = ri * 2 + 1, y = rj * 2 + 1;
		pMap[y * cx + x] = byBridge;
		pMap[(y - 1) * cx + x] = 0;
		pMap[(y + 1) * cx + x] = 0;
		pMap[y * cx + x - 1] = 0;
		pMap[y * cx + x + 1] = 0;
	}

	/* 残りの壁をクラスカル法で壊す(交差の部屋に面した壁はもう壊してある) */
	std::vector<int> aWall;		/* 部屋番号*2 + (0=右の壁 1=下の壁) */
	for (j = 0; j < nRY; j ++) {
		for (i = 0; i < nRX; i ++) {
			int r = j * nRX + i;
			if (abyCross[r]) {
				continue;
			}
			if ((i + 1 < nRX) && (abyCross[r + 1] == 0)) {
				aWall.push_back (r * 2);
			}
			if ((j + 1 < nRY) && (abyCross[r + nRX] == 0)) {
				aWall.push_back (r * 2 + 1);
			}
		}
	}
	for (i = (int)aWall.size () - 1; i > 0; i --) {
		int k = (int)((unsigned long)genrand () % (unsigned long)(i + 1));
		std::swap (aWall[i], aWall[k]);
	}
	for (size_t n = 0; n < aWall.size (); n ++) {
		int r = aWall[n] / 2;
		int r2 = (aWall[n] % 2) ? r + nRX : r + 1;
		int a = MazeFindRoot (aParent, r), b = MazeFindRoot (aParent, r2);
		if (a == b) {
			continue;
		}
		aParent[a] = b;
		int x = (r % nRX) * 2 + 1, y = (r / nRX) * 2 + 1;
		if (aWall[n] % 2) {
			pMap[(y + 1) * cx + x] = 0;
		} else {
			pMap[y * cx + x + 1] = 0;
		}
	}
}

/* ========================================================================= */
/* 関数名：MakeMazeProc														 */
/* 内容：迷路作成処理														 */
/* 日付：2004/02/28															 */
/* ========================================================================= */
void MakeMazeProc(BYTE byID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PSUBGAME_MAZEINFO pMaze = (PSUBGAME_MAZEINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pMaze->dwMapID];

	/* 現在作成中？ */
	if (pMaze->pMapTmp) {
		return;
	}

	SendCmdSYSMSGMAP (pMaze->dwMapID, "SYSTEM:迷路作成開始します。");

	/* スタート地点用イベント削除 */
	int nNo = SearchEvent (pMaze->dwMapID, (DWORD)pMaze->byStartX, (DWORD)pMaze->byStartY);
	if (nNo >= 0) {
		DeleteEvent (pMaze->dwMapID, nNo);
		pMap->pwMap[pMaze->byStartX + pMaze->byStartY * pMap->nCX] = 1;
		SendCmdCHGMAP (pMap->dwID, 1, pMaze->byStartX, pMaze->byStartY);
	}

	/* ゴール地点用イベント削除 */
	nNo = SearchEvent (pMaze->dwMapID, (DWORD)pMaze->byGoalFromX, (DWORD)pMaze->byGoalFromY);
	if (nNo >= 0) {
		DeleteEvent (pMaze->dwMapID, nNo);
	}
	nNo = SearchEvent (pMaze->dwMapID, (DWORD)pMaze->byGoalToX, (DWORD)pMaze->byGoalToY);
	if (nNo >= 0) {
		DeleteEvent (pMaze->dwMapID, nNo);
	}

	/* 迷路内の人を追い出す。 */
	PDWORD pTbl = pMap->paTblNoPC;
	BOOL bFlg = FALSE;
	int i;
	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pTbl[i] == -1) {
			if (bFlg == FALSE) {
				break;
			}
			i = -1;
			bFlg = FALSE;
			continue;
		}
		PCHARINFO pChar = g_aCharInfo[pTbl[i]];
		/* エリア内にいる？ */
		if ((pChar->dwMapID == pMaze->dwMapID) &&
			(pChar->dwX >= (DWORD)pMaze->byX1) && (pChar->dwX <= (DWORD)pMaze->byX2) &&
			(pChar->dwY >= (DWORD)pMaze->byY1) && (pChar->dwY <= (DWORD)pMaze->byY2)) {
			/* 追い出す */
			bFlg = TRUE;
			svrSock.SendCancel (pChar->pID);
			CharTargetCancelPC (pChar);
			pChar->byDirection = 1;
			pChar->byDirectionBack2 = pChar->byDirection;
			pChar->byDirectionBack = pChar->byDirection;
			pChar->dwX = (DWORD)pMaze->byStartX;
			pChar->dwY = (DWORD)pMaze->byStartY;
			pChar->dwXBack = pChar->dwX;
			pChar->dwYBack = pChar->dwY;
			SendCmdFADEINOUT (pChar->pID, 0);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			SendCmdCHARINFO (pChar->pID, -1);
			SendCmdFADEINOUT (pChar->pID, 1);

			SendCmdSYSMSG (pChar->pID, "SYSTEM:迷路作成の為強制移動しました。", SPEECHCMD_BATTLE1);
			i = -1;
			bFlg = FALSE;
		}
	}

	/* マップ設計図作成 */
	int cx, cy, xx, yy, nWidth;
	nWidth = GetMazePathWidth (pMaze);
	cx = (pMaze->byX2 - pMaze->byX1) / nWidth + 1;
	cy = (pMaze->byY2 - pMaze->byY1) / nWidth + 1;
	/* 作業位置(byTmpX/Y)が BYTE なので 255 マスまで */
	cx = (cx > 255) ? 255 : cx;
	cy = (cy > 255) ? 255 : cy;
	/* 狭すぎる範囲は作らない */
	if ((pMaze->byX2 <= pMaze->byX1) || (pMaze->byY2 <= pMaze->byY1) || (cx < 3) || (cy < 3)) {
		return;
	}
	xx = cx - 1;
	yy = cy - 1;

	pMaze->pMapTmp = new BYTE[cx * cy];
	ZeroMemory (pMaze->pMapTmp, cx * cy);
	PBYTE ptr = pMaze->pMapTmp;

	/* 立体交差あり？ */
	if (pMaze->byCrossRate && (pMaze->wChipIDBridgeV || pMaze->wChipIDBridgeH) && (cx >= 7) && (cy >= 7)) {
		MakeMazeWeave (pMaze->pMapTmp, cx, cy, pMaze);
		cy = 0;		/* 下の棒倒しはしない */
	}

	for (int y = 0; y < cy; y ++) {
		for (int x = 0; x < cx; x ++) {
			/* 外壁？ */
			if (y == 0 || y == yy || x == 0 || x == xx) {
				*ptr = 1;

			/* 基準点？ */
			} else if ((x % 2 == 0) && (y % 2 == 0)) {
				*ptr = 1;
				/* 上右下の３方向をランダムで選ぶ */
				switch (genrand () % 3) {
				case 0:
					if (*(ptr - cx) == 0) {
						*(ptr - cx) = 1;

					} else {
						x --;
						continue;
					}
					break;

				case 1:
					if (*(ptr + cx) == 0) {
						*(ptr + cx) = 1;

					} else {
						x --;
						continue;
					}
					break;

				case 2:
					if (*(ptr + 1) == 0) {
						*(ptr + 1) = 1;

					} else {
						x --;
						continue;
					}
					break;
				}
			}
			ptr ++;
		}
	}

	/* その他準備 */
	cy = (pMaze->byY2 - pMaze->byY1) / nWidth + 1;
	cy = (cy > 255) ? 255 : cy;
	pMaze->byWorkWidth = (BYTE)nWidth;
	pMaze->wWorkCX = (WORD)cx;
	pMaze->wWorkCY = (WORD)cy;
	pMaze->byTmpX = 0;
	pMaze->byTmpY = 0;
	pMaze->dwTimeCheck = 0;
	pMaze->dwTblIDCheck = 0;
}

/* ========================================================================= */
/* 関数名：MakeDungeonProc													 */
/* 内容：ダンジョン作成処理													 */
/* 日付：2004/02/28															 */
/* ========================================================================= */
void MakeDungeonProc(BYTE byID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[byID];
	PSUBGAME_DUNGEONINFO pDungeon = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
	PMAPINFO pMap = g_aMapInfo[pDungeon->dwMapID];

	/* 現在作成中？ */
	if (pDungeon->pMapTmp) {
		return;
	}

	SendCmdSYSMSGMAP (pDungeon->dwMapID, "SYSTEM:ダンジョン作成開始します。");

	/* スタート地点用イベント削除 */
	int nNo = SearchEvent (pDungeon->dwMapID, (DWORD)pDungeon->byStartX, (DWORD)pDungeon->byStartY);
	if (nNo >= 0) {
		DeleteEvent (pDungeon->dwMapID, nNo);
		pMap->pwMap[pDungeon->byStartX + pDungeon->byStartY * pMap->nCX] = 1;
		SendCmdCHGMAP (pMap->dwID, 1, pDungeon->byStartX, pDungeon->byStartY);
	}

	/* ゴール地点用イベント削除 */
	nNo = SearchEvent (pDungeon->dwMapID, (DWORD)pDungeon->byGoalX, (DWORD)pDungeon->byGoalY);
	if (nNo >= 0) {
		DeleteEvent (pDungeon->dwMapID, nNo);
	}

	/* 迷路内の人を追い出す。 */
	PDWORD pTbl = pMap->paTblNoPC;
	BOOL bFlg = FALSE;
	int i, j;
	for (i = 0; i < (int)pMap->dwCountTblNoPC; i ++) {
		if (pTbl[i] == -1) {
			if (bFlg == FALSE) {
				break;
			}
			i = -1;
			bFlg = FALSE;
			continue;
		}
		PCHARINFO pChar = g_aCharInfo[pTbl[i]];
		/* エリア内にいる？ */
		if ((pChar->dwMapID == pDungeon->dwMapID) &&
			(pChar->dwX >= (DWORD)pDungeon->byX1) && (pChar->dwX <= (DWORD)pDungeon->byX2) &&
			(pChar->dwY >= (DWORD)pDungeon->byY1) && (pChar->dwY <= (DWORD)pDungeon->byY2)) {
			/* 追い出す */
			bFlg = TRUE;
			svrSock.SendCancel (pChar->pID);
			CharTargetCancelPC (pChar);
			pChar->byDirection = 1;
			pChar->byDirectionBack2 = pChar->byDirection;
			pChar->byDirectionBack = pChar->byDirection;
			pChar->dwX = (DWORD)pDungeon->byStartX;
			pChar->dwY = (DWORD)pDungeon->byStartY;
			pChar->dwXBack = pChar->dwX;
			pChar->dwYBack = pChar->dwY;
			SendCmdFADEINOUT (pChar->pID, 0);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
			SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
			SendCmdCHARINFO (pChar->pID, -1);
			SendCmdFADEINOUT (pChar->pID, 1);

			SendCmdSYSMSG (pChar->pID, "SYSTEM:ダンジョン作成の為強制移動しました。", SPEECHCMD_BATTLE1);
			i = -1;
			bFlg = FALSE;
		}
	}

	/* マップ設計図作成 */
	int cx, cy, xx, yy,
		nTmpX[] = {1, 1, 0, 2},
		nTmpY[] = {0, 2, 1, 1};
	cx = (pDungeon->byX2 - pDungeon->byX1) / 2 + 1;
	cy = (pDungeon->byY2 - pDungeon->byY1) / 2 + 1;
	/* 狭すぎる範囲は作らない（部屋が入りきらない） */
	if ((pDungeon->byX2 <= pDungeon->byX1) || (pDungeon->byY2 <= pDungeon->byY1) || (cx < 12) || (cy < 12)) {
		return;
	}
	xx = cx - 1;
	yy = cy - 1;

	/* 乱数の初期化 */
	sgenrand (timeGetTime ());

	/* メモリ確保 */
	pDungeon->pMapTmp = new BYTE[cx * cy];
	ZeroMemory (pDungeon->pMapTmp, cx * cy);
	PBYTE ptr = pDungeon->pMapTmp;

	/* スタート地点を決定 */
	int sx = (int)((unsigned long)genrand () % (unsigned long)(cx - 6)) + 3;
	int sy = (int)((unsigned long)genrand () % (unsigned long)(cy - 6)) + 3;

	/* 最初の進行方向を決定 */
	int nDirection = (int)((unsigned long)genrand () % 4);
	ptr[sx + 1 + ((sy + 1) * cx)] = 1;

	/* 床を埋める */
	for (j = 0; j < 3; j ++) {
		for (i = 0; i < 3; i ++) {
			ptr[sx + i + ((sy + j) * cx)] = 1;
		}
	}
	/* 周りの壁を作成 */
	for (i = 0; i < 3; i ++) {
		ptr[sx + i + (sy * cx)] = 2;
		ptr[sx + i + ((sy + 2) * cx)] = 2;
		ptr[sx + ((sy + i) * cx)] = 2;
		ptr[sx + 2 + ((sy + i) * cx)] = 2;
	}
	ptr[sx + nTmpX[nDirection] + ((sy + nTmpY[nDirection]) * cx)] = 1;

	MakeDungeonProcTmp (ptr, sx, sy, nDirection, cx, cy);

	/* その他準備 */
	pDungeon->byTmpX = 0;
	pDungeon->byTmpY = 0;
	pDungeon->byGoalX = pDungeon->byStartX + (BYTE)((sx + 1) * 2);
	pDungeon->byGoalY = pDungeon->byStartY + (BYTE)((sy + 1) * 2) + 1;
	pDungeon->dwTimeCheck = 0;
	pDungeon->dwTblIDCheck = 0;
}

/* ========================================================================= */
/* 関数名：MakeDungeonProcTmp												 */
/* 内容：ダンジョン作成処理(メイン)											 */
/* 日付：2004/02/26															 */
/* ========================================================================= */
static BOOL MakeDungeonProcTmp(PBYTE pMaze, int x, int y, int nDirection, int cx, int cy)
{
	int i, j, nType = (int)((unsigned long)genrand () % 10);
	BOOL bRet = FALSE,
		 bCheck;

	int nTmpX[] = {0, 0, -1, 1},
		nTmpY[] = {-1, 1, 0, 0};

	switch (nType) {
	case 0:	/* 小部屋 */
	case 1:
		if (nDirection == 0 || nDirection == 2) {
			bCheck = CheckMakeDungeon (pMaze, x, y, nDirection, 5, cx, cy);
		} else if (nDirection == 1) {
			bCheck = CheckMakeDungeon (pMaze, x, y - 2, nDirection, 5, cx, cy);
		} else if (nDirection == 3) {
			bCheck = CheckMakeDungeon (pMaze, x - 2, y, nDirection, 5, cx, cy);
		}
		if (bCheck != TRUE) {
			switch (nDirection) {
			case 0:
				pMaze[x + 1 + ((y + 0) * cx)] = 2;
				break;
			case 1:
				pMaze[x + 1 + ((y + 2) * cx)] = 2;
				break;
			case 2:
				pMaze[x + 0 + ((y + 1) * cx)] = 2;
				break;
			case 3:
				pMaze[x + 2 + ((y + 1) * cx)] = 2;
				break;
			}
			return FALSE;
		}

		if (nDirection == 0 || nDirection == 2) {
			x += (nTmpX[nDirection] * 5);
			y += (nTmpY[nDirection] * 5);
		} else {
			x += (nTmpX[nDirection] * 3);
			y += (nTmpY[nDirection] * 3);
		}
		for (j = 0; j < 5; j ++) {
			for (i = 0; i < 5; i ++) {
				pMaze[x + i + ((y + j) * cx)] = 1;
			}
		}
		for (i = 0; i < 5; i ++) {
			pMaze[x + i + (y * cx)] = 2;
			pMaze[x + i + ((y + 4) * cx)] = 2;
			pMaze[x + ((y + i) * cx)] = 2;
			pMaze[x + 4 + ((y + i) * cx)] = 2;
		}
		switch (nDirection) {
		case 0:
			pMaze[x + 1 + ((y + 4) * cx)] = 3;
			break;
		case 1:
			pMaze[x + 1 + ((y + 0) * cx)] = 3;
			break;
		case 2:
			pMaze[x + 4 + ((y + 1) * cx)] = 3;
			break;
		case 3:
			pMaze[x + 0 + ((y + 1) * cx)] = 3;
			break;
		}
		bRet = FALSE;
		break;

	case 2:	/* 大部屋 */
		if (nDirection == 0 || nDirection == 2) {
			bCheck = CheckMakeDungeon (pMaze, x, y, nDirection, 9, cx, cy);
		} else if (nDirection == 1) {
			bCheck = CheckMakeDungeon (pMaze, x, y - 4, nDirection, 9, cx, cy);
		} else if (nDirection == 3) {
			bCheck = CheckMakeDungeon (pMaze, x - 4, y, nDirection, 9, cx, cy);
		}
		if (bCheck != TRUE) {
			switch (nDirection) {
			case 0:
				pMaze[x + 1 + ((y + 0) * cx)] = 2;
				break;
			case 1:
				pMaze[x + 1 + ((y + 2) * cx)] = 2;
				break;
			case 2:
				pMaze[x + 0 + ((y + 1) * cx)] = 2;
				break;
			case 3:
				pMaze[x + 2 + ((y + 1) * cx)] = 2;
				break;
			}
			return FALSE;
		}

		if (nDirection == 0 || nDirection == 2) {
			x += (nTmpX[nDirection] * 9);
			y += (nTmpY[nDirection] * 9);
		} else {
			x += (nTmpX[nDirection] * 3);
			y += (nTmpY[nDirection] * 3);
		}
		for (j = 0; j < 9; j ++) {
			for (i = 0; i < 9; i ++) {
				pMaze[x + i + ((y + j) * cx)] = 1;
			}
		}
		for (i = 0; i < 9; i ++) {
			pMaze[x + i + (y * cx)] = 2;
			pMaze[x + i + ((y + 8) * cx)] = 2;
			pMaze[x + ((y + i) * cx)] = 2;
			pMaze[x + 8 + ((y + i) * cx)] = 2;
		}
		switch (nDirection) {
		case 0:
			pMaze[x + 1 + ((y + 8) * cx)] = 3;
			break;
		case 1:
			pMaze[x + 1 + ((y + 0) * cx)] = 3;
			break;
		case 2:
			pMaze[x + 8 + ((y + 1) * cx)] = 3;
			break;
		case 3:
			pMaze[x + 0 + ((y + 1) * cx)] = 3;
			break;
		}
		bRet = FALSE;
		break;

	case 3:	/* T字路 */
	case 4:
	case 5:
	case 6:
	case 7:
		if (CheckMakeDungeon (pMaze, x, y, nDirection, 3, cx, cy) != TRUE) {
			switch (nDirection) {
			case 0:
				pMaze[x + 1 + ((y + 0) * cx)] = 2;
				break;
			case 1:
				pMaze[x + 1 + ((y + 2) * cx)] = 2;
				break;
			case 2:
				pMaze[x + 0 + ((y + 1) * cx)] = 2;
				break;
			case 3:
				pMaze[x + 2 + ((y + 1) * cx)] = 2;
				break;
			}
			return FALSE;
		}
		x += (nTmpX[nDirection] * 3);
		y += (nTmpY[nDirection] * 3);
		for (j = 0; j < 3; j ++) {
			for (i = 0; i < 3; i ++) {
				pMaze[x + i + ((y + j) * cx)] = 1;
			}
		}
		pMaze[x + 0 + ((y + 0) * cx)] = 2;
		pMaze[x + 1 + ((y + 0) * cx)] = 2;
		pMaze[x + 2 + ((y + 0) * cx)] = 2;
		pMaze[x + 0 + ((y + 2) * cx)] = 2;
		pMaze[x + 2 + ((y + 2) * cx)] = 2;
		switch (nDirection) {
		case 0:
			MakeDungeonProcTmp (pMaze, x, y, 2, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 3, cx, cy);
			break;
		case 1:
			pMaze[x + 1 + ((y + 0) * cx)] = 1;
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 2, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 3, cx, cy);
			break;
		case 2:
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 2, cx, cy);
			break;
		case 3:
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 3, cx, cy);
			break;
		}
		break;

	case 8:	/* 直進 */
	case 9:
		if (CheckMakeDungeon (pMaze, x, y, nDirection, 3, cx, cy) != TRUE) {
			switch (nDirection) {
			case 0:
				pMaze[x + 1 + ((y + 0) * cx)] = 2;
				break;
			case 1:
				pMaze[x + 1 + ((y + 2) * cx)] = 2;
				break;
			case 2:
				pMaze[x + 0 + ((y + 1) * cx)] = 2;
				break;
			case 3:
				pMaze[x + 2 + ((y + 1) * cx)] = 2;
				break;
			}
			return FALSE;
		}
		x += (nTmpX[nDirection] * 3);
		y += (nTmpY[nDirection] * 3);
		for (j = 0; j < 3; j ++) {
			for (i = 0; i < 3; i ++) {
				pMaze[x + i + ((y + j) * cx)] = 1;
			}
		}
		pMaze[x + 0 + ((y + 0) * cx)] = 2;
		pMaze[x + 0 + ((y + 1) * cx)] = 2;
		pMaze[x + 0 + ((y + 2) * cx)] = 2;
		pMaze[x + 2 + ((y + 0) * cx)] = 2;
		pMaze[x + 2 + ((y + 1) * cx)] = 2;
		pMaze[x + 2 + ((y + 2) * cx)] = 2;
		switch (nDirection) {
		case 0:
			MakeDungeonProcTmp (pMaze, x, y, 0, cx, cy);
			break;
		case 1:
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			break;
		case 2:
			pMaze[x + 2 + ((y + 1) * cx)] = 1;
			MakeDungeonProcTmp (pMaze, x, y, 0, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			break;
		case 3:
			pMaze[x + 0 + ((y + 1) * cx)] = 1;
			MakeDungeonProcTmp (pMaze, x, y, 0, cx, cy);
			MakeDungeonProcTmp (pMaze, x, y, 1, cx, cy);
			break;
		}
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckMakeDungeon													 */
/* 内容：進行方向のチェック													 */
/* 日付：2004/02/26															 */
/* ========================================================================= */
static BOOL CheckMakeDungeon(PBYTE pMaze, int x, int y, int nDirection, int nSize, int cx, int cy)
{
	BOOL bRet = TRUE;
	int i, j;

	switch (nDirection) {
	case 0:
		y -= nSize;
		break;
	case 1:
		y += nSize;
		break;
	case 2:
		x -= nSize;
		break;
	case 3:
		x += nSize;
		break;
	}

	if (x < 0 || y < 0 || x + nSize - 1 >= cx || y + nSize - 1 >= cy) {
		bRet = FALSE;

	} else {
		for (j = 0; j < nSize; j ++) {
			for (i = 0; i < nSize; i ++) {
				if (pMaze[i + x + ((j + y) * cx)] != 0) {
					bRet = FALSE;
					j = nSize;
					break;
				}
			}
		}
	}
	return bRet;
}

/* Copyright(C)URARA-works 2003-2005 */
