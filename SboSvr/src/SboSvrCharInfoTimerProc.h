/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboSvrCharInfoTimerProc.h									 */
/* 概要：		キャラ情報タイマーイベント処理の外部参照用定義				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/10/17													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void DeleteAllTimerEvent		(PCHARINFO pChar);					/* タイマーイベントを削除 */
extern void CharInfoTimerProc		(DWORD dwTblID);					/* キャラ情報タイマーイベント処理 */
extern BOOL CheckCharInfoTimer		(DWORD dwTblID, BYTE byType);		/* 指定したイベントを持っているかチェック */
extern void SetCharInfoTimerStatus	(PCHARINFO pChar, BYTE byType, BYTE byData, DWORD dwTimeValid);	/* 状態(加速・毒・眠り)をかける(同じ種類はかけ直し) */
extern BYTE GetHasteRate			(PCHARINFO pChar);					/* 加速の率(%)。かかっていなければ0 */
extern BOOL IsSleep					(PCHARINFO pChar);					/* 眠っているか */

/* Copyright(C)URARA-works 2003-2006 */
