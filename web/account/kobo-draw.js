/**
 * kobo-draw.js
 * ドット絵工房の描く画面（/kobo-draw?task=ID&from=base|blank|案id&work=作業中の案id）。
 * 仕様は docs/kobo.md。キャンバス・道具・保存。PC もスマホも同じコードで動く。
 *
 * 絵は task.w × task.h の Uint8Array（パレット番号）。task.palette のうち task.colors の番号だけ選べる。
 * 透明は task.transparent の番号（パレットの黒）。消しゴムはこの番号で塗る。
 * 描く部品（塗りつぶし・線・領域の切り貼り・PNG の読み書き）は管理者用エディタと同じ dot-core.js・dot-png.js。
 */
import {
	KEY_TOKEN, lsGet, lsSet, lsDel, api, el, imgUrl, refImgEl,
	loadIndexed, makePreview, thumbCanvas
} from '/dot-kobo.js';
import { floodFill, bresenham, getRegion, putRegion, fillRect } from '/dot-core.js';
import { encodeIndexedPng, indicesToImageData } from '/dot-png.js';

const $ = function (id) { return document.getElementById(id); };
const UNDO_MAX = 50;
const LOCAL_SAVE_MS = 3000;		// 端末への自動保存
const SERVER_SAVE_MS = 60000;	// サーバーへの作業中保存（「みんなに見せる」がオンの時だけ）

/* ---- 状態 ---- */
const S = {
	task: null, me: null, token: '',
	w: 0, h: 0, data: null,
	tool: 'pen', prevTool: 'pen', color: 0,
	undo: [], redo: [],
	sel: null, float: null, clip: null,
	z: 8, ox: 0, oy: 0, grid: true, hover: null,
	workId: '', parent: '', from: 'base',
	rev: 0, serverRev: -1, localRev: -1,
	imgDirty: true, busy: false, coolUntil: 0, canSave: false,
	spaceDown: false, closed: false
};
const view = $('view');
const ctx = view.getContext('2d');
const imgCanvas = document.createElement('canvas');
const imgCtx = imgCanvas.getContext('2d');
let dpr = 1, vw = 300, vh = 300;
let pal = [];			// 表示用パレット {r,g,b,a}
let pvMain = null, pvMini = null;
let drawQueued = false;

/* ============ 小さな道具 ============ */

