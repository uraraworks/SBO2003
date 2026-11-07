/* ========================================================================= */
/* admin-recipe.js															 */
/* 管理ページ（/admin）の「レシピ」											 */
/* ========================================================================= */
/*
 * 町の作り手（料理人・鍛冶屋など）と、作り手に頼んで作ってもらえるレシピを決める。
 *   1. 作り手を選ぶ（なければ追加）。名前と 3 つの台詞（話しかけた時・作った時・材料が足りない時）を決める。
 *   2. その作り手のレシピ（できる物・個数・材料 4 種まで・手数料・並び順）を足す・直す・消す。
 *   3. 保存すると /api/admin/recipe/save に送る（サーバーは server/node/sbosvr_data.js）。
 *      ログイン中のプレイヤー全員にすぐ反映され、「AI連携」の変更履歴に残る（元に戻せる）。
 * アイテムは名前で選ぶ（入力欄に名前を打つと候補が出る）。一覧は /api/admin/data/list の item から作る。
 * 作り手を呼び出すには NPC の台詞を「@cNN」（NN は作り手の番号 2 桁。例 @c00）にする。
 * 消すのは byState=0（空き枠）にすること。空き枠は次に足す時に使い回される。
 */
(function (root) {
	'use strict';

	var MAX_MAT = 4;				/* 材料の種類の上限（MAX_RECIPEMAT） */
	var ST_NONE = 0, ST_ON = 1, ST_OFF = 2;

	var api = null;					/* function (url, body) → Promise（session は呼ぶ側で付ける） */
	var bBound = false;				/* ボタンの割り当てが済んだか */
	var items = [];					/* [{id, name}]（アイテム。id 順） */
	var itemName = {};				/* id → 名前 */
	var itemLabel = {};				/* 「名前（#id）」→ id */
	var makers = [];				/* 作り手のレコード（空き枠を含む。添字が番号） */
	var recipes = [];				/* レシピのレコード（空き枠を含む。添字が番号） */
	var selMaker = -1;				/* 選んでいる作り手の番号（-1: なし、-2: 新しく足す） */
	var selRecipe = -1;				/* 選んでいるレシピの番号（-1: なし、-2: 新しく足す） */

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
		var e = $('rc-msg');
		e.textContent = text || '';
		e.classList.toggle('ok', !!bOk);
	}
	function errText(r) {
		return (r && (r.message || r.error)) || ('HTTP ' + (r && r.httpStatus));
	}
	function pad2(n) {
		return ('0' + n).slice(-2);
	}
	function stateText(n) {
		return n === ST_ON ? '有効' : n === ST_OFF ? '停止中' : '削除済み';
	}
	function nameOf(id) {
		return itemName[id] != null ? itemName[id] : (id ? '（不明 #' + id + '）' : '');
	}

	/* ---- データの読み込み ---- */

	/* kind の全件を読む（500 件ずつ） */
	function loadAll(kind, fields) {
		var out = [];
		function next(offset) {
			var body = { kind: kind, offset: offset, limit: 500 };
			if (fields) body.fields = fields;
			return api('/api/admin/data/list', body).then(function (r) {
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
		return Promise.all([
			loadAll('maker'),
			loadAll('recipe'),
			items.length ? Promise.resolve(null) : loadAll('item', ['szName'])
		]).then(function (r) {
			makers = fill(r[0]);
			recipes = fill(r[1]);
			if (r[2]) {
				items = [];
				itemName = {};
				itemLabel = {};
				fill(r[2]).forEach(function (rec, id) {
					if (rec && id > 0 && rec.szName) {
						items.push({ id: id, name: rec.szName });
						itemName[id] = rec.szName;
						itemLabel[rec.szName + '（#' + id + '）'] = id;
					}
				});
				buildDatalist();
			}
			setMsg('');
			render();
		}).then(null, function (err) {
			setMsg('読み込めませんでした: ' + (err && err.message || err));
		});
	}

	/* 欠けている番号を null で埋める */
	function fill(a) {
		var o = [];
		for (var i = 0; i < a.length; i++) o[i] = a[i] || null;
		return o;
	}

	function buildDatalist() {
		var dl = $('rc-items');
		dl.textContent = '';
		items.forEach(function (it) {
			var o = document.createElement('option');
			o.value = it.name + '（#' + it.id + '）';
			dl.appendChild(o);
		});
	}

	/* 入力欄の文字からアイテムの番号を決める（候補から選んだ形・番号だけ・名前が 1 つだけ合う、のどれか）。決まらなければ 0 */
	function resolveItem(text) {
		text = String(text || '').trim();
		if (!text) return 0;
		if (itemLabel[text] != null) return itemLabel[text];
		var m = /^#?(\d+)$/.exec(text);
		if (m && itemName[Number(m[1])] != null) return Number(m[1]);
		var hit = items.filter(function (it) { return it.name === text; });
		return hit.length === 1 ? hit[0].id : 0;
	}
	function itemText(id) {
		return id && itemName[id] != null ? itemName[id] + '（#' + id + '）' : '';
	}

	/* ---- 画面 ---- */

	function render() {
		renderMakers();
		renderMakerForm();
		renderRecipes();
		renderRecipeForm();
	}

	function countRecipes(mk) {
		var n = 0;
		recipes.forEach(function (r) {
			if (r && r.byState !== ST_NONE && r.byMaker === mk) n++;
		});
		return n;
	}

	function renderMakers() {
		var box = $('rc-makers');
		box.textContent = '';
		var n = 0;
		makers.forEach(function (m, id) {
			if (!m || m.byState === ST_NONE) return;
			n++;
			var b = el('button', 'rc-item' + (id === selMaker ? ' sel' : ''), '@c' + pad2(id) + '　' + m.szName);
			b.type = 'button';
			b.appendChild(el('span', 'sub-text', stateText(m.byState) + '・レシピ ' + countRecipes(id) + ' 件'));
			b.addEventListener('click', function () {
				selMaker = id;
				selRecipe = -1;
				setMsg('');
				render();
			});
			box.appendChild(b);
		});
		if (!n) {
			box.appendChild(el('div', 'sub-text', '作り手はまだいません。「作り手を足す」で作ってください。'));
		}
	}

	function renderMakerForm() {
		var card = $('rc-mk-card');
		var m = selMaker >= 0 ? makers[selMaker] : null;
		card.hidden = !(selMaker === -2 || m);
		$('rc-recipes-card').hidden = !m;
		if (card.hidden) return;
		if (selMaker === -2) {
			$('rc-mk-title').textContent = '作り手を足す';
			$('rc-mk-name').value = '';
			$('rc-mk-talk').value = 'いらっしゃい。@1何を作りましょうか？';
			$('rc-mk-done').value = 'はい、できました。@2';
			$('rc-mk-lack').value = '材料が足りないようです。@2';
			$('rc-mk-state').value = String(ST_ON);
			$('rc-mk-del').hidden = true;
			$('rc-mk-call').textContent = '足すと番号が決まります（NPC の台詞に @c + 番号 2 桁を書くと開きます）。';
		} else {
			$('rc-mk-title').textContent = '作り手 ' + pad2(selMaker) + '：' + m.szName;
			$('rc-mk-name').value = m.szName;
			$('rc-mk-talk').value = m.szMsgTalk;
			$('rc-mk-done').value = m.szMsgDone;
			$('rc-mk-lack').value = m.szMsgLack;
			$('rc-mk-state').value = String(m.byState);
			$('rc-mk-del').hidden = false;
			$('rc-mk-call').textContent = 'NPC の台詞を「@c' + pad2(selMaker) + '」にすると、この作り手の画面が開きます（話しかけても S キーでも）。';
		}
	}

	function renderRecipes() {
		var box = $('rc-recipes');
		box.textContent = '';
		if (selMaker < 0) return;
		var list = [];
		recipes.forEach(function (r, id) {
			if (r && r.byState !== ST_NONE && r.byMaker === selMaker) list.push({ id: id, r: r });
		});
		/* ゲームの一覧と同じ並び（並び順、同じなら番号順） */
		list.sort(function (a, b) { return (a.r.wSort - b.r.wSort) || (a.id - b.id); });
		list.forEach(function (x) {
			var r = x.r;
			var mats = [];
			for (var k = 0; k < MAX_MAT; k++) {
				if (r.awMatItem[k]) mats.push(nameOf(r.awMatItem[k]) + '×' + r.abyMatCount[k]);
			}
			var b = el('button', 'rc-item' + (x.id === selRecipe ? ' sel' : ''),
				nameOf(r.wItem) + (r.byCount > 1 ? '×' + r.byCount : ''));
			b.type = 'button';
			b.appendChild(el('span', 'sub-text', mats.join('、') + '　手数料 ' + r.dwGold + 'G　並び順 ' + r.wSort +
				(r.byState === ST_OFF ? '　（停止中）' : '')));
			b.addEventListener('click', function () {
				selRecipe = x.id;
				setMsg('');
				renderRecipes();
				renderRecipeForm();
			});
			box.appendChild(b);
		});
		if (!list.length) {
			box.appendChild(el('div', 'sub-text', 'この作り手のレシピはまだありません。'));
		}
	}

	function renderRecipeForm() {
		var card = $('rc-rc-card');
		var r = selRecipe >= 0 ? recipes[selRecipe] : null;
		card.hidden = !(selMaker >= 0 && (selRecipe === -2 || r));
		if (card.hidden) return;
		var i;
		if (selRecipe === -2) {
			$('rc-rc-title').textContent = 'レシピを足す';
			$('rc-rc-item').value = '';
			$('rc-rc-count').value = '1';
			for (i = 0; i < MAX_MAT; i++) {
				$('rc-mat' + i).value = '';
				$('rc-mc' + i).value = '1';
			}
			$('rc-rc-gold').value = '0';
			$('rc-rc-sort').value = String(nextSort());
			$('rc-rc-state').value = String(ST_ON);
			$('rc-rc-del').hidden = true;
		} else {
			$('rc-rc-title').textContent = 'レシピ ' + selRecipe + '：' + nameOf(r.wItem);
			$('rc-rc-item').value = itemText(r.wItem);
			$('rc-rc-count').value = String(r.byCount);
			for (i = 0; i < MAX_MAT; i++) {
				$('rc-mat' + i).value = itemText(r.awMatItem[i]);
				$('rc-mc' + i).value = String(r.awMatItem[i] ? r.abyMatCount[i] : 1);
			}
			$('rc-rc-gold').value = String(r.dwGold);
			$('rc-rc-sort').value = String(r.wSort);
			$('rc-rc-state').value = String(r.byState);
			$('rc-rc-del').hidden = false;
		}
	}

	/* 新しいレシピの並び順の初期値（今の最大 + 10） */
	function nextSort() {
		var max = 0;
		recipes.forEach(function (r) {
			if (r && r.byState !== ST_NONE && r.byMaker === selMaker && r.wSort > max) max = r.wSort;
		});
		return Math.min(65535, max + 10);
	}

	/* ---- 保存 ---- */

	function save(kind, id, changes, doneText) {
		setMsg('保存しています…', true);
		var body = { kind: kind, changes: changes };
		if (id != null) body.id = id;
		return api('/api/admin/recipe/save', body).then(function (r) {
			if (!r || !r.ok) {
				setMsg('保存できませんでした: ' + errText(r));
				return null;
			}
			return reload().then(function () {
				setMsg(doneText + '（ログイン中のプレイヤーにもすぐ届きます）', true);
				return r;
			});
		}, function (err) {
			setMsg('保存できませんでした: ' + (err && err.message || err));
			return null;
		});
	}

	function intVal(id, min, max, label) {
		var s = String($(id).value).trim();
		var n = Number(s);
		if (!/^\d+$/.test(s) || n < min || n > max) {
			throw new Error(label + 'は ' + min + '〜' + max + ' の整数で入力してください');
		}
		return n;
	}

	function saveMaker() {
		var name = $('rc-mk-name').value.trim();
		if (!name) {
			setMsg('名前を入力してください。');
			return;
		}
		var ch = {
			byState: Number($('rc-mk-state').value),
			szName: name,
			szMsgTalk: $('rc-mk-talk').value,
			szMsgDone: $('rc-mk-done').value,
			szMsgLack: $('rc-mk-lack').value
		};
		var bAdd = selMaker === -2;
		save('maker', bAdd ? null : selMaker, ch, bAdd ? '作り手を足しました' : '作り手を保存しました').then(function (r) {
			if (r && bAdd) {
				selMaker = r.id;
				render();
			}
		});
	}

	function delMaker() {
		var m = makers[selMaker];
		if (!m) return;
		var n = countRecipes(selMaker);
		if (!window.confirm('作り手「' + m.szName + '」を消しますか？' +
			(n ? '\nこの作り手のレシピ ' + n + ' 件は残りますが、作り手がいないので使えなくなります。' : '') +
			'\n（「AI連携」の変更履歴から元に戻せます）')) {
			return;
		}
		save('maker', selMaker, { byState: ST_NONE }, '作り手を消しました').then(function (r) {
			if (r) {
				selMaker = -1;
				selRecipe = -1;
				render();
			}
		});
	}

	function saveRecipe() {
		var ch, i, mi = [], mc = [], nMat = 0;
		try {
			var item = resolveItem($('rc-rc-item').value);
			if (!item) throw new Error('できるアイテムを、候補から選んでください');
			for (i = 0; i < MAX_MAT; i++) {
				var t = $('rc-mat' + i).value.trim();
				if (!t) {
					mi.push(0);
					mc.push(0);
					continue;
				}
				var id = resolveItem(t);
				if (!id) throw new Error('材料' + (i + 1) + 'のアイテムを、候補から選んでください');
				mi.push(id);
				mc.push(intVal('rc-mc' + i, 1, 255, '材料' + (i + 1) + 'の個数'));
				nMat++;
			}
			if (!nMat) throw new Error('材料を 1 種類以上入れてください');
			ch = {
				byState: Number($('rc-rc-state').value),
				byMaker: selMaker,
				wSort: intVal('rc-rc-sort', 0, 65535, '並び順'),
				wItem: item,
				byCount: intVal('rc-rc-count', 1, 255, 'できる個数'),
				awMatItem: mi,
				abyMatCount: mc,
				dwGold: intVal('rc-rc-gold', 0, 999999, '手数料')
			};
		} catch (e) {
			setMsg(e.message);
			return;
		}
		var bAdd = selRecipe === -2;
		save('recipe', bAdd ? null : selRecipe, ch, bAdd ? 'レシピを足しました' : 'レシピを保存しました').then(function (r) {
			if (r) {
				selRecipe = r.id;
				render();
			}
		});
	}

	function delRecipe() {
		var r = recipes[selRecipe];
		if (!r) return;
		if (!window.confirm('レシピ「' + nameOf(r.wItem) + '」を消しますか？\n（「AI連携」の変更履歴から元に戻せます）')) {
			return;
		}
		save('recipe', selRecipe, { byState: ST_NONE }, 'レシピを消しました').then(function (res) {
			if (res) {
				selRecipe = -1;
				render();
			}
		});
	}

	function bind() {
		if (bBound) return;
		bBound = true;
		$('rc-reload').addEventListener('click', function () { reload(); });
		$('rc-mk-add').addEventListener('click', function () {
			selMaker = -2;
			selRecipe = -1;
			setMsg('');
			render();
		});
		$('rc-mk-save').addEventListener('click', saveMaker);
		$('rc-mk-del').addEventListener('click', delMaker);
		$('rc-rc-add').addEventListener('click', function () {
			selRecipe = -2;
			setMsg('');
			renderRecipes();
			renderRecipeForm();
		});
		$('rc-rc-save').addEventListener('click', saveRecipe);
		$('rc-rc-del').addEventListener('click', delRecipe);
	}

	/* fnApi(url, body) → Promise<data>（管理ページの api 関数。session は呼ぶ側で入れる） */
	function open(fnApi) {
		api = fnApi;
		bind();
		reload();
	}

	root.SboRecipe = { open: open };
})(this);
