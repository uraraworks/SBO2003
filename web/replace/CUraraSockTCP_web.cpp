/* ========================================================================= */
/* CUraraSockTCP_web.cpp													 */
/* 汎用TCP通信クラス（Web版）：WebSocket でブリッジにつなぐ					 */
/* ========================================================================= */
/*
 * 流れ
 *  1. Connect で ws://<サーバー>:<ポート> に接続する。
 *     Module.sboWsUrl が設定されていればそちらを使う（シェル HTML の ?ws= で指定）。
 *  2. ブリッジは TCP のデータをパケット（8 バイトのヘッダ + 中身）単位で
 *     WebSocket の 1 メッセージにして送ってくる。
 *  3. 最初のパケットは接続前チェック用の 4 バイト。キーワードを作って返し、
 *     WM_URARASOCK_CONNECT を通知する（Windows 版と同じ）。
 *  4. 以降は CRC を確かめ、圧縮されていれば展開して WM_URARASOCK_RECV で渡す。
 *     wParam は new[] で確保したデータ（受け取った側が delete[] する）。
 */
#include "StdAfx.h"
#include "common/CUraraSockTCP.h"
#include "common/crc.h"

#include <emscripten.h>
#include <emscripten/websocket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 接続先の URL を作る（呼んだ側で free する） */
EM_JS(char *, W32JsMakeWsUrl, (const char *pszAddr, int nPort), {
	var url = Module['sboWsUrl'];
	if (!url) {
		var proto = (typeof location !== 'undefined' && location.protocol === 'https:') ? 'wss' : 'ws';
		url = proto + '://' + UTF8ToString(pszAddr) + ':' + nPort;
	}
	return stringToNewUTF8(url);
});

/* サーバーから「このページは古い版」と言われた時（デプロイ前から開いたままのタブ）。
   古いクライアントのまま新しいサーバーにつなぐと、マップなどのデータの読み方がずれるので読み込み直す */
EM_JS(void, W32JsReloadForNewVersion, (void), {
	if (Module['sboReloading']) {
		return;
	}
	Module['sboReloading'] = true;
	if (Module['sboOnNewVersion']) {
		Module['sboOnNewVersion']();
	} else {
		location.reload();
	}
});

static EM_BOOL OnOpen(int, const EmscriptenWebSocketOpenEvent *, void *pUser)
{
	((CUraraSockTCP *)pUser)->OnWsOpen();
	return EM_TRUE;
}

static EM_BOOL OnMessage(int, const EmscriptenWebSocketMessageEvent *pEvent, void *pUser)
{
	if (!pEvent->isText) {
		((CUraraSockTCP *)pUser)->OnWsMessage(pEvent->data, pEvent->numBytes);
	} else if (pEvent->data && strcmp((const char *)pEvent->data, "SBO_RELOAD") == 0) {
		W32JsReloadForNewVersion();
	}
	return EM_TRUE;
}

static EM_BOOL OnClose(int, const EmscriptenWebSocketCloseEvent *, void *pUser)
{
	((CUraraSockTCP *)pUser)->OnWsClose();
	return EM_TRUE;
}

static EM_BOOL OnError(int, const EmscriptenWebSocketErrorEvent *, void *pUser)
{
	printf("[sock] WebSocket でエラーが発生しました\n");
	((CUraraSockTCP *)pUser)->OnWsClose();
	return EM_TRUE;
}

CUraraSockTCP::CUraraSockTCP(void)
{
	m_pCrc				= new CCRC;
	m_nSocket			= 0;
	m_hWndParent		= NULL;
	m_bOpen				= FALSE;
	m_bPreCheck			= FALSE;
	m_dwSendDataSize	= 0;
	m_dwRecvDataSize	= 0;
	m_dwThrowghPutSend	= 0;
	m_dwThrowghPutRecv	= 0;
	m_dwTimeThrowghPut	= 0;
}

CUraraSockTCP::~CUraraSockTCP(void)
{
	Destroy();
	SAFE_DELETE(m_pCrc);
}

void CUraraSockTCP::Destroy(void)
{
	if (m_nSocket > 0) {
		int nSocket = m_nSocket;
		m_nSocket = 0;
		emscripten_websocket_close(nSocket, 1000, "destroy");
		emscripten_websocket_delete(nSocket);
	}
	m_hWndParent	= NULL;
	m_bOpen			= FALSE;
	m_bPreCheck		= FALSE;
}

BOOL CUraraSockTCP::Host(HWND, WORD, DWORD)
{
	/* ブラウザはサーバーになれない */
	return FALSE;
}

