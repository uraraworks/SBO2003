/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	SboSvrDataUtf8.cpp											 */
/* 概要：		保存データを Shift_JIS から UTF-8 へ読み替える				 */
/* ========================================================================= */
/*
 * 2026-11 の UTF-8 対応（段階 3）で、ゲームの中の文字列を Shift_JIS から UTF-8 に変えた。
 * SBODATA の .dat はこれまで Shift_JIS で書かれているので、サーバーを起動した時に一度だけ
 *   1. SBODATA をまるごと SBODATA_SJIS_<日時> へコピーしてバックアップを取る
 *   2. 読み込んだ情報の文字列（名前・台詞・メッセージなど）を UTF-8 に読み替える
 *   3. 全部保存し直し、SBODATA/SboDataUtf8.txt（目印）を作る
 * をする。目印があれば何もしない。
 * 読み込み処理はいじらず、読み込んだ後のメモリ上の情報を直すので、
 * .dat の形式ごとの違いを気にしなくて済む。
 */
#include "include.h"
#include "../../Common/Info/Item/InfoItem.h"
#include "../../Common/LibInfo/LibInfoItem.h"
#include "../../Common/Info/InfoJob.h"
#include "Glbldata.h"
#include "SaveLoad/SaveLoad.h"
#include "SboSvrDataUtf8.h"
#include <dirent.h>
#include <string>

/* 文字コード（platform/w32_sjis.cpp） */
std::string W32SjisToUtf8Str (const char *pszSjis, int nLen);

/* SboSvr.cpp の保存処理 */
extern BOOL SaveMapPartsInfo (void);
extern BOOL SaveMapInfo (void);
extern BOOL SaveLeaveInfo (void);

#define DATAUTF8_DIR		"SBODATA"
#define DATAUTF8_MARK		"SBODATA\\SboDataUtf8.txt"

static int s_nConv;			/* 読み替えた文字列の数 */
static int s_nCut;			/* 欄に入りきらず後ろを切った数 */

/* ------------------------------------------------------------------------- */
/* 固定長の欄の文字列を読み替える											 */
/* ------------------------------------------------------------------------- */
static void ConvFixed(LPSTR psz, int nSize)
{
	std::string str;

	if ((psz == NULL) || (nSize <= 0)) {
		return;
	}
	psz[nSize - 1] = 0;
	if (SboTextIsSingleByteOnly (psz)) {
		return;		/* ASCII だけなら同じ */
	}
	str = W32SjisToUtf8Str (psz, -1);
	if ((int)str.size () > nSize - 1) {
		s_nCut ++;
		printf ("[utf8] 欄に入りきらないので後ろを切りました: %s\n", str.c_str ());
	}
	ZeroMemory (psz, nSize);
	SboTextCopy (psz, nSize, str.c_str ());
	s_nConv ++;
}

/* ------------------------------------------------------------------------- */
/* new char[] で確保した文字列を読み替える（確保し直す）						 */
/* ------------------------------------------------------------------------- */
static void ConvHeap(LPSTR &psz)
{
	std::string str;
	LPSTR pszNew;

	if ((psz == NULL) || SboTextIsSingleByteOnly (psz)) {
		return;
	}
	str = W32SjisToUtf8Str (psz, -1);
	pszNew = new char[str.size () + 1];
	CopyMemory (pszNew, str.c_str (), str.size () + 1);
	SAFE_DELETE_ARRAY (psz);
	psz = pszNew;
	s_nConv ++;
}

/* ------------------------------------------------------------------------- */
/* CmyString などで持っている文字列を読み替えた結果を返す					 */
/* ------------------------------------------------------------------------- */
static std::string ConvStr(LPCSTR psz)
{
	if ((psz == NULL) || SboTextIsSingleByteOnly (psz)) {
		return std::string (psz ? psz : "");
	}
	s_nConv ++;
	return W32SjisToUtf8Str (psz, -1);
}