function u8ToB64(u8) {
	let s = '';
	for (let i = 0; i < u8.length; i += 0x8000) {
		s += String.fromCharCode.apply(null, u8.subarray(i, i + 0x8000));
	}
	return btoa(s);
}
function b64ToU8(b64) {
	const s = atob(b64);
	const u = new Uint8Array(s.length);
	for (let i = 0; i < s.length; i++) {
		u[i] = s.charCodeAt(i);
	}
	return u;
}
function clamp(v, a, b) {
	return Math.max(a, Math.min(b, v));
}
let toastTimer = 0;
function toast(text) {
	const t = $('toast');
	t.textContent = text;
	t.classList.add('show');
	clearTimeout(toastTimer);
	toastTimer = setTimeout(function () { t.classList.remove('show'); }, 2200);
}
function setMsg(text, bOk) {
	$('msg').textContent = text || '';
	$('msg').classList.toggle('ok', !!bOk);
}
function fatal(text) {
	const l = $('loading');
	l.hidden = false;
	l.textContent = '';
	const d = el('div');
	d.appendChild(el('p', '', text));
	const a = el('a', '', 'ドット絵工房へ戻る');
	a.href = '/kobo';
	d.appendChild(a);
	l.appendChild(d);
}
function clock() {
	return new Date().toLocaleTimeString('ja-JP', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}

/* ============ 絵のデータ ============ */

/** 浮いている選択範囲を重ねた絵（保存・プレビュー・スポイトはこちらを見る） */
function composite() {
	if (!S.float) {
		return S.data;
	}
	const c = S.data.slice();
	putRegion(c, S.w, S.h, S.float.x, S.float.y, S.float.buf, S.float.w, S.float.h, S.task.transparent);
	return c;
}

function onChange() {
	S.rev++;
	S.imgDirty = true;
	requestDraw();
}

function pushHistory() {
	S.undo.push(S.data.slice());
	if (S.undo.length > UNDO_MAX) {
		S.undo.shift();
	}
	S.redo.length = 0;
	updateUndoButtons();
}

function commitFloat() {
	const f = S.float;
	if (!f) {
		return;
	}
	putRegion(S.data, S.w, S.h, f.x, f.y, f.buf, f.w, f.h, S.task.transparent);
	S.sel = { x: f.x, y: f.y, w: f.w, h: f.h };
	S.float = null;
	onChange();
}

function undo() {
	commitFloat();
	if (!S.undo.length) {
		return;
	}
	S.redo.push(S.data.slice());
	S.data = S.undo.pop();
	onChange();
	updateUndoButtons();
}
function redo() {
	commitFloat();
	if (!S.redo.length) {
		return;
	}
	S.undo.push(S.data.slice());
	S.data = S.redo.pop();
	onChange();
	updateUndoButtons();
}
function updateUndoButtons() {
	$('undo').disabled = !S.undo.length && !S.float;
	$('redo').disabled = !S.redo.length;
}

/* ============ 表示 ============ */

function requestDraw() {
	if (drawQueued) {
		return;
	}
	drawQueued = true;
	requestAnimationFrame(function () {
		drawQueued = false;
		draw();
	});
}

function refreshImg() {
	const comp = composite();
	imgCtx.putImageData(indicesToImageData(comp, S.w, S.h, pal), 0, 0);
	if (pvMain) {
		pvMain.update(comp, S.w, S.h);
	}
	if (pvMini) {
		pvMini.update(comp, S.w, S.h);
	}
	S.imgDirty = false;
	updateUndoButtons();
}

function draw() {
	if (!S.task || !S.data) {
		return;
	}
	if (S.imgDirty) {
		refreshImg();
	}
	const { w, h, z, ox, oy } = S;
	ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
	ctx.fillStyle = '#101216';
	ctx.fillRect(0, 0, vw, vh);
	const iw = w * z;
	const ih = h * z;
	ctx.save();
	ctx.beginPath();
	ctx.rect(ox, oy, iw, ih);
	ctx.clip();
	/* 透明は市松模様 */
	const cs = Math.max(4, Math.round(z / 2));
	const i0 = Math.max(0, Math.floor(-ox / cs));
	const i1 = Math.min(Math.ceil(iw / cs), Math.ceil((vw - ox) / cs));
	const j0 = Math.max(0, Math.floor(-oy / cs));
	const j1 = Math.min(Math.ceil(ih / cs), Math.ceil((vh - oy) / cs));
	for (let j = j0; j < j1; j++) {
		for (let i = i0; i < i1; i++) {
			ctx.fillStyle = ((i + j) & 1) ? '#9a9a9a' : '#c8c8c8';
			ctx.fillRect(ox + i * cs, oy + j * cs, cs, cs);
		}
	}
	ctx.imageSmoothingEnabled = false;
	ctx.drawImage(imgCanvas, ox, oy, iw, ih);
	/* グリッド */
	if (S.grid) {
		if (z >= 5) {
			ctx.strokeStyle = 'rgba(0,0,0,0.22)';
			ctx.lineWidth = 1;
			ctx.beginPath();
			for (let x = 0; x <= w; x++) {
				const sx = Math.round(ox + x * z) + 0.5;
				if (sx >= 0 && sx <= vw) { ctx.moveTo(sx, oy); ctx.lineTo(sx, oy + ih); }
			}
			for (let y = 0; y <= h; y++) {
				const sy = Math.round(oy + y * z) + 0.5;
				if (sy >= 0 && sy <= vh) { ctx.moveTo(ox, sy); ctx.lineTo(ox + iw, sy); }
			}
			ctx.stroke();
		}
		/* コマの区切りは濃く */
		const cw = S.task.cw, ch = S.task.ch;
		ctx.strokeStyle = 'rgba(200,30,30,0.9)';
		ctx.lineWidth = 1;
		ctx.beginPath();
		for (let x = 0; x <= w; x += cw) {
			const sx = Math.round(ox + x * z) + 0.5;
			ctx.moveTo(sx, oy); ctx.lineTo(sx, oy + ih);
		}
		for (let y = 0; y <= h; y += ch) {
			const sy = Math.round(oy + y * z) + 0.5;
			ctx.moveTo(ox, sy); ctx.lineTo(ox + iw, sy);
		}
		ctx.stroke();
	}
	ctx.restore();
	ctx.strokeStyle = '#f0b84a';
	ctx.lineWidth = 1;
	ctx.strokeRect(ox - 0.5, oy - 0.5, iw + 1, ih + 1);

	/* 選択範囲 */
	const r = S.float || S.sel;
	if (r) {
		const x = ox + r.x * z, y = oy + r.y * z, rw = r.w * z, rh = r.h * z;
		ctx.lineWidth = 1;
		ctx.setLineDash([5, 4]);
		ctx.strokeStyle = '#000';
		ctx.strokeRect(x + 0.5, y + 0.5, rw, rh);
		ctx.strokeStyle = '#fff';
		ctx.lineDashOffset = 5;
		ctx.strokeRect(x + 0.5, y + 0.5, rw, rh);
		ctx.setLineDash([]);
		ctx.lineDashOffset = 0;
	}
	/* カーソルのマス */
	if (S.hover && S.hover.x >= 0 && S.hover.y >= 0 && S.hover.x < w && S.hover.y < h && z >= 3) {
		const x = ox + S.hover.x * z, y = oy + S.hover.y * z;
		ctx.strokeStyle = '#000';
		ctx.strokeRect(x + 0.5, y + 0.5, z - 1, z - 1);
		ctx.strokeStyle = '#fff';
		ctx.strokeRect(x + 1.5, y + 1.5, z - 3, z - 3);
	}
	$('info').textContent = (S.hover && S.hover.x >= 0 && S.hover.y >= 0 && S.hover.x < w && S.hover.y < h ? S.hover.x + ', ' + S.hover.y + '　' : '') +
		w + '×' + h + '　×' + (Math.round(z * 10) / 10);
}

function resize() {
	const r = $('stage').getBoundingClientRect();
	dpr = window.devicePixelRatio || 1;
	vw = Math.max(50, r.width);
	vh = Math.max(50, r.height);
	view.width = Math.round(vw * dpr);
	view.height = Math.round(vh * dpr);
	requestDraw();
}

function fit() {
	const z = Math.max(1, Math.floor(Math.min((vw - 24) / S.w, (vh - 24) / S.h)));
	S.z = z;
	S.ox = Math.round((vw - S.w * z) / 2);
	S.oy = Math.round((vh - S.h * z) / 2);
	requestDraw();
}

/** (px, py)（画面上の点）を動かさずに倍率を変える */
function zoomAt(px, py, nz) {
	nz = clamp(nz, 1, 80);
	const ix = (px - S.ox) / S.z;
	const iy = (py - S.oy) / S.z;
	S.z = nz;
	S.ox = px - ix * nz;
	S.oy = py - iy * nz;
	requestDraw();
}

function stepZoom(dir, px, py) {
	let nz;
	if (dir > 0) {
		nz = Math.max(Math.floor(S.z) + 1, Math.round(S.z * 1.2));
	} else {
		nz = Math.min(Math.ceil(S.z) - 1, Math.round(S.z / 1.2));
	}
	zoomAt(px == null ? vw / 2 : px, py == null ? vh / 2 : py, nz);
}

/* ============ 道具・色 ============ */

function setTool(t) {
	if (S.tool !== t) {
		if (S.tool !== 'select' || t !== 'select') {
			commitFloat();
		}
		S.prevTool = S.tool === 'picker' ? S.prevTool : S.tool;
	}
	S.tool = t;
	Array.prototype.forEach.call(document.querySelectorAll('#tools button'), function (b) {
		b.classList.toggle('active', b.dataset.tool === t);
	});
	view.classList.toggle('hand', t === 'hand');
	requestDraw();
}

function setColor(i) {
	S.color = i;
	Array.prototype.forEach.call(document.querySelectorAll('#palette .sw'), function (b) {
		b.classList.toggle('sel', +b.dataset.i === i);
	});
	const c = S.task.palette[i];
	const sw = $('cur-sw');
	if (i === S.task.transparent) {
		sw.className = 'checker';
		sw.style.background = '';
		$('cur-text').textContent = '透明（' + i + '）';
	} else {
		sw.className = '';
		sw.style.background = 'rgb(' + c[0] + ',' + c[1] + ',' + c[2] + ')';
		$('cur-text').textContent = '色 ' + i;
	}
	if (S.tool === 'eraser' || S.tool === 'picker') {
		setTool(S.tool === 'picker' ? S.prevTool : 'pen');
	}
}

function buildPalette() {
	const box = $('palette');
	box.textContent = '';
	S.task.colors.forEach(function (i) {
		const c = S.task.palette[i];
		const b = el('button', 'sw' + (i === S.task.transparent ? ' checker' : ''));
		b.type = 'button';
		b.dataset.i = i;
		b.title = (i === S.task.transparent ? '透明' : '色') + ' ' + i;
		if (i !== S.task.transparent) {
			b.style.background = 'rgb(' + c[0] + ',' + c[1] + ',' + c[2] + ')';
		}
		b.addEventListener('click', function () { setColor(i); });
		box.appendChild(b);
	});
}

function pickAt(x, y) {
	if (x < 0 || y < 0 || x >= S.w || y >= S.h) {
		return;
	}
	const v = composite()[y * S.w + x];
	if (S.task.colors.indexOf(v) < 0) {
		toast('その色は選べない色です');
		return;
	}
	setColor(v);
}

/* ============ 入力（マウス・ペン・タッチ） ============ */

const ptrs = new Map();
let mode = null;		// 'draw' 'pan' 'gesture' 'marquee' 'move' 'wait'
let snap = null;		// ストローク前の絵
let last = null;		// 前に描いたマス
let panStart = null;
let gest = null;
let mq = null;
let mv = null;

function pos(e) {
	const r = view.getBoundingClientRect();
	const px = e.clientX - r.left;
	const py = e.clientY - r.top;
	return { px: px, py: py, x: Math.floor((px - S.ox) / S.z), y: Math.floor((py - S.oy) / S.z) };
}

function plotPixel(x, y, v) {
	if (x < 0 || y < 0 || x >= S.w || y >= S.h) {
		return;
	}
	S.data[y * S.w + x] = v;
}

function toolValue() {
	return S.tool === 'eraser' ? S.task.transparent : S.color;
}

function startStroke(p) {
	commitFloat();
	snap = S.data.slice();
	mode = 'draw';
	last = p;
	const v = toolValue();
	plotPixel(p.x, p.y, v);
	onChange();
}

function endStroke() {
	if (snap) {
		let changed = false;
		for (let i = 0; i < snap.length; i++) {
			if (snap[i] !== S.data[i]) { changed = true; break; }
		}
		if (changed) {
			S.undo.push(snap);
			if (S.undo.length > UNDO_MAX) {
				S.undo.shift();
			}
			S.redo.length = 0;
		}
	}
	snap = null;
	updateUndoButtons();
}

function cancelCurrent() {
	if (mode === 'draw' && snap) {
		S.data = snap;
		snap = null;
		onChange();
	}
	if (mode === 'marquee') {
		S.sel = mq && mq.prev;
	}
	mode = null;
}

function inRect(r, p) {
	return r && p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;
}

function startTool(p) {
	const t = S.tool;
	if (t === 'hand') {
		startPan(p);
	} else if (t === 'pen' || t === 'eraser') {
		startStroke(p);
	} else if (t === 'picker') {
		pickAt(p.x, p.y);
		setTool(S.prevTool);
		mode = 'wait';
	} else if (t === 'fill') {
		if (p.x >= 0 && p.y >= 0 && p.x < S.w && p.y < S.h) {
			commitFloat();
			const before = S.data.slice();
			const clip = S.sel ? { x: S.sel.x, y: S.sel.y, w: S.sel.w, h: S.sel.h } : undefined;
			if (floodFill(S.data, S.w, S.h, p.x, p.y, S.color, clip) > 0) {
				S.undo.push(before);
				if (S.undo.length > UNDO_MAX) {
					S.undo.shift();
				}
				S.redo.length = 0;
				onChange();
			}
		}
		mode = 'wait';
	} else if (t === 'select') {
		if (S.float && inRect(S.float, p)) {
			mode = 'move';
			mv = { sx: p.x, sy: p.y, fx: S.float.x, fy: S.float.y, lifted: true };
		} else if (!S.float && S.sel && inRect(S.sel, p)) {
			mode = 'move';
			mv = { sx: p.x, sy: p.y, lifted: false };
		} else {
			commitFloat();
			const prev = S.sel;
			const x0 = clamp(p.x, 0, S.w - 1);
			const y0 = clamp(p.y, 0, S.h - 1);
			mq = { x0: x0, y0: y0, moved: false, prev: prev };
			S.sel = { x: x0, y: y0, w: 1, h: 1 };
			mode = 'marquee';
			requestDraw();
		}
	}
}

function liftSelection() {
	const s = S.sel;
	pushHistory();
	const buf = getRegion(S.data, S.w, S.h, s.x, s.y, s.w, s.h);
	fillRect(S.data, S.w, S.h, s.x, s.y, s.w, s.h, S.task.transparent);
	S.float = { buf: buf, x: s.x, y: s.y, w: s.w, h: s.h };
	mv.fx = s.x;
	mv.fy = s.y;
	mv.lifted = true;
}

function startPan(p) {
	mode = 'pan';
	panStart = { px: p.px, py: p.py, ox: S.ox, oy: S.oy };
}

function startGesture() {
	const a = Array.from(ptrs.values());
	const r = view.getBoundingClientRect();
	const mx = (a[0].x + a[1].x) / 2 - r.left;
	const my = (a[0].y + a[1].y) / 2 - r.top;
	gest = {
		mx: mx, my: my,
		d: Math.max(10, Math.hypot(a[0].x - a[1].x, a[0].y - a[1].y)),
		z: S.z, ox: S.ox, oy: S.oy
	};
	mode = 'gesture';
}

view.addEventListener('pointerdown', function (e) {
	view.setPointerCapture(e.pointerId);
	ptrs.set(e.pointerId, { x: e.clientX, y: e.clientY, type: e.pointerType });
	if (ptrs.size === 2 && e.pointerType === 'touch') {
		cancelCurrent();
		startGesture();
		return;
	}
	if (ptrs.size > 1) {
		return;
	}
	const p = pos(e);
	if (e.pointerType === 'mouse') {
		if (e.button === 1 || (e.button === 0 && S.spaceDown)) {
			e.preventDefault();
			startPan(p);
			return;
		}
		if (e.button === 2) {
			pickAt(p.x, p.y);
			mode = 'wait';
			return;
		}
		if (e.button !== 0) {
			return;
		}
	}
	startTool(p);
});

view.addEventListener('pointermove', function (e) {
	const rec = ptrs.get(e.pointerId);
	if (rec) {
		rec.x = e.clientX;
		rec.y = e.clientY;
	}
	const p = pos(e);
	if (e.pointerType === 'mouse') {
		S.hover = { x: p.x, y: p.y };
		if (!mode) {
			requestDraw();
		}
	}
	if (mode === 'gesture' && ptrs.size >= 2) {
		const a = Array.from(ptrs.values());
		const r = view.getBoundingClientRect();
		const mx = (a[0].x + a[1].x) / 2 - r.left;
		const my = (a[0].y + a[1].y) / 2 - r.top;
		const d = Math.hypot(a[0].x - a[1].x, a[0].y - a[1].y);
		const nz = clamp(gest.z * d / gest.d, 1, 80);
		const ix = (gest.mx - gest.ox) / gest.z;
		const iy = (gest.my - gest.oy) / gest.z;
		S.z = nz;
		S.ox = mx - ix * nz;
		S.oy = my - iy * nz;
		requestDraw();
		return;
	}
	if (!rec || ptrs.size > 1) {
		return;
	}
	if (mode === 'draw') {
		const v = toolValue();
		bresenham(last.x, last.y, p.x, p.y, function (x, y) { plotPixel(x, y, v); });
		last = p;
		onChange();
	} else if (mode === 'pan') {
		S.ox = panStart.ox + (p.px - panStart.px);
		S.oy = panStart.oy + (p.py - panStart.py);
		requestDraw();
	} else if (mode === 'marquee') {
		const x1 = clamp(p.x, 0, S.w - 1);
		const y1 = clamp(p.y, 0, S.h - 1);
		if (x1 !== mq.x0 || y1 !== mq.y0) {
			mq.moved = true;
		}
		S.sel = {
			x: Math.min(mq.x0, x1), y: Math.min(mq.y0, y1),
			w: Math.abs(x1 - mq.x0) + 1, h: Math.abs(y1 - mq.y0) + 1
		};
		requestDraw();
	} else if (mode === 'move') {
		const dx = p.x - mv.sx;
		const dy = p.y - mv.sy;
		if (!mv.lifted) {
			if (!dx && !dy) {
				return;
			}
			liftSelection();
		}
		S.float.x = mv.fx + dx;
		S.float.y = mv.fy + dy;
		S.sel = { x: S.float.x, y: S.float.y, w: S.float.w, h: S.float.h };
		onChange();
	}
});

function pointerEnd(e) {
	ptrs.delete(e.pointerId);
	if (mode === 'gesture') {
		if (ptrs.size < 2) {
			/* 倍率は整数に寄せる（ドットがそろって見える） */
			const nz = Math.max(1, Math.round(S.z));
			if (nz !== S.z) {
				zoomAt(gest.mx, gest.my, nz);
			}
			gest = null;
			mode = ptrs.size ? 'wait' : null;
		}
		return;
	}
	if (ptrs.size) {
		return;
	}
	if (mode === 'draw') {
		endStroke();
	} else if (mode === 'marquee') {
		if (!mq.moved) {
			S.sel = null;
		}
		requestDraw();
	}
	mode = null;
}
view.addEventListener('pointerup', pointerEnd);
view.addEventListener('pointercancel', pointerEnd);
view.addEventListener('pointerleave', function (e) {
	if (e.pointerType === 'mouse') {
		S.hover = null;
		requestDraw();
	}
});
view.addEventListener('contextmenu', function (e) { e.preventDefault(); });
view.addEventListener('wheel', function (e) {
	e.preventDefault();
	const p = pos(e);
	stepZoom(e.deltaY < 0 ? 1 : -1, p.px, p.py);
}, { passive: false });

/* ============ 範囲選択の操作 ============ */

function copySel() {
	if (S.float) {
		S.clip = { buf: S.float.buf.slice(), w: S.float.w, h: S.float.h };
	} else if (S.sel) {
		S.clip = { buf: getRegion(S.data, S.w, S.h, S.sel.x, S.sel.y, S.sel.w, S.sel.h), w: S.sel.w, h: S.sel.h };
	} else {
		toast('先に「範囲選択」で四角を選んでね');
		return false;
	}
	toast('コピーしました');
	return true;
}
function delSel() {
	if (S.float) {
		S.float = null;
		onChange();
	} else if (S.sel) {
		pushHistory();
		fillRect(S.data, S.w, S.h, S.sel.x, S.sel.y, S.sel.w, S.sel.h, S.task.transparent);
		onChange();
	} else {
		toast('先に「範囲選択」で四角を選んでね');
	}
}
function cutSel() {
	if (copySel()) {
		delSel();
	}
}
function pasteSel() {
	if (!S.clip) {
		toast('先に「コピー」してね');
		return;
	}
	setTool('select');
	commitFloat();
	pushHistory();
	const x = S.sel ? S.sel.x : Math.max(0, Math.floor(-S.ox / S.z));
	const y = S.sel ? S.sel.y : Math.max(0, Math.floor(-S.oy / S.z));
	S.float = { buf: S.clip.buf.slice(), x: x, y: y, w: S.clip.w, h: S.clip.h };
	S.sel = { x: x, y: y, w: S.clip.w, h: S.clip.h };
	onChange();
	toast('ドラッグで動かせます。Enter か、ほかをクリックで置きます');
}

/* ============ キーボード ============ */

function inField(e) {
	const t = e.target;
	return t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable);
}
window.addEventListener('keydown', function (e) {
	if (inField(e)) {
		return;
	}
	const mod = e.ctrlKey || e.metaKey;
	const k = e.key.toLowerCase();
	if (mod) {
		if (k === 'z') {
			e.preventDefault();
			if (e.shiftKey) { redo(); } else { undo(); }
		} else if (k === 'y') {
			e.preventDefault();
			redo();
		} else if (k === 'c') {
			e.preventDefault();
			copySel();
		} else if (k === 'x') {
			e.preventDefault();
			cutSel();
		} else if (k === 'v') {
			e.preventDefault();
			pasteSel();
		} else if (k === 's') {
			e.preventDefault();
			save(false);
		}
		return;
	}
	if (k === ' ') {
		e.preventDefault();
		S.spaceDown = true;
		view.classList.add('hand');
		return;
	}
	const map = { b: 'pen', p: 'pen', e: 'eraser', i: 'picker', f: 'fill', m: 'select', s: 'select', h: 'hand' };
	if (map[k]) {
		setTool(map[k]);
	} else if (k === 'g') {
		toggleGrid();
	} else if (k === '+' || k === '=') {
		stepZoom(1);
	} else if (k === '-') {
		stepZoom(-1);
	} else if (k === 'delete' || k === 'backspace') {
		e.preventDefault();
		delSel();
	} else if (k === 'enter') {
		commitFloat();
	} else if (k === 'escape') {
		commitFloat();
		S.sel = null;
		requestDraw();
	} else if (k.indexOf('arrow') === 0 && S.float) {
		e.preventDefault();
		const d = { arrowleft: [-1, 0], arrowright: [1, 0], arrowup: [0, -1], arrowdown: [0, 1] }[k];
		S.float.x += d[0];
		S.float.y += d[1];
		S.sel = { x: S.float.x, y: S.float.y, w: S.float.w, h: S.float.h };
		onChange();
	}
});
window.addEventListener('keyup', function (e) {
	if (e.key === ' ') {
		S.spaceDown = false;
		view.classList.toggle('hand', S.tool === 'hand');
	}
});

