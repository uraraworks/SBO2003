/**
 * dot-core.js
 * ドット絵エディタの「画面を使わない」部分（描画・塗りつぶし・合成・変形など）。
 *
 * なぜ分けるのか:
 *   DOM に触らない純粋な関数だけにしておくと、Node でそのままテストできる。
 *   絵を壊さないことが一番大事なので、ここは必ずテストできる形にしておく。
 *
 * 用語:
 *   - レイヤーの data は Int16Array。値はパレット番号 (0-255)、-1 は「空（何も描いていない）」。
 *   - ゲームでの透明色は「パレットの黒 (0,0,0)」。番号 0 ではない。
 */

export const EMPTY = -1;

// ----------------------------------------------------------------
// パレット
// ----------------------------------------------------------------

/** 色が黒 (= ゲームでの透明色) かどうか */
export function isBlack(c) {
  return c.r === 0 && c.g === 0 && c.b === 0;
}

/** パレットを 256 色にそろえて複製する（足りない分は黒）。保存時に 256 色を保つため。 */
export function normalizePalette(palette) {
  const out = [];
  for (let i = 0; i < 256; i++) {
    const c = palette[i];
    out.push(c ? { r: c.r, g: c.g, b: c.b, a: c.a ?? 255 } : { r: 0, g: 0, b: 0, a: 255 });
  }
  return out;
}

export function clonePalette(palette) {
  return palette.map((c) => ({ r: c.r, g: c.g, b: c.b, a: c.a ?? 255 }));
}

export function palettesEqual(a, b) {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) {
    if (a[i].r !== b[i].r || a[i].g !== b[i].g || a[i].b !== b[i].b) return false;
  }
  return true;
}

/** 最も近い色の番号（二乗距離）。同じ距離なら小さい番号。 */
export function nearestColorIndex(palette, r, g, b) {
  let best = 0;
  let bestD = Infinity;
  for (let i = 0; i < palette.length; i++) {
    const c = palette[i];
    const d = (c.r - r) ** 2 + (c.g - g) ** 2 + (c.b - b) ** 2;
    if (d < bestD) {
      bestD = d;
      best = i;
      if (d === 0) break;
    }
  }
  return best;
}

/** 消しゴムが塗る「透明色」の番号。黒が無ければ一番黒に近い色。 */
export function findTransparentIndex(palette) {
  for (let i = 0; i < palette.length; i++) {
    if (isBlack(palette[i])) return i;
  }
  return nearestColorIndex(palette, 0, 0, 0);
}

/** src のパレット番号 → dst のパレット番号 の対応表（Int16Array(256)） */
export function buildColorMap(srcPalette, dstPalette) {
  const map = new Int16Array(256);
  for (let i = 0; i < 256; i++) {
    const c = srcPalette[i] ?? { r: 0, g: 0, b: 0 };
    map[i] = nearestColorIndex(dstPalette, c.r, c.g, c.b);
  }
  return map;
}

/** 領域のデータを別パレット向けに変換する（-1 はそのまま） */
export function mapRegionColors(data, map) {
  const out = new Int16Array(data.length);
  for (let i = 0; i < data.length; i++) out[i] = data[i] < 0 ? -1 : map[data[i]];
  return out;
}

/** 範囲 a..b を両端の色から直線補間した新しいパレットを返す */
export function gradientPalette(palette, a, b) {
  const out = clonePalette(palette);
  const lo = Math.min(a, b);
  const hi = Math.max(a, b);
  const c0 = palette[lo];
  const c1 = palette[hi];
  const n = hi - lo;
  if (n < 2) return out;
  for (let i = lo + 1; i < hi; i++) {
    const t = (i - lo) / n;
    out[i] = {
      r: Math.round(c0.r + (c1.r - c0.r) * t),
      g: Math.round(c0.g + (c1.g - c0.g) * t),
      b: Math.round(c0.b + (c1.b - c0.b) * t),
      a: out[i].a ?? 255,
    };
  }
  return out;
}

// ----------------------------------------------------------------
// 図形（plot(x, y) を呼ぶだけ。範囲外のチェックは呼ぶ側の plot でする）
// ----------------------------------------------------------------

