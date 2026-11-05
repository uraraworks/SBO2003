/* ========================================================================= */
/* sbosvr_mapdesign.js														 */
/* マップづくりの手伝い: パーツ図鑑と配置案（Emscripten の --pre-js で組み込む）	 */
/* ========================================================================= */
/*
 * マップパーツ（マス目に入る番号）には名前も意味も無いので、AI が新しい街やダンジョンを考えられるよう
 * 「パーツ図鑑」を持つ。さらに AI が考えた配置を、ゲームのデータを変えずに「配置案」として保存し、
 * 管理ページで画像として見られるようにする。気に入った案は、管理者ウィンドウで見ながら手で置く。
 *
 * 1. パーツ図鑑（SBODATA/SboWebPartsDex.json）
 *    パーツごとの 名前・タグ・説明 と、組み合わせの見本（壁で囲った家など。マス目は配置案と同じ形）。
 *    ファイルが無い時は、管理ページと一緒に配る parts-dex-seed.json（最初の名前の下書き）から始める。
 *    これに、データから自動で分かること（通れるか・当たり判定の種類・アニメ・使われている回数と
 *    マップ・よく上下左右に来るパーツ）を足して返す。自動の分は 10 分ごとに数え直す。
 *    名前・タグ・説明・見本を変えられるのは、管理ページと「書き込みもできる」AI 用キー。
 *
 * 2. 配置案（SBODATA/SboWebMapPlans.json）
 *    {title, note, w, h, parts[行][列], marks:[{x, y, w?, h?, label, text?}]}。最大 128×128・MAX_PLANS 件。
 *    ゲームのデータは一切変えないので、読むだけのキーでも保存できる。
 *
 * MCP のツール（sbosvr_data.js の /mcp に足す）
 *   sbo_parts_dex        パーツ図鑑を引く（名前・タグ・番号・通れるか・使われているかで絞る）
 *   sbo_parts_samples    組み合わせの見本
 *   sbo_parts_dex_update 名前・タグ・説明・見本を変える（書き込みもできるキーだけ）
 *   sbo_plan_save / sbo_plan_list / sbo_plan_get / sbo_plan_delete  配置案
 * HTTP（AI 用キー）: /api/data/parts/dex・/api/data/parts/samples・/api/data/parts/update・
 *                    /api/data/plans/save・/api/data/plans/list・/api/data/plans/get・/api/data/plans/delete
 * 管理ページ用（{session}）: /api/admin/parts/dex・/api/admin/parts/update・/api/admin/plans/list・
 *                    /api/admin/plans/get・/api/admin/plans/delete
 */