BOOL CUraraSockTCP::Connect(HWND hWndParent, WORD wPort, LPSTR pszAddr)
{
	if (m_hWndParent || hWndParent == NULL || pszAddr == NULL) {
		return FALSE;
	}
	if (!emscripten_websocket_is_supported()) {
		return FALSE;
	}

	char *pszUrl = W32JsMakeWsUrl(pszAddr, (int)wPort);
	EmscriptenWebSocketCreateAttributes attr;
	emscripten_websocket_init_create_attributes(&attr);
	attr.url = pszUrl;
	attr.protocols = NULL;
	attr.createOnMainThread = EM_TRUE;
	m_nSocket = emscripten_websocket_new(&attr);
	printf("[sock] 接続開始: %s\n", pszUrl);
	free(pszUrl);
	if (m_nSocket <= 0) {
		m_nSocket = 0;
		return FALSE;
	}
	m_hWndParent	= hWndParent;
	m_bOpen			= FALSE;
	m_bPreCheck		= FALSE;

	emscripten_websocket_set_onopen_callback(m_nSocket, this, OnOpen);
	emscripten_websocket_set_onmessage_callback(m_nSocket, this, OnMessage);
	emscripten_websocket_set_onclose_callback(m_nSocket, this, OnClose);
	emscripten_websocket_set_onerror_callback(m_nSocket, this, OnError);
	return TRUE;
}

void CUraraSockTCP::DeleteClient(DWORD)
{
}

void CUraraSockTCP::SendCancel(DWORD)
{
	/* 送信キューを持たず、すぐ WebSocket に渡すので取り消すものは無い */
}

void CUraraSockTCP::Send(PBYTE pData, DWORD dwSize)
{
	SendTo(URARASOCK_IDBASE, pData, dwSize, 0, URARASOCK_SENDPRIORITY_MIDDLE);
}

void CUraraSockTCP::Send(PBYTE pData, DWORD dwSize, BYTE byPriority)
{
	SendTo(URARASOCK_IDBASE, pData, dwSize, 0, byPriority);
}

void CUraraSockTCP::Send(PBYTE pData, DWORD dwSize, DWORD dwTimeOut)
{
	SendTo(URARASOCK_IDBASE, pData, dwSize, dwTimeOut, URARASOCK_SENDPRIORITY_MIDDLE);
}

void CUraraSockTCP::Send(PBYTE pData, DWORD dwSize, DWORD dwTimeOut, BYTE byPriority)
{
	SendTo(URARASOCK_IDBASE, pData, dwSize, dwTimeOut, byPriority);
}

void CUraraSockTCP::SendTo(DWORD dwID, PBYTE pData, DWORD dwSize)
{
	SendTo(dwID, pData, dwSize, 0, URARASOCK_SENDPRIORITY_MIDDLE);
}

void CUraraSockTCP::SendTo(DWORD dwID, PBYTE pData, DWORD dwSize, BYTE byPriority)
{
	SendTo(dwID, pData, dwSize, 0, byPriority);
}

void CUraraSockTCP::SendTo(DWORD dwID, PBYTE pData, DWORD dwSize, DWORD dwTimeOut)
{
	SendTo(dwID, pData, dwSize, dwTimeOut, URARASOCK_SENDPRIORITY_MIDDLE);
}

void CUraraSockTCP::SendTo(DWORD, PBYTE pData, DWORD dwSize, DWORD, BYTE)
{
	if (pData == NULL || !m_bOpen) {
		return;
	}

	/* 圧縮する？（Windows 版と同じ判定・形式） */
	if (dwSize >= URARASOCK_USEZLIBSIZE) {
		LPBYTE pTmp = new BYTE[dwSize + 5];
		ZeroMemory(pTmp, dwSize + 5);
		m_Zlib.DeflateInit();
		DWORD dwRet = m_Zlib.Deflate(pData, dwSize, &pTmp[5], dwSize);
		m_Zlib.DeflateEnd();
		if (dwRet) {
			pTmp[0] = 0xFF;
			CopyMemory(&pTmp[1], &dwSize, 4);
			SendRaw(pTmp, dwRet + 5);
			SAFE_DELETE_ARRAY(pTmp);
			return;
		}
		SAFE_DELETE_ARRAY(pTmp);
	}
	SendRaw(pData, dwSize);
}

void CUraraSockTCP::SendRaw(PBYTE pData, DWORD dwSize)
{
	if (m_nSocket <= 0) {
		return;
	}
	DWORD dwTotal = sizeof(URARASOCK_PACKETINFO) + dwSize;
	PBYTE pFrame = new BYTE[dwTotal];
	PURARASOCK_PACKETINFO pInfo = (PURARASOCK_PACKETINFO)pFrame;
	pInfo->dwSize	= dwSize;
	pInfo->dwCRC	= m_pCrc->GetCRC(pData, dwSize);
	CopyMemory(pFrame + sizeof(URARASOCK_PACKETINFO), pData, dwSize);
	emscripten_websocket_send_binary(m_nSocket, pFrame, dwTotal);
	SAFE_DELETE_ARRAY(pFrame);

	m_dwSendDataSize += dwTotal;
	RenewThrowghPut();
}

