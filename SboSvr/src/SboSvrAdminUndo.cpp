/* ========================================================================= */
/* ファイル名：	SboSvrAdminUndo.cpp											 */
/* 内容：		管理者画面の「元に戻す」（画面単位のアンドゥ）				 */
/*				仕組みは SboSvrAdminUndo.h と docs/admin-undo.md を参照		 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "../Common/Info/InfoJob.h"
#include "SboSvrAdminUndo.h"
#include "SboSvrNpcRoute.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdio.h>

/* ========================================================================= */
/* 外部の関数																 */
/* ========================================================================= */
int		GetMapNo			(DWORD);										/* マップ番号取得（SboSvrRecvCmd.cpp） */
int		GetEventDataSize	(BYTE);											/* イベント種別毎のデータサイズ（SboSvrRecvCmd.cpp） */
/* 記録しておいたパケットを管理者の操作として処理し直す（SboSvrRecvCmd.cpp） */
void	RecvCmdReplayAdmin	(const std::vector<BYTE> &aPacket, DWORD pID, DWORD dwMapNo);
/* 送信の横取り（server/CUraraSockTCP_svr.cpp） */
void	SvrSendCaptureBegin	(void);
BOOL	SvrSendCaptureEnd	(std::vector<BYTE> &aDst);

/* ========================================================================= */
/* 定数																		 */
/* ========================================================================= */
#define ADMINUNDO_MERGETIME		3000					/* 同じ画面で続けて送った分を 1 回にまとめる間隔（ミリ秒） */
#define ADMINUNDO_MAXSTEP		50						/* 1 画面で覚えておく回数 */
#define ADMINUNDO_MAXBYTES		(16 * 1024 * 1024)		/* 全部で覚えておく量の上限（超えたら古い順に捨てる） */
#define ADMINUNDO_NOMAP			((DWORD)-1)

/* ========================================================================= */
/* 記録																		 */
/* ========================================================================= */

/* 変わる対象（コマンドごとに a・b・c の意味が違う。下の BuildState を参照） */
struct ADMINUNDO_KEY {
	BYTE	byCmd;
	DWORD	a, b, c;
	bool operator < (const ADMINUNDO_KEY &o) const {
		if (byCmd != o.byCmd) return byCmd < o.byCmd;
		if (a != o.a) return a < o.a;
		if (b != o.b) return b < o.b;
		return c < o.c;
	}
};

/* 対象 1 つ分の変更 */
struct ADMINUNDO_ENTRY {
	ADMINUNDO_KEY		Key;
	std::vector<BYTE>	aBefore;		/* 変更前の状態（このパケットを処理すると戻る） */
	std::vector<BYTE>	aAfter;			/* 変更後の状態（戻す前に、今もこのままか確かめる） */
	std::string			strIdent;		/* 対象の見分け（キャラなら名前。入れ替わっていたら戻さない） */
	DWORD				dwMapNo;		/* 処理し直す時のマップ番号（メッセージ・店用） */
};

/* 元に戻す 1 回分 */
struct ADMINUNDO_STEP {
	DWORD							dwID;			/* 番号（1 から） */
	BYTE							byScreen;		/* 画面(ADMINSCR_*) */
	DWORD							pID;			/* 変えた接続（API なら 0） */
	std::string						strSource;		/* 変えた人（キャラ名・API の名前） */
	DWORD							dwTickLast;		/* 最後に足した時刻（まとめる判定用） */
	time_t							tLast;			/* 最後に足した時刻（案内用） */
	std::vector<ADMINUNDO_ENTRY>	aEntry;			/* 変更（古い順） */
	size_t							nBytes;			/* 覚えている量 */
};

static std::vector<ADMINUNDO_STEP>	s_aStep;			/* 全画面の記録（古い順） */
static size_t						s_nBytes = 0;		/* 覚えている量の合計 */
static DWORD						s_dwNextStepID = 1;
static std::map<DWORD, BYTE>		s_mapScreen;		/* 接続ごとの開いている画面 */

/* 処理中のコマンドの記録（AdminUndoBefore から AdminUndoAfter まで） */
static BOOL						s_bPending = FALSE;
static ADMINUNDO_ENTRY			s_Pending;
static BYTE						s_byPendingScreen;
static DWORD					s_pIDPending;
static std::string				s_strPendingSource;

/* 元に戻している最中（その処理は記録しない） */
static BOOL						s_bReplaying = FALSE;

/* ========================================================================= */
/* 画面																		 */
/* ========================================================================= */

LPCSTR AdminUndoScreenName(BYTE byScreen)
{
	switch (byScreen) {
	case ADMINSCR_MAPPARTS:			return "パーツ（マップの塗り替え）";
	case ADMINSCR_MAPINFO:			return "マップ情報";
	case ADMINSCR_EVENT:			return "イベント情報";
	case ADMINSCR_MSG:				return "メッセージ";
	case ADMINSCR_SHOP:				return "店情報";
	case ADMINSCR_CHARSTATUS:		return "ステータス情報";
	case ADMINSCR_CHARITEM:			return "所持アイテム情報";
	case ADMINSCR_CHARMAGIC:		return "所持魔法情報";
	case ADMINSCR_STTMP:			return "ステータステンプレート情報";
	case ADMINSCR_ITEM:				return "アイテム一覧";
	case ADMINSCR_CEF:				return "ふきだし";
	case ADMINSCR_EFC:				return "効果いろいろ";
	case ADMINSCR_JOBINFO:			return "職業情報";
	case ADMINSCR_MAGICINFO:		return "魔法情報";
	case ADMINSCR_MAGICUSERINFO:	return "魔法利用者情報";
	case ADMINSCR_EQUUSERINFO:		return "装備職業情報";
	case ADMINSCR_NPCROUTE:			return "NPCの巡回";
	}
	return "（画面なし）";
}

