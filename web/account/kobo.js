/**
 * kobo.js
 * ドット絵工房（/kobo）の一覧・お題のページ・案の詳しい表示・コメント・提案。
 * 仕様は docs/kobo.md。ページは kobo.html、描く画面は kobo-draw.js。
 */
import {
	KEY_TOKEN, lsGet, api, el, dateText, displayName, imgUrl, refImgEl, fitScale,
	loadIndexed, displayPalette, indicesToCanvas, loadBgCanvas, composeOnBg, makePreview, thumbCanvas
} from '/dot-kobo.js';
import { decodeIndexedPng, encodeIndexedPng } from '/dot-png.js';

const KIND_TEXT = { char: 'NPC・敵', part: '背景パーツ', effect: 'エフェクト' };
const STATE_TEXT = { open: '募集中', judging: '選考中', done: 'できあがり' };
const PROPOSAL_STATUS = {
	new: '管理人が見るのを待っています',
	later: 'あとで考えます',
	declined: '今回は見送りになりました',
	tasked: 'お題になりました'
};
const AGREE_LINES = [
	'自分で描いた（考えた）ものです',
	'ゲームの一部として GPLv3 で配布されることがあります',
	'ほかの人が描き足してよいです',
	'採用されないこともあると分かっています'
];

const $ = function (id) { return document.getElementById(id); };

let token = lsGet(KEY_TOKEN) || '';
let me = null;               // { account, name, canPost }
let T = null;                // 開いているお題 { task, works, comments, myCheers }
let selWorkId = '';
let preview = null;          // 詳しい表示のプレビュー
let bigScale = 4;

/* ============ 共通 ============ */

function canAct() {
	return !!(me && me.canPost);
}

/** 小さな絵。透明が分かるように市松の台に載せる（task は palette・w・h を持つもの） */
function thumbBox(task, id, boxW, boxH, cls) {
	const box = el('div', 'checker ' + (cls || ''));
	box.appendChild(thumbCanvas(task, id, fitScale(task.w, task.h, boxW, boxH)));
	return box;
}

function setStatusPanel() {
	const p = $('status-panel');
	p.textContent = '';
	if (!token || (me === null && token)) {
		if (!token) {
			p.hidden = false;
			p.appendChild(el('p', '', '描いたりコメントしたりするには、このブラウザでゲームにログインしてね。'));
			p.appendChild(el('p', 'now', 'ゲームに一度ログインすると使えるようになります。見るだけなら、ログインしなくて大丈夫です。'));
			const b = el('div', 'buttons');
			const a = el('a', 'btn', 'ゲームを開く');
			a.href = '/';
			b.appendChild(a);
			p.appendChild(b);
			return;
		}
	}
	if (me && !me.canPost) {
		p.hidden = false;
		p.appendChild(el('p', '', '先にゲームでキャラを作ってから描いてね。'));
		const b = el('div', 'buttons');
		const a = el('a', 'btn', 'ゲームを開く');
		a.href = '/';
		b.appendChild(a);
		p.appendChild(b);
		return;
	}
	p.hidden = true;
}

function showView(name) {
	$('view-list').hidden = name !== 'list';
	$('view-task').hidden = name !== 'task';
}

function errText(data, fallback) {
	const M = {
		empty: '内容を書いてね。',
		long: 'ちょっと長すぎます。短くしてね。',
		wait: '続けて送るときは、少しだけ待ってね。',
		too_many: '今日はたくさん送ってくれたので、また明日お願いします。ありがとう！',
		no_char: '先にゲームでキャラを作ってね。',
		invalid_token: 'ログインが確かめられませんでした。ゲームでログインし直してね。',
		not_found: '見つかりませんでした。消えたのかもしれません。',
		agree: '同意にチェックを入れてね。',
		bad_png: '画像を読めませんでした。',
		bad_size: '画像が大きすぎます。',
		closed: 'このお題は今は募集していません。',
		not_allowed: 'それはできません。'
	};
	if (data && data.httpStatus === 401) {
		return M.invalid_token;
	}
	let t = (data && M[data.error]) || fallback || 'うまくいきませんでした。時間をおいて試してね。';
	if (data && data.error === 'wait' && data.waitSeconds) {
		t = '続けて送るときは、あと ' + data.waitSeconds + ' 秒ほど待ってね。';
	}
	return t;
}

/* ============ 一覧 ============ */

function loadList() {
	api('/api/kobo/tasks', { token: token || undefined }).then(function (data) {
		const box = $('groups');
		box.textContent = '';
		const tasks = data.tasks || [];
		renderResume(tasks, data.myWips || []);
		if (!tasks.length) {
			box.appendChild(el('p', 'empty', 'まだお題がありません。管理人がお題を出すと、ここに並びます。'));
			return;
		}
		['open', 'judging', 'done'].forEach(function (st) {
			const list = tasks.filter(function (t) { return t.state === st; });
			if (!list.length) {
				return;
			}
			const h = el('h3', '', STATE_TEXT[st] + '（' + list.length + '）');
			box.appendChild(h);
			const grid = el('div', 'cards');
			list.forEach(function (t) { grid.appendChild(taskCard(t)); });
			box.appendChild(grid);
		});
	}).catch(function () {
		$('groups').textContent = '';
		$('groups').appendChild(el('p', 'empty', 'サーバーにつながりませんでした。少し待ってから読み込み直してね。'));
	});
}

/* ============ 続きを描く ============ */

const DRAFT_PREFIX = 'kobo_draft:';	// 描く画面がこの端末に残す途中の絵（kobo-draw.js の draftKey と同じ）

function b64ToU8(b64) {
	const bin = atob(b64);
	const u8 = new Uint8Array(bin.length);
	for (let i = 0; i < bin.length; i++) {
		u8[i] = bin.charCodeAt(i);
	}
	return u8;
}

/** この端末の途中の絵（まだサーバーに作業中として保存していないもの）。[{taskId, from, at, d, w, h}] */
function localDrafts() {
	const a = [];
	try {
		for (let i = 0; i < window.localStorage.length; i++) {
			const k = window.localStorage.key(i);
			if (!k || k.indexOf(DRAFT_PREFIX) !== 0) {
				continue;
			}
			const rest = k.slice(DRAFT_PREFIX.length);
			const j = rest.indexOf(':');
			if (j < 0) {
				continue;
			}
			const d = JSON.parse(window.localStorage.getItem(k) || 'null');
			if (!d || !d.d) {
				continue;
			}
			const sub = rest.slice(j + 1);
			a.push({ taskId: rest.slice(0, j), workId: sub.indexOf('new-') === 0 ? '' : sub, from: sub.indexOf('new-') === 0 ? sub.slice(4) : '', at: d.at || 0, d: d.d, w: d.w, h: d.h });
		}
	} catch (e) { /* 読めなければ無し */ }
	return a;
}

/**
 * トップに「続きを描く」を出す。サーバーの作業中の案（自分の）と、この端末の途中の絵。
 * 募集中のお題のものだけ。同じ案は 1 つにまとめる（開くと描く画面が端末の途中の絵も聞いてくる）。
 */
