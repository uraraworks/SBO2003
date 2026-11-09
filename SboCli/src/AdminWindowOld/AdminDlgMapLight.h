/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		明るさ・灯り・見え方の設定ダイアログ処理定義ファイル		 */
/* ファイル名：	AdminDlgMapLight.h											 */
/* 作成日：		2026/11/09													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void ShowDlgMapLight(HWND hWndParent);							/* ダイアログ表示 */
extern void CloseDlgMapLight(void);										/* ダイアログ終了(送っていない変更は送るか聞く) */
extern BOOL IsDlgMapLightOpen(void);									/* 開いているか */
extern BOOL MapLightDlgOnPos(int x, int y);								/* ゲーム画面のクリック(使ったら TRUE) */
extern void MapLightDlgTick(void);										/* 自分のマップが変わったら閉じる（描画のたびに呼ぶ） */
extern void MapLightDlgOnSent(void);									/* マップ情報画面の「送信」で送った */

/* ゲーム画面・マップ情報画面への表示用 */
extern int MapLightDlgGetSelArea(void);									/* 選んでいる範囲(無ければ -1) */
extern int MapLightDlgGetSelLight(void);								/* 選んでいる光源(無ければ -1) */
extern BOOL MapLightDlgGetCorner(int &x, int &y);						/* 範囲の右下のクリック待ちならその左上 */
extern void MapDarkAreaLabel(int nNo, LPSTR pszText, int nSize);		/* 範囲を文字にする */
extern void MapLightLabel(int nNo, LPSTR pszText, int nSize);			/* 光源を文字にする */

/* Copyright(C)URARA-works 2026 */
