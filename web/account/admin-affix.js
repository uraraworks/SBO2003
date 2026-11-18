/* ========================================================================= */
/* admin-affix.js															 */
/* 管理ページ（/admin）の「付与」											 */
/* ========================================================================= */
/*
 * 装備に付く「鋭い」「腐った」「＋1」「の加護」などの付与（ディアブロ式の接頭辞・接尾辞）の表を決める。
 *   ・一覧から選んで直す／「付与を足す」で足す。保存すると /api/admin/recipe/save（kind=affix）に送る。
 *     ログイン中のプレイヤー全員にすぐ反映され、「AI連携」の変更履歴に残る。
 *   ・付与は消せない（派生アイテムが番号で指しているため）。出したくない時は「出やすさ」を 0 にする。
 *   ・強さ = 主の値（武器は攻撃力、防具・首飾り・指輪は守備力か魔防。アイテムの付加データ）+ 主の値×合計%/100 + 合計の固定加算（0〜255）。
 *   ・敵ごとの付く確率（byAffixRate）と格の上限（byAffixRank）は、ステータステンプレート（stTmp）の項目。
 *   仕様は docs/item-affix.md
 */
(function (root) {
	'use strict';

	var EQU = [
		['武器', 1], ['鎧', 2], ['盾', 4], ['兜', 8], ['小手', 16], ['靴', 32], ['首飾り', 64], ['指輪', 128]
	];

	var api = null;
	var bBound = false;
	var affixes = [];				/* 付与のレコード（添字が id。wID は別） */
	var sel = -1;					/* 選んでいる id（-1: なし、-2: 新しく足す） */

	function $(id) {
		return document.getElementById(id);
	}
	function el(tag, cls, text) {
		var e = document.createElement(tag);
		if (cls) e.className = cls;
		if (text != null) e.textContent = text;
		return e;
	}
	function setMsg(text, bOk) {
		var e = $('af-msg');
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}
	function errText(r) {
		return (r && (r.message || r.error)) || ('HTTP ' + (r && r.httpStatus));
	}
	function maskText(m) {
		if (m === 255) return '全部';
		return EQU.filter(function (e) { return m & e[1]; }).map(function (e) { return e[0]; }).join('・') || '（なし）';
	}
	function effectText(a) {
		var s = [];
		if (a.chRate) s.push((a.chRate > 0 ? '+' : '') + a.chRate + '%');
		if (a.chAdd) s.push((a.chAdd > 0 ? '+' : '') + a.chAdd);
		if (a.chDFMagic) s.push('魔防' + (a.chDFMagic > 0 ? '+' : '') + a.chDFMagic);
		return s.join(' ') || '効果なし';
	}

	function loadAll(kind) {
		var out = [];
		function next(offset) {
			return api('/api/admin/data/list', { kind: kind, offset: offset, limit: 500 }).then(function (r) {
				if (!r || !r.records) {
					throw new Error(errText(r));
				}
				r.records.forEach(function (rec) { out[rec.id] = rec; });
				return r.more ? next(offset + r.records.length) : out;
			});
		}
		return next(0);
	}

	function reload() {
		setMsg('読み込んでいます…', true);
		return loadAll('affix').then(function (r) {
			affixes = [];
			for (var i = 0; i < r.length; i++) affixes[i] = r[i] || null;
			setMsg('');
			render();
		}).then(null, function (err) {
			setMsg('読み込めませんでした: ' + (err && err.message || err));
		});
	}

	function render() {
		renderList();
		renderForm();
	}

	function renderList() {
		var box = $('af-list');
		box.textContent = '';
		var n = 0;
		affixes.forEach(function (a, id) {
			if (!a) return;
			n++;
			var b = el('button', 'rc-item' + (id === sel ? ' sel' : ''),
				(a.byPos ? '〜' : '') + a.szName + (a.byPos ? '' : '〜') + (a.byBad ? '　（ハズレ）' : ''));
			b.type = 'button';
			b.appendChild(el('span', 'sub-text', (a.byPos ? '後ろ' : '前') + '・格' + a.byRank + '・' + effectText(a) +
				'・' + maskText(a.wEquMask) + '・出やすさ ' + a.wWeight + '・売値 ' + a.wGoldRate + '%'));
			b.addEventListener('click', function () {
				sel = id;
				setMsg('');
				render();
			});
			box.appendChild(b);
		});
		if (!n) {
			box.appendChild(el('div', 'sub-text', '付与はまだありません（0 件の間は、装備に付与は付きません）。「付与を足す」で作ってください。'));
		}
	}

	function renderForm() {
		var card = $('af-card');
		var a = sel >= 0 ? affixes[sel] : null;
		card.hidden = !(sel === -2 || a);
		if (card.hidden) return;
		var v = a || { byPos: 0, szName: '', wEquMask: 255, chRate: 0, chAdd: 0, chDFMagic: 0, byRank: 1, wWeight: 100, wGoldRate: 100, byBad: 0 };
		$('af-title').textContent = a ? '付与 ' + a.wID + '：' + a.szName : '付与を足す';
		$('af-pos').value = String(v.byPos);
		$('af-name').value = v.szName;
		$('af-rate').value = String(v.chRate);
		$('af-add').value = String(v.chAdd);
		$('af-dfm').value = String(v.chDFMagic);
		$('af-rank').value = String(v.byRank);
		$('af-weight').value = String(v.wWeight);
		$('af-gold').value = String(v.wGoldRate);
		$('af-bad').checked = !!v.byBad;
		EQU.forEach(function (e, i) {
			$('af-equ' + i).checked = !!(v.wEquMask & e[1]);
		});
	}

	function intVal(id, min, max, label) {
		var s = String($(id).value).trim();
		var n = Number(s);
		if (!/^-?\d+$/.test(s) || n < min || n > max) {
			throw new Error(label + 'は ' + min + '〜' + max + ' の整数で入力してください');
		}
		return n;
	}

	function save() {
		var ch;
		try {
			var name = $('af-name').value.trim();
			if (!name) throw new Error('名前を入力してください');
			var mask = 0;
			EQU.forEach(function (e, i) { if ($('af-equ' + i).checked) mask |= e[1]; });
			if (!mask) throw new Error('付く装備を 1 つ以上選んでください');
			ch = {
				byPos: Number($('af-pos').value),
				szName: name,
				wEquMask: mask,
				chRate: intVal('af-rate', -128, 127, '主の値への%'),
				chAdd: intVal('af-add', -128, 127, '固定加算'),
				chDFMagic: intVal('af-dfm', -128, 127, '魔防加算'),
				byRank: intVal('af-rank', 1, 3, '格'),
				wWeight: intVal('af-weight', 0, 65535, '出やすさ'),
				wGoldRate: intVal('af-gold', 0, 65535, '売値の倍率'),
				byBad: $('af-bad').checked ? 1 : 0
			};
		} catch (e) {
			setMsg(e.message);
			return;
		}
		var bAdd = sel === -2;
		var body = { kind: 'affix', changes: ch };
		if (!bAdd) body.id = sel;
		setMsg('保存しています…', true);
		api('/api/admin/recipe/save', body).then(function (r) {
			if (!r || !r.ok) {
				setMsg('保存できませんでした: ' + errText(r));
				return;
			}
			sel = r.id;
			reload().then(function () {
				setMsg((bAdd ? '付与を足しました' : '付与を保存しました') + '（ログイン中のプレイヤーにもすぐ届きます）', true);
			});
		}, function (err) {
			setMsg('保存できませんでした: ' + (err && err.message || err));
		});
	}

	function bind() {
		if (bBound) return;
		bBound = true;
		$('af-reload').addEventListener('click', function () { reload(); });
		$('af-add').addEventListener('click', function () {
			sel = -2;
			setMsg('');
			render();
		});
		$('af-save').addEventListener('click', save);
	}

	function open(fnApi) {
		api = fnApi;
		bind();
		reload();
	}

	root.SboAffix = { open: open };
})(this);
