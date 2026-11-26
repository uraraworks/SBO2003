/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrTrialRoom.cpp											 */
/* 概要：		お試し個室(実装ファイル)									 */
/* ========================================================================= */
/*
 * NPC の台詞を "@xNN"（NN=お試し個室の番号。TRIALROOMINFO の並びの番号）にすると入口になる。
 * 話しかけると説明（szDesc）を会話窓に出して「入りますか？」（はい・いいえ）を聞く。
 * はいなら、その部屋のステータステンプレート（wLoanStTmp）の姿を借りて（LoanBegin）、
 * 部屋の入る座標へ移る。借りている間は討伐チャレンジ・番外編と同じ制限（店・預かり所・取り引き・町転送など）。
 * 経験値・お金・手に入れた物は借りた姿の中だけで、返す時（LoanEnd）に本物へ戻る。
 *
 * 出る条件（周期処理で見張る）：部屋の範囲の四角（同じマップ内）の外に出た。出口はマップのイベントで作る。
 * 倒れた時は「復活」を選んだ時にすぐ戻す（お金は減らさない）。切断・サーバー保存は SboSvrLoan に任せる。
 * 第 1 版は 1 人用（空間は分けず、ふつうの世界の部屋。パーティ・傭兵と一緒には入れない）。
 */
#include "include.h"
#include "Glbldata.h"
#include "CharProc/IncludeCharProc.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrQuest.h"
#include "SboSvrLoan.h"
#include "SboSvrTrialRoom.h"
#include <stdio.h>
#include <string.h>
#include <map>
#include <set>
#include <string>
#include <vector>

/* 「入りますか？」の問い（キャラごとに 1 つ。保存しない） */
typedef struct _TRIALASK {
	DWORD		dwNpcTblID;											/* 話しかけた入口のテーブルID */
	int			nRoom;												/* 部屋の番号 */
	DWORD		dwTime;												/* 問いかけた時刻 */
	DWORD		dwMapID;											/* 問いかけた時にいたマップ */
} TRIALASK;

enum {
	TRIALLEAVE_OUT = 0,												/* 範囲の外へ出た（出口） */
	TRIALLEAVE_DEAD,												/* 倒れて復活を選んだ */
	TRIALLEAVE_LOGOUT,												/* 切断 */
};

static std::map<DWORD, TRIALASK> s_mapAsk;						/* キャラのテーブルID → 問い */
static std::set<DWORD> s_setIn;									/* 個室の中にいる人のテーブルID（周期処理で見張る） */

/* 入口の台詞 "@xNN" の番号（入口でなければ -1） */
static int TrialRoomNoOf(PCHARINFO pNpc)
{
	const char *p;
	int n = 0;

	if (!TrialRoomIsNpc (pNpc)) {
		return -1;
	}
	p = pNpc->pszNpcSpeech + 2;
	while ((*p >= '0') && (*p <= '9') && (n < 10000)) {
		n = n * 10 + (*p - '0');
		p ++;
	}
	return n;
}

/* 部屋の情報（番号が無い・設定がそろっていなければ NULL。閉じている部屋は byState で別に見る） */
static PTRIALROOMINFO TrialRoomGet(int nRoom)
{
	PTRIALROOMINFO r;

	if ((nRoom < 0) || (nRoom >= g_aTrialRoomInfo.GetSize ())) {
		return NULL;
	}
	r = g_aTrialRoomInfo[nRoom];
	if ((r == NULL) || (r->wLoanStTmp >= (WORD)g_aStTmpInfo.GetSize ()) || (r->dwMapID >= (DWORD)g_aMapInfo.GetSize ()) ||
		(r->dwX1 > r->dwX2) || (r->dwY1 > r->dwY2) || (r->dwX < r->dwX1) || (r->dwX > r->dwX2) || (r->dwY < r->dwY1) || (r->dwY > r->dwY2)) {
		return NULL;
	}
	return r;
}

/* 部屋の範囲の中か（マップも合うこと） */
static BOOL TrialRoomInRect(PTRIALROOMINFO r, PCHARINFO p)
{
	return (p->dwMapID == r->dwMapID) && (p->dwX >= r->dwX1) && (p->dwX <= r->dwX2) && (p->dwY >= r->dwY1) && (p->dwY <= r->dwY2);
}

