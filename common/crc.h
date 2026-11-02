/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	crc.h														 */
/* 内容：		CRC計算クラス 定義ファイル									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/03/29													 */
/* ========================================================================= */

#ifndef CCRC_H
#define CCRC_H

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */
class CCRC
{
public:
	CCRC();													/* コンストラクタ */
	virtual ~CCRC();										/* デストラクタ */

	DWORD Update(DWORD dwSrc, PBYTE pBuf, DWORD dwSize);	/* CRCを更新 */
	DWORD GetCRC(PBYTE pBuf, DWORD dwSize);					/* CRCを取得 */


private:
	void MakeTbl(void);									/* 計算用テーブルを作成 */


private:
	PDWORD		m_padwCRC;								/* 計算用テーブル */
};

#endif
/* Copyright(C)URARA-works 2005 */
