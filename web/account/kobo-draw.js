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
import { encodeIndexedPng, decodeIndexedPng, indicesToImageData } from '/dot-png.js';
import { installTouch } from '/kobo-touch.js';

const $ = function (id) { return document.getElementById(id); };
const UNDO_MAX = 50;
const KEY_PV_PLAY = 'sbo_kobo_pv_play';		// プレビューを再生するか（'1' = 再生）
const KEY_SMP_ZOOM = 'sbo_kobo_smp_zoom';	// 見本の倍率
const KEY_REF_ZOOM = 'sbo_kobo_ref_zoom';	// 元イラストの倍率（0 = 欄の幅に合わせる）
const LOCAL_SAVE_MS = 3000;		// 端末への自動保存
const SERVER_SAVE_MS = 60000;	// サーバーへの作業中保存（「みんなに見せる」がオンの時だけ）
/* 拡大・スクロールは管理者用ドット絵エディタ（dot-editor.js）と同じ決まりにそろえる */
const PAD = 32;					// 絵のまわりの余白(px)。端まで画面の中央に持ってこられるように
const ZOOMS = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32];
const WHEEL_ZOOM_RATE = 0.002;	// Cmd+ホイール：回転量 1px あたり exp(量 × これ) 倍
const PINCH_ZOOM_RATE = 0.01;	// ピンチ（Mac の Chrome では Ctrl 付きのホイール）
const ZOOM_SNAP = 0.04;			// 整数倍にこれより近ければ吸い付ける
const WHEEL_ZOOM_STEP = 100;	// Cmd+Shift+ホイール：1 段階に要る回転量
const PINCH_ZOOM_STEP = 30;
const WHEEL_ZOOM_SPEEDS = { slow: 2.5, normal: 1, fast: 0.4 };
const WHEEL_ZOOM_COOLDOWN = 180;
const WHEEL_ZOOM_IDLE = 500;

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
	/* 「出す」の確認を開いている時はそちらにも */
	$('dlg-msg').textContent = text || '';
	$('dlg-msg').style.color = bOk ? 'var(--ok)' : '';
	if (text && !bOk && $('submit-dlg').hidden) {
		toast(text);
	}
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
	if (S.task && S.w) {
		clampView();
	}
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
	clampView();
	syncZoomSel();
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
		w + '×' + h;
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
	const zf = Math.min((vw - PAD * 2) / S.w, (vh - PAD * 2) / S.h);
	let z = ZOOMS[0];
	ZOOMS.forEach(function (v) { if (v <= zf) { z = v; } });
	S.z = z;
	S.ox = Math.round((vw - S.w * z) / 2);
	S.oy = Math.round((vh - S.h * z) / 2);
	requestDraw();
}

/**
 * 絵の位置をエディタのスクロールと同じ範囲に収める。
 * 絵（＋余白）が画面より小さい向きは真ん中、大きい向きは余白 PAD まで動かせる。
 */
function clampView() {
	const iw = S.w * S.z, ih = S.h * S.z;
	if (iw + PAD * 2 <= vw) {
		S.ox = (vw - iw) / 2;
	} else {
		S.ox = clamp(S.ox, vw - iw - PAD, PAD);
	}
	if (ih + PAD * 2 <= vh) {
		S.oy = (vh - ih) / 2;
	} else {
		S.oy = clamp(S.oy, vh - ih - PAD, PAD);
	}
}

/** 絵の (ix, iy) が画面の (sx, sy) に来るように倍率を変える */
function setZoom(nz, ix, iy, sx, sy) {
	nz = Math.round(clamp(nz, ZOOMS[0], ZOOMS[ZOOMS.length - 1]) * 100) / 100;
	S.z = nz;
	S.ox = sx - ix * nz;
	S.oy = sy - iy * nz;
	requestDraw();
}

/** (px, py)（画面上の点）を動かさずに倍率を変える */
function zoomAt(px, py, nz) {
	setZoom(nz, (px - S.ox) / S.z, (py - S.oy) / S.z, px, py);
}

/** 1, 2, 3, 4, 6, 8 … の決まった段階で 1 つ上・下 */
function nextZoom(dir) {
	if (dir > 0) {
		const up = ZOOMS.find(function (v) { return v > S.z; });
		return up == null ? S.z : up;
	}
	const down = ZOOMS.slice().reverse().find(function (v) { return v < S.z; });
	return down == null ? S.z : down;
}

function stepZoom(dir, px, py) {
	zoomAt(px == null ? vw / 2 : px, py == null ? vh / 2 : py, nextZoom(dir));
}

/* 画面右下の倍率一覧（一覧にない倍率の時は、その項目を一時的に足す） */
function zoomLabel(z) {
	return (Number.isInteger(z) ? String(z) : z.toFixed(2).replace(/0$/, '')) + '倍';
}
function syncZoomSel() {
	const sel = $('zoomSel');
	const key = String(S.z);
	if (!sel || sel.dataset.cur === key) {
		return;
	}
	sel.dataset.cur = key;
	const list = ZOOMS.indexOf(S.z) >= 0 ? ZOOMS : ZOOMS.concat([S.z]).sort(function (a, b) { return a - b; });
	sel.textContent = '';
	list.forEach(function (v) {
		const o = el('option', '', zoomLabel(v));
		o.value = String(v);
		sel.appendChild(o);
	});
	sel.value = key;
}