function toggleGrid() {
	S.grid = !S.grid;
	$('grid').classList.toggle('active', S.grid);
	requestDraw();
}

/* ============ 端末への自動保存 ============ */

function draftKey() {
	return 'kobo_draft:' + S.task.id + ':' + (S.workId || ('new-' + S.from));
}
function saveLocal() {
	if (!S.task || S.localRev === S.rev) {
		return;
	}
	try {
		const comp = composite();
		window.localStorage.setItem(draftKey(), JSON.stringify({
			w: S.w, h: S.h, d: u8ToB64(comp), at: Date.now(), parent: S.parent,
			note: $('note').value, share: $('share').checked, anon: anonValue()
		}));
		S.localRev = S.rev;
		$('autosave').textContent = 'この端末に自動保存しました（' + clock() + '）';
	} catch (e) {
		$('autosave').textContent = 'この端末への自動保存ができませんでした。';
	}
}
function readLocal() {
	try {
		const t = window.localStorage.getItem(draftKey());
		if (!t) {
			return null;
		}
		const d = JSON.parse(t);
		if (d.w !== S.w || d.h !== S.h || !d.d) {
			return null;
		}
		d.u8 = b64ToU8(d.d);
		return d.u8.length === S.w * S.h ? d : null;
	} catch (e) {
		return null;
	}
}
function clearLocal(key) {
	lsDel(key || draftKey());
}