function renderResume(tasks, myWips) {
	const box = $('resume');
	box.textContent = '';
	const byId = {};
	tasks.forEach(function (t) { byId[t.id] = t; });
	const items = [];
	const seen = {};
	myWips.forEach(function (w) {
		const t = byId[w.taskId];
		if (!t || t.state !== 'open') {
			return;
		}
		seen[w.id] = true;
		items.push({ task: t, at: w.updatedAt, href: drawUrl(t, { work: w.id }), img: w.img, sub: w.share ? '作業中（みんなに見せている）' : '作業中（自分だけ）' });
	});
	localDrafts().forEach(function (d) {
		const t = byId[d.taskId];
		if (!t || t.state !== 'open' || d.w !== t.w || d.h !== t.h) {
			return;
		}
		if (d.workId) {
			if (seen[d.workId]) {
				/* サーバーの作業中の案より端末の方が新しければ、時刻だけ新しくする */
				items.forEach(function (it) { if (it.href.indexOf(encodeURIComponent(d.workId)) >= 0 && d.at > it.at) { it.at = d.at; } });
			}
			return;
		}
		items.push({ task: t, at: d.at, href: drawUrl(t, { from: d.from || 'blank' }), u8: d.d, sub: 'この端末に残っている描きかけ' });
	});
	if (!items.length) {
		box.hidden = true;
		return;
	}
	items.sort(function (a, b) { return b.at - a.at; });
	box.hidden = false;
	box.appendChild(el('h3', '', '続きを描く'));
	const list = el('div', 'resume-list');
	items.forEach(function (it) {
		const t = it.task;
		const a = el('a', 'resume-item');
		a.href = it.href;
		const scale = fitScale(t.w, t.h, 96, 48);
		const th = el('div', 'checker');
		if (it.img) {
			th.appendChild(thumbCanvas(t, it.img, scale));
		} else {
			let cv;
			try {
				cv = indicesToCanvas(b64ToU8(it.u8), t.w, t.h, displayPalette(t));
			} catch (e) {
				cv = el('canvas');
			}
			cv.style.width = (t.w * scale) + 'px';
			th.appendChild(cv);
		}
		a.appendChild(th);
		const tx = el('div');
		tx.appendChild(el('b', '', t.title));
		tx.appendChild(el('small', '', it.sub));
		tx.appendChild(el('small', '', dateText(it.at)));
		a.appendChild(tx);
		list.appendChild(a);
	});
	box.appendChild(list);
}

function taskCard(t) {
	const a = el('a', 'card');
	a.href = '#task=' + encodeURIComponent(t.id);
	const imgId = t.adoptedImg || t.latestImg || t.base || '';
	if (imgId) {
		a.appendChild(thumbBox(t, imgId, 260, 96, 'card-thumb'));
	} else if (t.ref) {
		const box = el('div', 'card-thumb');
		const img = refImgEl(t.ref);
		img.style.maxHeight = '96px';
		box.appendChild(img);
		a.appendChild(box);
	}
	a.appendChild(el('div', 'card-title', t.title));
	const meta = el('div', 'card-meta');
	meta.appendChild(el('span', 'tag st-' + t.state, STATE_TEXT[t.state] || t.state));
	meta.appendChild(el('span', 'tag k-' + t.kind, KIND_TEXT[t.kind] || t.kind));
	meta.appendChild(el('span', '', '出した案 ' + (t.works || 0) + '　作業中 ' + (t.wip || 0)));
	a.appendChild(meta);
	return a;
}

/* ============ お題のページ ============ */

function loadTask(id, bKeepScroll) {
	const y = window.scrollY;
	return api('/api/kobo/task', { id: id, token: token || undefined }).then(function (data) {
		if (!data.task) {
			showView('task');
			const v = $('view-task');
			v.textContent = '';
			v.appendChild(backLink());
			v.appendChild(el('p', 'empty', 'このお題は見つかりませんでした。'));
			return;
		}
		T = data;
		T.myCheers = data.myCheers || [];
		T.works = data.works || [];
		T.comments = data.comments || [];
		showView('task');
		renderTask();
		if (bKeepScroll) {
			window.scrollTo(0, y);
		} else {
			window.scrollTo(0, 0);
		}
	}).catch(function () {
		showView('task');
		const v = $('view-task');
		v.textContent = '';
		v.appendChild(backLink());
		v.appendChild(el('p', 'empty', 'サーバーにつながりませんでした。少し待ってから読み込み直してね。'));
	});
}

function backLink() {
	const a = el('a', 'back', '← お題の一覧へ');
	a.href = '#';
	return a;
}

function drawUrl(task, params) {
	const q = new URLSearchParams({ task: task.id });
	Object.keys(params).forEach(function (k) { q.set(k, params[k]); });
	return '/kobo-draw?' + q.toString();
}

