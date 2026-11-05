/* ========================================================================= */
/* sbosvr_data.js															 */
/* AI 支援用のデータ API と MCP の入口（Emscripten の --pre-js で組み込む）		 */
/* ========================================================================= */
/*
 * 管理者ウィンドウで編集している設定データ（特技・職業とレベルアップ・特技を使える職業・
 * 装備できる職業・ステータステンプレート・アイテム）を、AI から読み書きできるようにする。
 * セーブデータの残り（キャラ・マップとマス目・マップパーツ・エフェクト・吹き出し・預かり所・サブゲーム）は
 * 読むだけ（バランス調整のシミュレーションやシナリオ作りの手伝い用）。アカウント名・パスワード・IP・MAC は出さない。
 * データそのものの読み書きは C 側（server/web_data_svr.cpp）が行い、ここは次を受け持つ。
 *
 * 1. AI 用キー
 *    管理ページ（/admin の「AI連携」）で発行する。読むだけ（read）か書き込みもできる（write）を選び、
 *    期限（日数）を付ける。キーそのものは発行した時に一度だけ見せ、サーバーは SHA-256 だけを
 *    SBODATA/SboWebAiKeys.json に保存する。取り消しもできる。管理者パスワードは AI に渡さない。
 *
 * 2. 下見 → 確定の 2 段階
 *    書き込み（update・add・undo）は、previewId を付けずに呼ぶと何も変えずに「変更前後の差分」と
 *    previewId を返す。同じ内容に previewId を付けてもう一度呼ぶと反映する。
 *    previewId は 10 分で切れ、1 回しか使えない。下見の後に誰かがそのデータを変えていたら反映しない。
 *
 * 3. 変更履歴と元に戻す
 *    反映するたびに、変更前と変更後の姿を SBODATA/SboWebDataHistory.json に残す（直近 MAX_HISTORY 件）。
 *    1 回の呼び出し（1 種類のデータ・何件でも）＝履歴 1 件で、元に戻すのもその単位。
 *    その後に別の変更があったデータは、force を付けない限り戻さない。追加したデータは消せないので戻せない。
 *    さらに、その日初めてそのデータを変える前に、データファイルを SBODATA/AiBackup/日付/ へ写しておく。
 *
 * 4. MCP（Model Context Protocol）
 *    POST /mcp で JSON-RPC を受ける（Streamable HTTP。応答は JSON だけでストリームは使わない）。
 *    Claude Code なら次の 1 行でつながる。
 *      claude mcp add --transport http sbo2003 https://<ホスト>/mcp --header "Authorization: Bearer <AI用キー>"
 *
 * 環境変数 SBO_AI_API=off で、この API と /mcp を止められる。
 *
 * HTTP（/api/data/ はすべて POST・JSON。キーは Authorization: Bearer <キー> か X-Sbo-Key ヘッダー）
 *   /api/data/schema    {}                                   → {kinds:[...]}
 *   /api/data/list      {kind, q?, offset?, limit?, fields?} → {kind, total, offset, records:[...]}
 *   /api/data/get       {kind, id}                           → {record}
 *   /api/data/tiles     {id, x?, y?, w?, h?}                 → マップのマス目（parts・flags・roof、最大 128×128）
 *   /api/data/update    {kind, updates:[{id, changes}], note?, previewId?}
 *   /api/data/add       {kind, records:[{...}], note?, previewId?}
 *   /api/data/history   {kind?, limit?}                      → {entries:[...]}
 *   /api/data/undo      {historyId, note?, force?, previewId?}
 *     書き込みは previewId 無し → {preview:true, previewId, items:[{id, before, after, diff}]}
 *                previewId 有り → {ok:true, historyId, items:[...]}
 * 管理ページ用（{session} は /api/admin/login で得たもの）
 *   /api/admin/ai/keys     {session}                          → {keys:[...]}
 *   /api/admin/ai/issue    {session, name, write, days}       → {key, info}
 *   /api/admin/ai/revoke   {session, id}                      → {ok}
 *   /api/admin/ai/history  {session, kind?, limit?}           → {entries:[...]}
 *   /api/admin/ai/undo     {session, historyId, force?, previewId?} → 上の undo と同じ
 *   /api/admin/data/list   {session, kind, offset?, limit?}   → 上の list と同じ（マップ資料の書き出し用。読むだけ）
 *   /api/admin/data/tiles  {session, id, x?, y?, w?, h?}      → 上の tiles と同じ
 */
