#!/usr/bin/env node
/*
 * ドット絵工房の確認用の簡易サーバー（wasm をビルドせず Node だけで動く）。
 *
 *   node tools/kobo/stub-server.js --port 8790 --data /tmp/kobo-stub [--fast]
 *
 * ・server/node/sbosvr_kobo.js をそのまま読み込む（Module・FS と Module['sboAccount'] は偽物）
 * ・端末トークン tokA=アリス(acca)・tokB=ボブ(accb)・tokC=キャラ無し(accc)。管理者は session 'admin' か Cookie
 * ・管理ページ /admin は最初からログイン済みで開く（/api/admin/me が Cookie を付けて ok を返す）
 * ・未知の /api/* は 200 で {}
 * ・--fast … 案の保存・コメントの間隔の制限をなくす（1 日あたりの数の制限は残る）。--port 0 なら空いているポート
 */
'use strict';
var fs = require('fs');
var path = require('path');
var http = require('http');
var vm = require('vm');

var ROOT = path.resolve(__dirname, '..', '..');
var args = process.argv.slice(2);
function arg(name, def) {
	var i = args.indexOf('--' + name);
	return i >= 0 && i + 1 < args.length ? args[i + 1] : def;
}
var PORT = Number(arg('port', '8790'));
var DATA = path.resolve(arg('data', '/tmp/kobo-stub'));
var FAST = args.indexOf('--fast') >= 0;
fs.mkdirSync(path.join(DATA, 'SBODATA'), { recursive: true });

/* ---------------- sboAccount の偽物（sbosvr_account.js の同名の関数と同じ動き） ---------------- */
var TOKENS = {
	tokA: { account: 'acca', name: 'アリス' },
	tokB: { account: 'accb', name: 'ボブ' },
	tokC: { account: 'accc', name: '' }
};
var ADMIN_COOKIE = 'sbo_admin_stub';

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

function cookieValue(req, name) {
	var m = new RegExp('(?:^|;\\s*)' + name + '=([^;]*)').exec(req.headers['cookie'] || '');
	return m ? m[1] : '';
}

var fakeAccount = {
	tokenInfo: function (token) {
		var t = TOKENS[token];
		return t ? { account: t.account, name: t.name } : null;
	},
	accInfo: function (acc) {
		for (var k in TOKENS) {
			if (TOKENS[k].account === acc) {
				return { account: acc, name: TOKENS[k].name };
			}
		}
		return null;
	},
	isAdminSession: function (session) {
		return session === 'admin';
	},
	withAdminCookie: function (body, req) {
		if (cookieValue(req, ADMIN_COOKIE)) {
			body.session = 'admin';
		}
		return body;
	},
	clientIp: function (req) {
		var xff = req.headers['x-forwarded-for'];
		if (xff) {
			var a = String(xff).split(',');
			return a[a.length - 1].trim();
		}
		return String(req.socket.remoteAddress || '').replace(/^::ffff:/, '');
	},
	reply: function (res, status, obj) {
		res.writeHead(status, {
			'Content-Type': 'application/json; charset=utf-8',
			'Cache-Control': 'no-store'
		});
		res.end(JSON.stringify(obj));
	},
	Limiter: Limiter
};

/* ---------------- sbosvr_kobo.js を読み込む ---------------- */
var Module = { sboAccount: fakeAccount };
if (FAST) {
	Module['sboKoboLimits'] = { newWorkInterval: 0, wipInterval: 0, commentPerMinute: 100000 };
}
var FS = { cwd: function () { return DATA; } };
var code = fs.readFileSync(path.join(ROOT, 'server/node/sbosvr_kobo.js'), 'utf8');
new Function('Module', 'FS', 'require', 'console', code)(Module, FS, require, console);

/* ---------------- 静的ファイル ---------------- */
var MIME = {
	'.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.mjs': 'text/javascript; charset=utf-8',
	'.json': 'application/json; charset=utf-8', '.css': 'text/css; charset=utf-8', '.png': 'image/png',
	'.jpg': 'image/jpeg', '.jpeg': 'image/jpeg', '.svg': 'image/svg+xml', '.ico': 'image/x-icon', '.wasm': 'application/wasm'
};
var PAGES = { '/kobo': 'kobo.html', '/kobo-draw': 'kobo-draw.html', '/admin': 'admin.html', '/dot': 'dot.html' };
var DIRS = ['web/account', 'web/changes', 'web/shell'].map(function (d) { return path.join(ROOT, d); });

