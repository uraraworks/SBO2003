/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGame.cpp												 */
/* 内容：		サブゲーム処理クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/18													 */
/* ========================================================================= */

#include "stdafx.h"
#include "SubGame/ProcSubGameBase.h"
#include "ProcSubGame.h"

/* ========================================================================= */
/* 関数名：	CProcSubGame::CProcSubGame										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

CProcSubGame::CProcSubGame()
{
	m_nProcNo = 0;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::~CProcSubGame										 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

CProcSubGame::~CProcSubGame()
{
	Destroy ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Destroy											 */
/* 内容：	破棄															 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

void CProcSubGame::Destroy(void)
{
	int i, nCount;
	PCProcSubGameBase pProc;

	nCount = m_aSubGame.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pProc = m_aSubGame[i];
		SAFE_DELETE (pProc);
	}
	m_aSubGame.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Proc												 */
/* 内容：	処理															 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

void CProcSubGame::Proc(void)
{
	int nCount;
	PCProcSubGameBase pProc;

	nCount = m_aSubGame.GetSize ();
	if (nCount <= 0) {
		return;
	}
	if (m_nProcNo >= nCount) {
		m_nProcNo = 0;
	}

	pProc = m_aSubGame[m_nProcNo];
	pProc->Proc ();
	m_nProcNo ++;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::GetNewID											 */
/* 内容：	追加用の新しいIDを取得											 */
/* 日付：	2006/04/06														 */
/* ========================================================================= */

BYTE CProcSubGame::GetNewID(void)
{
	BYTE byRet, byID;
	int i, nCount;
	PCProcSubGameBase pProc;

	byRet	= 1;
	nCount	= m_aSubGame.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pProc	= m_aSubGame[i];
		byID	= pProc->GetID ();
		if (byID >= byRet) {
			byRet = byID + 1;
		}
	}

	return byRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Add												 */
/* 内容：	サブゲーム処理を追加											 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

void CProcSubGame::Add(CProcSubGameBase *pProc)
{
	m_aSubGame.Add (pProc);
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Get												 */
/* 内容：	サブゲーム処理を取得											 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

CProcSubGameBase *CProcSubGame::Get(BYTE byID)
{
	int i, nCount;
	PCProcSubGameBase pRet, pProc;

	pRet	= NULL;
	nCount	= m_aSubGame.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pProc = m_aSubGame[i];
		if (pProc->GetID () == byID) {
			pRet = pProc;
			break;
		}
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Get												 */
/* 内容：	サブゲーム処理を取得											 */
/* 日付：	2006/04/07														 */
/* 備考：	指定された受付座標の情報を取得する								 */
/* ========================================================================= */

CProcSubGameBase *CProcSubGame::Get(
	DWORD dwMapID,		/* [in] マップID */
	BYTE byX,			/* [in] 座標(横) */
	BYTE byY)			/* [in] 座標(縦) */
{
	BOOL bResult;
	int i, nCount;
	PCProcSubGameBase pRet, pProc;

	pRet	= NULL;
	nCount	= m_aSubGame.GetSize ();

	for (i = 0; i < nCount; i ++) {
		pProc	= m_aSubGame[i];
		bResult	= pProc->IsInformationPos (dwMapID, byX, byY);
		if (bResult) {
			pRet = pProc;
			break;
		}
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::Get												 */
/* 内容：	サブゲーム処理を取得											 */
/* 日付：	2006/04/18														 */
/* ========================================================================= */

CProcSubGameBase *CProcSubGame::Get(int nNo)
{
	int nCount;
	PCProcSubGameBase pRet;

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
/* 関数名：	CProcSubGame::Delete											 */
/* 内容：	サブゲーム処理を削除											 */
/* 日付：	2006/03/18														 */
/* ========================================================================= */

void CProcSubGame::Delete(BYTE byID)
{
	int i, nCount;
	PCProcSubGameBase pProc;

	nCount = m_aSubGame.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pProc = m_aSubGame[i];
		if (pProc->GetID () == byID) {
			SAFE_DELETE (pProc);
			m_aSubGame.RemoveAt (i);
			break;
		}
	}
}


/* ========================================================================= */
/* 関数名：	CProcSubGame::GetCount											 */
/* 内容：	サブゲーム数を取得												 */
/* 日付：	2006/04/18														 */
/* ========================================================================= */

int CProcSubGame::GetCount(void)
{
	return m_aSubGame.GetSize ();
}

/* Copyright(C)URARA-works 2006 */
