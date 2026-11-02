/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	PacketBase.h												 */
/* 内容：		コマンド基底クラス 定義ファイル								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/08/01													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CPacketBase
{
public:
			CPacketBase();								/* コンストラクタ */
	virtual ~CPacketBase();								/* デストラクタ */

	DWORD	GetSize		(void);									/* パケットサイズを取得 */
	PBYTE	Get			(void);									/* パケットを取得 */


protected:
	void	RenewPacket		(PBYTE pPacket, DWORD dwSize);		/* パケットを更新 */
	void	CopyMemoryRenew	(PVOID pDst, CONST PVOID pSrc, DWORD dwSize, PBYTE &pPos);	/* メモリコピーしてポインタを進める */
	void	strcpyRenew		(LPSTR pszDst, LPCSTR pszSrc, PBYTE &pPos);					/* 文字列コピーしてポインタを進める */

public:
	PBYTE	m_pPacket;							/* パケット */
	DWORD	m_dwPacketSize;						/* パケットサイズ */
} CPacketBase, *PCPacketBase;

/* Copyright(C)URARA-works 2006 */