function renderTask() {
	const task = T.task;
	const v = $('view-task');
	if (preview) {
		preview.destroy();
		preview = null;
	}
	v.textContent = '';
	v.appendChild(backLink());

	const head = el('div', 'task-head');
	head.appendChild(el('h2', '', task.title));
	head.appendChild(el('span', 'tag st-' + task.state, STATE_TEXT[task.state] || task.state));
	head.appendChild(el('span', 'tag k-' + task.kind, KIND_TEXT[task.kind] || task.kind));
	v.appendChild(head);
	if (task.desc) {
		v.appendChild(el('div', 'task-desc', task.desc));
	}

	/* 採用された時 */
	if (task.adopted) {
		const cr = el('div', 'credits');
		const names = (task.adopted.credits || []).map(function (c) {
			const n = c.anonymous ? '匿名さん' : (c.name || '（名前なし）');
			const role = c.role === 'draw' ? '描いた人' : (c.role === 'base' ? '元の案の人' : '提案した人');
			return role + '：' + n;
		});
		cr.appendChild(el('strong', '', '採用されました！'));
		cr.appendChild(document.createTextNode('　' + names.join(' ／ ')));
		v.appendChild(cr);
	}

	/* 元イラストと見本 */
	const refs = el('div', 'refs');
	const refBox = el('div', 'ref-box');
	const h1 = el('h4', '', '何を描く？');
	h1.appendChild(el('span', '', '元イラスト'));
	refBox.appendChild(h1);
	if (task.ref) {
		const a = el('a');
		a.target = '_blank';
		a.rel = 'noopener';
		a.href = imgUrl(task.ref);
		const im = refImgEl(task.ref);
		im.addEventListener('error', function () { a.href = imgUrl(task.ref, 'jpg'); });
		a.appendChild(im);
		refBox.appendChild(a);
	} else {
		refBox.appendChild(el('p', 'now', task.base ? '元イラストはありません。今の絵や説明を見て描いてね。' : '元イラストはありません。見本や説明を見て描いてね。'));
		refBox.lastChild.style.color = 'var(--sub)';
	}
	refs.appendChild(refBox);

	const smpBox = el('div', 'ref-box');
	const h2 = el('h4', '', 'どう描く？');
	h2.appendChild(el('span', '', 'この絵柄に合わせよう'));
	smpBox.appendChild(h2);
	const imgs = el('div', 'imgs');
	(task.samples || []).forEach(function (id) {
		imgs.appendChild(thumbBox(task, id, 260, 160, ''));
	});
	if (task.base) {
		const wrap = el('div');
		wrap.appendChild(thumbBox(task, task.base, 260, 160, ''));
		wrap.appendChild(el('div', 'now', '今の絵'));
		wrap.lastChild.style.color = 'var(--sub)';
		wrap.lastChild.style.fontSize = '12px';
		imgs.appendChild(wrap);
	}
	smpBox.appendChild(imgs);
	refs.appendChild(smpBox);
	v.appendChild(refs);

	v.appendChild(el('div', 'hint', '上手さより、ゲームになじむかで選びます。見本と並べて、この絵柄に合わせてみてね。'));

	/* 描くボタン */
	const open = task.state === 'open';
	const draw = el('div', 'panel');
	if (open) {
		draw.appendChild(el('p', '', 'この絵を描いてみよう'));
		const b = el('div', 'buttons');
		const a2 = el('a', 'btn' + (task.base ? ' sub' : ''), 'まっさらから描く');
		a2.href = drawUrl(task, { from: 'blank' });
		if (task.base) {
			const a1 = el('a', 'btn', '今の絵から描く');
			a1.href = drawUrl(task, { from: 'base' });
			b.appendChild(a1);
		}
		b.appendChild(a2);
		draw.appendChild(b);
		draw.appendChild(el('p', 'note', 'だれかの案を元に描き足すには、下の案を選んで「この案を元に描く」を押してね。'));
		if (!canAct()) {
			draw.appendChild(el('p', 'note', token ? 'キャラを作ると、描いた絵を保存して出せます。' : 'ログインしていなくても描けますが、保存して出すにはゲームでのログインが要ります。'));
		}
	} else {
		draw.appendChild(el('p', '', task.state === 'judging' ? 'いまは選考中です。新しい案は出せません。案を見て「なじんでる」やコメントはできます。' : 'このお題は採用が決まりました。ありがとう！'));
	}
	v.appendChild(draw);

	/* 自分の作業中 */
	const myWips = T.works.filter(function (w) { return w.mine && w.status === 'wip' && !w.deleted; });
	if (myWips.length) {
		v.appendChild(el('h3', '', '自分の作業中'));
		myWips.forEach(function (w) {
			const row = el('div', 'actions');
			row.appendChild(thumbBox(task, w.img, 128, 48, 'node-thumb'));
			const a = el('a', 'btn small', '続きを描く');
			a.href = drawUrl(task, { work: w.id });
			row.appendChild(a);
			const sel = el('button', 'sub light small', '詳しく見る');
			sel.type = 'button';
			sel.addEventListener('click', function () { selectWork(w.id, true); });
			row.appendChild(sel);
			row.appendChild(el('span', 'tag wip', w.share ? '作業中（みんなに見せている）' : '作業中（自分だけ）'));
			v.appendChild(row);
		});
	}

	/* 系譜 */
	v.appendChild(el('h3', '', '案の系譜'));
	v.appendChild(el('p', 'lead', '一から描いた案が根、描き足した案が枝です。'));
	v.appendChild(renderTree());

	const detail = el('div');
	detail.id = 'detail';
	v.appendChild(detail);
	if (selWorkId) {
		renderDetail(false);
	}

	v.appendChild(adminNote());
}

function adminNote() {
	const d = el('p', 'lead', '');
	d.style.marginTop = '24px';
	return d;
}

function renderTree() {
	const works = T.works;
	const byId = {};
	works.forEach(function (w) { byId[w.id] = w; });
	const kids = {};
	const roots = [];
	works.forEach(function (w) {
		if (w.parent && byId[w.parent]) {
			(kids[w.parent] = kids[w.parent] || []).push(w);
		} else {
			roots.push(w);
		}
	});
	function order(a, b) {
		return (a.postedAt || a.at || 0) - (b.postedAt || b.at || 0);
	}
	if (!roots.length) {
		return el('p', 'empty', 'まだ案がありません。' + (T.task.state === 'open' ? '最初の 1 枚を描いてみませんか？' : ''));
	}
	function build(list) {
		const ul = el('ul', list === roots ? 'tree' : '');
		list.sort(order).forEach(function (w) {
			const li = el('li');
			li.appendChild(nodeEl(w));
			if (kids[w.id]) {
				li.appendChild(build(kids[w.id]));
			}
			ul.appendChild(li);
		});
		return ul;
	}
	return build(roots);
}

function nodeEl(w) {
	const task = T.task;
	if (w.deleted) {
		const n = el('div', 'node gone');
		n.appendChild(el('span', '', '消された案'));
		return n;
	}
	const n = el('button', 'node' + (w.status === 'wip' ? ' wipnode' : '') + (w.id === selWorkId ? ' sel' : ''));
	n.type = 'button';
	n.dataset.id = w.id;
	n.appendChild(thumbBox(task, w.img, 256, 96, 'node-thumb'));
	const body = el('div', 'node-body');
	const line = el('div', 'node-line');
	line.appendChild(el('span', 'node-name' + (w.anonymous ? ' anon' : ''), w.anonymous ? '匿名さん' : (w.name || '（名前なし）')));
	if (w.mine) {
		line.appendChild(el('span', 'tag mine', '自分'));
	}
	if (w.status === 'wip') {
		line.appendChild(el('span', 'tag wip', '作業中'));
	}
	if (task.adopted && task.adopted.workId === w.id) {
		line.appendChild(el('span', 'tag adopted', '採用'));
	}
	body.appendChild(line);
	if (w.note) {
		body.appendChild(el('div', 'node-note', w.note));
	}
	body.appendChild(el('div', 'node-stats', 'なじんでる ' + (w.cheers || 0) + '　コメント ' + (w.comments || 0)));
	n.appendChild(body);
	n.addEventListener('click', function () { selectWork(w.id, true); });
	return n;
}

function selectWork(id, bScroll) {
	selWorkId = id;
	const h = '#task=' + encodeURIComponent(T.task.id) + '&work=' + encodeURIComponent(id);
	if (location.hash !== h) {
		history.replaceState(null, '', location.pathname + location.search + h);
	}
	Array.prototype.forEach.call(document.querySelectorAll('.node'), function (n) {
		n.classList.toggle('sel', n.dataset.id === id);
	});
	renderDetail(bScroll);
}

/* ============ 案の詳しい表示 ============ */

