#pragma once
#include <string.h>
#include <stddef.h>
/* 題名はサーバーからの任意の拡張。クライアントからは従来の長さで送る。 */
inline DWORD DungeonSelectionCommandSize() { return (DWORD)offsetof (CMDPARA_DUNGEONSEL, szTitle); }
static_assert (sizeof (CMDPARA_DATA1) + sizeof (SUBGAME_DUNGEONINFO) + 8 < 65536, "サブゲーム情報は通信上限未満");
/* 受付だけが付ける末尾の印を外し、会話を閉じた時の続行を予約する。 */
inline BOOL DungeonReceptionMessage(char *psz)
{
    size_t n = strlen (psz);
    if (n < 4 || strcmp (psz + n - 4, "@2@D") != 0) return FALSE;
    psz[n - 2] = 0;
    return TRUE;
}
/* クライアントと回帰テストで同じパケットを使う。 */
inline void DungeonSelectionCommand(CMDPARA_DUNGEONSEL &p, DWORD dwTblID, BYTE byType, BYTE byIndex)
{
    memset (&p, 0, sizeof (p));
    p.byCmd = CMD_DUNGEONSEL;
    p.byType = byType;
    p.byIndex = byIndex;
    p.dwTblID = dwTblID;
}
