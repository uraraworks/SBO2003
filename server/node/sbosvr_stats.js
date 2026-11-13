/* ========================================================================= */
/* sbosvr_stats.js															 */
/* 利用環境（OS・ブラウザ・端末）の統計（Emscripten の --pre-js で組み込む）	 */
/* ========================================================================= */
/*
 * 「どんな環境で遊ばれているか」を管理ページ（/admin の「プレイヤー」タブ）で見るための記録と集計。
 *
 * ・ゲームにログインできた時に C 側（server/web_account_svr.cpp の WebStatNoteLogin）が
 *   noteLogin(接続スロット番号, アカウント名) を呼ぶ。環境は sbosvr_net.js が接続時に
 *   User-Agent から分類して接続に持たせている（classifyEnv）。
 *   生の User-Agent と IP アドレスは保存しない（分類した結果だけ持つ）。
 * ・環境は {os, browser, device}。数えるときのキーは "os|browser|device"。
 *     os      windows / mac / ios / android / linux / chromeos / other
 *     browser edge / chrome / safari / firefox / samsung / opera / native（Windows 版ソフト） / other
 *     device  pc / phone / tablet
 * ・保存先は SBODATA/SboWebStats.json。形式:
 *     {version, salt, since:'YYYY-MM-DD',
 *      days: {'YYYY-MM-DD': {'<アカウントの指紋>': {'os|browser|device': ログイン回数}}},
 *      old:  {'YYYY-MM-DD': {'os|browser|device': ログイン回数}}}
 *   日付は日本時間（JST）。アカウント名はそのまま書かず、ファイル内のソルト付き SHA-256 の先頭 16 桁にする。
 *   90 日より古い日は days から old へ移し、アカウントの部分を捨てる（全期間の延べ回数だけ残る）。
 * ・書き込みは数十秒に 1 回まとめて行い、終了時にも書く。
 *
 * HTTP（sbosvr_net.js が /api/admin/stats/ をここへ回す）
 *   POST /api/admin/stats/env {session, days: 1|7|30|90|0(全期間)}
 *     → {days, since, total, multi, axes: {os:[...], browser:[...], device:[...], osBrowser:[...]}}
 *        各行は {key, accounts, main, logins}
 *          accounts … 期間内にその環境で 1 回でも入ったアカウント数（複数の環境を使う人は両方に数える）
 *          main     … アカウントごとに、期間内で回数が一番多い環境 1 つだけで数えた人数（合計が実人数）
 *          logins   … 延べログイン回数
 *        total は実人数、multi は PC とスマホ/タブレットの両方を使った人数。
 *        days=0（全期間）は logins だけ（accounts・main・total・multi は null。古い日は人を覚えていないため）
 */
