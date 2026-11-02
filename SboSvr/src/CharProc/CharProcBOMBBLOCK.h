/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcBOMBBLOCK.h											 */
/* 内容：		キャラ処理(爆弾ゲームの壁)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcBOMBBLOCK : public CCharProcBase
{
public:
	CCharProcBOMBBLOCK();								/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcBOMBBLOCK, *PCCharProcBOMBBLOCK;

/* Copyright(C)URARA-works 2006 */
