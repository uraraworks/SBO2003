/* ========================================================================= */
/* admin-kobo.js															 */
/* 管理ページ（/admin）の「ドット絵工房」									 */
/* ========================================================================= */
/*
 * プレイヤーがみんなでゲームのドット絵を描き合う仕組み（仕様は docs/kobo.md）の管理者側。
 *   ・お題の一覧／作る・直す／詳しい画面（案の系譜・コメント・採用）／提案の一覧
 *   ・API は /api/admin/kobo/*（server/node/sbosvr_kobo.js）。session は admin.html の api() が付ける。
 *   ・絵はすべて 8bit パレット PNG。パレットは元のゲーム画像（/admin-grp/<名前>）のものをそのまま使う。
 *     読み書きは dot-png.js、透明色の判定は dot-core.js。
 * ES module。admin.html が openKobo() で動的に import し、open(api, onBadge) を呼ぶ。
 */
import { decodeIndexedPng, encodeIndexedPng } from './dot-png.js';
import { findTransparentIndex, guessCellSize, normalizePalette, palettesEqual } from './dot-core.js';

const KINDS = { char: 'キャラ（NPC・敵）', part: '背景パーツ', effect: 'エフェクト' };
const PREVIEWS = { walk: '歩き（向き4×2コマ）', tile: 'タイル（3×3に並べる）', anim: 'アニメ（順に再生）' };
const PREVIEW_OF_KIND = { char: 'walk', part: 'tile', effect: 'anim' };
const STATES = { open: '募集中', judging: '選考中', done: '採用済み', hidden: '隠す（管理者だけ）' };
const PSTATES = { new: '新着', later: '後で', declined: '見送り', tasked: 'お題にした' };
const ROLES = { draw: '描いた人', base: '元にした案の人', proposal: '提案した人' };
const ERRORS = {
	session: 'ログインの期限が切れました。', not_found: '見つかりません（消されたかもしれません）。',
	bad_png: 'PNG が正しくありません。', bad_size: '絵の大きさが合いません。', bad_palette: 'パレットが合いません。',
	long: '長すぎます。', empty: '空です。', too_many: '多すぎます。', not_allowed: 'できません。'
};
/* 送る JSON の上限（サーバーは 1MB まで）。base64 で 4/3 倍になるので、元イラストはその分を見込んで縮める */
const MAX_JSON = 1000000;
const MAX_REF_BYTES = 560000;
const BG_CELL = 16;

let api = null;			/* function (url, body) → Promise */
let onBadge = null;		/* function (件数) */
let root = null;		/* section#tab-kobo 内の描画先 */
let view = { name: 'list' };
let counts = { proposals: 0, hiddenComments: 0 };
let gridList = null;	/* /admin-grp/index.json の files */
const sheetCache = {};	/* 名前 → デコードした画像 */
let renderSeq = 0;		/* 古い読み込みの結果を捨てる印 */

