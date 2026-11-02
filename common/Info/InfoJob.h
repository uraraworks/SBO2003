/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoJob.h													 */
/* 内容：		職業情報クラス 定義ファイル									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/02/01													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 定数宣言																	 */
/* ========================================================================= */

#define JOBINFOFLG_VIEWITEMINFO			(0x0001)			/* アイテム情報が見れる */


/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* レベルアップ情報 */
typedef struct _LVUPINFO {
	BYTE		byPW;									/* 最大値(力) */
	BYTE		byDEX;									/* 最大値(素早さ) */
	BYTE		byST;									/* 最大値(体力) */
	BYTE		byIT;									/* 最大値(賢さ) */
	BYTE		byLK;									/* 最大値(運のよさ) */
	WORD		wMAXHP;									/* 最大値(最大HP) */
	WORD		wMAXMANA;								/* 最大値(最大Mana) */
	DWORD		dwExp;									/* レベルアップに必要な経験値 */
} LVUPINFO, *PLVUPINFO;
typedef CmyArray<PLVUPINFO, PLVUPINFO>	   CMYARRAYLVUPINFO;
typedef CmyArray<PLVUPINFO, PLVUPINFO>	*LPCMYARRAYLVUPINFO;

/* 職業情報 */
typedef struct _JOBINFO {
	BYTE				bySkillType;					/* 特技種別 */
	BYTE				byMagicType;					/* 魔法種別 */
	BYTE				byEquType;						/* 装備種別 */
	DWORD				dwJobID;						/* 職業ID */
	DWORD				dwFlg;							/* フラグ */
	LPSTR				pszName;						/* 名前 */
	CMYARRAYLVUPINFO	aLevelUpInfo;					/* レベルアップ情報 */
} JOBINFO, *PJOBINFO;
typedef CmyArray<PJOBINFO, PJOBINFO>	  ARRAYJOBINFO;
typedef CmyArray<PJOBINFO, PJOBINFO>	*PARRAYJOBINFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoJob
{
public:
			CInfoJob();											/* コンストラクタ */
	virtual ~CInfoJob();										/* デストラクタ */

	int			GetCount			(void);								/* データ数を取得 */
	int			GetNo				(DWORD dwJobID);					/* 配列番号を取得 */
	PJOBINFO	GetNew				(void);								/* 新規データを取得 */
	PLVUPINFO	GetNewLvUpInfo		(void);								/* 新規データを取得(レベルアップ情報) */
	DWORD		GetNewID			(void);								/* 新しい職業IDを取得 */
	DWORD		Add					(PJOBINFO pSrc);					/* データを追加 */
	void		Set					(PJOBINFO pSrc);					/* 既存データを更新 */
	void		Delete				(PJOBINFO pSrc);					/* 削除 */
	void		DeleteAll			(void);								/* 全て削除 */
	void		DeleteLvUpInfo		(PLVUPINFO pSrc);					/* 削除(レベルアップ情報) */
	void		DeleteAllLvUpInfo	(LPCMYARRAYLVUPINFO pSrc);			/* 全て削除(レベルアップ情報) */
	void		DeleteAllLvUpInfo	(PJOBINFO pSrc);					/* 全て削除(レベルアップ情報) */
	PJOBINFO	GetPtr				(int nNo);							/* 職業情報を取得 */
	PJOBINFO	GetPtr				(DWORD dwJobID);					/* 職業情報を取得 */
	void		SetJobName			(PJOBINFO pDst, LPCSTR pszJobName);	/* 職業名を設定 */
	void		Copy				(PJOBINFO pDst, PJOBINFO pSrc);		/* コピー */
	void		CopyLvUpInfo		(PLVUPINFO pDst, PLVUPINFO pSrc);					/* コピー(レベルアップ情報) */
	void		CopyLvUpInfo		(LPCMYARRAYLVUPINFO pDst, LPCMYARRAYLVUPINFO pSrc);	/* コピー(レベルアップ情報) */

	DWORD		GetSendDataSize		(int nNo);							/* 送信用データサイズを取得 */
	DWORD		GetSendDataSize		(DWORD dwJobID);					/* 送信用データサイズを取得 */
	DWORD		GetAllSendDataSize	(void);								/* 全送信用データサイズを取得 */
	PBYTE		GetSendData			(int nNo);							/* 送信用データを取得 */
	PBYTE		GetSendData			(DWORD dwJobID);					/* 送信用データを取得 */
	PBYTE		GetAllSendData		(void);								/* 全送信用データを取得 */
	PJOBINFO	SetSendData			(PBYTE pSrc, DWORD &dwSize);		/* 送信用データから設定 */


private:
	ARRAYJOBINFO	m_aJobInfo;					/* 職業情報 */
} CInfoJob, *PCInfoJob;

/* Copyright(C)URARA-works 2006 */