/** ブレゼンハムの直線。ペンのドラッグ補間にも使う */
export function bresenham(x0, y0, x1, y1, plot) {
  const dx = Math.abs(x1 - x0);
  const dy = -Math.abs(y1 - y0);
  const sx = x0 < x1 ? 1 : -1;
  const sy = y0 < y1 ? 1 : -1;
  let err = dx + dy;
  for (;;) {
    plot(x0, y0);
    if (x0 === x1 && y0 === y1) break;
    const e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

/** Shift 中の直線: 水平・垂直・45° のどれかに寄せた終点を返す */
export function constrainLine(x0, y0, x1, y1) {
  const dx = x1 - x0;
  const dy = y1 - y0;
  const adx = Math.abs(dx);
  const ady = Math.abs(dy);
  if (ady * 2 < adx) return [x1, y0];
  if (adx * 2 < ady) return [x0, y1];
  const m = Math.max(adx, ady);
  return [x0 + (dx < 0 ? -m : m), y0 + (dy < 0 ? -m : m)];
}

/** Shift 中の四角・円: 正方形になる終点を返す */
export function squareEnd(x0, y0, x1, y1) {
  const dx = x1 - x0;
  const dy = y1 - y0;
  const m = Math.max(Math.abs(dx), Math.abs(dy));
  return [x0 + (dx < 0 ? -m : m), y0 + (dy < 0 ? -m : m)];
}

export function rectPixels(x0, y0, x1, y1, filled, plot) {
  const lx = Math.min(x0, x1);
  const hx = Math.max(x0, x1);
  const ly = Math.min(y0, y1);
  const hy = Math.max(y0, y1);
  for (let y = ly; y <= hy; y++) {
    if (filled || y === ly || y === hy) {
      for (let x = lx; x <= hx; x++) plot(x, y);
    } else {
      plot(lx, y);
      if (hx !== lx) plot(hx, y);
    }
  }
}

/** 外接する四角 (x0,y0)-(x1,y1) にぴったり入る楕円（Zingl のアルゴリズム） */
export function ellipsePixels(x0, y0, x1, y1, filled, plot) {
  let a = Math.abs(x1 - x0);
  let b = Math.abs(y1 - y0);
  // 幅か高さが 2 マス以下だと楕円の式が崩れる。細い楕円は四角と同じなのでそちらに任せる
  if (a < 2 || b < 2) {
    rectPixels(x0, y0, x1, y1, true, plot);
    return;
  }
  let b1 = b & 1;
  let dx = 4 * (1 - a) * b * b;
  let dy = 4 * (b1 + 1) * a * a;
  let err = dx + dy + b1 * a * a;
  if (x0 > x1) { x0 = x1; x1 += a; }
  if (y0 > y1) y0 = y1;
  y0 += (b + 1) >> 1;
  y1 = y0 - b1;
  a = 8 * a * a;
  b1 = 8 * b * b;

  // 塗りつぶし用に、行ごとの左右端を覚えておく
  const spans = filled ? new Map() : null;
  const put = (x, y) => {
    if (filled) {
      const s = spans.get(y);
      if (!s) spans.set(y, [x, x]);
      else { if (x < s[0]) s[0] = x; if (x > s[1]) s[1] = x; }
    } else {
      plot(x, y);
    }
  };

  do {
    put(x1, y0); put(x0, y0); put(x0, y1); put(x1, y1);
    const e2 = 2 * err;
    if (e2 <= dy) { y0++; y1--; err += dy += a; }
    if (e2 >= dx || 2 * err > dy) { x0++; x1--; err += dx += b1; }
  } while (x0 <= x1);
  while (y0 - y1 < b) {
    put(x0 - 1, y0); put(x1 + 1, y0++);
    put(x0 - 1, y1); put(x1 + 1, y1--);
  }

  if (filled) {
    for (const [y, [lx, hx]] of spans) {
      for (let x = lx; x <= hx; x++) plot(x, y);
    }
  }
}

// ----------------------------------------------------------------
// 塗りつぶし・色の置き換え
// ----------------------------------------------------------------

function inClip(clip, x, y) {
  return !clip || (x >= clip.x && y >= clip.y && x < clip.x + clip.w && y < clip.y + clip.h);
}

/** 上下左右 4 方向の塗りつぶし。clip があればその四角の中だけ。変えた数を返す。 */
export function floodFill(data, w, h, sx, sy, val, clip) {
  if (sx < 0 || sy < 0 || sx >= w || sy >= h || !inClip(clip, sx, sy)) return 0;
  const target = data[sy * w + sx];
  if (target === val) return 0;
  const minX = clip ? Math.max(0, clip.x) : 0;
  const maxX = clip ? Math.min(w - 1, clip.x + clip.w - 1) : w - 1;
  const minY = clip ? Math.max(0, clip.y) : 0;
  const maxY = clip ? Math.min(h - 1, clip.y + clip.h - 1) : h - 1;
  let count = 0;
  const stack = [sx, sy];
  while (stack.length) {
    const y = stack.pop();
    let x = stack.pop();
    // 左端まで戻る
    while (x > minX && data[y * w + x - 1] === target) x--;
    let spanUp = false;
    let spanDown = false;
    while (x <= maxX && data[y * w + x] === target) {
      data[y * w + x] = val;
      count++;
      if (y > minY) {
        const hit = data[(y - 1) * w + x] === target;
        if (hit && !spanUp) { stack.push(x, y - 1); spanUp = true; }
        else if (!hit) spanUp = false;
      }
      if (y < maxY) {
        const hit = data[(y + 1) * w + x] === target;
        if (hit && !spanDown) { stack.push(x, y + 1); spanDown = true; }
        else if (!hit) spanDown = false;
      }
      x++;
    }
  }
  return count;
}

/** 同じ番号を全部置き換える（clip があればその四角の中だけ）。変えた数を返す。 */
export function replaceValue(data, w, h, from, to, clip) {
  if (from === to) return 0;
  let count = 0;
  const x0 = clip ? Math.max(0, clip.x) : 0;
  const y0 = clip ? Math.max(0, clip.y) : 0;
  const x1 = clip ? Math.min(w, clip.x + clip.w) : w;
  const y1 = clip ? Math.min(h, clip.y + clip.h) : h;
  for (let y = y0; y < y1; y++) {
    for (let x = x0; x < x1; x++) {
      const i = y * w + x;
      if (data[i] === from) { data[i] = to; count++; }
    }
  }
  return count;
}

// ----------------------------------------------------------------
// 領域の切り出し・貼り付け・変形
// ----------------------------------------------------------------

/** 領域を切り出す。画像の外は -1（空） */
export function getRegion(data, w, h, x, y, rw, rh) {
  const out = new Int16Array(rw * rh).fill(-1);
  for (let j = 0; j < rh; j++) {
    const sy = y + j;
    if (sy < 0 || sy >= h) continue;
    for (let i = 0; i < rw; i++) {
      const sx = x + i;
      if (sx < 0 || sx >= w) continue;
      out[j * rw + i] = data[sy * w + sx];
    }
  }
  return out;
}

/**
 * 領域を貼り付ける。-1（空）は貼らない。
 * skipValue を渡すと、その番号（透明色）のマスも貼らずに下の絵を残す。
 */
export function putRegion(data, w, h, x, y, src, sw, sh, skipValue) {
  for (let j = 0; j < sh; j++) {
    const dy = y + j;
    if (dy < 0 || dy >= h) continue;
    for (let i = 0; i < sw; i++) {
      const dx = x + i;
      if (dx < 0 || dx >= w) continue;
      const v = src[j * sw + i];
      if (v < 0) continue;
      if (skipValue !== undefined && skipValue !== null && v === skipValue) continue;
      data[dy * w + dx] = v;
    }
  }
}

/** 四角の中を fill で埋める */
export function fillRect(data, w, h, x, y, rw, rh, fill) {
  for (let j = Math.max(0, y); j < Math.min(h, y + rh); j++) {
    for (let i = Math.max(0, x); i < Math.min(w, x + rw); i++) data[j * w + i] = fill;
  }
}

export function flipH(data, w, h) {
  const out = new Int16Array(data.length);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) out[y * w + (w - 1 - x)] = data[y * w + x];
  return out;
}

export function flipV(data, w, h) {
  const out = new Int16Array(data.length);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) out[(h - 1 - y) * w + x] = data[y * w + x];
  return out;
}

