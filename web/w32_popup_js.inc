/* ========================================================================= */
/* ファイル名：	w32_popup_js.inc											 */
/* 概要：		小窓（管理者ウィンドウ・ダイアログ）の JavaScript 側			 */
/*				w32_html.cpp から読み込む									 */
/* ========================================================================= */
/*
 * 部品の種類（PopupType と同じ番号）
 *   0:ウィンドウ（入れ子の div）  1:入力欄  2:押しボタン  3:チェックボックス
 *   4:ラジオボタン  5:グループ枠  6:スタティック  7:コンボボックス
 *   8:リストボックス  9:スクロールバー  10:リストビュー  11:小窓の枠（トップレベル）
 */

EM_JS(void, W32JsPopInit, (void), {
	if (Module['w32Pop']) {
		return;
	}
	var P = {};
	Module['w32Pop'] = P;
	if (!Module['w32Html']) {
		Module['w32Html'] = {};
	}
	P.z = 10;

	var css = [
		'.w32f{position:absolute;box-sizing:border-box;border:3px solid #a9b8d0;background:#f0f0f0;color:#000;',
		'font:12px "MS PGothic","BIZ UDPGothic","Hiragino Sans","Hiragino Kaku Gothic ProN","Yu Gothic UI","Meiryo UI","Noto Sans CJK JP",sans-serif;line-height:normal;',
		'box-shadow:0 3px 12px rgba(0,0,0,.45);display:flex;flex-direction:column;user-select:none;text-align:left}',
		'.w32f.w32f-act{border-color:#6f8fc8}',
		'.w32f-title{height:19px;flex:none;display:flex;align-items:center;background:#c9d4e6;color:#333;padding:0 2px 0 5px;cursor:move;font-weight:bold;overflow:hidden}',
		'.w32f.w32f-act .w32f-title{background:linear-gradient(#5b86cf,#3f6cbb);color:#fff}',
		'.w32f-title>span{flex:1;overflow:hidden;white-space:nowrap;text-overflow:ellipsis}',
		'.w32f-close{width:22px;height:16px;border:none;border-radius:2px;background:#c94a4a;color:#fff;font:bold 11px sans-serif;padding:0;cursor:pointer}',
		'.w32f-close:hover{background:#e05555}',
		'.w32f-menu{height:19px;flex:none;display:flex;background:#f6f6f6;border-bottom:1px solid #d8d8d8;box-sizing:border-box}',
		'.w32f-menu>div{padding:0 7px;line-height:18px;cursor:default;white-space:nowrap}',
		'.w32f-menu>div:hover,.w32f-menu>div.w32-open{background:#cce4f7}',
		'.w32m-drop{position:fixed;background:#f5f5f5;border:1px solid #979797;box-shadow:2px 2px 5px rgba(0,0,0,.3);',
		'z-index:2147483000;min-width:140px;padding:2px 0;font:12px "MS PGothic","BIZ UDPGothic","Hiragino Sans","Hiragino Kaku Gothic ProN","Yu Gothic UI","Meiryo UI","Noto Sans CJK JP",sans-serif;color:#000}',
		'.w32m-drop>div{padding:3px 24px 3px 24px;white-space:nowrap;position:relative;cursor:default}',
		'.w32m-drop>div:hover{background:#cce4f7}',
		'.w32m-drop>div.w32-dis{color:#9a9a9a}',
		'.w32m-drop>div.w32-chk::before{content:"✓";position:absolute;left:8px}',
		'.w32m-drop>div.w32-sub{padding-left:36px}',
		'.w32m-drop>hr{border:none;border-top:1px solid #d0d0d0;margin:3px 2px}',
		'.w32f-client{position:relative;flex:none;overflow:hidden}',
		'.w32c{position:absolute;box-sizing:border-box;margin:0;font:inherit;color:inherit}',
		'.w32-wnd{overflow:hidden}',
		'.w32-dlg{background:#f0f0f0}',
		'.w32-cv{position:absolute;left:0;top:0;pointer-events:none}',
		'.w32-static{white-space:pre;overflow:visible;pointer-events:none;line-height:1.2}',
		'.w32-static[class*="w32-frame"]{overflow:hidden}',
		'.w32-static.w32-notify{pointer-events:auto}',
		'.w32-frame-black{border:1px solid #000}',
		'.w32-frame-gray{border:1px solid #a0a0a0}',
		'.w32-frame-etched{border:1px solid #a0a0a0;box-shadow:1px 1px 0 #fff}',
		'.w32-group{pointer-events:none}',
		'.w32-group::before{content:"";position:absolute;left:0;right:0;top:6px;bottom:0;border:1px solid #c8c8c8;border-radius:3px}',
		'.w32-group>span{position:absolute;left:7px;top:0;background:#f0f0f0;padding:0 2px;line-height:12px;white-space:nowrap}',
		'.w32-btn{padding:0;letter-spacing:-0.03em;background:#e1e1e1;border:1px solid #adadad;border-radius:0;cursor:default;white-space:nowrap;overflow:hidden}',
		'.w32-btn:hover:not(:disabled){background:#e5f1fb;border-color:#0078d7}',
		'.w32-btn:active:not(:disabled){background:#cce4f7}',
		'.w32-btn.w32-def{border-color:#0078d7}',
		'.w32-btn:disabled{color:#9a9a9a;border-color:#cfcfcf;background:#efefef}',
		'.w32-edit{padding:1px 2px;border:1px solid #7a7a7a;background:#fff;outline:none;border-radius:0;resize:none}',
		'.w32-edit:focus{border-color:#0078d7}',
		'.w32-edit:disabled,.w32-edit:read-only{background:#f0f0f0;color:#555}',
		'.w32-edit.w32-noborder{border:none}',
		'.w32-combo{padding:0 0 0 2px;border:1px solid #7a7a7a;background:#fff;border-radius:0;outline:none}',
		'.w32-combo:focus{border-color:#0078d7}',
		'.w32-combo:disabled{background:#f0f0f0;color:#888}',
		'.w32-list{padding:0;border:1px solid #828790;background:#fff;outline:none}',
		'.w32-check{display:flex;align-items:center;white-space:nowrap;overflow:visible;background:#f0f0f0}',
		'.w32-check>input{margin:0 4px 0 0;flex:none}',
		'.w32-check.w32-left{flex-direction:row-reverse;justify-content:flex-end}',
		'.w32-check.w32-left>input{margin:0 0 0 4px}',
		'.w32-sb{display:flex;background:#f0f0f0;user-select:none;overflow:hidden}',
		'.w32-sb.w32-v{flex-direction:column}',
		'.w32-sb>.w32-sbb{flex:none;display:flex;align-items:center;justify-content:center;font-size:8px;color:#606060;background:#f0f0f0}',
		'.w32-sb.w32-v>.w32-sbb{height:17px}',
		'.w32-sb.w32-h>.w32-sbb{width:17px}',
		'.w32-sb>.w32-sbb:hover{background:#dadada}',
		'.w32-sb>.w32-sbb:active{background:#606060;color:#fff}',
		'.w32-sb>.w32-sbt{flex:1;position:relative;min-width:0;min-height:0}',
		'.w32-sbt>.w32-sbth{position:absolute;background:#cdcdcd}',
		'.w32-sbt>.w32-sbth:hover{background:#a6a6a6}',
		'.w32-sbt>.w32-sbth.w32-drag{background:#606060}',
		'.w32-sb.w32-v .w32-sbth{left:2px;right:2px}',
		'.w32-sb.w32-h .w32-sbth{top:2px;bottom:2px}',
		'.w32-lv{background:#fff;border:1px solid #828790;overflow:auto}',
		'.w32-lv>table{border-collapse:collapse;table-layout:fixed;cursor:default}',
		'.w32-lv th{position:sticky;top:0;z-index:1;background:#fff;border-right:1px solid #e5e5e5;border-bottom:1px solid #e5e5e5;',
		'font-weight:normal;text-align:left;padding:1px 4px;overflow:hidden;white-space:nowrap;height:18px;box-sizing:border-box}',
		'.w32-lv td{padding:0 4px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;height:17px}',
		'.w32-lv.w32-grid td{border-right:1px solid #f0f0f0;border-bottom:1px solid #f0f0f0}',
		'.w32-lv tr.w32-sel td{background:#cce8ff}',
		'.w32-lv:focus-within tr.w32-sel td{background:#0078d7;color:#fff}',
		'.w32-lv td input{margin:0 3px 0 0;vertical-align:-2px;display:none}',
		'.w32-lv.w32-cb td input{display:inline-block}',
		'.w32-wnd[inert],.w32f-client[inert]{filter:none}',
		'.w32f-grip{position:absolute;z-index:5;touch-action:none}',
		'.w32f-grip.w32-r{top:0;right:-3px;width:7px;bottom:12px;cursor:ew-resize}',
		'.w32f-grip.w32-b{left:0;bottom:-3px;height:7px;right:12px;cursor:ns-resize}',
		'.w32f-grip.w32-rb{right:-3px;bottom:-3px;width:15px;height:15px;cursor:nwse-resize;',
		'background:linear-gradient(135deg,transparent 55%,#8a9bb8 55%,#8a9bb8 62%,transparent 62%,transparent 72%,#8a9bb8 72%,#8a9bb8 79%,transparent 79%)}'
	].join('');
	var style = document.createElement('style');
	style.textContent = css;
	document.head.appendChild(style);

	var layer = document.createElement('div');
	layer.id = 'sbo-popups';
	layer.style.cssText = 'position:absolute;left:0;top:0;width:0;height:0;z-index:1000;';
	document.body.appendChild(layer);
	P.layer = layer;

	/* 画面座標（メインウィンドウの左上が 0,0。メインウィンドウの大きさで拡大率を掛ける）
	   → ページ上の座標 */
	P.screenInfo = function (nScrCx) {
		var cv = Module['canvas'];
		var r = cv ? cv.getBoundingClientRect() : {left: 0, top: 0, width: 640};
		var s = (cv && nScrCx > 0 && r.width > 0) ? r.width / nScrCx : 1;
		return {x: r.left + window.scrollX, y: r.top + window.scrollY, s: s};
	};

	/* 「&File」→「File」（&& は & のまま） */
	P.strip = function (t) {
		/* EM_JS の中では正規表現に \ を書けないので split / join で置き換える */
		return t.split('&&').map(function (s) {
			return s.split('&').join('');
		}).join('&');
	};

	/* 一番手前にする */
	P.front = function (frame) {
		if (P.active && P.active !== frame) {
			P.active.classList.remove('w32f-act');
		}
		P.active = frame;
		frame.classList.add('w32f-act');
		frame.style.zIndex = String(++ P.z);
		/* 持ち主の後ろに隠れないよう、この小窓が持っている小窓も手前へ（Windows と同じ） */
		var owned = [];
		var frames = P.layer.querySelectorAll('.w32f');
		for (var i = 0; i < frames.length; i ++) {
			for (var o = frames[i].w32Owner; o; o = o.w32Owner) {
				if (o === frame) {
					owned.push(frames[i]);
					break;
				}
			}
		}
		owned.sort(function (a, b) { return (+a.style.zIndex || 0) - (+b.style.zIndex || 0); });
		for (var j = 0; j < owned.length; j ++) {
			owned[j].style.zIndex = String(++ P.z);
		}
		if (owned.length && frame.w32Client && frame.w32Client.inert) {
			frame.classList.remove('w32f-act');
			P.active = owned[owned.length - 1];
			P.active.classList.add('w32f-act');
		}
	};

	/* ---- メニュー ---- */
	P.closeMenu = function () {
		if (P.drop) {
			P.drop.remove();
			P.drop = null;
		}
		if (P.menuItem) {
			P.menuItem.classList.remove('w32-open');
			P.menuItem = null;
		}
		P.menuOwner = null;
	};
	document.addEventListener('mousedown', function (ev) {
		if (P.drop && !P.drop.contains(ev.target) && !(P.menuItem && P.menuItem.contains(ev.target))) {
			P.closeMenu();
		}
	}, true);
	P.openMenu = function (client, barItem, items) {
		P.closeMenu();
		var drop = document.createElement('div');
		drop.className = 'w32m-drop';
		var add = function (list, depth) {
			list.forEach(function (it) {
				if (it.sep) {
					drop.appendChild(document.createElement('hr'));
					return;
				}
				var d = document.createElement('div');
				d.textContent = P.strip(it.t);
				if (depth) {
					d.classList.add('w32-sub');
				}
				if (it.chk) {
					d.classList.add('w32-chk');
				}
				if (it.dis) {
					d.classList.add('w32-dis');
				}
				drop.appendChild(d);
				if (it.sub) {
					d.classList.add('w32-dis');
					add(it.sub, depth + 1);
					return;
				}
				d.addEventListener('click', function () {
					P.closeMenu();
					if (!it.dis) {
						_W32JsOnMenu(client.w32Id, it.id);
					}
				});
			});
		};
		add(items, 0);
		var r = barItem.getBoundingClientRect();
		drop.style.left = r.left + 'px';
		drop.style.top = r.bottom + 'px';
		document.body.appendChild(drop);
		/* 画面からはみ出さないように */
		var dr = drop.getBoundingClientRect();
		if (dr.right > window.innerWidth) {
			drop.style.left = Math.max(0, window.innerWidth - dr.width) + 'px';
		}
		P.drop = drop;
		P.menuItem = barItem;
		P.menuOwner = client;
		barItem.classList.add('w32-open');
	};

	/* ---- マウス（ウィンドウの何も無い所） ---- */
	P.attachMouse = function (el) {
		var send = function (ev, type) {
			if (ev.target !== el) {
				return;
			}
			var r = el.getBoundingClientRect();
			var b = ((ev.buttons & 1) ? 1 : 0) | ((ev.buttons & 2) ? 2 : 0);
			_W32JsOnWndMouse(el.w32Id, type, ev.button || 0, b, Math.floor(ev.clientX - r.left), Math.floor(ev.clientY - r.top));
		};
		el.addEventListener('mousedown', function (ev) {
			send(ev, 1);
		});
		el.addEventListener('mouseup', function (ev) {
			send(ev, 2);
		});
		el.addEventListener('dblclick', function (ev) {
			send(ev, 3);
		});
		el.addEventListener('mousemove', function (ev) {
			if (ev.buttons) {
				send(ev, 0);
			}
		});
		el.addEventListener('contextmenu', function (ev) {
			ev.preventDefault();
		});
	};

	/* ---- 入力欄のキー（ゲームには渡さない） ---- */
	P.attachKeys = function (el, id) {
		el.addEventListener('keydown', function (ev) {
			ev.stopPropagation();
			if (ev.isComposing || ev.keyCode === 229) {
				return;
			}
			var vk = Module['w32KeyToVk'] ? Module['w32KeyToVk'](ev) : ev.keyCode;
			_W32JsOnKey(vk, 1, id);
		});
		el.addEventListener('keyup', function (ev) {
			ev.stopPropagation();
			var vk = Module['w32KeyToVk'] ? Module['w32KeyToVk'](ev) : ev.keyCode;
			_W32JsOnKey(vk, 0, id);
		});
	};

	/* ---- 小窓の枠 ---- */
	P.makeFrame = function (id, style) {
		var frame = document.createElement('div');
		frame.className = 'w32f';
		frame.style.display = 'none';
		var title = document.createElement('div');
		title.className = 'w32f-title';
		var span = document.createElement('span');
		title.appendChild(span);
		if (style & 0x00080000) {		/* WS_SYSMENU */
			var close = document.createElement('button');
			close.className = 'w32f-close';
			close.type = 'button';
			close.textContent = '✕';
			close.title = '閉じる';
			close.addEventListener('mousedown', function (ev) {
				ev.stopPropagation();
			});
			close.addEventListener('click', function () {
				_W32JsOnClose(id);
			});
			title.appendChild(close);
		}
		var menu = document.createElement('div');
		menu.className = 'w32f-menu';
		menu.style.display = 'none';
		var client = document.createElement('div');
		client.className = 'w32f-client w32-wnd';
		frame.appendChild(title);
		frame.appendChild(menu);
		frame.appendChild(client);
		P.layer.appendChild(frame);
		client.w32Frame = frame;
		client.w32Title = span;
		client.w32Menu = menu;
		frame.addEventListener('mousedown', function () {
			P.front(frame);
		}, true);
		/* タイトルを持って動かす */
		title.addEventListener('mousedown', function (ev) {
			if (ev.button !== 0) {
				return;
			}
			ev.preventDefault();
			var sx = ev.clientX, sy = ev.clientY;
			var ox = frame.offsetLeft, oy = frame.offsetTop;
			var move = function (e) {
				var nx = Math.max(-frame.offsetWidth + 60, ox + e.clientX - sx);
				var ny = Math.max(0, oy + e.clientY - sy);
				frame.style.left = nx + 'px';
				frame.style.top = ny + 'px';
			};
			var up = function () {
				window.removeEventListener('mousemove', move);
				window.removeEventListener('mouseup', up);
				var info = P.screenInfo(client.w32ScrCx || 0);
				var x = Math.round((frame.offsetLeft - info.x) / info.s);
				var y = Math.round((frame.offsetTop - info.y) / info.s);
				client.w32X = x;
				client.w32Y = y;
				_W32JsOnFrameMove(id, x, y);
			};
			window.addEventListener('mousemove', move);
			window.addEventListener('mouseup', up);
		});
		/* 枠の右・下・右下を引っぱって大きさを変える（WS_THICKFRAME の小窓だけ） */
		if (style & 0x00040000) {
			['r', 'b', 'rb'].forEach(function (dir) {
				var grip = document.createElement('div');
				grip.className = 'w32f-grip w32-' + dir;
				grip.addEventListener('pointerdown', function (ev) {
					if (ev.button !== 0) {
						return;
					}
					ev.preventDefault();
					ev.stopPropagation();
					P.front(frame);
					var sx = ev.clientX, sy = ev.clientY;
					var ow = frame.offsetWidth, oh = frame.offsetHeight;
					/* 最初に出した大きさより小さくはしない（中身が作られた時の大きさなので） */
					var minW = client.w32MinW || 160, minH = client.w32MinH || 100;
					var cur = {w: ow, h: oh}, sent = {w: ow, h: oh}, timer = 0;
					var flush = function () {
						timer = 0;
						if (cur.w !== sent.w || cur.h !== sent.h) {
							sent.w = cur.w;
							sent.h = cur.h;
							_W32JsOnFrameSize(id, cur.w, cur.h);
						}
					};
					var move = function (e) {
						if (dir !== 'b') {
							cur.w = Math.max(minW, Math.round(ow + e.clientX - sx));
						}
						if (dir !== 'r') {
							cur.h = Math.max(minH, Math.round(oh + e.clientY - sy));
						}
						var menuH = (client.w32Menu.style.display === 'none') ? 0 : 19;
						frame.style.width = cur.w + 'px';
						frame.style.height = cur.h + 'px';
						client.style.width = Math.max(0, cur.w - 6) + 'px';
						client.style.height = Math.max(0, cur.h - 6 - 19 - menuH) + 'px';
						/* 中身の並べ直しは C 側でするので、送るのは間引く */
						if (!timer) {
							timer = window.requestAnimationFrame(flush);
						}
					};
					var up = function () {
						grip.removeEventListener('pointermove', move);
						grip.removeEventListener('pointerup', up);
						grip.removeEventListener('pointercancel', up);
						if (timer) {
							window.cancelAnimationFrame(timer);
						}
						flush();
					};
					grip.setPointerCapture(ev.pointerId);
					grip.addEventListener('pointermove', move);
					grip.addEventListener('pointerup', up);
					grip.addEventListener('pointercancel', up);
				});
				frame.appendChild(grip);
			});
		}
		return client;
	};

	/* ---- スクロールバー：矢印・溝・つまみを div で描く（OS のスクロールバーは隠れることがあるので） ---- */
	P.sbRange = function (el) {
		var sb = el.w32Sb;
		/* つまみが動ける範囲（ページ指定があれば max - page + 1 まで） */
		var top = sb.max - (sb.page > 0 ? sb.page - 1 : 0);
		return {min: sb.min, max: Math.max(sb.min, top)};
	};
	P.sbLayout = function (el) {
		var sb = el.w32Sb;
		var r = P.sbRange(el);
		var track = el.w32Track, thumb = el.w32Thumb;
		/* まだ DOM に載っていない時もあるので、溝の長さはコントロールの大きさから出す */
		var whole = el.w32Vert ? (el.w32H || el.offsetHeight) : (el.w32W || el.offsetWidth);
		var len = Math.max(0, whole - 34);
		var span = sb.max - sb.min + 1;
		var size = (sb.page > 0 && span > 0) ? Math.round(len * Math.min(1, sb.page / span)) : 17;
		size = Math.max(8, Math.min(len, size));
		var pos = (el.w32Drag !== undefined) ? el.w32Drag : sb.pos;
		pos = Math.max(r.min, Math.min(pos, r.max));
		var off = (r.max > r.min) ? Math.round((len - size) * (pos - r.min) / (r.max - r.min)) : 0;
		thumb.style.display = (r.max > r.min && len > 0) ? '' : 'none';
		if (el.w32Vert) {
			thumb.style.top = off + 'px';
			thumb.style.height = size + 'px';
		} else {
			thumb.style.left = off + 'px';
			thumb.style.width = size + 'px';
		}
		el.w32ThumbOff = off;
		el.w32ThumbSize = size;
		el.w32TrackLen = len;
	};
	P.makeScroll = function (el, id, vert) {
		el.className = 'w32c w32-sb ' + (vert ? 'w32-v' : 'w32-h');
		var a = document.createElement('div');
		var track = document.createElement('div');
		var thumb = document.createElement('div');
		var b = document.createElement('div');
		a.className = b.className = 'w32-sbb';
		a.textContent = vert ? '▲' : '◀';
		b.textContent = vert ? '▼' : '▶';
		track.className = 'w32-sbt';
		thumb.className = 'w32-sbth';
		track.appendChild(thumb);
		el.appendChild(a);
		el.appendChild(track);
		el.appendChild(b);
		el.w32Vert = vert;
		el.w32Track = track;
		el.w32Thumb = thumb;
		el.w32Sb = {min: 0, max: 100, pos: 0, page: 0};
		/* 押している間は繰り返し送る（Windows と同じく最初は少し待つ） */
		var repeat = function (ev, fn) {
			if (ev.button !== 0) {
				return;
			}
			ev.preventDefault();
			ev.stopPropagation();
			if (fn() === false) {
				return;
			}
			var t = 0, iv = 0;
			t = setTimeout(function () {
				iv = setInterval(function () {
					if (fn() === false) {
						clearInterval(iv);
					}
				}, 50);
			}, 350);
			var up = function () {
				clearTimeout(t);
				clearInterval(iv);
				window.removeEventListener('mouseup', up);
				_W32JsOnScroll(id, 8, el.w32Sb.pos);		/* SB_ENDSCROLL */
			};
			window.addEventListener('mouseup', up);
		};
		a.addEventListener('mousedown', function (ev) {
			repeat(ev, function () { _W32JsOnScroll(id, 0, el.w32Sb.pos); });		/* SB_LINEUP */
		});
		b.addEventListener('mousedown', function (ev) {
			repeat(ev, function () { _W32JsOnScroll(id, 1, el.w32Sb.pos); });		/* SB_LINEDOWN */
		});
		track.addEventListener('mousedown', function (ev) {
			if (ev.target !== track) {
				return;
			}
			var at = function () {
				var rc = track.getBoundingClientRect();
				var s = el.w32TrackLen ? rc[vert ? 'height' : 'width'] / el.w32TrackLen : 1;
				return ((vert ? ev.clientY - rc.top : ev.clientX - rc.left) / (s || 1));
			};
			repeat(ev, function () {
				var p = at();
				if (p < el.w32ThumbOff) {
					_W32JsOnScroll(id, 2, el.w32Sb.pos);		/* SB_PAGEUP */
				} else if (p > el.w32ThumbOff + el.w32ThumbSize) {
					_W32JsOnScroll(id, 3, el.w32Sb.pos);		/* SB_PAGEDOWN */
				} else {
					return false;
				}
			});
		});
		thumb.addEventListener('mousedown', function (ev) {
			if (ev.button !== 0) {
				return;
			}
			ev.preventDefault();
			ev.stopPropagation();
			var rc = track.getBoundingClientRect();
			var s = el.w32TrackLen ? rc[vert ? 'height' : 'width'] / el.w32TrackLen : 1;
			s = s || 1;
			var start = vert ? ev.clientY : ev.clientX;
			var off0 = el.w32ThumbOff;
			var r = P.sbRange(el);
			var last = el.w32Sb.pos;
			el.w32Drag = last;
			thumb.classList.add('w32-drag');
			var move = function (e) {
				var room = el.w32TrackLen - el.w32ThumbSize;
				var off = off0 + ((vert ? e.clientY : e.clientX) - start) / s;
				var v = (room > 0) ? r.min + Math.round((r.max - r.min) * off / room) : r.min;
				v = Math.max(r.min, Math.min(v, r.max));
				el.w32Drag = v;
				P.sbLayout(el);
				if (v !== last) {
					last = v;
					_W32JsOnScroll(id, 5, v);		/* SB_THUMBTRACK */
				}
			};
			var up = function () {
				window.removeEventListener('mousemove', move);
				window.removeEventListener('mouseup', up);
				thumb.classList.remove('w32-drag');
				el.w32Drag = undefined;
				_W32JsOnScroll(id, 4, last);		/* SB_THUMBPOSITION */
				_W32JsOnScroll(id, 8, last);		/* SB_ENDSCROLL */
				P.sbLayout(el);
			};
			window.addEventListener('mousemove', move);
			window.addEventListener('mouseup', up);
		});
		el.addEventListener('wheel', function (ev) {
			ev.preventDefault();
			_W32JsOnScroll(id, (ev.deltaY || ev.deltaX) < 0 ? 0 : 1, el.w32Sb.pos);
		}, {passive: false});
	};

	/* ---- リストビュー ---- */
	P.makeListView = function (el, id) {
		var table = document.createElement('table');
		var colgroup = document.createElement('colgroup');
		var thead = document.createElement('thead');
		var hrow = document.createElement('tr');
		var tbody = document.createElement('tbody');
		thead.appendChild(hrow);
		table.appendChild(colgroup);
		table.appendChild(thead);
		table.appendChild(tbody);
		el.appendChild(table);
		el.tabIndex = 0;
		el.w32Lv = {table: table, colgroup: colgroup, hrow: hrow, tbody: tbody, widths: []};
		var rowOf = function (ev) {
			var tr = ev.target.closest('tr');
			return (tr && tr.parentNode === tbody) ? tr.sectionRowIndex : -1;
		};
		tbody.addEventListener('click', function (ev) {
			if (ev.target.tagName === 'INPUT') {
				ev.preventDefault();
				_W32JsOnListView(id, rowOf(ev), 1);
				return;
			}
			/* Ctrl（Mac は command）で追加・外す、Shift で範囲選択（一つしか選べない一覧では普通のクリック） */
			var mod = ((ev.ctrlKey || ev.metaKey) ? 0x10 : 0) | (ev.shiftKey ? 0x20 : 0);
			_W32JsOnListView(id, rowOf(ev), mod);
		});
		el.addEventListener('mousedown', function (ev) {
			/* スクロールバーをつかんだ時は選択を外さない */
			if (ev.target === el && (ev.offsetX >= el.clientWidth || ev.offsetY >= el.clientHeight)) {
				return;
			}
			if (ev.target === el || ev.target === table) {
				_W32JsOnListView(id, -1, 0);
			}
		});
		tbody.addEventListener('dblclick', function (ev) {
			if (ev.target.tagName !== 'INPUT') {
				_W32JsOnListView(id, rowOf(ev), 2);
			}
		});
		el.addEventListener('contextmenu', function (ev) {
			ev.preventDefault();
			_W32JsOnListView(id, rowOf(ev), 3);
		});
		el.addEventListener('keydown', function (ev) {
			ev.stopPropagation();
			var lv = el.w32Lv;
			var cur = -1;
			for (var i = 0; i < lv.tbody.rows.length; i ++) {
				if (lv.tbody.rows[i].classList.contains('w32-sel')) {
					cur = i;
					break;
				}
			}
			if (ev.key === 'ArrowDown' && cur + 1 < lv.tbody.rows.length) {
				ev.preventDefault();
				_W32JsOnListView(id, cur + 1, 0);
			} else if (ev.key === 'ArrowUp' && cur > 0) {
				ev.preventDefault();
				_W32JsOnListView(id, cur - 1, 0);
			} else if (ev.key === ' ' && cur >= 0) {
				ev.preventDefault();
				_W32JsOnListView(id, cur, 1);
			}
		});
	};
	P.lvWidth = function (lv) {
		var w = 0;
		lv.widths.forEach(function (v) {
			w += v;
		});
		lv.table.style.width = w + 'px';
	};
	P.lvMakeRow = function (lv) {
		var tr = document.createElement('tr');
		var n = Math.max(1, lv.widths.length);
		for (var i = 0; i < n; i ++) {
			var td = document.createElement('td');
			if (i === 0) {
				var cb = document.createElement('input');
				cb.type = 'checkbox';
				cb.tabIndex = -1;
				td.appendChild(cb);
				td.appendChild(document.createElement('span'));
			}
			tr.appendChild(td);
		}
		return tr;
	};
});

