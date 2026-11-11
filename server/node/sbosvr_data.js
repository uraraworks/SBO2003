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
 *   /api/data/bookmark  {name, note?}                        → しおりを付ける（お試しの区切り）
 *   /api/data/bookmarks {limit?}                             → {bookmarks:[...]}
 *   /api/data/rollback  {bookmarkId, note?, force?, previewId?} → しおりの後の変更をまとめて元に戻す
 *     書き込みは previewId 無し → {preview:true, previewId, items:[{id, before, after, diff}]}
 *                previewId 有り → {ok:true, historyId, items:[...]}
 * 管理ページ用（{session} は /api/admin/login で得たもの）
 *   /api/admin/ai/keys     {session}                          → {keys:[...]}
 *   /api/admin/ai/issue    {session, name, write, days}       → {key, info}
 *   /api/admin/ai/revoke   {session, id}                      → {ok}
 *   /api/admin/ai/history  {session, kind?, limit?}           → {entries:[...]}
 *   /api/admin/ai/undo     {session, historyId, force?, previewId?} → 上の undo と同じ
 *   /api/admin/ai/bookmark・bookmarks・rollback {session, ...} → 上のしおりと同じ
 *   /api/admin/data/list   {session, kind, offset?, limit?}   → 上の list と同じ（マップ資料の書き出し用。読むだけ）
 *   /api/admin/data/tiles  {session, id, x?, y?, w?, h?}      → 上の tiles と同じ
 *   /api/admin/mapedit/apply {session, id, cells:[[x, y, 前, 後]], note?, force?} → マップ確認の編集を反映（履歴 1 件）
 *   /api/admin/mapedit/draft/get・list・save {session, id?, cells?} → 編集の下書き（SboWebMapDrafts.json）
 *   /api/admin/mapedit/snap/list・save・get・delete {session, id, name?, cells?, snapId?} → 名前を付けたスナップショット（SboWebMapSnapshots.json）
 *   /api/admin/mapedit/resize {session, id, top?, bottom?, left?, right?, fill?, check?} → マップを広げる（履歴に残らない）
 *   /api/admin/mapedit/add    {session, cx, cy, fill?, szName?} → マップを足す
 *   （AI 用キーでも /api/data/map/resize・/api/data/map/add で同じことができる。書き込みもできるキーだけ）
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
	var MAX_BOOKMARKS = 100;		/* 覚えておくしおりの数 */
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
		item: 'SboDataItemInfo.dat',
		/* 作り手とレシピ（ファイルが無い時は 0 件。初めて保存した時に作られる） */
		maker: 'SboDataMakerInfo.dat',
		recipe: 'SboDataRecipeInfo.dat',
		/* ギルドの仕事依頼（ファイルが無い時は 0 件。初めて保存した時に作られる） */
		quest: 'SboDataQuestInfo.dat',
		/* NPC（プレイヤーは書き換えられない。ファイルはプレイヤーと一緒） */
		char: 'SboDataCharInfo.dat',
		/* マップは 1 マップ 1 ファイル（SboDataMapInfo000.dat〜）。マス目（tiles）も同じファイル */
		map: 'SboDataMapInfo*.dat',
		mapParts: 'SboDataMapPartsInfo.dat',
		efc: 'SboDataEfcInfo.dat',
		comEfc: 'SboDataComEfcInfo.dat',
		subGame: 'SboDataSubGameInfo.dat'
	};
	/* 読むだけの種類（プレイヤーの預かり所。アカウント名・パスワード・IP などは C 側で出さない） */
	var READONLY_KINDS = ['leave'];
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
		if (kind === 'subGame' && out.detail) {
			out.detail = withoutSubGameVolatile(out.detail);
		}
		return out;
	}

	/* 迷路のゴールの位置（作るたびに変わる・読むだけ）を除いた detail */
	function withoutSubGameVolatile(detail) {
		var o = {};
		Object.keys(detail).forEach(function (k) {
			if (k !== 'byGoalFromX' && k !== 'byGoalFromY') {
				o[k] = detail[k];
			}
		});
		return o;
	}

	/* その日初めてそのデータを変える前に、データファイルの控えを取る */
	function backupFile(kind) {
		var pat = KIND_FILE[kind === 'tiles' ? 'map' : kind];
		var d = new Date();
		var day = d.getFullYear() + ('0' + (d.getMonth() + 1)).slice(-2) + ('0' + d.getDate()).slice(-2);
		var dir = path.join(dataDir(), 'AiBackup', day);
		var aName = [pat];
		if (pat.indexOf('*') >= 0) {
			var re = new RegExp('^' + pat.replace('.', '\\.').replace('*', '\\d+') + '$');
			try {
				aName = fs.readdirSync(dataDir()).filter(function (f) { return re.test(f); });
			} catch (e) {
				aName = [];
			}
		}
		aName.forEach(function (name) {
			var dst = path.join(dir, name);
			try {
				/* 元のファイルがまだ無い時（作り手・レシピの初めての保存）は控えが要らない */
				if (fs.existsSync(dst) || !fs.existsSync(path.join(dataDir(), name))) {
					return;
				}
				fs.mkdirSync(dir, { recursive: true });
				fs.copyFileSync(path.join(dataDir(), name), dst);
			} catch (e) {
				console.error('[data] 控えを取れません: ' + name + ' ' + e.message);
			}
		});
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
			if (!sameRec(kind, now, befores[id])) {
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
			/* 足す時は今の件数を番号にする（NPC は敵の発生などで件数が動くので、下見の時の番号は使わない） */
			var id = op.added ? count(kind) : op.id;
			var before = op.added ? null : getRecord(kind, id);
			var r = applyC(kind, id, op.changes, true, who.label);
			if (!r.ok) {
				revertDone(kind, aDone, who);
				fail(400, 'apply', kind + ' の ' + id + ' 番を反映できませんでした: ' + r.error +
						(aDone.length ? '（それまでの変更は元に戻しました）' : ''));
			}
			aDone.push({ id: r.id, added: r.added || undefined, before: before, after: r.record });
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
			if (!before) {
				fail(404, 'not_found', kind + ' の ' + id + ' 番は空き（または消えた NPC）です');
			}
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
		if (info && info.maxCount > 0 && total + a.records.length > info.maxCount) {
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
			pv.message += eraseChanges(kind) ? '（追加したものは元に戻すと消えます）' : '（追加は元に戻せません）';
			return pv;
		}
		takePreview(a.previewId, 'add', payload, who);
		/* NPC は敵の発生などで件数が動くので見ない（番号は反映する時に決まる） */
		if (kind !== 'char' && count(kind) !== total) {
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
			undoOf: e.undoOf || undefined, undoneBy: e.undoneBy || undefined, bookmarkId: e.bookmarkId || undefined,
			items: e.items.map(function (d) {
				if (e.kind === 'tiles') {
					/* マス目は範囲と変わったマスの数だけ（中身は sbo_map_tiles で） */
					return { id: d.id, name: d.name, region: [d.x, d.y, d.w, d.h], changed: d.changed };
				}
				var r = d.after || d.before || {};
				return { id: d.id, npcId: r.npcId || undefined, name: r.szName, added: d.added || undefined,
					deleted: (d.before && !d.after) || undefined, diff: diff(d.before, d.after) };
			})
		};
	}

	function opHistory(a) {
		var h = history();
		var limit = Math.min(100, Math.max(1, Math.floor(Number(a.limit) || 20)));
		var kind = a.kind ? (a.kind === 'tiles' ? 'tiles' : checkKind(a.kind)) : null;
		var aOut = [];
		for (var i = h.entries.length - 1; i >= 0 && aOut.length < limit; i--) {
			if (!kind || h.entries[i].kind === kind) {
				aOut.push(historySummary(h.entries[i]));
			}
		}
		return { entries: aOut };
	}

	/* ------------------------------------------------------------------ */
	/* 元に戻す（1 件ずつ・しおりまでまとめて）で使う下請け						 */
	/* ------------------------------------------------------------------ */
	/* 遊んでいる間に勝手に変わる項目（「後から変わったか」の判定では見ない） */
	var VOLATILE = {
		char: ['id', 'dwX', 'dwY', 'byDirection', 'wHP', 'wMana', 'online', 'byCharState', 'wAT', 'wDF', 'dwExp', 'dwGold', 'dwPlayTime', 'dwLoginCount'],
		map: ['items']
	};
	function stable(kind, rec) {
		if (kind === 'subGame' && rec && rec.detail) {
			var r2 = {};
			Object.keys(rec).forEach(function (k) {
				r2[k] = (k === 'detail') ? withoutSubGameVolatile(rec.detail) : rec[k];
			});
			return r2;
		}
		var v = VOLATILE[kind];
		if (!rec || !v) {
			return rec;
		}
		var o = {};
		Object.keys(rec).forEach(function (k) {
			if (v.indexOf(k) < 0) {
				o[k] = rec[k];
			}
		});
		return o;
	}
	function sameRec(kind, a, b) {
		return same(stable(kind, a || null), stable(kind, b || null));
	}

	/* 追加した記録を「消す」変更。消せない種類は null */
	function eraseChanges(kind) {
		if (kind === 'maker' || kind === 'recipe' || kind === 'quest') {
			return { byState: 0 };		/* 依頼は削除が無いので、追加した依頼は「止める」（0）にする */
		}

		if (kind === 'char') {
			return { deleted: true };
		}
		if (kind === 'subGame') {
			return { byType: 0 };		/* 追加した迷路は止める（枠は「無し」で残る） */
		}
		return null;
	}

	/* 記録の見分け方。NPC は npcId（再起動しても変わらない）、それ以外は番号 */
	function recKey(kind, d) {
		var r = d.before || d.after;
		if (kind === 'char' && r && r.npcId) {
			return kind + ':n' + r.npcId;
		}
		return kind + ':' + d.id;
	}

	/* その記録の今の番号（NPC は番号が振り直されていても npcId で探す）。いなければ -1 */
	function currentId(kind, d) {
		var r = d.after || d.before;
		if (kind === 'char' && r && r.npcId) {
			return callC(_SboWebCharFindNpc, [r.npcId]);
		}
		return d.id;
	}

	/*
	 * 記録を target の姿（null なら「無い」）へ戻す操作を作る。
	 * 返り値: null（何もしなくてよい）/ {skip: 理由} / {id, changes, added, preview}
	 */
	function restoreOp(kind, curId, now, target) {
		var changes, id = curId, added = false;
		if (!target) {
			if (!now) {
				return null;
			}
			changes = eraseChanges(kind);
			if (!changes) {
				return { skip: '追加した ' + kind + ' は消せないので残ります' };
			}
		} else if (!now) {
			if (kind !== 'char') {
				return { skip: kind + ' の ' + curId + ' 番が見つかりません' };
			}
			/* 消した NPC を足し直す（同じ NPC番号で） */
			changes = restorable(kind, target);
			changes.npcId = target.npcId;
			id = count(kind);
			added = true;
		} else {
			changes = restorable(kind, target);
		}
		var c = applyC(kind, id, changes, false);
		if (!c.ok) {
			fail(400, 'invalid', kind + ' の ' + curId + ' 番を戻せません: ' + c.error);
		}
		if (now && sameRec(kind, now, c.record)) {
			return null;		/* もうその姿（消した追加も含む） */
		}
		return { id: id, changes: changes, added: added, preview: c.record };
	}

	/* マップのマス目（範囲）を読む */
	function readTiles(mapNo, x, y, w, h) {
		var r = JSON.parse(UTF8ToString(_SboWebDataMapTiles(mapNo, x, y, w, h)));
		if (r.error) {
			fail(404, 'not_found', r.error);
		}
		return r.parts;
	}
	function setTiles(mapNo, x, y, parts, bApply) {
		var h = parts.length, w = h ? parts[0].length : 0;
		var csv = parts.map(function (row) { return row.join(','); }).join(',');
		return JSON.parse(UTF8ToString(callC(_SboWebDataTilesSet, [mapNo, x, y, w, h, csv, bApply ? 1 : 0])));
	}

	/* 種類ごとの操作をまとめて反映する。途中の種類で失敗したら、それまでの種類も元へ戻す */
	function applyKinds(aKindOps, who) {
		var aDoneByKind = [];
		for (var i = 0; i < aKindOps.length; i++) {
			try {
				aDoneByKind.push({ kind: aKindOps[i].kind, done: applyAll(aKindOps[i].kind, aKindOps[i].ops, who) });
			} catch (e) {
				for (var j = aDoneByKind.length - 1; j >= 0; j--) {
					revertDone(aDoneByKind[j].kind, aDoneByKind[j].done, who);
				}
				throw e;
			}
		}
		return aDoneByKind;
	}

	/* 反映したものを元へ戻す（失敗した時の後始末） */
	function revertDone(kind, aDone, who) {
		aDone.slice().reverse().forEach(function (d) {
			if (d.added) {
				var erase = eraseChanges(kind);
				if (erase) {
					applyC(kind, d.id, erase, true, who.label);
				}
			} else if (d.before && d.after) {
				applyC(kind, d.id, restorable(kind, d.before), true, who.label);
			} else if (d.before && kind === 'char') {
				var ch = restorable(kind, d.before);
				ch.npcId = d.before.npcId;
				applyC(kind, count(kind), ch, true, who.label);
			}
		});
	}

	/* 下見の後に、対象のデータが変わっていないか（befores は {kind: {番号: 姿}}） */
	function checkUnchangedAll(befores) {
		Object.keys(befores).forEach(function (kind) {
			Object.keys(befores[kind]).forEach(function (id) {
				var now = getRecord(kind, Number(id));
				if (!sameRec(kind, now, befores[kind][id])) {
					fail(409, 'changed', '下見の後に ' + kind + ' の ' + id + ' 番が変わりました。もう一度下見してください。');
				}
			});
		});
	}

	/*
	 * 履歴（古い順）を「その前の姿」へ戻す計画を立てる。
	 * 記録ごとに、最初の変更の前の姿へ戻す。マス目は新しい順に 1 つずつ戻す。
	 */
	function planRestore(aEntry, force) {
		var aRec = {}, aOrder = [], aTileEntry = [];
		aEntry.forEach(function (e) {
			if (e.kind === 'tiles') {
				aTileEntry.push(e);
				return;
			}
			e.items.forEach(function (d) {
				var key = recKey(e.kind, d);
				if (!aRec[key]) {
					aRec[key] = { kind: e.kind, d: d, before: d.before };
					aOrder.push(key);
				}
				aRec[key].last = d.after;
				aRec[key].lastD = d;
			});
		});
		var plan = { befores: {}, items: [], kindOps: {}, kindOrder: [], skip: [], conflicts: [], tiles: [] };
		aOrder.forEach(function (key) {
			var r = aRec[key];
			if (r.before && r.last && sameRec(r.kind, r.before, r.last)) {
				return;		/* 変えて、もう戻してある */
			}
			var curId = currentId(r.kind, r.lastD);
			var now = curId >= 0 ? getRecord(r.kind, curId) : null;
			if (!sameRec(r.kind, now, r.last)) {
				plan.conflicts.push({ kind: r.kind, id: curId >= 0 ? curId : r.d.id });
				if (!force) {
					return;
				}
			}
			var op = restoreOp(r.kind, curId, now, r.before);
			if (!op) {
				return;
			}
			if (op.skip) {
				plan.skip.push({ kind: r.kind, id: r.d.id, reason: op.skip });
				return;
			}
			if (!op.added) {
				plan.befores[r.kind] = plan.befores[r.kind] || {};
				plan.befores[r.kind][op.id] = now;
			}
			var name = (now && now.szName) || (r.before && r.before.szName) || undefined;
			plan.items.push({ kind: r.kind, id: op.added ? null : op.id, name: name, added: !r.before || undefined,
				restored: op.added || undefined, diff: diff(now, op.preview) });
			if (!plan.kindOps[r.kind]) {
				plan.kindOps[r.kind] = [];
				plan.kindOrder.push(r.kind);
			}
			plan.kindOps[r.kind].push({ id: op.id, changes: op.changes, added: op.added });
		});
		/* マス目: 新しい順に、今が「変更後」なら「変更前」へ（重なりは手前の結果を使って見る） */
		var overlay = {};
		aTileEntry.slice().reverse().forEach(function (e) {
			e.items.forEach(function (t) {
				var cur = readTiles(t.id, t.x, t.y, t.w, t.h);
				var bSame = true;
				for (var yy = 0; yy < t.h; yy++) {
					for (var xx = 0; xx < t.w; xx++) {
						var k = t.id + ':' + (t.x + xx) + ',' + (t.y + yy);
						var v = (k in overlay) ? overlay[k] : cur[yy][xx];
						if (v !== t.after[yy][xx]) {
							bSame = false;
						}
					}
				}
				if (!bSame) {
					plan.conflicts.push({ kind: 'tiles', id: t.id, region: [t.x, t.y, t.w, t.h] });
					if (!force) {
						return;
					}
				}
				for (var y2 = 0; y2 < t.h; y2++) {
					for (var x2 = 0; x2 < t.w; x2++) {
						overlay[t.id + ':' + (t.x + x2) + ',' + (t.y + y2)] = t.before[y2][x2];
					}
				}
				plan.tiles.push(t);
				plan.items.push({ kind: 'tiles', id: t.id, name: t.name, region: [t.x, t.y, t.w, t.h], changed: t.changed });
			});
		});
		return plan;
	}

	/* 計画を反映して履歴に残す。返り値は履歴の番号の配列 */
	function runPlan(plan, who, histBase) {
		var aDoneByKind = applyKinds(plan.kindOrder.map(function (k) { return { kind: k, ops: plan.kindOps[k] }; }), who);
		var aHist = aDoneByKind.map(function (dk) {
			var e = {};
			Object.keys(histBase).forEach(function (k) { e[k] = histBase[k]; });
			e.kind = dk.kind;
			e.items = dk.done;
			return addHistory(e).historyId;
		});
		if (plan.tiles.length) {
			backupFile('tiles');
			var aTileDone = [];
			plan.tiles.forEach(function (t) {
				var r = setTiles(t.id, t.x, t.y, t.before, true);
				if (r.ok) {
					aTileDone.push({ id: t.id, name: t.name, x: t.x, y: t.y, w: t.w, h: t.h, before: r.before, after: r.after, changed: r.changed });
				}
			});
			if (aTileDone.length) {
				var e2 = {};
				Object.keys(histBase).forEach(function (k) { e2[k] = histBase[k]; });
				e2.kind = 'tiles';
				e2.items = aTileDone;
				aHist.push(addHistory(e2).historyId);
			}
		}
		return { historyIds: aHist, done: aDoneByKind };
	}

	function planItemsText(a) {
		return a.map(function (x) { return x.kind + ' の ' + x.id + ' 番'; }).join('、');
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
		var plan = planRestore([e], !!a.force);
		if (plan.conflicts.length && !a.force) {
			fail(409, 'conflict', 'この後に変更されたデータがあるので戻しません（' + planItemsText(plan.conflicts) +
					'）。それでも戻すなら force を付けてください（その後の変更も消えます）。', { conflicts: plan.conflicts.map(function (x) { return x.id; }) });
		}
		if (!plan.items.length) {
			fail(400, 'nothing', '戻せるものがありません' + (plan.skip.length ? '（' + plan.skip[0].reason + '）' : ''));
		}
		var payload = { historyId: hid, force: !!a.force, note: a.note || '' };
		if (!a.previewId) {
			var pv = makePreview('undo', payload, who, plan.befores, plan.items);
			if (plan.skip.length) {
				pv.skipped = plan.skip.map(function (x) { return x.id; });
				pv.message += '（' + plan.skip.map(function (x) { return x.reason; }).join('、') + '）';
			}
			return pv;
		}
		var p = takePreview(a.previewId, 'undo', payload, who);
		checkUnchangedAll(p.befores);
		var r = runPlan(plan, who, { op: 'undo', by: who.label, undoOf: hid, note: String(a.note || '').slice(0, 500) });
		e.undoneBy = r.historyIds[0];
		writeJsonFile(strHistPath, h);
		log(who.label + ' が履歴 ' + hid + ' を元に戻した（履歴 ' + r.historyIds.join(', ') + '）');
		return { ok: true, historyId: r.historyIds[0], historyIds: r.historyIds, skipped: plan.skip.length ? plan.skip.map(function (x) { return x.id; }) : undefined,
			items: plan.items };
	}

	/*
	 * しおり（お試しの区切り）。付けた時の履歴の番号を覚えておき、
	 * 「このしおりまで戻す」でそれより後の変更をまとめて元に戻す（下見 → 確定）。
	 */
	function bookmarks() {
		var h = history();
		h.bmSeq = h.bmSeq || 0;
		h.bookmarks = h.bookmarks || [];
		return h;
	}

	function findBookmark(id) {
		var h = bookmarks();
		var bid = Number(id);
		for (var i = 0; i < h.bookmarks.length; i++) {
			if (h.bookmarks[i].bookmarkId === bid) {
				return h.bookmarks[i];
			}
		}
		fail(404, 'not_found', 'しおり ' + id + ' が見つかりません');
	}

	/* しおりの後の履歴（古い順）。古い履歴が消えていて戻せない時は null */
	function entriesAfter(bm) {
		var h = history();
		var a = h.entries.filter(function (e) { return e.historyId > bm.afterHistoryId; });
		var bLost = (bm.afterHistoryId < h.seq) && (!a.length || a[0].historyId > bm.afterHistoryId + 1);
		return bLost ? null : a;
	}

	/* しおりの後の変更のうち、まだ戻していないものの数（戻した操作そのものは数えない） */
	function countLive(a, bm) {
		return a.filter(function (e) {
			if (e.undoneBy || e.op === 'rollback') {
				return false;
			}
			return !(e.op === 'undo' && e.undoOf > bm.afterHistoryId);
		}).length;
	}

	function bookmarkPublic(bm) {
		var a = entriesAfter(bm);
		return {
			bookmarkId: bm.bookmarkId, name: bm.name, note: bm.note || undefined, at: new Date(bm.at).toISOString(), by: bm.by,
			afterHistoryId: bm.afterHistoryId, changes: a ? countLive(a, bm) : undefined, lost: a ? undefined : true,
			rolledBack: bm.rolledBack ? bm.rolledBack.map(function (r) { return { at: new Date(r.at).toISOString(), by: r.by, historyIds: r.historyIds }; }) : undefined
		};
	}

	function opBookmarkAdd(a, who) {
		requireWrite(who);
		var name = String(a.name || '').trim().slice(0, 60);
		if (!name) {
			fail(400, 'name', 'しおりの名前（name）を付けてください。例: 「レジェンタのお試し1」');
		}
		var h = bookmarks();
		h.bmSeq++;
		var bm = { bookmarkId: h.bmSeq, name: name, note: String(a.note || '').slice(0, 500), at: Date.now(), by: who.label, afterHistoryId: h.seq };
		h.bookmarks.push(bm);
		if (h.bookmarks.length > MAX_BOOKMARKS) {
			h.bookmarks.splice(0, h.bookmarks.length - MAX_BOOKMARKS);
		}
		writeJsonFile(strHistPath, h);
		log(who.label + ' がしおり ' + bm.bookmarkId + '「' + name + '」を付けた（履歴 ' + bm.afterHistoryId + ' の後）');
		return { ok: true, bookmark: bookmarkPublic(bm) };
	}

	function opBookmarks(a) {
		var h = bookmarks();
		var limit = Math.min(100, Math.max(1, Math.floor(Number(a.limit) || 20)));
		return { bookmarks: h.bookmarks.slice().reverse().slice(0, limit).map(bookmarkPublic) };
	}

	function opRollback(a, who) {
		requireWrite(who);
		var bm = findBookmark(a.bookmarkId);
		var aEntry = entriesAfter(bm);
		if (!aEntry) {
			fail(400, 'lost', 'しおりの後の古い履歴が消えている（' + MAX_HISTORY + ' 件より前）ので、まとめて戻せません');
		}
		var plan = planRestore(aEntry, !!a.force);
		if (plan.conflicts.length && !a.force) {
			fail(409, 'conflict', 'しおりの後に、AI 以外（管理者ウィンドウなど）からも変わったデータがあるので戻しません（' +
					planItemsText(plan.conflicts) + '）。それでも戻すなら force を付けてください（その変更も消えます）。', { conflicts: plan.conflicts });
		}
		if (!plan.items.length) {
			fail(400, 'nothing', 'しおりの後に戻す変更はありません' + (plan.skip.length ? '（' + plan.skip[0].reason + '）' : ''));
		}
		var payload = { bookmarkId: bm.bookmarkId, force: !!a.force, note: a.note || '' };
		if (!a.previewId) {
			var pv = makePreview('rollback', payload, who, plan.befores, plan.items);
			pv.bookmark = bookmarkPublic(bm);
			if (plan.skip.length) {
				pv.skipped = plan.skip;
				pv.message += '（' + plan.skip.map(function (x) { return x.reason; }).join('、') + '）';
			}
			return pv;
		}
		var p = takePreview(a.previewId, 'rollback', payload, who);
		checkUnchangedAll(p.befores);
		var note = String(a.note || '').slice(0, 500) || ('しおり「' + bm.name + '」まで戻す');
		var r = runPlan(plan, who, { op: 'rollback', by: who.label, bookmarkId: bm.bookmarkId, note: note });
		var h = history();
		aEntry.forEach(function (e) {
			if (!e.undoneBy) {
				e.undoneBy = r.historyIds[0];
			}
		});
		bm.rolledBack = bm.rolledBack || [];
		bm.rolledBack.push({ at: Date.now(), by: who.label, historyIds: r.historyIds });
		writeJsonFile(strHistPath, h);
		log(who.label + ' がしおり ' + bm.bookmarkId + '「' + bm.name + '」まで戻した（履歴 ' + r.historyIds.join(', ') + '）');
		return { ok: true, historyIds: r.historyIds, skipped: plan.skip.length ? plan.skip : undefined, items: plan.items };
	}

	/*
	 * マップのマス目を書き換える（{id, x, y, parts:[[...]], note?, previewId?}。下見 → 確定）。
	 * 履歴には範囲の前と後を残し、元に戻す・しおりで戻せる。
	 */
	function opTilesSet(a, who) {
		requireWrite(who);
		var id = checkId(a.id);
		var x = Math.floor(Number(a.x) || 0), y = Math.floor(Number(a.y) || 0);
		if (!Array.isArray(a.parts) || !a.parts.length || !Array.isArray(a.parts[0]) || !a.parts[0].length ||
				!a.parts.every(function (row) { return Array.isArray(row) && row.length === a.parts[0].length; })) {
			fail(400, 'parts', 'parts は [[パーツ番号,...],...]（行ごと、どの行も同じ長さ）で指定してください');
		}
		var r = setTiles(id, x, y, a.parts, false);
		if (!r.ok) {
			fail(400, 'invalid', r.error);
		}
		var map = getRecord('map', id);
		var name = map ? map.szName : '';
		var w = a.parts[0].length, h = a.parts.length;
		var befores = { tiles: {} };
		var item = { kind: 'tiles', id: id, name: name, region: [x, y, w, h], changed: r.changed };
		var payload = { id: id, x: x, y: y, parts: a.parts, note: a.note || '' };
		if (!a.previewId) {
			if (!r.changed) {
				fail(400, 'nothing', 'マス目は今と同じです（変わるマスがありません）');
			}
			var pv = makePreview('tiles', payload, who, {}, [item]);
			pv.tilesBefore = r.before;
			aPreview[pv.previewId].tilesBefore = r.before;
			return pv;
		}
		var p = takePreview(a.previewId, 'tiles', payload, who);
		if (!same(p.tilesBefore, r.before)) {
			fail(409, 'changed', '下見の後にマス目が変わりました。もう一度下見してください。');
		}
		backupFile('tiles');
		var done = setTiles(id, x, y, a.parts, true);
		if (!done.ok) {
			fail(400, 'apply', done.error);
		}
		var e = addHistory({ op: 'tiles', kind: 'tiles', by: who.label, note: String(a.note || '').slice(0, 500),
			items: [{ id: id, name: name, x: x, y: y, w: w, h: h, before: done.before, after: done.after, changed: done.changed }] });
		log(who.label + ' がマップ ' + id + ' のマス目を ' + done.changed + ' マス変更（履歴 ' + e.historyId + '）');
		return { ok: true, historyId: e.historyId, changed: done.changed };
	}

	/* ------------------------------------------------------------------ */
	/* 管理ページの「マップ確認」の編集モード										 */
	/* ------------------------------------------------------------------ */
	var MAPEDIT_BLOCK = 32;			/* 変わったマスを、この大きさの四角ごとにまとめて書き換える */
	var MAPEDIT_MAX_CELLS = 10000;	/* 1 回で送れるマスの数（JSON の大きさの上限 256KB に収まるよう） */
	var draftStore = null, strDraftPath = '';

	/* cells: [[x, y, 前のパーツ, 新しいパーツ], ...] を確かめて返す */
	function checkEditCells(id, cells) {
		if (!Array.isArray(cells) || !cells.length || cells.length > MAPEDIT_MAX_CELLS) {
			fail(400, 'cells', 'cells は [[x, y, 前, 後], ...] の配列で、1〜' + MAPEDIT_MAX_CELLS + ' 個にしてください');
		}
		var size = JSON.parse(UTF8ToString(_SboWebDataMapTiles(id, 0, 0, 1, 1)));
		if (size.error) {
			fail(404, 'not_found', size.error);
		}
		var nParts = count('mapParts');
		var seen = {};
		cells.forEach(function (c) {
			if (!Array.isArray(c) || c.length !== 4 || !c.every(function (v) { return typeof v === 'number' && Math.floor(v) === v; })) {
				fail(400, 'cells', 'cells の 1 個は [x, y, 前のパーツ, 新しいパーツ]（整数）です');
			}
			if (c[0] < 0 || c[1] < 0 || c[0] >= size.cx || c[1] >= size.cy) {
				fail(400, 'cells', '(' + c[0] + ', ' + c[1] + ') はマップの外です（' + size.cx + '×' + size.cy + '）');
			}
			if (c[3] < 0 || c[3] >= nParts) {
				fail(400, 'cells', 'パーツ番号 ' + c[3] + ' はありません（0〜' + (nParts - 1) + '）');
			}
			var k = c[0] + ',' + c[1];
			if (seen[k]) {
				fail(400, 'cells', '(' + k + ') が 2 回あります');
			}
			seen[k] = true;
		});
		return size;
	}

	/*
	 * 編集したマスをまとめて反映する（{session, id, cells:[[x, y, 前, 後], ...], note?, force?}）。
	 * 管理ページで確かめてから押すので下見は無し。「前」が今のサーバーと違うマス（ほかで変わった）があれば
	 * 何も変えずに conflicts を返す（force:true なら構わず書く）。
	 * 変わったマスを 32×32 の四角ごとに書き換え、履歴は 1 件（AI連携の履歴から元に戻せる）。
	 */
	function apiAdminMapEditApply(body) {
		var who = adminWho(body);
		var id = checkId(body.id);
		if (id >= count('map')) {
			fail(400, 'id', 'マップ ' + id + ' 番はありません');
		}
		var cells = body.cells;
		checkEditCells(id, cells);
		/* 四角ごとに分ける */
		var blocks = {}, order = [];
		cells.forEach(function (c) {
			var bx = Math.floor(c[0] / MAPEDIT_BLOCK), by = Math.floor(c[1] / MAPEDIT_BLOCK);
			var k = bx + ',' + by;
			if (!blocks[k]) {
				blocks[k] = { cells: [], x0: c[0], y0: c[1], x1: c[0], y1: c[1] };
				order.push(k);
			}
			var b = blocks[k];
			b.cells.push(c);
			b.x0 = Math.min(b.x0, c[0]);
			b.y0 = Math.min(b.y0, c[1]);
			b.x1 = Math.max(b.x1, c[0]);
			b.y1 = Math.max(b.y1, c[1]);
		});
		/* 今のマス目を読んで、ほかで変わったマスが無いか見る */
		var conflicts = [];
		order.forEach(function (k) {
			var b = blocks[k];
			b.w = b.x1 - b.x0 + 1;
			b.h = b.y1 - b.y0 + 1;
			b.now = readTiles(id, b.x0, b.y0, b.w, b.h);
			b.cells.forEach(function (c) {
				var v = b.now[c[1] - b.y0][c[0] - b.x0];
				if (v !== c[2] && v !== c[3]) {
					conflicts.push([c[0], c[1], c[2], v, c[3]]);
				}
			});
		});
		if (conflicts.length && !body.force) {
			return {
				ok: false, error: 'conflict', conflicts: conflicts.slice(0, 500), conflictCount: conflicts.length,
				message: '読み込んだ後に、ほかで変わったマスが ' + conflicts.length + ' 個あります（何も変えていません）'
			};
		}
		var map = getRecord('map', id);
		var name = map ? map.szName : '';
		backupFile('tiles');
		var items = [], nChanged = 0;
		order.forEach(function (k) {
			var b = blocks[k];
			var parts = b.now.map(function (row) { return row.slice(); });
			b.cells.forEach(function (c) {
				parts[c[1] - b.y0][c[0] - b.x0] = c[3];
			});
			var r = setTiles(id, b.x0, b.y0, parts, true);
			if (!r.ok) {
				fail(400, 'apply', r.error);
			}
			if (r.changed) {
				nChanged += r.changed;
				items.push({ id: id, name: name, x: b.x0, y: b.y0, w: b.w, h: b.h, before: r.before, after: r.after, changed: r.changed });
			}
		});
		if (!items.length) {
			return { ok: true, changed: 0, message: 'サーバーのマス目はもう同じでした' };
		}
		var e = addHistory({ op: 'tiles', kind: 'tiles', by: who.label,
			note: String(body.note || '管理ページのマップ編集').slice(0, 500), items: items });
		log(who.label + ' がマップ ' + id + ' のマス目を ' + nChanged + ' マス変更（マップ編集・履歴 ' + e.historyId + '）');
		return { ok: true, historyId: e.historyId, changed: nChanged };
	}

	/*
	 * マップの大きさを広げる（{id, top?, bottom?, left?, right?, fill?, check?}。縮めることはできない）。
	 * 上・左に広げると、そのマップの座標を持つもの（イベント・キャラ・復活地点・暗い範囲・光源・
	 * 巡回ルート・ほかのマップからの入口の行き先・依頼のNPC・サブゲームなど）を全部ずらす。
	 * AI連携の履歴には残さない（元に戻せない）ので、前の日ごとの控え（AiBackup）を取ってから変える。
	 * check:true なら確かめるだけ。
	 */
	function opMapResize(a, who) {
		requireWrite(who);
		var id = checkId(a.id);
		if (id >= count('map')) {
			fail(400, 'id', 'マップ ' + id + ' 番はありません');
		}
		var n = function (v) { return (typeof v === 'number' && isFinite(v)) ? Math.floor(v) : 0; };
		var args = [id, n(a.top), n(a.bottom), n(a.left), n(a.right), n(a.fill)];
		var r = JSON.parse(UTF8ToString(callC(_SboWebDataMapResize, args.concat([0]))));
		if (!r.ok || a.check) {
			if (!r.ok) fail(400, 'invalid', r.error);
			return r;
		}
		backupFile('tiles');
		r = JSON.parse(UTF8ToString(callC(_SboWebDataMapResize, args.concat([1]))));
		if (!r.ok) {
			fail(400, 'invalid', r.error);
		}
		schemaCache = null;
		log(who.label + ' がマップ ' + id + ' の大きさを ' + r.oldCx + '×' + r.oldCy + ' → ' + r.cx + '×' + r.cy + ' に広げた（左 ' + r.shiftX + '・上 ' + r.shiftY + ' ずらし）');
		return r;
	}

	/* マップを足す（{cx, cy, fill?, szName?}）。番号はいちばん後ろ */
	function opMapAdd(a, who) {
		requireWrite(who);
		var n = function (v) { return (typeof v === 'number' && isFinite(v)) ? Math.floor(v) : 0; };
		var r = JSON.parse(UTF8ToString(callC(_SboWebDataMapAdd, [n(a.cx), n(a.cy), n(a.fill), String(a.szName || '')])));
		if (!r.ok) {
			fail(400, 'invalid', r.error);
		}
		schemaCache = null;
		log(who.label + ' がマップ ' + r.id + '「' + String(a.szName || '') + '」（' + n(a.cx) + '×' + n(a.cy) + '）を足した');
		return r;
	}

	/* 下書き（SBODATA/SboWebMapDrafts.json。マップ 1 つに 1 つ。サーバーのデータは変えない） */
	function drafts() {
		if (!draftStore) {
			strDraftPath = path.join(dataDir(), 'SboWebMapDrafts.json');
			draftStore = readJsonFile(strDraftPath, {});
			draftStore.version = 1;
			draftStore.maps = draftStore.maps || {};
		}
		return draftStore;
	}
	function draftPublic(d) {
		return d ? { id: d.id, mapId: d.mapId, cx: d.cx, cy: d.cy, cells: d.cells, savedAt: d.savedAt, by: d.by } : null;
	}
	/* {session, id} → {draft:{id, mapId, cx, cy, cells, savedAt} | null} */
	function apiAdminMapEditDraftGet(body) {
		adminWho(body);
		var id = checkId(body.id);
		return { draft: draftPublic(drafts().maps[id]) };
	}
	/* {session} → {drafts:[{id, cells の数, savedAt}]}（下書きのあるマップの一覧） */
	function apiAdminMapEditDraftList(body) {
		adminWho(body);
		var ds = drafts();
		return {
			drafts: Object.keys(ds.maps).map(function (k) {
				var d = ds.maps[k];
				return { id: d.id, mapId: d.mapId, count: d.cells.length, savedAt: d.savedAt };
			})
		};
	}
	/* {session, id, cells:[[x, y, 前, 後], ...], mapId?, cx?, cy?}。cells が空なら消す */
	function apiAdminMapEditDraftSave(body) {
		var who = adminWho(body);
		var id = checkId(body.id);
		if (id >= count('map')) {
			fail(400, 'id', 'マップ ' + id + ' 番はありません');
		}
		var ds = drafts();
		if (!Array.isArray(body.cells) || !body.cells.length) {
			delete ds.maps[id];
			writeJsonFile(strDraftPath, ds);
			return { ok: true, deleted: true };
		}
		var size = checkEditCells(id, body.cells);
		var now = Date.now();
		ds.maps[id] = { id: id, mapId: size.mapId, cx: size.cx, cy: size.cy, cells: body.cells, savedAt: now, by: who.label };
		writeJsonFile(strDraftPath, ds);
		return { ok: true, savedAt: now, count: body.cells.length };
	}

	/*
	 * スナップショット（名前を付けたマップの姿。SBODATA/SboWebMapSnapshots.json。ゲームのデータは変えない）
	 * 編集画面の今の姿（サーバーのマス目に、まだ反映していない cells を重ねたもの）を丸ごと残し、
	 * あとで編集画面に読み込める（読み込んでも「サーバーへ反映」するまでゲームは変わらない）。
	 * マス目は「パーツ番号*続く数」をカンマでつないだ文字列（同じパーツが続く所が多いので短くなる）。
	 */
	var MAX_SNAPSHOTS = 30;			/* 1 マップに残せる数 */
	var snapStore = null, strSnapPath = '';
	function snaps() {
		if (!snapStore) {
			strSnapPath = path.join(dataDir(), 'SboWebMapSnapshots.json');
			snapStore = readJsonFile(strSnapPath, {});
			snapStore.version = 1;
			snapStore.seq = snapStore.seq || 0;
			snapStore.maps = snapStore.maps || {};
		}
		return snapStore;
	}
	/* マップのマス目を全部読む（[行][列]） */
	function readAllTiles(id) {
		var size = JSON.parse(UTF8ToString(_SboWebDataMapTiles(id, 0, 0, 1, 1)));
		if (size.error) {
			fail(404, 'not_found', size.error);
		}
		var rows = [];
		for (var y = 0; y < size.cy; y += 128) {
			for (var x = 0; x < size.cx; x += 128) {
				var part = readTiles(id, x, y, Math.min(128, size.cx - x), Math.min(128, size.cy - y));
				part.forEach(function (row, dy) {
					var r = rows[y + dy] = rows[y + dy] || [];
					row.forEach(function (v, dx) { r[x + dx] = v; });
				});
			}
		}
		return { cx: size.cx, cy: size.cy, mapId: size.mapId, rows: rows };
	}
	function packTiles(rows) {
		var out = [], prev = -1, n = 0;
		rows.forEach(function (row) {
			row.forEach(function (v) {
				if (v === prev) {
					n++;
					return;
				}
				if (n) out.push(n > 1 ? prev + '*' + n : String(prev));
				prev = v;
				n = 1;
			});
		});
		if (n) out.push(n > 1 ? prev + '*' + n : String(prev));
		return out.join(',');
	}
	function snapPublic(s) {
		return { snapId: s.snapId, name: s.name, at: s.at, by: s.by, cx: s.cx, cy: s.cy };
	}
	/* {session, id} → {snapshots:[{snapId, name, at, by, cx, cy}]}（新しい順） */
	function apiAdminMapSnapList(body) {
		adminWho(body);
		var id = checkId(body.id);
		return { snapshots: (snaps().maps[id] || []).slice().reverse().map(snapPublic) };
	}
	/* {session, id, name, cells?:[[x, y, 前, 後], ...]} → 今のマス目に cells を重ねた姿を残す */
	function apiAdminMapSnapSave(body) {
		var who = adminWho(body);
		var id = checkId(body.id);
		if (id >= count('map')) {
			fail(400, 'id', 'マップ ' + id + ' 番はありません');
		}
		var name = String(body.name || '').trim().slice(0, 60);
		if (!name) {
			fail(400, 'name', 'スナップショットの名前を入れてください');
		}
		var all = readAllTiles(id);
		if (Array.isArray(body.cells) && body.cells.length) {
			checkEditCells(id, body.cells);
			body.cells.forEach(function (c) { all.rows[c[1]][c[0]] = c[3]; });
		}
		var ss = snaps();
		var list = ss.maps[id] = ss.maps[id] || [];
		var s = { snapId: ++ss.seq, name: name, at: Date.now(), by: who.label, mapId: all.mapId, cx: all.cx, cy: all.cy, tiles: packTiles(all.rows) };
		list.push(s);
		var nDrop = Math.max(0, list.length - MAX_SNAPSHOTS);
		if (nDrop) list.splice(0, nDrop);
		writeJsonFile(strSnapPath, ss);
		return { ok: true, snapshot: snapPublic(s), dropped: nDrop || undefined };
	}
	/* {session, id, snapId} → {snapshot:{..., parts:[行][列]}} */
	function apiAdminMapSnapGet(body) {
		adminWho(body);
		var id = checkId(body.id);
		var s = (snaps().maps[id] || []).filter(function (x) { return x.snapId === body.snapId; })[0];
		if (!s) {
			fail(404, 'not_found', 'そのスナップショットはありません');
		}
		var flat = [];
		s.tiles.split(',').forEach(function (tok) {
			var a = tok.split('*'), v = Number(a[0]), n = a.length > 1 ? Number(a[1]) : 1;
			for (var i = 0; i < n; i++) flat.push(v);
		});
		var rows = [];
		for (var y = 0; y < s.cy; y++) rows.push(flat.slice(y * s.cx, (y + 1) * s.cx));
		var r = snapPublic(s);
		r.parts = rows;
		return { snapshot: r };
	}
	/* {session, id, snapId} → 消す */
	function apiAdminMapSnapDelete(body) {
		adminWho(body);
		var id = checkId(body.id);
		var ss = snaps();
		var list = ss.maps[id] || [];
		var n = list.length;
		ss.maps[id] = list.filter(function (x) { return x.snapId !== body.snapId; });
		if (ss.maps[id].length === n) {
			fail(404, 'not_found', 'そのスナップショットはありません');
		}
		writeJsonFile(strSnapPath, ss);
		return { ok: true };
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
		'ステータステンプレート・アイテム・作り手・レシピ・NPC と台詞・マップのイベント／店／メッセージ・マップパーツ・エフェクト・吹き出し・サブゲーム）を' +
		'読み書きできます。プレイヤーのキャラと預かり所は読むだけです。マップのマス目は sbo_map_tiles で読み、sbo_map_tiles_set で書きます。' +
		'NPC は番号（id）が再起動で変わることがあるので、npcId（変わらない番号）で見分けてください。' +
		'まず sbo_schema で種類と項目の意味を確かめてください。' +
		'書き込みは必ず 2 段階です: previewId 無しで呼ぶと差分が返るので、それを利用者に見せて了解をもらってから、' +
		'同じ引数に previewId を付けてもう一度呼んでください。反映はログイン中のプレイヤーにもすぐ届きます。' +
		'間違えた時は sbo_history で履歴を見て sbo_undo で戻せます。街をお試しで作り変える時は、先に sbo_bookmark でしおりを付けておくと、' +
		'sbo_rollback でしおりの後の変更（NPC の追加・削除・マス目も含む）をまとめて戻せます。' +
		'新しいマップの配置を考える時は、sbo_parts_dex（パーツ図鑑: パーツの名前・タグ・通れるか・よく隣に来るパーツ）と ' +
		'sbo_parts_samples（組み合わせの見本）を見て、案を sbo_plan_save で保存してください。配置案はゲームのデータを変えず、' +
		'管理ページの「配置案」で画像として見られます。';

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
			name: 'sbo_map_tiles_set', write: true,
			description: 'マップのマス目を書き換える。id は map の番号、(x, y) が左上、parts は [[mapParts の番号,...],...]（行ごと、どの行も同じ長さ、最大 128×128）。' +
				'マップの外にはみ出す所はエラー。履歴に残り sbo_undo・sbo_rollback で戻せる。2 段階は sbo_update と同じ。',
			inputSchema: {
				type: 'object', required: ['id', 'x', 'y', 'parts'],
				properties: {
					id: { type: 'integer', minimum: 0 },
					x: { type: 'integer', minimum: 0 }, y: { type: 'integer', minimum: 0 },
					parts: { type: 'array', minItems: 1, maxItems: 128, items: { type: 'array', minItems: 1, maxItems: 128, items: { type: 'integer', minimum: 0 } } },
					note: { type: 'string' },
					previewId: { type: 'string' }
				}
			},
			run: opTilesSet
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
			description: 'データを末尾に追加する。省いた項目は 0 や空になる。追加した NPC は元に戻すと消え、作り手・レシピは削除（byState=0）、迷路（subGame の byType 2）は止まる（byType=0）。' +
				'それ以外の種類の追加は元に戻せない。NPC は dwMapID・dwX・dwY が要る。2 段階は sbo_update と同じ。',
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
				properties: { kind: { type: 'string', enum: WRITE_KIND_ENUM.concat(['tiles']) }, limit: { type: 'integer', minimum: 1, maximum: 100 } }
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
		},
		{
			name: 'sbo_bookmark', write: true,
			description: 'しおりを付ける（お試しの区切り）。この後の変更は sbo_rollback でまとめて元に戻せる。' +
				'街をお試しで作り変える前などに、利用者が分かる名前を付けて呼ぶ。データは何も変わらない。',
			inputSchema: {
				type: 'object', required: ['name'],
				properties: { name: { type: 'string', description: '例: 「レジェンタのお試し1」' }, note: { type: 'string' } }
			},
			run: opBookmarkAdd
		},
		{
			name: 'sbo_bookmarks', write: false,
			description: 'しおりの一覧（新しい順）。changes はしおりの後の変更履歴の数、lost は古い履歴が消えて戻せないもの。',
			inputSchema: { type: 'object', properties: { limit: { type: 'integer', minimum: 1, maximum: 100 } } },
			run: opBookmarks
		},
		{
			name: 'sbo_rollback', write: true,
			description: 'しおりの後の変更をまとめて元に戻す（種類をまたいで。マス目は新しい順に）。追加した NPC は消し、消した NPC は同じ npcId で足し直す。' +
				'追加した作り手・レシピは削除（byState=0）、追加した迷路は止める（byType=0）。' +
				'AI 以外からも変わったデータがあると force を付けない限り戻さない。2 段階は sbo_update と同じ。',
			inputSchema: {
				type: 'object', required: ['bookmarkId'],
				properties: {
					bookmarkId: { type: 'integer' },
					force: { type: 'boolean' },
					note: { type: 'string' },
					previewId: { type: 'string' }
				}
			},
			run: opRollback
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
		'/api/data/tiles/set': opTilesSet,
		'/api/data/update': opUpdate,
		'/api/data/add': opAdd,
		'/api/data/history': opHistory,
		'/api/data/undo': opUndo,
		'/api/data/bookmark': opBookmarkAdd,
		'/api/data/bookmarks': opBookmarks,
		'/api/data/rollback': opRollback,
		'/api/data/map/resize': opMapResize,
		'/api/data/map/add': opMapAdd
	};
	var API_ADMIN = {
		'/api/admin/ai/keys': apiAdminKeys,
		'/api/admin/ai/issue': apiAdminIssue,
		'/api/admin/ai/revoke': apiAdminRevoke,
		'/api/admin/ai/history': function (body) { adminWho(body); return opHistory(body); },
		'/api/admin/ai/undo': function (body) { return opUndo(body, adminWho(body)); },
		'/api/admin/ai/tiles/set': function (body) { return opTilesSet(body, adminWho(body)); },
		'/api/admin/ai/bookmark': function (body) { return opBookmarkAdd(body, adminWho(body)); },
		'/api/admin/ai/bookmarks': function (body) { adminWho(body); return opBookmarks(body); },
		'/api/admin/ai/rollback': function (body) { return opRollback(body, adminWho(body)); },
		/* マップ資料の書き出し（管理ページの「マップ資料」）。読むだけ */
		'/api/admin/data/list': function (body) { adminWho(body); return opList(body); },
		'/api/admin/data/tiles': function (body) { adminWho(body); return opTiles(body); },
		/* 作り手・レシピの保存（管理ページの「レシピ」）。読む方は /api/admin/data/list */
		'/api/admin/recipe/save': apiAdminRecipeSave,
		/* NPC の巡回ルートを決める（管理ページの「NPCの巡回」） */
		'/api/admin/npcroute/set': apiAdminNpcRouteSet,
		/* マップ確認の編集モード（反映・下書き） */
		'/api/admin/mapedit/apply': apiAdminMapEditApply,
		'/api/admin/mapedit/draft/get': apiAdminMapEditDraftGet,
		'/api/admin/mapedit/draft/list': apiAdminMapEditDraftList,
		'/api/admin/mapedit/draft/save': apiAdminMapEditDraftSave,
		'/api/admin/mapedit/snap/list': apiAdminMapSnapList,
		'/api/admin/mapedit/snap/save': apiAdminMapSnapSave,
		'/api/admin/mapedit/snap/get': apiAdminMapSnapGet,
		'/api/admin/mapedit/snap/delete': apiAdminMapSnapDelete,
		'/api/admin/mapedit/resize': function (body) { return opMapResize(body, adminWho(body)); },
		'/api/admin/mapedit/add': function (body) { return opMapAdd(body, adminWho(body)); }
	};

	/*
	 * 作り手（maker）・レシピ（recipe）の保存（管理ページの「レシピ」タブ）。
	 * {session, kind, id?, changes}。id を省くか件数と同じにすると追加、それ以外は変更。
	 * 削除は changes に {byState: 0} を入れる（空き枠になる）。
	 * 管理ページでは下見の画面が別にあるので 2 段階にせず、AI 用の変更履歴（元に戻せる）に残して即反映する。
	 */
	function apiAdminRecipeSave(body) {
		var who = adminWho(body);
		var kind = body.kind;
		if (kind !== 'maker' && kind !== 'recipe') {
			fail(400, 'kind', 'kind は maker か recipe を指定してください');
		}
		var total = count(kind);
		var id = (body.id == null) ? total : checkId(body.id);
		if (id > total) {
			fail(400, 'id', kind + ' に ' + id + ' 番はありません（' + total + ' 件。追加は ' + total + ' 番）');
		}
		var changes = body.changes;
		if (!changes || typeof changes !== 'object' || Array.isArray(changes)) {
			fail(400, 'changes', 'changes は変える項目だけのオブジェクトで指定してください');
		}
		var added = (id === total);
		var info = kindInfo(kind);
		if (added && info && total >= info.maxCount) {
			fail(400, 'too_many', kind + ' は ' + info.maxCount + ' 件までです。削除済みの枠（byState=0）があればそこを使い回してください');
		}
		var r = applyC(kind, id, changes, false);
		if (!r.ok) {
			fail(400, 'invalid', r.error);
		}
		var aDone = applyAll(kind, [{ id: id, changes: changes, added: added }], who);
		var e = addHistory({ op: added ? 'add' : 'update', kind: kind, by: who.label, note: '', items: aDone });
		log(who.label + ' が ' + kind + ' の ' + id + ' 番を' + (added ? '追加' : '変更') + '（履歴 ' + e.historyId + '）');
		return { ok: true, id: id, added: added, historyId: e.historyId, record: aDone[0].after };
	}

	/* NPC の巡回ルート設定（管理ページの「NPCの巡回」）。C 側の SboWebNpcRouteSet に渡す */
	function apiAdminNpcRouteSet(body) {
		adminWho(body);
		var id = Number(body.id);
		if (!isFinite(id) || Math.floor(id) !== id || id < 0) {
			fail(400, 'id', 'キャラの番号が正しくありません');
		}
		var mode = Number(body.mode);
		if (mode !== 0 && mode !== 1) {
			fail(400, 'mode', '回り方は 0（折り返す）か 1（一周）です');
		}
		var wait = Number(body.wait);
		if (!isFinite(wait) || Math.floor(wait) !== wait || wait < 0 || wait > 60) {
			fail(400, 'wait', '止まる秒数は 0〜60 です');
		}
		var pts = body.points == null ? [] : body.points;
		if (!Array.isArray(pts)) {
			fail(400, 'points', '道しるべは [[x,y],...] の形で送ってください');
		}
		if (pts.length === 1) {
			fail(400, 'points', '道しるべは 2 つ以上置いてください（やめる時は 0 個）');
		}
		if (pts.length > 16) {
			fail(400, 'points', '道しるべは 16 個までです');
		}
		var aStr = pts.map(function (p) {
			var x = Array.isArray(p) ? Number(p[0]) : NaN, y = Array.isArray(p) ? Number(p[1]) : NaN;
			if (!isFinite(x) || !isFinite(y) || Math.floor(x) !== x || Math.floor(y) !== y || x < 0 || y < 0 || x > 255 || y > 255) {
				fail(400, 'points', '道しるべの場所が正しくありません');
			}
			return x + ',' + y;
		});
		var str = aStr.join(';');
		var r = JSON.parse(UTF8ToString(callC(_SboWebNpcRouteSet, [id, mode, wait, str])));
		if (r.error) {
			fail(400, 'route', r.error);
		}
		log('管理ページが キャラ ' + id + ' の巡回を' + (pts.length ? '設定（' + pts.length + ' 点・' + (mode ? '一周' : '折り返す') + '・' + wait + ' 秒）' : 'やめた'));
		return r;
	}

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

	/* 別のファイル（sbosvr_mapdesign.js のパーツ図鑑・配置案）が、同じキー・同じ MCP にツールを足すための入口 */
	var ext = {
		fail: fail,
		ApiError: ApiError,
		checkId: checkId,
		requireWrite: requireWrite,
		adminWho: adminWho,
		count: count,
		getRecord: getRecord,
		opTiles: opTiles,
		dataDir: dataDir,
		readJsonFile: readJsonFile,
		writeJsonFile: writeJsonFile,
		log: log,
		/* tool は TOOLS と同じ形（name・write・description・inputSchema・run） */
		addTool: function (tool) { TOOLS.push(tool); },
		addApiData: function (url, fn) { API_DATA[url] = fn; },
		addApiAdmin: function (url, fn) { API_ADMIN[url] = fn; }
	};

	return {
		handleHttp: handleHttp,
		ext: ext
	};
})();
