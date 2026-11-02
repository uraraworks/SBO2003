/* ========================================================================= */
/* SboCliWebAccount_web.cpp													 */
/* ブラウザ版のアカウント関係（端末トークンでのログイン）					 */
/* ========================================================================= */
/*
 * ID・パスワードでログインできたら、サーバーに端末トークンを発行してもらい
 * localStorage（sbo_device_token）に置く。次からはシェル HTML のログイン画面に
 * 「○○ ではじめる」ボタンが出て、押すと SboWebTokenLogin でそのトークンを使って
 * ログインする（パスワードはブラウザに残さない）。
 *
 *  - SboWebTokenLogin   … シェル HTML のボタンから。トークンを覚えて接続する
 *  - SboWebNewLogin     … 「すぐにはじめる」から。ページが作ったアカウント名・パスワードで接続する
 *                         （知らないアカウント名なのでサーバーが新しく作る）
 *  - SboWebLoginState   … ログイン画面の入力を受け付けているか（ボタンの出し入れ用）
 *  - WebAccSaveToken    … サーバーから届いたトークンをページへ渡す
 *  - WebAccOnTokenNG    … トークンが無効だった時にページへ知らせる
 */
#include "StdAfx.h"
#include "Glbldata.h"

#include <emscripten.h>
#include <string.h>

/* 端末トークンでログインする時のトークン（空なら ID・パスワードでログイン） */
char g_szWebToken[MAX_WEBTOKEN + 1];

EM_JS(void, SboJsSaveDeviceToken, (const char *pszToken), {
	if (typeof window !== 'undefined' && typeof window['sboOnDeviceToken'] === 'function') {
		window['sboOnDeviceToken'](UTF8ToString(pszToken));
	}
});

EM_JS(void, SboJsDeviceTokenNG, (void), {
	if (typeof window !== 'undefined' && typeof window['sboOnDeviceTokenNG'] === 'function') {
		window['sboOnDeviceTokenNG']();
	}
});

/* ========================================================================= */
/* 関数名：WebAccSaveToken													 */
/* 内容：サーバーから届いた端末トークンをページへ渡す						 */
/* ========================================================================= */
void WebAccSaveToken(LPCSTR pszToken)
{
	SboJsSaveDeviceToken(pszToken);
}

/* ========================================================================= */
/* 関数名：WebAccOnTokenNG													 */
/* 内容：端末トークンが無効だったことをページへ知らせる						 */
/* ========================================================================= */
void WebAccOnTokenNG(void)
{
	ZeroMemory(g_szWebToken, sizeof(g_szWebToken));
	SboJsDeviceTokenNG();
}

/* ========================================================================= */
/* 関数名：SboWebLoginState													 */
/* 内容：ログイン画面の状態（0:ログイン画面ではない 1:入力待ち 2:接続中）	 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebLoginState(void)
{
	if (g_byGameState != GAMESTATE_LOGIN || g_dwMsgIDLogin == 0) {
		return 0;
	}
	/* 接続できた後もログイン応答が届くまでは接続ボタンが押せないままなので、
	   それも「接続中」として扱う（ボタンが一度消えてまた出るのを防ぐ） */
	if (g_dwMsgIDConnect || (g_hWndCon && !IsWindowEnabled (g_hWndCon))) {
		return 2;
	}
	return 1;
}

/* ========================================================================= */
/* 関数名：SboWebTokenLogin													 */
/* 内容：端末トークンでログインする（ログイン画面のボタンから）				 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE void SboWebTokenLogin(const char *pszToken)
{
	if (pszToken == NULL || strlen(pszToken) != MAX_WEBTOKEN || SboWebLoginState() != 1) {
		return;
	}
	ZeroMemory(g_szWebToken, sizeof(g_szWebToken));
	strcpy(g_szWebToken, pszToken);
	PostMessage(g_hWndMain, WM_COMMAND, IDC_CONNECTTOKEN, 0);
}

/* ========================================================================= */
/* 関数名：SboWebNewLogin													 */
/* 内容：自動で作ったアカウントでログインする（「すぐにはじめる」から）		 */
/*       アカウント名は小文字英数字、パスワードは ASCII の 0x21〜0x7E のみ	 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE void SboWebNewLogin(const char *pszAcc, const char *pszPas)
{
	size_t i, nAcc, nPas;

	if (pszAcc == NULL || pszPas == NULL || SboWebLoginState() != 1) {
		return;
	}
	nAcc = strlen(pszAcc);
	nPas = strlen(pszPas);
	if (nAcc == 0 || nAcc >= MAX_ACCNAME || nPas == 0 || nPas >= MAX_ACCNAME) {
		return;
	}
	for (i = 0; i < nAcc; i ++) {
		if (!((pszAcc[i] >= 'a' && pszAcc[i] <= 'z') || (pszAcc[i] >= '0' && pszAcc[i] <= '9'))) {
			return;
		}
	}
	for (i = 0; i < nPas; i ++) {
		if ((unsigned char)pszPas[i] < 0x21 || (unsigned char)pszPas[i] > 0x7E) {
			return;
		}
	}
	ZeroMemory(g_szWebToken, sizeof(g_szWebToken));
	ZeroMemory(g_szAcc, sizeof(g_szAcc));
	ZeroMemory(g_szPas, sizeof(g_szPas));
	strcpy(g_szAcc, pszAcc);
	strcpy(g_szPas, pszPas);
	PostMessage(g_hWndMain, WM_COMMAND, IDC_CONNECTNEW, 0);
}
