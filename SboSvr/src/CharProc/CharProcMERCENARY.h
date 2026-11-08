/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* ファイル名：	CharProcMERCENARY.h											 */
/* 内容：		キャラ処理(傭兵)クラス 定義ファイル							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2026/11/08													 */
/* ========================================================================= */

#pragma once

#include "CharProcBase.h"

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CCharProcMERCENARY : public CCharProcBase
{
public:
	CCharProcMERCENARY();								/* コンストラクタ */

	BOOL Proc		(DWORD dwTime);								/* 処理 */

protected:
	BOOL	ProcFollow		(PCHARINFO pOwner, DWORD dwTime);		/* 雇い主についていく */
	BOOL	ProcBattle		(PCHARINFO pOwner, DWORD dwTime);		/* 近くの敵と戦う */
	DWORD	FindEnemy		(PCHARINFO pOwner);						/* 戦う相手を探す */
	BOOL	IsEnemyValid	(DWORD dwTblID);						/* 戦える敵か */
	BOOL	IsFront			(PCHARINFO pCharTmp);					/* 目の前にいるか */
	BOOL	CanWalk			(int x, int y, BOOL bDoor = FALSE);	/* 地形として歩けるマスか(人は見ない。bDoor=閉まった扉も通れる扱い) */
	BOOL	OpenDoor		(int x, int y);							/* そのマスの閉まった扉を開ける */
	void	FinishWarp		(PCHARINFO pOwner, DWORD dwTime);		/* 消えてから雇い主のとなりに出る */
	BOOL	CanStep			(int x, int y);							/* そのマスへ1歩進めるか */
	int		PathDir			(int nX, int nY, int nMaxDist, int *pnDist = NULL, BOOL bChars = TRUE);	/* 目標のとなりへ行く道の最初の向き */
	BOOL	StepToward		(int nX, int nY);						/* 目標のマスへ1歩近づく */
	BOOL	StepDirection	(int nDirection);						/* 指定の向きへ1歩進む */
	BOOL	WarpNearOwner	(PCHARINFO pOwner);						/* 雇い主のとなりへ移る */
	void	SyncParty		(PCHARINFO pOwner, DWORD dwTime);		/* 仲間の一覧とHP・Manaを知らせる */
	BOOL	IsInTail		(PCHARINFO pOwner);						/* 雇い主の「ついていく」の列に入っているか */
	PCHARINFO GetTailEnd	(PCHARINFO pOwner);						/* 雇い主の列のいちばん後ろ */

protected:
	DWORD	m_dwOwnerX;									/* 前に見た雇い主の位置(横) */
	DWORD	m_dwOwnerY;									/* 前に見た雇い主の位置(縦) */
	DWORD	m_dwEnemy;									/* 戦っている敵 */
	DWORD	m_dwLastSearch;								/* 最後に敵を探した時間 */
	DWORD	m_dwNoEnemyTime;							/* 敵が見つからなくなった時間 */
	DWORD	m_dwLastAtack;								/* 最後に攻撃した時間 */
	DWORD	m_dwStuckTime;								/* 雇い主に近づけなくなった時間 */
	DWORD	m_dwWarpTime;								/* 雇い主のとなりへ移るため消え始めた時間(0=移動中でない) */
	DWORD	m_dwLastOwnerPath;							/* 戦闘中に雇い主まで行けるか最後に調べた時間 */
	DWORD	m_dwBattleProgress;							/* 戦いで最後に殴った・近づけた時間 */
	DWORD	m_dwIgnoreEnemy;							/* しばらく相手にしない敵(近づけなかった) */
	DWORD	m_dwIgnoreUntil;							/* その敵を相手にしない時間の終わり */
	DWORD	m_dwDeadTime;								/* 倒れた時間 */
	DWORD	m_dwLastSync;								/* 最後に仲間へ知らせた時間 */
	WORD	m_wSyncHP;									/* 最後に知らせたHP */
	WORD	m_wSyncMana;								/* 最後に知らせたMana */
	BYTE	m_bySyncState;								/* 最後に知らせた状態 */
} CCharProcMERCENARY, *PCCharProcMERCENARY;


/* ========================================================================= */
/* 傭兵の管理（雇い主1人につき1人まで）										 */
/* ========================================================================= */

extern BOOL			IsMercenary			(PCHARINFO pChar);						/* 傭兵か */
extern PCHARINFO	GetMercenaryOwner	(PCHARINFO pMerc);						/* 傭兵の雇い主(いなければ NULL) */
extern PCHARINFO	GetMercenaryFront	(PCHARINFO pChar);						/* 目の前にいる生きた傭兵(いなければ NULL) */
extern void			MercenaryTalk		(PCHARINFO pChar, PCHARINFO pCharNpc);	/* 傭兵所のNPCに話しかけた(雇う・別れる) */
extern BOOL			MercenaryDamaged	(PCHARINFO pChar, PCHARINFO pMerc);		/* 傭兵が攻撃を受けた後の処理(倒れたら TRUE) */
extern BOOL			MercenaryItemCheck	(PCHARINFO pMerc, BYTE byItemType);		/* その種類の道具を傭兵に使えるか */
extern void			MercenaryTailOff	(PCHARINFO pMerc);						/* 傭兵を「ついていく」の列から外す */
extern void			MercenaryTailOffBack(PCHARINFO pChar);						/* すぐ後ろに並んでいる傭兵を列から外す */
extern void			MercenarySetBattleMode(PCHARINFO pChar, BOOL bBattle);		/* 雇い主の戦闘モードを覚える */

/* Copyright(C)URARA-works 2026 */