BYTE AdminUndoDefaultScreen(BYTE byCmd)
{
	switch (byCmd) {
	case CMD_CHGMAP:		return ADMINSCR_MAPPARTS;
	case CMD_CHGMAPINFO:	return ADMINSCR_MAPINFO;
	case CMD_CHGEVENT:		return ADMINSCR_EVENT;
	case CMD_CHGMSG:		return ADMINSCR_MSG;
	case CMD_CHGSHOP:		return ADMINSCR_SHOP;
	case CMD_CHARSTATUS:	return ADMINSCR_CHARSTATUS;
	case CMD_CHARITEM:		return ADMINSCR_CHARITEM;
	case CMD_STTMPLATE:		return ADMINSCR_STTMP;
	case CMD_CHGITEM:		return ADMINSCR_ITEM;
	case CMD_CHGCEF:		return ADMINSCR_CEF;
	case CMD_CHGEFC:		return ADMINSCR_EFC;
	case CMD_JOBINFO:		return ADMINSCR_JOBINFO;
	case CMD_MAGICINFO:		return ADMINSCR_MAGICINFO;
	case CMD_MAGICUSERINFO:	return ADMINSCR_MAGICUSERINFO;
	case CMD_EQUUSERINFO:	return ADMINSCR_EQUUSERINFO;
	case CMD_NPCROUTE:		return ADMINSCR_NPCROUTE;
	}
	return ADMINSCR_NONE;
}

void AdminUndoSetScreen(DWORD pID, BYTE byScreen)
{
	if (byScreen >= ADMINSCR_MAX) {
		byScreen = ADMINSCR_NONE;
	}
	s_mapScreen[pID] = byScreen;
}

BYTE AdminUndoGetScreen(DWORD pID)
{
	std::map<DWORD, BYTE>::iterator it = s_mapScreen.find (pID);
	return (it == s_mapScreen.end ()) ? (BYTE)ADMINSCR_NONE : it->second;
}

void AdminUndoOnDisconnect(DWORD pID)
{
	s_mapScreen.erase (pID);
}

/* ========================================================================= */
/* 対象の今の状態をパケットにする											 */
/*   クライアントがそのデータを変える時に送るのと同じ形にする。				 */
/*   対象が無い（追加・範囲外）なら FALSE									 */
/* ========================================================================= */

/* SendCmd の関数が作る送信データを横取りする（送信と受信で同じ形のコマンド用） */
#define CAPTURE(call)	{ SvrSendCaptureBegin (); call; return SvrSendCaptureEnd (aPacket); }

static void PutBytes(std::vector<BYTE> &aPacket, const void *pData, size_t nSize)
{
	const BYTE *p = (const BYTE *)pData;
	aPacket.insert (aPacket.end (), p, p + nSize);
}

static void PutStr(std::vector<BYTE> &aPacket, LPCSTR psz)
{
	if (psz == NULL) {
		psz = "";
	}
	PutBytes (aPacket, psz, strlen (psz) + 1);
}

static PMAPINFO GetMapByNo(DWORD dwMapNo)
{
	if (dwMapNo >= (DWORD)g_aMapInfo.GetSize ()) {
		return NULL;
	}
	return g_aMapInfo[dwMapNo];
}

