/**
 * dot-store.js
 * ブラウザの IndexedDB に置くもの（作業フォルダの記憶・一時保存の控え・ゲームで試す用の絵）。
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
// ---------------------------------------------------------------

export async function putDraft(draft) {
  const db = await editorDb();
  await run(db, 'drafts', 'readwrite', (s) => s.put(draft));
}

export async function getDraft(id) {
  const db = await editorDb();
  return (await run(db, 'drafts', 'readonly', (s) => s.get(id))) || null;
}

export async function deleteDraft(id) {
  const db = await editorDb();
  await run(db, 'drafts', 'readwrite', (s) => s.delete(id));
}

/** 控えを全部（大きいデータごと）読む。一覧に使うので新しい順に並べる */
export async function listDrafts() {
  const db = await editorDb();
  const all = (await run(db, 'drafts', 'readonly', (s) => s.getAll())) || [];
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
