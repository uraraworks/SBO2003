/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcATACK.h												 */
/* 内容：		キャラ処理(重なったキャラに攻撃)クラス 定義ファイル			 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcATACK : public CCharProcBase
{
public:
	CCharProcATACK();									/* コンストラクタ */

	BOOL ProcBow(DWORD dwTime);
	BOOL HitCurrentCell(void);
	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcATACK, *PCCharProcATACK;

/* Copyright(C)URARA-works 2006 */
