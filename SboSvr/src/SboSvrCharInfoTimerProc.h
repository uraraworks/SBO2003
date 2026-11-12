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
extern void SetCharInfoTimerStatusMin	(PCHARINFO pChar, BYTE byType, DWORD dwMin);	/* 攻撃力・守備力アップの最低でも上がる量 */
extern int  GetStatusUpBonus		(PCHARINFO pChar, BYTE byType, int nBase);	/* 攻撃力・守備力アップで上がる量(かかっていなければ0) */
extern void BuffSendState			(PCHARINFO pChar);					/* 攻撃力・守備力アップの残りを本人と仲間へ送る */
extern void BuffSendPartyAll		(PCHARINFO pChar);					/* パーティに入った時、仲間どうしでアップの残りを見せ合う */

/* Copyright(C)URARA-works 2003-2006 */