/* 部品を作る */
EM_JS(void, W32JsPopCreate, (int nID, int nParentID, int nType, int nStyle, int nExStyle, int bDialog), {
	var P = Module['w32Pop'];
	var parent = nParentID ? Module['w32Html'][nParentID] : null;
	var el;
	var sub = nStyle & 0x0F;
	if (nType === 11) {
		el = P.makeFrame(nID, nStyle);
		el.w32Frame.w32Owner = parent ? parent.w32Frame : null;
		el.w32Frame.w32Client = el;
		if (bDialog) {
			el.classList.add('w32-dlg');
		}
		P.attachMouse(el);
	} else if (nType === 0) {
		el = document.createElement('div');
		el.className = 'w32c w32-wnd' + (bDialog ? ' w32-dlg' : '');
		P.attachMouse(el);
	} else if (nType === 1) {
		var multi = (nStyle & 0x0004) !== 0;		/* ES_MULTILINE */
		el = document.createElement(multi ? 'textarea' : 'input');
		if (!multi) {
			el.type = (nStyle & 0x0020) ? 'password' : 'text';
		}
		el.className = 'w32c w32-edit';
		el.spellcheck = false;
		el.autocomplete = 'off';
		if (!(nStyle & 0x00800000) && !(nExStyle & 0x00000200)) {	/* WS_BORDER / WS_EX_CLIENTEDGE */
			el.classList.add('w32-noborder');
		}
		if (nStyle & 0x0800) {			/* ES_READONLY */
			el.readOnly = true;
		}
		if (nStyle & 0x0001) {
			el.style.textAlign = 'center';
		} else if (nStyle & 0x0002) {
			el.style.textAlign = 'right';
		}
		var number = (nStyle & 0x2000) !== 0;		/* ES_NUMBER */
		var lower = (nStyle & 0x0010) !== 0;
		var upper = (nStyle & 0x0008) !== 0;
		/* 数値の欄（ES_NUMBER か、プログラムが数字だけを入れた欄）は日本語入力を使わない欄にする。
		   ブラウザから IME を直接オフにはできないので、inputmode="numeric" で伝える
		   （Windows の Chrome/Edge は MS-IME・Google 日本語入力が半角になり、スマホは数字のキーボードになる。
		   Mac は効かない）。全角で打たれた数字は確定してから半角に直す */
		el.w32SetNum = function (on) {
			if (el.w32Num === on) {
				return;
			}
			el.w32Num = on;
			if (on) {
				el.inputMode = 'numeric';
				el.setAttribute('lang', 'en');
			} else {
				el.removeAttribute('inputmode');
				el.removeAttribute('lang');
			}
		};
		el.w32AutoNum = !number && !multi && el.type === 'text' && !el.readOnly;
		if (number) {
			el.w32SetNum(true);
		}
		var fix = function () {
			var v = el.value;
			if (el.w32Num) {
				/* 全角英数字記号は半角に（EM_JS の中ではユニコードのエスケープが使えないので文字コードで） */
				var t = '';
				for (var i = 0; i < v.length; i++) {
					var n = v.charCodeAt(i);
					if (n >= 0xFF01 && n <= 0xFF5E) {
						n -= 0xFEE0;
					} else if (n === 0x3000) {
						n = 0x20;
					} else if (n === 0x30FC || n === 0x2212) {	/* ー − は - に */
						n = 0x2D;
					}
					t += String.fromCharCode(n);
				}
				v = t;
			}
			if (number) {
				v = v.replace(/[^0-9]/g, '');
			}
			if (lower) {
				v = v.toLowerCase();
			}
			if (upper) {
				v = v.toUpperCase();
			}
			if (v === el.value) {
				return false;
			}
			var nCut = el.value.length - v.length;
			var s = Math.max(0, (el.selectionStart || 0) - nCut);
			el.value = v;
			if (document.activeElement === el) {
				el.setSelectionRange(s, s);
			}
			return true;
		};
		el.addEventListener('input', function (ev) {
			/* 日本語入力で変換中に書き換えると変換が壊れるので、確定してから直す */
			if (!ev.isComposing) {
				fix();
			}
			_W32JsOnEditChange(nID);
		});
		var fixLater = function () {
			if (fix()) {
				_W32JsOnEditChange(nID);
			}
		};
		el.addEventListener('compositionend', function () {
			/* compositionend の直後に確定の input が来るブラウザもあるので、少し後でも直す */
			fixLater();
			setTimeout(fixLater, 0);
		});
		/* 数値の欄で数字を変換中にしておくと、Mac の Chrome では変換候補が出て、
		   次の欄をクリックしても 1 回目は確定に使われてしまう。
		   変換中の文字が数字と - だけなら、その場で確定させて（いったんフォーカスを外して戻す）半角に直す */
		var isNumText = function (s) {
			if (!s) {
				return false;
			}
			for (var i = 0; i < s.length; i++) {
				var n = s.charCodeAt(i);
				if (!((n >= 0x30 && n <= 0x39) || (n >= 0xFF10 && n <= 0xFF19) ||
						n === 0x2D || n === 0xFF0D || n === 0x30FC || n === 0x2212)) {
					return false;
				}
			}
			return true;
		};
		var commitTimer = 0;
		el.addEventListener('compositionupdate', function (ev) {
			if (!el.w32Num || !isNumText(ev.data) || commitTimer) {
				return;
			}
			commitTimer = setTimeout(function () {
				commitTimer = 0;
				if (document.activeElement !== el) {
					return;
				}
				el.blur();
				el.focus();
				fixLater();
			}, 0);
		});
		P.attachKeys(el, nID);
	} else if (nType === 2) {
		el = document.createElement('button');
		el.type = 'button';
		el.className = 'w32c w32-btn' + (sub === 1 ? ' w32-def' : '');
		el.addEventListener('click', function () {
			_W32JsOnButton(nID);
		});
		el.addEventListener('keydown', function (ev) {
			ev.stopPropagation();
		});
	} else if (nType === 3 || nType === 4) {
		el = document.createElement('label');
		el.className = 'w32c w32-check' + ((nStyle & 0x0020) ? ' w32-left' : '');
		var box = document.createElement('input');
		box.type = (nType === 3) ? 'checkbox' : 'radio';
		var span = document.createElement('span');
		el.appendChild(box);
		el.appendChild(span);
		el.w32Box = box;
		el.w32Label = span;
		el.w32Focus = box;
		box.addEventListener('click', function () {
			_W32JsOnButton(nID);
		});
		box.addEventListener('keydown', function (ev) {
			ev.stopPropagation();
		});
	} else if (nType === 5) {
		el = document.createElement('div');
		el.className = 'w32c w32-group';
		el.w32Label = document.createElement('span');
		el.appendChild(el.w32Label);
	} else if (nType === 6) {
		el = document.createElement('div');
		el.className = 'w32c w32-static';
		var ss = nStyle & 0x1F;
		if (ss === 1) {
			el.style.textAlign = 'center';
		} else if (ss === 2) {
			el.style.textAlign = 'right';
		} else if (ss === 4 || ss === 7) {
			el.classList.add('w32-frame-black');
		} else if (ss === 5 || ss === 8) {
			el.classList.add('w32-frame-gray');
		} else if (ss === 0x12 || ss === 0x10 || ss === 0x11) {
			el.classList.add('w32-frame-etched');
		}
		if (ss === 4) {
			el.style.background = '#000';
		}
		if (nStyle & 0x1000) {			/* SS_SUNKEN */
			el.classList.add('w32-frame-gray');
		}
		if (nStyle & 0x0200) {			/* SS_CENTERIMAGE：縦も真ん中 */
			el.style.display = 'flex';
			el.style.alignItems = 'center';
		}
		el.w32NoPrefix = (nStyle & 0x0080) !== 0;
		if (nStyle & 0x0100) {			/* SS_NOTIFY */
			el.classList.add('w32-notify');
			P.attachMouse(el);
		}
	} else if (nType === 7 || nType === 8) {
		el = document.createElement('select');
		el.className = 'w32c ' + (nType === 7 ? 'w32-combo' : 'w32-list');
		if (nType === 8) {
			el.size = 2;
		}
		el.w32Combo = (nType === 7);
		el.addEventListener('change', function () {
			_W32JsOnSelect(nID, el.selectedIndex, 0);
		});
		if (nType === 8) {
			el.addEventListener('dblclick', function () {
				_W32JsOnSelect(nID, el.selectedIndex, 1);
			});
		}
		el.addEventListener('keydown', function (ev) {
			ev.stopPropagation();
		});
	} else if (nType === 9) {
		el = document.createElement('div');
		P.makeScroll(el, nID, (nStyle & 1) !== 0);
	} else if (nType === 10) {
		el = document.createElement('div');
		el.className = 'w32c w32-lv';
		if (!(nStyle & 0x00800000) && !(nExStyle & 0x00000200)) {
			el.style.borderColor = 'transparent';
		}
		P.makeListView(el, nID);
	} else {
		el = document.createElement('div');
		el.className = 'w32c';
	}
	el.w32Id = nID;
	el.w32Type = nType;
	el.dataset.w32Id = String(nID);
	if (nType !== 11) {
		el.style.display = 'none';
		(parent || P.layer).appendChild(el);
	}
	Module['w32Html'][nID] = el;
});

