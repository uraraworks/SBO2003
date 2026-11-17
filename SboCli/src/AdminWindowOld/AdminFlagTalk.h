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
extern void AdminFlagGroupOnRecv(const void *pPara);					/* サーバーから届いた（CMDPARA_ADMINFLAGGROUP） */

/* フラグの名前表 */
extern void AdminFlagNameRequest(void);									/* サーバーから取り直す（届くまで空） */
extern void AdminFlagNameSend(int nNo, const FLAGNAMEINFO *pInfo);		/* nNo 番目に書き込む（-1 は足す。削除は byState=0） */
extern int  AdminFlagNameGetCount(void);								/* 届いている件数 */
extern const FLAGNAMEINFO *AdminFlagNameGet(int nNo);					/* 届いている 1 件（まだ届いていなければ NULL） */
extern DWORD AdminFlagNameGetSeq(void);									/* 更新番号 */

/* フラグのグループ（印・数値の番号の帯に付けたメイン名。番号 n が帯に入れば「メイン-サブ」になる。見せ方・入力・絞り込みだけの層） */
extern void AdminFlagGroupRequest(void);								/* サーバーから取り直す（届くまで空） */
extern void AdminFlagGroupSend(int nNo, const FLAGGROUPINFO *pInfo);	/* nNo 番目に書き込む（-1 は足す。削除は byState=0） */
extern int  AdminFlagGroupGetCount(void);								/* 届いている件数 */
extern const FLAGGROUPINFO *AdminFlagGroupGet(int nNo);					/* 届いている 1 件（まだ届いていなければ NULL） */
extern DWORD AdminFlagGroupGetSeq(void);								/* 更新番号 */
extern void AdminFlagGroupLoadIfEmpty(void);							/* まだ一度も届いていなければ取り寄せる（画面を開く時に呼ぶ） */
extern const FLAGGROUPINFO *AdminFlagGroupFind(BYTE byKind, WORD wNo);	/* 番号が入る有効なグループ（無ければ NULL） */
extern BOOL AdminFlagGroupFormat(BYTE byKind, WORD wNo, char *pszOut, int nSize);	/* 「ルシー-3」を作る（どのグループにも入らなければ FALSE で空） */
extern BOOL AdminFlagParseNo(BYTE byKind, const char *pszText, WORD &wNo, char *pszErr, int nErrSize);	/* 番号の入力（「103」か「グループ番号-サブ」例 3-2。グループ名-サブも可）を番号にする。だめなら理由を pszErr に入れて FALSE */

/* 会話データ */
extern void AdminTalkRequest(void);										/* サーバーから取り直す（届くまで空） */
extern void AdminTalkSend(int nNo, const TALKINFO *pInfo);				/* nNo 番目に書き込む（-1 は足す。削除は byState=0） */
extern int  AdminTalkGetCount(void);									/* 届いている件数 */
extern const TALKINFO *AdminTalkGet(int nNo);							/* 届いている 1 件（まだ届いていなければ NULL） */
extern DWORD AdminTalkGetSeq(void);										/* 更新番号 */
extern BOOL AdminTalkIsLoaded(void);									/* 取り直しの返事が最後の 1 件まで届いたか（使用先・マップの名前も届いている） */
extern int  AdminTalkGetMapCount(void);									/* 届いているマップの数（絞り込みの選択肢） */
extern DWORD AdminTalkGetMapID(int nIndex);								/* nIndex 番目のマップID（範囲外は -1） */
extern const char *AdminTalkGetMapName(int nIndex);						/* nIndex 番目のマップの名前（空のことがある） */
extern void AdminTalkFormatMapName(DWORD dwMapID, char *pszOut, int nSize);	/* マップIDから表示用の名前（名前が無ければ「マップ番号」） */
extern int  AdminTalkGetUseCount(void);									/* 会話セットの使用先（"@t番号" の NPC）の数 */
extern const CMDPARA_ADMINTALKUSE *AdminTalkGetUse(int nIndex);			/* 使用先 1 件（wTalkNo=会話セット番号・dwMapID・dwNpcID・szName=NPC名） */

/* キャラのフラグ（プレイヤーのキャラ 1 人分。サーバーのキャラ番号 dwTblID で指す） */
extern void AdminCharFlagRequest(DWORD dwTblID);						/* そのキャラの印と数値を取り直す */
extern void AdminCharFlagSetBit(DWORD dwTblID, WORD wNo, BOOL bOn);		/* 印を立てる・消す */
extern void AdminCharFlagSetNum(DWORD dwTblID, WORD wNo, BYTE byVal);	/* 数値を変える */
extern BOOL AdminCharFlagGet(DWORD *pdwTblID, BYTE *pbyFlag, BYTE *pbyFlagNum);	/* 届いている 1 人分（届いていなければ FALSE。各配列は MAX_FLAGBITBYTE・MAX_FLAGNUM バイト。NULL 可） */
extern DWORD AdminCharFlagGetSeq(void);									/* 更新番号 */

/* 書き込みの結果 */
extern const ADMINFLAGTALKRESULT *AdminFlagTalkGetResult(void);			/* 直近の結果 */

/* 届いた時のお知らせ（画面が更新番号を見て描き直す）。届くたびに、登録された関数を全部呼ぶ */
extern void AdminFlagTalkAddListener(void (*pfnListener)(void));		/* 登録（画面を開いた時） */
extern void AdminFlagTalkRemoveListener(void (*pfnListener)(void));		/* 外す（画面を閉じた時） */

/* 名前表の引き方（画面で、印・数値の番号の横に名前を出すため） */
extern int  AdminFlagNameFindIndex(BYTE byKind, WORD wNo);				/* 名前表の配列の番号（削除済みの枠も含む。無ければ -1） */
extern const char *AdminFlagNameFind(BYTE byKind, WORD wNo);			/* 有効な名前（無ければ NULL） */
extern void AdminFlagNameLoadIfEmpty(void);								/* まだ一度も届いていなければ取り寄せる（画面を開く時に呼ぶ） */

/* Copyright(C)URARA-works 2026 */
