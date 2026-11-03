/* ========================================================================= */
/* ファイル名：	web_account_svr.cpp											 */
/* 概要：		ブラウザ版のアカウント関係（端末トークン・アカウント復旧）		 */
/* ========================================================================= */
/*
 * 端末トークンの保存・試行回数の制限・HTTP の受付は Node.js 側
 * （server/node/sbosvr_account.js）が持つ。ここはキャラ情報（g_aCharInfo）を
 * 見る必要がある部分だけを受け持つ。
 *
 *   C → JS : WebAccTokenToAccount / WebAccIssueToken（ゲームの通信から呼ぶ）
 *   JS → C : SboWebAccInfo        アカウントの表示用情報（JSON）
 *            SboWebAccRecoverFind アカウント復旧の照合（JSON）
 *            SboWebAccSetPassword 復旧したアカウントのパスワードを変える
 *            SboWebAccRename      キャラ名を変える（確かめるだけもできる）
 *            SboWebAdminSearch    管理ページのキャラ検索（キャラ名・アカウント名の一部）
 *
 * パスワードは C 側の外へ出さない（照合もここで行う）。
 * キャラ名は SJIS で持っているので、JS とは UTF-8 でやり取りする。
 */
#include "SboSvr/src/include.h"

#include <emscripten.h>
#include <ctype.h>
#include <string>

extern BOOL SaveCharInfo(void);

/* 文字コード（platform/w32_sjis.cpp） */
std::string W32SjisToUtf8Str(const char *pszSjis, int nLen = -1);
std::string W32Utf8ToSjisStr(const char *pszUtf8);

/* ------------------------------------------------------------------------- */
/* JS 側の端末トークン置き場													 */
/* ------------------------------------------------------------------------- */
EM_JS(int, W32JsWebAccTokenToAccount, (const char *pszToken, char *pszOut, int nSize), {
	var acc = Module['sboAccount'].tokenToAccount(UTF8ToString(pszToken));
	if (!acc) {
		return 0;
	}
	stringToUTF8(acc, pszOut, nSize);
	return 1;
});

EM_JS(int, W32JsWebAccIssueToken, (const char *pszAcc, char *pszOut, int nSize), {
	var token = Module['sboAccount'].issueToken(UTF8ToString(pszAcc));
	if (!token) {
		return 0;
	}
	stringToUTF8(token, pszOut, nSize);
	return 1;
});

/* 端末トークンからアカウント名を引く */
BOOL WebAccTokenToAccount(LPCSTR pszToken, LPSTR pszAcc, int nSize)
{
	if ((pszToken == NULL) || (strlen(pszToken) != MAX_WEBTOKEN)) {
		return FALSE;
	}
	return W32JsWebAccTokenToAccount(pszToken, pszAcc, nSize) ? TRUE : FALSE;
}

/* 端末トークンを発行する（ログイン済みのアカウントにだけ呼ぶ） */
BOOL WebAccIssueToken(LPCSTR pszAcc, LPSTR pszToken, int nSize)
{
	if ((pszAcc == NULL) || (pszAcc[0] == 0)) {
		return FALSE;
	}
	return W32JsWebAccIssueToken(pszAcc, pszToken, nSize) ? TRUE : FALSE;
}

/* ------------------------------------------------------------------------- */
/* 下請け																	 */
/* ------------------------------------------------------------------------- */

/* JSON の文字列にする（中身は UTF-8） */
static std::string JsonStr(const std::string &str)
{
	std::string out = "\"";
	for (size_t i = 0; i < str.size(); i ++) {
		unsigned char c = (unsigned char)str[i];
		if (c == '"' || c == '\\') {
			out += '\\';
			out += (char)c;
		} else if (c < 0x20) {
			char sz[8];
			snprintf(sz, sizeof(sz), "\\u%04x", c);
			out += sz;
		} else {
			out += (char)c;
		}
	}
	out += "\"";
	return out;
}

/* 前後の空白を取る */
static std::string Trim(const char *psz)
{
	std::string str = psz ? psz : "";
	size_t nStart = str.find_first_not_of(" \t\r\n");
	if (nStart == std::string::npos) {
		return "";
	}
	size_t nEnd = str.find_last_not_of(" \t\r\n");
	return str.substr(nStart, nEnd - nStart + 1);
}