/* 位置・表示・有効・チェック（小窓の枠は nScrCx に画面の幅を渡す） */
EM_JS(void, W32JsPopPos, (int nID, int x, int y, int cx, int cy, int bVisible, int bEnabled, int nCheck, int nScrCx), {
	var P = Module['w32Pop'];
	var el = Module['w32Html'][nID];
	if (!el) {
		return;
	}
	if (el.w32Frame) {
		var frame = el.w32Frame;
		var info = P.screenInfo(nScrCx);
		el.w32ScrCx = nScrCx;
		if (el.w32X !== x || el.w32Y !== y || !el.w32Placed) {
			el.w32X = x;
			el.w32Y = y;
			frame.style.left = Math.round(info.x + x * info.s) + 'px';
			frame.style.top = Math.round(info.y + y * info.s) + 'px';
			el.w32Placed = false;
		}
		if (!el.w32MinW) {
			el.w32MinW = cx;
			el.w32MinH = cy;
		}
		frame.style.width = cx + 'px';
		frame.style.height = cy + 'px';
		var menuH = (el.w32Menu.style.display === 'none') ? 0 : 19;
		el.style.width = Math.max(0, cx - 6) + 'px';
		el.style.height = Math.max(0, cy - 6 - 19 - menuH) + 'px';
		var wasHidden = frame.style.display === 'none';
		frame.style.display = bVisible ? '' : 'none';
		el.inert = !bEnabled;
		if (bVisible && (wasHidden || !el.w32Placed)) {
			/* 初めて出す時は、ページの見えている所に収める */
			var vw = document.documentElement.clientWidth, vh = document.documentElement.clientHeight;
			var left = frame.offsetLeft - window.scrollX, top = frame.offsetTop - window.scrollY;
			left = Math.max(0, Math.min(left, vw - cx));
			top = Math.max(0, Math.min(top, vh - cy));
			frame.style.left = (left + window.scrollX) + 'px';
			frame.style.top = (top + window.scrollY) + 'px';
			el.w32Placed = true;
			if (wasHidden) {
				P.front(frame);
			}
		}
		return;
	}
	el.style.left = x + 'px';
	el.style.top = y + 'px';
	el.style.width = cx + 'px';
	/* コンボボックスの高さはリストを開いた時の大きさなので、閉じた時の高さにする */
	el.style.height = (el.w32Combo ? Math.min(cy, 21) : cy) + 'px';
	el.style.display = bVisible ? '' : 'none';
	if (el.w32Box) {
		el.w32Box.disabled = !bEnabled;
		el.w32Box.checked = !!nCheck;
		el.style.color = bEnabled ? '' : '#9a9a9a';
	} else if (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA' || el.tagName === 'SELECT' || el.tagName === 'BUTTON') {
		el.disabled = !bEnabled;
	} else if (el.w32Type === 0 || el.w32Type === 10 || el.w32Type === 9) {
		el.inert = !bEnabled;
	} else if (el.w32Type === 6) {
		el.style.color = bEnabled ? '' : '#9a9a9a';
	}
	el.w32W = cx;
	el.w32H = cy;
	if (el.w32Sb) {
		Module['w32Pop'].sbLayout(el);
	}
});

