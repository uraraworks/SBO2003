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
 *                  （Enter で開くゲーム内の発言欄も SboWebSpeechSubmit からシェルの
 *                    分割送信に渡し、長い発言を分けて送る）
 *  - SaveMsgLog  … メニューの「ログ保存」で window.sboChatLogSave を呼び、
 *                  チャット欄のログをテキストファイル（UTF-8）としてダウンロードする
 *  - 吹き出し    … スマホは吹き出しウィンドウ（小窓）を出さないので、チャットの
 *                  シートの「吹き出し」タブから SboWebBalloonGrp / SboWebBalloonSend で出す
 */
#include "StdAfx.h"
#include "Glbldata.h"
#include "SboCliKeyProc.h"
#include "SboCliSendCmd.h"

#include <emscripten.h>
#include <string>

/* 互換層（platform/w32_sjis.cpp）の文字コード変換 */
std::string W32TextToUtf8Str(const char *psz, int nLen);
std::string W32Utf8ToTextStr(const char *pszUtf8);

/* 発言できる長さ（Windows 版の入力欄と同じ。SJIS の 40 バイトと同じ表示幅で数える） */
#define MSGLOG_SPEECH_MAX	40		/* 1 回の発言の上限（表示幅。半角 40 文字・全角 20 文字） */
/* URL を含む発言の上限（表示幅）。URL は途中で切ると開けないので、
   シェル HTML の分割（splitChat の CHAT_URL_SPEECH_MAX）と同じ長さまで 1 つの発言で送る */
#define MSGLOG_SPEECH_URL_MAX	1000

extern "C" void SboWebChatSubmit(const char *pszUtf8, int nKeyMod);

/* 発言した時の Shift/Ctrl（SboCliKeyProc.cpp。-1 ならキーの状態を見る） */
extern int g_nWebSpeechKeyMod;

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

EM_JS(void, SboJsChatNotify, (void), {
	if (typeof window !== 'undefined' && typeof window['sboChatNotify'] === 'function') {
		window['sboChatNotify']();
	}
});

/* 発言が来たことをシェルに知らせる（SboCliRecvCmd.cpp の RecvDataProcSPEECH から）。
   別のタブを見ている間は、タブの題名に新着の数を出す */
void SboWebChatNotify(void)
{
	SboJsChatNotify ();
}

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
	std::string strText = W32TextToUtf8Str (lpStr, -1);
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

/* チャット欄と同じ分割送信（シェル HTML の window.sboChatSend）へ渡す。
   シェルが古いなどで無い時は 0 を返す */
EM_JS(int, SboJsChatSend, (const char *pszUtf8, int nKeyMod), {
	if (typeof window === 'undefined' || typeof window['sboChatSend'] !== 'function') {
		return 0;
	}
	window['sboChatSend'](UTF8ToString(pszUtf8), nKeyMod);
	return 1;
});

/* ゲーム内の発言入力欄（HTML の入力欄）に、シェルの残り文字数の表示を付ける */
EM_JS(void, SboJsSpeechCountAttach, (int nHtmlID), {
	var el = Module['w32Html'] && Module['w32Html'][nHtmlID];
	if (el && typeof window !== 'undefined' && typeof window['sboChatCountAttach'] === 'function') {
		window['sboChatCountAttach'](el);
	}
});

extern int W32HtmlGetID(HWND hWnd);

/* ========================================================================= */
/* 関数名：SboWebSpeechInputMade											 */
/* 内容：Enter で開く発言入力欄を作った時に呼ぶ。全角 20 文字（1 回の発言）を	 */
/*		 超えるか送る前に分かるよう、残り文字数の表示を付ける				 */
/* ========================================================================= */
void SboWebSpeechInputMade(HWND hWnd)
{
	int nHtmlID = W32HtmlGetID (hWnd);
	if (nHtmlID) {
		SboJsSpeechCountAttach (nHtmlID);
	}
}

