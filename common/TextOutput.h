/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	TextOutput.h												 */
/* 内容：		テキスト出力クラス 定義ファイル								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/04/01													 */
/* ========================================================================= */

#ifndef _TEXTOUTPUT_H
#define _TEXTOUTPUT_H

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */
class CTextOutput
{
public:
	CTextOutput();													/* コンストラクタ */
	virtual ~CTextOutput();											/* デストラクタ */

	BOOL Create(LPCSTR pszFileName, BOOL bHeader, BOOL m_bReturn);	/* 初期化 */
	void Destroy(void);												/* 破棄 */
	void Write(LPCSTR format, ...);									/* 書き込み */


protected:
	void WriteProc(LPCSTR pszText);									/* 書き込み処理 */


protected:
	LPSTR				m_pszFileName;			/* ファイル名 */
	BOOL				m_bHeader;				/* ヘッダを書き込む */
	BOOL				m_bReturn;				/* 改行コードを書き込む */
	CRITICAL_SECTION	m_csWrite;				/* 書き込み排他用 */
};

#endif
/* Copyright(C)URARA-works 2005 */