/* 文字 */
EM_JS(void, W32JsPopText, (int nID, const char *pszUtf8), {
	var P = Module['w32Pop'];
	var el = Module['w32Html'][nID];
	if (!el) {
		return;
	}
	var text = UTF8ToString(pszUtf8);
	if (el.w32Title) {
		el.w32Title.textContent = text;
	} else if (el.w32Label) {
		el.w32Label.textContent = P.strip(text);
	} else if (el.tagName === 'BUTTON') {
		el.textContent = P.strip(text);
	} else if (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA') {
		var v = text.split('\r\n').join('\n');
		/* 管理者ウィンドウの数値の欄は ES_NUMBER が付いていないものが多いので、
		   プログラムが入れた中身が数字だけなら数値の欄とみなす（空なら決めない） */
		if (el.w32AutoNum && v !== '') {
			el.w32SetNum(/^-?[0-9]+$/.test(v));
		}
		if (el.value !== v) {
			el.value = v;
		}
	} else if (el.w32Type === 6) {
		/* スタティックは GDI で描いた canvas を消さないよう、文字用の span に入れる */
		if (!el.w32Text) {
			el.w32Text = document.createElement('span');
			el.appendChild(el.w32Text);
		}
		el.w32Text.textContent = el.w32NoPrefix ? text : P.strip(text);
	}
});

