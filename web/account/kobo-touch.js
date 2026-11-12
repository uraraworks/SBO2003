/*
 * ドット絵工房のタッチ操作（WebPaint98 の touchGesture.ts を JS に移したもの）
 *
 *   1 本指でなぞる … 描く
 *   2 本指          … スクロールと拡大縮小（一度 2 本指になったら、全部の指が離れるまで描かない）
 *   1 本指で長押し  … 色を拾う（動かさず 0.45 秒）
 *
 * 指が触れてから 80ms は「保留」にして、まだ描かない。その間に 2 本目の指が来たらスクロールにする。
 * 来なければ、それまでの動きもまとめて描き始めるので、線は欠けない（描き始めが 80ms 遅れるだけ）。
 * 保留を抜けてから 2 本目が来たら、描きかけの線は取り消す（dragCancel）。
 *
 * マウス・ペンはこれまでどおり各画面の pointer イベントで扱い、ここでは pointerType が touch のものだけ見る。
 */

export const TOUCH_PENDING_MS = 80;
export const TOUCH_MOVE_THRESHOLD_PX = 8;
export const TOUCH_LONG_PRESS_MS = 450;

/** 判定だけをする状態機械（DOM・タイマーは使わない。時刻と座標を受け取り、することを返す） */
export class TouchGestureMachine {
	constructor() {
		this.reset();
	}

	reset() {
		this.phase = 'idle';	// idle | pending | armed | dragging | longPressed | multi
		this.primaryId = -1;
		this.startedAt = 0;
		this.startPoint = null;
		this.lastPoint = null;
		this.buffered = [];
	}

	msUntilNextDeadline(now) {
		if (this.phase === 'pending') {
			return Math.max(0, this.startedAt + TOUCH_PENDING_MS - now);
		}
		if (this.phase === 'armed') {
			return Math.max(0, this.startedAt + TOUCH_LONG_PRESS_MS - now);
		}
		return null;
	}

	onStart(points, now) {
		if (!points.length) {
			return [];
		}
		if (points.length >= 2) {
			const acts = [];
			if (this.phase === 'dragging') {
				acts.push({ type: 'dragCancel' });
			}
			this.enterMulti();
			acts.push({ type: 'multiStart', points: points.slice() });
			return acts;
		}
		if (this.phase === 'multi') {
			return [];
		}
		const p = points[0];
		this.phase = 'pending';
		this.primaryId = p.id;
		this.startedAt = now;
		this.startPoint = p;
		this.lastPoint = p;
		this.buffered = [p];
		return [];
	}

	onMove(points, now) {
		if (this.phase === 'multi') {
			return points.length >= 2 ? [{ type: 'multiMove', points: points.slice() }] : [];
		}
		const p = points.find(function (q) { return q.id === this.primaryId; }, this);
		if (!p || !this.startPoint) {
			return [];
		}
		this.lastPoint = p;
		if (this.phase === 'pending') {
			this.buffered.push(p);
			return this.advance(now);
		}
		if (this.phase === 'armed') {
			if (!this.moved(p)) {
				return this.advance(now);
			}
			this.phase = 'dragging';
			const replay = dedupe(this.buffered.concat(p));
			this.buffered = [];
			return [{ type: 'dragStart', points: replay }];
		}
		if (this.phase === 'dragging') {
			return [{ type: 'dragMove', point: p }];
		}
		return [];
	}

	onEnd(p, remaining, now) {
		if (this.phase === 'multi') {
			if (remaining) {
				return [];
			}
			this.reset();
			return [{ type: 'multiEnd' }];
		}
		if (p.id !== this.primaryId) {
			return [];
		}
		const timed = this.advance(now);
		if (timed.some(function (a) { return a.type === 'longPress'; })) {
			this.reset();
			return timed;
		}
		const phase = this.phase;
		this.reset();
		if (phase === 'dragging') {
			return timed.concat({ type: 'dragEnd', point: p });
		}
		if (phase === 'pending' || phase === 'armed') {
			return timed.concat({ type: 'tap', point: p });
		}
		return timed;
	}

