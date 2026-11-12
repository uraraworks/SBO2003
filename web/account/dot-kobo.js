/**
 * dot-kobo.js
 * ドット絵工房（/kobo・/kobo-draw）の共通部品（ES module）
 *
 * 一覧・お題のページ（kobo.js）と描く画面（kobo-draw.js）の両方が使う。
 *   - API の呼び方・端末トークン・小さな道具
 *   - サーバーの画像（8bit パレット PNG）の読み込み
 *   - 種類ごと（walk / tile / anim）のプレビュー
 *
 * ファイル名を dot-*.js にしてあるのは、tools/web/build.sh が dot-*.js をまとめて配るため。
 */
import { decodeIndexedPng, indicesToImageData } from './dot-png.js';

export const KEY_TOKEN = 'sbo_device_token';

export function lsGet(k) {
	try { return window.localStorage.getItem(k); } catch (e) { return null; }
}
export function lsSet(k, v) {
	try { window.localStorage.setItem(k, v); } catch (e) { /* 保存できなくても動く */ }
}
export function lsDel(k) {
	try { window.localStorage.removeItem(k); } catch (e) { /* 同上 */ }
}

/** POST JSON。HTTP の状態は httpStatus に入れて返す（通信できない時だけ reject） */
export function api(url, body) {
	return fetch(url, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(body)
	}).then(function (res) {
		return res.json().catch(function () { return {}; }).then(function (data) {
			data.httpStatus = res.status;
			return data;
		});
	});
}

export function el(tag, cls, text) {
	const e = document.createElement(tag);
	if (cls) {
		e.className = cls;
	}
	if (text != null) {
		e.textContent = text;
	}
	return e;
}

export function dateText(ms) {
	if (!ms) {
		return '';
	}
	return new Date(ms).toLocaleString('ja-JP', { year: 'numeric', month: 'numeric', day: 'numeric', hour: '2-digit', minute: '2-digit' });
}

/** 名前の見せ方（匿名は「匿名さん」） */
export function displayName(o) {
	if (o.admin) {
		return '管理人';
	}
	return o.anonymous ? '匿名さん' : (o.name || '（名前なし）');
}

export function imgUrl(id, ext) {
	return '/kobo-img/' + id + '.' + (ext || 'png');
}

/** 元イラスト用の img（PNG か JPEG かは id だけでは分からないので、だめなら .jpg を試す） */
export function refImgEl(id, cls) {
	const img = el('img', cls || '');
	let triedJpg = false;
	img.alt = '元イラスト';
	img.addEventListener('error', function () {
		if (!triedJpg) {
			triedJpg = true;
			img.src = imgUrl(id, 'jpg');
		}
	});
	img.src = imgUrl(id, 'png');
	return img;
}

/** 整数倍で box に収まる倍率（最低 1） */
export function fitScale(w, h, boxW, boxH) {
	return Math.max(1, Math.min(Math.floor(boxW / w), Math.floor(boxH / h)));
}

/** 画像（パレット PNG）を読み込む。同じ id は 1 回だけ。 */
const imgCache = new Map();
export function loadIndexed(id) {
	if (!imgCache.has(id)) {
		const p = fetch(imgUrl(id)).then(function (res) {
			if (!res.ok) {
				throw new Error('image ' + res.status);
			}
			return res.arrayBuffer();
		}).then(function (buf) {
			return decodeIndexedPng(buf);
		});
		p.catch(function () { imgCache.delete(id); });
		imgCache.set(id, p);
	}
	return imgCache.get(id);
}

/** 表示用のパレット（お題のパレット。透明の番号は alpha 0） */
export function displayPalette(task) {
	return task.palette.map(function (c, i) {
		return { r: c[0], g: c[1], b: c[2], a: i === task.transparent ? 0 : 255 };
	});
}

export function indicesToCanvas(indices, w, h, pal) {
	const c = document.createElement('canvas');
	c.width = w;
	c.height = h;
	c.getContext('2d').putImageData(indicesToImageData(indices, w, h, pal), 0, 0);
	return c;
}

