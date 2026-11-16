/**
 * dot-store.js
 * ブラウザの IndexedDB に置くもの（作業フォルダの記憶・自動の控え・ゲームで試す用の絵）と、
 * サーバーに置く手の控え（一時保存）の読み書き。
 *
 * DB は 2 つ:
 *   - sbo-dot-editor  : エディタ自身のもの。handles（フォルダの記憶）と drafts（控え）
 *   - sbo-dot-preview : ゲーム側が読む約束の置き場。version 1 / objectStore "files" / keyPath なし。
 *                       キー = フォルダにあるそのままの PNG ファイル名、
 *                       値 = {name, bytes: ArrayBuffer, savedAt: Date.now()}
 *                       （ゲーム側は別の人が作るので、この形を変えないこと）
 */

function openDb(name, version, upgrade) {
  return new Promise((resolve, reject) => {
    const req = indexedDB.open(name, version);
    req.onupgradeneeded = () => upgrade(req.result);
    req.onsuccess = () => resolve(req.result);
    req.onerror = () => reject(req.error);
    req.onblocked = () => reject(new Error('データベースが他のタブで使われていて開けません'));
  });
}

let editorDbPromise = null;
function editorDb() {
  if (!editorDbPromise) {
    editorDbPromise = openDb('sbo-dot-editor', 1, (db) => {
      db.createObjectStore('handles');
      const drafts = db.createObjectStore('drafts', { keyPath: 'id' });
      drafts.createIndex('file', 'file');
    });
    // 開けなかった時に次回やり直せるようにする
    editorDbPromise.catch(() => { editorDbPromise = null; });
  }
  return editorDbPromise;
}

let previewDbPromise = null;
function previewDb() {
  if (!previewDbPromise) {
    previewDbPromise = openDb('sbo-dot-preview', 1, (db) => {
      // ゲーム側と取り決めた形: keyPath なし
      if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
    });
    previewDbPromise.catch(() => { previewDbPromise = null; });
  }
  return previewDbPromise;
}

/** 1 回のリクエストを Promise にする */
function run(db, storeName, mode, fn) {
  return new Promise((resolve, reject) => {
    const tx = db.transaction(storeName, mode);
    const store = tx.objectStore(storeName);
    const req = fn(store);
    let result;
    if (req) req.onsuccess = () => { result = req.result; };
    tx.oncomplete = () => resolve(result);
    tx.onerror = () => reject(tx.error);
    tx.onabort = () => reject(tx.error || new Error('保存が中断されました'));
  });
}

// ---------------------------------------------------------------
// 作業フォルダの記憶
// ---------------------------------------------------------------

export async function saveDirHandle(handle) {
  const db = await editorDb();
  await run(db, 'handles', 'readwrite', (s) => s.put(handle, 'dir'));
}

export async function loadDirHandle() {
  const db = await editorDb();
  return (await run(db, 'handles', 'readonly', (s) => s.get('dir'))) || null;
}

// ---------------------------------------------------------------
// 控え（一時保存）
//   手で残す控え（kind 'manual'）はサーバー（/api/admin/dot/drafts/*。管理者ログインが要る）に置き、
//   PC・タブレットなどどの端末からでも続きを描けるようにする。
//   自動の控え（auto）と保存前の元の絵（orig）は、その端末の IndexedDB に置く。
//   サーバーに届かない時（ログインしていない・通信できない）は、手の控えもこの端末に置く。
//   一覧ではサーバーのものに remote: true を付ける（中身の layers は開く時に読む）。
// ---------------------------------------------------------------

const API = '/api/admin/dot/drafts/';
/** サーバーの控えの状態: 'ok' | 'login'（ログインしていない） | 'offline'（通信できない） | ''（まだ） */
let remoteState = '';
const remoteIds = new Set();
let remoteCache = null; // {at, items}

export function getRemoteState() { return remoteState; }
export function isRemote(id) { return remoteIds.has(id); }

