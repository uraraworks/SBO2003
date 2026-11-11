/**
 * dot-editor.js
 * ドット絵エディタ (/dot) の画面まわり。
 * 描画の計算そのものは dot-core.js、ブラウザへの保存は dot-store.js、PNG の読み書きは dot-png.js。
 *
 * 全体の考え方:
 *   - 1 つの絵 = 1 つのタブ (tab オブジェクト)。レイヤー・履歴・拡大率などをタブごとに持つ。
 *   - レイヤーは Int16Array（-1 = 空）。見た目は「合成した 1 枚」を等倍の canvas に描いておき、
 *     画面にはそれを拡大して貼る。
 *   - 図形やペンは、描いている最中もレイヤーのデータを直接書き換える。
 *     ドラッグ中のプレビューは「描き始めの状態に戻してから描き直す」ことで出す。
 *     指を離した時に変更箇所だけを差分で履歴へ入れる。
 *   - 範囲選択を動かしている間（浮いている状態）も同じ方式。
 *     「浮かせた絵を抜いた状態」を base として持ち、動かすたびに base + 浮かせた絵で書き直す。
 *     こうすると合成・タイル確認・アニメ確認が特別扱いなしで動く。
 */

import { decodeIndexedPng, encodeIndexedPng } from './dot-png.js';
import * as C from './dot-core.js';
import * as store from './dot-store.js';

const $ = (id) => document.getElementById(id);
const PAD = 32; // 絵のまわりの余白(px)。端まで画面の中央に持ってこられるように
const ZOOMS = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32];
const HISTORY_LIMIT = 100;
// ホイールで拡大縮小する時の「1段階に必要な回転量」(px 換算、「ふつう」の時)。
// Magic Mouse やトラックパッドは細かい量のイベントが大量に来るので、量をためて
// これを超えるごとに1段階だけ変える。大きくするほどゆっくりになる。
// （Windows のふつうのマウスは 1 目盛りでだいたい 100）
const WHEEL_ZOOM_STEP = 100;
// トラックパッドのピンチ（Mac の Chrome では Ctrl 付きのホイールとして来る）用。
// ピンチは1回の量がとても小さいので別の値にする。
const PINCH_ZOOM_STEP = 30;
// 画面の「ホイールの拡大の速さ」で選ぶ倍率（上の量にかける。大きいほどゆっくり）
const WHEEL_ZOOM_SPEEDS = { slow: 2.5, normal: 1, fast: 0.4 };
const WHEEL_ZOOM_SPEED_KEY = 'sbo-dot-wheel-zoom-speed';
// 1段階変えたあと、次の段階まで最低これだけ間を空ける(ms)。慣性スクロールの勢いで
// 一気に何段も進まないようにするため。
const WHEEL_ZOOM_COOLDOWN = 180;
// これだけホイールが止まったら、ためた量を捨てる(ms)。
const WHEEL_ZOOM_IDLE = 500;
const IS_TEST = new URLSearchParams(location.search).get('test') === '1';

// ----------------------------------------------------------------
// 状態
// ----------------------------------------------------------------

const S = {
  tabs: [],
  active: null,
  dir: null,            // { handle, name, files: [{name, handle, size, thumb, bad}] }
  tool: 'pen',
  opts: {
    rectFill: false, ellipseFill: false, fillGlobal: false,
    snap: false, clip: true, keepIdx: false, skipTrans: false,
    pixGrid: true, cellGrid: true, cellNo: false, checker: true, used: false,
  },
  clipboard: null,      // { w, h, data: Int16Array, palette }
  spaceDown: false,
  hover: null,          // マウスの下の画素 {x, y}
  backedUp: new Set(),  // 今回の訪問で「保存前の元の絵」を控えた名前
  draftIndex: new Map(),// ファイル名 -> 控えの数（手動のもの）
  showAllDrafts: false,
  nextUid: 1,
};

const TOOLS = [
  ['pen', 'ペン', 'B'], ['eraser', '消しゴム', 'E'], ['line', '直線', 'L'],
  ['rect', '四角', 'U'], ['ellipse', '円', 'O'], ['fill', '塗りつぶし', 'G'],
  ['pick', 'スポイト', 'I'], ['select', '範囲選択', 'M'], ['hand', '手のひら', 'H'],
];

const cv = $('cv');
const ctx = cv.getContext('2d');
const vp = $('viewport');
const sizer = $('sizer');
let dpr = window.devicePixelRatio || 1;

// ----------------------------------------------------------------
// 小さな道具
// ----------------------------------------------------------------

function el(tag, attrs = {}, ...children) {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else if (v === true) e.setAttribute(k, '');
    else if (v !== false && v != null) e.setAttribute(k, v);
  }
  for (const c of children.flat()) if (c != null) e.append(c.nodeType ? c : document.createTextNode(String(c)));
  return e;
}

let toastTimer = 0;
function toast(msg, kind = '') {
  const t = $('toast');
  t.textContent = msg;
  t.className = 'show ' + kind;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.className = ''; }, kind === 'ng' ? 5000 : 2600);
}

const pad2 = (n) => String(n).padStart(2, '0');
function stamp(d = new Date()) {
  return `${d.getFullYear()}/${pad2(d.getMonth() + 1)}/${pad2(d.getDate())} ${pad2(d.getHours())}:${pad2(d.getMinutes())}`;
}
const hex2 = (n) => n.toString(16).padStart(2, '0');
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
const baseName = (n) => n.replace(/\.png$/i, '');
const isTyping = (t) => t && (t.tagName === 'TEXTAREA' || t.tagName === 'SELECT' || (t.tagName === 'INPUT' && ['text', 'number', 'search', 'password'].includes(t.type)));

function download(bytes, name) {
  const url = URL.createObjectURL(new Blob([bytes], { type: 'image/png' }));
  const a = el('a', { href: url, download: name });
  document.body.append(a);
  a.click();
  a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 10000);
}

/** 2 回押して実行するボタン（confirm() を使わない） */
function twoStep(btn, label, action) {
  let armed = false;
  let timer = 0;
  const orig = btn.textContent;
  btn.addEventListener('click', (e) => {
    e.stopPropagation();
    if (!armed) {
      armed = true;
      btn.textContent = label;
      timer = setTimeout(() => { armed = false; btn.textContent = orig; }, 3000);
    } else {
      clearTimeout(timer);
      armed = false;
      btn.textContent = orig;
      action();
    }
  });
}

// ----------------------------------------------------------------
// タブ（1 枚の絵）
// ----------------------------------------------------------------

function makeTab({ label, fileName, handle, w, h, palette, layers, draftId = null, autoId = null, fromDraft = false, dirty = false }) {
  const off = document.createElement('canvas');
  off.width = w; off.height = h;
  const offCtx = off.getContext('2d');
  const img = offCtx.createImageData(w, h);
  const tab = {
    uid: S.nextUid++,
    label, fileName, handle, w, h, palette, layers, cur: layers.length - 1,
    fg: 1, undo: [], redo: [],
    dirty, unwritten: fromDraft || dirty, fromDraft,
    draftId, draftName: null, autoId, autoTimer: 0,
    zoom: 1, scroll: null,
    cell: C.guessCellSize(fileName),
    sel: null, float: null, edit: null,
    lastCell: 0, pasteAt: null, palAnchor: 1, palRange: null,
    animText: '0,1',
    off, offCtx, img, u32: new Uint32Array(img.data.buffer),
    compDirty: true, usedDirty: true, used: null,
    transIdx: C.findTransparentIndex(palette),
    palEditBefore: null,
  };
  // 色番号の初期値は透明でない最初の色
  tab.fg = palette.findIndex((c) => !C.isBlack(c));
  if (tab.fg < 0) tab.fg = 0;
  return tab;
}

async function openBytes(fileName, bytes, handle = null) {
  const img = await decodeIndexedPng(bytes);
  const palette = C.normalizePalette(img.palette);
  const tab = makeTab({
    label: fileName, fileName, handle, w: img.width, h: img.height, palette,
    layers: [{ name: '元の絵', visible: true, ghost: false, data: Int16Array.from(img.indices) }],
  });
  addTab(tab);
  return tab;
}

function openDraftRecord(d) {
  const palette = C.normalizePalette(d.palette);
  const layers = d.layers.map((l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, data: Int16Array.from(l.data) }));
  const entry = S.dir && S.dir.files.find((f) => f.name === d.file);
  const tab = makeTab({
    label: d.name, fileName: d.file, handle: entry ? entry.handle : null, w: d.width, h: d.height,
    palette, layers,
    draftId: d.auto ? null : d.id, autoId: d.auto ? d.id : null,
    fromDraft: true, dirty: !!d.auto,
  });
  tab.draftName = d.auto ? null : d.name;
  addTab(tab);
  return tab;
}

function addTab(tab) {
  S.tabs.push(tab);
  activate(tab);
}

function activate(tab) {
  const old = S.active;
  if (old && old !== tab) {
    commitFloat(old);
    old.scroll = { l: vp.scrollLeft, t: vp.scrollTop };
  }
  S.active = tab;
  const first = tab && !tab.scroll;
  if (tab && first) tab.zoom = fitZoom(tab);
  renderTabs();
  syncPanels();
  layoutSizer();
  if (tab) {
    if (tab.scroll) { vp.scrollLeft = tab.scroll.l; vp.scrollTop = tab.scroll.t; }
    else centerView(tab);
  }
  renderFileList();
  renderDrafts();
  requestDraw();
}

function fitZoom(tab) {
  const vw = Math.max(200, vp.clientWidth - 40);
  const vh = Math.max(200, vp.clientHeight - 40);
  let best = 1;
  for (const z of ZOOMS) if (tab.w * z <= vw && tab.h * z <= vh) best = z;
  return best;
}

function centerView(tab) {
  vp.scrollLeft = (sizer.offsetWidth - vp.clientWidth) / 2;
  vp.scrollTop = (sizer.offsetHeight - vp.clientHeight) / 2;
}

async function closeTab(tab) {
  if (tab.dirty) {
    // 変更が残っている。自動の控えは残すので、あとで復元できる
    const ok = await askYesNo(`「${tab.label}」には保存していない変更があります。閉じますか？（自動の控えは残るので、次に開いた時に復元できます）`);
    if (!ok) return;
    await autosaveNow(tab);
  } else {
    await clearAutosave(tab);
  }
  clearTimeout(tab.autoTimer);
  const i = S.tabs.indexOf(tab);
  S.tabs.splice(i, 1);
  if (S.active === tab) {
    S.active = null;
    activate(S.tabs[Math.min(i, S.tabs.length - 1)] || null);
  } else {
    renderTabs();
  }
  renderFileList();
  renderDrafts();
}

/** はい/いいえ。confirm() の代わりに dialog を使う */
function askYesNo(msg) {
  return new Promise((resolve) => {
    const dlg = el('dialog', {},
      el('p', {}, msg),
      el('div', { class: 'row' },
        el('button', { onclick: () => { dlg.close('yes'); } }, 'はい'),
        el('button', { class: 'sub', onclick: () => { dlg.close('no'); } }, 'やめる')));
    dlg.addEventListener('close', () => { resolve(dlg.returnValue === 'yes'); dlg.remove(); });
    document.body.append(dlg);
    dlg.showModal();
  });
}

function renderTabs() {
  const box = $('tabs');
  box.textContent = '';
  for (const t of S.tabs) {
    box.append(el('div', { class: 'tab' + (t === S.active ? ' active' : ''), title: t.fileName, onclick: () => { if (t !== S.active) activate(t); } },
      t.fromDraft ? el('span', { class: 'draft-tag' }, '控え') : null,
      el('span', { class: 'label' }, t.label),
      t.dirty ? el('span', { class: 'dot', title: '保存していない変更があります' }, '●') : null,
      el('button', { class: 'x', title: 'タブを閉じる', onclick: (e) => { e.stopPropagation(); closeTab(t); } }, '×')));
  }
}

