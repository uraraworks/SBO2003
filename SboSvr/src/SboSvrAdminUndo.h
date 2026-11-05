/* ========================================================================= */
/* ファイル名：	SboSvrAdminUndo.h											 */
/* 内容：		管理者画面の「元に戻す」（画面単位のアンドゥ）				 */
/* ========================================================================= */
/*
 * 管理者が管理者ウィンドウで変えたデータを、画面ごとに 1 回分ずつ元に戻す。
 *
 *  - 管理者用のコマンドを処理する前に、変わる対象（アイテム 1 件・マップの 1 マスなど）の
 *    「今の状態」を、クライアントが送ってくるのと同じ形のパケットにして取っておく。
 *    処理した後の状態も取っておき、元に戻す時は前の状態のパケットを処理し直す。
 *    （反映・全員への通知は今ある受信処理がそのまま行う）
 *  - 記録は「その時に管理者が開いていた画面」ごとに分ける。同じ人が同じ画面で
 *    3 秒以内に続けて送った分（「送信」ボタンで何件も送る・マップを塗る、など）は 1 回分にまとめる。
 *  - 元に戻す前に、対象が記録した「変更後」のままかを確かめる。あとからゲーム中の変化や
 *    ほかの管理者・API の変更が入っていたら、上書きしないで戻すのをやめる。
 *  - 記録はサーバーのメモリだけに持つ（再起動すると消える）。
 *
 * API・MCP からデータを変える時も、AdminUndoApplyPacket（ブラウザ版サーバーでは
 * SboAdminApplyPacket）を通せば同じように記録され、同じ画面の単位で戻せる。
 * 詳しくは docs/admin-undo.md。
 */
#pragma once

#include <string>

/* 管理者用コマンドを処理する前に呼ぶ。記録の対象なら TRUE（処理の後に AdminUndoAfter を呼ぶ） */
BOOL	AdminUndoBefore		(BYTE byCmd, const BYTE *pData, DWORD dwSize, DWORD pID, LPCSTR pszSource, BYTE byScreen);
/* 管理者用コマンドを処理した後に呼ぶ（変わっていれば記録する） */
void	AdminUndoAfter		(void);

/* 管理者がいま開いている画面を覚える（ADMINSCR_*） */
void	AdminUndoSetScreen	(DWORD pID, BYTE byScreen);
BYTE	AdminUndoGetScreen	(DWORD pID);
/* 接続が切れた */
void	AdminUndoOnDisconnect	(DWORD pID);

/* その画面で次に戻す変更の案内。戻せる変更が無ければ FALSE */
BOOL	AdminUndoGetInfo	(BYTE byScreen, DWORD &dwStepID, WORD &wCount, std::string &strMsg);
/* その画面の最後の変更を元に戻す。dwStepID が 0 以外なら、その変更の時だけ戻す。
   pID は戻す処理を頼んだ接続（API からなら 0）。戻せたら TRUE */
BOOL	AdminUndoExec		(BYTE byScreen, DWORD dwStepID, DWORD pID, std::string &strMsg, WORD &wCount);

/* 画面の名前（「アイテム一覧」など） */
LPCSTR	AdminUndoScreenName	(BYTE byScreen);
/* 何も記録していないコマンドのいつもの画面（API から画面を指定されなかった時に使う） */
BYTE	AdminUndoDefaultScreen	(BYTE byCmd);
