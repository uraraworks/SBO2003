/* テスト用の通信の代わり（server/node/sbosvr_net.js の代わりに --pre-js で組み込む）。
   サーバーが各接続へ送ったバイト列を覚えておき、長さ・送信回数・SHA-256 を返す。 */
Module['sboNet'] = (function () {
	var crypto = require('crypto');
	var zlib = require('zlib');
	var aConn = {};		/* 接続番号 → { hash, len, calls, first, raw } */
	var bTrace = false;	/* true の間は送ったバイト列をそのまま覚える（raw） */

	function get(n) {
		if (!aConn[n]) {
			aConn[n] = { hash: crypto.createHash('sha256'), len: 0, calls: 0, first: null, raw: [], cmds: [] };
		}
		return aConn[n];
	}
	return {
		listen: function () { return true; },
		send: function (n, u8) {
			var c = get(n);
			c.hash.update(u8);
			c.len += u8.length;
			c.calls ++;
			if (!c.first) {
				c.first = Buffer.from(u8);
			}
			/* 先頭のコマンド番号を覚えておく（テスト用。圧縮されていれば広げて見る） */
			if (u8.length > 8) {
				var cmd = u8[8];
				if (cmd === 0xFF && u8.length > 13) {
					try { cmd = zlib.inflateSync(Buffer.from(u8.slice(13)))[0]; } catch (e) { cmd = 255; }
				}
				c.cmds.push(cmd);
			}
			if (bTrace) {
				c.raw.push(Buffer.from(u8));
			}
		},
		close: function () {},
		closeAll: function () {},
		count: function () { return 0; },
		config: function () {},
		/* テスト用 */
		report: function (n) {
			var c = get(n);
			/* hash は digest すると使えなくなるので複製して取る */
			var h = c.hash.copy().digest('hex').substr(0, 16);
			return 'len=' + c.len + ' calls=' + c.calls + ' sha=' + h;
		},
		calls: function (n) { return aConn[n] ? aConn[n].calls : 0; },
		cmdCount: function (n, cmd) {
			var c = aConn[n], k = 0;
			if (c) { c.cmds.forEach(function (x) { if (x === cmd) k ++; }); }
			return k;
		},
		/* 送ったコマンドの数（これから先の順番を調べる時の起点） */
		cmdLen: function (n) { return aConn[n] ? aConn[n].cmds.length : 0; },
		/* from 番目以降で、コマンド a が全部 コマンド b より前なら 1、逆なら 0、どちらかが無ければ -1 */
		cmdBefore: function (n, from, a, b) {
			var c = aConn[n], lastA = -1, firstB = -1;
			if (!c) { return -1; }
			for (var i = from; i < c.cmds.length; i ++) {
				if (c.cmds[i] === a) { lastA = i; }
				if (c.cmds[i] === b && firstB < 0) { firstB = i; }
			}
			if (lastA < 0 || firstB < 0) { return -1; }
			return lastA < firstB ? 1 : 0;
		},
		firstPacket: function (n) {
			var c = aConn[n];
			return c && c.first ? c.first : null;
		},
		reportAll: function () {
			var h = crypto.createHash('sha256'), tot = 0;
			Object.keys(aConn).map(Number).sort(function (a, b) { return a - b; }).forEach(function (k) {
				var c = aConn[k];
				h.update(k + ':' + c.len + ':' + c.calls + ':' + c.hash.copy().digest('hex') + ';');
				tot += c.len;
			});
			return 'total=' + tot + ' sha=' + h.digest('hex').substr(0, 16);
		},
		trace: function (bOn) {
			bTrace = bOn;
			if (bOn) {
				Object.keys(aConn).forEach(function (k) { aConn[k].raw = []; });
			}
		},
		/* 覚えたバイト列をパケット（サイズ4・CRC4・中身）に分け、先頭がコマンド nCmd の
		   中身の 2 バイト目（byData）を「,」区切りで返す。圧縮されたパケットは見ない */
		cmdData: function (n, nCmd) {
			var c = aConn[n];
			if (!c) {
				return '';
			}
			var b = Buffer.concat(c.raw), a = [], nPos = 0;
			while (nPos + 8 <= b.length) {
				var nSize = b.readUInt32LE(nPos);
				var body = b.subarray(nPos + 8, nPos + 8 + nSize);
				if (body.length >= 2 && body[0] === nCmd) {
					a.push(body[1]);
				}
				nPos += 8 + nSize;
			}
			return a.join(',');
		},
		/* 覚えたパケット（trace を入れた後のもの）のうち、コマンド nCmd で中身に文字列 sText を含むものがあれば 1 */
		hasText: function (n, nCmd, sText) {
			var c = aConn[n];
			if (!c) {
				return 0;
			}
			var b = Buffer.concat(c.raw), nPos = 0, t = Buffer.from(sText, 'utf8');
			while (nPos + 8 <= b.length) {
				var nSize = b.readUInt32LE(nPos);
				var body = b.subarray(nPos + 8, nPos + 8 + nSize);
				if (body.length > 5 && body[0] === 0xFF) {
					try { body = zlib.inflateSync(Buffer.from(body.subarray(5))); } catch (e) { body = Buffer.alloc(0); }
				}
				if (body.length >= 1 && body[0] === nCmd && body.indexOf(t) >= 0) {
					return 1;
				}
				nPos += 8 + nSize;
			}
			return 0;
		},
		/* 覚えたパケットのうち、コマンド nCmd のものの中身を、順番に 16 進にして「,」区切りで返す（圧縮は伸ばす） */
		cmdAll: function (n, nCmd) {
			var c = aConn[n], a = [], nPos = 0;
			if (!c) {
				return '';
			}
			var b = Buffer.concat(c.raw);
			while (nPos + 8 <= b.length) {
				var nSize = b.readUInt32LE(nPos);
				var body = b.subarray(nPos + 8, nPos + 8 + nSize);
				if (body.length > 5 && body[0] === 0xFF) {
					try { body = zlib.inflateSync(Buffer.from(body.subarray(5))); } catch (e) { body = Buffer.alloc(0); }
				}
				if (body.length >= 1 && body[0] === nCmd) {
					a.push(body.toString('hex'));
				}
				nPos += 8 + nSize;
			}
			return a.join(',');
		},
		/* 最後のコマンドを16進で返す。地形・差分の種類と、マスを含む差分に絞ることもできる。 */
		lastBody: function (n, nCmd, nValid, countOffset) {
			var c = aConn[n];
			if (!c) {
				return '';
			}
			var b = Buffer.concat(c.raw), s = '', nPos = 0;
			while (nPos + 8 <= b.length) {
				var nSize = b.readUInt32LE(nPos);
				var body = b.subarray(nPos + 8, nPos + 8 + nSize);
				/* 圧縮されたパケット（先頭 0xFF・元の大きさ 4 バイトの後ろが zlib）は伸ばして見る */
				if (body.length > 5 && body[0] === 0xFF) {
					try { body = zlib.inflateSync(Buffer.from(body.subarray(5))); } catch (e) { body = Buffer.alloc(0); }
				}
				if (body.length >= 1 && body[0] === nCmd && (nValid === undefined || body[1] === nValid) &&
                    (countOffset === undefined || body.readUInt16LE(countOffset) > 0)) {
					s = body.toString('hex');
				}
				nPos += 8 + nSize;
			}
			return s;
		},
		reset: function () { aConn = {}; }
	};
})();