// ----------------------------------------------------------------
// 変更の通知・履歴
// ----------------------------------------------------------------

function curLayer(tab) { return tab.layers[tab.cur]; }
/** 消しゴムの値: 一番下のレイヤーは透明色の番号、上のレイヤーは「空」 */
function eraseValue(tab) { return tab.cur === 0 ? tab.transIdx : -1; }

function changed(tab, { palette = false } = {}) {
  tab.compDirty = true;
  tab.usedDirty = true;
  if (palette) tab.transIdx = C.findTransparentIndex(tab.palette);
  requestDraw();
}

function markDirty(tab) {
  tab.dirty = true;
  tab.unwritten = true;
  renderTabs();
  renderFileList();
  scheduleAutosave(tab);
}

function pushHistory(tab, entry) {
  tab.undo.push(entry);
  if (tab.undo.length > HISTORY_LIMIT) tab.undo.shift();
  tab.redo = [];
  markDirty(tab);
}

function beginEdit(tab) {
  if (tab.edit) endEdit(tab);
  tab.edit = { li: tab.cur, before: curLayer(tab).data.slice() };
}

function endEdit(tab) {
  const e = tab.edit;
  tab.edit = null;
  if (!e) return false;
  const layer = tab.layers[e.li];
  if (!layer) return false;
  const rect = C.diffRect(e.before, layer.data, tab.w, tab.h);
  if (!rect) return false;
  pushHistory(tab, { ops: [{ t: 'px', li: e.li, rect }] });
  return true;
}

function snapStruct(tab) {
  return { layers: tab.layers.map((l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, data: l.data.slice() })), cur: tab.cur };
}
function restoreStruct(tab, s) {
  tab.layers = s.layers.map((l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, data: l.data.slice() }));
  tab.cur = clamp(s.cur, 0, tab.layers.length - 1);
}

/** レイヤーの数や順番が変わる操作は、前後の全体を履歴に入れる */
function structOp(tab, fn) {
  commitFloat(tab);
  const before = snapStruct(tab);
  fn();
  const after = snapStruct(tab);
  pushHistory(tab, { ops: [{ t: 'struct', before, after }] });
  tab.sel = null;
  changed(tab);
  renderLayers();
}

function applyOps(tab, ops, forward) {
  const list = forward ? ops : [...ops].reverse();
  for (const op of list) {
    if (op.t === 'px') {
      const layer = tab.layers[op.li];
      if (layer) C.applyRect(layer.data, tab.w, tab.h, op.rect, forward ? op.rect.after : op.rect.before);
    } else if (op.t === 'pal') {
      tab.palette = C.clonePalette(forward ? op.after : op.before);
    } else if (op.t === 'struct') {
      restoreStruct(tab, forward ? op.after : op.before);
    }
  }
}

function undo(tab) {
  if (!tab) return;
  if (tab.float) { cancelFloat(tab); return; } // 浮かせている間の「元に戻す」は、動かす前に戻すこと
  endEdit(tab);
  const e = tab.undo.pop();
  if (!e) { toast('これ以上は戻せません'); return; }
  applyOps(tab, e.ops, false);
  tab.redo.push(e);
  afterHistory(tab);
}

function redo(tab) {
  if (!tab) return;
  if (tab.float) commitFloat(tab);
  const e = tab.redo.pop();
  if (!e) { toast('やり直す操作がありません'); return; }
  applyOps(tab, e.ops, true);
  tab.undo.push(e);
  afterHistory(tab);
}

function afterHistory(tab) {
  tab.sel = null;
  changed(tab, { palette: true });
  markDirty(tab);
  renderLayers();
  renderPalette();
}

// ----------------------------------------------------------------
// 合成
// ----------------------------------------------------------------

function refreshComposite(tab) {
  if (!tab.compDirty) return;
  C.compositeInto(tab.u32, tab.w, tab.h, tab.layers, tab.palette);
  tab.offCtx.putImageData(tab.img, 0, 0);
  tab.compDirty = false;
}

function flattenTab(tab) {
  return C.flatten(tab.layers, tab.w, tab.h, tab.transIdx);
}

async function encodeTab(tab) {
  commitFloat(tab);
  return encodeIndexedPng({ width: tab.w, height: tab.h, indices: flattenTab(tab), palette: tab.palette });
}

// ----------------------------------------------------------------
// 表示（拡大・スクロール・描画）
// ----------------------------------------------------------------

function layoutSizer() {
  const tab = S.active;
  $('empty').hidden = !!tab;
  if (!tab) { sizer.style.width = sizer.style.height = '0px'; cv.style.display = 'none'; return; }
  cv.style.display = 'block';
  sizer.style.width = tab.w * tab.zoom + PAD * 2 + 'px';
  sizer.style.height = tab.h * tab.zoom + PAD * 2 + 'px';
  resizeCanvas();
}

function resizeCanvas() {
  dpr = window.devicePixelRatio || 1;
  const w = vp.clientWidth;
  const h = vp.clientHeight;
  cv.style.width = w + 'px';
  cv.style.height = h + 'px';
  cv.width = Math.max(1, Math.round(w * dpr));
  cv.height = Math.max(1, Math.round(h * dpr));
  requestDraw();
}

/** 絵の左上が、表示領域のどこ（px）にあるか */
function origin(tab) {
  const z = tab.zoom;
  const vw = vp.clientWidth;
  const vh = vp.clientHeight;
  const sw = tab.w * z + PAD * 2;
  const sh = tab.h * z + PAD * 2;
  return {
    ox: sw <= vw ? (vw - tab.w * z) / 2 : PAD - vp.scrollLeft,
    oy: sh <= vh ? (vh - tab.h * z) / 2 : PAD - vp.scrollTop,
  };
}

/** マウスの位置 → 絵の中の座標（小数） */
function toImgF(e) {
  const tab = S.active;
  const r = vp.getBoundingClientRect();
  const sx = e.clientX - r.left - vp.clientLeft;
  const sy = e.clientY - r.top - vp.clientTop;
  const { ox, oy } = origin(tab);
  return { fx: (sx - ox) / tab.zoom, fy: (sy - oy) / tab.zoom, sx, sy };
}
function toImg(e) {
  const p = toImgF(e);
  return { x: Math.floor(p.fx), y: Math.floor(p.fy), sx: p.sx, sy: p.sy, fx: p.fx, fy: p.fy };
}

/** 絵の座標 (ix, iy) が画面の (sx, sy) に来るように拡大率を変える */
function setZoom(tab, z, ix, iy, sx, sy) {
  z = clamp(z, ZOOMS[0], ZOOMS[ZOOMS.length - 1]);
  if (z === tab.zoom) return;
  tab.zoom = z;
  layoutSizer();
  vp.scrollLeft = ix * z + PAD - sx;
  vp.scrollTop = iy * z + PAD - sy;
  syncStatus();
  requestDraw();
}

function stepZoom(tab, dir) {
  if (dir > 0) return ZOOMS.find((z) => z > tab.zoom) ?? tab.zoom;
  return [...ZOOMS].reverse().find((z) => z < tab.zoom) ?? tab.zoom;
}

function zoomAtCenter(dir) {
  const tab = S.active;
  if (!tab) return;
  const { ox, oy } = origin(tab);
  const cx = vp.clientWidth / 2;
  const cy = vp.clientHeight / 2;
  setZoom(tab, stepZoom(tab, dir), (cx - ox) / tab.zoom, (cy - oy) / tab.zoom, cx, cy);
}

let drawQueued = false;
function requestDraw() {
  if (drawQueued) return;
  drawQueued = true;
  requestAnimationFrame(() => {
    drawQueued = false;
    drawAll();
  });
}

let checkerPattern = null;
function getChecker() {
  if (checkerPattern) return checkerPattern;
  const c = document.createElement('canvas');
  c.width = c.height = 16;
  const g = c.getContext('2d');
  g.fillStyle = '#bbb'; g.fillRect(0, 0, 16, 16);
  g.fillStyle = '#888'; g.fillRect(0, 0, 8, 8); g.fillRect(8, 8, 8, 8);
  checkerPattern = ctx.createPattern(c, 'repeat');
  return checkerPattern;
}

function drawAll() {
  drawMain();
  drawOverview();
  const tab = S.active;
  if (tab) {
    if (tab.usedDirty && S.opts.used) { tab.used = C.usedIndices(tab.layers); tab.usedDirty = false; renderPalette(); }
    else if (palDirty) renderPalette();
  }
  if ($('tile').closest('details').open) drawTile();
  drawAnim();
  syncStatus();
}
let palDirty = false;

function drawMain() {
  const tab = S.active;
  const cw = vp.clientWidth;
  const ch = vp.clientHeight;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, cw, ch);
  if (!tab) return;
  refreshComposite(tab);
  const z = tab.zoom;
  const { ox, oy } = origin(tab);
  const x0 = Math.max(0, Math.floor(-ox / z));
  const y0 = Math.max(0, Math.floor(-oy / z));
  const x1 = Math.min(tab.w, Math.ceil((cw - ox) / z));
  const y1 = Math.min(tab.h, Math.ceil((ch - oy) / z));
  if (x1 <= x0 || y1 <= y0) return;
  const dx = ox + x0 * z;
  const dy = oy + y0 * z;
  const dw = (x1 - x0) * z;
  const dh = (y1 - y0) * z;

  // 透明（黒）のところに見せる下地
  if (S.opts.checker) {
    ctx.save();
    ctx.translate(ox, oy);
    ctx.fillStyle = getChecker();
    ctx.fillRect(x0 * z, y0 * z, dw, dh);
    ctx.restore();
  } else {
    ctx.fillStyle = '#000';
    ctx.fillRect(dx, dy, dw, dh);
  }
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(tab.off, x0, y0, x1 - x0, y1 - y0, dx, dy, dw, dh);

  // 画素の線
  if (S.opts.pixGrid && z >= 4) {
    ctx.strokeStyle = 'rgba(128,128,128,0.45)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = x0; x <= x1; x++) { const sx = Math.round(ox + x * z) + 0.5; ctx.moveTo(sx, dy); ctx.lineTo(sx, dy + dh); }
    for (let y = y0; y <= y1; y++) { const sy = Math.round(oy + y * z) + 0.5; ctx.moveTo(dx, sy); ctx.lineTo(dx + dw, sy); }
    ctx.stroke();
  }
  // マスの線
  const cwid = tab.cell.w;
  const chei = tab.cell.h;
  if (S.opts.cellGrid) {
    ctx.strokeStyle = 'rgba(240,184,74,0.85)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = Math.ceil(x0 / cwid) * cwid; x <= x1; x += cwid) { const sx = Math.round(ox + x * z) + 0.5; ctx.moveTo(sx, dy); ctx.lineTo(sx, dy + dh); }
    for (let y = Math.ceil(y0 / chei) * chei; y <= y1; y += chei) { const sy = Math.round(oy + y * z) + 0.5; ctx.moveTo(dx, sy); ctx.lineTo(dx + dw, sy); }
    ctx.stroke();
  }
  // マスの番号
  if (S.opts.cellNo && cwid * z >= 20) {
    const cols = Math.max(1, Math.floor(tab.w / cwid));
    ctx.font = '11px sans-serif';
    ctx.textBaseline = 'top';
    for (let cy = Math.floor(y0 / chei); cy * chei < y1; cy++) {
      for (let cx = Math.floor(x0 / cwid); cx * cwid < x1; cx++) {
        if (cx >= cols) continue;
        const label = String(cy * cols + cx);
        const sx = ox + cx * cwid * z + 3;
        const sy = oy + cy * chei * z + 2;
        ctx.fillStyle = 'rgba(0,0,0,0.75)';
        ctx.fillText(label, sx + 1, sy + 1);
        ctx.fillStyle = '#ffe9a8';
        ctx.fillText(label, sx, sy);
      }
    }
  }
  // 絵のふち
  ctx.strokeStyle = 'rgba(255,255,255,0.5)';
  ctx.strokeRect(Math.round(ox) - 0.5, Math.round(oy) - 0.5, tab.w * z + 1, tab.h * z + 1);

  // 選択範囲
  if (tab.sel) {
    const s = tab.sel;
    const sx = Math.round(ox + s.x * z) + 0.5;
    const sy = Math.round(oy + s.y * z) + 0.5;
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);
    ctx.strokeStyle = '#000';
    ctx.strokeRect(sx, sy, s.w * z, s.h * z);
    ctx.lineDashOffset = 4;
    ctx.strokeStyle = '#fff';
    ctx.strokeRect(sx, sy, s.w * z, s.h * z);
    ctx.setLineDash([]);
    ctx.lineDashOffset = 0;
  }
  // マウスの下の 1 ドット
  const hv = S.hover;
  if (hv && z >= 3 && hv.x >= 0 && hv.y >= 0 && hv.x < tab.w && hv.y < tab.h && !['hand', 'select'].includes(S.tool)) {
    const sx = Math.round(ox + hv.x * z) + 0.5;
    const sy = Math.round(oy + hv.y * z) + 0.5;
    ctx.strokeStyle = '#000';
    ctx.strokeRect(sx - 1, sy - 1, z + 1, z + 1);
    ctx.strokeStyle = '#fff';
    ctx.strokeRect(sx, sy, z - 1, z - 1);
  }
}

