/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboPacket.h													 */
/* 内容：		通信パケットの文字列を可変長で送るための詰め直し			 */
/*																			 */
/*   メモリ上の構造体は今まで通り固定長の文字列欄を持つ。送る時だけ		 */
/*   文字列欄を抜いた残り（固定部分）を先に並べ、その後ろに文字列を		 */
/*   終端付きで順に並べる。受け取る側は逆の手順で構造体に戻す。			 */
/*																			 */
/*   [固定部分（文字列欄を除いた構造体のバイト列）][文字列1\0][文字列2\0]... */
/*   この後ろに、今まで通りの続きのデータ（NPC発言など）が並ぶ。			 */
/* ========================================================================= */

#pragma once

#include <stddef.h>

/* 構造体の中の文字列欄 1 つ分 */
typedef struct _SBOPACKSTR {
	WORD		wOffset;											/* 構造体の先頭からの位置 */
	WORD		wSize;												/* 欄の大きさ（終端込み） */
} SBOPACKSTR, *PSBOPACKSTR;

#define SBOPACKSTR_FIELD(type, member)	{ (WORD)offsetof (type, member), (WORD)sizeof (((type *)0)->member) }

/* 汎用（paStr は位置の小さい順に並べる） */
DWORD	SboPackSize	(const void *pStruct, DWORD dwStructSize, const SBOPACKSTR *paStr, int nCount);
PBYTE	SboPackPut	(PBYTE pDst, const void *pStruct, DWORD dwStructSize, const SBOPACKSTR *paStr, int nCount);
PBYTE	SboPackGet	(void *pStruct, DWORD dwStructSize, const BYTE *pSrc, const BYTE *pEnd, const SBOPACKSTR *paStr, int nCount);

/* 構造体ごとの関数
 *   SboPackSizeXXX (p)				: 詰めた後の大きさ
 *   SboPackPutXXX  (pDst, p)		: pDst に詰めて、次に書く位置を返す
 *   SboPackGetXXX  (p, pSrc, pEnd)	: pSrc から p に戻して、続きの位置を返す
 *									  （pEnd は受信データの終わり。NULL なら確かめない。
 *									    足りない時は NULL を返す）
 */
#define SBOPACK_DECLARE(name, type)															\
	DWORD	SboPackSize##name	(const type *p);											\
	PBYTE	SboPackPut##name	(PBYTE pDst, const type *p);								\
	PBYTE	SboPackGet##name	(type *p, const BYTE *pSrc, const BYTE *pEnd);

SBOPACK_DECLARE (CHARINFO,				CMDPARA_CHARINFO)
SBOPACK_DECLARE (CHARINFO2,				CMDPARA_CHARINFO2)
SBOPACK_DECLARE (ADDCHARINFO,			CMDPARA_ADDCHARINFO)
SBOPACK_DECLARE (NAMESEX,				CMDPARA_NAMESEX)
SBOPACK_DECLARE (CHGITEM,				CMDPARA_CHGITEM)
SBOPACK_DECLARE (ADDCHAREFC,			CMDPARA_ADDCHAREFC)
SBOPACK_DECLARE (ACCSEARCH,				CMDPARA_ACCSEARCH)
SBOPACK_DECLARE (MAGICINFO,				CMDPARA_MAGICINFO)
SBOPACK_DECLARE (MAGICUSERINFO,			CMDPARA_MAGICUSERINFO)
SBOPACK_DECLARE (EQUUSERINFO,			CMDPARA_EQUUSERINFO)
SBOPACK_DECLARE (WARSELECTTURN,			CMDPARA_WARSELECTTURN)
SBOPACK_DECLARE (WARSELECTMAGICTARGET,	CMDPARA_WARSELECTMAGICTARGET)
SBOPACK_DECLARE (ITEMINFO,				ITEMINFO)
SBOPACK_DECLARE (ITEMINFO_MSG,			ITEMINFO_MSG)
SBOPACK_DECLARE (STATUSTEMPLATEINFO,	STATUSTEMPLATEINFO)

/* アイテムの詳細データ（種別ごとの構造体。文字列欄があるのはメッセージ表示だけ） */
DWORD	SboPackSizeItemData	(BYTE byType, const void *pData, DWORD dwRawSize);
PBYTE	SboPackPutItemData	(PBYTE pDst, BYTE byType, const void *pData, DWORD dwRawSize);
PBYTE	SboPackGetItemData	(void *pData, BYTE byType, DWORD dwRawSize, const BYTE *pSrc, const BYTE *pEnd);

/* Copyright(C)URARA-works 2026 */
