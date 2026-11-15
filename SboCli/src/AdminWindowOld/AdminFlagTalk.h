/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		フラグの名前表・会話データ・キャラのフラグ(管理者用)定義ファイル	 */
/* ファイル名：	AdminFlagTalk.h												 */
/* 作成日：		2026/11/15													 */
/* ========================================================================= */
#pragma once

/*
 * 管理者ウィンドウから、サーバーのフラグの名前表・会話データ・キャラのフラグを
 * 見る・直すための通信と、届いたデータを手元に覚えておく所（画面はまだ無い）。
 * 画面は、ここの関数で一覧を取り直し（…Request）、届いたものを読み（…Get）、
 * 書き込み（…Send）の結果を AdminFlagTalkGetResult で読む。届くたびに更新番号が増えるので、
 * 画面は更新番号が変わったら描き直せばよい。
 */

/* 書き込みの結果（直近の 1 件） */
typedef struct _ADMINFLAGTALKRESULT {
	DWORD		dwSeq;												/* 届くたびに増える番号（0=まだ届いていない） */
	BYTE		byCmd;												/* どのコマンドの結果か（CMD_ADMINFLAGNAME・CMD_ADMINTALK・CMD_ADMINCHARFLAG） */
	BYTE		byResult;											/* 1=うまくいった 0=だめだった */
	int			nNo;												/* 書き込んだ番号（名前表・会話セットの配列の番号） */
	char		szMsg[MAX_ADMINQUESTMSG];							/* だめだった理由 */
} ADMINFLAGTALKRESULT, *PADMINFLAGTALKRESULT;

/* 受信（SboCliRecvCmd.cpp から呼ぶ） */
extern void AdminFlagNameOnRecv(const void *pPara);						/* サーバーから届いた（CMDPARA_ADMINFLAGNAME） */
extern void AdminTalkOnRecv(const void *pPara);							/* サーバーから届いた（CMDPARA_ADMINTALK） */
extern void AdminCharFlagOnRecv(const void *pPara);						/* サーバーから届いた（CMDPARA_ADMINCHARFLAG） */

/* フラグの名前表 */
extern void AdminFlagNameRequest(void);									/* サーバーから取り直す（届くまで空） */
extern void AdminFlagNameSend(int nNo, const FLAGNAMEINFO *pInfo);		/* nNo 番目に書き込む（-1 は足す。削除は byState=0） */
extern int  AdminFlagNameGetCount(void);								/* 届いている件数 */
extern const FLAGNAMEINFO *AdminFlagNameGet(int nNo);					/* 届いている 1 件（まだ届いていなければ NULL） */
extern DWORD AdminFlagNameGetSeq(void);									/* 更新番号 */

/* 会話データ */
extern void AdminTalkRequest(void);										/* サーバーから取り直す（届くまで空） */
extern void AdminTalkSend(int nNo, const TALKINFO *pInfo);				/* nNo 番目に書き込む（-1 は足す。削除は byState=0） */
extern int  AdminTalkGetCount(void);									/* 届いている件数 */
extern const TALKINFO *AdminTalkGet(int nNo);							/* 届いている 1 件（まだ届いていなければ NULL） */
extern DWORD AdminTalkGetSeq(void);										/* 更新番号 */

/* キャラのフラグ（プレイヤーのキャラ 1 人分。サーバーのキャラ番号 dwTblID で指す） */
extern void AdminCharFlagRequest(DWORD dwTblID);						/* そのキャラの印と数値を取り直す */
extern void AdminCharFlagSetBit(DWORD dwTblID, WORD wNo, BOOL bOn);		/* 印を立てる・消す */
extern void AdminCharFlagSetNum(DWORD dwTblID, WORD wNo, BYTE byVal);	/* 数値を変える */
extern BOOL AdminCharFlagGet(DWORD *pdwTblID, BYTE *pbyFlag, BYTE *pbyFlagNum);	/* 届いている 1 人分（届いていなければ FALSE。各配列は MAX_FLAGBITBYTE・MAX_FLAGNUM バイト。NULL 可） */
extern DWORD AdminCharFlagGetSeq(void);									/* 更新番号 */

/* 書き込みの結果 */
extern const ADMINFLAGTALKRESULT *AdminFlagTalkGetResult(void);			/* 直近の結果 */

/* Copyright(C)URARA-works 2026 */