/* 入力欄の中身（呼んだ側で free する。入力欄でなければ 0） */
EM_JS(char *, W32JsPopGetText, (int nID), {
	var el = Module['w32Html'][nID];
	if (!el || (el.tagName !== 'INPUT' && el.tagName !== 'TEXTAREA')) {
		return 0;
	}
	var v = el.value;
	if (el.tagName === 'TEXTAREA') {
		v = v.split('\r\n').join('\n').split('\n').join('\r\n');
	}
	return stringToNewUTF8(v);
});

/* コンボボックス・リストボックスの項目 nOp 0:全消し 1:挿入 2:削除 3:選択 */
EM_JS(void, W32JsPopItems, (int nID, int nOp, int nIndex, const char *pszUtf8), {
	var el = Module['w32Html'][nID];
	if (!el || el.tagName !== 'SELECT') {
		return;
	}
	if (nOp === 0) {
		el.length = 0;
	} else if (nOp === 1) {
		var opt = document.createElement('option');
		opt.textContent = UTF8ToString(pszUtf8);
		if (nIndex < 0 || nIndex >= el.options.length) {
			el.appendChild(opt);
		} else {
			el.insertBefore(opt, el.options[nIndex]);
		}
		/* 選択は C 側の値に合わせる（追加しただけで先頭が選ばれないように） */
		if (el.w32Sel === undefined || el.w32Sel < 0) {
			el.selectedIndex = -1;
		} else if (nIndex >= 0 && nIndex <= el.w32Sel) {
			el.w32Sel ++;
			el.selectedIndex = el.w32Sel;
		} else {
			el.selectedIndex = el.w32Sel;
		}
	} else if (nOp === 2) {
		if (nIndex >= 0 && nIndex < el.options.length) {
			el.remove(nIndex);
		}
	} else if (nOp === 3) {
		el.w32Sel = nIndex;
		el.selectedIndex = nIndex;
	}
});

