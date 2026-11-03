/* ========================================================================= */
/* ファイル名：	web_status_svr.cpp											 */
/* 概要：		サーバー状況（オンライン数など）を JS へ渡す					 */
/* ========================================================================= */
/*
 * Windows 版の SboSvr はウィンドウに「サーバー稼動時間・接続数・処理キャラ数・
 * 処理マップ数」を描いていた（SboSvrDrawWindow.cpp）。Node.js 版には画面が無いので、
 * 同じ値を JSON にして server/node/sbosvr_status.js へ渡し、HTTP で見られるようにする。
 *
 *   JS → C : SboSvrStatusJson  サーバー状況（JSON）
 *
 * キャラ名やアカウント名は出さない（誰でも見られる情報だけにする）。
 */
#include "SboSvr/src/include.h"

#include <emscripten.h>
#include <string>

static std::string s_strStatus;

/* 24 個の数を JSON の配列にする */
static std::string HourArrayJson(const DWORD *pdw)
{
	std::string out = "[";
	char sz[16];
	for (int i = 0; i < 24; i ++) {
		snprintf(sz, sizeof(sz), "%s%u", i ? "," : "", (unsigned)pdw[i]);
		out += sz;
	}
	out += "]";
	return out;
}

/*
 * サーバー状況
 *   online          ゲームに入っている人数（Windows 版の「接続数」と同じ g_dwOnline）
 *   maxConnect      入れる人数の上限（管理者は上限を超えても入れる）
 *   playerChars     作られたプレイヤーキャラの数（NPC を除く）
 *   chars / maps    処理キャラ数・処理マップ数（Windows 版の画面と同じ）
 *   uptimeSec       サーバー稼動時間（秒）
 *   hourRecord      時間帯ごとの最大人数の記録（SboSvr.ini の [OnlineCount]。サーバーの時刻で 0〜23 時）
 *   hourSinceStart  今回起動してからの時間帯ごとの人数（毎時 0 分の時点の値。Windows 版と同じ数え方）
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboSvrStatusJson(void)
{
	int i, nCount = g_aCharInfo.GetSize(), nPlayer = 0;
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if ((pChar->byNpcType == NPCTYPE_INIT) && pChar->szAcc[0]) {
			nPlayer ++;
		}
	}
	char sz[256];
	snprintf(sz, sizeof(sz),
			"{\"online\":%u,\"maxConnect\":%d,\"playerChars\":%d,\"chars\":%d,\"maps\":%d,\"uptimeSec\":%u",
			(unsigned)g_dwOnline, MAX_CONNECTCOUNT, nPlayer, nCount, g_aMapInfo.GetSize(),
			(unsigned)((timeGetTime() - g_dwServerStartTime) / 1000));
	s_strStatus = sz;
	s_strStatus += ",\"hourRecord\":" + HourArrayJson(g_dwOnlineBackMax);
	s_strStatus += ",\"hourSinceStart\":" + HourArrayJson(g_dwOnlineBack);
	s_strStatus += "}";
	return s_strStatus.c_str();
}