static BOOL BuildState(const ADMINUNDO_KEY &Key, std::vector<BYTE> &aPacket, std::string &strIdent)
{
	aPacket.clear ();
	strIdent.clear ();

	switch (Key.byCmd) {
	case CMD_CHGMAP:			/* マップの 1 マス（a=マップ番号 b=X c=Y） */
		{
			PMAPINFO pMap = GetMapByNo (Key.a);
			if ((pMap == NULL) || (pMap->pwMap == NULL) || (Key.b >= (DWORD)pMap->nCX) || (Key.c >= (DWORD)pMap->nCY)) {
				return FALSE;
			}
			CMDPARA_CHGMAP Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd	= CMD_CHGMAP;
			Para.dwID	= pMap->dwID;
			Para.nX		= (int)Key.b;
			Para.nY		= (int)Key.c;
			Para.wMap	= pMap->pwMap[Key.b + Key.c * pMap->nCX];
			PutBytes (aPacket, &Para, sizeof (Para));
		}
		return TRUE;

	case CMD_CHGMAPINFO:		/* マップ情報（a=マップ番号） */
		{
			PMAPINFO pMap = GetMapByNo (Key.a);
			if (pMap == NULL) {
				return FALSE;
			}
			CMDPARA_CHGMAPINFO Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd			= CMD_CHGMAPINFO;
			Para.byBGM			= pMap->byBGM;
			Para.byFlg			= pMap->byFlg;
			Para.byLightLevel	= pMap->byLightLevel;
			Para.byWeather		= pMap->byWeather;
			Para.dwID			= pMap->dwID;
			Para.nStartX		= pMap->nStartX;
			Para.nStartY		= pMap->nStartY;
			Para.nEntranceX		= pMap->nEntranceX;
			Para.nEntranceY		= pMap->nEntranceY;
			Para.clChg			= pMap->clChg;
			PutBytes (aPacket, &Para, sizeof (Para));
			PutStr (aPacket, pMap->pszName);
		}
		return TRUE;

	case CMD_CHGEVENT:			/* イベント（a=マップ番号 b=イベント番号） */
		{
			PMAPINFO pMap = GetMapByNo (Key.a);
			if ((pMap == NULL) || (Key.b >= (DWORD)pMap->nEventCount)) {
				return FALSE;
			}
			PEVENTINFO pEvent = &pMap->pEvent[Key.b];
			/* 座標も詳細も無いイベントは、処理し直すと消えてしまうので戻せない */
			if ((pEvent->pData == NULL) && (pEvent->dwX == 0) && (pEvent->dwY == 0)) {
				return FALSE;
			}
			CMDPARA_CHGEVENT Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd		= CMD_CHGEVENT;
			Para.dwID		= pMap->dwID;
			Para.dwDataID	= Key.b;
			Para.dwX		= pEvent->dwX;
			Para.dwY		= pEvent->dwY;
			Para.byType		= pEvent->byType;
			Para.pData		= pEvent->pData;
			PutBytes (aPacket, &Para, sizeof (Para));
			if (pEvent->pData) {
				PutBytes (aPacket, pEvent->pData, GetEventDataSize (pEvent->byType));
			}
			/* イベントは迷路などで消えて番号がずれるので、種類も見分けに使う */
			char szTmp[32];
			snprintf (szTmp, sizeof (szTmp), "%u", (unsigned)pEvent->byType);
			strIdent = szTmp;
		}
		return TRUE;

	case CMD_CHGMSG:			/* メッセージ（a=マップ番号 b=メッセージ番号） */
		{
			PMAPINFO pMap = GetMapByNo (Key.a);
			if ((pMap == NULL) || (Key.b >= (DWORD)pMap->nMsgCount)) {
				return FALSE;
			}
			CMDPARA_DATA3 Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd	= CMD_CHGMSG;
			Para.dwData	= Key.b;
			PutBytes (aPacket, &Para, sizeof (Para));
			PutStr (aPacket, pMap->pMsg[Key.b]);
		}
		return TRUE;

	case CMD_CHGSHOP:			/* 店（a=マップ番号 b=店番号） */
		{
			PMAPINFO pMap = GetMapByNo (Key.a);
			if ((pMap == NULL) || (pMap->pShopInfo == NULL) || (Key.b >= (DWORD)pMap->byCountShop)) {
				return FALSE;
			}
			PSHOPINFO pShop = &pMap->pShopInfo[Key.b];
			CMDPARA_SHOP Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd			= CMD_CHGSHOP;
			Para.byID			= (BYTE)Key.b;
			Para.byCountItem	= pShop->byCountItem;
			CopyMemory (Para.awItem, pShop->awItem, sizeof (Para.awItem));
			CopyMemory (Para.apMsg, pShop->apMsg, sizeof (Para.apMsg));
			PutBytes (aPacket, &Para, sizeof (Para));
			for (int i = 0; i < MAX_SHOPMSG; i ++) {
				if (pShop->apMsg[i]) {
					PutStr (aPacket, pShop->apMsg[i]);
				}
			}
		}
		return TRUE;

	case CMD_CHARSTATUS:		/* キャラのステータス（a=キャラ番号） */
	case CMD_CHARITEM:			/* キャラの所持アイテム（a=キャラ番号） */
		{
			if (Key.a >= (DWORD)g_aCharInfo.GetSize ()) {
				return FALSE;
			}
			PCHARINFO pChar = g_aCharInfo[Key.a];
			/* キャラの欄は使い回されるので、名前で同じキャラか見分ける */
			strIdent = pChar->szName;
			if (Key.byCmd == CMD_CHARSTATUS) {
				CAPTURE (SendCmdCHARSTATUS (0, Key.a));
			}
			CMDPARA_CHARITEM Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd		= CMD_CHARITEM;
			Para.pID		= pChar->pID;
			Para.dwTblID	= Key.a;
			CopyMemory (Para.waItem, pChar->waItem, sizeof (Para.waItem));
			CopyMemory (Para.waEqu, pChar->waEqu, sizeof (Para.waEqu));
			PutBytes (aPacket, &Para, sizeof (Para));
		}
		return TRUE;

	case CMD_STTMPLATE:			/* ステータステンプレート（a=テンプレート番号） */
		{
			if (Key.a >= (DWORD)g_aStTmpInfo.GetSize ()) {
				return FALSE;
			}
			std::vector<BYTE> aTmp (sizeof (CMDPARA_DATA1) + sizeof (STATUSTEMPLATEINFO) + 1024);
			PCMDPARA_DATA1 pPara = (PCMDPARA_DATA1)&aTmp[0];
			pPara->byCmd	= CMD_STTMPLATE;
			pPara->byData	= CMDSTTMPLATETYPE_CHG;
			pPara->dwTblID	= Key.a;
			PBYTE pEnd = SboPackPutSTATUSTEMPLATEINFO (&aTmp[sizeof (CMDPARA_DATA1)], g_aStTmpInfo[Key.a]);
			aPacket.assign (aTmp.begin (), aTmp.begin () + (pEnd - &aTmp[0]));
		}
		return TRUE;

	case CMD_CHGITEM:			/* アイテム（a=アイテム ID） */
		if (Key.a >= (DWORD)g_aItemInfo.GetSize ()) {
			return FALSE;
		}
		CAPTURE (SendCmdCHGITEM (0, Key.a));

	case CMD_CHGCEF:			/* ふきだし（a=番号） */
		if (Key.a >= MAX_COMEFC) {
			return FALSE;
		}
		CAPTURE (SendCmdCHGCEF (0, (WORD)Key.a));

	case CMD_CHGEFC:			/* 効果（a=番号） */
		{
			if (Key.a >= (DWORD)g_EfcInfo.GetCount ()) {
				return FALSE;
			}
			CMDPARA_CHGEFC Para;
			ZeroMemory (&Para, sizeof (Para));
			Para.byCmd	= CMD_CHGEFC;
			Para.wData	= (WORD)Key.a;
			PutBytes (aPacket, &Para, sizeof (Para));
			DWORD dwSize = g_EfcInfo.GetSendDataSize ((int)Key.a);
			PBYTE pData = g_EfcInfo.GetSendData ((int)Key.a);
			if (pData == NULL) {
				return FALSE;
			}
			PutBytes (aPacket, pData, dwSize);
			SAFE_DELETE_ARRAY (pData);
		}
		return TRUE;

	case CMD_JOBINFO:			/* 職業（a=職業 ID） */
		if ((Key.a > 0xFF) || (g_JobInfo.GetPtr ((int)Key.a) == NULL)) {
			return FALSE;
		}
		CAPTURE (SendCmdJOBINFO (0, (BYTE)Key.a));

	case CMD_MAGICINFO:			/* 魔法（a=ID） */
		if (Key.a >= (DWORD)g_aMagicInfo.GetSize ()) {
			return FALSE;
		}
		CAPTURE (SendCmdMAGICINFO (0, (BYTE)Key.a));

	case CMD_MAGICUSERINFO:		/* 魔法利用者（a=ID） */
		if (Key.a >= (DWORD)g_aMagicUserInfo.GetSize ()) {
			return FALSE;
		}
		CAPTURE (SendCmdMAGICUSERINFO (0, (BYTE)Key.a));

	case CMD_EQUUSERINFO:		/* 装備職業（a=ID） */
		if (Key.a >= (DWORD)g_aEquUserInfo.GetSize ()) {
			return FALSE;
		}
		CAPTURE (SendCmdEQUUSERINFO (0, (BYTE)Key.a));

	case CMD_NPCROUTE:			/* NPCの巡回ルート（a=キャラ番号） */
		{
			if (Key.a >= (DWORD)g_aCharInfo.GetSize ()) {
				return FALSE;
			}
			PCHARINFO pChar = g_aCharInfo[Key.a];
			/* キャラの欄は使い回されるので、名前で同じキャラか見分ける */
			strIdent = pChar->szName;
			CMDPARA_NPCROUTE Para;
			NpcRouteMakePara (Para, NPCROUTE_SET, pChar);
			PutBytes (aPacket, &Para, sizeof (Para));
		}
		return TRUE;
	}
	return FALSE;
}

