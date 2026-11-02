/* ========================================================================= */
/* ファイル名：	w32_file.cpp												 */
/* 概要：		Win32 互換レイヤ：ファイル・INI・モジュール・リソース		 */
/* ========================================================================= */
/*
 * 仕組み
 *  - サーバー（Node.js 版）では、実行ファイルはカレントディレクトリの
 *    "SboSvr.exe" にあることにする（データのあるフォルダで起動する）。
 *  - ブラウザ版では、実行ファイルは "/sbo/SboCli.exe" にあることにする。/sbo はブラウザの
 *    IndexedDB に保存される（web_main.cpp で IDBFS をマウントする）。
 *  - パスの '\' は '/' に直し、続いたスラッシュは 1 つにまとめる。
 *  - ファイルを書き換えたら、少し待ってから IndexedDB へ書き戻す。
 *  - リソース DLL は無いので、LoadLibrary はモジュール名だけを持つハンドルを返す。
 *    FindResource / LoadString は gen_resources.py が作った表を引き、
 *    preload したファイル（/res/<モジュール名>/...）を読む。
 */
#include "w32_internal.h"
#include "w32_resources.h"

#include <emscripten.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <map>

#ifdef W32_BROWSER
#define W32_EXE_PATH	"/sbo/SboCli.exe"
#define W32_BASE_DIR	"/sbo"
#else
#ifndef W32_EXE_NAME
#define W32_EXE_NAME	"SboSvr.exe"
#endif
#endif

/* 実行ファイルのあるフォルダ（末尾の / 無し） */
static std::string BaseDir(void)
{
#ifdef W32_BROWSER
	return W32_BASE_DIR;
#else
	char szBuf[4096];
	if (getcwd(szBuf, sizeof(szBuf)) == NULL) {
		return ".";
	}
	return szBuf;
#endif
}

/* ------------------------------------------------------------------------- */
/* パス・保存																 */
/* ------------------------------------------------------------------------- */

/* Windows のパスを仮想ファイルシステムのパスにする */
static std::string NormPath(LPCSTR pszName)
{
	std::string str;
	if (pszName == NULL) {
		return str;
	}
	for (const char *p = pszName; *p; p ++) {
		char c = *p;
		/* SJIS の 2 バイト目の '\' は区切りではない */
		if (IsDBCSLeadByte((BYTE)c) && p[1]) {
			str += c;
			str += p[1];
			p ++;
			continue;
		}
		if (c == '\\') {
			c = '/';
		}
		if (c == '/' && !str.empty() && str[str.size() - 1] == '/') {
			continue;
		}
		str += c;
	}
	/* "C:" などのドライブ名は捨てる */
	if (str.size() >= 2 && str[1] == ':') {
		str = str.substr(2);
	}
	if (str.empty() || str[0] != '/') {
		str = BaseDir() + "/" + str;
	}
	if (str.size() > 1 && str[str.size() - 1] == '/') {
		str.erase(str.size() - 1);
	}
	return str;
}

#ifdef W32_BROWSER
EM_JS(void, W32JsScheduleSync, (void), {
	if (Module['w32SyncTimer']) {
		return;
	}
	Module['w32SyncTimer'] = setTimeout(function () {
		Module['w32SyncTimer'] = 0;
		FS.syncfs(false, function (err) {
			if (err) {
				console.warn('[sbo] 保存データの書き戻しに失敗しました', err);
			}
		});
	}, 500);
});
#else
/* サーバー（Node.js 版）はふつうのファイルに書くので何もしない */
static void W32JsScheduleSync(void)
{
}
#endif

/* 保存データを IndexedDB へ書き戻す（まとめて実行される） */
static void ScheduleSync(void)
{
	W32JsScheduleSync();
}

/* ------------------------------------------------------------------------- */
/* モジュール																 */
/* ------------------------------------------------------------------------- */
struct W32Module {
	DWORD		dwMagic;
	std::string	strName;		/* 小文字のモジュール名（例: "sbogrpdata.dll"） */
};

#define W32MOD_MAGIC	0x57334D44		/* 'W3MD' */

