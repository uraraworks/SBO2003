/* ========================================================================= */
/* sbosvr_feedback.js														 */
/* ご意見箱（Emscripten の --pre-js で組み込む）								 */
/* ========================================================================= */
/*
 * プレイヤーが管理者へ要望・ご意見・不具合を送るページ（/feedback。web/account/feedback.html）の API。
 *
 * ・投稿できるのはブラウザ版でログインしたことのある人だけ（端末トークンで本人確認。sbosvr_account.js）
 * ・投稿ごとに選べる
 *     公開範囲 … public（みんなが見られる）／ admin（管理者だけ）
 *     名前     … キャラ名を出す ／ 匿名（ほかの人には「匿名さん」。管理者には誰か分かる）
 * ・公開の投稿は、ログインしていなくても一覧で読める（フォーラムのように）
 * ・管理者は管理ページ（/admin の「ご意見箱」）で全部を見て、返事・対応済み・隠す・消すができる。
 *   返事は投稿した本人に見え、公開の投稿ならみんなにも見える。
 * ・投稿した本人は自分の投稿を消せる
 *
 * 保存先は SBODATA/SboWebFeedback.json（直近 MAX_POSTS 件）。
 *
 * HTTP（すべて POST・JSON。sbosvr_net.js が /api/feedback/・/api/admin/feedback/ をここへ回す）
 *   /api/feedback/list          {token?, page, category?}     → {posts:[...], total, page, pageSize}
 *                                 公開で隠していないものだけ。token があれば自分の投稿に mine を付ける
 *   /api/feedback/mine          {token}                       → {posts:[...]}（管理者だけの投稿も含む）
 *   /api/feedback/me            {token}                       → {name, canPost, waitSeconds, maxLength}
 *   /api/feedback/post          {token, category, body, public, anonymous} → {ok, post} / {error}
 *   /api/feedback/delete        {token, id}                   → {ok}
 *   /api/admin/feedback/list    {session, filter, page}       → {posts:[...], total, open}
 *                                 filter: open（未対応）/ all
 *   /api/admin/feedback/update  {session, id, reply?, done?, hidden?} → {ok, post}
 *   /api/admin/feedback/delete  {session, id}                 → {ok}
 */