/** お題の背景（task.bg の 1 マス）を canvas にする。無ければ null */
export function loadBgCanvas(task) {
	if (!task.bg) {
		return Promise.resolve(null);
	}
	return loadIndexed(task.bg).then(function (img) {
		const pal = img.palette.map(function (c) { return { r: c.r, g: c.g, b: c.b, a: 255 }; });
		return indicesToCanvas(img.indices, img.width, img.height, pal);
	}).catch(function () { return null; });
}


/**
 * 小さな絵（canvas）。透明の番号は本当に透明にするので、台の市松模様が見える。
 * 読み込むまでは task.w × task.h の大きさの場所だけ取っておく。
 */
export function thumbCanvas(task, id, scale) {
	const cv = document.createElement('canvas');
	cv.width = task.w;
	cv.height = task.h;
	cv.className = 'px';
	cv.style.imageRendering = 'pixelated';	// どのページで使ってもぼかさない
	cv.style.width = (task.w * scale) + 'px';
	cv.style.maxWidth = '100%';
	cv.style.height = 'auto';
	loadIndexed(id).then(function (img) {
		const c = indicesToCanvas(img.indices, img.width, img.height, displayPalette(task));
		cv.width = c.width;
		cv.height = c.height;
		cv.style.width = (c.width * scale) + 'px';
		cv.getContext('2d').drawImage(c, 0, 0);
	}).catch(function () { /* 読めなければ空のまま */ });
	return cv;
}

const BG_FALLBACK = '#6f8f5f';

/** (x, y, w, h)（canvas の画素）を背景で敷き詰める。bg は 1 マスの canvas か null */
export function fillBg(ctx, bg, x, y, w, h, scale) {
	ctx.save();
	ctx.beginPath();
	ctx.rect(x, y, w, h);
	ctx.clip();
	ctx.imageSmoothingEnabled = false;
	if (!bg) {
		ctx.fillStyle = BG_FALLBACK;
		ctx.fillRect(x, y, w, h);
	} else {
		const tw = bg.width * scale;
		const th = bg.height * scale;
		for (let j = y; j < y + h; j += th) {
			for (let i = x; i < x + w; i += tw) {
				ctx.drawImage(bg, i, j, tw, th);
			}
		}
	}
	ctx.restore();
}

/** 背景の上に絵（canvas）を等倍で置いた姿を作る。pad は絵のまわりの余白 */
export function composeOnBg(bg, src, pad) {
	const c = document.createElement('canvas');
	c.width = src.width + pad * 2;
	c.height = src.height + pad * 2;
	const ctx = c.getContext('2d');
	fillBg(ctx, bg, 0, 0, c.width, c.height, 1);
	ctx.drawImage(src, pad, pad);
	return c;
}

/**
 * 種類ごとのプレビュー。container の中に作る。
 *   walk … 4 方向（上・下・左・右）の歩きアニメ（向き d のコマは列 d*2 と d*2+1）
 *   tile … 1 コマを 3×3 に並べて継ぎ目を見る（コマが複数なら切り替え）
 *   anim … コマを順に再生
 * opt.paused … true なら止めた状態で始める（描く画面。動き続けると気が散るので）
 * 返す: { update(indices, w, h), setScale(n), play(), pause(), step(d), isPlaying(), frameText(), destroy() }
 */
