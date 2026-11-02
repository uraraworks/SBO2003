/* ========================================================================= */
/* ファイル名：	DlgModal.h													 */
/* 内容：		モーダルダイアログを出して、閉じたら知らせを受ける			 */
/* ========================================================================= */
/*
 * DOMODAL_THEN(pDlg, pThis, pfnEnd, lParam)
 *   new したダイアログ pDlg をモーダルで出し、閉じたら
 *   pThis->pfnEnd(pDlg, nResult, lParam) を呼んでから pDlg を delete する。
 *
 * Windows 版は DoModal で閉じるまで待ってから呼ぶ。ブラウザ版は待てないので、
 * 閉じた時に互換層（DoModalAsync）から呼ばれる。どちらも呼ばれる順番は同じ。
 */

#pragma once

#ifdef __EMSCRIPTEN__
#define DOMODAL_THEN(pDlg, pThis, pfnEnd, lParam) \
	(pDlg)->DoModalAsync ((pThis), (AFX_PMODALEND)(pfnEnd), (LPARAM)(lParam))
#else
#define DOMODAL_THEN(pDlg, pThis, pfnEnd, lParam) \
	{ \
		int _nResult = (pDlg)->DoModal (); \
		((pThis)->*(pfnEnd)) ((pDlg), _nResult, (LPARAM)(lParam)); \
		delete (pDlg); \
	}
#endif