function renderDetail(bScroll) {
	const box = $('detail');
	if (preview) {
		preview.destroy();
		preview = null;
	}
	box.textContent = '';
	const w = T.works.find(function (x) { return x.id === selWorkId; });
	if (!w || w.deleted) {
		box.className = '';
		return;
	}
	const task = T.task;
	box.className = 'detail';

	const head = el('div', 'detail-head');
	head.appendChild(el('strong', '', w.anonymous ? '匿名さん' : (w.name || '（名前なし）')));
	if (w.mine) {
		head.appendChild(el('span', 'tag mine', '自分'));
	}
	if (w.status === 'wip') {
		head.appendChild(el('span', 'tag wip', w.share ? '作業中（みんなに見せている）' : '作業中（自分だけ）'));
	}
	if (task.adopted && task.adopted.workId === w.id) {
		head.appendChild(el('span', 'tag adopted', '採用'));
	}
	head.appendChild(el('span', 'comment-date', dateText(w.postedAt || w.updatedAt || w.at)));
	box.appendChild(head);
	if (w.note) {
		box.appendChild(el('div', 'task-desc', w.note));
	}

	/* 拡大表示 */
	const sb = el('div', 'scale-btns');
	sb.appendChild(el('span', '', '大きさ：'));
	const bigbox = el('div', 'bigbox checker');
	const bigCv = el('canvas', 'px');
	bigbox.appendChild(bigCv);
	for (let s = 2; s <= 8; s++) {
		const b = el('button', s === bigScale ? 'active' : '', s + '倍');
		b.type = 'button';
		b.addEventListener('click', function () {
			bigScale = s;
			Array.prototype.forEach.call(sb.querySelectorAll('button'), function (x) { x.classList.toggle('active', x === b); });
			applyBig();
		});
		sb.appendChild(b);
	}
	box.appendChild(sb);
	box.appendChild(bigbox);
	function applyBig() {
		if (bigCv.width) {
			bigCv.style.width = (bigCv.width * bigScale) + 'px';
			bigCv.style.height = (bigCv.height * bigScale) + 'px';
		}
	}

	/* 等倍で背景の上に・見本と並べる、プレビュー */
	const compare = el('div', 'compare');
	box.appendChild(el('h4', '', 'ゲームでの見え方（等倍）')).style.marginTop = '14px';
	box.appendChild(compare);
	const pvHead = el('h4', '', 'プレビュー');
	box.appendChild(pvHead);
	const pvWrap = el('div', 'pv-wrap');
	box.appendChild(pvWrap);

	Promise.all([loadIndexed(w.img), loadBgCanvas(task)]).then(function (r) {
		if (selWorkId !== w.id) {
			return;
		}
		const img = r[0];
		const bg = r[1];
		const pal = displayPalette(task);
		const cv = indicesToCanvas(img.indices, img.width, img.height, pal);
		bigCv.width = cv.width;
		bigCv.height = cv.height;
		const ctx = bigCv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.drawImage(cv, 0, 0);
		applyBig();

		function fig(label, canvas) {
			const f = el('figure');
			f.appendChild(el('figcaption', '', label));
			canvas.className = 'px';
			f.appendChild(canvas);
			compare.appendChild(f);
		}
		fig('この案', composeOnBg(bg, cv, 4));
		const smp = task.samples || [];
		Promise.all(smp.map(function (id) { return loadIndexed(id).catch(function () { return null; }); })).then(function (arr) {
			if (selWorkId !== w.id) {
				return;
			}
			arr.forEach(function (s) {
				if (s) {
					const c = indicesToCanvas(s.indices, s.width, s.height, pal);
					fig('見本', composeOnBg(bg, c, 4));
				}
			});
		});

		preview = makePreview(pvWrap, task, { scale: 2 });
		preview.update(img.indices, img.width, img.height);
	}).catch(function () {
		box.appendChild(el('p', 'empty', '絵を読み込めませんでした。'));
	});

	/* ボタン */
	const act = el('div', 'actions');
	if (w.status === 'posted' && task.state === 'open') {
		const a = el('a', 'btn', 'この案を元に描く');
		a.href = drawUrl(task, { from: w.id });
		act.appendChild(a);
	}
	if (w.mine && w.status === 'wip') {
		const a = el('a', 'btn', '続きを描く');
		a.href = drawUrl(task, { work: w.id });
		act.appendChild(a);
		const d = el('button', 'sub light', '消す');
		d.type = 'button';
		d.addEventListener('click', function () {
			if (!window.confirm('この作業中の案を消しますか？（元に戻せません）')) {
				return;
			}
			api('/api/kobo/delete', { token: token, id: w.id }).then(function (data) {
				if (data.ok) {
					selWorkId = '';
					history.replaceState(null, '', location.pathname + '#task=' + encodeURIComponent(task.id));
					loadTask(task.id, true);
				} else {
					window.alert(errText(data));
				}
			});
		});
		act.appendChild(d);
	}
	if (!w.mine && w.status === 'posted') {
		const on = T.myCheers.indexOf(w.id) >= 0;
		const c = el('button', 'cheer' + (on ? ' on' : ''), (on ? '✓ ' : '') + 'なじんでる（' + (w.cheers || 0) + '）');
		c.type = 'button';
		if (!canAct()) {
			c.disabled = true;
			c.title = 'ゲームにログインすると押せます';
		}
		c.addEventListener('click', function () {
			c.disabled = true;
			api('/api/kobo/cheer', { token: token, workId: w.id, on: !on }).then(function (data) {
				if (data.ok) {
					if (!on) {
						T.myCheers.push(w.id);
					} else {
						T.myCheers = T.myCheers.filter(function (x) { return x !== w.id; });
					}
					w.cheers = data.cheers != null ? data.cheers : w.cheers;
					renderTask();
				} else {
					c.disabled = false;
					window.alert(errText(data));
				}
			}).catch(function () { c.disabled = false; });
		});
		act.appendChild(c);
	} else if (w.mine && w.status === 'posted') {
		act.appendChild(el('span', 'tag', 'なじんでる ' + (w.cheers || 0) + '（自分の案には付けられません）'));
	}
	box.appendChild(act);

	box.appendChild(renderComments(w));
	if (bScroll) {
		box.scrollIntoView({ behavior: 'smooth', block: 'start' });
	}
}

/* ============ コメント ============ */

