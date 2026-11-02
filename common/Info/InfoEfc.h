/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoEfc.h													 */
/* 内容：		エフェクト情報クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/02													 */
/* ========================================================================= */

#pragma once

#include "InfoEfcAnime.h"

/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* エフェクト情報 */
typedef struct _EFCINFO {
	BYTE			byType;								/* グラフィック種別 */
	BYTE			bySound;							/* 効果音ID */
	CInfoEfcAnime	aAnime;								/* アニメーション情報 */
} EFCINFO, *PEFCINFO;
typedef CmyArray<PEFCINFO, PEFCINFO>	ARRAYEFCINFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoEfc
{
public:
			CInfoEfc();											/* コンストラクタ */
	virtual ~CInfoEfc();										/* デストラクタ */

	int				GetCount			(void);							/* データ数を取得 */
	int				GetCountAnime		(int nNo);						/* アニメーション情報データ数を取得 */
	PEFCINFO		GetNew				(void);							/* 新規データを取得 */
	void			AddNew				(void);							/* 空のデータを追加 */
	void			AddNewAnime			(int nNo);						/* 空のアニメーション情報を追加 */
	void			Add					(PEFCINFO pSrc);				/* データを追加 */
	void			Delete				(PEFCINFO pSrc);				/* 削除 */
	void			DeleteAll			(void);							/* 全て削除 */
	void			DeleteAllAnime		(int nNo);						/* アニメーション情報を全て削除 */
	PEFCINFO		GetPtr				(int nNo);						/* エフェクト情報を取得 */
	PEFCANIMEINFO	GetPtrAnime			(int nNo, int nNoAnime);		/* アニメーション情報を取得 */
	DWORD			GetSendDataSize		(int nNo);						/* 送信用データサイズを取得 */
	DWORD			GetAllSendDataSize	(void);							/* 全送信用データサイズを取得 */
	PBYTE			GetSendData			(int nNo);						/* 送信用データを取得 */
	PBYTE			GetAllSendData		(void);							/* 全送信用データを取得 */
	PEFCINFO		SetSendData			(PBYTE pSrc, DWORD &dwSize);	/* 送信用データから設定 */
	void			SetAllSendData		(PBYTE pSrc);					/* 全送信用データから設定 */
	void			Renew				(int nNo, PEFCINFO pSrc);		/* 更新 */


private:
	ARRAYEFCINFO		m_aEfcInfo;						/* エフェクト情報 */
} CInfoEfc, *PCInfoEfc;

/* Copyright(C)URARA-works 2006 */
