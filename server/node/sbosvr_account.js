/* ========================================================================= */
/* sbosvr_account.js															 */
/* ブラウザ版のアカウント関係（Emscripten の --pre-js で組み込む）				 */
/* ========================================================================= */
/*
 * 1. 端末トークン（ボタンでログイン）
 *    ID・パスワードでログインできた端末に、ランダムな 64 文字の合言葉を渡す。
 *    ブラウザは localStorage に置き、次からはそれでログインする。
 *    サーバーは SHA-256 だけを SBODATA/SboWebAccount.json に保存する。
 *
 * 2. 復旧キー（SBOP2 のログインコードと同じ考え方）
 *    「すぐにはじめる」で作った人はアカウント名・パスワードを知らないので、
 *    16 文字の復旧キー（例: K7QM-3XRA-9PWD-HT2F）を渡して保存してもらう。
 *    別の端末やブラウザのデータが消えた時に入れると、その端末に端末トークンを渡す。
 *    サーバーは SHA-256 だけを保存し、1 アカウントにつき 1 つ（作り直すと古いものは使えない）。
 *
 * 3. アカウント復旧（当時のアカウントを忘れた人向け）
 *    キャラ名＋（アカウント名 か パスワードの一部）で照合する。照合そのものは
 *    C 側（server/web_account_svr.cpp）が行い、パスワードは JS へ出てこない。
 *    どこまでを自動で通すかは RECOVER_POLICY（環境変数 SBO_RECOVER_POLICY）で決める。
 *      approval … 合っても必ず管理者の承認を待つ
 *      hybrid   … アカウント名とパスワードの一部の両方が合えばすぐ、片方なら承認待ち（既定）
 *      auto     … どちらか片方が合えばすぐ
 *    アカウント名もパスワードも分からない人は、キャラ名＋ひとこと（必須）だけで
 *    申請できる。この時はどの方式でも必ず管理者の承認待ちにする。
 *    復旧できたら、その端末に端末トークンを渡す（パスワードはその後で決め直せる）。
 *
 * 4. キャラ名の変更
 *    プレイヤー自身（端末トークンで本人確認）と管理者が変えられる。名前の決まり
 *    （長さ・使える文字・同名がいないか）は C 側（SboWebAccRename）で確かめる。
 *    プレイヤーが変える時だけ、次の決まりも足す（管理者は対象外）。
 *      ・ログイン中は変えられない（ゲームから出てから）
 *      ・一度変えたら RENAME_COOLDOWN_DAYS 日（環境変数 SBO_RENAME_COOLDOWN_DAYS、既定 7）は変えられない
 *      ・他の人が手放した名前は RENAME_RESERVE_DAYS 日（SBO_RENAME_RESERVE_DAYS、既定 30）は使えない
 *        （なりすまし防止。手放した本人は戻せる）
 *      ・NG_WORDS（SBO_RENAME_NG_WORDS、カンマ区切り）を含む名前は使えない
 *    変えた記録は SboWebAccount.json の renames に残し、管理ページで前の名前からも探せる。
 *
 * HTTP（すべて POST・JSON。sbosvr_net.js が /api/ をここへ回す）
 *   /api/device/me            {token}                      → {account, name, level, renameAfter}
 *   /api/device/forget        {token}                      → {ok}
 *   /api/device/set-password  {token, password}            → {ok}
 *   /api/device/issue-code    {token}                      → {code}（復旧キーを作る・作り直す）
 *   /api/device/redeem        {code}                       → {token, account, name}（復旧キーでこの端末を登録）
 *   /api/recover              {name, account, passwordPart, message}
 *                             → {result:"recovered", token, account, name}
 *                               {result:"pending", requestId, requestKey}
 *                               {result:"ng", reason}
 *   /api/recover/status       {requestId, requestKey}      → {status, token?, account?, name?}
 *   /api/admin/login          {account, password}          → {session}
 *   /api/admin/recover/list   {session}                    → {requests:[...]}
 *   /api/admin/recover/decide {session, id, approve}       → {ok}
 *   /api/device/rename        {token, name, check}         → {result:"ok", from, to} / {result:<理由>, ...}
 *                               check が true なら確かめるだけ（変えない）
 *   /api/admin/search         {session, q}                 → {results:[...], more}
 *   /api/admin/rename         {session, account, name, check} → {result:"ok", from, to} / {result:<理由>}
 */