Module['sboMapDesign'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var D = Module['sboData'] && Module['sboData'].ext;
	if (!D) {
		return null;
	}

	var MAX_PLANS = 200;			/* 残す配置案の数（古いものから消す） */
	var MAX_SIDE = 128;				/* 配置案・見本の 1 辺 */
	var MAX_SAMPLES = 300;
	var MAX_TAGS = 12;
	var STATS_TTL = 10 * 60 * 1000;	/* 自動で分かる分を数え直す間隔 */

	/* 当たり判定の種類（管理者ウィンドウのマップパーツ編集と同じ名前） */
	var HIT_NAME = ['無し', 'ぶつかる(音付き)', 'ぶつかる(音無し)', '隠れる1', '隠れる2(ぶつかる)', 'カウンター',
		'足跡(雪)', '足跡(砂)', '隠れる3', '隠れる4(ぶつかる)', '隠れる5(背景ごと隠れる)', '隠れる6(ぶつかる/音無し)',
		'一歩前へ進む', '一歩上へ進む', '一歩下へ進む', '一歩左へ進む', '一歩右へ進む', '水の波紋',
		'隠れる7(隠れる3で背景ごと隠れる)'];
	/* ぶつかる（通れない）種類。カウンターは向こう側の人と話せるが、自分は通れない */
	var HIT_BLOCK = { 1: 1, 2: 1, 4: 1, 5: 1, 9: 1, 11: 1 };
	/* 上を歩くと消える（屋根・木の上など）種類 */
	var HIT_HIDE = { 3: 1, 4: 1, 8: 1, 9: 1, 10: 1, 11: 1, 18: 1 };

	var dexStore = null, strDexPath = '';
	var planStore = null, strPlanPath = '';
	var stats = null;				/* {at, parts:{番号: {uses, maps:{map: 回数}, nb:{up:{}, ...}}}, mapName:[]} */

	/* ------------------------------------------------------------------ */
	/* 保存ファイル																 */
	/* ------------------------------------------------------------------ */
	/* 管理ページと一緒に配る最初の下書き（ブラウザ版の配布フォルダの admin-grp/parts-dex-seed.json） */
	function readSeed() {
		var www = Module['sboNet'] && Module['sboNet'].wwwDir ? Module['sboNet'].wwwDir() : '';
		if (!www) {
			return null;
		}
		return D.readJsonFile(path.join(www, 'admin-grp', 'parts-dex-seed.json'), null);
	}

	function dex() {
		if (!dexStore) {
			strDexPath = path.join(D.dataDir(), 'SboWebPartsDex.json');
			var o = fs.existsSync(strDexPath) ? D.readJsonFile(strDexPath, {}) : null;
			if (!o) {
				o = readSeed() || {};
				if (o.parts) {
					D.log('パーツ図鑑を最初の下書き（parts-dex-seed.json）から作りました');
				}
			}
			o.version = 1;
			o.parts = (o.parts && typeof o.parts === 'object') ? o.parts : {};
			o.samples = Array.isArray(o.samples) ? o.samples : [];
			o.sampleSeq = o.sampleSeq || o.samples.reduce(function (m, s) { return Math.max(m, s.id || 0); }, 0);
			dexStore = o;
		}
		return dexStore;
	}

	function saveDex() {
		D.writeJsonFile(strDexPath, dexStore);
	}

	function plans() {
		if (!planStore) {
			strPlanPath = path.join(D.dataDir(), 'SboWebMapPlans.json');
			planStore = D.readJsonFile(strPlanPath, {});
			planStore.version = 1;
			planStore.seq = planStore.seq || 0;
			planStore.plans = Array.isArray(planStore.plans) ? planStore.plans : [];
		}
		return planStore;
	}

	/* ------------------------------------------------------------------ */
	/* データから自動で分かること												 */
	/* ------------------------------------------------------------------ */
	function bump(o, k) {
		o[k] = (o[k] || 0) + 1;
	}

	/* 全マップのマス目を数える（128×128 ずつ読む） */
	function computeStats() {
		var nMap = D.count('map');
		var P = {}, aName = [];
		function at(no) {
			return P[no] || (P[no] = { uses: 0, maps: {}, pos: {}, nb: { up: {}, down: {}, left: {}, right: {} } });
		}
		for (var id = 0; id < nMap; id++) {
			var first = D.opTiles({ id: id, x: 0, y: 0, w: 1, h: 1 });
			var rec = D.getRecord('map', id);
			aName[id] = rec ? (rec.szName || '') : '';
			var cx = first.cx, cy = first.cy;
			var grid = [];
			for (var y0 = 0; y0 < cy; y0 += MAX_SIDE) {
				for (var x0 = 0; x0 < cx; x0 += MAX_SIDE) {
					var t = D.opTiles({ id: id, x: x0, y: y0, w: MAX_SIDE, h: MAX_SIDE });
					t.parts.forEach(function (row, dy) {
						var g = grid[y0 + dy] || (grid[y0 + dy] = []);
						row.forEach(function (v, dx) { g[x0 + dx] = v; });
					});
				}
			}
			for (var y = 0; y < cy; y++) {
				for (var x = 0; x < cx; x++) {
					var v = grid[y][x];
					var s = at(v);
					s.uses++;
					if (!s.maps[id]) {
						s.pos[id] = [x, y];	/* そのマップで最初に見つけた場所（使い方の例を見せる時に使う） */
					}
					bump(s.maps, id);
					if (y > 0) bump(s.nb.up, grid[y - 1][x]);
					if (y < cy - 1) bump(s.nb.down, grid[y + 1][x]);
					if (x > 0) bump(s.nb.left, grid[y][x - 1]);
					if (x < cx - 1) bump(s.nb.right, grid[y][x + 1]);
				}
			}
		}
		return { at: Date.now(), parts: P, mapName: aName };
	}

	function getStats() {
		if (!stats || Date.now() - stats.at > STATS_TTL) {
			var t = Date.now();
			stats = computeStats();
			D.log('パーツの使われ方を数えました（' + (Date.now() - t) + 'ms）');
		}
		return stats;
	}

	/* {番号: 回数} → 多い順の [{id, n}]（自分自身は除く） */
	function top(o, n, self) {
		return Object.keys(o).map(Number).filter(function (k) { return k !== self; }).sort(function (a, b) {
			return o[b] - o[a] || a - b;
		}).slice(0, n).map(function (k) { return { id: k, n: o[k] }; });
	}

	/* 1 パーツぶんの図鑑の行 */
	function entry(id, rec, bDetail) {
		var d = dex().parts[id] || {};
		var st = getStats().parts[id];
		var e = {
			id: id,
			name: d.name || '',
			tags: d.tags || [],
			note: d.note || '',
			hit: rec.byHitType,
			hitName: HIT_NAME[rec.byHitType] || String(rec.byHitType),
			passable: !HIT_BLOCK[rec.byHitType],
			hideOnTop: !!HIT_HIDE[rec.byHitType],
			anime: (rec.anime || []).length,
			uses: st ? st.uses : 0
		};
		if (bDetail && st) {
			var names = getStats().mapName;
			e.maps = top(st.maps, 5).map(function (m) {
				return { map: m.id, name: names[m.id] || '', n: m.n, x: st.pos[m.id][0], y: st.pos[m.id][1] };
			});
			e.neighbors = {};
			['up', 'down', 'left', 'right'].forEach(function (k) {
				e.neighbors[k] = top(st.nb[k], 4, id).map(function (m) {
					var dd = dex().parts[m.id];
					return { id: m.id, name: dd && dd.name || '', n: m.n };
				});
			});
		}
		return e;
	}

	/* ------------------------------------------------------------------ */
	/* 入力の確かめ																 */
	/* ------------------------------------------------------------------ */
	function str(v, max) {
		return String(v == null ? '' : v).replace(/[\u0000-\u0008\u000b\u000c\u000e-\u001f]/g, '').slice(0, max);
	}

	function tagList(v) {
		if (typeof v === 'string') {
			v = v.split(/[,、，\s]+/);
		}
		if (!Array.isArray(v)) {
			return [];
		}
		var seen = {};
		return v.map(function (t) { return str(t, 20).trim(); }).filter(function (t) {
			if (!t || seen[t]) {
				return false;
			}
			seen[t] = 1;
			return true;
		}).slice(0, MAX_TAGS);
	}

	/* マス目（parts[行][列]）を確かめる。番号はパーツの数より小さいこと */
	function checkGrid(a, label) {
		var nParts = D.count('mapParts');
		var grid = a.parts;
		if (!Array.isArray(grid) || grid.length === 0 || grid.length > MAX_SIDE) {
			D.fail(400, 'parts', label + 'の parts は [行][列] の 2 次元の配列で、1〜' + MAX_SIDE + ' 行にしてください');
		}
		var w = 0;
		grid.forEach(function (row) {
			if (!Array.isArray(row) || row.length > MAX_SIDE) {
				D.fail(400, 'parts', label + 'の parts の 1 行は ' + MAX_SIDE + ' 列までの配列にしてください');
			}
			w = Math.max(w, row.length);
		});
		if (w === 0) {
			D.fail(400, 'parts', label + 'の parts が空です');
		}
		return grid.map(function (row, y) {
			var out = [];
			for (var x = 0; x < w; x++) {
				var v = row[x] == null ? 0 : row[x];
				if (typeof v !== 'number' || !isFinite(v) || v < 0 || Math.floor(v) !== v || v >= nParts) {
					D.fail(400, 'parts', label + 'の (' + x + ',' + y + ') の番号 ' + JSON.stringify(v) + ' は使えません（0〜' + (nParts - 1) + '）');
				}
				out.push(v);
			}
			return out;
		});
	}

	function checkMarks(marks, w, h) {
		if (marks == null) {
			return [];
		}
		if (!Array.isArray(marks) || marks.length > 200) {
			D.fail(400, 'marks', 'marks は 200 個までの配列にしてください');
		}
		return marks.map(function (m) {
			m = m || {};
			var n = function (v, def) { return (typeof v === 'number' && isFinite(v)) ? Math.floor(v) : def; };
			var o = { x: Math.max(0, Math.min(w - 1, n(m.x, 0))), y: Math.max(0, Math.min(h - 1, n(m.y, 0))) };
			o.w = Math.max(1, Math.min(w - o.x, n(m.w, 1)));
			o.h = Math.max(1, Math.min(h - o.y, n(m.h, 1)));
			o.label = str(m.label, 16);
			o.text = str(m.text, 200);
			return o;
		});
	}

	/* ------------------------------------------------------------------ */
	/* パーツ図鑑																 */
	/* ------------------------------------------------------------------ */
	function opDex(a) {
		a = a || {};
		var nParts = D.count('mapParts');
		var q = a.q != null ? str(a.q, 40) : '';
		var tag = a.tag != null ? str(a.tag, 20) : '';
		var aIds = Array.isArray(a.ids) ? a.ids.filter(function (v) { return typeof v === 'number'; }).slice(0, 200) : null;
		var offset = Math.max(0, Math.floor(Number(a.offset) || 0));
		var limit = Math.min(500, Math.max(1, Math.floor(Number(a.limit) || 100)));
		var bDetail = a.detail === true || !!aIds;
		var out = [], nMatch = 0, tagCount = {}, nNamed = 0;
		var list = aIds || Array.apply(null, { length: nParts }).map(function (_, i) { return i; });
		var i;
		for (i = 0; i < list.length; i++) {
			var id = list[i];
			if (id < 0 || id >= nParts) {
				continue;
			}
			var d = dex().parts[id] || {};
			(d.tags || []).forEach(function (t) { bump(tagCount, t); });
			if (d.name) {
				nNamed++;
			}
			if (aIds == null) {
				if (q && String(id) !== q && String(d.name || '').indexOf(q) < 0 && String(d.note || '').indexOf(q) < 0 &&
						!(d.tags || []).some(function (t) { return t.indexOf(q) >= 0; })) {
					continue;
				}
				if (tag && (d.tags || []).indexOf(tag) < 0) {
					continue;
				}
				if (a.named === true && !d.name) continue;
				if (a.named === false && d.name) continue;
				var rec = D.getRecord('mapParts', id);
				if (typeof a.passable === 'boolean' && a.passable === !!HIT_BLOCK[rec.byHitType]) continue;
				if (a.used === true && !(getStats().parts[id] || {}).uses) continue;
				if (a.used === false && (getStats().parts[id] || {}).uses) continue;
				if (nMatch++ < offset || out.length >= limit) {
					continue;
				}
				out.push(entry(id, rec, bDetail));
			} else {
				out.push(entry(id, D.getRecord('mapParts', id), true));
			}
		}
		return {
			total: nParts,
			named: nNamed,
			matched: aIds ? out.length : nMatch,
			offset: offset,
			tags: Object.keys(tagCount).sort(function (x, y) { return tagCount[y] - tagCount[x] || (x < y ? -1 : 1); }).map(function (t) {
				return { tag: t, n: tagCount[t] };
			}),
			note: 'id は mapParts の番号（マップのマス目 parts に入る値）。passable=通れるか、hideOnTop=上を歩くと消える（屋根・木の上など）、' +
				'uses=全マップで使われているマスの数、maps=よく使われているマップ（x・y はそのマップで最初に見つけた場所）、neighbors=よく上下左右に来るパーツ（maps と neighbors は detail か ids の時だけ）。' +
				'0 番は何も無いマス。',
			parts: out
		};
	}

	function opSamples(a) {
		a = a || {};
		var q = a.q != null ? str(a.q, 40) : '';
		var list = dex().samples.filter(function (s) {
			if (a.id != null && s.id !== a.id) return false;
			if (q && (s.title + ' ' + s.note + ' ' + (s.tags || []).join(' ')).indexOf(q) < 0) return false;
			return true;
		});
		return {
			note: 'parts[行][列] は配置案と同じ形。左上が (0,0)。',
			samples: list.map(function (s) {
				var o = { id: s.id, title: s.title, note: s.note, tags: s.tags || [], w: s.w, h: s.h, by: s.by, at: s.at };
				if (a.grid !== false) {
					o.parts = s.parts;
				}
				return o;
			})
		};
	}

	/* 名前・タグ・説明・見本を変える（ゲームのデータではないので、下見なしですぐ保存） */
	function opDexUpdate(a, who) {
		D.requireWrite(who);
		a = a || {};
		var nParts = D.count('mapParts');
		var dx = dex();
		var nPart = 0, aSample = [];
		if (a.parts != null) {
			if (!Array.isArray(a.parts) || a.parts.length > 500) {
				D.fail(400, 'parts', 'parts は [{id, name?, tags?, note?}] の配列で、500 件までにしてください');
			}
			a.parts.forEach(function (p) {
				var id = D.checkId(p && p.id);
				if (id >= nParts) {
					D.fail(400, 'id', 'mapParts に ' + id + ' 番はありません（' + nParts + ' 件）');
				}
			});
			a.parts.forEach(function (p) {
				var cur = dx.parts[p.id] || {};
				if (p.name !== undefined) cur.name = str(p.name, 40).trim();
				if (p.tags !== undefined) cur.tags = tagList(p.tags);
				if (p.note !== undefined) cur.note = str(p.note, 300);
				cur.by = who.label;
				cur.at = Date.now();
				if (!cur.name && !(cur.tags || []).length && !cur.note) {
					delete dx.parts[p.id];
				} else {
					dx.parts[p.id] = cur;
				}
				nPart++;
			});
		}
		if (a.samples != null) {
			if (!Array.isArray(a.samples) || a.samples.length > 50) {
				D.fail(400, 'samples', 'samples は 50 件までの配列にしてください');
			}
			a.samples.forEach(function (s) {
				s = s || {};
				var idx = -1;
				if (s.id != null) {
					dx.samples.forEach(function (x, k) { if (x.id === s.id) idx = k; });
					if (idx < 0) {
						D.fail(404, 'not_found', '見本 ' + s.id + ' 番はありません');
					}
				}
				if (s['delete'] === true) {
					if (idx >= 0) {
						dx.samples.splice(idx, 1);
						aSample.push({ id: s.id, deleted: true });
					}
					return;
				}
				var grid = checkGrid(s, '見本');
				var o = {
					id: idx >= 0 ? dx.samples[idx].id : ++dx.sampleSeq,
					title: str(s.title, 60).trim() || '見本',
					note: str(s.note, 500),
					tags: tagList(s.tags),
					w: grid[0].length,
					h: grid.length,
					parts: grid,
					by: who.label,
					at: Date.now()
				};
				if (idx >= 0) {
					dx.samples[idx] = o;
				} else {
					if (dx.samples.length >= MAX_SAMPLES) {
						D.fail(400, 'too_many', '見本は ' + MAX_SAMPLES + ' 件までです');
					}
					dx.samples.push(o);
				}
				aSample.push({ id: o.id, title: o.title });
			});
		}
		saveDex();
		D.log(who.label + ' がパーツ図鑑を変更（パーツ ' + nPart + ' 件・見本 ' + aSample.length + ' 件）');
		return { ok: true, parts: nPart, samples: aSample };
	}

	/* ------------------------------------------------------------------ */
	/* 配置案																	 */
	/* ------------------------------------------------------------------ */
	function planSummary(p) {
		return { id: p.id, title: p.title, note: p.note, w: p.w, h: p.h, marks: (p.marks || []).length, by: p.by, at: p.at, updated: p.updated || p.at };
	}

	function findPlan(id) {
		var ps = plans().plans;
		for (var i = 0; i < ps.length; i++) {
			if (ps[i].id === id) {
				return i;
			}
		}
		return -1;
	}

	function opPlanSave(a, who) {
		a = a || {};
		var ps = plans();
		var grid = checkGrid(a, '配置案');
		var w = grid[0].length, h = grid.length;
		var idx = a.id != null ? findPlan(a.id) : -1;
		if (a.id != null && idx < 0) {
			D.fail(404, 'not_found', '配置案 ' + a.id + ' 番はありません（新しく保存する時は id を付けないでください）');
		}
		var now = Date.now();
		var p = {
			id: idx >= 0 ? ps.plans[idx].id : ++ps.seq,
			title: str(a.title, 60).trim() || '配置案',
			note: str(a.note, 2000),
			w: w,
			h: h,
			parts: grid,
			marks: checkMarks(a.marks, w, h),
			by: who.label,
			at: idx >= 0 ? ps.plans[idx].at : now,
			updated: now
		};
		if (idx >= 0) {
			ps.plans[idx] = p;
		} else {
			ps.plans.push(p);
			if (ps.plans.length > MAX_PLANS) {
				ps.plans.splice(0, ps.plans.length - MAX_PLANS);
			}
		}
		D.writeJsonFile(strPlanPath, ps);
		D.log(who.label + ' が配置案 ' + p.id + '「' + p.title + '」を保存（' + w + '×' + h + '）');
		return {
			ok: true,
			id: p.id,
			w: w,
			h: h,
			message: '保存しました（ゲームのデータは変えていません）。管理ページ（/admin）の「配置案」で画像として見られます。'
		};
	}

	function opPlanList() {
		return { plans: plans().plans.slice().reverse().map(planSummary) };
	}

	function opPlanGet(a) {
		var idx = findPlan(a && a.id);
		if (idx < 0) {
			D.fail(404, 'not_found', '配置案 ' + (a && a.id) + ' 番はありません');
		}
		return { plan: plans().plans[idx] };
	}

	function opPlanDelete(a, who) {
		var idx = findPlan(a && a.id);
		if (idx < 0) {
			D.fail(404, 'not_found', '配置案 ' + (a && a.id) + ' 番はありません');
		}
		var ps = plans();
		var p = ps.plans.splice(idx, 1)[0];
		D.writeJsonFile(strPlanPath, ps);
		D.log(who.label + ' が配置案 ' + p.id + '「' + p.title + '」を削除');
		return { ok: true };
	}

	/* ------------------------------------------------------------------ */
	/* 登録																		 */
	/* ------------------------------------------------------------------ */
	var GRID_SCHEMA = {
		type: 'array', minItems: 1, maxItems: MAX_SIDE,
		description: 'マス目。parts[行][列] に mapParts の番号（0 は何も無いマス）。左上が (0,0)、最大 ' + MAX_SIDE + '×' + MAX_SIDE + '。',
		items: { type: 'array', maxItems: MAX_SIDE, items: { type: 'integer', minimum: 0 } }
	};

	D.addTool({
		name: 'sbo_parts_dex', write: false,
		description: 'パーツ図鑑。マップパーツ（マス目の番号）ごとの名前・タグ・説明と、データから分かること（通れるか・上を歩くと消えるか・' +
			'全マップで使われた回数）を返す。ids を渡すか detail=true にすると、よく使われているマップと、よく上下左右に来るパーツも付く。' +
			'新しいマップを考える時は、まずタグ（床・壁・屋根・水・木など）で引いて、隣り合わせの組み合わせを確かめる。',
		inputSchema: {
			type: 'object',
			properties: {
				q: { type: 'string', description: '名前・説明・タグに含まれる文字か、番号' },
				tag: { type: 'string', description: 'このタグが付いているものだけ' },
				ids: { type: 'array', items: { type: 'integer', minimum: 0 }, maxItems: 200, description: 'この番号だけ（詳しい情報つき）' },
				named: { type: 'boolean', description: 'true=名前があるものだけ・false=まだ名前が無いものだけ' },
				passable: { type: 'boolean', description: 'true=通れるものだけ・false=ぶつかるものだけ' },
				used: { type: 'boolean', description: 'true=どこかのマップで使われているものだけ' },
				detail: { type: 'boolean', description: 'よく使われているマップ・よく隣に来るパーツも付ける（重くなる）' },
				offset: { type: 'integer', minimum: 0 },
				limit: { type: 'integer', minimum: 1, maximum: 500, description: '既定 100' }
			}
		},
		run: opDex
	});
	D.addTool({
		name: 'sbo_parts_samples', write: false,
		description: 'パーツの組み合わせの見本（壁で囲った家・道と草地の境目など）。マス目は配置案と同じ形。配置を考える時のお手本にする。',
		inputSchema: {
			type: 'object',
			properties: {
				q: { type: 'string', description: '題名・説明・タグに含まれる文字' },
				id: { type: 'integer' },
				grid: { type: 'boolean', description: 'false にするとマス目を省く（一覧だけ見たい時）' }
			}
		},
		run: opSamples
	});
	D.addTool({
		name: 'sbo_parts_dex_update', write: true,
		description: 'パーツ図鑑の名前・タグ・説明と、組み合わせの見本を変える（ゲームのデータではないので下見なしですぐ保存）。' +
			'parts は変えるパーツだけ。見本は id 無しで追加、id 付きで置き換え、{id, delete:true} で削除。',
		inputSchema: {
			type: 'object',
			properties: {
				parts: {
					type: 'array', maxItems: 500,
					items: {
						type: 'object', required: ['id'],
						properties: {
							id: { type: 'integer', minimum: 0 },
							name: { type: 'string', description: '短い名前（例: 石畳の床・木の壁（左上の角））' },
							tags: { type: 'array', items: { type: 'string' }, description: '例: 床・壁・屋根・水・木・家具・角・端' },
							note: { type: 'string', description: '使い方のメモ（例: 下に 1100 番の影と組み合わせる）' }
						}
					}
				},
				samples: {
					type: 'array', maxItems: 50,
					items: {
						type: 'object',
						properties: {
							id: { type: 'integer' },
							title: { type: 'string' },
							note: { type: 'string', description: 'こう使います、の説明' },
							tags: { type: 'array', items: { type: 'string' } },
							parts: GRID_SCHEMA,
							'delete': { type: 'boolean' }
						}
					}
				}
			}
		},
		run: opDexUpdate
	});
	D.addTool({
		name: 'sbo_plan_save', write: false,
		description: 'マップの配置案を保存する（ゲームのデータは変えない。読むだけのキーでも使える）。管理ページの「配置案」で画像として見られ、' +
			'管理者がそれを見ながら管理者ウィンドウで実際に置く。marks で「ここに宿屋の主人」のような書き込みを重ねられる。id を付けると上書き。',
		inputSchema: {
			type: 'object', required: ['parts'],
			properties: {
				id: { type: 'integer', description: '上書きする配置案の番号（新しく保存する時は省く）' },
				title: { type: 'string', description: '例: 港町の市場（案1）' },
				note: { type: 'string', description: '考えたこと・使ったパーツ・置く予定の NPC など' },
				parts: GRID_SCHEMA,
				marks: {
					type: 'array', maxItems: 200,
					items: {
						type: 'object', required: ['x', 'y'],
						properties: {
							x: { type: 'integer', minimum: 0 }, y: { type: 'integer', minimum: 0 },
							w: { type: 'integer', minimum: 1 }, h: { type: 'integer', minimum: 1 },
							label: { type: 'string', description: '絵に重ねる短い文字（例: A・宿・入口）' },
							text: { type: 'string', description: '説明（一覧に出る）' }
						}
					}
				}
			}
		},
		run: opPlanSave
	});
	D.addTool({
		name: 'sbo_plan_list', write: false,
		description: '保存してある配置案の一覧（新しい順。マス目は省く）。',
		inputSchema: { type: 'object', properties: {} },
		run: opPlanList
	});
	D.addTool({
		name: 'sbo_plan_get', write: false,
		description: '配置案 1 件をマス目つきで返す。',
		inputSchema: { type: 'object', required: ['id'], properties: { id: { type: 'integer' } } },
		run: opPlanGet
	});
	D.addTool({
		name: 'sbo_plan_delete', write: false,
		description: '配置案を消す（ゲームのデータは変えない）。',
		inputSchema: { type: 'object', required: ['id'], properties: { id: { type: 'integer' } } },
		run: opPlanDelete
	});

	D.addApiData('/api/data/parts/dex', opDex);
	D.addApiData('/api/data/parts/samples', opSamples);
	D.addApiData('/api/data/parts/update', opDexUpdate);
	D.addApiData('/api/data/plans/save', opPlanSave);
	D.addApiData('/api/data/plans/list', opPlanList);
	D.addApiData('/api/data/plans/get', opPlanGet);
	D.addApiData('/api/data/plans/delete', opPlanDelete);

	/* 管理ページ（パーツ図鑑・配置案のタブ） */
	D.addApiAdmin('/api/admin/parts/dex', function (body) { D.adminWho(body); return opDex(body); });
	D.addApiAdmin('/api/admin/parts/samples', function (body) { D.adminWho(body); return opSamples(body); });
	D.addApiAdmin('/api/admin/parts/update', function (body) { return opDexUpdate(body, D.adminWho(body)); });
	D.addApiAdmin('/api/admin/plans/save', function (body) { return opPlanSave(body, D.adminWho(body)); });
	D.addApiAdmin('/api/admin/plans/list', function (body) { D.adminWho(body); return opPlanList(body); });
	D.addApiAdmin('/api/admin/plans/get', function (body) { D.adminWho(body); return opPlanGet(body); });
	D.addApiAdmin('/api/admin/plans/delete', function (body) { return opPlanDelete(body, D.adminWho(body)); });

	return {};
})();
