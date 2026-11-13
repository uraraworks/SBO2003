/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcATACK.cpp											 */
/* 内容：		キャラ処理(重なったキャラに攻撃)クラス 実装ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "common/BowFlight.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrMagicProc.h"
#include "CharProcATACK.h"

/* ========================================================================= */
/* 関数名：	CCharProcATACK::CCharProcATACK									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcATACK::CCharProcATACK()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcATACK::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcATACK::Proc(DWORD dwTime)
{
	DWORD dwTblID;
	BOOL bRet, bResult;
	PCHARINFO pCharTarget, pCharParent;

	bRet = FALSE;
    if (m_pChar->dwBowDuration) return ProcBow(dwTime);
	/* 1 マス進む間隔(ms)。投げた雪玉は、速くなった歩き（1 マス約 280ms）に追いつかれないよう速くする */
	DWORD dwInterval = 200;
//Todo:画像ID設定
	if (m_pChar->wGrpID == 213) {
		dwInterval = 180;
	}
	if (abs (dwTime - m_pChar->dwLastDotMove) < (int)dwInterval) {
		goto Exit;
	}

	m_pChar->dwLastDotMove = timeGetTime ();
	if (m_pChar->dwParentTblID != -1) {
		PCHARINFO pCharParent;

		pCharParent = g_aCharInfo[m_pChar->dwParentTblID];
		/* 発動者と別マップ・別の空間？ */
		if ((m_pChar->dwMapID != pCharParent->dwMapID) || !IsSameSpace (m_pChar, pCharParent)) {
			/* 消滅 */
			m_pChar->dwTimeDelete = 1;
		}
	}

	/* 予定時間が経過した？ */
	if (m_pChar->dwTimeDelete) {
		if (abs (m_pChar->dwTimeLogin - m_pChar->dwLastDotMove) > (int)(m_pChar->dwTimeDelete * 1000)) {
			DeleteChar (m_pChar->dwTblID);
			goto Exit;
		}
	}

	/* 移動確率が設定されている？ */
	if (m_pChar->byMoveProbabilityX || m_pChar->byMoveProbabilityY) {
		int nTmp = genrand () % 100, nTmpCheck;

		if (m_pChar->byDirection >= 2) {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityX;
		} else {
			nTmpCheck = 100 - m_pChar->byMoveProbabilityY;
		}
		/* 移動決定？ */
		if (nTmp >= nTmpCheck) {
			int nX, nY;

			nX = m_pChar->dwX + anPosAddTblX[m_pChar->byDirection];
			nY = m_pChar->dwY + anPosAddTblY[m_pChar->byDirection];

			/* 障害物にぶるかる？ */
			if ((CheckCharBlock (m_pChar, m_pChar->dwMapID, nX, nY) == FALSE) &&
				(CheckCharBlock (m_pChar->dwMapID, nX, nY) || IsMagicWall (m_pChar->dwMapID, nX, nY))) {
				/* 次で消滅 */
				m_pChar->dwTimeDelete = 1;
				m_pChar->dwTimeLogin = 0;
				m_pChar->dwLastDotMove = 0;

			} else {
				m_pChar->wHP --;
				if (m_pChar->wHP == 0) {
					/* 次で消滅 */
					m_pChar->dwTimeDelete = 1;
					m_pChar->dwTimeLogin = 0;
					m_pChar->dwLastDotMove = 0;
				}
				m_pChar->dwX = (DWORD)nX;
				m_pChar->dwY = (DWORD)nY;
				m_pChar->byFlg |= CHARINFOFLG_MOVE;
			}
		}
	}

	return HitCurrentCell();
Exit:
	return bRet;
}

BOOL CCharProcATACK::HitCurrentCell(void)
{
	DWORD dwTblID;
	BOOL bRet = FALSE, bResult;
	PCHARINFO pCharTarget, pCharParent;
	dwTblID = CheckCharBlockPC (m_pChar->dwMapID, m_pChar->dwX, m_pChar->dwY, m_pChar->dwSpaceID);
	/* プレイヤーと重なってない？ */
	if (dwTblID == -1) {
		dwTblID = CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, m_pChar->dwX, m_pChar->dwY, m_pChar->dwSpaceID);
		if (dwTblID == -1) {
			goto Exit;
		}
	}

	if ((m_pChar->dwParentTblID == dwTblID) || (m_pChar->dwTblID == dwTblID)) {
		goto Exit;
	}

	pCharTarget = g_aCharInfo[dwTblID];

	/* ログオフ中？ */
	if (pCharTarget->byGameState == GAMESTATE_LOGOFF) {
		m_pChar->dwLastBattle = 0;
		m_pChar->dwTargetTblID = -1;
		goto Exit;
	}

	/* 生きていない？ */
	if (pCharTarget->wHP == 0) {
		m_pChar->dwLastBattle = 0;
		m_pChar->dwTargetTblID = -1;
		goto Exit;
	}

