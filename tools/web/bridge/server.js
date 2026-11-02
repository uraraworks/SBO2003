'use strict';
// SBOP2 の WebSocketBridge を SBO2003 用に持ってきたもの（既定の転送先ポートだけ 2004 に変更）

const net = require('net');
const WebSocket = require('ws');

// 設定（環境変数またはデフォルト値）
const WS_PORT = parseInt(process.env.WS_PORT || '9100', 10);
const TCP_HOST = process.env.TCP_HOST || '127.0.0.1';
const TCP_PORT = parseInt(process.env.TCP_PORT || '2004', 10);

const PACKETINFO_SIZE = 8;  // dwSize(4) + dwCRC(4)

console.log(`[Bridge] WebSocket待ち受け: ws://0.0.0.0:${WS_PORT}`);
console.log(`[Bridge] TCP転送先: ${TCP_HOST}:${TCP_PORT}`);

const wss = new WebSocket.Server({ port: WS_PORT });

wss.on('listening', () => {
    console.log(`[Bridge] 起動完了`);
});

wss.on('connection', (ws, req) => {
    const clientAddr = req.socket.remoteAddress;
    console.log(`[Bridge] WebSocket接続: ${clientAddr}`);

    // TCP接続を作成
    const tcp = new net.Socket();
    let tcpConnected = false;

    // TCP受信バッファ管理
    let recvBuf = Buffer.alloc(0);
    let needSize = PACKETINFO_SIZE;  // まずヘッダを読む
    let headerParsed = false;
    let packetPayloadSize = 0;

    tcp.connect(TCP_PORT, TCP_HOST, () => {
        tcpConnected = true;
        console.log(`[Bridge] TCP接続完了: ${TCP_HOST}:${TCP_PORT}`);
    });

    // TCP受信 → パケット境界を検出 → WebSocketフレームとして送信
    tcp.on('data', (chunk) => {
        recvBuf = Buffer.concat([recvBuf, chunk]);

        // バッファに十分なデータが溜まるまで繰り返し処理
        while (true) {
            if (!headerParsed) {
                // PACKETINFOヘッダ(8バイト)を待つ
                if (recvBuf.length < PACKETINFO_SIZE) {
                    break;
                }
                packetPayloadSize = recvBuf.readUInt32LE(0);  // dwSize
                headerParsed = true;
                needSize = PACKETINFO_SIZE + packetPayloadSize;
            }

            if (recvBuf.length < needSize) {
                break;  // ペイロードがまだ揃っていない
            }

            // 1パケット分のデータを切り出してWebSocketで送信
            const packet = recvBuf.slice(0, needSize);
            recvBuf = recvBuf.slice(needSize);

            if (ws.readyState === WebSocket.OPEN) {
                ws.send(packet);
            }

            // 次のパケットの準備
            headerParsed = false;
            needSize = PACKETINFO_SIZE;
            packetPayloadSize = 0;
        }
    });

    tcp.on('error', (err) => {
        console.log(`[Bridge] TCPエラー: ${err.message}`);
        if (ws.readyState === WebSocket.OPEN) {
            ws.close(1011, 'TCP error');
        }
    });

    tcp.on('close', () => {
        tcpConnected = false;
        console.log(`[Bridge] TCP切断`);
        if (ws.readyState === WebSocket.OPEN) {
            ws.close(1000, 'TCP closed');
        }
    });

    // WebSocket受信 → そのままTCPに転送
    ws.on('message', (data) => {
        if (tcpConnected && !tcp.destroyed) {
            // WebSocketバイナリフレームをそのままTCPに書き出し
            tcp.write(Buffer.from(data));
        }
    });

    ws.on('close', (code, reason) => {
        console.log(`[Bridge] WebSocket切断: code=${code}`);
        if (tcpConnected && !tcp.destroyed) {
            tcp.destroy();
        }
    });

    ws.on('error', (err) => {
        console.log(`[Bridge] WebSocketエラー: ${err.message}`);
        if (tcpConnected && !tcp.destroyed) {
            tcp.destroy();
        }
    });
});

wss.on('error', (err) => {
    console.error(`[Bridge] サーバーエラー: ${err.message}`);
});

process.on('SIGINT', () => {
    console.log('\n[Bridge] シャットダウン...');
    wss.close(() => {
        process.exit(0);
    });
});
