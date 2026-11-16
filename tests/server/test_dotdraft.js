/* ドット絵エディタの控え（server/node/sbosvr_dotdraft.js）の単体テスト
 *   実行: node tests/server/test_dotdraft.js    （「全部 OK」と出れば成功）
 *   管理者ログインの確かめ・保存・一覧・読み出し・上書きのぶつかり・名前変更・削除・おかしな中身を断るか。
 */
var assert = require('assert');
var fs = require('fs');
var os = require('os');
var path = require('path');
var EventEmitter = require('events');

var strDir = fs.mkdtempSync(path.join(os.tmpdir(), 'sbodot-'));
fs.mkdirSync(path.join(strDir, 'SBODATA'));
global.FS = { cwd: function () { return strDir; } };
global.Module = {};
/* 管理者ログインの代わり: Cookie の値が 'good' なら管理者 */
Module['sboAccount'] = {
	isAdminSession: function (s) { return s === 'good'; },
	withAdminCookie: function (body, req) {
		var m = /sbo_admin=([^;]+)/.exec(req.headers.cookie || '');
		if (m) {
			body.session = m[1];
		}
		return body;
	},
	reply: function (res, status, obj) { res.status = status; res.body = obj; res.done(); }
};
console.log = function () {};
require('../../server/node/sbosvr_dotdraft.js');
var dd = Module['sboDotDraft'];

var nOk = 0;
function eq(a, b, msg) {
	assert.deepStrictEqual(a, b, msg);
	nOk++;
}

function call(url, body, cookie, method) {
	return new Promise(function (resolve) {
		var req = new EventEmitter();
		req.method = method || 'POST';
		req.headers = { cookie: cookie === undefined ? 'sbo_admin=good' : cookie };
		req.resume = function () {};
		var res = { done: function () { resolve(res); } };
		var handled = dd.handleHttp(req, res, url);
		if (!handled) {
			resolve(null);
			return;
		}
		process.nextTick(function () {
			req.emit('data', Buffer.from(JSON.stringify(body || {})));
			req.emit('end');
		});
	});
}

function layer(w, h, v) {
	var a = new Int16Array(w * h).fill(v);
	return Buffer.from(a.buffer).toString('base64');
}

function draft(id, extra) {
	return Object.assign({
		id: id, file: 'char01.png', name: '試し', width: 4, height: 3,
		palette: [{ r: 0, g: 0, b: 0 }, { r: 255, g: 0, b: 0, a: 255 }],
		layers: [{ name: '元の絵', visible: true, ghost: false, data: layer(4, 3, 1) }],
		thumbnail: 'data:image/png;base64,AAAA', kind: 'manual'
	}, extra || {});
}

