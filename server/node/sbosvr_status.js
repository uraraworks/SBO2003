/* ========================================================================= */
/* sbosvr_status.js															 */
/* サーバー状況（オンライン数など）の HTTP（Emscripten の --pre-js で組み込む）	 */
/* ========================================================================= */
/*
 * SBOP2 と同じく、サーバーの状況を誰でも見られるようにする。
 * 値そのものは C 側（server/web_status_svr.cpp の SboSvrStatusJson）が返し、
 * ここでは「直近 24 時間の人数の移り変わり」を数えて足す。
 *
 * HTTP（GET。sbosvr_net.js がここへ回す。どちらも別のサイトから読めるよう CORS を許す）
 *   /health      → {"status":"ok","onlinePlayers":N}
 *                  SBOP2 の /health と同じ形。デプロイの確認（"ok" が含まれるか）にも使う。
 *   /api/status  → サーバー状況の詳しい値（サーバー状況ページ status.html が読む）
 *
 * 直近 24 時間の記録は 1 時間ごとの最大人数で、SBODATA/SboWebStatus.json に保存する
 * （再起動しても消えないように）。キャラ名などの個人の情報は扱わない。
 */
Module['sboStatus'] = (function () {
	var fs = require('fs');
	var path = require('path');

	var HOUR_MS = 3600 * 1000;
	var SAMPLE_MS = 30 * 1000;		/* 人数を数える間隔 */
	var KEEP_HOURS = 24;			/* 1 時間ごとの記録を残す数 */

	var nStartedAt = Date.now();
	var store = null;				/* { hours: [{t, max}], peak: {count, at} } */
	var strStorePath = '';
	var timer = null;

	function log(msg) {
		console.log('[status] ' + msg);
	}

	/* 保存してある記録を読む（データフォルダに移った後で呼ぶ） */
	function loadStore() {
		if (store) {
			return;
		}
		strStorePath = path.resolve(FS.cwd(), 'SBODATA', 'SboWebStatus.json');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object' || !Array.isArray(store.hours)) {
			store = { hours: [], peak: null };
		}
	}

	function saveStore() {
		try {
			var tmp = strStorePath + '.tmp';
			fs.writeFileSync(tmp, JSON.stringify(store));
			fs.renameSync(tmp, strStorePath);
		} catch (e) {
			console.error('[status] 保存できません: ' + e.message);
		}
	}

	/* C 側の値（失敗したら null） */
	function svrStatus() {
		try {
			return JSON.parse(UTF8ToString(_SboSvrStatusJson()));
		} catch (e) {
			return null;
		}
	}

	/* 今の人数を 1 時間ごとの記録に足す */
	function sample(st) {
		loadStore();
		var now = Date.now();
		var t = Math.floor(now / HOUR_MS) * HOUR_MS;
		var n = st.online >>> 0;
		var changed = false;
		var last = store.hours[store.hours.length - 1];
		if (!last || last.t !== t) {
			last = { t: t, max: n };
			store.hours.push(last);
			changed = true;
		} else if (n > last.max) {
			last.max = n;
			changed = true;
		}
		/* 24 時間より古いものは捨てる */
		while (store.hours.length && store.hours[0].t <= t - KEEP_HOURS * HOUR_MS) {
			store.hours.shift();
			changed = true;
		}
		if (!store.peak || n > store.peak.count) {
			store.peak = { count: n, at: new Date(now).toISOString() };
			changed = true;
		}
		if (changed) {
			saveStore();
		}
	}

	function tick() {
		var st = svrStatus();
		if (st) {
			sample(st);
		}
	}

	/* 直近 24 時間（古い順）。記録の無い時間は null（サーバーが止まっていた・記録前） */
	function recentHours() {
		var t0 = Math.floor(Date.now() / HOUR_MS) * HOUR_MS - (KEEP_HOURS - 1) * HOUR_MS;
		var map = {};
		store.hours.forEach(function (h) {
			map[h.t] = h.max;
		});
		var out = [];
		for (var i = 0; i < KEEP_HOURS; i ++) {
			var t = t0 + i * HOUR_MS;
			out.push({ at: new Date(t).toISOString(), max: (t in map) ? map[t] : null });
		}
		return out;
	}

	function detail(st) {
		sample(st);
		return {
			status: 'ok',
			updatedAt: new Date().toISOString(),
			onlinePlayers: st.online,
			maxConnect: st.maxConnect,
			playerChars: st.playerChars,
			chars: st.chars,
			maps: st.maps,
			uptimeSec: st.uptimeSec,
			startedAt: new Date(nStartedAt).toISOString(),
			/* サーバーの時刻と UTC の差（分）。hourRecord の「何時」はサーバーの時刻なので、ページ側で日本時間に直す */
			serverUtcOffsetMin: -new Date().getTimezoneOffset(),
			hourRecord: st.hourRecord,
			recentHours: recentHours(),
			peak: store.peak
		};
	}

	function reply(res, status, obj, method) {
		var body = JSON.stringify(obj);
		res.writeHead(status, {
			'Content-Type': 'application/json; charset=utf-8',
			'Cache-Control': 'no-store',
			'Access-Control-Allow-Origin': '*'
		});
		res.end(method === 'HEAD' ? undefined : body);
	}

	/* 扱った URL なら true */
	function handleHttp(req, res, url) {
		if (url !== '/health' && url !== '/api/status') {
			return false;
		}
		if (req.method !== 'GET' && req.method !== 'HEAD') {
			reply(res, 405, { error: 'method' }, req.method);
			return true;
		}
		var st = svrStatus();
		if (!st) {
			reply(res, 503, { status: 'starting' }, req.method);
			return true;
		}
		if (url === '/health') {
			reply(res, 200, { status: 'ok', onlinePlayers: st.online }, req.method);
			return true;
		}
		try {
			reply(res, 200, detail(st), req.method);
		} catch (e) {
			console.error('[status] 失敗: ' + (e && e.stack || e));
			reply(res, 500, { error: 'internal' }, req.method);
		}
		return true;
	}

	/* HTTP の待ち受けを始めた時に呼ぶ（誰も見に来なくても記録をつける） */
	function start() {
		if (timer) {
			return;
		}
		timer = setInterval(tick, SAMPLE_MS);
		if (timer.unref) {
			timer.unref();
		}
		log('サーバー状況の記録を始めました（/api/status・/health）');
	}

	return {
		handleHttp: handleHttp,
		start: start
	};
})();
