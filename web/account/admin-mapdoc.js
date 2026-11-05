/* ========================================================================= */
/* admin-mapdoc.js															 */
/* 管理ページ（/admin）の「マップ資料」: マップごとの NPC・敵・お店・イベントの	 */
/* 資料（Markdown）と全体図（PNG）を作って zip で書き出す						 */
/* ========================================================================= */
/*
 * データはサーバーの /api/admin/data/list・/api/admin/data/tiles（読むだけ）から読む。
 * 全体図はブラウザで描く（マップパーツの画像は /admin-grp/ に置いてある map01〜05.png・char01〜16.png）。
 * 出来上がる zip の中身:
 *   README.md         目次・敵の索引・気づいたこと
 *   maps/NNN.md       マップ 1 枚ぶんの資料（全体図・基本情報・敵・NPC・お店・イベント）
 *   images/NNN.png    全体図（屋根を外して、NPC の番号・敵の出る範囲・入口を重ねる）
 *   data/maps.json    AI に読ませる用のまとめ
 *
 * Markdown を作る部分（makeDocs）はブラウザの機能を使わないので、Node でも動かして確かめられる。
 */
(function (root) {
	'use strict';

	/* ---------------------------------------------------------------- */
	/* 表																	 */
	/* ---------------------------------------------------------------- */

	/* BGM の番号（マップの byBGM）→ 管理者ウィンドウで出る曲名 */
	var BGM = {"0":"無し","15":"戦闘１","16":"ボス１","17":"ボス２","18":"城１","19":"城２","20":"城３","21":"教会１","22":"教会２","23":"クライマックス","24":"フィールド１","25":"フィールド２","26":"フィールド３","27":"フィールド４","28":"フィールド５","29":"フィールド６","30":"ラスボス","31":"追憶","32":"メニュー","33":"密談，潜入","34":"山","35":"ピンチ１","36":"ピンチ２","37":"神殿","38":"古代神殿","39":"塔１","40":"塔２","41":"塔３","42":"町１","43":"町１（夜）","44":"町２","45":"村","46":"Clock Down","47":"Half Hearted","48":"オルゴール７","49":"猫の日向ぼっこ","50":"Opening","51":"Organ Exacise","52":"散る時","53":"戸惑いの時","66":"オルゴール1","67":"Cream Bossa","68":"夏の夜祭","71":"迷いの森","144":"望郷","145":"飛空艇１","146":"メイン２","151":"雪の里","152":"Bacation on the Beach","153":"Battle","154":"Dance to the Moon","155":"田園","156":"Destration of Death Across the Heaven","157":"Dreaming Stars","158":"火山洞窟","159":"不気味な工場地帯","160":"南の国","161":"おやすみ系03","162":"旅愁","163":"深々","164":"雪","165":"黄色い太陽","166":"NO BACK ANSWER","167":"農村","174":"闘竜士　-竜騎士の余興-","175":"故郷を守る者達の戦い","176":"右手に敵、左手に援軍、前方には義勇軍","177":"阻止すべき悪の陰謀だとか","178":"真夏の熱さとか澄み渡る空の青さだとかあと期間限定で売店を出してるあのおじさんとかいろいろ","179":"変なお店の名物おじさん","180":"酒場","181":"深い森の村に住む小人の踊り","182":"とことん田舎町","183":"氷の世界に訪れた夜の空を迎える遺跡","184":"異教徒達の集会","185":"教会の見える丘","186":"素敵な和服","187":"暗闇、何かの気配、その恐怖","188":"Ｃ級ホラー","189":"さぁ行こう、あの地平線の彼方へ","191":"bunch of the armed enemy","192":"どうも災難続きで笑えてくる一日の始まり","193":"追跡、奴はまだこの町にいる","194":"業を背負う者達","195":"遊戯的戦闘風楽曲","196":"風吹き荒れる草原","197":"ＦＭ音源風","198":"戦場を駆ける銃士","199":"民族の誇りを","200":"倒壊までの残り僅かな時間","201":"いつもより素敵に見える景色","202":"小さな戦いの勝利者達","203":"ドラムロール（ティンパニ）","204":"緊迫した雰囲気","205":"のんびりと散歩をしながら","212":"きらきら星","213":"きよしこの夜","214":"散る時PartⅡ","215":"ハッピーバースデイ・トゥ・ユウ","216":"Jingle Bells","217":"正月風BGM","218":"（聖夜）清しこの夜","219":"ジャングル","221":"OMOIDE","222":"戦闘","223":"恐怖、昼時のバーゲンセール","224":"緑溢れる広場","225":"暖かな北の大地での暮らし","252":"気付かないうちに過ぎ行くもの","253":"協会の見える丘","254":"旅路に思う、置き去りにした日常と、これからのこと","255":"激しいオープニング？"};

	var NPCTYPE = ['未設定', '移動しない', 'ランダムに移動', '押せる', '押せる(サイコロ)', '敵の発生地点', '得点板', '話すとランダム',
		'止まる/ランダム', '押せる(サイコロ2)', '押せる(サイコロ3)', '敵(反撃型)', 'ついてくる', '敵(追いかけて攻撃)',
		'移動しない(通り抜け可)', '爆弾', '重なると魔法', '重なると攻撃', '爆弾ゲームの壁', '爆弾ゲームのアイテム',
		'魔法を出し続ける', '押せて話すと切り替え', '予備', 'ついてくる2'];
	var NPCTYPE_PUT = 5;
	var ENEMY_TYPES = { 11: true, 13: true };
	var MSGEVT = ['自動', '調べる', '話しかける'];
	var WEATHER = ['なし', '雪', '雨', '昼夜'];
	var FLAGS = [[0x02, '戦闘できる'], [0x04, 'サブゲーム'], [0x08, '暗闇'], [0x10, 'マップ内放送'],
		[0x20, '移動系の特技禁止'], [0x40, '魔法禁止'], [0x01, 'エフェクト可']];
	var ITEMTYPE = ['効果なし', '特定位置へ移動', 'HP回復', '武器', '鎧', '盾', '兜', '種', 'サブゲーム参加', '酒',
		'メッセージ表示', '見た目変更', '明かり'];
	var ATTR = [['火', 'chAttrFire'], ['水', 'chAttrWater'], ['風', 'chAttrWind'], ['土', 'chAttrEarth'], ['光', 'chAttrLight'], ['闇', 'chAttrDark']];
	/* 屋根（その上を歩くと消える）のパーツは、全体図では背景だけ描く */
	var HIDE_MAIN = { 3: true, 4: true, 8: true, 9: true, 18: true };
	var ECOL = [[230, 60, 60], [240, 140, 30], [200, 40, 160], [150, 90, 220], [220, 200, 40], [60, 170, 230]];
	var NO_TEMPLATE = 4294967295;
	var GROUP_PLAY = ['ゲーム', 'サッカー', '劇場', '雪合戦', '鬼ごっこ', '遊戯', '迷路', '爆弾亭', '実験場', '戦闘場',
		'フリーエリア', '分譲', '会議室', '夏限定', '着せ替え'];

	function pad3(n) {
		return ('00' + n).slice(-3);
	}
	function pad2(n) {
		return ('0' + n).slice(-2);
	}
	function npcTypeName(t) {
		return t < NPCTYPE.length ? NPCTYPE[t] : String(t);
	}
	/* 表のマスに入れる文字（| と改行は表を崩すので置き換える） */
	function cell(s) {
		return String(s).replace(/\|/g, '｜').replace(/[\r\n]+/g, ' ');
	}

	/* ---------------------------------------------------------------- */
	/* 資料の中身（Markdown）											 */
	/* ---------------------------------------------------------------- */

	/*
	 * D = { maps, chars, items, stTmp, magic }（どれも API の records。番号は id）
	 * 返す: { files: [{path, text}], plans: [{id, ...描く内容}] }
	 */
	function makeDocs(D, opt) {
		opt = opt || {};
		var maps = {}, i;
		D.maps.forEach(function (m) { maps[m.id] = m; });
		var items = D.items, sttmp = D.stTmp, magic = D.magic;

		function mapName(id) {
			var m = maps[id];
			if (!m) {
				return '（無いマップ ' + id + '）';
			}
			return m.szName || '（名前なし）';
		}
		function itemName(id) {
			if (!(id > 0) || id >= items.length || !items[id]) {
				return null;
			}
			return items[id].szName || '（名前なし #' + id + '）';
		}
		/* 台詞の制御コードを読みやすく */
		function clean(msg) {
			if (!msg) {
				return '';
			}
			msg = msg.replace(/@4(\d{5})/g, function (s, n) {
				return '〔アイテム「' + (itemName(parseInt(n, 10)) || n) + '」をもらう〕';
			});
			msg = msg.replace(/@8(\d{3})(\d{3})(\d{3})/g, function (s, m, x, y) {
				return '〔復活地点を ' + mapName(parseInt(m, 10)) + ' (' + parseInt(x, 10) + ',' + parseInt(y, 10) + ') に〕';
			});
			msg = msg.replace(/@9\d{5}/g, '〔見た目を記録〕').replace(/@3\d\d/g, '');
			msg = msg.split('@0').join(' ▼ ').split('@1').join(' ').split('@2').join('');
			msg = msg.replace(/\|/g, '｜').replace(/\s+/g, ' ');
			return msg.trim();
		}
		function speechOf(c) {
			return c.szNpcSpeech || '';
		}
		function isFunctionSpeech(sp) {
			return sp === '@' || /^@[567a]/.test(sp);
		}
		function npcRole(c) {
			var sp = speechOf(c);
			if (sp.slice(0, 2) === '@5') return 'お店（' + sp.slice(2, 4) + '番）';
			if (sp.slice(0, 2) === '@7') return '買い取り専門（' + sp.slice(2, 4) + '番）';
			if (sp.slice(0, 2) === '@6') return '預かり所';
			if (sp.slice(0, 2) === '@a') return '冒険者登録所（転職）';
			if (sp === '@') return '着せ替え';
			var r = [];
			if (sp.indexOf('@4') >= 0) r.push('アイテムをくれる');
			if (sp.indexOf('@8') >= 0) r.push('復活地点の登録');
			return r.join('・');
		}
		function npcName(c) {
			if (c.szName) {
				return c.szName;
			}
			var mo = /^【([^】]+)】/.exec(speechOf(c));
			return mo ? mo[1] : '';
		}
		function tmplOf(c) {
			var t = c.dwStTmplateID;
			if (t !== NO_TEMPLATE && t < sttmp.length && sttmp[t]) {
				return { id: t, src: sttmp[t] };
			}
			return { id: null, src: c };
		}
		function drops(src) {
			return (src.waItem || []).filter(function (n) { return n; }).map(itemName).filter(function (n) { return n; });
		}
		function magics(src) {
			return (src.abyMagic || []).filter(function (n) { return n && n < magic.length && magic[n]; })
				.map(function (n) { return magic[n].szName; });
		}
		function attrs(src) {
			return ATTR.filter(function (a) { return src[a[1]]; }).map(function (a) {
				var v = src[a[1]];
				return a[0] + (v > 0 ? '+' : '') + v;
			}).join(' ');
		}
		function shopNo(sp) {
			if (sp.slice(0, 2) !== '@5' && sp.slice(0, 2) !== '@7') {
				return -1;
			}
			var n = parseInt(sp.slice(2, 4), 10);
			return isNaN(n) ? -1 : n;
		}
		function link(id, prefix) {
			return '[' + pad3(id) + ' ' + mapName(id) + '](' + (prefix || '') + pad3(id) + '.md)';
		}

		/* マップごとのキャラ（プレイヤーは出さない） */
		var byMap = {};
		D.chars.forEach(function (c) {
			if (!c || c.isPlayer) {
				return;
			}
			(byMap[c.dwMapID] = byMap[c.dwMapID] || []).push(c);
		});
		/* 入口の逆引き */
		var incoming = {};
		D.maps.forEach(function (m) {
			(m.events || []).forEach(function (e) {
				if (e.type === 1 && e.detail && e.detail.dwMapID != null) {
					(incoming[e.detail.dwMapID] = incoming[e.detail.dwMapID] || {})[m.id] = true;
				}
			});
		});

		var files = [], plans = [], summary = [], alljson = {}, enemyIndex = {};
		var mapIds = D.maps.map(function (m) { return m.id; }).sort(function (a, b) { return a - b; });

		mapIds.forEach(function (id) {
			var m = maps[id];
			var W = m.nCX, H = m.nCY;
			var list = byMap[id] || [];
			var npcs = [], spawns = [], fixed = [], objs = [];
			list.forEach(function (c) {
				var t = c.byNpcType;
				if (t === NPCTYPE_PUT) spawns.push(c);
				else if (ENEMY_TYPES[t]) fixed.push(c);
				else if (speechOf(c) || npcName(c)) npcs.push(c);
				else objs.push(c);
			});

			/* 敵（発生地点をテンプレートごとにまとめる） */
			var groups = [], groupOf = {};
			spawns.forEach(function (c) {
				var t = tmplOf(c);
				var key = t.id != null ? 't' + t.id : 'c' + c.id;
				if (!groupOf[key]) {
					groupOf[key] = { key: key, tmpl: t.id, src: t.src, list: [] };
					groups.push(groupOf[key]);
				}
				groupOf[key].list.push(c);
			});
			var erows = [];
			function enemyRow(label, src, c0, tmplId, points, col, bFixed) {
				return {
					label: label, name: src.szName || c0.szName || '（名前なし）', template: tmplId,
					level: src.byLevel, hp: src.wMaxHP, mana: src.wMaxMana, pw: src.byPW, dex: src.byDEX, st: src.byST,
					it: src.byIT, lk: src.byLK, exp: src.dwExp, gold: src.dwGold, drop: src.byDropProbability,
					drops: drops(src), magic: magics(src), attr: attrs(src), type: npcTypeName(src.byNpcType),
					points: points, grp: c0.wGrpID, col: col, fixed: !!bFixed
				};
			}
			groups.forEach(function (g, gi) {
				var c0 = g.list[0];
				var row = enemyRow('E' + (gi + 1), g.src, c0, g.tmpl, g.list.map(function (c) {
					return { x: c.dwX, y: c.dwY, max: c.byPutCount, interval: c.wTimePutInterval, areaX: c.byAreaX, areaY: c.byAreaY, charNo: c.id };
				}), ECOL[gi % ECOL.length], false);
				erows.push(row);
			});
			fixed.forEach(function (c) {
				var t = tmplOf(c);
				erows.push(enemyRow('E' + (erows.length + 1), t.src, c, t.id,
					[{ x: c.dwX, y: c.dwY, max: 1, interval: null, areaX: 0, areaY: 0, charNo: c.id }], ECOL[0], true));
			});
			erows.forEach(function (e) {
				(enemyIndex[e.name] = enemyIndex[e.name] || []).push({ map: id, hp: e.hp, exp: e.exp });
			});

			/* イベント */
			var warps = [], moves = [], msgs = [], doors = 0, heal = [], stinit = [];
			(m.events || []).forEach(function (e) {
				var d = e.detail || {};
				switch (e.type) {
				case 0: moves.push({ x: e.x, y: e.y, toX: d.dwX, toY: d.dwY }); break;
				case 1: warps.push({ x: e.x, y: e.y, to: d.dwMapID, toX: d.dwX, toY: d.dwY }); break;
				case 2:
					var no = d.dwNo || 0;
					msgs.push({ x: e.x, y: e.y, how: (d.byType || 0) < 3 ? MSGEVT[d.byType || 0] : d.byType, text: (m.messages || [])[no] || '' });
					break;
				case 3: doors++; break;
				case 4: heal.push([e.x, e.y]); break;
				case 5: stinit.push([e.x, e.y]); break;
				}
			});

			plans.push({ id: id, W: W, H: H, npcs: npcs, objs: objs, fixed: fixed, erows: erows, warps: warps, moves: moves, msgs: msgs });

			/* ---- Markdown ---- */
			var nm = mapName(id), fn = pad3(id);
			var flg = FLAGS.filter(function (f) { return m.byFlg & f[0]; }).map(function (f) { return f[1]; });
			var L = [];
			L.push('# ' + fn + ' ' + nm, '');
			L.push('![' + nm + '](../images/' + fn + '.png)', '');
			L.push('> 図の見かた: 青い数字＝NPC（下の表の番号）、赤など色つきの枠と E番号＝敵が出る範囲、水色の枠と「→番号」＝別のマップへの入口、緑の小枠＝同じマップ内の移動、黄色の小枠＝メッセージが出る場所。屋根は外して描いています。', '');
			L.push('## 基本情報', '', '| 項目 | 内容 |', '| --- | --- |');
			L.push('| マップ番号 | ' + id + ' |');
			L.push('| 大きさ | ' + W + ' × ' + H + ' マス |');
			L.push('| 初期位置 / 入口 | (' + m.nStartX + ',' + m.nStartY + ') / (' + m.nEntranceX + ',' + m.nEntranceY + ') |');
			L.push('| BGM | ' + (BGM[m.byBGM] || '?') + '（' + m.byBGM + '） |');
			L.push('| 天候 / 明るさ | ' + (m.byWeather < 4 ? WEATHER[m.byWeather] : m.byWeather) + ' / ' + (m.byLightLevel ? '暗さ ' + m.byLightLevel : 'ふつう') + ' |');
			L.push('| 設定 | ' + (flg.length ? flg.join('・') : '（なし：戦闘できない）') + ' |');
			var outs = {};
			warps.forEach(function (w) { if (w.to != null && w.to !== id) outs[w.to] = true; });
			var aOut = Object.keys(outs).map(Number).sort(function (a, b) { return a - b; });
			L.push('| 行ける先 | ' + (aOut.map(function (t) { return link(t); }).join('、') || 'なし') + ' |');
			var aIn = Object.keys(incoming[id] || {}).map(Number).filter(function (t) { return t !== id; }).sort(function (a, b) { return a - b; });
			L.push('| ここへ来る元 | ' + (aIn.map(function (t) { return link(t); }).join('、') || 'なし') + ' |');
			L.push('| 数 | NPC ' + npcs.length + '・敵 ' + erows.length + ' 種類・お店 ' + (m.shops || []).length + '・イベント ' + (m.events || []).length + ' |');
			L.push('');

			if (erows.length) {
				L.push('## 敵', '');
				L.push('| 図 | 名前 | Lv | HP | MP | 力/素早/体力/賢さ/運 | 経験値 | お金 | 落とす(%) | 落とす物 | 出る数・間隔 |');
				L.push('| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |');
				erows.forEach(function (e) {
					var pts = e.points, sp;
					if (e.fixed) {
						sp = '決まった位置 (' + pts[0].x + ',' + pts[0].y + ')';
					} else {
						var iv = {};
						pts.forEach(function (p) { iv[p.interval] = true; });
						sp = pts.length + ' か所・最大 ' + pts.reduce(function (s, p) { return s + p.max; }, 0) + ' 体・' +
							Object.keys(iv).map(Number).sort(function (a, b) { return a - b; }).join('/') + ' 秒ごと';
					}
					L.push('| ' + [e.label, cell(e.name), e.level, e.hp, e.mana, [e.pw, e.dex, e.st, e.it, e.lk].join('/'), e.exp, e.gold, e.drop,
						cell(e.drops.join('、') || '-'), sp].join(' | ') + ' |');
				});
				var extra = erows.filter(function (e) { return e.magic.length || e.attr; });
				if (extra.length) {
					L.push('');
					extra.forEach(function (e) {
						var s = [];
						if (e.magic.length) s.push('特技: ' + e.magic.join('、'));
						if (e.attr) s.push('属性: ' + e.attr);
						L.push('- ' + e.label + ' ' + e.name + ' … ' + s.join(' / '));
					});
				}
				L.push('', '<details><summary>発生地点のくわしい位置</summary>', '');
				L.push('| 図 | 中心 | 範囲(±) | 最大数 | 間隔(秒) | キャラ番号 |', '| --- | --- | --- | --- | --- | --- |');
				erows.forEach(function (e) {
					e.points.forEach(function (p) {
						L.push('| ' + [e.label, '(' + p.x + ',' + p.y + ')', p.areaX + '×' + p.areaY, p.max, p.interval != null ? p.interval : '-', p.charNo].join(' | ') + ' |');
					});
				});
				L.push('', '</details>', '');
			}

			if (npcs.length) {
				L.push('## NPC', '', '| 番号 | 名前 | 位置 | 動き | 役割 | 台詞 |', '| --- | --- | --- | --- | --- | --- |');
				npcs.forEach(function (c, k) {
					var sp = isFunctionSpeech(speechOf(c)) ? '-' : (clean(speechOf(c)) || '（台詞なし）');
					L.push('| ' + [k + 1, cell(npcName(c) || '-'), '(' + c.dwX + ',' + c.dwY + ')', npcTypeName(c.byNpcType), npcRole(c) || '-', sp].join(' | ') + ' |');
				});
				L.push('');
			}

			if ((m.shops || []).length) {
				L.push('## お店', '');
				var clerk = {};
				npcs.forEach(function (c, k) {
					var n = shopNo(speechOf(c));
					if (n >= 0) (clerk[n] = clerk[n] || []).push(String(k + 1));
				});
				m.shops.forEach(function (sh, k) {
					var its = (sh.items || []).filter(function (x) { return x; });
					var who = clerk[k] ? '（NPC ' + clerk[k].join('・') + '）' : '（このマップに店員なし）';
					var first = clean((sh.messages || [])[0] || '');
					L.push('### ' + pad2(k) + '番 ' + who, '');
					if (first) {
						L.push('「' + first.replace(/^＊｢/, '').replace(/」$/, '') + '」', '');
					}
					if (its.length) {
						L.push('| アイテム | 種類 | 値段 |', '| --- | --- | --- |');
						its.forEach(function (x) {
							var it = items[x];
							if (it) {
								L.push('| ' + cell(it.szName || '#' + x) + ' | ' + (it.byType < ITEMTYPE.length ? ITEMTYPE[it.byType] : it.byType) + ' | ' + it.dwGold + ' G |');
							}
						});
					} else {
						L.push('売り物なし（買い取りだけ）');
					}
					L.push('');
				});
			}

			if (warps.length || moves.length || msgs.length || doors || heal.length || stinit.length) {
				L.push('## イベント', '');
				if (warps.length) {
					L.push('**別のマップへの入口**', '', '| 場所 | 行き先 |', '| --- | --- |');
					warps.slice().sort(function (a, b) {
						return ((a.to == null ? -1 : a.to) - (b.to == null ? -1 : b.to)) || (a.y - b.y) || (a.x - b.x);
					}).forEach(function (w) {
						L.push('| (' + w.x + ',' + w.y + ') | ' + link(w.to) + ' の (' + w.toX + ',' + w.toY + ') |');
					});
					L.push('');
				}
				if (msgs.length) {
					L.push('**メッセージ（看板・調べるなど）**', '', '| 場所 | 出かた | 内容 |', '| --- | --- | --- |');
					msgs.forEach(function (g) {
						L.push('| (' + g.x + ',' + g.y + ') | ' + g.how + ' | ' + (clean(g.text) || '（空）') + ' |');
					});
					L.push('');
				}
				var other = [];
				var pts = function (a) { return a.map(function (p) { return '(' + p[0] + ',' + p[1] + ')'; }).join(' '); };
				if (moves.length) other.push('同じマップ内の移動（階段・ワープ床など） ' + moves.length + ' か所');
				if (doors) other.push('扉 ' + doors + ' か所');
				if (heal.length) other.push('HP/MP 回復 ' + heal.length + ' か所: ' + pts(heal));
				if (stinit.length) other.push('ステータス初期化 ' + stinit.length + ' か所: ' + pts(stinit));
				other.forEach(function (o) { L.push('- ' + o); });
				if (other.length) L.push('');
			}

			if (objs.length) {
				var cnt = {}, order = [];
				objs.forEach(function (c) {
					var k = npcTypeName(c.byNpcType);
					if (!cnt[k]) { cnt[k] = 0; order.push(k); }
					cnt[k]++;
				});
				L.push('## そのほかのキャラ（台詞なしの飾り・押せる物など）', '');
				L.push(order.map(function (k) { return k + ' ' + cnt[k]; }).join('、'), '');
			}
			L.push('---', '[目次へ戻る](../README.md)');
			files.push({ path: 'maps/' + fn + '.md', text: L.join('\n') + '\n' });

			var hps = erows.map(function (e) { return e.hp; }), exps = erows.map(function (e) { return e.exp; });
			summary.push({
				id: id, name: nm, w: W, h: H, npc: npcs.length, enemy: erows.length, shop: (m.shops || []).length,
				battle: !!(m.byFlg & 2), sub: !!(m.byFlg & 4),
				hp: erows.length ? [Math.min.apply(null, hps), Math.max.apply(null, hps)] : null,
				exp: erows.length ? [Math.min.apply(null, exps), Math.max.apply(null, exps)] : null,
				outs: aOut
			});
			alljson[id] = {
				name: nm, size: [W, H], bgm: m.byBGM, flags: flg, weather: m.byWeather, light: m.byLightLevel,
				npcs: npcs.map(function (c, k) {
					return { no: k + 1, charNo: c.id, name: npcName(c), x: c.dwX, y: c.dwY, type: c.byNpcType, role: npcRole(c), speech: speechOf(c) };
				}),
				enemies: erows.map(function (e) {
					var o = {};
					Object.keys(e).forEach(function (k) { if (k !== 'col') o[k] = e[k]; });
					return o;
				}),
				shops: (m.shops || []).map(function (sh, k) {
					return {
						no: k, items: (sh.items || []).filter(function (x) { return x; }).map(function (x) {
							return { id: x, name: itemName(x), gold: items[x] ? items[x].dwGold : null };
						}), msgs: sh.messages || []
					};
				}),
				warps: warps, messages: msgs
			};
		});

		/* ---- 目次（README.md） ---- */
		function group(s) {
			if (s.name === '（名前なし）') return '名前の無いマップ';
			if (s.sub || GROUP_PLAY.some(function (k) { return s.name.indexOf(k) >= 0; })) return '遊び場・イベント・その他';
			if (s.battle) return 'フィールド・ダンジョン（戦闘あり）';
			return '町・村・建物（戦闘なし）';
		}
		function range(r) {
			if (!r) return '-';
			return r[0] === r[1] ? String(r[0]) : r[0] + '〜' + r[1];
		}
		var GORDER = ['町・村・建物（戦闘なし）', 'フィールド・ダンジョン（戦闘あり）', '遊び場・イベント・その他', '名前の無いマップ'];
		var G = {};
		GORDER.forEach(function (g) { G[g] = []; });
		summary.forEach(function (s) { G[group(s)].push(s); });
		var sum = function (k) { return summary.reduce(function (a, s) { return a + s[k]; }, 0); };
		var R = ['# SBO2003 マップ資料（NPC・敵・お店・イベント）', '',
			'各マップの全体図と、そこにいる NPC・敵・お店・イベントをまとめた資料です。データ作り込みの相談・企画用。', '',
			'- 元データ: ' + (opt.source || 'サーバーのデータ'),
			'- 作成日時: ' + (opt.date || ''),
			'- 1 マップ 1 ファイル（`maps/番号.md`）＋全体図（`images/番号.png`）。AI に読ませる用の JSON は `data/`。', '',
			'## 全体の数', '', '| マップ | NPC | 敵の種類（のべ） | お店 |', '| --- | --- | --- | --- |',
			'| ' + summary.length + ' | ' + sum('npc') + ' | ' + sum('enemy') + ' | ' + sum('shop') + ' |', ''];
		GORDER.forEach(function (g) {
			if (!G[g].length) return;
			R.push('## ' + g, '', '| 番号 | 名前 | 大きさ | NPC | 敵 | 敵のHP | 敵の経験値 | お店 | 行ける先 |',
				'| --- | --- | --- | --- | --- | --- | --- | --- | --- |');
			G[g].forEach(function (s) {
				R.push('| ' + [pad3(s.id), '[' + cell(s.name) + '](maps/' + pad3(s.id) + '.md)', s.w + '×' + s.h, s.npc, s.enemy, range(s.hp), range(s.exp), s.shop,
					s.outs.map(pad3).join(' ') || '-'].join(' | ') + ' |');
			});
			R.push('');
		});
		R.push('## 敵の索引（経験値の少ない順）', '', '| 敵 | HP | 経験値 | 出るマップ |', '| --- | --- | --- | --- |');
		var uniq = function (a) {
			var o = {};
			a.forEach(function (v) { o[v] = true; });
			return Object.keys(o).map(Number).sort(function (x, y) { return x - y; });
		};
		Object.keys(enemyIndex).map(function (name) {
			var a = enemyIndex[name];
			return { name: name, a: a, minExp: Math.min.apply(null, a.map(function (x) { return x.exp; })) };
		}).sort(function (x, y) {
			return (x.minExp - y.minExp) || (x.name < y.name ? -1 : x.name > y.name ? 1 : 0);
		}).forEach(function (e) {
			R.push('| ' + [cell(e.name), uniq(e.a.map(function (x) { return x.hp; })).join('/'), uniq(e.a.map(function (x) { return x.exp; })).join('/'),
				uniq(e.a.map(function (x) { return x.map; })).map(function (t) { return link(t, 'maps/'); }).join('、')].join(' | ') + ' |');
		});
		R.push('', '## 資料の書式（各マップのファイル）', '',
			'1. **全体図** … 屋根を外した全体図。青い数字＝NPC、色つきの枠と E番号＝敵の出る範囲、水色の枠「→番号」＝別マップへの入口、緑の小枠＝同じマップ内の移動、黄色の小枠＝メッセージ。',
			'2. **基本情報** … 大きさ・BGM・天候・戦闘できるか・行ける先・来る元。',
			'3. **敵** … ステータステンプレートの値（Lv・HP・MP・能力・経験値・お金・落とす確率と物）と、発生地点ごとの数・間隔。テンプレートを使わない敵はキャラ自身の値。',
			'4. **NPC** … 名前（無ければ台詞の【】から）・位置・動き・役割（お店・預かり所・転職・着せ替え・アイテムをくれる等）・台詞（制御コードは読みやすく置き換え）。',
			'5. **お店** … 店番号ごとの売り物と値段。',
			'6. **イベント** … 入口の行き先、看板などのメッセージ、扉・回復・階段の数。', '',
			'台詞の置き換え: `@0`→「▼」（入力待ち）、`@1`→改行（空白）、`@4NNNNN`→〔アイテム「…」をもらう〕、`@8MMMXXXYYY`→〔復活地点を…に〕。', '',
			'## 気づいたこと（調整の候補）', '');
		var enemyTmpl = sttmp.filter(function (x) { return x && ENEMY_TYPES[x.byNpcType]; });
		var lv0 = enemyTmpl.filter(function (x) { return x.byLevel === 0; });
		R.push('- **Lv が 0 の敵テンプレート** … 敵のテンプレート ' + enemyTmpl.length + ' 件のうち ' + lv0.length + ' 件。強さの目安は HP と経験値で見るのが確実です。');
		var nb = summary.filter(function (s) { return s.battle && !s.enemy && s.name !== '（名前なし）'; });
		R.push('- **戦闘できるのに敵がいないマップ** … ' + (nb.map(function (s) { return link(s.id, 'maps/'); }).join('、') || 'なし'));
		var nc = [];
		mapIds.forEach(function (id) {
			var m = maps[id], have = {};
			if (!m.szName) return;
			(byMap[id] || []).forEach(function (c) {
				var n = shopNo(speechOf(c));
				if (n >= 0) have[n] = true;
			});
			(m.shops || []).forEach(function (sh, k) {
				var its = (sh.items || []).filter(function (x) { return x; });
				if (its.length && !have[k]) {
					nc.push(pad3(id) + ' ' + m.szName + ' の ' + pad2(k) + '番（' + its.slice(0, 2).map(itemName).join('、') + ' など）');
				}
			});
		});
		R.push('- **売り物はあるのに店員がいないお店** … ' + (nc.join(' / ') || 'なし'));
		var inn = {};
		D.chars.forEach(function (c) {
			if (c && !c.isPlayer && speechOf(c).indexOf('準備中') >= 0 && maps[c.dwMapID]) inn[c.dwMapID] = true;
		});
		var aInn = Object.keys(inn).map(Number).sort(function (a, b) { return a - b; });
		R.push('- **「準備中」と話す NPC がいるマップ** … ' + aInn.length + ' か所（' + aInn.map(function (t) { return pad3(t) + ' ' + mapName(t); }).join('、') + '）。宿屋などの準備中の台詞です。');
		var un = summary.filter(function (s) { return s.name === '（名前なし）'; });
		if (un.length) {
			R.push('- **名前の無いマップが ' + un.length + ' 枚**（' + un.map(function (s) { return pad3(s.id); }).join(' ') + '）');
		}
		var emp = summary.filter(function (s) { return !s.npc && !s.enemy && s.name !== '（名前なし）'; });
		R.push('- **NPC も敵もいないマップ** … ' + (emp.map(function (s) { return link(s.id, 'maps/'); }).join('、') || 'なし'));
		R.push('');
		files.unshift({ path: 'README.md', text: R.join('\n') + '\n' });
		files.push({ path: 'data/maps.json', text: JSON.stringify(alljson, null, 1) });
		files.push({ path: 'data/summary.json', text: JSON.stringify(summary, null, 1) });
		return { files: files, plans: plans };
	}

	/* ---------------------------------------------------------------- */
	/* 全体図（ブラウザの canvas で描く）								 */
	/* ---------------------------------------------------------------- */

	var T = 16;		/* 画像のマスの大きさ（クライアントは 2 倍にして使う） */

	function loadImage(url) {
		return new Promise(function (resolve, reject) {
			var img = new Image();
			img.onload = function () { resolve(img); };
			img.onerror = function () { reject(new Error(url + ' が読めません')); };
			img.src = url;
		});
	}
	/* 黒（0,0,0）を透明にした canvas にする（ゲームと同じカラーキー） */
	function keyed(img) {
		var cv = document.createElement('canvas');
		cv.width = img.width;
		cv.height = img.height;
		var ctx = cv.getContext('2d');
		ctx.drawImage(img, 0, 0);
		var d = ctx.getImageData(0, 0, cv.width, cv.height);
		for (var i = 0; i < d.data.length; i += 4) {
			if (d.data[i] === 0 && d.data[i + 1] === 0 && d.data[i + 2] === 0) {
				d.data[i + 3] = 0;
			}
		}
		ctx.putImageData(d, 0, 0);
		return cv;
	}
	function loadSheets(base) {
		var a = [], i;
		for (i = 1; i <= 5; i++) a.push(loadImage(base + 'map' + pad2(i) + '.png'));
		for (i = 1; i <= 16; i++) a.push(loadImage(base + 'char' + pad2(i) + '.png'));
		return Promise.all(a).then(function (imgs) {
			var k = imgs.map(keyed);
			return { map: k.slice(0, 5), chr: k.slice(5) };
		});
	}

	/* パーツ 1 つの 16×16 の絵（背景を重ねてから本体。屋根は本体を描かない） */
	function partCanvas(parts, sheets, cache, w) {
		if (cache[w]) {
			return cache[w];
		}
		var p = parts[w] || parts[0];
		var lv = p.byLevel, g = p.wGrpID, b = p.wBackID;
		if (p.anime && p.anime.length) {
			lv = p.anime[0].byLevel;
			g = p.anime[0].wGrpID;
			b = p.anime[0].wBackID;
		}
		var cv = document.createElement('canvas');
		cv.width = cv.height = T;
		var ctx = cv.getContext('2d');
		ctx.fillStyle = '#000';
		ctx.fillRect(0, 0, T, T);
		function put(no, alpha) {
			var s = Math.floor(no / 256), n = no % 256;
			if (s >= sheets.map.length) return;
			ctx.globalAlpha = alpha;
			ctx.drawImage(sheets.map[s], (n % 16) * T, Math.floor(n / 16) * T, T, T, 0, 0, T, T);
			ctx.globalAlpha = 1;
		}
		put(b ? b : g, 1);
		if (!HIDE_MAIN[p.byHitType]) {
			put(g, lv ? lv / 100 : 1);
		}
		cache[w] = cv;
		return cv;
	}

	function drawBadge(ctx, x, y, text, fill, sc) {
		var fs = Math.max(11, 6 * sc);
		ctx.font = fs + 'px sans-serif';
		var tw = ctx.measureText(text).width, h = Math.max(13, 7 * sc + 2);
		ctx.fillStyle = fill;
		ctx.strokeStyle = '#fff';
		ctx.lineWidth = 1;
		ctx.beginPath();
		if (ctx.roundRect) {
			ctx.roundRect(x - tw / 2 - 2, y - h / 2, tw + 4, h, 3);
		} else {
			ctx.rect(x - tw / 2 - 2, y - h / 2, tw + 4, h);
		}
		ctx.fill();
		ctx.stroke();
		ctx.fillStyle = '#fff';
		ctx.textAlign = 'center';
		ctx.textBaseline = 'middle';
		ctx.fillText(text, x, y + 1);
	}

	/* マップ 1 枚を描く。tiles は parts[行][列]。bSmall なら等倍（ファイルが 1/3 くらいになる） */
	function drawMap(plan, tiles, parts, sheets, bSmall) {
		var W = plan.W, H = plan.H;
		var sc = (!bSmall && Math.max(W, H) <= 128) ? 2 : 1, S = T * sc;
		var cv = document.createElement('canvas');
		cv.width = W * S;
		cv.height = H * S;
		var ctx = cv.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		var cache = {}, x, y;
		for (y = 0; y < H; y++) {
			var row = tiles[y] || [];
			for (x = 0; x < W; x++) {
				ctx.drawImage(partCanvas(parts, sheets, cache, row[x] || 0), x * S, y * S, S, S);
			}
		}
		function rgba(c, a) {
			return 'rgba(' + c[0] + ',' + c[1] + ',' + c[2] + ',' + a + ')';
		}
		/* 敵の出る範囲 */
		plan.erows.forEach(function (e) {
			if (e.fixed) return;
			e.points.forEach(function (p) {
				var x0 = Math.max(0, p.x - p.areaX), x1 = Math.min(W - 1, p.x + p.areaX);
				var y0 = Math.max(0, p.y - p.areaY), y1 = Math.min(H - 1, p.y + p.areaY);
				ctx.fillStyle = rgba(e.col, 0.16);
				ctx.fillRect(x0 * S, y0 * S, (x1 - x0 + 1) * S, (y1 - y0 + 1) * S);
				ctx.strokeStyle = rgba(e.col, 0.86);
				ctx.lineWidth = 2;
				ctx.strokeRect(x0 * S + 1, y0 * S + 1, (x1 - x0 + 1) * S - 2, (y1 - y0 + 1) * S - 2);
			});
		});
		/* キャラ（正面向き 1 コマ目） */
		function drawChar(grp, dir, px, py) {
			var no = Math.floor(grp / 16);
			if (!grp || no >= sheets.chr.length) return;
			var d = dir < 4 ? dir : 1;
			ctx.drawImage(sheets.chr[no], d * 32, (grp % 16) * T, T, T, px, py, S, S);
		}
		plan.npcs.concat(plan.objs, plan.fixed).forEach(function (c) {
			if (c.wGrpMain === 0) drawChar(c.wGrpID, c.byDirection, c.dwX * S, c.dwY * S - 4 * sc);
		});
		plan.erows.forEach(function (e) {
			if (e.fixed) return;
			e.points.forEach(function (p) { drawChar(e.grp, 1, p.x * S, p.y * S); });
		});
		/* イベント */
		ctx.lineWidth = 2;
		plan.warps.forEach(function (w) {
			ctx.strokeStyle = 'rgb(0,230,255)';
			ctx.strokeRect(w.x * S + 1, w.y * S + 1, S - 2, S - 2);
		});
		ctx.lineWidth = 1;
		plan.moves.forEach(function (w) {
			ctx.strokeStyle = 'rgba(120,255,120,0.9)';
			ctx.strokeRect(w.x * S + 2.5, w.y * S + 2.5, S - 5, S - 5);
		});
		plan.msgs.forEach(function (w) {
			ctx.strokeStyle = 'rgba(255,255,120,0.9)';
			ctx.strokeRect(w.x * S + 2.5, w.y * S + 2.5, S - 5, S - 5);
		});
		/* 番号 */
		plan.erows.forEach(function (e) {
			e.points.forEach(function (p) { drawBadge(ctx, p.x * S + S / 2, p.y * S - 2 * sc, e.label, rgba(e.col, 1), sc); });
		});
		plan.npcs.forEach(function (c, k) {
			drawBadge(ctx, c.dwX * S + S / 2, c.dwY * S - 2 * sc, String(k + 1), 'rgb(40,90,200)', sc);
		});
		plan.warps.forEach(function (w) {
			drawBadge(ctx, w.x * S + S / 2, w.y * S + S + 2 * sc, w.to != null ? '→' + w.to : '→?', 'rgb(0,140,170)', sc);
		});
		return cv;
	}

	/* ---------------------------------------------------------------- */
	/* zip（圧縮しないで詰める。PNG はもともと圧縮済み）					 */
	/* ---------------------------------------------------------------- */
	var CRC_TABLE = (function () {
		var t = [], c, n, k;
		for (n = 0; n < 256; n++) {
			c = n;
			for (k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
			t[n] = c >>> 0;
		}
		return t;
	})();
	function crc32(u8) {
		var c = 0xFFFFFFFF;
		for (var i = 0; i < u8.length; i++) c = CRC_TABLE[(c ^ u8[i]) & 0xFF] ^ (c >>> 8);
		return (c ^ 0xFFFFFFFF) >>> 0;
	}
	function utf8(s) {
		return new TextEncoder().encode(s);
	}
	/* entries: [{path, data: Uint8Array}] → Uint8Array の配列（Blob にする） */
	function makeZip(entries, date) {
		var parts = [], central = [], offset = 0;
		var d = date || new Date();
		var dosTime = (d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() >> 1);
		var dosDate = ((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate();
		entries.forEach(function (e) {
			var name = utf8(e.path), data = e.data, crc = crc32(data);
			var h = new DataView(new ArrayBuffer(30));
			h.setUint32(0, 0x04034b50, true);
			h.setUint16(4, 20, true);
			h.setUint16(6, 0x0800, true);		/* 名前は UTF-8 */
			h.setUint16(8, 0, true);			/* 圧縮なし */
			h.setUint16(10, dosTime, true);
			h.setUint16(12, dosDate, true);
			h.setUint32(14, crc, true);
			h.setUint32(18, data.length, true);
			h.setUint32(22, data.length, true);
			h.setUint16(26, name.length, true);
			h.setUint16(28, 0, true);
			parts.push(new Uint8Array(h.buffer), name, data);
			var c = new DataView(new ArrayBuffer(46));
			c.setUint32(0, 0x02014b50, true);
			c.setUint16(4, 20, true);
			c.setUint16(6, 20, true);
			c.setUint16(8, 0x0800, true);
			c.setUint16(10, 0, true);
			c.setUint16(12, dosTime, true);
			c.setUint16(14, dosDate, true);
			c.setUint32(16, crc, true);
			c.setUint32(20, data.length, true);
			c.setUint32(24, data.length, true);
			c.setUint16(28, name.length, true);
			c.setUint32(42, offset, true);
			central.push(new Uint8Array(c.buffer), name);
			offset += 30 + name.length + data.length;
		});
		var cdSize = central.reduce(function (s, a) { return s + a.length; }, 0);
		var end = new DataView(new ArrayBuffer(22));
		end.setUint32(0, 0x06054b50, true);
		end.setUint16(8, entries.length, true);
		end.setUint16(10, entries.length, true);
		end.setUint32(12, cdSize, true);
		end.setUint32(16, offset, true);
		return parts.concat(central, [new Uint8Array(end.buffer)]);
	}

	/* ---------------------------------------------------------------- */
	/* 全部をまとめて作る												 */
	/* ---------------------------------------------------------------- */

	/*
	 * api(url, body) → Promise<data>（管理ページの api 関数。session は呼ぶ側で入れる）
	 * onProgress(text)
 * opt: { mapId?（1 枚だけ）, small?（絵を等倍で描く）, grpBase? }
	 * 返す: Promise<{ blob, name, count }>
	 */
	function build(api, onProgress, opt) {
		opt = opt || {};
		var say = onProgress || function () {};
		function check(r) {
			if (!r || r.error || !r.records && !r.parts) {
				throw new Error((r && (r.message || r.error)) || '読めませんでした');
			}
			return r;
		}
		function listAll(kind, label) {
			var out = [];
			function next(offset) {
				say(label + 'を読んでいます…（' + out.length + ' 件）');
				return api('/api/admin/data/list', { kind: kind, offset: offset, limit: 500 }).then(function (r) {
					check(r);
					out = out.concat(r.records);
					return r.more ? next(offset + r.records.length) : out;
				});
			}
			return next(0).then(function () {
				/* 番号で引けるように並べる（抜けは null） */
				var a = [];
				out.forEach(function (rec) { a[rec.id] = rec; });
				return a;
			});
		}
		function compact(a) {
			return a.filter(function (r) { return r; });
		}
		var D = {}, sheets;
		var base = opt.grpBase || '/admin-grp/';
		return loadSheets(base).then(function (s) {
			sheets = s;
			return listAll('map', 'マップ');
		}).then(function (a) {
			D.maps = compact(a);
			return listAll('char', 'キャラ');
		}).then(function (a) {
			D.chars = compact(a);
			return listAll('item', 'アイテム');
		}).then(function (a) {
			D.items = a;
			return listAll('stTmp', 'ステータステンプレート');
		}).then(function (a) {
			D.stTmp = a;
			return listAll('magic', '特技');
		}).then(function (a) {
			D.magic = a;
			return listAll('mapParts', 'マップパーツ');
		}).then(function (a) {
			D.parts = a;
			var now = new Date();
			var res = makeDocs(D, {
				source: 'サーバー ' + location.host + ' のデータ（管理ページから書き出し）',
				date: now.getFullYear() + '-' + pad2(now.getMonth() + 1) + '-' + pad2(now.getDate()) + ' ' + pad2(now.getHours()) + ':' + pad2(now.getMinutes())
			});
			var entries = res.files.map(function (f) { return { path: f.path, data: utf8(f.text) }; });
			var only = opt.mapId != null ? res.plans.filter(function (p) { return p.id === opt.mapId; }) : res.plans;
			if (opt.mapId != null) {
				entries = entries.filter(function (e) { return e.path === 'maps/' + pad3(opt.mapId) + '.md'; });
			}
			var k = 0;
			function nextMap() {
				if (k >= only.length) {
					return Promise.resolve();
				}
				var plan = only[k++];
				say('全体図を描いています…（' + k + ' / ' + only.length + '）');
				return readTiles(plan).then(function (tiles) {
					var cv = drawMap(plan, tiles, D.parts, sheets, !!opt.small);
					return new Promise(function (resolve) { cv.toBlob(resolve, 'image/png'); });
				}).then(function (blob) {
					return blob.arrayBuffer();
				}).then(function (buf) {
					entries.push({ path: 'images/' + pad3(plan.id) + '.png', data: new Uint8Array(buf) });
					return nextMap();
				});
			}
			/* マス目は 1 回 128×128 まで */
			function readTiles(plan) {
				var tiles = [], jobs = [], x, y;
				for (y = 0; y < plan.H; y += 128) {
					for (x = 0; x < plan.W; x += 128) {
						jobs.push([x, y]);
					}
				}
				return jobs.reduce(function (p, j) {
					return p.then(function () {
						return api('/api/admin/data/tiles', { id: plan.id, x: j[0], y: j[1], w: 128, h: 128 }).then(function (r) {
							check(r);
							r.parts.forEach(function (row, dy) {
								var t = tiles[j[1] + dy] = tiles[j[1] + dy] || [];
								row.forEach(function (v, dx) { t[j[0] + dx] = v; });
							});
						});
					});
				}, Promise.resolve()).then(function () { return tiles; });
			}
			return nextMap().then(function () {
				say('zip にまとめています…');
				var stamp = now.getFullYear() + pad2(now.getMonth() + 1) + pad2(now.getDate());
				var name = opt.mapId != null ? 'sbo2003-map-' + pad3(opt.mapId) + '-' + stamp + '.zip' : 'sbo2003-map-docs-' + stamp + '.zip';
				return { blob: new Blob(makeZip(entries, now), { type: 'application/zip' }), name: name, count: only.length };
			});
		});
	}

	/* loadSheets・partCanvas はパーツ図鑑・配置案（admin-design.js）でも使う */
	var api = { makeDocs: makeDocs, drawMap: drawMap, makeZip: makeZip, crc32: crc32, build: build,
		loadSheets: loadSheets, partCanvas: partCanvas, T: T };
	if (typeof module !== 'undefined' && module.exports) {
		module.exports = api;
	} else {
		root.SboMapDoc = api;
	}
})(this);