	advance(now) {
		if (!this.startPoint || !this.lastPoint) {
			return [];
		}
		if (this.phase === 'pending' && now - this.startedAt >= TOUCH_PENDING_MS) {
			this.phase = 'armed';
			if (this.moved(this.lastPoint)) {
				this.phase = 'dragging';
				const pts = dedupe(this.buffered);
				this.buffered = [];
				return [{ type: 'dragStart', points: pts }];
			}
		}
		if (this.phase === 'armed' && now - this.startedAt >= TOUCH_LONG_PRESS_MS) {
			this.phase = 'longPressed';
			return [{ type: 'longPress', point: this.startPoint }];
		}
		return [];
	}

	cancel() {
		const acts = [];
		if (this.phase === 'dragging') {
			acts.push({ type: 'dragCancel' });
		}
		if (this.phase === 'multi') {
			acts.push({ type: 'multiEnd' });
		}
		this.reset();
		return acts;
	}

	moved(p) {
		return Math.hypot(p.x - this.startPoint.x, p.y - this.startPoint.y) > TOUCH_MOVE_THRESHOLD_PX;
	}

	enterMulti() {
		this.phase = 'multi';
		this.primaryId = -1;
		this.startPoint = null;
		this.lastPoint = null;
		this.buffered = [];
	}
}

function dedupe(points) {
	const r = [];
	points.forEach(function (p) {
		const q = r[r.length - 1];
		if (!q || q.x !== p.x || q.y !== p.y) {
			r.push(p);
		}
	});
	return r;
}

/**
 * 要素にタッチ操作を付ける。点は { id, x, y }（x, y は clientX, clientY）。
 * handlers: dragStart(points) dragMove(point) dragEnd(point) dragCancel() tap(point) longPress(point)
 *           multiStart(points) multiMove(points) multiEnd()
 * 戻り値は外す関数。
 */
export function installTouch(el, handlers) {
	const m = new TouchGestureMachine();
	const touches = new Map();
	let timer = 0;

	function now() {
		return performance.now();
	}
	function list() {
		return Array.from(touches.values());
	}
	function run(acts) {
		acts.forEach(function (a) {
			const fn = handlers[a.type];
			if (fn) {
				fn(a.points || a.point);
			}
		});
		schedule();
	}
	function schedule() {
		clearTimeout(timer);
		const d = m.msUntilNextDeadline(now());
		if (d === null) {
			return;
		}
		timer = setTimeout(function () { run(m.advance(now())); }, d + 1);
	}
	function pt(e) {
		return { id: e.pointerId, x: e.clientX, y: e.clientY };
	}
	function down(e) {
		if (e.pointerType !== 'touch') {
			return;
		}
		e.preventDefault();
		try {
			el.setPointerCapture(e.pointerId);
		} catch (err) { /* 取れなくてもよい */ }
		touches.set(e.pointerId, pt(e));
		run(m.onStart(list(), now()));
	}
	function move(e) {
		if (e.pointerType !== 'touch' || !touches.has(e.pointerId)) {
			return;
		}
		touches.set(e.pointerId, pt(e));
		run(m.onMove(list(), now()));
	}
	function up(e) {
		if (e.pointerType !== 'touch' || !touches.has(e.pointerId)) {
			return;
		}
		touches.delete(e.pointerId);
		run(m.onEnd(pt(e), touches.size, now()));
	}
	function cancel(e) {
		if (e.pointerType !== 'touch' || !touches.has(e.pointerId)) {
			return;
		}
		touches.delete(e.pointerId);
		if (!touches.size) {
			run(m.cancel());
		}
	}
	el.addEventListener('pointerdown', down);
	el.addEventListener('pointermove', move);
	el.addEventListener('pointerup', up);
	el.addEventListener('pointercancel', cancel);
	return function () {
		clearTimeout(timer);
		el.removeEventListener('pointerdown', down);
		el.removeEventListener('pointermove', move);
		el.removeEventListener('pointerup', up);
		el.removeEventListener('pointercancel', cancel);
	};
}
