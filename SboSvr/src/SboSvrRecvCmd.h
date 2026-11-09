/* Copyright(C)URARA-works 2003-2006 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			コマンド受信処理の外部参照用定義						 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
extern void		MyOutputDebugString			(LPCTSTR lpszFormat, ...);						/* デバッグメッセージ出力 */
extern BOOL		CheckAdminBan				(DWORD);										/* 管理者権限かチェックして、違ってたらＢＡＮ */
extern PCHARINFO AddChar					(PCHARINFO, BOOL bCheck=TRUE);					/* キャラ追加 */
extern void		DeleteChar					(DWORD);										/* キャラ削除 */
extern int		GetCharNo					(DWORD);										/* キャラ番号取得 */
extern int		GetMapNo					(DWORD);										/* マップ番号取得 */
extern int		GetEventDataSize			(BYTE);											/* イベント種別毎のデータサイズを取得 */
extern int		GetSubGameDataSize			(BYTE);											/* サブゲーム種別毎のデータサイズを取得 */
extern int		GetItemDataSize				(BYTE);											/* アイテム種別毎のデータサイズを取得 */
extern int		GetItemCount				(DWORD);										/* アイテム数を取得 */
extern int		GetSubGameID				(PSUBGAMEINFO);									/* サブゲームIDを取得 */
extern int		SearchEvent					(DWORD, DWORD, DWORD);							/* 座標を元にイベント番号検索 */
extern void		AddCharItem					(DWORD, DWORD);									/* アイテム追加 */
extern void		DelCharItem					(DWORD, BYTE);									/* アイテム捨てる */
extern BOOL		AddLeaveItem				(PCHARINFO, WORD);								/* 預かり所にアイテムを1個入れる */
extern BOOL		CheckCharItem				(DWORD, DWORD, BYTE);							/* アイテム持ってるかチェック */
extern void		AddTblNoPC					(int);											/* PC番号テーブルに追加 */
extern void		DeleteTblNoPC				(int);											/* PC番号テーブルから削除 */
extern void		RenewTblNoChar				(void);											/* PC番号テーブルを更新 */
extern void		DeleteChildChar				(PCHARINFO);									/* 自分が親のキャラを削除 */
extern void		SetInitCharData				(PCHARINFO);									/* キャラの初期値を設定 */
extern void		CharNpcIdEnsure				(PCHARINFO);									/* NPC番号がまだ無ければ振る(SboSvrSaveLoadChar.cpp) */
extern void		CharNpcIdRenumberAll		(void);											/* 読み込み後に NPC番号を振り直す */
extern int		CharFindByNpcId				(DWORD);										/* NPC番号からテーブル番号を探す */
extern void		CharTargetCancelPC			(PCHARINFO);									/* キャラのついていく解除 */
extern void		TailMoveChain				(PCHARINFO);									/* ついてきているキャラを順に動かす */
extern void		CharTargetPC				(PCHARINFO, PCHARINFO, DWORD);					/* キャラのついてく設定 */
extern BOOL		CheckCharX					(PCHARINFO pChar, DWORD dwX, DWORD dwXTarget);	/* キャラサイズに合わせた当たり判定(横) */
extern int		GetCharX					(PCHARINFO, DWORD, DWORD, BOOL);				/* キャラサイズに合わせた距離を求める(横) */
extern int		GetCharDistanceX			(PCHARINFO, PCHARINFO);							/* キャラサイズに合わせた距離を求める(横) */
extern DWORD	GetCharXPos					(PCHARINFO, DWORD, BYTE);						/* キャラサイズに合わせた座標を補正(横) */
extern BOOL		CheckCharBlock				(DWORD, DWORD, DWORD);							/* キャラのぶつかり判定 */
extern BOOL		CheckCharBlock				(PCHARINFO pChar, DWORD, DWORD, DWORD);			/* キャラのぶつかり判定 */
extern int		GetCharBridgeLayerSvr		(PCHARINFO pChar, BOOL *pbKnown);				/* 立体交差の橋の上(1)か下(2)か */
extern void		FitCharDirectionBridgeLayer	(PCHARINFO pChar);								/* ログアウト時の向きを橋の上か下かに合わせる */
extern DWORD	CheckCharBlockPC			(DWORD, DWORD, DWORD);							/* キャラのキャラとのぶつかり判定 */
extern DWORD	CheckCharBlockNPC			(DWORD, DWORD, DWORD);							/* キャラのキャラとのぶつかり判定(NPC) */
extern DWORD	CheckCharBlockNPC2			(DWORD, DWORD, DWORD, DWORD);					/* キャラのキャラとのぶつかり判定2(NPC) */
extern BOOL		CheckEventBlock				(DWORD, DWORD, DWORD, BYTE bCheck = 0xFF);		/* 指定座標にイベントがあるか判定 */
extern void		RenewATDF					(DWORD);										/* 攻撃力・守備力の更新 */
extern BOOL		CheckMagicUserInfo			(BYTE, BYTE);									/* 魔法利用者情報にあるか判定 */
extern void		SubProcMapMove				(PCHARINFO, DWORD, DWORD, DWORD);				/* マップ移動処理 */
extern void		SubProcMove					(PCHARINFO, DWORD, DWORD);						/* 同マップ内移動処理 */
extern void		DeleteEvent					(DWORD, int);									/* イベント情報を削除 */
extern void		RecvDataProc				(BYTE *, DWORD);								/* 受信データ処理 */
extern int		AddEvent					(DWORD);										/* イベント情報を追加 */
extern void		ProcBreakParty				(PCHARINFO);									/* パーティから抜ける */
extern void		ProcBreakUpParty			(PCHARINFO);									/* パーティを解散する */

/* 戦場ゲーム用の処理 */
extern int		SubWar_GetMemberCount		(PSUBGAME_WARINFO, BYTE);						/* 参加人数を取得 */
extern int		SubWar_CheckJoin			(PSUBGAME_WARINFO, DWORD);						/* 参加しているかチェック */
extern void		SubWar_Remove				(PSUBGAME_WARINFO, DWORD);						/* ゲームから離脱 */
extern void		SubWar_RemoveAndMove		(PSUBGAME_WARINFO, DWORD);						/* ゲームから離脱して受付位置に移動 */
extern void		SubWar_AddMember			(PSUBGAME_WARINFO, BYTE, DWORD);				/* 参加 */
extern void		SubWar_JudgeMsg				(PSUBGAME_WARINFO, LPCSTR);						/* 審判キャラ発言 */
extern void		SubWar_AllChgState			(PSUBGAME_WARINFO, BYTE);						/* 参加者全員の状態変更 */
extern void		SubWar_AllChgStateAndQuit	(PSUBGAME_WARINFO, BYTE, BYTE);					/* 参加者全員の状態変更(条件付き) */
extern void		SubWar_PrePare				(PSUBGAME_WARINFO);								/* 準備処理 */
extern void		SubWar_ChgTurn				(PSUBGAME_WARINFO);								/* ターン変更 */
extern void		SubWar_Quit					(PSUBGAME_WARINFO);								/* 試合終了 */

/* Copyright(C)URARA-works 2003-2006 */
