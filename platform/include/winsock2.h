/* winsock2.h の代わり。型と定数だけ用意する（通信は CUraraSockTCP の Web 版 / Mac・Linux 版の実装が担当） */
#pragma once
#include "win32compat.h"

typedef UINT_PTR	SOCKET;
#define INVALID_SOCKET	((SOCKET)(~0))
#define SOCKET_ERROR	(-1)

typedef struct in_addr_w32 {
	union {
		struct { BYTE s_b1, s_b2, s_b3, s_b4; } S_un_b;
		struct { WORD s_w1, s_w2; } S_un_w;
		DWORD S_addr;
	} S_un;
} IN_ADDR, *PIN_ADDR, *LPIN_ADDR;

typedef struct sockaddr_in_w32 {
	short	sin_family;
	WORD	sin_port;
	IN_ADDR	sin_addr;
	char	sin_zero[8];
} SOCKADDR_IN, *PSOCKADDR_IN, *LPSOCKADDR_IN;

typedef struct sockaddr_w32 {
	WORD	sa_family;
	char	sa_data[14];
} SOCKADDR, *PSOCKADDR, *LPSOCKADDR;

typedef struct WSAData {
	WORD	wVersion;
	WORD	wHighVersion;
	char	szDescription[257];
	char	szSystemStatus[129];
	unsigned short	iMaxSockets;
	unsigned short	iMaxUdpDg;
	char	*lpVendorInfo;
} WSADATA, *LPWSADATA;

#ifndef AF_INET
#define AF_INET		2
#endif
#ifndef SOCK_STREAM
#define SOCK_STREAM	1
#endif
#ifndef IPPROTO_TCP
#define IPPROTO_TCP	6
#endif
#define FD_READ		0x01
#define FD_WRITE	0x02
#define FD_ACCEPT	0x08
#define FD_CONNECT	0x10
#define FD_CLOSE	0x20
#define WSAGETSELECTEVENT(l)	LOWORD(l)
#define WSAGETSELECTERROR(l)	HIWORD(l)

int		WSAStartup(WORD wVersion, LPWSADATA pData);
int		WSACleanup(void);
int		WSAGetLastError(void);
