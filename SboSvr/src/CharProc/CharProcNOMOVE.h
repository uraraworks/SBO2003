/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcNOMOVE.h											 */
/* 内容：		キャラ処理(移動しない)クラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcNOMOVE : public CCharProcBase
{
public:
	CCharProcNOMOVE();									/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcNOMOVE, *PCCharProcNOMOVE;

/* Copyright(C)URARA-works 2006 */