/* 入口のNPCか */
BOOL TrialRoomIsNpc(PCHARINFO pNpc)
{
	return pNpc && (pNpc->byNpcType != NPCTYPE_INIT) && pNpc->pszNpcSpeech &&
		(pNpc->pszNpcSpeech[0] == '@') && (pNpc->pszNpcSpeech[1] == 'x') &&
		(pNpc->pszNpcSpeech[2] >= '0') && (pNpc->pszNpcSpeech[2] <= '9');
}

/* お試し個室の中か */
BOOL TrialRoomIs(PCHARINFO pChar)
{
	LOANBAK *pL = (pChar != NULL) ? LoanFind (pChar->dwTblID) : NULL;

	return (pL != NULL) && pL->bTrial;
}

/* 貸し出し中の制限で出す文 */
const char *TrialRoomBlockedText(PCHARINFO pChar)
{
	return TrialRoomIs (pChar) ? "お試し個室の中ではできません。" : "討伐チャレンジ中はできません。";
}

/* 会話窓で断る（最後に入力待ちを付ける） */
static void TrialRoomRefuse(PCHARINFO pChar, PCHARINFO pNpc, const char *pszText)
{
	char szTmp[256];

	snprintf (szTmp, sizeof (szTmp), "%s@2", pszText);
	SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szTmp);
}

