/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGame.h												 */
/* 内容：		サブゲーム処理クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/04/19													 */
/* ========================================================================= */

#pragma once

class CInfoSubGameBase;

/* ========================================================================= */
/* 定数の定義																 */
/* ========================================================================= */

typedef CmyArray<CInfoSubGameBase *, CInfoSubGameBase *>	CARRAYSUBGAME;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CProcSubGame
{
public:
			CProcSubGame();									/* コンストラクタ */
	virtual ~CProcSubGame();								/* デストラクタ */

	void Destroy	(void);											/* 破棄 */

	void				 Add		(CInfoSubGameBase *pInfo);				/* 追加 */
	CInfoSubGameBase	*Get		(int nNo);								/* 取得 */
	CInfoSubGameBase	*GetNew		(int nType);							/* 指定種別の新規サブゲーム情報を取得 */
	void				 Delete		(int nNo);								/* 削除 */
	void				 DeleteAll	(void);									/* 全て削除 */
	int					 GetCount	(void);									/* サブゲーム数を取得 */


private:
	CARRAYSUBGAME		m_aSubGame;					/* サブゲーム情報 */
} CProcSubGame, *PCProcSubGame;

/* Copyright(C)URARA-works 2006 */
