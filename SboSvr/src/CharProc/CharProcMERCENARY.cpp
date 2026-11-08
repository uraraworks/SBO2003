/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	CharProcMERCENARY.cpp										 */
/* 内容：		キャラ処理(傭兵)クラス 実装ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2026/11/08													 */
/* ========================================================================= */
/*
	傭兵（お試し版）
	・台詞が「@m」で始まるNPCに話しかけると、そのNPCと同じ見た目の傭兵が雇い主のとなりに出てくる。
	  もう一度話しかけるとお別れ。雇えるのは1人につき1人まで。
	  「@m05」のように数字を付けると、その番号のステータステンプレートの強さになる。
	  数字が無ければ、雇い主と同じくらいの強さ（レベル・能力値・HP・攻撃力・守備力を写す）。
	・ふだんは「ついていく」と同じように、雇い主が立っていたマスへ1歩ずつついていく。
	  離れすぎたり、しばらく近づけなかったら雇い主のとなりへ移る。
	・戦闘できるマップで、雇い主か傭兵を狙っている敵・雇い主のすぐ近くの敵がいれば、
	  列を離れて自分で近づいて殴る。敵がいなくなったら、また雇い主についていく。
	  攻撃の間隔は素早さで決まる（1〜3秒）。
	・敵からも狙われる。倒れたら10秒そのままで、その間に蘇生の道具を使えば起き上がる。
	・雇い主が別のマップへ行った・ログアウトした・倒れた時は、その場でいなくなる。
	  保存はしない（サーバーを再起動しても残らない）。
	・倒した敵の経験値・お金・道具は雇い主がもらう（傭兵の取り分は無し）。
	・雇い主（と、雇い主のパーティの仲間）の画面右下のHP・Mana一覧に並ぶ。
	  パーティの情報そのものには入れず、3秒ごとと、HP・Manaが変わった時に知らせ直す。
	・サーバー負荷：ついていく時は雇い主が動いた時だけ1歩、敵を探すのは0.5秒に1回
	  マップのNPC一覧を1回なめるだけなので、敵1匹ぶんとほぼ同じ。
*/

#include "stdafx.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSendCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "CharProcMERCENARY.h"
#include <map>

/* 戦闘中に1歩動いてから次に動けるまでの時間(ms)。敵と同じ */
#define MERC_STEPTIME			(320)
/* 雇い主へ近づく時の1歩の間隔(ms)。少し急ぐ */
#define MERC_FOLLOWSTEPTIME		(220)
/* 敵を探す間隔(ms) */
#define MERC_SEARCHTIME			(500)
/* 雇い主からこれより離れた敵は相手にしない(マス) */
#define MERC_LEASH				(6)
/* 雇い主からこれより離れたら戦うのをやめて戻る(マス) */
#define MERC_LEASHSELF			(8)
/* 雇い主からこれより離れたら、となりへ移る(マス) */
#define MERC_WARPDIST			(10)
/* 雇い主に近づけないまま、この時間たったら、となりへ移る(ms) */
#define MERC_STUCKTIME			(4000)
/* 倒れてから帰るまでの時間(ms) */
#define MERC_DEADTIME			(10000)
/* 仲間の一覧を知らせ直す間隔(ms) */
#define MERC_SYNCTIME			(3000)

/* 傭兵の tblID → 雇い主 */
typedef struct _MERCINFO {
	DWORD	dwOwnerTblID;								/* 雇い主のテーブルID */
	DWORD	dwOwnerPID;									/* 雇い主のプレイヤーID(ログインし直した別の人と間違えないため) */
} MERCINFO;
static std::map<DWORD, MERCINFO> s_mapMerc;

static void	MercenaryRemove		(PCHARINFO pMerc, PCHARINFO pOwner, LPCSTR pszFmt);	/* 傭兵を消す */
static int	GetMercRecipient	(PCHARINFO pOwner, PCHARINFO *apChar, int nMax);	/* 傭兵のことを知らせる相手 */
static BOOL	CanPutChar			(PCHARINFO pChar, DWORD dwMapID, int x, int y);		/* そのマスに立てるか */
static BOOL	FindPosNearOwner	(PCHARINFO pChar, PCHARINFO pOwner, int &x, int &y);	/* 雇い主のとなりの空いたマス */
static DWORD GetMercAtackWait	(PCHARINFO pChar);									/* 攻撃の間隔(ms) */


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::CCharProcMERCENARY							 */
/* 内容：	コンストラクタ													 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

CCharProcMERCENARY::CCharProcMERCENARY()
{
	m_dwOwnerX		= (DWORD)-1;
	m_dwOwnerY		= (DWORD)-1;
	m_dwEnemy		= (DWORD)-1;
	m_dwLastSearch	= 0;
	m_dwNoEnemyTime	= 0;
	m_dwLastAtack	= 0;
	m_dwStuckTime	= 0;
	m_dwDeadTime	= 0;
	m_dwLastSync	= 0;
	m_wSyncHP		= 0;
	m_wSyncMana		= 0;
	m_bySyncState	= 0;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::Proc										 */
