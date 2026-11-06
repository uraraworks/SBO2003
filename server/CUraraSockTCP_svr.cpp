/* ========================================================================= */
/* CUraraSockTCP_svr.cpp														 */
/* 汎用TCP通信クラス（サーバー用：Node.js 版）								 */
/* ========================================================================= */
/*
 * Windows 版は WSAAsyncSelect で隠しウィンドウにソケットイベントを送り、
 * 別スレッドのメッセージループで処理していた。ここでは同じことを
 * 1 スレッドで行う。
 *
 * 通信の下回りは server/node/sbosvr_net.js が Node の net / http で
 * TCP と WebSocket を受け、届いたデータを SboSockOnData() などで渡してくる。
 * WebSocket の相手（ブラウザ版クライアント）も TCP の相手と同じスロットで扱う。
 *
 * パケットの切り出し・接続前チェック・CRC・圧縮はこのファイルで行う。
 *  - 親ウィンドウへの通知は Windows 版と同じメッセージ・同じ引数で送る。
 *      WM_URARASOCK_ADDCLIENT  lParam = ID（接続前チェックが済んだ時）
 *      WM_URARASOCK_DECCLIENT  lParam = ID
 *      WM_URARASOCK_RECV       wParam = 受信データ（new[]、受け取った側が delete[]。
 *                              後ろは 0 埋めの余白付きで、本当の大きさは SvrTakeRecvSize で取り出す）
 *                              lParam = ID
 *    ID は URARASOCK_IDBASE + スロット番号。
 *  - 接続直後にサーバーから 4 バイトのキーワードを送り、
 *    (キーワード & URARASOCK_PRECHECK) * URARASOCK_PRECHECK が返ってきたら接続完了。
 *  - クライアントモード（Connect）は使わないので作っていない。
 */
#include <emscripten.h>
#include <stdio.h>
#include <vector>
#include <map>

#include "win32compat.h"
#include "common/CUraraSockTCP.h"
#include "common/crc.h"

/* 1 パケットの上限（管理者のマップ送信などでも数十 KB なので、余裕を見て 4MB）。
   これを超える大きさを名乗るパケットは、受信バッファを膨らませる嫌がらせとみなして切断する */
#define URARASOCK_MAXPACKETSIZE	(4 * 1024 * 1024)
/* 受信データの後ろに付ける 0 埋めの余白。短すぎるパケットが届いても、
   受信処理が構造体の大きさぶん読んだときに確保した範囲の外を読まないようにする */
#define URARASOCK_RECVPADDING	8192

/* 受け取った側に渡した受信データの本当の大きさ（SvrTakeRecvSize で取り出す） */
static std::map<const BYTE *, DWORD> s_mapRecvSize;

/* 受信データの本当の大きさを取り出す（取り出したら忘れる）。知らないデータなら 0 */
DWORD SvrTakeRecvSize(const BYTE *pData)
{
	std::map<const BYTE *, DWORD>::iterator it = s_mapRecvSize.find(pData);
	if (it == s_mapRecvSize.end()) {
		return 0;
	}
	DWORD dwSize = it->second;
	s_mapRecvSize.erase(it);
	return dwSize;
}

/* 受け取る側に渡すデータを作る（後ろを 0 で埋め、大きさを覚えておく） */
static PBYTE NewRecvData(DWORD dwSize)
{
	PBYTE pRet = new BYTE[dwSize + URARASOCK_RECVPADDING];
	ZeroMemory(pRet, dwSize + URARASOCK_RECVPADDING);
	s_mapRecvSize[pRet] = dwSize;
	return pRet;
}

/* スロット 1 つ分 */
struct CNativeSlot {
	int					nFd;					/* ソケット（Node.js 版は接続番号。-1 なら空き） */
	DWORD				dwAddr;					/* 相手の IP アドレス（ネットワークバイト順） */
	BOOL				bPreCheck;				/* 接続前チェックが済んだ */
	DWORD				dwPreCheck;				/* 送ったキーワード */
	BOOL				bClosing;				/* 送り終わったら閉じる */
	std::vector<BYTE>	aRecv;					/* 受信途中のデータ */
	std::vector<BYTE>	aSend;					/* 送信待ちのデータ */
	DWORD				dwSendSize, dwRecvSize;	/* この 1 秒の送受信量 */
	DWORD				dwThrowghPutSend, dwThrowghPutRecv;
	DWORD				dwTimeThrowghPut;
};