async function post(name, body) {
  let res;
  try {
    res = await fetch(API + name, {
      method: 'POST', credentials: 'same-origin',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body || {}),
    });
  } catch (err) {
    remoteState = 'offline';
    throw Object.assign(new Error('サーバーに届きませんでした'), { offline: true });
  }
  const data = await res.json().catch(() => ({}));
  if (res.status === 401) {
    remoteState = 'login';
    throw Object.assign(new Error('管理ページにログインしていません'), { login: true });
  }
  if (res.status === 404 && (name === 'list' || name === 'save')) {
    // サーバーが古くてこの API が無い
    remoteState = 'offline';
    throw Object.assign(new Error('サーバーがまだ控えの保存に対応していません'), { offline: true });
  }
  if (res.status === 413) throw Object.assign(new Error('絵が大きすぎてサーバーに送れません'), { offline: true });
  if (!res.ok && !data.error) throw new Error(`サーバーのエラー（${res.status}）`);
  remoteState = 'ok';
  return data;
}

// Int16Array ⇔ base64（リトルエンディアンのまま）
function b64ToU8(b64) {
  const s = atob(b64);
  const u8 = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) u8[i] = s.charCodeAt(i);
  return u8;
}
function u8ToB64(u8) {
  let s = '';
  for (let i = 0; i < u8.length; i += 0x8000) s += String.fromCharCode.apply(null, u8.subarray(i, i + 0x8000));
  return btoa(s);
}
async function pipeBytes(u8, stream) {
  const buf = await new Response(new Blob([u8]).stream().pipeThrough(stream)).arrayBuffer();
  return new Uint8Array(buf);
}
// レイヤーの中身を送る形に（gzip で縮められるブラウザなら縮める）
async function packLayer(data) {
  const a = Int16Array.from(data);
  const u8 = new Uint8Array(a.buffer);
  if (typeof CompressionStream === 'function') {
    try { return { data: u8ToB64(await pipeBytes(u8, new CompressionStream('gzip'))), enc: 'gz' }; } catch { /* 縮めずに送る */ }
  }
  return { data: u8ToB64(u8) };
}
async function unpackLayer(l) {
  let u8 = b64ToU8(l.data);
  if (l.enc === 'gz') u8 = await pipeBytes(u8, new DecompressionStream('gzip'));
  return new Int16Array(u8.buffer, u8.byteOffset, u8.byteLength >> 1).slice();
}

async function listRemote() {
  if (remoteCache && Date.now() - remoteCache.at < 1500) return remoteCache.items;
  try {
    const data = await post('list');
    const items = (data.items || []).map((m) => ({ ...m, remote: true, auto: false }));
    remoteIds.clear();
    for (const m of items) remoteIds.add(m.id);
    remoteCache = { at: Date.now(), items };
    return items;
  } catch (err) {
    if (!err.login && !err.offline) console.error(err);
    return [];
  }
}

function dropCache() { remoteCache = null; }

async function localPut(draft) {
  const db = await editorDb();
  await run(db, 'drafts', 'readwrite', (s) => s.put(draft));
}

/**
 * 控えを保存する。手の控えはサーバーへ（届かなければこの端末へ）。
 * opts.baseSavedAt: 開いた時の保存時刻（ほかの端末で上書きされていないかの確かめに使う）
 * 戻り値: {where: 'server'|'local', savedAt, conflict?, reason?}
 */
export async function putDraft(draft, opts = {}) {
  const manual = !draft.auto && (draft.kind || 'manual') === 'manual';
  if (manual && !opts.localOnly) {
    try {
      const item = {
        ...draft,
        layers: await Promise.all(draft.layers.map(async (l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, ...(await packLayer(l.data)) }))),
      };
      const data = await post('save', { item, baseSavedAt: opts.baseSavedAt || 0 });
      dropCache();
      if (data.error === 'conflict') return { where: 'server', conflict: true };
      if (data.error === 'full') throw new Error(`サーバーの控えがいっぱいです（${data.max} 件まで）。古い控えを消してください`);
      if (data.error) throw new Error('サーバーが受け取れませんでした（' + data.error + '）');
      remoteIds.add(draft.id);
      // この端末に同じ id の控えがあれば（サーバーへ移した時など）消す
      try { const db = await editorDb(); await run(db, 'drafts', 'readwrite', (s) => s.delete(draft.id)); } catch { /* 無くてよい */ }
      return { where: 'server', savedAt: data.item.savedAt };
    } catch (err) {
      if (!err.login && !err.offline) throw err;
      await localPut(draft);
      return { where: 'local', savedAt: draft.savedAt, reason: err.message };
    }
  }
  await localPut(draft);
  return { where: 'local', savedAt: draft.savedAt };
}

