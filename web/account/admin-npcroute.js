/* ========================================================================= */
/* admin-npcroute.js														 */
/* 管理ページ（/admin）の「NPCの巡回」											 */
/* ========================================================================= */
/*
 * NPC（動かない・ランダム移動）が、道しるべを順に歩く「巡回」を決める。
 *   1. マップを選ぶと、屋根を外した全体図を描き、そのマップの NPC を一覧にする。
 *   2. NPC を 1 体選ぶと、今のルートを絵の上に出す。絵のマスをタップすると道しるべが増える。
 *   3. 「保存」で /api/admin/npcroute/set に送る（サーバーは server/node/sbosvr_data.js）。保存するとすぐ動き出す。
 * 警告（黄色い注意）は保存を止めない。ゲーム側の歩き方は貪欲な 1 歩ずつ（経路探索なし）なので、
 * 道しるべの間は障害物の無い直線（同じ行か同じ列）が前提。
 * マスの絵は admin-mapdoc.js の loadSheets（/admin-grp/ の map01〜05.png）で読む。
 * 通れるかの判定は、サーバーの CharProcMOVERAND.cpp の IsMovePos に合わせている。
 */
(function (root) {
	'use strict';

	var T = 16;						/* 画像のマスの大きさ */
	var SC = 2;						/* 拡大率 */
	var S = T * SC;
	var MAX_POINTS = 16;
	var HIDE_MAIN = { 3: true, 4: true, 8: true, 9: true, 18: true };	/* 屋根として外す当たり判定タイプ（admin-design.js と同じ） */
	/* 歩ける当たり判定タイプ：PARTSHIT_NONE(0)・HIDE1(3)・MOVEFRONT/UP/DOWN/RIGHT/LEFT(12〜16)。IsMovePos と同じ */
	var WALK_HIT = { 0: true, 3: true, 12: true, 13: true, 14: true, 15: true, 16: true };
	var NPC_TYPE = { 1: '動かない', 2: 'ランダム移動' };

	var api = null;					/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var sheets = null;
	var parts = [];					/* mapParts の一覧（id 順） */
	var maps = [];					/* [{id, szName}] */
	var bReady = false;				/* 部品の読み込みが済んだか */
	var bBound = false;				/* ボタンの割り当てが済んだか */
	var loadingBase = null;
	var cur = null;					/* 開いているマップ {id, mapId, W, H, tiles, events, npcs} */
	var sel = null;					/* 選んでいる NPC（cur.npcs の 1 つ） */
	var route = [];					/* 編集中の道しるべ [[x,y],...] */
	var seq = 0;					/* 古い読み込みの結果を捨てるための番号 */
	var cvBase = null, cvBlock = null, cvOver = null;

	function $(id) {
		return document.getElementById(id);
	}
	function el(tag, cls, text) {
		var e = document.createElement(tag);
		if (cls) e.className = cls;
		if (text != null) e.textContent = text;
		return e;
	}
	function setMsg(text, bOk) {
		var e = $('nr-msg');
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}
	function errText(r) {
		return (r && (r.message || r.error)) || ('HTTP ' + (r && r.httpStatus));
	}
	function pad3(n) {
		return ('00' + n).slice(-3);
	}

	/* ---------------------------------------------------------------- */
	/* データの読み込み													 */
	/* ---------------------------------------------------------------- */
	function listAll(kind, opt) {
		var out = [];
		function next(offset) {
			var body = { kind: kind, offset: offset, limit: 500 };
			Object.keys(opt || {}).forEach(function (k) { body[k] = opt[k]; });
			return api('/api/admin/data/list', body).then(function (r) {
				if (!r.records) throw new Error(errText(r));
				out = out.concat(r.records);
				return r.more ? next(offset + r.records.length) : out;
			});
		}
		return next(0);
	}

	/* 絵・パーツ・マップ名（最初の 1 回だけ） */
	function ensureBase() {
		if (bReady) return Promise.resolve();
		if (loadingBase) return loadingBase;
		if (!root.SboMapDoc || !root.SboMapDoc.loadSheets) {
			return Promise.reject(new Error('admin-mapdoc.js が読めませんでした'));
		}
		loadingBase = root.SboMapDoc.loadSheets('/admin-grp/').then(function (s) {
			sheets = s;
			return listAll('mapParts');
		}).then(function (a) {
			parts = [];
			a.forEach(function (p) { parts[p.id] = p; });
			return listAll('map', { fields: ['szName'] });
		}).then(function (a) {
			maps = a;
			bReady = true;
		});
		loadingBase.then(null, function () { loadingBase = null; });
		return loadingBase;
	}

	/* マップのマス目を 128×128 ずつ読む */
	function readTiles() {
		var tiles = [], info = null;
		function one(x, y) {
			return api('/api/admin/data/tiles', { id: cur.id, x: x, y: y, w: 128, h: 128 }).then(function (r) {
				if (!r.parts) throw new Error(errText(r));
				info = r;
				r.parts.forEach(function (row, dy) {
					var t = tiles[y + dy] = tiles[y + dy] || [];
					row.forEach(function (v, dx) { t[x + dx] = v; });
				});
				return r;
			});
		}
		return one(0, 0).then(function (r0) {
			var jobs = [], x, y;
			for (y = 0; y < r0.cy; y += 128) {
				for (x = 0; x < r0.cx; x += 128) {
					if (x || y) jobs.push([x, y]);
				}
			}
			return jobs.reduce(function (p, j) {
				return p.then(function () { return one(j[0], j[1]); });
			}, Promise.resolve()).then(function () {
				return { tiles: tiles, W: r0.cx, H: r0.cy, mapId: r0.mapId };
			});
		});
	}

	function loadMap(id) {
		var my = ++seq;
		cur = null;
		sel = null;
		route = [];
		renderList();
		renderEditor();
		$('nr-scroll').hidden = true;
		$('nr-count').textContent = '';
		setMsg('マップを読んでいます…', true);
		var work = { id: id };
		cur = work;
		return readTiles().then(function (t) {
			if (my !== seq) throw 'stale';
			work.tiles = t.tiles;
			work.W = t.W;
			work.H = t.H;
			work.mapId = t.mapId;
			return api('/api/admin/data/list', { kind: 'map', match: { id: id }, fields: ['events'], limit: 1 });
		}).then(function (r) {
			if (my !== seq) throw 'stale';
			if (!r.records) throw new Error(errText(r));
			work.events = (r.records[0] && r.records[0].events) || [];
			return listAll('char', { match: { dwMapID: work.mapId }, fields: ['szName', 'byNpcType', 'dwMapID', 'dwX', 'dwY', 'byDirection', 'route', 'szNpcSpeech'] });
		}).then(function (a) {
			if (my !== seq) throw 'stale';
			work.npcs = a.filter(function (c) { return c.byNpcType === 1 || c.byNpcType === 2; });
			drawBase();
			$('nr-scroll').hidden = false;
			renderList();
			redraw();
			setMsg('');
		}).then(null, function (err) {
			if (err === 'stale') return;
			cur = null;
			setMsg('読めませんでした: ' + (err && err.message || err));
		});
	}

	/* ---------------------------------------------------------------- */
	/* 通れるかの判定（CharProcMOVERAND.cpp の IsMovePos と同じ）			 */
	/* ---------------------------------------------------------------- */
	function isEventCell(x, y) {
		return cur.events.some(function (e) { return e.x === x && e.y === y; });
	}
	/* 通れない理由。通れれば ''。 */
	function blockReason(x, y) {
		if (x < 0 || y < 0 || x >= cur.W || y >= cur.H) return 'マップの外';
		var no = (cur.tiles[y] || [])[x] || 0;
		var p = parts[no] || parts[0] || { byHitType: 0 };
		if (!WALK_HIT[p.byHitType]) {
			/* 6・7 は足跡の出る雪・砂。見た目は平地でも、NPC は歩けない */
			return (p.byHitType === 6 || p.byHitType === 7) ? '雪や砂の床（NPC は歩けません）' : '壁などで通れない';
		}
		if (isEventCell(x, y)) return '扉・ワープなどのイベント';
		return '';
	}

	/* ---------------------------------------------------------------- */
	/* 絵																 */
	/* ---------------------------------------------------------------- */
	var cacheTile = {};
	function tileImage(no) {
		if (cacheTile[no]) return cacheTile[no];
		var p = parts[no] || parts[0] || { byLevel: 0, wGrpID: 0, wBackID: 0, byHitType: 0 };
		var lv = p.byLevel, g = p.wGrpID, b = p.wBackID;
		if (p.anime && p.anime.length) {
			lv = p.anime[0].byLevel;
			g = p.anime[0].wGrpID;
			b = p.anime[0].wBackID;
		}
		var cv = document.createElement('canvas');
		cv.width = cv.height = T;
		var ctx = cv.getContext('2d');
		ctx.fillStyle = '#000';
		ctx.fillRect(0, 0, T, T);
		function put(n, alpha) {
			var s = Math.floor(n / 256), k = n % 256;
			if (s >= sheets.map.length) return;
			ctx.globalAlpha = alpha;
			ctx.drawImage(sheets.map[s], (k % 16) * T, Math.floor(k / 16) * T, T, T, 0, 0, T, T);
			ctx.globalAlpha = 1;
		}
		put(b ? b : g, 1);
		if (!HIDE_MAIN[p.byHitType]) {		/* 屋根は描かない（ゲームで屋根を外した時の見た目） */
			put(g, lv ? lv / 100 : 1);
		}
		cacheTile[no] = cv;
		return cv;
	}

	/* 全体図・通れないマスの赤・上に重ねる絵の 3 枚を作り直す */
	function drawBase() {
		var stack = $('nr-stack');
		stack.textContent = '';
		var w = cur.W * S, h = cur.H * S;
		stack.style.width = w + 'px';
		stack.style.height = h + 'px';
		function make(cls) {
			var cv = document.createElement('canvas');
			cv.width = w;
			cv.height = h;
			if (cls) cv.className = cls;
			stack.appendChild(cv);
			return cv;
		}
		cvBase = make('');
		cvBlock = make('nr-over');
		cvOver = make('nr-over');
		cvBlock.style.pointerEvents = 'none';
		var ctx = cvBase.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		var x, y;
		for (y = 0; y < cur.H; y++) {
			var row = cur.tiles[y] || [];
			for (x = 0; x < cur.W; x++) {
				ctx.drawImage(tileImage(row[x] || 0), x * S, y * S, S, S);
			}
		}
		/* イベントのマス（扉・ワープなど）は水色の枠 */
		ctx.strokeStyle = 'rgba(0,230,255,0.9)';
		ctx.lineWidth = 2;
		cur.events.forEach(function (e) {
			ctx.strokeRect(e.x * S + 1, e.y * S + 1, S - 2, S - 2);
		});
		/* 通れないマスの赤（チェックを入れた時だけ見せる） */
		var bc = cvBlock.getContext('2d');
		bc.fillStyle = 'rgba(255,60,60,0.38)';
		for (y = 0; y < cur.H; y++) {
			for (x = 0; x < cur.W; x++) {
				if (blockReason(x, y)) bc.fillRect(x * S, y * S, S, S);
			}
		}
		cvBlock.style.display = $('nr-block').checked ? '' : 'none';
		cvOver.addEventListener('click', onCanvasClick);
	}

	function badge(ctx, cx, cy, text, fill, r) {
		ctx.beginPath();
		ctx.arc(cx, cy, r, 0, Math.PI * 2);
		ctx.fillStyle = fill;
		ctx.fill();
		ctx.strokeStyle = '#fff';
		ctx.lineWidth = 2;
		ctx.stroke();
		ctx.fillStyle = '#fff';
		ctx.font = 'bold ' + Math.floor(r * 1.15) + 'px sans-serif';
		ctx.textAlign = 'center';
		ctx.textBaseline = 'middle';
		ctx.fillText(text, cx, cy + 1);
	}

	/* 道しるべ・線・NPC の印を描き直す */
	function redraw() {
		if (!cur || !cvOver) return;
		var ctx = cvOver.getContext('2d');
		ctx.clearRect(0, 0, cvOver.width, cvOver.height);
		var x, y;
		if ($('nr-lines').checked) {
			ctx.strokeStyle = 'rgba(255,255,255,0.25)';
			ctx.lineWidth = 1;
			ctx.beginPath();
			for (x = 0; x <= cur.W; x++) { ctx.moveTo(x * S + 0.5, 0); ctx.lineTo(x * S + 0.5, cur.H * S); }
			for (y = 0; y <= cur.H; y++) { ctx.moveTo(0, y * S + 0.5); ctx.lineTo(cur.W * S, y * S + 0.5); }
			ctx.stroke();
		}
		/* NPC の位置（四角の枠。選んでいるのはオレンジ、ほかは青） */
		cur.npcs.forEach(function (c) {
			var bSel = (c === sel);
			ctx.strokeStyle = bSel ? '#f0b84a' : 'rgba(90,150,255,0.95)';
			ctx.lineWidth = bSel ? 3 : 2;
			ctx.strokeRect(c.dwX * S + 2, c.dwY * S + 2, S - 4, S - 4);
			if (c.route && c.route.points && c.route.points.length >= 2 && !bSel) {
				ctx.fillStyle = 'rgba(90,150,255,0.95)';
				ctx.fillRect(c.dwX * S + S - 9, c.dwY * S + 2, 7, 7);
			}
		});
		if (!sel) return;
		var c0 = [sel.dwX * S + S / 2, sel.dwY * S + S / 2];
		function ctr(p) {
			return [p[0] * S + S / 2, p[1] * S + S / 2];
		}
		function line(a, b, color, w, dash) {
			ctx.strokeStyle = color;
			ctx.lineWidth = w;
			ctx.setLineDash(dash || []);
			ctx.beginPath();
			ctx.moveTo(a[0], a[1]);
			ctx.lineTo(b[0], b[1]);
			ctx.stroke();
			ctx.setLineDash([]);
		}
		var bLoop = $('nr-mode1').checked;
		if (route.length) {
			line(c0, ctr(route[0]), 'rgba(240,184,74,0.45)', 2, [5, 4]);		/* NPC の位置 → 1 点目は薄い線 */
		}
		for (var i = 1; i < route.length; i++) {
			line(ctr(route[i - 1]), ctr(route[i]), 'rgba(240,184,74,0.95)', 3);
		}
		if (bLoop && route.length >= 3) {
			line(ctr(route[route.length - 1]), ctr(route[0]), 'rgba(240,184,74,0.7)', 3, [8, 5]);
		}
		route.forEach(function (p, k) {
			var c = ctr(p);
			badge(ctx, c[0], c[1], String(k + 1), k === 0 ? '#2e9e5b' : '#c27a00', 10);
		});
	}

	/* ---------------------------------------------------------------- */
	/* 警告（保存は止めない）												 */
	/* ---------------------------------------------------------------- */
	function segmentWarn(a, b, label, aOut) {
		if (a[0] !== b[0] && a[1] !== b[1]) {
			aOut.push(label + ' が斜めです。同じ行か同じ列にまっすぐ置かないと、障害物にひっかかることがあります。');
			return;
		}
		var dx = Math.sign(b[0] - a[0]), dy = Math.sign(b[1] - a[1]);
		var x = a[0], y = a[1], bad = [];
		while (x !== b[0] || y !== b[1]) {
			x += dx;
			y += dy;
			if (blockReason(x, y)) bad.push('(' + x + ',' + y + ')');
		}
		if (bad.length) {
			aOut.push(label + ' の間に、通れないマスやイベントのマスがあります：' + bad.slice(0, 5).join(' ') + (bad.length > 5 ? ' ほか' : ''));
		}
	}

	function collectWarnings() {
		var out = [];
		if (!sel || route.length < 1) return out;
		var bLoop = $('nr-mode1').checked;
		segmentWarn([sel.dwX, sel.dwY], route[0], 'NPCの今の位置→道しるべ1', out);
		for (var i = 1; i < route.length; i++) {
			segmentWarn(route[i - 1], route[i], '道しるべ' + i + '→' + (i + 1), out);
		}
		if (bLoop && route.length >= 3) {
			segmentWarn(route[route.length - 1], route[0], '道しるべ' + route.length + '→1（一周の戻り）', out);
		}
		route.forEach(function (p, k) {
			var why = blockReason(p[0], p[1]);
			if (why) out.push('道しるべ' + (k + 1) + ' (' + p[0] + ',' + p[1] + ') は通れないマスです（' + why + '）。');
		});
		if (route.length === 1) out.push('道しるべは 2 つ以上置かないと保存できません。');
		return out;
	}

	/* ---------------------------------------------------------------- */
	/* 画面															 */
	/* ---------------------------------------------------------------- */
	/* 名前の無い NPC は、台詞の出だしを目印にする */
	function npcLabel(c) {
		var name = c.szName;
		if (!name) {
			var sp = String(c.szNpcSpeech || '').replace(/@\d+/g, '').replace(/\s+/g, '').slice(0, 14);
			name = sp ? '（' + sp + '…）' : '（名前なし）';
		}
		return '#' + c.id + ' ' + name;
	}

	function renderList() {
		var box = $('nr-list');
		box.textContent = '';
		if (!cur || !cur.npcs) return;
		$('nr-count').textContent = '動かない・ランダム移動の NPC：' + cur.npcs.length + ' 体';
		if (!cur.npcs.length) box.appendChild(el('div', 'sub-text', 'このマップには、設定できる NPC がいません。'));
		cur.npcs.forEach(function (c) {
			var b = el('button', 'nr-item' + (c === sel ? ' sel' : ''));
			b.type = 'button';
			b.appendChild(el('div', null, npcLabel(c)));
			var n = c.route && c.route.points ? c.route.points.length : 0;
			b.appendChild(el('span', 'sub-text', (NPC_TYPE[c.byNpcType] || '') + '・位置 (' + c.dwX + ',' + c.dwY + ')・' +
				(n >= 2 ? '巡回あり（' + n + ' 点・' + (c.route.mode ? '一周' : '折り返す') + '）' : '巡回なし')));
			b.addEventListener('click', function () { selectNpc(c); });
			box.appendChild(b);
		});
	}

	function renderEditor() {
		$('nr-edit').hidden = !sel;
		if (!sel) return;
		$('nr-title').textContent = npcLabel(sel) + '（' + (NPC_TYPE[sel.byNpcType] || '') + '）';
		var warns = collectWarnings();
		var ul = $('nr-warn');
		ul.textContent = '';
		warns.forEach(function (t) { ul.appendChild(el('li', null, t)); });
		ul.hidden = !warns.length;
		$('nr-points').textContent = route.length ? '道しるべ：' + route.map(function (p, k) {
			return (k + 1) + '(' + p[0] + ',' + p[1] + ')';
		}).join(' → ') + '（' + route.length + ' / ' + MAX_POINTS + '）' : '道しるべはまだありません。';
		$('nr-undo').disabled = !route.length;
		$('nr-clear').disabled = !route.length;
	}

	function selectNpc(c) {
		sel = c;
		var r = c.route || { mode: 0, wait: 3, points: [] };
		route = (r.points || []).map(function (p) { return [p[0], p[1]]; });
		$('nr-mode0').checked = !r.mode;
		$('nr-mode1').checked = !!r.mode;
		$('nr-wait').value = String(r.points && r.points.length ? r.wait : 3);
		setMsg('');
		renderList();
		renderEditor();
		redraw();
	}

	function changed() {
		renderEditor();
		redraw();
	}

	function onCanvasClick(ev) {
		if (!cur || !sel) {
			setMsg('先に、左の一覧から NPC を選んでください。');
			return;
		}
		var rect = cvOver.getBoundingClientRect();
		var x = Math.floor((ev.clientX - rect.left) / rect.width * cur.W);
		var y = Math.floor((ev.clientY - rect.top) / rect.height * cur.H);
		if (x < 0 || y < 0 || x >= cur.W || y >= cur.H) return;
		if (route.length >= MAX_POINTS) {
			setMsg('道しるべは ' + MAX_POINTS + ' 個までです。');
			return;
		}
		var last = route[route.length - 1];
		if (last && last[0] === x && last[1] === y) return;	/* 同じマスを続けて押した */
		setMsg('');
		route.push([x, y]);
		changed();
	}

	function save(bDelete) {
		if (!sel) return;
		var wait = Number($('nr-wait').value);
		if (!isFinite(wait) || Math.floor(wait) !== wait || wait < 0 || wait > 60) {
			setMsg('止まる秒数は 0 〜 60 の整数で入れてください。');
			return;
		}
		var pts = bDelete ? [] : route;
		if (pts.length === 1) {
			setMsg('道しるべは 2 つ以上置いてください。やめる時は「巡回をやめる」を押してください。');
			return;
		}
		if (bDelete && !window.confirm(npcLabel(sel) + ' の巡回をやめます。よろしいですか？')) return;
		var target = sel;
		var btns = ['nr-save', 'nr-del'];
		btns.forEach(function (id) { $(id).disabled = true; });
		setMsg('保存しています…', true);
		api('/api/admin/npcroute/set', {
			id: target.id,
			mode: $('nr-mode1').checked ? 1 : 0,
			wait: wait,
			points: pts
		}).then(function (r) {
			if (!r.ok || !r.char) {
				setMsg('保存できませんでした: ' + errText(r));
				return;
			}
			target.route = r.char.route || { mode: 0, wait: 0, points: [] };
			if (r.char.dwX != null) target.dwX = r.char.dwX;
			if (r.char.dwY != null) target.dwY = r.char.dwY;
			if (target === sel) {
				selectNpc(target);
			} else {
				renderList();
			}
			setMsg(pts.length ? '保存しました。' + npcLabel(target) + ' が巡回を始めます。' : '巡回をやめました。', true);
		}).then(null, function (err) {
			setMsg('保存できませんでした: ' + (err && err.message || err));
		}).then(function () {
			btns.forEach(function (id) { $(id).disabled = false; });
		});
	}

	function bind() {
		if (bBound) return;
		bBound = true;
		$('nr-map').addEventListener('change', function () {
			var v = $('nr-map').value;
			if (v !== '') loadMap(Number(v));
		});
		$('nr-reload').addEventListener('click', function () {
			var v = $('nr-map').value;
			if (v !== '') loadMap(Number(v));
		});
		$('nr-undo').addEventListener('click', function () { route.pop(); changed(); });
		$('nr-clear').addEventListener('click', function () { route = []; changed(); });
		$('nr-save').addEventListener('click', function () { save(false); });
		$('nr-del').addEventListener('click', function () { save(true); });
		$('nr-mode0').addEventListener('change', changed);
		$('nr-mode1').addEventListener('change', changed);
		$('nr-lines').addEventListener('change', redraw);
		$('nr-block').addEventListener('change', function () {
			if (cvBlock) cvBlock.style.display = $('nr-block').checked ? '' : 'none';
		});
	}

	/* fnApi(url, body) → Promise<data>（管理ページの api 関数。session は呼ぶ側で入れる） */
	function open(fnApi) {
		api = fnApi;
		bind();
		if (bReady) return;
		setMsg('読み込んでいます…', true);
		ensureBase().then(function () {
			var s = $('nr-map');
			s.textContent = '';
			var o0 = el('option', null, 'マップを選んでください');
			o0.value = '';
			s.appendChild(o0);
			maps.forEach(function (m) {
				var o = el('option', null, pad3(m.id) + ' ' + (m.szName || '（名前なし）'));
				o.value = String(m.id);
				s.appendChild(o);
			});
			setMsg('');
		}).then(null, function (err) {
			setMsg('読み込めませんでした: ' + (err && err.message || err));
		});
	}

	root.SboNpcRoute = { open: open };
})(this);