/* ------------------------------------------------------------------------- */
/* 読み込んだ情報の文字列を全部読み替える									 */
/* ------------------------------------------------------------------------- */
static void ConvAll(void)
{
	int i, j, k;

	/* キャラ（アカウント名・パスワードは英数字だが念のため） */
	for (i = 0; i < g_aCharInfo.GetSize (); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		ConvFixed (pChar->szName, sizeof (pChar->szName));
		ConvFixed (pChar->szAcc, sizeof (pChar->szAcc));
		ConvFixed (pChar->szPas, sizeof (pChar->szPas));
		ConvHeap (pChar->pszNpcSpeech);
		ConvHeap (pChar->pszSpeech);
	}

	/* マップ（マップ名・メッセージ・店の台詞） */
	for (i = 0; i < g_aMapInfo.GetSize (); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		ConvHeap (pMap->pszName);
		if (pMap->pMsg) {
			for (j = 0; j < pMap->nMsgCount; j ++) {
				ConvHeap (pMap->pMsg[j]);
			}
		}
		if (pMap->pShopInfo) {
			for (k = 0; k < pMap->byCountShop; k ++) {
				for (j = 0; j < MAX_SHOPMSG; j ++) {
					ConvHeap (pMap->pShopInfo[k].apMsg[j]);
				}
			}
		}
	}

	/* アイテム */
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		ConvFixed (pItem->szName, sizeof (pItem->szName));
		ConvHeap (pItem->pMsg);
		if ((pItem->byType == ITEMTYPE_MSG) && pItem->pData) {
			PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
			ConvFixed (pInfo->szCharEfc, sizeof (pInfo->szCharEfc));
			ConvHeap (pInfo->pMsgTo);
			ConvHeap (pInfo->pMsgMe);
		}
	}
	/* 新しい形のアイテム情報（SboDataItemInfoTest.dat。遊びには使っていない） */
	for (i = 0; i < g_LibInfoItem.GetCount (); i ++) {
		CInfoItemBase *pInfoItem = g_LibInfoItem.Get (i);
		if (pInfoItem == NULL) {
			continue;
		}
		pInfoItem->SetName (ConvStr (pInfoItem->GetName ()).c_str ());
		pInfoItem->SetMsg (ConvStr (pInfoItem->GetMsg ()).c_str ());
		if (pInfoItem->GetItemTypeID () == ITEMTYPE_MSG) {
			PCInfoItemMSG pMsg = (PCInfoItemMSG)pInfoItem;
			pMsg->SetCharEfc (ConvStr (pMsg->GetCharEfc ()).c_str ());
			pMsg->SetMsgTo (ConvStr (pMsg->GetMsgTo ()).c_str ());
			pMsg->SetMsgMe (ConvStr (pMsg->GetMsgMe ()).c_str ());
		}
	}

	/* 魔法・利用者・装備職業 */
	for (i = 0; i < g_aMagicInfo.GetSize (); i ++) {
		PMAGICINFO pInfo = g_aMagicInfo[i];
		ConvFixed (pInfo->szName, sizeof (pInfo->szName));
		ConvFixed (pInfo->szExplain, sizeof (pInfo->szExplain));
	}
	for (i = 0; i < g_aMagicUserInfo.GetSize (); i ++) {
		PMAGICUSERINFO pInfo = g_aMagicUserInfo[i];
		ConvFixed (pInfo->szName, sizeof (pInfo->szName));
	}
	for (i = 0; i < g_aEquUserInfo.GetSize (); i ++) {
		PEQUUSERINFO pInfo = g_aEquUserInfo[i];
		ConvFixed (pInfo->szName, sizeof (pInfo->szName));
	}

	/* 職業 */
	for (i = 0; i < g_JobInfo.GetCount (); i ++) {
		PJOBINFO pInfo = g_JobInfo.GetPtr (i);
		if (pInfo && pInfo->pszName && !SboTextIsSingleByteOnly (pInfo->pszName)) {
			std::string str = ConvStr (pInfo->pszName);
			g_JobInfo.SetJobName (pInfo, str.c_str ());
		}
	}

	/* ステータステンプレート */
	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
		ConvFixed (pInfo->szName, sizeof (pInfo->szName));
	}
}