/* ============ サーバーへの保存 ============ */

function anonValue() {
	const r = document.querySelector('input[name="anon"]:checked');
	return r ? r.value === '1' : false;
}

const NG_TEXT = {
	invalid_token: 'ログインが確かめられませんでした。ゲームでログインし直してね。',
	no_char: '先にゲームでキャラを作ってね。',
	too_many: 'これ以上は保存できません（作業中は 1 お題に 3 つまで、新しい案は 1 日 30 件までです）。',
	closed: 'このお題は今は募集していません。',
	agree: '同意にチェックを入れてね。',
	bad_png: '絵をうまく送れませんでした。読み込み直してね。',
	bad_size: '絵の大きさがお題と合いません。',
	bad_palette: '絵の色がお題のパレットと合いません。',
	long: 'ひとことが長すぎます（200 字まで）。',
	not_found: '元の案が見つかりませんでした（消されたのかもしれません）。',
	not_allowed: 'その操作はできません（出した案は変えられません）。',
	empty: '絵が空です。何か描いてから出してね。'
};
function errText(data) {
	if (data.httpStatus === 401) {
		return NG_TEXT.invalid_token;
	}
	if (data.error === 'wait') {
		return '続けて保存するときは、あと ' + (data.waitSeconds || 20) + ' 秒ほど待ってね。';
	}
	return NG_TEXT[data.error] || 'うまく保存できませんでした。時間をおいて試してね。';
}