static std::map<std::string, W32Module *> s_mapModule;

static std::string ToLower(const std::string &str)
{
	std::string strRet = str;
	for (size_t i = 0; i < strRet.size(); i ++) {
		if (strRet[i] >= 'A' && strRet[i] <= 'Z') {
			strRet[i] = (char)(strRet[i] - 'A' + 'a');
		}
	}
	return strRet;
}

static W32Module *GetModule(const std::string &strName)
{
	std::string strKey = ToLower(strName);
	/* パスが付いていたら名前だけにする */
	size_t nPos = strKey.find_last_of("/\\");
	if (nPos != std::string::npos) {
		strKey = strKey.substr(nPos + 1);
	}
	std::map<std::string, W32Module *>::iterator it = s_mapModule.find(strKey);
	if (it != s_mapModule.end()) {
		return it->second;
	}
	W32Module *pModule = new W32Module;
	pModule->dwMagic = W32MOD_MAGIC;
	pModule->strName = strKey;
	s_mapModule[strKey] = pModule;
	return pModule;
}

static W32Module *CheckModule(HMODULE hModule)
{
	if (hModule == NULL) {
		return GetModule("SboCli.exe");
	}
	W32Module *pModule = (W32Module *)hModule;
	if (pModule->dwMagic != W32MOD_MAGIC) {
		return NULL;
	}
	return pModule;
}

/* その名前のリソース・文字列を持っているか（DLL が存在するかの判定に使う） */
static BOOL HasResources(const std::string &strModule)
{
	for (const W32ResEntry *p = g_aW32ResEntry; p->pszModule; p ++) {
		if (ToLower(p->pszModule) == strModule) {
			return TRUE;
		}
	}
	for (const W32StrEntry *p = g_aW32StrEntry; p->pszModule; p ++) {
		if (ToLower(p->pszModule) == strModule) {
			return TRUE;
		}
	}
	return FALSE;
}

LPSTR PathFindFileName(LPCSTR pszPath)
{
	LPCSTR pszRet = pszPath;
	for (LPCSTR p = pszPath; *p; p ++) {
		if (IsDBCSLeadByte((BYTE)*p) && p[1]) {
			p ++;
			continue;
		}
		if ((*p == '\\' || *p == '/' || *p == ':') && p[1]) {
			pszRet = p + 1;
		}
	}
	return (LPSTR)pszRet;
}

DWORD GetModuleFileName(HMODULE, LPSTR pszFileName, DWORD nSize)
{
	if (pszFileName == NULL || nSize == 0) {
		return 0;
	}
#ifdef W32_BROWSER
	lstrcpyn(pszFileName, W32_EXE_PATH, (int)nSize);
#else
	/* ゲーム側は「最後の \\ まで」をフォルダとして切り出すので、Windows と同じく \\ 区切りで返す
	   （開く時に NormPath が / に戻す） */
	std::string strPath = BaseDir() + "/" + W32_EXE_NAME;
	for (size_t i = 0; i < strPath.size(); i ++) {
		if (strPath[i] == '/') {
			strPath[i] = '\\';
		}
	}
	lstrcpyn(pszFileName, strPath.c_str(), (int)nSize);
#endif
	return (DWORD)strlen(pszFileName);
}

HMODULE GetModuleHandle(LPCSTR pszName)
{
	return (HMODULE)GetModule(pszName ? pszName : "SboCli.exe");
}

HMODULE LoadLibrary(LPCSTR pszName)
{
	if (pszName == NULL) {
		return NULL;
	}
	W32Module *pModule = GetModule(pszName);
	/* Windows のシステム DLL は「ある」ことにする */
	static const char *apszSystem[] = {
		"kernel32.dll", "user32.dll", "gdi32.dll", "riched32.dll", "riched20.dll", NULL
	};
	for (int i = 0; apszSystem[i]; i ++) {
		if (pModule->strName == apszSystem[i]) {
			return (HMODULE)pModule;
		}
	}
	if (HasResources(pModule->strName)) {
		return (HMODULE)pModule;
	}
	SetLastError(ERROR_FILE_NOT_FOUND);
	return NULL;
}

