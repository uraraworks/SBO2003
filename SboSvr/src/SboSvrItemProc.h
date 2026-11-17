/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboSvrItemProc.h											 */
/* 概要：		アイテム処理の外部参照用定義								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/15													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void ItemProc(DWORD, BYTE, DWORD);								/* アイテム処理 */
extern void ItemMsgAddWait(LPSTR, int);									/* 最後に入力待ちを足す */
extern void ItemMsgExpand(LPSTR, int, LPCSTR, const BYTE *, const BYTE *);	/* 文の {f番号}・{v番号} を印・数値で置き換える */
extern void ItemProcImportant(DWORD, BYTE, DWORD);						/* 大事な物の欄の品を使う */
extern BOOL CheckItemSexDistinction(PITEMINFO, PCHARINFO);				/* 性別による使えるかチェック */
extern BOOL ItemUseMercenary(PCHARINFO, BYTE, PCHARINFO);				/* 傭兵が持っている道具を使う */
/* Copyright(C)URARA-works 2003-2006 */
