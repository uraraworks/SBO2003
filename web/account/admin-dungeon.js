/* ========================================================================= */
/* admin-dungeon.js															 */
/* 管理ページ（/admin）の「ダンジョン生成」									 */
/* ========================================================================= */
/*
 * 仕様は docs/dungeon-gen.md。テーマ（9分割の部屋・床や壁のパーツ・飾り）を登録し、お試し作成で結果を見る。
 *   API: /api/admin/dungeon/themes・theme/save・theme/delete・preview（server/node/sbosvr_dungeon.js）
 *   マス目の取り込み: /api/admin/data/tiles（admin-mapview.js と同じ）
 *   パーツの絵: admin-mapdoc.js の SboMapDoc.loadSheets / partCanvas（/admin-grp/）
 * お試し作成は、保存前の編集中のテーマをそのまま送って試せる。
 */
(function (root) {
	'use strict';

	var T = 16;							/* パーツの絵の大きさ */
	var api = null;						/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var bBound = false, loading = null, bReady = false;
	var sheets = null, parts = [], pcache = {};
	var themes = [];					/* 保存済み（サーバーの内容） */
	var cur = null;						/* 編集中のテーマ（id が無ければ未保存の新規） */
	var dirty = false;
	var selRoom = -1, selDeco = -1;
	var last = null;					/* 最後のお試し作成の結果 */
	var $root = null;

	/* 部分ごとの名前（テーマの項目） */
	var LISTS = [
		['floor', '床'], ['wallTop', '壁の天井（上段）'], ['wallFace', '壁の側面'], ['wallFill', 'その他の壁'],
		['arch', '出入口の床'], ['door', '扉（0 = 出入口と同じ）'], ['void', '何もない所（0 = 塗らない）']
	];
	var SCALAR = { door: 1, 'void': 1 };
	var KIND = ['ふつう', 'ボス', '入口'];
	var CELL_COLOR = ['#14161a', '#b9a87a', '#6b5a48', '#8f7c60', '#4d4136', '#e6d9a8', '#c06a2b'];
	var CELL_NAME = ['何もない', '床', '壁の天井', '壁の側面', '壁（その他）', '出入口の床', '扉'];
	var PARAMS = [
		['w', '幅', 40], ['h', '高さ', 40], ['seed', 'シード(0=ランダム)', 0], ['roomMin', '部屋の最小', 6], ['roomMax', '部屋の最大', 12],
		['floorMin', '床の最低 %', 35], ['archMin', '出入口の幅 最小', 2], ['archMax', '出入口の幅 最大', 4], ['doorRate', '扉 %', 20],
		['openRate', '広間 %', 8], ['loopRate', '輪にする %', 30], ['cutoff', '洞窟のやめる %', 25]
	];

	function $(id) { return document.getElementById(id); }
	function errText(r) { return (r && (r.message || r.error)) || ('HTTP ' + (r && r.httpStatus)); }
	function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }
	function num(v, def) { v = Math.floor(Number(v)); return isFinite(v) ? v : def; }
	function setMsg(text, bOk) {
		var e = $('dg-msg');
		if (!e) return;
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}

	/* ---------------------------------------------------------------- */
	/* 画面の部品																 */
	/* ---------------------------------------------------------------- */
	var CSS = '.dg-sec{margin:14px 0;padding:10px;border:1px solid var(--border);border-radius:8px}' +
		'.dg-sec h3{margin:0 0 6px;font-size:15px}' +
		'.dg-row{display:flex;flex-wrap:wrap;gap:6px 12px;align-items:center;margin:6px 0}' +
		'.dg-row label{display:inline-flex;align-items:center;gap:4px;margin:0}' +
		'.dg-row button,.dg-sec button{margin:4px 6px 0 0;padding:8px 12px;font-size:13px}' +
		'.dg-sec select,.dg-sec input[type=text]{width:auto;max-width:100%;box-sizing:border-box;background:var(--field);color:var(--fg);' +
		'border:1px solid var(--border);border-radius:6px;padding:6px 8px;font-size:16px}' +
		'.dg-n{width:5em!important}#dg-p-seed{width:9em!important}.dg-w{width:min(100%,22em)!important}' +
		'.dg-parts{display:grid;grid-template-columns:minmax(8em,12em) minmax(6em,16em) 1fr;gap:6px 8px;align-items:center}' +
		'.dg-thumbs{display:flex;flex-wrap:wrap;gap:2px}.dg-thumbs canvas{width:24px;height:24px;image-rendering:pixelated;border:1px solid var(--border)}' +
		'.dg-cols{display:flex;flex-wrap:wrap;gap:12px;align-items:flex-start}' +
		'.dg-scroll{overflow:auto;max-width:100%;max-height:75vh;border:1px solid var(--border)}' +
		'.dg-cv{display:block;image-rendering:pixelated;touch-action:none}' +
		'.dg-cap{font-size:12px;color:var(--sub);margin:2px 0}' +
		'.dg-legend span{display:inline-flex;align-items:center;gap:3px;margin-right:8px;font-size:12px}' +
		'.dg-legend i{display:inline-block;width:12px;height:12px;border:1px solid var(--border)}' +
		'@media(max-width:700px){.dg-parts{grid-template-columns:1fr}}';

	function build() {
		var h = '<style>' + CSS + '</style>';
		h += '<details class="mv-help"><summary>使い方</summary><p class="sub-text">ディアブロ1式のランダムダンジョンのテーマ（床・壁のパーツ、9分割の部屋、飾り）を登録し、' +
			'「お試し作成」で作った結果を見ます。お試し作成は、保存していない編集中のテーマでも試せます。ゲームの中にはまだつながっていません（仕様は docs/dungeon-gen.md）。</p></details>';
		h += '<div class="msg" id="dg-msg"></div>';
		h += '<div class="dg-sec"><h3>テーマ</h3><div class="dg-row"><select id="dg-sel"></select>' +
			'<button type="button" id="dg-new">追加</button><button type="button" id="dg-dup" class="sub">複製</button>' +
			'<button type="button" id="dg-del" class="sub">削除</button><button type="button" id="dg-save">保存</button>' +
			'<span class="sub-text" id="dg-dirty"></span></div>' +
			'<div class="dg-row"><button type="button" class="sub" id="dg-exp-all">全部を書き出す</button><button type="button" class="sub" id="dg-exp-one">選んだ1つを書き出す</button>' +
			'<label>読み込み <input type="file" id="dg-file" accept=".json,application/json"></label></div>' +
			'<div id="dg-imp" hidden><div class="dg-cap" id="dg-implist"></div><div class="dg-row">' +
			'<label><input type="radio" name="dg-impmode" value="add" checked> 追加として入れる（id を振り直す）</label>' +
			'<label><input type="radio" name="dg-impmode" value="replace"> 同じ id を上書き</label>' +
			'<button type="button" id="dg-impgo">入れる</button><button type="button" class="sub" id="dg-impcancel">やめる</button></div></div>' +
			'<div id="dg-edit" hidden>' +
			'<div class="dg-row"><label>名前 <input type="text" id="dg-name" maxlength="40"></label>' +
			'<label>作り方 <select id="dg-algo"><option value="1">1 = L1 聖堂</option><option value="3">3 = L3 洞窟</option></select></label></div>' +
			'<div class="dg-cap">パーツ番号はカンマ区切りで複数（床はくじで均等に混ぜます）。</div><div class="dg-parts" id="dg-lists"></div></div></div>';
		h += '<div class="dg-sec" id="dg-rooms-sec" hidden><h3>9分割の部屋（L1 用）</h3>' +
			'<div class="dg-row"><select id="dg-rsel"></select><button type="button" id="dg-rdel" class="sub">この部屋を削除</button></div>' +
			'<div class="dg-row"><label>マップ <input type="text" class="dg-n" id="dg-rm" inputmode="numeric" value="0"></label>' +
			'<label>x <input type="text" class="dg-n" id="dg-rx" inputmode="numeric" value="0"></label><label>y <input type="text" class="dg-n" id="dg-ry" inputmode="numeric" value="0"></label>' +
			'<label>w <input type="text" class="dg-n" id="dg-rw" inputmode="numeric" value="9"></label><label>h <input type="text" class="dg-n" id="dg-rh" inputmode="numeric" value="8"></label>' +
			'<button type="button" id="dg-rimp">マップから取り込む</button></div>' +
			'<div id="dg-redit" hidden><div class="dg-row"><label>名前 <input type="text" id="dg-rname" maxlength="40"></label>' +
			'<label>種類 <select id="dg-rkind"><option value="0">ふつう</option><option value="1">ボス</option><option value="2">入口</option></select></label>' +
			'<span class="sub-text" id="dg-rinfo"></span></div>' +
			'<div class="dg-cap">青い線（縦2本・横2本）をドラッグして切れ目を動かします。四隅はそのまま、辺は繰り返し、真ん中は縦横に繰り返します。</div>' +
			'<div class="dg-cols"><div><div class="dg-cap">元の部屋</div><div class="dg-scroll"><canvas class="dg-cv" id="dg-rcv"></canvas></div></div>' +
			'<div><div class="dg-cap" id="dg-s1c"></div><div class="dg-scroll"><canvas class="dg-cv" id="dg-s1"></canvas></div></div>' +
			'<div><div class="dg-cap" id="dg-s2c"></div><div class="dg-scroll"><canvas class="dg-cv" id="dg-s2"></canvas></div></div></div></div></div>';
		h += '<div class="dg-sec" id="dg-decos-sec" hidden><h3>飾り</h3>' +
			'<div class="dg-row"><select id="dg-dsel"></select><button type="button" id="dg-dadd">飾りを追加</button><button type="button" id="dg-ddel" class="sub">削除</button></div>' +
			'<div id="dg-dedit" hidden><div class="dg-row"><label>名前 <input type="text" id="dg-dname" maxlength="40"></label>' +
			'<label>w <input type="text" class="dg-n" id="dg-dw" inputmode="numeric"></label><label>h <input type="text" class="dg-n" id="dg-dh" inputmode="numeric"></label>' +
			'<label>置く所 <select id="dg-don"><option value="1">部屋の床</option><option value="3">壁の側面</option></select></label>' +
			'<label>rate % <input type="text" class="dg-n" id="dg-drate" inputmode="numeric"></label><label>max <input type="text" class="dg-n" id="dg-dmax" inputmode="numeric"></label></div>' +
			'<div class="dg-row"><label>cells（w*h 個。左上から行ごと、カンマ区切り） <input type="text" class="dg-w" id="dg-dcells"></label><span class="dg-thumbs" id="dg-dth"></span></div></div></div>';
		h += '<div class="dg-sec"><h3>お試し作成</h3><div class="dg-row" id="dg-params"></div>' +
			'<div class="dg-row"><label><input type="checkbox" id="dg-usetheme" checked> 編集中のテーマを使う</label>' +
			'<button type="button" id="dg-gen">作る</button><button type="button" id="dg-gen2" class="sub">別のシード</button>' +
			'<label>表示 <select id="dg-view"><option value="parts">パーツの絵</option><option value="meaning">意味の色</option></select></label>' +
			'<label>拡大 <select id="dg-zoom"><option value="0.5">1/2倍</option><option value="1" selected>1倍</option><option value="2">2倍</option><option value="4">4倍</option></select></label></div>' +
			'<div class="sub-text" id="dg-info"></div><div class="dg-legend" id="dg-legend" hidden></div>' +
			'<div class="dg-scroll"><canvas class="dg-cv" id="dg-out"></canvas></div></div>';
		$root.innerHTML = h;
		var ps = '';
		PARAMS.forEach(function (p) {
			ps += '<label>' + p[1] + ' <input type="text" class="dg-n" inputmode="numeric" id="dg-p-' + p[0] + '" value="' + p[2] + '"></label>';
		});
		$('dg-params').innerHTML = ps;
		var ls = '';
		LISTS.forEach(function (l) {
			ls += '<span>' + l[1] + '</span><input type="text" id="dg-l-' + l[0] + '" inputmode="numeric" autocomplete="off"><span class="dg-thumbs" id="dg-t-' + l[0] + '"></span>';
		});
		$('dg-lists').innerHTML = ls;
	}

	/* ---------------------------------------------------------------- */
	/* 絵																		 */
	/* ---------------------------------------------------------------- */
	function listAll(kind) {
		var out = [];
		function next(offset) {
			return api('/api/admin/data/list', { kind: kind, offset: offset, limit: 500 }).then(function (r) {
				if (!r.records) throw new Error(errText(r));
				out = out.concat(r.records);
				return r.more ? next(offset + r.records.length) : out;
			});
		}
		return next(0);
	}

	function ensureBase() {
		if (bReady) return Promise.resolve();
		if (loading) return loading;
		if (!root.SboMapDoc || !root.SboMapDoc.loadSheets) {
			return Promise.reject(new Error('admin-mapdoc.js が読めませんでした'));
		}
		loading = root.SboMapDoc.loadSheets('/admin-grp/').then(function (s) {
			sheets = s;
			return listAll('mapParts');
		}).then(function (a) {
			parts = [];
			a.forEach(function (p) { parts[p.id] = p; });
			bReady = true;
		});
		loading.then(null, function () { loading = null; });
		return loading;
	}

	function pimg(no) {
		if (!bReady || !parts[no]) return null;
		return root.SboMapDoc.partCanvas(parts, sheets, pcache, no);
	}

	/* マス目（grid[y][x] のパーツ番号の配列 or 平らな配列）を canvas に描く */
	function drawGrid(cv, w, h, get, tile) {
		cv.width = w * tile;
		cv.height = h * tile;
		var ctx = cv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.fillStyle = '#000';
		ctx.fillRect(0, 0, cv.width, cv.height);
		for (var y = 0; y < h; y++) {
			for (var x = 0; x < w; x++) {
				var v = get(x, y);
				var im = v == null ? null : pimg(v);
				if (im) ctx.drawImage(im, 0, 0, T, T, x * tile, y * tile, tile, tile);
			}
		}
		return ctx;
	}

	function thumbs(box, list) {
		box.textContent = '';
		list.slice(0, 24).forEach(function (no) {
			var im = pimg(no);
			if (!im) return;
			var c = document.createElement('canvas');
			c.width = c.height = T;
			c.title = String(no);
			c.getContext('2d').drawImage(im, 0, 0);
			box.appendChild(c);
		});
	}

	/* ---------------------------------------------------------------- */
	/* 9分割																	 */
	/* ---------------------------------------------------------------- */
	/* 部屋 r を W×H に当てる。四隅が入らない大きさなら null。返す値は [y][x] のパーツ番号 */
	function nineSlice(r, W, H) {
		var a = r.cutX[0], b = r.cutX[1], c = r.cutY[0], d = r.cutY[1];
		var rw = r.w - b, bh = r.h - d;
		if (W < a + rw || H < c + bh) return null;
		var mw = b - a, mh = d - c;
		function sx(x) {
			if (x < a) return x;
			if (x >= W - rw) return b + (x - (W - rw));
			return a + ((x - a) % mw);
		}
		function sy(y) {
			if (y < c) return y;
			if (y >= H - bh) return d + (y - (H - bh));
			return c + ((y - c) % mh);
		}
		var g = [];
		for (var y = 0; y < H; y++) {
			g[y] = [];
			for (var x = 0; x < W; x++) g[y][x] = r.cells[sy(y) * r.w + sx(x)];
		}
		return g;
	}

	function drawRoom() {
		var r = cur && cur.rooms[selRoom];
		if (!r) return;
		var tile = r.w * 24 > 480 ? Math.max(8, Math.floor(480 / r.w)) : 24;
		var ctx = drawGrid($('dg-rcv'), r.w, r.h, function (x, y) { return r.cells[y * r.w + x]; }, tile);
		ctx.strokeStyle = '#3aa0ff';
		ctx.lineWidth = 2;
		ctx.beginPath();
		r.cutX.forEach(function (x) { ctx.moveTo(x * tile, 0); ctx.lineTo(x * tile, r.h * tile); });
		r.cutY.forEach(function (y) { ctx.moveTo(0, y * tile); ctx.lineTo(r.w * tile, y * tile); });
		ctx.stroke();
		var minW = r.cutX[0] + (r.w - r.cutX[1]), minH = r.cutY[0] + (r.h - r.cutY[1]);
		sample('dg-s1', r, r.w + 5, r.h + 4, Math.min(tile, 16));
		sample('dg-s2', r, Math.max(minW, r.w - 3), Math.max(minH, r.h - 2), Math.min(tile, 16));
	}

	function sample(id, r, W, H, tile) {
		var g = nineSlice(r, W, H);
		var cap = $(id + 'c');
		cap.textContent = '見本 ' + W + '×' + H + '（幅' + (W - r.w >= 0 ? '+' : '') + (W - r.w) + '・高さ' + (H - r.h >= 0 ? '+' : '') + (H - r.h) + '）';
		if (!g) {
			$(id).width = $(id).height = 1;
			return;
		}
		drawGrid($(id), W, H, function (x, y) { return g[y][x]; }, tile);
	}

	var dragCut = null;			/* {axis:'x'|'y', i:0|1} */
	function bindRoomCanvas() {
		var cv = $('dg-rcv');
		function pos(ev) {
			var r = cv.getBoundingClientRect();
			var k = cv.width / r.width;
			return [(ev.clientX - r.left) * k, (ev.clientY - r.top) * k];
		}
		function tileSize() {
			var r = cur.rooms[selRoom];
			return cv.width / r.w;
		}
		cv.addEventListener('pointerdown', function (ev) {
			var r = cur && cur.rooms[selRoom];
			if (!r) return;
			var p = pos(ev), t = tileSize(), best = null, bd = 10 * (cv.width / cv.getBoundingClientRect().width);
			[0, 1].forEach(function (i) {
				var dx = Math.abs(p[0] - r.cutX[i] * t), dy = Math.abs(p[1] - r.cutY[i] * t);
				if (dx < bd) { bd = dx; best = { axis: 'x', i: i }; }
				if (dy < bd) { bd = dy; best = { axis: 'y', i: i }; }
			});
			dragCut = best;
			if (best) {
				cv.setPointerCapture(ev.pointerId);
				ev.preventDefault();
			}
		});
		cv.addEventListener('pointermove', function (ev) {
			var r = cur && cur.rooms[selRoom];
			if (!dragCut || !r) return;
			var p = pos(ev), t = tileSize();
			var key = dragCut.axis === 'x' ? 'cutX' : 'cutY', len = dragCut.axis === 'x' ? r.w : r.h;
			var v = Math.round(p[dragCut.axis === 'x' ? 0 : 1] / t), c = r[key];
			v = dragCut.i === 0 ? Math.max(1, Math.min(c[1] - 1, v)) : Math.max(c[0] + 1, Math.min(len, v));
			if (v !== c[dragCut.i]) {
				c[dragCut.i] = v;
				markDirty();
				drawRoom();
			}
		});
		function end() { dragCut = null; }
		cv.addEventListener('pointerup', end);
		cv.addEventListener('pointercancel', end);
	}

	/* ---------------------------------------------------------------- */
	/* テーマの一覧と編集欄														 */
	/* ---------------------------------------------------------------- */
	function markDirty() {
		dirty = true;
		$('dg-dirty').textContent = '（未保存の変更あり）';
	}
	function clean() {
		dirty = false;
		$('dg-dirty').textContent = '';
	}

	function blankTheme() {
		return { name: '新しいテーマ', algo: 1, floor: [1], wallTop: [1], wallFace: [1], wallFill: [1], 'void': 0, arch: [1], door: 0, rooms: [], decos: [] };
	}

	function fillSelect() {
		var s = $('dg-sel'), h = '';
		themes.forEach(function (t) { h += '<option value="' + t.id + '">' + t.id + ': ' + esc(t.name) + '</option>'; });
		if (cur && !cur.id) h += '<option value="new" selected>（未保存）' + esc(cur.name) + '</option>';
		s.innerHTML = h;
		if (cur && cur.id) s.value = String(cur.id);
		$('dg-del').disabled = !cur;
		$('dg-dup').disabled = !cur;
		$('dg-save').disabled = !cur;
	}

	function show() {
		fillSelect();
		$('dg-edit').hidden = !cur;
		$('dg-rooms-sec').hidden = !cur;
		$('dg-decos-sec').hidden = !cur;
		if (!cur) return;
		$('dg-name').value = cur.name;
		$('dg-algo').value = String(cur.algo);
		LISTS.forEach(function (l) {
			var v = cur[l[0]];
			$('dg-l-' + l[0]).value = SCALAR[l[0]] ? String(v) : v.join(',');
		});
		showThumbs();
		selRoom = cur.rooms.length ? Math.min(Math.max(selRoom, 0), cur.rooms.length - 1) : -1;
		selDeco = cur.decos.length ? Math.min(Math.max(selDeco, 0), cur.decos.length - 1) : -1;
		showRooms();
		showDecos();
	}

	function showThumbs() {
		LISTS.forEach(function (l) {
			var v = cur[l[0]];
			thumbs($('dg-t-' + l[0]), SCALAR[l[0]] ? (v ? [v] : []) : v);
		});
	}

	function parseList(s) {
		return String(s).split(/[,、，\s]+/).filter(function (x) { return x !== ''; }).map(Number).filter(function (n) { return isFinite(n) && n >= 0; }).map(Math.floor);
	}

	function pickTheme(t) {
		cur = t ? JSON.parse(JSON.stringify(t)) : null;
		selRoom = selDeco = -1;
		clean();
		show();
	}

	function load() {
		setMsg('読んでいます…', true);
		return ensureBase().then(function () {
			return api('/api/admin/dungeon/themes', {});
		}).then(function (r) {
			if (!r.themes) throw new Error(errText(r));
			themes = r.themes;
			setMsg('');
			if (!cur || (cur.id && !themes.some(function (t) { return t.id === cur.id; }))) {
				pickTheme(themes[0] || null);
			} else {
				show();
			}
		}).then(null, function (e) {
			setMsg('読めませんでした: ' + (e && e.message || e));
		});
	}

	function saveTheme() {
		if (!cur) return;
		setMsg('保存しています…', true);
		api('/api/admin/dungeon/theme/save', { theme: cur }).then(function (r) {
			if (!r.ok) {
				setMsg('保存できません: ' + errText(r));
				return;
			}
			var i = -1;
			themes.forEach(function (t, k) { if (t.id === r.theme.id) i = k; });
			if (i >= 0) themes[i] = r.theme; else themes.push(r.theme);
			pickTheme(r.theme);
			setMsg('保存しました（テーマ ' + r.theme.id + '）。', true);
		}, function (e) { setMsg('保存できません: ' + (e && e.message || e)); });
	}

	function delTheme() {
		if (!cur) return;
		if (!cur.id) {
			pickTheme(themes[0] || null);
			return;
		}
		if (!confirm('テーマ「' + cur.name + '」を消しますか。')) return;
		api('/api/admin/dungeon/theme/delete', { id: cur.id }).then(function (r) {
			if (!r.ok) {
				setMsg('消せません: ' + errText(r));
				return;
			}
			themes = themes.filter(function (t) { return t.id !== cur.id; });
			pickTheme(themes[0] || null);
			setMsg('消しました。', true);
		}, function (e) { setMsg('消せません: ' + (e && e.message || e)); });
	}

	/* ---------------------------------------------------------------- */
	/* 書き出し・読み込み														 */
	/* ---------------------------------------------------------------- */
	var impThemes = null;
	function download(name, obj) {
		var a = document.createElement('a');
		a.href = URL.createObjectURL(new Blob([JSON.stringify(obj, null, '\t')], { type: 'application/json' }));
		a.download = name;
		document.body.appendChild(a);
		a.click();
		setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
	}
	function readImportFile(f) {
		var fr = new FileReader();
		fr.onload = function () {
			try {
				var o = JSON.parse(fr.result);
				var a = Array.isArray(o) ? o : (o && Array.isArray(o.themes) ? o.themes : (o && o.floor ? [o] : null));
				if (!a || !a.length) throw new Error('テーマが見つかりません');
				a.forEach(function (t, i) {
					if (!t || typeof t !== 'object' || !Array.isArray(t.floor) || !Array.isArray(t.wallTop) || !Array.isArray(t.rooms || [])) {
						throw new Error((i + 1) + ' 件目がテーマの形ではありません');
					}
				});
				impThemes = a;
				$('dg-implist').textContent = a.length + ' 件: ' + a.map(function (t) {
					return (t.id ? t.id + ':' : '') + (t.name || '無題') + '（部屋' + (t.rooms || []).length + '・飾り' + (t.decos || []).length + '）';
				}).join(' / ');
				$('dg-imp').hidden = false;
				setMsg('');
			} catch (e) {
				impThemes = null;
				$('dg-imp').hidden = true;
				setMsg('読み込めません: ' + (e && e.message || e));
			}
		};
		fr.readAsText(f);
	}
	function doImport() {
		if (!impThemes) return;
		var mode = document.querySelector('input[name=dg-impmode]:checked').value;
		if (mode === 'replace' && !confirm('同じ id のテーマを上書きします。よいですか。')) return;
		api('/api/admin/dungeon/themes/import', { themes: impThemes, mode: mode }).then(function (r) {
			if (!r.ok) {
				setMsg('入れられません（何も変えていません）: ' + errText(r));
				return;
			}
			themes = r.themes;
			impThemes = null;
			$('dg-imp').hidden = true;
			$('dg-file').value = '';
			pickTheme(themes[0] || null);
			setMsg('入れました（追加 ' + r.added + '・上書き ' + r.replaced + '）。', true);
		}, function (e) { setMsg('入れられません: ' + (e && e.message || e)); });
	}

	/* ---------------------------------------------------------------- */
	/* 部屋																	 */
	/* ---------------------------------------------------------------- */
	function showRooms() {
		var s = $('dg-rsel'), h = '';
		cur.rooms.forEach(function (r, i) { h += '<option value="' + i + '">' + (i + 1) + ': ' + esc(r.name) + '（' + KIND[r.kind] + ' ' + r.w + '×' + r.h + '）</option>'; });
		s.innerHTML = h || '<option value="-1">（部屋はまだありません）</option>';
		if (selRoom >= 0) s.value = String(selRoom);
		$('dg-rdel').disabled = selRoom < 0;
		$('dg-redit').hidden = selRoom < 0;
		if (selRoom < 0) return;
		var r = cur.rooms[selRoom];
		$('dg-rname').value = r.name;
		$('dg-rkind').value = String(r.kind);
		$('dg-rinfo').textContent = r.src ? 'マップ ' + r.src.map + ' の (' + r.src.x + ',' + r.src.y + ') から' : '';
		drawRoom();
	}

	function importRoom() {
		var map = num($('dg-rm').value, 0), x = num($('dg-rx').value, 0), y = num($('dg-ry').value, 0);
		var w = num($('dg-rw').value, 0), h = num($('dg-rh').value, 0);
		if (w < 2 || h < 2 || w > 64 || h > 64 || x < 0 || y < 0) {
			setMsg('範囲は w・h が 2〜64、x・y が 0 以上にしてください。');
			return;
		}
		if (cur.rooms.length >= 50) {
			setMsg('部屋は 50 件までです。');
			return;
		}
		setMsg('マップを読んでいます…', true);
		api('/api/admin/data/tiles', { id: map, x: x, y: y, w: w, h: h }).then(function (r) {
			if (!r.parts) throw new Error(errText(r));
			var rows = r.parts, rh = rows.length, rw = rows[0] ? rows[0].length : 0;
			if (!rw) throw new Error('範囲がマップの外です');
			var cells = [];
			rows.forEach(function (row) { row.forEach(function (v) { cells.push(v); }); });
			cur.rooms.push({
				name: '部屋' + (cur.rooms.length + 1), kind: 0, w: rw, h: rh, cells: cells,
				cutX: rw >= 3 ? [1, rw - 1] : [1, 2], cutY: rh >= 3 ? [1, rh - 1] : [1, 2], src: { map: map, x: x, y: y }
			});
			selRoom = cur.rooms.length - 1;
			markDirty();
			showRooms();
			setMsg(rw === w && rh === h ? '取り込みました。' : 'マップの端までの ' + rw + '×' + rh + ' を取り込みました。', true);
		}).then(null, function (e) { setMsg('取り込めません: ' + (e && e.message || e)); });
	}

	/* ---------------------------------------------------------------- */
	/* 飾り																	 */
	/* ---------------------------------------------------------------- */
	function showDecos() {
		var s = $('dg-dsel'), h = '';
		cur.decos.forEach(function (d, i) { h += '<option value="' + i + '">' + (i + 1) + ': ' + esc(d.name) + '</option>'; });
		s.innerHTML = h || '<option value="-1">（飾りはまだありません）</option>';
		if (selDeco >= 0) s.value = String(selDeco);
		$('dg-ddel').disabled = selDeco < 0;
		$('dg-dedit').hidden = selDeco < 0;
		if (selDeco < 0) return;
		var d = cur.decos[selDeco];
		$('dg-dname').value = d.name;
		$('dg-dw').value = d.w;
		$('dg-dh').value = d.h;
		$('dg-don').value = String(d.on);
		$('dg-drate').value = d.rate;
		$('dg-dmax').value = d.max;
		$('dg-dcells').value = d.cells.join(',');
		thumbs($('dg-dth'), d.cells);
	}

	/* w・h が変わったら cells の長さを合わせる（足りない所は 0） */
	function fitDecoCells(d) {
		var n = d.w * d.h;
		while (d.cells.length < n) d.cells.push(0);
		d.cells.length = n;
	}

	/* ---------------------------------------------------------------- */
	/* お試し作成																 */
	/* ---------------------------------------------------------------- */
	function readParams(bNewSeed) {
		var p = {};
		PARAMS.forEach(function (q) {
			var v = $('dg-p-' + q[0]).value.trim();
			if (v !== '') p[q[0]] = num(v, q[2]);
		});
		if (bNewSeed) p.seed = 0;
		return p;
	}

	function generate(bNewSeed) {
		var body = { params: readParams(bNewSeed) };
		if ($('dg-usetheme').checked && cur) body.theme = cur;
		setMsg('作っています…', true);
		$('dg-gen').disabled = $('dg-gen2').disabled = true;
		ensureBase().then(function () {
			return api('/api/admin/dungeon/preview', body);
		}).then(function (r) {
			if (!r.ok) {
				setMsg('作れませんでした: ' + errText(r));
				return;
			}
			last = r;
			$('dg-p-seed').value = r.seed;		/* 同じ形をもう一度出せるよう、使ったシードを入れる */
			setMsg('');
			$('dg-info').textContent = 'シード ' + r.seed + '・作り直し ' + r.tries + ' 回・床 ' + r.floorCount + ' マス・部屋 ' + (r.rooms || []).length + ' 個・' + r.w + '×' + r.h;
			drawOut();
		}).then(null, function (e) {
			setMsg('作れませんでした: ' + (e && e.message || e));
		}).then(function () {
			$('dg-gen').disabled = $('dg-gen2').disabled = false;
		});
	}

	function drawOut() {
		var r = last, cv = $('dg-out'), lg = $('dg-legend');
		if (!r) return;
		var view = $('dg-view').value, tile = Math.max(2, Math.round(T * Number($('dg-zoom').value)));
		var ctx;
		lg.hidden = view !== 'meaning';
		if (view === 'parts') {
			if (!r.parts || !r.parts.length) {
				setMsg('パーツの絵がありません（テーマを使っていないか、テーマが空です）。「意味の色」で見てください。');
				view = 'meaning';
			}
		}
		if (view === 'parts') {
			ctx = drawGrid(cv, r.w, r.h, function (x, y) { return r.parts[y * r.w + x]; }, tile);
			return;
		}
		cv.width = r.w * tile;
		cv.height = r.h * tile;
		ctx = cv.getContext('2d');
		var x, y;
		for (y = 0; y < r.h; y++) {
			for (x = 0; x < r.w; x++) {
				ctx.fillStyle = CELL_COLOR[r.cells[y * r.w + x]] || '#f0f';
				ctx.fillRect(x * tile, y * tile, tile, tile);
			}
		}
		ctx.lineWidth = Math.max(1, tile / 8);
		ctx.strokeStyle = '#3aa0ff';
		(r.rooms || []).forEach(function (m) { ctx.strokeRect(m.x * tile + 0.5, m.y * tile + 0.5, m.w * tile - 1, m.h * tile - 1); });
		ctx.strokeStyle = '#ff4d8d';
		(r.links || []).forEach(function (l) { ctx.strokeRect(l.x * tile + 0.5, l.y * tile + 0.5, l.w * tile - 1, l.h * tile - 1); });
		function mark(p, ch, col) {
			if (!p) return;
			ctx.fillStyle = col;
			ctx.fillRect(p.x * tile, p.y * tile, tile, tile);
			ctx.fillStyle = '#000';
			ctx.font = 'bold ' + Math.max(8, tile) + 'px sans-serif';
			ctx.textAlign = 'center';
			ctx.textBaseline = 'middle';
			ctx.fillText(ch, (p.x + 0.5) * tile, (p.y + 0.55) * tile);
		}
		mark(r.start, 'S', '#4cd964');
		mark(r.goal, 'G', '#ff5a4d');
		var lh = '';
		CELL_NAME.forEach(function (n, i) { lh += '<span><i style="background:' + CELL_COLOR[i] + '"></i>' + i + ' ' + n + '</span>'; });
		lh += '<span><i style="border-color:#3aa0ff"></i>部屋の枠</span><span><i style="border-color:#ff4d8d"></i>出入口</span>' +
			'<span><i style="background:#4cd964"></i>S 入口</span><span><i style="background:#ff5a4d"></i>G ボス</span>';
		lg.innerHTML = lh;
	}

	/* ---------------------------------------------------------------- */
	/* 割り当て																	 */
	/* ---------------------------------------------------------------- */
	function bind() {
		if (bBound) return;
		bBound = true;
		bindRoomCanvas();
		$('dg-sel').addEventListener('change', function () {
			if (dirty && !confirm('保存していない変更があります。捨てて切り替えますか。')) { fillSelect(); return; }
			var id = num($('dg-sel').value, 0);
			pickTheme(themes.filter(function (t) { return t.id === id; })[0] || null);
		});
		$('dg-new').addEventListener('click', function () {
			if (dirty && !confirm('保存していない変更があります。捨てて新しく作りますか。')) return;
			pickTheme(blankTheme());
			markDirty();
		});
		$('dg-dup').addEventListener('click', function () {
			if (!cur) return;
			var c = JSON.parse(JSON.stringify(cur));
			delete c.id;
			c.name = c.name + '（複製）';
			cur = c;
			selRoom = selDeco = -1;
			show();
			markDirty();
		});
		$('dg-del').addEventListener('click', delTheme);
		$('dg-exp-all').addEventListener('click', function () { download('dungeon-themes.json', { themes: themes }); });
		$('dg-exp-one').addEventListener('click', function () {
			var id = num($('dg-sel').value, 0), t = themes.filter(function (x) { return x.id === id; })[0];
			if (!t) { setMsg('保存済みのテーマを選んでください。'); return; }
			download('dungeon-theme-' + t.id + '.json', { themes: [t] });
		});
		$('dg-file').addEventListener('change', function () { if ($('dg-file').files[0]) readImportFile($('dg-file').files[0]); });
		$('dg-impgo').addEventListener('click', doImport);
		$('dg-impcancel').addEventListener('click', function () { impThemes = null; $('dg-imp').hidden = true; $('dg-file').value = ''; });
		$('dg-save').addEventListener('click', saveTheme);
		$('dg-name').addEventListener('input', function () { cur.name = $('dg-name').value; markDirty(); });
		$('dg-name').addEventListener('change', fillSelect);
		$('dg-algo').addEventListener('change', function () { cur.algo = num($('dg-algo').value, 1); markDirty(); });
		LISTS.forEach(function (l) {
			$('dg-l-' + l[0]).addEventListener('input', function () {
				var a = parseList($('dg-l-' + l[0]).value);
				cur[l[0]] = SCALAR[l[0]] ? (a[0] || 0) : a;
				markDirty();
				showThumbs();
			});
		});
		$('dg-rsel').addEventListener('change', function () { selRoom = num($('dg-rsel').value, -1); showRooms(); });
		$('dg-rdel').addEventListener('click', function () {
			if (selRoom < 0 || !confirm('この部屋を削除しますか。')) return;
			cur.rooms.splice(selRoom, 1);
			selRoom = Math.min(selRoom, cur.rooms.length - 1);
			markDirty();
			showRooms();
		});
		$('dg-rimp').addEventListener('click', importRoom);
		$('dg-rname').addEventListener('input', function () { cur.rooms[selRoom].name = $('dg-rname').value; markDirty(); });
		$('dg-rname').addEventListener('change', showRooms);
		$('dg-rkind').addEventListener('change', function () { cur.rooms[selRoom].kind = num($('dg-rkind').value, 0); markDirty(); showRooms(); });
		$('dg-dsel').addEventListener('change', function () { selDeco = num($('dg-dsel').value, -1); showDecos(); });
		$('dg-dadd').addEventListener('click', function () {
			if (cur.decos.length >= 50) { setMsg('飾りは 50 件までです。'); return; }
			cur.decos.push({ name: '飾り' + (cur.decos.length + 1), w: 1, h: 1, cells: [0], on: 1, rate: 10, max: 8 });
			selDeco = cur.decos.length - 1;
			markDirty();
			showDecos();
		});
		$('dg-ddel').addEventListener('click', function () {
			if (selDeco < 0) return;
			cur.decos.splice(selDeco, 1);
			selDeco = Math.min(selDeco, cur.decos.length - 1);
			markDirty();
			showDecos();
		});
		function decoField(id, fn, bRedraw) {
			$(id).addEventListener('change', function () {
				var d = cur.decos[selDeco];
				if (!d) return;
				fn(d, $(id).value);
				markDirty();
				showDecos();
			});
		}
		decoField('dg-dname', function (d, v) { d.name = v; });
		decoField('dg-dw', function (d, v) { d.w = Math.max(1, Math.min(16, num(v, 1))); fitDecoCells(d); });
		decoField('dg-dh', function (d, v) { d.h = Math.max(1, Math.min(16, num(v, 1))); fitDecoCells(d); });
		decoField('dg-don', function (d, v) { d.on = num(v, 1); });
		decoField('dg-drate', function (d, v) { d.rate = Math.max(0, Math.min(100, num(v, 0))); });
		decoField('dg-dmax', function (d, v) { d.max = Math.max(0, Math.min(999, num(v, 0))); });
		decoField('dg-dcells', function (d, v) {
			d.cells = parseList(v);
			fitDecoCells(d);
		});
		$('dg-gen').addEventListener('click', function () { generate(false); });
		$('dg-gen2').addEventListener('click', function () { generate(true); });
		$('dg-view').addEventListener('change', drawOut);
		$('dg-zoom').addEventListener('change', drawOut);
	}

	function open(fn) {
		api = fn;
		$root = $('dg-root');
		if (!$root) return;
		if (!bBound) {
			build();
			bind();
		}
		load();
	}

	root.SboDungeon = { open: open };
})(window);