Module['sboData'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');

	var ENABLED = String(process.env.SBO_AI_API || 'on').toLowerCase() !== 'off';
	var MAX_BODY = 256 * 1024;		/* 受け取る JSON の大きさ */
	var MAX_HISTORY = 500;			/* 残す変更履歴の数 */
	var MAX_KEYS = 50;				/* 発行できるキーの数（取り消したものを含めない） */
	var MAX_ITEMS = 200;			/* 1 回で変えられる件数 */
	var PREVIEW_TTL = 10 * 60 * 1000;
	var MINUTE = 60 * 1000, DAY = 24 * 60 * MINUTE;
	var KEY_PREFIX = 'sboai_';

	/* データの種類 → 保存するファイル（日ごとの控えを取る） */
	var KIND_FILE = {
		magic: 'SboDataMagicInfo.dat',
		magicUser: 'SboDataMagicUserInfo.dat',
		equUser: 'SboDataEquUserInfo.dat',
		job: 'SboDataJobInfo.dat',
		stTmp: 'SboDataStTmpInfo.dat',
		item: 'SboDataItemInfo.dat'
	};
	/* 読むだけの種類（セーブデータの残り。アカウント名・パスワード・IP などは C 側で出さない） */
	var READONLY_KINDS = ['char', 'map', 'mapParts', 'efc', 'comEfc', 'leave', 'subGame'];
	var ALL_KINDS = Object.keys(KIND_FILE).concat(READONLY_KINDS);

	var keyStore = null, strKeyPath = '';
	var histStore = null, strHistPath = '';
	var aPreview = {};				/* previewId → { hash, expires, keyId, befores } */
	var aRate = {};					/* キー → { start, count }（1 分に 300 回まで） */
	var schemaCache = null;

	function log(msg) {
		console.log('[data] ' + msg);
	}

	function acc() {
		return Module['sboAccount'];
	}

	function sha256(s) {
		return crypto.createHash('sha256').update(String(s)).digest('hex');
	}

	function dataDir() {
		/* NODERAWFS なので FS.cwd() は実際のカレントフォルダ（--data で移った先） */
		return path.resolve(FS.cwd(), 'SBODATA');
	}

	function readJsonFile(file, def) {
		try {
			var o = JSON.parse(fs.readFileSync(file, 'utf8'));
			return (o && typeof o === 'object') ? o : def;
		} catch (e) {
			return def;
		}
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function writeJsonFile(file, obj) {
		var tmp = file + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(obj, null, '\t'));
			fs.renameSync(tmp, file);
		} catch (e) {
			console.error('[data] 保存できません: ' + file + ' ' + e.message);
		}
	}

	function keys() {
		if (!keyStore) {
			strKeyPath = path.join(dataDir(), 'SboWebAiKeys.json');
			keyStore = readJsonFile(strKeyPath, {});
			keyStore.version = 1;
			keyStore.keys = keyStore.keys || [];
		}
		return keyStore;
	}

	function history() {
		if (!histStore) {
			strHistPath = path.join(dataDir(), 'SboWebDataHistory.json');
			histStore = readJsonFile(strHistPath, {});
			histStore.version = 1;
			histStore.seq = histStore.seq || 0;
			histStore.entries = histStore.entries || [];
		}
		return histStore;
	}

	/* ------------------------------------------------------------------ */
	/* C 側の呼び出し															 */
	/* ------------------------------------------------------------------ */
	/* 文字列は UTF-8 にして渡し、数はそのまま渡す */
	function callC(fn, args) {
		var aFree = [];
		var aArg = args.map(function (a) {
			if (typeof a === 'number') {
				return a | 0;
			}
			var p = stringToNewUTF8(String(a == null ? '' : a));
			aFree.push(p);
			return p;
		});
		try {
			return fn.apply(null, aArg);
		} finally {
			aFree.forEach(function (p) { _free(p); });
		}
	}

	function schema() {
		var s = JSON.parse(UTF8ToString(callC(_SboWebDataSchema, [])));
		schemaCache = s;
		return s;
	}

	function kindInfo(kind) {
		var s = schemaCache || schema();
		for (var i = 0; i < s.kinds.length; i++) {
			if (s.kinds[i].kind === kind) {
				return s.kinds[i];
			}
		}
		return null;
	}

	function count(kind) {
		return callC(_SboWebDataCount, [kind]);
	}

	function getRecord(kind, id) {
		return JSON.parse(UTF8ToString(callC(_SboWebDataGet, [kind, id])));
	}

	/* source は管理者ウィンドウの「元に戻す」に出る名前 */
	function applyC(kind, id, changes, bApply, source) {
		return JSON.parse(UTF8ToString(callC(_SboWebDataApply, [kind, id, JSON.stringify(changes), bApply ? 1 : 0, source || 'API'])));
	}

	/* ------------------------------------------------------------------ */
	/* 下請け																	 */
	/* ------------------------------------------------------------------ */
	function ApiError(status, code, message, extra) {
		this.status = status;
		this.code = code;
		this.message = message;
		this.extra = extra;
	}

	function fail(status, code, message, extra) {
		throw new ApiError(status, code, message, extra);
	}

	function checkKind(kind) {
		if (typeof kind !== 'string' || ALL_KINDS.indexOf(kind) < 0) {
			fail(400, 'kind', '知らない種類です（' + ALL_KINDS.join(' / ') + '）');
		}
		return kind;
	}

	/* 書き込める種類か */
	function checkWritableKind(kind) {
		checkKind(kind);
		if (!KIND_FILE[kind]) {
			fail(400, 'readonly', kind + ' は読むだけです（書き込めるのは ' + Object.keys(KIND_FILE).join(' / ') + '）');
		}
		return kind;
	}

	function checkId(id) {
		if (typeof id !== 'number' || !isFinite(id) || id < 0 || Math.floor(id) !== id) {
			fail(400, 'id', 'id は 0 以上の整数で指定してください');
		}
		return id;
	}

	function same(a, b) {
		return JSON.stringify(a) === JSON.stringify(b);
	}

	/* 変わった項目だけを {項目: {from, to}} で返す（detail は 1 段下まで見る） */
	function diff(before, after) {
		var out = {};
		var aKey = {};
		Object.keys(before || {}).forEach(function (k) { aKey[k] = 1; });
		Object.keys(after || {}).forEach(function (k) { aKey[k] = 1; });
		delete aKey.id;
		Object.keys(aKey).forEach(function (k) {
			var a = before ? before[k] : undefined, b = after ? after[k] : undefined;
			if (same(a, b)) {
				return;
			}
			if (k === 'detail' && a && b && typeof a === 'object' && typeof b === 'object' && !Array.isArray(a)) {
				var d = diff(a, b);
				Object.keys(d).forEach(function (k2) { out['detail.' + k2] = d[k2]; });
				return;
			}
			/* レベルアップ情報のような「オブジェクトの配列」は行ごとに見る */
			if (Array.isArray(a) && Array.isArray(b) && (a.concat(b)).every(function (x) { return x && typeof x === 'object'; })) {
				for (var i = 0; i < Math.max(a.length, b.length); i++) {
					if (i >= b.length) {
						out[k + '[' + i + ']'] = { from: a[i], to: null };
					} else if (i >= a.length) {
						out[k + '[' + i + ']'] = { from: null, to: b[i] };
					} else {
						var d2 = diff(a[i], b[i]);
						Object.keys(d2).forEach(function (k2) { out[k + '[' + i + '].' + k2] = d2[k2]; });
					}
				}
				return;
			}
			out[k] = { from: a === undefined ? null : a, to: b === undefined ? null : b };
		});
		return out;
	}

	/* 元に戻す時に渡す変更（読むだけの項目と id を除いた丸ごと） */
	function restorable(kind, rec) {
		var out = {};
		var info = kindInfo(kind);
		var aRo = {};
		(info ? info.fields : []).forEach(function (f) {
			if (f.readonly) {
				aRo[f.name] = 1;
			}
		});
		Object.keys(rec).forEach(function (k) {
			if (k !== 'id' && !aRo[k]) {
				out[k] = rec[k];
			}
		});
		return out;
	}

	/* その日初めてそのデータを変える前に、データファイルの控えを取る */
	function backupFile(kind) {
		var name = KIND_FILE[kind];
		var d = new Date();
		var day = d.getFullYear() + ('0' + (d.getMonth() + 1)).slice(-2) + ('0' + d.getDate()).slice(-2);
		var dir = path.join(dataDir(), 'AiBackup', day);
		var dst = path.join(dir, name);
		try {
			if (fs.existsSync(dst)) {
				return;
			}
			fs.mkdirSync(dir, { recursive: true });
			fs.copyFileSync(path.join(dataDir(), name), dst);
		} catch (e) {
			console.error('[data] 控えを取れません: ' + name + ' ' + e.message);
		}
	}

	/* ------------------------------------------------------------------ */
	/* 下見 → 確定																 */
	/* ------------------------------------------------------------------ */
	function payloadHash(op, payload, who) {
		return sha256(op + '\n' + who.id + '\n' + JSON.stringify(payload));
	}

	function makePreview(op, payload, who, befores, items) {
		var now = Date.now();
		Object.keys(aPreview).forEach(function (k) {
			if (aPreview[k].expires < now) {
				delete aPreview[k];
			}
		});
		var id = crypto.randomBytes(12).toString('hex');
		aPreview[id] = { hash: payloadHash(op, payload, who), expires: now + PREVIEW_TTL, befores: befores };
		return {
			preview: true,
			previewId: id,
			message: 'まだ何も変えていません。内容を確かめて、同じ内容に previewId を付けてもう一度呼ぶと反映します（10 分以内・1 回だけ）。',
			items: items
		};
	}

	/* previewId が使えるか確かめて取り出す（1 回で消す） */
	function takePreview(previewId, op, payload, who) {
		var p = aPreview[String(previewId)];
		delete aPreview[String(previewId)];
		if (!p || p.expires < Date.now()) {
			fail(409, 'preview_expired', 'previewId が見つからないか期限切れです。もう一度 previewId 無しで呼んで下見してください。');
		}
		if (p.hash !== payloadHash(op, payload, who)) {
			fail(409, 'preview_mismatch', '下見した時と内容が違います。もう一度 previewId 無しで呼んで下見してください。');
		}
		return p;
	}

	/* 下見の後に、対象のデータが変わっていないか */
	function checkUnchanged(kind, befores) {
		Object.keys(befores).forEach(function (id) {
			var now = getRecord(kind, Number(id));
			if (!same(now, befores[id])) {
				fail(409, 'changed', '下見の後に ' + kind + ' の ' + id + ' 番が変わりました。もう一度下見してください。');
			}
		});
	}

	/* 履歴に残す */
	function addHistory(entry) {
		var h = history();
		h.seq++;
		entry.historyId = h.seq;
		entry.at = Date.now();
		h.entries.push(entry);
		if (h.entries.length > MAX_HISTORY) {
			h.entries.splice(0, h.entries.length - MAX_HISTORY);
		}
		writeJsonFile(strHistPath, h);
		return entry;
	}

	/* まとめて反映する。途中で失敗したら、それまでに変えたものを元へ戻す */
	function applyAll(kind, aOp, who) {
		var aDone = [];
		backupFile(kind);
		for (var i = 0; i < aOp.length; i++) {
			var op = aOp[i];
			var before = op.added ? null : getRecord(kind, op.id);
			var r = applyC(kind, op.id, op.changes, true, who.label);
			if (!r.ok) {
				for (var j = aDone.length - 1; j >= 0; j--) {
					if (!aDone[j].added) {
						applyC(kind, aDone[j].id, restorable(kind, aDone[j].before), true, who.label);
					}
				}
				fail(400, 'apply', kind + ' の ' + op.id + ' 番を反映できませんでした: ' + r.error +
						(aDone.length ? '（それまでの変更は元に戻しました）' : ''));
			}
			aDone.push({ id: r.id, added: r.added, before: before, after: r.record });
		}
		schemaCache = null;
		return aDone;
	}

	/* ------------------------------------------------------------------ */
	/* 操作（REST と MCP で共通）												 */
	/* ------------------------------------------------------------------ */
	function opSchema() {
		var s = schema();
		s.howTo = [
			'書き込み（update・add・undo）は 2 段階: まず previewId 無しで呼ぶと差分と previewId が返る（何も変わらない）。',
			'利用者に差分を見せて了解をもらってから、同じ内容に previewId を付けてもう一度呼ぶと反映される。',
			'反映すると、ログイン中のプレイヤーにもすぐ届き、データファイルも保存される。履歴から undo で戻せる。',
			'番号（id）は 0 から。他のデータの番号を参照する項目は note に書いてある。'
		];
		return s;
	}

	function opList(a) {
		var kind = checkKind(a.kind);
		var total = count(kind);
		var offset = Math.max(0, Math.floor(Number(a.offset) || 0));
		var limit = Math.min(500, Math.max(1, Math.floor(Number(a.limit) || 100)));
		var q = a.q != null ? String(a.q).slice(0, 64) : '';
		var aField = Array.isArray(a.fields) ? a.fields.map(String) : null;
		var match = (a.match && typeof a.match === 'object' && !Array.isArray(a.match)) ? a.match : null;
		var aOut = [];
		var nMatch = 0;
		for (var i = 0; i < total && aOut.length < limit; i++) {
			var rec = getRecord(kind, i);
			if (!rec) {
				continue;
			}
			if (q && String(rec.szName || '').indexOf(q) < 0) {
				continue;
			}
			if (match && !Object.keys(match).every(function (f) { return rec[f] === match[f]; })) {
				continue;
			}
			if (nMatch++ < offset) {
				continue;
			}
			if (aField) {
				var r2 = { id: rec.id };
				aField.forEach(function (f) {
					if (f in rec) {
						r2[f] = rec[f];
					}
				});
				rec = r2;
			}
			aOut.push(rec);
		}
		return { kind: kind, total: total, offset: offset, count: aOut.length, more: i < total, records: aOut };
	}

	function opGet(a) {
		var kind = checkKind(a.kind);
		var rec = getRecord(kind, checkId(a.id));
		if (!rec) {
			fail(404, 'not_found', kind + ' に ' + a.id + ' 番はありません（' + count(kind) + ' 件）');
		}
		return { record: rec };
	}

	/* マップのマス目（map の id で指定。w・h を省くと右下まで、最大 128×128） */
	function opTiles(a) {
		var id = checkId(a.id);
		var n = function (v) { return (typeof v === 'number' && isFinite(v)) ? Math.floor(v) : 0; };
		var r = JSON.parse(UTF8ToString(_SboWebDataMapTiles(id, n(a.x), n(a.y), n(a.w), n(a.h))));
		if (r.error) {
			fail(404, 'not_found', r.error);
		}
		r.note = 'parts[行][列] は mapParts の番号、flags はマスのフラグ、roof は屋根レベル。左上が (x, y)';
		return r;
	}

	function opUpdate(a, who) {
		requireWrite(who);
		var kind = checkWritableKind(a.kind);
		if (!Array.isArray(a.updates) || a.updates.length === 0 || a.updates.length > MAX_ITEMS) {
			fail(400, 'updates', 'updates は [{id, changes}] の配列で、1〜' + MAX_ITEMS + ' 件にしてください');
		}
		var aSeen = {};
		var total = count(kind);
		var befores = {}, items = [], aOp = [];
		a.updates.forEach(function (u) {
			var id = checkId(u && u.id);
			if (id >= total) {
				fail(400, 'id', kind + ' に ' + id + ' 番はありません（追加は add を使ってください）');
			}
			if (aSeen[id]) {
				fail(400, 'duplicate', '同じ番号（' + id + '）が 2 回あります。1 つにまとめてください');
			}
			aSeen[id] = 1;
			if (!u.changes || typeof u.changes !== 'object' || Array.isArray(u.changes)) {
				fail(400, 'changes', 'changes は変える項目だけのオブジェクトで指定してください');
			}
			var before = getRecord(kind, id);
			var r = applyC(kind, id, u.changes, false);
			if (!r.ok) {
				fail(400, 'invalid', kind + ' の ' + id + ' 番: ' + r.error);
			}
			befores[id] = before;
			items.push({ id: id, name: before.szName, diff: diff(before, r.record) });
			aOp.push({ id: id, changes: u.changes });
		});
		var payload = { kind: kind, updates: a.updates, note: a.note || '' };
		if (!a.previewId) {
			return makePreview('update', payload, who, befores, items);
		}
		var p = takePreview(a.previewId, 'update', payload, who);
		checkUnchanged(kind, p.befores);
		var aDone = applyAll(kind, aOp, who);
		var e = addHistory({ op: 'update', kind: kind, by: who.label, note: String(a.note || '').slice(0, 500), items: aDone });
		log(who.label + ' が ' + kind + ' を ' + aDone.length + ' 件変更（履歴 ' + e.historyId + '）');
		return { ok: true, historyId: e.historyId, items: aDone.map(function (d) { return { id: d.id, diff: diff(d.before, d.after) }; }) };
	}

	function opAdd(a, who) {
		requireWrite(who);
		var kind = checkWritableKind(a.kind);
		if (!Array.isArray(a.records) || a.records.length === 0 || a.records.length > MAX_ITEMS) {
			fail(400, 'records', 'records は追加する内容の配列で、1〜' + MAX_ITEMS + ' 件にしてください');
		}
		var total = count(kind);
		var info = kindInfo(kind);
		if (info && total + a.records.length > info.maxCount) {
			fail(400, 'too_many', kind + ' は ' + info.maxCount + ' 件までです');
		}
		var items = [], aOp = [];
		a.records.forEach(function (rec, i) {
			if (!rec || typeof rec !== 'object' || Array.isArray(rec)) {
				fail(400, 'records', 'records の各要素はオブジェクトで指定してください');
			}
			/* 追加の確かめは「次の番号」で行う（番号は反映する時に順に決まる） */
			var r = applyC(kind, total, rec, false);
			if (!r.ok) {
				fail(400, 'invalid', 'records[' + i + ']: ' + r.error);
			}
			r.record.id = total + i;
			items.push({ id: total + i, after: r.record });
			aOp.push({ id: total + i, changes: rec, added: true });
		});
		var payload = { kind: kind, records: a.records, note: a.note || '' };
		if (!a.previewId) {
			var pv = makePreview('add', payload, who, {}, items);
			pv.message += '（追加は元に戻せません）';
			return pv;
		}
		takePreview(a.previewId, 'add', payload, who);
		if (count(kind) !== total) {
			fail(409, 'changed', '下見の後に ' + kind + ' の件数が変わりました。もう一度下見してください。');
		}
		var aDone = applyAll(kind, aOp, who);
		var e = addHistory({ op: 'add', kind: kind, by: who.label, note: String(a.note || '').slice(0, 500), items: aDone });
		log(who.label + ' が ' + kind + ' を ' + aDone.length + ' 件追加（履歴 ' + e.historyId + '）');
		return { ok: true, historyId: e.historyId, items: aDone.map(function (d) { return { id: d.id, record: d.after }; }) };
	}

	function historySummary(e) {
		return {
			historyId: e.historyId, at: new Date(e.at).toISOString(), op: e.op, kind: e.kind, by: e.by, note: e.note,
			undoOf: e.undoOf || undefined, undoneBy: e.undoneBy || undefined,
			items: e.items.map(function (d) {
				return { id: d.id, name: (d.after || d.before || {}).szName, added: d.added || undefined, diff: diff(d.before, d.after) };
			})
		};
	}

	function opHistory(a) {
		var h = history();
		var limit = Math.min(100, Math.max(1, Math.floor(Number(a.limit) || 20)));
		var kind = a.kind ? checkKind(a.kind) : null;
		var aOut = [];
		for (var i = h.entries.length - 1; i >= 0 && aOut.length < limit; i--) {
			if (!kind || h.entries[i].kind === kind) {
				aOut.push(historySummary(h.entries[i]));
			}
		}
		return { entries: aOut };
	}

	function opUndo(a, who) {
		requireWrite(who);
		var h = history();
		var hid = Number(a.historyId);
		var e = null;
		h.entries.forEach(function (x) {
			if (x.historyId === hid) {
				e = x;
			}
		});
		if (!e) {
			fail(404, 'not_found', '履歴 ' + a.historyId + ' が見つかりません（古いものは消えています）');
		}
		if (e.undoneBy) {
			fail(400, 'already', '履歴 ' + hid + ' はもう元に戻してあります（履歴 ' + e.undoneBy + '）');
		}
		var kind = e.kind;
		var befores = {}, items = [], aOp = [], aSkip = [], aConflict = [];
		e.items.forEach(function (d) {
			if (d.added) {
				aSkip.push(d.id);
				return;
			}
			var now = getRecord(kind, d.id);
			if (!same(now, d.after)) {
				aConflict.push(d.id);
				if (!a.force) {
					return;
				}
			}
			var changes = restorable(kind, d.before);
			var r = applyC(kind, d.id, changes, false);
			if (!r.ok) {
				fail(400, 'invalid', kind + ' の ' + d.id + ' 番を戻せません: ' + r.error);
			}
			befores[d.id] = now;
			items.push({ id: d.id, name: now && now.szName, diff: diff(now, r.record) });
			aOp.push({ id: d.id, changes: changes });
		});
		if (aConflict.length && !a.force) {
			fail(409, 'conflict', 'この後に変更されたデータがあるので戻しません（' + kind + ' の ' + aConflict.join(', ') +
					' 番）。それでも戻すなら force を付けてください（その後の変更も消えます）。', { conflicts: aConflict });
		}
		if (aOp.length === 0) {
			fail(400, 'nothing', '戻せるものがありません' + (aSkip.length ? '（追加したデータは元に戻せません）' : ''));
		}
		var payload = { historyId: hid, force: !!a.force, note: a.note || '' };
		if (!a.previewId) {
			var pv = makePreview('undo', payload, who, befores, items);
			if (aSkip.length) {
				pv.skipped = aSkip;
				pv.message += '（追加した ' + aSkip.join(', ') + ' 番は消せないので、そのまま残ります）';
			}
			return pv;
		}
		var p = takePreview(a.previewId, 'undo', payload, who);
		checkUnchanged(kind, p.befores);
		var aDone = applyAll(kind, aOp, who);
		var e2 = addHistory({ op: 'undo', kind: kind, by: who.label, undoOf: hid, note: String(a.note || '').slice(0, 500), items: aDone });
		e.undoneBy = e2.historyId;
		writeJsonFile(strHistPath, h);
		log(who.label + ' が履歴 ' + hid + ' を元に戻した（履歴 ' + e2.historyId + '）');
		return { ok: true, historyId: e2.historyId, skipped: aSkip.length ? aSkip : undefined,
			items: aDone.map(function (d) { return { id: d.id, diff: diff(d.before, d.after) }; }) };
	}

	/* ------------------------------------------------------------------ */
	/* AI 用キー																	 */
	/* ------------------------------------------------------------------ */
	function requireWrite(who) {
		if (!who.write) {
			fail(403, 'read_only', 'このキーは読むだけです。書き込みには「書き込みもできる」キーを管理ページで発行してください。');
		}
	}

	function keyPublic(k) {
		return {
			id: k.id, name: k.name, write: k.write, prefix: k.prefix, created: k.created, expires: k.expires,
			lastUsed: k.lastUsed || 0, lastIp: k.lastIp || '', uses: k.uses || 0, revoked: k.revoked || 0,
			active: !k.revoked && (!k.expires || k.expires > Date.now())
		};
	}

	/* リクエストのキーを確かめる。使えなければ null */
	function authKey(req) {
		var s = String(req.headers['authorization'] || '');
		var m = /^Bearer\s+(\S+)$/i.exec(s);
		var key = m ? m[1] : String(req.headers['x-sbo-key'] || '');
		if (key.indexOf(KEY_PREFIX) !== 0 || key.length > 100) {
			return null;
		}
		var hash = sha256(key);
		var ks = keys();
		var k = null;
		ks.keys.forEach(function (x) {
			if (x.hash === hash) {
				k = x;
			}
		});
		if (!k || k.revoked || (k.expires && k.expires < Date.now())) {
			return null;
		}
		/* 使った記録（書き込みは 1 分に 1 回まで） */
		var now = Date.now();
		k.uses = (k.uses || 0) + 1;
		k.lastIp = acc().clientIp(req);
		if (now - (k.lastUsed || 0) > MINUTE) {
			k.lastUsed = now;
			writeJsonFile(strKeyPath, ks);
		}
		return { id: 'key:' + k.id, label: 'AIキー「' + k.name + '」', write: !!k.write, rateKey: k.id };
	}

	/* 1 分に 300 回まで */
	function rateOk(rateKey) {
		var now = Date.now();
		var r = aRate[rateKey];
		if (!r || now - r.start > MINUTE) {
			r = aRate[rateKey] = { start: now, count: 0 };
		}
		r.count++;
		return r.count <= 300;
	}

	function adminWho(body) {
		if (!acc().isAdminSession(body.session)) {
			fail(401, 'session', '管理ページにログインし直してください');
		}
		return { id: 'admin', label: '管理ページ', write: true, rateKey: 'admin' };
	}

	function apiAdminKeys(body) {
		adminWho(body);
		return { keys: keys().keys.slice().reverse().map(keyPublic) };
	}

	function apiAdminIssue(body, req) {
		adminWho(body);
		var ks = keys();
		var nActive = ks.keys.filter(function (k) { return keyPublic(k).active; }).length;
		if (nActive >= MAX_KEYS) {
			fail(400, 'too_many', '使えるキーが多すぎます。使っていないキーを取り消してください。');
		}
		var name = String(body.name || '').trim().slice(0, 40) || 'AI';
		var days = Math.floor(Number(body.days));
		if (!isFinite(days) || days < 0 || days > 365) {
			days = 30;
		}
		var key = KEY_PREFIX + crypto.randomBytes(24).toString('hex');
		var k = {
			id: crypto.randomBytes(6).toString('hex'), name: name, write: !!body.write,
			hash: sha256(key), prefix: key.slice(0, KEY_PREFIX.length + 6),
			created: Date.now(), expires: days ? Date.now() + days * DAY : 0, createdIp: acc().clientIp(req)
		};
		ks.keys.push(k);
		/* 取り消して 30 日たったものは消す */
		ks.keys = ks.keys.filter(function (x) { return !x.revoked || x.revoked > Date.now() - 30 * DAY; });
		writeJsonFile(strKeyPath, ks);
		log('AI用キーを発行: ' + name + '（' + (k.write ? '書き込み可' : '読むだけ') + '）');
		return { key: key, info: keyPublic(k) };
	}

	function apiAdminRevoke(body) {
		adminWho(body);
		var ks = keys();
		var k = null;
		ks.keys.forEach(function (x) {
			if (x.id === String(body.id)) {
				k = x;
			}
		});
		if (!k) {
			fail(404, 'not_found', 'キーが見つかりません');
		}
		if (!k.revoked) {
			k.revoked = Date.now();
			writeJsonFile(strKeyPath, ks);
			log('AI用キーを取り消し: ' + k.name);
		}
		return { ok: true };
	}

	/* ------------------------------------------------------------------ */
	/* MCP																		 */
	/* ------------------------------------------------------------------ */
	var MCP_VERSIONS = ['2025-06-18', '2025-03-26', '2024-11-05'];
	var MCP_INSTRUCTIONS =
		'SBO2003（オンラインRPG）のサーバーの設定データ（特技・職業とレベルアップ・特技を使える職業・装備できる職業・' +
		'ステータステンプレート・アイテム）を読み書きできます。セーブデータの残り（キャラ全員と NPC の台詞・マップのイベント／店／メッセージ・' +
		'マップパーツ・エフェクト・吹き出し・預かり所・サブゲーム）は読むだけです。マップのマス目は sbo_map_tiles で読みます。' +
		'まず sbo_schema で種類と項目の意味を確かめてください。' +
		'書き込みは必ず 2 段階です: previewId 無しで呼ぶと差分が返るので、それを利用者に見せて了解をもらってから、' +
		'同じ引数に previewId を付けてもう一度呼んでください。反映はログイン中のプレイヤーにもすぐ届きます。' +
		'間違えた時は sbo_history で履歴を見て sbo_undo で戻せます。';

	var KIND_ENUM = ALL_KINDS;
	var WRITE_KIND_ENUM = Object.keys(KIND_FILE);
	var TOOLS = [
		{
			name: 'sbo_schema', write: false,
			description: '扱えるデータの種類（kind）と、各項目の名前・意味・範囲・件数を返す。最初に呼ぶ。',
			inputSchema: { type: 'object', properties: {} },
			run: function () { return opSchema(); }
		},
		{
			name: 'sbo_list', write: false,
			description: '1 種類のデータを一覧する。q で名前（szName）の一部で絞り込み、fields で返す項目を絞れる（多い時の節約に）。',
			inputSchema: {
				type: 'object', required: ['kind'],
				properties: {
					kind: { type: 'string', enum: KIND_ENUM },
					q: { type: 'string', description: '名前に含まれる文字' },
					offset: { type: 'integer', minimum: 0 },
					limit: { type: 'integer', minimum: 1, maximum: 500, description: '既定 100' },
					fields: { type: 'array', items: { type: 'string' }, description: '返す項目（id は常に返す）' },
					match: { type: 'object', description: '項目の値がちょうど同じものだけ（例: {"isPlayer": true}・{"dwMapID": 3}）' }
				}
			},
			run: opList
		},
		{
			name: 'sbo_get', write: false,
			description: '1 件のデータを全部の項目つきで返す。',
			inputSchema: {
				type: 'object', required: ['kind', 'id'],
				properties: { kind: { type: 'string', enum: KIND_ENUM }, id: { type: 'integer', minimum: 0 } }
			},
			run: opGet
		},
		{
			name: 'sbo_map_tiles', write: false,
			description: 'マップのマス目を 2 次元の配列で返す（parts=mapParts の番号・flags・roof=屋根レベル）。id は map の番号（dwID ではない）。' +
				'x・y・w・h で範囲を切り出せる（省くと全体、1 回最大 128×128）。',
			inputSchema: {
				type: 'object', required: ['id'],
				properties: {
					id: { type: 'integer', minimum: 0 },
					x: { type: 'integer', minimum: 0 }, y: { type: 'integer', minimum: 0 },
					w: { type: 'integer', minimum: 1, maximum: 128 }, h: { type: 'integer', minimum: 1, maximum: 128 }
				}
			},
			run: opTiles
		},
		{
			name: 'sbo_update', write: true,
			description: '既存のデータを変える（何件でもまとめて。履歴 1 件になる）。changes には変える項目だけを書く。' +
				'previewId 無しだと何も変えずに差分と previewId を返す。利用者の了解後、同じ引数に previewId を付けて呼ぶと反映。',
			inputSchema: {
				type: 'object', required: ['kind', 'updates'],
				properties: {
					kind: { type: 'string', enum: WRITE_KIND_ENUM },
					updates: {
						type: 'array', minItems: 1, maxItems: MAX_ITEMS,
						items: {
							type: 'object', required: ['id', 'changes'],
							properties: { id: { type: 'integer', minimum: 0 }, changes: { type: 'object' } }
						}
					},
					note: { type: 'string', description: '変更の理由（履歴に残る）' },
					previewId: { type: 'string', description: '下見で返った previewId（付けると反映する）' }
				}
			},
			run: opUpdate
		},
		{
			name: 'sbo_add', write: true,
			description: 'データを末尾に追加する（追加は元に戻せない）。省いた項目は 0 や空になる。2 段階は sbo_update と同じ。',
			inputSchema: {
				type: 'object', required: ['kind', 'records'],
				properties: {
					kind: { type: 'string', enum: WRITE_KIND_ENUM },
					records: { type: 'array', minItems: 1, maxItems: MAX_ITEMS, items: { type: 'object' } },
					note: { type: 'string' },
					previewId: { type: 'string' }
				}
			},
			run: opAdd
		},
		{
			name: 'sbo_history', write: false,
			description: '変更履歴（新しい順）。管理ページや AI が API で変えたものが残る。',
			inputSchema: {
				type: 'object',
				properties: { kind: { type: 'string', enum: WRITE_KIND_ENUM }, limit: { type: 'integer', minimum: 1, maximum: 100 } }
			},
			run: opHistory
		},
		{
			name: 'sbo_undo', write: true,
			description: '履歴 1 件ぶんの変更を元に戻す。その後に別の変更があったデータは force を付けない限り戻さない。2 段階は sbo_update と同じ。',
			inputSchema: {
				type: 'object', required: ['historyId'],
				properties: {
					historyId: { type: 'integer' },
					force: { type: 'boolean' },
					note: { type: 'string' },
					previewId: { type: 'string' }
				}
			},
			run: opUndo
		}
	];

	function mcpOne(msg, who) {
		var isNotify = !msg || msg.id === undefined || msg.id === null;
		function ok(result) {
			return isNotify ? null : { jsonrpc: '2.0', id: msg.id, result: result };
		}
		function err(code, message) {
			return isNotify ? null : { jsonrpc: '2.0', id: msg.id, error: { code: code, message: message } };
		}
		if (!msg || msg.jsonrpc !== '2.0' || typeof msg.method !== 'string') {
			return msg && msg.id != null ? err(-32600, 'Invalid Request') : null;
		}
		switch (msg.method) {
		case 'initialize':
			var v = msg.params && msg.params.protocolVersion;
			return ok({
				protocolVersion: MCP_VERSIONS.indexOf(v) >= 0 ? v : MCP_VERSIONS[0],
				capabilities: { tools: { listChanged: false } },
				serverInfo: { name: 'sbo2003', title: 'SBO2003 設定データ', version: '1.0.0' },
				instructions: MCP_INSTRUCTIONS + (who.write ? '' : '（このキーは読むだけです）')
			});
		case 'ping':
			return ok({});
		case 'tools/list':
			return ok({
				tools: TOOLS.filter(function (t) { return who.write || !t.write; }).map(function (t) {
					return {
						name: t.name, description: t.description, inputSchema: t.inputSchema,
						annotations: { readOnlyHint: !t.write, destructiveHint: t.write, idempotentHint: !t.write, openWorldHint: false }
					};
				})
			});
		case 'tools/call':
			var p = msg.params || {};
			var tool = null;
			TOOLS.forEach(function (t) {
				if (t.name === p.name) {
					tool = t;
				}
			});
			if (!tool || (tool.write && !who.write)) {
				return err(-32602, '知らないツールです: ' + p.name);
			}
			try {
				var result = tool.run(p.arguments || {}, who);
				return ok({ content: [{ type: 'text', text: JSON.stringify(result) }] });
			} catch (e) {
				if (e instanceof ApiError) {
					var body = { error: e.code, message: e.message };
					if (e.extra) {
						Object.keys(e.extra).forEach(function (k) { body[k] = e.extra[k]; });
					}
					return ok({ content: [{ type: 'text', text: JSON.stringify(body) }], isError: true });
				}
				console.error('[data] MCP ' + p.name + ' で失敗: ' + (e && e.stack || e));
				return ok({ content: [{ type: 'text', text: 'サーバー内部のエラーです' }], isError: true });
			}
		default:
			if (msg.method.indexOf('notifications/') === 0) {
				return null;
			}
			return err(-32601, 'Method not found: ' + msg.method);
		}
	}

	function handleMcp(req, res) {
		/* ブラウザから別のサイト経由で呼ばれないよう、Origin があれば同じホストだけ */
		var origin = req.headers['origin'];
		if (origin) {
			var host = String(req.headers['x-forwarded-host'] || req.headers['host'] || '');
			var oHost = '';
			try {
				oHost = new URL(origin).host;
			} catch (e) {
				oHost = '';
			}
			if (!host || oHost !== host.split(',')[0].trim()) {
				replyJson(res, 403, { error: 'origin' });
				return;
			}
		}
		if (req.method !== 'POST') {
			res.writeHead(405, { 'Allow': 'POST', 'Content-Type': 'text/plain; charset=utf-8' });
			res.end('POST だけ受け付けます');
			return;
		}
		var who = authKey(req);
		if (!who) {
			res.writeHead(401, {
				'Content-Type': 'application/json; charset=utf-8',
				'WWW-Authenticate': 'Bearer realm="sbo2003"',
				'Cache-Control': 'no-store'
			});
			res.end(JSON.stringify({ jsonrpc: '2.0', id: null, error: { code: -32001, message: 'AI用キーが無いか、使えません（管理ページで発行してください）' } }));
			return;
		}
		if (!rateOk(who.rateKey)) {
			replyJson(res, 429, { jsonrpc: '2.0', id: null, error: { code: -32002, message: '呼び出しが多すぎます。少し待ってください' } });
			return;
		}
		readBody(req, function (body) {
			if (body === undefined) {
				replyJson(res, 400, { jsonrpc: '2.0', id: null, error: { code: -32700, message: 'Parse error' } });
				return;
			}
			var aOut;
			if (Array.isArray(body)) {
				aOut = body.map(function (m) { return mcpOne(m, who); }).filter(Boolean);
			} else {
				aOut = mcpOne(body, who);
			}
			if (!aOut || (Array.isArray(aOut) && aOut.length === 0)) {
				res.writeHead(202);
				res.end();
				return;
			}
			replyJson(res, 200, aOut);
		});
	}

	/* ------------------------------------------------------------------ */
	/* HTTP																		 */
	/* ------------------------------------------------------------------ */
	function replyJson(res, status, obj) {
		res.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
		res.end(JSON.stringify(obj));
	}

	/* JSON を読む（読めなければ undefined） */
	function readBody(req, cb) {
		var aBuf = [], nSize = 0, bOver = false;
		req.on('data', function (buf) {
			nSize += buf.length;
			if (nSize > MAX_BODY) {
				bOver = true;
				return;
			}
			aBuf.push(buf);
		});
		req.on('end', function () {
			if (bOver) {
				cb(undefined);
				return;
			}
			try {
				cb(JSON.parse(Buffer.concat(aBuf).toString('utf8') || '{}'));
			} catch (e) {
				cb(undefined);
			}
		});
	}

	var API_DATA = {
		'/api/data/schema': opSchema,
		'/api/data/list': opList,
		'/api/data/get': opGet,
		'/api/data/tiles': opTiles,
		'/api/data/update': opUpdate,
		'/api/data/add': opAdd,
		'/api/data/history': opHistory,
		'/api/data/undo': opUndo
	};
	var API_ADMIN = {
		'/api/admin/ai/keys': apiAdminKeys,
		'/api/admin/ai/issue': apiAdminIssue,
		'/api/admin/ai/revoke': apiAdminRevoke,
		'/api/admin/ai/history': function (body) { adminWho(body); return opHistory(body); },
		'/api/admin/ai/undo': function (body) { return opUndo(body, adminWho(body)); },
		/* マップ資料の書き出し（管理ページの「マップ資料」）。読むだけ */
		'/api/admin/data/list': function (body) { adminWho(body); return opList(body); },
		'/api/admin/data/tiles': function (body) { adminWho(body); return opTiles(body); }
	};

	function run(fn, body, req, res, who) {
		try {
			replyJson(res, 200, fn(body, who || req, req));
		} catch (e) {
			if (e instanceof ApiError) {
				var out = { error: e.code, message: e.message };
				if (e.extra) {
					Object.keys(e.extra).forEach(function (k) { out[k] = e.extra[k]; });
				}
				replyJson(res, e.status, out);
				return;
			}
			console.error('[data] ' + req.url + ' で失敗: ' + (e && e.stack || e));
			replyJson(res, 500, { error: 'internal' });
		}
	}

	/* sbosvr_net.js の HTTP から呼ばれる。受け持つ URL なら true */
	function handleHttp(req, res, url) {
		var bMcp = (url === '/mcp');
		var fnData = API_DATA[url], fnAdmin = API_ADMIN[url];
		if (!bMcp && !fnData && !fnAdmin) {
			return false;
		}
		if (!ENABLED) {
			replyJson(res, 404, { error: 'disabled', message: 'AI 用の API は止めてあります（SBO_AI_API=off）' });
			return true;
		}
		if (bMcp) {
			handleMcp(req, res);
			return true;
		}
		if (req.method !== 'POST') {
			replyJson(res, 405, { error: 'method' });
			return true;
		}
		var who = null;
		if (fnData) {
			who = authKey(req);
			if (!who) {
				res.writeHead(401, { 'Content-Type': 'application/json; charset=utf-8', 'WWW-Authenticate': 'Bearer realm="sbo2003"' });
				res.end(JSON.stringify({ error: 'key', message: 'AI用キーが無いか、使えません' }));
				return true;
			}
			if (!rateOk(who.rateKey)) {
				replyJson(res, 429, { error: 'too_many' });
				return true;
			}
		}
		readBody(req, function (body) {
			if (!body || typeof body !== 'object' || Array.isArray(body)) {
				replyJson(res, 400, { error: 'json' });
				return;
			}
			if (fnAdmin && acc()) {
				acc().withAdminCookie(body, req);		/* 管理ページのログインは Cookie */
			}
			if (fnData) {
				run(function (b) { return fnData(b, who); }, body, req, res);
			} else {
				run(function (b) { return fnAdmin(b, req); }, body, req, res);
			}
		});
		return true;
	}

	return {
		handleHttp: handleHttp
	};
})();