function renderComments(w) {
	const wrap = el('div');
	wrap.appendChild(el('h4', '', 'コメント')).style.marginTop = '14px';
	const list = T.comments.filter(function (c) { return c.workId === w.id; });
	const byId = {};
	list.forEach(function (c) { byId[c.id] = c; });
	const tops = [];
	const replies = {};
	list.forEach(function (c) {
		let top = c;
		let guard = 0;
		while (top.replyTo && byId[top.replyTo] && guard++ < 10) {
			top = byId[top.replyTo];
		}
		if (top === c) {
			tops.push(c);
		} else {
			(replies[top.id] = replies[top.id] || []).push(c);
		}
	});
	tops.sort(function (a, b) { return a.at - b.at; });
	if (!tops.length) {
		wrap.appendChild(el('p', 'empty', 'まだコメントはありません。'));
	}
	tops.forEach(function (c) {
		wrap.appendChild(commentEl(c, w, false));
		const slot = el('div');
		wrap.appendChild(slot);
		(replies[c.id] || []).sort(function (a, b) { return a.at - b.at; }).forEach(function (r) {
			slot.appendChild(commentEl(r, w, true));
		});
		if (w.mine && !c.hiddenByAuthor) {
			/* 返事の入力は「返事」ボタンで開く */
			c._slot = slot;
		}
	});

	/* 書く */
	const form = el('div', 'cform');
	form.style.marginTop = '12px';
	if (!canAct()) {
		form.appendChild(el('p', 'lead', token ? 'キャラを作ると、コメントを書けます。' : 'コメントを書くには、ゲームにログインしてね。'));
	} else {
		const ta = el('textarea');
		ta.maxLength = 600;
		ta.placeholder = 'この案へのコメント（直したほうがよいところ、よいところなど）';
		const cnt = el('div', 'count', '0 / 300');
		const msg = el('div', 'msg');
		const btn = el('button', '', 'コメントする');
		btn.type = 'button';
		ta.addEventListener('input', function () {
			const n = Array.from(ta.value.trim()).length;
			cnt.textContent = n + ' / 300';
			cnt.classList.toggle('over', n > 300);
		});
		btn.addEventListener('click', function () {
			const n = Array.from(ta.value.trim()).length;
			if (!n) {
				msg.textContent = errText({ error: 'empty' });
				return;
			}
			if (n > 300) {
				msg.textContent = errText({ error: 'long' });
				return;
			}
			btn.disabled = true;
			msg.textContent = '';
			api('/api/kobo/comment', { token: token, workId: w.id, body: ta.value }).then(function (data) {
				btn.disabled = false;
				if (data.ok) {
					loadTask(T.task.id, true);
				} else {
					msg.textContent = errText(data);
				}
			}).catch(function () { btn.disabled = false; msg.textContent = 'サーバーにつながりませんでした。'; });
		});
		form.appendChild(ta);
		form.appendChild(cnt);
		form.appendChild(msg);
		const bw = el('div', 'buttons');
		bw.appendChild(btn);
		form.appendChild(bw);
		if (!w.mine) {
			form.appendChild(el('p', 'lead', 'コメントへの返事ができるのは、その案を描いた本人と管理人だけです。')).style.marginTop = '8px';
		}
	}
	wrap.appendChild(form);
	return wrap;
}

function commentEl(c, w, bReply) {
	const box = el('div', 'comment' + (bReply ? ' reply' : '') + (c.hiddenByAuthor ? ' hiddenc' : ''));
	const head = el('div', 'comment-head');
	head.appendChild(el('span', 'comment-name', c.admin ? '管理人' : (c.anonymous ? '匿名さん' : (c.name || '（名前なし）'))));
	if (c.admin) {
		head.appendChild(el('span', 'tag admin', '管理人'));
	}
	if (c.mine) {
		head.appendChild(el('span', 'tag mine', '自分'));
	}
	if (c.hiddenByAuthor) {
		head.appendChild(el('span', 'tag', '隠しています（自分にだけ見えます）'));
	}
	head.appendChild(el('span', 'comment-date', dateText(c.at)));
	box.appendChild(head);
	box.appendChild(el('div', 'comment-body', c.body));

	const acts = el('div', 'comment-actions');
	if (w.mine && !bReply) {
		const r = el('button', '', '返事');
		r.type = 'button';
		r.addEventListener('click', function () { openReply(box, c, w); });
		acts.appendChild(r);
	}
	if (w.mine && !c.mine && !c.admin) {
		const h = el('button', '', c.hiddenByAuthor ? '戻す' : '隠す');
		h.type = 'button';
		h.addEventListener('click', function () {
			api('/api/kobo/hide-comment', { token: token, id: c.id, hide: !c.hiddenByAuthor }).then(function (data) {
				if (data.ok) {
					loadTask(T.task.id, true);
				} else {
					window.alert(errText(data));
				}
			});
		});
		acts.appendChild(h);
	}
	if (acts.childNodes.length) {
		box.appendChild(acts);
	}
	return box;
}

function openReply(commentBox, c, w) {
	const old = commentBox.parentNode.querySelector('.reply-form[data-for="' + c.id + '"]');
	if (old) {
		old.remove();
		return;
	}
	const f = el('div', 'reply-form cform');
	f.dataset.for = c.id;
	const ta = el('textarea');
	ta.maxLength = 600;
	ta.placeholder = '返事（300字まで）';
	const msg = el('div', 'msg');
	const b = el('button', 'small', '返事を送る');
	b.type = 'button';
	b.addEventListener('click', function () {
		const n = Array.from(ta.value.trim()).length;
		if (!n || n > 300) {
			msg.textContent = errText({ error: n ? 'long' : 'empty' });
			return;
		}
		b.disabled = true;
		api('/api/kobo/comment', { token: token, workId: w.id, body: ta.value, replyTo: c.id }).then(function (data) {
			b.disabled = false;
			if (data.ok) {
				loadTask(T.task.id, true);
			} else {
				msg.textContent = errText(data);
			}
		}).catch(function () { b.disabled = false; msg.textContent = 'サーバーにつながりませんでした。'; });
	});
	f.appendChild(ta);
	f.appendChild(msg);
	const bw = el('div', 'buttons');
	bw.appendChild(b);
	f.appendChild(bw);
	/* 返事の欄は、そのコメントの返事の並びの先頭に出す */
	const next = commentBox.nextSibling;
	commentBox.parentNode.insertBefore(f, next);
	ta.focus();
}

/* ============ 提案（こういうのどう？） ============ */

/* 小さなお絵かきの色は、種類ごとのゲーム画像のパレット（そこで使われている色だけ）。透明はパレットの黒 */
const PAD_SHEET = { char: 'char01.png', part: 'map01.png', effect: 'efc01.png' };
/* 描ける大きさ（ゲームのコマに合わせる） */
const PAD_SIZES = {
	char: [[16, 16], [16, 32], [32, 32], [48, 48]],
	part: [[16, 16], [32, 32]],
	effect: [[16, 16], [32, 32]]
};
const padSheetCache = new Map();
function loadPadSheet(kind) {
	const name = PAD_SHEET[kind] || PAD_SHEET.char;
	if (!padSheetCache.has(name)) {
		const pr = fetch('/admin-grp/' + encodeURIComponent(name)).then(function (res) {
			if (!res.ok) {
				throw new Error('sheet ' + res.status);
			}
			return res.arrayBuffer();
		}).then(function (buf) {
			return decodeIndexedPng(buf);
		}).then(function (img) {
			const palette = [];
			for (let i = 0; i < 256; i++) {
				const c = img.palette[i];
				palette.push(c ? { r: c.r, g: c.g, b: c.b, a: 255 } : { r: 0, g: 0, b: 0, a: 255 });
			}
			let transparent = 0;
			for (let i = 0; i < palette.length; i++) {
				if (!palette[i].r && !palette[i].g && !palette[i].b) {
					transparent = i;
					break;
				}
			}
			const used = new Set([transparent]);
			for (let i = 0; i < img.indices.length; i++) {
				used.add(img.indices[i]);
			}
			const colors = Array.from(used).sort(function (a, b) { return a - b; });
			return { palette: palette, transparent: transparent, colors: colors };
		});
		pr.catch(function () { padSheetCache.delete(name); });
		padSheetCache.set(name, pr);
	}
	return padSheetCache.get(name);
}

