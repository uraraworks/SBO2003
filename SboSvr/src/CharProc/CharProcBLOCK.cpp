/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBLOCK.cpp											 */
/* 内容：		キャラ処理(押せる)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcBLOCK.h"

/* ========================================================================= */
/* 関数名：	CCharProcBLOCK::CCharProcBLOCK									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcBLOCK::CCharProcBLOCK()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcBLOCK::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcBLOCK::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 500) {
		goto Exit;
	}

	if ((m_pChar->byFlg & CHARINFOFLG_MOVE) == 0) {
		bResult = PartsMoveProc (m_pChar);
		if (bResult) {
			m_pChar->dwLastDotMove = timeGetTime ();
		}
	}

	MoveStartPosProc (m_pChar);
	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
