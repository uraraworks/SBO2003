/* ========================================================================= */
/* admin-mapview.js															 */
/* 管理ページ（/admin）の「マップ確認」										 */
/* ========================================================================= */
/*
 * マップ全体の絵を出し、マウスを乗せたマスに枠を出して、そのマスの座標・パーツ・イベントを見せる。
 * データは読むだけ（何も書き換えない）。スマホではタップしたマスを出す。
 *   - 絵は admin-mapdoc.js の loadSheets（/admin-grp/ の map01〜05.png）で読む。
 *   - マス目は /api/admin/data/tiles、パーツ・マップ名・イベントは /api/admin/data/list（サーバーは server/node/sbosvr_data.js）。
 * 前の「NPCの巡回」（admin-npcroute.js）から絵の描き方・通れるかの判定を引き継いでいる。
 * 巡回の設定はゲーム内の管理者ウィンドウ（キャラ→NPCの巡回）に移ったので、Web 側の NPC 一覧はいったん外した。
 */
(function (root) {
	'use strict';

	var T = 16;						/* 画像のマスの大きさ */
	var HIDE_MAIN = { 3: true, 4: true, 8: true, 9: true, 18: true };	/* 屋根として外す当たり判定タイプ（admin-npcroute.js と同じ） */
	/* 歩ける当たり判定タイプ（サーバーの CharProcMOVERAND.cpp の IsMovePos と同じ） */
	var WALK_HIT = { 0: true, 3: true, 6: true, 7: true, 12: true, 13: true, 14: true, 15: true, 16: true, 17: true };
	/* 当たり判定の名前（server/node/sbosvr_mapdesign.js の HIT_NAME と同じ） */
	var HIT_NAME = ['無し', 'ぶつかる(音付き)', 'ぶつかる(音無し)', '隠れる1', '隠れる2(ぶつかる)', 'カウンター',
		'足跡(雪)', '足跡(砂)', '隠れる3', '隠れる4(ぶつかる)', '隠れる5(背景ごと隠れる)', '隠れる6(ぶつかる/音無し)',
		'一歩前へ進む', '一歩上へ進む', '一歩下へ進む', '一歩左へ進む', '一歩右へ進む', '水の波紋',
		'隠れる7(隠れる3で背景ごと隠れる)'];
	/* イベント種別（common/TypedefCommon.h の EVENTTYPE_*） */
	var EV_NAME = ['同じマップ内の移動', 'ほかのマップへの入口', 'メッセージ', '扉', 'HP/Mana 回復', 'ステータス初期化', '乗っている間マップ変更'];

	var api = null;					/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var sheets = null;
	var parts = [];					/* mapParts の一覧（id 順） */
	var maps = [];					/* [{id, szName}] */
	var mapName = {};				/* id → 名前 */
	var bReady = false;				/* 部品の読み込みが済んだか */
	var bBound = false;				/* ボタンの割り当てが済んだか */
	var loadingBase = null;
	var cur = null;					/* 開いているマップ {id, mapId, W, H, tiles, events} */
	var seq = 0;					/* 古い読み込みの結果を捨てるための番号 */
	var cvBase = null, cvBlock = null, cvLines = null, cvTouch = null;
	var boxCursor = null, boxPin = null;
	var pin = null;					/* クリック（タップ）で止めたマス [x, y] */
	var hover = null;				/* マウスが乗っているマス [x, y] */

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
		var e = $('mv-msg');
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}
	function errText(r) {
		return (r && (r.message || r.error)) || ('HTTP ' + (r && r.httpStatus));
	}
	function pad3(n) {
		return ('00' + n).slice(-3);
	}
	/* 拡大率（1 マスの表示の大きさ ＝ 16 × 拡大率） */
	function scale() {
		return Number($('mv-scale').value) || 2;
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
			mapName = {};
			a.forEach(function (m) { mapName[m.id] = m.szName || ''; });
			bReady = true;
		});
		loadingBase.then(null, function () { loadingBase = null; });
		return loadingBase;
	}

	/* マップのマス目を 128×128 ずつ読む */
	function readTiles(id) {
		var tiles = [];
		function one(x, y) {
			return api('/api/admin/data/tiles', { id: id, x: x, y: y, w: 128, h: 128 }).then(function (r) {
				if (!r.parts) throw new Error(errText(r));
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
		pin = null;
		hover = null;
		$('mv-scroll').hidden = true;
		$('mv-size').textContent = '';
		showInfo();
		setMsg('マップを読んでいます…', true);
		var work = { id: id };
		return readTiles(id).then(function (t) {
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
			work.evAt = {};
			work.events.forEach(function (e) {
				var k = e.x + ',' + e.y;
				(work.evAt[k] = work.evAt[k] || []).push(e);
			});
			cur = work;
			drawAll();
			$('mv-scroll').hidden = false;
			$('mv-size').textContent = 'マップID ' + cur.mapId + '・大きさ ' + cur.W + ' × ' + cur.H + ' マス（座標は 0〜' + (cur.W - 1) + ', 0〜' + (cur.H - 1) + '）';
			showInfo();
			setMsg('');
		}).then(null, function (err) {
			if (err === 'stale') return;
			setMsg('読めませんでした: ' + (err && err.message || err));
		});
	}

	/* ---------------------------------------------------------------- */
	/* 通れるかの判定（CharProcMOVERAND.cpp の IsMovePos と同じ）			 */
	/* ---------------------------------------------------------------- */
	function partAt(x, y) {
		var no = (cur.tiles[y] || [])[x] || 0;
		return { no: no, p: parts[no] || parts[0] || { byHitType: 0 } };
	}
	function isBlocked(x, y) {
		return !WALK_HIT[partAt(x, y).p.byHitType] || !!cur.evAt[x + ',' + y];
	}

	/* ---------------------------------------------------------------- */
	/* 絵																 */
	/* ---------------------------------------------------------------- */
	var cacheTile = {};
	function tileImage(no, bRoof) {
		var key = no + (bRoof ? 'r' : '');
		if (cacheTile[key]) return cacheTile[key];
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
		if (bRoof || !HIDE_MAIN[p.byHitType]) {		/* 「屋根を外す」の時は屋根を描かない */
			put(g, lv ? lv / 100 : 1);
		}
		cacheTile[key] = cv;
		return cv;
	}

	/* 全体図・通れないマスの赤・マス目の線を作り直す（拡大率・屋根を変えた時も） */
	function drawAll() {
		if (!cur) return;
		var S = T * scale();
		var stack = $('mv-stack');
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
		cvBlock = make('mv-over');
		cvLines = make('mv-over');
		var bRoof = !$('mv-roof').checked;
		var ctx = cvBase.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		var x, y;
		for (y = 0; y < cur.H; y++) {
			var row = cur.tiles[y] || [];
			for (x = 0; x < cur.W; x++) {
				ctx.drawImage(tileImage(row[x] || 0, bRoof), x * S, y * S, S, S);
			}
		}
		/* イベントのマス（扉・ワープなど）は水色の枠 */
		ctx.strokeStyle = 'rgba(0,230,255,0.9)';
		ctx.lineWidth = 2;
		cur.events.forEach(function (e) {
			ctx.strokeRect(e.x * S + 1, e.y * S + 1, S - 2, S - 2);
		});
		/* 通れないマスの赤 */
		var bc = cvBlock.getContext('2d');
		bc.fillStyle = 'rgba(255,60,60,0.38)';
		for (y = 0; y < cur.H; y++) {
			for (x = 0; x < cur.W; x++) {
				if (isBlocked(x, y)) bc.fillRect(x * S, y * S, S, S);
			}
		}
		/* マス目の線（10 マスごとに少し濃く） */
		var lc = cvLines.getContext('2d');
		lc.lineWidth = 1;
		[[1, 'rgba(255,255,255,0.22)'], [10, 'rgba(255,255,255,0.55)']].forEach(function (g) {
			lc.strokeStyle = g[1];
			lc.beginPath();
			for (x = 0; x <= cur.W; x += g[0]) { lc.moveTo(x * S + 0.5, 0); lc.lineTo(x * S + 0.5, h); }
			for (y = 0; y <= cur.H; y += g[0]) { lc.moveTo(0, y * S + 0.5); lc.lineTo(w, y * S + 0.5); }
			lc.stroke();
		});
		/* 枠（マウスの位置・止めた位置）は軽く動かせるよう div で重ねる */
		boxPin = el('div', 'mv-cursor mv-pin');
		boxCursor = el('div', 'mv-cursor');
		boxCursor.appendChild(el('span', 'mv-label'));
		boxPin.appendChild(el('span', 'mv-label'));
		stack.appendChild(boxPin);
		stack.appendChild(boxCursor);
		/* 一番上にマウスを受ける透明な板 */
		cvTouch = el('div', 'mv-touch');
		stack.appendChild(cvTouch);
		cvTouch.addEventListener('mousemove', onMove);
		cvTouch.addEventListener('mouseleave', onLeave);
		cvTouch.addEventListener('click', onClick);
		applyToggles();
		placeBox(boxCursor, hover);
		placeBox(boxPin, pin);
	}

	function applyToggles() {
		if (cvBlock) cvBlock.style.display = $('mv-block').checked ? '' : 'none';
		if (cvLines) cvLines.style.display = $('mv-lines').checked ? '' : 'none';
	}

	/* 枠を (x, y) のマスへ。null なら隠す */
	function placeBox(box, p) {
		if (!box) return;
		if (!p) {
			box.style.display = 'none';
			return;
		}
		var S = T * scale();
		box.style.display = '';
		box.style.left = (p[0] * S) + 'px';
		box.style.top = (p[1] * S) + 'px';
		box.style.width = S + 'px';
		box.style.height = S + 'px';
		var lb = box.firstChild;
		lb.textContent = p[0] + ', ' + p[1];
		/* 右端・下端の近くでは、ラベルを内側へ出す */
		lb.classList.toggle('left', p[0] >= cur.W - 3);
		lb.classList.toggle('below', p[1] < 1);
	}

	function cellFromEvent(ev) {
		var rect = cvTouch.getBoundingClientRect();
		var x = Math.floor((ev.clientX - rect.left) / rect.width * cur.W);
		var y = Math.floor((ev.clientY - rect.top) / rect.height * cur.H);
		if (x < 0 || y < 0 || x >= cur.W || y >= cur.H) return null;
		return [x, y];
	}

	function onMove(ev) {
		if (!cur) return;
		var p = cellFromEvent(ev);
		if (p && hover && p[0] === hover[0] && p[1] === hover[1]) return;
		hover = p;
		placeBox(boxCursor, hover);
		showInfo();
	}
	function onLeave() {
		hover = null;
		placeBox(boxCursor, null);
		showInfo();
	}
	/* クリック（タップ）で、そのマスを止めて出したままにする。同じマスをもう一度で外す */
	function onClick(ev) {
		if (!cur) return;
		var p = cellFromEvent(ev);
		if (!p || (pin && pin[0] === p[0] && pin[1] === p[1])) {
			pin = null;
		} else {
			pin = p;
		}
		placeBox(boxPin, pin);
		showInfo();
	}

	/* ---------------------------------------------------------------- */
	/* マスの情報															 */
	/* ---------------------------------------------------------------- */
	function evText(e) {
		var d = e.detail || {};
		var name = EV_NAME[e.type] != null ? EV_NAME[e.type] : '種別 ' + e.type;
		if (e.type === 0 && d.dwX != null) {
			return name + '（行き先 ' + d.dwX + ', ' + d.dwY + '）';
		}
		if ((e.type === 1 || e.type === 6) && d.dwMapID != null) {
			return name + '（行き先 ' + d.dwMapID + (mapName[d.dwMapID] ? ' ' + mapName[d.dwMapID] : '') +
				(e.type === 1 ? ' の ' + d.dwX + ', ' + d.dwY : '') + '）';
		}
		return name;
	}

	function cellText(p) {
		var a = partAt(p[0], p[1]);
		var hit = a.p.byHitType;
		var out = ['座標 ' + p[0] + ', ' + p[1], 'パーツ ' + a.no, '当たり判定「' + (HIT_NAME[hit] || hit) + '」',
			WALK_HIT[hit] ? '通れる' : '通れない'];
		(cur.evAt[p[0] + ',' + p[1]] || []).forEach(function (e) {
			out.push('イベント: ' + evText(e));
		});
		return out.join('・');
	}

	function showInfo() {
		var box = $('mv-info');
		box.textContent = '';
		if (!cur) return;
		if (!hover && !pin) {
			box.appendChild(el('div', 'sub-text', 'マップの上にマウスを乗せると、そのマスの座標が出ます（クリックで止めておけます）。'));
			return;
		}
		if (hover) box.appendChild(el('div', 'mv-line', cellText(hover)));
		if (pin && !(hover && hover[0] === pin[0] && hover[1] === pin[1])) {
			box.appendChild(el('div', 'mv-line mv-line-pin', '止めたマス：' + cellText(pin)));
		}
	}

	function bind() {
		if (bBound) return;
		bBound = true;
		$('mv-map').addEventListener('change', function () {
			var v = $('mv-map').value;
			if (v !== '') loadMap(Number(v));
		});
		$('mv-reload').addEventListener('click', function () {
			var v = $('mv-map').value;
			if (v !== '') loadMap(Number(v));
		});
		$('mv-scale').addEventListener('change', function () {
			/* 見ていた場所がずれないよう、左上のマスを合わせ直す */
			var sc = $('mv-scroll');
			var old = sc.dataset.scale ? Number(sc.dataset.scale) : 2;
			var fx = sc.scrollLeft / old, fy = sc.scrollTop / old;
			drawAll();
			sc.scrollLeft = fx * scale();
			sc.scrollTop = fy * scale();
			sc.dataset.scale = String(scale());
		});
		$('mv-roof').addEventListener('change', drawAll);
		$('mv-block').addEventListener('change', applyToggles);
		$('mv-lines').addEventListener('change', applyToggles);
	}

	/* fnApi(url, body) → Promise<data>（管理ページの api 関数。session は呼ぶ側で入れる） */
	function open(fnApi) {
		api = fnApi;
		bind();
		if (bReady) return;
		setMsg('読み込んでいます…', true);
		ensureBase().then(function () {
			var s = $('mv-map');
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

	root.SboMapView = { open: open };
})(this);
