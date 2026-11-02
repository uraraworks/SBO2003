/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcNOMOVE.cpp											 */
/* 内容：		キャラ処理(移動しない)クラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcNOMOVE.h"

/* ========================================================================= */
/* 関数名：	CCharProcNOMOVE::CCharProcNOMOVE								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

CCharProcNOMOVE::CCharProcNOMOVE()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcNOMOVE::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/17														 */
/* ========================================================================= */

BOOL CCharProcNOMOVE::Proc(DWORD dwTime)
{
	BOOL bRet, bResult;

	bRet = FALSE;
	if (abs (dwTime - m_pChar->dwLastDotMove) < 500) {
		goto Exit;
	}

	bResult = PartsMoveProc (m_pChar);

	if (bResult) {
		m_pChar->dwLastDotMove = timeGetTime ();
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
