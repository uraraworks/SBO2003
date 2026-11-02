/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	LibMusicLoader.h											 */
/* 内容：		AflMusicライブラリ読み込みクラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/10/18													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数ポインタ定義															 */
/* ========================================================================= */

typedef bool		(*LIBMUSICOpen1)(LPCSTR, LPCSTR);
typedef bool		(*LIBMUSICOpen2)(HMODULE, HRSRC, LPCSTR);
typedef bool		(*LIBMUSICClose)();
typedef bool		(*LIBMUSICFadeIn)(INT,INT);
typedef bool		(*LIBMUSICFadeOut)(INT);
typedef bool		(*LIBMUSICPlayTime)(DWORD);
typedef bool		(*LIBMUSICPlay1)(DWORDLONG);
typedef bool		(*LIBMUSICPlay2)();
typedef bool		(*LIBMUSICCont)();
typedef bool		(*LIBMUSICStop)();
typedef bool		(*LIBMUSICIsPlay)();
typedef void		(*LIBMUSICSetLoop)(bool);
typedef void		(*LIBMUSICSetRelativeVolume)(INT);
typedef LPCSTR		(*LIBMUSICGetTitle)();
typedef DWORDLONG	(*LIBMUSICGetAllTime)();
typedef DWORDLONG	(*LIBMUSICGetPlayTime)();


/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CLibMusicLoader
{
public:
			CLibMusicLoader();					/* コンストラクタ */
	virtual	~CLibMusicLoader();					/* デストラクタ */

	void	Load (void);						/* 読み込み */
	void	Free (void);						/* 開放 */

	BOOL		Open				(LPCSTR pFileName, LPCSTR pType = NULL);				/* ファイルからMIDIデータを読み込む */
	BOOL		Open				(HMODULE hResource, HRSRC hSrc, LPCSTR pType = NULL);	/* リソースからMIDIデータを読み込む */
	BOOL		Close				(void);													/* データをメモリから開放 */
	BOOL		FadeIn				(INT nTime, INT nVolume = -70);							/* フェードインを行う */
	BOOL		FadeOut				(INT nTime);											/* フェードアウトを行う */
	BOOL		PlayTime			(DWORD dwTime);											/* 指定時間からから演奏開始 */
	BOOL		Play				(DWORDLONG dwlCount);									/* 指定位置から演奏開始 */
	BOOL		Play				(void);													/* 演奏開始 */
	BOOL		Cont				(void);													/* 演奏再開 */
	BOOL		Stop				(void);													/* 演奏停止 */
	BOOL		IsPlay				(void);													/* 演奏中か判定 */
	void		SetLoop				(BOOL bLoop);											/* ループさせるか指定 */
	void		SetRelativeVolume	(INT nVolume);											/* ボリュームの設定 */
	LPCSTR		GetTitle			(void);													/* タイトル名取得 */
	DWORDLONG	GetAllTime			(void);													/* 総演奏時間を取得 */
	DWORDLONG	GetPlayTime			(void);													/* 演奏時間を取得 */


protected:
	HMODULE				m_hLib;		/* DLLのモジュールハンドル */

	/* 以下関数ポインタ */
	LIBMUSICOpen1				m_pOpen1;
	LIBMUSICOpen2				m_pOpen2;
	LIBMUSICClose				m_pClose;
	LIBMUSICFadeIn				m_pFadeIn;
	LIBMUSICFadeOut				m_pFadeOut;
	LIBMUSICPlayTime			m_pPlayTime;
	LIBMUSICPlay1				m_pPlay1;
	LIBMUSICPlay2				m_pPlay2;
	LIBMUSICCont				m_pCont;
	LIBMUSICStop				m_pStop;
	LIBMUSICIsPlay				m_pIsPlay;
	LIBMUSICSetLoop				m_pSetLoop;
	LIBMUSICSetRelativeVolume	m_pSetRelativeVolume;
	LIBMUSICGetTitle			m_pGetTitle;
	LIBMUSICGetAllTime			m_pGetAllTime;
	LIBMUSICGetPlayTime			m_pGetPlayTime;
} CLibMusicLoader, *PCLibMusicLoader;

/* Copyright(C)URARA-works 2005 */
