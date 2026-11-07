/**
 * dot-png.js
 * 8bit パレット PNG の読み書き（ドット絵エディタ /dot の土台）
 *
 * 出どころ:
 *   SBOP2 の lib/indexed-png.js を移植したもの。
 *
 * なぜ自前で書くのか:
 *   ゲーム側 (SboCli の ReadGrpData2 / MgrGrpData.cpp の Read256) は
 *   colortype がパレットでない PNG を読み込み拒否する。
 *   canvas の toBlob() は必ず RGBA PNG を吐くため、canvas で描いたものを
 *   そのまま保存すると「ブラウザでは正常に見えるのにゲームだけ読めない」
 *   画像が出来上がる。
 *   そこでパレットとインデックス列を保ったまま読み書きする最小の
 *   PNG デコーダ/エンコーダを用意する。
 *
 * 対応範囲は用途に絞る:
 *   - colortype 3 (パレット) / bitdepth 8 / interlace なし のみ
 *   - 読み込みは全フィルタ種別 (0-4) に対応、書き出しは None 固定
 *   - インデックス 0 = 透過 という SBOP2 の約束はここでは強制しない
 *     (tRNS はそのまま持ち回る)
 *
 * zlib は DecompressionStream / CompressionStream ("deflate" = zlib ラップ) を使う。
 * Node からも同じコードをテストできるようグローバル以外に依存しない。
 */

const PNG_SIGNATURE = [0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a];

// ----------------------------------------------------------------
// CRC32 (PNG 仕様の多項式 0xEDB88320)
// ----------------------------------------------------------------

const CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
    }
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(bytes) {
  let c = 0xffffffff;
  for (let i = 0; i < bytes.length; i++) {
    c = CRC_TABLE[(c ^ bytes[i]) & 0xff] ^ (c >>> 8);
  }
  return (c ^ 0xffffffff) >>> 0;
}

// ----------------------------------------------------------------
// zlib
// ----------------------------------------------------------------

async function streamThrough(bytes, stream) {
  const writer = stream.writable.getWriter();
  writer.write(bytes);
  writer.close();

  const chunks = [];
  let total = 0;
  const reader = stream.readable.getReader();
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    chunks.push(value);
    total += value.length;
  }

  const out = new Uint8Array(total);
  let pos = 0;
  for (const chunk of chunks) {
    out.set(chunk, pos);
    pos += chunk.length;
  }
  return out;
}

const inflate = (bytes) => streamThrough(bytes, new DecompressionStream("deflate"));
const deflate = (bytes) => streamThrough(bytes, new CompressionStream("deflate"));

// ----------------------------------------------------------------
// チャンク走査
// ----------------------------------------------------------------

function readUint32(bytes, offset) {
  return ((bytes[offset] << 24) | (bytes[offset + 1] << 16) |
          (bytes[offset + 2] << 8) | bytes[offset + 3]) >>> 0;
}

function writeUint32(bytes, offset, value) {
  bytes[offset]     = (value >>> 24) & 0xff;
  bytes[offset + 1] = (value >>> 16) & 0xff;
  bytes[offset + 2] = (value >>> 8) & 0xff;
  bytes[offset + 3] = value & 0xff;
}

function chunkType(bytes, offset) {
  return String.fromCharCode(bytes[offset], bytes[offset + 1], bytes[offset + 2], bytes[offset + 3]);
}

// ----------------------------------------------------------------
// スキャンラインのフィルタ解除 (PNG 仕様 9.2)
// bitdepth 8 / colortype 3 なので 1 ピクセル = 1 バイト (bpp = 1)
// ----------------------------------------------------------------

function unfilter(raw, width, height) {
  const bpp = 1;
  const stride = width;
  const out = new Uint8Array(width * height);

  let src = 0;
  for (let y = 0; y < height; y++) {
    const filterType = raw[src++];
    const rowStart = y * stride;
    const prevStart = (y - 1) * stride;

    for (let x = 0; x < stride; x++) {
      const rawByte = raw[src++];
      const a = x >= bpp ? out[rowStart + x - bpp] : 0;          // 左
      const b = y > 0 ? out[prevStart + x] : 0;                   // 上
      const c = (x >= bpp && y > 0) ? out[prevStart + x - bpp] : 0; // 左上

      let value;
      switch (filterType) {
        case 0: value = rawByte; break;
        case 1: value = rawByte + a; break;
        case 2: value = rawByte + b; break;
        case 3: value = rawByte + ((a + b) >> 1); break;
        case 4: {
          const p = a + b - c;
          const pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c);
          const pred = (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
          value = rawByte + pred;
          break;
        }
        default:
          throw new Error(`未対応の PNG フィルタ種別: ${filterType}`);
      }
      out[rowStart + x] = value & 0xff;
    }
  }
  return out;
}

// ----------------------------------------------------------------
// 公開 API
// ----------------------------------------------------------------

/**
 * 8bit パレット PNG をデコードする。
 * @param {ArrayBuffer|Uint8Array} source
 * @returns {Promise<{width:number, height:number, indices:Uint8Array,
 *                    palette:Array<{r:number,g:number,b:number,a:number}>}>}
 * @throws パレット PNG でない場合や未対応の形式の場合
 */