function bytesToBase64(bytes) {
	let bin = '';
	for (let i = 0; i < bytes.length; i += 0x8000) {
		bin += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
	}
	return btoa(bin);
}

let proposeOpen = false;	// 提案の入力欄を開いているか

function renderPropose() {
	const box = $('propose');
	box.textContent = '';
	if (!canAct()) {
		const p = el('section', 'panel');
		p.appendChild(el('p', '', token ? '先にゲームでキャラを作ってから提案してね。' : '提案するには、このブラウザでゲームにログインしてね。'));
		const bw = el('div', 'buttons');
		const a = el('a', 'btn', 'ゲームを開く');
		a.href = '/';
		bw.appendChild(a);
		p.appendChild(bw);
		box.appendChild(p);
		return;
	}
	const p = el('section', 'panel');
	p.appendChild(el('p', 'now', '「こんなものを描いてみたい」「ここのパーツがほしい」など、お題の案を管理人に送れます。見えるのは管理人だけです。'));

	const l1 = el('span', 'field-label', 'どんなお題？');
	p.appendChild(l1);
	const kinds = el('div', 'choices cats');
	[['char', 'NPC・敵'], ['part', '背景パーツ'], ['effect', 'エフェクト']].forEach(function (k, i) {
		const lb = el('label');
		const inp = el('input');
		inp.type = 'radio';
		inp.name = 'pk';
		inp.value = k[0];
		inp.checked = i === 0;
		lb.appendChild(inp);
		lb.appendChild(el('span', '', k[1]));
		kinds.appendChild(lb);
	});
	p.appendChild(kinds);

	p.appendChild(el('span', 'field-label', 'ひとこと（どんなものを描きたい？）'));
	const ta = el('textarea');
	ta.placeholder = '例）海辺のマップに、ヤドカリのようなモンスターがほしいです';
	p.appendChild(ta);
	const cnt = el('div', 'count', '0 / 1000');
	ta.addEventListener('input', function () {
		const n = Array.from(ta.value.trim()).length;
		cnt.textContent = n + ' / 1000';
		cnt.classList.toggle('over', n > 1000);
	});
	p.appendChild(cnt);

	/* ラフ絵（任意）。お絵かきで描いたものか、選んだ画像のどちらか（最後に使った方）を送る */
	p.appendChild(el('span', 'field-label', 'ラフ絵（なくても大丈夫）'));
	p.appendChild(el('p', 'note', '下のお絵かきで描くと、その絵がそのまま一緒に送られます。色はゲームのパレット、大きさはゲームのコマと同じです（右クリックで色を拾えます）。'));
	let roughSrc = '';		// '' | 'pad' | 'file'
	let fileData = '';		// 選んだ画像の PNG（base64）

	/* 小さなお絵かき */
	const padBar = el('div', 'buttons');
	padBar.style.alignItems = 'center';
	padBar.appendChild(el('span', '', '大きさ'));
	const sizeSel = el('select');
	padBar.appendChild(sizeSel);
	const pad = el('div', 'draw-pad');
	pad.style.marginTop = '8px';
	const cv = el('canvas');
	cv.style.cursor = 'crosshair';
	const tools = el('div', 'pad-tools');
	const er = el('button', 'sub small', '消しゴム');
	er.type = 'button';
	const clr = el('button', 'sub small', '全部消す');
	clr.type = 'button';
	const padNote = el('div', 'note');
	const side = el('div');
	side.appendChild(tools);
	const sb = el('div', 'buttons');
	sb.appendChild(er);
	sb.appendChild(clr);
	side.appendChild(sb);
	side.appendChild(padNote);
	pad.appendChild(cv);
	pad.appendChild(side);
	p.appendChild(padBar);
	p.appendChild(pad);

	let sheet = null;		// {palette, transparent, colors}
	let W = 16, H = 16, Z = 16;
	let px = new Uint8Array(W * H);
	let cur = 0;			// 今の色（パレット番号）
	let drawn = false;
	const ctx = cv.getContext('2d');

	function kindNow() {
		return (document.querySelector('input[name="pk"]:checked') || {}).value || 'char';
	}
	function css(i) {
		const c = sheet.palette[i];
		return 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')';
	}
	function redraw() {
		if (!sheet) {
			return;
		}
		for (let y = 0; y < H; y++) {
			for (let x = 0; x < W; x++) {
				const v = px[y * W + x];
				if (v === sheet.transparent) {
					/* 透明は市松模様（1 ドットを 4 つに割る） */
					const h2 = Z / 2;
					ctx.fillStyle = '#ffffff';
					ctx.fillRect(x * Z, y * Z, Z, Z);
					ctx.fillStyle = '#d6d6d6';
					ctx.fillRect(x * Z, y * Z, h2, h2);
					ctx.fillRect(x * Z + h2, y * Z + h2, Z - h2, Z - h2);
				} else {
					ctx.fillStyle = css(v);
					ctx.fillRect(x * Z, y * Z, Z, Z);
				}
			}
		}
		/* マス目（8 ドットごとに濃く） */
		ctx.lineWidth = 1;
		for (let x = 0; x <= W; x++) {
			ctx.strokeStyle = x % 8 === 0 ? 'rgba(0,0,0,0.35)' : 'rgba(0,0,0,0.1)';
			ctx.beginPath();
			ctx.moveTo(x * Z + 0.5, 0);
			ctx.lineTo(x * Z + 0.5, H * Z);
			ctx.stroke();
		}
		for (let y = 0; y <= H; y++) {
			ctx.strokeStyle = y % 8 === 0 ? 'rgba(0,0,0,0.35)' : 'rgba(0,0,0,0.1)';
			ctx.beginPath();
			ctx.moveTo(0, y * Z + 0.5);
			ctx.lineTo(W * Z, y * Z + 0.5);
			ctx.stroke();
		}
	}
	function selectColor(i) {
		cur = i;
		Array.prototype.forEach.call(tools.querySelectorAll('.sw'), function (b) {
			b.classList.toggle('sel', Number(b.dataset.idx) === i);
		});
		er.classList.toggle('sel', i === sheet.transparent);
	}
	/* 管理者用エディタと同じく 256 色を 16×16 に番号順で並べ、色もそのまま見せる。元の画像で使っている番号は右下に白い印 */
	function buildTools() {
		tools.textContent = '';
		const used = new Set(sheet.colors);
		for (let i = 0; i < 256; i++) {
			const c = sheet.palette[i];
			const b = el('button', 'sw' + (!c.r && !c.g && !c.b ? ' black' : '') + (used.has(i) ? ' use' : ''));
			b.type = 'button';
			b.dataset.idx = String(i);
			b.style.background = css(i);
			b.title = '番号 ' + i + (i === sheet.transparent ? '（透明）' : '  (' + c.r + ', ' + c.g + ', ' + c.b + ')');
			b.addEventListener('click', function () { selectColor(i); });
			tools.appendChild(b);
		}
	}
	function resetPad() {
		const sz = (sizeSel.value || '16x16').split('x');
		W = Number(sz[0]);
		H = Number(sz[1]);
		Z = Math.max(4, Math.floor(256 / Math.max(W, H)));
		cv.width = W * Z + 1;
		cv.height = H * Z + 1;
		px = new Uint8Array(W * H).fill(sheet ? sheet.transparent : 0);
		drawn = false;
		if (roughSrc === 'pad') {
			roughSrc = '';
		}
		padNote.textContent = W + '×' + H + ' ドット';
		redraw();
	}
	function loadKind(kind) {
		sizeSel.textContent = '';
		(PAD_SIZES[kind] || PAD_SIZES.char).forEach(function (s) {
			const o = el('option', '', s[0] + '×' + s[1]);
			o.value = s[0] + 'x' + s[1];
			sizeSel.appendChild(o);
		});
		padNote.textContent = '色を読み込んでいます…';
		return loadPadSheet(kind).then(function (sh) {
			sheet = sh;
			buildTools();
			resetPad();
			const first = sheet.colors.find(function (i) { return i !== sheet.transparent; });
			selectColor(first == null ? sheet.transparent : first);
		}).catch(function () {
			padNote.textContent = '色を読み込めませんでした。画像を選んで送ってください。';
		});
	}
	function confirmClear() {
		return !drawn || window.confirm('描いた絵を消して変えますか？');
	}
	let prevKind = 'char';
	Array.prototype.forEach.call(kinds.querySelectorAll('input'), function (inp) {
		inp.addEventListener('change', function () {
			if (!confirmClear()) {
				kinds.querySelector('input[value="' + prevKind + '"]').checked = true;
				return;
			}
			prevKind = kindNow();
			loadKind(prevKind);
		});
	});
	let prevSize = '16x16';
	sizeSel.addEventListener('change', function () {
		if (!confirmClear()) {
			sizeSel.value = prevSize;
			return;
		}
		prevSize = sizeSel.value;
		resetPad();
	});
	er.addEventListener('click', function () { selectColor(sheet.transparent); });
	clr.addEventListener('click', function () {
		if (!confirmClear()) {
			return;
		}
		resetPad();
	});

	let last = null;
	function padPoint(e) {
		const r = cv.getBoundingClientRect();
		return [Math.floor((e.clientX - r.left) / r.width * (cv.width / Z)), Math.floor((e.clientY - r.top) / r.height * (cv.height / Z))];
	}
	function plot(x, y) {
		if (x < 0 || y < 0 || x >= W || y >= H) {
			return;
		}
		px[y * W + x] = cur;
		drawn = true;
		roughSrc = 'pad';
		file.value = '';
		fileData = '';
		prev.textContent = '';
	}
	function line(a, b) {
		let [x0, y0] = a;
		const [x1, y1] = b;
		const dx = Math.abs(x1 - x0), dy = -Math.abs(y1 - y0);
		const sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
		let err = dx + dy;
		for (;;) {
			plot(x0, y0);
			if (x0 === x1 && y0 === y1) { break; }
			const e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
		}
	}
	cv.addEventListener('contextmenu', function (e) { e.preventDefault(); });
	cv.addEventListener('pointerdown', function (e) {
		if (!sheet) {
			return;
		}
		e.preventDefault();
		const pt = padPoint(e);
		/* 右クリック＝スポイト（その色を拾う） */
		if (e.button === 2) {
			if (pt[0] >= 0 && pt[1] >= 0 && pt[0] < W && pt[1] < H) {
				selectColor(px[pt[1] * W + pt[0]]);
			}
			return;
		}
		if (e.button !== 0) {
			return;
		}
		cv.setPointerCapture(e.pointerId);
		last = pt;
		plot(last[0], last[1]);
		redraw();
	});
	cv.addEventListener('pointermove', function (e) {
		if (!last) {
			return;
		}
		const pt = padPoint(e);
		line(last, pt);
		last = pt;
		redraw();
	});
	function up() { last = null; }
	cv.addEventListener('pointerup', up);
	cv.addEventListener('pointercancel', up);

	/* 画像を選んで送る（選ぶとお絵かきの代わりにこちらを送る） */
	const prev = el('div', 'rough-preview');
	const fileRow = el('div');
	fileRow.style.marginTop = '10px';
	fileRow.appendChild(el('span', 'note', '手元の画像を送る場合はこちら：'));
	const file = el('input');
	file.type = 'file';
	file.accept = 'image/png,image/jpeg,image/gif,image/webp';
	file.style.color = 'var(--panel-fg)';
	fileRow.appendChild(file);
	p.appendChild(fileRow);
	p.appendChild(prev);
	file.addEventListener('change', function () {
		const f = file.files && file.files[0];
		if (!f) {
			return;
		}
		const url = URL.createObjectURL(f);
		const im = new Image();
		im.onload = function () {
			/* 大きすぎる絵は 256 マスに縮める（PNG は 256KB まで） */
			const k = Math.min(1, 256 / Math.max(im.width, im.height));
			const c = document.createElement('canvas');
			c.width = Math.max(1, Math.round(im.width * k));
			c.height = Math.max(1, Math.round(im.height * k));
			const cx = c.getContext('2d');
			cx.imageSmoothingEnabled = k >= 1 ? false : true;
			cx.drawImage(im, 0, 0, c.width, c.height);
			URL.revokeObjectURL(url);
			fileData = c.toDataURL('image/png').split(',')[1];
			roughSrc = 'file';
			prev.textContent = '';
			const img = el('img');
			img.src = 'data:image/png;base64,' + fileData;
			prev.appendChild(img);
			prev.appendChild(el('span', 'note', ' この画像を送ります（お絵かきの絵は送りません）'));
			const rm = el('button', 'sub small', '画像をやめる');
			rm.type = 'button';
			rm.style.marginLeft = '8px';
			rm.addEventListener('click', function () {
				fileData = '';
				file.value = '';
				prev.textContent = '';
				roughSrc = drawn ? 'pad' : '';
			});
			prev.appendChild(rm);
		};
		im.onerror = function () {
			URL.revokeObjectURL(url);
			msg.textContent = 'その画像は読み込めませんでした。';
		};
		im.src = url;
	});
	/* 送る時のラフ絵（base64 PNG。無ければ ''） */
	function roughData() {
		if (roughSrc === 'file' && fileData) {
			return Promise.resolve(fileData);
		}
		if (roughSrc === 'pad' && drawn && sheet) {
			return encodeIndexedPng({ width: W, height: H, indices: px, palette: sheet.palette }).then(bytesToBase64);
		}
		return Promise.resolve('');
	}
	loadKind('char');

	/* 名前 */
	p.appendChild(el('span', 'field-label', '名前は？'));
	const nm = el('div', 'choices');
	const l0 = el('label');
	const r0 = el('input');
	r0.type = 'radio'; r0.name = 'pa'; r0.value = '0'; r0.checked = true;
	l0.appendChild(r0);
	const s0 = el('span', '', 'キャラ名を出す');
	s0.appendChild(el('small', '', '「' + (me.name || '') + '」と出ます'));
	l0.appendChild(s0);
	const l1b = el('label');
	const r1 = el('input');
	r1.type = 'radio'; r1.name = 'pa'; r1.value = '1';
	l1b.appendChild(r1);
	const s1 = el('span', '', '匿名にする');
	s1.appendChild(el('small', '', '「匿名さん」と出ます'));
	l1b.appendChild(s1);
	nm.appendChild(l0);
	nm.appendChild(l1b);
	p.appendChild(nm);
	p.appendChild(el('p', 'note', '匿名にしても、管理人にはだれの提案か分かります。匿名で出した提案は、お題になっても匿名のままです。'));

	/* 同意 */
	const ag = el('label', 'agree');
	const cb = el('input');
	cb.type = 'checkbox';
	ag.appendChild(cb);
	const agText = el('div');
	agText.appendChild(el('div', '', '次のことに同意します'));
	const ul = el('ul');
	AGREE_LINES.forEach(function (t) { ul.appendChild(el('li', '', t)); });
	agText.appendChild(ul);
	ag.appendChild(agText);
	p.appendChild(ag);

	const msg = el('div', 'msg');
	p.appendChild(msg);
	const bw = el('div', 'buttons');
	const send = el('button', '', '提案する');
	send.type = 'button';
	bw.appendChild(send);
	p.appendChild(bw);
	send.addEventListener('click', function () {
		const n = Array.from(ta.value.trim()).length;
		if (!n) {
			msg.textContent = errText({ error: 'empty' });
			return;
		}
		if (n > 1000) {
			msg.textContent = errText({ error: 'long' });
			return;
		}
		if (!cb.checked) {
			msg.textContent = errText({ error: 'agree' });
			return;
		}
		const kind = (document.querySelector('input[name="pk"]:checked') || {}).value;
		const anon = (document.querySelector('input[name="pa"]:checked') || {}).value === '1';
		if (!window.confirm('管理人に' + (anon ? '匿名で' : 'キャラ名つきで') + '提案を送ります。よろしいですか？')) {
			return;
		}
		send.disabled = true;
		msg.textContent = '';
		const body = { token: token, kind: kind, body: ta.value, anonymous: anon, agree: true };
		roughData().then(function (data64) {
			if (data64) {
				body.data = data64;
			}
			return api('/api/kobo/propose', body);
		}).then(function (data) {
			send.disabled = false;
			if (data.ok) {
				renderPropose();
				const ok = el('div', 'msg ok', '送りました！ありがとう。管理人の返事は下の「自分の提案」で見られます。');
				$('propose').insertBefore(ok, $('propose').firstChild);
				loadMyProposals();
			} else {
				msg.textContent = errText(data);
			}
		}).catch(function () { send.disabled = false; msg.textContent = 'サーバーにつながりませんでした。'; });
	});
	/* 入力欄とお絵かきはふだん閉じておき、「お題を提案する」で開く（送った後は開いたまま） */
	p.hidden = !proposeOpen;
	const openWrap = el('div', 'buttons');
	const openBtn = el('button', '', 'お題を提案する');
	openBtn.type = 'button';
	openBtn.addEventListener('click', function () {
		proposeOpen = true;
		p.hidden = false;
		openWrap.hidden = true;
		ta.focus();
	});
	openWrap.appendChild(openBtn);
	openWrap.hidden = proposeOpen;
	box.appendChild(openWrap);
	box.appendChild(p);

	box.appendChild(el('h3', '', '自分の提案'));
	const mine = el('div');
	mine.id = 'my-proposals';
	box.appendChild(mine);
	loadMyProposals();
}

