/* Copyright(C)URARA-works 2026 */
/* ========================================================================= */
/* 概要：		NPCの巡回ダイアログ処理定義ファイル							 */
/* ファイル名：	AdminDlgNpcRoute.h											 */
/* 作成日：		2026/11/06													 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void ShowDlgNpcRoute(HWND hWndParent);							/* ダイアログ表示(NPCの巡回) */
extern void CloseDlgNpcRoute(void);										/* ダイアログ終了(NPCの巡回) */
extern void NpcRouteOnRecv(const void *pPara);							/* サーバーからの通知(CMDPARA_NPCROUTE) */
extern void NpcRouteOnPosLeft(int x, int y);							/* 左クリック：道しるべを足す */
extern void NpcRouteOnPosRight(int x, int y);							/* 右クリック：そのマスのNPCを選ぶ */
extern void NpcRouteReload(void);										/* 巡回ルートを取り直す */
extern void NpcRouteTick(void);											/* 自分のマップが変わったか調べる（描画のたびに呼ぶ） */

/* ゲーム画面への描画用 */
extern BOOL NpcRouteIsListed(DWORD dwTblID);							/* 一覧にあるNPCか */
extern DWORD NpcRouteGetSelTblID(void);									/* 選んでいるNPCのテーブルID（無ければ -1） */
extern int NpcRouteGetEditCount(void);									/* 編集中の道しるべ数 */
extern void NpcRouteGetEditPoint(int nNo, int &x, int &y);				/* 編集中の道しるべの座標 */
extern int NpcRouteGetEditMode(void);									/* 編集中の回り方(NPCROUTEMODE_*) */

/* Copyright(C)URARA-works 2026 */
