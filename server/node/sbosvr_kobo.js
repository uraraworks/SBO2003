/* ========================================================================= */
/* sbosvr_kobo.js															 */
/* ドット絵工房（Emscripten の --pre-js で組み込む）							 */
/* ========================================================================= */
/*
 * プレイヤーみんなでゲームのドット絵を描き合うページ（/kobo・/kobo-draw。web/account/kobo*.html）の API。
 * 仕様は docs/kobo.md（保存形式・API・決まり）。
 *
 * ・管理者が「お題」を出し、プレイヤーが「案」を描いて出す（親つきで描き足す＝系譜）。
 * ・案へのコメントは誰でも書けるが、返事できるのはその案の本人と管理者だけ。
 * ・プレイヤーは「提案」を出せる（見えるのは管理者だけ）。管理者が「お題にする」とお題になる。
 * ・管理者が案を 1 つ採用する。描いた人・元の案の人・提案した人が協力者（credits）に残る。
 *
 * 保存先
 *   SBODATA/SboWebKobo.json   … 一覧（お題・案・コメント・なじんでる・提案）
 *   SBODATA/kobo/<id>.png     … 画像（元イラストの JPEG だけ .jpg）。書き換えず、新しい id を作る
 *
 * 絵は 8bit パレット PNG。案の PNG はお題と同じ大きさ・同じパレットでないと受け付けない（zlib で確かめる）。
 */