// ---- 全体図 ----
function drawOverview() {
  const tab = S.active;
  const ov = $('ov');
  const wrap = $('ovWrap');
  if (!tab) { ov.width = ov.height = 1; $('ovRect').style.display = 'none'; return; }
  refreshComposite(tab);
  const gs = C.gameScale(tab.fileName);
  const boxW = $('ovBox').clientWidth - 2 || 300;
  let scale = gs;
  while (scale > 1 && tab.w * scale > boxW) scale--;
  if (tab.w * scale > boxW) scale = boxW / tab.w; // それでも入らない時だけ縮小
  const cssW = Math.round(tab.w * scale);
  const cssH = Math.round(tab.h * scale);
  if (ov.width !== tab.w || ov.height !== tab.h) { ov.width = tab.w; ov.height = tab.h; }
  ov.style.width = cssW + 'px';
  ov.style.height = cssH + 'px';
  wrap.style.width = cssW + 'px';
  wrap.style.height = cssH + 'px';
  wrap.classList.toggle('black', !S.opts.checker);
  const g = ov.getContext('2d');
  g.clearRect(0, 0, tab.w, tab.h);
  g.drawImage(tab.off, 0, 0);
  $('ovNote').textContent = `${tab.w}×${tab.h}px / ゲームでは ${gs} 倍で表示${scale !== gs ? '（ここでは入りきらないので縮小）' : ''}`;

  // 今見えている範囲の枠
  const z = tab.zoom;
  const { ox, oy } = origin(tab);
  const vx0 = clamp(-ox / z, 0, tab.w);
  const vy0 = clamp(-oy / z, 0, tab.h);
  const vx1 = clamp((vp.clientWidth - ox) / z, 0, tab.w);
  const vy1 = clamp((vp.clientHeight - oy) / z, 0, tab.h);
  const r = $('ovRect');
  r.style.display = 'block';
  const k = cssW / tab.w;
  r.style.left = vx0 * k + 'px';
  r.style.top = vy0 * k + 'px';
  r.style.width = Math.max(2, (vx1 - vx0) * k - 2) + 'px';
  r.style.height = Math.max(2, (vy1 - vy0) * k - 2) + 'px';
}

function ovScrollTo(e) {
  const tab = S.active;
  if (!tab) return;
  const r = $('ov').getBoundingClientRect();
  const ix = ((e.clientX - r.left) / r.width) * tab.w;
  const iy = ((e.clientY - r.top) / r.height) * tab.h;
  vp.scrollLeft = ix * tab.zoom + PAD - vp.clientWidth / 2;
  vp.scrollTop = iy * tab.zoom + PAD - vp.clientHeight / 2;
  requestDraw();
}

// ---- タイル確認 ----
function currentRegion(tab) {
  if (tab.sel) return tab.sel;
  return C.cellRect(tab.lastCell, tab.w, tab.h, tab.cell.w, tab.cell.h);
}

function drawTile() {
  const tab = S.active;
  const c = $('tile');
  if (!tab) return;
  const r = currentRegion(tab);
  if (!r) return;
  refreshComposite(tab);
  const gs = C.gameScale(tab.fileName);
  const w = r.w * 3 * gs;
  const h = r.h * 3 * gs;
  if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }
  const g = c.getContext('2d');
  g.imageSmoothingEnabled = false;
  g.clearRect(0, 0, w, h);
  for (let j = 0; j < 3; j++) for (let i = 0; i < 3; i++) {
    g.drawImage(tab.off, r.x, r.y, r.w, r.h, i * r.w * gs, j * r.h * gs, r.w * gs, r.h * gs);
  }
  c.classList.toggle('black', !S.opts.checker);
}

// ---- アニメ確認 ----
let animTimer = 0;
let animFrame = 0;
let animPlaying = false;

function drawAnim() {
  const tab = S.active;
  const c = $('animCv');
  if (!tab || !$('animCv').closest('details').open) return;
  refreshComposite(tab);
  const gs = C.gameScale(tab.fileName);
  const frames = C.parseCellList($('animFrames').value);
  const w = tab.cell.w * gs;
  const h = tab.cell.h * gs;
  if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }
  const g = c.getContext('2d');
  g.imageSmoothingEnabled = false;
  g.clearRect(0, 0, w, h);
  c.classList.toggle('black', !S.opts.checker);
  if (!frames.length) { $('animInfo').textContent = 'マス番号を入れてください'; return; }
  const idx = animFrame % frames.length;
  const r = C.cellRect(frames[idx], tab.w, tab.h, tab.cell.w, tab.cell.h);
  $('animInfo').textContent = `${idx + 1}/${frames.length} コマ目（マス ${frames[idx]}）${r ? '' : ' ※絵の外です'}`;
  if (r) g.drawImage(tab.off, r.x, r.y, r.w, r.h, 0, 0, w, h);
}

function setAnimPlaying(on) {
  animPlaying = on;
  clearInterval(animTimer);
  $('animPlay').textContent = on ? '停止' : '再生';
  if (on) {
    const ms = clamp(Number($('animMs').value) || 200, 20, 5000);
    animTimer = setInterval(() => { animFrame++; drawAnim(); }, ms);
  }
}

// ---- ステータスバー ----
function syncStatus() {
  const tab = S.active;
  const st = $('status');
  if (!tab) { st.textContent = ''; return; }
  const h = S.hover;
  const parts = [];
  if (h && h.x >= 0 && h.y >= 0 && h.x < tab.w && h.y < tab.h) {
    const idx = C.pickIndex(tab.layers, tab.w, tab.h, h.x, h.y, tab.transIdx);
    const col = tab.palette[idx];
    const cell = C.cellNumberAt(h.x, h.y, tab.w, tab.h, tab.cell.w, tab.cell.h);
    parts.push(`座標 <b>${h.x}, ${h.y}</b>`);
    parts.push(`マス <b>${cell < 0 ? '-' : cell}</b>`);
    parts.push(`<span class="sw" style="background:rgb(${col.r},${col.g},${col.b})"></span>色番号 <b>${idx}</b> (<b>${col.r},${col.g},${col.b}</b>)`);
  } else {
    parts.push('座標 <b>-</b>');
  }
  parts.push(`拡大 <b>${tab.zoom}倍</b>`);
  parts.push(`${tab.w}×${tab.h}`);
  parts.push(`レイヤー <b>${esc(curLayer(tab).name)}</b>`);
  if (tab.sel) parts.push(`選択 <b>${tab.sel.x},${tab.sel.y} ${tab.sel.w}×${tab.sel.h}</b>${S.opts.clip && S.tool !== 'select' ? '（描くのはこの中だけ）' : ''}`);
  st.innerHTML = parts.map((p) => `<span>${p}</span>`).join('');
}
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

// ----------------------------------------------------------------
// 描画ツール
// ----------------------------------------------------------------

function drawClip(tab) {
  return S.opts.clip && tab.sel && !tab.float ? tab.sel : null;
}

function makePlot(tab, val) {
  const d = curLayer(tab).data;
  const w = tab.w;
  const h = tab.h;
  const clip = drawClip(tab);
  return (x, y) => {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    if (clip && (x < clip.x || y < clip.y || x >= clip.x + clip.w || y >= clip.y + clip.h)) return;
    d[y * w + x] = val;
  };
}

function toolValue(tab) {
  return S.tool === 'eraser' ? eraseValue(tab) : tab.fg;
}

let drag = null;

function canDraw(tab) {
  if (!curLayer(tab).visible) { toast('このレイヤーは隠れています。目のマークで表示してから描いてください', 'ng'); return false; }
  return true;
}

function setFg(tab, i) {
  tab.fg = clamp(i, 0, 255);
  tab.palAnchor = tab.fg;
  tab.palRange = null;
  renderPalette();
}

function eyedrop(tab, p) {
  if (p.x < 0 || p.y < 0 || p.x >= tab.w || p.y >= tab.h) return;
  setFg(tab, C.pickIndex(tab.layers, tab.w, tab.h, p.x, p.y, tab.transIdx));
}

function shapeEnd(tab, start, p, shift) {
  let ex = p.x, ey = p.y;
  if (shift) {
    if (S.tool === 'line') [ex, ey] = C.constrainLine(start.x, start.y, p.x, p.y);
    else [ex, ey] = C.squareEnd(start.x, start.y, p.x, p.y);
  }
  return [ex, ey];
}

function drawShape(tab, start, p, shift) {
  const [ex, ey] = shapeEnd(tab, start, p, shift);
  const plot = makePlot(tab, tab.fg);
  if (S.tool === 'line') C.bresenham(start.x, start.y, ex, ey, plot);
  else if (S.tool === 'rect') C.rectPixels(start.x, start.y, ex, ey, S.opts.rectFill, plot);
  else C.ellipsePixels(start.x, start.y, ex, ey, S.opts.ellipseFill, plot);
}

function snapRect(tab, a, b) {
  let x0 = Math.min(a.x, b.x), x1 = Math.max(a.x, b.x);
  let y0 = Math.min(a.y, b.y), y1 = Math.max(a.y, b.y);
  if (S.opts.snap) {
    const cw = tab.cell.w, ch = tab.cell.h;
    x0 = Math.floor(x0 / cw) * cw; x1 = (Math.floor(x1 / cw) + 1) * cw - 1;
    y0 = Math.floor(y0 / ch) * ch; y1 = (Math.floor(y1 / ch) + 1) * ch - 1;
  }
  x0 = clamp(x0, 0, tab.w - 1); x1 = clamp(x1, 0, tab.w - 1);
  y0 = clamp(y0, 0, tab.h - 1); y1 = clamp(y1, 0, tab.h - 1);
  return { x: x0, y: y0, w: x1 - x0 + 1, h: y1 - y0 + 1 };
}

const inRect = (p, r) => p.x >= r.x && p.y >= r.y && p.x < r.x + r.w && p.y < r.y + r.h;