/* スクロールバー */
EM_JS(void, W32JsPopScroll, (int nID, int nMin, int nMax, int nPos, int nPage), {
	var el = Module['w32Html'][nID];
	if (!el || !el.w32Sb) {
		return;
	}
	el.w32Sb = {min: nMin, max: nMax, pos: nPos, page: nPage};
	Module['w32Pop'].sbLayout(el);
});

/* リストビュー */
EM_JS(void, W32JsPopListView, (int nID, int nOp, int nRow, int nCol, const char *pszUtf8, int nValue), {
	var P = Module['w32Pop'];
	var el = Module['w32Html'][nID];
	if (!el || !el.w32Lv) {
		return;
	}
	var lv = el.w32Lv;
	var text = UTF8ToString(pszUtf8);
	var rows = lv.tbody.rows;
	switch (nOp) {
	case 0:
		lv.tbody.textContent = '';
		break;
	case 1:
		if (nRow === -1) {
			/* 幅だけ変える */
			if (nCol < lv.widths.length) {
				lv.widths[nCol] = nValue;
				lv.colgroup.children[nCol].style.width = nValue + 'px';
				P.lvWidth(lv);
			}
			break;
		}
		var col = document.createElement('col');
		col.style.width = nValue + 'px';
		var th = document.createElement('th');
		th.textContent = text;
		var first = lv.widths.length === 0;
		lv.colgroup.insertBefore(col, lv.colgroup.children[nCol] || null);
		lv.hrow.insertBefore(th, lv.hrow.children[nCol] || null);
		lv.widths.splice(nCol, 0, nValue);
		/* 既にある行にも列を足す（最初の列は行を作った時にあるのでそのまま） */
		if (!first) {
			for (var i = 0; i < rows.length; i ++) {
				var td = document.createElement('td');
				rows[i].insertBefore(td, rows[i].children[nCol] || null);
			}
		}
		P.lvWidth(lv);
		break;
	case 2:
		var tr = P.lvMakeRow(lv);
		lv.tbody.insertBefore(tr, rows[nRow] || null);
		break;
	case 3:
		var r = rows[nRow];
		if (!r) {
			break;
		}
		while (r.cells.length <= nCol) {
			r.appendChild(document.createElement('td'));
		}
		var cell = r.cells[nCol];
		if (nCol === 0) {
			cell.lastChild.textContent = text;
		} else {
			cell.textContent = text;
		}
		cell.title = text;
		break;
	case 4:
		if (rows[nRow]) {
			rows[nRow].remove();
		}
		break;
	case 5:
		if (rows[nRow]) {
			rows[nRow].classList.toggle('w32-sel', (nValue & 1) !== 0);
			var cb = rows[nRow].cells[0].firstChild;
			if (cb && cb.tagName === 'INPUT') {
				cb.checked = (nValue & 2) !== 0;
			}
		}
		break;
	case 6:
		el.classList.toggle('w32-grid', (nValue & 0x01) !== 0);
		el.classList.toggle('w32-cb', (nValue & 0x04) !== 0);
		break;
	case 7:
		if (rows[nRow]) {
			var top = rows[nRow].offsetTop - lv.hrow.offsetHeight;
			if (top < el.scrollTop) {
				el.scrollTop = top;
			} else if (rows[nRow].offsetTop + rows[nRow].offsetHeight > el.scrollTop + el.clientHeight) {
				el.scrollTop = rows[nRow].offsetTop + rows[nRow].offsetHeight - el.clientHeight;
			}
		}
		break;
	}
});