void CUraraSockTCP::OnWsOpen(void)
{
	printf("[sock] 接続しました\n");
	m_bOpen = TRUE;
}

void CUraraSockTCP::OnWsMessage(const BYTE *pData, DWORD dwSize)
{
	m_dwRecvDataSize += dwSize;
	RenewThrowghPut();

	/* ブリッジは 1 メッセージに 1 パケットを入れてくるが、念のため続けて読む */
	DWORD dwPos = 0;
	while (dwPos + sizeof(URARASOCK_PACKETINFO) <= dwSize) {
		URARASOCK_PACKETINFO Info;
		CopyMemory(&Info, pData + dwPos, sizeof(Info));
		dwPos += sizeof(Info);
		if (Info.dwSize > dwSize - dwPos) {
			printf("[sock] パケットのサイズが不正です (%u > %u)\n", Info.dwSize, dwSize - dwPos);
			break;
		}
		const BYTE *pBody = pData + dwPos;
		dwPos += Info.dwSize;

		/* 接続前チェック */
		if (!m_bPreCheck) {
			if (Info.dwSize != sizeof(DWORD)) {
				printf("[sock] 接続前チェックのデータが不正です\n");
				Destroy();
				return;
			}
			DWORD dwKey;
			CopyMemory(&dwKey, pBody, sizeof(DWORD));
			dwKey &= URARASOCK_PRECHECK;
			dwKey *= URARASOCK_PRECHECK;
			m_bPreCheck = TRUE;
			Send((PBYTE)&dwKey, sizeof(DWORD));
			if (m_hWndParent) {
				PostMessage(m_hWndParent, WM_URARASOCK_CONNECT, 0, 0);
			}
			continue;
		}

		/* チェックサムが不一致なら切断（Windows 版と同じ） */
		if (Info.dwCRC == 0 || Info.dwCRC != m_pCrc->GetCRC((PBYTE)pBody, Info.dwSize)) {
			printf("[sock] チェックサムが一致しません\n");
			HWND hWndParent = m_hWndParent;
			Destroy();
			if (hWndParent) {
				PostMessage(hWndParent, WM_URARASOCK_DISCONNECT, 0, 0);
			}
			return;
		}

		PBYTE pRecv = NULL;
		if (Info.dwSize >= 5 && pBody[0] == 0xFF) {
			/* 圧縮されている：[0xFF][元のサイズ(4)][圧縮データ] */
			DWORD dwSourceSize;
			CopyMemory(&dwSourceSize, &pBody[1], 4);
			pRecv = new BYTE[dwSourceSize];
			m_Zlib.InflateInit();
			DWORD dwRet = m_Zlib.Inflate((PBYTE)&pBody[5], Info.dwSize - 5, pRecv, dwSourceSize);
			m_Zlib.InflateEnd();
			if (dwRet == 0) {
				/* 解凍に失敗したので無視する */
				SAFE_DELETE_ARRAY(pRecv);
				continue;
			}
		} else {
			pRecv = new BYTE[Info.dwSize ? Info.dwSize : 1];
			CopyMemory(pRecv, pBody, Info.dwSize);
		}
		if (m_hWndParent) {
			PostMessage(m_hWndParent, WM_URARASOCK_RECV, (WPARAM)pRecv, (LPARAM)URARASOCK_IDBASE);
		} else {
			SAFE_DELETE_ARRAY(pRecv);
		}
	}
}

void CUraraSockTCP::OnWsClose(void)
{
	if (m_nSocket <= 0) {
		return;
	}
	printf("[sock] 切断しました\n");
	HWND hWndParent = m_hWndParent;
	Destroy();
	if (hWndParent) {
		PostMessage(hWndParent, WM_URARASOCK_DISCONNECT, 0, 0);
	}
}

void CUraraSockTCP::RenewThrowghPut(void)
{
	DWORD dwTime = GetTickCount();
	if (dwTime - m_dwTimeThrowghPut >= 1000) {
		m_dwThrowghPutSend	= m_dwSendDataSize;
		m_dwThrowghPutRecv	= m_dwRecvDataSize;
		m_dwSendDataSize	= 0;
		m_dwRecvDataSize	= 0;
		m_dwTimeThrowghPut	= dwTime;
	}
}

DWORD CUraraSockTCP::GetThrowghPutSend(DWORD)	{ return m_dwThrowghPutSend; }
DWORD CUraraSockTCP::GetThrowghPutRecv(DWORD)	{ return m_dwThrowghPutRecv; }
DWORD CUraraSockTCP::GetQueCount(DWORD)			{ return 0; }
DWORD CUraraSockTCP::GetIPAddress(DWORD)		{ return 0; }
