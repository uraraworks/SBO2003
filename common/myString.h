/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	myString.h													 */
/* 内容：		文字列クラス 定義ファイル									 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/05/25													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CmyString
{
public:
			CmyString();							/* コンストラクタ */
	virtual ~CmyString();							/* デストラクタ */

	void	Empty			(void);							/* 文字列を空にする */
	BOOL	IsEmpty			(void);							/* 文字列が空か判定 */
	int		GetLength		(void);							/* 文字列長を取得 */
	void	Format			(LPCSTR lpFormat, ...);			/* 書式文字列で初期化 */

	void	operator =		(CmyString &strSrc);			/* 文字列を初期化 */
	void	operator =		(LPCSTR pszSrc);				/* 文字列を初期化 */
	void	operator +=		(LPCSTR pszSrc);				/* 文字列を追加 */
	BOOL	operator ==		(LPCSTR pszSrc);				/* 文字列を比較 */
			operator LPCSTR	();								/* キャスト */


protected:
	void	Renew	(LPCSTR pszSrc);						/* 文字列を更新 */


protected:
	LPSTR	m_pszString;						/* 文字列データ */
} CmyString, *PCmyString;

/* Copyright(C)URARA-works 2006 */
