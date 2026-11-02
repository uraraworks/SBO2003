/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	Img32.h														 */
/* 内容：		32bitイメージクラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/01/24													 */
/* ========================================================================= */

#ifndef CIMG32_H
#define CIMG32_H

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CImg32
{
public:
			CImg32(void);										/* コンストラクタ */
	virtual ~CImg32();											/* デストラクタ */

	BOOL	Create				(int cx, int cy);										/* ビットマップを作成 */
	BOOL	CreateWithoutGdi	(int cx, int cy, LPBYTE pBits = NULL);					/* GDI非互換で作成 */
	void	Destroy				(void);													/* 破棄 */

	void	Clear				(void);													/* クリア */
	void	FillRect			(int x, int y, int cx, int cy, COLORREF clFill);		/* 矩形塗りつぶし */
	void	ChgLevel			(int x, int y, int cx, int cy, BYTE byLevel);			/* 明度を変更 */
	void	ChgColor			(int x, int y, int cx, int cy, COLORREF clChg);			/* 色素を変更 */

	void	Blt					(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BOOL bColorKey = FALSE);						/* 矩形転送 */
	void	BltLevel			(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BYTE byLevel, BOOL bColorKey = FALSE);		/* 明度を変えながら矩形転送 */
	void	BltAlpha			(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BYTE byLevel, BOOL bColorKey = FALSE);		/* 混ぜ合わせ矩形転送 */
	void	BltAlpha2			(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BYTE byLevelAlpha, BOOL bColorKey = FALSE);	/* カラーキーのみ混ぜ合わせ矩形転送 */
	void	BltTurnRightAndLeft	(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BOOL bColorKey = FALSE);						/* 左右反転して矩形転送 */
	void	BltTurnUpAndDown(int dx, int dy, int cx, int cy, CImg32 *pSrc, int sx, int sy, BOOL bColorKey = FALSE);							/* 上下反転して矩形転送 */	

	HDC		Lock				(void);									/* 画像をロック */
	void	Unlock				(void);									/* ロック解除 */

	HBITMAP	GetSafeHandle		(void);									/* ビットマップハンドルを取得 */
	int		Width				(void);									/* 幅を取得 */
	int		Height				(void);									/* 高さを取得 */
	int		GetImageSize		(void);									/* データサイズを取得 */
	LPBYTE	GetBits				(void);									/* ビットデータのアドレスを取得 */
	void	SetColorKey			(COLORREF clKey);						/* カラーキーを設定 */
	COLORREF GetColorKey		(void);									/* カラーキーを取得 */


protected:
	void	InitData			(void);									/* データを初期化 */
	DWORD	ColorrefToDword		(COLORREF cl);							/* 色素をDWORD値に変換 */
	BYTE	PercentToHex		(BYTE byPercent);						/* パーセントを255MAXの16進数に変換 */
	void	ClipPos				(int &dx, int &dy, int &cx, int &cy, CImg32 *pSrc, int &sx, int &sy);	/* 座標をクリッピング */


protected:
	HBITMAP			m_hBmp;								/* ビットマップハンドル */
	HBITMAP			m_hBmpBack;							/* ビットマップハンドル保存用 */
	BITMAPINFO		m_bmpInfo;							/* ビットマップ情報 */
	LPBYTE			m_pBits;							/* ビットデータ */
	HDC				m_hDC;								/* デバイスコンテキストハンドル */
	DWORD			m_dwColorKey;						/* カラーキー */
} CImg32, *PCImg32;

#endif
/* Copyright(C)URARA-works 2005 */
