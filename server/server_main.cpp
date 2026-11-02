/* ========================================================================= */
/* ファイル名：	server_main.cpp												 */
/* 概要：		Node.js 版サーバーの入口（画面なしで WinMain を呼ぶ）		 */
/* ========================================================================= */
/*
 * 使い方: node SboSvr.js [--headless] [--data <フォルダ>] [--http-port <番号>] [--www <フォルダ>]
 *
 *   --data <フォルダ>     SboSvr.ini と SBODATA/ があるフォルダ（省略時はカレントフォルダ）
 *   --headless            画面なしで動かす（いまは常に画面なしなので、SBOP2 と揃えるための印）
 *   --http-port <番号>    ブラウザ版を配る HTTP・WebSocket のポート（既定 8080、0 で使わない）
 *   --www <フォルダ>      ブラウザ版のファイルの場所（既定 out/web/dist）
 *   --save-npc-only       終了時に NPC だけを保存する（公開用の初期データ作り。tools/server/make-seed.sh 用）
 *
 * Ctrl+C（SIGINT）や SIGTERM を受けたら、メインウィンドウに WM_CLOSE を送って
 * Windows 版で「終了」を選んだ時と同じようにデータを保存してから終わる。
 */
#include <windows.h>

#include <emscripten.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern BOOL g_bSaveNpcOnly;			/* SboSvrSaveLoadChar.cpp */
extern int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode);

/* HTTP のポートと配るフォルダを Node.js 側（server/node/sbosvr_net.js）へ渡す */
EM_JS(void, W32JsNetConfig, (int nHttpPort, const char *pszWww), {
	var www = UTF8ToString(pszWww);
	if (!www) {
		www = require('path').resolve(__dirname, '..', 'web', 'dist');
	}
	Module['sboNet'].config(nHttpPort, www);
});

/* Node.js のシグナル処理から呼ばれる */
extern "C" EMSCRIPTEN_KEEPALIVE void SboSvrRequestClose(void)
{
	W32RequestClose();
}

/* SboSvr.cpp から呼ばれる設定の注意（SboSvr 側の文字列は SJIS 化されて文字化けするのでこちらで出す） */
void SvrLogSettingWarning(int nKind)
{
	switch (nKind) {
	case 0:
		printf("[SboSvr] 注意: SboSvr.ini の [Administrator] Password が空なので、管理者ではログインできません\n");
		break;
	case 1:
		printf("[SboSvr] 注意: SboSvr.ini の AllAdmin=1 なので、新しいアカウントは全員管理者になります（開発用）\n");
		break;
	}
}

static void Usage(const char *pszName)
{
	fprintf(stderr, "使い方: node SboSvr.js [--headless] [--data <フォルダ>] [--http-port <番号>] [--www <フォルダ>] [--save-npc-only]\n");
}

int main(int argc, char **argv)
{
	const char *pszData = NULL;
	int nHttpPort = 8080;
	const char *pszWww = "";
	for (int i = 1; i < argc; i ++) {
		if (strcmp(argv[i], "--headless") == 0) {
			/* いまは常に画面なし */
		} else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
			pszData = argv[++ i];
		} else if (strcmp(argv[i], "--http-port") == 0 && i + 1 < argc) {
			nHttpPort = atoi(argv[++ i]);
		} else if (strcmp(argv[i], "--www") == 0 && i + 1 < argc) {
			pszWww = argv[++ i];
		} else if (strcmp(argv[i], "--save-npc-only") == 0) {
			g_bSaveNpcOnly = TRUE;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			Usage(argv[0]);
			return 0;
		} else {
			Usage(argv[0]);
			return 1;
		}
	}

	/* ログをすぐ出す（VSCode のターミナルや launch.json で見やすいように） */
	setvbuf(stdout, NULL, _IOLBF, 0);

	/* --www の相対パスは、データフォルダに移る前のカレントフォルダから見る */
	W32JsNetConfig(nHttpPort, pszWww);

	if (pszData && chdir(pszData) != 0) {
		fprintf(stderr, "[SboSvr] データフォルダに移れません: %s\n", pszData);
		return 1;
	}
	struct stat st;
	if (stat("SBODATA", &st) != 0 || !S_ISDIR(st.st_mode)) {
		fprintf(stderr, "[SboSvr] SBODATA フォルダが見つかりません（--data で sbosvr.ini のあるフォルダを指定してください）\n");
		return 1;
	}


	char szCwd[1024];
	if (getcwd(szCwd, sizeof(szCwd)) == NULL) {
		szCwd[0] = '\0';
	}
	printf("[SboSvr] 起動します（データ: %s）\n", szCwd);
	int nRet = WinMain(NULL, NULL, (LPSTR)"", SW_HIDE);
	printf("[SboSvr] 終了しました\n");
	return nRet;
}