/* ホイールの速さは管理者用エディタで選んだものを使う（同じ端末なら同じ設定） */
function wheelSpeed() {
	let v = 'normal';
	try {
		v = window.localStorage.getItem('sbo-dot-wheel-zoom-speed') || 'normal';
	} catch (e) { /* 読めなければふつう */ }
	return WHEEL_ZOOM_SPEEDS[v] || 1;
}
function wheelDeltaPx(e) {
	const unit = e.deltaMode === 1 ? 16 : e.deltaMode === 2 ? vh : 1;
	const d = Math.abs(e.deltaY) >= Math.abs(e.deltaX) ? e.deltaY : e.deltaX;
	return d * unit;
}
/* 回転量をためて、一定量を超えたら拡大(1)・縮小(-1)。まだなら 0 */
const wheelAcc = { sum: 0, last: 0, stepAt: 0 };
function wheelZoomDir(e) {
	const now = performance.now();
	const dy = wheelDeltaPx(e);
	if (now - wheelAcc.last > WHEEL_ZOOM_IDLE || Math.sign(dy) !== Math.sign(wheelAcc.sum)) {
		wheelAcc.sum = 0;
	}
	wheelAcc.last = now;
	wheelAcc.sum += dy;
	if (now - wheelAcc.stepAt < WHEEL_ZOOM_COOLDOWN) {
		return 0;
	}
	const step = ((e.ctrlKey && !e.metaKey) ? PINCH_ZOOM_STEP : WHEEL_ZOOM_STEP) * wheelSpeed();
	if (Math.abs(wheelAcc.sum) < step) {
		return 0;
	}
	const dir = wheelAcc.sum < 0 ? 1 : -1;
	wheelAcc.sum = 0;
	wheelAcc.stepAt = now;
	return dir;
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
		$('cur-text').textContent = '番号 ' + i + '  (' + c[0] + ', ' + c[1] + ', ' + c[2] + ')';
	}
	if (S.tool === 'eraser' || S.tool === 'picker') {
		setTool(S.tool === 'picker' ? S.prevTool : 'pen');
	}
}

/*
 * パレット
 *   見本の色 … 見本（描き直しなら今の絵も）で使っている色を先に 8 列で（絵柄合わせの目安）
 *   パレット … 管理者用エディタと同じく 256 色を 16×16 に番号順で。色はそのまま見せ、
 *              使えない色（透明以外の真っ黒。ゲームではどれも透明になる）は ✗ を付けて押せない
 */
function paletteButton(i, usable) {
	const c = S.task.palette[i] || [0, 0, 0];
	const ok = usable.has(i);
	const black = !c[0] && !c[1] && !c[2];
	const b = el('button', 'sw' + (black ? ' black' : '') + (ok ? ' use' : ' off'));
	b.type = 'button';
	b.dataset.i = i;
	b.title = '番号 ' + i + (i === S.task.transparent ? '（透明）' : '  (' + c[0] + ', ' + c[1] + ', ' + c[2] + ')') + (ok ? '' : '　使えません（ゲームでは透明になる黒など）');
	b.style.background = 'rgb(' + c[0] + ',' + c[1] + ',' + c[2] + ')';
	if (ok) {
		b.addEventListener('click', function () { setColor(i); });
	} else {
		b.disabled = true;
	}
	return b;
}
function buildPalette() {
	const box = $('palette');
	box.textContent = '';
	const usable = new Set(S.task.colors);
	const smp = (S.task.sampleColors || S.sampleColorsLocal || []).filter(function (i) { return usable.has(i); });
	if (!smp.length && !S.sampleColorsLocal && (S.task.samples || []).length) {
		/* 古いお題（sampleColors が無い・空）は、見本の絵から数えて並べ直す */
		S.sampleColorsLocal = [];
		Promise.all(S.task.samples.map(loadIndexed)).then(function (imgs) {
			const set = new Set();
			imgs.forEach(function (img) { img.indices.forEach(function (i) { set.add(i); }); });
			S.sampleColorsLocal = Array.from(set).filter(function (i) { return i !== S.task.transparent; }).sort(function (a, b) { return a - b; });
			if (S.sampleColorsLocal.length) {
				S.task.sampleColors = S.sampleColorsLocal;
				buildPalette();
				setColor(S.color);
			}
		}).catch(function () { /* 読めなければ見本の色は出さない */ });
	}
	if (smp.length) {
		box.appendChild(el('div', 'palhead', '見本の色'));
		const g1 = el('div', 'palgrid sample');
		smp.forEach(function (i) { g1.appendChild(paletteButton(i, usable)); });
		box.appendChild(g1);
		box.appendChild(el('div', 'palhead', 'パレット'));
	}
	const g2 = el('div', 'palgrid');
	for (let i = 0; i < 256; i++) {
		g2.appendChild(paletteButton(i, usable));
	}
	box.appendChild(g2);
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

function startGesture(pts) {
	const a = pts || Array.from(ptrs.values());
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
	if (e.pointerType === 'touch') {
		return;		/* 指は下の installTouch で */
	}
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
		if ((e.metaKey || e.ctrlKey) && (e.button === 0 || e.button === 2)) {
			/* Cmd+左 = 拡大（押した点が真ん中に来る）、Cmd+右 = 縮小 */
			e.preventDefault();
			setZoom(nextZoom(e.button === 0 ? 1 : -1), (p.px - S.ox) / S.z, (p.py - S.oy) / S.z, vw / 2, vh / 2);
			mode = 'wait';
			return;
		}
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
	if (e.pointerType === 'touch') {
		return;
	}
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
		gestureMove(Array.from(ptrs.values()));
		return;
	}
	if (!rec || ptrs.size > 1) {
		return;
	}
	moveTo(p);
});