Module['sboStats'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var KEEP_DAYS = 90;				/* アカウントごとの記録を残す日数（今日を含む） */
	var SAVE_MS = 30000;			/* まとめて書く間隔 */
	var MAX_BODY = 4 * 1024;		/* 受け取る JSON の大きさ */
	var PERIODS = [0, 1, 7, 30, 90];

	var OS_LIST = ['windows', 'mac', 'ios', 'android', 'linux', 'chromeos', 'other'];
	var BROWSER_LIST = ['edge', 'chrome', 'safari', 'firefox', 'samsung', 'opera', 'native', 'other'];
	var DEVICE_LIST = ['pc', 'phone', 'tablet'];

	var store = null;
	var strStorePath = '';
	var bDirty = false;
	var timer = null;

	function acc() {
		return Module['sboAccount'];
	}

	function log(msg) {
		console.log('[stats] ' + msg);
	}

	/* ------------------------------------------------------------------ */
	/* User-Agent の分類（生の文字列は残さない）								 */
	/* ------------------------------------------------------------------ */
	/*
	 * strUA: User-Agent  bTouch: 接続 URL の tp=1（タッチ画面が 2 点以上）かどうか。
	 * iPadOS は User-Agent が Mac と同じなので、Mac でタッチ画面ならば iPad（ios・tablet）とみなす。
	 * LINE などのアプリ内ブラウザは browser: 'other'。
	 */
	function classifyEnv(strUA, bTouch) {
		var ua = String(strUA || '');
		var os = 'other', browser = 'other', device = 'pc';

		if (/iPhone|iPod/.test(ua)) {
			os = 'ios';
			device = 'phone';
		} else if (/iPad/.test(ua)) {
			os = 'ios';
			device = 'tablet';
		} else if (/Android/.test(ua)) {
			os = 'android';
			device = /Mobile/.test(ua) ? 'phone' : 'tablet';
		} else if (/CrOS/.test(ua)) {
			os = 'chromeos';
		} else if (/Windows/.test(ua)) {
			os = 'windows';
		} else if (/Macintosh|Mac OS X/.test(ua)) {
			if (bTouch) {
				os = 'ios';
				device = 'tablet';
			} else {
				os = 'mac';
			}
		} else if (/Linux|X11/.test(ua)) {
			os = 'linux';
		} else if (/Mobile/.test(ua)) {
			device = 'phone';
		}

		/* アプリ内ブラウザ（LINE・Facebook・Instagram・WeChat・X・Android の WebView など） */
		if (/\bLine\/|FBAN|FBAV|Instagram|MicroMessenger|Twitter|DuckDuckGo|; wv\)/i.test(ua)) {
			browser = 'other';
		} else if (/Edg\/|EdgA\/|EdgiOS\/|Edge\//.test(ua)) {		/* Edge・Opera・Samsung は Chrome を名乗るので先に見る */
			browser = 'edge';
		} else if (/OPR\/|OPiOS\/|Opera/.test(ua)) {
			browser = 'opera';
		} else if (/SamsungBrowser\//.test(ua)) {
			browser = 'samsung';
		} else if (/Firefox\/|FxiOS\//.test(ua)) {
			browser = 'firefox';
		} else if (/Chrome\/|CriOS\/|Chromium\//.test(ua)) {
			browser = 'chrome';
		} else if (/Safari\//.test(ua)) {
			browser = 'safari';
		}
		return { os: os, browser: browser, device: device };
	}

	/* Windows 版ソフト（TCP 接続） */
	function nativeEnv() {
		return { os: 'windows', browser: 'native', device: 'pc' };
	}

	function envKey(env) {
		return env.os + '|' + env.browser + '|' + env.device;
	}

	/* "os|browser|device" を分ける。知らない値は other・pc にそろえる */
	function parseKey(strKey) {
		var a = String(strKey).split('|');
		return {
			os: OS_LIST.indexOf(a[0]) >= 0 ? a[0] : 'other',
			browser: BROWSER_LIST.indexOf(a[1]) >= 0 ? a[1] : 'other',
			device: DEVICE_LIST.indexOf(a[2]) >= 0 ? a[2] : 'pc'
		};
	}

	/* ------------------------------------------------------------------ */
	/* 日付（日本時間）														 */
	/* ------------------------------------------------------------------ */
	function jstDate(nowMs) {
		return new Date(nowMs + 9 * 3600 * 1000).toISOString().slice(0, 10);
	}

	/* 'YYYY-MM-DD' から nDays 日ずらした日付 */
	function shiftDate(strDate, nDays) {
		var a = strDate.split('-');
		var t = Date.UTC(+a[0], +a[1] - 1, +a[2]) + nDays * 86400000;
		return new Date(t).toISOString().slice(0, 10);
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	function load() {
		if (store) {
			return store;
		}
		strStorePath = path.resolve(FS.cwd(), 'SBODATA', 'SboWebStats.json');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object') {
			store = {};
		}
		store.version = 1;
		if (typeof store.salt !== 'string' || store.salt.length < 16) {
			store.salt = crypto.randomBytes(16).toString('hex');		/* 初回だけ作る */
			bDirty = true;
		}
		store.days = store.days && typeof store.days === 'object' ? store.days : {};
		store.old = store.old && typeof store.old === 'object' ? store.old : {};
		if (typeof store.since !== 'string') {
			store.since = jstDate(Date.now());
			bDirty = true;
		}
		prune(Date.now());
		if (!timer) {
			timer = setInterval(flush, SAVE_MS);
			timer.unref();
		}
		return store;
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function flush() {
		if (!store || !bDirty) {
			return;
		}
		var tmp = strStorePath + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(store));
			fs.renameSync(tmp, strStorePath);
			bDirty = false;
		} catch (e) {
			console.error('[stats] 保存できません: ' + e.message);
		}
	}

	/* 90 日より古い日は、アカウントの部分を捨てて「日付 × 環境」の回数だけ old に残す */
	function prune(nowMs) {
		var strLimit = shiftDate(jstDate(nowMs), -(KEEP_DAYS - 1));
		var nMoved = 0;
		Object.keys(store.days).forEach(function (strDay) {
			if (strDay >= strLimit) {
				return;
			}
			var o = store.old[strDay] || (store.old[strDay] = {});
			var accs = store.days[strDay];
			Object.keys(accs).forEach(function (h) {
				Object.keys(accs[h]).forEach(function (k) {
					o[k] = (o[k] || 0) + accs[h][k];
				});
			});
			delete store.days[strDay];
			nMoved++;
		});
		if (nMoved) {
			bDirty = true;
			log(nMoved + ' 日分のアカウントごとの記録を整理しました');
		}
	}

	function hashAccount(strAcc) {
		return crypto.createHash('sha256').update(store.salt + '\n' + strAcc, 'utf8').digest('hex').slice(0, 16);
	}

	/* ログイン 1 回を記録する（nowMs はテスト用） */
	function record(strAcc, env, nowMs) {
		if (!strAcc || !env) {
			return;
		}
		load();
		nowMs = nowMs || Date.now();
		prune(nowMs);
		var strDay = jstDate(nowMs);
		var days = store.days[strDay] || (store.days[strDay] = {});
		var h = hashAccount(strAcc);
		var a = days[h] || (days[h] = {});
		var k = envKey(env);
		a[k] = (a[k] || 0) + 1;
		bDirty = true;
	}

	/* C 側（WebStatNoteLogin）から呼ばれる。ゲームにログインできた時 */
	function noteLogin(nSlot, strAcc) {
		var net = Module['sboNet'];
		var env = net && net.envOfSlot ? net.envOfSlot(nSlot) : null;
		try {
			record(strAcc, env);
		} catch (e) {
			console.error('[stats] 記録に失敗: ' + (e && e.stack || e));
		}
	}

	/* ------------------------------------------------------------------ */
	/* 集計																	 */
	/* ------------------------------------------------------------------ */
	/* 軸ごとのキーの出し方 */
	var AXES = {
		os: function (e) { return e.os; },
		browser: function (e) { return e.browser; },
		device: function (e) { return e.device; },
		osBrowser: function (e) { return e.os + '+' + e.browser; }
	};

	function addTo(map, key, name, n) {
		var m = map[key] || (map[key] = { key: key, accounts: 0, main: 0, logins: 0 });
		m[name] += n;
	}

	function toList(map, bPeople) {
		var a = Object.keys(map).map(function (k) { return map[k]; });
		if (!bPeople) {
			a.forEach(function (m) { m.accounts = null; m.main = null; });
		}
		a.sort(function (x, y) {
			return (y.main - x.main) || (y.accounts - x.accounts) || (y.logins - x.logins) || (x.key < y.key ? -1 : 1);
		});
		return a;
	}

	/* nDays 日分（0 は全期間）を集計する */
	function aggregate(nDays, nowMs) {
		load();
		nowMs = nowMs || Date.now();
		prune(nowMs);
		var strToday = jstDate(nowMs);
		var axes = {};
		Object.keys(AXES).forEach(function (n) { axes[n] = {}; });

		/* 全期間は延べ回数だけ（古い日は人を覚えていない） */
		if (nDays === 0) {
			var perKey = {};
			var addKey = function (k, n) { perKey[k] = (perKey[k] || 0) + n; };
			Object.keys(store.old).forEach(function (d) {
				Object.keys(store.old[d]).forEach(function (k) { addKey(k, store.old[d][k]); });
			});
			Object.keys(store.days).forEach(function (d) {
				Object.keys(store.days[d]).forEach(function (h) {
					Object.keys(store.days[d][h]).forEach(function (k) { addKey(k, store.days[d][h][k]); });
				});
			});
			Object.keys(perKey).forEach(function (k) {
				var e = parseKey(k);
				Object.keys(AXES).forEach(function (n) { addTo(axes[n], AXES[n](e), 'logins', perKey[k]); });
			});
			var outAll = { days: 0, since: store.since, total: null, multi: null, axes: {} };
			Object.keys(AXES).forEach(function (n) { outAll.axes[n] = toList(axes[n], false); });
			return outAll;
		}

		/* アカウントごとの {環境: {n: 回数, last: 最後に使った日}} */
		var strFrom = shiftDate(strToday, -(nDays - 1));
		var perAcc = {};
		Object.keys(store.days).forEach(function (d) {
			if (d < strFrom || d > strToday) {
				return;
			}
			Object.keys(store.days[d]).forEach(function (h) {
				var p = perAcc[h] || (perAcc[h] = {});
				Object.keys(store.days[d][h]).forEach(function (k) {
					var x = p[k] || (p[k] = { n: 0, last: '' });
					x.n += store.days[d][h][k];
					if (d > x.last) {
						x.last = d;
					}
				});
			});
		});

		var nTotal = 0, nMulti = 0;
		Object.keys(perAcc).forEach(function (h) {
			var p = perAcc[h], keys = Object.keys(p), best = null;
			var bPc = false, bMobile = false;
			nTotal++;
			keys.forEach(function (k) {
				var e = parseKey(k);
				if (e.device === 'pc') {
					bPc = true;
				} else {
					bMobile = true;
				}
				/* 回数が一番多い環境。同数なら最後に使った方、それも同じならキーの順 */
				if (!best || p[k].n > p[best].n || (p[k].n === p[best].n &&
						(p[k].last > p[best].last || (p[k].last === p[best].last && k < best)))) {
					best = k;
				}
			});
			if (bPc && bMobile) {
				nMulti++;
			}
			Object.keys(AXES).forEach(function (n) {
				var seen = {};
				keys.forEach(function (k) {
					var v = AXES[n](parseKey(k));
					addTo(axes[n], v, 'logins', p[k].n);
					if (!seen[v]) {
						seen[v] = true;
						addTo(axes[n], v, 'accounts', 1);
					}
				});
				addTo(axes[n], AXES[n](parseKey(best)), 'main', 1);
			});
		});
		var out = { days: nDays, since: store.since, total: nTotal, multi: nMulti, axes: {} };
		Object.keys(AXES).forEach(function (n) { out.axes[n] = toList(axes[n], true); });
		return out;
	}

	/* ------------------------------------------------------------------ */
	/* HTTP																	 */
	/* ------------------------------------------------------------------ */
	function reply(res, status, obj) {
		acc().reply(res, status, obj);
	}

	function readJson(req, cb) {
		var aBuf = [], nSize = 0, bOver = false;
		req.on('data', function (buf) {
			nSize += buf.length;
			if (nSize > MAX_BODY) {
				bOver = true;
				return;
			}
			aBuf.push(buf);
		});
		req.on('end', function () {
			if (bOver) {
				cb(null);
				return;
			}
			try {
				var obj = JSON.parse(Buffer.concat(aBuf).toString('utf8') || '{}');
				cb(obj && typeof obj === 'object' ? obj : null);
			} catch (e) {
				cb(null);
			}
		});
	}

	function apiEnv(body, req, res) {
		if (!acc().isAdminSession(body.session)) {
			reply(res, 401, { error: 'session' });
			return;
		}
		var nDays = parseInt(body.days, 10);
		if (PERIODS.indexOf(nDays) < 0) {
			reply(res, 200, { error: 'invalid' });
			return;
		}
		reply(res, 200, aggregate(nDays));
	}

	/* sbosvr_net.js の HTTP から呼ばれる。ここの API だけ受け持つ */
	function handleHttp(req, res, url) {
		if (url !== '/api/admin/stats/env' || !acc()) {
			return false;
		}
		if (req.method !== 'POST') {
			reply(res, 405, { error: 'method' });
			return true;
		}
		readJson(req, function (body) {
			if (!body) {
				reply(res, 400, { error: 'json' });
				return;
			}
			acc().withAdminCookie(body, req);		/* 管理ページのログインは Cookie */
			try {
				apiEnv(body, req, res);
			} catch (e) {
				console.error('[stats] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	/* 終了時にも書く（Ctrl+C やお知らせファイルでの停止も process.exit を通る） */
	process.on('exit', flush);

	return {
		classifyEnv: classifyEnv,
		nativeEnv: nativeEnv,
		noteLogin: noteLogin,
		record: record,
		aggregate: aggregate,
		prune: prune,
		load: load,
		flush: flush,
		handleHttp: handleHttp
	};
})();