/* ---------- 小道具 ---------- */
function h(tag, attrs, ...kids) {
	const e = document.createElement(tag);
	if (attrs) {
		for (const k of Object.keys(attrs)) {
			const v = attrs[k];
			if (v == null || v === false) continue;
			if (k === 'class') e.className = v;
			else if (k === 'text') e.textContent = v;
			else if (k === 'on') { for (const ev of Object.keys(v)) e.addEventListener(ev, v[ev]); }
			else if (k === 'style') e.style.cssText = v;
			else if (k in e && k !== 'list') e[k] = v;
			else e.setAttribute(k, v === true ? '' : v);
		}
	}
	for (const c of kids) {
		if (c == null || c === false) continue;
		e.appendChild(typeof c === 'string' ? document.createTextNode(c) : c);
	}
	return e;
}
function clear(e) { while (e.firstChild) e.removeChild(e.firstChild); }
function dateText(ms) {
	return ms ? new Date(ms).toLocaleString('ja-JP', { year: 'numeric', month: 'numeric', day: 'numeric', hour: '2-digit', minute: '2-digit' }) : '';
}
function errText(r) {
	if (!r) return 'つながりませんでした。';
	return (ERRORS[r.error] || r.message || r.error || ('HTTP ' + r.httpStatus)) + (r.error ? '（' + r.error + '）' : '');
}
function toB64(bytes) {
	let s = '';
	for (let i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
	return btoa(s);
}
function download(bytes, name, type) {
	const a = h('a', { href: URL.createObjectURL(new Blob([bytes], { type })), download: name });
	document.body.appendChild(a);
	a.click();
	a.remove();
	setTimeout(() => URL.revokeObjectURL(a.href), 15000);
}
function tag(text, cls) { return h('span', { class: 'tag ' + (cls || ''), text }); }
function msgBox() { return h('div', { class: 'msg' }); }
function setMsg(e, text, ok) { e.textContent = text || ''; e.classList.toggle('ok', !!ok); }

/* 画像（/kobo-img/<id>.png。元イラストだけ .jpg の時もあるので、png で読めなければ jpg を試す）。
   小さい絵は整数倍に拡大して、ぼやけさせない（image-rendering: pixelated） */
function imgUrl(id, ext) { return '/kobo-img/' + encodeURIComponent(id) + '.' + (ext || 'png'); }
function pic(id, maxW, cls) {
	const im = h('img', { class: 'kb-pic ' + (cls || ''), alt: '', loading: 'lazy' });
	let triedJpg = false;
	im.addEventListener('error', () => {
		if (!triedJpg) { triedJpg = true; im.src = imgUrl(id, 'jpg'); }
	});
	im.addEventListener('load', () => {
		const w = im.naturalWidth || 1;
		const s = Math.max(1, Math.min(8, Math.floor(maxW / w)));
		im.style.width = (w * s) + 'px';
		im.style.maxWidth = '100%';
		im.style.height = 'auto';
	});
	im.src = imgUrl(id);
	return im;
}
function who(x) {
	/* 管理者には、匿名でも本当の名前とアカウントを見せる */
	const n = x.admin ? '管理人' : (x.name || '（名前なし）');
	return h('span', { class: 'kb-who' },
		h('b', { text: n }),
		x.anonymous ? tag('匿名で出した', 'private') : null,
		x.account ? h('span', { class: 'sub-text', text: ' ' + x.account }) : null);
}

/* ---------- ゲーム画像 ---------- */
async function loadGridList() {
	if (gridList) return gridList;
	const res = await fetch('/admin-grp/index.json', { credentials: 'same-origin' });
	if (!res.ok) throw new Error('/admin-grp/index.json が読めません（HTTP ' + res.status + '）');
	const j = await res.json();
	gridList = (j.files || []).slice().sort((a, b) => a.localeCompare(b, 'ja'));
	return gridList;
}
async function loadSheet(name) {
	if (sheetCache[name]) return sheetCache[name];
	const res = await fetch('/admin-grp/' + encodeURIComponent(name), { credentials: 'same-origin', cache: 'no-cache' });
	if (!res.ok) throw new Error(name + ' が読めません（HTTP ' + res.status + '）');
	const img = await decodeIndexedPng(await res.arrayBuffer());
	img.transparent = findTransparentIndex(normalizePalette(img.palette));
	sheetCache[name] = img;
	return img;
}
/* 範囲の切り出し（palette はそろえた 256 色）→ PNG */
function cutOut(img, x, y, w, hh) {
	const out = new Uint8Array(w * hh);
	for (let j = 0; j < hh; j++) out.set(img.indices.subarray((y + j) * img.width + x, (y + j) * img.width + x + w), j * w);
	return out;
}
function cutPng(img, palette, x, y, w, hh) {
	return encodeIndexedPng({ width: w, height: hh, indices: cutOut(img, x, y, w, hh), palette });
}
/* 画像を canvas に拡大して描く（透明の番号は暗い市松で見せる） */
function bitmapOf(img, transparent) {
	const c = document.createElement('canvas');
	c.width = img.width; c.height = img.height;
	const d = new ImageData(img.width, img.height);
	for (let i = 0; i < img.indices.length; i++) {
		const k = img.indices[i];
		const p = img.palette[k] || { r: 0, g: 0, b: 0 };
		const x = i % img.width, y = (i / img.width) | 0;
		if (k === transparent) {
			const v = ((x >> 2) + (y >> 2)) & 1 ? 46 : 38;
			d.data.set([v, v + 4, v + 14, 255], i * 4);
		} else {
			d.data.set([p.r, p.g, p.b, 255], i * 4);
		}
	}
	c.getContext('2d').putImageData(d, 0, 0);
	return c;
}

/* ---------- 入口 ---------- */
export function open(apiFn, badgeFn, target) {
	api = apiFn;
	onBadge = badgeFn || (() => {});
	root = target || document.getElementById('kobo-root');
	if (!root.dataset.ready) {
		root.dataset.ready = '1';
	}
	if (view.name === 'form' && root.firstChild) return;	/* 作業中のフォームは消さない */
	show({ name: 'list' });
}

/* 上に「送りました」などを少しだけ出す */
function flash(text) {
	if (!root) return;
	const m = h('div', { class: 'msg ok kb-flash', text: text });
	root.insertBefore(m, root.children[1] || null);
	setTimeout(() => m.remove(), 6000);
}

/* 描く画面（別のタブ）で送ったら知らせが来るので、今の画面を読み込み直して「送りました」を出す */
const SAVED_TEXT = { fix: '管理人の修正を出しました', base: '今の絵を保存しました', sample: '見本を保存しました', proposal: '返事を送りました' };
window.addEventListener('message', (e) => {
	if (e.origin !== location.origin || !e.data || e.data.type !== 'kobo-admin-saved') return;
	if (view.name !== 'form') show(view);
	flash(SAVED_TEXT[e.data.mode] || '送りました');
});

function show(v) {
	view = v;
	const seq = ++renderSeq;
	clear(root);
	root.appendChild(navBar());
	const body = h('div', { class: 'kb-body' });
	root.appendChild(body);
	const job = v.name === 'list' ? renderList(body, seq)
		: v.name === 'detail' ? renderDetail(body, seq, v.id)
		: v.name === 'form' ? renderForm(body, seq, v)
		: renderProposals(body, seq, v.status || 'new');
	job.catch((e) => {
		if (seq !== renderSeq) return;
		body.appendChild(h('div', { class: 'msg', text: '表示できませんでした：' + (e && e.message || e) }));
	});
}
function navBar() {
	const nav = h('div', { class: 'kb-nav' });
	const add = (label, v, active, badge) => {
		const b = h('button', { type: 'button', class: active ? '' : 'sub', on: { click: () => show(v) } }, label);
		if (badge) b.appendChild(h('span', { class: 'badge', text: String(badge) }));
		nav.appendChild(b);
	};
	add('お題の一覧', { name: 'list' }, view.name === 'list' || view.name === 'detail', counts.hiddenComments);
	add('提案', { name: 'proposals' }, view.name === 'proposals', counts.proposals);
	add('お題を作る', { name: 'form' }, view.name === 'form' && !view.id);
	return nav;
}
function setCounts(c) {
	if (!c) return;
	counts = { proposals: c.proposals || 0, hiddenComments: c.hiddenComments || 0 };
	onBadge(counts.proposals + counts.hiddenComments);
}

/* ---------- お題の一覧 ---------- */
async function renderList(body, seq) {
	body.appendChild(h('p', { class: 'sub-text', text: '管理者がお題（新しく描く絵か、ゲーム画像のどこを描き直すか）を出し、プレイヤーが案を描いて出します。案は描き足し合い、コメントで直し、管理者が 1 つ採用します。今の絵柄を変えず、そろえて仕上げるのが大事です。' }));
	const r = await api('/api/admin/kobo/tasks', {});
	if (seq !== renderSeq) return;
	if (!r.tasks) { body.appendChild(h('div', { class: 'msg', text: errText(r) })); return; }
	setCounts(r.counts);
	const nav = root.querySelector('.kb-nav');
	if (nav) nav.replaceWith(navBar());
	if (r.counts) {
		body.appendChild(h('div', { class: 'kb-counts' },
			h('span', null, '新しい提案 ', h('b', { text: String(counts.proposals) }), ' 件'),
			h('span', null, '本人が隠したコメント ', h('b', { text: String(counts.hiddenComments) }), ' 件')));
	}
	if (!r.tasks.length) body.appendChild(h('div', { class: 'sub-text', text: 'まだお題がありません。「お題を作る」から出してください。' }));
	for (const st of ['open', 'judging', 'done', 'hidden']) {
		const list = r.tasks.filter((t) => t.state === st);
		if (!list.length) continue;
		body.appendChild(h('h3', { text: STATES[st] + '（' + list.length + '）' }));
		const grid = h('div', { class: 'kb-cards' });
		for (const t of list) {
			const card = h('div', { class: 'card kb-task' + (st === 'hidden' ? ' dim' : ''), on: { click: () => show({ name: 'detail', id: t.id }) } });
			const thumb = t.adoptedImg || t.latestImg || t.base;
			card.appendChild(h('div', { class: 'kb-thumb' }, thumb ? pic(thumb, 160) : null));
			card.appendChild(h('div', { class: 'name', text: t.title || '（題なし）' }));
			card.appendChild(h('div', { class: 'sub-text' }, tag(KINDS[t.kind] || t.kind), ' ' + t.sheet + ' ' + t.w + '×' + t.h));
			card.appendChild(h('div', { class: 'sub-text', text: '出した ' + (t.works || 0) + ' 件 ／ 作業中（見せている）' + (t.wip || 0) + ' 件' }));
			grid.appendChild(card);
		}
		body.appendChild(grid);
	}
}

/* ---------- お題の詳しい画面 ---------- */
async function renderDetail(body, seq, id) {
	const r = await api('/api/admin/kobo/task', { id });
	if (seq !== renderSeq) return;
	if (!r.task) { body.appendChild(h('div', { class: 'msg', text: errText(r) })); return; }
	const task = r.task;
	const works = r.works || [];
	const comments = r.comments || [];
	let selWork = (view.work && works.find((w) => w.id === view.work)) ? view.work : '';
	const msg = msgBox();

	const reload = () => show({ name: 'detail', id, work: selWork });

	/* 見出し */
	body.appendChild(h('div', { class: 'card' },
		h('div', { class: 'head' },
			h('span', { class: 'name', text: task.title }),
			tag(STATES[task.state] || task.state, task.state === 'done' ? 'done' : task.state === 'hidden' ? 'rejected' : 'approved'),
			tag(KINDS[task.kind] || task.kind)),
		h('div', { class: 'sub-text', text: (task.fresh ? '新しく描く ' + task.w + '×' + task.h + '（パレットは ' + task.sheet + '）' : task.sheet + '（' + task.x + ',' + task.y + '）' + task.w + '×' + task.h) + ' ／ コマ ' + task.cw + '×' + task.ch + ' ／ ' + (PREVIEWS[task.preview] || task.preview) + ' ／ ' + task.frameMs + 'ms' }),
		task.desc ? h('div', { class: 'message', text: task.desc }) : null,
		h('div', { class: 'kb-refs' },
			refBlock('今の絵', task.base ? pic(task.base, 200) : null, task.fresh ? '' : '/kobo-draw?admin=base&task=' + encodeURIComponent(id)),
			refBlock('背景', task.bg ? pic(task.bg, 64) : null),
			refBlock('元イラスト', task.ref ? pic(task.ref, 240, 'kb-refpic') : null),
			...(task.samples || []).map((s, i) => refBlock('見本 ' + (i + 1), pic(s, 200), '/kobo-draw?admin=sample&task=' + encodeURIComponent(id) + '&n=' + i))),
		h('div', { class: 'sub-text', text: '「編集」は描く画面を別のタブで開きます。保存したら、この画面を読み込み直してください。' }),
		h('div', null,
			h('button', { type: 'button', on: { click: () => show({ name: 'form', id }) } }, '直す'),
			h('button', { type: 'button', class: 'sub', on: { click: () => {
				if (!confirm('このお題を消します。案・コメントも全部消えます。よろしいですか？')) return;
				api('/api/admin/kobo/task-delete', { id }).then((d) => {
					if (d.ok) show({ name: 'list' }); else setMsg(msg, errText(d));
				});
			} } }, 'お題を消す')),
		msg));

	/* 採用 */
	if (task.adopted) body.appendChild(adoptedCard(task, works, reload, msg));

	/* 系譜 */
	body.appendChild(h('h3', { text: '案の系譜（' + works.filter((w) => !w.deleted).length + ' 件）' }));
	if (!works.length) body.appendChild(h('div', { class: 'sub-text', text: 'まだ案がありません。' }));
	const byId = {};
	for (const w of works) byId[w.id] = w;
	const kids = {};
	const roots = [];
	for (const w of works) {
		if (w.parent && byId[w.parent]) (kids[w.parent] = kids[w.parent] || []).push(w); else roots.push(w);
	}
	const cmtOf = (wid) => comments.filter((c) => c.workId === wid);
	const scale = Math.max(1, Math.min(8, Math.floor(200 / (task.w || 16))));
	const node = (w) => {
		const li = h('li');
		const cs = cmtOf(w.id);
		const nHid = cs.filter((c) => c.hiddenByAuthor).length;
		const card = h('div', { class: 'kb-work' + (w.hidden || w.deleted ? ' dim' : '') + (w.id === selWork ? ' sel' : '') + (task.adopted && task.adopted.workId === w.id ? ' adopted' : ''), 'data-work': w.id });
		const head = h('div', { class: 'head' }, w.deleted ? h('b', { text: '（消した案）' }) : who(w),
			w.status === 'posted' ? tag('出した', 'approved') : tag('作業中' + (w.share ? '（見せている）' : '（本人だけ）'), 'old'));
		if (w.hidden) head.appendChild(tag('隠した', 'rejected'));
		if (task.adopted && task.adopted.workId === w.id) head.appendChild(tag('採用', 'done'));
		card.appendChild(head);
		if (!w.deleted && w.img) {
			const im = pic(w.img, 200);
			im.style.width = ((task.w || 16) * scale) + 'px';
			card.appendChild(h('div', { class: 'kb-art' }, im));
		}
		card.appendChild(h('div', { class: 'sub-text', text: 'なじんでる ' + (w.cheers || 0) + ' ／ コメント ' + cs.length + (nHid ? '（本人が隠した ' + nHid + '）' : '') + (w.postedAt || w.updatedAt ? ' ／ ' + dateText(w.postedAt || w.updatedAt) : '') }));
		if (w.note) card.appendChild(h('div', { class: 'message', text: w.note }));
		if (!w.deleted) {
			const bar = h('div', { class: 'kb-bar' });
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => { selWork = w.id; markSel(); drawComments(); } } }, 'コメント' + (nHid ? ' ●' : '')));
			/* 作監修正：この案に描き足して、子に「管理人」の案として出す */
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => openDraw('/kobo-draw?admin=fix&task=' + encodeURIComponent(id) + '&from=' + encodeURIComponent(w.id)) } }, '描き足して返す'));
			if (w.status === 'posted' && !w.hidden && !(task.adopted && task.adopted.workId === w.id)) {
				bar.appendChild(h('button', { type: 'button', on: { click: () => {
					if (!confirm('この案を採用します（お題は「採用済み」になります）。よろしいですか？')) return;
					api('/api/admin/kobo/adopt', { taskId: id, workId: w.id }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
				} } }, '採用する'));
			}
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => {
				api('/api/admin/kobo/work', { id: w.id, hidden: !w.hidden }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
			} } }, w.hidden ? '隠すのをやめる' : '隠す'));
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => {
				if (!confirm('この案を消します（子の案がある時は、系譜を保つため印だけ残ります）。よろしいですか？')) return;
				api('/api/admin/kobo/work', { id: w.id, deleted: true }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
			} } }, '消す'));
			card.appendChild(bar);
		}
		li.appendChild(card);
		const ch = kids[w.id];
		if (ch && ch.length) li.appendChild(h('ul', { class: 'kb-tree' }, ...ch.map(node)));
		return li;
	};
	if (works.length) body.appendChild(h('div', { class: 'kb-treewrap' }, h('ul', { class: 'kb-tree root' }, ...roots.map(node))));

	/* コメント */
	const cbox = h('div', { class: 'card', id: 'kb-comments' });
	body.appendChild(cbox);
	function markSel() {
		for (const c of body.querySelectorAll('.kb-work')) c.classList.toggle('sel', c.dataset.work === selWork);
	}
	function drawComments() {
		clear(cbox);
		const w = byId[selWork];
		if (!w) { cbox.appendChild(h('div', { class: 'sub-text', text: '案の「コメント」ボタンを押すと、その案のコメントがここに出ます。' })); return; }
		cbox.appendChild(h('div', { class: 'head' }, h('b', { text: 'コメント：' }), w.deleted ? '（消した案）' : who(w)));
		const cs = cmtOf(w.id);
		const top = cs.filter((c) => !c.replyTo || !cs.find((x) => x.id === c.replyTo));
		const row = (c, depth) => {
			const box = h('div', { class: 'kb-cmt' + (c.hidden || c.hiddenByAuthor ? ' dim' : ''), style: 'margin-left:' + (depth * 18) + 'px' });
			const hd = h('div', { class: 'head' }, who(c), h('span', { class: 'sub-text', text: dateText(c.at) }));
			if (c.hiddenByAuthor) hd.appendChild(tag('本人が隠した', 'pending'));
			if (c.hidden) hd.appendChild(tag('管理者が隠した', 'rejected'));
			if (c.admin) hd.appendChild(tag('管理人', 'admin'));
			box.appendChild(hd);
			box.appendChild(h('div', { class: 'message', text: c.body }));
			const bar = h('div', { class: 'kb-bar' });
			const replyArea = h('div');
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => {
				if (replyArea.firstChild) { clear(replyArea); return; }
				replyArea.appendChild(commentForm(w.id, c.id, '管理人として返事を書く', () => reload()));
			} } }, '返事'));
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => {
				api('/api/admin/kobo/comment-update', { id: c.id, hidden: !c.hidden }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
			} } }, c.hidden ? '隠すのをやめる' : '隠す'));
			bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => {
				if (!confirm('このコメントを消します。よろしいですか？')) return;
				api('/api/admin/kobo/comment-update', { id: c.id, delete: true }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
			} } }, '消す'));
			box.appendChild(bar);
			box.appendChild(replyArea);
			const out = [box];
			for (const k of cs.filter((x) => x.replyTo === c.id)) out.push(...row(k, depth + 1));
			return out;
		};
		if (!cs.length) cbox.appendChild(h('div', { class: 'sub-text', text: 'まだコメントがありません。' }));
		for (const c of top) for (const e of row(c, 0)) cbox.appendChild(e);
		cbox.appendChild(h('div', { class: 'reply-box' }, commentForm(w.id, '', '管理人としてコメントする', () => reload())));
	}
	function commentForm(workId, replyTo, label, done) {
		const ta = h('textarea', { placeholder: label + '（300 字まで）', maxLength: 300, rows: 2 });
		const m = msgBox();
		return h('div', null, ta,
			h('button', { type: 'button', on: { click: (ev) => {
				const bodyText = ta.value.trim();
				if (!bodyText) { setMsg(m, '書いてください。'); return; }
				ev.target.disabled = true;
				api('/api/admin/kobo/comment', { workId, body: bodyText, replyTo: replyTo || undefined }).then((d) => {
					ev.target.disabled = false;
					if (d.ok) done(); else setMsg(m, errText(d));
				});
			} } }, '送る'), m);
	}
	drawComments();
}
function refBlock(label, content, editUrl) {
	if (!content) return null;
	return h('div', { class: 'kb-ref' }, h('div', { class: 'sub-text', text: label }), content,
		editUrl ? h('div', null, h('button', { type: 'button', class: 'sub', on: { click: () => openDraw(editUrl) } }, '編集')) : null);
}

