#!/usr/bin/env node
/*
 * ドット絵工房の API の確認（server/node/sbosvr_kobo.js を stub-server 越しに fetch で一通り試す）。
 *
 *   node tools/kobo/test-api.js
 *
 * stub-server を子プロセスで 2 つ（制限なし --fast／制限あり）、一時フォルダで起動する。Node 18 以上。
 */
'use strict';
var fs = require('fs');
var os = require('os');
var path = require('path');
var zlib = require('zlib');
var cp = require('child_process');
var url = require('url');

var ROOT = path.resolve(__dirname, '..', '..');
var STUB = path.join(__dirname, 'stub-server.js');
var nOk = 0, nNg = 0;

function ok(cond, msg) {
	if (cond) {
		nOk++;
	} else {
		nNg++;
		console.log('  NG: ' + msg);
	}
}
function eq(a, b, msg) {
	ok(JSON.stringify(a) === JSON.stringify(b), msg + '（' + JSON.stringify(a) + ' != ' + JSON.stringify(b) + '）');
}
function section(s) {
	console.log('- ' + s);
}

/* ---------------- stub-server の起動 ---------------- */
function startStub(bFast) {
	return new Promise(function (resolve, reject) {
		var dir = fs.mkdtempSync(path.join(os.tmpdir(), 'kobo-test-'));
		var a = [STUB, '--port', '0', '--data', dir];
		if (bFast) {
			a.push('--fast');
		}
		var child = cp.spawn(process.execPath, a, { stdio: ['ignore', 'pipe', 'inherit'] });
		var buf = '';
		child.stdout.on('data', function (d) {
			buf += d;
			var m = /listening (http:\/\/127\.0\.0\.1:\d+)/.exec(buf);
			if (m) {
				resolve({ base: m[1], dir: dir, child: child });
			}
		});
		child.on('exit', function (c) { reject(new Error('stub-server が終わった: ' + c)); });
	});
}

function client(base) {
	var c = {
		post: function (u, body) {
			return fetch(base + u, {
				method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body || {})
			}).then(function (r) {
				return r.json().then(function (j) { j._status = r.status; return j; });
			});
		},
		get: function (u) {
			return fetch(base + u).then(function (r) {
				return r.arrayBuffer().then(function (b) { return { status: r.status, headers: r.headers, body: Buffer.from(b) }; });
			});
		}
	};
	c.admin = function (u, body) {
		return c.post('/api/admin/kobo/' + u, Object.assign({ session: 'admin' }, body || {}));
	};
	c.p = function (tok, u, body) {
		return c.post('/api/kobo/' + u, Object.assign({ token: tok }, body || {}));
	};
	return c;
}

/* ---------------- PNG を作る（フィルタ指定つき） ---------------- */
var crcTable = (function () {
	var t = [];
	for (var n = 0; n < 256; n++) {
		var c = n;
		for (var k = 0; k < 8; k++) {
			c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
		}
		t.push(c >>> 0);
	}
	return t;
})();
function crc32(buf) {
	var c = 0xffffffff;
	for (var i = 0; i < buf.length; i++) {
		c = crcTable[(c ^ buf[i]) & 255] ^ (c >>> 8);
	}
	return (c ^ 0xffffffff) >>> 0;
}
function chunk(type, data) {
	var b = Buffer.alloc(12 + data.length);
	b.writeUInt32BE(data.length, 0);
	b.write(type, 4, 'latin1');
	data.copy(b, 8);
	b.writeUInt32BE(crc32(b.subarray(4, 8 + data.length)), 8 + data.length);
	return b;
}
function paeth(a, b, c) {
	var p = a + b - c, pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c);
	return (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
}
/* rows: 1 行 = stride バイト。filters: 行ごとのフィルタ種別（省略は 0） */
function makePng(w, h, ctype, bpp, rows, plte, filterOf) {
	var stride = w * bpp, raw = Buffer.alloc(h * (stride + 1));
	for (var y = 0; y < h; y++) {
		var f = filterOf ? filterOf(y) : 0;
		raw[y * (stride + 1)] = f;
		for (var x = 0; x < stride; x++) {
			var v = rows[y * stride + x];
			var a = x >= bpp ? rows[y * stride + x - bpp] : 0;
			var b = y > 0 ? rows[(y - 1) * stride + x] : 0;
			var c = (x >= bpp && y > 0) ? rows[(y - 1) * stride + x - bpp] : 0;
			var pred = [0, a, b, (a + b) >> 1, paeth(a, b, c)][f];
			raw[y * (stride + 1) + 1 + x] = (v - pred) & 255;
		}
	}
	var ihdr = Buffer.alloc(13);
	ihdr.writeUInt32BE(w, 0);
	ihdr.writeUInt32BE(h, 4);
	ihdr[8] = 8;
	ihdr[9] = ctype;
	var a = [Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), chunk('IHDR', ihdr)];
	if (plte) {
		a.push(chunk('PLTE', plte));
	}
	a.push(chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0)));
	return Buffer.concat(a);
}
function indexedPng(w, h, indices, palette, filterOf) {
	var plte = Buffer.alloc(768);
	palette.forEach(function (c, i) {
		plte[i * 3] = c[0]; plte[i * 3 + 1] = c[1]; plte[i * 3 + 2] = c[2];
	});
	return makePng(w, h, 3, 1, indices, plte, filterOf);
}
var b64 = function (b) { return b.toString('base64'); };

