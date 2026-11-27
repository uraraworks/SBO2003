/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* ファイル名：	SboSvrLoan.cpp												 */
/* 概要：		キャラ貸し出しの部品											 */
/*				（SboSvrQuest.cpp から切り出し。依頼・ダンジョンに依存しない）	 */
/* ========================================================================= */

#include "include.h"
#include "Glbldata.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrItemProc.h"
#include "SboSvrEventProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrLoan.h"
#include <string.h>
#include <map>
#include <algorithm>

/* 戻す場所（切断した人は、マップの表から外れてから動かす） */
typedef struct _LOANPOS {
	DWORD		dwMapID, dwX, dwY;
	BYTE		byDirection;
} LOANPOS;

static std::map<DWORD, LOANBAK> s_mapLoan;						/* 貸し出し中の人のテーブルID → 本物の控え */
static std::map<DWORD, LOANPOS> s_mapPendingPos;				/* 切断した人のテーブルID → 戻す場所 */

/* 控えとキャラの間で状態を写す（bToChar=TRUE で控え→キャラ）。位置は写さない */
static void LoanXfer(PCHARINFO p, LOANBAK *b, BOOL bToChar)
{
#define LOANX(f)	if (bToChar) { p->f = b->f; } else { b->f = p->f; }
#define LOANA(f)	if (bToChar) { memcpy (p->f, b->f, sizeof (p->f)); } else { memcpy (b->f, p->f, sizeof (p->f)); }
	LOANX (byLevel);	LOANX (byPW);	LOANX (byDEX);	LOANX (byST);	LOANX (byIT);	LOANX (byLK);
	LOANA (abyMagic);	LOANA (waItem);	LOANA (waEqu);
	LOANA (abyMagicKey);	LOANA (awKeyItem);
	LOANX (wMaxHP);	LOANX (wHP);	LOANX (wMaxMana);	LOANX (wMana);
	LOANX (dwGold);	LOANX (dwExp);
	LOANX (chAttrFire);	LOANX (chAttrWater);	LOANX (chAttrWind);
	LOANX (chAttrEarth);	LOANX (chAttrLight);	LOANX (chAttrDark);
#undef LOANX
#undef LOANA
}

/* 控えとキャラの状態・位置を入れ替える（保存の前後で使う） */
static void LoanSwap(PCHARINFO p, LOANBAK *b)
{
	LOANBAK Tmp = *b;
	DWORD dwMapID = p->dwMapID, dwX = p->dwX, dwY = p->dwY;
	BYTE byDirection = p->byDirection;

	LoanXfer (p, b, FALSE);			/* キャラ → 控え */
	LoanXfer (p, &Tmp, TRUE);		/* 元の控え → キャラ */
	p->dwMapID = Tmp.dwMapID;	p->dwX = Tmp.dwX;	p->dwY = Tmp.dwY;	p->byDirection = Tmp.byDirection;
	b->dwMapID = dwMapID;	b->dwX = dwX;	b->dwY = dwY;	b->byDirection = byDirection;
}