function toolDown(tab, p, e) {
  const tool = S.tool;
  if (tool === 'pick') { eyedrop(tab, p); return; }
  if (tool === 'select') { selectDown(tab, p); return; }
  commitFloat(tab);
  if (!canDraw(tab)) return;
  tab.lastCell = Math.max(0, C.cellNumberAt(clamp(p.x, 0, tab.w - 1), clamp(p.y, 0, tab.h - 1), tab.w, tab.h, tab.cell.w, tab.cell.h));
  if (tool === 'pen' || tool === 'eraser') {
    beginEdit(tab);
    const plot = makePlot(tab, toolValue(tab));
    plot(p.x, p.y);
    drag = { kind: 'pen', last: p, plot };
    changed(tab);
  } else if (tool === 'line' || tool === 'rect' || tool === 'ellipse') {
    beginEdit(tab);
    drag = { kind: 'shape', start: p, base: tab.edit.before };
    drawShape(tab, p, p, false);
    changed(tab);
  } else if (tool === 'fill') {
    if (p.x < 0 || p.y < 0 || p.x >= tab.w || p.y >= tab.h) return;
    beginEdit(tab);
    const d = curLayer(tab).data;
    const clip = drawClip(tab);
    if (S.opts.fillGlobal) C.replaceValue(d, tab.w, tab.h, d[p.y * tab.w + p.x], tab.fg, clip);
    else C.floodFill(d, tab.w, tab.h, p.x, p.y, tab.fg, clip);
    endEdit(tab);
    changed(tab);
  }
}

function toolMove(tab, p, e) {
  if (!drag) return;
  if (drag.kind === 'pen') {
    C.bresenham(drag.last.x, drag.last.y, p.x, p.y, drag.plot);
    drag.last = p;
    changed(tab);
  } else if (drag.kind === 'shape') {
    curLayer(tab).data.set(drag.base);
    drawShape(tab, drag.start, p, e.shiftKey);
    changed(tab);
  } else if (drag.kind === 'select') {
    tab.sel = snapRect(tab, drag.start, p);
    requestDraw();
  } else if (drag.kind === 'move') {
    const f = tab.float;
    f.x = drag.fx + (p.x - drag.sx);
    f.y = drag.fy + (p.y - drag.sy);
    applyFloat(tab);
  } else if (drag.kind === 'pan') {
    vp.scrollLeft = drag.l - (e.clientX - drag.cx);
    vp.scrollTop = drag.t - (e.clientY - drag.cy);
  }
}

function toolUp(tab, p) {
  if (!drag) return;
  const d = drag;
  drag = null;
  if (d.kind === 'pen' || d.kind === 'shape') {
    endEdit(tab);
    changed(tab);
  } else if (d.kind === 'select') {
    // クリックだけ（動かしていない・マスに合わせない）なら選択を外す
    if (tab.sel && tab.sel.w === 1 && tab.sel.h === 1 && !S.opts.snap && d.start.x === p.x && d.start.y === p.y) tab.sel = null;
    if (tab.sel) tab.lastCell = Math.max(0, C.cellNumberAt(tab.sel.x, tab.sel.y, tab.w, tab.h, tab.cell.w, tab.cell.h));
    requestDraw();
  } else if (d.kind === 'pan') {
    cv.classList.remove('panning');
  }
}

// ----------------------------------------------------------------
// 範囲選択・浮かせた絵
// ----------------------------------------------------------------

function selectDown(tab, p) {
  if (tab.sel && inRect(p, tab.sel)) {
    if (!canDraw(tab)) return;
    liftFloat(tab);
    drag = { kind: 'move', sx: p.x, sy: p.y, fx: tab.float.x, fy: tab.float.y };
  } else {
    commitFloat(tab);
    const q = { x: clamp(p.x, 0, tab.w - 1), y: clamp(p.y, 0, tab.h - 1) };
    tab.sel = snapRect(tab, q, q);
    drag = { kind: 'select', start: q };
    requestDraw();
  }
}

/** 選択範囲の絵を「浮かせる」。元の場所は空（一番下のレイヤーなら透明色）になる */
function liftFloat(tab) {
  if (tab.float || !tab.sel) return;
  const layer = curLayer(tab);
  const s = tab.sel;
  const before = layer.data.slice();
  const data = C.getRegion(layer.data, tab.w, tab.h, s.x, s.y, s.w, s.h);
  C.fillRect(layer.data, tab.w, tab.h, s.x, s.y, s.w, s.h, eraseValue(tab));
  tab.float = { x: s.x, y: s.y, w: s.w, h: s.h, data, base: layer.data.slice(), before, li: tab.cur };
  applyFloat(tab);
}

function applyFloat(tab) {
  const f = tab.float;
  const layer = tab.layers[f.li];
  layer.data.set(f.base);
  C.putRegion(layer.data, tab.w, tab.h, f.x, f.y, f.data, f.w, f.h, S.opts.skipTrans ? tab.transIdx : undefined);
  tab.sel = { x: f.x, y: f.y, w: f.w, h: f.h };
  changed(tab);
}

function commitFloat(tab) {
  const f = tab.float;
  if (!f) return;
  tab.float = null;
  const layer = tab.layers[f.li];
  const rect = C.diffRect(f.before, layer.data, tab.w, tab.h);
  tab.pasteAt = { x: f.x, y: f.y };
  if (rect) pushHistory(tab, { ops: [{ t: 'px', li: f.li, rect }] });
  changed(tab);
}

function cancelFloat(tab) {
  const f = tab.float;
  if (!f) return;
  tab.layers[f.li].data.set(f.before);
  tab.float = null;
  tab.sel = null;
  changed(tab);
}

function selectionOrAll(tab) {
  if (!tab.sel) tab.sel = { x: 0, y: 0, w: tab.w, h: tab.h };
  return tab.sel;
}

function copySelection(tab) {
  if (!tab.sel) { toast('先に範囲を選んでください'); return false; }
  const s = tab.sel;
  const data = tab.float ? tab.float.data.slice() : C.getRegion(curLayer(tab).data, tab.w, tab.h, s.x, s.y, s.w, s.h);
  S.clipboard = { w: s.w, h: s.h, data, palette: C.clonePalette(tab.palette) };
  toast('コピーしました');
  return true;
}

function clearSelection(tab) {
  if (!tab.sel) return;
  if (!canDraw(tab)) return;
  if (tab.float) {
    tab.float.data.fill(-1);
    applyFloat(tab);
    commitFloat(tab);
    return;
  }
  beginEdit(tab);
  const s = tab.sel;
  C.fillRect(curLayer(tab).data, tab.w, tab.h, s.x, s.y, s.w, s.h, eraseValue(tab));
  endEdit(tab);
  changed(tab);
}

function cutSelection(tab) {
  if (copySelection(tab)) clearSelection(tab);
}

function pasteClipboard(tab) {
  const cb = S.clipboard;
  if (!cb) { toast('貼るものがありません。先にコピーしてください'); return; }
  if (!canDraw(tab)) return;
  commitFloat(tab);
  let data = cb.data;
  if (!S.opts.keepIdx && !C.palettesEqual(cb.palette, tab.palette)) {
    data = C.mapRegionColors(data, C.buildColorMap(cb.palette, tab.palette));
  }
  // 置く場所: 前に貼った（動かした）場所があればそこ、なければ今見えている左上
  let pos = tab.pasteAt;
  if (!pos) {
    const { ox, oy } = origin(tab);
    pos = { x: Math.max(0, Math.ceil(-ox / tab.zoom)), y: Math.max(0, Math.ceil(-oy / tab.zoom)) };
  }
  const layer = curLayer(tab);
  const w = cb.w, h = cb.h;
  tab.float = { x: pos.x, y: pos.y, w, h, data: Int16Array.from(data), base: layer.data.slice(), before: layer.data.slice(), li: tab.cur };
  setTool('select');
  applyFloat(tab);
  toast('貼り付けました。ドラッグで動かして、Enter で確定します');
}

function transformSelection(kind) {
  const tab = S.active;
  if (!tab) return;
  if (!canDraw(tab)) return;
  const wasNone = !tab.sel;
  selectionOrAll(tab);
  const s = tab.sel;
  if (kind === 'rot' && s.w !== s.h) { toast('右90°回転は、正方形の選択だけでできます', 'ng'); if (wasNone) tab.sel = null; return; }
  liftFloat(tab);
  const f = tab.float;
  if (kind === 'h') f.data = C.flipH(f.data, f.w, f.h);
  else if (kind === 'v') f.data = C.flipV(f.data, f.w, f.h);
  else f.data = C.rotateCW(f.data, f.w, f.h).data;
  applyFloat(tab);
}

function replaceColor(scope) {
  const tab = S.active;
  if (!tab) return;
  const from = clamp(Number($('repFrom').value) || 0, 0, 255);
  const to = clamp(Number($('repTo').value) || 0, 0, 255);
  if (scope === 'sel' && !tab.sel) { toast('先に範囲を選んでください', 'ng'); return; }
  if (!canDraw(tab)) return;
  if (tab.float) {
    const n = C.replaceValue(tab.float.data, tab.float.w, tab.float.h, from, to);
    applyFloat(tab);
    toast(`${n} 個のドットを置き換えました`);
    return;
  }
  beginEdit(tab);
  const n = C.replaceValue(curLayer(tab).data, tab.w, tab.h, from, to, scope === 'sel' ? tab.sel : null);
  endEdit(tab);
  changed(tab);
  toast(n ? `${n} 個のドットを置き換えました` : '置き換えるドットはありませんでした');
}

// ----------------------------------------------------------------
// マウス・キーボード
// ----------------------------------------------------------------

cv.addEventListener('contextmenu', (e) => e.preventDefault()); // Mac の Ctrl+クリックでも右クリックメニューを出さない

cv.addEventListener('pointerdown', (e) => {
  const tab = S.active;
  if (!tab) return;
  e.preventDefault();
  cv.focus();
  const p = toImg(e);
  const mod = e.metaKey || e.ctrlKey;
  if (mod && (e.button === 0 || e.button === 2)) {
    // Cmd+左 = 拡大（押した点が真ん中に来る）、Cmd+右 = 縮小
    const dir = e.button === 0 ? 1 : -1;
    setZoom(tab, stepZoom(tab, dir), p.fx, p.fy, vp.clientWidth / 2, vp.clientHeight / 2);
    return;
  }
  cv.setPointerCapture(e.pointerId);
  if (e.button === 1 || (e.button === 0 && (S.spaceDown || S.tool === 'hand'))) {
    drag = { kind: 'pan', cx: e.clientX, cy: e.clientY, l: vp.scrollLeft, t: vp.scrollTop };
    cv.classList.add('panning');
    return;
  }
  if (e.button === 2) { eyedrop(tab, p); return; } // 右クリックはどのツールでもスポイト
  if (e.button === 0) toolDown(tab, p, e);
});

cv.addEventListener('pointermove', (e) => {
  const tab = S.active;
  if (!tab) return;
  const p = toImg(e);
  S.hover = p;
  if (drag) toolMove(tab, p, e);
  requestDraw();
});

cv.addEventListener('pointerleave', () => { if (!drag) { S.hover = null; requestDraw(); } });

function endPointer(e) {
  const tab = S.active;
  if (!tab || !drag) return;
  toolUp(tab, toImg(e));
  requestDraw();
}
cv.addEventListener('pointerup', endPointer);
cv.addEventListener('pointercancel', endPointer);

// Cmd+ホイールでカーソルの位置を中心に拡大縮小
vp.addEventListener('wheel', (e) => {
  const tab = S.active;
  if (!tab || !(e.metaKey || e.ctrlKey)) return;
  e.preventDefault();
  const dir = wheelZoomDir(e);
  if (!dir) return;
  const p = toImgF(e);
  setZoom(tab, stepZoom(tab, dir), p.fx, p.fy, p.sx, p.sy);
}, { passive: false });

