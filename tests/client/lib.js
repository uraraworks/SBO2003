'use strict';
// 静的配信 + ブラウザ起動の共通処理
const http = require('http');
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const MIME = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.wasm': 'application/wasm',
	'.data': 'application/octet-stream', '.css': 'text/css', '.json': 'application/json', '.png': 'image/png' };

function serve(dir) {
	return new Promise((resolve) => {
		const srv = http.createServer((req, res) => {
			let p = decodeURIComponent(req.url.split('?')[0]);
			if (p === '/') p = '/sbocli.html';
			const f = path.join(dir, p);
			if (!f.startsWith(dir) || !fs.existsSync(f) || fs.statSync(f).isDirectory()) {
				res.writeHead(404); res.end(); return;
			}
			res.writeHead(200, { 'Content-Type': MIME[path.extname(f)] || 'application/octet-stream' });
			fs.createReadStream(f).pipe(res);
		});
		srv.listen(0, '127.0.0.1', () => resolve({ srv, port: srv.address().port }));
	});
}

async function launch() {
	// Playwright 同梱の Chromium が無い環境でも動くよう、入っている Google Chrome を使う
	return chromium.launch({ channel: process.env.PW_CHANNEL || 'chrome' });
}

module.exports = { serve, launch };
