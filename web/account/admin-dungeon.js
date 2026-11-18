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
	var selRoom = -1, selDeco = -1, selOp = -1;
	var last = null, lastTheme = null;					/* 最後のお試し作成の結果 */
	var $root = null;
	var dex = null, dexLoading = null, palBuilt = false, palCb = null;	/* パーツ一覧のポップアップ */

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
		'.dg-pf{display:flex;flex-wrap:wrap;gap:4px;align-items:flex-start}' +
		'.dg-chipw{position:relative;display:inline-block}' +
		'.dg-chip{display:flex!important;flex-direction:column;align-items:center;margin:0!important;padding:2px!important;min-width:40px;background:#000;border:2px solid var(--border);border-radius:4px;color:#ddd;font-size:11px;cursor:pointer}' +
		'.dg-chip canvas{width:32px;height:32px;image-rendering:pixelated;display:block}' +
		'.dg-chip:hover{border-color:var(--accent)}' +
		'.dg-chipx{position:absolute;top:-6px;right:-6px;margin:0!important;padding:0!important;width:20px;height:20px;line-height:18px;border-radius:50%;font-size:13px;text-align:center}' +
		'.dg-pf .dg-add{margin:0!important;align-self:center}' +
		'.dg-sm{width:4em!important;padding:3px 5px!important;font-size:12px!important;margin-left:4px}' +
		'.dg-dgrid{display:grid;gap:2px;width:max-content}' +
		'.dg-palbg{position:fixed;inset:0;z-index:60;background:rgba(0,0,0,.6);display:flex;align-items:center;justify-content:center}' +
		'.dg-palbg[hidden]{display:none}' +
		'.dg-palbg .mv-pal{position:relative;right:auto;top:auto;width:min(560px,calc(100vw - 16px));height:min(600px,calc(100vh - 16px));resize:none}' +
		'.dg-palbg .mv-pal-head{cursor:default}' +
		'.dg-palbg .mv-pal-tools button{margin:0;padding:6px 10px}' +
		'.dg-guide{margin:14px 0;padding:14px;border:1px dashed var(--border);border-radius:8px;color:var(--sub)}' +
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
			'<div class="dg-guide" id="dg-guide" hidden>左の一覧でテーマを選ぶか、「追加」で新しいテーマを作ってください。テーマを選ぶと、パーツ・9分割の部屋・出入口・灯り・飾りの欄が出ます。</div>' +
			'<div id="dg-edit" hidden>' +
			'<div class="dg-row"><label>名前 <input type="text" id="dg-name" maxlength="40"></label>' +
			'<label>作り方 <select id="dg-algo"><option value="1">1 = L1 聖堂</option><option value="3">3 = L3 洞窟</option></select></label></div>' +
			'<div class="dg-cap">絵を押すとパーツ一覧から選べます。複数可の欄は「＋追加」と絵の×で増減（床はくじで均等に混ぜます）。数字の欄は上級者用です。</div><div class="dg-parts" id="dg-lists"></div></div></div>';
		h += '<div class="dg-sec" id="dg-rooms-sec" hidden><h3>9分割の部屋（L1 用）</h3>' +
			'<div class="dg-row"><label>大きさ w <input type="text" class="dg-n" id="dg-rw" inputmode="numeric" value="9"></label>' +
			'<label>h <input type="text" class="dg-n" id="dg-rh" inputmode="numeric" value="8"></label>' +
			'<button type="button" id="dg-radd">＋部屋を追加</button></div>' +
			'<div class="dg-row"><select id="dg-rsel"></select><button type="button" id="dg-rdup" class="sub">この部屋を複製</button><button type="button" id="dg-rdel" class="sub">この部屋を削除</button></div>' +
			'<div id="dg-redit" hidden><div class="dg-row"><label>名前 <input type="text" id="dg-rname" maxlength="40"></label>' +
			'<label>種類 <select id="dg-rkind"><option value="0">ふつう</option><option value="1">ボス</option><option value="2">入口</option></select></label>' +
			'<span class="sub-text" id="dg-rinfo"></span></div>' +
			'<div class="dg-row"><label><input type="checkbox" id="dg-rfixed"> 伸ばさずそのまま置く（固定パターン）</label>' +
			'<button type="button" class="sub" id="dg-rentclr">入口の位置を消す</button><span class="sub-text" id="dg-rentinfo"></span></div>' +
			'<div class="dg-row" id="dg-tb-r"></div><div class="dg-row" id="dg-rb-r"></div>' +
			'<div class="dg-cap" id="dg-rcap"></div>' +
			'<div class="dg-cols"><div><div class="dg-cap">部屋の絵</div><div class="dg-scroll"><canvas class="dg-cv" id="dg-rcv"></canvas></div></div>' +
			'<div id="dg-samples" class="dg-cols"><div><div class="dg-cap" id="dg-s1c"></div><div class="dg-scroll"><canvas class="dg-cv" id="dg-s1"></canvas></div></div>' +
			'<div><div class="dg-cap" id="dg-s2c"></div><div class="dg-scroll"><canvas class="dg-cv" id="dg-s2"></canvas></div></div></div></div></div></div>';
		h += '<div class="dg-sec" id="dg-ops-sec" hidden><h3>出入口パターン（最大50件）</h3>' +
			'<div class="dg-row"><label>壁の向き <select id="dg-oidir"><option value="0">0 = 横の壁（h は 2）</option><option value="1">1 = 縦の壁（w は 1）</option></select></label>' +
			'<label>種類 <select id="dg-oitype"><option value="1">1 = アーチ</option><option value="2">2 = 扉</option><option value="3">3 = 壁ごと抜く</option></select></label>' +
			'<label>大きさ w <input type="text" class="dg-n" id="dg-ow" inputmode="numeric" value="4"></label>' +
			'<label>h <input type="text" class="dg-n" id="dg-oh" inputmode="numeric" value="2"></label>' +
			'<button type="button" id="dg-oadd">＋パターンを追加</button></div>' +
			'<div class="dg-row"><select id="dg-osel"></select><button type="button" id="dg-odup" class="sub">このパターンを複製</button><button type="button" id="dg-odel" class="sub">このパターンを削除</button></div>' +
			'<div id="dg-oedit" hidden><div class="dg-row"><label>名前 <input type="text" id="dg-oname" maxlength="40"></label>' +
			'<label>壁の向き <select id="dg-odir"><option value="0">0 = 横の壁</option><option value="1">1 = 縦の壁</option></select></label>' +
			'<label>種類 <select id="dg-otype"><option value="1">1 = アーチ</option><option value="2">2 = 扉</option><option value="3">3 = 壁ごと抜く</option></select></label>' +
			'<span class="sub-text" id="dg-oinfo"></span></div>' +
			'<div class="dg-row" id="dg-oopenrow" hidden><button type="button" id="dg-oaddopen">開いた絵を作る（閉まった絵をコピー）</button>' +
			'<button type="button" class="sub" id="dg-oclropen">開いた絵を消す</button></div>' +
			'<div class="dg-row" id="dg-tb-o"></div><div class="dg-row" id="dg-rb-o"></div>' +
			'<div class="dg-cap" id="dg-ocap"></div>' +
			'<div class="dg-cols"><div><div class="dg-cap">閉まった絵（通常の絵）</div><div class="dg-scroll"><canvas class="dg-cv" id="dg-ocv"></canvas></div></div>' +
			'<div id="dg-oopenbox"><div class="dg-cap">開いた絵</div><div class="dg-scroll"><canvas class="dg-cv" id="dg-ocv2"></canvas></div></div></div></div></div>';
		h += '<div class="dg-sec" id="dg-lights-sec" hidden><h3>灯りの上書き表（最大8件）</h3>' +
			'<div class="dg-cap">パーツ番号ごとの「灯りの範囲」（0〜7）を、このテーマだけ上書きします。お試し作成の「暗くして見る」では、パーツ本来の値より先にこの表を使います。</div>' +
			'<div id="dg-lrows"></div><div class="dg-row"><button type="button" id="dg-ladd">行を足す</button></div></div>';
		h += '<div class="dg-sec" id="dg-decos-sec" hidden><h3>飾り</h3>' +
			'<div class="dg-row"><select id="dg-dsel"></select><button type="button" id="dg-dadd">＋飾りを追加</button><button type="button" id="dg-ddup" class="sub">複製</button><button type="button" id="dg-ddel" class="sub">削除</button></div>' +
			'<div id="dg-dedit" hidden><div class="dg-row"><label>名前 <input type="text" id="dg-dname" maxlength="40"></label>' +
			'<label>w <input type="text" class="dg-n" id="dg-dw" inputmode="numeric"></label><label>h <input type="text" class="dg-n" id="dg-dh" inputmode="numeric"></label>' +
			'<label>置く所 <select id="dg-don"><option value="1">部屋の床</option><option value="3">壁の側面</option></select></label>' +
			'<label>rate % <input type="text" class="dg-n" id="dg-drate" inputmode="numeric"></label><label>max <input type="text" class="dg-n" id="dg-dmax" inputmode="numeric"></label></div>' +
			'<div class="dg-row" id="dg-tb-d"></div><div class="dg-row" id="dg-rb-d"></div>' +
			'<div class="dg-scroll" style="max-height:60vh"><canvas class="dg-cv" id="dg-dcv"></canvas></div></div></div>';
		h += '<div class="dg-sec"><h3>お試し作成</h3><div class="dg-row" id="dg-params"></div>' +
			'<div class="dg-row"><label><input type="checkbox" id="dg-usetheme" checked> 編集中のテーマを使う</label>' +
			'<button type="button" id="dg-gen">作る</button><button type="button" id="dg-gen2" class="sub">別のシード</button>' +
			'<label>表示 <select id="dg-view"><option value="parts">パーツの絵</option><option value="meaning">意味の色</option></select></label>' +
			'<label>暗さ <select id="dg-dark"><option value="0">なし</option><option value="25">25</option><option value="50">50</option><option value="75">75</option><option value="100">100</option></select></label>' +
			'<label><input type="checkbox" id="dg-dchar"> 入口に人（灯り）</label>' +
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
			ls += '<span>' + l[1] + '</span><input type="text" class="dg-sm" id="dg-l-' + l[0] + '" inputmode="numeric" autocomplete="off" style="width:min(100%,9em)!important"><span class="dg-pf" id="dg-t-' + l[0] + '"></span>';
		});
		$('dg-lists').innerHTML = ls;
		var pg = document.createElement('div');
		pg.className = 'dg-palbg';
		pg.id = 'dg-pal';
		pg.hidden = true;
		pg.innerHTML = '<div class="mv-pal"><div class="mv-pal-head"><span id="dg-pal-title">パーツ一覧</span><button type="button" class="sub" id="dg-pal-close">閉じる</button></div>' +
			'<div class="mv-pal-tools"><input type="search" id="dg-pal-q" placeholder="番号・名前・タグで探す"><button type="button" class="sub" id="dg-pal-zero">0（なし）にする</button></div>' +
			'<div class="mv-pal-grid" id="dg-pal-grid"></div><div class="mv-pal-info" id="dg-pal-info"></div></div>';
		$root.appendChild(pg);
	}

	/* ---------------------------------------------------------------- */
	/* 絵																		 */
	/* ---------------------------------------------------------------- */
	function listAll(kind, extra) {
		var out = [];
		function next(offset) {
			var body = { kind: kind, offset: offset, limit: 500 };
			if (extra) for (var k in extra) body[k] = extra[k];
			return api('/api/admin/data/list', body).then(function (r) {
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
	/* パーツ一覧のポップアップ（絵の並び・検索・選択の見せ方は「マップ確認」の編集モードのパーツ一覧と同じ。class も共用）	 */
	/* ---------------------------------------------------------------- */
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
			filterPal();
		}, function () { dexLoading = null; });
		return dexLoading;
	}
	function partLabel(no) {
		var d = dex && dex[no];
		return 'パーツ ' + no + (d && d.name ? '「' + d.name + '」' : '') + (d && d.tags && d.tags.length ? '・' + d.tags.join(' ') : '');
	}
	function filterPal() {
		if (!palBuilt) return;
		var q = $('dg-pal-q').value.trim();
		Array.prototype.forEach.call($('dg-pal-grid').children, function (b) {
			var no = Number(b.dataset.no), d = dex && dex[no], bShow = true;
			if (q) bShow = String(no) === q || (!!d && (d.name.indexOf(q) >= 0 || d.tags.some(function (t) { return t.indexOf(q) >= 0; })));
			b.style.display = bShow ? '' : 'none';
		});
	}
	/* 一覧のスクロール位置は閉じる時に覚えておき、次に開いた時に戻す（ブラウザを閉じても残す） */
	var PAL_SCROLL_KEY = 'sbo_dungeon_pal_scroll';
	var palScroll = (function () { try { return Number(localStorage.getItem(PAL_SCROLL_KEY)) || 0; } catch (e) { return 0; } })();
	function savePalScroll() {
		palScroll = $('dg-pal-grid').scrollTop;
		try { localStorage.setItem(PAL_SCROLL_KEY, String(palScroll)); } catch (e) { /* 保存できなくても動く */ }
	}
	function closePal() { if (!$('dg-pal').hidden) savePalScroll(); $('dg-pal').hidden = true; palCb = null; }
	function pickNo(no) { var f = palCb; closePal(); if (f) f(no); }
	function buildPal() {
		if (palBuilt) return;
		palBuilt = true;
		var grid = $('dg-pal-grid');
		for (var i = 0; i < parts.length; i++) {
			if (!parts[i]) continue;
			var b = document.createElement('button'), cv = document.createElement('canvas');
			b.type = 'button';
			b.className = 'mv-pal-cell';
			b.dataset.no = String(i);
			b.title = String(i);
			cv.width = cv.height = T;
			var im = pimg(i);
			if (im) cv.getContext('2d').drawImage(im, 0, 0, T, T, 0, 0, T, T);
			b.appendChild(cv);
			grid.appendChild(b);
		}
		grid.addEventListener('click', function (ev) {
			var b = ev.target.closest && ev.target.closest('.mv-pal-cell');
			if (b) pickNo(Number(b.dataset.no));
		});
		grid.addEventListener('mouseover', function (ev) {
			var b = ev.target.closest && ev.target.closest('.mv-pal-cell');
			if (b) $('dg-pal-info').textContent = partLabel(Number(b.dataset.no));
		});
		$('dg-pal-q').addEventListener('input', filterPal);
		$('dg-pal-close').addEventListener('click', closePal);
		$('dg-pal-zero').addEventListener('click', function () { pickNo(0); });
		$('dg-pal').addEventListener('click', function (ev) { if (ev.target === $('dg-pal')) closePal(); });
		document.addEventListener('keydown', function (ev) { if (ev.key === 'Escape' && !$('dg-pal').hidden) closePal(); });
	}
	/* 一覧を開いて 1 つ選ばせる（cb(番号)。cur は今の番号） */
	function openPal(title, curNo, cb) {
		ensureBase().then(function () {
			buildPal();
			palCb = cb;
			$('dg-pal-title').textContent = title + ' のパーツを選ぶ';
			$('dg-pal').hidden = false;
			Array.prototype.forEach.call($('dg-pal-grid').children, function (b) {
				b.classList.toggle('sel', Number(b.dataset.no) === curNo);
			});
			var s = $('dg-pal-grid').querySelector('.sel');
			$('dg-pal-info').textContent = curNo == null ? '' : '今：' + partLabel(curNo);
			/* まず前回の位置へ。今のパーツがあれば、見えていない時だけそこまで動かす */
			$('dg-pal-grid').scrollTop = palScroll;
			if (s && s.scrollIntoView) s.scrollIntoView({ block: 'nearest' });
			ensureDex();
		}, function (e) { setMsg('パーツ一覧を開けません: ' + (e && e.message || e)); });
	}
	/* 今のパーツの絵のボタン（押すと一覧が開く） */
	function chipBtn(no, onClick) {
		var b = document.createElement('button'), cv = document.createElement('canvas'), n = document.createElement('span');
		b.type = 'button';
		b.className = 'dg-chip';
		b.title = partLabel(no);
		cv.width = cv.height = T;
		var im = pimg(no);
		if (im) cv.getContext('2d').drawImage(im, 0, 0);
		n.textContent = String(no);
		b.appendChild(cv);
		b.appendChild(n);
		b.addEventListener('click', onClick);
		return b;
	}
	/* 絵の並び＋追加＋×。o = { title, multi, get(): 配列, set(配列), clearable } */
	function partsField(box, o) {
		box.textContent = '';
		var arr = o.get();
		arr.forEach(function (no, i) {
			var w = document.createElement('span');
			w.className = 'dg-chipw';
			w.appendChild(chipBtn(no, function () {
				openPal(o.title, no, function (n) { var a = o.get().slice(); a[i] = n; o.set(a); });
			}));
			if (o.multi || o.clearable) {
				var x = document.createElement('button');
				x.type = 'button';
				x.className = 'dg-chipx sub';
				x.textContent = '×';
				x.title = '外す';
				x.addEventListener('click', function () { var a = o.get().slice(); a.splice(i, 1); o.set(a); });
				w.appendChild(x);
			}
			box.appendChild(w);
		});
		if (o.multi || !arr.length) {
			var ad = document.createElement('button');
			ad.type = 'button';
			ad.className = 'dg-add sub';
			ad.textContent = o.multi ? '＋追加' : '選ぶ';
			ad.addEventListener('click', function () {
				openPal(o.title, null, function (n) { var a = o.get().slice(); a.push(n); o.set(a); });
			});
			box.appendChild(ad);
		}
	}
	function renderList(key, bKeepInput) {
		var l = LISTS.filter(function (x) { return x[0] === key; })[0], v = cur[key];
		if (!bKeepInput) $('dg-l-' + key).value = SCALAR[key] ? String(v) : v.join(',');
		partsField($('dg-t-' + key), {
			title: l[1].replace(/（.*$/, ''), multi: !SCALAR[key], clearable: !!SCALAR[key],
			get: function () { var x = cur[key]; return SCALAR[key] ? (x ? [x] : []) : x; },
			set: function (a) { cur[key] = SCALAR[key] ? (a[0] || 0) : a; markDirty(); renderList(key); }
		});
	}
	function renderLight(l, box) {
		partsField(box, {
			title: '灯りを上書きするパーツ', multi: false, clearable: false,
			get: function () { return [l.wParts]; },
			set: function (a) { l.wParts = Math.max(0, a[0] || 0); markDirty(); showLights(); }
		});
	}
	var DTILE = 32;
	function renderDecoGrid() {
		var d = cur.decos[selDeco];
		if (!d) return;
		var ctx = drawGrid($('dg-dcv'), d.w, d.h, function (x, y) { return d.cells[y * d.w + x]; }, DTILE);
		ctx.strokeStyle = 'rgba(255,255,255,0.25)';
		ctx.lineWidth = 1;
		ctx.beginPath();
		for (var x = 1; x < d.w; x++) { ctx.moveTo(x * DTILE + 0.5, 0); ctx.lineTo(x * DTILE + 0.5, d.h * DTILE); }
		for (var y = 1; y < d.h; y++) { ctx.moveTo(0, y * DTILE + 0.5); ctx.lineTo(d.w * DTILE, y * DTILE + 0.5); }
		ctx.stroke();
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
		$('dg-samples').hidden = r.fixed;
		$('dg-rfixed').checked = r.fixed;
		$('dg-rcap').textContent = r.fixed ?
(tools.r === 'mark' ? '固定パターン: 9分割せず、このまま置きます。絵をクリックすると、出てくる位置（入口）の印を付けます（同じ所をもう一度クリックで消えます）。' : '道具が「' + (TOOL_NAME[tools.r] || '') + '」です。絵をクリック／なぞって描きます。') :
			(tools.r === 'mark' ? '青い線（縦2本・横2本）をドラッグして切れ目を動かします。四隅はそのまま、辺は繰り返し、真ん中は縦横に繰り返します。' +
			'線から離れた所をクリックすると、入口の位置の印を付けます（もう一度クリックで消えます）。' : '道具が「' + (TOOL_NAME[tools.r] || '') + '」です。絵をクリック／なぞって描きます（切れ目を動かすには「切れ目・入口の印」を選びます）。');
		var hasE = r.entryX >= 0 && r.entryY >= 0;
		$('dg-rentinfo').textContent = hasE ? '入口の位置: (' + r.entryX + ',' + r.entryY + ')' : '入口の位置: 無し';
		$('dg-rentclr').disabled = !hasE;
		if (!r.fixed) {
			ctx.strokeStyle = '#3aa0ff';
			ctx.lineWidth = 2;
			ctx.beginPath();
			r.cutX.forEach(function (x) { ctx.moveTo(x * tile, 0); ctx.lineTo(x * tile, r.h * tile); });
			r.cutY.forEach(function (y) { ctx.moveTo(0, y * tile); ctx.lineTo(r.w * tile, y * tile); });
			ctx.stroke();
		}
		if (hasE) {
			var ex = (r.entryX + 0.5) * tile, ey = (r.entryY + 0.5) * tile, rad = tile * 0.42;
			ctx.fillStyle = 'rgba(255,230,0,0.35)';
			ctx.fillRect(r.entryX * tile, r.entryY * tile, tile, tile);
			[['#000', 5], ['#ffe600', 2.5]].forEach(function (st) {
				ctx.strokeStyle = st[0];
				ctx.lineWidth = st[1];
				ctx.beginPath();
				ctx.arc(ex, ey, rad, 0, Math.PI * 2);
				ctx.moveTo(ex - rad * 1.5, ey);
				ctx.lineTo(ex + rad * 1.5, ey);
				ctx.moveTo(ex, ey - rad * 1.5);
				ctx.lineTo(ex, ey + rad * 1.5);
				ctx.stroke();
			});
		}
		if (r.fixed) return;
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
			if (!r || tools.r !== 'mark' || ev.button === 2) return;
			var p = pos(ev), t = tileSize(), best = null, bd = 10 * (cv.width / cv.getBoundingClientRect().width);
			[0, 1].forEach(function (i) {
				if (r.fixed) return;
				var dx = Math.abs(p[0] - r.cutX[i] * t), dy = Math.abs(p[1] - r.cutY[i] * t);
				if (dx < bd) { bd = dx; best = { axis: 'x', i: i }; }
				if (dy < bd) { bd = dy; best = { axis: 'y', i: i }; }
			});
			dragCut = best;
			if (best) {
				cv.setPointerCapture(ev.pointerId);
				ev.preventDefault();
			} else {
				var cx = Math.floor(p[0] / t), cy = Math.floor(p[1] / t);
				if (cx >= 0 && cy >= 0 && cx < r.w && cy < r.h) {
					if (r.entryX === cx && r.entryY === cy) { r.entryX = r.entryY = -1; } else { r.entryX = cx; r.entryY = cy; }
					markDirty();
					drawRoom();
				}
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
		return { name: '新しいテーマ', algo: 1, floor: [1], wallTop: [1], wallFace: [1], wallFill: [1], 'void': 0, arch: [1], door: 0, rooms: [], decos: [], openings: [], lights: [] };
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
		$('dg-guide').hidden = !!cur;
		$('dg-rooms-sec').hidden = !cur;
		$('dg-ops-sec').hidden = !cur;
		$('dg-lights-sec').hidden = !cur;
		$('dg-decos-sec').hidden = !cur;
		if (!cur) return;
		refreshTools();
		$('dg-name').value = cur.name;
		$('dg-algo').value = String(cur.algo);
		LISTS.forEach(function (l) {
			var v = cur[l[0]];
			$('dg-l-' + l[0]).value = SCALAR[l[0]] ? String(v) : v.join(',');
		});
		showThumbs();
		selRoom = cur.rooms.length ? Math.min(Math.max(selRoom, 0), cur.rooms.length - 1) : -1;
		selDeco = cur.decos.length ? Math.min(Math.max(selDeco, 0), cur.decos.length - 1) : -1;
		selOp = cur.openings.length ? Math.min(Math.max(selOp, 0), cur.openings.length - 1) : -1;
		showRooms();
		showOps();
		showLights();
		showDecos();
	}

	function showThumbs(bKeep) {
		LISTS.forEach(function (l) {
			renderList(l[0], bKeep);
		});
	}

	function parseList(s) {
		return String(s).split(/[,、，\s]+/).filter(function (x) { return x !== ''; }).map(Number).filter(function (n) { return isFinite(n) && n >= 0; }).map(Math.floor);
	}

	function norm(t) {
		if (!t) return t;
		t.openings = t.openings || [];
		t.lights = t.lights || [];
		t.rooms = t.rooms || [];
		t.decos = t.decos || [];
		t.rooms.forEach(function (r) {
			r.fixed = r.fixed === true;
			if (r.entryX == null) r.entryX = -1;
			if (r.entryY == null) r.entryY = -1;
		});
		return t;
	}

	function pickTheme(t) {
		cur = t ? norm(JSON.parse(JSON.stringify(t))) : null;
		selRoom = selDeco = selOp = -1;
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

	/* ---------------------------------------------------------------- */
	/* 格子に絵を描く道具（部屋・出入口・飾りで共通）						 */
	/* ---------------------------------------------------------------- */
	var brush = 1;						/* 描くパーツ */
	var tools = { r: 'mark', o: 'mark', d: 'paint' };		/* 区画ごとの道具: mark（切れ目・印）/ paint / erase / pick */
	var TOOL_NAME = { paint: '描く', erase: '消しゴム（0）', pick: 'スポイト' };
	var MARK_NAME = { r: '切れ目・入口の印', o: '通れる範囲' };
	function target(k) {
		return k === 'r' ? cur.rooms[selRoom] : k === 'o' ? cur.openings[selOp] : cur.decos[selDeco];
	}
	function refreshTools() {
		['r', 'o', 'd'].forEach(function (k) {
			var box = $('dg-tb-' + k);
			if (!box) return;
			box.textContent = '';
			var names = MARK_NAME[k] ? [['mark', MARK_NAME[k]]] : [];
			['paint', 'erase', 'pick'].forEach(function (t) { names.push([t, TOOL_NAME[t]]); });
			names.forEach(function (n) {
				var b = document.createElement('button');
				b.type = 'button';
				b.className = tools[k] === n[0] ? '' : 'sub';
				b.textContent = n[1];
				b.addEventListener('click', function () { tools[k] = n[0]; refreshTools(); });
				box.appendChild(b);
			});
			var lab = document.createElement('span');
			lab.className = 'sub-text';
			lab.textContent = ' 筆：';
			box.appendChild(lab);
			box.appendChild(chipBtn(brush, function () {
				openPal('筆', brush, function (n) { brush = n; if (tools[k] === 'erase' || tools[k] === 'pick') tools[k] = 'paint'; refreshTools(); });
			}));
			var hint = document.createElement('span');
			hint.className = 'dg-cap';
			hint.textContent = '右クリックで 0 に戻す';
			box.appendChild(hint);
			/* 大きさの変更（端に行・列を足す／削る） */
			var rb = $('dg-rb-' + k);
			rb.textContent = '';
			var l = document.createElement('span');
			l.className = 'sub-text';
			l.textContent = '大きさを変える：';
			rb.appendChild(l);
			[['t', '上'], ['b', '下'], ['l', '左'], ['r', '右']].forEach(function (sd) {
				[[1, '＋'], [-1, '－']].forEach(function (dd) {
					var b = document.createElement('button');
					b.type = 'button';
					b.className = 'sub';
					b.textContent = sd[1] + dd[1];
					b.title = sd[1] + 'の端に' + (sd[0] === 't' || sd[0] === 'b' ? '行' : '列') + (dd[0] > 0 ? 'を足す' : 'を削る');
					b.addEventListener('click', function () { resizeEdge(k, sd[0], dd[0]); });
					rb.appendChild(b);
				});
			});
		});
	}
	var LIMIT = { r: [2, 64], o: [1, 16], d: [1, 16] };
	function resizeEdge(k, side, d) {
		var o = target(k);
		if (!o) return;
		var horiz = side === 'l' || side === 'r';
		var nw = o.w + (horiz ? d : 0), nh = o.h + (horiz ? 0 : d), lim = LIMIT[k];
		if (nw < lim[0] || nh < lim[0] || nw > lim[1] || nh > lim[1]) { setMsg('大きさは ' + lim[0] + '〜' + lim[1] + ' の間にしてください。'); return; }
		if (k === 'o') { var er = checkOpSize(o.dir, nw, nh); if (er) { setMsg(er); return; } }
		var sx = side === 'l' ? d : 0, sy = side === 't' ? d : 0;
		function re(cells) {
			var out = [];
			for (var y = 0; y < nh; y++) for (var x = 0; x < nw; x++) {
				var ox = x - sx, oy = y - sy;
				out.push(ox >= 0 && oy >= 0 && ox < o.w && oy < o.h ? cells[oy * o.w + ox] : 0);
			}
			return out;
		}
		o.cells = re(o.cells);
		if (o.cellsOpen && o.cellsOpen.length === o.w * o.h) o.cellsOpen = re(o.cellsOpen);
		o.w = nw;
		o.h = nh;
		if (k === 'r') fixRoom(o, sx, sy);
		if (k === 'o') fixOp(o, o.dir === 0 ? sx : sy);
		markDirty();
		if (k === 'r') showRooms(); else if (k === 'o') showOps(); else showDecos();
	}
	function fixRoom(r, sx, sy) {
		r.cutX = r.cutX.map(function (v) { return v + sx; });
		r.cutY = r.cutY.map(function (v) { return v + sy; });
		if (!(r.cutX[0] > 0 && r.cutX[0] < r.cutX[1] && r.cutX[1] <= r.w)) r.cutX = r.w >= 3 ? [1, r.w - 1] : [1, 2];
		if (!(r.cutY[0] > 0 && r.cutY[0] < r.cutY[1] && r.cutY[1] <= r.h)) r.cutY = r.h >= 3 ? [1, r.h - 1] : [1, 2];
		if (r.entryX >= 0) {
			r.entryX += sx;
			r.entryY += sy;
			if (r.entryX < 0 || r.entryY < 0 || r.entryX >= r.w || r.entryY >= r.h) r.entryX = r.entryY = -1;
		}
	}
	function fixOp(o, shift) {
		var len = opLen(o);
		o.hole0 += shift;
		o.hole1 += shift;
		if (!(o.hole0 >= 0 && o.hole0 < o.hole1 && o.hole1 <= len)) { var hl = defHole(len); o.hole0 = hl[0]; o.hole1 = hl[1]; }
	}
	/* 格子の canvas に絵を描く操作。get() は { w, h, cells, tile, redraw } を返す（k = 区画） */
	function attachPaint(cv, k, get, bMarkIsPaint) {
		var down = false, once = null;
		function tl() { var t = tools[k]; return once || (t === 'mark' && bMarkIsPaint ? 'paint' : t); }
		function apply(ev) {
			var g = get();
			if (!g) return;
			var r = cv.getBoundingClientRect();
			var x = Math.floor((ev.clientX - r.left) * (cv.width / r.width) / g.tile), y = Math.floor((ev.clientY - r.top) * (cv.height / r.height) / g.tile);
			if (x < 0 || y < 0 || x >= g.w || y >= g.h) return;
			var t = tl(), i = y * g.w + x;
			if (t === 'pick') { brush = g.cells[i]; tools[k] = 'paint'; down = false; refreshTools(); return; }
			var v = t === 'erase' ? 0 : brush;
			if (g.cells[i] === v) return;
			g.cells[i] = v;
			markDirty();
			g.redraw();
		}
		cv.addEventListener('contextmenu', function (ev) { ev.preventDefault(); });
		cv.addEventListener('pointerdown', function (ev) {
			var t = tools[k];
			once = ev.button === 2 ? 'erase' : null;
			if (!once && t === 'mark' && !bMarkIsPaint) return;
			down = true;
			try { cv.setPointerCapture(ev.pointerId); } catch (e) { /* 無くても動く */ }
			ev.preventDefault();
			apply(ev);
		});
		cv.addEventListener('pointermove', function (ev) { if (down) apply(ev); });
		function end() { down = false; once = null; }
		cv.addEventListener('pointerup', end);
		cv.addEventListener('pointercancel', end);
	}
	function blank(n) { var a = []; for (var i = 0; i < n; i++) a.push(0); return a; }
	function addRoom() {
		var w = num($('dg-rw').value, 0), h = num($('dg-rh').value, 0);
		if (w < 2 || h < 2 || w > 64 || h > 64) { setMsg('部屋の大きさは w・h とも 2〜64 にしてください。'); return; }
		if (cur.rooms.length >= 50) { setMsg('部屋は 50 件までです。'); return; }
		cur.rooms.push({
			name: '部屋' + (cur.rooms.length + 1), kind: 0, w: w, h: h, cells: blank(w * h),
			cutX: w >= 3 ? [1, w - 1] : [1, 2], cutY: h >= 3 ? [1, h - 1] : [1, 2], fixed: false, entryX: -1, entryY: -1
		});
		selRoom = cur.rooms.length - 1;
		tools.r = 'paint';
		markDirty();
		showRooms();
		refreshTools();
		setMsg('空の部屋を足しました。筆のパーツを選んで、絵をなぞって描いてください。', true);
	}
	function addOpening() {
		if (cur.openings.length >= 50) { setMsg('出入口パターンは 50 件までです。'); return; }
		var dir = num($('dg-oidir').value, 0), type = num($('dg-oitype').value, 1), w = num($('dg-ow').value, 0), h = num($('dg-oh').value, 0);
		var er = checkOpSize(dir, w, h);
		if (!er && (w < 1 || h < 1 || w > 16 || h > 16)) er = '大きさは 1〜16 にしてください。';
		if (er) { setMsg(er); return; }
		var hl = defHole(dir === 0 ? w : h);
		cur.openings.push({ name: '出入口' + (cur.openings.length + 1), dir: dir, type: type, w: w, h: h, cells: blank(w * h), cellsOpen: [], hole0: hl[0], hole1: hl[1] });
		selOp = cur.openings.length - 1;
		tools.o = 'paint';
		markDirty();
		showOps();
		refreshTools();
	}
	function dupOpening() {
		var o = cur.openings[selOp];
		if (!o) return;
		if (cur.openings.length >= 50) { setMsg('出入口パターンは 50 件までです。'); return; }
		var c = JSON.parse(JSON.stringify(o));
		c.name = o.name + '(コピー)';
		cur.openings.push(c);
		selOp = cur.openings.length - 1;
		markDirty();
		showOps();
	}
	function dupDeco() {
		var d = cur.decos[selDeco];
		if (!d) return;
		if (cur.decos.length >= 50) { setMsg('飾りは 50 件までです。'); return; }
		var c = JSON.parse(JSON.stringify(d));
		c.name = d.name + '(コピー)';
		cur.decos.push(c);
		selDeco = cur.decos.length - 1;
		markDirty();
		showDecos();
	}
	function dupRoom() {
		var r = cur.rooms[selRoom];
		if (!r) return;
		if (cur.rooms.length >= 50) { setMsg('部屋は 50 件までです。'); return; }
		var c = JSON.parse(JSON.stringify(r));
		c.name = r.name + '(コピー)';
		cur.rooms.push(c);
		selRoom = cur.rooms.length - 1;
		markDirty();
		showRooms();
	}

	/* ---------------------------------------------------------------- */
	/* 出入口パターン															 */
	/* ---------------------------------------------------------------- */
	var OP_TYPE = ['', 'アーチ', '扉', '壁ごと抜く'], OP_DIR = ['横の壁', '縦の壁'];
	var dragHole = -1;			/* 0 = hole0, 1 = hole1 */

	function opLen(o) { return o.dir === 0 ? o.w : o.h; }
	function defHole(len) { return len >= 3 ? [1, len - 1] : [0, len]; }
	function opTile(o) { return Math.max(8, Math.min(32, Math.floor(480 / Math.max(o.w, o.h)))); }

	function drawOpCanvas(cv, o, cells) {
		var tile = opTile(o), len = opLen(o);
		var ctx = drawGrid(cv, o.w, o.h, function (x, y) { return cells[y * o.w + x]; }, tile);
		var a = o.hole0 * tile, b = o.hole1 * tile;
		ctx.fillStyle = 'rgba(60,220,120,0.28)';
		if (o.dir === 0) ctx.fillRect(a, 0, b - a, o.h * tile); else ctx.fillRect(0, a, o.w * tile, b - a);
		ctx.lineWidth = 2;
		ctx.strokeStyle = '#3aa0ff';
		ctx.beginPath();
		[a, b].forEach(function (v) {
			if (o.dir === 0) { ctx.moveTo(v, 0); ctx.lineTo(v, o.h * tile); } else { ctx.moveTo(0, v); ctx.lineTo(o.w * tile, v); }
		});
		ctx.stroke();
		return len;
	}

	function drawOp() {
		var o = cur && cur.openings[selOp];
		if (!o) return;
		var isDoor = o.type === 2, hasOpen = o.cellsOpen && o.cellsOpen.length === o.w * o.h;
		$('dg-oopenrow').hidden = !isDoor;
		$('dg-oclropen').disabled = !hasOpen;
		$('dg-oaddopen').disabled = hasOpen;
		$('dg-oopenbox').hidden = !(isDoor && hasOpen);
		$('dg-oinfo').textContent = o.w + '×' + o.h + '・通れる範囲 ' + o.hole0 + '〜' + o.hole1 + '（' + (o.dir === 0 ? '列' : '行') + '）' +
			(isDoor ? (hasOpen ? '・開いた絵あり' : '・開いた絵なし') : '');
		$('dg-ocap').textContent = '青い線（2本）をドラッグして通れる範囲（緑）を決めます。' + (o.dir === 0 ? '横の壁: 列で数えます。' : '縦の壁: 行で数えます。') +
			'線の外は両脇の柱や枠です。上の道具で「描く」にすると絵を描けます。' + (isDoor ? '扉は「開いた絵を作る」で同じ大きさの絵をもう 1 枚描きます。' : '');
		drawOpCanvas($('dg-ocv'), o, o.cells);
		if (isDoor && hasOpen) drawOpCanvas($('dg-ocv2'), o, o.cellsOpen);
	}

	function bindOpCanvas(cv) {
		function pos(ev) {
			var r = cv.getBoundingClientRect(), k = cv.width / r.width;
			return [(ev.clientX - r.left) * k, (ev.clientY - r.top) * k];
		}
		cv.addEventListener('pointerdown', function (ev) {
			var o = cur && cur.openings[selOp];
			if (!o || tools.o !== 'mark' || ev.button === 2) return;
			var p = pos(ev)[o.dir === 0 ? 0 : 1], t = opTile(o);
			var d0 = Math.abs(p - o.hole0 * t), d1 = Math.abs(p - o.hole1 * t);
			dragHole = d0 <= d1 ? 0 : 1;
			if (o.hole0 === o.hole1 - 1 && p > (o.hole0 + 0.5) * t) dragHole = 1;
			cv.setPointerCapture(ev.pointerId);
			ev.preventDefault();
		});
		cv.addEventListener('pointermove', function (ev) {
			var o = cur && cur.openings[selOp];
			if (dragHole < 0 || !o) return;
			var v = Math.round(pos(ev)[o.dir === 0 ? 0 : 1] / opTile(o)), len = opLen(o);
			v = dragHole === 0 ? Math.max(0, Math.min(o.hole1 - 1, v)) : Math.max(o.hole0 + 1, Math.min(len, v));
			var key = dragHole === 0 ? 'hole0' : 'hole1';
			if (v !== o[key]) {
				o[key] = v;
				markDirty();
				drawOp();
			}
		});
		function end() { dragHole = -1; }
		cv.addEventListener('pointerup', end);
		cv.addEventListener('pointercancel', end);
	}

	function showOps() {
		var s = $('dg-osel'), h = '';
		cur.openings.forEach(function (o, i) {
			h += '<option value="' + i + '">' + (i + 1) + ': ' + esc(o.name) + '（' + OP_DIR[o.dir] + '・' + OP_TYPE[o.type] + ' ' + o.w + '×' + o.h + '）</option>';
		});
		s.innerHTML = h || '<option value="-1">（出入口パターンはまだありません）</option>';
		if (selOp >= 0) s.value = String(selOp);
		$('dg-odel').disabled = selOp < 0;
		$('dg-oedit').hidden = selOp < 0;
		if (selOp < 0) return;
		var o = cur.openings[selOp];
		$('dg-oname').value = o.name;
		$('dg-odir').value = String(o.dir);
		$('dg-otype').value = String(o.type);
		drawOp();
	}


	function checkOpSize(dir, w, h) {
		if (dir === 0 && h !== 2) return '横の壁（dir 0）は高さ h を 2 にしてください。';
		if (dir === 1 && w !== 1) return '縦の壁（dir 1）は幅 w を 1 にしてください。';
		return '';
	}



	/* ---------------------------------------------------------------- */
	/* 灯りの上書き表															 */
	/* ---------------------------------------------------------------- */
	function showLights() {
		var box = $('dg-lrows');
		box.textContent = '';
		cur.lights.forEach(function (l, i) {
			var row = document.createElement('div');
			row.className = 'dg-row';
			var lab = document.createElement('label');
			lab.appendChild(document.createTextNode('パーツ '));
			var inp = document.createElement('input');
			inp.type = 'text';
			inp.className = 'dg-n dg-sm';
			inp.value = l.wParts;
			inp.setAttribute('inputmode', 'numeric');
			lab.appendChild(inp);
			row.appendChild(lab);
			var th = document.createElement('span');
			th.className = 'dg-pf';
			row.appendChild(th);
			renderLight(l, th);
			var lab2 = document.createElement('label');
			lab2.appendChild(document.createTextNode('灯りの範囲 '));
			var sel = document.createElement('select');
			for (var k = 0; k <= 7; k++) sel.innerHTML += '<option value="' + k + '">' + k + '</option>';
			sel.value = String(l.byLight);
			lab2.appendChild(sel);
			row.appendChild(lab2);
			var del = document.createElement('button');
			del.type = 'button';
			del.className = 'sub';
			del.textContent = '削除';
			row.appendChild(del);
			inp.addEventListener('change', function () { l.wParts = Math.max(0, num(inp.value, 0)); markDirty(); showLights(); });
			sel.addEventListener('change', function () { l.byLight = Math.max(0, Math.min(7, num(sel.value, 0))); markDirty(); });
			del.addEventListener('click', function () { cur.lights.splice(i, 1); markDirty(); showLights(); });
			box.appendChild(row);
		});
		$('dg-ladd').disabled = cur.lights.length >= 8;
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
		renderDecoGrid();
	}

	/* w・h が変わったら cells の長さを合わせる（足りない所は 0） */
	function fitDecoCells(d, nw, nh) {
		var out = [];
		for (var y = 0; y < nh; y++) for (var x = 0; x < nw; x++) out.push(x < d.w && y < d.h ? d.cells[y * d.w + x] : 0);
		d.cells = out;
		d.w = nw;
		d.h = nh;
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
			lastTheme = body.theme ? JSON.parse(JSON.stringify({ openings: body.theme.openings || [], lights: body.theme.lights || [] })) : null;
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

	/* ---------------------------------------------------------------- */
	/* 暗くして見る（ゲームの DrawMapLight・RenewMapDarkWork の簡易版）		 */
	/* ---------------------------------------------------------------- */
	/* 灯りの中心(sx,sy)から床を伝って広がり、壁は照らして止まる。返す値は w*h の 0/1 */
	function lightMask(r, sx, sy, range) {
		var w = r.w, h = r.h, m = new Uint8Array(w * h), q = [sx + sy * w], qi = 0, k = Math.ceil(range);
		m[q[0]] = 1;
		while (qi < q.length) {
			var c = q[qi++], cx = c % w, cy = (c - cx) / w;
			var v = r.cells[c];
			if (c !== q[0] && !(v === 1 || v === 5 || v === 6)) continue;	/* 壁などは照らすだけで先へ進まない */
			for (var dy = -1; dy <= 1; dy++) {
				for (var dx = -1; dx <= 1; dx++) {
					var nx = cx + dx, ny = cy + dy;
					if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
					if (Math.abs(nx - sx) > k || Math.abs(ny - sy) > k) continue;
					var n = nx + ny * w;
					if (m[n] || r.cells[n] === 0) continue;
					m[n] = 1;
					q.push(n);
				}
			}
		}
		return m;
	}

	function darken(ctx, r, tile) {
		var L = num($('dg-dark').value, 0);
		if (!L) return;
		var w = r.w, h = r.h, lights = [], i, x, y, ovr = {};
		((lastTheme && lastTheme.lights) || []).forEach(function (l) { ovr[l.wParts] = Math.min(7, l.byLight); });	/* テーマの上書き表を先に使う */
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				var no = r.parts[y * w + x], pt = parts[no];
				var bl = ovr.hasOwnProperty(no) ? ovr[no] : (pt && pt.byLight ? Math.min(7, pt.byLight) : 0);
				if (bl > 0 && r.cells[y * w + x] !== 0) {
					lights.push({ x: x, y: y, R: bl + 1.2, pow: 1, warm: 1, mask: lightMask(r, x, y, bl + 1) });
				}
			}
		}
		if ($('dg-dchar').checked && r.start) {
			/* キャラの灯り(明かりの道具あり)の半径: GetCharLightTiles と同じ式。暗さ - 明かりの強さ(20) */
			var lv = L - 20, rr = lv <= 0 ? 0 : Math.max(100 / lv + 0.2, 100 / L + 0.2 + 2.5);
			var rc = rr || 99;
			lights.push({ x: r.start.x, y: r.start.y, R: rc, pow: 1.6, warm: 0.5, mask: lightMask(r, r.start.x, r.start.y, rc) });
		}
		var W = r.w * tile, H = r.h * tile, img = ctx.getImageData(0, 0, W, H), d = img.data;
		var lt = new Float32Array(W * H), wm = new Float32Array(W * H);
		lights.forEach(function (l) {
			var cx = (l.x + 0.5) * tile, cy = (l.y + 0.5) * tile, R = l.R * tile, R2 = R * R;
			var x0 = Math.max(0, Math.floor(cx - R)), x1 = Math.min(W - 1, Math.ceil(cx + R));
			var y0 = Math.max(0, Math.floor(cy - R)), y1 = Math.min(H - 1, Math.ceil(cy + R));
			function mv(mx, my) { return mx < 0 || my < 0 || mx >= w || my >= h ? 0 : l.mask[mx + my * w]; }
			for (var py = y0; py <= y1; py++) {
				var ddy = py + 0.5 - cy, fy = (py + 0.5) / tile - 0.5, iy = Math.floor(fy), ty = fy - iy;
				for (var px = x0; px <= x1; px++) {
					var ddx = px + 0.5 - cx, dd = ddx * ddx + ddy * ddy;
					if (dd >= R2) continue;
					var fx = (px + 0.5) / tile - 0.5, ix = Math.floor(fx), tx = fx - ix;
					var a = mv(ix, iy) + (mv(ix + 1, iy) - mv(ix, iy)) * tx;
					var b = mv(ix, iy + 1) + (mv(ix + 1, iy + 1) - mv(ix, iy + 1)) * tx;
					var vis = Math.min(1, (a + (b - a) * ty) * 1.6);
					if (vis <= 0) continue;
					var add = (1 - Math.sqrt(dd) / R) * l.pow * vis;
					lt[px + py * W] += add;
					wm[px + py * W] += add * l.warm;
				}
			}
		});
		for (i = 0; i < W * H; i++) {
			var f = Math.min(1, lt[i]), wr = lt[i] > 0 ? Math.min(1, wm[i] / lt[i]) : 0, o = i * 4;
			d[o] = Math.min(255, d[o] * f * (1 + 0.25 * wr));
			d[o + 1] = d[o + 1] * f * (1 + 0.02 * wr);
			d[o + 2] = d[o + 2] * f * (1 - 0.25 * wr);
		}
		ctx.putImageData(img, 0, 0);
	}

	function drawOut() {
		var r = last, cv = $('dg-out'), lg = $('dg-legend');
		if (!r) return;
		var view = $('dg-view').value, tile = Math.max(2, Math.round(T * Number($('dg-zoom').value)));
		var ctx;
		lg.hidden = view !== 'meaning';
		cv.title = '';
		if (view === 'parts') {
			if (!r.parts || !r.parts.length) {
				setMsg('パーツの絵がありません（テーマを使っていないか、テーマが空です）。「意味の色」で見てください。');
				view = 'meaning';
			}
		}
		if (view === 'parts') {
			ctx = drawGrid(cv, r.w, r.h, function (x, y) { return r.parts[y * r.w + x]; }, tile);
			darken(ctx, r, tile);
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
		bindOpCanvas($('dg-ocv'));
		attachPaint($('dg-rcv'), 'r', function () {
			var r = cur && cur.rooms[selRoom];
			return r && { w: r.w, h: r.h, cells: r.cells, tile: $('dg-rcv').width / r.w, redraw: drawRoom };
		});
		attachPaint($('dg-ocv'), 'o', function () {
			var o = cur && cur.openings[selOp];
			return o && { w: o.w, h: o.h, cells: o.cells, tile: opTile(o), redraw: drawOp };
		});
		attachPaint($('dg-ocv2'), 'o', function () {
			var o = cur && cur.openings[selOp];
			return o && o.cellsOpen && o.cellsOpen.length === o.w * o.h && { w: o.w, h: o.h, cells: o.cellsOpen, tile: opTile(o), redraw: drawOp };
		}, true);
		attachPaint($('dg-dcv'), 'd', function () {
			var d = cur && cur.decos[selDeco];
			return d && { w: d.w, h: d.h, cells: d.cells, tile: DTILE, redraw: renderDecoGrid };
		});
		refreshTools();
		$('dg-out').addEventListener('mousemove', function (ev) {
			var cv = $('dg-out');
			if (!last || $('dg-view').value !== 'meaning' || !last.links) { cv.title = ''; return; }
			var b = cv.getBoundingClientRect(), k = cv.width / b.width, tile = Math.max(2, Math.round(T * Number($('dg-zoom').value)));
			var x = Math.floor((ev.clientX - b.left) * k / tile), y = Math.floor((ev.clientY - b.top) * k / tile), t = '';
			last.links.forEach(function (l) {
				if (x >= l.x && x < l.x + l.w && y >= l.y && y < l.y + l.h) {
					var ops = (lastTheme && lastTheme.openings) || [], op = l.tpl != null && l.tpl >= 0 ? ops[l.tpl] : null;
					t = '出入口 ' + (OP_TYPE[l.type] || l.type) + '（部屋 ' + l.a + ' - ' + l.b + '）' +
						(l.tpl != null && l.tpl >= 0 ? '・パターン ' + (l.tpl + 1) + (op ? '「' + op.name + '」' : '') : '・パターンなし');
				}
			});
			cv.title = t;
		});
		$('dg-rfixed').addEventListener('change', function () {
			var r = cur.rooms[selRoom];
			if (!r) return;
			r.fixed = $('dg-rfixed').checked;
			markDirty();
			drawRoom();
		});
		$('dg-rentclr').addEventListener('click', function () {
			var r = cur.rooms[selRoom];
			if (!r) return;
			r.entryX = r.entryY = -1;
			markDirty();
			drawRoom();
		});
		$('dg-osel').addEventListener('change', function () { selOp = num($('dg-osel').value, -1); showOps(); });
		$('dg-odel').addEventListener('click', function () {
			if (selOp < 0 || !confirm('この出入口パターンを削除しますか。')) return;
			cur.openings.splice(selOp, 1);
			selOp = Math.min(selOp, cur.openings.length - 1);
			markDirty();
			showOps();
		});
		$('dg-oclropen').addEventListener('click', function () {
			if (selOp < 0) return;
			cur.openings[selOp].cellsOpen = [];
			markDirty();
			showOps();
		});
		$('dg-oname').addEventListener('input', function () { cur.openings[selOp].name = $('dg-oname').value; markDirty(); });
		$('dg-oname').addEventListener('change', showOps);
		$('dg-odir').addEventListener('change', function () {
			var o = cur.openings[selOp], d = num($('dg-odir').value, 0), er = checkOpSize(d, o.w, o.h);
			if (er) { setMsg('向きを変えられません: ' + er); showOps(); return; }
			o.dir = d;
			var hl = defHole(opLen(o));
			o.hole0 = hl[0];
			o.hole1 = hl[1];
			markDirty();
			showOps();
		});
		$('dg-otype').addEventListener('change', function () {
			var o = cur.openings[selOp];
			o.type = num($('dg-otype').value, 1);
			if (o.type !== 2) o.cellsOpen = [];
			markDirty();
			showOps();
		});
		$('dg-ladd').addEventListener('click', function () {
			if (cur.lights.length >= 8) { setMsg('灯りの上書きは 8 件までです。'); return; }
			cur.lights.push({ wParts: 0, byLight: 1 });
			markDirty();
			showLights();
		});
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
			cur = norm(c);
			selRoom = selDeco = selOp = -1;
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
				showThumbs(true);
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
		$('dg-radd').addEventListener('click', addRoom);
		$('dg-rdup').addEventListener('click', dupRoom);
		$('dg-oadd').addEventListener('click', addOpening);
		$('dg-odup').addEventListener('click', dupOpening);
		$('dg-oaddopen').addEventListener('click', function () {
			var o = cur.openings[selOp];
			if (!o || o.type !== 2) return;
			o.cellsOpen = o.cells.slice();
			tools.o = 'paint';
			markDirty();
			showOps();
			refreshTools();
		});
		$('dg-ddup').addEventListener('click', dupDeco);
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
		decoField('dg-dw', function (d, v) { fitDecoCells(d, Math.max(1, Math.min(16, num(v, 1))), d.h); });
		decoField('dg-dh', function (d, v) { fitDecoCells(d, d.w, Math.max(1, Math.min(16, num(v, 1)))); });
		decoField('dg-don', function (d, v) { d.on = num(v, 1); });
		decoField('dg-drate', function (d, v) { d.rate = Math.max(0, Math.min(100, num(v, 0))); });
		decoField('dg-dmax', function (d, v) { d.max = Math.max(0, Math.min(999, num(v, 0))); });
		$('dg-gen').addEventListener('click', function () { generate(false); });
		$('dg-gen2').addEventListener('click', function () { generate(true); });
		$('dg-view').addEventListener('change', drawOut);
		$('dg-zoom').addEventListener('change', drawOut);
		$('dg-dark').addEventListener('change', drawOut);
		$('dg-dchar').addEventListener('change', drawOut);
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