static std::string Lower(const std::string &str)
{
	std::string out = str;
	for (size_t i = 0; i < out.size(); i ++) {
		out[i] = (char)tolower((unsigned char)out[i]);
	}
	return out;
}

/* プレイヤーのキャラか（NPC やアカウント名の無いものは除く） */
static BOOL IsPlayerChar(PCHARINFO pChar)
{
	return (pChar->byNpcType == NPCTYPE_INIT) && pChar->szAcc[0];
}

/* アカウント名からプレイヤーキャラを探す（ログインと同じく最初に見つかったもの） */
static PCHARINFO FindPlayerByAcc(const char *pszAcc)
{
	int i, nCount = g_aCharInfo.GetSize();
	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (IsPlayerChar(pChar) && (strcmp(pChar->szAcc, pszAcc) == 0)) {
			return pChar;
		}
	}
	return NULL;
}

/* キャラの表示用情報（JSON のメンバー部分） */
static std::string CharInfoJson(PCHARINFO pChar)
{
	char sz[256];
	std::string out;
	out += "\"account\":" + JsonStr(pChar->szAcc);
	out += ",\"name\":" + JsonStr(W32SjisToUtf8Str(pChar->szName));
	snprintf(sz, sizeof(sz),
			",\"level\":%d,\"made\":\"%04d/%02d/%02d\",\"playMinutes\":%u,\"loginCount\":%u"
			",\"admin\":%s,\"disabled\":%s,\"online\":%s",
			pChar->byLevel, pChar->wMakeYear, pChar->wMakeMonth, pChar->wMakeDay,
			(unsigned)(pChar->dwPlayTime / 60), (unsigned)pChar->dwLoginCount,
			(pChar->byAdminLV != ADMINLV_USER) ? "true" : "false",
			(pChar->byAccState & ACCSTATE_DISABLE) ? "true" : "false",
			pChar->pID ? "true" : "false");
	out += sz;
	snprintf(sz, sizeof(sz), ",\"tblID\":%u,\"mapID\":%u,\"x\":%u,\"y\":%u",
			(unsigned)pChar->dwTblID, (unsigned)pChar->dwMapID, (unsigned)pChar->dwX, (unsigned)pChar->dwY);
	out += sz;
	return out;
}

/*
 * パスワードの一部が合っているか
 *   連続した 3 文字以上で、かつパスワードの半分以上の長さがあること
 *   （大文字・小文字は区別しない。パスワードが空なら合わない）
 */
static BOOL MatchPasswordPart(const char *pszPas, const std::string &strPart)
{
	size_t nPas = strlen(pszPas);
	if (nPas == 0 || strPart.size() < 3 || strPart.size() * 2 < nPas) {
		return FALSE;
	}
	return (Lower(pszPas).find(Lower(strPart)) != std::string::npos) ? TRUE : FALSE;
}

/* ------------------------------------------------------------------------- */
/* JS（HTTP の受付）から呼ばれる												 */
/* ------------------------------------------------------------------------- */
static std::string s_strResult;

/* アカウントの表示用情報。見つからなければ "null" */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebAccInfo(const char *pszAcc)
{
	PCHARINFO pChar = FindPlayerByAcc(pszAcc ? pszAcc : "");
	s_strResult = pChar ? ("{" + CharInfoJson(pChar) + "}") : "null";
	return s_strResult.c_str();
}

