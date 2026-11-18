/* ========================================================================= */
/* ファイル名：	web_dungeon_svr.cpp											 */
/* 概要：		ダンジョン生成（管理ページ「ダンジョン生成」のお試し作成）		 */
/* ========================================================================= */
/*
 * 生成そのものはゲーム本体に依存しない SboSvr/src/DungeonGen/ が行う（docs/dungeon-gen.md）。
 *   JS → C : SboWebDungeonGen  {"params":{...},"theme":{...}} → 結果の JSON
 *            失敗した時は {"ok":false,"error":"..."}
 * 戻り値の文字列は次に呼ぶまで有効（JS 側で UTF8ToString して JSON.parse する）。
 */
#include "SboSvr/src/DungeonGen/DungeonGen.h"

#include <emscripten.h>
#include <string>

static std::string s_strDungeonResult;

extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDungeonGen(const char *pszJson)
{
	DungeonGen::Err e = DungeonGen::RunJson(pszJson, s_strDungeonResult);
	if (e != DungeonGen::ERR_NONE) {
		const char *pszMsg = "ダンジョンを作れませんでした";
		switch (e) {
		case DungeonGen::ERR_PARAM: pszMsg = "設定が範囲外です（大きさは 8〜128、部屋の大きさは 4 以上）"; break;
		case DungeonGen::ERR_RETRY: pszMsg = "作り直しの上限を超えました。大きさや床の割合を変えてください"; break;
		case DungeonGen::ERR_JSON: pszMsg = "入力の JSON が読めません"; break;
		case DungeonGen::ERR_ALGO: pszMsg = "未対応の作り方です"; break;
		default: break;
		}
		s_strDungeonResult = std::string("{\"ok\":false,\"error\":\"") + pszMsg + "\"}";
	}
	return s_strDungeonResult.c_str();
}
