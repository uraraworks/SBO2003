/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcSTOPRAND.h											 */
/* 内容：		キャラ処理(止まるかランダム)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/16													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcSTOPRAND : public CCharProcBase
{
public:
	CCharProcSTOPRAND();								/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcSTOPRAND, *PCCharProcSTOPRAND;

/* Copyright(C)URARA-works 2006 */