/** 右（時計回り）に 90 度。幅と高さが入れ替わる */
export function rotateCW(data, w, h) {
  const out = new Int16Array(data.length);
  // 元の (x, y) は新しい画像（幅 h, 高さ w）の (h-1-y, x) に行く
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) out[x * h + (h - 1 - y)] = data[y * w + x];
  return { data: out, w: h, h: w };
}

/** 変更があった範囲だけを取り出す（元に戻す履歴を小さくするため）。変化なしは null。 */
export function diffRect(before, after, w, h) {
  let minX = w, minY = h, maxX = -1, maxY = -1;
  for (let y = 0; y < h; y++) {
    const row = y * w;
    for (let x = 0; x < w; x++) {
      if (before[row + x] !== after[row + x]) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }
  if (maxX < 0) return null;
  const rw = maxX - minX + 1;
  const rh = maxY - minY + 1;
  return {
    x: minX, y: minY, w: rw, h: rh,
    before: getRegion(before, w, h, minX, minY, rw, rh),
    after: getRegion(after, w, h, minX, minY, rw, rh),
  };
}

/** diffRect で取った範囲を data に書き戻す（値そのまま。-1 も書く） */
export function applyRect(data, w, h, rect, values) {
  for (let j = 0; j < rect.h; j++) {
    for (let i = 0; i < rect.w; i++) {
      const x = rect.x + i;
      const y = rect.y + j;
      if (x < 0 || y < 0 || x >= w || y >= h) continue;
      data[y * w + x] = values[j * rect.w + i];
    }
  }
}

// ----------------------------------------------------------------
// 合成・平坦化
// ----------------------------------------------------------------

/** パレットを canvas の ImageData (リトルエンディアン ABGR) 用に詰める */
export function packPalette(palette) {
  const packed = new Uint32Array(256);
  const trans = new Uint8Array(256);
  for (let i = 0; i < 256; i++) {
    const c = palette[i] ?? { r: 0, g: 0, b: 0 };
    packed[i] = (255 << 24) | (c.b << 16) | (c.g << 8) | c.r;
    trans[i] = isBlack(c) ? 1 : 0;
  }
  return { packed, trans };
}

/**
 * 見た目用の合成。下のレイヤーから順に重ねる（一番上の「空でない」マスが勝つ）。
 * 透明色（黒）のマスはアルファ 0 にしておき、画面側で市松模様か黒を敷く。
 * 「下絵として半透明」のレイヤーは alpha で薄く重ねる。
 * out32 は幅×高さ個の Uint32Array。
 */
export function compositeInto(out32, w, h, layers, palette, ghostAlpha = 0.4) {
  const { packed, trans } = packPalette(palette);
  const n = w * h;
  const vis = layers.filter((l) => l.visible);
  out32.fill(0);
  for (const layer of vis) {
    const d = layer.data;
    if (!layer.ghost) {
      for (let i = 0; i < n; i++) {
        const v = d[i];
        if (v < 0) continue;
        out32[i] = trans[v] ? 0 : packed[v];
      }
    } else {
      for (let i = 0; i < n; i++) {
        const v = d[i];
        if (v < 0 || trans[v]) continue;
        const under = out32[i];
        const c = packed[v];
        if ((under >>> 24) === 0) {
          out32[i] = ((Math.round(255 * ghostAlpha) & 255) << 24) | (c & 0xffffff);
        } else {
          const r = Math.round((c & 255) * ghostAlpha + (under & 255) * (1 - ghostAlpha));
          const g = Math.round(((c >> 8) & 255) * ghostAlpha + ((under >> 8) & 255) * (1 - ghostAlpha));
          const b = Math.round(((c >> 16) & 255) * ghostAlpha + ((under >> 16) & 255) * (1 - ghostAlpha));
          out32[i] = (255 << 24) | (b << 16) | (g << 8) | r;
        }
      }
    }
  }
}

/**
 * 保存用の平坦化。見えているレイヤーを重ね、空のマスは透明色の番号にする。
 * 半透明表示は見た目だけなので、ここでは普通に重ねる。
 */
export function flatten(layers, w, h, transparentIndex) {
  const out = new Uint8Array(w * h).fill(transparentIndex);
  for (const layer of layers) {
    if (!layer.visible) continue;
    const d = layer.data;
    for (let i = 0; i < out.length; i++) {
      if (d[i] >= 0) out[i] = d[i];
    }
  }
  return out;
}

/** 画面に見えている一番上の色の番号（スポイト用）。何も無ければ透明色の番号。 */
export function pickIndex(layers, w, h, x, y, transparentIndex) {
  if (x < 0 || y < 0 || x >= w || y >= h) return transparentIndex;
  for (let k = layers.length - 1; k >= 0; k--) {
    const l = layers[k];
    if (!l.visible) continue;
    const v = l.data[y * w + x];
    if (v >= 0) return v;
  }
  return transparentIndex;
}

/** 使われている色の番号の集合（印をつける用）。見えているレイヤーだけ。 */
export function usedIndices(layers) {
  const used = new Uint8Array(256);
  for (const l of layers) {
    if (!l.visible) continue;
    const d = l.data;
    for (let i = 0; i < d.length; i++) if (d[i] >= 0) used[d[i]] = 1;
  }
  return used;
}

// ----------------------------------------------------------------
// マス（セル）の計算
// ----------------------------------------------------------------

/** ファイル名からマスの大きさの初期値を推測する */
export function guessCellSize(fileName) {
  const n = fileName.toLowerCase();
  if (n.includes('4848')) return { w: 48, h: 48 };
  if (n.includes('3232')) return { w: 32, h: 32 };
  if (n.includes('1632')) return { w: 16, h: 32 };
  return { w: 16, h: 16 };
}

/** ゲーム内の拡大率: 等倍なのは 3 枚だけ、あとは 2 倍 */
export function gameScale(fileName) {
  // Mac のフォルダは濁点が分離した形 (NFD) で名前を返すことがある。表は NFC で引く
  const n = fileName.normalize('NFC').toLowerCase();
  if (n === 'system.png' || n === 'タイトル.png' || n === 'アニメーションデータ.png') return 1;
  return 2;
}

/** マス番号 → 四角（行 × 列数 + 列）。範囲外は null */
export function cellRect(cellNo, imgW, imgH, cw, ch) {
  const cols = Math.max(1, Math.floor(imgW / cw));
  const rows = Math.max(1, Math.floor(imgH / ch));
  if (cellNo < 0 || cellNo >= cols * rows) return null;
  return { x: (cellNo % cols) * cw, y: Math.floor(cellNo / cols) * ch, w: cw, h: ch };
}

/** 画素 → マス番号 */
export function cellNumberAt(x, y, imgW, imgH, cw, ch) {
  const cols = Math.max(1, Math.floor(imgW / cw));
  const rows = Math.max(1, Math.floor(imgH / ch));
  const cx = Math.floor(x / cw);
  const cy = Math.floor(y / ch);
  if (cx < 0 || cy < 0 || cx >= cols || cy >= rows) return -1;
  return cy * cols + cx;
}

/** "0,1,0,2" や "0-3" のような入力をマス番号の配列にする */
export function parseCellList(text) {
  const out = [];
  for (const part of String(text).split(/[\s,、，]+/)) {
    if (!part) continue;
    const m = /^(\d+)-(\d+)$/.exec(part);
    if (m) {
      const a = Number(m[1]);
      const b = Number(m[2]);
      const step = a <= b ? 1 : -1;
      for (let i = a; i !== b + step && out.length < 1000; i += step) out.push(i);
    } else if (/^\d+$/.test(part)) {
      out.push(Number(part));
    }
  }
  return out;
}
