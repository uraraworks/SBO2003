/* ========================================================================= */
/* sbo_bgm.js																 */
/* BGM（MIDI）をブラウザで鳴らす（Web版）									 */
/* ========================================================================= */
/*
 * Windows 版は LibMusic.dll で MIDI を鳴らしていたが、ブラウザでは使えないので
 * spessasynth_lib（JS の MIDI シンセ）とサウンドフォント（GeneralUser GS）で鳴らす。
 *
 * ゲーム側（LibMusicLoader_web.cpp）からは window.SboBgm の関数を呼ぶ。
 * シンセやサウンドフォントの読み込みは時間がかかるので、ゲーム側からの指示は
 * 「今どうなっていてほしいか」として覚えておき、準備ができたらそれに合わせる。
 *
 * ブラウザは操作されるまで音を出させてくれないので、キー入力・クリック・タップの
 * たびに（動いていなければ）AudioContext を動かし直す。
 * resume() は操作のイベントの中で「すぐに」呼ばないと Safari などでは断られるので、
 * シンセの読み込みを待たずに呼ぶ（読み込みを待ってから呼ぶと、許可されないまま
 * 読み込みも進まず、なかなか鳴らなかった）。
 *
 * 読み込みはページを開いたらすぐに始めて、ログイン画面までに間に合うようにする。
 *
 * 音声ファイル(.m4a)の BGM
 * MIDI のほかに、音声ファイル（bgm/audio/ の .m4a）で鳴る曲もある（openAudio）。
 * HTMLAudioElement を MediaElementAudioSourceNode で MIDI と同じゲイン（音量）につなぐ
 * （iPhone は audio.volume が効かないので、音量はゲインで掛ける）。落としながら鳴らせるよう
 * preload で読み込み、一度落とした曲はブラウザの HTTP キャッシュ（版付き URL）にまかせる。
 * ループは「曲の最後まで鳴ったら 2.5 秒の間を空けて頭から」。
 * MIDI と音声は同時には鳴らさず、切り替える時にもう片方を必ず止める。
 * シンセの読み込みが終わっていなくても音声は鳴らせる。
 */