/* 描く画面を管理人として別のタブで開く（保存したらこの画面を読み込み直す） */
function openDraw(url) {
	window.open(url, '_blank');
}

/* 採用した案・協力者・ゲーム画像に貼る */
function adoptedCard(task, works, reload, msg) {
	const a = task.adopted;
	const work = works.find((w) => w.id === a.workId);
	const card = h('div', { class: 'card kb-adopted' });
	card.appendChild(h('div', { class: 'head' }, h('b', { text: '採用した案' }), tag('採用済み', 'done'), h('span', { class: 'sub-text', text: dateText(a.at) })));
	card.appendChild(h('div', { class: 'kb-refs' }, work && work.img ? refBlock('採用', pic(work.img, 240)) : null));
	card.appendChild(h('div', { class: 'sub-text', text: '協力者（クレジット）' }));
	const ul = h('ul', { class: 'history' });
	for (const c of a.credits || []) {
		ul.appendChild(h('li', null, (ROLES[c.role] || c.role) + '：', c.anonymous ? '匿名さん' : (c.name || '（名前なし）')));
	}
	card.appendChild(ul);
	const m = msgBox();
	card.appendChild(h('div', null,
		task.fresh ? null : h('button', { type: 'button', on: { click: async (ev) => {
			ev.target.disabled = true;
			try {
				setMsg(m, '作っています…');
				const bytes = await pasteIntoSheet(task, work);
				download(bytes, task.sheet, 'image/png');
				setMsg(m, task.sheet + ' をダウンロードしました。', true);
			} catch (e) { setMsg(m, '作れませんでした：' + (e && e.message || e)); }
			ev.target.disabled = false;
		} } }, 'ゲーム画像に貼って保存'),
		h('button', { type: 'button', class: 'sub', on: { click: async (ev) => {
			ev.target.disabled = true;
			try {
				const res = await fetch(imgUrl(work.img), { credentials: 'same-origin' });
				if (!res.ok) throw new Error('HTTP ' + res.status);
				download(new Uint8Array(await res.arrayBuffer()), (task.title || 'kobo').replace(/[\\/:*?"<>|]/g, '_') + '.png', 'image/png');
				setMsg(m, 'この案の PNG を保存しました。', true);
			} catch (e) { setMsg(m, '保存できませんでした：' + (e && e.message || e)); }
			ev.target.disabled = false;
		} } }, task.fresh ? 'この案の PNG を保存' : 'この案の PNG だけ保存'),
		h('button', { type: 'button', class: 'sub', on: { click: () => {
			if (!confirm('採用を取り消します（お題は「選考中」などに戻ります）。よろしいですか？')) return;
			api('/api/admin/kobo/adopt', { taskId: task.id, workId: '' }).then((d) => { if (d.ok) reload(); else setMsg(msg, errText(d)); });
		} } }, '採用を取り消す')));
	card.appendChild(h('p', { class: 'sub-text', text: task.fresh ? '新しく描くお題なので、保存した PNG（' + task.sheet + ' と同じパレット）を、置きたいゲーム画像の空いている場所に合成してください。色番号はそのまま使えます。' : 'ダウンロードした ' + task.sheet + ' を、Mac のクローンの SboGrpData/grp に置き換えてコミットすると、ゲームの絵が変わります（パレットは元のまま・指定の位置だけが変わります）。' }));
	card.appendChild(m);
	return card;
}
async function pasteIntoSheet(task, work) {
	if (!work || !work.img) throw new Error('採用した案の絵が見つかりません');
	const sheet = await (async () => {
		/* 毎回読み直す（キャッシュだと、置き換え後の古い絵に貼ってしまう） */
		delete sheetCache[task.sheet];
		return loadSheet(task.sheet);
	})();
	const res = await fetch(imgUrl(work.img), { credentials: 'same-origin' });
	if (!res.ok) throw new Error('案の絵が読めません（HTTP ' + res.status + '）');
	const art = await decodeIndexedPng(await res.arrayBuffer());
	if (art.width !== task.w || art.height !== task.h) throw new Error('案の絵の大きさ（' + art.width + '×' + art.height + '）がお題（' + task.w + '×' + task.h + '）と違います');
	if (task.x + task.w > sheet.width || task.y + task.h > sheet.height) throw new Error('範囲がゲーム画像の外にはみ出しています');
	if (!palettesEqual(normalizePalette(art.palette), normalizePalette(sheet.palette))) {
		if (!confirm('案のパレットがゲーム画像のパレットと違います。番号のまま貼りますが、色が変わるかもしれません。続けますか？')) throw new Error('やめました');
	}
	const out = sheet.indices.slice();
	for (let j = 0; j < task.h; j++) {
		out.set(art.indices.subarray(j * task.w, (j + 1) * task.w), (task.y + j) * sheet.width + task.x);
	}
	return encodeIndexedPng({ width: sheet.width, height: sheet.height, indices: out, palette: sheet.palette });
}

/* ---------- 提案 ---------- */
async function renderProposals(body, seq, status) {
	body.appendChild(h('p', { class: 'sub-text', text: 'プレイヤーからの「こういうのどう？」です（管理者だけが見えます）。「お題にする」と、お題を作る画面が開きます。' }));
	const tabs = h('div', { class: 'kb-nav' });
	for (const k of Object.keys(PSTATES)) {
		tabs.appendChild(h('button', { type: 'button', class: k === status ? '' : 'sub', on: { click: () => show({ name: 'proposals', status: k }) } }, PSTATES[k]));
	}
	body.appendChild(tabs);
	const r = await api('/api/admin/kobo/proposals', { status });
	if (seq !== renderSeq) return;
	if (!r.proposals) { body.appendChild(h('div', { class: 'msg', text: errText(r) })); return; }
	if (status === 'new') { setCounts({ proposals: r.proposals.length, hiddenComments: counts.hiddenComments }); const nav = root.querySelector('.kb-nav'); if (nav) nav.replaceWith(navBar()); }
	if (!r.proposals.length) body.appendChild(h('div', { class: 'sub-text', text: 'ありません。' }));
	for (const p of r.proposals) {
		const card = h('div', { class: 'card kb-prop' });
		card.appendChild(h('div', { class: 'head' }, who(p), tag(KINDS[p.kind] || p.kind || '種類なし'), tag(PSTATES[p.status] || p.status, p.status === 'new' ? 'pending' : ''),
			h('span', { class: 'sub-text', text: dateText(p.at) + (p.editedAt ? '（' + dateText(p.editedAt) + ' に本人が直した）' : '') })));
		card.appendChild(h('div', { class: 'message', text: p.body }));
		if (p.img) card.appendChild(h('div', { class: 'kb-art' }, pic(p.img, 256)));
		const m = msgBox();
		const ta = h('textarea', { rows: 2, placeholder: '返事（本人にだけ見えます）', value: p.reply || '' });
		const upd = (data, okText) => api('/api/admin/kobo/proposal-update', Object.assign({ id: p.id }, data)).then((d) => {
			if (d.ok) { show({ name: 'proposals', status }); flash(okText || '保存しました'); } else setMsg(m, errText(d));
		});
		const bar = h('div', { class: 'kb-bar' });
		for (const k of ['new', 'later', 'declined']) {
			if (k !== p.status && p.status !== 'tasked') bar.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => upd({ status: k }) } }, PSTATES[k] + 'にする'));
		}
		if (p.status === 'tasked' && p.taskId) bar.appendChild(h('button', { type: 'button', on: { click: () => show({ name: 'detail', id: p.taskId }) } }, 'お題を開く'));
		else bar.appendChild(h('button', { type: 'button', on: { click: () => show({ name: 'form', proposal: p }) } }, 'お題にする'));
		card.appendChild(ta);
		if (p.replyImg) {
			const older = p.img && (p.imgAt || p.at || 0) > (p.replyAt || 0);
			card.appendChild(h('div', { class: 'kb-ref' }, h('div', { class: 'sub-text', text: older ? '前に返した絵（この後に本人がラフ絵を描き直しました）' : '返事の絵' }), pic(p.replyImg, 256),
				h('div', null, h('button', { type: 'button', class: 'sub', on: { click: () => { if (confirm('返事の絵を外しますか？')) upd({ replyImg: '' }, '返事の絵を外しました'); } } }, '絵を外す'))));
		}
		card.appendChild(h('div', null, h('button', { type: 'button', class: 'sub', on: { click: () => upd({ reply: ta.value }, '返事を送りました') } }, '返事を保存'),
			h('button', { type: 'button', class: 'sub', on: { click: () => openDraw('/kobo-draw?admin=proposal&id=' + encodeURIComponent(p.id)) } }, !p.replyImg ? '描き足して返事' : (p.img && (p.imgAt || p.at || 0) > (p.replyAt || 0) ? '新しいラフ絵に描き足して返事' : '返事の絵を描き直す')),
			p.replyAt ? h('span', { class: 'sub-text', text: dateText(p.replyAt) + ' に返事済み' }) : null));
		card.appendChild(bar);
		card.appendChild(m);
		body.appendChild(card);
	}
}

