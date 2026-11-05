/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboPacket.cpp												 */
/* 内容：		通信パケットの文字列を可変長で送るための詰め直し			 */
/* ========================================================================= */

#include "StdAfx.h"
#include "SboText.h"
#include "TypedefCommon.h"
#include "SboPacket.h"

/* ------------------------------------------------------------------------- */
/* 欄の中の文字列の長さ（終端が無くても欄の外は読まない）					 */
/* ------------------------------------------------------------------------- */
static DWORD FieldLen(const BYTE *pField, WORD wSize)
{
	DWORD dwLen = 0;

	while ((dwLen + 1 < wSize) && pField[dwLen]) {
		dwLen ++;
	}
	return dwLen;
}

/* ========================================================================= */
/* 関数名：	SboPackSize														 */
/* 内容：	詰めた後の大きさ												 */
/* ========================================================================= */
DWORD SboPackSize(const void *pStruct, DWORD dwStructSize, const SBOPACKSTR *paStr, int nCount)
{
	const BYTE *pSrc = (const BYTE *)pStruct;
	DWORD dwSize = dwStructSize;
	int i;

	for (i = 0; i < nCount; i ++) {
		dwSize -= paStr[i].wSize;
		dwSize += FieldLen (pSrc + paStr[i].wOffset, paStr[i].wSize) + 1;
	}
	return dwSize;
}

/* ========================================================================= */
/* 関数名：	SboPackPut														 */
/* 内容：	固定部分、文字列の順に詰めて、次に書く位置を返す				 */
/* ========================================================================= */
PBYTE SboPackPut(PBYTE pDst, const void *pStruct, DWORD dwStructSize, const SBOPACKSTR *paStr, int nCount)
{
	const BYTE *pSrc = (const BYTE *)pStruct;
	DWORD dwPos = 0, dwLen;
	int i;

	/* 固定部分（文字列欄の間） */
	for (i = 0; i < nCount; i ++) {
		dwLen = paStr[i].wOffset - dwPos;
		CopyMemory (pDst, pSrc + dwPos, dwLen);
		pDst  += dwLen;
		dwPos  = paStr[i].wOffset + paStr[i].wSize;
	}
	dwLen = dwStructSize - dwPos;
	CopyMemory (pDst, pSrc + dwPos, dwLen);
	pDst += dwLen;

	/* 文字列（終端付き） */
	for (i = 0; i < nCount; i ++) {
		dwLen = FieldLen (pSrc + paStr[i].wOffset, paStr[i].wSize);
		CopyMemory (pDst, pSrc + paStr[i].wOffset, dwLen);
		pDst[dwLen] = 0;
		pDst += dwLen + 1;
	}
	return pDst;
}

/* ========================================================================= */
/* 関数名：	SboPackGet														 */
/* 内容：	詰めたデータを構造体に戻して、続きの位置を返す					 */
/*			（pEnd までに収まらない時は NULL）								 */
/* ========================================================================= */
PBYTE SboPackGet(void *pStruct, DWORD dwStructSize, const BYTE *pSrc, const BYTE *pEnd, const SBOPACKSTR *paStr, int nCount)
{
	BYTE *pDst = (BYTE *)pStruct;
	DWORD dwPos = 0, dwLen, dwFixed = dwStructSize;
	int i;

	for (i = 0; i < nCount; i ++) {
		dwFixed -= paStr[i].wSize;
	}
	if (pEnd && ((pSrc > pEnd) || ((DWORD)(pEnd - pSrc) < dwFixed))) {
		return NULL;
	}

	/* 固定部分 */
	for (i = 0; i < nCount; i ++) {
		dwLen = paStr[i].wOffset - dwPos;
		CopyMemory (pDst + dwPos, pSrc, dwLen);
		pSrc  += dwLen;
		dwPos  = paStr[i].wOffset + paStr[i].wSize;
	}
	dwLen = dwStructSize - dwPos;
	CopyMemory (pDst + dwPos, pSrc, dwLen);
	pSrc += dwLen;

	/* 文字列（欄に入りきらない分は文字の途中で切らずに捨てる） */
	for (i = 0; i < nCount; i ++) {
		LPSTR pszField = (LPSTR)(pDst + paStr[i].wOffset);

		/* 終端を探す（受信データの外は読まない） */
		dwLen = 0;
		for (;;) {
			if (pEnd && (pSrc + dwLen >= pEnd)) {
				ZeroMemory (pszField, paStr[i].wSize);
				return NULL;
			}
			if (pSrc[dwLen] == 0) {
				break;
			}
			dwLen ++;
		}
		ZeroMemory (pszField, paStr[i].wSize);
		SboTextCopy (pszField, paStr[i].wSize, (LPCSTR)pSrc);
		pSrc += dwLen + 1;
	}
	return (PBYTE)pSrc;
}

