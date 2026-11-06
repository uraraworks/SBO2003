/* テスト用の通信の代わり（server/node/sbosvr_net.js の代わりに --pre-js で組み込む）。
   サーバーが各接続へ送ったバイト列を覚えておき、長さ・送信回数・SHA-256 を返す。 */
Module['sboNet'] = (function () {
	var crypto = require('crypto');
	var aConn = {};		/* 接続番号 → { hash, len, calls, first } */

	function get(n) {
		if (!aConn[n]) {
			aConn[n] = { hash: crypto.createHash('sha256'), len: 0, calls: 0, first: null };
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
		reset: function () { aConn = {}; }
	};
})();