Module['sboAccount'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var RECOVER_POLICY = (process.env.SBO_RECOVER_POLICY || 'hybrid').toLowerCase();
	function envNum(name, def) {
		var n = parseFloat(process.env[name]);
		return isFinite(n) && n >= 0 ? n : def;
	}
	var RENAME_COOLDOWN_DAYS = envNum('SBO_RENAME_COOLDOWN_DAYS', 7);
	var RENAME_RESERVE_DAYS = envNum('SBO_RENAME_RESERVE_DAYS', 30);
	var NG_WORDS = (process.env.SBO_RENAME_NG_WORDS != null ? process.env.SBO_RENAME_NG_WORDS :
		'管理人,管理者,運営,ＧＭ').split(',').map(function (w) { return w.trim(); }).filter(Boolean);
	var MAX_RENAMES = 5000;		/* 名前を変えた記録はこれだけ残す */
	var MAX_BODY = 4096;
	var MINUTE = 60 * 1000, HOUR = 60 * MINUTE, DAY = 24 * HOUR;

	var store = null;			/* SboWebAccount.json の中身 */
	var strStorePath = '';
	var aAdminSession = {};		/* 承認ページのログイン → 期限 */

	function log(msg) {
		console.log('[account] ' + msg);
	}

	function sha256(str) {
		return crypto.createHash('sha256').update(String(str)).digest('hex');
	}

	function randomHex(nBytes) {
		return crypto.randomBytes(nBytes).toString('hex');
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	function load() {
		if (store) {
			return store;
		}
		/* NODERAWFS なので FS.cwd() は実際のカレントフォルダ（--data で移った先） */
		strStorePath = path.resolve(FS.cwd(), 'SBODATA', 'SboWebAccount.json');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object') {
			store = {};
		}
		store.version = 1;
		store.tokens = store.tokens || {};
		store.requests = store.requests || [];
		store.codes = store.codes || {};
		store.renames = store.renames || [];
		return store;
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function save() {
		var tmp = strStorePath + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(store, null, '\t'));
			fs.renameSync(tmp, strStorePath);
		} catch (e) {
			console.error('[account] 保存できません: ' + e.message);
		}
	}

	/* ------------------------------------------------------------------ */
	/* 端末トークン															 */
	/* ------------------------------------------------------------------ */
	function issueToken(acc) {
		load();
		var token = randomHex(32);
		store.tokens[sha256(token)] = { account: acc, created: Date.now(), lastUsed: 0 };
		save();
		return token;
	}

	function findToken(token) {
		if (!/^[0-9a-f]{64}$/.test(String(token || ''))) {
			return null;
		}
		load();
		var hash = sha256(token);
		var info = store.tokens[hash];
		return info ? { hash: hash, info: info } : null;
	}

	/* ゲームのログイン（C 側）から呼ばれる */
	function tokenToAccount(token) {
		var t = findToken(token);
		if (!t) {
			return '';
		}
		t.info.lastUsed = Date.now();
		save();
		return t.info.account;
	}

	/* そのアカウントの端末トークンを全部消す（復旧した時） */
	function revokeAccount(acc) {
		load();
		Object.keys(store.tokens).forEach(function (hash) {
			if (store.tokens[hash].account === acc) {
				delete store.tokens[hash];
			}
		});
	}

	/* ------------------------------------------------------------------ */
	/* 復旧キー																 */
	/* ------------------------------------------------------------------ */
	/* Crockford Base32（I, L, O, U を除く 32 文字）。256 は 32 で割り切れるので偏らない */
	var CODE_CHARS = '0123456789ABCDEFGHJKMNPQRSTVWXYZ';

	/* 入力をそろえる（ハイフン・空白を除いて大文字に。O→0、I・L→1 は同じとみなす） */
	function normalizeCode(code) {
		return String(code || '').toUpperCase().replace(/[^0-9A-Z]/g, '')
			.replace(/O/g, '0').replace(/[IL]/g, '1');
	}

	function issueCode(acc) {
		load();
		Object.keys(store.codes).forEach(function (hash) {
			if (store.codes[hash].account === acc) {
				delete store.codes[hash];
			}
		});
		var buf = crypto.randomBytes(16), code = '';
		for (var i = 0; i < 16; i++) {
			code += CODE_CHARS.charAt(buf[i] % 32);
		}
		store.codes[sha256(code)] = { account: acc, issued: Date.now() };
		save();
		return code.replace(/(.{4})(?=.)/g, '$1-');
	}

	/* ------------------------------------------------------------------ */
	/* C 側の呼び出し															 */
	/* ------------------------------------------------------------------ */
	function callStr(fn, args) {
		var aPtr = args.map(function (s) { return stringToNewUTF8(String(s == null ? '' : s)); });
		try {
			return UTF8ToString(fn.apply(null, aPtr));
		} finally {
			aPtr.forEach(function (p) { _free(p); });
		}
	}

	function callInt(fn, args) {
		var aPtr = args.map(function (s) { return stringToNewUTF8(String(s == null ? '' : s)); });
		try {
			return fn.apply(null, aPtr);
		} finally {
			aPtr.forEach(function (p) { _free(p); });
		}
	}

	function accInfo(acc) {
		return JSON.parse(callStr(_SboWebAccInfo, [acc]));
	}

	/* ------------------------------------------------------------------ */
	/* 試行回数の制限（失敗を数え、多すぎたらしばらく受け付けない）			 */
	/* ------------------------------------------------------------------ */
	function Limiter(nMax, nWindow, nLock) {
		this.nMax = nMax;
		this.nWindow = nWindow;
		this.nLock = nLock;
		this.a = {};
	}
	Limiter.prototype.check = function (key) {
		var e = this.a[key], now = Date.now();
		if (!e) {
			return 0;
		}
		if (e.until && e.until > now) {
			return Math.ceil((e.until - now) / 1000);
		}
		if (e.until || now - e.start > this.nWindow) {
			delete this.a[key];
		}
		return 0;
	};
	Limiter.prototype.fail = function (key) {
		var now = Date.now(), e = this.a[key];
		if (!e || now - e.start > this.nWindow) {
			e = this.a[key] = { start: now, count: 0, until: 0 };
		}
		e.count++;
		if (e.count >= this.nMax) {
			e.until = now + this.nLock;
		}
	};
	Limiter.prototype.reset = function (key) {
		delete this.a[key];
	};

	var limitRecoverIp = new Limiter(5, 30 * MINUTE, 30 * MINUTE);		/* 同じ IP から 30 分に 5 回失敗 → 30 分休み */
	var limitRecoverName = new Limiter(10, DAY, DAY);					/* 同じキャラ名で 1 日に 10 回失敗 → 1 日休み */
	var limitRequestIp = new Limiter(3, DAY, DAY);						/* 承認待ちの申請は同じ IP から 1 日 3 件まで */
	var limitAdminIp = new Limiter(5, 15 * MINUTE, 15 * MINUTE);		/* 承認ページのログイン */
	var limitCodeIp = new Limiter(10, 30 * MINUTE, 30 * MINUTE);		/* 復旧キーの入力は同じ IP から 30 分に 10 回失敗まで */
	var limitRenameIp = new Limiter(60, 30 * MINUTE, 30 * MINUTE);		/* キャラ名の変更（確かめるだけも含む）は同じ IP から 30 分に 60 回まで */

	/* ------------------------------------------------------------------ */
	/* HTTP の下請け															 */
	/* ------------------------------------------------------------------ */
	function clientIp(req) {
		/* Caddy などの手前のプロキシが最後に足したものを使う（先頭は利用者が書き換えられる） */
		var xff = req.headers['x-forwarded-for'];
		if (xff) {
			var a = String(xff).split(',');
			return a[a.length - 1].trim();
		}
		return String(req.socket.remoteAddress || '').replace(/^::ffff:/, '');
	}

	function reply(res, status, obj) {
		res.writeHead(status, {
			'Content-Type': 'application/json; charset=utf-8',
			'Cache-Control': 'no-store'
		});
		res.end(JSON.stringify(obj));
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

	/* ------------------------------------------------------------------ */
	/* 端末トークンの API														 */
	/* ------------------------------------------------------------------ */
	function apiDeviceMe(body, req, res) {
		var t = findToken(body.token);
		var info = t ? accInfo(t.info.account) : null;
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		reply(res, 200, {
			account: info.account, name: info.name, level: info.level,
			renameAfter: renameAfter(info.account), cooldownDays: RENAME_COOLDOWN_DAYS
		});
	}

	function apiDeviceForget(body, req, res) {
		var t = findToken(body.token);
		if (t) {
			delete store.tokens[t.hash];
			save();
		}
		reply(res, 200, { ok: true });
	}

	function apiDeviceSetPassword(body, req, res) {
		var t = findToken(body.token);
		if (!t) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		var nRet = callInt(_SboWebAccSetPassword, [t.info.account, str(body.password, 32)]);
		if (nRet === 1) {
			log('パスワード変更（端末トークン）: ' + t.info.account);
			reply(res, 200, { ok: true });
		} else if (nRet === 0) {
			reply(res, 400, { error: 'bad_password' });
		} else {
			reply(res, 409, { error: 'online' });
		}
	}

	function apiDeviceIssueCode(body, req, res) {
		var t = findToken(body.token);
		var info = t ? accInfo(t.info.account) : null;
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return;
		}
		var code = issueCode(info.account);
		log('復旧キーを発行: ' + info.account);
		reply(res, 200, { code: code, account: info.account, name: info.name });
	}

	function apiDeviceRedeem(body, req, res) {
		var ip = clientIp(req);
		var nWait = limitCodeIp.check(ip);
		if (nWait > 0) {
			reply(res, 429, { error: 'too_many', retryAfterSeconds: nWait });
			return;
		}
		load();
		var code = normalizeCode(body.code);
		var c = code.length === 16 ? store.codes[sha256(code)] : null;
		var info = c ? accInfo(c.account) : null;
		if (!info) {
			limitCodeIp.fail(ip);
			reply(res, 404, { error: 'not_found' });
			return;
		}
		limitCodeIp.reset(ip);
		var token = issueToken(info.account);
		log('復旧キーで端末を登録: ' + info.account + '（' + ip + '）');
		reply(res, 200, { token: token, account: info.account, name: info.name });
	}

	/* ------------------------------------------------------------------ */
	/* アカウント復旧の API														 */
	/* ------------------------------------------------------------------ */
	function apiRecover(body, req, res) {
		var ip = clientIp(req);
		var name = str(body.name, 40).trim();
		var account = str(body.account, 32).trim();
		var part = str(body.passwordPart, 32).trim();
		var message = str(body.message, 200);

		var nWait = Math.max(limitRecoverIp.check(ip), limitRecoverName.check(name));
		if (nWait > 0) {
			reply(res, 429, { result: 'ng', reason: 'too_many', retryAfterSeconds: nWait });
			return;
		}
		/* アカウント名もパスワードの一部も分からない時は、キャラ名だけで申請できる（必ず承認待ち）。
		   管理者が判断する材料になるので、その時はひとことを必須にする */
		var bNameOnly = !account && !part;
		if (!name || (bNameOnly && !message.trim())) {
			reply(res, 400, { result: 'ng', reason: bNameOnly && name ? 'need_message' : 'input' });
			return;
		}

		var found = JSON.parse(callStr(_SboWebAccRecoverFind, [name, account, part]));
		/* 管理者・ログイン禁止のアカウントは復旧の対象にしない */
		var aMatch = found.matches.filter(function (m) { return !m.admin && !m.disabled; });
		/* 同名キャラで複数合った時は、両方合ったものを優先する */
		var aBoth = aMatch.filter(function (m) { return m.byAccount && m.byPassword; });
		var m = aBoth.length === 1 ? aBoth[0] : (aMatch.length === 1 ? aMatch[0] : null);
		if (!m) {
			limitRecoverIp.fail(ip);
			limitRecoverName.fail(name);
			log('復旧: 一致なし（' + ip + '）');
			reply(res, 200, { result: 'ng', reason: aMatch.length > 1 ? 'ambiguous' : 'not_found' });
			return;
		}
		if (m.online) {
			reply(res, 200, { result: 'ng', reason: 'online' });
			return;
		}

		var bBoth = m.byAccount && m.byPassword;
		var bAuto = !bNameOnly && ((RECOVER_POLICY === 'auto') || (RECOVER_POLICY === 'hybrid' && bBoth));
		if (bAuto) {
			limitRecoverIp.reset(ip);
			revokeAccount(m.account);
			var token = issueToken(m.account);
			log('復旧: 自動で復旧 ' + m.account + '（' + ip + '）');
			reply(res, 200, { result: 'recovered', token: token, account: m.account, name: m.name });
			return;
		}

		/* 承認待ちにする */
		if (limitRequestIp.check(ip) > 0) {
			reply(res, 429, { result: 'ng', reason: 'too_many_requests' });
			return;
		}
		limitRequestIp.fail(ip);
		load();
		var key = randomHex(16);
		var reqInfo = {
			id: randomHex(8),
			keyHash: sha256(key),
			account: m.account,
			byAccount: !!m.byAccount,
			byPassword: !!m.byPassword,
			message: message,
			ip: ip,
			created: Date.now(),
			status: 'pending'
		};
		store.requests.push(reqInfo);
		save();
		log('復旧: 承認待ちの申請 ' + reqInfo.id + ' ' + m.account + '（' + ip + '）');
		reply(res, 200, { result: 'pending', requestId: reqInfo.id, requestKey: key, name: m.name });
	}

	function findRequest(id) {
		load();
		for (var i = 0; i < store.requests.length; i++) {
			if (store.requests[i].id === id) {
				return store.requests[i];
			}
		}
		return null;
	}

	function apiRecoverStatus(body, req, res) {
		var r = findRequest(str(body.requestId));
		if (!r || r.keyHash !== sha256(str(body.requestKey))) {
			reply(res, 404, { status: 'unknown' });
			return;
		}
		if (r.status === 'approved') {
			/* 承認されたら、ここで初めて端末トークンを作って渡す（渡すのは 1 回だけ） */
			revokeAccount(r.account);
			var token = issueToken(r.account);
			var info = accInfo(r.account) || {};
			r.status = 'delivered';
			r.delivered = Date.now();
			save();
			log('復旧: 承認済みの申請を受け取り ' + r.id + ' ' + r.account);
			reply(res, 200, { status: 'recovered', token: token, account: r.account, name: info.name || '' });
			return;
		}
		reply(res, 200, { status: r.status });
	}

	/* ------------------------------------------------------------------ */
	/* キャラ名の変更															 */
	/* ------------------------------------------------------------------ */
	function renameRecord(acc, from, to, by, ip) {
		load();
		store.renames.push({ account: acc, from: from, to: to, by: by, ip: ip, at: Date.now() });
		if (store.renames.length > MAX_RENAMES) {
			store.renames.splice(0, store.renames.length - MAX_RENAMES);
		}
		save();
	}

	/* プレイヤーが次に名前を変えられる時刻（今すぐなら 0） */
	function renameAfter(acc) {
		load();
		var nLast = 0;
		store.renames.forEach(function (r) {
			if (r.account === acc && r.by === 'self' && r.at > nLast) {
				nLast = r.at;
			}
		});
		var nAfter = nLast ? nLast + RENAME_COOLDOWN_DAYS * DAY : 0;
		return nAfter > Date.now() ? nAfter : 0;
	}

	/* 他の人が最近手放した名前か（なりすまし防止） */
	function isReservedName(name, acc) {
		load();
		var nSince = Date.now() - RENAME_RESERVE_DAYS * DAY;
		return store.renames.some(function (r) {
			return r.from === name && r.account !== acc && r.at > nSince;
		});
	}

	/* 比べやすくする（全角英数字→半角、大文字→小文字） */
	function foldWidth(str) {
		return String(str).replace(/[\uFF01-\uFF5E]/g, function (c) {
			return String.fromCharCode(c.charCodeAt(0) - 0xFEE0);
		}).toLowerCase();
	}
	function hasNgWord(name) {
		var f = foldWidth(name);
		return NG_WORDS.some(function (w) { return f.indexOf(foldWidth(w)) >= 0; });
	}

	function callRename(acc, name, bApply, bAllowOnline) {
		var aPtr = [stringToNewUTF8(acc), stringToNewUTF8(name)];
		try {
			return JSON.parse(UTF8ToString(_SboWebAccRename(aPtr[0], aPtr[1], bApply ? 1 : 0, bAllowOnline ? 1 : 0)));
		} finally {
			aPtr.forEach(function (p) { _free(p); });
		}
	}

	function apiDeviceRename(body, req, res) {
		var ip = clientIp(req);
		var nWait = limitRenameIp.check(ip);
		if (nWait > 0) {
			reply(res, 429, { result: 'too_many', retryAfterSeconds: nWait });
			return;
		}
		limitRenameIp.fail(ip);
		var t = findToken(body.token);
		var info = t ? accInfo(t.info.account) : null;
		if (!info) {
			reply(res, 401, { result: 'invalid_token' });
			return;
		}
		var name = str(body.name, 40).trim();
		var nAfter = renameAfter(info.account);
		if (nAfter) {
			reply(res, 200, { result: 'cooldown', renameAfter: nAfter });
			return;
		}
		if (name && hasNgWord(name)) {
			reply(res, 200, { result: 'ng_word' });
			return;
		}
		if (name && isReservedName(name, info.account)) {
			reply(res, 200, { result: 'reserved', reserveDays: RENAME_RESERVE_DAYS });
			return;
		}
		var bApply = !body.check;
		var r = callRename(info.account, name, bApply, false);
		if (r.result === 'ok' && bApply) {
			renameRecord(info.account, r.from, r.to, 'self', ip);
			log('キャラ名の変更（本人）: ' + info.account + ' ' + r.from + ' → ' + r.to + '（' + ip + '）');
			r.renameAfter = renameAfter(info.account);
		}
		reply(res, 200, r);
	}

	/* ------------------------------------------------------------------ */
	/* 承認ページ（管理者）の API												 */
	/* ------------------------------------------------------------------ */
	function apiAdminLogin(body, req, res) {
		var ip = clientIp(req);
		var nWait = limitAdminIp.check(ip);
		if (nWait > 0) {
			reply(res, 429, { error: 'too_many', retryAfterSeconds: nWait });
			return;
		}
		if (!callInt(_SboWebAccCheckAdmin, [str(body.account, 32), str(body.password, 32)])) {
			limitAdminIp.fail(ip);
			log('承認ページ: ログイン失敗（' + ip + '）');
			reply(res, 401, { error: 'login' });
			return;
		}
		limitAdminIp.reset(ip);
		var session = randomHex(32);
		aAdminSession[sha256(session)] = Date.now() + 2 * HOUR;
		reply(res, 200, { session: session });
	}

	/* 管理ページのログインが続いているか（続いていれば期限を延ばす） */
	function isAdminSession(session) {
		var hash = sha256(str(session, 128));
		var until = aAdminSession[hash];
		if (!until || until < Date.now()) {
			delete aAdminSession[hash];
			return false;
		}
		/* 使っている間は延ばす（最後に使ってから 2 時間） */
		aAdminSession[hash] = Date.now() + 2 * HOUR;
		return true;
	}

	function checkAdmin(body, res) {
		if (!isAdminSession(body.session)) {
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
		var aOut = store.requests.slice(-100).reverse().map(function (r) {
			var info = accInfo(r.account) || {};
			return {
				id: r.id, status: r.status, created: r.created, ip: r.ip, message: r.message,
				byAccount: r.byAccount, byPassword: r.byPassword,
				account: r.account, name: info.name || '', level: info.level || 0,
				made: info.made || '', playMinutes: info.playMinutes || 0, loginCount: info.loginCount || 0
			};
		});
		reply(res, 200, { requests: aOut, policy: RECOVER_POLICY });
	}

	function apiAdminDecide(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		var r = findRequest(str(body.id));
		if (!r || r.status !== 'pending') {
			reply(res, 404, { error: 'not_pending' });
			return;
		}
		r.status = body.approve ? 'approved' : 'rejected';
		r.decided = Date.now();
		save();
		log('復旧: 申請 ' + r.id + ' を' + (body.approve ? '承認' : '却下') + ' ' + r.account);
		reply(res, 200, { ok: true });
	}

	/* 管理ページの表示用に、端末トークン・復旧キー・名前の変更の記録を足す */
	function adminCharDetail(info) {
		load();
		var nTokens = 0, nLastUsed = 0;
		Object.keys(store.tokens).forEach(function (hash) {
			var tk = store.tokens[hash];
			if (tk.account === info.account) {
				nTokens++;
				nLastUsed = Math.max(nLastUsed, tk.lastUsed || tk.created || 0);
			}
		});
		info.devices = nTokens;
		info.lastWebLogin = nLastUsed;
		info.hasCode = Object.keys(store.codes).some(function (h) { return store.codes[h].account === info.account; });
		info.renames = store.renames.filter(function (r) { return r.account === info.account; })
			.slice(-10).reverse().map(function (r) { return { from: r.from, to: r.to, by: r.by, at: r.at }; });
		info.renameAfter = renameAfter(info.account);
		return info;
	}

	function apiAdminSearch(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		var q = str(body.q, 40).trim();
		if (!q) {
			reply(res, 200, { results: [], more: false });
			return;
		}
		var MAX = 30;
		var r = JSON.parse(callStr(_SboWebAdminSearch, [q, MAX]));
		var aSeen = {};
		r.results.forEach(function (c) { aSeen[c.account] = true; });
		/* 前の名前からも探す */
		load();
		store.renames.slice().reverse().forEach(function (rn) {
			if (r.results.length >= MAX || aSeen[rn.account]) {
				return;
			}
			if (String(rn.from).indexOf(q) >= 0) {
				var info = accInfo(rn.account);
				if (info) {
					info.matchedOldName = rn.from;
					r.results.push(info);
					aSeen[rn.account] = true;
				}
			}
		});
		r.results = r.results.map(adminCharDetail);
		reply(res, 200, r);
	}

	function apiAdminRename(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		var acc = str(body.account, 32);
		var bApply = !body.check;
		var r = callRename(acc, str(body.name, 40).trim(), bApply, true);
		if (r.result === 'ok' && bApply) {
			renameRecord(acc, r.from, r.to, 'admin', clientIp(req));
			log('キャラ名の変更（管理者）: ' + acc + ' ' + r.from + ' → ' + r.to);
			var info = accInfo(acc);
			r.char = info ? adminCharDetail(info) : null;
		}
		reply(res, 200, r);
	}

	var API = {
		'/api/device/me': apiDeviceMe,
		'/api/device/forget': apiDeviceForget,
		'/api/device/set-password': apiDeviceSetPassword,
		'/api/device/issue-code': apiDeviceIssueCode,
		'/api/device/redeem': apiDeviceRedeem,
		'/api/recover': apiRecover,
		'/api/recover/status': apiRecoverStatus,
		'/api/admin/login': apiAdminLogin,
		'/api/admin/recover/list': apiAdminList,
		'/api/admin/recover/decide': apiAdminDecide,
		'/api/device/rename': apiDeviceRename,
		'/api/admin/search': apiAdminSearch,
		'/api/admin/rename': apiAdminRename
	};

	/* sbosvr_net.js の HTTP から呼ばれる。/api/ で始まるものだけ受け持つ */
	function handleHttp(req, res, url) {
		if (url.indexOf('/api/') !== 0) {
			return false;
		}
		var fn = API[url];
		if (!fn) {
			reply(res, 404, { error: 'not_found' });
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
			try {
				fn(body, req, res);
			} catch (e) {
				console.error('[account] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	/* 端末トークンからアカウントの情報を引く（ご意見箱などほかのページ用。見つからなければ null） */
	function tokenInfo(token) {
		var t = findToken(token);
		return t ? accInfo(t.info.account) : null;
	}

	return {
		handleHttp: handleHttp,
		tokenToAccount: tokenToAccount,
		issueToken: issueToken,
		/* sbosvr_feedback.js などから使う */
		tokenInfo: tokenInfo,
		accInfo: accInfo,
		isAdminSession: isAdminSession,
		clientIp: clientIp,
		reply: reply,
		Limiter: Limiter
	};
})();