/* 内容：	処理															 */
/*			傭兵を消した時は、この処理のオブジェクトも消えるので、			 */
/*			すぐに戻ること													 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::Proc(DWORD dwTime)
{
	PCHARINFO pOwner;

	/* 雇われていない(管理者が種類だけ変えた等)なら何もしない */
	if (s_mapMerc.find (m_pChar->dwTblID) == s_mapMerc.end ()) {
		return FALSE;
	}

	pOwner = GetMercenaryOwner (m_pChar);
	/* 雇い主がログアウトした？ */
	if (pOwner == NULL) {
		MercenaryRemove (m_pChar, NULL, NULL);
		return TRUE;
	}
	/* 雇い主が別のマップへ行った？ */
	if (pOwner->dwMapID != m_pChar->dwMapID) {
		MercenaryRemove (m_pChar, pOwner, "%sとはここでお別れです。（傭兵は雇ったマップの中だけ一緒に行動します）");
		return TRUE;
	}
	/* 雇い主が倒れた？ */
	if ((pOwner->wHP == 0) || (pOwner->byCharState == CHARSTATE_DEAD)) {
		MercenaryRemove (m_pChar, pOwner, "%sは帰っていきました。");
		return TRUE;
	}

	/* 倒れている？ 時間内に蘇生の道具を使ってもらえなければ帰る */
	if ((m_pChar->wHP == 0) || (m_pChar->byCharState == CHARSTATE_DEAD)) {
		/* 倒れたら列から外れる（引きずられないように） */
		MercenaryTailOff (m_pChar);
		if (m_dwDeadTime == 0) {
			m_dwDeadTime = dwTime;
		} else if ((DWORD)(dwTime - m_dwDeadTime) >= MERC_DEADTIME) {
			MercenaryRemove (m_pChar, pOwner, "%sは力つきて帰っていきました。");
			return TRUE;
		}
		SyncParty (pOwner, dwTime);
		return FALSE;
	}
	/* 起き上がった？ */
	if (m_dwDeadTime) {
		m_dwDeadTime			= 0;
		m_pChar->byCharState	= CHARSTATE_MOVE;
		m_dwEnemy				= (DWORD)-1;
		m_dwStuckTime			= dwTime;
	}

	SyncParty (pOwner, dwTime);

	BOOL bTail = IsInTail (pOwner);

	/* 戦う相手がいれば、列を外れて戦う */
	if (ProcBattle (pOwner, dwTime)) {
		if (bTail) {
			MercenaryTailOff (m_pChar);
		}
		m_dwOwnerX = pOwner->dwX;
		m_dwOwnerY = pOwner->dwY;
		return TRUE;
	}

	/* 列に入っていれば、雇い主が動いた時に列ごと動く（「ついていく」と同じ） */
	if (bTail) {
		m_dwStuckTime	= dwTime;
		m_dwOwnerX		= pOwner->dwX;
		m_dwOwnerY		= pOwner->dwY;
		return FALSE;
	}

	/* 列から外れている（戦い終わった・ほかの人が割り込んだ等）。
	   列のいちばん後ろのとなりまで来たら、列に入り直す */
	MercenaryTailOff (m_pChar);
	PCHARINFO pEnd = GetTailEnd (pOwner);
	if ((pEnd->dwTailBackTblID == (DWORD)-1) &&
		(abs ((int)pEnd->dwX - (int)m_pChar->dwX) + abs ((int)pEnd->dwY - (int)m_pChar->dwY) == 1)) {
		m_pChar->dwTailTblID	= pEnd->dwTblID;
		m_pChar->dwTailBackTblID = (DWORD)-1;
		pEnd->dwTailBackTblID	= m_pChar->dwTblID;
		m_dwStuckTime			= dwTime;
		return FALSE;
	}

	return ProcFollow (pEnd, dwTime);
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::IsInTail									 */
/* 内容：	雇い主の「ついていく」の列に入っているか						 */
/*			（前のキャラをたどって雇い主に着き、つながりが両向きで合っている）	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::IsInTail(PCHARINFO pOwner)
{
	int i;
	PCHARINFO pCur, pPrev;

	pCur = m_pChar;
	for (i = 0; i < 100; i ++) {
		if ((pCur->dwTailTblID == (DWORD)-1) || (pCur->dwTailTblID >= (DWORD)g_aCharInfo.GetSize ())) {
			return FALSE;
		}
		pPrev = g_aCharInfo[pCur->dwTailTblID];
		if ((pPrev->dwTailBackTblID != pCur->dwTblID) || (pPrev->dwMapID != m_pChar->dwMapID)) {
			return FALSE;
		}
		if (pPrev == pOwner) {
			return TRUE;
		}
		pCur = pPrev;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::GetTailEnd									 */
/* 内容：	雇い主の列のいちばん後ろのキャラ（傭兵自身は除く）				 */
/*			雇い主に「ついていく」している人がいれば、その人の後ろにつく	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

PCHARINFO CCharProcMERCENARY::GetTailEnd(PCHARINFO pOwner)
{
	int i;
	PCHARINFO pCur, pNext;

	pCur = pOwner;
	for (i = 0; i < 100; i ++) {
		if ((pCur->dwTailBackTblID == (DWORD)-1) || (pCur->dwTailBackTblID >= (DWORD)g_aCharInfo.GetSize ())) {
			break;
		}
		pNext = g_aCharInfo[pCur->dwTailBackTblID];
		if ((pNext == m_pChar) || (pNext->dwTailTblID != pCur->dwTblID) ||
			(pNext->dwMapID != pOwner->dwMapID)) {
			break;
		}
		pCur = pNext;
	}
	return pCur;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::ProcFollow									 */
