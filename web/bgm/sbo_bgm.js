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
 *
 * audio 要素は 1 つだけ作って使い回す（曲を替える時は src を替える）
 * Safari は「ユーザー操作の中で一度も再生されていない media 要素」の play() を断る。
 * 曲ごとに new Audio() すると、操作の外（マップ移動でサーバーの返事を受けた後）で
 * 曲が変わるたびに未許可の要素になって鳴らなかった。WebKit では一度操作の中で
 * play() した要素は、src を替えても操作の外から play() できるので、
 * 要素と MediaElementAudioSourceNode（要素ごとに 1 回しか作れない）は最初に 1 回だけ作って
 * gain につなぎっぱなしにし、最初の操作の中で無音を一度 play() して許可をもらう。
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
	var audio = null;			/* 使い回す audio 要素（最初に 1 回だけ作る） */
	var audioSrc = null;		/* その MediaElementAudioSourceNode（gain につなぎっぱなし） */
	var audioName = null;		/* 今開いている音声の名前（開いていなければ null） */
	var audioGen = 0;			/* 開く・閉じるたびに増やす。古い曲のイベントや遅れて来た返事を見分ける */
	var audioUnlocked = false;	/* 要素がユーザー操作の中で一度 play() できた（許可をもらった） */
	var silentBusy = false;		/* 許可をもらうための無音を鳴らしている最中 */
	var silentSeq = 0;			/* その無音の番号 */
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

		/* AudioWorklet が無い環境（Safari 14.0 以前など）では MIDI だけ諦める（音声 BGM は鳴らせる） */
		if (!ctx.audioWorklet || typeof ctx.audioWorklet.addModule !== 'function') {
			failed = true;
			log('この環境では AudioWorklet が使えないので MIDI は鳴りません（音声 BGM は鳴ります）');
			starting = Promise.resolve();
			return starting;
		}

		starting = Promise.all([
			import(url('spessasynth.js')),
			fetch(url('GeneralUserGS.sf3')).then(function (res) {
				if (!res.ok) {
					throw new Error('サウンドフォントを読み込めませんでした（' + res.status + '）');
				}
				return res.arrayBuffer();
			}),
			new Promise(function (resolve) {
				resolve(ctx.audioWorklet.addModule(url('spessasynth_processor.min.js')));		/* 同期で例外が出ても Promise の失敗にする */
			})
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

	/* 音声ファイルを閉じる（再生を止めて src を外す。要素とつなぎは捨てない） */
	function clearLoopTimer() {
		if (loopTimer) {
			clearTimeout(loopTimer);
			loopTimer = 0;
		}
	}
	function resetAudioSrc(a) {
		try {
			a.pause();
			a.removeAttribute('src');
			a.load();		/* 読み込み中の通信もやめる */
		} catch (e) {
		}
	}
	function disposeAudio() {
		/* 何も開いていなければ何もしない（許可をもらう無音の再生を邪魔しない） */
		if (audioName === null) {
			return;
		}
		clearLoopTimer();
		audioGen++;
		if (audio) {
			resetAudioSrc(audio);
		}
		audioName = null;
		audioFailed = false;
		audioSerial = -1;
	}

	/* audio 要素と MediaElementAudioSourceNode を作る（1 回だけ。作れなければ false） */
	function ensureAudio() {
		if (audio) {
			return true;
		}
		if (!ctx || !gain) {
			return false;
		}
		var a = new Audio();
		a.preload = 'auto';
		a.loop = false;		/* ループは ended から間を空けて自分でやる */
		/* リスナーは 1 回だけ付ける。曲を替えた後に遅れて来た古い曲のイベントは、状態を見て無視する */
		a.addEventListener('ended', function () {
			if (audioName === null || !a.ended) {
				return;
			}
			log('音声が最後まで鳴りました: ' + audioName);
			syncAudio();
		});
		a.addEventListener('error', function () {
			if (audioName === null || !a.error) {
				return;
			}
			audioFailed = true;
			log('音声を読み込めませんでした: ' + audioName + '（' + a.error.code + ' ' + a.error.message + '）');
		});
		try {
			audioSrc = ctx.createMediaElementSource(a);
			audioSrc.connect(gain);
		} catch (e) {
			log('音声の出口を作れませんでした: ' + (e && e.message ? e.message : e));
			audioSrc = null;
			return false;
		}
		audio = a;
		return true;
	}

	/* 音声ファイルを開く（読み込みはここで始めておく。鳴らすのは play() から） */
	function openAudioFile(name) {
		disposeAudio();
		/* 許可をもらう無音の再生の最中でも、本物の曲を優先する（無音側の後始末は番号が合わないので何もしない） */
		silentBusy = false;
		silentSeq++;
		audioGen++;
		if (!ensureAudio()) {
			audioFailed = true;
			log('AudioContext が無いので音声 BGM は鳴りません: ' + name);
			return;
		}
		audioName = name;
		audioFailed = false;
		audioSerial = -1;
		audio.src = url('audio/' + name);
		log('音声を開きます: ' + audio.src);
	}

	/* 許可をもらうための無音の WAV（8kHz・モノラル・16bit・0.1 秒）を data: URI で作る */
	var silentUri = null;
	function makeSilentUri() {
		if (silentUri) {
			return silentUri;
		}
		var samples = 800;
		var bytes = new Uint8Array(44 + samples * 2);		/* 音のデータは 0 のまま（無音） */
		function str(p, t) {
			for (var i = 0; i < t.length; i++) {
				bytes[p + i] = t.charCodeAt(i);
			}
		}
		function u32(p, v) {
			bytes[p] = v & 0xff;
			bytes[p + 1] = (v >>> 8) & 0xff;
			bytes[p + 2] = (v >>> 16) & 0xff;
			bytes[p + 3] = (v >>> 24) & 0xff;
		}
		function u16(p, v) {
			bytes[p] = v & 0xff;
			bytes[p + 1] = (v >>> 8) & 0xff;
		}
		str(0, 'RIFF');
		u32(4, 36 + samples * 2);
		str(8, 'WAVE');
		str(12, 'fmt ');
		u32(16, 16);
		u16(20, 1);			/* PCM */
		u16(22, 1);			/* モノラル */
		u32(24, 8000);		/* 8kHz */
		u32(28, 16000);		/* 1 秒あたりのバイト数 */
		u16(32, 2);
		u16(34, 16);
		str(36, 'data');
		u32(40, samples * 2);
		var bin = '';
		for (var i = 0; i < bytes.length; i++) {
			bin += String.fromCharCode(bytes[i]);
		}
		silentUri = 'data:audio/wav;base64,' + btoa(bin);
		return silentUri;
	}

	/*
	 * 操作の中で、audio 要素に無音を一度 play() させて許可をもらう（イベントの中ですぐに呼ぶこと）
	 * 音声 BGM を開いている時は、その曲の play() のやり直しが許可も兼ねるので何もしない。
	 * 要素は gain につながっているので、無音を鳴らしても音は出ない。
	 */
	function unlockAudioElement() {
		if (audioUnlocked || silentBusy || audioName !== null || !ensureAudio()) {
			return;
		}
		var a = audio;
		var id = ++silentSeq;
		silentBusy = true;
		function finish(ok) {
			if (ok) {
				audioUnlocked = true;
			}
			/* 無音の最中に本物の曲が開かれた（番号が変わった）なら、要素は本物のものなので触らない */
			if (!silentBusy || id !== silentSeq) {
				return;
			}
			silentBusy = false;
			resetAudioSrc(a);
			log(ok ? '音声の再生を許可されました' : '音声の許可をもらえませんでした（次の操作でやり直します）');
		}
		var p;
		try {
			a.src = makeSilentUri();
			p = a.play();
		} catch (e) {
			finish(false);
			return;
		}
		if (p && p.then) {
			p.then(function () {
				finish(true);
			}, function () {
				finish(false);
			});
		} else {
			finish(true);
		}
	}

	/* play() が断られたら（操作の前など）何もしない。次の操作（unlock）か sync で、止まっていたらやり直す */
	function tryPlay(a) {
		var p;
		try {
			p = a.play();
		} catch (e) {
			return;
		}
		if (p && p.then) {
			var gen = audioGen;
			p.then(function () {
				if (gen === audioGen) {
					audioUnlocked = true;		/* 本物の曲が鳴った＝許可をもらえた */
				}
			}, function (e) {
				if (e && e.name !== 'AbortError') {
					log('音声の再生が断られました（次の操作でやり直します）: ' + (e.name || e));
				}
			});
		}
	}

	/* ゲーム側の指示（want）に音声ファイルを合わせる */
	function syncAudio() {
		var a = audio;
		if (!a || audioName === null) {
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
				var gen = audioGen;
				loopTimer = setTimeout(function () {
					loopTimer = 0;
					if (audioGen !== gen || !(want.kind === 'audio' && want.playing && want.loop)) {
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
		return want.playing && (want.kind === 'audio' ? audioName !== null : !!want.data);
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
		/* 音声ファイルの play() が断られて止まっていたら、この操作の中でやり直す（これで許可も付く） */
		if (want.kind === 'audio' && want.playing && audioName !== null && audio.paused && !loopTimer && !audio.ended && !audioFailed) {
			tryPlay(audio);
		}
		/* まだ許可をもらっていなければ、無音を一度鳴らして audio 要素の許可をもらう */
		unlockAudioElement();
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
			return audioName !== null;
		},
		close: function () {
			want.data = null;
			want.playing = false;
			disposeAudio();
			sync();
		},
		play: function () {
			if (want.kind === 'audio' ? audioName === null : !want.data) {
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
				return want.playing && audioName !== null && !audioFailed;
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
				audioUnlocked: audioUnlocked,
				audio: (audio && audioName !== null) ? {
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
