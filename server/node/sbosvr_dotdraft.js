/* ========================================================================= */
/* sbosvr_dotdraft.js														 */
/* ドット絵エディタの一時保存（控え）をサーバーに置く（Emscripten の --pre-js で組み込む） */
/* ========================================================================= */
/*
 * 管理ページのドット絵エディタ（/dot。web/account/dot-editor.js）の「一時保存」の控えを
 * サーバーに置いて、PC・タブレットなど別の端末からも続きを描けるようにする。
 * 最後の書き出しは今まで通り、PC のローカル作業フォルダへ（ここはゲームの絵を一切変えない）。
 *
 * ・管理者ログイン（Cookie。sbosvr_account.js の isAdminSession）が無いと使えない。
 * ・保存先は SBODATA/SboWebDotDrafts/。控え 1 件を 1 ファイル（<id>.json）に、
 *   一覧に出す分（名前・小さい絵など）を index.json にまとめる。
 * ・レイヤーの中身（色番号の Int16 の並び、リトルエンディアン）は base64 で受け取り、そのまま保存する。
 *   enc:'gz' の時は gzip で縮めたものの base64（ブラウザの CompressionStream）。
 *
 * 控え: {id, file, name, savedAt, width, height, palette:[{r,g,b,a}...], layers:[{name, visible, ghost, data(base64), enc?:'gz'}],
 *        thumbnail(dataURL), kind:'manual'}
 *
 * HTTP（sbosvr_net.js が /api/admin/dot/ をここへ回す。どれも POST）
 *   /api/admin/dot/drafts/list   {session}                        → {items:[一覧の分...]}（新しい順）
 *   /api/admin/dot/drafts/get    {session, id}                    → {item}
 *   /api/admin/dot/drafts/save   {session, item, baseSavedAt?}    → {ok, item:一覧の分}
 *        同じ id が既にあり、baseSavedAt（開いた時の保存時刻）と違う時は、別の端末で上書きされているので
 *        {error:'conflict', savedAt} を返す（クライアントは別の控えとして保存し直す）
 *   /api/admin/dot/drafts/rename {session, id, name}              → {ok, item}
 *   /api/admin/dot/drafts/delete {session, id}                    → {ok}
 */