BOOL FreeLibrary(HMODULE)
{
	return TRUE;
}

FARPROC GetProcAddress(HMODULE, LPCSTR)
{
	/* DLL の関数は使えない（呼ぶ側は NULL を想定している） */
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* リソース																	 */
/* ------------------------------------------------------------------------- */
struct W32Res {
	DWORD				dwMagic;
	const W32ResEntry	*pEntry;
	BOOL				bLoaded;
	std::vector<BYTE>	aData;
};

#define W32RES_MAGIC	0x57335253		/* 'W3RS' */

static std::map<const W32ResEntry *, W32Res *> s_mapRes;

static BOOL ReadWholeFile(const char *pszPath, std::vector<BYTE> &aData)
{
	FILE *fp = fopen(pszPath, "rb");
	if (fp == NULL) {
		return FALSE;
	}
	fseek(fp, 0, SEEK_END);
	long lSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	aData.resize(lSize > 0 ? lSize : 0);
	if (lSize > 0) {
		fread(&aData[0], 1, lSize, fp);
	}
	fclose(fp);
	return TRUE;
}

HRSRC FindResource(HMODULE hModule, LPCSTR pszName, LPCSTR pszType)
{
	W32Module *pModule = CheckModule(hModule);
	if (pModule == NULL || pszName == NULL || pszType == NULL) {
		return NULL;
	}
	std::string strType = IS_INTRESOURCE(pszType) ? "" : ToLower(pszType);
	for (const W32ResEntry *p = g_aW32ResEntry; p->pszModule; p ++) {
		if (ToLower(p->pszModule) != pModule->strName) {
			continue;
		}
		if (ToLower(p->pszType) != strType) {
			continue;
		}
		if (IS_INTRESOURCE(pszName)) {
			if (p->pszName || p->nID != (int)(ULONG_PTR)pszName) {
				continue;
			}
		} else if (pszName[0] == '#') {
			/* "#123" は数値ID */
			if (p->pszName || p->nID != atoi(pszName + 1)) {
				continue;
			}
		} else {
			if (p->pszName == NULL || strcasecmp(p->pszName, pszName) != 0) {
				continue;
			}
		}
		std::map<const W32ResEntry *, W32Res *>::iterator it = s_mapRes.find(p);
		if (it != s_mapRes.end()) {
			return (HRSRC)it->second;
		}
		W32Res *pRes = new W32Res;
		pRes->dwMagic = W32RES_MAGIC;
		pRes->pEntry = p;
		pRes->bLoaded = FALSE;
		s_mapRes[p] = pRes;
		return (HRSRC)pRes;
	}
	return NULL;
}

static W32Res *CheckRes(HRSRC hRes)
{
	W32Res *pRes = (W32Res *)hRes;
	if (pRes == NULL || pRes->dwMagic != W32RES_MAGIC) {
		return NULL;
	}
	return pRes;
}

HGLOBAL LoadResource(HMODULE, HRSRC hRes)
{
	W32Res *pRes = CheckRes(hRes);
	if (pRes == NULL || pRes->pEntry->bNoFile) {
		return NULL;
	}
	if (!pRes->bLoaded) {
		if (!ReadWholeFile(pRes->pEntry->pszPath, pRes->aData)) {
			printf("[w32] リソースのファイルがありません: %s\n", pRes->pEntry->pszPath);
			return NULL;
		}
		pRes->bLoaded = TRUE;
	}
	return (HGLOBAL)pRes;
}

LPVOID LockResource(HGLOBAL hResData)
{
	W32Res *pRes = CheckRes((HRSRC)hResData);
	if (pRes == NULL || pRes->aData.empty()) {
		return NULL;
	}
	return &pRes->aData[0];
}

DWORD SizeofResource(HMODULE hModule, HRSRC hRes)
{
	W32Res *pRes = CheckRes(hRes);
	if (pRes == NULL) {
		return 0;
	}
	if (!pRes->bLoaded) {
		LoadResource(hModule, hRes);
	}
	return (DWORD)pRes->aData.size();
}

BOOL FreeResource(HGLOBAL)
{
	return TRUE;
}

/* リソースの元ファイルのパス（Web版の差し替え実装から使う） */
const char *W32GetResourcePath(HRSRC hRes)
{
	W32Res *pRes = CheckRes(hRes);
	return pRes ? pRes->pEntry->pszPath : NULL;
}

int LoadString(HINSTANCE hInst, UINT uID, LPSTR pszBuffer, int nMax)
{
	W32Module *pModule = CheckModule(hInst);
	if (pModule == NULL || pszBuffer == NULL || nMax <= 0) {
		return 0;
	}
	for (const W32StrEntry *p = g_aW32StrEntry; p->pszModule; p ++) {
		if (ToLower(p->pszModule) == pModule->strName && p->nID == (int)uID) {
			lstrcpyn(pszBuffer, p->pszText, nMax);
			return (int)strlen(pszBuffer);
		}
	}
	pszBuffer[0] = 0;
	return 0;
}

HANDLE LoadImage(HINSTANCE, LPCSTR, UINT, int, int, UINT)
{
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* ファイル																	 */
/* ------------------------------------------------------------------------- */
struct W32File {
	DWORD	dwMagic;
	FILE	*fp;
	BOOL	bWrite;
};

#define W32FILE_MAGIC	0x57334649		/* 'W3FI' */

static W32File *CheckFile(HANDLE hFile)
{
	W32File *pFile = (W32File *)hFile;
	if (pFile == NULL || hFile == INVALID_HANDLE_VALUE || (ULONG_PTR)hFile < 0x10000) {
		return NULL;
	}
	if (pFile->dwMagic != W32FILE_MAGIC) {
		return NULL;
	}
	return pFile;
}

static BOOL FileExists(const std::string &strPath)
{
	struct stat st;
	return (stat(strPath.c_str(), &st) == 0) ? TRUE : FALSE;
}

HANDLE CreateFile(LPCSTR pszName, DWORD dwAccess, DWORD, LPSECURITY_ATTRIBUTES,
		DWORD dwCreation, DWORD, HANDLE)
{
	std::string strPath = NormPath(pszName);
	BOOL bExists = FileExists(strPath);
	BOOL bWrite = (dwAccess & GENERIC_WRITE) ? TRUE : FALSE;
	const char *pszMode = NULL;

	switch (dwCreation) {
	case CREATE_NEW:
		if (bExists) {
			SetLastError(ERROR_ALREADY_EXISTS);
			return INVALID_HANDLE_VALUE;
		}
		pszMode = "w+b";
		break;
	case CREATE_ALWAYS:
		pszMode = "w+b";
		break;
	case OPEN_EXISTING:
		if (!bExists) {
			SetLastError(ERROR_FILE_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}
		pszMode = bWrite ? "r+b" : "rb";
		break;
	case OPEN_ALWAYS:
		pszMode = bExists ? (bWrite ? "r+b" : "rb") : "w+b";
		break;
	case TRUNCATE_EXISTING:
		if (!bExists) {
			SetLastError(ERROR_FILE_NOT_FOUND);
			return INVALID_HANDLE_VALUE;
		}
		pszMode = "w+b";
		break;
	default:
		return INVALID_HANDLE_VALUE;
	}
	FILE *fp = fopen(strPath.c_str(), pszMode);
	if (fp == NULL) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}
	W32File *pFile = new W32File;
	pFile->dwMagic = W32FILE_MAGIC;
	pFile->fp = fp;
	pFile->bWrite = bWrite || (pszMode[0] == 'w');
	SetLastError((dwCreation == OPEN_ALWAYS || dwCreation == CREATE_ALWAYS) && bExists ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
	return (HANDLE)pFile;
}

BOOL ReadFile(HANDLE hFile, LPVOID pBuffer, DWORD dwSize, LPDWORD pdwRead, LPOVERLAPPED)
{
	W32File *pFile = CheckFile(hFile);
	if (pdwRead) {
		*pdwRead = 0;
	}
	if (pFile == NULL) {
		return FALSE;
	}
	size_t n = fread(pBuffer, 1, dwSize, pFile->fp);
	if (pdwRead) {
		*pdwRead = (DWORD)n;
	}
	return TRUE;
}

BOOL WriteFile(HANDLE hFile, LPCVOID pBuffer, DWORD dwSize, LPDWORD pdwWritten, LPOVERLAPPED)
{
	W32File *pFile = CheckFile(hFile);
	if (pdwWritten) {
		*pdwWritten = 0;
	}
	if (pFile == NULL) {
		return FALSE;
	}
	size_t n = fwrite(pBuffer, 1, dwSize, pFile->fp);
	if (pdwWritten) {
		*pdwWritten = (DWORD)n;
	}
	return (n == dwSize) ? TRUE : FALSE;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistance, PLONG, DWORD dwMethod)
{
	W32File *pFile = CheckFile(hFile);
	if (pFile == NULL) {
		return INVALID_SET_FILE_POINTER;
	}
	int nWhence = (dwMethod == FILE_END) ? SEEK_END : (dwMethod == FILE_CURRENT) ? SEEK_CUR : SEEK_SET;
	if (fseek(pFile->fp, lDistance, nWhence) != 0) {
		return INVALID_SET_FILE_POINTER;
	}
	return (DWORD)ftell(pFile->fp);
}

DWORD GetFileSize(HANDLE hFile, LPDWORD pdwHigh)
{
	W32File *pFile = CheckFile(hFile);
	if (pdwHigh) {
		*pdwHigh = 0;
	}
	if (pFile == NULL) {
		return INVALID_FILE_SIZE;
	}
	long lPos = ftell(pFile->fp);
	fseek(pFile->fp, 0, SEEK_END);
	long lSize = ftell(pFile->fp);
	fseek(pFile->fp, lPos, SEEK_SET);
	return (DWORD)lSize;
}

BOOL SetEndOfFile(HANDLE hFile)
{
	W32File *pFile = CheckFile(hFile);
	if (pFile == NULL) {
		return FALSE;
	}
	fflush(pFile->fp);
	return (ftruncate(fileno(pFile->fp), ftell(pFile->fp)) == 0) ? TRUE : FALSE;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
	W32File *pFile = CheckFile(hFile);
	if (pFile == NULL) {
		return FALSE;
	}
	fflush(pFile->fp);
	return TRUE;
}

BOOL CloseHandle(HANDLE hObject)
{
	W32File *pFile = CheckFile(hObject);
	if (pFile == NULL) {
		/* ミューテックスなど形だけのハンドル */
		return TRUE;
	}
	fclose(pFile->fp);
	if (pFile->bWrite) {
		ScheduleSync();
	}
	pFile->dwMagic = 0;
	delete pFile;
	return TRUE;
}

BOOL DeleteFile(LPCSTR pszName)
{
	BOOL bRet = (unlink(NormPath(pszName).c_str()) == 0) ? TRUE : FALSE;
	if (bRet) {
		ScheduleSync();
	}
	return bRet;
}

BOOL CopyFile(LPCSTR pszSrc, LPCSTR pszDst, BOOL bFailIfExists)
{
	std::string strDst = NormPath(pszDst);
	if (bFailIfExists && FileExists(strDst)) {
		return FALSE;
	}
	std::vector<BYTE> aData;
	if (!ReadWholeFile(NormPath(pszSrc).c_str(), aData)) {
		return FALSE;
	}
	FILE *fp = fopen(strDst.c_str(), "wb");
	if (fp == NULL) {
		return FALSE;
	}
	if (!aData.empty()) {
		fwrite(&aData[0], 1, aData.size(), fp);
	}
	fclose(fp);
	ScheduleSync();
	return TRUE;
}

BOOL MoveFile(LPCSTR pszSrc, LPCSTR pszDst)
{
	BOOL bRet = (rename(NormPath(pszSrc).c_str(), NormPath(pszDst).c_str()) == 0) ? TRUE : FALSE;
	if (bRet) {
		ScheduleSync();
	}
	return bRet;
}

BOOL CreateDirectory(LPCSTR pszName, LPSECURITY_ATTRIBUTES)
{
	std::string strPath = NormPath(pszName);
	if (mkdir(strPath.c_str(), 0777) != 0) {
		SetLastError(errno == EEXIST ? ERROR_ALREADY_EXISTS : ERROR_FILE_NOT_FOUND);
		return FALSE;
	}
	ScheduleSync();
	return TRUE;
}

DWORD GetFileAttributes(LPCSTR pszName)
{
	struct stat st;
	if (stat(NormPath(pszName).c_str(), &st) != 0) {
		return INVALID_FILE_ATTRIBUTES;
	}
	return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

DWORD GetCurrentDirectory(DWORD nSize, LPSTR pszBuffer)
{
	std::string strDir = BaseDir();
	if (pszBuffer && nSize) {
		lstrcpyn(pszBuffer, strDir.c_str(), (int)nSize);
	}
	return (DWORD)strDir.size();
}

BOOL SetCurrentDirectory(LPCSTR)
{
	return TRUE;
}

UINT GetWindowsDirectory(LPSTR pszBuffer, UINT nSize)
{
	if (pszBuffer && nSize) {
		lstrcpyn(pszBuffer, "/tmp", (int)nSize);
	}
	return 4;
}

DWORD GetTempPath(DWORD nSize, LPSTR pszBuffer)
{
	if (pszBuffer && nSize) {
		lstrcpyn(pszBuffer, "/tmp/", (int)nSize);
	}
	return 5;
}

/* ------------------------------------------------------------------------- */
/* INI ファイル																 */
/* ------------------------------------------------------------------------- */
/* 1 行ずつ持ち、書き換える時もコメントや並び順を崩さない */
static BOOL ReadLines(const std::string &strPath, std::vector<std::string> &astrLine)
{
	astrLine.clear();
	std::vector<BYTE> aData;
	if (!ReadWholeFile(strPath.c_str(), aData)) {
		return FALSE;
	}
	std::string strLine;
	for (size_t i = 0; i < aData.size(); i ++) {
		char c = (char)aData[i];
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			astrLine.push_back(strLine);
			strLine.clear();
			continue;
		}
		strLine += c;
	}
	if (!strLine.empty()) {
		astrLine.push_back(strLine);
	}
	return TRUE;
}

static std::string Trim(const std::string &str)
{
	size_t nStart = str.find_first_not_of(" \t");
	if (nStart == std::string::npos) {
		return "";
	}
	size_t nEnd = str.find_last_not_of(" \t");
	return str.substr(nStart, nEnd - nStart + 1);
}

/* セクション名の行なら名前を返す */
static BOOL IsSection(const std::string &strLine, std::string &strName)
{
	std::string str = Trim(strLine);
	if (str.size() < 2 || str[0] != '[') {
		return FALSE;
	}
	size_t nEnd = str.find(']');
	if (nEnd == std::string::npos) {
		return FALSE;
	}
	strName = Trim(str.substr(1, nEnd - 1));
	return TRUE;
}

/* キーの行ならキーと値を返す */
static BOOL IsKey(const std::string &strLine, std::string &strKey, std::string &strValue)
{
	std::string str = Trim(strLine);
	if (str.empty() || str[0] == ';') {
		return FALSE;
	}
	size_t nPos = str.find('=');
	if (nPos == std::string::npos) {
		return FALSE;
	}
	strKey = Trim(str.substr(0, nPos));
	strValue = Trim(str.substr(nPos + 1));
	/* 値を囲む "" は外す（Windows と同じ） */
	if (strValue.size() >= 2 && strValue[0] == '"' && strValue[strValue.size() - 1] == '"') {
		strValue = strValue.substr(1, strValue.size() - 2);
	}
	return TRUE;
}

static BOOL IniGet(LPCSTR pszFile, LPCSTR pszApp, LPCSTR pszKey, std::string &strValue)
{
	std::vector<std::string> astrLine;
	if (!ReadLines(NormPath(pszFile), astrLine)) {
		return FALSE;
	}
	BOOL bIn = FALSE;
	for (size_t i = 0; i < astrLine.size(); i ++) {
		std::string strName, strKey, strVal;
		if (IsSection(astrLine[i], strName)) {
			bIn = (strcasecmp(strName.c_str(), pszApp) == 0) ? TRUE : FALSE;
			continue;
		}
		if (bIn && IsKey(astrLine[i], strKey, strVal) && strcasecmp(strKey.c_str(), pszKey) == 0) {
			strValue = strVal;
			return TRUE;
		}
	}
	return FALSE;
}

UINT GetPrivateProfileInt(LPCSTR pszApp, LPCSTR pszKey, INT nDefault, LPCSTR pszFile)
{
	std::string strValue;
	if (pszApp == NULL || pszKey == NULL || !IniGet(pszFile, pszApp, pszKey, strValue)) {
		return (UINT)nDefault;
	}
	return (UINT)atoi(strValue.c_str());
}

DWORD GetPrivateProfileString(LPCSTR pszApp, LPCSTR pszKey, LPCSTR pszDefault, LPSTR pszRet, DWORD nSize, LPCSTR pszFile)
{
	if (pszRet == NULL || nSize == 0) {
		return 0;
	}
	std::string strValue;
	if (pszApp == NULL || pszKey == NULL || !IniGet(pszFile, pszApp, pszKey, strValue)) {
		strValue = pszDefault ? pszDefault : "";
	}
	lstrcpyn(pszRet, strValue.c_str(), (int)nSize);
	return (DWORD)strlen(pszRet);
}

BOOL WritePrivateProfileString(LPCSTR pszApp, LPCSTR pszKey, LPCSTR pszValue, LPCSTR pszFile)
{
	if (pszApp == NULL) {
		return FALSE;
	}
	std::string strPath = NormPath(pszFile);
	std::vector<std::string> astrLine;
	ReadLines(strPath, astrLine);

	/* セクションの範囲を探す */
	int nSecStart = -1, nSecEnd = (int)astrLine.size();
	for (int i = 0; i < (int)astrLine.size(); i ++) {
		std::string strName;
		if (IsSection(astrLine[i], strName)) {
			if (nSecStart >= 0) {
				nSecEnd = i;
				break;
			}
			if (strcasecmp(strName.c_str(), pszApp) == 0) {
				nSecStart = i;
			}
		}
	}

	if (pszKey == NULL) {
		/* セクションごと消す */
		if (nSecStart >= 0) {
			astrLine.erase(astrLine.begin() + nSecStart, astrLine.begin() + nSecEnd);
		}
	} else if (nSecStart < 0) {
		if (pszValue) {
			astrLine.push_back(std::string("[") + pszApp + "]");
			astrLine.push_back(std::string(pszKey) + "=" + pszValue);
		}
	} else {
		int nHit = -1;
		for (int i = nSecStart + 1; i < nSecEnd; i ++) {
			std::string strKey, strVal;
			if (IsKey(astrLine[i], strKey, strVal) && strcasecmp(strKey.c_str(), pszKey) == 0) {
				nHit = i;
				break;
			}
		}
		if (nHit >= 0) {
			if (pszValue) {
				astrLine[nHit] = std::string(pszKey) + "=" + pszValue;
			} else {
				astrLine.erase(astrLine.begin() + nHit);
			}
		} else if (pszValue) {
			/* セクションの最後（空行の手前）に足す */
			int nIns = nSecEnd;
			while (nIns > nSecStart + 1 && Trim(astrLine[nIns - 1]).empty()) {
				nIns --;
			}
			astrLine.insert(astrLine.begin() + nIns, std::string(pszKey) + "=" + pszValue);
		}
	}

	FILE *fp = fopen(strPath.c_str(), "wb");
	if (fp == NULL) {
		return FALSE;
	}
	for (size_t i = 0; i < astrLine.size(); i ++) {
		fputs(astrLine[i].c_str(), fp);
		fputs("\r\n", fp);
	}
	fclose(fp);
	ScheduleSync();
	return TRUE;
}

/* fopen で直接書かれたファイル（スクリーンショットなど）を保存するために外から呼ぶ */
void W32SyncFiles(void)
{
	ScheduleSync();
}
