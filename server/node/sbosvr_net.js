/* ========================================================================= */
/* sbosvr_net.js																 */
/* Node.js 版サーバーの通信・HTTP（Emscripten の --pre-js で組み込む）			 */
/* ========================================================================= */
/*
 * Windows 版の SboSvr は TCP（既定 2004 番）だけを受けていた。Node.js 版では
 *   - TCP           … Windows 版クライアント（SboCli.exe）用。ゲームの INI のポート
 *   - HTTP          … ブラウザ版クライアントのファイル（out/web/dist）を配る
 *   - WebSocket     … ブラウザ版クライアントのゲーム通信（HTTP と同じポートの /ws）
 * を 1 つのプロセスで受ける。WebSocket の中身は TCP と同じバイト列
 * （8 バイトのヘッダ + 本体）なので、C 側（CUraraSockTCP_svr.cpp）は区別しない。
 *
 * C 側とのやり取り
 *   C → JS : Module.sboNet.listen / send / close / closeAll
 *   JS → C : _SboSockOnAccept(接続番号, IP) → スロット番号（満員なら -1）
 *            _SboSockOnData(スロット番号, ポインタ, 長さ)
 *            _SboSockOnClose(スロット番号)
 * HTTP のポートと配るフォルダは C の main が Module.sboNet.config() で渡す。
 */
