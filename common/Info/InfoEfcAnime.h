/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	InfoEfcAnime.h												 */
/* 内容：		エフェクトアニメーション情報クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/01/02													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 構造体宣言																 */
/* ========================================================================= */

/* エフェクトアニメーション情報 */
typedef struct _EFCANIMEINFO {
	BYTE		byWait;												/* 待ち時間(×１０ミリ秒) */
	BYTE		byLevel;											/* 透明度 */
	WORD		wGrpID;												/* グラフィックID */
} EFCANIMEINFO, *PEFCANIMEINFO;
typedef CmyArray<PEFCANIMEINFO, PEFCANIMEINFO>	ARRAYEFCANIMEINFO;


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CInfoEfcAnime
{
public:
			CInfoEfcAnime();								/* コンストラクタ */
	virtual ~CInfoEfcAnime();								/* デストラクタ */

	int				GetCount			(void);							/* データ数を取得 */
	PEFCANIMEINFO	GetNew				(void);							/* 新規データを取得 */
	void			AddNew				(void);							/* 空のデータを追加 */
	void			Add					(PEFCANIMEINFO pSrc);			/* データを追加 */
	void			Delete				(PEFCANIMEINFO pSrc);			/* 削除 */
	void			DeleteAll			(void);							/* 全て削除 */
	PEFCANIMEINFO	GetPtr				(int nNo);						/* エフェクトアニメーション情報を取得 */
	DWORD			GetSendDataSize		(int nNo);						/* 送信用データサイズを取得 */
	DWORD			GetAllSendDataSize	(void);							/* 全送信用データサイズを取得 */
	PBYTE			GetSendData			(int nNo);						/* 送信用データを取得 */
	PBYTE			GetAllSendData		(void);							/* 全送信用データを取得 */
	PEFCANIMEINFO	SetSendData			(PBYTE pSrc, DWORD &dwSize);	/* 送信用データから設定 */
	void			Renew				(int nNo, PEFCANIMEINFO pSrc);	/* 更新 */



private:
	ARRAYEFCANIMEINFO	m_aEfcAnimeInfo;					/* エフェクトアニメーション情報 */
} CInfoEfcAnime, *PCInfoEfcAnime;

/* Copyright(C)URARA-works 2006 */
