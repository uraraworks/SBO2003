/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSEARCH.h											 */
/* 内容：		キャラ処理(検索してついていく)クラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcSEARCH : public CCharProcBase
{
public:
	CCharProcSEARCH();									/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcSEARCH, *PCCharProcSEARCH;

/* Copyright(C)URARA-works 2006 */