/* 貸し出しセット（ステータステンプレート）にする。持ち物の装備品は種類ごとに最初の 1 つを装備 */
static void LoanApply(PCHARINFO p, PSTATUSTEMPLATEINFO t)
{
	static const BYTE abyEquTypeTbl[MAX_EQUTYPE] = {
		ITEMTYPE_WEAPON, ITEMTYPE_ARMOR, ITEMTYPE_SHILD, ITEMTYPE_HELMET, ITEMTYPE_EQU_HAND,
		ITEMTYPE_EQU_LEG, ITEMTYPE_EQU_ACC1, ITEMTYPE_EQU_ACC2, ITEMTYPE_EQU_ACC3, 0
	};
	int i, j;

	p->byLevel	= max (t->byLevel, (BYTE)1);
	p->byPW		= t->byPW;
	p->byDEX	= t->byDEX;
	p->byST		= t->byST;
	p->byIT		= t->byIT;
	p->byLK		= t->byLK;
	p->wMaxHP	= max (t->wMaxHP, (WORD)1);
	p->wHP		= p->wMaxHP;
	p->wMaxMana	= t->wMaxMana;
	p->wMana	= t->wMaxMana;
	p->dwGold	= t->dwGold;
	p->dwExp	= 0;
	p->chAttrFire	= t->chAttrFire;
	p->chAttrWater	= t->chAttrWater;
	p->chAttrWind	= t->chAttrWind;
	p->chAttrEarth	= t->chAttrEarth;
	p->chAttrLight	= t->chAttrLight;
	p->chAttrDark	= t->chAttrDark;
	memcpy (p->abyMagic, t->abyMagic, sizeof (p->abyMagic));
	/* ショートカットは空から（元の登録は控えにあり、出る時に戻す） */
	ZeroMemory (p->abyMagicKey, sizeof (p->abyMagicKey));
	ZeroMemory (p->awKeyItem, sizeof (p->awKeyItem));
	ZeroMemory (p->waItem, sizeof (p->waItem));
	for (i = 0, j = 0; i < MAX_MYITEM; i ++) {
		/* 無いアイテムの番号は持たせない */
		if ((t->waItem[i] == 0) || (t->waItem[i] >= (WORD)g_aItemInfo.GetSize ())) {
			continue;
		}
		p->waItem[j ++] = t->waItem[i];
	}
	for (i = 0; i < MAX_EQUTYPE; i ++) {
		p->waEqu[i] = 0xFFFF;
		if (abyEquTypeTbl[i] == 0) {
			continue;
		}
		for (j = 0; j < MAX_MYITEM; j ++) {
			if (p->waItem[j] && (g_aItemInfo[p->waItem[j]]->byType == abyEquTypeTbl[i])) {
				p->waEqu[i] = (WORD)j;
				break;
			}
		}
	}
}

/* 状態を変えた後：タイマーの効果（加速・毒など）を消し、攻撃力・守備力を計算し直して知らせる */
void LoanSendAll(PCHARINFO p)
{
	/* 入る前（出る前）に出した、ダメージを出し続ける魔法などのキャラを消す（行き先まで持ち込まない）。
	   DeleteChildChar はプレイヤーの子を探す表が違うので、ここで全部のキャラから探す */
	{
		int i, nCount = g_aCharInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PCHARINFO pTmp = g_aCharInfo[i];
			if ((pTmp->dwParentTblID != p->dwTblID) || (pTmp == p)) {
				continue;
			}
			if ((pTmp->byNpcType == NPCTYPE_USEMAGIC) || (pTmp->byNpcType == NPCTYPE_ATACK) ||
				(pTmp->byNpcType == NPCTYPE_USEMAGICPARENT)) {
				DeleteChar (pTmp->dwTblID);
			}
		}
	}
	DeleteAllTimerEvent (p);
	p->chTimerPW = 0;
	p->chTimerST = 0;
	p->chTimerDEX = 0;
	p->dwLastBattle = 0;
	p->dwTargetTblID = -1;
	RenewATDF (p->dwTblID);
	if (p->pID != 0) {
		SendCmdCHARITEM (p->pID, p->dwTblID);
		SendCmdCHARSTATUS (p->pID, p->dwTblID);
		SendCmdCHARSTATUSHP (URARASOCK_SENDALL, p->dwTblID);
		SendMagicKeySetting (p);		/* 入れ替えたショートカットを知らせる */
	}
	/* 攻撃アップ・守りアップの表示も消す（入る前にかけた効果を持ち込まない・持ち出さない） */
	BuffSendState (p);
}

/* 貸し出し中か */
BOOL LoanIs(PCHARINFO p)
{
	return (p != NULL) && (s_mapLoan.find (p->dwTblID) != s_mapLoan.end ());
}

/* 控えを探す */
LOANBAK *LoanFind(DWORD dwTblID)
{
	std::map<DWORD, LOANBAK>::iterator it = s_mapLoan.find (dwTblID);

	return (it == s_mapLoan.end ()) ? NULL : &it->second;
}

