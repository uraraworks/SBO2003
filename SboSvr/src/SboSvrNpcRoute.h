/* ========================================================================= */
/* ファイル名：	SboSvrNpcRoute.h											 */
/* 内容：		NPCの巡回ルートの検査と反映									 */
/* ========================================================================= */
/*
 * 巡回ルートを変える入口は 3 つある（管理者ウィンドウの CMD_NPCROUTE・管理ページ（Web）・
 * 「元に戻す」）。どれも検査と反映をここで行い、ルールが食い違わないようにする。
 */
#pragma once

/* ルートを検査する。問題があればエラー文（UTF-8）、問題なければ NULL */
LPCSTR	NpcRouteCheck		(PCHARINFO pChar, int nMode, int nWait, int nCount, const int *pnX, const int *pnY);
/* ルートを反映する（動き方を作り直してキャラ情報を保存する。検査済みの値だけ渡すこと） */
void	NpcRouteApply		(PCHARINFO pChar, BYTE byMode, BYTE byWait, int nCount, const BYTE *pbyX, const BYTE *pbyY);
/* キャラの今のルートをコマンド（CMDPARA_NPCROUTE）にする */
void	NpcRouteMakePara	(CMDPARA_NPCROUTE &Para, BYTE byCmdSub, PCHARINFO pChar);
/* 巡回の設定ができる NPC か（動かない・ランダム移動） */
BOOL	NpcRouteIsTarget	(PCHARINFO pChar);
