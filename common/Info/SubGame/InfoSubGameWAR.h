/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoSubGameWAR.h											 */
/* 内容：		サブゲーム情報(戦場ゲーム)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/14													 */
/* ========================================================================= */

#pragma once

#include "InfoSubGameBase.h"

/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* サブゲーム：戦場ゲーム情報(メンバー情報) */
typedef struct _SUBGAME_WAR_MEMBERINFO {
	BYTE		byJob;								/* 職業 */
	DWORD		dwTblID;							/* テーブルID */
	DWORD		dwTblIDHP;							/* HPのテーブルID */
	DWORD		dwTblIDRes;							/* 蘇生カウンタのテーブルID */
	DWORD		dwTblIDTargetRes;					/* やられ中キャラのテーブルID */
} SUBGAME_WAR_MEMBERINFO, *PSUBGAME_WAR_MEMBERINFO;
typedef	CmyArray<PSUBGAME_WAR_MEMBERINFO, PSUBGAME_WAR_MEMBERINFO>	  ARRAYWARMEMBERINFO;
typedef	CmyArray<PSUBGAME_WAR_MEMBERINFO, PSUBGAME_WAR_MEMBERINFO>	*PARRAYWARMEMBERINFO;

/* サブゲーム：戦場ゲーム情報(チーム情報) */
typedef struct _SUBGAME_WAR_TEAMINFO {
	POINT		ptHP;								/* HP座標 */
	POINT		ptResurrection;						/* 蘇生カウンタ座標 */
	POINT		ptStart;							/* 開始座標 */
	POINT		ptCastle;							/* 城座標 */
	BYTE		byTurn;								/* 行動キャラ */
	DWORD		dwTurnTblID;						/* 行動キャラのテーブルID */
	ARRAYWARMEMBERINFO	aMemberInfo;				/* メンバー情報 */
} SUBGAME_WAR_TEAMINFO, *PSUBGAME_WAR_TEAMINFO;
typedef	CmyArray<PSUBGAME_WAR_TEAMINFO, PSUBGAME_WAR_TEAMINFO>	  ARRAYWARTEAMINFO;
typedef	CmyArray<PSUBGAME_WAR_TEAMINFO, PSUBGAME_WAR_TEAMINFO>	*PARRAYWARTEAMINFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoSubGameWAR : public CInfoSubGameBase
{
public:
			CInfoSubGameWAR();												/* コンストラクタ */
	virtual ~CInfoSubGameWAR();												/* デストラクタ */

	void Init					(void);												/* ゲーム情報を初期化 */
	void InitTeamInfo			(void);												/* チーム情報を初期化 */

	void AddMember				(int nTeamNo, PSUBGAME_WAR_MEMBERINFO pMemberInfo);	/* メンバーを追加 */
	void DeleteMember			(DWORD dwTblID);									/* メンバーを削除 */
	void DeleteMemberInfo		(int nTeamNo, int nMenberNo);						/* メンバー情報を削除 */
	void DeleteMemberInfo		(PSUBGAME_WAR_MEMBERINFO pMemberInfo);				/* メンバー情報を削除 */
	void DeleteAllMemberInfo	(PSUBGAME_WAR_TEAMINFO pTeamInfo);					/* 全メンバー情報を削除 */
	void DeleteAllMemberInfo	(int nTeamNo);										/* 全メンバー情報を削除 */

	PSUBGAME_WAR_MEMBERINFO	GetNewMemberInfo	(void);								/* 新しいメンバー情報を取得 */
	PSUBGAME_WAR_MEMBERINFO	GetMemberInfo		(int nTeamNo, int nMemberNo);		/* メンバー情報を取得 */
	PSUBGAME_WAR_MEMBERINFO	GetMemberInfo		(DWORD dwTblID);					/* メンバー情報を取得 */
	PSUBGAME_WAR_TEAMINFO	GetNewTeamInfo		(void);								/* 新しいチーム情報を取得 */
	PSUBGAME_WAR_TEAMINFO	GetTeamInfo			(int nTeamNo);						/* チーム情報を取得 */

	int GetTeamNo				(DWORD dwTblID);									/* チーム番号を取得 */
	int GetMemberNo				(int nTeamNo, DWORD dwTblID);						/* メンバー番号を取得 */
	int GetMemberCount			(int nTeamNo);										/* メンバー数を取得 */
	int GetMemberCount			(PSUBGAME_WAR_TEAMINFO pTeamInfo);					/* メンバー数を取得 */
	int GetTeamCount			(void);												/* チーム数を取得 */
	int GetAllMemberCount		(void);												/* 参加者合計を取得 */

	DWORD GetTurnTblID			(int nTeamNo);										/* 行動中キャラのテーブルIDを取得 */

	void AddTeam				(PSUBGAME_WAR_TEAMINFO pTeamInfo);					/* チームを追加 */
	void DeleteTeamInfo			(PSUBGAME_WAR_TEAMINFO pTeamInfo);					/* チーム情報を削除 */
	void DeleteAllTeamInfo		(void);												/* 全チーム情報を削除 */

	void	CopyMemoryRenewPos	(PVOID pDst, PVOID pSrc, DWORD dwSize, PBYTE &pPos);/* コピーしてポインタを更新 */
	DWORD	GetSendDataSize		(void);												/* 送信用データサイズを取得 */
	PBYTE	GetSendData			(void);												/* 送信用データを取得 */
	DWORD	SetSendData			(PBYTE pSrc);										/* 送信用データから設定 */
	LPCSTR	GetName				(void);												/* サブゲーム名を取得 */


public:
	BYTE		m_byMember;							/* １チームの参加人数 */
	BYTE		m_byRule;							/* ルール */
	BYTE		m_byState;							/* ゲーム状態 */
	BYTE		m_byAllMember;						/* 現在の参加人数 */
	BYTE		m_byTurn;							/* 行動しているチーム */
	DWORD		m_dwWaitTblID;						/* 応答待ちキャラのテーブルID */
	DWORD		m_dwFirstTimeProc;					/* 最初に処理した時間 */
	DWORD		m_dwLastTimeProc;					/* 最後に処理した時間 */
	DWORD		m_dwNextTimeProc;					/* 次の処理待ち時間 */
	DWORD		m_dwTblIDJudge;						/* 審判キャラのテーブルID */

	POINT		m_ptInformation;					/* 受付座標 */
	POINT		m_ptJudge;							/* 審判キャラ座標 */

	ARRAYWARTEAMINFO	m_aTeamInfo;				/* チーム情報 */
} CInfoSubGameWAR, *PCInfoSubGameWAR;

/* Copyright(C)URARA-works 2006 */
