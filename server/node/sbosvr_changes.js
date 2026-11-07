/* ========================================================================= */
/* sbosvr_changes.js														 */
/* 更新履歴（Emscripten の --pre-js で組み込む）								 */
/* ========================================================================= */
/*
 * プレイヤー向けの「何が変わったか」を見せるページ（/changes。web/changes/changes.html）の API。
 *
 * ・中身は SBODATA/SboWebChanges.json に保存する。管理者は管理ページ（/admin の「更新履歴」）で
 *   足す・直す・隠す・消すができる。
 * ・リポジトリの web/changes/changes.json（配布フォルダでは changes-seed.json）が「最初の中身」。
 *   起動して最初に使う時に、まだ取り込んでいない id の項目だけを取り込む。
 *   プルリクエストで changes.json に項目を足せば、デプロイ後に自動で載る。
 *   管理ページで直した・消した項目は、取り込み済みの id として覚えているので上書きしない。
 *
 * 項目: {id, date:'YYYY-MM-DD', kind:'新機能'|'改善'|'修正', text, pr:[番号...], hidden?, updatedAt?}
 *
 * HTTP（sbosvr_net.js が /api/changes・/api/admin/changes/ をここへ回す）
 *   GET  /api/changes                      → {items:[...], latest}（隠していないものだけ。新しい順）
 *   POST /api/admin/changes/list   {session}                         → {items:[...]}（隠したものも）
 *   POST /api/admin/changes/save   {session, id?, date, kind, text, pr, hidden} → {ok, item}
 *                                     id が無ければ新しく足す（その日の一番上に）
 *   POST /api/admin/changes/delete {session, id}                     → {ok}
 */