struct CNativeState {
	int							nListenFd;		/* 待ち受けソケット（Node.js 版は待ち受け中なら 1） */
	std::vector<CNativeSlot>	aSlot;
};

/* 見回り先（サーバーは 1 つしか作らない） */
static CUraraSockTCP *s_pPollTarget;

/* ------------------------------------------------------------------------- */
/* Node.js 側（server/node/sbosvr_net.js）の呼び出し							 */
/* ------------------------------------------------------------------------- */
EM_JS(int, W32JsSockListen, (int nPort), {
	return Module['sboNet'].listen(nPort) ? 1 : 0;
});
EM_JS(void, W32JsSockSend, (int nConn, const BYTE *pData, int nSize), {
	Module['sboNet'].send(nConn, HEAPU8.slice(pData, pData + nSize));
});
EM_JS(void, W32JsSockClose, (int nConn), {
	Module['sboNet'].close(nConn);
});
EM_JS(void, W32JsSockCloseAll, (void), {
	Module['sboNet'].closeAll();
});
/* いまつながっている数（SboSvr.cpp のメインループが使う） */
EM_JS(int, SvrNetConnCount, (void), {
	return Module['sboNet'].count();
});

/* Node.js から呼ばれる入口（接続・受信・切断） */
extern "C" {
EMSCRIPTEN_KEEPALIVE int SboSockOnAccept(int nConn, DWORD dwAddr)
{
	return s_pPollTarget ? s_pPollTarget->OnNetAccept(nConn, dwAddr) : -1;
}
EMSCRIPTEN_KEEPALIVE void SboSockOnData(int nNo, const BYTE *pData, int nSize)
{
	if (s_pPollTarget) {
		s_pPollTarget->OnNetData(nNo, pData, nSize);
	}
}
EMSCRIPTEN_KEEPALIVE void SboSockOnClose(int nNo)
{
	if (s_pPollTarget) {
		s_pPollTarget->OnNetClose(nNo);
	}
}
}

static void ResetSlot(CNativeSlot &Slot)
{
	Slot.nFd				= -1;
	Slot.dwAddr				= 0;
	Slot.bPreCheck			= FALSE;
	Slot.dwPreCheck			= 0;
	Slot.bClosing			= FALSE;
	Slot.aRecv.clear();
	Slot.aSend.clear();
	Slot.dwSendSize			= 0;
	Slot.dwRecvSize			= 0;
	Slot.dwThrowghPutSend	= 0;
	Slot.dwThrowghPutRecv	= 0;
	Slot.dwTimeThrowghPut	= 0;
}

static void RenewSlotThrowghPut(CNativeSlot &Slot)
{
	DWORD dwTime = GetTickCount();
	if (dwTime - Slot.dwTimeThrowghPut >= 1000) {
		Slot.dwThrowghPutSend	= Slot.dwSendSize;
		Slot.dwThrowghPutRecv	= Slot.dwRecvSize;
		Slot.dwSendSize			= 0;
		Slot.dwRecvSize			= 0;
		Slot.dwTimeThrowghPut	= dwTime;
	}
}


CUraraSockTCP::CUraraSockTCP(void)
{
	m_pCrc				= new CCRC;
	m_pNative			= new CNativeState;
	m_pNative->nListenFd = -1;
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
	SAFE_DELETE(m_pNative);
	SAFE_DELETE(m_pCrc);
}

void CUraraSockTCP::Destroy(void)
{
	if (m_pNative == NULL) {
		return;
	}
	if (m_pNative->nListenFd >= 0) {
		W32JsSockCloseAll();
	}
	for (size_t i = 0; i < m_pNative->aSlot.size(); i ++) {
		ResetSlot(m_pNative->aSlot[i]);
	}
	m_pNative->nListenFd = -1;
	if (s_pPollTarget == this) {
		s_pPollTarget = NULL;
	}
	m_hWndParent = NULL;
}

