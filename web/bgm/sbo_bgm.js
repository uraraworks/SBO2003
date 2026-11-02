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

	var MASTER_GAIN = 3.0;		/* 全体の音量（GeneralUser GS は小さめなので、効果音とつり合うように持ち上げる） */

	/* ゲーム側から見た状態 */
	var want = {
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
	var ready = false;
	var failed = false;
	var starting = null;

	function log(msg) {
		console.log('[bgm] ' + msg);
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

	/* ゲーム側の指示（want）にシンセを合わせる */
	function sync() {
		if (!ready) {
			return;
		}
		gain.gain.setTargetAtTime(want.volume * MASTER_GAIN, ctx.currentTime, 0.02);
		if (!want.playing || !want.data) {
			if (!seq.paused) {
				seq.pause();
			}
			synth.stopAll(true);
			/* 鳴らしていない間もシンセは無音を作り続けて CPU を使うので、AudioContext ごと止めておく */
			if (ctx.state === 'running') {
				ctx.suspend().catch(function () {});
			}
			return;
		}
		/* 止めていたら動かし直す（操作の外では断られることがあるが、その時は次の操作で unlock() が動かす） */
		if (ctx.state !== 'running') {
			ctx.resume().catch(function () {});
		}
		if (loadedSerial !== want.serial) {
			loadedSerial = want.serial;
			synth.stopAll(true);
			/* 送ると向こうに移される（detach される）ので写しを渡す */
			seq.loadNewSongList([{ binary: want.data.slice(0), fileName: 'bgm.mid' }]);
		}
		/* Infinity でずっと繰り返す（-1 は効かない） */
		seq.loopCount = want.loop ? Infinity : 0;
		if (seq.paused) {
			seq.play();
		}
	}

	/* 操作されたら音を出せるようにする（イベントの中ですぐに呼ぶこと） */
	var silentDone = false;
	function unlock() {
		start();
		if (!ctx || ctx.state === 'running') {
			return;
		}
		/* BGM を鳴らしていないので止めている時は、そのままにしておく */
		if (silentDone && ready && !(want.playing && want.data)) {
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
			want.data = bytes.slice().buffer;
			want.serial++;
			want.playing = false;
			start();
			sync();
			return true;
		},
		close: function () {
			want.data = null;
			want.playing = false;
			sync();
		},
		play: function () {
			if (!want.data) {
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
			return want.playing && !failed;
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
				duration: seq ? seq.duration : 0
			};
		}
	};
})();