function loadMyProposals() {
	const box = $('my-proposals');
	if (!box) {
		return;
	}
	api('/api/kobo/my-proposals', { token: token }).then(function (data) {
		box.textContent = '';
		const list = data.proposals || [];
		if (!list.length) {
			box.appendChild(el('p', 'empty', 'まだ提案していません。'));
			return;
		}
		list.slice().sort(function (a, b) { return b.at - a.at; }).forEach(function (pr) {
			const d = el('div', 'prop');
			const h = el('div', 'node-line');
			h.appendChild(el('span', 'tag k-' + pr.kind, KIND_TEXT[pr.kind] || pr.kind));
			h.appendChild(el('span', 'tag', PROPOSAL_STATUS[pr.status] || pr.status));
			if (pr.anonymous) {
				h.appendChild(el('span', 'tag', '匿名'));
			}
			h.appendChild(el('span', 'comment-date', dateText(pr.at)));
			d.appendChild(h);
			d.appendChild(el('div', 'prop-body', pr.body));
			if (pr.img) {
				const im = el('img');
				im.src = imgUrl(pr.img);
				im.alt = 'ラフ絵';
				d.appendChild(im);
			}
			if (pr.reply) {
				const r = el('div', 'reply');
				r.appendChild(el('div', 'comment-head', '管理人からの返事' + (pr.replyAt ? '（' + dateText(pr.replyAt) + '）' : '')));
				r.firstChild.style.fontWeight = 'bold';
				r.appendChild(el('div', 'prop-body', pr.reply));
				d.appendChild(r);
			}
			if (pr.status === 'tasked' && pr.taskId) {
				const a = el('a', '', 'お題を見る');
				a.href = '#task=' + encodeURIComponent(pr.taskId);
				d.appendChild(a);
			}
			box.appendChild(d);
		});
	}).catch(function () {
		box.textContent = '';
		box.appendChild(el('p', 'empty', 'サーバーにつながりませんでした。'));
	});
}

/* ============ 道案内 ============ */

function route() {
	const q = new URLSearchParams(location.hash.slice(1));
	const id = q.get('task');
	if (!id) {
		selWorkId = '';
		T = null;
		if (preview) {
			preview.destroy();
			preview = null;
		}
		showView('list');
		loadList();
		return;
	}
	selWorkId = q.get('work') || '';
	loadTask(id, false).then(function () {
		if (selWorkId && T && T.works.some(function (w) { return w.id === selWorkId; })) {
			renderDetail(true);
			const n = document.querySelector('.node[data-id="' + selWorkId + '"]');
			if (n) {
				n.classList.add('sel');
			}
		}
	});
}

window.addEventListener('hashchange', function () {
	/* 案を選んだだけの hash 変更（replaceState）では来ない。お題の移動の時だけ読み直す */
	route();
});

function init() {
	if (!token) {
		setStatusPanel();
		renderPropose();
		route();
		return;
	}
	api('/api/kobo/me', { token: token }).then(function (data) {
		if (data.httpStatus === 401 || !data.account) {
			token = '';
			me = null;
		} else {
			me = data;
		}
	}).catch(function () {
		me = null;
	}).then(function () {
		setStatusPanel();
		renderPropose();
		route();
	});
}

init();