function sendFile(res, file) {
	fs.readFile(file, function (err, buf) {
		if (err) {
			res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
			res.end('not found');
			return;
		}
		res.writeHead(200, {
			'Content-Type': MIME[path.extname(file).toLowerCase()] || 'application/octet-stream',
			'Cache-Control': 'no-store'
		});
		res.end(buf);
	});
}

/* dir の下の rel だけ許す（.. や \ は断る） */
function safeJoin(dir, rel) {
	if (!rel || rel.indexOf('\0') >= 0 || rel.indexOf('\\') >= 0 || rel.split('/').indexOf('..') >= 0) {
		return '';
	}
	var f = path.join(dir, rel);
	return f.indexOf(dir + path.sep) === 0 ? f : '';
}

function serveStatic(req, res, url) {
	var rel;
	try {
		rel = decodeURIComponent(url);
	} catch (e) {
		rel = '';
	}
	if (rel.indexOf('/admin-grp/') === 0) {
		var dirGrp = path.join(ROOT, 'SboGrpData', 'grp');
		var name = rel.slice('/admin-grp/'.length);
		if (name === 'index.json') {
			var files = fs.readdirSync(dirGrp).filter(function (f) { return /\.png$/i.test(f); }).sort();
			res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
			res.end(JSON.stringify({ files: files }));
			return;
		}
		var f = safeJoin(dirGrp, name);
		if (!f) {
			res.writeHead(404);
			res.end('not found');
			return;
		}
		sendFile(res, f);
		return;
	}
	var strPage = PAGES[rel] || rel.replace(/^\//, '');
	for (var i = 0; i < DIRS.length; i++) {
		var file = safeJoin(DIRS[i], strPage);
		if (file && fs.existsSync(file) && fs.statSync(file).isFile()) {
			sendFile(res, file);
			return;
		}
	}
	res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
	res.end('not found');
}

/* ---------------- 管理ページのログイン関係の偽物 ---------------- */
function readBody(req, cb) {
	var a = [];
	req.on('data', function (b) { a.push(b); });
	req.on('end', function () {
		var o = {};
		try {
			o = JSON.parse(Buffer.concat(a).toString('utf8') || '{}') || {};
		} catch (e) { /* 空でよい */ }
		cb(o);
	});
}

function setLogin(res, bOn) {
	res.setHeader('Set-Cookie', ADMIN_COOKIE + '=' + (bOn ? '1; Path=/; HttpOnly; SameSite=Strict' : '; Path=/; Max-Age=0'));
}

function fakeApi(req, res, url) {
	readBody(req, function (body) {
		if (url === '/api/admin/me') {
			setLogin(res, true);		/* 最初からログイン済み */
			fakeAccount.reply(res, 200, { ok: true, remember: true, created: Date.now(), lastUsed: Date.now() });
		} else if (url === '/api/admin/login') {
			setLogin(res, true);
			fakeAccount.reply(res, 200, { ok: true, remember: !!body.remember });
		} else if (url === '/api/admin/logout') {
			setLogin(res, false);
			fakeAccount.reply(res, 200, { ok: true });
		} else if (url === '/api/admin/sessions') {
			fakeAccount.reply(res, 200, { sessions: [], rememberDays: 30, maxDays: 90 });
		} else {
			fakeAccount.reply(res, 200, {});
		}
	});
}

var server = http.createServer(function (req, res) {
	var url = req.url.split('?')[0];
	if (Module['sboKobo'].handleHttp(req, res, url)) {
		return;
	}
	if (url.indexOf('/api/') === 0) {
		fakeApi(req, res, url);
		return;
	}
	if (url === '/') {
		res.writeHead(302, { Location: '/kobo' });
		res.end();
		return;
	}
	serveStatic(req, res, url);
});
server.listen(PORT, '127.0.0.1', function () {
	console.log('listening http://127.0.0.1:' + server.address().port + ' data=' + DATA + (FAST ? ' (fast)' : ''));
});