/* 雇っている傭兵がいるか */
static BOOL TrialRoomHasMercenary(PCHARINFO pChar)
{
	int i, nCount = g_aCharInfo.GetSize ();

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pTmp = g_aCharInfo[i];
		if (IsMercenary (pTmp) && (GetMercenaryOwner (pTmp) == pChar)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 自分のほかに、パーティの仲間が（遊んでいて）いるか */
static BOOL TrialRoomHasPartner(PCHARINFO pChar)
{
	PPARTYINFO pParty;
	int i;

	if ((pChar->dwPartyID == 0) || (pChar->dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return FALSE;
	}
	pParty = g_aPartyInfo[pChar->dwPartyID];
	for (i = 0; i < MAX_PARTYMEMBER; i ++) {
		DWORD dwTbl = pParty->adwTblID[i];
		if (dwTbl == 0) {
			break;
		}
		if ((dwTbl != pChar->dwTblID) && (dwTbl < (DWORD)g_aCharInfo.GetSize ()) && (g_aCharInfo[dwTbl]->pID != 0) &&
			(g_aCharInfo[dwTbl]->dwPartyID == pChar->dwPartyID)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* 入れるか確かめる。入れなければ断る文を返す（入れる時は NULL） */
static const char *TrialRoomCanEnter(PCHARINFO pChar, int nRoom)
{
	PTRIALROOMINFO r = TrialRoomGet (nRoom);

	if ((r == NULL) || (r->byState != TRIALROOMSTATE_OPEN)) {
		return "この個室は、いまは閉じています。";
	}
	if (pChar->byNpcType != NPCTYPE_INIT) {
		return "ここからは入れません。";
	}
	if (LoanIs (pChar)) {
		return "すでに別の姿を借りています。元に戻ってから来てください。";
	}
	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return "倒れている間は入れません。";
	}
	if (TrialRoomInRect (r, pChar)) {
		return "もう個室の中にいます。";
	}
	if (pChar->dwSpaceID != SPACE_NORMAL) {
		return "ここからは入れません。";
	}
	if (IsCharBattleMode (pChar)) {
		return "戦闘モードをやめてから来てください。";
	}
	if (pChar->abySubStatus[0] != 0) {
		return "ミニゲームの途中では入れません。";
	}
	if (TrialRoomHasPartner (pChar)) {
		return "パーティを組んでいる間は入れません（1 人用です）。パーティを抜けてから来てください。";
	}
	if (TrialRoomHasMercenary (pChar)) {
		return "傭兵と一緒には入れません。別れてから来てください。";
	}
	return NULL;
}

/* 部屋に入る（条件は確かめ済み） */
static void TrialRoomEnter(PCHARINFO pChar, int nRoom)
{
	PTRIALROOMINFO r = TrialRoomGet (nRoom);
	PSTATUSTEMPLATEINFO t;
	LOANBAK *pBak;
	char szTmp[256];

	if (r == NULL) {
		return;
	}
	t = g_aStTmpInfo[r->wLoanStTmp];
	CharTargetCancelPC (pChar);
	/* 戻る場所は、今いる場所と向き */
	pBak = LoanBegin (pChar, t, pChar->dwMapID, pChar->dwX, pChar->dwY, pChar->byDirection);
	pBak->bTrial = TRUE;
	pBak->wTrialRoom = (WORD)nRoom;
	s_setIn.insert (pChar->dwTblID);

	snprintf (szTmp, sizeof (szTmp), "%sに入りました。ここでは Lv%d 力%d 素早さ%d 体力%d 賢さ%d 運%d HP%d Mana%d の姿です。",
		r->szName, max ((int)t->byLevel, 1), t->byPW, t->byDEX, t->byST, t->byIT, t->byLK, max ((int)t->wMaxHP, 1), t->wMaxMana);
	SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
	SendCmdSYSMSG (pChar->pID, "出口から出ると、倒れて復活を選ぶと、切断すると、元の姿・元の場所に戻ります。経験値・お金・手に入れた物は中だけです。", SPEECHCMD_BATTLE1);

	/* 同じマップの中のワープ（座標変更）ではマップ情報が届かないので、マップを移る時と同じ流れで見え方を作り直す */
	SendCmdPLAYSOUND (pChar->pID, SOUND_MOVE);
	SendCmdFADEINOUT (pChar->pID, 0);
	SubProcMapMove (pChar, r->dwMapID, r->dwX, r->dwY);
}

/* 返す。出た後は元の姿・元の場所（切断では戻り位置を登録するだけ） */
static void TrialRoomLeave(PCHARINFO p, int nReason)
{
	LOANBAK Bak;

	if (!TrialRoomIs (p)) {
		s_setIn.erase (p->dwTblID);
		return;
	}
	s_setIn.erase (p->dwTblID);
	Bak = LoanEnd (p);

	if (p->byCharState == CHARSTATE_DEAD) {
		p->byCharState = CHARSTATE_MOVE;
	}
	/* 入る前の HP が 0 のことはないはずだが、念のため */
	if (p->wHP == 0) {
		p->wHP = max (p->wMaxHP, (WORD)1);
	}

	if (nReason == TRIALLEAVE_LOGOUT) {
		/* 切断：マップの表から外れた後に、入る前の場所へ（ログインか保存の時に動かす） */
		LoanPendingSet (p->dwTblID, Bak.dwMapID, Bak.dwX, Bak.dwY, Bak.byDirection);
		LoanSendAll (p);
		return;
	}

	LoanSendAll (p);
	if (nReason == TRIALLEAVE_DEAD) {
		SendCmdSYSMSG (p->pID, "力尽きてしまった…。個室を出て、元の姿に戻りました。", SPEECHCMD_BATTLE1);
	} else {
		SendCmdSYSMSG (p->pID, "個室を出たので、元の姿に戻りました。", SPEECHCMD_BATTLE1);
	}
	if (Bak.dwMapID < (DWORD)g_aMapInfo.GetSize ()) {
		SendCmdFADEINOUT (p->pID, 0);
		SendCmdCHGBGM (p->pID, p->dwMapID, 0);
		SubProcMapMove (p, Bak.dwMapID, Bak.dwX, Bak.dwY);
	}
}

/* 入口に話しかけた */
void TrialRoomTalk(PCHARINFO pChar, PCHARINFO pNpc)
{
	PTRIALROOMINFO r;
	const char *pszWhy;
	int nRoom = TrialRoomNoOf (pNpc);
	std::string strDesc, strMsg;
	char szDesc[MAX_TRIALROOMDESC * 2];
	char szMsg[MAX_TALKMSG];
	const char *pszSuffix = "@1@1入りますか？@y";
	TRIALASK Ask;
	int i;

	if ((pChar == NULL) || (pNpc == NULL) || (pChar->pID == 0) || (nRoom < 0)) {
		return;
	}
	s_mapAsk.erase (pChar->dwTblID);
	pszWhy = TrialRoomCanEnter (pChar, nRoom);
	if (pszWhy) {
		TrialRoomRefuse (pChar, pNpc, pszWhy);
		return;
	}
	r = TrialRoomGet (nRoom);
	/* 説明（改行は台詞の制御コード @1 にする。説明が無ければ名前） */
	strDesc = r->szDesc[0] ? r->szDesc : r->szName;
	for (i = 0; i < (int)strDesc.size (); i ++) {
		if (strDesc[i] == '\r') {
			continue;
		}
		if (strDesc[i] == '\n') {
			strMsg += "@1";
		} else {
			strMsg += strDesc[i];
		}
	}
	/* 最後の「入りますか？@y」が切れないよう、説明のほうを短く収める */
	SboTextCopy (szDesc, (int)(sizeof (szMsg) - strlen (pszSuffix)), strMsg.c_str ());
	snprintf (szMsg, sizeof (szMsg), "%s%s", szDesc, pszSuffix);

	Ask.dwNpcTblID = pNpc->dwTblID;
	Ask.nRoom = nRoom;
	Ask.dwTime = timeGetTime ();
	Ask.dwMapID = pChar->dwMapID;
	s_mapAsk[pChar->dwTblID] = Ask;
	SendCmdTALKMSG (pChar->pID, pNpc->dwTblID, szMsg);
}

/* 「入りますか？」への答え */
BOOL TrialRoomAnswer(PCHARINFO pChar, DWORD dwNpcTblID, BOOL bYes)
{
	std::map<DWORD, TRIALASK>::iterator it;
	TRIALASK Ask;
	PCHARINFO pNpc;
	const char *pszWhy;

	if ((pChar == NULL) || (pChar->pID == 0)) {
		return FALSE;
	}
	it = s_mapAsk.find (pChar->dwTblID);
	if ((it == s_mapAsk.end ()) || (it->second.dwNpcTblID != dwNpcTblID)) {
		return FALSE;
	}
	Ask = it->second;
	s_mapAsk.erase (it);
	/* 時間切れ・マップが変わった・入口から離れた答えは、黙って捨てる */
	if (((DWORD)(timeGetTime () - Ask.dwTime) > (DWORD)TALK_ASK_TIMEOUT) || (pChar->dwMapID != Ask.dwMapID) ||
		(dwNpcTblID >= (DWORD)g_aCharInfo.GetSize ())) {
		return TRUE;
	}
	pNpc = g_aCharInfo[dwNpcTblID];
	if (!TrialRoomIsNpc (pNpc) || (TrialRoomNoOf (pNpc) != Ask.nRoom) || (pNpc->dwMapID != pChar->dwMapID) ||
		!IsSameSpace (pChar, pNpc) || !IsCharSeenBy (pNpc, pChar) ||
		(abs ((int)pChar->dwX - (int)pNpc->dwX) > TALK_ASK_RANGE) || (abs ((int)pChar->dwY - (int)pNpc->dwY) > TALK_ASK_RANGE)) {
		return TRUE;
	}
	if (!bYes) {
		TrialRoomRefuse (pChar, pNpc, "またいつでもどうぞ。");
		return TRUE;
	}
	/* 問いのあとで状況が変わっていないか、もう一度確かめる */
	pszWhy = TrialRoomCanEnter (pChar, Ask.nRoom);
	if (pszWhy) {
		TrialRoomRefuse (pChar, pNpc, pszWhy);
		return TRUE;
	}
	TrialRoomEnter (pChar, Ask.nRoom);
	return TRUE;
}

/* 周期処理：範囲の外に出た人を元の姿・元の場所へ */
void TrialRoomProc(void)
{
	std::vector<DWORD> aIn (s_setIn.begin (), s_setIn.end ());
	size_t i;

	for (i = 0; i < aIn.size (); i ++) {
		PCHARINFO p = (aIn[i] < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[aIn[i]] : NULL;
		LOANBAK *pL = (p != NULL) ? LoanFind (aIn[i]) : NULL;
		PTRIALROOMINFO r;

		if ((p == NULL) || (pL == NULL) || !pL->bTrial) {
			s_setIn.erase (aIn[i]);
			continue;
		}
		r = TrialRoomGet (pL->wTrialRoom);
		if ((r == NULL) || !TrialRoomInRect (r, p)) {
			TrialRoomLeave (p, TRIALLEAVE_OUT);
		}
	}
}

/* 倒れて「復活」を選んだ */
BOOL TrialRoomResurrection(PCHARINFO pChar)
{
	if (!TrialRoomIs (pChar)) {
		return FALSE;
	}
	TrialRoomLeave (pChar, TRIALLEAVE_DEAD);
	return TRUE;
}

/* キャラが消える・切断した */
BOOL TrialRoomOnDecPlayer(PCHARINFO pChar)
{
	s_mapAsk.erase (pChar->dwTblID);
	if (!TrialRoomIs (pChar)) {
		return FALSE;
	}
	TrialRoomLeave (pChar, TRIALLEAVE_LOGOUT);
	return TRUE;
}