/* ------------------------------------------------------------------------- */
/* 構造体ごとの文字列欄の表と関数											 */
/* ------------------------------------------------------------------------- */
#define SBOPACK_DEFINE(name, type)																\
	DWORD SboPackSize##name (const type *p)														\
	{																							\
		return SboPackSize (p, sizeof (type), s_aStr##name, (int)(sizeof (s_aStr##name) / sizeof (s_aStr##name[0])));	\
	}																							\
	PBYTE SboPackPut##name (PBYTE pDst, const type *p)											\
	{																							\
		return SboPackPut (pDst, p, sizeof (type), s_aStr##name, (int)(sizeof (s_aStr##name) / sizeof (s_aStr##name[0])));	\
	}																							\
	PBYTE SboPackGet##name (type *p, const BYTE *pSrc, const BYTE *pEnd)						\
	{																							\
		return SboPackGet (p, sizeof (type), pSrc, pEnd, s_aStr##name, (int)(sizeof (s_aStr##name) / sizeof (s_aStr##name[0])));	\
	}

/* アカウント名・パスワード・MAC アドレスは英数字（とバイナリ）なので固定長のまま */
static const SBOPACKSTR s_aStrCHARINFO[]		= { SBOPACKSTR_FIELD (CMDPARA_CHARINFO,		szName) };
static const SBOPACKSTR s_aStrCHARINFO2[]		= { SBOPACKSTR_FIELD (CMDPARA_CHARINFO2,	szName) };
static const SBOPACKSTR s_aStrADDCHARINFO[]		= { SBOPACKSTR_FIELD (CMDPARA_ADDCHARINFO,	szName) };
static const SBOPACKSTR s_aStrNAMESEX[]			= { SBOPACKSTR_FIELD (CMDPARA_NAMESEX,		szName) };
static const SBOPACKSTR s_aStrCHGITEM[]			= { SBOPACKSTR_FIELD (CMDPARA_CHGITEM,		szName) };
static const SBOPACKSTR s_aStrADDCHAREFC[]		= { SBOPACKSTR_FIELD (CMDPARA_ADDCHAREFC,	szMsg) };
static const SBOPACKSTR s_aStrACCSEARCH[]		= { SBOPACKSTR_FIELD (CMDPARA_ACCSEARCH,	szName) };
static const SBOPACKSTR s_aStrMAGICINFO[]		= { SBOPACKSTR_FIELD (CMDPARA_MAGICINFO,	szName),
													SBOPACKSTR_FIELD (CMDPARA_MAGICINFO,	szExplain) };
static const SBOPACKSTR s_aStrMAGICUSERINFO[]	= { SBOPACKSTR_FIELD (CMDPARA_MAGICUSERINFO,	szName) };
static const SBOPACKSTR s_aStrEQUUSERINFO[]		= { SBOPACKSTR_FIELD (CMDPARA_EQUUSERINFO,		szName) };
static const SBOPACKSTR s_aStrWARSELECTTURN[]	= { SBOPACKSTR_FIELD (CMDPARA_WARSELECTTURN,	aszName[0]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTTURN,	aszName[1]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTTURN,	aszName[2]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTTURN,	aszName[3]) };
static const SBOPACKSTR s_aStrWARSELECTMAGICTARGET[] = {
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTMAGICTARGET,	aszName[0]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTMAGICTARGET,	aszName[1]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTMAGICTARGET,	aszName[2]),
													SBOPACKSTR_FIELD (CMDPARA_WARSELECTMAGICTARGET,	aszName[3]) };
static const SBOPACKSTR s_aStrITEMINFO[]		= { SBOPACKSTR_FIELD (ITEMINFO,				szName) };
static const SBOPACKSTR s_aStrITEMINFO_MSG[]	= { SBOPACKSTR_FIELD (ITEMINFO_MSG,			szCharEfc) };
static const SBOPACKSTR s_aStrSTATUSTEMPLATEINFO[] = { SBOPACKSTR_FIELD (STATUSTEMPLATEINFO, szName) };

/* 戦場ゲームの名前欄は 4 人分を前提に表を作っている */
typedef char SBOPACK_CHECK_WARMEMBER[(MAX_SUBGAME_WARMEMBER == 4) ? 1 : -1];

SBOPACK_DEFINE (CHARINFO,				CMDPARA_CHARINFO)
SBOPACK_DEFINE (CHARINFO2,				CMDPARA_CHARINFO2)
SBOPACK_DEFINE (ADDCHARINFO,			CMDPARA_ADDCHARINFO)
SBOPACK_DEFINE (NAMESEX,				CMDPARA_NAMESEX)
SBOPACK_DEFINE (CHGITEM,				CMDPARA_CHGITEM)
SBOPACK_DEFINE (ADDCHAREFC,				CMDPARA_ADDCHAREFC)
SBOPACK_DEFINE (ACCSEARCH,				CMDPARA_ACCSEARCH)
SBOPACK_DEFINE (MAGICINFO,				CMDPARA_MAGICINFO)
SBOPACK_DEFINE (MAGICUSERINFO,			CMDPARA_MAGICUSERINFO)
SBOPACK_DEFINE (EQUUSERINFO,			CMDPARA_EQUUSERINFO)
SBOPACK_DEFINE (WARSELECTTURN,			CMDPARA_WARSELECTTURN)
SBOPACK_DEFINE (WARSELECTMAGICTARGET,	CMDPARA_WARSELECTMAGICTARGET)
SBOPACK_DEFINE (ITEMINFO,				ITEMINFO)
SBOPACK_DEFINE (ITEMINFO_MSG,			ITEMINFO_MSG)
SBOPACK_DEFINE (STATUSTEMPLATEINFO,		STATUSTEMPLATEINFO)

/* ========================================================================= */
/* 関数名：	SboPackSizeItemData												 */
/* 内容：	アイテムの詳細データを詰めた後の大きさ							 */
/* ========================================================================= */
DWORD SboPackSizeItemData(BYTE byType, const void *pData, DWORD dwRawSize)
{
	if ((byType == ITEMTYPE_MSG) && (dwRawSize == sizeof (ITEMINFO_MSG))) {
		return SboPackSizeITEMINFO_MSG ((const ITEMINFO_MSG *)pData);
	}
	return dwRawSize;
}

/* ========================================================================= */
/* 関数名：	SboPackPutItemData												 */
/* 内容：	アイテムの詳細データを詰める									 */
/* ========================================================================= */
PBYTE SboPackPutItemData(PBYTE pDst, BYTE byType, const void *pData, DWORD dwRawSize)
{
	if ((byType == ITEMTYPE_MSG) && (dwRawSize == sizeof (ITEMINFO_MSG))) {
		return SboPackPutITEMINFO_MSG (pDst, (const ITEMINFO_MSG *)pData);
	}
	CopyMemory (pDst, pData, dwRawSize);
	return pDst + dwRawSize;
}

/* ========================================================================= */
/* 関数名：	SboPackGetItemData												 */
/* 内容：	アイテムの詳細データを戻す（pData は dwRawSize バイト）			 */
/* ========================================================================= */
PBYTE SboPackGetItemData(void *pData, BYTE byType, DWORD dwRawSize, const BYTE *pSrc, const BYTE *pEnd)
{
	if ((byType == ITEMTYPE_MSG) && (dwRawSize == sizeof (ITEMINFO_MSG))) {
		return SboPackGetITEMINFO_MSG ((ITEMINFO_MSG *)pData, pSrc, pEnd);
	}
	if (pEnd && ((pSrc > pEnd) || ((DWORD)(pEnd - pSrc) < dwRawSize))) {
		return NULL;
	}
	CopyMemory (pData, pSrc, dwRawSize);
	return (PBYTE)pSrc + dwRawSize;
}

/* Copyright(C)URARA-works 2026 */
