/* ========================================================================= */
/* ファイル名：	web_main.cpp												 */
/* 概要：		ブラウザ版の入口											 */
/* ========================================================================= */
/*
 * 1. /sbo に IDBFS（ブラウザの IndexedDB）をマウントし、保存データを読み込む。
 * 2. 読み込みが終わったら SboCliMain を呼ぶ（Windows 版の WinMain と同じ流れ）。
 *    SboCliMain の中で emscripten_set_main_loop が登録され、以降は
 *    ブラウザの描画フレームごとにメインループが回る。
 */
#include "../platform/w32_internal.h"

#include <emscripten.h>
#include <stdio.h>

extern void SboCliMain(LPSTR lpszArgs);

/* 保存データの読み込みが終わったら JavaScript から呼ばれる */
extern "C" EMSCRIPTEN_KEEPALIVE void W32WebStart(void)
{
	printf("[sbo] 起動します\n");
	static char s_szArgs[] = "";
	SboCliMain(s_szArgs);
}

int main(int, char **)
{
	EM_ASM({
		try {
			FS.mkdir('/sbo');
		} catch (e) {
		}
		FS.mount(IDBFS, {}, '/sbo');
		FS.syncfs(true, function (err) {
			if (err) {
				console.warn('[sbo] 保存データを読み込めませんでした', err);
			}
			/* ページを閉じる前に書き戻す */
			window.addEventListener('pagehide', function () {
				FS.syncfs(false, function () {});
			});
			_W32WebStart();
		});
	});
	return 0;
}
