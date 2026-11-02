/* Copyright(C)URARA-works 2006 */
/* ========================================================================= */
/* ファイル名：	ProcSubGameWAR.h											 */
/* 内容：		サブゲーム処理(戦場ゲーム)クラス 定義ファイル				 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2006/03/19													 */
/* ========================================================================= */

#pragma once

#include "ProcSubGameBase.h"

class CInfoSubGameWAR;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */

typedef class CProcSubGameWAR : public CProcSubGameBase
{
public:
			CProcSubGameWAR();										/* コンストラクタ */
	virtual ~CProcSubGameWAR();										/* デストラクタ */

	void	Init					(BYTE byID, CInfoSubGameBase *pInfo);	/* 初期化 */
	void	Proc					(void);									/* 処理 */
	BOOL	IsInformationPos		(DWORD dwMapID, BYTE byX, BYTE byY);	/* 指定座標が受付位置か判定 */
	int		GetType					(void);									/* サブゲーム種別を取得 */

	int		Rand					(void);													/* 乱数発生 */
	int		GetMemberCount			(int nTeam);											/* 参加人数を取得 */
	int		GetAllMemberCount		(void);													/* 全参加人数を取得 */
	int		GetMemberType			(void);													/* 1チームの最大参加人数を取得 */
	int		IsJoin					(DWORD dwTblID);										/* 参加しているかチェック */
	int		GetState				(void);													/* 状態を取得 */
	void	ChgState				(int nState);											/* 状態を変更 */
	BYTE	GetRule					(void);													/* ルールを取得 */
	void	SetRule					(BYTE byRule);											/* ルールを設定 */
	void	GetStartPos				(BYTE byTeam, POINT &ptDst);							/* 開始座標を取得 */
	void	Remove					(DWORD dwTblID);										/* ゲームから離脱 */
	void	RemoveAndMove			(DWORD dwTblID);										/* ゲームから離脱して受付位置に移動 */
	void	AddMember				(BYTE byTeam, DWORD dwTblID);							/* 参加 */
	void	JudgeMsg				(LPCSTR pszMsg);										/* 審判キャラ発言 */
	void	AllChgState				(BYTE byState);											/* 参加者全員の状態変更 */
	void	AllChgStateAndQuit		(BYTE byState, BYTE byCheckState);						/* 参加者全員の状態変更(条件付き) */
	void	UpTurn					(BYTE byTeam, BYTE byNo);								/* 順番を一つ手前にずらす */
	void	PrePare					(void);													/* 準備処理 */
	void	ChgTurn					(void);													/* ターン変更 */
	void	NextTurn				(void);													/* 次のキャラへ行動権限変更 */
	void	HPUp					(DWORD dwTblID, BYTE byPoint);							/* HP回復 */
	void	Atack					(DWORD dwTblIDTarget, BYTE byDifence);					/* 攻撃 */
	void	Damage					(DWORD dwTblIDTarget, BYTE byDamage, BYTE byEfect);		/* ダメージ処理 */
	void	Dead					(DWORD dwTblIDTarget);									/* やられた処理 */
	int		GetDeadPosNo			(BYTE byTeam);											/* 空いている待機場所を取得 */
	void	Resuscitation			(DWORD dwTblID, BOOL bProc);							/* 蘇生判定 */
	void	Quit					(void);													/* 試合終了 */
	void	MakeTargetFire			(PCMDPARA_WARSELECTMAGICTARGET pTarget);				/* 魔法対象者一覧作成(炎) */
	void	MakeTargetQure			(PCMDPARA_WARSELECTMAGICTARGET pTarget);				/* 魔法対象者一覧作成(回復) */
	void	MakeTargetResuscitation	(PCMDPARA_WARSELECTMAGICTARGET pTarget);				/* 魔法対象者一覧作成(蘇生) */
	void	UseMagicFire			(BYTE byNo);											/* 魔法使用(炎) */
	void	UseMagicQure			(BYTE byNo);											/* 魔法使用(回復) */
	void	UseMagicResuscitation	(BYTE byNo);											/* 魔法使用(蘇生) */

	CInfoSubGameWAR	*GetInfo		(void);					/* 戦場ゲーム情報を取得 */


private:
	/* 処理 */
	void	ProcINIT		(void);				/* 初期状態 */
	void	ProcCLEAN		(void);				/* 初期化 */
	void	ProcREADY		(void);				/* 準備完了 */
	void	ProcJOIN		(void);				/* 参加者募集中 */
	void	ProcSELECTRULE	(void);				/* ルール選択 */
	void	ProcSELECTJOB	(void);				/* 職業選択 */
	void	ProcSELECTTURN	(void);				/* 順番設定 */
	void	ProcGAMEPREPARE	(void);				/* 試合準備 */
	void	ProcGAME		(void);				/* 試合中 */
	void	ProcQUIT		(void);				/* 終了 */


private:
	CInfoSubGameWAR		*m_pInfo;
} CProcSubGameWAR, *PCProcSubGameWAR;

/* Copyright(C)URARA-works 2006 */