/* ---------- お題を作る・直す ---------- */
async function renderForm(body, seq, v) {
	const files = await loadGridList();
	if (seq !== renderSeq) return;
	let task = null;
	if (v.id) {
		const r = await api('/api/admin/kobo/task', { id: v.id });
		if (seq !== renderSeq) return;
		if (!r.task) { body.appendChild(h('div', { class: 'msg', text: errText(r) })); return; }
		task = r.task;
	}
	const prop = v.proposal || null;
	const editing = !!task;

	/* 入力の状態 */
	const S = {
		sheet: task ? task.sheet : '',
		img: null,
		cw: task ? task.cw : 16, ch: task ? task.ch : 16,
		sel: task ? { x: task.x, y: task.y, w: task.w, h: task.h } : null,
		samples: [],			/* [{x,y}] */
		samplesDirty: false,
		mode: 'row',
		scale: 3,
		bgFile: '', bgImg: null, bgCell: null, bgDirty: false, bgOff: false,
		refData: null, refType: 'png', refUrl: '', refInfo: '',
		rebase: false,
		/* 新しく描く（白紙から。置き場所は採用後に決める）か、ゲーム画像の一部を描き直すか */
		fresh: task ? !!task.fresh : true,
		cols: task && task.fresh ? Math.max(1, Math.round(task.w / task.cw)) : 0,
		rows: task && task.fresh ? Math.max(1, Math.round(task.h / task.ch)) : 0
	};
	const kindOf = task ? task.kind : (prop && KINDS[prop.kind] ? prop.kind : 'char');
	if (!S.cols) { S.cols = kindOf === 'char' ? 8 : 1; S.rows = 1; }

	const msg = msgBox();
	const f = {};
	f.title = h('input', { type: 'text', maxLength: 60, value: task ? task.title : '', placeholder: '例：骸骨兵を強そうにする' });
	f.desc = h('textarea', { rows: 4, maxLength: 2000, placeholder: 'どんな絵にしてほしいか（2000 字まで）', value: task ? task.desc : (prop ? prop.body : '') });
	f.kind = h('select', null, ...Object.keys(KINDS).map((k) => h('option', { value: k, text: KINDS[k], selected: k === kindOf })));
	f.state = h('select', null, ...Object.keys(STATES).map((k) => h('option', { value: k, text: STATES[k], selected: k === (task ? task.state : 'open') })));
	f.preview = h('select', null, ...Object.keys(PREVIEWS).map((k) => h('option', { value: k, text: PREVIEWS[k], selected: k === (task ? task.preview : PREVIEW_OF_KIND[kindOf]) })));
	f.frameMs = h('input', { type: 'number', min: 50, max: 2000, step: 10, value: task ? task.frameMs : 200, style: 'width:6em' });
	f.sheet = h('select', null, h('option', { value: '', text: '（選んでください）' }), ...files.map((n) => h('option', { value: n, text: n, selected: n === S.sheet })));
	f.cw = h('input', { type: 'number', min: 1, max: 128, value: S.cw, style: 'width:5em' });
	f.ch = h('input', { type: 'number', min: 1, max: 128, value: S.ch, style: 'width:5em' });
	f.cols = h('input', { type: 'number', min: 1, max: 16, value: S.cols, style: 'width:4em' });
	f.rows = h('input', { type: 'number', min: 1, max: 16, value: S.rows, style: 'width:4em' });
	f.scale = h('select', null, ...[1, 2, 3, 4, 5, 6, 8].map((n) => h('option', { value: n, text: n + ' 倍', selected: n === S.scale })));

	const canvas = h('canvas', { class: 'kb-canvas' });
	const selInfo = h('div', { class: 'sub-text' });
	const modeBox = h('div', { class: 'kb-modes' });
	const modes = { row: '1 行を選ぶ', drag: 'ドラッグで範囲を選ぶ', sample: '見本を選ぶ（クリック。もう一度で外す）' };
	for (const k of Object.keys(modes)) {
		modeBox.appendChild(h('label', { class: 'check' }, h('input', { type: 'radio', name: 'kb-mode', value: k, checked: k === S.mode, on: { change: () => { S.mode = k; draw(); } } }), modes[k]));
	}
	const existing = h('div', { class: 'kb-refs' });
	const sampleThumbs = h('div', { class: 'kb-refs' });
	const bgSelect = h('select', null);
	const bgCanvas = h('canvas', { class: 'kb-canvas kb-bgcanvas' });
	const bgPrev = h('div', { class: 'kb-refs' });
	const bgOff = h('input', { type: 'checkbox', on: { change: () => { S.bgOff = bgOff.checked; S.bgDirty = !S.bgOff; } } });
	const refFile = h('input', { type: 'file', accept: 'image/png,image/jpeg' });
	const refPrev = h('div', { class: 'kb-refs' });
	const rebase = h('input', { type: 'checkbox', on: { change: () => { S.rebase = rebase.checked; } } });

	function setKindDefaults() {
		f.preview.value = PREVIEW_OF_KIND[f.kind.value] || 'tile';
		if (S.fresh) {
			/* NPC・敵は 4 方向×2 コマの 1 行、ほかは 1 コマから */
			f.cols.value = f.kind.value === 'char' ? 8 : 1;
			f.rows.value = 1;
			S.sel = defaultSel(); S.samples = []; renderSampleThumbs();
		}
		if (f.kind.value === 'char') S.mode = 'row'; else if (S.mode === 'row') S.mode = 'drag';
		applyFresh();
	}
	/* 新規か描き直しかで、出す欄と選び方を切り替える */
	const freshNew = h('input', { type: 'radio', name: 'kb-fresh', checked: S.fresh });
	const freshOld = h('input', { type: 'radio', name: 'kb-fresh', checked: !S.fresh });
	const freshSize = h('label', null, 'コマ数 ', f.cols, ' × ', f.rows);
	const rebaseRow = h('label', { class: 'check' });
	function applyFresh() {
		freshSize.style.display = S.fresh ? '' : 'none';
		rebaseRow.style.display = S.fresh || !editing ? 'none' : '';
		if (S.fresh) S.mode = 'sample';
		else if (S.mode === 'sample' && !S.sel) S.mode = f.kind.value === 'char' ? 'row' : 'drag';
		for (const lb of modeBox.children) {
			const r = lb.querySelector('input');
			lb.style.display = S.fresh && r.value !== 'sample' ? 'none' : '';
			r.checked = r.value === S.mode;
		}
		draw();
	}
	function setFresh(b) {
		S.fresh = b;
		S.sel = defaultSel(); S.samples = []; S.samplesDirty = S.samplesDirty || !editing;
		renderSampleThumbs();
		applyFresh();
	}
	freshNew.addEventListener('change', () => setFresh(true));
	freshOld.addEventListener('change', () => setFresh(false));
	f.kind.addEventListener('change', setKindDefaults);

	/* 元の画像と、コマのグリッド・範囲・見本 */
	let bitmap = null;
	function cwch() {
		S.cw = Math.max(1, Math.floor(Number(f.cw.value) || 16));
		S.ch = Math.max(1, Math.floor(Number(f.ch.value) || 16));
		return [S.cw, S.ch];
	}
	function draw() {
		if (!S.img) { canvas.width = 0; canvas.height = 0; selInfo.textContent = '元のゲーム画像を選んでください。'; return; }
		const [cw, ch] = cwch();
		const sc = S.scale = Number(f.scale.value) || 3;
		canvas.width = S.img.width * sc; canvas.height = S.img.height * sc;
		const g = canvas.getContext('2d');
		g.imageSmoothingEnabled = false;
		g.drawImage(bitmap, 0, 0, canvas.width, canvas.height);
		g.lineWidth = 1;
		g.strokeStyle = 'rgba(255,255,255,0.18)';
		g.beginPath();
		for (let x = cw; x < S.img.width; x += cw) { g.moveTo(x * sc + 0.5, 0); g.lineTo(x * sc + 0.5, canvas.height); }
		for (let y = ch; y < S.img.height; y += ch) { g.moveTo(0, y * sc + 0.5); g.lineTo(canvas.width, y * sc + 0.5); }
		g.stroke();
		S.samples.forEach((s, i) => {
			if (!S.sel) return;
			g.strokeStyle = '#6fa8dc'; g.lineWidth = 2;
			g.strokeRect(s.x * sc + 1, s.y * sc + 1, S.sel.w * sc - 2, S.sel.h * sc - 2);
			g.fillStyle = '#6fa8dc'; g.font = 'bold 13px sans-serif';
			g.fillText('見本' + (i + 1), s.x * sc + 4, s.y * sc + 15);
		});
		if (S.sel && !S.fresh) {
			g.fillStyle = 'rgba(240,184,74,0.22)';
			g.fillRect(S.sel.x * sc, S.sel.y * sc, S.sel.w * sc, S.sel.h * sc);
			g.strokeStyle = '#f0b84a'; g.lineWidth = 2;
			g.strokeRect(S.sel.x * sc + 1, S.sel.y * sc + 1, S.sel.w * sc - 2, S.sel.h * sc - 2);
		}
		if (S.fresh && S.sel) {
			selInfo.textContent = '新しく描く絵：' + S.sel.w + '×' + S.sel.h + '（コマ ' + cw + '×' + ch + ' が ' + Math.round(S.sel.w / cw) + '×' + Math.round(S.sel.h / ch) + '）。この画像はパレット（使える色）と見本に使います。見本 ' + S.samples.length + ' 個';
			return;
		}
		selInfo.textContent = S.sel ? '描く範囲：(' + S.sel.x + ', ' + S.sel.y + ') ' + S.sel.w + '×' + S.sel.h + '（コマ ' + Math.round(S.sel.w / cw * 10) / 10 + '×' + Math.round(S.sel.h / ch * 10) / 10 + '）　見本 ' + S.samples.length + ' 個' : '画像の上で、描く範囲を選んでください。';
	}
	async function chooseSheet(name, keep) {
		S.sheet = name;
		if (!name) { S.img = null; bitmap = null; draw(); return; }
		try {
			S.img = await loadSheet(name);
		} catch (e) { setMsg(msg, e.message); return; }
		setMsg(msg, '');
		bitmap = bitmapOf(S.img, S.img.transparent);
		if (!keep) {
			const g = guessCellSize(name);
			f.cw.value = g.w; f.ch.value = g.h;
			S.sel = null; S.samples = []; S.samplesDirty = !editing;
			S.sel = defaultSel();
			renderSampleThumbs();
		}
		f.scale.value = String(Math.max(1, Math.min(6, Math.floor(900 / S.img.width))));
		draw();
	}
	function defaultSel() {
		const [cw, ch] = cwch();
		if (S.fresh) {
			const cols = Math.max(1, Math.min(16, Math.floor(Number(f.cols.value) || 1)));
			const rows = Math.max(1, Math.min(16, Math.floor(Number(f.rows.value) || 1)));
			return { x: 0, y: 0, w: cw * cols, h: ch * rows };
		}
		if (!S.img) return null;
		if (f.kind.value === 'char') return { x: 0, y: 0, w: Math.floor(S.img.width / cw) * cw, h: ch };
		return { x: 0, y: 0, w: cw, h: ch };
	}
	f.sheet.addEventListener('change', () => chooseSheet(f.sheet.value, false));
	f.scale.addEventListener('change', draw);
	f.cw.addEventListener('change', () => { S.sel = defaultSel(); S.samples = []; draw(); renderSampleThumbs(); });
	f.ch.addEventListener('change', () => { S.sel = defaultSel(); S.samples = []; draw(); renderSampleThumbs(); });
	f.cols.addEventListener('change', () => { S.sel = defaultSel(); S.samples = []; draw(); renderSampleThumbs(); });
	f.rows.addEventListener('change', () => { S.sel = defaultSel(); S.samples = []; draw(); renderSampleThumbs(); });

	let dragFrom = null;
	function cellAt(ev) {
		const r = canvas.getBoundingClientRect();
		const [cw, ch] = cwch();
		const px = (ev.clientX - r.left) * (canvas.width / r.width) / S.scale;
		const py = (ev.clientY - r.top) * (canvas.height / r.height) / S.scale;
		return { cx: Math.max(0, Math.min(Math.floor(S.img.width / cw) - 1, Math.floor(px / cw))), cy: Math.max(0, Math.min(Math.floor(S.img.height / ch) - 1, Math.floor(py / ch))), px, py };
	}
	canvas.addEventListener('pointerdown', (ev) => {
		if (!S.img) return;
		const [cw, ch] = cwch();
		const c = cellAt(ev);
		if (S.mode === 'row') {
			S.sel = { x: 0, y: c.cy * ch, w: Math.floor(S.img.width / cw) * cw, h: ch };
			S.samples = []; S.samplesDirty = S.samplesDirty || !editing; renderSampleThumbs();
		} else if (S.mode === 'drag') {
			dragFrom = c; canvas.setPointerCapture(ev.pointerId);
			S.sel = { x: c.cx * cw, y: c.cy * ch, w: cw, h: ch };
			S.samples = []; renderSampleThumbs();
		} else if (S.mode === 'sample') {
			if (!S.sel) { setMsg(msg, '先に描く範囲を選んでください。'); return; }
			if (!S.samplesDirty) { S.samples = []; S.samplesDirty = true; }
			const hit = S.samples.findIndex((s) => c.px >= s.x && c.px < s.x + S.sel.w && c.py >= s.y && c.py < s.y + S.sel.h);
			if (hit >= 0) S.samples.splice(hit, 1);
			else if (S.samples.length >= 4) { setMsg(msg, '見本は 4 個までです。'); return; }
			else {
				const x = Math.min(Math.round(Math.floor(c.px / cw) * cw), S.img.width - S.sel.w);
				const y = Math.min(Math.round(Math.floor(c.py / ch) * ch), S.img.height - S.sel.h);
				S.samples.push({ x: Math.max(0, x), y: Math.max(0, y) });
			}
			setMsg(msg, '');
			renderSampleThumbs();
		}
		draw();
	});
	canvas.addEventListener('pointermove', (ev) => {
		if (S.mode !== 'drag' || !dragFrom || !S.img) return;
		const [cw, ch] = cwch();
		const c = cellAt(ev);
		const x0 = Math.min(dragFrom.cx, c.cx), x1 = Math.max(dragFrom.cx, c.cx), y0 = Math.min(dragFrom.cy, c.cy), y1 = Math.max(dragFrom.cy, c.cy);
		S.sel = { x: x0 * cw, y: y0 * ch, w: (x1 - x0 + 1) * cw, h: (y1 - y0 + 1) * ch };
		draw();
	});
	canvas.addEventListener('pointerup', () => { dragFrom = null; });

	function renderSampleThumbs() {
		clear(sampleThumbs);
		if (editing && !S.samplesDirty) {
			for (const id of task.samples || []) sampleThumbs.appendChild(refBlock('今の見本', pic(id, 160)));
			if ((task.samples || []).length) sampleThumbs.appendChild(h('div', { class: 'sub-text', text: '見本を選び直すと、今の見本は入れ替わります。' }));
			return;
		}
		if (!S.img || !S.sel) return;
		S.samples.forEach((s, i) => {
			const c = h('canvas', { class: 'kb-pic' });
			c.width = S.sel.w; c.height = S.sel.h;
			const sub = { width: S.sel.w, height: S.sel.h, indices: cutOut(S.img, s.x, s.y, S.sel.w, S.sel.h), palette: S.img.palette };
			c.getContext('2d').drawImage(bitmapOf(sub, S.img.transparent), 0, 0);
			const sc = Math.max(1, Math.min(8, Math.floor(160 / S.sel.w)));
			c.style.width = (S.sel.w * sc) + 'px';
			sampleThumbs.appendChild(refBlock('見本 ' + (i + 1), c));
		});
		sampleThumbs.appendChild(h('button', { type: 'button', class: 'sub', on: { click: () => { S.samples = []; S.samplesDirty = true; renderSampleThumbs(); draw(); } } }, '見本を全部外す'));
	}

	/* 背景（map の 16×16 を 1 マス） */
	const maps = files.filter((n) => /^map\d*\.png$/i.test(n));
	for (const n of maps) bgSelect.appendChild(h('option', { value: n, text: n }));
	function drawBg() {
		if (!S.bgImg) return;
		const sc = 2;
		const bm = S.bgBitmap;
		bgCanvas.width = S.bgImg.width * sc; bgCanvas.height = S.bgImg.height * sc;
		const g = bgCanvas.getContext('2d');
		g.imageSmoothingEnabled = false;
		g.drawImage(bm, 0, 0, bgCanvas.width, bgCanvas.height);
		g.strokeStyle = 'rgba(255,255,255,0.18)'; g.lineWidth = 1; g.beginPath();
		for (let x = BG_CELL; x < S.bgImg.width; x += BG_CELL) { g.moveTo(x * sc + 0.5, 0); g.lineTo(x * sc + 0.5, bgCanvas.height); }
		for (let y = BG_CELL; y < S.bgImg.height; y += BG_CELL) { g.moveTo(0, y * sc + 0.5); g.lineTo(bgCanvas.width, y * sc + 0.5); }
		g.stroke();
		if (S.bgCell) { g.strokeStyle = '#f0b84a'; g.lineWidth = 2; g.strokeRect(S.bgCell.x * sc + 1, S.bgCell.y * sc + 1, BG_CELL * sc - 2, BG_CELL * sc - 2); }
		clear(bgPrev);
		if (editing && task.bg && !S.bgDirty) bgPrev.appendChild(refBlock('今の背景', pic(task.bg, 64)));
		if (S.bgCell) {
			const c = h('canvas', { class: 'kb-pic' });
			c.width = BG_CELL; c.height = BG_CELL;
			c.getContext('2d').drawImage(bitmapOf({ width: BG_CELL, height: BG_CELL, indices: cutOut(S.bgImg, S.bgCell.x, S.bgCell.y, BG_CELL, BG_CELL), palette: S.bgImg.palette }, -1), 0, 0);
			c.style.width = '64px';
			bgPrev.appendChild(refBlock('選んだ背景', c));
		}
	}
	function guessGrass(img) {
		/* 草っぽい（緑が強い）完全に不透明なマス。無ければ 0 番以外の最初の不透明なマス */
		const cols = Math.floor(img.width / BG_CELL), rows = Math.floor(img.height / BG_CELL);
		let first = null;
		for (let n = 1; n < cols * rows; n++) {
			const cx = (n % cols) * BG_CELL, cy = Math.floor(n / cols) * BG_CELL;
			let r = 0, gg = 0, b = 0, ok = true;
			for (let j = 0; j < BG_CELL && ok; j++) for (let i = 0; i < BG_CELL; i++) {
				const p = img.palette[img.indices[(cy + j) * img.width + cx + i]] || { r: 0, g: 0, b: 0 };
				if (p.r === 0 && p.g === 0 && p.b === 0) { ok = false; break; }
				r += p.r; gg += p.g; b += p.b;
			}
			if (!ok) continue;
			if (!first) first = { x: cx, y: cy };
			if (gg > r * 1.15 && gg > b * 1.15) return { x: cx, y: cy };
		}
		return first;
	}
	async function chooseBgFile(name, pick) {
		S.bgFile = name;
		const img = await loadSheet(name);
		S.bgImg = img; S.bgBitmap = bitmapOf(img, -1);
		if (pick) S.bgCell = guessGrass(img);
		drawBg();
	}
	bgSelect.addEventListener('change', () => { S.bgDirty = !S.bgOff; chooseBgFile(bgSelect.value, true).catch((e) => setMsg(msg, e.message)); });
	bgCanvas.addEventListener('click', (ev) => {
		if (!S.bgImg) return;
		const r = bgCanvas.getBoundingClientRect();
		const px = (ev.clientX - r.left) * (bgCanvas.width / r.width) / 2;
		const py = (ev.clientY - r.top) * (bgCanvas.height / r.height) / 2;
		S.bgCell = { x: Math.floor(px / BG_CELL) * BG_CELL, y: Math.floor(py / BG_CELL) * BG_CELL };
		S.bgDirty = true; S.bgOff = false; bgOff.checked = false;
		drawBg();
	});
	if (maps.length) {
		const def = maps.find((n) => n.toLowerCase() === 'map01.png') || maps[0];
		bgSelect.value = def;
		chooseBgFile(def, !editing).then(() => { if (!editing) S.bgDirty = true; }).catch((e) => setMsg(msg, e.message));
	}

	/* 元イラスト */
	refFile.addEventListener('change', async () => {
		const file = refFile.files[0];
		S.refData = null; clear(refPrev);
		if (!file) return;
		try {
			const r = await prepareRef(file);
			S.refData = r.bytes; S.refType = r.type;
			const url = URL.createObjectURL(new Blob([r.bytes], { type: r.type === 'jpg' ? 'image/jpeg' : 'image/png' }));
			refPrev.appendChild(refBlock('元イラスト（' + Math.round(r.bytes.length / 1024) + ' KB' + (r.shrunk ? '・縮めました' : '') + '）', h('img', { class: 'kb-pic', src: url, style: 'max-width:240px' })));
			setMsg(msg, '');
		} catch (e) { setMsg(msg, '元イラストを読めませんでした：' + (e && e.message || e)); }
	});
	if (editing && task.ref) refPrev.appendChild(refBlock('今の元イラスト（選び直すと入れ替わります）', pic(task.ref, 240)));
	if (editing && task.base) existing.appendChild(refBlock('今の絵（お題を作った時のもの）', pic(task.base, 200)));

	/* 保存 */
	const saveBtn = h('button', { type: 'button', on: { click: save } }, editing ? '保存する' : 'お題を作る');
	async function save() {
		try {
			if (!f.title.value.trim()) throw new Error('題を書いてください。');
			if (S.fresh) S.sel = defaultSel();
			if (!S.img || !S.sel) throw new Error(S.fresh ? 'パレットを借りるゲーム画像を選んでください。' : '元のゲーム画像と、描く範囲を選んでください。');
			const [cw, ch] = cwch();
			if (S.sel.w % cw || S.sel.h % ch) throw new Error('範囲がコマ（' + cw + '×' + ch + '）の整数倍になっていません。');
			saveBtn.disabled = true;
			setMsg(msg, '保存しています…');
			const palette256 = normalizePalette(S.img.palette);
			/*
			 * 使える色：パレットの全色。ただし真っ黒はゲームではどれも透明になるので、透明の番号以外の真っ黒は外す。
			 * 見本の色（sampleColors）：見本（と描き直しなら今の絵）で使っている色。描く画面で先頭に並べる目安
			 */
			const transparent = findTransparentIndex(palette256);
			const colors = [];
			for (let i = 0; i < 256; i++) {
				const c = palette256[i];
				if (i === transparent || c.r || c.g || c.b) colors.push(i);
			}
			const used = new Uint8Array(256);
			const mark = (idx) => { for (let i = 0; i < idx.length; i++) used[idx[i]] = 1; };
			let nSample = 0;
			if (S.samplesDirty) {
				for (const sp of S.samples) { mark(cutOut(S.img, sp.x, sp.y, S.sel.w, S.sel.h)); nSample++; }
			} else if (editing) {
				for (const id of task.samples || []) {
					const res = await fetch(imgUrl(id), { credentials: 'same-origin' });
					if (!res.ok) throw new Error('今の見本が読めません（HTTP ' + res.status + '）');
					mark((await decodeIndexedPng(await res.arrayBuffer())).indices);
					nSample++;
				}
			}
			if (nSample && !S.fresh) mark(cutOut(S.img, S.sel.x, S.sel.y, S.sel.w, S.sel.h));
			const sampleColors = [];
			for (let i = 0; i < 256; i++) if (used[i] && i !== transparent && colors.indexOf(i) >= 0) sampleColors.push(i);
			const body = {
				title: f.title.value.trim(), desc: f.desc.value, kind: f.kind.value, state: f.state.value,
				sheet: S.sheet, x: S.sel.x, y: S.sel.y, w: S.sel.w, h: S.sel.h, cw, ch,
				preview: f.preview.value, frameMs: Math.max(50, Math.floor(Number(f.frameMs.value) || 200)),
				palette: palette256.map((c) => [c.r, c.g, c.b]), colors, sampleColors, transparent
			};
			if (S.fresh) body.fresh = true;
			if (editing) body.id = task.id;
			if (prop) body.proposalId = prop.id; else if (editing && task.proposalId) body.proposalId = task.proposalId;
			const rangeChanged = editing && (task.sheet !== S.sheet || task.x !== S.sel.x || task.y !== S.sel.y || task.w !== S.sel.w || task.h !== S.sel.h);
			if (!S.fresh && (!editing || rangeChanged || S.rebase || task.fresh)) body.baseData = toB64(await cutPng(S.img, palette256, S.sel.x, S.sel.y, S.sel.w, S.sel.h));
			if (S.bgDirty && !S.bgOff && S.bgCell && S.bgImg) body.bgData = toB64(await cutPng(S.bgImg, normalizePalette(S.bgImg.palette), S.bgCell.x, S.bgCell.y, BG_CELL, BG_CELL));
			if (S.samplesDirty) {
				body.samplesData = [];
				for (const s of S.samples) body.samplesData.push(toB64(await cutPng(S.img, palette256, s.x, s.y, S.sel.w, S.sel.h)));
			}
			if (S.refData) { body.refData = toB64(S.refData); body.refType = S.refType; }
			if (JSON.stringify(body).length > MAX_JSON) throw new Error('送る内容が大きすぎます（1MB まで）。元イラストを小さくしてください。');
			const r = await api('/api/admin/kobo/task-save', body);
			saveBtn.disabled = false;
			if (!r.ok) throw new Error(errText(r));
			show({ name: 'detail', id: r.task.id });
		} catch (e) {
			saveBtn.disabled = false;
			setMsg(msg, e && e.message || String(e));
		}
	}

	/* 組み立て */
	if (prop) {
		body.appendChild(h('div', { class: 'card' }, h('div', { class: 'head' }, h('b', { text: '提案からお題を作ります' }), who(prop)),
			h('div', { class: 'message', text: prop.body }), prop.img ? h('div', { class: 'kb-art' }, pic(prop.img, 256)) : null,
			h('div', { class: 'sub-text', text: '保存すると提案は「お題にした」になります。匿名の提案は匿名のままです。' })));
	}
	const sec = (title, ...kids) => h('div', { class: 'card kb-sec' }, h('h3', { text: title, style: 'margin-top:0' }), ...kids);
	body.appendChild(h('h3', { text: editing ? 'お題を直す' : 'お題を作る' }));
	body.appendChild(sec('1. 題・説明・種類',
		h('label', null, '題'), f.title,
		h('label', null, '説明（プレイヤーに見えます）'), f.desc,
		h('div', { class: 'kb-row' },
			h('label', null, '種類 ', f.kind), h('label', null, '状態 ', f.state),
			h('label', null, '見え方 ', f.preview), h('label', null, '1 コマ ', f.frameMs, ' ms'))));
	rebaseRow.appendChild(rebase);
	rebaseRow.appendChild(document.createTextNode('今の絵（お題の「今の絵から描く」の元）を、ゲーム画像から取り直す（範囲を変えた時は自動で取り直します）'));
	body.appendChild(sec('2. 新しく描くか、ゲーム画像の一部を描き直すか',
		h('div', { class: 'kb-row' },
			h('label', { class: 'check' }, freshNew, '新しく描く（白紙から。どこに置くかは採用後にこちらで合成）'),
			h('label', { class: 'check' }, freshOld, 'ゲーム画像の一部を描き直す')),
		h('div', { class: 'kb-row' }, h('label', null, '画像 ', f.sheet), h('label', null, 'コマ ', f.cw, ' × ', f.ch), freshSize, h('label', null, '拡大 ', f.scale)),
		modeBox,
		h('div', { class: 'kb-scroll' }, canvas), selInfo, existing,
		rebaseRow,
		h('div', { class: 'sub-text', text: '見本（同じ画像の中の、描く範囲と同じ大きさの場所を 0〜4 個）：上の「見本を選ぶ」でクリック。見本で使っている色（描き直しなら今の絵の色も）は、描く画面で「見本の色」として先頭に並びます。色はパレットの全色を使えます（透明になる余分な真っ黒は除く）。' }), sampleThumbs));
	body.appendChild(sec('3. 背景（等倍で並べて見る時の 16×16 の 1 マス）',
		h('div', { class: 'kb-row' }, h('label', null, '画像 ', bgSelect), h('label', { class: 'check' }, bgOff, '背景を使わない')),
		h('div', { class: 'kb-scroll' }, bgCanvas), bgPrev));
	body.appendChild(sec('4. 元イラスト（何を描くか。PNG / JPEG、大きければ縮めます）', refFile, refPrev));
	body.appendChild(h('div', null, saveBtn,
		h('button', { type: 'button', class: 'sub', on: { click: () => show(editing ? { name: 'detail', id: task.id } : { name: 'list' }) } }, 'やめる'), msg));

	if (S.fresh) S.sel = defaultSel();
	if (S.sheet) await chooseSheet(S.sheet, true);
	applyFresh();
	renderSampleThumbs();
}

