/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGame.h												 */
/* 内容：		サブゲーム処理クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/18													 */
/* ========================================================================= */

#pragma once

class CProcSubGameBase;

/* ========================================================================= */
/* 定数の定義																 */
/* ========================================================================= */

typedef CmyArray<CProcSubGameBase *, CProcSubGameBase *>	CARRAYSUBGAME;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CProcSubGame
{
public:
			CProcSubGame();									/* コンストラクタ */
	virtual ~CProcSubGame();								/* デストラクタ */

	void Destroy	(void);											/* 破棄 */
	void Proc		(void);											/* 処理 */

	BYTE				 GetNewID	(void);									/* 追加用の新しいIDを取得 */
	void				 Add		(CProcSubGameBase *pProc);				/* サブゲーム処理を追加 */
	CProcSubGameBase	*Get		(BYTE byID);							/* サブゲーム処理を取得 */
	CProcSubGameBase	*Get		(DWORD dwMapID, BYTE byX, BYTE byY);	/* サブゲーム処理を取得 */
	CProcSubGameBase	*Get		(int nNo);								/* サブゲーム処理を取得 */
	void				 Delete		(BYTE byID);							/* サブゲーム処理を削除 */
	int					 GetCount	(void);									/* サブゲーム数を取得 */


private:
	int					m_nProcNo;					/* 処理中のサブゲーム */
	CARRAYSUBGAME		m_aSubGame;					/* サブゲーム処理 */
} CProcSubGame, *PCProcSubGame;

/* Copyright(C)URARA-works 2006 */
