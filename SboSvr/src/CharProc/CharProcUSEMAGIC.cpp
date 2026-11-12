/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcUSEMAGIC.cpp										 */
/* 内容：		キャラ処理(重なったキャラに魔法使用)クラス 実装ファイル		 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrMagicProc.h"
#include "SboSvrRecvCmd.h"
#include "CharProcUSEMAGIC.h"

/* ========================================================================= */
/* 関数名：	UseMagicHere													 */
/* 内容：	今いるマスに重なったキャラへ魔法を使う(使えたら TRUE)			 */
/* ========================================================================= */

static BOOL UseMagicHere(PCHARINFO pChar, BYTE byMagicID)
{
	DWORD dwTblID;

	dwTblID = CheckCharBlockPC (pChar->dwMapID, pChar->dwX, pChar->dwY, pChar->dwSpaceID);
	/* プレイヤーと重なってない？ */
	if (dwTblID == -1) {
		dwTblID = CheckCharBlockNPC2 (pChar->dwTblID, pChar->dwMapID, pChar->dwX, pChar->dwY, pChar->dwSpaceID);
		if (dwTblID == -1) {
			return FALSE;
		}
	}
	if (pChar->dwTblID == dwTblID) {
		return FALSE;
	}

	/* 魔法発動 */
	return MagicProc (pChar->dwTblID, byMagicID, dwTblID);
}

/* ========================================================================= */
/* 関数名：	CCharProcUSEMAGIC::CCharProcUSEMAGIC							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcUSEMAGIC::CCharProcUSEMAGIC()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcUSEMAGIC::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcUSEMAGIC::Proc(DWORD dwTime)
{
	BYTE byNo;
	BOOL bRet, bResult;
	PMAGICINFO pInfo;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 250) {
		goto Exit;
	}

	/* どの魔法使うか選択 */
	byNo = (BYTE)(genrand () % MAX_USEMAGIC);
	if (m_pChar->abyMagic[byNo] == 0) {
		goto Exit;
	}

	pInfo = g_aMagicInfo[byNo];
	switch (pInfo->byType) {
	case MAGICTYPE_RETURN:			/* 帰還 */
	case MAGICTYPE_PUT:				/* 魔法発生 */
		break;

	default:
		if (abs (g_dwTime - m_pChar->dwLastDotMove) < 500) {
			goto Exit;
		}
		break;
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
	if (abs (m_pChar->dwTimeLogin - m_pChar->dwLastDotMove) > (int)(m_pChar->dwTimeDelete * 1000)) {
		DeleteChar (m_pChar->dwTblID);
		goto Exit;
	}

	/* 一人に当たれば消える飛ぶ魔法は、動く前に今いるマスで当たるか確かめる。
	   (出た所が目の前の敵のマスだと、先に動いてすり抜けてしまい当たらなかった) */
	if (m_pChar->wMaxHP && (m_pChar->byMoveProbabilityX || m_pChar->byMoveProbabilityY)) {
		if (UseMagicHere (m_pChar, m_pChar->abyMagic[byNo])) {
			/* 次で消滅 */
			m_pChar->dwTimeDelete = 0;
			m_pChar->dwTimeLogin = 0;
			m_pChar->dwLastDotMove = 0;
			bRet = TRUE;
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
			if (CheckCharBlock (m_pChar, m_pChar->dwMapID, nX, nY) == FALSE) {
				/* 次で消滅 */
				m_pChar->dwTimeDelete = 0;
				m_pChar->dwTimeLogin = 0;
				m_pChar->dwLastDotMove = 0;

			} else {
				m_pChar->wHP --;
				if (m_pChar->wHP == 0) {
					/* 次で消滅 */
					m_pChar->dwTimeDelete = 0;
					m_pChar->dwTimeLogin = 0;
					m_pChar->dwLastDotMove = 0;
				}
				m_pChar->dwX = (DWORD)nX;
				m_pChar->dwY = (DWORD)nY;
				m_pChar->byFlg |= CHARINFOFLG_MOVE;
			}
		}
	}

	/* 重なったキャラに魔法発動 */
	bResult = UseMagicHere (m_pChar, m_pChar->abyMagic[byNo]);

	/* 対象は一人？ */
	if (bResult && m_pChar->wMaxHP) {
		/* 次で消滅 */
		m_pChar->dwTimeDelete = 0;
		m_pChar->dwTimeLogin = 0;
		m_pChar->dwLastDotMove = 0;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