Module['sboFeedback'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var MAX_POSTS = 5000;			/* 残す投稿の数 */
	var MAX_LENGTH = 1000;			/* 本文の長さ（文字） */
	var MAX_REPLY = 1000;			/* 管理者の返事の長さ（文字） */
	var MAX_BODY = 16 * 1024;		/* 受け取る JSON の大きさ */
	var PAGE_SIZE = 20;
	var POST_INTERVAL = 60 * 1000;	/* 同じアカウントは 1 分に 1 回まで */
	var POST_PER_DAY = 10;			/* 同じアカウントは 1 日 10 件まで */
	var DAY = 24 * 60 * 60 * 1000;
	var CATEGORIES = ['request', 'bug', 'opinion', 'other'];	/* 要望・不具合・ご意見・その他 */

	var store = null;
	var strStorePath = '';
	var limitIp = null;				/* 同じ IP からの投稿は 1 日 30 件まで（最初に使う時に作る） */

	function acc() {
		return Module['sboAccount'];
	}

	function log(msg) {
		console.log('[feedback] ' + msg);
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	function load() {
		if (store) {
			return store;
		}
		strStorePath = path.resolve(FS.cwd(), 'SBODATA', 'SboWebFeedback.json');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object') {
			store = {};
		}
		store.version = 1;
		store.posts = store.posts || [];
		return store;
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function save() {
		var tmp = strStorePath + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(store, null, '\t'));
			fs.renameSync(tmp, strStorePath);
		} catch (e) {
			console.error('[feedback] 保存できません: ' + e.message);
		}
	}

	function findPost(id) {
		load();
		for (var i = 0; i < store.posts.length; i++) {
			if (store.posts[i].id === id) {
				return i;
			}
		}
		return -1;
	}

	/* ------------------------------------------------------------------ */
	/* HTTP の下請け															 */
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

	function str(v, nMax) {
		return String(v == null ? '' : v).slice(0, nMax || 64);
	}

	/* 本文をそろえる（改行は \n に、制御文字は除き、前後の空白・3 行以上の空行は詰める） */
	function cleanText(v, nMax) {
		var s = String(v == null ? '' : v).replace(/\r\n?/g, '\n')
			.replace(/[\u0000-\u0008\u000B-\u001F\u007F​-‏‪-‮⁦-⁩]/g, '')
			.replace(/\t/g, '    ')
			.replace(/\n{4,}/g, '\n\n\n')
			.trim();
		return Array.from(s).length > nMax ? null : s;
	}

	/* ------------------------------------------------------------------ */
	/* 見せ方																	 */
	/* ------------------------------------------------------------------ */
	/* キャラ名はいまの名前を見せる（名前を変えた人も追えるように）。キャラが消えていたら投稿した時の名前 */
	function nameOf(p, cache) {
		if (!(p.account in cache)) {
			var info = acc().accInfo(p.account);
			cache[p.account] = info && info.name ? info.name : '';
		}
		return cache[p.account] || p.name || '';
	}

	function publicView(p, cache, strMyAcc) {
		var o = {
			id: p.id, category: p.category, body: p.body, at: p.at,
			name: p.anonymous ? '' : nameOf(p, cache),
			anonymous: !!p.anonymous, done: !!p.done
		};
		if (p.reply) {
			o.reply = p.reply;
			o.replyAt = p.replyAt;
		}
		if (strMyAcc && p.account === strMyAcc) {
			o.mine = true;
		}
		return o;
	}

	function mineView(p, cache) {
		var o = publicView(p, cache, p.account);
		o.name = nameOf(p, cache);
		o.public = !!p.public;
		o.hidden = !!p.hidden;
		return o;
	}

	function adminView(p, cache) {
		var o = mineView(p, cache);
		delete o.mine;
		o.account = p.account;
		o.ip = p.ip;
		o.postedName = p.name;
		return o;
	}

	/* このアカウントがあと何秒で投稿できるか（0 ならすぐ。-1 なら今日はもう無理） */
	function waitSeconds(strAcc) {
		load();
		var now = Date.now(), nLast = 0, nToday = 0;
		store.posts.forEach(function (p) {
			if (p.account !== strAcc) {
				return;
			}
			nLast = Math.max(nLast, p.at);
			if (now - p.at < DAY) {
				nToday++;
			}
		});
		if (nToday >= POST_PER_DAY) {
			return -1;
		}
		var nWait = nLast + POST_INTERVAL - now;
		return nWait > 0 ? Math.ceil(nWait / 1000) : 0;
	}

	function page(a, n) {
		var nPage = Math.max(0, Math.floor(Number(n) || 0));
		return { list: a.slice(nPage * PAGE_SIZE, (nPage + 1) * PAGE_SIZE), page: nPage };
	}

	/* ------------------------------------------------------------------ */
	/* プレイヤーの API														 */
	/* ------------------------------------------------------------------ */
	function apiList(body, req, res) {
		load();
		var info = body.token ? acc().tokenInfo(body.token) : null;
		var cat = CATEGORIES.indexOf(body.category) >= 0 ? body.category : '';
		var a = store.posts.filter(function (p) {
			return p.public && !p.hidden && (!cat || p.category === cat);
		}).reverse();
		var pg = page(a, body.page), cache = {};
		reply(res, 200, {
			posts: pg.list.map(function (p) { return publicView(p, cache, info && info.account); }),
			total: a.length, page: pg.page, pageSize: PAGE_SIZE
		});
	}

	function apiMine(body, req, res) {
		var info = acc().tokenInfo(body.token);
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		load();
		var cache = {};
		var a = store.posts.filter(function (p) { return p.account === info.account; }).reverse();
		reply(res, 200, { posts: a.slice(0, 100).map(function (p) { return mineView(p, cache); }) });
	}

	function apiMe(body, req, res) {
		var info = acc().tokenInfo(body.token);
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		var nWait = waitSeconds(info.account);
		reply(res, 200, {
			account: info.account, name: info.name || '', canPost: !!info.name,
			waitSeconds: nWait, maxLength: MAX_LENGTH
		});
	}

	function apiPost(body, req, res) {
		var info = acc().tokenInfo(body.token);
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		if (!info.name) {
			reply(res, 200, { error: 'no_char' });
			return;
		}
		var ip = acc().clientIp(req);
		limitIp = limitIp || new (acc().Limiter)(30, DAY, DAY);
		var nWait = waitSeconds(info.account);
		if (nWait !== 0 || limitIp.check(ip) > 0) {
			reply(res, 200, { error: nWait > 0 ? 'wait' : 'too_many', waitSeconds: nWait });
			return;
		}
		var text = cleanText(body.body, MAX_LENGTH);
		if (text === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_LENGTH });
			return;
		}
		if (!text) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		load();
		var p = {
			id: crypto.randomBytes(6).toString('hex'),
			account: info.account,
			name: info.name,
			category: CATEGORIES.indexOf(body.category) >= 0 ? body.category : 'other',
			body: text,
			public: !!body.public,
			anonymous: !!body.anonymous,
			at: Date.now(),
			ip: ip,
			done: false,
			hidden: false
		};
		store.posts.push(p);
		if (store.posts.length > MAX_POSTS) {
			store.posts.splice(0, store.posts.length - MAX_POSTS);
		}
		save();
		limitIp.fail(ip);
		log('投稿: ' + p.id + ' ' + info.account + '（' + (p.public ? '公開' : '管理者だけ') + (p.anonymous ? '・匿名' : '') + '、' + ip + '）');
		reply(res, 200, { ok: true, post: mineView(p, {}) });
	}

	function apiDelete(body, req, res) {
		var info = acc().tokenInfo(body.token);
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		var i = findPost(str(body.id, 32));
		if (i < 0 || store.posts[i].account !== info.account) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		store.posts.splice(i, 1);
		save();
		log('本人が消した: ' + body.id + ' ' + info.account);
		reply(res, 200, { ok: true });
	}

	/* ------------------------------------------------------------------ */
	/* 管理者の API（ログインは /api/admin/login と同じ）							 */
	/* ------------------------------------------------------------------ */
	function checkAdmin(body, res) {
		if (!acc().isAdminSession(body.session)) {
			reply(res, 401, { error: 'session' });
			return false;
		}
		return true;
	}

	function apiAdminList(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var nOpen = store.posts.filter(function (p) { return !p.done; }).length;
		var a = store.posts.filter(function (p) {
			return body.filter === 'all' || !p.done;
		}).reverse();
		var pg = page(a, body.page), cache = {};
		reply(res, 200, {
			posts: pg.list.map(function (p) { return adminView(p, cache); }),
			total: a.length, open: nOpen, page: pg.page, pageSize: PAGE_SIZE
		});
	}

	function apiAdminUpdate(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		var i = findPost(str(body.id, 32));
		if (i < 0) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var p = store.posts[i];
		if (body.reply != null) {
			var text = cleanText(body.reply, MAX_REPLY);
			if (text === null) {
				reply(res, 200, { error: 'long', maxLength: MAX_REPLY });
				return;
			}
			if (text) {
				p.reply = text;
				p.replyAt = Date.now();
			} else {
				delete p.reply;
				delete p.replyAt;
			}
		}
		if (body.done != null) {
			p.done = !!body.done;
		}
		if (body.hidden != null) {
			p.hidden = !!body.hidden;
		}
		save();
		log('管理者が更新: ' + p.id + (body.reply != null ? ' 返事' : '') +
			(body.done != null ? (p.done ? ' 対応済み' : ' 未対応') : '') +
			(body.hidden != null ? (p.hidden ? ' 隠す' : ' 隠すのをやめる') : ''));
		reply(res, 200, { ok: true, post: adminView(p, {}) });
	}

	function apiAdminDelete(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		var i = findPost(str(body.id, 32));
		if (i < 0) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		store.posts.splice(i, 1);
		save();
		log('管理者が消した: ' + body.id);
		reply(res, 200, { ok: true });
	}

	var API = {
		'/api/feedback/list': apiList,
		'/api/feedback/mine': apiMine,
		'/api/feedback/me': apiMe,
		'/api/feedback/post': apiPost,
		'/api/feedback/delete': apiDelete,
		'/api/admin/feedback/list': apiAdminList,
		'/api/admin/feedback/update': apiAdminUpdate,
		'/api/admin/feedback/delete': apiAdminDelete
	};

	/* sbosvr_net.js の HTTP から呼ばれる。ここの API だけ受け持つ（ほかは sbosvr_account.js へ） */
	function handleHttp(req, res, url) {
		var fn = API[url];
		if (!fn || !acc()) {
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
			try {
				fn(body, req, res);
			} catch (e) {
				console.error('[feedback] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	return {
		handleHttp: handleHttp
	};
})();
