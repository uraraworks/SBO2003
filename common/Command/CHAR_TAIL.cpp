/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CHAR_TAIL.cpp												 */
/* 内容：		コマンド(キャラ系:ついていく要求) 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/08/01													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "CHAR_TAIL.h"

/* ========================================================================= */
/* 関数名：	CCHAR_TAIL::CCHAR_TAIL											 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

CCHAR_TAIL::CCHAR_TAIL()
{
	m_dwTblID		= -1;
	m_dwTailTblID	= -1;
}


/* ========================================================================= */
/* 関数名：	CCHAR_TAIL::~CCHAR_TAIL											 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

CCHAR_TAIL::~CCHAR_TAIL()
{
}


/* ========================================================================= */
/* 関数名：	CCHAR_TAIL::Make												 */
/* 内容：	パケットを作成													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

void CCHAR_TAIL::Make(
	DWORD dwTblID,				/* [in] 対象キャラのテーブルID */
	DWORD dwTblIDTarget,		/* [in] ついていこうとしているキャラのテーブルID */
	DWORD dwResult)				/* [in] 結果 */
{
	PBYTE pData, pDataTmp;
	DWORD dwSize;
	PPACKETBASE pPacketBase;

	dwSize = sizeof (PACKETBASE)	+
			 sizeof (dwTblID)		+
			 sizeof (dwTblIDTarget)	+
			 sizeof (dwResult);

	pData = new BYTE[dwSize];
	ZeroMemory (pData, dwSize);
	pPacketBase = (PPACKETBASE)pData;

	pPacketBase->byCmdMain	= CMD_MAIN_CHAR;
	pPacketBase->byCmdSub	= CMDCHAR_TAIL;

	pDataTmp = (PBYTE)(pPacketBase + 1);
	CopyMemoryRenew (pDataTmp, &dwTblID,		sizeof (dwTblID),		pDataTmp);
	CopyMemoryRenew (pDataTmp, &dwTblIDTarget,	sizeof (dwTblIDTarget),	pDataTmp);
	CopyMemoryRenew (pDataTmp, &dwResult,		sizeof (dwResult),		pDataTmp);

	RenewPacket (pData, dwSize);
}


/* ========================================================================= */
/* 関数名：	CCHAR_TAIL::Set													 */
/* 内容：	パケットを設定													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

void CCHAR_TAIL::Set(PBYTE pPacket)
{
	PBYTE pDataTmp;

	pDataTmp = pPacket;

	pDataTmp += sizeof (PACKETBASE);
	CopyMemoryRenew (&m_dwTblID,		pDataTmp, sizeof (m_dwTblID),		pDataTmp);
	CopyMemoryRenew (&m_dwTailTblID,	pDataTmp, sizeof (m_dwTailTblID),	pDataTmp);
	CopyMemoryRenew (&m_dwResult,		pDataTmp, sizeof (m_dwResult),		pDataTmp);
}

/* Copyright(C)URARA-works 2006 */