/* ========================================================================= */
/* 届いたパケットから、変わる対象を読み取る									 */
/*   記録しないコマンド・追加・範囲外なら FALSE								 */
/* ========================================================================= */

static BOOL ParseKey(BYTE byCmd, const BYTE *pData, DWORD dwSize, ADMINUNDO_KEY &Key, DWORD &dwMapNo)
{
	ZeroMemory (&Key, sizeof (Key));
	Key.byCmd	= byCmd;
	dwMapNo		= ADMINUNDO_NOMAP;

#define NEED(type)	if (dwSize < sizeof (type)) { return FALSE; }

	switch (byCmd) {
	case CMD_CHGMAP:
		{
			NEED (CMDPARA_CHGMAP);
			PCMDPARA_CHGMAP pPara = (PCMDPARA_CHGMAP)pData;
			int nNo = GetMapNo (pPara->dwID);
			if ((nNo < 0) || (pPara->nX < 0) || (pPara->nY < 0)) {
				return FALSE;
			}
			Key.a = (DWORD)nNo;
			Key.b = (DWORD)pPara->nX;
			Key.c = (DWORD)pPara->nY;
		}
		break;

	case CMD_CHGMAPINFO:
		{
			NEED (CMDPARA_CHGMAPINFO);
			int nNo = GetMapNo (((PCMDPARA_CHGMAPINFO)pData)->dwID);
			if (nNo < 0) {
				return FALSE;
			}
			Key.a = (DWORD)nNo;
		}
		break;

	case CMD_CHGEVENT:
		{
			NEED (CMDPARA_CHGEVENT);
			PCMDPARA_CHGEVENT pPara = (PCMDPARA_CHGEVENT)pData;
			int nNo = GetMapNo (pPara->dwID);
			if (nNo < 0) {
				return FALSE;
			}
			Key.a = (DWORD)nNo;
			Key.b = pPara->dwDataID;
		}
		break;

	case CMD_CHGMSG:
	case CMD_CHGSHOP:
		{
			/* メッセージと店は、送ってきたキャラのいるマップのもの */
			DWORD dwTblID;
			if (byCmd == CMD_CHGMSG) {
				NEED (CMDPARA_DATA3);
				dwTblID	= ((PCMDPARA_DATA3)pData)->dwTblID;
				Key.b	= ((PCMDPARA_DATA3)pData)->dwData;
			} else {
				NEED (CMDPARA_SHOP);
				dwTblID	= ((PCMDPARA_SHOP)pData)->dwTblID;
				Key.b	= ((PCMDPARA_SHOP)pData)->byID;
			}
			if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
				return FALSE;
			}
			Key.a	= g_aCharInfo[dwTblID]->dwMapID;
			dwMapNo	= Key.a;
		}
		break;

	case CMD_CHARSTATUS:
		NEED (CMDPARA_CHARSTATUS);
		Key.a = ((PCMDPARA_CHARSTATUS)pData)->dwTblID;
		break;

	case CMD_CHARITEM:
		NEED (CMDPARA_CHARITEM);
		Key.a = ((PCMDPARA_CHARITEM)pData)->dwTblID;
		break;

	case CMD_STTMPLATE:
		NEED (CMDPARA_DATA1);
		/* 追加は戻せない（変更だけ） */
		if (((PCMDPARA_DATA1)pData)->byData != CMDSTTMPLATETYPE_CHG) {
			return FALSE;
		}
		Key.a = ((PCMDPARA_DATA1)pData)->dwTblID;
		break;

	case CMD_CHGITEM:
		{
			/* ID は名前より前の固定部分にある */
			CMDPARA_CHGITEM Para;
			if (SboPackGetCHGITEM (&Para, pData, pData + dwSize) == NULL) {
				return FALSE;
			}
			Key.a = Para.dwID;
		}
		break;

	case CMD_CHGCEF:
		NEED (CMDPARA_CHGCEF);
		Key.a = ((PCMDPARA_CHGCEF)pData)->wCount;
		break;

	case CMD_CHGEFC:
		NEED (CMDPARA_CHGEFC);
		Key.a = ((PCMDPARA_CHGEFC)pData)->wData;
		break;

	case CMD_JOBINFO:
		NEED (CMDPARA_JOBINFO);
		Key.a = ((PCMDPARA_JOBINFO)pData)->byID;
		break;

	case CMD_MAGICINFO:
	case CMD_MAGICUSERINFO:
	case CMD_EQUUSERINFO:
		/* ID はどれも byCmd の次の 1 バイト */
		if (dwSize < 2) {
			return FALSE;
		}
		Key.a = pData[1];
		break;

	case CMD_NPCROUTE:
		{
			NEED (CMDPARA_NPCROUTE);
			PCMDPARA_NPCROUTE pPara = (PCMDPARA_NPCROUTE)pData;
			/* 保存（SET）だけ記録する。要求・通知は記録しない */
			if ((pPara->byCmdSub != NPCROUTE_SET) || (pPara->dwTblID >= (DWORD)g_aCharInfo.GetSize ())) {
				return FALSE;
			}
			Key.a = pPara->dwTblID;
		}
		break;

	default:
		return FALSE;
	}
