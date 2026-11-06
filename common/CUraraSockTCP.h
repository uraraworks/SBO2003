/* ========================================================================= */
/* CUraraSockTCP.h（Web版・サーバーの Node.js 版・Mac/Linux版）				 */
/* 汎用TCP通信クラス：Windows 版と同じ使い方で、中身だけ差し替える			 */
/* ========================================================================= */
/*
 * Windows 版の CUraraSockTCP（WSAAsyncSelect + 隠しウィンドウ + スレッド）と
 * 同じ使い方ができるようにしたもの。パケットの形式
 * （8 バイトのヘッダ + CRC + zlib 圧縮、接続前チェック）は Windows 版と同じ。
 *
 *  - ブラウザ版（クライアント）: WebSocket でブリッジ（tools/web/bridge）につなぐ。
 *    実装は web/replace/CUraraSockTCP_web.cpp。
 *  - サーバー: Windows 版と同じウィンドウメッセージを親に送る。実装は server/CUraraSockTCP_svr.cpp。
 *    - Node.js 版: Node の net / http で TCP と WebSocket を受ける（server/node/sbosvr_net.js）。
 *    - Mac / Linux ネイティブ版: ふつうの TCP ソケットを PeekMessage のたびに poll で見回る。
 */
#ifndef CURARASOCKTCP_H
#define CURARASOCKTCP_H

#define URARASOCK_USEZLIB

#include "myZlib/myZlib.h"

class CCRC;

/* ========================================================================= */
/* 定数の定義（Windows 版と同じ）											 */
/* ========================================================================= */
#define WNDCLNAMESVR	"TCPMainWindowSvr"
#define WNDCLNAMECLI	"TCPMainWindowCli"
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }

#define URARASOCK_MAXCONNECT	1000
#define URARASOCK_IDBASE		100000
#define URARASOCK_SENDALL		0xFFFFFFFF
#define URARASOCK_PRECHECK		0x12BE46AC
#define URARASOCK_RECVBUFSIZE	(1024*64)
#define URARASOCK_USEZLIBSIZE	(128)
#define TIMERID_KEEPALIVE		0
#define TIMERID_KEEPALIVE_CLI	1
#define TIME_KEEPALIVE			60000
#define TIME_KEEPALIVE_CLI		30000

/* ウィンドウメッセージ */
enum {
	WM_SOCKADDQUE = WM_APP + 200,
	WM_SOCKCANCELQUE,
	WM_SOCKPRECHECK,
	WM_URARASOCK_HOST,
	WM_URARASOCK_ADDCLIENT,
	WM_URARASOCK_DECCLIENT,
	WM_URARASOCK_CONNECT,
	WM_URARASOCK_DISCONNECT,
	WM_URARASOCK_RECV,
	WM_URARASOCK_SEND,
	WM_SOCKEVENT,
};

/* 送信優先順位 */
enum URARASOCK_SENDPRIORITY {
	URARASOCK_SENDPRIORITY_HIGH = 0,
	URARASOCK_SENDPRIORITY_MIDDLE,
	URARASOCK_SENDPRIORITY_LOW,
};

/* 動作モード */
enum {
	URARASOCKMODE_SREVER = 0,
	URARASOCKMODE_CLIENT,
};

/* パケット情報 */
typedef struct _URARASOCK_PACKETINFO {
	DWORD					dwSize;								/* データサイズ */
	DWORD					dwCRC;								/* チェックサム */
} URARASOCK_PACKETINFO, *PURARASOCK_PACKETINFO;

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */
class CUraraSockTCP {
public:
	CUraraSockTCP(void);
	virtual ~CUraraSockTCP(void);

	void Destroy(void);										/* 後始末 */
	BOOL Host(HWND, WORD, DWORD);								/* 接続待ち開始（ブラウザでは不可） */
	BOOL Connect(HWND, WORD, LPSTR);							/* サーバーへ接続 */
	void DeleteClient(DWORD);									/* クライアントを切断（使わない） */
	void SendCancel(DWORD dwID = 0);							/* 送信キャンセル（送信は即時なので何もしない） */
	void Send(PBYTE, DWORD);									/* サーバーへデータ送信 */
	void Send(PBYTE, DWORD, BYTE);
	void Send(PBYTE, DWORD, DWORD);
	void Send(PBYTE, DWORD, DWORD, BYTE);
	void SendTo(DWORD, PBYTE, DWORD);
	void SendTo(DWORD, PBYTE, DWORD, BYTE);
	void SendTo(DWORD, PBYTE, DWORD, DWORD);
	void SendTo(DWORD, PBYTE, DWORD, DWORD, BYTE);
	/* 同じデータを複数の宛先へ送る（宛先を1つずつ SendTo した時と同じ結果。圧縮と CRC は1回だけ。サーバー用） */
	void SendToMulti(const DWORD *, DWORD, PBYTE, DWORD, DWORD, BYTE);
	DWORD GetThrowghPutSend(DWORD);							/* リンク・スループット量を取得 */
	DWORD GetThrowghPutRecv(DWORD);
	DWORD GetQueCount(DWORD);									/* キュー数を取得 */
	DWORD GetIPAddress(DWORD);									/* IPアドレスを取得 */

	/* WebSocket のイベント（web 実装の内部で使う） */
	void OnWsOpen(void);
	void OnWsMessage(const BYTE *pData, DWORD dwSize);
	void OnWsClose(void);

	/* Node.js からの知らせ（サーバーの Node.js 実装の内部で使う） */
	int  OnNetAccept(int nConn, DWORD dwAddr);
	void OnNetData(int nNo, const BYTE *pData, int nSize);
	void OnNetClose(int nNo);

private:
	void SendRaw(PBYTE pData, DWORD dwSize);					/* ヘッダを付けて送る */
	void RenewThrowghPut(void);
	PBYTE MakeSendData(PBYTE pData, DWORD &dwSize);				/* 必要なら圧縮する（new[] で返す） */
	void AcceptClients(void);									/* 新しい接続を受け付ける（サーバー用） */
	BOOL FlushSlot(int nNo);									/* 送信キューを流す（サーバー用） */
	void ReadSlot(int nNo);										/* 受信してパケットを取り出す（サーバー用） */
	void CloseSlot(int nNo);									/* 接続を閉じる（サーバー用） */
	void ReleaseSlot(int nNo);									/* スロットを空けて切断を知らせる（サーバー用） */
	void QueuePreCheck(int nNo);								/* 接続前チェックのキーワードを積む（サーバー用） */
	void ParseRecv(int nNo);									/* 受信データからパケットを切り出す（サーバー用） */

private:
	struct CNativeState	*m_pNative;								/* native 実装の状態（web では使わない） */
	CCRC		*m_pCrc;										/* CRC用オブジェクト */
	CmyZlib		m_Zlib;											/* 圧縮クラス */
	int			m_nSocket;										/* WebSocket のハンドル（0 なら無し） */
	HWND		m_hWndParent;									/* 通知先 */
	BOOL		m_bOpen;										/* WebSocket がつながっている */
	BOOL		m_bPreCheck;									/* 接続前チェックが済んだ */
	DWORD		m_dwSendDataSize,								/* １秒間の送信量 */
				m_dwRecvDataSize,								/* １秒間の受信量 */
				m_dwThrowghPutSend,
				m_dwThrowghPutRecv,
				m_dwTimeThrowghPut;
};

#endif
