#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
動いているサーバーに Windows 版クライアントと同じ形で TCP 接続し、
接続前チェックと最初の応答（CMD_CONNECT）が返ってくるかを確かめる。

使い方: tools/server/check_tcp.py [ホスト] [ポート]   （省略時は 127.0.0.1 2004）
"""
import socket
import struct
import sys
import zlib

URARASOCK_PRECHECK = 0x12BE46AC


def main():
	host = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
	port = int(sys.argv[2]) if len(sys.argv) > 2 else 2004
	s = socket.create_connection((host, port), timeout=5)

	def recv_exact(n):
		buf = b''
		while len(buf) < n:
			chunk = s.recv(n - len(buf))
			if not chunk:
				raise RuntimeError('サーバーに切断されました')
			buf += chunk
		return buf

	def recv_packet():
		size, crc = struct.unpack('<II', recv_exact(8))
		body = recv_exact(size)
		if crc != zlib.crc32(body):
			raise RuntimeError('CRC が一致しません')
		return body

	# 接続前チェック: キーワードを受け取り、決まった計算をして返す
	key = struct.unpack('<I', recv_packet())[0]
	ans = struct.pack('<I', ((key & URARASOCK_PRECHECK) * URARASOCK_PRECHECK) & 0xFFFFFFFF)
	s.sendall(struct.pack('<II', 4, zlib.crc32(ans)) + ans)

	# 最初の応答（接続応答）
	body = recv_packet()
	s.close()
	print('OK: 接続前チェックが通り、%d バイトの応答（コマンド 0x%02X）を受け取りました' % (len(body), body[0]))
	return 0


if __name__ == '__main__':
	try:
		sys.exit(main())
	except Exception as e:
		print('NG: %s' % e)
		sys.exit(1)