// ホイールの回転量をためて、一定量を超えたら拡大(1)・縮小(-1)を返す。まだなら 0
const wheelAcc = { sum: 0, last: 0, stepAt: 0 };
let wheelZoomSpeed = 'normal';
try { wheelZoomSpeed = localStorage.getItem(WHEEL_ZOOM_SPEED_KEY) || 'normal'; } catch (_) { /* 保存できない時は「ふつう」 */ }
{
  const sel = $('wheelZoomSpeed');
  if (sel) {
    if (!(wheelZoomSpeed in WHEEL_ZOOM_SPEEDS)) wheelZoomSpeed = 'normal';
    sel.value = wheelZoomSpeed;
    sel.addEventListener('change', () => {
      wheelZoomSpeed = sel.value;
      try { localStorage.setItem(WHEEL_ZOOM_SPEED_KEY, wheelZoomSpeed); } catch (_) { /* 無視 */ }
    });
  }
}
function wheelZoomDir(e) {
  const now = performance.now();
  // 行・ページ単位で来るマウスもあるので px にそろえる
  const unit = e.deltaMode === 1 ? 16 : e.deltaMode === 2 ? vp.clientHeight : 1;
  const dy = e.deltaY * unit;
  if (now - wheelAcc.last > WHEEL_ZOOM_IDLE || Math.sign(dy) !== Math.sign(wheelAcc.sum)) wheelAcc.sum = 0;
  wheelAcc.last = now;
  wheelAcc.sum += dy;
  if (now - wheelAcc.stepAt < WHEEL_ZOOM_COOLDOWN) return 0;
  const base = (e.ctrlKey && !e.metaKey) ? PINCH_ZOOM_STEP : WHEEL_ZOOM_STEP;
  const step = base * (WHEEL_ZOOM_SPEEDS[wheelZoomSpeed] ?? 1);
  if (Math.abs(wheelAcc.sum) < step) return 0;
  const dir = wheelAcc.sum < 0 ? 1 : -1;
  wheelAcc.sum = 0;
  wheelAcc.stepAt = now;
  return dir;
}

vp.addEventListener('scroll', () => requestDraw());
new ResizeObserver(() => { resizeCanvas(); }).observe(vp);

// 全体図: クリック・ドラッグで本体を動かす
$('ovWrap').addEventListener('pointerdown', (e) => {
  e.preventDefault();
  $('ovWrap').setPointerCapture(e.pointerId);
  $('ovWrap').dataset.drag = '1';
  ovScrollTo(e);
});
$('ovWrap').addEventListener('pointermove', (e) => { if ($('ovWrap').dataset.drag) ovScrollTo(e); });
$('ovWrap').addEventListener('pointerup', () => { delete $('ovWrap').dataset.drag; });

function setTool(t) {
  if (S.tool === t) return;
  const tab = S.active;
  if (tab && t !== 'select') commitFloat(tab);
  S.tool = t;
  renderTools();
  syncCursor();
  syncStatus();
  requestDraw();
}

function syncCursor() {
  cv.classList.toggle('pan', S.spaceDown || S.tool === 'hand');
  cv.classList.toggle('pick', !(S.spaceDown || S.tool === 'hand'));
}

document.addEventListener('keydown', (e) => {
  if (e.isComposing) return;
  const tab = S.active;
  const mod = e.metaKey || e.ctrlKey;
  const typing = isTyping(e.target);
  const key = e.key.toLowerCase();

  if (mod) {
    if (key === 's') { e.preventDefault(); if (e.shiftKey) saveDraftNow(tab, false); else saveToFile(tab); return; }
    if (typing) return; // 入力欄の中のコピー・貼り付けなどは邪魔しない
    if (key === 'z') { e.preventDefault(); if (e.shiftKey) redo(tab); else undo(tab); return; }
    if (key === 'y') { e.preventDefault(); redo(tab); return; }
    if (!tab) return;
    if (key === 'c') { e.preventDefault(); copySelection(tab); return; }
    if (key === 'x') { e.preventDefault(); cutSelection(tab); return; }
    if (key === 'v') { e.preventDefault(); pasteClipboard(tab); return; }
    if (key === 'a') { e.preventDefault(); commitFloat(tab); tab.sel = { x: 0, y: 0, w: tab.w, h: tab.h }; requestDraw(); return; }
    if (key === 'd') { e.preventDefault(); commitFloat(tab); tab.sel = null; requestDraw(); return; }
    return;
  }
  if (typing) return;

  if (e.code === 'Space') {
    e.preventDefault();
    if (!S.spaceDown) { S.spaceDown = true; syncCursor(); }
    return;
  }
  if (e.key === '?' || (e.key === '/' && e.shiftKey)) { e.preventDefault(); openHelp(); return; }
  if (!tab) return;

  if (e.shiftKey && (e.code === 'KeyU' || e.code === 'KeyO')) {
    // Shift+U / Shift+O は、塗りつぶしのある四角・円
    e.preventDefault();
    if (e.code === 'KeyU') { S.opts.rectFill = true; setTool('rect'); } else { S.opts.ellipseFill = true; setTool('ellipse'); }
    renderToolOpts();
    return;
  }
  const toolKeys = { KeyB: 'pen', KeyE: 'eraser', KeyL: 'line', KeyU: 'rect', KeyO: 'ellipse', KeyG: 'fill', KeyI: 'pick', KeyM: 'select', KeyH: 'hand' };
  if (!e.shiftKey && toolKeys[e.code]) { e.preventDefault(); setTool(toolKeys[e.code]); return; }
  if (e.key === '+' || e.key === '=' || e.key === ';') { e.preventDefault(); zoomAtCenter(1); return; }
  if (e.key === '-') { e.preventDefault(); zoomAtCenter(-1); return; }
  if (e.key === '[') { e.preventDefault(); setFg(tab, tab.fg - 1); return; }
  if (e.key === ']') { e.preventDefault(); setFg(tab, tab.fg + 1); return; }
  if (e.key === 'Escape') {
    if (tab.float) commitFloat(tab);
    else if (tab.sel) tab.sel = null;
    requestDraw();
    return;
  }
  if (e.key === 'Enter') { if (tab.float) { e.preventDefault(); commitFloat(tab); requestDraw(); } return; }
  if (e.key === 'Delete' || e.key === 'Backspace') { if (tab.sel) { e.preventDefault(); clearSelection(tab); } return; }
  if (e.key.startsWith('Arrow') && tab.sel) {
    e.preventDefault();
    if (!canDraw(tab)) return;
    const step = e.shiftKey ? (e.key === 'ArrowLeft' || e.key === 'ArrowRight' ? tab.cell.w : tab.cell.h) : 1;
    liftFloat(tab);
    if (e.key === 'ArrowLeft') tab.float.x -= step;
    if (e.key === 'ArrowRight') tab.float.x += step;
    if (e.key === 'ArrowUp') tab.float.y -= step;
    if (e.key === 'ArrowDown') tab.float.y += step;
    applyFloat(tab);
  }
});

document.addEventListener('keyup', (e) => {
  if (e.code === 'Space') { S.spaceDown = false; syncCursor(); }
});
window.addEventListener('blur', () => { S.spaceDown = false; syncCursor(); });

window.addEventListener('beforeunload', (e) => {
  if (S.tabs.some((t) => t.dirty)) { e.preventDefault(); e.returnValue = ''; }
});
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'hidden') for (const t of S.tabs) if (t.dirty) autosaveNow(t);
});

// ----------------------------------------------------------------
// 左: ツール表示・オプション
// ----------------------------------------------------------------

function renderTools() {
  const box = $('tools');
  box.textContent = '';
  for (const [id, name, key] of TOOLS) {
    box.append(el('button', { class: S.tool === id ? 'active' : '', onclick: () => setTool(id), title: `${name} (${key})` }, name, el('kbd', {}, key)));
  }
  renderToolOpts();
}

function renderToolOpts() {
  const box = $('toolOpts');
  box.textContent = '';
  const chk = (label, key) => el('label', { class: 'chk' },
    el('input', { type: 'checkbox', ...(S.opts[key] ? { checked: true } : {}), onchange: (e) => { S.opts[key] = e.target.checked; } }), label);
  if (S.tool === 'rect') box.append(el('div', { class: 'opt-group' }, chk('塗りつぶす (Shift+U)', 'rectFill'), el('div', { class: 'sub-text' }, 'Shift を押しながらで正方形')));
  if (S.tool === 'ellipse') box.append(el('div', { class: 'opt-group' }, chk('塗りつぶす (Shift+O)', 'ellipseFill'), el('div', { class: 'sub-text' }, 'Shift を押しながらで正円')));
  if (S.tool === 'line') box.append(el('div', { class: 'opt-group' }, el('div', { class: 'sub-text' }, 'Shift を押しながらで 0° / 45° / 90°')));
  if (S.tool === 'fill') box.append(el('div', { class: 'opt-group' }, chk('全体で同じ色を置き換え', 'fillGlobal')));
  if (S.tool === 'eraser') box.append(el('div', { class: 'opt-group' }, el('div', { class: 'sub-text' }, '一番下のレイヤーでは透明色（黒）を塗り、上のレイヤーでは「何もない」に戻します')));
  if (S.tool === 'select') box.append(el('div', { class: 'opt-group' }, el('div', { class: 'sub-text' }, '選択の中をドラッグで動かす / 矢印キーで 1 ドット（Shift でマス単位）')));
  box.append(el('div', { class: 'sub-text', style: 'margin-top:6px' }, '右クリック = スポイト / Cmd+クリック = 拡大 / Cmd+右クリック = 縮小 / スペースを押しながらドラッグ = 移動'));
}

function bindOption(id, key, after) {
  const e = $(id);
  e.checked = S.opts[key];
  e.addEventListener('change', () => { S.opts[key] = e.checked; if (after) after(); requestDraw(); });
}

// ----------------------------------------------------------------
// 右: パレット
// ----------------------------------------------------------------

const PCELL = 18;
function renderPalette() {
  palDirty = false;
  const tab = S.active;
  const c = $('pal');
  const g = c.getContext('2d');
  g.clearRect(0, 0, c.width, c.height);
  const info = $('palInfo');
  info.textContent = '';
  if (!tab) return;
  for (let i = 0; i < 256; i++) {
    const col = tab.palette[i];
    const x = (i % 16) * PCELL;
    const y = Math.floor(i / 16) * PCELL;
    g.fillStyle = `rgb(${col.r},${col.g},${col.b})`;
    g.fillRect(x, y, PCELL, PCELL);
    if (C.isBlack(col)) {
      // 透明色: 小さな市松模様の印
      g.fillStyle = '#555';
      g.fillRect(x + 2, y + 2, 4, 4); g.fillRect(x + 6, y + 6, 4, 4);
    }
    if (S.opts.used && tab.used && tab.used[i]) {
      g.fillStyle = '#000'; g.fillRect(x + PCELL - 7, y + PCELL - 7, 6, 6);
      g.fillStyle = '#fff'; g.fillRect(x + PCELL - 6, y + PCELL - 6, 4, 4);
    }
  }
  g.strokeStyle = 'rgba(0,0,0,0.5)';
  g.lineWidth = 1;
  g.beginPath();
  for (let i = 0; i <= 16; i++) { g.moveTo(i * PCELL + 0.5, 0); g.lineTo(i * PCELL + 0.5, 288); g.moveTo(0, i * PCELL + 0.5); g.lineTo(288, i * PCELL + 0.5); }
  g.stroke();
  // 範囲
  if (tab.palRange) {
    const [a, b] = tab.palRange;
    g.strokeStyle = '#f0b84a';
    g.lineWidth = 2;
    for (let i = Math.min(a, b); i <= Math.max(a, b); i++) g.strokeRect((i % 16) * PCELL + 1, Math.floor(i / 16) * PCELL + 1, PCELL - 2, PCELL - 2);
  }
  // 現在の色
  const cx = (tab.fg % 16) * PCELL;
  const cy = Math.floor(tab.fg / 16) * PCELL;
  g.lineWidth = 2;
  g.strokeStyle = '#000'; g.strokeRect(cx + 1, cy + 1, PCELL - 2, PCELL - 2);
  g.strokeStyle = '#fff'; g.strokeRect(cx + 2.5, cy + 2.5, PCELL - 5, PCELL - 5);

  const col = tab.palette[tab.fg];
  info.append(
    el('div', { class: 'sw', style: `background:rgb(${col.r},${col.g},${col.b})` }),
    el('div', {}, `番号 ${tab.fg}　R${col.r} G${col.g} B${col.b}`, C.isBlack(col) ? el('span', { class: 'sub-text' }, '（透明色）') : null));
  if (!$('palEdit').hidden) syncPalEdit();
}

