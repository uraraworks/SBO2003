/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	CharProcPUT.h												 */
/* 内容：		キャラ処理(NPC発生)クラス 定義ファイル						 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/17													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcPUT : public CCharProcBase
{
public:
	CCharProcPUT();										/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */
} CCharProcPUT, *PCCharProcPUT;

/* 発生元の能力（テンプレート指定中はテンプレート）を出てくるキャラへ写す */
void PutCharSetStatus	(PCHARINFO pParent, PCHARINFO pChar);
/* 発生元から出ているキャラへ、今の発生元の能力を反映する（管理者ウィンドウ・データ API で変えた時） */
void RenewPutChildChar	(PCHARINFO pParent);

/* Copyright(C)URARA-works 2006 */
