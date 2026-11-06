/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrBattleProc.h											 */
/* 概要：		戦闘処理の外部参照用定義									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/13													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void BattleProcCheck		(PCHARINFO);								/* 戦闘行動チェック */
extern void SetGuard			(PCHARINFO, BOOL);							/* 構え(防御)の開始・終了 */
extern BOOL IsGuard				(PCHARINFO);								/* 構えているか */
extern BOOL BattleProc			(DWORD, DWORD);								/* 戦闘処理 */
extern BOOL BattleProcWeapon	(PCHARINFO);								/* 武器処理 */
extern BOOL DamageAfterProcPC	(PCHARINFO, PCHARINFO);						/* ダメージ受けた後の処理(PC) */
extern BOOL DamageAfterProcNPC	(PCHARINFO, PCHARINFO);						/* ダメージ受けた後の処理(NPC) */
extern BOOL BattleProcAtack		(PCHARINFO, PCHARINFO);						/* 戦闘処理(直接攻撃) */
extern void ResurrectionBack	(PCHARINFO);								/* 復活位置で蘇生 */
extern void	AddStatus			(PCHARINFO pChar, BYTE byType, int nData);	/* パラメータの追加 */

/* Copyright(C)URARA-works 2003-2005 */