function palIndexAt(e) {
  const r = $('pal').getBoundingClientRect();
  const x = clamp(Math.floor(((e.clientX - r.left) / r.width) * 16), 0, 15);
  const y = clamp(Math.floor(((e.clientY - r.top) / r.height) * 16), 0, 15);
  return y * 16 + x;
}

$('pal').addEventListener('click', (e) => {
  const tab = S.active;
  if (!tab) return;
  const i = palIndexAt(e);
  if (e.shiftKey) {
    tab.palRange = [tab.palAnchor, i];
    tab.fg = i;
    renderPalette();
  } else {
    setFg(tab, i);
  }
});
$('pal').addEventListener('dblclick', (e) => {
  const tab = S.active;
  if (!tab) return;
  setFg(tab, palIndexAt(e));
  $('palEdit').hidden = false;
  syncPalEdit();
});
$('pal').addEventListener('pointermove', (e) => {
  const tab = S.active;
  if (!tab) return;
  const i = palIndexAt(e);
  const c = tab.palette[i];
  $('pal').title = `番号 ${i}  (${c.r}, ${c.g}, ${c.b})`;
});

function syncPalEdit() {
  const tab = S.active;
  if (!tab) return;
  const c = tab.palette[tab.fg];
  $('peColor').value = '#' + hex2(c.r) + hex2(c.g) + hex2(c.b);
  if (document.activeElement !== $('peR')) $('peR').value = c.r;
  if (document.activeElement !== $('peG')) $('peG').value = c.g;
  if (document.activeElement !== $('peB')) $('peB').value = c.b;
  $('peLabel').textContent = `番号 ${tab.fg} の色を変えています`;
}

/** 色を変える。入力中は画面に反映するだけにして、確定した時に履歴へ 1 つ入れる */
function editColor(r, g, b) {
  const tab = S.active;
  if (!tab) return;
  if (!tab.palEditBefore) tab.palEditBefore = C.clonePalette(tab.palette);
  tab.palette[tab.fg] = { r: clamp(r | 0, 0, 255), g: clamp(g | 0, 0, 255), b: clamp(b | 0, 0, 255), a: 255 };
  changed(tab, { palette: true });
  renderPalette();
}
function commitColorEdit() {
  const tab = S.active;
  if (!tab || !tab.palEditBefore) return;
  const before = tab.palEditBefore;
  tab.palEditBefore = null;
  if (C.palettesEqual(before, tab.palette)) return;
  pushHistory(tab, { ops: [{ t: 'pal', before, after: C.clonePalette(tab.palette) }] });
}
$('peColor').addEventListener('input', (e) => {
  const v = e.target.value;
  editColor(parseInt(v.slice(1, 3), 16), parseInt(v.slice(3, 5), 16), parseInt(v.slice(5, 7), 16));
});
for (const id of ['peR', 'peG', 'peB']) $(id).addEventListener('input', () => editColor(Number($('peR').value), Number($('peG').value), Number($('peB').value)));
for (const id of ['peColor', 'peR', 'peG', 'peB']) $(id).addEventListener('change', commitColorEdit);
$('peClose').addEventListener('click', () => { commitColorEdit(); $('palEdit').hidden = true; });

$('btnGradient').addEventListener('click', () => {
  const tab = S.active;
  if (!tab) return;
  if (!tab.palRange || tab.palRange[0] === tab.palRange[1]) { toast('先にパレットで、クリックしてから Shift+クリックで範囲を選んでください', 'ng'); return; }
  const before = C.clonePalette(tab.palette);
  tab.palette = C.gradientPalette(tab.palette, tab.palRange[0], tab.palRange[1]);
  pushHistory(tab, { ops: [{ t: 'pal', before, after: C.clonePalette(tab.palette) }] });
  changed(tab, { palette: true });
  renderPalette();
});
$('repFromCur').addEventListener('click', () => { if (S.active) $('repFrom').value = S.active.fg; });
$('repToCur').addEventListener('click', () => { if (S.active) $('repTo').value = S.active.fg; });
$('btnRepLayer').addEventListener('click', () => replaceColor('layer'));
$('btnRepSel').addEventListener('click', () => replaceColor('sel'));
$('btnSelReplace').addEventListener('click', () => replaceColor('sel'));

// ----------------------------------------------------------------
// 右: レイヤー
// ----------------------------------------------------------------

function renderLayers() {
  const tab = S.active;
  const box = $('layerList');
  box.textContent = '';
  if (!tab) return;
  for (let i = tab.layers.length - 1; i >= 0; i--) {
    const l = tab.layers[i];
    const row = el('div', { class: 'layer' + (i === tab.cur ? ' cur' : ''), onclick: () => selectLayer(i) },
      el('button', { class: 'sub mini eye' + (l.visible ? '' : ' off'), title: '表示 / 非表示', onclick: (e) => { e.stopPropagation(); l.visible = !l.visible; markDirty(tab); changed(tab); renderLayers(); } }, l.visible ? '目' : '消'),
      el('span', { class: 'nm', title: 'ダブルクリックで名前を変える', ondblclick: (e) => { e.stopPropagation(); renameLayer(i, e.currentTarget); } }, l.name),
      el('label', { class: 'chk', title: '下絵として半透明で見せる（見た目だけ）', onclick: (e) => e.stopPropagation() },
        el('input', { type: 'checkbox', ...(l.ghost ? { checked: true } : {}), onchange: (e) => { l.ghost = e.target.checked; markDirty(tab); changed(tab); } }), '下絵'));
    box.append(row);
  }
}

function selectLayer(i) {
  const tab = S.active;
  if (!tab || i === tab.cur) return;
  commitFloat(tab);
  tab.cur = i;
  tab.sel = tab.sel; // 選択範囲は残す
  renderLayers();
  syncStatus();
}

function renameLayer(i, nameEl) {
  const tab = S.active;
  const input = el('input', { type: 'text', class: 'rename', value: tab.layers[i].name });
  nameEl.replaceWith(input);
  input.focus();
  input.select();
  const done = (ok) => {
    if (ok && input.value.trim()) { tab.layers[i].name = input.value.trim(); markDirty(tab); }
    renderLayers();
    syncStatus();
  };
  input.addEventListener('keydown', (e) => { e.stopPropagation(); if (e.key === 'Enter') done(true); if (e.key === 'Escape') done(false); });
  input.addEventListener('blur', () => done(true));
}

function layerAdd() {
  const tab = S.active;
  if (!tab) return;
  structOp(tab, () => {
    const n = tab.layers.length;
    tab.layers.splice(tab.cur + 1, 0, { name: `レイヤー ${n}`, visible: true, ghost: false, data: new Int16Array(tab.w * tab.h).fill(-1) });
    tab.cur = tab.cur + 1;
  });
}
function layerDup() {
  const tab = S.active;
  if (!tab) return;
  structOp(tab, () => {
    const l = curLayer(tab);
    tab.layers.splice(tab.cur + 1, 0, { name: l.name + ' のコピー', visible: l.visible, ghost: l.ghost, data: l.data.slice() });
    tab.cur = tab.cur + 1;
  });
}
function layerDel() {
  const tab = S.active;
  if (!tab) return;
  if (tab.layers.length <= 1) { toast('最後の 1 枚は消せません', 'ng'); return; }
  structOp(tab, () => {
    tab.layers.splice(tab.cur, 1);
    tab.cur = Math.min(tab.cur, tab.layers.length - 1);
  });
}
function layerMove(dir) {
  const tab = S.active;
  if (!tab) return;
  const j = tab.cur + dir;
  if (j < 0 || j >= tab.layers.length) return;
  structOp(tab, () => {
    [tab.layers[tab.cur], tab.layers[j]] = [tab.layers[j], tab.layers[tab.cur]];
    tab.cur = j;
  });
}
function layerMerge() {
  const tab = S.active;
  if (!tab) return;
  if (tab.cur === 0) { toast('一番下のレイヤーは、下に結合する先がありません', 'ng'); return; }
  structOp(tab, () => {
    const up = tab.layers[tab.cur];
    const low = tab.layers[tab.cur - 1];
    for (let i = 0; i < up.data.length; i++) if (up.data[i] >= 0) low.data[i] = up.data[i];
    tab.layers.splice(tab.cur, 1);
    tab.cur = tab.cur - 1;
  });
}
$('lyAdd').addEventListener('click', layerAdd);
$('lyDup').addEventListener('click', layerDup);
$('lyDel').addEventListener('click', layerDel);
$('lyUp').addEventListener('click', () => layerMove(1));
$('lyDown').addEventListener('click', () => layerMove(-1));
$('lyMerge').addEventListener('click', layerMerge);

// ----------------------------------------------------------------
// 控え（一時保存）
// ----------------------------------------------------------------

function thumbOf(tab) {
  refreshComposite(tab);
  const k = Math.min(1, 96 / Math.max(tab.w, tab.h));
  const c = document.createElement('canvas');
  c.width = Math.max(1, Math.round(tab.w * k));
  c.height = Math.max(1, Math.round(tab.h * k));
  const g = c.getContext('2d');
  g.fillStyle = '#000';
  g.fillRect(0, 0, c.width, c.height);
  g.imageSmoothingEnabled = false;
  g.drawImage(tab.off, 0, 0, c.width, c.height);
  return c.toDataURL('image/png');
}

function draftRecord(tab, { id, name, auto = false, kind = 'manual' }) {
  return {
    id, file: tab.fileName, name, savedAt: Date.now(), width: tab.w, height: tab.h,
    palette: C.clonePalette(tab.palette),
    layers: tab.layers.map((l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, data: l.data.slice() })),
    thumbnail: thumbOf(tab), auto, kind,
  };
}
const newId = (p) => `${p}-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 7)}`;

async function saveDraftNow(tab, asNew) {
  if (!tab) return;
  try {
    commitFloat(tab);
    const overwrite = !asNew && tab.draftId;
    const id = overwrite ? tab.draftId : newId('d');
    const name = overwrite && tab.draftName ? tab.draftName : `${tab.fileName} ${stamp()}`;
    await store.putDraft(draftRecord(tab, { id, name }));
    tab.draftId = id;
    tab.draftName = name;
    tab.dirty = false; // 控えに残したので、うっかり閉じても戻せる
    renderTabs();
    await clearAutosave(tab);
    await refreshDraftIndex();
    renderFileList();
    renderDrafts();
    toast(overwrite ? `一時保存を上書きしました（${name}）` : `一時保存しました（${name}）`, 'ok');
  } catch (err) {
    console.error(err);
    toast('一時保存できませんでした: ' + err.message, 'ng');
  }
}

function scheduleAutosave(tab) {
  clearTimeout(tab.autoTimer);
  tab.autoTimer = setTimeout(() => autosaveNow(tab), 3000);
}

async function autosaveNow(tab) {
  clearTimeout(tab.autoTimer);
  if (!tab.dirty) return;
  try {
    if (!tab.autoId) tab.autoId = newId('auto');
    await store.putDraft(draftRecord(tab, { id: tab.autoId, name: `自動の控え: ${tab.label} ${stamp()}`, auto: true, kind: 'auto' }));
  } catch (err) {
    console.error('自動の控えに失敗', err);
  }
}

async function clearAutosave(tab) {
  clearTimeout(tab.autoTimer);
  if (!tab.autoId) return;
  const id = tab.autoId;
  tab.autoId = null;
  try { await store.deleteDraft(id); } catch (err) { console.error(err); }
}

async function refreshDraftIndex() {
  try {
    const all = await store.listDrafts();
    S.draftIndex = new Map();
    for (const d of all) if (!d.auto) S.draftIndex.set(d.file, (S.draftIndex.get(d.file) || 0) + 1);
  } catch (err) { console.error(err); }
}