async function save(bSubmit, bAuto) {
	if (!S.canSave) {
		if (!bAuto) {
			setMsg('保存して出すには、ゲームにログインしてキャラを作ってね。（絵はこの端末に自動で残ります）');
		}
		return false;
	}
	if (S.busy) {
		return false;
	}
	if (bAuto && Date.now() < S.coolUntil) {
		return false;
	}
	commitFloat();
	const note = $('note').value;
	if (Array.from(note).length > 200) {
		setMsg(NG_TEXT.long);
		return false;
	}
	if (bSubmit) {
		if (!$('agree').checked) {
			setMsg(NG_TEXT.agree);
			$('agree').scrollIntoView({ block: 'center' });
			return false;
		}
		if (!window.confirm('この絵を「出す」と、案として固定されて、あとから変えられません。\nみんなが見て、描き足しの元にできます。\n\n出しますか？')) {
			return false;
		}
	}
	S.busy = true;
	$('save-wip').disabled = true;
	$('submit').disabled = true;
	if (!bAuto) {
		setMsg(bSubmit ? '出しています...' : '保存しています...', true);
	}
	const rev = S.rev;
	try {
		const bytes = await encodeIndexedPng({
			width: S.w, height: S.h, indices: S.data,
			palette: S.task.palette.map(function (c) { return { r: c[0], g: c[1], b: c[2] }; })
		});
		const body = {
			token: S.token, taskId: S.task.id, data: u8ToB64(bytes), note: note,
			anonymous: anonValue(), share: $('share').checked, submit: !!bSubmit, agree: $('agree').checked
		};
		if (S.workId) {
			body.workId = S.workId;
		} else {
			body.parent = S.parent;
		}
		const data = await api('/api/kobo/save', body);
		if (!data.ok) {
			if (data.error === 'wait') {
				S.coolUntil = Date.now() + (data.waitSeconds || 20) * 1000;
			}
			if (bAuto) {
				$('autosave').textContent = 'サーバーへの自動保存はあとでやり直します（' + errText(data) + '）';
			} else {
				setMsg(errText(data));
			}
			return false;
		}
		const oldKey = draftKey();
		if (data.work && data.work.id && data.work.id !== S.workId) {
			S.workId = data.work.id;
			clearLocal(oldKey);
			const q = new URLSearchParams(location.search);
			q.delete('from');
			q.set('work', S.workId);
			history.replaceState(null, '', location.pathname + '?' + q.toString());
		}
		S.coolUntil = Date.now() + 20000;
		if (bSubmit) {
			clearLocal();
			S.closed = true;
			location.href = '/kobo#task=' + encodeURIComponent(S.task.id) + '&work=' + encodeURIComponent(S.workId);
			return true;
		}
		S.serverRev = rev;
		S.localRev = -1;
		saveLocal();
		const t = (bAuto ? '自動で' : '') + '作業中として保存しました（' + clock() + '）。' +
			($('share').checked ? 'みんなに見えています。' : '自分だけに見えています。');
		if (bAuto) {
			$('autosave').textContent = t;
		} else {
			setMsg(t, true);
		}
		return true;
	} catch (e) {
		if (bAuto) {
			$('autosave').textContent = 'サーバーにつながりませんでした。あとでやり直します。';
		} else {
			setMsg('サーバーにつながりませんでした。（絵はこの端末に残っています）');
		}
		return false;
	} finally {
		S.busy = false;
		$('save-wip').disabled = !S.canSave;
		$('submit').disabled = !S.canSave;
	}
}

