/* ========================================================================= */
/* admin-design.js															 */
/* 管理ページ（/admin）の「パーツ図鑑」と「配置案」								 */
/* ========================================================================= */
/*
 * パーツ図鑑 … マップパーツごとに 名前・タグ・説明 を付ける（AI が新しいマップを考える時の辞書）。
 *              データから分かること（通れるか・使われた回数・よく使われているマップ・よく隣に来るパーツ）も見られる。
 *              組み合わせの見本（マップから切り取る か 番号で書く）も作れる。
 * 配置案     … AI が sbo_plan_save で保存した配置を、ゲームのデータを変えずに画像で見る。
 *              パーツ番号・マス目の線・書き込み（marks）を重ねられ、PNG と番号の表（TSV）で持ち出せる。
 * サーバーは server/node/sbosvr_mapdesign.js の /api/admin/parts/*・/api/admin/plans/*。
 * パーツの絵は admin-mapdoc.js の loadSheets（/admin-grp/ の map01〜05.png）で読む。
 */
(function (root) {
	'use strict';

	var T = 16;
	var HIDE_MAIN = { 3: true, 4: true, 8: true, 9: true, 18: true };

	var api = null;				/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var sheets = null;			/* {map:[canvas×5]} */
	var parts = [];				/* mapParts の一覧（id 順） */
	var mapNames = [];
	var cacheBase = {}, cacheTop = {};
	var loading = null;

	function $(id) {
		return document.getElementById(id);
	}
	function el(tag, cls, text) {
		var e = document.createElement(tag);
		if (cls) e.className = cls;
		if (text != null) e.textContent = text;
		return e;
	}
	function setMsg(e, text, bOk) {
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}
	function pad3(n) {
		return ('00' + n).slice(-3);
	}
	function errText(r) {
		return r.message || r.error || ('HTTP ' + r.httpStatus);
	}
	function dateText(ms) {
		return ms ? new Date(ms).toLocaleString('ja-JP', { month: 'numeric', day: 'numeric', hour: '2-digit', minute: '2-digit' }) : '';
	}

	/* ---------------------------------------------------------------- */
	/* 共通: パーツの絵・マップの一覧										 */
	/* ---------------------------------------------------------------- */
	function listAll(kind, fields) {
		var out = [];
		function next(offset) {
			return api('/api/admin/data/list', { kind: kind, offset: offset, limit: 500, fields: fields }).then(function (r) {
				if (!r.records) throw new Error(errText(r));
				out = out.concat(r.records);
				return r.more ? next(offset + 500) : out;
			});
		}
		return next(0);
	}

	function ensureLoaded() {
		if (loading) return loading;
		if (!root.SboMapDoc || !root.SboMapDoc.loadSheets) {
			return Promise.reject(new Error('admin-mapdoc.js が読めませんでした'));
		}
		loading = root.SboMapDoc.loadSheets('/admin-grp/').then(function (s) {
			sheets = s;
			return listAll('mapParts');
		}).then(function (a) {
			a.forEach(function (p) { parts[p.id] = p; });
			return listAll('map', ['szName']);
		}).then(function (a) {
			a.forEach(function (m) { mapNames[m.id] = m.szName || ''; });
		});
		loading.catch(function () { loading = null; });
		return loading;
	}

	/* パーツ 1 つの 16×16 の絵。bTop=true なら屋根も描く（図鑑用）。false ならゲームで屋根を外した時の見た目 */
	function partImage(no, bTop) {
		var cache = bTop ? cacheTop : cacheBase;
		if (cache[no]) return cache[no];
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
			if (!sheets || s >= sheets.map.length) return;
			ctx.globalAlpha = alpha;
			ctx.drawImage(sheets.map[s], (k % 16) * T, Math.floor(k / 16) * T, T, T, 0, 0, T, T);
			ctx.globalAlpha = 1;
		}
		put(b ? b : g, 1);
		if (bTop || !HIDE_MAIN[p.byHitType]) {
			put(g, lv ? lv / 100 : 1);
		}
		cache[no] = cv;
		return cv;
	}

	function partCanvas(no, px) {
		var cv = document.createElement('canvas');
		cv.width = cv.height = px;
		cv.className = 'pd-img';
		var ctx = cv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.drawImage(partImage(no, true), 0, 0, px, px);
		return cv;
	}

	/* マス目を描く。opt: {scale, numbers, lines, marks, top} */
	function drawGrid(grid, opt) {
		opt = opt || {};
		var h = grid.length, w = h ? grid[0].length : 0;
		var S = T * (opt.scale || 2);
		var cv = document.createElement('canvas');
		cv.width = Math.max(1, w * S);
		cv.height = Math.max(1, h * S);
		var ctx = cv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		var x, y;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				ctx.drawImage(partImage(grid[y][x] || 0, !!opt.top), x * S, y * S, S, S);
			}
		}
		if (opt.lines) {
			ctx.strokeStyle = 'rgba(255,255,255,0.25)';
			ctx.lineWidth = 1;
			ctx.beginPath();
			for (x = 0; x <= w; x++) { ctx.moveTo(x * S + 0.5, 0); ctx.lineTo(x * S + 0.5, h * S); }
			for (y = 0; y <= h; y++) { ctx.moveTo(0, y * S + 0.5); ctx.lineTo(w * S, y * S + 0.5); }
			ctx.stroke();
		}
		if (opt.numbers) {
			var fs = Math.max(9, Math.floor(S / 3.6));
			ctx.font = 'bold ' + fs + 'px sans-serif';
			ctx.textAlign = 'center';
			ctx.textBaseline = 'middle';
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					var t = String(grid[y][x] || 0);
					var tw = ctx.measureText(t).width;
					ctx.fillStyle = 'rgba(0,0,0,0.6)';
					ctx.fillRect(x * S + S / 2 - tw / 2 - 2, y * S + S / 2 - fs / 2 - 1, tw + 4, fs + 2);
					ctx.fillStyle = '#fff';
					ctx.fillText(t, x * S + S / 2, y * S + S / 2 + 1);
				}
			}
		}
		if (opt.marks && opt.marks.length) {
			opt.marks.forEach(function (m) {
				ctx.strokeStyle = '#f0b84a';
				ctx.lineWidth = 2;
				ctx.strokeRect(m.x * S + 1, m.y * S + 1, (m.w || 1) * S - 2, (m.h || 1) * S - 2);
				if (m.label) {
					var f2 = Math.max(11, Math.floor(S / 2.6));
					ctx.font = 'bold ' + f2 + 'px sans-serif';
					var lw = ctx.measureText(m.label).width;
					ctx.fillStyle = '#f0b84a';
					ctx.fillRect(m.x * S + 1, m.y * S + 1, lw + 6, f2 + 4);
					ctx.fillStyle = '#1d1f24';
					ctx.textAlign = 'left';
					ctx.textBaseline = 'top';
					ctx.fillText(m.label, m.x * S + 4, m.y * S + 3);
				}
			});
		}
		return cv;
	}

	/* マップの一部（最大 128×128）を [行][列] で読む */
	function readTiles(mapId, x, y, w, h) {
		return api('/api/admin/data/tiles', { id: mapId, x: x, y: y, w: w, h: h }).then(function (r) {
			if (!r.parts) throw new Error(errText(r));
			return r;
		});
	}

	function download(blob, name) {
		var a = document.createElement('a');
		a.href = URL.createObjectURL(blob);
		a.download = name;
		document.body.appendChild(a);
		a.click();
		setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 60000);
	}

	function copyText(text, msgEl) {
		var done = function () { setMsg(msgEl, 'コピーしました。', true); };
		if (navigator.clipboard && navigator.clipboard.writeText) {
			navigator.clipboard.writeText(text).then(done, function () { setMsg(msgEl, 'コピーできませんでした。'); });
		} else {
			var ta = document.createElement('textarea');
			ta.value = text;
			document.body.appendChild(ta);
			ta.select();
			try { document.execCommand('copy'); done(); } catch (e) { setMsg(msgEl, 'コピーできませんでした。'); }
			ta.remove();
		}
	}

	function gridTsv(grid) {
		return grid.map(function (row) { return row.join('\t'); }).join('\n');
	}

	/* 「1 2 3」「1,2,3」「タブ区切り」の行をマス目にする */
	function parseGrid(text) {
		var rows = String(text || '').split(/\r?\n/).map(function (l) { return l.trim(); }).filter(function (l) { return l; });
		var grid = rows.map(function (l) {
			return l.split(/[\s,，、]+/).filter(function (v) { return v !== ''; }).map(Number);
		});
		var w = grid.reduce(function (m, r) { return Math.max(m, r.length); }, 0);
		if (!grid.length || !w) throw new Error('番号が入っていません');
		grid.forEach(function (r, y) {
			r.forEach(function (v, x) {
				if (!isFinite(v) || v < 0 || Math.floor(v) !== v) throw new Error((y + 1) + ' 行目の ' + (x + 1) + ' 個目が番号ではありません');
			});
			while (r.length < w) r.push(0);
		});
		return grid;
	}

	/* ---------------------------------------------------------------- */
	/* パーツ図鑑																 */
	/* ---------------------------------------------------------------- */
	var dex = { list: [], byId: {}, tags: [], selected: -1, page: 0 };
	var PAGE = 240;

	function loadDex() {
		return api('/api/admin/parts/dex', { limit: 500, offset: 0 }).then(function first(r) {
			if (!r.parts) throw new Error(errText(r));
			var all = r.parts, total = r.matched;
			dex.tags = r.tags;
			dex.named = r.named;
			function more(offset) {
				if (offset >= total) return Promise.resolve(all);
				return api('/api/admin/parts/dex', { limit: 500, offset: offset }).then(function (r2) {
					if (!r2.parts) throw new Error(errText(r2));
					all = all.concat(r2.parts);
					return more(offset + 500);
				});
			}
			return more(all.length);
		}).then(function (all) {
			dex.list = all;
			dex.byId = {};
			all.forEach(function (e) { dex.byId[e.id] = e; });
		});
	}

	function dexFiltered() {
		var q = $('pd-q').value.trim();
		var f = $('pd-filter').value;
		var tag = $('pd-tag').value;
		return dex.list.filter(function (e) {
			if (q && String(e.id) !== q && e.name.indexOf(q) < 0 && e.note.indexOf(q) < 0 &&
					!e.tags.some(function (t) { return t.indexOf(q) >= 0; })) return false;
			if (tag && e.tags.indexOf(tag) < 0) return false;
			switch (f) {
			case 'named': return !!e.name;
			case 'unnamed': return !e.name;
			case 'used': return e.uses > 0;
			case 'unnamed-used': return !e.name && e.uses > 0;
			case 'unused': return !e.uses;
			case 'pass': return e.passable;
			case 'block': return !e.passable;
			case 'roof': return e.hideOnTop;
			case 'anime': return e.anime > 0;
			}
			return true;
		}).sort(function (a, b) {
			return $('pd-sort').value === 'uses' ? (b.uses - a.uses || a.id - b.id) : a.id - b.id;
		});
	}

	function renderTagSelect() {
		var sel = $('pd-tag'), cur = sel.value;
		sel.textContent = '';
		var o = el('option', null, 'タグ: 全部');
		o.value = '';
		sel.appendChild(o);
		dex.tags.forEach(function (t) {
			var o2 = el('option', null, t.tag + '（' + t.n + '）');
			o2.value = t.tag;
			sel.appendChild(o2);
		});
		sel.value = cur;
	}

	function renderDexGrid() {
		var list = dexFiltered();
		var box = $('pd-grid');
		box.textContent = '';
		var pages = Math.max(1, Math.ceil(list.length / PAGE));
		if (dex.page >= pages) dex.page = pages - 1;
		$('pd-count').textContent = list.length + ' 件（名前あり ' + dex.list.filter(function (e) { return e.name; }).length + ' / 全 ' + dex.list.length + '）';
		$('pd-page').textContent = (dex.page + 1) + ' / ' + pages;
		$('pd-prev').disabled = dex.page <= 0;
		$('pd-next').disabled = dex.page >= pages - 1;
		list.slice(dex.page * PAGE, (dex.page + 1) * PAGE).forEach(function (e) {
			var b = el('button', 'pd-cell' + (e.id === dex.selected ? ' sel' : '') + (e.name ? '' : ' noname'));
			b.type = 'button';
			b.title = e.id + (e.name ? ' ' + e.name : '') + '（' + e.hitName + '・' + e.uses + ' マス）';
			b.appendChild(partCanvas(e.id, 32));
			b.appendChild(el('span', 'pd-no', String(e.id)));
			b.appendChild(el('span', 'pd-name', e.name || '　'));
			b.addEventListener('click', function () { selectPart(e.id); });
			box.appendChild(b);
		});
	}

	function partChip(id, n) {
		var e = dex.byId[id] || { name: '' };
		var b = el('button', 'pd-chip');
		b.type = 'button';
		b.title = id + ' ' + (e.name || '');
		b.appendChild(partCanvas(id, 24));
		b.appendChild(el('span', null, id + (e.name ? ' ' + e.name : '') + (n ? '（' + n + '）' : '')));
		b.addEventListener('click', function () { selectPart(id); });
		return b;
	}

	function selectPart(id) {
		dex.selected = id;
		var box = $('pd-detail');
		box.hidden = false;
		box.textContent = '';
		box.appendChild(el('div', 'sub-text', '読み込み中…'));
		renderDexGrid();
		api('/api/admin/parts/dex', { ids: [id] }).then(function (r) {
			if (!r.parts || !r.parts.length) throw new Error(errText(r));
			var e = r.parts[0];
			dex.byId[id] = Object.assign(dex.byId[id] || {}, e);
			renderDetail(e);
		}).catch(function (err) {
			box.textContent = '';
			box.appendChild(el('div', 'msg', '読めませんでした: ' + (err.message || err)));
		});
	}

	function renderDetail(e) {
		var box = $('pd-detail');
		box.textContent = '';
		var head = el('div', 'pd-head');
		head.appendChild(partCanvas(e.id, 64));
		var p = parts[e.id] || {};
		if (p.anime && p.anime.length > 1) {
			/* アニメするパーツはコマを並べる */
			p.anime.forEach(function (a) {
				var cv = document.createElement('canvas');
				cv.width = cv.height = 32;
				var c = cv.getContext('2d');
				c.imageSmoothingEnabled = false;
				[a.wBackID, a.wGrpID].forEach(function (n, k) {
					if (!n && k === 0) return;
					var s = Math.floor(n / 256), j = n % 256;
					if (sheets && s < sheets.map.length) c.drawImage(sheets.map[s], (j % 16) * T, Math.floor(j / 16) * T, T, T, 0, 0, 32, 32);
				});
				cv.className = 'pd-img';
				head.appendChild(cv);
			});
		}
		var info = el('div');
		info.appendChild(el('div', 'name', e.id + ' 番' + (e.name ? '　' + e.name : '（まだ名前がありません）')));
		info.appendChild(el('div', 'sub-text', (e.passable ? '通れる' : 'ぶつかる') + '・当たり判定「' + e.hitName + '」' +
			(e.hideOnTop ? '・上を歩くと消える' : '') + (e.anime ? '・アニメ ' + e.anime + ' コマ' : '') + '・全マップで ' + e.uses + ' マス'));
		head.appendChild(info);
		box.appendChild(head);

		/* 編集 */
		var lab = function (t, f) { var l = el('label', null, t); l.htmlFor = f; return l; };
		box.appendChild(lab('名前', 'pd-e-name'));
		var iName = el('input');
		iName.type = 'text';
		iName.id = 'pd-e-name';
		iName.maxLength = 40;
		iName.placeholder = '例: 石畳の床・木の壁（左上の角）';
		iName.value = e.name;
		box.appendChild(iName);
		box.appendChild(lab('タグ（、や空白で区切る）', 'pd-e-tags'));
		var iTags = el('input');
		iTags.type = 'text';
		iTags.id = 'pd-e-tags';
		iTags.placeholder = '例: 床、町、石';
		iTags.value = e.tags.join('、');
		box.appendChild(iTags);
		var tagRow = el('div', 'pd-tagpick');
		dex.tags.slice(0, 30).forEach(function (t) {
			var b = el('button', 'sub', t.tag);
			b.type = 'button';
			b.addEventListener('click', function () {
				var cur = iTags.value.split(/[,、，\s]+/).filter(function (v) { return v; });
				if (cur.indexOf(t.tag) < 0) cur.push(t.tag);
				iTags.value = cur.join('、');
			});
			tagRow.appendChild(b);
		});
		box.appendChild(tagRow);
		box.appendChild(lab('使い方のメモ', 'pd-e-note'));
		var iNote = el('textarea');
		iNote.id = 'pd-e-note';
		iNote.maxLength = 300;
		iNote.rows = 2;
		iNote.value = e.note;
		box.appendChild(iNote);
		var msg = el('div', 'msg');
		var bSave = el('button', null, '保存する');
		bSave.type = 'button';
		bSave.addEventListener('click', function () {
			bSave.disabled = true;
			api('/api/admin/parts/update', { parts: [{ id: e.id, name: iName.value, tags: iTags.value, note: iNote.value }] }).then(function (r) {
				if (!r.ok) throw new Error(errText(r));
				var cur = dex.byId[e.id];
				cur.name = iName.value.trim();
				cur.tags = iTags.value.split(/[,、，\s]+/).filter(function (v) { return v; });
				cur.note = iNote.value;
				setMsg(msg, '保存しました。', true);
				return api('/api/admin/parts/dex', { limit: 1 });
			}).then(function (r) {
				if (r && r.tags) {
					dex.tags = r.tags;
					renderTagSelect();
				}
				renderDexGrid();
			}).catch(function (err) {
				setMsg(msg, '保存できませんでした: ' + (err.message || err));
			}).then(function () { bSave.disabled = false; });
		});
		box.appendChild(bSave);
		var bNext = el('button', 'sub', '次の名前の無いパーツ');
		bNext.type = 'button';
		bNext.addEventListener('click', function () {
			var list = dexFiltered().filter(function (x) { return !x.name; });
			var nx = list.filter(function (x) { return x.id > e.id; })[0] || list[0];
			if (nx) selectPart(nx.id);
		});
		box.appendChild(bNext);
		box.appendChild(msg);

		/* よく隣に来るパーツ */
		if (e.neighbors) {
			box.appendChild(el('h3', null, 'よく隣に来るパーツ'));
			[['up', '上'], ['down', '下'], ['left', '左'], ['right', '右']].forEach(function (d) {
				var row = el('div', 'pd-nb');
				row.appendChild(el('span', 'pd-dir', d[1]));
				(e.neighbors[d[0]] || []).forEach(function (n) { row.appendChild(partChip(n.id, n.n)); });
				if (!(e.neighbors[d[0]] || []).length) row.appendChild(el('span', 'sub-text', 'なし'));
				box.appendChild(row);
			});
		}

		/* 使われている例（そのマップで最初に見つけた場所のまわり） */
		if (e.maps && e.maps.length) {
			box.appendChild(el('h3', null, '使われているマップ'));
			var mrow = el('div', 'pd-maps');
			var exBox = el('div', 'pd-example');
			e.maps.forEach(function (m, k) {
				var b = el('button', k === 0 ? '' : 'sub', pad3(m.map) + ' ' + (m.name || '（名前なし）') + '（' + m.n + '）');
				b.type = 'button';
				b.addEventListener('click', function () {
					Array.prototype.forEach.call(mrow.childNodes, function (c) { c.className = 'sub'; });
					b.className = '';
					showExample(exBox, m);
				});
				mrow.appendChild(b);
			});
			box.appendChild(mrow);
			box.appendChild(exBox);
			showExample(exBox, e.maps[0]);
		}
	}

	/* そのパーツのまわり 15×11 マスを描く（真ん中に枠） */
	function showExample(box, m) {
		var W = 15, H = 11;
		var x0 = Math.max(0, m.x - 7), y0 = Math.max(0, m.y - 5);
		box.textContent = '';
		readTiles(m.map, x0, y0, W, H).then(function (r) {
			var cv = drawGrid(r.parts, { scale: 2 });
			var c = cv.getContext('2d');
			c.strokeStyle = '#ff4040';
			c.lineWidth = 2;
			c.strokeRect((m.x - r.x) * 32 + 1, (m.y - r.y) * 32 + 1, 30, 30);
			cv.className = 'pd-map';
			box.appendChild(cv);
			box.appendChild(el('div', 'sub-text', pad3(m.map) + ' ' + (m.name || '') + ' の (' + m.x + ',' + m.y + ') のまわり（屋根は外しています）'));
			var bTake = el('button', 'sub', 'ここを見本にする');
			bTake.type = 'button';
			bTake.addEventListener('click', function () {
				$('ps-map').value = String(m.map);
				$('ps-x').value = r.x;
				$('ps-y').value = r.y;
				$('ps-w').value = r.w;
				$('ps-h').value = r.h;
				openSampleEditor(null, r.parts);
			});
			box.appendChild(bTake);
		}).catch(function (err) {
			box.appendChild(el('div', 'msg', '読めませんでした: ' + (err.message || err)));
		});
	}

	/* ---- 組み合わせの見本 ---- */
	var samples = [], editing = null;

	function loadSamples() {
		return api('/api/admin/parts/samples', {}).then(function (r) {
			if (!r.samples) throw new Error(errText(r));
			samples = r.samples;
			renderSamples();
		});
	}

	function renderSamples() {
		var box = $('ps-list');
		box.textContent = '';
		if (!samples.length) {
			box.appendChild(el('div', 'sub-text', 'まだ見本がありません。上のパーツの「使われているマップ」から切り取るか、下で作れます。'));
			return;
		}
		samples.forEach(function (s) {
			var c = el('div', 'card');
			c.appendChild(el('div', 'name', s.id + '. ' + s.title));
			if (s.tags.length) c.appendChild(el('div', 'sub-text', 'タグ: ' + s.tags.join('、')));
			var cv = drawGrid(s.parts, { scale: 2 });
			cv.className = 'pd-map';
			c.appendChild(cv);
			if (s.note) c.appendChild(el('div', 'pd-note', s.note));
			c.appendChild(el('div', 'sub-text', s.w + '×' + s.h + '・' + (s.by || '') + ' ' + dateText(s.at)));
			var bEdit = el('button', 'sub', '直す');
			bEdit.type = 'button';
			bEdit.addEventListener('click', function () { openSampleEditor(s, s.parts); });
			var bDel = el('button', 'sub', '消す');
			bDel.type = 'button';
			bDel.addEventListener('click', function () {
				if (!confirm('見本「' + s.title + '」を消しますか？')) return;
				api('/api/admin/parts/update', { samples: [{ id: s.id, 'delete': true }] }).then(function (r) {
					if (!r.ok) throw new Error(errText(r));
					return loadSamples();
				}).catch(function (err) { alert('消せませんでした: ' + (err.message || err)); });
			});
			c.appendChild(bEdit);
			c.appendChild(bDel);
			box.appendChild(c);
		});
	}

	function openSampleEditor(s, grid) {
		editing = s;
		$('ps-editor').hidden = false;
		$('ps-title').value = s ? s.title : '';
		$('ps-note').value = s ? s.note : '';
		$('ps-tags').value = s ? s.tags.join('、') : '';
		$('ps-grid').value = grid ? gridTsv(grid) : '';
		$('ps-save').textContent = s ? '見本 ' + s.id + ' を上書きする' : '見本として保存する';
		previewSample();
		$('ps-editor').scrollIntoView({ behavior: 'smooth', block: 'start' });
	}

	function previewSample() {
		var box = $('ps-preview');
		box.textContent = '';
		setMsg($('ps-msg'), '');
		if (!$('ps-grid').value.trim()) return;
		try {
			var cv = drawGrid(parseGrid($('ps-grid').value), { scale: 2 });
			cv.className = 'pd-map';
			box.appendChild(cv);
		} catch (err) {
			setMsg($('ps-msg'), err.message);
		}
	}

	/* ---------------------------------------------------------------- */
	/* 配置案																	 */
	/* ---------------------------------------------------------------- */
	var plans = [], plan = null;

	function loadPlans() {
		return api('/api/admin/plans/list', {}).then(function (r) {
			if (!r.plans) throw new Error(errText(r));
			plans = r.plans;
			renderPlanList();
		});
	}

	function renderPlanList() {
		var box = $('pl-list');
		box.textContent = '';
		if (!plans.length) {
			box.appendChild(el('div', 'sub-text', 'まだ配置案がありません。AI に「◯◯な街の配置案を作って sbo_plan_save で保存して」と頼むと、ここに出ます。'));
			return;
		}
		plans.forEach(function (p) {
			var b = el('button', 'pl-item' + (plan && plan.id === p.id ? ' sel' : ''));
			b.type = 'button';
			b.appendChild(el('span', 'name', p.id + '. ' + p.title));
			b.appendChild(el('span', 'sub-text', p.w + '×' + p.h + '・' + (p.by || '') + '・' + dateText(p.updated)));
			b.addEventListener('click', function () { openPlan(p.id); });
			box.appendChild(b);
		});
	}

	function openPlan(id) {
		setMsg($('pl-msg'), '');
		api('/api/admin/plans/get', { id: id }).then(function (r) {
			if (!r.plan) throw new Error(errText(r));
			plan = r.plan;
			renderPlanList();
			renderPlan();
		}).catch(function (err) {
			setMsg($('pl-msg'), '読めませんでした: ' + (err.message || err));
		});
	}

	function planCanvas() {
		var bNum = $('pl-num').checked;
		return drawGrid(plan.parts, {
			scale: bNum ? 3 : 2,
			numbers: bNum,
			lines: $('pl-lines').checked,
			marks: $('pl-marks').checked ? plan.marks : null,
			top: $('pl-roof').checked
		});
	}

	function renderPlan() {
		var box = $('pl-view');
		box.hidden = !plan;
		if (!plan) return;
		$('pl-title').textContent = plan.id + '. ' + plan.title;
		$('pl-sub').textContent = plan.w + '×' + plan.h + ' マス・' + (plan.by || '') + '・保存 ' + dateText(plan.updated);
		$('pl-note').textContent = plan.note || '';
		$('pl-note').hidden = !plan.note;
		var wrap = $('pl-canvas');
		wrap.textContent = '';
		var cv = planCanvas();
		cv.className = 'pd-map';
		wrap.appendChild(cv);
		var ml = $('pl-marklist');
		ml.textContent = '';
		(plan.marks || []).forEach(function (m) {
			if (!m.label && !m.text) return;
			ml.appendChild(el('li', null, (m.label ? '【' + m.label + '】' : '') + '(' + m.x + ',' + m.y + ')' +
				((m.w || 1) > 1 || (m.h || 1) > 1 ? ' ' + m.w + '×' + m.h : '') + (m.text ? ' ' + m.text : '')));
		});
		/* 使っているパーツ（多い順） */
		var cnt = {};
		plan.parts.forEach(function (row) { row.forEach(function (v) { cnt[v] = (cnt[v] || 0) + 1; }); });
		var used = $('pl-used');
		used.textContent = '';
		Object.keys(cnt).map(Number).sort(function (a, b) { return cnt[b] - cnt[a] || a - b; }).forEach(function (id) {
			used.appendChild(partChip(id, cnt[id]));
		});
	}

	/* ---------------------------------------------------------------- */
	/* 入口																		 */
	/* ---------------------------------------------------------------- */
	var bDexInit = false, bPlanInit = false;

	function openDex(fnApi) {
		api = fnApi;
		if (bDexInit) return;
		bDexInit = true;
		setMsg($('pd-msg'), '読み込み中…（パーツの使われ方を数えるので、最初は数秒かかります）', true);
		ensureLoaded().then(loadDex).then(function () {
			setMsg($('pd-msg'), '');
			renderTagSelect();
			renderDexGrid();
			var sel = $('ps-map');
			mapNames.forEach(function (n, id) {
				var o = el('option', null, pad3(id) + ' ' + (n || '（名前なし）'));
				o.value = String(id);
				sel.appendChild(o);
			});
			return loadSamples();
		}).catch(function (err) {
			bDexInit = false;
			setMsg($('pd-msg'), '読めませんでした: ' + (err.message || err));
		});

		['pd-q', 'pd-filter', 'pd-tag', 'pd-sort'].forEach(function (id) {
			$(id).addEventListener(id === 'pd-q' ? 'input' : 'change', function () { dex.page = 0; renderDexGrid(); });
		});
		$('pd-prev').addEventListener('click', function () { dex.page--; renderDexGrid(); });
		$('pd-next').addEventListener('click', function () { dex.page++; renderDexGrid(); });
		$('pd-reload').addEventListener('click', function () {
			loadDex().then(function () { renderTagSelect(); renderDexGrid(); });
		});

		$('ps-new').addEventListener('click', function () { openSampleEditor(null, null); });
		$('ps-cut').addEventListener('click', function () {
			var n = function (id) { return Math.max(0, Math.floor(Number($(id).value) || 0)); };
			readTiles(Number($('ps-map').value), n('ps-x'), n('ps-y'), Math.max(1, Math.min(32, n('ps-w'))), Math.max(1, Math.min(32, n('ps-h')))).then(function (r) {
				$('ps-grid').value = gridTsv(r.parts);
				previewSample();
			}).catch(function (err) { setMsg($('ps-msg'), '読めませんでした: ' + (err.message || err)); });
		});
		$('ps-grid').addEventListener('input', previewSample);
		$('ps-cancel').addEventListener('click', function () { $('ps-editor').hidden = true; editing = null; });
		$('ps-save').addEventListener('click', function () {
			var grid;
			try {
				grid = parseGrid($('ps-grid').value);
			} catch (err) {
				setMsg($('ps-msg'), err.message);
				return;
			}
			var s = { title: $('ps-title').value, note: $('ps-note').value, tags: $('ps-tags').value, parts: grid };
			if (editing) s.id = editing.id;
			api('/api/admin/parts/update', { samples: [s] }).then(function (r) {
				if (!r.ok) throw new Error(errText(r));
				$('ps-editor').hidden = true;
				editing = null;
				return loadSamples();
			}).catch(function (err) { setMsg($('ps-msg'), '保存できませんでした: ' + (err.message || err)); });
		});
	}

	function openPlans(fnApi) {
		api = fnApi;
		if (bPlanInit) {
			loadPlans().catch(function () {});
			return;
		}
		bPlanInit = true;
		setMsg($('pl-msg'), '読み込み中…', true);
		ensureLoaded().then(loadPlans).then(function () {
			setMsg($('pl-msg'), '');
		}).catch(function (err) {
			bPlanInit = false;
			setMsg($('pl-msg'), '読めませんでした: ' + (err.message || err));
		});
		['pl-num', 'pl-lines', 'pl-marks', 'pl-roof'].forEach(function (id) {
			$(id).addEventListener('change', renderPlan);
		});
		$('pl-reload').addEventListener('click', function () { loadPlans().catch(function () {}); });
		$('pl-png').addEventListener('click', function () {
			if (!plan) return;
			planCanvas().toBlob(function (b) { download(b, 'sbo2003-plan-' + plan.id + '.png'); }, 'image/png');
		});
		$('pl-tsv').addEventListener('click', function () {
			if (plan) copyText(gridTsv(plan.parts), $('pl-msg'));
		});
		$('pl-del').addEventListener('click', function () {
			if (!plan || !confirm('配置案「' + plan.title + '」を消しますか？（ゲームのデータは変わりません）')) return;
			api('/api/admin/plans/delete', { id: plan.id }).then(function (r) {
				if (!r.ok) throw new Error(errText(r));
				plan = null;
				renderPlan();
				return loadPlans();
			}).catch(function (err) { setMsg($('pl-msg'), '消せませんでした: ' + (err.message || err)); });
		});
	}

	root.SboDesign = { openDex: openDex, openPlans: openPlans, parseGrid: parseGrid };
})(this);