/*
 * アカウント復旧の照合
 *   キャラ名（完全一致）が同じプレイヤーキャラを全部調べ、
 *   アカウント名（大文字・小文字は区別しない）とパスワードの一部が合うかを返す。
 *   結果: {"candidates":キャラ名が合った数,"matches":[{...,"byAccount":bool,"byPassword":bool}]}
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebAccRecoverFind(const char *pszName, const char *pszAcc, const char *pszPasPart)
{
	std::string strName = W32Utf8ToSjisStr(Trim(pszName).c_str());
	std::string strAcc = Lower(Trim(pszAcc));
	std::string strPart = Trim(pszPasPart);
	int i, nCount = g_aCharInfo.GetSize(), nCandidates = 0;
	std::string strMatches;

	for (i = 0; i < nCount && !strName.empty(); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (!IsPlayerChar(pChar) || strcmp(pChar->szName, strName.c_str()) != 0) {
			continue;
		}
		nCandidates ++;
		BOOL bAcc = (!strAcc.empty() && Lower(pChar->szAcc) == strAcc) ? TRUE : FALSE;
		BOOL bPas = MatchPasswordPart(pChar->szPas, strPart);
		if (!bAcc && !bPas) {
			continue;
		}
		if (!strMatches.empty()) {
			strMatches += ",";
		}
		strMatches += "{" + CharInfoJson(pChar);
		strMatches += std::string(",\"byAccount\":") + (bAcc ? "true" : "false");
		strMatches += std::string(",\"byPassword\":") + (bPas ? "true" : "false") + "}";
	}
	char sz[64];
	snprintf(sz, sizeof(sz), "{\"candidates\":%d,\"matches\":[", nCandidates);
	s_strResult = sz + strMatches + "]}";
	return s_strResult.c_str();
}

/*
 * 復旧したアカウントのパスワードを変えて、キャラ情報を保存する
 *   使える文字は ASCII の 0x21〜0x7E、長さは 4〜(MAX_ACCNAME - 1) 文字
 *   戻り値: 1 = 変えた、0 = 使えないパスワード、-1 = アカウントが無い・ログイン中
 */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebAccSetPassword(const char *pszAcc, const char *pszPas)
{
	size_t i, nLen = pszPas ? strlen(pszPas) : 0;
	if (nLen < 4 || nLen > MAX_ACCNAME - 1) {
		return 0;
	}
	for (i = 0; i < nLen; i ++) {
		if ((unsigned char)pszPas[i] < 0x21 || (unsigned char)pszPas[i] > 0x7E) {
			return 0;
		}
	}
	PCHARINFO pChar = FindPlayerByAcc(pszAcc ? pszAcc : "");
	if (pChar == NULL || pChar->pID) {
		return -1;
	}
	ZeroMemory(pChar->szPas, sizeof(pChar->szPas));
	strcpy(pChar->szPas, pszPas);
	SaveCharInfo();
	return 1;
}

/* 管理者アカウントの ID・パスワードか（復旧の承認ページのログインに使う） */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebAccCheckAdmin(const char *pszAcc, const char *pszPas)
{
	if (pszAcc == NULL || pszPas == NULL || g_szAdminAcc[0] == 0 || g_szAdminPas[0] == 0) {
		return 0;
	}
	return (strcmp(pszAcc, g_szAdminAcc) == 0 && strcmp(pszPas, g_szAdminPas) == 0) ? 1 : 0;
}

/* ------------------------------------------------------------------------- */
/* キャラ名の変更																 */
/* ------------------------------------------------------------------------- */

/* SJIS の 2 バイト文字の 1 バイト目か */
static BOOL IsSjisLead(unsigned char c)
{
	return ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) ? TRUE : FALSE;
}

/*
 * キャラ名として使えるか確かめ、SJIS にしたものを返す
 *   ゲームの作成画面（SboCliMain.cpp の IDC_MAKE）・サーバーの RecvDataProcCHGNAMESEX と同じ決まり
 *     ・SJIS で 1〜(MAX_CHARNAME - 1) バイト（全角 7 文字・半角 14 文字まで）
 *     ・空白（半角・全角）や制御文字は使えない
 *     ・SJIS に無い文字（絵文字・一部の旧字など）は使えない
 *   戻り値: NULL = 使える、それ以外 = 使えない理由
 */
static const char *CheckCharName(const std::string &strUtf8, std::string &strSjis)
{
	if (strUtf8.empty()) {
		return "empty";
	}
	strSjis = W32Utf8ToSjisStr(strUtf8.c_str());
	/* SJIS に直して戻した時に元どおりでなければ、SJIS に無い文字が入っている */
	if (W32SjisToUtf8Str(strSjis.c_str()) != strUtf8) {
		return "charset";
	}
	if (strSjis.size() > MAX_CHARNAME - 1) {
		return "long";
	}
	for (size_t i = 0; i < strSjis.size(); i ++) {
		unsigned char c = (unsigned char)strSjis[i];
		if (IsSjisLead(c)) {
			if (i + 1 >= strSjis.size()) {
				return "charset";
			}
			/* 全角空白 */
			if (c == 0x81 && (unsigned char)strSjis[i + 1] == 0x40) {
				return "char";
			}
			i ++;
		} else if (c <= 0x20 || c == 0x7F || c == 0x80 || c == 0xA0 || c >= 0xFD) {
			return "char";
		}
	}
	return NULL;
}