BOOL CUraraSockTCP::Host(HWND hWndParent, WORD wPort, DWORD dwCount)
{
	if (m_hWndParent || hWndParent == NULL) {
		return FALSE;
	}
	if (dwCount == 0 || dwCount > URARASOCK_MAXCONNECT) {
		dwCount = URARASOCK_MAXCONNECT;
	}
	m_pNative->aSlot.resize(dwCount);
	for (size_t i = 0; i < m_pNative->aSlot.size(); i ++) {
		ResetSlot(m_pNative->aSlot[i]);
	}
	m_hWndParent = hWndParent;
	s_pPollTarget = this;
	if (!W32JsSockListen(wPort)) {
		m_hWndParent = NULL;
		s_pPollTarget = NULL;
		return FALSE;
	}
	m_pNative->nListenFd = 1;
	PostMessage(m_hWndParent, WM_URARASOCK_HOST, 0, 0);
	return TRUE;
}

BOOL CUraraSockTCP::Connect(HWND, WORD, LPSTR)
{
	/* サーバー専用 */
	return FALSE;
}

void CUraraSockTCP::DeleteClient(DWORD dwID)
{
	if (dwID < URARASOCK_IDBASE) {
		return;
	}
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwNo >= m_pNative->aSlot.size()) {
		return;
	}
	CNativeSlot &Slot = m_pNative->aSlot[dwNo];
	if (Slot.nFd < 0) {
		return;
	}
	/* Windows 版は FD_CLOSE をポストしていたので、送信待ちを送ってから閉じる */
	Slot.bClosing = TRUE;
	FlushSlot((int)dwNo);
}

void CUraraSockTCP::SendCancel(DWORD dwID)
{
	if (dwID < URARASOCK_IDBASE) {
		return;
	}
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwNo < m_pNative->aSlot.size()) {
		m_pNative->aSlot[dwNo].aSend.clear();
	}
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

/* 必要なら圧縮する（Windows 版と同じ判定・形式）。戻り値は new[] */
PBYTE CUraraSockTCP::MakeSendData(PBYTE pData, DWORD &dwSize)
{
	if (dwSize >= URARASOCK_USEZLIBSIZE) {
		LPBYTE pTmp = new BYTE[dwSize + 5];
		ZeroMemory(pTmp, dwSize + 5);
		m_Zlib.DeflateInit();
		DWORD dwRet = m_Zlib.Deflate(pData, dwSize, &pTmp[5], dwSize);
		m_Zlib.DeflateEnd();
		if (dwRet) {
			pTmp[0] = 0xFF;
			CopyMemory(&pTmp[1], &dwSize, 4);
			dwSize = dwRet + 5;
			return pTmp;
		}
		SAFE_DELETE_ARRAY(pTmp);
	}
	PBYTE pRet = new BYTE[dwSize ? dwSize : 1];
	CopyMemory(pRet, pData, dwSize);
	return pRet;
}

/* 送信の横取り（管理者画面の「元に戻す」が、送信用に作ったデータを記録に使う。
   SvrSendCaptureBegin から SvrSendCaptureEnd までの間は誰にも送らず、最初の 1 パケットだけ取っておく） */
static BOOL s_bSendCapture = FALSE;
static std::vector<BYTE> s_aSendCapture;
static BOOL s_bSendCaptured = FALSE;

void SvrSendCaptureBegin(void)
{
	s_bSendCapture	= TRUE;
	s_bSendCaptured	= FALSE;
	s_aSendCapture.clear();
}

/* 横取りを終える。取れたら TRUE で、aDst に中身（圧縮前）を入れる */
BOOL SvrSendCaptureEnd(std::vector<BYTE> &aDst)
{
	s_bSendCapture = FALSE;
	aDst.swap(s_aSendCapture);
	s_aSendCapture.clear();
	return s_bSendCaptured;
}

void CUraraSockTCP::SendTo(DWORD dwID, PBYTE pData, DWORD dwSize, DWORD dwTimeOut, BYTE byPriority)
{
	SendToMulti(&dwID, 1, pData, dwSize, dwTimeOut, byPriority);
}

/* 宛先を 1 つずつ SendTo した時と同じ結果になる（宛先の順に積み、すぐ渡す）。
   圧縮・CRC・パケットの組み立ては 1 回だけ行って、全員で使い回す。
   単一宛先は ID からスロットを直接引き、全員宛て（URARASOCK_SENDALL）だけ全スロットを見る */
