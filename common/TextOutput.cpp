/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	TextOutput.cpp												 */
/* 内容：		テキスト出力クラス 実装ファイル								 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/04/01													 */
/* ========================================================================= */

#include <windows.h>
#include <stdio.h>
#include "TextOutput.h"


/* ========================================================================= */
/* 関数名：	CTextOutput::CTextOutput										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
CTextOutput::CTextOutput()
{
	m_pszFileName = NULL;
	m_bHeader = FALSE;
	m_bReturn = TRUE;
}


/* ========================================================================= */
/* 関数名：	CTextOutput::~CTextOutput										 */
/* 内容：	デストラクタ													 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
CTextOutput::~CTextOutput()
{
	Destroy ();
}


/* ========================================================================= */
/* 関数名：	CTextOutput::Create												 */
/* 内容：	初期化															 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
BOOL CTextOutput::Create(
	LPCSTR pszFileName,		/* [in] 出力するファイル名 */
	BOOL bHeader,			/* [in] ヘッダを書き込む？ */
	BOOL bReturn)			/* [in] 改行コードを書き込む？ */
{
	BOOL bRet;

	bRet = FALSE;

	if (m_pszFileName) {
		goto Exit;
	}

	m_bHeader = bHeader;
	m_bReturn = bReturn;

	/* クリティカルセクションオブジェクトを初期化 */
	InitializeCriticalSection (&m_csWrite);

	m_pszFileName = new char[strlen (pszFileName) + 1];
	strcpy (m_pszFileName, pszFileName);

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	CTextOutput::Destroy											 */
/* 内容：	破棄															 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
void CTextOutput::Destroy(void)
{
	if (m_pszFileName) {
		delete [] m_pszFileName;
		m_pszFileName = NULL;
	}

	DeleteCriticalSection (&m_csWrite);
}


/* ========================================================================= */
/* 関数名：	CTextOutput::Write												 */
/* 内容：	書き込み														 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
void CTextOutput::Write(
	LPCSTR format, ...)		/* [in] 出力するフォーマット付き文字列 */
{
	int nLen;
	char szTmp[256];
	va_list ap;

	/* 引数の取り出し */
	va_start (ap, format);

	nLen = _vsnprintf (szTmp, sizeof (szTmp), format, ap);
	if (nLen <= 0) {
		goto Exit;
	}

	WriteProc (szTmp);

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CTextOutput::WriteProc											 */
/* 内容：	書き込み処理													 */
/* 日付：	2005/04/01														 */
/* ========================================================================= */
void CTextOutput::WriteProc(
	LPCSTR pszText)		/* [in] 出力するNULL終端文字列 */
{
	HANDLE hFile;
	DWORD dwBytes;
	char szTmp[128];
	SYSTEMTIME stSysTime;

	/* 排他開始 */
	EnterCriticalSection (&m_csWrite);

	if ((m_pszFileName == NULL) || (pszText == NULL)) {
		goto Exit;
	}

	/* ファイルを開く */
	hFile = CreateFile (m_pszFileName, GENERIC_WRITE | GENERIC_READ, 0, 0,
					OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		goto Exit;
	}
	/* ファイルポインタを終端に移動 */
	SetFilePointer (hFile, 0, 0, FILE_END);

	/* ヘッダを書き込む？ */
	if (m_bHeader) {
		GetLocalTime (&stSysTime);
		wsprintf (szTmp, "%04d/%02d/%02d %02d:%02d:%02d:%03d\t",
				stSysTime.wYear, stSysTime.wMonth,  stSysTime.wDay,
				stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond,
				stSysTime.wMilliseconds);
		WriteFile (hFile, szTmp, strlen (szTmp), &dwBytes, NULL);
	}

	/* 本文を書き込む */
	WriteFile (hFile, pszText, strlen (pszText), &dwBytes, NULL);

	/* 改行コードを書き込む？ */
	if (m_bReturn) {
		strcpy (szTmp, "\r\n");
		WriteFile (hFile, szTmp, strlen (szTmp), &dwBytes, NULL);
	}

	CloseHandle (hFile);

Exit:
	/* 排他終了 */
	LeaveCriticalSection (&m_csWrite);
}

/* Copyright(C)URARA-works 2005 */