/*
 * キャラ名を変える
 *   pszAcc       … アカウント名
 *   pszName      … 新しいキャラ名（UTF-8）
 *   bApply       … 0 なら確かめるだけ（変えない）
 *   bAllowOnline … 1 ならログイン中でも変える（管理者用。周りの人に新しい名前を送り直す）
 *   戻り値（JSON）: {"result":"ok","from":"前の名前","to":"新しい名前"}
 *                   {"result":"empty"|"charset"|"long"|"char"|"same"|"taken"|"notfound"|"online"}
 *   同じ名前のキャラ（NPC も含む）がいたら "taken"。
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebAccRename(const char *pszAcc, const char *pszName, int bApply, int bAllowOnline)
{
	std::string strUtf8 = Trim(pszName), strSjis;
	PCHARINFO pChar = FindPlayerByAcc(pszAcc ? pszAcc : "");
	const char *pszResult = NULL;
	int i, nCount = g_aCharInfo.GetSize();

	if (pChar == NULL) {
		pszResult = "notfound";
	} else if (pChar->szName[0] == 0) {
		/* まだキャラを作っていない（作成画面で決めてもらう） */
		pszResult = "notfound";
	} else if (pChar->pID && !bAllowOnline) {
		pszResult = "online";
	} else {
		pszResult = CheckCharName(strUtf8, strSjis);
	}
	if (pszResult == NULL && strcmp(pChar->szName, strSjis.c_str()) == 0) {
		pszResult = "same";
	}
	for (i = 0; i < nCount && pszResult == NULL; i ++) {
		PCHARINFO pCharCheck = g_aCharInfo[i];
		if (pCharCheck != pChar && strcmp(pCharCheck->szName, strSjis.c_str()) == 0) {
			pszResult = "taken";
		}
	}
	if (pszResult) {
		s_strResult = std::string("{\"result\":\"") + pszResult + "\"}";
		return s_strResult.c_str();
	}

	std::string strFrom = W32SjisToUtf8Str(pChar->szName);
	if (bApply) {
		ZeroMemory(pChar->szName, sizeof(pChar->szName));
		strcpy(pChar->szName, strSjis.c_str());
		SaveCharInfo();
		/* ログイン中なら、本人と周りの人に新しい名前を送る */
		if (pChar->pID) {
			SendCmdCHARINFO3((int)pChar->dwTblID);
			std::string strMsg = W32Utf8ToSjisStr(("SYSTEM:キャラ名が「" + strUtf8 + "」に変わりました。").c_str());
			SendCmdSYSMSG(pChar->pID, strMsg.c_str());
		}
	}
	s_strResult = "{\"result\":\"ok\",\"from\":" + JsonStr(strFrom) + ",\"to\":" + JsonStr(strUtf8) + "}";
	return s_strResult.c_str();
}

/*
 * 管理ページのキャラ検索
 *   キャラ名（一部でよい）かアカウント名（一部でよい・大文字小文字は区別しない）で探す。
 *   結果: {"results":[{...}],"more":bool}（nMax 件まで）
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebAdminSearch(const char *pszKey, int nMax)
{
	std::string strKey = Trim(pszKey), strKeyLower = Lower(strKey), strOut;
	int i, nCount = g_aCharInfo.GetSize(), nHit = 0;
	BOOL bMore = FALSE;

	for (i = 0; i < nCount && !strKey.empty(); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (!IsPlayerChar(pChar)) {
			continue;
		}
		BOOL bHit = (Lower(pChar->szAcc).find(strKeyLower) != std::string::npos) ? TRUE : FALSE;
		if (!bHit && pChar->szName[0]) {
			bHit = (W32SjisToUtf8Str(pChar->szName).find(strKey) != std::string::npos) ? TRUE : FALSE;
		}
		if (!bHit) {
			continue;
		}
		if (nHit >= nMax) {
			bMore = TRUE;
			break;
		}
		if (nHit) {
			strOut += ",";
		}
		strOut += "{" + CharInfoJson(pChar) + "}";
		nHit ++;
	}
	s_strResult = "{\"results\":[" + strOut + "],\"more\":" + (bMore ? "true" : "false") + "}";
	return s_strResult.c_str();
}