Module['sboNet'] = (function () {
	var net = require('net');
	var http = require('http');
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var aConn = {};			/* 接続番号 → { sock, ws, slot, closing } */
	var nNextConn = 1;
	var aServer = [];
	var nHttpPort = 0;
	var strWwwDir = '';
	/* WebSocket の 1 メッセージの上限（ゲーム側のパケット上限 4MB＋ヘッダに余裕を持たせた値） */
	var WS_MAX_MESSAGE = 4 * 1024 * 1024 + 1024;

	var MIME = {
		'.html': 'text/html; charset=utf-8',
		'.js': 'text/javascript; charset=utf-8',
		'.wasm': 'application/wasm',
		'.data': 'application/octet-stream',
		'.css': 'text/css; charset=utf-8',
		'.png': 'image/png',
		'.ico': 'image/x-icon',
		'.json': 'application/json'
	};

	function log(msg) {
		console.log('[net] ' + msg);
	}

	/* "1.2.3.4" → in_addr.s_addr と同じ並び（リトルエンディアンの DWORD） */
	function addrToDword(addr) {
		addr = String(addr || '').replace(/^::ffff:/, '');
		if (addr === '::1') {
			addr = '127.0.0.1';
		}
		var a = addr.split('.');
		if (a.length !== 4) {
			return 0;
		}
		return ((a[0] & 255) | ((a[1] & 255) << 8) | ((a[2] & 255) << 16) | ((a[3] & 255) << 24)) >>> 0;
	}

	/* 新しい接続を C 側に知らせる。満員なら閉じる */
	function accept(conn) {
		var id = nNextConn++;
		aConn[id] = conn;
		conn.id = id;
		var slot = _SboSockOnAccept(id, addrToDword(conn.addr));
		if (slot < 0) {
			log('満員のため切断: ' + conn.addr);
			delete aConn[id];
			conn.kill();
			return false;
		}
		conn.slot = slot;
		log('接続: ' + conn.addr + ' (' + conn.kind + ', ID:' + (100000 + slot) + ')');
		return true;
	}

	function onData(conn, buf) {
		if (conn.slot < 0 || conn.closing || buf.length === 0) {
			return;
		}
		var p = _malloc(buf.length);
		HEAPU8.set(buf, p);
		try {
			_SboSockOnData(conn.slot, p, buf.length);
		} finally {
			_free(p);
		}
	}

	/* 相手から切られた */
	function onClose(conn) {
		if (aConn[conn.id] !== conn) {
			return;
		}
		delete aConn[conn.id];
		if (!conn.closing && conn.slot >= 0) {
			log('切断: ' + conn.addr + ' (ID:' + (100000 + conn.slot) + ')');
			_SboSockOnClose(conn.slot);
		}
	}

	/* ------------------------------------------------------------------ */
	/* TCP（Windows 版クライアント）											 */
	/* ------------------------------------------------------------------ */
	function onTcp(sock) {
		sock.setNoDelay(true);
		var conn = {
			kind: 'TCP', addr: sock.remoteAddress, slot: -1, closing: false,
			send: function (u8) { sock.write(u8); },
			end: function () { sock.end(); },
			kill: function () { sock.destroy(); }
		};
		sock.on('data', function (buf) { onData(conn, buf); });
		sock.on('error', function () {});
		sock.on('close', function () { onClose(conn); });
		accept(conn);
	}

	/* ------------------------------------------------------------------ */
	/* WebSocket（ブラウザ版クライアント）										 */
	/* ------------------------------------------------------------------ */
	function wsFrame(opcode, payload) {
		var len = payload.length, head;
		if (len < 126) {
			head = Buffer.from([0x80 | opcode, len]);
		} else if (len < 65536) {
			head = Buffer.from([0x80 | opcode, 126, len >> 8, len & 255]);
		} else {
			head = Buffer.alloc(10);
			head[0] = 0x80 | opcode;
			head[1] = 127;
			head.writeUInt32BE(Math.floor(len / 4294967296), 2);
			head.writeUInt32BE(len >>> 0, 6);
		}
		return Buffer.concat([head, Buffer.from(payload)]);
	}

	function onUpgrade(req, sock, head) {
		var key = req.headers['sec-websocket-key'];
		if (!key || String(req.headers['upgrade'] || '').toLowerCase() !== 'websocket') {
			sock.destroy();
			return;
		}
		var accept_ = crypto.createHash('sha1').update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest('base64');
		var lines = [
			'HTTP/1.1 101 Switching Protocols',
			'Upgrade: websocket',
			'Connection: Upgrade',
			'Sec-WebSocket-Accept: ' + accept_
		];
		/* ブラウザがサブプロトコル（Emscripten は "binary"）を出してきたら同じものを返す */
		var proto = req.headers['sec-websocket-protocol'];
		if (proto) {
			lines.push('Sec-WebSocket-Protocol: ' + String(proto).split(',')[0].trim());
		}
		sock.write(lines.join('\r\n') + '\r\n\r\n');
		sock.setNoDelay(true);

		var addr = req.headers['x-forwarded-for'] ? String(req.headers['x-forwarded-for']).split(',')[0].trim() : sock.remoteAddress;
		var conn = {
			kind: 'WebSocket', addr: addr, slot: -1, closing: false,
			send: function (u8) { sock.write(wsFrame(2, u8)); },
			end: function () {
				try { sock.write(wsFrame(8, Buffer.from([0x03, 0xE8]))); } catch (e) {}
				sock.end();
			},
			kill: function () { sock.destroy(); }
		};
		var rest = head && head.length ? Buffer.from(head) : Buffer.alloc(0);
		var aFrag = [], nFragSize = 0;
		sock.on('data', function (buf) {
			rest = rest.length ? Buffer.concat([rest, buf]) : buf;
			while (rest.length >= 2) {
				var fin = rest[0] & 0x80, op = rest[0] & 0x0F;
				var masked = rest[1] & 0x80, len = rest[1] & 0x7F, pos = 2;
				if (len === 126) {
					if (rest.length < 4) { return; }
					len = rest.readUInt16BE(2);
					pos = 4;
				} else if (len === 127) {
					if (rest.length < 10) { return; }
					len = rest.readUInt32BE(2) * 4294967296 + rest.readUInt32BE(6);
					pos = 10;
				}
				/* 大きすぎるフレームは、届くまでメモリを使い続けるので切断する */
				if (len > WS_MAX_MESSAGE || nFragSize + len > WS_MAX_MESSAGE) {
					conn.kill();
					return;
				}
				var mask = null;
				if (masked) {
					if (rest.length < pos + 4) { return; }
					mask = rest.slice(pos, pos + 4);
					pos += 4;
				}
				if (rest.length < pos + len) {
					return;
				}
				var payload = Buffer.from(rest.slice(pos, pos + len));
				rest = rest.slice(pos + len);
				if (mask) {
					for (var i = 0; i < payload.length; i++) {
						payload[i] ^= mask[i & 3];
					}
				}
				if (op === 8) {			/* 閉じる */
					conn.end();
					return;
				} else if (op === 9) {	/* ping → pong */
					sock.write(wsFrame(10, payload));
				} else if (op === 0 || op === 1 || op === 2) {
					aFrag.push(payload);
					nFragSize += payload.length;
					if (fin) {
						onData(conn, aFrag.length === 1 ? aFrag[0] : Buffer.concat(aFrag));
						aFrag = [];
						nFragSize = 0;
					}
				}
			}
		});
		sock.on('error', function () {});
		sock.on('close', function () { onClose(conn); });
		accept(conn);
	}

	/* ------------------------------------------------------------------ */
	/* HTTP（ブラウザ版クライアントのファイルを配る）							 */
	/* ------------------------------------------------------------------ */
	/*
	 * キャッシュの考え方
	 *   ページ（.html）は毎回確かめる（no-cache + ETag）。その代わりページの中に
	 *   各ファイルの版（中身のハッシュ）を書き込み、ページからは「名前?v=版」で読ませる。
	 *   版の付いた URL は中身が変わらないので、ブラウザに 1 年覚えさせる（immutable）。
	 *   リロードしても .wasm / .data / サウンドフォントは問い合わせすら起きない。
	 *   ビルドし直して中身が変わったら版も変わるので、新しい URL として読み直される。
	 */
	var aHashCache = {};		/* フルパス → { mtime, size, hash } */
	var VER_EXT = { '.js': 1, '.wasm': 1, '.data': 1, '.sf3': 1, '.css': 1, '.png': 1, '.ico': 1 };

	/* ファイルの中身のハッシュ（更新日時と大きさが同じ間は覚えておく） */
	function fileHash(file, st, data) {
		var c = aHashCache[file];
		if (c && c.mtime === st.mtimeMs && c.size === st.size) {
			return c.hash;
		}
		var hash = crypto.createHash('sha1').update(data || fs.readFileSync(file)).digest('hex').slice(0, 16);
		aHashCache[file] = { mtime: st.mtimeMs, size: st.size, hash: hash };
		return hash;
	}

	/* 配るフォルダの直下と 1 段下（bgm/ など）のファイルの版の一覧 { "sbocli.wasm": "…", "bgm/GeneralUserGS.sf3": "…" } */
	function assetVersions() {
		var ver = {};
		function scan(dir, prefix, depth) {
			var names;
			try {
				names = fs.readdirSync(dir);
			} catch (e) {
				return;
			}
			names.forEach(function (name) {
				var full = path.join(dir, name);
				var st;
				try {
					st = fs.statSync(full);
				} catch (e) {
					return;
				}
				if (st.isDirectory()) {
					if (depth > 0) {
						scan(full, prefix + name + '/', depth - 1);
					}
				} else if (VER_EXT[path.extname(name).toLowerCase()]) {
					ver[prefix + name] = fileHash(full, st);
				}
			});
		}
		scan(strWwwDir, '', 1);
		return ver;
	}

	/* ページに版の一覧を書き込み、<script src="…"> にも版を付ける */
	function rewriteHtml(html, ver) {
		html = html.replace(/(<script[^>]*\ssrc=")([^":?#]+\.js)(")/g, function (all, a, name, b) {
			return ver[name] ? a + name + '?v=' + ver[name] + b : all;
		});
		/* ブラウザ版の接続先の既定を「このサーバーの /ws」にする（?ws= があればそちらが優先） */
		return html.replace('</head>',
			'<script>window.SBO_DEFAULT_WS = location.origin.replace(/^http/, "ws") + "/ws";\n' +
			'window.SBO_ASSET_VER = ' + JSON.stringify(ver) + ';</script>\n</head>');
	}

	function onHttp(req, res) {
		var aUrl = String(req.url || '/').split('?');
		var url = decodeURIComponent(aUrl[0]);
		var query = new URLSearchParams(aUrl.slice(1).join('?'));
		/* サーバー状況（/health・/api/status。sbosvr_status.js）。/health はデプロイの確認にも使う */
		if (Module['sboStatus'] && Module['sboStatus'].handleHttp(req, res, url)) {
			return;
		}
		if (url === '/status') {
			url = '/status.html';
		}
		/* アカウント関係の API（sbosvr_account.js） */
		if (Module['sboAccount'] && Module['sboAccount'].handleHttp(req, res, url)) {
			return;
		}
		if (url === '/') {
			url = '/sbocli.html';
		}
		var file = path.normalize(path.join(strWwwDir, url));
		if (!strWwwDir || file.indexOf(path.normalize(strWwwDir)) !== 0) {
			res.writeHead(403);
			res.end();
			return;
		}
		fs.stat(file, function (errStat, st) {
			if (errStat || !st.isFile()) {
				res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
				res.end('見つかりません: ' + url + (fs.existsSync(strWwwDir) ? '' : '\n（先に tools/web/build.sh でブラウザ版をビルドしてください）'));
				return;
			}
			var ext = path.extname(file).toLowerCase();
			var headers = { 'Content-Type': MIME[ext] || 'application/octet-stream' };
			var ifNoneMatch = String(req.headers['if-none-match'] || '').split(/\s*,\s*/);

			if (ext === '.html') {
				fs.readFile(file, function (err, data) {
					if (err) {
						res.writeHead(500);
						res.end();
						return;
					}
					data = Buffer.from(rewriteHtml(String(data), assetVersions()));
					var etag = '"' + crypto.createHash('sha1').update(data).digest('hex').slice(0, 16) + '"';
					send(data, etag, 'no-cache');
				});
				return;
			}

			/* 中身のハッシュは覚えておくので、304 を返すだけならファイルを読まずに済む */
			var cached = aHashCache[file];
			if (cached && cached.mtime === st.mtimeMs && cached.size === st.size) {
				if (reply304('"' + cached.hash + '"', cached.hash)) {
					return;
				}
			}
			fs.readFile(file, function (err, data) {
				if (err) {
					res.writeHead(500);
					res.end();
					return;
				}
				var hash = fileHash(file, st, data);
				if (reply304('"' + hash + '"', hash)) {
					return;
				}
				send(data, '"' + hash + '"', cacheControl(hash));
			});

			/* ?v= が今の中身の版と同じなら長く覚えさせる。違う・無い時は毎回確かめさせる */
			function cacheControl(hash) {
				return query.get('v') === hash ? 'public, max-age=31536000, immutable' : 'no-cache';
			}
			/* 中身が変わっていなければ 304 を返す */
			function reply304(etag, hash) {
				if (ifNoneMatch.indexOf(etag) < 0) {
					return false;
				}
				res.writeHead(304, { 'ETag': etag, 'Cache-Control': hash ? cacheControl(hash) : 'no-cache' });
				res.end();
				return true;
			}
			function send(data, etag, cc) {
				if (reply304(etag, null)) {
					return;
				}
				headers['Cache-Control'] = cc;
				headers['ETag'] = etag;
				headers['Content-Length'] = data.length;
				res.writeHead(200, headers);
				res.end(req.method === 'HEAD' ? undefined : data);
			}
		});
	}

	return {
		/* HTTP のポートと配るフォルダ（0 なら HTTP・WebSocket は使わない） */
		config: function (port, www) {
			nHttpPort = port;
			strWwwDir = www ? path.resolve(www) : '';
		},
		listen: function (port) {
			var tcp = net.createServer(onTcp);
			tcp.on('error', function (e) {
				console.error('[net] TCP ポート ' + port + ' で待ち受けできません: ' + e.message);
				process.exit(1);
			});
			tcp.listen(port, function () {
				log('TCP ポート ' + port + ' で待ち受けを開始しました（Windows 版クライアント用）');
			});
			aServer.push(tcp);
			if (nHttpPort) {
				var web = http.createServer(onHttp);
				web.on('upgrade', onUpgrade);
				web.on('error', function (e) {
					console.error('[net] HTTP ポート ' + nHttpPort + ' で待ち受けできません: ' + e.message);
					process.exit(1);
				});
				web.listen(nHttpPort, function () {
					log('HTTP ポート ' + nHttpPort + ' で待ち受けを開始しました');
					if (Module['sboStatus']) {
						Module['sboStatus'].start();
					}
					log('  ブラウザ版: http://localhost:' + nHttpPort + '/  （WebSocket は ws://localhost:' + nHttpPort + '/ws）');
				});
				aServer.push(web);
			}
			return true;
		},
		send: function (id, u8) {
			var conn = aConn[id];
			if (conn && !conn.closing) {
				conn.send(u8);
			}
		},
		/* こちらから閉じる（C 側はもうスロットを空けている） */
		close: function (id) {
			var conn = aConn[id];
			if (conn) {
				conn.closing = true;
				delete aConn[id];
				conn.end();
				setTimeout(function () { conn.kill(); }, 3000).unref();
			}
		},
		/* いまつながっている数（接続前チェック中も含む）。誰もいない時にメインループを長めに休ませるのに使う */
		count: function () {
			return Object.keys(aConn).length;
		},
		closeAll: function () {
			Object.keys(aConn).forEach(function (id) {
				aConn[id].closing = true;
				aConn[id].kill();
			});
			aConn = {};
			aServer.forEach(function (s) { s.close(); });
			aServer = [];
		}
	};
})();