/* ========================================================================= */
/* 関数名：SboWebSpeechSubmit												 */
/* 内容：ゲーム内の発言入力欄（Enter で開く欄）から発言する（文字列は内部の UTF-8）	 */
/*		 長い発言はチャット欄と同じように分けて順に送る						 */
/* ========================================================================= */
void SboWebSpeechSubmit(LPCSTR pszText, int nKeyMod)
{
	if ((pszText == NULL) || (pszText[0] == 0)) {
		return;
	}
	std::string strUtf8 = W32TextToUtf8Str (pszText, -1);
	if (SboJsChatSend (strUtf8.c_str (), nKeyMod)) {
		return;
	}
	/* 分割送信が使えない時は、これまでどおり上限で切って発言する */
	SboWebChatSubmit (strUtf8.c_str (), nKeyMod);
}

/* ========================================================================= */
/* 関数名：SboWebChatSubmit													 */
/* 内容：チャット欄から発言する（文字列は UTF-8）							 */
/*		 nKeyMod は入力した時のキー（0:なし 1:Shift 2:Ctrl）。管理者なら		 */
/*		 Shift で全体発言、Ctrl で管理者用内線になる（ゲーム内の発言欄と同じ）	 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE void SboWebChatSubmit(const char *pszUtf8, int nKeyMod)
{
	int nLen, nMax;

	if (pszUtf8 == NULL) {
		return;
	}
	/* マップを歩ける状態でなければ発言しない */
	if ((g_byGameState < GAMESTATE_MOVE) || (g_nMyCharID < 0) ||
		(g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return;
	}

	std::string strText = W32Utf8ToTextStr (pszUtf8);
	/* URL を含む発言は長くてもそのまま（チャット欄で 1 つのリンクとして見られるように） */
	nMax = MSGLOG_SPEECH_MAX;
	if ((strText.find ("http://") != std::string::npos) || (strText.find ("https://") != std::string::npos)) {
		nMax = MSGLOG_SPEECH_URL_MAX;
	}
	/* 文字の途中で切らないように詰める */
	nLen = SboTextFitWidth (strText.c_str (), nMax);
	if (nLen == 0) {
		return;
	}
	std::string strBuf (strText.c_str (), nLen);
	g_nWebSpeechKeyMod = nKeyMod;
	ChatMsgProc ((LPCTSTR)strBuf.c_str ());
	g_nWebSpeechKeyMod = -1;
}

/* ========================================================================= */
/* 関数名：SboWebBalloonGrp													 */
/* 内容：吹き出しの絵の番号を返す（シェル HTML が吹き出しの一覧を作るのに使う）	 */
/*		 nNo は吹き出しの番号（1〜MAX_COMEFC。吹き出しウィンドウのマス番号と	 */
/*		 同じ）、nFrame はアニメーションのコマ（0〜）。絵は IDP_EFC01 の		 */
/*		 16x16 のマスで、横 8 マスずつ並んでいる。							 */
/*		 その番号の吹き出しやコマが無ければ -1								 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebBalloonGrp(int nNo, int nFrame)
{
	if ((g_pComEfcInfo == NULL) || (nNo < 1) || (nNo > MAX_COMEFC)) {
		return -1;
	}
	PCOMEFCINFO pComEfc = &g_pComEfcInfo[nNo - 1];
	if ((pComEfc->pAnime == NULL) || (nFrame < 0) || (nFrame >= pComEfc->byAnimeCount)) {
		return -1;
	}
	return (int)pComEfc->pAnime[nFrame].byGrpID;
}

/* ========================================================================= */
/* 関数名：SboWebBalloonSend												 */
/* 内容：吹き出しを出す（吹き出しウィンドウのマスを押した時と同じ）			 */
/*		 出せた時は 1、マップを歩ける状態でない・吹き出しが無い時は 0		 */
/* ========================================================================= */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebBalloonSend(int nNo)
{
	if ((g_byGameState < GAMESTATE_MOVE) || (g_nMyCharID < 0) ||
		(g_nMyCharID >= g_aCharInfo.GetSize ())) {
		return 0;
	}
	if (SboWebBalloonGrp (nNo, 0) < 0) {
		return 0;
	}
	SendCmdCOMEFC ((BYTE)nNo);
	return 1;
}