#undef NEED
	return TRUE;
}

/* ========================================================================= */
/* 記録を消す																 */
/* ========================================================================= */

static size_t EntryBytes(const ADMINUNDO_ENTRY &Entry)
{
	return Entry.aBefore.size () + Entry.aAfter.size () + Entry.strIdent.size () + sizeof (ADMINUNDO_ENTRY);
}

static void EraseStep(size_t nIndex)
{
	s_nBytes -= s_aStep[nIndex].nBytes;
	s_aStep.erase (s_aStep.begin () + nIndex);
}

/* 決まりに合う変更を、全部の記録から消す（番号がずれて戻せなくなった時用） */
static void EraseEntries(BYTE byCmd, DWORD dwMapNo)
{
	for (size_t i = s_aStep.size (); i > 0; i --) {
		ADMINUNDO_STEP &Step = s_aStep[i - 1];
		for (size_t j = Step.aEntry.size (); j > 0; j --) {
			ADMINUNDO_ENTRY &Entry = Step.aEntry[j - 1];
			if ((Entry.Key.byCmd == byCmd) && (Entry.Key.a == dwMapNo)) {
				size_t nSize = EntryBytes (Entry);
				Step.nBytes	-= nSize;
				s_nBytes	-= nSize;
				Step.aEntry.erase (Step.aEntry.begin () + (j - 1));
			}
		}
		if (Step.aEntry.empty ()) {
			EraseStep (i - 1);
		}
	}
}

