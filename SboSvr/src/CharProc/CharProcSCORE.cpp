/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSCORE.cpp											 */
/* 内容：		キャラ処理(得点板)クラス 実装ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/18													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrMagicProc.h"
#include "CharProcSCORE.h"

/* ========================================================================= */
/* 関数名：	CCharProcSCORE::CCharProcSCORE									 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/01/18														 */
/* ========================================================================= */

CCharProcSCORE::CCharProcSCORE()
{
}


/* ========================================================================= */
/* 関数名：	CCharProcSCORE::Proc											 */
/* 内容：	処理															 */
/* 日付：	2006/01/18														 */
/* ========================================================================= */

BOOL CCharProcSCORE::Proc(DWORD dwTime)
{
	BOOL bRet;
	PMAPINFO pMap;

	bRet = FALSE;
	pMap = g_aMapInfo[m_pChar->dwMapID];

	/* 誰かいる？ */
	if (pMap->dwCountTblNoPC > 0) {
		goto Exit;
	}

	/* 初期化 */
	m_pChar->byEfect	= 0;
	m_pChar->clName		= RGB (255, 255, 255);
	m_pChar->clSpeech	= RGB (255, 255, 255);
	ZeroMemory (m_pChar->szName, sizeof (m_pChar->szName));

	bRet = TRUE;
Exit:
	return bRet;
}

/* Copyright(C)URARA-works 2006 */
