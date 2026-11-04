/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboCliMagicExplain.h										 */
/* 内容：		特技（魔法）の説明文 定義ファイル							 */
/* ========================================================================= */

#pragma once

/* 説明文を取得（管理者が書いた説明文があればそれ、無ければ数値から作る） */
extern void GetMagicExplain			(PMAGICINFO pInfo, LPSTR pszDst, int nSize);
/* 数値から説明文を作る（管理者用ウィンドウの「自動作成」でも使う） */
extern void MakeMagicExplainAuto	(PMAGICINFO pInfo, LPSTR pszDst, int nSize);
/* 説明文を決まった幅で折り返して行に分ける（全角の途中では切らない。改行もそのまま改行） */
extern int	SplitMagicExplain		(LPCSTR pszSrc, int nLineBytes, char aszDst[][128], int nMaxLine);