async function renderDrafts() {
  const box = $('drList');
  let all = [];
  try { all = await store.listDrafts(); } catch (err) { box.textContent = '控えを読めませんでした: ' + err.message; return; }
  const tab = S.active;
  const list = all.filter((d) => S.showAllDrafts || (tab && d.file === tab.fileName));
  box.textContent = '';
  if (!list.length) {
    box.append(el('div', { class: 'sub-text' }, S.showAllDrafts ? '控えはまだありません' : 'このファイルの控えはまだありません'));
    return;
  }
  for (const d of list) {
    const nameEl = el('div', { class: 'nm' }, d.name);
    const row = el('div', { class: 'draft' },
      el('div', { class: 'th' }, d.thumbnail ? el('img', { src: d.thumbnail }) : null),
      el('div', { class: 'info' },
        d.auto ? el('span', { class: 'tag auto' }, '自動') : null,
        d.kind === 'orig' ? el('span', { class: 'tag orig' }, '元の絵') : null,
        nameEl,
        el('div', { class: 'sub-text' }, `${d.file}　${stamp(new Date(d.savedAt))}`),
        el('div', { class: 'btns' },
          el('button', { class: 'mini', onclick: () => { openDraftRecord(d); } }, '開く'),
          el('button', { class: 'sub mini', onclick: () => startDraftRename(d, nameEl) }, '名前変更'),
          el('button', { class: 'sub mini', onclick: () => exportDraftPng(d) }, 'PNG を書き出す'),
          (() => {
            const b = el('button', { class: 'danger mini' }, '削除');
            twoStep(b, 'もう一度押すと削除', async () => { await store.deleteDraft(d.id); await refreshDraftIndex(); renderFileList(); renderDrafts(); });
            return b;
          })())));
    box.append(row);
  }
}

function startDraftRename(d, nameEl) {
  const input = el('input', { type: 'text', value: d.name, style: 'width:100%' });
  nameEl.replaceWith(input);
  input.focus();
  input.select();
  let finished = false;
  const done = async (ok) => {
    if (finished) return;
    finished = true;
    if (ok && input.value.trim() && input.value.trim() !== d.name) {
      d.name = input.value.trim();
      await store.putDraft(d);
      for (const t of S.tabs) if (t.draftId === d.id) { t.draftName = d.name; t.label = d.name; }
      renderTabs();
    }
    renderDrafts();
  };
  input.addEventListener('keydown', (e) => { e.stopPropagation(); if (e.key === 'Enter') done(true); if (e.key === 'Escape') done(false); });
  input.addEventListener('blur', () => done(true));
}

async function exportDraftPng(d) {
  try {
    const palette = C.normalizePalette(d.palette);
    const t = C.findTransparentIndex(palette);
    const bytes = await encodeIndexedPng({ width: d.width, height: d.height, indices: C.flatten(d.layers, d.width, d.height, t), palette });
    download(bytes, `${baseName(d.file)}_${d.name.replace(/[\\/:*?"<>|\s]+/g, '_')}.png`);
  } catch (err) {
    toast('書き出せませんでした: ' + err.message, 'ng');
  }
}

async function offerRestore() {
  let all = [];
  try { all = (await store.listDrafts()).filter((d) => d.auto); } catch { return; }
  const bar = $('restoreBar');
  bar.textContent = '';
  if (!all.length) { bar.hidden = true; return; }
  bar.hidden = false;
  bar.append(el('div', {}, '前回の自動の控えが残っています。続きから開けます。'));
  for (const d of all) {
    const item = el('div', { class: 'item' },
      el('span', {}, `${d.file}（${stamp(new Date(d.savedAt))}）`),
      el('button', { class: 'mini', onclick: () => { openDraftRecord(d); item.remove(); if (!bar.querySelector('.item')) bar.hidden = true; } }, '開く'),
      el('button', { class: 'danger mini', onclick: async () => { await store.deleteDraft(d.id); item.remove(); if (!bar.querySelector('.item')) bar.hidden = true; } }, '捨てる'));
    bar.append(item);
  }
}

// ----------------------------------------------------------------
// ファイルへ保存
// ----------------------------------------------------------------

function askSaveTarget(tab) {
  return new Promise((resolve) => {
    const dlg = $('targetDlg');
    const sel = $('targetSel');
    sel.textContent = '';
    const names = S.dir.files.map((f) => f.name);
    if (!names.includes(tab.fileName)) names.unshift(tab.fileName);
    for (const n of names) sel.append(el('option', { value: n }, n));
    sel.value = tab.fileName;
    $('targetMsg').textContent = `「${tab.label}」は控えから開いた絵です。どのファイルに上書きしますか？（元のファイルが選ばれています）`;
    const finish = (v) => { dlg.close(); resolve(v); };
    $('targetOk').onclick = () => finish(sel.value);
    $('targetCancel').onclick = () => finish(null);
    dlg.oncancel = () => resolve(null);
    dlg.showModal();
  });
}

/** 初めて上書きする前に、元のファイルの中身を「控え」に残す */
async function backupOriginal(name, handle) {
  if (S.backedUp.has(name)) return;
  S.backedUp.add(name);
  try {
    const file = await handle.getFile();
    const bytes = new Uint8Array(await file.arrayBuffer());
    const img = await decodeIndexedPng(bytes);
    const palette = C.normalizePalette(img.palette);
    const fake = {
      fileName: name, w: img.width, h: img.height, palette,
      layers: [{ name: '元の絵', visible: true, ghost: false, data: Int16Array.from(img.indices) }],
      transIdx: C.findTransparentIndex(palette), off: null,
    };
    // サムネイルは小さなキャンバスで作り直す
    const c = document.createElement('canvas');
    c.width = img.width; c.height = img.height;
    const g = c.getContext('2d');
    g.fillStyle = '#000'; g.fillRect(0, 0, c.width, c.height);
    const t = document.createElement('canvas');
    t.width = img.width; t.height = img.height;
    t.getContext('2d').putImageData(new ImageData(new Uint8ClampedArray(rgbaOf(img.indices, palette, C.findTransparentIndex(palette))), img.width, img.height), 0, 0);
    g.drawImage(t, 0, 0);
    const rec = {
      id: newId('orig'), file: name, name: `保存前の元の絵 ${stamp()}`, savedAt: Date.now(),
      width: img.width, height: img.height, palette, layers: fake.layers, thumbnail: c.toDataURL('image/png'),
      auto: false, kind: 'orig', originalBytes: bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
    };
    await store.putDraft(rec);
    await refreshDraftIndex();
  } catch (err) {
    console.error('元の絵の控えに失敗', err);
    S.backedUp.delete(name);
    throw err;
  }
}

function rgbaOf(indices, palette, transIdx) {
  const out = new Uint8ClampedArray(indices.length * 4);
  for (let i = 0; i < indices.length; i++) {
    const c = palette[indices[i]];
    out[i * 4] = c.r; out[i * 4 + 1] = c.g; out[i * 4 + 2] = c.b;
    out[i * 4 + 3] = C.isBlack(c) ? 0 : 255;
  }
  return out;
}

async function saveToFile(tab) {
  if (!tab) return;
  try {
    commitFloat(tab);
    let name = tab.fileName;
    let handle = tab.handle;
    if (tab.fromDraft && S.dir) {
      const chosen = await askSaveTarget(tab);
      if (!chosen) return;
      name = chosen;
      handle = S.dir.files.find((f) => f.name === chosen)?.handle || null;
    }
    const bytes = await encodeTab(tab);
    if (!handle || IS_TEST) {
      download(bytes, name);
      toast(handle || IS_TEST ? 'ダウンロードしました' : 'フォルダを開いていないので、ダウンロードとして保存しました', 'ok');
    } else {
      try {
        await backupOriginal(name, handle);
      } catch {
        toast('元の絵を控えに残せませんでしたが、保存は続けます', 'ng');
      }
      const w = await handle.createWritable();
      await w.write(bytes);
      await w.close();
      toast(`${name} に保存しました`, 'ok');
      const entry = S.dir && S.dir.files.find((f) => f.name === name);
      if (entry) { entry.size = bytes.length; entry.thumb = null; entry.bad = false; loadThumbs(); }
    }
    tab.dirty = false;
    tab.unwritten = false;
    if (tab.fromDraft) {
      tab.fromDraft = false;
      tab.fileName = name;
      tab.handle = handle;
      tab.label = name;
    }
    await clearAutosave(tab);
    renderTabs();
    renderFileList();
  } catch (err) {
    console.error(err);
    toast('保存できませんでした: ' + err.message, 'ng');
  }
}

// ----------------------------------------------------------------
// 作業フォルダ・ファイル一覧
// ----------------------------------------------------------------

async function loadFolder(handle) {
  let dir = handle;
  // SboGrpData/grp が中にあれば、そこまで自動で降りる
  try {
    const sgd = await handle.getDirectoryHandle('SboGrpData');
    dir = await sgd.getDirectoryHandle('grp');
  } catch {
    try { dir = await handle.getDirectoryHandle('grp'); } catch { /* そのまま */ }
  }
  const files = [];
  for await (const [name, h] of dir.entries()) {
    if (h.kind === 'file' && /\.png$/i.test(name)) {
      let size = 0;
      try { size = (await h.getFile()).size; } catch { /* 読めなくても一覧には出す */ }
      files.push({ name, handle: h, size, thumb: null, bad: false });
    }
  }
  files.sort((a, b) => a.name.localeCompare(b.name, 'ja'));
  S.dir = { handle, dir, name: dir === handle ? handle.name : `${handle.name}/…/${dir.name}`, files };
  // 開いているタブにも、同じ名前なら書き込み先をつなぎ直す
  for (const t of S.tabs) {
    if (!t.handle) { const f = files.find((x) => x.name === t.fileName); if (f) t.handle = f.handle; }
  }
  renderFileList();
  loadThumbs();
  renderDrafts();
}

let thumbRunning = false;
async function loadThumbs() {
  if (thumbRunning || !S.dir) return;
  thumbRunning = true;
  try {
    for (const f of S.dir.files) {
      if (f.thumb || f.bad) continue;
      try {
        const bytes = new Uint8Array(await (await f.handle.getFile()).arrayBuffer());
        const img = await decodeIndexedPng(bytes);
        const pal = C.normalizePalette(img.palette);
        const c = document.createElement('canvas');
        c.width = img.width; c.height = img.height;
        c.getContext('2d').putImageData(new ImageData(rgbaOf(img.indices, pal), img.width, img.height), 0, 0);
        const k = Math.min(1, 64 / img.width, 48 / img.height);
        const t = document.createElement('canvas');
        t.width = Math.max(1, Math.round(img.width * k));
        t.height = Math.max(1, Math.round(img.height * k));
        t.getContext('2d').drawImage(c, 0, 0, t.width, t.height);
        f.thumb = t.toDataURL('image/png');
      } catch {
        f.bad = true; // パレット PNG でないなど。開けない印だけ付ける
      }
      renderFileList();
      await new Promise((r) => setTimeout(r, 0));
    }
  } finally {
    thumbRunning = false;
  }
}

function fmtSize(n) { return n >= 1024 ? (n / 1024).toFixed(1) + 'KB' : n + 'B'; }

function renderFileList() {
  const box = $('fileList');
  box.textContent = '';
  $('dirName').textContent = S.dir ? `フォルダ: ${S.dir.name}（${S.dir.files.length} 枚）` : 'フォルダはまだ開いていません';
  if (!S.dir) return;
  for (const f of S.dir.files) {
    const tabs = S.tabs.filter((t) => t.fileName === f.name && !t.fromDraft);
    const dirty = S.tabs.some((t) => t.fileName === f.name && t.unwritten);
    const drafts = S.draftIndex.get(f.name) || 0;
    box.append(el('div', {
      class: 'file' + (tabs.length ? ' open' : '') + (S.active && S.active.fileName === f.name ? ' current' : ''),
      title: f.bad ? 'このファイルはパレット PNG として読めません' : f.name,
      onclick: () => openListed(f),
    },
      el('div', { class: 'th' }, f.thumb ? el('img', { src: f.thumb }) : f.bad ? '×' : null),
      el('div', { class: 'nm' }, f.name, el('div', { class: 'sz' }, fmtSize(f.size))),
      dirty ? el('span', { class: 'mk dirty', title: 'ファイルにまだ反映していない変更があります' }, '未保存') : null,
      drafts ? el('span', { class: 'mk draft', title: '一時保存の控えがあります' }, `控え${drafts}`) : null));
  }
}