/* 借りる：本物を控えに写し、テンプレートの姿にして知らせる（戻り先は控えに持つ） */
LOANBAK *LoanBegin(PCHARINFO p, PSTATUSTEMPLATEINFO t, DWORD dwMapID, DWORD dwX, DWORD dwY, BYTE byDirection)
{
	LOANBAK Bak;

	ZeroMemory (&Bak, sizeof (Bak));
	LoanXfer (p, &Bak, FALSE);
	Bak.dwMapID = dwMapID;
	Bak.dwX = dwX;
	Bak.dwY = dwY;
	Bak.byDirection = byDirection;
	s_mapLoan[p->dwTblID] = Bak;

	LoanApply (p, t);
	LoanSendAll (p);
	return &s_mapLoan[p->dwTblID];
}

/* 返す：控えから状態を戻して控えを消す（控えのコピーを返す） */
LOANBAK LoanEnd(PCHARINFO p)
{
	std::map<DWORD, LOANBAK>::iterator it = s_mapLoan.find (p->dwTblID);
	LOANBAK Bak;

	ZeroMemory (&Bak, sizeof (Bak));
	if (it == s_mapLoan.end ()) {
		return Bak;
	}
	Bak = it->second;
	s_mapLoan.erase (it);
	LoanXfer (p, &Bak, TRUE);
	return Bak;
}

/* 保存の前後で呼ぶ：貸し出し中の人は本物の状態と入る前の場所を保存させる（前後で同じ入れ替えをする）。
   切断した人の戻す場所もここで書き込む */
void LoanSaveSwap(BOOL bBegin)
{
	std::map<DWORD, LOANBAK>::iterator it;

	for (it = s_mapLoan.begin (); it != s_mapLoan.end (); it ++) {
		if (it->first < (DWORD)g_aCharInfo.GetSize ()) {
			LoanSwap (g_aCharInfo[it->first], &it->second);
		}
	}
	if (bBegin) {
		std::map<DWORD, LOANPOS>::iterator itPos = s_mapPendingPos.begin ();
		while (itPos != s_mapPendingPos.end ()) {
			PCHARINFO p = (itPos->first < (DWORD)g_aCharInfo.GetSize ()) ? g_aCharInfo[itPos->first] : NULL;
			/* もうログインし直している人は、ログインの時に動かしている */
			if ((p != NULL) && (p->pID == 0)) {
				p->dwMapID = itPos->second.dwMapID;
				p->dwX = p->dwXBack = itPos->second.dwX;
				p->dwY = p->dwYBack = itPos->second.dwY;
				p->byDirection = itPos->second.byDirection;
			}
			s_mapPendingPos.erase (itPos ++);
		}
	}
}

/* 切断した人の戻り位置を登録する（マップの表から外れた後に、ログインか保存の時に動かす） */
void LoanPendingSet(DWORD dwTblID, DWORD dwMapID, DWORD dwX, DWORD dwY, BYTE byDirection)
{
	LOANPOS Pos;

	Pos.dwMapID = dwMapID;
	Pos.dwX = dwX;
	Pos.dwY = dwY;
	Pos.byDirection = byDirection;
	s_mapPendingPos[dwTblID] = Pos;
}

/* ログイン時：切断した時の戻り位置があれば直接書き換えて消す（まだマップの表に入る前なので直接書き換える） */
void LoanPendingTake(PCHARINFO pChar)
{
	std::map<DWORD, LOANPOS>::iterator itPos = s_mapPendingPos.find (pChar->dwTblID);

	if (itPos != s_mapPendingPos.end ()) {
		pChar->dwMapID = itPos->second.dwMapID;
		pChar->dwX = pChar->dwXBack = itPos->second.dwX;
		pChar->dwY = pChar->dwYBack = itPos->second.dwY;
		pChar->byDirection = itPos->second.byDirection;
		s_mapPendingPos.erase (itPos);
	}
}