Module['sboChanges'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var MAX_ITEMS = 2000;			/* 残す項目の数 */
	var MAX_TEXT = 500;				/* 1 項目の長さ（文字） */
	var MAX_BODY = 16 * 1024;		/* 受け取る JSON の大きさ */
	var KINDS = ['新機能', '改善', '修正'];

	var store = null;
	var strStorePath = '';

	function acc() {
		return Module['sboAccount'];
	}

	function log(msg) {
		console.log('[changes] ' + msg);
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	/* 配布フォルダの changes-seed.json（リポジトリの web/changes/changes.json） */
	function readSeed() {
		var www = Module['sboNet'] && Module['sboNet'].wwwDir ? Module['sboNet'].wwwDir() : '';
		if (!www) {
			return [];
		}
		try {
			var obj = JSON.parse(fs.readFileSync(path.join(www, 'changes-seed.json'), 'utf8'));
			return obj && Array.isArray(obj.items) ? obj.items : [];
		} catch (e) {
			return [];
		}
	}

	/* 新しい日付が先。同じ日付の中は今の並びのまま */
	function sortItems() {
		var a = store.items.map(function (it, i) { return { it: it, i: i }; });
		a.sort(function (x, y) {
			return x.it.date < y.it.date ? 1 : x.it.date > y.it.date ? -1 : x.i - y.i;
		});
		store.items = a.map(function (x) { return x.it; });
	}

	function load() {
		if (store) {
			return store;
		}
		strStorePath = path.resolve(FS.cwd(), 'SBODATA', 'SboWebChanges.json');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object') {
			store = {};
		}
		store.version = 1;
		store.items = Array.isArray(store.items) ? store.items : [];
		store.seedIds = Array.isArray(store.seedIds) ? store.seedIds : [];
		importSeed();
		return store;
	}

	/* まだ取り込んでいない「最初の中身」の項目を取り込む */
	function importSeed() {
		var known = {}, nAdd = 0, prevId = null;
		store.seedIds.forEach(function (id) { known[id] = true; });
		readSeed().forEach(function (src) {
			var it = cleanItem(src);
			if (!it || !src.id) {
				return;
			}
			it.id = str(src.id, 32);
			if (!known[it.id]) {
				known[it.id] = true;
				store.seedIds.push(it.id);
				/* 「最初の中身」の並びを保つよう、1 つ前の項目のすぐ後ろに入れる */
				var at = prevId ? findItem(prevId) + 1 : 0;
				store.items.splice(Math.max(0, at), 0, it);
				nAdd++;
			}
			prevId = findItem(it.id) >= 0 ? it.id : prevId;
		});
		if (nAdd) {
			sortItems();
			save();
			log('最初の中身から ' + nAdd + ' 件を取り込みました');
		}
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function save() {
		var tmp = strStorePath + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(store, null, '\t'));
			fs.renameSync(tmp, strStorePath);
		} catch (e) {
			console.error('[changes] 保存できません: ' + e.message);
		}
	}

	function findItem(id) {
		for (var i = 0; i < store.items.length; i++) {
			if (store.items[i].id === id) {
				return i;
			}
		}
		return -1;
	}

	/* ------------------------------------------------------------------ */
	/* 項目をそろえる															 */
	/* ------------------------------------------------------------------ */
	function str(v, nMax) {
		return String(v == null ? '' : v).slice(0, nMax || 64);
	}

	/* 本文（改行は \n に、制御文字は除き、前後の空白を詰める）。長すぎる時は null */
	function cleanText(v) {
		var s = String(v == null ? '' : v).replace(/\r\n?/g, '\n')
			.replace(/[\u0000-\u0009\u000B-\u001F\u007F]/g, '')
			.replace(/\n{3,}/g, '\n\n')
			.trim();
		return s.length > MAX_TEXT ? null : s;
	}

	/* PR の番号は「85, 86」のような文字でも配列でも受け付ける */
	function cleanPr(v) {
		var a = Array.isArray(v) ? v : String(v == null ? '' : v).split(/[^0-9]+/);
		var out = [];
		a.forEach(function (n) {
			n = parseInt(n, 10);
			if (n > 0 && n < 1000000 && out.indexOf(n) < 0 && out.length < 20) {
				out.push(n);
			}
		});
		return out;
	}

	/* 足す・直す項目を確かめる。おかしい時は null */
	function cleanItem(src) {
		if (!src || typeof src !== 'object') {
			return null;
		}
		var date = str(src.date, 10);
		var text = cleanText(src.text);
		if (!/^\d{4}-\d{2}-\d{2}$/.test(date) || !text) {
			return null;
		}
		var it = {
			id: '',
			date: date,
			kind: KINDS.indexOf(src.kind) >= 0 ? src.kind : KINDS[1],
			text: text,
			pr: cleanPr(src.pr)
		};
		if (src.hidden) {
			it.hidden = true;
		}
		return it;
	}

	function publicView(it) {
		return { id: it.id, date: it.date, kind: it.kind, text: it.text, pr: it.pr || [] };
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

	function checkAdmin(body, res) {
		if (!acc().isAdminSession(body.session)) {
			reply(res, 401, { error: 'session' });
			return false;
		}
		return true;
	}

	function apiList(body, req, res) {
		load();
		var a = store.items.filter(function (it) { return !it.hidden; }).map(publicView);
		reply(res, 200, { items: a, latest: a.length ? a[0].date : '' });
	}

	function apiAdminList(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		reply(res, 200, { items: store.items, kinds: KINDS, maxLength: MAX_TEXT });
	}

	function apiAdminSave(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		if (cleanText(body.text) === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_TEXT });
			return;
		}
		var it = cleanItem(body);
		if (!it) {
			reply(res, 200, { error: 'invalid' });
			return;
		}
		it.updatedAt = Date.now();
		var id = str(body.id, 32), i;
		if (id) {
			i = findItem(id);
			if (i < 0) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
			it.id = id;
			store.items[i] = it;
			log('管理者が直した: ' + id);
		} else {
			if (store.items.length >= MAX_ITEMS) {
				reply(res, 200, { error: 'full' });
				return;
			}
			it.id = 'a' + Date.now().toString(36) + crypto.randomBytes(3).toString('hex');
			/* その日の一番上に入れる（並べ直すと日付の位置へ動く） */
			store.items.unshift(it);
			log('管理者が足した: ' + it.id + ' ' + it.date);
		}
		sortItems();
		save();
		reply(res, 200, { ok: true, item: it });
	}

	function apiAdminDelete(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var i = findItem(str(body.id, 32));
		if (i < 0) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		store.items.splice(i, 1);
		save();
		log('管理者が消した: ' + body.id);
		reply(res, 200, { ok: true });
	}

	var API = {
		'/api/changes': apiList,
		'/api/admin/changes/list': apiAdminList,
		'/api/admin/changes/save': apiAdminSave,
		'/api/admin/changes/delete': apiAdminDelete
	};

	/* sbosvr_net.js の HTTP から呼ばれる。ここの API だけ受け持つ */
	function handleHttp(req, res, url) {
		var fn = API[url];
		if (!fn || !acc()) {
			return false;
		}
		/* みんなが読む一覧は GET でもよい（ページを開いた時に読む） */
		if (url === '/api/changes' && req.method === 'GET') {
			try {
				fn({}, req, res);
			} catch (e) {
				console.error('[changes] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
			return true;
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
			if (url.indexOf('/api/admin/') === 0) {
				acc().withAdminCookie(body, req);		/* 管理ページのログインは Cookie */
			}
			try {
				fn(body, req, res);
			} catch (e) {
				console.error('[changes] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	return {
		handleHttp: handleHttp
	};
})();