/* 内容：	雇い主についていく												 */
/*			雇い主が1歩動いたら、雇い主が立っていたマスへ入る(ついていくと同じ)	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::ProcFollow(PCHARINFO pOwner, DWORD dwTime)
{
	int nDX, nDY, nDist;

	/* 雇い主が動いた？ */
	if ((pOwner->dwX != m_dwOwnerX) || (pOwner->dwY != m_dwOwnerY)) {
		DWORD dwPrevX = m_dwOwnerX, dwPrevY = m_dwOwnerY;

		m_dwOwnerX = pOwner->dwX;
		m_dwOwnerY = pOwner->dwY;

		/* 雇い主が立っていたマスが自分のとなりなら、そこへ入る */
		nDX = (int)dwPrevX - (int)m_pChar->dwX;
		nDY = (int)dwPrevY - (int)m_pChar->dwY;
		if ((dwPrevX != (DWORD)-1) && (abs (nDX) + abs (nDY) == 1)) {
			BYTE byDirection;
			if (nDY < 0) {
				byDirection = 0;
			} else if (nDY > 0) {
				byDirection = 1;
			} else if (nDX < 0) {
				byDirection = 2;
			} else {
				byDirection = 3;
			}
			m_pChar->byDirectionBack2	= m_pChar->byDirectionBack;
			m_pChar->byDirectionBack	= m_pChar->byDirection;
			m_pChar->dwXBack			= m_pChar->dwX;
			m_pChar->dwYBack			= m_pChar->dwY;
			m_pChar->byDirection		= byDirection;
			m_pChar->dwX				= dwPrevX;
			m_pChar->dwY				= dwPrevY;
			m_pChar->byFlg				|= (CHARINFOFLG_MOVE | CHARINFOFLG_DIRECTION);
			m_pChar->dwLastDotMove		= dwTime;
			m_dwStuckTime				= dwTime;
			return TRUE;
		}
	}

	nDX = (int)pOwner->dwX - (int)m_pChar->dwX;
	nDY = (int)pOwner->dwY - (int)m_pChar->dwY;
	nDist = abs (nDX) + abs (nDY);
	/* もうとなりにいる(雇い主が重なっている時は、雇い主が動くのを待つ) */
	if (nDist <= 1) {
		m_dwStuckTime = dwTime;
		return FALSE;
	}

	/* 離れすぎた・しばらく近づけない？ 雇い主のとなりへ移る */
	if ((nDist > MERC_WARPDIST) || ((DWORD)(dwTime - m_dwStuckTime) >= MERC_STUCKTIME)) {
		m_dwStuckTime = dwTime;
		return WarpNearOwner (pOwner);
	}

	if ((DWORD)(dwTime - m_pChar->dwLastDotMove) < MERC_FOLLOWSTEPTIME) {
		return FALSE;
	}
	return StepToward ((int)pOwner->dwX, (int)pOwner->dwY);
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::ProcBattle									 */
/* 内容：	近くの敵と戦う(戦っている間は TRUE)								 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::ProcBattle(PCHARINFO pOwner, DWORD dwTime)
{
	int nDX, nDY;
	PCHARINFO pEnemy;

	/* 戦闘できないマップ？ */
	if ((g_aMapInfo[m_pChar->dwMapID]->byFlg & MAPFLG_BATTLE) == 0) {
		m_dwEnemy = (DWORD)-1;
		return FALSE;
	}

	/* 戦っている敵がもういない・雇い主から離れすぎた？ */
	if (m_dwEnemy != (DWORD)-1) {
		BOOL bDrop = (IsEnemyValid (m_dwEnemy) == FALSE);
		if (!bDrop) {
			pEnemy = g_aCharInfo[m_dwEnemy];
			if ((abs ((int)pEnemy->dwX - (int)pOwner->dwX) + abs ((int)pEnemy->dwY - (int)pOwner->dwY) > MERC_LEASH) ||
				(abs ((int)m_pChar->dwX - (int)pOwner->dwX) + abs ((int)m_pChar->dwY - (int)pOwner->dwY) > MERC_LEASHSELF)) {
				bDrop = TRUE;
			}
		}
		if (bDrop) {
			m_dwEnemy = (DWORD)-1;
		}
	}

	/* 相手を探す */
	if (m_dwEnemy == (DWORD)-1) {
		if ((DWORD)(dwTime - m_dwLastSearch) < MERC_SEARCHTIME) {
			return FALSE;
		}
		m_dwLastSearch = dwTime;
		m_dwEnemy = FindEnemy (pOwner);
		if (m_dwEnemy == (DWORD)-1) {
			return FALSE;
		}
		/* 見つけてすぐには殴らず、少し間をおく */
		m_dwLastAtack = dwTime - GetMercAtackWait (m_pChar) / 2;
	}
	pEnemy = g_aCharInfo[m_dwEnemy];

	/* 目の前にいる？ 間隔がたったら殴る */
	if (IsFront (pEnemy)) {
		if ((DWORD)(dwTime - m_dwLastAtack) >= GetMercAtackWait (m_pChar)) {
			m_dwLastAtack			= dwTime;
			m_pChar->dwTargetTblID	= m_dwEnemy;
			m_pChar->byAtackRate	= 0;
			BattleProcAtack (m_pChar, pEnemy);
			m_pChar->dwTargetTblID	= (DWORD)-1;
			/* 倒した？ */
			if (IsEnemyValid (m_dwEnemy) == FALSE) {
				m_dwEnemy = (DWORD)-1;
			}
		}
		return TRUE;
	}

	/* となりにいるならそちらを向く */
	nDX = (int)pEnemy->dwX - (int)m_pChar->dwX;
	nDY = (int)pEnemy->dwY - (int)m_pChar->dwY;
	if (abs (nDX) + abs (nDY) == 1) {
		BYTE byDirection;
		if (nDY < 0) {
			byDirection = 0;
		} else if (nDY > 0) {
			byDirection = 1;
		} else if (nDX < 0) {
			byDirection = 2;
		} else {
			byDirection = 3;
		}
		if (m_pChar->byDirection != byDirection) {
			m_pChar->byDirection = byDirection;
			m_pChar->byFlg |= CHARINFOFLG_DIRECTION;
		}
		return TRUE;
	}

	/* 近づく */
	if ((DWORD)(dwTime - m_pChar->dwLastDotMove) >= MERC_STEPTIME) {
		StepToward ((int)pEnemy->dwX, (int)pEnemy->dwY);
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::FindEnemy									 */
/* 内容：	戦う相手を探す(いなければ -1)									 */
/*			雇い主か自分を狙っている敵、雇い主が狙っている敵、				 */
/*			雇い主のすぐ近く(2マス)・自分のとなりの敵のうち、一番近い敵		 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

DWORD CCharProcMERCENARY::FindEnemy(PCHARINFO pOwner)
{
	int j, nDistOwner, nDist, nDistMin;
	DWORD dwRet;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO pCharTmp;

	dwRet = (DWORD)-1;
	nDistMin = 0;
	pMap = g_aMapInfo[m_pChar->dwMapID];
	pData = pMap->paTblNoNPC;
	for (j = 0; j < (int)pMap->dwCountTblNoNPC; j ++) {
		if (pData[j] == -1) {
			break;
		}
		pCharTmp = g_aCharInfo[pData[j]];
		if ((pCharTmp->byNpcType != NPCTYPE_BATTLE1) && (pCharTmp->byNpcType != NPCTYPE_BATTLE2)) {
			continue;
		}
		if ((pCharTmp->wHP == 0) || (pCharTmp->byCharState == CHARSTATE_DEAD)) {
			continue;
		}
		nDistOwner = abs ((int)pCharTmp->dwX - (int)pOwner->dwX) + abs ((int)pCharTmp->dwY - (int)pOwner->dwY);
		if (nDistOwner > MERC_LEASH) {
			continue;
		}
		nDist = abs ((int)pCharTmp->dwX - (int)m_pChar->dwX) + abs ((int)pCharTmp->dwY - (int)m_pChar->dwY);
		if (!((pCharTmp->dwTargetTblID == pOwner->dwTblID) ||
			  (pCharTmp->dwTargetTblID == m_pChar->dwTblID) ||
			  (pOwner->dwTargetTblID == pCharTmp->dwTblID) ||
			  (nDistOwner <= 2) || (nDist <= 1))) {
			continue;
		}
		if ((dwRet == (DWORD)-1) || (nDist < nDistMin)) {
			dwRet = pCharTmp->dwTblID;
			nDistMin = nDist;
		}
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::IsEnemyValid								 */
/* 内容：	まだ戦える敵か(倒されて消えた・別のキャラに入れ替わった等を除く)	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::IsEnemyValid(DWORD dwTblID)
{
	PCHARINFO pCharTmp;

	if (dwTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return FALSE;
	}
	pCharTmp = g_aCharInfo[dwTblID];
	if ((pCharTmp->byNpcType != NPCTYPE_BATTLE1) && (pCharTmp->byNpcType != NPCTYPE_BATTLE2)) {
		return FALSE;
	}
	if ((pCharTmp->wHP == 0) || (pCharTmp->byCharState == CHARSTATE_DEAD) ||
		(pCharTmp->dwMapID != m_pChar->dwMapID)) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::IsFront										 */
/* 内容：	目の前にいるか(戦闘処理と同じ判定)								 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::IsFront(PCHARINFO pCharTmp)
{
	DWORD dwX, dwY;

	dwX = m_pChar->dwX + anPosAddTblX[m_pChar->byDirection];
	dwY = m_pChar->dwY + anPosAddTblY[m_pChar->byDirection];
	if (((CheckCharX (m_pChar, dwX, pCharTmp->dwX) == FALSE) && ((CheckCharX (pCharTmp, pCharTmp->dwX, dwX) == FALSE))) ||
		(dwY != pCharTmp->dwY)) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::CanStep										 */
/* 内容：	そのマスへ1歩進めるか											 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::CanStep(int x, int y)
{
	if (CanPutChar (m_pChar, m_pChar->dwMapID, x, y) == FALSE) {
		return FALSE;
	}
	/* ほかのNPCと重なる？ */
	if (CheckCharBlockNPC2 (m_pChar->dwTblID, m_pChar->dwMapID, (DWORD)x, (DWORD)y) != -1) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::StepToward									 */
/* 内容：	目標のマスへ1歩近づく(敵の「追いかける」と同じ決め方)			 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::StepToward(int nX, int nY)
{
	int nDX, nDY, nDirMain, nDirSub, nSide;

	nDX = nX - (int)m_pChar->dwX;
	nDY = nY - (int)m_pChar->dwY;
	if ((nDX == 0) && (nDY == 0)) {
		return FALSE;
	}

	/* 遠い方の軸を先に、だめならもう一方の軸 */
	nDirSub = -1;
	if (abs (nDX) >= abs (nDY)) {
		nDirMain = (nDX < 0) ? 2 : 3;
		if (nDY) {
			nDirSub = (nDY < 0) ? 0 : 1;
		}
	} else {
		nDirMain = (nDY < 0) ? 0 : 1;
		if (nDX) {
			nDirSub = (nDX < 0) ? 2 : 3;
		}
	}
	if (StepDirection (nDirMain)) {
		return TRUE;
	}
	if ((nDirSub >= 0) && StepDirection (nDirSub)) {
		return TRUE;
	}

	/* まっすぐ前がふさがっている時は横へよける */
	if (nDirSub < 0) {
		nSide = (nDirMain >= 2) ? 0 : 2;
		if (genrand () % 2) {
			nSide ++;
		}
		if (StepDirection (nSide)) {
			return TRUE;
		}
		if (StepDirection (abyDirectionTbl[nSide])) {
			return TRUE;
		}
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::StepDirection								 */
/* 内容：	指定の向きへ1歩進む												 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::StepDirection(int nDirection)
{
	int x, y;

	x = (int)m_pChar->dwX + anPosAddTblX[nDirection];
	y = (int)m_pChar->dwY + anPosAddTblY[nDirection];
	if (CanStep (x, y) == FALSE) {
		return FALSE;
	}

	m_pChar->byDirectionBack2	= m_pChar->byDirectionBack;
	m_pChar->byDirectionBack	= m_pChar->byDirection;
	m_pChar->dwXBack			= m_pChar->dwX;
	m_pChar->dwYBack			= m_pChar->dwY;
	m_pChar->byDirection		= (BYTE)nDirection;
	m_pChar->dwX				= (DWORD)x;
	m_pChar->dwY				= (DWORD)y;
	m_pChar->byFlg				|= (CHARINFOFLG_MOVE | CHARINFOFLG_DIRECTION);
	m_pChar->dwLastDotMove		= g_dwTime;

	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::WarpNearOwner								 */
/* 内容：	雇い主のとなりへ移る											 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

BOOL CCharProcMERCENARY::WarpNearOwner(PCHARINFO pOwner)
{
	int x, y;

	if (FindPosNearOwner (m_pChar, pOwner, x, y) == FALSE) {
		return FALSE;
	}
	m_pChar->dwXBack		= m_pChar->dwX = (DWORD)x;
	m_pChar->dwYBack		= m_pChar->dwY = (DWORD)y;
	m_pChar->byDirection	= pOwner->byDirection;
	m_pChar->dwLastDotMove	= g_dwTime;
	m_dwOwnerX				= pOwner->dwX;
	m_dwOwnerY				= pOwner->dwY;
	SendCmdCHARPOS (m_pChar->pID, m_pChar->dwTblID, 1, 2);
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	CCharProcMERCENARY::SyncParty									 */
/* 内容：	雇い主とパーティの仲間の、画面右下のHP・Mana一覧に並べる		 */
/*			パーティの解散などで一覧が消えても戻るよう、3秒ごとに知らせ直す	 */
/* 日付：	2026/11/08														 */
/* ========================================================================= */

void CCharProcMERCENARY::SyncParty(PCHARINFO pOwner, DWORD dwTime)
{
	int i, nCount;
	BOOL bPeriodic, bChange;
	PCHARINFO apChar[MAX_PARTYMEMBER + 1];

	bPeriodic	= ((m_dwLastSync == 0) || ((DWORD)(dwTime - m_dwLastSync) >= MERC_SYNCTIME));
	bChange		= ((m_wSyncHP != m_pChar->wHP) || (m_wSyncMana != m_pChar->wMana) ||
				   (m_bySyncState != m_pChar->byCharState));
	if (!bPeriodic && !bChange) {
		return;
	}

	nCount = GetMercRecipient (pOwner, apChar, MAX_PARTYMEMBER + 1);
	for (i = 0; i < nCount; i ++) {
		if (bPeriodic) {
			SendCmdPARTY (apChar[i]->dwTblID, m_pChar->dwTblID, CMDPARTY_JOINOK, 0);
		}
		SendCmdCHARSTATUSHPMANA (apChar[i]->dwTblID, m_pChar->dwTblID);
	}
	if (bPeriodic) {
		m_dwLastSync = (dwTime == 0) ? 1 : dwTime;
	}
	m_wSyncHP		= m_pChar->wHP;
	m_wSyncMana		= m_pChar->wMana;
	m_bySyncState	= m_pChar->byCharState;
}


/* ========================================================================= */
/* 関数名：	IsMercenary														 */
/* 内容：	傭兵か															 */
/* ========================================================================= */

BOOL IsMercenary(PCHARINFO pChar)
{
	return ((pChar != NULL) && (pChar->byNpcType == NPCTYPE_MERCENARY));
}


/* ========================================================================= */
/* 関数名：	MercenaryTailOff												 */
/* 内容：	傭兵を「ついていく」の列から外す（相手には何も送らない）		 */
/*			マップ移動で列ごと動かす処理に傭兵を乗せないためにも使う		 */
/* ========================================================================= */

void MercenaryTailOff(PCHARINFO pMerc)
{
	DWORD dwCount = (DWORD)g_aCharInfo.GetSize ();

	if (pMerc->dwTailTblID < dwCount) {
		PCHARINFO pPrev = g_aCharInfo[pMerc->dwTailTblID];
		if (pPrev->dwTailBackTblID == pMerc->dwTblID) {
			pPrev->dwTailBackTblID = (DWORD)-1;
		}
	}
	if (pMerc->dwTailBackTblID < dwCount) {
		PCHARINFO pNext = g_aCharInfo[pMerc->dwTailBackTblID];
		if (pNext->dwTailTblID == pMerc->dwTblID) {
			pNext->dwTailTblID = (DWORD)-1;
		}
	}
	pMerc->dwTailTblID		= (DWORD)-1;
	pMerc->dwTailBackTblID	= (DWORD)-1;
}


/* ========================================================================= */
/* 関数名：	GetMercenaryOwner												 */
/* 内容：	傭兵の雇い主(ログアウトした等でいなければ NULL)				 */
/* ========================================================================= */

PCHARINFO GetMercenaryOwner(PCHARINFO pMerc)
{
	PCHARINFO pOwner;
	std::map<DWORD, MERCINFO>::iterator it;

	if (IsMercenary (pMerc) == FALSE) {
		return NULL;
	}
	it = s_mapMerc.find (pMerc->dwTblID);
	if (it == s_mapMerc.end ()) {
		return NULL;
	}
	if (it->second.dwOwnerTblID >= (DWORD)g_aCharInfo.GetSize ()) {
		return NULL;
	}
	pOwner = g_aCharInfo[it->second.dwOwnerTblID];
	if ((pOwner->pID == 0) || (pOwner->pID != it->second.dwOwnerPID) || (pOwner->byNpcType != NPCTYPE_INIT) ||
		(pOwner->byGameState < GAMESTATE_MOVE) || (pOwner->byGameState == GAMESTATE_NPC)) {
		return NULL;
	}
	return pOwner;
}


/* ========================================================================= */
/* 関数名：	GetMercenaryFront												 */
/* 内容：	目の前にいる生きた傭兵(敵が殴る相手を探す時に使う)				 */
/* ========================================================================= */

PCHARINFO GetMercenaryFront(PCHARINFO pChar)
{
	DWORD dwX, dwY;
	PCHARINFO pMerc;
	std::map<DWORD, MERCINFO>::iterator it;

	if (s_mapMerc.empty ()) {
		return NULL;
	}
	dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
	dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];
	for (it = s_mapMerc.begin (); it != s_mapMerc.end (); it ++) {
		if (it->first >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		pMerc = g_aCharInfo[it->first];
		if (!IsMercenary (pMerc) || (pMerc->dwMapID != pChar->dwMapID)) {
			continue;
		}
		if ((pMerc->wHP == 0) || (pMerc->byCharState == CHARSTATE_DEAD)) {
			continue;
		}
		if (((CheckCharX (pChar, dwX, pMerc->dwX) == FALSE) && ((CheckCharX (pMerc, pMerc->dwX, dwX) == FALSE))) ||
			(dwY != pMerc->dwY)) {
			continue;
		}
		return pMerc;
	}
	return NULL;
}


/* ========================================================================= */
/* 関数名：	MercenaryTalk													 */
/* 内容：	傭兵所のNPC(台詞が「@m」で始まる)に話しかけた					 */
/*			雇っていなければ雇う、雇っていればお別れする					 */
/* ========================================================================= */

void MercenaryTalk(PCHARINFO pChar, PCHARINFO pCharNpc)
{
	int x, y, nTmpID;
	char szTmp[256];
	LPCSTR pszSpeech;
	PCHARINFO pCharNew;
	std::map<DWORD, MERCINFO>::iterator it;
	CCharProcBase CharProcBase;

	if ((pChar == NULL) || (pCharNpc == NULL) || (pChar->byNpcType != NPCTYPE_INIT)) {
		return;
	}

	/* もう雇っている？ お別れする */
	for (it = s_mapMerc.begin (); it != s_mapMerc.end (); it ++) {
		if (it->first >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		PCHARINFO pMerc = g_aCharInfo[it->first];
		if (GetMercenaryOwner (pMerc) == pChar) {
			MercenaryRemove (pMerc, pChar, "%sとお別れしました。");
			return;
		}
	}

	if ((pChar->wHP == 0) || (pChar->byCharState == CHARSTATE_DEAD)) {
		return;
	}

	/* 「@m05」のように数字があれば、その番号のステータステンプレートの強さ */
	nTmpID = -1;
	pszSpeech = pCharNpc->pszNpcSpeech;
	if (pszSpeech && (pszSpeech[2] >= '0') && (pszSpeech[2] <= '9')) {
		nTmpID = atoi (&pszSpeech[2]);
		if (nTmpID >= g_aStTmpInfo.GetSize ()) {
			nTmpID = -1;
		}
	}

	pCharNew = new CHARINFO;
	ZeroMemory ((PBYTE)pCharNew, sizeof (CHARINFO));
	pCharNew->byNpcType = NPCTYPE_MERCENARY;
	SetInitCharData (pCharNew);
	pCharNew->byGameState	= GAMESTATE_NPC;
	pCharNew->byCharState	= CHARSTATE_MOVE;
	pCharNew->dwMapID		= pChar->dwMapID;
	pCharNew->dwStartMapID	= pChar->dwMapID;
	pCharNew->wGrpID		= pCharNpc->wGrpID;
	pCharNew->wGrpMain		= pCharNpc->wGrpMain;
	pCharNew->wGrpSub		= pCharNpc->wGrpSub;
	pCharNew->clName		= pCharNpc->clName;
	pCharNew->clSpeech		= pCharNpc->clSpeech;
	pCharNew->byDirection	= pChar->byDirection;
	pCharNew->dwGold		= 0;
	pCharNew->dwExp			= 0;
	strncpy (pCharNew->szName, pCharNpc->szName, sizeof (pCharNew->szName) - 1);
	/* 名前の無いNPC(台詞の【】で名乗っている人も多い)なら「傭兵」 */
	if (pCharNew->szName[0] == 0) {
		strcpy (pCharNew->szName, "傭兵");
	}

	if (FindPosNearOwner (pCharNew, pChar, x, y) == FALSE) {
		delete pCharNew;
		SendCmdSYSMSG (pChar->pID, "ここは狭いので、少し広い場所で声をかけてください。", SPEECHCMD_BATTLE1);
		return;
	}
	pCharNew->dwXBack	= pCharNew->dwX = pCharNew->dwStartX = (DWORD)x;
	pCharNew->dwYBack	= pCharNew->dwY = pCharNew->dwStartY = (DWORD)y;

	if (nTmpID >= 0) {
		/* ステータステンプレートの強さ（特技は使わない） */
		PSTATUSTEMPLATEINFO pTmp = g_aStTmpInfo[nTmpID];
		pCharNew->byLevel		= pTmp->byLevel;
		pCharNew->byPW			= pTmp->byPW;
		pCharNew->byDEX			= pTmp->byDEX;
		pCharNew->byST			= pTmp->byST;
		pCharNew->byIT			= pTmp->byIT;
		pCharNew->byLK			= pTmp->byLK;
		pCharNew->wMaxHP		= pTmp->wMaxHP;
		pCharNew->wMaxMana		= pTmp->wMaxMana;
		pCharNew->wDFMagic		= pTmp->wDFMagic;
		pCharNew->chAttrFire	= pTmp->chAttrFire;
		pCharNew->chAttrWater	= pTmp->chAttrWater;
		pCharNew->chAttrWind	= pTmp->chAttrWind;
		pCharNew->chAttrEarth	= pTmp->chAttrEarth;
		pCharNew->chAttrLight	= pTmp->chAttrLight;
		pCharNew->chAttrDark	= pTmp->chAttrDark;
	} else {
		/* 雇い主と同じくらいの強さ */
		pCharNew->byLevel		= pChar->byLevel;
		pCharNew->byPW			= pChar->byPW;
		pCharNew->byDEX			= pChar->byDEX;
		pCharNew->byST			= pChar->byST;
		pCharNew->byIT			= pChar->byIT;
		pCharNew->byLK			= pChar->byLK;
		pCharNew->wMaxHP		= pChar->wMaxHP;
		pCharNew->wMaxMana		= pChar->wMaxMana;
		pCharNew->wDFMagic		= pChar->wDFMagic;
	}
	pCharNew->wHP	= pCharNew->wMaxHP = max (pCharNew->wMaxHP, (WORD)1);
	pCharNew->wMana	= pCharNew->wMaxMana;
	ZeroMemory (pCharNew->abyMagic, sizeof (pCharNew->abyMagic));
	ZeroMemory (pCharNew->waItem, sizeof (pCharNew->waItem));

	pCharNew = AddChar (pCharNew);
	pCharNew->pID = pCharNew->dwTblID + 1;
	if (pCharNew->pProc == NULL) {
		CharProcBase.SetProc (pCharNew);
	}

	/* 攻撃力・守備力 */
	RenewATDF (pCharNew->dwTblID);
	if (nTmpID < 0) {
		pCharNew->wAT = pChar->wAT;
		pCharNew->wDF = pChar->wDF;
	}

	MERCINFO info;
	info.dwOwnerTblID	= pChar->dwTblID;
	info.dwOwnerPID		= pChar->pID;
	s_mapMerc[pCharNew->dwTblID] = info;

	SendCmdCHARINFO3 (pCharNew->dwTblID);

	snprintf (szTmp, sizeof (szTmp), "%sが仲間になりました！ このマップの中で一緒に戦ってくれます。（もう一度話しかけるとお別れ）", pCharNew->szName);
	SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
}


/* ========================================================================= */
/* 関数名：	MercenaryDamaged												 */
/* 内容：	傭兵が攻撃を受けた後の処理(倒れたら TRUE)						 */
/* ========================================================================= */

BOOL MercenaryDamaged(PCHARINFO pChar, PCHARINFO pMerc)
{
	char szTmp[256];
	PCHARINFO pOwner;

	if (pMerc->wHP > 0) {
		return FALSE;
	}

	pMerc->byCharState = CHARSTATE_DEAD;
	/* 殴った敵は狙うのをやめる */
	if (pChar && (pChar->dwTargetTblID == pMerc->dwTblID)) {
		pChar->dwLastBattle		= 0;
		pChar->dwTargetTblID	= (DWORD)-1;
	}
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pMerc->dwTblID);

	pOwner = GetMercenaryOwner (pMerc);
	if (pOwner) {
		snprintf (szTmp, sizeof (szTmp), "%sは倒れてしまった！ 10秒以内に蘇生の道具を使うと起き上がります。", pMerc->szName);
		SendCmdSYSMSG (pOwner->pID, szTmp, SPEECHCMD_BATTLE1);
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	MercenaryItemCheck												 */
/* 内容：	その種類の道具を傭兵に使えるか(回復・蘇生だけ)					 */
/* ========================================================================= */

BOOL MercenaryItemCheck(PCHARINFO pMerc, BYTE byItemType)
{
	if (IsMercenary (pMerc) == FALSE) {
		return TRUE;
	}
	switch (byItemType) {
	case ITEMTYPE_HPUP:			/* HP回復 */
	case ITEMTYPE_MANAUP:		/* Mana回復 */
	case ITEMTYPE_HPMANAUP:		/* HP・Mana回復 */
	case ITEMTYPE_RESURRECTION:	/* 蘇生 */
		return TRUE;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	MercenaryRemove													 */
/* 内容：	傭兵を消す。pszFmt は雇い主へのお知らせ(%s が傭兵の名前)		 */
/*			傭兵の処理(this)も消えるので、呼んだ後はすぐ戻ること			 */
/* ========================================================================= */

static void MercenaryRemove(PCHARINFO pMerc, PCHARINFO pOwner, LPCSTR pszFmt)
{
	int i, j, nCount;
	char szTmp[256];
	DWORD dwTblID;
	PMAPINFO pMap;
	PDWORD pData;
	PCHARINFO apChar[MAX_PARTYMEMBER + 1];

	dwTblID = pMerc->dwTblID;

	if (pOwner) {
		/* 画面右下のHP・Mana一覧から外す */
		nCount = GetMercRecipient (pOwner, apChar, MAX_PARTYMEMBER + 1);
		for (i = 0; i < nCount; i ++) {
			SendCmdPARTY (apChar[i]->dwTblID, dwTblID, CMDPARTY_BREAK, 0);
		}
		if (pszFmt) {
			snprintf (szTmp, sizeof (szTmp), pszFmt, pMerc->szName);
			SendCmdSYSMSG (pOwner->pID, szTmp, SPEECHCMD_BATTLE1);
		}
	}

	/* 傭兵を狙っていた敵は狙うのをやめる(番号が別のキャラに使い回されるため) */
	pMap = g_aMapInfo[pMerc->dwMapID];
	pData = pMap->paTblNoNPC;
	for (j = 0; j < (int)pMap->dwCountTblNoNPC; j ++) {
		if (pData[j] == -1) {
			break;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pData[j]];
		if (pCharTmp->dwTargetTblID == dwTblID) {
			pCharTmp->dwTargetTblID	= (DWORD)-1;
			pCharTmp->dwLastBattle	= 0;
		}
	}

	/* 「ついていく」の列から外す（番号が別のキャラに使い回されるため） */
	MercenaryTailOff (pMerc);

	s_mapMerc.erase (dwTblID);
	DeleteChar (dwTblID);
}


/* ========================================================================= */
/* 関数名：	GetMercRecipient												 */
/* 内容：	傭兵のことを知らせる相手(雇い主と、雇い主のパーティの仲間)		 */
/* ========================================================================= */

static int GetMercRecipient(PCHARINFO pOwner, PCHARINFO *apChar, int nMax)
{
	int i, nCount;
	PPARTYINFO pPartyInfo;

	nCount = 0;
	if (pOwner->pID == 0) {
		return 0;
	}
	apChar[nCount ++] = pOwner;

	if ((pOwner->dwPartyID == 0) || (pOwner->dwPartyID >= (DWORD)g_aPartyInfo.GetSize ())) {
		return nCount;
	}
	pPartyInfo = g_aPartyInfo[pOwner->dwPartyID];
	if (pPartyInfo == NULL) {
		return nCount;
	}
	for (i = 0; (i < MAX_PARTYMEMBER) && (nCount < nMax); i ++) {
		if (pPartyInfo->adwTblID[i] == 0) {
			break;
		}
		if (pPartyInfo->adwTblID[i] >= (DWORD)g_aCharInfo.GetSize ()) {
			continue;
		}
		PCHARINFO pCharTmp = g_aCharInfo[pPartyInfo->adwTblID[i]];
		if ((pCharTmp == pOwner) || (pCharTmp->pID == 0) || (pCharTmp->byNpcType != NPCTYPE_INIT)) {
			continue;
		}
		apChar[nCount ++] = pCharTmp;
	}
	return nCount;
}


/* ========================================================================= */
/* 関数名：	CanPutChar														 */
/* 内容：	そのマスに立てるか(地形・イベント・プレイヤー)					 */
/* ========================================================================= */

static BOOL CanPutChar(PCHARINFO pChar, DWORD dwMapID, int x, int y)
{
	PMAPINFO pMap;

	pMap = g_aMapInfo[dwMapID];
	if ((x < 0) || (x >= pMap->nCX) || (y < 0) || (y >= pMap->nCY)) {
		return FALSE;
	}
	/* 障害物？ */
	if (CheckCharBlock (pChar, dwMapID, (DWORD)x, (DWORD)y) == FALSE) {
		return FALSE;
	}
	/* 扉・ワープなどのイベント？ */
	if (CheckEventBlock (dwMapID, (DWORD)x, (DWORD)y)) {
		return FALSE;
	}
	/* プレイヤーと重なる？ */
	if (CheckCharBlockPC (dwMapID, (DWORD)x, (DWORD)y) != -1) {
		return FALSE;
	}
	return TRUE;
}


/* ========================================================================= */
/* 関数名：	FindPosNearOwner												 */
/* 内容：	雇い主のとなりの空いたマス(後ろを先に探す)						 */
/* ========================================================================= */

static BOOL FindPosNearOwner(PCHARINFO pChar, PCHARINFO pOwner, int &x, int &y)
{
	int i, nDirection;
	int anDirection[5];

	/* 雇い主の後ろ、左右、前の順 */
	nDirection = abyDirectionTbl[pOwner->byDirection % 4];
	anDirection[0] = nDirection;
	anDirection[1] = (nDirection >= 2) ? 0 : 2;
	anDirection[2] = (nDirection >= 2) ? 1 : 3;
	anDirection[3] = pOwner->byDirection % 4;
	for (i = 0; i < 4; i ++) {
		x = (int)pOwner->dwX + anPosAddTblX[anDirection[i]];
		y = (int)pOwner->dwY + anPosAddTblY[anDirection[i]];
		if (CanPutChar (pChar, pOwner->dwMapID, x, y) == FALSE) {
			continue;
		}
		if (CheckCharBlockNPC2 (pChar->dwTblID, pOwner->dwMapID, (DWORD)x, (DWORD)y) != -1) {
			continue;
		}
		return TRUE;
	}
	return FALSE;
}


/* ========================================================================= */
/* 関数名：	GetMercAtackWait												 */
/* 内容：	攻撃の間隔(ms)。素早さ0で3秒〜255で1秒(プレイヤーのゲージと同じ幅)	 */
/* ========================================================================= */

static DWORD GetMercAtackWait(PCHARINFO pChar)
{
	int nTmp;

	nTmp = pChar->byDEX;
	nTmp = nTmp * 100 / 255;
	nTmp = max (nTmp, 1);
	nTmp = 1000 + (2000 - ((2000 * nTmp) / 100));
	/* 加速していれば間隔が短くなる */
	nTmp = nTmp * 100 / (100 + GetHasteRate (pChar));

	return (DWORD)nTmp;
}

/* Copyright(C)URARA-works 2026 */