/* ============ 開く・つなぎ合わせる ============ */

function fromImage(img) {
	if (img.width !== S.w || img.height !== S.h) {
		throw new Error('size');
	}
	return Uint8Array.from(img.indices);
}

async function boot() {
	const q = new URLSearchParams(location.search);
	const taskId = q.get('task');
	S.from = q.get('from') || 'base';
	const workParam = q.get('work') || '';
	S.token = lsGet(KEY_TOKEN) || '';
	if (!taskId) {
		fatal('お題が指定されていません。');
		return;
	}
	let td, me = null;
	try {
		const r = await Promise.all([
			api('/api/kobo/task', { id: taskId, token: S.token || undefined }),
			S.token ? api('/api/kobo/me', { token: S.token }).catch(function () { return null; }) : Promise.resolve(null)
		]);
		td = r[0];
		me = r[1];
	} catch (e) {
		fatal('サーバーにつながりませんでした。少し待ってから読み込み直してね。');
		return;
	}
	if (!td.task) {
		fatal('このお題は見つかりませんでした。');
		return;
	}
	const task = td.task;
	S.task = task;
	S.w = task.w;
	S.h = task.h;
	S.me = me && me.account ? me : null;
	pal = task.palette.map(function (c, i) { return { r: c[0], g: c[1], b: c[2], a: i === task.transparent ? 0 : 255 }; });
	imgCanvas.width = S.w;
	imgCanvas.height = S.h;

	/* 最初の絵 */
	let initial, note = '', share = false, anon = false;
	try {
		if (workParam) {
			const w = (td.works || []).find(function (x) { return x.id === workParam; });
			if (!w || !w.mine || w.status !== 'wip') {
				fatal('その作業中の案は見つかりませんでした（出した後・消した後かもしれません）。');
				return;
			}
			initial = fromImage(await loadIndexed(w.img));
			S.workId = w.id;
			S.parent = w.parent || '';
			note = w.note || '';
			share = !!w.share;
			anon = !!w.anonymous;
		} else if (S.from === 'blank' || (S.from === 'base' && !task.base)) {
			/* 新しく描くお題には「今の絵」が無いので、まっさらから */
			initial = new Uint8Array(S.w * S.h).fill(task.transparent);
		} else if (S.from === 'base') {
			initial = fromImage(await loadIndexed(task.base));
		} else {
			const w = (td.works || []).find(function (x) { return x.id === S.from && !x.deleted; });
			if (!w) {
				fatal('元にする案が見つかりませんでした（消されたのかもしれません）。');
				return;
			}
			initial = fromImage(await loadIndexed(w.img));
			S.parent = w.id;
		}
	} catch (e) {
		fatal(e.message === 'size' ? '絵の大きさがお題と合いません。' : '絵を読み込めませんでした。');
		return;
	}
	S.data = initial;
	$('note').value = note;
	$('share').checked = share;
	(document.querySelector('input[name="anon"][value="' + (anon ? '1' : '0') + '"]') || {}).checked = true;

	/* この端末に残っている途中の絵 */
	const draft = readLocal();
	if (draft) {
		let diff = false;
		for (let i = 0; i < draft.u8.length; i++) {
			if (draft.u8[i] !== initial[i]) { diff = true; break; }
		}
		if (diff && window.confirm('この端末に、前に描いていた途中の絵が残っています（' + new Date(draft.at).toLocaleString('ja-JP') + '）。\n続きから開きますか？\n（「キャンセル」で、最初の絵から始めます）')) {
			S.data = draft.u8;
			$('note').value = draft.note || '';
			$('share').checked = !!draft.share;
			(document.querySelector('input[name="anon"][value="' + (draft.anon ? '1' : '0') + '"]') || {}).checked = true;
		} else if (diff) {
			clearLocal();
		}
	}

	setupUi();
	$('loading').hidden = true;
	resize();
	fit();
	S.rev = 1;
	S.serverRev = 1;
	S.localRev = 1;
	onChange();
	S.serverRev = S.rev;
	S.localRev = S.rev;
	if (draft && S.data === draft.u8) {
		S.serverRev = -1;
	}
}