/* 2 本指のピンチ・スクロール（点は {x, y} の client 座標） */
function gestureMove(a) {
	{
		const r = view.getBoundingClientRect();
		const mx = (a[0].x + a[1].x) / 2 - r.left;
		const my = (a[0].y + a[1].y) / 2 - r.top;
		const d = Math.hypot(a[0].x - a[1].x, a[0].y - a[1].y);
		const nz = clamp(gest.z * d / gest.d, ZOOMS[0], ZOOMS[ZOOMS.length - 1]);
		const ix = (gest.mx - gest.ox) / gest.z;
		const iy = (gest.my - gest.oy) / gest.z;
		S.z = nz;
		S.ox = mx - ix * nz;
		S.oy = my - iy * nz;
		requestDraw();
	}
}

/* 押したまま動かした時（ペン・マウス・指で共通） */
function moveTo(p) {
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
}

function pointerEnd(e) {
	if (e.pointerType === 'touch') {
		return;
	}
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
	finishTool();
}
/* 指・ボタンを離した時 */
function finishTool() {
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
/* gesture の倍率を整数に寄せて終わる */
function endGesture() {
	if (gest) {
		const nz = Math.max(1, Math.round(S.z));
		if (nz !== S.z) {
			zoomAt(gest.mx, gest.my, nz);
		}
	}
	gest = null;
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
/*
 * スマホ: 1 本指で描く・2 本指でスクロールと拡大縮小・長押しで色を拾う（kobo-touch.js）。
 * 触れてから 80ms は描かずに待ち、その間に 2 本目が来たらスクロールにする。
 */
function touchPos(q) {
	return pos({ clientX: q.x, clientY: q.y });
}
installTouch(view, {
	dragStart: function (pts) {
		startTool(touchPos(pts[0]));
		pts.slice(1).forEach(function (q) { moveTo(touchPos(q)); });
	},
	dragMove: function (q) {
		moveTo(touchPos(q));
	},
	dragEnd: function () {
		finishTool();
	},
	dragCancel: function () {
		cancelCurrent();
	},
	tap: function (q) {
		const p = touchPos(q);
		startTool(p);
		finishTool();
	},
	longPress: function (q) {
		const p = touchPos(q);
		pickAt(p.x, p.y);
	},
	multiStart: function (pts) {
		cancelCurrent();
		startGesture(pts);
	},
	multiMove: function (pts) {
		if (mode === 'gesture') {
			gestureMove(pts);
		}
	},
	multiEnd: function () {
		endGesture();
	}
});
/*
 * ホイール（エディタと同じ）
 *   そのまま: スクロール（Shift で横）
 *   Cmd+ホイール: カーソルの位置を中心に細かく拡大縮小（整数倍に吸い付く）。ピンチも
 *   Cmd+Shift+ホイール: 1, 2, 3, 4, 6, 8 … の段階で 1 つずつ
 */
view.addEventListener('wheel', function (e) {
	e.preventDefault();
	const p = pos(e);
	if (!(e.metaKey || e.ctrlKey)) {
		const unit = e.deltaMode === 1 ? 16 : e.deltaMode === 2 ? vh : 1;
		let dx = e.deltaX * unit, dy = e.deltaY * unit;
		if (e.shiftKey && !dx) {
			dx = dy;
			dy = 0;
		}
		S.ox -= dx;
		S.oy -= dy;
		requestDraw();
		return;
	}
	if (e.shiftKey) {
		const dir = wheelZoomDir(e);
		if (dir) {
			stepZoom(dir, p.px, p.py);
		}
		return;
	}
	const dy = wheelDeltaPx(e);
	const pinch = e.ctrlKey && !e.metaKey;
	let z = S.z * Math.exp(-dy * (pinch ? PINCH_ZOOM_RATE : WHEEL_ZOOM_RATE) / wheelSpeed());
	const near = Math.round(z);
	if (near >= 1 && Math.abs(z - near) < ZOOM_SNAP * near && Math.abs(S.z - near) >= 0.005) {
		/* 整数倍をまたいだ・近づいた時は、いったん整数倍で止める */
		const crossed = (S.z - near) * (z - near) <= 0;
		if (crossed || Math.abs(z - near) < Math.abs(S.z - near)) {
			z = near;
		}
	}
	zoomAt(p.px, p.py, z);
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
	if (!$('submit-dlg').hidden) {
		/* 確認の窓を開いている間は、描く画面のキー操作は効かせない */
		if (e.key === 'Escape') {
			closeSubmit();
		}
		return;
	}
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
	if (S.admin) {
		/* 管理人の直しはプレイヤーの「続きを描く」に出さない */
		/* 土台の絵が変わったら（提案者が描き直した等）、前の描きかけは使わない */
		return 'kobo_admin_draft:' + S.admin + ':' + S.task.id + ':' + (S.adminTarget || '') + (S.adminSrc ? ':' + S.adminSrc : '');
	}
	return 'kobo_draft:' + S.task.id + ':' + (S.workId || ('new-' + S.from));
}
function saveLocal() {
	if (!S.task || S.localRev === S.rev) {
		return;
	}
	if (S.propose) {
		try {
			writeProposeDraft();
		} catch (e) { /* 次にまた試す */ }
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
	if (S.admin) {
		return bSubmit && !bAuto ? adminSave() : false;
	}
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
	if (bSubmit && !$('agree').checked) {
		setMsg(NG_TEXT.agree);
		return false;
	}
	S.busy = true;
	$('save-wip').disabled = true;
	$('submit').disabled = true;
	$('dlg-submit').disabled = true;
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
			/* 出したら工房のトップへ（「出しました」と出した案へのリンクを出す） */
			location.href = '/kobo#posted=' + encodeURIComponent(S.task.id) + '&work=' + encodeURIComponent(S.workId);
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
			toast('作業中として保存しました');
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
		$('dlg-submit').disabled = !$('agree').checked;
	}
}

/* ============ 開く・つなぎ合わせる ============ */

function fromImage(img) {
	if (img.width !== S.w || img.height !== S.h) {
		throw new Error('size');
	}
	return Uint8Array.from(img.indices);
}

/*
 * 管理人が直す時（?admin=…。管理画面のログインのまま開く）
 *   admin=fix&task=ID&from=案id   … 案に描き足して「管理人」の案として出す（作監修正）
 *   admin=base&task=ID             … お題の今の絵を直す
 *   admin=sample&task=ID&n=番号    … お題の見本を直す
 *   admin=proposal&id=提案id        … 提案のラフ絵に描き足して返事に付ける
 */
const ADMIN_TEXT = {
	fix: { btn: '修正を出す', title: '管理人の修正として出す', note: '直したところ（200字まで）', dlg: '元の案の下に「管理人」の案として並びます。元の案はそのまま残ります。' },
	base: { btn: '保存', title: 'お題の今の絵を保存', note: 'メモ（保存されません）', dlg: 'お題の「今の絵」を入れ替えます。見本の色も作り直します。' },
	sample: { btn: '保存', title: 'お題の見本を保存', note: 'メモ（保存されません）', dlg: 'お題の見本を入れ替えます。見本の色も作り直します。' },
	proposal: { btn: '返事を送る', title: '絵を付けて返事', note: '', dlg: '提案した人の「自分の提案」に、返事と一緒にこの絵が出ます。' }
};
const PAD_SHEET = { char: 'char01.png', part: 'map01.png', effect: 'efc01.png' };	// kobo.js の提案のお絵かきと同じ

/* 色そのままの PNG を、パレットの近い色の番号にする（kobo.js と同じ） */
function rgbaToIndexed(buf, palette, transparent) {
	return new Promise(function (resolve, reject) {
		const url = URL.createObjectURL(new Blob([buf], { type: 'image/png' }));
		const im = new Image();
		im.onload = function () {
			URL.revokeObjectURL(url);
			const c = document.createElement('canvas');
			c.width = im.width;
			c.height = im.height;
			const cx = c.getContext('2d');
			cx.drawImage(im, 0, 0);
			const d = cx.getImageData(0, 0, c.width, c.height).data;
			const usable = [];
			palette.forEach(function (pc, i) {
				if (i !== transparent && (pc[0] || pc[1] || pc[2])) {
					usable.push(i);
				}
			});
			const memo = new Map();
			const out = new Uint8Array(c.width * c.height);
			for (let k = 0; k < out.length; k++) {
				const r = d[k * 4], g = d[k * 4 + 1], b = d[k * 4 + 2];
				if (d[k * 4 + 3] < 128) {
					out[k] = transparent;
					continue;
				}
				const key = (r << 16) | (g << 8) | b;
				let best = memo.get(key);
				if (best === undefined) {
					let bd = Infinity;
					usable.forEach(function (i) {
						const pc = palette[i];
						const dd = (pc[0] - r) * (pc[0] - r) + (pc[1] - g) * (pc[1] - g) + (pc[2] - b) * (pc[2] - b);
						if (dd < bd) {
							bd = dd;
							best = i;
						}
					});
					memo.set(key, best);
				}
				out[k] = best;
			}
			resolve({ width: c.width, height: c.height, indices: out });
		};
		im.onerror = function () {
			URL.revokeObjectURL(url);
			reject(new Error('img'));
		};
		im.src = url;
	});
}

async function fetchBuf(url) {
	const res = await fetch(url);
	if (!res.ok) {
		throw new Error('http');
	}
	return res.arrayBuffer();
}

/* 提案のお絵かきのパレット（種類ごとのゲーム画像）。使える色は透明以外の真っ黒を除く全部 */
async function padPalette(kind) {
	const sheet = await decodeIndexedPng(await fetchBuf('/admin-grp/' + encodeURIComponent(PAD_SHEET[kind] || PAD_SHEET.char)));
	const palette = [];
	for (let i = 0; i < 256; i++) {
		const c = sheet.palette[i];
		palette.push(c ? [c.r, c.g, c.b] : [0, 0, 0]);
	}
	let transparent = palette.findIndex(function (c) { return !c[0] && !c[1] && !c[2]; });
	if (transparent < 0) {
		transparent = 0;
	}
	const colors = [];
	palette.forEach(function (c, i) {
		if (i === transparent || c[0] || c[1] || c[2]) {
			colors.push(i);
		}
	});
	return { palette: palette, transparent: transparent, colors: colors };
}
function padTask(id, title, w, h, pt, ref) {
	return {
		id: id, title: title, state: 'open', w: w, h: h, cw: w, ch: h, preview: 'tile', frameMs: 200,
		palette: pt.palette, colors: pt.colors, transparent: pt.transparent, sampleColors: [], samples: [],
		base: '', bg: '', ref: ref || ''
	};
}

/*
 * 提案のラフ絵を描く（?propose=提案id か new）。受け渡しは kobo.js の提案の下書き
 * （localStorage kobo_propose_draft:…）で、描いている間もそこへ書き戻す。「完了」で提案の画面へ戻る
 */
const PROPOSE_DRAFT_PREFIX = 'kobo_propose_draft:';
function readProposeDraft() {
	try {
		return JSON.parse(window.localStorage.getItem(PROPOSE_DRAFT_PREFIX + S.propose) || 'null');
	} catch (e) {
		return null;
	}
}
function writeProposeDraft() {
	const d = readProposeDraft() || { kind: S.proposeKind, body: '', anon: false };
	commitFloat();
	const comp = composite();
	let changed = false, any = false;
	for (let i = 0; i < comp.length; i++) {
		if (comp[i] !== S.proposeInitial[i]) {
			changed = true;
		}
		if (comp[i] !== S.task.transparent) {
			any = true;
		}
	}
	d.at = Date.now();
	d.size = S.w + 'x' + S.h;
	d.px = u8ToB64(comp);
	d.drawn = any;
	if (changed) {
		/* 描き直したら、選んでいた画像・今のラフ絵の代わりにこの絵を送る */
		d.roughSrc = any ? 'pad' : '';
		d.fileData = '';
	}
	window.localStorage.setItem(PROPOSE_DRAFT_PREFIX + S.propose, JSON.stringify(d));
	S.localRev = S.rev;
}
function finishPropose() {
	try {
		writeProposeDraft();
	} catch (e) {
		setMsg('この端末に残せませんでした（ブラウザの保存領域がいっぱいかもしれません）。');
		return;
	}
	S.closed = true;
	location.href = '/kobo#propose=' + encodeURIComponent(S.propose);
}
/* 右の欄の「見本」を「管理人の返事の絵」にして、返事のことばと「この絵から描き始める」を添える */
function showProposeReply(pr) {
	const h4 = $('samples').previousElementSibling.previousElementSibling;
	if (h4 && h4.tagName === 'H4') {
		h4.textContent = '管理人の返事の絵';
		h4.appendChild(el('small', '', '見ながら描けます（クリックで色を拾う）'));
	}
	const box = el('div');
	if (pr.reply) {
		box.appendChild(el('p', 'note', '管理人より：' + pr.reply));
	}
	const b = el('button', '', 'この絵から描き始める');
	b.type = 'button';
	b.title = '今の絵を返事の絵に置きかえます（戻すで元に戻せます）';
	b.style.marginTop = '6px';
	b.addEventListener('click', function () {
		loadIndexed(pr.replyImg).then(function (img) {
			if (img.width !== S.w || img.height !== S.h) {
				toast('大きさが違うので置きかえられませんでした（' + img.width + '×' + img.height + '）');
				return;
			}
			commitFloat();
			pushHistory();
			S.data = Uint8Array.from(img.indices);
			onChange();
			toast('返事の絵から描き始めます');
		}).catch(function () { toast('返事の絵を読み込めませんでした'); });
	});
	box.appendChild(b);
	$('samples').after(box);
}

async function proposeBoot(q) {
	S.propose = q.get('propose');
	document.body.classList.add('admin-mode');		/* プレイヤーの保存・出すの欄は使わない */
	const d = readProposeDraft() || {};
	S.proposeKind = d.kind || 'char';
	const sz = String(d.size || '16x16').split('x');
	const w = Math.max(1, Math.min(64, Number(sz[0]) || 16)), h = Math.max(1, Math.min(64, Number(sz[1]) || 16));
	let pt;
	try {
		pt = await padPalette(S.proposeKind);
	} catch (e) {
		fatal('色を読み込めませんでした。');
		return;
	}
	const task = padTask('propose-' + S.propose, '提案のラフ絵', w, h, pt, '');
	let initial = new Uint8Array(w * h).fill(task.transparent);
	if (d.px) {
		try {
			const a = b64ToU8(d.px);
			if (a.length === w * h) {
				initial = a;
			}
		} catch (e) { /* 白紙から */ }
	}
	/* 自分の提案を直す時、管理人の返事の絵があれば見本の欄に出して、見ながら描けるようにする */
	let reply = null;
	const token = lsGet(KEY_TOKEN) || '';
	if (S.propose !== 'new' && token) {
		try {
			const r = await api('/api/kobo/my-proposals', { token: token });
			reply = (r.proposals || []).find(function (x) { return x.id === S.propose; }) || null;
		} catch (e) { /* 読めなければ出さない */ }
		if (reply && reply.replyImg) {
			task.samples = [reply.replyImg];
		} else {
			reply = null;
		}
	}
	S.task = task;
	S.w = w;
	S.h = h;
	S.me = { name: '', canPost: true };
	S.proposeInitial = Uint8Array.from(initial);
	pal = task.palette.map(function (c, i) { return { r: c[0], g: c[1], b: c[2], a: i === task.transparent ? 0 : 255 }; });
	imgCanvas.width = S.w;
	imgCanvas.height = S.h;
	S.data = initial;
	setupUi();
	S.canSave = true;
	$('submit').disabled = false;
	$('submit').textContent = '完了';
	$('submit').title = '提案の画面に戻る（描いた絵が付きます）';
	$('back').title = '提案の画面に戻る';
	const banner = $('banner');
	banner.hidden = false;
	banner.textContent = '提案のラフ絵を描いています。描き終わったら「完了」で提案の画面に戻ります（描いている間もこの端末に残ります）。';
	/* 提案のラフ絵には元イラスト・見本が無いので、その欄は出さない（返事の絵だけ出す） */
	if (reply) {
		$('refs').hidden = true;
		$('refs').previousElementSibling.previousElementSibling.hidden = true;
		$('samples').nextElementSibling.hidden = true;
		showProposeReply(reply);
	} else {
		$('sec-refs').hidden = true;
	}
	$('loading').hidden = true;
	resize();
	fit();
	S.rev = 1;
	S.serverRev = 1;
	S.localRev = 1;
	onChange();
	S.serverRev = S.rev;
	S.localRev = S.rev;
}

/* 提案から、描く画面で使うお題の形を作る（パレットは種類ごとのゲーム画像、大きさはラフ絵） */
async function proposalTask(id) {
	const r = await api('/api/admin/kobo/proposals', {});
	if (!r.proposals) {
		throw new Error(r.httpStatus === 401 ? 'login' : 'load');
	}
	const prop = r.proposals.find(function (x) { return x.id === id; });
	if (!prop) {
		throw new Error('notfound');
	}
	const pt = await padPalette(prop.kind);
	const palette = pt.palette, transparent = pt.transparent, colors = pt.colors;
	/*
	 * 新しいほうの絵から描く。返事の絵の後に提案者がラフ絵を描き直していたら、そのラフ絵から。
	 * 64 マスより大きい絵は元イラストとして横に出し、白紙から
	 */
	let img = null, ref = '';
	const src = proposalBase(prop);
	S.adminSrc = src;
	if (src) {
		try {
			const buf = await fetchBuf(imgUrl(src));
			img = await decodeIndexedPng(buf).then(function (o) {
				return { width: o.width, height: o.height, indices: Uint8Array.from(o.indices) };
			}).catch(function () { return rgbaToIndexed(buf, palette, transparent); });
		} catch (e) {
			img = null;
		}
		if (img && (img.width > 64 || img.height > 64)) {
			img = null;
		}
		if (!img) {
			ref = src;
		}
	}
	const w = img ? img.width : 16, h = img ? img.height : 16;
	S.proposal = prop;
	return {
		task: padTask('proposal-' + prop.id, '提案への返事：' + Array.from(prop.body || '').slice(0, 20).join(''), w, h, pt, ref),
		initial: img ? img.indices : new Uint8Array(w * h).fill(transparent)
	};
}

/* 管理人が描き足す土台の絵（返事の絵とラフ絵の新しいほう） */
function proposalBase(prop) {
	if (prop.img && (!prop.replyImg || (prop.imgAt || prop.at || 0) > (prop.replyAt || 0))) {
		return prop.img;
	}
	return prop.replyImg || prop.img || '';
}

async function adminBoot(q) {
	S.admin = q.get('admin');
	if (!ADMIN_TEXT[S.admin]) {
		fatal('開き方が正しくありません。');
		return;
	}
	document.body.classList.add('admin-mode');
	let task, initial;
	try {
		if (S.admin === 'proposal') {
			S.adminTarget = q.get('id') || '';
			const o = await proposalTask(S.adminTarget);
			task = o.task;
			initial = o.initial;
		} else {
			const td = await api('/api/admin/kobo/task', { id: q.get('task') || '' });
			if (!td.task) {
				fatal(td.httpStatus === 401 ? '管理ページでログインしてから開いてね。' : 'このお題は見つかりませんでした。');
				return;
			}
			task = td.task;
			S.w = task.w;
			S.h = task.h;
			if (S.admin === 'fix') {
				const w = (td.works || []).find(function (x) { return x.id === q.get('from') && !x.deleted; });
				if (!w) {
					fatal('元にする案が見つかりませんでした。');
					return;
				}
				S.adminTarget = w.id;
				S.parent = w.id;
				initial = fromImage(await loadIndexed(w.img));
			} else if (S.admin === 'base') {
				if (!task.base) {
					fatal('このお題には今の絵がありません。');
					return;
				}
				S.adminTarget = 'base';
				initial = fromImage(await loadIndexed(task.base));
			} else {
				const n = Number(q.get('n')) || 0;
				if (!task.samples || !task.samples[n]) {
					fatal('その見本は見つかりませんでした。');
					return;
				}
				S.adminTarget = String(n);
				S.sampleIndex = n;
				initial = fromImage(await loadIndexed(task.samples[n]));
			}
		}
	} catch (e) {
		fatal(e.message === 'login' ? '管理ページでログインしてから開いてね。' : e.message === 'size' ? '絵の大きさがお題と合いません。' : '読み込めませんでした。');
		return;
	}
	S.task = task;
	S.w = task.w;
	S.h = task.h;
	S.me = { name: '管理人', canPost: true };
	pal = task.palette.map(function (c, i) { return { r: c[0], g: c[1], b: c[2], a: i === task.transparent ? 0 : 255 }; });
	imgCanvas.width = S.w;
	imgCanvas.height = S.h;
	S.data = initial;
	const draft = readLocal();
	if (draft && draft.u8.length === initial.length && draft.u8.some(function (v, i) { return v !== initial[i]; }) &&
		window.confirm('この端末に、前に直していた途中の絵が残っています（' + new Date(draft.at).toLocaleString('ja-JP') + '）。\n続きから開きますか？')) {
		S.data = draft.u8;
		$('note').value = draft.note || '';
	}
	setupUi();
	/* 管理人用の見た目 */
	const tx = ADMIN_TEXT[S.admin];
	S.canSave = true;
	$('submit').disabled = false;
	$('submit').textContent = tx.btn;
	$('dlg-title').textContent = tx.title;
	$('dlg-submit').textContent = tx.btn;
	$('dlg-note').textContent = tx.dlg;
	const banner = $('banner');
	banner.hidden = false;
	banner.textContent = '管理人として直しています。' + tx.dlg;
	if (S.admin === 'proposal') {
		$('note-label').textContent = '返事（1000字まで。なくても大丈夫）';
		$('note').hidden = true;
		const ta = el('textarea');
		ta.id = 'reply';
		ta.maxLength = 1000;
		ta.value = S.proposal.reply || '';
		ta.placeholder = '例）こんな感じでどうでしょう？色はゲームのパレットに合わせました';
		$('note').insertAdjacentElement('afterend', ta);
		/* 元の提案のことばも見えるように */
		const pv = el('p', 'note', '提案：' + (S.proposal.body || ''));
		$('note-label').insertAdjacentElement('beforebegin', pv);
	} else {
		$('note-label').textContent = tx.note;
	}
	$('loading').hidden = true;
	resize();
	fit();
	S.rev = 1;
	S.serverRev = 1;
	S.localRev = 1;
	onChange();
	S.serverRev = S.rev;
	S.localRev = S.rev;
}

/* 管理人の保存（出す窓の「修正を出す」「保存」「返事を送る」） */
async function adminSave() {
	if (S.busy) {
		return false;
	}
	commitFloat();
	S.busy = true;
	$('dlg-submit').disabled = true;
	setMsg('送っています...', true);
	try {
		const bytes = await encodeIndexedPng({
			width: S.w, height: S.h, indices: composite(),
			palette: S.task.palette.map(function (c) { return { r: c[0], g: c[1], b: c[2] }; })
		});
		const data64 = u8ToB64(bytes);
		let r;
		if (S.admin === 'fix') {
			r = await api('/api/admin/kobo/fix', { taskId: S.task.id, parent: S.parent, data: data64, note: $('note').value });
		} else if (S.admin === 'proposal') {
			r = await api('/api/admin/kobo/proposal-update', { id: S.proposal.id, reply: $('reply').value, replyData: data64 });
		} else {
			r = await api('/api/admin/kobo/task-image', { taskId: S.task.id, slot: S.admin, index: S.sampleIndex || 0, data: data64 });
		}
		if (!r.ok) {
			setMsg(r.httpStatus === 401 ? '管理ページのログインが切れています。管理ページでログインし直してね。（絵はこの端末に残っています）' : errText(r));
			return false;
		}
		clearLocal();
		S.closed = true;
		closeSubmit();
		/* 開いた管理ページに知らせて（読み込み直して「送りました」を出す）、このタブは閉じる */
		try {
			if (window.opener && !window.opener.closed) {
				window.opener.postMessage({ type: 'kobo-admin-saved', mode: S.admin }, location.origin);
			}
		} catch (e) { /* 知らせられなくてもよい */ }
		window.close();
		/* 閉じられなかった時（直接開いた時など）は、ここに出す */
		const banner = $('banner');
		banner.hidden = false;
		banner.textContent = '送りました！このタブは閉じて大丈夫です。管理ページを読み込み直すと出ます。';
		toast('送りました');
		S.canSave = false;
		$('submit').disabled = true;
		return true;
	} catch (e) {
		setMsg('サーバーにつながりませんでした。（絵はこの端末に残っています）');
		return false;
	} finally {
		S.busy = false;
		$('dlg-submit').disabled = false;
	}
}

async function boot() {
	const q = new URLSearchParams(location.search);
	if (q.get('admin')) {
		await adminBoot(q);
		return;
	}
	if (q.get('propose')) {
		await proposeBoot(q);
		return;
	}
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
		/*
		 * 拡大縮小。大きくする時は整数倍でくっきり（pixelated）、欄より大きい絵は「幅に合わせる」で縮めて見る。
		 * refZ = 0 は幅に合わせる
		 */
		let refZ = Number(lsGet(KEY_REF_ZOOM)) || 0;
		const applyRefZoom = function () {
			const nw = im.naturalWidth;
			if (!nw) {
				return;
			}
			const boxW = Math.max(100, refs.clientWidth || 300);
			const fitZ = boxW / nw;
			const z = refZ || (fitZ >= 1 ? Math.floor(fitZ) : fitZ);
			im.style.width = Math.round(nw * z) + 'px';
			im.style.height = 'auto';
			im.classList.toggle('shrink', z < 1);
			$('ref-z').textContent = refZ ? '×' + refZ : (z >= 1 ? '×' + z + '（幅）' : '幅に合わせる');
			$('ref-out').disabled = refZ === 1;
			$('ref-in').disabled = refZ >= 16;
			$('ref-fit').classList.toggle('active', !refZ);
		};
		const setRefZ = function (v) {
			refZ = v;
			lsSet(KEY_REF_ZOOM, String(v));
			applyRefZoom();
		};
		const curZ = function () { return refZ || Math.max(1, Math.floor(Math.max(100, refs.clientWidth || 300) / (im.naturalWidth || 1))); };
		$('ref-out').addEventListener('click', function () { setRefZ(Math.max(1, curZ() - 1)); });
		$('ref-in').addEventListener('click', function () { setRefZ(Math.min(16, curZ() + 1)); });
		$('ref-fit').addEventListener('click', function () { setRefZ(0); });
		im.addEventListener('load', function () {
			$('ref-zoom').hidden = false;
			applyRefZoom();
		});
	} else {
		refs.appendChild(el('p', 'note', '元イラストはありません。'));
	}
	const smp = $('samples');
	const smpCanvases = [];
	let smpZ = Number(lsGet(KEY_SMP_ZOOM)) || Math.max(1, Math.min(8, Math.floor(300 / task.w)));
	(task.samples || []).concat(task.base ? [task.base] : []).forEach(function (id, n, arr) {
		const wrap = el('div', 'checker');
		const cv = thumbCanvas(task, id, smpZ);
		cv.title = ((task.base && n === arr.length - 1) ? '今の絵' : '見本') + '（クリックで色を拾う）';
		/* 見本の上をクリック（右クリック・タップでも）すると、その色を描く色にする */
		cv.style.cursor = 'crosshair';
		cv.addEventListener('contextmenu', function (e) { e.preventDefault(); });
		cv.addEventListener('pointerdown', function (e) {
			e.preventDefault();
			const r = cv.getBoundingClientRect();
			const x = Math.floor((e.clientX - r.left) / r.width * cv.width);
			const y = Math.floor((e.clientY - r.top) / r.height * cv.height);
			loadIndexed(id).then(function (img) {
				if (x < 0 || y < 0 || x >= img.width || y >= img.height) {
					return;
				}
				const i = img.indices[y * img.width + x];
				if (task.colors.indexOf(i) >= 0) {
					setColor(i);
					toast(i === task.transparent ? '透明を拾いました' : '番号 ' + i + ' の色を拾いました');
				} else {
					setColor(task.transparent);
					toast('黒（ゲームでは透明）なので、透明にしました');
				}
			}).catch(function () { /* 読めなければ何もしない */ });
		});
		wrap.appendChild(cv);
		smp.appendChild(wrap);
		smpCanvases.push(cv);
	});
	/* 見本の拡大縮小（整数倍。読み込み前の canvas は task.w × task.h の大きさ） */
	function applySmpZoom() {
		smpCanvases.forEach(function (cv) { cv.style.width = (cv.width * smpZ) + 'px'; });
		$('smp-z').textContent = '×' + smpZ;
		$('smp-out').disabled = smpZ <= 1;
		$('smp-in').disabled = smpZ >= 16;
	}
	$('smp-out').addEventListener('click', function () { smpZ = Math.max(1, smpZ - 1); lsSet(KEY_SMP_ZOOM, String(smpZ)); applySmpZoom(); });
	$('smp-in').addEventListener('click', function () { smpZ = Math.min(16, smpZ + 1); lsSet(KEY_SMP_ZOOM, String(smpZ)); applySmpZoom(); });
	if (smpCanvases.length) {
		applySmpZoom();
	} else {
		$('smp-out').parentNode.hidden = true;
		smp.appendChild(el('p', 'note', '見本はありません。'));
	}

	/* プレビュー（最初は止めておく。描いている時に動くと気が散るので。再生したかは覚えておく） */
	const bPlay = lsGet(KEY_PV_PLAY) === '1';
	pvMain = makePreview($('preview'), task, { scale: 2, paused: !bPlay });
	if (window.matchMedia('(max-width: 860px)').matches) {
		pvMini = makePreview($('mini'), task, { scale: 1, compact: true, paused: !bPlay });
	}
	function syncPlay() {
		const on = pvMain.isPlaying();
		$('pv-play').textContent = on ? '■ 止める' : '▶ 再生';
		$('pv-play').classList.toggle('active', on);
		$('pv-prev').disabled = on;
		$('pv-next').disabled = on;
		$('pv-frame').textContent = pvMain.frameText();
	}
	pvMain.onFrame = function () { $('pv-frame').textContent = pvMain.frameText(); };
	$('pv-play').addEventListener('click', function () {
		const want = !pvMain.isPlaying();
		[pvMain, pvMini].forEach(function (pv) {
			if (pv) {
				if (want) { pv.play(); } else { pv.pause(); }
			}
		});
		lsSet(KEY_PV_PLAY, pvMain.isPlaying() ? '1' : '0');
		syncPlay();
	});
	$('pv-prev').addEventListener('click', function () { pvMain.step(-1); if (pvMini) { pvMini.step(-1); } });
	$('pv-next').addEventListener('click', function () { pvMain.step(1); if (pvMini) { pvMini.step(1); } });
	syncPlay();
	Array.prototype.forEach.call(document.querySelectorAll('[data-pvs]'), function (b) {
		b.addEventListener('click', function () {
			Array.prototype.forEach.call(document.querySelectorAll('[data-pvs]'), function (x) { x.classList.toggle('active', x === b); });
			pvMain.setScale(+b.dataset.pvs);
			S.imgDirty = true;
			requestDraw();
		});
	});

	buildPalette();
	const first = (task.sampleColors || []).concat(task.colors).find(function (i) { return i !== task.transparent && task.colors.indexOf(i) >= 0; });
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
$('zoomSel').addEventListener('change', function (e) {
	zoomAt(vw / 2, vh / 2, Number(e.target.value));
	view.focus && view.focus();
});
$('save-wip').addEventListener('click', function () { save(false, false); });
/* 「出す」は確認の窓を開く（ひとこと・名前の出し方・同意を確かめてから出す） */
function openSubmit() {
	if (S.propose) {
		finishPropose();
		return;
	}
	if (!S.canSave) {
		setMsg('保存して出すには、ゲームにログインしてキャラを作ってね。（絵はこの端末に自動で残ります）');
		return;
	}
	commitFloat();
	const comp = composite();
	const cv = $('dlg-canvas');
	cv.width = S.w;
	cv.height = S.h;
	cv.getContext('2d').putImageData(indicesToImageData(comp, S.w, S.h, pal), 0, 0);
	const sc = Math.max(1, Math.min(8, Math.floor(400 / S.w), Math.floor(160 / S.h)));
	cv.style.width = (S.w * sc) + 'px';
	$('dlg-msg').textContent = '';
	$('dlg-submit').disabled = S.admin ? false : !$('agree').checked;
	$('submit-dlg').hidden = false;
	$('note').focus();
}
function closeSubmit() {
	$('submit-dlg').hidden = true;
	saveLocal();
}
$('submit').addEventListener('click', openSubmit);
$('dlg-cancel').addEventListener('click', closeSubmit);
$('submit-dlg').addEventListener('click', function (e) {
	if (e.target === $('submit-dlg')) {
		closeSubmit();
	}
});
$('agree').addEventListener('change', function () { $('dlg-submit').disabled = (!S.admin && !$('agree').checked) || S.busy; });
$('dlg-submit').addEventListener('click', function () { save(true, false); });
$('back').addEventListener('click', function () {
	if (S.propose) {
		finishPropose();
		return;
	}
	if (S.admin) {
		/* 管理画面から別のタブで開いているので、閉じる（閉じられなければ管理ページへ） */
		saveLocal();
		S.closed = true;
		window.close();
		location.href = '/admin';
		return;
	}
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
