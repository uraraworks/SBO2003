/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	MacAddr.h													 */
/* 内容：		MACアドレス取得クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/04/12													 */
/* ========================================================================= */

#ifndef CMACADDR_H
#define CMACADDR_H

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CMacAddr
{
public:
			CMacAddr();										/* コンストラクタ */
	virtual ~CMacAddr();									/* デストラクタ */

	BOOL	Get				(PBYTE pDst, int nNo = 0);				/* MACアドレスを取得 */
	BOOL	GetStr			(LPSTR pszDst, int nNo = 0);			/* MACアドレスを取得(文字列) */
	int		GetCount		(void);									/* NIC数を取得 */
	BOOL	GetDeviceName	(LPSTR pszDst, int nNo = 0);			/* デバイス名を取得 */


protected:
	int		GetNICCount	(void);								/* NIC数を取得 */


protected:
	int		m_nCount;										/* NIC数 */
} CMacAddr, *PCMacAddr;

#endif
/* Copyright(C)URARA-works 2005 */