(function () {
	'use strict';

	/* このスクリプトと同じ場所にシンセとサウンドフォントを置いている */
	var BASE = (document.currentScript && document.currentScript.src) || location.href;
	function url(name) {
		/* サーバーがページに書き込んだ版があれば付ける（版付きの URL はブラウザが長く覚える） */
		var ver = window.SBO_ASSET_VER && window.SBO_ASSET_VER['bgm/' + name];
		return new URL(name, BASE).href + (ver ? '?v=' + ver : '');
	}

	var MASTER_GAIN = 3.0;		/* MIDI の音量（GeneralUser GS は小さめなので、効果音とつり合うように持ち上げる） */
	var AUDIO_GAIN = 1.5;		/* 音声ファイルの音量（MIDI とつり合うように調整する。Suno の曲は山が -3.4dB 前後なので、上げすぎると最大音量で割れる） */
	var AUDIO_LOOP_GAP = 2500;	/* 音声 BGM をループする時、曲の最後から頭に戻るまでの間(ms) */

	/* ゲーム側から見た状態 */
	var want = {
		kind: 'midi',		/* 今の曲の種類（'midi' か 'audio'） */
		data: null,			/* 開いている MIDI（ArrayBuffer） */
		serial: 0,			/* 開き直すたびに増やす */
		playing: false,
		loop: false,
		volume: 1.0
	};

	/* シンセ側の状態 */
	var ctx = null;
	var gain = null;
	var synth = null;
	var seq = null;
	var loadedSerial = -1;
	var midiActive = false;		/* seq.play() を呼んで、まだ止めていない（seq.paused はあてにならないので自分で覚える） */
	var ready = false;
	var failed = false;
	var starting = null;

	/* 音声ファイル側の状態 */
	var audio = null;			/* 開いている音声の audio 要素 */
	var audioSrc = null;		/* その MediaElementAudioSourceNode */
	var audioName = null;
	var audioFailed = false;	/* 読み込み・再生できなかった */
	var audioSerial = -1;		/* audio 要素を頭から鳴らし始めた時の want.serial */
	var loopTimer = 0;			/* 次のループまでの間のタイマー */

	function log(msg) {
		console.log('[bgm] ' + msg);
	}

	/*
	 * ループを「曲の頭 → 曲の終わり」にそろえた MIDI を作る
	 *
	 * ゲームの MIDI の多くには RPG ツクール式のループの印（CC#111）が入っている。
	 * spessasynth はこれを見つけると 2 周目からは印の位置に戻るので、前奏（曲の頭の
	 * 数小節）が 1 周目にしか鳴らなかった。Windows 版（AflMusic）のように、毎回曲の
	 * 頭から鳴らし直したいので、
	 *   1. ループの印に使われるコントロールチェンジ（CC#111・116〜119、spessasynth は
	 *      CC#2 の 0 と CC#4 も印と見る）を、何も起きない番号（CC#102）に書き換えて、
	 *      印として見られないようにする（CC#2・#4 はサウンドフォントでは音に効かない）
	 *   2. 最初のトラックの頭に「loopStart」、全トラックで一番遅い終わり（End of Track）
	 *      の位置に「loopEnd」のマーカーを足す
	 * spessasynth は、何も無いと最後の音（ノートオフ）でループするので、曲の終わりの
	 * 休みが切れて 2 周目が少し早く入ってしまう。2. でその休みも残す。
	 * ただし最後の音から End of Track まで 4 拍より長く空いている曲（短いジングルの
	 * 後ろに長い無音が付いているものなど）は、いままで通り最後の音でループする。
	 * 読めない形のデータだった時は、そのまま返す（いままで通り spessasynth にまかせる）。
	 */
	var LOOP_CC = { 111: 1, 116: 1, 117: 1, 118: 1, 119: 1 };
	var DUMMY_CC = 102;
	function prepareLoop(src) {
		try {
			return rewriteLoop(new Uint8Array(src));
		} catch (e) {
			log('ループの準備ができなかったので、そのまま鳴らします: ' + (e && e.message ? e.message : e));
			return src;
		}
	}
	function rewriteLoop(d) {
		function u32(p) {
			if (p + 4 > d.length) {
				throw new Error('データが途中で切れています');
			}
			return ((d[p] << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]) >>> 0;
		}
		function tag(p) {
			return String.fromCharCode(d[p], d[p + 1], d[p + 2], d[p + 3]);
		}
		function vlq(p) {
			var v = 0, b;
			do {
				if (p >= d.length) {
					throw new Error('データが途中で切れています');
				}
				b = d[p++];
				v = (v * 128) + (b & 0x7f);
			} while (b & 0x80);
			return { v: v, p: p };
		}
		if (tag(0) !== 'MThd') {
			throw new Error('MIDI ではありません');
		}
		var out = d.slice();		/* CC の書き換えはこの写しに入れる */
		var division = (d[12] << 8) | d[13];	/* 4 分音符の長さ（tick）。最上位ビットが立っていたら SMPTE 形式 */
		var lastVoice = 0;		/* 最後のチャンネルイベント（音・CC など）の時刻 */
		var pos = 8 + u32(4);
		var tracks = [];		/* { start, end, eotPos（EOT の delta の位置）, lastTick（EOT の前のイベントの時刻）, eotTick } */
		while (pos + 8 <= d.length) {
			var len = u32(pos + 4);
			var start = pos + 8, end = start + len;
			if (end > d.length) {
				throw new Error('トラックの長さがおかしい');
			}
			if (tag(pos) === 'MTrk') {
				var p = start, tick = 0, run = 0, lastTick = 0, eotPos = -1, eotTick = 0;
				while (p < end) {
					var evPos = p;
					var r = vlq(p);
					p = r.p;
					tick += r.v;
					var st = d[p];
					if (st >= 0x80) {
						p++;
					} else {
						st = run;
						if (!st) {
							throw new Error('ランニングステータスの前に状態がありません');
						}
					}
					if (st === 0xff) {
						var type = d[p++];
						r = vlq(p);
						p = r.p + r.v;
						if (type === 0x2f) {
							eotPos = evPos;
							eotTick = tick;
							break;
						}
					} else if (st === 0xf0 || st === 0xf7) {
						r = vlq(p);
						p = r.p + r.v;
					} else {
						run = st;
						lastVoice = Math.max(lastVoice, tick);
						var hi = st & 0xf0;
						if (hi === 0xb0 && (LOOP_CC[d[p]] || d[p] === 4 || (d[p] === 2 && d[p + 1] === 0))) {
							out[p] = DUMMY_CC;
						}
						p += (hi === 0xc0 || hi === 0xd0) ? 1 : 2;
					}
					lastTick = tick;
				}
				tracks.push({ start: start, end: end, eotPos: eotPos, lastTick: lastTick, eotTick: eotPos >= 0 ? eotTick : tick });
			}
			pos = end;
		}
		if (tracks.length === 0) {
			throw new Error('トラックがありません');
		}
		var endTick = 0;
		tracks.forEach(function (t) {
			endTick = Math.max(endTick, t.eotTick);
		});
		/* 終わりの休みが長すぎる曲は loopEnd を足さない（spessasynth が最後の音でループする） */
		var addEnd = !(division & 0x8000) && endTick - lastVoice <= division * 4;

		/* 最初のトラックを作り直す：頭に loopStart、終わりに loopEnd と EOT */
		var t0 = tracks[0];
		function vlqBytes(v) {
			var a = [v & 0x7f];
			while ((v = Math.floor(v / 128)) > 0) {
				a.unshift((v & 0x7f) | 0x80);
			}
			return a;
		}
		function marker(delta, text) {
			var a = vlqBytes(delta).concat([0xff, 0x06]).concat(vlqBytes(text.length));
			for (var i = 0; i < text.length; i++) {
				a.push(text.charCodeAt(i));
			}
			return a;
		}
		var head = marker(0, 'loopStart');
		var bodyEnd = t0.end;
		var tail = [];
		if (addEnd) {
			bodyEnd = t0.eotPos >= 0 ? t0.eotPos : t0.end;
			tail = marker(endTick - t0.lastTick, 'loopEnd').concat([0x00, 0xff, 0x2f, 0x00]);
		}
		var body = out.subarray(t0.start, bodyEnd);
		var newLen = head.length + body.length + tail.length;

		var result = new Uint8Array(out.length - (t0.end - t0.start) + newLen);
		var w = 0;
		result.set(out.subarray(0, t0.start - 4), 0);
		w = t0.start - 4;
		result[w++] = (newLen >>> 24) & 0xff;
		result[w++] = (newLen >>> 16) & 0xff;
		result[w++] = (newLen >>> 8) & 0xff;
		result[w++] = newLen & 0xff;
		result.set(head, w);
		w += head.length;
		result.set(body, w);
		w += body.length;
		result.set(tail, w);
		w += tail.length;
		result.set(out.subarray(t0.end), w);
		return result.buffer;
	}

	/* シンセとサウンドフォントを読み込む（1 回だけ） */
	function start() {
		if (starting) {
			return starting;
		}
		var AC = window.AudioContext || window.webkitAudioContext;
		if (!AC) {
			failed = true;
			log('この環境では AudioContext が使えないので BGM は鳴りません');
			return Promise.resolve();
		}
		ctx = new AC();
		ctx.onstatechange = function () {
			log('AudioContext: ' + ctx.state);
		};
		gain = ctx.createGain();
		gain.gain.value = 0;
		gain.connect(ctx.destination);

		starting = Promise.all([
			import(url('spessasynth.js')),
			fetch(url('GeneralUserGS.sf3')).then(function (res) {
				if (!res.ok) {
					throw new Error('サウンドフォントを読み込めませんでした（' + res.status + '）');
				}
				return res.arrayBuffer();
			}),
			ctx.audioWorklet.addModule(url('spessasynth_processor.min.js'))
		]).then(function (r) {
			var lib = r[0];
			synth = new lib.WorkletSynthesizer(ctx);
			synth.connect(gain);
			return synth.soundBankManager.addSoundBank(r[1], 'main').then(function () {
				return synth.isReady;
			}).then(function () {
				seq = new lib.Sequencer(synth);
				ready = true;
				log('準備ができました');
				sync();
			});
		}).catch(function (e) {
			failed = true;
			log('シンセの準備に失敗しました: ' + (e && e.message ? e.message : e));
		});
		return starting;
	}

	/* 音声ファイルを閉じる（再生を止めて、要素とつなぎを捨てる） */
	function clearLoopTimer() {
		if (loopTimer) {
			clearTimeout(loopTimer);
			loopTimer = 0;
		}
	}
	function disposeAudio() {
		clearLoopTimer();
		var a = audio;
		audio = null;
		if (audioSrc) {
			try {
				audioSrc.disconnect();
			} catch (e) {
			}
			audioSrc = null;
		}
		if (a) {
			try {
				a.pause();
				a.removeAttribute('src');
				a.load();		/* 読み込み中の通信もやめる */
			} catch (e) {
			}
		}
		audioName = null;
		audioFailed = false;
		audioSerial = -1;
	}

	/* 音声ファイルを開く（読み込みはここで始めておく。鳴らすのは play() から） */
	function openAudioFile(name) {
		disposeAudio();
		if (!ctx || !gain) {
			audioFailed = true;
			log('AudioContext が無いので音声 BGM は鳴りません: ' + name);
			return;
		}
		var a = new Audio();
		a.preload = 'auto';
		a.loop = false;		/* ループは ended から間を空けて自分でやる */
		a.addEventListener('ended', function () {
			if (audio !== a) {
				return;
			}
			log('音声が最後まで鳴りました: ' + audioName);
			syncAudio();
		});
		a.addEventListener('error', function () {
			if (audio !== a) {
				return;
			}
			audioFailed = true;
			log('音声を読み込めませんでした: ' + audioName + '（' + (a.error ? a.error.code + ' ' + a.error.message : '') + '）');
		});
		audio = a;
		audioName = name;
		audioSrc = ctx.createMediaElementSource(a);
		audioSrc.connect(gain);
		a.src = url('audio/' + name);
		log('音声を開きます: ' + a.src);
	}

	/* play() が断られたら（操作の前など）何もしない。次の操作（unlock）か sync で、止まっていたらやり直す */
	function tryPlay(a) {
		var p;
		try {
			p = a.play();
		} catch (e) {
			return;
		}
		if (p && p.catch) {
			p.catch(function (e) {
				if (e && e.name !== 'AbortError') {
					log('音声の再生が断られました（次の操作でやり直します）: ' + (e.name || e));
				}
			});
		}
	}

	/* ゲーム側の指示（want）に音声ファイルを合わせる */
	function syncAudio() {
		var a = audio;
		if (!a) {
			return;
		}
		if (!(want.kind === 'audio' && want.playing)) {
			clearLoopTimer();
			if (!a.paused) {
				a.pause();
			}
			return;
		}
		if (audioFailed) {
			return;
		}
		/* 鳴らし直す指示（play()）なら頭から */
		if (audioSerial !== want.serial) {
			audioSerial = want.serial;
			clearLoopTimer();
			try {
				a.currentTime = 0;
			} catch (e) {
			}
			tryPlay(a);
			return;
		}
		/* 最後まで鳴った：ループなら間を空けて頭から */
		if (a.ended) {
			if (want.loop && !loopTimer) {
				loopTimer = setTimeout(function () {
					loopTimer = 0;
					if (audio !== a || !(want.kind === 'audio' && want.playing && want.loop)) {
						return;
					}
					try {
						a.currentTime = 0;
					} catch (e) {
					}
					tryPlay(a);
					sync();
				}, AUDIO_LOOP_GAP);
			} else if (!want.loop) {
				clearLoopTimer();
			}
			return;
		}
		/* 止まっていたら（断られていた・間の最中でなければ）鳴らす */
		if (a.paused && !loopTimer) {
			tryPlay(a);
		}
	}

	/*
	 * MIDI を止める（シンセの準備ができていなければ何もしなくてよい）
	 *
	 * spessasynth の seq.paused は、止める指示（pause）への返事が遅れて届くと、鳴っているのに true に
	 * なることがある（pause() は paused が true だと何もしないので、止めそこなう）。MIDI だけの時は
	 * AudioContext ごと止めるので目立たなかったが、音声ファイルに切り替える時は止めそこなうと
	 * 両方鳴ってしまうので、自分で覚えた midiActive を見て、paused を空にしてから確実に止める。
	 */
	function pauseMidi() {
		if (!ready || !midiActive) {
			return;
		}
		midiActive = false;
		try {
			seq.pausedTime = undefined;
		} catch (e) {
		}
		seq.pause();
	}
	function stopMidi() {
		if (!ready) {
			return;
		}
		pauseMidi();
		synth.stopAll(true);
	}

	/* 今鳴らすものがあるか */
	function wantSound() {
		return want.playing && (want.kind === 'audio' ? !!audio : !!want.data);
	}

	/* ゲーム側の指示（want）にシンセ・音声ファイルを合わせる */
	function sync() {
		if (!ctx || !gain) {
			return;
		}
		var isAudio = (want.kind === 'audio');
		/* もう片方は必ず止める */
		if (isAudio) {
			stopMidi();
			loadedSerial = -1;
		} else {
			disposeAudio();
		}
		/* 音声の準備ができていれば、シンセの準備を待たずに音量を合わせる */
		if (isAudio || ready) {
			gain.gain.setTargetAtTime(want.volume * (isAudio ? AUDIO_GAIN : MASTER_GAIN), ctx.currentTime, 0.02);
		}
		if (isAudio) {
			syncAudio();
		}
		if (!wantSound()) {
			if (!isAudio && ready) {
				stopMidi();
			}
			/* 鳴らしていない間もシンセは無音を作り続けて CPU を使うので、AudioContext ごと止めておく */
			if ((ready || failed) && ctx.state === 'running') {
				ctx.suspend().catch(function () {});
			}
			return;
		}
		/* 止めていたら動かし直す（操作の外では断られることがあるが、その時は次の操作で unlock() が動かす） */
		if (ctx.state !== 'running') {
			ctx.resume().catch(function () {});
		}
		if (isAudio || !ready) {
			return;
		}
		if (loadedSerial !== want.serial) {
			loadedSerial = want.serial;
			synth.stopAll(true);
			/* 送ると向こうに移される（detach される）ので写しを渡す */
			seq.loadNewSongList([{ binary: want.data.slice(0), fileName: 'bgm.mid' }]);
		}
		/* Infinity でずっと繰り返す（-1 は効かない） */
		seq.loopCount = want.loop ? Infinity : 0;
		if (seq.paused || !midiActive) {
			seq.play();
		}
		midiActive = true;
	}

	/* 操作されたら音を出せるようにする（イベントの中ですぐに呼ぶこと） */
	var silentDone = false;
	function unlock() {
		start();
		/* 音声ファイルの play() が断られて止まっていたら、この操作の中でやり直す */
		if (want.kind === 'audio' && want.playing && audio && audio.paused && !loopTimer && !audio.ended && !audioFailed) {
			tryPlay(audio);
		}
		if (!ctx || ctx.state === 'running') {
			return;
		}
		/* BGM を鳴らしていないので止めている時は、そのままにしておく */
		if (silentDone && (ready || failed) && !wantSound()) {
			return;
		}
		ctx.resume().catch(function () {});
		/* iPhone などは操作の中で何か鳴らすと動き出すので、無音を一瞬鳴らしておく */
		if (!silentDone) {
			silentDone = true;
			try {
				var src = ctx.createBufferSource();
				src.buffer = ctx.createBuffer(1, 1, ctx.sampleRate);
				src.connect(ctx.destination);
				src.start(0);
			} catch (e) {
			}
		}
	}
	/* タッチでは touchstart・pointerdown は「操作」と見なされないので、離した時のイベントも見る */
	['keydown', 'mousedown', 'pointerdown', 'pointerup', 'touchend', 'click'].forEach(function (type) {
		window.addEventListener(type, unlock, true);
	});

	/* ページを開いたらすぐに読み込みを始める（鳴らせるのは操作されてから） */
	start();

	window.SboBgm = {
		/* MIDI データを開く（Uint8Array。中身は写して持つ） */
		open: function (bytes) {
			want.kind = 'midi';
			want.data = prepareLoop(bytes.slice().buffer);
			want.serial++;
			want.playing = false;
			start();
			sync();		/* 音声ファイルを開いていたら、ここで閉じる */
			return true;
		},
		/* 音声ファイル（bgm/audio/ の名前）を開く。読み込みはここで始める */
		openAudio: function (name) {
			want.kind = 'audio';
			want.data = null;
			want.serial++;
			want.playing = false;
			start();
			stopMidi();
			openAudioFile(String(name));
			sync();
			return !!audio;
		},
		close: function () {
			want.data = null;
			want.playing = false;
			disposeAudio();
			sync();
		},
		play: function () {
			if (want.kind === 'audio' ? !audio : !want.data) {
				return false;
			}
			want.playing = true;
			/* 頭から鳴らし直す */
			want.serial++;
			sync();
			return true;
		},
		stop: function () {
			want.playing = false;
			sync();
		},
		setLoop: function (loop) {
			want.loop = !!loop;
			sync();
		},
		/* 音量（0.0 〜） */
		setVolume: function (vol) {
			want.volume = Math.max(0, vol);
			sync();
		},
		isPlaying: function () {
			if (want.kind === 'audio') {
				return want.playing && !!audio && !audioFailed;
			}
			return want.playing && !failed;
		},
		/* 動画録画用：BGM の出口（音量を掛けた後）を返す。まだ作っていなければ null */
		recordTap: function () {
			return ctx && gain ? { ctx: ctx, node: gain } : null;
		},
		/* 確認用（開発者ツールのコンソールから呼ぶ） */
		status: function () {
			return {
				ready: ready,
				failed: failed,
				context: ctx ? ctx.state : 'none',
				playing: want.playing,
				loop: want.loop,
				volume: want.volume,
				time: seq ? seq.currentTime : 0,
				duration: seq ? seq.duration : 0,
				kind: want.kind,
				midiPaused: seq ? seq.paused : null,
				audio: audio ? {
					name: audioName,
					failed: audioFailed,
					paused: audio.paused,
					ended: audio.ended,
					time: audio.currentTime,
					duration: audio.duration,
					readyState: audio.readyState,
					waitingLoop: !!loopTimer
				} : null
			};
		}
	};
})();
