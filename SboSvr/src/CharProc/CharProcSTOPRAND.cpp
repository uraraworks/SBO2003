/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSTOPRAND.cpp										 */
/* 内容：		キャラ処理(止まるかランダム)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/16													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "CharProcSTOPRAND.h"

/* ========================================================================= */
/* 関数名：	CCharProcSTOPRAND::CCharProcSTOPRAND							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

CCharProcSTOPRAND::CCharProcSTOPRAND()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcSTOPRAND::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/16														 */
/* ========================================================================= */

BOOL CCharProcSTOPRAND::Proc(DWORD dwTime)
{
	BOOL bRet;
	DWORD dwTmp;
	char szTmp[2];

	bRet	= FALSE;
	dwTmp	= dwTime - m_dwLastTimeProc;

	if (dwTmp < 150) {
		goto Exit;
	}
	m_dwLastTimeProc		= dwTime;
	m_pChar->dwLastDotMove	= dwTime;

	/* 停止状態？ */
	if ((m_pChar->byEfect & 0x80) == 0) {
		goto Exit;
	}

	if (m_pChar->pszNpcSpeech && strlen (m_pChar->pszNpcSpeech)) {
		/* 指定最大値を取得 */
		szTmp[0] = m_pChar->pszNpcSpeech[0];
		szTmp[1] = 0;
		if (atoi (szTmp) == 0) {
			szTmp[0] = '8';
		}
		m_pChar->byEfect = (BYTE)(genrand () % atoi (szTmp));

	} else {
		m_pChar->byEfect = (BYTE)(genrand () % 8);
	}
	m_pChar->byEfect |= 0x80;
	SendCmdEFECTCHG (m_pChar->pID, m_pChar->dwTblID, m_pChar->byEfect);

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
