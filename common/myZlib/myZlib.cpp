/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	myZlib.cpp													 */
/* 内容：		zlibを扱う為のクラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/02/02													 */
/* ========================================================================= */

#include <windows.h>
#include "myZlib.h"

#if 0	/* 以下を任意のファイルにコピーして使用すると便利 */
/* libファイルのパス */
#define LIBCMNPATH	""

/* コンパイルオプション別のリンク設定 */
#ifdef _DEBUG
	#pragma comment(lib, LIBCMNPATH "zlibD.lib")
#else
	#pragma comment(lib, LIBCMNPATH "zlib.lib")
#endif
#endif

/* ========================================================================= */
/* 関数名：	CmyZlib::CmyZlib												 */
/* 内容：	コンストラクタ													 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

CmyZlib::CmyZlib()
{
	m_pStream = NULL;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::~CmyZlib												 */
/* 内容：	デストラクタ													 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

CmyZlib::~CmyZlib()
{
	if (m_pStream) {
		delete m_pStream;
		m_pStream = NULL;
	}
}

/* ========================================================================= */
/* 関数名：	CmyZlib::DeflateInit											 */
/* 内容：	圧縮:初期化														 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

BOOL CmyZlib::DeflateInit(
	int nLevel)		/* [in] 圧縮レベル */
{
	BOOL bRet;
	int nResult;

	bRet = FALSE;

	if (m_pStream) {
		goto Exit;
	}

	/* 情報構造体を確保して初期化 */
	m_pStream = new z_stream;
	m_pStream->zalloc	= Z_NULL;
	m_pStream->zfree	= Z_NULL;
	m_pStream->opaque	= Z_NULL;

	/* zlibの初期化関数を呼ぶ */
	nResult = deflateInit (m_pStream, nLevel);
	if (nResult != Z_OK) {
		delete m_pStream;
		m_pStream = NULL;
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::Deflate												 */
/* 内容：	圧縮:圧縮処理													 */
/* 日付：	2005/02/02														 */
/* 戻り値：	圧縮後のサイズ													 */
/* ========================================================================= */

DWORD CmyZlib::Deflate(
	LPBYTE pSrc,			/* [in] 圧縮元 */
	DWORD dwSrcSize,		/* [in] 圧縮元サイズ */
	LPBYTE pDst,			/* [in] 圧縮先 */
	DWORD dwDstSize)		/* [in] 圧縮先サイズ */
{
	DWORD dwRet;
	int nResult;

	dwRet = 0;
	m_pStream->next_in		= pSrc;
	m_pStream->avail_in		= dwSrcSize;
	m_pStream->next_out		= pDst;
	m_pStream->avail_out	= dwDstSize;
	deflateReset(m_pStream);

	/* 圧縮 */
	nResult = deflate (m_pStream, Z_FINISH);
	if (nResult == Z_STREAM_END) {
		/* 圧縮後のサイズを取得 */
		dwRet = m_pStream->total_out;
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::DeflateEnd												 */
/* 内容：	圧縮:後処理														 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

void CmyZlib::DeflateEnd(void)
{
	if (m_pStream == NULL) {
		return;
	}

	/* 後処理関数を呼ぶ */
	deflateEnd (m_pStream);

	delete m_pStream;
	m_pStream = NULL;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::InflateInit											 */
/* 内容：	解凍:初期化														 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

BOOL CmyZlib::InflateInit(void)
{
	BOOL bRet;
	int nResult;

	bRet = FALSE;

	if (m_pStream) {
		goto Exit;
	}

	/* 情報構造体を確保して初期化 */
	m_pStream = new z_stream;
	m_pStream->zalloc	= Z_NULL;
	m_pStream->zfree	= Z_NULL;
	m_pStream->opaque	= Z_NULL;

	/* zlibの初期化関数を呼ぶ */
	nResult = inflateInit (m_pStream);
	if (nResult != Z_OK) {
		delete m_pStream;
		m_pStream = NULL;
		goto Exit;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::Inflate												 */
/* 内容：	解凍:圧縮処理													 */
/* 日付：	2005/02/02														 */
/* 戻り値：	解凍後のサイズ													 */
/* ========================================================================= */

DWORD CmyZlib::Inflate(
	LPBYTE pSrc,			/* [in] 解凍元 */
	DWORD dwSrcSize,		/* [in] 解凍元サイズ */
	LPBYTE pDst,			/* [in] 解凍先 */
	DWORD dwDstSize)		/* [in] 解凍先サイズ */
{
	DWORD dwRet;
	int nResult;

	dwRet = 0;
	m_pStream->next_in		= pSrc;
	m_pStream->avail_in		= dwSrcSize;
	m_pStream->next_out		= pDst;
	m_pStream->avail_out	= dwDstSize;
	inflateReset (m_pStream);

	/* 解凍 */
	nResult = inflate (m_pStream, Z_FINISH);
	if (nResult == Z_STREAM_END) {
		/* 解凍後のサイズを取得 */
		dwRet = m_pStream->total_out;
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CmyZlib::InflateEnd												 */
/* 内容：	解凍:後処理														 */
/* 日付：	2005/02/02														 */
/* ========================================================================= */

void CmyZlib::InflateEnd(void)
{
	if (m_pStream == NULL) {
		return;
	}

	/* 後処理関数を呼ぶ */
	inflateEnd (m_pStream);

	delete m_pStream;
	m_pStream = NULL;
}
/* Copyright(C)URARA-works 2005 */
