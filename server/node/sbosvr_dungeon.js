/* ========================================================================= */
/* sbosvr_dungeon.js														 */
/* ダンジョン生成のテーマ登録とお試し作成（Emscripten の --pre-js で組み込む）	 */
/* ========================================================================= */
/*
 * 仕様は docs/dungeon-gen.md。テーマは SBODATA/SboWebDungeonThemes.json に {seq, themes:[...]} で保存する。
 * 生成そのものは C++（server/web_dungeon_svr.cpp の SboWebDungeonGen）に任せ、ここは入力の確かめと保存だけ。
 * 管理ページ用（{session}。管理者だけ）:
 *   /api/admin/dungeon/themes        → {themes}
 *   /api/admin/dungeon/theme/save    {theme}（id 無し・0 なら追加）→ {ok, theme}
 *   /api/admin/dungeon/theme/delete  {id} → {ok}
 *   /api/admin/dungeon/themes/import {themes, mode:'add'|'replace'} → {ok, added, replaced, themes}
 *   /api/admin/dungeon/preview       {themeId? , theme?, params} → SboWebDungeonGen の結果
 */
Module['sboDungeon'] = (function () {
	var path = require('path');
	var D = Module['sboData'] && Module['sboData'].ext;
	if (!D) {
		return null;
	}

	var MAX_THEMES = 50;
	var MAX_ROOMS = 50;
	var MAX_DECOS = 50;
	var MAX_OPENINGS = 50;
	var MAX_LIGHTS = 8;
	var MAX_OPENING_SIDE = 16;
	var MAX_LIST = 64;				/* 床などのパーツ番号の並びの長さ */
	var MAX_ROOM_SIDE = 64;
	var MAX_DECO_SIDE = 16;
	var ALGOS = [1, 3];

	var store = null, strPath = '';

	function load() {
		if (!store) {
			strPath = path.join(D.dataDir(), 'SboWebDungeonThemes.json');
			store = D.readJsonFile(strPath, {});
			store.themes = Array.isArray(store.themes) ? store.themes : [];
			store.seq = store.seq || store.themes.reduce(function (m, t) { return Math.max(m, t.id || 0); }, 0);
		}
		return store;
	}

	function save() {
		D.writeJsonFile(strPath, store);
	}

	function str(v, max) {
		return String(v == null ? '' : v).replace(/[\u0000-\u0008\u000b\u000c\u000e-\u001f]/g, '').slice(0, max);
	}

	function bad(label, msg) {
		D.fail(400, 'theme', label + msg);
	}

	/* 範囲つきの整数 */
	function int(v, min, max, label) {
		if (typeof v !== 'number' || !isFinite(v) || Math.floor(v) !== v || v < min || v > max) {
			bad(label, ' は ' + min + '〜' + max + ' の整数にしてください');
		}
		return v;
	}

	function partNo(v, nParts, label) {
		return int(v, 0, nParts - 1, label);
	}

	function partList(a, nParts, label, bAllowEmpty) {
		if (!Array.isArray(a) || a.length > MAX_LIST || (!bAllowEmpty && a.length === 0)) {
			bad(label, ' はパーツ番号の配列（' + (bAllowEmpty ? 0 : 1) + '〜' + MAX_LIST + ' 個）にしてください');
		}
		return a.map(function (v) { return partNo(v, nParts, label); });
	}

	function cuts(a, w, label) {
		if (!Array.isArray(a) || a.length !== 2) {
			bad(label, ' は [a, b] の 2 つにしてください');
		}
		int(a[0], 1, w, label);
		int(a[1], 1, w, label);
		if (!(0 < a[0] && a[0] < a[1] && a[1] <= w)) {
			bad(label, ' は 0 < a < b <= ' + w + ' にしてください');
		}
		return [a[0], a[1]];
	}

	function cellList(a, w, h, nParts, label) {
		if (!Array.isArray(a) || a.length !== w * h) {
			bad(label, ' の cells は w*h = ' + (w * h) + ' 個にしてください');
		}
		return a.map(function (v) { return partNo(v, nParts, label + ' の cells'); });
	}

	/* テーマを確かめて、きれいな形にして返す（id は呼ぶ側で決める） */
	function checkTheme(t) {
		if (!t || typeof t !== 'object') {
			D.fail(400, 'theme', 'theme が必要です');
		}
		var nParts = D.count('mapParts');
		var o = {
			name: str(t.name, 40).trim() || '無題',
			algo: int(t.algo, 1, 99, 'algo'),
			floor: partList(t.floor, nParts, 'floor', false),
			wallTop: partList(t.wallTop, nParts, 'wallTop', false),
			wallFace: partList(t.wallFace, nParts, 'wallFace', false),
			wallFill: partList(t.wallFill, nParts, 'wallFill', false),
			'void': partNo(t['void'] == null ? 0 : t['void'], nParts, 'void'),
			arch: partList(t.arch == null ? [] : t.arch, nParts, 'arch', true),
			door: partNo(t.door == null ? 0 : t.door, nParts, 'door'),
			bottomRows: int(t.bottomRows == null ? 1 : t.bottomRows, 1, 2, 'bottomRows'),
			rooms: [],
			decos: [],
			openings: [],
			lights: []
		};
		if (ALGOS.indexOf(o.algo) < 0) {
			bad('algo', ' は ' + ALGOS.join(' か ') + ' にしてください');
		}
		var rooms = t.rooms == null ? [] : t.rooms;
		var decos = t.decos == null ? [] : t.decos;
		if (!Array.isArray(rooms) || rooms.length > MAX_ROOMS) {
			bad('rooms', ' は ' + MAX_ROOMS + ' 件までの配列にしてください');
		}
		if (!Array.isArray(decos) || decos.length > MAX_DECOS) {
			bad('decos', ' は ' + MAX_DECOS + ' 件までの配列にしてください');
		}
		var openings = t.openings == null ? [] : t.openings;
		var lights = t.lights == null ? [] : t.lights;
		if (!Array.isArray(openings) || openings.length > MAX_OPENINGS) {
			bad('openings', ' は ' + MAX_OPENINGS + ' 件までの配列にしてください');
		}
		if (!Array.isArray(lights) || lights.length > MAX_LIGHTS) {
			bad('lights', ' は ' + MAX_LIGHTS + ' 件までの配列にしてください');
		}
		rooms.forEach(function (r, i) {
			var lb = '部屋 ' + (i + 1);
			r = r || {};
			var w = int(r.w, 1, MAX_ROOM_SIDE, lb + ' の w');
			var h = int(r.h, 1, MAX_ROOM_SIDE, lb + ' の h');
			var room = {
				name: str(r.name, 40).trim() || '部屋',
				kind: int(r.kind == null ? 0 : r.kind, 0, 2, lb + ' の kind'),
				w: w, h: h,
				cells: cellList(r.cells, w, h, nParts, lb),
				cutX: cuts(r.cutX, w, lb + ' の cutX'),
				cutY: cuts(r.cutY, h, lb + ' の cutY'),
				fixed: r.fixed === true,
				entryX: r.entryX == null ? -1 : int(r.entryX, -1, w - 1, lb + ' の entryX'),
				entryY: r.entryY == null ? -1 : int(r.entryY, -1, h - 1, lb + ' の entryY')
			};
			if (r.fixed != null && typeof r.fixed !== 'boolean') {
				bad(lb + ' の fixed', ' は true か false にしてください');
			}
			if (r.src && typeof r.src === 'object') {
				room.src = {
					map: int(r.src.map, 0, 99999, lb + ' の src.map'),
					x: int(r.src.x, 0, 99999, lb + ' の src.x'),
					y: int(r.src.y, 0, 99999, lb + ' の src.y')
				};
			}
			o.rooms.push(room);
		});
		decos.forEach(function (d, i) {
			var lb = '飾り ' + (i + 1);
			d = d || {};
			var w = int(d.w, 1, MAX_DECO_SIDE, lb + ' の w');
			var h = int(d.h, 1, MAX_DECO_SIDE, lb + ' の h');
			var on = int(d.on, 1, 3, lb + ' の on');
			if (on !== 1 && on !== 3) {
				bad(lb, ' の on は 1（部屋の床）か 3（壁の側面）にしてください');
			}
			o.decos.push({
				name: str(d.name, 40).trim() || '飾り',
				w: w, h: h,
				cells: cellList(d.cells, w, h, nParts, lb),
				on: on,
				rate: int(d.rate, 0, 100, lb + ' の rate'),
				max: int(d.max, 0, 999, lb + ' の max')
			});
		});
		openings.forEach(function (op, i) {
			var lb = '出入口パターン ' + (i + 1);
			op = op || {};
			var dir = int(op.dir, 0, 1, lb + ' の dir');
			var type = int(op.type, 1, 3, lb + ' の type');
			var w = int(op.w, 1, MAX_OPENING_SIDE, lb + ' の w');
			var h = int(op.h, 1, MAX_OPENING_SIDE, lb + ' の h');
			if (dir === 0 && h !== 2) {
				bad(lb, ' は横の壁（dir 0）なので h は 2 にしてください');
			}
			if (dir === 1 && w !== 1) {
				bad(lb, ' は縦の壁（dir 1）なので w は 1 にしてください');
			}
			var len = dir === 0 ? w : h;
			var hole0 = int(op.hole0, 0, len, lb + ' の hole0');
			var hole1 = int(op.hole1, 0, len, lb + ' の hole1');
			if (!(hole0 < hole1)) {
				bad(lb, ' は 0 <= hole0 < hole1 <= ' + len + ' にしてください');
			}
			var cellsOpen = op.cellsOpen == null ? [] : op.cellsOpen;
			if (!Array.isArray(cellsOpen) || (cellsOpen.length !== 0 && cellsOpen.length !== w * h)) {
				bad(lb, ' の cellsOpen は空か w*h = ' + (w * h) + ' 個にしてください');
			}
			o.openings.push({
				name: str(op.name, 40).trim() || '出入口',
				dir: dir, type: type, w: w, h: h,
				cells: cellList(op.cells, w, h, nParts, lb),
				cellsOpen: type === 2 ? cellList(cellsOpen, cellsOpen.length ? w : 0, cellsOpen.length ? h : 0, nParts, lb + ' の cellsOpen') : [],
				hole0: hole0, hole1: hole1
			});
		});
		lights.forEach(function (l, i) {
			var lb = '灯り ' + (i + 1);
			l = l || {};
			o.lights.push({
				wParts: partNo(l.wParts, nParts, lb + ' の wParts'),
				byLight: int(l.byLight, 0, 7, lb + ' の byLight')
			});
		});
		return o;
	}

	/* お試し作成の設定。数は範囲に収める（C++ 側でも確かめる） */
	var PARAM_RANGE = {
		w: [8, 128], h: [8, 128], seed: [0, 4294967295], roomMin: [3, 64], roomMax: [3, 64], floorMin: [0, 100],
		archMin: [1, 16], archMax: [1, 16], doorRate: [0, 100], openRate: [0, 100], loopRate: [0, 100], cutoff: [0, 100],
		vertRate: [0, 100], vertDoorRate: [-1, 100], vertArchMin: [0, 16], vertArchMax: [0, 16]
	};

	function checkParams(p) {
		var o = {};
		p = (p && typeof p === 'object') ? p : {};
		Object.keys(PARAM_RANGE).forEach(function (k) {
			if (p[k] == null || p[k] === '') {
				return;
			}
			var v = Number(p[k]);
			if (!isFinite(v)) {
				D.fail(400, 'params', k + ' は数にしてください');
			}
			o[k] = Math.max(PARAM_RANGE[k][0], Math.min(PARAM_RANGE[k][1], Math.floor(v)));
		});
		return o;
	}

	function findTheme(id) {
		var ts = load().themes;
		for (var i = 0; i < ts.length; i++) {
			if (ts[i].id === id) {
				return i;
			}
		}
		return -1;
	}

	function opThemes() {
		return { themes: load().themes };
	}

	function opSave(a, who) {
		a = a || {};
		var s = load();
		var t = checkTheme(a.theme);
		var id = a.theme.id;
		var idx = -1;
		if (id != null && id !== 0) {
			idx = findTheme(id);
			if (idx < 0) {
				D.fail(404, 'not_found', 'テーマ ' + id + ' 番はありません（新しく足す時は id を付けないか 0 にしてください）');
			}
		} else if (s.themes.length >= MAX_THEMES) {
			D.fail(400, 'too_many', 'テーマは ' + MAX_THEMES + ' 件までです');
		}
		var o = { id: idx >= 0 ? s.themes[idx].id : ++s.seq };
		Object.keys(t).forEach(function (k) { o[k] = t[k]; });
		if (idx >= 0) {
			s.themes[idx] = o;
		} else {
			s.themes.push(o);
		}
		save();
		D.log(who.label + ' がダンジョンのテーマ ' + o.id + '「' + o.name + '」を保存');
		return { ok: true, theme: o };
	}

	/* 書き出したテーマ（本番へ持っていく用）を入れる。全部確かめてから反映する。
	 * mode 'add' = id を振り直して追加、'replace' = 同じ id を上書き（無い id はその番号で追加） */
	function opImport(a, who) {
		a = a || {};
		var s = load();
		if (!Array.isArray(a.themes) || a.themes.length === 0 || a.themes.length > MAX_THEMES) {
			D.fail(400, 'themes', 'themes は 1〜' + MAX_THEMES + ' 件の配列にしてください');
		}
		if (a.mode !== 'add' && a.mode !== 'replace') {
			D.fail(400, 'mode', "mode は 'add' か 'replace' にしてください");
		}
		var checked = a.themes.map(function (t, i) {
			var o = checkTheme(t);
			var id = t && t.id;
			if (a.mode === 'replace' && !(typeof id === 'number' && Math.floor(id) === id && id > 0 && id < 1000000)) {
				D.fail(400, 'theme', (i + 1) + ' 件目のテーマに id がありません（上書きには id が要ります）');
			}
			return { id: id, t: o };
		});
		var list = s.themes.slice(), seq = s.seq, nAdd = 0, nRep = 0;
		checked.forEach(function (c) {
			var idx = -1;
			if (a.mode === 'replace') {
				list.forEach(function (x, k) { if (x.id === c.id) idx = k; });
			}
			var o = { id: idx >= 0 ? c.id : (a.mode === 'replace' ? c.id : ++seq) };
			Object.keys(c.t).forEach(function (k) { o[k] = c.t[k]; });
			if (idx >= 0) {
				list[idx] = o;
				nRep++;
			} else {
				list.push(o);
				nAdd++;
			}
			seq = Math.max(seq, o.id);
		});
		if (list.length > MAX_THEMES) {
			D.fail(400, 'too_many', 'テーマは ' + MAX_THEMES + ' 件までです（入れると ' + list.length + ' 件になります）');
		}
		s.themes = list;
		s.seq = seq;
		save();
		D.log(who.label + ' がダンジョンのテーマを読み込み（追加 ' + nAdd + '・上書き ' + nRep + '）');
		return { ok: true, added: nAdd, replaced: nRep, themes: s.themes };
	}

	function opDelete(a, who) {
		var idx = findTheme(a && a.id);
		if (idx < 0) {
			D.fail(404, 'not_found', 'テーマ ' + (a && a.id) + ' 番はありません');
		}
		var t = store.themes.splice(idx, 1)[0];
		save();
		D.log(who.label + ' がダンジョンのテーマ ' + t.id + '「' + t.name + '」を削除');
		return { ok: true };
	}

	function opPreview(a) {
		a = a || {};
		var theme = null;
		if (a.theme != null) {
			theme = checkTheme(a.theme);		/* 保存前の編集中のもの */
		} else if (a.themeId != null) {
			var idx = findTheme(a.themeId);
			if (idx < 0) {
				D.fail(404, 'not_found', 'テーマ ' + a.themeId + ' 番はありません');
			}
			theme = store.themes[idx];
		}
		if (typeof _SboWebDungeonGen !== 'function') {
			return { ok: false, error: 'サーバーが古い' };
		}
		var req = { params: checkParams(a.params) };
		if (theme) {
			req.theme = theme;
		}
		var p = stringToNewUTF8(JSON.stringify(req));
		try {
			return JSON.parse(UTF8ToString(_SboWebDungeonGen(p)));
		} catch (e) {
			return { ok: false, error: '生成に失敗しました（' + (e && e.message || e) + '）' };
		} finally {
			_free(p);
		}
	}

	D.addApiAdmin('/api/admin/dungeon/themes', function (body) { D.adminWho(body); return opThemes(); });
	D.addApiAdmin('/api/admin/dungeon/theme/save', function (body) { return opSave(body, D.adminWho(body)); });
	D.addApiAdmin('/api/admin/dungeon/themes/import', function (body) { return opImport(body, D.adminWho(body)); });
	D.addApiAdmin('/api/admin/dungeon/theme/delete', function (body) { return opDelete(body, D.adminWho(body)); });
	D.addApiAdmin('/api/admin/dungeon/preview', function (body) { D.adminWho(body); return opPreview(body); });

	return {};
})();
