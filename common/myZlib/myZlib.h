/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	myZlib.h													 */
/* 内容：		zlibを扱う為のクラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/02/02													 */
/* ========================================================================= */

#ifndef MYZLIB_H
#define MYZLIB_H

#include "zlib.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

class CmyZlib
{
public:
			CmyZlib();										/* コンストラクタ */
	virtual ~CmyZlib();										/* デストラクタ */

	/* 圧縮 */
	BOOL	DeflateInit	(int nLevel = Z_DEFAULT_COMPRESSION);							/* 初期化 */
	DWORD	Deflate		(LPBYTE pSrc, DWORD dwSrcSize, LPBYTE pDst, DWORD dwDstSize);	/* 圧縮処理 */
	void	DeflateEnd	(void);															/* 後処理 */

	/* 解凍 */
	BOOL	InflateInit	(void);															/* 初期化 */
	DWORD	Inflate		(LPBYTE pSrc, DWORD dwSrcSize, LPBYTE pDst, DWORD dwDstSize);	/* 解凍処理 */
	void	InflateEnd	(void);															/* 後処理 */


private:
	z_stream		*m_pStream;									/* 圧縮・解凍用構造体 */
};

#endif
/* Copyright(C)URARA-works 2005 */
