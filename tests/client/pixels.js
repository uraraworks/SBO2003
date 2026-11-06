'use strict';
// 画素比較テスト
//   node pixels.js <dist フォルダ> [--record]
//   --record … 結果を golden.json に書く（修正前のビルドで取る）
//   無し     … golden.json と比べる。ついでに「差分転送 == 全画面転送」の自己検査（?verify=1）も確認する
// 仮想時計（page.clock）で時刻とアニメを固定するので、同じビルドなら毎回同じ画素になる。
const fs = require('fs');
const path = require('path');
const { serve, launch } = require('./lib');

const GOLDEN = path.join(__dirname, 'golden.json');

// canvas 全体の画素ハッシュ（FNV-1a）
const HASH_JS = () => {
	const c = document.querySelector('canvas');
	const ctx = c.getContext('2d');
	const d = ctx.getImageData(0, 0, c.width, c.height).data;
	const u = new Uint32Array(d.buffer);
	let h = 2166136261 >>> 0;
	for (let i = 0; i < u.length; i++) {
		h = Math.imul(h ^ u[i], 16777619) >>> 0;
	}
	return c.width + 'x' + c.height + ':' + h.toString(16);
};

async function step(page, ms) {
	await page.clock.runFor(ms);
	// wasm の描画は同期なので、少し待てば JS 側の後処理も終わっている
	await page.waitForTimeout(5);
}

async function runScenario(page, name, hashes) {
	const rec = async (label) => { hashes[name + '/' + label] = await page.evaluate(HASH_JS); };
	if (name === 'title-login') {
		// ロゴ → タイトル（フェードを含む）。500ms ごと
		for (let i = 0; i < 24; i++) {
			await step(page, 496);
			await rec('t' + i);
		}
		await page.keyboard.press('Enter');
		// ログイン画面。流れる文字とアニメを 100ms ごと
		for (let i = 0; i < 60; i++) {
			await step(page, 96);
			await rec('login' + i);
		}
	}
}

async function once(dist, record) {
	const { srv, port } = await serve(dist);
	const browser = await launch();
	const all = {};
	let verifyFail = 0;
	for (const name of ['title-login']) {
		const page = await browser.newPage({ viewport: { width: 900, height: 900 } });
		let started = false;
		page.on('console', (m) => {
			if (/\[sbo\] 起動します/.test(m.text())) started = true;
			if (/verify/.test(m.text())) console.log("  " + m.text());
		});
		await page.clock.install({ time: 0 });
		await page.clock.pauseAt(1000);
		await page.goto(`http://127.0.0.1:${port}/sbocli.html${process.env.NOVERIFY ? "" : "?verify=1"}`);
		// 起動するまでは 16ms（rAF の間隔）ずつ時計を進める。起動の時刻が 16ms の倍数にそろい、描画フレームとの位相が毎回同じになる
		for (let i = 0; i < 4000 && !started; i++) {
			await page.clock.runFor(16);
			await page.waitForTimeout(5);
		}
		if (!started) throw new Error('起動しませんでした');
		await runScenario(page, name, all);
		const bad = await page.evaluate(() => (window.Module && window.Module.sboVerifyFail) || 0);
		verifyFail += bad;
		await page.close();
	}
	await browser.close();
	srv.close();
	if (record) {
		fs.writeFileSync(GOLDEN, JSON.stringify(all, null, 1));
		console.log('golden を記録: ' + Object.keys(all).length + ' 点');
		return 0;
	}
	const gold = JSON.parse(fs.readFileSync(GOLDEN, 'utf8'));
	let diff = 0;
	for (const k of Object.keys(gold)) {
		if (all[k] !== gold[k]) { diff++; if (diff <= 10) console.log('  不一致 ' + k + ' ' + gold[k] + ' → ' + all[k]); }
	}
	console.log(`画素比較: ${Object.keys(gold).length} 点中 不一致 ${diff} / 自己検査の失敗 ${verifyFail}`);
	return diff || verifyFail ? 1 : 0;
}

// 実行環境の負荷で時々だけ位相がずれることがあるので、最大 3 回まで再試行する
// （ビルドに本当の差があれば 3 回とも不一致になる）
(async () => {
	const dist = path.resolve(process.argv[2]);
	const record = process.argv.includes('--record');
	let rc = 1;
	for (let i = 0; i < 3 && rc; i++) {
		rc = await once(dist, record);
	}
	process.exit(rc);
})();