/* Ctrl+C などで止める時は、Windows 版で「終了」を選んだ時と同じくデータを保存してから終わる */
(function () {
	var nCount = 0;
	function onSignal(sig) {
		nCount++;
		if (nCount > 1) {
			console.log('[SboSvr] 保存を待たずに終了します');
			process.exit(1);
		}
		console.log('[SboSvr] ' + sig + ' を受けたので、データを保存して終了します（もう一度押すとすぐ終了）');
		_SboSvrRequestClose();
	}
	process.on('SIGINT', function () { onSignal('SIGINT'); });
	process.on('SIGTERM', function () { onSignal('SIGTERM'); });

	/*
	 * 停止ファイル（環境変数 SBO2003_STOP_FILE）が置かれたら、シグナルと同じく保存して終わる。
	 * Windows ではタスクスケジューラから起こしたプロセスに Ctrl+C を送れず、強制終了だと保存されないため
	 * （本番 VPS の deploy/production/stop.ps1 が使う）。ファイルは消さない（起動スクリプトが
	 * 「止めてほしくて止まった」のか「落ちた」のかを見分けるのに使う）。
	 */
	var strStopFile = process.env['SBO2003_STOP_FILE'];
	if (strStopFile) {
		var fs = require('fs');
		var timer = setInterval(function () {
			if (fs.existsSync(strStopFile)) {
				clearInterval(timer);
				onSignal('停止ファイル');
			}
		}, 1000);
		timer.unref();
	}
})();

Module['onExit'] = function (code) {
	console.log('[SboSvr] 終了しました（終了コード ' + code + '）');
};
