/* テスト用の通信の代わり（server/node/sbosvr_net.js の代わりに --pre-js で組み込む）。
   サーバーが各接続へ送ったバイト列を覚えておき、長さ・送信回数・SHA-256 を返す。 */
Module['sboNet'] = (function () {
	var crypto = require('crypto');
	var aConn = {};		/* 接続番号 → { hash, len, calls, first, raw } */
	var bTrace = false;	/* true の間は送ったバイト列をそのまま覚える（raw） */

	function get(n) {
		if (!aConn[n]) {
			aConn[n] = { hash: crypto.createHash('sha256'), len: 0, calls: 0, first: null, raw: [] };
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
		reset: function () { aConn = {}; }
	};
})();
