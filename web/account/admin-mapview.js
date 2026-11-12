/* ========================================================================= */
/* admin-mapview.js															 */
/* 管理ページ（/admin）の「マップ確認」										 */
/* ========================================================================= */
/*
 * マップ全体の絵を出し、マウスを乗せたマスに枠を出して、そのマスの座標・パーツ・イベントを見せる。
 * スマホではタップしたマスを出す。
 * 「編集モード」では、左クリック（ドラッグで続けて）で選んだパーツを置き、右クリックでそのマスのパーツを拾う。
 *   - 変更はまず下書き（/api/admin/mapedit/draft/*。サーバーの SboWebMapDrafts.json。ゲームには出ない）に自動で保存。
 *   - 「サーバーへ反映」で /api/admin/mapedit/apply に変わったマスだけ送る（AI連携の履歴に 1 件残り、元に戻せる）。
 *   - 元に戻す・やり直すはこの画面の中だけ（読み直すと消える）。
 *   - パーツ一覧は出したまま作業できる小窓（名前・タグは /api/admin/parts/dex のパーツ図鑑から）。
 *   - スナップショット（名前を付けて今の姿を残し、あとで読み込む）は /api/admin/mapedit/snap/*。
 *   - 「マップを広げる・新しいマップを足す」は /api/admin/mapedit/resize・add（すぐ反映。履歴からは戻せない）。
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
		'隠れる7(隠れる3で背景ごと隠れる)', '橋(縦に渡る。横切る人は下)', '橋(横に渡る。縦に通る人は下)'];
	/* イベント種別（common/TypedefCommon.h の EVENTTYPE_*） */
	var EV_NAME = ['同じマップ内の移動', 'ほかのマップへの入口', 'メッセージ', '扉', 'HP/Mana 回復', 'ステータス初期化', '乗っている間マップ変更'];

	var VIEW_R = 7;					/* ゲーム画面は自分のキャラを真ん中に左右・上下 7 マスずつ（DRAW_PARTS_X/Y）＝ 15×15 マス */
	var MAX_UNDO = 300;				/* 覚えておく「元に戻す」の数 */
	var SEND_CELLS = 8000;			/* 1 回の反映で送るマスの数（多い時は分けて送る） */
	var DRAFT_DELAY = 2000;			/* 最後に描いてから下書きを自動で保存するまで（ミリ秒） */
	var LS_RECENT = 'sbo_mapedit_recent';

	var api = null;					/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var sheets = null;
	var parts = [];					/* mapParts の一覧（id 順） */
	var maps = [];					/* [{id, szName}] */
	var mapName = {};				/* id → 名前 */
	var bReady = false;				/* 部品の読み込みが済んだか */
	var bBound = false;				/* ボタンの割り当てが済んだか */
	var loadingBase = null;
	var cur = null;					/* 開いているマップ {id, mapId, W, H, tiles, base, events, evAt} */
	var seq = 0;					/* 古い読み込みの結果を捨てるための番号 */
	var cvBase = null, cvBlock = null, cvDirty = null, cvLines = null, cvTouch = null;
	var boxCursor = null, boxPin = null, boxView = null, cvGhost = null;
	var pin = null;					/* クリック（タップ）で止めたマス [x, y] */
	var hover = null;				/* マウスが乗っているマス [x, y] */

	/* 編集 */
	var bEdit = false;				/* 編集モードか */
	var selPart = 1;				/* 置くパーツ */
	var recent = [];				/* 最近使ったパーツ */
	var dirty = {};					/* "x,y" → true（読み込んだ時（反映した時）と違うマス） */
	var nDirty = 0;
	var undoStack = [], redoStack = [];
	var stroke = null;				/* 描いている途中の 1 回分 {cells:[[x, y, 前, 後]], at:{"x,y": 番号}, last:[x, y]} */
	var touchTap = null;			/* スマホのタップ（動かさずに離したら 1 マス置く） */
	var wheelBlockUntil = 0;		/* この時刻（ms）まではマップ欄のホイールでのスクロールを止める（描いた直後の誤スクロール防ぎ） */
	var keepScroll = null;			/* 止めている間に戻すマップ欄のスクロール位置 [左, 上] */
	var draftTimer = 0;
	var draftSaving = null;			/* 保存中の Promise */
	var draftState = '';			/* 下書きの状態の文 */
	var bApplying = false;
	var dex = null;					/* パーツ図鑑 {番号: {name, tags}}（読めなければ null のまま） */
	var dexLoading = null;
	var palBuilt = false;

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
	function hhmm(t) {
		var d = new Date(t);
		return d.getHours() + ':' + ('0' + d.getMinutes()).slice(-2);
	}
	/* 拡大率（1 マスの表示の大きさ ＝ 16 × 拡大率） */
	function scale() {
		return Number($('mv-scale').value) || 2;
	}
	function bViewCursor() {
		return $('mv-cursor').value === 'view';
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

	/* パーツ図鑑の名前・タグ（パーツ一覧の検索用。読めなくても番号だけで使える） */
	function ensureDex() {
		if (dex || dexLoading) return dexLoading || Promise.resolve();
		var all = [];
		function more(offset) {
			return api('/api/admin/parts/dex', { limit: 500, offset: offset }).then(function (r) {
				if (!r.parts) throw new Error(errText(r));
				all = all.concat(r.parts);
				return (r.parts.length && all.length < r.matched) ? more(all.length) : all;
			});
		}
		dexLoading = more(0).then(function (a) {
			dex = {};
			a.forEach(function (e) { dex[e.id] = { name: e.name || '', tags: e.tags || [] }; });
			dexLoading = null;
			filterPalette();
			showPartInfo(selPart);
		}, function () {
			dexLoading = null;
		});
		return dexLoading;
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
		/* 前のマップの下書きは先に保存しておく */
		var pre = flushDraft();
		cur = null;
		unblockScroll();
		pin = null;
		hover = null;
		resetEdit();
		$('mv-scroll').hidden = true;
		$('mv-size').textContent = '';
		showInfo();
		setMsg('マップを読んでいます…', true);
		var work = { id: id };
		return pre.then(function () {
			return readTiles(id);
		}).then(function (t) {
			if (my !== seq) throw 'stale';
			work.tiles = t.tiles;
			work.base = t.tiles.map(function (row) { return row.slice(); });
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
			return api('/api/admin/mapedit/draft/get', { id: id }).then(null, function () { return {}; });
		}).then(function (rd) {
			if (my !== seq) throw 'stale';
			cur = work;
			loadSnapList();
			var msg = restoreDraft(rd && rd.draft);
			drawAll();
			$('mv-scroll').hidden = false;
			$('mv-editbar').hidden = false;
			fitScroll();
			$('mv-size').textContent = 'マップID ' + cur.mapId + '・大きさ ' + cur.W + ' × ' + cur.H + ' マス（座標は 0〜' + (cur.W - 1) + ', 0〜' + (cur.H - 1) + '）';
			showInfo();
			updateEditUi();
			setMsg(msg || '', !!msg);
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
	/* パーツの絵を別の canvas に写す（パーツ一覧・選んでいるパーツの見本用。屋根も描く） */
	function copyTile(cv, no) {
		var ctx = cv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.clearRect(0, 0, cv.width, cv.height);
		ctx.drawImage(tileImage(no, true), 0, 0, cv.width, cv.height);
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
		cvDirty = make('mv-over');
		cvLines = make('mv-over');
		var x, y;
		for (y = 0; y < cur.H; y++) {
			for (x = 0; x < cur.W; x++) {
				drawBaseCell(x, y);
			}
		}
		for (y = 0; y < cur.H; y++) {
			for (x = 0; x < cur.W; x++) {
				drawOverCell(x, y);
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
		/* 枠（マウスの位置・止めた位置・ゲーム画面の広さ）は軽く動かせるよう div で重ねる */
		boxView = el('div', 'mv-view');
		boxPin = el('div', 'mv-cursor mv-pin');
		boxCursor = el('div', 'mv-cursor');
		cvGhost = document.createElement('canvas');
		cvGhost.width = cvGhost.height = T;
		cvGhost.className = 'mv-ghost';
		boxCursor.appendChild(cvGhost);
		boxCursor.appendChild(el('span', 'mv-label'));
		boxPin.appendChild(el('span', 'mv-label'));
		stack.appendChild(boxView);
		stack.appendChild(boxPin);
		stack.appendChild(boxCursor);
		/* 一番上にマウスを受ける透明な板 */
		cvTouch = el('div', 'mv-touch');
		stack.appendChild(cvTouch);
		cvTouch.addEventListener('pointerdown', onDown);
		cvTouch.addEventListener('pointermove', onMove);
		cvTouch.addEventListener('pointerup', onUp);
		cvTouch.addEventListener('pointercancel', onCancel);
		cvTouch.addEventListener('pointerleave', onLeave);
		cvTouch.addEventListener('click', onClick);
		cvTouch.addEventListener('contextmenu', function (ev) {
			if (bEdit) ev.preventDefault();
		});
		applyToggles();
		updateGhost();
		placeCursor();
		placeBox(boxPin, pin);
	}

	/* 1 マスの絵（とイベントの水色の枠） */
	function drawBaseCell(x, y) {
		var S = T * scale();
		var ctx = cvBase.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.drawImage(tileImage((cur.tiles[y] || [])[x] || 0, !$('mv-roof').checked), x * S, y * S, S, S);
		if (cur.evAt[x + ',' + y]) {
			ctx.strokeStyle = 'rgba(0,230,255,0.9)';
			ctx.lineWidth = 2;
			ctx.strokeRect(x * S + 1, y * S + 1, S - 2, S - 2);
		}
	}
	/* 1 マスの通れない赤・変えた印 */
	function drawOverCell(x, y) {
		var S = T * scale();
		var bc = cvBlock.getContext('2d');
		bc.clearRect(x * S, y * S, S, S);
		if (isBlocked(x, y)) {
			bc.fillStyle = 'rgba(255,60,60,0.38)';
			bc.fillRect(x * S, y * S, S, S);
		}
		var dc = cvDirty.getContext('2d');
		dc.clearRect(x * S, y * S, S, S);
		if (dirty[x + ',' + y]) {
			var m = Math.max(4, Math.round(S / 4));
			dc.fillStyle = '#ffd400';
			dc.beginPath();
			dc.moveTo(x * S, y * S);
			dc.lineTo(x * S + m, y * S);
			dc.lineTo(x * S, y * S + m);
			dc.closePath();
			dc.fill();
			dc.strokeStyle = 'rgba(255,212,0,0.8)';
			dc.lineWidth = 1;
			dc.strokeRect(x * S + 0.5, y * S + 0.5, S - 1, S - 1);
		}
	}

	function applyToggles() {
		if (cvBlock) cvBlock.style.display = $('mv-block').checked ? '' : 'none';
		if (cvLines) cvLines.style.display = $('mv-lines').checked ? '' : 'none';
		if (cvDirty) cvDirty.style.display = $('mv-dirty').checked ? '' : 'none';
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
		var lb = box.lastChild;
		lb.textContent = p[0] + ', ' + p[1];
		/* 右端・下端の近くでは、ラベルを内側へ出す */
		lb.classList.toggle('left', p[0] >= cur.W - 3);
		lb.classList.toggle('below', p[1] < 1);
	}
	/* マウスの枠と、ゲーム画面の広さの枠 */
	function placeCursor() {
		placeBox(boxCursor, hover);
		if (!boxView) return;
		if (!hover || !bViewCursor()) {
			boxView.style.display = 'none';
			return;
		}
		var S = T * scale();
		boxView.style.display = '';
		boxView.style.left = ((hover[0] - VIEW_R) * S) + 'px';
		boxView.style.top = ((hover[1] - VIEW_R) * S) + 'px';
		boxView.style.width = boxView.style.height = ((VIEW_R * 2 + 1) * S) + 'px';
	}
	/* 編集モードでは、マウスのマスに置くパーツを薄く重ねて見せる */
	function updateGhost() {
		if (!cvGhost) return;
		cvGhost.style.display = bEdit ? '' : 'none';
		if (bEdit) copyTile(cvGhost, selPart);
	}

	function cellFromEvent(ev) {
		var rect = cvTouch.getBoundingClientRect();
		var x = Math.floor((ev.clientX - rect.left) / rect.width * cur.W);
		var y = Math.floor((ev.clientY - rect.top) / rect.height * cur.H);
		if (x < 0 || y < 0 || x >= cur.W || y >= cur.H) return null;
		return [x, y];
	}

	/* ---------------------------------------------------------------- */
	/* マウス・タッチ														 */
	/* ---------------------------------------------------------------- */
	function onDown(ev) {
		if (!cur || !bEdit) return;
		var p = cellFromEvent(ev);
		if (!p) return;
		/* 下の preventDefault で入力欄（パーツ一覧の検索など）から外れなくなるので、ここで外す（Ctrl+Z を効かせるため） */
		var ae = document.activeElement;
		if (ae && ae !== document.body && ae.blur) ae.blur();
		if (ev.pointerType === 'touch') {
			/* スマホは画面を動かせるよう、動かさずに離した時だけ 1 マス置く */
			touchTap = { id: ev.pointerId, p: p, x: ev.clientX, y: ev.clientY };
			return;
		}
		if (ev.button === 2) {
			ev.preventDefault();
			pickPart(p);
			return;
		}
		if (ev.button !== 0) return;
		ev.preventDefault();
		try { cvTouch.setPointerCapture(ev.pointerId); } catch (e) { /* 無くても描ける */ }
		var sc = $('mv-scroll');
		keepScroll = [sc.scrollLeft, sc.scrollTop];
		stroke = { cells: [], at: {}, last: p };
		paintCell(p[0], p[1]);
	}
	function onMove(ev) {
		if (!cur) return;
		if (touchTap && ev.pointerId === touchTap.id &&
				Math.abs(ev.clientX - touchTap.x) + Math.abs(ev.clientY - touchTap.y) > 10) {
			touchTap = null;
		}
		var p = cellFromEvent(ev);
		if (stroke && p && (ev.buttons & 1)) {
			/* 速く動かしても間が空かないよう、前のマスから線でつなぐ */
			linePoints(stroke.last, p).forEach(function (q) { paintCell(q[0], q[1]); });
			stroke.last = p;
		}
		if (p && hover && p[0] === hover[0] && p[1] === hover[1]) return;
		hover = p;
		placeCursor();
		showInfo();
	}
	function onUp(ev) {
		if (touchTap && ev.pointerId === touchTap.id) {
			var p = touchTap.p;
			touchTap = null;
			stroke = { cells: [], at: {}, last: p };
			paintCell(p[0], p[1]);
		}
		endStroke();
	}
	function onCancel() {
		touchTap = null;
		endStroke();
	}
	function onLeave(ev) {
		if (stroke && (ev.buttons & 1)) return;		/* 描いている途中は枠を残す */
		hover = null;
		placeCursor();
		showInfo();
	}
	/* クリック（タップ）で、そのマスを止めて出したままにする。同じマスをもう一度で外す（見るだけの時） */
	function onClick(ev) {
		if (!cur || bEdit) return;
		var p = cellFromEvent(ev);
		if (!p || (pin && pin[0] === p[0] && pin[1] === p[1])) {
			pin = null;
		} else {
			pin = p;
		}
		placeBox(boxPin, pin);
		showInfo();
	}

	/* a から b までのマス（a は含めない） */
	function linePoints(a, b) {
		var out = [];
		var x0 = a[0], y0 = a[1], x1 = b[0], y1 = b[1];
		var dx = Math.abs(x1 - x0), dy = -Math.abs(y1 - y0);
		var sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
		var err = dx + dy;
		while (x0 !== x1 || y0 !== y1) {
			var e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
			out.push([x0, y0]);
		}
		return out;
	}

	/* ---------------------------------------------------------------- */
	/* 編集																 */
	/* ---------------------------------------------------------------- */
	function resetEdit() {
		dirty = {};
		nDirty = 0;
		undoStack = [];
		redoStack = [];
		stroke = null;
		touchTap = null;
		draftState = '';
	}

	/* マスの中身を変えて描き直す（下書きの印も） */
	function setCell(x, y, v) {
		var row = cur.tiles[y];
		if (row[x] === v) return false;
		row[x] = v;
		var k = x + ',' + y;
		var bDirty = v !== cur.base[y][x];
		if (bDirty && !dirty[k]) {
			dirty[k] = true;
			nDirty++;
		} else if (!bDirty && dirty[k]) {
			delete dirty[k];
			nDirty--;
		}
		drawBaseCell(x, y);
		drawOverCell(x, y);
		return true;
	}

	function paintCell(x, y) {
		if (!stroke) return;
		var old = cur.tiles[y][x];
		if (!setCell(x, y, selPart)) return;
		var k = x + ',' + y;
		if (k in stroke.at) {
			stroke.cells[stroke.at[k]][3] = selPart;
		} else {
			stroke.at[k] = stroke.cells.length;
			stroke.cells.push([x, y, old, selPart]);
		}
		if (hover && hover[0] === x && hover[1] === y) showInfo();
	}

	function endStroke() {
		if (!stroke) return;
		var s = stroke;
		stroke = null;
		/* Magic Mouse などは、ボタンを離した時に表面をなでてホイールが出やすいので、離してから少しの間はスクロールしない */
		wheelBlockUntil = Date.now() + WHEEL_BLOCK_MS;
		if (!s.cells.length) return;
		pushAction(s.cells);
		useRecent(selPart);
	}

	/*
	 * マップ欄のホイール（Magic Mouse・トラックパッドのなで）。描いている間と、離してから少しの間は止める。
	 * Magic Mouse はクリックする前から表面に指が触れていて、なでの始まりがボタンを押す前になることがある。
	 * ブラウザは始まりの 1 回目を止めなかったなでを最後まで止められない（preventDefault が効かない）ので、
	 * スクロールしてしまったら押した時の位置へ戻す（onScroll）。
	 */
	var WHEEL_BLOCK_MS = 1000;
	var WHEEL_GAP_MS = 200;			/* 止めている間に続いたなで（慣性も）は、途切れるまでまとめて止める */
	function scrollBlocked() {
		return !!stroke || Date.now() < wheelBlockUntil;
	}
	function onWheel(ev) {
		if (ev.ctrlKey) return;			/* ピンチ・Ctrl+ホイールはブラウザの拡大のまま */
		if (!scrollBlocked()) return;
		if (ev.cancelable) ev.preventDefault();
		wheelBlockUntil = Math.max(wheelBlockUntil, Date.now() + WHEEL_GAP_MS);
	}
	function onScroll() {
		var sc = $('mv-scroll');
		if (scrollBlocked() && keepScroll) {
			if (sc.scrollLeft !== keepScroll[0]) sc.scrollLeft = keepScroll[0];
			if (sc.scrollTop !== keepScroll[1]) sc.scrollTop = keepScroll[1];
			return;
		}
		keepScroll = [sc.scrollLeft, sc.scrollTop];
	}
	/* プログラムで動かす時（拡大の切り替えなど）は止めない */
	function unblockScroll() {
		wheelBlockUntil = 0;
		keepScroll = null;
	}

	function pushAction(cells) {
		undoStack.push(cells);
		if (undoStack.length > MAX_UNDO) undoStack.shift();
		redoStack = [];
		changed();
	}

	function undo() {
		if (stroke || !undoStack.length) return;
		var cells = undoStack.pop();
		for (var i = cells.length - 1; i >= 0; i--) setCell(cells[i][0], cells[i][1], cells[i][2]);
		redoStack.push(cells);
		changed();
	}
	function redo() {
		if (stroke || !redoStack.length) return;
		var cells = redoStack.pop();
		cells.forEach(function (c) { setCell(c[0], c[1], c[3]); });
		undoStack.push(cells);
		changed();
	}

	/* 右クリックで、そのマスのパーツを拾う */
	function pickPart(p) {
		selectPart(cur.tiles[p[1]][p[0]] || 0);
	}

	function selectPart(no) {
		selPart = no;
		copyTile($('mv-sel-cv'), no);
		$('mv-sel-no').textContent = 'パーツ ' + no;
		updateGhost();
		var grid = $('mv-pal-grid');
		Array.prototype.forEach.call(grid.querySelectorAll('.mv-pal-cell.sel'), function (b) { b.classList.remove('sel'); });
		var b = grid.querySelector('[data-no="' + no + '"]');
		if (b) {
			b.classList.add('sel');
			if (!$('mv-pal').hidden && b.scrollIntoView) b.scrollIntoView({ block: 'nearest' });
		}
		showPartInfo(no);
		showInfo();
	}

	function useRecent(no) {
		recent = [no].concat(recent.filter(function (n) { return n !== no; })).slice(0, 20);
		try { localStorage.setItem(LS_RECENT, JSON.stringify(recent)); } catch (e) { /* 覚えられなくても困らない */ }
		drawRecent();
	}

	/* 変更を全部やめる（元に戻すで取り消せる） */
	function discardAll() {
		if (!cur || !nDirty) return;
		if (!confirm('反映していない変更（' + nDirty + ' マス）を全部やめて、読み込んだ時の姿に戻します。よろしいですか？\n（「元に戻す」で取り消せます）')) return;
		var cells = [];
		Object.keys(dirty).forEach(function (k) {
			var a = k.split(',').map(Number);
			cells.push([a[0], a[1], cur.tiles[a[1]][a[0]], cur.base[a[1]][a[0]]]);
		});
		cells.forEach(function (c) { setCell(c[0], c[1], c[3]); });
		pushAction(cells);
	}

	/* 何か変わった時：ボタン・下書きの自動保存 */
	function changed() {
		updateEditUi();
		showInfo();
		scheduleDraft();
	}

	function updateEditUi() {
		$('mv-undo').disabled = !undoStack.length;
		$('mv-redo').disabled = !redoStack.length;
		$('mv-apply').disabled = !nDirty || bApplying;
		$('mv-discard').disabled = !nDirty;
		$('mv-apply').textContent = nDirty ? 'サーバーへ反映（' + nDirty + ' マス）' : 'サーバーへ反映';
		var st = $('mv-state');
		st.textContent = nDirty ? ('反映していない変更 ' + nDirty + ' マス' + (draftState ? '・' + draftState : '')) :
			(draftState || '変更はありません');
		st.classList.toggle('dirty', !!nDirty);
	}

	/* 変わったマスの一覧 [[x, y, 前（読み込んだ時）, 後], ...] */
	function dirtyCells() {
		return Object.keys(dirty).map(function (k) {
			var a = k.split(',').map(Number);
			return [a[0], a[1], cur.base[a[1]][a[0]], cur.tiles[a[1]][a[0]]];
		});
	}

	/* ---------------------------------------------------------------- */
	/* 下書き（サーバーに保存。ゲームには出ない）							 */
	/* ---------------------------------------------------------------- */
	function scheduleDraft() {
		if (draftTimer) clearTimeout(draftTimer);
		draftTimer = setTimeout(function () {
			draftTimer = 0;
			saveDraft();
		}, DRAFT_DELAY);
		draftState = '下書き保存待ち';
		updateEditUi();
	}
	function saveDraft() {
		if (!cur) return Promise.resolve();
		if (draftTimer) {
			clearTimeout(draftTimer);
			draftTimer = 0;
		}
		var my = cur;
		var cells = dirtyCells();
		if (cells.length > 10000) {
			draftState = '変更が多すぎて下書きに残せません（反映してください）';
			updateEditUi();
			return Promise.resolve();
		}
		draftState = '下書きを保存中…';
		updateEditUi();
		var p = (draftSaving || Promise.resolve()).then(function () {
			return api('/api/admin/mapedit/draft/save', { id: my.id, cells: cells });
		}).then(function (r) {
			if (cur !== my) return;
			if (!r.ok) throw new Error(errText(r));
			draftState = cells.length ? '下書き保存 ' + hhmm(r.savedAt || Date.now()) : '';
			updateEditUi();
		}).then(null, function (err) {
			if (cur !== my) return;
			draftState = '下書きを保存できませんでした（' + (err && err.message || err) + '）';
			updateEditUi();
		});
		draftSaving = p;
		p.then(function () { if (draftSaving === p) draftSaving = null; });
		return p;
	}
	/* 保存待ちの下書きがあればすぐ保存する */
	function flushDraft() {
		if (draftTimer) return saveDraft();
		return draftSaving || Promise.resolve();
	}

	/* 読み込んだマップに下書きを重ねる。戻り値は知らせる文 */
	function restoreDraft(d) {
		if (!d || !d.cells || !d.cells.length) return '';
		if (d.cx !== cur.W || d.cy !== cur.H) {
			return '下書き（' + d.cells.length + ' マス）は、マップの大きさが変わったので使えませんでした。';
		}
		var nConflict = 0, nUse = 0;
		d.cells.forEach(function (c) {
			var row = cur.tiles[c[1]];
			if (!row) return;
			if (row[c[0]] !== c[2]) nConflict++;
			if (row[c[0]] === c[3]) return;
			row[c[0]] = c[3];
			dirty[c[0] + ',' + c[1]] = true;
			nUse++;
		});
		nDirty = nUse;
		draftState = '下書き ' + hhmm(d.savedAt);
		var msg = '前の下書き（' + nUse + ' マス・' + new Date(d.savedAt).toLocaleString() + ' 保存）の続きから始めます。まだゲームには出ていません。';
		if (nConflict) {
			msg += ' 下書きの後にサーバーで変わったマスが ' + nConflict + ' 個あります（反映すると下書きの方で上書きします）。';
		}
		return msg;
	}

	/* ---------------------------------------------------------------- */
	/* サーバーへ反映														 */
	/* ---------------------------------------------------------------- */
	function applyToServer() {
		if (!cur || !nDirty || bApplying) return;
		var my = cur;
		var cells = dirtyCells();
		if (!confirm('マップ「' + pad3(my.id) + ' ' + (mapName[my.id] || '') + '」の ' + cells.length + ' マスをゲームに反映します。\n' +
				'ログイン中の人の画面にもすぐ出ます（「AI連携」の履歴から元に戻せます）。よろしいですか？')) return;
		bApplying = true;
		updateEditUi();
		setMsg('反映しています…', true);
		var chunks = [];
		for (var i = 0; i < cells.length; i += SEND_CELLS) chunks.push(cells.slice(i, i + SEND_CELLS));
		var nDone = 0, aHist = [];
		function send(chunk, bForce) {
			return api('/api/admin/mapedit/apply', { id: my.id, cells: chunk, force: !!bForce }).then(function (r) {
				if (r.error === 'conflict') {
					var ex = r.conflicts.slice(0, 5).map(function (c) { return '(' + c[0] + ', ' + c[1] + ')'; }).join(' ');
					if (!confirm('読み込んだ後に、ほかで変わったマスが ' + r.conflictCount + ' 個あります（' + ex + (r.conflictCount > 5 ? ' など' : '') + '）。\n' +
							'こちらの変更で上書きしますか？\n（キャンセルすると反映を止めます。「読み直す」で今のサーバーの姿を見られます。下書きは残ります）')) {
						throw new Error('ほかで変わったマスがあるので止めました');
					}
					return send(chunk, true);
				}
				if (!r.ok) throw new Error(errText(r));
				return r;
			});
		}
		chunks.reduce(function (p, chunk) {
			return p.then(function () {
				return send(chunk, false).then(function (r) {
					/* 送れた分は「読み込んだ時の姿」を今の姿にする */
					chunk.forEach(function (c) {
						my.base[c[1]][c[0]] = c[3];
						var k = c[0] + ',' + c[1];
						if (dirty[k] && my.tiles[c[1]][c[0]] === c[3]) {
							delete dirty[k];
							nDirty--;
						}
						if (cur === my) drawOverCell(c[0], c[1]);
					});
					nDone += r.changed || 0;
					if (r.historyId) aHist.push(r.historyId);
				});
			});
		}, Promise.resolve()).then(function () {
			bApplying = false;
			if (cur !== my) return;
			setMsg('反映しました（' + nDone + ' マス' + (aHist.length ? '・AI連携の履歴 ' + aHist.join(', ') : '') + '）。', true);
			changed();
		}, function (err) {
			bApplying = false;
			if (cur !== my) return;
			setMsg('反映できませんでした: ' + (err && err.message || err) + (nDone ? '（' + nDone + ' マスは反映済み）' : ''));
			changed();
		});
	}

	/* ---------------------------------------------------------------- */
	/* パーツ一覧（出したまま作業できる小窓）								 */
	/* ---------------------------------------------------------------- */
	function partLabel(no) {
		var d = dex && dex[no];
		var p = parts[no] || {};
		var hit = p.byHitType || 0;
		return 'パーツ ' + no + (d && d.name ? '「' + d.name + '」' : '') + '・' + (HIT_NAME[hit] || hit) +
			(WALK_HIT[hit] ? '・通れる' : '・通れない') + (d && d.tags && d.tags.length ? '・' + d.tags.join(' ') : '');
	}
	function showPartInfo(no) {
		var box = $('mv-pal-info');
		if (box) box.textContent = '選んでいるパーツ：' + partLabel(no) + '（右クリックでマップから拾えます）';
	}
	function palCell(no) {
		var b = el('button', 'mv-pal-cell');
		b.type = 'button';
		b.dataset.no = String(no);
		var cv = document.createElement('canvas');
		cv.width = cv.height = T;
		copyTile(cv, no);
		b.appendChild(cv);
		b.title = String(no);
		b.addEventListener('mouseenter', function () {
			b.title = partLabel(no);
		});
		b.addEventListener('click', function () {
			selectPart(no);
		});
		return b;
	}
	function buildPalette() {
		if (palBuilt) return;
		palBuilt = true;
		var grid = $('mv-pal-grid');
		grid.textContent = '';
		for (var i = 0; i < parts.length; i++) {
			if (parts[i]) grid.appendChild(palCell(i));
		}
		drawRecent();
		selectPart(selPart);
		filterPalette();
	}
	function drawRecent() {
		var box = $('mv-pal-recent');
		box.textContent = '';
		if (!recent.length) return;
		box.appendChild(el('span', 'sub-text', '最近：'));
		recent.forEach(function (no) {
			if (parts[no]) box.appendChild(palCell(no));
		});
	}
	function filterPalette() {
		if (!palBuilt) return;
		var q = $('mv-pal-q').value.trim();
		var f = $('mv-pal-filter').value;
		var used = null;
		if (f === 'map' && cur) {
			used = {};
			cur.tiles.forEach(function (row) { row.forEach(function (v) { used[v] = true; }); });
		}
		Array.prototype.forEach.call($('mv-pal-grid').children, function (b) {
			var no = Number(b.dataset.no);
			var p = parts[no] || {};
			var bShow = true;
			if (q) {
				var d = dex && dex[no];
				bShow = String(no) === q || (!!d && (d.name.indexOf(q) >= 0 || d.tags.some(function (t) { return t.indexOf(q) >= 0; })));
			}
			if (bShow && used) bShow = !!used[no];
			if (bShow && f === 'walk') bShow = !!WALK_HIT[p.byHitType || 0];
			if (bShow && f === 'block') bShow = !WALK_HIT[p.byHitType || 0];
			b.style.display = bShow ? '' : 'none';
		});
	}
	function openPalette() {
		var pal = $('mv-pal');
		buildPalette();
		pal.hidden = !pal.hidden;
		if (!pal.hidden) {
			ensureDex();
			selectPart(selPart);
		}
	}
	/* 見出しを引っぱって動かす */
	function bindPaletteDrag() {
		var pal = $('mv-pal'), head = $('mv-pal-head');
		var drag = null;
		head.addEventListener('pointerdown', function (ev) {
			if (ev.target.tagName === 'BUTTON') return;
			var r = pal.getBoundingClientRect();
			drag = { dx: ev.clientX - r.left, dy: ev.clientY - r.top };
			try { head.setPointerCapture(ev.pointerId); } catch (e) { /* 無くても動かせる */ }
			ev.preventDefault();
		});
		head.addEventListener('pointermove', function (ev) {
			if (!drag) return;
			var x = Math.max(0, Math.min(window.innerWidth - 60, ev.clientX - drag.dx));
			var y = Math.max(0, Math.min(window.innerHeight - 40, ev.clientY - drag.dy));
			pal.style.left = x + 'px';
			pal.style.top = y + 'px';
			pal.style.right = 'auto';
		});
		function end() { drag = null; }
		head.addEventListener('pointerup', end);
		head.addEventListener('pointercancel', end);
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
		if (cur.base[p[1]][p[0]] !== a.no) {
			out.push('変更前はパーツ ' + cur.base[p[1]][p[0]]);
		}
		(cur.evAt[p[0] + ',' + p[1]] || []).forEach(function (e) {
			out.push('イベント: ' + evText(e));
		});
		return out.join('・');
	}

	function showInfo() {
		var box = $('mv-info');
		box.textContent = '';
		if (!cur) return;
		if (bEdit) {
			box.appendChild(el('div', 'mv-line mv-line-pin', '置くパーツ：' + partLabel(selPart)));
		}
		if (!hover && !pin) {
			box.appendChild(el('div', 'sub-text', bEdit ? '左クリックで置く（押したまま動かすと続けて置く）・右クリックでそのマスのパーツを拾う。' :
				'マップの上にマウスを乗せると、そのマスの座標が出ます（クリックで止めておけます）。'));
			return;
		}
		if (hover) box.appendChild(el('div', 'mv-line', cellText(hover)));
		if (pin && !(hover && hover[0] === pin[0] && hover[1] === pin[1])) {
			box.appendChild(el('div', 'mv-line mv-line-pin', '止めたマス：' + cellText(pin)));
		}
	}

	function setEdit(b) {
		bEdit = !!b;
		$('mv-edit').checked = bEdit;
		$('mv-tools').hidden = !bEdit;
		$('mv-sizebox').hidden = !bEdit;
		$('mv-snapbox').hidden = !bEdit;
		document.body.classList.toggle('mv-editing', bEdit);
		if (bEdit) {
			pin = null;
			placeBox(boxPin, null);
		} else {
			$('mv-pal').hidden = true;
		}
		updateGhost();
		updateEditUi();
		showInfo();
	}

	/* Ctrl+Z / Ctrl+Y（Mac は ⌘）。文字の入力欄にいる時は使わない */
	function onKey(ev) {
		if (!bEdit || !cur || $('tab-mapview').hidden) return;
		var t = ev.target && ev.target.tagName;
		if (t === 'INPUT' || t === 'TEXTAREA' || t === 'SELECT') return;
		if (!(ev.ctrlKey || ev.metaKey)) return;
		var k = String(ev.key).toLowerCase();
		if (k === 'z' && !ev.shiftKey) {
			ev.preventDefault();
			undo();
		} else if (k === 'y' || (k === 'z' && ev.shiftKey)) {
			ev.preventDefault();
			redo();
		}
	}

	/*
	 * マップの欄の高さを「画面の残り」にする（ページとマップの欄でスクロールが二重にならないように）。
	 * 上の欄（編集のボタン・マスの情報など）の高さが変わった時・画面の大きさが変わった時に合わせ直す。
	 * 上の欄が高すぎる時（スマホなど）は 240px は残し、その分だけページが動く。
	 */
	function fitScroll() {
		var sc = $('mv-scroll');
		if (!sc || sc.hidden || $('tab-mapview').hidden) return;
		var top = sc.getBoundingClientRect().top + window.pageYOffset;
		var h = Math.max(240, Math.floor(window.innerHeight - top - 12));
		sc.style.maxHeight = h + 'px';
		/* 下の余白の分だけまだページがはみ出していたら、その分縮める */
		var over = document.documentElement.scrollHeight - window.innerHeight;
		if (over > 0 && h > 240) {
			sc.style.maxHeight = Math.max(240, h - over) + 'px';
		}
	}
	function bindFit() {
		window.addEventListener('resize', fitScroll);
		if (root.ResizeObserver) {
			var ro = new ResizeObserver(function () { fitScroll(); });
			Array.prototype.forEach.call($('tab-mapview').children, function (c) {
				if (c.id !== 'mv-scroll' && c.id !== 'mv-pal') ro.observe(c);
			});
			ro.observe(document.querySelector('header') || document.body.firstElementChild);
		}
	}

	function bind() {
		if (bBound) return;
		bBound = true;
		bindFit();
		try {
			var a = JSON.parse(localStorage.getItem(LS_RECENT) || '[]');
			if (Array.isArray(a)) recent = a.filter(function (n) { return typeof n === 'number'; }).slice(0, 20);
		} catch (e) { /* 無ければ空 */ }
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
			unblockScroll();
			drawAll();
			sc.scrollLeft = fx * scale();
			sc.scrollTop = fy * scale();
			sc.dataset.scale = String(scale());
		});
		$('mv-roof').addEventListener('change', drawAll);
		$('mv-block').addEventListener('change', applyToggles);
		$('mv-lines').addEventListener('change', applyToggles);
		$('mv-dirty').addEventListener('change', applyToggles);
		$('mv-cursor').addEventListener('change', placeCursor);
		$('mv-edit').addEventListener('change', function () { setEdit($('mv-edit').checked); });
		$('mv-parts-btn').addEventListener('click', openPalette);
		$('mv-pal-close').addEventListener('click', function () { $('mv-pal').hidden = true; });
		$('mv-pal-q').addEventListener('input', filterPalette);
		$('mv-pal-filter').addEventListener('change', filterPalette);
		$('mv-undo').addEventListener('click', undo);
		$('mv-redo').addEventListener('click', redo);
		$('mv-draft-save').addEventListener('click', function () {
			saveDraft();
		});
		$('mv-discard').addEventListener('click', discardAll);
		$('mv-apply').addEventListener('click', applyToServer);
		$('mv-rs-sel').addEventListener('click', function () { $('mv-rs-fill').value = String(selPart); });
		$('mv-resize').addEventListener('click', resizeMap);
		$('mv-snap-save').addEventListener('click', saveSnap);
		$('mv-snap-name').addEventListener('keydown', function (ev) {
			if (ev.key === 'Enter' && !ev.isComposing) saveSnap();
		});
		$('mv-add').addEventListener('click', addMap);
		document.addEventListener('keydown', onKey);
		window.addEventListener('pointerup', endStroke);
		$('mv-scroll').addEventListener('wheel', onWheel, { passive: false });
		$('mv-scroll').addEventListener('scroll', onScroll, { passive: true });
		/* 下書きの保存待ちがある時は、閉じる前に知らせる */
		window.addEventListener('beforeunload', function (ev) {
			if (draftTimer || draftSaving || bApplying) {
				ev.preventDefault();
				ev.returnValue = '';
			}
		});
		bindPaletteDrag();
	}

	function fillMapSelect() {
		var s = $('mv-map');
		var v = s.value;
		s.textContent = '';
		var o0 = el('option', null, 'マップを選んでください');
		o0.value = '';
		s.appendChild(o0);
		maps.forEach(function (m) {
			var o = el('option', null, pad3(m.id) + ' ' + (m.szName || '（名前なし）'));
			o.value = String(m.id);
			s.appendChild(o);
		});
		s.value = v;
	}

	/* ---------------------------------------------------------------- */
	/* スナップショット（名前を付けて残すマップの姿。ゲームは変わらない）	 */
	/* ---------------------------------------------------------------- */
	function loadSnapList() {
		var box = $('mv-snap-list');
		if (!cur) {
			box.textContent = '';
			return Promise.resolve();
		}
		var my = cur;
		return api('/api/admin/mapedit/snap/list', { id: my.id }).then(function (r) {
			if (cur !== my) return;
			box.textContent = '';
			if (!r.snapshots) {
				box.appendChild(el('div', 'sub-text', '一覧を読めませんでした: ' + errText(r)));
				return;
			}
			if (!r.snapshots.length) {
				box.appendChild(el('div', 'sub-text', 'このマップのスナップショットはまだありません。'));
				return;
			}
			r.snapshots.forEach(function (s) {
				var row = el('div', 'mv-snap');
				row.appendChild(el('b', null, s.name));
				row.appendChild(el('span', 'sub-text', new Date(s.at).toLocaleString() + '・' + s.cx + '×' + s.cy +
					(s.cx !== my.W || s.cy !== my.H ? '（大きさが今と違うので読み込めません）' : '')));
				var bLoad = el('button', 'sub', '読み込む');
				bLoad.type = 'button';
				bLoad.disabled = s.cx !== my.W || s.cy !== my.H;
				bLoad.addEventListener('click', function () { loadSnap(s); });
				var bDel = el('button', 'sub', '消す');
				bDel.type = 'button';
				bDel.addEventListener('click', function () { deleteSnap(s); });
				row.appendChild(bLoad);
				row.appendChild(bDel);
				box.appendChild(row);
			});
		});
	}
	function saveSnap() {
		if (!cur) return;
		var name = $('mv-snap-name').value.trim();
		if (!name) {
			alert('スナップショットの名前を入れてください（例：修正前）。');
			return;
		}
		var my = cur;
		api('/api/admin/mapedit/snap/save', { id: my.id, name: name, cells: dirtyCells() }).then(function (r) {
			if (!r.ok) throw new Error(errText(r));
			$('mv-snap-name').value = '';
			setMsg('スナップショット「' + name + '」を残しました。' + (r.dropped ? '（いっぱいなので古いものを ' + r.dropped + ' 個消しました）' : ''), true);
			return loadSnapList();
		}).then(null, function (err) {
			setMsg('スナップショットを残せませんでした: ' + (err && err.message || err));
		});
	}
	function loadSnap(s) {
		if (!cur || stroke || bApplying) return;
		var my = cur;
		api('/api/admin/mapedit/snap/get', { id: my.id, snapId: s.snapId }).then(function (r) {
			if (cur !== my) return;
			if (!r.snapshot) throw new Error(errText(r));
			var p = r.snapshot.parts;
			if (r.snapshot.cx !== my.W || r.snapshot.cy !== my.H) throw new Error('マップの大きさが今と違います');
			var cells = [];
			for (var y = 0; y < my.H; y++) {
				for (var x = 0; x < my.W; x++) {
					if (my.tiles[y][x] !== p[y][x]) cells.push([x, y, my.tiles[y][x], p[y][x]]);
				}
			}
			if (!cells.length) {
				setMsg('今の姿はスナップショット「' + s.name + '」と同じです。', true);
				return;
			}
			if (!confirm('編集画面をスナップショット「' + s.name + '」の姿にします（' + cells.length + ' マスが変わります）。\n' +
					'「元に戻す」で取り消せます。ゲームに出すには、そのあと「サーバーへ反映」を押してください。')) return;
			cells.forEach(function (c) { setCell(c[0], c[1], c[3]); });
			pushAction(cells);
			setMsg('スナップショット「' + s.name + '」を読み込みました（' + cells.length + ' マス。まだゲームには出ていません）。', true);
		}).then(null, function (err) {
			setMsg('読み込めませんでした: ' + (err && err.message || err));
		});
	}
	function deleteSnap(s) {
		if (!cur) return;
		if (!confirm('スナップショット「' + s.name + '」を消します。よろしいですか？')) return;
		api('/api/admin/mapedit/snap/delete', { id: cur.id, snapId: s.snapId }).then(function (r) {
			if (!r.ok) throw new Error(errText(r));
			return loadSnapList();
		}).then(null, function (err) {
			setMsg('消せませんでした: ' + (err && err.message || err));
		});
	}

	/* ---------------------------------------------------------------- */
	/* マップを広げる・足す（すぐゲームに反映。履歴からは戻せない）		 */
	/* ---------------------------------------------------------------- */
	function numVal(id) {
		var v = Math.floor(Number($(id).value));
		return isFinite(v) && v > 0 ? v : 0;
	}
	function resizeMap() {
		if (!cur) {
			setMsg('先にマップを選んでください。');
			return;
		}
		if (nDirty || stroke || bApplying) {
			alert('反映していない変更があります。先に「サーバーへ反映」するか「変更を全部やめる」を押してください。');
			return;
		}
		var my = cur;
		var body = { id: my.id, top: numVal('mv-rs-top'), bottom: numVal('mv-rs-bottom'), left: numVal('mv-rs-left'),
			right: numVal('mv-rs-right'), fill: numVal('mv-rs-fill') };
		var check = {};
		Object.keys(body).forEach(function (k) { check[k] = body[k]; });
		check.check = true;
		api('/api/admin/mapedit/resize', check).then(function (r) {
			if (!r.ok) throw new Error(errText(r));
			var msg = 'マップ「' + pad3(my.id) + ' ' + (mapName[my.id] || '') + '」を ' + r.oldCx + '×' + r.oldCy + ' から ' + r.cx + '×' + r.cy + ' に広げます。\n' +
				'広げた所はパーツ ' + body.fill + ' で埋めます。';
			if (r.shiftX || r.shiftY) {
				msg += '\n上・左に広げるので、このマップの座標は全部 右へ ' + r.shiftX + '・下へ ' + r.shiftY + ' ずれます（イベント・NPC なども一緒にずらします）。';
			}
			msg += '\nログイン中の人にもすぐ反映され、履歴からは元に戻せません。よろしいですか？';
			if (!confirm(msg)) throw 'cancel';
			setMsg('広げています…', true);
			return api('/api/admin/mapedit/resize', body);
		}).then(function (r) {
			if (!r.ok) throw new Error(errText(r));
			['mv-rs-top', 'mv-rs-bottom', 'mv-rs-left', 'mv-rs-right'].forEach(function (id) { $(id).value = '0'; });
			return loadMap(my.id).then(function () {
				setMsg('広げました（' + r.oldCx + '×' + r.oldCy + ' → ' + r.cx + '×' + r.cy + '）。', true);
			});
		}).then(null, function (err) {
			if (err === 'cancel') return;
			setMsg('広げられませんでした: ' + (err && err.message || err));
		});
	}
	function addMap() {
		var name = $('mv-add-name').value.trim();
		var body = { cx: numVal('mv-add-cx'), cy: numVal('mv-add-cy'), fill: numVal('mv-add-fill'), szName: name };
		if (!name) {
			alert('新しいマップの名前を入れてください。');
			return;
		}
		if (!confirm('新しいマップ「' + name + '」（' + body.cx + '×' + body.cy + '・パーツ ' + body.fill + ' で埋める）を足します。\n' +
				'番号はいちばん後ろになります。履歴からは消せません。よろしいですか？')) return;
		flushDraft().then(function () {
			return api('/api/admin/mapedit/add', body);
		}).then(function (r) {
			if (!r.ok) throw new Error(errText(r));
			$('mv-add-name').value = '';
			return listAll('map', { fields: ['szName'] }).then(function (a) {
				maps = a;
				mapName = {};
				a.forEach(function (m) { mapName[m.id] = m.szName || ''; });
				fillMapSelect();
				$('mv-map').value = String(r.id);
				return loadMap(r.id);
			}).then(function () {
				setMsg('マップ ' + pad3(r.id) + '「' + name + '」を足しました。入口はほかのマップのイベント（ほかのマップへの入口）で作ってください。', true);
			});
		}).then(null, function (err) {
			setMsg('足せませんでした: ' + (err && err.message || err));
		});
	}

	/* fnApi(url, body) → Promise<data>（管理ページの api 関数。session は呼ぶ側で入れる） */
	function open(fnApi) {
		api = fnApi;
		bind();
		setTimeout(fitScroll, 0);
		if (bReady) return;
		setMsg('読み込んでいます…', true);
		ensureBase().then(function () {
			fillMapSelect();
			$('mv-editbar').hidden = false;
			copyTile($('mv-sel-cv'), selPart);
			$('mv-sel-no').textContent = 'パーツ ' + selPart;
			setMsg('');
		}).then(null, function (err) {
			setMsg('読み込めませんでした: ' + (err && err.message || err));
		});
	}

	root.SboMapView = { open: open };
})(this);
