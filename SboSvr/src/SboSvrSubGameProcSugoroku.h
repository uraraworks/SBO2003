/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			サブゲーム(すごろく)用処理の外部参照用定義				 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2004/12/29												 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
/* すごろく用の処理 */
extern int SubSugoroku_CheckJoin(PSUBGAME_SUGOROKUINFO, DWORD);				/* 参加しているかチェック */
extern void SubSugoroku_Remove(PSUBGAME_SUGOROKUINFO, DWORD);					/* ゲームから離脱 */
extern void SubSugoroku_RemoveAndMove(PSUBGAME_SUGOROKUINFO, DWORD);			/* ゲームから離脱して受付位置に移動 */
extern int SubSugoroku_AddMember(PSUBGAME_SUGOROKUINFO, BYTE, DWORD);			/* 参加 */
extern void SubSugoroku_Msg(PSUBGAME_SUGOROKUINFO, LPCSTR);					/* メッセージ */
extern void SubSugoroku_GetStartPos(PSUBGAME_SUGOROKUINFO, PDWORD, PDWORD);	/* スタート地点を取得 */
extern void SubSugoroku_GetGoalPos(PSUBGAME_SUGOROKUINFO, PDWORD, PDWORD);		/* ゴール地点を取得 */
extern void SubSugoroku_Quit(PSUBGAME_SUGOROKUINFO);							/* ゲーム終了 */
extern int SubSugoroku_GetNo(PSUBGAME_SUGOROKUINFO, DWORD);					/* テーブル番号を取得 */
extern int SubSugoroku_GetNextDirection(PSUBGAME_SUGOROKUINFO, BYTE);			/* 次の進行方向を取得 */
extern void SubSugoroku_Move(PSUBGAME_SUGOROKUINFO, BYTE);						/* 移動 */
extern BOOL SubSugoroku_CheckParts(PSUBGAME_SUGOROKUINFO, BYTE);				/* パーツのチェック */
extern void SubSugoroku_NextPlayer(PSUBGAME_SUGOROKUINFO);						/* 次のプレイヤーに交替 */
extern BOOL SubSugoroku_IsAllGoal(PSUBGAME_SUGOROKUINFO);						/* 全員ゴールしているか判定 */
extern void SubSugoroku_GoalProc(PSUBGAME_SUGOROKUINFO, BYTE);					/* ゴール処理 */

/* すごろくの土地取りルール（SboSvrSubGameProcLand.cpp。仕様は docs/land-sugoroku.md） */
extern void SubLand_Start(PSUBGAME_SUGOROKUINFO);								/* 土地取りを始める */
extern void SubLand_Stop(PSUBGAME_SUGOROKUINFO);								/* 進行を止める（終了処理の最初） */
extern void SubLand_Release(PSUBGAME_SUGOROKUINFO);								/* 進行の情報を捨てる（終了処理の最後） */
extern BOOL SubLand_Active(PSUBGAME_SUGOROKUINFO);								/* 土地取りの進行を持っているか */
extern void SubLand_Proc(PSUBGAME_SUGOROKUINFO, DWORD);							/* ゲーム開始後の処理（制限時間・駒の移動・片付け） */
extern void SubLand_Dice(PSUBGAME_SUGOROKUINFO, DWORD);							/* サイコロを振る */
extern void SubLand_Buy(PSUBGAME_SUGOROKUINFO, DWORD, BOOL);					/* 空き地を買うかの返事 */
extern void SubLand_OnLeave(PSUBGAME_SUGOROKUINFO, DWORD);						/* 参加者が抜けた（脱落） */
extern void SubLand_SendTo(PSUBGAME_SUGOROKUINFO, DWORD);						/* 1 人へ盤の状態を送る */
extern void SubLand_SendNone(PCHARINFO);										/* 盤の重ね描きを消させる */
extern LandGame *SubLand_GetGame(PSUBGAME_SUGOROKUINFO);						/* ルールの状態（テスト・調べる用。進行が無ければ NULL） */

/* Copyright(C)URARA-works 2003-2005 */