Module['sboKobo'] = (function () {
	var fs = require('fs');
	var path = require('path');
	var crypto = require('crypto');
	var zlib = require('zlib');

	var MAX_WORKS = 20000;			/* 残す案の数 */
	var MAX_COMMENTS = 20000;		/* 残すコメントの数 */
	var MAX_CHEERS = 100000;
	var MAX_PROPOSALS = 5000;
	var MAX_TASKS = 2000;
	var MAX_BODY = 1024 * 1024;		/* 受け取る JSON の大きさ（プレイヤー） */
	var MAX_BODY_ADMIN = 4 * 1024 * 1024;	/* 管理者のお題の保存（元イラスト 1MB＋見本など） */
	var MAX_PNG = 256 * 1024;		/* PNG の大きさ（バイト） */
	var MAX_REF = 1024 * 1024;		/* 元イラスト */
	var MAX_SIDE = 512;				/* ラフ絵・描く範囲の幅と高さ */
	var MAX_TITLE = 100;
	var MAX_DESC = 2000;
	var MAX_NOTE = 200;
	var MAX_COMMENT = 300;
	var MAX_ADMIN_COMMENT = 1000;
	var MAX_PROPOSAL = 1000;
	var MAX_REPLY = 1000;
	var AGREE_VERSION = 1;			/* 同意した文面の版 */
	var DAY = 24 * 60 * 60 * 1000;
	var KINDS = ['char', 'part', 'effect'];
	var STATES = ['open', 'judging', 'done', 'hidden'];
	var PREVIEWS = ['walk', 'tile', 'anim'];
	var PROPOSAL_STATUS = ['new', 'later', 'declined', 'tasked'];

	/* 制限（Module['sboKoboLimits'] で変えられる。確認用の stub-server --fast が使う） */
	var LIM = {
		newWorkInterval: 60 * 1000,	/* 新しい案は 1 アカウント 1 分に 1 件 */
		newWorkPerDay: 30,
		wipInterval: 20 * 1000,		/* 作業中の上書きは 1 アカウント 20 秒に 1 回 */
		wipPerTask: 3,				/* 1 アカウントの作業中は 1 お題につき 3 つ */
		commentPerMinute: 3,
		commentPerDay: 50,
		proposalPerDay: 3
	};

	var store = null;
	var strStorePath = '';
	var strImgDir = '';
	var memNew = {};				/* アカウントごとの「新しい案を作った時刻」（消されても数え直さない） */
	var memWip = {};				/* アカウントごとの「作業中を上書きした時刻」 */

	function acc() {
		return Module['sboAccount'];
	}

	function log(msg) {
		console.log('[kobo] ' + msg);
	}

	/* ------------------------------------------------------------------ */
	/* 保存																	 */
	/* ------------------------------------------------------------------ */
	function load() {
		if (store) {
			return store;
		}
		var o = Module['sboKoboLimits'];
		if (o && typeof o === 'object') {
			Object.keys(o).forEach(function (k) {
				if (k in LIM) {
					LIM[k] = o[k];
				}
			});
		}
		var strDir = path.resolve(FS.cwd(), 'SBODATA');
		strStorePath = path.join(strDir, 'SboWebKobo.json');
		strImgDir = path.join(strDir, 'kobo');
		try {
			store = JSON.parse(fs.readFileSync(strStorePath, 'utf8'));
		} catch (e) {
			store = null;
		}
		if (!store || typeof store !== 'object') {
			store = {};
		}
		store.version = 1;
		['tasks', 'works', 'comments', 'cheers', 'proposals'].forEach(function (k) {
			store[k] = Array.isArray(store[k]) ? store[k] : [];
		});
		try {
			fs.mkdirSync(strImgDir, { recursive: true });
		} catch (e) {
			console.error('[kobo] 画像のフォルダを作れません: ' + e.message);
		}
		return store;
	}

	/* 書きかけで落ちても壊れないよう、別名で書いてから置き換える */
	function save() {
		var tmp = strStorePath + '.tmp';
		try {
			fs.writeFileSync(tmp, JSON.stringify(store, null, '\t'));
			fs.renameSync(tmp, strStorePath);
		} catch (e) {
			console.error('[kobo] 保存できません: ' + e.message);
		}
	}

	function newId() {
		return crypto.randomBytes(6).toString('hex');
	}

	/* 画像を書いて id を返す（別名で書いてから置き換える） */
	function putImage(buf, ext) {
		var id = newId();
		var strFile = path.join(strImgDir, id + '.' + ext);
		fs.writeFileSync(strFile + '.tmp', buf);
		fs.renameSync(strFile + '.tmp', strFile);
		return id;
	}

	function delImage(id) {
		if (!id || !/^[0-9a-f]{12}$/.test(id)) {
			return;
		}
		['png', 'jpg'].forEach(function (ext) {
			try {
				fs.unlinkSync(path.join(strImgDir, id + '.' + ext));
			} catch (e) { /* 無ければそのまま */ }
		});
	}

	function findIn(a, id) {
		for (var i = 0; i < a.length; i++) {
			if (a[i].id === id) {
				return a[i];
			}
		}
		return null;
	}

	function removeFrom(a, o) {
		var i = a.indexOf(o);
		if (i >= 0) {
			a.splice(i, 1);
		}
	}

	/* ------------------------------------------------------------------ */
	/* PNG の確認（zlib）														 */
	/* ------------------------------------------------------------------ */
	function bad(code) {
		var e = new Error(code);
		e.kobo = code;
		return e;
	}

	/* base64 の文字列を Buffer に（大きすぎる・空なら bad_png） */
	function decodeB64(s, nMax) {
		if (typeof s !== 'string' || !s) {
			throw bad('bad_png');
		}
		s = s.replace(/^data:[^,]*,/, '');
		var buf = Buffer.from(s, 'base64');
		if (!buf.length || buf.length > nMax) {
			throw bad('bad_png');
		}
		return buf;
	}

	/* PNG の構造を調べる。IDAT は展開して raw に入れる。壊れていたら bad_png */
	function parsePng(buf) {
		var SIG = [0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a];
		if (buf.length < 8 + 25 || !SIG.every(function (b, i) { return buf[i] === b; })) {
			throw bad('bad_png');
		}
		var o = { w: 0, h: 0, depth: 0, ctype: 0, interlace: 0, plte: null }, aIdat = [], bEnd = false;
		var pos = 8, bFirst = true;
		while (pos + 12 <= buf.length) {
			var nLen = buf.readUInt32BE(pos), strType = buf.toString('latin1', pos + 4, pos + 8);
			if (nLen > buf.length - pos - 12) {
				throw bad('bad_png');
			}
			var data = buf.subarray(pos + 8, pos + 8 + nLen);
			if (bFirst && strType !== 'IHDR') {
				throw bad('bad_png');
			}
			bFirst = false;
			if (strType === 'IHDR') {
				if (nLen !== 13) {
					throw bad('bad_png');
				}
				o.w = data.readUInt32BE(0);
				o.h = data.readUInt32BE(4);
				o.depth = data[8];
				o.ctype = data[9];
				o.interlace = data[12];
				if (data[10] !== 0 || data[11] !== 0) {
					throw bad('bad_png');
				}
			} else if (strType === 'PLTE') {
				o.plte = Buffer.from(data);
			} else if (strType === 'IDAT') {
				aIdat.push(data);
			} else if (strType === 'IEND') {
				bEnd = true;
				break;
			}
			pos += 12 + nLen;
		}
		if (!bEnd || !aIdat.length || !o.w || !o.h || o.w > MAX_SIDE * 8 || o.h > MAX_SIDE * 8) {
			throw bad('bad_png');
		}
		var nChan = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[o.ctype];
		if (!nChan || [1, 2, 4, 8, 16].indexOf(o.depth) < 0) {
			throw bad('bad_png');
		}
		var nBits = nChan * o.depth;
		o.bpp = Math.max(1, nBits >> 3);
		o.stride = Math.ceil(o.w * nBits / 8);
		try {
			o.raw = zlib.inflateSync(Buffer.concat(aIdat), { maxOutputLength: o.h * (o.stride + 1) * 2 + 4096 });
		} catch (e) {
			throw bad('bad_png');
		}
		if (!o.interlace && o.raw.length !== o.h * (o.stride + 1)) {
			throw bad('bad_png');
		}
		return o;
	}

	/* フィルタ 0〜4 を外す（bpp = 1 画素のバイト数） */
	function unfilter(raw, h, stride, bpp) {
		var out = Buffer.alloc(h * stride);
		for (var y = 0; y < h; y++) {
			var nFilter = raw[y * (stride + 1)];
			var nSrc = y * (stride + 1) + 1, nDst = y * stride, nUp = nDst - stride;
			if (nFilter > 4) {
				throw bad('bad_png');
			}
			for (var x = 0; x < stride; x++) {
				var v = raw[nSrc + x];
				var a = x >= bpp ? out[nDst + x - bpp] : 0;
				var b = y > 0 ? out[nUp + x] : 0;
				var c = (x >= bpp && y > 0) ? out[nUp + x - bpp] : 0;
				switch (nFilter) {
				case 1: v += a; break;
				case 2: v += b; break;
				case 3: v += (a + b) >> 1; break;
				case 4:
					var p = a + b - c, pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c);
					v += (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
					break;
				}
				out[nDst + x] = v & 255;
			}
		}
		return out;
	}

	/* 8bit パレット PNG（colortype 3・interlace なし）として読む。{w, h, plte, indices} */
	function readIndexed(buf) {
		var o = parsePng(buf);
		if (o.depth !== 8 || o.ctype !== 3 || o.interlace !== 0 || !o.plte || o.plte.length % 3 !== 0 || o.plte.length > 768) {
			throw bad('bad_png');
		}
		o.indices = unfilter(o.raw, o.h, o.stride, 1);
		return o;
	}

	/* PLTE をお題のパレット（[[r,g,b]...256]）と比べる（足りない分は黒扱い） */
	function samePalette(plte, palette) {
		for (var i = 0; i < 256; i++) {
			var c = palette[i];
			for (var k = 0; k < 3; k++) {
				var v = i * 3 + k < plte.length ? plte[i * 3 + k] : 0;
				if (v !== c[k]) {
					return false;
				}
			}
		}
		return true;
	}

	/* 案の PNG：お題と同じ大きさ・同じパレット・使える色だけ。Buffer を返す（保存はそのまま） */
	function checkWorkPng(data, task) {
		var buf = decodeB64(data, MAX_PNG);
		var o = readIndexed(buf);
		if (o.w !== task.w || o.h !== task.h) {
			throw bad('bad_size');
		}
		if (!samePalette(o.plte, task.palette)) {
			throw bad('bad_palette');
		}
		var ok = {};
		task.colors.forEach(function (n) { ok[n] = true; });
		for (var i = 0; i < o.indices.length; i++) {
			if (!ok[o.indices[i]]) {
				throw bad('bad_palette');
			}
		}
		return buf;
	}

	/* 基準・見本・背景：PNG として正しいこと（パレット PNG） */
	function checkPlainPng(data) {
		var buf = decodeB64(data, MAX_PNG);
		readIndexed(buf);
		return buf;
	}

	/* 提案のラフ絵：PNG ならパレットでも RGBA でもよい（256KB・512×512 まで） */
	function checkRoughPng(data) {
		var buf = decodeB64(data, MAX_PNG);
		var o = parsePng(buf);
		if (o.w > MAX_SIDE || o.h > MAX_SIDE) {
			throw bad('bad_size');
		}
		return buf;
	}

	/* 元イラスト：PNG か JPEG（1MB まで）。{buf, ext} */
	function checkRef(data, strType) {
		var buf = decodeB64(data, MAX_REF);
		if (strType === 'jpg' || strType === 'jpeg') {
			if (buf.length < 4 || buf[0] !== 0xff || buf[1] !== 0xd8 || buf[2] !== 0xff) {
				throw bad('bad_png');
			}
			return { buf: buf, ext: 'jpg' };
		}
		parsePng(buf);
		return { buf: buf, ext: 'png' };
	}

	/* ------------------------------------------------------------------ */
	/* HTTP の下請け															 */
	/* ------------------------------------------------------------------ */
	function reply(res, status, obj) {
		acc().reply(res, status, obj);
	}

	function readJson(req, nMaxBody, cb) {
		var aBuf = [], nSize = 0, bOver = false;
		req.on('data', function (buf) {
			nSize += buf.length;
			if (nSize > nMaxBody) {
				bOver = true;
				return;
			}
			aBuf.push(buf);
		});
		req.on('end', function () {
			if (bOver) {
				cb(null);
				return;
			}
			try {
				var obj = JSON.parse(Buffer.concat(aBuf).toString('utf8') || '{}');
				cb(obj && typeof obj === 'object' ? obj : null);
			} catch (e) {
				cb(null);
			}
		});
	}

	function str(v, nMax) {
		return String(v == null ? '' : v).slice(0, nMax || 64);
	}

	function int(v, nMin, nMax, nDef) {
		var n = Math.floor(Number(v));
		if (!isFinite(n)) {
			return nDef;
		}
		return Math.min(nMax, Math.max(nMin, n));
	}

	/* 本文をそろえる（改行は \n に、制御文字は除き、前後の空白・3 行以上の空行は詰める）。長すぎたら null */
	function cleanText(v, nMax) {
		var s = String(v == null ? '' : v).replace(/\r\n?/g, '\n')
			.replace(/[\u0000-\u0008\u000B-\u001F\u007F​-‏‪-‮⁦-⁩]/g, '')
			.replace(/\t/g, '    ')
			.replace(/\n{4,}/g, '\n\n\n')
			.trim();
		return Array.from(s).length > nMax ? null : s;
	}

	/* 1 行の文字（題名など。改行は空白に） */
	function cleanLine(v, nMax) {
		return cleanText(String(v == null ? '' : v).replace(/\n/g, ' '), nMax);
	}

	/* ------------------------------------------------------------------ */
	/* 見せ方																	 */
	/* ------------------------------------------------------------------ */
	/* キャラ名はいまの名前を見せる。キャラが消えていたら出した時の名前 */
	function nameOf(account, strStored, cache) {
		if (!(account in cache)) {
			var info = acc().accInfo(account);
			cache[account] = info && info.name ? info.name : '';
		}
		return cache[account] || strStored || '';
	}

	function cheerCount(workId) {
		var n = 0;
		store.cheers.forEach(function (c) {
			if (c.workId === workId) {
				n++;
			}
		});
		return n;
	}

	function commentCount(workId, bAll) {
		var n = 0;
		store.comments.forEach(function (c) {
			if (c.workId === workId && (bAll || (!c.hidden && !c.hiddenByAuthor))) {
				n++;
			}
		});
		return n;
	}

	function workView(w, strMyAcc, cache) {
		return {
			id: w.id, parent: w.parent || '',
			name: w.admin ? '管理人' : (w.anonymous ? '' : nameOf(w.account, w.name, cache)),
			admin: !!w.admin,
			anonymous: !!w.anonymous, note: w.note || '', status: w.status, share: !!w.share,
			img: w.img, cheers: cheerCount(w.id), comments: commentCount(w.id, false),
			at: w.at, updatedAt: w.updatedAt, postedAt: w.postedAt || 0,
			mine: !!(strMyAcc && w.account === strMyAcc)
		};
	}

	function adminWorkView(w, cache) {
		var o = workView(w, '', cache);
		o.name = w.admin ? '管理人' : nameOf(w.account, w.name, cache);
		o.account = w.account;
		o.postedName = w.name;
		o.hidden = !!w.hidden;
		o.deleted = !!w.deleted;
		o.comments = commentCount(w.id, true);
		delete o.mine;
		return o;
	}

	function commentView(c, strMyAcc, cache, bAuthor) {
		var o = {
			id: c.id, workId: c.workId,
			name: c.admin ? (c.name || '管理人') : (c.anonymous ? '' : nameOf(c.account, c.name, cache)),
			anonymous: !!c.anonymous, admin: !!c.admin, body: c.body, replyTo: c.replyTo || '',
			at: c.at, mine: !!(strMyAcc && !c.admin && c.account === strMyAcc)
		};
		if (c.hiddenByAuthor && bAuthor) {
			o.hiddenByAuthor = true;
		}
		return o;
	}

	function adminCommentView(c, cache) {
		var o = commentView(c, '', cache, true);
		o.name = c.admin ? (c.name || '管理人') : nameOf(c.account, c.name, cache);
		o.account = c.account;
		o.hidden = !!c.hidden;
		o.hiddenByAuthor = !!c.hiddenByAuthor;
		o.ip = c.ip || '';
		delete o.mine;
		return o;
	}

	function proposalView(p, cache, bAdmin) {
		var o = {
			id: p.id, kind: p.kind, body: p.body, img: p.img || '',
			name: (bAdmin || !p.anonymous) ? nameOf(p.account, p.name, cache) : '',
			anonymous: !!p.anonymous, status: p.status, reply: p.reply || '', replyImg: p.replyImg || '', replyAt: p.replyAt || 0,
			taskId: p.taskId || '', at: p.at, editedAt: p.editedAt || 0, imgAt: p.imgAt || p.at
		};
		if (bAdmin) {
			o.account = p.account;
			o.ip = p.ip || '';
		}
		return o;
	}

	function taskCounts(t) {
		var nWorks = 0, nWip = 0, nLatest = 0, strLatest = '', strAdopted = '';
		store.works.forEach(function (w) {
			if (w.taskId !== t.id || w.hidden || w.deleted) {
				return;
			}
			if (w.status === 'posted') {
				nWorks++;
				if ((w.postedAt || 0) >= nLatest) {
					nLatest = w.postedAt || 0;
					strLatest = w.img;
				}
			} else if (w.share) {
				nWip++;
			}
			if (t.adopted && t.adopted.workId === w.id) {
				strAdopted = w.img;
			}
		});
		return { works: nWorks, wip: nWip, latestImg: strLatest, adoptedImg: strAdopted };
	}

	function taskView(t) {
		return JSON.parse(JSON.stringify(t));
	}

	function taskListView(t) {
		var o = taskView(t), c = taskCounts(t);
		o.works = c.works;
		o.wip = c.wip;
		o.latestImg = c.latestImg;
		o.adoptedImg = c.adoptedImg;
		return o;
	}

	/* 一覧の並び：募集中→選考中→採用済み→隠す、中は新しい順 */
	function sortTasks(a) {
		var order = { open: 0, judging: 1, done: 2, hidden: 3 };
		return a.slice().sort(function (x, y) {
			return (order[x.state] - order[y.state]) || (y.at - x.at);
		});
	}

	/* ------------------------------------------------------------------ */
	/* 数え・制限																 */
	/* ------------------------------------------------------------------ */
	/* 時刻の一覧（mem）と保存データ（store）の多いほうで、since 以降の数と最後の時刻を返す */
	function usage(aMem, aStore, since) {
		var nMem = 0, nStore = 0, nLast = 0;
		(aMem || []).forEach(function (t) {
			if (t >= since) {
				nMem++;
			}
			nLast = Math.max(nLast, t);
		});
		aStore.forEach(function (t) {
			if (t >= since) {
				nStore++;
			}
			nLast = Math.max(nLast, t);
		});
		return { count: Math.max(nMem, nStore), last: nLast };
	}

	function atsOf(a, strAcc, fn) {
		var r = [];
		a.forEach(function (o) {
			if (o.account === strAcc && (!fn || fn(o))) {
				r.push(o.at);
			}
		});
		return r;
	}

	/* 古い時刻を捨ててから足す */
	function remember(map, key) {
		var now = Date.now();
		var a = (map[key] || []).filter(function (t) { return now - t < DAY; });
		a.push(now);
		map[key] = a;
	}

	/* 制限に掛かるなら {error, waitSeconds} を返す。大丈夫なら null */
	function limitError(aTimes, nInterval, nPerDay, nPerMinute) {
		var now = Date.now(), nDay = 0, nMin = 0, nLast = 0;
		aTimes.forEach(function (t) {
			if (now - t < DAY) {
				nDay++;
			}
			if (now - t < 60 * 1000) {
				nMin++;
			}
			nLast = Math.max(nLast, t);
		});
		if (nDay >= nPerDay) {
			return { error: 'too_many' };
		}
		if (nPerMinute && nMin >= nPerMinute) {
			return { error: 'wait', waitSeconds: 60 };
		}
		var nWait = nLast + nInterval - now;
		if (nInterval && nWait > 0) {
			return { error: 'wait', waitSeconds: Math.ceil(nWait / 1000) };
		}
		return null;
	}

	/* ------------------------------------------------------------------ */
	/* 共通の確認																 */
	/* ------------------------------------------------------------------ */
	/* 端末トークンの人を返す。だめなら返事をして null */
	function player(body, res, bNeedChar) {
		var info = acc().tokenInfo(str(body.token, 128));
		if (!info) {
			reply(res, 401, { error: 'invalid_token' });
			return null;
		}
		if (bNeedChar && !info.name) {
			reply(res, 200, { error: 'no_char' });
			return null;
		}
		return info;
	}

	function checkAdmin(body, res) {
		if (!acc().isAdminSession(body.session)) {
			reply(res, 401, { error: 'session' });
			return false;
		}
		return true;
	}

	function fail(res, e) {
		if (e && e.kobo) {
			reply(res, 200, { error: e.kobo });
			return true;
		}
		throw e;
	}

	/* 見える案か：消した・隠した案は見えない。作業中は見せているものか自分のものだけ */
	function workVisible(w, strMyAcc) {
		if (!w || w.deleted || w.hidden) {
			return false;
		}
		return w.status === 'posted' || !!w.share || (!!strMyAcc && w.account === strMyAcc);
	}

	/* ------------------------------------------------------------------ */
	/* プレイヤーの API														 */
	/* ------------------------------------------------------------------ */
	function apiMe(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		reply(res, 200, { account: info.account, name: info.name || '', canPost: !!info.name, admin: false });
	}

	function apiTasks(body, req, res) {
		load();
		var a = sortTasks(store.tasks.filter(function (t) { return t.state !== 'hidden'; }));
		/* 端末トークンがあれば、自分の作業中の案（募集中のお題のもの）も返す（トップの「続きを描く」用） */
		var info = body.token ? acc().tokenInfo(str(body.token, 128)) : null;
		var aWips = [];
		if (info && info.account) {
			var mapOpen = {};
			a.forEach(function (t) {
				if (t.state === 'open') {
					mapOpen[t.id] = true;
				}
			});
			store.works.forEach(function (w) {
				if (w.account === info.account && w.status === 'wip' && !w.deleted && mapOpen[w.taskId]) {
					aWips.push({ id: w.id, taskId: w.taskId, img: w.img, share: !!w.share, updatedAt: w.updatedAt || w.at });
				}
			});
			aWips.sort(function (x, y) { return y.updatedAt - x.updatedAt; });
		}
		reply(res, 200, { tasks: a.map(taskListView), myWips: aWips });
	}

	function apiTask(body, req, res) {
		load();
		var info = body.token ? acc().tokenInfo(str(body.token, 128)) : null;
		var strMy = info ? info.account : '';
		var t = findIn(store.tasks, str(body.id, 32));
		if (!t || t.state === 'hidden') {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var cache = {}, aWorks = [], mapVisible = {}, mapMineAuthor = {};
		var mapHasChild = {};
		store.works.forEach(function (w) {
			if (w.taskId === t.id && w.parent) {
				mapHasChild[w.parent] = true;
			}
		});
		store.works.forEach(function (w) {
			if (w.taskId !== t.id) {
				return;
			}
			if (w.deleted) {
				if (mapHasChild[w.id]) {
					aWorks.push({ id: w.id, parent: w.parent || '', deleted: true });
				}
				return;
			}
			if (!workVisible(w, strMy)) {
				return;
			}
			mapVisible[w.id] = true;
			mapMineAuthor[w.id] = !!strMy && w.account === strMy;
			aWorks.push(workView(w, strMy, cache));
		});
		var aComments = [];
		store.comments.forEach(function (c) {
			if (!mapVisible[c.workId] || c.hidden) {
				return;
			}
			if (c.hiddenByAuthor && !mapMineAuthor[c.workId]) {
				return;
			}
			aComments.push(commentView(c, strMy, cache, mapMineAuthor[c.workId]));
		});
		var aCheers = [];
		store.cheers.forEach(function (c) {
			if (strMy && c.account === strMy && mapVisible[c.workId]) {
				aCheers.push(c.workId);
			}
		});
		reply(res, 200, { task: taskView(t), works: aWorks, comments: aComments, myCheers: aCheers });
	}

	function apiSave(body, req, res) {
		var info = player(body, res, true);
		if (!info) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.taskId, 32));
		if (!t || t.state === 'hidden') {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (t.state !== 'open') {
			reply(res, 200, { error: 'closed' });
			return;
		}
		var w = null, parent = null;
		if (body.workId) {
			w = findIn(store.works, str(body.workId, 32));
			if (!w || w.account !== info.account || w.taskId !== t.id || w.deleted) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
			if (w.status !== 'wip') {
				reply(res, 200, { error: 'not_allowed' });
				return;
			}
		} else if (body.parent) {
			parent = findIn(store.works, str(body.parent, 32));
			if (!parent || parent.taskId !== t.id || parent.status !== 'posted' || parent.deleted || parent.hidden) {
				reply(res, 200, { error: 'not_allowed' });
				return;
			}
		}
		var note = cleanText(body.note, MAX_NOTE);
		if (note === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_NOTE });
			return;
		}
		var bSubmit = !!body.submit;
		if (bSubmit && body.agree !== true) {
			reply(res, 200, { error: 'agree' });
			return;
		}
		/* 制限 */
		var e;
		if (w) {
			e = limitError(memWip[info.account] || [], LIM.wipInterval, 1e9, 0);
		} else {
			var aAts = atsOf(store.works, info.account).concat(memNew[info.account] || []);
			e = limitError(aAts, LIM.newWorkInterval, LIM.newWorkPerDay, 0);
			if (!e) {
				var nWip = 0;
				store.works.forEach(function (x) {
					if (x.taskId === t.id && x.account === info.account && x.status === 'wip' && !x.deleted) {
						nWip++;
					}
				});
				if (!bSubmit && nWip >= LIM.wipPerTask) {
					e = { error: 'too_many', max: LIM.wipPerTask };
				}
			}
			if (!e && store.works.length >= MAX_WORKS) {
				e = { error: 'too_many' };
			}
		}
		if (e) {
			reply(res, 200, e);
			return;
		}
		var buf;
		try {
			buf = checkWorkPng(body.data, t);
		} catch (err) {
			fail(res, err);
			return;
		}
		var now = Date.now();
		var strImg = putImage(buf, 'png');
		if (w) {
			delImage(w.img);
			w.img = strImg;
			remember(memWip, info.account);
		} else {
			w = {
				id: newId(), taskId: t.id, parent: parent ? parent.id : '',
				account: info.account, name: info.name, anonymous: false, note: '', status: 'wip', share: false,
				img: strImg, hidden: false, deleted: false, agree: 0, at: now, updatedAt: now, postedAt: 0
			};
			store.works.push(w);
			remember(memNew, info.account);
		}
		w.name = info.name;
		w.anonymous = !!body.anonymous;
		w.note = note;
		w.share = !!body.share;
		w.updatedAt = now;
		if (bSubmit) {
			w.status = 'posted';
			w.share = true;
			w.postedAt = now;
			w.agree = AGREE_VERSION;
		}
		save();
		log((bSubmit ? '出した: ' : '作業中: ') + w.id + ' ' + t.id + ' ' + info.account + (w.parent ? ' parent=' + w.parent : ''));
		reply(res, 200, { ok: true, work: workView(w, info.account, {}) });
	}

	/* 案を完全に消す（絵・コメント・なじんでる・元が消した案だけで子がいなくなった親も） */
	function removeWork(w) {
		delImage(w.img);
		store.comments = store.comments.filter(function (c) { return c.workId !== w.id; });
		store.cheers = store.cheers.filter(function (c) { return c.workId !== w.id; });
		removeFrom(store.works, w);
		var p = w.parent ? findIn(store.works, w.parent) : null;
		if (p && p.deleted && !store.works.some(function (x) { return x.parent === p.id; })) {
			removeWork(p);
		}
	}

	function apiDelete(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		load();
		var w = findIn(store.works, str(body.id, 32));
		if (!w || w.account !== info.account || w.deleted) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (w.status !== 'wip') {
			reply(res, 200, { error: 'not_allowed' });
			return;
		}
		removeWork(w);
		save();
		log('本人が消した: ' + w.id + ' ' + info.account);
		reply(res, 200, { ok: true });
	}

	function apiComment(body, req, res) {
		var info = player(body, res, true);
		if (!info) {
			return;
		}
		load();
		var w = findIn(store.works, str(body.workId, 32));
		var t = w ? findIn(store.tasks, w.taskId) : null;
		if (!w || !t || t.state === 'hidden' || !workVisible(w, info.account)) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var text = cleanText(body.body, MAX_COMMENT);
		if (text === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_COMMENT });
			return;
		}
		if (!text) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		var replyTo = '';
		if (body.replyTo) {
			var rc = findIn(store.comments, str(body.replyTo, 32));
			if (!rc || rc.workId !== w.id) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
			if (w.account !== info.account) {
				reply(res, 200, { error: 'not_allowed' });		/* 返事できるのは案の本人と管理者だけ */
				return;
			}
			replyTo = rc.id;
		}
		var e = limitError(atsOf(store.comments, info.account, function (c) { return !c.admin; }),
			0, LIM.commentPerDay, LIM.commentPerMinute);
		if (!e && store.comments.length >= MAX_COMMENTS) {
			e = { error: 'too_many' };
		}
		if (e) {
			reply(res, 200, e);
			return;
		}
		var c = {
			id: newId(), workId: w.id, account: info.account, name: info.name,
			anonymous: !!body.anonymous, admin: false, body: text, replyTo: replyTo,
			hiddenByAuthor: false, hidden: false, at: Date.now(), ip: acc().clientIp(req)
		};
		store.comments.push(c);
		save();
		log('コメント: ' + c.id + ' → ' + w.id + ' ' + info.account);
		reply(res, 200, { ok: true, comment: commentView(c, info.account, {}, w.account === info.account) });
	}

	function apiHideComment(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		load();
		var c = findIn(store.comments, str(body.id, 32));
		var w = c ? findIn(store.works, c.workId) : null;
		if (!c || !w || w.account !== info.account) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		c.hiddenByAuthor = !!body.hide;
		save();
		log('本人がコメントを' + (c.hiddenByAuthor ? '隠した' : '戻した') + ': ' + c.id);
		reply(res, 200, { ok: true });
	}

	function apiCheer(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		load();
		var w = findIn(store.works, str(body.workId, 32));
		var t = w ? findIn(store.tasks, w.taskId) : null;
		if (!w || !t || t.state === 'hidden' || !workVisible(w, info.account)) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (w.account === info.account) {
			reply(res, 200, { error: 'not_allowed' });		/* 自分の案には付けられない */
			return;
		}
		var cur = null;
		store.cheers.forEach(function (c) {
			if (c.workId === w.id && c.account === info.account) {
				cur = c;
			}
		});
		if (body.on && !cur) {
			if (store.cheers.length >= MAX_CHEERS) {
				reply(res, 200, { error: 'too_many' });
				return;
			}
			store.cheers.push({ workId: w.id, account: info.account, at: Date.now() });
			save();
		} else if (!body.on && cur) {
			removeFrom(store.cheers, cur);
			save();
		}
		reply(res, 200, { ok: true, cheers: cheerCount(w.id) });
	}

	function apiPropose(body, req, res) {
		var info = player(body, res, true);
		if (!info) {
			return;
		}
		load();
		var text = cleanText(body.body, MAX_PROPOSAL);
		if (text === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_PROPOSAL });
			return;
		}
		if (!text) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		if (body.agree !== true) {
			reply(res, 200, { error: 'agree' });
			return;
		}
		pruneWithdrawn();
		/* 取り下げた提案も 1 日の回数には数える（出しては取り下げるのくり返しを防ぐ） */
		var e = limitError(atsOf(store.proposals, info.account), 0, LIM.proposalPerDay, 0);
		if (!e && store.proposals.length >= MAX_PROPOSALS) {
			e = { error: 'too_many' };
		}
		if (e) {
			reply(res, 200, e);
			return;
		}
		var strImg = '';
		if (body.data) {
			try {
				strImg = putImage(checkRoughPng(body.data), 'png');
			} catch (err) {
				fail(res, err);
				return;
			}
		}
		var p = {
			id: newId(), account: info.account, name: info.name, anonymous: !!body.anonymous,
			kind: KINDS.indexOf(body.kind) >= 0 ? body.kind : 'char', body: text, img: strImg,
			status: 'new', reply: '', replyAt: 0, taskId: '', at: Date.now(), ip: acc().clientIp(req)
		};
		store.proposals.push(p);
		save();
		log('提案: ' + p.id + ' ' + info.account);
		reply(res, 200, { ok: true, proposal: proposalView(p, {}, false) });
	}

	/* 取り下げた提案は 1 日たったら消す（それまでは 1 日の回数に数えるため残す） */
	function pruneWithdrawn() {
		var now = Date.now();
		store.proposals = store.proposals.filter(function (p) {
			return p.status !== 'withdrawn' || now - (p.withdrawnAt || p.at) < DAY;
		});
	}

	/* 本人の提案を探す。お題になった・取り下げた提案は変えられない */
	function myProposal(body, res, info) {
		var p = findIn(store.proposals, str(body.id, 32));
		if (!p || p.account !== info.account || p.status === 'withdrawn') {
			reply(res, 404, { error: 'not_found' });
			return null;
		}
		if (p.status === 'tasked') {
			reply(res, 200, { error: 'tasked' });
			return null;
		}
		return p;
	}

	/* 自分の提案を直す。data でラフ絵を差し替え、removeImg で外す（どちらも無ければそのまま）。
	   見送りになった提案を直すと、もう一度見てもらえるよう「新着」に戻す */
	function apiProposalEdit(body, req, res) {
		var info = player(body, res, true);
		if (!info) {
			return;
		}
		load();
		var p = myProposal(body, res, info);
		if (!p) {
			return;
		}
		var text = cleanText(body.body, MAX_PROPOSAL);
		if (text === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_PROPOSAL });
			return;
		}
		if (!text) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		if (body.agree !== true) {
			reply(res, 200, { error: 'agree' });
			return;
		}
		var strImg = p.img || '';
		if (body.data) {
			try {
				strImg = putImage(checkRoughPng(body.data), 'png');
			} catch (err) {
				fail(res, err);
				return;
			}
		} else if (body.removeImg) {
			strImg = '';
		}
		if (strImg !== p.img) {
			delImage(p.img);
			p.imgAt = Date.now();		/* ラフ絵を変えた時刻（返事の絵より新しければ、管理人はこちらに描き足す） */
		}
		p.img = strImg;
		p.body = text;
		if (KINDS.indexOf(body.kind) >= 0) {
			p.kind = body.kind;
		}
		p.anonymous = !!body.anonymous;
		p.name = info.name;
		p.editedAt = Date.now();
		if (p.status === 'declined') {
			p.status = 'new';
		}
		save();
		log('提案を直した: ' + p.id + ' ' + info.account);
		reply(res, 200, { ok: true, proposal: proposalView(p, {}, false) });
	}

	/* 自分の提案を取り下げる。中身とラフ絵は消し、1 日の回数のための印だけ残す */
	function apiProposalWithdraw(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		load();
		var p = myProposal(body, res, info);
		if (!p) {
			return;
		}
		delImage(p.img);
		delImage(p.replyImg);
		p.img = '';
		p.replyImg = '';
		p.body = '';
		p.reply = '';
		p.status = 'withdrawn';
		p.withdrawnAt = Date.now();
		save();
		log('提案を取り下げた: ' + p.id + ' ' + info.account);
		reply(res, 200, { ok: true });
	}

	function apiMyProposals(body, req, res) {
		var info = player(body, res, false);
		if (!info) {
			return;
		}
		load();
		var cache = {};
		var a = store.proposals.filter(function (p) { return p.account === info.account && p.status !== 'withdrawn'; }).reverse();
		reply(res, 200, {
			proposals: a.map(function (p) {
				var o = proposalView(p, cache, true);		/* 本人には匿名でも自分の名前が見える */
				delete o.account;
				delete o.ip;
				return o;
			})
		});
	}

	/* ------------------------------------------------------------------ */
	/* 管理者の API（ログインは /api/admin/login と同じ）							 */
	/* ------------------------------------------------------------------ */
	function apiAdminTasks(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var nNew = store.proposals.filter(function (p) { return p.status === 'new'; }).length;
		var nHidden = store.comments.filter(function (c) { return c.hiddenByAuthor && !c.hidden; }).length;
		reply(res, 200, {
			tasks: sortTasks(store.tasks).map(taskListView),
			counts: { proposals: nNew, hiddenComments: nHidden }
		});
	}

	function apiAdminTask(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.id, 32));
		if (!t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var cache = {}, mapWork = {};
		var aWorks = store.works.filter(function (w) { return w.taskId === t.id; });
		aWorks.forEach(function (w) { mapWork[w.id] = true; });
		reply(res, 200, {
			task: taskView(t),
			works: aWorks.map(function (w) { return adminWorkView(w, cache); }),
			comments: store.comments.filter(function (c) { return mapWork[c.workId]; })
				.map(function (c) { return adminCommentView(c, cache); })
		});
	}

	/* お題の画像を入れ替える（古いほうは消す）。値が '' なら外す */
	function setImg(t, key, strNew) {
		if (t[key] && t[key] !== strNew) {
			delImage(t[key]);
		}
		t[key] = strNew;
	}

	function validPalette(a) {
		if (!Array.isArray(a) || a.length < 1 || a.length > 256) {
			return null;
		}
		var r = [];
		for (var i = 0; i < 256; i++) {
			var c = a[i] || [0, 0, 0];
			if (!Array.isArray(c) || c.length < 3) {
				return null;
			}
			r.push([int(c[0], 0, 255, 0), int(c[1], 0, 255, 0), int(c[2], 0, 255, 0)]);
		}
		return r;
	}

	function apiAdminTaskSave(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = body.id ? findIn(store.tasks, str(body.id, 32)) : null;
		if (body.id && !t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (!t && store.tasks.length >= MAX_TASKS) {
			reply(res, 200, { error: 'too_many' });
			return;
		}
		var title = cleanLine(body.title, MAX_TITLE);
		var desc = cleanText(body.desc, MAX_DESC);
		if (title === null || desc === null) {
			reply(res, 200, { error: 'long' });
			return;
		}
		if (!title) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		var sheet = str(body.sheet, 100);
		if (!/\.png$/i.test(sheet) || /[\/\\]/.test(sheet) || sheet.indexOf('..') >= 0) {
			reply(res, 200, { error: 'bad_sheet' });
			return;
		}
		var x = int(body.x, 0, 16384, 0), y = int(body.y, 0, 16384, 0);
		var w = int(body.w, 1, MAX_SIDE, 0), h = int(body.h, 1, MAX_SIDE, 0);
		if (!w || !h) {
			reply(res, 200, { error: 'bad_size' });
			return;
		}
		var palette = validPalette(body.palette);
		if (!palette) {
			reply(res, 200, { error: 'bad_palette' });
			return;
		}
		var colors = [];
		(Array.isArray(body.colors) ? body.colors : []).forEach(function (n) {
			n = Math.floor(Number(n));
			if (n >= 0 && n < 256 && colors.indexOf(n) < 0) {
				colors.push(n);
			}
		});
		var nTrans = int(body.transparent, 0, 255, 0);
		if (colors.indexOf(nTrans) < 0) {
			colors.push(nTrans);
		}
		colors.sort(function (a, b) { return a - b; });
		/* 見本の色（描く画面で先頭に並べる目安。使える色の中だけ） */
		var sampleColors = [];
		(Array.isArray(body.sampleColors) ? body.sampleColors : []).forEach(function (n) {
			n = Math.floor(Number(n));
			if (colors.indexOf(n) >= 0 && sampleColors.indexOf(n) < 0) {
				sampleColors.push(n);
			}
		});
		sampleColors.sort(function (a, b) { return a - b; });
		/* 画像（先に全部確かめてから書く） */
		var baseBuf = null, bgBuf = null, ref = null, aSamples = null;
		/* fresh = 新しい絵として描く（ゲーム画像のどこに置くかは採用後に管理者が決める）。sheet はパレットと見本を借りるだけ */
		var bFresh = !!body.fresh;
		var bBg = body.bgData !== undefined && body.bgData !== null;
		var bRef = body.refData !== undefined && body.refData !== null;
		try {
			if (bFresh) {
				/* 新しく描くお題（白紙から）。「今の絵」は持たない */
			} else if (body.baseData) {
				baseBuf = checkPlainPng(body.baseData);
			} else if (!t || !t.base) {
				throw bad('bad_png');
			}
			if (bBg && body.bgData) {
				bgBuf = checkPlainPng(body.bgData);
			}
			if (bRef && body.refData) {
				ref = checkRef(body.refData, str(body.refType, 8));
			}
			if (Array.isArray(body.samplesData)) {
				if (body.samplesData.length > 4) {
					throw bad('too_many');
				}
				aSamples = body.samplesData.map(checkPlainPng);
			}
		} catch (err) {
			fail(res, err);
			return;
		}
		var prop = null;
		if (body.proposalId) {
			prop = findIn(store.proposals, str(body.proposalId, 32));
			if (!prop || prop.status === 'withdrawn') {
				reply(res, 404, { error: 'not_found' });
				return;
			}
		}
		var now = Date.now(), bNew = !t;
		if (bNew) {
			t = { id: newId(), base: '', bg: '', ref: '', samples: [], proposalId: '', adopted: null, at: now };
			store.tasks.push(t);
		}
		t.title = title;
		t.desc = desc;
		t.kind = KINDS.indexOf(body.kind) >= 0 ? body.kind : (t.kind || 'char');
		t.state = STATES.indexOf(body.state) >= 0 ? body.state : (t.state || 'open');
		t.sheet = sheet;
		t.x = x; t.y = y; t.w = w; t.h = h;
		t.cw = int(body.cw, 1, MAX_SIDE, w);
		t.ch = int(body.ch, 1, MAX_SIDE, h);
		t.preview = PREVIEWS.indexOf(body.preview) >= 0 ? body.preview : (t.preview || 'tile');
		t.frameMs = int(body.frameMs, 30, 10000, 200);
		t.palette = palette;
		t.colors = colors;
		t.sampleColors = sampleColors;
		t.transparent = nTrans;
		t.fresh = bFresh;
		if (bFresh) {
			if (t.base) {
				setImg(t, 'base', '');
			}
		} else if (baseBuf) {
			setImg(t, 'base', putImage(baseBuf, 'png'));
		}
		if (bBg) {
			setImg(t, 'bg', bgBuf ? putImage(bgBuf, 'png') : '');
		}
		if (bRef) {
			setImg(t, 'ref', ref ? putImage(ref.buf, ref.ext) : '');
		}
		if (aSamples) {
			(t.samples || []).forEach(delImage);
			t.samples = aSamples.map(function (b) { return putImage(b, 'png'); });
		}
		if (prop) {
			t.proposalId = prop.id;
			prop.status = 'tasked';
			prop.taskId = t.id;
		}
		t.updatedAt = now;
		save();
		log('お題を' + (bNew ? '作った' : '更新した') + ': ' + t.id + ' ' + t.title + '（' + t.state + '）');
		reply(res, 200, { ok: true, task: taskView(t) });
	}

	function apiAdminTaskDelete(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.id, 32));
		if (!t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var mapWork = {};
		store.works = store.works.filter(function (w) {
			if (w.taskId !== t.id) {
				return true;
			}
			mapWork[w.id] = true;
			delImage(w.img);
			return false;
		});
		store.comments = store.comments.filter(function (c) { return !mapWork[c.workId]; });
		store.cheers = store.cheers.filter(function (c) { return !mapWork[c.workId]; });
		[t.base, t.bg, t.ref].concat(t.samples || []).forEach(delImage);
		store.proposals.forEach(function (p) {
			if (p.taskId === t.id) {
				p.taskId = '';
				p.status = 'later';
			}
		});
		removeFrom(store.tasks, t);
		save();
		log('お題を消した: ' + t.id);
		reply(res, 200, { ok: true });
	}

	function apiAdminWork(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var w = findIn(store.works, str(body.id, 32));
		if (!w) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (body.hidden != null) {
			w.hidden = !!body.hidden;
		}
		if (body.deleted) {
			var t = findIn(store.tasks, w.taskId);
			if (t && t.adopted && t.adopted.workId === w.id) {
				t.adopted = null;
				t.state = 'judging';
			}
			if (store.works.some(function (x) { return x.parent === w.id; })) {
				delImage(w.img);		/* 子がいる：系譜を保つため印だけ残す */
				w.img = '';
				w.deleted = true;
				w.hidden = true;
			} else {
				removeWork(w);
			}
		}
		save();
		log('管理者が案を更新: ' + w.id + (body.hidden != null ? (body.hidden ? ' 隠す' : ' 隠すのをやめる') : '') + (body.deleted ? ' 消す' : ''));
		reply(res, 200, { ok: true });
	}

	function apiAdminComment(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var w = findIn(store.works, str(body.workId, 32));
		if (!w) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var text = cleanText(body.body, MAX_ADMIN_COMMENT);
		if (text === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_ADMIN_COMMENT });
			return;
		}
		if (!text) {
			reply(res, 200, { error: 'empty' });
			return;
		}
		var replyTo = '';
		if (body.replyTo) {
			var rc = findIn(store.comments, str(body.replyTo, 32));
			if (!rc || rc.workId !== w.id) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
			replyTo = rc.id;
		}
		if (store.comments.length >= MAX_COMMENTS) {
			reply(res, 200, { error: 'too_many' });
			return;
		}
		var c = {
			id: newId(), workId: w.id, account: '', name: '管理人', anonymous: false, admin: true,
			body: text, replyTo: replyTo, hiddenByAuthor: false, hidden: false, at: Date.now(), ip: acc().clientIp(req)
		};
		store.comments.push(c);
		save();
		log('管理者のコメント: ' + c.id + ' → ' + w.id);
		reply(res, 200, { ok: true, comment: adminCommentView(c, {}) });
	}

	function apiAdminCommentUpdate(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var c = findIn(store.comments, str(body.id, 32));
		if (!c) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (body.delete) {
			/* 返事も一緒に消す */
			var mapGone = {};
			mapGone[c.id] = true;
			var bMore = true;
			while (bMore) {
				bMore = false;
				store.comments.forEach(function (x) {
					if (!mapGone[x.id] && x.replyTo && mapGone[x.replyTo]) {
						mapGone[x.id] = true;
						bMore = true;
					}
				});
			}
			store.comments = store.comments.filter(function (x) { return !mapGone[x.id]; });
		} else if (body.hidden != null) {
			c.hidden = !!body.hidden;
		}
		save();
		log('管理者がコメントを更新: ' + c.id + (body.delete ? ' 消す' : (body.hidden != null ? (c.hidden ? ' 隠す' : ' 隠すのをやめる') : '')));
		reply(res, 200, { ok: true });
	}

	/* 協力者：採用した案から parent をたどった全部の案の人（同じ人は 1 回）＋提案の人 */
	function makeCredits(w, t) {
		var cache = {}, credits = [], mapSeen = {}, nGuard = 0;
		for (var cur = w; cur && nGuard < 1000; cur = cur.parent ? findIn(store.works, cur.parent) : null, nGuard++) {
			/* 管理人が直した版（作監修正）は協力者に入れない */
			if (cur.admin || mapSeen[cur.account]) {
				continue;
			}
			mapSeen[cur.account] = true;
			credits.push({
				role: credits.length ? 'base' : 'draw',
				name: cur.anonymous ? '' : nameOf(cur.account, cur.name, cache),
				anonymous: !!cur.anonymous
			});
		}
		var p = t.proposalId ? findIn(store.proposals, t.proposalId) : null;
		if (p) {
			credits.push({ role: 'proposal', name: p.anonymous ? '' : nameOf(p.account, p.name, cache), anonymous: !!p.anonymous });
		}
		return credits;
	}

	/* お題の見本の色（見本と、描き直しなら今の絵で使っている色。使える色の中だけ）を作り直す */
	function rebuildSampleColors(t) {
		var used = {};
		var ids = (t.samples || []).slice();
		if (!t.fresh && t.base) {
			ids.push(t.base);
		}
		ids.forEach(function (id) {
			try {
				var o = readIndexed(fs.readFileSync(path.join(strImgDir, id + '.png')));
				for (var i = 0; i < o.indices.length; i++) {
					used[o.indices[i]] = true;
				}
			} catch (e) { /* 読めない絵は飛ばす */ }
		});
		t.sampleColors = t.colors.filter(function (n) { return used[n] && n !== t.transparent; });
	}

	/* お題の絵を管理画面で直す（slot: 'base' 今の絵 / 'sample' 見本 index 番目）。大きさ・パレットはお題と同じ */
	function apiAdminTaskImage(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.taskId, 32));
		if (!t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var nIndex = Number(body.index) || 0;
		if (body.slot === 'base') {
			if (t.fresh) {
				reply(res, 200, { error: 'not_allowed' });
				return;
			}
		} else if (body.slot === 'sample') {
			if (!t.samples || nIndex < 0 || nIndex >= t.samples.length) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
		} else {
			reply(res, 200, { error: 'not_allowed' });
			return;
		}
		var buf, o;
		try {
			buf = decodeB64(body.data, MAX_PNG);
			o = readIndexed(buf);
		} catch (err) {
			fail(res, err);
			return;
		}
		if (o.w !== t.w || o.h !== t.h) {
			reply(res, 200, { error: 'bad_size' });
			return;
		}
		if (!samePalette(o.plte, t.palette)) {
			reply(res, 200, { error: 'bad_palette' });
			return;
		}
		var strImg = putImage(buf, 'png');
		if (body.slot === 'base') {
			setImg(t, 'base', strImg);
		} else {
			delImage(t.samples[nIndex]);
			t.samples[nIndex] = strImg;
		}
		rebuildSampleColors(t);
		t.updatedAt = Date.now();
		save();
		log('管理者がお題の絵を直した: ' + t.id + ' ' + body.slot + (body.slot === 'sample' ? ' ' + nIndex : ''));
		reply(res, 200, { ok: true, task: taskView(t) });
	}

	/* 管理人が案に描き足す（作監修正）。元の案の子として「管理人」の案を出す。ひとことは直したところ */
	function apiAdminFix(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.taskId, 32));
		if (!t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		var parent = null;
		if (body.parent) {
			parent = findIn(store.works, str(body.parent, 32));
			if (!parent || parent.taskId !== t.id || parent.deleted) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
		}
		var note = cleanText(body.note, MAX_NOTE);
		if (note === null) {
			reply(res, 200, { error: 'long', maxLength: MAX_NOTE });
			return;
		}
		if (store.works.length >= MAX_WORKS) {
			reply(res, 200, { error: 'too_many' });
			return;
		}
		var buf;
		try {
			buf = checkWorkPng(body.data, t);
		} catch (err) {
			fail(res, err);
			return;
		}
		var now = Date.now();
		var w = {
			id: newId(), taskId: t.id, parent: parent ? parent.id : '', account: '', name: '管理人', admin: true,
			anonymous: false, note: note, status: 'posted', share: true, img: putImage(buf, 'png'),
			hidden: false, deleted: false, at: now, updatedAt: now, postedAt: now, ip: acc().clientIp(req)
		};
		store.works.push(w);
		save();
		log('管理人の修正: ' + w.id + ' → ' + (parent ? parent.id : '(根)'));
		reply(res, 200, { ok: true, work: adminWorkView(w, {}) });
	}

	function apiAdminAdopt(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var t = findIn(store.tasks, str(body.taskId, 32));
		if (!t) {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (!body.workId) {
			t.adopted = null;
			if (t.state === 'done') {
				t.state = 'judging';
			}
		} else {
			var w = findIn(store.works, str(body.workId, 32));
			if (!w || w.taskId !== t.id) {
				reply(res, 404, { error: 'not_found' });
				return;
			}
			if (w.status !== 'posted' || w.deleted || w.hidden) {
				reply(res, 200, { error: 'not_allowed' });
				return;
			}
			t.adopted = { workId: w.id, at: Date.now(), credits: makeCredits(w, t) };
			t.state = 'done';
		}
		t.updatedAt = Date.now();
		save();
		log('採用: ' + t.id + ' ' + (body.workId || '（取り消し）'));
		reply(res, 200, { ok: true, task: taskView(t) });
	}

	function apiAdminProposals(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var cache = {};
		var a = store.proposals.filter(function (p) {
			return p.status !== 'withdrawn' && (!body.status || p.status === body.status);
		}).reverse();
		reply(res, 200, { proposals: a.map(function (p) { return proposalView(p, cache, true); }) });
	}

	function apiAdminProposalUpdate(body, req, res) {
		if (!checkAdmin(body, res)) {
			return;
		}
		load();
		var p = findIn(store.proposals, str(body.id, 32));
		if (!p || p.status === 'withdrawn') {
			reply(res, 404, { error: 'not_found' });
			return;
		}
		if (body.status != null) {
			if (PROPOSAL_STATUS.indexOf(body.status) < 0) {
				reply(res, 200, { error: 'not_allowed' });
				return;
			}
			p.status = body.status;
		}
		if (body.reply != null) {
			var text = cleanText(body.reply, MAX_REPLY);
			if (text === null) {
				reply(res, 200, { error: 'long', maxLength: MAX_REPLY });
				return;
			}
			p.reply = text;
			p.replyAt = text ? Date.now() : 0;
		}
		/* 返事に付ける絵（管理人が描き足したもの）。replyData で入れ替え、replyImg: '' で外す */
		if (body.replyData) {
			var strReplyImg;
			try {
				strReplyImg = putImage(checkRoughPng(body.replyData), 'png');
			} catch (err) {
				fail(res, err);
				return;
			}
			delImage(p.replyImg);
			p.replyImg = strReplyImg;
			p.replyAt = Date.now();
		} else if (body.replyImg === '') {
			delImage(p.replyImg);
			p.replyImg = '';
		}
		save();
		log('管理者が提案を更新: ' + p.id + ' ' + p.status);
		reply(res, 200, { ok: true, proposal: proposalView(p, {}, true) });
	}

	var API = {
		'/api/kobo/me': apiMe,
		'/api/kobo/tasks': apiTasks,
		'/api/kobo/task': apiTask,
		'/api/kobo/save': apiSave,
		'/api/kobo/delete': apiDelete,
		'/api/kobo/comment': apiComment,
		'/api/kobo/hide-comment': apiHideComment,
		'/api/kobo/cheer': apiCheer,
		'/api/kobo/propose': apiPropose,
		'/api/kobo/my-proposals': apiMyProposals,
		'/api/kobo/proposal-edit': apiProposalEdit,
		'/api/kobo/proposal-withdraw': apiProposalWithdraw,
		'/api/admin/kobo/tasks': apiAdminTasks,
		'/api/admin/kobo/task': apiAdminTask,
		'/api/admin/kobo/task-save': apiAdminTaskSave,
		'/api/admin/kobo/task-delete': apiAdminTaskDelete,
		'/api/admin/kobo/work': apiAdminWork,
		'/api/admin/kobo/comment': apiAdminComment,
		'/api/admin/kobo/comment-update': apiAdminCommentUpdate,
		'/api/admin/kobo/adopt': apiAdminAdopt,
		'/api/admin/kobo/proposals': apiAdminProposals,
		'/api/admin/kobo/proposal-update': apiAdminProposalUpdate,
		'/api/admin/kobo/task-image': apiAdminTaskImage,
		'/api/admin/kobo/fix': apiAdminFix
	};

	/* GET /kobo-img/<id>.png|.jpg */
	function serveImage(req, res, url) {
		var m = /^\/kobo-img\/([0-9a-f]{12})\.(png|jpg)$/.exec(url);
		if (!m || (req.method !== 'GET' && req.method !== 'HEAD')) {
			res.writeHead(m ? 405 : 404, { 'Content-Type': 'text/plain; charset=utf-8' });
			res.end(m ? 'method' : 'not found');
			return;
		}
		load();
		/* 画面は id だけで呼ぶので、.png で頼まれて無ければ .jpg（元イラストの JPEG）も探す（逆も） */
		var strExt = m[2];
		fs.readFile(path.join(strImgDir, m[1] + '.' + strExt), function onRead(err, buf) {
			if (err && strExt === m[2]) {
				strExt = m[2] === 'png' ? 'jpg' : 'png';
				fs.readFile(path.join(strImgDir, m[1] + '.' + strExt), onRead);
				return;
			}
			if (err) {
				res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
				res.end('not found');
				return;
			}
			res.writeHead(200, {
				'Content-Type': strExt === 'png' ? 'image/png' : 'image/jpeg',
				'Content-Length': buf.length,
				'Cache-Control': 'public, max-age=31536000, immutable'
			});
			res.end(req.method === 'HEAD' ? undefined : buf);
		});
	}

	/* sbosvr_net.js の HTTP から呼ばれる。ここの API と画像だけ受け持つ（ほかは次へ） */
	function handleHttp(req, res, url) {
		if (url.indexOf('/kobo-img/') === 0) {
			serveImage(req, res, url);
			return true;
		}
		var fn = API[url];
		if (!fn || !acc()) {
			return false;
		}
		if (req.method !== 'POST') {
			reply(res, 405, { error: 'method' });
			return true;
		}
		var bAdmin = url.indexOf('/api/admin/') === 0;
		readJson(req, bAdmin ? MAX_BODY_ADMIN : MAX_BODY, function (body) {
			if (!body) {
				reply(res, 400, { error: 'json' });
				return;
			}
			if (bAdmin) {
				acc().withAdminCookie(body, req);		/* 管理ページのログインは Cookie */
			}
			try {
				fn(body, req, res);
			} catch (e) {
				console.error('[kobo] ' + url + ' で失敗: ' + (e && e.stack || e));
				reply(res, 500, { error: 'internal' });
			}
		});
		return true;
	}

	return {
		handleHttp: handleHttp
	};
})();