/* 覚えすぎた分を古い順に捨てる */
static void TrimSteps(BYTE byScreen)
{
	int nCount = 0;
	for (size_t i = s_aStep.size (); i > 0; i --) {
		if (s_aStep[i - 1].byScreen != byScreen) {
			continue;
		}
		nCount ++;
		if (nCount > ADMINUNDO_MAXSTEP) {
			EraseStep (i - 1);
		}
	}
	while ((s_nBytes > ADMINUNDO_MAXBYTES) && (s_aStep.size () > 1)) {
		EraseStep (0);
	}
}

/* ========================================================================= */
/* 記録する																	 */
/* ========================================================================= */

static BOOL BeginPending(BYTE byCmd, const ADMINUNDO_KEY &Key, DWORD dwMapNo, DWORD pID, LPCSTR pszSource, BYTE byScreen);

BOOL AdminUndoBefore(BYTE byCmd, const BYTE *pData, DWORD dwSize, DWORD pID, LPCSTR pszSource, BYTE byScreen)
{
	ADMINUNDO_KEY Key;
	DWORD dwMapNo;

	s_bPending = FALSE;
	if (s_bReplaying) {
		return FALSE;
	}
	if (!ParseKey (byCmd, pData, dwSize, Key, dwMapNo)) {
		return FALSE;
	}

	/* イベントの削除（座標 0,0 で詳細なし）は番号が詰まるので、そのマップのイベントの記録は使えなくなる */
	if (byCmd == CMD_CHGEVENT) {
		PCMDPARA_CHGEVENT pPara = (PCMDPARA_CHGEVENT)pData;
		if ((pPara->pData == NULL) && (pPara->dwX == 0) && (pPara->dwY == 0)) {
			EraseEntries (CMD_CHGEVENT, Key.a);
			return FALSE;
		}
	}

	return BeginPending (byCmd, Key, dwMapNo, pID, pszSource, byScreen);
}

/* API から、パケットを使わずに直接データを変える時に呼ぶ（対象は番号 1 つで決まるコマンドだけ） */
BOOL AdminUndoBeforeTarget(BYTE byCmd, DWORD dwID, LPCSTR pszSource, BYTE byScreen)
{
	ADMINUNDO_KEY Key;

	s_bPending = FALSE;
	if (s_bReplaying) {
		return FALSE;
	}
	switch (byCmd) {
	case CMD_STTMPLATE:
	case CMD_CHGITEM:
	case CMD_JOBINFO:
	case CMD_MAGICINFO:
	case CMD_MAGICUSERINFO:
	case CMD_EQUUSERINFO:
		break;
	default:
		return FALSE;
	}
	ZeroMemory (&Key, sizeof (Key));
	Key.byCmd	= byCmd;
	Key.a		= dwID;
	return BeginPending (byCmd, Key, ADMINUNDO_NOMAP, 0, pszSource, byScreen);
}

/* 対象が決まった後の記録の準備（AdminUndoBefore・AdminUndoBeforeTarget から） */
static BOOL BeginPending(BYTE byCmd, const ADMINUNDO_KEY &Key, DWORD dwMapNo, DWORD pID, LPCSTR pszSource, BYTE byScreen)
{
	s_Pending.Key		= Key;
	s_Pending.dwMapNo	= dwMapNo;
	s_Pending.aAfter.clear ();
	/* 対象がまだ無い（追加）なら記録しない */
	if (!BuildState (Key, s_Pending.aBefore, s_Pending.strIdent)) {
		return FALSE;
	}

	if ((byScreen == ADMINSCR_NONE) || (byScreen >= ADMINSCR_MAX)) {
		byScreen = AdminUndoDefaultScreen (byCmd);
	}
	s_byPendingScreen	= byScreen;
	s_pIDPending		= pID;
	s_strPendingSource	= pszSource ? pszSource : "";
	s_bPending			= TRUE;
	return TRUE;
}

void AdminUndoAfter(void)
{
	std::string strIdent;

	if (!s_bPending) {
		return;
	}
	s_bPending = FALSE;

	if (!BuildState (s_Pending.Key, s_Pending.aAfter, strIdent)) {
		return;
	}
	/* 変わっていなければ記録しない */
	if ((s_Pending.aAfter == s_Pending.aBefore) && (strIdent == s_Pending.strIdent)) {
		return;
	}
	s_Pending.strIdent = strIdent;

	DWORD dwTick = timeGetTime ();
	ADMINUNDO_STEP *pStep = NULL;
	/* 同じ人が同じ画面で続けて送った分はまとめる */
	if (!s_aStep.empty ()) {
		ADMINUNDO_STEP &Last = s_aStep.back ();
		if ((Last.byScreen == s_byPendingScreen) && (Last.pID == s_pIDPending) &&
				(Last.strSource == s_strPendingSource) &&
				(dwTick - Last.dwTickLast <= ADMINUNDO_MERGETIME)) {
			pStep = &Last;
		}
	}
	if (pStep == NULL) {
		ADMINUNDO_STEP Step;
		Step.dwID		= s_dwNextStepID ++;
		if (s_dwNextStepID == 0) {
			s_dwNextStepID = 1;
		}
		Step.byScreen	= s_byPendingScreen;
		Step.pID		= s_pIDPending;
		Step.strSource	= s_strPendingSource;
		Step.nBytes		= 0;
		s_aStep.push_back (Step);
		pStep = &s_aStep.back ();
	}
	pStep->dwTickLast	= dwTick;
	pStep->tLast		= time (NULL);
	pStep->aEntry.push_back (s_Pending);
	size_t nSize = EntryBytes (s_Pending);
	pStep->nBytes	+= nSize;
	s_nBytes		+= nSize;

	TrimSteps (s_byPendingScreen);
}