Module['sboDotDraft'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var zlib = require('zlib');

	var MAX_DRAFTS = 300;					/* 控えの数 */
	var MAX_BODY = 24 * 1024 * 1024;		/* 受け取る JSON の大きさ（大きな絵・レイヤーが多い時用） */
	var MAX_SIDE = 4096;					/* 絵の 1 辺 */
	var MAX_LAYERS = 64;
	var MAX_THUMB = 256 * 1024;				/* 小さい絵（dataURL）の長さ */

	var index = null;						/* {version, items:[一覧の分]} */
	var strDir = '';

	function acc() {
		return Module['sboAccount'];
	}

	function log(msg) {
		console.log('[dotdraft] ' + msg);
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	function dir() {
		if (!strDir) {
			strDir = path.resolve(FS.cwd(), 'SBODATA', 'SboWebDotDrafts');
		}
		if (!fs.existsSync(strDir)) {
			fs.mkdirSync(strDir, { recursive: true });
		}
		return strDir;
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function writeJson(file, obj) {
		var tmp = file + '.tmp';
		fs.writeFileSync(tmp, JSON.stringify(obj));
		fs.renameSync(tmp, file);
	}

	function load() {
		if (index) {
			return index;
		}
		var file = path.join(dir(), 'index.json');
		try {
			index = JSON.parse(fs.readFileSync(file, 'utf8'));
		} catch (e) {
			index = null;
		}
		if (!index || typeof index !== 'object') {
			index = {};
		}
		index.version = 1;
		index.items = Array.isArray(index.items) ? index.items : [];
		return index;
	}

	function saveIndex() {
		index.items.sort(function (a, b) { return b.savedAt - a.savedAt; });
		writeJson(path.join(dir(), 'index.json'), index);
	}

	function findMeta(id) {
		for (var i = 0; i < index.items.length; i++) {
			if (index.items[i].id === id) {
				return i;
			}
		}
		return -1;
	}

	function draftPath(id) {
		return path.join(dir(), id + '.json');
	}

	/* ------------------------------------------------------------------ */
	/* 中身を確かめる															 */
	/* ------------------------------------------------------------------ */
	function str(v, nMax) {
		return String(v == null ? '' : v).replace(/[\u0000-\u001F\u007F]/g, '').slice(0, nMax || 64);
	}

	function cleanId(v) {
		var s = String(v == null ? '' : v);
		return /^[A-Za-z0-9_-]{1,48}$/.test(s) ? s : '';
	}

	function cleanName(v) {
		return str(v, 100).trim();
	}

	function byte(v) {
		v = v | 0;
		return v < 0 ? 0 : v > 255 ? 255 : v;
	}

	/* 送られてきた控えを確かめてそろえる。おかしい時は null */
	function cleanDraft(src) {
		if (!src || typeof src !== 'object') {
			return null;
		}
		var id = cleanId(src.id);
		var w = src.width | 0, h = src.height | 0;
		if (!id || w < 1 || h < 1 || w > MAX_SIDE || h > MAX_SIDE) {
			return null;
		}
		if (!Array.isArray(src.palette) || src.palette.length < 1 || src.palette.length > 256) {
			return null;
		}
		if (!Array.isArray(src.layers) || src.layers.length < 1 || src.layers.length > MAX_LAYERS) {
			return null;
		}
		var nBytes = w * h * 2;
		var layers = [];
		for (var i = 0; i < src.layers.length; i++) {
			var l = src.layers[i];
			if (!l || typeof l.data !== 'string' || !/^[A-Za-z0-9+/]*={0,2}$/.test(l.data)) {
				return null;
			}
			var enc = l.enc === 'gz' ? 'gz' : '';
			if (enc) {
				/* gzip で縮めて送られてくる（ほとんど同じ色番号の並びなのでよく縮む）。ほどいて大きさだけ確かめ、縮めたまま置く */
				try {
					if (zlib.gunzipSync(Buffer.from(l.data, 'base64'), { maxOutputLength: nBytes + 1 }).length !== nBytes) {
						return null;
					}
				} catch (e) {
					return null;
				}
			} else if (Buffer.byteLength(l.data, 'base64') !== nBytes) {
				return null;
			}
			var o = { name: str(l.name, 64), visible: !!l.visible, ghost: !!l.ghost, data: l.data };
			if (enc) {
				o.enc = enc;
			}
			layers.push(o);
		}
		var thumb = typeof src.thumbnail === 'string' && src.thumbnail.length <= MAX_THUMB &&
			src.thumbnail.indexOf('data:image/png;base64,') === 0 ? src.thumbnail : '';
		return {
			id: id,
			file: str(src.file, 255),
			name: cleanName(src.name) || str(src.file, 255) || id,
			savedAt: Date.now(),
			width: w,
			height: h,
			palette: src.palette.map(function (c) {
				c = c || {};
				return { r: byte(c.r), g: byte(c.g), b: byte(c.b), a: c.a == null ? 255 : byte(c.a) };
			}),
			layers: layers,
			thumbnail: thumb,
			kind: 'manual'
		};
	}

	function metaOf(d) {
		return { id: d.id, file: d.file, name: d.name, savedAt: d.savedAt, width: d.width, height: d.height,
			layerCount: d.layers ? d.layers.length : (d.layerCount || 0), thumbnail: d.thumbnail || '', kind: 'manual' };
	}

	function readDraft(id) {
		try {
			return JSON.parse(fs.readFileSync(draftPath(id), 'utf8'));
		} catch (e) {
			return null;
		}
	}

	/* ------------------------------------------------------------------ */
	/* HTTP																	 */
	/* ------------------------------------------------------------------ */
	function reply(res, status, obj) {
		acc().reply(res, status, obj);
	}

	function readJson(req, cb) {
		var aBuf = [], nSize = 0, bOver = false;
		req.on('data', function (buf) {
			if (bOver) {
				return;
			}
			nSize += buf.length;
			if (nSize > MAX_BODY) {
				bOver = true;
				aBuf = [];
				return;
			}
			aBuf.push(buf);
		});
		req.on('end', function () {
			if (bOver) {
				cb(null, 'large');
				return;
			}
			try {
				var obj = JSON.parse(Buffer.concat(aBuf).toString('utf8') || '{}');
				cb(obj && typeof obj === 'object' ? obj : null, 'json');
			} catch (e) {
				cb(null, 'json');
			}
		});
	}

	function apiList(body, req, res) {
		load();
		reply(res, 200, { items: index.items, max: MAX_DRAFTS });
	}

	function apiGet(body, req, res) {
		load();
		var id = cleanId(body.id);
		var d = id && findMeta(id) >= 0 ? readDraft(id) : null;
		if (!d) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		reply(res, 200, { item: d });
	}

	function apiSave(body, req, res) {
		load();
		var d = cleanDraft(body.item);
		if (!d) {
			reply(res, 400, { error: 'invalid' });
			return;
		}
		var i = findMeta(d.id);
		if (i >= 0) {
			/* 別の端末で先に上書きされていたら、黙って消さない */
			var base = Number(body.baseSavedAt) || 0;
			if (base && index.items[i].savedAt !== base) {
				reply(res, 200, { error: 'conflict', savedAt: index.items[i].savedAt });
				return;
			}
		} else if (index.items.length >= MAX_DRAFTS) {
			reply(res, 200, { error: 'full', max: MAX_DRAFTS });
			return;
		}
		/* 同じミリ秒に 2 回保存されても、時刻で見分けられるようにずらす */
		if (i >= 0 && d.savedAt <= index.items[i].savedAt) {
			d.savedAt = index.items[i].savedAt + 1;
		}
		writeJson(draftPath(d.id), d);
		var meta = metaOf(d);
		if (i >= 0) {
			index.items[i] = meta;
		} else {
			index.items.push(meta);
		}
		saveIndex();
		log((i >= 0 ? '上書き: ' : '新しく保存: ') + d.id + ' ' + d.file + ' ' + d.width + 'x' + d.height);
		reply(res, 200, { ok: true, item: meta });
	}

	function apiRename(body, req, res) {
		load();
		var id = cleanId(body.id);
		var name = cleanName(body.name);
		var i = id ? findMeta(id) : -1;
		var d = i >= 0 ? readDraft(id) : null;
		if (!d) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (!name) {
			reply(res, 400, { error: 'invalid' });
			return;
		}
		/* 名前だけ変える。保存時刻は変えない（ほかの端末で開いている控えが「上書きされた」扱いにならないように） */
		d.name = name;
		writeJson(draftPath(id), d);
		index.items[i].name = name;
		saveIndex();
		reply(res, 200, { ok: true, item: index.items[i] });
	}

	function apiDelete(body, req, res) {
		load();
		var id = cleanId(body.id);
		var i = id ? findMeta(id) : -1;
		if (i < 0) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		index.items.splice(i, 1);
		saveIndex();
		try {
			fs.unlinkSync(draftPath(id));
		} catch (e) {
			/* ファイルが無くても一覧から消えれば良い */
		}
		log('削除: ' + id);
		reply(res, 200, { ok: true });
	}

	var API = {
		'/api/admin/dot/drafts/list': apiList,
		'/api/admin/dot/drafts/get': apiGet,
		'/api/admin/dot/drafts/save': apiSave,
		'/api/admin/dot/drafts/rename': apiRename,
		'/api/admin/dot/drafts/delete': apiDelete
	};

	/* sbosvr_net.js の HTTP から呼ばれる。ここの API だけ受け持つ */
	function handleHttp(req, res, url) {
		var fn = API[url];
		if (!fn || !acc()) {
			return false;
		}
		if (req.method !== 'POST') {
			reply(res, 405, { error: 'method' });
			return true;
		}
		/* 大きな中身を読む前に、ログインを確かめる（Cookie だけで分かる） */
		var pre = acc().withAdminCookie({}, req);
		if (pre.session && !acc().isAdminSession(pre.session)) {
			req.resume();
			reply(res, 401, { error: 'session' });
			return true;
		}
		readJson(req, function (body, why) {
			if (!body) {
				reply(res, why === 'large' ? 413 : 400, { error: why });
				return;
			}
			acc().withAdminCookie(body, req);		/* 管理ページのログインは Cookie */
			if (!acc().isAdminSession(body.session)) {
				reply(res, 401, { error: 'session' });
				return;
			}
			try {
				fn(body, req, res);
			} catch (e) {
				console.error('[dotdraft] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	return {
		handleHttp: handleHttp
	};
})();
