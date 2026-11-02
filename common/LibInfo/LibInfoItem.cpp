/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	LibInfoItem.cpp												 */
/* 内容：		アイテム情報ライブラリクラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/06/21													 */
/* ========================================================================= */

#include "StdAfx.h"
#include "../Info/Item/InfoItem.h"
#include "LibInfoItem.h"


/* ========================================================================= */
/* 関数名：	CLibInfoItem::CLibInfoItem										 */
/* 内容：	コンストラクタ													 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

CLibInfoItem::CLibInfoItem()
{
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::~CLibInfoItem										 */
/* 内容：	デストラクタ													 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

CLibInfoItem::~CLibInfoItem()
{
	DeleteAll ();
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetCount											 */
/* 内容：	登録数を取得													 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

int CLibInfoItem::GetCount(void)
{
	return m_aInfoItem.GetSize ();
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetType											 */
/* 内容：	種別を取得														 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

DWORD CLibInfoItem::GetType(DWORD dwItemID)
{
	DWORD dwRet;
	PCInfoItemBase pInfo;

	dwRet = 0;

	pInfo = Get (dwItemID);
	if (pInfo == NULL) {
		goto Exit;
	}

	dwRet = pInfo->GetItemTypeID ();
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::Add												 */
/* 内容：	追加															 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

void CLibInfoItem::Add(CInfoItemBase *pSrc)
{
	DWORD dwItemID;

	dwItemID = pSrc->GetItemID ();
	if (dwItemID == 0) {
		dwItemID = GetNewID ();
		pSrc->SetItemID (dwItemID);
	}

	m_aInfoItem.Add (pSrc);
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::DeleteAll											 */
/* 内容：	アイテム情報を全て削除											 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

void CLibInfoItem::DeleteAll(void)
{
	int i, nCount;
	PCInfoItemBase pInfo;

	nCount = m_aInfoItem.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = m_aInfoItem[i];
		SAFE_DELETE (pInfo);
	}
	m_aInfoItem.RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::Get												 */
/* 内容：	取得															 */
/* 日付：	2006/06/21														 */
/* ========================================================================= */

