/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcUSEMAGIC.h											 */
/* 内容：		キャラ処理(重なったキャラに魔法使用)クラス 定義ファイル		 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcUSEMAGIC : public CCharProcBase
{
public:
	CCharProcUSEMAGIC();								/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcUSEMAGIC, *PCCharProcUSEMAGIC;

/* Copyright(C)URARA-works 2006 */
