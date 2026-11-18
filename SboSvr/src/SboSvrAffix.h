/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	SboSvrAffix.h												 */
/* 概要：		付与（接頭辞・接尾辞）の処理の外部参照用定義				 */
/* 作成日：		2026/11/18													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern WORD AffixDeriveFindOrCreate(WORD, WORD, WORD, BYTE);			/* 派生アイテムの番号を探す（無ければ作って保存し、全員へ送る） */
extern BOOL AffixCanDrop(WORD);											/* 付与が付く装備か（武器〜指輪で、大事な物ではない） */
extern WORD AffixRollDrop(WORD, BYTE, BYTE);							/* ドロップした装備に付与をくじ引きする（付かなければ元の番号。結果は未鑑定） */
extern WORD AffixAppraise(WORD);										/* 未鑑定の番号を鑑定済みの番号にする（未鑑定でなければそのまま） */
extern DWORD AffixAppraiseCost(WORD);									/* 鑑定の代金（max(10, 元の値段/10)） */
extern PSTATUSTEMPLATEINFO AffixGetStTmp(PCHARINFO);					/* 敵のステータステンプレート（発生元から出た敵は発生元のもの。無ければ NULL） */
extern WORD AffixDropItem(PCHARINFO, WORD);								/* 敵が装備を落とす時の番号を決める（付与が付けば未鑑定の派生番号） */

/* Copyright(C)URARA-works 2003-2026 */