CInfoItemBase *CLibInfoItem::Get(DWORD dwItemID)
{
	int i, nCount;
	PCInfoItemBase pRet, pInfo;

	pRet = NULL;

	nCount = m_aInfoItem.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = m_aInfoItem[i];
		if (pInfo->GetItemID () == dwItemID) {
			pRet = pInfo;
			break;
		}
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::Get												 */
/* 内容：	取得															 */
/* 日付：	2006/06/26														 */
/* ========================================================================= */

CInfoItemBase *CLibInfoItem::Get(int nNo)
{
	int nCount;
	PCInfoItemBase pRet;

	pRet = NULL;

	nCount = m_aInfoItem.GetSize ();
	if (nNo >= nCount) {
		goto Exit;
	}

	pRet = m_aInfoItem[nNo];
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetNew											 */
/* 内容：	指定種別の新規データを取得										 */
/* 日付：	2006/07/12														 */
/* ========================================================================= */

CInfoItemBase *CLibInfoItem::GetNew(DWORD dwItemTypeID)
{
	CInfoItemBase *pRet;

	pRet = NULL;

	switch (dwItemTypeID) {
	case ITEMTYPE_NONE:					/* 効果無し */
		pRet = new CInfoItemBase;
		break;
	case ITEMTYPE_RURA:					/* 特定位置へ移動 */
		pRet = new CInfoItemMOVE;
		break;
	case ITEMTYPE_HPUP:					/* HP回復 */
		pRet = new CInfoItemHPUP;
		break;
	case ITEMTYPE_WEAPON:				/* 武器 */
		pRet = new CInfoItemWEAPON;
		break;
	case ITEMTYPE_ARMOR:				/* 鎧 */
		pRet = new CInfoItemARMOR;
		break;
	case ITEMTYPE_SHILD:				/* 盾 */
		pRet = new CInfoItemSHILD;
		break;
	case ITEMTYPE_HELMET:				/* 兜 */
		pRet = new CInfoItemHELMET;
		break;
	case ITEMTYPE_SEED:					/* 種 */
		pRet = new CInfoItemSEED;
		break;
	case ITEMTYPE_SUBGAME:				/* サブゲーム参加 */
		pRet = new CInfoItemSUBGAME;
		break;
	case ITEMTYPE_SAKE:					/* 酒 */
		pRet = new CInfoItemSAKE;
		break;
	case ITEMTYPE_MSG:					/* メッセージ表示 */
		pRet = new CInfoItemMSG;
		break;
	case ITEMTYPE_GRPCHG:				/* キャラ画像変更 */
		pRet = new CInfoItemGRPCHG;
		break;
	case ITEMTYPE_LIGHT:				/* 明かり */
		pRet = new CInfoItemLIGHT;
		break;
	case ITEMTYPE_HPDOWN:				/* HP減少 */
		pRet = new CInfoItemHPDOWN;
		break;
	case ITEMTYPE_MAGIC:				/* 魔法習得 */
		pRet = new CInfoItemSKILL;
		break;
	case ITEMTYPE_MANAUP:				/* Mana回復 */
		pRet = new CInfoItemMANAUP;
		break;
	case ITEMTYPE_HPMANAUP:				/* HP・Mana回復 */
		pRet = new CInfoItemHPMANAUP;
		break;
	case ITEMTYPE_EQU_HAND:				/* 装備(小手) */
		pRet = new CInfoItemEQU_HAND;
		break;
	case ITEMTYPE_EQU_LEG:				/* 装備(靴) */
		pRet = new CInfoItemEQU_LEG;
		break;
	case ITEMTYPE_EQU_ACC1:				/* 装備(首飾り) */
		pRet = new CInfoItemEQU_ACC1;
		break;
	case ITEMTYPE_EQU_ACC2:				/* 装備(指輪1) */
		pRet = new CInfoItemEQU_ACC2;
		break;
	case ITEMTYPE_EQU_ACC3:				/* 装備(指輪2) */
		pRet = new CInfoItemEQU_ACC3;
		break;
	case ITEMTYPE_RESURRECTION:			/* 蘇生 */
		pRet = new CInfoItemRESURRECTION;
		break;
	}

	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetDataSize										 */
/* 内容：	送信用データサイズを取得										 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

DWORD CLibInfoItem::GetDataSize(DWORD dwItemID)
{
	DWORD dwRet;
	CInfoItemBase *pInfoItemBase;

	dwRet = 0;

	pInfoItemBase = Get (dwItemID);
	if (pInfoItemBase == NULL) {
		goto Exit;
	}

	dwRet = pInfoItemBase->GetDataSize ();
Exit:
	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetData											 */
/* 内容：	データを取得													 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

PBYTE CLibInfoItem::GetData(DWORD dwItemID)
{
	PBYTE pRet;
	CInfoItemBase *pInfoItemBase;

	pRet = NULL;

	pInfoItemBase = Get (dwItemID);
	if (pInfoItemBase == NULL) {
		goto Exit;
	}

	pRet = pInfoItemBase->GetData ();
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::SetData											 */
/* 内容：	データから設定													 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

PBYTE CLibInfoItem::SetData(DWORD dwItemID, PBYTE pSrc)
{
	PBYTE pRet;
	DWORD dwItemTypeID;
	CInfoItemBase *pInfoItemBase, *pInfoItemBaseTmp;

	pRet = NULL;
	pInfoItemBaseTmp = NULL;

	if (dwItemID != 0) {
		pInfoItemBase = Get (dwItemID);
	}
	if (pInfoItemBase == NULL) {
		/* 種別を調べるために一旦基底用として読み込む */
		pInfoItemBaseTmp = new CInfoItemBase;
		pInfoItemBaseTmp->SetData (pSrc);

		dwItemTypeID	= pInfoItemBaseTmp->GetItemTypeID ();
		pInfoItemBase	= GetNew (dwItemTypeID);
		Add (pInfoItemBase);
	}

	pRet = pInfoItemBase->SetData (pSrc);

	SAFE_DELETE (pInfoItemBaseTmp);
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetAllDataSize									 */
/* 内容：	全データサイズを取得											 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

DWORD CLibInfoItem::GetAllDataSize(void)
{
	int i, nCount;
	DWORD dwRet;
	PCInfoItemBase pInfoItemBase;

	dwRet = 0;

	nCount = GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfoItemBase = Get (i);
		dwRet += pInfoItemBase->GetDataSize ();
	}

	return dwRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetAllData										 */
/* 内容：	全データを取得													 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

PBYTE CLibInfoItem::GetAllData(void)
{
	int i, nCount;
	DWORD dwSize, dwPos;
	PBYTE pRet, pAllData, pTmp;
	PCInfoItemBase pInfoItemBase;

	pRet = NULL;

	dwSize = GetAllDataSize ();
	if (dwSize == 0) {
		goto Exit;
	}

	pAllData	= new BYTE[dwSize];
	dwPos		= 0;

	nCount = GetCount ();
	for (i = 0; i < nCount; i ++) {
		pInfoItemBase = Get (i);
		dwSize	= pInfoItemBase->GetDataSize ();
		pTmp	= pInfoItemBase->GetData ();

		CopyMemory (&pAllData[dwPos], pTmp, dwSize);
		dwPos += dwSize;
		SAFE_DELETE_ARRAY (pTmp);
	}

	pRet = pAllData;
Exit:
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::SetAllData										 */
/* 内容：	全データを取得													 */
/* 日付：	2006/07/13														 */
/* ========================================================================= */

PBYTE CLibInfoItem::SetAllData(int nCount, PBYTE pSrc)
{
	int i;
	PBYTE pRet, pTmp;

	pRet = NULL;
	pTmp = pSrc;

	DeleteAll ();

	for (i = 0; i < nCount; i ++) {
		pTmp = SetData (0, pTmp);
	}

	pRet = pTmp;
	return pRet;
}


/* ========================================================================= */
/* 関数名：	CLibInfoItem::GetNewID											 */
/* 内容：	追加用の新しいIDを取得											 */
/* 日付：	2006/06/26														 */
/* ========================================================================= */

DWORD CLibInfoItem::GetNewID(void)
{
	int i, nCount;
	DWORD dwRet, dwTmp;
	PCInfoItemBase pInfo;

	dwRet = 1;
	dwTmp = 1;

	nCount = m_aInfoItem.GetSize ();
	for (i = 0; i < nCount; i ++) {
		pInfo = m_aInfoItem[i];
		if (pInfo->GetItemID () == dwTmp) {
			dwTmp ++;
			i = -1;
		}
	}

	dwRet = dwTmp;
	return dwRet;
}

/* Copyright(C)URARA-works 2006 */