void CUraraSockTCP::SendToMulti(const DWORD *pIDs, DWORD dwCount, PBYTE pData, DWORD dwSize, DWORD, BYTE byPriority)
{
	if (dwCount == 0) {
		return;
	}
	if (s_bSendCapture) {
		if (pData && !s_bSendCaptured) {
			s_aSendCapture.assign(pData, pData + dwSize);
			s_bSendCaptured = TRUE;
		}
		return;
	}
	if (pData == NULL || m_pNative->nListenFd < 0) {
		return;
	}

	/* 送る先のスロット番号を、宛先の順に集める */
	std::vector<size_t> aDst;
	const size_t nSlots = m_pNative->aSlot.size();
	for (DWORD k = 0; k < dwCount; k ++) {
		DWORD dwID = pIDs[k];
		if (dwID == URARASOCK_SENDALL) {
			for (size_t i = 0; i < nSlots; i ++) {
				CNativeSlot &Slot = m_pNative->aSlot[i];
				/* 全員宛ては接続前チェックが済んだ相手だけ */
				if (Slot.nFd >= 0 && !Slot.bClosing && Slot.bPreCheck) {
					aDst.push_back(i);
				}
			}
		} else if (dwID >= URARASOCK_IDBASE && (dwID - URARASOCK_IDBASE) < nSlots) {
			size_t i = dwID - URARASOCK_IDBASE;
			CNativeSlot &Slot = m_pNative->aSlot[i];
			if (Slot.nFd >= 0 && !Slot.bClosing) {
				aDst.push_back(i);
			}
		}
	}
	if (aDst.empty()) {
		return;
	}

	DWORD dwSendSize = dwSize;
	PBYTE pSend = MakeSendData(pData, dwSendSize);

	/* パケット = ヘッダ(サイズ・CRC) + 中身 */
	URARASOCK_PACKETINFO Info;
	Info.dwSize	= dwSendSize;
	Info.dwCRC	= m_pCrc->GetCRC(pSend, dwSendSize);
	std::vector<BYTE> aPacket(sizeof(Info) + dwSendSize);
	CopyMemory(&aPacket[0], &Info, sizeof(Info));
	CopyMemory(&aPacket[sizeof(Info)], pSend, dwSendSize);
	SAFE_DELETE_ARRAY(pSend);

	for (size_t k = 0; k < aDst.size(); k ++) {
		size_t i = aDst[k];
		CNativeSlot &Slot = m_pNative->aSlot[i];
		if (byPriority == URARASOCK_SENDPRIORITY_HIGH && Slot.aSend.empty()) {
			Slot.aSend.insert(Slot.aSend.begin(), aPacket.begin(), aPacket.end());
		} else {
			Slot.aSend.insert(Slot.aSend.end(), aPacket.begin(), aPacket.end());
		}
		/* Node.js が送信を溜めてくれるので、すぐ渡す */
		FlushSlot((int)i);
	}
}

DWORD CUraraSockTCP::GetThrowghPutSend(DWORD dwID)
{
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwID < URARASOCK_IDBASE || dwNo >= m_pNative->aSlot.size()) {
		return 0;
	}
	return m_pNative->aSlot[dwNo].dwThrowghPutSend;
}

DWORD CUraraSockTCP::GetThrowghPutRecv(DWORD dwID)
{
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwID < URARASOCK_IDBASE || dwNo >= m_pNative->aSlot.size()) {
		return 0;
	}
	return m_pNative->aSlot[dwNo].dwThrowghPutRecv;
}

DWORD CUraraSockTCP::GetQueCount(DWORD dwID)
{
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwID < URARASOCK_IDBASE || dwNo >= m_pNative->aSlot.size()) {
		return 0;
	}
	return m_pNative->aSlot[dwNo].aSend.empty() ? 0 : 1;
}

DWORD CUraraSockTCP::GetIPAddress(DWORD dwID)
{
	DWORD dwNo = dwID - URARASOCK_IDBASE;
	if (dwID < URARASOCK_IDBASE || dwNo >= m_pNative->aSlot.size()) {
		return 0;
	}
	return m_pNative->aSlot[dwNo].dwAddr;
}

