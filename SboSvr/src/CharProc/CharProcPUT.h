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
	BOOL PutOne		(DWORD dwTime, BOOL bSpaced = FALSE, const DWORD *pdwSeed = NULL);		/* 1体出す（くじで選んだマスがだめなら FALSE）。bSpaced は番外編ダンジョンの空間ごとの発生（数・時刻は呼ぶ側が数える）。pdwSeed があれば、位置のくじはそのシードの std::mt19937 で引く（日替わりダンジョン。どの組も同じ位置になる） */
	BOOL PutOneSpace	(DWORD dwTime, DWORD dwSpaceID, const DWORD *pdwSeed = NULL);			/* 指定した空間に 1体出す（番外編ダンジョン。発生元の数・時刻は動かさない。pdwSeed は PutOne と同じ） */
} CCharProcPUT, *PCCharProcPUT;

/* 発生元の能力（テンプレート指定中はテンプレート）を出てくるキャラへ写す */
void PutCharSetStatus	(PCHARINFO pParent, PCHARINFO pChar);
/* 発生元から出ているキャラへ、今の発生元の能力を反映する（管理者ウィンドウ・データ API で変えた時） */
void RenewPutChildChar	(PCHARINFO pParent);
void PutCharFillAll		(void);					/* 起動時に各NPC発生を発生数まで出す */

/* Copyright(C)URARA-works 2006 */