function setupUi() {
	const task = S.task;
	$('title').textContent = task.title;
	const sm = el('small', '', task.w + '×' + task.h);
	$('title').appendChild(sm);
	document.title = task.title + '｜描く｜ドット絵工房';

	/* 保存できるか */
	const banner = $('banner');
	S.canSave = !!(S.me && S.me.canPost) && task.state === 'open';
	if (task.state !== 'open') {
		banner.hidden = false;
		banner.textContent = 'このお題は今は募集していません。絵は描けますが、保存して出すことはできません。';
	} else if (!S.token || !S.me) {
		banner.hidden = false;
		banner.textContent = '';
		banner.appendChild(document.createTextNode('ログインしていないので、保存して出すことはできません。絵はこの端末に自動で残ります。'));
		const a = el('a', '', ' ゲームを開く');
		a.href = '/';
		banner.appendChild(a);
	} else if (!S.me.canPost) {
		banner.hidden = false;
		banner.textContent = '先にゲームでキャラを作ると、保存して出せます。絵はこの端末に自動で残ります。';
	}
	$('save-wip').disabled = !S.canSave;
	$('submit').disabled = !S.canSave;
	$('my-name').textContent = S.me ? '「' + (S.me.name || '') + '」' : '';

	/* 元イラスト・見本 */
	const refs = $('refs');
	if (task.ref) {
		const a = el('a');
		a.href = imgUrl(task.ref);
		a.target = '_blank';
		a.rel = 'noopener';
		const im = refImgEl(task.ref);
		im.addEventListener('error', function () { a.href = imgUrl(task.ref, 'jpg'); });
		a.appendChild(im);
		refs.appendChild(a);
	} else {
		refs.appendChild(el('p', 'note', '元イラストはありません。'));
	}
	const smp = $('samples');
	(task.samples || []).concat(task.base ? [task.base] : []).forEach(function (id, n, arr) {
		const wrap = el('div', 'checker');
		const cv = thumbCanvas(task, id, Math.max(1, Math.floor(300 / task.w)));
		cv.title = (task.base && n === arr.length - 1) ? '今の絵' : '見本';
		wrap.appendChild(cv);
		smp.appendChild(wrap);
	});
	if (!smp.firstChild) {
		smp.appendChild(el('p', 'note', '見本はありません。'));
	}

	/* プレビュー */
	pvMain = makePreview($('preview'), task, { scale: 2 });
	if (window.matchMedia('(max-width: 860px)').matches) {
		pvMini = makePreview($('mini'), task, { scale: 1, compact: true });
	}
	Array.prototype.forEach.call(document.querySelectorAll('[data-pvs]'), function (b) {
		b.addEventListener('click', function () {
			Array.prototype.forEach.call(document.querySelectorAll('[data-pvs]'), function (x) { x.classList.toggle('active', x === b); });
			pvMain.setScale(+b.dataset.pvs);
			S.imgDirty = true;
			requestDraw();
		});
	});

	buildPalette();
	const first = task.colors.find(function (i) { return i !== task.transparent; });
	setColor(first != null ? first : task.colors[0]);
	setTool('pen');
	$('grid').classList.toggle('active', S.grid);
	updateUndoButtons();
}

