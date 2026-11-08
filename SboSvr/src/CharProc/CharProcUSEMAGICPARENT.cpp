/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcUSEMAGICPARENT.cpp									 */
/* 内容：		キャラ処理(魔法発動し続ける)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrMagicProc.h"
#include "CharProcUSEMAGICPARENT.h"

/* ========================================================================= */
/* 関数名：	CCharProcUSEMAGICPARENT::CCharProcUSEMAGICPARENT				 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcUSEMAGICPARENT::CCharProcUSEMAGICPARENT()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcUSEMAGICPARENT::Proc									 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcUSEMAGICPARENT::Proc(DWORD dwTime)
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
	case MAGICTYPE_PUTSELF:			/* 魔法発生(自分) */
		break;

	default:
		if (abs (g_dwTime - m_pChar->dwLastDotMove) < 500) {
			goto Exit;
		}
		break;
	}

	m_pChar->dwLastDotMove = timeGetTime ();

	/* 予定時間が経過した？ */
	if (abs (m_pChar->dwTimeLogin - m_pChar->dwLastDotMove) > (int)(m_pChar->dwTimeDelete * 1000)) {
		DeleteChar (m_pChar->dwTblID);
		goto Exit;
	}

	/* 一人に当たれば消える飛ぶ魔法は、動く前に今いるマスで当たるか確かめる。
	   (出た所が目の前の敵のマスだと、先に動いてすり抜けてしまい当たらなかった) */
	if (m_pChar->wMaxHP && (m_pChar->byMoveProbabilityX || m_pChar->byMoveProbabilityY)) {
		if (MagicProc (m_pChar->dwTblID, m_pChar->abyMagic[byNo], m_pChar->dwParentTblID)) {
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

	/* 魔法発動 */
	bResult = MagicProc (m_pChar->dwTblID, m_pChar->abyMagic[byNo], m_pChar->dwParentTblID);

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