/* ========================================================================= */
/* 元に戻す																	 */
/* ========================================================================= */

/* その画面の最後の記録（無ければ -1） */
static int FindLastStep(BYTE byScreen, WORD *pwCount)
{
	int nRet = -1;
	WORD wCount = 0;
	for (size_t i = 0; i < s_aStep.size (); i ++) {
		if (s_aStep[i].byScreen == byScreen) {
			nRet = (int)i;
			wCount ++;
		}
	}
	if (pwCount) {
		*pwCount = wCount;
	}
	return nRet;
}

/* 変わった対象の数（同じ対象を何度も変えていても 1 つと数える） */
static int CountTargets(const ADMINUNDO_STEP &Step)
{
	std::set<ADMINUNDO_KEY> setKey;
	for (size_t i = 0; i < Step.aEntry.size (); i ++) {
		setKey.insert (Step.aEntry[i].Key);
	}
	return (int)setKey.size ();
}

/* 「3分前に」など（すぐ後に「〇〇さんが」が続く） */
static std::string AgoText(time_t t)
{
	char szTmp[64];
	long nSec = (long)(time (NULL) - t);
	if (nSec < 60) {
		return "ついさっき";
	} else if (nSec < 60 * 60) {
		snprintf (szTmp, sizeof (szTmp), "%ld分前に", nSec / 60);
	} else if (nSec < 24 * 60 * 60) {
		snprintf (szTmp, sizeof (szTmp), "%ld時間前に", nSec / (60 * 60));
	} else {
		snprintf (szTmp, sizeof (szTmp), "%ld日前に", nSec / (24 * 60 * 60));
	}
	return szTmp;
}

/* 「「アイテム一覧」で3分前に〇〇さんが変えた 2 件」 */
static std::string StepText(const ADMINUNDO_STEP &Step)
{
	char szTmp[64];
	std::string strRet = "「";
	strRet += AdminUndoScreenName (Step.byScreen);
	strRet += "」で";
	strRet += AgoText (Step.tLast);
	if (!Step.strSource.empty ()) {
		strRet += Step.strSource;
		strRet += "さんが";
	}
	snprintf (szTmp, sizeof (szTmp), "変えた %d 件", CountTargets (Step));
	strRet += szTmp;
	return strRet;
}

BOOL AdminUndoGetInfo(BYTE byScreen, DWORD &dwStepID, WORD &wCount, std::string &strMsg)
{
	int nIndex = FindLastStep (byScreen, &wCount);

	dwStepID = 0;
	if (nIndex < 0) {
		strMsg = "「";
		strMsg += AdminUndoScreenName (byScreen);
		strMsg += "」で元に戻せる変更はありません。\n（サーバーを起動してからの変更だけ戻せます）";
		return FALSE;
	}
	dwStepID	= s_aStep[nIndex].dwID;
	strMsg		= StepText (s_aStep[nIndex]);
	return TRUE;
}

BOOL AdminUndoExec(BYTE byScreen, DWORD dwStepID, DWORD pID, std::string &strMsg, WORD &wCount)
{
	char szTmp[128];
	int nIndex = FindLastStep (byScreen, &wCount);

	if (nIndex < 0) {
		AdminUndoGetInfo (byScreen, dwStepID, wCount, strMsg);
		return FALSE;
	}
	if ((dwStepID != 0) && (s_aStep[nIndex].dwID != dwStepID)) {
		strMsg = "確かめている間にこの画面の変更が増えたので、元に戻すのをやめました。\nもう一度「元に戻す」を選んでください。";
		return FALSE;
	}

	/* 記録からは先に外す（戻す処理は記録しない） */
	ADMINUNDO_STEP Step = s_aStep[nIndex];
	EraseStep ((size_t)nIndex);
	wCount --;

	/* 今も記録した「変更後」のままか確かめる（対象ごとに一番新しい変更で比べる） */
	std::set<ADMINUNDO_KEY> setChecked;
	std::vector<BYTE> aNow;
	std::string strIdent;
	int nConflict = 0;
	for (size_t i = Step.aEntry.size (); i > 0; i --) {
		const ADMINUNDO_ENTRY &Entry = Step.aEntry[i - 1];
		if (setChecked.count (Entry.Key)) {
			continue;
		}
		setChecked.insert (Entry.Key);
		if (!BuildState (Entry.Key, aNow, strIdent) ||
				(aNow != Entry.aAfter) || (strIdent != Entry.strIdent)) {
			nConflict ++;
		}
	}
	if (nConflict) {
		strMsg = StepText (Step);
		snprintf (szTmp, sizeof (szTmp), "変更のうち %d 件が、", nConflict);
		strMsg = strMsg + "\nを元に戻そうとしましたが、" + szTmp +
				"そのあとで変わっていた（ゲーム中の変化・ほかの管理者や API の変更など）ので、上書きしないようにやめました。\n"
				"この変更の記録は消しました。必要なら手で直してください。";
		return FALSE;
	}

	/* 新しい変更から順に、前の状態を処理し直す */
	s_bReplaying = TRUE;
	for (size_t i = Step.aEntry.size (); i > 0; i --) {
		const ADMINUNDO_ENTRY &Entry = Step.aEntry[i - 1];
		RecvCmdReplayAdmin (Entry.aBefore, pID, Entry.dwMapNo);
	}
	s_bReplaying = FALSE;

	strMsg = StepText (Step) + "を元に戻しました。";
	if (wCount) {
		snprintf (szTmp, sizeof (szTmp), "\n（この画面では、あと %d 回戻せます）", (int)wCount);
		strMsg += szTmp;
	}
	return TRUE;
}