/* GDI で描いた内容（R,G,B,A の並び）を canvas に写す */
EM_JS(void, W32JsPopPaint, (int nID, const unsigned char *pRgba, int cx, int cy), {
	var el = Module['w32Html'][nID];
	if (!el) {
		return;
	}
	var cv = el.w32Canvas;
	if (!cv) {
		cv = document.createElement('canvas');
		cv.className = 'w32-cv';
		el.insertBefore(cv, el.firstChild);
		el.w32Canvas = cv;
	}
	if (cv.width !== cx || cv.height !== cy) {
		cv.width = cx;
		cv.height = cy;
	}
	var ctx = cv.getContext('2d');
	var img = ctx.createImageData(cx, cy);
	img.data.set(HEAPU8.subarray(pRgba, pRgba + cx * cy * 4));
	ctx.putImageData(img, 0, 0);
});

/* メニューバー（JSON：[{t, id, sep, chk, dis, sub:[...]}]） */
EM_JS(void, W32JsPopMenu, (int nID, const char *pszJson), {
	var P = Module['w32Pop'];
	var el = Module['w32Html'][nID];
	if (!el || !el.w32Menu) {
		return;
	}
	var items = JSON.parse(UTF8ToString(pszJson));
	var bar = el.w32Menu;
	if (P.menuOwner === el) {
		P.closeMenu();
	}
	bar.textContent = '';
	items.forEach(function (it) {
		if (it.sep) {
			return;
		}
		var d = document.createElement('div');
		d.textContent = P.strip(it.t);
		if (it.dis) {
			d.style.color = '#9a9a9a';
		}
		d.addEventListener('mousedown', function (ev) {
			ev.preventDefault();
			if (P.menuItem === d) {
				P.closeMenu();
				return;
			}
			if (it.sub) {
				P.openMenu(el, d, it.sub);
			} else if (!it.dis) {
				_W32JsOnMenu(nID, it.id);
			}
		});
		/* 開いている時は、横の項目に乗せるだけで切り替える */
		d.addEventListener('mouseenter', function () {
			if (P.menuOwner === el && P.menuItem !== d && it.sub) {
				P.openMenu(el, d, it.sub);
			}
		});
		bar.appendChild(d);
	});
	var show = items.length > 0;
	var changed = (bar.style.display === 'none') === show;
	bar.style.display = show ? '' : 'none';
	if (changed && el.w32Frame.style.height) {
		var h = parseInt(el.w32Frame.style.height, 10);
		el.style.height = Math.max(0, h - 6 - 19 - (show ? 19 : 0)) + 'px';
	}
});