/* ------------------------------------------------------------------------- */
/* Node.js 版の受け口															 */
/* ------------------------------------------------------------------------- */
/* 新しい接続。空きスロットの番号を返す（満員なら -1 で、JS 側が閉じる） */
int CUraraSockTCP::OnNetAccept(int nConn, DWORD dwAddr)
{
	int nNo = -1;
	for (size_t i = 0; i < m_pNative->aSlot.size(); i ++) {
		if (m_pNative->aSlot[i].nFd < 0) {
			nNo = (int)i;
			break;
		}
	}
	if (nNo < 0) {
		return -1;
	}
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	ResetSlot(Slot);
	Slot.nFd		= nConn;
	Slot.dwAddr		= dwAddr;
	Slot.dwPreCheck	= GetTickCount();
	QueuePreCheck(nNo);
	FlushSlot(nNo);
	return nNo;
}

void CUraraSockTCP::OnNetData(int nNo, const BYTE *pData, int nSize)
{
	if (nNo < 0 || (size_t)nNo >= m_pNative->aSlot.size() || m_pNative->aSlot[nNo].nFd < 0) {
		return;
	}
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	Slot.aRecv.insert(Slot.aRecv.end(), pData, pData + nSize);
	Slot.dwRecvSize += (DWORD)nSize;
	RenewSlotThrowghPut(Slot);
	ParseRecv(nNo);
}

/* 相手から切られた（JS 側のソケットはもう閉じている） */
void CUraraSockTCP::OnNetClose(int nNo)
{
	if (nNo < 0 || (size_t)nNo >= m_pNative->aSlot.size() || m_pNative->aSlot[nNo].nFd < 0) {
		return;
	}
	m_pNative->aSlot[nNo].bClosing = FALSE;
	ReleaseSlot(nNo);
}


/* 接続確認用キーワードを送信キューに積む（接続前チェック中なので SendTo は使わない） */
void CUraraSockTCP::QueuePreCheck(int nNo)
{
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	DWORD dwData = Slot.dwPreCheck;
	URARASOCK_PACKETINFO Info;
	Info.dwSize	= sizeof(DWORD);
	Info.dwCRC	= m_pCrc->GetCRC((PBYTE)&dwData, sizeof(DWORD));
	Slot.aSend.insert(Slot.aSend.end(), (BYTE *)&Info, (BYTE *)&Info + sizeof(Info));
	Slot.aSend.insert(Slot.aSend.end(), (BYTE *)&dwData, (BYTE *)&dwData + sizeof(DWORD));
}

BOOL CUraraSockTCP::FlushSlot(int nNo)
{
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	if (!Slot.aSend.empty()) {
		W32JsSockSend(Slot.nFd, &Slot.aSend[0], (int)Slot.aSend.size());
		Slot.dwSendSize += (DWORD)Slot.aSend.size();
		RenewSlotThrowghPut(Slot);
		Slot.aSend.clear();
	}
	if (Slot.bClosing) {
		CloseSlot(nNo);
		return FALSE;
	}
	return TRUE;
}


