/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	CDXAudio.h													 */
/* 内容：		DirectX Audioを使うためのクラス								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/03/16													 */
/* ========================================================================= */
#include <dmusici.h>

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */
class CDXAudio
{
public:
	CDXAudio(void);										/* コンストラクタ */
	~CDXAudio(void);									/* デストラクタ */

	BOOL Create(void);									/* 初期化 */
	void Destroy(void);									/* 終了 */
	void SetResourceHandle(HMODULE);					/* リソースハンドルの設定 */

	BOOL GetSegFromRes(HRSRC, IDirectMusicSegment8 **, BOOL bMidi = FALSE);	/* リソースからのセグメント読み込み */
	void ReleaseSeg(IDirectMusicSegment8 **);			/* セグメントの開放 */

	BOOL PlayPrimary(IDirectMusicSegment8 **);			/* プライマリセグメントとして再生 */
	BOOL PlaySecoundary(IDirectMusicSegment8 **, DWORD dwDelayMs = 0);	/* セカンダリセグメントとして再生（dwDelayMs 後に鳴らす） */
	void SetVolPrimary(long);							/* プライマリセグメントの音量設定 */
	void SetVolSecoundary(long);						/* セカンダリセグメントの音量設定 */
	void Stop(IDirectMusicSegment8 **, DWORD);			/* 再生停止 */
	void SetLoopPoints(IDirectMusicSegment8 **, DWORD);	/* ループ範囲の設定 */
	BOOL IsPlaying(IDirectMusicSegment8 **);			/* 再生中判定 */

private:
	IDirectMusicPerformance8		*m_pPerformance;	/* パフォーマンスオブジェクト */
	IDirectMusicAudioPath8			*m_pDefAudioPath;	/* オーディオパス */
	IDirectMusicAudioPath8			*m_pDefAudioPath2;	/* オーディオパス2 */
	IDirectMusicLoader8				*m_pLoader;			/* ローダー */
	HMODULE							m_hResource;		/* リソースハンドル */
};

/* Copyright(C)URARA-works 2003-2005 */