export function makePreview(container, task, opt) {
	opt = opt || {};
	let scale = opt.scale || 2;
	const cw = task.cw;
	const ch = task.ch;
	const pal = displayPalette(task);
	let src = null;
	let bg = null;
	let frame = 0;
	let timer = 0;
	let draws = [];
	let nFrames = 1;
	let dead = false;
	let playing = !opt.paused;
	let onFrame = null;		// コマが変わった時に呼ぶ（「2 / 8」の表示用）

	function cols() {
		return src ? Math.max(1, Math.floor(src.width / cw)) : 1;
	}
	function nCells() {
		return src ? Math.max(1, cols() * Math.max(1, Math.floor(src.height / ch))) : 1;
	}
	function drawCell(ctx, idx, dx, dy) {
		const c = cols();
		const col = idx % c;
		const row = Math.floor(idx / c);
		ctx.drawImage(src, col * cw, row * ch, cw, ch, dx * scale, dy * scale, cw * scale, ch * scale);
	}
	function makeCanvas(wCells, hCells) {
		const cv = document.createElement('canvas');
		cv.width = cw * wCells * scale;
		cv.height = ch * hCells * scale;
		cv.className = 'pv-canvas';
		return cv;
	}

	function build() {
		container.textContent = '';
		draws = [];
		const kind = task.preview;
		if (kind === 'walk') {
			nFrames = 2;
			const wrap = el('div', 'pv-row');
			['上', '下', '左', '右'].forEach(function (label, d) {
				const item = el('div', 'pv-item');
				const cv = makeCanvas(1, 1);
				item.appendChild(cv);
				if (!opt.compact) {
					item.appendChild(el('div', 'pv-label', label));
				}
				wrap.appendChild(item);
				draws.push(function (f) {
					const ctx = cv.getContext('2d');
					ctx.imageSmoothingEnabled = false;
					fillBg(ctx, bg, 0, 0, cv.width, cv.height, scale);
					if (d * 2 + 1 < cols()) {
						drawCell(ctx, d * 2 + (f % 2), 0, 0);
					}
				});
			});
			container.appendChild(wrap);
		} else if (kind === 'tile') {
			const cv = makeCanvas(3, 3);
			container.appendChild(cv);
			draws.push(function (f) {
				const ctx = cv.getContext('2d');
				ctx.imageSmoothingEnabled = false;
				ctx.clearRect(0, 0, cv.width, cv.height);
				const idx = f % nCells();
				for (let j = 0; j < 3; j++) {
					for (let i = 0; i < 3; i++) {
						drawCell(ctx, idx, i * cw, j * ch);
					}
				}
			});
			nFrames = src ? nCells() : 1;
		} else {
			const cv = makeCanvas(1, 1);
			container.appendChild(cv);
			draws.push(function (f) {
				const ctx = cv.getContext('2d');
				ctx.imageSmoothingEnabled = false;
				fillBg(ctx, bg, 0, 0, cv.width, cv.height, scale);
				drawCell(ctx, f % nCells(), 0, 0);
			});
			nFrames = src ? nCells() : 1;
		}
		redraw();
	}

	function redraw() {
		if (!src) {
			return;
		}
		draws.forEach(function (fn) { fn(frame); });
	}

	function tick() {
		frame = (frame + 1) % 1000000;
		redraw();
		if (onFrame) {
			onFrame();
		}
	}
	function startTimer() {
		if (!timer && !dead) {
			timer = window.setInterval(tick, Math.max(60, task.frameMs || 200));
		}
	}
	function stopTimer() {
		if (timer) {
			window.clearInterval(timer);
			timer = 0;
		}
	}

	build();
	if (playing) {
		startTimer();
	}
	loadBgCanvas(task).then(function (c) {
		if (!dead) {
			bg = c;
			redraw();
		}
	});

	return {
		update: function (indices, w, h) {
			const firstTime = !src;
			src = indicesToCanvas(indices, w, h, pal);
			if (firstTime) {
				build();
			} else {
				redraw();
			}
		},
		setScale: function (n) {
			scale = n;
			build();
		},
		play: function () {
			playing = true;
			startTimer();
		},
		pause: function () {
			playing = false;
			stopTimer();
		},
		/** 止めている時に 1 コマ進める（d = 1）・戻す（d = -1） */
		step: function (d) {
			const n = Math.max(1, nFrames);
			frame = ((frame % n) + n + (d || 1)) % n;
			redraw();
			if (onFrame) {
				onFrame();
			}
		},
		isPlaying: function () {
			return playing;
		},
		frameText: function () {
			const n = Math.max(1, nFrames);
			return (frame % n + 1) + ' / ' + n;
		},
		set onFrame(fn) {
			onFrame = fn;
		},
		destroy: function () {
			dead = true;
			stopTimer();
			container.textContent = '';
		}
	};
}
