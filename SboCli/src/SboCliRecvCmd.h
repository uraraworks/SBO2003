/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* ファイル名：	SboCliRecvCmd.cpp											 */
/* 内容：		コマンド受信処理の外部参照用定義							 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2003/04/29													 */
/* ========================================================================= */

#pragma once

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern int	GetCharNo				(DWORD);													/* キャラ番号取得 */
extern int	GetCharNo2				(DWORD);													/* キャラ番号取得2 */
extern int	GetEventDataSize		(BYTE);														/* イベント種別毎のデータサイズを取得 */
extern int	GetSubGameDataSize		(BYTE);														/* サブゲーム種別毎のデータサイズを取得 */
extern int	GetItemDataSize			(BYTE);														/* アイテム種別毎のデータサイズを取得 */
extern int	GetItemCount			(DWORD);													/* アイテム数を取得 */
extern int	GetItemCountType		(DWORD, BYTE);												/* 種類指定でアイテム数を取得 */
extern int	GetItemCountTypeEqu		(DWORD, BYTE);												/* 種類指定でアイテム数を取得(装備不可は除く) */
extern int	GetItemCountTypeID		(DWORD, BYTE, BYTE);										/* 種類指定でアイテムIDを取得 */
extern int	GetItemCountTypeIDEqu	(DWORD, BYTE, BYTE);										/* 種類指定でアイテムIDを取得(装備不可は除く) */
extern int	GetItemCountTypeNo		(DWORD, BYTE, BYTE);										/* 種類指定でアイテム番号を取得 */
extern int	GetItemCountTypeNoEqu	(DWORD, BYTE, BYTE);										/* 種類指定でアイテム番号を取得(装備不可は除く) */
extern int	GetCharDistance			(PCHARINFO pCharTarget, DWORD dwMapID, int x, int y, BYTE byDirection);		/* 指定キャラとの距離を求める */
extern void	RenewATDF				(DWORD);													/* 攻撃力・守備力の更新 */
extern void	RenewCharPosInfo		(void);														/* キャラ座標情報更新 */
extern void	RenewUseMagicInfo		(void);														/* 使用できる魔法一覧更新 */
extern void	RenewMapDark			(PCHARINFO pChar);											/* 暗闇描画情報を更新 */
extern BOOL	CheckEquUserInfo		(WORD, BYTE);												/* 装備職業情報にあるか判定 */
extern BOOL	CheckItemSexDistinction	(PITEMINFO, PCHARINFO);										/* 性別による使えるかチェック */
extern int	CheckCharBlockPC		(DWORD, DWORD);												/* キャラのキャラとのぶつかり判定 */
extern BOOL	CheckBlockEventFront	(PCHARINFO pChar, int x, int y, BYTE byDirection);			/* 1歩前にイベントがあるか判定 */
extern BOOL	IsBlock					(int x, int y, BOOL bHide);									/* ぶつかり判定 */
extern BOOL	CheckCharX				(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget);				/* キャラサイズに合わせた当たり判定(横) */
extern int	GetCharCenterX			(PCHARINFO pChar);											/* キャラの中心位置を取得 */
extern BOOL	CheckTargetPos			(int nXFrom, int nYFrom, int nXTo, int nYTo, int nCount);	/* 指定座標までたどり着けるかチェック */
extern int	GetUseCharInfo			(int nArea, PCHARINFO pMyChar, BOOL bSelf, int nType, ARRAYUSECHARINFO &aDst, int &nLen);	/* 誰に使うか一覧を取得 */
extern void	DeleteUseCharInfo		(PARRAYUSECHARINFO pSrc);									/* 誰に使うか一覧を削除 */
extern int	GetAttackType			(PCHARINFO pChar);											/* 攻撃種別を取得 */
extern void	StartTurnGage			(PCHARINFO pChar, DWORD dwAtackTblID);						/* ターンゲージ開始 */
extern void	CancelTurnGage			(PCHARINFO pChar);											/* ターンゲージをキャンセル */
extern int	FindNextAttackTarget	(PCHARINFO pChar);											/* 連続攻撃の次の相手を探す */
extern BOOL	g_bGuard;																			/* Z を押し続けて構えている */
extern DWORD	g_dwGuardGageElapse;																/* 構えた時に溜まっていたゲージの時間(構えている間は止める) */
extern DWORD	GetTurnGageTime			(PCHARINFO pChar);											/* ターンゲージが満タンになるまでの時間(ms) */
extern BOOL	IsTurnGageJust			(PCHARINFO pChar);											/* ゲージ満タン直後(ジャストで押せる時間)か */
extern BOOL	g_bAutoGage;																		/* 目の前の敵で自動で溜め始めたゲージ */
extern DWORD	g_dwAutoGageLastFront;																/* 自動ゲージで最後に目の前に敵がいた時間 */
extern DWORD	g_dwAutoGageSuppress;																/* Z でやめた相手 */
extern DWORD	g_dwTurnGageContinue;																/* 殴って次のゲージを溜め始めた時間 */
extern void	ContinueTurnGage		(PCHARINFO pChar);											/* 連続攻撃：次のターンゲージを溜め始める */
extern int	FindFrontAttackTarget	(PCHARINFO pChar);											/* 目の前にいる敵を探す */
extern void	RecvDataProc			(PBYTE, DWORD);												/* 受信データ処理 */
extern void	RecvDataHoldProc		(void);														/* 保留中の受信データを処理 */
extern void	RecvDataHoldClear		(void);														/* 保留中の受信データを捨てる */

/* Copyright(C)URARA-works 2003-2006 */
