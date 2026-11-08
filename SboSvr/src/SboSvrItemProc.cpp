/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboSvrItemProc.cpp											 */
/* 概要：		アイテム処理												 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成日：		2004/07/15													 */
/* ========================================================================= */
#include "include.h"
#include "Glbldata.h"
#include "Proc/SubGame/ProcSubGameWAR.h"
#include "Proc/ProcSubGame.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrBattleProc.h"
#include "SboSvrMagicProc.h"
#include "SboSvrCharInfoTimerProc.h"
#include "SboSvrSubGameProcSugoroku.h"
#include "CharProc/CharProcMERCENARY.h"

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void ItemProc							(DWORD, BYTE, DWORD);						/* アイテム処理 */
BOOL CheckItemSexDistinction			(PITEMINFO, PCHARINFO);						/* 性別による使えるかチェック */
static BOOL	CheckUseTarget				(PITEMINFO, PCHARINFO);						/* 他のキャラに使えるかチェック */
static void	PlayItemSound				(PITEMINFO, PCHARINFO);						/* 効果音の再生 */
static void	SendSysMsg					(PCHARINFO, PCHARINFO, LPCTSTR);			/* メッセージ送信 */
static BOOL	ItemUse						(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理 */
static BOOL	ItemUse_NONE				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(効果無し) */
static BOOL	ItemUse_RURA				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(指定位置へ移動) */
static BOOL	ItemUse_HPUP				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(HP回復) */
static BOOL	ItemUse_SEED				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(種) */
static BOOL	ItemUse_SUBGAME				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(サブゲーム参加) */
static int	ItemUse_SUBGAME_BOMB		(PCHARINFO, BYTE, LPSTR);					/* アイテム処理(サブゲーム参加：爆弾ゲーム) */
static int	ItemUse_SUBGAME_WARold		(PCHARINFO, BYTE, LPSTR);					/* アイテム処理(サブゲーム参加：戦場ゲーム) */
static int	ItemUse_SUBGAME_WAR			(PCHARINFO, CProcSubGameWAR *pProc, LPSTR);	/* アイテム処理(サブゲーム参加：戦場ゲーム) */
static int	ItemUse_SUBGAME_SUGOROKU	(PCHARINFO, BYTE, LPSTR);					/* アイテム処理(サブゲーム参加：すごろく) */
static BOOL	ItemUse_SAKE				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(酒) */
static BOOL	ItemUse_MSG					(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(メッセージ表示) */
static BOOL	ItemUse_GRPCHG				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(キャラ画像変更) */
static BOOL	ItemUse_LIGHT				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(明かり) */
static BOOL	ItemUse_HPDOWN				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(HP減少) */
static BOOL	ItemUse_MAGIC				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(魔法習得) */
static BOOL	ItemUse_MANAUP				(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(Mana回復) */
static BOOL	ItemUse_HPMANAUP			(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(HP・Mana回復) */
static BOOL	ItemUse_RESURRECTION		(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(蘇生) */
static BOOL	ItemUse_USEMAGIC			(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);	/* アイテム処理(特技発動) */

extern long genrand();													/* 乱数取得 */

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
typedef BOOL (*ITEMUSE)(PCHARINFO, PCHARINFO, PITEMINFO, LPSTR);

/* 関数ジャンプテーブル */
static const ITEMUSE ItemUseTbl[] = {
	ItemUse_NONE,					/* 効果無し */
	ItemUse_RURA,					/* 特定位置へ移動 */
	ItemUse_HPUP,					/* HP回復 */
	NULL,							/* 武器 */
	NULL,							/* 鎧 */
	NULL,							/* 盾 */
	NULL,							/* 兜 */
	ItemUse_SEED,					/* 種 */
	ItemUse_SUBGAME,				/* サブゲーム参加 */
	ItemUse_SAKE,					/* 酒 */
	ItemUse_MSG,					/* メッセージ表示 */
	ItemUse_GRPCHG,					/* キャラ画像変更 */
	ItemUse_LIGHT,					/* 明かり */
	ItemUse_HPDOWN,					/* HP減少 */
	ItemUse_MAGIC,					/* 魔法習得 */
	ItemUse_MANAUP,					/* Mana回復 */
	ItemUse_HPMANAUP,				/* HP・Mana回復 */
	NULL,							/* 装備(小手) */
	NULL,							/* 装備(靴) */
	NULL,							/* 装備(首飾り) */
	NULL,							/* 装備(指輪1) */
	NULL,							/* 装備(指輪2) */
	ItemUse_RESURRECTION,			/* 蘇生 */
	ItemUse_USEMAGIC,				/* 特技発動 */
};

/* ========================================================================= */
/* 関数名：ItemProc															 */
/* 内容：アイテム処理														 */
/* 日付：2003/09/15															 */
/* ========================================================================= */
void ItemProc(DWORD dwTblID, BYTE byNo, DWORD dwUseTblID)
{
	BOOL bDrop, bResult;
	char szMsg[1024], szTmp[512];
	PITEMINFO pItem;
	PCHARINFO pChar, pCharUse;

	pChar = g_aCharInfo[dwTblID];
	pCharUse = g_aCharInfo[dwUseTblID];
	pItem = g_aItemInfo[pChar->waItem[byNo]];
	bDrop = TRUE;
	ZeroMemory (szMsg, sizeof (szMsg));

	/*********************************
		他キャラに使えるかチェック
	*********************************/

	if (dwTblID != dwUseTblID) {
		BOOL bCheck;
		DWORD dwX, dwY;

		bCheck = FALSE;
		dwX = pChar->dwX + anPosAddTblX[pChar->byDirection];
		dwY = pChar->dwY + anPosAddTblY[pChar->byDirection];

		/* オンライン中？ */
		if (pCharUse->byGameState >= GAMESTATE_MOVE) {
			/* 同じマップ？ */
			if (pChar->dwMapID == pCharUse->dwMapID) {
				/* 目の前にいる？ */
				if (CheckCharX (pCharUse, pCharUse->dwX, dwX) && (dwY == pCharUse->dwY)) {
					bCheck = TRUE;
				}
			}
		}

		/* 目の前にはいないらしい */
		if (bCheck == FALSE) {
			wsprintf (szMsg, "%sは前にいないので使えません。@2", pCharUse->szName);
			goto Exit;
		}

		bResult = CheckUseTarget (pItem, pCharUse);
		if (bResult == FALSE) {
			strcpy (szMsg, "自分以外には使えません。@2");
			goto Exit;
		}
		/* 傭兵に使えるのは回復・蘇生の道具だけ */
		if (MercenaryItemCheck (pCharUse, pItem->byType) == FALSE) {
			wsprintf (szMsg, "%sには使えません。@2", pCharUse->szName);
			goto Exit;
		}
	}


	/**********************************
		性別による使えるかチェック
	**********************************/

	bResult = CheckItemSexDistinction (pItem, pCharUse);

	/* 使用不可？ */
	if (bResult == FALSE) {
		wsprintf (szMsg, "%sには使用できないようです。@2", pCharUse->szName);
		goto Exit;
	}


	/*******************
		アイテム処理
	*******************/

	wsprintf (szTmp, "%sは %s を使いました。", pChar->szName, pItem->szName);
	SendSysMsg (pChar, pCharUse, szTmp);

	bResult = ItemUse (pChar, pCharUse, pItem, szMsg);
	/* アイテムを使用した？ */
	if (bResult) {
		if (pItem->byEraseProbability > 0) {
			if (genrand () % 100 >= pItem->byEraseProbability) {
				/* 今回は無くならない */
				bDrop = FALSE;
			}
		}
		/* 使うと無くなる？ */
		if (pItem->byFlg & ITEMFLG_LOST && bDrop) {
			DelCharItem (dwTblID, byNo);
			SendCmdCHARITEM (pChar->pID, pChar->dwTblID);
		}
	}

	if ((szMsg[0] < 0) && (szMsg[1] == 0)) {
		ZeroMemory (szMsg, sizeof (szMsg));

	} else {
		/* 使用しなくてメッセージが指定されていない？ */
		if (bResult == FALSE) {
			if (strlen (szMsg) == 0) {
				if (pItem->pMsg && strlen (pItem->pMsg)) {
					strcpy (szMsg, pItem->pMsg);
				} else {
					strcpy (szMsg, "何も起こりませんでした。@2");
				}
			}

		} else {
			if (strlen (szMsg) == 0) {
				if (pItem->pMsg && strlen (pItem->pMsg)) {
					strcpy (szMsg, pItem->pMsg);
				}
			}
		}
	}

Exit:
	/* 結果メッセージを表示 */
	if (strlen (szMsg) > 0) {
		SendCmdMAPMSG (pChar->pID, szMsg);
	}
	return;
}

/* ========================================================================= */
/* 関数名：CheckItemSexDistinction											 */
/* 内容：性別による使えるかチェック											 */
/* 日付：2004/07/18															 */
/* ========================================================================= */
BOOL CheckItemSexDistinction(PITEMINFO pItem, PCHARINFO pChar)
{
	BOOL bRet;

	bRet = FALSE;
	if (pItem->bySexDistinction == SEXDISTINCTION_ALL) {
		bRet = TRUE;

	/* 女性？ */
	} else if (pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE) {
		if (pItem->bySexDistinction == SEXDISTINCTION_FEMALE) {
			bRet = TRUE;
		}

	/* 男性 */
	} else {
		if (pItem->bySexDistinction == SEXDISTINCTION_MALE) {
			bRet = TRUE;
		}
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：CheckUseTarget													 */
/* 内容：他のキャラに使えるかチェック										 */
/* 日付：2003/09/15															 */
/* ========================================================================= */
static BOOL CheckUseTarget(PITEMINFO pItem, PCHARINFO pCharTarget)
{
	BOOL bRet = FALSE;

	switch (pItem->byType) {
	case ITEMTYPE_HPUP:			/* HP回復 */
	case ITEMTYPE_MSG:			/* メッセージ表示 */
	case ITEMTYPE_MANAUP:		/* Mana回復 */
	case ITEMTYPE_HPMANAUP:		/* HP・Mana回復 */
	case ITEMTYPE_HPDOWN:		/* HP減少 */
	case ITEMTYPE_RESURRECTION:	/* 蘇生 */
	case ITEMTYPE_USEMAGIC:		/* 特技発動 */
		bRet = TRUE;
		break;
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：PlayItemSound													 */
/* 内容：効果音の再生														 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static void PlayItemSound(PITEMINFO pItem, PCHARINFO pChar)
{
	if (pItem->bySound) {
		SendCmdPLAYSOUND (pChar->pID, pItem->bySound);
	}
}

/* ========================================================================= */
/* 関数名：SendSysMsg														 */
/* 内容：メッセージ送信														 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static void SendSysMsg(PCHARINFO pChar, PCHARINFO pCharTarget, LPCTSTR pszMsg)
{
	SendCmdSYSMSG (pChar->pID, pszMsg, SPEECHCMD_BATTLE1);

	/* 他のキャラに使った？ */
	if (pChar != pCharTarget) {
		SendCmdSYSMSG (pCharTarget->pID, pszMsg, SPEECHCMD_BATTLE1);
	}
}

/* ========================================================================= */
/* 関数名：ItemUse															 */
/* 内容：アイテム処理														 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;

	bRet = FALSE;

	/* 種類別に処理 */
	if ((pItem->byType < sizeof (ItemUseTbl) / sizeof (ItemUseTbl[0])) && ItemUseTbl[pItem->byType]) {
		bRet = ItemUseTbl[pItem->byType] (pChar, pCharTarget, pItem, pszMsg);
	}

	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_NONE														 */
/* 内容：アイテム処理(効果無し)												 */
/* 日付：2004/07/18															 */
/* ========================================================================= */
static BOOL ItemUse_NONE(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;

	bRet = FALSE;

	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}
	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	if (pItem->pMsg && strlen (pItem->pMsg)) {
		bRet = TRUE;
	}

Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_RURA														 */
/* 内容：アイテム処理(指定位置へ移動)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_RURA(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;
	DWORD dwTmp;
	PMAPINFO pMap;
	PITEMINFO_RURA pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_RURA)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	pMap = g_aMapInfo[pChar->dwMapID];
	dwTmp = (pInfo->dwEnableMapID & ~0x80000000);

	/* 禁止されているマップ？ */
	if (pMap->byFlg & MAPFLG_MOVE) {
		strcpy (pszMsg, "ここでは使えません。@2");
		goto Exit;
	}

	/* マップ指定？ */
	if (pInfo->dwEnableMapID & 0x80000000) {
		/* このマップでは使えない？ */
		if (pChar->dwMapID != dwTmp) {
			strcpy (pszMsg, "ここでは使えません。@2");
			goto Exit;
		}
	}

	SendCmdCHGBGM (pChar->pID, pChar->dwMapID, 0);

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	CharTargetCancelPC (pChar);
	SendCmdFADEINOUT (pChar->pID, 0);
	SubProcMapMove (pChar, pInfo->dwMapID, pInfo->dwX, pInfo->dwY);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_HPUP														 */
/* 内容：アイテム処理(HP回復)												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_HPUP(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int nTmp;
	BOOL bRet;
	WORD wTmp;
	char szTmp[64], szTmp2[16];
	PITEMINFO_HPUP pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_HPUP)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* すでに満たん？ */
	if (pCharTarget->wHP == pCharTarget->wMaxHP) {
		SendCmdSYSMSG (pChar->pID, "今は使う必要がありません。", SPEECHCMD_BATTLE1);
		*pszMsg = -1;
		goto Exit;
	}

	/* 確立で回復？ */
	if (pInfo->byRate) {
		wTmp = (pCharTarget->wMaxHP * 10 * pInfo->wMin / 1000);

	} else {
		nTmp = pInfo->wMax - pInfo->wMin + 1;
		nTmp = max (1, nTmp);
		wTmp = (pInfo->wMin + ((WORD)genrand () % nTmp));
	}

	/* 最大値を超えてしまう？ */
	if (wTmp + pCharTarget->wHP > pCharTarget->wMaxHP) {
		wTmp = pCharTarget->wMaxHP - pCharTarget->wHP;
	}

	pCharTarget->wHP += wTmp;

	/* 効果音再生 */
	PlayItemSound (pItem, pCharTarget);

	/* 文字エフェクト発動 */
	wsprintf (szTmp2, "%d", wTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp2);

	/* 満たんになった？ */
	if (pCharTarget->wHP == pCharTarget->wMaxHP) {
		wsprintf (szTmp, "%sのHPは完全に回復しました。", pCharTarget->szName);

	} else {
		wsprintf (szTmp, "%sのHPが%dポイント回復しました。", pCharTarget->szName, wTmp);
	}

	/* 回復メッセージ表示 */
	SendSysMsg (pChar, pCharTarget, szTmp);

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);
	pCharTarget->byEfect = EFECTTYPE_HOIMI;
	SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
	pCharTarget->byEfect = 0;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SEED														 */
/* 内容：アイテム処理(種)													 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_SEED(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int nTmp;
	BOOL bRet;
	char szTmp[16];
	PITEMINFO_SEED pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_SEED)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	nTmp = (genrand () % 2) + 1;

	switch (pInfo->byType) {
	case STATUSTYPE_PW:		/* 力 */
		if (pCharTarget->byPW >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->byPW + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->byPW;
		}
		pCharTarget->byPW += nTmp;
		wsprintf (pszMsg, "%sの力が%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_DEX:	/* 素早さ */
		if (pCharTarget->byDEX >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->byDEX + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->byDEX;
		}
		pCharTarget->byDEX += nTmp;
		wsprintf (pszMsg, "%sの素早さが%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_ST:	/* 体力 */
		if (pCharTarget->byST >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->byST + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->byST;
		}
		pCharTarget->byST += nTmp;
		wsprintf (pszMsg, "%sの体力が%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_IT:	/* 賢さ */
		if (pCharTarget->byIT >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->byIT + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->byIT;
		}
		pCharTarget->byIT += nTmp;
		wsprintf (pszMsg, "%sの賢さが%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_LK:	/* 運のよさ */
		if (pCharTarget->byLK >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->byLK + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->byLK;
		}
		pCharTarget->byLK += nTmp;
		wsprintf (pszMsg, "%sの運のよさが%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_MAXHP:	/* 最大HP */
		if (pCharTarget->wMaxHP >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->wMaxHP + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->wMaxHP;
		}
		pCharTarget->wMaxHP += nTmp;
		wsprintf (pszMsg, "%sの最大HPが%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_MAXMANA:	/* 最大Mana */
		if (pCharTarget->wMaxMana >= (BYTE)pInfo->wMax) {
			goto Exit;
		}
		if (pCharTarget->wMaxMana + nTmp >= pInfo->wMax) {
			nTmp = pInfo->wMax - pCharTarget->wMaxMana;
		}
		pCharTarget->wMaxMana += nTmp;
		wsprintf (pszMsg, "%sの最大Manaが%dポイント増えました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_PW2:		/* 力減少 */
		if ((int)(pCharTarget->byPW - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->byPW -= nTmp;
		wsprintf (pszMsg, "%sの力が%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_DEX2:		/* 素早さ減少 */
		if ((int)(pCharTarget->byDEX - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->byDEX -= nTmp;
		wsprintf (pszMsg, "%sの素早さが%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_ST2:		/* 体力減少 */
		if ((int)(pCharTarget->byST - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->byST -= nTmp;
		wsprintf (pszMsg, "%sの体力が%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_IT2:		/* 賢さ減少 */
		if ((int)(pCharTarget->byIT - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->byIT -= nTmp;
		wsprintf (pszMsg, "%sの賢さが%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_LK2:		/* 運のよさ減少 */
		if ((int)(pCharTarget->byLK - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->byLK -= nTmp;
		wsprintf (pszMsg, "%sの運のよさが%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_MAXHP2:		/* 最大HP減少 */
		if ((int)(pCharTarget->wMaxHP - nTmp) <= 0) {
			goto Exit;
		}
		pCharTarget->wMaxHP -= nTmp;
		if (pCharTarget->wHP > pCharTarget->wMaxHP) {
			pCharTarget->wHP = pCharTarget->wMaxHP;
		}
		wsprintf (pszMsg, "%sの最大HPが%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;

	case STATUSTYPE_MAXMANA2:	/* 最大Mana減少 */
		if ((int)(pCharTarget->wMaxMana - nTmp) < 0) {
			goto Exit;
		}
		pCharTarget->wMaxMana -= nTmp;
		if (pCharTarget->wMana > pCharTarget->wMaxMana) {
			pCharTarget->wMana = pCharTarget->wMaxMana;
		}
		wsprintf (pszMsg, "%sの最大Manaが%dポイント減ってしまいました！@2", pCharTarget->szName, nTmp);
		break;
	}

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);

	wsprintf (szTmp, "%d", nTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 255, 213), szTmp);

	pCharTarget->byEfect = EFECTTYPE_HOIMI;
	SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
	pCharTarget->byEfect = 0;

	/* パラメータを更新 */
	RenewATDF (pCharTarget->dwTblID);

	bRet = TRUE;
Exit:
	if (bRet == FALSE) {
		if (strlen (pszMsg) == 0) {
			wsprintf (pszMsg, "今の%sには効果がないようです。@2", pCharTarget->szName);
		}
	}
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SUBGAME													 */
/* 内容：アイテム処理(サブゲーム参加)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_SUBGAME(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int i, nCount, nProc, nProcTmp;
	BOOL bRet;
	PITEMINFO_SUBGAME pUseInfo;
	PSUBGAMEINFO pInfo;
	PCProcSubGameBase pProc;

	nProc = 0;
	bRet = FALSE;
	pUseInfo = (PITEMINFO_SUBGAME)pItem->pData;

	if (pUseInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	nCount = g_ProcSubGame.GetCount ();
	for (i = 0; i < nCount; i ++) {
		nProcTmp	= -1;
		pProc		= g_ProcSubGame.Get (i);

		if (pUseInfo->byType != pProc->GetType ()) {
			continue;
		}
		switch (pUseInfo->byType) {
		case SUBGAMEID_WAR:			/* 戦場ゲーム */
			nProcTmp = ItemUse_SUBGAME_WAR (pChar, (PCProcSubGameWAR)pProc, pszMsg);
			break;
		}
		if (nProcTmp == 1) {
			nProc = nProcTmp;
		}
		if ((nProcTmp == -1) || (nProcTmp == 99)) {
			nProc = nProcTmp;
			i = 0;
			break;
		}
	}

	if (i >= nCount) {
		nCount = g_aSubaGameInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			nProcTmp = -1;
			pInfo = g_aSubaGameInfo[i];

			/* 一致した？ */
			if (pInfo->byType == pUseInfo->byType) {
				switch (pInfo->byType) {
				case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
					nProcTmp = ItemUse_SUBGAME_BOMB (pChar, (BYTE)i, pszMsg);
					break;
				case SUBGAMEID_WAR:			/* 戦場ゲーム */
					nProcTmp = ItemUse_SUBGAME_WARold (pChar, (BYTE)i, pszMsg);
					break;
				case SUBGAMEID_SUGOROKU:	/* すごろく */
					nProcTmp = ItemUse_SUBGAME_SUGOROKU (pChar, (BYTE)i, pszMsg);
					break;
				}
				if (nProcTmp == 1) {
					nProc = nProcTmp;
				}
				if ((nProcTmp == -1) || (nProcTmp == 99)) {
					nProc = nProcTmp;
					break;
				}
			}
		}
	}

	switch (nProc) {
	case -1:
		goto Exit;
	case 0:
		strcpy (pszMsg, "まだ使える所がないようです。@2");
		goto Exit;
	case 1:
		strcpy (pszMsg, "ここは受け付け場所ではないようです。@2");
		goto Exit;
	case 99:	/* 正常処理 */
		break;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SUBGAME_BOMB												 */
/* 内容：アイテム処理(サブゲーム参加：爆弾ゲーム)							 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static int ItemUse_SUBGAME_BOMB(PCHARINFO pChar, BYTE byNo, LPSTR pszMsg)
{
	int nRet;
	PSUBGAMEINFO pInfo;
	PSUBGAME_BOMBINFO pBomb;
	PSUBSTATUS_BOMB pSubStatus;

	nRet = -1;
	pInfo = g_aSubaGameInfo[byNo];
	pBomb = (PSUBGAME_BOMBINFO)pInfo->pInfo;
	pSubStatus = (PSUBSTATUS_BOMB)pChar->abySubStatus;

	if (pChar->dwMapID != pBomb->dwMapID) {
		nRet = 1;
		goto Exit;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	if ((pChar->dwX != pBomb->dwX) || (pChar->dwY != pBomb->dwY)) {
		nRet = 1;
		goto Exit;
	}

	/* 途中参加禁止？ */
	if (pBomb->byFlgProgress) {
		if (pBomb->byFlgGameNow) {
			strcpy (pszMsg, "この試合への途中参加は禁止されています。@2");
			goto Exit;
		}
	}

	/* ゲームマスター？ */
	if (pSubStatus->byFlgGameMaster) {
		/* 別のゲームに入ろうとしている？ */
		if (pSubStatus->byID != byNo) {
			strcpy (pszMsg, "すでに別の試合に参加しています。@2");
			goto Exit;
		}
	}

	CharTargetCancelPC (pChar);
	pChar->byDirection = 1;
	pChar->byDirectionBack2 = pChar->byDirection;
	pChar->byDirectionBack = pChar->byDirection;
	pChar->dwX = pBomb->dwStartX;
	pChar->dwY = pBomb->dwStartY;
	pChar->dwXBack = pChar->dwX;
	pChar->dwYBack = pChar->dwY;
	SendCmdFADEINOUT (pChar->pID, 0);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 1, 2);
	SendCmdCHARPOS (pChar->pID, pChar->dwTblID, 0, 0);
	SendCmdCHARINFO (pChar->pID, -1);

	/* 爆弾ゲームに参加 */
	if (pSubStatus->byFlgGameMaster == 0) {
		ZeroMemory (pChar->abySubStatus, sizeof (pChar->abySubStatus));
	}

	pSubStatus->byID = byNo;
	/* 試合中？ */
	if (pBomb->byFlgGameNow) {
		pSubStatus->byFlgEntry = 1;
		pSubStatus->byCount = 0;
	}

	pSubStatus->byType = SUBGAMEID_BOMB;
	pSubStatus->byMaxCount = 1;
	pSubStatus->byPower = 3;

	/* アイテム無し？ */
	if (pBomb->byFlgUseItem == 0) {
		pSubStatus->byMaxCount = pBomb->byMaxBomb;
		pSubStatus->byPower = pBomb->byMaxFire;
	}

	/* ゲームマスター不在？ */
	if (pBomb->dwTblIDGameMaster == -1) {
		/* ゲーム情報を初期化 */
		pBomb->byFlgGameNow = 0;
		pBomb->byFlgProgress = 0;
		pBomb->byFlgUseBlock = 0;
		pBomb->byFlgUseItem = 0;
		pBomb->byGameTime = 60;
		pBomb->byGameTimeNow = pBomb->byGameTime;
		pBomb->byMaxBomb = 1;
		pBomb->byMaxFire = 3;
		pBomb->dwLastTime = 0;
		pBomb->dwTblIDGameMaster = pChar->dwTblID;
		pSubStatus->byFlgGameMaster = 1;
	}

	SendCmdSUBSTATUS (pChar->dwTblID);
	SendCmdSubBombTime (byNo, pBomb);
	SendCmdSubBOMBINFO (pChar->pID, byNo, pBomb);
	SendCmdFADEINOUT (pChar->pID, 1);

	nRet = 99;
Exit:
	return nRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SUBGAME_WARold											 */
/* 内容：アイテム処理(サブゲーム参加：戦場ゲーム)							 */
/* 日付：2004/08/10															 */
/* ========================================================================= */
static int ItemUse_SUBGAME_WARold(PCHARINFO pChar, BYTE byNo, LPSTR pszMsg)
{
	int nRet;
	BOOL bCheck;
	PSUBGAMEINFO pInfo;
	PSUBGAME_WARINFO pWar;
	PSUBSTATUS_WAR pSubStatus;

	nRet = -1;
	pInfo = g_aSubaGameInfo[byNo];
	pWar = (PSUBGAME_WARINFO)pInfo->pInfo;
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* マップが違う？ */
	if (pChar->dwMapID != pWar->dwMapID) {
		nRet = 1;
		goto Exit;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* 受付位置ではない？ */
	if ((pChar->dwX != (DWORD)pWar->byX) || (pChar->dwY != (DWORD)pWar->byY)) {
		nRet = 1;
		goto Exit;
	}

	/* 受付状態ではない？ */
	if ((pWar->byState != SUBGAME_WAR_STATE_READY) && (pWar->byState != SUBGAME_WAR_STATE_JOIN)) {
		strcpy (pszMsg, "すでに参加は締め切られています。@2");
		goto Exit;
	}

	bCheck = FALSE;
	switch (pWar->byMember) {
	case SUBGAME_WAR_MEMBER_3:
		if (pWar->byAllMember >= 6) {
			bCheck = TRUE;
		}
		break;
	case SUBGAME_WAR_MEMBER_4:
		if (pWar->byAllMember >= 8) {
			bCheck = TRUE;
		}
		break;
	}
	/* 定員オーバー？ */
	if (bCheck) {
		strcpy (pszMsg, "満員の為参加できません。@2");
		goto Exit;
	}

	/* どちらのチームに入るか送信 */
	ZeroMemory (pSubStatus, sizeof (SUBSTATUS_WAR));
	pSubStatus->byType = SUBGAMEID_WAR;
	pSubStatus->byID = byNo;
	SendCmdSUBSTATUS (pChar->dwTblID);

	pSubStatus->byState = SUBGAME_WAR_STATE_SELECTTEAM;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	strcpy (pszMsg, "戦場ゲームに参加します。@1コマンドを開いて参加チームを選択してください。@2");

	nRet = 99;
Exit:
	return nRet;
}


/* ========================================================================= */
/* 関数名：	ItemUse_SUBGAME_WAR												 */
/* 内容：	アイテム処理(サブゲーム参加：戦場ゲーム)						 */
/* 日付：	2004/08/10														 */
/* ========================================================================= */

static int ItemUse_SUBGAME_WAR(PCHARINFO pChar, CProcSubGameWAR *pProc, LPSTR pszMsg)
{
	int nRet, nState, nMember, nCount;
	BOOL bCheck, bResult;
	PSUBSTATUS_WAR pSubStatus;

	nRet = -1;
	pSubStatus = (PSUBSTATUS_WAR)pChar->abySubStatus;

	/* 受付位置ではない？ */
	bResult = pProc->IsInformationPos (pChar->dwMapID, (BYTE)pChar->dwX, (BYTE)pChar->dwY);
	if (bResult == FALSE) {
		nRet = 1;
		goto Exit;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	nState = pProc->GetState ();
	/* 受付状態ではない？ */
	if ((nState != SUBGAME_WAR_STATE_READY) && (nState != SUBGAME_WAR_STATE_JOIN)) {
		strcpy (pszMsg, "すでに参加は締め切られています。@2");
		goto Exit;
	}

	bCheck	= FALSE;
	/* GetMemberType は1チームの人数(3か4)を返すので、2チーム分で満員か判定する
	   （以前は設定値 SUBGAME_WAR_MEMBER_3/4 = 0/1 と比べていて、判定が効いていなかった） */
	nMember	= pProc->GetMemberType ();
	nCount	= pProc->GetAllMemberCount ();
	if (nCount >= nMember * 2) {
		bCheck = TRUE;
	}
	/* 定員オーバー？ */
	if (bCheck) {
		strcpy (pszMsg, "満員の為参加できません。@2");
		goto Exit;
	}

	/* どちらのチームに入るか送信 */
	ZeroMemory (pSubStatus, sizeof (SUBSTATUS_WAR));
	pSubStatus->byType	= SUBGAMEID_WAR;
	pSubStatus->byID	= pProc->GetID ();
	SendCmdSUBSTATUS (pChar->dwTblID);

	pSubStatus->byState = SUBGAME_WAR_STATE_SELECTTEAM;
	SendCmdSubWarCHGSTATE (pChar->pID, pSubStatus->byState);

	strcpy (pszMsg, "戦場ゲームに参加します。@1コマンドを開いて参加チームを選択してください。@2");

	nRet = 99;
Exit:
	return nRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SUBGAME_SUGOROKU											 */
/* 内容：アイテム処理(サブゲーム参加：すごろく)								 */
/* 日付：2004/12/23															 */
/* ========================================================================= */
static int ItemUse_SUBGAME_SUGOROKU(PCHARINFO pChar, BYTE byNo, LPSTR pszMsg)
{
	int nRet;
	BOOL bCheck;
	DWORD dwX, dwY;
	PSUBGAMEINFO pInfo;
	PSUBGAME_SUGOROKUINFO pSugoroku;
	PSUBSTATUS_SUGOROKU pSubStatus;
//nRet = 0;
//goto Exit;

	nRet = -1;
	pInfo = g_aSubaGameInfo[byNo];
	pSugoroku = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
	if (pSugoroku == NULL) {
		nRet = 1;
		goto Exit;
	}
	if (pChar->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}
	pSubStatus = (PSUBSTATUS_SUGOROKU)pChar->abySubStatus;

	/* マップが違う？ */
	if (pChar->dwMapID != pSugoroku->dwMapID) {
		nRet = 1;
		goto Exit;
	}

	/* 受付位置ではない？ */
	if ((pChar->dwX != (DWORD)pSugoroku->byX) || (pChar->dwY != (DWORD)pSugoroku->byY)) {
		nRet = 1;
		goto Exit;
	}

	dwX = dwY = 0;
	SubSugoroku_GetStartPos (pSugoroku, &dwX, &dwY);
	/* スタート位置が未設定？ */
	if (dwX == 0 || dwY == 0) {
		nRet = 0;
		goto Exit;
	}
	bCheck = FALSE;

	/* 参加モードの選択を通知 */
	ZeroMemory (pSubStatus, sizeof (SUBSTATUS_SUGOROKU));
	pSubStatus->byType = SUBGAMEID_SUGOROKU;
	pSubStatus->byID = byNo;
	SendCmdSUBSTATUS (pChar->dwTblID);

	pSubStatus->byState = SUBGAME_SUGOROKU_STATE_JOINMODE;
	SendCmdSubSugorokuCHGSTATE (pChar->pID, pSubStatus->byState);

	strcpy (pszMsg, "すごろくに参加します。@1コマンドを開いて参加モードを選択してください。@2");

	nRet = 99;
Exit:
	return nRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_SAKE														 */
/* 内容：アイテム処理(酒)													 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_SAKE(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;
	BYTE byDrunkLVBack;
	char chPoint, szTmp[256];
	PITEMINFO_SAKE pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_SAKE)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	if (pCharTarget->byDrunkLV >= (CHARSTATEDRUNK_MAX - 1)) {
		if (pInfo->chPoint > 0) {
			strcpy (pszMsg, "もう限界のようです。@2");
			goto Exit;
		}
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	chPoint = pInfo->chPoint;
	byDrunkLVBack = pCharTarget->byDrunkLV;
	/* 酔い覚まし？ */
	if (pCharTarget->byDrunkPoint + pInfo->chPoint < 0) {
		chPoint = pCharTarget->byDrunkPoint * -1;
	}

	/* 酔いポイントと酔いレベルを更新 */
	pCharTarget->byDrunkPoint += chPoint;
	pCharTarget->byDrunkPoint = min (pCharTarget->byDrunkPoint, (CHARSTATEDRUNK_MAX - 1) * 10);
	pCharTarget->byDrunkLV = pCharTarget->byDrunkPoint / 10;

	/* 結果メッセージの設定 */
	if (pInfo->chPoint > 0) {
		wsprintf (pszMsg, "「ごくっごくっ。」@1%sは%sを飲みほしました。",
				pCharTarget->szName, pItem->szName);
	} else {
		wsprintf (pszMsg, "%sは%sを飲みました。",
				pCharTarget->szName, pItem->szName);
	}

	/* 酔っている？ */
	if (pCharTarget->byDrunkLV > CHARSTATEDRUNK_NONE) {
		pCharTarget->dwLastSake = timeGetTime ();
		pCharTarget->dwLastSakeHikku = g_dwTime + 15000 + ((genrand () % 6) * 1000);
	}

	/* レベルアップ？ */
	if (byDrunkLVBack != pCharTarget->byDrunkLV) {
		if (pInfo->chPoint > 0) {
			wsprintf (szTmp, "@0「ひっく。」@1%s状態になりました。", aDrunkTypeTbl[pCharTarget->byDrunkLV]);
			pChar->dwLastSakeHikku = 0;

		} else {
			wsprintf (szTmp, "@0%s状態になりました。", aDrunkTypeTbl[pCharTarget->byDrunkLV]);
		}

		strcat (pszMsg, szTmp);
		if (pCharTarget->byDrunkLV == CHARSTATEDRUNK_NONE) {
			pChar->dwLastSake = 0;
			pChar->dwLastSakeHikku = 0;
		}

	} else if (pInfo->chPoint < 0) {
		strcat (pszMsg, "@0のどがうるおいました。");
	}

	strcat (pszMsg, "@2");

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_MSG														 */
/* 内容：アイテム処理(メッセージ表示)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_MSG(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;
	char szTmp[256];
	PITEMINFO_MSG pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_MSG)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* 自分に使う？ */
	if (pChar == pCharTarget) {
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pCharTarget);

	if (pInfo->pMsgTo) {
		wsprintf (szTmp, "%sに%s", pCharTarget->szName, pInfo->pMsgTo);
		if (pInfo->byType) {
			strcpy (pszMsg, szTmp);
		} else {
			SendCmdSYSMSG (pChar->pID, szTmp, SPEECHCMD_BATTLE1);
		}
	}

	if (pInfo->pMsgMe) {
		wsprintf (szTmp, "%sに%s", pChar->szName, pInfo->pMsgMe);
		if (pInfo->byType) {
			SendCmdMAPMSG (pCharTarget->pID, szTmp);

		} else {
			SendCmdSYSMSG (pCharTarget->pID, szTmp, SPEECHCMD_BATTLE1);
		}
	}

	/* 文字エフェクトあり？ */
	if (strlen (pInfo->szCharEfc)) {
		SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (255, 255, 255), pInfo->szCharEfc);
	}

	/* エフェクト番号あり？ */
	if (pInfo->byEfectID) {
		if (pInfo->byEfectID <= g_EfcInfo.GetCount ()) {
			pCharTarget->byEfect = pInfo->byEfectID;
			SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
			pCharTarget->byEfect = 0;
		}
	}

	if (strlen (pszMsg) == 0) {
		pszMsg[0] = -1;
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_GRPCHG													 */
/* 内容：アイテム処理(キャラ画像変更)										 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_GRPCHG(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;
	PITEMINFO_GRPCHG pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_GRPCHG)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	if (((pInfo->wGrpID == 0) && (pChar->wGrpID == pChar->wGrpIDBack)) ||
		((pInfo->wGrpID) && (pChar->wGrpID == pInfo->wGrpID))) {
		strcpy (pszMsg, "すでにこの衣装に着替えています。@2");
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	/* お気に入りが未設定？ */
	if (pChar->wGrpIDBack == 0) {
		pChar->wGrpIDBack = pChar->wGrpID;
	}

	/* とりあえずメイン番号を0にする */
	pChar->wGrpMain = 0;
	if (pInfo->wGrpID == 0) {
		pChar->wGrpID = pChar->wGrpIDBack;
	} else {
		pChar->wGrpID = pInfo->wGrpID;
	}
	SendCmdCHARINFO3 (pChar->dwTblID);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_LIGHT													 */
/* 内容：アイテム処理(明かり)												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_LIGHT(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	BOOL bRet;
	PMAPINFO pMap;
	PITEMINFO_LIGHT pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_LIGHT)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	pMap = g_aMapInfo[pChar->dwMapID];

	if (pMap->byLightLevel == 0) {
		strcpy (pszMsg, "ここで使う必要はありません。@2");
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	pChar->byLightLevel		= pInfo->byLevel;	/* 明るさレベル */
	pChar->byLightLevelBack = pInfo->byLevel;
	pChar->wLightTime		= pInfo->wTime;		/* 維持時間 */

	if (pChar->wLightTime == 0) {
		pChar->dwTimeLight = 0;					/* 明かりつけた時間 */

	} else {
		pChar->dwTimeLight = timeGetTime ();	/* 明かりつけた時間 */
	}

	SendCmdCHARINFO3 (pChar->dwTblID);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_HPDOWN													 */
/* 内容：アイテム処理(HP減少)												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_HPDOWN(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int nTmp;
	BOOL bRet;
	WORD wTmp;
	char szTmp[128], szTmp2[16];
	PITEMINFO_HPDOWN pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_HPDOWN)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	switch (pCharTarget->byNpcType) {
	case NPCTYPE_BATTLE1:
	case NPCTYPE_BATTLE2:
		break;
	default:
		strcpy (pszMsg, "使えない相手です。@2");
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	/* 確立で減少？ */
	if (pInfo->byRate) {
		wTmp = (pCharTarget->wMaxHP * 10 * pInfo->wMin / 1000);

	} else {
		nTmp = pInfo->wMax - pInfo->wMin;
		nTmp = max (1, nTmp);
		wTmp = (pInfo->wMin + ((WORD)genrand () % nTmp) + 1);
	}
	wTmp = max (1, wTmp);

	AddBattleDamage (pChar, pCharTarget, (int)wTmp);
	nTmp = pCharTarget->wHP;
	nTmp -= wTmp;
	pCharTarget->wHP = max (nTmp, 0);

	wsprintf (szTmp, "%sに%dのダメージ！", pCharTarget->szName, wTmp);

	wsprintf (szTmp2, "%d", wTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (250, 60, 4), szTmp2);

	SendSysMsg (pChar, pCharTarget, szTmp);

	/* ダメージ後の処理 */
	if (pCharTarget->byNpcType) {
		DamageAfterProcNPC (pChar, pCharTarget);

	} else {
		/* クライアントにステータス更新通知 */
		SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);

		/* エフェクト指定あり？ */
		if (pInfo->byEfectID) {
			pCharTarget->byEfect = pInfo->byEfectID;
			SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
			pCharTarget->byEfect = 0;
		}

		DamageAfterProcPC (pChar, pCharTarget);
	}

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_MAGIC													 */
/* 内容：アイテム処理(魔法習得)												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_MAGIC(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int i;
	BOOL bRet, bResult;
	PITEMINFO_MAGIC pInfo;
	PMAGICINFO pMagicInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_MAGIC)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	if (pInfo->byID >= g_aMagicInfo.GetSize ()) {
		PostMessage (g_hWndMain, WM_DECPLAYER, pChar->pID, (LPARAM)pChar->dwTblID);
		goto Exit;
	}

	pMagicInfo = g_aMagicInfo[pInfo->byID];

	/* 覚えられる職業かチェック */
	bResult = CheckMagicUserInfo (pMagicInfo->byUserInfo, pCharTarget->byJOB);
	if (bResult == FALSE) {
		strcpy (pszMsg, "現在の職業では使えません。@2");
		goto Exit;
	}

	/* すでに覚えているかチェック */
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		if (pCharTarget->abyMagic[i] == pInfo->byID) {
			break;
		}
	}

	/* すでに覚えている？ */
	if (i < MAX_USEMAGIC) {
		strcpy (pszMsg, "既に覚えています。@2");
		goto Exit;
	}

	/* 空きスペースをチェック */
	for (i = 0; i < MAX_USEMAGIC; i ++) {
		if (pCharTarget->abyMagic[i] == 0) {
			break;
		}
	}
	/* 空きがない？ */
	if (i >= MAX_USEMAGIC) {
		strcpy (pszMsg, "これ以上覚えられません。@2");
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	pCharTarget->abyMagic[i] = pInfo->byID;
	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);

	wsprintf (pszMsg, "%sは[%s]を習得しました。@2",
			pCharTarget->szName, pMagicInfo->szName);

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_MANAUP													 */
/* 内容：アイテム処理(Mana回復)												 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_MANAUP(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int nTmp;
	BOOL bRet;
	WORD wTmp;
	char szTmp[128], szTmp2[16];
	PITEMINFO_MANAUP pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_MANAUP)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* すでに満たん？ */
	if (pCharTarget->wMana == pCharTarget->wMaxMana) {
		SendCmdSYSMSG (pChar->pID, "今は使う必要がありません。", SPEECHCMD_BATTLE1);
		*pszMsg = -1;
		goto Exit;
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pChar);

	/* 確立で回復？ */
	if (pInfo->byRate) {
		wTmp = (pCharTarget->wMaxMana * 10 * pInfo->wMin / 1000);

	} else {
		nTmp = pInfo->wMax - pInfo->wMin + 1;
		nTmp = max (1, nTmp);
		wTmp = (pInfo->wMin + ((WORD)genrand () % nTmp));
	}

	if (wTmp + pCharTarget->wMana > pCharTarget->wMaxMana) {
		wTmp = pCharTarget->wMaxMana - pCharTarget->wMana;
	}
	pCharTarget->wMana += wTmp;

	/* 満たんになった？ */
	if (pCharTarget->wMana == pCharTarget->wMaxMana) {
		wsprintf (szTmp, "%sのManaは完全に回復しました。", pCharTarget->szName);
	} else {
		wsprintf (szTmp, "%sのManaが%dポイント回復しました。", pCharTarget->szName, wTmp);
	}
	SendSysMsg (pChar, pCharTarget, szTmp);

	/* 文字エフェクト発動 */
	wsprintf (szTmp2, "%d", wTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp2);

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);
	pCharTarget->byEfect = EFECTTYPE_HOIMI;
	SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
	pCharTarget->byEfect = 0;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_HPMANAUP													 */
/* 内容：アイテム処理(HP・Mana回復)											 */
/* 日付：2004/07/15															 */
/* ========================================================================= */
static BOOL ItemUse_HPMANAUP(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	int nTmp;
	BOOL bRet, bSend;
	WORD wTmp;
	char szTmp[128], szTmp2[16];
	PITEMINFO_HPMANAUP pInfo;

	bSend = FALSE;
	bRet = FALSE;
	pInfo = (PITEMINFO_HPMANAUP)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}
	if (pCharTarget->byCharState == CHARSTATE_DEAD) {
		goto Exit;
	}

	/* すでに満たん？ */
	if (pCharTarget->wHP == pCharTarget->wMaxHP) {
		if (pCharTarget->wMana == pCharTarget->wMaxMana) {
			SendCmdSYSMSG (pChar->pID, "今は使う必要がありません。", SPEECHCMD_BATTLE1);
			*pszMsg = -1;
			goto Exit;
		}
	}

	/* 効果音再生 */
	PlayItemSound (pItem, pCharTarget);

	if (pCharTarget->wHP < pCharTarget->wMaxHP) {
		/* 確立で回復？ */
		if (pInfo->byRateHP) {
			wTmp = (pCharTarget->wMaxHP * 10 * pInfo->wMinHP / 1000);

		} else {
			nTmp = pInfo->wMaxHP - pInfo->wMinHP + 1;
			nTmp = max (1, nTmp);
			wTmp = (pInfo->wMinHP + ((WORD)genrand () % nTmp));
		}

		if (wTmp + pCharTarget->wHP > pCharTarget->wMaxHP) {
			wTmp = pCharTarget->wMaxHP - pCharTarget->wHP;
		}
		pCharTarget->wHP += wTmp;

		/* 満たんになった？ */
		if (pCharTarget->wHP == pCharTarget->wMaxHP) {
			wsprintf (szTmp, "%sのHPは完全に回復しました。", pCharTarget->szName);
		} else {
			wsprintf (szTmp, "%sのHPが%dポイント回復しました。", pCharTarget->szName, wTmp);
		}

		/* 文字エフェクト発動 */
		wsprintf (szTmp2, "%d", wTmp);
		SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp2);

		SendSysMsg (pChar, pCharTarget, szTmp);
		bSend = TRUE;
	}

	if (pCharTarget->wMana < pCharTarget->wMaxMana) {
		/* 確立で回復？ */
		if (pInfo->byRateMana) {
			wTmp = (pCharTarget->wMaxMana * 10 * pInfo->wMinMana / 1000);

		} else {
			nTmp = pInfo->wMaxMana - pInfo->wMinMana;
			nTmp = max (1, nTmp);
			wTmp = (pInfo->wMinMana + ((WORD)genrand () % nTmp));
		}

		if (wTmp + pCharTarget->wMana > pCharTarget->wMaxMana) {
			wTmp = pCharTarget->wMaxMana - pCharTarget->wMana;
		}
		pCharTarget->wMana += wTmp;

		/* 満たんになった？ */
		if (pCharTarget->wMana == pCharTarget->wMaxMana) {
			wsprintf (szTmp, "%sのManaは完全に回復しました。", pCharTarget->szName);
		} else {
			wsprintf (szTmp, "%sのManaが%dポイント回復しました。", pCharTarget->szName, wTmp);
		}

		/* 文字エフェクト発動 */
		wsprintf (szTmp2, "%d", wTmp);
		SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp2);

		SendSysMsg (pChar, pCharTarget, szTmp);
		bSend = TRUE;
	}

	if (bSend) {
		/* クライアントにステータス更新通知 */
		SendCmdCHARSTATUS (pCharTarget->pID, pCharTarget->dwTblID);
		pCharTarget->byEfect = EFECTTYPE_HOIMI;
		SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
		pCharTarget->byEfect = 0;
	}

	bRet = TRUE;
Exit:
	return bRet;
}


/* ========================================================================= */
/* 関数名：	ItemUse_RESURRECTION											 */
/* 内容：	アイテム処理(蘇生)												 */
/* 日付：	2005/11/03														 */
/* ========================================================================= */

static BOOL ItemUse_RESURRECTION(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	long lTmp;
	BOOL bRet;
	char szTmp[64], szTmp2[16];
	PITEMINFO_RESURRECTION pInfo;

	bRet = FALSE;
	pInfo = (PITEMINFO_RESURRECTION)pItem->pData;

	if (pInfo == NULL) {
		goto Exit;
	}

	/* 死んでいない？ */
	if (pCharTarget->byCharState != CHARSTATE_DEAD) {
		SendCmdSYSMSG (pChar->pID, "今は使う必要がありません。", SPEECHCMD_BATTLE1);
		*pszMsg = -1;
		goto Exit;
	}

	lTmp = pCharTarget->wMaxHP * pInfo->byRate / 100;
	lTmp = max (lTmp, 1);
	if (pCharTarget->wHP + lTmp > pCharTarget->wMaxHP) {
		lTmp = pCharTarget->wMaxHP - pCharTarget->wHP;
	}
	pCharTarget->wHP		+= (WORD)lTmp;
	pCharTarget->byCharState = CHARSTATE_MOVE;

	/* 効果音再生 */
	PlayItemSound (pItem, pCharTarget);

	/* 文字エフェクト発動 */
	wsprintf (szTmp2, "%d", lTmp);
	SendCmdADDCHAREFC (pCharTarget->dwTblID, RGB (139, 213, 255), szTmp2);

	wsprintf (szTmp, "%sは生き返りました。", pCharTarget->szName);

	/* 回復メッセージ表示 */
	SendSysMsg (pChar, pCharTarget, szTmp);

	/* クライアントにステータス更新通知 */
	SendCmdCHARSTATUSHP (URARASOCK_SENDALL, pCharTarget->dwTblID);
	pCharTarget->byEfect = 14;
	SendCmdEFECTCHG (pCharTarget->pID, pCharTarget->dwTblID, pCharTarget->byEfect);
	pCharTarget->byEfect = 0;

	bRet = TRUE;
Exit:
	return bRet;
}

/* ========================================================================= */
/* 関数名：ItemUse_USEMAGIC													 */
/* 内容：アイテム処理(特技発動)。覚えていない特技でも出せて、Manaは減らない */
/*		 範囲や相手は特技の設定どおり(「選択」の特技は使った相手に出る)	 */
/* ========================================================================= */
static BOOL ItemUse_USEMAGIC(PCHARINFO pChar, PCHARINFO pCharTarget, PITEMINFO pItem, LPSTR pszMsg)
{
	PITEMINFO_MAGIC pInfo;

	pInfo = (PITEMINFO_MAGIC)pItem->pData;
	if ((pInfo == NULL) || (pInfo->byID == 0) || (pInfo->byID >= g_aMagicInfo.GetSize ())) {
		return FALSE;
	}
	if ((pChar->byCharState == CHARSTATE_DEAD) || (pChar->wHP == 0)) {
		return FALSE;
	}
	if (IsSleep (pChar)) {
		strcpy (pszMsg, "眠っていて使えません。@2");
		return FALSE;
	}
	if (MagicProcItem (pChar->dwTblID, pInfo->byID, pCharTarget->dwTblID) == FALSE) {
		strcpy (pszMsg, "効果のある相手がいませんでした。@2");
		return FALSE;
	}
	/* 効果音再生 */
	PlayItemSound (pItem, pChar);
	return TRUE;
}

/* Copyright(C)URARA-works 2003-2006 */