async function openListed(f) {
  const exist = S.tabs.find((t) => t.fileName === f.name && !t.fromDraft);
  if (exist) { activate(exist); return; }
  try {
    const bytes = new Uint8Array(await (await f.handle.getFile()).arrayBuffer());
    await openBytes(f.name, bytes, f.handle);
  } catch (err) {
    f.bad = true;
    renderFileList();
    toast(`${f.name} を開けませんでした: ${err.message}`, 'ng');
  }
}

async function pickFolder() {
  try {
    const h = await window.showDirectoryPicker({ mode: 'readwrite' });
    await store.saveDirHandle(h);
    $('btnReopenDir').hidden = true;
    await loadFolder(h);
  } catch (err) {
    if (err.name !== 'AbortError') toast('フォルダを開けませんでした: ' + err.message, 'ng');
  }
}

async function reopenFolder() {
  try {
    const h = await store.loadDirHandle();
    if (!h) return;
    // 権限の確認は、クリックされた直後でないと出せない
    if ((await h.requestPermission({ mode: 'readwrite' })) !== 'granted') { toast('フォルダの権限が許可されませんでした', 'ng'); return; }
    $('btnReopenDir').hidden = true;
    await loadFolder(h);
  } catch (err) {
    toast('前のフォルダを開けませんでした: ' + err.message, 'ng');
  }
}

async function openLocalFiles(fileList) {
  for (const f of fileList) {
    try {
      await openBytes(f.name, new Uint8Array(await f.arrayBuffer()), null);
    } catch (err) {
      toast(`${f.name} を開けませんでした: ${err.message}`, 'ng');
    }
  }
}

// ----------------------------------------------------------------
// ゲームで試す
// ----------------------------------------------------------------

async function tryInGame(tab) {
  if (!tab) return;
  try {
    const bytes = await encodeTab(tab);
    await store.putPreview(tab.fileName, bytes); // キーと name は NFC にそろえる（store 側）
    toast(`${tab.fileName.normalize('NFC')} をゲーム用に置きました。ゲーム（/）を開き直してください`, 'ok');
    renderPreviewList();
  } catch (err) {
    toast('置けませんでした: ' + err.message, 'ng');
  }
}

async function renderPreviewList() {
  const box = $('pvList');
  let list = [];
  try { list = await store.listPreviews(); } catch (err) { box.textContent = '読めませんでした: ' + err.message; return; }
  box.textContent = '';
  if (!list.length) { box.textContent = '今ゲーム用に置いている絵はありません'; return; }
  box.append(el('div', {}, '今ゲーム用に置いている絵:'));
  for (const it of list) {
    box.append(el('div', { class: 'row', style: 'margin:3px 0' },
      el('span', { style: 'flex:1;word-break:break-all;color:var(--fg)' }, `${it.name}（${fmtSize(it.size)}）`),
      el('button', { class: 'sub mini', onclick: async () => { await store.deletePreview(it.key); renderPreviewList(); } }, 'やめる')));
  }
}

// ----------------------------------------------------------------
// ヘルプ
// ----------------------------------------------------------------

const HELP = [
  ['B / E / L / U / O', 'ペン / 消しゴム / 直線 / 四角 / 円'],
  ['G / I / M / H', '塗りつぶし / スポイト / 範囲選択 / 手のひら'],
  ['Shift+U / Shift+O', '塗りつぶした四角 / 円'],
  ['Shift を押しながら描く', '直線は 0°・45°・90°、四角と円は正方形・正円'],
  ['右クリック', 'どのツールでもスポイト'],
  ['Cmd+クリック（Ctrl も可）', 'クリックした所を真ん中にして拡大'],
  ['Cmd+右クリック', '縮小'],
  ['Cmd+ホイール', 'カーソルの位置を中心に拡大縮小'],
  ['+ / -', '拡大 / 縮小'],
  ['スペース+ドラッグ / 中ボタンドラッグ', '画面を動かす'],
  ['[ / ]', '前の色 / 次の色'],
  ['Cmd+Z / Cmd+Shift+Z / Cmd+Y', '元に戻す / やり直す（100 回まで）'],
  ['Cmd+S', 'ファイルに保存'],
  ['Cmd+Shift+S', '一時保存（元のファイルは変えない）'],
  ['Cmd+C / X / V', '選択範囲のコピー / 切り取り / 貼り付け（開いている別の絵にも貼れます）'],
  ['Cmd+A / Cmd+D', '全選択 / 選択解除'],
  ['矢印キー', '選択範囲を 1 ドット動かす（Shift でマス単位）'],
  ['Delete', '選択範囲を透明にする'],
  ['Enter / Esc', '浮かせた絵を確定 / 選択を外す'],
  ['?', 'この一覧'],
];
function openHelp() {
  const t = $('helpTable');
  t.textContent = '';
  for (const [k, v] of HELP) t.append(el('tr', {}, el('td', {}, el('kbd', {}, k)), el('td', {}, v)));
  $('helpDlg').showModal();
}

// ----------------------------------------------------------------
// パネルの同期・初期化
// ----------------------------------------------------------------

function syncPanels() {
  const tab = S.active;
  renderLayers();
  renderPalette();
  if (tab) {
    $('cellW').value = tab.cell.w;
    $('cellH').value = tab.cell.h;
    syncPreset();
    $('animFrames').value = tab.animText;
    $('palEdit').hidden = true;
    if (S.opts.used) tab.usedDirty = true;
  }
  animFrame = 0;
  syncStatus();
}

function syncPreset() {
  const tab = S.active;
  if (!tab) return;
  const v = `${tab.cell.w}x${tab.cell.h}`;
  const has = [...$('cellPreset').options].some((o) => o.value === v);
  $('cellPreset').value = has ? v : 'custom';
}

function setCell(w, h) {
  const tab = S.active;
  if (!tab) return;
  tab.cell = { w: clamp(w | 0, 1, 512), h: clamp(h | 0, 1, 512) };
  $('cellW').value = tab.cell.w;
  $('cellH').value = tab.cell.h;
  syncPreset();
  requestDraw();
}

function init() {
  renderTools();
  syncCursor();
  bindOption('optSnap', 'snap');
  bindOption('optClip', 'clip');
  bindOption('optKeepIdx', 'keepIdx');
  bindOption('optSkipTrans', 'skipTrans');
  bindOption('optPixGrid', 'pixGrid');
  bindOption('optCellGrid', 'cellGrid');
  bindOption('optCellNo', 'cellNo');
  bindOption('optChecker', 'checker');
  bindOption('optUsed', 'used', () => { const t = S.active; if (t) { t.usedDirty = true; if (!S.opts.used) renderPalette(); } });

  $('cellPreset').addEventListener('change', (e) => {
    if (e.target.value === 'custom') return;
    const [w, h] = e.target.value.split('x').map(Number);
    setCell(w, h);
  });
  $('cellW').addEventListener('change', () => setCell(Number($('cellW').value), Number($('cellH').value)));
  $('cellH').addEventListener('change', () => setCell(Number($('cellW').value), Number($('cellH').value)));

  $('btnCopy').addEventListener('click', () => S.active && copySelection(S.active));
  $('btnCut').addEventListener('click', () => S.active && cutSelection(S.active));
  $('btnPaste').addEventListener('click', () => S.active && pasteClipboard(S.active));
  $('btnSelAll').addEventListener('click', () => { const t = S.active; if (t) { commitFloat(t); t.sel = { x: 0, y: 0, w: t.w, h: t.h }; requestDraw(); } });
  $('btnDeselect').addEventListener('click', () => { const t = S.active; if (t) { commitFloat(t); t.sel = null; requestDraw(); } });
  $('btnFlipH').addEventListener('click', () => transformSelection('h'));
  $('btnFlipV').addEventListener('click', () => transformSelection('v'));
  $('btnRot').addEventListener('click', () => transformSelection('rot'));

  $('btnSave').addEventListener('click', () => saveToFile(S.active));
  $('btnDraft').addEventListener('click', () => saveDraftNow(S.active, false));
  $('drSave').addEventListener('click', () => saveDraftNow(S.active, false));
  $('drSaveNew').addEventListener('click', () => saveDraftNow(S.active, true));
  $('drAll').addEventListener('change', (e) => { S.showAllDrafts = e.target.checked; renderDrafts(); });
  $('btnTry').addEventListener('click', () => tryInGame(S.active));
  $('pvTry').addEventListener('click', () => tryInGame(S.active));
  $('pvClear').addEventListener('click', async () => { await store.clearPreviews(); renderPreviewList(); toast('ゲーム用に置いた絵を全部やめました'); });
  $('btnHelp').addEventListener('click', openHelp);

  $('btnOpenFile').addEventListener('click', () => $('fileInput').click());
  $('fileInput').addEventListener('change', (e) => { openLocalFiles([...e.target.files]); e.target.value = ''; });
  $('btnOpenDir').addEventListener('click', pickFolder);
  $('btnReopenDir').addEventListener('click', reopenFolder);

  if (!window.showDirectoryPicker) {
    $('browserNote').hidden = false;
    $('btnOpenDir').disabled = true;
  }

  // アニメ確認
  $('animPlay').addEventListener('click', () => setAnimPlaying(!animPlaying));
  $('animMs').addEventListener('change', () => { if (animPlaying) setAnimPlaying(true); });
  $('animFrames').addEventListener('input', () => { if (S.active) S.active.animText = $('animFrames').value; animFrame = 0; drawAnim(); });
  $('animAdd').addEventListener('click', () => {
    const tab = S.active;
    if (!tab) return;
    const cur = $('animFrames').value.trim();
    $('animFrames').value = cur ? `${cur},${tab.lastCell}` : String(tab.lastCell);
    tab.animText = $('animFrames').value;
    drawAnim();
  });
  for (const id of ['tile', 'animCv']) $(id).closest('details').addEventListener('toggle', () => requestDraw());

  renderPreviewList();
  refreshDraftIndex().then(() => { renderFileList(); renderDrafts(); });
  offerRestore();
  // 前のフォルダがあれば「前のフォルダを開く」を出す
  store.loadDirHandle().then((h) => {
    if (h && window.showDirectoryPicker) {
      $('btnReopenDir').hidden = false;
      $('btnReopenDir').textContent = `前のフォルダを開く（${h.name}）`;
    }
  }).catch(() => {});

  renderTabs();
  layoutSizer();
  drawAll();

  if (IS_TEST) {
    // 自動テスト用の裏口（?test=1 の時だけ）。フォルダを選ばなくても絵を開ける
    window.__dotTest = {
      openBytes: (name, bytes) => openBytes(name, bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes)).then((t) => t.uid),
      S, core: C,
      get tab() { return S.active; },
      saveToFile: () => saveToFile(S.active),
      saveDraft: (asNew) => saveDraftNow(S.active, !!asNew),
      encode: async () => Array.from(await encodeTab(S.active)),
      autosave: () => autosaveNow(S.active),
      toImgScreen: (x, y) => {
        const tab = S.active;
        const r = vp.getBoundingClientRect();
        const { ox, oy } = origin(tab);
        return { x: r.left + vp.clientLeft + ox + (x + 0.5) * tab.zoom, y: r.top + vp.clientTop + oy + (y + 0.5) * tab.zoom };
      },
    };
  }
}

init();