/* ------------------------------------------------------------------------- */
/* SBODATA をまるごとバックアップする（戻り値は成功したか）					 */
/* ------------------------------------------------------------------------- */
static BOOL Backup(char *pszBackupDir, int nSize)
{
	DIR *pDir;
	struct dirent *pEnt;
	SYSTEMTIME sysTime;
	char szSrc[MAX_PATH], szDst[MAX_PATH];
	int nCount = 0;

	GetLocalTime (&sysTime);
	_snprintf (pszBackupDir, nSize, "SBODATA_SJIS_%04d%02d%02d_%02d%02d%02d",
			sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	if (!CreateDirectory (pszBackupDir, NULL)) {
		printf ("[utf8] バックアップのフォルダ %s を作れませんでした\n", pszBackupDir);
		return FALSE;
	}

	pDir = opendir (DATAUTF8_DIR);
	if (pDir == NULL) {
		return TRUE;	/* データが無い */
	}
	while ((pEnt = readdir (pDir)) != NULL) {
		if (pEnt->d_name[0] == '.') {
			continue;
		}
		_snprintf (szSrc, sizeof (szSrc), "%s\\%s", DATAUTF8_DIR, pEnt->d_name);
		if (GetFileAttributes (szSrc) & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		_snprintf (szDst, sizeof (szDst), "%s\\%s", pszBackupDir, pEnt->d_name);
		if (!CopyFile (szSrc, szDst, FALSE)) {
			printf ("[utf8] %s をバックアップできませんでした\n", szSrc);
			closedir (pDir);
			return FALSE;
		}
		nCount ++;
	}
	closedir (pDir);
	printf ("[utf8] SBODATA の %d ファイルを %s へバックアップしました\n", nCount, pszBackupDir);
	return TRUE;
}

/* ========================================================================= */
/* 関数名：	DataUtf8Migrate													 */
/* 内容：	古い Shift_JIS のデータなら、バックアップを取ってから			 */
/*			UTF-8 にして保存し直す（戻り値は起動を続けてよいか）			 */
/*			全部の情報を読み込んだ後に呼ぶ									 */
/* ========================================================================= */
BOOL DataUtf8Migrate(void)
{
	char szBackupDir[MAX_PATH];
	HANDLE hFile;
	DWORD dwBytes;
	static const char s_szMark[] =
		"このフォルダの .dat の文字列は UTF-8 です。\r\n"
		"（2026-11 の UTF-8 対応で、サーバーが Shift_JIS から読み替えた目印です。消さないでください）\r\n";

	/* もう UTF-8 になっている */
	if (GetFileAttributes (DATAUTF8_MARK) != INVALID_FILE_ATTRIBUTES) {
		return TRUE;
	}

	ZeroMemory (szBackupDir, sizeof (szBackupDir));
	if (GetFileAttributes ("SBODATA\\SboDataCharInfo.dat") != INVALID_FILE_ATTRIBUTES) {
		printf ("[utf8] SBODATA を Shift_JIS から UTF-8 に読み替えます\n");
		/* バックアップが取れなければ、データを書き換えずに起動をやめる */
		if (!Backup (szBackupDir, sizeof (szBackupDir))) {
			printf ("[utf8] バックアップに失敗したので、データを書き換えずに止めます\n");
			return FALSE;
		}
		s_nConv = 0;
		s_nCut = 0;
		ConvAll ();

		SaveCharInfo ();
		SaveMapPartsInfo ();
		SaveMapInfo ();
		SaveComEfcInfo ();
		SaveEfcInfo ();
		SaveItemInfo ();
		SaveStTmpInfo ();
		SaveLeaveInfo ();
		SaveSubGameInfo ();
		SaveJobInfo ();
		SaveMagicUserInfo ();
		SaveMagicInfo ();
		SaveEquUserInfo ();
		printf ("[utf8] %d 個の文字列を UTF-8 にして保存し直しました（切ったもの %d 個）\n", s_nConv, s_nCut);
	}

	/* 目印を作る */
	CreateDirectory (DATAUTF8_DIR, NULL);
	hFile = CreateFile (DATAUTF8_MARK, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE) {
		WriteFile (hFile, s_szMark, (DWORD)strlen (s_szMark), &dwBytes, NULL);
		CloseHandle (hFile);
	}
	return TRUE;
}

/* Copyright(C)URARA-works 2026 */