export async function decodeIndexedPng(source) {
  const bytes = source instanceof Uint8Array ? source : new Uint8Array(source);

  if (bytes.length < 8 || !PNG_SIGNATURE.every((b, i) => bytes[i] === b)) {
    throw new Error("PNG シグネチャがありません");
  }

  let width = 0, height = 0;
  let plte = null;
  let trns = null;
  const idatParts = [];
  let idatLength = 0;

  let offset = 8;
  while (offset + 8 <= bytes.length) {
    const length = readUint32(bytes, offset);
    const type = chunkType(bytes, offset + 4);
    const dataStart = offset + 8;

    if (type === "IHDR") {
      width  = readUint32(bytes, dataStart);
      height = readUint32(bytes, dataStart + 4);
      const bitDepth  = bytes[dataStart + 8];
      const colorType = bytes[dataStart + 9];
      const interlace = bytes[dataStart + 12];
      if (colorType !== 3) {
        throw new Error(`パレット PNG ではありません (colortype=${colorType})`);
      }
      if (bitDepth !== 8) {
        throw new Error(`bitdepth 8 以外は未対応です (bitdepth=${bitDepth})`);
      }
      if (interlace !== 0) {
        throw new Error("インターレース PNG は未対応です");
      }
    } else if (type === "PLTE") {
      plte = bytes.subarray(dataStart, dataStart + length);
    } else if (type === "tRNS") {
      trns = bytes.subarray(dataStart, dataStart + length);
    } else if (type === "IDAT") {
      idatParts.push(bytes.subarray(dataStart, dataStart + length));
      idatLength += length;
    } else if (type === "IEND") {
      break;
    }

    offset = dataStart + length + 4; // +4 は CRC
  }

  if (!plte) throw new Error("PLTE チャンクがありません");
  if (idatParts.length === 0) throw new Error("IDAT チャンクがありません");

  const compressed = new Uint8Array(idatLength);
  {
    let pos = 0;
    for (const part of idatParts) { compressed.set(part, pos); pos += part.length; }
  }

  const raw = await inflate(compressed);
  const expected = height * (width + 1);
  if (raw.length < expected) {
    throw new Error(`展開後のサイズが足りません (${raw.length} < ${expected})`);
  }

  const palette = [];
  for (let i = 0; i * 3 + 2 < plte.length; i++) {
    palette.push({
      r: plte[i * 3],
      g: plte[i * 3 + 1],
      b: plte[i * 3 + 2],
      a: trns && i < trns.length ? trns[i] : 255,
    });
  }

  return { width, height, indices: unfilter(raw, width, height), palette };
}

/**
 * 8bit パレット PNG をエンコードする。
 * スキャンラインのフィルタは None 固定（スプライトシート程度の大きさなら
 * deflate だけで十分小さくなるため、実装を単純に保つ）。
 */
export async function encodeIndexedPng({ width, height, indices, palette }) {
  if (!palette || palette.length === 0) throw new Error("パレットが空です");
  if (palette.length > 256) throw new Error("パレットは 256 色までです");
  if (indices.length !== width * height) {
    throw new Error(`インデックス数が寸法と一致しません (${indices.length} != ${width * height})`);
  }

  // フィルタ種別バイト(0) + 行データ
  const raw = new Uint8Array(height * (width + 1));
  for (let y = 0; y < height; y++) {
    raw[y * (width + 1)] = 0;
    raw.set(indices.subarray(y * width, (y + 1) * width), y * (width + 1) + 1);
  }
  const compressed = await deflate(raw);

  const chunks = [];

  const ihdr = new Uint8Array(13);
  writeUint32(ihdr, 0, width);
  writeUint32(ihdr, 4, height);
  ihdr[8] = 8;   // bitdepth
  ihdr[9] = 3;   // colortype = palette
  ihdr[10] = 0;  // compression
  ihdr[11] = 0;  // filter
  ihdr[12] = 0;  // interlace
  chunks.push(["IHDR", ihdr]);

  const plte = new Uint8Array(palette.length * 3);
  palette.forEach((c, i) => {
    plte[i * 3] = c.r; plte[i * 3 + 1] = c.g; plte[i * 3 + 2] = c.b;
  });
  chunks.push(["PLTE", plte]);

  // tRNS は末尾の不透明分を省略できる。透過色が無いなら出力しない。
  let lastTransparent = -1;
  palette.forEach((c, i) => { if ((c.a ?? 255) !== 255) lastTransparent = i; });
  if (lastTransparent >= 0) {
    const trns = new Uint8Array(lastTransparent + 1);
    for (let i = 0; i <= lastTransparent; i++) trns[i] = palette[i].a ?? 255;
    chunks.push(["tRNS", trns]);
  }

  chunks.push(["IDAT", compressed]);
  chunks.push(["IEND", new Uint8Array(0)]);

  let total = 8;
  for (const [, data] of chunks) total += 12 + data.length;

  const out = new Uint8Array(total);
  out.set(PNG_SIGNATURE, 0);
  let pos = 8;
  for (const [type, data] of chunks) {
    writeUint32(out, pos, data.length);
    const typeAndData = new Uint8Array(4 + data.length);
    for (let i = 0; i < 4; i++) typeAndData[i] = type.charCodeAt(i);
    typeAndData.set(data, 4);
    out.set(typeAndData, pos + 4);
    writeUint32(out, pos + 4 + typeAndData.length, crc32(typeAndData));
    pos += 12 + data.length;
  }
  return out;
}

/**
 * インデックス列とパレットから ImageData を作る（canvas 表示用）。
 */
export function indicesToImageData(indices, width, height, palette) {
  const data = new Uint8ClampedArray(width * height * 4);
  for (let i = 0; i < indices.length; i++) {
    const c = palette[indices[i]] ?? { r: 0, g: 0, b: 0, a: 0 };
    data[i * 4]     = c.r;
    data[i * 4 + 1] = c.g;
    data[i * 4 + 2] = c.b;
    data[i * 4 + 3] = c.a ?? 255;
  }
  return new ImageData(data, width, height);
}