async function main() {
	/* 元の画像：SboGrpData/grp/char05.png を dot-png.js で読んで切り出す */
	var png = await import(url.pathToFileURL(path.join(ROOT, 'web/account/dot-png.js')).href);
	var sheet = await png.decodeIndexedPng(fs.readFileSync(path.join(ROOT, 'SboGrpData/grp/char05.png')));
	var palette = sheet.palette.map(function (c) { return [c.r, c.g, c.b]; });
	function crop(x, y, w, h) {
		var a = new Uint8Array(w * h);
		for (var j = 0; j < h; j++) {
			for (var i = 0; i < w; i++) {
				a[j * w + i] = sheet.indices[(y + j) * sheet.width + x + i];
			}
		}
		return a;
	}
	var W = 32, H = 16;
	/* 色が 3 つ以上ある場所を探す */
	var base = null;
	for (var sy = 0; sy + H <= sheet.height && !base; sy += H) {
		var cand = crop(0, sy, W, H), kinds = {};
		cand.forEach(function (n) { kinds[n] = true; });
		if (Object.keys(kinds).length >= 3) {
			base = cand;
		}
	}
	var usedMap = {};
	base.forEach(function (n) { usedMap[n] = true; });
	var colors = Object.keys(usedMap).map(Number).sort(function (a, b) { return a - b; });
	var TRANS = colors[0];
	ok(colors.length >= 3, '切り出した絵に色がある');
	var unusable = 0;
	for (var i = 1; i < 256; i++) {
		if (colors.indexOf(i) < 0) { unusable = i; break; }
	}
	function draw(seed, filterOf) {
		var a = Uint8Array.from(base);
		a[(seed * 7) % a.length] = colors[1 + (seed % (colors.length - 1))];
		return b64(indexedPng(W, H, a, palette, filterOf));
	}
	var baseData = b64(indexedPng(W, H, base, palette));
	var rough = b64(makePng(8, 8, 6, 4, Buffer.alloc(8 * 8 * 4, 200)));	/* RGBA のラフ絵 */

	/* ================= 制限なし（--fast）のサーバー ================= */
	var S = await startStub(true);
	var api = client(S.base);
	try {
		section('管理者：ログイン確認・お題を作る');
		var r = await api.post('/api/admin/kobo/tasks', { session: 'bad' });
		eq(r._status, 401, '管理者でなければ 401');
		eq(r.error, 'session', 'session エラー');
		r = await api.p('tokX', 'me');
		eq(r.error, 'invalid_token', '知らないトークンは invalid_token');
		r = await api.p('tokA', 'me');
		eq([r.account, r.name, r.canPost, r.admin], ['acca', 'アリス', true, false], 'me');
		r = await api.p('tokC', 'me');
		eq(r.canPost, false, 'キャラ無しは canPost:false');

		/* 提案（アリスが匿名で）→ お題にする */
		r = await api.p('tokA', 'propose', { kind: 'char', body: 'こういうの', data: rough, anonymous: true, agree: false });
		eq(r.error, 'agree', '提案は同意が要る');
		r = await api.p('tokC', 'propose', { kind: 'char', body: 'x', agree: true });
		eq(r.error, 'no_char', '提案はキャラが要る');
		r = await api.p('tokA', 'propose', { kind: 'char', body: 'こういうの', data: 'AAAA', anonymous: true, agree: true });
		eq(r.error, 'bad_png', '壊れたラフ絵は断る');
		r = await api.p('tokA', 'propose', { kind: 'char', body: 'こういうの', data: b64(makePng(600, 4, 6, 4, Buffer.alloc(600 * 16))), anonymous: true, agree: true });
		eq(r.error, 'bad_size', '512 を超えるラフ絵は断る');
		r = await api.p('tokA', 'propose', { kind: 'char', body: 'こういうの', data: rough, anonymous: true, agree: true });
		ok(r.ok && r.proposal.status === 'new' && r.proposal.img, '提案できる');
		var propId = r.proposal.id;
		r = await api.p('tokA', 'my-proposals');
		eq(r.proposals.length, 1, 'my-proposals');
		r = await api.admin('tasks');
		eq(r.counts.proposals, 1, '新着の提案が数えられる');
		r = await api.admin('proposal-update', { id: propId, reply: 'いいね', status: 'later' });
		ok(r.ok && r.proposal.account === 'acca' && r.proposal.reply === 'いいね', '管理者は提案に返事・誰か分かる');
		r = await api.p('tokA', 'my-proposals');
		eq(r.proposals[0].reply, 'いいね', '返事が本人に見える');

		var taskBody = {
			title: '歩く女の子', desc: '説明', kind: 'char', state: 'open', sheet: 'char05.png',
			x: 0, y: 0, w: W, h: H, cw: 16, ch: 16, preview: 'walk', frameMs: 200,
			palette: palette, colors: colors, transparent: TRANS,
			baseData: baseData, bgData: baseData, refData: baseData, refType: 'png', samplesData: [baseData, baseData],
			proposalId: propId
		};
		r = await api.admin('task-save', Object.assign({}, taskBody, { baseData: 'AAAA' }));
		eq(r.error, 'bad_png', '壊れた基準は断る');
		r = await api.admin('task-save', Object.assign({}, taskBody, { title: '' }));
		eq(r.error, 'empty', '題名が要る');
		r = await api.admin('task-save', taskBody);
		ok(r.ok && r.task.id && r.task.base && r.task.bg && r.task.ref && r.task.samples.length === 2, 'お題を作れる');
		var task = r.task;
		eq(task.proposalId, propId, '提案から作ったお題');
		r = await api.admin('proposals', { status: 'tasked' });
		eq(r.proposals.length, 1, '提案が tasked に');
		ok(r.proposals[0].taskId === task.id, '提案の taskId');

		section('画像の配信');
		var im = await api.get('/kobo-img/' + task.base + '.png');
		eq(im.status, 200, '画像が取れる');
		eq(im.headers.get('content-type'), 'image/png', 'Content-Type');
		ok(/immutable/.test(im.headers.get('cache-control')), 'Cache-Control immutable');
		ok(im.body.equals(Buffer.from(baseData, 'base64')), '画像が同じ');
		im = await api.get('/kobo-img/' + task.ref + '.png');
		eq(im.status, 200, '元イラスト（png）');
		im = await api.get('/kobo-img/zzzzzzzzzzzz.png');
		eq(im.status, 404, '知らない id は 404');
		im = await api.get('/kobo-img/../../x.png');
		ok(im.status === 404, '変な id は 404');

		section('お題の一覧・見せ方');
		r = await api.post('/api/kobo/tasks', {});
		eq(r.tasks.length, 1, '一覧');
		eq([r.tasks[0].works, r.tasks[0].wip], [0, 0], '数');
		r = await api.post('/api/kobo/task', { id: task.id });
		eq(r.task.id, task.id, 'お題の詳細');
		r = await api.post('/api/kobo/task', { id: 'nothing' });
		eq(r._status, 404, '無いお題は 404');

		section('案：作業中の保存・上書き・共有・出す');
		r = await api.p('tokC', 'save', { taskId: task.id, data: draw(1) });
		eq(r.error, 'no_char', '保存はキャラが要る');
		r = await api.p('tokA', 'save', { taskId: task.id, data: draw(1), note: 'メモ', anonymous: false, share: false });
		ok(r.ok && r.work.status === 'wip' && r.work.mine, '一から作業中で保存');
		var wA = r.work;
		r = await api.p('tokB', 'task', {});
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokB' });
		eq(r.works.length, 0, '見せていない作業中はほかの人には見えない');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		eq(r.works.length, 1, '自分の作業中は見える');
		var oldImg = wA.img;
		r = await api.p('tokA', 'save', { taskId: task.id, workId: wA.id, data: draw(2), share: true, note: 'メモ2' });
		ok(r.ok && r.work.id === wA.id && r.work.img !== oldImg && r.work.share, '作業中を上書き（共有）');
		eq((await api.get('/kobo-img/' + oldImg + '.png')).status, 404, '前の絵は消える');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokB' });
		eq(r.works.length, 1, '見せている作業中は見える');
		r = await api.post('/api/kobo/tasks', {});
		eq(r.tasks[0].wip, 1, '見せている作業中の数');
		r = await api.p('tokB', 'save', { taskId: task.id, workId: wA.id, data: draw(3) });
		eq(r._status, 404, '人の案は上書きできない');
		r = await api.p('tokA', 'save', { taskId: task.id, workId: wA.id, data: draw(3), submit: true, agree: false });
		eq(r.error, 'agree', '出すには同意が要る');
		r = await api.p('tokA', 'save', { taskId: task.id, workId: wA.id, data: draw(3), submit: true, agree: true, note: 'できた' });
		ok(r.ok && r.work.status === 'posted' && r.work.postedAt > 0, '出す');
		wA = r.work;
		r = await api.p('tokA', 'save', { taskId: task.id, workId: wA.id, data: draw(4) });
		eq(r.error, 'not_allowed', '出した案は上書きできない');
		r = await api.p('tokA', 'delete', { id: wA.id });
		eq(r.error, 'not_allowed', '出した案は消せない');
		r = await api.p('tokA', 'save', { taskId: task.id, data: draw(5), parent: 'nothing' });
		eq(r.error, 'not_allowed', '親が無い');

		section('案：描き足し（親つき）・消す・フィルタ');
		r = await api.p('tokB', 'save', { taskId: task.id, parent: wA.id, data: draw(6, function (y) { return y % 5; }), anonymous: true, submit: true, agree: true });
		ok(r.ok && r.work.parent === wA.id && r.work.name === '' && r.work.anonymous, 'フィルタ 0〜4 の PNG で描き足して出す（匿名）');
		var wB = r.work;
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		var vB = r.works.filter(function (w) { return w.id === wB.id; })[0];
		ok(vB && vB.name === '' && !vB.mine, '匿名は名前が見えない');
		r = await api.p('tokB', 'save', { taskId: task.id, data: draw(7) });
		var wip1 = r.work;
		ok(r.ok, 'ボブの作業中を作る');
		r = await api.p('tokB', 'delete', { id: wip1.id });
		ok(r.ok, '自分の作業中は消せる');
		eq((await api.get('/kobo-img/' + wip1.img + '.png')).status, 404, '消した作業中の絵が消える');

		section('PNG の確認');
		var badPal = palette.map(function (c) { return c.slice(); });
		badPal[colors[1]][0] = (badPal[colors[1]][0] + 1) & 255;
		var a2 = Uint8Array.from(base);
		r = await api.p('tokA', 'save', { taskId: task.id, data: b64(indexedPng(W, H, a2, badPal)) });
		eq(r.error, 'bad_palette', 'パレット違い');
		r = await api.p('tokA', 'save', { taskId: task.id, data: b64(indexedPng(W + 1, H, new Uint8Array((W + 1) * H), palette)) });
		eq(r.error, 'bad_size', '大きさ違い');
		a2[3] = unusable;
		r = await api.p('tokA', 'save', { taskId: task.id, data: b64(indexedPng(W, H, a2, palette)) });
		eq(r.error, 'bad_palette', '使えない色');
		r = await api.p('tokA', 'save', { taskId: task.id, data: b64(makePng(W, H, 6, 4, Buffer.alloc(W * H * 4))) });
		eq(r.error, 'bad_png', 'RGBA は断る');
		r = await api.p('tokA', 'save', { taskId: task.id, data: 'bm90IGEgcG5n' });
		eq(r.error, 'bad_png', 'PNG でないもの');
		var shortPlte = Buffer.alloc(30);
		palette.slice(0, 10).forEach(function (c, i) { shortPlte[i * 3] = c[0]; shortPlte[i * 3 + 1] = c[1]; shortPlte[i * 3 + 2] = c[2]; });
		var pal2 = palette.map(function (c, i) { return i < 10 ? c : [0, 0, 0]; });
		var t2 = await api.admin('task-save', Object.assign({}, taskBody, {
			proposalId: undefined, title: '短いパレット', palette: pal2, colors: [0, 1, 2], transparent: 0, state: 'open',
			baseData: b64(indexedPng(W, H, new Uint8Array(W * H), pal2)), refData: undefined, bgData: undefined, samplesData: undefined
		}));
		ok(t2.ok, 'お題（短いパレット）');
		var short = makePng(W, H, 3, 1, new Uint8Array(W * H).fill(1), shortPlte);
		r = await api.p('tokA', 'save', { taskId: t2.task.id, data: b64(short) });
		ok(r.ok, 'PLTE が 768 未満でも残りは黒扱いで比べる');
		r = await api.admin('task-delete', { id: t2.task.id });
		ok(r.ok, 'お題を消す');

		section('コメント・返事・隠す');
		r = await api.p('tokC', 'comment', { workId: wA.id, body: 'やあ' });
		eq(r.error, 'no_char', 'コメントはキャラが要る');
		r = await api.p('tokB', 'comment', { workId: wA.id, body: '' });
		eq(r.error, 'empty', '空');
		r = await api.p('tokB', 'comment', { workId: wA.id, body: 'あ'.repeat(301) });
		eq(r.error, 'long', '長い');
		r = await api.p('tokB', 'comment', { workId: wA.id, body: 'いいね！' });
		ok(r.ok && r.comment.mine && r.comment.name === 'ボブ', 'コメント');
		var c1 = r.comment;
		r = await api.p('tokB', 'comment', { workId: wA.id, body: '返事', replyTo: c1.id });
		eq(r.error, 'not_allowed', '案の本人でない人は返事できない');
		r = await api.p('tokA', 'comment', { workId: wA.id, body: 'ありがとう', replyTo: c1.id });
		ok(r.ok && r.comment.replyTo === c1.id, '案の本人は返事できる');
		var c2 = r.comment;
		r = await api.admin('comment', { workId: wA.id, body: '管理人より', replyTo: c1.id });
		ok(r.ok && r.comment.admin && r.comment.name === '管理人', '管理者も返事できる');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokB' });
		eq(r.comments.length, 3, 'コメントが見える');
		eq(r.works.filter(function (w) { return w.id === wA.id; })[0].comments, 3, '案のコメント数');
		r = await api.p('tokB', 'hide-comment', { id: c1.id, hide: true });
		eq(r._status, 404, '人の案へのコメントは隠せない');
		r = await api.p('tokA', 'hide-comment', { id: c1.id, hide: true });
		ok(r.ok, '自分の案へのコメントを隠す');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokB' });
		ok(!r.comments.some(function (c) { return c.id === c1.id; }), 'ほかの人には見えない');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		var seen = r.comments.filter(function (c) { return c.id === c1.id; })[0];
		ok(seen && seen.hiddenByAuthor, '本人には hiddenByAuthor で見える');
		r = await api.admin('tasks');
		eq(r.counts.hiddenComments, 1, '管理者に届く（件数）');
		r = await api.admin('task', { id: task.id });
		var ac = r.comments.filter(function (c) { return c.id === c1.id; })[0];
		ok(ac && ac.hiddenByAuthor && ac.account === 'accb', '管理者には本人が隠したものも見える');
		ok(r.works.every(function (w) { return w.account; }), '管理者の案には account が付く');
		r = await api.admin('comment-update', { id: c2.id, hidden: true });
		ok(r.ok, '管理者がコメントを隠す');

		section('なじんでる');
		r = await api.p('tokA', 'cheer', { workId: wA.id, on: true });
		eq(r.error, 'not_allowed', '自分の案には付けられない');
		r = await api.p('tokB', 'cheer', { workId: wA.id, on: true });
		eq(r.cheers, 1, 'なじんでる');
		r = await api.p('tokB', 'cheer', { workId: wA.id, on: true });
		eq(r.cheers, 1, '1 人 1 回');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokB' });
		eq(r.myCheers, [wA.id], 'myCheers');
		r = await api.p('tokB', 'cheer', { workId: wA.id, on: false });
		eq(r.cheers, 0, '外す');
		r = await api.p('tokB', 'cheer', { workId: wA.id, on: true });
		eq(r.cheers, 1, 'また付ける');

		section('管理者：隠す・消す・採用');
		r = await api.admin('work', { id: wB.id, hidden: true });
		ok(r.ok, '案を隠す');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		ok(!r.works.some(function (w) { return w.id === wB.id; }), '隠した案は見えない');
		r = await api.admin('work', { id: wB.id, hidden: false });
		r = await api.admin('adopt', { taskId: task.id, workId: 'nothing' });
		eq(r._status, 404, '無い案は採用できない');
		r = await api.admin('adopt', { taskId: task.id, workId: wB.id });
		ok(r.ok && r.task.state === 'done', '採用');
		eq(r.task.adopted.credits, [
			{ role: 'draw', name: '', anonymous: true },
			{ role: 'base', name: 'アリス', anonymous: false },
			{ role: 'proposal', name: '', anonymous: true }
		], 'credits（描いた人・元の案の人・提案した人。匿名は名前なし）');
		r = await api.p('tokA', 'save', { taskId: task.id, data: draw(8) });
		eq(r.error, 'closed', '募集中でなければ保存できない');
		r = await api.post('/api/kobo/tasks', {});
		ok(r.tasks[0].adoptedImg === wB.img && r.tasks[0].works === 2, '一覧に採用した絵・数');
		r = await api.admin('adopt', { taskId: task.id, workId: '' });
		ok(r.ok && !r.task.adopted && r.task.state === 'judging', '採用の取り消し');
		r = await api.admin('task-save', Object.assign({ id: task.id }, taskBody, { state: 'open', baseData: undefined, bgData: undefined, refData: undefined, samplesData: undefined, proposalId: undefined }));
		ok(r.ok && r.task.base === task.base && r.task.ref === task.ref && r.task.samples.length === 2, '編集で省いた画像は元のまま');
		r = await api.admin('task-save', Object.assign({ id: task.id }, taskBody, { baseData: undefined, bgData: '', refData: undefined, samplesData: [], proposalId: undefined }));
		ok(r.ok && r.task.bg === '' && r.task.samples.length === 0, '背景を外す・見本を入れ替え');
		eq((await api.get('/kobo-img/' + task.bg + '.png')).status, 404, '外した画像は消える');

		section('新しく描くお題（白紙から・今の絵なし）');
		var freshBody = Object.assign({}, taskBody, { title: '新しい敵', fresh: true, baseData: undefined, bgData: undefined, refData: undefined, samplesData: undefined, proposalId: undefined });
		r = await api.admin('task-save', freshBody);
		ok(r.ok && r.task.fresh === true && !r.task.base, '今の絵なしで作れる');
		var tFresh = r.task;
		r = await api.post('/api/kobo/task', { id: tFresh.id, token: 'tokA' });
		ok(r.task && r.task.fresh && !r.task.base, 'プレイヤーにも新規と分かる');
		r = await api.p('tokA', 'save', { taskId: tFresh.id, data: draw(3), post: true, note: '', anonymous: false });
		ok(r.ok, '新規のお題にも出せる');
		r = await api.admin('task-save', Object.assign({ id: tFresh.id }, freshBody, { fresh: false }));
		eq(r.error, 'bad_png', '描き直しに切り替える時は今の絵が要る');
		r = await api.admin('task-save', Object.assign({ id: tFresh.id }, freshBody, { fresh: false, baseData: baseData }));
		ok(r.ok && !r.task.fresh && r.task.base, '描き直しに切り替え');
		var oldBase = r.task.base;
		r = await api.admin('task-save', Object.assign({ id: tFresh.id }, freshBody));
		ok(r.ok && r.task.fresh && !r.task.base, '新規に戻すと今の絵は外れる');
		eq((await api.get('/kobo-img/' + oldBase + '.png')).status, 404, '外した今の絵は消える');
		r = await api.admin('task-delete', { id: tFresh.id });
		ok(r.ok, '新規のお題を消す');

		section('消した案（子がいる時は印だけ）');
		r = await api.admin('work', { id: wA.id, deleted: true });
		ok(r.ok, '子のいる案を消す');
		eq((await api.get('/kobo-img/' + wA.img + '.png')).status, 404, '絵は消える');
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		var stub = r.works.filter(function (w) { return w.id === wA.id; })[0];
		eq(stub, { id: wA.id, parent: '', deleted: true }, '印だけ残る');
		r = await api.p('tokA', 'save', { taskId: task.id, parent: wA.id, data: draw(9) });
		eq(r.error, 'not_allowed', '消した案は親にできない');
		r = await api.admin('work', { id: wB.id, deleted: true });
		r = await api.post('/api/kobo/task', { id: task.id, token: 'tokA' });
		eq(r.works.length, 0, '子が無くなれば親の印も消える');
		eq((await api.get('/kobo-img/' + wB.img + '.png')).status, 404, '絵も消える');

		section('作業中は 1 お題 3 つまで・お題を消す');
		var aWip = [];
		for (var k = 0; k < 4; k++) {
			r = await api.p('tokA', 'save', { taskId: task.id, data: draw(10 + k), share: true });
			if (k < 3) {
				ok(r.ok, '作業中 ' + (k + 1));
				aWip.push(r.work);
			} else {
				eq(r.error, 'too_many', '4 つ目は断る');
			}
		}
		r = await api.admin('task-delete', { id: task.id });
		ok(r.ok, 'お題を消す');
		eq((await api.get('/kobo-img/' + aWip[0].img + '.png')).status, 404, '案の絵も消える');
		eq((await api.get('/kobo-img/' + task.base + '.png')).status, 404, '基準の絵も消える');
		r = await api.admin('tasks');
		eq(r.tasks.length, 0, 'お題が無くなる');
		var left = fs.readdirSync(path.join(S.dir, 'SBODATA', 'kobo')).filter(function (f) { return /\.(png|jpg)$/.test(f); });
		eq(left.length, 1, '残る画像は提案のラフ絵だけ');
	} finally {
		S.child.kill();
	}

	/* ================= 制限ありのサーバー ================= */
	section('制限');
	var L = await startStub(false);
	api = client(L.base);
	try {
		var tb = Object.assign({
			title: '制限', desc: '', kind: 'char', state: 'open', sheet: 'char05.png', x: 0, y: 0, w: W, h: H,
			cw: 16, ch: 16, preview: 'walk', frameMs: 200, palette: palette, colors: colors, transparent: TRANS, baseData: baseData
		});
		r = await api.admin('task-save', tb);
		var T = r.task;
		r = await api.p('tokA', 'save', { taskId: T.id, data: draw(1), share: true });
		ok(r.ok, '最初の案');
		var w1 = r.work;
		r = await api.p('tokA', 'save', { taskId: T.id, data: draw(2) });
		eq(r.error, 'wait', '新しい案は 1 分に 1 件');
		ok(r.waitSeconds > 0 && r.waitSeconds <= 60, 'waitSeconds');
		r = await api.p('tokA', 'save', { taskId: T.id, workId: w1.id, data: draw(3), share: true });
		ok(r.ok, '作業中の上書き（最初の 1 回）');
		r = await api.p('tokA', 'save', { taskId: T.id, workId: w1.id, data: draw(4) });
		eq(r.error, 'wait', '上書きは 20 秒に 1 回');
		r = await api.p('tokB', 'save', { taskId: T.id, data: draw(2) });
		ok(r.ok, 'ほかのアカウントは別に数える');
		for (var n = 1; n <= 4; n++) {
			r = await api.p('tokB', 'comment', { workId: w1.id, body: 'c' + n });
			if (n <= 3) {
				ok(r.ok, 'コメント ' + n);
			} else {
				eq(r.error, 'wait', 'コメントは 1 分に 3 件');
			}
		}
		for (n = 1; n <= 4; n++) {
			r = await api.p('tokA', 'propose', { kind: 'part', body: 'p' + n, agree: true });
			if (n <= 3) {
				ok(r.ok, '提案 ' + n);
			} else {
				eq(r.error, 'too_many', '提案は 1 日 3 件');
			}
		}
		r = await api.post('/api/kobo/me', {});
		eq(r._status, 401, 'token が無ければ 401');
	} finally {
		L.child.kill();
	}

	console.log('\n' + nOk + ' 件 OK、' + nNg + ' 件 NG');
	process.exit(nNg ? 1 : 0);
}

main().catch(function (e) {
	console.error(e);
	process.exit(1);
});