/* たまったデータからパケットを切り出す */
void CUraraSockTCP::ParseRecv(int nNo)
{
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	size_t nPos = 0;
	while (Slot.aRecv.size() - nPos >= sizeof(URARASOCK_PACKETINFO)) {
		URARASOCK_PACKETINFO Info;
		CopyMemory(&Info, &Slot.aRecv[nPos], sizeof(Info));
		/* 接続前チェック中は 4 バイト以外を受け付けない（Windows 版と同じ） */
		if (!Slot.bPreCheck && Info.dwSize != sizeof(DWORD)) {
			CloseSlot(nNo);
			return;
		}
		/* 大きすぎるパケットは受け取らない（届くまでバッファが膨らみ続けるので） */
		if (Info.dwSize > URARASOCK_MAXPACKETSIZE) {
			printf("[sock] パケットが大きすぎます (ID:%u %u bytes)\n", (unsigned)(nNo + URARASOCK_IDBASE), (unsigned)Info.dwSize);
			CloseSlot(nNo);
			return;
		}
		if (Slot.aRecv.size() - nPos - sizeof(Info) < Info.dwSize) {
			break;
		}
		const BYTE *pBody = &Slot.aRecv[nPos + sizeof(Info)];
		nPos += sizeof(Info) + Info.dwSize;

		if (!Slot.bPreCheck) {
			DWORD dwData, dwKey = Slot.dwPreCheck;
			CopyMemory(&dwData, pBody, sizeof(DWORD));
			dwKey &= URARASOCK_PRECHECK;
			dwKey *= URARASOCK_PRECHECK;
			if (dwData != dwKey) {
				printf("[sock] 接続前チェックが一致しません (ID:%u)\n", (unsigned)(nNo + URARASOCK_IDBASE));
				CloseSlot(nNo);
				return;
			}
			Slot.bPreCheck = TRUE;
			PostMessage(m_hWndParent, WM_URARASOCK_ADDCLIENT, 0, (LPARAM)(nNo + URARASOCK_IDBASE));
			continue;
		}

		/* 中身が空のパケットはキープアライブ */
		if (Info.dwSize == 0) {
			continue;
		}
		/* チェックサムが不一致なら切断（Windows 版と同じ） */
		if (Info.dwCRC == 0 || Info.dwCRC != m_pCrc->GetCRC((PBYTE)pBody, Info.dwSize)) {
			printf("[sock] チェックサムが一致しません (ID:%u)\n", (unsigned)(nNo + URARASOCK_IDBASE));
			CloseSlot(nNo);
			return;
		}

		PBYTE pRecv = NULL;
		if (Info.dwSize >= 5 && pBody[0] == 0xFF) {
			/* 圧縮されている：[0xFF][元のサイズ(4)][圧縮データ] */
			DWORD dwSourceSize;
			CopyMemory(&dwSourceSize, &pBody[1], 4);
			/* 展開後の大きさも上限を超えたら切断（巨大な確保でサーバーごと止まらないように） */
			if (dwSourceSize == 0 || dwSourceSize > URARASOCK_MAXPACKETSIZE) {
				printf("[sock] 展開後の大きさが不正です (ID:%u %u bytes)\n", (unsigned)(nNo + URARASOCK_IDBASE), (unsigned)dwSourceSize);
				CloseSlot(nNo);
				return;
			}
			pRecv = NewRecvData(dwSourceSize);
			m_Zlib.InflateInit();
			DWORD dwRet = m_Zlib.Inflate((PBYTE)&pBody[5], Info.dwSize - 5, pRecv, dwSourceSize);
			m_Zlib.InflateEnd();
			if (dwRet == 0) {
				SvrTakeRecvSize(pRecv);
				SAFE_DELETE_ARRAY(pRecv);
				continue;
			}
			s_mapRecvSize[pRecv] = dwRet;
		} else {
			pRecv = NewRecvData(Info.dwSize);
			CopyMemory(pRecv, pBody, Info.dwSize);
		}
		PostMessage(m_hWndParent, WM_URARASOCK_RECV, (WPARAM)pRecv, (LPARAM)(nNo + URARASOCK_IDBASE));
	}
	if (nPos > 0) {
		Slot.aRecv.erase(Slot.aRecv.begin(), Slot.aRecv.begin() + nPos);
	}
}

/* こちらから閉じる */
void CUraraSockTCP::CloseSlot(int nNo)
{
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	if (Slot.nFd < 0) {
		return;
	}
	W32JsSockClose(Slot.nFd);
	ReleaseSlot(nNo);
}

/* スロットを空けて、ゲーム側に切断を知らせる */
void CUraraSockTCP::ReleaseSlot(int nNo)
{
	CNativeSlot &Slot = m_pNative->aSlot[nNo];
	if (Slot.nFd < 0) {
		return;
	}
	BOOL bNotify = Slot.bPreCheck;
	ResetSlot(Slot);
	printf("[sock] 切断 (ID:%u)\n", (unsigned)(nNo + URARASOCK_IDBASE));
	/* 接続前チェックが済んでいない相手は、ゲーム側にはまだ知らせていない */
	if (bNotify && m_hWndParent) {
		PostMessage(m_hWndParent, WM_URARASOCK_DECCLIENT, 0, (LPARAM)(nNo + URARASOCK_IDBASE));
	}
}

/* クライアント側の処理（サーバーでは使わない） */
void CUraraSockTCP::OnWsOpen(void)								{}
void CUraraSockTCP::OnWsMessage(const BYTE *, DWORD)				{}
void CUraraSockTCP::OnWsClose(void)								{}
void CUraraSockTCP::SendRaw(PBYTE, DWORD)						{}
void CUraraSockTCP::RenewThrowghPut(void)						{}
