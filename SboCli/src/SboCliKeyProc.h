/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			キー入力処理の外部参照用定義							 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void KeyProc							(void);							/* キー入力処理 */
extern void KeyProcMoveNext					(void);							/* 歩き終わった時の続けて歩く処理 */
extern LRESULT CALLBACK AccWndProc			(HWND, UINT, WPARAM, LPARAM);	/* アカウント名入力用メッセージ処理 */
extern LRESULT CALLBACK PasWndProc			(HWND, UINT, WPARAM, LPARAM);	/* パスワード入力用メッセージ処理 */
extern LRESULT CALLBACK ConWndProc			(HWND, UINT, WPARAM, LPARAM);	/* 接続ボタン用メッセージ処理 */
extern LRESULT CALLBACK NameWndProc			(HWND, UINT, WPARAM, LPARAM);	/* キャラ名入力用メッセージ処理 */
extern LRESULT CALLBACK MakeWndProc			(HWND, UINT, WPARAM, LPARAM);	/* 作成ボタン用メッセージ処理 */
extern LRESULT CALLBACK SpeechWndProc		(HWND, UINT, WPARAM, LPARAM);	/* 発言入力欄用メッセージ処理 */
extern LRESULT CALLBACK SpeechEditWndProc	(HWND, UINT, WPARAM, LPARAM);	/* 自己紹介設定欄用メッセージ処理 */
extern LRESULT CALLBACK ChgPasswordWndProc	(HWND, UINT, WPARAM, LPARAM);	/* パスワード変更用メッセージ処理 */
extern void ChatMsgProc(LPCTSTR pszMsg);									/* 発言メッセージ処理 */
extern int  GetMagicKeyNo					(BYTE byMagicID);				/* 特技ショートカットの登録枠(0〜4。無ければ -1) */
extern BOOL IsMagicLearned					(PCHARINFO pChar, BYTE byMagicID);	/* 今の職業でその特技を覚えているか */
extern int  GetMagicKeyKind					(PMAGICINFO pInfo);				/* 帯の色分けの種類(0=攻撃 1=回復 2=補助) */
extern BOOL IsMagicKeyUsable				(PCHARINFO pChar, BYTE byMagicID);	/* 特技が今使えるか(帯を暗くするかの判定) */
extern BOOL IsMagicKeyMode					(PCHARINFO pChar);				/* 1〜5 キーで特技を使える状態か */
extern void RegisterMagicKey				(int nNo, BYTE byMagicID);		/* 特技を枠に登録する・外す */
extern void UseMagicKey						(PCHARINFO pChar, int nNo);		/* 枠の特技を使う */
/* Copyright(C)URARA-works 2003-2005 */
