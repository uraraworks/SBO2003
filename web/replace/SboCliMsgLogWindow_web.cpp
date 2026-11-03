/* ========================================================================= */
/* SboCliMsgLogWindow_web.cpp												 */
/* メッセージログウィンドウ（Web版）：ページの HTML のチャット欄で代用する	 */
/* ========================================================================= */
/*
 * Windows 版はリッチエディットの小窓にログを出し、下の入力欄から発言する。
 * ブラウザ版では小窓を作らず、シェル HTML のチャット欄（PC は画面横のパネル、
 * スマホは画面下のログ欄＋入力用のシート）に出す。
 *
 *  - AddMsgLog   … 時刻と色付きの 1 行を window.sboChatLogAdd へ渡す（UTF-8）
 *  - 発言        … チャット欄の入力を SboWebChatSubmit で受け取り ChatMsgProc へ
 *  - SaveMsgLog  … メニューの「ログ保存」で window.sboChatLogSave を呼び、
 *                  チャット欄のログをテキストファイル（UTF-8）としてダウンロードする
 */
#include "StdAfx.h"
#include "Glbldata.h"
#include "SboCliKeyProc.h"

#include <emscripten.h>
#include <string>

/* 互換層（platform/w32_sjis.cpp）の文字コード変換 */
std::string W32SjisToUtf8Str(const char *pszSjis, int nLen);
std::string W32Utf8ToSjisStr(const char *pszUtf8);

/* 発言できる長さ（Windows 版の入力欄と同じ 40 バイト） */
#define MSGLOG_SPEECH_MAX	40

EM_JS(void, SboJsChatLogAdd, (const char *pszTime, const char *pszText, int nColor), {
	if (typeof window === 'undefined' || typeof window['sboChatLogAdd'] !== 'function') {
		return;
	}
	var r = nColor & 0xFF, g = (nColor >> 8) & 0xFF, b = (nColor >> 16) & 0xFF;
	var color = 'rgb(' + r + ',' + g + ',' + b + ')';
	window['sboChatLogAdd'](UTF8ToString(pszTime), UTF8ToString(pszText), color);
});

/* 仮想パッドを出しているか（スマホなど） */
EM_JS(int, SboJsIsPadMode, (void), {
	return (typeof document !== 'undefined' && document.body.classList.contains('sbo-pad')) ? 1 : 0;
});

EM_JS(void, SboJsChatLogSave, (void), {
	if (typeof window !== 'undefined' && typeof window['sboChatLogSave'] === 'function') {
		window['sboChatLogSave']();
	}
});

EM_JS(void, SboJsChatLogOpen, (void), {
	if (typeof window !== 'undefined' && typeof window['sboChatLogOpen'] === 'function') {
		window['sboChatLogOpen']();
	}
});

/* ========================================================================= */
/* 関数名：MakeMsgLogWindow													 */
/* 内容：メッセージログウィンドウ作成（チャット欄を出す）					 */
/* ========================================================================= */
void MakeMsgLogWindow(void)
{
	SboJsChatLogOpen ();
	/* スマホではゲーム画面に重なってしまうので、吹き出しウィンドウは最初は出さない
	   （このあと MakeBalloonWindow が呼ばれる。オプションから出すこともできる） */
	if (SboJsIsPadMode ()) {
		g_bBalloonWindowView = FALSE;
	}
}

/* ========================================================================= */
/* 関数名：AddMsgLog														 */
/* 内容：メッセージログ追加													 */
/* ========================================================================= */
void AddMsgLog(LPSTR lpStr, COLORREF cl)
{
	SYSTEMTIME sysTime;
	char szTime[32];

	if (lpStr == NULL) {
		return;
	}
	GetLocalTime (&sysTime);
	wsprintf (szTime, "[%02d:%02d:%02d]", sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	if (cl == 0) {
		cl = RGB (255, 255, 255);
	}
	std::string strText = W32SjisToUtf8Str (lpStr, -1);
	SboJsChatLogAdd (szTime, strText.c_str (), (int)cl);
}

/* ========================================================================= */
/* 関数名：SaveMsgLog														 */
/* 内容：メッセージログ保存（テキストファイルとしてダウンロード）			 */
/* ========================================================================= */
void SaveMsgLog(void)
{
	/* Windows 版は切断時にも自動で保存していたが、ブラウザで勝手に
	   ダウンロードが始まると驚くので、メニューから選んだ時だけにする */
	if (g_byGameState == GAMESTATE_CONNECTLOST) {
		return;
	}
	SboJsChatLogSave ();
}

/* ========================================================================= */
/* 関数名：SboWebGetGameState												 */
/* 内容：ゲームの状態を返す（シェル HTML がチャット欄の出し入れに使う）		 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebGetGameState(void)
{
	return (int)g_byGameState;
}

/* ========================================================================= */
/* 関数名：SboWebChatSubmit													 */
/* 内容：チャット欄から発言する（文字列は UTF-8）							 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE void SboWebChatSubmit(const char *pszUtf8)
{
	char szBuf[MSGLOG_SPEECH_MAX + 2];
	int i, nLen;

	if (pszUtf8 == NULL) {
		return;
	}
	/* マップを歩ける状態でなければ発言しない */
	if ((g_byGameState < GAMESTATE_MOVE) || (g_nMyCharID < 0) ||
		(g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}

	std::string strSjis = W32Utf8ToSjisStr (pszUtf8);
	ZeroMemory (szBuf, sizeof (szBuf));
	/* 全角文字の途中で切らないように詰める */
	nLen = 0;
	for (i = 0; i < (int)strSjis.size (); ) {
		int nChar = IsDBCSLeadByte ((BYTE)strSjis[i]) ? 2 : 1;
		if (nLen + nChar > MSGLOG_SPEECH_MAX) {
			break;
		}
		memcpy (&szBuf[nLen], &strSjis[i], nChar);
		nLen += nChar;
		i += nChar;
	}
	if (nLen == 0) {
		return;
	}
	ChatMsgProc (szBuf);
}