/* 元イラストを送れる形にする：PNG/JPEG のまま MAX_REF_BYTES 以下ならそのまま、
   大きければ canvas で縮めて JPEG にする（長辺 1280 → 品質を下げ → さらに縮める） */
async function prepareRef(file) {
	if (!/^image\/(png|jpeg)$/.test(file.type)) throw new Error('PNG か JPEG を選んでください。');
	const raw = new Uint8Array(await file.arrayBuffer());
	const type = file.type === 'image/jpeg' ? 'jpg' : 'png';
	if (raw.length <= MAX_REF_BYTES) return { bytes: raw, type, shrunk: false };
	const bmp = await createImageBitmap(file);
	let side = 1280;
	for (let tries = 0; tries < 8; tries++) {
		const k = Math.min(1, side / Math.max(bmp.width, bmp.height));
		const c = document.createElement('canvas');
		c.width = Math.max(1, Math.round(bmp.width * k)); c.height = Math.max(1, Math.round(bmp.height * k));
		const g = c.getContext('2d');
		g.fillStyle = '#fff'; g.fillRect(0, 0, c.width, c.height);
		g.drawImage(bmp, 0, 0, c.width, c.height);
		for (const q of [0.88, 0.75, 0.6]) {
			const blob = await new Promise((res) => c.toBlob(res, 'image/jpeg', q));
			if (blob && blob.size <= MAX_REF_BYTES) return { bytes: new Uint8Array(await blob.arrayBuffer()), type: 'jpg', shrunk: true };
		}
		side = Math.floor(side * 0.75);
	}
	throw new Error('縮めても大きすぎます。');
}

/* admin.html が最初に件数だけ知りたい時用 */
window.SboKobo = { open };
