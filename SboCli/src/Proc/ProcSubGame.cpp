/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGame.cpp												 */
/* 内容：		サブゲーム処理クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/04/19													 */
/* ========================================================================= */

#include "stdafx.h"
#include "../../../Common/Info/SubGame/InfoSubGameWAR.h"
#include "ProcSubGame.h"

/* ========================================================================= */
/* 関数名：	CProcSubGame::CProcSubGame										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

CProcSubGame::CProcSubGame()
{
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::~CProcSubGame										 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

CProcSubGame::~CProcSubGame()
{
	Destroy ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Destroy											 */
/* 内容：	破棄															 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

void CProcSubGame::Destroy(void)
{
	int i, nCount;
	PCInfoSubGameBase pProc;

	nCount = m_aSubGame.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pProc = m_aSubGame[i];
		SAFE_DELETE (pProc);
	}
	m_aSubGame.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Add												 */
/* 内容：	追加															 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

void CProcSubGame::Add(CInfoSubGameBase *pInfo)
{
	m_aSubGame.Add (pInfo);
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Get												 */
/* 内容：	取得															 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

CInfoSubGameBase *CProcSubGame::Get(int nNo)
{
	int nCount;
	PCInfoSubGameBase pRet;

	pRet	= NULL;
	nCount	= m_aSubGame.GetSize ();

	if (nNo >= nCount) {
		goto Exit;
	}

	pRet = m_aSubGame[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::GetNew											 */
/* 内容：	指定種別の新規サブゲーム情報を取得								 */
/* 日付：	2006/04/20														 */
/* ========================================================================= */

CInfoSubGameBase *CProcSubGame::GetNew(int nType)
{
	PCInfoSubGameBase pRet;

	pRet = NULL;

	switch (nType) {
	case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
	case SUBGAMEID_MAZE:		/* 迷路 */
	case SUBGAMEID_DUNGEON:		/* ダンジョン */
		break;
	case SUBGAMEID_WAR:			/* 戦場ゲーム */
		pRet = new CInfoSubGameWAR;
		break;
	case SUBGAMEID_SUGOROKU:	/* すごろく */
		break;
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Delete											 */
/* 内容：	削除															 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

void CProcSubGame::Delete(int nNo)
{
	PCInfoSubGameBase pInfo;

	pInfo = m_aSubGame[nNo];
	SAFE_DELETE (pInfo);
	m_aSubGame.RemoveAt (nNo);
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::DeleteAll											 */
/* 内容：	全て削除														 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

void CProcSubGame::DeleteAll(void)
{
	int i, nCount;

	nCount = GetCount ();
	for (i = 0; i < nCount; i ++) {
		Delete (0);
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::GetCount											 */
/* 内容：	取得															 */
/* 日付：	2006/04/19														 */
/* ========================================================================= */

int CProcSubGame::GetCount(void)
{
	return m_aSubGame.GetSize ();
}

/* Copyright(C)URARA-works 2006 */
