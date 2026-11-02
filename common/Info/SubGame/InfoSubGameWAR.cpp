/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoSubGameWAR.cpp											 */
/* 内容：		サブゲーム情報(戦場ゲーム)クラス 実装ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/14													 */
/* ========================================================================= */

#include "stdafx.h"
#include "InfoSubGameWAR.h"

/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::CInfoSubGameWAR								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/03/14														 */
/* ========================================================================= */

CInfoSubGameWAR::CInfoSubGameWAR()
{
	Init ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::~CInfoSubGameWAR								 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/03/14														 */
/* ========================================================================= */

CInfoSubGameWAR::~CInfoSubGameWAR()
{
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::Init											 */
/* 内容：	ゲーム情報を初期化												 */
/* 日付：	2006/03/17														 */
/* ========================================================================= */

void CInfoSubGameWAR::Init(void)
{
	CInfoSubGameBase::Init ();

	m_byMember			= 0;
	m_byRule			= 0;
	m_byState			= 0;
	m_byAllMember		= 0;
	m_byTurn			= 0;
	m_dwWaitTblID		= -1;
	m_dwFirstTimeProc	= 0;
	m_dwLastTimeProc	= 0;
	m_dwNextTimeProc	= 0;
	m_dwTblIDJudge		= -1;

	ZeroMemory (&m_ptInformation, sizeof (m_ptInformation));
	ZeroMemory (&m_ptJudge, sizeof (m_ptJudge));

	DeleteAllTeamInfo ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::InitTeamInfo									 */
/* 内容：	チーム情報を初期化												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::InitTeamInfo(void)
{
	int i, nCount;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	nCount = GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = GetTeamInfo (i);

		DeleteAllMemberInfo (pTeamInfo);
		pTeamInfo->byTurn = 0;
	}
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::AddMember										 */
/* 内容：	メンバーを追加													 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::AddMember(
	int nTeamNo,								/* [in] チーム番号 */
	PSUBGAME_WAR_MEMBERINFO pMemberInfo)		/* [in] メンバー情報 */
{
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo = GetTeamInfo (nTeamNo);
	if (pTeamInfo == NULL) {
		return;
	}

	pTeamInfo->aMemberInfo.Add (pMemberInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteMember									 */
/* 内容：	メンバーを削除													 */
/* 日付：	2006/03/23														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteMember(DWORD dwTblID)
{
	int nTeamNo, nMemberNo;

	nTeamNo		= GetTeamNo (dwTblID);
	nMemberNo	= GetMemberNo (nTeamNo, dwTblID);

	DeleteMemberInfo (nTeamNo, nMemberNo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteMemberInfo								 */
/* 内容：	メンバー情報を削除												 */
/* 日付：	2006/03/23														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteMemberInfo(int nTeamNo, int nMemberNo)
{
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo	= GetTeamInfo (nTeamNo);
	pMemberInfo	= GetMemberInfo (nTeamNo, nMemberNo);

	DeleteMemberInfo (pMemberInfo);
	pTeamInfo->aMemberInfo.RemoveAt (nMemberNo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteMemberInfo								 */
/* 内容：	メンバー情報を削除												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteMemberInfo(PSUBGAME_WAR_MEMBERINFO pMemberInfo)
{
	SAFE_DELETE (pMemberInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteAllMemberInfo							 */
/* 内容：	全メンバー情報を削除											 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteAllMemberInfo(PSUBGAME_WAR_TEAMINFO pTeamInfo)
{
	int i, nCount;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	nCount = GetMemberCount (pTeamInfo);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = pTeamInfo->aMemberInfo[i];
		DeleteMemberInfo (pMemberInfo);
	}
	pTeamInfo->aMemberInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteAllMemberInfo							 */
/* 内容：	全メンバー情報を削除											 */
/* 日付：	2006/04/04														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteAllMemberInfo(int nTeamNo)
{
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pTeamInfo = GetTeamInfo (nTeamNo);
	if (pTeamInfo == NULL) {
		return;
	}

	DeleteAllMemberInfo (pTeamInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetNewMemberInfo								 */
/* 内容：	新しいメンバー情報を取得										 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

PSUBGAME_WAR_MEMBERINFO CInfoSubGameWAR::GetNewMemberInfo(void)
{
	PSUBGAME_WAR_MEMBERINFO pRet;

	pRet = new SUBGAME_WAR_MEMBERINFO;
	pRet->byJob				= 0;
	pRet->dwTblID			= -1;
	pRet->dwTblIDHP			= -1;
	pRet->dwTblIDRes		= -1;
	pRet->dwTblIDTargetRes	= -1;

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetMemberInfo									 */
/* 内容：	メンバー情報を取得												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

PSUBGAME_WAR_MEMBERINFO CInfoSubGameWAR::GetMemberInfo(
	int nTeamNo,		/* [in] チーム番号 */
	int nMenberNo)		/* [in] メンバー番号 */
{
	PSUBGAME_WAR_MEMBERINFO pRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pRet = NULL;

	pTeamInfo = GetTeamInfo (nTeamNo);
	if (pTeamInfo == NULL) {
		goto Exit;
	}
	if (nMenberNo >= GetMemberCount (pTeamInfo)) {
		goto Exit;
	}

	pRet = pTeamInfo->aMemberInfo[nMenberNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetMemberInfo									 */
/* 内容：	メンバー情報を取得												 */
/* 日付：	2006/03/23														 */
/* ========================================================================= */

PSUBGAME_WAR_MEMBERINFO CInfoSubGameWAR::GetMemberInfo(
	DWORD dwTblID)		/* [in] テーブルID */
{
	int i, j, nCount, nCount2;
	PSUBGAME_WAR_MEMBERINFO pRet, pMemberInfo;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	pRet = NULL;

	nCount = GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = GetTeamInfo (i);

		nCount2 = GetMemberCount (i);
		for (j = 0; j < nCount2; j ++) {
			pMemberInfo = GetMemberInfo (i, j);

			if (pMemberInfo->dwTblID == dwTblID) {
				pRet = pMemberInfo;
				goto Exit;
			}
		}
	}

Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetNewTeamInfo									 */
/* 内容：	新しいチーム情報を取得											 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

PSUBGAME_WAR_TEAMINFO CInfoSubGameWAR::GetNewTeamInfo(void)
{
	PSUBGAME_WAR_TEAMINFO pRet;

	pRet = new SUBGAME_WAR_TEAMINFO;
	ZeroMemory (&pRet->ptHP,			sizeof (pRet->ptHP));
	ZeroMemory (&pRet->ptResurrection,	sizeof (pRet->ptResurrection));
	ZeroMemory (&pRet->ptStart,			sizeof (pRet->ptStart));
	ZeroMemory (&pRet->ptCastle,		sizeof (pRet->ptCastle));
	pRet->byTurn		= 0;
	pRet->dwTurnTblID	= -1;

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetTeamInfo									 */
/* 内容：	チーム情報を取得												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

PSUBGAME_WAR_TEAMINFO CInfoSubGameWAR::GetTeamInfo(
	int nTeamNo)		/* [in] チーム番号 */
{
	PSUBGAME_WAR_TEAMINFO pRet;

	pRet = NULL;
	if (nTeamNo >= GetTeamCount ()) {
		goto Exit;
	}

	pRet = m_aTeamInfo[nTeamNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetTeamNo										 */
/* 内容：	チーム番号を取得												 */
/* 日付：	2006/03/23														 */
/* 戻り値：	-1:不参加 0:チーム1 1:チーム2									 */
/* ========================================================================= */

int CInfoSubGameWAR::GetTeamNo(DWORD dwTblID)
{
	int i, j, nCount, nCount2, nRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	nRet = -1;

	/* チーム情報分ループ */
	nCount = GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = GetTeamInfo (i);

		/* メンバー情報分ループ */
		nCount2 = GetMemberCount (i);
		for (j = 0; j < nCount2; j ++) {
			pMemberInfo = GetMemberInfo (i, j);

			if (pMemberInfo->dwTblID == dwTblID) {
				nRet = i;
				goto Exit;
			}
		}
	}

Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetMemberNo									 */
/* 内容：	メンバー番号を取得												 */
/* 日付：	2006/03/23														 */
/* 戻り値：	-1:不参加														 */
/* ========================================================================= */

int CInfoSubGameWAR::GetMemberNo(int nTeamNo, DWORD dwTblID)
{
	int i, nCount, nRet;
	PSUBGAME_WAR_MEMBERINFO pMemberInfo;

	nRet = -1;

	nCount = GetMemberCount (nTeamNo);
	for (i = 0; i < nCount; i ++) {
		pMemberInfo = GetMemberInfo (nTeamNo, i);
		if (pMemberInfo->dwTblID == dwTblID) {
			nRet = i;
			break;
		}
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetMemberCount									 */
/* 内容：	メンバー数を取得												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

int CInfoSubGameWAR::GetMemberCount(int nTeamNo)
{
	int nRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	nRet = 0;

	pTeamInfo = GetTeamInfo (nTeamNo);
	if (pTeamInfo == NULL) {
		goto Exit;
	}

	nRet = GetMemberCount (pTeamInfo);
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetMemberCount									 */
/* 内容：	メンバー数を取得												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

int CInfoSubGameWAR::GetMemberCount(PSUBGAME_WAR_TEAMINFO pTeamInfo)
{
	return pTeamInfo->aMemberInfo.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetTeamCount									 */
/* 内容：	チーム数を取得													 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

int CInfoSubGameWAR::GetTeamCount(void)
{
	return m_aTeamInfo.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetAllMemberCount								 */
/* 内容：	参加者合計を取得												 */
/* 日付：	2006/03/24														 */
/* ========================================================================= */

int CInfoSubGameWAR::GetAllMemberCount(void)
{
	int i, nCount, nRet;

	nRet	= 0;
	nCount	= GetTeamCount ();

	for (i = 0; i < nCount; i ++) {
		nRet += GetMemberCount (i);
	}

	return nRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetTurnTblID									 */
/* 内容：	行動中キャラのテーブルIDを取得									 */
/* 日付：	2006/03/24														 */
/* ========================================================================= */

DWORD CInfoSubGameWAR::GetTurnTblID(int nTeamNo)
{
	DWORD dwRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	dwRet = -1;

	pTeamInfo = GetTeamInfo (nTeamNo);
	if (pTeamInfo == NULL) {
		goto Exit;
	}

	dwRet = pTeamInfo->dwTurnTblID;
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::AddTeam										 */
/* 内容：	チームを追加													 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::AddTeam(PSUBGAME_WAR_TEAMINFO pTeamInfo)
{
	m_aTeamInfo.Add (pTeamInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteTeamInfo									 */
/* 内容：	チーム情報を削除												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteTeamInfo(PSUBGAME_WAR_TEAMINFO pTeamInfo)
{
	if (pTeamInfo == NULL) {
		return;
	}

	DeleteAllMemberInfo (pTeamInfo);
	SAFE_DELETE (pTeamInfo);
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::DeleteAllTeamInfo								 */
/* 内容：	全チーム情報を削除												 */
/* 日付：	2006/03/16														 */
/* ========================================================================= */

void CInfoSubGameWAR::DeleteAllTeamInfo(void)
{
	int i, nCount;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	nCount = GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = m_aTeamInfo[i];
		DeleteTeamInfo (pTeamInfo);
	}
	m_aTeamInfo.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::CopyMemoryRenewPos								 */
/* 内容：	コピーしてポインタを更新										 */
/* 日付：	2006/04/11														 */
/* ========================================================================= */

void CInfoSubGameWAR::CopyMemoryRenewPos(
	PVOID pDst,			/* [in] コピー先 */
	PVOID pSrc,			/* [in] コピー元 */
	DWORD dwSize,		/* [in] コピーするデータサイズ */
	PBYTE &pPos)		/* [ou] 更新するポインタ */
{
	CopyMemory (pDst, pSrc, dwSize);
	pPos += dwSize;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetSendDataSize								 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/04/11														 */
/* ========================================================================= */

DWORD CInfoSubGameWAR::GetSendDataSize(void)
{
	int i, nCount;
	DWORD dwRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	dwRet = 0;

	dwRet += sizeof (m_dwID);			/* サブゲームID */
	dwRet += sizeof (m_dwMapID);		/* マップID */
	dwRet += sizeof (m_rcArea);			/* ゲームエリア */
	dwRet += sizeof (BYTE);				/* チーム数 */
	dwRet += sizeof (m_byMember);		/* １チームの参加人数 */
	dwRet += sizeof (m_ptInformation);	/* 受付座標 */
	dwRet += sizeof (m_ptJudge);		/* 審判キャラ座標 */

	nCount = GetTeamCount ();
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = m_aTeamInfo[i];

		dwRet += sizeof (pTeamInfo->ptHP);				/* HP座標 */
		dwRet += sizeof (pTeamInfo->ptResurrection);	/* 蘇生カウンタ座標 */
		dwRet += sizeof (pTeamInfo->ptStart);			/* 開始座標 */
		dwRet += sizeof (pTeamInfo->ptCastle);			/* 城座標 */
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetSendData									 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/04/11														 */
/* 備考：	呼び出し元で開放する必要あり									 */
/* ========================================================================= */

PBYTE CInfoSubGameWAR::GetSendData(void)
{
	int i, nCount;
	BYTE byTeamCount;
	PBYTE pRet, pData;
	DWORD dwSize;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	dwSize	= GetSendDataSize ();
	pRet	= new BYTE[dwSize];
	pData	= pRet;

	nCount		= GetTeamCount ();
	byTeamCount	= nCount;

	CopyMemoryRenewPos (pData, &m_dwID,				sizeof (m_dwID),			pData);	/* サブゲームID */
	CopyMemoryRenewPos (pData, &m_dwMapID,			sizeof (m_dwMapID),			pData);	/* マップID */
	CopyMemoryRenewPos (pData, &m_rcArea,			sizeof (m_rcArea),			pData);	/* ゲームエリア */
	CopyMemoryRenewPos (pData, &byTeamCount,		sizeof (byTeamCount),		pData);	/* チーム数 */
	CopyMemoryRenewPos (pData, &m_byMember,			sizeof (m_byMember),		pData);	/* １チームの参加人数 */
	CopyMemoryRenewPos (pData, &m_ptInformation,	sizeof (m_ptInformation),	pData);	/* 受付座標 */
	CopyMemoryRenewPos (pData, &m_ptJudge,			sizeof (m_ptJudge),			pData);	/* 審判キャラ座標 */

	for (i = 0; i < nCount; i ++) {
		pTeamInfo = m_aTeamInfo[i];

		CopyMemoryRenewPos (pData, &pTeamInfo->ptHP,			sizeof (pTeamInfo->ptHP),			pData);	/* HP座標 */
		CopyMemoryRenewPos (pData, &pTeamInfo->ptResurrection,	sizeof (pTeamInfo->ptResurrection),	pData);	/* 蘇生カウンタ座標 */
		CopyMemoryRenewPos (pData, &pTeamInfo->ptStart,			sizeof (pTeamInfo->ptStart),		pData);	/* 開始座標 */
		CopyMemoryRenewPos (pData, &pTeamInfo->ptCastle,		sizeof (pTeamInfo->ptCastle),		pData);	/* 城座標 */
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::SetSendData									 */
/* 内容：	送信用データから設定											 */
/* 日付：	2006/04/11														 */
/* ========================================================================= */

DWORD CInfoSubGameWAR::SetSendData(PBYTE pSrc)
{
	int i, nCount;
	BYTE byTeamCount;
	PBYTE pSrcBack;
	DWORD dwRet;
	PSUBGAME_WAR_TEAMINFO pTeamInfo;

	DeleteAllTeamInfo ();

	pSrcBack = pSrc;

	CopyMemoryRenewPos (&m_dwID,			pSrc, sizeof (m_dwID),			pSrc);	/* サブゲームID */
	CopyMemoryRenewPos (&m_dwMapID,			pSrc, sizeof (m_dwMapID),		pSrc);	/* マップID */
	CopyMemoryRenewPos (&m_rcArea,			pSrc, sizeof (m_rcArea),		pSrc);	/* ゲームエリア */
	CopyMemoryRenewPos (&byTeamCount,		pSrc, sizeof (byTeamCount),		pSrc);	/* チーム数 */
	CopyMemoryRenewPos (&m_byMember,		pSrc, sizeof (m_byMember),		pSrc);	/* １チームの参加人数 */
	CopyMemoryRenewPos (&m_ptInformation,	pSrc, sizeof (m_ptInformation),	pSrc);	/* 受付座標 */
	CopyMemoryRenewPos (&m_ptJudge,			pSrc, sizeof (m_ptJudge),		pSrc);	/* 審判キャラ座標 */

	nCount = byTeamCount;
	for (i = 0; i < nCount; i ++) {
		pTeamInfo = GetNewTeamInfo ();

		CopyMemoryRenewPos (&pTeamInfo->ptHP,			pSrc, sizeof (pTeamInfo->ptHP),				pSrc);	/* HP座標 */
		CopyMemoryRenewPos (&pTeamInfo->ptResurrection,	pSrc, sizeof (pTeamInfo->ptResurrection),	pSrc);	/* 蘇生カウンタ座標 */
		CopyMemoryRenewPos (&pTeamInfo->ptStart,		pSrc, sizeof (pTeamInfo->ptStart),			pSrc);	/* 開始座標 */
		CopyMemoryRenewPos (&pTeamInfo->ptCastle,		pSrc, sizeof (pTeamInfo->ptCastle),			pSrc);	/* 城座標 */
		AddTeam (pTeamInfo);
	}

	dwRet = pSrc - pSrcBack;

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CInfoSubGameWAR::GetName										 */
/* 内容：	サブゲーム名を取得												 */
/* 日付：	2006/04/21														 */
/* ========================================================================= */

LPCSTR CInfoSubGameWAR::GetName(void)
{
	return "戦場ゲーム";
}

/* Copyright(C)URARA-works 2006 */
