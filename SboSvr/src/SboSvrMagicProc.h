/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrMagicProc.h											 */
/* 概要：		魔法処理の外部参照用定義									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/13													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern BOOL MagicProc(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);		/* 魔法処理 */
extern BOOL MagicProcItem(DWORD dwTblID, BYTE byID, DWORD dwTblIDTarget);	/* 魔法処理(アイテムから発動) */
extern BOOL IsTownWarpMagic(BYTE byID);											/* 町転送の特技か */
extern BOOL IsMagicWall(DWORD dwMapID, int x, int y);	/* 特技や飛び道具を遮る地形か */
extern BOOL IsMagicReachChar(PCHARINFO pChar, PCHARINFO pCharTarget);			/* 使った人から相手まで壁越しでなく届くか */
extern int  TownWarpMakeList(PCHARINFO pChar, PDWORD padwMapID, PDWORD padwX, PDWORD padwY);	/* 町転送の行き先一覧を作る */
extern void TownWarpRecord(PCHARINFO pChar, DWORD dwMapID, DWORD dwX, DWORD dwY, int nForgetMapID);	/* 教会で復活位置を記録し、行き先帳に書く */
/* Copyright(C)URARA-works 2003-2005 */