//Todo:画像ID設定
	/* 雪？ */
	if (m_pChar->wGrpID == 213) {
		BOOL bTmp;
		switch (pCharTarget->byNpcType) {
		case NPCTYPE_ATACK:			/* 重なったキャラに攻撃 */
			if (pCharTarget->wGrpID != 213) {
				goto Exit;
			}
			/* 次で消滅 */
			pCharTarget->dwTimeDelete = 1;
			pCharTarget->dwTimeLogin = 0;
			pCharTarget->dwLastDotMove = 0;
			break;
		case NPCTYPE_INIT:
		case NPCTYPE_MOVERAND:	/* ランダム移動？ */
		case NPCTYPE_SEARCH:	/* 検索してついていく */
		case NPCTYPE_NOMOVE:	/* 移動しない */
		case NPCTYPE_BATTLE1:	/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
		case NPCTYPE_BATTLE2:	/* 周り２キャラの範囲で検索してついていって攻撃 */
			break;
		default:
			goto Exit;
		}
		bTmp = FALSE;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, 77);
		/* 次で消滅 */
		m_pChar->dwTimeDelete = 1;
		m_pChar->dwTimeLogin = 0;
		m_pChar->dwLastDotMove = 0;
		if (pCharTarget->dwTargetTblID != -1) {
			PCHARINFO pCharTmp;

			pCharTmp = g_aCharInfo[pCharTarget->dwTargetTblID];
			/* 得点板？ */
			if (pCharTmp->byNpcType == NPCTYPE_SCORE) {
				if (pCharTmp->byEfect < 5) {
					pCharTmp->byEfect ++;
					SendCmdEFECTCHG (pCharTmp->pID, pCharTmp->dwTblID, pCharTmp->byEfect);
				}
				if (pCharTmp->byEfect == 5) {
					bTmp = TRUE;
				}
			}
		}
		if (bTmp == FALSE) {
			SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 255, 255), "HIT!");
		} else {
			SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 0, 0), "OUT!");
		}
		goto Exit;
	}

	pCharParent = g_aCharInfo[m_pChar->dwParentTblID];
	if (pCharParent->byNpcType) {
		if (pCharTarget->byNpcType != NPCTYPE_INIT) {
			goto Exit;
		}
	} else {
		switch (pCharTarget->byNpcType) {
		case NPCTYPE_BATTLE1:	/* ランダム移動、最初に攻撃してきたキャラをターゲットに。追いかけない。 */
		case NPCTYPE_BATTLE2:	/* 周り２キャラの範囲で検索してついていって攻撃 */
		case NPCTYPE_MIMIC:		/* 宝箱の敵（正体前。当たると正体を現す） */
			break;
		default:
			goto Exit;
		}
	}

	m_pChar->dwTargetTblID = dwTblID;
	bResult = BattleProcAtack (m_pChar, g_aCharInfo[dwTblID]);

	/* 対象は一人？ */
//	if (bResult && m_pChar->wMaxHP) {
		/* 次で消滅 */
		m_pChar->dwTimeDelete = 1;
		m_pChar->dwTimeLogin = 0;
		m_pChar->dwLastDotMove = 0;
//	}
	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */

/* 1ピクセル以下に分割して通過マスを既存の接触判定へ渡す。発射後は追尾しない。 */
BOOL CCharProcATACK::ProcBow(DWORD now)
{
    PCHARINFO c = m_pChar;
    if (c->dwParentTblID >= (DWORD)g_aCharInfo.GetSize() ||
        g_aCharInfo[c->dwParentTblID]->pID != c->dwBowOwnerPID ||
        c->dwMapID != g_aCharInfo[c->dwParentTblID]->dwMapID ||
        !IsSameSpace(c, g_aCharInfo[c->dwParentTblID])) {
        DeleteChar(c->dwTblID); return FALSE;
    }
    DWORD elapsed = min(now - c->dwBowStartTime, c->dwBowDuration);
    int dx = c->nBowEndX - c->nBowStartX, dy = c->nBowEndY - c->nBowStartY;
    int pixels = max(abs(dx), abs(dy));
    int from = (int)((double)pixels * c->dwBowElapsed / c->dwBowDuration);
    int to = (int)((double)pixels * elapsed / c->dwBowDuration);
    for (int i = from + 1; i <= to; ++i) {
        int x = (int)floor((c->nBowStartX + (double)dx * i / pixels + 16) / 32);
        int y = (int)floor((c->nBowStartY + (double)dy * i / pixels + 16) / 32);
        if (x == (int)c->dwX && y == (int)c->dwY) continue;
        /* 斜めに角を横切る時も壁を抜けない。カウンターは既存と同様に通す。 */
        BOOL blocked = IsMagicWall(c->dwMapID, x, y) ||
            (!CheckCharBlock(c, c->dwMapID, x, y) && CheckCharBlock(c->dwMapID, x, y));
        if (x != (int)c->dwX && y != (int)c->dwY) {
            blocked |= IsMagicWall(c->dwMapID, x, c->dwY) || IsMagicWall(c->dwMapID, c->dwX, y);
        }
        c->dwBowElapsed = (DWORD)((double)c->dwBowDuration * i / pixels);
        if (blocked) { DeleteChar(c->dwTblID); return FALSE; }
        c->dwX = x; c->dwY = y;
        c->byFlg |= CHARINFOFLG_MOVE;
        if (HitCurrentCell()) { DeleteChar(c->dwTblID); return TRUE; }
    }
    c->dwBowElapsed = elapsed;
    if (elapsed >= c->dwBowDuration) { DeleteChar(c->dwTblID); return FALSE; }
    return FALSE;
}