/** 一覧の 1 件（サーバーのものは中身なし）から、開ける控え（layers 入り）を作る */
export async function loadDraft(d) {
  if (!d.remote) return d;
  const data = await post('get', { id: d.id });
  if (!data.item) throw new Error(data.error === 'not_found' ? 'この控えはもうサーバーにありません（ほかの端末で消された？）' : 'サーバーから読めませんでした');
  const it = data.item;
  const layers = await Promise.all(it.layers.map(async (l) => ({ name: l.name, visible: l.visible, ghost: l.ghost, data: await unpackLayer(l) })));
  return { ...it, remote: true, auto: false, layers };
}

export async function renameDraft(d, name) {
  if (d.remote || remoteIds.has(d.id)) {
    const data = await post('rename', { id: d.id, name });
    dropCache();
    if (data.error) throw new Error('名前を変えられませんでした（' + data.error + '）');
    return;
  }
  const full = await getDraft(d.id);
  if (!full) return;
  full.name = name;
  await localPut(full);
}

/** この端末の控え（id で引く） */
export async function getDraft(id) {
  const db = await editorDb();
  return (await run(db, 'drafts', 'readonly', (s) => s.get(id))) || null;
}

export async function deleteDraft(id) {
  if (remoteIds.has(id)) {
    const data = await post('delete', { id });
    dropCache();
    if (data.error && data.error !== 'not_found') throw new Error('消せませんでした（' + data.error + '）');
    remoteIds.delete(id);
    return;
  }
  const db = await editorDb();
  await run(db, 'drafts', 'readwrite', (s) => s.delete(id));
}

/** 控えを全部（この端末の分は大きいデータごと・サーバーの分は一覧だけ）読む。新しい順に並べる */
export async function listDrafts() {
  const db = await editorDb();
  const local = (await run(db, 'drafts', 'readonly', (s) => s.getAll())) || [];
  const remote = await listRemote();
  const ids = new Set(remote.map((d) => d.id));
  const all = remote.concat(local.filter((d) => !ids.has(d.id)));
  all.sort((a, b) => b.savedAt - a.savedAt);
  return all;
}

// ---------------------------------------------------------------
// ゲームで試す
// ---------------------------------------------------------------

export async function putPreview(fileName, bytes) {
  // ゲーム内のパスは NFC。Mac のフォルダが NFD で返しても、キーと name は必ず NFC にそろえる
  fileName = fileName.normalize('NFC');
  const db = await previewDb();
  // 渡された Uint8Array の裏の ArrayBuffer がずれていても大丈夫なように、ぴったりの ArrayBuffer にする
  const buf = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
  await run(db, 'files', 'readwrite', (s) => s.put({ name: fileName, bytes: buf, savedAt: Date.now() }, fileName));
}

export async function listPreviews() {
  const db = await previewDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction('files', 'readonly');
    const store = tx.objectStore('files');
    const out = [];
    const req = store.openCursor();
    req.onsuccess = () => {
      const cur = req.result;
      if (cur) {
        const v = cur.value || {};
        out.push({ key: cur.key, name: v.name || String(cur.key), size: v.bytes ? v.bytes.byteLength : 0, savedAt: v.savedAt || 0 });
        cur.continue();
      } else {
        resolve(out);
      }
    };
    req.onerror = () => reject(req.error);
  });
}

export async function deletePreview(key) {
  const db = await previewDb();
  await run(db, 'files', 'readwrite', (s) => s.delete(key));
}

export async function clearPreviews() {
  const db = await previewDb();
  await run(db, 'files', 'readwrite', (s) => s.clear());
}