(async function () {
	var r;
	/* 受け持たない URL */
	eq(await call('/api/admin/changes/list', {}), null, '他の URL は受け持たない');
	/* ログイン */
	r = await call('/api/admin/dot/drafts/list', {}, 'sbo_admin=bad');
	eq(r.status, 401, '違う Cookie は 401');
	r = await call('/api/admin/dot/drafts/list', {}, '');
	eq(r.status, 401, 'ログイン無しは 401');
	r = await call('/api/admin/dot/drafts/list', {}, '', 'GET');
	eq(r.status, 405, 'GET は断る');
	r = await call('/api/admin/dot/drafts/list', {});
	eq([r.status, r.body.items], [200, []], '最初は空');

	/* 保存 → 一覧 → 読み出し */
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-abc') });
	eq(r.body.ok, true, '保存できる');
	var saved = r.body.item.savedAt;
	eq(typeof saved, 'number');
	eq(r.body.item.layers, undefined, '一覧の分にレイヤーは入らない');
	r = await call('/api/admin/dot/drafts/list', {});
	eq(r.body.items.map(function (m) { return [m.id, m.name, m.layerCount, m.thumbnail]; }), [['d-abc', '試し', 1, 'data:image/png;base64,AAAA']]);
	r = await call('/api/admin/dot/drafts/get', { id: 'd-abc' });
	eq(r.body.item.layers[0].data, layer(4, 3, 1), 'レイヤーの中身がそのまま戻る');
	eq(r.body.item.palette[1], { r: 255, g: 0, b: 0, a: 255 });
	eq(r.body.item.palette[0].a, 255, 'a が無ければ 255');

	/* 上書き（開いた時の時刻が合っていれば上書き、違えばぶつかり） */
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-abc', { layers: [{ name: 'x', visible: true, data: layer(4, 3, 2) }] }), baseSavedAt: saved });
	eq(r.body.ok, true, '同じ時刻なら上書き');
	var saved2 = r.body.item.savedAt;
	assert(saved2 > saved, '上書きで時刻が進む');
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-abc'), baseSavedAt: saved });
	eq(r.body.error, 'conflict', '古い時刻からの上書きはぶつかり');
	r = await call('/api/admin/dot/drafts/get', { id: 'd-abc' });
	eq(r.body.item.layers[0].data, layer(4, 3, 2), 'ぶつかった時は中身を変えない');

	/* 名前変更（時刻は変えない） */
	r = await call('/api/admin/dot/drafts/rename', { id: 'd-abc', name: '  新しい名前 ' });
	eq([r.body.ok, r.body.item.name, r.body.item.savedAt], [true, '新しい名前', saved2]);
	r = await call('/api/admin/dot/drafts/rename', { id: 'd-zzz', name: 'a' });
	eq(r.status, 404);

	/* おかしな中身を断る */
	var bad = [
		draft('../x'),
		draft('d-1', { width: 0 }),
		draft('d-1', { width: 5000 }),
		draft('d-1', { layers: [{ data: layer(4, 2, 0) }] }),
		draft('d-1', { layers: [{ data: '!!!' }] }),
		draft('d-1', { layers: [] }),
		draft('d-1', { palette: [] })
	];
	for (var i = 0; i < bad.length; i++) {
		r = await call('/api/admin/dot/drafts/save', { item: bad[i] });
		eq(r.status, 400, 'おかしな控え ' + i + ' は断る');
	}
	/* gzip で縮めたレイヤー（大きさが合えば受け取り、縮めたまま置く） */
	var zlib = require('zlib');
	var gz = zlib.gzipSync(Buffer.from(new Int16Array(12).fill(3).buffer)).toString('base64');
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-gz', { layers: [{ name: 'g', visible: true, data: gz, enc: 'gz' }] }) });
	eq(r.body.ok, true, 'gzip のレイヤーを受け取る');
	r = await call('/api/admin/dot/drafts/get', { id: 'd-gz' });
	eq([r.body.item.layers[0].data, r.body.item.layers[0].enc], [gz, 'gz'], '縮めたまま返す');
	var gzBad = zlib.gzipSync(Buffer.from(new Int16Array(11).buffer)).toString('base64');
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-gz2', { layers: [{ data: gzBad, enc: 'gz' }] }) });
	eq(r.status, 400, 'ほどいた大きさが違う gzip は断る');
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-gz3', { layers: [{ data: layer(4, 3, 0), enc: 'gz' }] }) });
	eq(r.status, 400, 'gzip でない中身に gz は断る');
	await call('/api/admin/dot/drafts/delete', { id: 'd-gz' });

	/* 小さい絵が dataURL でなければ捨てる */
	r = await call('/api/admin/dot/drafts/save', { item: draft('d-2', { thumbnail: 'javascript:alert(1)' }) });
	eq(r.body.item.thumbnail, '', '変な小さい絵は捨てる');

	/* ファイルから読み直しても同じ（index.json） */
	var idx = JSON.parse(fs.readFileSync(path.join(strDir, 'SBODATA', 'SboWebDotDrafts', 'index.json'), 'utf8'));
	eq(idx.items.map(function (m) { return m.id; }), ['d-2', 'd-abc'], '新しい順に保存');

	/* 削除 */
	r = await call('/api/admin/dot/drafts/delete', { id: 'd-abc' });
	eq(r.body.ok, true);
	eq(fs.existsSync(path.join(strDir, 'SBODATA', 'SboWebDotDrafts', 'd-abc.json')), false, 'ファイルも消える');
	r = await call('/api/admin/dot/drafts/get', { id: 'd-abc' });
	eq(r.status, 404);
	r = await call('/api/admin/dot/drafts/delete', { id: 'd-abc' });
	eq(r.status, 404);

	fs.rmSync(strDir, { recursive: true, force: true });
	process.stdout.write('全部 OK（' + nOk + ' 件）\n');
})().catch(function (e) {
	process.stderr.write((e && e.stack || e) + '\n');
	process.exit(1);
});