/* ========================================================================= */
/* ブラウザ版サーバー（Node.js）から呼ぶ入口（API・MCP 用）					 */
/* ========================================================================= */
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* 管理者用コマンドを API から処理する（SboSvrRecvCmd.cpp） */
BOOL RecvCmdApplyAdminPacket(const BYTE *pData, DWORD dwSize, LPCSTR pszSource, BYTE byScreen);

static std::string s_strUndoJson;

static void JsonStr(std::string &strDst, const std::string &strSrc)
{
	strDst += '"';
	for (size_t i = 0; i < strSrc.size (); i ++) {
		unsigned char c = (unsigned char)strSrc[i];
		switch (c) {
		case '"':	strDst += "\\\"";	break;
		case '\\':	strDst += "\\\\";	break;
		case '\n':	strDst += "\\n";	break;
		case '\r':	strDst += "\\r";	break;
		case '\t':	strDst += "\\t";	break;
		default:
			if (c < 0x20) {
				char szTmp[8];
				snprintf (szTmp, sizeof (szTmp), "\\u%04x", c);
				strDst += szTmp;
			} else {
				strDst += (char)c;
			}
			break;
		}
	}
	strDst += '"';
}

extern "C" {

/*
 * 管理者用コマンドのパケット（クライアントが送るのと同じ形）を処理し、「元に戻す」に記録する。
 *   pszSource  変えた人として案内に出す名前（例 "API"）
 *   nScreen    記録する画面（ADMINSCR_*）。0 ならコマンドからいつもの画面を選ぶ
 * 処理したら 1
 */
EMSCRIPTEN_KEEPALIVE int SboAdminApplyPacket(const BYTE *pData, int nSize, const char *pszSource, int nScreen)
{
	if ((pData == NULL) || (nSize <= 0)) {
		return 0;
	}
	return RecvCmdApplyAdminPacket (pData, (DWORD)nSize, pszSource, (BYTE)nScreen) ? 1 : 0;
}

/*
 * 画面ごとの戻せる変更の一覧
 *   [{"screen":10,"name":"アイテム一覧","count":2,"stepId":15,"text":"「アイテム一覧」で…"}, ...]
 *   （戻せる変更がある画面だけ）
 */
EMSCRIPTEN_KEEPALIVE const char *SboAdminUndoListJson(void)
{
	char szTmp[128];
	s_strUndoJson = "[";
	BOOL bFirst = TRUE;
	for (int i = ADMINSCR_NONE + 1; i < ADMINSCR_MAX; i ++) {
		DWORD dwStepID;
		WORD wCount;
		std::string strMsg;
		if (!AdminUndoGetInfo ((BYTE)i, dwStepID, wCount, strMsg)) {
			continue;
		}
		if (!bFirst) {
			s_strUndoJson += ",";
		}
		bFirst = FALSE;
		snprintf (szTmp, sizeof (szTmp), "{\"screen\":%d,\"name\":", i);
		s_strUndoJson += szTmp;
		JsonStr (s_strUndoJson, AdminUndoScreenName ((BYTE)i));
		snprintf (szTmp, sizeof (szTmp), ",\"count\":%u,\"stepId\":%u,\"text\":", (unsigned)wCount, (unsigned)dwStepID);
		s_strUndoJson += szTmp;
		JsonStr (s_strUndoJson, strMsg);
		s_strUndoJson += "}";
	}
	s_strUndoJson += "]";
	return s_strUndoJson.c_str ();
}

/*
 * その画面の最後の変更を元に戻す（nStepId が 0 以外なら、その変更の時だけ）
 *   {"ok":true,"count":1,"text":"…を元に戻しました。"}
 */
EMSCRIPTEN_KEEPALIVE const char *SboAdminUndoExecJson(int nScreen, unsigned int nStepId)
{
	char szTmp[64];
	std::string strMsg;
	WORD wCount = 0;
	BOOL bRet = FALSE;

	if ((nScreen > ADMINSCR_NONE) && (nScreen < ADMINSCR_MAX)) {
		bRet = AdminUndoExec ((BYTE)nScreen, (DWORD)nStepId, 0, strMsg, wCount);
	} else {
		strMsg = "画面の番号が正しくありません。";
	}
	snprintf (szTmp, sizeof (szTmp), "{\"ok\":%s,\"count\":%u,\"text\":", bRet ? "true" : "false", (unsigned)wCount);
	s_strUndoJson = szTmp;
	JsonStr (s_strUndoJson, strMsg);
	s_strUndoJson += "}";
	return s_strUndoJson.c_str ();
}

}
#endif