/* ---- ボタン ---- */
Array.prototype.forEach.call(document.querySelectorAll('#tools button'), function (b) {
	b.addEventListener('click', function () { setTool(b.dataset.tool); });
});
$('undo').addEventListener('click', undo);
$('redo').addEventListener('click', redo);
$('copy').addEventListener('click', copySel);
$('cut').addEventListener('click', cutSel);
$('paste').addEventListener('click', pasteSel);
$('del').addEventListener('click', delSel);
$('grid').addEventListener('click', toggleGrid);
$('fit').addEventListener('click', fit);
$('zin').addEventListener('click', function () { stepZoom(1); });
$('zout').addEventListener('click', function () { stepZoom(-1); });
$('save-wip').addEventListener('click', function () { save(false, false); });
$('submit').addEventListener('click', function () { save(true, false); });
$('back').addEventListener('click', function () {
	if (!S.task) {
		location.href = '/kobo';
		return;
	}
	saveLocal();
	if (S.canSave && S.serverRev !== S.rev && !window.confirm('サーバーに保存していない変更があります（この端末には残ります）。\nお題のページへ戻りますか？')) {
		return;
	}
	S.closed = true;
	location.href = '/kobo#task=' + encodeURIComponent(S.task.id);
});
function openSide(secId) {
	$('side').classList.add('open');
	const s = $(secId);
	if (s) {
		$('side').scrollTop = s.offsetTop - 8;
	}
}
$('open-ref').addEventListener('click', function () { openSide('sec-refs'); });
$('open-save').addEventListener('click', function () { openSide('sec-save'); });
$('close-side').addEventListener('click', function () { $('side').classList.remove('open'); });
$('mini').addEventListener('click', function () { openSide('sec-preview'); });

window.addEventListener('resize', resize);
if (window.ResizeObserver) {
	new ResizeObserver(resize).observe($('stage'));
}

/* ひとこと・名前・見せるの変更も「変わった」として数える（自動保存の対象） */
['note', 'share'].forEach(function (id) {
	$(id).addEventListener('input', function () { S.rev++; });
	$(id).addEventListener('change', function () { S.rev++; });
});
Array.prototype.forEach.call(document.querySelectorAll('input[name="anon"]'), function (r) {
	r.addEventListener('change', function () { S.rev++; });
});

/* 自動保存 */
setInterval(saveLocal, LOCAL_SAVE_MS);
setInterval(function () {
	/* サーバーへは「みんなに見せる」がオンで、前の保存から変わっている時だけ */
	if (S.task && $('share').checked && S.serverRev !== S.rev && !mode) {
		save(false, true);
	}
}, SERVER_SAVE_MS);
document.addEventListener('visibilitychange', function () {
	if (document.visibilityState === 'hidden') {
		saveLocal();
	}
});
window.addEventListener('pagehide', saveLocal);
window.addEventListener('beforeunload', function (e) {
	if (S.task && !S.closed && S.canSave && S.serverRev !== S.rev) {
		saveLocal();
		e.preventDefault();
		e.returnValue = '';
	}
});

boot();
