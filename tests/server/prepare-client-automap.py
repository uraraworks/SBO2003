#!/usr/bin/env python3
"""AO で本物のブラウザ受信・探索送信処理を使うため、描画以外を抽出する。"""
import pathlib
import sys

root, output = map(pathlib.Path, sys.argv[1:])
automap = (root / 'SboCli/src/SboCliAutoMap.cpp').read_text()
transport = (root / 'web/replace/CUraraSockTCP_web.cpp').read_text()
# 関数の境界で切り、ロジック自体は変更しない。
state = automap[automap.index('static DWORD s_dwSpace'):automap.index('/* ========================================================================= */\n/* 関数名：AutoMapIsAvailable')]
recv = transport[transport.index('void CUraraSockTCP::OnWsMessage'):transport.index('void CUraraSockTCP::OnWsClose')]
text = '''// AO 用。本体から自動抽出するので直接編集しない。
#include "AUTOMAP_CORE_PATH"
namespace ClientAutoMapTest {
static BOOL g_bDraw;
static BYTE g_byTextFadeInOut;
static DWORD g_dwTimeTextFadeInOut;
static PMAPINFO g_pMapInfo;
static int g_nMyCharID;
struct Characters : std::vector<PCHARINFO> { int GetSize() const { return size(); } };
static Characters g_aCharInfo;
static BYTE dark[(DRAW_PARTS_X * 2 + 3) * (DRAW_PARTS_Y * 2 + 3)];
static BYTE *g_pMapDarkTmp = dark;
static BOOL IsMapViewMode() { return FALSE; }
static BOOL GetMapLightLevel() { return FALSE; }
static BOOL IsMapDarkFlg() { return FALSE; }
static BOOL CheckCharBlock(DWORD, DWORD) { return TRUE; }
static std::vector<std::vector<BYTE> > outgoing;
struct Sender { void Send(PBYTE p, DWORD n) { outgoing.push_back(std::vector<BYTE>(p, p+n)); } };
static Sender cliSock;
void AutoMapUpdate();
'''
text += state
text += '''
static DWORD deliveredSize;
static BOOL DispatchMessageForTest(HWND, UINT msg, WPARAM data, LPARAM size) {
    if (msg == WM_URARASOCK_RECV) {
        deliveredSize = size;
        PBYTE p = (PBYTE)data;
        // SboCliMain と同じく LPARAM を受信サイズとして渡す。
        if (*p == CMD_DUNGEONMAP) RecvDataProcDUNGEONMAP(p, size);
        delete[] p;
    }
    return TRUE;
}
class CUraraSockTCP {
public:
    DWORD m_dwRecvDataSize = 0;
    BOOL m_bPreCheck = TRUE;
    HWND m_hWndParent = (HWND)1;
    CCRC crc;
    CCRC *m_pCrc = &crc;
    CmyZlib m_Zlib;
    void RenewThrowghPut() {}
    void Destroy() { abort(); }
    void Send(PBYTE, DWORD) { abort(); }
    void OnWsMessage(const BYTE *, DWORD);
};
'''
text += "\n#define PostMessage ClientAutoMapTest::DispatchMessageForTest\n" + recv + "\n#undef PostMessage\n"
text += '''
static void Begin(PCHARINFO p, PMAPINFO map) {
    AutoMapReset(); outgoing.clear();
    g_pMapInfo = map; g_nMyCharID = 0;
    g_aCharInfo.clear(); g_aCharInfo.push_back(p);
}
static void Receive(const std::vector<BYTE> &body, BOOL compressed) {
    CUraraSockTCP socket;
    std::vector<BYTE> wire = body;
    if (compressed) {
        CmyZlib z;
        wire.resize(body.size() * 2 + 128);
        wire[0] = 0xff;
        DWORD size = body.size(); memcpy(&wire[1], &size, 4);
        z.DeflateInit();
        DWORD n = z.Deflate((PBYTE)&body[0], size, &wire[5], wire.size() - 5);
        z.DeflateEnd(); if (!n) abort(); wire.resize(n + 5);
    }
    URARASOCK_PACKETINFO info;
    info.dwSize = wire.size(); info.dwCRC = socket.crc.GetCRC(&wire[0], wire.size());
    std::vector<BYTE> frame(sizeof(info) + wire.size());
    memcpy(&frame[0], &info, sizeof(info)); memcpy(&frame[sizeof(info)], &wire[0], wire.size());
    socket.OnWsMessage(&frame[0], frame.size());
}
}
'''
text = text.replace("AUTOMAP_CORE_PATH", str(root / "SboCli/src/SboCliAutoMapCore.h"))
output.parent.mkdir(parents=True, exist_ok=True)
if not output.exists() or output.read_text() != text:
    output.write_text(text)
