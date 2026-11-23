/* ========================================================================= */
/* ファイル名：	SboSvrMapSwapEdit.h											 */
/* 内容：		マップの差し替え表（印で変わるマス・宝箱・リサイクル箱）の	 */
/*				書き込みの決まり（確かめ・番号の自動振り）					 */
/* ========================================================================= */
/*
 * 差し替え表（MAPINFO の aSwap）を書き換える入口は 2 つある。
 *   ・データ API（server/web_data_svr.cpp の map の swaps。JSON）
 *   ・ゲーム内の管理者ウィンドウ（CMD_ADMINSWAP。SboSvrRecvCmd.cpp）
 * どちらも、入力を MAPSWAPEDITROW の並びにして MapSwapBuild に渡す。
 * 範囲・種類に合わない欄・重なり・番号の自動振り（消した番号を使い回さない）の決まりは
 * ここだけに書き、入口ごとに二重に持たない。
 * 反映（MapSwapSet）と、そのマップの全員への送り直し（MapSwapSendMap）、保存も、入口で同じ順に行う。
 */
#pragma once

#include <string>
#include <vector>

/* 入力で「指定した欄」（種類に合わない欄を指定したら断る。番号は指定しなければ自動で振る） */
#define MAPSWAPEDIT_FLAG		(0x0001)						/* wFlag */
#define MAPSWAPEDIT_CMP			(0x0002)						/* byCmp */
#define MAPSWAPEDIT_CHESTNO		(0x0004)						/* wChestNo */
#define MAPSWAPEDIT_BOXNO		(0x0008)						/* wBoxNo */
#define MAPSWAPEDIT_GIVEITEM	(0x0010)						/* wGiveItem */
#define MAPSWAPEDIT_MSG			(0x0020)						/* szMsg */
#define MAPSWAPEDIT_MEMO		(0x0040)						/* szMemo */
#define MAPSWAPEDIT_NEEDFLAG	(0x0080)						/* wNeedFlag（宝箱だけ。開けるのに要る印。FLAG_NOBIT=条件なし） */
#define MAPSWAPEDIT_LOCKMSG		(0x0100)						/* szLockMsg（宝箱だけ。開けられない時の文） */

/* 入力 1 行。値は範囲を確かめる前なので、広い型で持つ */
typedef struct _MAPSWAPEDITROW {
	long long		nKind;										/* MAPSWAPKIND_* */
	long long		nX;											/* X */
	long long		nY;											/* Y */
	long long		nParts;										/* 差し替え後のパーツ */
	DWORD			dwHas;										/* 指定した欄（MAPSWAPEDIT_*） */
	long long		nFlag;										/* 印の番号 */
	long long		nCmp;										/* MAPSWAPCMP_* */
	long long		nChestNo;									/* 宝箱の番号 */
	long long		nBoxNo;										/* リサイクル箱の番号 */
	long long		nGiveItem;									/* 渡す品 */
	std::string		strMsg;										/* 調べた時の文 */
	std::string		strMemo;									/* 管理用のメモ */
	long long		nNeedFlag;									/* 開けるのに要る印 */
	std::string		strLockMsg;									/* 開けられない時の文 */
	_MAPSWAPEDITROW() : nKind (0), nX (0), nY (0), nParts (0), dwHas (0), nFlag (0), nCmp (0), nChestNo (0), nBoxNo (0), nGiveItem (0), nNeedFlag (FLAG_NOBIT) {}
} MAPSWAPEDITROW, *PMAPSWAPEDITROW;

/* 表を確かめて w.aSwap・w.bySwapCount・w.wChestHigh・w.wBoxHigh に作る。
   w は変更後のマップ（番号の高さは今の値から始まる）。nMapNo はそのマップの番号（ほかのマップとの重なりを調べる）。
   pszPrefix はエラーの行の呼び名（"swaps" なら "swaps[3].x は…"）。失敗したら strErr に理由を入れて FALSE（w の表は不定） */
BOOL	MapSwapBuild			(int nMapNo, MAPINFO &w, const std::vector<MAPSWAPEDITROW> &aRow, LPCSTR pszPrefix, std::string &strErr);
/* 今の行を入力の形にする（1 行だけ直す・消す時に、ほかの行をそのまま渡すため） */
void	MapSwapToEdit			(const MAPSWAP &Sw, MAPSWAPEDITROW &Row);
/* 通信の行 ⇔ 入力・今の行 */
void	MapSwapEditFromWire		(const ADMINSWAPROW &Wire, MAPSWAPEDITROW &Row);
void	MapSwapWireFromSwap		(const MAPSWAP &Sw, ADMINSWAPROW &Wire);

/* 表（と番号の覚え）が変わったか */
BOOL	MapSwapChanged			(const MAPINFO &Cur, const MAPINFO &w);
/* MapSwapBuild で作った表を、マップへ反映する（送り直し・保存は呼ぶ側） */
void	MapSwapSet				(PMAPINFO pMap, const MAPINFO &w);

/* 全マップを通した番号 */
BOOL	ChestNoUsedElsewhere	(int nSkipMap, int nChestNo);		/* 宝箱の番号を、ほかのマップの行が使っているか（nSkipMap のマップは数えない） */
int		ChestHighAll			(void);								/* 今までに使った宝箱の番号の最大+1（消した番号の覚えを含む。次に自動で振る番号） */
BOOL	RecycleBoxNoUsedElsewhere(int nSkipMap, int nBoxNo);		/* リサイクル箱の番号を、ほかのマップの行が使っているか */
int		RecycleBoxHighAll		(void);								/* 今までに使ったリサイクル箱の番号の最大（0=まだ無し。次に自動で振る番号はこの+1） */
