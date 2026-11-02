/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CHAR_STATEINFO.cpp											 */
/* 内容：		コマンド(キャラ系:状態通知) 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/07/25													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "CHAR_STATEINFO.h"

/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::CCHAR_STATEINFO								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

CCHAR_STATEINFO::CCHAR_STATEINFO()
{
	m_pPacket		= NULL;
	m_dwPacketSize	= 0;
	m_dwTblID		= -1;
	m_dwTailTblID	= -1;
}


/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::~CCHAR_STATEINFO								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

CCHAR_STATEINFO::~CCHAR_STATEINFO()
{
	SAFE_DELETE_ARRAY (m_pPacket);
}


/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::Make											 */
/* 内容：	パケットを作成													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

void CCHAR_STATEINFO::Make(PCHARINFO pChar)
{
	PCMDPARA_CHAR_STATEINFO pSendData;

	m_dwPacketSize	= sizeof (CMDPARA_CHAR_STATEINFO);
	m_pPacket		= new BYTE[m_dwPacketSize];
	ZeroMemory (m_pPacket, m_dwPacketSize);

	pSendData = (PCMDPARA_CHAR_STATEINFO)m_pPacket;

	pSendData->byCmdMain	= CMD_MAIN_CHAR;
	pSendData->byCmdSub		= CMDCHAR_STATEINFO;
	pSendData->dwTblID		= pChar->dwTblID;
	pSendData->dwTailTblID	= pChar->dwTailTblID;
}


/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::Set											 */
/* 内容：	パケットを設定													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

void CCHAR_STATEINFO::Set(PBYTE pPacket)
{
	PCMDPARA_CHAR_STATEINFO pRecvData;

	pRecvData = (PCMDPARA_CHAR_STATEINFO)pPacket;

	m_dwTblID		= pRecvData->dwTblID;
	m_dwTailTblID	= pRecvData->dwTailTblID;
}


/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::GetSize										 */
/* 内容：	パケットサイズを取得											 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

DWORD CCHAR_STATEINFO::GetSize(void)
{
	return m_dwPacketSize;
}


/* ========================================================================= */
/* 関数名：	CCHAR_STATEINFO::Get											 */
/* 内容：	パケットを取得													 */
/* 日付：	2006/07/25														 */
/* ========================================================================= */

PBYTE CCHAR_STATEINFO::Get(void)
{
	return m_pPacket;
}

/* Copyright(C)URARA-works 2006 */
