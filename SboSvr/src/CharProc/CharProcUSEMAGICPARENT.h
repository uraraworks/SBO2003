/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcUSEMAGICPARENT.h									 */
/* 内容：		キャラ処理(魔法発動し続ける)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcUSEMAGICPARENT : public CCharProcBase
{
public:
	CCharProcUSEMAGICPARENT();							/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcUSEMAGICPARENT, *PCCharProcUSEMAGICPARENT;

/* Copyright(C)URARA-works 2006 */
